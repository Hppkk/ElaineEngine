#include "ElainePrecompiledHeader.h"
#include "render/ElaineRenderDocCapture.h"
#include "RenderDoc/ElaineRenderDocCaptureInternal.h"
#include "RenderDoc/ElaineRenderDocCaptureState.h"
#include <charconv>
#include <filesystem>
#include <iostream>
#include <fstream>

#if ELAINE_WITH_RENDERDOC && defined(_WIN32)
#include "RenderDoc/renderdoc_app.h"
#include <Windows.h>
#endif

namespace Elaine
{
    namespace
    {
        RenderDocInternal::CaptureState State;
#if ELAINE_WITH_RENDERDOC && defined(_WIN32)
        // Retain the module until process exit: Vulkan loader dispatch tables may still reference it.
        bool Initialized = false;

        class AppDriver final : public RenderDocInternal::Driver
        {
        public:
            explicit AppDriver(RENDERDOC_API_1_6_0* InAPI) : API(InAPI) {}
            bool IsCapturing() override { return API->IsFrameCapturing() != 0; }
            bool Start(void* Device) override
            { API->StartFrameCapture(Device, nullptr); return IsCapturing(); }
            bool End(void* Device) override { return API->EndFrameCapture(Device, nullptr) != 0; }
            void Discard(void* Device) override { API->DiscardFrameCapture(Device, nullptr); }
            uint32_t CaptureCount() override { return API->GetNumCaptures(); }
            std::string CapturePath(uint32_t Index) override
            {
                uint32_t Length = 0;
                if (!API->GetCapture(Index, nullptr, &Length, nullptr) || !Length) return {};
                std::string Path(static_cast<size_t>(Length) + 1, '\0');
                if (!API->GetCapture(Index, Path.data(), &Length, nullptr)) return {};
                Path.resize(std::char_traits<char>::length(Path.c_str())); return Path;
            }
            void SetTitle(const std::string& Title) override { API->SetCaptureTitle(Title.c_str()); }
            bool Open(const std::string& Path) override
            { const auto Argument = "\"" + Path + "\""; return API->LaunchReplayUI(0, Argument.c_str()) != 0; }
        private:
            RENDERDOC_API_1_6_0* API;
        };

        std::filesystem::path FindInstall()
        {
            // The installer registers qrenderdoc for .rdc files. No registry writes are needed.
            wchar_t Value[32768]; DWORD Bytes = sizeof(Value);
            if (RegGetValueW(HKEY_CLASSES_ROOT, L"RenderDoc.RDCCapture.1\\DefaultIcon", nullptr,
                RRF_RT_REG_SZ, nullptr, Value, &Bytes) == ERROR_SUCCESS)
            {
                std::wstring Path(Value);
                if (!Path.empty() && Path.front() == L'"') Path = Path.substr(1, Path.find(L'"', 1) - 1);
                else if (auto Comma = Path.rfind(L','); Comma != std::wstring::npos) Path.resize(Comma);
                auto DLL = std::filesystem::path(Path).parent_path() / L"renderdoc.dll";
                if (std::filesystem::is_regular_file(DLL)) return DLL;
            }
            wchar_t ProgramFiles[32768];
            if (GetEnvironmentVariableW(L"ProgramFiles", ProgramFiles, 32768))
            {
                auto DLL = std::filesystem::path(ProgramFiles) / L"RenderDoc/renderdoc.dll";
                if (std::filesystem::is_regular_file(DLL)) return DLL;
            }
            return {};
        }
        std::filesystem::path DefaultOutput()
        {
            wchar_t File[32768]; const auto Length = GetModuleFileNameW(nullptr, File, 32768);
            if (!Length || Length >= 32768) throw std::runtime_error("Cannot locate executable for RenderDoc output");
            auto Directory = std::filesystem::path(File).parent_path();
            for (auto Parent = Directory; !Parent.empty(); Parent = Parent.parent_path())
            {
                if (std::filesystem::is_directory(Parent / "Contents")) return Parent / "Intermediate/RenderDoc";
                if (Parent == Parent.parent_path()) break;
            }
            return Directory / "Intermediate/RenderDoc";
        }
#endif
    }

    bool RenderDocCapture::InitializeFromCommandLine(int Argc, char** Argv, std::string& Error)
    {
        bool Requested = false; uint64_t AutoFrame = 0;
        std::string DLL, Output;
        for (int i = 1; i < Argc; ++i)
        {
            const std::string Arg(Argv[i]);
            if (Arg == "--renderdoc") Requested = true;
            else if (Arg.rfind("--renderdoc=", 0) == 0)
            { Requested = true; DLL = Arg.substr(12); if (DLL.empty()) { Error = "Empty RenderDoc DLL path"; return false; } }
            else if (Arg.rfind("--renderdoc-output=", 0) == 0)
            { Output = Arg.substr(19); if (Output.empty()) { Error = "Empty RenderDoc output path"; return false; } }
            else if (Arg.rfind("--renderdoc-capture-frame=", 0) == 0)
            {
                const auto Number = Arg.substr(26);
                const auto Parsed = std::from_chars(Number.data(), Number.data() + Number.size(), AutoFrame);
                if (Parsed.ec != std::errc{} || Parsed.ptr != Number.data() + Number.size() || !AutoFrame)
                { Error = "RenderDoc capture frame must be a positive integer"; return false; }
            }
        }
#if ELAINE_WITH_RENDERDOC && defined(_WIN32)
        if (Initialized) { Error = "RenderDoc initialization must run once before graphics initialization"; return false; }
        Initialized = true;
        try
        {
            HMODULE Module = GetModuleHandleW(L"renderdoc.dll");
            if (!Module && Requested)
            {
                auto Path = DLL.empty() ? FindInstall() : std::filesystem::u8path(DLL);
                if (Path.empty() || !Path.is_absolute() || !std::filesystem::is_regular_file(Path))
                    throw std::runtime_error("RenderDoc DLL was not found; use --renderdoc=<absolute DLL path>");
                // The matching layer manifest is required even when the DLL itself loads correctly.
                const auto Manifest = Path.parent_path() / "renderdoc.json";
                if (!std::filesystem::is_regular_file(Manifest)) throw std::runtime_error("RenderDoc Vulkan layer manifest is missing");
                auto Json = JsonCpp::parse(std::ifstream(Manifest));
                const auto& Layer = Json.at("layer");
                if (Layer.at("name") != "VK_LAYER_RENDERDOC_Capture") throw std::runtime_error("Unexpected RenderDoc Vulkan layer manifest");
                // Add only to this process's layer search path; preserve other validation/tool layers.
                wchar_t Existing[32768]{};
                GetEnvironmentVariableW(L"VK_ADD_LAYER_PATH", Existing, 32768);
                auto Search = Path.parent_path().wstring();
                if (*Existing) Search += std::wstring(L";") + Existing;
                if (!SetEnvironmentVariableW(L"VK_ADD_LAYER_PATH", Search.c_str())) throw std::runtime_error("Cannot configure Vulkan layer search path");
                // Follow this installation's implicit-layer activation rather than guessing its version.
                for (auto It = Layer.at("enable_environment").begin(); It != Layer.at("enable_environment").end(); ++It)
                    if (!SetEnvironmentVariableA(It.key().c_str(), It.value().get<std::string>().c_str())) throw std::runtime_error("Cannot enable RenderDoc Vulkan layer");
                Module = LoadLibraryExW(Path.c_str(), nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
                if (!Module) throw std::runtime_error("Loading RenderDoc failed: Win32 " + std::to_string(GetLastError()));
            }
            if (!Module)
            {
                if (AutoFrame || !Output.empty()) throw std::runtime_error("RenderDoc capture options require injection or --renderdoc");
                return true;
            }
            auto GetAPI = reinterpret_cast<pRENDERDOC_GetAPI>(GetProcAddress(Module, "RENDERDOC_GetAPI"));
            RENDERDOC_API_1_6_0* API = nullptr;
            if (!GetAPI || !GetAPI(eRENDERDOC_API_Version_1_6_0, reinterpret_cast<void**>(&API)))
                throw std::runtime_error("RenderDoc application API 1.6 is unavailable");
            auto Directory = Output.empty() ? DefaultOutput() : std::filesystem::absolute(std::filesystem::u8path(Output));
            std::filesystem::create_directories(Directory);
            const auto Prefix = (Directory / "ElaineVulkan").u8string();
            API->SetCaptureFilePathTemplate(reinterpret_cast<const char*>(Prefix.c_str()));
            // Engine-owned explicit capture avoids F12 selecting DX11's Present frame.
            API->SetCaptureKeys(nullptr, 0);
            State.Connect(std::make_shared<AppDriver>(API), AutoFrame);
            std::cout << "RenderDoc application API connected; Vulkan layer will be checked at instance creation\n";
            return true;
        }
        catch (const std::exception& Exception)
        { Error = Exception.what(); State.Fail(Error); return false; }
#else
        if (Requested || AutoFrame || !Output.empty())
        { Error = "RenderDoc support is disabled in this build (requires a Debug configuration and ELAINE_WITH_RENDERDOC=ON)"; return false; }
        return true;
#endif
    }
    RenderDocCaptureStatus RenderDocCapture::GetStatus() { return State.Snapshot(); }
    bool RenderDocCapture::RequestCapture() { return State.Request(); }
    bool RenderDocCapture::OpenLastCapture() { return State.Open(); }
    void RenderDocCapture::Shutdown() { State.Disconnect(); }

    namespace RenderDocInternal
    {
#if ELAINE_WITH_RENDERDOC && defined(_WIN32)
        bool IsConnected() { return State.Snapshot().Connected; }
        void VulkanReady() { State.Ready(); }
        void VulkanUnavailable(const std::string& Reason) { State.Fail(Reason); }
        void BeginFrame(void* Device) { State.Begin(Device); }
        void EndFrame()
        {
            const auto Before = State.Snapshot().Phase; State.End();
            if (Before == RenderDocCapturePhase::Capturing)
            {
                const auto Result = State.Snapshot();
                if (Result.Phase == RenderDocCapturePhase::Succeeded) { LOG_INFO("RenderDoc Vulkan frame {}: {}", Result.Frame, Result.CapturePath); }
                else { LOG_ERROR("RenderDoc capture failed: {}", Result.Message); }
            }
        }
        void AbortFrame(const char* Reason) { State.Abort(Reason); }
        void DetachVulkan() { State.Detach(); }
#else
        bool IsConnected() { return false; }
        void VulkanReady() {}
        void VulkanUnavailable(const std::string&) {}
        void BeginFrame(void*) {}
        void EndFrame() {}
        void AbortFrame(const char*) {}
        void DetachVulkan() {}
#endif
    }
}

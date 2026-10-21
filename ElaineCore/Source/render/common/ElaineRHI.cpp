#include "ElainePrecompiledHeader.h"
#include "render/common/ElaineRHI.h"
#include "render/common/ElaineRHICommandList.h"
#include "render/common/ElaineRenderModule.h"
#include "ElaineThreadManager.h"
#include "render/common/ElaineRHICommandContext.h"
#include "ElaineBarrier.h"
#include "TaskGraph/ElaineTaskGraph.h"

namespace Elaine
{
	class _RHIPrivate;

	_RHIPrivate* _GRHI = nullptr;
	EBarrier* G_GfxBarrier = nullptr;
	EBarrier* G_RenderBarrier = nullptr;
	ThreadWrap* G_RHIThread = nullptr;

	static void RHIThreadMain();
    static std::atomic_bool Stopping{false};
    static bool UseRHIThread = true;
    static thread_local bool RenderOwnsContext = false;
    static std::mutex ReleaseMutex;
    static std::vector<RHIResource*> PendingRelease;

    static std::atomic<uint64_t> LastValidationErrorCount{0};
    uint64_t GetRHIValidationErrorCount()
    {
        auto* RHI = GetDynamicRHI();
        return RHI ? RHI->GetValidationErrorCount() : LastValidationErrorCount.load();
    }
    void ReleaseRHIResource(RHIResource* Resource) noexcept
    {
        if (!_GRHI) { delete Resource; return; }
        std::lock_guard<std::mutex> Lock(ReleaseMutex);
        PendingRelease.push_back(Resource);
    }
    void FlushRHIResourceReleases()
    {
        for (;;)
        {
            std::vector<RHIResource*> Batch;
            { std::lock_guard<std::mutex> Lock(ReleaseMutex); Batch.swap(PendingRelease); }
            if (Batch.empty()) break;
            for (auto* Resource : Batch) delete Resource;
        }
    }
    void CheckRHICreationThread()
    {
        if (!ThreadManager::instance()->CheckThread(NamedThread::RenderThread) || (_GRHI && !RenderOwnsContext))
            throw std::logic_error("RHI creation requires the RenderThread synchronization window");
    }
    void CheckRHIContextThread()
    {
        if (!ThreadManager::instance()->CheckThread(NamedThread::RHIThread)) CheckRHICreationThread();
    }
    static void PollGPU()
    {
        if (auto* RHI = GetDynamicRHI()) RHI->ProcessGPUCompletions();
    }


	class _RHIPrivate
	{
	public:
		void _Initilize(const RHI_PARAM_DESC& InDesc)
        {
            mRHIType = InDesc.RHIType;
            mDynamicRHI = CreateDynamicRHI(InDesc.RHIType);
            mDynamicRHI->Initialize(InDesc);
        }

		void _Destroy()
		{
			LastValidationErrorCount = mDynamicRHI->GetValidationErrorCount();
			mDynamicRHI = nullptr;
		}
		ERHI_TYPE GetType() const { return mRHIType; }
		DynamicRHI* GetDynamicRHI() const { return mDynamicRHI; }

	private:
		ERHI_TYPE mRHIType = Vulkan;
		DynamicRHI* mDynamicRHI = nullptr;
	};

	DynamicRHI::~DynamicRHI()
	{

	}



    std::shared_ptr<const RHIShaderCompileResult> DynamicRHI::PrepareShader(const std::string&, const std::string&, EShaderStage)
    {
        throw std::runtime_error("Shader preparation is not implemented by this RHI backend");
    }

    void DynamicRHI::ProcessGPUCompletions()
    {
        FlushRHIResourceReleases();
    }

    bool InitEngineRHI(const RHI_PARAM_DESC& InDesc)
    {
        Stopping = false; UseRHIThread = InDesc.UseRHIThread; LastValidationErrorCount = 0;
        _GRHI = new _RHIPrivate();
        _GRHI->_Initilize(InDesc);
        G_GfxBarrier = new EBarrier();
        G_RenderBarrier = new EBarrier();
        if (UseRHIThread)
            G_RHIThread = ThreadManager::instance()->GetOrCreateThread(NamedThread::RHIThread, RHIThreadMain);
        else G_RenderBarrier->Signal();
        return true;
    }

    bool DestroyEngineRHI()
    {
        auto Destroy = [] {
            CheckRHICreationThread();
            Stopping = true;
            if (G_RHIThread)
            {
                G_GfxBarrier->Signal();
                ThreadManager::instance()->DestroyThread(G_RHIThread); G_RHIThread = nullptr;
            }
            auto* Context = GetDynamicRHI()->GetDefaultCommandContext();
            Context->GetRHICommandListMgr()->Clear();
            Context->RHIWaitIdle();
            FlushRHIResourceReleases();
            _GRHI->_Destroy();
            FlushRHIResourceReleases();
            SAFE_DELETE(_GRHI);
            SAFE_DELETE(G_GfxBarrier); SAFE_DELETE(G_RenderBarrier);
        };
        if (ThreadManager::instance()->CheckThread(NamedThread::RenderThread)) Destroy();
        else
        {
            auto Done = std::make_shared<std::promise<void>>(); auto Ready = Done->get_future();
            TaskGraph::TaskGraph::instance()->CreateAndDispatchWhenReady([Destroy, Done] {
                try { Destroy(); Done->set_value(); } catch (...) { Done->set_exception(std::current_exception()); }
            }, NamedThread::RenderThread);
            Ready.get();
        }
        return true;
    }

    static void RHIThreadMain()
    {
        G_RenderBarrier->Signal();
        while (true)
        {
            G_GfxBarrier->Wait();
            if (Stopping) break;
            auto* Manager = GetDynamicRHI()->GetDefaultCommandContext()->GetRHICommandListMgr();
            try { Manager->SwapCommands(); Manager->ExecuteCommands(); }
            catch (const std::exception& Error) { LOG_ERROR("RHI batch failed: {}", Error.what()); }
            PollGPU();
            G_RenderBarrier->Signal();
        }
    }

	bool RecordRHICommand(EM_RHICommand InCmd, void* InRHIResourceHandle)
	{
		
		return true;
	}

    void WaitForRHIThread_Gfx() { G_GfxBarrier->Wait(); }

    void NotifyForRHIThread_Gfx()
    {
        RenderOwnsContext = false;
        if (!_GRHI) return;
        if (UseRHIThread) G_GfxBarrier->Signal();
        else
        {
            RenderOwnsContext = true;
            auto* Manager = GetDynamicRHI()->GetDefaultCommandContext()->GetRHICommandListMgr();
            try { Manager->SwapCommands(); Manager->ExecuteCommands(); }
            catch (const std::exception& Error) { LOG_ERROR("RHI batch failed: {}", Error.what()); }
            PollGPU();
            RenderOwnsContext = false;
            G_RenderBarrier->Signal();
        }
    }
    void WaitForRenderThread_Gfx()
    {
        if (_GRHI) G_RenderBarrier->Wait();
        RenderOwnsContext = true;
    }
    void NotifyForRenderThread_Gfx() { if (G_RenderBarrier) G_RenderBarrier->Signal(); }

	DynamicRHI* GetDynamicRHI()
	{
		return _GRHI ? _GRHI->GetDynamicRHI() : nullptr;
	}
}
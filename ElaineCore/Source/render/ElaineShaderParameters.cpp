#include "ElainePrecompiledHeader.h"
#include "render/ElaineEngineShaderParameters.h"
#include "render/common/ElaineRHICommandList.h"
#include "ElaineHash.h"
#include <algorithm>

namespace Elaine
{
    namespace
    {
        uint32 Align(uint32 Value, uint32 Alignment) { return (Value + Alignment - 1) / Alignment * Alignment; }

        const char* GroupName(RHIResourceGroup Group)
        {
            switch (Group)
            {
            case RHIResourceGroup::PerFrame: return "PerFrame";
            case RHIResourceGroup::PerMaterial: return "PerMaterial";
            case RHIResourceGroup::PerObject: return "PerObject";
            case RHIResourceGroup::PerPass: return "PerPass";
            default: throw std::invalid_argument("Invalid shader parameter group");
            }
        }

        RHIShaderMember ValueMember(const ShaderParameterMember& M, uint32& Offset, bool Std140 = true)
        {
            RHIShaderMember Result;
            Result.Name = M.Name; Result.Type = M.ValueType; Result.Rows = M.Rows;
            Result.Columns = M.Columns; Result.ArrayCount = M.ArrayCount;
            uint32 Alignment = M.Rows == 3 ? 16 : M.Rows * 4;
            uint32 Size = M.Rows * 4;
            if (M.Columns > 1) { Alignment = 16; Result.MatrixStride = 16; Size = 16 * M.Columns; }
            if (M.IsArray) { if (Std140) Alignment = std::max(16u, Alignment); Result.ArrayStride = Align(Size, Alignment); Size = Result.ArrayStride * M.ArrayCount; }
            Result.Offset = Align(Offset, Alignment); Result.Size = Size; Offset = Result.Offset + Size;
            return Result;
        }

        std::string DeclareMembers(const std::vector<ShaderParameterMember>& Members)
        {
            std::string Source;
            for (const auto& M : Members)
            {
                if (M.ResourceType != RHIShaderResourceType::Uniform) continue;
                if (M.Name.empty() || M.GLSLType.empty() || !M.ArrayCount)
                    throw std::invalid_argument("Invalid shader member declaration: " + M.Name);
                Source += "    " + M.GLSLType + " " + M.Name;
                if (M.IsArray) Source += "[" + std::to_string(M.ArrayCount) + "]";
                Source += ";\n";
            }
            return Source;
        }

        struct GeneratedInclude { const ShaderParameterMetadata* Metadata; std::string Source; uint64_t Signature; };
        const std::vector<GeneratedInclude>& Registry()
        {
            // Constructed once, before worker loading starts; no runtime mutation.
            static const auto Includes = [] {
                std::vector<GeneratedInclude> Result;
                const ShaderParameterMetadata* Entries[] = { &ViewShaderParameters::GetMetadata(), &ObjectShaderParameters::GetMetadata(),
                    &LightingFrameShaderParameters::GetMetadata(), &LightingPassShaderParameters::GetMetadata() };
                for (const auto* Entry : Entries)
                {
                    auto Source = GenerateShaderParameterSource(*Entry);
                    Result.push_back({Entry, Source, FNV_1A_HASH_64(Source)});
                }
                return Result;
            }();
            return Includes;
        }

        void CopyParameters(RHIShaderParameterSet& Out, const ShaderParameterMetadata& Metadata, const void* Data)
        {
            if (!Data) throw std::invalid_argument("Shader parameter data is null");
            for (const auto& M : Metadata.Members) M.CopyValue(Data, M, Out, Metadata.BlockName);
        }
    }

    ShaderParameterMember MakeShaderTextureMember(const char* Name, size_t Offset, TextureDimension Dimension)
    {
        ShaderParameterMember M; M.Name = Name; M.CPUOffset = Offset;
        M.ResourceType = RHIShaderResourceType::Texture; M.Dimension = Dimension;
        M.CopyValue = [](const void* Data, const ShaderParameterMember& Member, RHIShaderParameterSet& Out, const std::string&) {
            const auto& Texture = *reinterpret_cast<const RHITextureHandle*>(static_cast<const uint8*>(Data) + Member.CPUOffset);
            Out.SetTexture(Member.Name, Texture.get());
        };
        return M;
    }

    std::string GenerateShaderParameterSource(const ShaderParameterMetadata& Metadata)
    {
        const std::string Group = GroupName(Metadata.Group);
        const std::string Guard = "ELAINE_GENERATED_" + std::to_string(FNV_1A_HASH_64(Metadata.IncludePath));
        std::string Source = "#ifndef " + Guard + "\n#define " + Guard + "\n#include \"ResourceBindings.inl\"\n";
        const auto Uniforms = DeclareMembers(Metadata.Members);
        if (!Uniforms.empty()) Source += "ELAINE_UNIFORM(" + Group + ", Uniform0, " + Metadata.BlockName + ")\n{\n" + Uniforms + "};\n";
        uint32 TextureSlot = 0, BufferSlot = 0;
        std::vector<std::string> Names;
        for (const auto& M : Metadata.Members)
        {
            if (std::find(Names.begin(), Names.end(), M.Name) != Names.end()) throw std::invalid_argument("Duplicate shader member: " + M.Name);
            Names.push_back(M.Name);
            if (M.ResourceType == RHIShaderResourceType::Texture)
            {
                if (TextureSlot >= 16) throw std::invalid_argument("Shader texture slots exhausted: " + M.Name);
                const char* Macro = nullptr;
                switch (M.Dimension)
                {
                case TextureDimension::Texture2D: Macro = "ELAINE_TEXTURE2D"; break;
                case TextureDimension::Texture2DArray: Macro = "ELAINE_TEXTURE2D_ARRAY"; break;
                case TextureDimension::TextureCube: Macro = "ELAINE_TEXTURE_CUBE"; break;
                default: throw std::invalid_argument("Unsupported shader texture dimension: " + M.Name);
                }
                Source += std::string(Macro) + "(" + Group + ", Texture" + std::to_string(TextureSlot++) + ", " + M.Name + ")\n";
            }
            else if (M.ResourceType == RHIShaderResourceType::ReadOnlyBuffer)
            {
                if (BufferSlot >= 8 || M.BufferMembers.empty()) throw std::invalid_argument("Invalid shader buffer: " + M.Name);
                const auto Element = "struct " + M.BufferElementType + "\n{\n" + DeclareMembers(M.BufferMembers) + "};\n";
                const auto ElementGuard = "ELAINE_ELEMENT_" + std::to_string(FNV_1A_HASH_64(Element));
                Source += "#ifndef " + ElementGuard + "\n#define " + ElementGuard + "\n" + Element + "#endif\n";
                Source += "ELAINE_READONLY_BUFFER(" + Group + ", Buffer" + std::to_string(BufferSlot++) + ", " + M.Name + ")\n{\n    " +
                    M.BufferElementType + " " + M.BufferArrayName + "[" + (M.BufferArrayCount ? std::to_string(M.BufferArrayCount) : "") + "];\n};\n";
            }
        }
        return Source + "#endif\n";
    }

    void InitializeShaderParameterRegistry() { (void)Registry(); }

    bool GetGeneratedShaderInclude(const std::string& Path, std::string& Source, uint64_t& Signature)
    {
        for (const auto& Include : Registry()) if (Path == Include.Metadata->IncludePath)
        { Source = Include.Source; Signature = Include.Signature; return true; }
        return false;
    }

    void ValidateShaderParameterLayout(const ShaderParameterMetadata& Metadata, const RHIShaderResourceLayout& Layout)
    {
        if (Layout.Group != static_cast<uint32>(Metadata.Group)) throw std::runtime_error("Shader parameter group mismatch: " + Layout.Name);
        if (Layout.Name == Metadata.BlockName)
        {
            if (Layout.Type != RHIShaderResourceType::Uniform || Layout.Binding != 0) throw std::runtime_error("Shader uniform declaration mismatch: " + Layout.Name);
            uint32 Offset = 0, Count = 0;
            for (const auto& M : Metadata.Members) if (M.ResourceType == RHIShaderResourceType::Uniform)
            {
                const auto Expected = ValueMember(M, Offset);
                auto It = std::find_if(Layout.Members.begin(), Layout.Members.end(), [&](const RHIShaderMember& Member) { return Member.Name == M.Name; });
                // GLSL/SPIR-V represent uniform bool storage as a 32-bit integer.
                auto Actual = It == Layout.Members.end() ? RHIShaderMember{} : *It;
                if (Expected.Type == RHIShaderValueType::Boolean && (Actual.Type == RHIShaderValueType::Integer || Actual.Type == RHIShaderValueType::UnsignedInteger)) Actual.Type = RHIShaderValueType::Boolean;
                if (!(Expected == Actual)) throw std::runtime_error("Shader uniform contract mismatch: " + Layout.Name + "." + M.Name);
                ++Count;
            }
            if (Count != Layout.Members.size()) throw std::runtime_error("Unexpected shader uniform member: " + Layout.Name);
            return;
        }
        uint32 TextureSlot = 8, BufferSlot = 24;
        for (const auto& M : Metadata.Members)
        {
            uint32 Binding = 0;
            if (M.ResourceType == RHIShaderResourceType::Texture) Binding = TextureSlot++;
            else if (M.ResourceType == RHIShaderResourceType::ReadOnlyBuffer) Binding = BufferSlot++;
            else continue;
            if (M.Name != Layout.Name) continue;
            if (Layout.Type != M.ResourceType || Layout.Binding != Binding ||
                (M.ResourceType == RHIShaderResourceType::Texture && (Layout.Dimension != M.Dimension || Layout.IntegerSample || Layout.DepthSample || Layout.Multisampled)))
                throw std::runtime_error("Shader resource contract mismatch: " + Layout.Name);
            if (M.ResourceType == RHIShaderResourceType::ReadOnlyBuffer)
            {
                uint32 Offset = 0, Alignment = 1;
                std::vector<RHIShaderMember> Expected;
                for (const auto& Element : M.BufferMembers)
                {
                    if (Element.ResourceType != RHIShaderResourceType::Uniform)
                        throw std::runtime_error("Buffer elements cannot contain resources: " + Layout.Name + "." + Element.Name);
                    Expected.push_back(ValueMember(Element, Offset, false));
                    Alignment = std::max(Alignment, Element.Columns > 1 || Element.Rows == 3 ? 16u : Element.Rows * 4);
                }
                const uint32 Stride = Align(Offset, Alignment);
                if (Layout.ElementStride != Stride || Layout.Size != Stride * M.BufferArrayCount || Layout.Members != Expected)
                    throw std::runtime_error("Shader buffer element contract mismatch: " + Layout.Name);
            }
            return;
        }
        throw std::runtime_error("Unknown fixed shader resource: " + Layout.Name);
    }

    void ValidateShaderParameterLayouts(const std::vector<RHIShaderResourceLayout>& Layouts, const std::vector<std::string>& GeneratedIncludes)
    {
        for (const auto& Include : Registry())
        {
            const auto& M = *Include.Metadata;
            const bool UsedInclude = std::find(GeneratedIncludes.begin(), GeneratedIncludes.end(), M.IncludePath) != GeneratedIncludes.end();
            for (const auto& Layout : Layouts)
            {
                const bool OwnName = Layout.Name == M.BlockName || std::any_of(M.Members.begin(), M.Members.end(),
                    [&](const ShaderParameterMember& Member) { return Member.ResourceType != RHIShaderResourceType::Uniform && Member.Name == Layout.Name; });
                if (OwnName) ValidateShaderParameterLayout(M, Layout);
                else if (UsedInclude && Layout.Group == static_cast<uint32>(M.Group))
                    throw std::runtime_error("Resource is not declared by the fixed parameter struct: " + Layout.Name + " (" + M.BlockName + ")");
            }
        }
        // No requirement that optimized-away declarations remain in the module.
    }

    void ApplyShaderParameters(RHI_DRAW_RESOURCE_BINDING& Target, const ShaderParameterMetadata& Metadata, const void* Data)
    {
        CopyParameters(Target.GetParameters(Metadata.Group), Metadata, Data);
    }
    void ApplyShaderParameters(RHICommandList& Target, const ShaderParameterMetadata& Metadata, const void* Data)
    {
        if (Metadata.Group != RHIResourceGroup::PerFrame) throw std::invalid_argument("Command-list parameters must belong to a view/frame");
        RHIShaderParameterSet Parameters;
        CopyParameters(Parameters, Metadata, Data);
        Target.SetFrameParameters(Parameters);
    }

    std::vector<uint8> PackShaderUniform(const RHIShaderResourceLayout& Layout, const RHIShaderParameterSet& Parameters)
    {
        auto Block = Parameters.BlockValues.find(Layout.Name);
        const auto& Values = Block == Parameters.BlockValues.end() ? Parameters.Values : Block->second;
        std::vector<uint8> Bytes(Layout.Size, 0);
        for (const auto& Member : Layout.Members)
        {
            const auto QualifiedName = Layout.Name + "." + Member.Name;
            auto It = Values.find(Member.Name);
            if (It == Values.end()) throw std::runtime_error("Required uniform parameter is not set: " + QualifiedName);
            const auto& Value = It->second;
            if (Value.Type != Member.Type && Value.Type != RHIShaderValueType::Raw &&
                !(Value.Type == RHIShaderValueType::Boolean && (Member.Type == RHIShaderValueType::Integer || Member.Type == RHIShaderValueType::UnsignedInteger)))
                throw std::runtime_error("Uniform parameter type mismatch: " + QualifiedName);
            const size_t TightSize = size_t(Member.Rows) * Member.Columns * 4;
            const size_t InputSize = TightSize * Member.ArrayCount;
            if (Value.Bytes.size() != InputSize && !(Member.Type == RHIShaderValueType::Float && Member.Columns == 1 && Member.ArrayCount == 1 && Value.Bytes.size() == 16 && InputSize <= 16))
                throw std::runtime_error("Uniform parameter size mismatch: " + QualifiedName);
            if (Member.Offset > Bytes.size() || Member.Size > Bytes.size() - Member.Offset)
                throw std::runtime_error("Invalid uniform member range: " + QualifiedName);
            for (uint32 Array = 0; Array < Member.ArrayCount; ++Array)
            {
                const size_t Target = Member.Offset + (Member.ArrayStride ? Array * Member.ArrayStride : 0);
                const auto* Source = Value.Bytes.data() + Array * TightSize;
                if (Member.Columns == 1)
                {
                    if (Target + TightSize > Bytes.size()) throw std::runtime_error("Uniform array overflow: " + QualifiedName);
                    std::memcpy(Bytes.data() + Target, Source, TightSize);
                }
                else for (uint32 Row = 0; Row < Member.Rows; ++Row) for (uint32 Col = 0; Col < Member.Columns; ++Col)
                {
                    const size_t Offset = Target + (Member.RowMajor ? Row : Col) * Member.MatrixStride + (Member.RowMajor ? Col : Row) * 4;
                    if (Offset + 4 > Bytes.size()) throw std::runtime_error("Uniform matrix overflow: " + QualifiedName);
                    std::memcpy(Bytes.data() + Offset, Source + (Row * Member.Columns + Col) * 4, 4);
                }
            }
        }
        return Bytes;
    }
}

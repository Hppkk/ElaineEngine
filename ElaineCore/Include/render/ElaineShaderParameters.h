#pragma once
#include "render/common/ElaineRHIProtocol.h"
#include "math/ElaineVector2.h"
#include "math/ElaineVector3.h"
#include "math/ElaineVector4.h"
#include "math/ElaineMatrix4.h"
#include <cstddef>
#include <cstring>
#include <type_traits>
#include <utility>

namespace Elaine
{
    template<int Index> struct ShaderParameterIndex {};

    struct ShaderParameterMember
    {
        std::string Name, GLSLType;
        RHIShaderValueType ValueType = RHIShaderValueType::Float;
        RHIShaderResourceType ResourceType = RHIShaderResourceType::Uniform;
        uint32 Rows = 1, Columns = 1, ArrayCount = 1;
        bool IsArray = false;
        size_t CPUOffset = 0;
        TextureDimension Dimension = TextureDimension::Texture2D;
        std::string BufferElementType, BufferArrayName;
        uint32 BufferArrayCount = 0;
        std::vector<ShaderParameterMember> BufferMembers;
        void (*CopyValue)(const void*, const ShaderParameterMember&, RHIShaderParameterSet&, const std::string&) = nullptr;
    };

    struct ShaderParameterMetadata
    {
        std::string IncludePath, BlockName;
        RHIResourceGroup Group = RHIResourceGroup::Invalid;
        std::vector<ShaderParameterMember> Members;
    };

    template<class T> struct ShaderParameterValueTraits;
#define ELAINE_SHADER_VALUE_TRAITS(CPPType, GLSLName, Kind, RowCount, ColumnCount) \
    template<> struct ShaderParameterValueTraits<CPPType> { \
        static constexpr const char* Name = GLSLName; \
        static constexpr RHIShaderValueType Type = RHIShaderValueType::Kind; \
        static constexpr uint32 Rows = RowCount, Columns = ColumnCount; };
    ELAINE_SHADER_VALUE_TRAITS(float, "float", Float, 1, 1)
    ELAINE_SHADER_VALUE_TRAITS(int32, "int", Integer, 1, 1)
    ELAINE_SHADER_VALUE_TRAITS(uint32, "uint", UnsignedInteger, 1, 1)
    ELAINE_SHADER_VALUE_TRAITS(bool, "bool", Boolean, 1, 1)
    ELAINE_SHADER_VALUE_TRAITS(Vector2, "vec2", Float, 2, 1)
    ELAINE_SHADER_VALUE_TRAITS(Vector3, "vec3", Float, 3, 1)
    ELAINE_SHADER_VALUE_TRAITS(Vector4, "vec4", Float, 4, 1)
    ELAINE_SHADER_VALUE_TRAITS(Matrix4x4, "mat4", Matrix, 4, 4)
#undef ELAINE_SHADER_VALUE_TRAITS

    template<class T>
    ShaderParameterMember MakeShaderValueMember(const char* Name, size_t Offset, uint32 Count = 1, bool IsArray = false)
    {
        using Traits = ShaderParameterValueTraits<T>;
        ShaderParameterMember Member;
        Member.Name = Name; Member.GLSLType = Traits::Name; Member.ValueType = Traits::Type;
        Member.Rows = Traits::Rows; Member.Columns = Traits::Columns;
        Member.CPUOffset = Offset; Member.ArrayCount = Count; Member.IsArray = IsArray;
        Member.CopyValue = [](const void* Data, const ShaderParameterMember& M, RHIShaderParameterSet& Out, const std::string& Block) {
            const auto* Values = reinterpret_cast<const T*>(static_cast<const uint8*>(Data) + M.CPUOffset);
            auto& Value = Out.BlockValues[Block][M.Name];
            Value.Type = M.ValueType;
            const size_t ComponentBytes = size_t(M.Rows) * M.Columns * 4;
            Value.Bytes.resize(ComponentBytes * M.ArrayCount);
            for (uint32 I = 0; I < M.ArrayCount; ++I)
            {
                if constexpr (std::is_same_v<T, bool>)
                {
                    const int32 Bool = Values[I] ? 1 : 0;
                    std::memcpy(Value.Bytes.data() + I * ComponentBytes, &Bool, 4);
                }
                else
                    std::memcpy(Value.Bytes.data() + I * ComponentBytes, &Values[I], ComponentBytes);
            }
        };
        return Member;
    }

    ElaineCoreExport ShaderParameterMember MakeShaderTextureMember(const char* Name, size_t Offset, TextureDimension Dimension);
    template<class Element>
    ShaderParameterMember MakeShaderBufferMember(const char* Name, size_t Offset, const char* Type, const char* ArrayName, uint32 Count)
    {
        ShaderParameterMember Member;
        Member.Name = Name; Member.CPUOffset = Offset; Member.ResourceType = RHIShaderResourceType::ReadOnlyBuffer;
        Member.BufferElementType = Type; Member.BufferArrayName = ArrayName; Member.BufferArrayCount = Count;
        Member.BufferMembers = Element::GetShaderMembers();
        Member.CopyValue = [](const void* Data, const ShaderParameterMember& M, RHIShaderParameterSet& Out, const std::string&) {
            Out.Buffers[M.Name] = *reinterpret_cast<const RHIBufferView*>(static_cast<const uint8*>(Data) + M.CPUOffset);
        };
        return Member;
    }

    template<class T, int First, size_t... I>
    std::vector<ShaderParameterMember> CollectShaderMembers(std::index_sequence<I...>)
    {
        return {T::DescribeShaderMember(ShaderParameterIndex<First + static_cast<int>(I)>{})...};
    }

    ElaineCoreExport std::string GenerateShaderParameterSource(const ShaderParameterMetadata& Metadata);
    ElaineCoreExport void InitializeShaderParameterRegistry();
    ElaineCoreExport bool GetGeneratedShaderInclude(const std::string& Path, std::string& Source, uint64_t& Signature);
    ElaineCoreExport void ValidateShaderParameterLayouts(const std::vector<RHIShaderResourceLayout>& Layouts,
        const std::vector<std::string>& GeneratedIncludes);
    ElaineCoreExport void ValidateShaderParameterLayout(const ShaderParameterMetadata& Metadata, const RHIShaderResourceLayout& Layout);
    ElaineCoreExport void ApplyShaderParameters(RHI_DRAW_RESOURCE_BINDING& Target, const ShaderParameterMetadata& Metadata, const void* Data);
    ElaineCoreExport void ApplyShaderParameters(class RHICommandList& Target, const ShaderParameterMetadata& Metadata, const void* Data);
    ElaineCoreExport std::vector<uint8> PackShaderUniform(const RHIShaderResourceLayout& Layout, const RHIShaderParameterSet& Parameters);

    template<class Target, class T>
    void SetShaderParameters(Target& Out, const T& Parameters)
    {
        ApplyShaderParameters(Out, T::GetMetadata(), &Parameters);
    }

    inline void CopyMatrixRows(Vector4 (&Rows)[4], const Matrix4x4& Matrix)
    {
        for (size_t I = 0; I < 4; ++I)
            Rows[I] = Vector4(Matrix[I][0], Matrix[I][1], Matrix[I][2], Matrix[I][3]);
    }
}

// Each member macro declares its field and metadata together. __COUNTER__ is local
// to the range bounded by BEGIN/END; it is never used as a persistent parameter ID.
#define BEGIN_SHADER_PARAMETER_STRUCT(Type) \
    struct Type { using ThisShaderStruct = Type; \
        static constexpr int FirstShaderMember = __COUNTER__ + 1;
#define SHADER_PARAMETER(Type, Member) \
    Type Member{}; \
    static Elaine::ShaderParameterMember DescribeShaderMember(Elaine::ShaderParameterIndex<__COUNTER__>) { \
        return Elaine::MakeShaderValueMember<Type>(#Member, offsetof(ThisShaderStruct, Member)); }
#define SHADER_PARAMETER_ARRAY(Type, Member, Count) \
    Type Member[Count]{}; \
    static Elaine::ShaderParameterMember DescribeShaderMember(Elaine::ShaderParameterIndex<__COUNTER__>) { \
        return Elaine::MakeShaderValueMember<Type>(#Member, offsetof(ThisShaderStruct, Member), Count, true); }
#define SHADER_PARAMETER_TEXTURE(Dimension, Member) \
    Elaine::RHITextureHandle Member; \
    static Elaine::ShaderParameterMember DescribeShaderMember(Elaine::ShaderParameterIndex<__COUNTER__>) { \
        return Elaine::MakeShaderTextureMember(#Member, offsetof(ThisShaderStruct, Member), Elaine::TextureDimension::Dimension); }
#define SHADER_PARAMETER_READONLY_BUFFER_ARRAY(ElementType, Member, ArrayMember, Count) \
    Elaine::RHIBufferView Member; \
    static Elaine::ShaderParameterMember DescribeShaderMember(Elaine::ShaderParameterIndex<__COUNTER__>) { \
        return Elaine::MakeShaderBufferMember<ElementType>(#Member, offsetof(ThisShaderStruct, Member), #ElementType, #ArrayMember, Count); }
#define SHADER_PARAMETER_READONLY_BUFFER(ElementType, Member) \
    SHADER_PARAMETER_READONLY_BUFFER_ARRAY(ElementType, Member, Data, 0)
#define END_SHADER_PARAMETER_STRUCT() \
        static std::vector<Elaine::ShaderParameterMember> GetShaderMembers() { \
            return Elaine::CollectShaderMembers<ThisShaderStruct, FirstShaderMember>(std::make_index_sequence<__COUNTER__ - FirstShaderMember>{}); } \
        static const Elaine::ShaderParameterMetadata& GetMetadata(); };
#define IMPLEMENT_SHADER_PARAMETER_STRUCT(Type, Include, Block, Domain) \
    inline const Elaine::ShaderParameterMetadata& Type::GetMetadata() { \
        static const Elaine::ShaderParameterMetadata Metadata{Include, Block, Elaine::RHIResourceGroup::Domain, GetShaderMembers()}; \
        return Metadata; }

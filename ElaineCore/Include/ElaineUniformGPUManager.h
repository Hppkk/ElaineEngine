#pragma once
#include "ElaineMatrix4.h"
#include "common/ElaineRHIProtocol.h"
#include "ElaineName.h"

namespace Elaine
{
    // float4

    enum UniformBufferSemantic
    {
        U_ViewMatrix,
        U_ProjectionMatrix,
        U_ViewProjectionMatrix,
        U_CameraPosition,
        U_CameraDirection,
        U_LightDirection,
        U_LightColor,
        CommonUniformCount,
    };

    enum PrivateUniformSemanticVS
    {

    };

    enum PrivateUniformSemanticPS
    {

    };

    struct CommonUniformBufferCPU
    {
        Matrix4x4 U_ViewMatrix;
        Matrix4x4 U_ProjectionMatrix;
        Matrix4x4 U_ViewProjectionMatrix;
        Vector4   U_CameraPosition;
        Vector4   U_CameraDirection;
        Vector4   U_LightDirection;
        Vector4   U_LightColor;
    };

    enum TextureSemantics
    {
        BaseColor,
        Normal,
        Specular,
        Ambient,
        Metallic,
        Roughness,

        // Virtual Texture semantics
        // These are used when a material is bound via VT pipeline.
        // The indirection texture maps virtual UVs to physical atlas locations.
        // Physical atlas textures contain the actual tile pixel data per layer.
        VT_Indirection,                // usampler2D - indirection lookup texture (R16G16B16A16_UINT)
        VT_PhysicalAtlas_BaseColor,    // sampler2D - physical atlas for BaseColor layer
        VT_PhysicalAtlas_Normal,       // sampler2D - physical atlas for Normal layer
        VT_PhysicalAtlas_RMA,          // sampler2D - physical atlas for Roughness/Metallic/AO layer
        VT_PhysicalAtlas_Emissive,     // sampler2D - physical atlas for Emissive layer

        Tex_Count,
    };

    enum VertexSemantic : uint8_t
    {
        POSITION,
        NORMAL,
        TANGENT,
        BITANGENT,
        TEXCOORD,
        COLOR,
        BLEND_WEIGHTS,
        BLEND_INDICES,
        INSTANCE_MATRIX,
        INSTANCE_COLOR,
        CUSTOM,
        Count
    };

    class ElaineCoreExport SemanticsRegister
    {
    public:
        static void Initialize();
        static void RegisterTextureSemantics();
        static void RegisterVertexSemantics();
        static const Name& GetName(TextureSemantics InTextureSemantice);
        static TextureSemantics GetSemantics(const Name& InTextureSemantice);
        static const Name& GetName(VertexSemantic InTextureSemantice);
        static VertexSemantic GetVertexSemantic(const Name& InSemantice);

    private:
        inline static std::unordered_map<Name, TextureSemantics> mTextureSemantics;
        inline static std::unordered_map<TextureSemantics, Name> mTextureSemanticsStr;
        inline static std::unordered_map<Name, VertexSemantic> mVertexSemantics;
        inline static std::unordered_map<VertexSemantic, Name> mVertexSemanticStr;
    };
}
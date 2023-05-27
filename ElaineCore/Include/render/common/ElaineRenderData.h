#pragma once
#include "ElaineRHITypes.h"

namespace Elaine
{
    class BufferData
    {
    public:
        size_t m_size = 0;
        void* m_data = nullptr;

        BufferData() = delete;
        BufferData(size_t size)
        {
            m_size = size;
            m_data = malloc(size);
        }
        ~BufferData()
        {
            if (m_data)
            {
                free(m_data);
            }
        }
        bool isValid() const { return m_data != nullptr; }
    };

    class TextureData
    {
    public:
        uint32_t m_width = 0;
        uint32_t m_height = 0;
        uint32_t m_depth = 0;
        uint32_t m_mip_levels = 0;
        uint32_t m_array_layers = 0;
        void* m_pixels = nullptr;

        RHIFormat m_format = RHI_FORMAT_MAX_ENUM;
        ELAINE_IMAGE_TYPE   m_type = ELAINE_IMAGE_TYPE::ELAINE_IMAGE_TYPE_UNKNOWM;

        TextureData() = default;
        ~TextureData()
        {
            if (m_pixels)
            {
                free(m_pixels);
            }
        }
        bool isValid() const { return m_pixels != nullptr; }
    };

    struct MeshVertexDataDefinition
    {
        float x, y, z;    // position
        float nx, ny, nz; // normal
        float tx, ty, tz; // tangent
        float u, v;       // UV coordinates
    };

    struct MeshVertexBindingDataDefinition
    {
        int m_index0{ 0 };
        int m_index1{ 0 };
        int m_index2{ 0 };
        int m_index3{ 0 };

        float m_weight0{ 0.f };
        float m_weight1{ 0.f };
        float m_weight2{ 0.f };
        float m_weight3{ 0.f };
    };

    struct MeshSourceDesc
    {
        std::string m_mesh_file;

        bool   operator==(const MeshSourceDesc& rhs) const { return m_mesh_file == rhs.m_mesh_file; }
        size_t getHashValue() const { return std::hash<std::string> {}(m_mesh_file); }
    };

    struct MaterialSourceDesc
    {
        std::string m_base_color_file;
        std::string m_metallic_roughness_file;
        std::string m_normal_file;
        std::string m_occlusion_file;
        std::string m_emissive_file;

        bool operator==(const MaterialSourceDesc& rhs) const
        {
            return m_base_color_file == rhs.m_base_color_file &&
                m_metallic_roughness_file == rhs.m_metallic_roughness_file &&
                m_normal_file == rhs.m_normal_file &&
                m_occlusion_file == rhs.m_occlusion_file &&
                m_emissive_file == rhs.m_emissive_file;
        }

        size_t getHashValue() const
        {
            size_t hash = 0;
            hash_combine(hash,
                m_base_color_file,
                m_metallic_roughness_file,
                m_normal_file,
                m_occlusion_file,
                m_emissive_file);
            return hash;
        }
    };

    struct StaticMeshData
    {
        std::shared_ptr<BufferData> m_vertex_buffer;
        std::shared_ptr<BufferData> m_index_buffer;
    };

    struct RenderMeshData
    {
        StaticMeshData              m_static_mesh_data;
        std::shared_ptr<BufferData> m_skeleton_binding_buffer;
    };

    struct RenderMaterialData
    {
        std::shared_ptr<TextureData> m_base_color_texture;
        std::shared_ptr<TextureData> m_metallic_roughness_texture;
        std::shared_ptr<TextureData> m_normal_texture;
        std::shared_ptr<TextureData> m_occlusion_texture;
        std::shared_ptr<TextureData> m_emissive_texture;
    };
}
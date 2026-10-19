#include "ElainePrecompiledHeader.h"
#include "render/vulkan/ElaineVulkanMaterialReflection.h"
#include "ElaineHash.h"
#include "spirv_cross/spirv_reflect.hpp"

namespace VulkanRHI
{
    static RHIPipeline::MaterialParameterType GetParameterType(const spirv_cross::SPIRType& Type)
    {
        if (!Type.array.empty()) return RHIPipeline::MaterialParameterType::Array;
        if (Type.basetype == spirv_cross::SPIRType::Boolean) return RHIPipeline::MaterialParameterType::Boolean;
        if (Type.basetype == spirv_cross::SPIRType::Int || Type.basetype == spirv_cross::SPIRType::UInt) return RHIPipeline::MaterialParameterType::Integer;
        if (Type.basetype != spirv_cross::SPIRType::Float) return RHIPipeline::MaterialParameterType::Unknown;
        if (Type.columns > 1) return RHIPipeline::MaterialParameterType::Matrix;
        return Type.vecsize == 1 ? RHIPipeline::MaterialParameterType::Scalar : RHIPipeline::MaterialParameterType::Vector;
    }

    bool ReflectMaterialParameters(const SPVCodeDesc& InCode, std::vector<RHIPipeline::MaterialParameterInfo>& OutParameters)
    {
        OutParameters.clear();
        if (InCode.mSPVCodeContent == nullptr || InCode.mContentSize == 0)
            return false;

        spirv_cross::CompilerReflection Reflection(InCode.mSPVCodeContent, InCode.mContentSize);
        const spirv_cross::ShaderResources Resources = Reflection.get_shader_resources();
        for (const auto& Resource : Resources.uniform_buffers)
        {
            if (!Reflection.has_decoration(Resource.id, spv::DecorationDescriptorSet) ||
                Reflection.get_decoration(Resource.id, spv::DecorationDescriptorSet) != 2)
                continue;

            const uint32_t Set = Reflection.get_decoration(Resource.id, spv::DecorationDescriptorSet);
            const uint32_t Binding = Reflection.get_decoration(Resource.id, spv::DecorationBinding);
            const auto& BlockType = Reflection.get_type(Resource.base_type_id);
            for (uint32_t Index = 0; Index < BlockType.member_types.size(); ++Index)
            {
                RHIPipeline::MaterialParameterInfo Info;
                Info.Name = Reflection.get_member_name(Resource.base_type_id, Index);
                Info.NameHash = Elaine::FNV_1A_HASH_64(Info.Name);
                Info.Type = GetParameterType(Reflection.get_type(BlockType.member_types[Index]));
                Info.Offset = Reflection.type_struct_member_offset(BlockType, Index);
                Info.Size = static_cast<uint32_t>(Reflection.get_declared_struct_member_size(BlockType, Index));
                const auto& MemberType = Reflection.get_type(BlockType.member_types[Index]);
                Info.ArrayCount = MemberType.array.empty() ? 1u : static_cast<uint32_t>(MemberType.array[0]);
                Info.Set = Set;
                Info.Binding = Binding;
                OutParameters.push_back(std::move(Info));
            }
        }
        return !OutParameters.empty();
    }
}

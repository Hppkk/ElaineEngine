#include "ElainePrecompiledHeader.h"
#include "render/vulkan/ElaineVulkanDescriptorSet.h"
#include "render/vulkan/ElaineVulkanDevice.h"
#include "render/vulkan/ElaineVulkanUniformBuffer.h"
#include "render/vulkan/ElaineVulkanTexture.h"

using namespace Elaine;

namespace VulkanRHI
{
    VulkanDescriptorPool::VulkanDescriptorPool(VulkanDevice* InDevice, uint32 InMaxSetsAllocations)
    {
        mDevice = InDevice;
        mMaxSetsAllocations = InMaxSetsAllocations * (VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT - VK_DESCRIPTOR_TYPE_SAMPLER + 1);
        std::vector<VkDescriptorPoolSize> PoolSizes;
        PoolSizes.resize(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT - VK_DESCRIPTOR_TYPE_SAMPLER + 1);
        for (uint32 TypeIndex = VK_DESCRIPTOR_TYPE_SAMPLER; TypeIndex <= VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT; ++TypeIndex)
        {
            VkDescriptorType DescriptorType = (VkDescriptorType)TypeIndex;
            VkDescriptorPoolSize* DescriptorPoolSize = new(&PoolSizes[TypeIndex])VkDescriptorPoolSize();
            Memory::MemoryZero(DescriptorPoolSize, sizeof(VkDescriptorPoolSize));
            DescriptorPoolSize->type = DescriptorType;
            DescriptorPoolSize->descriptorCount = mMaxSetsAllocations * 1;
        }

        VkDescriptorPoolCreateInfo PoolInfo;
        Memory::MemoryZero(PoolInfo);
        PoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        PoolInfo.poolSizeCount = PoolSizes.size();
        PoolInfo.pPoolSizes = PoolSizes.data();
        PoolInfo.maxSets = mMaxSetsAllocations;
        vkCreateDescriptorPool(mDevice->GetDevice(), &PoolInfo, VULKAN_CPU_ALLOCATOR, &mHandle);
    }
    bool VulkanDescriptorPool::AllocateDescriptorSets(const VkDescriptorSetAllocateInfo& InDescriptorSetAllocateInfo, VkDescriptorSet* OutSets)
    {
        VkDescriptorSetAllocateInfo DescriptorSetAllocateInfo = InDescriptorSetAllocateInfo;
        DescriptorSetAllocateInfo.descriptorPool = mHandle;
        return VK_SUCCESS == vkAllocateDescriptorSets(mDevice->GetDevice(), &DescriptorSetAllocateInfo, OutSets);

    }

    void VulkanDescriptorPool::Reset()
    {
        if (mHandle)
        {
            vkResetDescriptorPool(mDevice->GetDevice(), mHandle, 0);
        }
        mNumAllocatedSets = 0u;
    }

    VulkanDescriptorSetManager::VulkanDescriptorSetManager(VulkanDevice* InDevice)
    {
        mDevice = InDevice;
        mFreePool = new VulkanDescriptorPool(InDevice, 64);
        //mPools.push_back(mFreePool);
    }

    bool VulkanDescriptorSetManager::AllocateDescriptorSets(const VkDescriptorSetAllocateInfo& InDescriptorSetAllocateInfo, VulkanDescriptorSet** OutSets)
    {
        if (!mFreePool->CanAllocate())
        {
            for (auto& AllocPool : mPools)
            {
                if (AllocPool->CanAllocate())
                {
                    bool Result = false;
                    Result = AllocPool->AllocateDescriptorSets(InDescriptorSetAllocateInfo, &(*OutSets)->mHandle);
                    if (Result)
                    {
                        if (AllocPool->CanAllocate())
                        {
                            std::swap(AllocPool, mFreePool);
                            return Result;
                        }
                    }
                }
            }

            mPools.push_back(mFreePool);
            mFreePool = new VulkanDescriptorPool(mDevice, 64); 
        }

        return mFreePool->AllocateDescriptorSets(InDescriptorSetAllocateInfo, &(*OutSets)->mHandle);;
    }

    void VulkanDescriptorSetManager::WriteUniformBufferToDescriptorSet(VulkanUniformBuffer* InUniformBuffer, VulkanDescriptorSet* InDescriptorSet)
    {
        VkDescriptorBufferInfo BufferInfo{};
        BufferInfo.buffer = InUniformBuffer->GetHandle();
        BufferInfo.offset = InUniformBuffer->GetOffset();
        BufferInfo.range = InUniformBuffer->GetBufferSize();
        VkWriteDescriptorSet DescriptorWrite{};
        DescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        DescriptorWrite.dstSet = InDescriptorSet->mHandle;
        DescriptorWrite.dstBinding = 0;
        DescriptorWrite.dstArrayElement = 0;
        DescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        DescriptorWrite.descriptorCount = 1;
        DescriptorWrite.pBufferInfo = &BufferInfo;
        DescriptorWrite.pImageInfo = RHI_NULL_HANDLE; 
        DescriptorWrite.pTexelBufferView = RHI_NULL_HANDLE;
        vkUpdateDescriptorSets(mDevice->GetDevice(), 1, &DescriptorWrite, 0, RHI_NULL_HANDLE);
    }

    void VulkanDescriptorSetManager::WriteImageToDescriptorSet(VulkanTexture* InImage, VkSampler InSampler, VulkanDescriptorSet* InDescriptorSet)
    {
        VkDescriptorImageInfo ImageInfo{};
        ImageInfo.imageLayout = InImage->GetImageLayout();
        ImageInfo.imageView = InImage->GetTextureView().mView;
        ImageInfo.sampler = InSampler;
        VkWriteDescriptorSet DescriptorWrite{};
        DescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        DescriptorWrite.dstSet = InDescriptorSet->mHandle;
        DescriptorWrite.dstBinding = 0;
        DescriptorWrite.dstArrayElement = 0;
        DescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        DescriptorWrite.descriptorCount = 1;
        DescriptorWrite.pBufferInfo = RHI_NULL_HANDLE;
        DescriptorWrite.pImageInfo = &ImageInfo;
        DescriptorWrite.pTexelBufferView = RHI_NULL_HANDLE;
        vkUpdateDescriptorSets(mDevice->GetDevice(), 1, &DescriptorWrite, 0, RHI_NULL_HANDLE);
    }

    VulkanDescriptorSetManager::~VulkanDescriptorSetManager()
    {
        for (auto Pool : mPools)
        {
            SAFE_DELETE(Pool);
        }
        mPools.clear();
    }
    VkDescriptorSetLayout VulkanDescriptorSetManager::CreateDescriptorSetLayout(const VkDescriptorSetLayoutCreateInfo& InCreateInfo)
    {
        VkDescriptorSetLayout Result;
        vkCreateDescriptorSetLayout(mDevice->GetDevice(), &InCreateInfo, VULKAN_CPU_ALLOCATOR, &Result);
        mDescriptorSetLayouts.insert(Result);
        return Result;
    }

    VulkanDescriptorSet::VulkanDescriptorSet(int32 InSet, bool InEmpty)
        : mSet(InSet)
        , mbEmpty(InEmpty)
    {

    }
}
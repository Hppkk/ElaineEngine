#include "ElainePrecompiledHeader.h"
#include "render/vulkan/ElaineVulkanDescriptorSet.h"
#include "render/vulkan/ElaineVulkanDevice.h"

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
}
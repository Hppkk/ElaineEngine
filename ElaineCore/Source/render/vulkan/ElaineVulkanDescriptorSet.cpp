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
        VkBuffer Buffer = InUniformBuffer->GetHandle();
        VkDeviceSize Offset = InUniformBuffer->GetOffset();
        VkDeviceSize Range = InUniformBuffer->GetBufferSize();
        
        // 检查绑定是否变化，避免重复更新导致验证错误
        if (InDescriptorSet->IsSameBufferBinding(Buffer, Offset, Range))
        {
            return;  // 绑定未变化，跳过更新
        }
        
        VkDescriptorBufferInfo BufferInfo{};
        BufferInfo.buffer = Buffer;
        BufferInfo.offset = Offset;
        BufferInfo.range = Range;
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
        
        // 记录绑定状态
        InDescriptorSet->SetBoundBuffer(Buffer, Offset, Range);
    }

    void VulkanDescriptorSetManager::WriteImageToDescriptorSet(VulkanTexture* InImage, VkSampler InSampler, VulkanDescriptorSet* InDescriptorSet)
    {
        VkImageView ImageView = InImage->GetTextureView().mView;
        
        // 检查绑定是否变化，避免重复更新导致验证错误 (VUID-vkUpdateDescriptorSets-None-03047)
        if (InDescriptorSet->IsSameImageBinding(ImageView, InSampler))
        {
            return;  // 绑定未变化，跳过更新
        }
        
        VkDescriptorImageInfo ImageInfo{};
        ImageInfo.imageLayout = InImage->GetImageLayout();
        ImageInfo.imageView = ImageView;
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
        
        // 记录绑定状态
        InDescriptorSet->SetBoundImage(ImageView, InSampler);
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
        , mDirty(true)
        , mBoundImageView(VK_NULL_HANDLE)
        , mBoundSampler(VK_NULL_HANDLE)
        , mBoundBuffer(VK_NULL_HANDLE)
        , mBoundBufferOffset(0)
        , mBoundBufferRange(0)
    {
    }

    void VulkanDescriptorSet::SetBoundImage(VkImageView InView, VkSampler InSampler)
    {
        mBoundImageView = InView;
        mBoundSampler = InSampler;
        mDirty = false;
    }

    bool VulkanDescriptorSet::IsSameImageBinding(VkImageView InView, VkSampler InSampler) const
    {
        return !mDirty && mBoundImageView == InView && mBoundSampler == InSampler;
    }

    void VulkanDescriptorSet::SetBoundBuffer(VkBuffer InBuffer, VkDeviceSize InOffset, VkDeviceSize InRange)
    {
        mBoundBuffer = InBuffer;
        mBoundBufferOffset = InOffset;
        mBoundBufferRange = InRange;
        mDirty = false;
    }

    bool VulkanDescriptorSet::IsSameBufferBinding(VkBuffer InBuffer, VkDeviceSize InOffset, VkDeviceSize InRange) const
    {
        return !mDirty && mBoundBuffer == InBuffer && mBoundBufferOffset == InOffset && mBoundBufferRange == InRange;
    }

    //=============================================================================
    // VulkanFrameDescriptorAllocator Implementation
    //=============================================================================

    VulkanFrameDescriptorAllocator::VulkanFrameDescriptorAllocator(
        VulkanDevice* InDevice, 
        uint32 InFrameCount, 
        VulkanDescriptorSetManager* InManager)
        : mDevice(InDevice)
        , mManager(InManager)
        , mFrameCount(InFrameCount)
    {
        // 为每帧创建独立的 DescriptorPool
        mFramePools.resize(InFrameCount);
        mAllocatedSets.resize(InFrameCount);
        mCurrentAllocIndex.resize(InFrameCount, 0);

        for (uint32 i = 0; i < InFrameCount; ++i)
        {
            mFramePools[i] = new VulkanDescriptorPool(InDevice, 32);
        }
    }

    VulkanFrameDescriptorAllocator::~VulkanFrameDescriptorAllocator()
    {
        for (auto* Pool : mFramePools)
        {
            SAFE_DELETE(Pool);
        }
        mFramePools.clear();

        for (auto& FrameSets : mAllocatedSets)
        {
            for (auto* Set : FrameSets)
            {
                SAFE_DELETE(Set);
            }
            FrameSets.clear();
        }
        mAllocatedSets.clear();
    }

    VulkanDescriptorSet* VulkanFrameDescriptorAllocator::AllocateForFrame(
        uint32 InFrameIndex, 
        VkDescriptorSetLayout InLayout, 
        int32 InSetIndex)
    {
        if (InFrameIndex >= mFrameCount)
        {
            return nullptr;
        }

        auto& FrameSets = mAllocatedSets[InFrameIndex];
        uint32& AllocIndex = mCurrentAllocIndex[InFrameIndex];

        // 尝试复用已分配的 DescriptorSet
        if (AllocIndex < FrameSets.size())
        {
            VulkanDescriptorSet* ExistingSet = FrameSets[AllocIndex];
            ExistingSet->MarkDirty();  // 标记为需要更新
            ++AllocIndex;
            return ExistingSet;
        }

        // 需要分配新的 DescriptorSet
        VulkanDescriptorSet* NewSet = new VulkanDescriptorSet(InSetIndex, false);

        VkDescriptorSetAllocateInfo AllocInfo = {};
        AllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        AllocInfo.descriptorSetCount = 1;
        AllocInfo.pSetLayouts = &InLayout;

        VulkanDescriptorPool* Pool = mFramePools[InFrameIndex];
        if (!Pool->CanAllocate())
        {
            // Pool 已满，创建新的 Pool
            // 直接替换 TODO 
            delete Pool;
            mFramePools[InFrameIndex] = new VulkanDescriptorPool(mDevice, 64);
            Pool = mFramePools[InFrameIndex];
        }

        if (Pool->AllocateDescriptorSets(AllocInfo, &NewSet->mHandle))
        {
            FrameSets.push_back(NewSet);
            ++AllocIndex;
            return NewSet;
        }

        // 分配失败
        delete NewSet;
        return nullptr;
    }

    void VulkanFrameDescriptorAllocator::ResetFrame(uint32 InFrameIndex)
    {
        if (InFrameIndex >= mFrameCount)
        {
            return;
        }

        // 重置分配索引，下一帧从头开始复用
        mCurrentAllocIndex[InFrameIndex] = 0;

        // 将所有该帧的 DescriptorSet 标记为 dirty，以便重新写入
        for (auto* Set : mAllocatedSets[InFrameIndex])
        {
            Set->MarkDirty();
        }
    }
}
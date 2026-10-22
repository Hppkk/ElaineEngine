#include "ElainePrecompiledHeader.h"
#include "render/ElaineRenderSystem.h"
#include "render/common/ElaineRHICommandContext.h"
#include "ElaineRenderCommandQueue.h"
#include "render/common/ElaineRHICommandList.h"
#include "render/common/ElaineRHICommandContext.h"

namespace Elaine
{
	RenderSystem::RenderSystem()
	{

	}

	void RenderSystem::Initialize(const RHI_PARAM_DESC& InDesc)
	{
		mWindowHandle = InDesc.NativeHandle;
		InitEngineRHI(InDesc);
		mImmedCommandCtx = GetDynamicRHI()->GetDefaultCommandContext();
		mRenderCommandQueue = new RenderCommandQueue();
	}

    void RenderSystem::InitializeRenderResources()
    {
        CheckRHICreationThread();
        if (mResourcesInitialized) return;
        auto* Context = GetDynamicRHI()->GetDefaultCommandContext();
        auto* List = Context->GetRHICommandListMgr()->CreateCommandList();
        const uint8 Pixels[][4] = {{0,0,0,0}, {255,255,255,255}, {0,0,0,255}, {128,128,255,255}};
        RHITextureHandle Textures[static_cast<size_t>(RHIDefaultTexture::Count)];
        for (size_t i = 1; i < static_cast<size_t>(RHIDefaultTexture::Count); ++i)
        {
            RHITextureDesc Desc; Desc.mExtent = Vector2(1,1); Desc.mFormat = PF_R8G8B8A8;
            Desc.mFlags = TextureCreateFlags::ShaderResource;
            Textures[i] = Context->RHICreateTexture(Desc, nullptr);
            Context->UploadTexture(Textures[i].get(), Pixels[i], sizeof(Pixels[i]));
        }
        List->SubmitUploads();
        for (size_t i = 1; i < static_cast<size_t>(RHIDefaultTexture::Count); ++i) mDefaultTextures[i] = std::move(Textures[i]);
        mResourcesInitialized = true;
    }
    RHITextureHandle RenderSystem::GetDefaultTexture(RHIDefaultTexture Type) const
    {
        const size_t Index = static_cast<size_t>(Type);
        if (!mResourcesInitialized || !Index || Index >= static_cast<size_t>(RHIDefaultTexture::Count))
            throw std::logic_error("Default render texture is not initialized");
        return mDefaultTextures[Index];
    }
    void RenderSystem::ReleaseRenderResources()
    {
        CheckRHICreationThread();
        for (auto& Texture : mDefaultTextures) Texture.reset();
        GetDynamicRHI()->GetDefaultCommandContext()->RHIReleaseUniformPool();
        mResourcesInitialized = false;
    }

	RHIBufferHandle RenderSystem::CreateBuffer(BufferUsageFlags InUsage, ERHIAccess InResourceState, void* InData, size_t InSize)
	{
        CheckRHICreationThread();
        auto* Context = GetDynamicRHI()->GetDefaultCommandContext();
        auto* List = Context->GetRHICommandListMgr()->CreateCommandList();
        auto Buffer = Context->RHICreateBuffer(static_cast<uint32>(InSize), InUsage | BufferUsageFlags::Static, 0, ERHIAccess::Unknown, nullptr);
        if (InData && Buffer) Context->UploadBuffer(Buffer.get(), InData, static_cast<uint32>(InSize), InResourceState);
        List->SubmitUploads();
        return Buffer;
	}

	RenderSystem::~RenderSystem()
	{
		//GetDynamicRHI()->DestroyCommandContext(mImmedCommandCtx);
		mImmedCommandCtx = nullptr;
		DestroyEngineRHI();
		SAFE_DELETE(mRenderCommandQueue);
	}

}

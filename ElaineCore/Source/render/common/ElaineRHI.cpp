#include "ElainePrecompiledHeader.h"
#include "render/common/ElaineRHI.h"
#include "render/common/ElaineRHICommandList.h"
#include "render/common/ElaineRenderModule.h"
#include "ElaineThreadManager.h"
#include "render/common/ElaineRHICommandContext.h"
#include "ElaineBarrier.h"

namespace Elaine
{
	class _RHIPrivate;

	_RHIPrivate* _GRHI = nullptr;
	EBarrier* G_GfxBarrier = nullptr;
	EBarrier* G_RenderBarrier = nullptr;
	ThreadWrap* G_RHIThread = nullptr;

	static void RHIThreadMain();

	class _RHIPrivate
	{
	public:
		void _Initilize(const RHI_PARAM_DESC& InDesc)
		{
			mRHIType = InDesc.RHIType;
			switch (InDesc.RHIType)
			{
			case Elaine::Vulkan:
				mDynamicRHI = new VulkanRHI::VulkanDynamicRHI();
				mDynamicRHI->Initialize(InDesc);
				break;
			case Elaine::DirectX11:
				break;
			case Elaine::DirectX12:
				break;
			case Elaine::Metal:
				break;
			case Elaine::OpenGl:
				break;
			default:
				break;
			}
		}

		void _Destroy()
		{
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



	bool InitEngineRHI(const RHI_PARAM_DESC& InDesc)
	{
		_GRHI = new _RHIPrivate();
		//@TODO : move to rhi thread init.
		_GRHI->_Initilize(InDesc);
		G_GfxBarrier = new EBarrier();
		G_RenderBarrier = new EBarrier();
		G_RHIThread = ThreadManager::instance()->GetOrCreateThread(NamedThread::RHIThread, RHIThreadMain);
		return true;
	}

	bool DestroyEngineRHI()
	{

		_GRHI->_Destroy();
		SAFE_DELETE(_GRHI);
		return true;
	}

	static void RHIThreadMain()
	{
		G_RenderBarrier->Signal();
		RHICommandContext* GfxCommandCtx = GetDynamicRHI()->GetDefaultCommandContext();
		while (true)
		{
			G_GfxBarrier->Wait();
			RHICommandListManager* RHICmdListMgr = GfxCommandCtx->GetRHICommandListMgr();
			RHICmdListMgr->SwapCommands();
			//GfxCommandCtx->RHIBeginFrame();
			RHICmdListMgr->ExecuteCommands();
			//GfxCommandCtx->RHIEndFrame();
			G_RenderBarrier->Signal();
		}
	}

	bool RecordRHICommand(EM_RHICommand InCmd, void* InRHIResourceHandle)
	{
		
		return true;
	}

	void ElaineCoreExport WaitForRHIThread_Gfx()
	{
		G_GfxBarrier->Wait();
	}

	void ElaineCoreExport NotifyForRHIThread_Gfx()
	{
		G_GfxBarrier->Signal();
	}

	void ElaineCoreExport WaitForRenderThread_Gfx()
	{
		G_RenderBarrier->Wait();
	}

	void ElaineCoreExport NotifyForRenderThread_Gfx()
	{
		G_RenderBarrier->Signal();
	}

	DynamicRHI* GetDynamicRHI()
	{
		return _GRHI->GetDynamicRHI();
	}
}
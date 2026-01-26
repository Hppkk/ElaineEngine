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
	ThreadWrap* G_GfxThread = nullptr;
	ThreadWrap* G_ComputeThread = nullptr;
	ThreadWrap* G_TransfThread = nullptr;

	static void GraphicThreadMain();
	static void ComputeThreadMain();
	static void TransfThreadMain();

	class _RHIPrivate
	{
	public:
		void _Initilize(const RHI_PARAM_DESC& InDesc)
		{
			_RHIType = InDesc.RHIType;
			switch (InDesc.RHIType)
			{
			case Elaine::Vulkan:
				_DynamicRHI = new VulkanRHI::VulkanDynamicRHI();
				_DynamicRHI->Initialize(InDesc);
				break;
			case Elaine::Dx11:
				break;
			case Elaine::Dx12:
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
			_DynamicRHI = nullptr;
		}
		RHITYPE GetType() const { return _RHIType; }
		DynamicRHI* GetDynamicRHI() const { return _DynamicRHI; }

	private:
		RHITYPE _RHIType = Vulkan;
		DynamicRHI* _DynamicRHI = nullptr;
	};

	DynamicRHI::~DynamicRHI()
	{

	}



	bool InitEngineRHI(const RHI_PARAM_DESC& InDesc)
	{
		_GRHI = new _RHIPrivate();
		_GRHI->_Initilize(InDesc);
		G_GfxBarrier = new EBarrier();
		G_RenderBarrier = new EBarrier();
		G_GfxThread = ThreadManager::instance()->GetOrCreateThread(NamedThread::RHIGraphicThread, GraphicThreadMain);
		G_ComputeThread = ThreadManager::instance()->GetOrCreateThread(NamedThread::RHIComputeThread, ComputeThreadMain);
		G_TransfThread = ThreadManager::instance()->GetOrCreateThread(NamedThread::RHITransferThread, TransfThreadMain);
		return true;
	}

	bool DestroyEngineRHI()
	{

		_GRHI->_Destroy();
		SAFE_DELETE(_GRHI);
		return true;
	}

	static void GraphicThreadMain()
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

	static void ComputeThreadMain()
	{
		RHICommandContext* ComputeCommandCtx = GetDynamicRHI()->GetDefaultComputeContext();
		while (true)
		{

		}
	}

	static void TransfThreadMain()
	{
		RHICommandContext* TransfCommandCtx = GetDynamicRHI()->GetDefaultTransferContext();
		while (true)
		{

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
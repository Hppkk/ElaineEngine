#include "ElainePrecompiledHeader.h"
#include "RenderProxy/ElaineMeshRenderProxy.h"

namespace Elaine
{
	StaticMeshRenderProxy::StaticMeshRenderProxy()
	{
		mType = EProxyType::StaticMesh;
	}

	StaticMeshRenderProxy::~StaticMeshRenderProxy()
	{

	}

	void StaticMeshRenderProxy::UpdateRenderQueue(RenderQueueSet* InRenderQueue)
	{
		if (!IsBindingsInitialized())
			return;

	}

}
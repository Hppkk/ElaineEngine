#include "ElainePrecompiledHeader.h"
#include "RenderProxy/ElaineTessellationRenderProxy.h"

namespace Elaine
{
	TessellationRenderProxy::TessellationRenderProxy(TessellationType InType)
		: mTessellationType(InType)
	{

	}

	TessellationRenderProxy::~TessellationRenderProxy()
	{

	}

	void TessellationRenderProxy::ChangeTessellation(TessellationType InType)
	{
		if (InType == mTessellationType)
			return;

		CreateGeometry(InType);
	}

	void TessellationRenderProxy::CreateGeometry(TessellationType InType)
	{
		switch (InType)
		{
		case Elaine::TessellationType::None:
			LOG_ERROR("Can not create geometry type TessellationType::None.");
			break;
		case Elaine::TessellationType::Cube:
			CreateCube();
			break;
		case Elaine::TessellationType::Sphere:
			CreateSphere();
			break;
		case Elaine::TessellationType::Cone:
			CreateCone();
			break;
		default:
			break;
		}
	}

	void TessellationRenderProxy::CreateCube()
	{

	}

	void TessellationRenderProxy::CreateSphere()
	{

	}

	void TessellationRenderProxy::CreateCone()
	{

	}
}
#pragma once
#include "ElaineCorePrerequirements.h"
#include "RenderProxy/ElaineRenderProxy.h"

namespace Elaine
{
	enum class TessellationType
	{
		None,
		Cube,
		Sphere,
		Cone,
	};

	class ElaineCoreExport TessellationRenderProxy : public RenderProxy
	{
	public:
		TessellationRenderProxy(TessellationType InType);
		virtual ~TessellationRenderProxy();
		void ChangeTessellation(TessellationType InType);
	private:
		void CreateGeometry(TessellationType InType);
		void CreateCube();
		void CreateSphere();
		void CreateCone();

	private:
		TessellationType mTessellationType = TessellationType::None;
	};
}
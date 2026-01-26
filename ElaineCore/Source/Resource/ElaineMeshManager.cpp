#include <ElainePrecompiledHeader.h>
#include <ElaineMeshManager.h>

namespace Elaine
{
	MeshManager::MeshManager()
		: ResourceManager(RT_Mesh)
	{

	}

	MeshManager::~MeshManager()
	{

	}

	ResourceBasePtr Elaine::MeshManager::CreateResourceImpl(const std::string& InPath)
	{
		return ResourcePtr<Mesh>(new Mesh(this, InPath));
	}
}
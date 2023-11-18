#pragma once

namespace Elaine
{
	class ElaineCoreExport Mesh :ResourceBase
	{
	public:
		Mesh() = default;
		Mesh(ResourceManager* pManager, const std::string& res_name);

	};
}
#pragma once

namespace Elaine
{
	//This System is abandoned. Now use TaskGraph.
	class ElaineCoreExport JobSystem :public Singleton<JobSystem>
	{
	public:
		JobSystem();
		~JobSystem();
	private:

	};
}
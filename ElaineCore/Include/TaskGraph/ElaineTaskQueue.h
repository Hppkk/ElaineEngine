#include "ElaineCorePrerequirements.h"

namespace TaskGraph
{
	class ElaineCoreExport TaskQueue
	{
	public:
		TaskQueue();
		~TaskQueue();

		bool PopTask();
	};
}
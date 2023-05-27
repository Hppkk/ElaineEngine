#include "ElaineRoot.h"
#include "ElaineLogSystem.h"

namespace Elaine
{
#ifdef __cplusplus
extern "C" {
#endif // _cplusplus

	int main(int argc, char** argv)
	{
		new Root(em_Editor);
		Root::instance()->Init();
		LOG_INFO("EngineStart")
			while (1)
			{

			}

		delete Root::instance();
		return 0;
	}
#ifdef __cplusplus
}
#endif // __cplusplus

}
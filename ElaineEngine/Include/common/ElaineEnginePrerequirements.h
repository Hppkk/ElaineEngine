#pragma once
#include "ElainePatform.h"

namespace Elaine
{
#if ELAINE_PLATFORM == ELAINE_PLATFORM_WINDOWS
	#ifdef EAGLEENGINE_EXPORTS
		#ifndef ElaineEngineExport
			#define ElaineEngineExport  __declspec(dllexport)
		#endif //
	#else 
		#ifndef ElaineEngineExport
			#define ElaineEngineExport  __declspec(dllimport)
		#endif
	#endif // 
#endif // 
}

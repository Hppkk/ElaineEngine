#pragma once
#include "ElainePatform.h"

namespace Elaine
{
#if  ELAINE_PLATFORM == ELAINE_PLATFORM_WINDOWS
	#ifdef ELAINECORE_DLLEXPORT
		#ifndef ElaineCoreExport
			#define ElaineCoreExport  __declspec(dllexport)
		#endif // !ElaineCoreExport
	#else 
		#ifndef ElaineCoreExport
			#define ElaineCoreExport  __declspec(dllimport)
		#endif
	#endif // ElaineCORE_DLLEXPORT
#endif // WIN32
}

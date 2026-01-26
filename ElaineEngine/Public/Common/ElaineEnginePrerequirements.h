#pragma once
#include "ElainePatform.h"
#include "Elaine.h"
#include "ElaineName.h"

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

#ifndef REGISTER_COM_FACTORY
#define REGISTER_COM_FACTORY(ComType) \
{  \
	ComponentFactory* factory = new ComType##Factory(#ComType);\
	ComponentFactoryManager::instance()->RegisterFactory(#ComType, factory);  \
}
#endif // !REGISTER_COM_FACTORY

#ifndef DEFINE_COM_FACTORY
#define DEFINE_COM_FACTORY(ComType) \
class ComType##Factory :public ComponentFactory \
{   \
public:  \
	ComType##Factory(const char* InType) : ComponentFactory(InType) {}  \
	virtual ~ComType##Factory() {}  \
	virtual Component* CreateComponentImpl(GameObject * InObject) override { Component* NewCom = new ComType(InObject); return NewCom; } \
	virtual ComponentInfo* CreateComponentInfoImpl() override { ComponentInfo* NewComInfo = new ComType##Info(); return NewComInfo; } \
};
#endif
}

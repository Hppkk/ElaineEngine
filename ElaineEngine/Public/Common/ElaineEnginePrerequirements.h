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
	virtual ActorComponent* CreateComponentImpl(Actor * InObject) override { ActorComponent* NewCom = new ComType(InObject); return NewCom; } \
	virtual ActorComponentInfo* CreateActorComponentInfoImpl() override { ActorComponentInfo* NewComInfo = new ComType##Info(); return NewComInfo; } \
};
#endif
}

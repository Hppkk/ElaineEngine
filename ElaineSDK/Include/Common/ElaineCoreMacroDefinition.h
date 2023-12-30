#pragma once

namespace Elaine
{
#ifndef SAFE_DELETE
#define SAFE_DELETE(ptr) if(ptr != nullptr){delete ptr; ptr = nullptr;}
#endif // !SAFE_DELETE
#ifndef GetName
#define GetTypeStringName(val) #val
#endif // !GetName

#ifndef REGISTERCOMFACTORY
#define REGISTERCOMFACTORY(comType) ComponentFactory* factory = new comType##Factory();\
	ComponentFactoryManager::instance()->registerComFactory(#comType, factory);
#endif // !REGISTERCOMFACTORY

}
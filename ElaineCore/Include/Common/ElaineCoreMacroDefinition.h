#pragma once

namespace Elaine
{
#ifndef SAFE_DELETE
#define SAFE_DELETE(ptr) if(ptr != nullptr){delete ptr; ptr = nullptr;}
#endif // !SAFE_DELETE
#ifndef GetName
#define GetTypeStringName(val) #val
#endif // !GetName

#ifndef STR_NAME
#define STR_NAME(VAL) #VAL
#endif // !STR_NAME

#ifndef WSTR_NAME
#define WSTR_NAME(VAL) L#VAL
#endif // !WSTR_NAME


#ifndef REGISTERCOMFACTORY
#define REGISTERCOMFACTORY(comType) ComponentFactory* factory = new comType##Factory();\
	ComponentFactoryManager::instance()->registerComFactory(#comType, factory);
#endif // !REGISTERCOMFACTORY

	template<typename Enum>
	constexpr bool EnumHasAnyFlags(Enum Flags, Enum Contains)
	{
		return (((__underlying_type(Enum))Flags) & (__underlying_type(Enum))Contains) != 0;
	}

	template<typename Enum>
	constexpr bool EnumHasAllFlags(Enum Flags, Enum Contains)
	{
		return (((__underlying_type(Enum))Flags) & (__underlying_type(Enum))Contains) == ((__underlying_type(Enum))Contains);
	}
}
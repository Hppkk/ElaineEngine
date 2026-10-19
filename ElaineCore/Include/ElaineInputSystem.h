#pragma once
#include "ElaineCorePrerequirements.h"
#include "Common/ElaineSingleton.h"
#include "ElaineInputRouter.h"

namespace Elaine
{
    // Platform code only queues events; the application chooses the routing point.
    class ELAINE_INPUT_API InputSystem : public Singleton<InputSystem>, public InputRouter
    {
    public:
        InputSystem();
        static InputSystem* TryGet() { return m_instance; }
    };
}

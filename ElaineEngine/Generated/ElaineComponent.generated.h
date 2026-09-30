// ============================================================
// AUTO-GENERATED FILE by ElaineBuildTool - DO NOT EDIT
// Source: ElaineActorComponent.h
// ============================================================
#pragma once

#include "ElaineTypeDescriptor.h"
#include <nlohmann/json_fwd.hpp>

// ======================================================
// GENERATED_BODY for Elaine::ActorComponent
// ======================================================

namespace Elaine { namespace Generated { struct Z_Register_ActorComponent; } }
#ifdef _HAS_EDITOR_
namespace Elaine { namespace EditorGenerated { struct Z_Drawer_ActorComponent; } }
#endif

#undef GENERATED_BODY
#ifdef _HAS_EDITOR_
#define GENERATED_BODY() \
    friend struct Elaine::Generated::Z_Register_ActorComponent; \
    friend struct Elaine::EditorGenerated::Z_Drawer_ActorComponent; \
public: \
    static Elaine::TypeDescriptor* StaticClass(); \
    virtual Elaine::TypeDescriptor* GetClass() const; \
    static const char* StaticClassName() { return "ActorComponent"; } \
    virtual void Serialize(nlohmann::json& outJson) const; \
    virtual void Deserialize(const nlohmann::json& inJson); \
private:
#else
#define GENERATED_BODY() \
    friend struct Elaine::Generated::Z_Register_ActorComponent; \
public: \
    static Elaine::TypeDescriptor* StaticClass(); \
    virtual Elaine::TypeDescriptor* GetClass() const; \
    static const char* StaticClassName() { return "ActorComponent"; } \
    virtual void Serialize(nlohmann::json& outJson) const; \
    virtual void Deserialize(const nlohmann::json& inJson); \
private:
#endif


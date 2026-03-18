#pragma once
#include "ElaineTypeDescriptor.h"
#include <string>
#include <functional>
#include <unordered_map>
#include <nlohmann/json_fwd.hpp>

namespace Elaine
{
	using JsonCpp = nlohmann::json;

	// ============================================================
	// ITypeSerializer — interface for custom type serialization
	// ============================================================
	class ELAINE_REFLECTION_API ITypeSerializer
	{
	public:
		virtual ~ITypeSerializer() = default;
		virtual void Serialize(const void* data, JsonCpp& parent, const char* name) = 0;
		virtual void Deserialize(void* data, const JsonCpp& parent, const char* name) = 0;
	};

	// ============================================================
	// TypeSerializerRegistry — register serializers for custom types
	// ============================================================
	class ELAINE_REFLECTION_API TypeSerializerRegistry
	{
	public:
		static TypeSerializerRegistry& Instance()
		{
			static TypeSerializerRegistry sInstance;
			return sInstance;
		}

		void Register(const std::string& typeName, ITypeSerializer* serializer)
		{
			mSerializers[typeName] = serializer;
		}

		ITypeSerializer* Find(const std::string& typeName) const
		{
			auto it = mSerializers.find(typeName);
			return (it != mSerializers.end()) ? it->second : nullptr;
		}

	private:
		TypeSerializerRegistry() = default;
		std::unordered_map<std::string, ITypeSerializer*> mSerializers;
	};

	// ============================================================
	// ReflectionSerializer — serialize/deserialize using reflection
	// ============================================================
	class ELAINE_REFLECTION_API ReflectionSerializer
	{
	public:
		// Serialize all non-Transient properties to a JSON object
		static void Serialize(const void* obj, const TypeDescriptor* desc, JsonCpp& outJson);

		// Deserialize all non-Transient properties from a JSON object
		static void Deserialize(void* obj, const TypeDescriptor* desc, const JsonCpp& inJson);

		// Convenience: serialize to JSON string
		static std::string SerializeToString(const void* obj, const TypeDescriptor* desc);

		// Convenience: deserialize from JSON string
		static bool DeserializeFromString(void* obj, const TypeDescriptor* desc, const std::string& jsonStr);

	private:
		// Built-in type handlers
		static void SerializeProperty(const void* obj, const PropertyDescriptor& prop, JsonCpp& parent);
		static void DeserializeProperty(void* obj, const PropertyDescriptor& prop, const JsonCpp& parent);
	};
}

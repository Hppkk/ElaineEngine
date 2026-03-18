#include "ElaineSerializer.h"
#include <nlohmann/json.hpp>
#include <cstring>

namespace Elaine
{
	// ============================================================
	// SerializeProperty — dispatch by type name
	// ============================================================
	void ReflectionSerializer::SerializeProperty(const void* obj, const PropertyDescriptor& prop, JsonCpp& parent)
	{
		const void* data = prop.GetPtrIn(obj);
		const char* name = prop.Name;
		const char* typeName = prop.TypeName;

		// Check for custom serializer first
		ITypeSerializer* custom = TypeSerializerRegistry::Instance().Find(typeName);
		if (custom)
		{
			custom->Serialize(data, parent, name);
			return;
		}

		// Built-in types
		if (strcmp(typeName, "float") == 0)
		{
			parent[name] = *static_cast<const float*>(data);
		}
		else if (strcmp(typeName, "double") == 0)
		{
			parent[name] = *static_cast<const double*>(data);
		}
		else if (strcmp(typeName, "int") == 0 || strcmp(typeName, "int32") == 0 ||
				 strcmp(typeName, "int32_t") == 0)
		{
			parent[name] = *static_cast<const int*>(data);
		}
		else if (strcmp(typeName, "uint32") == 0 || strcmp(typeName, "uint32_t") == 0 ||
				 strcmp(typeName, "unsigned int") == 0)
		{
			parent[name] = *static_cast<const unsigned int*>(data);
		}
		else if (strcmp(typeName, "uint8") == 0 || strcmp(typeName, "uint8_t") == 0 ||
				 strcmp(typeName, "unsigned char") == 0)
		{
			parent[name] = static_cast<int>(*static_cast<const unsigned char*>(data));
		}
		else if (strcmp(typeName, "bool") == 0)
		{
			parent[name] = *static_cast<const bool*>(data);
		}
		else if (strcmp(typeName, "std::string") == 0 ||
				 strcmp(typeName, "std::basic_string<char>") == 0)
		{
			parent[name] = *static_cast<const std::string*>(data);
		}
		else
		{
			// Try to serialize as a nested reflected type
			TypeDescriptor* nestedDesc = TypeRegistry::Instance().Find(typeName);
			if (nestedDesc)
			{
				JsonCpp child = JsonCpp::object();
				Serialize(data, nestedDesc, child);
				parent[name] = child;
			}
			// else: unsupported type, skip silently
		}
	}

	// ============================================================
	// DeserializeProperty — dispatch by type name
	// ============================================================
	void ReflectionSerializer::DeserializeProperty(void* obj, const PropertyDescriptor& prop, const JsonCpp& parent)
	{
		void* data = prop.GetPtrIn(obj);
		const char* name = prop.Name;
		const char* typeName = prop.TypeName;

		if (!parent.contains(name))
			return;

		const auto& item = parent[name];

		// Check for custom serializer first
		ITypeSerializer* custom = TypeSerializerRegistry::Instance().Find(typeName);
		if (custom)
		{
			custom->Deserialize(data, parent, name);
			return;
		}

		// Built-in types
		if (strcmp(typeName, "float") == 0)
		{
			if (item.is_number())
				*static_cast<float*>(data) = item.get<float>();
		}
		else if (strcmp(typeName, "double") == 0)
		{
			if (item.is_number())
				*static_cast<double*>(data) = item.get<double>();
		}
		else if (strcmp(typeName, "int") == 0 || strcmp(typeName, "int32") == 0 ||
				 strcmp(typeName, "int32_t") == 0)
		{
			if (item.is_number_integer())
				*static_cast<int*>(data) = item.get<int>();
		}
		else if (strcmp(typeName, "uint32") == 0 || strcmp(typeName, "uint32_t") == 0 ||
				 strcmp(typeName, "unsigned int") == 0)
		{
			if (item.is_number_unsigned())
				*static_cast<unsigned int*>(data) = item.get<unsigned int>();
		}
		else if (strcmp(typeName, "uint8") == 0 || strcmp(typeName, "uint8_t") == 0 ||
				 strcmp(typeName, "unsigned char") == 0)
		{
			if (item.is_number_integer())
				*static_cast<unsigned char*>(data) = static_cast<unsigned char>(item.get<int>());
		}
		else if (strcmp(typeName, "bool") == 0)
		{
			if (item.is_boolean())
				*static_cast<bool*>(data) = item.get<bool>();
		}
		else if (strcmp(typeName, "std::string") == 0 ||
				 strcmp(typeName, "std::basic_string<char>") == 0)
		{
			if (item.is_string())
				*static_cast<std::string*>(data) = item.get<std::string>();
		}
		else
		{
			// Try nested reflected type
			TypeDescriptor* nestedDesc = TypeRegistry::Instance().Find(typeName);
			if (nestedDesc && item.is_object())
			{
				Deserialize(data, nestedDesc, item);
			}
		}
	}

	// ============================================================
	// Serialize — iterate all serializable properties
	// ============================================================
	void ReflectionSerializer::Serialize(const void* obj, const TypeDescriptor* desc, JsonCpp& outJson)
	{
		if (!obj || !desc)
			return;

		for (auto& prop : desc->GetProperties())
		{
			if (prop.IsTransient())
				continue;
			SerializeProperty(obj, prop, outJson);
		}
	}

	// ============================================================
	// Deserialize — iterate all serializable properties
	// ============================================================
	void ReflectionSerializer::Deserialize(void* obj, const TypeDescriptor* desc, const JsonCpp& inJson)
	{
		if (!obj || !desc)
			return;

		for (auto& prop : desc->GetProperties())
		{
			if (prop.IsTransient())
				continue;
			DeserializeProperty(obj, prop, inJson);
		}
	}

	// ============================================================
	// Convenience: to/from string
	// ============================================================
	std::string ReflectionSerializer::SerializeToString(const void* obj, const TypeDescriptor* desc)
	{
		JsonCpp root = JsonCpp::object();
		Serialize(obj, desc, root);
		return root.dump();
	}

	bool ReflectionSerializer::DeserializeFromString(void* obj, const TypeDescriptor* desc, const std::string& jsonStr)
	{
		try
		{
			JsonCpp root = JsonCpp::parse(jsonStr);
			Deserialize(obj, desc, root);
			return true;
		}
		catch (const JsonCpp::parse_error&)
		{
			return false;
		}
	}
}

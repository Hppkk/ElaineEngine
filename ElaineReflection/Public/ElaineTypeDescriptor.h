#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <cstddef>
#include <functional>
#include <cstring>

#ifdef ELAINEREFLECTION_EXPORTS
	#define ELAINE_REFLECTION_API __declspec(dllexport)
#else
	#define ELAINE_REFLECTION_API __declspec(dllimport)
#endif

namespace Elaine
{
	// ============================================================
	// PropertyMeta — key/value metadata from EPROPERTY(...) args
	// ============================================================
	using MetaMap = std::unordered_map<std::string, std::string>;

	// ============================================================
	// PropertyDescriptor — describes a single reflected property
	// ============================================================
	struct ELAINE_REFLECTION_API PropertyDescriptor
	{
		const char* Name		= nullptr;
		const char* TypeName	= nullptr;
		size_t		Offset		= 0;
		size_t		Size		= 0;
		MetaMap		Meta;		// Metadata from EPROPERTY(...) arguments

		// Read/write helpers using void* base pointer
		void* GetPtrIn(void* obj) const
		{
			return static_cast<char*>(obj) + Offset;
		}

		const void* GetPtrIn(const void* obj) const
		{
			return static_cast<const char*>(obj) + Offset;
		}

		// Metadata convenience accessors
		bool HasMeta(const char* key) const
		{
			return Meta.find(key) != Meta.end();
		}

		const std::string& GetMeta(const char* key) const
		{
			static const std::string empty;
			auto it = Meta.find(key);
			return (it != Meta.end()) ? it->second : empty;
		}

		const char* GetMetaCStr(const char* key, const char* def = "") const
		{
			auto it = Meta.find(key);
			return (it != Meta.end()) ? it->second.c_str() : def;
		}

		float GetMetaFloat(const char* key, float def = 0.f) const
		{
			auto it = Meta.find(key);
			if (it != Meta.end())
			{
				try { return std::stof(it->second); }
				catch (...) { return def; }
			}
			return def;
		}

		int GetMetaInt(const char* key, int def = 0) const
		{
			auto it = Meta.find(key);
			if (it != Meta.end())
			{
				try { return std::stoi(it->second); }
				catch (...) { return def; }
			}
			return def;
		}

		bool GetMetaBool(const char* key, bool def = false) const
		{
			auto it = Meta.find(key);
			if (it != Meta.end())
			{
				const auto& v = it->second;
				if (v == "true" || v == "1" || v.empty()) return true;
				if (v == "false" || v == "0") return false;
			}
			return def;
		}

		// Common metadata shortcuts
		bool IsTransient()   const { return HasMeta("Transient"); }
		bool IsHidden()      const { return HasMeta("Hidden"); }
		bool IsReadOnly()    const { return HasMeta("ReadOnly"); }
		bool IsSerializable() const { return !IsTransient(); }

		const char* GetDisplayName() const
		{
			auto it = Meta.find("DisplayName");
			return (it != Meta.end()) ? it->second.c_str() : Name;
		}

		const char* GetTooltip() const
		{
			return GetMetaCStr("Tooltip", "");
		}

		const char* GetCategory() const
		{
			return GetMetaCStr("Category", "Default");
		}
	};

	// ============================================================
	// FunctionParameter — a parameter in a reflected function
	// ============================================================
	struct ELAINE_REFLECTION_API FunctionParameter
	{
		const char* Name		= nullptr;
		const char* TypeName	= nullptr;
	};

	// ============================================================
	// FunctionDescriptor — describes a single reflected function
	// ============================================================
	struct ELAINE_REFLECTION_API FunctionDescriptor
	{
		const char* Name			= nullptr;
		const char* ReturnTypeName	= nullptr;
		std::vector<FunctionParameter> Parameters;
		MetaMap		Meta;

		bool HasMeta(const char* key) const
		{
			return Meta.find(key) != Meta.end();
		}

		const char* GetDisplayName() const
		{
			auto it = Meta.find("DisplayName");
			return (it != Meta.end()) ? it->second.c_str() : Name;
		}

		const char* GetTooltip() const
		{
			auto it = Meta.find("Tooltip");
			return (it != Meta.end()) ? it->second.c_str() : "";
		}

		const char* GetCategory() const
		{
			auto it = Meta.find("Category");
			return (it != Meta.end()) ? it->second.c_str() : "Default";
		}
	};

	// ============================================================
	// TypeDescriptor — describes a reflected class/struct
	// ============================================================
	class ELAINE_REFLECTION_API TypeDescriptor
	{
	public:
		TypeDescriptor() = default;
		~TypeDescriptor() = default;

		const char* GetClassName() const { return mClassName; }
		const char* GetParentClassName() const { return mParentClassName; }
		const std::vector<PropertyDescriptor>& GetProperties() const { return mProperties; }
		const std::vector<FunctionDescriptor>& GetFunctions() const { return mFunctions; }
		const MetaMap& GetMeta() const { return mMeta; }

		void SetClassName(const char* name) { mClassName = name; }
		void SetParentClassName(const char* name) { mParentClassName = name; }

		void AddProperty(const PropertyDescriptor& prop)
		{
			mProperties.push_back(prop);
		}

		void AddFunction(const FunctionDescriptor& func)
		{
			mFunctions.push_back(func);
		}

		void SetMeta(const std::string& key, const std::string& value)
		{
			mMeta[key] = value;
		}

		// Find a property by name
		const PropertyDescriptor* FindProperty(const char* name) const
		{
			for (auto& prop : mProperties)
			{
				if (strcmp(prop.Name, name) == 0)
					return &prop;
			}
			return nullptr;
		}

		// Find a function by name
		const FunctionDescriptor* FindFunction(const char* name) const
		{
			for (auto& func : mFunctions)
			{
				if (strcmp(func.Name, name) == 0)
					return &func;
			}
			return nullptr;
		}

		// Get serializable properties (not Transient)
		std::vector<const PropertyDescriptor*> GetSerializableProperties() const
		{
			std::vector<const PropertyDescriptor*> result;
			for (auto& prop : mProperties)
			{
				if (prop.IsSerializable())
					result.push_back(&prop);
			}
			return result;
		}

		// Get category names for grouped display
		std::vector<std::string> GetCategories() const
		{
			std::vector<std::string> cats;
			for (auto& prop : mProperties)
			{
				if (prop.IsHidden()) continue;
				std::string cat = prop.GetCategory();
				bool found = false;
				for (auto& c : cats)
				{
					if (c == cat) { found = true; break; }
				}
				if (!found) cats.push_back(cat);
			}
			return cats;
		}

		// Get properties for a specific category
		std::vector<const PropertyDescriptor*> GetPropertiesInCategory(const char* category) const
		{
			std::vector<const PropertyDescriptor*> result;
			for (auto& prop : mProperties)
			{
				if (prop.IsHidden()) continue;
				if (strcmp(prop.GetCategory(), category) == 0)
					result.push_back(&prop);
			}
			return result;
		}

	private:
		const char*						mClassName		= nullptr;
		const char*						mParentClassName = nullptr;
		std::vector<PropertyDescriptor>	mProperties;
		std::vector<FunctionDescriptor>	mFunctions;
		MetaMap							mMeta;
	};

	// ============================================================
	// TypeRegistry — global singleton for type lookup
	// ============================================================
	class ELAINE_REFLECTION_API TypeRegistry
	{
	public:
		static TypeRegistry& Instance()
		{
			static TypeRegistry sInstance;
			return sInstance;
		}

		void Register(TypeDescriptor* desc)
		{
			if (desc && desc->GetClassName())
			{
				mTypes[desc->GetClassName()] = desc;
			}
		}

		TypeDescriptor* Find(const char* className) const
		{
			auto it = mTypes.find(className);
			return (it != mTypes.end()) ? it->second : nullptr;
		}

		TypeDescriptor* Find(const std::string& className) const
		{
			return Find(className.c_str());
		}

		const std::unordered_map<std::string, TypeDescriptor*>& GetAllTypes() const
		{
			return mTypes;
		}

	private:
		TypeRegistry() = default;
		~TypeRegistry() = default;
		TypeRegistry(const TypeRegistry&) = delete;
		TypeRegistry& operator=(const TypeRegistry&) = delete;

		std::unordered_map<std::string, TypeDescriptor*> mTypes;
	};

	// ============================================================
	// AutoRegister — helper used in generated code for static init
	// ============================================================
	struct ELAINE_REFLECTION_API AutoTypeRegister
	{
		AutoTypeRegister(TypeDescriptor* desc)
		{
			TypeRegistry::Instance().Register(desc);
		}
	};
}

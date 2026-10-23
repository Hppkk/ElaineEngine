#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <cstddef>
#include <functional>
#include <cstring>
#include <memory>
#include <utility>

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
    struct PropertyDescriptor;
    enum class PropertyChangeType { ValueSet, Interactive, EditFinished };
    struct PropertyChangedEvent
    {
        const PropertyDescriptor* Property = nullptr;
        std::string MemberPath;
        PropertyChangeType Type = PropertyChangeType::ValueSet;
        bool IsProperty(const char* Name) const;
    };

	struct ELAINE_REFLECTION_API PropertyDescriptor
	{
		const char* Name		= nullptr;
		const char* TypeName	= nullptr;
		size_t		Offset		= 0;
		size_t		Size		= 0;
		MetaMap		Meta;		// Metadata from EPROPERTY(...) arguments

        // Generated typed operations: never byte-copy strings or nested objects.
        void* (*CopyValue)(const void*) = nullptr;
        void (*DestroyValue)(void*) = nullptr;
        void (*AssignValue)(void*, const void*) = nullptr;
        bool (*EqualValue)(const void*, const void*) = nullptr;
        template<class T> void SetValueOperations()
        {
            CopyValue = [](const void* Value) -> void* { return new T(*static_cast<const T*>(Value)); };
            DestroyValue = [](void* Value) { delete static_cast<T*>(Value); };
            AssignValue = [](void* To, const void* From) { *static_cast<T*>(To) = *static_cast<const T*>(From); };
            EqualValue = [](const void* A, const void* B) {
                if constexpr (requires(const T& X, const T& Y) { X == Y; })
                    return *static_cast<const T*>(A) == *static_cast<const T*>(B);
                else return false;
            };
        }

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

    inline bool PropertyChangedEvent::IsProperty(const char* Name) const
    { return Property && Property->Name && std::strcmp(Property->Name, Name) == 0; }

    // Owns a typed editor copy. The live object is changed only at commit.
    class PropertyEditValue
    {
    public:
        PropertyEditValue(const PropertyDescriptor& Property, const void* Object)
            : mProperty(Property), mValue(Property.CopyValue ? Property.CopyValue(Property.GetPtrIn(Object)) : nullptr) {}
        ~PropertyEditValue() { if (mValue) mProperty.DestroyValue(mValue); }
        PropertyEditValue(const PropertyEditValue&) = delete;
        PropertyEditValue& operator=(const PropertyEditValue&) = delete;
        void* Get() const { return mValue; }
    private:
        const PropertyDescriptor& mProperty;
        void* mValue;
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
        template<class T> void SetEditNotifier()
        {
            mEditNotifier = [](void* Object, const PropertyChangedEvent& Event) {
                if constexpr (requires(T& Value) { Value.PostEditChangeProperty(Event); })
                    static_cast<T*>(Object)->PostEditChangeProperty(Event);
            };
        }
        void NotifyPropertyChanged(void* Object, const PropertyChangedEvent& Event) const
        { if (mEditNotifier) mEditNotifier(Object, Event); }

        bool CommitPropertyEdit(void* Object, const PropertyDescriptor& Property, const void* Value,
            const std::string& MemberPath = {}, PropertyChangeType Change = PropertyChangeType::ValueSet) const
        {
            if (!Object || !Value || Property.IsReadOnly() || !Property.AssignValue) return false;
            void* Destination = Property.GetPtrIn(Object);
            if (Property.EqualValue && Property.EqualValue(Destination, Value)) return false;
            Property.AssignValue(Destination, Value);
            NotifyPropertyChanged(Object, {&Property, MemberPath.empty() ? Property.Name : MemberPath, Change});
            return true;
        }

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
		void (*mEditNotifier)(void*, const PropertyChangedEvent&) = nullptr;
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

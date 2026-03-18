#pragma once
#include <string>
#include <unordered_map>
#include <functional>

namespace Editor
{
	// ============================================================
	// PropertyDrawerRegistry — maps type names to auto-generated
	// ImGui draw functions. Generated .editor.generated.cpp files
	// register their draw functions here at static-init time.
	// ============================================================

	// Draw function signature: takes void* to object, returns true if changed
	using PropertyDrawFunc = std::function<bool(void* obj)>;

	class PropertyDrawerRegistry
	{
	public:
		static PropertyDrawerRegistry& Instance()
		{
			static PropertyDrawerRegistry sInstance;
			return sInstance;
		}

		void Register(const std::string& className, PropertyDrawFunc func)
		{
			mDrawers[className] = func;
		}

		// Draw properties for an object given its type name
		bool Draw(const std::string& className, void* obj) const
		{
			auto it = mDrawers.find(className);
			if (it != mDrawers.end())
				return it->second(obj);
			return false;
		}

		bool HasDrawer(const std::string& className) const
		{
			return mDrawers.find(className) != mDrawers.end();
		}

		const std::unordered_map<std::string, PropertyDrawFunc>& GetAll() const
		{
			return mDrawers;
		}

	private:
		PropertyDrawerRegistry() = default;
		std::unordered_map<std::string, PropertyDrawFunc> mDrawers;
	};

	// Helper for static registration from generated code
	struct AutoDrawerRegister
	{
		AutoDrawerRegister(const char* className, PropertyDrawFunc func)
		{
			PropertyDrawerRegistry::Instance().Register(className, func);
		}
	};
}

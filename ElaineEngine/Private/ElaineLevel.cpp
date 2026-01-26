#include "ElainePrecompiledHeader.h"
#include "ElaineLevel.h"
#include "ElaineWorld.h"
#include "ElaineDataStream.h"
#include "ElaineGameObjectInfoMgr.h"
#include "ElaineGameObjectMgr.h"

namespace Elaine
{
	Level::Level(World* InWorld)
		: mWorld(InWorld)
	{

	}

	Level::~Level()
	{

	}

	void Level::SetWorldOffset(const Vector3& InOffset)
	{
		if (InOffset == mWorldOffset)
			return;

		mWorldOffset = InOffset;
		mWorldOrigin = mWorld->GetWorldOrigin() + mWorldOffset;
	}

	bool Level::Load(const std::string& InPath)
	{
		// Load level JSON (can be a full level or a chunk file)
		std::string FullPath = Root::instance()->GetResourcePath() + InPath;
		DataStream JsonFileStream(FullPath, DataStream::Out);
		JsonFileStream.ReadAll();
		JsonCpp JsonData(JsonFileStream.GetDataStream());

		if (JsonData.is_null())
			return false;

		if (JsonData.contains("WorldOffset"))
		{
			const auto& arr = JsonData["WorldOffset"];
			if (arr.is_array() && arr.size() >= 3)
			{
				Vector3 offset;
				offset.x = arr[0].get<float>();
				offset.y = arr[1].get<float>();
				offset.z = arr[2].get<float>();
				SetWorldOffset(offset);
			}
		}

		// Load inline gameobjects
		if (JsonData.contains("GameObjectArray"))
		{
			for (const auto& goJson : JsonData["GameObjectArray"])
			{
				GameObjectInfoPtr NewInfo = GameObjectInfoMgr::instance()->GetResource<GameObjectInfo>("");
				NewInfo->ImportData(goJson);

				GameObject* NewGO = mWorld->GetGameObjectMgr()->CreateGameObjectByInfo(NewInfo);
				mGameObjects.push_back(NewGO);
			}
		}

		// Or load references to external gameobject resources
		if (JsonData.contains("GameObjectRefs"))
		{
			for (const auto& pathNode : JsonData["GameObjectRefs"])
			{
				if (!pathNode.is_string())
					continue;
				std::string goPath = pathNode.get<std::string>();
				GameObject* NewGO = mWorld->GetGameObjectMgr()->CreateGameObjectByInfo(goPath, false);
				mGameObjects.push_back(NewGO);
			}
		}

		return true;
	}

	void Level::OnLevelLoaded(World* InWorld)
	{

	}

	void Level::OnLevelUnloaded()
	{

	}
}
#pragma once
#include <functional>

#include "create_functions.h"
#include "halley/file_formats/config_file.h"
#include "halley/data_structures/maybe.h"
#include "halley/entity/entity.h"

namespace Halley {
	class World;
	class Resources;
	class EntityScene;
	
	class EntityFactory {
	public:
		enum class UpdateMode {
			//TransformOnly,
			UpdateAll,
			UpdateAllDeleteOld
		};
		
		explicit EntityFactory(World& world, Resources& resources);
		virtual ~EntityFactory();

		EntityRef createEntity(const char* prefabName);
		EntityRef createEntity(const String& prefabName);
		EntityRef createEntity(const ConfigNode& node);
		EntityRef createPrefab(std::shared_ptr<const Prefab> prefab);
		EntityScene createScene(std::shared_ptr<const Prefab> scene);
		
		void updateEntityTree(EntityRef& entity, const ConfigNode& node);
		void updateScene(std::vector<EntityRef>& entities, const ConfigNode& node);

		ConfigNode serializeEntity(EntityRef entity);

		template <typename T>
		CreateComponentFunctionResult createComponent(EntityRef& e, const ConfigNode& componentData)
		{
			CreateComponentFunctionResult result;
			result.componentId = T::componentIndex;
			
			auto comp = e.tryGetComponent<T>();
			if (comp) {
				comp->deserialize(context, componentData);
			} else {
				T component;
				component.deserialize(context, componentData);
				e.addComponent<T>(std::move(component));
				result.created = true;
			}

			return result;
		}


	private:
		World& world;
		Resources& resources;
		ConfigNodeSerializationContext context;

		ConfigNode dummyPrefab;

		void createEntityTreeForScene(const ConfigNode& node, EntityScene& curScene, std::shared_ptr<const Prefab> prefab, std::optional<int> index = {});
		EntityRef createEntityTree(const ConfigNode& node, EntityScene* curScene);
		EntityRef createEntity(std::optional<EntityRef> parent, const ConfigNode& node, bool populate, EntityScene* curScene);
		
		void updateEntity(EntityRef& entity, const ConfigNode& node, UpdateMode mode = UpdateMode::UpdateAll);
		void doUpdateEntityTree(EntityRef& entity, const ConfigNode& node, bool refreshing);
		
		std::shared_ptr<const Prefab> getPrefab(const String& id) const;
		const ConfigNode& getPrefabNode(const String& id) const;

		void startContext();
		ConfigNodeSerializationContext makeContext() const;
	};

	class EntitySerializationContext {
	public:
		World& world;
		std::map<UUID, EntityId> uuids;

		EntitySerializationContext(World& world);

		void clear();
	};
}

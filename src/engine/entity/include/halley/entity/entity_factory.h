#pragma once
#include <functional>

#include "create_functions.h"
#include "prefab.h"
#include "halley/file_formats/config_file.h"
#include "halley/data_structures/maybe.h"
#include "halley/entity/entity.h"

namespace Halley {
	class World;
	class Resources;
	class EntityScene;
	class EntityData;
	
	class EntityFactory {
	public:
		enum class UpdateMode {
			//TransformOnly,
			UpdateAll,
			UpdateAllDeleteOld
		};

		struct SerializationOptions {
			EntitySerialization::Type type = EntitySerialization::Type::Undefined;
			std::function<bool(EntityRef)> serializeAsStub;

			SerializationOptions() = default;
			explicit SerializationOptions(EntitySerialization::Type type, std::function<bool(EntityRef)> serializeAsStub = {})
				: type(type)
				, serializeAsStub(std::move(serializeAsStub))
			{}
		};

		explicit EntityFactory(World& world, Resources& resources);
		virtual ~EntityFactory();

		EntityRef createEntity(const char* prefabName);
		EntityRef createEntity(const String& prefabName);
		EntityRef createEntity(const std::shared_ptr<const Prefab>& prefab);
		EntityRef createEntity(const EntityData& data, EntityRef parent = EntityRef());
		EntityScene createScene(const std::shared_ptr<const Prefab>& scene);

		void updateEntity(EntityRef& entity, const EntityData& data);
		void updateScene(std::vector<EntityRef>& entities, const std::shared_ptr<const Prefab>& scene, EntitySerialization::Type sourceType);

		EntityData serializeEntity(EntityRef entity, const SerializationOptions& options, bool canStoreParent = true);

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

		EntityRef createEntity(const EntityData& data, EntityRef parent, const std::shared_ptr<const Prefab>& prevPrefab);
		EntityRef doCreateEntity(const EntityData& data, EntityRef parent, const std::shared_ptr<const Prefab>& prefab);

		std::shared_ptr<const Prefab> getPrefab(const String& id) const;
		EntityData getEntityData(const EntityData& src, const std::shared_ptr<const Prefab>& prefab) const;

		void startContext(EntitySerialization::Type sourceType);
		ConfigNodeSerializationContext makeContext() const;
	};

	class EntitySerializationContext {
	public:
		World& world;
		std::map<UUID, EntityId> uuids;
		std::vector<std::map<UUID, UUID>> uuidMapping; //prefab -> instance

		EntitySerializationContext(World& world);

		void clear();
	};
}

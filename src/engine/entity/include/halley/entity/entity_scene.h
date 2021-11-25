#pragma once

#include <memory>
#include <vector>

#include "entity.h"
#include "prefab.h"

namespace Halley {
	class EntityFactory;
	class ConfigFile;
	
	class EntityScene {
	public:
		class IEntitySceneUpdateCallbacks {
		public:
			virtual ~IEntitySceneUpdateCallbacks() = default;

			virtual void onEntityAdded(EntityRef entity, const EntityData& data, uint8_t worldPartition) = 0;
			virtual void onEntityUpdated(EntityRef entity, const EntityDataDelta& data, uint8_t worldPartition) = 0;
			virtual void onEntityRemoved(EntityRef entity, uint8_t worldPartition) = 0;
		};
		
		explicit EntityScene(bool allowReload = false, uint8_t worldPartition = 0);
		
		std::vector<EntityRef>& getEntities();
		const std::vector<EntityRef>& getEntities() const;

		bool needsUpdate() const;
		void update(EntityFactory& factory, IEntitySceneUpdateCallbacks* callbacks = nullptr);

		void addPrefabReference(const std::shared_ptr<const Prefab>& prefab, const EntityRef& entity);
		void addRootEntity(EntityRef entity);

		uint8_t getWorldPartition() const;
		void validate(uint8_t worldPartition);

	private:
		class PrefabObserver {
		public:			
			PrefabObserver(std::shared_ptr<const Prefab> prefab);
			
			bool needsUpdate() const;
			
			void updateEntities(EntityFactory& factory, EntityScene& scene, IEntitySceneUpdateCallbacks* callbacks, uint8_t worldPartition) const;
			void markUpdated();

			void addEntity(EntityRef entity);

			const std::shared_ptr<const Prefab>& getPrefab() const;

		private:
			std::shared_ptr<const Prefab> prefab;
			std::vector<EntityId> entityIds;
			int assetVersion = 0;

			std::vector<EntityRef> getEntities(World& world) const;
		};

		std::vector<EntityRef> entities;
		std::vector<PrefabObserver> prefabObservers;
		std::vector<PrefabObserver> sceneObservers;
		bool allowReload = false;
		uint8_t worldPartition = 0;

		PrefabObserver& getOrMakeObserver(const std::shared_ptr<const Prefab>& prefab);
	};
}

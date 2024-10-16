#pragma once

#include <memory>
#include "halley/data_structures/vector.h"

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

			virtual void onEntityAdded(EntityRef entity, const EntityData& data, WorldPartitionId worldPartition) = 0;
			virtual void onEntityUpdated(EntityRef entity, const EntityDataDelta& data, WorldPartitionId worldPartition) = 0;
			virtual void onEntityRemoved(EntityRef entity, WorldPartitionId worldPartition) = 0;
		};
		
		EntityScene(bool allowReload = false, WorldPartitionId worldPartition = 0, String variant = "");
		
		Vector<EntityId>& getEntities();
		const Vector<EntityId>& getEntities() const;

		const String& getVariant() const;

		bool needsUpdate() const;
		void update(EntityFactory& factory, IEntitySceneUpdateCallbacks* callbacks = nullptr);

		void addPrefabReference(const std::shared_ptr<const Prefab>& prefab, const EntityRef& entity);
		void addRootEntity(EntityRef entity);

		WorldPartitionId getWorldPartition() const;
		void validate(WorldPartitionId worldPartition, World& world);

		void destroyEntities(World& world);

	private:
		class PrefabObserver {
		public:			
			PrefabObserver(std::shared_ptr<const Prefab> prefab);
			
			bool needsUpdate() const;
			
			void updateEntities(EntityFactory& factory, EntityScene& scene, IEntitySceneUpdateCallbacks* callbacks, WorldPartitionId worldPartition);
			void markUpdated();

			void addEntity(EntityRef entity);

			const std::shared_ptr<const Prefab>& getPrefab() const;

		private:
			std::shared_ptr<const Prefab> prefab;
			Vector<EntityId> entityIds;
			int assetVersion = 0;

			Vector<EntityRef> getEntities(World& world) const;
		};

		Vector<EntityId> entities;
		Vector<PrefabObserver> prefabObservers;
		Vector<PrefabObserver> sceneObservers;
		String variant;
		bool allowReload = false;
		WorldPartitionId worldPartition = 0;

		PrefabObserver& getOrMakeObserver(const std::shared_ptr<const Prefab>& prefab);
	};
}

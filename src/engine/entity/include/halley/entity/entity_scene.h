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
		explicit EntityScene(bool allowReload = false, uint8_t worldPartition = 0);
		
		std::vector<EntityRef>& getEntities();
		const std::vector<EntityRef>& getEntities() const;

		bool needsUpdate() const;
		void update(EntityFactory& factory);
		void updateOnEditor(EntityFactory& factory);

		void addPrefabReference(const std::shared_ptr<const Prefab>& prefab, const EntityRef& entity);
		void addRootEntity(EntityRef entity);

		uint8_t getWorldPartition() const;

	private:
		class PrefabObserver {
		public:			
			PrefabObserver(std::shared_ptr<const Prefab> prefab);
			
			bool needsUpdate() const;
			
			void updateEntities(EntityFactory& factory,EntityScene& scene) const;
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

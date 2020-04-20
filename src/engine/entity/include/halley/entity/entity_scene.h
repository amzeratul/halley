#pragma once

#include <memory>
#include <vector>

#include "entity.h"

namespace Halley {
	class EntityFactory;
	class ConfigFile;
	
	class EntityScene {
	public:
		std::vector<EntityRef>& getEntities();
		const std::vector<EntityRef>& getEntities() const;

		bool needsUpdate() const;
		void update(EntityFactory& factory);

		void addPrefabReference(const std::shared_ptr<const Prefab>& prefab, const EntityRef& entity, std::optional<int> index = {});
		void addRootEntity(EntityRef entity);

	private:
		class ObservedEntity {
		public:
			EntityRef entity;
			std::optional<int> index;

			ObservedEntity(EntityRef entity, std::optional<int> index);
		};
		
		class PrefabObserver {
		public:
			std::shared_ptr<const ConfigFile> config;
			int assetVersion = 0;
			std::vector<ObservedEntity> entities;

			PrefabObserver(std::shared_ptr<const ConfigFile> config);
			bool needsUpdate() const;
			void update(EntityFactory& factory);
		};

		std::vector<EntityRef> entities;
		std::vector<PrefabObserver> prefabObservers;

		PrefabObserver& getOrMakeObserver(const std::shared_ptr<const ConfigFile>& config);
	};
}

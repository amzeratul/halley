#pragma once

#include "halley/file_formats/config_file.h"
#include "entity_data_delta.h"

namespace Halley {	
	class Prefab : public ConfigFile {
	public:
		static std::unique_ptr<Prefab> loadResource(ResourceLoader& loader);
		constexpr static AssetType getAssetType() { return AssetType::Prefab; }

		void reload(Resource&& resource) override;
		void makeDefault();

		virtual bool isScene() const;

		const EntityData& getEntityData() const;
		const std::vector<EntityData>& getEntityDatas() const;
		std::map<UUID, const EntityData*> getEntityDataMap() const;

		const std::map<UUID, EntityDataDelta>& getEntitiesModified() const;
		const std::set<UUID>& getEntitiesRemoved() const;

	protected:
		void loadEntityData();
		virtual std::vector<EntityData> makeEntityDatas() const;
		std::vector<EntityData> entityDatas;
		
		std::map<UUID, EntityDataDelta> entitiesModified;
		std::set<UUID> entitiesRemoved;
	};

	class Scene final : public Prefab {
	public:
		static std::unique_ptr<Scene> loadResource(ResourceLoader& loader);
		constexpr static AssetType getAssetType() { return AssetType::Scene; }

		bool isScene() const override;
		
		void reload(Resource&& resource) override;
		void makeDefault();

	protected:
		std::vector<EntityData> makeEntityDatas() const override;
	};
}

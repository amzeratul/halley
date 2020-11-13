#pragma once

#include "halley/file_formats/config_file.h"
#include "entity_data_delta.h"

namespace Halley {	
	class Prefab : public Resource {
	public:
		static std::unique_ptr<Prefab> loadResource(ResourceLoader& loader);
		constexpr static AssetType getAssetType() { return AssetType::Prefab; }

		void reload(Resource&& resource) override;
		void makeDefault();

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

		void parseYAML(gsl::span<const gsl::byte> yaml);
		String toYAML() const;

		virtual bool isScene() const;

		const EntityData& getEntityData() const;
		const std::vector<EntityData>& getEntityDatas() const;
		std::map<UUID, const EntityData*> getEntityDataMap() const;

		const std::map<UUID, EntityDataDelta>& getEntitiesModified() const;
		const std::set<UUID>& getEntitiesAdded() const;
		const std::set<UUID>& getEntitiesRemoved() const;

		const ConfigNode& getRoot() const;
		ConfigNode& getRoot();

	protected:
		struct Deltas {
			std::map<UUID, EntityDataDelta> entitiesModified;
			std::set<UUID> entitiesAdded;
			std::set<UUID> entitiesRemoved;
		};

		void loadEntityData();
		virtual std::vector<EntityData> makeEntityDatas() const;
		Deltas generatePrefabDeltas(const Prefab& newPrefab) const;
		
		std::vector<EntityData> entityDatas;
		ConfigFile config;

		Deltas deltas;
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
		Deltas generateSceneDeltas(const Scene& newScene) const;
	};
}

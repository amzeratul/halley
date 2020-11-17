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

		EntityData& getEntityData();
		const EntityData& getEntityData() const;
		virtual gsl::span<const EntityData> getEntityDatas() const;
		virtual gsl::span<EntityData> getEntityDatas();
		std::map<UUID, const EntityData*> getEntityDataMap() const;

		const std::map<UUID, EntityDataDelta>& getEntitiesModified() const;
		const std::set<UUID>& getEntitiesAdded() const;
		const std::set<UUID>& getEntitiesRemoved() const;

		ConfigNode& getGameData(const String& key);
		const ConfigNode* tryGetGameData(const String& key) const;

		virtual String getPrefabName() const;

		EntityData* findEntityData(const UUID& uuid);

	protected:
		struct Deltas {
			std::map<UUID, EntityDataDelta> entitiesModified;
			std::set<UUID> entitiesAdded;
			std::set<UUID> entitiesRemoved;
		};

		void loadEntityData();
		virtual EntityData makeEntityData() const;
		Deltas generatePrefabDeltas(const Prefab& newPrefab) const;
		
		EntityData entityData;
		ConfigFile config;
		ConfigFile gameData;

		Deltas deltas;

	private:
		ConfigNode& getEntityNodeRoot();
	};

	class Scene final : public Prefab {
	public:
		static std::unique_ptr<Scene> loadResource(ResourceLoader& loader);
		constexpr static AssetType getAssetType() { return AssetType::Scene; }

		bool isScene() const override;
		
		void reload(Resource&& resource) override;
		void makeDefault();

		gsl::span<const EntityData> getEntityDatas() const override;
		gsl::span<EntityData> getEntityDatas() override;
		String getPrefabName() const override;
		
	protected:
		EntityData makeEntityData() const override;
		Deltas generateSceneDeltas(const Scene& newScene) const;
	};
}

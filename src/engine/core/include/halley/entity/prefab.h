#pragma once

#include "halley/file_formats/config_file.h"
#include "entity_data_delta.h"
#include "halley/lua/lua_reference.h"

namespace Halley {
	class SceneVariant;

	class Prefab : public AsyncResource {
	public:		
		static std::shared_ptr<Prefab> loadResource(ResourceLoader& loader);
		constexpr static AssetType getAssetType() { return AssetType::Prefab; }

		virtual AssetType getPrefabType() const { return AssetType::Prefab; }

		void reload(Resource&& resource) override;
		void makeDefault();

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

		void parseYAML(gsl::span<const gsl::byte> yaml);
		String toYAML() const;

		void parseConfigNode(const ConfigNode& node);
		virtual ConfigNode toConfigNode() const;

		virtual bool isScene() const;

		EntityData& getEntityData();
		const EntityData& getEntityData() const;
		virtual gsl::span<const EntityData> getEntityDatas() const;
		virtual gsl::span<EntityData> getEntityDatas();
		std::map<UUID, const EntityData*> getEntityDataMap() const;

		const std::map<UUID, EntityDataDelta>& getEntitiesModified() const;
		const std::set<UUID>& getEntitiesAdded() const;
		const std::set<UUID>& getEntitiesRemoved() const;

		void setGameData(const String& key, ConfigNode data);
		void removeGameData(const String& key);
		ConfigNode& getGameData(const String& key);
		const ConfigNode* tryGetGameData(const String& key) const;

		virtual String getPrefabName() const;
		String getPrefabIcon() const;

		EntityData* findEntityData(const UUID& uuid);

		virtual std::shared_ptr<Prefab> clone() const;

		void preloadDependencies(Resources& resources) const;

		ResourceMemoryUsage getMemoryUsage() const override;

		void generateUUIDs();

	protected:
		struct Deltas {
			std::map<UUID, EntityDataDelta> entitiesModified;
			std::set<UUID> entitiesAdded;
			std::set<UUID> entitiesRemoved;
		};

		virtual EntityData makeEntityData(const ConfigNode& node) const;
		Deltas generatePrefabDeltas(const Prefab& newPrefab) const;
		virtual ConfigNode entityToConfigNode() const;
		
		EntityData entityData;
		ConfigFile gameData;

		Deltas deltas;

		void doPreloadDependencies(const EntityData& entityData, Resources& resources) const;
	};

	class Scene final : public Prefab {
	public:
		static std::shared_ptr<Scene> loadResource(ResourceLoader& loader);
		constexpr static AssetType getAssetType() { return AssetType::Scene; }

		AssetType getPrefabType() const override { return AssetType::Scene; }

		bool isScene() const override;
		
		void reload(Resource&& resource) override;
		void makeDefault();

		gsl::span<const EntityData> getEntityDatas() const override;
		gsl::span<EntityData> getEntityDatas() override;
		String getPrefabName() const override;

		std::shared_ptr<Prefab> clone() const override;

		ConfigNode entityToConfigNode() const override;

		Vector<SceneVariant> getVariants() const;

	protected:
		EntityData makeEntityData(const ConfigNode& node) const override;
		Deltas generateSceneDeltas(const Scene& newScene) const;
	};

	class SceneVariant {
	public:
		String id;
		LuaExpression conditions;

		SceneVariant() = default;
		SceneVariant(String id, LuaExpression conditions = {});
		SceneVariant(const ConfigNode& node);

		ConfigNode toConfigNode() const;
	};
}

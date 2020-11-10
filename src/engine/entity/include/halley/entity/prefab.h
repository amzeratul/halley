#pragma once

#include "halley/file_formats/config_file.h"

namespace Halley {
	class Prefab : public ConfigFile {
	public:
		static std::unique_ptr<Prefab> loadResource(ResourceLoader& loader);
		constexpr static AssetType getAssetType() { return AssetType::Prefab; }

		void reload(Resource&& resource) override;
		void makeDefault();

		const EntityData& getEntityData() const;
		const std::vector<EntityData>& getEntityDatas() const;

	protected:
		void loadEntityData();
		virtual std::vector<EntityData> makeEntityDatas() const;
		std::vector<EntityData> entityDatas;
	};

	class Scene final : public Prefab {
	public:
		static std::unique_ptr<Scene> loadResource(ResourceLoader& loader);
		constexpr static AssetType getAssetType() { return AssetType::Scene; }

		void reload(Resource&& resource) override;
		void makeDefault();

	protected:
		std::vector<EntityData> makeEntityDatas() const override;
	};
}

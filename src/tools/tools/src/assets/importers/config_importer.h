#pragma once
#include "halley/plugin/iasset_importer.h"
#include "halley/file_formats/config_file.h"

namespace Halley
{
	class Prefab;
	class Project;
	class ConfigFile;

	class ConfigImporter : public IAssetImporter
	{
	public:
		ImportAssetType getType() const override { return ImportAssetType::ConfigFile; }

		void import(const ImportingAsset& asset, IAssetCollector& collector) override;
	};

	class PrefabImporter : public IAssetImporter
	{
	public:
		PrefabImporter(Project& project);

		ImportAssetType getType() const override { return ImportAssetType::Prefab; }

		void import(const ImportingAsset& asset, IAssetCollector& collector) override;

		static void validateEntityData(Project& project, Prefab& prefab);

	private:
		Project& project;
	};

	class SceneImporter : public IAssetImporter
	{
	public:
		SceneImporter(Project& project);

		ImportAssetType getType() const override { return ImportAssetType::Scene; }

		void import(const ImportingAsset& asset, IAssetCollector& collector) override;

	private:
		Project& project;
	};

	class NavmeshSetImporter : public IAssetImporter
	{
	public:
		ImportAssetType getType() const override { return ImportAssetType::NavmeshSet; }

		void import(const ImportingAsset& asset, IAssetCollector& collector) override;
	};
}

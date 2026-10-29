#include "config_importer.h"

#include "halley/entity/prefab.h"
#include "halley/file_formats/config_file.h"
#include "halley/file_formats/yaml_convert.h"
#include "halley/navigation/navmesh_set.h"
#include "halley/tools/ecs/ecs_data.h"
#include "halley/tools/project/project.h"

using namespace Halley;

void ConfigImporter::import(const ImportingAsset& asset, IAssetCollector& collector)
{
	YAMLConvert::ParseOptions options;
	options.assetId = asset.inputFiles.at(0).name.getNativeString(false);
	options.throwOnError = true;
	ConfigFile config = YAMLConvert::parseConfig(gsl::as_bytes(gsl::span<const Byte>(asset.inputFiles.at(0).data)), options);
	
	Metadata meta = asset.inputFiles.at(0).metadata;
	meta.set("asset_compression", "lz4");

	collector.output(Path(asset.assetId).replaceExtension("").string(), AssetType::ConfigFile, Serializer::toBytes(config), meta);
}

PrefabImporter::PrefabImporter(Project& project)
	: project(project)
{
}

void PrefabImporter::import(const ImportingAsset& asset, IAssetCollector& collector)
{
	Prefab prefab;
	prefab.parseYAML(gsl::as_bytes(gsl::span<const Byte>(asset.inputFiles.at(0).data)));

	validateEntityData(project, prefab);

	Metadata meta = asset.inputFiles.at(0).metadata;
	meta.set("asset_compression", "lz4");

	collector.output(Path(asset.assetId).replaceExtension("").string(), AssetType::Prefab, Serializer::toBytes(prefab, SerializerOptions(SerializerOptions::maxVersion)), meta);
}

void PrefabImporter::validateEntityData(Project& project, Prefab& prefab)
{
	auto& ecsData = project.getECSData();

	class Callback : public IComponentConfigNodeFormatCallback {
	public:
		Callback(ECSData& ecsData)
			: ecsData(ecsData)
		{}

		void process(std::string_view componentName, std::string_view fieldName, ConfigNode& data) const override
		{
			const auto compIter = ecsData.getComponents().find(componentName);
			if (compIter != ecsData.getComponents().end()) {
				for (const auto& member: compIter->second.members) {
					if (member.name == fieldName) {
						processField(data, member.type.name);
					}
				}
			}
		}

	private:
		ECSData& ecsData;

		static void toFloat2(ConfigNode& data)
		{
			const auto& seq = data.asSequence();
			if (seq.size() == 2) {
				Vector2f v;
				v.x = seq[0].asFloat();
				v.y = seq[1].asFloat();
				data = v;
			}
		}

		static void toInt2(ConfigNode& data)
		{
			const auto& seq = data.asSequence();
			if (seq.size() == 2) {
				Vector2i v;
				v.x = seq[0].asInt();
				v.y = seq[1].asInt();
				data = v;
			}
		}

		static bool processField(ConfigNode& data, std::string_view typeName)
		{
			if (data.getType() == ConfigNodeType::Sequence) {
				if (typeName == "Halley::Vector2f" || typeName == "Vector2f") {
					toFloat2(data);
					return true;
				} else if (typeName == "Halley::Vector2i" || typeName == "Vector2i") {
					toInt2(data);
					return true;
				} else if (typeName == "Halley::Vector<Vector2f>" || typeName == "Halley::VertexList" || typeName == "Halley::Polygon" || typeName == "WorldPolygon") {
					for (auto& s: data.asSequence()) {
						toFloat2(s);
					}
					return true;
				}
			}
			return false;
		}
	};

	const auto callback = Callback(ecsData);
	for (auto& entityData: prefab.getEntityDatas()) {
		entityData.ensureComponentConfigNodeFormat(callback);
	}
}

SceneImporter::SceneImporter(Project& project)
	: project(project)
{
}

void SceneImporter::import(const ImportingAsset& asset, IAssetCollector& collector)
{
	Scene scene;
	scene.parseYAML(gsl::as_bytes(gsl::span<const Byte>(asset.inputFiles.at(0).data)));

	PrefabImporter::validateEntityData(project, scene);

	Metadata meta = asset.inputFiles.at(0).metadata;
	meta.set("asset_compression", "lz4");

	collector.output(Path(asset.assetId).replaceExtension("").string(), AssetType::Scene, Serializer::toBytes(scene, SerializerOptions(SerializerOptions::maxVersion)), meta);
}

void NavmeshSetImporter::import(const ImportingAsset& asset, IAssetCollector& collector)
{
	ConfigFile config = YAMLConvert::parseConfig(gsl::as_bytes(gsl::span<const Byte>(asset.inputFiles.at(0).data)));
	auto navmeshSet = NavmeshSet(config.getRoot());

	Metadata meta = asset.inputFiles.at(0).metadata;
	meta.set("asset_compression", "lz4");

	collector.output(Path(asset.assetId).replaceExtension("").string(), AssetType::NavmeshSet, Serializer::toBytes(navmeshSet, SerializerOptions(SerializerOptions::maxVersion)), meta);
}
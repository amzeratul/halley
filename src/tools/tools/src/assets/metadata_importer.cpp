#include "halley/tools/assets/metadata_importer.h"
#include "halley/resources/resource_data.h"
#include "halley/file_formats/halley-yamlcpp.h"
#include "halley/data_structures/maybe.h"
#include "halley/file_formats/yaml_convert.h"
#include "halley/text/string_converter.h"
using namespace Halley;

static void loadMetaTable(Metadata& meta, const ConfigNode& root, bool dirMeta)
{
	if (root.getType() == ConfigNodeType::Map) {
		for (const auto& [key, value] : root.asMap()) {
			meta.set(key, ConfigNode(value));
			if (dirMeta) {
				meta.set(":" + key, ConfigNode(value));
			}
		}
	}
}

void MetadataImporter::loadMetaData(Metadata& meta, const Path& path, bool isDirectoryMeta, const Path& inputFilePath)
{
	const auto data = ResourceDataStatic::loadFromFileSystem(path);
	const auto configFile = YAMLConvert::parseConfig(data ? data->getSpan() : gsl::span<const gsl::byte>());
	const auto& root = configFile.getRoot();

	const String inputFilePathStr = inputFilePath.toString();

	if (isDirectoryMeta) {
		for (const auto& rootList: root) {
			bool matches = true;
			if (rootList.hasKey("match")) {
				matches = false;
				for (auto& pattern: rootList["match"]) {
					auto p = pattern.asString();
					if (inputFilePathStr.contains(p)) {
						matches = true;
						break;
					}
				}
			}
			if (matches && rootList.hasKey("data")) {
				loadMetaTable(meta, rootList["data"], true);
				return;
			}
		}
	} else {
		loadMetaTable(meta, root, false);
	}
}

Metadata MetadataImporter::getMetaData(const Path& inputFilePath, std::optional<Path> dirMetaPath, std::optional<Path> privateMetaPath)
{
	Metadata meta;
	try {
		if (dirMetaPath) {
			loadMetaData(meta, dirMetaPath.value(), true, inputFilePath);
		}
		if (privateMetaPath) {
			loadMetaData(meta, privateMetaPath.value(), false, inputFilePath);
		}
	} catch (std::exception& e) {
		throw Exception("Error parsing metafile for " + inputFilePath + ": " + e.what(), HalleyExceptions::Tools);
	}

	meta.convertToLatestVersion();
	
	return meta;
}

#include "halley/tools/assets/metadata_importer.h"
#include "halley/resources/resource_data.h"
#include "halley/file_formats/halley-yamlcpp.h"
#include "halley/data_structures/maybe.h"
#include "halley/text/string_converter.h"
using namespace Halley;

static void loadMetaTable(Metadata& meta, const YAML::Node& root, bool dirMeta)
{
	for (YAML::const_iterator it = root.begin(); it != root.end(); ++it) {
		String key = it->first.as<std::string>();
		String value = it->second.as<std::string>();
		meta.set(key, value);
		if (dirMeta) {
			meta.set(":" + key, value);
		}
	}
}

void MetadataImporter::loadMetaData(Metadata& meta, const Path& path, bool isDirectoryMeta, const Path& inputFilePath)
{
	const auto data = ResourceDataStatic::loadFromFileSystem(path);
	auto root = YAML::Load(data ? data->getString() : "");

	const String inputFilePathStr = inputFilePath.toString();

	if (isDirectoryMeta) {
		for (const auto& rootList: root) {
			bool matches = true;
			if (rootList["match"]) {
				matches = false;
				for (auto& pattern: rootList["match"]) {
					auto p = pattern.as<std::string>();
					if (inputFilePathStr.contains(p)) {
						matches = true;
						break;
					}
				}
			}
			if (matches && rootList["data"]) {
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

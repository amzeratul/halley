#include "halley/tools/assets/metadata_importer.h"
#include "halley/resources/resource_data.h"
#include "../yaml/halley-yamlcpp.h"
#include "halley/data_structures/maybe.h"
#include "halley/text/string_converter.h"
using namespace Halley;

static void loadMetaTable(Metadata& meta, const YAML::Node& root)
{
	for (YAML::const_iterator it = root.begin(); it != root.end(); ++it) {
		String key = it->first.as<std::string>();
		String value = it->second.as<std::string>();
		meta.set(key, value);
	}
}

void MetadataImporter::loadMetaData(Metadata& meta, const Path& path, bool isDirectoryMeta, String assetId)
{
	const auto data = ResourceDataStatic::loadFromFileSystem(path);
	auto root = YAML::Load(data ? data->getString() : "");

	if (isDirectoryMeta) {
		for (const auto& rootList: root) {
			bool matches = true;
			if (rootList["match"]) {
				matches = false;
				for (auto& pattern: rootList["match"]) {
					auto p = pattern.as<std::string>();
					if (assetId.contains(p)) {
						matches = true;
						break;
					}
				}
			}
			if (matches && rootList["data"]) {
				loadMetaTable(meta, rootList["data"]);
				return;
			}
		}
	} else {
		loadMetaTable(meta, root);
	}
}

Metadata MetadataImporter::getMetaData(Path inputFilePath, Maybe<Path> dirMetaPath, Maybe<Path> privateMetaPath)
{
	Metadata meta;
	try {
		if (dirMetaPath) {
			loadMetaData(meta, dirMetaPath.get(), true, inputFilePath.toString());
		}
		if (privateMetaPath) {
			loadMetaData(meta, privateMetaPath.get(), false, inputFilePath.toString());
		}
	} catch (std::exception& e) {
		throw Exception("Error parsing metafile for " + inputFilePath + ": " + e.what(), HalleyExceptions::Tools);
	}
	return meta;
}

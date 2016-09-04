#include "halley/tools/assets/iasset_importer.h"
#include "halley/resources/resource_data.h"
#include "halley/resources/metadata.h"
#include "halley/tools/assets/import_assets_database.h"

using namespace Halley;

String IAssetImporter::getAssetId(Path file) const
{
	String name = file.string();
	if (name.endsWith(".meta")) {
		name = name.substr(0, name.length() - 5);
	}
	return name;
}

Path IAssetImporter::getMainFile(const ImportAssetsDatabaseEntry& asset)
{
	for (auto& i : asset.inputFiles) {
		if (i.first.extension() != ".meta") {
			return i.first;
		}
	}
	return Path();
}

std::unique_ptr<Metadata> IAssetImporter::getMetaData(const ImportAssetsDatabaseEntry& asset)
{
	for (auto& i: asset.inputFiles) {
		if (i.first.extension() == ".meta") {
			return std::make_unique<Metadata>(*ResourceDataStatic::loadFromFileSystem(asset.srcDir / i.first));
		}
	}
	return std::unique_ptr<Metadata>();
}

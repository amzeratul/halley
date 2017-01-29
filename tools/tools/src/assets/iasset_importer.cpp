#include "halley/tools/assets/iasset_importer.h"
#include "halley/resources/resource_data.h"
#include "halley/tools/file/filesystem.h"

using namespace Halley;

String IAssetImporter::getAssetId(const Path& file) const
{
	String name = file.string();
	if (name.endsWith(".meta")) {
		name = name.substr(0, name.length() - 5);
	}
	return name;
}

void IAssetImporter::setAssetsSrc(const std::vector<Path>& paths)
{
	assetsSrc = paths;
}

Bytes IAssetImporter::readAdditionalFile(Path filePath) const
{
	for (auto path : assetsSrc) {
		Path f = path / filePath;
		if (FileSystem::exists(f)) {
			return FileSystem::readFile(f);
		}
	}
	throw Exception("Unable to find asset dependency: \"" + filePath.getString() + "\"");
}

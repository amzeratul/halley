#include "halley/tools/assets/iasset_importer.h"
#include "halley/resources/resource_data.h"
#include "halley/resources/metadata.h"

using namespace Halley;

std::unique_ptr<Metadata> IAssetImporter::getMetaData(Path path)
{
	try {
		return std::make_unique<Metadata>(*ResourceDataStatic::loadFromFileSystem(path.string() + ".meta"));
	}
	catch (...) {
		return std::unique_ptr<Metadata>();
	}
}

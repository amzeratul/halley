#include "halley/tools/assets/iasset_importer.h"
#include "halley/resources/resource_data.h"

using namespace Halley;

String IAssetImporter::getAssetId(Path file) const
{
	String name = file.string();
	if (name.endsWith(".meta")) {
		name = name.substr(0, name.length() - 5);
	}
	return name;
}

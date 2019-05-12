#include "mesh_importer.h"
#include "../../mesh/wavefront_reader.h"

using namespace Halley;

void MeshImporter::import(const ImportingAsset& asset, IAssetCollector& collector)
{
	for (auto& f: asset.inputFiles) {
		if (f.name.getExtension() == ".obj") {
			auto reader = std::make_unique<WavefrontReader>();
			std::unique_ptr<Mesh> result = reader->parse(f.data);
			collector.output(asset.assetId, AssetType::Mesh, Serializer::toBytes(*result));
		}
	}
}

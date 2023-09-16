#include "mesh_importer.h"
#include "../../mesh/wavefront_reader.h"
#include "../../mesh/gltf_reader.h"

using namespace Halley;

void MeshImporter::import(const ImportingAsset& asset, IAssetCollector& collector)
{
	for (auto& f: asset.inputFiles) {
		if (f.name.getExtension() == ".obj") {
			auto reader = std::make_unique<WavefrontReader>();
			std::unique_ptr<Mesh> result = reader->parse(f.data);
			collector.output(asset.assetId, AssetType::Mesh, Serializer::toBytes(*result));
		}
		if (f.name.getExtension() == ".glb") {
			auto reader = std::make_unique<GLTFReader>();
			std::unique_ptr<Mesh> result = reader->parse(asset, f.data);
			collector.output(asset.assetId, AssetType::Mesh, Serializer::toBytes(*result));
		}
	}
}

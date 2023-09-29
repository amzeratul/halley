#include "mesh_importer.h"
#include "../../mesh/wavefront_reader.h"
#include "../../mesh/gltf_reader.h"

using namespace Halley;

void MeshImporter::import(const ImportingAsset& asset, IAssetCollector& collector)
{
	for (auto& f: asset.inputFiles) {
		if (f.name.getExtension() == ".obj") {
			const auto reader = std::make_unique<WavefrontReader>();
			std::unique_ptr<Mesh> result = reader->parse(f.data);
			collector.output(asset.assetId, AssetType::Mesh, Serializer::toBytes(*result));
		}
		if (f.name.getExtension() == ".glb") {
			const auto reader = std::make_unique<GLTFReader>();

			auto name = asset.inputFiles.at(0).name;
			name = name.replaceExtension("");

			std::unique_ptr<Mesh> result = reader->parseBinary(name, f.data, f.metadata);
			collector.output(asset.assetId, AssetType::Mesh, Serializer::toBytes(*result));
		}
		if (f.name.getExtension() == ".gltf") {
			Path basePath = asset.inputFiles.at(0).name.parentPath();
			const auto reader = std::make_unique<GLTFReader>();

			auto name = asset.inputFiles.at(0).name;
			name = name.replaceExtension(".bin");

			auto binData = collector.readAdditionalFile(name);
			name = name.replaceExtension("");

			std::unique_ptr<Mesh> result = reader->parseASCII(name, binData, f.data, f.metadata);
			collector.output(asset.assetId, AssetType::Mesh, Serializer::toBytes(*result));
		}
	}
}

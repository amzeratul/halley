#include "mesh_importer.h"
#include "../../mesh/wavefront_reader.h"

using namespace Halley;

void MeshImporter::import(const ImportingAsset& asset, IAssetCollector& collector)
{
	for (auto& f: asset.inputFiles) {
		collector.output(asset.assetId, AssetType::Mesh, Serializer::toBytes(*parse(f.name, f.data, collector)));
	}
}

std::unique_ptr<Mesh> MeshImporter::parse(const Path& filename, const Bytes& bytes, IAddionalFileReader& reader)
{
	if (filename.getExtension() == ".obj") {
		return WavefrontReader::parse(bytes, reader);
	}

	return {};
}

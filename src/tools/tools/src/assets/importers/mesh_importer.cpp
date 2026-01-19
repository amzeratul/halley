#include "mesh_importer.h"
#include "../../mesh/wavefront_reader.h"

using namespace Halley;

void MeshImporter::import(const ImportingAsset& asset, IAssetCollector& collector)
{
	for (auto& f: asset.inputFiles) {
		if (auto mesh = parse(f.name, f.data, collector)) {
			collector.output(asset.assetId, AssetType::Mesh, Serializer::toBytes(*mesh));
		}
	}
}

std::unique_ptr<Mesh> MeshImporter::parse(const Path& filename, const Bytes& bytes, IAddionalFileReader& reader)
{
	if (filename.getExtension() == ".obj") {
		return WavefrontReader::parse(filename, bytes, reader);
	}

	return {};
}

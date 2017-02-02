#include "shader_importer.h"
#include "halley/tools/assets/import_assets_database.h"
#include "halley/file/byte_serializer.h"
#include "halley/resources/metadata.h"
#include "halley/tools/file/filesystem.h"
#include "halley/core/graphics/shader.h"

using namespace Halley;

void ShaderImporter::import(const ImportingAsset& asset, IAssetCollector& collector)
{
	ShaderFile shader;
	for (auto& input: asset.inputFiles) {
		auto shaderType = fromString<ShaderType>(input.name.getExtension().mid(1));
		shader.shaders[int(shaderType)] = input.data;
	}

	collector.output(asset.assetId, AssetType::Shader, Serializer::toBytes(shader));
}

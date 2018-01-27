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
		String strData = String(reinterpret_cast<const char*>(input.data.data()), input.data.size());
		if (asset.metadata->getString("language", "") == "glsl") {
			strData = "#version 330\n" + strData;
		}
		Bytes data(strData.size());
		memcpy(data.data(), strData.c_str(), data.size());
		shader.shaders[shaderType] = data;
	}

	collector.output(asset.assetId, AssetType::Shader, Serializer::toBytes(shader), *asset.metadata);
}

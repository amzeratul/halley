#include "shader_importer.h"
#include "halley/tools/assets/import_assets_database.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/resources/metadata.h"
#include "halley/tools/file/filesystem.h"
#include "halley/core/graphics/shader.h"
#include "halley/support/logger.h"

#ifdef _MSC_VER
#include <D3Dcompiler.h>
#pragma comment(lib, "D3DCompiler.lib")
#endif

using namespace Halley;

void ShaderImporter::import(const ImportingAsset& asset, IAssetCollector& collector)
{
	ShaderFile shader;
	for (auto& input: asset.inputFiles) {
		const auto shaderType = fromString<ShaderType>(input.name.getExtension().mid(1));
		const String language = input.metadata.getString("language", "");

		if (language == "glsl") {
			String strData = String(reinterpret_cast<const char*>(input.data.data()), input.data.size());
			strData = "#version 330\n" + strData;
			Bytes data(strData.size());
			memcpy(data.data(), strData.c_str(), data.size());
			shader.shaders[shaderType] = data;
		} else if (language == "hlsl") {
			shader.shaders[shaderType] = compileHLSL(input.name.toString(), shaderType, input.data);
		}
	}

	collector.output(asset.assetId, AssetType::Shader, Serializer::toBytes(shader), asset.inputFiles.at(0).metadata);
}

Bytes ShaderImporter::compileHLSL(const String& name, ShaderType type, const Bytes& bytes) const
{
#ifdef _MSC_VER

	String target;

	switch (type) {
	case ShaderType::Vertex:
		target = "vs_4_0";
		break;

	case ShaderType::Pixel:
		target = "ps_4_0";
		break;

	case ShaderType::Geometry:
		target = "gs_4_0";
		break;

	default:
		throw Exception("Unsupported shader type: " + toString(type), HalleyExceptions::Tools);
	}

	ID3D10Blob *codeBlob = nullptr;
	ID3D10Blob *errorBlob = nullptr;
	const HRESULT hResult = D3DCompile2(bytes.data(), bytes.size(), name.c_str(), nullptr, nullptr, "main", target.c_str(), 0, 0, 0, nullptr, 0, &codeBlob, &errorBlob);
	if (hResult != S_OK) {
		const auto errorMessage = String(reinterpret_cast<const char*>(errorBlob->GetBufferPointer()), errorBlob->GetBufferSize());
		errorBlob->Release();
		throw Exception("Failed to compile shader " + name + " (" + toString(type) + "):\n" + errorMessage, HalleyExceptions::Tools);
	}

	auto result = Bytes(codeBlob->GetBufferSize());
	memcpy_s(result.data(), result.size(), codeBlob->GetBufferPointer(), codeBlob->GetBufferSize());

	if (errorBlob) {
		errorBlob->Release();
	}
	if (codeBlob) {
		codeBlob->Release();
	}

	return result;

#else

	Logger::logWarning("Compiling HLSL shaders is not supported on non-Windows platforms.");
	return Bytes();

#endif
}

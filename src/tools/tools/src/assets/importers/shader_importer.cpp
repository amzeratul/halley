#include "shader_importer.h"
#include "halley/tools/assets/import_assets_database.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/resources/metadata.h"
#include "halley/tools/file/filesystem.h"
#include "halley/core/graphics/shader.h"
#include "halley/support/logger.h"
#include <ShaderConductor/ShaderConductor.hpp>

#ifdef _MSC_VER
#include <D3Dcompiler.h>
#pragma comment(lib, "D3DCompiler.lib")
#pragma comment(lib, "ShaderConductor.lib")
#endif

using namespace Halley;

void ShaderImporter::import(const ImportingAsset& asset, IAssetCollector& collector)
{
	std::map<String, ShaderFile> shaders;
	const Metadata meta = asset.inputFiles.at(0).metadata;
	if (meta.getString("language", "") != "hlsl") {
		return;
	}

	std::array<String, 3> languages = {{ "hlsl", "glsl", "metal" }};

	for (auto& language: languages) {
		ShaderFile shader;
		for (auto& input: asset.inputFiles) {
			const auto shaderType = fromString<ShaderType>(input.name.getExtension().mid(1));
			shader.shaders[shaderType] = fromHLSL(input.name.toString(), shaderType, input.data, language);
		}

		String id = asset.assetId.split(':').at(0) + ":" + language;
		Metadata curMeta = meta;
		curMeta.set("language", language);
		collector.output(id, AssetType::Shader, Serializer::toBytes(shader), curMeta);
	}
}

Bytes ShaderImporter::fromHLSL(const String& name, ShaderType type, const Bytes& data, const String& dstLanguage) const
{
	if (dstLanguage == "hlsl") {
		return compileHLSL(name, type, data);
	} else {
		using namespace ShaderConductor;

		ShaderStage stage;
		switch (type) {
		case ShaderType::Pixel:
			stage = ShaderStage::PixelShader;
			break;
		case ShaderType::Vertex:
			stage = ShaderStage::VertexShader;
			break;
		case ShaderType::Geometry:
			stage = ShaderStage::GeometryShader;
			break;
		default:
			throw Exception("Invalid stage: " + toString(type), HalleyExceptions::Tools);
		}

		String srcStr(reinterpret_cast<const char*>(data.data()), data.size());
		
		Compiler::SourceDesc source;
		source.fileName = name.c_str();
		source.entryPoint = "main";
		source.stage = stage;
		source.source = srcStr.c_str();
		source.numDefines = 0;
		source.defines = nullptr;
		
		Compiler::TargetDesc target;
		if (dstLanguage == "glsl") {
			target.language = ShadingLanguage::Glsl;
			target.version = "330";
		} else if (dstLanguage == "metal") {
			target.language = ShadingLanguage::Msl_iOS;
			target.version = "221";
		}
		
		Compiler::Options options;
		options.shiftAllCBuffersBindings = 0;
		options.shiftAllSamplersBindings = 0;
		options.shiftAllTexturesBindings = 0;
		options.shiftAllUABuffersBindings = 0;
		
		auto result = Compiler::Compile(source, options, target);

		if (result.hasError) {
			const auto msg = String(reinterpret_cast<const char*>(result.errorWarningMsg->Data()), result.errorWarningMsg->Size());
			DestroyBlob(result.target);
			DestroyBlob(result.errorWarningMsg);
			throw Exception("Error converting shader to " + dstLanguage + ": " + msg, HalleyExceptions::Tools);
		}

		Bytes bytes(result.target->Size());
		memcpy(bytes.data(), result.target->Data(), bytes.size());
		DestroyBlob(result.target);
		DestroyBlob(result.errorWarningMsg);
		
		return bytes;
	}
}

void ShaderImporter::importOld(const ImportingAsset& asset, IAssetCollector& collector)
{
	ShaderFile shader;
	for (auto& input: asset.inputFiles) {
		const auto shaderType = fromString<ShaderType>(input.name.getExtension().mid(1));
		const String language = input.metadata.getString("language", "");

		if (language == "glsl" || language == "metal") {
			// Runtime-compiled shader languages
			String strData = String(reinterpret_cast<const char*>(input.data.data()), input.data.size());
			if (language == "glsl") {
				strData = "#version 330\n" + strData;
			}
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

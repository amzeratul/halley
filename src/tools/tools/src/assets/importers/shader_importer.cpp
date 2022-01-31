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
#endif

using namespace Halley;

thread_local String glslShaderName;
thread_local FlatMap<String, int> glslVariantMap;

void ShaderImporter::import(const ImportingAsset& asset, IAssetCollector& collector)
{
	std::map<String, ShaderFile> shaders;
	const Metadata meta = asset.inputFiles.at(0).metadata;
	const auto language = meta.getString("language", "");

	ShaderFile shader;
	for (auto& input: asset.inputFiles) {
		const auto shaderType = fromString<ShaderType>(input.name.getExtension().mid(1));

		Bytes data = input.data;
		if (language == "hlsl") {
			data = compileHLSL(input.name.toString(), shaderType, data);
		}
		
		shader.shaders[shaderType] = data;
	}

	collector.output(asset.assetId, AssetType::Shader, Serializer::toBytes(shader), meta);
}

Bytes ShaderImporter::convertHLSL(const String& name, ShaderType type, const Bytes& data, const String& dstLanguage)
{
	if (dstLanguage == "hlsl") {
		return data;
	}
	
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
	
	Compiler::Options options = {};
	options.shiftAllCBuffersBindings = 0;
	options.shiftAllSamplersBindings = 0;
	options.shiftAllTexturesBindings = 0;
	options.shiftAllUABuffersBindings = 0;
	options.inheritCombinedSamplerBindings = true;

	Compiler::SourceDesc source = {};
	source.fileName = name.c_str();
	source.entryPoint = "main";
	source.stage = stage;
	source.source = srcStr.c_str();
	source.numDefines = 0;
	source.defines = nullptr;
	
	Compiler::TargetDesc target = {};
	if (dstLanguage == "glsl") {
		target.language = ShadingLanguage::Glsl;
		target.version = "330";
	} else if (dstLanguage == "metal") {
		target.language = ShadingLanguage::Msl_iOS;
		target.version = "221";
	} else if (dstLanguage == "spirv") {
		target.language = ShadingLanguage::SpirV;
		target.version = "460";
	}
	
	auto result = Compiler::Compile(source, options, target);

	if (result.hasError) {
		const auto msg = String(reinterpret_cast<const char*>(result.errorWarningMsg.Data()), result.errorWarningMsg.Size());
		throw Exception("Error converting shader to " + dstLanguage + ": " + msg, HalleyExceptions::Tools);
	}

	Bytes bytes(result.target.Size());
	memcpy(bytes.data(), result.target.Data(), bytes.size());

	if (dstLanguage == "glsl") {
		patchGLSL(name, type, bytes);
	}

	return bytes;
}

Bytes ShaderImporter::compileHLSL(const String& name, ShaderType type, const Bytes& bytes)
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

/*
	1) Halley defines a whole block of varying attributes. They are all written
	in the vertex shader, but not all of them are read from by the pixel
	shader. This causes warnings when linking shader programs on MacOS.

	2) ShaderConductor prefixes variants with "out_var_" in vertex shaders, but
	"in_var_" in pixel shaders. This confuses GLSL with older versions. Here we
	"normalize" them to have the same identifiers on both sides.

	3) Uniform block definitions are rewritten to "type_<Name> { ... } Name",
	which breaks lookup by name on the C++ side. We undo this.

	4) Sampler names get mangled too. We try to revert them to the original names.
*/
void ShaderImporter::patchGLSL(const String& name, ShaderType type, Bytes& data)
{
	if (type != ShaderType::Vertex && type != ShaderType::Pixel) {
		return;
	}

	String code(reinterpret_cast<const char*>(data.data()), data.size());

	if (name != glslShaderName) {
		if (type == ShaderType::Vertex) {
			Logger::logWarning("Patching GLSL only works if pixel shader is compiled before vertex shader!");
			return;
		}

		glslShaderName = name;
		glslVariantMap.clear();

		// Build a map of all vertex shader inputs, counting how often they are used.

		size_t pos = 0;
		while (pos != String::npos) {
			size_t n = code.find("in_var_", pos);
			if (n != String::npos) {
				// Look for the next non-alphanumeric char. This should be the end
				// of the variant name.
				size_t ne = n + 7;
				while (isalnum(code[ne])) ne++;
				// Store in map, counting # of occurrences.
				String ident = code.substr(n, ne - n);
				int count = 0;
				if (glslVariantMap.contains(ident)) {
					count = glslVariantMap[ident] + 1;
				}
				glslVariantMap[ident] = count;
				// forward marker
				n = ne;
			}
			pos = n;
		}

		// For unused inputs, remove the source line which declares them.
		// For used inputs, "remap" the binding locations.

		int slot = 0;

		for (auto& pair : glslVariantMap)
		{
			size_t n = code.find(pair.first);
			Ensures(n != String::npos);
			size_t ne = n + pair.first.size();
			while (n > 0 && code[n - 1] != '\n') n--;
			while (code[ne] != '\n' && code[ne] != '\0') ne++;
			if (pair.second == 0) {
				while (n < ne) {
					code[n] = ' ';
					n++;
				}
			} else {
				size_t loc = code.find("location = ", n);
				if (loc < ne) {
					loc += 11;
					Ensures(slot < 20);
					if (slot > 9) {
						code[loc++] = '1';
					}
					code[loc++] = static_cast<char>('0' + (slot % 10));
					while ((code[loc] != ')') && (loc < ne)) code[loc++] = ' ';
					slot++;
				}
			}
		}

		// Rename the remaining/used inputs.

		code = code.replaceAll("in_var_", "xy_var_");

		// Search for sampler uniforms.

		{
			size_t pos = 0;
			while (pos != String::npos) {
				size_t n = code.find("uniform sampler2D SPIRV_Cross_Combined");
				if (n != String::npos) {
					n += 18;
					size_t nn = n + 20;
					size_t ne = nn + 1;
					while (isalnum(code[ne]) && strncmp(&code[ne], "sampler", 7) != 0) ne++;
					String samplerName = code.substr(nn, ne - nn);
					while (isalnum(code[ne])) ne++;
					String fullSamplerName = code.substr(n, ne - n);
					code = code.replaceAll(fullSamplerName, samplerName);
				}
				pos = n;
			}
		}
	} else {
		if (type == ShaderType::Pixel) {
			Logger::logWarning("Patching GLSL only works if vertex shader is compiled after pixel shader!");
			return;
		}

		// For all unused pixel shader inputs, look for *outputs* in the vertex
		// shader, and remove lines with them, which should be:
		// - declarations
		// - simple assignments (hopefully)
		//
		// For used inputs, remap binding locations just as above.

		int slot = 0;

		for (auto& pair : glslVariantMap)
		{
			String ident = pair.first.replaceOne("in_var_", "out_var_");
			size_t pos = 0;
			while (pos != String::npos) {
				size_t n = code.find(ident.c_str(), pos);
				if (n != String::npos) {
					size_t ne = n + 8;
					while (n > 0 && code[n - 1] != '\n') n--;
					while (code[ne] != '\n' && code[ne] != '\0') ne++;
					if (pair.second == 0) {
						while (n < ne) {
							code[n] = ' ';
							n++;
						}
					} else {
						size_t loc = code.find("location = ", n);
						if (loc < ne) {
							loc += 11;
							Ensures(slot < 20);
							if (slot > 9) {
								code[loc++] = '1';
							}
							code[loc++] = static_cast<char>('0' + (slot % 10));
							while ((code[loc] != ')') && (loc < ne)) code[loc++] = ' ';
							slot++;
						}
						n = ne;
					}
				}
				pos = n;
			}
		}
		
		// Rename the remaining/used outputs.

		code = code.replaceAll("out_var_", "xy_var_");
	}

	// Search for uniform blocks.

	{
		size_t pos = 0;
		while (pos != String::npos) {
			size_t n = code.find("uniform type_");
			if (n != String::npos) {
				n += 8;
				size_t ne = n + 5;
				while (isalnum(code[ne])) ne++;
				String blockTypeName = code.substr(n, ne - n);
				String blockName = blockTypeName.substr(5);
				code = code.replaceAll(" " + blockName + ";", ";");
				code = code.replaceAll(blockName + ".", "");
				code = code.replaceAll(blockTypeName, blockName);
			}
			pos = n;
		}
	}

	// Write patched code.

	data.resize(code.size() - 1);
	memcpy(data.data(), code.c_str(), data.size());
}

#include "shader_importer.h"
#include "halley/tools/assets/import_assets_database.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/resources/metadata.h"
#include "halley/tools/file/filesystem.h"
#include "halley/graphics/material/material_definition.h"
#include "halley/graphics/shader.h"
#include "halley/support/logger.h"

#ifdef __APPLE__
#include <ShaderConductor.hpp>
#else 
#include <ShaderConductor/ShaderConductor.hpp>
#endif

#ifdef _MSC_VER
#include <D3Dcompiler.h>
#pragma comment(lib, "D3DCompiler.lib")

#include <d3d12.h>
#include <dxcapi.h>
#include <wrl.h>
using namespace Microsoft::WRL;
#endif

using namespace Halley;

thread_local String glsl410ShaderName;
thread_local Vector<String> glsl410PSInputVariants;
thread_local FlatMap<String, int> glsl410PSInputVariantsCount;

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
		target.version = "430";
	} else if (dstLanguage == "glsl410") {
		target.language = ShadingLanguage::Glsl;
		target.version = "410";
	} else if (dstLanguage == "glsl300es") {
		target.language = ShadingLanguage::Essl;
		target.version = "300";
	} else if (dstLanguage == "metal") {
		target.language = ShadingLanguage::Msl_macOS;
		target.version = "221";
	} else if (dstLanguage == "spirv") {
		target.language = ShadingLanguage::SpirV;
		target.version = "15";
	}
	
	auto result = Compiler::Compile(source, options, target);

	if (result.hasError) {
		const auto msg = String(reinterpret_cast<const char*>(result.errorWarningMsg.Data()), result.errorWarningMsg.Size());
		throw Exception("Error converting shader to " + dstLanguage + ": " + msg, HalleyExceptions::Tools);
	}

	Bytes bytes(result.target.Size());
	memcpy(bytes.data(), result.target.Data(), bytes.size());

	if (dstLanguage == "glsl410") {
		patchGLSL410(name, type, bytes);
	}
	
	if (dstLanguage == "glsl300es" || dstLanguage == "glsl") {
		patchGLSLCombinedTexSamplers(name, type, bytes);
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

Bytes ShaderImporter::compileDXIL(const String& name, ShaderType type, const Bytes& bytes, const MaterialDefinition& material) {
#ifdef _MSC_VER
    ComPtr<IDxcUtils> utils;
    if (FAILED(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils)))) {
        throw Exception("Failed to create DXC utils instance", HalleyExceptions::Tools);
    }

    // Create blob from shader source.
    ComPtr<IDxcBlobEncoding> sourceBlob;
    if (FAILED(utils->CreateBlobFromPinned(bytes.data(), (uint32_t) bytes.size(),
                                 DXC_CP_UTF8, sourceBlob.GetAddressOf()))) {
        throw Exception("Failed to create DXC source blob", HalleyExceptions::Tools);
    }

    // Create DXC compiler instance, and fill/build its arguments.
    ComPtr<IDxcCompiler3> compiler;
    if (FAILED(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)))) {
        throw Exception("Failed to create DXC compiler instance", HalleyExceptions::Tools);
    }

    DxcBuffer sourceBuffer = {sourceBlob->GetBufferPointer(), sourceBlob->GetBufferSize(), DXC_CP_UTF8};

    const wchar_t *arguments[] = {
            //DXC_ARG_WARNINGS_ARE_ERRORS,
            DXC_ARG_OPTIMIZATION_LEVEL3,
            L"-Qstrip_debug",
            L"-Qstrip_priv",
    };

    StringUTF16 targetProfile;
    if (type == ShaderType::Pixel) {
        targetProfile = L"ps_6_0";
    } else if (type == ShaderType::Vertex) {
        targetProfile = L"vs_6_0";
    } else {
        throw Exception("Wrong shader type for DXC compiler " + toString(type), HalleyExceptions::Tools);
    }

    ComPtr<IDxcCompilerArgs> compilerArguments;
    if (FAILED(utils->BuildArguments(
            name.getUTF16().c_str(),
            nullptr,
            targetProfile.c_str(),
            arguments,
            _countof(arguments),
            nullptr,
            0,
            compilerArguments.GetAddressOf()
    ))) {
        throw Exception("Failed to assemble DXC compiler arguments", HalleyExceptions::Tools);
    }

    ComPtr<IDxcResult> result;
    if (FAILED(compiler->Compile(
            &sourceBuffer,
            compilerArguments->GetArguments(),
            compilerArguments->GetCount(),
            nullptr,
            IID_PPV_ARGS(&result)
    ))) {
        throw Exception("Internal DXC compiler error", HalleyExceptions::Tools);
    }

    HRESULT resultCode = 0;
    if (FAILED(result->GetStatus(&resultCode))) {
        throw Exception("Failed to query DXC compile status", HalleyExceptions::Tools);
    }

    ComPtr<IDxcBlobUtf8> errors;
    if (FAILED(result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr))) {
        throw Exception("Failed to query DXC error output", HalleyExceptions::Tools);
    } else {
        size_t len = errors->GetStringLength();
        if (len > 0) {
            if (FAILED(resultCode)) {
                throw Exception("DXC compile error: " + String(errors->GetStringPointer()), HalleyExceptions::Tools);
            } else {
                Logger::logWarning("DXC compile warning: " + String(errors->GetStringPointer()));
            }
        }
    }

    ComPtr<IDxcBlob> resultBlob;
    if (FAILED(result->GetResult(resultBlob.GetAddressOf()))) {
        throw Exception("Failed to query DXC result blob", HalleyExceptions::Tools);
    }

    // Now construct a root signature from material definition.
    Bytes rootSignatureBlob = buildRootSignature(material);

    // Use a DXC container builder to attach signature to shader.
    ComPtr<IDxcBlob> containerResultBlob;
    {
        ComPtr<IDxcContainerBuilder> builder;
        if (FAILED(DxcCreateInstance(CLSID_DxcContainerBuilder, IID_PPV_ARGS(&builder)))) {
            throw Exception("Failed to create container builder", HalleyExceptions::Tools);
        }

        if (FAILED(builder->Load(resultBlob.Get()))) {
            throw Exception("Failed to load shader blob to container", HalleyExceptions::Tools);
        }

        // TODO: may check for an existing signature here, and either replace or keep it

        ComPtr<IDxcBlobEncoding> newSignatureBlob;
        {
            DxcBuffer signatureBuffer = {};
            signatureBuffer.Ptr = rootSignatureBlob.data();
            signatureBuffer.Size = rootSignatureBlob.size();
            signatureBuffer.Encoding = DXC_CP_ACP;

            void* signatureData;
            uint32_t signatureSize;

            if (FAILED(utils->GetDxilContainerPart(
                    &signatureBuffer,
                    DXC_PART_ROOT_SIGNATURE,
                    &signatureData,
                    &signatureSize
            ))) {
                throw Exception("Failed to extract signature data", HalleyExceptions::Tools);
            }

            if (FAILED(utils->CreateBlob(
                    signatureData,
                    signatureSize,
                    DXC_CP_ACP,
                    newSignatureBlob.GetAddressOf()
            ))) {
                throw Exception("Failed to create new signature blob", HalleyExceptions::Tools);
            }
        }

        // Add to builder and serialize.
        if (FAILED(builder->AddPart(DXC_PART_ROOT_SIGNATURE, newSignatureBlob.Get()))) {
            throw Exception("Failed to add signature blob", HalleyExceptions::Tools);
        }

        ComPtr<IDxcOperationResult> containerResult;
        if (FAILED(builder->SerializeContainer(containerResult.GetAddressOf()))) {
            throw Exception("Failed to serialize container", HalleyExceptions::Tools);
        }

        /*
         * Do not forget to check for errors right here. If shader code and
         * signature do not match, this will report something, hopefully.
         */
        resultCode = 0;
        containerResult->GetStatus(&resultCode);

        if (FAILED(resultCode)) {
            ComPtr<IDxcBlobEncoding> buildErrors;
            containerResult->GetErrorBuffer(buildErrors.GetAddressOf());
            if (buildErrors->GetBufferSize() > 0) {
                throw Exception((const char*) buildErrors->GetBufferPointer(), HalleyExceptions::Tools);
            } else {
                throw Exception("Container serialization reported an error", HalleyExceptions::Tools);
            }
        }

        if (FAILED(containerResult->GetResult(containerResultBlob.GetAddressOf()))) {
            throw Exception("Failed to query container serialize output", HalleyExceptions::Tools);
        }
    }

    // Now containerResultBlob has the shader binary we are looking for.
    auto output = Bytes(containerResultBlob->GetBufferSize());
    memcpy_s(output.data(), output.size(),
             containerResultBlob->GetBufferPointer(), containerResultBlob->GetBufferSize());

    return output;
#else
    Logger::logWarning("Compiling DXIL shaders is not supported on non-Windows platforms.");
	return Bytes();
#endif
}

Bytes ShaderImporter::buildRootSignature(const MaterialDefinition& material)
{
    Vector<D3D12_ROOT_PARAMETER1> parameters;

    // Add material uniform blocks as CBV descriptors.
    uint32_t cbReg = 0;
    for (auto& uniformBlock : material.getUniformBlocks()) {
        D3D12_ROOT_PARAMETER1 cbv = {};
        cbv.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        cbv.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

        cbv.Descriptor.ShaderRegister = cbReg;
        cbv.Descriptor.RegisterSpace = 0;
        cbv.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;

        parameters.emplace_back(cbv);

        // TODO: this ignores any material values?

        cbReg++;
    }

    // Add material textures as one descriptor table, and samplers as a second
    // table (DX12 doesn't allow mixing both in one).
    //
    // One sampler per texture slot. Some parameters are not available at
    // compile time, so we can't use static samplers.
    if (!material.getTextures().empty()) {
        uint32_t numTextures = (uint32_t) material.getTextures().size();

        D3D12_ROOT_PARAMETER1 srv = {};
        srv.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        srv.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_DESCRIPTOR_RANGE1 srvRange = {};
        srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        srvRange.NumDescriptors = numTextures;
        srvRange.BaseShaderRegister = 0;
        srvRange.RegisterSpace = 0;
        srvRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
        srvRange.OffsetInDescriptorsFromTableStart = 0;

        srv.DescriptorTable.NumDescriptorRanges = 1;
        srv.DescriptorTable.pDescriptorRanges = &srvRange;

        parameters.emplace_back(srv);

        D3D12_ROOT_PARAMETER1 sam = {};
        sam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        sam.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_DESCRIPTOR_RANGE1 samRange = {};
        samRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
        samRange.NumDescriptors = numTextures;
        samRange.BaseShaderRegister = 0;
        samRange.RegisterSpace = 0;
        samRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
        samRange.OffsetInDescriptorsFromTableStart = 0;

        sam.DescriptorTable.NumDescriptorRanges = 1;
        sam.DescriptorTable.pDescriptorRanges = &samRange;

        parameters.emplace_back(sam);
    }

    D3D12_VERSIONED_ROOT_SIGNATURE_DESC versionedDesc = {};
    versionedDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;

    D3D12_ROOT_SIGNATURE_DESC1* rootDesc = &versionedDesc.Desc_1_1;
    rootDesc->NumParameters = (uint32_t) parameters.size();
    rootDesc->pParameters = parameters.data();
    rootDesc->Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
                       D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
                       D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
                       D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    ComPtr<ID3D10Blob> signature;
    ComPtr<ID3D10Blob> errors;

    HRESULT result = D3D12SerializeVersionedRootSignature(
            &versionedDesc,
            signature.GetAddressOf(),
            errors.GetAddressOf()
    );

    if (FAILED(result) || (errors && errors->GetBufferSize() > 0)) {
        if (errors && errors->GetBufferSize() > 0) {
            throw Exception((const char*) errors->GetBufferPointer(), HalleyExceptions::Tools);
        } else {
            throw Exception("Root signature serialization reported an error", HalleyExceptions::Tools);
        }
    }

    Bytes output(signature->GetBufferSize());
    memcpy_s(output.data(), output.size(),
             signature->GetBufferPointer(), signature->GetBufferSize());

    return output;
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
void ShaderImporter::patchGLSL410(const String& name, ShaderType type, Bytes& data)
{
	if (type != ShaderType::Vertex && type != ShaderType::Pixel) {
		return;
	}

	String code(reinterpret_cast<const char*>(data.data()), data.size());

	if (name != glsl410ShaderName) {
		if (type == ShaderType::Vertex) {
			Logger::logWarning("Patching GLSL only works if pixel shader is compiled before vertex shader. Shader '" + name + "' might not behave correctly!");
			return;
		}

		glsl410ShaderName = name;
		glsl410PSInputVariants.clear();
		glsl410PSInputVariantsCount.clear();

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
				if (!glsl410PSInputVariantsCount.contains(ident)) {
					glsl410PSInputVariants.emplace_back(ident);
					glsl410PSInputVariantsCount[ident] = -1;
				}
				glsl410PSInputVariantsCount[ident] = glsl410PSInputVariantsCount[ident] + 1;
				// forward marker
				n = ne;
			}
			pos = n;
		}

		// For unused inputs, remove the source line which declares them.
		// For used inputs, "remap" the binding locations.

		for (const auto& pair : glsl410PSInputVariantsCount) {
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
				int slot = 0;
				for (const String& ident : glsl410PSInputVariants) {
					if (ident == pair.first) {
						break;
					}
					if (glsl410PSInputVariantsCount[ident] > 0) {
						slot++;
					}
				}
				size_t loc = code.find("location = ", n);
				if (loc < ne) {
					loc += 11;
					Ensures(slot < 20);
					if (slot > 9) {
						code[loc++] = '1';
					}
					code[loc++] = static_cast<char>('0' + (slot % 10));
					while ((code[loc] != ')') && (loc < ne)) code[loc++] = ' ';
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
			Logger::logWarning("Patching GLSL only works if vertex shader is compiled after pixel shader. Shader '" + name + "' might not behave correctly!");
			return;
		}

		// For all unused pixel shader inputs, look for *outputs* in the vertex
		// shader, and remove lines with them, which should be:
		// - declarations
		// - simple assignments (hopefully)
		//
		// For used inputs, remap binding locations just as above.

		for (const auto& pair : glsl410PSInputVariantsCount) {
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
						int slot = 0;
						for (const String& origIdent : glsl410PSInputVariants) {
							if (origIdent == pair.first) {
								break;
							}
							if (glsl410PSInputVariantsCount[origIdent] > 0) {
								slot++;
							}
						}
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

		glsl410ShaderName = "";
		glsl410PSInputVariants.clear();
		glsl410PSInputVariantsCount.clear();
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

void ShaderImporter::patchGLSLCombinedTexSamplers(const String& name, ShaderType type, Bytes& data)
{
	String source(reinterpret_cast<const char*>(data.data()), data.size());

	HashMap<String, String> remaps;

	auto findMatches = [&](const String& matchPattern) {
		for (size_t pos = 0; (pos = source.find(matchPattern.c_str(), pos)) != std::string::npos; ) {
			const size_t imageStart = pos + matchPattern.length();
			const size_t imageEnd = std::min(source.find("sampler", imageStart), source.find("SPIRV_Cross", imageStart));
			const size_t combinedEnd = source.find(";", imageEnd);

			const String imageName = source.substr(imageStart, imageEnd - imageStart);
			const String combinedName = "SPIRV_Cross_Combined" + source.substr(imageStart, combinedEnd - imageStart);
			remaps[combinedName] = imageName;

			pos = combinedEnd;
		}
	};

	findMatches("uniform highp sampler2D SPIRV_Cross_Combined");
	findMatches("uniform sampler2D SPIRV_Cross_Combined");

	for (const auto& [k, v]: remaps) {
		source = source.replaceAll(k, v);
	}

	data.resize(source.length());
	memcpy(data.data(), source.c_str(), data.size());
}

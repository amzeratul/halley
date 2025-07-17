#include <regex>
using namespace Halley;

static String toFilterName(D3D12_FILTER type)
{
    switch (type) {
        case D3D12_FILTER_MIN_MAG_MIP_POINT:
            return "FILTER_MIN_MAG_MIP_POINT";
        case D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR:
            return "FILTER_MIN_MAG_POINT_MIP_LINEAR";
        case D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT:
            return "FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT";
        case D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR:
            return "FILTER_MIN_POINT_MAG_MIP_LINEAR";
        case D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT:
            return "FILTER_MIN_LINEAR_MAG_MIP_POINT";
        case D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
            return "FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR";
        case D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT:
            return "FILTER_MIN_MAG_LINEAR_MIP_POINT";
        case D3D12_FILTER_MIN_MAG_MIP_LINEAR:
            return "FILTER_MIN_MAG_MIP_LINEAR";
        case D3D12_FILTER_ANISOTROPIC:
            return "FILTER_ANISOTROPIC";
        case D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT:
            return "FILTER_COMPARISON_MIN_MAG_MIP_POINT";
        case D3D12_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR:
            return "FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR";
        case D3D12_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT:
            return "FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT";
        case D3D12_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR:
            return "FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR";
        case D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT:
            return "FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT";
        case D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
            return "FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR";
        case D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT:
            return "FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT";
        case D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR:
            return "FILTER_COMPARISON_MIN_MAG_MIP_LINEAR";
        case D3D12_FILTER_COMPARISON_ANISOTROPIC:
            return "FILTER_COMPARISON_ANISOTROPIC";
        case D3D12_FILTER_MINIMUM_MIN_MAG_MIP_POINT:
            return "FILTER_MINIMUM_MIN_MAG_MIP_POINT";
        case D3D12_FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR:
            return "FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR";
        case D3D12_FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT:
            return "FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT";
        case D3D12_FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR:
            return "FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR";
        case D3D12_FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT:
            return "FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT";
        case D3D12_FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
            return "FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR";
        case D3D12_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT:
            return "FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT";
        case D3D12_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR:
            return "FILTER_MINIMUM_MIN_MAG_MIP_LINEAR";
        case D3D12_FILTER_MINIMUM_ANISOTROPIC:
            return "FILTER_MINIMUM_ANISOTROPIC";
        case D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_POINT:
            return "FILTER_MAXIMUM_MIN_MAG_MIP_POINT";
        case D3D12_FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR:
            return "FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR";
        case D3D12_FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT:
            return "FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT";
        case D3D12_FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR:
            return "FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR";
        case D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT:
            return "FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT";
        case D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
            return "FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR";
        case D3D12_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT:
            return "FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT";
        case D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR:
            return "FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR";
        case D3D12_FILTER_MAXIMUM_ANISOTROPIC:
            return "FILTER_MAXIMUM_ANISOTROPIC";
    }

    throw Exception("Invalid D3D12 type", HalleyExceptions::Tools);
}

static String toAddressModeName(D3D12_TEXTURE_ADDRESS_MODE type)
{
    switch (type) {
        case D3D12_TEXTURE_ADDRESS_MODE_WRAP:
            return "TEXTURE_ADDRESS_WRAP";
        case D3D12_TEXTURE_ADDRESS_MODE_MIRROR:
            return "TEXTURE_ADDRESS_MIRROR";
        case D3D12_TEXTURE_ADDRESS_MODE_CLAMP:
            return "TEXTURE_ADDRESS_CLAMP";
        case D3D12_TEXTURE_ADDRESS_MODE_BORDER:
            return "TEXTURE_ADDRESS_BORDER";
        case D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE:
            return "TEXTURE_ADDRESS_MIRROR_ONCE";
    }

    throw Exception("Invalid D3D12 type", HalleyExceptions::Tools);
}

static String toCompFuncName(D3D12_COMPARISON_FUNC type)
{
    switch (type) {
        case D3D12_COMPARISON_FUNC_NEVER:
            return "COMPARISON_NEVER";
        case D3D12_COMPARISON_FUNC_LESS:
            return "COMPARISON_LESS";
        case D3D12_COMPARISON_FUNC_EQUAL:
            return "COMPARISON_EQUAL";
        case D3D12_COMPARISON_FUNC_LESS_EQUAL:
            return "COMPARISON_LESS_EQUAL";
        case D3D12_COMPARISON_FUNC_GREATER:
            return "COMPARISON_GREATER";
        case D3D12_COMPARISON_FUNC_NOT_EQUAL:
            return "COMPARISON_NOT_EQUAL";
        case D3D12_COMPARISON_FUNC_GREATER_EQUAL:
            return "COMPARISON_GREATER_EQUAL";
        case D3D12_COMPARISON_FUNC_ALWAYS:
            return "COMPARISON_ALWAYS";
    }

    throw Exception("Invalid D3D12 type", HalleyExceptions::Tools);
}

static String toBorderColorName(D3D12_STATIC_BORDER_COLOR type)
{
    switch (type) {
        case D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK:
            return "STATIC_BORDER_COLOR_TRANSPARENT_BLACK";
        case D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK:
            return "STATIC_BORDER_COLOR_OPAQUE_BLACK";
        case D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE:
            return "STATIC_BORDER_COLOR_OPAQUE_WHITE";
    }

    throw Exception("Invalid D3D12 type", HalleyExceptions::Tools);
}

static String toShaderVisibilityName(D3D12_SHADER_VISIBILITY type)
{
    switch (type) {
        case D3D12_SHADER_VISIBILITY_ALL:
            return "SHADER_VISIBILITY_ALL";
        case D3D12_SHADER_VISIBILITY_VERTEX:
            return "SHADER_VISIBILITY_VERTEX";
        case D3D12_SHADER_VISIBILITY_HULL:
            return "SHADER_VISIBILITY_HULL";
        case D3D12_SHADER_VISIBILITY_DOMAIN:
            return "SHADER_VISIBILITY_DOMAIN";
        case D3D12_SHADER_VISIBILITY_GEOMETRY:
            return "SHADER_VISIBILITY_GEOMETRY";
        case D3D12_SHADER_VISIBILITY_PIXEL:
            return "SHADER_VISIBILITY_PIXEL";
#if 0
        case D3D12_SHADER_VISIBILITY_AMPLIFICATION:
            return "SHADER_VISIBILITY_AMPLIFICATION";
        case D3D12_SHADER_VISIBILITY_MESH:
            return "SHADER_VISIBILITY_MESH";
#endif
    }

    throw Exception("Invalid D3D12 type", HalleyExceptions::Tools);
}

static String printRootSignature(const D3D12_ROOT_SIGNATURE_DESC1* rootDesc)
{
    String out = "[RootSignature(\n";

    /*
     * flags
     */
    out += "\"  RootFlags(\" \\\n";

    if (rootDesc->Flags == 0) {
        out = out.substr(0, out.length() - 4);
        out += "0),\" \\\n";
    } else {
        if ((rootDesc->Flags & D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT) != 0) {
            out += "\"    ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |\" \\\n";
        }
        if ((rootDesc->Flags & D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS) != 0) {
            out += "\"    DENY_HULL_SHADER_ROOT_ACCESS |\" \\\n";
        }
        if ((rootDesc->Flags & D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS) != 0) {
            out += "\"    DENY_DOMAIN_SHADER_ROOT_ACCESS |\" \\\n";
        }
        if ((rootDesc->Flags & D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS) != 0) {
            out += "\"    DENY_GEOMETRY_SHADER_ROOT_ACCESS |\" \\\n";
        }

        out = out.substr(0, out.length() - 6);
        out += "),\" \\\n";
    }

    /*
     * parameters
     */
    const D3D12_ROOT_PARAMETER1* param = rootDesc->pParameters;
    for (uint32_t i = 0; i < rootDesc->NumParameters; i++, param++) {
        switch (param->ParameterType) {
            case D3D12_ROOT_PARAMETER_TYPE_CBV: {
                out += "\"  CBV(b" + toString(param->Descriptor.ShaderRegister) + ",\" \\\n";

                if (param->Descriptor.RegisterSpace != 0) {
                    out += "\"    space=" + toString(param->Descriptor.RegisterSpace) + ",\" \\\n";
                }
                if (param->ShaderVisibility != D3D12_SHADER_VISIBILITY_ALL) {
                	out += "\"    visibility=" + toShaderVisibilityName(param->ShaderVisibility) + ",\" \\\n";
                }

                /*
                 * TODO(CoDi): flags
                 */

		        out = out.substr(0, out.length() - 5);
                out += "),\" \\\n";

                break;
            }

            case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE: {
                out += "\"  DescriptorTable(\" \\\n";

                const D3D12_DESCRIPTOR_RANGE1* range = param->DescriptorTable.pDescriptorRanges;
                for (uint32_t j = 0; j < param->DescriptorTable.NumDescriptorRanges; j++, range++) {
                    switch (range->RangeType) {
                        case D3D12_DESCRIPTOR_RANGE_TYPE_SRV: {
                            out += "\"    SRV(t" + toString(range->BaseShaderRegister) + ",\" \\\n";
                            break;
                        }

						case D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER: {
                            out += "\"    Sampler(s" + toString(range->BaseShaderRegister) + ",\" \\\n";
	                        break;
                        }

                        default: {
						    throw Exception("Invalid descriptor range type", HalleyExceptions::Tools);
                        }
                    }

                    if (range->NumDescriptors != D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND) {
                        out += "\"      numDescriptors=" + toString(range->NumDescriptors) + ",\" \\\n";
                    }
                    if (range->RegisterSpace != 0) {
                        out += "\"      space=" + toString(range->RegisterSpace) + ",\" \\\n";
                    }
                    if (range->OffsetInDescriptorsFromTableStart != D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND) {
                        out += "\"      offset=" + toString(range->OffsetInDescriptorsFromTableStart) + ",\" \\\n";
                    }

                    /*
                     * TODO(CoDi): flags
                     */

			        out = out.substr(0, out.length() - 5);
                    out += "),\" \\\n";
                }

                if (param->ShaderVisibility != D3D12_SHADER_VISIBILITY_ALL) {
                	out += "\"    visibility=" + toShaderVisibilityName(param->ShaderVisibility) + ",\" \\\n";
                }

		        out = out.substr(0, out.length() - 5);
                out += "),\" \\\n";

                break;
            }

            default: {
			    throw Exception("Invalid root parameter type", HalleyExceptions::Tools);
            }
        }
    }

    /*
     * static samplers
     */
    const D3D12_STATIC_SAMPLER_DESC* sampler = rootDesc->pStaticSamplers;
    for (uint32_t i = 0; i < rootDesc->NumStaticSamplers; i++, sampler++) {
        out += "\"  StaticSampler(s" + toString(sampler->ShaderRegister) + ",\" \\\n";

        if (sampler->Filter != D3D12_FILTER_ANISOTROPIC) {
            out += "\"    filter=" + toFilterName(sampler->Filter) + ",\" \\\n";
        }
        if (sampler->AddressU != D3D12_TEXTURE_ADDRESS_MODE_WRAP) {
            out += "\"    addressU=" + toAddressModeName(sampler->AddressU) + ",\" \\\n";
        }
        if (sampler->AddressV != D3D12_TEXTURE_ADDRESS_MODE_WRAP) {
            out += "\"    addressV=" + toAddressModeName(sampler->AddressV) + ",\" \\\n";
        }
        if (sampler->AddressW != D3D12_TEXTURE_ADDRESS_MODE_WRAP) {
            out += "\"    addressW=" + toAddressModeName(sampler->AddressW) + ",\" \\\n";
        }
        if (sampler->MipLODBias != 0.0f) {
            out += "\"    mipLODBias=" + toString(sampler->MipLODBias) + ",\" \\\n";
        }
        if (sampler->MaxAnisotropy != D3D12_MAX_MAXANISOTROPY) {
            out += "\"    maxAnisotropy=" + toString(sampler->MaxAnisotropy) + ",\" \\\n";
        }
        if (sampler->ComparisonFunc != D3D12_COMPARISON_FUNC_LESS_EQUAL) {
            out += "\"    comparisonFunc=" + toCompFuncName(sampler->ComparisonFunc) + ",\" \\\n";
        }
        if (sampler->BorderColor != D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE) {
            out += "\"    borderColor=" + toBorderColorName(sampler->BorderColor) + ",\" \\\n";
        }
        if (sampler->MinLOD != 0.0f) {
            out += "\"    minLOD=" + toString(sampler->MinLOD) + ",\" \\\n";
        }
        if (sampler->MaxLOD != D3D12_FLOAT32_MAX) {
            out += "\"    maxLOD=" + toString(sampler->MaxLOD) + ",\" \\\n";
        }
        if (sampler->RegisterSpace != 0) {
            out += "\"    space=" + toString(sampler->RegisterSpace) + ",\" \\\n";
        }
        if (sampler->ShaderVisibility != D3D12_SHADER_VISIBILITY_ALL) {
            out += "\"    visibility=" + toShaderVisibilityName(sampler->ShaderVisibility) + ",\" \\\n";
        }

        out = out.substr(0, out.length() - 5);
        out += "),\" \\\n";
    }

	out = out.substr(0, out.length() - 5);
    out += "\" \\\n";

    out += ")]\n";

    return out;
}

static String buildRootSignature(const MaterialDefinition& material)
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
    D3D12_DESCRIPTOR_RANGE1 srvRange = {};
    D3D12_DESCRIPTOR_RANGE1 samRange = {};
    if (!material.getTextures().empty()) {
        uint32_t numTextures = (uint32_t) material.getTextures().size();

	    D3D12_ROOT_PARAMETER1 srv = {};
        srv.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        srv.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

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

    // Serialize the signature. This isn't actually needed (anymore), but let's
    // keep it here for additional validation.
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

    return printRootSignature(rootDesc);
}

static Bytes compileDXILCommon(DxcCreateInstanceProc createInstanceFn, const String& name, ShaderType type, const Bytes& bytes, const String& language, const MaterialDefinition& material) {
#ifdef _MSC_VER
    Ensures(createInstanceFn != nullptr);

    // First, parse the material to create a root signature, and output this
    // signature as block in HLSL syntax.
    String rootSignature = buildRootSignature(material);

    // Now inject the signature into the HLSL code. For that we need to scan
    // for the main entry function.
    // TODO: this isn't very bullet-proof, nor very efficient.
    String shaderCodeWithSignature;
    {
    	String source = String(bytes).replaceAll("\r\n", "\n");
        Vector<String> lines = source.split("\n");

        // Right now this only looks for " main(...)".
        std::regex matchFunc = std::regex(R"( main\(.*\))");
        bool foundMatch = false;

        for (const auto& line : lines) {
	        if (std::regex_search(line.cppStr(), matchFunc)) {
		        if (foundMatch) {
			        throw Exception("Found more than one shader entry points, that's probably wrong!", HalleyExceptions::Tools);
		        } else {
			        foundMatch = true;
                    shaderCodeWithSignature += rootSignature;
		        }
	        }
            shaderCodeWithSignature += line + "\n";
        }

        if (!foundMatch) {
	        throw Exception("Failed to locate shader entry point", HalleyExceptions::Tools);
        }
    }

    ComPtr<IDxcUtils> utils;
    if (FAILED(createInstanceFn(CLSID_DxcUtils, IID_PPV_ARGS(&utils)))) {
        throw Exception("Failed to create DXC utils instance", HalleyExceptions::Tools);
    }

    // Create blob from *patched* shader source.
    ComPtr<IDxcBlobEncoding> sourceBlob;
    if (FAILED(utils->CreateBlobFromPinned(shaderCodeWithSignature.c_str(),
        (uint32_t) shaderCodeWithSignature.size(), DXC_CP_UTF8, sourceBlob.GetAddressOf()))) {
        throw Exception("Failed to create DXC source blob", HalleyExceptions::Tools);
    }

    // Create DXC compiler instance, and fill/build its arguments.
    ComPtr<IDxcCompiler3> compiler;
    if (FAILED(createInstanceFn(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)))) {
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

    String sourceName = material.getName() + " (" + toString(type) + " shader)";

    ComPtr<IDxcCompilerArgs> compilerArguments;
    if (FAILED(utils->BuildArguments(
            sourceName.getUTF16().c_str(),
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

    ComPtr<IDxcIncludeHandler> includeHandler;
    utils->CreateDefaultIncludeHandler(&includeHandler);

    ComPtr<IDxcResult> result;
    if (FAILED(compiler->Compile(
            &sourceBuffer,
            compilerArguments->GetArguments(),
            compilerArguments->GetCount(),
            includeHandler.Get(),
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
                throw Exception(String(errors->GetStringPointer()), HalleyExceptions::Tools);
            } else {
                Logger::logWarning(String(errors->GetStringPointer()));
            }
        }
    }

    ComPtr<IDxcBlob> resultBlob;
    if (FAILED(result->GetResult(resultBlob.GetAddressOf()))) {
        throw Exception("Failed to query DXC result blob", HalleyExceptions::Tools);
    }

#if 1
    // Use a DXC container builder to attach signature to shader.
    ComPtr<IDxcBlob> containerResultBlob;
    {
        ComPtr<IDxcContainerBuilder> builder;
        if (FAILED(createInstanceFn(CLSID_DxcContainerBuilder, IID_PPV_ARGS(&builder)))) {
            throw Exception("Failed to create container builder", HalleyExceptions::Tools);
        }

        if (FAILED(builder->Load(resultBlob.Get()))) {
            throw Exception("Failed to load shader blob to container", HalleyExceptions::Tools);
        }

        // TODO: may check for an existing signature here, and either replace or keep it

#if 0
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
#endif
#define DXC_E_MISSING_PART 0x80AA0012
        HRESULT hr = builder->RemovePart(DXC_PART_PDB);
        if (FAILED(hr) && hr != DXC_E_MISSING_PART) {
            throw Exception("Error removing PDB blob", HalleyExceptions::Tools);
        }
        hr = builder->RemovePart(DXC_PART_PDB_NAME);
        if (FAILED(hr) && hr != DXC_E_MISSING_PART) {
            throw Exception("Error removing PDB_NAME blob", HalleyExceptions::Tools);
        }
        hr = builder->RemovePart(DXC_PART_PRIVATE_DATA);
        if (FAILED(hr) && hr != DXC_E_MISSING_PART) {
            throw Exception("Error removing PRIVATE_DATA blob", HalleyExceptions::Tools);
        }
        hr = builder->RemovePart(DXC_PART_REFLECTION_DATA);
        if (FAILED(hr) && hr != DXC_E_MISSING_PART) {
            throw Exception("Error removing REFLECTION_DATA blob", HalleyExceptions::Tools);
        }

        ComPtr<IDxcOperationResult> containerResult;
        if (FAILED(builder->SerializeContainer(containerResult.GetAddressOf()))) {
            throw Exception("Failed to serialize container", HalleyExceptions::Tools);
        }

        /*
         * Do not forget to check for errors right here. If we messed up
         * fiddling with the container, this will report something, hopefully.
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
#else

    auto output = Bytes(resultBlob->GetBufferSize());
    memcpy_s(output.data(), output.size(),
             resultBlob->GetBufferPointer(), resultBlob->GetBufferSize());
#endif

    return output;
#else
    Logger::logWarning("Compiling DXIL shaders is not supported on non-Windows platforms.");
	return Bytes();
#endif
}

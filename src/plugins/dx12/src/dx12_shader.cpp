#include "dx12_shader.h"

#include "dx12_video.h"

using namespace Halley;

DX12Shader::DX12Shader(DX12Video& video, const ShaderDefinition& definition)
{
    for (auto& shader : definition.shaders) {
        loadShader(video, shader.first, shader.second, definition.name);
    }
}

DX12Shader::~DX12Shader() = default;

int DX12Shader::getUniformLocation(const String& name, ShaderType stage)
{
    return -1;
}

int DX12Shader::getBlockLocation(const String& name, ShaderType stage)
{
    return -1;
}

void DX12Shader::fillPipelineStateDesc(D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc) const
{
    desc.pRootSignature = rootSignature.Get();
    desc.VS.pShaderBytecode = vertexBlob.data();
    desc.VS.BytecodeLength = vertexBlob.size();
    desc.PS.pShaderBytecode = pixelBlob.data();
    desc.PS.BytecodeLength = pixelBlob.size();
}

void DX12Shader::loadShader(DX12Video& video, ShaderType type, const Bytes& bytes, const String& name)
{
    HRESULT result = S_OK;

    switch (type) {
        case ShaderType::Vertex: {
            vertexBlob = bytes;
            result = video.getDevice().CreateRootSignature(0, bytes.data(), bytes.size(), D3D12_IID_ARGS(rootSignature));
            break;
        }

        case ShaderType::Pixel: {
            pixelBlob = bytes;
            break;
        }

        default:
            break;
    }

    if (result != S_OK) {
        throw Exception("Unable to create shader " + name + " (" + toString(type) + ").", HalleyExceptions::VideoPlugin);
    }

#ifdef DEV_BUILD
    String debugName = "RootSignature:" + name;
    rootSignature->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT) debugName.size(), debugName.c_str());
#endif
}

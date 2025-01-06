#pragma once

#include "halley_dx12.h"
#include "halley/graphics/shader.h"

namespace Halley {

    class DX12Video;

    class DX12Shader final : public Shader
    {
    public:
        explicit DX12Shader(DX12Video& video, const ShaderDefinition& definition);
        ~DX12Shader() override;

        int getUniformLocation(const String& name, ShaderType stage) override;
        int getBlockLocation(const String& name, ShaderType stage) override;

        void fillPipelineStateDesc(D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc) const;

    private:
        Bytes vertexBlob;
        Bytes pixelBlob;
        ComPtr<ID3D12RootSignature> rootSignature;

        void loadShader(DX12Video& video, ShaderType type, const Bytes& bytes, const String& name);
    };

}

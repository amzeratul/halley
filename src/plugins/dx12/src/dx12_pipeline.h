#pragma once

#include "halley_dx12.h"

namespace Halley {

    class DX12Video;
    class Material;

    class DX12Pipeline
    {
    public:
        explicit DX12Pipeline(int numPasses);
        ~DX12Pipeline();

        void bind(DX12Video& video, const Material& material, int passN);

        static uint64_t getMaterialHash(const Material& material);

    private:
        struct Pass
        {
            ComPtr<ID3D12PipelineState> state;
            ComPtr<ID3D12RootSignature> rootSignature;
            uint8_t stencilReference = 0;
        };

        Vector<Pass> passes;
    };

}

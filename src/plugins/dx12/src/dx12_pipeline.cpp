#include "dx12_pipeline.h"

#include "dx12_shader.h"
#include "dx12_video.h"
#include "halley/graphics/material/material.h"
#include "halley/graphics/material/material_definition.h"
#include "halley/utils/hash.h"

using namespace Halley;

static DXGI_FORMAT toFormat(ShaderParameterType type)
{
    switch (type) {
        case ShaderParameterType::Float:
            return DXGI_FORMAT_R32_FLOAT;
        case ShaderParameterType::Float2:
            return DXGI_FORMAT_R32G32_FLOAT;
        case ShaderParameterType::Float3:
            return DXGI_FORMAT_R32G32B32_FLOAT;
        case ShaderParameterType::Float4:
            return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case ShaderParameterType::Int:
            return DXGI_FORMAT_R32_SINT;
        case ShaderParameterType::Int2:
            return DXGI_FORMAT_R32G32_SINT;
        case ShaderParameterType::Int3:
            return DXGI_FORMAT_R32G32B32_SINT;
        case ShaderParameterType::Int4:
            return DXGI_FORMAT_R32G32B32A32_SINT;
        case ShaderParameterType::UInt:
            return DXGI_FORMAT_R32_UINT;
        default:
            break;
    }

    throw Exception("Unknown format", HalleyExceptions::VideoPlugin);
}

static D3D12_COMPARISON_FUNC getComparisonFunc(DepthStencilComparisonFunction f)
{
    switch (f) {
        case DepthStencilComparisonFunction::Always:
            return D3D12_COMPARISON_FUNC_ALWAYS;
        case DepthStencilComparisonFunction::Never:
            return D3D12_COMPARISON_FUNC_NEVER;
        case DepthStencilComparisonFunction::Equal:
            return D3D12_COMPARISON_FUNC_EQUAL;
        case DepthStencilComparisonFunction::NotEqual:
            return D3D12_COMPARISON_FUNC_NOT_EQUAL;
        case DepthStencilComparisonFunction::Less:
            return D3D12_COMPARISON_FUNC_LESS;
        case DepthStencilComparisonFunction::LessEqual:
            return D3D12_COMPARISON_FUNC_LESS_EQUAL;
        case DepthStencilComparisonFunction::Greater:
            return D3D12_COMPARISON_FUNC_GREATER;
        case DepthStencilComparisonFunction::GreaterEqual:
            return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
    }

    return D3D12_COMPARISON_FUNC_NEVER;
}

static D3D12_STENCIL_OP getOperation(StencilWriteOperation op)
{
    switch (op) {
        case StencilWriteOperation::Zero:
            return D3D12_STENCIL_OP_ZERO;
        case StencilWriteOperation::Invert:
            return D3D12_STENCIL_OP_INVERT;
        case StencilWriteOperation::Keep:
            return D3D12_STENCIL_OP_KEEP;
        case StencilWriteOperation::Replace:
            return D3D12_STENCIL_OP_REPLACE;
        case StencilWriteOperation::IncrementClamp:
            return D3D12_STENCIL_OP_INCR_SAT;
        case StencilWriteOperation::IncrementWrap:
            return D3D12_STENCIL_OP_INCR;
        case StencilWriteOperation::DecrementClamp:
            return D3D12_STENCIL_OP_DECR_SAT;
        case StencilWriteOperation::DecrementWrap:
            return D3D12_STENCIL_OP_DECR;
    }
    return D3D12_STENCIL_OP_KEEP;
}

static void fillDepthStencilDesc(D3D12_DEPTH_STENCIL_DESC& desc, const MaterialDepthStencil& definition)
{
    desc.DepthEnable = definition.isDepthTestEnabled() || definition.isDepthWriteEnabled();
    desc.DepthWriteMask = definition.isDepthWriteEnabled() ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
    desc.DepthFunc = getComparisonFunc(definition.isDepthTestEnabled() ? definition.getDepthComparisonFunction() : DepthStencilComparisonFunction::Always);

    desc.StencilEnable = definition.isStencilTestEnabled();
    desc.StencilReadMask = definition.getStencilReadMask();
    desc.StencilWriteMask = definition.getStencilWriteMask();

    desc.FrontFace.StencilFailOp = getOperation(definition.getStencilOpStencilFail());
    desc.FrontFace.StencilDepthFailOp = getOperation(definition.getStencilOpDepthFail());
    desc.FrontFace.StencilPassOp = getOperation(definition.getStencilOpPass());
    desc.FrontFace.StencilFunc = getComparisonFunc(definition.getStencilComparisonFunction());

    desc.BackFace.StencilFailOp = desc.FrontFace.StencilFailOp;
    desc.BackFace.StencilDepthFailOp = desc.FrontFace.StencilDepthFailOp;
    desc.BackFace.StencilPassOp = desc.FrontFace.StencilPassOp;
    desc.BackFace.StencilFunc = desc.FrontFace.StencilFunc;
}

static D3D12_CULL_MODE toCullMode[] = {
        D3D12_CULL_MODE_NONE,
        D3D12_CULL_MODE_FRONT,
        D3D12_CULL_MODE_BACK,
};

static void fillBlendMode(D3D12_BLEND_DESC& desc, const BlendType& blend)
{
    desc.AlphaToCoverageEnable = false;
    desc.IndependentBlendEnable = false;

    auto& target = desc.RenderTarget[0];

    target.BlendEnable = true;
    target.LogicOpEnable = false;
    target.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    switch (blend.mode) {
        case BlendMode::Alpha:
            target.BlendOp = D3D12_BLEND_OP_ADD;
            target.SrcBlend = blend.premultiplied ? D3D12_BLEND_ONE : D3D12_BLEND_SRC_ALPHA;
            target.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
            target.BlendOpAlpha = D3D12_BLEND_OP_ADD;
            target.SrcBlendAlpha = D3D12_BLEND_ONE;
            target.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
            break;

        case BlendMode::Add:
            target.BlendOp = D3D12_BLEND_OP_ADD;
            target.SrcBlend = blend.premultiplied ? D3D12_BLEND_ONE : D3D12_BLEND_SRC_ALPHA;
            target.DestBlend = D3D12_BLEND_ONE;
            target.BlendOpAlpha = D3D12_BLEND_OP_ADD;
            target.SrcBlendAlpha = D3D12_BLEND_ONE;
            target.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
            break;

        case BlendMode::Multiply:
            target.BlendOp = D3D12_BLEND_OP_ADD;
            target.SrcBlend = D3D12_BLEND_DEST_COLOR;
            target.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
            target.BlendOpAlpha = D3D12_BLEND_OP_ADD;
            target.SrcBlendAlpha = D3D12_BLEND_ONE;
            target.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
            break;

        case BlendMode::Invert:
            target.BlendOp = D3D12_BLEND_OP_ADD;
            target.SrcBlend = D3D12_BLEND_INV_DEST_COLOR;
            target.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
            target.BlendOpAlpha = D3D12_BLEND_OP_ADD;
            target.SrcBlendAlpha = D3D12_BLEND_ONE;
            target.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
            break;

        case BlendMode::Max:
            target.BlendOp = D3D12_BLEND_OP_MAX;
            target.SrcBlend = blend.premultiplied ? D3D12_BLEND_ONE : D3D12_BLEND_SRC_ALPHA;
            target.DestBlend = D3D12_BLEND_ONE;
            target.BlendOpAlpha = D3D12_BLEND_OP_MAX;
            target.SrcBlendAlpha = D3D12_BLEND_ONE;
            target.DestBlendAlpha = D3D12_BLEND_ONE;
            break;

        case BlendMode::Min:
            target.BlendOp = D3D12_BLEND_OP_MIN;
            target.SrcBlend = blend.premultiplied ? D3D12_BLEND_ONE : D3D12_BLEND_SRC_ALPHA;
            target.DestBlend = D3D12_BLEND_ONE;
            target.BlendOpAlpha = D3D12_BLEND_OP_MAX;
            target.SrcBlendAlpha = D3D12_BLEND_ONE;
            target.DestBlendAlpha = D3D12_BLEND_ONE;
            break;

        case BlendMode::Opaque:
        default:
            target.BlendEnable = false;
    }
}

DX12Pipeline::DX12Pipeline(int numPasses)
{
    passes.resize(numPasses);
}

DX12Pipeline::~DX12Pipeline() {
    for (auto& pass : passes) {
        pass.state.Reset();
        pass.rootSignature.Reset();
    }
}

void DX12Pipeline::bind(DX12Video& video, const Material& material, int passN)
{
    auto& pass = passes[passN];

    if (pass.state == nullptr) {
        // Create pipeline state objects on the fly, at first use.

        // Parse material attributes to assemble input layout.
        Vector<D3D12_INPUT_ELEMENT_DESC> layout;
        auto& attribs = material.getDefinition().getAttributes();

        for (auto& attr : attribs) {
            D3D12_INPUT_ELEMENT_DESC elem = {};

            elem.SemanticName = attr.semantic.c_str();
            elem.SemanticIndex = attr.semanticIndex;
            elem.Format = toFormat(attr.type);
            elem.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
            elem.AlignedByteOffset = attr.offset;

            Ensures(attr.offset % 4 == 0);

            layout.emplace_back(elem);
        }

        auto& materialPass = material.getDefinition().getPass(passN);
        const auto blend = materialPass.getBlend();
        const auto& depthStencil = material.getDepthStencil(passN);
        auto shader = (const DX12Shader*) &materialPass.getShader();

        // Now setup pipeline state description.
        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};

        shader->fillPipelineStateDesc(desc);
        pass.rootSignature = desc.pRootSignature;

        fillBlendMode(desc.BlendState, blend);
        
        desc.SampleMask = UINT_MAX;

        desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        desc.RasterizerState.CullMode = toCullMode[(int) materialPass.getCulling()];
        desc.RasterizerState.FrontCounterClockwise = true;
        desc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
        desc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
        desc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
        desc.RasterizerState.DepthClipEnable = true;
        desc.RasterizerState.MultisampleEnable = false;
        desc.RasterizerState.AntialiasedLineEnable = false;
        desc.RasterizerState.ForcedSampleCount = 0;
        desc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

        fillDepthStencilDesc(desc.DepthStencilState, depthStencil);
        pass.stencilReference = depthStencil.getStencilReference();

        desc.InputLayout.pInputElementDescs = layout.data();
        desc.InputLayout.NumElements = (uint32_t) layout.size();

        desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

        desc.NumRenderTargets = 1;
        desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

        desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

        desc.SampleDesc.Count = 1;

        if (FAILED(video.getDevice().CreateGraphicsPipelineState(&desc, D3D12_IID_ARGS(pass.state)))) {
            throw Exception("Failed to create pipeline", HalleyExceptions::VideoPlugin);
        }

#ifdef DEV_BUILD
        String name = "Pipeline:" + material.getDefinition().getAssetId() + "/pass" + toString(passN);
        pass.state->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT) name.size(), name.c_str());
#endif
    }

    auto cmdList = video.getCmdList();

    cmdList->SetGraphicsRootSignature(pass.rootSignature.Get());
    cmdList->SetPipelineState(pass.state.Get());

    cmdList->OMSetStencilRef(pass.stencilReference);
}

uint64_t DX12Pipeline::getMaterialHash(const Material& material)
{
    /*
     * Calculates a variant of the material's partial hash.
     *
     * Currently, Material::computeHashes() takes dataBlock *contents* into
     * account. We do not want to do that - it would generate a new hash for
     * any changes in shader uniform data.
     */
    Hash::Hasher hasher;

    hasher.feed(&material.getDefinition());

    for (const auto& dataBlock: material.getDataBlocks()) {
        hasher.feed(dataBlock.getBindPoint());
        hasher.feed(dataBlock.getType());
    }

    hasher.feed(material.getStencilReferenceOverride().has_value());
    hasher.feed(material.getStencilReferenceOverride().value_or(0));

    uint32_t numPasses = material.getPassesEnabled().to_ulong();

    hasher.feed(material.isDepthStencilEnabled());
    hasher.feed(numPasses);

    /*
     * Depth-stencil is part of the pipeline state.
     */
    for (uint32_t pass = 0; pass < numPasses; pass++) {
        const auto& depthStencil = material.getDepthStencil(int(pass));
        hasher.feed(depthStencil.getHash());
    }

    return hasher.digest();
}

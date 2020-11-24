#include "halley/halley_block.hlsl"

cbuffer ModelBlock : register(b1) {
    float4x4 u_modelMatrix;
};

struct VIn {
    float4 position : POSITION;
    float4 normal : NORMAL;
    float4 colour : COLOUR;
    float4 texCoord0 : TEXCOORD0;
};

struct VOut {
    float4 position : SV_POSITION;
    float4 colour : COLOR0;
    float4 texCoord0 : TEXCOORD0;
    float3 normal : NORMAL;
};

VOut main(VIn input) {
    float4x4 normMatrix = u_mvp;
    normMatrix[3][0] = 0;
    normMatrix[3][1] = 0;
    normMatrix[3][2] = 0;
    normMatrix[0][3] = 0;
    normMatrix[1][3] = 0;
    normMatrix[2][3] = 0;
    normMatrix[3][3] = 1;

    float4 norm4 = mul(normMatrix, input.normal);

    VOut result;

    result.position = mul(u_mvp, mul(u_modelMatrix, input.position));
    result.normal = norm4.xyz / norm4.w;
    result.colour = input.colour;
    result.texCoord0 = input.texCoord0;

    return result;
}

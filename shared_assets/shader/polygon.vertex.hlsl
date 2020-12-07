#include "halley/halley_block.hlsl"

struct VIn {
    float4 colour : COLOUR;
    float2 position : POSITION;
    float2 normal : NORMAL;
    float2 width : WIDTH;
};

struct VOut {
    float4 position : SV_POSITION;
    float4 colour : COLOR0;
};

VOut main(VIn input) {
    VOut result;

    result.position = mul(u_mvp, float4(input.position, 0, 1));
    result.colour = input.colour;

    return result;
}

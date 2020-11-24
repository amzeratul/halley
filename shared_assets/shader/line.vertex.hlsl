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
    float  vertPos : POSITION1;
    float  width : POSITION2;
    float2 worldPos : POSITION3;
};

VOut main(VIn input) {
    VOut result;

    float width = input.width.x;
    float myPos = input.width.y;
    float zoom = mul(u_mvp, float4(1, 0, 0, 0)).x * u_viewPortSize.x;

    float vertPos = 0.5 * (width + 1.0 / zoom) * myPos;
    float2 pos = input.position + vertPos * input.normal;
    result.position = mul(u_mvp, float4(pos, 0, 1));
    result.colour = input.colour;
    result.vertPos = vertPos;
    result.width = width;
    result.worldPos = pos;

    return result;
}

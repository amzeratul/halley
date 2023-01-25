#include "halley/sprite_attribute.hlsl"
#include "halley/sprite_basic_vertex.hlsl"

cbuffer MaterialBlock : register(b1) {
    float u_smoothness;
    float u_outline;
    float u_shadowDistance;
    float u_shadowSmoothness;
    float4 u_outlineColour;
    float4 u_shadowColour;
};

VOut main(VIn input) {
    input.position += u_shadowDistance.xx;
    VOut result;
    basicVertex(input, result);
    return result;
}

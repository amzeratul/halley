#include "halley/sprite_attribute.hlsl"

cbuffer HalleyBlock : register(b0) {
    float4x4 u_mvp;
};

float2 getTexCoord(float4 texCoords, float2 vertPos, float texCoordRotation) {
    float2 texPos = lerp(vertPos, float2(1.0 - vertPos.y, vertPos.x), texCoordRotation);
    return float2(lerp(texCoords.xy, texCoords.zw, texPos.xy));
}

void getColours(float4 inColour, out float4 baseColour, out float4 addColour) {
    float4 inputCol = float4(inColour.rgb * inColour.a, inColour.a); // Premultiply alpha
    float4 baseCol = clamp(inputCol, float4(0, 0, 0, 0), float4(1, 1, 1, 1));
    baseColour = baseCol;
    addColour = clamp(inputCol - baseCol, float4(0, 0, 0, 0), float4(1, 1, 1, 0));
}

float4 getVertexPosition(float2 position, float2 pivot, float2 size, float2 vertPos, float angle) {
    float c = cos(angle);
    float s = sin(angle);
    float2x2 m = { c, -s, s, c };
    
    float2 pos = position + mul(m, ((vertPos - pivot) * size));
    return mul(u_mvp, float4(pos, 0.0, 1.0));
}

VOut main(VIn input) {
    VOut result;

    result.texCoord0 = getTexCoord(input.texCoord0, input.vertPos.zw, input.textureRotation);
    result.pixelTexCoord0 = result.texCoord0 * input.size;
    result.custom0 = input.custom0;
    result.vertPos = input.vertPos.xy;
    result.pixelPos = input.size * input.scale * input.vertPos.xy;
    getColours(input.colour, result.colour, result.colourAdd);
    result.position = getVertexPosition(input.position, input.pivot, input.size * input.scale, input.vertPos.xy, input.rotation);

    return result;
}

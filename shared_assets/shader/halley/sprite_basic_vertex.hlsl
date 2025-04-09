#include "halley/halley_block.hlsl"

float2 getTexCoord(float4 texCoords, float2 vertPos, float texCoordRotation) {
    float2 texPos = lerp(vertPos, float2(1.0 - vertPos.y, vertPos.x), texCoordRotation);
    return float2(lerp(texCoords.xy, texCoords.zw, texPos.xy));
}

void getColours(float4 inColour, out float4 baseColour, out float4 addColour, bool premultiply) {
    float4 inputCol = premultiply ? float4(saturate(inColour.rgb) * inColour.a, inColour.a) : saturate(inColour);
    baseColour = inputCol;
    addColour = saturate(inColour - float4(1, 1, 1, 1));
}

float2 getWorldPosition(float2 position, float2 pivot, float2 size, float2 vertPos, float angle) {
    float c = cos(angle);
    float s = sin(angle);
    float2x2 m = { c, -s, s, c };
    
    return position + mul(m, ((vertPos - pivot) * size));
}

void basicVertex(VIn input, out VOut output, bool premultiply) {
    const float2 worldPos = getWorldPosition(input.position, input.pivot, input.size * input.scale, input.vertPos.xy, input.rotation);

    output.texCoord0 = getTexCoord(input.texCoord0, input.vertPos.zw, input.textureRotation);
    output.texCoord1 = getTexCoord(input.texCoord1, input.vertPos.zw, input.textureRotation);
    output.pixelTexCoord0 = output.texCoord0 * input.size;
    output.pixelTexCoord1 = output.texCoord1 * input.size;
    output.custom0 = input.custom0;
    output.custom1 = input.custom1;
    output.custom2 = input.custom2;
    output.custom3 = input.custom3;
    output.vertPos = input.vertPos.xy;
    output.pixelPos = worldPos;
    output.pivot = input.pivot;
    output.scale = input.scale;
    getColours(input.colour, output.colour, output.colourAdd, premultiply);
    output.colourNoPremultiply = input.colour;
    output.position = mul(u_mvp, float4(worldPos, 0.0, 1.0));
    output.size = input.size.xy;
    output.texCoord0Bounds = input.texCoord0;
    output.texCoord1Bounds = input.texCoord1;
}

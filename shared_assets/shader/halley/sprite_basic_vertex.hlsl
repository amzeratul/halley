#include "halley/halley_block.hlsl"
#include "halley/sprite_data_block.hlsl"

float2 getTexCoord(float4 texCoords, float2 vertPos, float texCoordRotation) {
    float2 texPos = lerp(vertPos, float2(1.0 - vertPos.y, vertPos.x), texCoordRotation);
    return float2(lerp(texCoords.xy, texCoords.zw, texPos.xy));
}

void getColours(float4 inColour, out float4 baseColour, out float4 addColour, bool premultiply) {
    float4 inputCol = premultiply ? float4(inColour.rgb * inColour.a, inColour.a) : inColour;
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

void basicVertexOffset(VIn inputVertex, out VOut output, bool premultiply, float2 offset) {
    OIn input = objectData[inputVertex.idx];

    output.texCoord0 = getTexCoord(input.texCoord0, inputVertex.vertPos.zw, input.textureRotation);
    output.texCoord1 = getTexCoord(input.texCoord1, inputVertex.vertPos.zw, input.textureRotation);
    output.pixelTexCoord0 = output.texCoord0 * input.size;
    output.pixelTexCoord1 = output.texCoord1 * input.size;
    output.custom0 = input.custom0;
    output.custom1 = input.custom1;
    output.custom2 = input.custom2;
    output.custom3 = input.custom3;
    output.vertPos = inputVertex.vertPos.xy;
    output.pixelPos = input.size * input.scale * inputVertex.vertPos.xy;
    output.pivot = input.pivot;
    output.scale = input.scale;
    getColours(input.colour, output.colour, output.colourAdd, premultiply);
    output.colourNoPremultiply = input.colour;
    output.position = getVertexPosition(input.position + offset, input.pivot, input.size * input.scale, inputVertex.vertPos.xy, input.rotation);
    output.size = input.size.xy;
    output.texCoord0Bounds = input.texCoord0;
}

void basicVertex(VIn inputVertex, out VOut output, bool premultiply) {
    basicVertexOffset(inputVertex, output, premultiply, float2(0, 0));
}

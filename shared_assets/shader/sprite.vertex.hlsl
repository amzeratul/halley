cbuffer HalleyBlock {
    float4x4 u_mvp;
};

struct VIn {
    float4 vertPos : VERTPOS;
    float2 position : POSITION;
    float2 pivot : PIVOT;
    float2 size : SIZE;
    float2 scale : SCALE;
    float4 colour : COLOUR;
    float4 texCoord0 : TEXCOORD0;
    float rotation : ROTATION;
    float textureRotation : TEXTUREROTATION;
};

struct VOut {
    float4 position : SV_POSITION;
    float2 texCoord0 : TEXCOORD0;
    float2 pixelTexCoord0 : TEXCOORD1;
    float4 colour : COLOR0;
    float4 colourAdd : COLOR1;
    float2 vertPos : POSITION1;
    float2 pixelPos : POSITION2;
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
    float2x2 m = { c, s, -s, c };
    
    float2 pos = position + mul(m, ((vertPos - pivot) * size));
    return mul(u_mvp, float4(pos, 0.0, 1.0));
}

VOut VShader(VIn input) {
    VOut result;

    result.texCoord0 = getTexCoord(input.texCoord0, input.vertPos.zw, input.textureRotation);
    result.pixelTexCoord0 = result.texCoord0 * input.size;
    result.vertPos = input.vertPos.xy;
    result.pixelPos = input.size * input.scale * input.vertPos.xy;
    getColours(input.colour, result.colour, result.colourAdd);
    result.position = getVertexPosition(input.position, input.pivot, input.size * input.scale, input.vertPos.xy, input.rotation);

    return result;
}

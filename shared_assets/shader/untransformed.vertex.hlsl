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

VOut main(VIn input)
{
    VOut output;
    output.position = float4(input.position + input.size * input.vertPos.xy, 0.0, 1.0);
    output.colour = input.colour;
    return output;
}

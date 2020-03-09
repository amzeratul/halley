// Vertex input
struct VIn {
    float4 vertPos : VERTPOS;
    float2 position : POSITION;
    float2 pivot : PIVOT;
    float2 size : SIZE;
    float2 scale : SCALE;
    float4 colour : COLOUR;
    float4 texCoord0 : TEXCOORD0;
    float4 custom0 : CUSTOM0;
    float rotation : ROTATION;
    float textureRotation : TEXTUREROTATION;
};

// Vertex output / Pixel input
struct VOut {
    float4 position : SV_POSITION;
    float2 texCoord0 : TEXCOORD0;
    float2 pixelTexCoord0 : TEXCOORD1;
    float4 custom0: CUSTOM0;
    float4 colour : COLOR0;
    float4 colourAdd : COLOR1;
    float2 vertPos : POSITION1;
    float2 pixelPos : POSITION2;
};

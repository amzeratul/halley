// Vertex input
struct VIn {
    float4 vertPos : VERTPOS;
    int idx : OBJECTIDX;
};

// Vertex output / Pixel input
struct VOut {
    float4 position : SV_POSITION;
    float2 texCoord0 : TEXCOORD0;
    float2 pixelTexCoord0 : PIXELTEXCOORD0;
    float2 texCoord1 : TEXCOORD1;
    float2 pixelTexCoord1 : PIXELTEXCOORD1;
    float4 custom0 : CUSTOM0;
    float4 custom1 : CUSTOM1;
    float4 custom2 : CUSTOM2;
    float4 custom3 : CUSTOM3;
    float4 colour : COLOR0;
    float4 colourAdd : COLOR1;
    float4 colourNoPremultiply : COLOR2;
    float2 vertPos : POSITION1;
    float2 pixelPos : POSITION2;
    float2 pivot : POSITION3;
    float2 size : SIZE0;
    float2 scale : SCALE;
    float4 texCoord0Bounds : TEXCOORD0BOUNDS;
};

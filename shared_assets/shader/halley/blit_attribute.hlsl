// Vertex input
struct BlitIn {
    float4 position : POSITION;
    float4 texCoord0 : TEXCOORD0;
};

// Vertex output / Pixel input
struct BlitOut {
    float4 position : SV_POSITION;
    float2 texCoord0 : TEXCOORD0;
};

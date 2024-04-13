// Object input
struct OIn {
    float2 position;
    float2 pivot;
    float2 size;
    float2 scale;
    float4 colour;
    float4 texCoord0;
    float4 texCoord1;
    float4 custom0;
    float4 custom1;
    float4 custom2;
    float4 custom3;
    float rotation;
    float textureRotation;
    float2 __padding;
};

//StructuredBuffer<OIn> objectData : register(t0);

cbuffer HalleyObjectAttrib : register(b1) {
    OIn objectData[100];
};

cbuffer MaterialBlock : register(b1) {
    float u_smoothness;
    float u_outline;
    float u_shadowSmoothness;
    float2 u_shadowDistance;
    float4 u_outlineColour;
    float4 u_shadowColour;
};

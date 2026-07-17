#include "halley/sprite_attribute.hlsl"

Texture2D tex0 : register(t0);
SamplerState sampler0 : register(s0);

cbuffer Params : register(b1)
{
    float u_intensity;
	int u_colourBlindType;
};



float sRGBToLinear(float v) {
    return clamp(v < 0.04045 ? v / 12.92 : pow((v + 0.055) / 1.055, 2.4), 0.0, 1.0);
}

float linearTosRGB(float v) {
    return clamp(v < 0.0031308 ? v * 12.92 : pow(v, 1 / 2.4) * 1.055 - 0.055, 0.0, 1.0);
}

float3 sRGBToLinear(float3 rgb) {
    return float3(sRGBToLinear(rgb.r), sRGBToLinear(rgb.g), sRGBToLinear(rgb.b));
}

float3 linearTosRGB(float3 rgb) {
    return float3(linearTosRGB(rgb.r), linearTosRGB(rgb.g), linearTosRGB(rgb.b));
}


// Viénot 1999
float3 simulateProtan(float3 rgb) {
    float3x3 vienotProtan = {
        { 0.11238, 0.88762, 0.00000 },
        { 0.11238, 0.88762, -0.00000 },
        { 0.00401, -0.00401, 1.00000 }
    };

    return linearTosRGB(mul(vienotProtan, sRGBToLinear(rgb)));
}

// Viénot 1999
float3 simulateDeutan(float3 rgb) {
    float3x3 vienotDeutan = {
        { 0.29275, 0.70725, 0.00000 },
        { 0.29275, 0.70725, -0.00000 },
        { -0.02234, 0.02234, 1.00000 }
    };

    return linearTosRGB(mul(vienotDeutan, sRGBToLinear(rgb)));
}

// Viénot 1999
float3 simulateTritanBad(float3 rgb) {
    float3x3 vienotTritan = {
        { 1.00000, 0.14461, -0.14461 },
        { 0.00000, 0.85924, 0.14076 },
        { -0.00000, 0.85924, 0.14076 }
    };

    return linearTosRGB(mul(vienotTritan, sRGBToLinear(rgb)));
}

// Brette 1997
float3 simulateTritan(float3 rgb) {
    float3x3 params1 = {
        { 1.01277, 0.13548, -0.14826 },
        { -0.01243, 0.86812, 0.14431 },
        { 0.07589, 0.80500, 0.11911 },
    };
    float3x3 params2 = {
        { 0.93678, 0.18979, -0.12657 },
        { 0.06154, 0.81526, 0.12320 },
        { -0.37562, 1.12767, 0.24796 },
    };
    float3 separationPlane = float3(0.03901, -0.02788, -0.01113);

    float3 lin = sRGBToLinear(rgb);
    return linearTosRGB(mul(dot(lin, separationPlane) >= 0 ? params1 : params2, lin));
}

float4 main(VOut input) : SV_TARGET {
	float4 col = tex0.Sample(sampler0, input.texCoord0.xy);
	return float4(simulateTritan(col.rgb), col.a);
}

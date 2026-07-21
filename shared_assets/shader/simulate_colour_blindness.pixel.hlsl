#include "halley/sprite_attribute.hlsl"

Texture2D tex0 : register(t0);
SamplerState sampler0 : register(s0);

cbuffer Params : register(b1)
{
    float u_intensity;
	int u_colourBlindType;
};


// Base on https://github.com/DaltonLens/libDaltonLens/blob/master/libDaltonLens.c


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
float3 simulateProtanVienot99(float3 rgb) {
    float3x3 vienotProtan = {
        { 0.11238, 0.88762, 0.00000 },
        { 0.11238, 0.88762, -0.00000 },
        { 0.00401, -0.00401, 1.00000 }
    };

    float3 lin = sRGBToLinear(rgb);
    float3 result = mul(vienotProtan, lin);
    return linearTosRGB(lerp(lin, result, u_intensity));
}

// Viénot 1999
float3 simulateDeutanVienot99(float3 rgb) {
    float3x3 vienotDeutan = {
        { 0.29275, 0.70725, 0.00000 },
        { 0.29275, 0.70725, -0.00000 },
        { -0.02234, 0.02234, 1.00000 }
    };

    float3 lin = sRGBToLinear(rgb);
    float3 result = mul(vienotDeutan, lin);
    return linearTosRGB(lerp(lin, result, u_intensity));
}

// Viénot 1999
float3 simulateTritanVienot99(float3 rgb) {
    float3x3 vienotTritan = {
        { 1.00000, 0.14461, -0.14461 },
        { 0.00000, 0.85924, 0.14076 },
        { -0.00000, 0.85924, 0.14076 }
    };

    float3 lin = sRGBToLinear(rgb);
    float3 result = mul(vienotTritan, lin);
    return linearTosRGB(lerp(lin, result, u_intensity));
}

// Brette 1997
float3 simulateTritanBrette97(float3 rgb) {
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
    float3 result = mul(dot(lin, separationPlane) >= 0 ? params1 : params2, lin);
    return linearTosRGB(lerp(lin, result, u_intensity));
}

float3 simulateAchroma(float3 rgb) {
    float value = dot(rgb, float3(0.114, 0.587, 0.299));
    float3 result =  float3(value, value, value);
    return lerp(rgb, result, u_intensity);
}

float4 main(VOut input) : SV_TARGET {
	float4 col = tex0.Sample(sampler0, input.texCoord0.xy);
    if (u_colourBlindType == 1) {
    	return float4(simulateProtanVienot99(col.rgb), col.a);
    } else if (u_colourBlindType == 2) {
    	return float4(simulateDeutanVienot99(col.rgb), col.a);
    } else if (u_colourBlindType == 3) {
    	return float4(simulateTritanBrette97(col.rgb), col.a);
    } else if (u_colourBlindType == 4) {
    	return float4(simulateAchroma(col.rgb), col.a);
    } else {
        return col;
    }
}

#include "halley/sprite_attribute.hlsl"

Texture2D image : register(t0);
SamplerState sampler0 : register(s0);

cbuffer TexParams : register(b1) {
    float2 u_texSize0;
};

float4 smoothPixel(Texture2D tex, float2 uv, float sharpness)
{
    float2 size = u_texSize0;
    float2 pxCoord = size * uv;
    float pxSize = fwidth(pxCoord);
    float2 sampleCoord = (floor(pxCoord) + smoothstep(0, 1, frac(pxCoord) / pxSize) - 0.5) / size;
    return tex.Sample(sampler0, lerp(uv, sampleCoord, smoothstep(1, 2, 1.0 / pxSize)));
}

float4 main(VOut input) : SV_TARGET {
    float4 col = smoothPixel(image, input.texCoord0.xy, 1.0);
    return col * input.colour + input.colourAdd * col.a;
}

#include "halley/sprite_attribute.hlsl"

Texture2D tex0 : register(t0);
SamplerState sampler0 : register(s0);

float4 main(VOut input) : SV_TARGET {
    const float frameWidth = input.custom0.x;
    const float frameHeight = input.custom0.y;
    const float texWidth = input.custom0.z;
    const float texHeight = input.custom0.w;
    const float yHeight = floor(texHeight * 2.0 / 3.0);
    const float halfWidth = floor(texWidth * 0.5);
    const float uvPlaneY = yHeight / texHeight;

    float2 yPlaneStart = float2(0.0, 0.0);
    float2 uPlaneStart = float2(0.0, uvPlaneY);
    float2 vPlaneStart = float2(1.0 / texWidth, uvPlaneY);

    float2 texCoord = float2(input.texCoord0.x * frameWidth / texWidth, input.texCoord0.y * frameHeight / texHeight);
    float2 uvTexCoord = float2(floor(texCoord.x * halfWidth) / halfWidth + (0.5 / texWidth), texCoord.y * 0.5);

	float y = tex0.Sample(sampler0, texCoord + yPlaneStart).r;
    float u = tex0.Sample(sampler0, uvTexCoord + uPlaneStart).r;
    float v = tex0.Sample(sampler0, uvTexCoord + vPlaneStart).r;
    
    float c = 1.164383 * (y - 0.0625);
    float d = u - 0.5;
    float e = v - 0.5;

    float r = c + 1.596027 * e;
    float g = c - 0.391762 * d - 0.812968 * e;
    float b = c + 2.017232 * d;

    return float4(saturate(r), saturate(g), saturate(b), 1.0) * input.colour + input.colourAdd;
}

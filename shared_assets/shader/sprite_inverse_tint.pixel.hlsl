#include "halley/sprite_attribute.hlsl"

Texture2D tex0 : register(t0);
SamplerState sampler0 : register(s0);

float4 dePremultiply(float4 col) {
	return float4(clamp(col.rgb / max(col.a, 0.001), float3(0, 0, 0), float3(1, 1, 1)), col.a);
}

float4 premultiply(float4 col) {
	return float4(col.rgb * col.a, col.a);
}

float4 main(VOut input) : SV_TARGET {
	float4 col = dePremultiply(tex0.Sample(sampler0, input.texCoord0.xy));
	float3 invTint = lerp(float3(1, 1, 1), input.colour.rgb, col.rgb);
	return premultiply(float4(invTint, col.a * input.colour.a));
}

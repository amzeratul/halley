#include "halley/sprite_attribute.hlsl"

Texture2D tex0 : register(t0);
SamplerState sampler0 : register(s0);

float4 main(VOut input) : SV_TARGET {
	float4 origCol = tex0.Sample(sampler0, input.texCoord0.xy);
	float3 col = lerp(input.custom0.rgb, input.colour.rgb, clamp(pow(origCol.rgb, input.custom0.a), float3(0, 0, 0), float3(1, 1, 1)));
	float alpha = origCol.a * input.colour.a;
	return float4(col.rgb * alpha, alpha);
}

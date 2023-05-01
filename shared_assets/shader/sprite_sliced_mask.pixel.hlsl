#include "halley/sprite_attribute.hlsl"

Texture2D tex0 : register(t0);
Texture2D tex1 : register(t1);
SamplerState sampler0 : register(s0);
SamplerState sampler1 : register(s1);

float4 main(VOut input) : SV_TARGET {
	float4 maskCol = tex0.Sample(sampler0, input.texCoord0.xy);
	float4 col = tex1.Sample(sampler1, input.vertPos.xy);
	return col * input.colour * maskCol.aaaa + input.colourAdd * col.a * maskCol.aaaa;
}

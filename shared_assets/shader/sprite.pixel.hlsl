#include "halley/sprite_attribute.hlsl"

Texture2D tex0 : register(t0);
SamplerState sampler0 : register(s0);

float4 main(VOut input) : SV_TARGET {
	float4 col = tex0.Sample(sampler0, input.texCoord0.xy);
	return col * input.colour + input.colourAdd * col.a;
}

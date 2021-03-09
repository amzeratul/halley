#include "halley/sprite_attribute.hlsl"

Texture2D<uint2> tex0 : register(t0);
SamplerState sampler0 : register(s0);

float4 main(VOut input) : SV_TARGET {
	uint2 col = tex0.Load(int3(input.pixelTexCoord0.xy, 0));
	return float4(col.ggg / 4.0, 1.0);
}

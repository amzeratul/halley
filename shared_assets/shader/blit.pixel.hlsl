#include "halley/blit_attribute.hlsl"

Texture2D tex0 : register(t0);
SamplerState sampler0 : register(s0);

float4 main(BlitOut input) : SV_TARGET {
	return tex0.Sample(sampler0, input.texCoord0);
}

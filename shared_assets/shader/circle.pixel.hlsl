#include "halley/sprite_attribute.hlsl"

Texture2D image : register(t0);
SamplerState sampler0 : register(s0);

float4 main(VOut input) : SV_TARGET {
	float pxLen = ddx(input.texCoord0.x) * 2;

	float alpha = smoothstep(1, 1 - pxLen, length(2 * (input.texCoord0.xy - float2(0.5, 0.5))));
	return (input.colour + input.colourAdd) * alpha;
}

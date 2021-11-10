#include "halley/sprite_attribute.hlsl"

Texture2D tex0 : register(t0);
SamplerState sampler0 : register(s0);

float4 main(VOut input) : SV_TARGET {
	const float threshold = 0.01;
	const int thickness = 2;

	float2 texCoordGrad = float2(ddx(input.texCoord0.x), ddy(input.texCoord0.y));

	float4 colCentre = tex0.Sample(sampler0, input.texCoord0.xy);
	if (colCentre.a >= threshold) {
		discard;
	}

	for (int x = -thickness; x <= thickness; ++x) {
		for (int y = -thickness; y <= thickness; ++y) {
			float4 col = tex0.Sample(sampler0, input.texCoord0.xy + float2(x, y) * texCoordGrad);
			if (col.a >= threshold) {
				return float4(1, 1, 0, 1);
			}
		}
	}

	return float4(0, 0, 0, 0);
}

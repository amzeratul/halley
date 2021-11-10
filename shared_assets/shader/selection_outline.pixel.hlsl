#include "halley/sprite_attribute.hlsl"

Texture2D tex0 : register(t0);
SamplerState sampler0 : register(s0);

float sampleAlpha(float2 coord0, float2 coord1) {
	if (coord1.x < 0 || coord1.y < 0 || coord1.x >= 1 || coord1.y >= 1) {
		return 0;
	}
	return tex0.Sample(sampler0, coord0).a;
}

float4 main(VOut input) : SV_TARGET {
	const float threshold = 0.01;
	const int thickness = 2;

	float2 texCoord0Grad = float2(ddx(input.texCoord0.x), ddy(input.texCoord0.y));
	float2 texCoord1Grad = float2(ddx(input.texCoord1.x), ddy(input.texCoord1.y));

	float colCentre = sampleAlpha(input.texCoord0.xy, input.texCoord1.xy);
	if (colCentre >= threshold) {
		discard;
	}

	for (int x = -thickness; x <= thickness; ++x) {
		for (int y = -thickness; y <= thickness; ++y) {
			float col = sampleAlpha(input.texCoord0.xy + float2(x, y) * texCoord0Grad, input.texCoord1.xy + float2(x, y) * texCoord1Grad);
			if (col >= threshold) {
				return float4(1, 1, 0, 1);
			}
		}
	}

	return float4(0, 0, 0, 0);
}

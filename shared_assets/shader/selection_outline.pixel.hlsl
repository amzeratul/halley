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
	const float4 outlineCol = input.colour;

	//float2 texCoord0Grad = float2(ddx(input.texCoord0.x), ddy(input.texCoord0.y));
	float2 texCoord1Grad = float2(ddx(input.texCoord1.x), ddy(input.texCoord1.y));
	float2 texCoord0Grad = input.custom0.xy;

	float colCentre = sampleAlpha(input.texCoord0.xy, input.texCoord1.xy);
	bool hasOutline = false;

	if (colCentre >= threshold) {
		//discard;
		return outlineCol;
	}

	float2 samples[12] = { float2(1, 0), float2(1, 1), float2(0, 1), float2(-1, 1), float2(-1, 0), float2(-1, -1), float2(0, -1), float2(1, -1), float2(2, 0), float2(-2, 0), float2(0, -2), float2(0, 2) };
	for (int i = 0; i < 12; ++i) {
		float col = sampleAlpha(input.texCoord0.xy + samples[i] * texCoord0Grad, input.texCoord1.xy + samples[i] * texCoord1Grad);
		if (col >= threshold) {
			hasOutline = true;
		}
	}

	return hasOutline ? outlineCol : float4(0, 0, 0, 0);
}

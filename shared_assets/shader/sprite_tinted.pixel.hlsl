#include "halley/sprite_attribute.hlsl"
#include "halley/colour.hlsl"

Texture2D image : register(t0);
SamplerState sampler0 : register(s0);

float4 main(VOut input) : SV_TARGET {
	float4 col = image.Sample(sampler0, input.texCoord0.xy);
	float3 hsv = rgbToHsv(col.rgb);
	float3 tintHsv = rgbToHsv(input.colour.rgb);
	return float4(hsvToRgb(float3(tintHsv.rg, hsv.b)) * input.colour.a, col.a * input.colour.a);
}

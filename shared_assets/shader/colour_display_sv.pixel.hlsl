#include "halley/sprite_attribute.hlsl"
#include "halley/colour.hlsl"

float4 main(VOut input) : SV_TARGET {
	float2 coord = input.texCoord0.xy;
	float h = input.custom0.r;
	float s = coord.x;
	float v = 1 - coord.y;
	return float4(hsvToRgb(h, s, v), 1);
}

#include "halley/sprite_attribute.hlsl"
#include "halley/colour.hlsl"

float4 main(VOut input) : SV_TARGET {
	float2 coord = input.texCoord0.xy;
	float h = 1 - coord.y;
	float s = 1;
	float v = 1;
	return float4(hsvToRgb(h, s, v), 1);
}

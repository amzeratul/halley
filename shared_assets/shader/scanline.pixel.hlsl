#include "halley/sprite_attribute.hlsl"

cbuffer MaterialBlock : register(b1) {
	float4 u_col0;
	float4 u_col1;
	float u_distance;
};

float4 main(VOut input) : SV_TARGET {
	float4 col = lerp(u_col0, u_col1, frac(input.texCoord0.y / u_distance));
	return col * input.colour;
}

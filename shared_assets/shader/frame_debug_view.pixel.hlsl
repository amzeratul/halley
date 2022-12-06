#include "halley/sprite_attribute.hlsl"

float4 main(VOut input) : SV_TARGET {
	float2 vertPos = input.vertPos;
	float2 d = float2(abs(ddx(vertPos.x)), abs(ddy(vertPos.y)));

	float dist = max(max(d.x - vertPos.x, d.x + vertPos.x - 1), max(d.y - vertPos.y, d.y + vertPos.y - 1));
	if (dist >= 0) {
		return float4(1, 0, 0, 1);
	}

	return float4(1, 0, 0, 0.25);
}

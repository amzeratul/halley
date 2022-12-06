#include "halley/sprite_attribute.hlsl"

float4 main(VOut input) : SV_TARGET {
	float2 vertPos = input.vertPos;
	float2 d = float2(abs(ddx(vertPos.x)), abs(ddy(vertPos.y)));
	float x0 = vertPos.x - d.x;
	float x1 = vertPos.x + d.x;
	float y0 = vertPos.y - d.y;
	float y1 = vertPos.y + d.y;
	if (x0 < 0 || x1 >= 1 || y0 < 0 || y1 >= 1) {
		return float4(1, 0, 0, 1);
	}

	return float4(1, 0, 0, 0.25);
}

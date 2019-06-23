#include <metal_stdlib>
using namespace metal;

struct VertexOut {
	float4 colour;
	float vertPos;
	float width;
	float2 worldPos;
	float4 position [[ position ]];
};

fragment float4 pixel_func (
	VertexOut vert [[ stage_in ]]
) {
	float edge = vert.width * 0.5;
	float invFeather = 2.0 / dfdx(vert.worldPos.x);
	float v = clamp((edge - abs(vert.vertPos)) * invFeather, -1, 1) * 0.5 + 0.5;
	float a = vert.colour.a * v;

	return float4(vert.colour.rgb * a, a);
}

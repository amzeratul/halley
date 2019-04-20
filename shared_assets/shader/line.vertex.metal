#include <metal_stdlib>
using namespace metal;

struct HalleyBlock {
	float4x4 mvp;
};

struct VertexIn {
	float4 colour;
	float2 position;
	float2 normal;
	float2 width;
};

struct VertexOut {
	float4 colour;
	float vertPos;
	float width;
	float2 worldPos;
	float4 position [[ position ]];
};

vertex VertexOut vertex_func (
	const device VertexIn* vertex_array [[ buffer(0) ]],
	constant HalleyBlock& halley_block [[ buffer(1) ]],
	unsigned int vid [[ vertex_id ]]
) {
	auto v = vertex_array[vid];
	auto out = VertexOut();

	float width = v.width.x;
	float myPos = v.width.y;

	float vertPos = 0.5 * (width + 1) * myPos;
	float2 pos = v.position + vertPos * v.normal;

	out.colour = v.colour;
	out.vertPos = vertPos;
	out.width = width;
	out.worldPos = pos;
	out.position = halley_block.mvp * float4(pos, 0, 1);
	return out;
}

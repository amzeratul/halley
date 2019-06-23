#include <metal_stdlib>
using namespace metal;

struct VertexIn {
	float4 vertPos;
	float2 position;
	float2 pivot;
	float2 size;
	float2 scale;
	float4 colour;
	float4 texCoord0;
	float rotation;
	float textureRotation;
};

struct VertexOut {
	float2 texCoord0;
	float2 pixelTexCoord0;
	float4 colour;
	float4 colourAdd;
	float2 vertPos;
	float2 pixelPos;
	float4 position [[ position ]];
};

vertex VertexOut vertex_func (
	const device VertexIn* vertex_array [[ buffer(0) ]],
	unsigned int vid [[ vertex_id ]]
) {
	auto v = vertex_array[vid];

	auto out = VertexOut();
	out.colour = v.colour;
	out.position = float4(v.position + v.size * v.vertPos.xy, 0.0, 1.0);
	return out;
}

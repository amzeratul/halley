#include <metal_stdlib>
using namespace metal;

// Must match output of vertex shader
struct VertexOut {
	float2 texCoord0;
	float2 pixelTexCoord0;
	float4 colour;
	float4 colourAdd;
	float2 vertPos;
	float2 pixelPos;
	float4 position [[ position ]];
};

fragment float4 pixel_func (
	VertexOut v [[ stage_in ]],
	texture2d<float> tex0 [[ texture(0) ]],
	sampler sampler0 [[ sampler(0) ]]
) {
	float4 col = tex0.sample(sampler0, v.texCoord0);
	return col * v.colour + v.colourAdd * col.a;
}

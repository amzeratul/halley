#include <metal_stdlib>
using namespace metal;

struct MaterialBlock {
	float smoothness;
	float outline;
	float4 outlineColour;
};

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
	sampler sampler0 [[ sampler(0) ]],
	constant MaterialBlock& material [[ buffer(2) ]]
) {
	float dx = abs(dfdx(v.pixelTexCoord0.x) / dfdx(v.position.x));
	float dy = abs(dfdy(v.pixelTexCoord0.y) / dfdy(v.position.y));
	float texGrad = max(dx, dy);

	float a = tex0.sample(sampler0, v.texCoord0).r;
	float s = max(material.smoothness * texGrad, 0.001);
	float inEdge = 0.51;

	float edge0 = clamp(inEdge - s, 0.01, 0.98);
	float edge1 = clamp(inEdge + s, edge0 + 0.01, 0.99);
	float edge = smoothstep(edge0, edge1, a) * v.colour.a;

	return float4(v.colour.rgb, v.colour.a * edge);
}

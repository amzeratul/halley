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
	float dx = abs(dfdx(v.texCoord0.x));
	float dy = abs(dfdy(v.texCoord0.y));
	float texGrad = max(dx, dy);

	float a = tex0.sample(sampler0, v.texCoord0).r;
	float s = max(material.smoothness * texGrad, 0.001);
	float inEdge = 0.5;
	float outEdge = inEdge - clamp(material.outline, 0.0, 0.995) * 0.5;

	float edge0 = clamp(outEdge - s, 0.01, 0.98);
	float edge1 = clamp(outEdge + s, edge0 + 0.01, 0.99);

	float edge = smoothstep(edge0, edge1, a) * v.colour.a;
	float4 col = material.outlineColour;
	float alpha = col.a * edge;
	return float4(col.rgb * alpha, alpha);
}

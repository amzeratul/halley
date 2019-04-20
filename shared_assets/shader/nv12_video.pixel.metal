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
	const float frameWidth = 1920.0;
	const float frameHeight = 1080.0;
	const float width = 1920.0;
	const float halfWidth = floor(width * 0.5);
	const float yHeight = 1088.0;
	const float uvHeight = 544.0;
	const float height = yHeight + uvHeight;
	const float uvPlaneY = yHeight / height;

	float2 yPlaneStart = float2(0.0, 0.0);
	float2 uPlaneStart = float2(0.0, uvPlaneY);
	float2 vPlaneStart = float2(1.0 / width, uvPlaneY);

	float2 texCoord = float2(v.texCoord0.x, v.texCoord0.y * frameHeight / height);
	float2 uvTexCoord = float2(floor(texCoord.x * halfWidth) / halfWidth + (0.5 / width), texCoord.y * 0.5);

	float y = tex0.sample(sampler0, texCoord + yPlaneStart).r;
	float u = tex0.sample(sampler0, uvTexCoord + uPlaneStart).r;
	float v = tex0.sample(sampler0, uvTexCoord + vPlaneStart).r;

	float c = 1.164383 * (y - 0.0625);
	float d = u - 0.5;
	float e = v - 0.5;

	float r = c + 1.596027 * e;
	float g = c - 0.391762 * d - 0.812968 * e;
	float b = c + 2.017232 * d;

	return float4(
		clamp(r, 0, 1), clamp(g, 0, 1), clamp(b, 0, 1), 1.0
	) * v.colour + v.colourAdd;
}

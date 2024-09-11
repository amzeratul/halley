#include "halley/sprite_attribute.hlsl"
#include "halley/text.hlsl"

Texture2D tex0 : register(t0);
SamplerState sampler0 : register(s0);

float median(float3 rgb) {
    return max(min(rgb.r, rgb.g), min(max(rgb.r, rgb.g), rgb.b));
}

float4 main(VOut input) : SV_TARGET {
	float dx = abs(ddx(input.texCoord0.x));
	float dy = abs(ddy(input.texCoord0.y));
	float texGrad = max(dx, dy);

	float4 rgba = tex0.Sample(sampler0, input.texCoord0);
	float s = max(u_smoothness * texGrad, 0.001);
	float inEdge = 0.5;
	float outEdge = inEdge - clamp(u_outline, 0.0, 0.995) * 0.5;

	float fillEdge0 = clamp(inEdge - s, 0.01, 0.98);
	float fillEdge1 = clamp(inEdge + s, fillEdge0 + 0.01, 0.99);
	float fillEdge = smoothstep(fillEdge0, fillEdge1, median(rgba.rgb)) * input.colour.a;

	float edge0 = clamp(outEdge - s, 0.01, 0.98);
	float edge1 = clamp(outEdge + s, edge0 + 0.01, 0.99);
	float edge = smoothstep(edge0, edge1, rgba.a) * input.colour.a;

	float4 col = u_outlineColour;
	float alpha = col.a * (edge - fillEdge);
	return float4(col.rgb * alpha, alpha);
}

#include "halley/sprite_attribute.hlsl"

Texture2D image : register(t0);
SamplerState sampler0 : register(s0);

struct SpriteData {
	float4 col;
};

StructuredBuffer<SpriteData> spriteData : register(t1);

float4 main(VOut input) : SV_TARGET {
	float4 ssboCol = lerp(
		lerp(spriteData[0].col, spriteData[1].col, input.vertPos.x),
		lerp(spriteData[3].col, spriteData[2].col, input.vertPos.x),
		input.vertPos.y);

	float4 col = image.Sample(sampler0, input.texCoord0.xy) * ssboCol;

	float alpha = col.a * input.colour.a;
	return float4(clamp(col.rgb * input.colour.rgb + input.colourAdd.rgb * alpha, float3(0, 0, 0), float3(alpha, alpha, alpha)), alpha);
}

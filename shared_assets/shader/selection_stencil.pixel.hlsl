#include "halley/sprite_attribute.hlsl"

Texture2D tex0 : register(t0);
SamplerState sampler0 : register(s0);

cbuffer OutlineParams : register(b1)
{
	int u_texBPP0;
};


float4 main(VOut input) : SV_TARGET {
	float4 col = tex0.Sample(sampler0, input.texCoord0.xy);
	float alpha = u_texBPP0 == 1 ? (col.r > 0.5 / 255.0 ? 1.0 : 0.0) : col.a;
	if (alpha < 0.01) {
		discard;
	}
	return float4(0, 0, 0, 0);
}

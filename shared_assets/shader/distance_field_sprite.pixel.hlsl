Texture2D tex0 : register(t0);
SamplerState sampler0 : register(s0) {
	Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
};

cbuffer MaterialBlock : register(b1) {
	float u_smoothness;
	float u_outline;
	float4 u_outlineColour;
};

struct VOut {
    float4 position : SV_POSITION;
    float2 texCoord0 : TEXCOORD0;
    float2 pixelTexCoord0 : TEXCOORD1;
    float4 custom0: CUSTOM0;
    float4 colour : COLOR0;
    float4 colourAdd : COLOR1;
    float2 vertPos : POSITION1;
    float2 pixelPos : POSITION2;
};


float4 main(VOut input) : SV_TARGET {
	float dx = abs(ddx(input.texCoord0.x));
	float dy = abs(ddy(input.texCoord0.y));
	float texGrad = max(dx, dy);

	float a = tex0.Sample(sampler0, input.texCoord0).r;
	float s = u_smoothness * texGrad;
	float inEdge = 0.5;
	float outEdge = inEdge - clamp(u_outline, 0.0, 0.995) * 0.5;

	float edge = smoothstep(clamp(outEdge - s, 0.001, 1.0), clamp(outEdge + s, 0.0, 0.999), a);
	float outline = 1.0 - smoothstep(inEdge - s, inEdge + s, a);
	float4 colFill = input.colour;
	float4 colOutline = u_outlineColour;
	float4 col = lerp(colFill, colOutline, outline);
	return float4(col.rgb, col.a * edge);
}

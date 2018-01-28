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
    float4 colour : COLOR0;
    float4 colourAdd : COLOR1;
    float2 vertPos : POSITION1;
    float2 pixelPos : POSITION2;
};


float4 PShader(VOut input) : SV_TARGET {
	float dx = abs(ddx(input.pixelTexCoord0.x) / ddx(input.position.x));
	float dy = abs(ddy(input.pixelTexCoord0.y) / ddy(input.position.y));
	float texGrad = max(dx, dy);

	float a = tex0.Sample(sampler0, input.texCoord0).a;
	float s = max(u_smoothness * texGrad, 0.001);
	float inEdge = 0.5;
	float outEdge = inEdge - clamp(u_outline, 0.0, 0.995) * 0.5;

	float edge0 = clamp(outEdge - s, 0.01, 0.98);
	float edge1 = clamp(outEdge + s, edge0 + 0.01, 0.99);

	float edge = smoothstep(edge0, edge1, a) * input.colour.a;
	float4 col = u_outlineColour;
	return float4(col.rgb, col.a * edge);
}

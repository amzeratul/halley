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


float4 main(VOut input) : SV_TARGET {
	float dx = abs(ddx(input.texCoord0.x));
	float dy = abs(ddy(input.texCoord0.y));
	float texGrad = max(dx, dy);

	float a = tex0.Sample(sampler0, input.texCoord0).r;
	float s = max(u_smoothness * texGrad, 0.001);
	float inEdge = 0.51;

	float edge0 = clamp(inEdge - s, 0.01, 0.98);
	float edge1 = clamp(inEdge + s, edge0 + 0.01, 0.99);
	float edge = smoothstep(edge0, edge1, a) * input.colour.a;
	float4 colFill = input.colour;
	float alpha = colFill.a * edge;
	return float4(colFill.rgb * alpha, alpha);
}

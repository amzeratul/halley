Texture2D tex0 : register(t0);

SamplerState pixelSampler {
	FILTER = D3D11_FILTER_MIN_MAG_MIP_POINT;
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
	float4 col = tex0.Sample(pixelSampler, input.texCoord0.xy);
	return col * input.colour + input.colourAdd * col.a;
}

cbuffer MaterialBlock : register(b1) {
	float4 u_col0;
	float4 u_col1;
	float u_distance;
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
	float4 col = lerp(u_col0, u_col1, frac(input.texCoord0.y / u_distance));
	return col * input.colour;
}

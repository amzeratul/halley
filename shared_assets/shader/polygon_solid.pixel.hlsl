struct VOut {
    float4 position : SV_POSITION;
    float4 colour : COLOR0;
};

float4 main(VOut input) : SV_TARGET {
    float a = input.colour.a;
	return float4(input.colour.rgb * a, a);
}

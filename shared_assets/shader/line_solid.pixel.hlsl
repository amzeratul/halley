struct VOut {
    float4 position : SV_POSITION;
    float4 colour : COLOR0;
    float  vertPos : POSITION1;
    float  width: POSITION2;
    float2 worldPos : POSITION3;
};

float4 main(VOut input) : SV_TARGET {
    float edge = input.width * 0.5;
    float invFeather = 2.0 / ddx(input.worldPos.x);
    float v = clamp((edge - abs(input.vertPos)) * invFeather, -1, 1) * 0.5 + 0.5;

    // Generate gamma-correct alpha
    // Should be pow(v, 1.0 / 2.2), but sqrt() is faster and the difference is very small
    float a = input.colour.a * sqrt(v);

	return float4(input.colour.rgb * a, a);
}

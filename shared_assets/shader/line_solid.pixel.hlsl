struct VOut {
    float4 position : SV_POSITION;
    float4 colour : COLOR0;
    float  vertPos : POSITION1;
    float  width: POSITION2;
    float2 worldPos : POSITION3;
    float4 dashing : POSITION4;
};

float4 main(VOut input) : SV_TARGET {
    // Width size and antialiasing
    float edge = input.width * 0.5;
    float invFeather = 2.0 / ddx(input.worldPos.x);
    float widthAA = clamp((edge - abs(input.vertPos)) * invFeather, -1, 1) * 0.5 + 0.5;
    
    // Length antialiasing (for dashing)
    float curLen = input.dashing.x;
    float dashLen = input.dashing.y;
    float gapLen = input.dashing.z;
    float period = dashLen + gapLen;
    float lengthAA = 1;
    if (gapLen > 0.0001) {
        lengthAA = (curLen % period) < dashLen ? 1.0 : 0.0;
    }
    
    float aa = widthAA * lengthAA;
    float a = input.colour.a * aa;

	return float4(input.colour.rgb * a, a);
}

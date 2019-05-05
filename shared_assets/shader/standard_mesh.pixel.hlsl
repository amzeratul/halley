struct VOut {
    float4 position : SV_POSITION;
    float4 normal : NORMAL;
    float4 colour : COLOR0;
    float4 texCoord0 : TEXCOORD0;
};

float4 main(VOut input) : SV_TARGET {
    return input.colour;
}

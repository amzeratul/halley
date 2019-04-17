cbuffer HalleyBlock : register(b0) {
    float4x4 u_mvp;
};

struct VIn {
    float4 colour : COLOUR;
    float2 position : POSITION;
    float2 normal : NORMAL;
    float2 width : WIDTH;
};

struct VOut {
    float4 position : SV_POSITION;
    float4 colour : COLOR0;
    float  vertPos : POSITION1;
    float  width : POSITION2;
};

VOut main(VIn input) {
    VOut result;

    float width = input.width.x;
    float vertPos = 0.5 * (width + 1) * input.width.y;
    float2 pos = input.position + vertPos * input.normal;
    result.position = mul(u_mvp, float4(pos, 0, 1));
    result.colour = input.colour;
    result.vertPos = vertPos;
    result.width = width;

    return result;
}

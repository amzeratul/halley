cbuffer HalleyBlock : register(b0) {
    float4x4 u_mvp;
};

cbuffer ModelBlock : register(b1) {
    float4x4 u_modelMatrix;
};

struct VIn {
    float4 position : POSITION;
    float4 normal : NORMAL;
    float4 colour : COLOUR;
    float4 texCoord0 : TEXCOORD0;
};

struct VOut {
    float4 position : SV_POSITION;
    float4 normal : NORMAL;
    float4 colour : COLOR0;
    float4 texCoord0 : TEXCOORD0;
};

VOut main(VIn input) {
    VOut result;

    float4 pos = float4((input.position.xyz) * 50, input.position.w);

    result.position = mul(u_mvp, mul(u_modelMatrix, pos));
    result.normal = input.normal;
    result.colour = input.colour;
    result.texCoord0 = input.texCoord0;

    return result;
}

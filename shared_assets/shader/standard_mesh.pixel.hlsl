Texture2D tex0 : register(t0);
SamplerState sampler0 : register(s0);

struct VOut {
    float4 position : SV_POSITION;
    float4 normal : NORMAL;
    float4 colour : COLOR0;
    float4 texCoord0 : TEXCOORD0;
};

float4 main(VOut input) : SV_TARGET {
    float4 col = tex0.Sample(sampler0, input.texCoord0.xy);
    return input.colour * col;
}

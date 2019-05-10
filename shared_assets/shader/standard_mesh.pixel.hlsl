Texture2D tex0 : register(t0);
SamplerState sampler0 : register(s0);

cbuffer HalleyBlock : register(b0) {
    float4x4 u_mvp;
};

struct VOut {
    float4 position : SV_POSITION;
    float4 colour : COLOR0;
    float4 texCoord0 : TEXCOORD0;
    float3 normal : NORMAL;
};

float4 main(VOut input) : SV_TARGET {
    float3 lightPos = float3(2, 2, 2);
    float3 eyePos = float3(0, 0, -1);
    float3 fragPos = input.position.xyz / input.position.w;

    float3 lightDelta = fragPos - lightPos;
    float intensity = clamp(10.0f / dot(lightDelta, lightDelta), 0, 1);

    float3 n = normalize(input.normal);
    float3 i = normalize(lightDelta);
    float3 v = normalize(eyePos - fragPos);
    float3 r = normalize(-2 * dot(i, n) * n + i);
    float cosA = max(0, -dot(r, v));

    float ambient = 0.5;
    float diffuse = max(dot(n, i), 0) * intensity;
    float specular = pow(cosA, 100) * intensity * 0.117;
    float light = min(ambient + diffuse + specular, 1.0);

    float4 col = tex0.Sample(sampler0, input.texCoord0.xy);
    return input.colour * col * light;
}

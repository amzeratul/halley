Texture2D tex0 : register(t0);
SamplerState sampler0 : register(s0);

#include "halley/halley_block.hlsl"

struct VOut {
    float4 position : SV_POSITION;
    float4 colour : COLOR0;
    float4 texCoord0 : TEXCOORD0;
    float3 normal : NORMAL;
};

cbuffer MaterialProperties : register(b2) {
    float4 u_colAmbient;
    float4 u_colDiffuse;
    float4 u_colSpecular;
    float4 u_colEmissive;
    float4 u_colTransmissivity;
    float u_specularExponent;
};

float4 main(VOut input) : SV_TARGET {
    const float gamma = 2.2;
    const float invGamma = 1.0 / gamma;

    float3 lightPos = float3(2, 2, 2);
    float3 eyePos = float3(0, 0, -1);
    float3 fragPos = input.position.xyz / input.position.w;

    float3 lightDelta = fragPos - lightPos;
    float intensity = clamp(10.0f / dot(lightDelta, lightDelta), 0, 1);

    float3 ambientLight = u_colAmbient.rgb;
    float3 emissiveLight = u_colEmissive.rgb;

    float3 n = normalize(input.normal);
    float3 i = normalize(lightDelta);
    float3 diffuseLight = max(dot(n, i), 0) * intensity;

    float3 v = normalize(eyePos - fragPos);
    float3 r = normalize(-2 * dot(i, n) * n + i);
    float cosA = max(0, -dot(r, v));
    float3 specularLight = pow(cosA, u_specularExponent) * intensity * u_colSpecular.rgb;

    float3 light = min(ambientLight + diffuseLight, float3(1.0, 1.0, 1.0));

    float4 col = input.colour * pow(tex0.Sample(sampler0, input.texCoord0.xy), gamma) * u_colDiffuse;
    float4 linearResult = col * float4(light, 1.0) + float4(emissiveLight + specularLight, 0);
    return pow(linearResult, invGamma);
}

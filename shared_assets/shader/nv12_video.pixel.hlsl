Texture2D tex0 : register(t0);
SamplerState sampler0 : register(s0);

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
    const float frameWidth = 1920.0;
    const float frameHeight = 1080.0;
    const float width = 1920.0;
    const float halfWidth = floor(width * 0.5);
    const float yHeight = 1088.0;
    const float uvHeight = 544.0;
    const float height = yHeight + uvHeight;
    const float uvPlaneY = yHeight / height;

    float2 yPlaneStart = float2(0.0, 0.0);
    float2 uPlaneStart = float2(0.0, uvPlaneY);
    float2 vPlaneStart = float2(1.0 / width, uvPlaneY);

    float2 texCoord = float2(input.texCoord0.x, input.texCoord0.y * frameHeight / height);
    float2 uvTexCoord = float2(floor(texCoord.x * halfWidth) / halfWidth + (0.5 / width), texCoord.y * 0.5);

	float y = tex0.Sample(sampler0, texCoord + yPlaneStart).r;
    float u = tex0.Sample(sampler0, uvTexCoord + uPlaneStart).r;
    float v = tex0.Sample(sampler0, uvTexCoord + vPlaneStart).r;
    
    float c = 1.164383 * (y - 0.0625);
    float d = u - 0.5;
    float e = v - 0.5;

    float r = c + 1.596027 * e;
    float g = c - 0.391762 * d - 0.812968 * e;
    float b = c + 2.017232 * d;

    return float4(saturate(r), saturate(g), saturate(b), 1.0) * input.colour + input.colourAdd;
}

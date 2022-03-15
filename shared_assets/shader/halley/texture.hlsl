
float2 getTextureSize(Texture2D tex) {
    float w, h;
    tex.GetDimensions(w, h);
    return float2(w, h);
}

float4 sampleHorizontalAA(Texture2D tex, float2 coord) {
    float2 texSize = getTextureSize(tex);
    float2 pixelCoord = coord * texSize;
    float pixelSize = abs(ddx(pixelCoord));
    
    float2 p0 = pixelCoord - float2(0.5, 0) * pixelSize;
    float2 p1 = p0 + float2(pixelSize, 0);

    float4 t0 = tex.Load(int3(p0, 0));
    float4 t1 = tex.Load(int3(p1, 0));

    float p0t = (1.0 - frac(p0.x)) * t0.a;
    float p1t = (frac(p1.x)) * t1.a;
    if (p0t + p1t < 0.01) {
        return float4(0, 0, 0, 0);
    }
    
    float4 tl = lerp(t1, t0, t0.a);
    float4 tr = lerp(t0, t1, t1.a);

    float amount = p1t / (p0t + p1t);
    float alpha = amount < 0.5 ? t0.a : t1.a;
    float4 result = lerp(tl, tr, amount);
    result.a = alpha;

    return float4(result.rgb * result.a, result.a);
}

float3 hsvToRgb(float h, float s, float v)
{
    float r = 0;
    float g = 0;
    float b = 0;
    if (s == 0) {
        r = clamp(v * 255, 0, 255);
        g = b = r;
    } else {
        h = h % 1.0;
        int hi = int(h * 6);
        float f = h * 6 - hi;
        float p = v * (1 - s);
        float q = v * (1 - f * s);
        float t = v * (1 - (1 - f) * s);

        switch (hi) {
        case 0:	r = v; g = t; b = p; break;
        case 1:	r = q; g = v; b = p; break;
        case 2:	r = p; g = v; b = t; break;
        case 3:	r = p; g = q; b = v; break;
        case 4:	r = t; g = p; b = v; break;
        case 5:	r = v; g = p; b = q; break;
        }
    }

    return float3(r, g, b);
}

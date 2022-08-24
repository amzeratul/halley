float3 hsvToRgb(float3 hsv)
{
    float h = hsv.x;
    float s = hsv.y;
    float v = hsv.z;
    float r = 0;
    float g = 0;
    float b = 0;
    if (s == 0) {
        r = clamp(v, 0, 1);
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

float3 rgbToHsv(float3 rgb, float defaultHue = 0, float defaultSaturation = 0)
{
    float ma = max(rgb.r, max(rgb.g, rgb.b));
    float mi = min(rgb.r, min(rgb.g, rgb.b));
    float c = ma - mi;
    float hp = defaultHue * 6.0;
    if (c > 0.0000001) {
        if (abs(ma - rgb.r) < 0.0000001) {
            hp = (((rgb.g - rgb.b) / c) + 6.0) % 6.0;
        } else if (abs(ma - rgb.g) < 0.0000001) {
            hp = (rgb.b - rgb.r) / c + 2;
        } else {
            hp = (rgb.r - rgb.g) / c + 4;
        }
    }
    float h = hp / 6.0;
    float v = ma;
    float s = v > 0.0000001 ? c / v : defaultSaturation;
    return float3(h, s, v);
}

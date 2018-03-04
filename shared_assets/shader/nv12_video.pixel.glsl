uniform sampler2D tex0;

in vec2 v_texCoord0;
in vec4 v_colour;
in vec4 v_colourAdd;

out vec4 outCol;

void main() {
    const float uvPlaneY = 0.66666667 + (6 / 1080.0);

    vec2 yPlaneStart = vec2(0.0, 0.0);
    vec2 uPlaneStart = vec2(0.0, uvPlaneY);
    vec2 vPlaneStart = vec2(1.0 / 1920.0, uvPlaneY);

    vec2 texCoord = vec2(v_texCoord0.x, v_texCoord0.y * uvPlaneY);
    vec2 uvTexCoord = vec2(floor(texCoord.x * 960.0) / 960.0 + (0.5 / 1920.0), texCoord.y * 0.5);

	float y = texture(tex0, texCoord + yPlaneStart).r;
    float u = texture(tex0, uvTexCoord + uPlaneStart).r;
    float v = texture(tex0, uvTexCoord + vPlaneStart).r;
    
    float c = 1.164383 * (y - 0.0625);
    float d = u - 0.5;
    float e = v - 0.5;

    float r = c + 1.596027 * e;
    float g = c - 0.391762 * d - 0.812968 * e;
    float b = c + 2.017232 * d;

    outCol = vec4(clamp(r, 0, 1), clamp(g, 0, 1), clamp(b, 0, 1), 1.0) * v_colour + v_colourAdd;
}

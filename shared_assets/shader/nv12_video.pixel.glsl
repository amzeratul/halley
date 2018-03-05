uniform sampler2D tex0;

in vec2 v_texCoord0;
in vec4 v_colour;
in vec4 v_colourAdd;

out vec4 outCol;

void main() {
    const float frameWidth = 1920.0;
    const float frameHeight = 1080.0;
    const float width = 1920.0;
    const float halfWidth = floor(width * 0.5);
    const float yHeight = 1088.0;
    const float uvHeight = 544.0;
    const float height = yHeight + uvHeight;
    const float uvPlaneY = yHeight / height;

    vec2 yPlaneStart = vec2(0.0, 0.0);
    vec2 uPlaneStart = vec2(0.0, uvPlaneY);
    vec2 vPlaneStart = vec2(1.0 / width, uvPlaneY);

    vec2 texCoord = vec2(v_texCoord0.x, v_texCoord0.y * frameHeight / height);
    vec2 uvTexCoord = vec2(floor(texCoord.x * halfWidth) / halfWidth + (0.5 / width), texCoord.y * 0.5);

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

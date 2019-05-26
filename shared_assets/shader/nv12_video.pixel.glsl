uniform sampler2D tex0;

in highp vec2 v_texCoord0;
in highp vec4 v_colour;
in highp vec4 v_colourAdd;

out highp vec4 outCol;

void main() {
    const highp float frameWidth = 1920.0;
    const highp float frameHeight = 1080.0;
    const highp float width = 1920.0;
    const highp float halfWidth = floor(width * 0.5);
    const highp float yHeight = 1088.0;
    const highp float uvHeight = 544.0;
    const highp float height = yHeight + uvHeight;
    const highp float uvPlaneY = yHeight / height;

    highp vec2 yPlaneStart = vec2(0.0, 0.0);
    highp vec2 uPlaneStart = vec2(0.0, uvPlaneY);
    highp vec2 vPlaneStart = vec2(1.0 / width, uvPlaneY);

    highp vec2 texCoord = vec2(v_texCoord0.x, v_texCoord0.y * frameHeight / height);
    highp vec2 uvTexCoord = vec2(floor(texCoord.x * halfWidth) / halfWidth + (0.5 / width), texCoord.y * 0.5);

	highp float y = texture(tex0, texCoord + yPlaneStart).r;
    highp float u = texture(tex0, uvTexCoord + uPlaneStart).r;
    highp float v = texture(tex0, uvTexCoord + vPlaneStart).r;
    
    highp float c = 1.164383 * (y - 0.0625);
    highp float d = u - 0.5;
    highp float e = v - 0.5;

    highp float r = c + 1.596027 * e;
    highp float g = c - 0.391762 * d - 0.812968 * e;
    highp float b = c + 2.017232 * d;

    outCol = vec4(clamp(r, 0, 1), clamp(g, 0, 1), clamp(b, 0, 1), 1.0) * v_colour + v_colourAdd;
}

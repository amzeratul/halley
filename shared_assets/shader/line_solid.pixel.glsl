in highp vec4 v_colour;
in highp float v_vertPos;
in highp float v_width;
in highp vec2 v_worldPos;

out highp vec4 outCol;

void main() {
    highp float edge = v_width * 0.5;
    highp float invFeather = 2.0 / dFdx(v_worldPos.x);
    highp float v = clamp((edge - abs(v_vertPos)) * invFeather, -1, 1) * 0.5 + 0.5;
    highp float a = v_colour.a * v;

	outCol = vec4(v_colour.rgb * a, a);
}

in vec4 v_colour;
in float v_vertPos;
in float v_width;
in vec2 v_worldPos;

out vec4 outCol;

void main() {
    float edge = v_width * 0.5;
    float invFeather = 2.0 / dFdx(v_worldPos.x);
    float v = clamp((edge - abs(v_vertPos)) * invFeather, -1, 1) * 0.5 + 0.5;
    float a = v_colour.a * v;

	outCol = vec4(v_colour.rgb * a, a);
}

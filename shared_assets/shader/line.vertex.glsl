layout(std140) uniform HalleyBlock {
	highp mat4 u_mvp;
};

in highp vec4 a_colour;
in highp vec2 a_position;
in highp vec2 a_normal;
in highp vec2 a_width;

out vec4 v_colour;
out float v_vertPos;
out float v_width;
out vec2 v_worldPos;

void main() {
    highp float width = a_width.x;
    highp float myPos = a_width.y;
    highp float vertPos = 0.5 * (width + 1) * myPos;
    highp vec2 pos = a_position + vertPos * a_normal;
    v_colour = a_colour;
    v_vertPos = vertPos;
    v_width = width;
    v_worldPos = pos;

    gl_Position = u_mvp * vec4(pos, 0, 1);
}

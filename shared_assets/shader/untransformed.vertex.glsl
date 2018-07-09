in vec4 a_vertPos;
in vec2 a_position;
in vec2 a_size;
in vec4 a_colour;

out vec4 v_colour;

void main() {
    v_colour = a_colour;
    gl_Position = vec4(a_position + a_size * a_vertPos.xy, 0.0, 1.0);
}

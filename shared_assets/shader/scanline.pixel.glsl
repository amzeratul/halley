#version 140

uniform vec4 u_col0;
uniform vec4 u_col1;
uniform float u_distance;

in vec4 v_texCoord0;
in vec4 v_colour;
in vec4 v_colourAdd;

out vec4 outCol;

void main() {
	vec4 col = mix(u_col0, u_col1, fract(v_texCoord0.y / u_distance));
	outCol = col * v_colour;
}

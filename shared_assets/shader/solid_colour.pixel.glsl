#version 140

in vec4 v_texCoord0;
in vec4 v_colour;
in vec4 v_colourAdd;

out vec4 outCol;

void main() {
	outCol = v_colour;
}

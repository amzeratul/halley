#version 140

uniform sampler2D tex0;

in vec4 v_texCoord0;
in vec4 v_color;

out vec4 outCol;

void main() {
	outCol = texture(tex0, v_texCoord0.xy) * v_color;
}

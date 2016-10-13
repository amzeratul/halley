#version 140

uniform sampler2D tex0;

in vec4 v_texCoord0;

out vec4 outCol;

void main() {
	outCol = texture(tex0, v_texCoord0.xy) * vec4(1, 0, 0, 0.5);
}

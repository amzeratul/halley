#version 140

uniform sampler2D tex0;

in vec4 v_texCoord0;
in vec4 v_color;
in vec4 v_colorAdd;

out vec4 outCol;

void main() {
	vec4 col = texture(tex0, v_texCoord0.xy);
	outCol = col * v_color + v_colorAdd * col.a;
}

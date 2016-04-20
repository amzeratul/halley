#version 140

uniform sampler2D tex0;

in vec4 v_texCoord0;
in vec4 v_color;
in vec4 v_colorAdd;

out vec4 outCol;

void main() {
	float a = texture(tex0, v_texCoord0.xy).a;
	float edge = smoothstep(0.27, 0.33, a);
	float outline = 1.0 - smoothstep(0.47, 0.53, a);
	vec3 colFill = vec3(1, 1, 1);
	vec3 colOutline = vec3(0, 0, 0);
	outCol = vec4(mix(colFill, colOutline, outline), edge);
}

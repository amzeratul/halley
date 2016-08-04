#version 140

uniform sampler2D tex0;
uniform float u_smoothness;
uniform float u_outline;
uniform vec4 u_outlineColour;

in vec4 v_texCoord0;
in vec4 v_colour;
in vec4 v_colourAdd;

out vec4 outCol;

void main() {
	float a = texture(tex0, v_texCoord0.xy).a;
	float s = u_smoothness * 0.5;
	float inEdge = 0.5;

	float edge = smoothstep(clamp(inEdge - s, 0.01, 1.0), clamp(inEdge + s, 0.0, 0.99), a) * v_colour.a;
	vec4 colFill = v_colour;
	outCol = vec4(colFill.rgb, colFill.a * edge);
}

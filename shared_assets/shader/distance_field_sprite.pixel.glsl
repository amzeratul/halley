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
	float outEdge = inEdge - clamp(u_outline, 0.0, 0.95) * 0.5;

	float edge = smoothstep(clamp(outEdge - s, 0.01, 1.0), clamp(outEdge + s, 0.0, 0.99), a);
	float outline = 1.0 - smoothstep(inEdge - s, inEdge + s, a);
	vec4 colFill = v_colour;
	vec4 colOutline = u_outlineColour;
	vec4 col = mix(colFill, colOutline, outline);
	outCol = vec4(col.rgb, col.a * edge);
}

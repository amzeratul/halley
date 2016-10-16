#version 140

uniform sampler2D tex0;
uniform float u_smoothness;
uniform float u_outline;
uniform vec4 u_outlineColour;

in vec2 v_texCoord0;
in vec2 v_pixelTexCoord0;
in vec4 v_colour;
in vec4 v_colourAdd;

in vec4 gl_FragCoord;

out vec4 outCol;

void main() {
	float dx = abs(dFdx(v_pixelTexCoord0.x) / dFdx(gl_FragCoord.x));
	float dy = abs(dFdy(v_pixelTexCoord0.y) / dFdy(gl_FragCoord.y));
	float texGrad = max(dx, dy);

	float a = texture(tex0, v_texCoord0).a;
	float s = u_smoothness * texGrad;
	float inEdge = 0.5;

	float edge = smoothstep(clamp(inEdge - s, 0.01, 1.0), clamp(inEdge + s, 0.0, 0.99), a) * v_colour.a;
	vec4 colFill = v_colour;
	outCol = vec4(colFill.rgb, colFill.a * edge);
}

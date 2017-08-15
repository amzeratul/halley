uniform sampler2D tex0;
layout(std140) uniform MaterialBlock {
	float u_smoothness;
	float u_outline;
	vec4 u_outlineColour;
};

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
	float s = max(u_smoothness * texGrad, 0.001);
	float inEdge = 0.51;

	float edge0 = clamp(inEdge - s, 0.01, 0.98);
	float edge1 = clamp(inEdge + s, edge0 + 0.01, 0.99);
	float edge = smoothstep(edge0, edge1, a) * v_colour.a;
	vec4 colFill = v_colour;
	outCol = vec4(colFill.rgb, colFill.a * edge);
}

uniform sampler2D tex0;
layout(std140) uniform MaterialBlock {
	highp float u_smoothness;
	highp float u_outline;
	highp vec4 u_outlineColour;
};

in highp vec2 v_texCoord0;
in highp vec2 v_pixelTexCoord0;
in highp vec4 v_colour;
in highp vec4 v_colourAdd;

out highp vec4 outCol;

void main() {
	highp float dx = abs(dFdx(v_pixelTexCoord0.x) / dFdx(gl_FragCoord.x));
	highp float dy = abs(dFdy(v_pixelTexCoord0.y) / dFdy(gl_FragCoord.y));
	highp float texGrad = max(dx, dy);

	highp float a = texture(tex0, v_texCoord0).r;
	highp float s = max(u_smoothness * texGrad, 0.001);
	highp float inEdge = 0.51;

	highp float edge0 = clamp(inEdge - s, 0.01, 0.98);
	highp float edge1 = clamp(inEdge + s, edge0 + 0.01, 0.99);
	highp float edge = smoothstep(edge0, edge1, a) * v_colour.a;
	highp vec4 colFill = v_colour;
	outCol = vec4(colFill.rgb, colFill.a * edge);
}

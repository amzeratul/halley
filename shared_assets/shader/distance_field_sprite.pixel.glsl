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
	highp float dx = abs(dFdx(v_texCoord0.x));
	highp float dy = abs(dFdy(v_texCoord0.y));
	highp float texGrad = max(dx, dy);

	highp float a = texture(tex0, v_texCoord0).r;
	highp float s = u_smoothness * texGrad;
	highp float inEdge = 0.5;
	highp float outEdge = inEdge - clamp(u_outline, 0.0, 0.995) * 0.5;

	highp float edge = smoothstep(clamp(outEdge - s, 0.001, 1.0), clamp(outEdge + s, 0.0, 0.999), a);
	highp float outline = 1.0 - smoothstep(inEdge - s, inEdge + s, a);
	highp vec4 colFill = v_colour;
	highp vec4 colOutline = u_outlineColour;
	highp vec4 col = mix(colFill, colOutline, outline);
	highp float alpha = col.a * edge;
	outCol = vec4(col.rgb * alpha, alpha);
}

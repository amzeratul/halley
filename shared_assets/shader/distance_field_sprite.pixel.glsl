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
	vec2 dxy = abs(dFdy(v_pixelTexCoord0.xy) / dFdy(gl_FragCoord.xy));
	//float dx = abs(dFdx(v_pixelTexCoord0.x) / dFdx(gl_FragCoord.x));
	//float dy = abs(dFdy(v_pixelTexCoord0.y) / dFdy(gl_FragCoord.y));
	float texGrad = max(dxy.x, dxy.y);

	float a = texture(tex0, v_texCoord0).a;
	float s = u_smoothness * texGrad;
	float inEdge = 0.5;
	float outEdge = inEdge - clamp(u_outline, 0.0, 0.995) * 0.5;

	float edge = smoothstep(clamp(outEdge - s, 0.001, 1.0), clamp(outEdge + s, 0.0, 0.999), a);
	float outline = 1.0 - smoothstep(inEdge - s, inEdge + s, a);
	vec4 colFill = v_colour;
	vec4 colOutline = u_outlineColour;
	vec4 col = mix(colFill, colOutline, outline);
	outCol = vec4(col.rgb, col.a * edge);
}

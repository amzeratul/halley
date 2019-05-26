uniform sampler2D tex0;

in highp vec2 v_texCoord0;
in highp vec4 v_colour;
in highp vec4 v_colourAdd;

out highp vec4 outCol;

void main() {
	highp vec4 col = texture(tex0, v_texCoord0.xy);
	outCol = col * v_colour + v_colourAdd * col.a;
}

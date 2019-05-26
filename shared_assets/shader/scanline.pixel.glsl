layout(std140) uniform MaterialBlock {
	highp vec4 u_col0;
	highp vec4 u_col1;
	highp float u_distance;
};

in highp vec2 v_texCoord0;
in highp vec4 v_colour;
in highp vec4 v_colourAdd;

out highp vec4 outCol;

void main() {
	highp vec4 col = mix(u_col0, u_col1, fract(v_texCoord0.y / u_distance));
	outCol = col * v_colour;
}

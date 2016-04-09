#version 150

uniform mat4 u_mvp;
attribute vec4 a_position;
attribute vec4 a_size;
attribute vec4 a_color;
attribute vec4 a_texCoord0;
attribute vec2 a_vertPos;

varying vec4 v_texCoord0;
varying vec4 v_color;

void main() {
	v_texCoord0 = vec4(mix(a_texCoord0.x, a_texCoord0.z, a_vertPos.x), mix(a_texCoord0.y, a_texCoord0.w, a_vertPos.y), 0.0, 0.0);
	v_color = a_color;
	
	float c = cos(a_size.z);
	float s = sin(a_size.z);
	mat2 m = mat2(c, s, -s, c);
	vec2 pos = a_position.xy + m * ((a_vertPos - a_position.zw) * a_size.xy);
	gl_Position = u_mvp * vec4(pos.x, pos.y, 0.0, 1.0);
}

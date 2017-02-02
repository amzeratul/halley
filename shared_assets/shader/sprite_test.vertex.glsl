uniform mat4 u_mvp;
uniform float u_time;

in vec4 a_position;
in vec4 a_size;
in vec2 a_texCoord0;
in vec4 a_vertPos;

out vec4 v_texCoord0;
//out vec4 v_colour;

void main() {
	v_texCoord0 = vec4(mix(a_texCoord0.x, a_texCoord0.z, a_vertPos.x), mix(a_texCoord0.y, a_texCoord0.w, a_vertPos.y), 0.0, 0.0);
	
	float c = cos(a_size.z + u_time * 1.32);
	float s = sin(a_size.z + u_time * 1.32);
	mat2 m = mat2(c, s, -s, c);
	vec2 pos = a_position.xy + m * ((a_vertPos.xy - a_position.zw) * a_size.xy) + vec2(cos(u_time) * 10, sin(u_time) * 10);
	gl_Position = u_mvp * vec4(pos.x, pos.y, 0.0, 1.0);
}

#version 140

uniform mat4 u_mvp;

in vec4 a_position;
in vec4 a_size;
in vec4 a_color;
in vec4 a_texCoord0;
in vec2 a_vertPos;

out vec4 v_texCoord0;
out vec4 v_color;
out vec4 v_colorAdd;

void main() {
	v_texCoord0 = vec4(mix(a_texCoord0.x, a_texCoord0.z, a_vertPos.x), mix(a_texCoord0.y, a_texCoord0.w, a_vertPos.y), 0.0, 0.0);
	
	vec4 inputCol = vec4(a_color.rgb * a_color.a, a_color.a); // Premultiply alpha
	vec4 baseCol = clamp(inputCol, vec4(0, 0, 0, 0), vec4(1, 1, 1, 1));
	v_color = baseCol;
	v_colorAdd = clamp(inputCol - baseCol, vec4(0, 0, 0, 0), vec4(1, 1, 1, 0));
	
	float c = cos(a_size.z);
	float s = sin(a_size.z);
	mat2 m = mat2(c, s, -s, c);
	vec2 pos = a_position.xy + m * ((a_vertPos - a_position.zw) * a_size.xy);
	gl_Position = u_mvp * vec4(pos.x, pos.y, 0.0, 1.0);
}

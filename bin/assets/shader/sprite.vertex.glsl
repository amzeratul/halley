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

vec4 getTexCoord(vec4 texCoords, vec2 vertPos, float texCoordRotation) {
	vec2 texPos = mix(vertPos, vec2(1.0 - vertPos.y, vertPos.x), texCoordRotation);
	return vec4(mix(texCoords.xy, texCoords.zw, texPos.xy), 0.0, 0.0);
}

void getColours(vec4 inColour, out vec4 baseColour, out vec4 addColour) {
	vec4 inputCol = vec4(inColour.rgb * inColour.a, inColour.a); // Premultiply alpha
	vec4 baseCol = clamp(inputCol, vec4(0, 0, 0, 0), vec4(1, 1, 1, 1));
	baseColour = baseCol;
	addColour = clamp(inputCol - baseCol, vec4(0, 0, 0, 0), vec4(1, 1, 1, 0));
}

vec4 getVertexPosition(vec2 position, vec2 pivot, vec2 size, vec2 vertPos, float angle) {
	float c = cos(angle);
	float s = sin(angle);
	mat2 m = mat2(c, s, -s, c);
	
	vec2 pos = position + m * ((vertPos - pivot) * size);
	return u_mvp * vec4(pos, 0.0, 1.0);
}

void main() {
	v_texCoord0 = getTexCoord(a_texCoord0, a_vertPos, a_size.w);
	getColours(a_color, v_color, v_colorAdd);
	gl_Position = getVertexPosition(a_position.xy, a_position.zw, a_size.xy, a_vertPos, a_size.z);
}

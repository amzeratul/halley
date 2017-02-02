uniform mat4 u_mvp;

in vec4 a_vertPos;
in vec2 a_position;
in vec2 a_pivot;
in vec2 a_size;
in vec2 a_scale;
in vec4 a_colour;
in vec4 a_texCoord0;
in float a_rotation;
in float a_textureRotation;

out vec2 v_texCoord0;
out vec2 v_pixelTexCoord0;
out vec4 v_colour;
out vec4 v_colourAdd;
out vec2 v_vertPos;
out vec2 v_pixelPos;

vec2 getTexCoord(vec4 texCoords, vec2 vertPos, float texCoordRotation) {
	vec2 texPos = mix(vertPos, vec2(1.0 - vertPos.y, vertPos.x), texCoordRotation);
	return vec2(mix(texCoords.xy, texCoords.zw, texPos.xy));
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
	v_texCoord0 = getTexCoord(a_texCoord0, a_vertPos.zw, a_textureRotation);
	v_pixelTexCoord0 = v_texCoord0 * a_size;
	v_vertPos = a_vertPos.xy;
	v_pixelPos = a_size * a_scale * a_vertPos.xy;
	getColours(a_colour, v_colour, v_colourAdd);
	gl_Position = getVertexPosition(a_position, a_pivot, a_size * a_scale, a_vertPos.xy, a_rotation);
}

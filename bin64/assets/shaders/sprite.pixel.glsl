#version 120

uniform sampler2D tex0;
varying vec4 v_texCoord0;
varying vec4 v_color;

void main() {
	gl_FragColor = texture2D(tex0, v_texCoord0.xy) * v_color;
}

#version 110
varying vec4 color;
varying vec4 coords;
uniform float displacement;
uniform vec2 scale;
uniform float time;

float deform(vec2 pos) {
	return sin(pos.x) * cos(pos.y);
}

void main(void) {
	color = gl_Color;
	vec4 vertex = gl_Vertex;
	vec2 position = vertex.xy * scale + time;
	vertex.z += displacement * deform(position);
	gl_Position = gl_ModelViewProjectionMatrix * vertex;
	coords = vec4(gl_MultiTexCoord0.xy,gl_MultiTexCoord1.xy);
}
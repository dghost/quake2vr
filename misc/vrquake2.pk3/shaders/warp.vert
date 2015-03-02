#version 110
varying vec4 color;
varying vec4 coords;
uniform float displacement;
uniform vec2 scale;
uniform float time;

float deform(vec2 pos, float time) {
	vec2 position = pos * scale + time;
	return sin(position.x) * cos(position.y);
}

void main(void) {
	color = gl_Color;
	vec4 vertex = gl_Vertex;
	vertex.z += displacement * deform(vertex.xy, time);
	gl_Position = gl_ModelViewProjectionMatrix * vertex;
	vec4 eyePos = gl_ModelViewMatrix * vertex;
	gl_FogFragCoord = abs(eyePos.z/eyePos.w);
	coords = vec4(gl_MultiTexCoord0.xy,gl_MultiTexCoord1.xy);
}

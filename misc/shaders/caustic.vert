#version 110
varying vec4 color;
varying vec4 coords;

void main(void) {
	color = gl_Color;
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	coords = vec4(gl_MultiTexCoord0.xy,gl_MultiTexCoord1.xy);
}
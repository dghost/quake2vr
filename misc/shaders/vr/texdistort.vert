#version 110

varying vec2 texCoords;
void main(void) {
	gl_Position = gl_ProjectionMatrix * gl_Vertex;
	texCoords = gl_MultiTexCoord0.xy;
}
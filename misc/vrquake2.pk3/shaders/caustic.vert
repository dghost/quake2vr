#version 110
varying vec4 color;
varying vec4 coords;

void main(void) {
	color = gl_Color;
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	vec4 eyePos = gl_ModelViewMatrix * gl_Vertex;
	gl_FogFragCoord = abs(eyePos.z/eyePos.w);
	coords = vec4(gl_MultiTexCoord0.xy,gl_MultiTexCoord1.xy);
}

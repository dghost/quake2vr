#version 110

attribute vec2 Position;
attribute vec2 TexCoord;

varying vec2 texCoords;

void main(void) {
	gl_Position = vec4(Position,1.0,1.0);
	texCoords = TexCoord;
}
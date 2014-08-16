#version 110

varying vec2 texCoords;

uniform sampler2D tex;

void main(void)
{
	gl_FragColor = texture2D( tex, texCoords );	
}

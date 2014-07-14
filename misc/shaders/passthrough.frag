#version 120

varying vec2 texCoords;

uniform sampler2D tex;

void main(void)
{
	gl_FragColor = texture2D( tex, texCoords );	
}

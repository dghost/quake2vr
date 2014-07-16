#version 110

varying vec2 texCoords;

uniform sampler2D tex;

uniform float gamma;

void main(void)
{
	vec4 color = texture2D( tex, texCoords );
	gl_FragColor.rgb = pow(color.rgb, vec3( 1.0/ gamma) );
	gl_FragColor.a = color.a;
}

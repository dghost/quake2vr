#version 110

varying vec2 texCoords;
uniform vec3 minColor;

uniform sampler2D tex;

uniform float gamma;

void main(void)
{
	vec4 color = texture2D( tex, texCoords );
	gl_FragColor.rgb = max(pow(color.rgb, vec3( 1.0/ gamma) ),minColor);
	gl_FragColor.a = color.a;
}

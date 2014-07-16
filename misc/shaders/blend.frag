#version 110

varying vec2 texCoords;

uniform sampler2D tex;

uniform vec4 blendColor;

void main(void)
{
	vec4 FragmentColor = texture2D( tex, texCoords );
	gl_FragColor = vec4(mix(FragmentColor.rgb,blendColor.rgb,blendColor.a),1.0);	
}

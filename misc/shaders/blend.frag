#version 120

varying vec2 texCoords;

uniform sampler2D tex;

uniform vec4 blendColor = vec4(1.0);

void main(void)
{
	vec4 FragmentColor = texture2D( tex, texCoords );
	gl_FragColor = vec4(mix(FragmentColor.rgb,blendColor.rgb,blendColor.a),1.0);	
}

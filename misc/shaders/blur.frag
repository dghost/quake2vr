#version 120

varying vec2 texCoords[9];

uniform sampler2D tex;

uniform float weight[5] = float[]( 0.2270270270, 0.1945945946, 0.1216216216,
                                   0.0540540541, 0.0162162162 );

void main(void)
{
	vec4 FragmentColor = texture2D( tex, texCoords[4] ) * weight[0];
    for (int i=1; i<5; i++) {
		if (weight[i] > 0)
		{
			vec4 samples = texture2D( tex, texCoords[4 + i] ) + texture2D( tex, texCoords[4 - i] );
			FragmentColor = samples * weight[i] + FragmentColor;
		}
	}
	gl_FragColor = FragmentColor;	
}

#version 120

varying vec2 texCoords[9];

uniform sampler2D blurTex;
uniform sampler2D sourceTex;

uniform float weight[5] = float[]( 0.2270270270, 0.1945945946, 0.1216216216,
                                   0.0540540541, 0.0162162162 );

void main(void)
{

//	vec4 DestinationColor = texture2D( sourceTex, texCoords[4] );
	vec4 FragmentColor = texture2D( blurTex, texCoords[4] ) * weight[0];
    for (int i=1; i<5; i++) {
		if (weight[i] > 0)
		{
			vec4 samples = texture2D( blurTex, texCoords[4 + i] ) + texture2D( blurTex, texCoords[4 - i] );
			FragmentColor = samples * weight[i] + FragmentColor;
		}
	}
	gl_FragColor = FragmentColor;// + (1.0 - DestinationColor);	
}

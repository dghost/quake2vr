#version 120

varying vec2 texCoords[9];

uniform sampler2D tex;

uniform float weight[5] = float[]( 0.2270270270, 0.1945945946, 0.1216216216,
                                   0.0540540541, 0.0162162162 );

void main(void)
{
	vec4 FragmentColor = vec4(0.0);
    for (int i=-4; i<=4; i++) {
		int w = (i >= 0 ? i : -i);
		if (weight[w] > 0) {
			FragmentColor += texture2D( tex, texCoords[4 + i]) * weight[w];
		}
	}
	gl_FragColor = FragmentColor;	
}

#version 120

varying vec2 texCoords[9];

uniform sampler2D blurTex;

uniform float minThreshold  = 0.15;
uniform vec3 lumConversion = vec3(0.299, 0.587, 0.114);  // ITU-R BT.601 luma coefficients
uniform float weight[5] = float[]( 0.2270270270, 0.1945945946, 0.1216216216,
                                   0.0540540541, 0.0162162162 );

uniform float falloff = 1.5;

vec4 filter(sampler2D texture, vec2 tc)
{
	vec4 sample = texture2D( texture, tc );
	float lum = dot(sample.rgb, lumConversion);
	float lum2 = max(lum - minThreshold, 0.0f);
	sample.rgb *= (lum2/lum);
	sample.rgb = pow(sample.rgb,vec3(falloff));
	return sample;
}

void main(void)
{
	vec4 FragmentColor = vec4(0.0);
    for (int i=-4; i<=4; i++) {
		int w = (i >= 0 ? i : -i);
		if (weight[w] > 0) {
			FragmentColor += filter( blurTex, texCoords[4 + i]) * weight[w];
		}
	}
	gl_FragColor = FragmentColor;	
}

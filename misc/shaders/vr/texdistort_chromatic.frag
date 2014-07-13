#version 110

varying vec2 texCoords;
uniform sampler2D tex;
uniform sampler2D dist;

void main()
{
	vec4 vRead = texture2D(dist,texCoords);
	vec2 tcGreen = vec2(vRead.r + vRead.b, vRead.g + vRead.a) / 2.0;
	if (!all(equal(clamp(tcGreen, vec2(0.0,0.0), vec2(1.0,1.0)),tcGreen)))
	{
		gl_FragColor = vec4(0.0,0.0,0.0,1.0);
	}
	else
	{
		vec4 final;
		final.r = texture2D(tex, vRead.rg).r;
		final.ga = texture2D(tex, tcGreen).ga;
		final.b = texture2D(tex, vRead.ba).b;
		gl_FragColor = final;
	}
}
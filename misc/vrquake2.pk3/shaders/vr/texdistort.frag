#version 110

varying vec2 texCoords;
uniform sampler2D tex;
uniform sampler2D dist;

void main()
{
	vec2 tc = texture2D(dist,texCoords).rg;
	if (any(bvec2(clamp(tc, vec2(0.0,0.0), vec2(1.0,1.0))-tc)))
		gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
	else
		gl_FragColor = texture2D(tex, tc);
}
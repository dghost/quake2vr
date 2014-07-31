#version 110

uniform sampler2D currentFrame;
uniform sampler2D lastFrame;

varying vec4 oColor;
varying vec2 oTexCoord0;
varying vec2 oTexCoord1;
varying vec2 oTexCoord2;

uniform vec2 OverdriveScales;

void main()
{
	float ResultR = texture2DLod(currentFrame, oTexCoord0, 0.0).r;
	float ResultG = texture2DLod(currentFrame, oTexCoord1, 0.0).g;
	float ResultB = texture2DLod(currentFrame, oTexCoord2, 0.0).b;
	vec4 outColor = vec4(ResultR * oColor.r, ResultG * oColor.g, ResultB * oColor.b, 1.0);

	// pixel luminance overdrive
	
	if(OverdriveScales.x > 0.0)
	{
		vec3 oldColor = texture2DLod(lastFrame, (gl_FragCoord.xy * 0.5 + 0.5), 0.0).rgb;
		
		vec3 newColor = outColor.rgb;
		
		vec3 adjustedScales;
		adjustedScales.x = newColor.x > oldColor.x ? OverdriveScales.x : OverdriveScales.y;
		adjustedScales.y = newColor.y > oldColor.y ? OverdriveScales.x : OverdriveScales.y;
		adjustedScales.z = newColor.z > oldColor.z ? OverdriveScales.x : OverdriveScales.y;
		
		vec3 overdriveColor = clamp(newColor + (newColor - oldColor) * adjustedScales,0.0,1.0);
		outColor = vec4(overdriveColor, 1.0);
	}   

	gl_FragColor = outColor;
};

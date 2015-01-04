#version 110

uniform sampler2D currentFrame;
uniform sampler2D lastFrame;

varying vec4 oColor;
varying vec2 oTexCoord0;
varying vec2 oTexCoord1;
varying vec2 oTexCoord2;

uniform vec2 InverseResolution;
uniform vec2 OverdriveScales;
uniform float VignetteFade;
uniform float Desaturate;
vec3 lumConversion = vec3(0.299, 0.587, 0.114);  // ITU-R BT.601 luma coefficients

void main()
{
	float ResultR = texture2DLod(currentFrame, oTexCoord0, 0.0).r;
	float ResultG = texture2DLod(currentFrame, oTexCoord1, 0.0).g;
	float ResultB = texture2DLod(currentFrame, oTexCoord2, 0.0).b;
	vec3 newColor = vec3(ResultR, ResultG, ResultB);
	
	if (Desaturate > 0.0)
	{
		float lum = dot(newColor,lumConversion);
		newColor = mix(newColor,vec3(lum),Desaturate);
	}

	if (VignetteFade > 0.0)
	{
		newColor = newColor * oColor.xxx;
	}	
		
	// pixel luminance overdrive
	if(OverdriveScales.x > 0.0)
	{
		vec3 oldColor = texture2DLod(lastFrame, (gl_FragCoord.xy * InverseResolution), 0.0).rgb;	
		
		vec3 adjustedScales;
		adjustedScales.x = newColor.x > oldColor.x ? OverdriveScales.x : OverdriveScales.y;
		adjustedScales.y = newColor.y > oldColor.y ? OverdriveScales.x : OverdriveScales.y;
		adjustedScales.z = newColor.z > oldColor.z ? OverdriveScales.x : OverdriveScales.y;
		
		newColor = clamp(newColor + (newColor - oldColor) * adjustedScales,0.0,1.0);
	}   
	gl_FragColor = vec4(newColor,1.0);		
}
#version 110
#extension EXT_gpu_shader4: enable

#ifdef EXT_gpu_shader4
#define TEXTURE texture2DLod
#else
#define TEXTURE texture2D
#endif

uniform sampler2D currentFrame;
uniform sampler2D lastFrame;

varying vec4 oColor;
varying vec2 oTexCoord;


uniform vec2 InverseResolution;
uniform vec2 OverdriveScales;
uniform bool VignetteFade;
uniform float Desaturate;
const vec3 lumConversion = vec3(0.299, 0.587, 0.114);  // ITU-R BT.601 luma coefficients

void main()
{
	vec3 newColor = TEXTURE(currentFrame, oTexCoord, 0.0).rgb;
	
	if (Desaturate > 0.0)
	{
		float lum = dot(newColor,lumConversion);
		newColor = mix(newColor,vec3(lum),Desaturate);
	}

	if (VignetteFade)
	{
		newColor = newColor * oColor.xxx;
	}	
	
	// pixel luminance overdrive
	if(OverdriveScales.x > 0.0)
	{
		vec3 oldColor = TEXTURE(lastFrame, (gl_FragCoord.xy * InverseResolution), 0.0).rgb;	
		
		vec3 adjustedScales;
		adjustedScales.x = newColor.x > oldColor.x ? OverdriveScales.x : OverdriveScales.y;
		adjustedScales.y = newColor.y > oldColor.y ? OverdriveScales.x : OverdriveScales.y;
		adjustedScales.z = newColor.z > oldColor.z ? OverdriveScales.x : OverdriveScales.y;
		
		newColor = clamp(newColor + (newColor - oldColor) * adjustedScales,0.0,1.0);
	}   
	gl_FragColor = vec4(newColor,1.0);		
}
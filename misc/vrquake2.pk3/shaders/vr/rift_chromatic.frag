#version 110
#extension EXT_gpu_shader4: enable

#ifdef EXT_gpu_shader4
#define TEXTURE(tex, coord) texture2DLod(tex,coord,0.0)
#else
#define TEXTURE(tex,coord) texture2D(tex, coord) 
#endif

uniform sampler2D CurrentFrame;
uniform sampler2D LastFrame;

varying vec4 oColor;
varying vec2 oTexCoord0;
varying vec2 oTexCoord1;
varying vec2 oTexCoord2;

uniform vec2 InverseResolution;
uniform vec2 OverdriveScales;
uniform bool VignetteFade;
uniform float Desaturate;

const vec3 lumConversion = vec3(0.299, 0.587, 0.114);  // ITU-R BT.601 luma coefficients

void main()
{
	float ResultR = TEXTURE(CurrentFrame, oTexCoord0).r;
	float ResultG = TEXTURE(CurrentFrame, oTexCoord1).g;
	float ResultB = TEXTURE(CurrentFrame, oTexCoord2).b;
	vec3 newColor = vec3(ResultR, ResultG, ResultB);
	
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
		vec3 oldColor = TEXTURE(LastFrame, (gl_FragCoord.xy * InverseResolution)).rgb;	
		
		vec3 adjustedScales;
		adjustedScales.x = newColor.x > oldColor.x ? OverdriveScales.x : OverdriveScales.y;
		adjustedScales.y = newColor.y > oldColor.y ? OverdriveScales.x : OverdriveScales.y;
		adjustedScales.z = newColor.z > oldColor.z ? OverdriveScales.x : OverdriveScales.y;
		
		newColor = clamp(newColor + (newColor - oldColor) * adjustedScales,0.0,1.0);
	}   
	gl_FragColor = vec4(newColor,1.0);		
}
#version 110

uniform sampler2D tex;
uniform vec3 minColor;

varying vec4 oColor;
varying vec2 oTexCoord;

void main()
{
   vec3 color = max(texture2DLod(tex, oTexCoord, 0.0).rgb * oColor.rgb,minColor);   
   gl_FragColor = vec4(color,1.0);
};
#version 110

uniform sampler2D tex;

varying vec4 oColor;
varying vec2 oTexCoord0;
varying vec2 oTexCoord1;
varying vec2 oTexCoord2;


void main()
{
   float ResultR = texture2DLod(tex, oTexCoord0, 0.0).r;
   float ResultG = texture2DLod(tex, oTexCoord1, 0.0).g;
   float ResultB = texture2DLod(tex, oTexCoord2, 0.0).b;
   gl_FragColor = vec4(ResultR * oColor.r, ResultG * oColor.g, ResultB * oColor.b, 1.0);
};

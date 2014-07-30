#version 110

uniform sampler2D tex;

varying vec4 oColor;
varying vec2 oTexCoord;

void main()
{
   gl_FragColor = texture2DLod(tex, oTexCoord, 0.0) * oColor;
   gl_FragColor.a = 1.0;
};
#version 110

uniform vec2 EyeToSourceUVScale;
uniform vec2 EyeToSourceUVOffset;

attribute vec2 Position;
attribute vec4 Color;
attribute vec2 TexCoord;

varying vec4 oColor;
varying vec2 oTexCoord;

void main()
{
   gl_Position.x = Position.x;
   gl_Position.y = Position.y;
   gl_Position.z = 0.5;
   gl_Position.w = 1.0;
// Vertex inputs are in TanEyeAngle space for the R,G,B channels (i.e. after chromatic aberration and distortion).
// Scale them into the correct [0-1],[0-1] UV lookup space (depending on eye)
   oTexCoord = TexCoord * EyeToSourceUVScale + EyeToSourceUVOffset;
   oTexCoord.y = 1.0 - oTexCoord.y;
   oColor = Color;              // Used for vignette fade.
}
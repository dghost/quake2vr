uniform vec2 EyeToSourceUVScale;
uniform vec2 EyeToSourceUVOffset;

attribute vec2 Position;
attribute vec4 Color;
attribute vec2 TexCoord0;
attribute vec2 TexCoord1;
attribute vec2 TexCoord2;

varying vec4 oColor;
varying vec2 oTexCoord0;
varying vec2 oTexCoord1;
varying vec2 oTexCoord2;

void main()
{
   gl_Position.x = Position.x;
   gl_Position.y = Position.y;
   gl_Position.z = 0.5;
   gl_Position.w = 1.0;

// Vertex inputs are in TanEyeAngle space for the R,G,B channels (i.e. after chromatic aberration and distortion).
// Scale them into the correct [0-1],[0-1] UV lookup space (depending on eye)
   oTexCoord0 = TexCoord0 * EyeToSourceUVScale + EyeToSourceUVOffset;
   oTexCoord0.y = 1.0-oTexCoord0.y;
   oTexCoord1 = TexCoord1 * EyeToSourceUVScale + EyeToSourceUVOffset;
   oTexCoord1.y = 1.0-oTexCoord1.y;
   oTexCoord2 = TexCoord2 * EyeToSourceUVScale + EyeToSourceUVOffset;
   oTexCoord2.y = 1.0-oTexCoord2.y;

   oColor = Color; // Used for vignette fade.
};
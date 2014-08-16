#version 110

uniform vec2 EyeToSourceUVScale;
uniform vec2 EyeToSourceUVOffset;
uniform mat4 EyeRotationStart;
uniform mat4 EyeRotationEnd;

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
   gl_Position.z = 0.0;
   gl_Position.w = 1.0;

// Vertex inputs are in TanEyeAngle space for the R,G,B channels (i.e. after chromatic aberration and distortion).
// These are now real world vectors in direction (x,y,1) relative to the eye of the HMD.
   vec3 TanEyeAngleR = vec3 ( TexCoord0.x, TexCoord0.y, 1.0 );
   vec3 TanEyeAngleG = vec3 ( TexCoord1.x, TexCoord1.y, 1.0 );
   vec3 TanEyeAngleB = vec3 ( TexCoord2.x, TexCoord2.y, 1.0 );

// Accurate time warp lerp vs. faster
#if 1
// Apply the two 3x3 timewarp rotations to these vectors.
   vec3 TransformedRStart = (EyeRotationStart * vec4(TanEyeAngleR, 0)).xyz;
   vec3 TransformedGStart = (EyeRotationStart * vec4(TanEyeAngleG, 0)).xyz;
   vec3 TransformedBStart = (EyeRotationStart * vec4(TanEyeAngleB, 0)).xyz;
   vec3 TransformedREnd   = (EyeRotationEnd * vec4(TanEyeAngleR, 0)).xyz;
   vec3 TransformedGEnd   = (EyeRotationEnd * vec4(TanEyeAngleG, 0)).xyz;
   vec3 TransformedBEnd   = (EyeRotationEnd * vec4(TanEyeAngleB, 0)).xyz;

// And blend between them.
   vec3 TransformedR = mix ( TransformedRStart, TransformedREnd, Color.a );
   vec3 TransformedG = mix ( TransformedGStart, TransformedGEnd, Color.a );
   vec3 TransformedB = mix ( TransformedBStart, TransformedBEnd, Color.a );
#else
   mat3 EyeRotation;
   EyeRotation[0] = mix ( EyeRotationStart[0], EyeRotationEnd[0], Color.a ).xyz;
   EyeRotation[1] = mix ( EyeRotationStart[1], EyeRotationEnd[1], Color.a ).xyz;
   EyeRotation[2] = mix ( EyeRotationStart[2], EyeRotationEnd[2], Color.a ).xyz;
   vec3 TransformedR   = EyeRotation * TanEyeAngleR;
   vec3 TransformedG   = EyeRotation * TanEyeAngleG;
   vec3 TransformedB   = EyeRotation * TanEyeAngleB;
#endif

// Project them back onto the Z=1 plane of the rendered images.
   float RecipZR = 1.0 / TransformedR.z;
   float RecipZG = 1.0 / TransformedG.z;
   float RecipZB = 1.0 / TransformedB.z;
   vec2 FlattenedR = vec2 ( TransformedR.x * RecipZR, TransformedR.y * RecipZR );
   vec2 FlattenedG = vec2 ( TransformedG.x * RecipZG, TransformedG.y * RecipZG );
   vec2 FlattenedB = vec2 ( TransformedB.x * RecipZB, TransformedB.y * RecipZB );

// These are now still in TanEyeAngle space.
// Scale them into the correct [0-1],[0-1] UV lookup space (depending on eye)
   vec2 SrcCoordR = FlattenedR * EyeToSourceUVScale + EyeToSourceUVOffset;
   vec2 SrcCoordG = FlattenedG * EyeToSourceUVScale + EyeToSourceUVOffset;
   vec2 SrcCoordB = FlattenedB * EyeToSourceUVScale + EyeToSourceUVOffset;

   oTexCoord0 = SrcCoordR;
   oTexCoord0.y = 1.0-oTexCoord0.y;
   oTexCoord1 = SrcCoordG;
   oTexCoord1.y = 1.0-oTexCoord1.y;
   oTexCoord2 = SrcCoordB;
   oTexCoord2.y = 1.0-oTexCoord2.y;

   oColor = vec4(Color.r, Color.r, Color.r, Color.r);              // Used for vignette fade.
}
uniform vec2 EyeToSourceUVScale;
uniform vec2 EyeToSourceUVOffset;
uniform mat4 EyeRotationStart;
uniform mat4 EyeRotationEnd;

attribute vec2 Position;
attribute vec4 Color;
attribute vec2 TexCoord;

varying vec4 oColor;
varying vec2 oTexCoord;

void main()
{
   gl_Position.x = Position.x;
   gl_Position.y = Position.y;
   gl_Position.z = 0.0;
   gl_Position.w = 1.0;

// Vertex inputs are in TanEyeAngle space for the R,G,B channels (i.e. after chromatic aberration and distortion).
// These are now real world vectors in direction (x,y,1) relative to the eye of the HMD.
   vec3 TanEyeAngle = vec3 ( TexCoord.x, TexCoord.y, 1.0 );

// Accurate time warp lerp vs. faster
#if 1
// Apply the two 3x3 timewarp rotations to these vectors.
   vec3 TransformedStart = (EyeRotationStart * vec4(TanEyeAngle, 0)).xyz;
   vec3 TransformedEnd   = (EyeRotationEnd * vec4(TanEyeAngle, 0)).xyz;
// And blend between them.
   vec3 Transformed = mix ( TransformedStart, TransformedEnd, Color.a );
#else
   mat4 EyeRotation = mix ( EyeRotationStart, EyeRotationEnd, Color.a );
   vec3 Transformed   = EyeRotation * TanEyeAngle;
#endif

// Project them back onto the Z=1 plane of the rendered images.
   float RecipZ = 1.0 / Transformed.z;
   vec2 Flattened = vec2 ( Transformed.x * RecipZ, Transformed.y * RecipZ );

// These are now still in TanEyeAngle space.
// Scale them into the correct [0-1],[0-1] UV lookup space (depending on eye)
   vec2 SrcCoord = Flattened * EyeToSourceUVScale + EyeToSourceUVOffset;
   oTexCoord = SrcCoord;
   oTexCoord.y = 1.0-oTexCoord.y;
   oColor = vec4(Color.r, Color.r, Color.r, Color.r);              // Used for vignette fade.
}
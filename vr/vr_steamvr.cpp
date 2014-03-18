#ifndef NO_STEAM

#include "vr_steamvr.h"
#include <steamvr.h>
#include <cstdlib>

static vr::IHmd *hmd;


Sint32 SteamVR_Init()
{
	if (hmd)
	{
		vr::VR_Shutdown();
		hmd = NULL;
	}
	return !hmd;
}

Sint32 SteamVR_Enable()
{
	vr::HmdError error;
	hmd = vr::VR_Init(&error);
	if (hmd)
	{
		//		SteamVR_Disable();
	}
	return !!hmd;
}

void SteamVR_Disable()
{
	if (hmd)
	{
		vr::VR_Shutdown();
		hmd = NULL;
	}
}

void SteamVR_Shutdown()
{
	SteamVR_Disable();
}

Sint32 SteamVR_GetSettings(svr_settings_t *settings)
{
	if (hmd)
	{
		vr::HmdMatrix44_t mat;
		float left, right, top, bottom;
		float centerRight;
		double f;
		settings->initialized = 1;
		hmd->GetWindowBounds(&settings->xPos,&settings->yPos,&settings->width,&settings->height);
		hmd->GetRecommendedRenderTargetSize(&settings->targetWidth,&settings->targetHeight);
		settings->scaleX = (float) settings->targetWidth / (float) settings->width;
		settings->scaleY = (float) settings->targetHeight / (float) settings->height;
		mat = hmd->GetEyeMatrix(vr::Eye_Right);
		settings->ipd = (float) SDL_fabs(mat.m[0][3]) * 2.0f;
		hmd->GetProjectionRaw(vr::Eye_Right,&left,&right,&top,&bottom);
		centerRight = right - left;
		centerRight /= 2.0;
		f = SDL_atan(bottom) * 2.0f;
		settings->aspect = centerRight / bottom;
		settings->viewFovY = (float) (f *180.0f / M_PI);
		settings->viewFovX = settings->viewFovY / settings->aspect;
		settings->projOffset = right - centerRight;
		hmd->GetDriverId(settings->deviceName,128);
		return 1;
	}
	return 0;
}

Sint32 SteamVR_GetDistortionTextures(eye_t eye, Uint32 width, Uint32 height, Uint32 normalBits, float *normal, float *chroma)
{
	if (!hmd)
		return 0;
	SDL_memset(normal,0,sizeof(float) * width * height * normalBits);
	SDL_memset(chroma,0,sizeof(float) * width * height * 4);

	for(Uint32 y = 0; y < height; y++ )
	{
		for(Uint32 x = 0; x < width; x++ )
		{
			int offset = normalBits * ( x + y * width );
			int chromaOffset = 4 * ( x + y * width );

			float u = ( (float)x + 0.5f) / (float)width;
			float v = ( (float)y + 0.5f) / (float)height;
			vr::DistortionCoordinates_t coords = hmd->ComputeDistortion( (vr::Hmd_Eye) eye, u, v );

			// Put red and green UVs into the texture. We'll estimate the green UV in the shader from
			// these two.

			chroma[chromaOffset + 0] = (float)( coords.rfRed[0] );
			chroma[chromaOffset + 1] = (float)( coords.rfRed[1] );
			chroma[chromaOffset + 2] = (float)( coords.rfBlue[0] );
			chroma[chromaOffset + 3] = (float)( coords.rfBlue[1] );

			normal[offset + 0] = (float)( coords.rfGreen[0] );
			normal[offset + 1] = (float)( coords.rfGreen[1] );

		}
	}
	return 1;
}


void SteamVR_GetEyeViewport(eye_t eye, Uint32 *xpos, Uint32 *ypos, Uint32 *width, Uint32 *height)
{
	if (hmd)
	{
		hmd->GetEyeOutputViewport((vr::Hmd_Eye) eye , vr::API_OpenGL, xpos, ypos, width, height);
	}
}

void SteamVR_ResetHMDOrientation(void)
{
	if (hmd)
	{
		hmd->ZeroTracker();
	}
}

void SteamVR_GetOrientationAndPosition(float prediction, float orientation[3], float position[3])
{
	for (int i = 0; i < 3; i++)
	{
		orientation[i] = 0;
		position[i] = 0;
	}

	if (hmd)
	{
		float psign = -1.0f;
		float a, b, c;
		float D = 1;
		float S = 1;
		float pM;
		vr::HmdMatrix34_t mat;
		vr::HmdTrackingResult result;
		hmd->GetWorldFromHeadPose(prediction,&mat, &result);


		pM = psign*mat.m[1][2];
		if (pM < -1.0f + 0.0000001f)
		{ // South pole singularity
			a = 0.0f;
			b = (float) (-S*D* M_PI * 0.5);
			c = (float) (S*D*SDL_atan2( psign*mat.m[0][1], mat.m[0][0] ));
		}
		else if (pM > 1.0f - 0.0000001f)
		{ // North pole singularity
			a = 0.0f;
			b = (float) (S*D* M_PI * 0.5);
			c = (float) (S*D*SDL_atan2( psign*mat.m[0][1], mat.m[0][0] ));
		}
		else
		{ // Normat.mal case (nonsingular)
			a = (float) (S*D*SDL_atan2( -psign*mat.m[0][2], mat.m[2][2] ));
			b = (float) (S*D*SDL_asin(pM));
			c = (float) (S*D*SDL_atan2( -psign*mat.m[1][0], mat.m[1][1] ));
		}

		orientation[0] = (float) (-b * 180.0f / M_PI);
		orientation[1] = (float) (a * 180.0f / M_PI);
		orientation[2] = (float) (-c * 180.0f / M_PI);
	}
}

#endif //NO_STEAM
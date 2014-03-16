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
		settings->initialized = 1;
		hmd->GetWindowBounds(&settings->xPos,&settings->yPos,&settings->width,&settings->height);
		hmd->GetRecommendedRenderTargetSize(&settings->targetWidth,&settings->targetHeight);
		settings->scaleX = (float) settings->targetWidth / (float) settings->width;
		settings->scaleY = (float) settings->targetHeight / (float) settings->height;
		hmd->GetDriverId(settings->deviceName,128);
		return 1;
	}
	return 0;
}

distcoords_t SteamVR_GetDistortionCoords(eye_t eye, float u, float v )
{
	distcoords_t output = { { 0, 0 }, {0, 0} , {0, 0}};
	if (hmd)
	{
		vr::DistortionCoordinates_t coords = hmd->ComputeDistortion((vr::Hmd_Eye) eye, u, v);	
		SDL_memcpy(&output,&coords,sizeof(output));
	} 
	return output;
};

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
			b = -S*D* M_PI * 0.5;
			c = S*D*SDL_atan2( psign*mat.m[0][1], mat.m[0][0] );
		}
		else if (pM > 1.0f - 0.0000001f)
		{ // North pole singularity
			a = 0.0f;
			b = S*D* M_PI * 0.5;
			c = S*D*SDL_atan2( psign*mat.m[0][1], mat.m[0][0] );
		}
		else
		{ // Normat.mal case (nonsingular)
			a = S*D*SDL_atan2( -psign*mat.m[0][2], mat.m[2][2] );
			b = S*D*SDL_asin(pM);
			c = S*D*SDL_atan2( -psign*mat.m[1][0], mat.m[1][1] );
		}

		orientation[0] = (float) (-b * 180 / M_PI);
		orientation[1] = (float) a * 180 / M_PI;
		orientation[2] = (float) (-c * 180 / M_PI);
	}
}

float SteamVR_GetIPD()
{
	if (hmd)
	{
		vr::HmdMatrix44_t mat;
		mat = hmd->GetEyeMatrix(vr::Eye_Right);
		return (float) SDL_fabs(mat.m[0][3]) * 2.0f;
	}
	return 0.0f;
}

void SteamVR_GetProjectionRaw(float *left, float *right, float *top, float *bottom)
{
	if (hmd)
	{
		hmd->GetProjectionRaw(vr::Eye_Right,left, right, top, bottom);
	}
}

#endif //NO_STEAM
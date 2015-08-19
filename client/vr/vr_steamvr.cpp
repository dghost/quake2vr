#ifndef NO_STEAM

#include "include/vr_steamvr.h"
#include <steam/steam_api.h>
#include <steam/steamvr.h>
#include <cstdlib>

static vr::IHmd *hmd;


int32_t SteamVR_Init()
{
	SteamAPI_Init();
	if (hmd)
	{
		vr::VR_Shutdown();
		hmd = NULL;
	}
	return !hmd;
}

int32_t SteamVR_Enable()
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
	SteamAPI_Shutdown();
}

int32_t SteamVR_GetSettings(svr_settings_t *settings)
{
	if (hmd)
	{
		vr::HmdMatrix34_t mat;
		float left, right, top, bottom;
		float centerRight;
		double f;
		settings->initialized = 1;
		hmd->GetWindowBounds(&settings->xPos,&settings->yPos,&settings->width,&settings->height);
		hmd->GetRecommendedRenderTargetSize(&settings->targetWidth,&settings->targetHeight);
		settings->scaleX = (float) settings->targetWidth / (float) settings->width;
		settings->scaleY = (float) settings->targetHeight / (float) settings->height;
		mat = hmd->GetHeadFromEyePose(vr::Eye_Right);
		settings->ipd = (float) SDL_fabs(mat.m[0][3]);
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

int32_t SteamVR_GetDistortionTextures(eye_t eye, uint32_t width, uint32_t height, uint32_t normalBits, float *normal, float *chroma)
{
	if (!hmd)
		return 0;
	SDL_memset(normal,0,sizeof(float) * width * height * normalBits);
	SDL_memset(chroma,0,sizeof(float) * width * height * 4);

	for(uint32_t y = 0; y < height; y++ )
	{
		for(uint32_t x = 0; x < width; x++ )
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


void SteamVR_GetEyeViewport(eye_t eye, uint32_t *xpos, uint32_t *ypos, uint32_t *width, uint32_t *height)
{
	if (hmd)
	{
		hmd->GetEyeOutputViewport((vr::Hmd_Eye) eye , xpos, ypos, width, height);
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
		float a, b, c;
		float pM;
		vr::HmdMatrix34_t mat;
		vr::HmdTrackingResult result;
		hmd->GetTrackerFromHeadPose (prediction,&mat, &result);

		pM = -mat.m[1][2];
		if (pM < -1.0f + 0.0000001f)
		{ // South pole singularity
			a = 0.0f;
			b = (float) (M_PI * -0.5);
			c = (float) SDL_atan2( -mat.m[0][1], mat.m[0][0] );
		}
		else if (pM > 1.0f - 0.0000001f)
		{ // North pole singularity
			a = 0.0f;
			b = (float) M_PI * 0.5;
			c = (float) SDL_atan2( -mat.m[0][1], mat.m[0][0] );
		}
		else
		{ 
			a = (float) SDL_atan2( mat.m[0][2], mat.m[2][2] );
			b = (float) SDL_asin(pM);
			c = (float) SDL_atan2( mat.m[1][0], mat.m[1][1] );
		}

		orientation[0] = (float) (-b * 180.0f / M_PI);
		orientation[1] = (float) (a * 180.0f / M_PI);
		orientation[2] = (float) (-c * 180.0f / M_PI);

		position[0] = mat.m[0][3];
		position[1] = mat.m[1][3];
		position[2] = mat.m[2][3];
	}

}

#endif //NO_STEAM
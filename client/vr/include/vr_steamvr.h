#ifndef __VR_STEAMVR_H
#define __VR_STEAMVR_H
#ifndef NO_STEAM

#include <SDL_stdinc.h>

typedef struct
{
	float rfRed[2];
	float rfGreen[2];
	float rfBlue[2];
} distcoords_t;

typedef enum SVR_Eye
{
	SVR_Left = 0,
	SVR_Right = 1
} eye_t;

typedef struct {
	Uint32 initialized;
	float ipd;
	float aspect;
	float viewFovY;
	float viewFovX;
	float projOffset;
	Sint32 xPos;
	Sint32 yPos;
	Uint32 width;
	Uint32 height;
	Uint32 targetWidth;
	Uint32 targetHeight;
	float scaleX;
	float scaleY;
	char deviceName[128];
} svr_settings_t;

#ifdef __cplusplus
extern "C" {
#endif

	Sint32 SteamVR_Init();
	Sint32 SteamVR_Enable();
	void SteamVR_Disable();
	void SteamVR_Shutdown();


	Sint32 SteamVR_GetSettings(svr_settings_t *settings);
	void SteamVR_GetEyeViewport(eye_t eye, Uint32 *xpos, Uint32 *ypos, Uint32 *width, Uint32 *height);
	void SteamVR_ResetHMDOrientation(void);
	void SteamVR_GetOrientationAndPosition(float prediction, float orientation[3], float position[3]);
	void SteamVR_GetProjectionRaw(float *left, float *right, float *top, float *bottom);
	Sint32 SteamVR_GetDistortionTextures(eye_t eye, Uint32 width, Uint32 height, Uint32 normalBits, float *normal, float *chroma);

#ifdef __cplusplus
}
#endif


#endif // NO_STEAM
#endif // __VR_STEAMVR_H
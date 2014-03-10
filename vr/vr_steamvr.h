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


#ifdef __cplusplus
extern "C" {
#endif

	Sint32 SteamVR_Init();
	Sint32 SteamVR_Enable();
	void SteamVR_Disable();
	void SteamVR_Shutdown();

#ifdef __cplusplus
}
#endif


#endif // NO_STEAM
#endif // __VR_STEAMVR_H
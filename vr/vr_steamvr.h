#ifndef __VR_STEAMVR_H
#define __VR_STEAMVR_H
#ifndef NO_STEAM

#ifdef __cplusplus
extern "C" {
#endif

	int SteamVR_Init();
	int SteamVR_Enable();
	void SteamVR_Disable();
	void SteamVR_Shutdown();

#ifdef __cplusplus
}
#endif


#endif // NO_STEAM
#endif // __VR_STEAMVR_H
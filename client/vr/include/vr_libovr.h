#ifndef __VR_LIBOVR_H
#define __VR_LIBOVR_H
#include <SDL_stdinc.h>

#ifdef __cplusplus
extern "C" {
#endif

	typedef struct {
		Uint32 initialized;
		Sint32 xPos;
		Sint32 yPos;
		Uint32 h_resolution;
		Uint32 v_resolution;
		float h_screen_size;
		float v_screen_size;
		float interpupillary_distance;
		float lens_separation_distance;
		float eye_to_screen_distance;
		float distortion_k[4];
		float chrom_abr[4];
		char deviceString[32];
		char deviceName[32];
	} ovr_settings_t;

	Sint32 LibOVR_Init(void);
	Sint32 LibOVR_DeviceInit(void);
	void LibOVR_DeviceRelease(void);
	void LibOVR_Shutdown(void);
	Sint32 LibOVR_GetOrientation(float euler[3]);
	Sint32 LibOVR_GetSettings(ovr_settings_t *settings);
	Sint32 LibOVR_SetPredictionTime(float time);
	Sint32 LibOVR_EnableDriftCorrection(void);
	void LibOVR_DisableDriftCorrection(void);
	Sint32 LibOVR_IsDriftCorrectionEnabled(void);
	void LibOVR_ResetHMDOrientation(void);
	Sint32 LibOVR_IsLatencyTesterAvailable(void);
	void LibOVR_ProcessLatencyInputs(void);
	Sint32 LibOVR_GetLatencyTestColor(float color[4]);
	const char* LibOVR_ProcessLatencyResults();

#ifdef __cplusplus
}
#endif

#endif
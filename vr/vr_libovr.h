#ifndef __VR_LIBOVR_H
#define __VR_LIBOVR_H


#ifdef __cplusplus
extern "C" {
#endif

	typedef struct {
		unsigned int initialized;
		unsigned int h_resolution;
		unsigned int v_resolution;
		unsigned int xPos;
		unsigned int yPos;
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

	int LibOVR_Init(void);
	int LibOVR_IsHMDAvailable(void);
	int LibOVR_DeviceInit(void);
	void LibOVR_DeviceRelease(void);
	void LibOVR_Shutdown(void);
	int LibOVR_GetOrientation(float euler[3]);
	int LibOVR_GetSettings(ovr_settings_t *settings);
	int LibOVR_SetPredictionTime(float time);
	int LibOVR_EnableDriftCorrection(void);
	void LibOVR_DisableDriftCorrection(void);
	int LibOVR_IsDriftCorrectionEnabled(void);
	void LibOVR_ResetHMDOrientation(void);
	int LibOVR_IsLatencyTesterAvailable(void);
	void LibOVR_ProcessLatencyInputs(void);
	int LibOVR_GetLatencyTestColor(float color[4]);
	const char* LibOVR_ProcessLatencyResults();

#ifdef __cplusplus
}
#endif

#endif
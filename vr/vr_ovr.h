#ifndef __VR_OVR_H
#define __VR_OVR_H


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
	} vr_ovr_settings_t;

	int VR_OVR_Init(void);
	void VR_OVR_Shutdown(void);
	int VR_OVR_GetOrientation(float euler[3]);
	int VR_OVR_GetSettings(vr_ovr_settings_t *settings);
	int VR_OVR_SetPredictionTime(float time);
	int VR_OVR_EnableMagneticCorrection();
	void VR_OVR_DisableMagneticCorrection();
	void VR_OVR_ResetHMDOrientation();
#ifdef __cplusplus
}
#endif

#endif
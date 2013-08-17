#include "vr_ovr.h"
#include "OVR.h"
#include <math.h>

static OVR::DeviceManager *devMgr = NULL;
static OVR::HMDDevice *hmdDev = NULL;
static OVR::SensorDevice *sensDev = NULL;
static OVR::SensorFusion *sensFus = NULL;
static OVR::Util::MagCalibration *magCal = NULL;
static OVR::HMDInfo hmdInfo;
static float lastOrientation[3];

#define M_PI 3.14159265358979323846f

int VR_OVR_Init() {
	OVR::System::Init(OVR::Log::ConfigureDefaultLog(OVR::LogMask_All));
	devMgr = OVR::DeviceManager::Create();
	hmdDev = devMgr->EnumerateDevices<OVR::HMDDevice>().CreateDevice();
	if (!hmdDev)
	{
		devMgr->Release();
		devMgr = NULL;
		return 0;
	}
	
	sensDev = hmdDev->GetSensor();
	if (!sensDev)
	{
		devMgr->Release();
		devMgr = NULL;
		hmdDev->Release();
		hmdDev = NULL;
		return 0;
	}
	sensFus = new OVR::SensorFusion();
	sensFus->AttachToSensor(sensDev);
	sensFus->SetYawCorrectionEnabled(false);
	sensFus->SetPrediction(0.0,false);
	return 1;
}

void VR_OVR_Shutdown() {
	if (devMgr) {
		devMgr->Release();
		devMgr = NULL;
	}

	if (hmdDev) {
		hmdDev->Release();
		hmdDev = NULL;
	}

	if (sensDev) {
		sensDev->Release();
		sensDev = NULL;
	}

	if (sensFus) {
		delete sensFus;
		sensFus = NULL;
	}
	
	if (magCal) {
		delete magCal;
		magCal = NULL;
	}
}

void VR_OVR_ResetHMDOrientation()
{
	if (!sensFus)
		return;

	sensFus->Reset();

	if(magCal && magCal->IsCalibrated())
		magCal->BeginAutoCalibration(*sensFus);
}

int VR_OVR_GetOrientation(float euler[3])
{
	if (!sensFus)
		return 0;

	if (magCal && magCal->IsAutoCalibrating()) {
		magCal->UpdateAutoCalibration(*sensFus);
	}

	// GetPredictedOrientation() works even if prediction is disabled
	OVR::Quatf q = sensFus->GetPredictedOrientation();
	
	q.GetEulerAngles<OVR::Axis_Y, OVR::Axis_X, OVR::Axis_Z>(&euler[1], &euler[0], &euler[2]);

	euler[0] = (-euler[0] * 180.0f) / M_PI;
	euler[1] = (euler[1] * 180.0f) / M_PI;
	euler[2] = (-euler[2] * 180.0f) / M_PI;
	return 1;
}

int VR_OVR_GetSettings(vr_ovr_settings_t *settings) {
	if (!hmdDev || !hmdDev->GetDeviceInfo(&hmdInfo))
		return 0;
	settings->initialized = 1;
	settings->h_resolution = hmdInfo.HResolution;
	settings->v_resolution = hmdInfo.VResolution;
	settings->h_screen_size = hmdInfo.HScreenSize;
	settings->v_screen_size = hmdInfo.VScreenSize;

	settings->interpupillary_distance = hmdInfo.InterpupillaryDistance;
	settings->lens_separation_distance = hmdInfo.LensSeparationDistance;
	settings->eye_to_screen_distance = hmdInfo.EyeToScreenDistance;

	memcpy(settings->distortion_k, hmdInfo.DistortionK, sizeof(float) * 4);
	memcpy(settings->chrom_abr, hmdInfo.ChromaAbCorrection, sizeof(float) * 4);
	strncpy(settings->devName, hmdInfo.ProductName,31);
	return 1;
}

int VR_OVR_SetPredictionTime(float time) {
	if (!sensFus)
		return 0;

	if (time > 0.0f)
		sensFus->SetPrediction(time, true);
	else
		sensFus->SetPrediction(0.0,false);
	return 1;
}

int VR_OVR_EnableMagneticCorrection() {
	if (!sensFus)
		return 0;
	
	sensFus->SetYawCorrectionEnabled(true);
	if (!sensFus->HasMagCalibration())
	{
		if (!magCal)
			magCal = new OVR::Util::MagCalibration();

		magCal->BeginAutoCalibration(*sensFus);
	}


	return 1;
}

void VR_OVR_DisableMagneticCorrection() {
	if (!sensFus)
		return;

	sensFus->SetYawCorrectionEnabled(false);
	if (magCal)
	{
		delete magCal;
		magCal = NULL;
	}
}
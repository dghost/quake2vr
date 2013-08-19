#include "vr_ovr.h"
#include "OVR.h"
#include <math.h>

static bool device_available;
static OVR::DeviceManager *manager = NULL;
static OVR::HMDDevice *hmd = NULL;
static OVR::SensorDevice *sensor = NULL;
static OVR::SensorFusion *fusion = NULL;
static OVR::Util::MagCalibration *magnet = NULL;
static OVR::HMDInfo hmdInfo;

static bool initialized = false;

#define M_PI 3.14159265358979323846f

int VR_OVR_Init() {
	if (!initialized)
	{
		OVR::System::Init(OVR::Log::ConfigureDefaultLog(OVR::LogMask_All));

		initialized = true;
	}

	if (!manager)
		manager = OVR::DeviceManager::Create();
	
	if (manager && !hmd)
		hmd = manager->EnumerateDevices<OVR::HMDDevice>().CreateDevice();
	

	if (hmd && !sensor)
		sensor = hmd->GetSensor();

	if (!sensor)
	{
		VR_OVR_Shutdown();
		return 0;
	}

	fusion = new OVR::SensorFusion();
	fusion->AttachToSensor(sensor);
	fusion->SetYawCorrectionEnabled(false);
	fusion->SetPrediction(0.0,false);
	return 1;
}

void VR_OVR_Shutdown() {
	initialized = false;
	if (manager) {
		manager->Release();
		manager = NULL;
	}

	if (hmd) {
		hmd->Release();
		hmd = NULL;
	}

	if (sensor) {
		sensor->Release();
		sensor = NULL;
	}

	if (fusion) {
		delete fusion;
		fusion = NULL;
	}
	
	if (magnet) {
		delete magnet;
		magnet = NULL;
	}
}

void VR_OVR_ResetHMDOrientation()
{
	if (!fusion)
		return;

	fusion->Reset();

	if(magnet && magnet->IsCalibrated())
		magnet->BeginAutoCalibration(*fusion);
}

int VR_OVR_GetOrientation(float euler[3])
{
	if (!fusion)
		return 0;

	if (magnet && magnet->IsAutoCalibrating()) {
		magnet->UpdateAutoCalibration(*fusion);
	}

	// GetPredictedOrientation() works even if prediction is disabled
	OVR::Quatf q = fusion->GetPredictedOrientation();
	
	q.GetEulerAngles<OVR::Axis_Y, OVR::Axis_X, OVR::Axis_Z>(&euler[1], &euler[0], &euler[2]);

	euler[0] = (-euler[0] * 180.0f) / M_PI;
	euler[1] = (euler[1] * 180.0f) / M_PI;
	euler[2] = (-euler[2] * 180.0f) / M_PI;
	return 1;
}

int VR_OVR_GetSettings(vr_ovr_settings_t *settings) {
	if (!hmd || !hmd->GetDeviceInfo(&hmdInfo))
		return 0;
	settings->initialized = 1;
	settings->h_resolution = hmdInfo.HResolution;
	settings->v_resolution = hmdInfo.VResolution;
	settings->h_screen_size = hmdInfo.HScreenSize;
	settings->v_screen_size = hmdInfo.VScreenSize;
	settings->xPos = hmdInfo.DesktopX;
	settings->yPos = hmdInfo.DesktopY;
	settings->interpupillary_distance = hmdInfo.InterpupillaryDistance;
	settings->lens_separation_distance = hmdInfo.LensSeparationDistance;
	settings->eye_to_screen_distance = hmdInfo.EyeToScreenDistance;
	memcpy(settings->distortion_k, hmdInfo.DistortionK, sizeof(float) * 4);
	memcpy(settings->chrom_abr, hmdInfo.ChromaAbCorrection, sizeof(float) * 4);
	strncpy(settings->deviceString, hmdInfo.ProductName,31);
	strncpy(settings->deviceName,hmdInfo.DisplayDeviceName,31);
	return 1;
}

int VR_OVR_SetPredictionTime(float time) {
	if (!fusion)
		return 0;

	if (time > 0.0f)
		fusion->SetPrediction(time, true);
	else
		fusion->SetPrediction(0.0,false);
	return 1;
}

int VR_OVR_EnableMagneticCorrection() {
	if (!fusion)
		return 0;
	
	fusion->SetYawCorrectionEnabled(true);
	if (!fusion->HasMagCalibration())
	{
		if (!magnet)
			magnet = new OVR::Util::MagCalibration();

		magnet->BeginAutoCalibration(*fusion);
	}


	return 1;
}

void VR_OVR_DisableMagneticCorrection() {
	if (!fusion)
		return;

	fusion->SetYawCorrectionEnabled(false);
	if (magnet)
	{
		delete magnet;
		magnet = NULL;
	}
}
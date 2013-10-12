#include "vr_libovr.h"
#include "OVR.h"
#include <math.h>

static bool device_available;
static OVR::DeviceManager *manager = NULL;
static OVR::HMDDevice *hmd = NULL;
static OVR::SensorDevice *sensor = NULL;
static OVR::SensorFusion *fusion = NULL;
static OVR::HMDInfo hmdInfo;

static bool initialized = false;

#define M_PI 3.14159265358979323846f

int LibOVR_Init()
{
	if (!initialized)
	{
		OVR::System::Init(OVR::Log::ConfigureDefaultLog(OVR::LogMask_All));
		manager = OVR::DeviceManager::Create();
		if (!manager)
			return 0;
		initialized = true;
	}
	return 1;	
}

int LibOVR_IsDeviceAvailable()
{
	if (!LibOVR_Init)
		return 0;

	if (!hmd)
	{
		OVR::HMDDevice *test = manager->EnumerateDevices<OVR::HMDDevice>().CreateDevice();
		if (!test)
			return 0;
		OVR::SensorDevice *fusion = test->GetSensor();
		if (!fusion)
		{
			test->Release();
			return 0;
		}
		fusion->Release();
		test->Release();
		return 1;
	}
	return 1;
}

void LibOVR_DeviceRelease() {
	if (!initialized)
		return;

	
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
	
}

int LibOVR_DeviceInit() {
	if (!initialized)
		return 0;

	if (manager && !hmd)
		hmd = manager->EnumerateDevices<OVR::HMDDevice>().CreateDevice();
	

	if (hmd && !sensor)
		sensor = hmd->GetSensor();

	if (!sensor || !hmd)
	{
		LibOVR_DeviceRelease();
		return 0;
	}

	fusion = new OVR::SensorFusion();
	fusion->AttachToSensor(sensor);
	fusion->SetYawCorrectionEnabled(false);
	fusion->SetPrediction(0.0,false);
	return 1;
}

void LibOVR_Shutdown() {

	LibOVR_DeviceRelease();

	initialized = false;
	if (manager) {
		manager->Release();
		manager = NULL;
	}
}

void LibOVR_ResetHMDOrientation()
{
	if (!fusion)
		return;

	fusion->Reset();
}

int LibOVR_GetOrientation(float euler[3])
{
	if (!fusion)
		return 0;

	// GetPredictedOrientation() works even if prediction is disabled
	OVR::Quatf q = fusion->GetPredictedOrientation();
	
	q.GetEulerAngles<OVR::Axis_Y, OVR::Axis_X, OVR::Axis_Z>(&euler[1], &euler[0], &euler[2]);

	euler[0] = (-euler[0] * 180.0f) / M_PI;
	euler[1] = (euler[1] * 180.0f) / M_PI;
	euler[2] = (-euler[2] * 180.0f) / M_PI;
	return 1;
}

int LibOVR_GetSettings(ovr_settings_t *settings) {
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

int LibOVR_SetPredictionTime(float time) {
	float timeInSec = time / 1000.0f;
	if (!fusion)
		return 0;

	if (time > 0.0f)
		fusion->SetPrediction(timeInSec, true);
	else
		fusion->SetPrediction(0.0f,false);
	return 1;
}

int LibOVR_IsDriftCorrectionEnabled() {
	
	return (fusion && fusion->IsYawCorrectionEnabled()) ? 1 : 0;
}


int LibOVR_EnableDriftCorrection() {
	if (!fusion)
		return 0;
	
	if (!fusion->HasMagCalibration())
		return 0;

	fusion->SetYawCorrectionEnabled(true);
	return 1;
}

void LibOVR_DisableDriftCorrection() {
	if (!fusion)
		return;

	fusion->SetYawCorrectionEnabled(false);
}
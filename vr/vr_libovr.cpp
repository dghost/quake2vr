#include "vr_libovr.h"
#include "OVR.h"
#include <math.h>

static bool device_available;
static OVR::DeviceManager *manager = NULL;
static OVR::HMDDevice *hmd = NULL;
static OVR::SensorDevice *sensor = NULL;
static OVR::SensorFusion *fusion = NULL;
static OVR::LatencyTestDevice *latencyTester = NULL;
static OVR::Util::LatencyTest *latencyUtil = NULL;

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
	return (manager != NULL);	
}

int LibOVR_InitHMD()
{
	if (hmd)
		return 1;

	if (!LibOVR_Init())
		return 0;

	if (manager)
		hmd = manager->EnumerateDevices<OVR::HMDDevice>().CreateDevice();
	
	return (hmd != NULL);
}

int LibOVR_InitSensor() {
	if (fusion)
		return 1;

	if (!LibOVR_InitHMD())
		return 0;


	if (!sensor)
		sensor = hmd->GetSensor();

	if (sensor)
	{
		fusion = new OVR::SensorFusion();
		fusion->AttachToSensor(sensor);
		fusion->SetYawCorrectionEnabled(false);
		fusion->SetPrediction(0.0,false);
	}
	return (sensor != NULL && fusion != NULL);
}

int LibOVR_InitLatencyTest() {
	if (latencyUtil)
		return 1;

	if (!LibOVR_Init())
		return 0;
	
	
	if (!latencyTester)
		latencyTester = manager->EnumerateDevices<OVR::LatencyTestDevice>().CreateDevice();

	if (latencyTester)
	{
		latencyUtil = new OVR::Util::LatencyTest;
		latencyUtil->SetDevice(latencyTester);
	}

	return (latencyTester != NULL && latencyUtil != NULL);
}
int LibOVR_IsDeviceAvailable()
{
	return (LibOVR_InitHMD());
}

void LibOVR_DeviceRelease() {
	if (!initialized)
		return;

	if (latencyUtil)
	{
		delete latencyUtil;
		latencyUtil = NULL;
	}

	if (latencyTester)
	{
		latencyTester->Release();
		latencyTester=NULL;
	}
	
	if (hmd) {
		hmd->Release();
		hmd = NULL;
	}

	if (fusion) {
		delete fusion;
		fusion = NULL;
	}

	if (sensor) {
		sensor->Release();
		sensor = NULL;
	}


	
}

int LibOVR_DeviceInit() {
	return (LibOVR_InitHMD());
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
	if (!LibOVR_InitSensor())
		return;

	fusion->Reset();
}

int LibOVR_GetOrientation(float euler[3])
{
	if (!LibOVR_InitSensor())
		return 0;


	// GetPredictedOrientation() works even if prediction is disabled
	OVR::Quatf q = fusion->GetPredictedOrientation();
	
	q.GetEulerAngles<OVR::Axis_Y, OVR::Axis_X, OVR::Axis_Z>(&euler[1], &euler[0], &euler[2]);

	euler[0] = (-euler[0] * 180.0f) / M_PI;
	euler[1] = (euler[1] * 180.0f) / M_PI;
	euler[2] = (-euler[2] * 180.0f) / M_PI;
	return 1;
}

int LibOVR_GetLatencyTestColor(float color[4])
{
	OVR::Color colorOVR;
	if (latencyUtil && latencyUtil->DisplayScreenColor(colorOVR))
	{
		colorOVR.GetRGBA(&color[0],&color[1],&color[2],&color[3]);
		return 1;
	}
	return 0;
}

void LibOVR_ProcessLatencyInputs()
{
	if (LibOVR_InitLatencyTest())
		latencyUtil->ProcessInputs();

}

const char* LibOVR_ProcessLatencyResults()
{
	if (!latencyUtil)
		return 0;
	return latencyUtil->GetResultsString();
}

int LibOVR_GetSettings(ovr_settings_t *settings) {
	OVR::HMDInfo hmdInfo;
	if (!LibOVR_InitHMD() || !hmd->GetDeviceInfo(&hmdInfo))
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
	if (!LibOVR_InitSensor())
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
	if (!LibOVR_InitSensor())
		return 0;
	
	if (!fusion->HasMagCalibration())
		return 0;

	fusion->SetYawCorrectionEnabled(true);
	return 1;
}

void LibOVR_DisableDriftCorrection() {
	if (!LibOVR_InitSensor())
		return;

	fusion->SetYawCorrectionEnabled(false);
}
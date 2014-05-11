/*
 * Copyright (C) 2012 Yamagi Burmeister
 * Copyright (C) 2010 skuller.net
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
 * USA.
 *
 * =======================================================================
 *
 * Low level, platform depended "qal" API implementation. This files
 * provides functions to load, initialize, shutdown und unload the
 * OpenAL library and connects the "qal" funtion pointers to the
 * OpenAL functions. It shopuld work on Windows and unixoid Systems,
 * other platforms may need an own implementation. This source file
 * was taken from Q2Pro and modified by the YQ2 authors.
 *
 * =======================================================================
 *
 * This was taken from YQ2 and modified for Q2VR by it's authors
 *
 */

#ifdef USE_OPENAL

#define DEFAULT_OPENAL_DRIVER "openal-mob"

#ifdef _WIN32
#define DEFAULT_LIBRARY_EXTENSION ".dll"
#else
#define DEFAULT_LIBRARY_EXTENSION ".so"
#endif


#include "include/AL/al.h"
#include "include/AL/alc.h"
#include "include/AL/alext.h"
#include "include/alConfigMob.h"

#include "../../qcommon/qcommon.h"
#include "include/local.h"
#include "include/qal.h"

#include <SDL.h>

static ALCcontext *context;
static ALCdevice *device;
static cvar_t *al_device;
static cvar_t *al_driver;
static void *handle;

const MOB_ConfigKeyValue g_soundConfig[] =
{
#ifdef _WIN32
	// The default sound output on Windows can't be forced to 44.1 KHz. Outputting at 44.1 KHz is essential to support HRTF, so adding this is on Windows is a good idea
	{ MOB_ConfigKey_root_drivers, "dsound" },
#endif // #if _WIN32
	// If you want to use HRTFs, you should be outputting to Stereo sound
	{ MOB_ConfigKey_root_channels, "stereo" },
	{ MOB_ConfigKey_root_hrtf, (const char*) 0 }, // This is a union, and const char * is the first type, so we have to cast it.
	{ MOB_ConfigKey_NULL, 0 }, // This is the terminator for the config array
};

/* Function pointers for OpenAL management */
static LPALCCREATECONTEXT qalcCreateContext;
static LPALCMAKECONTEXTCURRENT qalcMakeContextCurrent;
static LPALCPROCESSCONTEXT qalcProcessContext;
static LPALCSUSPENDCONTEXT qalcSuspendContext;
static LPALCDESTROYCONTEXT qalcDestroyContext;
static LPALCGETCURRENTCONTEXT qalcGetCurrentContext;
static LPALCGETCONTEXTSDEVICE qalcGetContextsDevice;
static LPALCOPENDEVICE qalcOpenDevice;
static LPALCCLOSEDEVICE qalcCloseDevice;
static LPALCGETERROR qalcGetError;
static LPALCISEXTENSIONPRESENT qalcIsExtensionPresent;
static LPALCGETPROCADDRESS qalcGetProcAddress;
static LPALCGETENUMVALUE qalcGetEnumValue;
static LPALCGETSTRING qalcGetString;
static LPALCGETINTEGERV qalcGetIntegerv;
static LPALCCAPTUREOPENDEVICE qalcCaptureOpenDevice;
static LPALCCAPTURECLOSEDEVICE qalcCaptureCloseDevice;
static LPALCCAPTURESTART qalcCaptureStart;
static LPALCCAPTURESTOP qalcCaptureStop;
static LPALCCAPTURESAMPLES qalcCaptureSamples ;

/* Declaration of function pointers used
   to connect OpenAL to our internal API */
LPALENABLE qalEnable;
LPALDISABLE qalDisable;
LPALISENABLED qalIsEnabled;
LPALGETSTRING qalGetString;
LPALGETBOOLEANV qalGetBooleanv;
LPALGETINTEGERV qalGetIntegerv;
LPALGETFLOATV qalGetFloatv;
LPALGETDOUBLEV qalGetDoublev;
LPALGETBOOLEAN qalGetBoolean;
LPALGETINTEGER qalGetInteger;
LPALGETFLOAT qalGetFloat;
LPALGETDOUBLE qalGetDouble;
LPALGETERROR qalGetError;
LPALISEXTENSIONPRESENT qalIsExtensionPresent;
LPALGETPROCADDRESS qalGetProcAddress;
LPALGETENUMVALUE qalGetEnumValue;
LPALLISTENERF qalListenerf;
LPALLISTENER3F qalListener3f;
LPALLISTENERFV qalListenerfv;
LPALLISTENERI qalListeneri;
LPALLISTENER3I qalListener3i;
LPALLISTENERIV qalListeneriv;
LPALGETLISTENERF qalGetListenerf;
LPALGETLISTENER3F qalGetListener3f;
LPALGETLISTENERFV qalGetListenerfv;
LPALGETLISTENERI qalGetListeneri;
LPALGETLISTENER3I qalGetListener3i;
LPALGETLISTENERIV qalGetListeneriv;
LPALGENSOURCES qalGenSources;
LPALDELETESOURCES qalDeleteSources;
LPALISSOURCE qalIsSource;
LPALSOURCEF qalSourcef;
LPALSOURCE3F qalSource3f;
LPALSOURCEFV qalSourcefv;
LPALSOURCEI qalSourcei;
LPALSOURCE3I qalSource3i;
LPALSOURCEIV qalSourceiv;
LPALGETSOURCEF qalGetSourcef;
LPALGETSOURCE3F qalGetSource3f;
LPALGETSOURCEFV qalGetSourcefv;
LPALGETSOURCEI qalGetSourcei;
LPALGETSOURCE3I qalGetSource3i;
LPALGETSOURCEIV qalGetSourceiv;
LPALSOURCEPLAYV qalSourcePlayv;
LPALSOURCESTOPV qalSourceStopv;
LPALSOURCEREWINDV qalSourceRewindv;
LPALSOURCEPAUSEV qalSourcePausev;
LPALSOURCEPLAY qalSourcePlay;
LPALSOURCESTOP qalSourceStop;
LPALSOURCEREWIND qalSourceRewind;
LPALSOURCEPAUSE qalSourcePause;
LPALSOURCEQUEUEBUFFERS qalSourceQueueBuffers;
LPALSOURCEUNQUEUEBUFFERS qalSourceUnqueueBuffers;
LPALGENBUFFERS qalGenBuffers;
LPALDELETEBUFFERS qalDeleteBuffers;
LPALISBUFFER qalIsBuffer;
LPALBUFFERDATA qalBufferData;
LPALBUFFERF qalBufferf;
LPALBUFFER3F qalBuffer3f;
LPALBUFFERFV qalBufferfv;
LPALBUFFERI qalBufferi;
LPALBUFFER3I qalBuffer3i;
LPALBUFFERIV qalBufferiv;
LPALGETBUFFERF qalGetBufferf;
LPALGETBUFFER3F qalGetBuffer3f;
LPALGETBUFFERFV qalGetBufferfv;
LPALGETBUFFERI qalGetBufferi;
LPALGETBUFFER3I qalGetBuffer3i;
LPALGETBUFFERIV qalGetBufferiv;
LPALDOPPLERFACTOR qalDopplerFactor;
LPALDOPPLERVELOCITY qalDopplerVelocity;
LPALSPEEDOFSOUND qalSpeedOfSound;
LPALDISTANCEMODEL qalDistanceModel;
LPALGENFILTERS qalGenFilters;
LPALFILTERI qalFilteri;
LPALFILTERF qalFilterf;
LPALDELETEFILTERS qalDeleteFilters;
LPALCDEVICEENABLEHRTFMOB qalcDeviceEnableHrtfMOB;
LPALSETCONFIGMOB qalSetConfigMOB;

/*
 * Gives information over the OpenAL
 * implementation and it's state
 */
void QAL_SoundInfo()
{
	Com_Printf("OpenAL settings:\n");
    Com_Printf("AL_VENDOR: %s\n", qalGetString(AL_VENDOR));
    Com_Printf("AL_RENDERER: %s\n", qalGetString(AL_RENDERER));
    Com_Printf("AL_VERSION: %s\n", qalGetString(AL_VERSION));
    Com_Printf("AL_EXTENSIONS: %s\n", qalGetString(AL_EXTENSIONS));

	if (qalcIsExtensionPresent(NULL, "ALC_ENUMERATE_ALL_EXT"))
	{
		const char *devs = qalcGetString(NULL, ALC_ALL_DEVICES_SPECIFIER);

		Com_Printf("\nAvailable OpenAL devices:\n");

		if (devs == NULL)
		{
			Com_Printf("- No devices found. Depending on your\n");
			Com_Printf("  platform this may be expected and\n");
			Com_Printf("  doesn't indicate a problem!\n");
		}
		else
		{
			while (devs && *devs)
			{
				Com_Printf("- %s\n", devs);
				devs += strlen(devs) + 1;
			}
		}
	}

   	if (qalcIsExtensionPresent(NULL, "ALC_ENUMERATE_ALL_EXT"))
	{
		const char *devs = qalcGetString(device, ALC_DEVICE_SPECIFIER);

		Com_Printf("\nCurrent OpenAL device:\n");

		if (devs == NULL)
		{
			Com_Printf("- No OpenAL device in use\n");
		}
		else
		{
			Com_Printf("- %s\n", devs);
		}
	}
	{
		ALCint attr_size;
		ALCint * attributes;
		int i = 0;
		qalcGetIntegerv(device, ALC_ATTRIBUTES_SIZE, sizeof(attr_size), &attr_size);
		attributes = (ALCint *) malloc(attr_size * sizeof(ALCint));
		qalcGetIntegerv(device, ALC_ALL_ATTRIBUTES, attr_size, attributes);
		for (i = 0; i < attr_size; i += 2)
		{
			if (attributes[i] == ALC_FREQUENCY)
				Com_Printf("ALC_FREQUENCY: %i\n", attributes[i + 1]);
		}
		free(attributes);
	}

}

void QAL_SetHRTF(qboolean enable)
{
	if (qalcDeviceEnableHrtfMOB)
	{
		ALboolean result = AL_FALSE;
		ALboolean mode = (enable ? AL_TRUE : AL_FALSE);
		result = qalcDeviceEnableHrtfMOB(device, mode);
		if (result = !AL_TRUE)
			Com_Printf("Error: cannot set HRTF mode\n");
//		else
//			Com_Printf("Successfully %s HRTF mode\n", (enable ? "enabled" : "disabled"));
	}
	else {
		Com_Printf("Error: Setting HRTF mode is not supported by this OpenAL driver\n");
	}
}
/*
 * Shuts OpenAL down, frees all context and
 * device handles and unloads the shared lib.
 */
void
QAL_Shutdown()
{
    if (context)
   	{
        qalcMakeContextCurrent( NULL );
        qalcDestroyContext( context );
        context = NULL;
    }

	if (device)
	{
        qalcCloseDevice( device );
        device = NULL;
    }

	/* Disconnect function pointers used
	   for OpenAL management calls */
	qalcCreateContext = NULL;
	qalcMakeContextCurrent = NULL;
	qalcProcessContext = NULL;
	qalcSuspendContext = NULL;
	qalcDestroyContext = NULL;
	qalcGetCurrentContext = NULL;
	qalcGetContextsDevice = NULL;
	qalcOpenDevice = NULL;
	qalcCloseDevice = NULL;
	qalcGetError = NULL;
	qalcIsExtensionPresent = NULL;
	qalcGetProcAddress = NULL;
	qalcGetEnumValue = NULL;
	qalcGetString = NULL;
	qalcGetIntegerv = NULL;
	qalcCaptureOpenDevice = NULL;
	qalcCaptureCloseDevice = NULL;
	qalcCaptureStart = NULL;
	qalcCaptureStop = NULL;
	qalcCaptureSamples = NULL;

	/* Disconnect OpenAL
	 * function pointers */
	qalEnable = NULL;
	qalDisable = NULL;
	qalIsEnabled = NULL;
	qalGetString = NULL;
	qalGetBooleanv = NULL;
	qalGetIntegerv = NULL;
	qalGetFloatv = NULL;
	qalGetDoublev = NULL;
	qalGetBoolean = NULL;
	qalGetInteger = NULL;
	qalGetFloat = NULL;
	qalGetDouble = NULL;
	qalGetError = NULL;
	qalIsExtensionPresent = NULL;
	qalGetProcAddress = NULL;
	qalGetEnumValue = NULL;
	qalListenerf = NULL;
	qalListener3f = NULL;
	qalListenerfv = NULL;
	qalListeneri = NULL;
	qalListener3i = NULL;
	qalListeneriv = NULL;
	qalGetListenerf = NULL;
	qalGetListener3f = NULL;
	qalGetListenerfv = NULL;
	qalGetListeneri = NULL;
	qalGetListener3i = NULL;
	qalGetListeneriv = NULL;
	qalGenSources = NULL;
	qalDeleteSources = NULL;
	qalIsSource = NULL;
	qalSourcef = NULL;
	qalSource3f = NULL;
	qalSourcefv = NULL;
	qalSourcei = NULL;
	qalSource3i = NULL;
	qalSourceiv = NULL;
	qalGetSourcef = NULL;
	qalGetSource3f = NULL;
	qalGetSourcefv = NULL;
	qalGetSourcei = NULL;
	qalGetSource3i = NULL;
	qalGetSourceiv = NULL;
	qalSourcePlayv = NULL;
	qalSourceStopv = NULL;
	qalSourceRewindv = NULL;
	qalSourcePausev = NULL;
	qalSourcePlay = NULL;
	qalSourceStop = NULL;
	qalSourceRewind = NULL;
	qalSourcePause = NULL;
	qalSourceQueueBuffers = NULL;
	qalSourceUnqueueBuffers = NULL;
	qalGenBuffers = NULL;
	qalDeleteBuffers = NULL;
	qalIsBuffer = NULL;
	qalBufferData = NULL;
	qalBufferf = NULL;
	qalBuffer3f = NULL;
	qalBufferfv = NULL;
	qalBufferi = NULL;
	qalBuffer3i = NULL;
	qalBufferiv = NULL;
	qalGetBufferf = NULL;
	qalGetBuffer3f = NULL;
	qalGetBufferfv = NULL;
	qalGetBufferi = NULL;
	qalGetBuffer3i = NULL;
	qalGetBufferiv = NULL;
	qalDopplerFactor = NULL;
	qalDopplerVelocity = NULL;
	qalSpeedOfSound = NULL;
	qalDistanceModel = NULL;
	qalGenFilters = NULL;
	qalFilteri = NULL;
	qalFilterf = NULL;
	qalDeleteFilters = NULL;

	// OpenAL-MOB specific functions
	qalSetConfigMOB = NULL;
	qalcDeviceEnableHrtfMOB = NULL;

	/* Unload the shared lib */
	SDL_UnloadObject(handle);
    handle = NULL;
}

/*
 * Loads the OpenAL shared lib, creates
 * a context and device handle.
 */
qboolean
QAL_Init()
{
	char *libraries[] = {DEFAULT_OPENAL_DRIVER,"openal32",0};
	char name[256];
	char buffer[300];
	int i = 0;
	int sndfreq = (Cvar_Get("s_khz", "44", CVAR_ARCHIVE))->value;

	if (sndfreq == 48)
	{
		sndfreq = 48000;
	}
	else if (sndfreq == 44)
	{
		sndfreq = 44100;
	}
	else if (sndfreq == 22)
	{
		sndfreq = 22050;
	}
	else if (sndfreq == 11)
	{
		sndfreq = 11025;
	}


	/* DEFAULT_OPENAL_DRIVER is defined at compile time via the compiler */
	al_driver = Cvar_Get("al_driver", DEFAULT_OPENAL_DRIVER, CVAR_ARCHIVE);
	al_device = Cvar_Get("al_device", "", CVAR_ARCHIVE);

		
	strncpy(name, al_driver->string, sizeof(name));
	strncpy(buffer, name, sizeof(buffer));
	strncat(buffer, DEFAULT_LIBRARY_EXTENSION, sizeof(buffer) -strlen(name));
	Com_Printf("LoadLibrary(%s)\n", buffer);

	/* Load the library */
	handle = SDL_LoadObject(buffer);
	
	// prevent the user from screwing themselves by setting an invalid library
	for (i = 0; !handle && libraries[i] != NULL ; i++)
	{
		strncpy(name, libraries[i], sizeof(name));
		strncpy(buffer, name, sizeof(buffer));
		strncat(buffer, DEFAULT_LIBRARY_EXTENSION, sizeof(buffer) -strlen(name));
		Com_Printf("LoadLibrary(%s)\n", buffer);
		handle = SDL_LoadObject(buffer);
	}

	if (!handle)
	{
		Com_Printf("Loading %s failed! Disabling OpenAL.\n", al_driver->string);
		return false;
	}


	Cvar_Set("al_driver", name);


	/* Connect function pointers to management functions */
	qalcCreateContext = SDL_LoadFunction(handle, "alcCreateContext");
	qalcMakeContextCurrent = SDL_LoadFunction(handle, "alcMakeContextCurrent");
	qalcProcessContext = SDL_LoadFunction(handle, "alcProcessContext");
	qalcSuspendContext = SDL_LoadFunction(handle, "alcSuspendContext");
	qalcDestroyContext = SDL_LoadFunction(handle, "alcDestroyContext");
	qalcGetCurrentContext = SDL_LoadFunction(handle, "alcGetCurrentContext");
	qalcGetContextsDevice = SDL_LoadFunction(handle, "alcGetContextsDevice");
	qalcOpenDevice = SDL_LoadFunction(handle, "alcOpenDevice");
	qalcCloseDevice = SDL_LoadFunction(handle, "alcCloseDevice");
	qalcGetError = SDL_LoadFunction(handle, "alcGetError");
	qalcIsExtensionPresent = SDL_LoadFunction(handle, "alcIsExtensionPresent");
	qalcGetProcAddress = SDL_LoadFunction(handle, "alcGetProcAddress");
	qalcGetEnumValue = SDL_LoadFunction(handle, "alcGetEnumValue");
	qalcGetString = SDL_LoadFunction(handle, "alcGetString");
	qalcGetIntegerv = SDL_LoadFunction(handle, "alcGetIntegerv");
	qalcCaptureOpenDevice = SDL_LoadFunction(handle, "alcCaptureOpenDevice");
	qalcCaptureCloseDevice = SDL_LoadFunction(handle, "alcCaptureCloseDevice");
	qalcCaptureStart = SDL_LoadFunction(handle, "alcCaptureStart");
	qalcCaptureStop = SDL_LoadFunction(handle, "alcCaptureStop");
	qalcCaptureSamples = SDL_LoadFunction(handle, "alcCaptureSamples");

	/* Connect function pointers to
	   to OpenAL API functions */
	qalEnable = SDL_LoadFunction(handle, "alEnable");
	qalDisable = SDL_LoadFunction(handle, "alDisable");
	qalIsEnabled = SDL_LoadFunction(handle, "alIsEnabled");
	qalGetString = SDL_LoadFunction(handle, "alGetString");
	qalGetBooleanv = SDL_LoadFunction(handle, "alGetBooleanv");
	qalGetIntegerv = SDL_LoadFunction(handle, "alGetIntegerv");
	qalGetFloatv = SDL_LoadFunction(handle, "alGetFloatv");
	qalGetDoublev = SDL_LoadFunction(handle, "alGetDoublev");
	qalGetBoolean = SDL_LoadFunction(handle, "alGetBoolean");
	qalGetInteger = SDL_LoadFunction(handle, "alGetInteger");
	qalGetFloat = SDL_LoadFunction(handle, "alGetFloat");
	qalGetDouble = SDL_LoadFunction(handle, "alGetDouble");
	qalGetError = SDL_LoadFunction(handle, "alGetError");
	qalIsExtensionPresent = SDL_LoadFunction(handle, "alIsExtensionPresent");
	qalGetProcAddress = SDL_LoadFunction(handle, "alGetProcAddress");
	qalGetEnumValue = SDL_LoadFunction(handle, "alGetEnumValue");
	qalListenerf = SDL_LoadFunction(handle, "alListenerf");
	qalListener3f = SDL_LoadFunction(handle, "alListener3f");
	qalListenerfv = SDL_LoadFunction(handle, "alListenerfv");
	qalListeneri = SDL_LoadFunction(handle, "alListeneri");
	qalListener3i = SDL_LoadFunction(handle, "alListener3i");
	qalListeneriv = SDL_LoadFunction(handle, "alListeneriv");
	qalGetListenerf = SDL_LoadFunction(handle, "alGetListenerf");
	qalGetListener3f = SDL_LoadFunction(handle, "alGetListener3f");
	qalGetListenerfv = SDL_LoadFunction(handle, "alGetListenerfv");
	qalGetListeneri = SDL_LoadFunction(handle, "alGetListeneri");
	qalGetListener3i = SDL_LoadFunction(handle, "alGetListener3i");
	qalGetListeneriv = SDL_LoadFunction(handle, "alGetListeneriv");
	qalGenSources = SDL_LoadFunction(handle, "alGenSources");
	qalDeleteSources = SDL_LoadFunction(handle, "alDeleteSources");
	qalIsSource = SDL_LoadFunction(handle, "alIsSource");
	qalSourcef = SDL_LoadFunction(handle, "alSourcef");
	qalSource3f = SDL_LoadFunction(handle, "alSource3f");
	qalSourcefv = SDL_LoadFunction(handle, "alSourcefv");
	qalSourcei = SDL_LoadFunction(handle, "alSourcei");
	qalSource3i = SDL_LoadFunction(handle, "alSource3i");
	qalSourceiv = SDL_LoadFunction(handle, "alSourceiv");
	qalGetSourcef = SDL_LoadFunction(handle, "alGetSourcef");
	qalGetSource3f = SDL_LoadFunction(handle, "alGetSource3f");
	qalGetSourcefv = SDL_LoadFunction(handle, "alGetSourcefv");
	qalGetSourcei = SDL_LoadFunction(handle, "alGetSourcei");
	qalGetSource3i = SDL_LoadFunction(handle, "alGetSource3i");
	qalGetSourceiv = SDL_LoadFunction(handle, "alGetSourceiv");
	qalSourcePlayv = SDL_LoadFunction(handle, "alSourcePlayv");
	qalSourceStopv = SDL_LoadFunction(handle, "alSourceStopv");
	qalSourceRewindv = SDL_LoadFunction(handle, "alSourceRewindv");
	qalSourcePausev = SDL_LoadFunction(handle, "alSourcePausev");
	qalSourcePlay = SDL_LoadFunction(handle, "alSourcePlay");
	qalSourceStop = SDL_LoadFunction(handle, "alSourceStop");
	qalSourceRewind = SDL_LoadFunction(handle, "alSourceRewind");
	qalSourcePause = SDL_LoadFunction(handle, "alSourcePause");
	qalSourceQueueBuffers = SDL_LoadFunction(handle, "alSourceQueueBuffers");
	qalSourceUnqueueBuffers = SDL_LoadFunction(handle, "alSourceUnqueueBuffers");
	qalGenBuffers = SDL_LoadFunction(handle, "alGenBuffers");
	qalDeleteBuffers = SDL_LoadFunction(handle, "alDeleteBuffers");
	qalIsBuffer = SDL_LoadFunction(handle, "alIsBuffer");
	qalBufferData = SDL_LoadFunction(handle, "alBufferData");
	qalBufferf = SDL_LoadFunction(handle, "alBufferf");
	qalBuffer3f = SDL_LoadFunction(handle, "alBuffer3f");
	qalBufferfv = SDL_LoadFunction(handle, "alBufferfv");
	qalBufferi = SDL_LoadFunction(handle, "alBufferi");
	qalBuffer3i = SDL_LoadFunction(handle, "alBuffer3i");
	qalBufferiv = SDL_LoadFunction(handle, "alBufferiv");
	qalGetBufferf = SDL_LoadFunction(handle, "alGetBufferf");
	qalGetBuffer3f = SDL_LoadFunction(handle, "alGetBuffer3f");
	qalGetBufferfv = SDL_LoadFunction(handle, "alGetBufferfv");
	qalGetBufferi = SDL_LoadFunction(handle, "alGetBufferi");
	qalGetBuffer3i = SDL_LoadFunction(handle, "alGetBuffer3i");
	qalGetBufferiv = SDL_LoadFunction(handle, "alGetBufferiv");
	qalDopplerFactor = SDL_LoadFunction(handle, "alDopplerFactor");
	qalDopplerVelocity = SDL_LoadFunction(handle, "alDopplerVelocity");
	qalSpeedOfSound = SDL_LoadFunction(handle, "alSpeedOfSound");
	qalDistanceModel = SDL_LoadFunction(handle, "alDistanceModel");
	qalGenFilters = SDL_LoadFunction(handle, "alGenFilters");
	qalFilteri = SDL_LoadFunction(handle, "alFilteri");
	qalFilterf = SDL_LoadFunction(handle, "alFilterf");
	qalDeleteFilters = SDL_LoadFunction(handle, "alDeleteFilters");

	// OpenAL-MOB specific functions
	qalSetConfigMOB = SDL_LoadFunction(handle, "alSetConfigMOB");
	qalcDeviceEnableHrtfMOB = SDL_LoadFunction(handle, "alcDeviceEnableHrtfMOB");
	
	if (qalSetConfigMOB)
	{
		Com_Printf("...found OpenAL-MOB\n");
		sndfreq = 44100;
		qalSetConfigMOB(g_soundConfig);
	}

	/* Open the OpenAL device */
	{
		char* devices = (char*)qalcGetString(NULL, ALC_DEVICE_SPECIFIER);
		while(!device && devices && (void *) *devices != NULL)
		{
			Com_Printf("...found OpenAL device: %s\n",devices);
			devices += strlen(devices) + 1; //next device
		}
	}

	{
		const char *dev = al_device->string[0] ? al_device->string : qalcGetString(NULL,ALC_DEFAULT_DEVICE_SPECIFIER);
		Com_Printf("...attempting to open OpenAL device '%s': ",dev);
		device = qalcOpenDevice(dev);
	}

	if (!device)
	{
		Com_Printf("failed!\n...attempting to open default OpenAL device: ");
		device = qalcOpenDevice(qalcGetString(NULL,ALC_DEFAULT_DEVICE_SPECIFIER));
	}

	if(!device)
	{
		Com_DPrintf("failed\n");
		QAL_Shutdown();
		return false;
	}

	Com_Printf("ok\n");

	/* Create the OpenAL context */
	Com_Printf("...creating OpenAL context: ");



	// the parameters
	{
		const ALint params[] =
		{
			ALC_FREQUENCY, sndfreq,   // The HRTF only works for 44.1KHz output.      
			//ALC_FREQUENCY, 44100,   // The HRTF only works for 44.1KHz output.      
			0,          // Null terminator
		};
		context = qalcCreateContext(device, params);
	}

	if(!context)
	{
		Com_DPrintf("failed\n");
		QAL_Shutdown();
		return false;
	}

	Com_Printf("ok\n");



	/* Set the created context as current context */
	Com_Printf("...making context current: ");

	if (!qalcMakeContextCurrent(context))
	{
		Com_DPrintf("failed\n");
		QAL_Shutdown();
		return false;
	}

	Com_Printf("ok\n");

	/* Print OpenAL informations */
	Com_Printf("\n");
	QAL_SoundInfo();
	Com_Printf("\n");

    return true;
}

#endif /* USE_OPENAL */

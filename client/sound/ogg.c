/*
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
 * This file implements an interface to libvorbis for decoding
 * OGG/Vorbis files. Strongly spoken this file isn't part of the sound
 * system but part of the main client. It justs converts Vorbis streams
 * into normal, raw Wave stream which are injected into the backends as 
 * if they were normal "raw" samples. At this moment only background
 * music playback and in theory .cin movie file playback is supported.
 *
 * =======================================================================
 */

//#include <sys/time.h>
#include <errno.h>

//#include "../include/vorbisfile.h"
#include "include/stb_vorbis.c"
#include "../client.h"
#include "include/local.h"
#include "include/vorbis.h"

static qboolean ogg_first_init = true; /* First initialization flag. */
static qboolean ogg_started = false;   /* Initialization flag. */
static int ogg_bigendian = 0;
static byte *ogg_buffer;				/* File buffer. */
static sset_t ogg_filelist;
static short ovBuf[8192];               /* Buffer for sound. */
static int ogg_curfile;				/* Index of currently played file. */
static int ovSection;					/* Position in Ogg Vorbis file. */
static ogg_status_t ogg_status;		/* Status indicator. */
static cvar_t *ogg_autoplay;			/* Play this song when started. */
static cvar_t *ogg_check;				/* Check Ogg files or not. */
static cvar_t *ogg_playlist;			/* Playlist. */
static cvar_t *ogg_sequence;			/* Sequence play indicator. */
static cvar_t *ogg_volume;				/* Music volume. */
static stb_vorbis *ovFile;			/* Ogg Vorbis file. */
static stb_vorbis_info ogg_info;			/* Ogg Vorbis file information */
static int ogg_numbufs;				/* Number of buffers for OpenAL */

/*
 * Initialize the Ogg Vorbis subsystem.
 */
void
OGG_Init(void)
{
	cvar_t *cv; /* Cvar pointer. */

	if (ogg_started)
	{
		return;
	}

	Com_Printf("Starting Ogg Vorbis.\n");

	/* Skip initialization if disabled. */
	cv = Cvar_Get("ogg_enable", "1", CVAR_ARCHIVE);

	if (cv->value < 1)
	{
		Com_Printf("Ogg Vorbis not initializing.\n");
		return;
	}

	if (bigendien == true)
	{
		ogg_bigendian = 1;
	}

	/* Cvars. */
	ogg_autoplay = Cvar_Get("ogg_autoplay", "", CVAR_ARCHIVE);
	ogg_check = Cvar_Get("ogg_check", "0", CVAR_ARCHIVE);
	ogg_playlist = Cvar_Get("ogg_playlist", "playlist", CVAR_ARCHIVE);
	ogg_sequence = Cvar_Get("ogg_sequence", "loop", CVAR_ARCHIVE);
	ogg_volume = Cvar_Get("ogg_volume", "0.7", CVAR_ARCHIVE);

	/* Console commands. */
	Cmd_AddCommand("ogg_list", OGG_ListCmd);
	Cmd_AddCommand("ogg_pause", OGG_PauseCmd);
	Cmd_AddCommand("ogg_play", OGG_PlayCmd);
	Cmd_AddCommand("ogg_reinit", OGG_Reinit);
	Cmd_AddCommand("ogg_resume", OGG_ResumeCmd);
	Cmd_AddCommand("ogg_status", OGG_StatusCmd);
	Cmd_AddCommand("ogg_stop", OGG_Stop);

	/* Build list of files. */
    Q_SSetInit(&ogg_filelist, 25, MAX_OSPATH, TAG_AUDIO);

	if (ogg_playlist->string[0] != '\0')
	{
		OGG_LoadPlaylist(ogg_playlist->string);
	}

	if (ogg_filelist.currentSize == 0)
	{
		OGG_LoadFileList();
	}

	/* Check if we have Ogg Vorbis files. */
	if (ogg_filelist.currentSize <= 0)
	{
		Com_Printf("No Ogg Vorbis files found.\n");
		ogg_started = true; /* For OGG_Shutdown(). */
		OGG_Shutdown();
		return;
	}

	/* Initialize variables. */
	if (ogg_first_init)
	{
        ovFile = NULL;
		ogg_buffer = NULL;
		ogg_curfile = -1;
		ogg_status = STOP;
		ogg_first_init = false;
	}

	ogg_started = true;

	Com_Printf("%d Ogg Vorbis files found.\n", ogg_filelist.currentSize);

	/* Autoplay support. */
	if (ogg_autoplay->string[0] != '\0')
	{
		OGG_ParseCmd(ogg_autoplay->string);
	}
}

/*
 * Shutdown the Ogg Vorbis subsystem.
 */
void
OGG_Shutdown(void)
{
	if (!ogg_started)
	{
		return;
	}

	Com_Printf("Shutting down Ogg Vorbis.\n");

	OGG_Stop();

	/* Free the list of files. */
    Q_SSetFree(&ogg_filelist);
    
	/* Remove console commands. */
	Cmd_RemoveCommand("ogg_list");
	Cmd_RemoveCommand("ogg_pause");
	Cmd_RemoveCommand("ogg_play");
	Cmd_RemoveCommand("ogg_reinit");
	Cmd_RemoveCommand("ogg_resume");
	Cmd_RemoveCommand("ogg_status");
	Cmd_RemoveCommand("ogg_stop");

	ogg_started = false;
}

/*
 * Reinitialize the Ogg Vorbis subsystem.
 */
void
OGG_Reinit(void)
{
	OGG_Shutdown();
	OGG_Init();
}

/*
 * Check if the file is a valid Ogg Vorbis file.
 */
qboolean
OGG_Check(const char *name)
{
	qboolean res;        /* Return value. */
	byte *buffer;        /* File buffer. */
	int size;            /* File size. */
	stb_vorbis *ovf;  /* Ogg Vorbis file. */
    int error = VORBIS__no_error;

	if (ogg_check->value <= 0)
	{
		return true;
	}

	res = false;

	if ((size = FS_LoadFile(name, (void **)&buffer)) > 0)
	{
        if ((ovf = stb_vorbis_open_memory(buffer, size, &error, NULL))) {
            res = true;
            stb_vorbis_close(ovf);
        }
		FS_FreeFile(buffer);
	}

	return res;
}

/*
 * Load list of Ogg Vorbis files in "music".
 */
void
OGG_LoadFileList(void)
{
	int i;		 /* Loop counter. */
    int numFiles = 0;
    sset_t files;
    
    Q_SSetInit(&files, 25, MAX_OSPATH, TAG_AUDIO);
    
	/* Get file list. */
	numFiles = FS_ListFilesWithPaks(va("%s/*.ogg", OGG_DIR),
		   	&files, 0, SFF_SUBDIR | SFF_HIDDEN |
			SFF_SYSTEM);

	/* Check if there are posible Ogg files. */
	if (numFiles == 0)
	{
        Q_SSetFree(&files);
		return;
	}

    for (i = 0; i < files.currentSize; i++)
    {
        const char *p = Q_SSetGetString(&files, i);
        if (OGG_Check(p))
        {
            Q_SSetInsert(&ogg_filelist, p);
        }
    }
}

/*
 * Load playlist.
 */
void
OGG_LoadPlaylist(const char *playlist)
{
	byte *buffer; /* Buffer to read the file. */
	char *ptr;    /* Pointer for parsing the file. */
	int size;     /* Length of buffer and strings. */
	/* Open playlist. */
	if ((size = FS_LoadFile(va("%s/%s.lst", OGG_DIR, 
				  ogg_playlist->string), (void **)&buffer)) < 0)
	{
		Com_Printf("OGG_LoadPlaylist: could not open playlist: %s.\n",
				strerror(errno));
		return;
	}

	/* Count the files in playlist. */
	for (ptr = strtok((char *)buffer, "\n");
		 ptr != NULL;
		 ptr = strtok(NULL, "\n"))
	{
        char *p = NULL;
		if ((byte *)ptr != buffer)
		{
			ptr[-1] = '\n';
		}
        
        p = va("%s/%s", OGG_DIR, ptr);
		if (OGG_Check(va("%s/%s", OGG_DIR, ptr)))
		{
            Q_SSetInsert(&ogg_filelist, p);
        }
	}
    
	/* Free file buffer. */
	FS_FreeFile(buffer);
}

/*
 * Play Ogg Vorbis file (with absolute or relative index).
 */
qboolean
OGG_Open(ogg_seek_t type, int offset)
{
	int size;     /* File size. */
	int pos = -1; /* Absolute position. */
	int res = 0;      /* Error indicator. */
    int error = VORBIS__no_error;
    const char *p = NULL;
	switch (type)
	{
		case ABS:

			/* Absolute index. */
			if ((offset < 0) || (offset >= ogg_filelist.currentSize))
			{
				Com_Printf("OGG_Open: %d out of range.\n", offset + 1);
				return false;
			}
			else
			{
				pos = offset;
			}

			break;
		case REL:

			/* Simulate a loopback. */
			if ((ogg_curfile == -1) && (offset < 0))
			{
				offset++;
			}

			while (ogg_curfile + offset < 0)
			{
				offset += ogg_filelist.currentSize;
			}

			while (ogg_curfile + offset >= ogg_filelist.currentSize)
			{
				offset -= ogg_filelist.currentSize;
			}

			pos = ogg_curfile + offset;
			break;
	}

	/* Check running music. */
	if (ogg_status == PLAY)
	{
		if (ogg_curfile == pos)
		{
			return true;
		}

		else
		{
			OGG_Stop();
		}
	}

    p = Q_SSetGetString(&ogg_filelist, pos);
	/* Find file. */
	if ((size = FS_LoadFile(p, (void **)&ogg_buffer)) == -1)
	{
		Com_Printf("OGG_Open: could not open %d (%s): %s.\n",
				pos, p, strerror(errno));
		return false;
	}

	/* Open ogg vorbis file. */
    if (!(ovFile = stb_vorbis_open_memory(ogg_buffer, size, &error, NULL)))
	{
		Com_Printf("OGG_Open: '%s' is not a valid Ogg Vorbis file (error %i).\n", p, res);
        FS_FreeFile(ogg_buffer);
		ogg_buffer = NULL;
		return false;
	}

    ogg_info = stb_vorbis_get_info(ovFile);

	/* Play file. */
	ovSection = 0;
	ogg_curfile = pos;
	ogg_status = PLAY;

	return true;
}

/*
 * Play Ogg Vorbis file (with name only).
 */
qboolean
OGG_OpenName(const char *filename)
{
	char *name; /* File name. */
	int i;		/* Loop counter. */

	/* If the track name is '00', stop playback */
	if (!strncmp(filename, "00", sizeof(char) * 3))
	{
		OGG_PauseCmd();
		return false;
	}

	name = va("%s/%s.ogg", OGG_DIR, filename);

	for (i = 0; i < ogg_filelist.currentSize; i++)
	{
		if (strcmp(name, Q_SSetGetString(&ogg_filelist, i)) == 0)
		{
			break;
		}
	}

	if (i < ogg_filelist.currentSize)
	{
		return OGG_Open(ABS, i);
	}

	else
	{
		Com_Printf("OGG_OpenName: '%s' not in the list.\n", filename);
		return false;
	}
}

/*
 * Play a portion of the currently opened file.
 */
int
OGG_Read(void)
{
	int res; /* Number of bytes read. */
    res = stb_vorbis_get_frame_short_interleaved(ovFile, ogg_info.channels, ovBuf, sizeof(ovBuf));
	/* Read and resample. */
	S_RawSamples(res,ogg_info.sample_rate, OGG_SAMPLEWIDTH, ogg_info.channels,
			(byte *)ovBuf, ogg_volume->value);

	/* Check for end of file. */
	if (res == 0)
	{
		OGG_Stop();
		OGG_Sequence();
	}

	return res;
}

/*
 * Play files in sequence.
 */
void
OGG_Sequence(void)
{
	if (strcmp(ogg_sequence->string, "next") == 0)
	{
		OGG_Open(REL, 1);
	}

	else if (strcmp(ogg_sequence->string, "prev") == 0)
	{
		OGG_Open(REL, -1);
	}

	else if (strcmp(ogg_sequence->string, "random") == 0)
	{
		OGG_Open(ABS, rand() % ogg_filelist.currentSize);
	}

	else if (strcmp(ogg_sequence->string, "loop") == 0)
	{
		OGG_Open(REL, 0);
	}

	else if (strcmp(ogg_sequence->string, "none") != 0)
	{
		Com_Printf("Invalid value of ogg_sequence: %s\n", ogg_sequence->string);
		Cvar_Set("ogg_sequence", "none");
	}
}

/*
 * Stop playing the current file.
 */
void
OGG_Stop(void)
{
	if (ogg_status == STOP)
	{
		return;
	}

#ifdef USE_OPENAL
	if (sound_started == SS_OAL)
	{
		AL_UnqueueRawSamples();
	}
#endif

    stb_vorbis_close(ovFile);
	ogg_status = STOP;
	ogg_numbufs = 0;

	if (ogg_buffer != NULL)
	{
		FS_FreeFile(ogg_buffer);
		ogg_buffer = NULL;
	}
}

/*
 * Stream music.
 */
void
OGG_Stream(void)
{
	if (!ogg_started)
	{
		return;
	}

	if (ogg_status == PLAY)
	{
#ifdef USE_OPENAL
		if (sound_started == SS_OAL)
		{
            
			/* Calculate the number of buffers used
			   for storing decoded OGG/Vorbis data.
			   We take the number of active buffers
			   at startup (at this point most of the
			   samples should be precached and loaded
			   into buffers) and add 64. Empircal
			   testing showed, that at most times
			   at least 52 buffers remain available
			   for OGG/Vorbis, enough for about 3
			   seconds playback. The music won't
			   stutter as long as the framerate
			   stayes over 1 FPS. */
            /* since moving to a pre-allocated streaming buffer model
               the above is no longer accurate. I'm leaving the comment
               intact because the information is useful
               --dghost 01/25/2015 */

            
			/* activeStreamBuffers is the count of stream buffers,
               maxStreamBuffers is the total available stream buffers */
			while (activeStreamBuffers < maxStreamBuffers)
			{
//                Com_Printf("%i %i\n",activeStreamBuffers, maxStreamBuffers);
				OGG_Read();
			}
		}
		else /* using SDL */
#endif
		{
			if (sound_started == SS_SDL)
			{
				/* Read that number samples into the buffer, that
				   were played since the last call to this function.
				   This keeps the buffer at all times at an "optimal"
				   fill level. */
				while (paintedtime + MAX_RAW_SAMPLES - 2048 > s_rawend)
				{
					OGG_Read();
				}
			}
		} /* using SDL */
	} /* ogg_status == PLAY */
}

/*
 * List Ogg Vorbis files.
 */
void
OGG_ListCmd(void)
{
	int i;

	for (i = 0; i < ogg_filelist.currentSize; i++)
	{
		Com_Printf("%d %s\n", i + 1, Q_SSetGetString(&ogg_filelist, i));
	}

	Com_Printf("%d Ogg Vorbis files.\n", ogg_filelist.currentSize);
}

/*
 * Parse play controls.
 */
void
OGG_ParseCmd(const char *arg)
{
	int n;
	cvar_t *ogg_enable;

	ogg_enable = Cvar_Get("ogg_enable", "1", CVAR_ARCHIVE);

	switch (arg[0])
	{
		case '#':
			n = (int)strtol(arg + 1, (char **)NULL, 10) - 1;
			OGG_Open(ABS, n);
			break;
		case '?':
			OGG_Open(ABS, rand() % ogg_filelist.currentSize);
			break;
		case '>':

			if (strlen(arg) > 1)
			{
				OGG_Open(REL, (int)strtol(arg + 1, (char **)NULL, 10));
			}

			else
			{
				OGG_Open(REL, 1);
			}

			break;
		case '<':

			if (strlen(arg) > 1)
			{
				OGG_Open(REL, -(int)strtol(arg + 1, (char **)NULL, 10));
			}

			else
			{
				OGG_Open(REL, -1);
			}

			break;
		default:

			if (ogg_enable->value > 0)
			{
				OGG_OpenName(arg);
			}

			break;
	}
}

/*
 * Pause current song.
 */
void
OGG_PauseCmd(void)
{
	if (ogg_status == PLAY)
	{
		ogg_status = PAUSE;
		ogg_numbufs = 0;
	}
}

/*
 * Play control.
 */
void
OGG_PlayCmd(void)
{
	if (Cmd_Argc() < 2)
	{
		Com_Printf("Usage: ogg_play {filename | #n | ? | >n | <n}\n");
		return;
	}

	OGG_ParseCmd(Cmd_Argv(1));
}

/*
 * Resume current song.
 */
void
OGG_ResumeCmd(void)
{
	if (ogg_status == PAUSE)
	{
		ogg_status = PLAY;
	}
}

/*
 * Display status.
 */
void
OGG_StatusCmd(void)
{
	switch (ogg_status)
	{
		case PLAY:
        {
            unsigned int numSamples = stb_vorbis_stream_length_in_samples(ovFile);
            unsigned int curSample = stb_vorbis_get_sample_offset(ovFile);
            float trackLength = stb_vorbis_stream_length_in_seconds(ovFile);
            
            Com_Printf("Playing file %d (%s) at %0.2f seconds.\n",
                       ogg_curfile + 1, Q_SSetGetString(&ogg_filelist, ogg_curfile), (curSample /(float) numSamples) * trackLength);
        }
			break;
		case PAUSE:
        {
            unsigned int numSamples = stb_vorbis_stream_length_in_samples(ovFile);
            unsigned int curSample = stb_vorbis_get_sample_offset(ovFile);
            float trackLength = stb_vorbis_stream_length_in_seconds(ovFile);
            
            Com_Printf("Paused file %d (%s) at %0.2f seconds.\n",
                       ogg_curfile + 1, Q_SSetGetString(&ogg_filelist, ogg_curfile), (curSample /(float) numSamples) * trackLength);
        }
            break;
        case STOP:

			if (ogg_curfile == -1)
			{
				Com_Printf("Stopped.\n");
			}

			else
			{
				Com_Printf("Stopped file %d (%s).\n",
						ogg_curfile + 1, Q_SSetGetString(&ogg_filelist, ogg_curfile));
			}

			break;
	}
}


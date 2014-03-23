/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "qcommon.h"
#include "include/unzip.h"

// enables faster binary pak searck, still experimental
#define BINARY_PACK_SEARCH


#pragma warning (disable : 4715)

/*
=============================================================================

QUAKE FILESYSTEM

=============================================================================
*/

/*
All of Quake's data access is through a hierchal file system, but the
contents of the file system can be transparently merged from several
sources.

The "base directory" is the path to the directory holding the
quake.exe and all game directories.  The sys_* files pass this
to host_init in quakeparms_t->basedir.  This can be overridden
with the "+set game" command line parm to allow code debugging
in a different directory.  The base directory is only used
during filesystem initialization.

The "game directory" is the first tree on the search path and directory
that all generated files (savegames, screenshots, demos, config files)
will be saved to.  This can be overridden with the "-game" command line
parameter.  The game directory can never be changed while quake is
executing.  This is a precacution against having a malicious server
instruct clients to write files over areas they shouldn't.
*/

#define BASEDIRNAME				"baseq2"

#define MAX_HANDLES				32
#define MAX_READ				0x10000
#define MAX_WRITE				0x10000
#define MAX_FIND_FILES			0x04000


//
// in memory
//

//
// Berserk's pk3 file support
//

typedef struct {
	char			name[MAX_QPATH];
	fsMode_t		mode;
	FILE			*file;				// Only one of file or
	unzFile			*zip;				// zip will be used
} fsHandle_t;

typedef struct fsLink_s {
	char			*from;
	int32_t				length;
	char			*to;
	struct fsLink_s	*next;
} fsLink_t;

typedef struct {
	char			name[MAX_QPATH];
	long			hash;				// To speed up searching
	int32_t				size;
	int32_t				offset;				// This is ignored in PK3 files
	qboolean		ignore;				// Whether this file should be ignored
} fsPackFile_t;

typedef struct {
	char			name[MAX_OSPATH];
	FILE			*pak;
	unzFile			*pk3;
	int32_t				numFiles;
	fsPackFile_t	*files;
	uint32_t	contentFlags;
} fsPack_t;

typedef struct fsSearchPath_s {
	char			path[MAX_OSPATH];	// Only one of path or
	fsPack_t		*pack;				// pack will be used
	struct fsSearchPath_s	*next;
} fsSearchPath_t;

fsHandle_t		fs_handles[MAX_HANDLES];
fsLink_t		*fs_links;
fsSearchPath_t	*fs_searchPaths;
fsSearchPath_t	*fs_baseSearchPaths;

char			fs_gamedir[MAX_OSPATH];
static char		fs_currentGame[MAX_QPATH];

static char				fs_fileInPath[MAX_OSPATH];
static qboolean			fs_fileInPack;

int32_t		file_from_pak = 0;		// This is set by FS_FOpenFile
int32_t		file_from_pk3 = 0;		// This is set by FS_FOpenFile
char	last_pk3_name[MAX_QPATH];	// This is set by FS_FOpenFile

cvar_t	*fs_homepath;
cvar_t	*fs_basedir;
cvar_t	*fs_cddir;
cvar_t	*fs_gamedirvar;
cvar_t	*fs_debug;


void Com_FileExtension (const char *path, char *dst, int32_t dstSize);

/*
=================
Com_FilePath

Returns the path up to, but not including the last /
=================
*/
void Com_FilePath (const char *path, char *dst, int32_t dstSize){

	const char	*s, *last;

	s = last = path + strlen(path);
	while (*s != '/' && *s != '\\' && s != path){
		last = s-1;
		s--;
	}

	Q_strncpyz(dst, path, dstSize);
	if (last-path < dstSize)
		dst[last-path] = 0;
}


char *type_extensions[] =
{
	"bsp",
	"md2",
	"md3",
	"sp2",
	"dm2",
	"cin",
	"roq",
	"wav",
	"ogg",
	"pcx",
	"wal",
	"tga",
	"jpg",
	"png",
	"cfg",
	"txt",
	"def",
	"alias",
	"arena",
	"script",
//	"shader",
	"hud",
//	"menu",
//	"efx",
	0
};

/*
=================
FS_TypeFlagForPakItem
Returns bit flag based on pak item's extension.
=================
*/
uint32_t FS_TypeFlagForPakItem (char *itemName)
{
	int32_t		i;	
	char	extension[8];

	Com_FileExtension (itemName, extension, sizeof(extension));
	for (i=0; type_extensions[i]; i++) {
		if ( !Q_stricmp(extension, type_extensions[i]) )
			return (1<<i);
	}
	return 0;
}


/*
=================
FS_FileLength
=================
*/
int32_t FS_FileLength (FILE *f)
{
	int32_t cur, end;

	cur = ftell(f);
	fseek(f, 0, SEEK_END);
	end = ftell(f);
	fseek(f, cur, SEEK_SET);

	return end;
}

#if 0
/*
================
FS_Filelength
================
*/
int32_t FS_Filelength (FILE *f)
{
	int32_t		pos;
	int32_t		end;

	pos = ftell (f);
	fseek (f, 0, SEEK_END);
	end = ftell (f);
	fseek (f, pos, SEEK_SET);

	return end;
}
#endif


/*
============
FS_CreatePath

Creates any directories needed to store the given filename
============
*/
void FS_CreatePath (char *path)
{
	char	*ofs;

	FS_DPrintf("FS_CreatePath( %s )\n", path);

	if (strstr(path, "..") || strstr(path, "::") || strstr(path, "\\\\") || strstr(path, "//"))
	{
		Com_Printf(S_COLOR_YELLOW"WARNING: refusing to create relative path '%s'\n", path);
		return;
	}

	for (ofs = path+1 ; *ofs ; ofs++)
	{
		if (*ofs == '/' || *ofs == '\\') // Q2E changed
		//if (*ofs == '/')
		{	// create the directory
			*ofs = 0;
			Sys_Mkdir (path);
			*ofs = '/';
		}
	}
}


// Psychospaz's mod detector
qboolean modType (char *name)
{
	fsSearchPath_t	*search;

	for (search = fs_searchPaths ; search ; search = search->next)
	{
		if (strstr (search->path, name))
			return true;
	}
	return (0);
}


// This enables Rogue menu options for Q2MP4
qboolean roguepath ()
{
	if (modType("rogue") || modType("q2mp4"))
		return true;
	return false;
}


/*
=================
FS_DPrintf
=================
*/
void FS_DPrintf (const char *format, ...)
{
	char		msg[1024];
	va_list		argPtr;

	if (!fs_debug->value)
		return;

	va_start(argPtr, format);
//	vsprintf(msg, format, argPtr);
	Q_vsnprintf (msg, sizeof(msg), format, argPtr);
	va_end(argPtr);

	Com_Printf("%s", msg);
}


/*
=================
FS_GameDir

Called to find where to write a file (demos, savegames, etc...)
=================
*/
char *FS_GameDir (void)
{
	return fs_gamedir;
}

char *FS_Gamedir (void)
{
	return fs_gamedir;
}


/*
=================
FS_DeletePath

TODO: delete tree contents
=================
*/
void FS_DeletePath (char *path)
{
	FS_DPrintf("FS_DeletePath( %s )\n", path);

	Sys_Rmdir(path);
}


/*
=================
FS_FileForHandle

Returns a FILE * for a fileHandle_t
=================
*/
fsHandle_t	*FS_GetFileByHandle (fileHandle_t f);

FILE *FS_FileForHandle (fileHandle_t f)
{
	fsHandle_t	*handle;

	handle = FS_GetFileByHandle(f);

	if (handle->zip)
		Com_Error(ERR_DROP, "FS_FileForHandle: can't get FILE on zip file");

	if (!handle->file)
		Com_Error(ERR_DROP, "FS_FileForHandle: NULL");

	return handle->file;
}


/*
=================
FS_HandleForFile

Finds a free fileHandle_t
=================
*/
fsHandle_t *FS_HandleForFile (const char *path, fileHandle_t *f)
{
	fsHandle_t	*handle;
	int32_t			i;

	handle = fs_handles;
	for (i = 0; i < MAX_HANDLES; i++, handle++)
	{
		if (!handle->file && !handle->zip)
		{
			strcpy(handle->name, path);
			*f = i + 1;
			return handle;
		}
	}

	// Failed
	Com_Error(ERR_DROP, "FS_HandleForFile: none free");
}


/*
=================
FS_GetFileByHandle

Returns a fsHandle_t * for the given fileHandle_t
=================
*/
fsHandle_t *FS_GetFileByHandle (fileHandle_t f)
{
	if (f <= 0 || f > MAX_HANDLES)
		Com_Error(ERR_DROP, "FS_GetFileByHandle: out of range");

	return &fs_handles[f-1];
}

#ifdef BINARY_PACK_SEARCH
/*
=================
FS_FindPackItem

Performs a binary search by hashed filename
to find pack items in a sorted pack
=================
*/
int32_t FS_FindPackItem (fsPack_t *pack, char *itemName, long itemHash)
{
	int32_t		smax, smin, smidpt;	//, counter = 0;
	int32_t		i;	//, matchStart, matchEnd;

	// catch null pointers
	if ( !pack || !itemName )
		return -1;

	smin = 0;	smax = pack->numFiles;
	while ( (smax - smin) > 5 )	//&& counter < pack->numFiles )
	{
		smidpt = (smax + smin) / 2;
		if (pack->files[smidpt].hash > itemHash)	// before midpoint
			smax = smidpt;
		else if (pack->files[smidpt].hash < itemHash)	// after midpoint
			smin = smidpt;
		else	// pack->files[smidpt].hash == itemHash
			break;
	//	counter++;
	}
	for (i=smin; i<smax; i++)
	{	// make sure this entry is not blacklisted & compare filenames
		if ( pack->files[i].hash == itemHash && !pack->files[i].ignore
			&& !Q_stricmp(pack->files[i].name, itemName) )
			return i;
	}
	// found a match, scan for identical hashes
/*	if (pack->files[smidpt].hash == itemHash)
	{
		for (matchStart = smidpt-1; matchStart >= 0 && pack->files[matchStart].hash == itemHash; matchStart--);
		for (matchEnd = smidpt+1; matchEnd < pack->numFiles && pack->files[matchEnd].hash == itemHash; matchEnd++);
		for (i = matchStart; i < matchEnd; i++)
		{	// make sure this entry is not blacklisted & compare filenames
			if ( pack->files[i].hash == itemHash && !pack->files[i].ignore
				&& !Q_stricmp(pack->files[i].name, itemName) )
				return i;
		}
	}*/
	return -1;
}
#endif	// BINARY_PACK_SEARCH

/*
=================
FS_FOpenFileAppend

Returns file size or -1 on error
=================
*/
int32_t FS_FOpenFileAppend (fsHandle_t *handle)
{
	char	path[MAX_OSPATH];

	FS_CreatePath(handle->name);

	Com_sprintf(path, sizeof(path), "%s/%s", fs_gamedir, handle->name);

	handle->file = fopen(path, "ab");
	if (handle->file)
	{
		if (fs_debug->value)
			Com_Printf("FS_FOpenFileAppend: %s\n", path);

		return FS_FileLength(handle->file);
	}

	if (fs_debug->value)
		Com_Printf("FS_FOpenFileAppend: couldn't open %s\n", path);

	return -1;
}


/*
=================
FS_FOpenFileWrite

Always returns 0 or -1 on error
=================
*/
int32_t FS_FOpenFileWrite (fsHandle_t *handle)
{
	char	path[MAX_OSPATH];

	FS_CreatePath(handle->name);

	Com_sprintf(path, sizeof(path), "%s/%s", fs_gamedir, handle->name);

	handle->file = fopen(path, "wb");
	if (handle->file)
	{
		if (fs_debug->value)
			Com_Printf("FS_FOpenFileWrite: %s\n", path);
		return 0;
	}

	if (fs_debug->value)
		Com_Printf("FS_FOpenFileWrite: couldn't open %s\n", path);

	return -1;
}


/*
=================
FS_FOpenFileRead

Returns file size or -1 if not found.
Can open separate files as well as files inside pack files (both PAK 
and PK3).
=================
*/
int32_t FS_FOpenFileRead (fsHandle_t *handle)
{
	fsSearchPath_t	*search;
	fsPack_t		*pack;
	char			path[MAX_OSPATH];
	long			hash;
	int32_t				i;
	uint32_t	typeFlag;

	// Knightmare- hack global vars for autodownloads
	file_from_pak = 0;
	file_from_pk3 = 0;
	Com_sprintf(last_pk3_name, sizeof(last_pk3_name), "\0");
	hash = Com_HashFileName(handle->name, 0, false);
	typeFlag = FS_TypeFlagForPakItem(handle->name);

	// Search through the path, one element at a time
	for (search = fs_searchPaths; search; search = search->next)
	{
		if (search->pack)
		{	// Search inside a pack file
			pack = search->pack;

			// skip if pack doesn't contain this type of file
			if ((typeFlag != 0)) {
				if (!(pack->contentFlags & typeFlag))
					continue;
			}

#ifdef BINARY_PACK_SEARCH
			// find index of pack item
			i = FS_FindPackItem (pack, handle->name, hash);
			// found it!
			if ( i != -1 && i >= 0 && i < pack->numFiles )
			{
#else
			for (i = 0; i < pack->numFiles; i++)
			{
				if (pack->files[i].ignore)	// skip blacklisted files
					continue;
				if (hash != pack->files[i].hash)	// compare hash first
					continue;
#endif	// 	BINARY_PACK_SEARCH
				if (!Q_stricmp(pack->files[i].name, handle->name))
				{
					// Found it!
					Com_FilePath(pack->name, fs_fileInPath, sizeof(fs_fileInPath));
					fs_fileInPack = true;

					if (fs_debug->value)
						Com_Printf("FS_FOpenFileRead: %s (found in %s)\n", handle->name, pack->name);

					if (pack->pak)
					{	// PAK
						file_from_pak = 1; // Knightmare added
						handle->file = fopen(pack->name, "rb");
						if (handle->file)
						{
							fseek(handle->file, pack->files[i].offset, SEEK_SET);

							return pack->files[i].size;
						}
					}
					else if (pack->pk3)
					{	// PK3
						file_from_pk3 = 1; // Knightmare added
						Com_sprintf(last_pk3_name, sizeof(last_pk3_name), strrchr(pack->name, '/')+1); // Knightmare added
						handle->zip = unzOpen(pack->name);
						if (handle->zip)
						{
							if (unzLocateFile(handle->zip, handle->name, 2) == UNZ_OK)
							{
								if (unzOpenCurrentFile(handle->zip) == UNZ_OK)
									return pack->files[i].size;
							}

							unzClose(handle->zip);
						}
					}

					Com_Error(ERR_FATAL, "Couldn't reopen %s", pack->name);
				}
				else
					Com_Printf("FS_FOpenFileRead: different filenames with identical hash (%s, %s)!\n", pack->files[i].name, handle->name);
			}
		}
		else
		{	// Search in a directory tree
			Com_sprintf(path, sizeof(path), "%s/%s", search->path, handle->name);

			handle->file = fopen(path, "rb");
			if (handle->file)
			{	// Found it!
				Q_strncpyz(fs_fileInPath, search->path, sizeof(fs_fileInPath));
				fs_fileInPack = false;

				if (fs_debug->value)
					Com_Printf("FS_FOpenFileRead: %s (found in %s)\n", handle->name, search->path);

				return FS_FileLength(handle->file);
			}
		}
	}

	// Not found!
	fs_fileInPath[0] = 0;
	fs_fileInPack = false;

	if (fs_debug->value)
		Com_Printf("FS_FOpenFileRead: couldn't find %s\n", handle->name);

	return -1;
}

/*
=================
FS_FOpenFile

Opens a file for "mode".
Returns file size or -1 if an error occurs/not found.
Can open separate files as well as files inside pack files (both PAK 
and PK3).
=================
*/
int32_t FS_FOpenFile (const char *name, fileHandle_t *f, fsMode_t mode)
{
	fsHandle_t	*handle;
	int32_t			size;

	handle = FS_HandleForFile(name, f);

	Q_strncpyz(handle->name, name, sizeof(handle->name));
	handle->mode = mode;

	switch (mode)
	{
	case FS_READ:
		size = FS_FOpenFileRead(handle);
		break;
	case FS_WRITE:
		size = FS_FOpenFileWrite(handle);
		break;
	case FS_APPEND:
		size = FS_FOpenFileAppend(handle);
		break;
	default:
		Com_Error(ERR_FATAL, "FS_FOpenFile: bad mode (%i)", mode);
	}

	if (size != -1)
		return size;

	// Couldn't open, so free the handle
	memset(handle, 0, sizeof(*handle));

	*f = 0;
	return -1;
}


/*
=================
FS_FCloseFile
=================
*/
void FS_FCloseFile (fileHandle_t f)
{
	fsHandle_t *handle;

	handle = FS_GetFileByHandle(f);

	if (handle->file)
		fclose(handle->file);
	else if (handle->zip) {
		unzCloseCurrentFile(handle->zip);
		unzClose(handle->zip);
	}

	memset(handle, 0, sizeof(*handle));
}

/*
=================
FS_Read

Properly handles partial reads
=================
*/
int32_t FS_Read (void *buffer, int32_t size, fileHandle_t f)
{
	fsHandle_t	*handle;
	int32_t			remaining, r;
	byte		*buf;

	handle = FS_GetFileByHandle(f);

	// Read
	remaining = size;
	buf = (byte *)buffer;

	while (remaining)
	{
		if (handle->file)
			r = fread(buf, 1, remaining, handle->file);
		else if (handle->zip)
			r = unzReadCurrentFile(handle->zip, buf, remaining);
		else
			return 0;

		if (r == 0)
		{
			//Com_Error(ERR_FATAL, va("FS_Read: 0 bytes read from %s", handle->name));
			Com_DPrintf(S_COLOR_YELLOW"FS_Read: 0 bytes read from %s\n", handle->name);
			return size - remaining;
		}
		else if (r == -1)
			Com_Error(ERR_FATAL, "FS_Read: -1 bytes read from %s", handle->name);

		remaining -= r;
		buf += r;
	}

	return size;
}


/*
=================
FS_FRead

Properly handles partial reads of size up to count times
No error if it can't read
=================
*/
int32_t FS_FRead (void *buffer, int32_t size, int32_t count, fileHandle_t f)
{
	fsHandle_t	*handle;
	int32_t			loops, remaining, r;
	byte		*buf;

	handle = FS_GetFileByHandle(f);

	// Read
	loops = count;
	//remaining = size;
	buf = (byte *)buffer;

	while (loops)
	{	// Read in chunks
		remaining = size;
		while (remaining)
		{
			if (handle->file)
				r = fread(buf, 1, remaining, handle->file);
			else if (handle->zip)
				r = unzReadCurrentFile(handle->zip, buf, remaining);
			else
				return 0;

			if (r == 0)
			{
				//Com_Printf(S_COLOR_RED"FS_FRead: 0 bytes read from %s\n", handle->name);
				return size - remaining;
			}
			else if (r == -1)
				Com_Error(ERR_FATAL, "FS_FRead: -1 bytes read from %s", handle->name);

			remaining -= r;
			buf += r;
		}
		loops--;
	}
	return size;
}

/*
=================
FS_Write

Properly handles partial writes
=================
*/
int32_t FS_Write (const void *buffer, int32_t size, fileHandle_t f){

	fsHandle_t	*handle;
	int32_t			remaining, w;
	byte		*buf;

	handle = FS_GetFileByHandle(f);

	// Write
	remaining = size;
	buf = (byte *)buffer;

	while (remaining)
	{
		if (handle->file)
			w = fwrite(buf, 1, remaining, handle->file);
		else if (handle->zip)
			Com_Error(ERR_FATAL, "FS_Write: can't write to zip file %s", handle->name);
		else
			return 0;

		if (w == 0)
		{
			Com_Printf(S_COLOR_RED"FS_Write: 0 bytes written to %s\n", handle->name);
			return size - remaining;
		}
		else if (w == -1)
			Com_Error(ERR_FATAL, "FS_Write: -1 bytes written to %s", handle->name);

		remaining -= w;
		buf += w;
	}

	return size;
}

/*
=================
FS_FTell
=================
*/
int32_t FS_FTell (fileHandle_t f)
{

	fsHandle_t *handle;

	handle = FS_GetFileByHandle(f);

	if (handle->file)
		return ftell(handle->file);
	else if (handle->zip)
		return unztell(handle->zip);

	return 0;
}

/*
=================
FS_ListPak

Generates a listing of the contents of a pak file
=================
*/
char **FS_ListPak (char *find, int32_t *num)
{
	fsSearchPath_t	*search;
	//char			netpath[MAX_OSPATH];
	fsPack_t		*pak;

	int32_t nfiles = 0, nfound = 0;
	char **list = 0;
	int32_t i;

	// now check pak files
	for (search = fs_searchPaths; search; search = search->next)
	{
		if (!search->pack)
			continue;

		pak = search->pack;

		// now find and build list
		for (i=0 ; i<pak->numFiles ; i++) {
			if (!pak->files[i].ignore)
				nfiles++;
		}
	}

	list =  (char **) malloc( sizeof( char * ) * nfiles );
	memset( list, 0, sizeof( char * ) * nfiles );

	for (search = fs_searchPaths; search; search = search->next)
	{
		if (!search->pack)
			continue;

		pak = search->pack;

		// now find and build list
		for (i=0 ; i<pak->numFiles ; i++)
		{
			if (!pak->files[i].ignore && strstr(pak->files[i].name, find))
			{
				list[nfound] = strdup(pak->files[i].name);
				nfound++;
			}
		}
	}
	
	*num = nfound;

	return list;		
}

/*
=================
FS_Seek
=================
*/
void FS_Seek (fileHandle_t f, int32_t offset, fsOrigin_t origin)
{
	fsHandle_t		*handle;
	unz_file_info	info;
	int32_t				remaining, r, len;
	byte			dummy[0x8000];

	handle = FS_GetFileByHandle(f);

	if (handle->file)
	{
		switch (origin)
		{
		case FS_SEEK_SET:
			fseek(handle->file, offset, SEEK_SET);
			break;
		case FS_SEEK_CUR:
			fseek(handle->file, offset, SEEK_CUR);
			break;
		case FS_SEEK_END:
			fseek(handle->file, offset, SEEK_END);
			break;
		default:
			Com_Error(ERR_FATAL, "FS_Seek: bad origin (%i)", origin);
		}
	}
	else if (handle->zip)
	{
		switch (origin)
		{
		case FS_SEEK_SET:
			remaining = offset;
			break;
		case FS_SEEK_CUR:
			remaining = offset + unztell(handle->zip);
			break;
		case FS_SEEK_END:
			unzGetCurrentFileInfo(handle->zip, &info, NULL, 0, NULL, 0, NULL, 0);

			remaining = offset + info.uncompressed_size;
			break;
		default:
			Com_Error(ERR_FATAL, "FS_Seek: bad origin (%i)", origin);
		}

		// Reopen the file
		unzCloseCurrentFile(handle->zip);
		unzOpenCurrentFile(handle->zip);

		// Skip until the desired offset is reached
		while (remaining)
		{
			len = remaining;
			if (len > sizeof(dummy))
				len = sizeof(dummy);

			r = unzReadCurrentFile(handle->zip, dummy, len);
			if (r <= 0)
				break;

			remaining -= r;
		}
	}
}

/*
=================
FS_Tell

Returns -1 if an error occurs
=================
*/
int32_t FS_Tell (fileHandle_t f)
{
	fsHandle_t *handle;

	handle = FS_GetFileByHandle(f);

	if (handle->file)
		return ftell(handle->file);
	else if (handle->zip)
		return unztell(handle->zip);
}

/*
=================
FS_FileExists
================
*/
qboolean FS_FileExists (char *path)
{
	fileHandle_t f;

	FS_FOpenFile(path, &f, FS_READ);
	if (f)
	{
		FS_FCloseFile(f);
		return true;
	}
	return false;
}

/*
=================
FS_RenameFile
=================
*/
void FS_RenameFile (const char *oldPath, const char *newPath)
{
	FS_DPrintf("FS_RenameFile( %s, %s )\n", oldPath, newPath);

	if (rename(oldPath, newPath))
		FS_DPrintf("FS_RenameFile: failed to rename %s to %s\n", oldPath, newPath);
}

/*
=================
FS_DeleteFile
=================
*/
void FS_DeleteFile (const char *path)
{
	FS_DPrintf("FS_DeleteFile( %s )\n", path);

	if (remove(path))
		FS_DPrintf("FS_DeleteFile: failed to delete %s\n", path);
}

/*
=================
FS_LoadFile

"path" is relative to the Quake search path.
Returns file size or -1 if the file is not found.
A NULL buffer will just return the file size without loading.
=================
*/
int32_t FS_LoadFile (char *path, void **buffer)
{
	fileHandle_t	f;
	byte			*buf;
	int32_t				size;

	buf = NULL;

	size = FS_FOpenFile(path, &f, FS_READ);
	if (size == -1 || size == 0)
	{
		if (buffer)
			*buffer = NULL;
		return size;
	}
	if (!buffer)
	{
		FS_FCloseFile(f);
		return size;
	}
	buf = Z_Malloc(size);
	*buffer = buf;

	FS_Read(buf, size, f);

	FS_FCloseFile(f);

	return size;
}

/*
=================
FS_FreeFile
=================
*/
void FS_FreeFile (void *buffer)
{
	if (!buffer)
	{
		FS_DPrintf("FS_FreeFile: NULL buffer\n");
		return;
	}
	Z_Free(buffer);
}

// Some incompetently packaged mods have these files in their paks!
char *pakfile_ignore_names[] =
{
	"save/",
	"scrnshot/",
	"autoexec.cfg",
	"vrconfig.cfg",
	0
};


/*
=================
FS_FileInPakBlacklist

Checks against a blacklist to see if a file
should not be loaded from a pak.
=================
*/
qboolean FS_FileInPakBlacklist (char *filename, qboolean isPk3)
{
	int32_t			i;
	char		*compare;
	qboolean	ignore = false;

	compare = filename;
	if (compare[0] == '/')	// remove leading slash
		compare++;

	for (i=0; pakfile_ignore_names[i]; i++) {
		if ( !Q_strncasecmp(compare, pakfile_ignore_names[i], strlen(pakfile_ignore_names[i])) )
			ignore = true;
		// Ogg files can't load from .paks
		if ( !isPk3 && !strcmp(COM_FileExtension(compare), "ogg") )
			ignore = true;
	}

//	if (ignore)
//		Com_Printf ("FS_LoadPAK: file %s blacklisted!\n", filename);
//	else if ( !strncmp (filename, "save/", 5) )
//		Com_Printf ("FS_LoadPAK: file %s not blacklisted.\n", filename);
	return ignore;
}


#ifdef BINARY_PACK_SEARCH
/*
=================
FS_PakFileCompare
 
Used for sorting pak entries by hash
=================
*/
long *nameHashes = NULL;
int32_t FS_PakFileCompare (const void *f1, const void *f2)
{
	if (!nameHashes)
		return 1;

	return (nameHashes[*((int32_t *)(f1))] - nameHashes[*((int32_t *)(f2))]);
}
#endif	// BINARY_PACK_SEARCH


/*
=================
FS_LoadPAK
 
Takes an explicit (not game tree related) path to a pack file.

Loads the header and directory, adding the files at the beginning of
the list so they override previous pack files.
=================
*/
fsPack_t *FS_LoadPAK (const char *packPath)
{
	int32_t				numFiles, i;
	fsPackFile_t	*files;
	fsPack_t		*pack;
	FILE			*handle;
	dpackheader_t		header;
	dpackfile_t		info[MAX_FILES_IN_PACK];
	unsigned		contentFlags = 0;
#ifdef BINARY_PACK_SEARCH
	int32_t				*sortIndices;
	long			*sortHashes;
#endif	// BINARY_PACK_SEARCH

	handle = fopen(packPath, "rb");
	if (!handle)
		return NULL;

	fread(&header, 1, sizeof(dpackheader_t), handle);
	
	if (LittleLong(header.ident) != IDPAKHEADER)
	{
		fclose(handle);
		Com_Error(ERR_FATAL, "FS_LoadPAK: %s is not a pack file", packPath);
	}

	header.dirofs = LittleLong(header.dirofs);
	header.dirlen = LittleLong(header.dirlen);

	numFiles = header.dirlen / sizeof(dpackfile_t);
	if (numFiles > MAX_FILES_IN_PACK || numFiles == 0)
	{
		fclose(handle);
		Com_Error(ERR_FATAL, "FS_LoadPAK: %s has %i files", packPath, numFiles);
	}

	files = Z_Malloc(numFiles * sizeof(fsPackFile_t));

	fseek(handle, header.dirofs, SEEK_SET);
	fread(info, 1, header.dirlen, handle);

#ifdef BINARY_PACK_SEARCH
	// create sort table
	sortIndices = Z_Malloc(numFiles * sizeof(int32_t));
	sortHashes = Z_Malloc(numFiles * sizeof(unsigned));
	nameHashes = sortHashes;
	for (i = 0; i < numFiles; i++)
	{
		sortIndices[i] = i;
		sortHashes[i] = Com_HashFileName(info[i].name, 0, false);
	}
	qsort((void *)sortIndices, numFiles, sizeof(int32_t), FS_PakFileCompare);

	// Parse the directory
	for (i = 0; i < numFiles; i++)
	{
		strcpy(files[i].name, info[sortIndices[i]].name);
		files[i].hash = sortHashes[sortIndices[i]];
		files[i].offset = LittleLong(info[sortIndices[i]].filepos);
		files[i].size = LittleLong(info[sortIndices[i]].filelen);
		files[i].ignore = FS_FileInPakBlacklist(files[i].name, false);	// check against pak loading blacklist
		if (!files[i].ignore)	// add type flag for this file
			contentFlags |= FS_TypeFlagForPakItem(files[i].name);
	}

	// free sort table
	Z_Free (sortIndices);
	Z_Free (sortHashes);
	nameHashes = NULL;
#else	// Parse the directory
	for (i = 0; i < numFiles; i++)
	{
		strcpy(files[i].name, info[i].name);
		files[i].hash = Com_HashFileName(info[i].name, 0, false);	// Added to speed up seaching
		files[i].offset = LittleLong(info[i].filepos);
		files[i].size = LittleLong(info[i].filelen);
		files[i].ignore = FS_FileInPakBlacklist(info[i].name, false);	// check against pak loading blacklist
		if (!files[i].ignore)	// add type flag for this file
			contentFlags |= FS_TypeFlagForPakItem(files[i].name);
	}
#endif	// BINARY_PACK_SEARCH

	pack = Z_Malloc(sizeof(fsPack_t));
	strcpy(pack->name, packPath);
	pack->pak = handle;
	pack->pk3 = NULL;
	pack->numFiles = numFiles;
	pack->files = files;
	pack->contentFlags = contentFlags;

	return pack;
}

/*
=================
FS_AddPAKFile

Adds a Pak file to the searchpath
=================
*/
void FS_AddPAKFile (const char *packPath)
{
	fsSearchPath_t	*search;
	fsPack_t		*pack;

    pack = FS_LoadPAK (packPath);
    if (!pack)
        return;
    search = Z_Malloc (sizeof(fsSearchPath_t));
    search->pack = pack;
    search->next = fs_searchPaths;
    fs_searchPaths = search;
}

/*
=================
FS_LoadPK3

Takes an explicit (not game tree related) path to a pack file.

Loads the header and directory, adding the files at the beginning of
the list so they override previous pack files.
=================
*/
fsPack_t *FS_LoadPK3 (const char *packPath)
{
	int32_t				numFiles, i = 0;
	fsPackFile_t	*files;
	fsPack_t		*pack;
	unzFile			*handle;
	unz_global_info	global;
	unz_file_info	info;
	int32_t				status;
	unsigned		contentFlags = 0;
	char			fileName[MAX_QPATH];
#ifdef BINARY_PACK_SEARCH
	fsPackFile_t	*tmpFiles;
	int32_t				*sortIndices;
	long			*sortHashes;
#endif	// BINARY_PACK_SEARCH

	handle = unzOpen(packPath);
	if (!handle)
		return NULL;

	if (unzGetGlobalInfo(handle, &global) != UNZ_OK)
	{
		unzClose(handle);
		Com_Error(ERR_FATAL, "FS_LoadPK3: %s is not a pack file", packPath);
	}
	numFiles = global.number_entry;
	if (numFiles > MAX_FILES_IN_PACK || numFiles == 0)
	{
		unzClose(handle);
		Com_Error(ERR_FATAL, "FS_LoadPK3: %s has %i files", packPath, numFiles);
	}
	files = Z_Malloc(numFiles * sizeof(fsPackFile_t));

#ifdef BINARY_PACK_SEARCH
	// create sort table
	tmpFiles = Z_Malloc(numFiles * sizeof(fsPackFile_t));
	sortIndices = Z_Malloc(numFiles * sizeof(int32_t));
	sortHashes = Z_Malloc(numFiles * sizeof(unsigned));
	nameHashes = sortHashes;	

	// Parse the directory
	status = unzGoToFirstFile(handle);
	while (status == UNZ_OK)
	{
		fileName[0] = 0;
		unzGetCurrentFileInfo(handle, &info, fileName, MAX_QPATH, NULL, 0, NULL, 0);
		sortIndices[i] = i;
		strcpy(tmpFiles[i].name, fileName);
		tmpFiles[i].hash = sortHashes[i] = Com_HashFileName(fileName, 0, false);	// Added to speed up seaching
		tmpFiles[i].offset = -1;		// Not used in ZIP files
		tmpFiles[i].size = info.uncompressed_size;
		tmpFiles[i].ignore = FS_FileInPakBlacklist(fileName, true);	// check against pak loading blacklist
		if (!tmpFiles[i].ignore)	// add type flag for this file
			contentFlags |= FS_TypeFlagForPakItem(tmpFiles[i].name);
		i++;
		status = unzGoToNextFile(handle);
	}

	// sort by hash and copy to final file table
	qsort((void *)sortIndices, numFiles, sizeof(int32_t), FS_PakFileCompare);
	for (i=0; i < numFiles; i++)
	{
		strcpy(files[i].name, tmpFiles[sortIndices[i]].name);
		files[i].hash = tmpFiles[sortIndices[i]].hash;
		files[i].offset = tmpFiles[sortIndices[i]].offset;
		files[i].size = tmpFiles[sortIndices[i]].size;
		files[i].ignore = tmpFiles[sortIndices[i]].ignore;
	}

	// free sort table
	Z_Free (tmpFiles);
	Z_Free (sortIndices);
	Z_Free (sortHashes);
	nameHashes = NULL;
#else	// Parse the directory
	status = unzGoToFirstFile(handle);
	while (status == UNZ_OK)
	{
		fileName[0] = 0;
		unzGetCurrentFileInfo(handle, &info, fileName, MAX_QPATH, NULL, 0, NULL, 0);

		strcpy(files[i].name, fileName);
		files[i].hash = Com_HashFileName(fileName, 0, false);	// Added to speed up seaching
		files[i].offset = -1;		// Not used in ZIP files
		files[i].size = info.uncompressed_size;
		files[i].ignore = FS_FileInPakBlacklist(fileName, true);	// check against pak loading blacklist
		if (!files[i].ignore)	// add type flag for this file
			contentFlags |= FS_TypeFlagForPakItem(files[i].name);
		i++;

		status = unzGoToNextFile(handle);
	}
#endif	// BINARY_PACK_SEARCH

	pack = Z_Malloc(sizeof(fsPack_t));
	strcpy(pack->name, packPath);
	pack->pak = NULL;
	pack->pk3 = handle;
	pack->numFiles = numFiles;
	pack->files = files;
	pack->contentFlags = contentFlags;

	return pack;
}

/*
=================
FS_AddPK3File

Adds a Pk3 file to the searchpath
=================
*/
void FS_AddPK3File (const char *packPath)
{
	fsSearchPath_t	*search;
	fsPack_t		*pack;

    pack = FS_LoadPK3 (packPath);
    if (!pack)
        return;
    search = Z_Malloc (sizeof(fsSearchPath_t));
    search->pack = pack;
    search->next = fs_searchPaths;
    fs_searchPaths = search;
}

/*
=================
FS_AddGameDirectory

Sets fs_gameDir, adds the directory to the head of the path, then loads
and adds all the pack files found (in alphabetical order).
 
PK3 files are loaded later so they override PAK files.
=================
*/
void FS_AddGameDirectory (const char *dir)
{
	fsSearchPath_t	*search;
//	fsPack_t		*pack;
	char			packPath[MAX_OSPATH];
	int32_t				i, j;
	// VoiD -S- *.pak support
	char *path = NULL;
	char findname[1024];
	char **dirnames;
	int32_t ndirs;
	char *tmp;
	// VoiD -E- *.pak support

	strcpy(fs_gamedir, dir);

	//
	// Add the directory to the search path
	//
	search = Z_Malloc(sizeof(fsSearchPath_t));
	strcpy(search->path, dir);
	search->path[sizeof(search->path)-1] = 0;
	search->next = fs_searchPaths;
	fs_searchPaths = search;

	//
	// add any pak files in the format pak0.pak pak1.pak, ...
	//
	for (i=0; i<100; i++)    // Pooy - paks can now go up to 100
	{
		Com_sprintf (packPath, sizeof(packPath), "%s/pak%i.pak", dir, i);
		FS_AddPAKFile (packPath);
	}
    //
    // NeVo - pak3's!
    // add any pk3 files in the format pak0.pk3 pak1.pk3, ...
    //
    for (i=0; i<100; i++)    // Pooy - paks can now go up to 100
    {
        Com_sprintf (packPath, sizeof(packPath), "%s/pak%i.pk3", dir, i);
        FS_AddPK3File (packPath);
    }

    for (i=0; i<2; i++)
    {	// NeVo - Set filetype
        switch (i) {
            case 0:
			default:
                // Standard Quake II pack file '.pak'
                Com_sprintf( findname, sizeof(findname), "%s/%s", dir, "*.pak" );
                break;
            case 1: 
                // Quake III pack file '.pk3'
                Com_sprintf( findname, sizeof(findname), "%s/%s", dir, "*.pk3" );
                break;
        }
		// VoiD -S- *.pack support
        tmp = findname;
        while ( *tmp != 0 )
        {
            if ( *tmp == '\\' )
                *tmp = '/';
            tmp++;
        }
        if ( ( dirnames = FS_ListFiles( findname, &ndirs, 0, 0 ) ) != 0 )
        {
            for ( j=0; j < ndirs-1; j++ )
            {	// don't reload numbered pak files
				int32_t		k;
				char	buf[16];
				char	buf2[16];
				qboolean numberedpak = false;
				for (k=0; k<100; k++)
				{
					Com_sprintf( buf, sizeof(buf), "/pak%i.pak", k);
					Com_sprintf( buf2, sizeof(buf2), "/pak%i.pk3", k);
					if ( strstr(dirnames[j], buf) || strstr(dirnames[j], buf2)) {
						numberedpak = true;
						break;
					}
				}
				if (numberedpak)
					continue;
                if ( strrchr( dirnames[j], '/' ) )
                {
					if (i==1)
						FS_AddPK3File (dirnames[j]);
					else
						FS_AddPAKFile (dirnames[j]);
                }
                free( dirnames[j] );
            }
            free( dirnames );
        }
        // VoiD -E- *.pack support
    } 
}

/*
=================
FS_NextPath

Allows enumerating all of the directories in the search path
=================
*/
char *FS_NextPath (char *prevPath)
{
	fsSearchPath_t	*search;
	char			*prev;

	if (!prevPath)
		return fs_gamedir;

	prev = fs_gamedir;
	for (search = fs_searchPaths; search; search = search->next)
	{
		if (search->pack)
			continue;

		if (prevPath == prev)
			return search->path;

		prev = search->path;
	}
	return NULL;
}


/*
=================
FS_Path_f
=================
*/
void FS_Path_f (void)
{
	fsSearchPath_t	*search;
	fsHandle_t		*handle;
	fsLink_t		*link;
	int32_t				totalFiles = 0, i;

	Com_Printf("Current search path:\n");

	for (search = fs_searchPaths; search; search = search->next)
	{
		if (search->pack)
		{
			Com_Printf("%s (%i files)\n", search->pack->name, search->pack->numFiles);
			totalFiles += search->pack->numFiles;
		}
		else
			Com_Printf("%s\n", search->path);
	}

	//Com_Printf("\n");

	for (i = 0, handle = fs_handles; i < MAX_HANDLES; i++, handle++)
	{
		if (handle->file || handle->zip)
			Com_Printf("Handle %i: %s\n", i + 1, handle->name);
	}

	for (i = 0, link = fs_links; link; i++, link = link->next)
		Com_Printf("Link %i: %s -> %s\n", i, link->from, link->to);

	Com_Printf("-------------------------------------\n");

	Com_Printf("%i files in PAK/PK3 files\n\n", totalFiles);
}

/*
=================
FS_Startup

TODO: close open files for game dir
=================
*/
void FS_Startup (void)
{
	if (strstr(fs_gamedirvar->string, "..") || strstr(fs_gamedirvar->string, ".")
		|| strstr(fs_gamedirvar->string, "/") || strstr(fs_gamedirvar->string, "\\")
		|| strstr(fs_gamedirvar->string, ":") || !fs_gamedirvar->string[0])
	{
		//Com_Printf("Invalid game directory\n");
		Cvar_ForceSet("game", BASEDIRNAME);
	}

	// Check for game override
	if (stricmp(fs_gamedirvar->string, fs_currentGame))
	{
		fsSearchPath_t	*next;
		fsPack_t		*pack;

		// Free up any current game dir info
		while (fs_searchPaths != fs_baseSearchPaths)
		{
			if (fs_searchPaths->pack)
			{
				pack = fs_searchPaths->pack;

				if (pack->pak)
					fclose(pack->pak);
				if (pack->pk3)
					unzClose(pack->pk3);

				Z_Free(pack->files);
				Z_Free(pack);
			}

			next = fs_searchPaths->next;
			Z_Free(fs_searchPaths);
			fs_searchPaths = next;
		}

		if (!stricmp(fs_gamedirvar->string, BASEDIRNAME))	// Don't add baseq2 again
			strcpy(fs_gamedir, fs_basedir->string);
		else
		{
			// Add the directories
			FS_AddGameDirectory(va("%s/%s", fs_homepath->string, fs_gamedirvar->string));
		}
	}

	strcpy(fs_currentGame, fs_gamedirvar->string);

	FS_Path_f();
}

/*
=================
FS_Init
=================
*/
void FS_Dir_f (void);
void FS_Link_f (void);
char *Sys_GetCurrentDirectory (void);

void FS_InitFilesystem (void)
{
	// Register our commands and cvars
	Cmd_AddCommand("path", FS_Path_f);
	Cmd_AddCommand("link", FS_Link_f);
	Cmd_AddCommand("dir", FS_Dir_f);

	Com_Printf("\n----- Filesystem Initialization -----\n");

	// basedir <path>
	// allows the game to run from outside the data tree
	fs_basedir = Cvar_Get ("basedir", ".", CVAR_NOSET);

	// cddir <path>
	// Logically concatenates the cddir after the basedir for 
	// allows the game to run from outside the data tree
	fs_cddir = Cvar_Get("cddir", "", CVAR_NOSET);
	if (fs_cddir->string[0])
		FS_AddGameDirectory (va("%s/"BASEDIRNAME, fs_cddir->string) );

	// start up with baseq2 by default
	FS_AddGameDirectory (va("%s/"BASEDIRNAME, fs_basedir->string) );

	// any set gamedirs will be freed up to here
	fs_baseSearchPaths = fs_searchPaths;

	strcpy(fs_currentGame, BASEDIRNAME);

	// check for game override
	fs_homepath = Cvar_Get("homepath", Sys_GetCurrentDirectory(), CVAR_NOSET);
	fs_debug = Cvar_Get("fs_debug", "0", 0);
	fs_gamedirvar = Cvar_Get ("game", "", CVAR_LATCH|CVAR_SERVERINFO);
	if (fs_gamedirvar->string[0])
		FS_SetGamedir (fs_gamedirvar->string);

	FS_Path_f(); // output path data
}

/*
=================
FS_Shutdown
=================
*/
void FS_Shutdown (void)
{
	fsHandle_t		*handle;
	fsSearchPath_t	*next;
	fsPack_t		*pack;
	int32_t				i;

	Cmd_RemoveCommand("dir");
	//Cmd_RemoveCommand("fdir");
	Cmd_RemoveCommand("link");
	Cmd_RemoveCommand("path");

	// Close all files
	for (i = 0, handle = fs_handles; i < MAX_HANDLES; i++, handle++)
	{
		if (handle->file)
			fclose(handle->file);
		if (handle->zip)
		{
			unzCloseCurrentFile(handle->zip);
			unzClose(handle->zip);
		}
	}

	// Free the search paths
	while (fs_searchPaths)
	{
		if (fs_searchPaths->pack)
		{
			pack = fs_searchPaths->pack;

			if (pack->pak)
				fclose(pack->pak);
			if (pack->pk3)
				unzClose(pack->pk3);

			Z_Free(pack->files);
			Z_Free(pack);
		}
		next = fs_searchPaths->next;
		Z_Free(fs_searchPaths);
		fs_searchPaths = next;
	}
}


/*
================
FS_SetGamedir

Sets the gamedir and path to a different directory.
================
*/
void FS_SetGamedir (char *dir)
{
	fsSearchPath_t	*next;

	if (strstr(dir, "..") || strstr(dir, "/")
		|| strstr(dir, "\\") || strstr(dir, ":") )
	{
		Com_Printf ("Gamedir should be a single filename, not a path\n");
		return;
	}

	//
	// free up any current game dir info
	//
	while (fs_searchPaths != fs_baseSearchPaths)
	{
		if (fs_searchPaths->pack)
		{
			if (fs_searchPaths->pack->pak)
				fclose(fs_searchPaths->pack->pak);
			if (fs_searchPaths->pack->pk3)
				unzClose(fs_searchPaths->pack->pk3);
			Z_Free (fs_searchPaths->pack->files);
			Z_Free (fs_searchPaths->pack);
		}
		next = fs_searchPaths->next;
		Z_Free (fs_searchPaths);
		fs_searchPaths = next;
	}

	//
	// flush all data, so it will be forced to reload
	//
	if (dedicated && !dedicated->value)
		Cbuf_AddText ("vid_restart\nsnd_restart\n");

	Com_sprintf (fs_gamedir, sizeof(fs_gamedir), "%s/%s", fs_basedir->string, dir);

	if (!strcmp(dir,BASEDIRNAME) || (*dir == 0))
	{
		Cvar_FullSet ("gamedir", "", CVAR_SERVERINFO|CVAR_NOSET);
		Cvar_FullSet ("game", "", CVAR_LATCH|CVAR_SERVERINFO);
	}
	else
	{
		Cvar_FullSet ("gamedir", dir, CVAR_SERVERINFO|CVAR_NOSET);
		if (fs_cddir->string[0])
			FS_AddGameDirectory (va("%s/%s", fs_cddir->string, dir) );
		FS_AddGameDirectory (va("%s/%s", fs_basedir->string, dir) );
	}
}


/*
================
FS_Link_f

Creates a filelink_t
================
*/
void FS_Link_f (void)
{
	fsLink_t	*l, **prev;

	if (Cmd_Argc() != 3)
	{
		Com_Printf ("USAGE: link <from> <to>\n");
		return;
	}

	// see if the link already exists
	prev = &fs_links;
	for (l=fs_links ; l ; l=l->next)
	{
		if (!strcmp (l->from, Cmd_Argv(1)))
		{
			Z_Free (l->to);
			if (!strlen(Cmd_Argv(2)))
			{	// delete it
				*prev = l->next;
				Z_Free (l->from);
				Z_Free (l);
				return;
			}
			l->to = CopyString (Cmd_Argv(2));
			return;
		}
		prev = &l->next;
	}

	// create a new link
	l = Z_Malloc(sizeof(*l));
	l->next = fs_links;
	fs_links = l;
	l->from = CopyString(Cmd_Argv(1));
	l->length = strlen(l->from);
	l->to = CopyString(Cmd_Argv(2));
}


/*
=============
FS_ExecAutoexec
=============
*/
void FS_ExecAutoexec (void)
{
	char *dir;
	char name [MAX_QPATH];

	dir = Cvar_VariableString("gamedir");
	if (*dir)
		Com_sprintf(name, sizeof(name), "%s/%s/autoexec.cfg", fs_basedir->string, dir); 
	else
		Com_sprintf(name, sizeof(name), "%s/%s/autoexec.cfg", fs_basedir->string, BASEDIRNAME); 
	if (Sys_FindFirst(name, 0, SFF_SUBDIR | SFF_HIDDEN | SFF_SYSTEM))
		Cbuf_AddText ("exec autoexec.cfg\n");
	Sys_FindClose();
}

/*
================
FS_ListFiles
================
*/
char **FS_ListFiles (char *findname, int32_t *numfiles, unsigned musthave, unsigned canthave)
{
	char *s;
	int32_t nfiles = 0;
	char **list = 0;

	s = Sys_FindFirst( findname, musthave, canthave );
	while ( s )
	{
		if ( s[strlen(s)-1] != '.' )
			nfiles++;
		s = Sys_FindNext( musthave, canthave );
	}
	Sys_FindClose ();

	if ( !nfiles ) {
		*numfiles = 0;
		return NULL;
	}

	nfiles++; // add space for a guard
	*numfiles = nfiles;

	list = malloc( sizeof( char * ) * nfiles );
	if (list)
	{
		memset( list, 0, sizeof( char * ) * nfiles );

		s = Sys_FindFirst( findname, musthave, canthave );
		nfiles = 0;
		while ( s )
		{
			if ( s[strlen(s)-1] != '.' )
			{
				list[nfiles] = strdup( s );
#ifdef _WIN32
				strlwr( list[nfiles] );
#endif
				nfiles++;
			}
			s = Sys_FindNext( musthave, canthave );
		}
		Sys_FindClose ();
	}
	return list;
}

/*
 * Compare file attributes (musthave and canthave) in packed files. If
 * "output" is not NULL, "size" is greater than zero and the file matches the
 * attributes then a copy of the matching string will be placed there (with
 * SFF_SUBDIR it changes).
 */
qboolean
ComparePackFiles(const char *findname, const char *name, unsigned musthave,
		unsigned canthave, char *output, int size)
{
	qboolean retval;
	char *ptr;
	char buffer[MAX_OSPATH];

	strncpy(buffer, name, sizeof(buffer));

	if ((canthave & SFF_SUBDIR) && (name[strlen(name) - 1] == '/'))
	{
		return false;
	}

	if (musthave & SFF_SUBDIR)
	{
		if ((ptr = strrchr(buffer, '/')) != NULL)
		{
			*ptr = '\0';
		}
		else
		{
			return false;
		}
	}

	if ((musthave & SFF_HIDDEN) || (canthave & SFF_HIDDEN))
	{
		if ((ptr = strrchr(buffer, '/')) == NULL)
		{
			ptr = buffer;
		}

		if (((musthave & SFF_HIDDEN) && (ptr[1] != '.')) ||
			((canthave & SFF_HIDDEN) && (ptr[1] == '.')))
		{
			return false;
		}
	}

	if (canthave & SFF_RDONLY)
	{
		return false;
	}

	retval = glob_match((char *)findname, buffer);

	if (retval && (output != NULL))
	{
		strncpy(output, buffer, size);
	}

	return retval;
}

/*
 * Create a list of files that match a criteria.
 * Searchs are relative to the game directory and use all the search paths
 * including .pak and .pk3 files.
 */
char **
FS_ListFiles2(char *findname, int *numfiles,
		unsigned musthave, unsigned canthave)
{
	fsSearchPath_t *search; /* Search path. */
	int i, j; /* Loop counters. */
	int nfiles; /* Number of files found. */
	int tmpnfiles; /* Temp number of files. */
	char **tmplist; /* Temporary list of files. */
	char **list; /* List of files found. */
	char path[MAX_OSPATH]; /* Temporary path. */

	nfiles = 0;
	list = malloc(sizeof(char *));

	for (search = fs_searchPaths; search != NULL; search = search->next)
	{
		if (search->pack != NULL)
		{
			for (i = 0, j = 0; i < search->pack->numFiles; i++)
			{
				if (ComparePackFiles(findname, search->pack->files[i].name,
							musthave, canthave, NULL, 0))
				{
					j++;
				}
			}

			if (j == 0)
			{
				continue;
			}

			nfiles += j;
			list = realloc(list, nfiles * sizeof(char *));

			for (i = 0, j = nfiles - j; i < search->pack->numFiles; i++)
			{
				if (ComparePackFiles(findname, search->pack->files[i].name,
							musthave, canthave, path, sizeof(path)))
				{
					list[j++] = strdup(path);
				}
			}
		}
		else if (search->path != NULL)
		{

			Com_sprintf(path, sizeof(path), "%s/%s", search->path, findname);
			tmplist = FS_ListFiles(path, &tmpnfiles, musthave, canthave);

			if (tmplist != NULL)
			{
				tmpnfiles--;
				nfiles += tmpnfiles;
				list = realloc(list, nfiles * sizeof(char *));

				for (i = 0, j = nfiles - tmpnfiles; i < tmpnfiles; i++, j++)
				{
					list[j] = strdup(tmplist[i] + strlen(search->path) + 1);
				}

				FS_FreeFileList(tmplist, tmpnfiles + 1);
			}
		}
	}

	/* Delete duplicates. */
	tmpnfiles = 0;

	for (i = 0; i < nfiles; i++)
	{
		if (list[i] == NULL)
		{
			continue;
		}

		for (j = i + 1; j < nfiles; j++)
		{
			if ((list[j] != NULL) &&
				(strcmp(list[i], list[j]) == 0))
			{
				free(list[j]);
				list[j] = NULL;
				tmpnfiles++;
			}
		}
	}

	if (tmpnfiles > 0)
	{
		nfiles -= tmpnfiles;
		tmplist = malloc(nfiles * sizeof(char *));

		for (i = 0, j = 0; i < nfiles + tmpnfiles; i++)
		{
			if (list[i] != NULL)
			{
				tmplist[j++] = list[i];
			}
		}

		free(list);
		list = tmplist;
	}

	/* Add a guard. */
	if (nfiles > 0)
	{
		nfiles++;
		list = realloc(list, nfiles * sizeof(char *));
		list[nfiles - 1] = NULL;
	}

	else
	{
		free(list);
		list = NULL;
	}

	*numfiles = nfiles;

	return list;
}
/*
=================
FS_FreeFileList
=================
*/
void FS_FreeFileList (char **list, int32_t n)
{
	int32_t i;

	for (i = 0; i < n; i++)
	{
		if (list && list[i])
		{
			free(list[i]);
			list[i] = 0;
		}
	}
	free(list);
}

/*
=================
FS_ItemInList
=================
*/
qboolean FS_ItemInList (char *check, int32_t num, char **list)
{
	int32_t i;
	for (i=0;i<num;i++)
		if (!Q_strcasecmp(check, list[i]))
			return true;
	return false;
}

/*
=================
FS_InsertInList
=================
*/
void FS_InsertInList (char **list, char *insert, int32_t len, int32_t start)
{
	int32_t i;
	if (!list) return;

	for (i=start; i<len; i++)
	{
		if (!list[i])
		{
			list[i] = strdup(insert);
			return;
		}
	}
	list[len] = strdup(insert);
}

/*
================
FS_Dir_f
================
*/
void FS_Dir_f (void)
{
	char	*path = NULL;
	char	findname[1024];
	char	wildcard[1024] = "*.*";
	char	**dirnames;
	int32_t		ndirs;

	if ( Cmd_Argc() != 1 )
	{
		strcpy( wildcard, Cmd_Argv( 1 ) );
	}

	while ( ( path = FS_NextPath( path ) ) != NULL )
	{
		char *tmp = findname;

		Com_sprintf( findname, sizeof(findname), "%s/%s", path, wildcard );

		while ( *tmp != 0 )
		{
			if ( *tmp == '\\' ) 
				*tmp = '/';
			tmp++;
		}
		Com_Printf( "Directory of %s\n", findname );
		Com_Printf( "----\n" );

		if ( ( dirnames = FS_ListFiles( findname, &ndirs, 0, 0 ) ) != 0 )
		{
			int32_t i;

			for ( i = 0; i < ndirs-1; i++ )
			{
				if ( strrchr( dirnames[i], '/' ) )
					Com_Printf( "%s\n", strrchr( dirnames[i], '/' ) + 1 );
				else
					Com_Printf( "%s\n", dirnames[i] );

				free( dirnames[i] );
			}
			free( dirnames );
		}
		Com_Printf( "\n" );
	};
}

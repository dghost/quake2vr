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
#include "zip/unzip.h"
#include <sys/stat.h>

#ifndef _WIN32
#define stricmp strcasecmp
#define strnicmp strncasecmp
#else
#define stat _stat
#define S_ISDIR(mode)  (((mode) & _S_IFMT) == _S_IFDIR)
#endif

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
	char			*to;
	struct fsLink_s	*next;
    int32_t			length;
} fsLink_t;

typedef struct {
    const char      *name;
	int32_t         hash;				// To speed up searching
	int32_t			size;
	int32_t			offset;				// This is ignored in PK3 files
	qboolean		ignore;				// Whether this file should be ignored
} fsPackFile_t;

typedef struct {
	char			name[MAX_OSPATH];
    stable_t        fileNames;
	FILE			*pak;
	unzFile			*pk3;
	int32_t			numFiles;
	uint32_t        contentFlags;
    fsPackFile_t	*files;
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

cvar_t	*fs_basedir;
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
	"frag",
	"vert",
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
    Q_strlwr(extension);
	for (i=0; type_extensions[i]; i++) {
		if ( !strcmp(extension, type_extensions[i]) )
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
FS_Gamedir

Called to find where to write a file (demos, savegames, etc...)
=================
*/
const char *FS_Gamedir (void)
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
    return NULL; // not called
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

/*
=================
FS_FindPackItem

Performs a binary search by hashed filename
to find pack items in a sorted pack
=================
*/
int32_t FS_FindPackItem (fsPack_t *pack, char *itemName)
{
    char buffer[MAX_OSPATH];
	int32_t		smax, smin, smidpt;	//, counter = 0;
	int32_t		i;	//, matchStart, matchEnd;
    int32_t     itemIndex = -1;
    int32_t     numLookups = 0;
    int32_t     overlap = 0;
	// catch null pointers
	if ( !pack || !itemName)
        return -1;
    
    Q_strlcpy_lower(buffer, itemName, MAX_OSPATH);

    itemIndex = Q_STLookup(&pack->fileNames, buffer);
    
    if (itemIndex < 0)
		return -1;
    
	smin = 0;	smax = pack->numFiles;
	while ( (smax - smin) > 1 )	//&& counter < pack->numFiles )
	{

		smidpt = (int) ((smax + smin) / 2.0);

		if (pack->files[smidpt].hash > itemIndex)	// before midpoint
			smax = smidpt;
		else if (pack->files[smidpt].hash < itemIndex)	// after midpoint
			smin = smidpt;
        else {
            smin = smidpt;
            smax = smidpt;
        }
		numLookups++;
	}
	for (i=smin; i<=smax; i++)
	{	// make sure this entry is not blacklisted & compare filenames
		if ( (pack->files[i].hash == itemIndex) && !pack->files[i].ignore) {
			return i;
        }
        overlap++;
	}
	return -1;
}

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
	int32_t				i;
	uint32_t	typeFlag;

	// Knightmare- hack global vars for autodownloads
	file_from_pak = 0;
	file_from_pk3 = 0;
	Com_sprintf(last_pk3_name, sizeof(last_pk3_name), "\0");

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

            // find index of pack item
			i = FS_FindPackItem (pack, handle->name);
            // found it!
            if ( i != -1 && i >= 0 && i < pack->numFiles )
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
                    handle->zip = (unzFile*)unzOpen(pack->name);
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
int32_t FS_ListPak (char *find, sset_t *output)
{
    fsSearchPath_t	*search;
    //char			netpath[MAX_OSPATH];
    fsPack_t		*pak;
    
    int32_t nfound = 0;
    int32_t i;
    
    
    for (search = fs_searchPaths; search; search = search->next)
    {
        if (!search->pack)
            continue;
        
        pak = search->pack;
        
        // now find and build list
        for (i=0 ; i<pak->numFiles ; i++)
        {
            const char *name = pak->files[i].name;
            if (!pak->files[i].ignore && strstr(name, find))
            {
                Q_SSetInsert(output, name);
                nfound++;
            }
        }
    }
    
    return nfound;
}

/*
=================
FS_ListPakOld

Generates a listing of the contents of a pak file
Only provided to maintain backwards compatibility with KMQ2
=================
*/
char **FS_ListPakOld (char *find, int32_t *num)
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

	list =  (char **) Z_TagMalloc( sizeof( char * ) * nfiles , TAG_SYSTEM);

	for (search = fs_searchPaths; search; search = search->next)
	{
		if (!search->pack)
			continue;

		pak = search->pack;

		// now find and build list
		for (i=0 ; i<pak->numFiles ; i++)
		{
            const char *name = pak->files[i].name;
			if (!pak->files[i].ignore && strstr(name, find))
			{
				list[nfound] = (char*)Z_TagStrdup(name, TAG_SYSTEM);
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
    return -1;
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
int32_t FS_LoadFile (const char *path, void **buffer)
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
	buf = (byte*)Z_TagMalloc(size, TAG_SYSTEM);
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
char* pakfile_ignore_names[] =
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
    stable_t        filenameTable = {0, 0, 0};
	fsPackFile_t	*files;
	fsPack_t		*pack;
	FILE			*handle;
	dpackheader_t		header;
	dpackfile_t		info[MAX_FILES_IN_PACK];
	uint32_t		contentFlags = 0;

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
    Q_STInit(&filenameTable, numFiles * MAX_QPATH, MAX_QPATH, TAG_SYSTEM);

    files = (fsPackFile_t*)Z_TagMalloc(numFiles * sizeof(fsPackFile_t), TAG_SYSTEM);

	fseek(handle, header.dirofs, SEEK_SET);
	fread(info, 1, header.dirlen, handle);

	for (i = 0; i < numFiles; i++)
	{
        char buffer[MAX_OSPATH];
        dpackfile_t cur = info[i];
        Q_strlcpy_lower(buffer, cur.name, MAX_OSPATH);
        files[i].hash = Q_STAutoRegister(&filenameTable, buffer);
        files[i].name = Q_STGetString(&filenameTable, files[i].hash);
        files[i].offset = LittleLong(cur.filepos);
        files[i].size = LittleLong(cur.filelen);
        files[i].ignore = FS_FileInPakBlacklist(buffer, false);	// check against pak loading blacklist
        if (!files[i].ignore)	// add type flag for this file
            contentFlags |= FS_TypeFlagForPakItem(buffer);
	}

    //Com_Printf("Registering file %s\n", packPath);
    //Com_Printf("String table contains %i bytes\n", filenameTable.size);
    Q_STAutoPack(&filenameTable);
    //Com_Printf("Packed string table contains %i bytes\n", filenameTable.size);

    pack = (fsPack_t*)Z_TagMalloc(sizeof(fsPack_t), TAG_SYSTEM);
	strcpy(pack->name, packPath);
	pack->pak = handle;
	pack->pk3 = NULL;
	pack->numFiles = numFiles;
	pack->files = files;
	pack->contentFlags = contentFlags;
    pack->fileNames = filenameTable;

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
    search = (fsSearchPath_t*)Z_TagMalloc (sizeof(fsSearchPath_t), TAG_SYSTEM);
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
	uint32_t		contentFlags = 0;
	char			fileName[MAX_QPATH];
    stable_t        filenameTable = {0, 0, 0};

	handle = (unzFile*)unzOpen(packPath);
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
	files = (fsPackFile_t*)Z_TagMalloc(numFiles * sizeof(fsPackFile_t), TAG_SYSTEM);

    Q_STInit(&filenameTable, numFiles * MAX_QPATH, MAX_QPATH, TAG_SYSTEM);
    
    
	status = unzGoToFirstFile(handle);
	while (status == UNZ_OK)
	{
        char buffer[MAX_OSPATH];

		fileName[0] = 0;
		unzGetCurrentFileInfo(handle, &info, fileName, MAX_QPATH, NULL, 0, NULL, 0);

		Q_strlcpy_lower(buffer, fileName, MAX_OSPATH);

        files[i].hash = Q_STAutoRegister(&filenameTable, buffer);
        files[i].name = Q_STGetString(&filenameTable, files[i].hash);
		files[i].offset = -1;		// Not used in ZIP files
		files[i].size = info.uncompressed_size;
		files[i].ignore = FS_FileInPakBlacklist(buffer, true);	// check against pak loading blacklist
		if (!files[i].ignore)	// add type flag for this file
			contentFlags |= FS_TypeFlagForPakItem(buffer);
		i++;

		status = unzGoToNextFile(handle);
	}

	pack = (fsPack_t*)Z_TagMalloc(sizeof(fsPack_t), TAG_SYSTEM);
	strcpy(pack->name, packPath);
	pack->pak = NULL;
	pack->pk3 = handle;
	pack->numFiles = numFiles;
	pack->files = files;
	pack->contentFlags = contentFlags;
    pack->fileNames = filenameTable;
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
	search = (fsSearchPath_t*)Z_TagMalloc(sizeof(fsSearchPath_t), TAG_SYSTEM);
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
//	char *path = NULL;
	char findname[1024];
	char *tmp;
	// VoiD -E- *.pak support

	strcpy(fs_gamedir, dir);

	//
	// Add the directory to the search path
	//
	search = (fsSearchPath_t*)Z_TagMalloc(sizeof(fsSearchPath_t), TAG_SYSTEM);
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

    Com_sprintf (packPath, sizeof(packPath), "%s/vrquake2.pk3", dir, i);
    FS_AddPK3File (packPath);

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
        sset_t dirs;
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
        
        if (Q_SSetInit(&dirs, 50, MAX_OSPATH, TAG_SYSTEM)) {
            if ((FS_ListFiles( findname, &dirs, 0, 0 ) ) )
            {
                for ( j=0; j < dirs.currentSize; j++ )
                {	// don't reload numbered pak files
                    const char *dirname = Q_SSetGetString(&dirs, j);
                    int32_t		k;
                    char	buf[16];
                    char	buf2[16];
                    qboolean numberedpak = false;
                    
                    if (strstr(dirname, "/vrquake2.pk3"))
                        continue;
                    
                    for (k=0; k<100; k++)
                    {
                        Com_sprintf( buf, sizeof(buf), "/pak%i.pak", k);
                        Com_sprintf( buf2, sizeof(buf2), "/pak%i.pk3", k);
                        if ( strstr(dirname, buf) || strstr(dirname, buf2)) {
                            numberedpak = true;
                            break;
                        }
                    }
                    if (numberedpak)
                        continue;
                    if ( strrchr( dirname, '/' ) )
                    {
                        if (i==1)
                            FS_AddPK3File (dirname);
                        else
                            FS_AddPAKFile (dirname);
                    }
                }
            }
            Q_SSetFree(&dirs);
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
	char			*prev = NULL;

	if (!prevPath)
		return fs_gamedir;

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
	if (Q_strcasecmp(fs_gamedirvar->string, fs_currentGame))
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

		if (!Q_strcasecmp(fs_gamedirvar->string, BASEDIRNAME))	// Don't add baseq2 again
			strcpy(fs_gamedir, fs_basedir->string);
		else
		{
			// Add the directories
			FS_AddGameDirectory(va("%s/%s", fs_gamedirvar->string, fs_gamedirvar->string));
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
char *Sys_GetBaseDir (void);
void FS_InitFilesystem (void)
{
	// Register our commands and cvars
	Cmd_AddCommand("path", FS_Path_f);
	Cmd_AddCommand("link", FS_Link_f);
	Cmd_AddCommand("dir", FS_Dir_f);
    
	Com_Printf("\n----- Filesystem Initialization -----\n");

	// basedir <path>
	// allows the game to run from outside the data tree
	fs_basedir = Cvar_Get ("basedir", Sys_GetBaseDir(), CVAR_NOSET);

	// start up with baseq2 by default
	FS_AddGameDirectory (va("%s/"BASEDIRNAME, fs_basedir->string) );

	// any set gamedirs will be freed up to here
	fs_baseSearchPaths = fs_searchPaths;

	strcpy(fs_currentGame, BASEDIRNAME);

	// check for game override
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
            
            Q_STFree(&pack->fileNames);
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
void FS_SetGamedir (const char *dir)
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
			l->to = (char*)Z_TagStrdup (Cmd_Argv(2), TAG_SYSTEM);
			return;
		}
		prev = &l->next;
	}

	// create a new link
	l = (fsLink_t*)Z_TagMalloc(sizeof(*l), TAG_SYSTEM);
	l->next = fs_links;
	fs_links = l;
	l->from = (char*)Z_TagStrdup(Cmd_Argv(1), TAG_SYSTEM);
	l->length = strlen(l->from);
	l->to = (char*)Z_TagStrdup(Cmd_Argv(2), TAG_SYSTEM);
}


/*
=============
FS_ExecAutoexec
=============
*/
void FS_ExecAutoexec (void)
{
	const char *dir;
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
int32_t FS_ListFiles (char *findname, sset_t *ss, uint32_t musthave, uint32_t canthave)
{
    assert(ss != NULL);

	char *s;
    uint32_t numFiles = 0;


    s = Sys_FindFirst( findname, musthave, canthave );
    while ( s )
    {
        if ( s[strlen(s)-1] != '.' ) {
            numFiles++;
            
#ifdef _WIN32
            Q_strlwr(s);
#endif
            Q_SSetInsert(ss, s);
            
        }
        s = Sys_FindNext( musthave, canthave );
    }
    Sys_FindClose ();
    
    return numFiles;
}

/*
 ================
 FS_ListFilesRelative
 ================
 */
int32_t FS_ListFilesRelative (const char *path, const char *pattern, sset_t *ss, uint32_t musthave, uint32_t canthave)
{
    assert(ss != NULL);
    char findname[MAX_OSPATH]; /* Temporary path. */

    char *s;
    uint32_t numFiles = 0;
    int32_t len = strlen(path);
    Com_sprintf(findname, sizeof(findname), "%s/%s", path, pattern);
    
    s = Sys_FindFirst( findname, musthave, canthave );
    while ( s )
    {
        if ( s[strlen(s)-1] != '.' ) {
            numFiles++;
            
#ifdef _WIN32
            Q_strlwr(s);
#endif
            Q_SSetInsert(ss, s + len + 1);
            
        }
        s = Sys_FindNext( musthave, canthave );
    }
    Sys_FindClose ();
    
    return numFiles;
}

/*
 ================
 FS_CountFiles
 ================
 */
int32_t FS_CountFiles (char *findname, uint32_t musthave, uint32_t canthave)
{
    char *s;
    uint32_t numFiles = 0;
    
    
    s = Sys_FindFirst( findname, musthave, canthave );
    while ( s )
    {
        if ( s[strlen(s)-1] != '.' ) {
            numFiles++;
        }
        s = Sys_FindNext( musthave, canthave );
    }
    Sys_FindClose ();
    
    return numFiles;
}

/*
 * Compare file attributes (musthave and canthave) in packed files. If
 * "output" is not NULL, "size" is greater than zero and the file matches the
 * attributes then a copy of the matching string will be placed there (with
 * SFF_SUBDIR it changes).
 */
qboolean
ComparePackFiles(const char *findname, const char *name, uint32_t musthave,
		uint32_t canthave, char *output, int size)
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
int32_t
FS_ListFilesWithPaks(char *findname, sset_t *output,
		uint32_t musthave, uint32_t canthave)
{
	fsSearchPath_t *search; /* Search path. */
	int i; /* Loop counters. */
	int nfiles; /* Number of files found. */
	char path[MAX_OSPATH]; /* Temporary path. */

	nfiles = 0;
    
    for (search = fs_searchPaths; search != NULL; search = search->next)
    {
        if (search->pack != NULL)
        {
            fsPackFile_t *files = search->pack->files;
            int numFiles = search->pack->numFiles;
            for (i = 0; i < numFiles; i++)
            {
                if (ComparePackFiles(findname, files[i].name,
                                     musthave, canthave, path, sizeof(path)))
                {
                    nfiles++;
                    Q_SSetInsert(output, path);
                }
            }
        }
        else if (search->path[0] != 0)
        {
            nfiles += FS_ListFilesRelative(search->path, findname, output, musthave, canthave);
        }
        
    }
    return nfiles;
}

int32_t
FS_CountFilesWithPaks(char *findname, uint32_t musthave, uint32_t canthave)
{
    fsSearchPath_t *search; /* Search path. */
    int i; /* Loop counters. */
    int nfiles = 0; /* Number of files found. */
    char path[MAX_OSPATH]; /* Temporary path. */
    
    for (search = fs_searchPaths; search != NULL; search = search->next)
    {
        if (search->pack != NULL)
        {
            fsPackFile_t *files = search->pack->files;
            int numFiles = search->pack->numFiles;
            for (i = 0; i < numFiles; i++)
            {
                if (ComparePackFiles(findname, files[i].name,
                                     musthave, canthave, path, sizeof(path)))
                {
                    nfiles++;
                }
            }
        }
        else if (search->path[0] != 0)
        {
            Com_sprintf(path, sizeof(path), "%s/%s", search->path, findname);
            nfiles += FS_CountFiles(path, musthave, canthave);
        }
        
    }
    return nfiles;
}
/*
=================
FS_FreeFileList
=================
*/
void FS_FreeFileList (char **list, int32_t n)
{
	int32_t i;

    if (list) {
        for (i = 0; i < n; i++)
        {
            if (list[i])
            {
                Z_Free(list[i]);
                list[i] = 0;
            }
        }
        Z_Free(list);
    }
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
			list[i] = (char*)Z_TagStrdup(insert, TAG_SYSTEM);
			return;
		}
	}
	list[len] = (char*)Z_TagStrdup(insert, TAG_SYSTEM);
}

static qboolean FS_IsDirectory(const char *path) {
    struct stat result;
    if (!stat(path, &result)) {
        return S_ISDIR(result.st_mode);
    }
    return false;
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

	if ( Cmd_Argc() != 1 )
	{
		strcpy( wildcard, Cmd_Argv( 1 ) );
	}
    
	while ( ( path = FS_NextPath( path ) ) != NULL )
	{
		char *tmp = findname;
        sset_t dirs;
		Com_sprintf( findname, sizeof(findname), "%s/%s", path, wildcard );

		while ( *tmp != 0 )
		{
			if ( *tmp == '\\' ) 
				*tmp = '/';
			tmp++;
		}
        Com_Printf( "Directory of %s\n", findname );
        Com_Printf( "----\n" );
        if (Q_SSetInit(&dirs, 50, MAX_OSPATH, TAG_SYSTEM)) {
            
            if ( FS_ListFiles( findname, &dirs, 0, 0 ))
            {
                int32_t i, numDirs;
                const char	**dirnames = Q_SSetMakeStrings(&dirs, &numDirs);

                for ( i = 0; i < numDirs; i++ )
                {
                    if (FS_IsDirectory(dirnames[i])) {
                        const char *name = strrchr(dirnames[i], '/');
                        if (name) {
                            name += 1;
                        } else {
                            name = dirnames[i];
                        }
                        Com_Printf( S_COLOR_CYAN "%s/\n", name );
                        dirnames[i] = NULL;
                    }
                }
                for ( i = 0; i < numDirs; i++ )
                {
                    if (dirnames[i]) {
                        const char *name = strrchr(dirnames[i], '/');
                        if (name) {
                            name += 1;
                        } else {
                            name = dirnames[i];
                        }
                        if (name[0] != '.')
                            Com_Printf( S_COLOR_WHITE "%s\n", name );
                    }
                }
                Z_Free( dirnames );
            }
            Q_SSetFree(&dirs);
        }
        Com_Printf( "\n" );
    };
}

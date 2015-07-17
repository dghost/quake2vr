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

// qcommon.h -- definitions common between client and server, but not game.dll
#ifndef __QCOMMON_H
#define __QCOMMON_H

#include "shared/q_shared.h"
#include "glob.h"

#define	VERSION	"4.2" //was 3.21
#define VR_VER "1.9.3-pre"

#define	BASEDIRNAME	"baseq2"

#define DEFAULTPAK			"pak"
#define DEFAULTMODEL		"male"
#define DEFAULTSKIN			"grunt"

#ifdef _WIN32
#define BUILDSTRING "Windows"
#elif defined(__linux__)
#define BUILDSTRING "Linux"
#elif defined(__APPLE__)
#define BUILDSTRING "OS X"
#else
#define BUILDSTRING "Unknown OS"
#endif

#if defined(_M_IX86) || defined(__i386__)
#define	CPUSTRING	"x86"
#elif defined(_M_X64) || defined(__x86_64__)
#define CPUSTRING   "x86_64"
#else
#define CPUSTRING "Unknown"
#endif

//============================================================================

typedef struct sizebuf_s
{
	qboolean	allowoverflow;	// if false, do a Com_Error
	qboolean	overflowed;		// set to true if the buffer size failed
	byte	*data;
	int32_t		maxsize;
	int32_t		cursize;
	int32_t		readcount;
} sizebuf_t;

void SZ_Init (sizebuf_t *buf, byte *data, int32_t length);
void SZ_Clear (sizebuf_t *buf);
void *SZ_GetSpace (sizebuf_t *buf, int32_t length);
void SZ_Write (sizebuf_t *buf, const void *data, int32_t length);
void SZ_Print (sizebuf_t *buf, const char *data);	// strcats onto the sizebuf

//============================================================================

struct usercmd_s;
struct entity_state_s;

void MSG_WriteChar (sizebuf_t *sb, int32_t c);
void MSG_WriteByte (sizebuf_t *sb, int32_t c);
void MSG_WriteShort (sizebuf_t *sb, int32_t c);
void MSG_WriteLong (sizebuf_t *sb, int32_t c);
void MSG_WriteFloat (sizebuf_t *sb, float f);
void MSG_WriteString (sizebuf_t *sb, const char *s);
void MSG_WriteCoord (sizebuf_t *sb, float f);
void MSG_WritePos (sizebuf_t *sb, vec3_t pos);
void MSG_WriteAngle (sizebuf_t *sb, float f);
void MSG_WriteAngle16 (sizebuf_t *sb, float f);
void MSG_WriteDeltaUsercmd (sizebuf_t *sb, struct usercmd_s *from, struct usercmd_s *cmd);
void MSG_WriteDeltaEntity (struct entity_state_s *from, struct entity_state_s *to, sizebuf_t *msg, qboolean force, qboolean newentity);
void MSG_WriteDir (sizebuf_t *sb, vec3_t vector);


void	MSG_BeginReading (sizebuf_t *sb);

int32_t		MSG_ReadChar (sizebuf_t *sb);
int32_t		MSG_ReadByte (sizebuf_t *sb);
int32_t		MSG_ReadShort (sizebuf_t *sb);
int32_t		MSG_ReadLong (sizebuf_t *sb);
float	MSG_ReadFloat (sizebuf_t *sb);
char	*MSG_ReadString (sizebuf_t *sb);
char	*MSG_ReadStringLine (sizebuf_t *sb);

float	MSG_ReadCoord (sizebuf_t *sb);
void	MSG_ReadPos (sizebuf_t *sb, vec3_t pos);
float	MSG_ReadAngle (sizebuf_t *sb);
float	MSG_ReadAngle16 (sizebuf_t *sb);
void	MSG_ReadDeltaUsercmd (sizebuf_t *sb, struct usercmd_s *from, struct usercmd_s *cmd);

void	MSG_ReadDir (sizebuf_t *sb, vec3_t vector);

void	MSG_ReadData (sizebuf_t *sb, void *buffer, int32_t size);

#ifdef LARGE_MAP_SIZE // 24-bit pmove origin coordinate transmission code
void	MSG_WritePMCoordNew (sizebuf_t *sb, int32_t in);
int32_t		MSG_ReadPMCoordNew (sizebuf_t *msg_read);
#endif

//============================================================================

extern	qboolean		bigendien;
extern  uint32_t        sys_cacheline;
extern	int16_t	BigShort (int16_t l);
extern	int16_t	LittleShort (int16_t l);
extern	int32_t		BigLong (int32_t l);
extern	int32_t		LittleLong (int32_t l);
extern	float	BigFloat (float l);
extern	float	LittleFloat (float l);

//============================================================================


int32_t	COM_Argc (void);
char *COM_Argv (int32_t arg);	// range and null checked
void COM_ClearArgv (int32_t arg);
int32_t COM_CheckParm (char *parm);
void COM_AddParm (char *parm);

void COM_Init (void);
void COM_InitArgv (int32_t argc, char **argv);

void Q_strncpyz (char *dst, const char *src, int32_t dstSize);
void Q_strncatz (char *dst, const char *src, int32_t dstSize);

void StripHighBits (char *string, int32_t highbits);
void ExpandNewLines (char *string);

qboolean IsValidChar (int32_t c);
void ExpandNewLines (char *string);
char *StripQuotes (char *string);
const char *MakePrintable (const void *subject, size_t numchars);

//============================================================================

void Info_Print (char *s);


/* crc.h */

void CRC_Init(uint16_t *crcvalue);
void CRC_ProcessByte(uint16_t *crcvalue, byte data);
uint16_t CRC_Value(uint16_t crcvalue);
uint16_t CRC_Block (byte *start, int32_t count);



/*
==============================================================

PROTOCOL

==============================================================
*/

// protocol.h -- communications protocols

#define	PROTOCOL_VERSION		56 // changed from 0.19, was 35
#define	R1Q2_PROTOCOL_VERSION	35
#define	OLD_PROTOCOL_VERSION	34

//=========================================

#define	PORT_MASTER	27900
//#define	PORT_CLIENT	27901
#define PORT_CLIENT    (rand()%11000)+5000 // Knightmare- random port for protection from Q2MSGS
#define	PORT_SERVER	27910

//=========================================
//Knightmare- increase UPDATE_BACKUP to eliminate "U_REMOVE: oldnum != newnum"
#define	UPDATE_BACKUP	64	// copies of entity_state_t to keep buffered
							// must be power of two
//#define	UPDATE_BACKUP	16

#define	UPDATE_MASK		(UPDATE_BACKUP-1)

//==================
// the svc_strings[] array in cl_parse.c should mirror this
//==================

//
// server to client
//
enum svc_ops_e
{
	svc_bad,

	// these ops are known to the game dll
	svc_muzzleflash,
	svc_muzzleflash2,
	svc_temp_entity,
	svc_layout,
	svc_inventory,

	// the rest are private to the client and server
	svc_nop,
	svc_disconnect,
	svc_reconnect,
	svc_sound,					// <see code>
	svc_print,					// [byte] id [string] null terminated string
	svc_stufftext,				// [string] stuffed into client's console buffer, should be \n terminated
	svc_serverdata,				// [long] protocol ...
	svc_configstring,			// [int16_t] [string]
	svc_spawnbaseline,		
	svc_centerprint,			// [string] to put in center of the screen
	svc_download,				// [int16_t] size [size bytes]
	svc_playerinfo,				// variable
	svc_packetentities,			// [...]
	svc_deltapacketentities,	// [...]
	svc_frame,
	svc_fog						// = 21 Knightmare added
};

//==============================================

//
// client to server
//
enum clc_ops_e
{
	clc_bad,
	clc_nop, 		
	clc_move,				// [[usercmd_t]
	clc_userinfo,			// [[userinfo string]
	clc_stringcmd			// [string] message
};

//==============================================

// plyer_state_t communication

#define	PS_M_TYPE			(1<<0)
#define	PS_M_ORIGIN			(1<<1)
#define	PS_M_VELOCITY		(1<<2)
#define	PS_M_TIME			(1<<3)
#define	PS_M_FLAGS			(1<<4)
#define	PS_M_GRAVITY		(1<<5)
#define	PS_M_DELTA_ANGLES	(1<<6)

#define	PS_VIEWOFFSET		(1<<7)
#define	PS_VIEWANGLES		(1<<8)
#define	PS_KICKANGLES		(1<<9)
#define	PS_BLEND			(1<<10)
#define	PS_FOV				(1<<11)
#define	PS_WEAPONINDEX		(1<<12)
#define	PS_WEAPONFRAME		(1<<13)
#define	PS_RDFLAGS			(1<<14)

#define	PS_BBOX				(1<<15)		// for R1Q2 protocol

// Knightmare- bits for sending weapon skin, second weapon model
#define	PS_WEAPONSKIN		(1<<15)
#define	PS_WEAPONINDEX2		(1<<16)
#define	PS_WEAPONFRAME2		(1<<17)
#define	PS_WEAPONSKIN2		(1<<18)
// Knightmare- bits for sending player speed
#define	PS_MAXSPEED			(1<<19)
#define	PS_DUCKSPEED		(1<<20)
#define	PS_WATERSPEED		(1<<21)
#define	PS_ACCEL			(1<<22)
#define	PS_STOPSPEED		(1<<23)
// end Knightmare

//==============================================

// user_cmd_t communication

// ms and light always sent, the others are optional
#define	CM_ANGLE1 	(1<<0)
#define	CM_ANGLE2 	(1<<1)
#define	CM_ANGLE3 	(1<<2)
#define	CM_FORWARD	(1<<3)
#define	CM_SIDE		(1<<4)
#define	CM_UP		(1<<5)
#define	CM_BUTTONS	(1<<6)
#define	CM_IMPULSE	(1<<7)

//==============================================

// a sound without an ent or pos will be a local only sound
#define	SND_VOLUME		(1<<0)		// a byte
#define	SND_ATTENUATION	(1<<1)		// a byte
#define	SND_POS			(1<<2)		// three coordinates
#define	SND_ENT			(1<<3)		// a int16_t 0-2: channel, 3-12: entity
#define	SND_OFFSET		(1<<4)		// a byte, msec offset from frame start

#define DEFAULT_SOUND_PACKET_VOLUME	1.0
#define DEFAULT_SOUND_PACKET_ATTENUATION 1.0

//==============================================

// entity_state_t communication

// try to pack the common update flags into the first byte
#define	U_ORIGIN1	(1<<0)
#define	U_ORIGIN2	(1<<1)
#define	U_ANGLE2	(1<<2)
#define	U_ANGLE3	(1<<3)
#define	U_FRAME8	(1<<4)		// frame is a byte
#define	U_EVENT		(1<<5)
#define	U_REMOVE	(1<<6)		// REMOVE this entity, don't add it
#define	U_MOREBITS1	(1<<7)		// read one additional byte

// second byte
#define	U_NUMBER16	(1<<8)		// NUMBER8 is implicit if not set
#define	U_ORIGIN3	(1<<9)
#define	U_ANGLE1	(1<<10)
#define	U_MODEL		(1<<11)
#define U_RENDERFX8	(1<<12)		// fullbright, etc
#define	U_EFFECTS8	(1<<14)		// autorotate, trails, etc
#define	U_MOREBITS2	(1<<15)		// read one additional byte

// third byte
#define	U_SKIN8		(1<<16)
#define	U_FRAME16	(1<<17)		// frame is a int16_t
#define	U_RENDERFX16 (1<<18)	// 8 + 16 = 32
#define	U_EFFECTS16	(1<<19)		// 8 + 16 = 32
#define	U_MODEL2	(1<<20)		// weapons, flags, etc
#define	U_MODEL3	(1<<21)
#define	U_MODEL4	(1<<22)
#define	U_MOREBITS3	(1<<23)		// read one additional byte

// fourth byte
#define	U_OLDORIGIN	(1<<24)		// FIXME: get rid of this
#define	U_SKIN16	(1<<25)
#define	U_SOUND		(1<<26)
#define	U_SOLID		(1<<27)

// Knightmare- 1/18/2002- bits for extra model indices
#define	U_VELOCITY	(1<<28)	// for R1Q2 protocol
#define	U_MODEL5	(1<<28)		
#define	U_MODEL6	(1<<29)
#define	U_MODEL7_8	(1<<30)	// not enough bits, so we'll fudge this
#define	U_ATTENUAT	(1<<30)	// alternate, sound attenuation
#define	U_ALPHA		(1<<31)	// transparency
// end Knightmare

/*
==============================================================

CMD

Command text buffering and command execution

==============================================================
*/

/*

Any number of commands can be added in a frame, from several different sources.
Most commands come from either keybindings or console line input, but remote
servers can also send across commands and entire text files can be execed.

The + command line options are also added to the command buffer.

The game starts with a Cbuf_AddText ("exec quake.rc\n"); Cbuf_Execute ();

*/

#define	EXEC_NOW	0		// don't return until completed
#define	EXEC_INSERT	1		// insert at current position, but don't run yet
#define	EXEC_APPEND	2		// add to end of the command buffer

void Cbuf_Init (void);
// allocates an initial text buffer that will grow as needed

void Cbuf_AddText (const char *text);
// as new commands are generated from the console or keybindings,
// the text is added to the end of the command buffer.

void Cbuf_InsertText (const char *text);
// when a command wants to issue other commands immediately, the text is
// inserted at the beginning of the buffer, before any remaining unexecuted
// commands.

void Cbuf_ExecuteText (int32_t exec_when, char *text);
// this can be used in place of either Cbuf_AddText or Cbuf_InsertText

void Cbuf_AddEarlyCommands (qboolean clear);
// adds all the +set commands from the command line

qboolean Cbuf_AddLateCommands (void);
// adds all the remaining + commands from the command line
// Returns true if any late commands were added, which
// will keep the demoloop from immediately starting

void Cbuf_Execute (void);
// Pulls off \n terminated lines of text from the command buffer and sends
// them through Cmd_ExecuteString.  Stops when the buffer is empty.
// Normally called once per frame, but may be explicitly invoked.
// Do not call inside a command function!

void Cbuf_CopyToDefer (void);
void Cbuf_InsertFromDefer (void);
// These two functions are used to defer any pending commands while a map
// is being loaded

//===========================================================================

/*

Command execution takes a null terminated string, breaks it into tokens,
then searches for a command or variable that matches the first token.

*/

typedef void (*xcommand_t) (void);

void	Cmd_Init (void);

void	Cmd_AddCommand (char *cmd_name, xcommand_t function);
// called by the init functions of other parts of the program to
// register commands and functions to call for them.
// The cmd_name is referenced later, so it should not be in temp memory
// if function is NULL, the command will be forwarded to the server
// as a clc_stringcmd instead of executed locally
void	Cmd_RemoveCommand (char *cmd_name);

qboolean Cmd_Exists (char *cmd_name);
// used by the cvar code to check for cvar / command name overlap

typedef struct completion_t {
    uint32_t number;
    const char *match;
} completion_t;

completion_t Cmd_CompleteCommand (char *partial);
// attempts to match a partial command for automatic command line completion
// returns NULL if nothing fits

int32_t		Cmd_Argc (void);
char	*Cmd_Argv (int32_t arg);
char	*Cmd_Args (void);
// The functions that execute commands get their parameters with these
// functions. Cmd_Argv () will return an empty string, not a NULL
// if arg > argc, so string operations are always safe.

void	Cmd_TokenizeString (char *text, qboolean macroExpand);
// Takes a null terminated string.  Does not need to be /n terminated.
// breaks the string up into arg tokens.

void	Cmd_ExecuteString (char *text);
// Parses a single line of text into arguments and tries to execute it
// as if it was typed at the console

void	Cmd_ForwardToServer (void);
// adds the current command line as a clc_stringcmd to the client message.
// things like godmode, noclip, etc, are commands directed to the server,
// so when they are typed in at the console, they will need to be forwarded.


/*
==============================================================

CVAR

==============================================================
*/

/*

cvar_t variables are used to hold scalar or string variables that can be changed or displayed at the console or prog code as well as accessed directly
in C code.

The user can access cvars from the console in three ways:
r_draworder			prints the current value
r_draworder 0		sets the current value to 0
set r_draworder 0	as above, but creates the cvar if not present
Cvars are restricted from having the same names as commands to keep this
interface from being ambiguous.
*/

#define CVAR_HASHMAP_WIDTH 0x80
#define CVAR_HASHMAP_MASK 0x7F
extern	cvar_t	*cvar_vars[CVAR_HASHMAP_WIDTH];
extern  stable_t cvarNames;

cvar_t *Cvar_Get (const char *var_name, const char *value, int32_t flags);
// creates the variable if it doesn't exist, or returns the existing one
// if it exists, the value will not be changed, but flags will be ORed in
// that allows variables to be unarchived without needing bitflags

cvar_t 	*Cvar_Set (const char *var_name, const char *value);
// will create the variable if it doesn't exist

cvar_t *Cvar_ForceSet (const char *var_name, const char *value);
// will set the variable even if NOSET or LATCH

cvar_t 	*Cvar_FullSet (const char *var_name, const char *value, int32_t flags);


void	Cvar_SetValue (const char *var_name, float value);
// expands value to a string and calls Cvar_Set

void Cvar_SetInteger (const char *var_name, int32_t integer);
// expands value to a string and calls Cvar_Set

float	Cvar_VariableValue (const char *var_name);
// returns 0 if not defined or non numeric

int32_t Cvar_VariableInteger (const char *var_name);
// returns 0 if not defined or non numeric

const char	*Cvar_VariableString (const char *var_name);
// returns an empty string if not defined

// Knightmare added
float Cvar_DefaultValue (const char *var_name);
// returns 0 if not defined or non numeric
const char	*Cvar_DefaultString (const char *var_name);
// returns an empty string if not defined
// Knightmare added
cvar_t *Cvar_SetToDefault (const char *var_name);
// end Knightmare

const char 	*Cvar_CompleteVariable (const char *partial);
// attempts to match a partial variable name for command line completion
// returns NULL if nothing fits

void	Cvar_GetLatchedVars (void);
// any CVAR_LATCHED variables that have been set will now take effect

void Cvar_FixCheatVars (qboolean allowCheats);
// called from CL_FixCvarCheats to lock cheat cvars in multiplayer

qboolean Cvar_Command (void);
// called by Cmd_ExecuteString when Cmd_Argv(0) doesn't match a known
// command.  Returns true if the command was a variable reference that
// was handled. (print or change)

void 	Cvar_WriteVariables (const char *path);
// appends lines containing "set variable value" for all variables
// with the archive flag set to true.


void	Cvar_Init (void);

char	*Cvar_Userinfo (void);
// returns an info string containing all the CVAR_USERINFO cvars

char	*Cvar_Serverinfo (void);
// returns an info string containing all the CVAR_SERVERINFO cvars

extern	qboolean	userinfo_modified;
// this is set each time a CVAR_USERINFO variable is changed
// so that the client knows to send it to the server

// checks cvar range
void    Cvar_AssertRange (cvar_t *var, float min, float max, qboolean isInteger);

/*
==============================================================

NET

==============================================================
*/

// net.h -- quake's interface to the networking layer

#define	PORT_ANY	-1

//Knightmare- increase max message size to eliminate SZ_Getspace: Overflow
//Mark Shan is just gonna love this!!
#ifndef NET_SERVER_BUILD
#define	MAX_MSGLEN		44800
#else
#define	MAX_MSGLEN		1400
#endif
//end Knightmare

#define	PACKET_HEADER	10			// two ints and a int16_t

typedef enum {NA_LOOPBACK, NA_BROADCAST, NA_IP, NA_IPX, NA_BROADCAST_IPX} netadrtype_t;

typedef enum {NS_CLIENT, NS_SERVER} netsrc_t;

typedef struct
{
	netadrtype_t	type;

	byte	ip[4];
	byte	ipx[10];

	uint16_t	port;
} netadr_t;

void		NET_Init (void);
void		NET_Shutdown (void);

void		NET_Config (qboolean multiplayer);

qboolean	NET_GetPacket (netsrc_t sock, netadr_t *net_from, sizebuf_t *net_message);
void		NET_SendPacket (netsrc_t sock, int32_t length, void *data, netadr_t to);

qboolean	NET_CompareAdr (netadr_t a, netadr_t b);
qboolean	NET_CompareBaseAdr (netadr_t a, netadr_t b);
qboolean	NET_IsLocalAddress (netadr_t adr);
char		*NET_AdrToString (netadr_t a);
qboolean	NET_StringToAdr (const char *s, netadr_t *a);
void		NET_Sleep(int32_t msec);

//============================================================================

#define	OLD_AVG		0.99		// total = oldtotal*OLD_AVG + new*(1-OLD_AVG)

#define	MAX_LATENT	32

typedef struct
{
	qboolean	fatal_error;

	netsrc_t	sock;

	int32_t			dropped;			// between last packet and previous

	int32_t			last_received;		// for timeouts
	int32_t			last_sent;			// for retransmits

	netadr_t	remote_address;
	int32_t			qport;				// qport value to write when transmitting

// sequencing variables
	int32_t			incoming_sequence;
	int32_t			incoming_acknowledged;
	int32_t			incoming_reliable_acknowledged;	// single bit

	int32_t			incoming_reliable_sequence;		// single bit, maintained local

	int32_t			outgoing_sequence;
	int32_t			reliable_sequence;			// single bit
	int32_t			last_reliable_sequence;		// sequence number of last send

// reliable staging and holding areas
	sizebuf_t	message;		// writing buffer to send to server
	byte		message_buf[MAX_MSGLEN-16];		// leave space for header

// message is copied to this buffer when it is first transfered
	int32_t			reliable_length;
	byte		reliable_buf[MAX_MSGLEN-16];	// unacked reliable message
} netchan_t;

extern	netadr_t	net_from;
extern	sizebuf_t	net_message;
extern	byte		net_message_buffer[MAX_MSGLEN];


void Netchan_Init (void);
void Netchan_Setup (netsrc_t sock, netchan_t *chan, netadr_t adr, int32_t qport);

qboolean Netchan_NeedReliable (netchan_t *chan);
void Netchan_Transmit (netchan_t *chan, int32_t length, byte *data);
void Netchan_OutOfBand (int32_t net_socket, netadr_t adr, int32_t length, byte *data);
void Netchan_OutOfBandPrint (int32_t net_socket, netadr_t adr, char *format, ...);
qboolean Netchan_Process (netchan_t *chan, sizebuf_t *msg);

qboolean Netchan_CanReliable (netchan_t *chan);


/*
==============================================================

CMODEL

==============================================================
*/


#include "../qcommon/qfiles.h"

cmodel_t	*CM_LoadMap (char *name, qboolean clientload, unsigned *checksum);
cmodel_t	*CM_InlineModel (char *name);	// *1, *2, etc

int32_t			CM_NumClusters (void);
int32_t			CM_NumInlineModels (void);
char		*CM_EntityString (void);

// creates a clipping hull for an arbitrary box
int32_t			CM_HeadnodeForBox (vec3_t mins, vec3_t maxs);


// returns an ORed contents mask
int32_t			CM_PointContents (vec3_t p, int32_t headnode);
int32_t			CM_TransformedPointContents (vec3_t p, int32_t headnode, vec3_t origin, vec3_t angles);

trace_t		CM_BoxTrace (vec3_t start, vec3_t end,
						  vec3_t mins, vec3_t maxs,
						  int32_t headnode, int32_t brushmask);
trace_t		CM_TransformedBoxTrace (vec3_t start, vec3_t end,
						  vec3_t mins, vec3_t maxs,
						  int32_t headnode, int32_t brushmask,
						  vec3_t origin, vec3_t angles);

byte		*CM_ClusterPVS (int32_t cluster);
byte		*CM_ClusterPHS (int32_t cluster);

int32_t			CM_PointLeafnum (vec3_t p);

// call with topnode set to the headnode, returns with topnode
// set to the first node that splits the box
int32_t			CM_BoxLeafnums (vec3_t mins, vec3_t maxs, int32_t *list,
							int32_t listsize, int32_t *topnode);

int32_t			CM_LeafContents (int32_t leafnum);
int32_t			CM_LeafCluster (int32_t leafnum);
int32_t			CM_LeafArea (int32_t leafnum);

void		CM_SetAreaPortalState (int32_t portalnum, qboolean open);
qboolean	CM_AreasConnected (int32_t area1, int32_t area2);

int32_t			CM_WriteAreaBits (byte *buffer, int32_t area);
qboolean	CM_HeadnodeVisible (int32_t headnode, byte *visbits);

void		CM_WritePortalState (FILE *f);


/*
==============================================================

PLAYER MOVEMENT CODE

Common between server and client so prediction matches

==============================================================
*/

#define DEFAULT_MAXSPEED	300
#define DEFAULT_DUCKSPEED	100
#define DEFAULT_WATERSPEED	400
#define DEFAULT_ACCELERATE	10
#define DEFAULT_STOPSPEED	100

extern float pm_airaccelerate;

void Pmove (pmove_t *pmove);

/*
==============================================================

FILESYSTEM

==============================================================
*/


typedef int32_t fileHandle_t;

typedef enum {
	FS_READ,
	FS_WRITE,
	FS_APPEND
} fsMode_t;

typedef enum {
	FS_SEEK_CUR,
	FS_SEEK_SET,
	FS_SEEK_END
} fsOrigin_t;

typedef enum {
	FS_SEARCH_PATH_EXTENSION,
	FS_SEARCH_BY_FILTER,
	FS_SEARCH_FULL_PATH
} fsSearchType_t;

extern	int32_t		file_from_pak;
extern	int32_t		file_from_pk3;
extern	char	last_pk3_name[MAX_QPATH];

void		FS_Startup (void);
void		FS_InitFilesystem (void);
void		FS_Shutdown (void);

void		FS_DPrintf (const char *format, ...);
FILE		*FS_FileForHandle (fileHandle_t f);
int32_t			FS_FOpenFile (const char *name, fileHandle_t *f, fsMode_t mode);
void		FS_FCloseFile (fileHandle_t f);
int32_t			FS_Read (void *buffer, int32_t size, fileHandle_t f);
int32_t			FS_FRead (void *buffer, int32_t size, int32_t count, fileHandle_t f);
int32_t			FS_Write (const void *buffer, int32_t size, fileHandle_t f);
void		FS_Seek (fileHandle_t f, int32_t offset, fsOrigin_t origin);
int32_t			FS_FTell (fileHandle_t f);
int32_t			FS_Tell (fileHandle_t f);
qboolean	FS_FileExists (char *path);
void		FS_CopyFile (const char *srcPath, const char *dstPath);
void		FS_RenameFile (const char *oldPath, const char *newPath);
void		FS_DeleteFile (const char *path);

char		*FS_GameDir (void);
void		FS_CreatePath (char *path);
void		FS_DeletePath (char *path);
char		*FS_NextPath (char *prevPath);
int32_t     FS_ListFiles (char *findname, sset_t *ss, uint32_t musthave, uint32_t canthave);
int32_t     FS_ListPak (char *find, sset_t *output);
char		**FS_ListPakOld (char *find, int32_t *num); // pak list function
int32_t     FS_ListFilesRelative (const char *path, const char *pattern, sset_t *ss, uint32_t musthave, uint32_t canthave);
int32_t     FS_ListFilesWithPaks(char *findname, sset_t *output, uint32_t musthave, uint32_t canthave);
int32_t     FS_CountFiles (char *findname, uint32_t musthave, uint32_t canthave);
int32_t     FS_CountFilesWithPaks(char *findname, uint32_t musthave, uint32_t canthave);
void		FS_FreeFileList (char **list, int32_t n);
qboolean	FS_ItemInList (char *check, int32_t num, char **list);
void		FS_InsertInList (char **list, char *insert, int32_t len, int32_t start);
void		FS_Dir_f (void);

void		FS_ExecAutoexec (void);
int32_t			FS_GetFileList (const char *path, const char *extension, char *buffer, int32_t size, fsSearchType_t searchType);

int32_t			FS_LoadFile (const char *path, void **buffer);
void		FS_AddPK3File (const char *packPath); // add pk3 file function
void		FS_SetGamedir (const char *dir);
char		*FS_Gamedir (void);
void		FS_FreeFile (void *buffer);


/*
==============================================================

MISC

==============================================================
*/


#define	ERR_FATAL	0		// exit the entire game with a popup window
#define	ERR_DROP	1		// print to console and disconnect from game
#define	ERR_QUIT	2		// not an error, just a normal exit

#define	EXEC_NOW	0		// don't return until completed
#define	EXEC_INSERT	1		// insert at current position, but don't run yet
#define	EXEC_APPEND	2		// add to end of the command buffer

#define	PRINT_ALL		0
#define PRINT_DEVELOPER	1	// only print when "developer 1"

void		Com_BeginRedirect (int32_t target, char *buffer, int32_t buffersize, void (*flush));
void		Com_EndRedirect (void);
void 		Com_Printf (char *fmt, ...);
void 		Com_DPrintf (char *fmt, ...);
void 		Com_Error (int32_t code, char *fmt, ...);
void 		Com_Quit (void);

int32_t			Com_ServerState (void);		// this should have just been a cvar...
void		Com_SetServerState (int32_t state);

unsigned	Com_BlockChecksum (void *buffer, int32_t length);
byte		COM_BlockSequenceCRCByte (byte *base, int32_t length, int32_t sequence);

float	frand(void);	// 0 ti 1
float	crand(void);	// -1 to 1

extern	cvar_t	*developer;
extern	cvar_t	*dedicated;
extern	cvar_t	*host_speeds;
extern	cvar_t	*log_stats;

// Knightmare- for the game DLL to tell what engine it's running under
extern	cvar_t *sv_engine;
extern	cvar_t *sv_engine_version;
extern	cvar_t *sv_entfile;			// whether to use .ent file

// Knightmare added
extern	cvar_t *fs_gamedirvar;
extern	cvar_t *fs_basedir;

extern	FILE *log_stats_file;

// host_speeds times
extern	int32_t		time_before_game;
extern	int32_t		time_after_game;
extern	int32_t		time_before_ref;
extern	int32_t		time_after_ref;

enum {
    // untagged always has the lowest priority. It should not be being used. period.
    TAG_UNTAGGED = 0x8000,
    // the following pre-defined zones have the highest priority on any tag chain
    TAG_MENU = 0x7FF8,
    TAG_SERVER = 0x7FF9,
    TAG_CLIENT = 0x7FFA,
    TAG_RENDERER = 0x7FFB,
    TAG_AUDIO = 0x7FFC,
    TAG_GAME = 0x7FFD,
    TAG_LEVEL = 0x7FFE,
    TAG_SYSTEM = 0x7FFF,
    // these correspond with legacy game libraries
    TAG_GAME_LEGACY = 765,
    TAG_LEVEL_LEGACY = 766,
};

void Z_Free (void *ptr);
void *Z_Malloc (int32_t size);			// returns 0 filled memory
void *Z_TagMalloc (int32_t size, int16_t tag);
void *Z_Strdup (const char *string);
void *Z_TagStrdup (const char *string, int16_t tag);
void *Z_Realloc(void* ptr, int32_t size);
void Z_FreeTags (int16_t tag);

void Qcommon_Init (int32_t argc, char **argv);
void Qcommon_Frame (int32_t msec);
void Qcommon_Shutdown (void);

#define NUMVERTEXNORMALS	162
extern	vec3_t	bytedirs[NUMVERTEXNORMALS];

// this is in the client code, but can be used for debugging from server
void SCR_DebugGraph (float value, int32_t color);


/*
 ==============================================================
 
HASHING FUNCTIONS
 
 ==============================================================
 */
#ifndef Q2VR_ENGINE_MOD
typedef struct hash128_t { uint32_t v[4]; } hash128_t;
typedef struct hash32_t {uint32_t h; } hash32_t;
#endif

hash128_t Q_Hash128(const char *string, uint32_t len);
hash128_t Q_HashSanitized128(const char *string);
int32_t Q_HashEquals128(hash128_t hash1, hash128_t hash2);
int32_t Q_HashCompare128(hash128_t hash1, hash128_t hash2);
hash32_t Q_Hash32(const char *string, uint32_t len);
hash32_t Q_HashSanitized32(const char *string);
int32_t Q_HashEquals32(hash32_t hash1, hash32_t hash2);
int32_t Q_HashCompare32(hash32_t hash1, hash32_t hash2);


/*
 ==============================================================
 
 STRING TABLE FUNCTIONS
 
 ==============================================================
 */
#ifndef Q2VR_ENGINE_MOD
typedef struct stable_t {
    void *st;
    uint32_t size;
    int16_t tag;
} stable_t;
#endif

qboolean Q_STInit(stable_t *st,uint32_t size, int32_t avgLength, int16_t memoryTag);
void Q_STFree(stable_t *st);
qboolean Q_STGrow(stable_t *st, size_t newSize);
qboolean Q_STShrink(stable_t *st, size_t newSize);
int32_t Q_STRegister(const stable_t *st, const char *string);
int32_t Q_STLookup(const stable_t *st, const char *string);
const char *Q_STGetString(const stable_t *st, int token);
int32_t Q_STUsedBytes(const stable_t *st);
int32_t Q_STAutoRegister(stable_t *st, const char *string);
int32_t Q_STAutoPack(stable_t *st);


/*
 ==============================================================
 
 STRING SET FUNCTIONS
 
 ==============================================================
 */
#ifndef Q2VR_ENGINE_MOD
typedef struct sset_t {
    stable_t table;
    const char **strings;
    int32_t *tokens;
    uint32_t currentSize;
    uint32_t maxSize;
    uint16_t tag;
} sset_t;
#endif

qboolean Q_SSetInit(sset_t *ss,uint32_t maxSize, int32_t avgLength, int16_t memoryTag);
void Q_SSetFree(sset_t *ss);
qboolean Q_SSetContains(const sset_t *ss, const char *string);
qboolean Q_SSetInsert(sset_t *ss, const char *string);
qboolean Q_SSetDuplicate(sset_t *source, sset_t *dest);
const char *Q_SSetGetString(sset_t *ss, int32_t index);
int32_t Q_SSetGetStrings(sset_t *source, const char **strings, int32_t maxStrings);

/*
==============================================================

NON-PORTABLE SYSTEM SERVICES

==============================================================
*/

void	Sys_Init (void);

void	Sys_AppActivate (void);

void	Sys_UnloadGame (void);
void	*Sys_FindLibrary(const char *dllnames[]);
void	*Sys_GetGameAPI (void *parms);
// loads the game dll and calls the api init function

char	*Sys_ConsoleInput (void);
void	Sys_ConsoleOutput (char *string);
void	Sys_SendKeyEvents (void);
void	Sys_Error (char *error, ...);
void	Sys_Quit (void);
char	*Sys_GetClipboardData( void );
void	Sys_CopyProtect (void);

/*
==============================================================

CLIENT / SERVER SYSTEMS

==============================================================
*/

void CL_Init (void);
void CL_Drop (void);
void CL_Shutdown (void);
void CL_Frame (int32_t msec);
void Con_Print (char *text);
void SCR_BeginLoadingPlaque (void);

void SV_Init (void);
void SV_Shutdown (char *finalmsg, qboolean reconnect);
void SV_Frame (int32_t msec);


#endif // __QCOMMON_H

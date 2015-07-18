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
// cmd.c -- Quake script command processing module

#include "qcommon.h"

void Cmd_ForwardToServer (void);

#define ALIAS_BLOCK_MAX 64
#define	MAX_ALIAS_NAME	32

#define INITIAL_CVARTABLE_SIZE 20480
#define INITIAL_COMMAND_TABLE_SIZE 8096

stable_t cmdNames = {0, INITIAL_COMMAND_TABLE_SIZE};
stable_t aliasNames = {0, INITIAL_COMMAND_TABLE_SIZE};
stable_t aliasTable = {0, INITIAL_COMMAND_TABLE_SIZE};

typedef struct cmdalias_s
{
	struct cmdalias_s	*next;
    int32_t  nameIndex;
    int32_t  cmdIndex;
} cmdalias_t;


typedef struct aliasblock_s {
    cmdalias_t aliases[ALIAS_BLOCK_MAX];
    struct aliasblock_s *next;
    int32_t maxIndex;
    uint16_t numAllocated;
} aliasblock_t;

static aliasblock_t *alias_head = NULL, *alias_tail = NULL;

#define CMDALIAS_HASHMAP_WIDTH 0x8
#define CMDALIAS_HASHMAP_MASK 0x7
cmdalias_t	*cmd_alias[CMDALIAS_HASHMAP_WIDTH];

qboolean	cmd_wait;

#define	ALIAS_LOOP_COUNT	16
int32_t		alias_count;		// for detecting runaway loops


//=============================================================================

/*
============
Cmd_Wait_f

Causes execution of the remainder of the command buffer to be delayed until
next frame.  This allows commands like:
bind g "impulse 5 ; +attack ; wait ; -attack ; impulse 2"
============
*/
void Cmd_Wait_f (void)
{
	cmd_wait = true;
}


/*
=============================================================================

						COMMAND BUFFER

=============================================================================
*/

sizebuf_t	cmd_text;
byte		cmd_text_buf[32768]; // Knightmare increased, was 8192

byte		defer_text_buf[32768]; // Knightmare increased, was 8192
char		temp_text_buf[32768]; // Knightmare increased, was 8192

/*
============
Cbuf_Init
============
*/
void Cbuf_Init (void)
{
	SZ_Init (&cmd_text, cmd_text_buf, sizeof(cmd_text_buf));
}

/*
============
Cbuf_AddText

Adds command text at the end of the buffer
============
*/
void Cbuf_AddText (const char *text)
{
	int32_t		l;
	
	l = strlen (text);

	if (cmd_text.cursize + l >= cmd_text.maxsize)
	{
		Com_Printf ("Cbuf_AddText: overflow\n");
		return;
	}
	SZ_Write (&cmd_text, text, strlen (text));
}


/*
============
Cbuf_InsertText

Adds command text immediately after the current command
Adds a \n to the text
FIXME: actually change the command buffer to do less copying
============
*/
void Cbuf_InsertText (const char *text)
{
	char	*temp = temp_text_buf;
	int32_t		templen;
// copy off any commands still remaining in the exec buffer
	templen = cmd_text.cursize;
	if (templen)
	{
//        memset(temp,0,(templen + 1) * sizeof(char));
		memcpy(temp, cmd_text.data, templen);
		SZ_Clear (&cmd_text);
	}
		
// add the entire text of the file
	Cbuf_AddText (text);
	
// add the copied off data
	if (templen)
	{
		SZ_Write (&cmd_text, temp, templen);
	}
}


/*
============
Cbuf_CopyToDefer
============
*/
void Cbuf_CopyToDefer (void)
{
	memcpy(defer_text_buf, cmd_text_buf, cmd_text.cursize);
	defer_text_buf[cmd_text.cursize] = 0;
	cmd_text.cursize = 0;
}

/*
============
Cbuf_InsertFromDefer
============
*/
void Cbuf_InsertFromDefer (void)
{
	Cbuf_InsertText ((char *)defer_text_buf);
	defer_text_buf[0] = 0;
}


/*
============
Cbuf_ExecuteText
============
*/
void Cbuf_ExecuteText (int32_t exec_when, char *text)
{
	switch (exec_when)
	{
	case EXEC_NOW:
		Cmd_ExecuteString (text);
		break;
	case EXEC_INSERT:
		Cbuf_InsertText (text);
		break;
	case EXEC_APPEND:
		Cbuf_AddText (text);
		break;
	default:
		Com_Error (ERR_FATAL, "Cbuf_ExecuteText: bad exec_when");
	}
}

/*
============
Cbuf_Execute
============
*/
void Cbuf_Execute (void)
{
	int32_t		i;
	char	*text;
	char	line[1024];
	int32_t		quotes;

	alias_count = 0;		// don't allow infinite alias loops

	while (cmd_text.cursize)
	{
// find a \n or ; line break
		text = (char *)cmd_text.data;

		quotes = 0;
		for (i=0 ; i< cmd_text.cursize ; i++)
		{
			if (text[i] == '"')
				quotes++;
			if ( !(quotes&1) &&  text[i] == ';')
				break;	// don't break if inside a quoted string
			if (text[i] == '\n')
				break;
		}

		// [SkulleR]'s fix for overflow vulnerability
		if (i > sizeof(line) - 1)
			i =  sizeof(line) - 1; 		
		memcpy (line, text, i); // stack buffer overflow possible
		line[i] = 0;
		
// delete the text from the command buffer and move remaining commands down
// this is necessary because commands (exec, alias) can insert data at the
// beginning of the text buffer

		if (i == cmd_text.cursize)
			cmd_text.cursize = 0;
		else
		{
			i++;
			cmd_text.cursize -= i;
			memmove (text, text+i, cmd_text.cursize);
		}

// execute the command line
		Cmd_ExecuteString (line);
		
		if (cmd_wait)
		{
			// skip out while text still remains in buffer, leaving it
			// for next frame
			cmd_wait = false;
			break;
		}
	}
}


/*
===============
Cbuf_AddEarlyCommands

Adds command line parameters as script statements
Commands lead with a +, and continue until another +

Set commands are added early, so they are guaranteed to be set before
the client and server initialize for the first time.

Other commands are added late, after all initialization is complete.
===============
*/
void Cbuf_AddEarlyCommands (qboolean clear)
{
	int32_t		i;
	char	*s;

	for (i=0 ; i<COM_Argc() ; i++)
	{
		s = COM_Argv(i);
		if (strcmp (s, "+set"))
			continue;
		Cbuf_AddText (va("set %s %s\n", COM_Argv(i+1), COM_Argv(i+2)));
		if (clear)
		{
			COM_ClearArgv(i);
			COM_ClearArgv(i+1);
			COM_ClearArgv(i+2);
		}
		i+=2;
	}
}

/*
=================
Cbuf_AddLateCommands

Adds command line parameters as script statements
Commands lead with a + and continue until another + or -
quake +vid_ref gl +map amlev1

Returns true if any late commands were added, which
will keep the demoloop from immediately starting
=================
*/
qboolean Cbuf_AddLateCommands (void)
{
	int32_t		i, j;
	int32_t		s;
	char	*text = temp_text_buf;
	int32_t		argc;
	qboolean	ret;

// build the combined string to parse from
	s = 0;
	argc = COM_Argc();
	for (i=1 ; i<argc ; i++)
	{
        if (COM_Argv(i)[0]) {
            s += strlen (COM_Argv(i)) + 1;
        }
	}
	if (!s)
		return false;
		
    memset(text, 0, (s+1)*sizeof(char));
	for (i=1 ; i<argc ; i++)
	{
        if (COM_Argv(i)[0]) {
            strcat (text,COM_Argv(i));
            if (i != argc-1)
                strcat (text, " ");
        }

	}
		
	for (i=0 ; i<s-1 ; i++)
	{
		if (text[i] == '+')
		{
			i++;

			for (j=i ; (text[j] != '+') && (text[j] != '-') && (text[j] != 0) ; j++)
				;
			text[j] = '\n';
			
		}
	}

	ret = (text[0] != 0);
	if (ret)
		Cbuf_AddText (text);
    
	return ret;
}


/*
==============================================================================

						SCRIPT COMMANDS

==============================================================================
*/

/*
===============
Cmd_Exec_f
===============
*/
void Cmd_Exec_f (void)
{
	char	*f, *f2;
	int32_t		len;

	if (Cmd_Argc () != 2)
	{
		Com_Printf ("exec <filename> : execute a script file\n");
		return;
	}

	len = FS_LoadFile (Cmd_Argv(1), (void **)&f);
	if (!f)
	{
		Com_Printf ("couldn't exec %s\n",Cmd_Argv(1));
		return;
	}
	Com_Printf ("execing %s\n",Cmd_Argv(1));
	
	// the file doesn't have a trailing 0, so we need to copy it off
	f2 = (char*)Z_Realloc(f, len+2); // Echon fix- was len+1

	f2[len] = '\n';  // Echon fix added
	f2[len+1] = '\0'; // Echon fix- was len, = 0

	Cbuf_InsertText (f2);

	FS_FreeFile (f2);
}


/*
===============
Cmd_Echo_f

Just prints the rest of the line to the console
===============
*/
void Cmd_Echo_f (void)
{
	int32_t		i;
	
	for (i=1 ; i<Cmd_Argc() ; i++)
		Com_Printf ("%s ",Cmd_Argv(i));
	Com_Printf ("\n");
}

/*
===============
Cmd_Alias_f

Creates a new command that executes a command string (possibly ; seperated)
===============
*/
void Cmd_Alias_f (void)
{
	cmdalias_t	*a = NULL;
	char		cmd[1024];
	int32_t			i, c;
	char		*s;
    int32_t      nameIndex;
    int32_t     index;
    char        buffer[MAX_ALIAS_NAME];
    
	if (Cmd_Argc() == 1)
	{
        aliasblock_t *block = alias_head;
		Com_Printf ("Current alias commands:\n");
        while (block != NULL) {
            int j;
            int m = block->numAllocated;
            for (j = 0; j < m; j++) {
                cmdalias_t *alias = &block->aliases[j];
                const char *name = Q_STGetString(&aliasNames, alias->nameIndex);
                const char *cmd = Q_STGetString(&aliasTable, alias->cmdIndex);
                Com_Printf ("  %s : %s", name, cmd);
            }
            block = block->next;
        }
		return;
	}
    
	s = Cmd_Argv(1);
	if (strlen(s) >= MAX_ALIAS_NAME)
	{
		Com_Printf ("Alias name is too long\n");
		return;
	}
    
    nameIndex = Q_STLookup(&aliasNames, s);

    if (nameIndex >= 0) {
        // if the alias already exists, reuse it
        for (a = cmd_alias[nameIndex & CMDALIAS_HASHMAP_MASK] ; a ; a=a->next)
        {
            if (a->nameIndex == nameIndex);
            {
                break;
            }
        }
    }
    
	if (!a)
	{
        if (!alias_head) {
            alias_tail = Z_TagMalloc(sizeof(*alias_tail), TAG_SYSTEM);
            alias_head = alias_tail;
        } else if ((int32_t)alias_tail->numAllocated == ALIAS_BLOCK_MAX) {
            alias_tail->next = Z_TagMalloc(sizeof(*alias_tail), TAG_SYSTEM);
            alias_tail = alias_tail->next;
        }
        assert(alias_tail != NULL);
        
        a = &alias_tail->aliases[alias_tail->numAllocated++];
	}
    
    // copy the rest of the command line
	cmd[0] = 0;		// start out with a null string
	c = Cmd_Argc();
	for (i=2 ; i< c ; i++)
	{
		strcat (cmd, Cmd_Argv(i));
		if (i != (c - 1))
			strcat (cmd, " ");
	}
	strcat (cmd, "\n");
    
    Q_strlcpy_lower(buffer, s, sizeof(buffer));

    a->nameIndex = Q_STAutoRegister(&aliasNames, buffer);
    a->cmdIndex = Q_STAutoRegister(&aliasTable, cmd);
    alias_tail->maxIndex = a->nameIndex;
    index = a->nameIndex & CMDALIAS_HASHMAP_MASK;
    if (!a->next) {
        a->next = cmd_alias[index];
        cmd_alias[index] = a;
    }
}

/*
=============================================================================

					COMMAND EXECUTION

=============================================================================
*/

typedef struct cmd_function_s
{
	struct cmd_function_s	*next;
    int32_t                 name_index;
    xcommand_t				function;
} cmd_function_t;

#define CMD_BLOCK_MAX 64

static	int32_t			cmd_argc;
static	char		cmd_argv[MAX_STRING_TOKENS][MAX_TOKEN_CHARS];
static	char		*cmd_null_string = "";
static	char		cmd_args[MAX_STRING_CHARS];

#define CMD_HASHMAP_WIDTH 0x80
#define CMD_HASHMAP_MASK 0x7F

typedef struct cmdblock_s {
    cmd_function_t cmds[CMD_BLOCK_MAX];
    struct cmdblock_s *next;
    int32_t maxIndex;
    uint16_t numAllocated;
} cmdblock_t;


static cmdblock_t *cmd_head = NULL, *cmd_tail = NULL;

static	cmd_function_t	*cmd_functions[CMD_HASHMAP_WIDTH];		// possible commands to execute

/*
============
Cmd_Argc
============
*/
int32_t		Cmd_Argc (void)
{
	return cmd_argc;
}

/*
============
Cmd_Argv
============
*/
char	*Cmd_Argv (int32_t arg)
{
	if ( (uint32_t)arg >= cmd_argc )
		return cmd_null_string;
	return cmd_argv[arg];	
}

/*
============
Cmd_Args

Returns a single string containing argv(1) to argv(argc()-1)
============
*/
char		*Cmd_Args (void)
{
	return cmd_args;
}


/*
======================
Cmd_MacroExpandString
======================
*/
char *Cmd_MacroExpandString (char *text)
{
	int32_t		i, j, count, len;
	qboolean	inquote;
	char	*scan;
	static	char	expanded[MAX_STRING_CHARS];
	char	temporary[MAX_STRING_CHARS];
    const char	*token;
    char *start;

	inquote = false;
	scan = text;

	len = strlen (scan);
	if (len >= MAX_STRING_CHARS)
	{
		Com_Printf ("Line exceeded %i chars, discarded.\n", MAX_STRING_CHARS);
		return NULL;
	}

	count = 0;

	for (i=0 ; i<len ; i++)
	{
		if (scan[i] == '"')
			inquote ^= 1;
		if (inquote)
			continue;	// don't expand inside quotes
		if (scan[i] != '$')
			continue;
		// scan out the complete macro
		start = scan+i+1;
		token = COM_Parse (&start);
		if (!start)
			continue;
	
		token = Cvar_VariableString (token);

		j = strlen(token);
		len += j;
		if (len >= MAX_STRING_CHARS)
		{
			Com_Printf ("Expanded line exceeded %i chars, discarded.\n", MAX_STRING_CHARS);
			return NULL;
		}

		strncpy (temporary, scan, i);
		strcpy (temporary+i, token);
		strcpy (temporary+i+j, start);

		strcpy (expanded, temporary);
		scan = expanded;
		i--;

		if (++count == 100)
		{
			Com_Printf ("Macro expansion loop, discarded.\n");
			return NULL;
		}
	}

	if (inquote)
	{
		Com_Printf ("Line has unmatched quote, discarded.\n");
		return NULL;
	}

	return scan;
}


/*
============
Cmd_TokenizeString

Parses the given string into command line tokens.
$Cvars will be expanded unless they are in a quoted token
============
*/
void Cmd_TokenizeString (char *text, qboolean macroExpand)
{
	char	*com_token;

    memset(cmd_argv,0,sizeof(cmd_argv));
		
	cmd_argc = 0;
	cmd_args[0] = 0;
	
	// macro expand the text
	if (macroExpand)
		text = Cmd_MacroExpandString (text);
	if (!text)
		return;

	while (1)
	{
// skip whitespace up to a /n
		while (*text && *text <= ' ' && *text != '\n')
		{
			text++;
		}
		
		if (*text == '\n')
		{	// a newline seperates commands in the buffer
			text++;
			break;
		}

		if (!*text)
			return;

		// set cmd_args to everything after the first arg
		if (cmd_argc == 1)
		{
			int32_t		l;

			// [SkulleR]'s fix for overflow vulnerability
			//strcpy (cmd_args, text);
			strncpy (cmd_args, text,sizeof(cmd_args)-1);
			cmd_args[sizeof(cmd_args)-1] = 0; 

			// strip off any trailing whitespace
			l = strlen(cmd_args) - 1;
			for ( ; l >= 0 ; l--)
				if (cmd_args[l] <= ' ')
					cmd_args[l] = 0;
				else
					break;
		}
			
		com_token = COM_Parse (&text);
		if (!text)
			return;

		if (cmd_argc < MAX_STRING_TOKENS)
		{
			strncpy (cmd_argv[cmd_argc], com_token, MAX_TOKEN_CHARS);
			cmd_argc++;
		}
	}
	
}


/*
============
Cmd_AddCommand
============
*/
void	Cmd_AddCommand (char *cmd_name, xcommand_t function)
{
	cmd_function_t	*cmd;
    int32_t nameIndex;
    int index;
    char buffer[MAX_TOKEN_CHARS];
    Q_strlcpy_lower(buffer, cmd_name, sizeof(buffer));

// fail if the command is a variable name
	if (Cvar_VariableString(buffer)[0])
	{
		Com_Printf ("Cmd_AddCommand: %s already defined as a var\n", buffer);
		return;
	}
	
    nameIndex = Q_STLookup(&cmdNames, buffer);
// fail if the command already exists
    if (nameIndex >= 0) {
        int i;
        cmdblock_t *c = cmd_head;
        index = nameIndex&CMD_HASHMAP_MASK;
        for (cmd = cmd_functions[index]; cmd; cmd = cmd->next) {
            if (cmd->name_index == nameIndex) {
                Com_Printf ("Cmd_AddCommand: %s already defined\n", buffer);
                return;
            }
        }
        // reuse the existing command definition
        while (c) {
            if (nameIndex <= c->maxIndex)
            {
                
                for (i = 0; i < c->numAllocated; i++) {
                    if (c->cmds[i].name_index == nameIndex) {
                        c->cmds[i].function = function;
                        c->cmds[i].next = cmd_functions[index];
                        cmd_functions[index] = &c->cmds[i];
                        return;
                    }
                }
            }
            c = c->next;
        }
    }
    
    nameIndex = Q_STAutoRegister(&cmdNames, buffer);

    if (!cmd_head) {
        cmd_head = Z_TagMalloc(sizeof(*cmd_head), TAG_SYSTEM);
        cmd_tail = cmd_head;
    } else if (cmd_tail->numAllocated == CMD_BLOCK_MAX) {
        cmd_tail->next = Z_TagMalloc(sizeof(*cmd_tail), TAG_SYSTEM);
        cmd_tail = cmd_tail->next;
    
    }
    
    cmd = &cmd_tail->cmds[cmd_tail->numAllocated++];
	cmd->function = function;
    cmd->name_index = nameIndex;
    cmd_tail->maxIndex = nameIndex;
    index = nameIndex&CMD_HASHMAP_MASK;
	cmd->next = cmd_functions[index];
	cmd_functions[index] = cmd;
}

/*
============
Cmd_RemoveCommand
============
*/
void	Cmd_RemoveCommand (char *cmd_name)
{
    char buffer[MAX_TOKEN_CHARS];
    int32_t nameIndex;

    Q_strlcpy_lower(buffer, cmd_name, sizeof(buffer));
    nameIndex = Q_STLookup(&cmdNames, buffer);

    if (nameIndex >= 0) {
        cmd_function_t	*cmd, **back;

        back = &cmd_functions[nameIndex&CMD_HASHMAP_MASK];

        while (1)
        {
            cmd = *back;
            if (!cmd)
            {
                Com_Printf ("Cmd_RemoveCommand: %s not added\n", cmd_name);
                return;
            }
            if (cmd->name_index == nameIndex)
            {
                *back = cmd->next;
                cmd->next = NULL;
                cmd->function = NULL;
                return;
            }
            back = &cmd->next;
        }
    }
    Com_Printf ("Cmd_RemoveCommand: %s not added\n", cmd_name);
    return;
}

/*
============
Cmd_Exists
============
*/
qboolean	Cmd_Exists (char *cmd_name)
{
	cmd_function_t	*cmd;
    int32_t nameIndex;
    char buffer[MAX_TOKEN_CHARS];
    Q_strlcpy_lower(buffer, cmd_name, sizeof(buffer));
    nameIndex = Q_STLookup(&cmdNames, buffer);

    if (nameIndex >= 0) {
        for (cmd=cmd_functions[nameIndex&CMD_HASHMAP_MASK] ; cmd ; cmd=cmd->next)
        {
            if (nameIndex == cmd->name_index)
                return true;
        }
    }
	return false;
}





/*
============
Cmd_CompleteCommand
============
*/

/*char *Cmd_CompleteCommand (char *partial)
{
	cmd_function_t	*cmd;
	int32_t				len;
	cmdalias_t		*a;
	
	len = strlen(partial);
	
	if (!len)
		return NULL;
		
// check for exact match
	for (cmd=cmd_functions ; cmd ; cmd=cmd->next)
		if (!strcmp (partial,cmd->name))
			return cmd->name;
	for (a=cmd_alias ; a ; a=a->next)
		if (!strcmp (partial, a->name))
			return a->name;

// check for partial match
	for (cmd=cmd_functions ; cmd ; cmd=cmd->next)
		if (!strncmp (partial,cmd->name, len))
			return cmd->name;
	for (a=cmd_alias ; a ; a=a->next)
		if (!strncmp (partial, a->name, len))
			return a->name;

	return NULL;
}*/
char            retval[256];

int strsort(const void *str1, const void *str2) {
    const char *a = *(const char **)str1;
    const char *b = *(const char **)str2;
	return strcasecmp(a,b);
}

completion_t Cmd_CompleteCommand (char *partial)
{
	cmd_function_t	*cmd;
	int32_t			len,i,j,o,p;
	cvar_t			*cvar;
	const char		*pmatch[1024];
	qboolean		diff = false;
    completion_t    result = {0, NULL};
    aliasblock_t      *a = alias_head;
	len = strlen(partial);

    if (!len)
        return result;

    i=0;
    
    for (j = 0; j < CMD_HASHMAP_WIDTH; j++) {
        for (cmd=cmd_functions[j] ; cmd ; cmd=cmd->next) {
            const char *name = Q_STGetString(&cmdNames, cmd->name_index);
            if (!Q_strncasecmp (partial, name, len)) {
                pmatch[i]=name;
                i++;
            }
        }
    }
    
    while (a) {
        int32_t numAliases = a->numAllocated;
        for (j= 0; j < numAliases; j++ ){
            const char *name = Q_STGetString(&aliasNames, a->aliases[j].nameIndex);
            if (!Q_strncasecmp (partial, name, len)) {
                pmatch[i]= name;
                i++;
            }
        }
        a = a->next;
    }
    
    for (j = 0; j < CVAR_HASHMAP_WIDTH; j++) {
        for (cvar=cvar_vars[j] ; cvar ; cvar=cvar->next) {
            if (!Q_strncasecmp (partial,cvar->name, len)) {
                pmatch[i]=cvar->name;
                i++;
            }
        }
    }
    
	if (i) {
        result.number = i;
        if (i == 1) {
            result.match = pmatch[0];
            return result;
        }
        qsort(pmatch,i, sizeof(char *), strsort);
        
        Com_Printf("\nListing matches for '%s'...\n",partial);
		for (o=0; o<i; o++)
			Com_Printf("  %s\n",pmatch[o]);

		strcpy(retval,""); p=0;
		while (!diff && p < 256) {
			retval[p]=pmatch[0][p];
			for (o=0; o<i; o++) {
				if (p > strlen(pmatch[o]))
					continue;
				if (retval[p] != pmatch[o][p]) {
					retval[p] = 0;
					diff=false;
				}
			}
			p++;
		}
		Com_Printf("Found %i matches\n",i);
        result.match = retval;
	}

    return result;
}

qboolean Cmd_IsComplete (const char *command)
{
	cmd_function_t	*cmd;
    int32_t         index;
    char buffer[MAX_TOKEN_CHARS];
    Q_strlcpy_lower(buffer, command, sizeof(buffer));
    
    index = Q_STLookup(&cmdNames, buffer);

    if (index >= 0) {
        // check for exact match
        for (cmd=cmd_functions[index&CMD_HASHMAP_MASK] ; cmd ; cmd=cmd->next)
            if (cmd->name_index == index)
                return true;
    }
    index = Q_STLookup(&aliasNames, buffer);

    if (index >= 0)
        return true;
    

    index = Q_STLookup(&cvarNames, buffer);
    if (index >= 0)
        return true;

	return false;
}

/*
============
Cmd_ExecuteString

A complete command line has been parsed, so try to execute it
FIXME: lookupnoadd the token to speed search?
============
*/
void	Cmd_ExecuteString (char *text)
{	
	cmd_function_t	*cmd;
	cmdalias_t		*a;
    int32_t index;
    char buffer[MAX_TOKEN_CHARS];
    
	Cmd_TokenizeString (text, true);
			
	// execute the command line
	if (!Cmd_Argc())
		return;		// no tokens

    Q_strlcpy_lower(buffer, cmd_argv[0], sizeof(buffer));
    
    index = Q_STLookup(&cmdNames, buffer);
    
	// check functions
    if (index >= 0) {
        for (cmd=cmd_functions[index&CMD_HASHMAP_MASK] ; cmd ; cmd=cmd->next)
        {
            if (index == cmd->name_index)
            {
                if (!cmd->function)
                {	// forward to server command
                    Cmd_ExecuteString (va("cmd %s", text));
                }
                else
                    cmd->function ();
                return;
            }
        }
    }

    Q_strlcpy_lower(buffer, cmd_argv[0], sizeof(buffer));

    index = Q_STLookup(&aliasNames, buffer);
	// check alias
    if (index >= 0) {
        for (a=cmd_alias[index&CMDALIAS_HASHMAP_MASK] ; a ; a=a->next)
        {
            if (index == a->nameIndex)
            {
                if (++alias_count == ALIAS_LOOP_COUNT)
                {
                    Com_Printf ("ALIAS_LOOP_COUNT\n");
                    return;
                }
                Cbuf_InsertText(Q_STGetString(&aliasTable,a->cmdIndex));
                return;
            }
        }
    }
	// check cvars
	if (Cvar_Command ())
		return;

	// send it as a server command if we are connected
	Cmd_ForwardToServer ();
}

/*
============
Cmd_List_f
============
*/
void Cmd_List_f (void)
{
	cmd_function_t	*cmd;
	int32_t			i,j;
	i = 0;
    for (j = 0; j < CMD_HASHMAP_WIDTH; j++) {
        for (cmd=cmd_functions[j] ; cmd ; cmd=cmd->next, i++) {
            const char *name = Q_STGetString(&cmdNames, cmd->name_index);
            Com_Printf ("%s\n", name);
        }
    }
	Com_Printf ("%i commands\n", i);
}

/*
============
Cmd_Init
============
*/
void Cmd_Init (void)
{
//
// register our commands
//
    Q_STInit(&cvarNames, INITIAL_CVARTABLE_SIZE, 10, TAG_SYSTEM);
    Q_STInit(&cmdNames, INITIAL_COMMAND_TABLE_SIZE, 10, TAG_SYSTEM);
    Q_STInit(&aliasNames, INITIAL_COMMAND_TABLE_SIZE, 10, TAG_SYSTEM);
    Q_STInit(&aliasTable, INITIAL_COMMAND_TABLE_SIZE, 10, TAG_SYSTEM);
    
    memset(cmd_functions,0,sizeof(cmd_functions));
    memset(cmd_alias,0,sizeof(cmd_alias));
	Cmd_AddCommand ("cmdlist",Cmd_List_f);
	Cmd_AddCommand ("exec",Cmd_Exec_f);
	Cmd_AddCommand ("echo",Cmd_Echo_f);
	Cmd_AddCommand ("alias",Cmd_Alias_f);
	Cmd_AddCommand ("wait", Cmd_Wait_f);
}


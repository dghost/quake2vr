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
// cvar.c -- dynamic variable tracking

#include "qcommon.h"
#include "wildcard.h"
cvar_t	*cvar_vars;

qboolean	cvar_allowCheats = true;

/*
============
Cvar_InfoValidate
============
*/
static qboolean Cvar_InfoValidate (char *s)
{
	if (strstr (s, "\\"))
		return false;
	if (strstr (s, "\""))
		return false;
	if (strstr (s, ";"))
		return false;
	return true;
}

/*
============
Cvar_FindVar
============
*/
static cvar_t *Cvar_FindVar (char *var_name)
{
	cvar_t	*var;
    hash32_t hash = Q_Hash32(var_name, strlen(var_name));
	for (var=cvar_vars ; var ; var=var->next)
		if (!Q_HashEquals32(hash, var->hash) && !strcmp (var_name, var->name))
			return var;

	return NULL;
}


/*
============
Cvar_VariableValue
============
*/
float Cvar_VariableValue (char *var_name)
{
	cvar_t	*var;
	
	var = Cvar_FindVar (var_name);
	if (!var)
		return 0;
	return atof (var->string);
}


/*
=================
Cvar_VariableInteger
=================
*/
int32_t Cvar_VariableInteger (char *var_name)
{
	cvar_t	*var;

	var = Cvar_FindVar(var_name);
	if (!var)
		return 0;
	return atoi (var->string);
}


/*
============
Cvar_VariableString
============
*/
char *Cvar_VariableString (char *var_name)
{
	cvar_t *var;
	
	var = Cvar_FindVar (var_name);
	if (!var)
		return "";
	return var->string;
}

/*
============
Cvar_DefaultValue
Knightmare added
============
*/
float Cvar_DefaultValue (char *var_name)
{
	cvar_t	*var;
	var = Cvar_FindVar (var_name);
	if (!var)
		return 0;
#ifdef NEW_CVAR_MEMBERS
	return atof (var->default_string);
#else
	return var->value;
#endif
}


/*
============
Cvar_DefaultString
Knightmare added
============
*/
char *Cvar_DefaultString (char *var_name)
{
	cvar_t *var;

	var = Cvar_FindVar (var_name);
	if (!var)
		return "";
#ifdef NEW_CVAR_MEMBERS
	return var->default_string;
#else
	return var->string;
#endif
}

/*
============
Cvar_CompleteVariable
============
*/
char *Cvar_CompleteVariable (char *partial)
{
	cvar_t		*cvar;
	int32_t			len;
    hash32_t hash;
	len = strlen(partial);
	
	if (!len)
		return NULL;
		
    hash = Q_Hash32(partial, len);
	// check exact match
	for (cvar=cvar_vars ; cvar ; cvar=cvar->next)
		if (!Q_HashEquals32(hash, cvar->hash) && !strcmp (partial,cvar->name))
			return cvar->name;

	// check partial match
	for (cvar=cvar_vars ; cvar ; cvar=cvar->next)
		if (!strncmp (partial,cvar->name, len))
			return cvar->name;

	return NULL;
}


/*
============
Cvar_Get

If the variable already exists, the value will not be set
The flags will be or'ed in if the variable exists.
============
*/
cvar_t *Cvar_Get (char *var_name, char *var_value, int32_t flags)
{
	cvar_t	*var;
    
	if (flags & (CVAR_USERINFO | CVAR_SERVERINFO))
	{
		if (!Cvar_InfoValidate (var_name))
		{
			Com_Printf("invalid info cvar name\n");
			return NULL;
		}
	}

    var = Cvar_FindVar (var_name);
	if (var)
	{
		var->flags |= flags;
		// Knightmare- added cvar defaults
#ifdef NEW_CVAR_MEMBERS
		if (var_value)
		{
			Z_Free(var->default_string);
			var->default_string = Z_TagStrdup (var_value, ZONE_SYSTEM);
		}
#endif
		return var;
	}

	if (!var_value)
		return NULL;

	if (flags & (CVAR_USERINFO | CVAR_SERVERINFO))
	{
		if (!Cvar_InfoValidate (var_value))
		{
			Com_Printf("invalid info cvar value\n");
			return NULL;
		}
	}

	var = Z_TagMalloc (sizeof(*var), ZONE_SYSTEM);
	var->name = Z_TagStrdup (var_name, ZONE_SYSTEM);
	var->string = Z_TagStrdup (var_value, ZONE_SYSTEM);
	// Knightmare- added cvar defaults
#ifdef NEW_CVAR_MEMBERS
	var->default_string = Z_TagStrdup (var_value, ZONE_SYSTEM);
	var->integer = atoi(var->string);
#endif
	var->modified = true;
	var->value = atof (var->string);
    var->hash = Q_Hash32(var_name, strlen(var_name));

	// link the variable in
	var->next = cvar_vars;
	cvar_vars = var;

	var->flags = flags;

	return var;
}

/*
============
Cvar_Set2
============
*/
cvar_t *Cvar_Set2 (char *var_name, char *value, qboolean force)
{
	cvar_t		*var;

	var = Cvar_FindVar (var_name);
	if (!var)
	{	// create it
		return Cvar_Get (var_name, value, 0);
	}

	if (var->flags & (CVAR_USERINFO | CVAR_SERVERINFO))
	{
		if (!Cvar_InfoValidate (value))
		{
			Com_Printf("invalid info cvar value\n");
			return var;
		}
	}

	if (!force)
	{
		if (var->flags & CVAR_NOSET)
		{
			Com_Printf ("%s is write protected.\n", var_name);
			return var;
		}

		if ((var->flags & CVAR_CHEAT) && !cvar_allowCheats)
		{
			Com_Printf ("%s is cheat protected.\n", var_name);
			return var;
		}

		if (var->flags & CVAR_LATCH)
		{
			if (var->latched_string)
			{
				if (strcmp(value, var->latched_string) == 0)
					return var;
				Z_Free (var->latched_string);
			}
			else
			{
				if (strcmp(value, var->string) == 0)
					return var;
			}

			if (Com_ServerState())
			{
				Com_Printf ("%s will be changed for next game.\n", var_name);
				var->latched_string = Z_TagStrdup(value, ZONE_SYSTEM);
			}
			else
			{
				var->string = Z_TagStrdup(value, ZONE_SYSTEM);
				var->value = atof (var->string);
				if (!strcmp(var->name, "game"))
				{
					FS_SetGamedir (var->string);
					FS_ExecAutoexec ();
				}
			}
			return var;
		}
	}
	else
	{
		if (var->latched_string)
		{
			Z_Free (var->latched_string);
			var->latched_string = NULL;
		}
	}

	if (!strcmp(value, var->string))
		return var;		// not changed

	var->modified = true;

	if (var->flags & CVAR_USERINFO)
		userinfo_modified = true;	// transmit at next oportunity
	
	Z_Free (var->string);	// free the old value string
	
	var->string = Z_TagStrdup(value, ZONE_SYSTEM);
	var->value = atof (var->string);
#ifdef NEW_CVAR_MEMBERS
	var->integer = atoi(var->string);
#endif

	return var;
}

/*
============
Cvar_ForceSet
============
*/
cvar_t *Cvar_ForceSet (char *var_name, char *value)
{
	return Cvar_Set2 (var_name, value, true);
}

/*
============
Cvar_Set
============
*/
cvar_t *Cvar_Set (char *var_name, char *value)
{
	return Cvar_Set2 (var_name, value, false);
}


/*
============
Cvar_SetToDefault
Knightmare added
============
*/
cvar_t *Cvar_SetToDefault (char *var_name)
{
	return Cvar_Set2 (var_name, Cvar_DefaultString(var_name), false);
}


/*
============
Cvar_FullSet
============
*/
cvar_t *Cvar_FullSet (char *var_name, char *value, int32_t flags)
{
	cvar_t	*var;
	
	var = Cvar_FindVar (var_name);
	if (!var)
	{	// create it
		return Cvar_Get (var_name, value, flags);
	}

	var->modified = true;

	if (var->flags & CVAR_USERINFO)
		userinfo_modified = true;	// transmit at next oportunity
	
	Z_Free (var->string);	// free the old value string
	
	var->string = Z_TagStrdup(value, ZONE_SYSTEM);
	var->value = atof (var->string);
	var->flags = flags;

	return var;
}


/*
============
Cvar_SetValue
============
*/
void Cvar_SetValue (char *var_name, float value)
{
	char	val[32];

	if (value == (int32_t)value)
		Com_sprintf (val, sizeof(val), "%i",(int32_t)value);
	else
		Com_sprintf (val, sizeof(val), "%f",value);
	Cvar_Set (var_name, val);
}


/*
=================
Cvar_SetInteger
=================
*/
void Cvar_SetInteger (char *var_name, int32_t integer)
{
	char	val[32];

	Com_sprintf (val, sizeof(val), "%i",integer);
	Cvar_Set (var_name, val);
}


/*
============
Cvar_GetLatchedVars

Any variables with latched values will now be updated
============
*/
void Cvar_GetLatchedVars (void)
{
	cvar_t	*var;

	for (var = cvar_vars ; var ; var = var->next)
	{
		if (!var->latched_string)
			continue;
		Z_Free (var->string);
		var->string = var->latched_string;
		var->latched_string = NULL;
		var->value = atof(var->string);
		if (!strcmp(var->name, "game"))
		{
			FS_SetGamedir (var->string);
			FS_ExecAutoexec ();
		}
	}
}


/*
=================
Cvar_FixCheatVars

Resets cvars that could be used for multiplayer cheating
Borrowed from Q2E
=================
*/
void Cvar_FixCheatVars (qboolean allowCheats)
{

#ifdef NEW_CVAR_MEMBERS
	cvar_t	*var;

	if (cvar_allowCheats == allowCheats)
		return;
	cvar_allowCheats = allowCheats;

	if (cvar_allowCheats)
		return;

	for (var = cvar_vars; var; var = var->next)
	{
		if (!(var->flags & CVAR_CHEAT))
			continue;

		if (!Q_stricmp(var->string, var->default_string))
			continue;

		Cvar_Set2 (var->name, var->default_string, true);
	}
#endif
}


/*
============
Cvar_Command

Handles variable inspection and changing from the console
============
*/
qboolean Cvar_Command (void)
{
	cvar_t			*v;

// check variables
	v = Cvar_FindVar (Cmd_Argv(0));
	if (!v)
		return false;
		
// perform a variable print or set
	if (Cmd_Argc() == 1)
	{	// Knightmare- show latched value if applicable
#ifdef NEW_CVAR_MEMBERS
		if ((v->flags & CVAR_LATCH) && v->latched_string)
			Com_Printf ("\"%s\" is \"%s\" : default is \"%s\" : latched to \"%s\"\n", v->name, v->string, v->default_string, v->latched_string);
		else if (v->flags & CVAR_NOSET)
			Com_Printf ("\"%s\" is \"%s\"\n", v->name, v->string, v->default_string);
		else
			Com_Printf ("\"%s\" is \"%s\" : default is \"%s\"\n", v->name, v->string, v->default_string);
#else
		if ((v->flags & CVAR_LATCH) && v->latched_string)
			Com_Printf ("\"%s\" is \"%s\" : latched to \"%s\"\n", v->name, v->string, v->latched_string);
		else
			Com_Printf ("\"%s\" is \"%s\"\n", v->name, v->string);
#endif
		return true;
	}

	Cvar_Set (v->name, Cmd_Argv(1));
	return true;
}


/*
============
Cvar_Set_f

Allows setting and defining of arbitrary cvars from console
============
*/
void Cvar_Set_f (void)
{
	int32_t		c;
	int32_t		flags;

	c = Cmd_Argc();
	if (c != 3 && c != 4)
	{
		Com_Printf ("usage: set <variable> <value> [u / s]\n");
		return;
	}

	if (c == 4)
	{
		if (!strcmp(Cmd_Argv(3), "u"))
			flags = CVAR_USERINFO;
		else if (!strcmp(Cmd_Argv(3), "s"))
			flags = CVAR_SERVERINFO;
		else
		{
			Com_Printf ("flags can only be 'u' or 's'\n");
			return;
		}
		Cvar_FullSet (Cmd_Argv(1), Cmd_Argv(2), flags);
	}
	else
		Cvar_Set (Cmd_Argv(1), Cmd_Argv(2));
}


/*
=================
Cvar_Toggle_f

Allows toggling of arbitrary cvars from console
=================
*/	
void Cvar_Toggle_f (void)
{
	cvar_t	*var;

	if (Cmd_Argc() != 2){
		Com_Printf("Usage: toggle <variable>\n");
		return;
	}

	var = Cvar_FindVar(Cmd_Argv(1));
	if (!var){
		Com_Printf("'%s' is not a variable\n", Cmd_Argv(1));
		return;
	}
#ifdef NEW_CVAR_MEMBERS
	Cvar_Set2(var->name, va("%i", !var->integer), false);
#else
	Cvar_Set2(var->name, va("%i", (int) !var->value), false);
#endif
}


/*
=================
Cvar_Reset_f

Allows resetting of arbitrary cvars from console
=================
*/
void Cvar_Reset_f (void)
{
	cvar_t *var;
#ifdef NEW_CVAR_MEMBERS
	if (Cmd_Argc() != 2){
		Com_Printf("Usage: reset <variable>\n");
		return;
	}

	var = Cvar_FindVar(Cmd_Argv(1));
	if (!var){
		Com_Printf("'%s' is not a variable\n", Cmd_Argv(1));
		return;
	}


	Cvar_Set2(var->name, var->default_string, false);
#else
	Com_Printf("Error: unsupported command\n");
#endif
}


/*
============
Cvar_WriteVariables

Appends lines containing "set variable value" for all variables
with the archive flag set to true.
============
*/
void Cvar_WriteVariables (char *path)
{
	cvar_t	*var;
	char	buffer[1024];
	FILE	*f;

	f = fopen (path, "a");
	for (var = cvar_vars ; var ; var = var->next)
	{
		if (var->flags & CVAR_ARCHIVE)
		{
			Com_sprintf (buffer, sizeof(buffer), "set %s \"%s\"\n", var->name, var->string);
			fprintf (f, "%s", buffer);
		}
	}
	fclose (f);
}


/*
============
Cvar_List_f
============
*/
void Cvar_List_f (void)
{
	cvar_t	*var;
	int32_t		i, j, c;
	char	*wc;

	// RIOT's Quake3-sytle cvarlist
	c = Cmd_Argc();

	if (c != 1 && c!= 2)
	{
		Com_Printf ("usage: cvarlist [wildcard]\n");
		return;
	}

	if (c == 2)
		wc = Cmd_Argv(1);
	else
		wc = "*";

	i = 0;
	j = 0;
	for (var = cvar_vars; var; var = var->next, i++)
	{
		if (wildcardfit (wc, var->name))
		//if (strstr (var->name, Cmd_Argv(1)))
		{
			j++;
			if (var->flags & CVAR_ARCHIVE)
				Com_Printf ("A");
			else
				Com_Printf (" ");

			if (var->flags & CVAR_USERINFO)
				Com_Printf ("U");
			else
				Com_Printf (" ");

			if (var->flags & CVAR_SERVERINFO)
				Com_Printf ("S");
			else
				Com_Printf (" ");

			if (var->flags & CVAR_NOSET)
				Com_Printf ("-");
			else if (var->flags & CVAR_LATCH)
				Com_Printf ("L");
			else
				Com_Printf (" ");

			if (var->flags & CVAR_CHEAT)
				Com_Printf("C");
			else
				Com_Printf(" ");

			// show latched value if applicable
#ifdef NEW_CVAR_MEMBERS
			if ((var->flags & CVAR_LATCH) && var->latched_string)
				Com_Printf (" %s \"%s\" - default: \"%s\" - latched: \"%s\"\n", var->name, var->string, var->default_string, var->latched_string);
			else
				Com_Printf (" %s \"%s\" - default: \"%s\"\n", var->name, var->string, var->default_string);
#else
			if ((var->flags & CVAR_LATCH) && var->latched_string)
				Com_Printf (" %s \"%s\" - latched: \"%s\"\n", var->name, var->string, var->latched_string);
			else
				Com_Printf (" %s \"%s\"\n", var->name, var->string);

#endif
		}
	}
	Com_Printf (" %i cvars, %i matching\n", i, j);
}


qboolean userinfo_modified;


char	*Cvar_BitInfo (int32_t bit)
{
	static char	info[MAX_INFO_STRING];
	cvar_t	*var;

	info[0] = 0;

	for (var = cvar_vars ; var ; var = var->next)
	{
		if (var->flags & bit)
			Info_SetValueForKey (info, var->name, var->string);
	}
	return info;
}

// returns an info string containing all the CVAR_USERINFO cvars
char	*Cvar_Userinfo (void)
{
	return Cvar_BitInfo (CVAR_USERINFO);
}

// returns an info string containing all the CVAR_SERVERINFO cvars
char	*Cvar_Serverinfo (void)
{
	return Cvar_BitInfo (CVAR_SERVERINFO);
}

/*
============
Cvar_Init

Reads in all archived cvars
============
*/
void Cvar_Init (void)
{
	Cmd_AddCommand ("set", Cvar_Set_f);
	Cmd_AddCommand ("toggle", Cvar_Toggle_f);
	Cmd_AddCommand ("reset", Cvar_Reset_f);
	Cmd_AddCommand ("cvarlist", Cvar_List_f);

}

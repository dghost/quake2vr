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


stable_t cvarNames = {0, 20480};
cvar_t	*cvar_vars[CVAR_HASHMAP_WIDTH];

qboolean	cvar_allowCheats = true;

stable_t cvarValues = {0, 20480};

/*
 ============
 Cvar_RebuildNames
 ============
 */

static void Cvar_RebuildNames (qboolean silent) {
    int i = 0;
    cvar_t *cvar;
    if (!silent)
        Com_Printf(S_COLOR_RED"Rebuilding Cvar Name Table\n");
    for (i = 0; i < CVAR_HASHMAP_WIDTH; i++) {
        for (cvar=cvar_vars[i] ; cvar ; cvar=cvar->next) {
            cvar->name = Q_STGetString(&cvarNames,cvar->index);
            if (!silent)
                Com_Printf(S_COLOR_YELLOW"%s\n", cvar->name);
        }
    }
}

static void Cvar_RebuildValues (qboolean silent) {
    int i = 0;
    cvar_t *cvar;
    if (!silent)
        Com_Printf(S_COLOR_RED"Rebuilding Cvar Value Table\n");
    for (i = 0; i < CVAR_HASHMAP_WIDTH; i++) {
        for (cvar=cvar_vars[i] ; cvar ; cvar=cvar->next) {
            cvar->string = Q_STGetString(&cvarValues,cvar->string_index);
            if ((cvar->flags & CVAR_LATCH)  && (cvar->latched_index >= 0)) {
                cvar->latched_string = Q_STGetString(&cvarValues,cvar->latched_index);
            }
            if (!silent)
                Com_Printf(S_COLOR_YELLOW"%s: "S_COLOR_CYAN"%s "S_COLOR_GREEN"%s\n",cvar->name, cvar->string, cvar->latched_string ? cvar->latched_string : "");

        }
    }
}

static inline void _Grow_Names() {
    Q_STGrow(&cvarNames, cvarNames.size * 2);
    Cvar_RebuildNames(true);
}
static inline void _Cvar_SetName(cvar_t *var, const char *value) {
    var->index = Q_STRegister(&cvarNames, value);
    if (var->index < 0)
    {
        _Grow_Names();
        var->index = Q_STRegister(&cvarNames, value);
    }
    var->name = Q_STGetString(&cvarNames, var->index);
}

static inline void _Grow_Values() {
    Q_STGrow(&cvarValues, cvarValues.size * 2);
    Cvar_RebuildValues(true);
}

static inline void _Cvar_SetValue(cvar_t *var, const char *value) {
    var->string_index = Q_STRegister(&cvarValues, value);
    if (var->string_index < 0)
    {
        _Grow_Values();
        var->string_index = Q_STRegister(&cvarValues, value);
    }
    var->string = Q_STGetString(&cvarValues, var->string_index);
    
    var->value = atof (var->string);
#ifdef NEW_CVAR_MEMBERS
    var->integer = atoi(var->string);
#endif
}

static inline void _Cvar_SetLatchedValue(cvar_t *var, const char *value) {
    var->latched_index = Q_STRegister(&cvarValues, value);
    if (var->latched_index < 0)
    {
        _Grow_Values();
        var->latched_index = Q_STRegister(&cvarValues, value);
    }
    var->latched_string = Q_STGetString(&cvarValues, var->latched_index);
}

static inline void _Cvar_SetDefaultValue(cvar_t *var, const char *value) {
    var->default_index = Q_STRegister(&cvarValues, value);
    if (var->default_index < 0)
    {
        _Grow_Values();
        var->default_index = Q_STRegister(&cvarValues, value);
    }
}


/*
============
Cvar_InfoValidate
============
*/
static qboolean Cvar_InfoValidate (const char *s)
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
static cvar_t *Cvar_FindVar (const char *var_name)
{
	cvar_t	*var;
    char buffer[MAX_TOKEN_CHARS];
    
    Q_strlcpy_lower(buffer, var_name, sizeof(buffer));
    int32_t index = Q_STLookup(&cvarNames,buffer);
    
    if (index >= 0) {
        for (var=cvar_vars[index&CVAR_HASHMAP_MASK] ; var ; var=var->next)
            if (index == var->index)
                return var;
    }
	return NULL;
}


/*
============
Cvar_VariableValue
============
*/
float Cvar_VariableValue (const char *var_name)
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
int32_t Cvar_VariableInteger (const char *var_name)
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
const char *Cvar_VariableString (const char *var_name)
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
float Cvar_DefaultValue (const char *var_name)
{
	cvar_t	*var;
	var = Cvar_FindVar (var_name);
	if (!var)
		return 0;
#ifdef NEW_CVAR_MEMBERS
	return atof (Q_STGetString(&cvarValues,var->default_index));
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
const char *Cvar_DefaultString (const char *var_name)
{
	cvar_t *var;

	var = Cvar_FindVar (var_name);
	if (!var)
		return "";
#ifdef NEW_CVAR_MEMBERS
	return Q_STGetString(&cvarValues,var->default_index);
#else
	return var->string;
#endif
}

/*
============
Cvar_CompleteVariable
============
*/
const char *Cvar_CompleteVariable (const char *partial)
{
	cvar_t		*cvar;
	int32_t			len;
    int32_t index;
    int i;
    char buffer[MAX_TOKEN_CHARS];

	len = strlen(partial);

	if (!len)
		return NULL;
    Q_strlcpy_lower(buffer, partial, sizeof(buffer));
    index = Q_STLookup(&cvarNames,buffer);
    if (index >= 0) {
        return Q_STGetString(&cvarNames, index);
    }
    // check partial match
    for (i = 0; i < CVAR_HASHMAP_WIDTH; i++) {
        for (cvar=cvar_vars[i] ; cvar ; cvar=cvar->next) {
            if (!strncmp(buffer, cvar->name, len))
                return cvar->name;
        }
    }
    return NULL;
}


/*
============
Cvar_Get

If the variable already exists, the value will not be set
The flags will be or'ed in if the variable exists.
============
*/
cvar_t *Cvar_Get (const char *var_name, const char *var_value, int32_t flags)
{
	cvar_t	*var;
    int index;
    char buffer[MAX_TOKEN_CHARS];
    
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
            _Cvar_SetDefaultValue(var, var_value);
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
    Q_strlcpy_lower(buffer, var_name, sizeof(buffer));
    
	var = (cvar_t*)Z_TagMalloc (sizeof(*var), TAG_SYSTEM);
    _Cvar_SetValue(var, var_value);
    
	// Knightmare- added cvar defaults
#ifdef NEW_CVAR_MEMBERS
    _Cvar_SetDefaultValue(var, var_value);
#endif
	var->modified = true;

    _Cvar_SetName(var, buffer);
    
    var->latched_index = -1;
    var->latched_string = NULL;
    
    index = var->index & CVAR_HASHMAP_MASK;
	// link the variable in
	var->next = cvar_vars[index];
    cvar_vars[index] = var;

	var->flags = flags;

	return var;
}

/*
 ============
 Cvar_Set_Internal
 ============
 */
cvar_t *Cvar_Set_Internal (cvar_t *var, const char *value, qboolean force)
{
    assert(var);
    
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
            Com_Printf ("%s is write protected.\n", var->name);
            return var;
        }
        
        if ((var->flags & CVAR_CHEAT) && !cvar_allowCheats)
        {
            Com_Printf ("%s is cheat protected.\n", var->name);
            return var;
        }
        
        if (var->flags & CVAR_LATCH)
        {
            if (var->latched_string)
            {
                if (strcmp(value, var->latched_string) == 0)
                    return var;
            }
            else
            {
                if (strcmp(value, var->string) == 0)
                    return var;
            }
            
            if (Com_ServerState())
            {
                _Cvar_SetLatchedValue(var, value);
                Com_Printf ("%s will be changed for next game.\n", var->name);
            }
            else
            {
                _Cvar_SetValue(var, value);
                
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
            var->latched_string = NULL;
            var->latched_index = -1;
        }
    }
    
    if (!strcmp(value, var->string))
        return var;		// not changed
    
    var->modified = true;
    
    if (var->flags & CVAR_USERINFO)
        userinfo_modified = true;	// transmit at next oportunity
    
    _Cvar_SetValue(var, value);
    
    return var;
}


/*
============
Cvar_Set2
============
*/
cvar_t *Cvar_Set2 (const char *var_name, const char *value, qboolean force)
{
	cvar_t		*var;

	var = Cvar_FindVar (var_name);
	if (!var)
	{	// create it
		return Cvar_Get (var_name, value, 0);
	}

    return Cvar_Set_Internal(var, value, force);
}

/*
============
Cvar_ForceSet
============
*/
cvar_t *Cvar_ForceSet (const char *var_name, const char *value)
{
	return Cvar_Set2 (var_name, value, true);
}

/*
============
Cvar_Set
============
*/
cvar_t *Cvar_Set (const char *var_name, const char *value)
{
	return Cvar_Set2 (var_name, value, false);
}


/*
============
Cvar_SetToDefault
Knightmare added
============
*/
cvar_t *Cvar_SetToDefault (const char *var_name)
{
	return Cvar_Set2 (var_name, Cvar_DefaultString(var_name), false);
}


/*
============
Cvar_FullSet
============
*/
cvar_t *Cvar_FullSet (const char *var_name, const char *value, int32_t flags)
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
	
    _Cvar_SetValue(var, value);
    
	var->flags = flags;

	return var;
}


/*
============
Cvar_SetValue
============
*/
void Cvar_SetValue (const char *var_name, float value)
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
void Cvar_SetInteger (const char *var_name, int32_t integer)
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
    int i;
    for (i = 0; i< CVAR_HASHMAP_WIDTH; i++) {
        for (var = cvar_vars[i] ; var ; var = var->next)
        {
            if (!var->latched_string)
                continue;
            var->string = var->latched_string;
            var->string_index = var->latched_index;
            var->latched_string = NULL;
            var->latched_index = -1;
            var->value = atof(var->string);
#ifdef NEW_CVAR_MEMBERS
            var->integer = atoi(var->string);
#endif
            if (!strcmp(var->name, "game"))
            {
                FS_SetGamedir (var->string);
                FS_ExecAutoexec ();
            }
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
    int i;
    
	if (cvar_allowCheats == allowCheats)
		return;
	cvar_allowCheats = allowCheats;

	if (cvar_allowCheats)
		return;

    for (i = 0; i < CVAR_HASHMAP_WIDTH; i++) {
        for (var = cvar_vars[i]; var; var = var->next)
        {
            const char *default_string;
            if (!(var->flags & CVAR_CHEAT))
                continue;
            default_string = Q_STGetString(&cvarValues,var->default_index);
            
            if (!Q_strcasecmp(var->string, default_string))
                continue;
            
            Cvar_Set_Internal(var, default_string, true);
        }
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
			Com_Printf ("\"%s\" is \"%s\" : default is \"%s\" : latched to \"%s\"\n", v->name, v->string, Q_STGetString(&cvarValues,v->default_index), v->latched_string);
		else if (v->flags & CVAR_NOSET)
			Com_Printf ("\"%s\" is \"%s\"\n", v->name, v->string, Q_STGetString(&cvarValues,v->default_index));
		else
			Com_Printf ("\"%s\" is \"%s\" : default is \"%s\"\n", v->name, v->string, Q_STGetString(&cvarValues,v->default_index));
#else
		if ((v->flags & CVAR_LATCH) && v->latched_string)
			Com_Printf ("\"%s\" is \"%s\" : latched to \"%s\"\n", v->name, v->string, v->latched_string);
		else
			Com_Printf ("\"%s\" is \"%s\"\n", v->name, v->string);
#endif
		return true;
	}

	Cvar_Set_Internal(v, Cmd_Argv(1), false);
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
	Cvar_Set_Internal(var, va("%i", !var->integer), false);
#else
	Cvar_Set_Internal(var, va("%i", (int) !var->value), false);
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


	Cvar_Set_Internal(var, Q_STGetString(&cvarValues,var->default_index), false);
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
void Cvar_WriteVariables (const char *path)
{
	cvar_t	*var;
	char	buffer[1024];
	FILE	*f;
    int i = 0;
    
	f = fopen (path, "a");
    for (i = 0; i < CVAR_HASHMAP_WIDTH; i++) {
        for (var = cvar_vars[i] ; var ; var = var->next)
        {
            if (var->flags & CVAR_ARCHIVE)
            {
                Com_sprintf (buffer, sizeof(buffer), "set %s \"%s\"\n", var->name, var->string);
                fprintf (f, "%s", buffer);
            }
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
	int32_t		i, j, k, c;
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
    for (k = 0; k < CVAR_HASHMAP_WIDTH; k++) {
        for (var = cvar_vars[k]; var; var = var->next, i++)
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
                    Com_Printf (" %s \"%s\" - default: \"%s\" - latched: \"%s\"\n", var->name, var->string, Q_STGetString(&cvarValues,var->default_index), var->latched_string);
                else
                    Com_Printf (" %s \"%s\" - default: \"%s\"\n", var->name, var->string, Q_STGetString(&cvarValues,var->default_index));
#else
                if ((var->flags & CVAR_LATCH) && var->latched_string)
                    Com_Printf (" %s \"%s\" - latched: \"%s\"\n", var->name, var->string, var->latched_string);
                else
                    Com_Printf (" %s \"%s\"\n", var->name, var->string);
                
#endif
            }
        }
    }
	Com_Printf (" %i cvars, %i matching\n", i, j);
    Com_Printf (" %i bytes in use by name table\n", cvarNames.size);
    Com_Printf (" %i bytes in use by value table\n", cvarValues.size);
}

void Cvar_Rebuild_f (void) {
    Cvar_RebuildNames(false);
    Cvar_RebuildValues(false);
}


qboolean userinfo_modified;


char	*Cvar_BitInfo (int32_t bit)
{
	static char	info[MAX_INFO_STRING];
	cvar_t	*var;
    int i;
    
	info[0] = 0;
    for (i = 0; i < CVAR_HASHMAP_WIDTH; i++) {
        for (var = cvar_vars[i] ; var ; var = var->next)
        {
            if (var->flags & bit) {
                Info_SetValueForKey (info, var->name, var->string);
            }
        }
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



void Cvar_AssertRange (cvar_t *var, float min, float max, qboolean isInteger)
{
    if (!var)
        return;
#ifdef NEW_CVAR_MEMBERS
    if (isInteger && (var->value != (float) var->integer))
    {
        Com_Printf (S_COLOR_YELLOW"Warning: cvar '%s' must be an integer (%f)\n", var->name, var->value);
        Cvar_Set_Internal(var, va("%d", var->integer),false);
    }
#else
    if (isInteger && (var->value != (float)((int) var->value)))
    {
        Com_Printf (S_COLOR_YELLOW"Warning: cvar '%s' must be an integer (%f)\n", var->name, var->value);
        Cvar_Set_Internal(var, va("%d", var->integer),false);
    }
#endif
    if (var->value < min)
    {
        Com_Printf (S_COLOR_YELLOW"Warning: cvar '%s' is out of range (%f < %f)\n", var->name, var->value, min);
        Cvar_Set_Internal(var, va("%d", var->integer),false);
    }
    else if (var->value > max)
    {
        Com_Printf (S_COLOR_YELLOW"Warning: cvar '%s' is out of range (%f > %f)\n", var->name, var->value, max);
        Cvar_Set_Internal(var, va("%d", var->integer),false);
    }
}

/*
============
Cvar_Init

Reads in all archived cvars
============
*/
void Cvar_Init (void)
{
    memset(cvar_vars, 0, sizeof(cvar_vars));
    Q_STInit(&cvarValues, MAX_TOKEN_CHARS / 4);

	Cmd_AddCommand ("set", Cvar_Set_f);
	Cmd_AddCommand ("toggle", Cvar_Toggle_f);
	Cmd_AddCommand ("reset", Cvar_Reset_f);
	Cmd_AddCommand ("cvarlist", Cvar_List_f);
    Cmd_AddCommand ("cvarrebuild", Cvar_Rebuild_f);
}

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

// ui_game.c -- the single player menu and credits

#include <ctype.h>
#ifdef _WIN32
#include <io.h>
#endif
#include "../client.h"
#include "include/ui_local.h"


static cvar_t *ui_moddirs_require_library;

struct modname_s {
    const char *dirname;
    const char *longname;
};

static const struct modname_s modnames[] = {
    {"baseq2",  "Quake II VR"},
    {"xatrix",  "The Reckoning"},
    {"rogue",   "Ground Zero"},
    {"lazarus", "Lazarus"},
    {"3tctf",   "3 Team CTF"},
    {0,0}
};

static const char** shortnames;
static const char** longnames;
static int num_names = 0;

static void allocateNameArrays(int numNames) {
    int size = sizeof(char *) * numNames + 1;
    num_names = numNames;
    shortnames = Z_TagMalloc(size, TAG_MENU);
    longnames = Z_TagMalloc(size, TAG_MENU);
}

static void deallocateNameArrays() {
    if (shortnames) {
        Z_Free((void *) shortnames);
        Z_Free((void *) longnames);
        shortnames = NULL;
        longnames = NULL;
    }
    num_names = 0;
}


sset_t mod_name_storage;
sset_t mod_title_storage;

static int currentGame = 0;

static void loadModNames() {
    int i;
    const char *currentGameName = Cvar_VariableString("game");
    sset_t dirs;
    Q_SSetInit(&dirs, 10, 10, TAG_MENU);
    FS_GetGameDirs(&dirs, ui_moddirs_require_library->integer);
    Q_SSetInit(&mod_name_storage, dirs.currentSize, 8, TAG_MENU);
    Q_SSetInit(&mod_title_storage, dirs.currentSize, 16, TAG_MENU);

    for (i = 0; modnames[i].dirname != NULL; i++) {
        if (Q_SSetContains(&dirs, modnames[i].dirname)) {
            if (Q_SSetInsert(&mod_name_storage,modnames[i].dirname))
                Q_SSetInsert(&mod_title_storage,modnames[i].longname);
        }
    }
    
    for (i = 0; i < dirs.currentSize; i++) {
        const char *dir = Q_SSetGetString(&dirs, i);
        if (Q_SSetInsert(&mod_name_storage, dir))
            Q_SSetInsert(&mod_title_storage, dir);
    }
    
    
    allocateNameArrays(mod_name_storage.currentSize);
    Q_SSetGetStrings(&mod_name_storage, shortnames, mod_name_storage.currentSize);
    Q_SSetGetStrings(&mod_title_storage, longnames, mod_title_storage.currentSize);
    Q_SSetFree(&dirs);

    currentGame = -1;
    for (i = 0; i < num_names; i++) {
        Com_Printf("Comparing %s and %s\n", currentGameName, shortnames[i]);
        if (!strcasecmp(currentGameName, shortnames[i])) {
            currentGame = i;
            break;
        }
    }
}

static void unloadModNames() {
    deallocateNameArrays();
    Q_SSetFree(&mod_name_storage);
    Q_SSetFree(&mod_title_storage);
}



/*
=============================================================================

GAME MENU

=============================================================================
*/

static menuframework_s	s_game_mod_menu;
static menulist_s		s_game_mod_choice;

static menulist_s		s_game_mod_filter;

static menulist_s		s_game_mod_legacy;


static menuseparator_s	s_blankline;
static menuaction_s		s_game_mod_accept;


static void refreshModNames() {
    char currentGameOption[1024];
    int i;
    Com_sprintf(currentGameOption, sizeof(currentGameOption), "%s", shortnames[s_game_mod_choice.curvalue]);
    unloadModNames();
    loadModNames();
    for (i = 0; i < num_names; i++) {
        Com_Printf("Comparing %s and %s\n", currentGameOption, shortnames[i]);
        if (!strcasecmp(currentGameOption, shortnames[i])) {
            s_game_mod_choice.curvalue = i;
            break;
        }
    }
    if (s_game_mod_choice.curvalue >= mod_name_storage.currentSize) {
        s_game_mod_choice.curvalue = 0;
    }
    s_game_mod_choice.itemnames = longnames;
}

void SV_StartMod (char *mod);

void Game_Mod_Accept( void *unused ) {
    if (currentGame != s_game_mod_choice.curvalue) {
        char buffer[1024];
        Com_sprintf(buffer, sizeof(buffer), "%s", shortnames[s_game_mod_choice.curvalue]);
        Com_DPrintf("Loading game mod %s\n", buffer);
        UI_ForceMenuOff();
        SV_StartMod(buffer);
    }
}

void Game_Mod_Close ( void ) {
    unloadModNames();
}

void Game_Mod_Filter ( void *unused ) {
    char buff[10];
    Com_sprintf(buff, 10, "%i", s_game_mod_filter.curvalue);
    Cvar_ForceSet("ui_moddirs_require_library", buff);
    Com_sprintf(buff, 10, "%i", s_game_mod_legacy.curvalue);
    Cvar_ForceSet("sv_legacy_libraries", buff);;
    refreshModNames();
}

void Game_Mod_MenuInit( void )
{
	int32_t y = 0;

    static const char *yes_no_names[] =
    {
        "no", "yes", 0
    };

    ui_moddirs_require_library = Cvar_Get("ui_moddirs_require_library", "1", CVAR_ARCHIVE|CVAR_ENGINE);
    
    loadModNames();
    
	s_game_mod_menu.x = SCREEN_WIDTH*0.5 - 24;
	//s_game_menu.y = 0;
	s_game_mod_menu.nitems = 0;
    
    s_game_mod_choice.generic.type = MTYPE_SPINCONTROL;
    s_game_mod_choice.generic.x	= 0;
    s_game_mod_choice.generic.y	= y += MENU_LINE_SIZE;
    s_game_mod_choice.generic.name	= "mod";
    s_game_mod_choice.generic.callback = NULL;
    s_game_mod_choice.itemnames = longnames;
    s_game_mod_choice.curvalue = (currentGame >= 0 ? currentGame : 0);

	s_blankline.generic.type = MTYPE_SEPARATOR;

    s_game_mod_filter.generic.type = MTYPE_SPINCONTROL;
    s_game_mod_filter.generic.x	= 0;
    s_game_mod_filter.generic.y	= y += 2*MENU_LINE_SIZE;
    s_game_mod_filter.generic.name	= "require game library";
    s_game_mod_filter.generic.callback = Game_Mod_Filter;
    s_game_mod_filter.itemnames = yes_no_names;
    s_game_mod_filter.curvalue = ui_moddirs_require_library->integer ? 1 : 0;
    
    s_game_mod_legacy.generic.type = MTYPE_SPINCONTROL;
    s_game_mod_legacy.generic.x	= 0;
    s_game_mod_legacy.generic.y	= y += MENU_LINE_SIZE;
    s_game_mod_legacy.generic.name	= "load legacy libraries";
    s_game_mod_legacy.generic.callback = Game_Mod_Filter;
    s_game_mod_legacy.itemnames = yes_no_names;
    s_game_mod_legacy.curvalue = Cvar_VariableInteger("sv_legacy_libraries") ? 1 : 0;
    s_blankline.generic.type = MTYPE_SEPARATOR;

    s_game_mod_accept.generic.type	= MTYPE_ACTION;
	s_game_mod_accept.generic.flags  = QMF_LEFT_JUSTIFY;
	s_game_mod_accept.generic.x		= 0;
	s_game_mod_accept.generic.y		= y += 2*MENU_LINE_SIZE;
	s_game_mod_accept.generic.name	= " accept";
	s_game_mod_accept.generic.callback = Game_Mod_Accept;

	Menu_AddItem( &s_game_mod_menu, ( void * ) &s_game_mod_choice );

    Menu_AddItem( &s_game_mod_menu, ( void * ) &s_game_mod_filter );
    Menu_AddItem( &s_game_mod_menu, ( void * ) &s_game_mod_legacy );

    
	Menu_AddItem( &s_game_mod_menu, ( void * ) &s_game_mod_accept );

	Menu_Center( &s_game_mod_menu );
}

void Game_Mod_MenuDraw( void )
{
    Menu_DrawBanner( "m_banner_game" );
    Menu_AdjustCursor( &s_game_mod_menu, 1 );
    Menu_Draw( &s_game_mod_menu );
}

const char *Game_Mod_MenuKey( int32_t key )
{
    return Default_MenuKey( &s_game_mod_menu, key );
}

void M_Menu_Game_Mod_f (void)
{
	Game_Mod_MenuInit();
    UI_PushMenu(Game_Mod_MenuDraw, Game_Mod_MenuKey, Game_Mod_Close);
}

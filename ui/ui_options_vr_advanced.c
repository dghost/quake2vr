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

// ui_options_vr.c -- the interface options menu

#include <ctype.h>
#ifdef _WIN32
#include <io.h>
#endif
#include "../client/client.h"
#include "ui_local.h"

/*
=======================================================================

INTERFACE MENU

=======================================================================
*/

static menuframework_s	s_options_vr_advanced_menu;
static menuseparator_s	s_options_vr_advanced_header;
static menulist_s		s_options_vr_advanced_autoenable_box;
static menulist_s		s_options_vr_advanced_nosleep_box;
static menulist_s		s_options_vr_advanced_hudtrans_box;
static menulist_s		s_options_vr_advanced_hudbounce_box;
static menuaction_s		s_options_vr_advanced_defaults_action;
static menuaction_s		s_options_vr_advanced_back_action;


static void AutoFunc( void *unused )
{
	Cvar_SetInteger("vr_autoenable",s_options_vr_advanced_autoenable_box.curvalue);
}

static void SleepFunc( void *unused )
{
	Cvar_SetInteger("vr_nosleep",s_options_vr_advanced_nosleep_box.curvalue);
}

static void BounceFunc( void *unused )
{
	Cvar_SetInteger("vr_hud_bounce",s_options_vr_advanced_hudbounce_box.curvalue);
}

static void TransFunc( void *unused )
{
	Cvar_SetInteger("vr_hud_transparency",s_options_vr_advanced_hudtrans_box.curvalue);
}


static void VRAdvSetMenuItemValues( void )
{
	s_options_vr_advanced_autoenable_box.curvalue = ( Cvar_VariableInteger("vr_autoenable") );
	s_options_vr_advanced_nosleep_box.curvalue = ( Cvar_VariableInteger("vr_nosleep") );
	s_options_vr_advanced_hudtrans_box.curvalue = ( Cvar_VariableInteger("vr_hud_transparency") );
	s_options_vr_advanced_hudbounce_box.curvalue = ( Cvar_VariableInteger("vr_hud_bounce") );
}

static void VRAdvResetDefaultsFunc ( void *unused )
{
	Cvar_SetToDefault("vr_autoenable");
	Cvar_SetToDefault("vr_nosleep");
	Cvar_SetToDefault("vr_hud_transparency");
	Cvar_SetToDefault("vr_hud_bounce");
	VRAdvSetMenuItemValues();
}

void Options_VR_Advanced_MenuInit ( void )
{
	static const char *yesno_names[] =
	{
		"no",
		"yes",
		0
	};

	int y = 3*MENU_LINE_SIZE;

	s_options_vr_advanced_menu.x = SCREEN_WIDTH*0.5;
	s_options_vr_advanced_menu.y = SCREEN_HEIGHT*0.5 - 58;
	s_options_vr_advanced_menu.nitems = 0;

	s_options_vr_advanced_header.generic.type		= MTYPE_SEPARATOR;
	s_options_vr_advanced_header.generic.name		= "vr advanced configuration";
	s_options_vr_advanced_header.generic.x		= MENU_FONT_SIZE/2 * strlen(s_options_vr_advanced_header.generic.name);
	s_options_vr_advanced_header.generic.y		= 0;

	s_options_vr_advanced_autoenable_box.generic.type			= MTYPE_SPINCONTROL;
	s_options_vr_advanced_autoenable_box.generic.x			= MENU_FONT_SIZE;
	s_options_vr_advanced_autoenable_box.generic.y			= y;
	s_options_vr_advanced_autoenable_box.generic.name			= "auto-enable vr mode";
	s_options_vr_advanced_autoenable_box.generic.callback		= AutoFunc;
	s_options_vr_advanced_autoenable_box.itemnames			= yesno_names;
	s_options_vr_advanced_autoenable_box.generic.statusbar	= "automatically enable VR support what starting Quake II VR";

	s_options_vr_advanced_nosleep_box.generic.type			= MTYPE_SPINCONTROL;
	s_options_vr_advanced_nosleep_box.generic.x			= MENU_FONT_SIZE;
	s_options_vr_advanced_nosleep_box.generic.y			= y+=MENU_LINE_SIZE;
	s_options_vr_advanced_nosleep_box.generic.name			= "disable idle thread yielding";
	s_options_vr_advanced_nosleep_box.generic.callback		= SleepFunc;
	s_options_vr_advanced_nosleep_box.itemnames			= yesno_names;
	s_options_vr_advanced_nosleep_box.generic.statusbar	= "prevents stuttering but increases cpu utilization";

	s_options_vr_advanced_hudtrans_box.generic.type			= MTYPE_SPINCONTROL;
	s_options_vr_advanced_hudtrans_box.generic.x			= MENU_FONT_SIZE;
	s_options_vr_advanced_hudtrans_box.generic.y			= y+=2*MENU_LINE_SIZE;
	s_options_vr_advanced_hudtrans_box.generic.name			= "transparent hud";
	s_options_vr_advanced_hudtrans_box.generic.callback		= TransFunc;
	s_options_vr_advanced_hudtrans_box.itemnames			= yesno_names;
	s_options_vr_advanced_hudtrans_box.generic.statusbar	= "enables or disables hud transparency";

	s_options_vr_advanced_hudbounce_box.generic.type			= MTYPE_SPINCONTROL;
	s_options_vr_advanced_hudbounce_box.generic.x			= MENU_FONT_SIZE;
	s_options_vr_advanced_hudbounce_box.generic.y			= y+=MENU_LINE_SIZE;
	s_options_vr_advanced_hudbounce_box.generic.name			= "hud bouncing";
	s_options_vr_advanced_hudbounce_box.generic.callback		= BounceFunc;
	s_options_vr_advanced_hudbounce_box.itemnames			= yesno_names;
	s_options_vr_advanced_hudbounce_box.generic.statusbar	= "enables or disables the hud responding to head tracking";

	s_options_vr_advanced_defaults_action.generic.type		= MTYPE_ACTION;
	s_options_vr_advanced_defaults_action.generic.x			= MENU_FONT_SIZE;
	s_options_vr_advanced_defaults_action.generic.y			= 18*MENU_LINE_SIZE;
	s_options_vr_advanced_defaults_action.generic.name		= "reset defaults";
	s_options_vr_advanced_defaults_action.generic.callback	= VRAdvResetDefaultsFunc;
	s_options_vr_advanced_defaults_action.generic.statusbar	= "resets all interface settings to internal defaults";

	s_options_vr_advanced_back_action.generic.type		= MTYPE_ACTION;
	s_options_vr_advanced_back_action.generic.x			= MENU_FONT_SIZE;
	s_options_vr_advanced_back_action.generic.y			= 20*MENU_LINE_SIZE;
	s_options_vr_advanced_back_action.generic.name		= "back to options";
	s_options_vr_advanced_back_action.generic.callback	= UI_BackMenu;

	Menu_AddItem( &s_options_vr_advanced_menu, ( void * ) &s_options_vr_advanced_header );

	Menu_AddItem( &s_options_vr_advanced_menu, ( void * ) &s_options_vr_advanced_autoenable_box );
	Menu_AddItem( &s_options_vr_advanced_menu, ( void * ) &s_options_vr_advanced_nosleep_box );

	Menu_AddItem( &s_options_vr_advanced_menu, ( void * ) &s_options_vr_advanced_hudtrans_box );
	Menu_AddItem( &s_options_vr_advanced_menu, ( void * ) &s_options_vr_advanced_hudbounce_box );

	Menu_AddItem( &s_options_vr_advanced_menu, ( void * ) &s_options_vr_advanced_defaults_action );
	Menu_AddItem( &s_options_vr_advanced_menu, ( void * ) &s_options_vr_advanced_back_action );

	VRAdvSetMenuItemValues();
}

void Options_VR_Advanced_MenuDraw (void)
{
	Menu_DrawBanner( "m_banner_options" );
	Menu_AdjustCursor( &s_options_vr_advanced_menu, 1 );
	Menu_Draw( &s_options_vr_advanced_menu );
}

const char *Options_VR_Advanced_MenuKey( int key )
{
	return Default_MenuKey( &s_options_vr_advanced_menu, key );
}

void M_Menu_Options_VR_Advanced_f (void)
{
	Options_VR_Advanced_MenuInit();
	UI_PushMenu ( Options_VR_Advanced_MenuDraw, Options_VR_Advanced_MenuKey );
}

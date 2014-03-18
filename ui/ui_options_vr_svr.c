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

static menuframework_s	s_options_vr_svr_menu;
static menuseparator_s	s_options_vr_svr_header;
static menulist_s		s_options_vr_svr_enable_box;
static menulist_s		s_options_vr_svr_distortion_box;

static menuaction_s		s_options_vr_svr_defaults_action;
static menuaction_s		s_options_vr_svr_back_action;

static void DistortionFunc( void *unused )
{
	Cvar_SetInteger( "vr_svr_distortion", 3 - s_options_vr_svr_distortion_box.curvalue  );
}

static void EnableFunc( void *unused)
{
	Cvar_SetInteger("vr_svr_enable", s_options_vr_svr_enable_box.curvalue);

}
static void VRSVRSetMenuItemValues( void )
{
	s_options_vr_svr_enable_box.curvalue = ( !! Cvar_VariableInteger("vr_svr_enable") );
	s_options_vr_svr_distortion_box.curvalue = ( 3 - ClampCvar(0, 3, Cvar_VariableInteger("vr_svr_distortion")) );
}

static void VRSVRResetDefaultsFunc ( void *unused )
{
	Cvar_SetToDefault ("vr_svr_enable");
	Cvar_SetToDefault ("vr_svr_distortion");

	VRSVRSetMenuItemValues();
}

static void VRSVRConfigAccept (void)
{
	// placeholder in case needed in future
	DistortionFunc(NULL);
}

static void BackFunc ( void *unused )
{
	VRSVRConfigAccept();
	UI_BackMenu(NULL);
}

void Options_VR_SVR_MenuInit ( void )
{
	static const char *distortion_names[] =
	{
		"low",
		"medium",
		"high*",
		"full*",
		0
	};

	static const char *enable_names[] =
	{
		"disable",
		"enable",
		0
	};

	Sint32 y = 3*MENU_LINE_SIZE;

	s_options_vr_svr_menu.x = SCREEN_WIDTH*0.5;
	s_options_vr_svr_menu.y = SCREEN_HEIGHT*0.5 - 58;
	s_options_vr_svr_menu.nitems = 0;

	s_options_vr_svr_header.generic.type		= MTYPE_SEPARATOR;
	s_options_vr_svr_header.generic.name		= "steam vr configuration";
	s_options_vr_svr_header.generic.x		= MENU_FONT_SIZE/2 * strlen(s_options_vr_svr_header.generic.name);
	s_options_vr_svr_header.generic.y		= 0;

	
	s_options_vr_svr_enable_box.generic.type		= MTYPE_SPINCONTROL;
	s_options_vr_svr_enable_box.generic.x			= MENU_FONT_SIZE;
	s_options_vr_svr_enable_box.generic.y			= y;
	s_options_vr_svr_enable_box.generic.name		= "steam vr support";
	s_options_vr_svr_enable_box.generic.callback	= EnableFunc;
	s_options_vr_svr_enable_box.itemnames			= enable_names;
	s_options_vr_svr_enable_box.generic.statusbar	= "enable or disable native steam vr support";

	s_options_vr_svr_distortion_box.generic.type			= MTYPE_SPINCONTROL;
	s_options_vr_svr_distortion_box.generic.x			= MENU_FONT_SIZE;
	s_options_vr_svr_distortion_box.generic.y			= y+=MENU_LINE_SIZE;
	s_options_vr_svr_distortion_box.generic.name			= "distortion quality";
	//s_options_vr_svr_distortion_box.generic.callback		= DistortionFunc;
	s_options_vr_svr_distortion_box.itemnames			= distortion_names;
	s_options_vr_svr_distortion_box.generic.statusbar	= "adjusts the quality of distortion applied *not recommended";

	s_options_vr_svr_defaults_action.generic.type		= MTYPE_ACTION;
	s_options_vr_svr_defaults_action.generic.x			= MENU_FONT_SIZE;
	s_options_vr_svr_defaults_action.generic.y			= y+=2*MENU_LINE_SIZE;
	s_options_vr_svr_defaults_action.generic.name		= "reset defaults";
	s_options_vr_svr_defaults_action.generic.callback	= VRSVRResetDefaultsFunc;
	s_options_vr_svr_defaults_action.generic.statusbar	= "resets all oculus rift settings to internal defaults";

	s_options_vr_svr_back_action.generic.type		= MTYPE_ACTION;
	s_options_vr_svr_back_action.generic.x			= MENU_FONT_SIZE;
	s_options_vr_svr_back_action.generic.y			= y+=2*MENU_LINE_SIZE;
	s_options_vr_svr_back_action.generic.name		= "back to options";
	s_options_vr_svr_back_action.generic.callback	= BackFunc;

	Menu_AddItem( &s_options_vr_svr_menu, ( void * ) &s_options_vr_svr_header );

	Menu_AddItem( &s_options_vr_svr_menu, ( void * ) &s_options_vr_svr_enable_box );

	Menu_AddItem( &s_options_vr_svr_menu, ( void * ) &s_options_vr_svr_distortion_box );
	
	Menu_AddItem( &s_options_vr_svr_menu, ( void * ) &s_options_vr_svr_defaults_action );
	Menu_AddItem( &s_options_vr_svr_menu, ( void * ) &s_options_vr_svr_back_action );

	VRSVRSetMenuItemValues();
}

void Options_VR_SVR_MenuDraw (void)
{
	Menu_DrawBanner( "m_banner_options" );
	Menu_AdjustCursor( &s_options_vr_svr_menu, 1 );
	Menu_Draw( &s_options_vr_svr_menu );
}

const char *Options_VR_SVR_MenuKey( Sint32 key )
{
	if ( key == K_ESCAPE		|| key == K_GAMEPAD_B
		|| key == K_GAMEPAD_BACK	|| key == K_GAMEPAD_START
		)
		VRSVRConfigAccept();
	return Default_MenuKey( &s_options_vr_svr_menu, key );
}

void M_Menu_Options_VR_SVR_f (void)
{
	Options_VR_SVR_MenuInit();
	UI_PushMenu ( Options_VR_SVR_MenuDraw, Options_VR_SVR_MenuKey );
}

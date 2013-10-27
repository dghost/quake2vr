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

static menuframework_s	s_options_vr_ovr_menu;
static menuseparator_s	s_options_vr_ovr_header;
static menulist_s		s_options_vr_ovr_drift_box;
static menulist_s		s_options_vr_ovr_chroma_box;
static menulist_s		s_options_vr_ovr_distortion_box;
static menulist_s		s_options_vr_ovr_scale_box;
static menulist_s		s_options_vr_ovr_debug_box;
static menuaction_s		s_options_vr_ovr_defaults_action;
static menuaction_s		s_options_vr_ovr_back_action;


static void DriftFunc( void *unused )
{
	Cvar_SetInteger( "vr_ovr_driftcorrection", s_options_vr_ovr_drift_box.curvalue);
}

static void ChromaFunc( void *unused )
{
	Cvar_SetInteger( "vr_ovr_chromatic", s_options_vr_ovr_chroma_box.curvalue);
}

static void DistortionFunc( void *unused )
{
	Cvar_SetInteger( "vr_ovr_distortion", s_options_vr_ovr_distortion_box.curvalue);
}

static void ScaleFunc( void *unused )
{
	Cvar_SetInteger( "vr_ovr_scale", s_options_vr_ovr_scale_box.curvalue - 1);
}

static void DebugFunc( void *unused)
{
	Cvar_SetInteger("vr_ovr_debug",s_options_vr_ovr_debug_box.curvalue);
}
static void VROVRSetMenuItemValues( void )
{
	s_options_vr_ovr_drift_box.curvalue = ( Cvar_VariableInteger("vr_ovr_driftcorrection") );
	s_options_vr_ovr_chroma_box.curvalue = ( Cvar_VariableInteger("vr_ovr_chromatic") );
	s_options_vr_ovr_distortion_box.curvalue = ( Cvar_VariableInteger("vr_ovr_distortion") );
	s_options_vr_ovr_scale_box.curvalue = ( Cvar_VariableInteger("vr_ovr_scale")  + 1 );
	s_options_vr_ovr_debug_box.curvalue = ( Cvar_VariableInteger("vr_ovr_debug") );
}

static void VROVRResetDefaultsFunc ( void *unused )
{
	Cvar_SetToDefault ("vr_ovr_driftcorrection");
	Cvar_SetToDefault ("vr_ovr_chromatic");
	Cvar_SetToDefault ("vr_ovr_distortion");
	Cvar_SetToDefault ("vr_ovr_scale");
	Cvar_SetToDefault ("vr_ovr_debug");
	VROVRSetMenuItemValues();
}

void Options_VR_OVR_MenuInit ( void )
{
	static const char *yesno_names[] =
	{
		"no",
		"yes",
		0
	};

	static const char *scale_names[] = 
	{
		"inner edge",
		"outer edge",
		"none",
		0
	};

	static const char *debug_names[] =
	{
		"disabled",
		"DK1 emulation",
		"HD DK emulation",
		0
	};

	int y = 3*MENU_LINE_SIZE;

	s_options_vr_ovr_menu.x = SCREEN_WIDTH*0.5;
	s_options_vr_ovr_menu.y = SCREEN_HEIGHT*0.5 - 58;
	s_options_vr_ovr_menu.nitems = 0;

	s_options_vr_ovr_header.generic.type		= MTYPE_SEPARATOR;
	s_options_vr_ovr_header.generic.name		= "oculus rift configuration";
	s_options_vr_ovr_header.generic.x		= MENU_FONT_SIZE/2 * strlen(s_options_vr_ovr_header.generic.name);
	s_options_vr_ovr_header.generic.y		= 0;


	s_options_vr_ovr_drift_box.generic.type			= MTYPE_SPINCONTROL;
	s_options_vr_ovr_drift_box.generic.x			= MENU_FONT_SIZE;
	s_options_vr_ovr_drift_box.generic.y			= y;
	s_options_vr_ovr_drift_box.generic.name			= "drift correction";
	s_options_vr_ovr_drift_box.generic.callback		= DriftFunc;
	s_options_vr_ovr_drift_box.itemnames			= yesno_names;
	s_options_vr_ovr_drift_box.generic.statusbar	= "enable magnetic drift correction";

	s_options_vr_ovr_chroma_box.generic.type			= MTYPE_SPINCONTROL;
	s_options_vr_ovr_chroma_box.generic.x			= MENU_FONT_SIZE;
	s_options_vr_ovr_chroma_box.generic.y			= y+=MENU_LINE_SIZE;
	s_options_vr_ovr_chroma_box.generic.name			= "chromatic correction";
	s_options_vr_ovr_chroma_box.generic.callback		= ChromaFunc;
	s_options_vr_ovr_chroma_box.itemnames			= yesno_names;
	s_options_vr_ovr_chroma_box.generic.statusbar	= "applies chromatic aberration correction to the distortion shader";


	s_options_vr_ovr_scale_box.generic.type			= MTYPE_SPINCONTROL;
	s_options_vr_ovr_scale_box.generic.x			= MENU_FONT_SIZE;
	s_options_vr_ovr_scale_box.generic.y			= y+=2*MENU_LINE_SIZE;
	s_options_vr_ovr_scale_box.generic.name			= "distortion scaling";
	s_options_vr_ovr_scale_box.generic.callback		= ScaleFunc;
	s_options_vr_ovr_scale_box.itemnames			= scale_names;
	s_options_vr_ovr_scale_box.generic.statusbar	= "adjusts the size of the post-distortion view";

	s_options_vr_ovr_distortion_box.generic.type			= MTYPE_SPINCONTROL;
	s_options_vr_ovr_distortion_box.generic.x			= MENU_FONT_SIZE;
	s_options_vr_ovr_distortion_box.generic.y			= y+=MENU_LINE_SIZE;
	s_options_vr_ovr_distortion_box.generic.name			= "distortion";
	s_options_vr_ovr_distortion_box.generic.callback		= DistortionFunc;
	s_options_vr_ovr_distortion_box.itemnames			= yesno_names;
	s_options_vr_ovr_distortion_box.generic.statusbar	= "enables the barrel distortion shader";

	s_options_vr_ovr_debug_box.generic.type			= MTYPE_SPINCONTROL;
	s_options_vr_ovr_debug_box.generic.x			= MENU_FONT_SIZE;
	s_options_vr_ovr_debug_box.generic.y			= y+=2*MENU_LINE_SIZE;
	s_options_vr_ovr_debug_box.generic.name			= "debug mode";
	s_options_vr_ovr_debug_box.generic.callback		= DebugFunc;
	s_options_vr_ovr_debug_box.itemnames			= debug_names;
	s_options_vr_ovr_debug_box.generic.statusbar	= "enables oculus rift debugging support";

	s_options_vr_ovr_defaults_action.generic.type		= MTYPE_ACTION;
	s_options_vr_ovr_defaults_action.generic.x			= MENU_FONT_SIZE;
	s_options_vr_ovr_defaults_action.generic.y			= 18*MENU_LINE_SIZE;
	s_options_vr_ovr_defaults_action.generic.name		= "reset defaults";
	s_options_vr_ovr_defaults_action.generic.callback	= VROVRResetDefaultsFunc;
	s_options_vr_ovr_defaults_action.generic.statusbar	= "resets all interface settings to internal defaults";

	s_options_vr_ovr_back_action.generic.type		= MTYPE_ACTION;
	s_options_vr_ovr_back_action.generic.x			= MENU_FONT_SIZE;
	s_options_vr_ovr_back_action.generic.y			= 20*MENU_LINE_SIZE;
	s_options_vr_ovr_back_action.generic.name		= "back to options";
	s_options_vr_ovr_back_action.generic.callback	= UI_BackMenu;

	Menu_AddItem( &s_options_vr_ovr_menu, ( void * ) &s_options_vr_ovr_header );

	Menu_AddItem( &s_options_vr_ovr_menu, ( void * ) &s_options_vr_ovr_drift_box );
	Menu_AddItem( &s_options_vr_ovr_menu, ( void * ) &s_options_vr_ovr_chroma_box );
	Menu_AddItem( &s_options_vr_ovr_menu, ( void * ) &s_options_vr_ovr_distortion_box );
	Menu_AddItem( &s_options_vr_ovr_menu, ( void * ) &s_options_vr_ovr_scale_box );
	Menu_AddItem( &s_options_vr_ovr_menu, ( void * ) &s_options_vr_ovr_debug_box );	
	Menu_AddItem( &s_options_vr_ovr_menu, ( void * ) &s_options_vr_ovr_defaults_action );
	Menu_AddItem( &s_options_vr_ovr_menu, ( void * ) &s_options_vr_ovr_back_action );

	VROVRSetMenuItemValues();
}

void Options_VR_OVR_MenuDraw (void)
{
	Menu_DrawBanner( "m_banner_options" );
	Menu_AdjustCursor( &s_options_vr_ovr_menu, 1 );
	Menu_Draw( &s_options_vr_ovr_menu );
}

const char *Options_VR_OVR_MenuKey( int key )
{
	return Default_MenuKey( &s_options_vr_ovr_menu, key );
}

void M_Menu_Options_VR_OVR_f (void)
{
	Options_VR_OVR_MenuInit();
	UI_PushMenu ( Options_VR_OVR_MenuDraw, Options_VR_OVR_MenuKey );
}

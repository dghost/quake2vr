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

static menuframework_s	s_options_vr_menu;
static menuseparator_s	s_options_vr_header;
static menulist_s		s_options_vr_aimmode_box;
static menulist_s		s_options_vr_crosshair_box;
static menuslider_s		s_options_vr_crosshair_bright_slider;
static menuslider_s		s_options_vr_crosshair_size_slider;
static menuslider_s		s_options_vr_hud_depth_slider;
static menuslider_s		s_options_vr_hud_fov_slider;
static menulist_s		s_options_vr_neckmodel_box;
static menuaction_s		s_options_vr_enable_action;
static menuaction_s		s_options_vr_advanced_action;
static menuaction_s		s_options_vr_ovr_action;
static menuaction_s		s_options_vr_defaults_action;
static menuaction_s		s_options_vr_back_action;


static void AimmodeFunc( void *unused )
{
	Cvar_SetInteger( "vr_aimmode", s_options_vr_aimmode_box.curvalue);
}


static void EnableDisableFunc( void *unused )
{
	if (Cvar_VariableInteger("vr_enabled") > 0)
		Cmd_ExecuteString("vr_disable");
	else
		Cmd_ExecuteString("vr_enable");
}

static void CrosshairFunc( void *unused )
{
	Cvar_SetInteger( "vr_crosshair", s_options_vr_crosshair_box.curvalue);
	Cvar_SetInteger( "vr_crosshair_brightness", s_options_vr_crosshair_bright_slider.curvalue);
	Cvar_SetValue( "vr_crosshair_size", s_options_vr_crosshair_size_slider.curvalue / 4.0f);
}

static void HUDFunc( void *unused )
{
	Cvar_SetValue( "vr_hud_depth", s_options_vr_hud_depth_slider.curvalue / 20.0f);
	Cvar_SetInteger( "vr_hud_fov", s_options_vr_hud_fov_slider.curvalue);
}

static void NeckFunc( void *unused )
{
	Cvar_SetInteger( "vr_neckmodel", s_options_vr_neckmodel_box.curvalue );
}

static void VRSetMenuItemValues( void )
{
	s_options_vr_aimmode_box.curvalue = ( Cvar_VariableValue("vr_aimmode") );
	s_options_vr_crosshair_box.curvalue = ( Cvar_VariableValue("vr_crosshair") );
	s_options_vr_crosshair_bright_slider.curvalue = ( Cvar_VariableValue("vr_crosshair_brightness") );
	s_options_vr_crosshair_size_slider.curvalue = ( Cvar_VariableValue("vr_crosshair_size") * 4.0 );
	s_options_vr_hud_depth_slider.curvalue = ( Cvar_VariableValue("vr_hud_depth") * 20.0f);
	s_options_vr_hud_fov_slider.curvalue = ( Cvar_VariableValue("vr_hud_fov") );
	s_options_vr_neckmodel_box.curvalue = ( Cvar_VariableValue("vr_neckmodel") );
}

static void VRResetDefaultsFunc ( void *unused )
{
	Cvar_SetToDefault ("vr_aimmode");
	Cvar_SetToDefault ("vr_crosshair");
	Cvar_SetToDefault ("vr_crosshair_brightness");
	Cvar_SetToDefault ("vr_crosshair_size");
	Cvar_SetToDefault ("vr_hud_depth");
	Cvar_SetToDefault ("vr_hud_fov");
	Cvar_SetToDefault ("vr_neckmodel");
	VRSetMenuItemValues();
}

void Options_VR_MenuInit ( void )
{
	static const char *yesno_names[] =
	{
		"no",
		"yes",
		0
	};

	static const char *aimmode_names[] =
	{
		"no head tracking",
		"head aiming + mouse steering",
		"head aiming + mouse pitch/steering",
		"mouse aiming",
		"mouse aiming + mouse pitch",
		"TF2 Mode 2 (bounded deadzone)",
		"TF2 Mode 3 (view move)",
		"TF2 Mode 4 (aim move)",
		"fully decoupled view/aiming",
		0
	};

	static const char *crosshair_names[] =
	{
		"none",
		"dot",
		0
	};

	int y = 3*MENU_LINE_SIZE;

	s_options_vr_menu.x = SCREEN_WIDTH*0.5;
	s_options_vr_menu.y = SCREEN_HEIGHT*0.5 - 58;
	s_options_vr_menu.nitems = 0;

	s_options_vr_header.generic.type		= MTYPE_SEPARATOR;
	s_options_vr_header.generic.name		= "vr configuration";
	s_options_vr_header.generic.x		= MENU_FONT_SIZE/2 * strlen(s_options_vr_header.generic.name);
	s_options_vr_header.generic.y		= 0;

	s_options_vr_aimmode_box.generic.type			= MTYPE_SPINCONTROL;
	s_options_vr_aimmode_box.generic.x			= MENU_FONT_SIZE;
	s_options_vr_aimmode_box.generic.y			= y;
	s_options_vr_aimmode_box.generic.name			= "aim mode";
	s_options_vr_aimmode_box.generic.callback		= AimmodeFunc;
	s_options_vr_aimmode_box.itemnames			= aimmode_names;
	s_options_vr_aimmode_box.generic.statusbar	= "select which aim mode to use";

	s_options_vr_crosshair_box.generic.type			= MTYPE_SPINCONTROL;
	s_options_vr_crosshair_box.generic.x			= MENU_FONT_SIZE;
	s_options_vr_crosshair_box.generic.y			= y+=2*MENU_LINE_SIZE;
	s_options_vr_crosshair_box.generic.name			= "crosshair type";
	s_options_vr_crosshair_box.generic.callback		= CrosshairFunc;
	s_options_vr_crosshair_box.itemnames			= crosshair_names;
	s_options_vr_crosshair_box.generic.statusbar	= "select what type of crosshair is enabled";

	s_options_vr_crosshair_bright_slider.generic.type		= MTYPE_SLIDER;
	s_options_vr_crosshair_bright_slider.generic.x			= MENU_FONT_SIZE;
	s_options_vr_crosshair_bright_slider.generic.y			= y+=MENU_LINE_SIZE;
	s_options_vr_crosshair_bright_slider.generic.name		= "crosshair brightness";
	s_options_vr_crosshair_bright_slider.generic.callback	= CrosshairFunc;
	s_options_vr_crosshair_bright_slider.minvalue			= 1;
	s_options_vr_crosshair_bright_slider.maxvalue			= 100;
	s_options_vr_crosshair_bright_slider.generic.statusbar	= "changes opacity of crosshair";

	s_options_vr_crosshair_size_slider.generic.type		= MTYPE_SLIDER;
	s_options_vr_crosshair_size_slider.generic.x			= MENU_FONT_SIZE;
	s_options_vr_crosshair_size_slider.generic.y			= y+=MENU_LINE_SIZE;
	s_options_vr_crosshair_size_slider.generic.name		= "crosshair size";
	s_options_vr_crosshair_size_slider.generic.callback	= CrosshairFunc;
	s_options_vr_crosshair_size_slider.minvalue			= 1;
	s_options_vr_crosshair_size_slider.maxvalue			= 20;
	s_options_vr_crosshair_size_slider.generic.statusbar	= "changes size of crosshair";

	s_options_vr_neckmodel_box.generic.type			= MTYPE_SPINCONTROL;
	s_options_vr_neckmodel_box.generic.x			= MENU_FONT_SIZE;
	s_options_vr_neckmodel_box.generic.y			= y+=2*MENU_LINE_SIZE;
	s_options_vr_neckmodel_box.generic.name			= "enable head/neck model";
	s_options_vr_neckmodel_box.generic.callback		= NeckFunc;
	s_options_vr_neckmodel_box.itemnames			= yesno_names;
	s_options_vr_neckmodel_box.generic.statusbar	= "enable or disable the head/neck model";

	s_options_vr_hud_depth_slider.generic.type		= MTYPE_SLIDER;
	s_options_vr_hud_depth_slider.generic.x			= MENU_FONT_SIZE;
	s_options_vr_hud_depth_slider.generic.y			= y+=2*MENU_LINE_SIZE;
	s_options_vr_hud_depth_slider.generic.name		= "hud depth";
	s_options_vr_hud_depth_slider.generic.callback	= HUDFunc;
	s_options_vr_hud_depth_slider.minvalue			= 5;
	s_options_vr_hud_depth_slider.maxvalue			= 100;
	s_options_vr_hud_depth_slider.generic.statusbar	= "changes HUD depth";

	s_options_vr_hud_fov_slider.generic.type		= MTYPE_SLIDER;
	s_options_vr_hud_fov_slider.generic.x			= MENU_FONT_SIZE;
	s_options_vr_hud_fov_slider.generic.y			= y+=MENU_LINE_SIZE;
	s_options_vr_hud_fov_slider.generic.name		= "hud fov";
	s_options_vr_hud_fov_slider.generic.callback	= HUDFunc;
	s_options_vr_hud_fov_slider.minvalue			= 30;
	s_options_vr_hud_fov_slider.maxvalue			= 90;
	s_options_vr_hud_fov_slider.generic.statusbar	= "changes size of the HUD";

	s_options_vr_advanced_action.generic.type		= MTYPE_ACTION;
	s_options_vr_advanced_action.generic.x			= MENU_FONT_SIZE;
	s_options_vr_advanced_action.generic.y			= y+=2*MENU_LINE_SIZE;
	s_options_vr_advanced_action.generic.name		= "advanced options";
	s_options_vr_advanced_action.generic.callback	= M_Menu_Options_VR_Advanced_f;
	s_options_vr_advanced_action.generic.statusbar	= "advanced configuration options";

	s_options_vr_ovr_action.generic.type		= MTYPE_ACTION;
	s_options_vr_ovr_action.generic.x			= MENU_FONT_SIZE;
	s_options_vr_ovr_action.generic.y			= y+=MENU_LINE_SIZE;
	s_options_vr_ovr_action.generic.name		= "oculus rift options";
	s_options_vr_ovr_action.generic.callback	= M_Menu_Options_VR_OVR_f;
	s_options_vr_ovr_action.generic.statusbar	= "oculus rift configuration";


	s_options_vr_enable_action.generic.type		= MTYPE_ACTION;
	s_options_vr_enable_action.generic.x			= MENU_FONT_SIZE;
	s_options_vr_enable_action.generic.y			= y+=2*MENU_LINE_SIZE;
	s_options_vr_enable_action.generic.name		= "enable/disable vr";
	s_options_vr_enable_action.generic.callback	= EnableDisableFunc;
	s_options_vr_enable_action.generic.statusbar	= "enable or disable virtual reality support";

	s_options_vr_defaults_action.generic.type		= MTYPE_ACTION;
	s_options_vr_defaults_action.generic.x			= MENU_FONT_SIZE;
	s_options_vr_defaults_action.generic.y			= y+=MENU_LINE_SIZE;
	s_options_vr_defaults_action.generic.name		= "reset defaults";
	s_options_vr_defaults_action.generic.callback	= VRResetDefaultsFunc;
	s_options_vr_defaults_action.generic.statusbar	= "resets all interface settings to internal defaults";

	s_options_vr_back_action.generic.type		= MTYPE_ACTION;
	s_options_vr_back_action.generic.x			= MENU_FONT_SIZE;
	s_options_vr_back_action.generic.y			= y+=2*MENU_LINE_SIZE;
	s_options_vr_back_action.generic.name		= "back to options";
	s_options_vr_back_action.generic.callback	= UI_BackMenu;

	
	Menu_AddItem( &s_options_vr_menu, ( void * ) &s_options_vr_header );
	Menu_AddItem( &s_options_vr_menu, ( void * ) &s_options_vr_aimmode_box );
	Menu_AddItem( &s_options_vr_menu, ( void * ) &s_options_vr_crosshair_box );
	Menu_AddItem( &s_options_vr_menu, ( void * ) &s_options_vr_crosshair_bright_slider );
	Menu_AddItem( &s_options_vr_menu, ( void * ) &s_options_vr_crosshair_size_slider );
	Menu_AddItem( &s_options_vr_menu, ( void * ) &s_options_vr_neckmodel_box );
	Menu_AddItem( &s_options_vr_menu, ( void * ) &s_options_vr_hud_depth_slider );
	Menu_AddItem( &s_options_vr_menu, ( void * ) &s_options_vr_hud_fov_slider );
	Menu_AddItem( &s_options_vr_menu, ( void * ) &s_options_vr_advanced_action );
	Menu_AddItem( &s_options_vr_menu, ( void * ) &s_options_vr_ovr_action );
	Menu_AddItem( &s_options_vr_menu, ( void * ) &s_options_vr_enable_action );
	Menu_AddItem( &s_options_vr_menu, ( void * ) &s_options_vr_defaults_action );
	Menu_AddItem( &s_options_vr_menu, ( void * ) &s_options_vr_back_action );

	VRSetMenuItemValues();
}

void Options_VR_MenuDraw (void)
{
	Menu_DrawBanner( "m_banner_options" );
	Menu_AdjustCursor( &s_options_vr_menu, 1 );
	Menu_Draw( &s_options_vr_menu );
}

const char *Options_VR_MenuKey( int key )
{
	return Default_MenuKey( &s_options_vr_menu, key );
}

void M_Menu_Options_VR_f (void)
{
	Options_VR_MenuInit();
	UI_PushMenu ( Options_VR_MenuDraw, Options_VR_MenuKey );
}

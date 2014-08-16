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
#include "../client.h"
#include "include/ui_local.h"

/*
=======================================================================

INTERFACE MENU

=======================================================================
*/

static menuframework_s	s_options_vr_menu;
static menuseparator_s	s_options_vr_header;
static menulist_s		s_options_vr_aimmode_box;
static menufield_s		s_options_vr_aimmode_deadzone_pitch_field;
static menufield_s		s_options_vr_aimmode_deadzone_yaw_field;
static menulist_s		s_options_vr_viewmove_box;

static menuslider_s		s_options_vr_walkspeed_slider;

static menulist_s		s_options_vr_autoipd_box;
static menufield_s		s_options_vr_ipd_field;

static menuaction_s		s_options_vr_enable_action;
static menuaction_s		s_options_vr_advanced_action;
static menuaction_s		s_options_vr_ovr_action;
static menuaction_s		s_options_vr_svr_action;

static menuaction_s		s_options_vr_defaults_action;
static menuaction_s		s_options_vr_back_action;

extern cvar_t *vr_aimmode_deadzone_yaw;
extern cvar_t *vr_aimmode_deadzone_pitch;
extern cvar_t *vr_ipd;

static void AimmodeFunc( void *unused )
{
	Cvar_SetInteger( "vr_aimmode", s_options_vr_aimmode_box.curvalue);
}

static void ViewmodeFunc( void *unused )
{
	Cvar_SetInteger( "vr_viewmove", s_options_vr_viewmove_box.curvalue);
}

static void EnableDisableFunc( void *unused )
{
	if (Cvar_VariableInteger("vr_enabled") > 0)
		Cmd_ExecuteString("vr_disable");
	else
		Cmd_ExecuteString("vr_enable");
}

static void AutoIPDFunc ( void *unused )
{
	Cvar_SetInteger( "vr_autoipd", s_options_vr_autoipd_box.curvalue);
}

static void VRSetMenuItemValues( void )
{
	s_options_vr_aimmode_box.curvalue = ( Cvar_VariableValue("vr_aimmode") );
	s_options_vr_viewmove_box.curvalue = ( Cvar_VariableValue("vr_viewmove") );
	s_options_vr_autoipd_box.curvalue = ( Cvar_VariableValue("vr_autoipd") );
	strcpy( s_options_vr_aimmode_deadzone_pitch_field.buffer, vr_aimmode_deadzone_pitch->string );
	s_options_vr_aimmode_deadzone_pitch_field.cursor = strlen( vr_aimmode_deadzone_pitch->string );
	strcpy( s_options_vr_aimmode_deadzone_yaw_field.buffer, vr_aimmode_deadzone_yaw->string );
	s_options_vr_aimmode_deadzone_yaw_field.cursor = strlen( vr_aimmode_deadzone_yaw->string );
	strcpy( s_options_vr_ipd_field.buffer, vr_ipd->string );
	s_options_vr_ipd_field.cursor = strlen( vr_ipd->string );
	s_options_vr_walkspeed_slider.curvalue = (int) (( ClampCvar(0.5,1.5, Cvar_VariableValue("vr_walkspeed")) - 0.5) * 10.0);
}

static void VRResetDefaultsFunc ( void *unused )
{
	Cvar_SetToDefault ("vr_aimmode");
	Cvar_SetToDefault ("vr_viewmove");
	Cvar_SetToDefault ("vr_aimmode_deadzone_pitch");
	Cvar_SetToDefault ("vr_aimmode_deadzone_yaw");
	Cvar_SetToDefault ("vr_autoipd");
	Cvar_SetToDefault ("vr_ipd");
	Cvar_SetToDefault ("vr_walkspeed");
	VRSetMenuItemValues();
}

static void DeadzoneFunc ( void *unused )
{
	float temp;

	temp = ClampCvar(0,360,atof(s_options_vr_aimmode_deadzone_pitch_field.buffer));
	Cvar_SetInteger("vr_aimmode_deadzone_pitch",temp);

	temp = ClampCvar(0,360,atof(s_options_vr_aimmode_deadzone_yaw_field.buffer));
	Cvar_SetInteger("vr_aimmode_deadzone_yaw",temp);

	strcpy( s_options_vr_aimmode_deadzone_pitch_field.buffer, vr_aimmode_deadzone_pitch->string );
	s_options_vr_aimmode_deadzone_pitch_field.cursor = strlen( vr_aimmode_deadzone_pitch->string );
	strcpy( s_options_vr_aimmode_deadzone_yaw_field.buffer, vr_aimmode_deadzone_yaw->string );
	s_options_vr_aimmode_deadzone_yaw_field.cursor = strlen( vr_aimmode_deadzone_yaw->string );
}

static void WalkFunc (void *unused )
{
	float temp = s_options_vr_walkspeed_slider.curvalue / 10.0;
	temp += 0.5;
	Cvar_SetValue("vr_walkspeed",temp);
}

static void IPDFunc( void *unused )
{
	float temp;
	char string[6];

	temp = ClampCvar(0,100,atof(s_options_vr_ipd_field.buffer));
	strncpy(string, va("%.2f",temp), sizeof(string));
	Cvar_Set("vr_ipd", string);
	strcpy( s_options_vr_ipd_field.buffer, vr_ipd->string );
	s_options_vr_ipd_field.cursor = strlen( vr_ipd->string );
}

void VRConfigAccept (void)
{
	DeadzoneFunc(NULL);
	IPDFunc(NULL);
}

static void AdvancedFunc ( void *unused )
{
	VRConfigAccept();
	M_Menu_Options_VR_Advanced_f();
}

static void OVRFunc ( void *unused )
{
	VRConfigAccept();
	M_Menu_Options_VR_OVR_f();
}

static void SVRFunc ( void *unused )
{
	VRConfigAccept();
	M_Menu_Options_VR_SVR_f();
}


void BackFunc ( void *unused )
{
	VRConfigAccept();
	UI_BackMenu(NULL);
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
		"head aiming",
		"* head aiming + mouse pitch",
		"mouse aiming",
		"* mouse aiming + mouse pitch",
		"strict deadzone",
		"deadzone w/ view move",
		"deadzone w/ aim move",
		"decoupled view/aiming",
		0
	};

	static const char *auto_names[] =
	{
		"custom",
		"auto",
		0
	};


	int32_t y = 3*MENU_LINE_SIZE;

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
	s_options_vr_aimmode_box.generic.statusbar	= "WARNING: items marked with a * do not work well in multiplayer";

	s_options_vr_viewmove_box.generic.type		= MTYPE_SPINCONTROL;
	s_options_vr_viewmove_box.generic.x			= MENU_FONT_SIZE;
	s_options_vr_viewmove_box.generic.y			= y+=MENU_LINE_SIZE;
	s_options_vr_viewmove_box.generic.name			= "force view move";
	s_options_vr_viewmove_box.generic.callback		= ViewmodeFunc;
	s_options_vr_viewmove_box.itemnames			= yesno_names;
	s_options_vr_viewmove_box.generic.statusbar	= "force movement in direction of view";


	s_options_vr_aimmode_deadzone_pitch_field.generic.type = MTYPE_FIELD;
	s_options_vr_aimmode_deadzone_pitch_field.generic.flags = QMF_LEFT_JUSTIFY;
	s_options_vr_aimmode_deadzone_pitch_field.generic.name = "vertical deadzone";
	s_options_vr_aimmode_deadzone_pitch_field.generic.statusbar	= "sets the vertical aiming deadzone in degrees";
	s_options_vr_aimmode_deadzone_pitch_field.generic.callback = DeadzoneFunc;
	s_options_vr_aimmode_deadzone_pitch_field.generic.x		= MENU_FONT_SIZE;
	s_options_vr_aimmode_deadzone_pitch_field.generic.y		= y+=2*MENU_LINE_SIZE;
	s_options_vr_aimmode_deadzone_pitch_field.length	= 5;
	s_options_vr_aimmode_deadzone_pitch_field.visible_length = 5;
	strcpy( s_options_vr_aimmode_deadzone_pitch_field.buffer, vr_aimmode_deadzone_pitch->string );
	s_options_vr_aimmode_deadzone_pitch_field.cursor = strlen( vr_aimmode_deadzone_pitch->string );

	s_options_vr_aimmode_deadzone_yaw_field.generic.type = MTYPE_FIELD;
	s_options_vr_aimmode_deadzone_yaw_field.generic.flags = QMF_LEFT_JUSTIFY;
	s_options_vr_aimmode_deadzone_yaw_field.generic.name = "horizontal deadzone";
	s_options_vr_aimmode_deadzone_yaw_field.generic.statusbar	= "sets the horizontal aiming deadzone in degrees";
	s_options_vr_aimmode_deadzone_yaw_field.generic.callback = DeadzoneFunc;
	s_options_vr_aimmode_deadzone_yaw_field.generic.x		= MENU_FONT_SIZE;
	s_options_vr_aimmode_deadzone_yaw_field.generic.y		= y+=2*MENU_LINE_SIZE;
	s_options_vr_aimmode_deadzone_yaw_field.length	= 5;
	s_options_vr_aimmode_deadzone_yaw_field.visible_length = 5;
	strcpy( s_options_vr_aimmode_deadzone_yaw_field.buffer, vr_aimmode_deadzone_yaw->string );
	s_options_vr_aimmode_deadzone_yaw_field.cursor = strlen( vr_aimmode_deadzone_yaw->string );

	s_options_vr_walkspeed_slider.generic.type		= MTYPE_SLIDER;
	s_options_vr_walkspeed_slider.generic.x			= MENU_FONT_SIZE;
	s_options_vr_walkspeed_slider.generic.y			= y+=2*MENU_LINE_SIZE;
	s_options_vr_walkspeed_slider.generic.name		= "walking speed";
	s_options_vr_walkspeed_slider.generic.callback	= WalkFunc;
	s_options_vr_walkspeed_slider.minvalue			= 0;
	s_options_vr_walkspeed_slider.maxvalue			= 10;
	s_options_vr_walkspeed_slider.generic.statusbar	= "sets the walking speed";

	s_options_vr_autoipd_box.generic.type			= MTYPE_SPINCONTROL;
	s_options_vr_autoipd_box.generic.x			= MENU_FONT_SIZE;
	s_options_vr_autoipd_box.generic.y			= y+= 2*MENU_LINE_SIZE;
	s_options_vr_autoipd_box.generic.name			= "interpupillary distance";
	s_options_vr_autoipd_box.generic.callback		= AutoIPDFunc;
	s_options_vr_autoipd_box.itemnames			= auto_names;
	s_options_vr_autoipd_box.generic.statusbar	= "choose whether to use auto or custom interpupillary distance";

	s_options_vr_ipd_field.generic.type = MTYPE_FIELD;
	s_options_vr_ipd_field.generic.flags = QMF_LEFT_JUSTIFY;
	s_options_vr_ipd_field.generic.name = "custom interpupillary distance";
	s_options_vr_ipd_field.generic.statusbar = "sets a custom interpupillary distance in millimeters";
	s_options_vr_ipd_field.generic.callback = IPDFunc;
	s_options_vr_ipd_field.generic.x		= MENU_FONT_SIZE;
	s_options_vr_ipd_field.generic.y		= y+=2*MENU_LINE_SIZE;
	s_options_vr_ipd_field.length	= 5;
	s_options_vr_ipd_field.visible_length = 5;
	strcpy( s_options_vr_ipd_field.buffer, vr_ipd->string );
	s_options_vr_ipd_field.cursor = strlen( vr_ipd->string );


	s_options_vr_advanced_action.generic.type		= MTYPE_ACTION;
	s_options_vr_advanced_action.generic.x			= MENU_FONT_SIZE;
	s_options_vr_advanced_action.generic.y			= y+=2*MENU_LINE_SIZE;
	s_options_vr_advanced_action.generic.name		= "advanced options";
	s_options_vr_advanced_action.generic.callback	= AdvancedFunc;
	s_options_vr_advanced_action.generic.statusbar	= "advanced configuration options";

	s_options_vr_ovr_action.generic.type		= MTYPE_ACTION;
	s_options_vr_ovr_action.generic.x			= MENU_FONT_SIZE;
	s_options_vr_ovr_action.generic.y			= y+=MENU_LINE_SIZE;
	s_options_vr_ovr_action.generic.name		= "oculus rift options";
	s_options_vr_ovr_action.generic.callback	= OVRFunc;
	s_options_vr_ovr_action.generic.statusbar	= "oculus rift configuration";

	s_options_vr_svr_action.generic.type		= MTYPE_ACTION;
	s_options_vr_svr_action.generic.x			= MENU_FONT_SIZE;
	s_options_vr_svr_action.generic.y			= y+=MENU_LINE_SIZE;
	s_options_vr_svr_action.generic.name		= "steam vr options";
	s_options_vr_svr_action.generic.callback	= SVRFunc;
	s_options_vr_svr_action.generic.statusbar	= "steam vr configuration";


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
	s_options_vr_defaults_action.generic.statusbar	= "resets virtual reality settings to internal defaults";

	s_options_vr_back_action.generic.type		= MTYPE_ACTION;
	s_options_vr_back_action.generic.x			= MENU_FONT_SIZE;
	s_options_vr_back_action.generic.y			= y+=2*MENU_LINE_SIZE;
	s_options_vr_back_action.generic.name		= "back to options";
	s_options_vr_back_action.generic.callback	= BackFunc;


	Menu_AddItem( &s_options_vr_menu, ( void * ) &s_options_vr_header );
	Menu_AddItem( &s_options_vr_menu, ( void * ) &s_options_vr_aimmode_box );
	Menu_AddItem( &s_options_vr_menu, ( void * ) &s_options_vr_viewmove_box );
	Menu_AddItem( &s_options_vr_menu, ( void * ) &s_options_vr_aimmode_deadzone_pitch_field );
	Menu_AddItem( &s_options_vr_menu, ( void * ) &s_options_vr_aimmode_deadzone_yaw_field );
	Menu_AddItem( &s_options_vr_menu, ( void * ) &s_options_vr_walkspeed_slider );
	Menu_AddItem( &s_options_vr_menu, ( void * ) &s_options_vr_autoipd_box );
	Menu_AddItem( &s_options_vr_menu, ( void * ) &s_options_vr_ipd_field );
	Menu_AddItem( &s_options_vr_menu, ( void * ) &s_options_vr_advanced_action );
	Menu_AddItem( &s_options_vr_menu, ( void * ) &s_options_vr_ovr_action );
	Menu_AddItem( &s_options_vr_menu, ( void * ) &s_options_vr_svr_action );
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

const char *Options_VR_MenuKey( int32_t key )
{
	if ( key == K_ESCAPE		|| key == K_GAMEPAD_B
		|| key == K_GAMEPAD_BACK	|| key == K_GAMEPAD_START
		)
		VRConfigAccept();

	return Default_MenuKey( &s_options_vr_menu, key );
}


void M_Menu_Options_VR_f (void)
{
	Options_VR_MenuInit();
	UI_PushMenu ( Options_VR_MenuDraw, Options_VR_MenuKey );
}

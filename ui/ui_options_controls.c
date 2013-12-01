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

// ui_options_controls.c -- the controls options menu

#include <ctype.h>
#ifdef _WIN32
#include <io.h>
#endif
#include "../client/client.h"
#include "ui_local.h"

/*
=======================================================================

CONTROLS MENU

=======================================================================
*/

static menuframework_s	s_options_controls_menu;
static menuseparator_s	s_options_controls_header;
static menuslider_s		s_options_controls_sensitivity_slider;
static menulist_s		s_options_controls_alwaysrun_box;
static menulist_s		s_options_controls_invertmouse_box;
static menulist_s		s_options_controls_xbox_stickmode_box;
static menulist_s		s_options_controls_xbox_sticktoggle_box;
static menuslider_s		s_options_controls_xbox_trigger_threshold_slider;
static menuslider_s		s_options_controls_xbox_pitch_sensitivity_slider;
static menuslider_s		s_options_controls_xbox_yaw_sensitivity_slider;


static menuaction_s		s_options_controls_customize_keys_action;
static menuaction_s		s_options_controls_defaults_action;
static menuaction_s		s_options_controls_back_action;


static void MouseSpeedFunc( void *unused )
{
	Cvar_SetValue( "sensitivity", s_options_controls_sensitivity_slider.curvalue / 2.0F );
}

static void AlwaysRunFunc( void *unused )
{
	Cvar_SetValue( "cl_run", s_options_controls_alwaysrun_box.curvalue );
}

static void InvertMouseFunc( void *unused )
{
	Cvar_SetValue( "m_pitch", -m_pitch->value );
}

static void CustomizeControlsFunc( void *unused )
{
	M_Menu_Keys_f();
}

static void XboxStickFunc( void *unused )
{
	Cvar_SetValue( "xbox_stick_mode", s_options_controls_xbox_stickmode_box.curvalue );
}

static void XboxStickToggleFunc( void *unused )
{
	Cvar_SetValue( "xbox_stick_toggle", s_options_controls_xbox_sticktoggle_box.curvalue );
}
static void XboxTriggerFunc ( void *unused )
{
	Cvar_SetValue( "xbox_trigger_threshold", s_options_controls_xbox_trigger_threshold_slider.curvalue / 25.0f);
}

static void XboxViewFunc ( void *unused )
{
	Cvar_SetValue( "xbox_pitch_sensitivity", s_options_controls_xbox_pitch_sensitivity_slider.curvalue / 4.0f);
	Cvar_SetValue( "xbox_yaw_sensitivity", s_options_controls_xbox_yaw_sensitivity_slider.curvalue / 4.0f);
}


static void ControlsSetMenuItemValues( void )
{
	s_options_controls_sensitivity_slider.curvalue	= ( Cvar_VariableValue("sensitivity") ) * 2;
	s_options_controls_invertmouse_box.curvalue		= Cvar_VariableValue("m_pitch") < 0;

	Cvar_SetValue( "cl_run", ClampCvar( 0, 1, Cvar_VariableValue("cl_run") ) );
	s_options_controls_alwaysrun_box.curvalue		= Cvar_VariableValue("cl_run");

	Cvar_SetValue( "xbox_stick_mode", ClampCvar( 0, 1, Cvar_VariableValue("xbox_stick_mode") ) );
	s_options_controls_xbox_stickmode_box.curvalue		= Cvar_VariableValue("xbox_stick_mode");

	Cvar_SetValue( "xbox_stick_toggle", ClampCvar( 0, 1, Cvar_VariableValue("xbox_stick_toggle") ) );
	s_options_controls_xbox_sticktoggle_box.curvalue		= Cvar_VariableValue("xbox_stick_toggle");

	Cvar_SetValue( "xbox_trigger_threshold", ClampCvar( 0.04, 0.96, Cvar_VariableValue("xbox_trigger_threshold") ) );
	s_options_controls_xbox_trigger_threshold_slider.curvalue = Cvar_VariableValue("xbox_trigger_threshold") * 25.0f;
	
	Cvar_SetValue( "xbox_pitch_sensitivity", ClampCvar( 0.25, 4.0, Cvar_VariableValue("xbox_pitch_sensitivity") ) );
	s_options_controls_xbox_pitch_sensitivity_slider.curvalue = Cvar_VariableValue("xbox_pitch_sensitivity") * 4.0f;
	
	Cvar_SetValue( "xbox_yaw_sensitivity", ClampCvar( 0.25, 4.0, Cvar_VariableValue("xbox_yaw_sensitivity") ) );
	s_options_controls_xbox_yaw_sensitivity_slider.curvalue = Cvar_VariableValue("xbox_yaw_sensitivity") * 4.0f;
}

static void ControlsResetDefaultsFunc ( void *unused )
{


	Cbuf_AddText ("exec defaultbinds.cfg\n"); // reset default binds
	Cbuf_Execute();

	Cvar_SetToDefault ("sensitivity");
	Cvar_SetToDefault ("m_pitch");
	Cvar_SetToDefault ("m_yaw");
	Cvar_SetToDefault ("m_forward");
	Cvar_SetToDefault ("m_side");

	Cvar_SetToDefault ("cl_run");
	Cvar_SetToDefault ("lookspring");
	Cvar_SetToDefault ("lookstrafe");
	Cvar_SetToDefault ("freelook");
	Cvar_SetToDefault ("in_controller");

	Cvar_SetToDefault ("xbox_usernum");
	Cvar_SetToDefault ("xbox_stick_mode");
	Cvar_SetToDefault ("xbox_stick_toggle");
	Cvar_SetToDefault ("xbox_trigger_threshold");
	Cvar_SetToDefault ("xbox_pitch_sensitivity");
	Cvar_SetToDefault ("xbox_yaw_sensitivity");

	ControlsSetMenuItemValues();
}

void Options_Controls_MenuInit ( void )
{
	static const char *yesno_names[] =
	{
		"no",
		"yes",
		0
	};

	static const char *xbox_stick_names[] =
	{
		"move / view",
		"view / move",
		0
	};

	int y = 3*MENU_LINE_SIZE;

	s_options_controls_menu.x = SCREEN_WIDTH*0.5;
	s_options_controls_menu.y = SCREEN_HEIGHT*0.5 - 58;
	s_options_controls_menu.nitems = 0;

	s_options_controls_header.generic.type	= MTYPE_SEPARATOR;
	s_options_controls_header.generic.name	= "Controls";
	s_options_controls_header.generic.x		= MENU_FONT_SIZE/2 * strlen(s_options_controls_header.generic.name);
	s_options_controls_header.generic.y		= 0;

	s_options_controls_alwaysrun_box.generic.type		= MTYPE_SPINCONTROL;
	s_options_controls_alwaysrun_box.generic.x			= 0;
	s_options_controls_alwaysrun_box.generic.y			= y;
	s_options_controls_alwaysrun_box.generic.name		= "always run";
	s_options_controls_alwaysrun_box.generic.callback	= AlwaysRunFunc;
	s_options_controls_alwaysrun_box.itemnames			= yesno_names;
	s_options_controls_alwaysrun_box.generic.statusbar	= "enables running as default movement";

	s_options_controls_invertmouse_box.generic.type			= MTYPE_SPINCONTROL;
	s_options_controls_invertmouse_box.generic.x			= 0;
	s_options_controls_invertmouse_box.generic.y			= y+=MENU_LINE_SIZE;
	s_options_controls_invertmouse_box.generic.name			= "invert y-axis";
	s_options_controls_invertmouse_box.generic.callback		= InvertMouseFunc;
	s_options_controls_invertmouse_box.itemnames			= yesno_names;
	s_options_controls_invertmouse_box.generic.statusbar	= "inverts controller y-axis movement";


	s_options_controls_sensitivity_slider.generic.type		= MTYPE_SLIDER;
	s_options_controls_sensitivity_slider.generic.x			= 0;
	s_options_controls_sensitivity_slider.generic.y			= y+=2*MENU_LINE_SIZE;
	s_options_controls_sensitivity_slider.generic.name		= "mouse speed";
	s_options_controls_sensitivity_slider.generic.callback	= MouseSpeedFunc;
	s_options_controls_sensitivity_slider.minvalue			= 2;
	s_options_controls_sensitivity_slider.maxvalue			= 22;
	s_options_controls_sensitivity_slider.generic.statusbar	= "changes sensitivity of mouse for head movement";
	

	s_options_controls_xbox_stickmode_box.generic.type		= MTYPE_SPINCONTROL;
	s_options_controls_xbox_stickmode_box.generic.x			= 0;
	s_options_controls_xbox_stickmode_box.generic.y			= y+=2*MENU_LINE_SIZE;
	s_options_controls_xbox_stickmode_box.generic.name		= "xbox thumb stick mode";
	s_options_controls_xbox_stickmode_box.generic.callback	= XboxStickFunc;
	s_options_controls_xbox_stickmode_box.itemnames			= xbox_stick_names;
	s_options_controls_xbox_stickmode_box.generic.statusbar	= "sets mode for the xbox 360 controller thumb sticks";

	
	s_options_controls_xbox_sticktoggle_box.generic.type		= MTYPE_SPINCONTROL;
	s_options_controls_xbox_sticktoggle_box.generic.x			= 0;
	s_options_controls_xbox_sticktoggle_box.generic.y			= y+=MENU_LINE_SIZE;
	s_options_controls_xbox_sticktoggle_box.generic.name		= "toggle xbox thumb sticks";
	s_options_controls_xbox_sticktoggle_box.generic.callback	= XboxStickToggleFunc;
	s_options_controls_xbox_sticktoggle_box.itemnames			= yesno_names;
	s_options_controls_xbox_sticktoggle_box.generic.statusbar	= "enables thumb stick toggling instead of click-and-hold";

	s_options_controls_xbox_trigger_threshold_slider.generic.type		= MTYPE_SLIDER;
	s_options_controls_xbox_trigger_threshold_slider.generic.x			= 0;
	s_options_controls_xbox_trigger_threshold_slider.generic.y			= y+=MENU_LINE_SIZE;
	s_options_controls_xbox_trigger_threshold_slider.generic.name		= "xbox trigger sensitivity";
	s_options_controls_xbox_trigger_threshold_slider.generic.callback	= XboxTriggerFunc;
	s_options_controls_xbox_trigger_threshold_slider.minvalue			= 1;
	s_options_controls_xbox_trigger_threshold_slider.maxvalue			= 24;
	s_options_controls_xbox_trigger_threshold_slider.generic.statusbar	= "changes xbox 360 controller trigger sensitivity";	
	
	s_options_controls_xbox_pitch_sensitivity_slider.generic.type		= MTYPE_SLIDER;
	s_options_controls_xbox_pitch_sensitivity_slider.generic.x			= 0;
	s_options_controls_xbox_pitch_sensitivity_slider.generic.y			= y+=MENU_LINE_SIZE;
	s_options_controls_xbox_pitch_sensitivity_slider.generic.name		= "xbox pitch speed";
	s_options_controls_xbox_pitch_sensitivity_slider.generic.callback	= XboxViewFunc;
	s_options_controls_xbox_pitch_sensitivity_slider.minvalue			= 1;
	s_options_controls_xbox_pitch_sensitivity_slider.maxvalue			= 16;
	s_options_controls_xbox_pitch_sensitivity_slider.generic.statusbar	= "changes xbox 360 controller view pitch speed";
	
	s_options_controls_xbox_yaw_sensitivity_slider.generic.type		= MTYPE_SLIDER;
	s_options_controls_xbox_yaw_sensitivity_slider.generic.x			= 0;
	s_options_controls_xbox_yaw_sensitivity_slider.generic.y			= y+=MENU_LINE_SIZE;
	s_options_controls_xbox_yaw_sensitivity_slider.generic.name		= "xbox yaw speed";
	s_options_controls_xbox_yaw_sensitivity_slider.generic.callback	= XboxViewFunc;
	s_options_controls_xbox_yaw_sensitivity_slider.minvalue			= 1;
	s_options_controls_xbox_yaw_sensitivity_slider.maxvalue			= 16;
	s_options_controls_xbox_yaw_sensitivity_slider.generic.statusbar	= "changes xbox 360 controller view yaw speed";

	s_options_controls_customize_keys_action.generic.type		= MTYPE_ACTION;
	s_options_controls_customize_keys_action.generic.x			= MENU_FONT_SIZE;
	s_options_controls_customize_keys_action.generic.y			= y+=2*MENU_LINE_SIZE;
	s_options_controls_customize_keys_action.generic.name		= "customize controls";
	s_options_controls_customize_keys_action.generic.callback	= CustomizeControlsFunc;

	s_options_controls_defaults_action.generic.type			= MTYPE_ACTION;
	s_options_controls_defaults_action.generic.x			= MENU_FONT_SIZE;
	s_options_controls_defaults_action.generic.y			= y+=2*MENU_LINE_SIZE;
	s_options_controls_defaults_action.generic.name			= "reset defaults";
	s_options_controls_defaults_action.generic.callback		= ControlsResetDefaultsFunc;
	s_options_controls_defaults_action.generic.statusbar	= "resets all control settings to internal defaults";

	s_options_controls_back_action.generic.type		= MTYPE_ACTION;
	s_options_controls_back_action.generic.x		= MENU_FONT_SIZE;
	s_options_controls_back_action.generic.y		= y+=2*MENU_LINE_SIZE;
	s_options_controls_back_action.generic.name		= "back to options";
	s_options_controls_back_action.generic.callback	= UI_BackMenu;

	Menu_AddItem( &s_options_controls_menu, ( void * ) &s_options_controls_header );
	Menu_AddItem( &s_options_controls_menu, ( void * ) &s_options_controls_alwaysrun_box );
	Menu_AddItem( &s_options_controls_menu, ( void * ) &s_options_controls_invertmouse_box );
	Menu_AddItem( &s_options_controls_menu, ( void * ) &s_options_controls_sensitivity_slider );
	Menu_AddItem( &s_options_controls_menu, ( void * ) &s_options_controls_xbox_stickmode_box );
	
	Menu_AddItem( &s_options_controls_menu, ( void * ) &s_options_controls_xbox_sticktoggle_box );
	Menu_AddItem( &s_options_controls_menu, ( void * ) &s_options_controls_xbox_trigger_threshold_slider );
	Menu_AddItem( &s_options_controls_menu, ( void * ) &s_options_controls_xbox_pitch_sensitivity_slider );
	Menu_AddItem( &s_options_controls_menu, ( void * ) &s_options_controls_xbox_yaw_sensitivity_slider );

	Menu_AddItem( &s_options_controls_menu, ( void * ) &s_options_controls_customize_keys_action );
	Menu_AddItem( &s_options_controls_menu, ( void * ) &s_options_controls_defaults_action );
	Menu_AddItem( &s_options_controls_menu, ( void * ) &s_options_controls_back_action );

	ControlsSetMenuItemValues();
}

void Options_Controls_MenuDraw (void)
{
	Menu_DrawBanner( "m_banner_options" );

	Menu_AdjustCursor( &s_options_controls_menu, 1 );
	Menu_Draw( &s_options_controls_menu );
}

const char *Options_Controls_MenuKey( int key )
{
	return Default_MenuKey( &s_options_controls_menu, key );
}

void M_Menu_Options_Controls_f (void)
{
	Options_Controls_MenuInit();
	UI_PushMenu ( Options_Controls_MenuDraw, Options_Controls_MenuKey );
}

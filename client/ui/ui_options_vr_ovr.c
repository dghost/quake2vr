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
#include "../../client/vr/include/vr_ovr.h"

/*
=======================================================================

INTERFACE MENU

=======================================================================
*/

static menuframework_s	s_options_vr_ovr_menu;
static menuseparator_s	s_options_vr_ovr_header;
static menulist_s		s_options_vr_ovr_enable_box;
static menulist_s		s_options_vr_ovr_device_box;
static menulist_s		s_options_vr_ovr_maxfov_box;
static menulist_s		s_options_vr_ovr_prediction_box;
static menulist_s		s_options_vr_ovr_lowpersistence_box;
static menulist_s		s_options_vr_ovr_lumaoverdrive_box;
static menulist_s		s_options_vr_ovr_dk2_color_hack_box;
static menulist_s		s_options_vr_ovr_timewarp_box;
static menulist_s		s_options_vr_ovr_debug_box;

static menuaction_s		s_options_vr_ovr_defaults_action;
static menuaction_s		s_options_vr_ovr_back_action;



char *def = "default";

static char *hmds[255];
ovrname_t *VR_OVR_GetNameList();

static void InitNames()
{
	int i =0;
	ovrname_t *names = VR_OVR_GetNameList();

	memset(hmds,0,sizeof(char *) * 255);
	hmds[0] = def;
	while (names[i].label[0] != 0 && i < 254)
	{
		hmds[i+1] = names[i].label;
		i++;
	}

}

static int FindSN()
{
	int i = 0;
	ovrname_t *names = VR_OVR_GetNameList();

	while (names[i].label[0] != 0 && i < 254)
	{
		if (!strncmp(vr_ovr_device->string,names[i].serialnumber,strlen(names[i].serialnumber)))
			return i + 1;
		i++;
	}
	return 0;
}

static void ScaleFunc( void *unused )
{
	Cvar_SetInteger( "vr_ovr_maxfov", s_options_vr_ovr_maxfov_box.curvalue);
}

static void DebugFunc( void *unused)
{
	Cvar_SetInteger("vr_ovr_debug",s_options_vr_ovr_debug_box.curvalue);
}

static void EnableFunc( void *unused)
{
	Cvar_SetInteger("vr_ovr_enable", s_options_vr_ovr_enable_box.curvalue);
}

static void PredictionFunc( void *unused)
{
	Cvar_SetInteger("vr_ovr_autoprediction", s_options_vr_ovr_prediction_box.curvalue);
}

static void WarpFunc( void *unused)
{
	Cvar_SetInteger("vr_ovr_timewarp", s_options_vr_ovr_timewarp_box.curvalue);
}

static void PersistenceFunc( void *unused)
{
	Cvar_SetInteger("vr_ovr_lowpersistence", s_options_vr_ovr_lowpersistence_box.curvalue);
}

static void LumaFunc( void *unused)
{
	Cvar_SetInteger("vr_ovr_lumoverdrive", s_options_vr_ovr_lumaoverdrive_box.curvalue);
}

static void ColorHackFunc( void *unused)
{
	Cvar_SetInteger("vr_ovr_dk2_color_hack", s_options_vr_ovr_dk2_color_hack_box.curvalue);
}

static void DeviceFunc( void *unused )
{
	ovrname_t *names = VR_OVR_GetNameList();
	int value = s_options_vr_ovr_device_box.curvalue;
	if ( value == 0)
		Cvar_Set("vr_ovr_device","");
	else
		Cvar_Set("vr_ovr_device",names[value -1].serialnumber);
}

static void VROVRSetMenuItemValues( void )
{
	s_options_vr_ovr_enable_box.curvalue = ( !! Cvar_VariableInteger("vr_ovr_enable") );
	s_options_vr_ovr_maxfov_box.curvalue = ( Cvar_VariableInteger("vr_ovr_maxfov") );
	s_options_vr_ovr_debug_box.curvalue = ( Cvar_VariableInteger("vr_ovr_debug") );
	s_options_vr_ovr_prediction_box.curvalue = ( Cvar_VariableInteger("vr_ovr_autoprediction") );
	s_options_vr_ovr_timewarp_box.curvalue = ( Cvar_VariableInteger("vr_ovr_timewarp") );
	s_options_vr_ovr_lowpersistence_box.curvalue = ( Cvar_VariableInteger("vr_ovr_lowpersistence") );
	s_options_vr_ovr_lumaoverdrive_box.curvalue = ( Cvar_VariableInteger("vr_ovr_lumoverdrive") );
	s_options_vr_ovr_dk2_color_hack_box.curvalue = ( Cvar_VariableInteger("vr_ovr_dk2_color_hack") );
}

static void VROVRResetDefaultsFunc ( void *unused )
{
	Cvar_SetToDefault ("vr_ovr_enable");
	Cvar_SetToDefault ("vr_ovr_maxfov");
	Cvar_SetToDefault ("vr_ovr_debug");
	Cvar_SetToDefault ("vr_ovr_autoprediction");
	Cvar_SetToDefault ("vr_ovr_timewarp");
	Cvar_SetToDefault ("vr_ovr_lowpersistence");
	Cvar_SetToDefault ("vr_ovr_lumoverdrive");
	Cvar_SetToDefault ("vr_ovr_dk2_color_hack");
	Cvar_SetToDefault ("vr_ovr_device");
	VROVRSetMenuItemValues();
}

static void VROVRConfigAccept (void)
{
}

static void BackFunc ( void *unused )
{
	VROVRConfigAccept();
	UI_BackMenu(NULL);
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
		"default",
		"screen maximum",
		0
	};

	static const char *auto_names[] =
	{
		"custom",
		"auto",
		0
	};

	static const char *debug_names[] =
	{
		"disabled",
		"DK1 emulation",
		"HD DK emulation",
		"DK2 Emulation",
		0
	};

	static const char *enable_names[] =
	{
		"disable",
		"enable",
		0
	};


	int32_t y = 3*MENU_LINE_SIZE;



	s_options_vr_ovr_menu.x = SCREEN_WIDTH*0.5;
	s_options_vr_ovr_menu.y = SCREEN_HEIGHT*0.5 - 80;
	s_options_vr_ovr_menu.nitems = 0;

	s_options_vr_ovr_header.generic.type		= MTYPE_SEPARATOR;
	s_options_vr_ovr_header.generic.name		= "oculus rift configuration";
	s_options_vr_ovr_header.generic.x		= MENU_FONT_SIZE/2 * strlen(s_options_vr_ovr_header.generic.name);
	s_options_vr_ovr_header.generic.y		= 0;

	
	s_options_vr_ovr_enable_box.generic.type		= MTYPE_SPINCONTROL;
	s_options_vr_ovr_enable_box.generic.x			= MENU_FONT_SIZE;
	s_options_vr_ovr_enable_box.generic.y			= y;
	s_options_vr_ovr_enable_box.generic.name		= "oculus rift support";
	s_options_vr_ovr_enable_box.generic.callback	= EnableFunc;
	s_options_vr_ovr_enable_box.itemnames			= enable_names;
	s_options_vr_ovr_enable_box.generic.statusbar	= "enable or disable native oculus rift support";

	if (Cvar_VariableInteger("vr_enabled") == HMD_RIFT)
	{
		InitNames();

		s_options_vr_ovr_device_box.generic.type		= MTYPE_SPINCONTROL;
		s_options_vr_ovr_device_box.generic.x			= MENU_FONT_SIZE;
		s_options_vr_ovr_device_box.generic.y			= y+=2*MENU_LINE_SIZE;
		s_options_vr_ovr_device_box.generic.name		= "device";
		s_options_vr_ovr_device_box.generic.callback	= DeviceFunc;
		s_options_vr_ovr_device_box.itemnames			= hmds;
		s_options_vr_ovr_device_box.generic.statusbar	= "selects device to use";
	
		s_options_vr_ovr_device_box.curvalue = FindSN();

	}
	s_options_vr_ovr_timewarp_box.generic.type		= MTYPE_SPINCONTROL;
	s_options_vr_ovr_timewarp_box.generic.x			= MENU_FONT_SIZE;
	s_options_vr_ovr_timewarp_box.generic.y			= y+=2*MENU_LINE_SIZE;
	s_options_vr_ovr_timewarp_box.generic.name		= "time warp";
	s_options_vr_ovr_timewarp_box.generic.callback	= WarpFunc;
	s_options_vr_ovr_timewarp_box.itemnames			= enable_names;
	s_options_vr_ovr_timewarp_box.generic.statusbar	= "applies time warping during distortion";

	s_options_vr_ovr_prediction_box.generic.type		= MTYPE_SPINCONTROL;
	s_options_vr_ovr_prediction_box.generic.x			= MENU_FONT_SIZE;
	s_options_vr_ovr_prediction_box.generic.y			= y+=MENU_LINE_SIZE;
	s_options_vr_ovr_prediction_box.generic.name		= "auto motion prediction";
	s_options_vr_ovr_prediction_box.generic.callback	= PredictionFunc;
	s_options_vr_ovr_prediction_box.itemnames			= enable_names;
	s_options_vr_ovr_prediction_box.generic.statusbar	= "sets motion prediction time automatically";

	s_options_vr_ovr_maxfov_box.generic.type		= MTYPE_SPINCONTROL;
	s_options_vr_ovr_maxfov_box.generic.x			= MENU_FONT_SIZE;
	s_options_vr_ovr_maxfov_box.generic.y			= y+=MENU_LINE_SIZE;
	s_options_vr_ovr_maxfov_box.generic.name		= "distortion fov";
	s_options_vr_ovr_maxfov_box.generic.callback	= ScaleFunc;
	s_options_vr_ovr_maxfov_box.itemnames			= scale_names;
	s_options_vr_ovr_maxfov_box.generic.statusbar	= "adjusts the size of the post-distortion view";

	s_options_vr_ovr_lowpersistence_box.generic.type		= MTYPE_SPINCONTROL;
	s_options_vr_ovr_lowpersistence_box.generic.x			= MENU_FONT_SIZE;
	s_options_vr_ovr_lowpersistence_box.generic.y			= y+=MENU_LINE_SIZE;
	s_options_vr_ovr_lowpersistence_box.generic.name		= "low persistence";
	s_options_vr_ovr_lowpersistence_box.generic.callback	= PersistenceFunc;
	s_options_vr_ovr_lowpersistence_box.itemnames			= yesno_names;
	s_options_vr_ovr_lowpersistence_box.generic.statusbar	= "enables low persistence on DK2 and above";

	s_options_vr_ovr_lumaoverdrive_box.generic.type		= MTYPE_SPINCONTROL;
	s_options_vr_ovr_lumaoverdrive_box.generic.x			= MENU_FONT_SIZE;
	s_options_vr_ovr_lumaoverdrive_box.generic.y			= y+=MENU_LINE_SIZE;
	s_options_vr_ovr_lumaoverdrive_box.generic.name		= "luminance overdrive";
	s_options_vr_ovr_lumaoverdrive_box.generic.callback	= LumaFunc;
	s_options_vr_ovr_lumaoverdrive_box.itemnames			= yesno_names;
	s_options_vr_ovr_lumaoverdrive_box.generic.statusbar	= "enables lumanance overdrive on DK2 and above";

	s_options_vr_ovr_dk2_color_hack_box.generic.type		= MTYPE_SPINCONTROL;
	s_options_vr_ovr_dk2_color_hack_box.generic.x			= MENU_FONT_SIZE;
	s_options_vr_ovr_dk2_color_hack_box.generic.y			= y+=MENU_LINE_SIZE;
	s_options_vr_ovr_dk2_color_hack_box.generic.name		= "DK2 black boost";
	s_options_vr_ovr_dk2_color_hack_box.generic.callback	= ColorHackFunc;
	s_options_vr_ovr_dk2_color_hack_box.itemnames			= yesno_names;
	s_options_vr_ovr_dk2_color_hack_box.generic.statusbar	= "boosts black levels to reduce bluring on DK2 and above";



	s_options_vr_ovr_debug_box.generic.type			= MTYPE_SPINCONTROL;
	s_options_vr_ovr_debug_box.generic.x			= MENU_FONT_SIZE;
	s_options_vr_ovr_debug_box.generic.y			= y+=MENU_LINE_SIZE;
	s_options_vr_ovr_debug_box.generic.name			= "debug rendering";
	s_options_vr_ovr_debug_box.generic.callback		= DebugFunc;
	s_options_vr_ovr_debug_box.itemnames			= debug_names;
	s_options_vr_ovr_debug_box.generic.statusbar	= "enables oculus rift debug rendering support";

	s_options_vr_ovr_defaults_action.generic.type		= MTYPE_ACTION;
	s_options_vr_ovr_defaults_action.generic.x			= MENU_FONT_SIZE;
	s_options_vr_ovr_defaults_action.generic.y			= y+=2*MENU_LINE_SIZE;
	s_options_vr_ovr_defaults_action.generic.name		= "reset defaults";
	s_options_vr_ovr_defaults_action.generic.callback	= VROVRResetDefaultsFunc;
	s_options_vr_ovr_defaults_action.generic.statusbar	= "resets all oculus rift settings to internal defaults";

	s_options_vr_ovr_back_action.generic.type		= MTYPE_ACTION;
	s_options_vr_ovr_back_action.generic.x			= MENU_FONT_SIZE;
	s_options_vr_ovr_back_action.generic.y			= y+=2*MENU_LINE_SIZE;
	s_options_vr_ovr_back_action.generic.name		= "back to options";
	s_options_vr_ovr_back_action.generic.callback	= BackFunc;

	Menu_AddItem( &s_options_vr_ovr_menu, ( void * ) &s_options_vr_ovr_header );

	Menu_AddItem( &s_options_vr_ovr_menu, ( void * ) &s_options_vr_ovr_enable_box );
	
	if (Cvar_VariableInteger("vr_enabled") == HMD_RIFT)
		Menu_AddItem( &s_options_vr_ovr_menu, ( void * ) &s_options_vr_ovr_device_box );
	Menu_AddItem( &s_options_vr_ovr_menu, ( void * ) &s_options_vr_ovr_timewarp_box );
	Menu_AddItem( &s_options_vr_ovr_menu, ( void * ) &s_options_vr_ovr_prediction_box );
	Menu_AddItem( &s_options_vr_ovr_menu, ( void * ) &s_options_vr_ovr_maxfov_box );
	Menu_AddItem( &s_options_vr_ovr_menu, ( void * ) &s_options_vr_ovr_lowpersistence_box );	
	Menu_AddItem( &s_options_vr_ovr_menu, ( void * ) &s_options_vr_ovr_lumaoverdrive_box );
	Menu_AddItem( &s_options_vr_ovr_menu, ( void * ) &s_options_vr_ovr_dk2_color_hack_box );

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

const char *Options_VR_OVR_MenuKey( int32_t key )
{
	if ( key == K_ESCAPE		|| key == K_GAMEPAD_B
		|| key == K_GAMEPAD_BACK	|| key == K_GAMEPAD_START
		)
		VROVRConfigAccept();
	return Default_MenuKey( &s_options_vr_ovr_menu, key );
}

void M_Menu_Options_VR_OVR_f (void)
{
	Options_VR_OVR_MenuInit();
	UI_PushMenu ( Options_VR_OVR_MenuDraw, Options_VR_OVR_MenuKey );
}

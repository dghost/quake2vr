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
static menulist_s		s_options_vr_ovr_filtermode_box;
static menufield_s		s_options_vr_ovr_prediction_field;
static menulist_s		s_options_vr_ovr_distortion_box;
static menulist_s		s_options_vr_ovr_autoscale_box;
static menufield_s		s_options_vr_ovr_scale_field;
static menulist_s		s_options_vr_ovr_autolensdistance_box;
static menufield_s		s_options_vr_ovr_lensdistance_field;
static menulist_s		s_options_vr_ovr_latency_box;
static menulist_s		s_options_vr_ovr_debug_box;

static menuaction_s		s_options_vr_ovr_defaults_action;
static menuaction_s		s_options_vr_ovr_back_action;
extern cvar_t *vr_ovr_prediction;
extern cvar_t *vr_ovr_lensdistance;
extern cvar_t *vr_ovr_scale;

static void DriftFunc( void *unused )
{
	Cvar_SetInteger( "vr_ovr_driftcorrection", s_options_vr_ovr_drift_box.curvalue);
}

static void ChromaFunc( void *unused )
{
	Cvar_SetInteger( "vr_ovr_chromatic", s_options_vr_ovr_chroma_box.curvalue);
}

static void BicubicFunc( void *unused )
{
	Cvar_SetInteger( "vr_ovr_filtermode", s_options_vr_ovr_filtermode_box.curvalue);
}

static void DistortionFunc( void *unused )
{
	Cvar_SetInteger( "vr_ovr_distortion", s_options_vr_ovr_distortion_box.curvalue);
}

static void ScaleFunc( void *unused )
{
	Cvar_SetInteger( "vr_ovr_autoscale", s_options_vr_ovr_autoscale_box.curvalue);
}

static void AutoFunc( void *unused )
{
	Cvar_SetInteger( "vr_ovr_autolensdistance", s_options_vr_ovr_autolensdistance_box.curvalue);
}

static void LatencyFunc( void *unused)
{
	Cvar_SetInteger("vr_ovr_latencytest",s_options_vr_ovr_latency_box.curvalue);
}

static void DebugFunc( void *unused)
{
	Cvar_SetInteger("vr_ovr_debug",s_options_vr_ovr_debug_box.curvalue);
}

static void VROVRSetMenuItemValues( void )
{
	s_options_vr_ovr_drift_box.curvalue = ( Cvar_VariableInteger("vr_ovr_driftcorrection") );
	s_options_vr_ovr_chroma_box.curvalue = ( Cvar_VariableInteger("vr_ovr_chromatic") );
	s_options_vr_ovr_filtermode_box.curvalue = ( Cvar_VariableInteger("vr_ovr_filtermode") );
	s_options_vr_ovr_distortion_box.curvalue = ( Cvar_VariableInteger("vr_ovr_distortion") );
	s_options_vr_ovr_autoscale_box.curvalue = ( Cvar_VariableInteger("vr_ovr_autoscale") );
	s_options_vr_ovr_latency_box.curvalue = (Cvar_VariableInteger("vr_ovr_latencytest") );
	s_options_vr_ovr_debug_box.curvalue = ( Cvar_VariableInteger("vr_ovr_debug") );
	s_options_vr_ovr_autolensdistance_box.curvalue = ( Cvar_VariableInteger("vr_ovr_autolensdistance") );

	strcpy( s_options_vr_ovr_lensdistance_field.buffer, vr_ovr_lensdistance->string );
	s_options_vr_ovr_lensdistance_field.cursor = strlen( vr_ovr_lensdistance->string );

	strcpy( s_options_vr_ovr_prediction_field.buffer, vr_ovr_prediction->string );
	s_options_vr_ovr_prediction_field.cursor = strlen( vr_ovr_prediction->string );

	strcpy( s_options_vr_ovr_scale_field.buffer, vr_ovr_scale->string );
	s_options_vr_ovr_scale_field.cursor = strlen( vr_ovr_scale->string );
}

static void VROVRResetDefaultsFunc ( void *unused )
{
	Cvar_SetToDefault ("vr_ovr_driftcorrection");
	Cvar_SetToDefault ("vr_ovr_chromatic");
	Cvar_SetToDefault ("vr_ovr_filtermode");
	Cvar_SetToDefault ("vr_ovr_distortion");
	Cvar_SetToDefault ("vr_ovr_autoscale");
	Cvar_SetToDefault ("vr_ovr_scale");
	Cvar_SetToDefault ("vr_ovr_latencytest");
	Cvar_SetToDefault ("vr_ovr_debug");
	Cvar_SetToDefault ("vr_ovr_prediction");
	Cvar_SetToDefault ("vr_ovr_autolensdistance");
	Cvar_SetToDefault ("vr_ovr_lensdistance");
	VROVRSetMenuItemValues();
}

static void CustomScaleFunc(void *unused)
{
	float temp;
	char string[6];

	temp = ClampCvar(1,2,atof(s_options_vr_ovr_scale_field.buffer));
	strncpy(string, va("%.2f",temp), sizeof(string));
	Cvar_Set("vr_ovr_scale",string);
	strcpy( s_options_vr_ovr_scale_field.buffer, vr_ovr_scale->string );
	s_options_vr_ovr_scale_field.cursor = strlen( vr_ovr_scale->string );
}

static void CustomPredictionFunc(void *unused)
{
	float temp;

	temp = ClampCvar(0,75,atof(s_options_vr_ovr_prediction_field.buffer));
	Cvar_SetInteger("vr_ovr_prediction",temp);
	strcpy( s_options_vr_ovr_prediction_field.buffer, vr_ovr_prediction->string );
	s_options_vr_ovr_prediction_field.cursor = strlen( vr_ovr_prediction->string );
}

static void CustomLensFunc(void *unused)
{
	float temp;
	char string[6];

	temp = ClampCvar(0,100,atof(s_options_vr_ovr_lensdistance_field.buffer));
	strncpy(string, va("%.2f",temp), sizeof(string));
	Cvar_Set("vr_ovr_lensdistance",string);
	strcpy( s_options_vr_ovr_lensdistance_field.buffer, vr_ovr_lensdistance->string );
	s_options_vr_ovr_lensdistance_field.cursor = strlen( vr_ovr_lensdistance->string );
}

static void VROVRConfigAccept (void)
{
	CustomScaleFunc(NULL);
	CustomPredictionFunc(NULL);
	CustomLensFunc(NULL);
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
		"custom",
		"nearest edge",
		"furthest edge",
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
		0
	};

	static const char *filter_names[] =
	{
		"bilinear",
		"bspline bilinear",
		0
	};

	Sint32 y = 3*MENU_LINE_SIZE;

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

	s_options_vr_ovr_filtermode_box.generic.type		= MTYPE_SPINCONTROL;
	s_options_vr_ovr_filtermode_box.generic.x			= MENU_FONT_SIZE;
	s_options_vr_ovr_filtermode_box.generic.y			= y+=MENU_LINE_SIZE;
	s_options_vr_ovr_filtermode_box.generic.name		= "filter mode";
	s_options_vr_ovr_filtermode_box.generic.callback	= BicubicFunc;
	s_options_vr_ovr_filtermode_box.itemnames			= filter_names;
	s_options_vr_ovr_filtermode_box.generic.statusbar	= "chooses texture filtering mode used with the distortion shader";

	s_options_vr_ovr_prediction_field.generic.type = MTYPE_FIELD;
	s_options_vr_ovr_prediction_field.generic.flags = QMF_LEFT_JUSTIFY;
	s_options_vr_ovr_prediction_field.generic.name = "motion prediction";
	s_options_vr_ovr_prediction_field.generic.statusbar	= "sets the amount of motion prediction to apply in milliseconds";
	s_options_vr_ovr_prediction_field.generic.callback = CustomPredictionFunc;
	s_options_vr_ovr_prediction_field.generic.x		= MENU_FONT_SIZE;
	s_options_vr_ovr_prediction_field.generic.y		= y+=2*MENU_LINE_SIZE;
	s_options_vr_ovr_prediction_field.length	= 5;
	s_options_vr_ovr_prediction_field.visible_length = 5;
	strcpy( s_options_vr_ovr_prediction_field.buffer, vr_ovr_prediction->string );
	s_options_vr_ovr_prediction_field.cursor = strlen( vr_ovr_prediction->string );


	
	s_options_vr_ovr_distortion_box.generic.type			= MTYPE_SPINCONTROL;
	s_options_vr_ovr_distortion_box.generic.x			= MENU_FONT_SIZE;
	s_options_vr_ovr_distortion_box.generic.y			= y+=2*MENU_LINE_SIZE;
	s_options_vr_ovr_distortion_box.generic.name			= "distortion";
	s_options_vr_ovr_distortion_box.generic.callback		= DistortionFunc;
	s_options_vr_ovr_distortion_box.itemnames			= yesno_names;
	s_options_vr_ovr_distortion_box.generic.statusbar	= "enables the barrel distortion shader";

	s_options_vr_ovr_autoscale_box.generic.type			= MTYPE_SPINCONTROL;
	s_options_vr_ovr_autoscale_box.generic.x			= MENU_FONT_SIZE;
	s_options_vr_ovr_autoscale_box.generic.y			= y+=MENU_LINE_SIZE;
	s_options_vr_ovr_autoscale_box.generic.name			= "distortion scaling";
	s_options_vr_ovr_autoscale_box.generic.callback		= ScaleFunc;
	s_options_vr_ovr_autoscale_box.itemnames			= scale_names;
	s_options_vr_ovr_autoscale_box.generic.statusbar	= "adjusts the size of the post-distortion view";
		
	s_options_vr_ovr_scale_field.generic.type = MTYPE_FIELD;
	s_options_vr_ovr_scale_field.generic.flags = QMF_LEFT_JUSTIFY;
	s_options_vr_ovr_scale_field.generic.name = "custom distortion scale";
	s_options_vr_ovr_scale_field.generic.statusbar	= "sets a custom distortion scale factor";
	s_options_vr_ovr_scale_field.generic.callback = CustomScaleFunc;
	s_options_vr_ovr_scale_field.generic.x		= MENU_FONT_SIZE;
	s_options_vr_ovr_scale_field.generic.y		= y+=2*MENU_LINE_SIZE;
	s_options_vr_ovr_scale_field.length	= 5;
	s_options_vr_ovr_scale_field.visible_length = 5;
	strcpy( s_options_vr_ovr_scale_field.buffer, vr_ovr_scale->string );
	s_options_vr_ovr_scale_field.cursor = strlen( vr_ovr_scale->string );

	s_options_vr_ovr_autolensdistance_box.generic.type			= MTYPE_SPINCONTROL;
	s_options_vr_ovr_autolensdistance_box.generic.x			= MENU_FONT_SIZE;
	s_options_vr_ovr_autolensdistance_box.generic.y			= y+=2*MENU_LINE_SIZE;
	s_options_vr_ovr_autolensdistance_box.generic.name			= "lens separation distance";
	s_options_vr_ovr_autolensdistance_box.generic.callback		= AutoFunc;
	s_options_vr_ovr_autolensdistance_box.itemnames			= auto_names;
	s_options_vr_ovr_autolensdistance_box.generic.statusbar	= "sets whether to use the automatic or custom lens separation distance";

	s_options_vr_ovr_lensdistance_field.generic.type = MTYPE_FIELD;
	s_options_vr_ovr_lensdistance_field.generic.flags = QMF_LEFT_JUSTIFY;
	s_options_vr_ovr_lensdistance_field.generic.name = "custom lens separation distance";
	s_options_vr_ovr_lensdistance_field.generic.statusbar	= "sets a custom lens seperation distance";
	s_options_vr_ovr_lensdistance_field.generic.callback = CustomLensFunc;
	s_options_vr_ovr_lensdistance_field.generic.x		= MENU_FONT_SIZE;
	s_options_vr_ovr_lensdistance_field.generic.y		= y+=2*MENU_LINE_SIZE;
	s_options_vr_ovr_lensdistance_field.length	= 5;
	s_options_vr_ovr_lensdistance_field.visible_length = 5;
	strcpy( s_options_vr_ovr_lensdistance_field.buffer, vr_ovr_lensdistance->string );
	s_options_vr_ovr_lensdistance_field.cursor = strlen( vr_ovr_lensdistance->string );

	s_options_vr_ovr_latency_box.generic.type		= MTYPE_SPINCONTROL;
	s_options_vr_ovr_latency_box.generic.x			= MENU_FONT_SIZE;
	s_options_vr_ovr_latency_box.generic.y			= y+=2*MENU_LINE_SIZE;
	s_options_vr_ovr_latency_box.generic.name		= "latency tester";
	s_options_vr_ovr_latency_box.generic.callback	= LatencyFunc;
	s_options_vr_ovr_latency_box.itemnames			= yesno_names;
	s_options_vr_ovr_latency_box.generic.statusbar	= "enables support for the oculus latency tester";

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

	Menu_AddItem( &s_options_vr_ovr_menu, ( void * ) &s_options_vr_ovr_drift_box );
	Menu_AddItem( &s_options_vr_ovr_menu, ( void * ) &s_options_vr_ovr_chroma_box );
	Menu_AddItem( &s_options_vr_ovr_menu, ( void * ) &s_options_vr_ovr_filtermode_box );
	Menu_AddItem( &s_options_vr_ovr_menu, ( void * ) &s_options_vr_ovr_prediction_field );
	
	Menu_AddItem( &s_options_vr_ovr_menu, ( void * ) &s_options_vr_ovr_distortion_box );
	Menu_AddItem( &s_options_vr_ovr_menu, ( void * ) &s_options_vr_ovr_autoscale_box );
	Menu_AddItem( &s_options_vr_ovr_menu, ( void * ) &s_options_vr_ovr_scale_field );
		
	Menu_AddItem( &s_options_vr_ovr_menu, ( void * ) &s_options_vr_ovr_autolensdistance_box );
	Menu_AddItem( &s_options_vr_ovr_menu, ( void * ) &s_options_vr_ovr_lensdistance_field );

	Menu_AddItem( &s_options_vr_ovr_menu, ( void * ) &s_options_vr_ovr_latency_box );	
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

const char *Options_VR_OVR_MenuKey( Sint32 key )
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

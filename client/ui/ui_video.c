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

// ui_video.c -- the video options menu

#include "../client.h"
#include "include/ui_local.h"
#include "../renderer/include/r_local.h"

extern cvar_t *vid_ref;

void Menu_Video_Init (void);

typedef struct vidmode_s
{
	const char *description;
	int32_t         width, height;
} vidmode_t;

/*
=======================================================================

VIDEO MENU

=======================================================================
*/

static menuframework_s	s_video_menu;
static menulist_s		s_mode_list;
static menulist_s  		s_fs_box;
static menuslider_s		s_brightness_slider;
static menulist_s		s_texqual_box;
static menulist_s		s_texfilter_box;
static menulist_s		s_aniso_box;
static menulist_s		s_antialias_box;
//static menulist_s		s_npot_mipmap_box;
static menulist_s  		s_texcompress_box;
static menulist_s  		s_vsync_box;
static menulist_s		s_fencesync_box;
static menulist_s		s_refresh_box;	// Knightmare- refresh rate option
static menuaction_s		s_advanced_action;
static menuaction_s		s_defaults_action;
static menuaction_s		s_apply_action;
static menuaction_s		s_backmain_action;


static vidmode_t vid_modes[] =
{
	{ "640x480",	640, 480},
	{ "800x600",	800, 600},
	{ "1024x768",	1024, 768},
	{ "1280x960",	1280, 960},
	{ "1280x1024",	1280, 1024},
	{ "1600x1200", 1600, 1200},
	{ "1280x720",	1280, 720},
	{ "1280x768",	1280, 768},
	{ "1280x800",	1280, 800},
	{ "1360x768",	1360, 768},
	{ "1366x768",	1366, 768},
	{ "1440x900",	1440, 900},
	{ "1600x900",	1600, 900},
	{ "1680x1050",	1680, 1050},
	{ "1920x1080",	1920, 1080},
	{ "1920x1200",	1920, 1200},
	{ "2560x1440",	2560, 1440},
	{ "2560x1600",	2560, 1600}
};

#define NUM_VIDEO_MODES (sizeof(vid_modes) / sizeof(vid_modes[0]))

static void BrightnessCallback( void *s )
{
	Cvar_SetValue( "vid_brightness", s_brightness_slider.curvalue / 20.0f);
}

static void VsyncCallback ( void *unused )
{
	int32_t mode = s_vsync_box.curvalue;

	Cvar_SetValue( "r_swapinterval", (mode >= 1));
	Cvar_SetValue( "r_adaptivevsync", (mode >= 2));
}

static void FenceSyncCallback (void *unused )
{
	Cvar_SetValue( "r_fencesync", s_fencesync_box.curvalue);
}

static void AntialiasCallback ( void *usused )
{
	int old = Cvar_VariableInteger("r_antialias");
	int newVal = s_antialias_box.curvalue;
	float oldscale, newscale, ratio;
	oldscale = R_AntialiasGetScale();
	Cvar_SetValue( "r_antialias", newVal);
	newscale = R_AntialiasGetScale();

	if (Cvar_VariableInteger("vr_enabled"))
		return;

	ratio = newscale / oldscale;
	cursor.x *= ratio;
	cursor.y *= ratio;
}

static void AdvancedOptions( void *s )
{
	M_Menu_Video_Advanced_f ();
}

static void ResetVideoDefaults ( void *unused )
{
	Cvar_SetToDefault ("vid_fullscreen");
	Cvar_SetToDefault ("vid_brightness");
	Cvar_SetToDefault ("vid_width");
	Cvar_SetToDefault ("vid_height");
	Cvar_SetToDefault ("r_texturemode");
	Cvar_SetToDefault ("r_anisotropic");
	Cvar_SetToDefault ("r_picmip");
	Cvar_SetToDefault ("r_swapinterval");
	Cvar_SetToDefault ("r_adaptivevsync");
	Cvar_SetToDefault ("r_fencesync");
	Cvar_SetToDefault ("r_lateframe_ratio");
	Cvar_SetToDefault ("r_lateframe_threshold");
	Cvar_SetToDefault ("r_lateframe_decay");
	Cvar_SetToDefault ("vid_refresh");
	Cvar_SetToDefault ("cl_widescreen_fov");

	Cvar_SetToDefault ("r_modulate");
	Cvar_SetToDefault ("r_intensity");
	Cvar_SetToDefault ("r_rgbscale");
	Cvar_SetToDefault ("r_trans_lighting");
	Cvar_SetToDefault ("r_warp_lighting");
	Cvar_SetToDefault ("r_lightcutoff");
	Cvar_SetToDefault ("r_glass_envmaps");
	Cvar_SetToDefault ("r_solidalpha");
	Cvar_SetToDefault ("r_waterquality");
	Cvar_SetToDefault ("r_waterwave");
	Cvar_SetToDefault ("r_blur");
	Cvar_SetToDefault ("r_blur_radius");
	Cvar_SetToDefault ("r_blur_texscale");
	Cvar_SetToDefault ("r_particle_overdraw");
	Cvar_SetToDefault ("r_bloom");
	Cvar_SetToDefault ("r_bloom_falloff");
	Cvar_SetToDefault ("r_bloom_minthreshold");
	Cvar_SetToDefault ("r_bloom_radius");
	Cvar_SetToDefault ("r_bloom_texscale");
	Cvar_SetToDefault ("r_model_shading");
	Cvar_SetToDefault ("r_shadows");
	Cvar_SetToDefault ("r_shelltype");
	Cvar_SetToDefault ("r_screenshot_jpeg");
	Cvar_SetToDefault ("r_screenshot_jpeg_quality");
	Cvar_SetToDefault ("r_saveshotsize");
	Cvar_SetToDefault ("r_antialias");
	Cvar_SetToDefault ("r_s3tc");
	Menu_Video_Init();
}

static void prepareVideoRefresh( void )
{
	// set the right mode for refresh
	Cvar_ForceSet( "vid_ref", "gl" );
	Cvar_ForceSet( "gl_driver", "opengl32" );

	// tell them they're modified so they refresh
	vid_ref->modified = true;
}


static void ApplyChanges( void *unused )
{
	int32_t		temp;

	temp = ClampCvar(0,NUM_VIDEO_MODES,s_mode_list.curvalue);
	if (temp != NUM_VIDEO_MODES)
	{

		Cvar_SetValue( "vid_width", vid_modes[temp].width ); // offset for eliminating < 640x480 modes
		Cvar_SetValue( "vid_height", vid_modes[temp].height ); // offset for eliminating < 640x480 modes
	}

	Cvar_SetValue( "vid_fullscreen", s_fs_box.curvalue );
	Cvar_SetValue( "vid_brightness", s_brightness_slider.curvalue/20.0f );
	Cvar_SetValue( "r_picmip", s_texqual_box.curvalue );
	Cvar_SetValue( "r_s3tc",s_texcompress_box.curvalue);
	// Knightmare- refesh rate option
	switch (s_refresh_box.curvalue)
	{
	case 9:
		Cvar_SetValue ("vid_refresh", 150);
		break;
	case 8:
		Cvar_SetValue ("vid_refresh", 120);
		break;
	case 7:
		Cvar_SetValue ("vid_refresh", 110);
		break;
	case 6:
		Cvar_SetValue ("vid_refresh", 100);
		break;
	case 5:
		Cvar_SetValue ("vid_refresh", 85);
		break;
	case 4:
		Cvar_SetValue ("vid_refresh", 75);
		break;
	case 3:
		Cvar_SetValue ("vid_refresh", 72);
		break;
	case 2:
		Cvar_SetValue ("vid_refresh", 70);
		break;
	case 1:
		Cvar_SetValue ("vid_refresh", 60);
		break;
	case 0:
	default:
		Cvar_SetValue ("vid_refresh", 0);
		break;
	}

	if (s_texfilter_box.curvalue == 0)
		Cvar_Set("r_texturemode", "GL_LINEAR_MIPMAP_NEAREST");
	else if (s_texfilter_box.curvalue == 1)
		Cvar_Set("r_texturemode", "GL_LINEAR_MIPMAP_LINEAR");

	switch ((int32_t)s_aniso_box.curvalue)
	{
		case 1: Cvar_SetValue( "r_anisotropic", 2.0 ); break;
		case 2: Cvar_SetValue( "r_anisotropic", 4.0 ); break;
		case 3: Cvar_SetValue( "r_anisotropic", 8.0 ); break;
		case 4: Cvar_SetValue( "r_anisotropic", 16.0 ); break;
		default:
		case 0: Cvar_SetValue( "r_anisotropic", 0.0 ); break;
	}


	VsyncCallback(NULL);
//	FenceSyncCallback(NULL);
//	AntialiasCallback(NULL);

//	Cvar_SetValue( "r_swapinterval", s_vsync_box.curvalue );

	prepareVideoRefresh ();

	//UI_ForceMenuOff();
}


// Knightmare added
int32_t texfilter_box_setval (void)
{
	char *texmode = Cvar_VariableString("r_texturemode");
	if (!Q_strcasecmp(texmode, "GL_LINEAR_MIPMAP_NEAREST"))
		return 0;
	else
		return 1;
}

// Knightmare- refresh rate option
int32_t refresh_box_setval (void)
{
	int32_t refreshVar = (int32_t)Cvar_VariableValue ("vid_refresh");

	if (refreshVar == 150)
		return 9;
	else if (refreshVar == 120)
		return 8;
	else if (refreshVar == 110)
		return 7;
	else if (refreshVar == 100)
		return 6;
	else if (refreshVar == 85)
		return 5;
	else if (refreshVar == 75)
		return 4;
	else if (refreshVar == 72)
		return 3;
	else if (refreshVar == 70)
		return 2;
	else if (refreshVar == 60)
		return 1;
	else
		return 0;
}

static const char *aniso0_names[] =
{
	"not supported",
	0
};

static const char *aniso2_names[] =
{
	"off",
	"2x",
	0
};

static const char *aniso4_names[] =
{
	"off",
	"2x",
	"4x",
	0
};

static const char *aniso8_names[] =
{
	"off",
	"2x",
	"4x",
	"8x",
	0
};

static const char *aniso16_names[] =
{
	"off",
	"2x",
	"4x",
	"8x",
	"16x",
	0
};

static const char **GetAnisoNames ()
{
	float aniso_avail = Cvar_VariableValue("r_anisotropic_avail");
	if (aniso_avail < 2.0)
		return aniso0_names;
	else if (aniso_avail < 4.0)
		return aniso2_names;
	else if (aniso_avail < 8.0)
		return aniso4_names;
	else if (aniso_avail < 16.0)
		return aniso8_names;
	else // >= 16.0
		return aniso16_names;
}


float GetAnisoCurValue ()
{
	float aniso_avail = Cvar_VariableValue("r_anisotropic_avail");
	float anisoValue = ClampCvar (0, aniso_avail, Cvar_VariableValue("r_anisotropic"));
	if (aniso_avail == 0) // not available
		return 0;
	if (anisoValue < 2.0)
		return 0;
	else if (anisoValue < 4.0)
		return 1;
	else if (anisoValue < 8.0)
		return 2;
	else if (anisoValue < 16.0)
		return 3;
	else // >= 16.0
		return 4;
}



/*
================
Menu_Video_Init
================
*/
void Menu_Video_Init (void)
{
	// Knightmare- added 1280x1024, 1400x1050, 856x480, 1024x480 modes, removed 320x240, 400x300, 512x384 modes
	static const char *resolutions[] = 
	{
		"[640x480  ]",
		"[800x600  ]",
		"[1024x768 ]",
		"[1280x960 ]",
		"[1280x1024]", // Knightmare added
		"[1600x1200]",
		"[1280x720 ]", // Knightmare added
		"[1280x768 ]", // Knightmare added
		"[1280x800 ]", // Knightmare added
		"[1360x768 ]", // Knightmare added
		"[1366x768 ]", // Knightmare added
		"[1440x900 ]", // Knightmare added
		"[1600x900 ]", // Knightmare added
		"[1680x1050]", // Knightmare added
		"[1920x1080]", // Knightmare added
		"[1920x1200]", // Knightmare added
		"[2560x1440]", // Knightmare added
		"[2560x1600]", // Knightmare added
		"[custom   ]",
		0
	};

	static const char *refreshrate_names[] = 
	{
		"[default]",
		"[60Hz   ]",
		"[70Hz   ]",
		"[72Hz   ]",
		"[75Hz   ]",
		"[85Hz   ]",
		"[100Hz  ]",
		"[110Hz  ]",
		"[120Hz  ]",
		"[150Hz  ]",
		0
	};
	static const char *yesno_names[] =
	{
		"no",
		"yes",
		0
	};
	static const char *sync_names[] =
	{
		"no",
		"vsync",
		"late frame",
		0
	};
	static const char *mip_names[] =
	{
		"bilinear",
		"trilinear",
		0
	};
	static const char *lmh_names[] =
	{
		"low",
		"medium",
		"high",
		"highest",
		0
	};

	static const char *vsync_names[] =
	{
		"no",
		"yes",
		"adaptive",
		0
	};


	static const char *antialias_names[] =
	{
		"off",
		"4x SSAA",
		"FXAA",
		"FXAA FSS",
		0
	};

	int32_t		y = 0;
	float	temp;

	int32_t i;
	int32_t j = sizeof(vid_modes) / sizeof(vidmode_t);
	for (i = 0; i < NUM_VIDEO_MODES; i++)
	{
		if (vid_modes[i].width == Cvar_VariableInteger("vid_width") 
			&& vid_modes[i].height == Cvar_VariableInteger("vid_height") )
			break;
	}
	temp = i;

	if ( !con_font_size )
		con_font_size = Cvar_Get ("con_font_size", "8", CVAR_ARCHIVE);

	s_video_menu.x = SCREEN_WIDTH*0.5;
//	s_video_menu.x = viddef.width * 0.50;
	s_video_menu.y = SCREEN_HEIGHT*0.5 - 80;
	s_video_menu.nitems = 0;

	s_mode_list.generic.type		= MTYPE_SPINCONTROL;
	s_mode_list.generic.name		= "video mode";
	s_mode_list.generic.x			= 0;
	s_mode_list.generic.y			= y;
	s_mode_list.itemnames			= resolutions;




	s_mode_list.curvalue			= temp; // offset for getting rid of < 640x480 resolutions
	s_mode_list.generic.statusbar	= "changes screen resolution";
	
	if (glConfig.ext_framebuffer_object)
	{
		s_antialias_box.generic.type		= MTYPE_SPINCONTROL;
		s_antialias_box.generic.x			= 0;
		s_antialias_box.generic.y			= y += MENU_LINE_SIZE;
		s_antialias_box.generic.name		= "post-process anti-aliasing";
		s_antialias_box.generic.callback	= AntialiasCallback;
		s_antialias_box.curvalue			= Cvar_VariableInteger("r_antialias");
		s_antialias_box.itemnames			= antialias_names;
		s_antialias_box.generic.statusbar	= "selects a software post-processing antialiasing technique";
	}


	s_fs_box.generic.type			= MTYPE_SPINCONTROL;
	s_fs_box.generic.x				= 0;
	s_fs_box.generic.y				= y += MENU_LINE_SIZE;
	s_fs_box.generic.name			= "fullscreen";
	s_fs_box.itemnames				= yesno_names;
	s_fs_box.curvalue				= Cvar_VariableValue("vid_fullscreen");
	s_fs_box.generic.statusbar		= "changes bettween fullscreen and windowed display";

	s_brightness_slider.generic.type		= MTYPE_SLIDER;
	s_brightness_slider.generic.x			= 0;
	s_brightness_slider.generic.y			= y += 2 * MENU_LINE_SIZE;
	s_brightness_slider.generic.name		= "brightness";
	s_brightness_slider.generic.callback	= BrightnessCallback;
	s_brightness_slider.minvalue			= 0;
	s_brightness_slider.maxvalue			= 20;
	s_brightness_slider.curvalue			= Cvar_VariableValue("vid_brightness") * 20.0f;
	s_brightness_slider.generic.statusbar	= "changes display brightness";

	s_texfilter_box.generic.type		= MTYPE_SPINCONTROL;
	s_texfilter_box.generic.x			= 0;
	s_texfilter_box.generic.y			= y += 2*MENU_LINE_SIZE;
	s_texfilter_box.generic.name		= "texture filter";
	s_texfilter_box.curvalue			= texfilter_box_setval();
	s_texfilter_box.itemnames			= mip_names;
	s_texfilter_box.generic.statusbar	= "changes texture filtering mode";

	s_aniso_box.generic.type		= MTYPE_SPINCONTROL;
	s_aniso_box.generic.x			= 0;
	s_aniso_box.generic.y			= y += MENU_LINE_SIZE;
	s_aniso_box.generic.name		= "anisotropic filter";
	s_aniso_box.curvalue			= GetAnisoCurValue();
	s_aniso_box.itemnames			= GetAnisoNames();
	s_aniso_box.generic.statusbar	= "changes level of anisotropic mipmap filtering";

	s_texqual_box.generic.type		= MTYPE_SPINCONTROL;
	s_texqual_box.generic.x			= 0;
	s_texqual_box.generic.y			= y += MENU_LINE_SIZE;
	s_texqual_box.generic.name		= "texture quality";
	s_texqual_box.curvalue			= ClampCvar (0, 3, 3-Cvar_VariableValue("r_picmip"));
	s_texqual_box.itemnames			= lmh_names;
	s_texqual_box.generic.statusbar	= "changes maximum texture size (highest = no limit)";

	if (glConfig.ext_texture_compression_s3tc)
	{
		s_texcompress_box.generic.type		= MTYPE_SPINCONTROL;
		s_texcompress_box.generic.x			= 0;
		s_texcompress_box.generic.y			= y += MENU_LINE_SIZE;
		s_texcompress_box.generic.name		= "texture compression";
		s_texcompress_box.itemnames			= yesno_names;
		s_texcompress_box.curvalue			= ClampCvar(0,1,Cvar_VariableValue("r_s3tc"));
		s_texcompress_box.generic.statusbar	= "enables texture compression";
	}

	s_vsync_box.generic.type			= MTYPE_SPINCONTROL;
	s_vsync_box.generic.x				= 0;
	s_vsync_box.generic.y				= y += 2*MENU_LINE_SIZE;
	s_vsync_box.generic.name			= "video sync";
	s_vsync_box.generic.callback		= VsyncCallback;

	if (glConfig.ext_swap_control_tear)
	{
		int32_t temp = !!Cvar_VariableInteger("r_swapinterval");
		temp += temp ? !!Cvar_VariableInteger("r_adaptivevsync") : 0;
		s_vsync_box.curvalue			= temp;
		s_vsync_box.itemnames			= vsync_names;
	} else {
		s_vsync_box.curvalue			= !!Cvar_VariableValue("r_swapinterval");
		s_vsync_box.itemnames			= yesno_names;
	}
	s_vsync_box.generic.statusbar		= "sync framerate with monitor refresh";

	if (glConfig.arb_sync)
	{
		s_fencesync_box.generic.type			= MTYPE_SPINCONTROL;
		s_fencesync_box.generic.x				= 0;
		s_fencesync_box.generic.y				= y += MENU_LINE_SIZE;
		s_fencesync_box.generic.name			= "gpu frame pacing";
		s_fencesync_box.generic.callback		= FenceSyncCallback;
		s_fencesync_box.curvalue				= ClampCvar(0,2, abs(Cvar_VariableInteger("r_fencesync")));
		s_fencesync_box.itemnames				= sync_names;
		s_fencesync_box.generic.statusbar		= "forces the gpu to synchronize after monitor refresh";
	}
	// Knightmare- refresh rate option
	s_refresh_box.generic.type			= MTYPE_SPINCONTROL;
	s_refresh_box.generic.x				= 0;
	s_refresh_box.generic.y				= y += MENU_LINE_SIZE;
	s_refresh_box.generic.name			= "refresh rate";
	s_refresh_box.curvalue				= refresh_box_setval();
	s_refresh_box.itemnames				= refreshrate_names;
	s_refresh_box.generic.statusbar		= "sets refresh rate for fullscreen modes";

	
	s_advanced_action.generic.type		= MTYPE_ACTION;
	s_advanced_action.generic.name		= "advanced options";
	s_advanced_action.generic.x			= 0;
	s_advanced_action.generic.y			= y += 3*MENU_LINE_SIZE;
	s_advanced_action.generic.callback	= AdvancedOptions;

	s_defaults_action.generic.type		= MTYPE_ACTION;
	s_defaults_action.generic.name		= "reset to defaults";
	s_defaults_action.generic.x			= 0;
	s_defaults_action.generic.y			= y += 3*MENU_LINE_SIZE;
	s_defaults_action.generic.callback	= ResetVideoDefaults;
	s_defaults_action.generic.statusbar	= "resets all video settings to internal defaults";

	// changed cancel to apply changes, thanx to MrG
	s_apply_action.generic.type			= MTYPE_ACTION;
	s_apply_action.generic.name			= "apply changes";
	s_apply_action.generic.x			= 0;
	s_apply_action.generic.y			= y += 2*MENU_LINE_SIZE;
	s_apply_action.generic.callback		= ApplyChanges;

	s_backmain_action.generic.type		= MTYPE_ACTION;
	s_backmain_action.generic.name		= "back to main";
	s_backmain_action.generic.x			= 0;
	s_backmain_action.generic.y			= y += 2*MENU_LINE_SIZE;
	s_backmain_action.generic.callback	= UI_BackMenu;

	Menu_AddItem( &s_video_menu, ( void * ) &s_mode_list );
	if (glConfig.ext_framebuffer_object)
		Menu_AddItem( &s_video_menu, ( void * ) &s_antialias_box );

	Menu_AddItem( &s_video_menu, ( void * ) &s_fs_box );
	Menu_AddItem( &s_video_menu, ( void * ) &s_brightness_slider );
	Menu_AddItem( &s_video_menu, ( void * ) &s_texfilter_box );
	Menu_AddItem( &s_video_menu, ( void * ) &s_aniso_box );
	Menu_AddItem( &s_video_menu, ( void * ) &s_texqual_box );

//	Menu_AddItem( &s_video_menu, ( void * ) &s_npot_mipmap_box );
	
	if (glConfig.ext_texture_compression_s3tc)
		Menu_AddItem( &s_video_menu, ( void * ) &s_texcompress_box );
	Menu_AddItem( &s_video_menu, ( void * ) &s_vsync_box );

	if (glConfig.arb_sync)
		Menu_AddItem( &s_video_menu, ( void * ) &s_fencesync_box );

	Menu_AddItem( &s_video_menu, ( void * ) &s_refresh_box );

	Menu_AddItem( &s_video_menu, ( void * ) &s_advanced_action );
	Menu_AddItem( &s_video_menu, ( void * ) &s_defaults_action );
	Menu_AddItem( &s_video_menu, ( void * ) &s_apply_action );
	Menu_AddItem( &s_video_menu, ( void * ) &s_backmain_action );

//	Menu_Center( &s_video_menu );
//	s_video_menu.x -= MENU_FONT_SIZE;
}

/*
================
Menu_Video_Draw
================
*/
void Menu_Video_Draw (void)
{
	//int32_t w, h;

	// draw the banner
	Menu_DrawBanner("m_banner_video");

	// move cursor to a reasonable starting position
	Menu_AdjustCursor( &s_video_menu, 1 );

	// draw the menu
	Menu_Draw( &s_video_menu );
}

/*
================
Video_MenuKey
================
*/
const char *Video_MenuKey( int32_t key )
{
	return Default_MenuKey( &s_video_menu, key );
}

void M_Menu_Video_f (void)
{
	Menu_Video_Init();
	UI_PushMenu( Menu_Video_Draw, Video_MenuKey );
}

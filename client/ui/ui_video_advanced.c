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

// ui_video_advanced.c -- the advanced video menu
 
#include <ctype.h>
#ifdef _WIN32
#include <io.h>
#endif
#include "../client.h"
#include "include/ui_local.h"

extern cvar_t *vid_ref;
// this cvar is needed for checking if it's been modified
cvar_t	*r_intensity;

/*
=======================================================================

ADVANCED VIDEO MENU

=======================================================================
*/
static menuframework_s	s_video_advanced_menu;
static menuseparator_s	s_options_advanced_header;	
static menulist_s  		s_rgbscale_box;
static menulist_s  		s_trans_lighting_box;
static menulist_s  		s_warp_lighting_box;
static menuslider_s		s_lightcutoff_slider;
static menulist_s  		s_solidalpha_box;
static menulist_s  		s_texshader_warp_box;
static menuslider_s  	s_waterwave_slider;
static menulist_s		s_particle_overdraw_box;
static menulist_s		s_lightbloom_box;
static menulist_s		s_modelshading_box;
static menulist_s		s_shadows_box;
static menulist_s  		s_glass_envmap_box;
static menuslider_s		s_blur_slider;
static menulist_s  		s_screenshotjpeg_box;
static menuslider_s  	s_screenshotjpegquality_slider;
static menulist_s  		s_saveshotsize_box;
static menuaction_s		s_advanced_apply_action;
static menuaction_s		s_back_action;


static void Video_Advanced_MenuSetValues ( void )
{
	Cvar_SetValue( "r_rgbscale", ClampCvar( 1, 2, Cvar_VariableValue("r_rgbscale") ) );
	if (Cvar_VariableValue("r_rgbscale") == 1)
		s_rgbscale_box.curvalue = 0;
	else
		s_rgbscale_box.curvalue = 1;

	Cvar_SetValue( "r_trans_lighting", ClampCvar( 0, 2, Cvar_VariableValue("r_trans_lighting") ) );
	s_trans_lighting_box.curvalue = Cvar_VariableValue("r_trans_lighting");

	Cvar_SetValue( "r_warp_lighting", ClampCvar( 0, 1, Cvar_VariableValue("r_warp_lighting") ) );
	s_warp_lighting_box.curvalue = Cvar_VariableValue("r_warp_lighting");

	Cvar_SetValue( "r_lightcutoff", ClampCvar( 0, 64, Cvar_VariableValue("r_lightcutoff") ) );
	s_lightcutoff_slider.curvalue = Cvar_VariableValue("r_lightcutoff") / 8.0f;

	Cvar_SetValue( "r_glass_envmaps", ClampCvar( 0, 1, Cvar_VariableValue("r_glass_envmaps") ) );
	s_glass_envmap_box.curvalue	= Cvar_VariableValue("r_glass_envmaps");

	Cvar_SetValue( "r_solidalpha", ClampCvar( 0, 1, Cvar_VariableValue("r_solidalpha") ) );
	s_solidalpha_box.curvalue = Cvar_VariableValue("r_solidalpha");

	Cvar_SetValue( "r_waterquality", ClampCvar( 0, 3, Cvar_VariableValue("r_waterquality") ) );
	s_texshader_warp_box.curvalue = Cvar_VariableValue("r_waterquality");

	Cvar_SetValue( "r_waterwave", ClampCvar( 0, 24, Cvar_VariableValue("r_waterwave") ) );
	s_waterwave_slider.curvalue = Cvar_VariableValue("r_waterwave");

	Cvar_SetValue( "r_particle_overdraw", ClampCvar( 0, 1, Cvar_VariableValue("r_particle_overdraw") ) );
	s_particle_overdraw_box.curvalue = Cvar_VariableValue("r_particle_overdraw");

	Cvar_SetValue( "r_bloom", ClampCvar( 0, 1, Cvar_VariableValue("r_bloom") ) );
	s_lightbloom_box.curvalue = Cvar_VariableValue("r_bloom");

	Cvar_SetValue( "r_model_shading", ClampCvar( 0, 3, Cvar_VariableValue("r_model_shading") ) );
	s_modelshading_box.curvalue	= Cvar_VariableValue("r_model_shading");

	Cvar_SetValue( "r_shadows", ClampCvar( 0, 3, Cvar_VariableValue("r_shadows") ) );
	s_shadows_box.curvalue	= Cvar_VariableValue("r_shadows");

	Cvar_SetValue( "r_blur_radius", ClampCvar( 0, 8, Cvar_VariableValue("r_blur_radius")));
	s_blur_slider.curvalue = Cvar_VariableValue("r_blur_radius");

	Cvar_SetValue( "r_screenshot_jpeg", ClampCvar( 0, 1, Cvar_VariableValue("r_screenshot_jpeg") ) );
	s_screenshotjpeg_box.curvalue = Cvar_VariableValue("r_screenshot_jpeg");

	Cvar_SetValue( "r_screenshot_jpeg_quality", ClampCvar( 50, 100, Cvar_VariableValue("r_screenshot_jpeg_quality") ) );
	s_screenshotjpegquality_slider.curvalue	= (Cvar_VariableValue("r_screenshot_jpeg_quality") -50) / 5;

	Cvar_SetValue( "r_saveshotsize", ClampCvar( 0, 1, Cvar_VariableValue("r_saveshotsize") ) );
	s_saveshotsize_box.curvalue	= Cvar_VariableValue("r_saveshotsize");
}

static void RGBSCaleCallback ( void *unused )
{
	Cvar_SetValue( "r_rgbscale", s_rgbscale_box.curvalue + 1);
}

static void TransLightingCallback ( void *unused )
{
	Cvar_SetValue( "r_trans_lighting", s_trans_lighting_box.curvalue);
}

static void WarpLightingCallback ( void *unused )
{
	Cvar_SetValue( "r_warp_lighting", s_warp_lighting_box.curvalue);
}

static void LightCutoffCallback( void *unused )
{
	Cvar_SetValue( "r_lightcutoff", s_lightcutoff_slider.curvalue * 8.0f);
}

static void GlassEnvmapCallback ( void *unused )
{
	Cvar_SetValue( "r_glass_envmaps", s_glass_envmap_box.curvalue);
}

static void SolidAlphaCallback ( void *unused )
{
	Cvar_SetValue( "r_solidalpha", s_solidalpha_box.curvalue);
}

static void TexShaderWarpCallback ( void *unused )
{
	Cvar_SetValue( "r_waterquality", s_texshader_warp_box.curvalue);
}

static void WaterWaveCallback ( void *unused )
{
	Cvar_SetValue( "r_waterwave", s_waterwave_slider.curvalue);
}

static void ParticleOverdrawCallback( void *unused )
{
	Cvar_SetValue( "r_particle_overdraw", s_particle_overdraw_box.curvalue);
}

static void LightBloomCallback( void *unused )
{
	Cvar_SetValue( "r_bloom", s_lightbloom_box.curvalue);
}

static void ModelShadingCallback ( void *unused )
{
	Cvar_SetValue( "r_model_shading", s_modelshading_box.curvalue);
}

static void ShadowsCallback ( void *unused )
{
	Cvar_SetValue( "r_shadows", s_shadows_box.curvalue);
}

static void JPEGScreenshotCallback ( void *unused )
{
	Cvar_SetValue( "r_screenshot_jpeg", s_screenshotjpeg_box.curvalue);
}

static void JPEGScreenshotQualityCallback ( void *unused )
{
	Cvar_SetValue( "r_screenshot_jpeg_quality", (s_screenshotjpegquality_slider.curvalue * 5 + 50));
}

static void SaveshotSizeCallback ( void *unused )
{
	Cvar_SetValue( "r_saveshotsize", s_saveshotsize_box.curvalue);
}

static void BlurCallback( void *unused )
{
	Cvar_SetValue( "r_blur_radius", s_blur_slider.curvalue);
}

static void AdvancedMenuApplyChanges ( void *unused )
{
	// update for modified r_intensity
	if (r_intensity->modified)
		vid_ref->modified = true;
}

/*
================
Menu_Video_Advanced_Init
================
*/
void Menu_Video_Advanced_Init (void)
{
	static const char *yesno_names[] =
	{
		"no",
		"yes",
		0
	};
	static const char *lighting_names[] =
	{
		"no",
		"vertex",
		"lightmap (if available)",
		0
	};
	static const char *shading_names[] =
	{
		"off",
		"low",
		"medium",
		"high",
		0
	};
	static const char *shadow_names[] =
	{
		"no",
		"static planar",
		"dynamic planar",
		"projection",
		0
	};
	static const char *ifsupported_names[] =
	{
		"no",
		"if supported",
		0
	};
	static const char *quality_names[] =
	{
		"low",
		"medium",
		"high",
		"extreme",
		0
	};

	int32_t y = 0;

	r_intensity = Cvar_Get ("r_intensity", "1", 0);

	s_video_advanced_menu.x = SCREEN_WIDTH*0.5;
	s_video_advanced_menu.y = SCREEN_HEIGHT*0.5 - 100;
	s_video_advanced_menu.nitems = 0;

	s_options_advanced_header.generic.type		= MTYPE_SEPARATOR;
	s_options_advanced_header.generic.name		= "Advanced Options";
	s_options_advanced_header.generic.x			= MENU_FONT_SIZE/2 * strlen(s_options_advanced_header.generic.name);
	s_options_advanced_header.generic.y			= y;

	s_rgbscale_box.generic.type				= MTYPE_SPINCONTROL;
	s_rgbscale_box.generic.x				= 0;
	s_rgbscale_box.generic.y				= y += MENU_LINE_SIZE;
	s_rgbscale_box.generic.name				= "RGB enhance";
	s_rgbscale_box.generic.callback			= RGBSCaleCallback;
	s_rgbscale_box.itemnames				= yesno_names;
	s_rgbscale_box.generic.statusbar		= "brightens textures without washing them out";

	s_trans_lighting_box.generic.type		= MTYPE_SPINCONTROL;
	s_trans_lighting_box.generic.x			= 0;
	s_trans_lighting_box.generic.y			= y += MENU_LINE_SIZE;
	s_trans_lighting_box.generic.name		= "translucent lighting";
	s_trans_lighting_box.generic.callback	= TransLightingCallback;
	s_trans_lighting_box.itemnames			= lighting_names;
	s_trans_lighting_box.generic.statusbar	= "lighting on translucent surfaces";

	s_warp_lighting_box.generic.type		= MTYPE_SPINCONTROL;
	s_warp_lighting_box.generic.x			= 0;
	s_warp_lighting_box.generic.y			= y += MENU_LINE_SIZE;
	s_warp_lighting_box.generic.name		= "warp surface lighting";
	s_warp_lighting_box.generic.callback	= WarpLightingCallback;
	s_warp_lighting_box.itemnames			= yesno_names;
	s_warp_lighting_box.generic.statusbar	= "vertex lighting on water and other warping surfaces";

	s_lightcutoff_slider.generic.type		= MTYPE_SLIDER;
	s_lightcutoff_slider.generic.x			= 0;
	s_lightcutoff_slider.generic.y			= y += MENU_LINE_SIZE;
	s_lightcutoff_slider.generic.name		= "dynamic light cutoff";
	s_lightcutoff_slider.generic.callback	= LightCutoffCallback;
	s_lightcutoff_slider.minvalue			= 0;
	s_lightcutoff_slider.maxvalue			= 8;
	s_lightcutoff_slider.generic.statusbar	= "lower = smoother blend, higher = faster";

	s_glass_envmap_box.generic.type			= MTYPE_SPINCONTROL;
	s_glass_envmap_box.generic.x			= 0;
	s_glass_envmap_box.generic.y			= y += MENU_LINE_SIZE;
	s_glass_envmap_box.generic.name			= "glass envmaps";
	s_glass_envmap_box.generic.callback		= GlassEnvmapCallback;
	s_glass_envmap_box.itemnames			= yesno_names;
	s_glass_envmap_box.generic.statusbar	= "enable environment mapping on transparent surfaces";

	s_solidalpha_box.generic.type			= MTYPE_SPINCONTROL;
	s_solidalpha_box.generic.x				= 0;
	s_solidalpha_box.generic.y				= y += MENU_LINE_SIZE;
	s_solidalpha_box.generic.name			= "solid alphas";
	s_solidalpha_box.generic.callback		= SolidAlphaCallback;
	s_solidalpha_box.itemnames				= yesno_names;
	s_solidalpha_box.generic.statusbar		= "enable solid drawing of trans33 + trans66 surfaces";

	s_texshader_warp_box.generic.type		= MTYPE_SPINCONTROL;
	s_texshader_warp_box.generic.x			= 0;
	s_texshader_warp_box.generic.y			= y += MENU_LINE_SIZE;
	s_texshader_warp_box.generic.name		= "water quality";
	s_texshader_warp_box.generic.callback	= TexShaderWarpCallback;
	s_texshader_warp_box.itemnames			= quality_names;
	s_texshader_warp_box.generic.statusbar	= "enables hardware water warping and caustic effect";

	s_waterwave_slider.generic.type			= MTYPE_SLIDER;
	s_waterwave_slider.generic.x			= 0;
	s_waterwave_slider.generic.y			= y += MENU_LINE_SIZE;
	s_waterwave_slider.generic.name			= "water wave size";
	s_waterwave_slider.generic.callback		= WaterWaveCallback;
	s_waterwave_slider.minvalue				= 0;
	s_waterwave_slider.maxvalue				= 24;
	s_waterwave_slider.generic.statusbar	= "size of waves on flat water surfaces";

	s_blur_slider.generic.type			= MTYPE_SLIDER;
	s_blur_slider.generic.x				= 0;
	s_blur_slider.generic.y				= y += MENU_LINE_SIZE;
	s_blur_slider.generic.name			= "blur strength";
	s_blur_slider.generic.callback		= BlurCallback;
	s_blur_slider.minvalue				= 0;
	s_blur_slider.maxvalue				= 8;
	s_blur_slider.generic.statusbar	= "strength of blur applied";

	s_particle_overdraw_box.generic.type		= MTYPE_SPINCONTROL;
	s_particle_overdraw_box.generic.x			= 0;
	s_particle_overdraw_box.generic.y			= y += 2*MENU_LINE_SIZE;
	s_particle_overdraw_box.generic.name		= "particle overdraw";
	s_particle_overdraw_box.generic.callback	= ParticleOverdrawCallback;
	s_particle_overdraw_box.itemnames			= yesno_names;
	s_particle_overdraw_box.generic.statusbar	= "redraw particles over trans surfaces";

	s_lightbloom_box.generic.type			= MTYPE_SPINCONTROL;
	s_lightbloom_box.generic.x				= 0;
	s_lightbloom_box.generic.y				= y += MENU_LINE_SIZE;
	s_lightbloom_box.generic.name			= "light blooms";
	s_lightbloom_box.generic.callback		= LightBloomCallback;
	s_lightbloom_box.itemnames				= yesno_names;
	s_lightbloom_box.generic.statusbar		= "enables blooming of bright lights";

	s_modelshading_box.generic.type			= MTYPE_SPINCONTROL;
	s_modelshading_box.generic.x			= 0;
	s_modelshading_box.generic.y			= y += MENU_LINE_SIZE;
	s_modelshading_box.generic.name			= "model shading";
	s_modelshading_box.generic.callback		= ModelShadingCallback;
	s_modelshading_box.itemnames			= shading_names;
	s_modelshading_box.generic.statusbar	= "level of shading to use on models";

	s_shadows_box.generic.type				= MTYPE_SPINCONTROL;
	s_shadows_box.generic.x					= 0;
	s_shadows_box.generic.y					= y += MENU_LINE_SIZE;
	s_shadows_box.generic.name				= "entity shadows";
	s_shadows_box.generic.callback			= ShadowsCallback;
	s_shadows_box.itemnames					= shadow_names;
	s_shadows_box.generic.statusbar			= "type of model shadows to draw";

	s_screenshotjpeg_box.generic.type			= MTYPE_SPINCONTROL;
	s_screenshotjpeg_box.generic.x				= 0;
	s_screenshotjpeg_box.generic.y				= y += 2*MENU_LINE_SIZE;
	s_screenshotjpeg_box.generic.name			= "JPEG screenshots";
	s_screenshotjpeg_box.generic.callback		= JPEGScreenshotCallback;
	s_screenshotjpeg_box.itemnames				= yesno_names;
	s_screenshotjpeg_box.generic.statusbar		= "whether to take JPG screenshots instead of TGA";

	s_screenshotjpegquality_slider.generic.type			= MTYPE_SLIDER;
	s_screenshotjpegquality_slider.generic.x			= 0;
	s_screenshotjpegquality_slider.generic.y			= y += MENU_LINE_SIZE;
	s_screenshotjpegquality_slider.generic.name			= "JPEG screenshot quality";
	s_screenshotjpegquality_slider.generic.callback		= JPEGScreenshotQualityCallback;
	s_screenshotjpegquality_slider.minvalue				= 0;
	s_screenshotjpegquality_slider.maxvalue				= 10;
	s_screenshotjpegquality_slider.generic.statusbar	= "quality of JPG screenshots, 50-100%";

	s_saveshotsize_box.generic.type				= MTYPE_SPINCONTROL;
	s_saveshotsize_box.generic.x				= 0;
	s_saveshotsize_box.generic.y				= y += MENU_LINE_SIZE;
	s_saveshotsize_box.generic.name				= "hi-res saveshots";
	s_saveshotsize_box.generic.callback			= SaveshotSizeCallback;
	s_saveshotsize_box.itemnames				= yesno_names;
	s_saveshotsize_box.generic.statusbar		= "hi-res saveshots when running at 800x600 or higher";

	s_advanced_apply_action.generic.type		= MTYPE_ACTION;
	s_advanced_apply_action.generic.name		= "apply changes";
	s_advanced_apply_action.generic.x			= 0;
	s_advanced_apply_action.generic.y			= y += 2*MENU_LINE_SIZE;
	s_advanced_apply_action.generic.callback	= AdvancedMenuApplyChanges;

	s_back_action.generic.type = MTYPE_ACTION;
	s_back_action.generic.name = "back";
	s_back_action.generic.x    = 0;
	s_back_action.generic.y    = y += 2*MENU_LINE_SIZE;
	s_back_action.generic.callback = UI_BackMenu;

	Video_Advanced_MenuSetValues();

	Menu_AddItem( &s_video_advanced_menu, ( void * ) &s_options_advanced_header );
	Menu_AddItem( &s_video_advanced_menu, ( void * ) &s_rgbscale_box );
	Menu_AddItem( &s_video_advanced_menu, ( void * ) &s_trans_lighting_box );
	Menu_AddItem( &s_video_advanced_menu, ( void * ) &s_warp_lighting_box );
	Menu_AddItem( &s_video_advanced_menu, ( void * ) &s_lightcutoff_slider );
	Menu_AddItem( &s_video_advanced_menu, ( void * ) &s_glass_envmap_box );
	Menu_AddItem( &s_video_advanced_menu, ( void * ) &s_solidalpha_box );
	Menu_AddItem( &s_video_advanced_menu, ( void * ) &s_texshader_warp_box );
	Menu_AddItem( &s_video_advanced_menu, ( void * ) &s_waterwave_slider );
	Menu_AddItem( &s_video_advanced_menu, ( void * ) &s_blur_slider );
	Menu_AddItem( &s_video_advanced_menu, ( void * ) &s_particle_overdraw_box );
	Menu_AddItem( &s_video_advanced_menu, ( void * ) &s_lightbloom_box );
	Menu_AddItem( &s_video_advanced_menu, ( void * ) &s_modelshading_box );
	Menu_AddItem( &s_video_advanced_menu, ( void * ) &s_shadows_box );
	Menu_AddItem( &s_video_advanced_menu, ( void * ) &s_screenshotjpeg_box );
	Menu_AddItem( &s_video_advanced_menu, ( void * ) &s_screenshotjpegquality_slider );
	Menu_AddItem( &s_video_advanced_menu, ( void * ) &	s_saveshotsize_box );

	Menu_AddItem( &s_video_advanced_menu, ( void * ) &s_advanced_apply_action );

	Menu_AddItem( &s_video_advanced_menu, ( void * ) &s_back_action );

//	Menu_Center( &s_video_advanced_menu );
//	s_video_advanced_menu.x -= MENU_FONT_SIZE;	
}

/*
================
Menu_Video_Advanced_Draw
================
*/
void Menu_Video_Advanced_Draw (void)
{
	//int32_t w, h;

	// draw the banner
	Menu_DrawBanner("m_banner_video");

	// move cursor to a reasonable starting position
	Menu_AdjustCursor( &s_video_advanced_menu, 1 );

	// draw the menu
	Menu_Draw( &s_video_advanced_menu );
}

/*
================
Video_Advanced_MenuKey
================
*/
const char *Video_Advanced_MenuKey( int32_t key )
{
	return Default_MenuKey( &s_video_advanced_menu, key );

}

void M_Menu_Video_Advanced_f (void)
{
	Menu_Video_Advanced_Init();
	UI_PushMenu( Menu_Video_Advanced_Draw, Video_Advanced_MenuKey );
}

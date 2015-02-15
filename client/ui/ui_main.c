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

// ui_main.c -- the main menu & support functions
 
#include <ctype.h>
#ifdef _WIN32
#include <io.h>
#endif
#include "../client.h"
#include "include/ui_local.h"

static int32_t	m_main_cursor;

// for checking if quad cursor model is available
#define QUAD_CURSOR_MODEL	"models/ui/quad_cursor.md2"
static qboolean	quadModel_loaded;
static struct model_s *quadModel;


/*
=======================================================================

MAIN MENU

=======================================================================
*/

#define	MAIN_ITEMS	5

char *main_names[] =
{
	"m_main_game",
	"m_main_multiplayer",
	"m_main_options",
	"m_main_video",
	"m_main_quit",
	0
};


/*
=============
FindMenuCoords
=============
*/
void FindMenuCoords (int32_t *xoffset, int32_t *ystart, int32_t *totalheight, int32_t *widest)
{
	int32_t w, h, i;

	*totalheight = 0;
	*widest = -1;

	for (i = 0; main_names[i] != 0; i++)
	{
		R_DrawGetPicSize (&w, &h, main_names[i]);
		if (w > *widest)
			*widest = w;
		*totalheight += (h + 12);
	}

	*xoffset = (SCREEN_WIDTH - *widest + 70) * 0.5;
	*ystart = SCREEN_HEIGHT*0.5 - 100;
}


/*
=============
UI_DrawMainCursor

Draws an animating cursor with the point at
x,y.  The pic will extend to the left of x,
and both above and below y.
=============
*/
void UI_DrawMainCursor (int32_t x, int32_t y, int32_t f)
{
	char	cursorname[80];
	static	qboolean cached;
	int32_t		w,h;
    struct image_s *cursor_frame = NULL;
	if (!cached)
	{
		int32_t i;

		for (i = 0; i < NUM_MAINMENU_CURSOR_FRAMES; i++) {
			Com_sprintf (cursorname, sizeof(cursorname), "m_cursor%d", i);
			R_DrawFindPic (cursorname);
		}
		cached = true;
	}

	Com_sprintf (cursorname, sizeof(cursorname), "m_cursor%d", f);
    if ((cursor_frame = R_DrawFindPic(cursorname))) {
        R_DrawGetImageSize (&w, &h, cursor_frame);
        SCR_DrawImage (x, y, w, h, ALIGN_CENTER, cursor_frame, 1.0);
    }
}


/*
=============
UI_CheckQuadModel

Checks for quad damage model.
=============
*/

void UI_CheckQuadModel (void)
{

	quadModel = R_RegisterModel (QUAD_CURSOR_MODEL);
	
	quadModel_loaded = (quadModel != NULL);
}

/*
=============
UI_DrawMainCursor3D

Draws a rotating quad damage model.
=============
*/
void UI_DrawMainCursor3D (int32_t x, int32_t y)
{
	refdef_t	refdef;
	entity_t	quadEnt, *ent;
	float		rx, ry, rw, rh;
	int32_t			yaw;

	yaw = anglemod(cls.realtime/10);

	memset(&refdef, 0, sizeof(refdef));
	memset (&quadEnt, 0, sizeof(quadEnt));

	// size 24x34
	rx = x;				ry = y;
	rw = 24;			rh = 34;
	SCR_AdjustFrom640 (&rx, &ry, &rw, &rh, ALIGN_CENTER);
	refdef.x = rx;		refdef.y = ry;
	refdef.width = rw;	refdef.height = rh;
	refdef.fov_x = 40;
	refdef.fov_y = CalcFov (refdef.fov_x, refdef.width, refdef.height);
	refdef.time = cls.realtime*0.001;
	refdef.areabits = 0;
	refdef.lightstyles = 0;
	refdef.rdflags = RDF_NOWORLDMODEL;
	refdef.num_entities = 0;
	refdef.entities = &quadEnt;

	ent = &quadEnt;
	ent->model = R_RegisterModel(QUAD_CURSOR_MODEL);
	ent->flags = RF_FULLBRIGHT|RF_NOSHADOW|RF_DEPTHHACK;
	VectorSet (ent->origin, 40, 0, -18);
	VectorCopy( ent->origin, ent->oldorigin );
	ent->frame = 0;
	ent->oldframe = 0;
	ent->backlerp = 0.0;
	ent->angles[1] = yaw;
	refdef.num_entities++;

	R_RenderFrame( &refdef );
}



/*
=============
M_Main_Draw
=============
*/
void M_Main_Draw (void)
{
	int32_t i;
	int32_t w, h, last_h;
	int32_t ystart;
	int32_t	xoffset;
	int32_t widest = -1;
	int32_t totalheight = 0;
	char litname[80];
    struct image_s *img;
	FindMenuCoords (&xoffset, &ystart, &totalheight, &widest);

	for (i = 0; main_names[i] != 0; i++)
		if (i != m_main_cursor) {
            struct image_s *img;
            if ((img = R_DrawFindPic(main_names[i]))) {
                R_DrawGetImageSize (&w, &h, img );
                SCR_DrawImage (xoffset, (ystart + i*40+3), w, h, ALIGN_CENTER, img , 1.0);
            }
		}

	strcpy (litname, main_names[m_main_cursor]);
	strcat (litname, "_sel");
    img = NULL;
    if ((img = R_DrawFindPic(litname))) {
        R_DrawGetImageSize (&w, &h, img);
        SCR_DrawImage (xoffset-1, (ystart + m_main_cursor*40+2), w+2, h+2, ALIGN_CENTER, img, 1.0);
    }
	// Draw our nifty quad damage model as a cursor if it's loaded.
	if (quadModel_loaded)
		UI_DrawMainCursor3D (xoffset-27, ystart+(m_main_cursor*40+1));
	else
		UI_DrawMainCursor (xoffset-25, ystart+(m_main_cursor*40+1), (int32_t)(cls.realtime/100)%NUM_MAINMENU_CURSOR_FRAMES);

    img = NULL;
    if ((img = R_DrawFindPic("m_main_plaque"))) {
        R_DrawGetImageSize (&w, &h, img);
        SCR_DrawImage (xoffset-(w/2+50), ystart, w, h, ALIGN_CENTER, img, 1.0);
        last_h = h;
    }

    img = NULL;
    if ((img = R_DrawFindPic("m_main_logo"))) {
        R_DrawGetImageSize (&w, &h, img);
        SCR_DrawImage (xoffset-(w/2+50), ystart+last_h+20, w, h, ALIGN_CENTER, img, 1.0);
        last_h = h;
    }
}


/*
=============
OpenMenuFromMain
=============
*/
void OpenMenuFromMain (void)
{
	switch (m_main_cursor)
	{
		case 0:
			M_Menu_Game_f ();
			break;

		case 1:
			M_Menu_Multiplayer_f();
			break;

		case 2:
			M_Menu_Options_f ();
			break;

		case 3:
			M_Menu_Video_f ();
			break;

		case 4:
			M_Menu_Quit_f ();
			break;
	}
}


/*
=============
UI_CheckMainMenuMouse
=============
*/
void UI_CheckMainMenuMouse (void)
{
	int32_t ystart;
	int32_t	xoffset;
	int32_t widest;
	int32_t totalheight;
	int32_t i, oldhover;
	char *sound = NULL;
	mainmenuobject_t buttons[MAIN_ITEMS];

	oldhover = MainMenuMouseHover;
	MainMenuMouseHover = 0;

	FindMenuCoords(&xoffset, &ystart, &totalheight, &widest);
	for (i = 0; main_names[i] != 0; i++)
		UI_AddMainButton (&buttons[i], i, xoffset, ystart+(i*40+3), main_names[i]);

	// Exit with double click 2nd mouse button
	if (!cursor.buttonused[MOUSEBUTTON2] && cursor.buttonclicks[MOUSEBUTTON2]==2)
	{
		UI_PopMenu();
		sound = menu_out_sound;
		cursor.buttonused[MOUSEBUTTON2] = true;
		cursor.buttonclicks[MOUSEBUTTON2] = 0;
	}

	for (i=MAIN_ITEMS-1; i>=0; i--)
	{
		if (cursor.x>=buttons[i].min[0] && cursor.x<=buttons[i].max[0] &&
			cursor.y>=buttons[i].min[1] && cursor.y<=buttons[i].max[1])
		{
			if (cursor.mouseaction)
				m_main_cursor = i;

			MainMenuMouseHover = 1 + i;

			if (oldhover == MainMenuMouseHover && MainMenuMouseHover-1 == m_main_cursor &&
				!cursor.buttonused[MOUSEBUTTON1] && cursor.buttonclicks[MOUSEBUTTON1]==1)
			{
				OpenMenuFromMain();
				sound = menu_move_sound;
				cursor.buttonused[MOUSEBUTTON1] = true;
				cursor.buttonclicks[MOUSEBUTTON1] = 0;
			}
			break;
		}
	}

	if (!MainMenuMouseHover)
	{
		cursor.buttonused[MOUSEBUTTON1] = false;
		cursor.buttonclicks[MOUSEBUTTON1] = 0;
		cursor.buttontime[MOUSEBUTTON1] = 0;
	}

	cursor.mouseaction = false;

	if (sound)
		S_StartLocalSound(sound);
}


/*
=============
M_Main_Key
=============
*/
const char *M_Main_Key (int32_t key)
{
	const char *sound = menu_move_sound;

	switch (key)
	{
	case K_GAMEPAD_B:
	case K_GAMEPAD_BACK:
	case K_GAMEPAD_START:
	case K_ESCAPE:
#ifdef ERASER_COMPAT_BUILD // special hack for Eraser build
		if (cls.state == ca_disconnected)
			M_Menu_Quit_f ();
		else
			UI_PopMenu ();
#else	// can't exit menu if disconnected,
		// so restart demo loop
		if (cls.state == ca_disconnected)
			Cbuf_AddText ("d1\n");
		UI_PopMenu ();
#endif
		break;
	case K_GAMEPAD_DOWN:
	case K_GAMEPAD_LSTICK_DOWN:
	case K_GAMEPAD_RSTICK_DOWN:
	case K_KP_DOWNARROW:
	case K_DOWNARROW:
		if (++m_main_cursor >= MAIN_ITEMS)
			m_main_cursor = 0;
		return sound;
	case K_GAMEPAD_UP:
	case K_GAMEPAD_LSTICK_UP:
	case K_GAMEPAD_RSTICK_UP:
	case K_KP_UPARROW:
	case K_UPARROW:
		if (--m_main_cursor < 0)
			m_main_cursor = MAIN_ITEMS - 1;
		return sound;

	case K_GAMEPAD_A:
	case K_KP_ENTER:
	case K_ENTER:
		m_entersound = true;

		switch (m_main_cursor)
		{
		case 0:
			M_Menu_Game_f ();
			break;

		case 1:
			M_Menu_Multiplayer_f();
			break;

		case 2:
			M_Menu_Options_f ();
			break;

		case 3:
			M_Menu_Video_f ();
			break;

		case 4:
			M_Menu_Quit_f ();
			break;
		}
	}
	return NULL;
}


/*
=============
M_Menu_Main_f
=============
*/
void M_Menu_Main_f (void)
{
	UI_CheckQuadModel ();
	UI_PushMenu (M_Main_Draw, M_Main_Key, NULL);
}

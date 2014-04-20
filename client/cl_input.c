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
// cl.input.c  -- builds an intended movement command to send to the server

#include "client.h"

cvar_t	*cl_nodelta;

extern	unsigned	sys_frame_time;
unsigned	frame_msec;
unsigned	old_sys_frame_time;

/*
===============================================================================

KEY BUTTONS

Continuous button event tracking is complicated by the fact that two different
input sources (say, mouse button 1 and the control key) can both press the
same button, but the button should only be released when both of the
pressing key have been released.

When a key event issues a button command (+forward, +attack, etc), it appends
its key number as a parameter to the command so it can be matched up with
the release.

state bit 0 is the current state of the key
state bit 1 is edge triggered on the up to down transition
state bit 2 is edge triggered on the down to up transition


Key_Event (int32_t key, qboolean down, unsigned time);

  +mlook src time

===============================================================================
*/


kbutton_t	in_klook;
kbutton_t	in_left, in_right, in_forward, in_back;
kbutton_t	in_lookup, in_lookdown, in_moveleft, in_moveright;
kbutton_t	in_strafe, in_speed, in_use, in_attack, in_attack2;
kbutton_t	in_up, in_down;

int32_t			in_impulse;


void KeyDown (kbutton_t *b)
{
	int32_t		k;
	char	*c;
	
	c = Cmd_Argv(1);
	if (c[0])
		k = atoi(c);
	else
		k = -1;		// typed manually at the console for continuous down

	if (k == b->down[0] || k == b->down[1])
		return;		// repeating key
	
	if (!b->down[0])
		b->down[0] = k;
	else if (!b->down[1])
		b->down[1] = k;
	else
	{
		Com_Printf ("Three keys down for a button!\n");
		return;
	}
	
	if (b->state & 1)
		return;		// still down

	// save timestamp
	c = Cmd_Argv(2);
	b->downtime = atoi(c);
	if (!b->downtime)
		b->downtime = sys_frame_time - 100;

	b->state |= 1 + 2;	// down + impulse down
}

void KeyUp (kbutton_t *b)
{
	int32_t		k;
	char	*c;
	unsigned	uptime;

	c = Cmd_Argv(1);
	if (c[0])
		k = atoi(c);
	else
	{ // typed manually at the console, assume for unsticking, so clear all
		b->down[0] = b->down[1] = 0;
		b->state = 4;	// impulse up
		return;
	}

	if (b->down[0] == k)
		b->down[0] = 0;
	else if (b->down[1] == k)
		b->down[1] = 0;
	else
		return;		// key up without coresponding down (menu pass through)
	if (b->down[0] || b->down[1])
		return;		// some other key is still holding it down

	if (!(b->state & 1))
		return;		// still up (this should not happen)

	// save timestamp
	c = Cmd_Argv(2);
	uptime = atoi(c);
	if (uptime)
		b->msec += uptime - b->downtime;
	else
		b->msec += 10;

	b->state &= ~1;		// now up
	b->state |= 4; 		// impulse up
}

void IN_KLookDown (void) {KeyDown(&in_klook);}
void IN_KLookUp (void) {KeyUp(&in_klook);}
void IN_UpDown(void) {KeyDown(&in_up);}
void IN_UpUp(void) {KeyUp(&in_up);}
void IN_DownDown(void) {KeyDown(&in_down);}
void IN_DownUp(void) {KeyUp(&in_down);}
void IN_LeftDown(void) {KeyDown(&in_left);}
void IN_LeftUp(void) {KeyUp(&in_left);}
void IN_RightDown(void) {KeyDown(&in_right);}
void IN_RightUp(void) {KeyUp(&in_right);}
void IN_ForwardDown(void) {KeyDown(&in_forward);}
void IN_ForwardUp(void) {KeyUp(&in_forward);}
void IN_BackDown(void) {KeyDown(&in_back);}
void IN_BackUp(void) {KeyUp(&in_back);}
void IN_LookupDown(void) {KeyDown(&in_lookup);}
void IN_LookupUp(void) {KeyUp(&in_lookup);}
void IN_LookdownDown(void) {KeyDown(&in_lookdown);}
void IN_LookdownUp(void) {KeyUp(&in_lookdown);}
void IN_MoveleftDown(void) {KeyDown(&in_moveleft);}
void IN_MoveleftUp(void) {KeyUp(&in_moveleft);}
void IN_MoverightDown(void) {KeyDown(&in_moveright);}
void IN_MoverightUp(void) {KeyUp(&in_moveright);}

void IN_SpeedDown(void) {KeyDown(&in_speed);}
void IN_SpeedUp(void) {KeyUp(&in_speed);}
void IN_StrafeDown(void) {KeyDown(&in_strafe);}
void IN_StrafeUp(void) {KeyUp(&in_strafe);}

void IN_AttackDown(void) {KeyDown(&in_attack);}
void IN_AttackUp(void) {KeyUp(&in_attack);}

//Knightmare added
void IN_Attack2Down(void) {KeyDown(&in_attack2);}
void IN_Attack2Up(void) {KeyUp(&in_attack2);}
// end Knightmare

void IN_UseDown (void) {KeyDown(&in_use);}
void IN_UseUp (void) {KeyUp(&in_use);}

void IN_Impulse (void) {in_impulse=atoi(Cmd_Argv(1));}

/*
===============
CL_KeyState

Returns the fraction of the frame that the key was down
===============
*/
float CL_KeyState (kbutton_t *key)
{
	float		val;
	int32_t			msec;

	key->state &= 1;		// clear impulses

	msec = key->msec;
	key->msec = 0;

	if (key->state)
	{	// still down
		msec += sys_frame_time - key->downtime;
		key->downtime = sys_frame_time;
	}

#if 0
	if (msec)
	{
		Com_Printf ("%i ", msec);
	}
#endif

	val = (float)msec / frame_msec;
	if (val < 0)
		val = 0;
	if (val > 1)
		val = 1;

	return val;
}




//==========================================================================

cvar_t	*cl_upspeed;
cvar_t	*cl_forwardspeed;
cvar_t	*cl_sidespeed;

cvar_t	*cl_yawspeed;
cvar_t	*cl_pitchspeed;

cvar_t	*cl_run;

cvar_t	*cl_anglespeedkey;


/*
================
CL_AdjustAngles

Moves the local angle positions
================
*/
void CL_AdjustAngles (void)
{
	float	speed;
	float	up, down;
	
	if (in_speed.state & 1)
		speed = cls.frametime * cl_anglespeedkey->value;
	else
		speed = cls.frametime;

	if (!(in_strafe.state & 1))
	{
		cl.aimangles[YAW] -= speed*cl_yawspeed->value*CL_KeyState (&in_right);
		cl.aimangles[YAW] += speed*cl_yawspeed->value*CL_KeyState (&in_left);
	}
	if (in_klook.state & 1)
	{
		cl.aimangles[PITCH] -= speed*cl_pitchspeed->value * CL_KeyState (&in_forward);
		cl.aimangles[PITCH] += speed*cl_pitchspeed->value * CL_KeyState (&in_back);
	}
	
	up = CL_KeyState (&in_lookup);
	down = CL_KeyState(&in_lookdown);
	
	cl.aimangles[PITCH] -= speed*cl_pitchspeed->value * up;
	cl.aimangles[PITCH] += speed*cl_pitchspeed->value * down;
}

/*
================
CL_BaseMove

Send the intended movement message to the server
================
*/
void CL_BaseMove (usercmd_t *cmd)
{	
	CL_AdjustAngles ();
	
	memset (cmd, 0, sizeof(*cmd));
	
	VectorCopy (cl.aimangles, cmd->angles);
	if (in_strafe.state & 1)
	{
		cmd->sidemove += cl_sidespeed->value * CL_KeyState (&in_right);
		cmd->sidemove -= cl_sidespeed->value * CL_KeyState (&in_left);
	}

	cmd->sidemove += cl_sidespeed->value * CL_KeyState (&in_moveright);
	cmd->sidemove -= cl_sidespeed->value * CL_KeyState (&in_moveleft);

	cmd->upmove += cl_upspeed->value * CL_KeyState (&in_up);
	cmd->upmove -= cl_upspeed->value * CL_KeyState (&in_down);

	if (! (in_klook.state & 1) )
	{	
		cmd->forwardmove += cl_forwardspeed->value * CL_KeyState (&in_forward);
		cmd->forwardmove -= cl_forwardspeed->value * CL_KeyState (&in_back);
	}	

//
// adjust for speed key / running
//
	if ( (in_speed.state & 1) ^ (int32_t)(cl_run->value) )
	{
		cmd->forwardmove *= 2;
		cmd->sidemove *= 2;
		cmd->upmove *= 2;
	}	
}

void CL_ClampPitch (void)
{
	float	pitch;

	pitch = SHORT2ANGLE(cl.frame.playerstate.pmove.delta_angles[PITCH]);
	if (pitch > 180)
		pitch -= 360;

	if (cl.aimangles[PITCH] + pitch < -360)
		cl.aimangles[PITCH] += 360; // wrapped
	if (cl.aimangles[PITCH] + pitch > 360)
		cl.aimangles[PITCH] -= 360; // wrapped

	if (cl.aimangles[PITCH] + pitch > 89)
		cl.aimangles[PITCH] = 89 - pitch;
	if (cl.aimangles[PITCH] + pitch < -89)
		cl.aimangles[PITCH] = -89 - pitch;

	if (cl.bodyangles[PITCH] + pitch < -360)
		cl.bodyangles[PITCH] += 360; // wrapped
	if (cl.bodyangles[PITCH] + pitch > 360)
		cl.bodyangles[PITCH] -= 360; // wrapped

	if (cl.bodyangles[PITCH] + pitch > 89)
		cl.bodyangles[PITCH] = 89 - pitch;
	if (cl.bodyangles[PITCH] + pitch < -89)
		cl.bodyangles[PITCH] = -89 - pitch;
}

/*
==============
CL_FinishMove
==============
*/
void CL_FinishMove (usercmd_t *cmd)
{
	int32_t		ms;
	int32_t		i;
//
// figure button bits
//	
	if ( in_attack.state & 3 )
		cmd->buttons |= BUTTON_ATTACK;
	in_attack.state &= ~2;
	
	// Knightmare added
	if ( in_attack2.state & 3 )
		cmd->buttons |= BUTTON_ATTACK2;
	in_attack2.state &= ~2;
	// end Knightamre

	if (in_use.state & 3)
		cmd->buttons |= BUTTON_USE;
	in_use.state &= ~2;

	if (anykeydown && cls.key_dest == key_game)
		cmd->buttons |= BUTTON_ANY;

	// send milliseconds of time to apply the move
	ms = cls.frametime * 1000;
	if (ms > 250)
		ms = 100;		// time was unreasonable
	cmd->msec = ms;

	CL_ClampPitch ();
	
	for (i=0 ; i<3 ; i++)
		cmd->angles[i] = ANGLE2SHORT(cl.aimangles[i]);

	cmd->impulse = in_impulse;
	in_impulse = 0;

// send the ambient light level at the player's current position
	cmd->lightlevel = (byte)cl_lightlevel->value;
}

void VR_Move (usercmd_t *cmd)
{
	vec3_t predAngles, predDelta, view;
	vec3_t orientation, orientationDelta;
	qboolean viewmove;
	int32_t i;
	int32_t mode = (int32_t) vr_aimmode->value;

	if (mode == 6)
		viewmove = true;
	else if (mode == 7)
		viewmove = false;
	else
		viewmove = (qboolean) vr_viewmove->value;

	VR_GetOrientation(orientation);
	VR_GetOrientationDelta(orientationDelta);

	// dghost - UGLY HACK
	// todo: fix this shit
	// 10/02/2013: In making this behave properly I have managed to make it even worse.

	for (i = 0; i < 3; i++)
	{
		predDelta[i] =  SHORT2ANGLE(cl.frame.playerstate.pmove.delta_angles[i]);
	}

	VectorAdd(cl.bodyangles, predDelta, predAngles);
	VectorAdd(cl.bodyangles,cl.viewangles,view);
	view[YAW] += predDelta[YAW];

	switch (mode)
	{
	case VR_AIMMODE_DISABLE:
		VectorAdd(cl.in_delta,predAngles,predAngles);
		VectorCopy(predAngles,view);
		break;
	case VR_AIMMODE_HEAD_MYAW_MPITCH:
		VectorAdd(cl.in_delta, predAngles, predAngles);
		view[PITCH] = predAngles[PITCH] += orientationDelta[PITCH];
		predAngles[YAW] = view[YAW] = predAngles[YAW] + orientationDelta[YAW];
		view[ROLL] = orientation[ROLL];
		break;
	case VR_AIMMODE_MOUSE_MYAW_MPITCH:
		VectorAdd(cl.in_delta,predAngles,predAngles);
		view[ROLL] = orientation[ROLL];
		view[PITCH] = predAngles[PITCH] + orientation[PITCH];
		view[YAW]   = predAngles[YAW] + orientation[YAW];
		break;

	default:
	case VR_AIMMODE_HEAD_MYAW:
	case VR_AIMMODE_MOUSE_MYAW:
	case VR_AIMMODE_TF2_MODE2:
	case VR_AIMMODE_TF2_MODE3:
	case VR_AIMMODE_TF2_MODE4:

		{
			float pitch;
			qboolean relativeView, relativeAimPitch,restrictDirection;
			float deadzoneYaw = vr_aimmode_deadzone_yaw->value / 2.0f;
			float deadzonePitch = vr_aimmode_deadzone_pitch->value / 2.0f;
			relativeAimPitch = true;
			restrictDirection = true;
			switch (mode)
			{
			case VR_AIMMODE_HEAD_MYAW:
				deadzoneYaw = 0;
				deadzonePitch = 0;
			case VR_AIMMODE_TF2_MODE2:
				relativeView = false;
				break;
			case VR_AIMMODE_MOUSE_MYAW:
				deadzoneYaw = 0;
				deadzonePitch = 180;
				restrictDirection = false;
				relativeAimPitch = false;
			default:
			case VR_AIMMODE_TF2_MODE3:
			case VR_AIMMODE_TF2_MODE4:
				relativeView = true;
				break;
			}

			if (relativeView)
			{
				float yawDelta = (view[YAW] + orientationDelta[YAW]) - (predAngles[YAW] + cl.aimdelta[YAW]);
				float yawOffset = 0;

				AngleClamp(&yawDelta);
				view[PITCH] = orientation[PITCH];
				view[ROLL] = orientation[ROLL];
				if (cl.in_delta[YAW] < 0)
				{
					if (yawDelta >= deadzoneYaw)
					{
						predAngles[YAW] += cl.in_delta[YAW];
						yawOffset = cl.in_delta[YAW];
					} else {
						cl.aimdelta[YAW] += cl.in_delta[YAW];
					}


				} else if (cl.in_delta[YAW] > 0) {
					if (yawDelta <= -deadzoneYaw)
					{
						predAngles[YAW] += cl.in_delta[YAW];
						yawOffset = cl.in_delta[YAW];
					} else {
						cl.aimdelta[YAW] += cl.in_delta[YAW];
					}

				} else if (restrictDirection)
				{
					if (fabs(yawDelta) >= deadzoneYaw)
					{
						predAngles[YAW] += cl.in_delta[YAW];
						yawOffset = cl.in_delta[YAW];
					} else {
						cl.aimdelta[YAW] += cl.in_delta[YAW];
					}
				}


				if (cl.aimdelta[YAW] > deadzoneYaw)
				{
					float delta = cl.aimdelta[YAW] - deadzoneYaw;
					//				yawOffset -= delta;
					predAngles[YAW] += delta;
					cl.aimdelta[YAW] -= delta;
				} else if (cl.aimdelta[YAW] < -deadzoneYaw)
				{
					float delta = cl.aimdelta[YAW] + deadzoneYaw;
					//				yawOffset -= delta;
					predAngles[YAW] += delta;
					cl.aimdelta[YAW] -= delta;

				}


				view[YAW] += yawOffset + orientationDelta[YAW];

			} else {
				float diffYaw;

				predAngles[YAW] += cl.in_delta[YAW];
				view[YAW] += orientationDelta[YAW];
				view[PITCH] = orientation[PITCH];
				view[ROLL] = orientation[ROLL];


				diffYaw = view[YAW] - predAngles[YAW];

				if (abs(diffYaw) > deadzoneYaw)
				{
					predAngles[YAW] += orientationDelta[YAW];
					view[YAW] += cl.in_delta[YAW];
					diffYaw = view[YAW] - predAngles[YAW];

					if (abs(diffYaw) > deadzoneYaw)
					{
						if (diffYaw > 0.0)
							predAngles[YAW] = view[YAW] - deadzoneYaw;
						else
							predAngles[YAW] = view[YAW] + deadzoneYaw;
					}
				}

			}

			if (relativeAimPitch)
			{
				predAngles[PITCH] = view[PITCH];				
				pitch = cl.aimdelta[PITCH] + cl.in_delta[PITCH] - orientationDelta[PITCH];

			}
			else 
			{
				predAngles[PITCH] = -SHORT2ANGLE(cl.frame.playerstate.pmove.delta_angles[PITCH]);
				pitch = cl.aimdelta[PITCH] + cl.in_delta[PITCH];
			}
			AngleClamp(&pitch);
			if (fabs(pitch) <= deadzonePitch)
				cl.aimdelta[PITCH] = pitch;
			AngleClamp(&cl.aimdelta[PITCH]);
		}
		break;
	case VR_AIMMODE_DECOUPLED:
		VectorAdd(predAngles,cl.in_delta,predAngles);
		VectorCopy(orientation, view);
		view[YAW] += predDelta[YAW];
		break;
	}

	if (cl.aimdelta[PITCH] + predDelta[PITCH]  > 89)
		cl.aimdelta[PITCH] = 89 - predDelta[PITCH] ;
	if (cl.aimdelta[PITCH] + predDelta[PITCH]  < -89)
		cl.aimdelta[PITCH] = -89 - predDelta[PITCH] ;

	VectorSubtract(predAngles, predDelta, cl.bodyangles);
	VectorSubtract(view,cl.bodyangles,cl.viewangles);
	cl.viewangles[YAW] -= predDelta[YAW];
	VectorClamp(cl.viewangles);
	VectorClamp(cl.bodyangles);
	VectorAdd(cl.bodyangles,cl.aimdelta,cl.aimangles);

//	VectorClamp(cl.aimangles);
	if (viewmove)
	{
		float forward, sideways;
		vec3_t diff;
		VectorAdd(predAngles, cl.aimdelta, predAngles);
		VectorSubtract(predAngles, view, diff);
		forward = cmd->forwardmove * cosf(DEG2RAD(diff[YAW])) - cmd->sidemove * sinf(DEG2RAD(diff[YAW]));
		sideways = cmd->sidemove * cosf(DEG2RAD(diff[YAW])) + cmd->forwardmove * sinf(DEG2RAD(diff[YAW]));

		cmd->forwardmove = forward;
		cmd->sidemove = sideways;
	}

}

/*
=================
CL_CreateCmd
=================
*/
usercmd_t CL_CreateCmd (void)
{
	usercmd_t	cmd;

	frame_msec = sys_frame_time - old_sys_frame_time;
	if (frame_msec < 1)
		frame_msec = 1;
	if (frame_msec > 200)
		frame_msec = 200;
	
	VectorSet(cl.in_delta, 0, 0, 0);
	// get basic movement from keyboard
	CL_BaseMove (&cmd);

	// allow mice or other external controllers to add to the move
	IN_Move (&cmd);

	if (cl_paused->value)
		VectorSet(cl.in_delta,0,0,0);

	if (!vr_enabled->value)
	{
		VectorAdd(cl.in_delta, cl.bodyangles, cl.bodyangles);
		VectorCopy(cl.bodyangles, cl.viewangles);
		VectorCopy(cl.bodyangles, cl.aimangles);
	}
	else {
		VR_Move(&cmd);
	}
//	Com_Printf("%f %f %f\n",cl.aimangles[0],cl.aimangles[1],cl.aimangles[2]);
	CL_FinishMove (&cmd);

	old_sys_frame_time = sys_frame_time;

//cmd.impulse = cls.framecount;

	return cmd;
}


void IN_CenterView (void)
{
	cl.bodyangles[PITCH] = -SHORT2ANGLE(cl.frame.playerstate.pmove.delta_angles[PITCH]);
}

/*
============
CL_InitInput
============
*/
void CL_InitInput (void)
{
	Cmd_AddCommand ("centerview",IN_CenterView);

	Cmd_AddCommand ("+moveup",IN_UpDown);
	Cmd_AddCommand ("-moveup",IN_UpUp);
	Cmd_AddCommand ("+movedown",IN_DownDown);
	Cmd_AddCommand ("-movedown",IN_DownUp);
	Cmd_AddCommand ("+left",IN_LeftDown);
	Cmd_AddCommand ("-left",IN_LeftUp);
	Cmd_AddCommand ("+right",IN_RightDown);
	Cmd_AddCommand ("-right",IN_RightUp);
	Cmd_AddCommand ("+forward",IN_ForwardDown);
	Cmd_AddCommand ("-forward",IN_ForwardUp);
	Cmd_AddCommand ("+back",IN_BackDown);
	Cmd_AddCommand ("-back",IN_BackUp);
	Cmd_AddCommand ("+lookup", IN_LookupDown);
	Cmd_AddCommand ("-lookup", IN_LookupUp);
	Cmd_AddCommand ("+lookdown", IN_LookdownDown);
	Cmd_AddCommand ("-lookdown", IN_LookdownUp);
	Cmd_AddCommand ("+strafe", IN_StrafeDown);
	Cmd_AddCommand ("-strafe", IN_StrafeUp);
	Cmd_AddCommand ("+moveleft", IN_MoveleftDown);
	Cmd_AddCommand ("-moveleft", IN_MoveleftUp);
	Cmd_AddCommand ("+moveright", IN_MoverightDown);
	Cmd_AddCommand ("-moveright", IN_MoverightUp);
	Cmd_AddCommand ("+speed", IN_SpeedDown);
	Cmd_AddCommand ("-speed", IN_SpeedUp);
	Cmd_AddCommand ("+attack", IN_AttackDown);
	Cmd_AddCommand ("-attack", IN_AttackUp);
	
	// Knightmare added
	Cmd_AddCommand ("+attack2", IN_Attack2Down);
	Cmd_AddCommand ("-attack2", IN_Attack2Up);
	// end Knightmare

	Cmd_AddCommand ("+use", IN_UseDown);
	Cmd_AddCommand ("-use", IN_UseUp);
	Cmd_AddCommand ("impulse", IN_Impulse);
	Cmd_AddCommand ("+klook", IN_KLookDown);
	Cmd_AddCommand ("-klook", IN_KLookUp);

	cl_nodelta = Cvar_Get ("cl_nodelta", "0", 0);
}



/*
=================
CL_SendCmd
=================
*/
void CL_SendCmd (void)
{
	sizebuf_t	buf;
	byte		data[128];
	int32_t			i;
	usercmd_t	*cmd, *oldcmd;
	usercmd_t	nullcmd;
	int32_t			checksumIndex;

	// clear buffer
	memset (&buf, 0, sizeof(buf));

	// build a command even if not connected

	// save this command off for prediction
	i = cls.netchan.outgoing_sequence & (CMD_BACKUP-1);
	cmd = &cl.cmds[i];
	cl.cmd_time[i] = cls.realtime;	// for netgraph ping calculation

	*cmd = CL_CreateCmd ();

	cl.cmd = *cmd;

	if (cls.state == ca_disconnected || cls.state == ca_connecting)
		return;

	if (cls.state == ca_connected)
	{
		if (cls.netchan.message.cursize	|| curtime - cls.netchan.last_sent > 1000 )
			Netchan_Transmit (&cls.netchan, 0, buf.data);	
		return;
	}

	// send a userinfo update if needed
	if (userinfo_modified)
	{
		CL_FixUpGender();
		userinfo_modified = false;
		MSG_WriteByte (&cls.netchan.message, clc_userinfo);
		MSG_WriteString (&cls.netchan.message, Cvar_Userinfo() );
	}

	SZ_Init (&buf, data, sizeof(data));

	// Knightmare- removed this, put ESC-only substitute in keys.c
	/*if (cmd->buttons && cl.cinematictime > 0 && !cl.attractloop 
		&& cls.realtime - cl.cinematictime > 1000)
	{	// skip the rest of the cinematic
		SCR_FinishCinematic ();
	}*/

	// begin a client move command
	MSG_WriteByte (&buf, clc_move);

	// save the position for a checksum byte
	checksumIndex = buf.cursize;
	MSG_WriteByte (&buf, 0);

	// let the server know what the last frame we
	// got was, so the next message can be delta compressed
	if (cl_nodelta->value || !cl.frame.valid || cls.demowaiting)
		MSG_WriteLong (&buf, -1);	// no compression
	else
		MSG_WriteLong (&buf, cl.frame.serverframe);

	// send this and the previous cmds in the message, so
	// if the last packet was dropped, it can be recovered
	i = (cls.netchan.outgoing_sequence-2) & (CMD_BACKUP-1);
	cmd = &cl.cmds[i];
	memset (&nullcmd, 0, sizeof(nullcmd));
	MSG_WriteDeltaUsercmd (&buf, &nullcmd, cmd);
	oldcmd = cmd;

	i = (cls.netchan.outgoing_sequence-1) & (CMD_BACKUP-1);
	cmd = &cl.cmds[i];
	MSG_WriteDeltaUsercmd (&buf, oldcmd, cmd);
	oldcmd = cmd;

	i = (cls.netchan.outgoing_sequence) & (CMD_BACKUP-1);
	cmd = &cl.cmds[i];
	MSG_WriteDeltaUsercmd (&buf, oldcmd, cmd);

	// calculate a checksum over the move commands
	buf.data[checksumIndex] = COM_BlockSequenceCRCByte(
		buf.data + checksumIndex + 1, buf.cursize - checksumIndex - 1,
		cls.netchan.outgoing_sequence);

	//
	// deliver the message
	//
	Netchan_Transmit (&cls.netchan, buf.cursize, buf.data);	
}



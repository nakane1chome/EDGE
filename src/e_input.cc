//----------------------------------------------------------------------------
//  EDGE2 Input handling
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2009  The EDGE2 Team.
// 
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//----------------------------------------------------------------------------
//
//  Based on the DOOM source code, released by Id Software under the
//  following copyright:
//
//    Copyright (C) 1993-1996 by id Software, Inc.
//
//----------------------------------------------------------------------------
//
// -MH- 1998/07/02 Added key_flyup and key_flydown variables (no logic yet)
// -MH- 1998/08/18 Flyup and flydown logic
//

#include "i_defs.h"
#include "i_defs_gl.h" //glFog logic
//#include "../src/r_units.h"
#include "../src/r_gldefs.h"

#include "dm_defs.h"
#include "dm_state.h"
#include "e_player.h"
#include "e_event.h"
#include "e_input.h"
#include "e_main.h"
#include "hu_stuff.h"
#include "m_math.h"
#include "m_misc.h"
#include "r_misc.h"
#include "z_zone.h"



extern bool CON_Responder(event_t *ev);
extern bool   M_Responder(event_t *ev);
extern bool   G_Responder(event_t *ev);

extern int I_JoyGetAxis(int n);


//
// EVENT HANDLING
//
// Events are asynchronous inputs generally generated by the game user.
// Events can be discarded if no responder claims them
//
#define MAXEVENTS  128

static event_t events[MAXEVENTS];
static int eventhead;
static int eventtail;

//
// controls (have defaults) 
// 
int key_right;
int key_left;
int key_lookup;
int key_lookdown;
int key_lookcenter;

// -ES- 1999/03/28 Zoom Key
int key_zoom;

// -CA- 2015/5/29 Fog Key (will replace with DDF commands later)
int key_fog; //bool   gp

int key_up;
int key_down;
int key_strafeleft;
int key_straferight;
int key_fire;
int key_use;
int key_strafe;
int key_speed;
int key_autorun;
int key_nextweapon;
int key_prevweapon;
int key_map;
int key_180;
int key_talk;
int key_console;
int key_mlook;
int key_secondatk;
int key_reload;
int key_action1;
int key_action2;
int key_action3;
int key_action4;


// -MH- 1998/07/10 Flying keys
int key_flyup;
int key_flydown;

int key_weapons[10];

#define MAXPLMOVE  (forwardmove[1])

static int forwardmove[2] = {25, 50};
static int sidemove[2]    = {24, 40};
static int upwardmove[2]  = {20, 30};

static int angleturn[3] = {640, 1280, 320};  // + slow turn 
static int mlookturn[3] = {400,  800, 200};

#define SLOWTURNTICS    6

#define NUMKEYS         512

#define GK_DOWN  0x01
#define GK_UP    0x02

static byte gamekeydown[NUMKEYS];

static int turnheld;   // for accelerative turning 
static int mlookheld;  // for accelerative mlooking 

//-------------------------------------------
// -KM-  1998/09/01 Analogue binding
// -ACB- 1998/09/06 Two-stage turning switch
//
int mouse_xaxis;
int mouse_yaxis;

int mouse_xsens;
int mouse_ysens;

int joy_axis[6] = { 0, 0, 0, 0, 0, 0 };

static int joy_last_raw[6];

static int mouse_ss_hack = 0;

// The last one is ignored (AXIS_DISABLE)
static float ball_deltas[6] = {0, 0, 0, 0, 0, 0};
static float  joy_forces[6] = {0, 0, 0, 0, 0, 0};

cvar_c joy_dead;
cvar_c joy_peak;
cvar_c joy_tuning;

cvar_c in_running;
cvar_c in_stageturn;
cvar_c mouse_filter;

cvar_c debug_mouse;
cvar_c debug_joyaxis;

// Speed controls
int var_turnspeed;
int var_mlookspeed;
int var_forwardspeed;
int var_sidespeed;
int var_flyspeed;


static float sensitivities[16] =
{
	0.10, 0.25, 0.35, 0.50,
	0.75, 1.00, 1.56, 2.21,
	3.13, 4.42, 6.26, 8.84,
	12.5, 17.7, 25.0, 35.4
};

static float speed_factors[12] =
{
	0.15, 0.25, 0.33, 0.42,
	0.50, 0.66, 0.83, 1.00,
	1.50, 2.00, 2.80, 4.00
};


float JoyAxisFromRaw(int raw)
{
	SYS_ASSERT(abs(raw) <= 32768);

	float v = raw / 32768.0f;
	
	if (fabs(v) <= joy_dead.f + 0.01)
		return 0;

	if (fabs(v) >= joy_peak.f - 0.01)
		return (v < 0) ? -1.0f : +1.0f;

	SYS_ASSERT(joy_peak.f > joy_dead.f);

	float t = CLAMP(0.2f, joy_tuning.f, 5.0f);

	if (v >= 0)
	{
		v = (v - joy_dead.f) / (joy_peak.f - joy_dead.f);
		return pow(v, 1.0f / t);
	}
	else
	{
		v = (-v - joy_dead.f) / (joy_peak.f - joy_dead.f);
		return - pow(v, 1.0f / t);
	}
}

static void UpdateJoyAxis(int n)
{
	if (joy_axis[n] == AXIS_DISABLE)
		return;

	int raw = I_JoyGetAxis(n);
	int old = joy_last_raw[n];

	joy_last_raw[n] = raw;

	// cooked value = average of last two raw samples
	int cooked = (raw + old) >> 1;

	float force = JoyAxisFromRaw(cooked);

	// perform inversion
	if ((joy_axis[n]+1) & 1)
		force = -force;

	if (debug_joyaxis.d == n+1)
	{
		I_Printf("Axis%d : raw %+05d --> %+7.3f\n", n+1, raw, force);
	}

	int axis = (joy_axis[n]+1) >> 1;

	joy_forces[axis] += force;
}


bool E_MatchesKey(int keyvar, int key)
{
	return ((keyvar >> 16) == key) ||
	       ((keyvar & 0xffff) == key);
}

bool E_IsKeyPressed(int keyvar)
{
#ifdef DEVELOPERS
	if ((keyvar >> 16) > NUMKEYS)
		I_Error("Invalid key!");
	else if ((keyvar & 0xffff) > NUMKEYS)
		I_Error("Invalid key!");
#endif

	if (gamekeydown[keyvar >> 16] & GK_DOWN)
		return true;

	if (gamekeydown[keyvar & 0xffff] & GK_DOWN)
		return true;

	return false;
}

static inline void AddKeyForce(int axis, int upkeys, int downkeys, float qty = 1.0f)
{
	//let movement keys cancel each other out
	if (E_IsKeyPressed(upkeys))
	{
		joy_forces[axis] += qty;
	}
	if (E_IsKeyPressed(downkeys))
	{
		joy_forces[axis] -= qty;
	}
}

static void UpdateForces(void)
{
	for (int k = 0; k < 6; k++)
		joy_forces[k] = 0;

	// ---Keyboard---

	AddKeyForce(AXIS_TURN,    key_right,  key_left);
	AddKeyForce(AXIS_MLOOK,   key_lookup, key_lookdown);
	AddKeyForce(AXIS_FORWARD, key_up,     key_down);
	// -MH- 1998/08/18 Fly down
	AddKeyForce(AXIS_FLY,     key_flyup,  key_flydown);
	AddKeyForce(AXIS_STRAFE,  key_straferight, key_strafeleft);

	// ---Joystick---

	for (int j = 0; j < 6; j++)
		UpdateJoyAxis(j);
}

#if 0  // UNUSED ???
static int CmdChecksum(ticcmd_t * cmd)
{
	int i;
	int sum = 0;

	for (i = 0; i < (int)sizeof(ticcmd_t) / 4 - 1; i++)
		sum += ((int *)cmd)[i];

	return sum;
}
#endif

void E_BuildTiccmd_Other(ticcmd_t * cmd)
{
	///
	/// -AJA- very hacky stuff here to test out split-screen mode
	///

	Z_Clear(cmd, ticcmd_t, 1);

	//-- Turning --
	{
		float turn = angleturn[0] * ball_deltas[AXIS_TURN] / 64.0;

		cmd->angleturn = I_ROUND(turn);
	}

	//-- Mlook --
	{
		float mlook = mlookturn[0] * ball_deltas[AXIS_MLOOK] / 64.0;

		cmd->mlookturn = I_ROUND(mlook);
	}

	//-- Forward --
	{
		if (mouse_ss_hack & 0x6)
			cmd->forwardmove = forwardmove[1];
	}

	//-- Buttons --
	if (mouse_ss_hack & 1)
		cmd->buttons |= BT_ATTACK;

	if (mouse_ss_hack & 0x6)
		cmd->buttons |= BT_USE;

	for (int k = 0; k < 6; k++)
		ball_deltas[k] = 0;
}

//
// E_BuildTiccmd
//
// Builds a ticcmd from all of the available inputs
//
// -ACB- 1998/07/02 Added Vertical angle checking for mlook.
// -ACB- 1998/07/10 Reformatted: I can read the code! :)
// -ACB- 1998/09/06 Apply speed controls to -KM-'s analogue controls
//
static bool allow180 = true;
static bool allowzoom = true;
static bool allowautorun = true;

void E_BuildTiccmd(ticcmd_t * cmd)
{
	UpdateForces();

	if (splitscreen_mode && cmd->player_idx == consoleplayer1)
	{
		E_BuildTiccmd_Other(cmd);
		return;
	}

	Z_Clear(cmd, ticcmd_t, 1);

	bool strafe = E_IsKeyPressed(key_strafe);
	int  speed  = E_IsKeyPressed(key_speed) ? 1 : 0;
	

	if (in_running.d)
		speed = !speed;

	//
	// -KM- 1998/09/01 use two stage accelerative turning on all devices
	//
	// -ACB- 1998/09/06 Allow stage turning to be switched off for
	//                  analogue devices...
	//
	int t_speed = speed;

	if (fabs(joy_forces[AXIS_TURN]) > 0.2f)
		turnheld++;
	else
		turnheld = 0;

	// slow turn ?
	if (turnheld < SLOWTURNTICS && in_stageturn.d)
		t_speed = 2;

	int m_speed = speed;

	if (fabs(joy_forces[AXIS_MLOOK]) > 0.2f)
		mlookheld++;
	else
		mlookheld = 0;

	// slow mlook ?
	if (mlookheld < SLOWTURNTICS && in_stageturn.d)
		m_speed = 2;


	// Turning
	if (! strafe)
	{
		float turn = angleturn[t_speed] * joy_forces[AXIS_TURN];
		
		turn *= speed_factors[var_turnspeed];

		// -ACB- 1998/09/06 Angle Turn Speed Control
		turn += angleturn[t_speed] * ball_deltas[AXIS_TURN] / 64.0;

		cmd->angleturn = I_ROUND(turn);
	}

	// MLook
	{
		// -ACB- 1998/07/02 Use VertAngle for Look/up down.
		float mlook = mlookturn[m_speed] * joy_forces[AXIS_MLOOK];

		mlook *= speed_factors[var_mlookspeed];

		mlook += mlookturn[m_speed] * ball_deltas[AXIS_MLOOK] / 64.0;

		cmd->mlookturn = I_ROUND(mlook);
	}

	// Forward
	{
		float forward = forwardmove[speed] * joy_forces[AXIS_FORWARD];

		forward *= speed_factors[var_forwardspeed];

		// -ACB- 1998/09/06 Forward Move Speed Control
		forward += forwardmove[speed] * ball_deltas[AXIS_FORWARD] / 64.0;

		forward = CLAMP(-MAXPLMOVE, forward, MAXPLMOVE);

		cmd->forwardmove = I_ROUND(forward);
	}

	// Sideways
	{
		float side = sidemove[speed] * joy_forces[AXIS_STRAFE];

		if (strafe)
			side += sidemove[speed] * joy_forces[AXIS_TURN];

		side *= speed_factors[var_sidespeed];

		// -ACB- 1998/09/06 Side Move Speed Control
		side += sidemove[speed] * ball_deltas[AXIS_STRAFE] / 64.0;

		if (strafe)
			side += sidemove[speed] * ball_deltas[AXIS_TURN] / 64.0;

		side = CLAMP(-MAXPLMOVE, side, MAXPLMOVE);

		cmd->sidemove = I_ROUND(side);
	}

	// Upwards  -MH- 1998/08/18 Fly Up/Down movement
	{
		float upward = upwardmove[speed] * joy_forces[AXIS_FLY];

		upward *= speed_factors[var_flyspeed];

		upward += upwardmove[speed] * ball_deltas[AXIS_FLY] / 64.0;

		upward = CLAMP(-MAXPLMOVE, upward, MAXPLMOVE);

		cmd->upwardmove = I_ROUND(upward);
	}


	// ---Buttons---

	if (E_IsKeyPressed(key_fire))
		cmd->buttons |= BT_ATTACK;

	if (E_IsKeyPressed(key_use))
		cmd->buttons |= BT_USE;

	if (E_IsKeyPressed(key_secondatk))
		cmd->extbuttons |= EBT_SECONDATK;

	if (E_IsKeyPressed(key_reload))
		cmd->extbuttons |= EBT_RELOAD;

	if (E_IsKeyPressed(key_action1))
		cmd->extbuttons |= EBT_ACTION1;

	if (E_IsKeyPressed(key_action2))
		cmd->extbuttons |= EBT_ACTION2;
		
	if (E_IsKeyPressed(key_action3))
		cmd->extbuttons |= EBT_ACTION3;

	if (E_IsKeyPressed(key_action4))
		cmd->extbuttons |= EBT_ACTION4;

	// -ACB- 1998/07/02 Use CENTER flag to center the vertical look.
	if (E_IsKeyPressed(key_lookcenter))
		cmd->extbuttons |= EBT_CENTER;

	// -KM- 1998/11/25 Weapon change key
	for (int w = 0; w < 10; w++)
	{
		if (E_IsKeyPressed(key_weapons[w]))
		{
			cmd->buttons |= BT_CHANGE;
			cmd->buttons |= w << BT_WEAPONSHIFT;
			break;
		}
	}

	if (E_IsKeyPressed(key_nextweapon))
	{
		cmd->buttons |= BT_CHANGE;
		cmd->buttons |= (BT_NEXT_WEAPON << BT_WEAPONSHIFT);
	}
	else if (E_IsKeyPressed(key_prevweapon))
	{
		cmd->buttons |= BT_CHANGE;
		cmd->buttons |= (BT_PREV_WEAPON << BT_WEAPONSHIFT);
	}

	// You have to release the 180 deg turn key before you can press it again
	if (E_IsKeyPressed(key_180))
	{
		if (allow180)
			cmd->angleturn ^= (s16_t)0x8000;

		allow180 = false;
	}
	else
		allow180 = true;

	// -ES- 1999/03/28 Zoom Key
	if (E_IsKeyPressed(key_zoom))
	{
		if (allowzoom)
		{
			cmd->extbuttons |= EBT_ZOOM;
			allowzoom = false;
		}
	}
	else
		allowzoom = true;
	
//	if (E_IsKeyPressed(key_fog) && !gp)
//	{
	//	bool gp = E_IsKeyPressed(key_fog);
      //  gp=TRUE;                    // gp Is Set To TRUE
		//	fogfilter+=1;                   // Increase fogfilter By One
	//	if (fogfilter>2)             // Is fogfilter Greater Than 2?
	//	{
     //   fogfilter=0;                // If So, Set fogfilter To Zero
	//	}
    //glFogi (GL_FOG_MODE, fogMode[fogfilter]);   // Fog Mode
	//}
		//if (!keys['G'])                     // Has The G Key Been Released?
	//	if (!E_IsKeyPressed(key_fog))
	//	{
	//	gp=FALSE;                   // If So, gp Is Set To FALSE
		//}

	// -AJA- 2000/04/14: Autorun toggle
	if (E_IsKeyPressed(key_autorun))
	{
		if (allowautorun)
		{
			in_running = in_running.d ? 0 : 1;
			allowautorun = false;
		}
	}
	else
		allowautorun = true;


	cmd->chatchar = HU_DequeueChatChar();

	for (int k = 0; k < 6; k++)
		ball_deltas[k] = 0;
}

//
// Get info needed to make ticcmd_ts for the players.
// 
bool INP_Responder(event_t * ev)
{
	int sym = ev->data1;

	switch (ev->type)
	{
		case ev_keydown:
		case ev_mousedown:
			if (splitscreen_mode && sym >= KEYD_MOUSE1 && sym <= KEYD_MOUSE6)
			{
				mouse_ss_hack |= (1 << (sym - KEYD_MOUSE1));
				return true;
			}

			if (ev->data1 < NUMKEYS)
			{
				gamekeydown[ev->data1] &= ~GK_UP;
				gamekeydown[ev->data1] |=  GK_DOWN;
			}

			// eat key down events 
			return true;

		case ev_keyup:
		case ev_mouseup:
			if (splitscreen_mode && sym >= KEYD_MOUSE1 && sym <= KEYD_MOUSE6)
			{
				mouse_ss_hack &= ~(1 << (sym - KEYD_MOUSE1));
				return false;
			}

			if (ev->data1 < NUMKEYS)
			{
				gamekeydown[ev->data1] |= GK_UP;
			}

			// always let key up events filter down 
			return false;

		case ev_mouse:
		{
			float dx = ev->data2;
			float dy = ev->data3;

			// perform inversion
			if ((mouse_xaxis+1) & 1) dx = -dx;
			if ((mouse_yaxis+1) & 1) dy = -dy;

			dx *= sensitivities[mouse_xsens];
			dy *= sensitivities[mouse_ysens];

			if (debug_mouse.d)
				I_Printf("Mouse %+04d %+04d --> %+7.2f %+7.2f\n",
				         ev->data2, ev->data3, dx, dy);

			// -AJA- 1999/07/27: Mlook key like quake's.
			if (E_IsKeyPressed(key_mlook))
			{
				ball_deltas[AXIS_TURN]  += dx;
				ball_deltas[AXIS_MLOOK] += dy;
			}
			else
			{
				ball_deltas[(mouse_xaxis+1) >> 1] += dx;
				ball_deltas[(mouse_yaxis+1) >> 1] += dy;
			}

			return true;  // eat events
		}

		default:
			break;
	}

	return false;
}

//
// Sets the turbo scale (100 is normal)
//
void E_SetTurboScale(int scale)
{
	forwardmove[0] = 25 * scale / 100;
	forwardmove[1] = 50 * scale / 100;

	sidemove[0]    = 24 * scale / 100;
	sidemove[1]    = 40 * scale / 100;
}


void E_ClearInput(void)
{
	Z_Clear(gamekeydown, byte, NUMKEYS);

	turnheld  = 0;
	mlookheld = 0;

	mouse_ss_hack = 0;
}

//
// Finds all keys in the gamekeydown[] array which have been released
// and clears them.  The value is NOT cleared by INP_Responder() since
// that prevents very fast presses (also the mousewheel) from being
// down long enough to be noticed by E_BuildTiccmd().
//
// -AJA- 2005/02/17: added this.
//
void E_UpdateKeyState(void)
{
	for (int k = 0; k < NUMKEYS; k++)
		if (gamekeydown[k] & GK_UP)
			gamekeydown[k] = 0;
}

//
// Generate events which should release all current keys.
//
void E_ReleaseAllKeys(void)
{
	int i;
	for (i = 0; i < NUMKEYS; i++)
	{
		if (gamekeydown[i] & GK_DOWN)
		{
			event_t ev;
			
			ev.type = ev_keyup;
			ev.data1 = i;
			///ev.value.key.unicode = 0;

			E_PostEvent(&ev);
		}
	}
}

//
// Called by the I/O functions when input is detected
//
void E_PostEvent(event_t * ev)
{
	events[eventhead] = *ev;
	eventhead = ++eventhead % MAXEVENTS;

#ifdef DEBUG_KEY_EV  //!!!!
if (ev->type == ev_keydown || ev->type == ev_keyup)
{
	L_WriteDebug("EVENT @ %08x %d %s\n",
		I_ReadMicroSeconds()/1000,
		ev->data1,
		(ev->type == ev_keyup) ? "keyup" : "keydown");
}
else if (ev->type == ev_mousedown || ev->type == ev_mouseup)
{
	L_WriteDebug("EVENT @ %08x %d %s\n",
		I_ReadMicroSeconds()/1000,
		ev->data1,
		(ev->type == ev_mouseup) ? "mouseup" : "mousedown");
}
#endif
}

//
// Send all the events of the given timestamp down the responder chain
//
void E_ProcessEvents(void)
{
	event_t *ev;

	for (; eventtail != eventhead; eventtail = (eventtail + 1) % MAXEVENTS)
	{
		ev = &events[eventtail];

		if (CON_Responder(ev))
			continue;  // Console ate the event

		if (chat_on && HU_Responder(ev))
			continue;  // let chat eat the event first of all

		if (M_Responder(ev))
			continue;  // menu ate the event

		G_Responder(ev);  // let game eat it, nobody else wanted it
	}
}


//----------------------------------------------------------------------------

typedef struct specialkey_s
{
	int key;

	const char *name;
}
specialkey_t;

static specialkey_t special_keys[] =
{
    { KEYD_RIGHTARROW, "Right Arrow" },
    { KEYD_LEFTARROW, "Left Arrow" },
    { KEYD_UPARROW, "Up Arrow" },
    { KEYD_DOWNARROW, "Down Arrow" },
    { KEYD_ESCAPE, "Escape" },
    { KEYD_ENTER, "Enter" },
    { KEYD_TAB, "Tab" },

    { KEYD_BACKSPACE, "Backspace" },
    { KEYD_EQUALS, "Equals" },
    { KEYD_MINUS, "Minus" },
    { KEYD_RSHIFT, "Shift" },
    { KEYD_RCTRL, "Ctrl" },
    { KEYD_RALT, "Alt" },
    { KEYD_INSERT, "Insert" },
    { KEYD_DELETE, "Delete" },
    { KEYD_PGDN, "PageDown" },
    { KEYD_PGUP, "PageUp" },
    { KEYD_HOME, "Home" },
    { KEYD_END, "End" },
    { KEYD_SCRLOCK,  "ScrollLock" },
    { KEYD_NUMLOCK,  "NumLock" },
    { KEYD_CAPSLOCK, "CapsLock" },
    { KEYD_END, "End" },
    { '\'', "\'" },
    { KEYD_SPACE, "Space" },
    { KEYD_TILDE, "Tilde" },
    { KEYD_PAUSE, "Pause" },

	// function keys
    { KEYD_F1,  "F1" },
    { KEYD_F2,  "F2" },
    { KEYD_F3,  "F3" },
    { KEYD_F4,  "F4" },
    { KEYD_F5,  "F5" },
    { KEYD_F6,  "F6" },
    { KEYD_F7,  "F7" },
    { KEYD_F8,  "F8" },
    { KEYD_F9,  "F9" },
    { KEYD_F10, "F10" },
    { KEYD_F11, "F11" },
    { KEYD_F12, "F12" },

	// numeric keypad
	{ KEYD_KP0, "KP_0" },
	{ KEYD_KP1, "KP_1" },
	{ KEYD_KP2, "KP_2" },
	{ KEYD_KP3, "KP_3" },
	{ KEYD_KP4, "KP_4" },
	{ KEYD_KP5, "KP_5" },
	{ KEYD_KP6, "KP_6" },
	{ KEYD_KP7, "KP_7" },
	{ KEYD_KP8, "KP_8" },
	{ KEYD_KP9, "KP_9" },
	
	{ KEYD_KP_DOT,   "KP_DOT" },
	{ KEYD_KP_PLUS,  "KP_PLUS" },
	{ KEYD_KP_MINUS, "KP_MINUS" },
	{ KEYD_KP_STAR,  "KP_STAR" },
	{ KEYD_KP_SLASH, "KP_SLASH" },
	{ KEYD_KP_EQUAL, "KP_EQUAL" },
	{ KEYD_KP_ENTER, "KP_ENTER" },

	// mouse buttons
    { KEYD_MOUSE1, "Mouse1" },
    { KEYD_MOUSE2, "Mouse2" },
    { KEYD_MOUSE3, "Mouse3" },
    { KEYD_MOUSE4, "Mouse4" },
    { KEYD_MOUSE5, "Mouse5" },
    { KEYD_MOUSE6, "Mouse6" },
    { KEYD_WHEEL_UP, "Wheel Up" },
    { KEYD_WHEEL_DN, "Wheel Down" },

	// joystick buttons
    { KEYD_JOY1,  "Joy1" },
    { KEYD_JOY2,  "Joy2" },
    { KEYD_JOY3,  "Joy3" },
    { KEYD_JOY4,  "Joy4" },
    { KEYD_JOY5,  "Joy5" },
    { KEYD_JOY6,  "Joy6" },
    { KEYD_JOY7,  "Joy7" },
    { KEYD_JOY8,  "Joy8" },
    { KEYD_JOY9,  "Joy9" },
    { KEYD_JOY10, "Joy10" },
    { KEYD_JOY11, "Joy11" },
    { KEYD_JOY12, "Joy12" },
    { KEYD_JOY13, "Joy13" },
    { KEYD_JOY14, "Joy14" },
    { KEYD_JOY15, "Joy15" },

	// THE END
    { -1, NULL }
};


const char *E_GetKeyName(int key)
{
	static char buffer[32];

	if (toupper(key) >= ',' && toupper(key) <= ']')
	{
		buffer[0] = key;
		buffer[1] = 0;

		return buffer;
	}

	for (int i = 0 ; special_keys[i].name ; i++)
	{
		if (special_keys[i].key == key)
			return special_keys[i].name;
	}

	sprintf(buffer, "Key%03d", key);

	return buffer;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab

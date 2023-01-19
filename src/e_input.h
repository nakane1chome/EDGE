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

#ifndef __E_INPUT_H__
#define __E_INPUT_H__

#include "e_event.h"
#include "e_ticcmd.h"

void E_ClearInput(void);
void E_BuildTiccmd(ticcmd_t * cmd, int which_player); //after (), add , int which_player
void E_BuildTiccmd_Other(ticcmd_t * cmd);
void E_ReleaseAllKeys(void);
void E_SetTurboScale(int scale);
void E_UpdateKeyState(void);

void E_ProcessEvents(void);
void E_PostEvent(event_t * ev);

bool E_IsKeyPressed(int keyvar);
bool E_MatchesKey(int keyvar, int key);

const char *E_GetKeyName(int key);

bool INP_Responder(event_t * ev);

typedef struct {
  int key_right;
  int key_left;
  int key_lookup;
  int key_lookdown;
  int key_lookcenter;
  int key_zoom;
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
  int key_180;
  int key_talk;
  int key_mlook;
  int key_secondatk;
  int key_reload;
  int key_action1;
  int key_action2;
  int key_action3;
  int key_action4;
  int key_flyup;
  int key_flydown;
  int key_weapons[10];
  int mouse_xaxis;
  int mouse_yaxis;
  int mouse_xsens;
  int mouse_ysens;
  int joy_axis[6];// = { 0, 0, 0, 0, 0, 0 };
} input_options_t;

extern input_options_t input_options;

// -KM- 1998/09/01 Analogue binding stuff, These hold what axis they bind to.
//
// -ACB- 1998/09/06 Analogue binding:
//                   Two stage turning, angleturn control
//                   horzmovement control, vertmovement control
//                   strafemovediv;
//
extern int var_turnspeed;
extern int var_mlookspeed;
extern int var_forwardspeed;
extern int var_sidespeed;
extern int var_flyspeed;


//automap stuff...let's leave this alone for right now
//and save for future work...
extern int key_pause;
extern int key_console;
extern int key_map;
extern int key_am_up;
extern int key_am_down;
extern int key_am_left;
extern int key_am_right;

extern int key_am_zoomin;
extern int key_am_zoomout;

extern int key_am_follow;
extern int key_am_grid;
extern int key_am_mark;
extern int key_am_clear;

#endif  /* __E_INPUT_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab

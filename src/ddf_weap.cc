//----------------------------------------------------------------------------
//  EDGE Data Definition File Code (Weapons)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2004  The EDGE Team.
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
// Player Weapons Setup and Parser Code
//
// -KM- 1998/11/25 File Written
//

#include "i_defs.h"

#include "ddf_locl.h"
#include "ddf_main.h"
#include "dm_state.h"
#include "e_search.h"
#include "m_math.h"
#include "m_fixed.h"
#include "p_action.h"
#include "p_mobj.h"
#include "z_zone.h"

#undef  DF
#define DF  DDF_CMD

static weaponinfo_t buffer_weapon;
static weaponinfo_t *dynamic_weapon;

static const weaponinfo_t template_weapon =
{
  DDF_BASE_NIL,  // ddf

  NULL,        // attack
  AM_NoAmmo,   // ammo;
  0,           // ammopershot;
  1,           // clip;
  false,       // autofire;
  0.0f,        // kick;

  NULL,        // sa_attack;
  AM_NoAmmo,   // sa_ammo;
  0,           // sa_ammopershot;
  1,           // sa_clip;
  false,       // sa_autofire;

  0,    // first_state
  0,    // last_state
  
  0,    // up_state;
  0,    // down_state;
  0,    // ready_state;
  0,    // attack_state;
  0,    // reload_state;
  0,    // flash_state;
  0,    // sa_attack_state;
  0,    // sa_reload_state;
  0,    // sa_flash_state;
  0,    // crosshair;
  0,    // zoom_state;

  false,  // autogive;
  false,  // feedback;
  -1,     // upgraded_weap;
  0,      // priority;
  false,  // dangerous;

  NULL,   // eject_attack;
  NULL,   // idle;
  NULL,   // engaged;
  NULL,   // hit;
  NULL,   // start;
  NULL,   // sound1;
  NULL,   // sound2;
  NULL,   // sound3;

  false,  // nothrust;
  -1,     // bind_key;
  WPSP_None, // special_flags;
  0,      // zoom_fov
  false,  // refire_inacc
  false,  // show_clip
  PERCENT_MAKE(100), // bobbing
  PERCENT_MAKE(100)  // swaying
};

weaponinfo_t ** weaponinfo = NULL;
int numweapons = 0;
int num_disabled_weapons = 0;

static stack_array_t weaponinfo_a;


// -KM- 1998/11/25 Always 10 weapon keys, 1 - 9 and 0
weaponkey_t weaponkey[10];

static void DDF_WGetAmmo(const char *info, void *storage);
static void DDF_WGetUpgrade(const char *info, void *storage);
static void DDF_WGetSpecialFlags(const char *info, void *storage);

#undef  DDF_CMD_BASE
#define DDF_CMD_BASE  buffer_weapon

static const commandlist_t weapon_commands[] =
{
  DF("AMMOTYPE", ammo, DDF_WGetAmmo),
  DF("AMMOPERSHOT", ammopershot, DDF_MainGetNumeric),
  DF("CLIPSIZE", clip, DDF_MainGetNumeric),
  DF("AUTOMATIC", autofire, DDF_MainGetBoolean),
  DF("SEC AMMOTYPE", sa_ammo, DDF_WGetAmmo),
  DF("SEC AMMOPERSHOT", sa_ammopershot, DDF_MainGetNumeric),
  DF("SEC CLIPSIZE", sa_clip, DDF_MainGetNumeric),
  DF("SEC AUTOMATIC", sa_autofire, DDF_MainGetBoolean),
  DF("ATTACK", attack, DDF_MainRefAttack),
  DF("SECOND ATTACK", sa_attack, DDF_MainRefAttack),
  DF("EJECT ATTACK", eject_attack, DDF_MainRefAttack),
  DF("FREE", autogive, DDF_MainGetBoolean),
  DF("BINDKEY", bind_key, DDF_MainGetNumeric),
  DF("PRIORITY", priority, DDF_MainGetNumeric),
  DF("DANGEROUS", dangerous, DDF_MainGetBoolean),
  DF("UPGRADES", upgraded_weap, DDF_WGetUpgrade),
  DF("IDLE SOUND", idle, DDF_MainLookupSound),
  DF("ENGAGED SOUND", engaged, DDF_MainLookupSound),
  DF("HIT SOUND", hit, DDF_MainLookupSound),
  DF("START SOUND", start, DDF_MainLookupSound),
  DF("NOTHRUST", nothrust, DDF_MainGetBoolean),
  DF("FEEDBACK", feedback, DDF_MainGetBoolean),
  DF("KICK", kick, DDF_MainGetFloat),
  DF("SPECIAL", special_flags, DDF_WGetSpecialFlags),
  DF("ZOOM FOV", zoom_fov, DDF_MainGetAngle),
  DF("REFIRE INACCURATE", refire_inacc, DDF_MainGetBoolean),
  DF("SHOW CLIP", show_clip, DDF_MainGetBoolean),
  DF("BOBBING", bobbing, DDF_MainGetPercent),
  DF("SWAYING", swaying, DDF_MainGetPercent),
  
  // -AJA- backwards compatibility cruft...
  DF("!SOUND1", sound1, DDF_MainLookupSound),
  DF("!SOUND2", sound2, DDF_MainLookupSound),
  DF("!SOUND3", sound3, DDF_MainLookupSound),

  DDF_CMD_END
};

static const state_starter_t weapon_starters[] =
{
  {"UP",        "UP",     &buffer_weapon.up_state},
  {"DOWN",      "DOWN",   &buffer_weapon.down_state},
  {"READY",     "READY",  &buffer_weapon.ready_state},
  {"ATTACK",    "READY",  &buffer_weapon.attack_state},
  {"RELOAD",    "READY",  &buffer_weapon.reload_state},
  {"FLASH",     "REMOVE", &buffer_weapon.flash_state},
  {"SECATTACK", "READY",  &buffer_weapon.sa_attack_state},
  {"SECRELOAD", "READY",  &buffer_weapon.sa_reload_state},
  {"SECFLASH",  "REMOVE", &buffer_weapon.sa_flash_state},
  {"CROSSHAIR", "REMOVE", &buffer_weapon.crosshair},
  {"ZOOM",      "ZOOM",   &buffer_weapon.zoom_state},
  {NULL, NULL, NULL}
};

static const actioncode_t weapon_actions[] =
{
  {"NOTHING", NULL, NULL},
  {"RAISE",             A_Raise, NULL},
  {"LOWER",             A_Lower, NULL},
  {"READY",             A_WeaponReady, NULL},
  {"SHOOT",             A_WeaponShoot, DDF_StateGetAttack},
  {"EJECT",             A_WeaponEject, DDF_StateGetAttack},
  {"REFIRE",            A_ReFire, NULL},
  {"NOFIRE",            A_NoFire, NULL},
  {"NOFIRE RETURN",     A_NoFireReturn, NULL},
  {"KICK",              A_WeaponKick, DDF_StateGetFloat},
  {"SETCROSS",          A_SetCrosshair, NULL},
  {"TARGET",            A_GotTarget, NULL},
  {"LIGHT0",            A_Light0, NULL},
  {"LIGHT1",            A_Light1, NULL},
  {"LIGHT2",            A_Light2, NULL},
  {"CHECKRELOAD",       A_CheckReload, NULL},
  {"FLASH",             A_GunFlash, NULL},
  {"PLAYSOUND",         A_WeaponPlaySound, DDF_StateGetSound},
  {"KILLSOUND",         A_WeaponKillSound, NULL},
  {"JUMP",              A_WeaponJump, DDF_StateGetJump},
  {"RTS ENABLE TAGGED", A_WeaponEnableRadTrig,  DDF_StateGetInteger},
  {"RTS DISABLE TAGGED",A_WeaponDisableRadTrig, DDF_StateGetInteger},
  {"TRANS SET",         A_WeaponTransSet,  DDF_StateGetPercent},
  {"TRANS FADE",        A_WeaponTransFade, DDF_StateGetPercent},
  {"SEC SHOOT",         A_WeaponShootSA, DDF_StateGetAttack},
  {"SEC REFIRE",        A_ReFireSA, NULL},
  {"SEC NOFIRE",        A_NoFireSA, NULL},
  {"SEC NOFIRE RETURN", A_NoFireReturnSA, NULL},
  {"SEC FLASH",         A_GunFlashSA, NULL},
  {"SEC CHECKRELOAD",   A_CheckReloadSA, NULL},

  // -AJA- backwards compatibility cruft...
  {"!SOUND1",           A_SFXWeapon1, NULL},
  {"!SOUND2",           A_SFXWeapon2, NULL},
  {"!SOUND3",           A_SFXWeapon3, NULL},
  {"!RANDOMJUMP", NULL, NULL},
  {NULL, NULL, NULL}
};

const specflags_t ammo_types[] =
{
    {"NOAMMO",  AM_NoAmmo, 0},
    
    {"BULLETS", AM_Bullet, 0},
    {"SHELLS",  AM_Shell,  0},
    {"ROCKETS", AM_Rocket, 0},
    {"CELLS",   AM_Cell,   0},
    
    {"PELLETS", AM_Pellet, 0},
    {"NAILS",   AM_Nail,   0},
    {"GRENADES",AM_Grenade,0},
    {"GAS",     AM_Gas,    0},

    {NULL, 0, 0}
};

static bool WeaponTryParseState(const char *field, 
    const char *contents, int index, bool is_last)
{
  int i;
  const state_starter_t *starter;
  const char *pos;

  char labname[68];

  if (strncasecmp(field, "STATES(", 7) != 0)
    return false;

  // extract label name
  field += 7;

  pos = strchr(field, ')');
  
  if (pos == NULL || pos == field || pos > (field+64))
    return false;

  Z_StrNCpy(labname, field, pos - field);
  
  // check for the "standard" states
  starter = NULL;

  for (i=0; weapon_starters[i].label; i++)
    if (DDF_CompareName(weapon_starters[i].label, labname) == 0)
      break;
  
  if (weapon_starters[i].label)
    starter = &weapon_starters[i];

  DDF_StateReadState(contents, labname,
      &buffer_weapon.first_state, &buffer_weapon.last_state,
      starter ? starter->state_num : NULL, index, 
      is_last ? starter ? starter->last_redir : "READY" : NULL, 
      weapon_actions);

  return true;
}


//
//  DDF PARSE ROUTINES
//

static bool WeaponStartEntry(const char *name)
{
  int i;
  bool replaces = false;

  if (name && name[0])
  {
    for (i=num_disabled_weapons; i < numweapons; i++)
    {
      if (DDF_CompareName(weaponinfo[i]->ddf.name, name) == 0)
      {
        dynamic_weapon = weaponinfo[i];
        replaces = true;
        break;
      }
    }
    
    // ** NOTE **
    // Normally we adjust the pointer array here to keep the
    // newest entries at the end.  For weapons however, there are
    // integer indices stored in various places (E.g. benefit
    // system), so we must keep the order one-to-one between
    // pointer and weaponinfo_t structure.
  }

  // not found, create a new one
  if (! replaces)
  {
    Z_SetArraySize(&weaponinfo_a, ++numweapons);
 
    dynamic_weapon = weaponinfo[numweapons - 1];
    dynamic_weapon->ddf.name = (name && name[0]) ? Z_StrDup(name) :
        DDF_MainCreateUniqueName("UNNAMED_WEAPON", numweapons);
  }

  dynamic_weapon->ddf.number = 0;

  // instantiate the static entry
  buffer_weapon = template_weapon;

  return replaces;
}

static void WeaponParseField(const char *field, const char *contents,
    int index, bool is_last)
{
#if (DEBUG_DDF)  
  L_WriteDebug("WEAPON_PARSE: %s = %s;\n", field, contents);
#endif

  if (DDF_MainParseField(weapon_commands, field, contents))
    return;
  
  if (WeaponTryParseState(field, contents, index, is_last))
    return;

  // handle properties
  if (index == 0 && DDF_CompareName(contents, "TRUE") == 0)
  {
    DDF_WGetSpecialFlags(field, NULL);  // FIXME FOR OFFSETS
    return;
  }

  DDF_WarnError("Unknown weapons.ddf command: %s\n", field);
}

static void WeaponFinishEntry(void)
{
  ddf_base_t base;
  
  if (! buffer_weapon.first_state)
    DDF_Error("Weapon `%s' has missing states.\n",
        dynamic_weapon->ddf.name);

  DDF_StateFinishStates(buffer_weapon.first_state, buffer_weapon.last_state);

  // check stuff...

  if (buffer_weapon.ammopershot < 0)
  {
    DDF_WarnError("Bad AMMOPERSHOT value for weapon: %d\n",
      buffer_weapon.ammopershot);
    buffer_weapon.ammopershot = 1;
  }

  // backwards compatibility (REMOVE for 1.26)
  if (buffer_weapon.priority < 0)
  {
    if (! no_obsoletes)
      DDF_Warning("Using PRIORITY=-1 in weapons.ddf is obsolete !\n");

    buffer_weapon.dangerous = true;
    buffer_weapon.priority = 10;
  }
 
  // transfer static entry to dynamic entry
  
  base = dynamic_weapon->ddf;
  dynamic_weapon[0] = buffer_weapon;
  dynamic_weapon->ddf = base;

  // compute CRC...
  CRC32_Init(&dynamic_weapon->ddf.crc);

  CRC32_ProcessInt(&dynamic_weapon->ddf.crc, dynamic_weapon->ammo);
  CRC32_ProcessInt(&dynamic_weapon->ddf.crc, dynamic_weapon->ammopershot);

  // FIXME: do more stuff...

  CRC32_Done(&dynamic_weapon->ddf.crc);
}

static void WeaponClearAll(void)
{
  // not safe to delete weapons, there are (integer) references

  num_disabled_weapons = numweapons;
}


//
// DDF_ReadWeapons
//
void DDF_ReadWeapons(void *data, int size)
{
  readinfo_t weapons;

  weapons.memfile = (char*)data;
  weapons.memsize = size;
  weapons.tag = "WEAPONS";
  weapons.entries_per_dot = 1;

  if (weapons.memfile)
  {
    weapons.message = NULL;
    weapons.filename = NULL;
    weapons.lumpname = "DDFWEAP";
  }
  else
  {
    weapons.message = "DDF_InitWeapons";
    weapons.filename = "weapons.ddf";
    weapons.lumpname = NULL;
  }

  weapons.start_entry  = WeaponStartEntry;
  weapons.parse_field  = WeaponParseField;
  weapons.finish_entry = WeaponFinishEntry;
  weapons.clear_all    = WeaponClearAll;

  DDF_MainReadFile(&weapons);
}

void DDF_WeaponInit(void)
{
  Z_InitStackArray(&weaponinfo_a, (void ***)&weaponinfo, sizeof(weaponinfo_t), 0);
}

void DDF_WeaponCleanUp(void)
{
  // compute the weaponkey array

  int key, i;

  memset(&weaponkey, 0, sizeof(weaponkey));

  for (key=0; key < 10; key++)
  {
    weaponkey_t *w = &weaponkey[key];
    
    for (i=num_disabled_weapons; i < numweapons; i++)
    {
      if (weaponinfo[i]->bind_key != key)
        continue;
      
      Z_Resize(w->choices, weaponinfo_t *, ++w->numchoices);
      w->choices[w->numchoices-1] = weaponinfo[i];
    }

#if (DEBUG_DDF)
    L_WriteDebug("DDF_Weap: CHOICES ON KEY %d:\n", key);
    for (i=0; i < w->numchoices; i++)
    {
      L_WriteDebug("  [%s] pri=%d\n", w->choices[i]->ddf.name,
          w->choices[i]->priority);
    }
#endif

    if (w->numchoices < 2)
      continue;
    
    // sort choices based on weapon priority
#define CMP(a, b)  ((a)->priority < (b)->priority)
    QSORT(weaponinfo_t *, w->choices, w->numchoices, CUTOFF);
#undef CMP

  }
}

static void DDF_WGetAmmo(const char *info, void *storage)
{
  int *ammo = (int *)storage;
  int flag_value;

  switch (DDF_MainCheckSpecialFlag(info, ammo_types, &flag_value,
      false, false))
  {
    case CHKF_Positive:
    case CHKF_Negative:
      (*ammo) = flag_value;
      break;
    
    case CHKF_User:
    case CHKF_Unknown:
        DDF_WarnError("Unknown Ammo type '%s'\n", info);
      break;
  }
}

int DDF_WeaponLookup(const char *name)
{
  int i;

  for (i=num_disabled_weapons; i < numweapons; i++)
  {
    if (DDF_CompareName(weaponinfo[i]->ddf.name, name) == 0)
      return i;
  }

  return -1;
}

static void DDF_WGetUpgrade(const char *info, void *storage)
{
  int *dest = (int *)storage;

  *dest = DDF_WeaponLookup(info);

  if (*dest < 0)
    DDF_WarnError("Unknown weapon to upgrade: %s\n", info);
}

static specflags_t weapon_specials[] =
{
    {"SILENT TO MONSTERS", WPSP_SilentToMonsters, 0},
    {NULL, WPSP_None, 0}
};

//
// DDF_WGetSpecialFlags
//
static void DDF_WGetSpecialFlags(const char *info, void *storage)
{
  int flag_value;

  switch (DDF_MainCheckSpecialFlag(info, weapon_specials, &flag_value,
      true, false))
  {
    case CHKF_Positive:
      buffer_weapon.special_flags = (weapon_flag_e)(buffer_weapon.special_flags | flag_value);
      break;
    
    case CHKF_Negative:
      buffer_weapon.special_flags = (weapon_flag_e)(buffer_weapon.special_flags & ~flag_value);
      break;

    case CHKF_User:
    case CHKF_Unknown:
      DDF_WarnError("DDF_WGetSpecialFlags: Unknown Special: %s", info);
      break;
  }
}

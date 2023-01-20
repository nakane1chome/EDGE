//----------------------------------------------------------------------------
//  EDGE2 Console Variables
//----------------------------------------------------------------------------
// 
//  Copyright (c) 2007-2009  The EDGE2 Team.
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

#ifndef __CON_VAR_H__
#define __CON_VAR_H__

#include <vector>
#include <cassert>
#include <string>
#include <cmath>

class cvar_c
{
public:
	// this is incremented each time a value is set.
	// (Note: whether the value is different is not checked)
	int modified;

private:
	union {
		int *d;
		float *f;
		std::string *str;
	};

	struct { // FIXME: should be union, but struct because std::string doesn't work in union
		int d;
		float f;
		std::string str;
	} defval;

	enum class cvar_type_e {
		CVT_INT,
		CVT_FLOAT,
		CVT_STRING
	} type;

	enum bufsize_e { BUFSIZE = 24 };

	char buffer[BUFSIZE];
	char defbuffer[BUFSIZE];

public:

	inline cvar_c(int *value) {
		type = cvar_type_e::CVT_INT;
		d = value;
		defval.d = *value;
	}
	inline cvar_c(float *value) {
		type = cvar_type_e::CVT_FLOAT;
		f = value;
		defval.f = *value;
	}
	inline cvar_c(std::string *value) {
		type = cvar_type_e::CVT_STRING;
		str = value;
		defval.str = *value;
	}
	// cvar_c(const cvar_c& other);

	~cvar_c();

	inline void reset_to_default() {
		if(type == cvar_type_e::CVT_STRING) {
			*str = defval.str;
		} else if(type == cvar_type_e::CVT_INT) {
			*d = defval.d;
		} else if(type == cvar_type_e::CVT_FLOAT) {
			*f = defval.f;
		}
	}

	inline const char * const get_casted_default_string() {
		if(type == cvar_type_e::CVT_STRING) {
			return defval.str.c_str();
		} else if(type == cvar_type_e::CVT_INT) {
			sprintf(defbuffer, "%d", defval.d);
			return defbuffer;
		} else if(type == cvar_type_e::CVT_FLOAT) {
			float ab = fabs(defval.f);

			if (ab >= 1e10)  // handle huge numbers
				sprintf(defbuffer, "%1.5e", defval.f);
			else if (ab >= 1e5)
				sprintf(defbuffer, "%1.1f", defval.f);
			else if (ab >= 1e3)
				sprintf(defbuffer, "%1.3f", defval.f);
			else if (ab >= 1.0)
				sprintf(defbuffer, "%1.5f", defval.f);
			else
				sprintf(defbuffer, "%1.7f", defval.f);

			return defbuffer;
		}
		return NULL;
	}

	cvar_c& operator= (int value);
	cvar_c& operator= (float value);
	cvar_c& operator= (const char *value);
	cvar_c& operator= (std::string value);

	inline int &get_int() {
		assert(type == cvar_type_e::CVT_INT);
		return *d;
	}

	inline float &get_float() {
		assert(type == cvar_type_e::CVT_FLOAT);
		return *f;
	}

	inline std::string &get_string() {
		assert(type == cvar_type_e::CVT_STRING);
		return *str;
	}

	inline const char * const get_casted_string() {
		if(type == cvar_type_e::CVT_STRING) {
			return str->c_str();
		} else if(type == cvar_type_e::CVT_INT) {
			sprintf(buffer, "%d", *d);
			return buffer;
		} else if(type == cvar_type_e::CVT_FLOAT) {
			float ab = fabs(*f);

			if (ab >= 1e10)  // handle huge numbers
				sprintf(buffer, "%1.5e", *f);
			else if (ab >= 1e5)
				sprintf(buffer, "%1.1f", *f);
			else if (ab >= 1e3)
				sprintf(buffer, "%1.3f", *f);
			else if (ab >= 1.0)
				sprintf(buffer, "%1.5f", *f);
			else
				sprintf(buffer, "%1.7f", *f);

			return buffer;
		}
		return NULL;
	}

	// this checks and clears the 'modified' value
	inline bool CheckModified()
	{
		if (modified)
		{
			modified = 0; return true;
		}
		return false;
	}
};


typedef struct cvar_link_s
{
	// name of variable
	const char *name;

	// the console variable itself
	cvar_c *var;

	// flags (a combination of letters, "" for none)
	const char *flags;

	cvar_link_s *next;
}
cvar_link_t;

extern cvar_link_t *all_cvars_list;
// extern cvar_link_t all_cvars[];


void CON_ResetAllVars(bool initial = false);
// sets all cvars to their default value.
// When 'initial' is true, the modified counts are set to 0.

cvar_link_t * CON_FindVar(const char *name);
// look for a CVAR with the given name.

bool CON_MatchPattern(const char *name, const char *pat);

int CON_MatchAllVars(std::vector<const char *>& list,
                     const char *pattern, const char *flags = "");
// find all cvars which match the pattern, and copy pointers to
// them into the given list.  The flags parameter, if present,
// contains lowercase letters to match the CVAR with the flag,
// and/or uppercase letters to require the flag to be absent.
//
// Returns number of matches found.

bool CON_SetVar(const char *name, const char *flags, const char *value);
// sets the cvar with the given name (possibly an alias) with the
// given value.  The flags parameter can limit the search, and
// must begin with an 'A' to prevent matching aliases.
// Returns true if the cvar was found.

void CON_HandleProgramArgs(void);
// scan the program arguments and set matching cvars

#define DEF_CVAR(cvar_name, cvar_type, cvar_flags, ...) \
	cvar_type cvar_name = (__VA_ARGS__);\
	cvar_c cvar_name##_cv_(&cvar_name);\
	cvar_link_s cvar_name##_cv_link_ {#cvar_name, &cvar_name##_cv_, cvar_flags, NULL};\
	int cvar_name##_cv_setup_() {\
		cvar_name##_cv_link_.next = ::all_cvars_list;\
		::all_cvars_list = &cvar_name##_cv_link_;\
		return 0;\
	}\
	int cvar_name##_cv_setup_hack_ = cvar_name##_cv_setup_();

#endif // __CON_VAR_H__

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab

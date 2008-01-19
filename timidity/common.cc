/*
    TiMidity -- Experimental MIDI to WAVE converter
    Copyright (C) 1995 Tuukka Toivonen <toivonen@clinet.fi>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   common.c
*/

#include "config.h"

#include <errno.h>

#include "common.h"
#include "mix.h"
#include "ctrlmode.h"

/* I guess "rb" should be right for any libc */
#define OPEN_MODE "rb"

char current_filename[1024];

static PathList *pathlist=NULL;


/* This is meant to find and open files for reading, possibly piping
   them through a decompressor. */
FILE *open_file(const char *name, int decompress, int noise_mode)
{
	FILE *fp;
	PathList *plp;

	if (!name || !(*name))
	{
		ctl_msg(CMSG_ERROR, VERB_NORMAL, "Attempted to open nameless file.");
		return 0;
	}

#ifdef DEFAULT_PATH
	if (pathlist==NULL)
	{
		/* Generate path list */
		add_to_pathlist(DEFAULT_PATH);
	}
#endif

	/* First try the given name */

	strncpy(current_filename, name, 1023);
	current_filename[1023]='\0';

	ctl_msg(CMSG_INFO, VERB_DEBUG, "Trying to open %s", current_filename);
	if ((fp = fopen(current_filename, OPEN_MODE)))
		return fp;

#ifdef ENOENT
	if (noise_mode && (errno != ENOENT))
	{
		ctl_msg(CMSG_ERROR, VERB_NORMAL, "%s: %s", 
				current_filename, strerror(errno));
		return 0;
	}
#endif

	if (name[0] != DIRSEPARATOR)
	{
		/* Try along the path then */
		for (plp = pathlist; plp; plp = plp->next)
		{
			current_filename[0] = 0;

			int l = strlen(plp->path);
			if (l > 0)
			{
				strcpy(current_filename, plp->path);

				if (current_filename[l-1] != DIRSEPARATOR)
				{
					current_filename[l]   = DIRSEPARATOR;
					current_filename[l+1] = 0;
				}
			}
			
			strcat(current_filename, name);

			ctl_msg(CMSG_INFO, VERB_DEBUG, "Trying to open %s", current_filename);
			if ((fp = fopen(current_filename, OPEN_MODE)))
				return fp;
#ifdef ENOENT
			if (noise_mode && (errno != ENOENT))
			{
				ctl_msg(CMSG_ERROR, VERB_NORMAL, "%s: %s", 
						current_filename, strerror(errno));
				return 0;
			}
#endif
		}
	}

	/* Nothing could be opened. */

	*current_filename=0;

	if (noise_mode >= 2)
		ctl_msg(CMSG_ERROR, VERB_NORMAL, "%s: %s", name, strerror(errno));

	return NULL;
}

/* This closes files opened with open_file */
void close_file(FILE *fp)
{
	fclose(fp);
}

/* This is meant for skipping a few bytes in a file or fifo. */
void skip(FILE *fp, size_t len)
{
	size_t c;
	char tmp[1024];

	while (len>0)
	{
		c=len;
		if (c>1024) c=1024;
		len-=c;
		if (c!=fread(tmp, 1, c, fp))
			ctl_msg(CMSG_ERROR, VERB_NORMAL, "%s: skip: %s",
					current_filename, strerror(errno));
	}
}

/* This'll allocate memory or die. */
void *safe_malloc(size_t count)
{
	if (count > (1<<21))
	{
		I_Error("Timidity felt like allocating %d bytes. This must be a bug.\n", count);
		/* NOT REACHED */
	}

	void *p = malloc(count);

	if (! p)
	{
		I_Error("Timidity: Couldn't malloc %d bytes.\n", count);
		/* NOT REACHED */
	}

	return p;
}

char *safe_strdup(const char *orig)
{
	if (! orig) orig = "";

	int len = strlen(orig);

	char *result = (char *)safe_malloc(len+1);

	strcpy(result, orig);;

	return result;
}

/* This adds a directory to the path list */
void add_to_pathlist(char *s)
{
	PathList *plp = (PathList*) safe_malloc(sizeof(PathList));
	plp->path = safe_strdup(s);
	///  strcpy((plp->path=safe_malloc(strlen(s)+1)),s);
	plp->next=pathlist;
	pathlist=plp;
}

/* Free memory associated to path list */
void free_pathlist(void)
{
	PathList *plp, *next_plp;

	for (plp = pathlist; plp; plp = next_plp)
	{
		if (plp->path)
		{
			free(plp->path);
			plp->path=NULL;
		}

		next_plp = plp->next;
		free(plp);
	}
	pathlist = NULL;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab

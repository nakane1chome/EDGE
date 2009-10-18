//----------------------------------------------------------------------------
//  EDGE Rendering Data Handling Code
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2008  The EDGE Team.
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
// -ACB- 1998/09/09 Reformatted File Layout.
// -KM- 1998/09/27 Colourmaps can be dynamically changed.
// -ES- 2000/02/12 Moved most of this module to w_texture.c.

#include "i_defs.h"

#include "e_search.h"
#include "g_state.h"
#include "m_argv.h"
#include "m_misc.h"
#include "p_local.h"
#include "r_image.h"
#include "r_sky.h"
#include "w_flat.h"
#include "w_wad.h"
#include "w_texture.h"
#include "z_zone.h"


void W_InitFlats(void)
{
	int max_file = W_GetNumFiles();
	int j, file;

	I_Printf("W_InitFlats...\n");

	// iterate over each file, creating our big array of flats

	int total = 0;

	for (file=0; file < max_file; file++)
	{
		epi::u32array_c& lumps = W_GetListLumps(file, LMPLST_Flats);
		total += lumps.GetSize();
	}

	if (total == 0)
		I_Error("No flats found !  Make sure the chosen IWAD is valid.\n");

	int *F_lumps = new int[total];
	int numflats = 0;

	for (file=0; file < max_file; file++)
	{
		epi::u32array_c& lumps = W_GetListLumps(file, LMPLST_Flats);
		int lumpnum = lumps.GetSize();

		for (j=0; j < lumpnum; j++)
			F_lumps[numflats++] = lumps[j];
	}

	// now sort the flats, primarily by increasing name, secondarily by
	// increasing lump number (a measure of newness).

#define CMP(a, b)  \
	(strcmp(W_GetLumpName(a), W_GetLumpName(b)) < 0 || \
	 (strcmp(W_GetLumpName(a), W_GetLumpName(b)) == 0 && a < b))
		QSORT(int, F_lumps, numflats, CUTOFF);
#undef CMP

	// remove duplicate names.  We rely on the fact that newer lumps
	// have greater lump values than older ones.  Because the QSORT took
	// newness into account, only the last entry in a run of identically
	// named flats needs to be kept.

	for (j=1; j < numflats; j++)
	{
		int a = F_lumps[j - 1];
		int b = F_lumps[j];

		if (strcmp(W_GetLumpName(a), W_GetLumpName(b)) == 0)
			F_lumps[j - 1] = -1;
	}

	for (j=0; j < numflats; j++)
	{
		if (F_lumps[j] < 0)
			continue;

#if 0  // DEBUGGING
		I_Debugf("FLAT #%d:  lump=%d  name=[%s]\n", j,
				F_lumps[j], W_GetLumpName(F_lumps[j]));
#endif
		R_ImageCreateFlat(W_GetLumpName(F_lumps[j]), F_lumps[j]);
	}
    
	delete[] F_lumps;
}


//----------------------------------------------------------------------------

//
// FLAT ANIMATIONS
//
// Here are the rules for flats, they get a bit hairy, but are the
// simplest thing which achieves expected behaviour:
//
// 1. When two flats in different wads have the same name, the flat
//    in the _later_ wad overrides the flat in the earlier wad.  This
//    allows pwads to replace iwad flats -- as is usual.  For general
//    use of flats (e.g. in levels) their order is not an issue.
//
// 2. The flat animation sequence is determined by the _earliest_ wad
//    which contains _both_ the start and the end flat.  The sequence
//    contained in that wad becomes the animation sequence (the list
//    of flat names).  These names are then looked up normally, so
//    flats in newer wads will get used if their name matches one in
//    the sequence.
//
// -AJA- 2001/01/28: reworked flat animations.
// 
void R_AddFlatAnim(animdef_c *anim)
{
	if (anim->pics.GetSize() == 0)  // old way
	{
		int start = W_CheckNumForName(anim->startname);
		int end   = W_CheckNumForName(anim->endname);

		int file;
		int s_offset, e_offset;

		int i;

		if (start == -1 || end == -1)
		{
			// sequence not valid.  Maybe it is the DOOM 1 IWAD.
			return;
		}

		file = W_FindFlatSequence(anim->startname, anim->endname, 
				&s_offset, &e_offset);

		if (file < 0)
		{
			I_Warning("Missing flat animation: %s-%s not in any wad.\n",
					(const char*)anim->startname, (const char*)anim->endname);
			return;
		}

		epi::u32array_c& lumps = W_GetListLumps(file, LMPLST_Flats);
		int total = lumps.GetSize();

		SYS_ASSERT(s_offset <= e_offset);
		SYS_ASSERT(e_offset < total);

		// determine animation sequence
		total = e_offset - s_offset + 1;

		const image_c **flats = new const image_c* [total];

		// lookup each flat
		for (i=0; i < total; i++)
		{
			const char *name = W_GetLumpName(lumps[s_offset + i]);

			// Note we use W_ImageFromFlat() here.  It might seem like a good
			// optimisation to use the lump number directly, but we can't do
			// that -- the lump list does NOT take overriding flats (in newer
			// pwads) into account.

			flats[i] = R_ImageLookup(name, INS_Flat, ILF_Null|ILF_Exact|ILF_NoNew);
		}

		R_AnimateImageSet(flats, total, anim->speed);
		delete[] flats;
	}

	// -AJA- 2004/10/27: new SEQUENCE command for anims

	int total = anim->pics.GetSize();

	if (total == 1)
		return;

	const image_c **flats = new const image_c* [total];

	for (int i = 0; i < total; i++)
	{
		flats[i] = R_ImageLookup(anim->pics[i], INS_Flat, ILF_Null|ILF_Exact);
	}

	R_AnimateImageSet(flats, total, anim->speed);
	delete[] flats;
}

//
// TEXTURE ANIMATIONS
//
// Here are the rules for textures:
//
// 1. The TEXTURE1/2 lumps require a PNAMES lump to complete their
//    meaning.  Some wads have the TEXTURE1/2 lump(s) but lack a
//    PNAMES lump -- in this case the next oldest PNAMES lump is used
//    (e.g. the one in the IWAD).
// 
// 2. When two textures in different wads have the same name, the
//    texture in the _later_ wad overrides the one in the earlier wad,
//    as is usual.  For general use of textures (e.g. in levels),
//    their ordering is not an issue.
//
// 3. The texture animation sequence is determined by the _latest_ wad
//    whose TEXTURE1/2 lump contains _both_ the start and the end
//    texture.  The sequence within that lump becomes the animation
//    sequence (the list of texture names).  These names are then
//    looked up normally, so textures in newer wads can get used if
//    their name matches one in the sequence.
// 
// -AJA- 2001/06/17: reworked texture animations.
// 
void R_AddTextureAnim(animdef_c *anim)
{
	if (anim->pics.GetSize() == 0)  // old way
	{
		int set, s_offset, e_offset;

		set = W_FindTextureSequence(anim->startname, anim->endname,
				&s_offset, &e_offset);

		if (set < 0)
		{
			// sequence not valid.  Maybe it is the DOOM 1 IWAD.
			return;
		}

		SYS_ASSERT(s_offset <= e_offset);

		int total = e_offset - s_offset + 1;
		const image_c **texs = new const image_c* [total];

		// lookup each texture
		for (int i = 0; i < total; i++)
		{
			const char *name = W_TextureNameInSet(set, s_offset + i);
			texs[i] = R_ImageLookup(name, INS_Texture, ILF_Null|ILF_Exact|ILF_NoNew);
		}

		R_AnimateImageSet(texs, total, anim->speed);
		delete[] texs;

		return;
	}

	// -AJA- 2004/10/27: new SEQUENCE command for anims

	int total = anim->pics.GetSize();

	if (total == 1)
		return;

	const image_c **texs = new const image_c* [total];

	for (int i = 0; i < total; i++)
	{
		texs[i] = R_ImageLookup(anim->pics[i], INS_Texture, ILF_Null|ILF_Exact);
	}

	R_AnimateImageSet(texs, total, anim->speed);
	delete[] texs;
}


void R_AddGraphicAnim(animdef_c *anim)
{
	int total = anim->pics.GetSize();

	SYS_ASSERT(total != 0);

	if (total == 1)
		return;

	const image_c **users = new const image_c* [total];

	for (int i = 0; i < total; i++)
	{
		users[i] = R_ImageLookup(anim->pics[i], INS_Graphic, ILF_Null|ILF_Exact);
	}

	R_AnimateImageSet(users, total, anim->speed);
	delete[] users;
}


void W_InitPicAnims(void)
{
	epi::array_iterator_c it;
	animdef_c *A;

	// loop through animdefs, and add relevant anims.
	// Note: reverse order, give priority to newer anims.
	for (it = animdefs.GetTailIterator(); it.IsValid(); it--)
	{
		A = ITERATOR_TO_TYPE(it, animdef_c*);

		SYS_ASSERT(A);

		switch (A->type)
		{
			case animdef_c::A_Texture:
				R_AddTextureAnim(A);
				break;

			case animdef_c::A_Flat:
				R_AddFlatAnim(A);
				break;

			case animdef_c::A_Graphic:
				R_AddGraphicAnim(A);
				break;
		}
	}
}


//----------------------------------------------------------------------------

//
// Preloads all relevant graphics for the level.
//
// -AJA- 2001/06/18: Reworked for image system.
//                   
void W_PrecacheLevel(void)
{
	int max_image;
	int count = 0;
	int i;

	// do sprites first -- when memory is tight, they'll be the first
	// images to be removed from the cache(s).

	if (true) /// M_CheckParm("-fastsprite") || M_CheckParm("-fastsprites"))
	{
		W_PrecacheSprites();
	}

	// maximum possible images
	max_image = 1 + 3 * numsides + 2 * numsectors;

	const image_c ** images = new const image_c* [max_image];

	// Sky texture is always present.
	images[count++] = sky_image;

	// add in sidedefs
	for (i=0; i < numsides; i++)
	{
		if (sides[i].top.image)
			images[count++] = sides[i].top.image;

		if (sides[i].middle.image)
			images[count++] = sides[i].middle.image;

		if (sides[i].bottom.image)
			images[count++] = sides[i].bottom.image;
	}

	SYS_ASSERT(count <= max_image);

	// add in planes
	for (i=0; i < numsectors; i++)
	{
		if (sectors[i].floor.image)
			images[count++] = sectors[i].floor.image;

		if (sectors[i].ceil.image)
			images[count++] = sectors[i].ceil.image;
	}

	SYS_ASSERT(count <= max_image);

	// Sort the images, so we can ignore the duplicates

#define CMP(a, b)  (a < b)
	QSORT(const image_c *, images, count, CUTOFF);
#undef CMP

	for (i=0; i < count; i++)
	{
		SYS_ASSERT(images[i]);

		if (i+1 < count && images[i] == images[i + 1])
			continue;

		if (images[i] == skyflatimage)
			continue;

		images[i]->PreCache();
	}

	RGL_PreCacheSky();

	delete[] images;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
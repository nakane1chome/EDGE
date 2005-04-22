//------------------------------------------------------------------------
//  EDITOR : Editing widget
//------------------------------------------------------------------------
//
//  Editor for DDF   (C) 2005  The EDGE Team
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
//------------------------------------------------------------------------
//
//  Based on the "editor.cxx" sample program from FLTK 1.1.6,
//  as described in Chapter 4 of the FLTK Programmer's Guide.
//  
//  Copyright 1998-2004 by Bill Spitzak, Mike Sweet and others.
//
//------------------------------------------------------------------------

#include "defs.h"


#define FLOWOVER_STYLE(ch)  ((ch) == 'S' || (ch) == 'T' || (ch) == 'I')


void style_unfinished_cb(int, void *);
void style_update_cb(int, int, int, int, const char *, void *);


W_Editor::W_Editor(int X, int Y, int W, int H, const char *label) :
	Fl_Text_Editor(X, Y, W, H, label),
	cur_scheme(-1)
{
	textbuf  = new Fl_Text_Buffer;
	stylebuf = new Fl_Text_Buffer;

	buffer(textbuf);

	SetScheme(SCHEME_Dark, 18);

	textbuf->add_modify_callback(style_update_cb, this);
}

W_Editor::~W_Editor()
{
	delete textbuf;
	delete stylebuf;
}

void W_Editor::SetScheme(int kind, int font_h)
{
	textfont(FL_COURIER);

	if (kind == SCHEME_Dark)
	{
		color(FL_BLACK /* background */, FL_WHITE /* selection */);
		cursor_color(FL_WHITE);
	}
	else
	{
		color(FL_WHITE /* background */, FL_BLACK /* selection */);
		cursor_color(FL_BLACK);
	}

	for (int i = 0; i < TABLE_SIZE; i++)
	{
		table_dark [i].size = font_h;
		table_light[i].size = font_h;
	}

	highlight_data(stylebuf, (kind == SCHEME_Dark) ? table_dark : table_light,
		TABLE_SIZE, 'Z', style_unfinished_cb, this);

	cur_scheme = kind;
}

int W_Editor::handle(int event)
{
	if (event == FL_KEYDOWN)
	{
		int key = Fl::event_key();

		if (key == FL_Tab)
			return 1;
	}

	// pass it along to daddy...
	return Fl_Text_Editor::handle(event);
}

//---------------------------------------------------------------------------

Fl_Text_Display::Style_Table_Entry W_Editor::table_dark[W_Editor::TABLE_SIZE] =
{
	{ FL_WHITE,      FL_COURIER,        14, 0 }, // A - Average stuff
	{ FL_GREEN,      FL_COURIER_BOLD,   14, 0 }, // B - Backslash \X
	{ FL_BLUE,       FL_COURIER,        14, 0 }, // C - Comments //
	{ FL_MAGENTA,    FL_COURIER,        14, 0 }, // D - Directives #
	{ FL_GREEN,      FL_COURIER_BOLD,   14, 0 }, // E - ERRORS
	{ FL_WHITE,      FL_COURIER_BOLD,   14, 0 }, // F
	{ FL_WHITE,      FL_COURIER_BOLD,   14, 0 }, // G
	{ FL_WHITE,      FL_COURIER_BOLD,   14, 0 }, // H
	{ FL_RED,        FL_COURIER,        14, 0 }, // I - Items [ ]
	{ FL_WHITE,      FL_COURIER,        14, 0 }, // J 
	{ FL_WHITE,      FL_COURIER,        14, 0 }, // K - Keyword
	{ FL_WHITE,      FL_COURIER,        14, 0 }, // L 
	{ FL_WHITE,      FL_COURIER,        14, 0 }, // M 
	{ FL_YELLOW,     FL_COURIER,        14, 0 }, // N - Numbers
	{ FL_WHITE,      FL_COURIER,        14, 0 }, // O 
	{ FL_WHITE,      FL_COURIER,        14, 0 }, // P - Parentheses ( )
	{ FL_GREEN,      FL_COURIER,        14, 0 }, // Q
	{ FL_WHITE,      FL_COURIER_BOLD,   14, 0 }, // R
	{ FL_GREEN,      FL_COURIER,        14, 0 }, // S - Strings ""
	{ FL_MAGENTA,    FL_COURIER_BOLD,   14, 0 }  // T - Tags < >
};

Fl_Text_Display::Style_Table_Entry W_Editor::table_light[W_Editor::TABLE_SIZE] =
{
	{ FL_BLACK,      FL_COURIER,        14, 0 }, // A - Average stuff
	{ FL_GREEN,      FL_COURIER_BOLD,   14, 0 }, // B - Backslash \X
	{ FL_BLUE,       FL_COURIER,        14, 0 }, // C - Comments //
	{ FL_MAGENTA,    FL_COURIER,        14, 0 }, // D - Directives #
	{ FL_GREEN,      FL_COURIER_BOLD,   14, 0 }, // E - ERRORS
	{ FL_BLACK,      FL_COURIER_BOLD,   14, 0 }, // F
	{ FL_BLACK,      FL_COURIER_BOLD,   14, 0 }, // G
	{ FL_BLACK,      FL_COURIER_BOLD,   14, 0 }, // H
	{ FL_RED,        FL_COURIER,        14, 0 }, // I - Items [ ]
	{ FL_BLACK,      FL_COURIER,        14, 0 }, // J 
	{ FL_BLACK,      FL_COURIER,        14, 0 }, // K - Keyword
	{ FL_BLACK,      FL_COURIER,        14, 0 }, // L 
	{ FL_BLACK,      FL_COURIER,        14, 0 }, // M 
	{ FL_BLACK,      FL_COURIER,        14, 0 }, // N - Numbers
	{ FL_BLACK,      FL_COURIER,        14, 0 }, // O 
	{ FL_BLACK,      FL_COURIER,        14, 0 }, // P - Parentheses ( )
	{ FL_GREEN,      FL_COURIER,        14, 0 }, // Q 
	{ FL_BLACK,      FL_COURIER_BOLD,   14, 0 }, // R
	{ FL_GREEN,      FL_COURIER,        14, 0 }, // S - Strings ""
	{ FL_MAGENTA,    FL_COURIER_BOLD,   14, 0 }  // T - Tags < >
};



//
// 'style_unfinished_cb()' - Update unfinished styles.
//

void style_unfinished_cb(int, void*)
{
}


//
// 'style_update()' - Update the style buffer...
//

void style_update_cb(int pos,		// I - Position of update
		 int nInserted,	// I - Number of inserted chars
	     int nDeleted,	// I - Number of deleted chars
         int nRestyled,	// I - Number of restyled chars
	     const char * deletedText,// I - Text that was deleted
         void *cbArg) 	// I - Callback data
{
	(void)nRestyled;
	(void)deletedText;

	W_Editor *WE = (W_Editor*) cbArg;

	// If this is just a selection change, just unselect the style buffer...
	if (nInserted == 0 && nDeleted == 0)
	{
		WE->stylebuf->unselect();
		return;
	}

	// Track changes in the text buffer...
	if (nInserted > 0)
	{
		// Insert characters into the style buffer...
		char *style = new char[nInserted + 1];
		memset(style, 'A', nInserted);
		style[nInserted] = '\0';

		WE->stylebuf->replace(pos, pos + nDeleted, style);
		delete[] style;
	}
	else
	{
		// Just delete characters in the style buffer...
		WE->stylebuf->remove(pos, pos + nDeleted);
	}

	// Select the area that was just updated to avoid unnecessary
	// callbacks...
	WE->stylebuf->select(pos, pos + nInserted - nDeleted);

	// --------------------------------------------------------

	// Re-parse the changed region; we do this by parsing from the
	// beginning of the line of the changed region to the end of
	// the line of the changed region...  Then we check the last
	// style character and keep updating if we have a multi-line
	// comment character...

	int start = WE->textbuf->line_start(pos);
	int end   = WE->textbuf->line_end(pos + nInserted);

	if (end + 1 < WE->textbuf->length())
		end++;

	for (;;)
	{
		// NOTE: 'end' could be invalid if last line has no trailing NUL.
		//       The character() method returns NUL in this case.
		char last_before = WE->stylebuf->character(end-1);
#if 0
fprintf(stderr, "last_before: %d,%d,%d -> %c%c%c\n",
WE->textbuf->character(end-2), WE->textbuf->character(end-1), WE->textbuf->character(end),
WE->stylebuf->character(end-2), WE->stylebuf->character(end-1), WE->stylebuf->character(end));
#endif
		WE->ParseStyleRange(start, end);

		char last_after = WE->stylebuf->character(end-1);
#if 0
fprintf(stderr, "last_after: %d,%d,%d -> %c%c%c\n",
WE->textbuf->character(end-2), WE->textbuf->character(end-1), WE->textbuf->character(end),
WE->stylebuf->character(end-2), WE->stylebuf->character(end-1), WE->stylebuf->character(end));
#endif
		if (last_before == last_after)
			break;

		// The newline ('\n') on the end line changed styles, so
		// reparse another chunk.

		start = end;

		if (start >= WE->textbuf->length())
			break;

		end = WE->textbuf->skip_lines(start, 5);
	}
}

void W_Editor::ParseStyleRange(int start, int end)
{
	// get style of previous line (from the '\n' character)
	char context = 'A';

	if (start > 0)
		context = stylebuf->character(start - 1);

	char *text  = textbuf->text_range(start, end);
	char *style = stylebuf->text_range(start, end);
	{
		ParseStyle(text, text + (end - start), style, context);

		stylebuf->replace(start, end, style);
	}
	free(text);
	free(style);

	redisplay_range(start, end);
}

bool W_Editor::Load(const char *filename)
{
	//!!!!! FIXME: remove <CR> characters
	if (textbuf->insertfile(filename, 0) != 0)
	{
		fl_alert("Loading file %s failed\n", filename);
		exit(9); //!!!!
	}

	return true;
}

//!!!!! FIXME: Save method


//---------------------------------------------------------------------------
//  STYLE PARSING CODE
//---------------------------------------------------------------------------


//
// 'compare_keywords()' - Compare two keywords...
//

int compare_keywords(const void *a, const void *b)
{
	return (strcmp(*((const char **)a), *((const char **)b)));
}


void W_Editor::ParseStyle(const char *text, const char *t_end, char *style,
						  char context)
{
	SYS_ASSERT(t_end > text);

	bool at_col0 = true;

	if (FLOWOVER_STYLE(context))
	{
		int len = 0;

		switch (context)
		{
			case 'S':
				len = ParseString(text, t_end, style);
				break;

			case 'T':
				len = ParseTag(text, t_end, style);
				break;

			case 'I':
				len = ParseEntry(text, t_end, style);
				break;
			
			default:
				AssertFail("FLOWOVER_STYLE '%c' is not handled.\n", context);
				break;  /* NOT REACHED */
		}

		text += len;
		style += len;
		at_col0 = false;
	}

	while (text < t_end)
	{
		if (*text == '\n')
		{
			text++;
			*style++ = 'A';
			at_col0 = true;
			continue;
		}
		else if (*text == '\"')
		{
			text++;
			*style++ = 'S';

			int len = ParseString(text, t_end, style);
			text += len;
			style += len;
			at_col0 = false;
			continue;
		}
		else if (at_col0 && *text == '<')  // Tags
		{
			text++;
			*style++ = 'T';

			int len = ParseTag(text, t_end, style);
			text += len;
			style += len;
			at_col0 = false;
			continue;
		}
		else if (at_col0 && *text == '#')  // Directives
		{
			text++;
			*style++ = 'D';

			int len = ParseUntilEOL(text, t_end, style, 'D');
			text += len;
			style += len;
			at_col0 = false;
			continue;
		}
		else if (strncmp(text, "//", 2) == 0 && text+1 < t_end)
		{
			text += 2;
			*style++ = 'C';
			*style++ = 'C';

			int len = ParseComment(text, t_end, style);
			text += len;
			style += len;
			at_col0 = false;
			continue;
		}
		else if (at_col0 && *text == '[')
		{
			text++;
			*style++ = 'E';

			int len = ParseEntry(text, t_end, style);
			text += len;
			style += len;
			at_col0 = false;
			continue;
		}
#if 0
		else if (*text == '(')  // Brackets
		{
			text++;
			*style++ = 'P';

			int len = ParseBrackets(text, t_end, style);
			text += len;
			style += len;
			at_col0 = false;
			continue;
		}
#endif
		else if (isdigit(*text) || (*text == '-' && text+1 < t_end && isdigit(text[1])))
		{
			text++;
			*style++ = 'N';

			int len = ParseNumber(text, t_end, style);
			text += len;
			style += len;
			at_col0 = false;
			continue;
		}
		else if (isalpha(*text) || *text == '_' || *text == '#')
		{
			int len = ParseKeyword(text, t_end, style);
			text += len;
			style += len;
			at_col0 = false;
			continue;
		}

		text++;
		*style++ = 'A';
		at_col0 = false;
	}
}

int W_Editor::ParseString(const char *text, const char *t_end, char *style)
{
	const char *t_orig = text;

	while (text < t_end)
	{
		if (*text == '\"')
		{
			// End-quote of string
			text++;
			*style++ = 'S';
			break;
		}

		if (text[0] == '\\' && text+1 < t_end)
		{
			// Escape sequence, including \"
			text += 2;
			*style++ = 'B';
			*style++ = 'B';
			continue;
		}

		text++;
		*style++ = 'S';
	}

	return (text - t_orig);
}

int W_Editor::ParseUntilEOL(const char *text, const char *t_end, char *style,
	char context)
{
	const char *t_orig = text;

	while (text < t_end)
	{
		if (*text == '\n')
			break;

		text++;
		*style++ = context;
	}

	return (text - t_orig);
}

int W_Editor::ParseComment(const char *text, const char *t_end, char *style)
{
	const char *t_orig = text;

	while (text < t_end)
	{
		if (*text == '\n')
			break;

		text++;
		*style++ = 'C';
	}

	return (text - t_orig);
}

int W_Editor::ParseTag(const char *text, const char *t_end, char *style)
{
	const char *t_orig = text;

	while (text < t_end)
	{
///		if (*text == '\n')
///			break;

		if (*text == '>')
		{
			text++;
			*style++ = 'T';
			break;
		}

		text++;
		*style++ = 'T';
	}

	return (text - t_orig);
}

int W_Editor::ParseEntry(const char *text, const char *t_end, char *style)
{
	const char *t_orig = text;

	while (text < t_end)
	{
///		if (*text == '\n')
///			break;

		if (*text == ']')
		{
			text++;
			*style++ = 'E';
			break;
		}

		text++;
		*style++ = 'E';
	}

	return (text - t_orig);
}

int W_Editor::ParseBrackets(const char *text, const char *t_end, char *style)
{
	const char *t_orig = text;

	while (text < t_end)
	{
///		if (*text == '\n')
///			break;

		if (*text == ')')
		{
			text++;
			*style++ = 'P';
			break;
		}

		text++;
		*style++ = 'P';
	}

	return (text - t_orig);
}

int W_Editor::ParseNumber(const char *text, const char *t_end, char *style)
{
	const char *t_orig = text;

	while (text < t_end)
	{
		if (*text == '%')
		{
			text++;
			*style++ = 'N';
			break;
		}

		if (! isdigit(*text) && *text != '.')
			break;

		text++;
		*style++ = 'N';
	}

	return (text - t_orig);
}

int W_Editor::ParseKeyword(const char *text, const char *t_end, char *style)
{
	const char *t_orig = text;

	while (text < t_end)
	{
		if (! isalnum(*text) && *text != '_' && *text != '#')
			break;

		text++;
		*style++ = 'K';
	}

	return (text - t_orig);
}


/**
 * \file grafmode.c
 * \brief Load a list of possible graphics modes.
 *
 * Copyright (c) 2011 Brett Reid
 *
 * This work is free software; you can redistribute it and/or modify it
 * under the terms of either:
 *
 * a) the GNU General Public License as published by the Free Software
 *    Foundation, version 2, or
 *
 * b) the "Angband license":
 *    This software may be copied and distributed for educational, research,
 *    and not for profit purposes provided that this copyright and statement
 *    are included in all such copies.  Other copyrights may also apply.
 */
/*
 * Imported from Angband 4.2.0 to support main-cocoa.m for Hengband.  Did not
 * bring over the datafile parsing (datafile.h and datafile.c) so the
 * implementation here is different from that in Angband.
 */
#include "system/angband.h"
#include "system/grafmode.h"
#include "io/files-util.h"
#include "util/angband-files.h"
#include "view/display-messages.h"

#define GFPARSE_HAVE_NOTHING (0)
#define GFPARSE_HAVE_NAME (1)
#define GFPARSE_HAVE_DIR (2)
#define GFPARSE_HAVE_SIZE (4)
#define GFPARSE_HAVE_PREF (8)
#define GFPARSE_HAVE_EXTRA (16)
#define GFPARSE_HAVE_GRAF (32)

typedef struct GrafModeParserState {
    graphics_mode* list;
    char* file_name;
    int dir_len;
    int line_no;
    int stage;
    errr result;
} GrafModeParserState;


/* This is the global state exposed to Angband. */
graphics_mode *graphics_modes;
graphics_mode *current_graphics_mode = NULL;
int graphics_mode_high_id;


static int check_last_mode(GrafModeParserState* pgps) {
    int result = 0;

    if ((pgps->stage & GFPARSE_HAVE_DIR) == 0) {
	result = 1;
	msg_format("no directory set for tile set, %s, in %s",
		   pgps->list->menuname, pgps->file_name);
    }
    if ((pgps->stage & GFPARSE_HAVE_SIZE) == 0) {
	result = 1;
	msg_format("no size set for tile set, %s, in %s",
		   pgps->list->menuname, pgps->file_name);
    }
    if ((pgps->stage & GFPARSE_HAVE_PREF) == 0) {
	result = 1;
	msg_format("no preference file for tile set, %s, in %s",
		   pgps->list->menuname, pgps->file_name);
    }
    if ((pgps->stage & GFPARSE_HAVE_GRAF) == 0) {
	result = 1;
	msg_format("no graf string set for tile set, %s, in %s",
		   pgps->list->menuname, pgps->file_name);
    }
    return result;
}


static void parse_line(GrafModeParserState* pgps, const char* line) {
    static const char whitespc[] = " \t\v\f";
    int offset = 0;
    int stage;

    while (line[offset] && strchr(whitespc, line[offset]) != 0) {
	++offset;
    }
    if (! line[offset] || line[offset] == '#') {
	return;
    }

    if (strncmp(line + offset, "name:", 5) == 0) {
	stage = GFPARSE_HAVE_NAME;
	offset += 5;
    } else if (strncmp(line + offset, "directory:", 10) == 0) {
	stage = GFPARSE_HAVE_DIR;
	offset += 10;
    } else if (strncmp(line + offset, "size:", 5) == 0) {
	stage = GFPARSE_HAVE_SIZE;
	offset += 5;
    } else if (strncmp(line + offset, "pref:", 5) == 0) {
	stage = GFPARSE_HAVE_PREF;
	offset += 5;
    } else if (strncmp(line + offset, "extra:", 6) == 0) {
	stage = GFPARSE_HAVE_EXTRA;
	offset += 6;
    } else if (strncmp(line + offset, "graf:", 5) == 0) {
	stage = GFPARSE_HAVE_GRAF;
	offset += 5;
    } else {
        msg_format("Unexpected data at line %d of %s", pgps->line_no,
		   pgps->file_name);
	pgps->result = 1;
	return;
    }

    if (stage == GFPARSE_HAVE_NAME) {
	graphics_mode *new_mode;

	if (pgps->stage != GFPARSE_HAVE_NOTHING) {
	    if (check_last_mode(pgps) != 0) {
		pgps->result = 1;
	    }
	}

	pgps->stage = GFPARSE_HAVE_NAME;
	new_mode = (graphics_mode *) malloc(sizeof(graphics_mode));
	if (new_mode == 0) {
	    pgps->result = 1;
	    msg_format("failed memory allocation for tile set "
		       "information at line %d of %s", pgps->line_no,
		       pgps->file_name);
	    return;
	}
	new_mode->pNext = pgps->list;
	new_mode->grafID = 0;
	new_mode->alphablend = 0;
	new_mode->overdrawRow = 0;
	new_mode->overdrawMax = 0;
	new_mode->cell_width = 0;
	new_mode->cell_height = 0;
	new_mode->path[0] = '\0';
	new_mode->pref[0] = '\0';
	new_mode->file[0] = '\0';
	new_mode->menuname[0] = '\0';
	new_mode->graf[0] = '\0';
	pgps->list = new_mode;
    } else {
	if (pgps->stage == GFPARSE_HAVE_NOTHING) {
	    pgps->result = 1;
	    msg_format("values set before tile set name given at line %d"
		       " of %s", pgps->line_no, pgps->file_name);
	    return;
	}
	if (pgps->stage & stage) {
	    msg_format("values set more than once for tile set, %s, at line "
		       " %d of %s", pgps->list->menuname, pgps->line_no,
		       pgps->file_name);
	}
    }

    switch (stage) {
    case GFPARSE_HAVE_NAME:
	{
	    unsigned int id;
	    int nscan;

	    if (sscanf(line + offset, "%u:%n", &id, &nscan) == 1) {
		if (id > 255) {
		    pgps->result = 1;
		    msg_format("ID greater than 255 for tile set at line"
			       " %d of %s", pgps->line_no, pgps->file_name);
		} else if (id == GRAPHICS_NONE) {
		    pgps->result = 1;
		    msg_format("ID of tile set matches value, %d, reserved "
			       " for no graphics at line %d of %s",
			       GRAPHICS_NONE, pgps->line_no, pgps->file_name);
		} else {
		    graphics_mode *mode = pgps->list->pNext;

		    while (1) {
			if (mode == 0) {
			    break;
			}
			if (mode->grafID == id) {
			    pgps->result = 1;
			    msg_format("ID for tile set, %s, at line %d of %s"
				       " is the same as for tile set %s",
				       pgps->list->menuname, pgps->line_no,
				       pgps->file_name, mode->menuname);
			    break;
			}
			mode = mode->pNext;
		    }
		    pgps->list->grafID = id;
		}
		offset += nscan;
		if (strlen(line + offset) >= sizeof(pgps->list->menuname)) {
		    pgps->result = 1;
		    msg_format("name is too long for tile set at line %d"
			       " of %s", pgps->line_no, pgps->file_name);
		} else if (line[offset] == '\0') {
		    pgps->result = 1;
		    msg_format("empty name for tile set at line %d of %s",
			       pgps->line_no, pgps->file_name);
		} else {
		    strcpy(pgps->list->menuname, line + offset);
		}
	    } else {
		pgps->result = 1;
		msg_format("malformed ID for tile set at line %d of %s",
			   pgps->line_no, pgps->file_name);
	    }
	}
	break;

    case GFPARSE_HAVE_DIR:
	{
	    size_t len = strlen(line + offset);
	    size_t sep_len = strlen(PATH_SEP);

	    if (len >= sizeof(pgps->list->path) ||
		len + pgps->dir_len + sep_len >= sizeof(pgps->list->path)) {
		pgps->result = 1;
		msg_format("directory name is too long for tile set, %s, at"
			   " line %d of %s", pgps->list->menuname,
			   pgps->line_no, pgps->file_name);
	    } else if (line[offset] == '\0') {
		pgps->result = 1;
		msg_format("empty directory name for tile set, %s, at line"
			   " line %d of %s", pgps->list->menuname,
			   pgps->line_no, pgps->file_name);
	    } else {
		/*
		 * Temporarily hack the path to list.txt so it is not necessary
		 * to separately store the base directory for the tile files.
		 */
		char chold = pgps->file_name[pgps->dir_len];

		pgps->file_name[pgps->dir_len] = '\0';
		path_build(
		    pgps->list->path,
		    sizeof(pgps->list->path),
		    pgps->file_name,
		    line + offset
		);
		pgps->file_name[pgps->dir_len] = chold;
	    }
	}
	break;

    case GFPARSE_HAVE_SIZE:
	{
	    unsigned w, h;
	    int nscan;

	    if (sscanf(line + offset, "%u:%u:%n", &w, &h, &nscan) == 2) {
		if (w > 0) {
		    pgps->list->cell_width = w;
		} else {
		    pgps->result = 1;
		    msg_format("zero width for tile set, %s, at line"
			       " %d of %s", pgps->list->menuname,
			       pgps->line_no, pgps->file_name);
		}
		if (h > 0) {
		    pgps->list->cell_height = h;
		} else {
		    pgps->result = 1;
		    msg_format("zero height for tile set, %s, at line"
			       " %d of %s", pgps->list->menuname,
			       pgps->line_no, pgps->file_name);
		}
		offset += nscan;
		if (strlen(line + offset) >= sizeof(pgps->list->file)) {
		    pgps->result = 1;
		    msg_format("file name is too long for tile set, %s,"
			       " at line %d of %s", pgps->list->menuname,
			       pgps->line_no, pgps->file_name);
		} else if (line[offset] == '\0') {
		    pgps->result = 1;
		    msg_format("empty file name for tile set, %s, at line %d"
			       " of %s", pgps->list->menuname, pgps->line_no,
			       pgps->file_name);
		} else {
		    (void) strcpy(pgps->list->file, line + offset);
		}
	    } else {
		pgps->result = 1;
		msg_format("malformed dimensions for tile set, %s, at line"
			   " %d of %s", pgps->list->menuname, pgps->line_no,
			   pgps->file_name);
	    }
	}
	break;

    case GFPARSE_HAVE_PREF:
	if (strlen(line + offset) >= sizeof(pgps->list->pref)) {
	    pgps->result = 1;
	    msg_format("preference file name is too long for tile set, %s, "
		       "at line %d of %s", pgps->list->menuname, pgps->line_no,
		       pgps->file_name);
	} else if (line[offset] == '\0') {
	    pgps->result = 1;
	    msg_format("empty preference file name for tile set, %s, "
		       "at line %d of %s", pgps->list->menuname, pgps->line_no,
		       pgps->file_name);
	} else {
	    strcpy(pgps->list->pref, line + offset);
	}
	break;

    case GFPARSE_HAVE_EXTRA:
	{
	    unsigned int alpha, startdbl, enddbl;
	    int nscan;

	    if (sscanf(line + offset, "%u:%u:%u%n",
		       &alpha, &startdbl, &enddbl, &nscan) == 3 &&
		(line[offset + nscan] == '\0' ||
		 strchr(whitespc, line[offset + nscan]) != 0 ||
		 line[offset + nscan] == '#')) {
		if (startdbl > 255 || enddbl > 255) {
		    pgps->result = 1;
		    msg_format("overdrawMax or overdrawRow is greater than"
			       " 255 for tile set, %s, at line %d of %s",
			       pgps->list->menuname, pgps->line_no,
			       pgps->file_name);
		} else if (enddbl < startdbl) {
		    pgps->result = 1;
		    msg_format("overdrawMax less than overdrawRow for tile"
			       "set, %s, at line %d of %s",
			       pgps->list->menuname, pgps->line_no,
			       pgps->file_name);
		} else {
		    pgps->list->alphablend = (alpha != 0);
		    pgps->list->overdrawRow = startdbl;
		    pgps->list->overdrawMax = enddbl;
		}
	    } else {
		pgps->result = 1;
		msg_format("malformed data for tile set, %s, at line %d of"
			   " %s", pgps->list->menuname, pgps->line_no,
			   pgps->file_name);
	    }
	}
	break;

    case GFPARSE_HAVE_GRAF:
	if (strlen(line + offset) >= sizeof(pgps->list->graf)) {
	    pgps->result = 1;
	    msg_format("graf string is too long for tile set, %s, at line %d"
		       " of %s", pgps->list->menuname, pgps->line_no,
		       pgps->file_name);
	} else if (line[offset] == '\0') {
	    pgps->result = 1;
	    msg_format("empty graf string for tile set, %s, at line %d of %s",
		       pgps->list->menuname, pgps->line_no, pgps->file_name);
	} else {
	    strcpy(pgps->list->graf, line + offset);
	}
	break;
    }

    if (pgps->result == 0) {
	pgps->stage |= stage;
    }
}


static void finish_parse_grafmode(GrafModeParserState* pgps,
				  int transfer_results) {
    /*
     * Check what was read for the last mode parsed, since parse_line did
     * not.
     */
    if (transfer_results) {
	if (pgps->list == 0 || pgps->stage == GFPARSE_HAVE_NOTHING) {
	    msg_format("no graphics modes in %s", pgps->file_name);
	} else {
	    if (check_last_mode(pgps) != 0) {
		transfer_results = 0;
		pgps->result = 1;
	    }
	}
    }

    if (transfer_results) {
	graphics_mode *mode = pgps->list;
	int max = GRAPHICS_NONE;
	int count = 0;
	graphics_mode *new_list;

	while (mode) {
	    if (mode->grafID > max) {
		max = mode->grafID;
	    }
	    ++count;
	    mode = mode->pNext;
	}

	/* Assemble the modes into a contiguous block of memory. */
	new_list = (graphics_mode *)
	    malloc(sizeof(graphics_mode) * (count + 1));
	if (new_list != 0) {
	    int i;

	    mode = pgps->list;
	    for (i = count - 1; i >= 0; --i, mode = mode->pNext) {
		memcpy(&(new_list[i]), mode, sizeof(graphics_mode));
		new_list[i].pNext = &(new_list[i + 1]);
	    }

	    /* Hardcode the no graphics option. */
	    new_list[count].pNext = NULL;
	    new_list[count].grafID = GRAPHICS_NONE;
	    new_list[count].alphablend = 0;
	    new_list[count].overdrawRow = 0;
	    new_list[count].overdrawMax = 0;
	    strncpy(
		new_list[count].pref, "none", sizeof(new_list[count].pref));
	    strncpy(
		new_list[count].path, "", sizeof(new_list[count].path));
	    strncpy(
		new_list[count].file, "", sizeof(new_list[count].file));
	    strncpy(
		new_list[count].menuname,
		"Classic ASCII",
		sizeof(new_list[count].menuname)
	    );
	    strncpy(
		new_list[count].graf, "ascii", sizeof(new_list[count].graf));

	    /* Release the old global state. */
	    close_graphics_modes();

	    graphics_modes = new_list;
	    graphics_mode_high_id = max;
	    /* Set the default graphics mode to be no graphics */
	    current_graphics_mode = &(graphics_modes[count]);
	} else {
	    pgps->result = 1;
	    msg_print("failed memory allocation for new graphics modes");
	}
    }

    /* Release the memory allocated for parsing the file. */
    while (pgps->list != 0) {
	graphics_mode *mode = pgps->list;

	pgps->list = mode->pNext;
	free(mode);
    }
}


bool init_graphics_modes(void) {
    char buf[1024];
    char line[1024];
    GrafModeParserState gps = { 0, buf, 0, 0, GFPARSE_HAVE_NOTHING, 0 };
    FILE *f;

    /* Build the filename */
    path_build(line, sizeof(line), ANGBAND_DIR_XTRA, "graf");
    gps.dir_len = strlen(line);
    path_build(buf, sizeof(buf), line, "list.txt");

    f = angband_fopen(buf, FileOpenMode::READ);
    if (!f) {
	msg_format("Cannot open '%s'.", buf);
	gps.result = 1;
    } else {
	while (angband_fgets(f, line, sizeof line) == 0) {
	    ++gps.line_no;
	    parse_line(&gps, line);
	    if (gps.result != 0) {
		break;
	    }
	}

	finish_parse_grafmode(&gps, gps.result == 0);
	angband_fclose(f);
    }

    /* Result */
    return gps.result == 0;
}


void close_graphics_modes(void) {
    if (graphics_modes) {
	free(graphics_modes);
	graphics_modes = NULL;
    }
    current_graphics_mode = NULL;
}


graphics_mode *get_graphics_mode(byte id) {
    graphics_mode *test = graphics_modes;
    while (test) {
	if (test->grafID == id) {
	    return test;
	}
	test = test->pNext;
    }
    return NULL;
}

﻿#pragma once

#include "system/angband.h"

/*!
 * チートオプションの最大数 / Number of cheating options
 */
#define CHEAT_MAX 10

/*
 * Available "options"
 *	- Address of actual option variable (or NULL)
 *	- Normal Value (TRUE or FALSE)
 *	- Option Page Number (or zero)
 *	- Savefile Set (or zero)
 *	- Savefile Bit in that set
 *	- Textual name (or NULL)
 *	- Textual description
 */
typedef struct option_type
{
	bool	*o_var;
	byte	o_norm;
	byte	o_page;
	byte	o_set;
	byte	o_bit;
	concptr	o_text;
	concptr	o_desc;
} option_type;

extern const option_type option_info[];
extern const option_type cheat_info[CHEAT_MAX];
extern const option_type autosave_info[2];

void extract_option_vars(void);
void do_cmd_options_aux(int page, concptr info);
void do_cmd_options(void);

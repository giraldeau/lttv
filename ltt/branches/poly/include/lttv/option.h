/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2003-2004 Michel Dagenais
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License Version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, 
 * MA 02111-1307, USA.
 */

#ifndef OPTION_H
#define OPTION_H

/* Define a new option with a long name (--long_name), a short
   one character name (-c), a descriptive text, the argument type, and a
   pointer to where the argument value will be stored. For an option of
   type LTTV_OPT_NONE, the argument is a boolean value set to true when the
   option is present. The option hook is called if non NULL. */

typedef enum _LttvOptionType
{LTTV_OPT_NONE, LTTV_OPT_STRING, LTTV_OPT_INT, LTTV_OPT_LONG } 
LttvOptionType;

typedef void (*LttvOptionHook)(void *hook_data);

void lttv_option_add(const char *long_name, const char char_name,
		const char *description, const char *arg_description,
		const LttvOptionType t, void *p,
		const LttvOptionHook h, void *hook_data);


/* Remove an option */

void lttv_option_remove(const char *long_name);


/* Parse command line options. It is possible to add options (through the
   hooks being called) while the parsing is done. The new options will be
   used for subsequent command line arguments. */

void lttv_option_parse(int argc, char **argv);

void lttv_option_show_help(void);

#endif // OPTION_H

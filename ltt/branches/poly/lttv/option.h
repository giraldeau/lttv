#ifndef OPTION_H
#define OPTION_H

/* Options parsing mechanism */

/* Define a new command line option with a long name (--long_name), a short
   one character name (-c), a descriptive text, the argument type, and a
   pointer to where the argument value will be stored. For an option of
   type LTTV_OPT_NONE, the argument is a boolean value set to true when the
   option is present. */

/* Initial draft by Michel Dagenais May 2003
 * Reworked by Mathieu Desnoyers, May 2003
 */

typedef enum _lttv_option_type 
{LTTV_OPT_NONE, LTTV_OPT_STRING, LTTV_OPT_INT, LTTV_OPT_LONG } 
lttv_option_type;

typedef void (*lttv_option_hook)(void *hook_data);

void lttv_option_add(const char *long_name, const char char_name,
		const char *description, const char *argDescription,
		const lttv_option_type t, void *p,
		const lttv_option_hook h, void *hook_data);



#endif // OPTION_H

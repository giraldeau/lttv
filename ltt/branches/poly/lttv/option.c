#include <popt.h>

#include <lttv/hook.h>
#include "lttv.h"
#include "option.h"

/* Extensible array of popt command line options. Modules add options as
   they are loaded and initialized. */

typedef struct _lttv_option {
  lttv_option_hook hook;
  void *hook_data;
} lttv_option;

static GArray *lttv_options_command;

static GArray *lttv_options_command_popt;

// unneeded   static lttv_key *key ;

static int command_argc;

static char **command_argv;

/* Lists of hooks to be called at different places */

static lttv_hooks
  *hooks_options_before,
  *hooks_options_after;

static gboolean init_done = FALSE;

void lttv_options_command_parse(void *hook_data, void *call_data);


void lttv_option_init(int argc, char **argv) {

  lttv_hooks *hooks_init_after;

  if(init_done) return;
  else init_done = TRUE;

  command_argc = argc;
  command_argv = argv;

  hooks_options_before = lttv_hooks_new();
  hooks_options_after = lttv_hooks_new();

  lttv_attributes_set_pointer_pathname(lttv_global_attributes(), 
      "hooks/options/before", hooks_options_before);

  lttv_attributes_set_pointer_pathname(lttv_global_attributes(),
      "hooks/options/after",  hooks_options_after);

  lttv_options_command_popt = g_array_new(0,0,sizeof(struct poptOption));
  lttv_options_command = g_array_new(0,0,sizeof(lttv_option));

  hooks_init_after = lttv_attributes_get_pointer_pathname(lttv_global_attributes(),
		  "hooks/init/after");
  lttv_hooks_add(hooks_init_after, lttv_options_command_parse, NULL);

}

void lttv_option_destroy() {

  struct poptOption *poption;

  int i;
  
  for(i=0; i < lttv_options_command_popt->len ; i++) {
    poption = &g_array_index (lttv_options_command_popt, struct poptOption, i);

    g_free((gpointer)poption->longName);
    g_free((gpointer)poption->descrip);
    g_free((gpointer)poption->argDescrip);
  }
  g_array_free(lttv_options_command_popt,TRUE) ;
  g_array_free(lttv_options_command,TRUE) ;

  lttv_attributes_set_pointer_pathname(lttv_global_attributes(), 
      "hooks/options/before", NULL);

  lttv_attributes_set_pointer_pathname(lttv_global_attributes(),
      "hooks/options/after",  NULL);

  lttv_hooks_destroy(hooks_options_before);
  lttv_hooks_destroy(hooks_options_after);

}


static int poptToLTT[] = { 
  POPT_ARG_NONE, POPT_ARG_STRING, POPT_ARG_INT, POPT_ARG_LONG
};


void lttv_option_add(const char *long_name, const char char_name,
		const char *description, const char *argDescription,
		const lttv_option_type t, void *p, 
		const lttv_option_hook h, void *hook_data)
{
  struct poptOption poption;

  lttv_option option;

  poption.longName = (char *)g_strdup(long_name);
  poption.shortName = char_name;
  poption.descrip = (char *)g_strdup(description);
  poption.argDescrip = (char *)g_strdup(argDescription);
  poption.argInfo = poptToLTT[t];
  poption.arg = p;
  poption.val = lttv_options_command->len + 1;

  option.hook = h;
  option.hook_data = hook_data;

  g_array_append_val(lttv_options_command_popt,poption);
  g_array_append_val(lttv_options_command,option);
}


static struct poptOption endOption = { NULL, '\0', 0, NULL, 0};

/* As we may load modules in the hooks called for argument processing,
 * we have to recreate the argument context each time the
 * lttv_options_command_popt is modified. This way we will be able to
 * parse arguments defined by the modules
 */

void lttv_options_command_parse(void *hook_data, void *call_data) 
{
  int rc;
  int lastrc;
  poptContext c;
  lttv_option *option;

  lttv_hooks_call(hooks_options_before,NULL);
  /* Always add then remove the null option around the get context */
  g_array_append_val(lttv_options_command_popt, endOption);
  /* Compiler warning caused by const char ** for command_argv in header */
  /* Nothing we can do about it. Header should not put it const. */
  c = poptGetContext("lttv", command_argc, (const char**)command_argv,
      (struct poptOption *)(lttv_options_command_popt->data),0);

  /* We remove the null option here to be able to add options correctly */
  g_array_remove_index(lttv_options_command_popt,
		  lttv_options_command_popt->len - 1);

  /* There is no last good offset */
  lastrc = -1;

  /* Parse options while not end of options event */
  while((rc = poptGetNextOpt(c)) != -1) {
	  
    if(rc == POPT_ERROR_BADOPT) {
      /* We need to redo the context with information added by modules */
      g_array_append_val(lttv_options_command_popt, endOption);
      poptFreeContext(c);  
      c = poptGetContext("lttv", command_argc, (const char**)command_argv,
          (struct poptOption *)lttv_options_command_popt->data,0);
      g_array_remove_index(lttv_options_command_popt,
		  lttv_options_command_popt->len);

      /* Cut out the already parsed elements */
      if(lastrc != -1)
        while(poptGetNextOpt(c) != lastrc) { } ;
      
      /* Get the same option once again */
      g_assert(rc = poptGetNextOpt(c) != -1) ;
      if(rc == POPT_ERROR_BADOPT) {
        /* If here again we have a parsing error with all context info ok,
	 * then there is a problem in the arguments themself, give up */
        g_critical("option %s: %s", poptBadOption(c,0), poptStrerror(rc));
	break ;
      }
    }
    
    /* Remember this offset as the last good option value */
    lastrc = rc;

    /* Execute the hook registered with this option */
    option = ((lttv_option *)lttv_options_command->data) + rc - 1;
    if(option->hook != NULL) option->hook(option->hook_data);
 
  }

  poptFreeContext(c);

  lttv_hooks_call(hooks_options_after,NULL);
  
}


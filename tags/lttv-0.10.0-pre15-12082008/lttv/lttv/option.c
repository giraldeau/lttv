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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <popt.h>
#include <glib.h>
#include <lttv/module.h>
#include <lttv/option.h>

typedef struct _LttvOption {
  char *long_name;
  char char_name;
  char *description;
  char *arg_description;
  LttvOptionType t;
  gpointer p;
  LttvOptionHook hook;
  gpointer hook_data;

  /* Keep the order of addition */
  guint val;
} LttvOption;

GHashTable *options;


static void
list_options(gpointer key, gpointer value, gpointer user_data)
{
  GPtrArray *list = (GPtrArray *)user_data;
  LttvOption *option = (LttvOption *)value;

  if(list->len < option->val)
    g_ptr_array_set_size(list, option->val);
  list->pdata[option->val-1] = option;
}


static void
free_option(LttvOption *option)
{
  g_free(option->long_name);
  g_free(option->description);
  g_free(option->arg_description);
  g_free(option);
}


void lttv_option_add(const char *long_name, const char char_name,
    const char *description, const char *arg_description,
    const LttvOptionType t, void *p,
    const LttvOptionHook h, void *hook_data)
{
  LttvOption *option;

  g_log(G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "Add option %s", long_name);
  if(g_hash_table_lookup(options, long_name) != NULL) {
    g_warning("duplicate option");
    return;
  }

  option = g_new(LttvOption, 1);
  option->long_name = g_strdup(long_name);
  option->char_name = char_name;
  option->description = g_strdup(description);
  option->arg_description = g_strdup(arg_description);
  option->t = t;
  option->p = p;
  option->hook = h;
  option->hook_data = hook_data;
  option->val = g_hash_table_size(options) + 1;
  g_hash_table_insert(options, option->long_name, option);
}


void 
lttv_option_remove(const char *long_name) 
{
  LttvOption *option = g_hash_table_lookup(options, long_name);

  g_log(G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "Remove option %s", long_name);
  if(option == NULL) {
    g_warning("trying to remove unknown option %s", long_name);
    return;
  }
  g_hash_table_remove(options, long_name);
  free_option(option);
}


static int poptToLTT[] = { 
  POPT_ARG_NONE, POPT_ARG_STRING, POPT_ARG_INT, POPT_ARG_LONG
};

static struct poptOption endOption = { NULL, '\0', 0, NULL, 0, NULL, NULL };


static void 
build_popts(GPtrArray **plist, struct poptOption **ppopts, poptContext *pc,
    int argc, char **argv)
{
  LttvOption *option;

  GPtrArray *list;

  struct poptOption *popts;

  poptContext c;

  guint i;

  list = g_ptr_array_sized_new(g_hash_table_size(options));

  g_hash_table_foreach(options, list_options, list);

  /* Build a popt options array from our list */

  popts = g_new(struct poptOption, list->len + 1);

  /* add the options in the reverse order, so last additions are parsed first */
  for(i = 0 ; i < list->len ; i++) {
    guint reverse_i = list->len-1-i;
    option = (LttvOption *)list->pdata[i];
    popts[reverse_i].longName = option->long_name;
    popts[reverse_i].shortName = option->char_name;
    popts[reverse_i].descrip = option->description;
    popts[reverse_i].argDescrip = option->arg_description;
    popts[reverse_i].argInfo = poptToLTT[option->t];
    popts[reverse_i].arg = option->p;
    popts[reverse_i].val = option->val;
  }

  /* Terminate the array for popt and create the context */

  popts[list->len] = endOption;
  c = poptGetContext(argv[0], argc, (const char**)argv, popts, 0);

  *plist = list;
  *ppopts = popts;
  *pc = c;
}


static void 
destroy_popts(GPtrArray **plist, struct poptOption **ppopts, poptContext *pc)
{
  g_ptr_array_free(*plist, TRUE); *plist = NULL;
  g_free(*ppopts); *ppopts = NULL;
  poptFreeContext(*pc);  
}


void lttv_option_parse(int argc, char **argv)
{
  GPtrArray *list;

  LttvOption *option;

  int i, rc, first_arg;

  struct poptOption *popts;

  poptContext c;

  i = 0;

  first_arg = 0;

  guint hash_size = 0;

  build_popts(&list, &popts, &c, argc, argv);

  /* Parse options while not end of options event */

  while((rc = poptGetNextOpt(c)) != -1) {

    /* The option was recognized and the rc value returned is the argument
       position in the array. Call the associated hook if present. */
  
    if(rc > 0) {
      option = (LttvOption *)(list->pdata[rc - 1]);
      g_log(G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "Option %s encountered", 
          option->long_name);
      hash_size = g_hash_table_size(options);
      if(option->hook != NULL) { 
        g_log(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Option %s hook called", 
            option->long_name);
        option->hook(option->hook_data);
      }
      i++;

      /* If the size of the option hash changed, add new options
       * right now. It resolves the conflict of multiple same short
       * option use.
       */
      if(hash_size != g_hash_table_size(options)) {
        destroy_popts(&list, &popts, &c);
        build_popts(&list, &popts, &c, argc, argv);

        /* Get back to the same argument */

        first_arg = i;
        for(i = 0; i < first_arg; i++) {
          rc = poptGetNextOpt(c);
          option = (LttvOption *)(list->pdata[rc - 1]);
          g_log(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Option %s rescanned, skipped",
              option->long_name);
        }
      }
    } 

    else if(rc == POPT_ERROR_BADOPT && i != first_arg) {
      g_log(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, 
          "Option %s not recognized, rescan options with new additions",
	  poptBadOption(c,0));

      /* Perhaps this option is newly added, restart parsing */

      destroy_popts(&list, &popts, &c);
      build_popts(&list, &popts, &c, argc, argv);

      /* Get back to the same argument */

      first_arg = i;
      for(i = 0; i < first_arg; i++) {
        rc = poptGetNextOpt(c);
        option = (LttvOption *)(list->pdata[rc - 1]);
        g_log(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Option %s rescanned, skipped",
            option->long_name);
      }
    }

    else {

      /* The option has some error and it is not because this is a newly
         added option not recognized. */

      g_error("option %s: %s", poptBadOption(c,0), poptStrerror(rc));
      break;
    }
    
  }

  destroy_popts(&list, &popts, &c);
}

/* CHECK */
static void show_help(LttvOption *option)
{
  printf("--%s  -%c  argument: %s\n" , option->long_name,
																			option->char_name,
																			option->arg_description);
  printf("                     %s\n" , option->description);

}

void lttv_option_show_help(void)
{
  GPtrArray *list = g_ptr_array_new();

  guint i;

  g_hash_table_foreach(options, list_options, list);

	printf("Built-in commands available:\n");
	printf("\n");

  for(i = 0 ; i < list->len ; i++) {
    show_help((LttvOption *)list->pdata[i]);
  }
  g_ptr_array_free(list, TRUE);
}

static void init()
{
  g_log(G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "Init option.c");
  options = g_hash_table_new(g_str_hash, g_str_equal);
}


static void destroy()
{
  GPtrArray *list = g_ptr_array_new();

  guint i;

  g_log(G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "Destroy option.c");
  g_hash_table_foreach(options, list_options, list);
  g_hash_table_destroy(options);

  for(i = 0 ; i < list->len ; i++) {
    free_option((LttvOption *)list->pdata[i]);
  }
  g_ptr_array_free(list, TRUE);
}

LTTV_MODULE("option", "Command line options processing", \
    "Functions to add, remove and parse command line options", \
    init, destroy)

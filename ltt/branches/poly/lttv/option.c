
#include <popt.h>
#include <glib.h>
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
} LttvOption;

GHashTable *options;


static void
list_options(gpointer key, gpointer value, gpointer user_data)
{
  g_ptr_array_add((GPtrArray *)user_data, value);
}


static void
free_option(LttvOption *option)
{
  g_free(option->long_name);
  g_free(option->description);
  g_free(option->arg_description);
  g_free(option);
}


void lttv_option_init(int argc, char **argv)
{
  options = g_hash_table_new(g_str_hash, g_str_equal);
}


void lttv_option_destroy()
{
  LttvOption option;

  GPtrArray *list = g_ptr_array_new();

  int i;

  g_hash_table_foreach(options, list_options, list);
  g_hash_table_destroy(options);

  for(i = 0 ; i < list->len ; i++) {
    free_option((LttvOption *)list->pdata[i]);
  }
  g_ptr_array_free(list, TRUE);
}


void lttv_option_add(const char *long_name, const char char_name,
    const char *description, const char *arg_description,
    const LttvOptionType t, void *p,
    const LttvOptionHook h, void *hook_data)
{
  LttvOption *option;

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
  g_hash_table_insert(options, option->long_name, option);
}


void 
lttv_option_remove(const char *long_name) 
{
  LttvOption *option = g_hash_table_lookup(options, long_name);

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

static struct poptOption endOption = { NULL, '\0', 0, NULL, 0};


static void 
build_popts(GPtrArray **plist, struct poptOption **ppopts, poptContext *pc,
    int argc, char **argv)
{
  LttvOption *option;

  GPtrArray *list;

  struct poptOption *popts;

  poptContext c;

  guint i;

  list = g_ptr_array_new();

  g_hash_table_foreach(options, list_options, list);

  /* Build a popt options array from our list */

  popts = g_new(struct poptOption, list->len + 1);

  for(i = 0 ; i < list->len ; i++) {
    option = (LttvOption *)list->pdata[i];
    popts[i].longName = option->long_name;
    popts[i].shortName = option->char_name;
    popts[i].descrip = option->description;
    popts[i].argDescrip = option->arg_description;
    popts[i].argInfo = poptToLTT[option->t];
    popts[i].arg = option->p;
    popts[i].val = i + 1;
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

  build_popts(&list, &popts, &c, argc, argv);

  /* Parse options while not end of options event */

  while((rc = poptGetNextOpt(c)) != -1) {

    /* The option was recognized and the rc value returned is the argument
       position in the array. Call the associated hook if present. */
  
    if(rc > 0) {
      option = (LttvOption *)(list->pdata[rc - 1]);
      if(option->hook != NULL) option->hook(option->hook_data);
      i++;
    } 

    else if(rc == POPT_ERROR_BADOPT && i != first_arg) {

      /* Perhaps this option is newly added, restart parsing */

      destroy_popts(&list, &popts, &c);
      build_popts(&list, &popts, &c, argc, argv);

      /* Get back to the same argument */

      first_arg = i;
      for(i = 0; i < first_arg; i++) poptGetNextOpt(c);
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

static void show_help(LttvOption *option)
{
  printf("--%s  -%c  argument: %s\n" , option->long_name,
																			option->char_name,
																			option->arg_description);
  printf("                     %s\n" , option->description);

}

void lttv_option_show_help(void)
{
	LttvOption option;

  GPtrArray *list = g_ptr_array_new();

  int i;

  g_hash_table_foreach(options, list_options, list);

	printf("Built-in commands available:\n");
	printf("\n");

  for(i = 0 ; i < list->len ; i++) {
    show_help((LttvOption *)list->pdata[i]);
  }
  g_ptr_array_free(list, TRUE);


}

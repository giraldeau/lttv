
#include <lttv/hook.h>
#include <lttv/module.h>

#include <lttv/lttv.h>
#include <lttv/attribute.h>
#include <lttv/option.h>

#include <lttv/traceSet.h>

#include <ltt/trace.h>
/* The main program maintains a few central data structures and relies
   on modules for the rest. These data structures may be accessed by modules
   through an exported API */

/* Extensible array of popt command line options. Modules add options as
   they are loaded and initialized. */


static lttv_attributes *attributes_global;

static lttv_hooks
  *hooks_init_after,
  *hooks_program_before,
  *hooks_program_main,
  *hooks_program_after;

// trace sets has to be put one in each new window_traceset
static lttv_trace_set *traces;

static char *aModule, *aPath, *aTrace;

static int aArgc;

static char **aArgv;

static void lttv_module_option(void *hook_data);

static void lttv_module_path_option(void *hook_data);

static void lttv_trace_option(void *hook_data);

#ifdef MEMDEBUG
extern struct GMemVTable *glib_mem_profiler_table;
#endif

/* Since everything is done in modules, the main program only takes care
   of the infrastructure. */

int main(int argc, char **argv) {

  aArgc = argc;
  aArgv = argv;

#ifdef MEMDEBUG
  g_mem_set_vtable(glib_mem_profiler_table);
  g_message("Memory summary before main");
  g_mem_profile();
#endif

  attributes_global = lttv_attributes_new();

//  traces = lttv_trace_set_new();
//  lttv_attributes_set_pointer_pathname(attributes_global, "trace_set/default", traces);

  /* Initialize the hooks */

  hooks_init_after = lttv_hooks_new();
  lttv_attributes_set_pointer_pathname(attributes_global, "hooks/init/after", 
      hooks_init_after);


  hooks_program_before = lttv_hooks_new();
  lttv_attributes_set_pointer_pathname(attributes_global, "hooks/program/before", 
      hooks_program_before);

  hooks_program_main = lttv_hooks_new();
  lttv_attributes_set_pointer_pathname(attributes_global, "hooks/program/main", 
      hooks_program_main);

  hooks_program_after = lttv_hooks_new();
  lttv_attributes_set_pointer_pathname(attributes_global, "hooks/program/after", 
      hooks_program_after);

  /* Initialize the command line options processing */

  lttv_option_init(argc,argv);
  lttv_module_init(argc,argv);
  // FIXME lttv_analyse_init(argc,argv);

  /* Initialize the module loading */

  lttv_module_path_add("/usr/lib/lttv/plugins");

  /* Add some built-in options */

  lttv_option_add("module",'m', "load a module", "name of module to load", 
      LTTV_OPT_STRING, &aModule, lttv_module_option, NULL);
 
  lttv_option_add("modules-path", 'L', 
      "add a directory to the module search path", 
      "directory to add to the path", LTTV_OPT_STRING, &aPath, 
      lttv_module_path_option, NULL);

  lttv_option_add("trace", 't', 
      "add a trace to the trace set to analyse", 
      "pathname of the directory containing the trace", 
      LTTV_OPT_STRING, &aTrace, lttv_trace_option, NULL);

  lttv_hooks_call(hooks_init_after, NULL);

  lttv_hooks_call(hooks_program_before, NULL);
  lttv_hooks_call(hooks_program_main, NULL);
  lttv_hooks_call(hooks_program_after, NULL);

  /* Finalize the command line options processing */
  lttv_module_destroy();
  lttv_option_destroy();
  // FIXME lttv_analyse_destroy();
  lttv_attributes_destroy(attributes_global);

#ifdef MEMDEBUG
    g_message("Memory summary after main");
    g_mem_profile();
#endif

  
}

lttv_attributes *lttv_global_attributes()
{
	return attributes_global;
}


void lttv_module_option(void *hook_data)
{ 
  lttv_module_load(aModule,aArgc,aArgv,STANDALONE);
}


void lttv_module_path_option(void *hook_data)
{
  lttv_module_path_add(aPath);
}


void lttv_trace_option(void *hook_data)
{ 
//  lttv_trace *trace;

//  trace = lttv_trace_open(aTrace);
//  if(trace == NULL) g_critical("cannot open trace %s", aTrace);
//  lttv_trace_set_add(traces, trace);
}



/* The statistics are for a complete time interval. These structures differ
   from the system state since they relate to static components of the 
   system (all processes which existed instead of just the currently 
   existing processes). 

   At each level are kept statistics as well as pointers to subcomponents.
   During the trace processing, the statistics are accumulated at the
   lowest level component level. Then, in the "after" hooks of the processing,
   these statistics are summed to get the values at higher levels (process,
   CPU, trace, traceSet). */

typedef struct _lttv_trace_set_stats {
  lttv_attributes *stats;
  lttv_attributes *traces;
} lttv_trace_set_stats;

typedef struct _lttv_trace_stats {
  lttv_attributes *stats;
  lttv_attributes *CPUs;
  lttv_attributes *processes;
  lttv_attributes *int_calls;
  lttv_attributes *block_devices;
} lttv_trace_stats;

typedef struct _lttv_cpu_stats {
  lttv_attributes *processes;
  lttv_attributes *int_calls;
  lttv_attributes *block_devices;
} lttv_cpu_stats;

typedef struct _lttv_process_identity {
  char *names;
  lttv_time entry, exit;
} lttv_process_identity;

typedef struct _lttv_process_stats {
  lttv_attributes *stats;
  lttv_process_identify *p;
  lttv_attributes *int_calls;
  lttv_attributes *block_devices;
} lttv_process_stats;

typedef lttv_int_stats {
  lttv_attributes *stats;
  lttv_int_type type;
} lttv_int_stats;

typedef lttv_block_device_stats {
  lttv_attributes *stats;
} lttv_block_device_stats;


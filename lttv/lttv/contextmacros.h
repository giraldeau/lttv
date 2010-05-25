/* This file is part of the Linux Trace Toolkit trace reading library
 * Copyright (C) 2004 Mathieu Desnoyers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License Version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/* This is a header that contains macro helpers to give an object-oriented
 * access to the Trace context, state and statistics.
 */


#include <ltt/trace.h>
#include <lttv/tracecontext.h>
#include <lttv/state.h>
#include <lttv/stats.h>

/************ TracesetContext get methods ***************/

/* LTTV_TRACESET_CONTEXT_GET_TRACE_CONTEXT
 *
 * Input : LttvTracesetContext *tsc
 *         int                  trace number  (0 .. num_traces -1)
 * returns : (LttvTraceContext *)
 */
#define LTTV_TRACESET_CONTEXT_GET_TRACE_CONTEXT(tsc, trace_num)\
		(tsc->traces[trace_num])

/* LTTV_TRACESET_CONTEXT_GET_NUM_TRACES
 *
 * Input : LttvTracesetContext *tsc
 * returns : (guint)     the number of traces
 */
#define LTTV_TRACESET_CONTEXT_GET_NUM_TRACES(tsc)\
		(lttv_traceset_number(tsc->ts))

/* LTTV_TRACESET_CONTEXT_GET_TRACESET_STATS
 *
 * Input : LttvTracesetContext *tsc
 * returns : (LttvTracesetStats*)
 */
#define LTTV_TRACESET_CONTEXT_GET_TRACESET_STATS(tsc)\
		((LttvTracesetStats*)tsc)


/************ TraceContext get methods ***************/

/* LTTV_TRACE_CONTEXT_GET_TRACESET_CONTEXT
 *
 * Input : LttvTraceContext *tc
 * returns : (LttvTracesetContext *)
 */
#define LTTV_TRACE_CONTEXT_GET_TRACESET_CONTEXT(tc)\
		(tc->ts_context)

/* LTTV_TRACE_CONTEXT_GET_TRACE_INDEX
 *
 * Input : LttvTraceContext *tc
 * returns : (guint)      trace context index in its traceset context.
 */
#define LTTV_TRACE_CONTEXT_GET_TRACE_INDEX(tc)\
		(tc->index)



/* LTTV_TRACE_CONTEXT_GET_TRACE_STATE
 *
 * Input : LttvTraceContext *tc
 * returns : (LttvTraceState *)
 */
#define LTTV_TRACE_CONTEXT_GET_TRACE_STATE(tc)\
		((LttvTraceState*)tc)

/* LTTV_TRACE_CONTEXT_GET_TRACE_STATS
 *
 * Input : LttvTraceContext *tc
 * returns : (LttvTraceStats *)
 */
#define LTTV_TRACE_CONTEXT_GET_TRACE_STATS(tc)\
		((LttvTraceStats*)tc)

/* LTTV_TRACE_CONTEXT_GET_CPU_TRACEFILE_CONTEXT
 *
 * Input : LttvTraceContext *tc
 *         guint             cpu_index (0 .. number_cpu-1)
 * returns : (LttvTracefileContext*)
 */
#define LTTV_TRACE_CONTEXT_GET_CPU_TRACEFILE_CONTEXT(tc, cpu_index)\
		( tc->tracefiles[cpu_index + ltt_trace_control_tracefile_number(tc->t)] )

/* LTTV_TRACE_CONTEXT_GET_NUMBER_CPU
 *
 * input : LttvTraceContext *tc
 * returns : (guint)    number_cpu
 */
#define LTTV_TRACE_CONTEXT_GET_NUMBER_CPU(tc)\
		( ltt_trace_per_cpu_tracefile_number(tc->t) )


/* LTTV_TRACE_CONTEXT_GET_CONTROL_TRACEFILE_CONTEXT
 *
 * Input : LttvTraceContext *tc
 *         guint             control_index (0 .. number_control-1)
 * returns : (LttvTracefileContext*)
 */
#define LTTV_TRACE_CONTEXT_GET_CONTROL_TRACEFILE_CONTEXT(tc, control_index)\
		(tc->tracefiles[control_index])

/* LTTV_TRACE_CONTEXT_GET_NUMBER_CONTROL
 *
 * Input : LttvTraceContext *tc
 * returns : (guint)    number_control
 */
#define LTTV_TRACE_CONTEXT_GET_NUMBER_CONTROL(tc)\
		( ltt_trace_control_tracefile_number(tc->t) )

/* LTTV_TRACE_CONTEXT_GET_TRACE
 *
 * Input : LttvTraceContext *tc
 * returns : (LttvTrace*)
 *
 * NOTE : see traceset.h for LttvTrace methods
 */
#define LTTV_TRACE_CONTEXT_GET_TRACE(tc)\
		(tc->vt)





/************ TracefileContext get methods ***************/

/* LTTV_TRACEFILE_CONTEXT_GET_TRACE_CONTEXT
 *
 * Input : LttvTracefileContext *tfc
 * returns : (LttvTraceContext*)
 */
#define LTTV_TRACEFILE_CONTEXT_GET_TRACE_CONTEXT(tfc)\
		(tfc->t_context)

/* LTTV_TRACEFILE_CONTEXT_GET_EVENT
 *
 * Input : LttvTracefileContext *tfc
 * returns : (LttEvent *)
 */
#define LTTV_TRACEFILE_CONTEXT_GET_EVENT(tfc)\
		(tfc->e)

/* LTTV_TRACEFILE_CONTEXT_GET_TRACEFILE_STATE
 *
 * Input : LttvTracefileContext *tfc
 * returns : (LttvTracefileState *)
 */
#define LTTV_TRACEFILE_CONTEXT_GET_TRACEFILE_STATE(tfc)\
		((LttvTracefileState*)tfc)

/* LTTV_TRACEFILE_CONTEXT_GET_TRACEFILE_STATS
 *
 * Input : LttvTracefileContext *tfc
 * returns : (LttvTracefileStats *)
 */
#define LTTV_TRACEFILE_CONTEXT_GET_TRACEFILE_STATS(tfc)\
		((LttvTracefileStats*)tfc)

/* LTTV_TRACEFILE_CONTEXT_GET_TRACEFILE_INDEX
 * 
 * Returns the tracefile index.
 *
 * It checks if it's a control tracefile or a cpu tracefile and returns the
 * cpu_index or control_index, depending of the case.
 * 
 * Input : LttvTracefileContext *tfc
 * returns : (guint)     cpu_index or control_index.
 */
#define LTTV_TRACEFILE_CONTEXT_GET_TRACEFILE_INDEX(tfc)\
		(tfc->control?\
			tfc->index:\
			tfc->index-ltt_trace_control_tracefile_number(tfc->t_context->t))



/************ TraceState get methods ***************/

/* LTTV_TRACE_STATE_GET_TRACE_CONTEXT
 *
 * Input : LttvTraceState *tse
 * returns : (LttvTraceContext*)
 *
 */
#define LTTV_TRACE_STATE_GET_TRACE_CONTEXT(tse)\
		((LttvTraceContext*)tse)

/* LTTV_TRACE_STATE_GET_EVENTTYPE_NAME
 *
 * Input : LttvTraceState *tse
 *         guint           eventtype_number
 * returns : (GQuark)
 *
 * NOTE : use g_quark_to_string to convert a GQuark into a static char *
 */
#define LTTV_TRACE_STATE_GET_EVENTTYPE_NAME(tse, eventtype_number)\
		(tse->eventtype_names[eventtype_number])

/* LTTV_TRACE_STATE_GET_SYSCALL_NAME
 *
 * Input : LttvTraceState *tse
 *         guint           syscall_number
 * returns : (GQuark)
 *
 * NOTE : use g_quark_to_string to convert a GQuark into a static char *
 */
#define LTTV_TRACE_STATE_GET_SYSCALL_NAME(tse, syscall_number)\
		(tse->syscall_names[syscall_number])

/* LTTV_TRACE_STATE_GET_TRAP_NAME
 *
 * Input : LttvTraceState *tse
 *         guint           trap_number
 * returns : (GQuark)
 *
 * NOTE : use g_quark_to_string to convert a GQuark into a static char *
 */
#define LTTV_TRACE_STATE_GET_TRAP_NAME(tse, trap_number)\
		(tse->trap_names[trap_number])

/* LTTV_TRACE_STATE_GET_IRQ_NAME
 *
 * Input : LttvTraceState *tse
 *         guint           irq_number
 * returns : (GQuark)
 *
 * NOTE : use g_quark_to_string to convert a GQuark into a static char *
 */
#define LTTV_TRACE_STATE_GET_IRQ_NAME(tse, irq_number)\
		(tse->irq_names[irq_number])


/* LTTV_TRACE_STATE_GET_PROCESS_STATE
 *
 * Input : LttvTraceState *tse
 *         guint           pid
 *         guint           cpu_index  (0 .. number_cpu-1)
 * returns : (LttvProcessState *)
 *
 * NOTE : if pid is 0, the special process corresponding to the CPU that
 *                     corresponds to the tracefile will be returned.
 *        if pid is different than 0, the process returned may be running
 *                     on any cpu of the trace.
 */
#define LTTV_TRACE_STATE_GET_PROCESS_STATE(tse, pid, cpu_index)\
		(lttv_state_find_process( \
			(LttvTraceFileState*)tse->parent->tracefiles[\
				cpu_index+\
				ltt_trace_control_tracefile_number((LttvTraceContext*)tse->t)], pid))


/* LTTV_TRACE_STATE_GET_NUMBER_CPU
 *
 * input : LttvTraceState *tse
 * returns : (guint)    number_cpu
 */
#define LTTV_TRACE_STATE_GET_NUMBER_CPU(tse)\
		( ltt_trace_per_cpu_tracefile_number((LttvTraceState*)tse->t) )




/************ TracefileState get methods ***************/

/* LTTV_TRACEFILE_STATE_GET_TRACEFILE_CONTEXT
 *
 * Input : LttvTracefileState *tfse
 * returns : (LttvTracefileContext*)
 *
 */
#define LTTV_TRACEFILE_STATE_GET_TRACEFILE_CONTEXT(tfse)\
		((LttvTracefileContext*)tfse)


/* LTTV_TRACEFILE_STATE_GET_CURRENT_PROCESS_STATE
 *
 * Returns the state of the current process.
 *
 * Input : LttvTracefileState *tfse
 * returns : (LttvProcessState *)
 */
#define LTTV_TRACEFILE_STATE_GET_CURRENT_PROCESS_STATE(tfse)\
		(tfse->process)

/* LTTV_TRACEFILE_STATE_GET_CPU_NAME
 *
 * Input : LttvTracefileState *tfse
 * returns : (GQuark)
 *
 * NOTE : use g_quark_to_string to convert a GQuark into a static char *
 */
#define LTTV_TRACEFILE_STATE_GET_CPU_NAME(tfse)\
		(tfse->cpu_name)


/* LTTV_TRACEFILE_STATE_GET_PROCESS_STATE
 *
 * Input : LttvTracefileState *tfse
 *         guint           pid
 * returns : (LttvProcessState *)
 *
 * NOTE : if pid is 0, the special process corresponding to the CPU that
 *                     corresponds to the tracefile will be returned.
 *        if pid is different than 0, the process returned may be running
 *                     on any cpu of the trace.
 */
#define LTTV_TRACEFILE_STATE_GET_PROCESS_STATE(tfse, pid)\
		(lttv_state_find_process(tfse, pid))





/************ ProcessState get methods ***************/
/* Use direct access to LttvProcessState members for other attributes */
/* see struct _LttvProcessState definition in state.h */

/* LTTV_PROCESS_STATE_GET_CURRENT_EXECUTION_STATE
 *
 * Input : LttvProcessState *pse
 * returns : (LttvExecutionState*)
 */
#define LTTV_PROCESS_STATE_GET_EXECUTION_STATE(pse)\
		(pse->state)

/* LTTV_PROCESS_STATE_GET_NESTED_EXECUTION_STATE
 *
 * Input : LttvProcessState *pse
 *         guint             nest_number (0 to num_nest-1)
 * returns : (LttvExecutionState*)
 */
#define LTTV_PROCESS_STATE_GET_NESTED_EXECUTION_STATE(pse, num_nest)\
		(&g_array_index(pse->execution_stack,LttvExecutionState,num_nest))


/* LTTV_PROCESS_STATE_GET_NUM_NESTED_EXECUTION_STATES
 *
 * Returns the number of nested execution states currently on the stack.
 * 
 * Input : LttvProcessState *pse
 * returns : (guint)
 */
#define LTTV_PROCESS_STATE_GET_NUM_NESTED_EXECUTION_STATES(pse)\
		(pse->execution_stack->len)


/************ ExecutionState get methods ***************/
/* Use direct access to LttvExecutionState members to access attributes */
/* see struct _LttvExecutionState definition in state.h */






/* Statistics */





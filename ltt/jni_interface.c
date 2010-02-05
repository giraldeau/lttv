/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2010 William Bourque
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

/* Important to get consistent size_t type */
#define _FILE_OFFSET_BITS 64

#include <jni.h>

#include <ltt/trace.h>
#include <ltt/time.h>
#include <ltt/marker.h>
#include <glib.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/* 
***FIXME***
***HACK***
 We've got hell of a problem passing "unsigned int64" to java, as there is no equivalent type
   The closer we can use is "long" which is signed, so only 32 (or 63?) bits are valid
   Plus, even if we are within the "32 bits" capacity, java sometime screw itself trying to convert "unsigned 64 -> signed 64"
   This happen especially often when RETURNING a jlong
   So when returning a jlong, we should convert it like this : "unsigned 64"->"signed 64"->jlong
*/
#define CONVERT_UINT64_TO_JLONG(n) (jlong)(gint64)(n)
#define CONVERT_INT64_TO_JLONG(n) (jlong)(gint64)(n)

/* To ease calcul involving nano */
#define BILLION 1000000000


#if __WORDSIZE == 64
        #define CONVERT_JLONG_TO_PTR(p) (p)
	#define CONVERT_PTR_TO_JLONG(p) (jlong)(p)
        /* Define the "gint" type we should use for pointer. */
        #define GINT_TYPE_FOR_PTR gint64
#else
        /* Conversion to int first to save warning */
        #define CONVERT_JLONG_TO_PTR(p) (int)(p)
        #define CONVERT_PTR_TO_JLONG(p) (jlong)(int)(p)
        /* Define the "gint" type we should use for pointer. */
        #define GINT_TYPE_FOR_PTR gint32
#endif

/* Structure to encapsulate java_data to pass as arguments */
struct java_calling_data
{
    JNIEnv *env;
    jobject jobj;
};

/*  ***TODO***
 All these struct are used to call g_datalist_foreach()
 Find a better way! This is ugly!
*/
struct addMarkersArgs
{ 
    struct java_calling_data *java_args;
    struct marker_data *mdata;
};

struct saveTimeArgs
{
        GArray *saveTimeArray;
};

struct saveTimeAndTracefile
{
        LttTime time;
        LttTracefile *tracefile;
};

/* 
### COMMON Methods ###
#
Empty method to turn off the debug (debug waste time while printing) */
void ignore_and_drop_message(const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data) {
}


/* JNI method to call printf from the java side */
JNIEXPORT void JNICALL Java_org_eclipse_linuxtools_lttng_jni_Jni_1C_1Common_ltt_1printC(JNIEnv *env, jobject jobj, jstring new_string) {
        const char *c_msg = (*env)->GetStringUTFChars(env, new_string, 0);
        
        printf("%s", c_msg );
        
        (*env)->ReleaseStringUTFChars(env, new_string, c_msg);
}
/* 
#
#### */


/* 
### TRACE methods ###
#
JNI mapping of   < LttTrace *ltt_trace_open(const gchar *pathname)  > (trace.h) */
JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1openTrace(JNIEnv *env, jobject jobj, jstring pathname, jboolean show_debug) {
        
        if ( !show_debug) {
                /* Make sure we don't use any debug (speed up the read) */
                g_log_set_handler(NULL, G_LOG_LEVEL_INFO, ignore_and_drop_message, NULL);
                g_log_set_handler(NULL, G_LOG_LEVEL_DEBUG, ignore_and_drop_message, NULL);
        }
        
        const char *c_pathname = (*env)->GetStringUTFChars(env, pathname, 0);
        LttTrace *newPtr = ltt_trace_open( c_pathname );
        
        (*env)->ReleaseStringUTFChars(env, pathname, c_pathname);
        
        return CONVERT_PTR_TO_JLONG(newPtr); 
}

/* JNI mapping of  < void ltt_trace_close(LttTrace *t)  > (trace.h) */
JNIEXPORT void JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1closeTrace(JNIEnv *env, jobject jobj, jlong trace_ptr){
        
        LttTrace *newPtr = (LttTrace*)CONVERT_JLONG_TO_PTR(trace_ptr);
        
        ltt_trace_close(newPtr);
}

/* Get the tracepath */
JNIEXPORT jstring JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getTracepath(JNIEnv *env, jobject jobj, jlong trace_ptr) {
        LttTrace *newPtr = (LttTrace*)CONVERT_JLONG_TO_PTR(trace_ptr);
        
        return (*env)->NewStringUTF(env, g_quark_to_string( newPtr->pathname) );
}


/* Get of num_cpu */
JNIEXPORT jint JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getCpuNumber(JNIEnv *env, jobject jobj, jlong trace_ptr) {
        LttTrace *newPtr = (LttTrace*)CONVERT_JLONG_TO_PTR(trace_ptr);
        
        return (jint)newPtr->num_cpu;
}

/* Get of arch_type */
JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getArchType(JNIEnv *env, jobject jobj, jlong trace_ptr) {
        LttTrace *newPtr = (LttTrace*)CONVERT_JLONG_TO_PTR(trace_ptr);
        
        return (jlong)newPtr->arch_type;
}

/* Get of arch_variant */
JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getArchVariant(JNIEnv *env, jobject jobj, jlong trace_ptr) {
        LttTrace *newPtr = (LttTrace*)CONVERT_JLONG_TO_PTR(trace_ptr);
        
        return (jlong)newPtr->arch_variant;
}

/* Get of arch_size */
JNIEXPORT jshort JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getArchSize(JNIEnv *env, jobject jobj, jlong trace_ptr) {
        LttTrace *newPtr = (LttTrace*)CONVERT_JLONG_TO_PTR(trace_ptr);
        
        return (jshort)newPtr->arch_size;
}

/* Get of ltt_major_version */
JNIEXPORT jshort JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getLttMajorVersion(JNIEnv *env, jobject jobj, jlong trace_ptr) {
        LttTrace *newPtr = (LttTrace*)CONVERT_JLONG_TO_PTR(trace_ptr);
        
        return (jshort)newPtr->ltt_major_version;
}

/* Get of ltt_minor_version */
JNIEXPORT jshort JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getLttMinorVersion(JNIEnv *env, jobject jobj, jlong trace_ptr) {
        LttTrace *newPtr = (LttTrace*)CONVERT_JLONG_TO_PTR(trace_ptr);
        
        return (jshort)newPtr->ltt_minor_version;
}

/* Get of flight_recorder */
JNIEXPORT jshort JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getFlightRecorder(JNIEnv *env, jobject jobj, jlong trace_ptr) {
        LttTrace *newPtr = (LttTrace*)CONVERT_JLONG_TO_PTR(trace_ptr);
        
        return (jshort)newPtr->flight_recorder;
}

/* Get of freq_scale */
JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getFreqScale(JNIEnv *env, jobject jobj, jlong trace_ptr) {
        LttTrace *newPtr = (LttTrace*)CONVERT_JLONG_TO_PTR(trace_ptr);
        
        return (jlong)newPtr->freq_scale;
}

/* Get of start_freq */
JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getStartFreq(JNIEnv *env, jobject jobj, jlong trace_ptr) {
        LttTrace *newPtr = (LttTrace*)CONVERT_JLONG_TO_PTR(trace_ptr);
        
        return CONVERT_UINT64_TO_JLONG(newPtr->start_freq);
}

/* Get of start_tsc */
JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getStartTimestampCurrentCounter(JNIEnv *env, jobject jobj, jlong trace_ptr) {
        LttTrace *newPtr = (LttTrace*)CONVERT_JLONG_TO_PTR(trace_ptr);
        
        return CONVERT_UINT64_TO_JLONG(newPtr->start_tsc);
}

/* Get of start_monotonic */
JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getStartMonotonic(JNIEnv *env, jobject jobj, jlong trace_ptr) {
        LttTrace *newPtr = (LttTrace*)CONVERT_JLONG_TO_PTR(trace_ptr);
        
        return CONVERT_UINT64_TO_JLONG(newPtr->start_monotonic);
}

/* Access to start_time */
/* Note that we are calling the setTimeFromC function in Jaf_Time from here */
JNIEXPORT void JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1feedStartTime(JNIEnv *env, jobject jobj, jlong trace_ptr, jobject time_jobj) {
        LttTrace *newPtr = (LttTrace*)CONVERT_JLONG_TO_PTR(trace_ptr);
        
        jclass accessClass = (*env)->GetObjectClass(env, time_jobj);
        jmethodID accessFunction = (*env)->GetMethodID(env, accessClass, "setTimeFromC", "(J)V");
        
        jlong fullTime = (CONVERT_UINT64_TO_JLONG(newPtr->start_time.tv_sec)*BILLION) + CONVERT_UINT64_TO_JLONG(newPtr->start_time.tv_nsec);
        
        (*env)->CallVoidMethod(env, time_jobj, accessFunction, fullTime );
}

/* Access to start_time_from_tsc */
/* Note that we are calling the setTimeFromC function in Jaf_Time from here */
JNIEXPORT void JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1feedStartTimeFromTimestampCurrentCounter(JNIEnv *env, jobject jobj, jlong trace_ptr, jobject time_jobj) {
        LttTrace *newPtr = (LttTrace*)CONVERT_JLONG_TO_PTR(trace_ptr);
        
        jclass accessClass = (*env)->GetObjectClass(env, time_jobj);
        jmethodID accessFunction = (*env)->GetMethodID(env, accessClass, "setTimeFromC", "(J)V");
        
        jlong fullTime = (CONVERT_UINT64_TO_JLONG(newPtr->start_time_from_tsc.tv_sec)*BILLION) + CONVERT_UINT64_TO_JLONG(newPtr->start_time_from_tsc.tv_nsec);
        
        (*env)->CallVoidMethod(env, time_jobj, accessFunction, fullTime);
}


/* g_list_data function for the "for_each" call in Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getAllTracefiles  */
void g_datalist_foreach_addTracefilesOfTrace(GQuark name, gpointer data, gpointer user_data) {
        struct java_calling_data *args = (struct java_calling_data*)user_data;
        
        jclass accessClass = (*args->env)->GetObjectClass(args->env, args->jobj);
        jmethodID accessFunction = (*args->env)->GetMethodID(args->env, accessClass, "addTracefileFromC", "(Ljava/lang/String;J)V");
        
        GArray *tracefile_array = (GArray*)data;
        LttTracefile *tracefile;
        jlong newPtr;
        
        unsigned int i;
        for (i=0; i<tracefile_array->len; i++) {
                tracefile = &g_array_index(tracefile_array, LttTracefile, i);
                
                newPtr = CONVERT_PTR_TO_JLONG(tracefile);
                
                (*args->env)->CallVoidMethod(args->env, args->jobj, accessFunction, (*args->env)->NewStringUTF(args->env, g_quark_to_string(tracefile->name) ), newPtr );
        }
}

/* Function to fill up the java map with the event type found in tracefile (the name) */
JNIEXPORT void JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1feedAllTracefiles(JNIEnv *env, jobject jobj, jlong trace_ptr) {
        LttTrace *newPtr = (LttTrace*)CONVERT_JLONG_TO_PTR(trace_ptr);
        
        struct java_calling_data args = { env, jobj };
        
        g_datalist_foreach(&newPtr->tracefiles, &g_datalist_foreach_addTracefilesOfTrace, &args);
}


/* g_list_data function for the "for_each" call in Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1feedTracefileTimeRange */
/*      used to save the current timestamp for each tracefile */
void g_datalist_foreach_saveTracefilesTime(GQuark name, gpointer data, gpointer user_data) {
        struct saveTimeArgs *args = (struct saveTimeArgs*)user_data;
        
        GArray *tracefile_array = (GArray*)data;
        GArray *save_array = args->saveTimeArray;
        
        LttTracefile *tracefile;
        struct saveTimeAndTracefile *savedData;
        
        unsigned int i;
        for (i=0; i<tracefile_array->len; i++) {
                tracefile = &g_array_index(tracefile_array, LttTracefile, i);
                
                /* Allocate a new LttTime for each tracefile (so it won't change if the tracefile seek somewhere else) */
                savedData = (struct saveTimeAndTracefile*)malloc( sizeof(struct saveTimeAndTracefile) );
                savedData->time.tv_sec = tracefile->event.event_time.tv_sec;
                savedData->time.tv_nsec = tracefile->event.event_time.tv_nsec;
                savedData->tracefile = tracefile;
                /* Append the saved data to the array */
                g_array_append_val(save_array, savedData);
        }
}


/* Obtain the range of the trace (i.e. "start time" and "end time") */
/* Note : This function, unlike ltt_trace_time_span_get, is assured to return all tracefiles to their correct position after operation */
/* NOTE : this method is quite heavy to use!!! */
JNIEXPORT void JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1feedTracefileTimeRange(JNIEnv *env, jobject jobj, jlong trace_ptr, jobject jstart_time, jobject jend_time) {
        LttTrace *newPtr = (LttTrace*)CONVERT_JLONG_TO_PTR(trace_ptr);
        
        /* Allocate ourself a new array to save the data in */
        GArray *savedDataArray = g_array_new(FALSE, FALSE, sizeof(struct saveTimeAndTracefile*) );
        struct saveTimeArgs args = { savedDataArray };
        /* Call g_datalist_foreach_saveTracefilesTime for each element in the GData to save the time */
        g_datalist_foreach(&newPtr->tracefiles, &g_datalist_foreach_saveTracefilesTime, &args);
        
        /* Call to ltt_trace_time_span_get to find the current start and end time */
        /* NOTE : This WILL change the current block of the tracefile (i.e. its timestamp) */
        LttTime tmpStartTime = { 0, 0 };
        LttTime tmpEndTime = { 0, 0 };
        ltt_trace_time_span_get(newPtr, &tmpStartTime, &tmpEndTime);
        
        /* Seek back to the correct time for each tracefile and free the allocated memory */
        struct saveTimeAndTracefile *savedData;
        unsigned int i;
        for (i=0; i<savedDataArray->len; i++) {
                savedData = g_array_index(savedDataArray, struct saveTimeAndTracefile*, i);
                /* Seek back to the correct time */
                /* Some time will not be consistent here (i.e. unitialized data) */
                /*      but the seek should work just fine with that */
                ltt_tracefile_seek_time(savedData->tracefile, savedData->time);
                
                /* Free the memory allocated for this saveTimeAndTracefile entry */
                free( savedData );
        }
        /* Free the memory allocated for the GArray */
        g_array_free(savedDataArray, TRUE);
        
        /* Send the start and end time back to the java */
        /* We do it last to make sure a problem won't leave us with unfred memory */
        jclass startAccessClass = (*env)->GetObjectClass(env, jstart_time);
        jmethodID startAccessFunction = (*env)->GetMethodID(env, startAccessClass, "setTimeFromC", "(J)V");
        jlong startTime = (CONVERT_UINT64_TO_JLONG(tmpStartTime.tv_sec)*BILLION) + CONVERT_UINT64_TO_JLONG(tmpStartTime.tv_nsec);
        (*env)->CallVoidMethod(env, jstart_time, startAccessFunction, startTime);
        
        jclass endAccessClass = (*env)->GetObjectClass(env, jend_time);
        jmethodID endAccessFunction = (*env)->GetMethodID(env, endAccessClass, "setTimeFromC", "(J)V");
        jlong endTime = (CONVERT_UINT64_TO_JLONG(tmpEndTime.tv_sec)*BILLION) + CONVERT_UINT64_TO_JLONG(tmpEndTime.tv_nsec);
        (*env)->CallVoidMethod(env, jend_time, endAccessFunction, endTime);
}

/* Function to print the content of a trace */
JNIEXPORT void JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1printTrace(JNIEnv *env, jobject jobj, jlong trace_ptr) {
        
        LttTrace *newPtr = (LttTrace*)CONVERT_JLONG_TO_PTR(trace_ptr);
        
        printf("pathname                : %s\n"     ,g_quark_to_string(newPtr->pathname) );
        printf("num_cpu                 : %u\n"     ,(unsigned int)(newPtr->num_cpu) );
        printf("arch_type               : %u\n"     ,(unsigned int)(newPtr->arch_type) );
        printf("arch_variant            : %u\n"     ,(unsigned int)(newPtr->arch_variant) );
        printf("arch_size               : %u\n"     ,(unsigned short)(newPtr->arch_size) );
        printf("ltt_major_version       : %u\n"     ,(unsigned short)(newPtr->ltt_major_version) );
        printf("ltt_minor_version       : %u\n"     ,(unsigned short)(newPtr->ltt_minor_version) );
        printf("flight_recorder         : %u\n"     ,(unsigned short)(newPtr->flight_recorder) );
        printf("freq_scale              : %u\n"     ,(unsigned int)(newPtr->freq_scale) );
        printf("start_freq              : %lu\n"    ,(long unsigned int)(newPtr->start_freq) );
        printf("start_tsc               : %lu\n"    ,(long unsigned int)(newPtr->start_tsc) );
        printf("start_monotonic         : %lu\n"    ,(long unsigned int)(newPtr->start_monotonic) );
        printf("start_time ptr          : %p\n"     ,&newPtr->start_time);
        printf("   tv_sec               : %lu\n"    ,(long unsigned int)(newPtr->start_time.tv_sec) );
        printf("   tv_nsec              : %lu\n"    ,(long unsigned int)(newPtr->start_time.tv_nsec) );
        printf("start_time_from_tsc ptr : %p\n"     ,&newPtr->start_time_from_tsc);
        printf("   tv_sec               : %lu\n"    ,(long unsigned int)(newPtr->start_time_from_tsc.tv_sec) );
        printf("   tv_nsec              : %lu\n"    ,(long unsigned int)(newPtr->start_time_from_tsc.tv_nsec) );
        printf("tracefiles ptr          : %p\n"     ,newPtr->tracefiles);
        printf("\n");
}
/* 
#
### */




/* 
### TRACEFILE methods ###
# */

/* Get of cpu_online */
JNIEXPORT jboolean JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getIsCpuOnline(JNIEnv *env, jobject jobj, jlong tracefile_ptr) {
        LttTracefile *newPtr = (LttTracefile*)CONVERT_JLONG_TO_PTR(tracefile_ptr);
        
        return (jboolean)newPtr->cpu_online;
}

/* Get of long_name */
JNIEXPORT jstring JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getTracefilepath(JNIEnv *env, jobject jobj, jlong tracefile_ptr) {
        LttTracefile *newPtr = (LttTracefile*)CONVERT_JLONG_TO_PTR(tracefile_ptr);
        
        return (*env)->NewStringUTF(env, g_quark_to_string(newPtr->long_name) );
}

/* Get of name */
JNIEXPORT jstring JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getTracefilename(JNIEnv *env, jobject jobj, jlong tracefile_ptr) {
        LttTracefile *newPtr = (LttTracefile*)CONVERT_JLONG_TO_PTR(tracefile_ptr);
        
        return (*env)->NewStringUTF(env, g_quark_to_string(newPtr->name) );
}

/* Get of cpu_num */
JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getCpuNumber(JNIEnv *env, jobject jobj, jlong tracefile_ptr) {
        LttTracefile *newPtr = (LttTracefile*)CONVERT_JLONG_TO_PTR(tracefile_ptr);
        
        return (jlong)newPtr->cpu_num;
}

/* Get of tid */
JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getTid(JNIEnv *env, jobject jobj, jlong tracefile_ptr) {
        LttTracefile *newPtr = (LttTracefile*)CONVERT_JLONG_TO_PTR(tracefile_ptr);
        
        return (jlong)newPtr->tid;
}

/* Get of pgid */
JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getPgid(JNIEnv *env, jobject jobj, jlong tracefile_ptr) {
        LttTracefile *newPtr = (LttTracefile*)CONVERT_JLONG_TO_PTR(tracefile_ptr);
        
        return (jlong)newPtr->pgid;
}

/* Get of creation */
JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getCreation(JNIEnv *env, jobject jobj, jlong tracefile_ptr) {
        LttTracefile *newPtr = (LttTracefile*)CONVERT_JLONG_TO_PTR(tracefile_ptr);
        
        return CONVERT_UINT64_TO_JLONG(newPtr->creation);
}

/* Get of trace */
JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getTracePtr(JNIEnv *env, jobject jobj, jlong tracefile_ptr) {
        LttTracefile *newPtr = (LttTracefile*)CONVERT_JLONG_TO_PTR(tracefile_ptr);
        
        return CONVERT_PTR_TO_JLONG(newPtr->trace);
}

/* Get of mdata */
JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getMarkerDataPtr(JNIEnv *env, jobject jobj, jlong tracefile_ptr) {
        LttTracefile *newPtr = (LttTracefile*)CONVERT_JLONG_TO_PTR(tracefile_ptr);
        
        return CONVERT_PTR_TO_JLONG(newPtr->mdata);
}

/* Get of fd */
JNIEXPORT jint JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getCFileDescriptor(JNIEnv *env, jobject jobj, jlong tracefile_ptr) {
        LttTracefile *newPtr = (LttTracefile*)CONVERT_JLONG_TO_PTR(tracefile_ptr);
        
        return (jint)newPtr->fd;
}

/* Get of file_size */
JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getFileSize(JNIEnv *env, jobject jobj, jlong tracefile_ptr) {
        LttTracefile *newPtr = (LttTracefile*)CONVERT_JLONG_TO_PTR(tracefile_ptr);
        
        return CONVERT_UINT64_TO_JLONG(newPtr->file_size);
}

/* Get of num_blocks */
JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBlockNumber(JNIEnv *env, jobject jobj, jlong tracefile_ptr) {
        LttTracefile *newPtr = (LttTracefile*)CONVERT_JLONG_TO_PTR(tracefile_ptr);
        
        return (jlong)newPtr->num_blocks;
}

/* Get of reverse_bo */
JNIEXPORT jboolean JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getIsBytesOrderReversed(JNIEnv *env, jobject jobj, jlong tracefile_ptr) {
        LttTracefile *newPtr = (LttTracefile*)CONVERT_JLONG_TO_PTR(tracefile_ptr);
        
        return (jboolean)newPtr->reverse_bo;
}

/* Get of float_word_order */
JNIEXPORT jboolean JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getIsFloatWordOrdered(JNIEnv *env, jobject jobj, jlong tracefile_ptr) {
        LttTracefile *newPtr = (LttTracefile*)CONVERT_JLONG_TO_PTR(tracefile_ptr);
        
        return (jboolean)newPtr->float_word_order;
}

/* Get of alignment */
JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getAlignement(JNIEnv *env, jobject jobj, jlong tracefile_ptr) {
        LttTracefile *newPtr = (LttTracefile*)CONVERT_JLONG_TO_PTR(tracefile_ptr);
        
        return CONVERT_UINT64_TO_JLONG(newPtr->alignment);
}

/* Get of buffer_header_size */
JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBufferHeaderSize(JNIEnv *env, jobject jobj, jlong tracefile_ptr) {
        LttTracefile *newPtr = (LttTracefile*)CONVERT_JLONG_TO_PTR(tracefile_ptr);
        
        return CONVERT_UINT64_TO_JLONG(newPtr->buffer_header_size);
}

/* Get of tscbits */
JNIEXPORT jint JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBitsOfCurrentTimestampCounter(JNIEnv *env, jobject jobj, jlong tracefile_ptr) {
        LttTracefile *newPtr = (LttTracefile*)CONVERT_JLONG_TO_PTR(tracefile_ptr);
        
        return (jint)newPtr->tscbits;
}

/* Get of eventbits */
JNIEXPORT jint JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBitsOfEvent(JNIEnv *env, jobject jobj, jlong tracefile_ptr) {
        LttTracefile *newPtr = (LttTracefile*)CONVERT_JLONG_TO_PTR(tracefile_ptr);
        
        return (jint)newPtr->eventbits;
}

/* Get of tsc_mask */
JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getCurrentTimestampCounterMask(JNIEnv *env, jobject jobj, jlong tracefile_ptr) {
        LttTracefile *newPtr = (LttTracefile*)CONVERT_JLONG_TO_PTR(tracefile_ptr);
        
        return CONVERT_UINT64_TO_JLONG(newPtr->tsc_mask);
}

/* Get of tsc_mask_next_bit */
JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getCurrentTimestampCounterMaskNextBit(JNIEnv *env, jobject jobj, jlong tracefile_ptr) {
        LttTracefile *newPtr = (LttTracefile*)CONVERT_JLONG_TO_PTR(tracefile_ptr);
        
        return CONVERT_UINT64_TO_JLONG(newPtr->tsc_mask_next_bit);
}

/* Get of events_lost */
JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getEventsLost(JNIEnv *env, jobject jobj, jlong tracefile_ptr) {
        LttTracefile *newPtr = (LttTracefile*)CONVERT_JLONG_TO_PTR(tracefile_ptr);
        
        return (jlong)newPtr->events_lost;
}

/* Get of subbuf_corrupt */
JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getSubBufferCorrupt(JNIEnv *env, jobject jobj, jlong tracefile_ptr) {
        LttTracefile *newPtr = (LttTracefile*)CONVERT_JLONG_TO_PTR(tracefile_ptr);
        
        return (jlong)newPtr->subbuf_corrupt;
}

/* Get of event */
JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getEventPtr(JNIEnv *env, jobject jobj, jlong tracefile_ptr) {
        LttTracefile *newPtr = (LttTracefile*)CONVERT_JLONG_TO_PTR(tracefile_ptr);
        
        return CONVERT_PTR_TO_JLONG(&newPtr->event);
}

/* Get of buffer */
JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBufferPtr(JNIEnv *env, jobject jobj, jlong tracefile_ptr) {
        LttTracefile *newPtr = (LttTracefile*)CONVERT_JLONG_TO_PTR(tracefile_ptr);
        
        return CONVERT_PTR_TO_JLONG(&newPtr->buffer);
}

/* Get of buffer size */
JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBufferSize(JNIEnv *env, jobject jobj, jlong tracefile_ptr) {
        LttTracefile *newPtr = (LttTracefile*)CONVERT_JLONG_TO_PTR(tracefile_ptr);
        
        return (jlong)newPtr->buffer.size;
}


/* g_list_data function for the "for_each" call in Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getAllMarkers */
void g_hash_table_foreach_addMarkersOfTracefile(gpointer key, gpointer data, gpointer user_data) {
        struct addMarkersArgs *args = (struct addMarkersArgs*)user_data;
        struct java_calling_data *jargs = (struct java_calling_data*)args->java_args;
        
        jclass accessClass = (*jargs->env)->GetObjectClass(jargs->env, jargs->jobj);
        jmethodID accessFunction = (*jargs->env)->GetMethodID(jargs->env, accessClass, "addMarkersFromC", "(IJ)V");
        
        unsigned long marker_id = (unsigned long)data;
        
        /* The hash table store an ID... we will use the ID to access the array. */
        GArray *marker = args->mdata->markers;
        struct marker_info *newPtr = &g_array_index(marker, struct marker_info, marker_id);
        
        (*jargs->env)->CallVoidMethod(jargs->env, jargs->jobj, accessFunction, marker_id, CONVERT_PTR_TO_JLONG(newPtr) );
}

/* Function to fill up the java map with the event type found in tracefile (the name) */
JNIEXPORT void JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1feedAllMarkers(JNIEnv *env, jobject jobj, jlong tracefile_ptr) {
        LttTracefile *newPtr = (LttTracefile*)CONVERT_JLONG_TO_PTR(tracefile_ptr);
        
        /* ***TODO***
        Find a better way! This is ugly! */
        struct java_calling_data java_args = { env, jobj };
        struct addMarkersArgs args = { &java_args, newPtr->mdata };
        
        g_hash_table_foreach( newPtr->mdata->markers_hash, &g_hash_table_foreach_addMarkersOfTracefile,  &args);
}


/* Function to print the content of a tracefile */
JNIEXPORT void JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1printTracefile(JNIEnv *env, jobject jobj, jlong tracefile_ptr) {
        
        LttTracefile *newPtr = (LttTracefile*)CONVERT_JLONG_TO_PTR(tracefile_ptr);
        printf("cpu_online              : %i\n"     ,(int)newPtr->cpu_online);
        printf("long_name               : %s\n"     ,g_quark_to_string(newPtr->long_name));
        printf("name                    : %s\n"     ,g_quark_to_string(newPtr->name));
        printf("cpu_num                 : %u\n"     ,(unsigned int)(newPtr->cpu_num));
        printf("tid                     : %u\n"     ,(unsigned int)(newPtr->tid));
        printf("pgid                    : %u\n"     ,(unsigned int)(newPtr->pgid));
        printf("creation                : %lu\n"    ,(long unsigned int)(newPtr->creation));
        printf("trace ptr               : %p\n"     ,newPtr->trace);
        printf("marker ptr              : %p\n"     ,newPtr->mdata);
        printf("fd                      : %i\n"     ,(int)(newPtr->fd));
        printf("file_size               : %u\n"     ,(unsigned int)(newPtr->file_size));
        printf("num_blocks              : %u\n"     ,(unsigned int)(newPtr->num_blocks));
        printf("reverse_bo              : %i\n"     ,(int)newPtr->reverse_bo);
        printf("float_word_order        : %i\n"     ,(int)newPtr->float_word_order);
        printf("alignment               : %i\n"     ,(int)newPtr->alignment);
        printf("buffer_header_size      : %i\n"     ,(int)newPtr->buffer_header_size);
        printf("tscbits                 : %u\n"     ,(unsigned short)newPtr->tscbits);
        printf("eventbits               : %u\n"     ,(unsigned short)newPtr->eventbits);
        printf("tsc_mask                : %lu\n"    ,(long unsigned int)newPtr->tsc_mask);
        printf("tsc_mask_next_bit       : %lu\n"    ,(long unsigned int)newPtr->tsc_mask_next_bit);
        printf("events_lost             : %u\n"     ,(unsigned int)newPtr->events_lost);
        printf("subbuf_corrupt          : %u\n"     ,(unsigned int)newPtr->subbuf_corrupt);
        printf("event ptr               : %p\n"     ,&newPtr->event);
        printf("buffer ptr              : %p\n"     ,&newPtr->buffer);
        printf("\n");
}
/* 
#
### */



/* 
### EVENT methods ###
#  */

/* Method to get the read status */
/*    This method will read the next event and then seek back its initial position */
/*    Lttv assume that every tracefile have at least 1 event, but we have not guarantee after this one. */
/*    We will read the next event and return the status of that read */
/*    We will then seek back to our initial position */
/*    Note : this method is expensive and should not be used too often */
JNIEXPORT jint JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1positionToFirstEvent(JNIEnv *env, jobject jobj, jlong tracefile_ptr) {
        LttTracefile *tracefilePtr = (LttTracefile*)CONVERT_JLONG_TO_PTR(tracefile_ptr);
        
        /* Ask ltt to read the next events on the given tracefiles
           Returned value can be :  
            0               if everything went fine (EOK)
            ERANGE  = 34    out of range, back to last event (might be system dependent?)
            EPERM   = 1     error while reading              (might be system dependent?)  */
        
        
        /* Seek to the start time... this will also read the first event, as we want. */
        int returnedValue = ltt_tracefile_seek_time(tracefilePtr, ((struct LttTrace)*(tracefilePtr->trace)).start_time_from_tsc);
        
        return (jint)returnedValue;
}

/* Method to read next event */
JNIEXPORT jint JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1readNextEvent(JNIEnv *env, jobject jobj, jlong tracefile_ptr) {
        LttTracefile *newPtr = (LttTracefile*)CONVERT_JLONG_TO_PTR(tracefile_ptr);
        
        /* Ask ltt to read the next events on the given tracefiles
           Returned value can be :  
            0               if everything went fine (EOK)
            ERANGE  = 34    out of range, back to last event (might be system dependent?)
            EPERM   = 1     error while reading              (might be system dependent?)  */
        
        
        /* ***FIXME***
          This might fail on the FIRST event, as the timestamp before the first read is uninitialized
          However, LTT make the assumption that all tracefile have AT LEAST one event, so we got to run with it  */
        
        /* Save "last time" before moving, to be able to get back if needed */
        LttTime lastTime = newPtr->event.event_time;
        
        int returnedValue = ltt_tracefile_read(newPtr);
        
        /* We need to get back to previous after an error to keep a sane state */
        if ( returnedValue != 0 ) {
                ltt_tracefile_seek_time(newPtr, lastTime);
        }
        
        return (jint)returnedValue;
}

/* Method to seek to a certain event */
JNIEXPORT jint JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1seekEvent(JNIEnv *env, jobject jobj, jlong tracefile_ptr, jobject time_jobj) {
        LttTracefile *newPtr = (LttTracefile*)CONVERT_JLONG_TO_PTR(tracefile_ptr);
        
        guint64 fullTime = 0;
        
        jclass accessClass = (*env)->GetObjectClass(env, time_jobj);
        jmethodID getTimeFunction = (*env)->GetMethodID(env, accessClass, "getTime", "()J");
        fullTime = (*env)->CallLongMethod(env, time_jobj, getTimeFunction);
        
        /* ***HACK***
           Conversion from jlong -> C long seems to be particularly sloppy
           Depending how and where (inlined a function or as a single operation) we do this, we might end up with wrong number
           The following asignation of guint64 seems to work well.
           MAKE SURE TO PERFORM SEVERAL TESTS IF YOU CHANGE THIS.  */
        guint64 seconds = fullTime/BILLION;
        guint64 nanoSeconds = fullTime%BILLION;
        
        LttTime seekTime = { (unsigned long)seconds, (unsigned long)nanoSeconds };
        
        /* Ask ltt to read the next events on the given tracefiles
           Returned value can be :  
            0               if everything went fine (EOK)
            ERANGE  = 34    out of range, back to last event (might be system dependent?)
            EPERM   = 1     error while reading              (might be system dependent?)  */
        
        int returnedValue = ltt_tracefile_seek_time(newPtr, seekTime);
        return (jint)returnedValue;
}

/* Get of tracefile */
JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getTracefilePtr(JNIEnv *env, jobject jobj, jlong event_ptr) {
        LttEvent *newPtr = (LttEvent*)CONVERT_JLONG_TO_PTR(event_ptr);
        
        return CONVERT_PTR_TO_JLONG(newPtr->tracefile);
}

/* Get of block */
JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getBlock(JNIEnv *env, jobject jobj, jlong event_ptr) {
        LttEvent *newPtr = (LttEvent*)CONVERT_JLONG_TO_PTR(event_ptr);
        
        return (jlong)newPtr->block;
}

/* Get of offset */
JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getOffset(JNIEnv *env, jobject jobj, jlong event_ptr) {
        LttEvent *newPtr = (LttEvent*)CONVERT_JLONG_TO_PTR(event_ptr);
        
        return (jlong)newPtr->offset;
}

/* Get of tsc */
JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getCurrentTimestampCounter(JNIEnv *env, jobject jobj, jlong event_ptr) {
        LttEvent *newPtr = (LttEvent*)CONVERT_JLONG_TO_PTR(event_ptr);
        
        return CONVERT_UINT64_TO_JLONG(newPtr->tsc);
}

/* Get of timestamp */
JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getTimestamp(JNIEnv *env, jobject jobj, jlong event_ptr) {
        LttEvent *newPtr = (LttEvent*)CONVERT_JLONG_TO_PTR(event_ptr);
        
        return (jlong)newPtr->timestamp;
}

/* Get of event_id */
JNIEXPORT jint JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getEventMarkerId(JNIEnv *env, jobject jobj, jlong event_ptr) {
        LttEvent *newPtr = (LttEvent*)CONVERT_JLONG_TO_PTR(event_ptr);
        
        return (jint)newPtr->event_id;
}

/* Get time in nanoseconds */
JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getNanosencondsTime(JNIEnv *env, jobject jobj, jlong event_ptr) {
        LttEvent *newPtr = (LttEvent*)CONVERT_JLONG_TO_PTR(event_ptr);
        
        return (CONVERT_UINT64_TO_JLONG(newPtr->event_time.tv_sec)*BILLION) + CONVERT_UINT64_TO_JLONG(newPtr->event_time.tv_nsec);
}

/* Fill event_time into an object */
JNIEXPORT void JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1feedEventTime(JNIEnv *env, jobject jobj, jlong event_ptr, jobject time_jobj) {
        LttEvent *newPtr = (LttEvent*)CONVERT_JLONG_TO_PTR(event_ptr);
        
        jclass accessClass = (*env)->GetObjectClass(env, time_jobj);
        jmethodID accessFunction = (*env)->GetMethodID(env, accessClass, "setTimeFromC", "(J)V");
        
        jlong fullTime = (CONVERT_UINT64_TO_JLONG(newPtr->event_time.tv_sec)*BILLION) + CONVERT_UINT64_TO_JLONG(newPtr->event_time.tv_nsec);
        
        (*env)->CallVoidMethod(env, time_jobj, accessFunction, fullTime);
}

/* Access method to the data */
/* The data are in "byte" form */
JNIEXPORT void JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getDataContent(JNIEnv *env, jobject jobj, jlong event_ptr, jlong data_size, jbyteArray dataArray) {
        LttEvent *newPtr = (LttEvent*)CONVERT_JLONG_TO_PTR(event_ptr);
        
        (*env)->SetByteArrayRegion(env, dataArray, 0, (jsize)data_size, newPtr->data);
}

/* Get of data_size */
JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getEventDataSize(JNIEnv *env, jobject jobj, jlong event_ptr) {
        LttEvent *newPtr = (LttEvent*)CONVERT_JLONG_TO_PTR(event_ptr);
        
        return (jlong)newPtr->data_size;
}

/* Get of event_size */
JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getEventSize(JNIEnv *env, jobject jobj, jlong event_ptr) {
        LttEvent *newPtr = (LttEvent*)CONVERT_JLONG_TO_PTR(event_ptr);
        
        return (jlong)newPtr->event_size;
}

/* Get of count */
JNIEXPORT jint JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getCount(JNIEnv *env, jobject jobj, jlong event_ptr) {
        LttEvent *newPtr = (LttEvent*)CONVERT_JLONG_TO_PTR(event_ptr);
        
        return (jint)newPtr->count;
}

/* Get of overflow_nsec */
JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getOverflowNanoSeconds(JNIEnv *env, jobject jobj, jlong event_ptr) {
        LttEvent *newPtr = (LttEvent*)CONVERT_JLONG_TO_PTR(event_ptr);
        
        return CONVERT_UINT64_TO_JLONG(newPtr->overflow_nsec);
}


/* Function to print the content of a event */
JNIEXPORT void JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1printEvent(JNIEnv *env, jobject jobj, jlong event_ptr) {
        LttEvent *newPtr = (LttEvent*)CONVERT_JLONG_TO_PTR(event_ptr);
        
        printf("tracefile               : %p\n"  ,(void*)newPtr->tracefile );
        printf("block                   : %u\n"  ,(unsigned int)newPtr->block );
        printf("offset                  : %u\n"  ,(unsigned int)newPtr->offset );
        printf("tsc                     : %lu\n" ,(long unsigned int)newPtr->tsc );
        printf("timestamp               : %u\n"  ,(unsigned int)newPtr->timestamp );
        printf("event_id                : %u\n"  ,(unsigned short)newPtr->event_id );
        printf("event_time              : %p\n"  ,(void*) &newPtr->event_time );
        printf("   sec                  : %lu\n" ,(long unsigned int)(newPtr->event_time.tv_sec) );
        printf("   nsec                 : %lu\n" ,(long unsigned int)(newPtr->event_time.tv_nsec) );
        printf("data                    : %p\n"  ,(void*) newPtr->data );
        printf("data_size               : %u\n"  ,(unsigned int)newPtr->data_size );
        printf("event_size              : %u\n"  ,(unsigned int)newPtr->event_size );
        printf("count                   : %d\n"  ,(int)newPtr->count );
        printf("overflow_nsec           : %ld\n" ,(long)newPtr->overflow_nsec );
        printf("\n");
}
/* 
#
### */


/* 
### MARKER method ###
#  */

/* Get of name */
JNIEXPORT jstring JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getName(JNIEnv *env, jobject jobj, jlong marker_info_ptr) {
        struct marker_info *newPtr = (struct marker_info*)CONVERT_JLONG_TO_PTR(marker_info_ptr);
        
        return (*env)->NewStringUTF(env, g_quark_to_string(newPtr->name));
}

/* Get of format */
JNIEXPORT jstring JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getFormatOverview(JNIEnv *env, jobject jobj, jlong marker_info_ptr) {
        struct marker_info *newPtr = (struct marker_info*)CONVERT_JLONG_TO_PTR(marker_info_ptr);
        
        return (*env)->NewStringUTF(env, newPtr->format);
}

/* Get of size */
JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getSize(JNIEnv *env, jobject jobj, jlong marker_info_ptr) {
        struct marker_info *newPtr = (struct marker_info*)CONVERT_JLONG_TO_PTR(marker_info_ptr);
        
        return (jlong)newPtr->size;
}

/* Method to get all markerField pointers */
JNIEXPORT void JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getAllMarkerFields(JNIEnv *env, jobject jobj, jlong marker_info_ptr) {
        struct marker_info *newPtr = (struct marker_info*)CONVERT_JLONG_TO_PTR(marker_info_ptr);
        
        jclass accessClass = (*env)->GetObjectClass(env, jobj);
        jmethodID accessFunction = (*env)->GetMethodID(env, accessClass, "addMarkerFieldFromC", "(Ljava/lang/String;J)V");
        
        GArray *field_array = (GArray*)newPtr->fields;
        struct marker_field *field;
        jlong marker_field_ptr;
        
        unsigned int i;
        for (i=0; i<field_array->len; i++) {
                field = &g_array_index(field_array, struct marker_field, i);
                
                marker_field_ptr = CONVERT_PTR_TO_JLONG(field);
                
                (*env)->CallVoidMethod(env, jobj, accessFunction, (*env)->NewStringUTF(env, g_quark_to_string(field->name) ), marker_field_ptr );
        }
}



/* Get of largest_align */
JNIEXPORT jshort JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getLargestAlign(JNIEnv *env, jobject jobj, jlong marker_info_ptr) {
        struct marker_info *newPtr = (struct marker_info*)CONVERT_JLONG_TO_PTR(marker_info_ptr);
        
        return (jshort)newPtr->largest_align;
}

/* Get of int_size */
JNIEXPORT jshort JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getIntSize(JNIEnv *env, jobject jobj, jlong marker_info_ptr) {
        struct marker_info *newPtr = (struct marker_info*)CONVERT_JLONG_TO_PTR(marker_info_ptr);
        
        return (jshort)newPtr->int_size;
}

/* Get of long_size */
JNIEXPORT jshort JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getLongSize(JNIEnv *env, jobject jobj, jlong marker_info_ptr) {
        struct marker_info *newPtr = (struct marker_info*)CONVERT_JLONG_TO_PTR(marker_info_ptr);
        
        return (jshort)newPtr->long_size;
}

/* Get of pointer_size */
JNIEXPORT jshort JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getPointerSize(JNIEnv *env, jobject jobj, jlong marker_info_ptr) {
        struct marker_info *newPtr = (struct marker_info*)CONVERT_JLONG_TO_PTR(marker_info_ptr);
        
        return (jshort)newPtr->pointer_size;
}

/* Get of size_t_size */
JNIEXPORT jshort JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getSize_1tSize(JNIEnv *env, jobject jobj, jlong marker_info_ptr) {
        struct marker_info *newPtr = (struct marker_info*)CONVERT_JLONG_TO_PTR(marker_info_ptr);
        
        return (jshort)newPtr->size_t_size;
}

/* Get of alignment */
JNIEXPORT jshort JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getAlignement(JNIEnv *env, jobject jobj, jlong marker_info_ptr) {
        struct marker_info *newPtr = (struct marker_info*)CONVERT_JLONG_TO_PTR(marker_info_ptr);
        
        return (jshort)newPtr->alignment;
}

/* Get of next */
JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getNextMarkerPtr(JNIEnv *env, jobject jobj, jlong marker_info_ptr) {
        struct marker_info *newPtr = (struct marker_info*)CONVERT_JLONG_TO_PTR(marker_info_ptr);
        
        return CONVERT_PTR_TO_JLONG(newPtr->next);
}


/* Function to print the content of a marker */
JNIEXPORT void JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1printMarker(JNIEnv *env, jobject jobj, jlong marker_info_ptr) {
        struct marker_info *newPtr = (struct marker_info*)CONVERT_JLONG_TO_PTR(marker_info_ptr);
        
        printf("name          : %s\n"  ,g_quark_to_string(newPtr->name) );
        printf("format        : %s\n"  ,newPtr->format );
        printf("size          : %li\n" ,(long int)newPtr->size );
        printf("largest_align : %u\n"  ,(unsigned short)newPtr->largest_align );
        printf("fields        : %p\n"  ,newPtr->fields );
        printf("int_size      : %u\n"  ,(unsigned short)newPtr->int_size );
        printf("long_size     : %u\n"  ,(unsigned short)newPtr->long_size );
        printf("pointer_size  : %u\n"  ,(unsigned short)newPtr->pointer_size );
        printf("size_t_size   : %u\n"  ,(unsigned short)newPtr->size_t_size );
        printf("alignment     : %u\n"  ,(unsigned short)newPtr->alignment );
        printf("next          : %p\n"  ,newPtr->next );
        printf("\n");
}
/* 
#
### */



/* 
### MARKERFIELD Method
#  */

/* Get of name */
JNIEXPORT jstring JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getField(JNIEnv *env, jobject jobj, jlong marker_field_ptr) {
        struct marker_field *newPtr = (struct marker_field*)CONVERT_JLONG_TO_PTR(marker_field_ptr);
        
        return (*env)->NewStringUTF(env, g_quark_to_string(newPtr->name));
}

/* Get of type */
JNIEXPORT jint JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getType(JNIEnv *env, jobject jobj, jlong marker_field_ptr) {
        struct marker_field *newPtr = (struct marker_field*)CONVERT_JLONG_TO_PTR(marker_field_ptr);
        
        return (jint)newPtr->type;
}

/* Get of offset */
JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getOffset(JNIEnv *env, jobject jobj, jlong marker_field_ptr) {
        struct marker_field *newPtr = (struct marker_field*)CONVERT_JLONG_TO_PTR(marker_field_ptr);
        
        return CONVERT_UINT64_TO_JLONG(newPtr->offset);
}

/* Get of size */
JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getSize(JNIEnv *env, jobject jobj, jlong marker_field_ptr) {
        struct marker_field *newPtr = (struct marker_field*)CONVERT_JLONG_TO_PTR(marker_field_ptr);
        
        return CONVERT_UINT64_TO_JLONG(newPtr->size);
}

/* Get of alignment */
JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getAlignment(JNIEnv *env, jobject jobj, jlong marker_field_ptr) {
        struct marker_field *newPtr = (struct marker_field*)CONVERT_JLONG_TO_PTR(marker_field_ptr);
        
        return CONVERT_UINT64_TO_JLONG(newPtr->alignment);
}

/* Get of attributes */
JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getAttributes(JNIEnv *env, jobject jobj, jlong marker_field_ptr) {
        struct marker_field *newPtr = (struct marker_field*)CONVERT_JLONG_TO_PTR(marker_field_ptr);
        
        return CONVERT_UINT64_TO_JLONG(newPtr->attributes);
}

/* Get of static_offset */
JNIEXPORT jint JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getStatic_1offset(JNIEnv *env, jobject jobj, jlong marker_field_ptr) {
        struct marker_field *newPtr = (struct marker_field*)CONVERT_JLONG_TO_PTR(marker_field_ptr);
        
        return (jint)newPtr->static_offset;
}

/* Get of fmt */
JNIEXPORT jstring JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getFormat(JNIEnv *env, jobject jobj, jlong marker_field_ptr) {
        struct marker_field *newPtr = (struct marker_field*)CONVERT_JLONG_TO_PTR(marker_field_ptr);
        
        return (*env)->NewStringUTF(env, newPtr->fmt->str);
}

/* Function to print the content of a marker_field */
JNIEXPORT void JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1printMarkerField(JNIEnv *env, jobject jobj, jlong marker_field_ptr) {
        struct marker_field *newPtr = (struct marker_field*)CONVERT_JLONG_TO_PTR(marker_field_ptr);
        
        printf("name          : %s\n"  ,g_quark_to_string(newPtr->name) );
        printf("type          : %i\n"  ,(int)newPtr->type );
        printf("offset        : %lu\n" ,(long unsigned int)newPtr->offset );
        printf("size          : %lu\n" ,(long unsigned int)newPtr->size );
        printf("alignment     : %lu\n" ,(long unsigned int)newPtr->alignment );
        printf("attributes    : %lu\n" ,(long unsigned int)newPtr->attributes );
        printf("static_offset : %i\n"  ,(int)newPtr->static_offset );
        printf("fmt           : %s\n"  ,newPtr->fmt->str );
        printf("\n");
}
/* 
#
### */


/* 
### PARSER Method
# */

/* This function will do the actual parsing  */
/*      It will then call java to assign the parsed data to the object "javaObj" */
JNIEXPORT void JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniParser_ltt_1getParsedData(JNIEnv *env, jclass accessClass, jobject javaObj, jlong event_ptr, jlong marker_field_ptr) {
        LttEvent newEventPtr = *(LttEvent*)(CONVERT_JLONG_TO_PTR(event_ptr));
        struct marker_field *newMarkerFieldPtr = (struct marker_field*)CONVERT_JLONG_TO_PTR(marker_field_ptr);
        
        jmethodID accessFunction = NULL;
        
        
        /* 
          There is a very limited number of type in LTT
          We will switch on the type for this field and act accordingly
             NOTE : We will save all integer into "long" type, as there is no signed/unsigned in java */
        
        /* ***HACK***
           It seems the marker_field->type is absolutely not consistent, especially about pointer!
              Sometime pointer are saved in String, sometime as Int, sometime as pointer...
           We will do an extra check on type "LTT_TYPE_UNSIGNED_INT" to check if the marker_field->format is hint of a pointer  */
        switch ( newMarkerFieldPtr->type ) {
                case LTT_TYPE_SIGNED_INT : 
                        accessFunction = (*env)->GetStaticMethodID(env, accessClass, "addLongToParsingFromC", "(Ljava/lang/Object;J)V");
                        (*env)->CallStaticVoidMethod(   env, 
                                                        accessClass, 
                                                        accessFunction, 
                                                        javaObj, 
                                                        ltt_event_get_long_int(&newEventPtr, newMarkerFieldPtr)
                                                     );
                        
                        break;
                
                case LTT_TYPE_UNSIGNED_INT :
                        /* If the format seems to be a pointer, add it as a pointer */
                        if ( (strncmp(newMarkerFieldPtr->fmt->str, "0x%llX", newMarkerFieldPtr->fmt->len) == 0 ) || (strncmp(newMarkerFieldPtr->fmt->str, "%llX", newMarkerFieldPtr->fmt->len) == 0 ) ) {
                                #if __WORDSIZE == 64
                                        accessFunction = (*env)->GetStaticMethodID(env, accessClass, "addLongPointerToParsingFromC", "(Ljava/lang/Object;J)V");
                                #else
                                        accessFunction = (*env)->GetStaticMethodID(env, accessClass, "addIntPointerToParsingFromC", "(Ljava/lang/Object;J)V");
                                #endif
                                (*env)->CallStaticVoidMethod(   env, 
                                                                accessClass, 
                                                                accessFunction, 
                                                                javaObj, 
                                                                CONVERT_PTR_TO_JLONG(ltt_event_get_long_unsigned(&newEventPtr, newMarkerFieldPtr) )
                                                             );
                        }
                        /* Otherwise, add it as a number */
                        else {
                                accessFunction = (*env)->GetStaticMethodID(env, accessClass, "addLongToParsingFromC", "(Ljava/lang/Object;J)V");
                                (*env)->CallStaticVoidMethod(   env, 
                                                                accessClass,
                                                                accessFunction,
                                                                javaObj,
                                                                ltt_event_get_long_unsigned(&newEventPtr, newMarkerFieldPtr)
                                                             );
                        }
                        
                        break;
                        
                case LTT_TYPE_POINTER :
                        #if __WORDSIZE == 64
                                accessFunction = (*env)->GetStaticMethodID(env, accessClass, "addLongPointerToParsingFromC", "(Ljava/lang/Object;J)V");
                        #else
                                accessFunction = (*env)->GetStaticMethodID(env, accessClass, "addIntPointerToParsingFromC", "(Ljava/lang/Object;J)V");
                        #endif
                        (*env)->CallStaticVoidMethod(   env, 
                                                        accessClass, 
                                                        accessFunction, 
                                                        javaObj, 
                                                        CONVERT_PTR_TO_JLONG(*(GINT_TYPE_FOR_PTR*)(newEventPtr.data + newMarkerFieldPtr->offset))
                                                     );
                        break;
                        
                case LTT_TYPE_STRING :
                        accessFunction = (*env)->GetStaticMethodID(env, accessClass, "addStringToParsingFromC", "(Ljava/lang/Object;Ljava/lang/String;)V");
                        (*env)->CallStaticVoidMethod(   env,
                                                        accessClass,
                                                        accessFunction,
                                                        javaObj,
                                                        (*env)->NewStringUTF(env, ltt_event_get_string(&newEventPtr, newMarkerFieldPtr) )
                                                     );
                        break;
                        
                case LTT_TYPE_COMPACT :
                case LTT_TYPE_NONE :
                default :
                        printf("Warning : Unrecognized format type! Skipping! (Java_org_eclipse_linuxtools_lttng_jni_JniParser_ltt_1getParsedData)");
                        break;
        }
        
}
/* 
#
###  */

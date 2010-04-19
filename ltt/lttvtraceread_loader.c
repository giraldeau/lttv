
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
#include <dlfcn.h>

int nb_id = 0;

struct version_correlation {
        int     id;
        char    *libname;
        void    *static_handle;
};

struct version_correlation *version_table = NULL;

struct function_tables {
        JNIEXPORT void JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_Jni_1C_1Common_ltt_1printC)(JNIEnv *env, jobject jobj, jstring new_string);
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1openTrace)(JNIEnv *env, jobject jobj, jstring pathname, jboolean show_debug);
        JNIEXPORT void JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1closeTrace)(JNIEnv *env, jobject jobj, jlong trace_ptr);
        JNIEXPORT jstring JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getTracepath)(JNIEnv *env, jobject jobj, jlong trace_ptr);
        JNIEXPORT jint JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getCpuNumber)(JNIEnv *env, jobject jobj, jlong trace_ptr);
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getArchType)(JNIEnv *env, jobject jobj, jlong trace_ptr);
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getArchVariant)(JNIEnv *env, jobject jobj, jlong trace_ptr);
        JNIEXPORT jshort JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getArchSize)(JNIEnv *env, jobject jobj, jlong trace_ptr);
        JNIEXPORT jshort JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getLttMajorVersion)(JNIEnv *env, jobject jobj, jlong trace_ptr);
        JNIEXPORT jshort JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getLttMinorVersion)(JNIEnv *env, jobject jobj, jlong trace_ptr);
        JNIEXPORT jshort JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getFlightRecorder)(JNIEnv *env, jobject jobj, jlong trace_ptr);
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getFreqScale)(JNIEnv *env, jobject jobj, jlong trace_ptr);
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getStartFreq)(JNIEnv *env, jobject jobj, jlong trace_ptr);
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getStartTimestampCurrentCounter)(JNIEnv *env, jobject jobj, jlong trace_ptr);
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getStartMonotonic)(JNIEnv *env, jobject jobj, jlong trace_ptr);
        JNIEXPORT void JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1feedStartTime)(JNIEnv *env, jobject jobj, jlong trace_ptr, jobject time_jobj);
        JNIEXPORT void JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1feedStartTimeFromTimestampCurrentCounter)(JNIEnv *env, jobject jobj, jlong trace_ptr, jobject time_jobj);
        JNIEXPORT void JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1feedAllTracefiles)(JNIEnv *env, jobject jobj, jlong trace_ptr);
        JNIEXPORT void JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1feedTracefileTimeRange)(JNIEnv *env, jobject jobj, jlong trace_ptr, jobject jstart_time, jobject jend_time);
        JNIEXPORT void JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1printTrace)(JNIEnv *env, jobject jobj, jlong trace_ptr);
        JNIEXPORT jboolean JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getIsCpuOnline)(JNIEnv *env, jobject jobj, jlong tracefile_ptr);
        JNIEXPORT jstring JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getTracefilepath)(JNIEnv *env, jobject jobj, jlong tracefile_ptr);
        JNIEXPORT jstring JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getTracefilename)(JNIEnv *env, jobject jobj, jlong tracefile_ptr);
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getCpuNumber)(JNIEnv *env, jobject jobj, jlong tracefile_ptr);
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getTid)(JNIEnv *env, jobject jobj, jlong tracefile_ptr);
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getPgid)(JNIEnv *env, jobject jobj, jlong tracefile_ptr);
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getCreation)(JNIEnv *env, jobject jobj, jlong tracefile_ptr);
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getTracePtr)(JNIEnv *env, jobject jobj, jlong tracefile_ptr);
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getMarkerDataPtr)(JNIEnv *env, jobject jobj, jlong tracefile_ptr);
        JNIEXPORT jint JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getCFileDescriptor)(JNIEnv *env, jobject jobj, jlong tracefile_ptr);
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getFileSize)(JNIEnv *env, jobject jobj, jlong tracefile_ptr);
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBlockNumber)(JNIEnv *env, jobject jobj, jlong tracefile_ptr);
        JNIEXPORT jboolean JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getIsBytesOrderReversed)(JNIEnv *env, jobject jobj, jlong tracefile_ptr);
        JNIEXPORT jboolean JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getIsFloatWordOrdered)(JNIEnv *env, jobject jobj, jlong tracefile_ptr);
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getAlignement)(JNIEnv *env, jobject jobj, jlong tracefile_ptr);
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBufferHeaderSize)(JNIEnv *env, jobject jobj, jlong tracefile_ptr);
        JNIEXPORT jint JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBitsOfCurrentTimestampCounter)(JNIEnv *env, jobject jobj, jlong tracefile_ptr);
        JNIEXPORT jint JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBitsOfEvent)(JNIEnv *env, jobject jobj, jlong tracefile_ptr);
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getCurrentTimestampCounterMask)(JNIEnv *env, jobject jobj, jlong tracefile_ptr);
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getCurrentTimestampCounterMaskNextBit)(JNIEnv *env, jobject jobj, jlong tracefile_ptr);
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getEventsLost)(JNIEnv *env, jobject jobj, jlong tracefile_ptr);
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getSubBufferCorrupt)(JNIEnv *env, jobject jobj, jlong tracefile_ptr);
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getEventPtr)(JNIEnv *env, jobject jobj, jlong tracefile_ptr);
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBufferPtr)(JNIEnv *env, jobject jobj, jlong tracefile_ptr);
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBufferSize)(JNIEnv *env, jobject jobj, jlong tracefile_ptr);
        JNIEXPORT void JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1feedAllMarkers)(JNIEnv *env, jobject jobj, jlong tracefile_ptr);
        JNIEXPORT void JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1printTracefile)(JNIEnv *env, jobject jobj, jlong tracefile_ptr);
        JNIEXPORT jint JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1positionToFirstEvent)(JNIEnv *env, jobject jobj, jlong tracefile_ptr);
        JNIEXPORT jint JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1readNextEvent)(JNIEnv *env, jobject jobj, jlong tracefile_ptr);
        JNIEXPORT jint JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1seekEvent)(JNIEnv *env, jobject jobj, jlong tracefile_ptr, jobject time_jobj);
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getTracefilePtr)(JNIEnv *env, jobject jobj, jlong event_ptr);
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getBlock)(JNIEnv *env, jobject jobj, jlong event_ptr);
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getOffset)(JNIEnv *env, jobject jobj, jlong event_ptr);
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getCurrentTimestampCounter)(JNIEnv *env, jobject jobj, jlong event_ptr);
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getTimestamp)(JNIEnv *env, jobject jobj, jlong event_ptr);
        JNIEXPORT jint JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getEventMarkerId)(JNIEnv *env, jobject jobj, jlong event_ptr);
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getNanosencondsTime)(JNIEnv *env, jobject jobj, jlong event_ptr);
        JNIEXPORT void JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1feedEventTime)(JNIEnv *env, jobject jobj, jlong event_ptr, jobject time_jobj);
        JNIEXPORT void JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getDataContent)(JNIEnv *env, jobject jobj, jlong event_ptr, jlong data_size, jbyteArray dataArray);
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getEventDataSize)(JNIEnv *env, jobject jobj, jlong event_ptr);
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getEventSize)(JNIEnv *env, jobject jobj, jlong event_ptr);
        JNIEXPORT jint JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getCount)(JNIEnv *env, jobject jobj, jlong event_ptr);
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getOverflowNanoSeconds)(JNIEnv *env, jobject jobj, jlong event_ptr);
        JNIEXPORT void JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1printEvent)(JNIEnv *env, jobject jobj, jlong event_ptr);
        JNIEXPORT jstring JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getName)(JNIEnv *env, jobject jobj, jlong marker_info_ptr);
        JNIEXPORT jstring JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getFormatOverview)(JNIEnv *env, jobject jobj, jlong marker_info_ptr);
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getSize)(JNIEnv *env, jobject jobj, jlong marker_info_ptr);
        JNIEXPORT void JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getAllMarkerFields)(JNIEnv *env, jobject jobj, jlong marker_info_ptr);
        JNIEXPORT jshort JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getLargestAlign)(JNIEnv *env, jobject jobj, jlong marker_info_ptr);
        JNIEXPORT jshort JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getIntSize)(JNIEnv *env, jobject jobj, jlong marker_info_ptr);
        JNIEXPORT jshort JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getLongSize)(JNIEnv *env, jobject jobj, jlong marker_info_ptr);
        JNIEXPORT jshort JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getPointerSize)(JNIEnv *env, jobject jobj, jlong marker_info_ptr);
        JNIEXPORT jshort JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getSize_1tSize)(JNIEnv *env, jobject jobj, jlong marker_info_ptr);
        JNIEXPORT jshort JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getAlignement)(JNIEnv *env, jobject jobj, jlong marker_info_ptr);
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getNextMarkerPtr)(JNIEnv *env, jobject jobj, jlong marker_info_ptr);
        JNIEXPORT void JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1printMarker)(JNIEnv *env, jobject jobj, jlong marker_info_ptr);
        JNIEXPORT jstring JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getField)(JNIEnv *env, jobject jobj, jlong marker_field_ptr);
        JNIEXPORT jint JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getType)(JNIEnv *env, jobject jobj, jlong marker_field_ptr);
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getOffset)(JNIEnv *env, jobject jobj, jlong marker_field_ptr);
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getSize)(JNIEnv *env, jobject jobj, jlong marker_field_ptr);
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getAlignment)(JNIEnv *env, jobject jobj, jlong marker_field_ptr);
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getAttributes)(JNIEnv *env, jobject jobj, jlong marker_field_ptr);
        JNIEXPORT jint JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getStatic_1offset)(JNIEnv *env, jobject jobj, jlong marker_field_ptr);
        JNIEXPORT jstring JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getFormat)(JNIEnv *env, jobject jobj, jlong marker_field_ptr);
        JNIEXPORT void JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1printMarkerField)(JNIEnv *env, jobject jobj, jlong marker_field_ptr);
        JNIEXPORT void JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniParser_ltt_1getParsedData)(JNIEnv *env, jclass accessClass, jobject javaObj, jlong event_ptr, jlong marker_field_ptr);
};

struct function_tables *version_functions_table = NULL;

void ignore_and_drop_message(const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data) {
}

JNIEXPORT void JNICALL Java_org_eclipse_linuxtools_lttng_jni_factory_JniTraceVersion_ltt_1getTraceVersion(JNIEnv *env, jobject jobj, jstring tracepath) {
        
        void *handle = dlopen("liblttvtraceread.so", RTLD_LAZY );
        
        if (!handle ) {
                printf ("WARNING : Failed to initialize library handle from %s!\n", "liblttvtraceread.so");
        }
        JNIEXPORT void JNICALL (*functor)(JNIEnv *env, jobject jobj, jstring tracepath);
        functor=dlsym(handle, "Java_org_eclipse_linuxtools_lttng_jni_factory_JniTraceVersion_ltt_1getTraceVersion");
        
        char *error = dlerror();
        if ( error != NULL)  {
                printf ("Call failed with : %s\n", error);
        }
        else {
                (*functor)(env, jobj, tracepath);
        }
}

void freeAllHandle() {
        if ( version_table != NULL ) {
                free(version_table);
                version_table = NULL;
        }
        
        if ( version_functions_table != NULL ) {
                free(version_functions_table);
                version_functions_table = NULL;
        }
}

void freeHandle(int handle_id) {
        
        if ( handle_id >= nb_id ) {
                if (version_table[handle_id].static_handle != NULL) {
                        /* Memory will be freed by dlclose as well */
                        dlclose(version_table[handle_id].static_handle);
                        version_table[handle_id].static_handle = NULL;
                        free(version_table[handle_id].libname);
                        version_table[handle_id].libname = NULL;
                }
        }
        
        int isEmpty = 1;
        int n;
        for ( n=0; n<nb_id; n++) {
                if ( version_table[n].static_handle != NULL ) {
                        isEmpty = 0;
                }
        }
        
        if ( isEmpty == 1 ) {
                freeAllHandle();
        }
}

JNIEXPORT void JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1freeHandle(JNIEnv *env, jobject jobj, jint lib_id) {
        // Call function to free the memory
        freeHandle(lib_id);
}

JNIEXPORT jint JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1initializeHandle(JNIEnv *env, jobject jobj, jstring libname) {
        
        jint lib_id = -1;
        const char* c_path = (*env)->GetStringUTFChars(env, libname, 0);
        
        int isLoaded = 0;
        int n;
        for ( n=0; n<nb_id; n++) {
                if ( strncmp(version_table[n].libname, c_path, strlen(version_table[n].libname) ) == 0 ) {
                        isLoaded = 1;
                	 lib_id = version_table[n].id;
                }
        }
        
        if ( isLoaded == 0 ) {
                void *new_handle = dlopen(c_path, RTLD_LAZY );
                
                if (!new_handle ) {
                        printf ("WARNING : Failed to initialize library handle from %s!\n", c_path);
                }
                else {
                        lib_id = nb_id;
                        nb_id++;
                        
                        void* new_version_table = malloc(sizeof(struct version_correlation)*(nb_id) );
                        void* new_function_tables = malloc(sizeof(struct function_tables)*(nb_id) );
                        
                        if ( nb_id > 1) {
                                memcpy(new_version_table,version_table, sizeof(struct version_correlation)*(nb_id-1) );
                                free( version_table );
                                
                                memcpy(new_function_tables,version_functions_table, sizeof(struct function_tables)*(nb_id-1) );
                                free( version_functions_table) ;
                        }
                        
                        version_table = (struct version_correlation *)new_version_table;
                        version_table[lib_id].id = lib_id;
                        version_table[lib_id].libname = (char*)malloc( strlen(c_path) );
                        strncpy(version_table[lib_id].libname, c_path, strlen(c_path));
                        version_table[lib_id].static_handle = new_handle;
                        
                        version_functions_table = (struct function_tables*)new_function_tables;
                        

                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_Jni_1C_1Common_ltt_1printC = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_Jni_1C_1Common_ltt_1printC");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1openTrace = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1openTrace");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1closeTrace = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1closeTrace");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getTracepath = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getTracepath");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getCpuNumber = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getCpuNumber");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getArchType = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getArchType");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getArchVariant = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getArchVariant");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getArchSize = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getArchSize");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getLttMajorVersion = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getLttMajorVersion");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getLttMinorVersion = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getLttMinorVersion");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getFlightRecorder = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getFlightRecorder");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getFreqScale = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getFreqScale");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getStartFreq = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getStartFreq");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getStartTimestampCurrentCounter = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getStartTimestampCurrentCounter");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getStartMonotonic = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getStartMonotonic");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1feedStartTime = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1feedStartTime");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1feedStartTimeFromTimestampCurrentCounter = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1feedStartTimeFromTimestampCurrentCounter");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1feedAllTracefiles = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1feedAllTracefiles");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1feedTracefileTimeRange = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1feedTracefileTimeRange");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1printTrace = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1printTrace");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getIsCpuOnline = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getIsCpuOnline");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getTracefilepath = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getTracefilepath");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getTracefilename = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getTracefilename");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getCpuNumber = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getCpuNumber");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getTid = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getTid");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getPgid = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getPgid");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getCreation = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getCreation");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getTracePtr = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getTracePtr");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getMarkerDataPtr = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getMarkerDataPtr");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getCFileDescriptor = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getCFileDescriptor");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getFileSize = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getFileSize");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBlockNumber = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBlockNumber");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getIsBytesOrderReversed = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getIsBytesOrderReversed");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getIsFloatWordOrdered = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getIsFloatWordOrdered");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getAlignement = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getAlignement");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBufferHeaderSize = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBufferHeaderSize");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBitsOfCurrentTimestampCounter = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBitsOfCurrentTimestampCounter");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBitsOfEvent = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBitsOfEvent");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getCurrentTimestampCounterMask = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getCurrentTimestampCounterMask");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getCurrentTimestampCounterMaskNextBit = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getCurrentTimestampCounterMaskNextBit");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getEventsLost = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getEventsLost");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getSubBufferCorrupt = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getSubBufferCorrupt");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getEventPtr = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getEventPtr");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBufferPtr = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBufferPtr");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBufferSize = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBufferSize");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1feedAllMarkers = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1feedAllMarkers");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1printTracefile = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1printTracefile");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1positionToFirstEvent = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1positionToFirstEvent");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1readNextEvent = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1readNextEvent");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1seekEvent = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1seekEvent");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getTracefilePtr = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getTracefilePtr");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getBlock = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getBlock");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getOffset = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getOffset");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getCurrentTimestampCounter = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getCurrentTimestampCounter");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getTimestamp = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getTimestamp");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getEventMarkerId = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getEventMarkerId");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getNanosencondsTime = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getNanosencondsTime");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1feedEventTime = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1feedEventTime");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getDataContent = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getDataContent");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getEventDataSize = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getEventDataSize");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getEventSize = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getEventSize");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getCount = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getCount");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getOverflowNanoSeconds = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getOverflowNanoSeconds");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1printEvent = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1printEvent");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getName = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getName");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getFormatOverview = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getFormatOverview");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getSize = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getSize");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getAllMarkerFields = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getAllMarkerFields");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getLargestAlign = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getLargestAlign");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getIntSize = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getIntSize");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getLongSize = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getLongSize");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getPointerSize = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getPointerSize");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getSize_1tSize = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getSize_1tSize");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getAlignement = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getAlignement");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getNextMarkerPtr = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getNextMarkerPtr");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1printMarker = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1printMarker");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getField = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getField");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getType = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getType");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getOffset = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getOffset");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getSize = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getSize");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getAlignment = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getAlignment");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getAttributes = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getAttributes");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getStatic_1offset = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getStatic_1offset");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getFormat = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getFormat");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1printMarkerField = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1printMarkerField");
                        version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniParser_ltt_1getParsedData = dlsym(version_table[lib_id].static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniParser_ltt_1getParsedData");
                }
        }
        
        return lib_id;
}

JNIEXPORT void JNICALL Java_org_eclipse_linuxtools_lttng_jni_Jni_1C_1Common_ltt_1printC(JNIEnv *env, jobject jobj, jint lib_id, jstring new_string) {
        (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_Jni_1C_1Common_ltt_1printC)(env, jobj, new_string);
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1openTrace(JNIEnv *env, jobject jobj, jint lib_id, jstring pathname, jboolean show_debug) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1openTrace)(env, jobj, pathname, show_debug);
}


JNIEXPORT void JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1closeTrace(JNIEnv *env, jobject jobj, jint lib_id, jlong trace_ptr) {
        (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1closeTrace)(env, jobj, trace_ptr);
}


JNIEXPORT jstring JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getTracepath(JNIEnv *env, jobject jobj, jint lib_id, jlong trace_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getTracepath)(env, jobj, trace_ptr);
}


JNIEXPORT jint JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getCpuNumber(JNIEnv *env, jobject jobj, jint lib_id, jlong trace_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getCpuNumber)(env, jobj, trace_ptr);
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getArchType(JNIEnv *env, jobject jobj, jint lib_id, jlong trace_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getArchType)(env, jobj, trace_ptr);
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getArchVariant(JNIEnv *env, jobject jobj, jint lib_id, jlong trace_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getArchVariant)(env, jobj, trace_ptr);
}


JNIEXPORT jshort JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getArchSize(JNIEnv *env, jobject jobj, jint lib_id, jlong trace_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getArchSize)(env, jobj, trace_ptr);
}


JNIEXPORT jshort JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getLttMajorVersion(JNIEnv *env, jobject jobj, jint lib_id, jlong trace_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getLttMajorVersion)(env, jobj, trace_ptr);
}


JNIEXPORT jshort JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getLttMinorVersion(JNIEnv *env, jobject jobj, jint lib_id, jlong trace_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getLttMinorVersion)(env, jobj, trace_ptr);
}


JNIEXPORT jshort JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getFlightRecorder(JNIEnv *env, jobject jobj, jint lib_id, jlong trace_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getFlightRecorder)(env, jobj, trace_ptr);
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getFreqScale(JNIEnv *env, jobject jobj, jint lib_id, jlong trace_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getFreqScale)(env, jobj, trace_ptr);
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getStartFreq(JNIEnv *env, jobject jobj, jint lib_id, jlong trace_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getStartFreq)(env, jobj, trace_ptr);
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getStartTimestampCurrentCounter(JNIEnv *env, jobject jobj, jint lib_id, jlong trace_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getStartTimestampCurrentCounter)(env, jobj, trace_ptr);
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getStartMonotonic(JNIEnv *env, jobject jobj, jint lib_id, jlong trace_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getStartMonotonic)(env, jobj, trace_ptr);
}


JNIEXPORT void JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1feedStartTime(JNIEnv *env, jobject jobj, jint lib_id, jlong trace_ptr, jobject time_jobj) {
        (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1feedStartTime)(env, jobj, trace_ptr, time_jobj);
}


JNIEXPORT void JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1feedStartTimeFromTimestampCurrentCounter(JNIEnv *env, jobject jobj, jint lib_id, jlong trace_ptr, jobject time_jobj) {
        (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1feedStartTimeFromTimestampCurrentCounter)(env, jobj, trace_ptr, time_jobj);
}


JNIEXPORT void JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1feedAllTracefiles(JNIEnv *env, jobject jobj, jint lib_id, jlong trace_ptr) {
        (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1feedAllTracefiles)(env, jobj, trace_ptr);
}


JNIEXPORT void JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1feedTracefileTimeRange(JNIEnv *env, jobject jobj, jint lib_id, jlong trace_ptr, jobject jstart_time, jobject jend_time) {
        (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1feedTracefileTimeRange)(env, jobj, trace_ptr, jstart_time, jend_time);
}


JNIEXPORT void JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1printTrace(JNIEnv *env, jobject jobj, jint lib_id, jlong trace_ptr) {
        (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1printTrace)(env, jobj, trace_ptr);
}


JNIEXPORT jboolean JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getIsCpuOnline(JNIEnv *env, jobject jobj, jint lib_id, jlong tracefile_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getIsCpuOnline)(env, jobj, tracefile_ptr);
}


JNIEXPORT jstring JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getTracefilepath(JNIEnv *env, jobject jobj, jint lib_id, jlong tracefile_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getTracefilepath)(env, jobj, tracefile_ptr);
}


JNIEXPORT jstring JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getTracefilename(JNIEnv *env, jobject jobj, jint lib_id, jlong tracefile_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getTracefilename)(env, jobj, tracefile_ptr);
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getCpuNumber(JNIEnv *env, jobject jobj, jint lib_id, jlong tracefile_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getCpuNumber)(env, jobj, tracefile_ptr);
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getTid(JNIEnv *env, jobject jobj, jint lib_id, jlong tracefile_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getTid)(env, jobj, tracefile_ptr);
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getPgid(JNIEnv *env, jobject jobj, jint lib_id, jlong tracefile_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getPgid)(env, jobj, tracefile_ptr);
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getCreation(JNIEnv *env, jobject jobj, jint lib_id, jlong tracefile_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getCreation)(env, jobj, tracefile_ptr);
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getTracePtr(JNIEnv *env, jobject jobj, jint lib_id, jlong tracefile_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getTracePtr)(env, jobj, tracefile_ptr);
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getMarkerDataPtr(JNIEnv *env, jobject jobj, jint lib_id, jlong tracefile_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getMarkerDataPtr)(env, jobj, tracefile_ptr);
}


JNIEXPORT jint JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getCFileDescriptor(JNIEnv *env, jobject jobj, jint lib_id, jlong tracefile_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getCFileDescriptor)(env, jobj, tracefile_ptr);
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getFileSize(JNIEnv *env, jobject jobj, jint lib_id, jlong tracefile_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getFileSize)(env, jobj, tracefile_ptr);
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBlockNumber(JNIEnv *env, jobject jobj, jint lib_id, jlong tracefile_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBlockNumber)(env, jobj, tracefile_ptr);
}


JNIEXPORT jboolean JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getIsBytesOrderReversed(JNIEnv *env, jobject jobj, jint lib_id, jlong tracefile_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getIsBytesOrderReversed)(env, jobj, tracefile_ptr);
}


JNIEXPORT jboolean JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getIsFloatWordOrdered(JNIEnv *env, jobject jobj, jint lib_id, jlong tracefile_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getIsFloatWordOrdered)(env, jobj, tracefile_ptr);
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getAlignement(JNIEnv *env, jobject jobj, jint lib_id, jlong tracefile_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getAlignement)(env, jobj, tracefile_ptr);
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBufferHeaderSize(JNIEnv *env, jobject jobj, jint lib_id, jlong tracefile_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBufferHeaderSize)(env, jobj, tracefile_ptr);
}


JNIEXPORT jint JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBitsOfCurrentTimestampCounter(JNIEnv *env, jobject jobj, jint lib_id, jlong tracefile_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBitsOfCurrentTimestampCounter)(env, jobj, tracefile_ptr);
}


JNIEXPORT jint JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBitsOfEvent(JNIEnv *env, jobject jobj, jint lib_id, jlong tracefile_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBitsOfEvent)(env, jobj, tracefile_ptr);
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getCurrentTimestampCounterMask(JNIEnv *env, jobject jobj, jint lib_id, jlong tracefile_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getCurrentTimestampCounterMask)(env, jobj, tracefile_ptr);
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getCurrentTimestampCounterMaskNextBit(JNIEnv *env, jobject jobj, jint lib_id, jlong tracefile_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getCurrentTimestampCounterMaskNextBit)(env, jobj, tracefile_ptr);
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getEventsLost(JNIEnv *env, jobject jobj, jint lib_id, jlong tracefile_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getEventsLost)(env, jobj, tracefile_ptr);
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getSubBufferCorrupt(JNIEnv *env, jobject jobj, jint lib_id, jlong tracefile_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getSubBufferCorrupt)(env, jobj, tracefile_ptr);
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getEventPtr(JNIEnv *env, jobject jobj, jint lib_id, jlong tracefile_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getEventPtr)(env, jobj, tracefile_ptr);
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBufferPtr(JNIEnv *env, jobject jobj, jint lib_id, jlong tracefile_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBufferPtr)(env, jobj, tracefile_ptr);
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBufferSize(JNIEnv *env, jobject jobj, jint lib_id, jlong tracefile_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBufferSize)(env, jobj, tracefile_ptr);
}


JNIEXPORT void JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1feedAllMarkers(JNIEnv *env, jobject jobj, jint lib_id, jlong tracefile_ptr) {
        (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1feedAllMarkers)(env, jobj, tracefile_ptr);
}


JNIEXPORT void JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1printTracefile(JNIEnv *env, jobject jobj, jint lib_id, jlong tracefile_ptr) {
        (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1printTracefile)(env, jobj, tracefile_ptr);
}


JNIEXPORT jint JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1positionToFirstEvent(JNIEnv *env, jobject jobj, jint lib_id, jlong tracefile_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1positionToFirstEvent)(env, jobj, tracefile_ptr);
}


JNIEXPORT jint JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1readNextEvent(JNIEnv *env, jobject jobj, jint lib_id, jlong tracefile_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1readNextEvent)(env, jobj, tracefile_ptr);
}


JNIEXPORT jint JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1seekEvent(JNIEnv *env, jobject jobj, jint lib_id, jlong tracefile_ptr, jobject time_jobj) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1seekEvent)(env, jobj, tracefile_ptr, time_jobj);
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getTracefilePtr(JNIEnv *env, jobject jobj, jint lib_id, jlong event_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getTracefilePtr)(env, jobj, event_ptr);
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getBlock(JNIEnv *env, jobject jobj, jint lib_id, jlong event_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getBlock)(env, jobj, event_ptr);
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getOffset(JNIEnv *env, jobject jobj, jint lib_id, jlong event_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getOffset)(env, jobj, event_ptr);
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getCurrentTimestampCounter(JNIEnv *env, jobject jobj, jint lib_id, jlong event_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getCurrentTimestampCounter)(env, jobj, event_ptr);
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getTimestamp(JNIEnv *env, jobject jobj, jint lib_id, jlong event_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getTimestamp)(env, jobj, event_ptr);
}


JNIEXPORT jint JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getEventMarkerId(JNIEnv *env, jobject jobj, jint lib_id, jlong event_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getEventMarkerId)(env, jobj, event_ptr);
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getNanosencondsTime(JNIEnv *env, jobject jobj, jint lib_id, jlong event_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getNanosencondsTime)(env, jobj, event_ptr);
}


JNIEXPORT void JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1feedEventTime(JNIEnv *env, jobject jobj, jint lib_id, jlong event_ptr, jobject time_jobj) {
        (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1feedEventTime)(env, jobj, event_ptr, time_jobj);
}


JNIEXPORT void JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getDataContent(JNIEnv *env, jobject jobj, jint lib_id, jlong event_ptr, jlong data_size, jbyteArray dataArray) {
        (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getDataContent)(env, jobj, event_ptr, data_size, dataArray);
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getEventDataSize(JNIEnv *env, jobject jobj, jint lib_id, jlong event_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getEventDataSize)(env, jobj, event_ptr);
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getEventSize(JNIEnv *env, jobject jobj, jint lib_id, jlong event_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getEventSize)(env, jobj, event_ptr);
}


JNIEXPORT jint JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getCount(JNIEnv *env, jobject jobj, jint lib_id, jlong event_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getCount)(env, jobj, event_ptr);
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getOverflowNanoSeconds(JNIEnv *env, jobject jobj, jint lib_id, jlong event_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getOverflowNanoSeconds)(env, jobj, event_ptr);
}


JNIEXPORT void JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1printEvent(JNIEnv *env, jobject jobj, jint lib_id, jlong event_ptr) {
        (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1printEvent)(env, jobj, event_ptr);
}


JNIEXPORT jstring JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getName(JNIEnv *env, jobject jobj, jint lib_id, jlong marker_info_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getName)(env, jobj, marker_info_ptr);
}


JNIEXPORT jstring JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getFormatOverview(JNIEnv *env, jobject jobj, jint lib_id, jlong marker_info_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getFormatOverview)(env, jobj, marker_info_ptr);
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getSize(JNIEnv *env, jobject jobj, jint lib_id, jlong marker_info_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getSize)(env, jobj, marker_info_ptr);
}


JNIEXPORT void JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getAllMarkerFields(JNIEnv *env, jobject jobj, jint lib_id, jlong marker_info_ptr) {
        (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getAllMarkerFields)(env, jobj, marker_info_ptr);
}


JNIEXPORT jshort JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getLargestAlign(JNIEnv *env, jobject jobj, jint lib_id, jlong marker_info_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getLargestAlign)(env, jobj, marker_info_ptr);
}


JNIEXPORT jshort JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getIntSize(JNIEnv *env, jobject jobj, jint lib_id, jlong marker_info_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getIntSize)(env, jobj, marker_info_ptr);
}


JNIEXPORT jshort JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getLongSize(JNIEnv *env, jobject jobj, jint lib_id, jlong marker_info_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getLongSize)(env, jobj, marker_info_ptr);
}


JNIEXPORT jshort JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getPointerSize(JNIEnv *env, jobject jobj, jint lib_id, jlong marker_info_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getPointerSize)(env, jobj, marker_info_ptr);
}


JNIEXPORT jshort JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getSize_1tSize(JNIEnv *env, jobject jobj, jint lib_id, jlong marker_info_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getSize_1tSize)(env, jobj, marker_info_ptr);
}


JNIEXPORT jshort JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getAlignement(JNIEnv *env, jobject jobj, jint lib_id, jlong marker_info_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getAlignement)(env, jobj, marker_info_ptr);
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getNextMarkerPtr(JNIEnv *env, jobject jobj, jint lib_id, jlong marker_info_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getNextMarkerPtr)(env, jobj, marker_info_ptr);
}


JNIEXPORT void JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1printMarker(JNIEnv *env, jobject jobj, jint lib_id, jlong marker_info_ptr) {
        (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1printMarker)(env, jobj, marker_info_ptr);
}


JNIEXPORT jstring JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getField(JNIEnv *env, jobject jobj, jint lib_id, jlong marker_field_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getField)(env, jobj, marker_field_ptr);
}


JNIEXPORT jint JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getType(JNIEnv *env, jobject jobj, jint lib_id, jlong marker_field_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getType)(env, jobj, marker_field_ptr);
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getOffset(JNIEnv *env, jobject jobj, jint lib_id, jlong marker_field_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getOffset)(env, jobj, marker_field_ptr);
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getSize(JNIEnv *env, jobject jobj, jint lib_id, jlong marker_field_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getSize)(env, jobj, marker_field_ptr);
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getAlignment(JNIEnv *env, jobject jobj, jint lib_id, jlong marker_field_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getAlignment)(env, jobj, marker_field_ptr);
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getAttributes(JNIEnv *env, jobject jobj, jint lib_id, jlong marker_field_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getAttributes)(env, jobj, marker_field_ptr);
}


JNIEXPORT jint JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getStatic_1offset(JNIEnv *env, jobject jobj, jint lib_id, jlong marker_field_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getStatic_1offset)(env, jobj, marker_field_ptr);
}


JNIEXPORT jstring JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getFormat(JNIEnv *env, jobject jobj, jint lib_id, jlong marker_field_ptr) {
        return (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getFormat)(env, jobj, marker_field_ptr);
}


JNIEXPORT void JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1printMarkerField(JNIEnv *env, jobject jobj, jint lib_id, jlong marker_field_ptr) {
        (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1printMarkerField)(env, jobj, marker_field_ptr);
}


JNIEXPORT void JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniParser_ltt_1getParsedData(JNIEnv *env, jclass accessClass, jint lib_id, jobject javaObj, jlong event_ptr, jlong marker_field_ptr) {
        (version_functions_table[lib_id].Java_org_eclipse_linuxtools_lttng_jni_JniParser_ltt_1getParsedData)(env, accessClass, javaObj, event_ptr, marker_field_ptr);
}




#include <jni.h>
#include "Sample.h"
#include <stdio.h>
#include <unistd.h>

#define LTT_TRACE
#define LTT_BLOCKING 1
#include <ltt/ltt-facility-user_generic.h>

JNIEXPORT void JNICALL Java_ltt_ThreadBrand_trace_1java_1generic_1thread_1brand
  (JNIEnv *env, jclass jc, jstring jstr)
{
  const char *str;
  str = (*env)->GetStringUTFChars(env, jstr, NULL);
  if (str == NULL) return; // out of memory error thrown
	trace_user_generic_thread_brand(str);
  (*env)->ReleaseStringUTFChars(env, jstr, str);
}


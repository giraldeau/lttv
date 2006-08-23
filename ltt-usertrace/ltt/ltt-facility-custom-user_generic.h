#ifndef _LTT_FACILITY_CUSTOM_USER_GENERIC_H_
#define _LTT_FACILITY_CUSTOM_USER_GENERIC_H_

#include <sys/types.h>
#include <ltt/ltt-facility-id-user_generic.h>
#include <ltt/ltt-usertrace.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

static int trace_user_generic_slow_printf(
		const char *fmt, ...)
#ifndef LTT_TRACE
{
}
#else
{
	/* Guess we need no more than 100 bytes. */
	int n, size = 100;
	char *p, *np;
	va_list ap;
	int ret;

	if ((p = malloc (size)) == NULL)
		return -1;

	while (1) {
		/* Try to print in the allocated space. */
		va_start(ap, fmt);
		n = vsnprintf (p, size, fmt, ap);
		va_end(ap);
		/* If that worked, trace the string. */
		if (n > -1 && n < size) {
			ret = trace_user_generic_slow_printf_param_buffer(p, n+1);
			free(p);
			return ret;
		}
		/* Else try again with more space. */
		if (n > -1)    /* glibc 2.1 */
			 size = n+1; /* precisely what is needed */
		else           /* glibc 2.0 */
			 size *= 2;  /* twice the old size */
		if ((np = realloc (p, size)) == NULL) {
			 free(p);
			 return -1;
		} else {
			 p = np;
		}
 	}
}
#endif //LTT_TRACE

#ifdef __cplusplus
} /* end of extern "C" */
#endif

#endif //_LTT_FACILITY_CUSTOM_USER_GENERIC_H_

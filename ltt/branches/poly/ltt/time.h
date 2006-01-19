/* This file is part of the Linux Trace Toolkit trace reading library
 * Copyright (C) 2003-2004 Michel Dagenais
 *               2005 Mathieu Desnoyers
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

#ifndef LTT_TIME_H
#define LTT_TIME_H

#include <glib.h>
#include <ltt/compiler.h>
#include <math.h>

typedef struct _LttTime {
  unsigned long tv_sec;
  unsigned long tv_nsec;
} LttTime;


#define NANOSECONDS_PER_SECOND 1000000000

/* We give the DIV and MUL constants so we can always multiply, for a
 * division as well as a multiplication of NANOSECONDS_PER_SECOND */
/* 2^30/1.07374182400631629848 = 1000000000.0 */ 
#define DOUBLE_SHIFT_CONST_DIV 1.07374182400631629848
#define DOUBLE_SHIFT 30

/* 2^30*0.93132257461547851562 = 1000000000.0000000000 */ 
#define DOUBLE_SHIFT_CONST_MUL 0.93132257461547851562


/* 1953125 * 2^9 = NANOSECONDS_PER_SECOND */
#define LTT_TIME_UINT_SHIFT_CONST 1953125
#define LTT_TIME_UINT_SHIFT 9


static const LttTime ltt_time_zero = { 0, 0 };

static const LttTime ltt_time_one = { 0, 1 };

static const LttTime ltt_time_infinite = { G_MAXUINT, NANOSECONDS_PER_SECOND };

static inline LttTime ltt_time_sub(LttTime t1, LttTime t2) 
{
  LttTime res;
  res.tv_sec  = t1.tv_sec  - t2.tv_sec;
  res.tv_nsec = t1.tv_nsec - t2.tv_nsec;
  /* unlikely : given equal chance to be anywhere in t1.tv_nsec, and
   * higher probability of low value for t2.tv_sec, we will habitually
   * not wrap.
   */
  if(unlikely(t1.tv_nsec < t2.tv_nsec)) {
    res.tv_sec--;
    res.tv_nsec += NANOSECONDS_PER_SECOND;
  }
  return res;
}


static inline LttTime ltt_time_add(LttTime t1, LttTime t2) 
{
  LttTime res;
  res.tv_nsec = t1.tv_nsec + t2.tv_nsec;
  res.tv_sec = t1.tv_sec + t2.tv_sec;
  /* unlikely : given equal chance to be anywhere in t1.tv_nsec, and
   * higher probability of low value for t2.tv_sec, we will habitually
   * not wrap.
   */
  if(unlikely(res.tv_nsec >= NANOSECONDS_PER_SECOND)) {
    res.tv_sec++;
    res.tv_nsec -= NANOSECONDS_PER_SECOND;
  }
  return res;
}

/* Fastest comparison : t1 > t2 */
static inline int ltt_time_compare(LttTime t1, LttTime t2)
{
  int ret=0;
  if(likely(t1.tv_sec > t2.tv_sec)) ret = 1;
  else if(unlikely(t1.tv_sec < t2.tv_sec)) ret = -1;
  else if(likely(t1.tv_nsec > t2.tv_nsec)) ret = 1;
  else if(unlikely(t1.tv_nsec < t2.tv_nsec)) ret = -1;
  
  return ret;
}

#define LTT_TIME_MIN(a,b) ((ltt_time_compare((a),(b)) < 0) ? (a) : (b))
#define LTT_TIME_MAX(a,b) ((ltt_time_compare((a),(b)) > 0) ? (a) : (b))

#define MAX_TV_SEC_TO_DOUBLE 0x7FFFFF
static inline double ltt_time_to_double(LttTime t1)
{
  /* We lose precision if tv_sec is > than (2^23)-1
   * 
   * Max values that fits in a double (53 bits precision on normalised 
   * mantissa):
   * tv_nsec : NANOSECONDS_PER_SECONDS : 2^30
   *
   * So we have 53-30 = 23 bits left for tv_sec.
   * */
#ifdef EXTRA_CHECK
  g_assert(t1.tv_sec <= MAX_TV_SEC_TO_DOUBLE);
  if(t1.tv_sec > MAX_TV_SEC_TO_DOUBLE)
    g_warning("Precision loss in conversion LttTime to double");
#endif //EXTRA_CHECK
  return ((double)((guint64)t1.tv_sec<<DOUBLE_SHIFT)
                  * (double)DOUBLE_SHIFT_CONST_MUL)
                  + (double)t1.tv_nsec;
}


static inline LttTime ltt_time_from_double(double t1)
{
  /* We lose precision if tv_sec is > than (2^23)-1
   * 
   * Max values that fits in a double (53 bits precision on normalised 
   * mantissa):
   * tv_nsec : NANOSECONDS_PER_SECONDS : 2^30
   *
   * So we have 53-30 = 23 bits left for tv_sec.
   * */
#ifdef EXTRA_CHECK
  g_assert(t1 <= MAX_TV_SEC_TO_DOUBLE);
  if(t1 > MAX_TV_SEC_TO_DOUBLE)
    g_warning("Conversion from non precise double to LttTime");
#endif //EXTRA_CHECK
  LttTime res;
  //res.tv_sec = t1/(double)NANOSECONDS_PER_SECOND;
  res.tv_sec = (guint64)(t1 * DOUBLE_SHIFT_CONST_DIV) >> DOUBLE_SHIFT;
  res.tv_nsec = (t1 - (((guint64)res.tv_sec<<LTT_TIME_UINT_SHIFT))
                               * LTT_TIME_UINT_SHIFT_CONST);
  return res;
}

/* Use ltt_time_to_double and ltt_time_from_double to check for lack
 * of precision.
 */
static inline LttTime ltt_time_mul(LttTime t1, double d)
{
  LttTime res;

  double time_double = ltt_time_to_double(t1);

  time_double = time_double * d;

  res = ltt_time_from_double(time_double);

  return res;

#if 0
  /* What is that ? (Mathieu) */
  if(f == 0.0){
    res.tv_sec = 0;
    res.tv_nsec = 0;
  }else{
  double d;
    d = 1.0/f;
    sec = t1.tv_sec / (double)d;
    res.tv_sec = sec;
    res.tv_nsec = t1.tv_nsec / (double)d + (sec - res.tv_sec) *
                  NANOSECONDS_PER_SECOND;
    res.tv_sec += res.tv_nsec / NANOSECONDS_PER_SECOND;
    res.tv_nsec %= NANOSECONDS_PER_SECOND;
  }
  return res;
#endif //0
}


/* Use ltt_time_to_double and ltt_time_from_double to check for lack
 * of precision.
 */
static inline LttTime ltt_time_div(LttTime t1, double d)
{
  LttTime res;

  double time_double = ltt_time_to_double(t1);

  time_double = time_double / d;

  res = ltt_time_from_double(time_double);

  return res;


#if 0
  double sec;
  LttTime res;

  sec = t1.tv_sec / (double)f;
  res.tv_sec = sec;
  res.tv_nsec = t1.tv_nsec / (double)f + (sec - res.tv_sec) *
      NANOSECONDS_PER_SECOND;
  res.tv_sec += res.tv_nsec / NANOSECONDS_PER_SECOND;
  res.tv_nsec %= NANOSECONDS_PER_SECOND;
  return res;
#endif //0
}


static inline guint64 ltt_time_to_uint64(LttTime t1)
{
  return (((guint64)t1.tv_sec*LTT_TIME_UINT_SHIFT_CONST) << LTT_TIME_UINT_SHIFT)
                       + (guint64)t1.tv_nsec;
}


#define MAX_TV_SEC_TO_UINT64 0x3FFFFFFFFFFFFFFFULL

/* The likely branch is with sec != 0, because most events in a bloc
 * will be over 1s from the block start. (see tracefile.c)
 */
static inline LttTime ltt_time_from_uint64(guint64 t1)
{
  /* We lose precision if tv_sec is > than (2^62)-1
   * */
#ifdef EXTRA_CHECK
  g_assert(t1 <= MAX_TV_SEC_TO_UINT64);
  if(t1 > MAX_TV_SEC_TO_UINT64)
    g_warning("Conversion from uint64 to non precise LttTime");
#endif //EXTRA_CHECK
  LttTime res;
  //if(unlikely(t1 >= NANOSECONDS_PER_SECOND)) {
  if(likely(t1>>LTT_TIME_UINT_SHIFT >= LTT_TIME_UINT_SHIFT_CONST)) {
    //res.tv_sec = t1/NANOSECONDS_PER_SECOND;
    res.tv_sec = (t1>>LTT_TIME_UINT_SHIFT)
                         /LTT_TIME_UINT_SHIFT_CONST; // acceleration
    res.tv_nsec = (t1 - res.tv_sec*NANOSECONDS_PER_SECOND);
  } else {
    res.tv_sec = 0;
    res.tv_nsec = (guint32)t1;
  }
  return res;
}

#endif // LTT_TIME_H

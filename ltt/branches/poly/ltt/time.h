/* This file is part of the Linux Trace Toolkit trace reading library
 * Copyright (C) 2003-2004 Michel Dagenais
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


typedef struct _LttTime {
  unsigned long tv_sec;
  unsigned long tv_nsec;
} LttTime;


static const unsigned long NANOSECONDS_PER_SECOND = 1000000000;

static const LttTime ltt_time_zero = { 0, 0};


static inline LttTime ltt_time_sub(LttTime t1, LttTime t2) 
{
  LttTime res;
  res.tv_sec  = t1.tv_sec  - t2.tv_sec;
  if(t1.tv_nsec < t2.tv_nsec) {
    res.tv_sec--;
    res.tv_nsec = NANOSECONDS_PER_SECOND + t1.tv_nsec - t2.tv_nsec;
  }
  else {
    res.tv_nsec = t1.tv_nsec - t2.tv_nsec;
  }
  return res;
}


static inline LttTime ltt_time_add(LttTime t1, LttTime t2) 
{
  LttTime res;
  res.tv_sec  = t1.tv_sec  + t2.tv_sec;
  res.tv_nsec = t1.tv_nsec + t2.tv_nsec;
  if(res.tv_nsec >= NANOSECONDS_PER_SECOND) {
    res.tv_sec++;
    res.tv_nsec -= NANOSECONDS_PER_SECOND;
  }
  return res;
}


static inline LttTime ltt_time_mul(LttTime t1, float f)
{
  LttTime res;
  float d;
  double sec;

  if(f == 0.0){
    res.tv_sec = 0;
    res.tv_nsec = 0;
  }else{
    d = 1.0/f;
    sec = t1.tv_sec / (double)d;
    res.tv_sec = sec;
    res.tv_nsec = t1.tv_nsec / (double)d + (sec - res.tv_sec) *
                  NANOSECONDS_PER_SECOND;
    res.tv_sec += res.tv_nsec / NANOSECONDS_PER_SECOND;
    res.tv_nsec %= NANOSECONDS_PER_SECOND;
  }
  return res;
}


static inline LttTime ltt_time_div(LttTime t1, float f)
{
  double sec;
  LttTime res;

  sec = t1.tv_sec / (double)f;
  res.tv_sec = sec;
  res.tv_nsec = t1.tv_nsec / (double)f + (sec - res.tv_sec) *
      NANOSECONDS_PER_SECOND;
  res.tv_sec += res.tv_nsec / NANOSECONDS_PER_SECOND;
  res.tv_nsec %= NANOSECONDS_PER_SECOND;
  return res;
}


static inline int ltt_time_compare(LttTime t1, LttTime t2)
{
  if(t1.tv_sec > t2.tv_sec) return 1;
  if(t1.tv_sec < t2.tv_sec) return -1;
  if(t1.tv_nsec > t2.tv_nsec) return 1;
  if(t1.tv_nsec < t2.tv_nsec) return -1;
  return 0;
}


static inline double ltt_time_to_double(LttTime t1)
{
  return (double)t1.tv_sec + (double)t1.tv_nsec / NANOSECONDS_PER_SECOND;
}


static inline LttTime ltt_time_from_double(double t1)
{
  LttTime res;
  res.tv_sec = t1;
  res.tv_nsec = (t1 - res.tv_sec) * NANOSECONDS_PER_SECOND;
  return res;
}

#endif // LTT_TIME_H

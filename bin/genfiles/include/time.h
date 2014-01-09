#ifndef _TIME_H_
#define _TIME_H_
#ifndef _size_t_def_
#define _size_t_def_
typedef unsigned int size_t;
#endif
#ifndef ___clock_t_def_
#define ___clock_t_def_
typedef long __clock_t;
#endif
#ifndef ___time_t_def_
#define ___time_t_def_
typedef long __time_t;
#endif
#ifndef ___clockid_t_def_
#define ___clockid_t_def_
typedef int __clockid_t;
#endif
#ifndef ___timer_t_def_
#define ___timer_t_def_
typedef int __timer_t;
#endif
#ifndef _clock_t_def_
#define _clock_t_def_
typedef __clock_t clock_t;
#endif
#ifndef _time_t_def_
#define _time_t_def_
typedef __time_t time_t;
#endif
#ifndef _clockid_t_def_
#define _clockid_t_def_
typedef __clockid_t clockid_t;
#endif
#ifndef _timer_t_def_
#define _timer_t_def_
typedef __timer_t timer_t;
#endif
#ifndef _timespec_def_
#define _timespec_def_
struct timespec{
  __time_t tv_sec;
  long tv_nsec;
};
#endif
#ifndef _tm_def_
#define _tm_def_
struct tm{
  int tm_sec;
  int tm_min;
  int tm_hour;
  int tm_mday;
  int tm_mon;
  int tm_year;
  int tm_wday;
  int tm_yday;
  int tm_isdst;
  long tm_gmtoff;
  const char * tm_zone;
};
#endif
#ifndef _itimerspec_def_
#define _itimerspec_def_
struct itimerspec{
  struct timespec it_interval;
  struct timespec it_value;
};
#endif
#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME 0
#endif
#ifndef CLOCK_THREAD_CPUTIME_ID
#define CLOCK_THREAD_CPUTIME_ID 3
#endif
#ifndef TIMER_ABSTIME
#define TIMER_ABSTIME 1
#endif
#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 1
#endif
#ifndef CLOCK_PROCESS_CPUTIME_ID
#define CLOCK_PROCESS_CPUTIME_ID 2
#endif
#ifndef CLOCKS_PER_SEC
#define CLOCKS_PER_SEC 1000000l
#endif

  /* Missing declarations for
     strptime
     tzname;
  */

  /* We'd like to
     include {tm}
     but in Linux tm is a struct containing a pointer
          no biggie---just treat it as if it only has one character;
	  don't always use it anyway.
  */

  // We'd like to
  // include { sigevent }
  // but in Linux this pulls in structures that contain void *'s.


  extern "C" char @{26} asctime(const struct tm @);

  extern "C" char @{26}`r asctime_r(const struct tm @, char @{26}`r);

  extern "C" clock_t clock(void);

  // Note, posix says the second arg of clock_getres can be NULL
  extern "C" int clock_getres(clockid_t, struct timespec *);

  extern "C" int clock_gettime(clockid_t, struct timespec @);

  extern "C" int clock_settime(clockid_t, const struct timespec @);

  extern "C" char @{26} ctime(const time_t @);

  extern "C" char @{26}`r ctime_r(const time_t @, char @{26}`r);

  extern "C" double difftime(time_t,time_t);

  extern "C" struct tm @gmtime(const time_t @);

  extern "C" struct tm @`r gmtime_r(const time_t @, struct tm @`r);

  extern "C" struct tm @localtime(const time_t @);

  extern "C" struct tm @`r localtime_r(const time_t @, struct tm @`r);

  extern "C" time_t mktime(const struct tm @);

  // Note, posix says the second arg of nanosleep may be NULL
  //  extern "C" int nanosleep(const struct timespec @, struct timespec *);

  extern size_t strftime(char ?, size_t, const char ?, const struct tm @);

  extern "C" time_t time(time_t *);

  // Omitted because of the sigevent problem
  // Note, posix says the second arg of timer_create may be NULL
  //  extern "C" int timer_create(clockid_t, struct sigevent *, timer_t @);

  extern "C" int timer_delete(timer_t);

  extern "C" int timer_getoverrun(timer_t);

  extern "C" int timer_gettime(timer_t, struct itimerspec @);

  // Note, posix says the third arg of timer_settime may be NULL
  extern "C" int timer_settime(timer_t, int, const struct itimerspec @,
                               struct itimerspec *);

  extern "C" void tzset(void);

  extern "C" long timezone;

  extern "C" int daylight;
#endif

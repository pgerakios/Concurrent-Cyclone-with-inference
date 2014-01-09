#ifndef _SYS_TIMEB_H_
#define _SYS_TIMEB_H_
#ifndef ___time_t_def_
#define ___time_t_def_
typedef long __time_t;
#endif
#ifndef _time_t_def_
#define _time_t_def_
typedef __time_t time_t;
#endif
#ifndef _timeb_def_
#define _timeb_def_
struct timeb{
  time_t time;
  unsigned short millitm;
  short timezone;
  short dstflag;
};
#endif

  extern "C" int ftime(struct timeb @); // (LEGACY)
#endif

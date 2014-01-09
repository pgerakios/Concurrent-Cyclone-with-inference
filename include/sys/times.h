#ifndef _SYS_TIMES_H_
#define _SYS_TIMES_H_
#ifndef ___clock_t_def_
#define ___clock_t_def_
typedef long __clock_t;
#endif
#ifndef _clock_t_def_
#define _clock_t_def_
typedef __clock_t clock_t;
#endif
#ifndef _tms_def_
#define _tms_def_
struct tms{
  clock_t tms_utime;
  clock_t tms_stime;
  clock_t tms_cutime;
  clock_t tms_cstime;
};
#endif

  extern "C" clock_t times(struct tms @);
#endif

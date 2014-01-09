#ifndef _UTIME_H_
#define _UTIME_H_
#ifndef ___time_t_def_
#define ___time_t_def_
typedef long __time_t;
#endif
#ifndef _time_t_def_
#define _time_t_def_
typedef __time_t time_t;
#endif
#ifndef _utimbuf_def_
#define _utimbuf_def_
struct utimbuf{
  __time_t actime;
  __time_t modtime;
};
#endif

  // Note, posix says the second arg of utime may be NULL
  extern "C" int utime(const char @, const struct utimbuf *);
#endif

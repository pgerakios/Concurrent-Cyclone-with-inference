#ifndef _SYS_TIME_H_
#define _SYS_TIME_H_
#ifndef ___time_t_def_
#define ___time_t_def_
typedef long __time_t;
#endif
#ifndef ___suseconds_t_def_
#define ___suseconds_t_def_
typedef long __suseconds_t;
#endif
#ifndef _time_t_def_
#define _time_t_def_
typedef __time_t time_t;
#endif
#ifndef _timeval_def_
#define _timeval_def_
struct timeval{
  __time_t tv_sec;
  __suseconds_t tv_usec;
};
#endif
#ifndef _suseconds_t_def_
#define _suseconds_t_def_
typedef __suseconds_t suseconds_t;
#endif
#ifndef ___fd_mask_def_
#define ___fd_mask_def_
typedef long __fd_mask;
#endif
#ifndef _fd_set_def_
#define _fd_set_def_
typedef struct {__fd_mask __fds_bits[1024U / (8U * sizeof(__fd_mask))];} fd_set;
#endif
#ifndef _timezone_def_
#define _timezone_def_
struct timezone{
  int tz_minuteswest;
  int tz_dsttime;
};
#endif
#ifndef ___itimer_which_def_
#define ___itimer_which_def_
enum __itimer_which{
  ITIMER_REAL = 0,
  ITIMER_VIRTUAL = 1,
  ITIMER_PROF = 2
};
#endif
#ifndef _itimerval_def_
#define _itimerval_def_
struct itimerval{
  struct timeval it_interval;
  struct timeval it_value;
};
#endif
#ifndef ITIMER_VIRTUAL
#define ITIMER_VIRTUAL ITIMER_VIRTUAL
#endif
#ifndef FD_SETSIZE
#define FD_SETSIZE __FD_SETSIZE
#endif
#ifndef ITIMER_PROF
#define ITIMER_PROF ITIMER_PROF
#endif
#ifndef __FD_SETSIZE
#define __FD_SETSIZE 1024
#endif
#ifndef ITIMER_REAL
#define ITIMER_REAL ITIMER_REAL
#endif

  /* FIX: Cygwin defines a struct _types_fd_set and then a typedef with the
     same name.  Our current scheme for omitting repeated declarations does
     not use separate namespaces for structs and typedefs, so the typedef
     is omitted by mistake.  This ugly hack fixes it in Cygwin for now. */
  /* JGM: had to comment this out for now
  #ifdef __types_fd_set_def_
  #ifndef __cygwin_fd_set_hack
  #define __cygwin_fd_set_hack
  typedef struct _types_fd_set _types_fd_set;
  #endif
  #endif
  */

  /* FIX: OpenBSD defines a struct fd_set and then a typedef
     with the same name.  Our current scheme for omitting repeated
     declarations does not use separate namespaces for structs and
     typedefs, so the typedef is omitted by mistake.  This ugly hack
     fixes it for now. */
  #ifndef __fd_set_hack
  #define __fd_set_hack
  #ifdef __OpenBSD__
  typedef struct fd_set fd_set;
  #endif
  #endif

  extern "C" int getitimer(int, struct itimerval @);

  /* The second arg here looks like it is a legacy feature.  The POSIX
     docs say that if it is non-NULL, then the behavior is undefined.
     Therefore, we have a stub that forces the argument to be NULL. */
  extern int gettimeofday(struct timeval @, struct timezone *);

  /* Stubs for these are defined by sys/select.h */
  extern int select(int, fd_set *, fd_set *, fd_set *, struct timeval *);
  extern void FD_CLR(int, fd_set @);
  extern int FD_ISSET(int, fd_set @);
  extern void FD_SET(int, fd_set @);
  extern void FD_ZERO(fd_set @);

  extern "C" int setitimer(int, const struct itimerval @, struct itimerval *);

  extern "C" int utimes(const char @, const struct timeval *{2}); // LEGACY
#endif

#ifndef _SYS_SELECT_H_
#define _SYS_SELECT_H_
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
#ifndef ___sigset_t_def_
#define ___sigset_t_def_
typedef struct {unsigned long __val[1024U / (8U * sizeof(unsigned long))];} __sigset_t;
#endif
#ifndef _sigset_t_def_
#define _sigset_t_def_
typedef __sigset_t sigset_t;
#endif
#ifndef _timespec_def_
#define _timespec_def_
struct timespec{
  __time_t tv_sec;
  long tv_nsec;
};
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
#ifndef FD_SETSIZE
#define FD_SETSIZE __FD_SETSIZE
#endif
#ifndef __FD_SETSIZE
#define __FD_SETSIZE 1024
#endif

  /* FIX: Cygwin defines a struct _types_fd_set and then a typedef with the
     same name.  Our current scheme for omitting repeated declarations does
     not use separate namespaces for structs and typedefs, so the typedef
     is omitted by mistake.  This ugly hack fixes it in Cygwin for now. */
  /* JGM:  had to comment this out for now
  #ifdef __types_fd_set_def_
  #ifndef __cygwin_fd_set_hack
  #define __cygwin_fd_set_hack
  typedef struct _types_fd_set _types_fd_set;
  #endif
  #endif
  */

  /* FIX: *BSD and Solaris define a struct fd_set and then a typedef
     with the same name.  Our current scheme for omitting repeated
     declarations does not use separate namespaces for structs and
     typedefs, so the typedef is omitted by mistake.  This ugly hack
     fixes it for now. */
  /* JGM: we now support defining a struct fd_set and a typedef with
     the same name, so I'm commenting these out for now.
  #ifndef __fd_set_hack
  #define __fd_set_hack
  #ifdef __OpenBSD__
  typedef struct fd_set fd_set;
  #endif
  #ifdef __FreeBSD__
  typedef struct fd_set fd_set;
  #endif
  #ifdef __sparc__
  typedef struct fd_set fd_set;
  #endif
  #endif
  */

  extern int select(int a, fd_set *b, fd_set *c, fd_set *d,
                    struct timeval *e);

  /* Posix says these can be macros or functions.  Typically they are
     macros but in Linux at least they use asm, which we don't support
     in Cyclone.  So, we make them functions. */
  extern void FD_CLR(int, fd_set @);
  extern int FD_ISSET(int, fd_set @);
  extern void FD_SET(int, fd_set @);
  extern void FD_ZERO(fd_set @);
#endif

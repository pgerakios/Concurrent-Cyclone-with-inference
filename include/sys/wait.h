#ifndef _SYS_WAIT_H_
#define _SYS_WAIT_H_
#ifndef ___pid_t_def_
#define ___pid_t_def_
typedef int __pid_t;
#endif
#ifndef ___id_t_def_
#define ___id_t_def_
typedef unsigned int __id_t;
#endif
#ifndef ___time_t_def_
#define ___time_t_def_
typedef long __time_t;
#endif
#ifndef ___suseconds_t_def_
#define ___suseconds_t_def_
typedef long __suseconds_t;
#endif
#ifndef _pid_t_def_
#define _pid_t_def_
typedef __pid_t pid_t;
#endif
#ifndef _id_t_def_
#define _id_t_def_
typedef __id_t id_t;
#endif
#ifndef _timeval_def_
#define _timeval_def_
struct timeval{
  __time_t tv_sec;
  __suseconds_t tv_usec;
};
#endif
#ifndef _rusage_def_
#define _rusage_def_
struct rusage{
  struct timeval ru_utime;
  struct timeval ru_stime;
  long ru_maxrss;
  long ru_ixrss;
  long ru_idrss;
  long ru_isrss;
  long ru_minflt;
  long ru_majflt;
  long ru_nswap;
  long ru_inblock;
  long ru_oublock;
  long ru_msgsnd;
  long ru_msgrcv;
  long ru_nsignals;
  long ru_nvcsw;
  long ru_nivcsw;
};
#endif
#ifndef _idtype_t_def_
#define _idtype_t_def_
typedef enum {P_ALL = 0U,
	      P_PID = 1U,
	      P_PGID = 2U} idtype_t;
#endif
#ifndef _rusage_def_
#define _rusage_def_
struct rusage;
#endif
#ifndef _rusage_def_
#define _rusage_def_
struct rusage;
#endif
#ifndef __WAIT_INT
#define __WAIT_INT(status) (__extension__ (((union { __typeof(status) __in; int __i; }) { .__in = (status) }).__i))
#endif
#ifndef WIFEXITED
#define WIFEXITED(status) __WIFEXITED(__WAIT_INT(status))
#endif
#ifndef WUNTRACED
#define WUNTRACED 2
#endif
#ifndef WCONTINUED
#define WCONTINUED 8
#endif
#ifndef WIFCONTINUED
#define WIFCONTINUED(status) __WIFCONTINUED(__WAIT_INT(status))
#endif
#ifndef WTERMSIG
#define WTERMSIG(status) __WTERMSIG(__WAIT_INT(status))
#endif
#ifndef __WIFSIGNALED
#define __WIFSIGNALED(status) (((signed char) (((status) & 0x7f) + 1) >> 1) > 0)
#endif
#ifndef __WEXITSTATUS
#define __WEXITSTATUS(status) (((status) & 0xff00) >> 8)
#endif
#ifndef __WTERMSIG
#define __WTERMSIG(status) ((status) & 0x7f)
#endif
#ifndef WEXITED
#define WEXITED 4
#endif
#ifndef WNOHANG
#define WNOHANG 1
#endif
#ifndef WEXITSTATUS
#define WEXITSTATUS(status) __WEXITSTATUS(__WAIT_INT(status))
#endif
#ifndef WSTOPSIG
#define WSTOPSIG(status) __WSTOPSIG(__WAIT_INT(status))
#endif
#ifndef __WIFEXITED
#define __WIFEXITED(status) (__WTERMSIG(status) == 0)
#endif
#ifndef __WSTOPSIG
#define __WSTOPSIG(status) __WEXITSTATUS(status)
#endif
#ifndef WIFSTOPPED
#define WIFSTOPPED(status) __WIFSTOPPED(__WAIT_INT(status))
#endif
#ifndef WIFSIGNALED
#define WIFSIGNALED(status) __WIFSIGNALED(__WAIT_INT(status))
#endif
#ifndef WSTOPPED
#define WSTOPPED 2
#endif
#ifndef WNOWAIT
#define WNOWAIT 0x01000000
#endif
#ifndef __WIFCONTINUED
#define __WIFCONTINUED(status) ((status) == __W_CONTINUED)
#endif
#ifndef __WIFSTOPPED
#define __WIFSTOPPED(status) (((status) & 0xff) == 0x7f)
#endif
#ifndef __W_CONTINUED
#define __W_CONTINUED 0xffff
#endif

  /* siginfo_t is omitted because it contains a pointer */
  /* waitid is omitted because it needs siginfo_t */
  /*
    extern "C" int waitid(idtype_t, id_t, siginfo_t @, int);
  */

  extern "C" pid_t wait(int *);

  #define HAVE_WAITPID
  extern "C" pid_t waitpid(pid_t, int *, int);
#endif

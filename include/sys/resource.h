#ifndef _SYS_RESOURCE_H_
#define _SYS_RESOURCE_H_
#ifndef ___rlim_t_def_
#define ___rlim_t_def_
typedef unsigned long __rlim_t;
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
#ifndef _timeval_def_
#define _timeval_def_
struct timeval{
  __time_t tv_sec;
  __suseconds_t tv_usec;
};
#endif
#ifndef ___rlimit_resource_def_
#define ___rlimit_resource_def_
enum __rlimit_resource{
  RLIMIT_CPU = 0,
  RLIMIT_FSIZE = 1,
  RLIMIT_DATA = 2,
  RLIMIT_STACK = 3,
  RLIMIT_CORE = 4,
  __RLIMIT_RSS = 5,
  RLIMIT_NOFILE = 7,
  __RLIMIT_OFILE = 7,
  RLIMIT_AS = 9,
  __RLIMIT_NPROC = 6,
  __RLIMIT_MEMLOCK = 8,
  __RLIMIT_LOCKS = 10,
  __RLIMIT_SIGPENDING = 11,
  __RLIMIT_MSGQUEUE = 12,
  __RLIMIT_NICE = 13,
  __RLIMIT_RTPRIO = 14,
  __RLIMIT_NLIMITS = 15,
  __RLIM_NLIMITS = 15
};
#endif
#ifndef _rlim_t_def_
#define _rlim_t_def_
typedef __rlim_t rlim_t;
#endif
#ifndef _rlimit_def_
#define _rlimit_def_
struct rlimit{
  rlim_t rlim_cur;
  rlim_t rlim_max;
};
#endif
#ifndef ___rusage_who_def_
#define ___rusage_who_def_
enum __rusage_who{
  RUSAGE_SELF = 0,
  RUSAGE_CHILDREN = -1
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
#ifndef ___priority_which_def_
#define ___priority_which_def_
enum __priority_which{
  PRIO_PROCESS = 0,
  PRIO_PGRP = 1,
  PRIO_USER = 2
};
#endif
#ifndef _id_t_def_
#define _id_t_def_
typedef __id_t id_t;
#endif
#ifndef RLIMIT_AS
#define RLIMIT_AS RLIMIT_AS
#endif
#ifndef RUSAGE_CHILDREN
#define RUSAGE_CHILDREN RUSAGE_CHILDREN
#endif
#ifndef PRIO_USER
#define PRIO_USER PRIO_USER
#endif
#ifndef RLIMIT_DATA
#define RLIMIT_DATA RLIMIT_DATA
#endif
#ifndef RLIM_INFINITY
#define RLIM_INFINITY ((unsigned long int)(~0UL))
#endif
#ifndef RLIM_SAVED_CUR
#define RLIM_SAVED_CUR RLIM_INFINITY
#endif
#ifndef PRIO_PGRP
#define PRIO_PGRP PRIO_PGRP
#endif
#ifndef RLIMIT_CPU
#define RLIMIT_CPU RLIMIT_CPU
#endif
#ifndef RUSAGE_SELF
#define RUSAGE_SELF RUSAGE_SELF
#endif
#ifndef RLIMIT_CORE
#define RLIMIT_CORE RLIMIT_CORE
#endif
#ifndef RLIMIT_NOFILE
#define RLIMIT_NOFILE RLIMIT_NOFILE
#endif
#ifndef RLIMIT_FSIZE
#define RLIMIT_FSIZE RLIMIT_FSIZE
#endif
#ifndef RLIM_SAVED_MAX
#define RLIM_SAVED_MAX RLIM_INFINITY
#endif
#ifndef PRIO_PROCESS
#define PRIO_PROCESS PRIO_PROCESS
#endif
#ifndef RLIMIT_STACK
#define RLIMIT_STACK RLIMIT_STACK
#endif

  /* OS X doesn't define id_t */
  #ifndef _id_t_def_
  #define _id_t_def_
  typedef int id_t;
  #endif
  extern "C" int getpriority(int, id_t);

  extern "C" int getrlimit(int, struct rlimit @);

  extern "C" int getrusage(int, struct rusage @);

  extern "C" int setpriority(int, id_t, int);

  extern "C" int setrlimit(int, const struct rlimit @);
#endif

#ifndef _UNISTD_H_
#define _UNISTD_H_
#ifndef _size_t_def_
#define _size_t_def_
typedef unsigned int size_t;
#endif
#ifndef ___uid_t_def_
#define ___uid_t_def_
typedef unsigned int __uid_t;
#endif
#ifndef ___gid_t_def_
#define ___gid_t_def_
typedef unsigned int __gid_t;
#endif
#ifndef ___off_t_def_
#define ___off_t_def_
typedef long __off_t;
#endif
#ifndef ___pid_t_def_
#define ___pid_t_def_
typedef int __pid_t;
#endif
#ifndef ___useconds_t_def_
#define ___useconds_t_def_
typedef unsigned int __useconds_t;
#endif
#ifndef ___ssize_t_def_
#define ___ssize_t_def_
typedef int __ssize_t;
#endif
#ifndef _gid_t_def_
#define _gid_t_def_
typedef __gid_t gid_t;
#endif
#ifndef _uid_t_def_
#define _uid_t_def_
typedef __uid_t uid_t;
#endif
#ifndef _off_t_def_
#define _off_t_def_
typedef __off_t off_t;
#endif
#ifndef _pid_t_def_
#define _pid_t_def_
typedef __pid_t pid_t;
#endif
#ifndef _ssize_t_def_
#define _ssize_t_def_
typedef __ssize_t ssize_t;
#endif
#ifndef ___anonymous_enum_103___def_
#define ___anonymous_enum_103___def_
enum __anonymous_enum_103__{
  _PC_LINK_MAX = 0U,
  _PC_MAX_CANON = 1U,
  _PC_MAX_INPUT = 2U,
  _PC_NAME_MAX = 3U,
  _PC_PATH_MAX = 4U,
  _PC_PIPE_BUF = 5U,
  _PC_CHOWN_RESTRICTED = 6U,
  _PC_NO_TRUNC = 7U,
  _PC_VDISABLE = 8U,
  _PC_SYNC_IO = 9U,
  _PC_ASYNC_IO = 10U,
  _PC_PRIO_IO = 11U,
  _PC_SOCK_MAXBUF = 12U,
  _PC_FILESIZEBITS = 13U,
  _PC_REC_INCR_XFER_SIZE = 14U,
  _PC_REC_MAX_XFER_SIZE = 15U,
  _PC_REC_MIN_XFER_SIZE = 16U,
  _PC_REC_XFER_ALIGN = 17U,
  _PC_ALLOC_SIZE_MIN = 18U,
  _PC_SYMLINK_MAX = 19U,
  _PC_2_SYMLINKS = 20U
};
#endif
#ifndef ___anonymous_enum_105___def_
#define ___anonymous_enum_105___def_
enum __anonymous_enum_105__{
  _CS_PATH = 0U,
  _CS_V6_WIDTH_RESTRICTED_ENVS = 1U,
  _CS_GNU_LIBC_VERSION = 2U,
  _CS_GNU_LIBPTHREAD_VERSION = 3U,
  _CS_LFS_CFLAGS = 1000,
  _CS_LFS_LDFLAGS = 1001U,
  _CS_LFS_LIBS = 1002U,
  _CS_LFS_LINTFLAGS = 1003U,
  _CS_LFS64_CFLAGS = 1004U,
  _CS_LFS64_LDFLAGS = 1005U,
  _CS_LFS64_LIBS = 1006U,
  _CS_LFS64_LINTFLAGS = 1007U,
  _CS_XBS5_ILP32_OFF32_CFLAGS = 1100,
  _CS_XBS5_ILP32_OFF32_LDFLAGS = 1101U,
  _CS_XBS5_ILP32_OFF32_LIBS = 1102U,
  _CS_XBS5_ILP32_OFF32_LINTFLAGS = 1103U,
  _CS_XBS5_ILP32_OFFBIG_CFLAGS = 1104U,
  _CS_XBS5_ILP32_OFFBIG_LDFLAGS = 1105U,
  _CS_XBS5_ILP32_OFFBIG_LIBS = 1106U,
  _CS_XBS5_ILP32_OFFBIG_LINTFLAGS = 1107U,
  _CS_XBS5_LP64_OFF64_CFLAGS = 1108U,
  _CS_XBS5_LP64_OFF64_LDFLAGS = 1109U,
  _CS_XBS5_LP64_OFF64_LIBS = 1110U,
  _CS_XBS5_LP64_OFF64_LINTFLAGS = 1111U,
  _CS_XBS5_LPBIG_OFFBIG_CFLAGS = 1112U,
  _CS_XBS5_LPBIG_OFFBIG_LDFLAGS = 1113U,
  _CS_XBS5_LPBIG_OFFBIG_LIBS = 1114U,
  _CS_XBS5_LPBIG_OFFBIG_LINTFLAGS = 1115U,
  _CS_POSIX_V6_ILP32_OFF32_CFLAGS = 1116U,
  _CS_POSIX_V6_ILP32_OFF32_LDFLAGS = 1117U,
  _CS_POSIX_V6_ILP32_OFF32_LIBS = 1118U,
  _CS_POSIX_V6_ILP32_OFF32_LINTFLAGS = 1119U,
  _CS_POSIX_V6_ILP32_OFFBIG_CFLAGS = 1120U,
  _CS_POSIX_V6_ILP32_OFFBIG_LDFLAGS = 1121U,
  _CS_POSIX_V6_ILP32_OFFBIG_LIBS = 1122U,
  _CS_POSIX_V6_ILP32_OFFBIG_LINTFLAGS = 1123U,
  _CS_POSIX_V6_LP64_OFF64_CFLAGS = 1124U,
  _CS_POSIX_V6_LP64_OFF64_LDFLAGS = 1125U,
  _CS_POSIX_V6_LP64_OFF64_LIBS = 1126U,
  _CS_POSIX_V6_LP64_OFF64_LINTFLAGS = 1127U,
  _CS_POSIX_V6_LPBIG_OFFBIG_CFLAGS = 1128U,
  _CS_POSIX_V6_LPBIG_OFFBIG_LDFLAGS = 1129U,
  _CS_POSIX_V6_LPBIG_OFFBIG_LIBS = 1130U,
  _CS_POSIX_V6_LPBIG_OFFBIG_LINTFLAGS = 1131U
};
#endif
#ifndef _CS_POSIX_V6_ILP32_OFFBIG_LIBS
#define _CS_POSIX_V6_ILP32_OFFBIG_LIBS _CS_POSIX_V6_ILP32_OFFBIG_LIBS
#endif
#ifndef _POSIX_THREAD_PRIORITY_SCHEDULING
#define _POSIX_THREAD_PRIORITY_SCHEDULING 200112L
#endif
#ifndef _POSIX_SHARED_MEMORY_OBJECTS
#define _POSIX_SHARED_MEMORY_OBJECTS 200112L
#endif
#ifndef _POSIX2_SW_DEV
#define _POSIX2_SW_DEV 200112L
#endif
#ifndef _POSIX_THREAD_SAFE_FUNCTIONS
#define _POSIX_THREAD_SAFE_FUNCTIONS 200112L
#endif
#ifndef _POSIX_PRIORITIZED_IO
#define _POSIX_PRIORITIZED_IO 200112L
#endif
#ifndef _XOPEN_CRYPT
#define _XOPEN_CRYPT 1
#endif
#ifndef _POSIX_CLOCK_SELECTION
#define _POSIX_CLOCK_SELECTION -1
#endif
#ifndef _POSIX_TYPED_MEMORY_OBJECTS
#define _POSIX_TYPED_MEMORY_OBJECTS -1
#endif
#ifndef _XOPEN_VERSION
#define _XOPEN_VERSION 4
#endif
#ifndef _POSIX_TRACE_EVENT_FILTER
#define _POSIX_TRACE_EVENT_FILTER -1
#endif
#ifndef _POSIX_SPIN_LOCKS
#define _POSIX_SPIN_LOCKS 200112L
#endif
#ifndef _CS_PATH
#define _CS_PATH _CS_PATH
#endif
#ifndef _POSIX_CPUTIME
#define _POSIX_CPUTIME 0
#endif
#ifndef _POSIX_JOB_CONTROL
#define _POSIX_JOB_CONTROL 1
#endif
#ifndef _PC_ASYNC_IO
#define _PC_ASYNC_IO _PC_ASYNC_IO
#endif
#ifndef _POSIX_THREAD_CPUTIME
#define _POSIX_THREAD_CPUTIME 0
#endif
#ifndef _POSIX_READER_WRITER_LOCKS
#define _POSIX_READER_WRITER_LOCKS 200112L
#endif
#ifndef _PC_PRIO_IO
#define _PC_PRIO_IO _PC_PRIO_IO
#endif
#ifndef _POSIX_SYNCHRONIZED_IO
#define _POSIX_SYNCHRONIZED_IO 200112L
#endif
#ifndef _CS_POSIX_V6_LPBIG_OFFBIG_CFLAGS
#define _CS_POSIX_V6_LPBIG_OFFBIG_CFLAGS _CS_POSIX_V6_LPBIG_OFFBIG_CFLAGS
#endif
#ifndef _CS_POSIX_V6_ILP32_OFFBIG_CFLAGS
#define _CS_POSIX_V6_ILP32_OFFBIG_CFLAGS _CS_POSIX_V6_ILP32_OFFBIG_CFLAGS
#endif
#ifndef _XOPEN_REALTIME_THREADS
#define _XOPEN_REALTIME_THREADS 1
#endif
#ifndef _XOPEN_ENH_I18N
#define _XOPEN_ENH_I18N 1
#endif
#ifndef _CS_POSIX_V6_LPBIG_OFFBIG_LDFLAGS
#define _CS_POSIX_V6_LPBIG_OFFBIG_LDFLAGS _CS_POSIX_V6_LPBIG_OFFBIG_LDFLAGS
#endif
#ifndef _POSIX_SPAWN
#define _POSIX_SPAWN 200112L
#endif
#ifndef _CS_POSIX_V6_ILP32_OFFBIG_LDFLAGS
#define _CS_POSIX_V6_ILP32_OFFBIG_LDFLAGS _CS_POSIX_V6_ILP32_OFFBIG_LDFLAGS
#endif
#ifndef _PC_MAX_INPUT
#define _PC_MAX_INPUT _PC_MAX_INPUT
#endif
#ifndef _PC_FILESIZEBITS
#define _PC_FILESIZEBITS _PC_FILESIZEBITS
#endif
#ifndef F_OK
#define F_OK 0
#endif
#ifndef SEEK_CUR
#define SEEK_CUR 1
#endif
#ifndef STDERR_FILENO
#define STDERR_FILENO 2
#endif
#ifndef _POSIX_CHOWN_RESTRICTED
#define _POSIX_CHOWN_RESTRICTED 1
#endif
#ifndef _PC_CHOWN_RESTRICTED
#define _PC_CHOWN_RESTRICTED _PC_CHOWN_RESTRICTED
#endif
#ifndef _POSIX_THREAD_SPORADIC_SERVER
#define _POSIX_THREAD_SPORADIC_SERVER -1
#endif
#ifndef _PC_REC_MAX_XFER_SIZE
#define _PC_REC_MAX_XFER_SIZE _PC_REC_MAX_XFER_SIZE
#endif
#ifndef _POSIX_PRIORITY_SCHEDULING
#define _POSIX_PRIORITY_SCHEDULING 200112L
#endif
#ifndef _POSIX_NO_TRUNC
#define _POSIX_NO_TRUNC 1
#endif
#ifndef _POSIX_REGEXP
#define _POSIX_REGEXP 1
#endif
#ifndef _POSIX_SPORADIC_SERVER
#define _POSIX_SPORADIC_SERVER -1
#endif
#ifndef _POSIX_MEMORY_PROTECTION
#define _POSIX_MEMORY_PROTECTION 200112L
#endif
#ifndef _POSIX2_LOCALEDEF
#define _POSIX2_LOCALEDEF 200112L
#endif
#ifndef _POSIX_TIMERS
#define _POSIX_TIMERS 200112L
#endif
#ifndef _PC_VDISABLE
#define _PC_VDISABLE _PC_VDISABLE
#endif
#ifndef _CS_POSIX_V6_LPBIG_OFFBIG_LIBS
#define _CS_POSIX_V6_LPBIG_OFFBIG_LIBS _CS_POSIX_V6_LPBIG_OFFBIG_LIBS
#endif
#ifndef _PC_LINK_MAX
#define _PC_LINK_MAX _PC_LINK_MAX
#endif
#ifndef _POSIX_SHELL
#define _POSIX_SHELL 1
#endif
#ifndef _XOPEN_SHM
#define _XOPEN_SHM 1
#endif
#ifndef _POSIX_ADVISORY_INFO
#define _POSIX_ADVISORY_INFO 200112L
#endif
#ifndef F_LOCK
#define F_LOCK 1
#endif
#ifndef _CS_POSIX_V6_LP64_OFF64_LDFLAGS
#define _CS_POSIX_V6_LP64_OFF64_LDFLAGS _CS_POSIX_V6_LP64_OFF64_LDFLAGS
#endif
#ifndef _XOPEN_LEGACY
#define _XOPEN_LEGACY 1
#endif
#ifndef _POSIX_THREAD_ATTR_STACKSIZE
#define _POSIX_THREAD_ATTR_STACKSIZE 200112L
#endif
#ifndef _PC_NAME_MAX
#define _PC_NAME_MAX _PC_NAME_MAX
#endif
#ifndef _PC_MAX_CANON
#define _PC_MAX_CANON _PC_MAX_CANON
#endif
#ifndef _PC_NO_TRUNC
#define _PC_NO_TRUNC _PC_NO_TRUNC
#endif
#ifndef _POSIX_VERSION
#define _POSIX_VERSION 200112L
#endif
#ifndef _PC_REC_INCR_XFER_SIZE
#define _PC_REC_INCR_XFER_SIZE _PC_REC_INCR_XFER_SIZE
#endif
#ifndef F_ULOCK
#define F_ULOCK 0
#endif
#ifndef _POSIX_THREAD_PROCESS_SHARED
#define _POSIX_THREAD_PROCESS_SHARED -1
#endif
#ifndef _POSIX_TRACE_INHERIT
#define _POSIX_TRACE_INHERIT -1
#endif
#ifndef _CS_POSIX_V6_ILP32_OFF32_LDFLAGS
#define _CS_POSIX_V6_ILP32_OFF32_LDFLAGS _CS_POSIX_V6_ILP32_OFF32_LDFLAGS
#endif
#ifndef _POSIX_MEMLOCK
#define _POSIX_MEMLOCK 200112L
#endif
#ifndef _POSIX_THREAD_ATTR_STACKADDR
#define _POSIX_THREAD_ATTR_STACKADDR 200112L
#endif
#ifndef _PC_PATH_MAX
#define _PC_PATH_MAX _PC_PATH_MAX
#endif
#ifndef _POSIX_THREADS
#define _POSIX_THREADS 200112L
#endif
#ifndef _CS_POSIX_V6_ILP32_OFF32_CFLAGS
#define _CS_POSIX_V6_ILP32_OFF32_CFLAGS _CS_POSIX_V6_ILP32_OFF32_CFLAGS
#endif
#ifndef F_TLOCK
#define F_TLOCK 2
#endif
#ifndef F_TEST
#define F_TEST 3
#endif
#ifndef _POSIX_TIMEOUTS
#define _POSIX_TIMEOUTS 200112L
#endif
#ifndef SEEK_SET
#define SEEK_SET 0
#endif
#ifndef W_OK
#define W_OK 2
#endif
#ifndef _POSIX_ASYNCHRONOUS_IO
#define _POSIX_ASYNCHRONOUS_IO 200112L
#endif
#ifndef _POSIX_MONOTONIC_CLOCK
#define _POSIX_MONOTONIC_CLOCK 0
#endif
#ifndef _PC_SYNC_IO
#define _PC_SYNC_IO _PC_SYNC_IO
#endif
#ifndef _CS_POSIX_V6_LP64_OFF64_CFLAGS
#define _CS_POSIX_V6_LP64_OFF64_CFLAGS _CS_POSIX_V6_LP64_OFF64_CFLAGS
#endif
#ifndef _POSIX_RAW_SOCKETS
#define _POSIX_RAW_SOCKETS 200112L
#endif
#ifndef _PC_REC_XFER_ALIGN
#define _PC_REC_XFER_ALIGN _PC_REC_XFER_ALIGN
#endif
#ifndef _PC_REC_MIN_XFER_SIZE
#define _PC_REC_MIN_XFER_SIZE _PC_REC_MIN_XFER_SIZE
#endif
#ifndef _XOPEN_REALTIME
#define _XOPEN_REALTIME 1
#endif
#ifndef _POSIX_MEMLOCK_RANGE
#define _POSIX_MEMLOCK_RANGE 200112L
#endif
#ifndef STDIN_FILENO
#define STDIN_FILENO 0
#endif
#ifndef _POSIX_REALTIME_SIGNALS
#define _POSIX_REALTIME_SIGNALS 200112L
#endif
#ifndef _POSIX_TRACE
#define _POSIX_TRACE -1
#endif
#ifndef _POSIX_FSYNC
#define _POSIX_FSYNC 200112L
#endif
#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1
#endif
#ifndef _POSIX_VDISABLE
#define _POSIX_VDISABLE '\0'
#endif
#ifndef _POSIX_THREAD_PRIO_INHERIT
#define _POSIX_THREAD_PRIO_INHERIT -1
#endif
#ifndef _POSIX_SEMAPHORES
#define _POSIX_SEMAPHORES 200112L
#endif
#ifndef _PC_ALLOC_SIZE_MIN
#define _PC_ALLOC_SIZE_MIN _PC_ALLOC_SIZE_MIN
#endif
#ifndef SEEK_END
#define SEEK_END 2
#endif
#ifndef _XOPEN_UNIX
#define _XOPEN_UNIX 1
#endif
#ifndef _POSIX_MAPPED_FILES
#define _POSIX_MAPPED_FILES 200112L
#endif
#ifndef _CS_POSIX_V6_ILP32_OFF32_LIBS
#define _CS_POSIX_V6_ILP32_OFF32_LIBS _CS_POSIX_V6_ILP32_OFF32_LIBS
#endif
#ifndef _POSIX_MESSAGE_PASSING
#define _POSIX_MESSAGE_PASSING 200112L
#endif
#ifndef R_OK
#define R_OK 4
#endif
#ifndef _POSIX2_VERSION
#define _POSIX2_VERSION 200112L
#endif
#ifndef _POSIX_THREAD_PRIO_PROTECT
#define _POSIX_THREAD_PRIO_PROTECT -1
#endif
#ifndef _CS_POSIX_V6_LP64_OFF64_LIBS
#define _CS_POSIX_V6_LP64_OFF64_LIBS _CS_POSIX_V6_LP64_OFF64_LIBS
#endif
#ifndef _POSIX2_CHAR_TERM
#define _POSIX2_CHAR_TERM 200112L
#endif
#ifndef _POSIX_ASYNC_IO
#define _POSIX_ASYNC_IO 1
#endif
#ifndef _POSIX2_C_BIND
#define _POSIX2_C_BIND 200112L
#endif
#ifndef _POSIX2_C_DEV
#define _POSIX2_C_DEV 200112L
#endif
#ifndef _POSIX_TRACE_LOG
#define _POSIX_TRACE_LOG -1
#endif
#ifndef _POSIX_SAVED_IDS
#define _POSIX_SAVED_IDS 1
#endif
#ifndef _POSIX_BARRIERS
#define _POSIX_BARRIERS 200112L
#endif
#ifndef X_OK
#define X_OK 1
#endif
#ifndef _PC_PIPE_BUF
#define _PC_PIPE_BUF _PC_PIPE_BUF
#endif

  // EXTREMELY irritating: under Linux, useconds_t is not defined
  // in sys/wait.h, CONTRARY to the posix spec.  Instead __useconds_t
  // is defined.  So we include __useconds_t above and do the following:
  #if defined(___useconds_t_def_) && !defined(_useconds_t_def_)
  #define _useconds_t_def_
  typedef __useconds_t useconds_t;
  #endif

  /* Cygwin does not define this type so we take a guess :-( */
  /* FIX: make this specific to cygwin ?? */
  #ifndef _useconds_t_def_
  #define _useconds_t_def_
  typedef unsigned int useconds_t;
  #endif

  // We'd like to do
  // include {
  // _SC_2_C_BIND
  // _SC_2_C_DEV
  // _SC_2_C_VERSION
  // _SC_2_CHAR_TERM
  // _SC_2_FORT_DEV
  // _SC_2_FORT_RUN
  // _SC_2_LOCALEDEF
  // _SC_2_PBS
  // _SC_2_PBS_ACCOUNTING
  // _SC_2_PBS_CHECKPOINT
  // _SC_2_PBS_LOCATE
  // _SC_2_PBS_MESSAGE
  // _SC_2_PBS_TRACK
  // _SC_2_SW_DEV
  // _SC_2_UPE
  // _SC_2_VERSION
  // _SC_ADVISORY_INFO
  // _SC_ARG_MAX
  // _SC_AIO_LISTIO_MAX
  // _SC_AIO_MAX
  // _SC_AIO_PRIO_DELTA_MAX
  // _SC_ASYNCHRONOUS_IO
  // _SC_ATEXIT_MAX
  // _SC_BARRIERS
  // _SC_BC_BASE_MAX
  // _SC_BC_DIM_MAX
  // _SC_BC_SCALE_MAX
  // _SC_BC_STRING_MAX
  // _SC_CHILD_MAX
  // _SC_CLK_TCK
  // _SC_CLOCK_SELECTION
  // _SC_COLL_WEIGHTS_MAX
  // _SC_CPUTIME
  // _SC_DELAYTIMER_MAX
  // _SC_EXPR_NEST_MAX
  // _SC_FILE_LOCKING
  // _SC_FSYNC
  // _SC_GETGR_R_SIZE_MAX
  // _SC_GETPW_R_SIZE_MAX
  // _SC_HOST_NAME_MAX
  // _SC_IOV_MAX
  // _SC_JOB_CONTROL
  // _SC_LINE_MAX
  // _SC_LOGIN_NAME_MAX
  // _SC_MAPPED_FILES
  // _SC_MEMLOCK
  // _SC_MEMLOCK_RANGE
  // _SC_MEMORY_PROTECTION
  // _SC_MESSAGE_PASSING
  // _SC_MONOTONIC_CLOCK
  // _SC_MQ_OPEN_MAX
  // _SC_MQ_PRIO_MAX
  // _SC_NGROUPS_MAX
  // _SC_OPEN_MAX
  // _SC_PAGE_SIZE
  // _SC_PAGESIZE
  // _SC_PRIORITIZED_IO
  // _SC_PRIORITY_SCHEDULING
  // _SC_RE_DUP_MAX
  // _SC_READER_WRITER_LOCKS
  // _SC_REALTIME_SIGNALS
  // _SC_REGEXP
  // _SC_RTSIG_MAX
  // _SC_SAVED_IDS
  // _SC_SEMAPHORES
  // _SC_SEM_NSEMS_MAX
  // _SC_SEM_VALUE_MAX
  // _SC_SHARED_MEMORY_OBJECTS
  // _SC_SHELL
  // _SC_SIGQUEUE_MAX
  // _SC_SPAWN
  // _SC_SPIN_LOCKS
  // _SC_SPORADIC_SERVER
  // _SC_STREAM_MAX
  // _SC_SYNCHRONIZED_IO
  // _SC_THREAD_ATTR_STACKADDR
  // _SC_THREAD_ATTR_STACKSIZE
  // _SC_THREAD_CPUTIME
  // _SC_THREAD_DESTRUCTOR_ITERATIONS
  // _SC_THREAD_KEYS_MAX
  // _SC_THREAD_PRIO_INHERIT
  // _SC_THREAD_PRIO_PROTECT
  // _SC_THREAD_PRIORITY_SCHEDULING
  // _SC_THREAD_PROCESS_SHARED
  // _SC_THREAD_SAFE_FUNCTIONS
  // _SC_THREAD_SPORADIC_SERVER
  // _SC_THREAD_STACK_MIN
  // _SC_THREAD_THREADS_MAX
  // _SC_TIMEOUTS
  // _SC_THREADS
  // _SC_TIMER_MAX
  // _SC_TIMERS
  // _SC_TRACE
  // _SC_TRACE_EVENT_FILTER
  // _SC_TRACE_INHERIT
  // _SC_TRACE_LOG
  // _SC_TTY_NAME_MAX
  // _SC_TYPED_MEMORY_OBJECTS
  // _SC_TZNAME_MAX
  // _SC_V6_ILP32_OFF32
  // _SC_V6_ILP32_OFFBIG
  // _SC_V6_LP64_OFF64
  // _SC_V6_LPBIG_OFFBIG
  // _SC_VERSION
  // _SC_XBS5_ILP32_OFF32
  // _SC_XBS5_ILP32_OFFBIG
  // _SC_XBS5_LP64_OFF64
  // _SC_XBS5_LPBIG_OFFBIG
  // _SC_XOPEN_CRYPT
  // _SC_XOPEN_ENH_I18N
  // _SC_XOPEN_LEGACY
  // _SC_XOPEN_REALTIME
  // _SC_XOPEN_REALTIME_THREADS
  // _SC_XOPEN_SHM
  // _SC_XOPEN_STREAMS
  // _SC_XOPEN_UNIX
  // _SC_XOPEN_VERSION
  // _SC_XOPEN_XCU_VERSION
  // }
  // but we can't because under linux this pulls in some code like
  //   enum { foo, bar = foo };
  // and cyclone complains that the expression (foo) of field bar is not
  // a constant.

// Need to #include our getopt.h file to grab getopt and friends
  #include <getopt.h>

// Under Linux, crypt doesn't always get included here for some reason
  // Hmmmm... same for Cygwin
#if defined(__linux__) || defined(__CYGWIN32__) || defined(__CYGWIN__)
  extern "C" char *crypt (const char @__key, const char @__salt);
#endif

  extern "C" int access(const char @,int);

  extern "C" unsigned alarm(unsigned);

  extern "C" int chdir(const char @);

  extern "C" int chown(const char @,uid_t,gid_t);

  extern "C" int chroot(const char @);

  extern "C" int close(int);

  // Not supported yet
  // extern size_t confstr(int, char *, size_t);

  extern "C" int dup(int);

  extern "C" int dup2(int, int);

  //  extern int execl(const char ?, const char ?, ... const char ?);

  // Not supported yet
  // extern int execle(const char *, const char *, ...);

  extern int execlp(const char @, const char @`r, ... const char *`r);

  // Not supported yet
  // extern int execv(const char *, char *const []);

  // FIX: what did I lose by removing the const from this spec?
  //  extern int execve(const char ?, const char ? const ?, const char ? const ?);
    extern "C" int execve(const char @ path, 
			  char *`r::TR* @zeroterm argv, char *`r2::TR* @zeroterm envp);
//    extern int execve(const char ?, char ? const ?, char ? const ?);

  extern "C" int execvp(const char @ file, const char ** @zeroterm argv);

  extern "C" void _exit(int);

  extern "C" int fchown(int, uid_t, gid_t);

  extern "C" int fchdir(int);

  extern "C" int fdatasync(int);

  extern "C" pid_t fork(void);

  extern "C" long fpathconf(int, int);

  extern "C" int fsync(int);

  extern "C" int ftruncate(int, off_t);

  extern char ?`r getcwd(char ?`r buf, size_t size);

  extern "C" gid_t getegid(void);

  extern "C" uid_t geteuid(void);

  extern "C" gid_t getgid(void);

  // Not supported yet
  // extern int getgroups(int, gid_t []);

  extern "C" long gethostid(void);

  extern int gethostname(char ? @nozeroterm, size_t);

  // Not supported yet
  // extern char *getlogin(void);

  // Not supported yet
  // extern int getopt(int, char * const [], const char *);

  /* Note, getpass is not required by POSIX */
  extern "C" char * getpass(const char @);

  extern "C" pid_t getpgrp(void);

  extern "C" pid_t getpid(void);

  extern "C" pid_t getppid(void);

  extern "C" uid_t getuid(void);

  // Not supported yet
  // extern char *getwd(char *); (LEGACY)

  extern "C" int isatty(int);

  // Not supported yet
  // extern int lchown(const char *, uid_t, gid_t);

  extern "C" int link(const char @,const char @);

  extern "C" int lockf(int, int, off_t);

  extern "C" off_t lseek(int, off_t, int);

  extern "C" int nice(int);

  // Not supported yet
  // extern long pathconf(const char *, int);

  extern "C" int pause(void);

  extern "C" int pipe(int @{2});

  extern ssize_t read(int, char ? @nozeroterm, size_t);

  // Not supported yet
  // extern ssize_t readlink(const char *restrict, char *restrict, size_t);

  extern "C" int rmdir(const char @);

  extern "C" int setegid(gid_t);

  extern "C" int seteuid(uid_t);

  extern "C" int setgid(gid_t);

  extern "C" int setpgid(pid_t, pid_t);

  extern "C" pid_t setpgrp(void);

  extern "C" int setregid(gid_t, gid_t);

  extern "C" int setreuid(uid_t, uid_t);

  extern "C" pid_t setsid(void);

  extern "C" int setuid(uid_t);

  extern "C" unsigned sleep(unsigned);

  extern "C" int symlink(const char @ path1, const char @ path2);

  extern "C" void sync(void);

  extern "C" long sysconf(int);

  extern "C" pid_t tcgetpgrp(int);

  extern "C" int tcsetpgrp(int, pid_t);

  extern int truncate(const char @, off_t);

  // Not supported yet
  // extern char *ttyname(int);

  // Not supported yet
  // extern int ttyname_r(int, char *, size_t);

  extern "C" useconds_t ualarm(useconds_t, useconds_t);

  extern "C" int unlink(const char @ pathname);

  extern "C" int usleep(useconds_t);

  extern "C" pid_t vfork(void);

  extern ssize_t write(int, const char ? @nozeroterm, size_t);

  /* intptr_t is omitted */
  /* optarg is omitted */

// Conflict with hand-coded getopt.h so not supported yet
//  extern "C" int optind;

// Conflict with hand-coded getopt.h so not supported yet
//  extern "C" int opterr;

// Conflict with hand-coded getopt.h so not supported yet
//  extern "C" int optopt;
#endif

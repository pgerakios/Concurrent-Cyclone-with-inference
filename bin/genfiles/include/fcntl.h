#ifndef _FCNTL_H_
#define _FCNTL_H_
#ifndef ___mode_t_def_
#define ___mode_t_def_
typedef unsigned int __mode_t;
#endif
#ifndef ___off_t_def_
#define ___off_t_def_
typedef long __off_t;
#endif
#ifndef ___pid_t_def_
#define ___pid_t_def_
typedef int __pid_t;
#endif
#ifndef _mode_t_def_
#define _mode_t_def_
typedef __mode_t mode_t;
#endif
#ifndef _off_t_def_
#define _off_t_def_
typedef __off_t off_t;
#endif
#ifndef _pid_t_def_
#define _pid_t_def_
typedef __pid_t pid_t;
#endif
#ifndef _flock_def_
#define _flock_def_
struct flock{
  short l_type;
  short l_whence;
  __off_t l_start;
  __off_t l_len;
  __pid_t l_pid;
};
#endif
#ifndef O_NOCTTY
#define O_NOCTTY 0400
#endif
#ifndef O_ACCMODE
#define O_ACCMODE 0003
#endif
#ifndef F_RDLCK
#define F_RDLCK 0
#endif
#ifndef F_GETLK
#define F_GETLK 5
#endif
#ifndef F_DUPFD
#define F_DUPFD 0
#endif
#ifndef O_WRONLY
#define O_WRONLY 01
#endif
#ifndef O_APPEND
#define O_APPEND 02000
#endif
#ifndef FD_CLOEXEC
#define FD_CLOEXEC 1
#endif
#ifndef O_NONBLOCK
#define O_NONBLOCK 04000
#endif
#ifndef F_UNLCK
#define F_UNLCK 2
#endif
#ifndef F_GETFD
#define F_GETFD 1
#endif
#ifndef F_GETFL
#define F_GETFL 3
#endif
#ifndef O_RSYNC
#define O_RSYNC O_SYNC
#endif
#ifndef O_DSYNC
#define O_DSYNC O_SYNC
#endif
#ifndef F_SETOWN
#define F_SETOWN 8
#endif
#ifndef O_RDWR
#define O_RDWR 02
#endif
#ifndef F_SETFL
#define F_SETFL 4
#endif
#ifndef F_GETOWN
#define F_GETOWN 9
#endif
#ifndef F_SETLK
#define F_SETLK 6
#endif
#ifndef O_SYNC
#define O_SYNC 010000
#endif
#ifndef O_CREAT
#define O_CREAT 0100
#endif
#ifndef F_WRLCK
#define F_WRLCK 1
#endif
#ifndef F_SETLKW
#define F_SETLKW 7
#endif
#ifndef O_RDONLY
#define O_RDONLY 00
#endif
#ifndef O_TRUNC
#define O_TRUNC 01000
#endif
#ifndef O_EXCL
#define O_EXCL 0200
#endif
#ifndef F_SETFD
#define F_SETFD 2
#endif

  // FIX: ADV is not supported

  datatype FcntlArg<`r::R> {
    Flock(struct flock @`r);
    Long(long);
  };
  typedef datatype FcntlArg<`r>@`r fcntlarg_t<`r>;

  extern int fcntl(int fd, int cmd, ... inject fcntlarg_t);
  extern int open(const char *, int, ... mode_t);
  // FIX: should arg be non-NULL?
  extern "C" int creat(const char *,mode_t);
#endif

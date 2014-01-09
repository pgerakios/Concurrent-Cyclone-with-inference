#ifndef _SYS_STAT_H_
#define _SYS_STAT_H_
#ifndef ___u_quad_t_def_
#define ___u_quad_t_def_
typedef unsigned long long __u_quad_t;
#endif
#ifndef ___dev_t_def_
#define ___dev_t_def_
typedef __u_quad_t __dev_t;
#endif
#ifndef ___uid_t_def_
#define ___uid_t_def_
typedef unsigned int __uid_t;
#endif
#ifndef ___gid_t_def_
#define ___gid_t_def_
typedef unsigned int __gid_t;
#endif
#ifndef ___ino_t_def_
#define ___ino_t_def_
typedef unsigned long __ino_t;
#endif
#ifndef ___mode_t_def_
#define ___mode_t_def_
typedef unsigned int __mode_t;
#endif
#ifndef ___nlink_t_def_
#define ___nlink_t_def_
typedef unsigned int __nlink_t;
#endif
#ifndef ___off_t_def_
#define ___off_t_def_
typedef long __off_t;
#endif
#ifndef ___time_t_def_
#define ___time_t_def_
typedef long __time_t;
#endif
#ifndef ___blksize_t_def_
#define ___blksize_t_def_
typedef long __blksize_t;
#endif
#ifndef ___blkcnt_t_def_
#define ___blkcnt_t_def_
typedef long __blkcnt_t;
#endif
#ifndef _ino_t_def_
#define _ino_t_def_
typedef __ino_t ino_t;
#endif
#ifndef _dev_t_def_
#define _dev_t_def_
typedef __dev_t dev_t;
#endif
#ifndef _gid_t_def_
#define _gid_t_def_
typedef __gid_t gid_t;
#endif
#ifndef _mode_t_def_
#define _mode_t_def_
typedef __mode_t mode_t;
#endif
#ifndef _nlink_t_def_
#define _nlink_t_def_
typedef __nlink_t nlink_t;
#endif
#ifndef _uid_t_def_
#define _uid_t_def_
typedef __uid_t uid_t;
#endif
#ifndef _off_t_def_
#define _off_t_def_
typedef __off_t off_t;
#endif
#ifndef _time_t_def_
#define _time_t_def_
typedef __time_t time_t;
#endif
#ifndef _timespec_def_
#define _timespec_def_
struct timespec{
  __time_t tv_sec;
  long tv_nsec;
};
#endif
#ifndef _blkcnt_t_def_
#define _blkcnt_t_def_
typedef __blkcnt_t blkcnt_t;
#endif
#ifndef _stat_def_
#define _stat_def_
struct stat{
  __dev_t st_dev;
  unsigned short __pad1;
  __ino_t st_ino;
  __mode_t st_mode;
  __nlink_t st_nlink;
  __uid_t st_uid;
  __gid_t st_gid;
  __dev_t st_rdev;
  unsigned short __pad2;
  __off_t st_size;
  __blksize_t st_blksize;
  __blkcnt_t st_blocks;
  struct timespec st_atim;
  struct timespec st_mtim;
  struct timespec st_ctim;
  unsigned long __unused4;
  unsigned long __unused5;
};
#endif
#ifndef _stat_def_
#define _stat_def_
extern int  __attribute__((nothrow))stat<`GR2::R,`GR3::R>(const char *`GR2 __file,
							  struct stat *`GR3 __buf);
#endif
#ifndef __S_IFBLK
#define __S_IFBLK 0060000
#endif
#ifndef S_ISCHR
#define S_ISCHR(mode) __S_ISTYPE((mode), __S_IFCHR)
#endif
#ifndef __S_ISTYPE
#define __S_ISTYPE(mode,mask) (((mode) & __S_IFMT) == (mask))
#endif
#ifndef S_TYPEISSEM
#define S_TYPEISSEM(buf) __S_TYPEISSEM(buf)
#endif
#ifndef __S_IREAD
#define __S_IREAD 0400
#endif
#ifndef S_ISGID
#define S_ISGID __S_ISGID
#endif
#ifndef S_IFBLK
#define S_IFBLK __S_IFBLK
#endif
#ifndef S_IXOTH
#define S_IXOTH (S_IXGRP >> 3)
#endif
#ifndef S_TYPEISSHM
#define S_TYPEISSHM(buf) __S_TYPEISSHM(buf)
#endif
#ifndef __S_ISVTX
#define __S_ISVTX 01000
#endif
#ifndef S_IRWXG
#define S_IRWXG (S_IRWXU >> 3)
#endif
#ifndef S_IRWXO
#define S_IRWXO (S_IRWXG >> 3)
#endif
#ifndef S_IRWXU
#define S_IRWXU (__S_IREAD|__S_IWRITE|__S_IEXEC)
#endif
#ifndef S_IFMT
#define S_IFMT __S_IFMT
#endif
#ifndef st_ctime
#define st_ctime st_ctim.tv_sec
#endif
#ifndef __S_IFCHR
#define __S_IFCHR 0020000
#endif
#ifndef S_ISVTX
#define S_ISVTX __S_ISVTX
#endif
#ifndef S_ISDIR
#define S_ISDIR(mode) __S_ISTYPE((mode), __S_IFDIR)
#endif
#ifndef S_IFCHR
#define S_IFCHR __S_IFCHR
#endif
#ifndef st_atime
#define st_atime st_atim.tv_sec
#endif
#ifndef S_IWUSR
#define S_IWUSR __S_IWRITE
#endif
#ifndef __S_IFREG
#define __S_IFREG 0100000
#endif
#ifndef S_IFREG
#define S_IFREG __S_IFREG
#endif
#ifndef S_ISLNK
#define S_ISLNK(mode) __S_ISTYPE((mode), __S_IFLNK)
#endif
#ifndef S_ISFIFO
#define S_ISFIFO(mode) __S_ISTYPE((mode), __S_IFIFO)
#endif
#ifndef st_mtime
#define st_mtime st_mtim.tv_sec
#endif
#ifndef __S_IFDIR
#define __S_IFDIR 0040000
#endif
#ifndef __S_IFMT
#define __S_IFMT 0170000
#endif
#ifndef S_IWGRP
#define S_IWGRP (S_IWUSR >> 3)
#endif
#ifndef __S_TYPEISSEM
#define __S_TYPEISSEM(buf) ((buf)->st_mode - (buf)->st_mode)
#endif
#ifndef __S_TYPEISSHM
#define __S_TYPEISSHM(buf) ((buf)->st_mode - (buf)->st_mode)
#endif
#ifndef S_IRUSR
#define S_IRUSR __S_IREAD
#endif
#ifndef S_IFDIR
#define S_IFDIR __S_IFDIR
#endif
#ifndef __S_IEXEC
#define __S_IEXEC 0100
#endif
#ifndef __S_IWRITE
#define __S_IWRITE 0200
#endif
#ifndef S_IFSOCK
#define S_IFSOCK __S_IFSOCK
#endif
#ifndef S_TYPEISMQ
#define S_TYPEISMQ(buf) __S_TYPEISMQ(buf)
#endif
#ifndef S_ISBLK
#define S_ISBLK(mode) __S_ISTYPE((mode), __S_IFBLK)
#endif
#ifndef __S_IFLNK
#define __S_IFLNK 0120000
#endif
#ifndef S_IXUSR
#define S_IXUSR __S_IEXEC
#endif
#ifndef S_IWOTH
#define S_IWOTH (S_IWGRP >> 3)
#endif
#ifndef S_IRGRP
#define S_IRGRP (S_IRUSR >> 3)
#endif
#ifndef S_ISREG
#define S_ISREG(mode) __S_ISTYPE((mode), __S_IFREG)
#endif
#ifndef __S_IFSOCK
#define __S_IFSOCK 0140000
#endif
#ifndef __S_ISUID
#define __S_ISUID 04000
#endif
#ifndef S_IFLNK
#define S_IFLNK __S_IFLNK
#endif
#ifndef __S_TYPEISMQ
#define __S_TYPEISMQ(buf) ((buf)->st_mode - (buf)->st_mode)
#endif
#ifndef __S_IFIFO
#define __S_IFIFO 0010000
#endif
#ifndef S_ISSOCK
#define S_ISSOCK(mode) __S_ISTYPE((mode), __S_IFSOCK)
#endif
#ifndef S_IXGRP
#define S_IXGRP (S_IXUSR >> 3)
#endif
#ifndef S_ISUID
#define S_ISUID __S_ISUID
#endif
#ifndef S_IROTH
#define S_IROTH (S_IRGRP >> 3)
#endif
#ifndef S_IFIFO
#define S_IFIFO __S_IFIFO
#endif
#ifndef __S_ISGID
#define __S_ISGID 02000
#endif

  // EXTREMELY irritating: under Linux this is not defined
  // in sys/stat.h, CONTRARY to the posix spec.  Instead __blksize_t
  // is defined.
  #if defined(___blksize_t_def_) && !defined(_blksize_t_def_)
  #define _blksize_t_def_
  typedef __blksize_t blksize_t;
  #endif
  /* JGM: no longer seems to be a problem for Cygwin so I'm getting
  *  rid of this. */
  /* Cygwin needs these definitions to smooth out the interface */
  //#if defined(__CYGWIN32__) || defined(__CYGWIN__)
  //#ifdef __CYGWIN_USE_BIG_TYPES__
  //#define st_uid  __st_uid32
  //#define st_gid  __st_gid32
  //#define st_size __st_size64
  //#else
  //#define st_uid  __st_uid16
  //#define st_gid  __st_gid16
  //#define st_size __st_size32
  //#endif /* __CYGWIN_USE_BIG_TYPES__ */
  //#endif /* defined(__CYGWIN32__) || defined(__CYGWIN__) */

  extern "C" int chmod(const char @, mode_t);

  extern "C" int fchmod(int, mode_t);

  extern "C" int fstat(int fd, struct stat @buf);

  extern "C" int lstat(const char @ filename, struct stat @buf);

  extern "C" int mkdir(const char @ pathname, mode_t mode);

  extern "C" int mkfifo(const char @ pathname, mode_t mode);

  extern "C" int mknod(const char @ pathname, mode_t mode, dev_t);

  extern "C" int stat(const char @, struct stat @);

  extern "C" mode_t umask(mode_t mask);
#endif

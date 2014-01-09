#ifndef _SYS_TYPES_H_
#define _SYS_TYPES_H_
#ifndef _size_t_def_
#define _size_t_def_
typedef unsigned int size_t;
#endif
#ifndef ___u_char_def_
#define ___u_char_def_
typedef unsigned char __u_char;
#endif
#ifndef ___u_short_def_
#define ___u_short_def_
typedef unsigned short __u_short;
#endif
#ifndef ___u_int_def_
#define ___u_int_def_
typedef unsigned int __u_int;
#endif
#ifndef ___u_long_def_
#define ___u_long_def_
typedef unsigned long __u_long;
#endif
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
#ifndef ___key_t_def_
#define ___key_t_def_
typedef int __key_t;
#endif
#ifndef ___clockid_t_def_
#define ___clockid_t_def_
typedef int __clockid_t;
#endif
#ifndef ___timer_t_def_
#define ___timer_t_def_
typedef void * __timer_t;
#endif
#ifndef ___blkcnt_t_def_
#define ___blkcnt_t_def_
typedef long __blkcnt_t;
#endif
#ifndef ___fsblkcnt_t_def_
#define ___fsblkcnt_t_def_
typedef unsigned long __fsblkcnt_t;
#endif
#ifndef ___fsfilcnt_t_def_
#define ___fsfilcnt_t_def_
typedef unsigned long __fsfilcnt_t;
#endif
#ifndef ___ssize_t_def_
#define ___ssize_t_def_
typedef int __ssize_t;
#endif
#ifndef _u_char_def_
#define _u_char_def_
typedef __u_char u_char;
#endif
#ifndef _u_short_def_
#define _u_short_def_
typedef __u_short u_short;
#endif
#ifndef _u_int_def_
#define _u_int_def_
typedef __u_int u_int;
#endif
#ifndef _u_long_def_
#define _u_long_def_
typedef __u_long u_long;
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
#ifndef _pid_t_def_
#define _pid_t_def_
typedef __pid_t pid_t;
#endif
#ifndef _id_t_def_
#define _id_t_def_
typedef __id_t id_t;
#endif
#ifndef _ssize_t_def_
#define _ssize_t_def_
typedef __ssize_t ssize_t;
#endif
#ifndef _key_t_def_
#define _key_t_def_
typedef __key_t key_t;
#endif
#ifndef _time_t_def_
#define _time_t_def_
typedef __time_t time_t;
#endif
#ifndef _clockid_t_def_
#define _clockid_t_def_
typedef __clockid_t clockid_t;
#endif
#ifndef _timer_t_def_
#define _timer_t_def_
typedef __timer_t timer_t;
#endif
#ifndef _suseconds_t_def_
#define _suseconds_t_def_
typedef __suseconds_t suseconds_t;
#endif
#ifndef _blkcnt_t_def_
#define _blkcnt_t_def_
typedef __blkcnt_t blkcnt_t;
#endif
#ifndef _fsblkcnt_t_def_
#define _fsblkcnt_t_def_
typedef __fsblkcnt_t fsblkcnt_t;
#endif
#ifndef _fsfilcnt_t_def_
#define _fsfilcnt_t_def_
typedef __fsfilcnt_t fsfilcnt_t;
#endif

/* The pthread types are omitted for now since we don't support
   threads */
/* The trace types are also omitted */
#endif

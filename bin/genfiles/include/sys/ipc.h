#ifndef _SYS_IPC_H_
#define _SYS_IPC_H_
#ifndef ___uid_t_def_
#define ___uid_t_def_
typedef unsigned int __uid_t;
#endif
#ifndef ___gid_t_def_
#define ___gid_t_def_
typedef unsigned int __gid_t;
#endif
#ifndef ___mode_t_def_
#define ___mode_t_def_
typedef unsigned int __mode_t;
#endif
#ifndef ___key_t_def_
#define ___key_t_def_
typedef int __key_t;
#endif
#ifndef _ipc_perm_def_
#define _ipc_perm_def_
struct ipc_perm{
  __key_t __key;
  __uid_t uid;
  __gid_t gid;
  __uid_t cuid;
  __gid_t cgid;
  unsigned short mode;
  unsigned short __pad1;
  unsigned short __seq;
  unsigned short __pad2;
  unsigned long __unused1;
  unsigned long __unused2;
};
#endif
#ifndef _uid_t_def_
#define _uid_t_def_
typedef __uid_t uid_t;
#endif
#ifndef _gid_t_def_
#define _gid_t_def_
typedef __gid_t gid_t;
#endif
#ifndef _mode_t_def_
#define _mode_t_def_
typedef __mode_t mode_t;
#endif
#ifndef _key_t_def_
#define _key_t_def_
typedef __key_t key_t;
#endif
#ifndef IPC_CREAT
#define IPC_CREAT 01000
#endif
#ifndef IPC_STAT
#define IPC_STAT 2
#endif
#ifndef IPC_EXCL
#define IPC_EXCL 02000
#endif
#ifndef IPC_PRIVATE
#define IPC_PRIVATE ((__key_t) 0)
#endif
#ifndef IPC_NOWAIT
#define IPC_NOWAIT 04000
#endif
#ifndef IPC_SET
#define IPC_SET 1
#endif
#ifndef IPC_RMID
#define IPC_RMID 0
#endif

  extern "C" key_t ftok(const char @, int);
#endif

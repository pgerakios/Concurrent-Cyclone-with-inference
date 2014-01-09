#ifndef _PWD_H_
#define _PWD_H_
#ifndef ___uid_t_def_
#define ___uid_t_def_
typedef unsigned int __uid_t;
#endif
#ifndef ___gid_t_def_
#define ___gid_t_def_
typedef unsigned int __gid_t;
#endif
#ifndef _gid_t_def_
#define _gid_t_def_
typedef __gid_t gid_t;
#endif
#ifndef _uid_t_def_
#define _uid_t_def_
typedef __uid_t uid_t;
#endif
#ifndef _passwd_def_
#define _passwd_def_
struct passwd{
  char * pw_name;
  char * pw_passwd;
  __uid_t pw_uid;
  __gid_t pw_gid;
  char * pw_gecos;
  char * pw_dir;
  char * pw_shell;
};
#endif

  extern "C" struct passwd *getpwnam(const char @name);

  extern "C" struct passwd *getpwuid(uid_t uid);
#endif

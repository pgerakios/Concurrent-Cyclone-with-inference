#ifndef _DIRENT_H_
#define _DIRENT_H_
#ifndef ___ino_t_def_
#define ___ino_t_def_
typedef unsigned long __ino_t;
#endif
#ifndef ___off_t_def_
#define ___off_t_def_
typedef long __off_t;
#endif
#ifndef _ino_t_def_
#define _ino_t_def_
typedef __ino_t ino_t;
#endif
#ifndef _dirent_def_
#define _dirent_def_
struct dirent{
  __ino_t d_ino;
  __off_t d_off;
  unsigned short d_reclen;
  unsigned char d_type;
  char d_name[256U];
};
#endif
#ifndef NAME_MAX
#define NAME_MAX 255
#endif

  // There is an odd case for arm-elf + newlib where the file dirent.h
  // can exist but not define anything.  Because of the way we handle
  // the DIR type, this would lead to parse errors in the buildlib
  // output.  So, we make sure opendir is defined, here and below,
  // before generating output.
  //
  // We define DIR as an abstract struct because it can contain
  // pointers, e.g., in cygwin
  extern struct __cycDIR;  
  typedef struct __cycDIR DIR;

  // struct dirent on Mac OS X does not have a d_ino member as
  // required by POSIX, instead it is named d_fileno.
  // This #define makes cross-platform porting easier.
  // Probably this applies to other BSD variants...
#ifdef __APPLE__
#define d_ino d_fileno
#endif

  extern int closedir(DIR @);

  extern DIR *opendir(const char @);

  extern struct dirent @readdir(DIR @);

  // FIX: We don't handle this yet. The third arg is an array,
  // needs a wrapper
  //int readdir_r(DIR *, struct dirent *, struct dirent **);

  extern void rewinddir(DIR @);

  extern void seekdir(DIR @, long);

  extern long telldir(DIR @);
#endif

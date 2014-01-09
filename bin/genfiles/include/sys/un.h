#ifndef _SYS_UN_H_
#define _SYS_UN_H_
#ifndef _sa_family_t_def_
#define _sa_family_t_def_
typedef unsigned short sa_family_t;
#endif
#ifndef _sockaddr_un_def_
#define _sockaddr_un_def_
struct sockaddr_un{
  sa_family_t sun_family;
  char sun_path[108U];
};
#endif

  /* Not defined in OS X */
  #ifndef _sa_family_t_def_
  #define _sa_family_t_def_
  typedef unsigned char sa_family_t;
  #endif
#endif

#ifndef _SYS_UTSNAME_H_
#define _SYS_UTSNAME_H_
#ifndef _utsname_def_
#define _utsname_def_
struct utsname{
  char sysname[65U];
  char nodename[65U];
  char release[65U];
  char version[65U];
  char machine[65U];
  char __domainname[65U];
};
#endif

  extern "C" int uname(struct utsname @);
#endif

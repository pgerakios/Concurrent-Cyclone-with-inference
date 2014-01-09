#ifndef _GRP_H_
#define _GRP_H_
#ifndef ___gid_t_def_
#define ___gid_t_def_
typedef unsigned int __gid_t;
#endif
#ifndef _gid_t_def_
#define _gid_t_def_
typedef __gid_t gid_t;
#endif
#ifndef _group_def_
#define _group_def_
struct group{
  char @ gr_name;
  char @ gr_passwd;
  gid_t gr_gid;
  char *@zeroterm *@zeroterm gr_mem;
};
#endif

  extern "C" int initgroups(const char @ @zeroterm user, gid_t group);

  extern "C" int setgroups(tag_t<`i> size, const gid_t @{valueof(`i)} list);

  extern "C" struct group *getgrnam(const char @ name);

  extern "C" struct group *getgrgid(gid_t gid);
#endif

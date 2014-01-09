#ifndef _SYS_MMAN_H_
#define _SYS_MMAN_H_
#ifndef _size_t_def_
#define _size_t_def_
typedef unsigned int size_t;
#endif
#ifndef ___off_t_def_
#define ___off_t_def_
typedef long __off_t;
#endif
#ifndef _off_t_def_
#define _off_t_def_
typedef __off_t off_t;
#endif
#ifndef PROT_NONE
#define PROT_NONE 0x0
#endif
#ifndef PROT_EXEC
#define PROT_EXEC 0x4
#endif
#ifndef MAP_PRIVATE
#define MAP_PRIVATE 0x02
#endif
#ifndef MAP_ANON
#define MAP_ANON MAP_ANONYMOUS
#endif
#ifndef PROT_WRITE
#define PROT_WRITE 0x2
#endif
#ifndef PROT_READ
#define PROT_READ 0x1
#endif
#ifndef MAP_SHARED
#define MAP_SHARED 0x01
#endif
#ifndef MAP_GROWSDOWN
#define MAP_GROWSDOWN 0x00100
#endif
#ifndef MAP_FIXED
#define MAP_FIXED 0x10
#endif
#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS 0x20
#endif

  // We leave out some of the functions/flags, including mapping
  // in executable content, or fixing the address of the mapped memory

  // So that we can properly return a char ? pointer, we define
  // MAP_FAILED as NULL
  #define MAP_FAILED NULL

  // The traditional first arg to mmap should be 0 anyway, so don't
  // even bother to pass it to us.
  extern char ? @nozeroterm mmap(char ? @nozeroterm, size_t length, 
	                         int prot, int flags, 
		                 int fd, off_t offset);

  extern int munmap(const char ? @nozeroterm start, size_t length);
#endif

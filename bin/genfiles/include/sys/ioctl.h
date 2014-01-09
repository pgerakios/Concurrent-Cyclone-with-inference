#ifndef _SYS_IOCTL_H_
#define _SYS_IOCTL_H_
#ifndef FIONBIO
#define FIONBIO 0x5421
#endif
#ifndef FIONREAD
#define FIONREAD 0x541B
#endif

  // ioctl normally takes a void * as its arg, but
  //   for the moment we forbid it to taking only ints.
  //   Ultimately should change to use datatypes as
  //   socket commands do
  extern "C" int ioctl(int fd, int cmd, int @arg);
#endif

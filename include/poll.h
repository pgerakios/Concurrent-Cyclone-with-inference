#ifndef _POLL_H_
#define _POLL_H_
#ifndef _nfds_t_def_
#define _nfds_t_def_
typedef unsigned long nfds_t;
#endif
#ifndef _pollfd_def_
#define _pollfd_def_
struct pollfd{
  int fd;
  short events;
  short revents;
};
#endif
#ifndef POLLNVAL
#define POLLNVAL 0x020
#endif
#ifndef POLLOUT
#define POLLOUT 0x004
#endif
#ifndef POLLHUP
#define POLLHUP 0x010
#endif
#ifndef POLLIN
#define POLLIN 0x001
#endif
#ifndef POLLERR
#define POLLERR 0x008
#endif
#ifndef POLLPRI
#define POLLPRI 0x002
#endif

  /* Cygwin does not define this type so we take a guess :-( */
  /* FIX: make this specific to cygwin ?? */
  #ifndef _nfds_t_def_
  #define _nfds_t_def_
  typedef unsigned int nfds_t;
  #endif

  extern int poll (struct pollfd?, nfds_t, int);
#endif

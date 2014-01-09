#ifndef _SYS_SOCKET_H_
#define _SYS_SOCKET_H_
#ifndef _size_t_def_
#define _size_t_def_
typedef unsigned int size_t;
#endif
#ifndef ___ssize_t_def_
#define ___ssize_t_def_
typedef int __ssize_t;
#endif
#ifndef ___socklen_t_def_
#define ___socklen_t_def_
typedef unsigned int __socklen_t;
#endif
#ifndef _ssize_t_def_
#define _ssize_t_def_
typedef __ssize_t ssize_t;
#endif
#ifndef _socklen_t_def_
#define _socklen_t_def_
typedef __socklen_t socklen_t;
#endif
#ifndef ___socket_type_def_
#define ___socket_type_def_
enum __socket_type{
  SOCK_STREAM = 1,
  SOCK_DGRAM = 2,
  SOCK_RAW = 3,
  SOCK_RDM = 4,
  SOCK_SEQPACKET = 5,
  SOCK_PACKET = 10
};
#endif
#ifndef _sa_family_t_def_
#define _sa_family_t_def_
typedef unsigned short sa_family_t;
#endif
#ifndef _sockaddr_def_
#define _sockaddr_def_
struct sockaddr{
  sa_family_t sa_family;
  char sa_data[14U];
};
#endif
#ifndef ___anonymous_enum_79___def_
#define ___anonymous_enum_79___def_
enum __anonymous_enum_79__{
  MSG_OOB = 1,
  MSG_PEEK = 2,
  MSG_DONTROUTE = 4,
  MSG_CTRUNC = 8,
  MSG_PROXY = 16,
  MSG_TRUNC = 32,
  MSG_DONTWAIT = 64,
  MSG_EOR = 128,
  MSG_WAITALL = 256,
  MSG_FIN = 512,
  MSG_SYN = 1024,
  MSG_CONFIRM = 2048,
  MSG_RST = 4096,
  MSG_ERRQUEUE = 8192,
  MSG_NOSIGNAL = 16384,
  MSG_MORE = 32768
};
#endif
#ifndef _cmsghdr_def_
#define _cmsghdr_def_
struct cmsghdr{
  size_t cmsg_len;
  int cmsg_level;
  int cmsg_type;
  unsigned char __cmsg_data[1U];
};
#endif
#ifndef ___anonymous_enum_80___def_
#define ___anonymous_enum_80___def_
enum __anonymous_enum_80__{
  SCM_RIGHTS = 1,
  SCM_CREDENTIALS = 2
};
#endif
#ifndef _linger_def_
#define _linger_def_
struct linger{
  int l_onoff;
  int l_linger;
};
#endif
#ifndef ___anonymous_enum_81___def_
#define ___anonymous_enum_81___def_
enum __anonymous_enum_81__{
  SHUT_RD = 0,
  SHUT_WR = 1U,
  SHUT_RDWR = 2U
};
#endif
#ifndef SO_KEEPALIVE
#define SO_KEEPALIVE 9
#endif
#ifndef SOCK_RAW
#define SOCK_RAW SOCK_RAW
#endif
#ifndef AF_INET6
#define AF_INET6 PF_INET6
#endif
#ifndef MSG_WAITALL
#define MSG_WAITALL MSG_WAITALL
#endif
#ifndef MSG_PEEK
#define MSG_PEEK MSG_PEEK
#endif
#ifndef SO_REUSEADDR
#define SO_REUSEADDR 2
#endif
#ifndef SO_LINGER
#define SO_LINGER 13
#endif
#ifndef SOCK_SEQPACKET
#define SOCK_SEQPACKET SOCK_SEQPACKET
#endif
#ifndef SO_BROADCAST
#define SO_BROADCAST 6
#endif
#ifndef PF_UNIX
#define PF_UNIX PF_LOCAL
#endif
#ifndef AF_UNSPEC
#define AF_UNSPEC PF_UNSPEC
#endif
#ifndef SO_ACCEPTCONN
#define SO_ACCEPTCONN 30
#endif
#ifndef SO_TYPE
#define SO_TYPE 3
#endif
#ifndef SCM_RIGHTS
#define SCM_RIGHTS SCM_RIGHTS
#endif
#ifndef SHUT_RDWR
#define SHUT_RDWR SHUT_RDWR
#endif
#ifndef MSG_CTRUNC
#define MSG_CTRUNC MSG_CTRUNC
#endif
#ifndef MSG_DONTROUTE
#define MSG_DONTROUTE MSG_DONTROUTE
#endif
#ifndef SO_DONTROUTE
#define SO_DONTROUTE 5
#endif
#ifndef SOL_SOCKET
#define SOL_SOCKET 1
#endif
#ifndef PF_UNSPEC
#define PF_UNSPEC 0
#endif
#ifndef SO_RCVLOWAT
#define SO_RCVLOWAT 18
#endif
#ifndef SO_SNDTIMEO
#define SO_SNDTIMEO 21
#endif
#ifndef PF_INET6
#define PF_INET6 10
#endif
#ifndef SOCK_DGRAM
#define SOCK_DGRAM SOCK_DGRAM
#endif
#ifndef SO_OOBINLINE
#define SO_OOBINLINE 10
#endif
#ifndef CMSG_DATA
#define CMSG_DATA(cmsg) ((cmsg)->__cmsg_data)
#endif
#ifndef AF_INET
#define AF_INET PF_INET
#endif
#ifndef MSG_TRUNC
#define MSG_TRUNC MSG_TRUNC
#endif
#ifndef MSG_OOB
#define MSG_OOB MSG_OOB
#endif
#ifndef SO_SNDBUF
#define SO_SNDBUF 7
#endif
#ifndef SO_DEBUG
#define SO_DEBUG 1
#endif
#ifndef SO_RCVBUF
#define SO_RCVBUF 8
#endif
#ifndef SOCK_STREAM
#define SOCK_STREAM SOCK_STREAM
#endif
#ifndef AF_UNIX
#define AF_UNIX PF_UNIX
#endif
#ifndef SO_SNDLOWAT
#define SO_SNDLOWAT 19
#endif
#ifndef SO_ERROR
#define SO_ERROR 4
#endif
#ifndef MSG_EOR
#define MSG_EOR MSG_EOR
#endif
#ifndef PF_INET
#define PF_INET 2
#endif
#ifndef SOMAXCONN
#define SOMAXCONN 128
#endif
#ifndef SHUT_RD
#define SHUT_RD SHUT_RD
#endif
#ifndef SHUT_WR
#define SHUT_WR SHUT_WR
#endif
#ifndef SO_RCVTIMEO
#define SO_RCVTIMEO 20
#endif
#ifndef PF_LOCAL
#define PF_LOCAL 1
#endif

  /* JGM: mini_httpd needs sockaddr, so I've added it back in:
       We omit struct sockaddr because we don't use it in Cyclone */
  /* We omit struct sockaddr_storage because we don't use it in Cyclone */
  /* We omit struct msghdr because it contains a void * field */
  /* We omit CMSG_NXTHDR and CMSG_FIRSTHDR because they pull in msghdr */
  /* We omit struct iovec because it contains a void * field */

  /* We omit these because they rely on msghdr */
  /*
    extern ssize_t sendmsg(int fd, const struct msghdr *message, int flags);
    extern ssize_t recvmsg(int fd, struct msghdr *message, int flags);
  */

  /* Many architectures don't define this :-( */
  #ifndef _socklen_t_def_
  #define _socklen_t_def_
  #ifndef socklen_t
  /* Cygwin #defines socklen_t */
  typedef int socklen_t;
  #endif
  #endif
  /* Not defined in OS X */
  #ifndef _sa_family_t_def_
  #define _sa_family_t_def_
  typedef unsigned char sa_family_t;
  #endif

  extern datatype SockAddr<`r::R> {
    SA_sockaddr_in(struct sockaddr_in @`r);
    /* We will add other cases as necessary, e.g., SockAddr_In6 */

    /* The remaining cases are for the argument following the type-varying
       argument; they could be eliminated if we had per-arg injection. */
    /* NOTE: ORDER MATTERS! on these inject things */
    SA_socklenptr(socklen_t @`r); // accept, getpeername, getsockname, recvfrom
    SA_socklen(socklen_t); // bind, connect, sendto
  };
  typedef datatype SockAddr<`r> @`r SA<`r>;

  extern datatype SockOpt<`r::R> {
    SO_int(int @`r);
    SO_timeval(struct timeval @`r);
    SO_linger(struct linger@`r);
    /* The remaining cases are for the argument following the type-varying
       argument; they could be eliminated if we had per-arg injection. */
    SO_socklenptr(socklen_t @`r); // getsockopt
    SO_socklen(socklen_t);        // setsockopt
  };
  typedef datatype SockOpt<`r> @`r SO<`r>;

  extern int accept(int fd, ... inject SA);

  datatype exn { extern SocketError };

  extern int bind(int fd, ... inject SA);

  extern int connect(int fd, ... inject SA);

  extern int getpeername(int fd, ... inject SA);

  extern int getsockname(int fd, ... inject SA);

  extern int getsockopt(int fd, int level, int optname, ... inject SO);

  extern "C" int listen(int fd, int n);

  extern ssize_t recv(int fd, char ? @nozeroterm buf, size_t n, int flags);

  extern ssize_t recvfrom(int fd, char ? @nozeroterm buf, size_t n, int flags,
                          ... inject SA);

  extern ssize_t send(int fd, const char ? @nozeroterm buf, size_t n, int flags);

  extern ssize_t sendto(int fd, const char ? @nozeroterm buf, size_t n, int flags,
                        ... inject SA);

  extern int setsockopt(int fd, int level, int optname, ... inject SO);

  extern "C" int shutdown(int fd, int how);

  extern "C" int sockatmark(int);

  extern "C" int socket(int domain, int type, int protocol);

  extern "C" int socketpair(int domain, int type, int protocol, int @{2} fds);
#endif

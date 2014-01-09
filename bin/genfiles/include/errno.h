#ifndef _ERRNO_H_
#define _ERRNO_H_
#ifndef EMULTIHOP
#define EMULTIHOP 72
#endif
#ifndef EAFNOSUPPORT
#define EAFNOSUPPORT 97
#endif
#ifndef EACCES
#define EACCES 13
#endif
#ifndef EDESTADDRREQ
#define EDESTADDRREQ 89
#endif
#ifndef EILSEQ
#define EILSEQ 84
#endif
#ifndef ESPIPE
#define ESPIPE 29
#endif
#ifndef ENOTTY
#define ENOTTY 25
#endif
#ifndef EBADF
#define EBADF 9
#endif
#ifndef ERANGE
#define ERANGE 34
#endif
#ifndef ECANCELED
#define ECANCELED 125
#endif
#ifndef ETXTBSY
#define ETXTBSY 26
#endif
#ifndef ENOMEM
#define ENOMEM 12
#endif
#ifndef EINPROGRESS
#define EINPROGRESS 115
#endif
#ifndef ENOTEMPTY
#define ENOTEMPTY 39
#endif
#ifndef EPROTOTYPE
#define EPROTOTYPE 91
#endif
#ifndef ENOMSG
#define ENOMSG 42
#endif
#ifndef EALREADY
#define EALREADY 114
#endif
#ifndef ETIMEDOUT
#define ETIMEDOUT 110
#endif
#ifndef ENODATA
#define ENODATA 61
#endif
#ifndef EINTR
#define EINTR 4
#endif
#ifndef ENOLINK
#define ENOLINK 67
#endif
#ifndef EPERM
#define EPERM 1
#endif
#ifndef ELOOP
#define ELOOP 40
#endif
#ifndef ENETDOWN
#define ENETDOWN 100
#endif
#ifndef ESTALE
#define ESTALE 116
#endif
#ifndef ENOTSOCK
#define ENOTSOCK 88
#endif
#ifndef ENOSR
#define ENOSR 63
#endif
#ifndef ECHILD
#define ECHILD 10
#endif
#ifndef EPIPE
#define EPIPE 32
#endif
#ifndef EBADMSG
#define EBADMSG 74
#endif
#ifndef EADDRINUSE
#define EADDRINUSE 98
#endif
#ifndef EISDIR
#define EISDIR 21
#endif
#ifndef EIDRM
#define EIDRM 43
#endif
#ifndef EINVAL
#define EINVAL 22
#endif
#ifndef EOVERFLOW
#define EOVERFLOW 75
#endif
#ifndef EBUSY
#define EBUSY 16
#endif
#ifndef EPROTO
#define EPROTO 71
#endif
#ifndef ENODEV
#define ENODEV 19
#endif
#ifndef EROFS
#define EROFS 30
#endif
#ifndef E2BIG
#define E2BIG 7
#endif
#ifndef EDEADLK
#define EDEADLK 35
#endif
#ifndef ENOTDIR
#define ENOTDIR 20
#endif
#ifndef ECONNRESET
#define ECONNRESET 104
#endif
#ifndef ENXIO
#define ENXIO 6
#endif
#ifndef ENAMETOOLONG
#define ENAMETOOLONG 36
#endif
#ifndef EADDRNOTAVAIL
#define EADDRNOTAVAIL 99
#endif
#ifndef ETIME
#define ETIME 62
#endif
#ifndef EPROTONOSUPPORT
#define EPROTONOSUPPORT 93
#endif
#ifndef EIO
#define EIO 5
#endif
#ifndef ENETUNREACH
#define ENETUNREACH 101
#endif
#ifndef EXDEV
#define EXDEV 18
#endif
#ifndef EDQUOT
#define EDQUOT 122
#endif
#ifndef ENOSPC
#define ENOSPC 28
#endif
#ifndef ENOEXEC
#define ENOEXEC 8
#endif
#ifndef EMSGSIZE
#define EMSGSIZE 90
#endif
#ifndef EDOM
#define EDOM 33
#endif
#ifndef ENOSTR
#define ENOSTR 60
#endif
#ifndef EFBIG
#define EFBIG 27
#endif
#ifndef ESRCH
#define ESRCH 3
#endif
#ifndef ENOLCK
#define ENOLCK 37
#endif
#ifndef ENFILE
#define ENFILE 23
#endif
#ifndef ENOSYS
#define ENOSYS 38
#endif
#ifndef ENOTCONN
#define ENOTCONN 107
#endif
#ifndef ENOTSUP
#define ENOTSUP EOPNOTSUPP
#endif
#ifndef ECONNABORTED
#define ECONNABORTED 103
#endif
#ifndef EISCONN
#define EISCONN 106
#endif
#ifndef ENOPROTOOPT
#define ENOPROTOOPT 92
#endif
#ifndef EMFILE
#define EMFILE 24
#endif
#ifndef ENOBUFS
#define ENOBUFS 105
#endif
#ifndef EFAULT
#define EFAULT 14
#endif
#ifndef EWOULDBLOCK
#define EWOULDBLOCK EAGAIN
#endif
#ifndef ECONNREFUSED
#define ECONNREFUSED 111
#endif
#ifndef EAGAIN
#define EAGAIN 11
#endif
#ifndef EEXIST
#define EEXIST 17
#endif
#ifndef ENOENT
#define ENOENT 2
#endif
#ifndef EHOSTUNREACH
#define EHOSTUNREACH 113
#endif
#ifndef EOPNOTSUPP
#define EOPNOTSUPP 95
#endif

extern "C" int *__CYCLONE_ERRNO();
#define errno (*__CYCLONE_ERRNO())
#endif

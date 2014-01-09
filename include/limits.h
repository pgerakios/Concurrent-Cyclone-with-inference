#ifndef _LIMITS_H_
#define _LIMITS_H_
#ifndef _POSIX2_BC_SCALE_MAX
#define _POSIX2_BC_SCALE_MAX 99
#endif
#ifndef RE_DUP_MAX
#define RE_DUP_MAX (0x7fff)
#endif
#ifndef __CHAR_BIT__
#define __CHAR_BIT__ 8
#endif
#ifndef SHRT_MAX
#define SHRT_MAX __SHRT_MAX__
#endif
#ifndef PIPE_BUF
#define PIPE_BUF 4096
#endif
#ifndef MQ_PRIO_MAX
#define MQ_PRIO_MAX 32768
#endif
#ifndef _POSIX2_BC_DIM_MAX
#define _POSIX2_BC_DIM_MAX 2048
#endif
#ifndef _POSIX_TTY_NAME_MAX
#define _POSIX_TTY_NAME_MAX 9
#endif
#ifndef _POSIX2_BC_STRING_MAX
#define _POSIX2_BC_STRING_MAX 1000
#endif
#ifndef SHRT_MIN
#define SHRT_MIN (-SHRT_MAX - 1)
#endif
#ifndef ARG_MAX
#define ARG_MAX 131072
#endif
#ifndef SSIZE_MAX
#define SSIZE_MAX LONG_MAX
#endif
#ifndef _POSIX_DELAYTIMER_MAX
#define _POSIX_DELAYTIMER_MAX 32
#endif
#ifndef _POSIX_SYMLINK_MAX
#define _POSIX_SYMLINK_MAX 255
#endif
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
#ifndef __SHRT_MAX__
#define __SHRT_MAX__ 32767
#endif
#ifndef NAME_MAX
#define NAME_MAX 255
#endif
#ifndef SCHAR_MAX
#define SCHAR_MAX __SCHAR_MAX__
#endif
#ifndef _POSIX_AIO_MAX
#define _POSIX_AIO_MAX 1
#endif
#ifndef _POSIX_PIPE_BUF
#define _POSIX_PIPE_BUF 512
#endif
#ifndef _POSIX2_EXPR_NEST_MAX
#define _POSIX2_EXPR_NEST_MAX 32
#endif
#ifndef SCHAR_MIN
#define SCHAR_MIN (-SCHAR_MAX - 1)
#endif
#ifndef _POSIX_AIO_LISTIO_MAX
#define _POSIX_AIO_LISTIO_MAX 2
#endif
#ifndef __SCHAR_MAX__
#define __SCHAR_MAX__ 127
#endif
#ifndef _POSIX_THREAD_THREADS_MAX
#define _POSIX_THREAD_THREADS_MAX 64
#endif
#ifndef _POSIX_MQ_OPEN_MAX
#define _POSIX_MQ_OPEN_MAX 8
#endif
#ifndef _POSIX_LINK_MAX
#define _POSIX_LINK_MAX 8
#endif
#ifndef COLL_WEIGHTS_MAX
#define COLL_WEIGHTS_MAX 255
#endif
#ifndef USHRT_MAX
#define USHRT_MAX (SHRT_MAX * 2 + 1)
#endif
#ifndef _POSIX_CHILD_MAX
#define _POSIX_CHILD_MAX 25
#endif
#ifndef _POSIX_NAME_MAX
#define _POSIX_NAME_MAX 14
#endif
#ifndef RTSIG_MAX
#define RTSIG_MAX 32
#endif
#ifndef _POSIX2_RE_DUP_MAX
#define _POSIX2_RE_DUP_MAX 255
#endif
#ifndef MAX_INPUT
#define MAX_INPUT 255
#endif
#ifndef _POSIX_LOGIN_NAME_MAX
#define _POSIX_LOGIN_NAME_MAX 9
#endif
#ifndef DELAYTIMER_MAX
#define DELAYTIMER_MAX 2147483647
#endif
#ifndef LONG_MAX
#define LONG_MAX __LONG_MAX__
#endif
#ifndef _POSIX_THREAD_DESTRUCTOR_ITERATIONS
#define _POSIX_THREAD_DESTRUCTOR_ITERATIONS 4
#endif
#ifndef _POSIX_HOST_NAME_MAX
#define _POSIX_HOST_NAME_MAX 255
#endif
#ifndef NGROUPS_MAX
#define NGROUPS_MAX 65536
#endif
#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 64
#endif
#ifndef _POSIX_SEM_NSEMS_MAX
#define _POSIX_SEM_NSEMS_MAX 256
#endif
#ifndef _POSIX_SIGQUEUE_MAX
#define _POSIX_SIGQUEUE_MAX 32
#endif
#ifndef _POSIX_NGROUPS_MAX
#define _POSIX_NGROUPS_MAX 8
#endif
#ifndef _POSIX_PATH_MAX
#define _POSIX_PATH_MAX 256
#endif
#ifndef LONG_MIN
#define LONG_MIN (-LONG_MAX - 1L)
#endif
#ifndef _POSIX_RE_DUP_MAX
#define _POSIX_RE_DUP_MAX 255
#endif
#ifndef _POSIX_TZNAME_MAX
#define _POSIX_TZNAME_MAX 6
#endif
#ifndef _POSIX_SSIZE_MAX
#define _POSIX_SSIZE_MAX 32767
#endif
#ifndef _POSIX_CLOCKRES_MIN
#define _POSIX_CLOCKRES_MIN 20000000
#endif
#ifndef _POSIX_SYMLOOP_MAX
#define _POSIX_SYMLOOP_MAX 8
#endif
#ifndef CHAR_MAX
#define CHAR_MAX SCHAR_MAX
#endif
#ifndef ULONG_MAX
#define ULONG_MAX (LONG_MAX * 2UL + 1UL)
#endif
#ifndef LOGIN_NAME_MAX
#define LOGIN_NAME_MAX 256
#endif
#ifndef CHAR_BIT
#define CHAR_BIT __CHAR_BIT__
#endif
#ifndef CHAR_MIN
#define CHAR_MIN SCHAR_MIN
#endif
#ifndef MAX_CANON
#define MAX_CANON 255
#endif
#ifndef AIO_PRIO_DELTA_MAX
#define AIO_PRIO_DELTA_MAX 20
#endif
#ifndef LINE_MAX
#define LINE_MAX _POSIX2_LINE_MAX
#endif
#ifndef _POSIX_STREAM_MAX
#define _POSIX_STREAM_MAX 8
#endif
#ifndef _POSIX_RTSIG_MAX
#define _POSIX_RTSIG_MAX 8
#endif
#ifndef _POSIX_MAX_INPUT
#define _POSIX_MAX_INPUT 255
#endif
#ifndef PTHREAD_STACK_MIN
#define PTHREAD_STACK_MIN 16384
#endif
#ifndef _POSIX_ARG_MAX
#define _POSIX_ARG_MAX 4096
#endif
#ifndef UCHAR_MAX
#define UCHAR_MAX (SCHAR_MAX * 2 + 1)
#endif
#ifndef _POSIX2_COLL_WEIGHTS_MAX
#define _POSIX2_COLL_WEIGHTS_MAX 2
#endif
#ifndef INT_MAX
#define INT_MAX __INT_MAX__
#endif
#ifndef PTHREAD_DESTRUCTOR_ITERATIONS
#define PTHREAD_DESTRUCTOR_ITERATIONS _POSIX_THREAD_DESTRUCTOR_ITERATIONS
#endif
#ifndef _POSIX2_LINE_MAX
#define _POSIX2_LINE_MAX 2048
#endif
#ifndef __LONG_MAX__
#define __LONG_MAX__ 2147483647L
#endif
#ifndef _POSIX2_CHARCLASS_NAME_MAX
#define _POSIX2_CHARCLASS_NAME_MAX 14
#endif
#ifndef EXPR_NEST_MAX
#define EXPR_NEST_MAX _POSIX2_EXPR_NEST_MAX
#endif
#ifndef INT_MIN
#define INT_MIN (-INT_MAX - 1)
#endif
#ifndef TTY_NAME_MAX
#define TTY_NAME_MAX 32
#endif
#ifndef __INT_MAX__
#define __INT_MAX__ 2147483647
#endif
#ifndef UINT_MAX
#define UINT_MAX (INT_MAX * 2U + 1U)
#endif
#ifndef _POSIX_SEM_VALUE_MAX
#define _POSIX_SEM_VALUE_MAX 32767
#endif
#ifndef BC_DIM_MAX
#define BC_DIM_MAX _POSIX2_BC_DIM_MAX
#endif
#ifndef _POSIX_MAX_CANON
#define _POSIX_MAX_CANON 255
#endif
#ifndef BC_SCALE_MAX
#define BC_SCALE_MAX _POSIX2_BC_SCALE_MAX
#endif
#ifndef CHARCLASS_NAME_MAX
#define CHARCLASS_NAME_MAX 2048
#endif
#ifndef _POSIX2_BC_BASE_MAX
#define _POSIX2_BC_BASE_MAX 99
#endif
#ifndef _POSIX_MQ_PRIO_MAX
#define _POSIX_MQ_PRIO_MAX 32
#endif
#ifndef CHILD_MAX
#define CHILD_MAX 999
#endif
#ifndef _POSIX_TIMER_MAX
#define _POSIX_TIMER_MAX 32
#endif
#ifndef BC_STRING_MAX
#define BC_STRING_MAX _POSIX2_BC_STRING_MAX
#endif
#ifndef PTHREAD_KEYS_MAX
#define PTHREAD_KEYS_MAX 1024
#endif
#ifndef _POSIX_OPEN_MAX
#define _POSIX_OPEN_MAX 20
#endif
#ifndef _POSIX_THREAD_KEYS_MAX
#define _POSIX_THREAD_KEYS_MAX 128
#endif
#ifndef BC_BASE_MAX
#define BC_BASE_MAX _POSIX2_BC_BASE_MAX
#endif
#ifndef MB_LEN_MAX
#define MB_LEN_MAX 16
#endif
#endif

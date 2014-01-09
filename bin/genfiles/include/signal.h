#ifndef _SIGNAL_H_
#define _SIGNAL_H_
#ifndef ___sig_atomic_t_def_
#define ___sig_atomic_t_def_
typedef int __sig_atomic_t;
#endif
#ifndef ___sigset_t_def_
#define ___sigset_t_def_
typedef struct {unsigned long __val[1024U / (8U * sizeof(unsigned long))];} __sigset_t;
#endif
#ifndef _sig_atomic_t_def_
#define _sig_atomic_t_def_
typedef __sig_atomic_t sig_atomic_t;
#endif
#ifndef _sigset_t_def_
#define _sigset_t_def_
typedef __sigset_t sigset_t;
#endif
#ifndef ___uid_t_def_
#define ___uid_t_def_
typedef unsigned int __uid_t;
#endif
#ifndef ___pid_t_def_
#define ___pid_t_def_
typedef int __pid_t;
#endif
#ifndef ___clock_t_def_
#define ___clock_t_def_
typedef long __clock_t;
#endif
#ifndef ___sighandler_t_def_
#define ___sighandler_t_def_
typedef void (* __sighandler_t)(int);
#endif
#ifndef _sigval_t_def_
#define _sigval_t_def_
typedef union sigval{
  int sival_int;
  void * sival_ptr;
} sigval_t;
#endif
#ifndef _siginfo_t_def_
#define _siginfo_t_def_
typedef struct siginfo{
  int si_signo;
  int si_errno;
  int si_code;
  union {int _pad[128U / sizeof(int) - 3U];
	 struct {__pid_t si_pid;
		 __uid_t si_uid;} _kill;
	 struct {int si_tid;
		 int si_overrun;
		 sigval_t si_sigval;} _timer;
	 struct {__pid_t si_pid;
		 __uid_t si_uid;
		 sigval_t si_sigval;} _rt;
	 struct {__pid_t si_pid;
		 __uid_t si_uid;
		 int si_status;
		 __clock_t si_utime;
		 __clock_t si_stime;} _sigchld;
	 struct {void * si_addr;} _sigfault;
	 struct {long si_band;
		 int si_fd;} _sigpoll;} _sifields;
} siginfo_t;
#endif
#ifndef ___anonymous_enum_60___def_
#define ___anonymous_enum_60___def_
enum __anonymous_enum_60__{
  SIGEV_SIGNAL = 0,
  SIGEV_NONE = 1U,
  SIGEV_THREAD = 2U,
  SIGEV_THREAD_ID = 4
};
#endif
#ifndef _sigaction_def_
#define _sigaction_def_
struct sigaction{
  union {__sighandler_t sa_handler;
	 void (* sa_sigaction)(int,siginfo_t *,void *);} __sigaction_handler;
  __sigset_t sa_mask;
  int sa_flags;
  void (* sa_restorer)();
};
#endif
#ifndef _sigaction_def_
#define _sigaction_def_
extern int  __attribute__((nothrow))sigaction<`GR0::R,`GR1::R>(int __sig,
							       const struct sigaction *`GR0 __act,
							       struct sigaction *`GR1 __oact);
#endif
#ifndef sa_sigaction
#define sa_sigaction __sigaction_handler.sa_sigaction
#endif
#ifndef SIGBUS
#define SIGBUS 7
#endif
#ifndef SIGTTIN
#define SIGTTIN 21
#endif
#ifndef SIGPROF
#define SIGPROF 27
#endif
#ifndef SIGFPE
#define SIGFPE 8
#endif
#ifndef SIGHUP
#define SIGHUP 1
#endif
#ifndef SIGTTOU
#define SIGTTOU 22
#endif
#ifndef SIGUSR1
#define SIGUSR1 10
#endif
#ifndef SA_SIGINFO
#define SA_SIGINFO 4
#endif
#ifndef SA_ONSTACK
#define SA_ONSTACK 0x08000000
#endif
#ifndef SIGURG
#define SIGURG 23
#endif
#ifndef SIGIO
#define SIGIO 29
#endif
#ifndef SIGQUIT
#define SIGQUIT 3
#endif
#ifndef SIGEV_NONE
#define SIGEV_NONE SIGEV_NONE
#endif
#ifndef sa_handler
#define sa_handler __sigaction_handler.sa_handler
#endif
#ifndef SA_NODEFER
#define SA_NODEFER 0x40000000
#endif
#ifndef SIGABRT
#define SIGABRT 6
#endif
#ifndef SA_RESETHAND
#define SA_RESETHAND 0x80000000
#endif
#ifndef SIG_BLOCK
#define SIG_BLOCK 0
#endif
#ifndef SIGTRAP
#define SIGTRAP 5
#endif
#ifndef SIGEV_THREAD
#define SIGEV_THREAD SIGEV_THREAD
#endif
#ifndef SIGVTALRM
#define SIGVTALRM 26
#endif
#ifndef SIGPOLL
#define SIGPOLL SIGIO
#endif
#ifndef SIG_UNBLOCK
#define SIG_UNBLOCK 1
#endif
#ifndef SIGSEGV
#define SIGSEGV 11
#endif
#ifndef SIGCONT
#define SIGCONT 18
#endif
#ifndef SIGPIPE
#define SIGPIPE 13
#endif
#ifndef SIGXFSZ
#define SIGXFSZ 25
#endif
#ifndef SIGCHLD
#define SIGCHLD 17
#endif
#ifndef SIGSYS
#define SIGSYS 31
#endif
#ifndef SIGSTOP
#define SIGSTOP 19
#endif
#ifndef SA_NOCLDSTOP
#define SA_NOCLDSTOP 1
#endif
#ifndef SIGALRM
#define SIGALRM 14
#endif
#ifndef SIGUSR2
#define SIGUSR2 12
#endif
#ifndef SIGTSTP
#define SIGTSTP 20
#endif
#ifndef SIGKILL
#define SIGKILL 9
#endif
#ifndef SIGXCPU
#define SIGXCPU 24
#endif
#ifndef SA_RESTART
#define SA_RESTART 0x10000000
#endif
#ifndef SIGEV_SIGNAL
#define SIGEV_SIGNAL SIGEV_SIGNAL
#endif
#ifndef SA_NOCLDWAIT
#define SA_NOCLDWAIT 2
#endif
#ifndef SIGILL
#define SIGILL 4
#endif
#ifndef SIGINT
#define SIGINT 2
#endif
#ifndef SIGTERM
#define SIGTERM 15
#endif
#ifndef SIG_SETMASK
#define SIG_SETMASK 2
#endif

  // We'd like to
  // include { sigevent sigval stack_t sigstack siginfo_t }
  // but in Linux this pulls in structures that contain void *'s.

  // If SIG_HOLD is defined in the C library, it is defined as an
  // integer cast to a function pointer, which won't be allowed in
  // Cyclone, so we undef it here.  We define _SIG_HOLD_def_ to
  // remember that we should support it, though.
  #ifdef SIG_HOLD
  #define _SIG_HOLD_def_
  #undef SIG_HOLD
  #endif

  extern "C" int kill(pid_t, int);

  extern "C" int raise(int);

  extern "C" int sigaddset(sigset_t@, int);  

  extern "C" int sigdelset(sigset_t@, int);

  extern "C" int sigemptyset(sigset_t@);

  extern "C" int sigfillset(sigset_t@);

  extern "C" int siginterrupt(int,int);

  extern "C" int sigismember(sigset_t@, int);

  typedef void (@sigarg_t)(int);
  extern void SIG_DFL(int);
  extern void SIG_ERR(int);
  extern void SIG_IGN(int);
  #ifdef _SIG_HOLD_def_
  extern void SIG_HOLD(int); // Rare
  #endif

  extern sigarg_t signal(int, sigarg_t);

  extern int sigaction(int, struct sigaction *, struct sigaction *);
  extern "C" struct sigaction fresh_sigaction();
#endif

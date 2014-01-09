#ifndef _SYS_SYSLOG_H_
#define _SYS_SYSLOG_H_
#ifndef LOG_PERROR
#define LOG_PERROR 0x20
#endif
#ifndef LOG_PID
#define LOG_PID 0x01
#endif
#ifndef LOG_ERR
#define LOG_ERR 3
#endif
#ifndef LOG_PRI
#define LOG_PRI(p) ((p) & LOG_PRIMASK)
#endif
#ifndef LOG_DAEMON
#define LOG_DAEMON (3<<3)
#endif
#ifndef LOG_MAKEPRI
#define LOG_MAKEPRI(fac,pri) (((fac) << 3) | (pri))
#endif
#ifndef LOG_NOWAIT
#define LOG_NOWAIT 0x10
#endif
#ifndef LOG_MAIL
#define LOG_MAIL (2<<3)
#endif
#ifndef LOG_UPTO
#define LOG_UPTO(pri) ((1 << ((pri)+1)) - 1)
#endif
#ifndef LOG_MASK
#define LOG_MASK(pri) (1 << (pri))
#endif
#ifndef LOG_FAC
#define LOG_FAC(p) (((p) & LOG_FACMASK) >> 3)
#endif
#ifndef LOG_CRIT
#define LOG_CRIT 2
#endif
#ifndef LOG_NOTICE
#define LOG_NOTICE 5
#endif
#ifndef LOG_CRON
#define LOG_CRON (9<<3)
#endif
#ifndef LOG_INFO
#define LOG_INFO 6
#endif
#ifndef LOG_FTP
#define LOG_FTP (11<<3)
#endif
#ifndef LOG_WARNING
#define LOG_WARNING 4
#endif
#ifndef LOG_AUTH
#define LOG_AUTH (4<<3)
#endif
#ifndef LOG_CONS
#define LOG_CONS 0x02
#endif
#ifndef LOG_ALERT
#define LOG_ALERT 1
#endif
#ifndef LOG_UUCP
#define LOG_UUCP (8<<3)
#endif
#ifndef LOG_NDELAY
#define LOG_NDELAY 0x08
#endif
#ifndef LOG_SYSLOG
#define LOG_SYSLOG (5<<3)
#endif
#ifndef LOG_EMERG
#define LOG_EMERG 0
#endif
#ifndef LOG_FACMASK
#define LOG_FACMASK 0x03f8
#endif
#ifndef LOG_KERN
#define LOG_KERN (0<<3)
#endif
#ifndef LOG_NFACILITIES
#define LOG_NFACILITIES 24
#endif
#ifndef LOG_ODELAY
#define LOG_ODELAY 0x04
#endif
#ifndef LOG_PRIMASK
#define LOG_PRIMASK 0x07
#endif
#ifndef LOG_NEWS
#define LOG_NEWS (7<<3)
#endif
#ifndef LOG_AUTHPRIV
#define LOG_AUTHPRIV (10<<3)
#endif
#ifndef LOG_LOCAL0
#define LOG_LOCAL0 (16<<3)
#endif
#ifndef LOG_LOCAL1
#define LOG_LOCAL1 (17<<3)
#endif
#ifndef LOG_LOCAL2
#define LOG_LOCAL2 (18<<3)
#endif
#ifndef LOG_LOCAL3
#define LOG_LOCAL3 (19<<3)
#endif
#ifndef LOG_LOCAL4
#define LOG_LOCAL4 (20<<3)
#endif
#ifndef LOG_LOCAL5
#define LOG_LOCAL5 (21<<3)
#endif
#ifndef LOG_LOCAL6
#define LOG_LOCAL6 (22<<3)
#endif
#ifndef LOG_LOCAL7
#define LOG_LOCAL7 (23<<3)
#endif
#ifndef LOG_DEBUG
#define LOG_DEBUG 7
#endif
#ifndef LOG_LPR
#define LOG_LPR (6<<3)
#endif
#ifndef LOG_USER
#define LOG_USER (1<<3)
#endif

extern "C" void closelog (void);
extern "C" int setlogmask (int);
  // This declares a not quite printf like function, we need to write
  // it in Cyclone.
  // void openlog (const char *, int, int);
  // void syslog (int, const char *, ...);
#endif

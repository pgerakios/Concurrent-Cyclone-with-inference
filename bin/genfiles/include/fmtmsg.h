#ifndef _FMTMSG_H_
#define _FMTMSG_H_
#ifndef ___anonymous_enum_22___def_
#define ___anonymous_enum_22___def_
enum __anonymous_enum_22__{
  MM_HARD = 1,
  MM_SOFT = 2,
  MM_FIRM = 4,
  MM_APPL = 8,
  MM_UTIL = 16,
  MM_OPSYS = 32,
  MM_RECOVER = 64,
  MM_NRECOV = 128,
  MM_PRINT = 256,
  MM_CONSOLE = 512
};
#endif
#ifndef ___anonymous_enum_23___def_
#define ___anonymous_enum_23___def_
enum __anonymous_enum_23__{
  MM_NOSEV = 0,
  MM_HALT = 1U,
  MM_ERROR = 2U,
  MM_WARNING = 3U,
  MM_INFO = 4U
};
#endif
#ifndef ___anonymous_enum_24___def_
#define ___anonymous_enum_24___def_
enum __anonymous_enum_24__{
  MM_NOTOK = -1,
  MM_OK = 0,
  MM_NOMSG = 1,
  MM_NOCON = 4
};
#endif
#ifndef MM_CONSOLE
#define MM_CONSOLE MM_CONSOLE
#endif
#ifndef MM_UTIL
#define MM_UTIL MM_UTIL
#endif
#ifndef MM_NULLMC
#define MM_NULLMC ((long int) 0)
#endif
#ifndef MM_NOSEV
#define MM_NOSEV MM_NOSEV
#endif
#ifndef MM_HARD
#define MM_HARD MM_HARD
#endif
#ifndef MM_RECOVER
#define MM_RECOVER MM_RECOVER
#endif
#ifndef MM_NRECOV
#define MM_NRECOV MM_NRECOV
#endif
#ifndef MM_PRINT
#define MM_PRINT MM_PRINT
#endif
#ifndef MM_HALT
#define MM_HALT MM_HALT
#endif
#ifndef MM_NOTOK
#define MM_NOTOK MM_NOTOK
#endif
#ifndef MM_OPSYS
#define MM_OPSYS MM_OPSYS
#endif
#ifndef MM_INFO
#define MM_INFO MM_INFO
#endif
#ifndef MM_NOMSG
#define MM_NOMSG MM_NOMSG
#endif
#ifndef MM_OK
#define MM_OK MM_OK
#endif
#ifndef MM_ERROR
#define MM_ERROR MM_ERROR
#endif
#ifndef MM_NULLSEV
#define MM_NULLSEV 0
#endif
#ifndef MM_FIRM
#define MM_FIRM MM_FIRM
#endif
#ifndef MM_WARNING
#define MM_WARNING MM_WARNING
#endif
#ifndef MM_APPL
#define MM_APPL MM_APPL
#endif
#ifndef MM_NOCON
#define MM_NOCON MM_NOCON
#endif
#ifndef MM_SOFT
#define MM_SOFT MM_SOFT
#endif

#define MM_NULLLBL ((const char ?)NULL)
#define MM_NULLTXT ((const char ?)NULL)
#define MM_NULLACT ((const char ?)NULL)
#define MM_NULLTAG ((const char ?)NULL)
/* FIX: should I worry about these char *s being non-NULL */
extern "C" int fmtmsg(long a, const char * b, int c, const char * d,
		      const char * e, const char * f);
#endif

#ifndef _LOCALE_H_
#define _LOCALE_H_
#ifndef ___anonymous_enum_28___def_
#define ___anonymous_enum_28___def_
enum __anonymous_enum_28__{
  __LC_CTYPE = 0,
  __LC_NUMERIC = 1,
  __LC_TIME = 2,
  __LC_COLLATE = 3,
  __LC_MONETARY = 4,
  __LC_MESSAGES = 5,
  __LC_ALL = 6,
  __LC_PAPER = 7,
  __LC_NAME = 8,
  __LC_ADDRESS = 9,
  __LC_TELEPHONE = 10,
  __LC_MEASUREMENT = 11,
  __LC_IDENTIFICATION = 12
};
#endif
#ifndef _lconv_def_
#define _lconv_def_
struct lconv{
  char * decimal_point;
  char * thousands_sep;
  char * grouping;
  char * int_curr_symbol;
  char * currency_symbol;
  char * mon_decimal_point;
  char * mon_thousands_sep;
  char * mon_grouping;
  char * positive_sign;
  char * negative_sign;
  char int_frac_digits;
  char frac_digits;
  char p_cs_precedes;
  char p_sep_by_space;
  char n_cs_precedes;
  char n_sep_by_space;
  char p_sign_posn;
  char n_sign_posn;
  char __int_p_cs_precedes;
  char __int_p_sep_by_space;
  char __int_n_cs_precedes;
  char __int_n_sep_by_space;
  char __int_p_sign_posn;
  char __int_n_sign_posn;
};
#endif
#ifndef LC_ALL
#define LC_ALL __LC_ALL
#endif
#ifndef LC_CTYPE
#define LC_CTYPE __LC_CTYPE
#endif
#ifndef LC_TIME
#define LC_TIME __LC_TIME
#endif
#ifndef LC_ADDRESS
#define LC_ADDRESS __LC_ADDRESS
#endif
#ifndef LC_MONETARY
#define LC_MONETARY __LC_MONETARY
#endif
#ifndef LC_IDENTIFICATION
#define LC_IDENTIFICATION __LC_IDENTIFICATION
#endif
#ifndef LC_MEASUREMENT
#define LC_MEASUREMENT __LC_MEASUREMENT
#endif
#ifndef LC_PAPER
#define LC_PAPER __LC_PAPER
#endif
#ifndef LC_COLLATE
#define LC_COLLATE __LC_COLLATE
#endif
#ifndef LC_NUMERIC
#define LC_NUMERIC __LC_NUMERIC
#endif
#ifndef LC_TELEPHONE
#define LC_TELEPHONE __LC_TELEPHONE
#endif
#ifndef LC_MESSAGES
#define LC_MESSAGES __LC_MESSAGES
#endif
#ifndef LC_NAME
#define LC_NAME __LC_NAME
#endif

  extern "C" char *setlocale (int category, const char *locale);

  extern "C" struct lconv *localeconv (void);
#endif

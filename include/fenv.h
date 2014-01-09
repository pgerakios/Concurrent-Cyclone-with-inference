#ifndef _FENV_H_
#define _FENV_H_
#ifndef ___anonymous_enum_20___def_
#define ___anonymous_enum_20___def_
enum __anonymous_enum_20__{
  FE_INVALID = 1,
  __FE_DENORM = 2,
  FE_DIVBYZERO = 4,
  FE_OVERFLOW = 8,
  FE_UNDERFLOW = 16,
  FE_INEXACT = 32
};
#endif
#ifndef ___anonymous_enum_21___def_
#define ___anonymous_enum_21___def_
enum __anonymous_enum_21__{
  FE_TONEAREST = 0,
  FE_DOWNWARD = 1024,
  FE_UPWARD = 2048,
  FE_TOWARDZERO = 3072
};
#endif
#ifndef _fexcept_t_def_
#define _fexcept_t_def_
typedef unsigned short fexcept_t;
#endif
#ifndef _fenv_t_def_
#define _fenv_t_def_
typedef struct {unsigned short __control_word;
		unsigned short __unused1;
		unsigned short __status_word;
		unsigned short __unused2;
		unsigned short __tags;
		unsigned short __unused3;
		unsigned int __eip;
		unsigned short __cs_selector;
		unsigned int __opcode:11;
		unsigned int __unused4:5;
		unsigned int __data_offset;
		unsigned short __data_selector;
		unsigned short __unused5;} fenv_t;
#endif
#ifndef FE_INVALID
#define FE_INVALID FE_INVALID
#endif
#ifndef FE_OVERFLOW
#define FE_OVERFLOW FE_OVERFLOW
#endif
#ifndef FE_DIVBYZERO
#define FE_DIVBYZERO FE_DIVBYZERO
#endif
#ifndef FE_TONEAREST
#define FE_TONEAREST FE_TONEAREST
#endif
#ifndef FE_UPWARD
#define FE_UPWARD FE_UPWARD
#endif
#ifndef FE_DFL_ENV
#define FE_DFL_ENV ((__const fenv_t *) -1)
#endif
#ifndef FE_TOWARDZERO
#define FE_TOWARDZERO FE_TOWARDZERO
#endif
#ifndef FE_INEXACT
#define FE_INEXACT FE_INEXACT
#endif
#ifndef FE_ALL_EXCEPT
#define FE_ALL_EXCEPT (FE_INEXACT | FE_DIVBYZERO | FE_UNDERFLOW | FE_OVERFLOW | FE_INVALID)
#endif
#ifndef FE_UNDERFLOW
#define FE_UNDERFLOW FE_UNDERFLOW
#endif
#ifndef FE_DOWNWARD
#define FE_DOWNWARD FE_DOWNWARD
#endif

  extern "C" int feclearexcept(int);

  extern "C" int fegetexceptflag(fexcept_t @, int);

  extern "C" int feraiseexcept(int);

  extern "C" int fesetexceptflag(const fexcept_t @, int);

  extern "C" int fetestexcept(int);

  extern "C" int fegetround(void);

  extern "C" int fesetround(int);

  extern "C" int fegetenv(fenv_t @);

  extern "C" int feholdexcept(fenv_t @);

  extern "C" int fesetenv(const fenv_t @);

  extern "C" int feupdateenv(const fenv_t @);
#endif

#ifndef _MATH_H_
#define _MATH_H_
#ifndef _isinf_def_
#define _isinf_def_
extern int  __attribute__((nothrow))isinf(double __value) __attribute__((const));
#endif
#ifndef _isnan_def_
#define _isnan_def_
extern int  __attribute__((nothrow))isnan(double __value) __attribute__((const));
#endif
#ifndef M_SQRT1_2
#define M_SQRT1_2 0.70710678118654752440
#endif
#ifndef M_PI_4
#define M_PI_4 0.78539816339744830962
#endif
#ifndef M_E
#define M_E 2.7182818284590452354
#endif
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923
#endif
#ifndef M_2_PI
#define M_2_PI 0.63661977236758134308
#endif
#ifndef M_LOG2E
#define M_LOG2E 1.4426950408889634074
#endif
#ifndef M_LN10
#define M_LN10 2.30258509299404568402
#endif
#ifndef M_SQRT2
#define M_SQRT2 1.41421356237309504880
#endif
#ifndef M_LOG10E
#define M_LOG10E 0.43429448190325182765
#endif
#ifndef M_1_PI
#define M_1_PI 0.31830988618379067154
#endif
#ifndef M_LN2
#define M_LN2 0.69314718055994530942
#endif

  /* I'd like to do
     include { HUGE_VAL }
     but under Cygwin this brings in 
     extern const union __dmath  __attribute__((dllimport))__infinity[];
     which does not have an array size.
     Works fine in Linux, though.
  */

  extern "C" double acos(double);

  extern "C" float acosf(float);

  extern "C" double acosh(double);

  extern "C" float acoshf(float);

  extern "C" long double acoshl(long double);

  extern "C" long double acosl(long double);

  extern "C" double asin(double);

  extern "C" float asinf(float);

  extern "C" double asinh(double);

  extern "C" float asinhf(float);

  extern "C" long double asinhl(long double);

  extern "C" long double asinl(long double);

  extern "C" double atan(double);

  extern "C" double atan2(double, double);

  extern "C" float atan2f(float, float);

  extern "C" long double atan2l(long double, long double);

  extern "C" float atanf(float);

  extern "C" double atanh(double);

  extern "C" float atanhf(float);

  extern "C" long double atanhl(long double);

  extern "C" long double atanl(long double);

  extern "C" double cbrt(double);

  extern "C" float cbrtf(float);

  extern "C" long double cbrtl(long double);

  extern "C" double ceil(double);

  extern "C" float ceilf(float);

  extern "C" long double ceill(long double);

  extern "C" double copysign(double, double);

  extern "C" float copysignf(float, float);

  extern "C" long double copysignl(long double, long double);

  extern "C" double cos(double);

  extern "C" float cosf(float);

  extern "C" double cosh(double);

  extern "C" float coshf(float);

  extern "C" long double coshl(long double);

  extern "C" long double cosl(long double);

  extern "C" double erf(double);

  extern "C" double erfc(double);

  extern "C" float erfcf(float);

  extern "C" long double erfcl(long double);

  extern "C" float erff(float);

  extern "C" long double erfl(long double);

  extern "C" double exp(double);

  extern "C" float expf(float);

  extern "C" long double expl(long double);

  extern "C" double expm1(double);

  extern "C" float expm1f(float);

  extern "C" long double expm1l(long double);

  extern "C" double fabs(double);

  extern "C" float fabsf(float);

  extern "C" long double fabsl(long double);

  extern "C" double floor(double);

  extern "C" float floorf(float);

  extern "C" long double floorl(long double);

  extern "C" double fmod(double, double);

  extern "C" float fmodf(float, float);

  extern "C" long double fmodl(long double, long double);

  extern "C" double frexp(double, int @);

  extern "C" float frexpf(float value, int @);

  extern "C" long double frexpl(long double value, int @);

  extern "C" double hypot(double, double);

  extern "C" float hypotf(float, float);

  extern "C" long double hypotl(long double, long double);

  extern "C" int ilogb(double);

  extern "C" int ilogbf(float);

  extern "C" int ilogbl(long double);

  extern "C" double j0(double);

  extern "C" double j1(double);

  extern "C" double jn(int, double);

  extern "C" double ldexp(double, int);

  extern "C" float ldexpf(float, int);

  extern "C" long double ldexpl(long double, int);

  extern "C" double lgamma(double);

  extern "C" float lgammaf(float);

  extern "C" long double lgammal(long double);

  extern "C" double log(double);

  extern "C" double log10(double);

  extern "C" float log10f(float);

  extern "C" long double log10l(long double);

  extern "C" double log1p(double);

  extern "C" float log1pf(float);

  extern "C" long double log1pl(long double);

  extern "C" double logb(double);

  extern "C" float logbf(float);

  extern "C" long double logbl(long double);

  extern "C" float logf(float);

  extern "C" long double logl(long double);

  extern "C" double modf(double, double @);

  extern "C" float modff(float, float @);

  extern "C" long double modfl(long double, long double @);

  extern "C" double nextafter(double, double);

  extern "C" float nextafterf(float, float);

  extern "C" long double nextafterl(long double, long double);

  extern "C" double pow(double, double);

  extern "C" float powf(float, float);

  extern "C" long double powl(long double, long double);

  extern "C" double remainder(double, double);

  extern "C" float remainderf(float, float);

  extern "C" long double remainderl(long double, long double);

  extern "C" double rint(double);

  extern "C" float rintf(float);

  extern "C" long double rintl(long double);

  extern "C" double scalbn(double, int);

  extern "C" float scalbnf(float, int);

  extern "C" long double scalbnl(long double, int);

  extern "C" double sin(double);

  extern "C" float sinf(float);

  extern "C" double sinh(double);

  extern "C" float sinhf(float);

  extern "C" long double sinhl(long double);

  extern "C" long double sinl(long double);

  extern "C" double sqrt(double);

  extern "C" float sqrtf(float);

  extern "C" long double sqrtl(long double);

  extern "C" double tan(double);

  extern "C" float tanf(float);

  extern "C" double tanh(double);

  extern "C" float tanhf(float);

  extern "C" long double tanhl(long double);

  extern "C" long double tanl(long double);

  extern "C" double y0(double);

  extern "C" double y1(double);

  extern "C" double yn(int, double);

  extern "C" int signgam;
#endif

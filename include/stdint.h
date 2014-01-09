#ifndef _STDINT_H_
#define _STDINT_H_
#ifndef _int8_t_def_
#define _int8_t_def_
typedef signed char int8_t;
#endif
#ifndef _int16_t_def_
#define _int16_t_def_
typedef short int16_t;
#endif
#ifndef _int32_t_def_
#define _int32_t_def_
typedef int int32_t;
#endif
#ifndef _int64_t_def_
#define _int64_t_def_
typedef long long int64_t;
#endif
#ifndef _uint8_t_def_
#define _uint8_t_def_
typedef unsigned char uint8_t;
#endif
#ifndef _uint16_t_def_
#define _uint16_t_def_
typedef unsigned short uint16_t;
#endif
#ifndef _uint32_t_def_
#define _uint32_t_def_
typedef unsigned int uint32_t;
#endif
#ifndef _uint64_t_def_
#define _uint64_t_def_
typedef unsigned long long uint64_t;
#endif
#ifndef _int_least8_t_def_
#define _int_least8_t_def_
typedef signed char int_least8_t;
#endif
#ifndef _int_least16_t_def_
#define _int_least16_t_def_
typedef short int_least16_t;
#endif
#ifndef _int_least32_t_def_
#define _int_least32_t_def_
typedef int int_least32_t;
#endif
#ifndef _int_least64_t_def_
#define _int_least64_t_def_
typedef long long int_least64_t;
#endif
#ifndef _uint_least8_t_def_
#define _uint_least8_t_def_
typedef unsigned char uint_least8_t;
#endif
#ifndef _uint_least16_t_def_
#define _uint_least16_t_def_
typedef unsigned short uint_least16_t;
#endif
#ifndef _uint_least32_t_def_
#define _uint_least32_t_def_
typedef unsigned int uint_least32_t;
#endif
#ifndef _uint_least64_t_def_
#define _uint_least64_t_def_
typedef unsigned long long uint_least64_t;
#endif
#ifndef _int_fast8_t_def_
#define _int_fast8_t_def_
typedef signed char int_fast8_t;
#endif
#ifndef _int_fast16_t_def_
#define _int_fast16_t_def_
typedef int int_fast16_t;
#endif
#ifndef _int_fast32_t_def_
#define _int_fast32_t_def_
typedef int int_fast32_t;
#endif
#ifndef _int_fast64_t_def_
#define _int_fast64_t_def_
typedef long long int_fast64_t;
#endif
#ifndef _uint_fast8_t_def_
#define _uint_fast8_t_def_
typedef unsigned char uint_fast8_t;
#endif
#ifndef _uint_fast16_t_def_
#define _uint_fast16_t_def_
typedef unsigned int uint_fast16_t;
#endif
#ifndef _uint_fast32_t_def_
#define _uint_fast32_t_def_
typedef unsigned int uint_fast32_t;
#endif
#ifndef _uint_fast64_t_def_
#define _uint_fast64_t_def_
typedef unsigned long long uint_fast64_t;
#endif
#ifndef _intptr_t_def_
#define _intptr_t_def_
typedef int intptr_t;
#endif
#ifndef _uintptr_t_def_
#define _uintptr_t_def_
typedef unsigned int uintptr_t;
#endif
#ifndef _intmax_t_def_
#define _intmax_t_def_
typedef long long intmax_t;
#endif
#ifndef _uintmax_t_def_
#define _uintmax_t_def_
typedef unsigned long long uintmax_t;
#endif
#ifndef WCHAR_MAX
#define WCHAR_MAX __WCHAR_MAX
#endif
#ifndef WCHAR_MIN
#define WCHAR_MIN __WCHAR_MIN
#endif
#ifndef __INT64_C
#define __INT64_C(c) c ## LL
#endif
#ifndef PTRDIFF_MAX
#define PTRDIFF_MAX (2147483647)
#endif
#ifndef PTRDIFF_MIN
#define PTRDIFF_MIN (-2147483647-1)
#endif
#ifndef SIZE_MAX
#define SIZE_MAX (4294967295U)
#endif
#ifndef WINT_MAX
#define WINT_MAX (4294967295u)
#endif
#ifndef UINTPTR_MAX
#define UINTPTR_MAX (4294967295U)
#endif
#ifndef WINT_MIN
#define WINT_MIN (0u)
#endif
#ifndef __UINT64_C
#define __UINT64_C(c) c ## ULL
#endif
#ifndef SIG_ATOMIC_MAX
#define SIG_ATOMIC_MAX (2147483647)
#endif
#ifndef SIG_ATOMIC_MIN
#define SIG_ATOMIC_MIN (-2147483647-1)
#endif
#ifndef INTPTR_MAX
#define INTPTR_MAX (2147483647)
#endif
#ifndef __WCHAR_MAX
#define __WCHAR_MAX (2147483647l)
#endif
#ifndef INTPTR_MIN
#define INTPTR_MIN (-2147483647-1)
#endif
#ifndef __WCHAR_MIN
#define __WCHAR_MIN (-2147483647l - 1l)
#endif
#ifndef UINTMAX_MAX
#define UINTMAX_MAX (__UINT64_C(18446744073709551615))
#endif
#ifndef UINTMAX_C
#define UINTMAX_C(c) c ## ULL
#endif
#ifndef INTMAX_MAX
#define INTMAX_MAX (__INT64_C(9223372036854775807))
#endif
#ifndef INTMAX_MIN
#define INTMAX_MIN (-__INT64_C(9223372036854775807)-1)
#endif
#ifndef INTMAX_C
#define INTMAX_C(c) c ## LL
#endif

  // Note that intptr_t, uintptr_t, and related macros
  // all involve casting to and from void *, so they
  // are probably useless in Cyclone
#endif

#ifndef _STDLIB_H_
#define _STDLIB_H_
#ifndef _size_t_def_
#define _size_t_def_
typedef unsigned int size_t;
#endif
#ifndef _wchar_t_def_
#define _wchar_t_def_
typedef long wchar_t;
#endif
#ifndef _div_t_def_
#define _div_t_def_
typedef struct {int quot;
		int rem;} div_t;
#endif
#ifndef _ldiv_t_def_
#define _ldiv_t_def_
typedef struct {long quot;
		long rem;} ldiv_t;
#endif
#ifndef ___ctype_get_mb_cur_max_def_
#define ___ctype_get_mb_cur_max_def_
extern size_t  __attribute__((nothrow))__ctype_get_mb_cur_max();
#endif
#ifndef EXIT_FAILURE
#define EXIT_FAILURE 1
#endif
#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#endif
#ifndef RAND_MAX
#define RAND_MAX 2147483647
#endif
#ifndef MB_CUR_MAX
#define MB_CUR_MAX (__ctype_get_mb_cur_max ())
#endif

  // malloc is a primitive in Cyclone so we don't need this declaration
  //  extern "C" void *malloc(size_t);

  // These functions aren't supported in Cyclone
  //  extern "C" void *calloc(size_t, size_t);
  //  extern "C" void *realloc(void *, size_t);

  // This group of functions is not yet supported in Cyclone
  //  extern "C" long a64l(const char *);
  //  extern "C" void *bsearch(const void *, const void *, size_t, size_t, int (*)(const void *, const void *));
  //  extern "C" char *ecvt(double, int, int *restrict, int *restrict); (LEGACY)
  //                                                                      extern "C" char *fcvt(double, int, int *restrict, int *restrict); // (LEGACY)
  //  extern "C" char *gcvt(double, int, char *); // (LEGACY)
  //  extern "C" int getsubopt(char **, char *const *, char **);
  //  extern "C" char *initstate(unsigned, char *, size_t);
  //  extern "C" char *l64a(long);
  //  extern "C" int mblen(const char *, size_t);
  //  extern "C" size_t mbstowcs(wchar_t *restrict, const char *restrict, size_t);
  //  extern "C" int mbtowc(wchar_t *restrict, const char *restrict, size_t);
  //  extern "C" char *mktemp(char *); // (LEGACY)
  //  extern "C" int posix_memalign(void **, size_t, size_t);
  //  extern "C" int posix_openpt(int);
  //  extern "C" char *ptsname(int);
  //  extern "C" char *realpath(const char *restrict, char *restrict);
  //  extern "C" int setenv(const char *, const char *, int);
  //  extern "C" void setkey(const char *);
  //  extern "C" char *setstate(const char *);
  //  extern "C" float strtof(const char *restrict, char **restrict);
  //  extern "C" long double strtold(const char *restrict, char **restrict);
  //  extern "C" long long strtoll(const char *restrict, char **restrict, int);
  //  extern "C" unsigned long long strtoull(const char *restrict, char **restrict, int);
  //  extern "C" int unsetenv(const char *);
  //  extern "C" size_t wcstombs(char *restrict, const wchar_t *restrict, size_t);
  //  extern "C" int wctomb(char *, wchar_t);


  // This is the type of abort according to Posix, but we have a
  // conflicting type for abort in core.h.
  //  extern "C" void abort(void);

  // core.h version duplicated here
  extern "C" `a abort() __attribute__((noreturn));

  extern "C" int abs(int);

  // FIX: this might not be safe.  The posix docs say that if
  // a function registered with atexit uses longjmp when called
  // by exit(), the results are undefined.  Since we use longjmp
  // for exceptions, we should look into this.  We may end up
  // having to write our own exit() function.
  extern "C" int atexit(void (@)(void));

  extern "C" double atof(const char @);

  extern "C" int atoi(const char @);

  extern "C" long atol(const char @);

  extern long long atoll(const char @);

  extern "C" div_t div(int, int);

  extern "C" double drand48(void);

  extern "C" double erand48(unsigned short @{3});

  // duplicates version in core.h
  extern "C" void exit(int) __attribute__((noreturn)) ;

  extern void free(`a::A ?);

  // FIX: should this be const?  Seems like we want these things
  //   immutable
  extern "C" char * getenv(const char @);

  extern "C" long jrand48(unsigned short @{3});

  // This is the correct type according to Posix but it produces
  // an error under Linux.  It looks like we are translating long
  // to int; not sure if this is the problem.
  //  extern "C" long labs(long);

  extern "C" void lcong48(unsigned short @{7});

  extern "C" ldiv_t ldiv(long, long);

  extern "C" long lrand48(void);

  // must be at least 6 chars long, with the last 6 ending in XXXXXX.
  // The C code checks this, so we can use a more liberal Cyclone type.
  extern "C" int mkstemp(char @);

  extern "C" long mrand48(void);

  extern "C" long nrand48(unsigned short @{3});

  extern "C" int putenv(const char @);

  extern void qsort(`a::A ?, size_t, tag_t<valueof_t(sizeof(`a))>,
                    int (@)(const `a @, const `a @));

  extern "C" int rand(void);

  extern "C" int rand_r(unsigned @);

  extern "C" long random(void);

  extern "C" unsigned short seed48(unsigned short @{3});

  extern "C" void srand(unsigned);

  extern "C" void srand48(long);

  extern "C" void srandom(unsigned);

  // FIX: should be const?
  extern "C" double strtod(char @`r, char @`r *);

  // FIX: should be const?
  extern "C" long strtol(char @`r, char @`r *, int);

  // FIX: should be const?
  extern "C" unsigned long strtoul(char @`r, char @`r *, int);

  extern "C" int system(const char @);
#endif

#ifndef _FNMATCH_H_
#define _FNMATCH_H_
#ifndef FNM_NOMATCH
#define FNM_NOMATCH 1
#endif
#ifndef FNM_PATHNAME
#define FNM_PATHNAME (1 << 0)
#endif
#ifndef FNM_NOESCAPE
#define FNM_NOESCAPE (1 << 1)
#endif
#ifndef FNM_PERIOD
#define FNM_PERIOD (1 << 2)
#endif

  // FIX: does this old comment still apply:
  // Our implementation copies the first two arguments.  This is
  // only a stopgap measure, the function should be rewitten in
  // Cyclone.
  extern "C" int fnmatch(char @, char @, int);
#endif

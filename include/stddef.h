#ifndef _STDDEF_H_
#define _STDDEF_H_
#ifndef _ptrdiff_t_def_
#define _ptrdiff_t_def_
typedef int ptrdiff_t;
#endif
#ifndef _size_t_def_
#define _size_t_def_
typedef unsigned int size_t;
#endif
#ifndef _wchar_t_def_
#define _wchar_t_def_
typedef long wchar_t;
#endif

  // NULL is not defined here because it is a keyword in Cyclone
#endif

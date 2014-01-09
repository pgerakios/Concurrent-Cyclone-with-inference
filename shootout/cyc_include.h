#define XRGN
#include <core.h>

/*  extern "C" void *htbl(region_t<`r>) //debug function
    #ifdef XRGN
    @ieffect( {`r,`bar1,`nat0} )
    @oeffect( {`r,`bar1,`nat0} )
    @nothrow
    @re_entrant
    #endif
    ;*/
extern "C" unsigned int sleep(unsigned int seconds)
#ifdef XRGN
  @nothrow
  @re_entrant
#endif
  ;


extern struct __cycFILE;
#ifndef _FILE_def_
#define _FILE_def_
typedef struct __cycFILE FILE;
#endif
extern const FILE @stdout;
extern const FILE @stdin;
extern const FILE @stderr;

extern int fflush(const FILE @)
#ifdef XRGN
  @nothrow
  @re_entrant
#endif
  ;
extern mbuffer_t<`r> strncpy(mbuffer_t<`r>,buffer_t,unsigned int);
extern mstring_t<`r> strcpy(mstring_t<`r> dest,buffer_t src);

extern char ? @nozeroterm`r fgets(char ? @nozeroterm`r, int, const FILE @)
#ifdef XRGN
  @ieffect( {`r,`bar1,`nat1} )
  @oeffect( {`r,`bar1,`nat1} )
  @nothrow
  @re_entrant
#endif
  ;

extern unsigned int strlen(buffer_t<`r> s)
#ifdef XRGN
  @ieffect( {`r,`bar1,`nat1} )
  @oeffect( {`r,`bar1,`nat1} )
  @nothrow
  @re_entrant
#endif
  ;

/* Extremely ad-hoc !*/
extern int strncmp(buffer_t<`r> s1, buffer_t s2, unsigned int len)
#ifdef XRGN
  @ieffect( {`r,`bar1,`nat1} )
  @oeffect( {`r,`bar1,`nat1} )
  @nothrow
  @re_entrant
#endif
  ;



extern "C" int write(int fd, const void @{1}`r buf, int count)
#ifdef XRGN
  @ieffect( {`r,`bar1,`nat1} )
  @oeffect( {`r,`bar1,`nat1} )
  @nothrow
  @re_entrant
#endif
  ;

extern  void set_default_xregion_page_size( unsigned int )
#ifdef XRGN
  @nothrow
  @re_entrant
#endif
  ;

namespace Core{

  extern "C include" {
    extern void exit(int); 
    `a::A ? Cyc_Core_xmkfat(__nn_cyclone_internal_array_t<`a,`i,`r> arr,
                            sizeof_t<`a> s,
                            __cyclone_internal_singleton<`i> n) {
      struct _fat_ptr res;
      res.curr = arr;
      res.base = arr;
      res.last_plus_one = arr + s*n;
      return res;
    }
    `a::A @ Cyc_Core_safecast( `a::A *p ){
      if( p == 0 ) exit(-1);
      return p;
    }

  } export { Cyc_Core_xmkfat , Cyc_Core_safecast}

  extern `a @`r safecast( `a *`r )
#ifdef XRGN
    @ieffect( {`r,`bar1,`nat1} )
    @oeffect( {`r,`bar1,`nat1} )
    @nothrow
    @re_entrant
#endif
    ;

  extern `a?`r xmkfat(__nn_cyclone_internal_array_t<`a,`i,`r> arr,
                      sizeof_t<`a> s, __cyclone_internal_singleton<`i> n)
#ifdef XRGN
    @ieffect( {`r,`bar1,`nat1} )
    @oeffect( {`r,`bar1,`nat1} )
    @nothrow
    @re_entrant
#endif
    ;

  extern  void main_join_until(unsigned int,unsigned int)
#ifdef XRGN
    @nothrow
    @re_entrant
#endif
    ;

#define main_join() Core::main_join_until(0,0)

  extern  void waitValue( volatile int @`r ptr,
                          int ptr1,
                          int eq
                          )
#ifdef XRGN
    @ieffect( {`r,`bar1,`nat0} )
    @oeffect( {`r,`bar1,`nat0} )
    @nothrow
    @re_entrant
#endif
    ;

  extern  void checkValueYield( volatile int @`r ptr,
                          int ptr1,
                          int eq
                          )
#ifdef XRGN
    @ieffect( {`r,`bar1,`nat0} )
    @oeffect( {`r,`bar1,`nat0} )
    @nothrow
    @re_entrant
#endif
    ;

  extern  void signalValue( volatile int @`r ptr,
                            int procs
                            )
#ifdef XRGN
    @ieffect( {`r,`bar1,`nat0} )
    @oeffect( {`r,`bar1,`nat0} )
    @nothrow
    @re_entrant
#endif
    ;


  extern void print_xstring(const char *@nozeroterm`r)
#ifdef XRGN
    @ieffect( {`r,`bar1,`nat1} )
    @oeffect( {`r,`bar1,`nat1} )
    @nothrow
    @re_entrant
#endif
    ;
}
extern "C"  int sched_yield(void) @re_entrant;
extern "C" int puts(const char @);
extern mstring_t<`r> strcpy(mstring_t<`r> dest,buffer_t src);

//#include <stdlib.h>
//#include <stdio.h>

extern "C" void exit(int)
#ifdef XRGN
  @nothrow
  @re_entrant
#endif
  __attribute__((noreturn))
  ;
extern "C" int atoi(const char @);

extern datatype PrintArg<`r> {
  String_pa(const char ? @nozeroterm`r);
  Int_pa(unsigned long);
  Double_pa(double);
  LongDouble_pa(long double);
  ShortPtr_pa(short @`r);
  IntPtr_pa(unsigned long @`r);
};

#ifndef _parg_t_def_
#define _parg_t_def_
typedef datatype PrintArg<`r> @`r parg_t<`r>;
#endif

extern int printf(const char ?, ... inject parg_t)
#ifdef XRGN
  @nothrow
  @re_entrant
#endif
  __attribute__((format(printf,1,2)))
  ;

extern int fprintf(const FILE @, const char ?, ... inject parg_t)
#ifdef XRGN
  @nothrow
  @re_entrant
#endif
  __attribute__((format(printf,2,3)))
  ;

extern char ?`r rprintf(region_t<`r>, const char ?, ... inject parg_t)
#ifdef XRGN
  @ieffect( {`r,`bar1,`nat0} )
  @oeffect( {`r,`bar1,`nat0} )
  @nothrow
  @re_entrant
#endif
  __attribute__((format(printf,2,3)))
  ;

extern unsigned int fwrite(const char ? @nozeroterm `r,
                           unsigned int, unsigned int, const FILE @)
#ifdef XRGN
  @ieffect( {`r,`bar1,`nat0} )
  @oeffect( {`r,`bar1,`nat0} )
  @nothrow
  @re_entrant
#endif
  ;

using Core;

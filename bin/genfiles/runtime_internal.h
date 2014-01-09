/* This file is part of the Cyclone Library.
   Copyright (C) 2004 Dan Grossman, AT&T

   This library is free software; you can redistribute it and/or it
   under the terms of the GNU Lesser General Public License as
   published by the Free Software Foundation; either version 2.1 of
   the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place, Suite
   330, Boston, MA 02111-1307 USA. */
#ifndef RUNTIME_INTERNAL_H
#define RUNTIME_INTERNAL_H

/************* PLATFORM-SPECIFIC ELEMENTS *****************/

#if defined(GEEKOS)

//#define HAVE_THREADS 1

#include <geekos/setjmp.h>
#include <geekos/screen.h>  // for error printing
#include <geekos/kthread.h> // for Exit
#include <geekos/string.h> // for memset

// #include <limits.h> // for magic numbers
#define INT_MAX 0x7fffffff
#define bzero(_addr,_szb) memset(_addr,'\0',_szb)

// error printing functions
#define errprintf Print
#define errquit(arg...) { Print(arg); Exit(1); }

// thread-local accessor functions
#define create_tlocal_key Tlocal_Create
#define get_tlocal Tlocal_Get
#define put_tlocal Tlocal_Put

#else

#include <setjmp.h> // precore_c.h uses jmp_buf without defining it
#include <stdio.h>  // for error printing
#include <limits.h> // for magic numbers
#ifdef _HAVE_PTHREAD_
#define HAVE_THREADS 1
#include <pthread.h>

// thread-local accessor functions
#define tlocal_key_t pthread_key_t
#define create_tlocal_key pthread_key_create
#define get_tlocal pthread_getspecific
#define put_tlocal pthread_setspecific
#endif

// error printing functions
void exit(int);
#define errprintf(arg...) fprintf(stderr,##arg)
#define errquit(arg...) { fprintf(stderr,##arg); exit(1); }

#endif

#define MAX_ALLOC_SIZE INT_MAX

// only need things here that are not in cyc_include.h, which end up
// included because of the following:

// The C include file precore_c.h is produced (semi) automatically
// from the Cyclone include file core.h.  Note, it now includes
// the contents of cyc_include.h

/* RUNTIME_CYC defined to prevent including parts of precore_c.h that
   might cause problems, particularly relating to region profiling */
#define RUNTIME_CYC
#include "precore_c.h"
#include "runtime_lowlevel.h"


/* INIT and FINI ROUTINES:
 * These should be called before executing any Cyclone code from C.
   Only call them once (see runtime_cyc.c). */

/************** EXCEPTION ROUTINES ************/
//extern void _init_exceptions();  // defined in runtime_exception.c
extern char *_set_top_handler(); // defined in runtime_exception.c
/*************** STACK ROUTINES *******************/

//extern void _init_stack();       // defined in runtime_stack.c
// pushes a frame on the stack
void _push_frame(struct _RuntimeStack *frame);

// pop N+1 frames from the stack (error if stack_size < N+1)
void _npop_frame(unsigned int n);

// returns top frame on the stack (NULL if stack is empty)
//struct _RuntimeStack * _top_frame();
#define _top_frame() ((struct _RuntimeStack *) tcb()->stack._current_frame)


// pops off frames until a frame with the given tag is reached.  This
// frame is returned, or else NULL if none found.
//struct _RuntimeStack * _pop_frame_until(int tag);
//private
void* _throw_stack_fn(); 


typedef void (*stackoverflow_handler_t) 
	(int emergency, void *scp);
extern int stackoverflow_install_handler
	(stackoverflow_handler_t handler, 
	 void* extra_stack, unsigned long extra_stack_size);
extern int sigsegv_install_handler(int (*)(void*,int));
void _exn_stackoverflow_handler (int emergency, void *scp);
int  _exn_segv_handler (void *, int);

/************* MEMORY ROUTINES ************************/
extern void _init_regions(); 
// defined in runtime_memory.c

/* This is called when the program is finished, to finalize
   any profiling or other bookkeeping. */
extern void _fini_regions();     // defined in runtime_memory.c


// better not get called with the heap or unique regions ...
struct _RegionCaps
{
 unsigned int  rcnt; //Region count.One per thread.
 unsigned int  lock;  //lock for this region
 unsigned int  writer_cv; // condition var for writers
 unsigned int  reader_cv; // condition var for readers
 unsigned int  writer_q; 
 unsigned int  reader_q; 
//Reader lock count: one count per thread
// rdcnt is not taken into account when
// the write count is positive.
  unsigned int rd_cnt; 
};

struct bget_region_key;
struct _XRegionHandle
{
  int is_xrgn;
  struct _RegionCaps caps;
  struct _RegionPage *curr;
  char               *offset;
  char               *last_plus_one;
  struct bget_region_key *key;
//  struct _XRegionHandle *parent;
  struct _XTreeHandle *single_thread_key; 
//  void *sub_regions;
};

void print_xtree( struct _XTreeHandle *,const unsigned int);

struct hashtable;
typedef  struct _XTreeHandle *  entry_t;
typedef  struct _XRegionHandle * tkey_t;
struct hashtable * create_hashtable(unsigned int );
int hashtable_insert(struct hashtable **h, entry_t );
entry_t hashtable_search(struct hashtable *h, tkey_t);
int  hashtable_remove(struct hashtable **h, tkey_t);
void hashtable_destroy(struct hashtable *);

#define LPAREN (
#define RPAREN )

////////////////////////////////////////////////////////////////////
struct _ExnRecord {
	struct _handler_cons top_handler;
	struct _xtunion_struct *_exn_thrown;
	const char *_exn_filename;
	int	_exn_lineno;
   int (*uncaught_fun)();
	int in_uncaught_fun; 
};

struct _StackRecord {
 struct _EffectFrame * _current_effect_frame;
 struct _RuntimeStack *_current_frame;
};

struct _ThreadKey {
	unsigned int  tid; // store tid for fast lookup
	struct hashtable *h; // map: _XRegionHandle -> _XTreeKey
	struct _ExnRecord exn;
	struct _StackRecord stack;
};
//#define _ThreadKeyInitializer {0,0,0,0,{{{0,0}},0,0,0,0,0},{0,0}}

#define tcb()  ((struct _ThreadKey *) &_thread_key)
#define tid() (tcb()->tid)

#ifdef HAVE_THREADS

//#define DEBUG_get_tkey
#ifdef  DEBUG_get_tkey
#define dbgmsg_get_tkey(arg...)  { fprintf(stderr,##arg); fflush(stderr);}
#else
#define dbgmsg_get_tkey(arg...){}
#endif


// thread control block
#define get_tkey(z) LPAREN(struct _XTreeHandle *) LPAREN { \
 struct _XTreeHandle *ret = NULL;\
 assert(z > 100);\
 if( z->caps.rcnt == 1 ){\
   ret = (struct _XTreeHandle *)\
		z->single_thread_key;\
   if( ret == NULL ) \
     ret = hashtable_search(tcb()->h,z); \
 } else { \
   ret = hashtable_search(tcb()->h,z); \
 }\
 assert(ret > 100);\
 ret;\
} RPAREN RPAREN

#define hinsert(tkey) hashtable_insert(&(tcb()->h),tkey)
#define hremove(tkey) hashtable_remove(&(tcb()->h),tkey)
 
#else
#define hinsert(tkey) {}
#define hremove(tkey) {}
#define get_tkey(z)   (z->single_thread_key)
#endif

void _xregion_cap_th(int rgc,int wc,int rc ,
                     struct _XTreeHandle *t);
unsigned int _next_tid(); 
void _init_thread_context(unsigned int id);
int mprotect(const void *addr, size_t len, int prot);
extern __thread struct _ThreadKey _thread_key;
extern void GC_free(void *);
extern int strncmp(const char *s1, const char *s2, size_t n);
extern void pthread_exit(void *);
extern volatile unsigned int all_done;

void *memset(void *s, int c, size_t n);
void *memcpy(void *dest, const void *src, size_t n);
size_t strlen(const char *s);
void *alloca(size_t size);
void *calloc(size_t nmemb, size_t size);
void *malloc(size_t size);
void free(void *ptr);
void *realloc(void *ptr, size_t size);

#ifdef HAVE_THREADS
 //pasted from ../gc/include/gc_pthread_redirects.h
 int GC_pthread_create(pthread_t *new_thread,
                          const pthread_attr_t *attr,
                void *(*start_routine)(void *), void *arg);
    //int GC_pthread_sigmask(int how, const sigset_t *set, sigset_t *oset);
 int GC_pthread_join(pthread_t thread, void **retval);
 int GC_pthread_detach(pthread_t thread);
#endif
#endif

/* This file is part of the Cyclone Library.
   Copyright (C) 2001 AT&T

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

#include <stddef.h> // for size_t

#ifdef CYC_REGION_PROFILE
#undef GC_malloc
#undef GC_malloc_atomic
#undef GC_free
#undef GC_size
#endif

/****************************************************************************/
/* Stuff that's shared amongst all the different ways to implement malloc() */
/****************************************************************************/

/* total number of bytes allocated */
static size_t total_bytes_allocd = 0;

unsigned int GC_gc_no = 0;
int GC_dont_expand = 0;
int GC_use_entire_heap = 0;

size_t GC_get_total_bytes() {
  return total_bytes_allocd;
}

void GC_set_max_heap_size(unsigned int sz) {
  return;
}

/* These type/macro defns are taken from gc/include/gc.h and must
   be kept in sync. */
#   define GC_PROTO(args) args
typedef unsigned long GC_word;
typedef void (*GC_warn_proc) GC_PROTO((char *msg, GC_word arg));

void GC_default_warn_proc(char *msg, GC_word arg) {
  return;
}
GC_warn_proc GC_set_warn_proc(GC_warn_proc p) {
  return GC_default_warn_proc;
}

typedef void * GC_PTR;
typedef void (*GC_finalization_proc)
  	GC_PROTO((GC_PTR obj, GC_PTR client_data));
#define GC_API extern

GC_API void
GC_register_finalizer_no_order (GC_PTR obj, GC_finalization_proc fn, GC_PTR cd,
                                GC_finalization_proc *ofn, GC_PTR *ocd) {
  // empty
}


#if !defined(GEEKOS)

/* This is the Doug Lea allocator, rather than the platform malloc.  
   At the moment, only allow this for Linux & Apple, though in principle it
   should work in general. */

#if (defined(__linux__) || defined(__APPLE__))

#ifdef CYC_REGION_PROFILE
#include <unistd.h>
/* Override sbrk in the allocator so that it prints profiling info */
extern void CYCALLOCPROFILE_GC_add_to_heap(void*,unsigned int);
void *CYCALLOCPROFILE_sbrk(int incr) {
  void *before = sbrk(0);
  void *ret = sbrk(incr);
  // FIX: doesn't handle deallocation (incr < 0)
  if ((int)ret >= 0 && incr > 0)
    CYCALLOCPROFILE_GC_add_to_heap(before,incr);
  return ret;
}
#define MORECORE CYCALLOCPROFILE_sbrk
#endif

//#define DOUG_LEA
#ifdef _HAVE_PTHREAD_
/* Make this implementation of malloc safe */
#define USE_MALLOC_LOCK
#endif

//#include "malloc.c"

#ifdef  _HAVE_PTHREAD_

#ifndef DOUG_LEA
#define pthread_t       int
#define pthread_attr_t  int
int pthread_create(pthread_t *new_thread,
                          const pthread_attr_t *attr,
                void *(*start_routine)(void *), void *arg);
#endif

int GC_pthread_create(pthread_t *new_thread,
                          const pthread_attr_t *attr,
                void *(*start_routine)(void *), void *arg){
	return pthread_create(new_thread,attr,start_routine,arg);
}
#ifndef DOUG_LEA
#undef pthread_t
#undef pthread_attr_t
#endif
#endif 

#ifdef DOUG_LEA
#define malloc_sizeb(p,n) mUSABLe(p)
#define calloc cALLOc
#define malloc mALLOc
#define realloc rEALLOc
#define free fREe
#endif

size_t GC_size(void *x) {
 return 
 #ifdef DOUG_LEA
    mUSABLe(x);
 #else
 	 0;
 #endif

}

size_t GC_get_heap_size() {
/*   return mALLINFo().arena; */
/*   return mALLINFo().uordblks; */
  	return 
	 #ifdef DOUG_LEA
	 mALLINFo().usmblks
	#else 
		0
	#endif
	;
}

size_t GC_get_free_bytes() {  
return
	 #ifdef DOUG_LEA
		 mALLINFo().fordblks;
	 #else
		0
	 #endif
	;
}

#else

/* This is the platform default allocator (malloc) */

#include <stddef.h> // needed for size_t

static size_t last_alloc_bytes = 0;

#define malloc_sizeb(p,n) (last_alloc_bytes = n)

/* hack: assumes this is called immediately after an allocation */
size_t GC_size(void *x) {
  return last_alloc_bytes;
}

size_t GC_get_heap_size() {
  return 0;
}

size_t GC_get_free_bytes() {
  return 0;
}

#endif
#include <stdio.h> 
void *GC_malloc(int x) {
  // FIX:  I'm calling calloc to ensure the memory is zero'd.  This
  // is because I had to define GC_calloc in runtime_memory.c
  void *p = (void*)calloc(sizeof(char),x);
#ifdef DOUG_LEA
  total_bytes_allocd += malloc_sizeb(p,x);
#endif
  return p;
}

void *GC_malloc_atomic(int x) {
  // FIX:  I'm calling calloc to ensure the memory is zero'd.  This
  // is because I had to define GC_calloc in runtime_memory.c
  void *p = (void*)malloc(x);
#ifdef DOUG_LEA  
  total_bytes_allocd += malloc_sizeb(p,x);
#endif
  return p;
}

void *GC_realloc(void *x, size_t n) {
#ifdef GC_realloc	
  size_t sz = malloc_sizeb(x,0);
#endif  
  void *p = (void *)realloc(x,n);
#ifdef DOUG_LEA  
  total_bytes_allocd = total_bytes_allocd + (malloc_sizeb(p,n)-sz);
#endif
  return p;
}

void GC_free(void *x) {
  free(x);
}

#else /* defined(GEEKOS) */

#include <stddef.h> // needed for size_t
#include <geekos/int.h>
#include <geekos/bget.h>

#if defined(CYC_REGION_PROFILE)
#error "Region profiling not supported for GeekOS (no GC_size function)"
#endif

size_t GC_get_heap_size() {
  bool iflag;
  bufsize allocsize, freesize, ign2, ign3, ign4;

  iflag = Begin_Int_Atomic();
  bstats(&allocsize,&freesize,&ign2,&ign3,&ign4);
  End_Int_Atomic(iflag);

  return allocsize+freesize;
}

size_t GC_get_free_bytes() {
  bool iflag;
  bufsize freesize, ign1, ign2, ign3, ign4;

  iflag = Begin_Int_Atomic();
  bstats(&ign1,&freesize,&ign2,&ign3,&ign4);
  End_Int_Atomic(iflag);

  return freesize;
}

void *GC_malloc(int x) {
  bool iflag;
  void *result;

  KASSERT(x > 0);

  iflag = Begin_Int_Atomic();
  /* FIX: I'm calling bgetz to ensure the memory is zero'd.  This is
     because I had to define GC_calloc in runtime_memory.c */
  result = bgetz(x);
  total_bytes_allocd += x; /* FIX: include overhead? */
  End_Int_Atomic(iflag);

  return result;
}

void *GC_malloc_atomic(int x) {
  return GC_malloc(x);
}

void *GC_realloc(void *x, size_t n) {
  bool iflag;
  void *result;

  KASSERT(n > 0);

  iflag = Begin_Int_Atomic();
  /* FIX: I'm calling bgetz to ensure the memory is zero'd.  This is
     because I had to define GC_calloc in runtime_memory.c */
  result = bgetr(x,n);
  total_bytes_allocd += n; /* FIX: subtract old size, add overhead */
  End_Int_Atomic(iflag);

  return result;
}

void GC_free(void *x) {
  bool iflag;
  
  iflag = Begin_Int_Atomic();
  brel(x);
  End_Int_Atomic(iflag);
}
#endif

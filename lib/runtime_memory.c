/* This file is part of the Cyclone Library.
   Copyright (C) 2001 Greg Morrisett, AT&T

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

// This is the C "runtime library" to be used with the output of the
// Cyclone to C translator

#ifdef CYC_REGION_PROFILE
#include <stdarg.h>
#include <signal.h>
#include <time.h> // for clock()
#endif

#include "runtime_internal.h"

void print_xtree( struct _XTreeHandle *tk, const unsigned int depth);


#define stack_offset		  ({struct _RegionHandle tmp; \
									 (int) &tmp.s - (int) &tmp;})
#define cast_stack_rgn(x) ((struct _RegionHandle *) \
									(((char *) x) - stack_offset ))
			
#define cast_rgn_stack(x)  ((struct _RuntimeStack *) \
									(((char *) x) + stack_offset))

#define __RUNTIME_XREGION_DISABLE_GC

//#define  __RUNTIME_MEMORY_DEBUG_LOCKOP
//#define __RUNTIME_MEMORY_DEBUG_NEW_XREGION
//#define __RUNTIME_MEMORY_DEBUG_XFREE
//#define __RUNTIME_MEMORY_DEBUG_XMALLOC


//#define __RUNTIME_MEMORY_DEBUG
//#define __RUNTIME_MEMORY_DEBUG_ADD_SUBREGION
//#define __RUNTIME_MEMORY_DEBUG__XDEC
//#define __RUNTIME_MEMORY_DEBUG_XDEC_FREE
//#define __RUNTIME_MEMORY_DEBUG_XDEC_BOTH
//#define __RUNTIME_REGION_DISABLE_GC

#ifdef CYC_REGION_PROFILE
	#undef __RUNTIME_REGION_DISABLE_GC
	#undef __RUNTIME_XREGION_DISABLE_GC
#endif

//////////////////////////////////////////////
#ifdef   __RUNTIME_MEMORY_DEBUG_XDEC_BOTH
 #define dbgmsg_xdecboth(arg...)  { fprintf(stderr,##arg); fflush(stderr);}
#else
	#define dbgmsg_xdecboth(arg...)  {}
#endif

#ifdef  __RUNTIME_MEMORY_DEBUG_XMALLOC
 #define dbgmsg_xmalloc(arg...)  { fprintf(stderr,##arg); fflush(stderr);}
#else
	#define dbgmsg_xmalloc(arg...)  {}
#endif


#ifdef  __RUNTIME_MEMORY_DEBUG_XFREE
 #define dbgmsg_xfree(arg...)  { fprintf(stderr,##arg); fflush(stderr);}
#else
#define dbgmsg_xfree(arg...)  {}
#endif

#ifdef  __RUNTIME_MEMORY_DEBUG__XDEC
 #define dbgmsg__xdec(arg...)  { fprintf(stderr,##arg); fflush(stderr);}
 #define dbgmsg_xdec_free(arg...)  { fprintf(stderr,##arg); fflush(stderr);} 
#else
	#define dbgmsg__xdec(arg...)  {}
	#ifdef __RUNTIME_MEMORY_DEBUG_XDEC_FREE
		 #define dbgmsg_xdec_free(arg...)  { fprintf(stderr,##arg); fflush(stderr);} 
	#else
		#define dbgmsg_xdec_free(arg...)  {}
	#endif
#endif

#ifdef __RUNTIME_MEMORY_DEBUG_NEW_XREGION
 #define dbgmsg_new_xregion(arg...)  { fprintf(stderr,##arg); fflush(stderr);}
#else
	#define dbgmsg_new_xregion(arg...)  {}
#endif

#ifdef __RUNTIME_MEMORY_DEBUG_ADD_SUBREGION
 #define dbgmsg_add_subregion(arg...)  { fprintf(stderr,##arg); fflush(stderr);} 
#else
 #define dbgmsg_add_subregion(arg...)  {} 
#endif

#ifdef  __RUNTIME_MEMORY_DEBUG 
 #define dbgmsg(arg...)  { fprintf(stderr,##arg); fflush(stderr);} 
#else
 #define dbgmsg(arg...)  {} 
#endif

#ifndef CYC_REGION_PROFILE
// defined in cyc_include.h when profiling turned on
struct _RegionPage {
#ifdef CYC_REGION_PROFILE
  unsigned total_bytes;
  unsigned free_bytes;
#endif
  struct _RegionPage *next;
  char data[1];  /*FJS: used to be size 0, but that's forbidden in ansi c*/
};
#endif

#ifdef CYC_REGION_PROFILE
static FILE *alloc_log = NULL;
extern unsigned int GC_gc_no;
#endif
extern size_t GC_get_heap_size();
extern size_t GC_get_free_bytes();
extern size_t GC_get_total_bytes();


static int region_get_heap_size(struct _RegionHandle *r);
static int region_get_free_bytes(struct _RegionHandle *r);
static int region_get_total_bytes(struct _RegionHandle *r);

// FIX: I'm putting GC_calloc and GC_calloc_atomic in here as just
// calls to GC_malloc and GC_malloc_atomic respectively.  This will
// *not* work for the nogc option.
void *GC_calloc(unsigned int n, unsigned int t) {
  return (void *)GC_malloc(n*t);
}

void *GC_calloc_atomic(unsigned int n, unsigned int t) {
  unsigned int p = n*t;
  // the collector does not zero things when you call malloc atomic...
  void *res = GC_malloc_atomic(p);
  if (res == NULL) {
    errprintf("GC_calloc_atomic failure");
    _throw_badalloc();
  }
  memset(res,0,p);
  return res;
}

void *_bounded_GC_malloc(int n, const char*file, int lineno){
  void * res = NULL;
  if((unsigned)n >= MAX_ALLOC_SIZE){
    errprintf( "malloc size ( = %d ) is too big or negative\n",n);
    _throw_badalloc_fn(file, lineno);
  }else{
    res = GC_malloc(n) ;
    if (res == NULL){
      _throw_badalloc_fn(file, lineno);
    }
  }
  return res;
}
void *_bounded_GC_malloc_atomic(int n, const char*file, int lineno){
  void * res = NULL;
  if((unsigned)n >= MAX_ALLOC_SIZE){
    errprintf( "malloc size ( = %d ) is too big or negative\n",n);
    _throw_badalloc_fn(file, lineno);
  }else{
    res = GC_malloc_atomic(n);
    if( res == NULL){
      _throw_badalloc_fn(file,lineno);
    }
  }
  return res;
}
void *_bounded_GC_calloc(unsigned n, unsigned s, const char*file, int lineno){
  unsigned int p = n*s;
  void * res = NULL;
  if(p >= MAX_ALLOC_SIZE){
    errprintf( "calloc size ( = %d ) is too big or negative\n",p);
    _throw_badalloc_fn(file, lineno);
  }else{
    res =GC_calloc(n,s);
    if (res == NULL){
      _throw_badalloc_fn(file, lineno);
    }
  }
  return res;
} 
void *_bounded_GC_calloc_atomic(unsigned n,unsigned s,const char*file,int lineno){
  unsigned int p = n*s;
  void * res = NULL;
  if(p >= MAX_ALLOC_SIZE){
    errprintf( "calloc size ( = %d ) is too big or negative\n",p);
    _throw_badalloc_fn(file, lineno);
  }else{
    res = GC_calloc(n,s);
    if (res == NULL){
      _throw_badalloc_fn(file,lineno);
    }
  }
  return res;
}

#ifdef CYC_REGION_PROFILE
void _profile_free_region_cleanup(struct _RuntimeStack *handler) {
  _profile_free_region(cast_stack_rgn(handler),NULL,NULL,0);
}
#endif

void __free_region( struct _RuntimeStack * rts )
{
// 	dbgmsg("\nPOPING region : %p code = %p" , rts);
  _free_region(cast_stack_rgn(rts));
}

void _push_region(struct _RegionHandle * r) {
  //dbgmsg("pushing region %x\n",(unsigned int)r);  
  r->s.tag = 1;
#ifdef CYC_REGION_PROFILE
  r->s.cleanup = _profile_free_region_cleanup;
#else
  r->s.cleanup = (void (*)(struct _RuntimeStack *))__free_region;
#endif
//  dbgmsg("\nPUSHING region : %p code = %p real code = %p diff = %d diff2 = %d" ,  cast_rgn_stack(r), 	  	cast_rgn_stack(r)->cleanup, __free_region, (int)(cast_rgn_stack(r)) - (int)r, (int) &(r->s) - (int) r );
  _push_frame(cast_rgn_stack(r));
}

void _pop_region() {
  struct _RuntimeStack *f = _top_frame();
  if (f == NULL || f->tag != 1) 
    errquit("internal error: _pop_region");
  _npop_frame(0);
}

#ifdef CYC_REGION_PROFILE
int rgn_total_bytes = 0;
static int rgn_freed_bytes = 0;
static int heap_total_bytes = 0;
static int heap_total_atomic_bytes = 0;
static int unique_total_bytes = 0;
static int unique_freed_bytes = 0;
static int refcnt_total_bytes = 0;
static int refcnt_freed_bytes = 0;
#endif


// exported in core.h
#define CYC_CORE_HEAP_REGION (NULL)
#define CYC_CORE_UNIQUE_REGION ((struct _RegionHandle *)1)
#define CYC_CORE_REFCNT_REGION ((struct _RegionHandle *)2)
struct _RegionHandle *Cyc_Core_heap_region = CYC_CORE_HEAP_REGION;
struct _RegionHandle *Cyc_Core_unique_region = CYC_CORE_UNIQUE_REGION;
struct _RegionHandle *Cyc_Core_refcnt_region = CYC_CORE_REFCNT_REGION;

/////// UNIQUE REGION //////////

// for freeing unique pointers; might want to make this "free"
// eventually (but currently this is set to a no-op in libc.cys).
// Note that this is not recursive; it assumes that programmer
// has freed nested pointers (otherwise will be grabbed by the GC).
void Cyc_Core_ufree(unsigned char *ptr) {
  if (ptr == NULL) return;
  else {
#ifdef CYC_REGION_PROFILE
    unsigned int sz = GC_size(ptr);
    unique_freed_bytes += sz;
    // output special "alloc" event here, where we have a negative size
    if (alloc_log != NULL) {
      fprintf(alloc_log,"%u @\tunique\talloc\t-%d\t%d\t%d\t%d\t%x\n",
              clock(),
	      sz,
	      region_get_heap_size(CYC_CORE_UNIQUE_REGION), 
	      region_get_free_bytes(CYC_CORE_UNIQUE_REGION),
	      region_get_total_bytes(CYC_CORE_UNIQUE_REGION),
              (unsigned int)ptr);
    }
    // FIX:  JGM -- I moved this before the endif -- it was after,
    // but I'm pretty sure we don't need this unless we're profiling.
    GC_register_finalizer_no_order(ptr,NULL,NULL,NULL,NULL);
#endif
    GC_free(ptr);
  }
}

/////// REFERENCE-COUNTED REGION //////////

/* XXX need to make this 2 words in advance, for double-word alignment? */
static int *get_refcnt(unsigned char *ptr) {
  if (ptr == NULL) return NULL;
  else {
    int *res = (int *)(ptr - sizeof(int));
/*     errprintf("getting count, refptr=%x, cnt=%x\n",ptr.base,res); */
    return res;
  }
}

// Assumes no pointer arithmetic on reference-counted pointers, so
// that this is always pointing to the base of the pointer.
int Cyc_Core_refptr_count(unsigned char *ptr) {
  int *cnt = get_refcnt(ptr);
  if (cnt != NULL) return *cnt;
  else return 0;
}

// Need to use a fat pointer here, so that we preserve the bounds
// information when aliasing.
struct _fat_ptr Cyc_Core_alias_refptr(struct _fat_ptr ptr) {
  int *cnt = get_refcnt(ptr.base);
  if (cnt != NULL) *cnt = *cnt + 1;
/*   errprintf("refptr=%x, cnt=%x, updated *cnt=%d\n",ptr.base,cnt, */
/* 	  cnt != NULL ? *cnt : 0); */
  return ptr;
}

void Cyc_Core_drop_refptr(unsigned char *ptr) {
  int *cnt = get_refcnt(ptr);
  if (cnt != NULL) {
/*     errprintf("refptr=%x, cnt=%x, *cnt=%d\n",ptr.base,cnt,*cnt); */
    *cnt = *cnt - 1;
    if (*cnt == 0) { // no more references
/*       errprintf("freeing refptr=%x\n",ptr.base); */
#ifdef CYC_REGION_PROFILE
      unsigned int sz = GC_size(ptr - sizeof(int));
      refcnt_freed_bytes += sz;
      if (alloc_log != NULL) {
	fprintf(alloc_log,"%u @\trefcnt\talloc\t-%d\t%d\t%d\t%d\t%x\n",
                clock(),
		sz,
		region_get_heap_size(CYC_CORE_REFCNT_REGION), 
		region_get_free_bytes(CYC_CORE_REFCNT_REGION),
		region_get_total_bytes(CYC_CORE_REFCNT_REGION),
                (unsigned int)ptr);
      }
#endif
      GC_free(ptr - sizeof(int));
    }
  }
}

/////////////////////////////////////////////////////////////////////
#ifdef CYC_REGION_PROFILE
static int once_fopen = 0;
#endif

void _init_regions() {
#ifdef CYC_REGION_PROFILE
  if( !once_fopen ){
	  alloc_log = fopen("amon.out","w");
	  once_fopen = 1;
  }
#endif
}

void _fini_regions() {
#ifdef CYC_REGION_PROFILE
  fprintf(stderr,"rgn_total_bytes: %d\n",rgn_total_bytes);
  fprintf(stderr,"rgn_freed_bytes: %d\n",rgn_freed_bytes);
  fprintf(stderr,"heap_total_bytes: %d\n",heap_total_bytes);
  fprintf(stderr,"heap_total_atomic_bytes: %d\n",heap_total_atomic_bytes);
  fprintf(stderr,"unique_total_bytes: %d\n",unique_total_bytes);
  fprintf(stderr,"unique_free_bytes: %d\n",unique_freed_bytes);
  fprintf(stderr,"refcnt_total_bytes: %d\n",refcnt_total_bytes);
  fprintf(stderr,"refcnt_free_bytes: %d\n",refcnt_freed_bytes);
  fprintf(stderr,"number of collections: %d\n",GC_gc_no);
  if (alloc_log != NULL)
    fclose(alloc_log);
#endif
}

// minimum page size for a region
#define CYC_MIN_REGION_PAGE_SIZE 480

// controls the default page size for a region
static size_t default_region_page_size = CYC_MIN_REGION_PAGE_SIZE;

// set the default region page size -- returns false and has no effect 
// if s is not at least CYC_MIN_REGION_PAGE_SIZE
bool Cyc_set_default_region_page_size(size_t s) {
  if (s < CYC_MIN_REGION_PAGE_SIZE) 
    return 0;
  default_region_page_size = s;
  return 1;
}

#ifdef __RUNTIME_REGION_DISABLE_GC
  #define region_calloc(x,y,z,w) calloc(x,y)
  #define region_malloc(x,y,z) malloc(x)
  #define region_free(x)   free(x)
#else
  #define region_calloc(x,y,z,w) _bounded_GC_calloc(x,y,z,w)
  #define region_malloc(x,y,z)   _bounded_GC_malloc(x,y,z);
  #define region_free(x)   GC_free(x)
#endif

static void grow_region(struct _RegionHandle *r, unsigned int s) {
  struct _RegionPage *p;
  unsigned int prev_size, next_size;
  prev_size = r->last_plus_one - ((char *)r->curr + sizeof(struct _RegionPage));
  next_size = prev_size * 2;

  if (next_size < s) 
    next_size = s + default_region_page_size;

  // Note, we call calloc here to ensure we get zero-filled pages
  p = region_calloc(sizeof(struct _RegionPage) + next_size,1,__FILE__, __LINE__);
  if (!p) {
    errprintf("grow_region failure");
    _throw_badalloc();
  }
  p->next = r->curr;
#ifdef CYC_REGION_PROFILE
  p->total_bytes = GC_size(p);
  p->free_bytes = next_size;
  if (alloc_log != NULL) {
    fprintf(alloc_log,"%u @\t%s\tresize\t%d\t%d\t%d\t%d\n",
            clock(),
	    r->name,
	    GC_size(p),
	    GC_get_heap_size(), GC_get_free_bytes(), GC_get_total_bytes());
  }
#else
  r->used_bytes += next_size;
  r->wasted_bytes += r->last_plus_one - r->offset;
#endif
  r->curr = p;
  r->offset = ((char *)p) + sizeof(struct _RegionPage);
  r->last_plus_one = r->offset + next_size;
}

// better not get called with the heap or unique regions ...
static void _get_first_region_page(struct _RegionHandle *r, unsigned int s) {
  struct _RegionPage *p;
  unsigned int page_size = 
    default_region_page_size < s ? s : default_region_page_size;
  p = (struct _RegionPage *) region_calloc(sizeof(struct _RegionPage) + 
                                      page_size,1, __FILE__, __LINE__);
  if (p == NULL) 
    _throw_badalloc();
#ifdef CYC_REGION_PROFILE
  p->total_bytes = GC_size(p);
  p->free_bytes = page_size;
  if (alloc_log != NULL) {
    fprintf(alloc_log,"%u @\t%s\tresize\t%d\t%d\t%d\t%d\n",
            clock(),
	    r->name,
	    GC_size(p),
	    GC_get_heap_size(), GC_get_free_bytes(), GC_get_total_bytes());
  }
#endif
  p->next = NULL;
  r->curr = p;
  r->offset = ((char *)p) + sizeof(struct _RegionPage);
  r->last_plus_one = r->offset + page_size;
#ifndef CYC_REGION_PROFILE
  r->used_bytes = page_size;
  r->wasted_bytes = 0;
#endif
}

// allocate s bytes within region r
void * _region_malloc(struct _RegionHandle *r, unsigned int s) {
  char *result;
  if (r > (struct _RegionHandle *)_CYC_MAX_REGION_CONST) {
	  if( r->is_xrgn )
		 return _xregion_malloc((struct _XRegionHandle *)r,s);
#ifndef CYC_NOALIGN
    // round s up to the nearest _CYC_MIN_ALIGNMENT value
    s =  (s + _CYC_MIN_ALIGNMENT - 1) & (~(_CYC_MIN_ALIGNMENT - 1));
#endif
    // if no page yet, then fetch one
    if (r->curr == 0) {
      _get_first_region_page(r,s);
      result = r->offset;
    } else {
      result = r->offset;
      // else check for space on the current page
      if (s > (r->last_plus_one - result)) {
        grow_region(r,s);
        result = r->offset;
      }
    }
    r->offset = result + s;
#ifdef CYC_REGION_PROFILE
    r->curr->free_bytes = r->curr->free_bytes - s;
    rgn_total_bytes += s;
#endif
    return (void *)result;
  } else if (r != CYC_CORE_REFCNT_REGION) {
    result = _bounded_GC_malloc(s,__FILE__, __LINE__);
    if(!result)
      _throw_badalloc();
#ifdef CYC_REGION_PROFILE
    {
      unsigned int actual_size = GC_size(result);
      if (r == CYC_CORE_HEAP_REGION)
	heap_total_bytes += actual_size;
      else
	unique_total_bytes += actual_size;
    }
#endif
    return (void *)result;
  }
  else { // (r == CYC_CORE_REFCNT_REGION)
    // need to add a word for the reference count.  We use a word to
    // keep the resulting memory word-aligned.  Then bump the pointer.
    // FIX: probably need to keep it double-word aligned!
    result = _bounded_GC_malloc(s+sizeof(int),__FILE__, __LINE__);
    if(!result)
      _throw_badalloc();
    *(int *)result = 1;
#ifdef CYC_REGION_PROFILE
    refcnt_total_bytes += GC_size(result);
#endif
    result += sizeof(int);
/*     errprintf("alloc refptr=%x\n",result); */
    return (void *)result;
  }
}

// allocate s bytes within region r
void * _region_calloc(struct _RegionHandle *r, unsigned int n, unsigned int t) 
{
  unsigned int s = n*t;
  char *result;
  if (r > (struct _RegionHandle *)_CYC_MAX_REGION_CONST) {
		 if( r->is_xrgn )
			 return _xregion_calloc((struct _XRegionHandle *)r,s);
    // round s up to the nearest _CYC_MIN_ALIGNMENT value
#ifndef CYC_NOALIGN
    s =  (s + _CYC_MIN_ALIGNMENT - 1) & (~(_CYC_MIN_ALIGNMENT - 1));
#endif
    // if no page yet, then fetch one
    if (r->curr == 0) {
      _get_first_region_page(r,s);
      result = r->offset;
    } else {
      result = r->offset;
      // else check for space on the current page
      if (s > (r->last_plus_one - result)) {
        grow_region(r,s);
        result = r->offset;
      }
    }
    r->offset = result + s;
#ifdef CYC_REGION_PROFILE
    r->curr->free_bytes = r->curr->free_bytes - s;
    rgn_total_bytes += s;
#endif
    return (void *)result;
  } else if (r != CYC_CORE_REFCNT_REGION) {
    // allocate in the heap
    result = _bounded_GC_calloc(n,t,__FILE__, __LINE__);
    if(!result)
      _throw_badalloc();
#ifdef CYC_REGION_PROFILE
    {
      unsigned int actual_size = GC_size(result);
      if (r == Cyc_Core_heap_region)
	heap_total_bytes += actual_size;
      else 
	unique_total_bytes += actual_size;
    }
#endif
    return result;
  } else { // r == CYC_CORE_REFCNT_REGION)
    // allocate in the heap + 1 word for the refcount
    result = _bounded_GC_calloc(n*t+sizeof(int),1,__FILE__, __LINE__);
    if(!result)
      _throw_badalloc();
    *(int *)result = 1;
#ifdef CYC_REGION_PROFILE
    refcnt_total_bytes += GC_size(result);
#endif
    result += sizeof(int);
    return result;
  }
}

// allocate a new page and return a region handle for a new region.
struct _RegionHandle _new_region(const char *rgn_name) {
  struct _RegionHandle r;

  /* we're now lazy about allocating a region page */
  //struct _RegionPage *p;

  // we use calloc to make sure the memory is zero'd
  //p = (struct _RegionPage *)GC_calloc(sizeof(struct _RegionPage) + 
  //                                  default_region_page_size,1);
  //if (p == NULL) 
  //  _throw_badalloc();
  //p->next = NULL;
#ifdef CYC_REGION_PROFILE
  //p->total_bytes = sizeof(struct _RegionPage) + default_region_page_size;
  //p->free_bytes = default_region_page_size;
  r.name = rgn_name;
#else
  r.used_bytes = 0;
  r.wasted_bytes = 0;
#endif
  r.s.tag = 1;
  r.s.next = NULL;
  r.is_xrgn = 0;
  r.curr = 0;
  r.offset = 0;
  r.last_plus_one = 0;
  //r.offset = ((char *)p) + sizeof(struct _RegionPage);
  //r.last_plus_one = r.offset + default_region_page_size;
  return r;
}

// free all the resources associated with a region (except the handle)
//   (assumes r is not heap or unique region)
void _free_region(struct _RegionHandle *r) {
  struct _RegionPage *p;

  if( r->is_xrgn )
    errquit("internal error: _free_region: attempting to free a xrgn");

  /* free pages */
  p = r->curr;
  while (p != NULL) {
    struct _RegionPage *n = p->next;
#ifdef CYC_REGION_PROFILE
    unsigned int sz = GC_size(p);
    GC_free(p);
    rgn_freed_bytes += sz;
    if (alloc_log != NULL) {
      fprintf(alloc_log,"%u @\t%s\tresize\t-%d\t%d\t%d\t%d\n",
              clock(),
	      r->name, sz,
	      GC_get_heap_size(), GC_get_free_bytes(), GC_get_total_bytes());
    }
#else
    region_free(p);
#endif
    p = n;
  }
  r->curr = 0;
  r->offset = 0;
  r->last_plus_one = 0;
}

// Dynamic Regions
// Note that struct Cyc_Core_DynamicRegion is defined in cyc_include.h.

// We use this struct when returning a newly created dynamic region.
// The wrapper is needed because the Cyclone interface uses an existential.
struct Cyc_Core_NewDynamicRegion {
  struct Cyc_Core_DynamicRegion *key;
};

// Create a new dynamic region and return a unique pointer for the key.
struct Cyc_Core_NewDynamicRegion Cyc_Core__new_ukey(const char *file,
						    const char *func,
						    int lineno) {
  struct Cyc_Core_NewDynamicRegion res;
  res.key = _bounded_GC_malloc(sizeof(struct Cyc_Core_DynamicRegion),__FILE__,
			       __LINE__);
  if (!res.key)
    _throw_badalloc();
#ifdef CYC_REGION_PROFILE
  res.key->h = _profile_new_region("dynamic_unique",file,func,lineno);
#else
  res.key->h = _new_region("dynamic_unique");
#endif
  return res;
}

// XXX change to use refcount routines above
// Create a new dynamic region and return a reference-counted pointer 
// for the key.
struct Cyc_Core_NewDynamicRegion Cyc_Core__new_rckey(const char *file,
						     const char *func,
						     int lineno) {
  struct Cyc_Core_NewDynamicRegion res;
  int *krc = _bounded_GC_malloc(sizeof(int)+sizeof(struct Cyc_Core_DynamicRegion),
				__FILE__, __LINE__);
  //errprintf("creating rckey.  Initial address is %x\n",krc);fflush(stderr);
  if (!krc)
    _throw_badalloc();
  *krc = 1;
  res.key = (struct Cyc_Core_DynamicRegion *)(krc + 1);
  //errprintf("results key address is %x\n",res.key);fflush(stderr);
#ifdef CYC_REGION_PROFILE
  res.key->h = _profile_new_region("dynamic_refcnt",file,func,lineno);
#else
  res.key->h = _new_region("dynamic_refcnt");
#endif
  return res;
}

// Destroy a dynamic region, given the unique key to it.
void Cyc_Core_free_ukey(struct Cyc_Core_DynamicRegion *k) {
#ifdef CYC_REGION_PROFILE
  _profile_free_region(&k->h,NULL,NULL,0);
#else
  _free_region(&k->h);
#endif
  GC_free(k);
}

// Drop a reference for a dynamic region, possibly freeing it.
void Cyc_Core_free_rckey(struct Cyc_Core_DynamicRegion *k) {
  //errprintf("freeing rckey %x\n",k);
  int *p = ((int *)k) - 1;
  //errprintf("count is address %x, value %d\n",p,*p);
  unsigned c = (*p) - 1;
  if (c >= *p) {
    errquit("internal error: free rckey bad count");
  }
  *p = c;
  if (c == 0) {
    //errprintf("count at zero, freeing region\n");
#ifdef CYC_REGION_PROFILE
    _profile_free_region(&k->h,NULL,NULL,0);
#else
    _free_region(&k->h);
#endif
    //errprintf("freeing ref-counted pointer\n");
    GC_free(p);
  }
}

// Open a key (unique or reference-counted), extract the handle
// for the dynamic region, and pass it along with env to the
// body function pointer, returning the result.
void *Cyc_Core_open_region(struct Cyc_Core_DynamicRegion *k,
                           void *env,
                           void *body(struct _RegionHandle *h,
                                      void *env)) {
  return body(&k->h,env);
}

#ifdef CYC_REGION_PROFILE

// called before each profiled allocation to see if a GC has occurred
// since the last allocation, and if so makes a log entry for it based
// on the amount of freed data.
static void _profile_check_gc() {
  static unsigned int last_GC_no = 0;
  if (GC_gc_no != last_GC_no) {
    last_GC_no = GC_gc_no;
    if (alloc_log != NULL) {
      fprintf(alloc_log,"%u gc-%d\theap\tgc\t%d\t%d\t%d\n",
              clock(),
	      last_GC_no, GC_get_heap_size(),
	      GC_get_free_bytes(), GC_get_total_bytes());
    }
  }
}

static int region_get_heap_size(struct _RegionHandle *r) {
  if (r > (struct _RegionHandle *)_CYC_MAX_REGION_CONST) {
	 if( r->is_xrgn )
		 return 0;
    struct _RegionPage *p = r->curr;
    int sz = 0;
    while (p != NULL) {
      sz += p->total_bytes;
      p = p->next;
    }  
    return sz;
  }
  else
    return GC_get_heap_size();
}

/* Two choices: print the "effective" free bytes, which are just
   the ones in the current page, or print the non-allocated bytes,
   which are the free bytes in all the pages.  Doing the former. */
static int region_get_free_bytes(struct _RegionHandle *r) {
  if (r > (struct _RegionHandle *)_CYC_MAX_REGION_CONST) {
	 if( r->is_xrgn )
		 return 0;

    struct _RegionPage *p = r->curr;
    if (p != NULL)
      return p->free_bytes;
    else 
      return 0;
  }
  else
    return GC_get_free_bytes();
}

static int region_get_total_bytes(struct _RegionHandle *r) {
  if (r > (struct _RegionHandle *)_CYC_MAX_REGION_CONST) {
	 if( r->is_xrgn )
		 return 0;

    struct _RegionPage *p = r->curr;
    int sz = 0;
    while (p != NULL) {
      sz = sz + (p->total_bytes - p->free_bytes);
      p = p->next;
    }  
    return sz;
  }
  else {
    unsigned int unique_avail_bytes =
      unique_total_bytes - unique_freed_bytes;
    unsigned int refcnt_avail_bytes =
      refcnt_total_bytes - refcnt_freed_bytes;
    if (r == CYC_CORE_UNIQUE_REGION) return unique_avail_bytes;
    else if (r == CYC_CORE_REFCNT_REGION) return refcnt_avail_bytes;
    else return GC_get_total_bytes() - unique_avail_bytes - refcnt_avail_bytes;
  }
}

struct _RegionHandle _profile_new_region(const char *rgn_name, 
					 const char *file,
                                         const char *func,
                                         int lineno) {
  int len;
  char *buf;
  static int cnt = 0;

  len = strlen(rgn_name)+10;
  buf = _bounded_GC_malloc_atomic(len, __FILE__, __LINE__);
  if(!buf)
    _throw_badalloc();
  else
    snprintf(buf,len,"%d-%s",cnt++,rgn_name);

  if (alloc_log != NULL) {
    fprintf(alloc_log,"%u %s:%s:%d\t%s\tcreate\t%d\t%d\t%d\n",
            clock(),
            file,func,
	    lineno,buf,
	    GC_get_heap_size(),GC_get_free_bytes(),GC_get_total_bytes());
  }

  return _new_region(buf);
}

void _profile_free_region(struct _RegionHandle *r, const char *file, const char *func, int lineno) {
  // should never be heap, unique, or refcnt ...
  _free_region(r);
  if (alloc_log != NULL) {
    if (file == NULL)
      fprintf(alloc_log,"%u @\t%s\tfree\t%d\t%d\t%d\n",
              clock(),
              (r == CYC_CORE_HEAP_REGION ? "heap" :
               (r == CYC_CORE_UNIQUE_REGION ? "unique" :
                (r == CYC_CORE_REFCNT_REGION ? "refcnt" : r->name))),
              GC_get_heap_size(),GC_get_free_bytes(),GC_get_total_bytes());
    else
      fprintf(alloc_log,"%u %s:%s:%d\t%s\tfree\t%d\t%d\t%d\n",
              clock(),
              file,func,lineno,
              (r == CYC_CORE_HEAP_REGION ? "heap" :
               (r == CYC_CORE_UNIQUE_REGION ? "unique" :
                (r == CYC_CORE_REFCNT_REGION ? "refcnt" : r->name))),
              GC_get_heap_size(),GC_get_free_bytes(),GC_get_total_bytes());
  }
}

typedef void * GC_PTR; /* Taken from gc/include/gc.h, must be kept in sync. */

static void reclaim_finalizer(GC_PTR obj, GC_PTR client_data) {
  if (alloc_log != NULL)
    fprintf(alloc_log,
            "%u @ @ reclaim \t%x\n",
            clock(),
            (unsigned int)obj);
}

static void set_finalizer(GC_PTR addr) {
  GC_register_finalizer_no_order(addr,reclaim_finalizer,NULL,NULL,NULL);
}

void * _profile_region_malloc(struct _RegionHandle *r, unsigned int s,
                              const char *file, const char *func, int lineno) {
  char *addr;
  addr = _region_malloc(r,s);
  _profile_check_gc();
  if (alloc_log != NULL) {
    if (r == CYC_CORE_HEAP_REGION) {
      s = GC_size(addr);
      set_finalizer((GC_PTR)addr);
    }
    if (r == CYC_CORE_UNIQUE_REGION) {
      s = GC_size(addr);
      set_finalizer((GC_PTR)addr);
    }
    else if (r == CYC_CORE_REFCNT_REGION)
      s = GC_size(addr-sizeof(int)); // back up to before the refcnt
    fprintf(alloc_log,"%u %s:%s:%d\t%s\talloc\t%d\t%d\t%d\t%d\t%x\n",
            clock(),
            file,func,lineno,
	    (r == CYC_CORE_HEAP_REGION ? "heap" :
	     (r == CYC_CORE_UNIQUE_REGION ? "unique" :
	      (r == CYC_CORE_REFCNT_REGION ? "refcnt" : r->name))), s,
	    region_get_heap_size(r), 
	    region_get_free_bytes(r),
	    region_get_total_bytes(r),
            (unsigned int)addr);
  }
  return (void *)addr;
}

void * _profile_region_calloc(struct _RegionHandle *r, unsigned int t1,
                              unsigned int t2,
                              const char *file, const char *func, int lineno) {
  char *addr;
  unsigned s = t1 * t2;
  addr = _region_calloc(r,t1,t2);
  _profile_check_gc();
  if (alloc_log != NULL) {
    if (r == CYC_CORE_HEAP_REGION) {
      s = GC_size(addr);
      set_finalizer((GC_PTR)addr);
    }
    if (r == CYC_CORE_UNIQUE_REGION) {
      s = GC_size(addr);
      set_finalizer((GC_PTR)addr);
    }
    else if (r == CYC_CORE_REFCNT_REGION)
      s = GC_size(addr-sizeof(int)); // back up to before the refcnt
    fprintf(alloc_log,"%u %s:%s:%d\t%s\talloc\t%d\t%d\t%d\t%d\t%x\n",
            clock(),
            file,func,lineno,
	    (r == CYC_CORE_HEAP_REGION ? "heap" :
	     (r == CYC_CORE_UNIQUE_REGION ? "unique" :
	      (r == CYC_CORE_REFCNT_REGION ? "refcnt" : r->name))), s,
	    region_get_heap_size(r), 
	    region_get_free_bytes(r),
	    region_get_total_bytes(r),
            (unsigned int)addr);
  }
  return (void *)addr;
}

static void *_do_profile(void *result, int is_atomic,
			 const char *file, const char *func, int lineno) {  
  int n;
  set_finalizer((GC_PTR)result);
  _profile_check_gc();
  n = GC_size(result);
  heap_total_bytes += n;
  if (is_atomic) heap_total_atomic_bytes += n;
  if (alloc_log != NULL) {
    fprintf(alloc_log,"%u %s:%s:%d\theap\talloc\t%d\t%d\t%d\t%d\t%x\n",
            clock(),
            file,func,lineno,n,
	    GC_get_heap_size(),GC_get_free_bytes(),GC_get_total_bytes(),
            (unsigned int)result);
  }
  return result;
}

void * _profile_GC_malloc(int n, const char *file, const char *func, int lineno) {
  void * result;
  result =  _bounded_GC_malloc(n,__FILE__, __LINE__);
  if(!result)
    _throw_badalloc();
  return _do_profile(result,0,file,func,lineno);
}

void * _profile_GC_malloc_atomic(int n, const char *file, const char *func,
                                 int lineno) {
  void * result;
  result =  _bounded_GC_malloc_atomic(n, file, lineno);
  if(!result)
    _throw_badalloc();
  return _do_profile(result,1,file,func,lineno);
}

void * _profile_GC_calloc(unsigned n, unsigned s, const char *file, const char *func, int lineno) {
  void * result;
  result =  _bounded_GC_calloc(n,s,__FILE__, __LINE__);
  if(!result)
    _throw_badalloc();
  return _do_profile(result,0,file,func,lineno);
}

void * _profile_GC_calloc_atomic(unsigned n, unsigned s, 
				 const char *file, const char *func, int lineno) {
  void * result;
  result =  _bounded_GC_calloc_atomic(n,s, __FILE__, __LINE__);
  if(!result)
    _throw_badalloc();
  return _do_profile(result,1,file,func,lineno);
}

#else

static int region_get_heap_size(struct _RegionHandle *r) {
  if (r > (struct _RegionHandle *)_CYC_MAX_REGION_CONST) {
	 if( r->is_xrgn )
		 return 0;
    unsigned used_bytes = r->used_bytes;
    return used_bytes;
  }
  else
    return GC_get_heap_size();
}

static int region_get_free_bytes(struct _RegionHandle *r) {
  if (r > (struct _RegionHandle *)_CYC_MAX_REGION_CONST) {
	  if( r->is_xrgn )
		 return 0;
    unsigned free_bytes = r->last_plus_one - r->offset;
    return free_bytes;
  }
  else
    return GC_get_free_bytes();
}

static int region_get_total_bytes(struct _RegionHandle *r) {
  if (r > (struct _RegionHandle *)_CYC_MAX_REGION_CONST) {
	  if( r->is_xrgn )
		 return 0;
    unsigned used_bytes = r->used_bytes;
    unsigned wasted_bytes = r->wasted_bytes;
    unsigned free_bytes = r->last_plus_one - r->offset;
    return (used_bytes - wasted_bytes - free_bytes);
  }
  else {
    return GC_get_total_bytes();
  }
}

#endif

int region_used_bytes(struct _RegionHandle *r) {
  return region_get_heap_size(r);
}
int region_free_bytes(struct _RegionHandle *r) {
  return region_get_free_bytes(r);
}
int region_alloc_bytes(struct _RegionHandle *r) {
  return region_get_total_bytes(r);
}

// Called from gc/alloc.c for allocation profiling.  Must be
// defined even if CYC_REGION_PROFILE is not.
void CYCALLOCPROFILE_GC_add_to_heap(void *p,unsigned long bytes) {
#ifdef CYC_REGION_PROFILE
  if (alloc_log != NULL) {
    fprintf(alloc_log,"%u @\theap\tgc_add_to_heap\t%x\t%u\t%d\t%d\t%d\n",
            clock(),
            (unsigned int)p,bytes,
	    GC_get_heap_size(),GC_get_free_bytes(),GC_get_total_bytes());
  }
#endif
}
void CYCALLOCPROFILE_mark(const char *s) {
#ifdef CYC_REGION_PROFILE
  if (alloc_log != NULL)
    fprintf(alloc_log,"%u @\t@\tmark\t%s\n", clock(), s);
#endif
}

/******* for turning off gc warnings about blacklisted blocks *******/
/* These type/macro defns are taken from gc/include/gc.h and must
   be kept in sync. */
#   define GC_PROTO(args) args
typedef unsigned long GC_word;
typedef void (*GC_warn_proc) GC_PROTO((char *msg, GC_word arg));

extern void GC_default_warn_proc(char *msg, GC_word arg);
extern GC_warn_proc GC_set_warn_proc(GC_warn_proc p);

static void GC_noblacklist_warn_proc(char *msg, GC_word arg) {
  if (!msg) return;
  if (!strncmp(msg,"Needed to allocate blacklisted block",
               strlen("Needed to allocate blacklisted block")))
    return;
  GC_default_warn_proc(msg,arg);
}

/* Use these in Cyclone programs */
void GC_blacklist_warn_set() {
  GC_set_warn_proc(GC_default_warn_proc);
}
void GC_blacklist_warn_clear() {
  GC_set_warn_proc(GC_noblacklist_warn_proc);
}
//////////////////////////////////////////////////////////////////////
/////////     X-Region Stuff         /////////////////////////////////
/////////////////////////////////////////////////////////////////////
#ifdef __RUNTIME_XREGION_DISABLE_GC
  #define xregion_calloc(x,y,z,w) calloc(x,y)
  #define xregion_malloc(x) malloc(x)
  #define xregion_free(x)   free(x)
#else
  #define xregion_calloc(x,y,z,w) _bounded_GC_calloc(x,y,z,w)
  #define xregion_malloc(x) GC_malloc(x)
  #define xregion_free(x)   GC_free(x)
#endif

#define CYC_MIN_XREGION_PAGE_SIZE 480

// controls the default page size for a region
static size_t default_xregion_page_size = CYC_MIN_XREGION_PAGE_SIZE;

// set the default region page size -- returns false and has no effect 
// if s is not at least CYC_MIN_XREGION_PAGE_SIZE
bool Cyc_set_default_xregion_page_size(size_t s) {
//  if (s < CYC_MIN_REGION_PAGE_SIZE) 
  //  return 0;
  default_xregion_page_size = s;
  return 1;
}

// allocate s bytes within region r
void * _xregion_calloc(struct _XRegionHandle *r,
                       unsigned int s) {
  return _xregion_malloc(r,s);
}

void Cyc_Core_print_region_pages(struct _XRegionHandle *r ){
 /*   wait_lock(&r->caps.lock);
  struct _RegionPage *p = r->curr;
    fprintf(stderr,"\n[%d] @@PAGES for %p ...(first=%p)",tcb()->tid,r,
			 							p);
  for(;p;p=p->next){
    fprintf(stderr,"\n[%d] @@PAGES HANDLE=%p PAGE=%p",tcb()->tid,r,p);
  }
   unlock(&r->caps.lock);*/
}

//#define NDEBUG  //remove assert code
//#include <assert.h>
//void assert(int b ){   if(!b) errquit("assertion issue"); }
struct _XRegionHandle *_new_xregion(
	struct _XRegionHandle *parent,
	unsigned int idx, //const char *rgn_name
        struct _XTreeHandle *tkey
	)
{	//ret.sub_regions = 0;
   //	ret.parent = (parent == (struct _XRegionHandle *) CYC_CORE_HEAP_REGION)?NULL:parent;
	struct _XRegionHandle *z;
	/* Step 1: initialize global data */
	{ struct _XRegionHandle ret;
	/* Step 1.0: global counts and locks*/
        memset(&ret.caps,0,sizeof(struct _RegionCaps));
        ret.caps.writer_cv = tcb()->tid;
		  ret.caps.lock = 0;
  	  	  ret.caps.rcnt = 1; // single thread uses this region
			/* Step 1.1: initialize region-related stuff*/

	    ret.curr = (struct _RegionPage *)0;
	 	 ret.offset = 0;
	 	 ret.last_plus_one = 0;
	 	 ret.key  = 0;
		/* Step 1.2: initialize global key cache 
		 * (used for thread-local rg)*/
		ret.single_thread_key = tkey;
		ret.is_xrgn = 1;

		ret.mapped = 0;
	/* Step 1.3: heap allocate the global region and initialize it */
		z = (struct _XRegionHandle *) 
            xregion_malloc(sizeof(struct _XRegionHandle));
   	assert((void *)z > (void *)100);
		*z = ret; 
		assert(z->curr == 0);
	   dbgmsg_new_xregion("\n[%X] _new_xregion: idx=%d"
			  					" xregion_global_handle=%p"
                        " parent=%p tree_handle=%p\n",
								tcb()->tid,idx,z,parent,tkey);
	}
	/* Step 2: initialize tkey */
   /* Step 2.1: find parent tkey and register this key to the parent*/
	unsigned short wr_par=0,rd_par=0; // implicit ancestor lock counts 
	if((void *)parent != (void *) NULL ){
	  struct _XTreeHandle *parent_tkey = get_tkey(parent);
	  // insert key to parent key
	  tkey->next_key = parent_tkey->sub_keys;
	  parent_tkey->sub_keys = tkey;
	  tkey->parent_key = parent_tkey;
     wr_par = parent_tkey->caps.wr_cnt;
     rd_par = parent_tkey->caps.rd_cnt;
	} else {
	  tkey->parent_key = NULL;
     tkey->next_key = NULL;
     wr_par = rd_par = 0;
   } 
	tkey->next = NULL; // this key is not inserted to the hashtbl yet
	tkey->sub_keys = NULL;
	/* Step 2.2: initialize counts */
        //inherit counts from parent
	tkey->caps.wr_cnt = wr_par + 1;
	tkey->caps.rd_cnt = rd_par;
        // counts of this thread to the new region
	tkey->caps.rcnt = 1;
	/*Step 2.3:  initialize XRegionHandle*/
	tkey->h = z;


	/* Step 3: insert key in local hashtable*/
	 hinsert(tkey);
	 return z;
}

static void _xregion_free(struct _XRegionHandle *r) {
  if(!r)
    errquit("\n[RUNTIME _xregion_free] r is NULL.");

  struct _RegionPage *p;
/*  for(p=r->curr;p;p=p->next){
    dbgmsg_xfree("\n[%X] @@ _xregion_free: BEFORE "
                 " heap_handle=%p page=%p",tcb()->tid,r,p);
  }*/
//	fprintf(stderr,"\n[%d] _XREGION_FREE" , tcb()->tid);
//	 Cyc_Core_print_region_pages(r);

  //free memory mapped files
  struct Mapped* m = r->mapped;
  for(;m;) {
          m->mfree(m->begin,m->size);
          void *mtmp = m->next;
          free(m);
          m = mtmp;
  }
  //free region pages
  for(p=r->curr;p;){
    dbgmsg_xfree("\n[%X] @@ _xregion_free: "
                 " heap_handle=%p page=%p",tcb()->tid,r,p);
	 void *mem = p;
	 p=p->next;
    xregion_free(mem);
  }
  r->curr = 0;
  r->offset = 0;
  r->last_plus_one = 0;
  dbgmsg_xfree("\n[%X] @@ _xregion_free: heap_handle="
               "%p\n",tcb()->tid,r);
  memset(r,sizeof (struct _XRegionHandle),1);
  xregion_free(r);
}
/*struct _RegionPage
#ifdef CYC_REGION_PROFILE
{ unsigned total_bytes;
  unsigned free_bytes;
  struct _RegionPage *next;
  char data[1];
}
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
*/
volatile int threads = 0;


void attach_region(struct _XRegionHandle *r, char *arg, int  len, void (*fun)(void *,int)) {
     mapped_t *m = malloc(sizeof(struct Mapped));
     m->begin = arg;
     m->size =  len;
     m->mfree = fun;
     void *lock = &r->caps.lock;
     wait_lock(lock);
     m->next = r->mapped;
     r->mapped = m;
     unlock(lock);
}


struct MapRet {
	char *begin;
	unsigned n;
};
#define PROT_READ 1
#define MAP_SHARED 1
#define MAP_FAILED ((void *)-1)

void *mmap(void *addr, unsigned int length, int prot, int flags,
                  int fd, long long offset);

int munmap(void *addr,  unsigned int length);

static void my_free(void *p , int len){
   free(p);
}

struct MapRet Cyc_Core_map_file(struct _XRegionHandle *r, char *arg) {
          long long len = 0;
          char *p = 0;
          int fd;
                         FILE *f = fopen(arg,"r");
                         if(!f) goto out;
                         fd = fileno(f);

                         fseek(f, 0, SEEK_END); // seek to end of file
                         len = ftell(f); // get current file pointer
                         fseek(f, 0, SEEK_SET); // seek back to beginning of file
          p = (char *) malloc((len+1));
          if(fread(p,1,len,f)!=len){
             exit(-1);
          }
          fclose(f); 
          p[len] = '\0';

         attach_region(r,p,len,my_free);
         struct MapRet ret;
out:         
         ret.begin = p;
         ret.n = len;
         return ret;
}

/*
struct MapRet
 Cyc_Core_map_file(struct _XRegionHandle *r, char *arg) {

   long long len;
   char *p;
   int fd;
  struct MapRet ret = {(void *)0x0,0};

   FILE *f = fopen(arg,"r");
   if(!f) return ret;
   fd = fileno(f);

   fseek(f, 0, SEEK_END); // seek to end of file
   len = ftell(f); // get current file pointer
   fseek(f, 0, SEEK_SET); // seek back to beginning of file
   p = mmap (0,(unsigned int)len,PROT_READ, MAP_SHARED, fd, 0);
			 fclose(f); 
   if (p == MAP_FAILED) {
     return ret;
   }

   attach_region(r,p,len,munmap);
   ret.begin = p;
   ret.n = len;
   return ret;
}
 */


// allocate s bytes within xregion r
void * _xregion_malloc(struct _XRegionHandle *r,
	 	  	    	        unsigned int s) {
  //unsigned int shared = r->caps.rcnt;
#ifndef CYC_NOALIGN
    // round s up to the nearest _CYC_MIN_ALIGNMENT value
    s =  (s + _CYC_MIN_ALIGNMENT - 1) & (~(_CYC_MIN_ALIGNMENT - 1));
#endif
//  if(shared)
  void *lock = &r->caps.lock;
  unsigned int rcnt = r->caps.rcnt;

  if( rcnt != 1)
    wait_lock(lock);

//  fprintf(stderr,"\n[%d] GOT LOCK %p.", tcb()->tid,lock);
/*  int tmp_threads =  __sync_fetch_and_add(&threads,1);
  fprintf(stderr,"\n[%d] GOT LOCK %p h=%p",tcb()->tid,lock,r);
  if(tmp_threads != 0)
	  errquit("\n[%d]BUG [%p] !!! shared=%d tmp_threads=%d"
			    " lret=%d lock=%d",
				 tcb()->tid,lock,
			  	 shared,tmp_threads,lret,r->caps.lock);*/

  if(s > (r->last_plus_one - r->offset)) {
    unsigned int next_size = !r->offset ?
                      default_xregion_page_size :
                      ((unsigned int) 
                      ((long long) r->last_plus_one - 
	    	      (long long) (r->curr + 1)))*2;
    if(next_size < s) next_size = s;
    const unsigned int alloc_size = 
      sizeof(struct _RegionPage) + next_size;
    struct _RegionPage *p = (struct _RegionPage *) 
            GC_malloc_atomic(alloc_size);
         //  xregion_calloc(alloc_size,1,__FILE__, __LINE__);
    if(p == NULL) 
      _throw_badalloc();

	 //struct _RegionPage *p0 ;
  	/* for(p0=r->curr;p0;p0=p0->next) {
    dbgmsg_xmalloc("\n[%d] @@ _xregion_malloc: BEFORE "
                 " heap_handle=%p page=%p",tcb()->tid,r,p0);
	  }*/

    p->next = r->curr;
    r->curr = p;
    r->last_plus_one = (r->offset = (char *)(p+1))  + next_size;

	 /*for(p0=r->curr;p0;p0=p0->next) {
    	dbgmsg_xmalloc("\n[%d] @@ _xregion_malloc: AFTER "
      	           " heap_handle=%p page=%p",tcb()->tid,r,p0);
	  }

	 dbgmsg_xmalloc("\n[%d] @@ _xregion_malloc: "
			 			 "Allocated new page %p (next=%p,size=%d,size2=%d",
						 	tcb()->tid,p,next_size,
						  ((long long) r->last_plus_one - 
							(long long) (r->curr + 1))
						);*/
  } 
  void *offset = r->offset;
  r->offset += s;

/*  tmp_threads  = __sync_fetch_and_add(&threads,-1);
  if(tmp_threads != 1)
	  fprintf(stderr,"\n[%d]BUG  2[%p]!!! shared=%d tmp_threads=%d"
			    " lret=%d lock=%d", 
				 tcb()->tid,lock,
			  	 shared,tmp_threads,lret,r->caps.lock);

  fprintf(stderr,"\n[%d] RELEASED LOCK %p h=%p",tcb()->tid,lock,r);*/
  //if(shared)
  //fprintf(stderr,"\n[%d] RELEASED LOCK %p.", tcb()->tid,lock);
  if( rcnt != 1)
    unlock(lock);
  return offset;
}
/*
 *IMPORTANT NOTE: children of the original tree may be in different
 * 	 	  order than the children of the new thread ...
 *	 	  does this matter when it comes to locking trees?
 * 	 	  Yes it does! It may cause a deadlock if the tree
 *		  is locked bottom up !
 *
 *  
 *        LOCKING MUST BE TOP-DOWN OTHERWISE A DEADLOCK MAY OCCUR
 *
 *     struct _LocalCaps {
 *       unsigned short rcnt; // local ref count 
 *       unsigned short wr_cnt; // local write count
 *       unsigned short rd_cnt; // locks acquired
 *     			// by ancestor regions. 
 *    } caps;
 *
 *   struct _RegionCaps  {
 *     unsigned int rcnt; //Region count.One per thread.
 *      unsigned int  lock;  //lock for this region
 *      unsigned int  writer_cv; // condition var for writers
 *      unsigned int  reader_cv; // condition var for readers
 *      unsigned int  writer_q; 
 *      unsigned int  reader_q; 
 *      unsigned int rd_cnt; 
 *   };
 */
//#define __RUNTIME_MEMORY_DEBUG_XLINC
//#define __RUNTIME_MEMORY_DEBUG_XLDEC
//#define __RUNTIME_MEMORY_DEBUG_XLSTEAL
#ifdef __RUNTIME_MEMORY_DEBUG_LOCKOP
 #define dbgmsg_lockop(arg...)  { fprintf(stderr,##arg); fflush(stderr);} 
#else
 #define dbgmsg_lockop(arg...)  {} 
#endif

static void __xregion_lockop(struct _XTreeHandle *tk,
		                       const int write_lock,  const int read_lock) { 

 if(!write_lock && !read_lock) return;
 struct _XRegionHandle *h = tk->h;
// fprintf(stderr,"\n[%d] LOCKOP START" , tcb()->tid);
// Cyc_Core_print_region_pages(h);
 const unsigned int self = tcb()->tid; 
 unsigned short *local_ref_cnt = &tk->caps.rcnt;

 assert(*local_ref_cnt > 0);

 short *local_wr_cnt = &tk->caps.wr_cnt;
 short *local_rd_cnt = &tk->caps.rd_cnt;
 int *rd_cnt = &h->caps.rd_cnt;

   dbgmsg_lockop("\n[%d] __xregion_lockop:  : RH=%p TH=%p"
                 " (wr=%d,rd=%d,grd=%d) + (%d,%d)\n",	
		 tcb()->tid,h,tk,local_wr_cnt[0],
                 local_rd_cnt[0],rd_cnt[0],
                 write_lock,read_lock); 

   /* STEP 1: lock op on sub-regions */
   struct _XTreeHandle *child;
   int i;
   for( i=0,child = tk->sub_keys ; child != NULL ; 
        child = child->next_key,i++) {
       dbgmsg_lockop("\n[%d] __xregion_locop LOCK STEP1.%d: %p "
                    " (tree_key=%p)\n",
                     self,i,tk,child);
       __xregion_lockop(child,write_lock,read_lock);
   }

 unsigned int *lock  = &h->caps.lock;
 unsigned int *wr_cv = &h->caps.writer_cv;
 unsigned int *rd_cv = &h->caps.reader_cv;
 unsigned int *wr_q  = &h->caps.writer_q;
 unsigned int *rd_q  = &h->caps.reader_q;
 int wait_wq = 0;
 int wait_rq = 0;
 int wait_val = 0;
 assert(*local_wr_cnt >= 0);
 assert(*local_rd_cnt >= 0);
 assert(*local_wr_cnt + write_lock >= 0);
 assert(*local_rd_cnt + read_lock >= 0);
 assert( *wr_cv != self ||
         (*wr_cv == self &&
            *local_wr_cnt > 0 &&
           ((local_rd_cnt[0] && rd_cnt[0] == 1) ||
            (!local_rd_cnt[0] && rd_cnt[0] == 0)
           )
          )
        );
  do { 
   // wait if necessary
    if( wait_wq) { 
       dbgmsg_lockop("\n[%d] __xregion_lockop:  : waiting in write queue",	
		tcb()->tid);
      while(1) {
		  wait_val = *wr_cv;
		  if(!wait_val) break; 
        futex_wait((int *) wr_cv,(int)wait_val);
		}
      dbgmsg_lockop("\n[%d] __xregion_lockop:  : exited write queue",	
		tcb()->tid); 
     // __sync_fetch_and_add(wr_q,-1);//remove from write q
    } else if(wait_rq) {
       dbgmsg_lockop("\n[%d] __xregion_lockop:  : waiting in read queue",	
		tcb()->tid); 
      while(1) {
		  wait_val = *rd_cv;
		  if(!wait_val) break; 
        futex_wait((int *) rd_cv,(int) wait_val);
		}
      dbgmsg_lockop("\n[%d] __xregion_lockop:  : exited read queue",	
		tcb()->tid); 
      //__sync_fetch_and_add(rd_q,-1);//remove to read q
    }


    dbgmsg_lockop("\n[%d] __xregion_lockop:  : waiting for lock %p",	
		tcb()->tid,lock); 
    wait_lock(lock); // acquire exclusive access to region
    //remove from queue
    if( wait_wq)  wr_q[0] -= 1;
    else if(wait_rq) rd_q[0] -= 1;
    wait_wq = wait_rq = 0; // reset wait flags
 
    dbgmsg_lockop("\n[%d] __xregion_lockop:  : acquired lock %p",	
		tcb()->tid,lock); 

    if(write_lock) {
      if(local_wr_cnt[0]){
        dbgmsg_lockop("\n[%d] __xregion_lockop: "
				  			 "  WR: %p ->  local(%d) + %d\n",	
		tcb()->tid,h,local_wr_cnt[0],write_lock); 
        if(!(local_wr_cnt[0] += write_lock))
         *wr_cv = 0; // release writer lock
      } else {
       const int canlock = *rd_cnt < 2 &&
                         (!rd_cnt[0] || *local_rd_cnt ) &&
                         *wr_cv == 0;
        if(canlock) {
          dbgmsg_lockop("\n[%d] __xregion_lockop:  WR: CAN LOCK\n",	
		tcb()->tid); 
         *rd_cv = 1; // readers will have to wait
         local_wr_cnt[0] += write_lock; //increment local count
         *wr_cv = self; // "acquire" lock
        } else {
          dbgmsg_lockop("\n[%d] __xregion_lockop:  WR: CAN'T LOCK: "
                        "(rd_cnt=%d,local_rd_cnt=%d,wr_cv=%d)",	
                        tcb()->tid,rd_cnt[0],local_rd_cnt[0],wr_cv[0]); 
         wait_wq = 1; //wait in queue
         wr_q[0] += 1;
       }
     }
   }
 
   if(read_lock) {
     if(local_rd_cnt[0]) {
        dbgmsg_lockop("\n[%d] __xregion_lockop:  RD: %p ->  local(%d) + %d\n",	
   	 	tcb()->tid,h,local_rd_cnt[0],read_lock); 
       if(!(local_rd_cnt[0] += read_lock)) { 
         rd_cnt[0]--;
        dbgmsg_lockop("\n[%d] __xregion_lockop:  RD: %p ->  local = 0. Releasing RD count = %d\n",	
   	 	tcb()->tid,h,rd_cnt[0]); 
       }
     } else { // no local counts
         unsigned int wr = *wr_cv;
         if(wr != self &&  wr != 0) {
           dbgmsg_lockop("\n[%d] __xregion_lockop:  RD: FOUND Writer waiting in Q\n",	
              	 	tcb()->tid); 
           wait_rq = 1;
           rd_q[0] += 1;
         } else {
             rd_cnt[0]++;
            dbgmsg_lockop("\n[%d] __xregion_lockop:  RD: NO OTHER writer"
								 " wr_cv=%d. rd_cnt=%d\n",tcb()->tid,wr,rd_cnt[0]); 
             *local_rd_cnt += read_lock;
         }
     }
   }
   // if there's no writer
   if(!wr_cv[0]) {
     // if there's writers waiting and no readers reading
     if(*wr_q && !rd_cnt[0] ) 
       wake_single(wr_cv);
     else if(*rd_q) { // if there exist readers waiting
       *rd_cv = 0;
        wake_all(rd_cv);
     }
   }

   // assert that when there's a writer, then there's
   // at most one reader: this thread
  assert( *wr_cv != self ||
         (*wr_cv == self &&
            *local_wr_cnt > 0 &&
           ((local_rd_cnt[0] && rd_cnt[0] == 1) ||
            (!local_rd_cnt[0] && rd_cnt[0] == 0)
           )
          )
        );

    dbgmsg_lockop("\n[%d] __xregion_lockop: released lock %p",	
		 			  tcb()->tid,lock); 

   // ALWAYS unlock lock   
   unlock(lock);
 } while(wait_wq || wait_rq);

   dbgmsg_lockop("\n[%d] __xregion_lockop EXIT  : RH=%p TH=%p"
                 " (wr=%d,rd=%d,grd=%d) + (%d,%d)\n",	
		 			  tcb()->tid,h,tk,local_wr_cnt[0],
                 local_rd_cnt[0],rd_cnt[0],
                 write_lock,read_lock); 

  assert(*local_wr_cnt >= 0);
  assert(*local_rd_cnt >= 0);

 //fprintf(stderr,"\n[%d] LOCKOP END" , tcb()->tid);
// Cyc_Core_print_region_pages(h);

}


static int __xregion_refcnt(struct _XTreeHandle *tk,
                      int rg ) {
   unsigned short *rcnt = &tk->caps.rcnt; 
   unsigned int   *g_rcnt= &tk->h->caps.rcnt;
	struct _XRegionHandle *h = tk->h;
   assert(rcnt);
   assert(g_rcnt);
	//fprintf(stderr,"\n[%d] ZERO %p %d", tcb()->tid,h,*g_rcnt);
   assert(*rcnt > 0);
   assert(*rcnt + rg >= 0);
   assert(*g_rcnt > 0);
	//fprintf(stderr,"\n[%d] FIRST", tcb()->tid);
	//print_xtree(tk,0);
   if(rcnt[0]+rg > 0) {
     *rcnt +=rg;
	  return 1;
	}
  
//	fprintf(stderr,"\n[%d] _XREGION_REFCNT A" , tcb()->tid);
//	 Cyc_Core_print_region_pages(h);

   hremove(h);
   struct _XTreeHandle *iter,*prev;
    //1: release children
   for( iter = tk->sub_keys;   
        iter != NULL;iter = iter->next_key){
		__xregion_refcnt(iter,-(int)iter->caps.rcnt);
   }

//	fprintf(stderr,"\n[%d] _XREGION_REFCNT B" , tcb()->tid);
//	 Cyc_Core_print_region_pages(h);

	//fprintf(stderr,"\n[%d] SECOND", tcb()->tid);
	//print_xtree(tk,0);
 	//2: remove all locks acquired by this region
   __xregion_lockop(tk,- (int) tk->caps.wr_cnt,
                    - (int) tk->caps.rd_cnt);
   //3: remove handle "tk" from parent
   if( tk->parent_key != NULL ) {
     for( prev=NULL,iter = tk->parent_key->sub_keys; 
          iter != NULL && iter != tk ;
          prev=iter,iter = iter->next_key);
     if( prev == NULL )
       tk->parent_key->sub_keys = tk->next_key;
     else
       prev->next_key = tk->next_key;
	  
	  tk->parent_key = 0;
   }

   //4: if global ref count is zero, 
   // physically deallocate region
   *rcnt = 0;
   unsigned int prev_cnt =  __sync_fetch_and_add(g_rcnt,-1);
   if(prev_cnt == 1) {

//		fprintf(stderr,"\n[%d] _XREGION_REFCNT C" , tcb()->tid);
//	 Cyc_Core_print_region_pages(h);

	  dbgmsg_xfree("\n[%d] @@ _xregion_free: BEFORE %p\n",tcb()->tid,h);
     _xregion_free(tk->h);
	  dbgmsg_xfree("\n[%d] @@ _xregion_free: AFTER %p\n",tcb()->tid,h);
	  tk->h = 0;
   }
  return 0;
}

void _xregion_cap_th(int rgc,int wc,int rc ,
                  struct _XTreeHandle *t) {
   if(__xregion_refcnt(t,rgc) && (wc || rc)) 
      __xregion_lockop(t,wc,rc);
} 

void _xregion_cap(struct _XRegionHandle *h,
                  int rgc,int wc,int rc ) {
   _xregion_cap_th(rgc,wc,rc,get_tkey(h));
  return;
} 
/***********************************************************************
 *
 ************************************************************************/
///////////////////////////////////////////////////////////////////////
void Cyc_Core_print_xtree(struct _XRegionHandle *r0) {
	struct _XTreeHandle *h = get_tkey(r0);
  print_xtree(h,0);
}

void print_xtree( struct _XTreeHandle *tk, const unsigned int depth){
 struct _XTreeHandle *iter;
 int i;

 if( tk == NULL && depth == 0 ){
   fprintf(stderr,"\n[%d] Printing XTree :"
           " %p ==> Empty",
	  tcb()->tid,tk);
   return;
 }

 if( depth == 0 )
   fprintf(stderr,
          "\n----------------------------------\n"
          "[Dumping Tree - tid=%X]"
          "\n----------------------------------\n",
   	  tcb()->tid);

 for( i = 0 ; i < depth ; i++ ) printf("\t");
   fprintf(stderr,
          "- Node = %p Local(Rg=%d,Wr_Lk=%d,Rd_Lk=%d)"
          "   parent_key = %p xhandle = %p [sub_keys=%p]"
          "  Global(Rg=%d,Rd_Cnt=%d)",
   	  tk,(int) tk->caps.rcnt,(int) tk->caps.wr_cnt, 
          (int) tk->caps.rd_cnt, tk->parent_key,tk->h,
          tk->sub_keys, tk->h->caps.rcnt, 
         tk->h->caps.rd_cnt
         );

/* if( tk->h != NULL)
   fprintf(stderr," global count = %d\n\n", 
            tk->h->caps.rcnt);
 else 
   fprintf(stderr," global count = <null>\n\n");*/

 for(iter = tk->sub_keys; iter != NULL;iter = iter->next_key)
   print_xtree(iter,depth+1);
		
 if( depth == 0 )
   fprintf(stderr,
          "\n----------------------------------\n");
}
/*******************************************************************
 *
 *   Hash table stuff
 *
 ********************************************************************/
 #define my_malloc(x) xregion_malloc(x)
 #define my_free(x)   xregion_free(x)
 #define H_CACHE_SIZE  2
struct hashtable {
	 entry_t cache[H_CACHE_SIZE];
	 short cache_idx; // will be padded
    unsigned int tablelength;
    unsigned int entrycount;
    unsigned int upper_loadlimit;
    unsigned int lower_loadlimit;
    unsigned int primeindex;
    entry_t table[];
};


static inline unsigned int indexFor
(unsigned int tablelength, void * hashvalue) {
//	if( sizeof(void * ) == sizeof(int) )
    return (unsigned int )( (unsigned int) hashvalue % tablelength);
//	else{ 
//	unsigned long long	hv = (unsigned long long) hashvalue;
  //  return (unsigned int) 
	//	 	( hv % (unsigned long long)tablelength); 
//	}
};

static const unsigned int primes[] = {
53, 97, 193, 389,
769, 1543, 3079, 6151,
12289, 24593, 49157, 98317,
196613, 393241, 786433, 1572869,
3145739, 6291469, 12582917, 25165843,
50331653, 100663319, 201326611, 402653189,
805306457, 1610612741
};
const unsigned int prime_table_length = sizeof(primes)/sizeof(primes[0]);
const int max_load_factor = 200;
const int min_load_factor = 50;

#define htb_sz(x) (sizeof(struct hashtable) +(sizeof(entry_t) * x))

struct hashtable * create_hashtable(unsigned int minsize )
{
    struct hashtable *h;
    unsigned int pindex, size = primes[0];
    for (pindex=0; pindex < prime_table_length; pindex++) {
        if (primes[pindex] > minsize) { size = primes[pindex]; break; }
    }
	 const int tbl_sz = htb_sz(size);
    h = (struct hashtable *) my_malloc(tbl_sz);
    if (NULL == h) return NULL; 
    memset(h, 0, tbl_sz);
    h->tablelength  = size;
    h->primeindex   = pindex;
    h->entrycount   = 0;
    h->upper_loadlimit    = (unsigned int) (size * max_load_factor)/100;
    h->lower_loadlimit    = (unsigned int) (size * min_load_factor)/100;
	 h->cache[0] = 0;
	 h->cache[1] = 0;
	 h->cache_idx =0;
    return h;
}

static struct hashtable *hashtable_expand(struct hashtable *h, int grow)
{
    entry_t *iter,*tbl,*orig,t,t1;
    unsigned int newsize, i, index,sz;
	 int pindex = h->primeindex;
    if ( grow){
		  if( pindex == (prime_table_length - 1)) return 0;
	    newsize = primes[pindex+1];
	 } else{
	    if ( pindex == 0 ) return h;
	    newsize = primes[pindex-1];
	 }

	 const int tbl_sz = htb_sz(newsize);
	 struct hashtable *h1 = (struct hashtable *) my_malloc(tbl_sz);
		if( h1 == NULL ) return NULL;
      memset(h1, 0, tbl_sz);
		memcpy(h1,h, (int) &(h1->table[0]) - (int) h1); //copy cache
          for (i = 0,sz=h->tablelength,iter=&(h->table[0]),
				  orig = &((h1->table[0]));  i < sz ;   i++,iter++) {
			  for( t = *iter ;  t != NULL ; ){
                index = indexFor(newsize,t->h);
					 t1 = t;
					 t = t->next;
					 if( t == t1 ) break;
					 tbl = &(orig[index]); //save head of list
					 t1->next = *tbl;
					 *tbl = t1;
            }
		  }
	 my_free(h);
	 if( grow )
		 h1->primeindex++;
	 else
		 h1->primeindex--;
    h1->tablelength = newsize;
    h1->upper_loadlimit   = (unsigned int) (newsize * max_load_factor)/100;
    h1->lower_loadlimit   = (unsigned int) (newsize * min_load_factor)/100;
    return h1;
}


int hashtable_insert(struct hashtable **h,entry_t kv )
{
	 //printf("\n h => %p\n", h); fflush(stdout);
//	 printf("\n[%X] hashtable_insert : tbl = %p key = %p val = %p\n",pthread_self(),*h,	kv->h,(void *)kv); fflush(stdout);

    unsigned int index;
	 struct hashtable *h1 = *h;
    if ((h1->entrycount)+1 > h1->upper_loadlimit)
    {
        // Ignore the return value. If expand fails, we should
        // * still try cramming just this value into the existing table
         // * -- we may not have memory for a larger table, but one more
        // * element may be ok. Next time we insert, we'll try expanding again./
        h1 = hashtable_expand(h1,1);
		  if( h1 == NULL)  return 0;
		  *h = h1;
    }
	 h1->entrycount++;
    index = indexFor(h1->tablelength,kv->h);
	 kv->next = h1->table[index];
//	  dbgmsg_new_xregion("\n[%X] HASHTABLE INSERT: h=%p len=%d index=%d before=%p after=%p\n",pthread_self(),h1,h1->tablelength,index,kv->next,kv);

	 if( kv->next == kv ) kv->next = NULL;
//		 errquit("\n[%X]hashtable_insert\n",pthread_self(),);
	 //if( pthread_self() == pid && reg != NULL ) hashtable_search(h1,reg);
	 h1->table[index] = kv;
	 //update cache
	 h1->cache[h1->cache_idx++] = kv;
	 if( h1->cache_idx == H_CACHE_SIZE) h1->cache_idx = 0;
//	 h->cache[1] = 0;

	 //if( pthread_self() == pid && reg != NULL ) hashtable_search(h1,reg);

//	  dbgmsg_new_xregion("\n[%X] HASHTABLE INSERT: h=%p index=%d after=%p\n",pthread_self(),h1,index,h1->table[index]);

	 //cache it
	 //short *s = &(h1->cache_idx);
	 //if( *s == H_CACHE_SIZE ) *s = 0;
	 //h1->cache[*s] = kv;
	 //(*s)++;
    return 1;
}
static void errquit0( const char *p , void *k ){
  errquit(p,k);
}

entry_t hashtable_search(struct hashtable *h, tkey_t k)
{ 
	 //search cache
	 if( h->cache[0] != 0 && h->cache[0]->h == k ) return h->cache[0];
	 if( h->cache[1] != 0 && h->cache[1]->h == k ) return h->cache[1];

	 unsigned int index;//,sz ;
	 /// *entry_t *iter = &(h->cache[0]);
	 //for( index = 0 ,sz = h->cache_idx ; index < sz ; index++,iter++ )
		// if( (void *) (*iter)->h == k ) return *iter;* /
    entry_t e;
    index = indexFor(h->tablelength,k);
    for( e = (h->table[index]);  e != NULL && (k != e->h) ; e = e->next);
	 if( e == NULL ) 
	       errquit0("hashtable search for key : %p"
                        " failed.",k);
	 //update cache
	 h->cache[h->cache_idx++] = e;
	 if( h->cache_idx == H_CACHE_SIZE) h->cache_idx = 0;
	 return e;
}

int hashtable_remove(struct hashtable **h, tkey_t k) //assumes only one key
{
	struct hashtable *h1 = *h;
	//invalidate cache
	if( h1->cache[0] != 0 && h1->cache[0]->h == k ) h1->cache[0] = 0;  
	else if( h1->cache[1] != 0 && h1->cache[1]->h == k ) h1->cache[1] = 0;

    entry_t e,pv,*iter;
    unsigned index;//,sz;
    // clear cache
	 // *for( index = 0 ,sz = h->cache_idx,iter=&(h->cache[index]) ;
	 //				 index < sz ; index++,iter++ )
		// if( (*iter)->h == k ){
		//		 memmove(iter,iter+1,sz-index-1);
		//		 h->cache_idx--;
		//		 break;
		//}* /
	   // remove from table
      index = indexFor(h1->tablelength,k);
		iter = &(h1->table[index]);
//printf("\n[%X]__xdec:  HASHTABLE REMOVE BEFORE: h=%p  tree_handle = %p idx=%d tbl=%p\n",pthread_self(),h1,k,index,(h1->table[index]));

	   for( e = *iter , pv = NULL; 
				e != NULL && k != e->h ; pv=e,e = e->next );
		 if( e == NULL ) 
			 errquit("Attempting to remove a non-existent key");
	    h1->entrycount--;
//	 if( pthread_self() == pid && reg != NULL && k != reg ) hashtable_search(h1,reg);
		 if( pv == NULL) *iter = e->next;
		 else if( e->next == pv ) pv->next = NULL;
		 else pv->next = e->next;
//	 if( pthread_self() == pid && reg != NULL && k != reg ) hashtable_search(h1,reg);
//  		dbgmsg_xdec_free("\n[%X]__xdec:   HASHTABLE REMOVE AFTER: h=%p tree_handle = %p tbl=%p\n",pthread_self(),h1,k,(h1->table[index]));
		 // / compaction * /
	    if(h1->primeindex > 0 && (h1->entrycount) <= h1->lower_loadlimit){
           h1 = hashtable_expand(h1,0);
			  if( h1 == NULL)  return 0;
			  *h = h1;
   	 }
		 return 1;
}

void hashtable_destroy(struct hashtable *h)
{    my_free(h); }
///////////////////////////////////////////////////
//


#include <setjmp.h>
/* This is a C header file to be used by the output of the Cyclone to
   C translator.  The corresponding definitions are in file lib/runtime_*.c */
#ifndef _CYC_INCLUDE_H_
#define _CYC_INCLUDE_H_

/* Need one of these per thread (see runtime_stack.c). The runtime maintains 
   a stack that contains either _handler_cons structs or _RegionHandle structs.
   The tag is 0 for a handler_cons and 1 for a region handle.  */
struct _RuntimeStack {
  int tag; 
  struct _RuntimeStack *next;
  void (*cleanup)(struct _RuntimeStack *frame);
};

#ifndef offsetof
/* should be size_t, but int is fine. */
#define offsetof(t,n) ((int)(&(((t *)0)->n)))
#endif

/* Fat pointers */
struct _fat_ptr {
  unsigned char *curr; 
  unsigned char *base; 
  unsigned char *last_plus_one; 
};  

/* Discriminated Unions */
struct _xtunion_struct { char *tag; };

/* Regions */
struct _RegionPage
#ifdef CYC_REGION_PROFILE
{ unsigned total_bytes;
  unsigned free_bytes;
  /* MWH: wish we didn't have to include the stuff below ... */
  struct _RegionPage *next;
  char data[1];
}
#endif
; // abstract -- defined in runtime_memory.c
struct _RegionHandle {
  int is_xrgn; 
  struct _RuntimeStack s;
  struct _RegionPage *curr;
  char               *offset;
  char               *last_plus_one;
  struct _DynRegionHandle *sub_regions;
#ifdef CYC_REGION_PROFILE
  const char         *name;
#else
  unsigned used_bytes;
  unsigned wasted_bytes;
#endif
};
struct _DynRegionFrame {
  struct _RuntimeStack s;
  struct _DynRegionHandle *x;
};
// A dynamic region is just a region handle.  The wrapper struct is for type
// abstraction.
struct Cyc_Core_DynamicRegion {
  struct _RegionHandle h;
};

struct _RegionHandle _new_region(const char*);
void* _region_malloc(struct _RegionHandle*, unsigned);
void* _region_calloc(struct _RegionHandle*, unsigned , unsigned );
void   _free_region(struct _RegionHandle*);
struct _RegionHandle*_open_dynregion(struct _DynRegionFrame*,struct _DynRegionHandle*);
void   _pop_dynregion();

void _push_region(struct _RegionHandle *);
void _pop_region();


/* X-Regions*/
struct _XRegionHandle;
//parent_key can be found through the hashtable
struct _XTreeHandle { // stack allocated.

     struct _XRegionHandle *h; // heap allocated.
     struct _XTreeHandle *next; // next-entry in the hash-table

     struct _XTreeHandle *parent_key; // a linked-list parent 
     struct _XTreeHandle *sub_keys; // a linked-list of subkeys
     struct _XTreeHandle *next_key; // next sibling

	  struct _LocalCaps {
		  unsigned char rcnt; // local ref count 
		  unsigned char lcnt; // local lock count
		  unsigned short lcnt_other; // locks acquired
		 										// by ancestor regions. 
	  } caps;
};

struct _XRegionHandle *
	_new_xregion( struct _XRegionHandle *,
					  unsigned int,
					  struct _XTreeHandle *
			      );

void * _xregion_malloc(struct _XRegionHandle *,unsigned int ); 
void * _xregion_calloc(struct _XRegionHandle *,unsigned int ); 


/* run-time effects */
struct _EffectFrameCaps {
	unsigned char idx; //index -> formal_handles or inner_handles
	unsigned char rg; //region count
	unsigned char lk; //lock count
};

struct _EffectFrame 
{
  struct _RuntimeStack s;
  unsigned char *formal_handles; 
  unsigned char max_formal;
  void **inner_handles;
};

void _push_effect( struct _EffectFrame *,
						 const struct _EffectFrameCaps *,
						 const unsigned int 
					  );
void _spawn_with_effect(const struct _EffectFrameCaps *,
								 const unsigned int, 
						 		 void (*)(void *) ,
						 		 void *,
								 const unsigned char *,
								 const unsigned int,
								 const unsigned int,
								 const unsigned int
					  		  );

void _pop_effect();
void _peek_effect(  void **  );



/* Exceptions */
struct _handler_cons {
  struct _RuntimeStack s;
  jmp_buf handler;
};

void _push_handler(struct _handler_cons *);
void _npop_handler(int);
void _pop_handler();


#ifndef _throw
void* _throw_null_fn(const char*,unsigned);
void* _throw_arraybounds_fn(const char*,unsigned);
void* _throw_badalloc_fn(const char*,unsigned);
void* _throw_match_fn(const char*,unsigned);
void* _throw_fn(void*,const char*,unsigned);
void* _rethrow(void*);
#define _throw_null() (_throw_null_fn(__FILE__,__LINE__))
#define _throw_arraybounds() (_throw_arraybounds_fn(__FILE__,__LINE__))
#define _throw_badalloc() (_throw_badalloc_fn(__FILE__,__LINE__))
#define _throw_match() (_throw_match_fn(__FILE__,__LINE__))
#define _throw(e) (_throw_fn((e),__FILE__,__LINE__))
#endif

struct _xtunion_struct* Cyc_Core_get_exn_thrown();
/* Built-in Exceptions */
struct Cyc_Null_Exception_exn_struct { char *tag; };
struct Cyc_Array_bounds_exn_struct { char *tag; };
struct Cyc_Match_Exception_exn_struct { char *tag; };
struct Cyc_Bad_alloc_exn_struct { char *tag; };
extern char Cyc_Null_Exception[];
extern char Cyc_Array_bounds[];
extern char Cyc_Match_Exception[];
extern char Cyc_Bad_alloc[];

/* Built-in Run-time Checks and company */
#ifdef CYC_ANSI_OUTPUT
#define _INLINE  
#else
#define _INLINE inline
#endif

#ifdef NO_CYC_NULL_CHECKS
#define _check_null(ptr) (ptr)
#else
#define _check_null(ptr) \
  ({ void*_cks_null = (void*)(ptr); \
     if (!_cks_null) _throw_null(); \
     _cks_null; })
#endif

#ifdef NO_CYC_BOUNDS_CHECKS
#define _check_known_subscript_notnull(ptr,bound,elt_sz,index)\
   (((char*)ptr) + (elt_sz)*(index))
#ifdef NO_CYC_NULL_CHECKS
#define _check_known_subscript_null _check_known_subscript_notnull
#else
#define _check_known_subscript_null(ptr,bound,elt_sz,index) ({ \
  char*_cks_ptr = (char*)(ptr);\
  int _index = (index);\
  if (!_cks_ptr) _throw_null(); \
  _cks_ptr + (elt_sz)*_index; })
#endif
#define _zero_arr_plus_fn(orig_x,orig_sz,orig_i,f,l) ((orig_x)+(orig_i))
#define _zero_arr_plus_char_fn _zero_arr_plus_fn
#define _zero_arr_plus_short_fn _zero_arr_plus_fn
#define _zero_arr_plus_int_fn _zero_arr_plus_fn
#define _zero_arr_plus_float_fn _zero_arr_plus_fn
#define _zero_arr_plus_double_fn _zero_arr_plus_fn
#define _zero_arr_plus_longdouble_fn _zero_arr_plus_fn
#define _zero_arr_plus_voidstar_fn _zero_arr_plus_fn
#else
#define _check_known_subscript_null(ptr,bound,elt_sz,index) ({ \
  char*_cks_ptr = (char*)(ptr); \
  unsigned _cks_index = (index); \
  if (!_cks_ptr) _throw_null(); \
  if (_cks_index >= (bound)) _throw_arraybounds(); \
  _cks_ptr + (elt_sz)*_cks_index; })
#define _check_known_subscript_notnull(ptr,bound,elt_sz,index) ({ \
  char*_cks_ptr = (char*)(ptr); \
  unsigned _cks_index = (index); \
  if (_cks_index >= (bound)) _throw_arraybounds(); \
  _cks_ptr + (elt_sz)*_cks_index; })

/* _zero_arr_plus_*_fn(x,sz,i,filename,lineno) adds i to zero-terminated ptr
   x that has at least sz elements */
char* _zero_arr_plus_char_fn(char*,unsigned,int,const char*,unsigned);
short* _zero_arr_plus_short_fn(short*,unsigned,int,const char*,unsigned);
int* _zero_arr_plus_int_fn(int*,unsigned,int,const char*,unsigned);
float* _zero_arr_plus_float_fn(float*,unsigned,int,const char*,unsigned);
double* _zero_arr_plus_double_fn(double*,unsigned,int,const char*,unsigned);
long double* _zero_arr_plus_longdouble_fn(long double*,unsigned,int,const char*, unsigned);
void** _zero_arr_plus_voidstar_fn(void**,unsigned,int,const char*,unsigned);
#endif

/* _get_zero_arr_size_*(x,sz) returns the number of elements in a
   zero-terminated array that is NULL or has at least sz elements */
int _get_zero_arr_size_char(const char*,unsigned);
int _get_zero_arr_size_short(const short*,unsigned);
int _get_zero_arr_size_int(const int*,unsigned);
int _get_zero_arr_size_float(const float*,unsigned);
int _get_zero_arr_size_double(const double*,unsigned);
int _get_zero_arr_size_longdouble(const long double*,unsigned);
int _get_zero_arr_size_voidstar(const void**,unsigned);

/* _zero_arr_inplace_plus_*_fn(x,i,filename,lineno) sets
   zero-terminated pointer *x to *x + i */
char* _zero_arr_inplace_plus_char_fn(char**,int,const char*,unsigned);
short* _zero_arr_inplace_plus_short_fn(short**,int,const char*,unsigned);
int* _zero_arr_inplace_plus_int(int**,int,const char*,unsigned);
float* _zero_arr_inplace_plus_float_fn(float**,int,const char*,unsigned);
double* _zero_arr_inplace_plus_double_fn(double**,int,const char*,unsigned);
long double* _zero_arr_inplace_plus_longdouble_fn(long double**,int,const char*,unsigned);
void** _zero_arr_inplace_plus_voidstar_fn(void***,int,const char*,unsigned);
/* like the previous functions, but does post-addition (as in e++) */
char* _zero_arr_inplace_plus_post_char_fn(char**,int,const char*,unsigned);
short* _zero_arr_inplace_plus_post_short_fn(short**x,int,const char*,unsigned);
int* _zero_arr_inplace_plus_post_int_fn(int**,int,const char*,unsigned);
float* _zero_arr_inplace_plus_post_float_fn(float**,int,const char*,unsigned);
double* _zero_arr_inplace_plus_post_double_fn(double**,int,const char*,unsigned);
long double* _zero_arr_inplace_plus_post_longdouble_fn(long double**,int,const char *,unsigned);
void** _zero_arr_inplace_plus_post_voidstar_fn(void***,int,const char*,unsigned);
#define _zero_arr_plus_char(x,s,i) \
  (_zero_arr_plus_char_fn(x,s,i,__FILE__,__LINE__))
#define _zero_arr_plus_short(x,s,i) \
  (_zero_arr_plus_short_fn(x,s,i,__FILE__,__LINE__))
#define _zero_arr_plus_int(x,s,i) \
  (_zero_arr_plus_int_fn(x,s,i,__FILE__,__LINE__))
#define _zero_arr_plus_float(x,s,i) \
  (_zero_arr_plus_float_fn(x,s,i,__FILE__,__LINE__))
#define _zero_arr_plus_double(x,s,i) \
  (_zero_arr_plus_double_fn(x,s,i,__FILE__,__LINE__))
#define _zero_arr_plus_longdouble(x,s,i) \
  (_zero_arr_plus_longdouble_fn(x,s,i,__FILE__,__LINE__))
#define _zero_arr_plus_voidstar(x,s,i) \
  (_zero_arr_plus_voidstar_fn(x,s,i,__FILE__,__LINE__))
#define _zero_arr_inplace_plus_char(x,i) \
  _zero_arr_inplace_plus_char_fn((char **)(x),i,__FILE__,__LINE__)
#define _zero_arr_inplace_plus_short(x,i) \
  _zero_arr_inplace_plus_short_fn((short **)(x),i,__FILE__,__LINE__)
#define _zero_arr_inplace_plus_int(x,i) \
  _zero_arr_inplace_plus_int_fn((int **)(x),i,__FILE__,__LINE__)
#define _zero_arr_inplace_plus_float(x,i) \
  _zero_arr_inplace_plus_float_fn((float **)(x),i,__FILE__,__LINE__)
#define _zero_arr_inplace_plus_double(x,i) \
  _zero_arr_inplace_plus_double_fn((double **)(x),i,__FILE__,__LINE__)
#define _zero_arr_inplace_plus_longdouble(x,i) \
  _zero_arr_inplace_plus_longdouble_fn((long double **)(x),i,__FILE__,__LINE__)
#define _zero_arr_inplace_plus_voidstar(x,i) \
  _zero_arr_inplace_plus_voidstar_fn((void ***)(x),i,__FILE__,__LINE__)
#define _zero_arr_inplace_plus_post_char(x,i) \
  _zero_arr_inplace_plus_post_char_fn((char **)(x),(i),__FILE__,__LINE__)
#define _zero_arr_inplace_plus_post_short(x,i) \
  _zero_arr_inplace_plus_post_short_fn((short **)(x),(i),__FILE__,__LINE__)
#define _zero_arr_inplace_plus_post_int(x,i) \
  _zero_arr_inplace_plus_post_int_fn((int **)(x),(i),__FILE__,__LINE__)
#define _zero_arr_inplace_plus_post_float(x,i) \
  _zero_arr_inplace_plus_post_float_fn((float **)(x),(i),__FILE__,__LINE__)
#define _zero_arr_inplace_plus_post_double(x,i) \
  _zero_arr_inplace_plus_post_double_fn((double **)(x),(i),__FILE__,__LINE__)
#define _zero_arr_inplace_plus_post_longdouble(x,i) \
  _zero_arr_inplace_plus_post_longdouble_fn((long double **)(x),(i),__FILE__,__LINE__)
#define _zero_arr_inplace_plus_post_voidstar(x,i) \
  _zero_arr_inplace_plus_post_voidstar_fn((void***)(x),(i),__FILE__,__LINE__)

#ifdef NO_CYC_BOUNDS_CHECKS
#define _check_fat_subscript(arr,elt_sz,index) ((arr).curr + (elt_sz) * (index))
#define _untag_fat_ptr(arr,elt_sz,num_elts) ((arr).curr)
#else
#define _check_fat_subscript(arr,elt_sz,index) ({ \
  struct _fat_ptr _cus_arr = (arr); \
  unsigned char *_cus_ans = _cus_arr.curr + (elt_sz) * (index); \
  /* JGM: not needed! if (!_cus_arr.base) _throw_null();*/ \
  if (_cus_ans < _cus_arr.base || _cus_ans >= _cus_arr.last_plus_one) \
    _throw_arraybounds(); \
  _cus_ans; })
#define _untag_fat_ptr(arr,elt_sz,num_elts) ({ \
  struct _fat_ptr _arr = (arr); \
  unsigned char *_curr = _arr.curr; \
  if ((_curr < _arr.base || _curr + (elt_sz) * (num_elts) > _arr.last_plus_one) &&\
      _curr != (unsigned char *)0) \
    _throw_arraybounds(); \
  _curr; })
#endif

#define _tag_fat(tcurr,elt_sz,num_elts) ({ \
  struct _fat_ptr _ans; \
  unsigned _num_elts = (num_elts);\
  _ans.base = _ans.curr = (void*)(tcurr); \
  /* JGM: if we're tagging NULL, ignore num_elts */ \
  _ans.last_plus_one = _ans.base ? (_ans.base + (elt_sz) * _num_elts) : 0; \
  _ans; })

#define _get_fat_size(arr,elt_sz) \
  ({struct _fat_ptr _arr = (arr); \
    unsigned char *_arr_curr=_arr.curr; \
    unsigned char *_arr_last=_arr.last_plus_one; \
    (_arr_curr < _arr.base || _arr_curr >= _arr_last) ? 0 : \
    ((_arr_last - _arr_curr) / (elt_sz));})

#define _fat_ptr_plus(arr,elt_sz,change) ({ \
  struct _fat_ptr _ans = (arr); \
  int _change = (change);\
  _ans.curr += (elt_sz) * _change;\
  _ans; })
#define _fat_ptr_inplace_plus(arr_ptr,elt_sz,change) ({ \
  struct _fat_ptr * _arr_ptr = (arr_ptr); \
  _arr_ptr->curr += (elt_sz) * (change);\
  *_arr_ptr; })
#define _fat_ptr_inplace_plus_post(arr_ptr,elt_sz,change) ({ \
  struct _fat_ptr * _arr_ptr = (arr_ptr); \
  struct _fat_ptr _ans = *_arr_ptr; \
  _arr_ptr->curr += (elt_sz) * (change);\
  _ans; })

//Not a macro since initialization order matters. Defined in runtime_zeroterm.c.
struct _fat_ptr _fat_ptr_decrease_size(struct _fat_ptr,unsigned sz,unsigned numelts);

/* Allocation */
void* GC_malloc(int);
void* GC_malloc_atomic(int);
void* GC_calloc(unsigned,unsigned);
void* GC_calloc_atomic(unsigned,unsigned);
// bound the allocation size to be < MAX_ALLOC_SIZE. See macros below for usage.
#define MAX_MALLOC_SIZE (1 << 28)
void* _bounded_GC_malloc(int,const char*,int);
void* _bounded_GC_malloc_atomic(int,const char*,int);
void* _bounded_GC_calloc(unsigned,unsigned,const char*,int);
void* _bounded_GC_calloc_atomic(unsigned,unsigned,const char*,int);
/* these macros are overridden below ifdef CYC_REGION_PROFILE */
#ifndef CYC_REGION_PROFILE
#define _cycalloc(n) _bounded_GC_malloc(n,__FILE__,__LINE__)
#define _cycalloc_atomic(n) _bounded_GC_malloc_atomic(n,__FILE__,__LINE__)
#define _cyccalloc(n,s) _bounded_GC_calloc(n,s,__FILE__,__LINE__)
#define _cyccalloc_atomic(n,s) _bounded_GC_calloc_atomic(n,s,__FILE__,__LINE__)
#endif

static _INLINE unsigned int _check_times(unsigned x, unsigned y) {
  unsigned long long whole_ans = 
    ((unsigned long long) x)*((unsigned long long)y);
  unsigned word_ans = (unsigned)whole_ans;
  if(word_ans < whole_ans || word_ans > MAX_MALLOC_SIZE)
    _throw_badalloc();
  return word_ans;
}

#define _CYC_MAX_REGION_CONST 2
#define _CYC_MIN_ALIGNMENT (sizeof(double))

#ifdef CYC_REGION_PROFILE
extern int rgn_total_bytes;
#endif

static _INLINE void *_fast_region_malloc(struct _RegionHandle *r, unsigned orig_s) {  
  if (r > (struct _RegionHandle *)_CYC_MAX_REGION_CONST && r->curr != 0) { 
#ifdef CYC_NOALIGN
    unsigned s =  orig_s;
#else
    unsigned s =  (orig_s + _CYC_MIN_ALIGNMENT - 1) & (~(_CYC_MIN_ALIGNMENT -1)); 
#endif
    char *result; 
    result = r->offset; 
    if (s <= (r->last_plus_one - result)) {
      r->offset = result + s; 
#ifdef CYC_REGION_PROFILE
    r->curr->free_bytes = r->curr->free_bytes - s;
    rgn_total_bytes += s;
#endif
      return result;
    }
  } 
  return _region_malloc(r,orig_s); 
}

#ifdef CYC_REGION_PROFILE
/* see macros below for usage. defined in runtime_memory.c */
void* _profile_GC_malloc(int,const char*,const char*,int);
void* _profile_GC_malloc_atomic(int,const char*,const char*,int);
void* _profile_GC_calloc(unsigned,unsigned,const char*,const char*,int);
void* _profile_GC_calloc_atomic(unsigned,unsigned,const char*,const char*,int);
void* _profile_region_malloc(struct _RegionHandle*,unsigned,const char*,const char*,int);
void* _profile_region_calloc(struct _RegionHandle*,unsigned,unsigned,const char *,const char*,int);
struct _RegionHandle _profile_new_region(const char*,const char*,const char*,int);
void _profile_free_region(struct _RegionHandle*,const char*,const char*,int);
#ifndef RUNTIME_CYC
#define _new_region(n) _profile_new_region(n,__FILE__,__FUNCTION__,__LINE__)
#define _free_region(r) _profile_free_region(r,__FILE__,__FUNCTION__,__LINE__)
#define _region_malloc(rh,n) _profile_region_malloc(rh,n,__FILE__,__FUNCTION__,__LINE__)
#define _region_calloc(rh,n,t) _profile_region_calloc(rh,n,t,__FILE__,__FUNCTION__,__LINE__)
#  endif
#define _cycalloc(n) _profile_GC_malloc(n,__FILE__,__FUNCTION__,__LINE__)
#define _cycalloc_atomic(n) _profile_GC_malloc_atomic(n,__FILE__,__FUNCTION__,__LINE__)
#define _cyccalloc(n,s) _profile_GC_calloc(n,s,__FILE__,__FUNCTION__,__LINE__)
#define _cyccalloc_atomic(n,s) _profile_GC_calloc_atomic(n,s,__FILE__,__FUNCTION__,__LINE__)
#endif
#endif
 struct Cyc_Core_Opt{void*v;};extern char Cyc_Core_Invalid_argument[17U];struct Cyc_Core_Invalid_argument_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Failure[8U];struct Cyc_Core_Failure_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Impossible[11U];struct Cyc_Core_Impossible_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Not_found[10U];struct Cyc_Core_Not_found_exn_struct{char*tag;};
# 146 "core.h"
extern struct Cyc_Core_Not_found_exn_struct Cyc_Core_Not_found_val;extern char Cyc_Core_Unreachable[12U];struct Cyc_Core_Unreachable_exn_struct{char*tag;struct _fat_ptr f1;};
# 165
extern struct _RegionHandle*Cyc_Core_heap_region;
# 168
extern struct _RegionHandle*Cyc_Core_unique_region;struct Cyc_Core_DynamicRegion;struct Cyc_Core_NewDynamicRegion{struct Cyc_Core_DynamicRegion*key;};struct Cyc_Core_ThinRes{void*arr;unsigned nelts;};struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};
# 61 "list.h"
int Cyc_List_length(struct Cyc_List_List*x);
# 72
struct Cyc_List_List*Cyc_List_rcopy(struct _RegionHandle*,struct Cyc_List_List*x);
# 79
struct Cyc_List_List*Cyc_List_rmap(struct _RegionHandle*,void*(*f)(void*),struct Cyc_List_List*x);
# 86
struct Cyc_List_List*Cyc_List_rmap_c(struct _RegionHandle*,void*(*f)(void*,void*),void*env,struct Cyc_List_List*x);extern char Cyc_List_List_mismatch[14U];struct Cyc_List_List_mismatch_exn_struct{char*tag;};
# 100
struct Cyc_List_List*Cyc_List_rmap2(struct _RegionHandle*,void*(*f)(void*,void*),struct Cyc_List_List*x,struct Cyc_List_List*y);
# 112
struct Cyc_List_List*Cyc_List_rmap3(struct _RegionHandle*,void*(*f)(void*,void*,void*),struct Cyc_List_List*x,struct Cyc_List_List*y,struct Cyc_List_List*z);
# 153
void*Cyc_List_fold_right(void*(*f)(void*,void*),struct Cyc_List_List*x,void*accum);
# 157
void*Cyc_List_fold_right_c(void*(*f)(void*,void*,void*),void*,struct Cyc_List_List*x,void*accum);
# 167
struct Cyc_List_List*Cyc_List_rrevappend(struct _RegionHandle*,struct Cyc_List_List*x,struct Cyc_List_List*y);
# 175
struct Cyc_List_List*Cyc_List_rrev(struct _RegionHandle*,struct Cyc_List_List*x);
# 178
struct Cyc_List_List*Cyc_List_imp_rev(struct Cyc_List_List*x);
# 190
struct Cyc_List_List*Cyc_List_rappend(struct _RegionHandle*,struct Cyc_List_List*x,struct Cyc_List_List*y);
# 205
struct Cyc_List_List*Cyc_List_rflatten(struct _RegionHandle*,struct Cyc_List_List*x);
# 214
struct Cyc_List_List*Cyc_List_rmerge_sort(struct _RegionHandle*,int(*cmp)(void*,void*),struct Cyc_List_List*x);
# 220
struct Cyc_List_List*Cyc_List_rimp_merge_sort(int(*cmp)(void*,void*),struct Cyc_List_List*x);
# 230
struct Cyc_List_List*Cyc_List_rmerge(struct _RegionHandle*,int(*cmp)(void*,void*),struct Cyc_List_List*a,struct Cyc_List_List*b);
# 234
struct Cyc_List_List*Cyc_List_imp_merge(int(*cmp)(void*,void*),struct Cyc_List_List*a,struct Cyc_List_List*b);extern char Cyc_List_Nth[4U];struct Cyc_List_Nth_exn_struct{char*tag;};
# 276
struct Cyc_List_List*Cyc_List_rzip(struct _RegionHandle*r1,struct _RegionHandle*r2,struct Cyc_List_List*x,struct Cyc_List_List*y);
# 288
struct Cyc_List_List*Cyc_List_rzip3(struct _RegionHandle*r1,struct _RegionHandle*r2,struct Cyc_List_List*x,struct Cyc_List_List*y,struct Cyc_List_List*z);struct _tuple0{struct Cyc_List_List*f1;struct Cyc_List_List*f2;};
# 303
struct _tuple0 Cyc_List_rsplit(struct _RegionHandle*r1,struct _RegionHandle*r2,struct Cyc_List_List*x);struct _tuple1{struct Cyc_List_List*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;};
# 310
struct _tuple1 Cyc_List_rsplit3(struct _RegionHandle*r3,struct _RegionHandle*r4,struct _RegionHandle*r5,struct Cyc_List_List*x);
# 354
struct Cyc_List_List*Cyc_List_delete_cmp(int(*cmp)(void*,void*),struct Cyc_List_List*l,void*x);
# 367
struct _fat_ptr Cyc_List_rto_array(struct _RegionHandle*r,struct Cyc_List_List*x);
# 374
struct Cyc_List_List*Cyc_List_rfrom_array(struct _RegionHandle*r2,struct _fat_ptr arr);
# 380
struct Cyc_List_List*Cyc_List_rtabulate(struct _RegionHandle*r,int n,void*(*f)(int));
struct Cyc_List_List*Cyc_List_rtabulate_c(struct _RegionHandle*r,int n,void*(*f)(void*,int),void*env);
# 397
struct Cyc_List_List*Cyc_List_rfilter(struct _RegionHandle*r,int(*f)(void*),struct Cyc_List_List*x);
# 400
struct Cyc_List_List*Cyc_List_rfilter_c(struct _RegionHandle*r,int(*f)(void*,void*),void*env,struct Cyc_List_List*x);
# 32 "list.cyc"
int Cyc_List_length(struct Cyc_List_List*x){
int i=0;
# 35
while(x != 0){
++ i;
x=x->tl;}
# 39
return i;}
# 43
void*Cyc_List_hd(struct Cyc_List_List*x){
return x->hd;}
# 48
struct Cyc_List_List*Cyc_List_tl(struct Cyc_List_List*x){
return x->tl;}
# 54
struct Cyc_List_List*Cyc_List_rlist(struct _RegionHandle*r,struct _fat_ptr argv){
struct Cyc_List_List*result=0;
{unsigned i=_get_fat_size(argv,sizeof(void*))- (unsigned)1;for(0;i < _get_fat_size(argv,sizeof(void*));-- i){
result=({struct Cyc_List_List*_tmp3=_region_malloc(r,sizeof(*_tmp3));_tmp3->hd=((void**)argv.curr)[(int)i],_tmp3->tl=result;_tmp3;});}}
return result;}
# 63
struct Cyc_List_List*Cyc_List_list(struct _fat_ptr argv){
struct Cyc_List_List*result=0;
{int i=(int)(_get_fat_size(argv,sizeof(void*))- (unsigned)1);for(0;i >= 0;-- i){
result=({struct Cyc_List_List*_tmp5=_cycalloc(sizeof(*_tmp5));_tmp5->hd=*((void**)_check_fat_subscript(argv,sizeof(void*),i)),_tmp5->tl=result;_tmp5;});}}
return result;}
# 71
struct Cyc_List_List*Cyc_List_rcopy(struct _RegionHandle*r2,struct Cyc_List_List*x){
struct Cyc_List_List*result;struct Cyc_List_List*prev;
# 74
if(x == 0)return 0;result=({struct Cyc_List_List*_tmp7=_region_malloc(r2,sizeof(*_tmp7));
_tmp7->hd=x->hd,_tmp7->tl=0;_tmp7;});
prev=result;
for(x=x->tl;x != 0;x=x->tl){
struct Cyc_List_List*temp=({struct Cyc_List_List*_tmp8=_region_malloc(r2,sizeof(*_tmp8));_tmp8->hd=x->hd,_tmp8->tl=0;_tmp8;});
prev->tl=temp;
prev=temp;}
# 82
return result;}
# 85
struct Cyc_List_List*Cyc_List_copy(struct Cyc_List_List*x){
return({Cyc_List_rcopy(Cyc_Core_heap_region,x);});}
# 90
struct Cyc_List_List*Cyc_List_rmap(struct _RegionHandle*r2,void*(*f)(void*),struct Cyc_List_List*x){
struct Cyc_List_List*result;struct Cyc_List_List*prev;
# 93
if(x == 0)return 0;result=({struct Cyc_List_List*_tmpB=_region_malloc(r2,sizeof(*_tmpB));
({void*_tmp95=({f(x->hd);});_tmpB->hd=_tmp95;}),_tmpB->tl=0;_tmpB;});
prev=result;
for(x=x->tl;x != 0;x=x->tl){
struct Cyc_List_List*temp=({struct Cyc_List_List*_tmpC=_region_malloc(r2,sizeof(*_tmpC));({void*_tmp96=({f(x->hd);});_tmpC->hd=_tmp96;}),_tmpC->tl=0;_tmpC;});
prev->tl=temp;
prev=temp;}
# 101
return result;}
# 104
struct Cyc_List_List*Cyc_List_map(void*(*f)(void*),struct Cyc_List_List*x){
return({Cyc_List_rmap(Cyc_Core_heap_region,f,x);});}
# 109
struct Cyc_List_List*Cyc_List_rmap_c(struct _RegionHandle*r2,void*(*f)(void*,void*),void*env,struct Cyc_List_List*x){
struct Cyc_List_List*result;struct Cyc_List_List*prev;
# 112
if(x == 0)return 0;result=({struct Cyc_List_List*_tmpF=_region_malloc(r2,sizeof(*_tmpF));
({void*_tmp97=({f(env,x->hd);});_tmpF->hd=_tmp97;}),_tmpF->tl=0;_tmpF;});
prev=result;
for(x=x->tl;x != 0;x=x->tl){
struct Cyc_List_List*e=({struct Cyc_List_List*_tmp10=_region_malloc(r2,sizeof(*_tmp10));({void*_tmp98=({f(env,x->hd);});_tmp10->hd=_tmp98;}),_tmp10->tl=0;_tmp10;});
prev->tl=e;
prev=e;}
# 120
return result;}
# 123
struct Cyc_List_List*Cyc_List_map_c(void*(*f)(void*,void*),void*env,struct Cyc_List_List*x){
return({Cyc_List_rmap_c(Cyc_Core_heap_region,f,env,x);});}char Cyc_List_List_mismatch[14U]="List_mismatch";
# 129
struct Cyc_List_List_mismatch_exn_struct Cyc_List_List_mismatch_val={Cyc_List_List_mismatch};
# 134
struct Cyc_List_List*Cyc_List_rmap2(struct _RegionHandle*r3,void*(*f)(void*,void*),struct Cyc_List_List*x,struct Cyc_List_List*y){
# 136
struct Cyc_List_List*result;struct Cyc_List_List*prev;
# 138
if(x == 0 && y == 0)return 0;if(
x == 0 || y == 0)(int)_throw(& Cyc_List_List_mismatch_val);
# 138
result=({struct Cyc_List_List*_tmp13=_region_malloc(r3,sizeof(*_tmp13));
# 141
({void*_tmp99=({f(x->hd,y->hd);});_tmp13->hd=_tmp99;}),_tmp13->tl=0;_tmp13;});
prev=result;
# 144
x=x->tl;
y=y->tl;
# 147
while(x != 0 && y != 0){
struct Cyc_List_List*temp=({struct Cyc_List_List*_tmp14=_region_malloc(r3,sizeof(*_tmp14));({void*_tmp9A=({f(x->hd,y->hd);});_tmp14->hd=_tmp9A;}),_tmp14->tl=0;_tmp14;});
prev->tl=temp;
prev=temp;
x=x->tl;
y=y->tl;}
# 154
if(x != 0 || y != 0)(int)_throw(& Cyc_List_List_mismatch_val);return result;}
# 158
struct Cyc_List_List*Cyc_List_map2(void*(*f)(void*,void*),struct Cyc_List_List*x,struct Cyc_List_List*y){
return({Cyc_List_rmap2(Cyc_Core_heap_region,f,x,y);});}
# 163
struct Cyc_List_List*Cyc_List_rmap3(struct _RegionHandle*r3,void*(*f)(void*,void*,void*),struct Cyc_List_List*x,struct Cyc_List_List*y,struct Cyc_List_List*z){
# 165
struct Cyc_List_List*result;struct Cyc_List_List*prev;
# 167
if((x == 0 && y == 0)&& z == 0)return 0;if(
(x == 0 || y == 0)|| z == 0)(int)_throw(& Cyc_List_List_mismatch_val);
# 167
result=({struct Cyc_List_List*_tmp17=_region_malloc(r3,sizeof(*_tmp17));
# 170
({void*_tmp9B=({f(x->hd,y->hd,z->hd);});_tmp17->hd=_tmp9B;}),_tmp17->tl=0;_tmp17;});
prev=result;
# 173
x=x->tl;
y=y->tl;
z=z->tl;
# 177
while((x != 0 && y != 0)&& z != 0){
struct Cyc_List_List*temp=({struct Cyc_List_List*_tmp18=_region_malloc(r3,sizeof(*_tmp18));({void*_tmp9C=({f(x->hd,y->hd,z->hd);});_tmp18->hd=_tmp9C;}),_tmp18->tl=0;_tmp18;});
prev->tl=temp;
prev=temp;
x=x->tl;
y=y->tl;
z=z->tl;}
# 185
if((x != 0 || y != 0)|| z != 0)(int)_throw(& Cyc_List_List_mismatch_val);return result;}
# 189
struct Cyc_List_List*Cyc_List_map3(void*(*f)(void*,void*,void*),struct Cyc_List_List*x,struct Cyc_List_List*y,struct Cyc_List_List*z){
return({Cyc_List_rmap3(Cyc_Core_heap_region,f,x,y,z);});}
# 196
void Cyc_List_app(void*(*f)(void*),struct Cyc_List_List*x){
while(x != 0){
({f(x->hd);});
x=x->tl;}}
# 203
void Cyc_List_app_c(void*(*f)(void*,void*),void*env,struct Cyc_List_List*x){
while(x != 0){
({f(env,x->hd);});
x=x->tl;}}
# 213
void Cyc_List_iter(void(*f)(void*),struct Cyc_List_List*x){
while(x != 0){
({f(x->hd);});
x=x->tl;}}
# 220
void Cyc_List_iter_c(void(*f)(void*,void*),void*env,struct Cyc_List_List*x){
while(x != 0){
({f(env,x->hd);});
x=x->tl;}}
# 229
void Cyc_List_app2(void*(*f)(void*,void*),struct Cyc_List_List*x,struct Cyc_List_List*y){
while(x != 0 && y != 0){
({f(x->hd,y->hd);});
x=x->tl;
y=y->tl;}
# 235
if(x != 0 || y != 0)(int)_throw(& Cyc_List_List_mismatch_val);}
# 238
void Cyc_List_app2_c(void*(*f)(void*,void*,void*),void*env,struct Cyc_List_List*x,struct Cyc_List_List*y){
while(x != 0 && y != 0){
({f(env,x->hd,y->hd);});
x=x->tl;
y=y->tl;}
# 244
if(x != 0 || y != 0)(int)_throw(& Cyc_List_List_mismatch_val);}
# 249
void Cyc_List_iter2(void(*f)(void*,void*),struct Cyc_List_List*x,struct Cyc_List_List*y){
while(x != 0 && y != 0){
({f(x->hd,y->hd);});
x=x->tl;
y=y->tl;}
# 255
if(x != 0 || y != 0)(int)_throw(& Cyc_List_List_mismatch_val);}
# 257
void Cyc_List_iter2_c(void(*f)(void*,void*,void*),void*env,struct Cyc_List_List*x,struct Cyc_List_List*y){
while(x != 0 && y != 0){
({f(env,x->hd,y->hd);});
x=x->tl;
y=y->tl;}
# 263
if(x != 0 || y != 0)(int)_throw(& Cyc_List_List_mismatch_val);}
# 269
void*Cyc_List_fold_left(void*(*f)(void*,void*),void*accum,struct Cyc_List_List*x){
while(x != 0){
accum=({f(accum,x->hd);});
x=x->tl;}
# 274
return accum;}
# 277
void*Cyc_List_fold_left_c(void*(*f)(void*,void*,void*),void*env,void*accum,struct Cyc_List_List*x){
while(x != 0){
accum=({f(env,accum,x->hd);});
x=x->tl;}
# 282
return accum;}
# 288
void*Cyc_List_fold_right(void*(*f)(void*,void*),struct Cyc_List_List*x,void*accum){
if(x == 0)return accum;else{
return({void*(*_tmp25)(void*,void*)=f;void*_tmp26=x->hd;void*_tmp27=({Cyc_List_fold_right(f,x->tl,accum);});_tmp25(_tmp26,_tmp27);});}}
# 292
void*Cyc_List_fold_right_c(void*(*f)(void*,void*,void*),void*env,struct Cyc_List_List*x,void*accum){
if(x == 0)return accum;else{
return({void*(*_tmp29)(void*,void*,void*)=f;void*_tmp2A=env;void*_tmp2B=x->hd;void*_tmp2C=({Cyc_List_fold_right_c(f,env,x->tl,accum);});_tmp29(_tmp2A,_tmp2B,_tmp2C);});}}
# 299
struct Cyc_List_List*Cyc_List_rrevappend(struct _RegionHandle*r2,struct Cyc_List_List*x,struct Cyc_List_List*y){
# 301
while(x != 0){
y=({struct Cyc_List_List*_tmp2E=_region_malloc(r2,sizeof(*_tmp2E));_tmp2E->hd=x->hd,_tmp2E->tl=y;_tmp2E;});
x=x->tl;}
# 305
return y;}
# 308
struct Cyc_List_List*Cyc_List_revappend(struct Cyc_List_List*x,struct Cyc_List_List*y){
return({Cyc_List_rrevappend(Cyc_Core_heap_region,x,y);});}
# 313
struct Cyc_List_List*Cyc_List_rrev(struct _RegionHandle*r2,struct Cyc_List_List*x){
# 315
if(x == 0)
return 0;
# 315
return({Cyc_List_rrevappend(r2,x,0);});}
# 320
struct Cyc_List_List*Cyc_List_rev(struct Cyc_List_List*x){
return({Cyc_List_rrev(Cyc_Core_heap_region,x);});}
# 325
struct Cyc_List_List*Cyc_List_imp_rev(struct Cyc_List_List*x){
if(x == 0)return x;else{
# 328
struct Cyc_List_List*first=x;
struct Cyc_List_List*second=x->tl;
x->tl=0;
while(second != 0){
struct Cyc_List_List*temp=second->tl;
second->tl=first;
first=second;
second=temp;}
# 337
return first;}}
# 342
struct Cyc_List_List*Cyc_List_rappend(struct _RegionHandle*r2,struct Cyc_List_List*x,struct Cyc_List_List*y){
struct Cyc_List_List*result;struct Cyc_List_List*prev;
# 345
if(x == 0)return y;if(y == 0)
return({Cyc_List_rcopy(r2,x);});
# 345
result=({struct Cyc_List_List*_tmp34=_region_malloc(r2,sizeof(*_tmp34));
# 347
_tmp34->hd=x->hd,_tmp34->tl=0;_tmp34;});
prev=result;
for(x=x->tl;x != 0;x=x->tl){
struct Cyc_List_List*temp=({struct Cyc_List_List*_tmp35=_region_malloc(r2,sizeof(*_tmp35));_tmp35->hd=x->hd,_tmp35->tl=0;_tmp35;});
prev->tl=temp;
prev=temp;}
# 354
prev->tl=y;
return result;}
# 358
struct Cyc_List_List*Cyc_List_append(struct Cyc_List_List*x,struct Cyc_List_List*y){
return({Cyc_List_rappend(Cyc_Core_heap_region,x,y);});}
# 364
struct Cyc_List_List*Cyc_List_imp_append(struct Cyc_List_List*x,struct Cyc_List_List*y){
struct Cyc_List_List*z;
# 367
if(x == 0)return y;if(y == 0)
return x;
# 367
for(z=x;((struct Cyc_List_List*)_check_null(z))->tl != 0;z=z->tl){
# 370
;}
z->tl=y;
return x;}
# 376
struct Cyc_List_List*Cyc_List_rflatten(struct _RegionHandle*r3,struct Cyc_List_List*x){
return({({struct Cyc_List_List*(*_tmp9E)(struct Cyc_List_List*(*f)(struct _RegionHandle*,struct Cyc_List_List*,struct Cyc_List_List*),struct _RegionHandle*env,struct Cyc_List_List*x,struct Cyc_List_List*accum)=({struct Cyc_List_List*(*_tmp39)(struct Cyc_List_List*(*f)(struct _RegionHandle*,struct Cyc_List_List*,struct Cyc_List_List*),struct _RegionHandle*env,struct Cyc_List_List*x,struct Cyc_List_List*accum)=(struct Cyc_List_List*(*)(struct Cyc_List_List*(*f)(struct _RegionHandle*,struct Cyc_List_List*,struct Cyc_List_List*),struct _RegionHandle*env,struct Cyc_List_List*x,struct Cyc_List_List*accum))Cyc_List_fold_right_c;_tmp39;});struct _RegionHandle*_tmp9D=r3;_tmp9E(Cyc_List_rappend,_tmp9D,x,0);});});}
# 380
struct Cyc_List_List*Cyc_List_flatten(struct Cyc_List_List*x){
return({Cyc_List_rflatten(Cyc_Core_heap_region,x);});}
# 385
struct Cyc_List_List*Cyc_List_imp_merge(int(*less_eq)(void*,void*),struct Cyc_List_List*a,struct Cyc_List_List*b){
struct Cyc_List_List*c;struct Cyc_List_List*d;
# 388
if(a == 0)return b;if(b == 0)
return a;
# 388
if(({less_eq(a->hd,b->hd);})<= 0){
# 396
c=a;
a=a->tl;}else{
# 399
c=b;
b=b->tl;}
# 402
d=c;
# 404
while(a != 0 && b != 0){
# 406
if(({less_eq(a->hd,b->hd);})<= 0){
c->tl=a;
c=a;
a=a->tl;}else{
# 411
c->tl=b;
c=b;
b=b->tl;}}
# 417
if(a == 0)
c->tl=b;else{
# 420
c->tl=a;}
return d;}
# 425
struct Cyc_List_List*Cyc_List_rimp_merge_sort(int(*less_eq)(void*,void*),struct Cyc_List_List*x){
if(x == 0)return x;{struct Cyc_List_List*temp=x->tl;
# 428
if(temp == 0)return x;{struct Cyc_List_List*a=x;struct Cyc_List_List*aptr=a;
# 432
struct Cyc_List_List*b=temp;struct Cyc_List_List*bptr=b;
x=b->tl;
while(x != 0){
aptr->tl=x;
aptr=x;
x=x->tl;
if(x != 0){
bptr->tl=x;
bptr=x;
x=x->tl;}}
# 444
aptr->tl=0;
bptr->tl=0;
return({struct Cyc_List_List*(*_tmp3D)(int(*less_eq)(void*,void*),struct Cyc_List_List*a,struct Cyc_List_List*b)=Cyc_List_imp_merge;int(*_tmp3E)(void*,void*)=less_eq;struct Cyc_List_List*_tmp3F=({Cyc_List_rimp_merge_sort(less_eq,a);});struct Cyc_List_List*_tmp40=({Cyc_List_rimp_merge_sort(less_eq,b);});_tmp3D(_tmp3E,_tmp3F,_tmp40);});}}}
# 454
struct Cyc_List_List*Cyc_List_rmerge_sort(struct _RegionHandle*r2,int(*less_eq)(void*,void*),struct Cyc_List_List*x){
return({struct Cyc_List_List*(*_tmp42)(int(*less_eq)(void*,void*),struct Cyc_List_List*x)=Cyc_List_rimp_merge_sort;int(*_tmp43)(void*,void*)=less_eq;struct Cyc_List_List*_tmp44=({Cyc_List_rcopy(r2,x);});_tmp42(_tmp43,_tmp44);});}
# 459
struct Cyc_List_List*Cyc_List_rmerge(struct _RegionHandle*r3,int(*less_eq)(void*,void*),struct Cyc_List_List*a,struct Cyc_List_List*b){
# 463
struct Cyc_List_List*c;struct Cyc_List_List*d;
# 466
if(a == 0)return({Cyc_List_rcopy(r3,b);});if(b == 0)
return({Cyc_List_rcopy(r3,a);});
# 466
if(({less_eq(a->hd,b->hd);})<= 0){
# 474
c=({struct Cyc_List_List*_tmp46=_region_malloc(r3,sizeof(*_tmp46));_tmp46->hd=a->hd,_tmp46->tl=0;_tmp46;});
a=a->tl;}else{
# 477
c=({struct Cyc_List_List*_tmp47=_region_malloc(r3,sizeof(*_tmp47));_tmp47->hd=b->hd,_tmp47->tl=0;_tmp47;});
b=b->tl;}
# 480
d=c;
# 482
while(a != 0 && b != 0){
# 484
struct Cyc_List_List*temp;
if(({less_eq(a->hd,b->hd);})<= 0){
temp=({struct Cyc_List_List*_tmp48=_region_malloc(r3,sizeof(*_tmp48));_tmp48->hd=a->hd,_tmp48->tl=0;_tmp48;});
c->tl=temp;
c=temp;
a=a->tl;}else{
# 491
temp=({struct Cyc_List_List*_tmp49=_region_malloc(r3,sizeof(*_tmp49));_tmp49->hd=b->hd,_tmp49->tl=0;_tmp49;});
c->tl=temp;
c=temp;
b=b->tl;}}
# 498
if(a == 0)
({struct Cyc_List_List*_tmp9F=({Cyc_List_rcopy(r3,b);});c->tl=_tmp9F;});else{
# 501
({struct Cyc_List_List*_tmpA0=({Cyc_List_rcopy(r3,a);});c->tl=_tmpA0;});}
return d;}
# 505
struct Cyc_List_List*Cyc_List_merge_sort(int(*less_eq)(void*,void*),struct Cyc_List_List*x){
return({Cyc_List_rmerge_sort(Cyc_Core_heap_region,less_eq,x);});}
# 509
struct Cyc_List_List*Cyc_List_merge(int(*less_eq)(void*,void*),struct Cyc_List_List*a,struct Cyc_List_List*b){
return({Cyc_List_rmerge(Cyc_Core_heap_region,less_eq,a,b);});}char Cyc_List_Nth[4U]="Nth";
# 515
struct Cyc_List_Nth_exn_struct Cyc_List_Nth_val={Cyc_List_Nth};
# 520
void*Cyc_List_nth(struct Cyc_List_List*x,int i){
# 522
while(i > 0 && x != 0){
-- i;
x=x->tl;}
# 526
if(i < 0 || x == 0)(int)_throw(& Cyc_List_Nth_val);return x->hd;}
# 532
struct Cyc_List_List*Cyc_List_nth_tail(struct Cyc_List_List*x,int i){
if(i < 0)(int)_throw(& Cyc_List_Nth_val);while(i != 0){
# 535
if(x == 0)(int)_throw(& Cyc_List_Nth_val);x=x->tl;
# 537
-- i;}
# 539
return x;}
# 532
int Cyc_List_forall(int(*pred)(void*),struct Cyc_List_List*x){
# 545
while(x != 0 &&({pred(x->hd);})){x=x->tl;}
return x == 0;}
# 532
int Cyc_List_forall_c(int(*pred)(void*,void*),void*env,struct Cyc_List_List*x){
# 549
while(x != 0 &&({pred(env,x->hd);})){x=x->tl;}
return x == 0;}
# 532
int Cyc_List_exists(int(*pred)(void*),struct Cyc_List_List*x){
# 556
while(x != 0 && !({pred(x->hd);})){x=x->tl;}
return x != 0;}
# 532
int Cyc_List_exists_c(int(*pred)(void*,void*),void*env,struct Cyc_List_List*x){
# 560
while(x != 0 && !({pred(env,x->hd);})){x=x->tl;}
return x != 0;}
# 564
void*Cyc_List_find_c(void*(*pred)(void*,void*),void*env,struct Cyc_List_List*x){
void*v=0;
for(0;x != 0;x=x->tl){
v=({pred(env,x->hd);});
if((unsigned)v)break;}
# 570
return v;}struct _tuple2{void*f1;void*f2;};
# 575
struct Cyc_List_List*Cyc_List_rzip(struct _RegionHandle*r3,struct _RegionHandle*r4,struct Cyc_List_List*x,struct Cyc_List_List*y){
# 577
struct Cyc_List_List*result;struct Cyc_List_List*prev;struct Cyc_List_List*temp;
# 579
if(x == 0 && y == 0)return 0;if(
x == 0 || y == 0)(int)_throw(& Cyc_List_List_mismatch_val);
# 579
result=({struct Cyc_List_List*_tmp55=_region_malloc(r3,sizeof(*_tmp55));
# 582
({struct _tuple2*_tmpA1=({struct _tuple2*_tmp54=_region_malloc(r4,sizeof(*_tmp54));_tmp54->f1=x->hd,_tmp54->f2=y->hd;_tmp54;});_tmp55->hd=_tmpA1;}),_tmp55->tl=0;_tmp55;});
prev=result;
# 585
x=x->tl;
y=y->tl;
# 588
while(x != 0 && y != 0){
temp=({struct Cyc_List_List*_tmp57=_region_malloc(r3,sizeof(*_tmp57));({struct _tuple2*_tmpA2=({struct _tuple2*_tmp56=_region_malloc(r4,sizeof(*_tmp56));_tmp56->f1=x->hd,_tmp56->f2=y->hd;_tmp56;});_tmp57->hd=_tmpA2;}),_tmp57->tl=0;_tmp57;});
prev->tl=temp;
prev=temp;
x=x->tl;
y=y->tl;}
# 595
if(x != 0 || y != 0)(int)_throw(& Cyc_List_List_mismatch_val);return result;}
# 599
struct Cyc_List_List*Cyc_List_zip(struct Cyc_List_List*x,struct Cyc_List_List*y){
return({Cyc_List_rzip(Cyc_Core_heap_region,Cyc_Core_heap_region,x,y);});}struct _tuple3{void*f1;void*f2;void*f3;};
# 603
struct Cyc_List_List*Cyc_List_rzip3(struct _RegionHandle*r3,struct _RegionHandle*r4,struct Cyc_List_List*x,struct Cyc_List_List*y,struct Cyc_List_List*z){
# 605
struct Cyc_List_List*result;struct Cyc_List_List*prev;struct Cyc_List_List*temp;
# 607
if((x == 0 && y == 0)&& z == 0)return 0;if(
(x == 0 || y == 0)|| z == 0)(int)_throw(& Cyc_List_List_mismatch_val);
# 607
result=({struct Cyc_List_List*_tmp5B=_region_malloc(r3,sizeof(*_tmp5B));
# 610
({struct _tuple3*_tmpA3=({struct _tuple3*_tmp5A=_region_malloc(r4,sizeof(*_tmp5A));_tmp5A->f1=x->hd,_tmp5A->f2=y->hd,_tmp5A->f3=z->hd;_tmp5A;});_tmp5B->hd=_tmpA3;}),_tmp5B->tl=0;_tmp5B;});
prev=result;
# 613
x=x->tl;
y=y->tl;
z=z->tl;
# 617
while((x != 0 && y != 0)&& z != 0){
temp=({struct Cyc_List_List*_tmp5D=_region_malloc(r3,sizeof(*_tmp5D));({struct _tuple3*_tmpA4=({struct _tuple3*_tmp5C=_region_malloc(r4,sizeof(*_tmp5C));_tmp5C->f1=x->hd,_tmp5C->f2=y->hd,_tmp5C->f3=z->hd;_tmp5C;});_tmp5D->hd=_tmpA4;}),_tmp5D->tl=0;_tmp5D;});
prev->tl=temp;
prev=temp;
x=x->tl;
y=y->tl;
z=z->tl;}
# 625
if((x != 0 || y != 0)|| z != 0)(int)_throw(& Cyc_List_List_mismatch_val);return result;}
# 629
struct Cyc_List_List*Cyc_List_zip3(struct Cyc_List_List*x,struct Cyc_List_List*y,struct Cyc_List_List*z){
return({Cyc_List_rzip3(Cyc_Core_heap_region,Cyc_Core_heap_region,x,y,z);});}
# 635
struct _tuple0 Cyc_List_rsplit(struct _RegionHandle*r3,struct _RegionHandle*r4,struct Cyc_List_List*x){
# 637
struct Cyc_List_List*result1;struct Cyc_List_List*prev1;struct Cyc_List_List*temp1;
struct Cyc_List_List*result2;struct Cyc_List_List*prev2;struct Cyc_List_List*temp2;
# 640
if(x == 0)return({struct _tuple0 _tmp90;_tmp90.f1=0,_tmp90.f2=0;_tmp90;});prev1=(result1=({struct Cyc_List_List*_tmp60=_region_malloc(r3,sizeof(*_tmp60));
# 642
_tmp60->hd=(((struct _tuple2*)x->hd)[0]).f1,_tmp60->tl=0;_tmp60;}));
prev2=(result2=({struct Cyc_List_List*_tmp61=_region_malloc(r4,sizeof(*_tmp61));_tmp61->hd=(((struct _tuple2*)x->hd)[0]).f2,_tmp61->tl=0;_tmp61;}));
# 645
for(x=x->tl;x != 0;x=x->tl){
temp1=({struct Cyc_List_List*_tmp62=_region_malloc(r3,sizeof(*_tmp62));_tmp62->hd=(((struct _tuple2*)x->hd)[0]).f1,_tmp62->tl=0;_tmp62;});
temp2=({struct Cyc_List_List*_tmp63=_region_malloc(r4,sizeof(*_tmp63));_tmp63->hd=(((struct _tuple2*)x->hd)[0]).f2,_tmp63->tl=0;_tmp63;});
prev1->tl=temp1;
prev2->tl=temp2;
prev1=temp1;
prev2=temp2;}
# 653
return({struct _tuple0 _tmp91;_tmp91.f1=result1,_tmp91.f2=result2;_tmp91;});}
# 656
struct _tuple0 Cyc_List_split(struct Cyc_List_List*x){
return({Cyc_List_rsplit(Cyc_Core_heap_region,Cyc_Core_heap_region,x);});}
# 663
struct _tuple1 Cyc_List_rsplit3(struct _RegionHandle*r3,struct _RegionHandle*r4,struct _RegionHandle*r5,struct Cyc_List_List*x){
# 665
struct Cyc_List_List*result1;struct Cyc_List_List*prev1;struct Cyc_List_List*temp1;
struct Cyc_List_List*result2;struct Cyc_List_List*prev2;struct Cyc_List_List*temp2;
struct Cyc_List_List*result3;struct Cyc_List_List*prev3;struct Cyc_List_List*temp3;
# 669
if(x == 0)return({struct _tuple1 _tmp92;_tmp92.f1=0,_tmp92.f2=0,_tmp92.f3=0;_tmp92;});prev1=(result1=({struct Cyc_List_List*_tmp66=_region_malloc(r3,sizeof(*_tmp66));
# 671
_tmp66->hd=(((struct _tuple3*)x->hd)[0]).f1,_tmp66->tl=0;_tmp66;}));
prev2=(result2=({struct Cyc_List_List*_tmp67=_region_malloc(r4,sizeof(*_tmp67));_tmp67->hd=(((struct _tuple3*)x->hd)[0]).f2,_tmp67->tl=0;_tmp67;}));
prev3=(result3=({struct Cyc_List_List*_tmp68=_region_malloc(r5,sizeof(*_tmp68));_tmp68->hd=(((struct _tuple3*)x->hd)[0]).f3,_tmp68->tl=0;_tmp68;}));
# 675
for(x=x->tl;x != 0;x=x->tl){
temp1=({struct Cyc_List_List*_tmp69=_region_malloc(r3,sizeof(*_tmp69));_tmp69->hd=(((struct _tuple3*)x->hd)[0]).f1,_tmp69->tl=0;_tmp69;});
temp2=({struct Cyc_List_List*_tmp6A=_region_malloc(r4,sizeof(*_tmp6A));_tmp6A->hd=(((struct _tuple3*)x->hd)[0]).f2,_tmp6A->tl=0;_tmp6A;});
temp3=({struct Cyc_List_List*_tmp6B=_region_malloc(r5,sizeof(*_tmp6B));_tmp6B->hd=(((struct _tuple3*)x->hd)[0]).f3,_tmp6B->tl=0;_tmp6B;});
prev1->tl=temp1;
prev2->tl=temp2;
prev3->tl=temp3;
prev1=temp1;
prev2=temp2;
prev3=temp3;}
# 686
return({struct _tuple1 _tmp93;_tmp93.f1=result1,_tmp93.f2=result2,_tmp93.f3=result3;_tmp93;});}
# 689
struct _tuple1 Cyc_List_split3(struct Cyc_List_List*x){
return({Cyc_List_rsplit3(Cyc_Core_heap_region,Cyc_Core_heap_region,Cyc_Core_heap_region,x);});}
# 689
int Cyc_List_memq(struct Cyc_List_List*l,void*x){
# 697
while(l != 0){
if(l->hd == x)return 1;l=l->tl;}
# 701
return 0;}
# 689
int Cyc_List_mem(int(*cmp)(void*,void*),struct Cyc_List_List*l,void*x){
# 705
while(l != 0){
if(({cmp(l->hd,x);})== 0)return 1;l=l->tl;}
# 709
return 0;}
# 715
void*Cyc_List_assoc(struct Cyc_List_List*l,void*x){
while(l != 0){
if((((struct _tuple2*)l->hd)[0]).f1 == x)return(((struct _tuple2*)l->hd)[0]).f2;l=l->tl;}
# 720
(int)_throw(& Cyc_Core_Not_found_val);}
# 723
void*Cyc_List_assoc_cmp(int(*cmp)(void*,void*),struct Cyc_List_List*l,void*x){
while(l != 0){
if(({cmp((*((struct _tuple2*)l->hd)).f1,x);})== 0)return(*((struct _tuple2*)l->hd)).f2;l=l->tl;}
# 728
(int)_throw(& Cyc_Core_Not_found_val);}
# 735
struct Cyc_List_List*Cyc_List_delete_cmp(int(*cmp)(void*,void*),struct Cyc_List_List*l,void*x){
struct Cyc_List_List*prev=0;
struct Cyc_List_List*iter=l;
while(iter != 0){
if(({cmp(iter->hd,x);})== 0){
if(prev == 0)
return((struct Cyc_List_List*)_check_null(l))->tl;else{
# 743
prev->tl=iter->tl;
return l;}}
# 739
prev=iter;
# 748
iter=iter->tl;}
# 750
(int)_throw(& Cyc_Core_Not_found_val);}
# 753
static int Cyc_List_ptrequal(void*x,void*y){
return !(x == y);}
# 760
struct Cyc_List_List*Cyc_List_delete(struct Cyc_List_List*l,void*x){
return({Cyc_List_delete_cmp(Cyc_List_ptrequal,l,x);});}
# 760
int Cyc_List_mem_assoc(struct Cyc_List_List*l,void*x){
# 767
while(l != 0){
if((((struct _tuple2*)l->hd)[0]).f1 == x)return 1;l=l->tl;}
# 771
return 0;}
# 760
int Cyc_List_mem_assoc_cmp(int(*cmp)(void*,void*),struct Cyc_List_List*l,void*x){
# 777
while(l != 0){
if(({cmp((((struct _tuple2*)l->hd)[0]).f1,x);})== 0)return 1;l=l->tl;}
# 781
return 0;}
# 787
struct Cyc_Core_Opt*Cyc_List_check_unique(int(*cmp)(void*,void*),struct Cyc_List_List*x){
while(x != 0){
struct Cyc_List_List*tl=x->tl;
if(tl != 0){
if(({cmp(x->hd,tl->hd);})== 0)return({struct Cyc_Core_Opt*_tmp77=_cycalloc(sizeof(*_tmp77));_tmp77->v=x->hd;_tmp77;});}
# 790
x=tl;}
# 794
return 0;}
# 798
struct _fat_ptr Cyc_List_rto_array(struct _RegionHandle*r2,struct Cyc_List_List*x){
int s=({Cyc_List_length(x);});
return({unsigned _tmp7A=(unsigned)s;void**_tmp79=({struct _RegionHandle*_tmpA5=r2;_region_malloc(_tmpA5,_check_times(_tmp7A,sizeof(void*)));});({{unsigned _tmp94=(unsigned)s;unsigned i;for(i=0;i < _tmp94;++ i){({void*_tmpA6=({void*v=((struct Cyc_List_List*)_check_null(x))->hd;x=x->tl;v;});_tmp79[i]=_tmpA6;});}}0;});_tag_fat(_tmp79,sizeof(void*),_tmp7A);});}
# 803
struct _fat_ptr Cyc_List_to_array(struct Cyc_List_List*x){
return({Cyc_List_rto_array(Cyc_Core_heap_region,x);});}
# 808
struct Cyc_List_List*Cyc_List_rfrom_array(struct _RegionHandle*r2,struct _fat_ptr arr){
struct Cyc_List_List*ans=0;
{int i=(int)(_get_fat_size(arr,sizeof(void*))- (unsigned)1);for(0;i >= 0;-- i){
ans=({struct Cyc_List_List*_tmp7D=_region_malloc(r2,sizeof(*_tmp7D));_tmp7D->hd=*((void**)_check_fat_subscript(arr,sizeof(void*),i)),_tmp7D->tl=ans;_tmp7D;});}}
return ans;}
# 815
struct Cyc_List_List*Cyc_List_from_array(struct _fat_ptr arr){
return({Cyc_List_rfrom_array(Cyc_Core_heap_region,arr);});}
# 819
struct Cyc_List_List*Cyc_List_rtabulate(struct _RegionHandle*r,int n,void*(*f)(int)){
struct Cyc_List_List*res=0;
{int i=0;for(0;i < n;++ i){
res=({struct Cyc_List_List*_tmp80=_region_malloc(r,sizeof(*_tmp80));({void*_tmpA7=({f(i);});_tmp80->hd=_tmpA7;}),_tmp80->tl=res;_tmp80;});}}
# 824
return({Cyc_List_imp_rev(res);});}
# 827
struct Cyc_List_List*Cyc_List_tabulate(int n,void*(*f)(int)){
return({Cyc_List_rtabulate(Cyc_Core_heap_region,n,f);});}
# 831
struct Cyc_List_List*Cyc_List_rtabulate_c(struct _RegionHandle*r,int n,void*(*f)(void*,int),void*env){
struct Cyc_List_List*res=0;
{int i=0;for(0;i < n;++ i){
res=({struct Cyc_List_List*_tmp83=_region_malloc(r,sizeof(*_tmp83));({void*_tmpA8=({f(env,i);});_tmp83->hd=_tmpA8;}),_tmp83->tl=res;_tmp83;});}}
return({Cyc_List_imp_rev(res);});}
# 838
struct Cyc_List_List*Cyc_List_tabulate_c(int n,void*(*f)(void*,int),void*env){
return({Cyc_List_rtabulate_c(Cyc_Core_heap_region,n,f,env);});}
# 842
int Cyc_List_list_cmp(int(*cmp)(void*,void*),struct Cyc_List_List*l1,struct Cyc_List_List*l2){
for(0;l1 != 0 && l2 != 0;(l1=l1->tl,l2=l2->tl)){
# 845
if((unsigned)l1 == (unsigned)l2)
return 0;{
# 845
int i=({cmp(l1->hd,l2->hd);});
# 848
if(i != 0)
return i;}}
# 851
if(l1 != 0)
return 1;
# 851
if(l2 != 0)
# 854
return - 1;
# 851
return 0;}
# 842
int Cyc_List_list_prefix(int(*cmp)(void*,void*),struct Cyc_List_List*l1,struct Cyc_List_List*l2){
# 860
for(0;l1 != 0 && l2 != 0;(l1=l1->tl,l2=l2->tl)){
# 862
if((unsigned)l1 == (unsigned)l2)
return 1;{
# 862
int i=({cmp(l1->hd,l2->hd);});
# 865
if(i != 0)
return 0;}}
# 868
return l1 == 0;}
# 871
struct Cyc_List_List*Cyc_List_rfilter_c(struct _RegionHandle*r,int(*f)(void*,void*),void*env,struct Cyc_List_List*l){
if(l == 0)
return 0;{
# 872
struct Cyc_List_List*result=({struct Cyc_List_List*_tmp89=_region_malloc(r,sizeof(*_tmp89));
# 875
_tmp89->hd=l->hd,_tmp89->tl=0;_tmp89;});
struct Cyc_List_List*end=result;
struct Cyc_List_List*temp;
for(0;l != 0;l=l->tl){
if(({f(env,l->hd);})){
temp=({struct Cyc_List_List*_tmp88=_region_malloc(r,sizeof(*_tmp88));_tmp88->hd=l->hd,_tmp88->tl=0;_tmp88;});
end->tl=temp;
end=temp;}}
# 885
return result->tl;}}
# 888
struct Cyc_List_List*Cyc_List_filter_c(int(*f)(void*,void*),void*env,struct Cyc_List_List*l){
return({Cyc_List_rfilter_c(Cyc_Core_heap_region,f,env,l);});}
# 892
struct Cyc_List_List*Cyc_List_rfilter(struct _RegionHandle*r,int(*f)(void*),struct Cyc_List_List*l){
if(l == 0)
return 0;{
# 893
struct Cyc_List_List*result=({struct Cyc_List_List*_tmp8D=_region_malloc(r,sizeof(*_tmp8D));
# 896
_tmp8D->hd=l->hd,_tmp8D->tl=0;_tmp8D;});
struct Cyc_List_List*end=result;
struct Cyc_List_List*temp;
for(0;l != 0;l=l->tl){
if(({f(l->hd);})){
temp=({struct Cyc_List_List*_tmp8C=_region_malloc(r,sizeof(*_tmp8C));_tmp8C->hd=l->hd,_tmp8C->tl=0;_tmp8C;});
end->tl=temp;
end=temp;}}
# 906
return result->tl;}}
# 909
struct Cyc_List_List*Cyc_List_filter(int(*f)(void*),struct Cyc_List_List*l){
return({Cyc_List_rfilter(Cyc_Core_heap_region,f,l);});}

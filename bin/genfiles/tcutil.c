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
 struct Cyc_Core_Opt{void*v;};
# 119 "core.h"
int Cyc_Core_intcmp(int,int);extern char Cyc_Core_Invalid_argument[17U];struct Cyc_Core_Invalid_argument_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Failure[8U];struct Cyc_Core_Failure_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Impossible[11U];struct Cyc_Core_Impossible_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Not_found[10U];struct Cyc_Core_Not_found_exn_struct{char*tag;};extern char Cyc_Core_Unreachable[12U];struct Cyc_Core_Unreachable_exn_struct{char*tag;struct _fat_ptr f1;};
# 165
extern struct _RegionHandle*Cyc_Core_heap_region;
# 168
extern struct _RegionHandle*Cyc_Core_unique_region;struct Cyc_Core_DynamicRegion;struct Cyc_Core_NewDynamicRegion{struct Cyc_Core_DynamicRegion*key;};struct Cyc_Core_ThinRes{void*arr;unsigned nelts;};struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};
# 54 "list.h"
struct Cyc_List_List*Cyc_List_list(struct _fat_ptr);
# 61
int Cyc_List_length(struct Cyc_List_List*x);
# 76
struct Cyc_List_List*Cyc_List_map(void*(*f)(void*),struct Cyc_List_List*x);
# 83
struct Cyc_List_List*Cyc_List_map_c(void*(*f)(void*,void*),void*env,struct Cyc_List_List*x);
# 86
struct Cyc_List_List*Cyc_List_rmap_c(struct _RegionHandle*,void*(*f)(void*,void*),void*env,struct Cyc_List_List*x);extern char Cyc_List_List_mismatch[14U];struct Cyc_List_List_mismatch_exn_struct{char*tag;};
# 94
struct Cyc_List_List*Cyc_List_map2(void*(*f)(void*,void*),struct Cyc_List_List*x,struct Cyc_List_List*y);
# 133
void Cyc_List_iter(void(*f)(void*),struct Cyc_List_List*x);
# 135
void Cyc_List_iter_c(void(*f)(void*,void*),void*env,struct Cyc_List_List*x);
# 161
struct Cyc_List_List*Cyc_List_revappend(struct Cyc_List_List*x,struct Cyc_List_List*y);
# 178
struct Cyc_List_List*Cyc_List_imp_rev(struct Cyc_List_List*x);
# 205
struct Cyc_List_List*Cyc_List_rflatten(struct _RegionHandle*,struct Cyc_List_List*x);extern char Cyc_List_Nth[4U];struct Cyc_List_Nth_exn_struct{char*tag;};
# 261
int Cyc_List_exists_c(int(*pred)(void*,void*),void*env,struct Cyc_List_List*x);
# 270
struct Cyc_List_List*Cyc_List_zip(struct Cyc_List_List*x,struct Cyc_List_List*y);
# 276
struct Cyc_List_List*Cyc_List_rzip(struct _RegionHandle*r1,struct _RegionHandle*r2,struct Cyc_List_List*x,struct Cyc_List_List*y);struct _tuple0{struct Cyc_List_List*f1;struct Cyc_List_List*f2;};
# 303
struct _tuple0 Cyc_List_rsplit(struct _RegionHandle*r1,struct _RegionHandle*r2,struct Cyc_List_List*x);
# 322
int Cyc_List_mem(int(*compare)(void*,void*),struct Cyc_List_List*l,void*x);
# 336
void*Cyc_List_assoc_cmp(int(*cmp)(void*,void*),struct Cyc_List_List*l,void*x);
# 383
int Cyc_List_list_cmp(int(*cmp)(void*,void*),struct Cyc_List_List*l1,struct Cyc_List_List*l2);struct Cyc___cycFILE;
# 51 "cycboot.h"
extern struct Cyc___cycFILE*Cyc_stdout;struct Cyc_String_pa_PrintArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Int_pa_PrintArg_struct{int tag;unsigned long f1;};struct Cyc_Double_pa_PrintArg_struct{int tag;double f1;};struct Cyc_LongDouble_pa_PrintArg_struct{int tag;long double f1;};struct Cyc_ShortPtr_pa_PrintArg_struct{int tag;short*f1;};struct Cyc_IntPtr_pa_PrintArg_struct{int tag;unsigned long*f1;};
# 73
struct _fat_ptr Cyc_aprintf(struct _fat_ptr,struct _fat_ptr);
# 88
int Cyc_fflush(struct Cyc___cycFILE*);
# 100
int Cyc_fprintf(struct Cyc___cycFILE*,struct _fat_ptr,struct _fat_ptr);struct Cyc_ShortPtr_sa_ScanfArg_struct{int tag;short*f1;};struct Cyc_UShortPtr_sa_ScanfArg_struct{int tag;unsigned short*f1;};struct Cyc_IntPtr_sa_ScanfArg_struct{int tag;int*f1;};struct Cyc_UIntPtr_sa_ScanfArg_struct{int tag;unsigned*f1;};struct Cyc_StringPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_DoublePtr_sa_ScanfArg_struct{int tag;double*f1;};struct Cyc_FloatPtr_sa_ScanfArg_struct{int tag;float*f1;};struct Cyc_CharPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};extern char Cyc_FileCloseError[15U];struct Cyc_FileCloseError_exn_struct{char*tag;};extern char Cyc_FileOpenError[14U];struct Cyc_FileOpenError_exn_struct{char*tag;struct _fat_ptr f1;};
# 38 "string.h"
unsigned long Cyc_strlen(struct _fat_ptr s);
# 49 "string.h"
int Cyc_strcmp(struct _fat_ptr s1,struct _fat_ptr s2);
int Cyc_strptrcmp(struct _fat_ptr*s1,struct _fat_ptr*s2);struct Cyc_PP_Ppstate;struct Cyc_PP_Out;struct Cyc_PP_Doc;struct Cyc_Position_Error;struct Cyc_Iter_Iter{void*env;int(*next)(void*env,void*dest);};struct Cyc_Dict_T;struct Cyc_Dict_Dict{int(*rel)(void*,void*);struct _RegionHandle*r;const struct Cyc_Dict_T*t;};extern char Cyc_Dict_Present[8U];struct Cyc_Dict_Present_exn_struct{char*tag;};extern char Cyc_Dict_Absent[7U];struct Cyc_Dict_Absent_exn_struct{char*tag;};struct Cyc_Relations_Reln;struct _union_Nmspace_Rel_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Abs_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_C_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Loc_n{int tag;int val;};union Cyc_Absyn_Nmspace{struct _union_Nmspace_Rel_n Rel_n;struct _union_Nmspace_Abs_n Abs_n;struct _union_Nmspace_C_n C_n;struct _union_Nmspace_Loc_n Loc_n;};
# 110 "absyn.h"
extern union Cyc_Absyn_Nmspace Cyc_Absyn_Loc_n;struct _tuple1{union Cyc_Absyn_Nmspace f1;struct _fat_ptr*f2;};
# 184
enum Cyc_Absyn_DefExn{Cyc_Absyn_De_NullCheck =0U,Cyc_Absyn_De_BoundsCheck =1U,Cyc_Absyn_De_PatternCheck =2U,Cyc_Absyn_De_AllocCheck =3U};
# 194
enum Cyc_Absyn_Scope{Cyc_Absyn_Static =0U,Cyc_Absyn_Abstract =1U,Cyc_Absyn_Public =2U,Cyc_Absyn_Extern =3U,Cyc_Absyn_ExternC =4U,Cyc_Absyn_Register =5U};struct Cyc_Absyn_Tqual{int print_const: 1;int q_volatile: 1;int q_restrict: 1;int real_const: 1;unsigned loc;};
# 215
enum Cyc_Absyn_Size_of{Cyc_Absyn_Char_sz =0U,Cyc_Absyn_Short_sz =1U,Cyc_Absyn_Int_sz =2U,Cyc_Absyn_Long_sz =3U,Cyc_Absyn_LongLong_sz =4U};
enum Cyc_Absyn_Sign{Cyc_Absyn_Signed =0U,Cyc_Absyn_Unsigned =1U,Cyc_Absyn_None =2U};
enum Cyc_Absyn_AggrKind{Cyc_Absyn_StructA =0U,Cyc_Absyn_UnionA =1U};
# 220
enum Cyc_Absyn_AliasQual{Cyc_Absyn_Aliasable =0U,Cyc_Absyn_Unique =1U,Cyc_Absyn_Top =2U};
# 225
enum Cyc_Absyn_KindQual{Cyc_Absyn_AnyKind =0U,Cyc_Absyn_MemKind =1U,Cyc_Absyn_BoxKind =2U,Cyc_Absyn_XRgnKind =3U,Cyc_Absyn_RgnKind =4U,Cyc_Absyn_EffKind =5U,Cyc_Absyn_IntKind =6U,Cyc_Absyn_BoolKind =7U,Cyc_Absyn_PtrBndKind =8U};struct Cyc_Absyn_Kind{enum Cyc_Absyn_KindQual kind;enum Cyc_Absyn_AliasQual aliasqual;};struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct{int tag;struct Cyc_Absyn_Kind*f1;};struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct{int tag;struct Cyc_Core_Opt*f1;};struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct{int tag;struct Cyc_Core_Opt*f1;struct Cyc_Absyn_Kind*f2;};struct Cyc_Absyn_Tvar{struct _fat_ptr*name;int identity;void*kind;};struct Cyc_Absyn_PtrLoc{unsigned ptr_loc;unsigned rgn_loc;unsigned zt_loc;};struct Cyc_Absyn_PtrAtts{void*rgn;void*nullable;void*bounds;void*zero_term;struct Cyc_Absyn_PtrLoc*ptrloc;};struct Cyc_Absyn_PtrInfo{void*elt_type;struct Cyc_Absyn_Tqual elt_tq;struct Cyc_Absyn_PtrAtts ptr_atts;};struct Cyc_Absyn_VarargInfo{struct _fat_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;int inject;};struct Cyc_Absyn_Star_Absyn_Cap_struct{int tag;};struct Cyc_Absyn_Bot_Absyn_Cap_struct{int tag;};struct Cyc_Absyn_Nat_Absyn_Cap_struct{int tag;int f1;int f2;};struct Cyc_Absyn_RgnEffect{void*name;struct Cyc_List_List*caps;void*parent;};struct Cyc_Absyn_FnInfo{struct Cyc_List_List*tvars;void*effect;struct Cyc_Absyn_Tqual ret_tqual;void*ret_type;struct Cyc_List_List*args;int c_varargs;struct Cyc_Absyn_VarargInfo*cyc_varargs;struct Cyc_List_List*rgn_po;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*requires_clause;struct Cyc_List_List*requires_relns;struct Cyc_Absyn_Exp*ensures_clause;struct Cyc_List_List*ensures_relns;struct Cyc_List_List*ieffect;struct Cyc_List_List*oeffect;struct Cyc_List_List*throws;int reentrant;};struct Cyc_Absyn_UnknownDatatypeInfo{struct _tuple1*name;int is_extensible;};struct _union_DatatypeInfo_UnknownDatatype{int tag;struct Cyc_Absyn_UnknownDatatypeInfo val;};struct _union_DatatypeInfo_KnownDatatype{int tag;struct Cyc_Absyn_Datatypedecl**val;};union Cyc_Absyn_DatatypeInfo{struct _union_DatatypeInfo_UnknownDatatype UnknownDatatype;struct _union_DatatypeInfo_KnownDatatype KnownDatatype;};
# 346
union Cyc_Absyn_DatatypeInfo Cyc_Absyn_UnknownDatatype(struct Cyc_Absyn_UnknownDatatypeInfo);struct Cyc_Absyn_UnknownDatatypeFieldInfo{struct _tuple1*datatype_name;struct _tuple1*field_name;int is_extensible;};struct _union_DatatypeFieldInfo_UnknownDatatypefield{int tag;struct Cyc_Absyn_UnknownDatatypeFieldInfo val;};struct _tuple2{struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;};struct _union_DatatypeFieldInfo_KnownDatatypefield{int tag;struct _tuple2 val;};union Cyc_Absyn_DatatypeFieldInfo{struct _union_DatatypeFieldInfo_UnknownDatatypefield UnknownDatatypefield;struct _union_DatatypeFieldInfo_KnownDatatypefield KnownDatatypefield;};struct _tuple3{enum Cyc_Absyn_AggrKind f1;struct _tuple1*f2;struct Cyc_Core_Opt*f3;};struct _union_AggrInfo_UnknownAggr{int tag;struct _tuple3 val;};struct _union_AggrInfo_KnownAggr{int tag;struct Cyc_Absyn_Aggrdecl**val;};union Cyc_Absyn_AggrInfo{struct _union_AggrInfo_UnknownAggr UnknownAggr;struct _union_AggrInfo_KnownAggr KnownAggr;};
# 366
union Cyc_Absyn_AggrInfo Cyc_Absyn_UnknownAggr(enum Cyc_Absyn_AggrKind,struct _tuple1*,struct Cyc_Core_Opt*);struct Cyc_Absyn_ArrayInfo{void*elt_type;struct Cyc_Absyn_Tqual tq;struct Cyc_Absyn_Exp*num_elts;void*zero_term;unsigned zt_loc;};struct Cyc_Absyn_Aggr_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Enum_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;};struct Cyc_Absyn_Datatype_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;};struct Cyc_Absyn_TypeDecl{void*r;unsigned loc;};struct Cyc_Absyn_VoidCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_IntCon_Absyn_TyCon_struct{int tag;enum Cyc_Absyn_Sign f1;enum Cyc_Absyn_Size_of f2;};struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct{int tag;int f1;};struct Cyc_Absyn_RgnHandleCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_XRgnHandleCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_TagCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_HeapCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_UniqueCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_RefCntCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_AccessCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_CAccessCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_JoinCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_RgnsCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_TrueCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_FalseCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_ThinCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_FatCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct{int tag;struct _tuple1*f1;struct Cyc_Absyn_Enumdecl*f2;};struct Cyc_Absyn_AnonEnumCon_Absyn_TyCon_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_BuiltinCon_Absyn_TyCon_struct{int tag;struct _fat_ptr f1;struct Cyc_Absyn_Kind*f2;};struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_DatatypeInfo f1;};struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_DatatypeFieldInfo f1;};struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_AggrInfo f1;};struct Cyc_Absyn_AppType_Absyn_Type_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Evar_Absyn_Type_struct{int tag;struct Cyc_Core_Opt*f1;void*f2;int f3;struct Cyc_Core_Opt*f4;};struct Cyc_Absyn_VarType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Tvar*f1;};struct Cyc_Absyn_PointerType_Absyn_Type_struct{int tag;struct Cyc_Absyn_PtrInfo f1;};struct Cyc_Absyn_ArrayType_Absyn_Type_struct{int tag;struct Cyc_Absyn_ArrayInfo f1;};struct Cyc_Absyn_FnType_Absyn_Type_struct{int tag;struct Cyc_Absyn_FnInfo f1;};struct Cyc_Absyn_TupleType_Absyn_Type_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct{int tag;enum Cyc_Absyn_AggrKind f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_TypedefType_Absyn_Type_struct{int tag;struct _tuple1*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_Typedefdecl*f3;void*f4;};struct Cyc_Absyn_ValueofType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct{int tag;struct Cyc_Absyn_TypeDecl*f1;void**f2;};struct Cyc_Absyn_TypeofType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_NoTypes_Absyn_Funcparams_struct{int tag;struct Cyc_List_List*f1;unsigned f2;};struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct{int tag;struct Cyc_List_List*f1;int f2;struct Cyc_Absyn_VarargInfo*f3;void*f4;struct Cyc_List_List*f5;struct Cyc_Absyn_Exp*f6;struct Cyc_Absyn_Exp*f7;struct Cyc_List_List*f8;struct Cyc_List_List*f9;struct Cyc_List_List*f10;int f11;};
# 478 "absyn.h"
enum Cyc_Absyn_Format_Type{Cyc_Absyn_Printf_ft =0U,Cyc_Absyn_Scanf_ft =1U};struct Cyc_Absyn_Regparm_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Stdcall_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Cdecl_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Fastcall_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Noreturn_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Const_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Aligned_att_Absyn_Attribute_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Packed_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Section_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Nocommon_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Shared_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Unused_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Weak_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Dllimport_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Dllexport_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_instrument_function_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Constructor_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Destructor_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_check_memory_usage_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Format_att_Absyn_Attribute_struct{int tag;enum Cyc_Absyn_Format_Type f1;int f2;int f3;};struct Cyc_Absyn_Initializes_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Noliveunique_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Consume_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Pure_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Mode_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Alias_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Always_inline_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_throw_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Non_null_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Deprecated_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_VectorSize_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Carray_mod_Absyn_Type_modifier_struct{int tag;void*f1;unsigned f2;};struct Cyc_Absyn_ConstArray_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;unsigned f3;};struct Cyc_Absyn_Pointer_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_Absyn_PtrAtts f1;struct Cyc_Absyn_Tqual f2;};struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct{int tag;void*f1;};struct Cyc_Absyn_TypeParams_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_List_List*f1;unsigned f2;int f3;};struct Cyc_Absyn_Attributes_mod_Absyn_Type_modifier_struct{int tag;unsigned f1;struct Cyc_List_List*f2;};struct _union_Cnst_Null_c{int tag;int val;};struct _tuple4{enum Cyc_Absyn_Sign f1;char f2;};struct _union_Cnst_Char_c{int tag;struct _tuple4 val;};struct _union_Cnst_Wchar_c{int tag;struct _fat_ptr val;};struct _tuple5{enum Cyc_Absyn_Sign f1;short f2;};struct _union_Cnst_Short_c{int tag;struct _tuple5 val;};struct _tuple6{enum Cyc_Absyn_Sign f1;int f2;};struct _union_Cnst_Int_c{int tag;struct _tuple6 val;};struct _tuple7{enum Cyc_Absyn_Sign f1;long long f2;};struct _union_Cnst_LongLong_c{int tag;struct _tuple7 val;};struct _tuple8{struct _fat_ptr f1;int f2;};struct _union_Cnst_Float_c{int tag;struct _tuple8 val;};struct _union_Cnst_String_c{int tag;struct _fat_ptr val;};struct _union_Cnst_Wstring_c{int tag;struct _fat_ptr val;};union Cyc_Absyn_Cnst{struct _union_Cnst_Null_c Null_c;struct _union_Cnst_Char_c Char_c;struct _union_Cnst_Wchar_c Wchar_c;struct _union_Cnst_Short_c Short_c;struct _union_Cnst_Int_c Int_c;struct _union_Cnst_LongLong_c LongLong_c;struct _union_Cnst_Float_c Float_c;struct _union_Cnst_String_c String_c;struct _union_Cnst_Wstring_c Wstring_c;};
# 574
enum Cyc_Absyn_Primop{Cyc_Absyn_Plus =0U,Cyc_Absyn_Times =1U,Cyc_Absyn_Minus =2U,Cyc_Absyn_Div =3U,Cyc_Absyn_Mod =4U,Cyc_Absyn_Eq =5U,Cyc_Absyn_Neq =6U,Cyc_Absyn_Gt =7U,Cyc_Absyn_Lt =8U,Cyc_Absyn_Gte =9U,Cyc_Absyn_Lte =10U,Cyc_Absyn_Not =11U,Cyc_Absyn_Bitnot =12U,Cyc_Absyn_Bitand =13U,Cyc_Absyn_Bitor =14U,Cyc_Absyn_Bitxor =15U,Cyc_Absyn_Bitlshift =16U,Cyc_Absyn_Bitlrshift =17U,Cyc_Absyn_Numelts =18U};
# 581
enum Cyc_Absyn_Incrementor{Cyc_Absyn_PreInc =0U,Cyc_Absyn_PostInc =1U,Cyc_Absyn_PreDec =2U,Cyc_Absyn_PostDec =3U};struct Cyc_Absyn_VarargCallInfo{int num_varargs;struct Cyc_List_List*injectors;struct Cyc_Absyn_VarargInfo*vai;};struct Cyc_Absyn_StructField_Absyn_OffsetofField_struct{int tag;struct _fat_ptr*f1;};struct Cyc_Absyn_TupleIndex_Absyn_OffsetofField_struct{int tag;unsigned f1;};
# 599
enum Cyc_Absyn_Coercion{Cyc_Absyn_Unknown_coercion =0U,Cyc_Absyn_No_coercion =1U,Cyc_Absyn_Null_to_NonNull =2U,Cyc_Absyn_Other_coercion =3U};struct Cyc_Absyn_MallocInfo{int is_calloc;struct Cyc_Absyn_Exp*rgn;void**elt_type;struct Cyc_Absyn_Exp*num_elts;int fat_result;int inline_call;};struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct{int tag;union Cyc_Absyn_Cnst f1;};struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Pragma_e_Absyn_Raw_exp_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct{int tag;enum Cyc_Absyn_Primop f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;enum Cyc_Absyn_Incrementor f2;};struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_VarargCallInfo*f3;int f4;struct _tuple0*f5;};struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;int f2;};struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Exp*f2;int f3;enum Cyc_Absyn_Coercion f4;};struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;int f3;int f4;};struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;int f3;int f4;};struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Tuple_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;};struct _tuple9{struct _fat_ptr*f1;struct Cyc_Absyn_Tqual f2;void*f3;};struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct{int tag;struct _tuple9*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;int f4;};struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;int f3;};struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct{int tag;struct _tuple1*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct Cyc_Absyn_Aggrdecl*f4;};struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Datatypedecl*f2;struct Cyc_Absyn_Datatypefield*f3;};struct Cyc_Absyn_Enum_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_Spawn_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;struct _tuple0*f3;};struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_MallocInfo f1;};struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Core_Opt*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Stmt*f1;};struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;};struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct{int tag;int f1;struct _fat_ptr f2;struct Cyc_List_List*f3;struct Cyc_List_List*f4;struct Cyc_List_List*f5;};struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Exp{void*topt;void*r;unsigned loc;void*annot;};struct Cyc_Absyn_Skip_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;struct Cyc_Absyn_Stmt*f3;};struct _tuple10{struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct{int tag;struct _tuple10 f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Break_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Continue_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Goto_s_Absyn_Raw_stmt_struct{int tag;struct _fat_ptr*f1;};struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _tuple10 f2;struct _tuple10 f3;struct Cyc_Absyn_Stmt*f4;};struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;void*f3;};struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Switch_clause**f2;};struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Decl*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct{int tag;struct _fat_ptr*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct _tuple10 f2;};struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_List_List*f2;void*f3;};struct Cyc_Absyn_Stmt{void*r;unsigned loc;void*annot;};struct Cyc_Absyn_Wild_p_Absyn_Raw_pat_struct{int tag;};struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_AliasVar_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_TagInt_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Tuple_p_Absyn_Raw_pat_struct{int tag;struct Cyc_List_List*f1;int f2;};struct Cyc_Absyn_Pointer_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Pat*f1;};struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct{int tag;union Cyc_Absyn_AggrInfo*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;int f4;};struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;struct Cyc_List_List*f3;int f4;};struct Cyc_Absyn_Null_p_Absyn_Raw_pat_struct{int tag;};struct Cyc_Absyn_Int_p_Absyn_Raw_pat_struct{int tag;enum Cyc_Absyn_Sign f1;int f2;};struct Cyc_Absyn_Char_p_Absyn_Raw_pat_struct{int tag;char f1;};struct Cyc_Absyn_Float_p_Absyn_Raw_pat_struct{int tag;struct _fat_ptr f1;int f2;};struct Cyc_Absyn_Enum_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_AnonEnum_p_Absyn_Raw_pat_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_UnknownId_p_Absyn_Raw_pat_struct{int tag;struct _tuple1*f1;};struct Cyc_Absyn_UnknownCall_p_Absyn_Raw_pat_struct{int tag;struct _tuple1*f1;struct Cyc_List_List*f2;int f3;};struct Cyc_Absyn_Exp_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Pat{void*r;void*topt;unsigned loc;};struct Cyc_Absyn_Switch_clause{struct Cyc_Absyn_Pat*pattern;struct Cyc_Core_Opt*pat_vars;struct Cyc_Absyn_Exp*where_clause;struct Cyc_Absyn_Stmt*body;unsigned loc;};struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct{int tag;struct _tuple1*f1;};struct Cyc_Absyn_Global_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Funname_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Param_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Local_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Pat_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Vardecl{enum Cyc_Absyn_Scope sc;struct _tuple1*name;unsigned varloc;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*initializer;void*rgn;struct Cyc_List_List*attributes;int escapes;};struct Cyc_Absyn_Fndecl{enum Cyc_Absyn_Scope sc;int is_inline;struct _tuple1*name;struct Cyc_Absyn_Stmt*body;struct Cyc_Absyn_FnInfo i;void*cached_type;struct Cyc_Core_Opt*param_vardecls;struct Cyc_Absyn_Vardecl*fn_vardecl;};struct Cyc_Absyn_Aggrfield{struct _fat_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*width;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*requires_clause;};struct Cyc_Absyn_AggrdeclImpl{struct Cyc_List_List*exist_vars;struct Cyc_List_List*rgn_po;struct Cyc_List_List*fields;int tagged;};struct Cyc_Absyn_Aggrdecl{enum Cyc_Absyn_AggrKind kind;enum Cyc_Absyn_Scope sc;struct _tuple1*name;struct Cyc_List_List*tvs;struct Cyc_Absyn_AggrdeclImpl*impl;struct Cyc_List_List*attributes;int expected_mem_kind;};struct Cyc_Absyn_Datatypefield{struct _tuple1*name;struct Cyc_List_List*typs;unsigned loc;enum Cyc_Absyn_Scope sc;};struct Cyc_Absyn_Datatypedecl{enum Cyc_Absyn_Scope sc;struct _tuple1*name;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*fields;int is_extensible;};struct Cyc_Absyn_Enumfield{struct _tuple1*name;struct Cyc_Absyn_Exp*tag;unsigned loc;};struct Cyc_Absyn_Enumdecl{enum Cyc_Absyn_Scope sc;struct _tuple1*name;struct Cyc_Core_Opt*fields;};struct Cyc_Absyn_Typedefdecl{struct _tuple1*name;struct Cyc_Absyn_Tqual tq;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*kind;void*defn;struct Cyc_List_List*atts;int extern_c;};struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Pat*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;void*f4;};struct Cyc_Absyn_Letv_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;};struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;};struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Typedefdecl*f1;};struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct{int tag;struct _fat_ptr*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct{int tag;struct _tuple1*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_ExternC_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;};struct _tuple11{unsigned f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct _tuple11*f4;};struct Cyc_Absyn_Porton_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Portoff_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Tempeston_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Tempestoff_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Decl{void*r;unsigned loc;};struct Cyc_Absyn_ArrayElement_Absyn_Designator_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_FieldName_Absyn_Designator_struct{int tag;struct _fat_ptr*f1;};extern char Cyc_Absyn_EmptyAnnot[11U];struct Cyc_Absyn_EmptyAnnot_Absyn_AbsynAnnot_struct{char*tag;};
# 954 "absyn.h"
int Cyc_Absyn_qvar_cmp(struct _tuple1*,struct _tuple1*);
# 956
int Cyc_Absyn_tvar_cmp(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*);
# 963
struct Cyc_Absyn_Tqual Cyc_Absyn_empty_tqual(unsigned);
# 968
void*Cyc_Absyn_compress_kb(void*);
# 974
int Cyc_Absyn_type2bool(int def,void*);
# 979
void*Cyc_Absyn_new_evar(struct Cyc_Core_Opt*k,struct Cyc_Core_Opt*tenv);
# 984
extern void*Cyc_Absyn_uint_type;extern void*Cyc_Absyn_ulong_type;extern void*Cyc_Absyn_ulonglong_type;
# 986
extern void*Cyc_Absyn_sint_type;extern void*Cyc_Absyn_slong_type;extern void*Cyc_Absyn_slonglong_type;
# 991
extern void*Cyc_Absyn_heap_rgn_type;extern void*Cyc_Absyn_unique_rgn_type;extern void*Cyc_Absyn_refcnt_rgn_type;
# 993
extern void*Cyc_Absyn_empty_effect;
# 995
extern void*Cyc_Absyn_true_type;extern void*Cyc_Absyn_false_type;
# 997
extern void*Cyc_Absyn_void_type;void*Cyc_Absyn_var_type(struct Cyc_Absyn_Tvar*);void*Cyc_Absyn_access_eff(void*);void*Cyc_Absyn_join_eff(struct Cyc_List_List*);void*Cyc_Absyn_regionsof_eff(void*);void*Cyc_Absyn_enum_type(struct _tuple1*n,struct Cyc_Absyn_Enumdecl*d);
# 1022
extern void*Cyc_Absyn_fat_bound_type;
# 1024
void*Cyc_Absyn_thin_bounds_exp(struct Cyc_Absyn_Exp*);
# 1026
void*Cyc_Absyn_bounds_one();
# 1028
void*Cyc_Absyn_pointer_type(struct Cyc_Absyn_PtrInfo);
# 1033
void*Cyc_Absyn_atb_type(void*t,void*rgn,struct Cyc_Absyn_Tqual tq,void*b,void*zero_term);
# 1053
void*Cyc_Absyn_datatype_type(union Cyc_Absyn_DatatypeInfo,struct Cyc_List_List*args);
# 1055
void*Cyc_Absyn_aggr_type(union Cyc_Absyn_AggrInfo,struct Cyc_List_List*args);
# 1058
struct Cyc_Absyn_Exp*Cyc_Absyn_new_exp(void*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_New_exp(struct Cyc_Absyn_Exp*rgn_handle,struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_copy_exp(struct Cyc_Absyn_Exp*);
struct Cyc_Absyn_Exp*Cyc_Absyn_const_exp(union Cyc_Absyn_Cnst,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_null_exp(unsigned);
# 1068
struct Cyc_Absyn_Exp*Cyc_Absyn_uint_exp(unsigned,unsigned);
# 1075
struct Cyc_Absyn_Exp*Cyc_Absyn_varb_exp(void*,unsigned);
# 1077
struct Cyc_Absyn_Exp*Cyc_Absyn_pragma_exp(struct _fat_ptr,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_primop_exp(enum Cyc_Absyn_Primop,struct Cyc_List_List*,unsigned);
# 1081
struct Cyc_Absyn_Exp*Cyc_Absyn_swap_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
# 1091
struct Cyc_Absyn_Exp*Cyc_Absyn_assignop_exp(struct Cyc_Absyn_Exp*,struct Cyc_Core_Opt*,struct Cyc_Absyn_Exp*,unsigned);
# 1093
struct Cyc_Absyn_Exp*Cyc_Absyn_increment_exp(struct Cyc_Absyn_Exp*,enum Cyc_Absyn_Incrementor,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_conditional_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_and_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_or_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_seq_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
# 1100
struct Cyc_Absyn_Exp*Cyc_Absyn_throw_exp(struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_rethrow_exp(struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_noinstantiate_exp(struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_instantiate_exp(struct Cyc_Absyn_Exp*,struct Cyc_List_List*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_cast_exp(void*,struct Cyc_Absyn_Exp*,int user_cast,enum Cyc_Absyn_Coercion,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_address_exp(struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_sizeoftype_exp(void*t,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_sizeofexp_exp(struct Cyc_Absyn_Exp*e,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_offsetof_exp(void*,struct Cyc_List_List*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_deref_exp(struct Cyc_Absyn_Exp*,unsigned);
# 1112
struct Cyc_Absyn_Exp*Cyc_Absyn_subscript_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_tuple_exp(struct Cyc_List_List*,unsigned);
# 1117
struct Cyc_Absyn_Exp*Cyc_Absyn_valueof_exp(void*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_asm_exp(int,struct _fat_ptr,struct Cyc_List_List*,struct Cyc_List_List*,struct Cyc_List_List*,unsigned);
# 1120
struct Cyc_Absyn_Exp*Cyc_Absyn_extension_exp(struct Cyc_Absyn_Exp*,unsigned);
# 1157
struct Cyc_Absyn_Decl*Cyc_Absyn_alias_decl(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Vardecl*,struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Vardecl*Cyc_Absyn_new_vardecl(unsigned varloc,struct _tuple1*,void*,struct Cyc_Absyn_Exp*init);
# 1203
struct Cyc_Absyn_Aggrfield*Cyc_Absyn_lookup_field(struct Cyc_List_List*,struct _fat_ptr*);
# 1205
struct Cyc_Absyn_Aggrfield*Cyc_Absyn_lookup_decl_field(struct Cyc_Absyn_Aggrdecl*,struct _fat_ptr*);struct _tuple12{struct Cyc_Absyn_Tqual f1;void*f2;};
# 1207
struct _tuple12*Cyc_Absyn_lookup_tuple_field(struct Cyc_List_List*,int);
# 1215
int Cyc_Absyn_fntype_att(void*);
# 1217
int Cyc_Absyn_equal_att(void*,void*);
# 1219
int Cyc_Absyn_attribute_cmp(void*,void*);
# 1225
struct Cyc_Absyn_Aggrdecl*Cyc_Absyn_get_known_aggrdecl(union Cyc_Absyn_AggrInfo);
# 1271
struct Cyc_Absyn_Exp*Cyc_Absyn_spawn_exp(struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2,struct Cyc_List_List*f1,struct Cyc_List_List*f2,unsigned loc);
# 1280
struct Cyc_Absyn_Tvar*Cyc_Absyn_type2tvar(void*t);
# 1285
int Cyc_Absyn_get_debug();
# 1350
struct Cyc_List_List*Cyc_Absyn_copy_effect(struct Cyc_List_List*f);
# 1358
struct Cyc_List_List*Cyc_Absyn_rsubstitute_effect(struct _RegionHandle*rgn,struct Cyc_List_List*inst,struct Cyc_List_List*l);
# 1394
int Cyc_Absyn_copy_reentrant(int);
int Cyc_Absyn_rsubstitute_reentrant(struct _RegionHandle*rgn,struct Cyc_List_List*inst,int re);
# 1400
extern const int Cyc_Absyn_noreentrant;struct Cyc_Absynpp_Params{int expand_typedefs;int qvar_to_Cids;int add_cyc_prefix;int to_VC;int decls_first;int rewrite_temp_tvars;int print_all_tvars;int print_all_kinds;int print_all_effects;int print_using_stmts;int print_externC_stmts;int print_full_evars;int print_zeroterm;int generate_line_directives;int use_curr_namespace;struct Cyc_List_List*curr_namespace;};
# 62 "absynpp.h"
struct _fat_ptr Cyc_Absynpp_typ2string(void*);
# 64
struct _fat_ptr Cyc_Absynpp_kind2string(struct Cyc_Absyn_Kind*);
# 67
struct _fat_ptr Cyc_Absynpp_exp2string(struct Cyc_Absyn_Exp*);
# 74
struct _fat_ptr Cyc_Absynpp_tvar2string(struct Cyc_Absyn_Tvar*);
# 27 "warn.h"
void Cyc_Warn_vwarn(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap);
# 33
void Cyc_Warn_verr(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap);
# 38
void*Cyc_Warn_vimpos(struct _fat_ptr fmt,struct _fat_ptr ap);struct Cyc_Warn_String_Warn_Warg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Warn_Exp_Warn_Warg_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Warn_Typ_Warn_Warg_struct{int tag;void*f1;};struct Cyc_Warn_Qvar_Warn_Warg_struct{int tag;struct _tuple1*f1;};struct _tuple13{unsigned f1;int f2;};
# 28 "evexp.h"
struct _tuple13 Cyc_Evexp_eval_const_uint_exp(struct Cyc_Absyn_Exp*e);
# 41 "evexp.h"
int Cyc_Evexp_same_const_exp(struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2);
int Cyc_Evexp_lte_const_exp(struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2);
# 45
int Cyc_Evexp_const_exp_cmp(struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2);
# 28 "unify.h"
int Cyc_Unify_unify_kindbound(void*,void*);
int Cyc_Unify_unify(void*,void*);
# 32
void Cyc_Unify_occurs(void*evar,struct _RegionHandle*r,struct Cyc_List_List*env,void*t);struct _union_RelnOp_RConst{int tag;unsigned val;};struct _union_RelnOp_RVar{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RNumelts{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RType{int tag;void*val;};struct _union_RelnOp_RParam{int tag;unsigned val;};struct _union_RelnOp_RParamNumelts{int tag;unsigned val;};struct _union_RelnOp_RReturn{int tag;unsigned val;};union Cyc_Relations_RelnOp{struct _union_RelnOp_RConst RConst;struct _union_RelnOp_RVar RVar;struct _union_RelnOp_RNumelts RNumelts;struct _union_RelnOp_RType RType;struct _union_RelnOp_RParam RParam;struct _union_RelnOp_RParamNumelts RParamNumelts;struct _union_RelnOp_RReturn RReturn;};
# 41 "relations-ap.h"
union Cyc_Relations_RelnOp Cyc_Relations_RParam(unsigned);union Cyc_Relations_RelnOp Cyc_Relations_RParamNumelts(unsigned);union Cyc_Relations_RelnOp Cyc_Relations_RReturn();
# 50
enum Cyc_Relations_Relation{Cyc_Relations_Req =0U,Cyc_Relations_Rneq =1U,Cyc_Relations_Rlte =2U,Cyc_Relations_Rlt =3U};struct Cyc_Relations_Reln{union Cyc_Relations_RelnOp rop1;enum Cyc_Relations_Relation relation;union Cyc_Relations_RelnOp rop2;};
# 84
struct Cyc_List_List*Cyc_Relations_exp2relns(struct _RegionHandle*r,struct Cyc_Absyn_Exp*e);
# 110
struct Cyc_List_List*Cyc_Relations_copy_relns(struct _RegionHandle*,struct Cyc_List_List*);
# 131
int Cyc_Relations_check_logical_implication(struct Cyc_List_List*r1,struct Cyc_List_List*r2);struct Cyc_RgnOrder_RgnPO;extern char Cyc_Tcenv_Env_error[10U];struct Cyc_Tcenv_Env_error_exn_struct{char*tag;};struct Cyc_Tcenv_Genv{struct Cyc_Dict_Dict aggrdecls;struct Cyc_Dict_Dict datatypedecls;struct Cyc_Dict_Dict enumdecls;struct Cyc_Dict_Dict typedefs;struct Cyc_Dict_Dict ordinaries;};struct Cyc_Tcenv_Fenv;struct Cyc_Tcenv_Tenv{struct Cyc_List_List*ns;struct Cyc_Tcenv_Genv*ae;struct Cyc_Tcenv_Fenv*le;int allow_valueof: 1;int in_extern_c_include: 1;int in_tempest: 1;int tempest_generalize: 1;};
# 90 "tcenv.h"
enum Cyc_Tcenv_NewStatus{Cyc_Tcenv_NoneNew =0U,Cyc_Tcenv_InNew =1U,Cyc_Tcenv_InNewAggr =2U};
# 153
int Cyc_Tcenv_rgn_outlives_rgn(struct Cyc_Tcenv_Tenv*te,void*rgn1,void*rgn2);
# 30 "tcutil.h"
void*Cyc_Tcutil_impos(struct _fat_ptr,struct _fat_ptr);
void Cyc_Tcutil_terr(unsigned,struct _fat_ptr,struct _fat_ptr);
void Cyc_Tcutil_warn(unsigned,struct _fat_ptr,struct _fat_ptr);
# 40
int Cyc_Tcutil_is_char_type(void*);
int Cyc_Tcutil_is_any_int_type(void*);
int Cyc_Tcutil_is_any_float_type(void*);
int Cyc_Tcutil_is_integral_type(void*);
int Cyc_Tcutil_is_arithmetic_type(void*);
# 47
int Cyc_Tcutil_is_pointer_type(void*);
# 60
int Cyc_Tcutil_is_bits_only_type(void*);
# 62
int Cyc_Tcutil_is_noreturn_fn_type(void*);
# 75
struct Cyc_Absyn_Exp*Cyc_Tcutil_get_bounds_exp(void*def,void*b);
# 80
int Cyc_Tcutil_is_fat_pointer_type_elt(void*t,void**elt_dest);
# 84
int Cyc_Tcutil_is_zero_ptr_type(void*t,void**ptr_type,int*is_fat,void**elt_type);
# 87
struct Cyc_Absyn_Exp*Cyc_Tcutil_get_bounds_exp(void*def,void*b);
# 90
int Cyc_Tcutil_is_integral(struct Cyc_Absyn_Exp*);
# 92
int Cyc_Tcutil_is_zero(struct Cyc_Absyn_Exp*);
# 97
void*Cyc_Tcutil_copy_type(void*);
# 100
struct Cyc_Absyn_Exp*Cyc_Tcutil_deep_copy_exp(int preserve_types,struct Cyc_Absyn_Exp*);
# 103
int Cyc_Tcutil_kind_leq(struct Cyc_Absyn_Kind*,struct Cyc_Absyn_Kind*);
# 108
struct Cyc_Absyn_Kind*Cyc_Tcutil_tvar_kind(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Kind*def);
struct Cyc_Absyn_Kind*Cyc_Tcutil_type_kind(void*);
void*Cyc_Tcutil_compress(void*);
void Cyc_Tcutil_unchecked_cast(struct Cyc_Absyn_Exp*,void*,enum Cyc_Absyn_Coercion);
# 113
int Cyc_Tcutil_coerce_sint_type(struct Cyc_Absyn_Exp*);
# 116
int Cyc_Tcutil_coerce_arg(struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Exp*,void*,int*alias_coercion);
int Cyc_Tcutil_coerce_assign(struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Exp*,void*);
# 120
int Cyc_Tcutil_silent_castable(struct Cyc_Tcenv_Tenv*te,unsigned,void*,void*);
# 122
enum Cyc_Absyn_Coercion Cyc_Tcutil_castable(struct Cyc_Tcenv_Tenv*te,unsigned,void*,void*);
# 124
int Cyc_Tcutil_subtype(struct Cyc_Tcenv_Tenv*te,struct Cyc_List_List*assume,void*t1,void*t2);
# 127
int Cyc_Tcutil_zero_to_null(void*,struct Cyc_Absyn_Exp*);
# 132
extern int Cyc_Tcutil_warn_alias_coerce;
# 135
extern int Cyc_Tcutil_warn_region_coerce;
# 138
extern struct Cyc_Absyn_Kind Cyc_Tcutil_xrk;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_rk;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_ak;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_bk;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_mk;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_ek;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_ik;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_boolk;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_ptrbk;
# 148
extern struct Cyc_Absyn_Kind Cyc_Tcutil_trk;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_tak;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_tbk;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_tmk;
# 153
extern struct Cyc_Absyn_Kind Cyc_Tcutil_urk;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_uak;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_ubk;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_umk;
# 158
extern struct Cyc_Core_Opt Cyc_Tcutil_rko;
extern struct Cyc_Core_Opt Cyc_Tcutil_ako;
extern struct Cyc_Core_Opt Cyc_Tcutil_bko;
extern struct Cyc_Core_Opt Cyc_Tcutil_mko;
extern struct Cyc_Core_Opt Cyc_Tcutil_iko;
extern struct Cyc_Core_Opt Cyc_Tcutil_eko;
extern struct Cyc_Core_Opt Cyc_Tcutil_boolko;
extern struct Cyc_Core_Opt Cyc_Tcutil_ptrbko;
# 167
extern struct Cyc_Core_Opt Cyc_Tcutil_trko;
extern struct Cyc_Core_Opt Cyc_Tcutil_tako;
extern struct Cyc_Core_Opt Cyc_Tcutil_tbko;
extern struct Cyc_Core_Opt Cyc_Tcutil_tmko;
# 172
extern struct Cyc_Core_Opt Cyc_Tcutil_urko;
extern struct Cyc_Core_Opt Cyc_Tcutil_uako;
extern struct Cyc_Core_Opt Cyc_Tcutil_ubko;
extern struct Cyc_Core_Opt Cyc_Tcutil_umko;
# 177
struct Cyc_Core_Opt*Cyc_Tcutil_kind_to_opt(struct Cyc_Absyn_Kind*k);
void*Cyc_Tcutil_kind_to_bound(struct Cyc_Absyn_Kind*k);
# 186
int Cyc_Tcutil_typecmp(void*,void*);
int Cyc_Tcutil_aggrfield_cmp(struct Cyc_Absyn_Aggrfield*,struct Cyc_Absyn_Aggrfield*);
# 189
void*Cyc_Tcutil_substitute(struct Cyc_List_List*,void*);
# 191
void*Cyc_Tcutil_rsubstitute(struct _RegionHandle*,struct Cyc_List_List*,void*);
struct Cyc_List_List*Cyc_Tcutil_rsubst_rgnpo(struct _RegionHandle*,struct Cyc_List_List*,struct Cyc_List_List*);
# 197
struct Cyc_Absyn_Exp*Cyc_Tcutil_rsubsexp(struct _RegionHandle*,struct Cyc_List_List*,struct Cyc_Absyn_Exp*);
# 200
int Cyc_Tcutil_subset_effect(int may_constrain_evars,void*e1,void*e2);
# 204
int Cyc_Tcutil_region_in_effect(int constrain,void*r,void*e);
# 224
void Cyc_Tcutil_check_bound(unsigned,unsigned i,void*,int do_warn);
# 237
int Cyc_Tcutil_is_noalias_region(void*r,int must_be_unique);
# 240
int Cyc_Tcutil_is_noalias_pointer(void*t,int must_be_unique);
# 245
int Cyc_Tcutil_is_noalias_path(struct Cyc_Absyn_Exp*);
# 250
int Cyc_Tcutil_is_noalias_pointer_or_aggr(void*);struct _tuple14{int f1;void*f2;};
# 254
struct _tuple14 Cyc_Tcutil_addressof_props(struct Cyc_Absyn_Exp*);
# 257
void*Cyc_Tcutil_normalize_effect(void*e);
# 261
struct Cyc_Absyn_Tvar*Cyc_Tcutil_new_tvar(void*);
# 263
int Cyc_Tcutil_new_tvar_id();
# 265
void Cyc_Tcutil_add_tvar_identity(struct Cyc_Absyn_Tvar*);
# 278
int Cyc_Tcutil_is_const_exp(struct Cyc_Absyn_Exp*);
# 285
int Cyc_Tcutil_extract_const_from_typedef(unsigned,int declared_const,void*);
# 303
void*Cyc_Tcutil_promote_array(void*t,void*rgn,int convert_tag);
# 306
int Cyc_Tcutil_zeroable_type(void*);
# 310
int Cyc_Tcutil_force_type2bool(int desired,void*);
# 318
int Cyc_Tcutil_admits_zero(void*);
void Cyc_Tcutil_replace_rops(struct Cyc_List_List*,struct Cyc_Relations_Reln*);
# 322
int Cyc_Tcutil_fast_tvar_cmp(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*);
# 327
int Cyc_Tcutil_tycon_cmp(void*,void*);
int Cyc_Tcutil_star_cmp(int(*cmp)(void*,void*),void*,void*);
void*Cyc_Tcutil_rgns_of(void*t);
# 37 "tcutil.cyc"
static void Cyc_Tcutil_check_subeffect(void*t1,void*t2);
int Cyc_Tcutil_is_pointer_effect(void*t){
# 40
void*_stmttmp0=({Cyc_Tcutil_compress(t);});void*_tmp0=_stmttmp0;void*_tmp1;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp0)->tag == 3U){_LL1: _tmp1=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp0)->f1).ptr_atts).rgn;_LL2: {void*r=_tmp1;
# 43
void*_stmttmp1=({Cyc_Tcutil_compress(r);});void*_tmp2=_stmttmp1;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2)->tag == 0U){if(((struct Cyc_Absyn_JoinCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2)->f1)->tag == 11U){_LL6: _LL7:
# 46
 return 1;}else{goto _LL8;}}else{_LL8: _LL9:
 return 0;}_LL5:;}}else{_LL3: _LL4:
# 49
 return 0;}_LL0:;}
# 38
int Cyc_Tcutil_is_void_type(void*t){
# 55
void*_stmttmp2=({Cyc_Tcutil_compress(t);});void*_tmp4=_stmttmp2;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp4)->tag == 0U){if(((struct Cyc_Absyn_VoidCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp4)->f1)->tag == 0U){_LL1: _LL2:
 return 1;}else{goto _LL3;}}else{_LL3: _LL4:
 return 0;}_LL0:;}
# 38
int Cyc_Tcutil_is_array_type(void*t){
# 61
void*_stmttmp3=({Cyc_Tcutil_compress(t);});void*_tmp6=_stmttmp3;if(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp6)->tag == 4U){_LL1: _LL2:
 return 1;}else{_LL3: _LL4:
 return 0;}_LL0:;}
# 38
int Cyc_Tcutil_is_heap_rgn_type(void*t){
# 67
void*_stmttmp4=({Cyc_Tcutil_compress(t);});void*_tmp8=_stmttmp4;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp8)->tag == 0U){if(((struct Cyc_Absyn_HeapCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp8)->f1)->tag == 6U){_LL1: _LL2:
 return 1;}else{goto _LL3;}}else{_LL3: _LL4:
 return 0;}_LL0:;}
# 38
int Cyc_Tcutil_is_pointer_type(void*t){
# 73
void*_stmttmp5=({Cyc_Tcutil_compress(t);});void*_tmpA=_stmttmp5;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmpA)->tag == 3U){_LL1: _LL2:
 return 1;}else{_LL3: _LL4:
 return 0;}_LL0:;}
# 38
int Cyc_Tcutil_is_char_type(void*t){
# 80
void*_stmttmp6=({Cyc_Tcutil_compress(t);});void*_tmpC=_stmttmp6;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpC)->tag == 0U){if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpC)->f1)->tag == 1U){if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpC)->f1)->f2 == Cyc_Absyn_Char_sz){_LL1: _LL2:
 return 1;}else{goto _LL3;}}else{goto _LL3;}}else{_LL3: _LL4:
 return 0;}_LL0:;}
# 38
int Cyc_Tcutil_is_any_int_type(void*t){
# 87
void*_stmttmp7=({Cyc_Tcutil_compress(t);});void*_tmpE=_stmttmp7;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpE)->tag == 0U){if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpE)->f1)->tag == 1U){_LL1: _LL2:
 return 1;}else{goto _LL3;}}else{_LL3: _LL4:
 return 0;}_LL0:;}
# 38
int Cyc_Tcutil_is_any_float_type(void*t){
# 94
void*_stmttmp8=({Cyc_Tcutil_compress(t);});void*_tmp10=_stmttmp8;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp10)->tag == 0U){if(((struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp10)->f1)->tag == 2U){_LL1: _LL2:
 return 1;}else{goto _LL3;}}else{_LL3: _LL4:
 return 0;}_LL0:;}
# 38
int Cyc_Tcutil_is_integral_type(void*t){
# 101
void*_stmttmp9=({Cyc_Tcutil_compress(t);});void*_tmp12=_stmttmp9;void*_tmp13;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp12)->tag == 0U){_LL1: _tmp13=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp12)->f1;_LL2: {void*c=_tmp13;
# 103
void*_tmp14=c;switch(*((int*)_tmp14)){case 1U: _LL6: _LL7:
 goto _LL9;case 5U: _LL8: _LL9:
 goto _LLB;case 17U: _LLA: _LLB:
 goto _LLD;case 18U: _LLC: _LLD:
 return 1;default: _LLE: _LLF:
 return 0;}_LL5:;}}else{_LL3: _LL4:
# 110
 return 0;}_LL0:;}
# 38
int Cyc_Tcutil_is_signed_type(void*t){
# 114
void*_stmttmpA=({Cyc_Tcutil_compress(t);});void*_tmp16=_stmttmpA;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp16)->tag == 0U)switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp16)->f1)){case 1U: if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp16)->f1)->f1 == Cyc_Absyn_Signed){_LL1: _LL2:
 return 1;}else{goto _LL5;}case 2U: _LL3: _LL4:
 return 1;default: goto _LL5;}else{_LL5: _LL6:
 return 0;}_LL0:;}
# 38
int Cyc_Tcutil_is_arithmetic_type(void*t){
# 121
return({Cyc_Tcutil_is_integral_type(t);})||({Cyc_Tcutil_is_any_float_type(t);});}
# 38
int Cyc_Tcutil_is_strict_arithmetic_type(void*t){
# 124
return({Cyc_Tcutil_is_any_int_type(t);})||({Cyc_Tcutil_is_any_float_type(t);});}
# 38
int Cyc_Tcutil_is_function_type(void*t){
# 127
void*_stmttmpB=({Cyc_Tcutil_compress(t);});void*_tmp1A=_stmttmpB;if(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1A)->tag == 5U){_LL1: _LL2:
 return 1;}else{_LL3: _LL4:
 return 0;}_LL0:;}
# 38
int Cyc_Tcutil_is_boxed(void*t){
# 133
return(int)({Cyc_Tcutil_type_kind(t);})->kind == (int)Cyc_Absyn_BoxKind;}
# 38
int Cyc_Tcutil_is_integral(struct Cyc_Absyn_Exp*e){
# 141
void*_stmttmpC=({Cyc_Tcutil_compress((void*)_check_null(e->topt));});void*_tmp1D=_stmttmpC;void*_tmp1E;if(((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp1D)->tag == 1U){_LL1: _LL2:
 return({Cyc_Unify_unify((void*)_check_null(e->topt),Cyc_Absyn_sint_type);});}else{_LL3: _tmp1E=_tmp1D;_LL4: {void*t=_tmp1E;
return({Cyc_Tcutil_is_integral_type(t);});}}_LL0:;}
# 38
int Cyc_Tcutil_is_numeric(struct Cyc_Absyn_Exp*e){
# 149
if(({Cyc_Tcutil_is_integral(e);}))
return 1;{
# 149
void*_stmttmpD=({Cyc_Tcutil_compress((void*)_check_null(e->topt));});void*_tmp20=_stmttmpD;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp20)->tag == 0U){if(((struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp20)->f1)->tag == 2U){_LL1: _LL2:
# 152
 return 1;}else{goto _LL3;}}else{_LL3: _LL4:
 return 0;}_LL0:;}}
# 38
int Cyc_Tcutil_is_zeroterm_pointer_type(void*t){
# 159
void*_stmttmpE=({Cyc_Tcutil_compress(t);});void*_tmp22=_stmttmpE;void*_tmp23;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp22)->tag == 3U){_LL1: _tmp23=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp22)->f1).ptr_atts).zero_term;_LL2: {void*ztl=_tmp23;
# 161
return({Cyc_Tcutil_force_type2bool(0,ztl);});}}else{_LL3: _LL4:
 return 0;}_LL0:;}
# 38
int Cyc_Tcutil_is_nullable_pointer_type(void*t){
# 168
void*_stmttmpF=({Cyc_Tcutil_compress(t);});void*_tmp25=_stmttmpF;void*_tmp26;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp25)->tag == 3U){_LL1: _tmp26=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp25)->f1).ptr_atts).nullable;_LL2: {void*nbl=_tmp26;
# 170
return({Cyc_Tcutil_force_type2bool(0,nbl);});}}else{_LL3: _LL4:
 return 0;}_LL0:;}
# 38
int Cyc_Tcutil_is_fat_ptr(void*t){
# 177
void*_stmttmp10=({Cyc_Tcutil_compress(t);});void*_tmp28=_stmttmp10;void*_tmp29;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp28)->tag == 3U){_LL1: _tmp29=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp28)->f1).ptr_atts).bounds;_LL2: {void*b=_tmp29;
# 179
return({Cyc_Unify_unify(Cyc_Absyn_fat_bound_type,b);});}}else{_LL3: _LL4:
 return 0;}_LL0:;}
# 38
int Cyc_Tcutil_is_fat_pointer_type_elt(void*t,void**elt_type_dest){
# 187
void*_stmttmp11=({Cyc_Tcutil_compress(t);});void*_tmp2B=_stmttmp11;void*_tmp2D;void*_tmp2C;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp2B)->tag == 3U){_LL1: _tmp2C=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp2B)->f1).elt_type;_tmp2D=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp2B)->f1).ptr_atts).bounds;_LL2: {void*elt_type=_tmp2C;void*b=_tmp2D;
# 189
if(({Cyc_Unify_unify(b,Cyc_Absyn_fat_bound_type);})){
*elt_type_dest=elt_type;
return 1;}else{
# 193
return 0;}}}else{_LL3: _LL4:
 return 0;}_LL0:;}
# 38
int Cyc_Tcutil_is_zero_pointer_type_elt(void*t,void**elt_type_dest){
# 201
void*_stmttmp12=({Cyc_Tcutil_compress(t);});void*_tmp2F=_stmttmp12;void*_tmp31;void*_tmp30;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp2F)->tag == 3U){_LL1: _tmp30=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp2F)->f1).elt_type;_tmp31=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp2F)->f1).ptr_atts).zero_term;_LL2: {void*elt_type=_tmp30;void*zt=_tmp31;
# 203
*elt_type_dest=elt_type;
return({Cyc_Absyn_type2bool(0,zt);});}}else{_LL3: _LL4:
 return 0;}_LL0:;}
# 38
int Cyc_Tcutil_is_zero_ptr_type(void*t,void**ptr_type,int*is_fat,void**elt_type){
# 214
void*_stmttmp13=({Cyc_Tcutil_compress(t);});void*_tmp33=_stmttmp13;void*_tmp37;struct Cyc_Absyn_Exp*_tmp36;struct Cyc_Absyn_Tqual _tmp35;void*_tmp34;void*_tmp3A;void*_tmp39;void*_tmp38;switch(*((int*)_tmp33)){case 3U: _LL1: _tmp38=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp33)->f1).elt_type;_tmp39=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp33)->f1).ptr_atts).bounds;_tmp3A=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp33)->f1).ptr_atts).zero_term;_LL2: {void*elt=_tmp38;void*bnds=_tmp39;void*zt=_tmp3A;
# 216
if(({Cyc_Absyn_type2bool(0,zt);})){
*ptr_type=t;
*elt_type=elt;
{void*_stmttmp14=({Cyc_Tcutil_compress(bnds);});void*_tmp3B=_stmttmp14;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp3B)->tag == 0U){if(((struct Cyc_Absyn_FatCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp3B)->f1)->tag == 16U){_LL8: _LL9:
*is_fat=1;goto _LL7;}else{goto _LLA;}}else{_LLA: _LLB:
*is_fat=0;goto _LL7;}_LL7:;}
# 223
return 1;}else{
return 0;}}case 4U: _LL3: _tmp34=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp33)->f1).elt_type;_tmp35=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp33)->f1).tq;_tmp36=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp33)->f1).num_elts;_tmp37=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp33)->f1).zero_term;_LL4: {void*elt=_tmp34;struct Cyc_Absyn_Tqual tq=_tmp35;struct Cyc_Absyn_Exp*n=_tmp36;void*zt=_tmp37;
# 226
if(({Cyc_Absyn_type2bool(0,zt);})){
*elt_type=elt;
*is_fat=0;
({void*_tmp703=({Cyc_Tcutil_promote_array(t,Cyc_Absyn_heap_rgn_type,0);});*ptr_type=_tmp703;});
return 1;}else{
return 0;}}default: _LL5: _LL6:
 return 0;}_LL0:;}
# 38
int Cyc_Tcutil_is_fat_pointer_type(void*t){
# 240
void*ignore=Cyc_Absyn_void_type;
return({Cyc_Tcutil_is_fat_pointer_type_elt(t,& ignore);});}
# 38
int Cyc_Tcutil_is_bound_one(void*b){
# 246
struct Cyc_Absyn_Exp*eopt=({struct Cyc_Absyn_Exp*(*_tmp40)(void*def,void*b)=Cyc_Tcutil_get_bounds_exp;void*_tmp41=({Cyc_Absyn_bounds_one();});void*_tmp42=b;_tmp40(_tmp41,_tmp42);});
if(eopt == 0)return 0;{struct Cyc_Absyn_Exp*e=eopt;
# 249
struct _tuple13 _stmttmp15=({Cyc_Evexp_eval_const_uint_exp(e);});int _tmp3F;unsigned _tmp3E;_LL1: _tmp3E=_stmttmp15.f1;_tmp3F=_stmttmp15.f2;_LL2: {unsigned i=_tmp3E;int known=_tmp3F;
return known && i == (unsigned)1;}}}
# 38
int Cyc_Tcutil_is_bits_only_type(void*t){
# 255
void*_stmttmp16=({Cyc_Tcutil_compress(t);});void*_tmp44=_stmttmp16;struct Cyc_List_List*_tmp45;struct Cyc_List_List*_tmp46;void*_tmp48;void*_tmp47;struct Cyc_List_List*_tmp4A;void*_tmp49;switch(*((int*)_tmp44)){case 0U: _LL1: _tmp49=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp44)->f1;_tmp4A=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp44)->f2;_LL2: {void*c=_tmp49;struct Cyc_List_List*ts=_tmp4A;
# 257
void*_tmp4B=c;struct Cyc_Absyn_Aggrdecl*_tmp4C;switch(*((int*)_tmp4B)){case 0U: _LLC: _LLD:
 goto _LLF;case 1U: _LLE: _LLF:
 goto _LL11;case 2U: _LL10: _LL11:
 return 1;case 17U: _LL12: _LL13:
 goto _LL15;case 18U: _LL14: _LL15:
 return 0;case 22U: if(((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)_tmp4B)->f1).UnknownAggr).tag == 1){_LL16: _LL17:
 return 0;}else{_LL18: _tmp4C=*((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)_tmp4B)->f1).KnownAggr).val;_LL19: {struct Cyc_Absyn_Aggrdecl*ad=_tmp4C;
# 265
if(ad->impl == 0)
return 0;{
# 265
int okay=1;
# 268
{struct Cyc_List_List*fs=((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->fields;for(0;fs != 0;fs=fs->tl){
if(!({Cyc_Tcutil_is_bits_only_type(((struct Cyc_Absyn_Aggrfield*)fs->hd)->type);})){okay=0;break;}}}
# 268
if(okay)
# 270
return 1;{
# 268
struct _RegionHandle _tmp4D=_new_region("rgn");struct _RegionHandle*rgn=& _tmp4D;_push_region(rgn);
# 272
{struct Cyc_List_List*inst=({Cyc_List_rzip(rgn,rgn,ad->tvs,ts);});
{struct Cyc_List_List*fs=((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->fields;for(0;fs != 0;fs=fs->tl){
if(!({int(*_tmp4E)(void*t)=Cyc_Tcutil_is_bits_only_type;void*_tmp4F=({Cyc_Tcutil_rsubstitute(rgn,inst,((struct Cyc_Absyn_Aggrfield*)fs->hd)->type);});_tmp4E(_tmp4F);})){int _tmp50=0;_npop_handler(0U);return _tmp50;}}}{
# 273
int _tmp51=1;_npop_handler(0U);return _tmp51;}}
# 272
;_pop_region();}}}}default: _LL1A: _LL1B:
# 276
 return 0;}_LLB:;}case 4U: _LL3: _tmp47=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp44)->f1).elt_type;_tmp48=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp44)->f1).zero_term;_LL4: {void*t=_tmp47;void*zero_term=_tmp48;
# 281
return !({Cyc_Absyn_type2bool(0,zero_term);})&&({Cyc_Tcutil_is_bits_only_type(t);});}case 6U: _LL5: _tmp46=((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp44)->f1;_LL6: {struct Cyc_List_List*tqs=_tmp46;
# 283
for(0;tqs != 0;tqs=tqs->tl){
if(!({Cyc_Tcutil_is_bits_only_type((*((struct _tuple12*)tqs->hd)).f2);}))return 0;}
# 283
return 1;}case 7U: _LL7: _tmp45=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp44)->f2;_LL8: {struct Cyc_List_List*fs=_tmp45;
# 287
for(0;fs != 0;fs=fs->tl){
if(!({Cyc_Tcutil_is_bits_only_type(((struct Cyc_Absyn_Aggrfield*)fs->hd)->type);}))return 0;}
# 287
return 1;}default: _LL9: _LLA:
# 290
 return 0;}_LL0:;}
# 38
int Cyc_Tcutil_is_noreturn_fn_type(void*t){
# 297
void*_stmttmp17=({Cyc_Tcutil_compress(t);});void*_tmp53=_stmttmp17;struct Cyc_List_List*_tmp54;void*_tmp55;switch(*((int*)_tmp53)){case 3U: _LL1: _tmp55=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp53)->f1).elt_type;_LL2: {void*elt=_tmp55;
return({Cyc_Tcutil_is_noreturn_fn_type(elt);});}case 5U: _LL3: _tmp54=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp53)->f1).attributes;_LL4: {struct Cyc_List_List*atts=_tmp54;
# 300
for(0;atts != 0;atts=atts->tl){
void*_stmttmp18=(void*)atts->hd;void*_tmp56=_stmttmp18;if(((struct Cyc_Absyn_Noreturn_att_Absyn_Attribute_struct*)_tmp56)->tag == 4U){_LL8: _LL9:
 return 1;}else{_LLA: _LLB:
 continue;}_LL7:;}
# 305
return 0;}default: _LL5: _LL6:
 return 0;}_LL0:;}
# 38
int Cyc_Tcutil_warn_region_coerce=0;
# 314
void Cyc_Tcutil_terr(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap){
# 316
({Cyc_Warn_verr(loc,fmt,ap);});}
# 318
void*Cyc_Tcutil_impos(struct _fat_ptr fmt,struct _fat_ptr ap){
# 320
({Cyc_Warn_vimpos(fmt,ap);});}
# 322
void Cyc_Tcutil_warn(unsigned sg,struct _fat_ptr fmt,struct _fat_ptr ap){
# 324
({Cyc_Warn_vwarn(sg,fmt,ap);});}
# 328
int Cyc_Tcutil_fast_tvar_cmp(struct Cyc_Absyn_Tvar*tv1,struct Cyc_Absyn_Tvar*tv2){
return tv1->identity - tv2->identity;}
# 328
void*Cyc_Tcutil_compress(void*t){
# 334
void*_tmp5C=t;void*_tmp5D;struct Cyc_Absyn_Exp*_tmp5E;struct Cyc_Absyn_Exp*_tmp5F;void**_tmp60;void**_tmp61;switch(*((int*)_tmp5C)){case 1U: if(((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp5C)->f2 == 0){_LL1: _LL2:
 goto _LL4;}else{_LL7: _tmp61=(void**)&((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp5C)->f2;_LL8: {void**t2opt_ref=_tmp61;
# 344
void*ta=(void*)_check_null(*t2opt_ref);
void*t2=({Cyc_Tcutil_compress(ta);});
if(t2 != ta)
*t2opt_ref=t2;
# 346
return t2;}}case 8U: if(((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp5C)->f4 == 0){_LL3: _LL4:
# 336
 return t;}else{_LL5: _tmp60=(void**)&((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp5C)->f4;_LL6: {void**topt_ref=_tmp60;
# 338
void*ta=(void*)_check_null(*topt_ref);
void*t2=({Cyc_Tcutil_compress(ta);});
if(t2 != ta)
*topt_ref=t2;
# 340
return t2;}}case 9U: _LL9: _tmp5F=((struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_tmp5C)->f1;_LLA: {struct Cyc_Absyn_Exp*e=_tmp5F;
# 350
({Cyc_Evexp_eval_const_uint_exp(e);});{
void*_stmttmp19=e->r;void*_tmp62=_stmttmp19;void*_tmp63;if(((struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct*)_tmp62)->tag == 40U){_LL12: _tmp63=(void*)((struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct*)_tmp62)->f1;_LL13: {void*t2=_tmp63;
return({Cyc_Tcutil_compress(t2);});}}else{_LL14: _LL15:
 return t;}_LL11:;}}case 11U: _LLB: _tmp5E=((struct Cyc_Absyn_TypeofType_Absyn_Type_struct*)_tmp5C)->f1;_LLC: {struct Cyc_Absyn_Exp*e=_tmp5E;
# 356
void*t2=e->topt;
if(t2 != 0)return t2;else{
return t;}}case 10U: if(((struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_tmp5C)->f2 != 0){_LLD: _tmp5D=*((struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_tmp5C)->f2;_LLE: {void*t=_tmp5D;
return({Cyc_Tcutil_compress(t);});}}else{goto _LLF;}default: _LLF: _LL10:
 return t;}_LL0:;}
# 328
void*Cyc_Tcutil_copy_type(void*t);
# 369
static struct Cyc_List_List*Cyc_Tcutil_copy_types(struct Cyc_List_List*ts){
return({Cyc_List_map(Cyc_Tcutil_copy_type,ts);});}
# 372
static void*Cyc_Tcutil_copy_kindbound(void*kb){
void*_stmttmp1A=({Cyc_Absyn_compress_kb(kb);});void*_tmp66=_stmttmp1A;struct Cyc_Absyn_Kind*_tmp67;switch(*((int*)_tmp66)){case 1U: _LL1: _LL2:
 return(void*)({struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*_tmp68=_cycalloc(sizeof(*_tmp68));_tmp68->tag=1U,_tmp68->f1=0;_tmp68;});case 2U: _LL3: _tmp67=((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp66)->f2;_LL4: {struct Cyc_Absyn_Kind*k=_tmp67;
return(void*)({struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*_tmp69=_cycalloc(sizeof(*_tmp69));_tmp69->tag=2U,_tmp69->f1=0,_tmp69->f2=k;_tmp69;});}default: _LL5: _LL6:
 return kb;}_LL0:;}
# 379
static struct Cyc_Absyn_Tvar*Cyc_Tcutil_copy_tvar(struct Cyc_Absyn_Tvar*tv){
# 381
return({struct Cyc_Absyn_Tvar*_tmp6B=_cycalloc(sizeof(*_tmp6B));_tmp6B->name=tv->name,_tmp6B->identity=- 1,({void*_tmp704=({Cyc_Tcutil_copy_kindbound(tv->kind);});_tmp6B->kind=_tmp704;});_tmp6B;});}
# 384
static struct _tuple9*Cyc_Tcutil_copy_arg(struct _tuple9*arg){
void*_tmp6F;struct Cyc_Absyn_Tqual _tmp6E;struct _fat_ptr*_tmp6D;_LL1: _tmp6D=arg->f1;_tmp6E=arg->f2;_tmp6F=arg->f3;_LL2: {struct _fat_ptr*x=_tmp6D;struct Cyc_Absyn_Tqual y=_tmp6E;void*t=_tmp6F;
return({struct _tuple9*_tmp70=_cycalloc(sizeof(*_tmp70));_tmp70->f1=x,_tmp70->f2=y,({void*_tmp705=({Cyc_Tcutil_copy_type(t);});_tmp70->f3=_tmp705;});_tmp70;});}}
# 388
static struct _tuple12*Cyc_Tcutil_copy_tqt(struct _tuple12*arg){
return({struct _tuple12*_tmp72=_cycalloc(sizeof(*_tmp72));_tmp72->f1=(*arg).f1,({void*_tmp706=({Cyc_Tcutil_copy_type((*arg).f2);});_tmp72->f2=_tmp706;});_tmp72;});}
# 391
static struct Cyc_Absyn_Exp*Cyc_Tcutil_deep_copy_exp_opt(int preserve_types,struct Cyc_Absyn_Exp*e){
if(e == 0)return 0;else{
return({Cyc_Tcutil_deep_copy_exp(preserve_types,e);});}}
# 395
static struct Cyc_Absyn_Aggrfield*Cyc_Tcutil_copy_field(struct Cyc_Absyn_Aggrfield*f){
return({struct Cyc_Absyn_Aggrfield*_tmp75=_cycalloc(sizeof(*_tmp75));_tmp75->name=f->name,_tmp75->tq=f->tq,({void*_tmp708=({Cyc_Tcutil_copy_type(f->type);});_tmp75->type=_tmp708;}),_tmp75->width=f->width,_tmp75->attributes=f->attributes,({
struct Cyc_Absyn_Exp*_tmp707=({Cyc_Tcutil_deep_copy_exp_opt(1,f->requires_clause);});_tmp75->requires_clause=_tmp707;});_tmp75;});}struct _tuple15{void*f1;void*f2;};
# 399
static struct _tuple15*Cyc_Tcutil_copy_rgncmp(struct _tuple15*x){
void*_tmp78;void*_tmp77;_LL1: _tmp77=x->f1;_tmp78=x->f2;_LL2: {void*r1=_tmp77;void*r2=_tmp78;
return({struct _tuple15*_tmp79=_cycalloc(sizeof(*_tmp79));({void*_tmp70A=({Cyc_Tcutil_copy_type(r1);});_tmp79->f1=_tmp70A;}),({void*_tmp709=({Cyc_Tcutil_copy_type(r2);});_tmp79->f2=_tmp709;});_tmp79;});}}
# 403
static void*Cyc_Tcutil_tvar2type(struct Cyc_Absyn_Tvar*t){
return({void*(*_tmp7B)(struct Cyc_Absyn_Tvar*)=Cyc_Absyn_var_type;struct Cyc_Absyn_Tvar*_tmp7C=({Cyc_Tcutil_copy_tvar(t);});_tmp7B(_tmp7C);});}
# 403
void*Cyc_Tcutil_copy_type(void*t){
# 412
void*_stmttmp1B=({Cyc_Tcutil_compress(t);});void*_tmp7E=_stmttmp1B;struct Cyc_Absyn_Datatypedecl*_tmp7F;struct Cyc_Absyn_Enumdecl*_tmp80;struct Cyc_Absyn_Aggrdecl*_tmp81;struct Cyc_Absyn_Typedefdecl*_tmp84;struct Cyc_List_List*_tmp83;struct _tuple1*_tmp82;struct Cyc_Absyn_Exp*_tmp85;struct Cyc_Absyn_Exp*_tmp86;struct Cyc_List_List*_tmp88;enum Cyc_Absyn_AggrKind _tmp87;struct Cyc_List_List*_tmp89;int _tmp9A;struct Cyc_List_List*_tmp99;struct Cyc_List_List*_tmp98;struct Cyc_List_List*_tmp97;struct Cyc_List_List*_tmp96;struct Cyc_Absyn_Exp*_tmp95;struct Cyc_List_List*_tmp94;struct Cyc_Absyn_Exp*_tmp93;struct Cyc_List_List*_tmp92;struct Cyc_List_List*_tmp91;struct Cyc_Absyn_VarargInfo*_tmp90;int _tmp8F;struct Cyc_List_List*_tmp8E;void*_tmp8D;struct Cyc_Absyn_Tqual _tmp8C;void*_tmp8B;struct Cyc_List_List*_tmp8A;unsigned _tmp9F;void*_tmp9E;struct Cyc_Absyn_Exp*_tmp9D;struct Cyc_Absyn_Tqual _tmp9C;void*_tmp9B;struct Cyc_Absyn_PtrLoc*_tmpA6;void*_tmpA5;void*_tmpA4;void*_tmpA3;void*_tmpA2;struct Cyc_Absyn_Tqual _tmpA1;void*_tmpA0;struct Cyc_Absyn_Tvar*_tmpA7;struct Cyc_List_List*_tmpA9;void*_tmpA8;void*_tmpAA;switch(*((int*)_tmp7E)){case 0U: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp7E)->f2 == 0){_LL1: _tmpAA=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp7E)->f1;_LL2: {void*c=_tmpAA;
return t;}}else{_LL3: _tmpA8=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp7E)->f1;_tmpA9=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp7E)->f2;_LL4: {void*c=_tmpA8;struct Cyc_List_List*ts=_tmpA9;
return(void*)({struct Cyc_Absyn_AppType_Absyn_Type_struct*_tmpAB=_cycalloc(sizeof(*_tmpAB));_tmpAB->tag=0U,_tmpAB->f1=c,({struct Cyc_List_List*_tmp70B=({Cyc_Tcutil_copy_types(ts);});_tmpAB->f2=_tmp70B;});_tmpAB;});}}case 1U: _LL5: _LL6:
 return t;case 2U: _LL7: _tmpA7=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp7E)->f1;_LL8: {struct Cyc_Absyn_Tvar*tv=_tmpA7;
return({void*(*_tmpAC)(struct Cyc_Absyn_Tvar*)=Cyc_Absyn_var_type;struct Cyc_Absyn_Tvar*_tmpAD=({Cyc_Tcutil_copy_tvar(tv);});_tmpAC(_tmpAD);});}case 3U: _LL9: _tmpA0=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp7E)->f1).elt_type;_tmpA1=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp7E)->f1).elt_tq;_tmpA2=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp7E)->f1).ptr_atts).rgn;_tmpA3=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp7E)->f1).ptr_atts).nullable;_tmpA4=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp7E)->f1).ptr_atts).bounds;_tmpA5=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp7E)->f1).ptr_atts).zero_term;_tmpA6=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp7E)->f1).ptr_atts).ptrloc;_LLA: {void*elt=_tmpA0;struct Cyc_Absyn_Tqual tq=_tmpA1;void*rgn=_tmpA2;void*nbl=_tmpA3;void*bs=_tmpA4;void*zt=_tmpA5;struct Cyc_Absyn_PtrLoc*loc=_tmpA6;
# 418
void*elt2=({Cyc_Tcutil_copy_type(elt);});
void*rgn2=({Cyc_Tcutil_copy_type(rgn);});
void*nbl2=({Cyc_Tcutil_copy_type(nbl);});
struct Cyc_Absyn_Tqual tq2=tq;
# 423
void*bs2=({Cyc_Tcutil_copy_type(bs);});
void*zt2=({Cyc_Tcutil_copy_type(zt);});
return(void*)({struct Cyc_Absyn_PointerType_Absyn_Type_struct*_tmpAE=_cycalloc(sizeof(*_tmpAE));_tmpAE->tag=3U,(_tmpAE->f1).elt_type=elt2,(_tmpAE->f1).elt_tq=tq2,((_tmpAE->f1).ptr_atts).rgn=rgn2,((_tmpAE->f1).ptr_atts).nullable=nbl2,((_tmpAE->f1).ptr_atts).bounds=bs2,((_tmpAE->f1).ptr_atts).zero_term=zt2,((_tmpAE->f1).ptr_atts).ptrloc=loc;_tmpAE;});}case 4U: _LLB: _tmp9B=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp7E)->f1).elt_type;_tmp9C=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp7E)->f1).tq;_tmp9D=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp7E)->f1).num_elts;_tmp9E=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp7E)->f1).zero_term;_tmp9F=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp7E)->f1).zt_loc;_LLC: {void*et=_tmp9B;struct Cyc_Absyn_Tqual tq=_tmp9C;struct Cyc_Absyn_Exp*eo=_tmp9D;void*zt=_tmp9E;unsigned ztl=_tmp9F;
# 427
return(void*)({struct Cyc_Absyn_ArrayType_Absyn_Type_struct*_tmpAF=_cycalloc(sizeof(*_tmpAF));_tmpAF->tag=4U,({void*_tmp70E=({Cyc_Tcutil_copy_type(et);});(_tmpAF->f1).elt_type=_tmp70E;}),(_tmpAF->f1).tq=tq,({struct Cyc_Absyn_Exp*_tmp70D=({Cyc_Tcutil_deep_copy_exp_opt(1,eo);});(_tmpAF->f1).num_elts=_tmp70D;}),({
void*_tmp70C=({Cyc_Tcutil_copy_type(zt);});(_tmpAF->f1).zero_term=_tmp70C;}),(_tmpAF->f1).zt_loc=ztl;_tmpAF;});}case 5U: _LLD: _tmp8A=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp7E)->f1).tvars;_tmp8B=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp7E)->f1).effect;_tmp8C=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp7E)->f1).ret_tqual;_tmp8D=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp7E)->f1).ret_type;_tmp8E=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp7E)->f1).args;_tmp8F=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp7E)->f1).c_varargs;_tmp90=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp7E)->f1).cyc_varargs;_tmp91=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp7E)->f1).rgn_po;_tmp92=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp7E)->f1).attributes;_tmp93=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp7E)->f1).requires_clause;_tmp94=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp7E)->f1).requires_relns;_tmp95=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp7E)->f1).ensures_clause;_tmp96=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp7E)->f1).ensures_relns;_tmp97=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp7E)->f1).ieffect;_tmp98=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp7E)->f1).oeffect;_tmp99=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp7E)->f1).throws;_tmp9A=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp7E)->f1).reentrant;_LLE: {struct Cyc_List_List*tvs=_tmp8A;void*effopt=_tmp8B;struct Cyc_Absyn_Tqual rt_tq=_tmp8C;void*rt=_tmp8D;struct Cyc_List_List*args=_tmp8E;int c_varargs=_tmp8F;struct Cyc_Absyn_VarargInfo*cyc_varargs=_tmp90;struct Cyc_List_List*rgn_po=_tmp91;struct Cyc_List_List*atts=_tmp92;struct Cyc_Absyn_Exp*req=_tmp93;struct Cyc_List_List*req_rlns=_tmp94;struct Cyc_Absyn_Exp*ens=_tmp95;struct Cyc_List_List*ens_rlns=_tmp96;struct Cyc_List_List*ieff=_tmp97;struct Cyc_List_List*oeff=_tmp98;struct Cyc_List_List*throws=_tmp99;int reentrant=_tmp9A;
# 431
struct Cyc_List_List*tvs2=({({struct Cyc_List_List*(*_tmp70F)(struct Cyc_Absyn_Tvar*(*f)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmpB4)(struct Cyc_Absyn_Tvar*(*f)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_Tvar*(*f)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*x))Cyc_List_map;_tmpB4;});_tmp70F(Cyc_Tcutil_copy_tvar,tvs);});});
void*effopt2=effopt == 0?0:({Cyc_Tcutil_copy_type(effopt);});
void*rt2=({Cyc_Tcutil_copy_type(rt);});
struct Cyc_List_List*args2=({({struct Cyc_List_List*(*_tmp710)(struct _tuple9*(*f)(struct _tuple9*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmpB3)(struct _tuple9*(*f)(struct _tuple9*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct _tuple9*(*f)(struct _tuple9*),struct Cyc_List_List*x))Cyc_List_map;_tmpB3;});_tmp710(Cyc_Tcutil_copy_arg,args);});});
int c_varargs2=c_varargs;
struct Cyc_Absyn_VarargInfo*cyc_varargs2=0;
if(cyc_varargs != 0){
struct Cyc_Absyn_VarargInfo*cv=cyc_varargs;
cyc_varargs2=({struct Cyc_Absyn_VarargInfo*_tmpB0=_cycalloc(sizeof(*_tmpB0));_tmpB0->name=cv->name,_tmpB0->tq=cv->tq,({void*_tmp711=({Cyc_Tcutil_copy_type(cv->type);});_tmpB0->type=_tmp711;}),_tmpB0->inject=cv->inject;_tmpB0;});}{
# 437
struct Cyc_List_List*rgn_po2=({({struct Cyc_List_List*(*_tmp712)(struct _tuple15*(*f)(struct _tuple15*),struct Cyc_List_List*x)=({
# 442
struct Cyc_List_List*(*_tmpB2)(struct _tuple15*(*f)(struct _tuple15*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct _tuple15*(*f)(struct _tuple15*),struct Cyc_List_List*x))Cyc_List_map;_tmpB2;});_tmp712(Cyc_Tcutil_copy_rgncmp,rgn_po);});});
struct Cyc_List_List*atts2=atts;
struct Cyc_Absyn_Exp*req2=({Cyc_Tcutil_deep_copy_exp_opt(1,req);});
struct Cyc_List_List*req_rlns2=({Cyc_Relations_copy_relns(Cyc_Core_heap_region,req_rlns);});
struct Cyc_Absyn_Exp*ens2=({Cyc_Tcutil_deep_copy_exp_opt(1,ens);});
struct Cyc_List_List*ens_rlns2=({Cyc_Relations_copy_relns(Cyc_Core_heap_region,ens_rlns);});
struct Cyc_List_List*ieff2=({Cyc_Absyn_copy_effect(ieff);});
struct Cyc_List_List*oeff2=({Cyc_Absyn_copy_effect(oeff);});
int reentrant2=({Cyc_Absyn_copy_reentrant(reentrant);});
return(void*)({struct Cyc_Absyn_FnType_Absyn_Type_struct*_tmpB1=_cycalloc(sizeof(*_tmpB1));_tmpB1->tag=5U,(_tmpB1->f1).tvars=tvs2,(_tmpB1->f1).effect=effopt2,(_tmpB1->f1).ret_tqual=rt_tq,(_tmpB1->f1).ret_type=rt2,(_tmpB1->f1).args=args2,(_tmpB1->f1).c_varargs=c_varargs2,(_tmpB1->f1).cyc_varargs=cyc_varargs2,(_tmpB1->f1).rgn_po=rgn_po2,(_tmpB1->f1).attributes=atts2,(_tmpB1->f1).requires_clause=req2,(_tmpB1->f1).requires_relns=req_rlns2,(_tmpB1->f1).ensures_clause=ens2,(_tmpB1->f1).ensures_relns=ens_rlns2,(_tmpB1->f1).ieffect=ieff2,(_tmpB1->f1).oeffect=oeff2,(_tmpB1->f1).throws=throws,(_tmpB1->f1).reentrant=reentrant2;_tmpB1;});}}case 6U: _LLF: _tmp89=((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp7E)->f1;_LL10: {struct Cyc_List_List*tqts=_tmp89;
# 454
return(void*)({struct Cyc_Absyn_TupleType_Absyn_Type_struct*_tmpB6=_cycalloc(sizeof(*_tmpB6));_tmpB6->tag=6U,({struct Cyc_List_List*_tmp714=({({struct Cyc_List_List*(*_tmp713)(struct _tuple12*(*f)(struct _tuple12*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmpB5)(struct _tuple12*(*f)(struct _tuple12*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct _tuple12*(*f)(struct _tuple12*),struct Cyc_List_List*x))Cyc_List_map;_tmpB5;});_tmp713(Cyc_Tcutil_copy_tqt,tqts);});});_tmpB6->f1=_tmp714;});_tmpB6;});}case 7U: _LL11: _tmp87=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp7E)->f1;_tmp88=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp7E)->f2;_LL12: {enum Cyc_Absyn_AggrKind k=_tmp87;struct Cyc_List_List*fs=_tmp88;
return(void*)({struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*_tmpB8=_cycalloc(sizeof(*_tmpB8));_tmpB8->tag=7U,_tmpB8->f1=k,({struct Cyc_List_List*_tmp716=({({struct Cyc_List_List*(*_tmp715)(struct Cyc_Absyn_Aggrfield*(*f)(struct Cyc_Absyn_Aggrfield*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmpB7)(struct Cyc_Absyn_Aggrfield*(*f)(struct Cyc_Absyn_Aggrfield*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_Aggrfield*(*f)(struct Cyc_Absyn_Aggrfield*),struct Cyc_List_List*x))Cyc_List_map;_tmpB7;});_tmp715(Cyc_Tcutil_copy_field,fs);});});_tmpB8->f2=_tmp716;});_tmpB8;});}case 9U: _LL13: _tmp86=((struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_tmp7E)->f1;_LL14: {struct Cyc_Absyn_Exp*e=_tmp86;
return(void*)({struct Cyc_Absyn_ValueofType_Absyn_Type_struct*_tmpB9=_cycalloc(sizeof(*_tmpB9));_tmpB9->tag=9U,_tmpB9->f1=e;_tmpB9;});}case 11U: _LL15: _tmp85=((struct Cyc_Absyn_TypeofType_Absyn_Type_struct*)_tmp7E)->f1;_LL16: {struct Cyc_Absyn_Exp*e=_tmp85;
return(void*)({struct Cyc_Absyn_TypeofType_Absyn_Type_struct*_tmpBA=_cycalloc(sizeof(*_tmpBA));_tmpBA->tag=11U,_tmpBA->f1=e;_tmpBA;});}case 8U: _LL17: _tmp82=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp7E)->f1;_tmp83=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp7E)->f2;_tmp84=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp7E)->f3;_LL18: {struct _tuple1*tdn=_tmp82;struct Cyc_List_List*ts=_tmp83;struct Cyc_Absyn_Typedefdecl*td=_tmp84;
# 459
return(void*)({struct Cyc_Absyn_TypedefType_Absyn_Type_struct*_tmpBB=_cycalloc(sizeof(*_tmpBB));_tmpBB->tag=8U,_tmpBB->f1=tdn,({struct Cyc_List_List*_tmp717=({Cyc_Tcutil_copy_types(ts);});_tmpBB->f2=_tmp717;}),_tmpBB->f3=td,_tmpBB->f4=0;_tmpBB;});}default: switch(*((int*)((struct Cyc_Absyn_TypeDecl*)((struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_tmp7E)->f1)->r)){case 0U: _LL19: _tmp81=((struct Cyc_Absyn_Aggr_td_Absyn_Raw_typedecl_struct*)(((struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_tmp7E)->f1)->r)->f1;_LL1A: {struct Cyc_Absyn_Aggrdecl*ad=_tmp81;
# 462
struct Cyc_List_List*targs=({({struct Cyc_List_List*(*_tmp718)(void*(*f)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmpBF)(void*(*f)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(void*(*f)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*x))Cyc_List_map;_tmpBF;});_tmp718(Cyc_Tcutil_tvar2type,ad->tvs);});});
return({void*(*_tmpBC)(union Cyc_Absyn_AggrInfo,struct Cyc_List_List*args)=Cyc_Absyn_aggr_type;union Cyc_Absyn_AggrInfo _tmpBD=({Cyc_Absyn_UnknownAggr(ad->kind,ad->name,0);});struct Cyc_List_List*_tmpBE=targs;_tmpBC(_tmpBD,_tmpBE);});}case 1U: _LL1B: _tmp80=((struct Cyc_Absyn_Enum_td_Absyn_Raw_typedecl_struct*)(((struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_tmp7E)->f1)->r)->f1;_LL1C: {struct Cyc_Absyn_Enumdecl*ed=_tmp80;
# 465
return({Cyc_Absyn_enum_type(ed->name,0);});}default: _LL1D: _tmp7F=((struct Cyc_Absyn_Datatype_td_Absyn_Raw_typedecl_struct*)(((struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_tmp7E)->f1)->r)->f1;_LL1E: {struct Cyc_Absyn_Datatypedecl*dd=_tmp7F;
# 467
struct Cyc_List_List*targs=({({struct Cyc_List_List*(*_tmp719)(void*(*f)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmpC3)(void*(*f)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(void*(*f)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*x))Cyc_List_map;_tmpC3;});_tmp719(Cyc_Tcutil_tvar2type,dd->tvs);});});
return({void*(*_tmpC0)(union Cyc_Absyn_DatatypeInfo,struct Cyc_List_List*args)=Cyc_Absyn_datatype_type;union Cyc_Absyn_DatatypeInfo _tmpC1=({Cyc_Absyn_UnknownDatatype(({struct Cyc_Absyn_UnknownDatatypeInfo _tmp6B1;_tmp6B1.name=dd->name,_tmp6B1.is_extensible=0;_tmp6B1;}));});struct Cyc_List_List*_tmpC2=targs;_tmpC0(_tmpC1,_tmpC2);});}}}_LL0:;}
# 474
static void*Cyc_Tcutil_copy_designator(int preserve_types,void*d){
void*_tmpC5=d;struct _fat_ptr*_tmpC6;struct Cyc_Absyn_Exp*_tmpC7;if(((struct Cyc_Absyn_ArrayElement_Absyn_Designator_struct*)_tmpC5)->tag == 0U){_LL1: _tmpC7=((struct Cyc_Absyn_ArrayElement_Absyn_Designator_struct*)_tmpC5)->f1;_LL2: {struct Cyc_Absyn_Exp*e=_tmpC7;
return(void*)({struct Cyc_Absyn_ArrayElement_Absyn_Designator_struct*_tmpC8=_cycalloc(sizeof(*_tmpC8));_tmpC8->tag=0U,({struct Cyc_Absyn_Exp*_tmp71A=({Cyc_Tcutil_deep_copy_exp(preserve_types,e);});_tmpC8->f1=_tmp71A;});_tmpC8;});}}else{_LL3: _tmpC6=((struct Cyc_Absyn_FieldName_Absyn_Designator_struct*)_tmpC5)->f1;_LL4: {struct _fat_ptr*v=_tmpC6;
return d;}}_LL0:;}struct _tuple16{struct Cyc_List_List*f1;struct Cyc_Absyn_Exp*f2;};
# 481
static struct _tuple16*Cyc_Tcutil_copy_eds(int preserve_types,struct _tuple16*e){
return({struct _tuple16*_tmpCB=_cycalloc(sizeof(*_tmpCB));({struct Cyc_List_List*_tmp71E=({({struct Cyc_List_List*(*_tmp71D)(void*(*f)(int,void*),int env,struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmpCA)(void*(*f)(int,void*),int env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(void*(*f)(int,void*),int env,struct Cyc_List_List*x))Cyc_List_map_c;_tmpCA;});int _tmp71C=preserve_types;_tmp71D(Cyc_Tcutil_copy_designator,_tmp71C,(e[0]).f1);});});_tmpCB->f1=_tmp71E;}),({struct Cyc_Absyn_Exp*_tmp71B=({Cyc_Tcutil_deep_copy_exp(preserve_types,(e[0]).f2);});_tmpCB->f2=_tmp71B;});_tmpCB;});}
# 481
struct Cyc_Absyn_Exp*Cyc_Tcutil_deep_copy_exp(int preserve_types,struct Cyc_Absyn_Exp*e){
# 486
struct Cyc_Absyn_Exp*new_e;
int pt=preserve_types;
{void*_stmttmp1C=e->r;void*_tmpCD=_stmttmp1C;struct Cyc_List_List*_tmpD2;struct Cyc_List_List*_tmpD1;struct Cyc_List_List*_tmpD0;struct _fat_ptr _tmpCF;int _tmpCE;void*_tmpD3;struct _fat_ptr*_tmpD5;struct Cyc_Absyn_Exp*_tmpD4;struct Cyc_List_List*_tmpD7;struct Cyc_Core_Opt*_tmpD6;struct Cyc_Absyn_Exp*_tmpD9;struct Cyc_Absyn_Exp*_tmpD8;int _tmpDF;int _tmpDE;struct Cyc_Absyn_Exp*_tmpDD;void**_tmpDC;struct Cyc_Absyn_Exp*_tmpDB;int _tmpDA;struct Cyc_Absyn_Enumfield*_tmpE1;void*_tmpE0;struct Cyc_Absyn_Enumfield*_tmpE3;struct Cyc_Absyn_Enumdecl*_tmpE2;struct Cyc_List_List*_tmpE7;void*_tmpE6;struct Cyc_Absyn_Tqual _tmpE5;struct _fat_ptr*_tmpE4;struct Cyc_List_List*_tmpE9;void*_tmpE8;struct Cyc_Absyn_Aggrdecl*_tmpED;struct Cyc_List_List*_tmpEC;struct Cyc_List_List*_tmpEB;struct _tuple1*_tmpEA;int _tmpF0;void*_tmpEF;struct Cyc_Absyn_Exp*_tmpEE;int _tmpF4;struct Cyc_Absyn_Exp*_tmpF3;struct Cyc_Absyn_Exp*_tmpF2;struct Cyc_Absyn_Vardecl*_tmpF1;struct Cyc_Absyn_Datatypefield*_tmpF7;struct Cyc_Absyn_Datatypedecl*_tmpF6;struct Cyc_List_List*_tmpF5;struct Cyc_List_List*_tmpF8;struct Cyc_Absyn_Exp*_tmpFA;struct Cyc_Absyn_Exp*_tmpF9;int _tmpFE;int _tmpFD;struct _fat_ptr*_tmpFC;struct Cyc_Absyn_Exp*_tmpFB;int _tmp102;int _tmp101;struct _fat_ptr*_tmp100;struct Cyc_Absyn_Exp*_tmpFF;struct Cyc_List_List*_tmp103;struct Cyc_Absyn_Exp*_tmp104;struct Cyc_Absyn_Exp*_tmp105;struct Cyc_List_List*_tmp107;void*_tmp106;struct Cyc_Absyn_Exp*_tmp108;void*_tmp109;struct Cyc_Absyn_Exp*_tmp10A;struct Cyc_Absyn_Exp*_tmp10C;struct Cyc_Absyn_Exp*_tmp10B;enum Cyc_Absyn_Coercion _tmp110;int _tmp10F;struct Cyc_Absyn_Exp*_tmp10E;void*_tmp10D;struct Cyc_List_List*_tmp112;struct Cyc_Absyn_Exp*_tmp111;struct Cyc_Absyn_Exp*_tmp113;int _tmp115;struct Cyc_Absyn_Exp*_tmp114;struct Cyc_List_List*_tmp11B;struct Cyc_List_List*_tmp11A;int _tmp119;struct Cyc_Absyn_VarargCallInfo*_tmp118;struct Cyc_List_List*_tmp117;struct Cyc_Absyn_Exp*_tmp116;struct Cyc_List_List*_tmp126;struct Cyc_List_List*_tmp125;int _tmp124;int _tmp123;void*_tmp122;struct Cyc_Absyn_Tqual _tmp121;struct _fat_ptr*_tmp120;struct Cyc_List_List*_tmp11F;int _tmp11E;struct Cyc_List_List*_tmp11D;struct Cyc_Absyn_Exp*_tmp11C;struct Cyc_Absyn_Exp*_tmp129;struct Cyc_Core_Opt*_tmp128;struct Cyc_Absyn_Exp*_tmp127;struct Cyc_Absyn_Exp*_tmp12C;struct Cyc_Absyn_Exp*_tmp12B;struct Cyc_Absyn_Exp*_tmp12A;struct Cyc_List_List*_tmp130;struct Cyc_List_List*_tmp12F;struct Cyc_Absyn_Exp*_tmp12E;struct Cyc_Absyn_Exp*_tmp12D;struct Cyc_Absyn_Exp*_tmp132;struct Cyc_Absyn_Exp*_tmp131;struct Cyc_Absyn_Exp*_tmp134;struct Cyc_Absyn_Exp*_tmp133;struct Cyc_Absyn_Exp*_tmp136;struct Cyc_Absyn_Exp*_tmp135;enum Cyc_Absyn_Incrementor _tmp138;struct Cyc_Absyn_Exp*_tmp137;struct Cyc_List_List*_tmp13A;enum Cyc_Absyn_Primop _tmp139;struct _fat_ptr _tmp13B;void*_tmp13C;union Cyc_Absyn_Cnst _tmp13D;switch(*((int*)_tmpCD)){case 0U: _LL1: _tmp13D=((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmpCD)->f1;_LL2: {union Cyc_Absyn_Cnst c=_tmp13D;
new_e=({Cyc_Absyn_const_exp(c,e->loc);});goto _LL0;}case 1U: _LL3: _tmp13C=(void*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmpCD)->f1;_LL4: {void*b=_tmp13C;
new_e=({Cyc_Absyn_varb_exp(b,e->loc);});goto _LL0;}case 2U: _LL5: _tmp13B=((struct Cyc_Absyn_Pragma_e_Absyn_Raw_exp_struct*)_tmpCD)->f1;_LL6: {struct _fat_ptr p=_tmp13B;
new_e=({Cyc_Absyn_pragma_exp(p,e->loc);});goto _LL0;}case 3U: _LL7: _tmp139=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmpCD)->f1;_tmp13A=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmpCD)->f2;_LL8: {enum Cyc_Absyn_Primop p=_tmp139;struct Cyc_List_List*es=_tmp13A;
new_e=({struct Cyc_Absyn_Exp*(*_tmp13E)(enum Cyc_Absyn_Primop,struct Cyc_List_List*,unsigned)=Cyc_Absyn_primop_exp;enum Cyc_Absyn_Primop _tmp13F=p;struct Cyc_List_List*_tmp140=({({struct Cyc_List_List*(*_tmp720)(struct Cyc_Absyn_Exp*(*f)(int,struct Cyc_Absyn_Exp*),int env,struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp141)(struct Cyc_Absyn_Exp*(*f)(int,struct Cyc_Absyn_Exp*),int env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_Exp*(*f)(int,struct Cyc_Absyn_Exp*),int env,struct Cyc_List_List*x))Cyc_List_map_c;_tmp141;});int _tmp71F=pt;_tmp720(Cyc_Tcutil_deep_copy_exp,_tmp71F,es);});});unsigned _tmp142=e->loc;_tmp13E(_tmp13F,_tmp140,_tmp142);});goto _LL0;}case 5U: _LL9: _tmp137=((struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*)_tmpCD)->f1;_tmp138=((struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*)_tmpCD)->f2;_LLA: {struct Cyc_Absyn_Exp*e1=_tmp137;enum Cyc_Absyn_Incrementor i=_tmp138;
new_e=({struct Cyc_Absyn_Exp*(*_tmp143)(struct Cyc_Absyn_Exp*,enum Cyc_Absyn_Incrementor,unsigned)=Cyc_Absyn_increment_exp;struct Cyc_Absyn_Exp*_tmp144=({Cyc_Tcutil_deep_copy_exp(pt,e1);});enum Cyc_Absyn_Incrementor _tmp145=i;unsigned _tmp146=e->loc;_tmp143(_tmp144,_tmp145,_tmp146);});goto _LL0;}case 7U: _LLB: _tmp135=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmpCD)->f1;_tmp136=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmpCD)->f2;_LLC: {struct Cyc_Absyn_Exp*e1=_tmp135;struct Cyc_Absyn_Exp*e2=_tmp136;
new_e=({struct Cyc_Absyn_Exp*(*_tmp147)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_and_exp;struct Cyc_Absyn_Exp*_tmp148=({Cyc_Tcutil_deep_copy_exp(pt,e1);});struct Cyc_Absyn_Exp*_tmp149=({Cyc_Tcutil_deep_copy_exp(pt,e2);});unsigned _tmp14A=e->loc;_tmp147(_tmp148,_tmp149,_tmp14A);});goto _LL0;}case 8U: _LLD: _tmp133=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmpCD)->f1;_tmp134=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmpCD)->f2;_LLE: {struct Cyc_Absyn_Exp*e1=_tmp133;struct Cyc_Absyn_Exp*e2=_tmp134;
new_e=({struct Cyc_Absyn_Exp*(*_tmp14B)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_or_exp;struct Cyc_Absyn_Exp*_tmp14C=({Cyc_Tcutil_deep_copy_exp(pt,e1);});struct Cyc_Absyn_Exp*_tmp14D=({Cyc_Tcutil_deep_copy_exp(pt,e2);});unsigned _tmp14E=e->loc;_tmp14B(_tmp14C,_tmp14D,_tmp14E);});goto _LL0;}case 9U: _LLF: _tmp131=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmpCD)->f1;_tmp132=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmpCD)->f2;_LL10: {struct Cyc_Absyn_Exp*e1=_tmp131;struct Cyc_Absyn_Exp*e2=_tmp132;
new_e=({struct Cyc_Absyn_Exp*(*_tmp14F)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_seq_exp;struct Cyc_Absyn_Exp*_tmp150=({Cyc_Tcutil_deep_copy_exp(pt,e1);});struct Cyc_Absyn_Exp*_tmp151=({Cyc_Tcutil_deep_copy_exp(pt,e2);});unsigned _tmp152=e->loc;_tmp14F(_tmp150,_tmp151,_tmp152);});goto _LL0;}case 34U: _LL11: _tmp12D=((struct Cyc_Absyn_Spawn_e_Absyn_Raw_exp_struct*)_tmpCD)->f1;_tmp12E=((struct Cyc_Absyn_Spawn_e_Absyn_Raw_exp_struct*)_tmpCD)->f2;_tmp12F=(((struct Cyc_Absyn_Spawn_e_Absyn_Raw_exp_struct*)_tmpCD)->f3)->f1;_tmp130=(((struct Cyc_Absyn_Spawn_e_Absyn_Raw_exp_struct*)_tmpCD)->f3)->f2;_LL12: {struct Cyc_Absyn_Exp*e1=_tmp12D;struct Cyc_Absyn_Exp*e2=_tmp12E;struct Cyc_List_List*env_eff_in=_tmp12F;struct Cyc_List_List*env_eff_out=_tmp130;
new_e=({struct Cyc_Absyn_Exp*(*_tmp153)(struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2,struct Cyc_List_List*f1,struct Cyc_List_List*f2,unsigned loc)=Cyc_Absyn_spawn_exp;struct Cyc_Absyn_Exp*_tmp154=({Cyc_Tcutil_deep_copy_exp(pt,e1);});struct Cyc_Absyn_Exp*_tmp155=({Cyc_Tcutil_deep_copy_exp(pt,e2);});struct Cyc_List_List*_tmp156=({Cyc_Absyn_copy_effect(env_eff_in);});struct Cyc_List_List*_tmp157=({Cyc_Absyn_copy_effect(env_eff_out);});unsigned _tmp158=e->loc;_tmp153(_tmp154,_tmp155,_tmp156,_tmp157,_tmp158);});goto _LL0;}case 6U: _LL13: _tmp12A=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmpCD)->f1;_tmp12B=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmpCD)->f2;_tmp12C=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmpCD)->f3;_LL14: {struct Cyc_Absyn_Exp*e1=_tmp12A;struct Cyc_Absyn_Exp*e2=_tmp12B;struct Cyc_Absyn_Exp*e3=_tmp12C;
# 499
new_e=({struct Cyc_Absyn_Exp*(*_tmp159)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_conditional_exp;struct Cyc_Absyn_Exp*_tmp15A=({Cyc_Tcutil_deep_copy_exp(pt,e1);});struct Cyc_Absyn_Exp*_tmp15B=({Cyc_Tcutil_deep_copy_exp(pt,e2);});struct Cyc_Absyn_Exp*_tmp15C=({Cyc_Tcutil_deep_copy_exp(pt,e3);});unsigned _tmp15D=e->loc;_tmp159(_tmp15A,_tmp15B,_tmp15C,_tmp15D);});goto _LL0;}case 4U: _LL15: _tmp127=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmpCD)->f1;_tmp128=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmpCD)->f2;_tmp129=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmpCD)->f3;_LL16: {struct Cyc_Absyn_Exp*e1=_tmp127;struct Cyc_Core_Opt*po=_tmp128;struct Cyc_Absyn_Exp*e2=_tmp129;
# 501
new_e=({struct Cyc_Absyn_Exp*(*_tmp15E)(struct Cyc_Absyn_Exp*,struct Cyc_Core_Opt*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_assignop_exp;struct Cyc_Absyn_Exp*_tmp15F=({Cyc_Tcutil_deep_copy_exp(pt,e1);});struct Cyc_Core_Opt*_tmp160=(unsigned)po?({struct Cyc_Core_Opt*_tmp161=_cycalloc(sizeof(*_tmp161));_tmp161->v=(void*)po->v;_tmp161;}): 0;struct Cyc_Absyn_Exp*_tmp162=({Cyc_Tcutil_deep_copy_exp(pt,e2);});unsigned _tmp163=e->loc;_tmp15E(_tmp15F,_tmp160,_tmp162,_tmp163);});
goto _LL0;}case 10U: if(((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmpCD)->f3 != 0){_LL17: _tmp11C=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmpCD)->f1;_tmp11D=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmpCD)->f2;_tmp11E=(((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmpCD)->f3)->num_varargs;_tmp11F=(((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmpCD)->f3)->injectors;_tmp120=((((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmpCD)->f3)->vai)->name;_tmp121=((((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmpCD)->f3)->vai)->tq;_tmp122=((((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmpCD)->f3)->vai)->type;_tmp123=((((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmpCD)->f3)->vai)->inject;_tmp124=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmpCD)->f4;_tmp125=(((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmpCD)->f5)->f1;_tmp126=(((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmpCD)->f5)->f2;_LL18: {struct Cyc_Absyn_Exp*e1=_tmp11C;struct Cyc_List_List*es=_tmp11D;int n=_tmp11E;struct Cyc_List_List*is=_tmp11F;struct _fat_ptr*nm=_tmp120;struct Cyc_Absyn_Tqual tq=_tmp121;void*t=_tmp122;int i=_tmp123;int resolved=_tmp124;struct Cyc_List_List*env_eff_in=_tmp125;struct Cyc_List_List*env_eff_out=_tmp126;
# 504
new_e=({struct Cyc_Absyn_Exp*(*_tmp164)(void*,unsigned)=Cyc_Absyn_new_exp;void*_tmp165=(void*)({struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*_tmp16A=_cycalloc(sizeof(*_tmp16A));_tmp16A->tag=10U,({
struct Cyc_Absyn_Exp*_tmp72A=({Cyc_Tcutil_deep_copy_exp(pt,e1);});_tmp16A->f1=_tmp72A;}),({struct Cyc_List_List*_tmp729=({({struct Cyc_List_List*(*_tmp728)(struct Cyc_Absyn_Exp*(*f)(int,struct Cyc_Absyn_Exp*),int env,struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp166)(struct Cyc_Absyn_Exp*(*f)(int,struct Cyc_Absyn_Exp*),int env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_Exp*(*f)(int,struct Cyc_Absyn_Exp*),int env,struct Cyc_List_List*x))Cyc_List_map_c;_tmp166;});int _tmp727=pt;_tmp728(Cyc_Tcutil_deep_copy_exp,_tmp727,es);});});_tmp16A->f2=_tmp729;}),({
struct Cyc_Absyn_VarargCallInfo*_tmp726=({struct Cyc_Absyn_VarargCallInfo*_tmp168=_cycalloc(sizeof(*_tmp168));_tmp168->num_varargs=n,_tmp168->injectors=is,({
struct Cyc_Absyn_VarargInfo*_tmp725=({struct Cyc_Absyn_VarargInfo*_tmp167=_cycalloc(sizeof(*_tmp167));_tmp167->name=nm,_tmp167->tq=tq,({void*_tmp724=({Cyc_Tcutil_copy_type(t);});_tmp167->type=_tmp724;}),_tmp167->inject=i;_tmp167;});_tmp168->vai=_tmp725;});_tmp168;});
# 506
_tmp16A->f3=_tmp726;}),_tmp16A->f4=resolved,({
# 508
struct _tuple0*_tmp723=({struct _tuple0*_tmp169=_cycalloc(sizeof(*_tmp169));({struct Cyc_List_List*_tmp722=({Cyc_Absyn_copy_effect(env_eff_in);});_tmp169->f1=_tmp722;}),({struct Cyc_List_List*_tmp721=({Cyc_Absyn_copy_effect(env_eff_out);});_tmp169->f2=_tmp721;});_tmp169;});_tmp16A->f5=_tmp723;});_tmp16A;});unsigned _tmp16B=e->loc;_tmp164(_tmp165,_tmp16B);});
# 510
goto _LL0;}}else{_LL19: _tmp116=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmpCD)->f1;_tmp117=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmpCD)->f2;_tmp118=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmpCD)->f3;_tmp119=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmpCD)->f4;_tmp11A=(((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmpCD)->f5)->f1;_tmp11B=(((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmpCD)->f5)->f2;_LL1A: {struct Cyc_Absyn_Exp*e1=_tmp116;struct Cyc_List_List*es=_tmp117;struct Cyc_Absyn_VarargCallInfo*vci=_tmp118;int resolved=_tmp119;struct Cyc_List_List*env_eff_in=_tmp11A;struct Cyc_List_List*env_eff_out=_tmp11B;
# 513
new_e=({struct Cyc_Absyn_Exp*(*_tmp16C)(void*,unsigned)=Cyc_Absyn_new_exp;void*_tmp16D=(void*)({struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*_tmp170=_cycalloc(sizeof(*_tmp170));_tmp170->tag=10U,({struct Cyc_Absyn_Exp*_tmp731=({Cyc_Tcutil_deep_copy_exp(pt,e1);});_tmp170->f1=_tmp731;}),({struct Cyc_List_List*_tmp730=({({struct Cyc_List_List*(*_tmp72F)(struct Cyc_Absyn_Exp*(*f)(int,struct Cyc_Absyn_Exp*),int env,struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp16E)(struct Cyc_Absyn_Exp*(*f)(int,struct Cyc_Absyn_Exp*),int env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_Exp*(*f)(int,struct Cyc_Absyn_Exp*),int env,struct Cyc_List_List*x))Cyc_List_map_c;_tmp16E;});int _tmp72E=pt;_tmp72F(Cyc_Tcutil_deep_copy_exp,_tmp72E,es);});});_tmp170->f2=_tmp730;}),_tmp170->f3=vci,_tmp170->f4=resolved,({
struct _tuple0*_tmp72D=({struct _tuple0*_tmp16F=_cycalloc(sizeof(*_tmp16F));({struct Cyc_List_List*_tmp72C=({Cyc_Absyn_copy_effect(env_eff_in);});_tmp16F->f1=_tmp72C;}),({struct Cyc_List_List*_tmp72B=({Cyc_Absyn_copy_effect(env_eff_out);});_tmp16F->f2=_tmp72B;});_tmp16F;});_tmp170->f5=_tmp72D;});_tmp170;});unsigned _tmp171=e->loc;_tmp16C(_tmp16D,_tmp171);});
# 516
goto _LL0;}}case 11U: _LL1B: _tmp114=((struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct*)_tmpCD)->f1;_tmp115=((struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct*)_tmpCD)->f2;_LL1C: {struct Cyc_Absyn_Exp*e1=_tmp114;int b=_tmp115;
# 518
new_e=b?({struct Cyc_Absyn_Exp*(*_tmp172)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_rethrow_exp;struct Cyc_Absyn_Exp*_tmp173=({Cyc_Tcutil_deep_copy_exp(pt,e1);});unsigned _tmp174=e->loc;_tmp172(_tmp173,_tmp174);}):({struct Cyc_Absyn_Exp*(*_tmp175)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_throw_exp;struct Cyc_Absyn_Exp*_tmp176=({Cyc_Tcutil_deep_copy_exp(pt,e1);});unsigned _tmp177=e->loc;_tmp175(_tmp176,_tmp177);});
goto _LL0;}case 12U: _LL1D: _tmp113=((struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*)_tmpCD)->f1;_LL1E: {struct Cyc_Absyn_Exp*e1=_tmp113;
# 521
new_e=({struct Cyc_Absyn_Exp*(*_tmp178)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_noinstantiate_exp;struct Cyc_Absyn_Exp*_tmp179=({Cyc_Tcutil_deep_copy_exp(pt,e1);});unsigned _tmp17A=e->loc;_tmp178(_tmp179,_tmp17A);});goto _LL0;}case 13U: _LL1F: _tmp111=((struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*)_tmpCD)->f1;_tmp112=((struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*)_tmpCD)->f2;_LL20: {struct Cyc_Absyn_Exp*e1=_tmp111;struct Cyc_List_List*ts=_tmp112;
# 523
new_e=({struct Cyc_Absyn_Exp*(*_tmp17B)(struct Cyc_Absyn_Exp*,struct Cyc_List_List*,unsigned)=Cyc_Absyn_instantiate_exp;struct Cyc_Absyn_Exp*_tmp17C=({Cyc_Tcutil_deep_copy_exp(pt,e1);});struct Cyc_List_List*_tmp17D=({Cyc_List_map(Cyc_Tcutil_copy_type,ts);});unsigned _tmp17E=e->loc;_tmp17B(_tmp17C,_tmp17D,_tmp17E);});goto _LL0;}case 14U: _LL21: _tmp10D=(void*)((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmpCD)->f1;_tmp10E=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmpCD)->f2;_tmp10F=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmpCD)->f3;_tmp110=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmpCD)->f4;_LL22: {void*t=_tmp10D;struct Cyc_Absyn_Exp*e1=_tmp10E;int b=_tmp10F;enum Cyc_Absyn_Coercion c=_tmp110;
# 525
new_e=({struct Cyc_Absyn_Exp*(*_tmp17F)(void*,struct Cyc_Absyn_Exp*,int user_cast,enum Cyc_Absyn_Coercion,unsigned)=Cyc_Absyn_cast_exp;void*_tmp180=({Cyc_Tcutil_copy_type(t);});struct Cyc_Absyn_Exp*_tmp181=({Cyc_Tcutil_deep_copy_exp(pt,e1);});int _tmp182=b;enum Cyc_Absyn_Coercion _tmp183=c;unsigned _tmp184=e->loc;_tmp17F(_tmp180,_tmp181,_tmp182,_tmp183,_tmp184);});goto _LL0;}case 16U: _LL23: _tmp10B=((struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct*)_tmpCD)->f1;_tmp10C=((struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct*)_tmpCD)->f2;_LL24: {struct Cyc_Absyn_Exp*eo=_tmp10B;struct Cyc_Absyn_Exp*e1=_tmp10C;
# 527
new_e=({struct Cyc_Absyn_Exp*(*_tmp185)(struct Cyc_Absyn_Exp*rgn_handle,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_New_exp;struct Cyc_Absyn_Exp*_tmp186=(unsigned)eo?({Cyc_Tcutil_deep_copy_exp(pt,eo);}): 0;struct Cyc_Absyn_Exp*_tmp187=({Cyc_Tcutil_deep_copy_exp(pt,e1);});unsigned _tmp188=e->loc;_tmp185(_tmp186,_tmp187,_tmp188);});goto _LL0;}case 15U: _LL25: _tmp10A=((struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct*)_tmpCD)->f1;_LL26: {struct Cyc_Absyn_Exp*e1=_tmp10A;
new_e=({struct Cyc_Absyn_Exp*(*_tmp189)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_address_exp;struct Cyc_Absyn_Exp*_tmp18A=({Cyc_Tcutil_deep_copy_exp(pt,e1);});unsigned _tmp18B=e->loc;_tmp189(_tmp18A,_tmp18B);});goto _LL0;}case 17U: _LL27: _tmp109=(void*)((struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct*)_tmpCD)->f1;_LL28: {void*t=_tmp109;
new_e=({struct Cyc_Absyn_Exp*(*_tmp18C)(void*t,unsigned)=Cyc_Absyn_sizeoftype_exp;void*_tmp18D=({Cyc_Tcutil_copy_type(t);});unsigned _tmp18E=e->loc;_tmp18C(_tmp18D,_tmp18E);});goto _LL0;}case 18U: _LL29: _tmp108=((struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct*)_tmpCD)->f1;_LL2A: {struct Cyc_Absyn_Exp*e1=_tmp108;
new_e=({struct Cyc_Absyn_Exp*(*_tmp18F)(struct Cyc_Absyn_Exp*e,unsigned)=Cyc_Absyn_sizeofexp_exp;struct Cyc_Absyn_Exp*_tmp190=({Cyc_Tcutil_deep_copy_exp(pt,e1);});unsigned _tmp191=e->loc;_tmp18F(_tmp190,_tmp191);});goto _LL0;}case 19U: _LL2B: _tmp106=(void*)((struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct*)_tmpCD)->f1;_tmp107=((struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct*)_tmpCD)->f2;_LL2C: {void*t=_tmp106;struct Cyc_List_List*ofs=_tmp107;
new_e=({struct Cyc_Absyn_Exp*(*_tmp192)(void*,struct Cyc_List_List*,unsigned)=Cyc_Absyn_offsetof_exp;void*_tmp193=({Cyc_Tcutil_copy_type(t);});struct Cyc_List_List*_tmp194=ofs;unsigned _tmp195=e->loc;_tmp192(_tmp193,_tmp194,_tmp195);});goto _LL0;}case 20U: _LL2D: _tmp105=((struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_tmpCD)->f1;_LL2E: {struct Cyc_Absyn_Exp*e1=_tmp105;
new_e=({struct Cyc_Absyn_Exp*(*_tmp196)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_deref_exp;struct Cyc_Absyn_Exp*_tmp197=({Cyc_Tcutil_deep_copy_exp(pt,e1);});unsigned _tmp198=e->loc;_tmp196(_tmp197,_tmp198);});goto _LL0;}case 42U: _LL2F: _tmp104=((struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct*)_tmpCD)->f1;_LL30: {struct Cyc_Absyn_Exp*e1=_tmp104;
new_e=({struct Cyc_Absyn_Exp*(*_tmp199)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_extension_exp;struct Cyc_Absyn_Exp*_tmp19A=({Cyc_Tcutil_deep_copy_exp(pt,e1);});unsigned _tmp19B=e->loc;_tmp199(_tmp19A,_tmp19B);});goto _LL0;}case 24U: _LL31: _tmp103=((struct Cyc_Absyn_Tuple_e_Absyn_Raw_exp_struct*)_tmpCD)->f1;_LL32: {struct Cyc_List_List*es=_tmp103;
new_e=({struct Cyc_Absyn_Exp*(*_tmp19C)(struct Cyc_List_List*,unsigned)=Cyc_Absyn_tuple_exp;struct Cyc_List_List*_tmp19D=({({struct Cyc_List_List*(*_tmp733)(struct Cyc_Absyn_Exp*(*f)(int,struct Cyc_Absyn_Exp*),int env,struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp19E)(struct Cyc_Absyn_Exp*(*f)(int,struct Cyc_Absyn_Exp*),int env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_Exp*(*f)(int,struct Cyc_Absyn_Exp*),int env,struct Cyc_List_List*x))Cyc_List_map_c;_tmp19E;});int _tmp732=pt;_tmp733(Cyc_Tcutil_deep_copy_exp,_tmp732,es);});});unsigned _tmp19F=e->loc;_tmp19C(_tmp19D,_tmp19F);});goto _LL0;}case 21U: _LL33: _tmpFF=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmpCD)->f1;_tmp100=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmpCD)->f2;_tmp101=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmpCD)->f3;_tmp102=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmpCD)->f4;_LL34: {struct Cyc_Absyn_Exp*e1=_tmpFF;struct _fat_ptr*n=_tmp100;int f1=_tmp101;int f2=_tmp102;
# 536
new_e=({struct Cyc_Absyn_Exp*(*_tmp1A0)(void*,unsigned)=Cyc_Absyn_new_exp;void*_tmp1A1=(void*)({struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*_tmp1A2=_cycalloc(sizeof(*_tmp1A2));_tmp1A2->tag=21U,({struct Cyc_Absyn_Exp*_tmp734=({Cyc_Tcutil_deep_copy_exp(pt,e1);});_tmp1A2->f1=_tmp734;}),_tmp1A2->f2=n,_tmp1A2->f3=f1,_tmp1A2->f4=f2;_tmp1A2;});unsigned _tmp1A3=e->loc;_tmp1A0(_tmp1A1,_tmp1A3);});goto _LL0;}case 22U: _LL35: _tmpFB=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmpCD)->f1;_tmpFC=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmpCD)->f2;_tmpFD=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmpCD)->f3;_tmpFE=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmpCD)->f4;_LL36: {struct Cyc_Absyn_Exp*e1=_tmpFB;struct _fat_ptr*n=_tmpFC;int f1=_tmpFD;int f2=_tmpFE;
# 538
new_e=({struct Cyc_Absyn_Exp*(*_tmp1A4)(void*,unsigned)=Cyc_Absyn_new_exp;void*_tmp1A5=(void*)({struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*_tmp1A6=_cycalloc(sizeof(*_tmp1A6));_tmp1A6->tag=22U,({struct Cyc_Absyn_Exp*_tmp735=({Cyc_Tcutil_deep_copy_exp(pt,e1);});_tmp1A6->f1=_tmp735;}),_tmp1A6->f2=n,_tmp1A6->f3=f1,_tmp1A6->f4=f2;_tmp1A6;});unsigned _tmp1A7=e->loc;_tmp1A4(_tmp1A5,_tmp1A7);});goto _LL0;}case 23U: _LL37: _tmpF9=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmpCD)->f1;_tmpFA=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmpCD)->f2;_LL38: {struct Cyc_Absyn_Exp*e1=_tmpF9;struct Cyc_Absyn_Exp*e2=_tmpFA;
# 540
new_e=({struct Cyc_Absyn_Exp*(*_tmp1A8)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_subscript_exp;struct Cyc_Absyn_Exp*_tmp1A9=({Cyc_Tcutil_deep_copy_exp(pt,e1);});struct Cyc_Absyn_Exp*_tmp1AA=({Cyc_Tcutil_deep_copy_exp(pt,e2);});unsigned _tmp1AB=e->loc;_tmp1A8(_tmp1A9,_tmp1AA,_tmp1AB);});goto _LL0;}case 26U: _LL39: _tmpF8=((struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*)_tmpCD)->f1;_LL3A: {struct Cyc_List_List*eds=_tmpF8;
# 542
new_e=({struct Cyc_Absyn_Exp*(*_tmp1AC)(void*,unsigned)=Cyc_Absyn_new_exp;void*_tmp1AD=(void*)({struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*_tmp1AF=_cycalloc(sizeof(*_tmp1AF));_tmp1AF->tag=26U,({struct Cyc_List_List*_tmp738=({({struct Cyc_List_List*(*_tmp737)(struct _tuple16*(*f)(int,struct _tuple16*),int env,struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp1AE)(struct _tuple16*(*f)(int,struct _tuple16*),int env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct _tuple16*(*f)(int,struct _tuple16*),int env,struct Cyc_List_List*x))Cyc_List_map_c;_tmp1AE;});int _tmp736=pt;_tmp737(Cyc_Tcutil_copy_eds,_tmp736,eds);});});_tmp1AF->f1=_tmp738;});_tmp1AF;});unsigned _tmp1B0=e->loc;_tmp1AC(_tmp1AD,_tmp1B0);});goto _LL0;}case 31U: _LL3B: _tmpF5=((struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*)_tmpCD)->f1;_tmpF6=((struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*)_tmpCD)->f2;_tmpF7=((struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*)_tmpCD)->f3;_LL3C: {struct Cyc_List_List*es=_tmpF5;struct Cyc_Absyn_Datatypedecl*dtd=_tmpF6;struct Cyc_Absyn_Datatypefield*dtf=_tmpF7;
# 544
new_e=({struct Cyc_Absyn_Exp*(*_tmp1B1)(void*,unsigned)=Cyc_Absyn_new_exp;void*_tmp1B2=(void*)({struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*_tmp1B4=_cycalloc(sizeof(*_tmp1B4));_tmp1B4->tag=31U,({struct Cyc_List_List*_tmp73B=({({struct Cyc_List_List*(*_tmp73A)(struct Cyc_Absyn_Exp*(*f)(int,struct Cyc_Absyn_Exp*),int env,struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp1B3)(struct Cyc_Absyn_Exp*(*f)(int,struct Cyc_Absyn_Exp*),int env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_Exp*(*f)(int,struct Cyc_Absyn_Exp*),int env,struct Cyc_List_List*x))Cyc_List_map_c;_tmp1B3;});int _tmp739=pt;_tmp73A(Cyc_Tcutil_deep_copy_exp,_tmp739,es);});});_tmp1B4->f1=_tmp73B;}),_tmp1B4->f2=dtd,_tmp1B4->f3=dtf;_tmp1B4;});unsigned _tmp1B5=e->loc;_tmp1B1(_tmp1B2,_tmp1B5);});goto _LL0;}case 27U: _LL3D: _tmpF1=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmpCD)->f1;_tmpF2=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmpCD)->f2;_tmpF3=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmpCD)->f3;_tmpF4=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmpCD)->f4;_LL3E: {struct Cyc_Absyn_Vardecl*vd=_tmpF1;struct Cyc_Absyn_Exp*e1=_tmpF2;struct Cyc_Absyn_Exp*e2=_tmpF3;int b=_tmpF4;
# 546
new_e=({struct Cyc_Absyn_Exp*(*_tmp1B6)(void*,unsigned)=Cyc_Absyn_new_exp;void*_tmp1B7=(void*)({struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*_tmp1B8=_cycalloc(sizeof(*_tmp1B8));_tmp1B8->tag=27U,_tmp1B8->f1=vd,({struct Cyc_Absyn_Exp*_tmp73D=({Cyc_Tcutil_deep_copy_exp(pt,e1);});_tmp1B8->f2=_tmp73D;}),({struct Cyc_Absyn_Exp*_tmp73C=({Cyc_Tcutil_deep_copy_exp(pt,e2);});_tmp1B8->f3=_tmp73C;}),_tmp1B8->f4=b;_tmp1B8;});unsigned _tmp1B9=e->loc;_tmp1B6(_tmp1B7,_tmp1B9);});goto _LL0;}case 28U: _LL3F: _tmpEE=((struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*)_tmpCD)->f1;_tmpEF=(void*)((struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*)_tmpCD)->f2;_tmpF0=((struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*)_tmpCD)->f3;_LL40: {struct Cyc_Absyn_Exp*e=_tmpEE;void*t=_tmpEF;int b=_tmpF0;
# 548
new_e=({struct Cyc_Absyn_Exp*(*_tmp1BA)(void*,unsigned)=Cyc_Absyn_new_exp;void*_tmp1BB=(void*)({struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*_tmp1BC=_cycalloc(sizeof(*_tmp1BC));_tmp1BC->tag=28U,({struct Cyc_Absyn_Exp*_tmp73F=({Cyc_Tcutil_deep_copy_exp(pt,e);});_tmp1BC->f1=_tmp73F;}),({void*_tmp73E=({Cyc_Tcutil_copy_type(t);});_tmp1BC->f2=_tmp73E;}),_tmp1BC->f3=b;_tmp1BC;});unsigned _tmp1BD=e->loc;_tmp1BA(_tmp1BB,_tmp1BD);});
goto _LL0;}case 29U: _LL41: _tmpEA=((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_tmpCD)->f1;_tmpEB=((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_tmpCD)->f2;_tmpEC=((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_tmpCD)->f3;_tmpED=((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_tmpCD)->f4;_LL42: {struct _tuple1*n=_tmpEA;struct Cyc_List_List*ts=_tmpEB;struct Cyc_List_List*eds=_tmpEC;struct Cyc_Absyn_Aggrdecl*agr=_tmpED;
# 551
new_e=({struct Cyc_Absyn_Exp*(*_tmp1BE)(void*,unsigned)=Cyc_Absyn_new_exp;void*_tmp1BF=(void*)({struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*_tmp1C1=_cycalloc(sizeof(*_tmp1C1));_tmp1C1->tag=29U,_tmp1C1->f1=n,({struct Cyc_List_List*_tmp743=({Cyc_List_map(Cyc_Tcutil_copy_type,ts);});_tmp1C1->f2=_tmp743;}),({struct Cyc_List_List*_tmp742=({({struct Cyc_List_List*(*_tmp741)(struct _tuple16*(*f)(int,struct _tuple16*),int env,struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp1C0)(struct _tuple16*(*f)(int,struct _tuple16*),int env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct _tuple16*(*f)(int,struct _tuple16*),int env,struct Cyc_List_List*x))Cyc_List_map_c;_tmp1C0;});int _tmp740=pt;_tmp741(Cyc_Tcutil_copy_eds,_tmp740,eds);});});_tmp1C1->f3=_tmp742;}),_tmp1C1->f4=agr;_tmp1C1;});unsigned _tmp1C2=e->loc;_tmp1BE(_tmp1BF,_tmp1C2);});
# 553
goto _LL0;}case 30U: _LL43: _tmpE8=(void*)((struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct*)_tmpCD)->f1;_tmpE9=((struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct*)_tmpCD)->f2;_LL44: {void*t=_tmpE8;struct Cyc_List_List*eds=_tmpE9;
# 555
new_e=({struct Cyc_Absyn_Exp*(*_tmp1C3)(void*,unsigned)=Cyc_Absyn_new_exp;void*_tmp1C4=(void*)({struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct*_tmp1C6=_cycalloc(sizeof(*_tmp1C6));_tmp1C6->tag=30U,({void*_tmp747=({Cyc_Tcutil_copy_type(t);});_tmp1C6->f1=_tmp747;}),({struct Cyc_List_List*_tmp746=({({struct Cyc_List_List*(*_tmp745)(struct _tuple16*(*f)(int,struct _tuple16*),int env,struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp1C5)(struct _tuple16*(*f)(int,struct _tuple16*),int env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct _tuple16*(*f)(int,struct _tuple16*),int env,struct Cyc_List_List*x))Cyc_List_map_c;_tmp1C5;});int _tmp744=pt;_tmp745(Cyc_Tcutil_copy_eds,_tmp744,eds);});});_tmp1C6->f2=_tmp746;});_tmp1C6;});unsigned _tmp1C7=e->loc;_tmp1C3(_tmp1C4,_tmp1C7);});
goto _LL0;}case 25U: _LL45: _tmpE4=(((struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct*)_tmpCD)->f1)->f1;_tmpE5=(((struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct*)_tmpCD)->f1)->f2;_tmpE6=(((struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct*)_tmpCD)->f1)->f3;_tmpE7=((struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct*)_tmpCD)->f2;_LL46: {struct _fat_ptr*vopt=_tmpE4;struct Cyc_Absyn_Tqual tq=_tmpE5;void*t=_tmpE6;struct Cyc_List_List*eds=_tmpE7;
# 558
new_e=({struct Cyc_Absyn_Exp*(*_tmp1C8)(void*,unsigned)=Cyc_Absyn_new_exp;void*_tmp1C9=(void*)({struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct*_tmp1CC=_cycalloc(sizeof(*_tmp1CC));_tmp1CC->tag=25U,({struct _tuple9*_tmp74C=({struct _tuple9*_tmp1CA=_cycalloc(sizeof(*_tmp1CA));_tmp1CA->f1=vopt,_tmp1CA->f2=tq,({void*_tmp74B=({Cyc_Tcutil_copy_type(t);});_tmp1CA->f3=_tmp74B;});_tmp1CA;});_tmp1CC->f1=_tmp74C;}),({
struct Cyc_List_List*_tmp74A=({({struct Cyc_List_List*(*_tmp749)(struct _tuple16*(*f)(int,struct _tuple16*),int env,struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp1CB)(struct _tuple16*(*f)(int,struct _tuple16*),int env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct _tuple16*(*f)(int,struct _tuple16*),int env,struct Cyc_List_List*x))Cyc_List_map_c;_tmp1CB;});int _tmp748=pt;_tmp749(Cyc_Tcutil_copy_eds,_tmp748,eds);});});_tmp1CC->f2=_tmp74A;});_tmp1CC;});unsigned _tmp1CD=e->loc;_tmp1C8(_tmp1C9,_tmp1CD);});
goto _LL0;}case 32U: _LL47: _tmpE2=((struct Cyc_Absyn_Enum_e_Absyn_Raw_exp_struct*)_tmpCD)->f1;_tmpE3=((struct Cyc_Absyn_Enum_e_Absyn_Raw_exp_struct*)_tmpCD)->f2;_LL48: {struct Cyc_Absyn_Enumdecl*ed=_tmpE2;struct Cyc_Absyn_Enumfield*ef=_tmpE3;
new_e=e;goto _LL0;}case 33U: _LL49: _tmpE0=(void*)((struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct*)_tmpCD)->f1;_tmpE1=((struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct*)_tmpCD)->f2;_LL4A: {void*t=_tmpE0;struct Cyc_Absyn_Enumfield*ef=_tmpE1;
# 563
new_e=({struct Cyc_Absyn_Exp*(*_tmp1CE)(void*,unsigned)=Cyc_Absyn_new_exp;void*_tmp1CF=(void*)({struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct*_tmp1D0=_cycalloc(sizeof(*_tmp1D0));_tmp1D0->tag=33U,({void*_tmp74D=({Cyc_Tcutil_copy_type(t);});_tmp1D0->f1=_tmp74D;}),_tmp1D0->f2=ef;_tmp1D0;});unsigned _tmp1D1=e->loc;_tmp1CE(_tmp1CF,_tmp1D1);});goto _LL0;}case 35U: _LL4B: _tmpDA=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmpCD)->f1).is_calloc;_tmpDB=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmpCD)->f1).rgn;_tmpDC=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmpCD)->f1).elt_type;_tmpDD=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmpCD)->f1).num_elts;_tmpDE=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmpCD)->f1).fat_result;_tmpDF=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmpCD)->f1).inline_call;_LL4C: {int ic=_tmpDA;struct Cyc_Absyn_Exp*r=_tmpDB;void**t=_tmpDC;struct Cyc_Absyn_Exp*n=_tmpDD;int res=_tmpDE;int inlc=_tmpDF;
# 565
struct Cyc_Absyn_Exp*e2=({Cyc_Absyn_copy_exp(e);});
struct Cyc_Absyn_Exp*r1=r;if(r != 0)r1=({Cyc_Tcutil_deep_copy_exp(pt,r);});{void**t1=t;
if(t != 0)t1=({void**_tmp1D2=_cycalloc(sizeof(*_tmp1D2));({void*_tmp74E=({Cyc_Tcutil_copy_type(*t);});*_tmp1D2=_tmp74E;});_tmp1D2;});({void*_tmp74F=(void*)({struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*_tmp1D3=_cycalloc(sizeof(*_tmp1D3));
_tmp1D3->tag=35U,(_tmp1D3->f1).is_calloc=ic,(_tmp1D3->f1).rgn=r1,(_tmp1D3->f1).elt_type=t1,(_tmp1D3->f1).num_elts=n,(_tmp1D3->f1).fat_result=res,(_tmp1D3->f1).inline_call=inlc;_tmp1D3;});
# 567
e2->r=_tmp74F;});
# 569
new_e=e2;
goto _LL0;}}case 36U: _LL4D: _tmpD8=((struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct*)_tmpCD)->f1;_tmpD9=((struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct*)_tmpCD)->f2;_LL4E: {struct Cyc_Absyn_Exp*e1=_tmpD8;struct Cyc_Absyn_Exp*e2=_tmpD9;
new_e=({struct Cyc_Absyn_Exp*(*_tmp1D4)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_swap_exp;struct Cyc_Absyn_Exp*_tmp1D5=({Cyc_Tcutil_deep_copy_exp(pt,e1);});struct Cyc_Absyn_Exp*_tmp1D6=({Cyc_Tcutil_deep_copy_exp(pt,e2);});unsigned _tmp1D7=e->loc;_tmp1D4(_tmp1D5,_tmp1D6,_tmp1D7);});goto _LL0;}case 37U: _LL4F: _tmpD6=((struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*)_tmpCD)->f1;_tmpD7=((struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*)_tmpCD)->f2;_LL50: {struct Cyc_Core_Opt*nopt=_tmpD6;struct Cyc_List_List*eds=_tmpD7;
# 573
struct Cyc_Core_Opt*nopt1=nopt;
if(nopt != 0)nopt1=({struct Cyc_Core_Opt*_tmp1D8=_cycalloc(sizeof(*_tmp1D8));_tmp1D8->v=(struct _tuple1*)nopt->v;_tmp1D8;});new_e=({struct Cyc_Absyn_Exp*(*_tmp1D9)(void*,unsigned)=Cyc_Absyn_new_exp;void*_tmp1DA=(void*)({struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*_tmp1DC=_cycalloc(sizeof(*_tmp1DC));
_tmp1DC->tag=37U,_tmp1DC->f1=nopt1,({struct Cyc_List_List*_tmp752=({({struct Cyc_List_List*(*_tmp751)(struct _tuple16*(*f)(int,struct _tuple16*),int env,struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp1DB)(struct _tuple16*(*f)(int,struct _tuple16*),int env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct _tuple16*(*f)(int,struct _tuple16*),int env,struct Cyc_List_List*x))Cyc_List_map_c;_tmp1DB;});int _tmp750=pt;_tmp751(Cyc_Tcutil_copy_eds,_tmp750,eds);});});_tmp1DC->f2=_tmp752;});_tmp1DC;});unsigned _tmp1DD=e->loc;_tmp1D9(_tmp1DA,_tmp1DD);});
goto _LL0;}case 38U: _LL51: _LL52:
# 578
(int)_throw(({struct Cyc_Core_Failure_exn_struct*_tmp1DF=_cycalloc(sizeof(*_tmp1DF));_tmp1DF->tag=Cyc_Core_Failure,({struct _fat_ptr _tmp753=({const char*_tmp1DE="deep_copy: statement expressions unsupported";_tag_fat(_tmp1DE,sizeof(char),45U);});_tmp1DF->f1=_tmp753;});_tmp1DF;}));case 39U: _LL53: _tmpD4=((struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct*)_tmpCD)->f1;_tmpD5=((struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct*)_tmpCD)->f2;_LL54: {struct Cyc_Absyn_Exp*e1=_tmpD4;struct _fat_ptr*fn=_tmpD5;
new_e=({struct Cyc_Absyn_Exp*(*_tmp1E0)(void*,unsigned)=Cyc_Absyn_new_exp;void*_tmp1E1=(void*)({struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct*_tmp1E2=_cycalloc(sizeof(*_tmp1E2));_tmp1E2->tag=39U,({struct Cyc_Absyn_Exp*_tmp754=({Cyc_Tcutil_deep_copy_exp(pt,e1);});_tmp1E2->f1=_tmp754;}),_tmp1E2->f2=fn;_tmp1E2;});unsigned _tmp1E3=e->loc;_tmp1E0(_tmp1E1,_tmp1E3);});
goto _LL0;}case 40U: _LL55: _tmpD3=(void*)((struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct*)_tmpCD)->f1;_LL56: {void*t=_tmpD3;
new_e=({struct Cyc_Absyn_Exp*(*_tmp1E4)(void*,unsigned)=Cyc_Absyn_valueof_exp;void*_tmp1E5=({Cyc_Tcutil_copy_type(t);});unsigned _tmp1E6=e->loc;_tmp1E4(_tmp1E5,_tmp1E6);});
goto _LL0;}default: _LL57: _tmpCE=((struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct*)_tmpCD)->f1;_tmpCF=((struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct*)_tmpCD)->f2;_tmpD0=((struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct*)_tmpCD)->f3;_tmpD1=((struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct*)_tmpCD)->f4;_tmpD2=((struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct*)_tmpCD)->f5;_LL58: {int v=_tmpCE;struct _fat_ptr t=_tmpCF;struct Cyc_List_List*o=_tmpD0;struct Cyc_List_List*i=_tmpD1;struct Cyc_List_List*c=_tmpD2;
new_e=({Cyc_Absyn_asm_exp(v,t,o,i,c,e->loc);});goto _LL0;}}_LL0:;}
# 586
if(preserve_types){
new_e->topt=e->topt;
new_e->annot=e->annot;}
# 586
return new_e;}struct _tuple17{enum Cyc_Absyn_KindQual f1;enum Cyc_Absyn_KindQual f2;};struct _tuple18{enum Cyc_Absyn_AliasQual f1;enum Cyc_Absyn_AliasQual f2;};
# 481
int Cyc_Tcutil_kind_leq(struct Cyc_Absyn_Kind*ka1,struct Cyc_Absyn_Kind*ka2){
# 602 "tcutil.cyc"
enum Cyc_Absyn_AliasQual _tmp1E9;enum Cyc_Absyn_KindQual _tmp1E8;_LL1: _tmp1E8=ka1->kind;_tmp1E9=ka1->aliasqual;_LL2: {enum Cyc_Absyn_KindQual k1=_tmp1E8;enum Cyc_Absyn_AliasQual a1=_tmp1E9;
enum Cyc_Absyn_AliasQual _tmp1EB;enum Cyc_Absyn_KindQual _tmp1EA;_LL4: _tmp1EA=ka2->kind;_tmp1EB=ka2->aliasqual;_LL5: {enum Cyc_Absyn_KindQual k2=_tmp1EA;enum Cyc_Absyn_AliasQual a2=_tmp1EB;
# 605
if((int)k1 != (int)k2){
struct _tuple17 _stmttmp1D=({struct _tuple17 _tmp6B2;_tmp6B2.f1=k1,_tmp6B2.f2=k2;_tmp6B2;});struct _tuple17 _tmp1EC=_stmttmp1D;switch(_tmp1EC.f1){case Cyc_Absyn_BoxKind: switch(_tmp1EC.f2){case Cyc_Absyn_MemKind: _LL7: _LL8:
 goto _LLA;case Cyc_Absyn_AnyKind: _LL9: _LLA:
 goto _LLC;default: goto _LLF;}case Cyc_Absyn_MemKind: if(_tmp1EC.f2 == Cyc_Absyn_AnyKind){_LLB: _LLC:
 goto _LL6;}else{goto _LLF;}case Cyc_Absyn_XRgnKind: if(_tmp1EC.f2 == Cyc_Absyn_RgnKind){_LLD: _LLE:
 goto _LL6;}else{goto _LLF;}default: _LLF: _LL10:
 return 0;}_LL6:;}
# 605
if((int)a1 != (int)a2){
# 615
struct _tuple18 _stmttmp1E=({struct _tuple18 _tmp6B3;_tmp6B3.f1=a1,_tmp6B3.f2=a2;_tmp6B3;});struct _tuple18 _tmp1ED=_stmttmp1E;switch(_tmp1ED.f1){case Cyc_Absyn_Aliasable: if(_tmp1ED.f2 == Cyc_Absyn_Top){_LL12: _LL13:
 goto _LL15;}else{goto _LL16;}case Cyc_Absyn_Unique: if(_tmp1ED.f2 == Cyc_Absyn_Top){_LL14: _LL15:
 return 1;}else{goto _LL16;}default: _LL16: _LL17:
 return 0;}_LL11:;}
# 605
return 1;}}}
# 481 "tcutil.cyc"
struct Cyc_Absyn_Kind*Cyc_Tcutil_tvar_kind(struct Cyc_Absyn_Tvar*tv,struct Cyc_Absyn_Kind*def){
# 624 "tcutil.cyc"
void*_stmttmp1F=({Cyc_Absyn_compress_kb(tv->kind);});void*_tmp1EF=_stmttmp1F;struct Cyc_Absyn_Kind*_tmp1F0;struct Cyc_Absyn_Kind*_tmp1F1;switch(*((int*)_tmp1EF)){case 0U: _LL1: _tmp1F1=((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp1EF)->f1;_LL2: {struct Cyc_Absyn_Kind*k=_tmp1F1;
return k;}case 2U: _LL3: _tmp1F0=((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp1EF)->f2;_LL4: {struct Cyc_Absyn_Kind*k=_tmp1F0;
return k;}default: _LL5: _LL6:
({void*_tmp755=(void*)({struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*_tmp1F2=_cycalloc(sizeof(*_tmp1F2));_tmp1F2->tag=2U,_tmp1F2->f1=0,_tmp1F2->f2=def;_tmp1F2;});tv->kind=_tmp755;});return def;}_LL0:;}struct _tuple19{struct Cyc_Absyn_Tvar*f1;void*f2;};
# 631
struct _tuple19 Cyc_Tcutil_swap_kind(void*t,void*kb){
void*_stmttmp20=({Cyc_Tcutil_compress(t);});void*_tmp1F4=_stmttmp20;struct Cyc_Absyn_Tvar*_tmp1F5;if(((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp1F4)->tag == 2U){_LL1: _tmp1F5=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp1F4)->f1;_LL2: {struct Cyc_Absyn_Tvar*tv=_tmp1F5;
# 634
void*oldkb=tv->kind;
tv->kind=kb;
return({struct _tuple19 _tmp6B4;_tmp6B4.f1=tv,_tmp6B4.f2=oldkb;_tmp6B4;});}}else{_LL3: _LL4:
({struct Cyc_String_pa_PrintArg_struct _tmp1F9=({struct Cyc_String_pa_PrintArg_struct _tmp6B5;_tmp6B5.tag=0U,({struct _fat_ptr _tmp756=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t);}));_tmp6B5.f1=_tmp756;});_tmp6B5;});void*_tmp1F6[1U];_tmp1F6[0]=& _tmp1F9;({int(*_tmp758)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp1F8)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Tcutil_impos;_tmp1F8;});struct _fat_ptr _tmp757=({const char*_tmp1F7="swap_kind: cannot update the kind of %s";_tag_fat(_tmp1F7,sizeof(char),40U);});_tmp758(_tmp757,_tag_fat(_tmp1F6,sizeof(void*),1U));});});}_LL0:;}
# 643
static struct Cyc_Absyn_Kind*Cyc_Tcutil_field_kind(void*field_type,struct Cyc_List_List*ts,struct Cyc_List_List*tvs){
# 645
struct Cyc_Absyn_Kind*k=({Cyc_Tcutil_type_kind(field_type);});
if(ts != 0 &&(k == & Cyc_Tcutil_ak || k == & Cyc_Tcutil_tak)){
# 649
struct _RegionHandle _tmp1FB=_new_region("temp");struct _RegionHandle*temp=& _tmp1FB;_push_region(temp);
{struct Cyc_List_List*inst=0;
# 652
for(0;tvs != 0;(tvs=tvs->tl,ts=ts->tl)){
struct Cyc_Absyn_Tvar*tv=(struct Cyc_Absyn_Tvar*)tvs->hd;
void*t=(void*)((struct Cyc_List_List*)_check_null(ts))->hd;
struct Cyc_Absyn_Kind*_stmttmp21=({Cyc_Tcutil_tvar_kind(tv,& Cyc_Tcutil_bk);});struct Cyc_Absyn_Kind*_tmp1FC=_stmttmp21;switch(((struct Cyc_Absyn_Kind*)_tmp1FC)->kind){case Cyc_Absyn_IntKind: _LL1: _LL2:
 goto _LL4;case Cyc_Absyn_AnyKind: _LL3: _LL4:
# 658
 inst=({struct Cyc_List_List*_tmp1FE=_region_malloc(temp,sizeof(*_tmp1FE));({struct _tuple19*_tmp759=({struct _tuple19*_tmp1FD=_region_malloc(temp,sizeof(*_tmp1FD));_tmp1FD->f1=tv,_tmp1FD->f2=t;_tmp1FD;});_tmp1FE->hd=_tmp759;}),_tmp1FE->tl=inst;_tmp1FE;});goto _LL0;default: _LL5: _LL6:
 goto _LL0;}_LL0:;}
# 662
if(inst != 0){
field_type=({void*(*_tmp1FF)(struct _RegionHandle*,struct Cyc_List_List*,void*)=Cyc_Tcutil_rsubstitute;struct _RegionHandle*_tmp200=temp;struct Cyc_List_List*_tmp201=({Cyc_List_imp_rev(inst);});void*_tmp202=field_type;_tmp1FF(_tmp200,_tmp201,_tmp202);});
k=({Cyc_Tcutil_type_kind(field_type);});}}
# 650
;_pop_region();}
# 646
return k;}
# 643
struct Cyc_Absyn_Kind*Cyc_Tcutil_type_kind(void*t){
# 676
void*_stmttmp22=({Cyc_Tcutil_compress(t);});void*_tmp204=_stmttmp22;struct Cyc_Absyn_Typedefdecl*_tmp205;struct Cyc_Absyn_Exp*_tmp206;struct Cyc_Absyn_PtrInfo _tmp207;struct Cyc_List_List*_tmp209;void*_tmp208;struct Cyc_Absyn_Tvar*_tmp20A;struct Cyc_Core_Opt*_tmp20B;switch(*((int*)_tmp204)){case 1U: _LL1: _tmp20B=((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp204)->f1;_LL2: {struct Cyc_Core_Opt*k=_tmp20B;
return(struct Cyc_Absyn_Kind*)((struct Cyc_Core_Opt*)_check_null(k))->v;}case 2U: _LL3: _tmp20A=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp204)->f1;_LL4: {struct Cyc_Absyn_Tvar*tv=_tmp20A;
return({Cyc_Tcutil_tvar_kind(tv,& Cyc_Tcutil_bk);});}case 0U: _LL5: _tmp208=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp204)->f1;_tmp209=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp204)->f2;_LL6: {void*c=_tmp208;struct Cyc_List_List*ts=_tmp209;
# 680
void*_tmp20C=c;int _tmp210;struct Cyc_Absyn_AggrdeclImpl*_tmp20F;struct Cyc_List_List*_tmp20E;enum Cyc_Absyn_AggrKind _tmp20D;struct Cyc_Absyn_Kind*_tmp211;enum Cyc_Absyn_Size_of _tmp212;switch(*((int*)_tmp20C)){case 0U: _LL1E: _LL1F:
 return& Cyc_Tcutil_mk;case 1U: _LL20: _tmp212=((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp20C)->f2;_LL21: {enum Cyc_Absyn_Size_of sz=_tmp212;
return((int)sz == (int)2U ||(int)sz == (int)3U)?& Cyc_Tcutil_bk:& Cyc_Tcutil_mk;}case 2U: _LL22: _LL23:
 return& Cyc_Tcutil_mk;case 17U: _LL24: _LL25:
 goto _LL27;case 18U: _LL26: _LL27:
 goto _LL29;case 4U: _LL28: _LL29:
 goto _LL2B;case 3U: _LL2A: _LL2B:
 return& Cyc_Tcutil_bk;case 7U: _LL2C: _LL2D:
 return& Cyc_Tcutil_urk;case 6U: _LL2E: _LL2F:
 return& Cyc_Tcutil_rk;case 8U: _LL30: _LL31:
 return& Cyc_Tcutil_trk;case 19U: _LL32: _tmp211=((struct Cyc_Absyn_BuiltinCon_Absyn_TyCon_struct*)_tmp20C)->f2;_LL33: {struct Cyc_Absyn_Kind*k=_tmp211;
return k;}case 5U: _LL34: _LL35:
 return& Cyc_Tcutil_bk;case 9U: _LL36: _LL37:
 goto _LL39;case 10U: _LL38: _LL39:
 goto _LL3B;case 11U: _LL3A: _LL3B:
 goto _LL3D;case 12U: _LL3C: _LL3D:
 return& Cyc_Tcutil_ek;case 14U: _LL3E: _LL3F:
 goto _LL41;case 13U: _LL40: _LL41:
 return& Cyc_Tcutil_boolk;case 15U: _LL42: _LL43:
 goto _LL45;case 16U: _LL44: _LL45:
 return& Cyc_Tcutil_ptrbk;case 20U: _LL46: _LL47:
 return& Cyc_Tcutil_ak;case 21U: if(((((struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)_tmp20C)->f1).KnownDatatypefield).tag == 2){_LL48: _LL49:
# 703
 return& Cyc_Tcutil_mk;}else{_LL4A: _LL4B:
# 705
({void*_tmp213=0U;({int(*_tmp75B)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp215)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Tcutil_impos;_tmp215;});struct _fat_ptr _tmp75A=({const char*_tmp214="type_kind: Unresolved DatatypeFieldType";_tag_fat(_tmp214,sizeof(char),40U);});_tmp75B(_tmp75A,_tag_fat(_tmp213,sizeof(void*),0U));});});}default: if(((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)_tmp20C)->f1).UnknownAggr).tag == 1){_LL4C: _LL4D:
# 709
 return& Cyc_Tcutil_ak;}else{_LL4E: _tmp20D=(*((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)_tmp20C)->f1).KnownAggr).val)->kind;_tmp20E=(*((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)_tmp20C)->f1).KnownAggr).val)->tvs;_tmp20F=(*((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)_tmp20C)->f1).KnownAggr).val)->impl;_tmp210=(*((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)_tmp20C)->f1).KnownAggr).val)->expected_mem_kind;_LL4F: {enum Cyc_Absyn_AggrKind strOrU=_tmp20D;struct Cyc_List_List*tvs=_tmp20E;struct Cyc_Absyn_AggrdeclImpl*i=_tmp20F;int expected_mem_kind=_tmp210;
# 711
if(i == 0){
if(expected_mem_kind)
return& Cyc_Tcutil_mk;else{
# 715
return& Cyc_Tcutil_ak;}}{
# 711
struct Cyc_List_List*fields=i->fields;
# 718
if(fields == 0)return& Cyc_Tcutil_mk;if((int)strOrU == (int)0U){
# 721
for(0;((struct Cyc_List_List*)_check_null(fields))->tl != 0;fields=fields->tl){;}{
void*last_type=((struct Cyc_Absyn_Aggrfield*)fields->hd)->type;
struct Cyc_Absyn_Kind*k=({Cyc_Tcutil_field_kind(last_type,ts,tvs);});
if(k == & Cyc_Tcutil_ak || k == & Cyc_Tcutil_tak)return k;}}else{
# 728
for(0;fields != 0;fields=fields->tl){
void*type=((struct Cyc_Absyn_Aggrfield*)fields->hd)->type;
struct Cyc_Absyn_Kind*k=({Cyc_Tcutil_field_kind(type,ts,tvs);});
if(k == & Cyc_Tcutil_ak || k == & Cyc_Tcutil_tak)return k;}}
# 734
return& Cyc_Tcutil_mk;}}}}_LL1D:;}case 5U: _LL7: _LL8:
# 736
 return& Cyc_Tcutil_ak;case 7U: _LL9: _LLA:
 return& Cyc_Tcutil_mk;case 3U: _LLB: _tmp207=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp204)->f1;_LLC: {struct Cyc_Absyn_PtrInfo pinfo=_tmp207;
# 739
void*_stmttmp23=({Cyc_Tcutil_compress((pinfo.ptr_atts).bounds);});void*_tmp216=_stmttmp23;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp216)->tag == 0U)switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp216)->f1)){case 15U: _LL51: _LL52: {
# 741
enum Cyc_Absyn_AliasQual _stmttmp24=({Cyc_Tcutil_type_kind((pinfo.ptr_atts).rgn);})->aliasqual;enum Cyc_Absyn_AliasQual _tmp217=_stmttmp24;switch(_tmp217){case Cyc_Absyn_Aliasable: _LL58: _LL59:
 return& Cyc_Tcutil_bk;case Cyc_Absyn_Unique: _LL5A: _LL5B:
 return& Cyc_Tcutil_ubk;case Cyc_Absyn_Top: _LL5C: _LL5D:
 goto _LL5F;default: _LL5E: _LL5F: return& Cyc_Tcutil_tbk;}_LL57:;}case 16U: _LL53: _LL54:
# 747
 goto _LL56;default: goto _LL55;}else{_LL55: _LL56: {
# 749
enum Cyc_Absyn_AliasQual _stmttmp25=({Cyc_Tcutil_type_kind((pinfo.ptr_atts).rgn);})->aliasqual;enum Cyc_Absyn_AliasQual _tmp218=_stmttmp25;switch(_tmp218){case Cyc_Absyn_Aliasable: _LL61: _LL62:
 return& Cyc_Tcutil_mk;case Cyc_Absyn_Unique: _LL63: _LL64:
 return& Cyc_Tcutil_umk;case Cyc_Absyn_Top: _LL65: _LL66:
 goto _LL68;default: _LL67: _LL68: return& Cyc_Tcutil_tmk;}_LL60:;}}_LL50:;}case 9U: _LLD: _LLE:
# 755
 return& Cyc_Tcutil_ik;case 11U: _LLF: _LL10:
# 759
 return& Cyc_Tcutil_ak;case 4U: _LL11: _tmp206=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp204)->f1).num_elts;_LL12: {struct Cyc_Absyn_Exp*num_elts=_tmp206;
# 761
if(num_elts == 0 ||({Cyc_Tcutil_is_const_exp(num_elts);}))return& Cyc_Tcutil_mk;return& Cyc_Tcutil_ak;}case 6U: _LL13: _LL14:
# 763
 return& Cyc_Tcutil_mk;case 8U: _LL15: _tmp205=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp204)->f3;_LL16: {struct Cyc_Absyn_Typedefdecl*td=_tmp205;
# 765
if(td == 0 || td->kind == 0)
({struct Cyc_String_pa_PrintArg_struct _tmp21C=({struct Cyc_String_pa_PrintArg_struct _tmp6B6;_tmp6B6.tag=0U,({struct _fat_ptr _tmp75C=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t);}));_tmp6B6.f1=_tmp75C;});_tmp6B6;});void*_tmp219[1U];_tmp219[0]=& _tmp21C;({int(*_tmp75E)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp21B)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Tcutil_impos;_tmp21B;});struct _fat_ptr _tmp75D=({const char*_tmp21A="type_kind: typedef found: %s";_tag_fat(_tmp21A,sizeof(char),29U);});_tmp75E(_tmp75D,_tag_fat(_tmp219,sizeof(void*),1U));});});
# 765
return(struct Cyc_Absyn_Kind*)((struct Cyc_Core_Opt*)_check_null(td->kind))->v;}default: switch(*((int*)((struct Cyc_Absyn_TypeDecl*)((struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_tmp204)->f1)->r)){case 0U: _LL17: _LL18:
# 768
 return& Cyc_Tcutil_ak;case 1U: _LL19: _LL1A:
 return& Cyc_Tcutil_bk;default: _LL1B: _LL1C:
 return& Cyc_Tcutil_ak;}}_LL0:;}
# 643
int Cyc_Tcutil_kind_eq(struct Cyc_Absyn_Kind*k1,struct Cyc_Absyn_Kind*k2){
# 775
return k1 == k2 ||(int)k1->kind == (int)k2->kind &&(int)k1->aliasqual == (int)k2->aliasqual;}
# 643
int Cyc_Tcutil_same_atts(struct Cyc_List_List*a1,struct Cyc_List_List*a2){
# 779
{struct Cyc_List_List*a=a1;for(0;a != 0;a=a->tl){
if(!({Cyc_List_exists_c(Cyc_Absyn_equal_att,(void*)a->hd,a2);}))return 0;}}
# 779
{
# 781
struct Cyc_List_List*a=a2;
# 779
for(0;a != 0;a=a->tl){
# 782
if(!({Cyc_List_exists_c(Cyc_Absyn_equal_att,(void*)a->hd,a1);}))return 0;}}
# 779
return 1;}
# 643
int Cyc_Tcutil_is_regparm0_att(void*a){
# 787
void*_tmp220=a;if(((struct Cyc_Absyn_Regparm_att_Absyn_Attribute_struct*)_tmp220)->tag == 0U){if(((struct Cyc_Absyn_Regparm_att_Absyn_Attribute_struct*)_tmp220)->f1 == 0){_LL1: _LL2:
 return 1;}else{goto _LL3;}}else{_LL3: _LL4:
 return 0;}_LL0:;}
# 643
int Cyc_Tcutil_equiv_fn_atts(struct Cyc_List_List*a1,struct Cyc_List_List*a2){
# 794
{struct Cyc_List_List*a=a1;for(0;a != 0;a=a->tl){
if(!({Cyc_List_exists_c(Cyc_Absyn_equal_att,(void*)a->hd,a2);})&& !({Cyc_Tcutil_is_regparm0_att((void*)a->hd);}))return 0;}}
# 794
{
# 796
struct Cyc_List_List*a=a2;
# 794
for(0;a != 0;a=a->tl){
# 797
if(!({Cyc_List_exists_c(Cyc_Absyn_equal_att,(void*)a->hd,a1);})&& !({Cyc_Tcutil_is_regparm0_att((void*)a->hd);}))return 0;}}
# 794
return 1;}
# 643
void*Cyc_Tcutil_rgns_of(void*t);
# 804
static void*Cyc_Tcutil_rgns_of_field(struct Cyc_Absyn_Aggrfield*af){
return({Cyc_Tcutil_rgns_of(af->type);});}
# 808
static struct _tuple19*Cyc_Tcutil_region_free_subst(struct Cyc_Absyn_Tvar*tv){
void*t;
{struct Cyc_Absyn_Kind*_stmttmp26=({Cyc_Tcutil_tvar_kind(tv,& Cyc_Tcutil_bk);});struct Cyc_Absyn_Kind*_tmp224=_stmttmp26;switch(((struct Cyc_Absyn_Kind*)_tmp224)->kind){case Cyc_Absyn_RgnKind: switch(((struct Cyc_Absyn_Kind*)_tmp224)->aliasqual){case Cyc_Absyn_Unique: _LL1: _LL2:
 t=Cyc_Absyn_unique_rgn_type;goto _LL0;case Cyc_Absyn_Aliasable: _LL3: _LL4:
 t=Cyc_Absyn_heap_rgn_type;goto _LL0;default: goto _LLD;}case Cyc_Absyn_EffKind: _LL5: _LL6:
 t=Cyc_Absyn_empty_effect;goto _LL0;case Cyc_Absyn_IntKind: _LL7: _LL8:
 t=(void*)({struct Cyc_Absyn_ValueofType_Absyn_Type_struct*_tmp225=_cycalloc(sizeof(*_tmp225));_tmp225->tag=9U,({struct Cyc_Absyn_Exp*_tmp75F=({Cyc_Absyn_uint_exp(0U,0U);});_tmp225->f1=_tmp75F;});_tmp225;});goto _LL0;case Cyc_Absyn_BoolKind: _LL9: _LLA:
 t=Cyc_Absyn_true_type;goto _LL0;case Cyc_Absyn_PtrBndKind: _LLB: _LLC:
 t=Cyc_Absyn_fat_bound_type;goto _LL0;default: _LLD: _LLE:
 t=Cyc_Absyn_sint_type;goto _LL0;}_LL0:;}
# 819
return({struct _tuple19*_tmp226=_cycalloc(sizeof(*_tmp226));_tmp226->f1=tv,_tmp226->f2=t;_tmp226;});}
# 808
void*Cyc_Tcutil_rgns_of(void*t){
# 827
void*_stmttmp27=({Cyc_Tcutil_compress(t);});void*_tmp228=_stmttmp27;struct Cyc_List_List*_tmp229;struct Cyc_List_List*_tmp22A;struct Cyc_List_List*_tmp231;struct Cyc_Absyn_VarargInfo*_tmp230;struct Cyc_List_List*_tmp22F;void*_tmp22E;struct Cyc_Absyn_Tqual _tmp22D;void*_tmp22C;struct Cyc_List_List*_tmp22B;struct Cyc_List_List*_tmp232;void*_tmp233;void*_tmp235;void*_tmp234;struct Cyc_List_List*_tmp236;switch(*((int*)_tmp228)){case 0U: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp228)->f2 == 0){_LL1: _LL2:
 return Cyc_Absyn_empty_effect;}else{if(((struct Cyc_Absyn_JoinCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp228)->f1)->tag == 11U){_LL3: _LL4:
 return t;}else{_LL5: _tmp236=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp228)->f2;_LL6: {struct Cyc_List_List*ts=_tmp236;
# 831
struct Cyc_List_List*new_ts=({Cyc_List_map(Cyc_Tcutil_rgns_of,ts);});
return({void*(*_tmp237)(void*e)=Cyc_Tcutil_normalize_effect;void*_tmp238=({Cyc_Absyn_join_eff(new_ts);});_tmp237(_tmp238);});}}}case 1U: _LL7: _LL8:
 goto _LLA;case 2U: _LL9: _LLA: {
# 835
struct Cyc_Absyn_Kind*_stmttmp28=({Cyc_Tcutil_type_kind(t);});struct Cyc_Absyn_Kind*_tmp239=_stmttmp28;switch(((struct Cyc_Absyn_Kind*)_tmp239)->kind){case Cyc_Absyn_RgnKind: _LL1E: _LL1F:
 return({Cyc_Absyn_access_eff(t);});case Cyc_Absyn_XRgnKind: _LL20: _LL21:
# 841
 return({Cyc_Absyn_access_eff(t);});case Cyc_Absyn_EffKind: _LL22: _LL23:
 return t;case Cyc_Absyn_IntKind: _LL24: _LL25:
 return Cyc_Absyn_empty_effect;default: _LL26: _LL27:
 return({Cyc_Absyn_regionsof_eff(t);});}_LL1D:;}case 3U: _LLB: _tmp234=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp228)->f1).elt_type;_tmp235=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp228)->f1).ptr_atts).rgn;_LLC: {void*et=_tmp234;void*r=_tmp235;
# 848
return({void*(*_tmp23A)(void*e)=Cyc_Tcutil_normalize_effect;void*_tmp23B=({void*(*_tmp23C)(struct Cyc_List_List*)=Cyc_Absyn_join_eff;struct Cyc_List_List*_tmp23D=({void*_tmp23E[2U];({void*_tmp761=({Cyc_Absyn_access_eff(r);});_tmp23E[0]=_tmp761;}),({void*_tmp760=({Cyc_Tcutil_rgns_of(et);});_tmp23E[1]=_tmp760;});Cyc_List_list(_tag_fat(_tmp23E,sizeof(void*),2U));});_tmp23C(_tmp23D);});_tmp23A(_tmp23B);});}case 4U: _LLD: _tmp233=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp228)->f1).elt_type;_LLE: {void*et=_tmp233;
# 850
return({void*(*_tmp23F)(void*e)=Cyc_Tcutil_normalize_effect;void*_tmp240=({Cyc_Tcutil_rgns_of(et);});_tmp23F(_tmp240);});}case 7U: _LLF: _tmp232=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp228)->f2;_LL10: {struct Cyc_List_List*fs=_tmp232;
# 852
return({void*(*_tmp241)(void*e)=Cyc_Tcutil_normalize_effect;void*_tmp242=({void*(*_tmp243)(struct Cyc_List_List*)=Cyc_Absyn_join_eff;struct Cyc_List_List*_tmp244=({({struct Cyc_List_List*(*_tmp762)(void*(*f)(struct Cyc_Absyn_Aggrfield*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp245)(void*(*f)(struct Cyc_Absyn_Aggrfield*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(void*(*f)(struct Cyc_Absyn_Aggrfield*),struct Cyc_List_List*x))Cyc_List_map;_tmp245;});_tmp762(Cyc_Tcutil_rgns_of_field,fs);});});_tmp243(_tmp244);});_tmp241(_tmp242);});}case 5U: _LL11: _tmp22B=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp228)->f1).tvars;_tmp22C=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp228)->f1).effect;_tmp22D=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp228)->f1).ret_tqual;_tmp22E=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp228)->f1).ret_type;_tmp22F=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp228)->f1).args;_tmp230=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp228)->f1).cyc_varargs;_tmp231=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp228)->f1).rgn_po;_LL12: {struct Cyc_List_List*tvs=_tmp22B;void*eff=_tmp22C;struct Cyc_Absyn_Tqual rt_tq=_tmp22D;void*rt=_tmp22E;struct Cyc_List_List*args=_tmp22F;struct Cyc_Absyn_VarargInfo*cyc_varargs=_tmp230;struct Cyc_List_List*rpo=_tmp231;
# 862
void*e=({void*(*_tmp246)(struct Cyc_List_List*,void*)=Cyc_Tcutil_substitute;struct Cyc_List_List*_tmp247=({({struct Cyc_List_List*(*_tmp763)(struct _tuple19*(*f)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp248)(struct _tuple19*(*f)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct _tuple19*(*f)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*x))Cyc_List_map;_tmp248;});_tmp763(Cyc_Tcutil_region_free_subst,tvs);});});void*_tmp249=(void*)_check_null(eff);_tmp246(_tmp247,_tmp249);});
return({Cyc_Tcutil_normalize_effect(e);});}case 6U: _LL13: _tmp22A=((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp228)->f1;_LL14: {struct Cyc_List_List*tqts=_tmp22A;
# 865
struct Cyc_List_List*ts=0;
for(0;tqts != 0;tqts=tqts->tl){
ts=({struct Cyc_List_List*_tmp24A=_cycalloc(sizeof(*_tmp24A));_tmp24A->hd=(*((struct _tuple12*)tqts->hd)).f2,_tmp24A->tl=ts;_tmp24A;});}
_tmp229=ts;goto _LL16;}case 8U: _LL15: _tmp229=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp228)->f2;_LL16: {struct Cyc_List_List*ts=_tmp229;
# 870
return({void*(*_tmp24B)(void*e)=Cyc_Tcutil_normalize_effect;void*_tmp24C=({void*(*_tmp24D)(struct Cyc_List_List*)=Cyc_Absyn_join_eff;struct Cyc_List_List*_tmp24E=({Cyc_List_map(Cyc_Tcutil_rgns_of,ts);});_tmp24D(_tmp24E);});_tmp24B(_tmp24C);});}case 10U: _LL17: _LL18:
({void*_tmp24F=0U;({int(*_tmp765)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp251)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Tcutil_impos;_tmp251;});struct _fat_ptr _tmp764=({const char*_tmp250="typedecl in rgns_of";_tag_fat(_tmp250,sizeof(char),20U);});_tmp765(_tmp764,_tag_fat(_tmp24F,sizeof(void*),0U));});});case 9U: _LL19: _LL1A:
 goto _LL1C;default: _LL1B: _LL1C:
 return Cyc_Absyn_empty_effect;}_LL0:;}
# 808
void*Cyc_Tcutil_normalize_effect(void*e){
# 881
e=({Cyc_Tcutil_compress(e);});{
void*_tmp253=e;void*_tmp254;struct Cyc_List_List**_tmp255;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp253)->tag == 0U)switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp253)->f1)){case 11U: _LL1: _tmp255=(struct Cyc_List_List**)&((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp253)->f2;_LL2: {struct Cyc_List_List**es=_tmp255;
# 884
int redo_join=0;
{struct Cyc_List_List*effs=*es;for(0;effs != 0;effs=effs->tl){
void*eff=(void*)effs->hd;
({void*_tmp766=(void*)({void*(*_tmp256)(void*t)=Cyc_Tcutil_compress;void*_tmp257=({Cyc_Tcutil_normalize_effect(eff);});_tmp256(_tmp257);});effs->hd=_tmp766;});{
void*_stmttmp29=(void*)effs->hd;void*_tmp258=_stmttmp29;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp258)->tag == 0U)switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp258)->f1)){case 11U: _LL8: _LL9:
 goto _LLB;case 9U: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp258)->f2 != 0){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)((struct Cyc_List_List*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp258)->f2)->hd)->tag == 0U)switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)((struct Cyc_List_List*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp258)->f2)->hd)->f1)){case 6U: if(((struct Cyc_List_List*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp258)->f2)->tl == 0){_LLA: _LLB:
 goto _LLD;}else{goto _LL10;}case 8U: if(((struct Cyc_List_List*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp258)->f2)->tl == 0){_LLC: _LLD:
 goto _LLF;}else{goto _LL10;}case 7U: if(((struct Cyc_List_List*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp258)->f2)->tl == 0){_LLE: _LLF:
# 893
 redo_join=1;goto _LL7;}else{goto _LL10;}default: goto _LL10;}else{goto _LL10;}}else{goto _LL10;}default: goto _LL10;}else{_LL10: _LL11:
 goto _LL7;}_LL7:;}}}
# 897
if(!redo_join)return e;{struct Cyc_List_List*effects=0;
# 899
{struct Cyc_List_List*effs=*es;for(0;effs != 0;effs=effs->tl){
void*c=({Cyc_Tcutil_compress((void*)effs->hd);});
void*_tmp259=c;struct Cyc_List_List*_tmp25A;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp259)->tag == 0U)switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp259)->f1)){case 11U: _LL13: _tmp25A=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp259)->f2;_LL14: {struct Cyc_List_List*nested_effs=_tmp25A;
# 903
effects=({Cyc_List_revappend(nested_effs,effects);});
goto _LL12;}case 9U: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp259)->f2 != 0){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)((struct Cyc_List_List*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp259)->f2)->hd)->tag == 0U)switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)((struct Cyc_List_List*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp259)->f2)->hd)->f1)){case 6U: if(((struct Cyc_List_List*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp259)->f2)->tl == 0){_LL15: _LL16:
 goto _LL18;}else{goto _LL1B;}case 8U: if(((struct Cyc_List_List*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp259)->f2)->tl == 0){_LL17: _LL18:
 goto _LL1A;}else{goto _LL1B;}case 7U: if(((struct Cyc_List_List*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp259)->f2)->tl == 0){_LL19: _LL1A:
 goto _LL12;}else{goto _LL1B;}default: goto _LL1B;}else{goto _LL1B;}}else{goto _LL1B;}default: goto _LL1B;}else{_LL1B: _LL1C:
 effects=({struct Cyc_List_List*_tmp25B=_cycalloc(sizeof(*_tmp25B));_tmp25B->hd=c,_tmp25B->tl=effects;_tmp25B;});goto _LL12;}_LL12:;}}
# 911
({struct Cyc_List_List*_tmp767=({Cyc_List_imp_rev(effects);});*es=_tmp767;});
return e;}}case 12U: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp253)->f2 != 0){_LL3: _tmp254=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp253)->f2)->hd;_LL4: {void*t=_tmp254;
# 914
void*_stmttmp2A=({Cyc_Tcutil_compress(t);});void*_tmp25C=_stmttmp2A;switch(*((int*)_tmp25C)){case 1U: _LL1E: _LL1F:
 goto _LL21;case 2U: _LL20: _LL21:
 return e;default: _LL22: _LL23:
 return({Cyc_Tcutil_rgns_of(t);});}_LL1D:;}}else{goto _LL5;}default: goto _LL5;}else{_LL5: _LL6:
# 919
 return e;}_LL0:;}}
# 924
static void*Cyc_Tcutil_dummy_fntype(void*eff){
struct Cyc_Absyn_FnType_Absyn_Type_struct*fntype=({struct Cyc_Absyn_FnType_Absyn_Type_struct*_tmp264=_cycalloc(sizeof(*_tmp264));_tmp264->tag=5U,(_tmp264->f1).tvars=0,(_tmp264->f1).effect=eff,({
struct Cyc_Absyn_Tqual _tmp768=({Cyc_Absyn_empty_tqual(0U);});(_tmp264->f1).ret_tqual=_tmp768;}),(_tmp264->f1).ret_type=Cyc_Absyn_void_type,(_tmp264->f1).args=0,(_tmp264->f1).c_varargs=0,(_tmp264->f1).cyc_varargs=0,(_tmp264->f1).rgn_po=0,(_tmp264->f1).attributes=0,(_tmp264->f1).requires_clause=0,(_tmp264->f1).requires_relns=0,(_tmp264->f1).ensures_clause=0,(_tmp264->f1).ensures_relns=0,(_tmp264->f1).ieffect=0,(_tmp264->f1).oeffect=0,(_tmp264->f1).throws=0,(_tmp264->f1).reentrant=Cyc_Absyn_noreentrant;_tmp264;});
# 939
return({void*(*_tmp25E)(void*t,void*rgn,struct Cyc_Absyn_Tqual tq,void*b,void*zero_term)=Cyc_Absyn_atb_type;void*_tmp25F=(void*)fntype;void*_tmp260=Cyc_Absyn_heap_rgn_type;struct Cyc_Absyn_Tqual _tmp261=({Cyc_Absyn_empty_tqual(0U);});void*_tmp262=({Cyc_Absyn_bounds_one();});void*_tmp263=Cyc_Absyn_false_type;_tmp25E(_tmp25F,_tmp260,_tmp261,_tmp262,_tmp263);});}
# 943
static int Cyc_Tcutil_compare_tyvars(void*r,void*r2,int constrain){
# 945
if(constrain)return({Cyc_Unify_unify(r,r2);});r2=({Cyc_Tcutil_compress(r2);});
# 947
if(r == r2)
# 949
return 1;{
# 947
struct _tuple15 _stmttmp2B=({struct _tuple15 _tmp6B7;_tmp6B7.f1=r,_tmp6B7.f2=r2;_tmp6B7;});struct _tuple15 _tmp266=_stmttmp2B;struct Cyc_Absyn_Tvar*_tmp268;struct Cyc_Absyn_Tvar*_tmp267;if(((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp266.f1)->tag == 2U){if(((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp266.f2)->tag == 2U){_LL1: _tmp267=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp266.f1)->f1;_tmp268=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp266.f2)->f1;_LL2: {struct Cyc_Absyn_Tvar*tv1=_tmp267;struct Cyc_Absyn_Tvar*tv2=_tmp268;
# 953
return({Cyc_Absyn_tvar_cmp(tv1,tv2);})== 0;}}else{goto _LL3;}}else{_LL3: _LL4:
 return 0;}_LL0:;}}
# 943
int Cyc_Tcutil_region_in_effect(int constrain,void*r,void*e){
# 964
r=({Cyc_Tcutil_compress(r);});
# 970
if((r == Cyc_Absyn_heap_rgn_type || r == Cyc_Absyn_unique_rgn_type)|| r == Cyc_Absyn_refcnt_rgn_type)
# 972
return 1;{
# 970
void*ce=({Cyc_Tcutil_compress(e);});
# 975
void*_tmp26A=ce;struct Cyc_Core_Opt*_tmp26D;void**_tmp26C;struct Cyc_Core_Opt*_tmp26B;struct Cyc_List_List*_tmp26E;void*_tmp26F;struct Cyc_List_List*_tmp270;void*_tmp271;switch(*((int*)_tmp26A)){case 0U: switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp26A)->f1)){case 9U: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp26A)->f2 != 0){_LL1: _tmp271=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp26A)->f2)->hd;_LL2: {void*r2=_tmp271;
return({Cyc_Tcutil_compare_tyvars(r,r2,constrain);});}}else{goto _LLB;}case 11U: _LL3: _tmp270=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp26A)->f2;_LL4: {struct Cyc_List_List*es=_tmp270;
# 978
for(0;es != 0;es=es->tl){
if(({Cyc_Tcutil_region_in_effect(constrain,r,(void*)es->hd);}))
# 981
return 1;}
# 978
return 0;}case 12U: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp26A)->f2 != 0){_LL5: _tmp26F=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp26A)->f2)->hd;_LL6: {void*t=_tmp26F;
# 985
void*_stmttmp2C=({Cyc_Tcutil_rgns_of(t);});void*_tmp272=_stmttmp2C;void*_tmp273;void*_tmp274;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp272)->tag == 0U){if(((struct Cyc_Absyn_RgnsCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp272)->f1)->tag == 12U){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp272)->f2 != 0){_LLE: _tmp274=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp272)->f2)->hd;_LLF: {void*t=_tmp274;
# 987
if(!constrain)return 0;{void*_stmttmp2D=({Cyc_Tcutil_compress(t);});void*_tmp275=_stmttmp2D;struct Cyc_Core_Opt*_tmp278;void**_tmp277;struct Cyc_Core_Opt*_tmp276;if(((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp275)->tag == 1U){_LL13: _tmp276=((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp275)->f1;_tmp277=(void**)&((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp275)->f2;_tmp278=((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp275)->f4;_LL14: {struct Cyc_Core_Opt*k=_tmp276;void**p=_tmp277;struct Cyc_Core_Opt*s=_tmp278;
# 992
void*ev=({Cyc_Absyn_new_evar(& Cyc_Tcutil_eko,s);});
# 995
({Cyc_Unify_occurs(ev,Cyc_Core_heap_region,(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(s))->v,r);});
({void*_tmp76A=({void*(*_tmp279)(void*eff)=Cyc_Tcutil_dummy_fntype;void*_tmp27A=({void*(*_tmp27B)(struct Cyc_List_List*)=Cyc_Absyn_join_eff;struct Cyc_List_List*_tmp27C=({void*_tmp27D[2U];_tmp27D[0]=ev,({void*_tmp769=({Cyc_Absyn_access_eff(r);});_tmp27D[1]=_tmp769;});Cyc_List_list(_tag_fat(_tmp27D,sizeof(void*),2U));});_tmp27B(_tmp27C);});_tmp279(_tmp27A);});*p=_tmp76A;});
return 1;}}else{_LL15: _LL16:
 return 0;}_LL12:;}}}else{goto _LL10;}}else{goto _LL10;}}else{_LL10: _tmp273=_tmp272;_LL11: {void*e2=_tmp273;
# 1000
return({Cyc_Tcutil_region_in_effect(constrain,r,e2);});}}_LLD:;}}else{goto _LLB;}case 10U: _LL9: _tmp26E=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp26A)->f2;_LLA: {struct Cyc_List_List*r2=_tmp26E;
# 1015
return 1;}default: goto _LLB;}case 1U: _LL7: _tmp26B=((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp26A)->f1;_tmp26C=(void**)&((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp26A)->f2;_tmp26D=((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp26A)->f4;_LL8: {struct Cyc_Core_Opt*k=_tmp26B;void**p=_tmp26C;struct Cyc_Core_Opt*s=_tmp26D;
# 1003
if(k == 0 ||(int)((struct Cyc_Absyn_Kind*)k->v)->kind != (int)Cyc_Absyn_EffKind)
({void*_tmp27E=0U;({int(*_tmp76C)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp280)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Tcutil_impos;_tmp280;});struct _fat_ptr _tmp76B=({const char*_tmp27F="effect evar has wrong kind";_tag_fat(_tmp27F,sizeof(char),27U);});_tmp76C(_tmp76B,_tag_fat(_tmp27E,sizeof(void*),0U));});});
# 1003
if(!constrain)
# 1005
return 0;{
# 1003
void*ev=({Cyc_Absyn_new_evar(& Cyc_Tcutil_eko,s);});
# 1011
({Cyc_Unify_occurs(ev,Cyc_Core_heap_region,(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(s))->v,r);});{
void*new_typ=({void*(*_tmp281)(struct Cyc_List_List*)=Cyc_Absyn_join_eff;struct Cyc_List_List*_tmp282=({void*_tmp283[2U];_tmp283[0]=ev,({void*_tmp76D=({Cyc_Absyn_access_eff(r);});_tmp283[1]=_tmp76D;});Cyc_List_list(_tag_fat(_tmp283,sizeof(void*),2U));});_tmp281(_tmp282);});
*p=new_typ;
return 1;}}}default: _LLB: _LLC:
# 1016
 return 0;}_LL0:;}}
# 1023
static int Cyc_Tcutil_type_in_effect(int may_constrain_evars,void*t,void*e){
t=({Cyc_Tcutil_compress(t);});{
void*_stmttmp2E=({void*(*_tmp297)(void*e)=Cyc_Tcutil_normalize_effect;void*_tmp298=({Cyc_Tcutil_compress(e);});_tmp297(_tmp298);});void*_tmp285=_stmttmp2E;struct Cyc_Core_Opt*_tmp288;void**_tmp287;struct Cyc_Core_Opt*_tmp286;void*_tmp289;struct Cyc_List_List*_tmp28A;switch(*((int*)_tmp285)){case 0U: switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp285)->f1)){case 9U: _LL1: _LL2:
 return 0;case 11U: _LL3: _tmp28A=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp285)->f2;_LL4: {struct Cyc_List_List*es=_tmp28A;
# 1028
for(0;es != 0;es=es->tl){
if(({Cyc_Tcutil_type_in_effect(may_constrain_evars,t,(void*)es->hd);}))
return 1;}
# 1028
return 0;}case 12U: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp285)->f2 != 0){_LL5: _tmp289=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp285)->f2)->hd;_LL6: {void*t2=_tmp289;
# 1033
t2=({Cyc_Tcutil_compress(t2);});
if(t == t2)return 1;if(may_constrain_evars)
return({Cyc_Unify_unify(t,t2);});{
# 1034
void*_stmttmp2F=({Cyc_Tcutil_rgns_of(t);});void*_tmp28B=_stmttmp2F;void*_tmp28C;void*_tmp28D;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp28B)->tag == 0U){if(((struct Cyc_Absyn_RgnsCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp28B)->f1)->tag == 12U){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp28B)->f2 != 0){_LLC: _tmp28D=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp28B)->f2)->hd;_LLD: {void*t3=_tmp28D;
# 1038
struct _tuple15 _stmttmp30=({struct _tuple15 _tmp6B8;({void*_tmp76E=({Cyc_Tcutil_compress(t3);});_tmp6B8.f1=_tmp76E;}),_tmp6B8.f2=t2;_tmp6B8;});struct _tuple15 _tmp28E=_stmttmp30;struct Cyc_Absyn_Tvar*_tmp290;struct Cyc_Absyn_Tvar*_tmp28F;if(((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp28E.f1)->tag == 2U){if(((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp28E.f2)->tag == 2U){_LL11: _tmp28F=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp28E.f1)->f1;_tmp290=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp28E.f2)->f1;_LL12: {struct Cyc_Absyn_Tvar*tv1=_tmp28F;struct Cyc_Absyn_Tvar*tv2=_tmp290;
return({Cyc_Unify_unify(t,t2);});}}else{goto _LL13;}}else{_LL13: _LL14:
 return t3 == t2;}_LL10:;}}else{goto _LLE;}}else{goto _LLE;}}else{_LLE: _tmp28C=_tmp28B;_LLF: {void*e2=_tmp28C;
# 1042
return({Cyc_Tcutil_type_in_effect(may_constrain_evars,t,e2);});}}_LLB:;}}}else{goto _LL9;}default: goto _LL9;}case 1U: _LL7: _tmp286=((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp285)->f1;_tmp287=(void**)&((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp285)->f2;_tmp288=((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp285)->f4;_LL8: {struct Cyc_Core_Opt*k=_tmp286;void**p=_tmp287;struct Cyc_Core_Opt*s=_tmp288;
# 1045
if(k == 0 ||(int)((struct Cyc_Absyn_Kind*)k->v)->kind != (int)Cyc_Absyn_EffKind)
({void*_tmp291=0U;({int(*_tmp770)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp293)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Tcutil_impos;_tmp293;});struct _fat_ptr _tmp76F=({const char*_tmp292="effect evar has wrong kind";_tag_fat(_tmp292,sizeof(char),27U);});_tmp770(_tmp76F,_tag_fat(_tmp291,sizeof(void*),0U));});});
# 1045
if(!may_constrain_evars)
# 1047
return 0;{
# 1045
void*ev=({Cyc_Absyn_new_evar(& Cyc_Tcutil_eko,s);});
# 1053
({Cyc_Unify_occurs(ev,Cyc_Core_heap_region,(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(s))->v,t);});{
void*new_typ=({void*(*_tmp294)(struct Cyc_List_List*)=Cyc_Absyn_join_eff;struct Cyc_List_List*_tmp295=({void*_tmp296[2U];_tmp296[0]=ev,({void*_tmp771=({Cyc_Absyn_regionsof_eff(t);});_tmp296[1]=_tmp771;});Cyc_List_list(_tag_fat(_tmp296,sizeof(void*),2U));});_tmp294(_tmp295);});
*p=new_typ;
return 1;}}}default: _LL9: _LLA:
 return 0;}_LL0:;}}
# 1064
static int Cyc_Tcutil_variable_in_effect(int may_constrain_evars,struct Cyc_Absyn_Tvar*v,void*e){
e=({Cyc_Tcutil_compress(e);});{
void*_tmp29A=e;struct Cyc_Core_Opt*_tmp29D;void**_tmp29C;struct Cyc_Core_Opt*_tmp29B;void*_tmp29E;void*_tmp29F;struct Cyc_List_List*_tmp2A0;struct Cyc_Absyn_Tvar*_tmp2A1;switch(*((int*)_tmp29A)){case 2U: _LL1: _tmp2A1=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp29A)->f1;_LL2: {struct Cyc_Absyn_Tvar*v2=_tmp2A1;
return({Cyc_Absyn_tvar_cmp(v,v2);})== 0;}case 0U: switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp29A)->f1)){case 11U: _LL3: _tmp2A0=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp29A)->f2;_LL4: {struct Cyc_List_List*es=_tmp2A0;
# 1069
for(0;es != 0;es=es->tl){
if(({Cyc_Tcutil_variable_in_effect(may_constrain_evars,v,(void*)es->hd);}))
return 1;}
# 1069
return 0;}case 12U: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp29A)->f2 != 0){_LL5: _tmp29F=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp29A)->f2)->hd;_LL6: {void*t=_tmp29F;
# 1074
void*_stmttmp31=({Cyc_Tcutil_rgns_of(t);});void*_tmp2A2=_stmttmp31;void*_tmp2A3;void*_tmp2A4;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2A2)->tag == 0U){if(((struct Cyc_Absyn_RgnsCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2A2)->f1)->tag == 12U){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2A2)->f2 != 0){_LLE: _tmp2A4=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2A2)->f2)->hd;_LLF: {void*t2=_tmp2A4;
# 1076
if(!may_constrain_evars)return 0;{void*_stmttmp32=({Cyc_Tcutil_compress(t2);});void*_tmp2A5=_stmttmp32;struct Cyc_Core_Opt*_tmp2A8;void**_tmp2A7;struct Cyc_Core_Opt*_tmp2A6;if(((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp2A5)->tag == 1U){_LL13: _tmp2A6=((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp2A5)->f1;_tmp2A7=(void**)&((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp2A5)->f2;_tmp2A8=((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp2A5)->f4;_LL14: {struct Cyc_Core_Opt*k=_tmp2A6;void**p=_tmp2A7;struct Cyc_Core_Opt*s=_tmp2A8;
# 1082
void*ev=({Cyc_Absyn_new_evar(& Cyc_Tcutil_eko,s);});
# 1084
if(!({({int(*_tmp773)(int(*compare)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_List_List*l,struct Cyc_Absyn_Tvar*x)=({int(*_tmp2A9)(int(*compare)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_List_List*l,struct Cyc_Absyn_Tvar*x)=(int(*)(int(*compare)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_List_List*l,struct Cyc_Absyn_Tvar*x))Cyc_List_mem;_tmp2A9;});struct Cyc_List_List*_tmp772=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(s))->v;_tmp773(Cyc_Tcutil_fast_tvar_cmp,_tmp772,v);});}))return 0;({void*_tmp775=({void*(*_tmp2AA)(void*eff)=Cyc_Tcutil_dummy_fntype;void*_tmp2AB=({void*(*_tmp2AC)(struct Cyc_List_List*)=Cyc_Absyn_join_eff;struct Cyc_List_List*_tmp2AD=({void*_tmp2AE[2U];_tmp2AE[0]=ev,({
void*_tmp774=({Cyc_Absyn_var_type(v);});_tmp2AE[1]=_tmp774;});Cyc_List_list(_tag_fat(_tmp2AE,sizeof(void*),2U));});_tmp2AC(_tmp2AD);});_tmp2AA(_tmp2AB);});
# 1084
*p=_tmp775;});
# 1086
return 1;}}else{_LL15: _LL16:
 return 0;}_LL12:;}}}else{goto _LL10;}}else{goto _LL10;}}else{_LL10: _tmp2A3=_tmp2A2;_LL11: {void*e2=_tmp2A3;
# 1089
return({Cyc_Tcutil_variable_in_effect(may_constrain_evars,v,e2);});}}_LLD:;}}else{goto _LLB;}case 10U: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp29A)->f2 != 0){_LL9: _tmp29E=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp29A)->f2)->hd;_LLA: {void*r2=_tmp29E;
# 1110
void*name=r2;
{void*_tmp2B6=name;void*_tmp2B7;if(((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp2B6)->tag == 1U){_LL18: _tmp2B7=(void*)((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp2B6)->f2;if(_tmp2B7 == 0){_LL19: {void*v1=_tmp2B7;
# 1114
return 0;}}else{goto _LL1A;}}else{_LL1A: _LL1B:
 goto _LL17;}_LL17:;}{
# 1117
struct Cyc_Absyn_Tvar*v1=({Cyc_Absyn_type2tvar(name);});
# 1121
return({Cyc_Absyn_tvar_cmp(v,v1);})== 0;}}}else{goto _LLB;}default: goto _LLB;}case 1U: _LL7: _tmp29B=((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp29A)->f1;_tmp29C=(void**)&((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp29A)->f2;_tmp29D=((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp29A)->f4;_LL8: {struct Cyc_Core_Opt*k=_tmp29B;void**p=_tmp29C;struct Cyc_Core_Opt*s=_tmp29D;
# 1092
if(k == 0 ||(int)((struct Cyc_Absyn_Kind*)k->v)->kind != (int)Cyc_Absyn_EffKind)
({void*_tmp2AF=0U;({int(*_tmp777)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp2B1)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Tcutil_impos;_tmp2B1;});struct _fat_ptr _tmp776=({const char*_tmp2B0="effect evar has wrong kind";_tag_fat(_tmp2B0,sizeof(char),27U);});_tmp777(_tmp776,_tag_fat(_tmp2AF,sizeof(void*),0U));});});{
# 1092
void*ev=({Cyc_Absyn_new_evar(& Cyc_Tcutil_eko,s);});
# 1098
if(!({({int(*_tmp779)(int(*compare)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_List_List*l,struct Cyc_Absyn_Tvar*x)=({int(*_tmp2B2)(int(*compare)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_List_List*l,struct Cyc_Absyn_Tvar*x)=(int(*)(int(*compare)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_List_List*l,struct Cyc_Absyn_Tvar*x))Cyc_List_mem;_tmp2B2;});struct Cyc_List_List*_tmp778=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(s))->v;_tmp779(Cyc_Tcutil_fast_tvar_cmp,_tmp778,v);});}))
return 0;{
# 1098
void*new_typ=({void*(*_tmp2B3)(struct Cyc_List_List*)=Cyc_Absyn_join_eff;struct Cyc_List_List*_tmp2B4=({void*_tmp2B5[2U];_tmp2B5[0]=ev,({
# 1100
void*_tmp77A=({Cyc_Absyn_var_type(v);});_tmp2B5[1]=_tmp77A;});Cyc_List_list(_tag_fat(_tmp2B5,sizeof(void*),2U));});_tmp2B3(_tmp2B4);});
*p=new_typ;
return 1;}}}default: _LLB: _LLC:
# 1124
 return 0;}_LL0:;}}
# 1129
static int Cyc_Tcutil_evar_in_effect(void*evar,void*e){
e=({Cyc_Tcutil_compress(e);});{
void*_tmp2B9=e;void*_tmp2BA;struct Cyc_List_List*_tmp2BB;switch(*((int*)_tmp2B9)){case 0U: switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2B9)->f1)){case 11U: _LL1: _tmp2BB=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2B9)->f2;_LL2: {struct Cyc_List_List*es=_tmp2BB;
# 1133
for(0;es != 0;es=es->tl){
if(({Cyc_Tcutil_evar_in_effect(evar,(void*)es->hd);}))
return 1;}
# 1133
return 0;}case 12U: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2B9)->f2 != 0){_LL3: _tmp2BA=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2B9)->f2)->hd;_LL4: {void*t=_tmp2BA;
# 1138
void*_stmttmp33=({Cyc_Tcutil_rgns_of(t);});void*_tmp2BC=_stmttmp33;void*_tmp2BD;void*_tmp2BE;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2BC)->tag == 0U){if(((struct Cyc_Absyn_RgnsCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2BC)->f1)->tag == 12U){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2BC)->f2 != 0){_LLA: _tmp2BE=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2BC)->f2)->hd;_LLB: {void*t2=_tmp2BE;
return 0;}}else{goto _LLC;}}else{goto _LLC;}}else{_LLC: _tmp2BD=_tmp2BC;_LLD: {void*e2=_tmp2BD;
return({Cyc_Tcutil_evar_in_effect(evar,e2);});}}_LL9:;}}else{goto _LL7;}default: goto _LL7;}case 1U: _LL5: _LL6:
# 1142
 return evar == e;default: _LL7: _LL8:
 return 0;}_LL0:;}}
# 1129
int Cyc_Tcutil_subset_effect(int may_constrain_evars,void*e1,void*e2){
# 1159 "tcutil.cyc"
void*comp=({Cyc_Tcutil_compress(e1);});
void*_tmp2C0=comp;struct Cyc_Core_Opt*_tmp2C2;void**_tmp2C1;struct Cyc_Absyn_Tvar*_tmp2C3;void*_tmp2C4;void*_tmp2C5;void*_tmp2C6;struct Cyc_List_List*_tmp2C7;switch(*((int*)_tmp2C0)){case 0U: switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2C0)->f1)){case 11U: _LL1: _tmp2C7=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2C0)->f2;_LL2: {struct Cyc_List_List*es=_tmp2C7;
# 1162
for(0;es != 0;es=es->tl){
if(!({Cyc_Tcutil_subset_effect(may_constrain_evars,(void*)es->hd,e2);}))return 0;}
# 1162
return 1;}case 9U: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2C0)->f2 != 0){_LL3: _tmp2C6=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2C0)->f2)->hd;_LL4: {void*r=_tmp2C6;
# 1172
int b=({Cyc_Tcutil_region_in_effect(may_constrain_evars,r,e2);})||
 may_constrain_evars &&({Cyc_Unify_unify(r,Cyc_Absyn_heap_rgn_type);});
return b;}}else{goto _LLD;}case 12U: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2C0)->f2 != 0){_LL7: _tmp2C5=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2C0)->f2)->hd;_LL8: {void*t=_tmp2C5;
# 1177
void*_stmttmp34=({Cyc_Tcutil_rgns_of(t);});void*_tmp2C8=_stmttmp34;void*_tmp2C9;void*_tmp2CA;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2C8)->tag == 0U){if(((struct Cyc_Absyn_RgnsCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2C8)->f1)->tag == 12U){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2C8)->f2 != 0){_LL10: _tmp2CA=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2C8)->f2)->hd;_LL11: {void*t2=_tmp2CA;
# 1182
return({Cyc_Tcutil_type_in_effect(may_constrain_evars,t2,e2);})||
 may_constrain_evars &&({Cyc_Unify_unify(t2,Cyc_Absyn_sint_type);});}}else{goto _LL12;}}else{goto _LL12;}}else{_LL12: _tmp2C9=_tmp2C8;_LL13: {void*e=_tmp2C9;
return({Cyc_Tcutil_subset_effect(may_constrain_evars,e,e2);});}}_LLF:;}}else{goto _LLD;}case 10U: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2C0)->f2 != 0){_LLB: _tmp2C4=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2C0)->f2)->hd;_LLC: {void*r2=_tmp2C4;
# 1205
void*name=r2;
{void*_tmp2CB=name;void*_tmp2CC;if(((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp2CB)->tag == 1U){_LL15: _tmp2CC=(void*)((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp2CB)->f2;_LL16: {void*v1=_tmp2CC;
# 1209
int b1=({Cyc_Tcutil_region_in_effect(1,name,e2);});
if(!b1)
# 1212
return 0;
# 1210
return 1;}}else{_LL17: _LL18:
# 1215
 goto _LL14;}_LL14:;}{
# 1217
struct Cyc_Absyn_Tvar*v=({Cyc_Absyn_type2tvar(name);});
int b=({Cyc_Tcutil_variable_in_effect(may_constrain_evars,v,e2);});
# 1222
return b;}}}else{goto _LLD;}default: goto _LLD;}case 2U: _LL5: _tmp2C3=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp2C0)->f1;_LL6: {struct Cyc_Absyn_Tvar*v=_tmp2C3;
# 1175
return({Cyc_Tcutil_variable_in_effect(may_constrain_evars,v,e2);});}case 1U: _LL9: _tmp2C1=(void**)&((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp2C0)->f2;_tmp2C2=((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp2C0)->f4;_LLA: {void**p=_tmp2C1;struct Cyc_Core_Opt*s=_tmp2C2;
# 1187
if(!({Cyc_Tcutil_evar_in_effect(e1,e2);})){
# 1192
*p=Cyc_Absyn_empty_effect;
# 1195
return 1;}else{
# 1197
return 0;}}default: _LLD: _LLE:
# 1223
({struct Cyc_String_pa_PrintArg_struct _tmp2D0=({struct Cyc_String_pa_PrintArg_struct _tmp6BA;_tmp6BA.tag=0U,({
struct _fat_ptr _tmp77B=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(e1);}));_tmp6BA.f1=_tmp77B;});_tmp6BA;});struct Cyc_String_pa_PrintArg_struct _tmp2D1=({struct Cyc_String_pa_PrintArg_struct _tmp6B9;_tmp6B9.tag=0U,({struct _fat_ptr _tmp77C=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(e2);}));_tmp6B9.f1=_tmp77C;});_tmp6B9;});void*_tmp2CD[2U];_tmp2CD[0]=& _tmp2D0,_tmp2CD[1]=& _tmp2D1;({int(*_tmp77E)(struct _fat_ptr fmt,struct _fat_ptr ap)=({
# 1223
int(*_tmp2CF)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Tcutil_impos;_tmp2CF;});struct _fat_ptr _tmp77D=({const char*_tmp2CE="subset_effect: bad effect: e1=%s  e2=%s";_tag_fat(_tmp2CE,sizeof(char),40U);});_tmp77E(_tmp77D,_tag_fat(_tmp2CD,sizeof(void*),2U));});});}_LL0:;}
# 1234
static int Cyc_Tcutil_sub_rgnpo(struct Cyc_List_List*rpo1,struct Cyc_List_List*rpo2){
# 1236
{struct Cyc_List_List*r1=rpo1;for(0;r1 != 0;r1=r1->tl){
struct _tuple15*_stmttmp35=(struct _tuple15*)r1->hd;void*_tmp2D4;void*_tmp2D3;_LL1: _tmp2D3=_stmttmp35->f1;_tmp2D4=_stmttmp35->f2;_LL2: {void*t1a=_tmp2D3;void*t1b=_tmp2D4;
int found=t1a == Cyc_Absyn_heap_rgn_type;
{struct Cyc_List_List*r2=rpo2;for(0;r2 != 0 && !found;r2=r2->tl){
struct _tuple15*_stmttmp36=(struct _tuple15*)r2->hd;void*_tmp2D6;void*_tmp2D5;_LL4: _tmp2D5=_stmttmp36->f1;_tmp2D6=_stmttmp36->f2;_LL5: {void*t2a=_tmp2D5;void*t2b=_tmp2D6;
if(({Cyc_Unify_unify(t1a,t2a);})&&({Cyc_Unify_unify(t1b,t2b);})){
found=1;
break;}}}}
# 1246
if(!found)return 0;}}}
# 1248
return 1;}
# 1254
int Cyc_Tcutil_same_rgn_po(struct Cyc_List_List*rpo1,struct Cyc_List_List*rpo2){
# 1256
return({Cyc_Tcutil_sub_rgnpo(rpo1,rpo2);})&&({Cyc_Tcutil_sub_rgnpo(rpo2,rpo1);});}
# 1259
static int Cyc_Tcutil_tycon2int(void*t){
void*_tmp2D9=t;switch(*((int*)_tmp2D9)){case 0U: _LL1: _LL2:
 return 0;case 1U: switch(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp2D9)->f1){case Cyc_Absyn_Unsigned: switch(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp2D9)->f2){case Cyc_Absyn_Char_sz: _LL3: _LL4:
 return 1;case Cyc_Absyn_Short_sz: _LL9: _LLA:
# 1265
 return 4;case Cyc_Absyn_Int_sz: _LLF: _LL10:
# 1268
 return 7;case Cyc_Absyn_Long_sz: _LL15: _LL16:
# 1271
 return 7;case Cyc_Absyn_LongLong_sz: _LL1B: _LL1C:
# 1274
 return 13;default: goto _LL4D;}case Cyc_Absyn_Signed: switch(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp2D9)->f2){case Cyc_Absyn_Char_sz: _LL5: _LL6:
# 1263
 return 2;case Cyc_Absyn_Short_sz: _LLB: _LLC:
# 1266
 return 5;case Cyc_Absyn_Int_sz: _LL11: _LL12:
# 1269
 return 8;case Cyc_Absyn_Long_sz: _LL17: _LL18:
# 1272
 return 8;case Cyc_Absyn_LongLong_sz: _LL1D: _LL1E:
# 1275
 return 14;default: goto _LL4D;}case Cyc_Absyn_None: switch(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp2D9)->f2){case Cyc_Absyn_Char_sz: _LL7: _LL8:
# 1264
 return 3;case Cyc_Absyn_Short_sz: _LLD: _LLE:
# 1267
 return 6;case Cyc_Absyn_Int_sz: _LL13: _LL14:
# 1270
 return 9;case Cyc_Absyn_Long_sz: _LL19: _LL1A:
# 1273
 return 9;case Cyc_Absyn_LongLong_sz: _LL1F: _LL20:
# 1276
 return 15;default: goto _LL4D;}default: goto _LL4D;}case 2U: switch(((struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct*)_tmp2D9)->f1){case 0U: _LL21: _LL22:
 return 16;case 1U: _LL23: _LL24:
 return 17;default: _LL25: _LL26:
 return 18;}case 3U: _LL27: _LL28:
 return 19;case 5U: _LL29: _LL2A:
 return 20;case 6U: _LL2B: _LL2C:
 return 21;case 7U: _LL2D: _LL2E:
 return 22;case 8U: _LL2F: _LL30:
 return 23;case 9U: _LL31: _LL32:
 return 24;case 11U: _LL33: _LL34:
 return 25;case 12U: _LL35: _LL36:
 return 26;case 13U: _LL37: _LL38:
 return 27;case 14U: _LL39: _LL3A:
 return 28;case 16U: _LL3B: _LL3C:
 return 29;case 15U: _LL3D: _LL3E:
 return 30;case 17U: _LL3F: _LL40:
 return 31;case 18U: _LL41: _LL42:
 return 32;case 19U: _LL43: _LL44:
 return 33;case 20U: _LL45: _LL46:
 return 34;case 21U: _LL47: _LL48:
 return 35;case 22U: _LL49: _LL4A:
 return 36;case 4U: _LL4B: _LL4C:
 return 37;default: _LL4D: _LL4E:
({void*_tmp2DA=0U;({int(*_tmp780)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp2DC)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Tcutil_impos;_tmp2DC;});struct _fat_ptr _tmp77F=({const char*_tmp2DB="bad con";_tag_fat(_tmp2DB,sizeof(char),8U);});_tmp780(_tmp77F,_tag_fat(_tmp2DA,sizeof(void*),0U));});});}_LL0:;}
# 1302
static int Cyc_Tcutil_type_case_number(void*t){
void*_tmp2DE=t;void*_tmp2DF;switch(*((int*)_tmp2DE)){case 1U: _LL1: _LL2:
 return 1;case 2U: _LL3: _LL4:
 return 2;case 3U: _LL5: _LL6:
 return 3;case 4U: _LL7: _LL8:
 return 4;case 5U: _LL9: _LLA:
 return 5;case 6U: _LLB: _LLC:
 return 6;case 7U: _LLD: _LLE:
 return 7;case 8U: _LLF: _LL10:
 return 8;case 9U: _LL11: _LL12:
 return 9;case 10U: _LL13: _LL14:
 return 10;case 11U: _LL15: _LL16:
 return 11;default: _LL17: _tmp2DF=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2DE)->f1;_LL18: {void*c=_tmp2DF;
return 12 + ({Cyc_Tcutil_tycon2int(c);});}}_LL0:;}
# 1318
static int Cyc_Tcutil_enumfield_cmp(struct Cyc_Absyn_Enumfield*e1,struct Cyc_Absyn_Enumfield*e2){
int qc=({Cyc_Absyn_qvar_cmp(e1->name,e2->name);});
if(qc != 0)return qc;return({({int(*_tmp782)(int(*cmp)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*),struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*)=({
int(*_tmp2E1)(int(*cmp)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*),struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*)=(int(*)(int(*cmp)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*),struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*))Cyc_Tcutil_star_cmp;_tmp2E1;});struct Cyc_Absyn_Exp*_tmp781=e1->tag;_tmp782(Cyc_Evexp_const_exp_cmp,_tmp781,e2->tag);});});}
# 1323
static struct _tuple1*Cyc_Tcutil_get_datatype_qvar(union Cyc_Absyn_DatatypeInfo i){
union Cyc_Absyn_DatatypeInfo _tmp2E3=i;struct _tuple1*_tmp2E4;struct Cyc_Absyn_Datatypedecl*_tmp2E5;if((_tmp2E3.KnownDatatype).tag == 2){_LL1: _tmp2E5=*(_tmp2E3.KnownDatatype).val;_LL2: {struct Cyc_Absyn_Datatypedecl*dd=_tmp2E5;
return dd->name;}}else{_LL3: _tmp2E4=((_tmp2E3.UnknownDatatype).val).name;_LL4: {struct _tuple1*n=_tmp2E4;
return n;}}_LL0:;}struct _tuple20{struct _tuple1*f1;struct _tuple1*f2;};
# 1329
static struct _tuple20 Cyc_Tcutil_get_datatype_field_qvars(union Cyc_Absyn_DatatypeFieldInfo i){
union Cyc_Absyn_DatatypeFieldInfo _tmp2E7=i;struct _tuple1*_tmp2E9;struct _tuple1*_tmp2E8;struct Cyc_Absyn_Datatypefield*_tmp2EB;struct Cyc_Absyn_Datatypedecl*_tmp2EA;if((_tmp2E7.KnownDatatypefield).tag == 2){_LL1: _tmp2EA=((_tmp2E7.KnownDatatypefield).val).f1;_tmp2EB=((_tmp2E7.KnownDatatypefield).val).f2;_LL2: {struct Cyc_Absyn_Datatypedecl*dd=_tmp2EA;struct Cyc_Absyn_Datatypefield*df=_tmp2EB;
# 1332
return({struct _tuple20 _tmp6BB;_tmp6BB.f1=dd->name,_tmp6BB.f2=df->name;_tmp6BB;});}}else{_LL3: _tmp2E8=((_tmp2E7.UnknownDatatypefield).val).datatype_name;_tmp2E9=((_tmp2E7.UnknownDatatypefield).val).field_name;_LL4: {struct _tuple1*d=_tmp2E8;struct _tuple1*f=_tmp2E9;
# 1334
return({struct _tuple20 _tmp6BC;_tmp6BC.f1=d,_tmp6BC.f2=f;_tmp6BC;});}}_LL0:;}struct _tuple21{enum Cyc_Absyn_AggrKind f1;struct _tuple1*f2;};
# 1337
static struct _tuple21 Cyc_Tcutil_get_aggr_kind_and_qvar(union Cyc_Absyn_AggrInfo i){
union Cyc_Absyn_AggrInfo _tmp2ED=i;struct Cyc_Absyn_Aggrdecl*_tmp2EE;struct _tuple1*_tmp2F0;enum Cyc_Absyn_AggrKind _tmp2EF;if((_tmp2ED.UnknownAggr).tag == 1){_LL1: _tmp2EF=((_tmp2ED.UnknownAggr).val).f1;_tmp2F0=((_tmp2ED.UnknownAggr).val).f2;_LL2: {enum Cyc_Absyn_AggrKind k=_tmp2EF;struct _tuple1*n=_tmp2F0;
return({struct _tuple21 _tmp6BD;_tmp6BD.f1=k,_tmp6BD.f2=n;_tmp6BD;});}}else{_LL3: _tmp2EE=*(_tmp2ED.KnownAggr).val;_LL4: {struct Cyc_Absyn_Aggrdecl*ad=_tmp2EE;
return({struct _tuple21 _tmp6BE;_tmp6BE.f1=ad->kind,_tmp6BE.f2=ad->name;_tmp6BE;});}}_LL0:;}
# 1343
int Cyc_Tcutil_tycon_cmp(void*t1,void*t2){
if(t1 == t2)return 0;{int i1=({Cyc_Tcutil_tycon2int(t1);});
# 1346
int i2=({Cyc_Tcutil_tycon2int(t2);});
if(i1 != i2)return i1 - i2;{struct _tuple15 _stmttmp37=({struct _tuple15 _tmp6BF;_tmp6BF.f1=t1,_tmp6BF.f2=t2;_tmp6BF;});struct _tuple15 _tmp2F2=_stmttmp37;union Cyc_Absyn_AggrInfo _tmp2F4;union Cyc_Absyn_AggrInfo _tmp2F3;union Cyc_Absyn_DatatypeFieldInfo _tmp2F6;union Cyc_Absyn_DatatypeFieldInfo _tmp2F5;union Cyc_Absyn_DatatypeInfo _tmp2F8;union Cyc_Absyn_DatatypeInfo _tmp2F7;struct Cyc_List_List*_tmp2FA;struct Cyc_List_List*_tmp2F9;struct _fat_ptr _tmp2FC;struct _fat_ptr _tmp2FB;struct _tuple1*_tmp2FE;struct _tuple1*_tmp2FD;switch(*((int*)_tmp2F2.f1)){case 17U: if(((struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct*)_tmp2F2.f2)->tag == 17U){_LL1: _tmp2FD=((struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct*)_tmp2F2.f1)->f1;_tmp2FE=((struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct*)_tmp2F2.f2)->f1;_LL2: {struct _tuple1*n1=_tmp2FD;struct _tuple1*n2=_tmp2FE;
# 1350
return({Cyc_Absyn_qvar_cmp(n1,n2);});}}else{goto _LLD;}case 19U: if(((struct Cyc_Absyn_BuiltinCon_Absyn_TyCon_struct*)_tmp2F2.f2)->tag == 19U){_LL3: _tmp2FB=((struct Cyc_Absyn_BuiltinCon_Absyn_TyCon_struct*)_tmp2F2.f1)->f1;_tmp2FC=((struct Cyc_Absyn_BuiltinCon_Absyn_TyCon_struct*)_tmp2F2.f2)->f1;_LL4: {struct _fat_ptr s1=_tmp2FB;struct _fat_ptr s2=_tmp2FC;
return({Cyc_strcmp((struct _fat_ptr)s1,(struct _fat_ptr)s2);});}}else{goto _LLD;}case 18U: if(((struct Cyc_Absyn_AnonEnumCon_Absyn_TyCon_struct*)_tmp2F2.f2)->tag == 18U){_LL5: _tmp2F9=((struct Cyc_Absyn_AnonEnumCon_Absyn_TyCon_struct*)_tmp2F2.f1)->f1;_tmp2FA=((struct Cyc_Absyn_AnonEnumCon_Absyn_TyCon_struct*)_tmp2F2.f2)->f1;_LL6: {struct Cyc_List_List*fs1=_tmp2F9;struct Cyc_List_List*fs2=_tmp2FA;
# 1353
return({({int(*_tmp784)(int(*cmp)(struct Cyc_Absyn_Enumfield*,struct Cyc_Absyn_Enumfield*),struct Cyc_List_List*l1,struct Cyc_List_List*l2)=({int(*_tmp2FF)(int(*cmp)(struct Cyc_Absyn_Enumfield*,struct Cyc_Absyn_Enumfield*),struct Cyc_List_List*l1,struct Cyc_List_List*l2)=(int(*)(int(*cmp)(struct Cyc_Absyn_Enumfield*,struct Cyc_Absyn_Enumfield*),struct Cyc_List_List*l1,struct Cyc_List_List*l2))Cyc_List_list_cmp;_tmp2FF;});struct Cyc_List_List*_tmp783=fs1;_tmp784(Cyc_Tcutil_enumfield_cmp,_tmp783,fs2);});});}}else{goto _LLD;}case 20U: if(((struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*)_tmp2F2.f2)->tag == 20U){_LL7: _tmp2F7=((struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*)_tmp2F2.f1)->f1;_tmp2F8=((struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*)_tmp2F2.f2)->f1;_LL8: {union Cyc_Absyn_DatatypeInfo info1=_tmp2F7;union Cyc_Absyn_DatatypeInfo info2=_tmp2F8;
# 1355
struct _tuple1*q1=({Cyc_Tcutil_get_datatype_qvar(info1);});
struct _tuple1*q2=({Cyc_Tcutil_get_datatype_qvar(info2);});
return({Cyc_Absyn_qvar_cmp(q1,q2);});}}else{goto _LLD;}case 21U: if(((struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)_tmp2F2.f2)->tag == 21U){_LL9: _tmp2F5=((struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)_tmp2F2.f1)->f1;_tmp2F6=((struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)_tmp2F2.f2)->f1;_LLA: {union Cyc_Absyn_DatatypeFieldInfo info1=_tmp2F5;union Cyc_Absyn_DatatypeFieldInfo info2=_tmp2F6;
# 1359
struct _tuple20 _stmttmp38=({Cyc_Tcutil_get_datatype_field_qvars(info1);});struct _tuple1*_tmp301;struct _tuple1*_tmp300;_LL10: _tmp300=_stmttmp38.f1;_tmp301=_stmttmp38.f2;_LL11: {struct _tuple1*d1=_tmp300;struct _tuple1*f1=_tmp301;
struct _tuple20 _stmttmp39=({Cyc_Tcutil_get_datatype_field_qvars(info2);});struct _tuple1*_tmp303;struct _tuple1*_tmp302;_LL13: _tmp302=_stmttmp39.f1;_tmp303=_stmttmp39.f2;_LL14: {struct _tuple1*d2=_tmp302;struct _tuple1*f2=_tmp303;
int c=({Cyc_Absyn_qvar_cmp(d1,d2);});
if(c != 0)return c;return({Cyc_Absyn_qvar_cmp(f1,f2);});}}}}else{goto _LLD;}case 22U: if(((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)_tmp2F2.f2)->tag == 22U){_LLB: _tmp2F3=((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)_tmp2F2.f1)->f1;_tmp2F4=((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)_tmp2F2.f2)->f1;_LLC: {union Cyc_Absyn_AggrInfo info1=_tmp2F3;union Cyc_Absyn_AggrInfo info2=_tmp2F4;
# 1365
struct _tuple21 _stmttmp3A=({Cyc_Tcutil_get_aggr_kind_and_qvar(info1);});struct _tuple1*_tmp305;enum Cyc_Absyn_AggrKind _tmp304;_LL16: _tmp304=_stmttmp3A.f1;_tmp305=_stmttmp3A.f2;_LL17: {enum Cyc_Absyn_AggrKind k1=_tmp304;struct _tuple1*q1=_tmp305;
struct _tuple21 _stmttmp3B=({Cyc_Tcutil_get_aggr_kind_and_qvar(info2);});struct _tuple1*_tmp307;enum Cyc_Absyn_AggrKind _tmp306;_LL19: _tmp306=_stmttmp3B.f1;_tmp307=_stmttmp3B.f2;_LL1A: {enum Cyc_Absyn_AggrKind k2=_tmp306;struct _tuple1*q2=_tmp307;
int c=({Cyc_Absyn_qvar_cmp(q1,q2);});
if(c != 0)return c;return(int)k1 - (int)k2;}}}}else{goto _LLD;}default: _LLD: _LLE:
# 1370
 return 0;}_LL0:;}}}
# 1374
int Cyc_Tcutil_star_cmp(int(*cmp)(void*,void*),void*a1,void*a2){
if(a1 == a2)return 0;if(
a1 == 0 && a2 != 0)return - 1;
# 1375
if(
# 1377
a1 != 0 && a2 == 0)return 1;
# 1375
return({({int(*_tmp786)(void*,void*)=cmp;void*_tmp785=(void*)_check_null(a1);_tmp786(_tmp785,(void*)_check_null(a2));});});}
# 1380
static int Cyc_Tcutil_tqual_cmp(struct Cyc_Absyn_Tqual tq1,struct Cyc_Absyn_Tqual tq2){
int i1=(tq1.real_const + (tq1.q_volatile << 1))+ (tq1.q_restrict << 2);
int i2=(tq2.real_const + (tq2.q_volatile << 1))+ (tq2.q_restrict << 2);
return({Cyc_Core_intcmp(i1,i2);});}
# 1385
static int Cyc_Tcutil_tqual_type_cmp(struct _tuple12*tqt1,struct _tuple12*tqt2){
void*_tmp30C;struct Cyc_Absyn_Tqual _tmp30B;_LL1: _tmp30B=tqt1->f1;_tmp30C=tqt1->f2;_LL2: {struct Cyc_Absyn_Tqual tq1=_tmp30B;void*t1=_tmp30C;
void*_tmp30E;struct Cyc_Absyn_Tqual _tmp30D;_LL4: _tmp30D=tqt2->f1;_tmp30E=tqt2->f2;_LL5: {struct Cyc_Absyn_Tqual tq2=_tmp30D;void*t2=_tmp30E;
int tqc=({Cyc_Tcutil_tqual_cmp(tq1,tq2);});
if(tqc != 0)return tqc;return({Cyc_Tcutil_typecmp(t1,t2);});}}}
# 1393
int Cyc_Tcutil_aggrfield_cmp(struct Cyc_Absyn_Aggrfield*f1,struct Cyc_Absyn_Aggrfield*f2){
int zsc=({Cyc_strptrcmp(f1->name,f2->name);});
if(zsc != 0)return zsc;{int tqc=({Cyc_Tcutil_tqual_cmp(f1->tq,f2->tq);});
# 1397
if(tqc != 0)return tqc;{int tc=({Cyc_Tcutil_typecmp(f1->type,f2->type);});
# 1399
if(tc != 0)return tc;{int ac=({Cyc_List_list_cmp(Cyc_Absyn_attribute_cmp,f1->attributes,f2->attributes);});
# 1401
if(ac != 0)return ac;ac=({({int(*_tmp788)(int(*cmp)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*),struct Cyc_Absyn_Exp*a1,struct Cyc_Absyn_Exp*a2)=({
int(*_tmp310)(int(*cmp)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*),struct Cyc_Absyn_Exp*a1,struct Cyc_Absyn_Exp*a2)=(int(*)(int(*cmp)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*),struct Cyc_Absyn_Exp*a1,struct Cyc_Absyn_Exp*a2))Cyc_Tcutil_star_cmp;_tmp310;});struct Cyc_Absyn_Exp*_tmp787=f1->width;_tmp788(Cyc_Evexp_const_exp_cmp,_tmp787,f2->width);});});
if(ac != 0)return ac;return({({int(*_tmp78A)(int(*cmp)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*),struct Cyc_Absyn_Exp*a1,struct Cyc_Absyn_Exp*a2)=({
int(*_tmp311)(int(*cmp)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*),struct Cyc_Absyn_Exp*a1,struct Cyc_Absyn_Exp*a2)=(int(*)(int(*cmp)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*),struct Cyc_Absyn_Exp*a1,struct Cyc_Absyn_Exp*a2))Cyc_Tcutil_star_cmp;_tmp311;});struct Cyc_Absyn_Exp*_tmp789=f1->requires_clause;_tmp78A(Cyc_Evexp_const_exp_cmp,_tmp789,f2->requires_clause);});});}}}}
# 1410
int Cyc_Tcutil_typecmp(void*t1,void*t2){
t1=({Cyc_Tcutil_compress(t1);});
t2=({Cyc_Tcutil_compress(t2);});
if(t1 == t2)return 0;{int shallowcmp=({int(*_tmp356)(int,int)=Cyc_Core_intcmp;int _tmp357=({Cyc_Tcutil_type_case_number(t1);});int _tmp358=({Cyc_Tcutil_type_case_number(t2);});_tmp356(_tmp357,_tmp358);});
# 1415
if(shallowcmp != 0)
return shallowcmp;{
# 1415
struct _tuple15 _stmttmp3C=({struct _tuple15 _tmp6C2;_tmp6C2.f1=t2,_tmp6C2.f2=t1;_tmp6C2;});struct _tuple15 _tmp313=_stmttmp3C;struct Cyc_Absyn_Exp*_tmp315;struct Cyc_Absyn_Exp*_tmp314;struct Cyc_Absyn_Exp*_tmp317;struct Cyc_Absyn_Exp*_tmp316;struct Cyc_List_List*_tmp31B;enum Cyc_Absyn_AggrKind _tmp31A;struct Cyc_List_List*_tmp319;enum Cyc_Absyn_AggrKind _tmp318;struct Cyc_List_List*_tmp31D;struct Cyc_List_List*_tmp31C;struct Cyc_Absyn_FnInfo _tmp31F;struct Cyc_Absyn_FnInfo _tmp31E;void*_tmp327;struct Cyc_Absyn_Exp*_tmp326;struct Cyc_Absyn_Tqual _tmp325;void*_tmp324;void*_tmp323;struct Cyc_Absyn_Exp*_tmp322;struct Cyc_Absyn_Tqual _tmp321;void*_tmp320;void*_tmp333;void*_tmp332;void*_tmp331;void*_tmp330;struct Cyc_Absyn_Tqual _tmp32F;void*_tmp32E;void*_tmp32D;void*_tmp32C;void*_tmp32B;void*_tmp32A;struct Cyc_Absyn_Tqual _tmp329;void*_tmp328;struct Cyc_Absyn_Tvar*_tmp335;struct Cyc_Absyn_Tvar*_tmp334;struct Cyc_List_List*_tmp339;void*_tmp338;struct Cyc_List_List*_tmp337;void*_tmp336;switch(*((int*)_tmp313.f1)){case 0U: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp313.f2)->tag == 0U){_LL1: _tmp336=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp313.f1)->f1;_tmp337=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp313.f1)->f2;_tmp338=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp313.f2)->f1;_tmp339=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp313.f2)->f2;_LL2: {void*c1=_tmp336;struct Cyc_List_List*ts1=_tmp337;void*c2=_tmp338;struct Cyc_List_List*ts2=_tmp339;
# 1421
int c=({Cyc_Tcutil_tycon_cmp(c1,c2);});
if(c != 0)return c;return({Cyc_List_list_cmp(Cyc_Tcutil_typecmp,ts1,ts2);});}}else{goto _LL15;}case 1U: if(((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp313.f2)->tag == 1U){_LL3: _LL4:
# 1425
({void*_tmp33A=0U;({int(*_tmp78C)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp33C)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Tcutil_impos;_tmp33C;});struct _fat_ptr _tmp78B=({const char*_tmp33B="typecmp: can only compare closed types";_tag_fat(_tmp33B,sizeof(char),39U);});_tmp78C(_tmp78B,_tag_fat(_tmp33A,sizeof(void*),0U));});});}else{goto _LL15;}case 2U: if(((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp313.f2)->tag == 2U){_LL5: _tmp334=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp313.f1)->f1;_tmp335=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp313.f2)->f1;_LL6: {struct Cyc_Absyn_Tvar*tv2=_tmp334;struct Cyc_Absyn_Tvar*tv1=_tmp335;
# 1429
return({Cyc_Core_intcmp(tv1->identity,tv2->identity);});}}else{goto _LL15;}case 3U: if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp313.f2)->tag == 3U){_LL7: _tmp328=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp313.f1)->f1).elt_type;_tmp329=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp313.f1)->f1).elt_tq;_tmp32A=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp313.f1)->f1).ptr_atts).rgn;_tmp32B=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp313.f1)->f1).ptr_atts).nullable;_tmp32C=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp313.f1)->f1).ptr_atts).bounds;_tmp32D=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp313.f1)->f1).ptr_atts).zero_term;_tmp32E=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp313.f2)->f1).elt_type;_tmp32F=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp313.f2)->f1).elt_tq;_tmp330=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp313.f2)->f1).ptr_atts).rgn;_tmp331=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp313.f2)->f1).ptr_atts).nullable;_tmp332=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp313.f2)->f1).ptr_atts).bounds;_tmp333=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp313.f2)->f1).ptr_atts).zero_term;_LL8: {void*t2a=_tmp328;struct Cyc_Absyn_Tqual tqual2a=_tmp329;void*rgn2=_tmp32A;void*null2a=_tmp32B;void*b2=_tmp32C;void*zt2=_tmp32D;void*t1a=_tmp32E;struct Cyc_Absyn_Tqual tqual1a=_tmp32F;void*rgn1=_tmp330;void*null1a=_tmp331;void*b1=_tmp332;void*zt1=_tmp333;
# 1433
int etc=({Cyc_Tcutil_typecmp(t1a,t2a);});
if(etc != 0)return etc;{int rc=({Cyc_Tcutil_typecmp(rgn1,rgn2);});
# 1436
if(rc != 0)return rc;{int tqc=({Cyc_Tcutil_tqual_cmp(tqual1a,tqual2a);});
# 1438
if(tqc != 0)return tqc;{int cc=({Cyc_Tcutil_typecmp(b1,b2);});
# 1440
if(cc != 0)return cc;{int zc=({Cyc_Tcutil_typecmp(zt1,zt2);});
# 1442
if(zc != 0)return zc;{int bc=({Cyc_Tcutil_typecmp(b1,b2);});
# 1444
if(bc != 0)return bc;return({Cyc_Tcutil_typecmp(null1a,null2a);});}}}}}}}else{goto _LL15;}case 4U: if(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp313.f2)->tag == 4U){_LL9: _tmp320=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp313.f1)->f1).elt_type;_tmp321=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp313.f1)->f1).tq;_tmp322=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp313.f1)->f1).num_elts;_tmp323=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp313.f1)->f1).zero_term;_tmp324=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp313.f2)->f1).elt_type;_tmp325=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp313.f2)->f1).tq;_tmp326=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp313.f2)->f1).num_elts;_tmp327=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp313.f2)->f1).zero_term;_LLA: {void*t2a=_tmp320;struct Cyc_Absyn_Tqual tq2a=_tmp321;struct Cyc_Absyn_Exp*e1=_tmp322;void*zt1=_tmp323;void*t1a=_tmp324;struct Cyc_Absyn_Tqual tq1a=_tmp325;struct Cyc_Absyn_Exp*e2=_tmp326;void*zt2=_tmp327;
# 1449
int tqc=({Cyc_Tcutil_tqual_cmp(tq1a,tq2a);});
if(tqc != 0)return tqc;{int tc=({Cyc_Tcutil_typecmp(t1a,t2a);});
# 1452
if(tc != 0)return tc;{int ztc=({Cyc_Tcutil_typecmp(zt1,zt2);});
# 1454
if(ztc != 0)return ztc;if(e1 == e2)
return 0;
# 1454
if(
# 1456
e1 == 0 || e2 == 0)
({void*_tmp33D=0U;({int(*_tmp78E)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp33F)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Tcutil_impos;_tmp33F;});struct _fat_ptr _tmp78D=({const char*_tmp33E="missing expression in array index";_tag_fat(_tmp33E,sizeof(char),34U);});_tmp78E(_tmp78D,_tag_fat(_tmp33D,sizeof(void*),0U));});});
# 1454
return({({int(*_tmp790)(int(*cmp)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*),struct Cyc_Absyn_Exp*a1,struct Cyc_Absyn_Exp*a2)=({
# 1459
int(*_tmp340)(int(*cmp)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*),struct Cyc_Absyn_Exp*a1,struct Cyc_Absyn_Exp*a2)=(int(*)(int(*cmp)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*),struct Cyc_Absyn_Exp*a1,struct Cyc_Absyn_Exp*a2))Cyc_Tcutil_star_cmp;_tmp340;});struct Cyc_Absyn_Exp*_tmp78F=e1;_tmp790(Cyc_Evexp_const_exp_cmp,_tmp78F,e2);});});}}}}else{goto _LL15;}case 5U: if(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp313.f2)->tag == 5U){_LLB: _tmp31E=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp313.f1)->f1;_tmp31F=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp313.f2)->f1;_LLC: {struct Cyc_Absyn_FnInfo f1=_tmp31E;struct Cyc_Absyn_FnInfo f2=_tmp31F;
# 1462
if(({Cyc_Unify_unify(t1,t2);}))return 0;{int r=({Cyc_Tcutil_typecmp(f1.ret_type,f2.ret_type);});
# 1464
if(r != 0)return r;r=({Cyc_Tcutil_tqual_cmp(f1.ret_tqual,f2.ret_tqual);});
# 1466
if(r != 0)return r;{struct Cyc_List_List*args1=f1.args;
# 1468
struct Cyc_List_List*args2=f2.args;
for(0;args1 != 0 && args2 != 0;(args1=args1->tl,args2=args2->tl)){
struct _tuple9 _stmttmp3D=*((struct _tuple9*)args1->hd);void*_tmp342;struct Cyc_Absyn_Tqual _tmp341;_LL18: _tmp341=_stmttmp3D.f2;_tmp342=_stmttmp3D.f3;_LL19: {struct Cyc_Absyn_Tqual tq1=_tmp341;void*t1=_tmp342;
struct _tuple9 _stmttmp3E=*((struct _tuple9*)args2->hd);void*_tmp344;struct Cyc_Absyn_Tqual _tmp343;_LL1B: _tmp343=_stmttmp3E.f2;_tmp344=_stmttmp3E.f3;_LL1C: {struct Cyc_Absyn_Tqual tq2=_tmp343;void*t2=_tmp344;
r=({Cyc_Tcutil_tqual_cmp(tq1,tq2);});
if(r != 0)return r;r=({Cyc_Tcutil_typecmp(t1,t2);});
# 1475
if(r != 0)return r;}}}
# 1477
if(args1 != 0)return 1;if(args2 != 0)
return - 1;
# 1477
if(
# 1479
f1.c_varargs && !f2.c_varargs)return 1;
# 1477
if(
# 1480
!f1.c_varargs && f2.c_varargs)return - 1;
# 1477
if(f1.cyc_varargs != 0 & f2.cyc_varargs == 0)
# 1481
return 1;
# 1477
if(f1.cyc_varargs == 0 & f2.cyc_varargs != 0)
# 1482
return - 1;
# 1477
if(f1.cyc_varargs != 0 & f2.cyc_varargs != 0){
# 1484
r=({({struct Cyc_Absyn_Tqual _tmp791=((struct Cyc_Absyn_VarargInfo*)_check_null(f1.cyc_varargs))->tq;Cyc_Tcutil_tqual_cmp(_tmp791,((struct Cyc_Absyn_VarargInfo*)_check_null(f2.cyc_varargs))->tq);});});
if(r != 0)return r;r=({Cyc_Tcutil_typecmp((f1.cyc_varargs)->type,(f2.cyc_varargs)->type);});
# 1487
if(r != 0)return r;if(
(f1.cyc_varargs)->inject && !(f2.cyc_varargs)->inject)return 1;
# 1487
if(
# 1489
!(f1.cyc_varargs)->inject &&(f2.cyc_varargs)->inject)return - 1;}
# 1477
r=({Cyc_Tcutil_star_cmp(Cyc_Tcutil_typecmp,f1.effect,f2.effect);});
# 1492
if(r != 0)return r;{struct Cyc_List_List*rpo1=f1.rgn_po;
# 1494
struct Cyc_List_List*rpo2=f2.rgn_po;
for(0;rpo1 != 0 && rpo2 != 0;(rpo1=rpo1->tl,rpo2=rpo2->tl)){
struct _tuple15 _stmttmp3F=*((struct _tuple15*)rpo1->hd);void*_tmp346;void*_tmp345;_LL1E: _tmp345=_stmttmp3F.f1;_tmp346=_stmttmp3F.f2;_LL1F: {void*t1a=_tmp345;void*t1b=_tmp346;
struct _tuple15 _stmttmp40=*((struct _tuple15*)rpo2->hd);void*_tmp348;void*_tmp347;_LL21: _tmp347=_stmttmp40.f1;_tmp348=_stmttmp40.f2;_LL22: {void*t2a=_tmp347;void*t2b=_tmp348;
r=({Cyc_Tcutil_typecmp(t1a,t2a);});if(r != 0)return r;r=({Cyc_Tcutil_typecmp(t1b,t2b);});
if(r != 0)return r;}}}
# 1501
if(rpo1 != 0)return 1;if(rpo2 != 0)
return - 1;
# 1501
r=({({int(*_tmp793)(int(*cmp)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*),struct Cyc_Absyn_Exp*a1,struct Cyc_Absyn_Exp*a2)=({
# 1503
int(*_tmp349)(int(*cmp)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*),struct Cyc_Absyn_Exp*a1,struct Cyc_Absyn_Exp*a2)=(int(*)(int(*cmp)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*),struct Cyc_Absyn_Exp*a1,struct Cyc_Absyn_Exp*a2))Cyc_Tcutil_star_cmp;_tmp349;});struct Cyc_Absyn_Exp*_tmp792=f1.requires_clause;_tmp793(Cyc_Evexp_const_exp_cmp,_tmp792,f2.requires_clause);});});
if(r != 0)return r;r=({({int(*_tmp795)(int(*cmp)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*),struct Cyc_Absyn_Exp*a1,struct Cyc_Absyn_Exp*a2)=({
int(*_tmp34A)(int(*cmp)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*),struct Cyc_Absyn_Exp*a1,struct Cyc_Absyn_Exp*a2)=(int(*)(int(*cmp)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*),struct Cyc_Absyn_Exp*a1,struct Cyc_Absyn_Exp*a2))Cyc_Tcutil_star_cmp;_tmp34A;});struct Cyc_Absyn_Exp*_tmp794=f1.ensures_clause;_tmp795(Cyc_Evexp_const_exp_cmp,_tmp794,f2.ensures_clause);});});
if(r != 0)return r;({void*_tmp34B=0U;({int(*_tmp79A)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp350)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Tcutil_impos;_tmp350;});struct _fat_ptr _tmp799=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp34E=({struct Cyc_String_pa_PrintArg_struct _tmp6C1;_tmp6C1.tag=0U,({
# 1509
struct _fat_ptr _tmp796=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t1);}));_tmp6C1.f1=_tmp796;});_tmp6C1;});struct Cyc_String_pa_PrintArg_struct _tmp34F=({struct Cyc_String_pa_PrintArg_struct _tmp6C0;_tmp6C0.tag=0U,({struct _fat_ptr _tmp797=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t2);}));_tmp6C0.f1=_tmp797;});_tmp6C0;});void*_tmp34C[2U];_tmp34C[0]=& _tmp34E,_tmp34C[1]=& _tmp34F;({struct _fat_ptr _tmp798=({const char*_tmp34D="typecmp: function type comparison should never get here! %s and %s";_tag_fat(_tmp34D,sizeof(char),67U);});Cyc_aprintf(_tmp798,_tag_fat(_tmp34C,sizeof(void*),2U));});});_tmp79A(_tmp799,_tag_fat(_tmp34B,sizeof(void*),0U));});});}}}}}else{goto _LL15;}case 6U: if(((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp313.f2)->tag == 6U){_LLD: _tmp31C=((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp313.f1)->f1;_tmp31D=((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp313.f2)->f1;_LLE: {struct Cyc_List_List*ts2=_tmp31C;struct Cyc_List_List*ts1=_tmp31D;
# 1512
return({({int(*_tmp79C)(int(*cmp)(struct _tuple12*,struct _tuple12*),struct Cyc_List_List*l1,struct Cyc_List_List*l2)=({int(*_tmp351)(int(*cmp)(struct _tuple12*,struct _tuple12*),struct Cyc_List_List*l1,struct Cyc_List_List*l2)=(int(*)(int(*cmp)(struct _tuple12*,struct _tuple12*),struct Cyc_List_List*l1,struct Cyc_List_List*l2))Cyc_List_list_cmp;_tmp351;});struct Cyc_List_List*_tmp79B=ts1;_tmp79C(Cyc_Tcutil_tqual_type_cmp,_tmp79B,ts2);});});}}else{goto _LL15;}case 7U: if(((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp313.f2)->tag == 7U){_LLF: _tmp318=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp313.f1)->f1;_tmp319=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp313.f1)->f2;_tmp31A=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp313.f2)->f1;_tmp31B=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp313.f2)->f2;_LL10: {enum Cyc_Absyn_AggrKind k2=_tmp318;struct Cyc_List_List*fs2=_tmp319;enum Cyc_Absyn_AggrKind k1=_tmp31A;struct Cyc_List_List*fs1=_tmp31B;
# 1515
if((int)k1 != (int)k2){
if((int)k1 == (int)0U)return - 1;else{
return 1;}}
# 1515
return({({int(*_tmp79E)(int(*cmp)(struct Cyc_Absyn_Aggrfield*,struct Cyc_Absyn_Aggrfield*),struct Cyc_List_List*l1,struct Cyc_List_List*l2)=({
# 1518
int(*_tmp352)(int(*cmp)(struct Cyc_Absyn_Aggrfield*,struct Cyc_Absyn_Aggrfield*),struct Cyc_List_List*l1,struct Cyc_List_List*l2)=(int(*)(int(*cmp)(struct Cyc_Absyn_Aggrfield*,struct Cyc_Absyn_Aggrfield*),struct Cyc_List_List*l1,struct Cyc_List_List*l2))Cyc_List_list_cmp;_tmp352;});struct Cyc_List_List*_tmp79D=fs1;_tmp79E(Cyc_Tcutil_aggrfield_cmp,_tmp79D,fs2);});});}}else{goto _LL15;}case 9U: if(((struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_tmp313.f2)->tag == 9U){_LL11: _tmp316=((struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_tmp313.f1)->f1;_tmp317=((struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_tmp313.f2)->f1;_LL12: {struct Cyc_Absyn_Exp*e1=_tmp316;struct Cyc_Absyn_Exp*e2=_tmp317;
# 1520
_tmp314=e1;_tmp315=e2;goto _LL14;}}else{goto _LL15;}case 11U: if(((struct Cyc_Absyn_TypeofType_Absyn_Type_struct*)_tmp313.f2)->tag == 11U){_LL13: _tmp314=((struct Cyc_Absyn_TypeofType_Absyn_Type_struct*)_tmp313.f1)->f1;_tmp315=((struct Cyc_Absyn_TypeofType_Absyn_Type_struct*)_tmp313.f2)->f1;_LL14: {struct Cyc_Absyn_Exp*e1=_tmp314;struct Cyc_Absyn_Exp*e2=_tmp315;
# 1522
return({Cyc_Evexp_const_exp_cmp(e1,e2);});}}else{goto _LL15;}default: _LL15: _LL16:
({void*_tmp353=0U;({int(*_tmp7A0)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp355)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Tcutil_impos;_tmp355;});struct _fat_ptr _tmp79F=({const char*_tmp354="Unmatched case in typecmp";_tag_fat(_tmp354,sizeof(char),26U);});_tmp7A0(_tmp79F,_tag_fat(_tmp353,sizeof(void*),0U));});});}_LL0:;}}}
# 1529
static int Cyc_Tcutil_will_lose_precision(void*t1,void*t2){
struct _tuple15 _stmttmp41=({struct _tuple15 _tmp6C4;({void*_tmp7A2=({Cyc_Tcutil_compress(t1);});_tmp6C4.f1=_tmp7A2;}),({void*_tmp7A1=({Cyc_Tcutil_compress(t2);});_tmp6C4.f2=_tmp7A1;});_tmp6C4;});struct _tuple15 _tmp35A=_stmttmp41;void*_tmp35C;void*_tmp35B;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp35A.f1)->tag == 0U){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp35A.f2)->tag == 0U){_LL1: _tmp35B=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp35A.f1)->f1;_tmp35C=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp35A.f2)->f1;_LL2: {void*c1=_tmp35B;void*c2=_tmp35C;
# 1532
struct _tuple15 _stmttmp42=({struct _tuple15 _tmp6C3;_tmp6C3.f1=c1,_tmp6C3.f2=c2;_tmp6C3;});struct _tuple15 _tmp35D=_stmttmp42;int _tmp35F;int _tmp35E;switch(*((int*)_tmp35D.f1)){case 2U: switch(*((int*)_tmp35D.f2)){case 2U: _LL6: _tmp35E=((struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct*)_tmp35D.f1)->f1;_tmp35F=((struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct*)_tmp35D.f2)->f1;_LL7: {int i1=_tmp35E;int i2=_tmp35F;
return i2 < i1;}case 1U: _LL8: _LL9:
 goto _LLB;case 5U: _LLA: _LLB:
 return 1;default: goto _LL26;}case 1U: switch(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp35D.f1)->f2){case Cyc_Absyn_LongLong_sz: if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp35D.f2)->tag == 1U){if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp35D.f2)->f2 == Cyc_Absyn_LongLong_sz){_LLC: _LLD:
 return 0;}else{goto _LLE;}}else{_LLE: _LLF:
 return 1;}case Cyc_Absyn_Long_sz: switch(*((int*)_tmp35D.f2)){case 1U: switch(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp35D.f2)->f2){case Cyc_Absyn_Int_sz: _LL10: _LL11:
# 1540
 goto _LL13;case Cyc_Absyn_Short_sz: _LL18: _LL19:
# 1545
 goto _LL1B;case Cyc_Absyn_Char_sz: _LL1E: _LL1F:
# 1548
 goto _LL21;default: goto _LL26;}case 2U: if(((struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct*)_tmp35D.f2)->f1 == 0){_LL14: _LL15:
# 1543
 goto _LL17;}else{goto _LL26;}default: goto _LL26;}case Cyc_Absyn_Int_sz: switch(*((int*)_tmp35D.f2)){case 1U: switch(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp35D.f2)->f2){case Cyc_Absyn_Long_sz: _LL12: _LL13:
# 1541
 return 0;case Cyc_Absyn_Short_sz: _LL1A: _LL1B:
# 1546
 goto _LL1D;case Cyc_Absyn_Char_sz: _LL20: _LL21:
# 1549
 goto _LL23;default: goto _LL26;}case 2U: if(((struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct*)_tmp35D.f2)->f1 == 0){_LL16: _LL17:
# 1544
 goto _LL19;}else{goto _LL26;}default: goto _LL26;}case Cyc_Absyn_Short_sz: if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp35D.f2)->tag == 1U){if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp35D.f2)->f2 == Cyc_Absyn_Char_sz){_LL22: _LL23:
# 1550
 goto _LL25;}else{goto _LL26;}}else{goto _LL26;}default: goto _LL26;}case 5U: if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp35D.f2)->tag == 1U)switch(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp35D.f2)->f2){case Cyc_Absyn_Short_sz: _LL1C: _LL1D:
# 1547
 goto _LL1F;case Cyc_Absyn_Char_sz: _LL24: _LL25:
# 1551
 return 1;default: goto _LL26;}else{goto _LL26;}default: _LL26: _LL27:
# 1553
 return 0;}_LL5:;}}else{goto _LL3;}}else{_LL3: _LL4:
# 1555
 return 0;}_LL0:;}
# 1529
void*Cyc_Tcutil_max_arithmetic_type(void*t1,void*t2){
# 1560
{struct _tuple15 _stmttmp43=({struct _tuple15 _tmp6C6;({void*_tmp7A4=({Cyc_Tcutil_compress(t1);});_tmp6C6.f1=_tmp7A4;}),({void*_tmp7A3=({Cyc_Tcutil_compress(t2);});_tmp6C6.f2=_tmp7A3;});_tmp6C6;});struct _tuple15 _tmp361=_stmttmp43;void*_tmp363;void*_tmp362;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp361.f1)->tag == 0U){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp361.f2)->tag == 0U){_LL1: _tmp362=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp361.f1)->f1;_tmp363=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp361.f2)->f1;_LL2: {void*c1=_tmp362;void*c2=_tmp363;
# 1562
{struct _tuple15 _stmttmp44=({struct _tuple15 _tmp6C5;_tmp6C5.f1=c1,_tmp6C5.f2=c2;_tmp6C5;});struct _tuple15 _tmp364=_stmttmp44;int _tmp366;int _tmp365;if(((struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct*)_tmp364.f1)->tag == 2U){if(((struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct*)_tmp364.f2)->tag == 2U){_LL6: _tmp365=((struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct*)_tmp364.f1)->f1;_tmp366=((struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct*)_tmp364.f2)->f1;_LL7: {int i1=_tmp365;int i2=_tmp366;
# 1564
if(i1 != 0 && i1 != 1)return t1;else{
if(i2 != 0 && i2 != 1)return t2;else{
if(i1 >= i2)return t1;else{
return t2;}}}}}else{_LL8: _LL9:
 return t1;}}else{if(((struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct*)_tmp364.f2)->tag == 2U){_LLA: _LLB:
 return t2;}else{if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp364.f1)->tag == 1U){if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp364.f1)->f1 == Cyc_Absyn_Unsigned){if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp364.f1)->f2 == Cyc_Absyn_LongLong_sz){_LLC: _LLD:
 goto _LLF;}else{if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp364.f2)->tag == 1U){if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp364.f2)->f1 == Cyc_Absyn_Unsigned){if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp364.f2)->f2 == Cyc_Absyn_LongLong_sz)goto _LLE;else{if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp364.f1)->f2 == Cyc_Absyn_Long_sz)goto _LL14;else{if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp364.f2)->f2 == Cyc_Absyn_Long_sz)goto _LL16;else{if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp364.f1)->f2 == Cyc_Absyn_Int_sz)goto _LL1C;else{if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp364.f2)->f2 == Cyc_Absyn_Int_sz)goto _LL1E;else{goto _LL24;}}}}}}else{if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp364.f2)->f2 == Cyc_Absyn_LongLong_sz)goto _LL12;else{switch(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp364.f1)->f2){case Cyc_Absyn_Long_sz: goto _LL14;case Cyc_Absyn_Int_sz: goto _LL1C;default: if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp364.f2)->f2 == Cyc_Absyn_Long_sz)goto _LL22;else{goto _LL24;}}}}}else{if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp364.f1)->f2 == Cyc_Absyn_Long_sz){_LL14: _LL15:
# 1574
 goto _LL17;}else{if(((struct Cyc_Absyn_TagCon_Absyn_TyCon_struct*)_tmp364.f2)->tag == 5U)goto _LL1A;else{if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp364.f1)->f2 == Cyc_Absyn_Int_sz){_LL1C: _LL1D:
# 1579
 goto _LL1F;}else{goto _LL24;}}}}}}else{if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp364.f2)->tag == 1U){if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp364.f2)->f1 == Cyc_Absyn_Unsigned){if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp364.f2)->f2 == Cyc_Absyn_LongLong_sz)goto _LLE;else{if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp364.f1)->f2 == Cyc_Absyn_LongLong_sz)goto _LL10;else{switch(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp364.f2)->f2){case Cyc_Absyn_Long_sz: goto _LL16;case Cyc_Absyn_Int_sz: goto _LL1E;default: if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp364.f1)->f2 == Cyc_Absyn_Long_sz)goto _LL20;else{goto _LL24;}}}}}else{if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp364.f1)->f2 == Cyc_Absyn_LongLong_sz)goto _LL10;else{if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp364.f2)->f2 == Cyc_Absyn_LongLong_sz)goto _LL12;else{if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp364.f1)->f2 == Cyc_Absyn_Long_sz)goto _LL20;else{if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp364.f2)->f2 == Cyc_Absyn_Long_sz)goto _LL22;else{goto _LL24;}}}}}}else{if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp364.f1)->f2 == Cyc_Absyn_LongLong_sz){_LL10: _LL11:
# 1572
 goto _LL13;}else{if(((struct Cyc_Absyn_TagCon_Absyn_TyCon_struct*)_tmp364.f2)->tag == 5U)goto _LL1A;else{if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp364.f1)->f2 == Cyc_Absyn_Long_sz){_LL20: _LL21:
# 1581
 goto _LL23;}else{goto _LL24;}}}}}}else{if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp364.f2)->tag == 1U){if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp364.f2)->f1 == Cyc_Absyn_Unsigned)switch(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp364.f2)->f2){case Cyc_Absyn_LongLong_sz: _LLE: _LLF:
# 1571
 return Cyc_Absyn_ulonglong_type;case Cyc_Absyn_Long_sz: _LL16: _LL17:
# 1575
 return Cyc_Absyn_ulong_type;default: if(((struct Cyc_Absyn_TagCon_Absyn_TyCon_struct*)_tmp364.f1)->tag == 5U)goto _LL18;else{if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp364.f2)->f2 == Cyc_Absyn_Int_sz){_LL1E: _LL1F:
# 1580
 return Cyc_Absyn_uint_type;}else{goto _LL24;}}}else{if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp364.f2)->f2 == Cyc_Absyn_LongLong_sz){_LL12: _LL13:
# 1573
 return Cyc_Absyn_slonglong_type;}else{if(((struct Cyc_Absyn_TagCon_Absyn_TyCon_struct*)_tmp364.f1)->tag == 5U)goto _LL18;else{if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp364.f2)->f2 == Cyc_Absyn_Long_sz){_LL22: _LL23:
# 1582
 return Cyc_Absyn_slong_type;}else{goto _LL24;}}}}}else{if(((struct Cyc_Absyn_TagCon_Absyn_TyCon_struct*)_tmp364.f1)->tag == 5U){_LL18: _LL19:
# 1577
 goto _LL1B;}else{if(((struct Cyc_Absyn_TagCon_Absyn_TyCon_struct*)_tmp364.f2)->tag == 5U){_LL1A: _LL1B:
 goto _LL1D;}else{_LL24: _LL25:
# 1583
 goto _LL5;}}}}}}_LL5:;}
# 1585
goto _LL0;}}else{goto _LL3;}}else{_LL3: _LL4:
 goto _LL0;}_LL0:;}
# 1588
return Cyc_Absyn_sint_type;}
# 1529
int Cyc_Tcutil_coerce_list(struct Cyc_Tcenv_Tenv*te,void*t,struct Cyc_List_List*es){
# 1596
struct Cyc_Core_Opt*max_arith_type=0;
{struct Cyc_List_List*el=es;for(0;el != 0;el=el->tl){
void*t1=({Cyc_Tcutil_compress((void*)_check_null(((struct Cyc_Absyn_Exp*)el->hd)->topt));});
if(({Cyc_Tcutil_is_arithmetic_type(t1);})){
if(max_arith_type == 0 ||({Cyc_Tcutil_will_lose_precision(t1,(void*)max_arith_type->v);}))
# 1602
max_arith_type=({struct Cyc_Core_Opt*_tmp368=_cycalloc(sizeof(*_tmp368));_tmp368->v=t1;_tmp368;});}}}
# 1605
if(max_arith_type != 0){
if(!({Cyc_Unify_unify(t,(void*)max_arith_type->v);}))
return 0;}
# 1605
{
# 1609
struct Cyc_List_List*el=es;
# 1605
for(0;el != 0;el=el->tl){
# 1610
if(!({Cyc_Tcutil_coerce_assign(te,(struct Cyc_Absyn_Exp*)el->hd,t);})){
({struct Cyc_String_pa_PrintArg_struct _tmp36B=({struct Cyc_String_pa_PrintArg_struct _tmp6C8;_tmp6C8.tag=0U,({
struct _fat_ptr _tmp7A5=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t);}));_tmp6C8.f1=_tmp7A5;});_tmp6C8;});struct Cyc_String_pa_PrintArg_struct _tmp36C=({struct Cyc_String_pa_PrintArg_struct _tmp6C7;_tmp6C7.tag=0U,({struct _fat_ptr _tmp7A6=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string((void*)_check_null(((struct Cyc_Absyn_Exp*)el->hd)->topt));}));_tmp6C7.f1=_tmp7A6;});_tmp6C7;});void*_tmp369[2U];_tmp369[0]=& _tmp36B,_tmp369[1]=& _tmp36C;({unsigned _tmp7A8=((struct Cyc_Absyn_Exp*)el->hd)->loc;struct _fat_ptr _tmp7A7=({const char*_tmp36A="type mismatch: expecting %s but found %s";_tag_fat(_tmp36A,sizeof(char),41U);});Cyc_Tcutil_terr(_tmp7A8,_tmp7A7,_tag_fat(_tmp369,sizeof(void*),2U));});});
return 0;}}}
# 1605
return 1;}
# 1529
int Cyc_Tcutil_coerce_to_bool(struct Cyc_Absyn_Exp*e){
# 1621
if(!({Cyc_Tcutil_coerce_sint_type(e);})){
void*_stmttmp45=({Cyc_Tcutil_compress((void*)_check_null(e->topt));});void*_tmp36E=_stmttmp45;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp36E)->tag == 3U){_LL1: _LL2:
({Cyc_Tcutil_unchecked_cast(e,Cyc_Absyn_uint_type,Cyc_Absyn_Other_coercion);});goto _LL0;}else{_LL3: _LL4:
 return 0;}_LL0:;}
# 1621
return 1;}
# 1529
int Cyc_Tcutil_coerce_uint_type(struct Cyc_Absyn_Exp*e){
# 1631
if(({Cyc_Unify_unify((void*)_check_null(e->topt),Cyc_Absyn_uint_type);}))
return 1;
# 1631
if(({Cyc_Tcutil_is_integral_type((void*)_check_null(e->topt));})){
# 1635
if(({Cyc_Tcutil_will_lose_precision((void*)_check_null(e->topt),Cyc_Absyn_uint_type);}))
({void*_tmp370=0U;({unsigned _tmp7AA=e->loc;struct _fat_ptr _tmp7A9=({const char*_tmp371="integral size mismatch; conversion supplied";_tag_fat(_tmp371,sizeof(char),44U);});Cyc_Tcutil_warn(_tmp7AA,_tmp7A9,_tag_fat(_tmp370,sizeof(void*),0U));});});
# 1635
({Cyc_Tcutil_unchecked_cast(e,Cyc_Absyn_uint_type,Cyc_Absyn_No_coercion);});
# 1638
return 1;}
# 1631
return 0;}
# 1529
int Cyc_Tcutil_coerce_sint_type(struct Cyc_Absyn_Exp*e){
# 1645
if(({Cyc_Unify_unify((void*)_check_null(e->topt),Cyc_Absyn_sint_type);}))
return 1;
# 1645
if(({Cyc_Tcutil_is_integral_type((void*)_check_null(e->topt));})){
# 1649
if(({Cyc_Tcutil_will_lose_precision((void*)_check_null(e->topt),Cyc_Absyn_sint_type);}))
({void*_tmp373=0U;({unsigned _tmp7AC=e->loc;struct _fat_ptr _tmp7AB=({const char*_tmp374="integral size mismatch; conversion supplied";_tag_fat(_tmp374,sizeof(char),44U);});Cyc_Tcutil_warn(_tmp7AC,_tmp7AB,_tag_fat(_tmp373,sizeof(void*),0U));});});
# 1649
({Cyc_Tcutil_unchecked_cast(e,Cyc_Absyn_sint_type,Cyc_Absyn_No_coercion);});
# 1652
return 1;}
# 1645
return 0;}
# 1529
int Cyc_Tcutil_force_type2bool(int desired,void*t){
# 1660
({Cyc_Unify_unify(desired?Cyc_Absyn_true_type: Cyc_Absyn_false_type,t);});
return({Cyc_Absyn_type2bool(desired,t);});}
# 1529
void*Cyc_Tcutil_force_bounds_one(void*t){
# 1666
({int(*_tmp377)(void*,void*)=Cyc_Unify_unify;void*_tmp378=t;void*_tmp379=({Cyc_Absyn_bounds_one();});_tmp377(_tmp378,_tmp379);});
return({Cyc_Tcutil_compress(t);});}
# 1529
struct Cyc_Absyn_Exp*Cyc_Tcutil_get_thin_bound(struct Cyc_List_List*ts){
# 1671
void*t=({Cyc_Tcutil_compress((void*)((struct Cyc_List_List*)_check_null(ts))->hd);});
void*_tmp37B=t;struct Cyc_Absyn_Exp*_tmp37C;if(((struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_tmp37B)->tag == 9U){_LL1: _tmp37C=((struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_tmp37B)->f1;_LL2: {struct Cyc_Absyn_Exp*e=_tmp37C;
return e;}}else{_LL3: _LL4: {
# 1675
struct Cyc_Absyn_Exp*v=({Cyc_Absyn_valueof_exp(t,0U);});
v->topt=Cyc_Absyn_uint_type;
return v;}}_LL0:;}
# 1529
struct Cyc_Absyn_Exp*Cyc_Tcutil_get_bounds_exp(void*def,void*b){
# 1685
({Cyc_Unify_unify(def,b);});{
void*_stmttmp46=({Cyc_Tcutil_compress(b);});void*_tmp37E=_stmttmp46;struct Cyc_List_List*_tmp37F;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp37E)->tag == 0U)switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp37E)->f1)){case 16U: _LL1: _LL2:
 return 0;case 15U: _LL3: _tmp37F=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp37E)->f2;_LL4: {struct Cyc_List_List*ts=_tmp37F;
return({Cyc_Tcutil_get_thin_bound(ts);});}default: goto _LL5;}else{_LL5: _LL6:
({struct Cyc_String_pa_PrintArg_struct _tmp383=({struct Cyc_String_pa_PrintArg_struct _tmp6C9;_tmp6C9.tag=0U,({struct _fat_ptr _tmp7AD=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(b);}));_tmp6C9.f1=_tmp7AD;});_tmp6C9;});void*_tmp380[1U];_tmp380[0]=& _tmp383;({int(*_tmp7AF)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp382)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Tcutil_impos;_tmp382;});struct _fat_ptr _tmp7AE=({const char*_tmp381="get_bounds_exp: %s";_tag_fat(_tmp381,sizeof(char),19U);});_tmp7AF(_tmp7AE,_tag_fat(_tmp380,sizeof(void*),1U));});});}_LL0:;}}
# 1529
struct Cyc_Absyn_Exp*Cyc_Tcutil_get_ptr_bounds_exp(void*def,void*t){
# 1694
void*_stmttmp47=({Cyc_Tcutil_compress(t);});void*_tmp385=_stmttmp47;void*_tmp386;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp385)->tag == 3U){_LL1: _tmp386=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp385)->f1).ptr_atts).bounds;_LL2: {void*b=_tmp386;
# 1696
return({Cyc_Tcutil_get_bounds_exp(def,b);});}}else{_LL3: _LL4:
({struct Cyc_String_pa_PrintArg_struct _tmp38A=({struct Cyc_String_pa_PrintArg_struct _tmp6CA;_tmp6CA.tag=0U,({struct _fat_ptr _tmp7B0=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t);}));_tmp6CA.f1=_tmp7B0;});_tmp6CA;});void*_tmp387[1U];_tmp387[0]=& _tmp38A;({int(*_tmp7B2)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp389)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Tcutil_impos;_tmp389;});struct _fat_ptr _tmp7B1=({const char*_tmp388="get_ptr_bounds_exp not pointer: %s";_tag_fat(_tmp388,sizeof(char),35U);});_tmp7B2(_tmp7B1,_tag_fat(_tmp387,sizeof(void*),1U));});});}_LL0:;}
# 1529
void*Cyc_Tcutil_any_bool(struct Cyc_List_List*tvs){
# 1703
return({Cyc_Absyn_new_evar(& Cyc_Tcutil_boolko,({struct Cyc_Core_Opt*_tmp38C=_cycalloc(sizeof(*_tmp38C));_tmp38C->v=tvs;_tmp38C;}));});}
# 1529
void*Cyc_Tcutil_any_bounds(struct Cyc_List_List*tvs){
# 1707
return({Cyc_Absyn_new_evar(& Cyc_Tcutil_ptrbko,({struct Cyc_Core_Opt*_tmp38E=_cycalloc(sizeof(*_tmp38E));_tmp38E->v=tvs;_tmp38E;}));});}
# 1529
static int Cyc_Tcutil_ptrsubtype(struct Cyc_Tcenv_Tenv*te,struct Cyc_List_List*assume,void*t1,void*t2);struct _tuple22{struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};
# 1842 "tcutil.cyc"
int Cyc_Tcutil_silent_castable(struct Cyc_Tcenv_Tenv*te,unsigned loc,void*t1,void*t2){
t1=({Cyc_Tcutil_compress(t1);});
t2=({Cyc_Tcutil_compress(t2);});{
struct _tuple15 _stmttmp48=({struct _tuple15 _tmp6CE;_tmp6CE.f1=t1,_tmp6CE.f2=t2;_tmp6CE;});struct _tuple15 _tmp390=_stmttmp48;void*_tmp398;struct Cyc_Absyn_Exp*_tmp397;struct Cyc_Absyn_Tqual _tmp396;void*_tmp395;void*_tmp394;struct Cyc_Absyn_Exp*_tmp393;struct Cyc_Absyn_Tqual _tmp392;void*_tmp391;struct Cyc_Absyn_PtrInfo _tmp39A;struct Cyc_Absyn_PtrInfo _tmp399;switch(*((int*)_tmp390.f1)){case 3U: if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp390.f2)->tag == 3U){_LL1: _tmp399=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp390.f1)->f1;_tmp39A=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp390.f2)->f1;_LL2: {struct Cyc_Absyn_PtrInfo pinfo_a=_tmp399;struct Cyc_Absyn_PtrInfo pinfo_b=_tmp39A;
# 1847
int okay=1;
# 1849
if(!({Cyc_Unify_unify((pinfo_a.ptr_atts).nullable,(pinfo_b.ptr_atts).nullable);}))
# 1851
okay=!({Cyc_Tcutil_force_type2bool(0,(pinfo_a.ptr_atts).nullable);});
# 1849
if(!({Cyc_Unify_unify((pinfo_a.ptr_atts).bounds,(pinfo_b.ptr_atts).bounds);})){
# 1854
struct _tuple22 _stmttmp49=({struct _tuple22 _tmp6CB;({struct Cyc_Absyn_Exp*_tmp7B4=({struct Cyc_Absyn_Exp*(*_tmp3A3)(void*def,void*b)=Cyc_Tcutil_get_bounds_exp;void*_tmp3A4=({Cyc_Absyn_bounds_one();});void*_tmp3A5=(pinfo_a.ptr_atts).bounds;_tmp3A3(_tmp3A4,_tmp3A5);});_tmp6CB.f1=_tmp7B4;}),({
struct Cyc_Absyn_Exp*_tmp7B3=({struct Cyc_Absyn_Exp*(*_tmp3A6)(void*def,void*b)=Cyc_Tcutil_get_bounds_exp;void*_tmp3A7=({Cyc_Absyn_bounds_one();});void*_tmp3A8=(pinfo_b.ptr_atts).bounds;_tmp3A6(_tmp3A7,_tmp3A8);});_tmp6CB.f2=_tmp7B3;});_tmp6CB;});
# 1854
struct _tuple22 _tmp39B=_stmttmp49;struct Cyc_Absyn_Exp*_tmp39D;struct Cyc_Absyn_Exp*_tmp39C;if(_tmp39B.f2 == 0){_LLA: _LLB:
# 1857
 okay=1;goto _LL9;}else{if(_tmp39B.f1 == 0){_LLC: _LLD:
# 1860
 if(({Cyc_Tcutil_force_type2bool(0,(pinfo_a.ptr_atts).zero_term);})&&({int(*_tmp39E)(void*,void*)=Cyc_Unify_unify;void*_tmp39F=({Cyc_Absyn_bounds_one();});void*_tmp3A0=(pinfo_b.ptr_atts).bounds;_tmp39E(_tmp39F,_tmp3A0);}))
# 1862
goto _LL9;
# 1860
okay=0;
# 1864
goto _LL9;}else{_LLE: _tmp39C=_tmp39B.f1;_tmp39D=_tmp39B.f2;_LLF: {struct Cyc_Absyn_Exp*e1=_tmp39C;struct Cyc_Absyn_Exp*e2=_tmp39D;
# 1866
okay=okay &&({({struct Cyc_Absyn_Exp*_tmp7B5=(struct Cyc_Absyn_Exp*)_check_null(e2);Cyc_Evexp_lte_const_exp(_tmp7B5,(struct Cyc_Absyn_Exp*)_check_null(e1));});});
# 1870
if(!({Cyc_Tcutil_force_type2bool(0,(pinfo_b.ptr_atts).zero_term);}))
({void*_tmp3A1=0U;({unsigned _tmp7B7=loc;struct _fat_ptr _tmp7B6=({const char*_tmp3A2="implicit cast to shorter array";_tag_fat(_tmp3A2,sizeof(char),31U);});Cyc_Tcutil_warn(_tmp7B7,_tmp7B6,_tag_fat(_tmp3A1,sizeof(void*),0U));});});
# 1870
goto _LL9;}}}_LL9:;}
# 1849
okay=
# 1877
okay &&(!(pinfo_a.elt_tq).real_const ||(pinfo_b.elt_tq).real_const);
# 1880
if(!({Cyc_Unify_unify((pinfo_a.ptr_atts).rgn,(pinfo_b.ptr_atts).rgn);})){
int check_outlives=({Cyc_Tcenv_rgn_outlives_rgn(te,(pinfo_a.ptr_atts).rgn,(pinfo_b.ptr_atts).rgn);});
# 1885
if(check_outlives){
if(Cyc_Tcutil_warn_region_coerce)
({struct Cyc_String_pa_PrintArg_struct _tmp3AB=({struct Cyc_String_pa_PrintArg_struct _tmp6CD;_tmp6CD.tag=0U,({
struct _fat_ptr _tmp7B8=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string((pinfo_a.ptr_atts).rgn);}));_tmp6CD.f1=_tmp7B8;});_tmp6CD;});struct Cyc_String_pa_PrintArg_struct _tmp3AC=({struct Cyc_String_pa_PrintArg_struct _tmp6CC;_tmp6CC.tag=0U,({
struct _fat_ptr _tmp7B9=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string((pinfo_b.ptr_atts).rgn);}));_tmp6CC.f1=_tmp7B9;});_tmp6CC;});void*_tmp3A9[2U];_tmp3A9[0]=& _tmp3AB,_tmp3A9[1]=& _tmp3AC;({unsigned _tmp7BB=loc;struct _fat_ptr _tmp7BA=({const char*_tmp3AA="implicit cast from region %s to region %s";_tag_fat(_tmp3AA,sizeof(char),42U);});Cyc_Tcutil_warn(_tmp7BB,_tmp7BA,_tag_fat(_tmp3A9,sizeof(void*),2U));});});}else{
okay=0;}}
# 1880
okay=
# 1894
okay &&(({Cyc_Unify_unify((pinfo_a.ptr_atts).zero_term,(pinfo_b.ptr_atts).zero_term);})||
# 1896
({Cyc_Tcutil_force_type2bool(1,(pinfo_a.ptr_atts).zero_term);})&&(pinfo_b.elt_tq).real_const);{
# 1904
int deep_subtype=
({int(*_tmp3B0)(void*,void*)=Cyc_Unify_unify;void*_tmp3B1=({Cyc_Absyn_bounds_one();});void*_tmp3B2=(pinfo_b.ptr_atts).bounds;_tmp3B0(_tmp3B1,_tmp3B2);})&& !({Cyc_Tcutil_force_type2bool(0,(pinfo_b.ptr_atts).zero_term);});
# 1910
okay=okay &&(({Cyc_Unify_unify(pinfo_a.elt_type,pinfo_b.elt_type);})||
(deep_subtype &&((pinfo_b.elt_tq).real_const ||({int(*_tmp3AD)(struct Cyc_Absyn_Kind*ka1,struct Cyc_Absyn_Kind*ka2)=Cyc_Tcutil_kind_leq;struct Cyc_Absyn_Kind*_tmp3AE=& Cyc_Tcutil_ak;struct Cyc_Absyn_Kind*_tmp3AF=({Cyc_Tcutil_type_kind(pinfo_b.elt_type);});_tmp3AD(_tmp3AE,_tmp3AF);})))&&({Cyc_Tcutil_ptrsubtype(te,0,pinfo_a.elt_type,pinfo_b.elt_type);}));
# 1915
return okay;}}}else{goto _LL7;}case 4U: if(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp390.f2)->tag == 4U){_LL3: _tmp391=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp390.f1)->f1).elt_type;_tmp392=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp390.f1)->f1).tq;_tmp393=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp390.f1)->f1).num_elts;_tmp394=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp390.f1)->f1).zero_term;_tmp395=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp390.f2)->f1).elt_type;_tmp396=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp390.f2)->f1).tq;_tmp397=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp390.f2)->f1).num_elts;_tmp398=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp390.f2)->f1).zero_term;_LL4: {void*t1a=_tmp391;struct Cyc_Absyn_Tqual tq1a=_tmp392;struct Cyc_Absyn_Exp*e1=_tmp393;void*zt1=_tmp394;void*t2a=_tmp395;struct Cyc_Absyn_Tqual tq2a=_tmp396;struct Cyc_Absyn_Exp*e2=_tmp397;void*zt2=_tmp398;
# 1919
int okay;
# 1922
okay=({Cyc_Unify_unify(zt1,zt2);})&&(
(e1 != 0 && e2 != 0)&&({Cyc_Evexp_same_const_exp(e1,e2);}));
# 1925
return(okay &&({Cyc_Unify_unify(t1a,t2a);}))&&(!tq1a.real_const || tq2a.real_const);}}else{goto _LL7;}case 0U: if(((struct Cyc_Absyn_TagCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp390.f1)->f1)->tag == 5U){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp390.f2)->tag == 0U){if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp390.f2)->f1)->tag == 1U){_LL5: _LL6:
# 1927
 return 0;}else{goto _LL7;}}else{goto _LL7;}}else{goto _LL7;}default: _LL7: _LL8:
# 1929
 return({Cyc_Unify_unify(t1,t2);});}_LL0:;}}
# 1842
void*Cyc_Tcutil_pointer_elt_type(void*t){
# 1934
void*_stmttmp4A=({Cyc_Tcutil_compress(t);});void*_tmp3B4=_stmttmp4A;void*_tmp3B5;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp3B4)->tag == 3U){_LL1: _tmp3B5=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp3B4)->f1).elt_type;_LL2: {void*e=_tmp3B5;
return e;}}else{_LL3: _LL4:
({void*_tmp3B6=0U;({int(*_tmp7BD)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp3B8)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Tcutil_impos;_tmp3B8;});struct _fat_ptr _tmp7BC=({const char*_tmp3B7="pointer_elt_type";_tag_fat(_tmp3B7,sizeof(char),17U);});_tmp7BD(_tmp7BC,_tag_fat(_tmp3B6,sizeof(void*),0U));});});}_LL0:;}
# 1842
void*Cyc_Tcutil_pointer_region(void*t){
# 1940
void*_stmttmp4B=({Cyc_Tcutil_compress(t);});void*_tmp3BA=_stmttmp4B;struct Cyc_Absyn_PtrAtts*_tmp3BB;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp3BA)->tag == 3U){_LL1: _tmp3BB=(struct Cyc_Absyn_PtrAtts*)&(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp3BA)->f1).ptr_atts;_LL2: {struct Cyc_Absyn_PtrAtts*p=_tmp3BB;
return p->rgn;}}else{_LL3: _LL4:
({void*_tmp3BC=0U;({int(*_tmp7BF)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp3BE)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Tcutil_impos;_tmp3BE;});struct _fat_ptr _tmp7BE=({const char*_tmp3BD="pointer_elt_type";_tag_fat(_tmp3BD,sizeof(char),17U);});_tmp7BF(_tmp7BE,_tag_fat(_tmp3BC,sizeof(void*),0U));});});}_LL0:;}
# 1842
int Cyc_Tcutil_rgn_of_pointer(void*t,void**rgn){
# 1947
void*_stmttmp4C=({Cyc_Tcutil_compress(t);});void*_tmp3C0=_stmttmp4C;void*_tmp3C1;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp3C0)->tag == 3U){_LL1: _tmp3C1=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp3C0)->f1).ptr_atts).rgn;_LL2: {void*r=_tmp3C1;
# 1949
*rgn=r;
return 1;}}else{_LL3: _LL4:
 return 0;}_LL0:;}
# 1842
int Cyc_Tcutil_admits_zero(void*t){
# 1959
void*_stmttmp4D=({Cyc_Tcutil_compress(t);});void*_tmp3C3=_stmttmp4D;void*_tmp3C5;void*_tmp3C4;switch(*((int*)_tmp3C3)){case 0U: switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp3C3)->f1)){case 1U: _LL1: _LL2:
 goto _LL4;case 2U: _LL3: _LL4:
 return 1;default: goto _LL7;}case 3U: _LL5: _tmp3C4=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp3C3)->f1).ptr_atts).nullable;_tmp3C5=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp3C3)->f1).ptr_atts).bounds;_LL6: {void*nullable=_tmp3C4;void*bounds=_tmp3C5;
# 1966
return !({Cyc_Unify_unify(Cyc_Absyn_fat_bound_type,bounds);})&&({Cyc_Tcutil_force_type2bool(0,nullable);});}default: _LL7: _LL8:
 return 0;}_LL0:;}
# 1842
int Cyc_Tcutil_is_zero(struct Cyc_Absyn_Exp*e){
# 1973
void*_stmttmp4E=e->r;void*_tmp3C7=_stmttmp4E;struct Cyc_Absyn_Exp*_tmp3C9;void*_tmp3C8;struct _fat_ptr _tmp3CA;switch(*((int*)_tmp3C7)){case 0U: switch(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp3C7)->f1).Wchar_c).tag){case 5U: if((((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp3C7)->f1).Int_c).val).f2 == 0){_LL1: _LL2:
 goto _LL4;}else{goto _LLF;}case 2U: if((((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp3C7)->f1).Char_c).val).f2 == 0){_LL3: _LL4:
 goto _LL6;}else{goto _LLF;}case 4U: if((((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp3C7)->f1).Short_c).val).f2 == 0){_LL5: _LL6:
 goto _LL8;}else{goto _LLF;}case 6U: if((((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp3C7)->f1).LongLong_c).val).f2 == 0){_LL7: _LL8:
 goto _LLA;}else{goto _LLF;}case 3U: _LLB: _tmp3CA=((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp3C7)->f1).Wchar_c).val;_LLC: {struct _fat_ptr s=_tmp3CA;
# 1981
unsigned long l=({Cyc_strlen((struct _fat_ptr)s);});
int i=0;
if(l >= (unsigned long)2 &&(int)*((const char*)_check_fat_subscript(s,sizeof(char),0))== (int)'\\'){
if((int)*((const char*)_check_fat_subscript(s,sizeof(char),1))== (int)'0')i=2;else{
if(((int)((const char*)s.curr)[1]== (int)'x' && l >= (unsigned long)3)&&(int)*((const char*)_check_fat_subscript(s,sizeof(char),2))== (int)'0')i=3;else{
return 0;}}
for(0;(unsigned long)i < l;++ i){
if((int)*((const char*)_check_fat_subscript(s,sizeof(char),i))!= (int)'0')return 0;}
# 1987
return 1;}else{
# 1991
return 0;}}default: goto _LLF;}case 2U: _LL9: _LLA:
# 1979
 return 1;case 14U: _LLD: _tmp3C8=(void*)((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp3C7)->f1;_tmp3C9=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp3C7)->f2;_LLE: {void*t=_tmp3C8;struct Cyc_Absyn_Exp*e2=_tmp3C9;
# 1992
return({Cyc_Tcutil_is_zero(e2);})&&({Cyc_Tcutil_admits_zero(t);});}default: _LLF: _LL10:
 return 0;}_LL0:;}
# 1842
struct Cyc_Absyn_Kind Cyc_Tcutil_xrk={Cyc_Absyn_XRgnKind,Cyc_Absyn_Aliasable};
# 1998
struct Cyc_Absyn_Kind Cyc_Tcutil_rk={Cyc_Absyn_RgnKind,Cyc_Absyn_Aliasable};
struct Cyc_Absyn_Kind Cyc_Tcutil_ak={Cyc_Absyn_AnyKind,Cyc_Absyn_Aliasable};
struct Cyc_Absyn_Kind Cyc_Tcutil_bk={Cyc_Absyn_BoxKind,Cyc_Absyn_Aliasable};
struct Cyc_Absyn_Kind Cyc_Tcutil_mk={Cyc_Absyn_MemKind,Cyc_Absyn_Aliasable};
struct Cyc_Absyn_Kind Cyc_Tcutil_ik={Cyc_Absyn_IntKind,Cyc_Absyn_Aliasable};
struct Cyc_Absyn_Kind Cyc_Tcutil_ek={Cyc_Absyn_EffKind,Cyc_Absyn_Aliasable};
struct Cyc_Absyn_Kind Cyc_Tcutil_boolk={Cyc_Absyn_BoolKind,Cyc_Absyn_Aliasable};
struct Cyc_Absyn_Kind Cyc_Tcutil_ptrbk={Cyc_Absyn_PtrBndKind,Cyc_Absyn_Aliasable};
# 2007
struct Cyc_Absyn_Kind Cyc_Tcutil_trk={Cyc_Absyn_RgnKind,Cyc_Absyn_Top};
struct Cyc_Absyn_Kind Cyc_Tcutil_tak={Cyc_Absyn_AnyKind,Cyc_Absyn_Top};
struct Cyc_Absyn_Kind Cyc_Tcutil_tbk={Cyc_Absyn_BoxKind,Cyc_Absyn_Top};
struct Cyc_Absyn_Kind Cyc_Tcutil_tmk={Cyc_Absyn_MemKind,Cyc_Absyn_Top};
# 2012
struct Cyc_Absyn_Kind Cyc_Tcutil_urk={Cyc_Absyn_RgnKind,Cyc_Absyn_Unique};
struct Cyc_Absyn_Kind Cyc_Tcutil_uak={Cyc_Absyn_AnyKind,Cyc_Absyn_Unique};
struct Cyc_Absyn_Kind Cyc_Tcutil_ubk={Cyc_Absyn_BoxKind,Cyc_Absyn_Unique};
struct Cyc_Absyn_Kind Cyc_Tcutil_umk={Cyc_Absyn_MemKind,Cyc_Absyn_Unique};
# 2017
struct Cyc_Core_Opt Cyc_Tcutil_rko={(void*)& Cyc_Tcutil_rk};
struct Cyc_Core_Opt Cyc_Tcutil_ako={(void*)& Cyc_Tcutil_ak};
struct Cyc_Core_Opt Cyc_Tcutil_bko={(void*)& Cyc_Tcutil_bk};
struct Cyc_Core_Opt Cyc_Tcutil_mko={(void*)& Cyc_Tcutil_mk};
struct Cyc_Core_Opt Cyc_Tcutil_iko={(void*)& Cyc_Tcutil_ik};
struct Cyc_Core_Opt Cyc_Tcutil_eko={(void*)& Cyc_Tcutil_ek};
struct Cyc_Core_Opt Cyc_Tcutil_boolko={(void*)& Cyc_Tcutil_boolk};
struct Cyc_Core_Opt Cyc_Tcutil_ptrbko={(void*)& Cyc_Tcutil_ptrbk};
# 2026
struct Cyc_Core_Opt Cyc_Tcutil_xrko={(void*)& Cyc_Tcutil_xrk};
struct Cyc_Core_Opt Cyc_Tcutil_trko={(void*)& Cyc_Tcutil_trk};
struct Cyc_Core_Opt Cyc_Tcutil_tako={(void*)& Cyc_Tcutil_tak};
struct Cyc_Core_Opt Cyc_Tcutil_tbko={(void*)& Cyc_Tcutil_tbk};
struct Cyc_Core_Opt Cyc_Tcutil_tmko={(void*)& Cyc_Tcutil_tmk};
# 2032
struct Cyc_Core_Opt Cyc_Tcutil_urko={(void*)& Cyc_Tcutil_urk};
struct Cyc_Core_Opt Cyc_Tcutil_uako={(void*)& Cyc_Tcutil_uak};
struct Cyc_Core_Opt Cyc_Tcutil_ubko={(void*)& Cyc_Tcutil_ubk};
struct Cyc_Core_Opt Cyc_Tcutil_umko={(void*)& Cyc_Tcutil_umk};
# 2037
struct Cyc_Core_Opt*Cyc_Tcutil_kind_to_opt(struct Cyc_Absyn_Kind*ka){
enum Cyc_Absyn_AliasQual _tmp3CD;enum Cyc_Absyn_KindQual _tmp3CC;_LL1: _tmp3CC=ka->kind;_tmp3CD=ka->aliasqual;_LL2: {enum Cyc_Absyn_KindQual k=_tmp3CC;enum Cyc_Absyn_AliasQual a=_tmp3CD;
{enum Cyc_Absyn_AliasQual _tmp3CE=a;switch(_tmp3CE){case Cyc_Absyn_Aliasable: _LL4: _LL5: {
# 2041
enum Cyc_Absyn_KindQual _tmp3CF=k;switch(_tmp3CF){case Cyc_Absyn_AnyKind: _LLD: _LLE:
 return& Cyc_Tcutil_ako;case Cyc_Absyn_MemKind: _LLF: _LL10:
 return& Cyc_Tcutil_mko;case Cyc_Absyn_BoxKind: _LL11: _LL12:
 return& Cyc_Tcutil_bko;case Cyc_Absyn_RgnKind: _LL13: _LL14:
 return& Cyc_Tcutil_rko;case Cyc_Absyn_XRgnKind: _LL15: _LL16:
 return& Cyc_Tcutil_xrko;case Cyc_Absyn_EffKind: _LL17: _LL18:
 return& Cyc_Tcutil_eko;case Cyc_Absyn_IntKind: _LL19: _LL1A:
 return& Cyc_Tcutil_iko;case Cyc_Absyn_BoolKind: _LL1B: _LL1C:
 return& Cyc_Tcutil_bko;case Cyc_Absyn_PtrBndKind: _LL1D: _LL1E:
 goto _LL20;default: _LL1F: _LL20: return& Cyc_Tcutil_ptrbko;}_LLC:;}case Cyc_Absyn_Unique: _LL6: _LL7:
# 2053
{enum Cyc_Absyn_KindQual _tmp3D0=k;switch(_tmp3D0){case Cyc_Absyn_AnyKind: _LL22: _LL23:
 return& Cyc_Tcutil_uako;case Cyc_Absyn_MemKind: _LL24: _LL25:
 return& Cyc_Tcutil_umko;case Cyc_Absyn_BoxKind: _LL26: _LL27:
 return& Cyc_Tcutil_ubko;case Cyc_Absyn_RgnKind: _LL28: _LL29:
 return& Cyc_Tcutil_urko;default: _LL2A: _LL2B:
 goto _LL21;}_LL21:;}
# 2060
goto _LL3;case Cyc_Absyn_Top: _LL8: _LL9:
# 2062
{enum Cyc_Absyn_KindQual _tmp3D1=k;switch(_tmp3D1){case Cyc_Absyn_AnyKind: _LL2D: _LL2E:
 return& Cyc_Tcutil_tako;case Cyc_Absyn_MemKind: _LL2F: _LL30:
 return& Cyc_Tcutil_tmko;case Cyc_Absyn_BoxKind: _LL31: _LL32:
 return& Cyc_Tcutil_tbko;case Cyc_Absyn_RgnKind: _LL33: _LL34:
 return& Cyc_Tcutil_trko;default: _LL35: _LL36:
 goto _LL2C;}_LL2C:;}
# 2069
goto _LL3;default: _LLA: _LLB:
 goto _LL3;}_LL3:;}
# 2072
({struct Cyc_String_pa_PrintArg_struct _tmp3D5=({struct Cyc_String_pa_PrintArg_struct _tmp6CF;_tmp6CF.tag=0U,({struct _fat_ptr _tmp7C0=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_kind2string(ka);}));_tmp6CF.f1=_tmp7C0;});_tmp6CF;});void*_tmp3D2[1U];_tmp3D2[0]=& _tmp3D5;({int(*_tmp7C2)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp3D4)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Tcutil_impos;_tmp3D4;});struct _fat_ptr _tmp7C1=({const char*_tmp3D3="kind_to_opt: bad kind %s\n";_tag_fat(_tmp3D3,sizeof(char),26U);});_tmp7C2(_tmp7C1,_tag_fat(_tmp3D2,sizeof(void*),1U));});});}}
# 2037
void*Cyc_Tcutil_kind_to_bound(struct Cyc_Absyn_Kind*k){
# 2076
return(void*)({struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*_tmp3D7=_cycalloc(sizeof(*_tmp3D7));_tmp3D7->tag=0U,_tmp3D7->f1=k;_tmp3D7;});}
# 2078
struct Cyc_Core_Opt*Cyc_Tcutil_kind_to_bound_opt(struct Cyc_Absyn_Kind*k){
return({struct Cyc_Core_Opt*_tmp3D9=_cycalloc(sizeof(*_tmp3D9));({void*_tmp7C3=({Cyc_Tcutil_kind_to_bound(k);});_tmp3D9->v=_tmp7C3;});_tmp3D9;});}
# 2078
int Cyc_Tcutil_zero_to_null(void*t2,struct Cyc_Absyn_Exp*e1){
# 2085
if(!({Cyc_Tcutil_is_zero(e1);}))
return 0;{
# 2085
void*_stmttmp4F=({Cyc_Tcutil_compress(t2);});void*_tmp3DB=_stmttmp4F;void*_tmp3DC;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp3DB)->tag == 3U){_LL1: _tmp3DC=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp3DB)->f1).ptr_atts).nullable;_LL2: {void*nbl=_tmp3DC;
# 2089
if(!({Cyc_Tcutil_force_type2bool(1,nbl);}))
return 0;
# 2089
({void*_tmp7C4=({Cyc_Absyn_null_exp(0U);})->r;e1->r=_tmp7C4;});
# 2092
e1->topt=t2;
return 1;}}else{_LL3: _LL4:
 return 0;}_LL0:;}}
# 2078
int Cyc_Tcutil_warn_alias_coerce=0;struct _tuple23{struct Cyc_Absyn_Decl*f1;struct Cyc_Absyn_Exp*f2;};
# 2104
struct _tuple23 Cyc_Tcutil_insert_alias(struct Cyc_Absyn_Exp*e,void*e_type){
static struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct rgn_kb={0U,& Cyc_Tcutil_rk};
# 2108
static int counter=0;
struct _tuple1*v=({struct _tuple1*_tmp3EE=_cycalloc(sizeof(*_tmp3EE));_tmp3EE->f1=Cyc_Absyn_Loc_n,({struct _fat_ptr*_tmp7C7=({struct _fat_ptr*_tmp3ED=_cycalloc(sizeof(*_tmp3ED));({struct _fat_ptr _tmp7C6=(struct _fat_ptr)({struct Cyc_Int_pa_PrintArg_struct _tmp3EC=({struct Cyc_Int_pa_PrintArg_struct _tmp6D1;_tmp6D1.tag=1U,_tmp6D1.f1=(unsigned long)counter ++;_tmp6D1;});void*_tmp3EA[1U];_tmp3EA[0]=& _tmp3EC;({struct _fat_ptr _tmp7C5=({const char*_tmp3EB="__aliasvar%d";_tag_fat(_tmp3EB,sizeof(char),13U);});Cyc_aprintf(_tmp7C5,_tag_fat(_tmp3EA,sizeof(void*),1U));});});*_tmp3ED=_tmp7C6;});_tmp3ED;});_tmp3EE->f2=_tmp7C7;});_tmp3EE;});
struct Cyc_Absyn_Vardecl*vd=({Cyc_Absyn_new_vardecl(0U,v,e_type,e);});
struct Cyc_Absyn_Exp*ve=({({void*_tmp7C8=(void*)({struct Cyc_Absyn_Local_b_Absyn_Binding_struct*_tmp3E9=_cycalloc(sizeof(*_tmp3E9));_tmp3E9->tag=4U,_tmp3E9->f1=vd;_tmp3E9;});Cyc_Absyn_varb_exp(_tmp7C8,e->loc);});});
# 2117
struct Cyc_Absyn_Tvar*tv=({Cyc_Tcutil_new_tvar((void*)& rgn_kb);});
# 2119
{void*_stmttmp50=({Cyc_Tcutil_compress(e_type);});void*_tmp3DE=_stmttmp50;struct Cyc_Absyn_PtrLoc*_tmp3E5;void*_tmp3E4;void*_tmp3E3;void*_tmp3E2;void*_tmp3E1;struct Cyc_Absyn_Tqual _tmp3E0;void*_tmp3DF;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp3DE)->tag == 3U){_LL1: _tmp3DF=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp3DE)->f1).elt_type;_tmp3E0=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp3DE)->f1).elt_tq;_tmp3E1=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp3DE)->f1).ptr_atts).rgn;_tmp3E2=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp3DE)->f1).ptr_atts).nullable;_tmp3E3=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp3DE)->f1).ptr_atts).bounds;_tmp3E4=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp3DE)->f1).ptr_atts).zero_term;_tmp3E5=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp3DE)->f1).ptr_atts).ptrloc;_LL2: {void*et=_tmp3DF;struct Cyc_Absyn_Tqual tq=_tmp3E0;void*old_r=_tmp3E1;void*nb=_tmp3E2;void*b=_tmp3E3;void*zt=_tmp3E4;struct Cyc_Absyn_PtrLoc*pl=_tmp3E5;
# 2121
{void*_stmttmp51=({Cyc_Tcutil_compress(old_r);});void*_tmp3E6=_stmttmp51;struct Cyc_Core_Opt*_tmp3E8;void**_tmp3E7;if(((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp3E6)->tag == 1U){_LL6: _tmp3E7=(void**)&((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp3E6)->f2;_tmp3E8=((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp3E6)->f4;_LL7: {void**topt=_tmp3E7;struct Cyc_Core_Opt*ts=_tmp3E8;
# 2123
void*new_r=({Cyc_Absyn_var_type(tv);});
*topt=new_r;
goto _LL5;}}else{_LL8: _LL9:
 goto _LL5;}_LL5:;}
# 2128
goto _LL0;}}else{_LL3: _LL4:
 goto _LL0;}_LL0:;}
# 2132
e->topt=0;
vd->initializer=0;{
# 2136
struct Cyc_Absyn_Decl*d=({Cyc_Absyn_alias_decl(tv,vd,e,e->loc);});
# 2138
return({struct _tuple23 _tmp6D0;_tmp6D0.f1=d,_tmp6D0.f2=ve;_tmp6D0;});}}
# 2143
static int Cyc_Tcutil_can_insert_alias(struct Cyc_Absyn_Exp*e,void*e_type,void*wants_type,unsigned loc){
# 2146
if((({Cyc_Tcutil_is_noalias_path(e);})&&({Cyc_Tcutil_is_noalias_pointer(e_type,0);}))&&({Cyc_Tcutil_is_pointer_type(e_type);})){
# 2151
void*_stmttmp52=({Cyc_Tcutil_compress(wants_type);});void*_tmp3F0=_stmttmp52;void*_tmp3F1;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp3F0)->tag == 3U){_LL1: _tmp3F1=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp3F0)->f1).ptr_atts).rgn;_LL2: {void*r2=_tmp3F1;
# 2153
if(({Cyc_Tcutil_is_heap_rgn_type(r2);}))return 0;{struct Cyc_Absyn_Kind*k=({Cyc_Tcutil_type_kind(r2);});
# 2155
return(int)k->kind == (int)Cyc_Absyn_RgnKind &&(int)k->aliasqual == (int)Cyc_Absyn_Aliasable;}}}else{_LL3: _LL4:
 return 0;}_LL0:;}
# 2146
return 0;}
# 2162
static struct Cyc_List_List*Cyc_Tcutil_get_pointer_effect(void*t){
# 2164
void*_stmttmp53=({Cyc_Tcutil_compress(t);});void*_tmp3F3=_stmttmp53;struct Cyc_Absyn_Tvar*_tmp3F4;struct Cyc_List_List*_tmp3F5;switch(*((int*)_tmp3F3)){case 0U: if(((struct Cyc_Absyn_JoinCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp3F3)->f1)->tag == 11U){_LL1: _tmp3F5=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp3F3)->f2;_LL2: {struct Cyc_List_List*ts=_tmp3F5;
# 2167
struct Cyc_List_List*ret=0;
for(0;ts != 0;ts=ts->tl){
# 2170
void*_stmttmp54=(void*)ts->hd;void*_tmp3F6=_stmttmp54;struct Cyc_Absyn_Tvar*_tmp3F7;if(((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp3F6)->tag == 2U){_LL8: _tmp3F7=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp3F6)->f1;_LL9: {struct Cyc_Absyn_Tvar*tv=_tmp3F7;
# 2172
ret=({struct Cyc_List_List*_tmp3F8=_cycalloc(sizeof(*_tmp3F8));_tmp3F8->hd=tv,_tmp3F8->tl=ret;_tmp3F8;});
goto _LL7;}}else{_LLA: _LLB:
 return 0;}_LL7:;}
# 2177
return ret;}}else{goto _LL5;}case 2U: _LL3: _tmp3F4=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp3F3)->f1;_LL4: {struct Cyc_Absyn_Tvar*tv=_tmp3F4;
return({struct Cyc_List_List*_tmp3F9=_cycalloc(sizeof(*_tmp3F9));_tmp3F9->hd=tv,_tmp3F9->tl=0;_tmp3F9;});}default: _LL5: _LL6:
 return 0;}_LL0:;}
# 2183
static void Cyc_Tcutil_check_subeffect(void*t1,void*t2){
# 2185
if(({Cyc_Absyn_get_debug();})){
# 2187
({struct Cyc_String_pa_PrintArg_struct _tmp3FD=({struct Cyc_String_pa_PrintArg_struct _tmp6D3;_tmp6D3.tag=0U,({
struct _fat_ptr _tmp7C9=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t1);}));_tmp6D3.f1=_tmp7C9;});_tmp6D3;});struct Cyc_String_pa_PrintArg_struct _tmp3FE=({struct Cyc_String_pa_PrintArg_struct _tmp6D2;_tmp6D2.tag=0U,({struct _fat_ptr _tmp7CA=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t2);}));_tmp6D2.f1=_tmp7CA;});_tmp6D2;});void*_tmp3FB[2U];_tmp3FB[0]=& _tmp3FD,_tmp3FB[1]=& _tmp3FE;({struct Cyc___cycFILE*_tmp7CC=Cyc_stdout;struct _fat_ptr _tmp7CB=({const char*_tmp3FC="\ncheck_subeffect(%s,%s)\n";_tag_fat(_tmp3FC,sizeof(char),25U);});Cyc_fprintf(_tmp7CC,_tmp7CB,_tag_fat(_tmp3FB,sizeof(void*),2U));});});
# 2190
({Cyc_fflush(Cyc_stdout);});}}
# 2183
int Cyc_Tcutil_coerce_arg(struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Exp*e,void*t2,int*alias_coercion){
# 2198
void*t1=({Cyc_Tcutil_compress((void*)_check_null(e->topt));});
enum Cyc_Absyn_Coercion c;
int do_alias_coercion=0;
# 2202
if(({Cyc_Unify_unify(t1,t2);}))
return 1;
# 2202
if(({Cyc_Absyn_get_debug();})){
# 2206
({struct Cyc_String_pa_PrintArg_struct _tmp402=({struct Cyc_String_pa_PrintArg_struct _tmp6D5;_tmp6D5.tag=0U,({
struct _fat_ptr _tmp7CD=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t1);}));_tmp6D5.f1=_tmp7CD;});_tmp6D5;});struct Cyc_String_pa_PrintArg_struct _tmp403=({struct Cyc_String_pa_PrintArg_struct _tmp6D4;_tmp6D4.tag=0U,({struct _fat_ptr _tmp7CE=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t2);}));_tmp6D4.f1=_tmp7CE;});_tmp6D4;});void*_tmp400[2U];_tmp400[0]=& _tmp402,_tmp400[1]=& _tmp403;({struct Cyc___cycFILE*_tmp7D0=Cyc_stdout;struct _fat_ptr _tmp7CF=({const char*_tmp401="\ncoerce_arg: unify(%s,%s) failed. Checking subtypes...";_tag_fat(_tmp401,sizeof(char),55U);});Cyc_fprintf(_tmp7D0,_tmp7CF,_tag_fat(_tmp400,sizeof(void*),2U));});});
# 2209
({Cyc_fflush(Cyc_stdout);});}
# 2202
if(
# 2212
({Cyc_Tcutil_is_arithmetic_type(t2);})&&({Cyc_Tcutil_is_arithmetic_type(t1);})){
# 2214
if(({Cyc_Tcutil_will_lose_precision(t1,t2);}))
({struct Cyc_String_pa_PrintArg_struct _tmp406=({struct Cyc_String_pa_PrintArg_struct _tmp6D7;_tmp6D7.tag=0U,({
struct _fat_ptr _tmp7D1=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t1);}));_tmp6D7.f1=_tmp7D1;});_tmp6D7;});struct Cyc_String_pa_PrintArg_struct _tmp407=({struct Cyc_String_pa_PrintArg_struct _tmp6D6;_tmp6D6.tag=0U,({struct _fat_ptr _tmp7D2=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t2);}));_tmp6D6.f1=_tmp7D2;});_tmp6D6;});void*_tmp404[2U];_tmp404[0]=& _tmp406,_tmp404[1]=& _tmp407;({unsigned _tmp7D4=e->loc;struct _fat_ptr _tmp7D3=({const char*_tmp405="integral size mismatch; %s -> %s conversion supplied";_tag_fat(_tmp405,sizeof(char),53U);});Cyc_Tcutil_warn(_tmp7D4,_tmp7D3,_tag_fat(_tmp404,sizeof(void*),2U));});});
# 2214
({Cyc_Tcutil_unchecked_cast(e,t2,Cyc_Absyn_No_coercion);});
# 2218
return 1;}
# 2202
if(({Cyc_Tcutil_can_insert_alias(e,t1,t2,e->loc);})){
# 2223
if(Cyc_Tcutil_warn_alias_coerce)
({struct Cyc_String_pa_PrintArg_struct _tmp40A=({struct Cyc_String_pa_PrintArg_struct _tmp6DA;_tmp6DA.tag=0U,({
struct _fat_ptr _tmp7D5=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_exp2string(e);}));_tmp6DA.f1=_tmp7D5;});_tmp6DA;});struct Cyc_String_pa_PrintArg_struct _tmp40B=({struct Cyc_String_pa_PrintArg_struct _tmp6D9;_tmp6D9.tag=0U,({struct _fat_ptr _tmp7D6=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t1);}));_tmp6D9.f1=_tmp7D6;});_tmp6D9;});struct Cyc_String_pa_PrintArg_struct _tmp40C=({struct Cyc_String_pa_PrintArg_struct _tmp6D8;_tmp6D8.tag=0U,({struct _fat_ptr _tmp7D7=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t2);}));_tmp6D8.f1=_tmp7D7;});_tmp6D8;});void*_tmp408[3U];_tmp408[0]=& _tmp40A,_tmp408[1]=& _tmp40B,_tmp408[2]=& _tmp40C;({unsigned _tmp7D9=e->loc;struct _fat_ptr _tmp7D8=({const char*_tmp409="implicit alias coercion for %s:%s to %s";_tag_fat(_tmp409,sizeof(char),40U);});Cyc_Tcutil_warn(_tmp7D9,_tmp7D8,_tag_fat(_tmp408,sizeof(void*),3U));});});
# 2223
*alias_coercion=1;}
# 2202
if(({Cyc_Tcutil_silent_castable(te,e->loc,t1,t2);})){
# 2230
({Cyc_Tcutil_unchecked_cast(e,t2,Cyc_Absyn_Other_coercion);});
return 1;}
# 2202
if(({Cyc_Tcutil_zero_to_null(t2,e);}))
# 2234
return 1;
# 2202
if((int)(c=({Cyc_Tcutil_castable(te,e->loc,t1,t2);}))!= (int)Cyc_Absyn_Unknown_coercion){
# 2237
if((int)c != (int)1U)
({Cyc_Tcutil_unchecked_cast(e,t2,c);});
# 2237
if((int)c != (int)2U)
# 2240
({struct Cyc_String_pa_PrintArg_struct _tmp40F=({struct Cyc_String_pa_PrintArg_struct _tmp6DC;_tmp6DC.tag=0U,({struct _fat_ptr _tmp7DA=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t1);}));_tmp6DC.f1=_tmp7DA;});_tmp6DC;});struct Cyc_String_pa_PrintArg_struct _tmp410=({struct Cyc_String_pa_PrintArg_struct _tmp6DB;_tmp6DB.tag=0U,({struct _fat_ptr _tmp7DB=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t2);}));_tmp6DB.f1=_tmp7DB;});_tmp6DB;});void*_tmp40D[2U];_tmp40D[0]=& _tmp40F,_tmp40D[1]=& _tmp410;({unsigned _tmp7DD=e->loc;struct _fat_ptr _tmp7DC=({const char*_tmp40E="implicit cast from %s to %s.";_tag_fat(_tmp40E,sizeof(char),29U);});Cyc_Tcutil_warn(_tmp7DD,_tmp7DC,_tag_fat(_tmp40D,sizeof(void*),2U));});});
# 2237
return 1;}
# 2202
return 0;}
# 2183
int Cyc_Tcutil_coerce_assign(struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Exp*e,void*t){
# 2253
int bogus=0;
return({Cyc_Tcutil_coerce_arg(te,e,t,& bogus);});}
# 2183
static struct Cyc_List_List*Cyc_Tcutil_flatten_type(struct _RegionHandle*r,int flatten,void*t1);struct _tuple24{struct Cyc_List_List*f1;struct _RegionHandle*f2;int f3;};
# 2271 "tcutil.cyc"
static struct Cyc_List_List*Cyc_Tcutil_flatten_type_f(struct _tuple24*env,struct Cyc_Absyn_Aggrfield*x){
# 2273
struct _tuple24 _stmttmp55=*env;int _tmp415;struct _RegionHandle*_tmp414;struct Cyc_List_List*_tmp413;_LL1: _tmp413=_stmttmp55.f1;_tmp414=_stmttmp55.f2;_tmp415=_stmttmp55.f3;_LL2: {struct Cyc_List_List*inst=_tmp413;struct _RegionHandle*r=_tmp414;int flatten=_tmp415;
void*t=inst == 0?x->type:({Cyc_Tcutil_rsubstitute(r,inst,x->type);});
struct Cyc_List_List*ts=({Cyc_Tcutil_flatten_type(r,flatten,t);});
if(({Cyc_List_length(ts);})== 1)
return({struct Cyc_List_List*_tmp417=_region_malloc(r,sizeof(*_tmp417));({struct _tuple12*_tmp7DE=({struct _tuple12*_tmp416=_region_malloc(r,sizeof(*_tmp416));_tmp416->f1=x->tq,_tmp416->f2=t;_tmp416;});_tmp417->hd=_tmp7DE;}),_tmp417->tl=0;_tmp417;});
# 2276
return ts;}}struct _tuple25{struct _RegionHandle*f1;int f2;};
# 2281
static struct Cyc_List_List*Cyc_Tcutil_rcopy_tqt(struct _tuple25*env,struct _tuple12*x){
struct _tuple25 _stmttmp56=*env;int _tmp41A;struct _RegionHandle*_tmp419;_LL1: _tmp419=_stmttmp56.f1;_tmp41A=_stmttmp56.f2;_LL2: {struct _RegionHandle*r=_tmp419;int flatten=_tmp41A;
struct _tuple12 _stmttmp57=*x;void*_tmp41C;struct Cyc_Absyn_Tqual _tmp41B;_LL4: _tmp41B=_stmttmp57.f1;_tmp41C=_stmttmp57.f2;_LL5: {struct Cyc_Absyn_Tqual tq=_tmp41B;void*t=_tmp41C;
struct Cyc_List_List*ts=({Cyc_Tcutil_flatten_type(r,flatten,t);});
if(({Cyc_List_length(ts);})== 1)
return({struct Cyc_List_List*_tmp41E=_region_malloc(r,sizeof(*_tmp41E));({struct _tuple12*_tmp7DF=({struct _tuple12*_tmp41D=_region_malloc(r,sizeof(*_tmp41D));_tmp41D->f1=tq,_tmp41D->f2=t;_tmp41D;});_tmp41E->hd=_tmp7DF;}),_tmp41E->tl=0;_tmp41E;});
# 2285
return ts;}}}
# 2289
static struct Cyc_List_List*Cyc_Tcutil_flatten_type(struct _RegionHandle*r,int flatten,void*t1){
# 2292
if(flatten){
t1=({Cyc_Tcutil_compress(t1);});{
void*_tmp420=t1;struct Cyc_List_List*_tmp421;struct Cyc_List_List*_tmp422;struct Cyc_List_List*_tmp424;struct Cyc_Absyn_Aggrdecl*_tmp423;switch(*((int*)_tmp420)){case 0U: switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp420)->f1)){case 0U: _LL1: _LL2:
 return 0;case 22U: if(((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp420)->f1)->f1).KnownAggr).tag == 2){_LL5: _tmp423=*((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp420)->f1)->f1).KnownAggr).val;_tmp424=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp420)->f2;_LL6: {struct Cyc_Absyn_Aggrdecl*ad=_tmp423;struct Cyc_List_List*ts=_tmp424;
# 2310
if((((int)ad->kind == (int)Cyc_Absyn_UnionA || ad->impl == 0)||((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->exist_vars != 0)||((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->rgn_po != 0)
# 2312
return({struct Cyc_List_List*_tmp42C=_region_malloc(r,sizeof(*_tmp42C));({struct _tuple12*_tmp7E1=({struct _tuple12*_tmp42B=_region_malloc(r,sizeof(*_tmp42B));({struct Cyc_Absyn_Tqual _tmp7E0=({Cyc_Absyn_empty_tqual(0U);});_tmp42B->f1=_tmp7E0;}),_tmp42B->f2=t1;_tmp42B;});_tmp42C->hd=_tmp7E1;}),_tmp42C->tl=0;_tmp42C;});{
# 2310
struct Cyc_List_List*inst=({Cyc_List_rzip(r,r,ad->tvs,ts);});
# 2314
struct _tuple24 env=({struct _tuple24 _tmp6DD;_tmp6DD.f1=inst,_tmp6DD.f2=r,_tmp6DD.f3=flatten;_tmp6DD;});
struct Cyc_List_List*_stmttmp58=((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->fields;struct Cyc_List_List*_tmp42D=_stmttmp58;struct Cyc_List_List*_tmp42F;struct Cyc_Absyn_Aggrfield*_tmp42E;if(_tmp42D == 0){_LL11: _LL12:
 return 0;}else{_LL13: _tmp42E=(struct Cyc_Absyn_Aggrfield*)_tmp42D->hd;_tmp42F=_tmp42D->tl;_LL14: {struct Cyc_Absyn_Aggrfield*hd=_tmp42E;struct Cyc_List_List*tl=_tmp42F;
# 2318
struct Cyc_List_List*hd2=({Cyc_Tcutil_flatten_type_f(& env,hd);});
env.f3=0;{
struct Cyc_List_List*tl2=({({struct Cyc_List_List*(*_tmp7E3)(struct _RegionHandle*,struct Cyc_List_List*(*f)(struct _tuple24*,struct Cyc_Absyn_Aggrfield*),struct _tuple24*env,struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp431)(struct _RegionHandle*,struct Cyc_List_List*(*f)(struct _tuple24*,struct Cyc_Absyn_Aggrfield*),struct _tuple24*env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct _RegionHandle*,struct Cyc_List_List*(*f)(struct _tuple24*,struct Cyc_Absyn_Aggrfield*),struct _tuple24*env,struct Cyc_List_List*x))Cyc_List_rmap_c;_tmp431;});struct _RegionHandle*_tmp7E2=r;_tmp7E3(_tmp7E2,Cyc_Tcutil_flatten_type_f,& env,tl);});});
struct Cyc_List_List*tts=({struct Cyc_List_List*_tmp430=_region_malloc(r,sizeof(*_tmp430));_tmp430->hd=hd2,_tmp430->tl=tl2;_tmp430;});
return({Cyc_List_rflatten(r,tts);});}}}_LL10:;}}}else{goto _LL9;}default: goto _LL9;}case 6U: _LL3: _tmp422=((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp420)->f1;_LL4: {struct Cyc_List_List*tqs=_tmp422;
# 2297
struct _tuple25 env=({struct _tuple25 _tmp6DE;_tmp6DE.f1=r,_tmp6DE.f2=flatten;_tmp6DE;});
# 2299
struct Cyc_List_List*_tmp425=tqs;struct Cyc_List_List*_tmp427;struct _tuple12*_tmp426;if(_tmp425 == 0){_LLC: _LLD:
 return 0;}else{_LLE: _tmp426=(struct _tuple12*)_tmp425->hd;_tmp427=_tmp425->tl;_LLF: {struct _tuple12*hd=_tmp426;struct Cyc_List_List*tl=_tmp427;
# 2302
struct Cyc_List_List*hd2=({Cyc_Tcutil_rcopy_tqt(& env,hd);});
env.f2=0;{
struct Cyc_List_List*tl2=({({struct Cyc_List_List*(*_tmp7E5)(struct _RegionHandle*,struct Cyc_List_List*(*f)(struct _tuple25*,struct _tuple12*),struct _tuple25*env,struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp42A)(struct _RegionHandle*,struct Cyc_List_List*(*f)(struct _tuple25*,struct _tuple12*),struct _tuple25*env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct _RegionHandle*,struct Cyc_List_List*(*f)(struct _tuple25*,struct _tuple12*),struct _tuple25*env,struct Cyc_List_List*x))Cyc_List_rmap_c;_tmp42A;});struct _RegionHandle*_tmp7E4=r;_tmp7E5(_tmp7E4,Cyc_Tcutil_rcopy_tqt,& env,tqs);});});
struct Cyc_List_List*temp=({struct Cyc_List_List*_tmp429=_region_malloc(r,sizeof(*_tmp429));_tmp429->hd=hd2,_tmp429->tl=tl2;_tmp429;});
return({({struct _RegionHandle*_tmp7E6=r;Cyc_List_rflatten(_tmp7E6,({struct Cyc_List_List*_tmp428=_region_malloc(r,sizeof(*_tmp428));_tmp428->hd=hd2,_tmp428->tl=tl2;_tmp428;}));});});}}}_LLB:;}case 7U: if(((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp420)->f1 == Cyc_Absyn_StructA){_LL7: _tmp421=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp420)->f2;_LL8: {struct Cyc_List_List*fs=_tmp421;
# 2325
struct _tuple24 env=({struct _tuple24 _tmp6DF;_tmp6DF.f1=0,_tmp6DF.f2=r,_tmp6DF.f3=flatten;_tmp6DF;});
struct Cyc_List_List*_tmp432=fs;struct Cyc_List_List*_tmp434;struct Cyc_Absyn_Aggrfield*_tmp433;if(_tmp432 == 0){_LL16: _LL17:
 return 0;}else{_LL18: _tmp433=(struct Cyc_Absyn_Aggrfield*)_tmp432->hd;_tmp434=_tmp432->tl;_LL19: {struct Cyc_Absyn_Aggrfield*hd=_tmp433;struct Cyc_List_List*tl=_tmp434;
# 2329
struct Cyc_List_List*hd2=({Cyc_Tcutil_flatten_type_f(& env,hd);});
env.f3=0;{
struct Cyc_List_List*tl2=({({struct Cyc_List_List*(*_tmp7E8)(struct _RegionHandle*,struct Cyc_List_List*(*f)(struct _tuple24*,struct Cyc_Absyn_Aggrfield*),struct _tuple24*env,struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp436)(struct _RegionHandle*,struct Cyc_List_List*(*f)(struct _tuple24*,struct Cyc_Absyn_Aggrfield*),struct _tuple24*env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct _RegionHandle*,struct Cyc_List_List*(*f)(struct _tuple24*,struct Cyc_Absyn_Aggrfield*),struct _tuple24*env,struct Cyc_List_List*x))Cyc_List_rmap_c;_tmp436;});struct _RegionHandle*_tmp7E7=r;_tmp7E8(_tmp7E7,Cyc_Tcutil_flatten_type_f,& env,tl);});});
struct Cyc_List_List*tts=({struct Cyc_List_List*_tmp435=_region_malloc(r,sizeof(*_tmp435));_tmp435->hd=hd2,_tmp435->tl=tl2;_tmp435;});
return({Cyc_List_rflatten(r,tts);});}}}_LL15:;}}else{goto _LL9;}default: _LL9: _LLA:
# 2335
 goto _LL0;}_LL0:;}}
# 2292
return({struct Cyc_List_List*_tmp438=_region_malloc(r,sizeof(*_tmp438));
# 2338
({struct _tuple12*_tmp7EA=({struct _tuple12*_tmp437=_region_malloc(r,sizeof(*_tmp437));({struct Cyc_Absyn_Tqual _tmp7E9=({Cyc_Absyn_empty_tqual(0U);});_tmp437->f1=_tmp7E9;}),_tmp437->f2=t1;_tmp437;});_tmp438->hd=_tmp7EA;}),_tmp438->tl=0;_tmp438;});}
# 2342
static int Cyc_Tcutil_sub_attributes(struct Cyc_List_List*a1,struct Cyc_List_List*a2){
{struct Cyc_List_List*t=a1;for(0;t != 0;t=t->tl){
void*_stmttmp59=(void*)t->hd;void*_tmp43A=_stmttmp59;switch(*((int*)_tmp43A)){case 23U: _LL1: _LL2:
 goto _LL4;case 4U: _LL3: _LL4:
 goto _LL6;case 20U: _LL5: _LL6:
 continue;default: _LL7: _LL8:
# 2349
 if(!({Cyc_List_exists_c(Cyc_Absyn_equal_att,(void*)t->hd,a2);}))return 0;}_LL0:;}}
# 2352
for(0;a2 != 0;a2=a2->tl){
if(!({Cyc_List_exists_c(Cyc_Absyn_equal_att,(void*)a2->hd,a1);}))return 0;}
# 2355
return 1;}
# 2358
static int Cyc_Tcutil_isomorphic(void*t1,void*t2){
struct _tuple15 _stmttmp5A=({struct _tuple15 _tmp6E0;({void*_tmp7EC=({Cyc_Tcutil_compress(t1);});_tmp6E0.f1=_tmp7EC;}),({void*_tmp7EB=({Cyc_Tcutil_compress(t2);});_tmp6E0.f2=_tmp7EB;});_tmp6E0;});struct _tuple15 _tmp43C=_stmttmp5A;enum Cyc_Absyn_Size_of _tmp43E;enum Cyc_Absyn_Size_of _tmp43D;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp43C.f1)->tag == 0U){if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp43C.f1)->f1)->tag == 1U){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp43C.f2)->tag == 0U){if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp43C.f2)->f1)->tag == 1U){_LL1: _tmp43D=((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp43C.f1)->f1)->f2;_tmp43E=((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp43C.f2)->f1)->f2;_LL2: {enum Cyc_Absyn_Size_of b1=_tmp43D;enum Cyc_Absyn_Size_of b2=_tmp43E;
# 2361
return((int)b1 == (int)b2 ||(int)b1 == (int)2U &&(int)b2 == (int)3U)||
(int)b1 == (int)3U &&(int)b2 == (int)Cyc_Absyn_Int_sz;}}else{goto _LL3;}}else{goto _LL3;}}else{goto _LL3;}}else{_LL3: _LL4:
 return 0;}_LL0:;}
# 2358
int Cyc_Tcutil_subtype(struct Cyc_Tcenv_Tenv*te,struct Cyc_List_List*assume,void*t1,void*t2){
# 2371
if(({Cyc_Unify_unify(t1,t2);}))return 1;{
struct Cyc_List_List*a=assume;
# 2371
for(0;a != 0;a=a->tl){
# 2373
if(({Cyc_Unify_unify(t1,(*((struct _tuple15*)a->hd)).f1);})&&({Cyc_Unify_unify(t2,(*((struct _tuple15*)a->hd)).f2);}))
return 1;}}
# 2371
t1=({Cyc_Tcutil_compress(t1);});
# 2376
t2=({Cyc_Tcutil_compress(t2);});{
struct _tuple15 _stmttmp5B=({struct _tuple15 _tmp6E1;_tmp6E1.f1=t1,_tmp6E1.f2=t2;_tmp6E1;});struct _tuple15 _tmp440=_stmttmp5B;struct Cyc_Absyn_FnInfo _tmp442;struct Cyc_Absyn_FnInfo _tmp441;struct Cyc_List_List*_tmp447;struct Cyc_Absyn_Datatypedecl*_tmp446;struct Cyc_List_List*_tmp445;struct Cyc_Absyn_Datatypefield*_tmp444;struct Cyc_Absyn_Datatypedecl*_tmp443;void*_tmp453;void*_tmp452;void*_tmp451;void*_tmp450;struct Cyc_Absyn_Tqual _tmp44F;void*_tmp44E;void*_tmp44D;void*_tmp44C;void*_tmp44B;void*_tmp44A;struct Cyc_Absyn_Tqual _tmp449;void*_tmp448;switch(*((int*)_tmp440.f1)){case 3U: if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp440.f2)->tag == 3U){_LL1: _tmp448=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp440.f1)->f1).elt_type;_tmp449=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp440.f1)->f1).elt_tq;_tmp44A=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp440.f1)->f1).ptr_atts).rgn;_tmp44B=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp440.f1)->f1).ptr_atts).nullable;_tmp44C=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp440.f1)->f1).ptr_atts).bounds;_tmp44D=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp440.f1)->f1).ptr_atts).zero_term;_tmp44E=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp440.f2)->f1).elt_type;_tmp44F=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp440.f2)->f1).elt_tq;_tmp450=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp440.f2)->f1).ptr_atts).rgn;_tmp451=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp440.f2)->f1).ptr_atts).nullable;_tmp452=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp440.f2)->f1).ptr_atts).bounds;_tmp453=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp440.f2)->f1).ptr_atts).zero_term;_LL2: {void*t_a=_tmp448;struct Cyc_Absyn_Tqual q_a=_tmp449;void*rt_a=_tmp44A;void*null_a=_tmp44B;void*b_a=_tmp44C;void*zt_a=_tmp44D;void*t_b=_tmp44E;struct Cyc_Absyn_Tqual q_b=_tmp44F;void*rt_b=_tmp450;void*null_b=_tmp451;void*b_b=_tmp452;void*zt_b=_tmp453;
# 2383
if(q_a.real_const && !q_b.real_const)
return 0;
# 2383
if(
# 2386
(!({Cyc_Unify_unify(null_a,null_b);})&&({Cyc_Absyn_type2bool(0,null_a);}))&& !({Cyc_Absyn_type2bool(0,null_b);}))
# 2388
return 0;
# 2383
if(
# 2390
(({Cyc_Unify_unify(zt_a,zt_b);})&& !({Cyc_Absyn_type2bool(0,zt_a);}))&&({Cyc_Absyn_type2bool(0,zt_b);}))
# 2392
return 0;
# 2383
if(
# 2394
(!({Cyc_Unify_unify(rt_a,rt_b);})&& !({Cyc_Tcenv_rgn_outlives_rgn(te,rt_a,rt_b);}))&& !({Cyc_Tcutil_subtype(te,assume,rt_a,rt_b);}))
# 2396
return 0;
# 2383
if(!({Cyc_Unify_unify(b_a,b_b);})){
# 2399
struct Cyc_Absyn_Exp*e1=({struct Cyc_Absyn_Exp*(*_tmp457)(void*def,void*b)=Cyc_Tcutil_get_bounds_exp;void*_tmp458=({Cyc_Absyn_bounds_one();});void*_tmp459=b_a;_tmp457(_tmp458,_tmp459);});
struct Cyc_Absyn_Exp*e2=({struct Cyc_Absyn_Exp*(*_tmp454)(void*def,void*b)=Cyc_Tcutil_get_bounds_exp;void*_tmp455=({Cyc_Absyn_bounds_one();});void*_tmp456=b_b;_tmp454(_tmp455,_tmp456);});
if(e1 != e2){
if((e1 == 0 || e2 == 0)|| !({Cyc_Evexp_lte_const_exp(e2,e2);}))
return 0;}}
# 2383
if(
# 2408
!q_b.real_const && q_a.real_const){
if(!({int(*_tmp45A)(struct Cyc_Absyn_Kind*ka1,struct Cyc_Absyn_Kind*ka2)=Cyc_Tcutil_kind_leq;struct Cyc_Absyn_Kind*_tmp45B=& Cyc_Tcutil_ak;struct Cyc_Absyn_Kind*_tmp45C=({Cyc_Tcutil_type_kind(t_b);});_tmp45A(_tmp45B,_tmp45C);}))
return 0;}{
# 2383
int deep_subtype=
# 2415
({int(*_tmp45F)(void*,void*)=Cyc_Unify_unify;void*_tmp460=b_b;void*_tmp461=({Cyc_Absyn_bounds_one();});_tmp45F(_tmp460,_tmp461);})&& !({Cyc_Tcutil_force_type2bool(0,zt_b);});
# 2419
return(deep_subtype &&({({struct Cyc_Tcenv_Tenv*_tmp7F0=te;struct Cyc_List_List*_tmp7EF=({struct Cyc_List_List*_tmp45E=_cycalloc(sizeof(*_tmp45E));({struct _tuple15*_tmp7ED=({struct _tuple15*_tmp45D=_cycalloc(sizeof(*_tmp45D));_tmp45D->f1=t1,_tmp45D->f2=t2;_tmp45D;});_tmp45E->hd=_tmp7ED;}),_tmp45E->tl=assume;_tmp45E;});void*_tmp7EE=t_a;Cyc_Tcutil_ptrsubtype(_tmp7F0,_tmp7EF,_tmp7EE,t_b);});})||({Cyc_Unify_unify(t_a,t_b);}))||({Cyc_Tcutil_isomorphic(t_a,t_b);});}}}else{goto _LL7;}case 0U: if(((struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp440.f1)->f1)->tag == 21U){if(((((struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp440.f1)->f1)->f1).KnownDatatypefield).tag == 2){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp440.f2)->tag == 0U){if(((struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp440.f2)->f1)->tag == 20U){if(((((struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp440.f2)->f1)->f1).KnownDatatype).tag == 2){_LL3: _tmp443=(((((struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp440.f1)->f1)->f1).KnownDatatypefield).val).f1;_tmp444=(((((struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp440.f1)->f1)->f1).KnownDatatypefield).val).f2;_tmp445=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp440.f1)->f2;_tmp446=*((((struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp440.f2)->f1)->f1).KnownDatatype).val;_tmp447=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp440.f2)->f2;_LL4: {struct Cyc_Absyn_Datatypedecl*dd1=_tmp443;struct Cyc_Absyn_Datatypefield*df=_tmp444;struct Cyc_List_List*ts1=_tmp445;struct Cyc_Absyn_Datatypedecl*dd2=_tmp446;struct Cyc_List_List*ts2=_tmp447;
# 2426
if(dd1 != dd2 &&({Cyc_Absyn_qvar_cmp(dd1->name,dd2->name);})!= 0)return 0;if(({
# 2428
int _tmp7F1=({Cyc_List_length(ts1);});_tmp7F1 != ({Cyc_List_length(ts2);});}))return 0;
# 2426
for(0;ts1 != 0;(
# 2429
ts1=ts1->tl,ts2=ts2->tl)){
if(!({Cyc_Unify_unify((void*)ts1->hd,(void*)((struct Cyc_List_List*)_check_null(ts2))->hd);}))return 0;}
# 2426
return 1;}}else{goto _LL7;}}else{goto _LL7;}}else{goto _LL7;}}else{goto _LL7;}}else{goto _LL7;}case 5U: if(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp440.f2)->tag == 5U){_LL5: _tmp441=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp440.f1)->f1;_tmp442=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp440.f2)->f1;_LL6: {struct Cyc_Absyn_FnInfo f1=_tmp441;struct Cyc_Absyn_FnInfo f2=_tmp442;
# 2435
if(f1.tvars != 0 || f2.tvars != 0){
struct Cyc_List_List*tvs1=f1.tvars;
struct Cyc_List_List*tvs2=f2.tvars;
if(({int _tmp7F2=({Cyc_List_length(tvs1);});_tmp7F2 != ({Cyc_List_length(tvs2);});}))return 0;{struct Cyc_List_List*inst=0;
# 2440
while(tvs1 != 0){
if(!({Cyc_Unify_unify_kindbound(((struct Cyc_Absyn_Tvar*)tvs1->hd)->kind,((struct Cyc_Absyn_Tvar*)((struct Cyc_List_List*)_check_null(tvs2))->hd)->kind);}))return 0;inst=({struct Cyc_List_List*_tmp463=_cycalloc(sizeof(*_tmp463));
({struct _tuple19*_tmp7F4=({struct _tuple19*_tmp462=_cycalloc(sizeof(*_tmp462));_tmp462->f1=(struct Cyc_Absyn_Tvar*)tvs2->hd,({void*_tmp7F3=({Cyc_Absyn_var_type((struct Cyc_Absyn_Tvar*)tvs1->hd);});_tmp462->f2=_tmp7F3;});_tmp462;});_tmp463->hd=_tmp7F4;}),_tmp463->tl=inst;_tmp463;});
tvs1=tvs1->tl;
tvs2=tvs2->tl;}
# 2446
if(inst != 0){
f1.tvars=0;
f2.tvars=0;
return({({struct Cyc_Tcenv_Tenv*_tmp7F7=te;struct Cyc_List_List*_tmp7F6=assume;void*_tmp7F5=(void*)({struct Cyc_Absyn_FnType_Absyn_Type_struct*_tmp464=_cycalloc(sizeof(*_tmp464));_tmp464->tag=5U,_tmp464->f1=f1;_tmp464;});Cyc_Tcutil_subtype(_tmp7F7,_tmp7F6,_tmp7F5,(void*)({struct Cyc_Absyn_FnType_Absyn_Type_struct*_tmp465=_cycalloc(sizeof(*_tmp465));_tmp465->tag=5U,_tmp465->f1=f2;_tmp465;}));});});}}}
# 2435
if(!({Cyc_Tcutil_subtype(te,assume,f1.ret_type,f2.ret_type);}))
# 2453
return 0;{
# 2435
struct Cyc_List_List*args1=f1.args;
# 2455
struct Cyc_List_List*args2=f2.args;
# 2458
if(({int _tmp7F8=({Cyc_List_length(args1);});_tmp7F8 != ({Cyc_List_length(args2);});}))return 0;for(0;args1 != 0;(
# 2460
args1=args1->tl,args2=args2->tl)){
struct _tuple9 _stmttmp5C=*((struct _tuple9*)args1->hd);void*_tmp467;struct Cyc_Absyn_Tqual _tmp466;_LLA: _tmp466=_stmttmp5C.f2;_tmp467=_stmttmp5C.f3;_LLB: {struct Cyc_Absyn_Tqual tq1=_tmp466;void*t1=_tmp467;
struct _tuple9 _stmttmp5D=*((struct _tuple9*)((struct Cyc_List_List*)_check_null(args2))->hd);void*_tmp469;struct Cyc_Absyn_Tqual _tmp468;_LLD: _tmp468=_stmttmp5D.f2;_tmp469=_stmttmp5D.f3;_LLE: {struct Cyc_Absyn_Tqual tq2=_tmp468;void*t2=_tmp469;
# 2464
if(tq2.real_const && !tq1.real_const || !({Cyc_Tcutil_subtype(te,assume,t2,t1);}))
return 0;}}}
# 2468
if(f1.c_varargs != f2.c_varargs)return 0;if(
f1.cyc_varargs != 0 && f2.cyc_varargs != 0){
struct Cyc_Absyn_VarargInfo v1=*f1.cyc_varargs;
struct Cyc_Absyn_VarargInfo v2=*f2.cyc_varargs;
# 2473
if((v2.tq).real_const && !(v1.tq).real_const || !({Cyc_Tcutil_subtype(te,assume,v2.type,v1.type);}))
# 2475
return 0;}else{
if(f1.cyc_varargs != 0 || f2.cyc_varargs != 0)return 0;}
# 2468
if(!({({void*_tmp7F9=(void*)_check_null(f1.effect);Cyc_Tcutil_subset_effect(1,_tmp7F9,(void*)_check_null(f2.effect));});}))
# 2478
return 0;
# 2468
if(!({Cyc_Tcutil_sub_rgnpo(f1.rgn_po,f2.rgn_po);}))
# 2480
return 0;
# 2468
if(!({Cyc_Tcutil_sub_attributes(f1.attributes,f2.attributes);}))
# 2482
return 0;
# 2468
if(!({Cyc_Relations_check_logical_implication(f2.requires_relns,f1.requires_relns);}))
# 2486
return 0;
# 2468
if(!({Cyc_Relations_check_logical_implication(f1.ensures_relns,f2.ensures_relns);}))
# 2490
return 0;
# 2468
return 1;}}}else{goto _LL7;}default: _LL7: _LL8:
# 2493
 return 0;}_LL0:;}}
# 2503
static int Cyc_Tcutil_ptrsubtype(struct Cyc_Tcenv_Tenv*te,struct Cyc_List_List*assume,void*t1,void*t2){
# 2507
struct Cyc_List_List*tqs1=({Cyc_Tcutil_flatten_type(Cyc_Core_heap_region,1,t1);});
struct Cyc_List_List*tqs2=({Cyc_Tcutil_flatten_type(Cyc_Core_heap_region,1,t2);});
for(0;tqs2 != 0;(tqs2=tqs2->tl,tqs1=tqs1->tl)){
if(tqs1 == 0)return 0;{struct _tuple12*_stmttmp5E=(struct _tuple12*)tqs1->hd;void*_tmp46C;struct Cyc_Absyn_Tqual _tmp46B;_LL1: _tmp46B=_stmttmp5E->f1;_tmp46C=_stmttmp5E->f2;_LL2: {struct Cyc_Absyn_Tqual tq1=_tmp46B;void*t1a=_tmp46C;
# 2512
struct _tuple12*_stmttmp5F=(struct _tuple12*)tqs2->hd;void*_tmp46E;struct Cyc_Absyn_Tqual _tmp46D;_LL4: _tmp46D=_stmttmp5F->f1;_tmp46E=_stmttmp5F->f2;_LL5: {struct Cyc_Absyn_Tqual tq2=_tmp46D;void*t2a=_tmp46E;
if(tq1.real_const && !tq2.real_const)return 0;if(
(tq2.real_const ||({int(*_tmp46F)(struct Cyc_Absyn_Kind*ka1,struct Cyc_Absyn_Kind*ka2)=Cyc_Tcutil_kind_leq;struct Cyc_Absyn_Kind*_tmp470=& Cyc_Tcutil_ak;struct Cyc_Absyn_Kind*_tmp471=({Cyc_Tcutil_type_kind(t2a);});_tmp46F(_tmp470,_tmp471);}))&&({Cyc_Tcutil_subtype(te,assume,t1a,t2a);}))
# 2516
continue;
# 2513
if(({Cyc_Unify_unify(t1a,t2a);}))
# 2518
continue;
# 2513
if(({Cyc_Tcutil_isomorphic(t1a,t2a);}))
# 2520
continue;
# 2513
return 0;}}}}
# 2523
return 1;}
# 2503
enum Cyc_Absyn_Coercion Cyc_Tcutil_castable(struct Cyc_Tcenv_Tenv*te,unsigned loc,void*t1,void*t2){
# 2529
if(({Cyc_Unify_unify(t1,t2);}))
return Cyc_Absyn_No_coercion;
# 2529
t1=({Cyc_Tcutil_compress(t1);});
# 2532
t2=({Cyc_Tcutil_compress(t2);});
# 2534
{void*_tmp473=t2;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp473)->tag == 0U)switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp473)->f1)){case 0U: _LL1: _LL2:
 return Cyc_Absyn_No_coercion;case 1U: switch(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp473)->f1)->f2){case Cyc_Absyn_Int_sz: _LL3: _LL4:
# 2537
 goto _LL6;case Cyc_Absyn_Long_sz: _LL5: _LL6:
# 2539
 if((int)({Cyc_Tcutil_type_kind(t1);})->kind == (int)Cyc_Absyn_BoxKind)return Cyc_Absyn_Other_coercion;goto _LL0;default: goto _LL7;}default: goto _LL7;}else{_LL7: _LL8:
# 2541
 goto _LL0;}_LL0:;}{
# 2543
void*_tmp474=t1;void*_tmp475;struct Cyc_Absyn_Enumdecl*_tmp476;void*_tmp47A;struct Cyc_Absyn_Exp*_tmp479;struct Cyc_Absyn_Tqual _tmp478;void*_tmp477;void*_tmp480;void*_tmp47F;void*_tmp47E;void*_tmp47D;struct Cyc_Absyn_Tqual _tmp47C;void*_tmp47B;switch(*((int*)_tmp474)){case 3U: _LLA: _tmp47B=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp474)->f1).elt_type;_tmp47C=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp474)->f1).elt_tq;_tmp47D=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp474)->f1).ptr_atts).rgn;_tmp47E=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp474)->f1).ptr_atts).nullable;_tmp47F=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp474)->f1).ptr_atts).bounds;_tmp480=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp474)->f1).ptr_atts).zero_term;_LLB: {void*t_a=_tmp47B;struct Cyc_Absyn_Tqual q_a=_tmp47C;void*rt_a=_tmp47D;void*null_a=_tmp47E;void*b_a=_tmp47F;void*zt_a=_tmp480;
# 2552
{void*_tmp481=t2;void*_tmp487;void*_tmp486;void*_tmp485;void*_tmp484;struct Cyc_Absyn_Tqual _tmp483;void*_tmp482;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp481)->tag == 3U){_LL19: _tmp482=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp481)->f1).elt_type;_tmp483=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp481)->f1).elt_tq;_tmp484=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp481)->f1).ptr_atts).rgn;_tmp485=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp481)->f1).ptr_atts).nullable;_tmp486=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp481)->f1).ptr_atts).bounds;_tmp487=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp481)->f1).ptr_atts).zero_term;_LL1A: {void*t_b=_tmp482;struct Cyc_Absyn_Tqual q_b=_tmp483;void*rt_b=_tmp484;void*null_b=_tmp485;void*b_b=_tmp486;void*zt_b=_tmp487;
# 2554
enum Cyc_Absyn_Coercion coercion=3U;
struct Cyc_List_List*assump=({struct Cyc_List_List*_tmp492=_cycalloc(sizeof(*_tmp492));({struct _tuple15*_tmp7FA=({struct _tuple15*_tmp491=_cycalloc(sizeof(*_tmp491));_tmp491->f1=t1,_tmp491->f2=t2;_tmp491;});_tmp492->hd=_tmp7FA;}),_tmp492->tl=0;_tmp492;});
int quals_okay=q_b.real_const || !q_a.real_const;
# 2566 "tcutil.cyc"
int deep_castable=
({int(*_tmp48E)(void*,void*)=Cyc_Unify_unify;void*_tmp48F=b_b;void*_tmp490=({Cyc_Absyn_bounds_one();});_tmp48E(_tmp48F,_tmp490);})&& !({Cyc_Tcutil_force_type2bool(0,zt_b);});
# 2569
int ptrsub=quals_okay &&(
((deep_castable &&({Cyc_Tcutil_ptrsubtype(te,assump,t_a,t_b);})||({Cyc_Unify_unify(t_a,t_b);}))||({Cyc_Tcutil_isomorphic(t_a,t_b);}))||({Cyc_Unify_unify(t_b,Cyc_Absyn_void_type);}));
# 2572
int zeroterm_ok=({Cyc_Unify_unify(zt_a,zt_b);})|| !({Cyc_Absyn_type2bool(0,zt_b);});
# 2574
int bitcase=ptrsub?0:((({Cyc_Tcutil_is_bits_only_type(t_a);})&&({Cyc_Tcutil_is_char_type(t_b);}))&& !({Cyc_Tcutil_force_type2bool(0,zt_b);}))&&(
# 2576
q_b.real_const || !q_a.real_const);
int bounds_ok=({Cyc_Unify_unify(b_a,b_b);});
if(!bounds_ok && !bitcase){
struct Cyc_Absyn_Exp*e_a=({struct Cyc_Absyn_Exp*(*_tmp48B)(void*def,void*b)=Cyc_Tcutil_get_bounds_exp;void*_tmp48C=({Cyc_Absyn_bounds_one();});void*_tmp48D=b_a;_tmp48B(_tmp48C,_tmp48D);});
struct Cyc_Absyn_Exp*e_b=({struct Cyc_Absyn_Exp*(*_tmp488)(void*def,void*b)=Cyc_Tcutil_get_bounds_exp;void*_tmp489=({Cyc_Absyn_bounds_one();});void*_tmp48A=b_b;_tmp488(_tmp489,_tmp48A);});
if((e_a != 0 && e_b != 0)&&({Cyc_Evexp_lte_const_exp(e_b,e_a);}))
bounds_ok=1;else{
if(e_a == 0 || e_b == 0)
bounds_ok=1;}}{
# 2578
int t1_nullable=({Cyc_Tcutil_force_type2bool(0,null_a);});
# 2587
int t2_nullable=({Cyc_Tcutil_force_type2bool(0,null_b);});
if(t1_nullable && !t2_nullable)
coercion=2U;
# 2588
({Cyc_Tcutil_check_subeffect(rt_a,rt_b);});
# 2593
if(((bounds_ok && zeroterm_ok)&&(ptrsub || bitcase))&&(
({Cyc_Unify_unify(rt_a,rt_b);})||({Cyc_Tcenv_rgn_outlives_rgn(te,rt_a,rt_b);})))
# 2597
return coercion;else{
return Cyc_Absyn_Unknown_coercion;}}}}else{_LL1B: _LL1C:
 goto _LL18;}_LL18:;}
# 2601
return Cyc_Absyn_Unknown_coercion;}case 4U: _LLC: _tmp477=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp474)->f1).elt_type;_tmp478=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp474)->f1).tq;_tmp479=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp474)->f1).num_elts;_tmp47A=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp474)->f1).zero_term;_LLD: {void*t1a=_tmp477;struct Cyc_Absyn_Tqual tq1a=_tmp478;struct Cyc_Absyn_Exp*e1=_tmp479;void*zt1=_tmp47A;
# 2603
{void*_tmp493=t2;void*_tmp497;struct Cyc_Absyn_Exp*_tmp496;struct Cyc_Absyn_Tqual _tmp495;void*_tmp494;if(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp493)->tag == 4U){_LL1E: _tmp494=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp493)->f1).elt_type;_tmp495=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp493)->f1).tq;_tmp496=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp493)->f1).num_elts;_tmp497=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp493)->f1).zero_term;_LL1F: {void*t2a=_tmp494;struct Cyc_Absyn_Tqual tq2a=_tmp495;struct Cyc_Absyn_Exp*e2=_tmp496;void*zt2=_tmp497;
# 2605
int okay=
(((e1 != 0 && e2 != 0)&&({Cyc_Unify_unify(zt1,zt2);}))&&({Cyc_Evexp_lte_const_exp(e2,e1);}))&&({Cyc_Evexp_lte_const_exp(e1,e2);});
# 2608
return
# 2610
(okay &&({Cyc_Unify_unify(t1a,t2a);}))&&(!tq1a.real_const || tq2a.real_const)?Cyc_Absyn_No_coercion: Cyc_Absyn_Unknown_coercion;}}else{_LL20: _LL21:
# 2612
 return Cyc_Absyn_Unknown_coercion;}_LL1D:;}
# 2614
return Cyc_Absyn_Unknown_coercion;}case 0U: switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp474)->f1)){case 17U: _LLE: _tmp476=((struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp474)->f1)->f2;_LLF: {struct Cyc_Absyn_Enumdecl*ed1=_tmp476;
# 2618
{void*_tmp498=t2;struct Cyc_Absyn_Enumdecl*_tmp499;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp498)->tag == 0U){if(((struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp498)->f1)->tag == 17U){_LL23: _tmp499=((struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp498)->f1)->f2;_LL24: {struct Cyc_Absyn_Enumdecl*ed2=_tmp499;
# 2620
if((((struct Cyc_Absyn_Enumdecl*)_check_null(ed1))->fields != 0 &&((struct Cyc_Absyn_Enumdecl*)_check_null(ed2))->fields != 0)&&({
int _tmp7FB=({Cyc_List_length((struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(ed1->fields))->v);});_tmp7FB >= ({Cyc_List_length((struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(ed2->fields))->v);});}))
return Cyc_Absyn_Other_coercion;
# 2620
goto _LL22;}}else{goto _LL25;}}else{_LL25: _LL26:
# 2624
 goto _LL22;}_LL22:;}
# 2626
goto _LL11;}case 1U: _LL10: _LL11:
 goto _LL13;case 2U: _LL12: _LL13:
# 2629
 return({Cyc_Tcutil_is_strict_arithmetic_type(t2);})?Cyc_Absyn_Other_coercion: Cyc_Absyn_Unknown_coercion;case 3U: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp474)->f2 != 0){_LL14: _tmp475=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp474)->f2)->hd;_LL15: {void*r1=_tmp475;
# 2632
{void*_tmp49A=t2;void*_tmp49B;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp49A)->tag == 0U){if(((struct Cyc_Absyn_RgnHandleCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp49A)->f1)->tag == 3U){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp49A)->f2 != 0){_LL28: _tmp49B=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp49A)->f2)->hd;_LL29: {void*r2=_tmp49B;
# 2634
if(({Cyc_Tcenv_rgn_outlives_rgn(te,r1,r2);}))return Cyc_Absyn_No_coercion;goto _LL27;}}else{goto _LL2A;}}else{goto _LL2A;}}else{_LL2A: _LL2B:
# 2636
 goto _LL27;}_LL27:;}
# 2638
return Cyc_Absyn_Unknown_coercion;}}else{goto _LL16;}default: goto _LL16;}default: _LL16: _LL17:
 return Cyc_Absyn_Unknown_coercion;}_LL9:;}}
# 2644
void Cyc_Tcutil_unchecked_cast(struct Cyc_Absyn_Exp*e,void*t,enum Cyc_Absyn_Coercion c){
if(({Cyc_Unify_unify((void*)_check_null(e->topt),t);}))
return;{
# 2645
struct Cyc_Absyn_Exp*inner=({Cyc_Absyn_copy_exp(e);});
# 2648
({void*_tmp7FC=(void*)({struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*_tmp49D=_cycalloc(sizeof(*_tmp49D));_tmp49D->tag=14U,_tmp49D->f1=t,_tmp49D->f2=inner,_tmp49D->f3=0,_tmp49D->f4=c;_tmp49D;});e->r=_tmp7FC;});
e->topt=t;}}
# 2644
static int Cyc_Tcutil_tvar_id_counter=0;
# 2654
int Cyc_Tcutil_new_tvar_id(){
return Cyc_Tcutil_tvar_id_counter ++;}
# 2654
static int Cyc_Tcutil_tvar_counter=0;
# 2659
struct Cyc_Absyn_Tvar*Cyc_Tcutil_new_tvar(void*k){
int i=Cyc_Tcutil_tvar_counter ++;
struct _fat_ptr s=(struct _fat_ptr)({struct Cyc_Int_pa_PrintArg_struct _tmp4A5=({struct Cyc_Int_pa_PrintArg_struct _tmp6E2;_tmp6E2.tag=1U,_tmp6E2.f1=(unsigned long)i;_tmp6E2;});void*_tmp4A3[1U];_tmp4A3[0]=& _tmp4A5;({struct _fat_ptr _tmp7FD=({const char*_tmp4A4="#%d";_tag_fat(_tmp4A4,sizeof(char),4U);});Cyc_aprintf(_tmp7FD,_tag_fat(_tmp4A3,sizeof(void*),1U));});});
return({struct Cyc_Absyn_Tvar*_tmp4A2=_cycalloc(sizeof(*_tmp4A2));({struct _fat_ptr*_tmp7FE=({unsigned _tmp4A1=1;struct _fat_ptr*_tmp4A0=_cycalloc(_check_times(_tmp4A1,sizeof(struct _fat_ptr)));_tmp4A0[0]=s;_tmp4A0;});_tmp4A2->name=_tmp7FE;}),_tmp4A2->identity=- 1,_tmp4A2->kind=k;_tmp4A2;});}
# 2659
int Cyc_Tcutil_is_temp_tvar(struct Cyc_Absyn_Tvar*t){
# 2666
struct _fat_ptr s=*t->name;
return(int)*((const char*)_check_fat_subscript(s,sizeof(char),0))== (int)'#';}
# 2659
void*Cyc_Tcutil_fndecl2type(struct Cyc_Absyn_Fndecl*fd){
# 2672
if(fd->cached_type == 0){
# 2678
struct Cyc_List_List*fn_type_atts=0;
{struct Cyc_List_List*atts=(fd->i).attributes;for(0;atts != 0;atts=atts->tl){
if(({Cyc_Absyn_fntype_att((void*)atts->hd);}))
fn_type_atts=({struct Cyc_List_List*_tmp4A8=_cycalloc(sizeof(*_tmp4A8));_tmp4A8->hd=(void*)atts->hd,_tmp4A8->tl=fn_type_atts;_tmp4A8;});}}{
# 2679
struct Cyc_Absyn_FnInfo type_info=fd->i;
# 2683
type_info.attributes=fn_type_atts;{
struct Cyc_Absyn_FnType_Absyn_Type_struct*ret=({struct Cyc_Absyn_FnType_Absyn_Type_struct*_tmp4A9=_cycalloc(sizeof(*_tmp4A9));_tmp4A9->tag=5U,_tmp4A9->f1=type_info;_tmp4A9;});
# 2687
return(void*)ret;}}}
# 2672
return(void*)_check_null(fd->cached_type);}
# 2695
static void Cyc_Tcutil_replace_rop(struct Cyc_List_List*args,union Cyc_Relations_RelnOp*rop){
# 2697
union Cyc_Relations_RelnOp _stmttmp60=*rop;union Cyc_Relations_RelnOp _tmp4AB=_stmttmp60;struct Cyc_Absyn_Vardecl*_tmp4AC;struct Cyc_Absyn_Vardecl*_tmp4AD;switch((_tmp4AB.RNumelts).tag){case 2U: _LL1: _tmp4AD=(_tmp4AB.RVar).val;_LL2: {struct Cyc_Absyn_Vardecl*vd=_tmp4AD;
# 2699
struct _tuple1 _stmttmp61=*vd->name;struct _fat_ptr*_tmp4AF;union Cyc_Absyn_Nmspace _tmp4AE;_LL8: _tmp4AE=_stmttmp61.f1;_tmp4AF=_stmttmp61.f2;_LL9: {union Cyc_Absyn_Nmspace nmspace=_tmp4AE;struct _fat_ptr*var=_tmp4AF;
if(!((int)((nmspace.Loc_n).tag == 4)))goto _LL0;if(({({struct _fat_ptr _tmp7FF=(struct _fat_ptr)*var;Cyc_strcmp(_tmp7FF,({const char*_tmp4B0="return_value";_tag_fat(_tmp4B0,sizeof(char),13U);}));});})== 0){
# 2702
({union Cyc_Relations_RelnOp _tmp800=({Cyc_Relations_RReturn();});*rop=_tmp800;});
goto _LL0;}{
# 2700
unsigned c=0U;
# 2706
{struct Cyc_List_List*a=args;for(0;a != 0;(a=a->tl,c ++)){
struct _tuple9*_stmttmp62=(struct _tuple9*)a->hd;struct _fat_ptr*_tmp4B1;_LLB: _tmp4B1=_stmttmp62->f1;_LLC: {struct _fat_ptr*vopt=_tmp4B1;
if(vopt != 0){
if(({Cyc_strcmp((struct _fat_ptr)*var,(struct _fat_ptr)*vopt);})== 0){
({union Cyc_Relations_RelnOp _tmp801=({Cyc_Relations_RParam(c);});*rop=_tmp801;});
break;}}}}}
# 2715
goto _LL0;}}}case 3U: _LL3: _tmp4AC=(_tmp4AB.RNumelts).val;_LL4: {struct Cyc_Absyn_Vardecl*vd=_tmp4AC;
# 2717
struct _tuple1 _stmttmp63=*vd->name;struct _fat_ptr*_tmp4B3;union Cyc_Absyn_Nmspace _tmp4B2;_LLE: _tmp4B2=_stmttmp63.f1;_tmp4B3=_stmttmp63.f2;_LLF: {union Cyc_Absyn_Nmspace nmspace=_tmp4B2;struct _fat_ptr*var=_tmp4B3;
if(!((int)((nmspace.Loc_n).tag == 4)))goto _LL0;{unsigned c=0U;
# 2720
{struct Cyc_List_List*a=args;for(0;a != 0;(a=a->tl,c ++)){
struct _tuple9*_stmttmp64=(struct _tuple9*)a->hd;struct _fat_ptr*_tmp4B4;_LL11: _tmp4B4=_stmttmp64->f1;_LL12: {struct _fat_ptr*vopt=_tmp4B4;
if(vopt != 0){
if(({Cyc_strcmp((struct _fat_ptr)*var,(struct _fat_ptr)*vopt);})== 0){
({union Cyc_Relations_RelnOp _tmp802=({Cyc_Relations_RParamNumelts(c);});*rop=_tmp802;});
break;}}}}}
# 2729
goto _LL0;}}}default: _LL5: _LL6:
 goto _LL0;}_LL0:;}
# 2734
void Cyc_Tcutil_replace_rops(struct Cyc_List_List*args,struct Cyc_Relations_Reln*r){
# 2736
({Cyc_Tcutil_replace_rop(args,& r->rop1);});
({Cyc_Tcutil_replace_rop(args,& r->rop2);});}
# 2740
static struct Cyc_List_List*Cyc_Tcutil_extract_relns(struct Cyc_List_List*args,struct Cyc_Absyn_Exp*e){
# 2742
if(e == 0)return 0;{struct Cyc_List_List*relns=({Cyc_Relations_exp2relns(Cyc_Core_heap_region,e);});
# 2744
({({void(*_tmp804)(void(*f)(struct Cyc_List_List*,struct Cyc_Relations_Reln*),struct Cyc_List_List*env,struct Cyc_List_List*x)=({void(*_tmp4B7)(void(*f)(struct Cyc_List_List*,struct Cyc_Relations_Reln*),struct Cyc_List_List*env,struct Cyc_List_List*x)=(void(*)(void(*f)(struct Cyc_List_List*,struct Cyc_Relations_Reln*),struct Cyc_List_List*env,struct Cyc_List_List*x))Cyc_List_iter_c;_tmp4B7;});struct Cyc_List_List*_tmp803=args;_tmp804(Cyc_Tcutil_replace_rops,_tmp803,relns);});});
return relns;}}
# 2740
void*Cyc_Tcutil_snd_tqt(struct _tuple12*t){
# 2749
return(*t).f2;}
static struct _tuple12*Cyc_Tcutil_map2_tq(struct _tuple12*pr,void*t){
void*_tmp4BB;struct Cyc_Absyn_Tqual _tmp4BA;_LL1: _tmp4BA=pr->f1;_tmp4BB=pr->f2;_LL2: {struct Cyc_Absyn_Tqual tq=_tmp4BA;void*t2=_tmp4BB;
if(t2 == t)return pr;else{
return({struct _tuple12*_tmp4BC=_cycalloc(sizeof(*_tmp4BC));_tmp4BC->f1=tq,_tmp4BC->f2=t;_tmp4BC;});}}}struct _tuple26{struct _fat_ptr*f1;struct Cyc_Absyn_Tqual f2;};struct _tuple27{struct _tuple26*f1;void*f2;};
# 2756
static struct _tuple27*Cyc_Tcutil_substitute_f1(struct _RegionHandle*rgn,struct _tuple9*y){
return({struct _tuple27*_tmp4BF=_region_malloc(rgn,sizeof(*_tmp4BF));({struct _tuple26*_tmp805=({struct _tuple26*_tmp4BE=_region_malloc(rgn,sizeof(*_tmp4BE));_tmp4BE->f1=(*y).f1,_tmp4BE->f2=(*y).f2;_tmp4BE;});_tmp4BF->f1=_tmp805;}),_tmp4BF->f2=(*y).f3;_tmp4BF;});}
# 2760
static struct _tuple9*Cyc_Tcutil_substitute_f2(struct _tuple9*orig_arg,void*t){
struct _tuple9 _stmttmp65=*orig_arg;void*_tmp4C3;struct Cyc_Absyn_Tqual _tmp4C2;struct _fat_ptr*_tmp4C1;_LL1: _tmp4C1=_stmttmp65.f1;_tmp4C2=_stmttmp65.f2;_tmp4C3=_stmttmp65.f3;_LL2: {struct _fat_ptr*vopt_orig=_tmp4C1;struct Cyc_Absyn_Tqual tq_orig=_tmp4C2;void*t_orig=_tmp4C3;
if(t == t_orig)return orig_arg;return({struct _tuple9*_tmp4C4=_cycalloc(sizeof(*_tmp4C4));
_tmp4C4->f1=vopt_orig,_tmp4C4->f2=tq_orig,_tmp4C4->f3=t;_tmp4C4;});}}
# 2760
static struct Cyc_List_List*Cyc_Tcutil_substs(struct _RegionHandle*rgn,struct Cyc_List_List*inst,struct Cyc_List_List*ts);
# 2770
static struct Cyc_Absyn_Exp*Cyc_Tcutil_copye(struct Cyc_Absyn_Exp*old,void*r){
# 2772
return({struct Cyc_Absyn_Exp*_tmp4C6=_cycalloc(sizeof(*_tmp4C6));_tmp4C6->topt=old->topt,_tmp4C6->r=r,_tmp4C6->loc=old->loc,_tmp4C6->annot=old->annot;_tmp4C6;});}
# 2770
struct Cyc_Absyn_Exp*Cyc_Tcutil_rsubsexp(struct _RegionHandle*r,struct Cyc_List_List*inst,struct Cyc_Absyn_Exp*e){
# 2778
void*_stmttmp66=e->r;void*_tmp4C8=_stmttmp66;void*_tmp4C9;struct Cyc_List_List*_tmp4CB;void*_tmp4CA;struct Cyc_Absyn_Exp*_tmp4CC;struct Cyc_Absyn_Exp*_tmp4CD;void*_tmp4CE;enum Cyc_Absyn_Coercion _tmp4D2;int _tmp4D1;struct Cyc_Absyn_Exp*_tmp4D0;void*_tmp4CF;struct Cyc_Absyn_Exp*_tmp4D4;struct Cyc_Absyn_Exp*_tmp4D3;struct _tuple0*_tmp4D7;struct Cyc_Absyn_Exp*_tmp4D6;struct Cyc_Absyn_Exp*_tmp4D5;struct Cyc_Absyn_Exp*_tmp4D9;struct Cyc_Absyn_Exp*_tmp4D8;struct Cyc_Absyn_Exp*_tmp4DB;struct Cyc_Absyn_Exp*_tmp4DA;struct Cyc_Absyn_Exp*_tmp4DE;struct Cyc_Absyn_Exp*_tmp4DD;struct Cyc_Absyn_Exp*_tmp4DC;struct Cyc_List_List*_tmp4E0;enum Cyc_Absyn_Primop _tmp4DF;switch(*((int*)_tmp4C8)){case 0U: _LL1: _LL2:
 goto _LL4;case 32U: _LL3: _LL4:
 goto _LL6;case 33U: _LL5: _LL6:
 goto _LL8;case 2U: _LL7: _LL8:
 goto _LLA;case 1U: _LL9: _LLA:
 return e;case 3U: _LLB: _tmp4DF=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp4C8)->f1;_tmp4E0=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp4C8)->f2;_LLC: {enum Cyc_Absyn_Primop p=_tmp4DF;struct Cyc_List_List*es=_tmp4E0;
# 2786
if(({Cyc_List_length(es);})== 1){
struct Cyc_Absyn_Exp*e1=(struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(es))->hd;
struct Cyc_Absyn_Exp*new_e1=({Cyc_Tcutil_rsubsexp(r,inst,e1);});
if(new_e1 == e1)return e;return({struct Cyc_Absyn_Exp*(*_tmp4E1)(struct Cyc_Absyn_Exp*old,void*r)=Cyc_Tcutil_copye;struct Cyc_Absyn_Exp*_tmp4E2=e;void*_tmp4E3=(void*)({struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*_tmp4E5=_cycalloc(sizeof(*_tmp4E5));
_tmp4E5->tag=3U,_tmp4E5->f1=p,({struct Cyc_List_List*_tmp806=({struct Cyc_Absyn_Exp*_tmp4E4[1U];_tmp4E4[0]=new_e1;Cyc_List_list(_tag_fat(_tmp4E4,sizeof(struct Cyc_Absyn_Exp*),1U));});_tmp4E5->f2=_tmp806;});_tmp4E5;});_tmp4E1(_tmp4E2,_tmp4E3);});}else{
if(({Cyc_List_length(es);})== 2){
struct Cyc_Absyn_Exp*e1=(struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(es))->hd;
struct Cyc_Absyn_Exp*e2=(struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(es->tl))->hd;
struct Cyc_Absyn_Exp*new_e1=({Cyc_Tcutil_rsubsexp(r,inst,e1);});
struct Cyc_Absyn_Exp*new_e2=({Cyc_Tcutil_rsubsexp(r,inst,e2);});
if(new_e1 == e1 && new_e2 == e2)return e;return({struct Cyc_Absyn_Exp*(*_tmp4E6)(struct Cyc_Absyn_Exp*old,void*r)=Cyc_Tcutil_copye;struct Cyc_Absyn_Exp*_tmp4E7=e;void*_tmp4E8=(void*)({struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*_tmp4EA=_cycalloc(sizeof(*_tmp4EA));
_tmp4EA->tag=3U,_tmp4EA->f1=p,({struct Cyc_List_List*_tmp807=({struct Cyc_Absyn_Exp*_tmp4E9[2U];_tmp4E9[0]=new_e1,_tmp4E9[1]=new_e2;Cyc_List_list(_tag_fat(_tmp4E9,sizeof(struct Cyc_Absyn_Exp*),2U));});_tmp4EA->f2=_tmp807;});_tmp4EA;});_tmp4E6(_tmp4E7,_tmp4E8);});}else{
return({void*_tmp4EB=0U;({struct Cyc_Absyn_Exp*(*_tmp809)(struct _fat_ptr fmt,struct _fat_ptr ap)=({struct Cyc_Absyn_Exp*(*_tmp4ED)(struct _fat_ptr fmt,struct _fat_ptr ap)=(struct Cyc_Absyn_Exp*(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Tcutil_impos;_tmp4ED;});struct _fat_ptr _tmp808=({const char*_tmp4EC="primop does not have 1 or 2 args!";_tag_fat(_tmp4EC,sizeof(char),34U);});_tmp809(_tmp808,_tag_fat(_tmp4EB,sizeof(void*),0U));});});}}}case 6U: _LLD: _tmp4DC=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp4C8)->f1;_tmp4DD=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp4C8)->f2;_tmp4DE=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp4C8)->f3;_LLE: {struct Cyc_Absyn_Exp*e1=_tmp4DC;struct Cyc_Absyn_Exp*e2=_tmp4DD;struct Cyc_Absyn_Exp*e3=_tmp4DE;
# 2800
struct Cyc_Absyn_Exp*new_e1=({Cyc_Tcutil_rsubsexp(r,inst,e1);});
struct Cyc_Absyn_Exp*new_e2=({Cyc_Tcutil_rsubsexp(r,inst,e2);});
struct Cyc_Absyn_Exp*new_e3=({Cyc_Tcutil_rsubsexp(r,inst,e3);});
if((new_e1 == e1 && new_e2 == e2)&& new_e3 == e3)return e;return({({struct Cyc_Absyn_Exp*_tmp80A=e;Cyc_Tcutil_copye(_tmp80A,(void*)({struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*_tmp4EE=_cycalloc(sizeof(*_tmp4EE));_tmp4EE->tag=6U,_tmp4EE->f1=new_e1,_tmp4EE->f2=new_e2,_tmp4EE->f3=new_e3;_tmp4EE;}));});});}case 7U: _LLF: _tmp4DA=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp4C8)->f1;_tmp4DB=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp4C8)->f2;_LL10: {struct Cyc_Absyn_Exp*e1=_tmp4DA;struct Cyc_Absyn_Exp*e2=_tmp4DB;
# 2806
struct Cyc_Absyn_Exp*new_e1=({Cyc_Tcutil_rsubsexp(r,inst,e1);});
struct Cyc_Absyn_Exp*new_e2=({Cyc_Tcutil_rsubsexp(r,inst,e2);});
if(new_e1 == e1 && new_e2 == e2)return e;return({({struct Cyc_Absyn_Exp*_tmp80B=e;Cyc_Tcutil_copye(_tmp80B,(void*)({struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*_tmp4EF=_cycalloc(sizeof(*_tmp4EF));_tmp4EF->tag=7U,_tmp4EF->f1=new_e1,_tmp4EF->f2=new_e2;_tmp4EF;}));});});}case 8U: _LL11: _tmp4D8=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp4C8)->f1;_tmp4D9=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp4C8)->f2;_LL12: {struct Cyc_Absyn_Exp*e1=_tmp4D8;struct Cyc_Absyn_Exp*e2=_tmp4D9;
# 2811
struct Cyc_Absyn_Exp*new_e1=({Cyc_Tcutil_rsubsexp(r,inst,e1);});
struct Cyc_Absyn_Exp*new_e2=({Cyc_Tcutil_rsubsexp(r,inst,e2);});
if(new_e1 == e1 && new_e2 == e2)return e;return({({struct Cyc_Absyn_Exp*_tmp80C=e;Cyc_Tcutil_copye(_tmp80C,(void*)({struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*_tmp4F0=_cycalloc(sizeof(*_tmp4F0));_tmp4F0->tag=8U,_tmp4F0->f1=new_e1,_tmp4F0->f2=new_e2;_tmp4F0;}));});});}case 34U: _LL13: _tmp4D5=((struct Cyc_Absyn_Spawn_e_Absyn_Raw_exp_struct*)_tmp4C8)->f1;_tmp4D6=((struct Cyc_Absyn_Spawn_e_Absyn_Raw_exp_struct*)_tmp4C8)->f2;_tmp4D7=((struct Cyc_Absyn_Spawn_e_Absyn_Raw_exp_struct*)_tmp4C8)->f3;_LL14: {struct Cyc_Absyn_Exp*e1=_tmp4D5;struct Cyc_Absyn_Exp*e2=_tmp4D6;struct _tuple0*f=_tmp4D7;
# 2817
struct Cyc_Absyn_Exp*new_e1=({Cyc_Tcutil_rsubsexp(r,inst,e1);});
struct Cyc_Absyn_Exp*new_e2=({Cyc_Tcutil_rsubsexp(r,inst,e2);});
if(new_e1 == e1 && new_e2 == e2)return e;return({({struct Cyc_Absyn_Exp*_tmp80D=e;Cyc_Tcutil_copye(_tmp80D,(void*)({struct Cyc_Absyn_Spawn_e_Absyn_Raw_exp_struct*_tmp4F1=_cycalloc(sizeof(*_tmp4F1));_tmp4F1->tag=34U,_tmp4F1->f1=new_e1,_tmp4F1->f2=new_e2,_tmp4F1->f3=f;_tmp4F1;}));});});}case 9U: _LL15: _tmp4D3=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp4C8)->f1;_tmp4D4=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp4C8)->f2;_LL16: {struct Cyc_Absyn_Exp*e1=_tmp4D3;struct Cyc_Absyn_Exp*e2=_tmp4D4;
# 2824
struct Cyc_Absyn_Exp*new_e1=({Cyc_Tcutil_rsubsexp(r,inst,e1);});
struct Cyc_Absyn_Exp*new_e2=({Cyc_Tcutil_rsubsexp(r,inst,e2);});
if(new_e1 == e1 && new_e2 == e2)return e;return({({struct Cyc_Absyn_Exp*_tmp80E=e;Cyc_Tcutil_copye(_tmp80E,(void*)({struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*_tmp4F2=_cycalloc(sizeof(*_tmp4F2));_tmp4F2->tag=9U,_tmp4F2->f1=new_e1,_tmp4F2->f2=new_e2;_tmp4F2;}));});});}case 14U: _LL17: _tmp4CF=(void*)((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp4C8)->f1;_tmp4D0=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp4C8)->f2;_tmp4D1=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp4C8)->f3;_tmp4D2=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp4C8)->f4;_LL18: {void*t=_tmp4CF;struct Cyc_Absyn_Exp*e1=_tmp4D0;int b=_tmp4D1;enum Cyc_Absyn_Coercion c=_tmp4D2;
# 2830
struct Cyc_Absyn_Exp*new_e1=({Cyc_Tcutil_rsubsexp(r,inst,e1);});
void*new_typ=({Cyc_Tcutil_rsubstitute(r,inst,t);});
if(new_e1 == e1 && new_typ == t)return e;return({({struct Cyc_Absyn_Exp*_tmp80F=e;Cyc_Tcutil_copye(_tmp80F,(void*)({struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*_tmp4F3=_cycalloc(sizeof(*_tmp4F3));_tmp4F3->tag=14U,_tmp4F3->f1=new_typ,_tmp4F3->f2=new_e1,_tmp4F3->f3=b,_tmp4F3->f4=c;_tmp4F3;}));});});}case 17U: _LL19: _tmp4CE=(void*)((struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct*)_tmp4C8)->f1;_LL1A: {void*t=_tmp4CE;
# 2835
void*new_typ=({Cyc_Tcutil_rsubstitute(r,inst,t);});
if(new_typ == t)return e;return({({struct Cyc_Absyn_Exp*_tmp810=e;Cyc_Tcutil_copye(_tmp810,(void*)({struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct*_tmp4F4=_cycalloc(sizeof(*_tmp4F4));_tmp4F4->tag=17U,_tmp4F4->f1=new_typ;_tmp4F4;}));});});}case 18U: _LL1B: _tmp4CD=((struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct*)_tmp4C8)->f1;_LL1C: {struct Cyc_Absyn_Exp*e1=_tmp4CD;
# 2839
struct Cyc_Absyn_Exp*new_e1=({Cyc_Tcutil_rsubsexp(r,inst,e1);});
if(new_e1 == e1)return e;return({({struct Cyc_Absyn_Exp*_tmp811=e;Cyc_Tcutil_copye(_tmp811,(void*)({struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct*_tmp4F5=_cycalloc(sizeof(*_tmp4F5));_tmp4F5->tag=18U,_tmp4F5->f1=new_e1;_tmp4F5;}));});});}case 42U: _LL1D: _tmp4CC=((struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct*)_tmp4C8)->f1;_LL1E: {struct Cyc_Absyn_Exp*e1=_tmp4CC;
# 2843
struct Cyc_Absyn_Exp*new_e1=({Cyc_Tcutil_rsubsexp(r,inst,e1);});
if(new_e1 == e1)return e;return({({struct Cyc_Absyn_Exp*_tmp812=e;Cyc_Tcutil_copye(_tmp812,(void*)({struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct*_tmp4F6=_cycalloc(sizeof(*_tmp4F6));_tmp4F6->tag=42U,_tmp4F6->f1=new_e1;_tmp4F6;}));});});}case 19U: _LL1F: _tmp4CA=(void*)((struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct*)_tmp4C8)->f1;_tmp4CB=((struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct*)_tmp4C8)->f2;_LL20: {void*t=_tmp4CA;struct Cyc_List_List*f=_tmp4CB;
# 2847
void*new_typ=({Cyc_Tcutil_rsubstitute(r,inst,t);});
if(new_typ == t)return e;return({({struct Cyc_Absyn_Exp*_tmp813=e;Cyc_Tcutil_copye(_tmp813,(void*)({struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct*_tmp4F7=_cycalloc(sizeof(*_tmp4F7));_tmp4F7->tag=19U,_tmp4F7->f1=new_typ,_tmp4F7->f2=f;_tmp4F7;}));});});}case 40U: _LL21: _tmp4C9=(void*)((struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct*)_tmp4C8)->f1;_LL22: {void*t=_tmp4C9;
# 2851
void*new_typ=({Cyc_Tcutil_rsubstitute(r,inst,t);});
if(new_typ == t)return e;{void*_stmttmp67=({Cyc_Tcutil_compress(new_typ);});void*_tmp4F8=_stmttmp67;struct Cyc_Absyn_Exp*_tmp4F9;if(((struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_tmp4F8)->tag == 9U){_LL26: _tmp4F9=((struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_tmp4F8)->f1;_LL27: {struct Cyc_Absyn_Exp*e=_tmp4F9;
# 2855
return e;}}else{_LL28: _LL29:
 return({({struct Cyc_Absyn_Exp*_tmp814=e;Cyc_Tcutil_copye(_tmp814,(void*)({struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct*_tmp4FA=_cycalloc(sizeof(*_tmp4FA));_tmp4FA->tag=40U,_tmp4FA->f1=new_typ;_tmp4FA;}));});});}_LL25:;}}default: _LL23: _LL24:
# 2858
 return({void*_tmp4FB=0U;({struct Cyc_Absyn_Exp*(*_tmp816)(struct _fat_ptr fmt,struct _fat_ptr ap)=({struct Cyc_Absyn_Exp*(*_tmp4FD)(struct _fat_ptr fmt,struct _fat_ptr ap)=(struct Cyc_Absyn_Exp*(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Tcutil_impos;_tmp4FD;});struct _fat_ptr _tmp815=({const char*_tmp4FC="non-type-level-expression in Tcutil::rsubsexp";_tag_fat(_tmp4FC,sizeof(char),46U);});_tmp816(_tmp815,_tag_fat(_tmp4FB,sizeof(void*),0U));});});}_LL0:;}
# 2862
static struct Cyc_Absyn_Exp*Cyc_Tcutil_rsubs_exp_opt(struct _RegionHandle*r,struct Cyc_List_List*inst,struct Cyc_Absyn_Exp*e){
# 2865
if(e == 0)return 0;else{
return({Cyc_Tcutil_rsubsexp(r,inst,e);});}}
# 2869
static struct Cyc_Absyn_Aggrfield*Cyc_Tcutil_subst_aggrfield(struct _RegionHandle*r,struct Cyc_List_List*inst,struct Cyc_Absyn_Aggrfield*f){
# 2872
void*t=f->type;
struct Cyc_Absyn_Exp*req=f->requires_clause;
void*new_t=({Cyc_Tcutil_rsubstitute(r,inst,t);});
struct Cyc_Absyn_Exp*new_req=({Cyc_Tcutil_rsubs_exp_opt(r,inst,req);});
if(t == new_t && req == new_req)return f;else{
return({struct Cyc_Absyn_Aggrfield*_tmp500=_cycalloc(sizeof(*_tmp500));_tmp500->name=f->name,_tmp500->tq=f->tq,_tmp500->type=new_t,_tmp500->width=f->width,_tmp500->attributes=f->attributes,_tmp500->requires_clause=new_req;_tmp500;});}}
# 2882
static struct Cyc_List_List*Cyc_Tcutil_subst_aggrfields(struct _RegionHandle*r,struct Cyc_List_List*inst,struct Cyc_List_List*fs){
# 2885
if(fs == 0)return 0;{struct Cyc_Absyn_Aggrfield*f=(struct Cyc_Absyn_Aggrfield*)fs->hd;
# 2887
struct Cyc_List_List*t=fs->tl;
struct Cyc_Absyn_Aggrfield*new_f=({Cyc_Tcutil_subst_aggrfield(r,inst,f);});
struct Cyc_List_List*new_t=({Cyc_Tcutil_subst_aggrfields(r,inst,t);});
if(new_f == f && new_t == t)return fs;return({struct Cyc_List_List*_tmp502=_cycalloc(sizeof(*_tmp502));
_tmp502->hd=new_f,_tmp502->tl=new_t;_tmp502;});}}
# 2894
struct Cyc_List_List*Cyc_Tcutil_rsubst_rgnpo(struct _RegionHandle*rgn,struct Cyc_List_List*inst,struct Cyc_List_List*rgn_po){
# 2897
struct _tuple0 _stmttmp68=({Cyc_List_rsplit(rgn,rgn,rgn_po);});struct Cyc_List_List*_tmp505;struct Cyc_List_List*_tmp504;_LL1: _tmp504=_stmttmp68.f1;_tmp505=_stmttmp68.f2;_LL2: {struct Cyc_List_List*rpo1a=_tmp504;struct Cyc_List_List*rpo1b=_tmp505;
struct Cyc_List_List*rpo2a=({Cyc_Tcutil_substs(rgn,inst,rpo1a);});
struct Cyc_List_List*rpo2b=({Cyc_Tcutil_substs(rgn,inst,rpo1b);});
if(rpo2a == rpo1a && rpo2b == rpo1b)
return rgn_po;else{
# 2903
return({Cyc_List_zip(rpo2a,rpo2b);});}}}
# 2894
void*Cyc_Tcutil_rsubstitute(struct _RegionHandle*rgn,struct Cyc_List_List*inst,void*t){
# 2913
void*_stmttmp69=({Cyc_Tcutil_compress(t);});void*_tmp507=_stmttmp69;struct Cyc_Absyn_Exp*_tmp508;struct Cyc_Absyn_Exp*_tmp509;struct Cyc_List_List*_tmp50B;void*_tmp50A;void*_tmp50C;void*_tmp50D;struct Cyc_List_List*_tmp50F;enum Cyc_Absyn_AggrKind _tmp50E;struct Cyc_List_List*_tmp510;int _tmp51F;struct Cyc_List_List*_tmp51E;struct Cyc_List_List*_tmp51D;struct Cyc_List_List*_tmp51C;struct Cyc_Absyn_Exp*_tmp51B;struct Cyc_Absyn_Exp*_tmp51A;struct Cyc_List_List*_tmp519;struct Cyc_List_List*_tmp518;struct Cyc_Absyn_VarargInfo*_tmp517;int _tmp516;struct Cyc_List_List*_tmp515;void*_tmp514;struct Cyc_Absyn_Tqual _tmp513;void*_tmp512;struct Cyc_List_List*_tmp511;void*_tmp525;void*_tmp524;void*_tmp523;void*_tmp522;struct Cyc_Absyn_Tqual _tmp521;void*_tmp520;unsigned _tmp52A;void*_tmp529;struct Cyc_Absyn_Exp*_tmp528;struct Cyc_Absyn_Tqual _tmp527;void*_tmp526;void*_tmp52E;struct Cyc_Absyn_Typedefdecl*_tmp52D;struct Cyc_List_List*_tmp52C;struct _tuple1*_tmp52B;struct Cyc_Absyn_Tvar*_tmp52F;switch(*((int*)_tmp507)){case 2U: _LL1: _tmp52F=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp507)->f1;_LL2: {struct Cyc_Absyn_Tvar*v=_tmp52F;
# 2917
struct _handler_cons _tmp530;_push_handler(& _tmp530);{int _tmp532=0;if(setjmp(_tmp530.handler))_tmp532=1;if(!_tmp532){{void*_tmp534=({({void*(*_tmp818)(int(*cmp)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_List_List*l,struct Cyc_Absyn_Tvar*x)=({void*(*_tmp533)(int(*cmp)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_List_List*l,struct Cyc_Absyn_Tvar*x)=(void*(*)(int(*cmp)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_List_List*l,struct Cyc_Absyn_Tvar*x))Cyc_List_assoc_cmp;_tmp533;});struct Cyc_List_List*_tmp817=inst;_tmp818(Cyc_Absyn_tvar_cmp,_tmp817,v);});});_npop_handler(0U);return _tmp534;};_pop_handler();}else{void*_tmp531=(void*)Cyc_Core_get_exn_thrown();void*_tmp535=_tmp531;void*_tmp536;if(((struct Cyc_Core_Not_found_exn_struct*)_tmp535)->tag == Cyc_Core_Not_found){_LL1C: _LL1D:
 return t;}else{_LL1E: _tmp536=_tmp535;_LL1F: {void*exn=_tmp536;(int)_rethrow(exn);}}_LL1B:;}}}case 8U: _LL3: _tmp52B=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp507)->f1;_tmp52C=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp507)->f2;_tmp52D=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp507)->f3;_tmp52E=(void*)((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp507)->f4;_LL4: {struct _tuple1*n=_tmp52B;struct Cyc_List_List*ts=_tmp52C;struct Cyc_Absyn_Typedefdecl*td=_tmp52D;void*topt=_tmp52E;
# 2920
struct Cyc_List_List*new_ts=({Cyc_Tcutil_substs(rgn,inst,ts);});
return new_ts == ts?t:(void*)({struct Cyc_Absyn_TypedefType_Absyn_Type_struct*_tmp537=_cycalloc(sizeof(*_tmp537));_tmp537->tag=8U,_tmp537->f1=n,_tmp537->f2=new_ts,_tmp537->f3=td,_tmp537->f4=topt;_tmp537;});}case 4U: _LL5: _tmp526=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp507)->f1).elt_type;_tmp527=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp507)->f1).tq;_tmp528=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp507)->f1).num_elts;_tmp529=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp507)->f1).zero_term;_tmp52A=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp507)->f1).zt_loc;_LL6: {void*t1=_tmp526;struct Cyc_Absyn_Tqual tq=_tmp527;struct Cyc_Absyn_Exp*e=_tmp528;void*zt=_tmp529;unsigned ztl=_tmp52A;
# 2923
void*new_t1=({Cyc_Tcutil_rsubstitute(rgn,inst,t1);});
struct Cyc_Absyn_Exp*new_e=e == 0?0:({Cyc_Tcutil_rsubsexp(rgn,inst,e);});
void*new_zt=({Cyc_Tcutil_rsubstitute(rgn,inst,zt);});
return(new_t1 == t1 && new_e == e)&& new_zt == zt?t:(void*)({struct Cyc_Absyn_ArrayType_Absyn_Type_struct*_tmp538=_cycalloc(sizeof(*_tmp538));
_tmp538->tag=4U,(_tmp538->f1).elt_type=new_t1,(_tmp538->f1).tq=tq,(_tmp538->f1).num_elts=new_e,(_tmp538->f1).zero_term=new_zt,(_tmp538->f1).zt_loc=ztl;_tmp538;});}case 3U: _LL7: _tmp520=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp507)->f1).elt_type;_tmp521=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp507)->f1).elt_tq;_tmp522=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp507)->f1).ptr_atts).rgn;_tmp523=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp507)->f1).ptr_atts).nullable;_tmp524=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp507)->f1).ptr_atts).bounds;_tmp525=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp507)->f1).ptr_atts).zero_term;_LL8: {void*t1=_tmp520;struct Cyc_Absyn_Tqual tq=_tmp521;void*r=_tmp522;void*n=_tmp523;void*b=_tmp524;void*zt=_tmp525;
# 2929
void*new_t1=({Cyc_Tcutil_rsubstitute(rgn,inst,t1);});
void*new_r=({Cyc_Tcutil_rsubstitute(rgn,inst,r);});
void*new_b=({Cyc_Tcutil_rsubstitute(rgn,inst,b);});
void*new_zt=({Cyc_Tcutil_rsubstitute(rgn,inst,zt);});
if(((new_t1 == t1 && new_r == r)&& new_b == b)&& new_zt == zt)
return t;
# 2933
return({Cyc_Absyn_pointer_type(({struct Cyc_Absyn_PtrInfo _tmp6E3;_tmp6E3.elt_type=new_t1,_tmp6E3.elt_tq=tq,(_tmp6E3.ptr_atts).rgn=new_r,(_tmp6E3.ptr_atts).nullable=n,(_tmp6E3.ptr_atts).bounds=new_b,(_tmp6E3.ptr_atts).zero_term=new_zt,(_tmp6E3.ptr_atts).ptrloc=0;_tmp6E3;}));});}case 5U: _LL9: _tmp511=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp507)->f1).tvars;_tmp512=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp507)->f1).effect;_tmp513=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp507)->f1).ret_tqual;_tmp514=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp507)->f1).ret_type;_tmp515=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp507)->f1).args;_tmp516=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp507)->f1).c_varargs;_tmp517=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp507)->f1).cyc_varargs;_tmp518=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp507)->f1).rgn_po;_tmp519=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp507)->f1).attributes;_tmp51A=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp507)->f1).requires_clause;_tmp51B=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp507)->f1).ensures_clause;_tmp51C=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp507)->f1).ieffect;_tmp51D=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp507)->f1).oeffect;_tmp51E=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp507)->f1).throws;_tmp51F=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp507)->f1).reentrant;_LLA: {struct Cyc_List_List*vs=_tmp511;void*eff=_tmp512;struct Cyc_Absyn_Tqual rtq=_tmp513;void*rtyp=_tmp514;struct Cyc_List_List*args=_tmp515;int c_varargs=_tmp516;struct Cyc_Absyn_VarargInfo*cyc_varargs=_tmp517;struct Cyc_List_List*rgn_po=_tmp518;struct Cyc_List_List*atts=_tmp519;struct Cyc_Absyn_Exp*req=_tmp51A;struct Cyc_Absyn_Exp*ens=_tmp51B;struct Cyc_List_List*ieff=_tmp51C;struct Cyc_List_List*oeff=_tmp51D;struct Cyc_List_List*throws=_tmp51E;int reentrant=_tmp51F;
# 2940
{struct Cyc_List_List*p=vs;for(0;p != 0;p=p->tl){
inst=({struct Cyc_List_List*_tmp53A=_region_malloc(rgn,sizeof(*_tmp53A));({struct _tuple19*_tmp81A=({struct _tuple19*_tmp539=_region_malloc(rgn,sizeof(*_tmp539));_tmp539->f1=(struct Cyc_Absyn_Tvar*)p->hd,({void*_tmp819=({Cyc_Absyn_var_type((struct Cyc_Absyn_Tvar*)p->hd);});_tmp539->f2=_tmp819;});_tmp539;});_tmp53A->hd=_tmp81A;}),_tmp53A->tl=inst;_tmp53A;});}}{
struct _tuple0 _stmttmp6A=({struct _tuple0(*_tmp544)(struct _RegionHandle*r1,struct _RegionHandle*r2,struct Cyc_List_List*x)=Cyc_List_rsplit;struct _RegionHandle*_tmp545=rgn;struct _RegionHandle*_tmp546=rgn;struct Cyc_List_List*_tmp547=({({struct Cyc_List_List*(*_tmp81D)(struct _RegionHandle*,struct _tuple27*(*f)(struct _RegionHandle*,struct _tuple9*),struct _RegionHandle*env,struct Cyc_List_List*x)=({
struct Cyc_List_List*(*_tmp548)(struct _RegionHandle*,struct _tuple27*(*f)(struct _RegionHandle*,struct _tuple9*),struct _RegionHandle*env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct _RegionHandle*,struct _tuple27*(*f)(struct _RegionHandle*,struct _tuple9*),struct _RegionHandle*env,struct Cyc_List_List*x))Cyc_List_rmap_c;_tmp548;});struct _RegionHandle*_tmp81C=rgn;struct _RegionHandle*_tmp81B=rgn;_tmp81D(_tmp81C,Cyc_Tcutil_substitute_f1,_tmp81B,args);});});_tmp544(_tmp545,_tmp546,_tmp547);});
# 2942
struct Cyc_List_List*_tmp53C;struct Cyc_List_List*_tmp53B;_LL21: _tmp53B=_stmttmp6A.f1;_tmp53C=_stmttmp6A.f2;_LL22: {struct Cyc_List_List*qs=_tmp53B;struct Cyc_List_List*ts=_tmp53C;
# 2944
struct Cyc_List_List*args2=args;
struct Cyc_List_List*ts2=({Cyc_Tcutil_substs(rgn,inst,ts);});
if(ts2 != ts)
args2=({({struct Cyc_List_List*(*_tmp81F)(struct _tuple9*(*f)(struct _tuple9*,void*),struct Cyc_List_List*x,struct Cyc_List_List*y)=({struct Cyc_List_List*(*_tmp53D)(struct _tuple9*(*f)(struct _tuple9*,void*),struct Cyc_List_List*x,struct Cyc_List_List*y)=(struct Cyc_List_List*(*)(struct _tuple9*(*f)(struct _tuple9*,void*),struct Cyc_List_List*x,struct Cyc_List_List*y))Cyc_List_map2;_tmp53D;});struct Cyc_List_List*_tmp81E=args;_tmp81F(Cyc_Tcutil_substitute_f2,_tmp81E,ts2);});});{
# 2946
void*eff2;
# 2949
if(eff == 0)
eff2=0;else{
# 2952
void*new_eff=({Cyc_Tcutil_rsubstitute(rgn,inst,eff);});
if(new_eff == eff)
eff2=eff;else{
# 2956
eff2=new_eff;}}{
# 2958
struct Cyc_Absyn_VarargInfo*cyc_varargs2;
if(cyc_varargs == 0)
cyc_varargs2=0;else{
# 2962
struct Cyc_Absyn_VarargInfo _stmttmp6B=*cyc_varargs;int _tmp541;void*_tmp540;struct Cyc_Absyn_Tqual _tmp53F;struct _fat_ptr*_tmp53E;_LL24: _tmp53E=_stmttmp6B.name;_tmp53F=_stmttmp6B.tq;_tmp540=_stmttmp6B.type;_tmp541=_stmttmp6B.inject;_LL25: {struct _fat_ptr*n=_tmp53E;struct Cyc_Absyn_Tqual tq=_tmp53F;void*t=_tmp540;int i=_tmp541;
void*t2=({Cyc_Tcutil_rsubstitute(rgn,inst,t);});
if(t2 == t)cyc_varargs2=cyc_varargs;else{
cyc_varargs2=({struct Cyc_Absyn_VarargInfo*_tmp542=_cycalloc(sizeof(*_tmp542));_tmp542->name=n,_tmp542->tq=tq,_tmp542->type=t2,_tmp542->inject=i;_tmp542;});}}}{
# 2967
struct Cyc_List_List*rgn_po2=({Cyc_Tcutil_rsubst_rgnpo(rgn,inst,rgn_po);});
struct Cyc_Absyn_Exp*req2=({Cyc_Tcutil_rsubs_exp_opt(rgn,inst,req);});
struct Cyc_Absyn_Exp*ens2=({Cyc_Tcutil_rsubs_exp_opt(rgn,inst,ens);});
struct Cyc_List_List*req_relns2=({Cyc_Tcutil_extract_relns(args2,req2);});
struct Cyc_List_List*ens_relns2=({Cyc_Tcutil_extract_relns(args2,ens2);});
struct Cyc_List_List*ieff2=({Cyc_Absyn_rsubstitute_effect(rgn,inst,ieff);});
struct Cyc_List_List*oeff2=({Cyc_Absyn_rsubstitute_effect(rgn,inst,oeff);});
int reentrant2=({Cyc_Absyn_rsubstitute_reentrant(rgn,inst,reentrant);});
return(void*)({struct Cyc_Absyn_FnType_Absyn_Type_struct*_tmp543=_cycalloc(sizeof(*_tmp543));_tmp543->tag=5U,(_tmp543->f1).tvars=vs,(_tmp543->f1).effect=eff2,(_tmp543->f1).ret_tqual=rtq,({void*_tmp820=({Cyc_Tcutil_rsubstitute(rgn,inst,rtyp);});(_tmp543->f1).ret_type=_tmp820;}),(_tmp543->f1).args=args2,(_tmp543->f1).c_varargs=c_varargs,(_tmp543->f1).cyc_varargs=cyc_varargs2,(_tmp543->f1).rgn_po=rgn_po2,(_tmp543->f1).attributes=atts,(_tmp543->f1).requires_clause=req2,(_tmp543->f1).requires_relns=req_relns2,(_tmp543->f1).ensures_clause=ens2,(_tmp543->f1).ensures_relns=ens_relns2,(_tmp543->f1).ieffect=ieff2,(_tmp543->f1).oeffect=oeff2,(_tmp543->f1).throws=throws,(_tmp543->f1).reentrant=reentrant;_tmp543;});}}}}}}case 6U: _LLB: _tmp510=((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp507)->f1;_LLC: {struct Cyc_List_List*tqts=_tmp510;
# 2980
struct Cyc_List_List*ts2=0;
int change=0;
{struct Cyc_List_List*ts1=tqts;for(0;ts1 != 0;ts1=ts1->tl){
void*t1=(*((struct _tuple12*)ts1->hd)).f2;
void*t2=({Cyc_Tcutil_rsubstitute(rgn,inst,t1);});
if(t1 != t2)
change=1;
# 2985
ts2=({struct Cyc_List_List*_tmp549=_region_malloc(rgn,sizeof(*_tmp549));
# 2988
_tmp549->hd=t2,_tmp549->tl=ts2;_tmp549;});}}
# 2990
if(!change)
return t;
# 2990
return(void*)({struct Cyc_Absyn_TupleType_Absyn_Type_struct*_tmp54F=_cycalloc(sizeof(*_tmp54F));
# 2992
_tmp54F->tag=6U,({struct Cyc_List_List*_tmp821=({struct Cyc_List_List*(*_tmp54A)(struct _tuple12*(*f)(struct _tuple12*,void*),struct Cyc_List_List*x,struct Cyc_List_List*y)=({struct Cyc_List_List*(*_tmp54B)(struct _tuple12*(*f)(struct _tuple12*,void*),struct Cyc_List_List*x,struct Cyc_List_List*y)=(struct Cyc_List_List*(*)(struct _tuple12*(*f)(struct _tuple12*,void*),struct Cyc_List_List*x,struct Cyc_List_List*y))Cyc_List_map2;_tmp54B;});struct _tuple12*(*_tmp54C)(struct _tuple12*pr,void*t)=Cyc_Tcutil_map2_tq;struct Cyc_List_List*_tmp54D=tqts;struct Cyc_List_List*_tmp54E=({Cyc_List_imp_rev(ts2);});_tmp54A(_tmp54C,_tmp54D,_tmp54E);});_tmp54F->f1=_tmp821;});_tmp54F;});}case 7U: _LLD: _tmp50E=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp507)->f1;_tmp50F=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp507)->f2;_LLE: {enum Cyc_Absyn_AggrKind k=_tmp50E;struct Cyc_List_List*fs=_tmp50F;
# 2994
struct Cyc_List_List*new_fs=({Cyc_Tcutil_subst_aggrfields(rgn,inst,fs);});
if(fs == new_fs)return t;return(void*)({struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*_tmp550=_cycalloc(sizeof(*_tmp550));
_tmp550->tag=7U,_tmp550->f1=k,_tmp550->f2=new_fs;_tmp550;});}case 1U: _LLF: _tmp50D=(void*)((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp507)->f2;_LL10: {void*r=_tmp50D;
# 2998
if(r != 0)return({Cyc_Tcutil_rsubstitute(rgn,inst,r);});else{
return t;}}case 0U: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp507)->f2 == 0){_LL11: _tmp50C=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp507)->f1;_LL12: {void*c=_tmp50C;
return t;}}else{_LL13: _tmp50A=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp507)->f1;_tmp50B=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp507)->f2;_LL14: {void*c=_tmp50A;struct Cyc_List_List*ts=_tmp50B;
# 3002
struct Cyc_List_List*new_ts=({Cyc_Tcutil_substs(rgn,inst,ts);});
if(ts == new_ts)return t;else{
return(void*)({struct Cyc_Absyn_AppType_Absyn_Type_struct*_tmp551=_cycalloc(sizeof(*_tmp551));_tmp551->tag=0U,_tmp551->f1=c,_tmp551->f2=new_ts;_tmp551;});}}}case 9U: _LL15: _tmp509=((struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_tmp507)->f1;_LL16: {struct Cyc_Absyn_Exp*e=_tmp509;
# 3006
struct Cyc_Absyn_Exp*new_e=({Cyc_Tcutil_rsubsexp(rgn,inst,e);});
return new_e == e?t:(void*)({struct Cyc_Absyn_ValueofType_Absyn_Type_struct*_tmp552=_cycalloc(sizeof(*_tmp552));_tmp552->tag=9U,_tmp552->f1=new_e;_tmp552;});}case 11U: _LL17: _tmp508=((struct Cyc_Absyn_TypeofType_Absyn_Type_struct*)_tmp507)->f1;_LL18: {struct Cyc_Absyn_Exp*e=_tmp508;
# 3009
struct Cyc_Absyn_Exp*new_e=({Cyc_Tcutil_rsubsexp(rgn,inst,e);});
return new_e == e?t:(void*)({struct Cyc_Absyn_TypeofType_Absyn_Type_struct*_tmp553=_cycalloc(sizeof(*_tmp553));_tmp553->tag=11U,_tmp553->f1=new_e;_tmp553;});}default: _LL19: _LL1A:
({void*_tmp554=0U;({int(*_tmp823)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp556)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Tcutil_impos;_tmp556;});struct _fat_ptr _tmp822=({const char*_tmp555="found typedecltype in rsubs";_tag_fat(_tmp555,sizeof(char),28U);});_tmp823(_tmp822,_tag_fat(_tmp554,sizeof(void*),0U));});});}_LL0:;}
# 3037 "tcutil.cyc"
static struct Cyc_List_List*Cyc_Tcutil_substs(struct _RegionHandle*rgn,struct Cyc_List_List*inst,struct Cyc_List_List*ts){
# 3040
if(ts == 0)
return 0;{
# 3040
void*old_hd=(void*)ts->hd;
# 3043
struct Cyc_List_List*old_tl=ts->tl;
void*new_hd=({Cyc_Tcutil_rsubstitute(rgn,inst,old_hd);});
struct Cyc_List_List*new_tl=({Cyc_Tcutil_substs(rgn,inst,old_tl);});
if(old_hd == new_hd && old_tl == new_tl)
return ts;
# 3046
return({struct Cyc_List_List*_tmp558=_cycalloc(sizeof(*_tmp558));
# 3048
_tmp558->hd=new_hd,_tmp558->tl=new_tl;_tmp558;});}}
# 3051
extern void*Cyc_Tcutil_substitute(struct Cyc_List_List*inst,void*t){
if(inst != 0)
return({Cyc_Tcutil_rsubstitute(Cyc_Core_heap_region,inst,t);});else{
return t;}}
# 3058
struct _tuple19*Cyc_Tcutil_make_inst_var(struct Cyc_List_List*s,struct Cyc_Absyn_Tvar*tv){
struct Cyc_Core_Opt*k=({struct Cyc_Core_Opt*(*_tmp55D)(struct Cyc_Absyn_Kind*ka)=Cyc_Tcutil_kind_to_opt;struct Cyc_Absyn_Kind*_tmp55E=({Cyc_Tcutil_tvar_kind(tv,& Cyc_Tcutil_bk);});_tmp55D(_tmp55E);});
return({struct _tuple19*_tmp55C=_cycalloc(sizeof(*_tmp55C));_tmp55C->f1=tv,({void*_tmp825=({({struct Cyc_Core_Opt*_tmp824=k;Cyc_Absyn_new_evar(_tmp824,({struct Cyc_Core_Opt*_tmp55B=_cycalloc(sizeof(*_tmp55B));_tmp55B->v=s;_tmp55B;}));});});_tmp55C->f2=_tmp825;});_tmp55C;});}struct _tuple28{struct Cyc_List_List*f1;struct _RegionHandle*f2;};
# 3063
struct _tuple19*Cyc_Tcutil_r_make_inst_var(struct _tuple28*env,struct Cyc_Absyn_Tvar*tv){
# 3065
struct _RegionHandle*_tmp561;struct Cyc_List_List*_tmp560;_LL1: _tmp560=env->f1;_tmp561=env->f2;_LL2: {struct Cyc_List_List*s=_tmp560;struct _RegionHandle*rgn=_tmp561;
struct Cyc_Core_Opt*k=({struct Cyc_Core_Opt*(*_tmp564)(struct Cyc_Absyn_Kind*ka)=Cyc_Tcutil_kind_to_opt;struct Cyc_Absyn_Kind*_tmp565=({Cyc_Tcutil_tvar_kind(tv,& Cyc_Tcutil_bk);});_tmp564(_tmp565);});
return({struct _tuple19*_tmp563=_region_malloc(rgn,sizeof(*_tmp563));_tmp563->f1=tv,({void*_tmp827=({({struct Cyc_Core_Opt*_tmp826=k;Cyc_Absyn_new_evar(_tmp826,({struct Cyc_Core_Opt*_tmp562=_cycalloc(sizeof(*_tmp562));_tmp562->v=s;_tmp562;}));});});_tmp563->f2=_tmp827;});_tmp563;});}}
# 3070
void Cyc_Tcutil_check_bitfield(unsigned loc,void*field_type,struct Cyc_Absyn_Exp*width,struct _fat_ptr*fn){
# 3072
if(width != 0){
unsigned w=0U;
if(!({Cyc_Tcutil_is_const_exp(width);}))
({struct Cyc_String_pa_PrintArg_struct _tmp569=({struct Cyc_String_pa_PrintArg_struct _tmp6E4;_tmp6E4.tag=0U,_tmp6E4.f1=(struct _fat_ptr)((struct _fat_ptr)*fn);_tmp6E4;});void*_tmp567[1U];_tmp567[0]=& _tmp569;({unsigned _tmp829=loc;struct _fat_ptr _tmp828=({const char*_tmp568="bitfield %s does not have constant width";_tag_fat(_tmp568,sizeof(char),41U);});Cyc_Tcutil_terr(_tmp829,_tmp828,_tag_fat(_tmp567,sizeof(void*),1U));});});else{
# 3077
struct _tuple13 _stmttmp6C=({Cyc_Evexp_eval_const_uint_exp(width);});int _tmp56B;unsigned _tmp56A;_LL1: _tmp56A=_stmttmp6C.f1;_tmp56B=_stmttmp6C.f2;_LL2: {unsigned i=_tmp56A;int known=_tmp56B;
if(!known)
({void*_tmp56C=0U;({unsigned _tmp82B=loc;struct _fat_ptr _tmp82A=({const char*_tmp56D="cannot evaluate bitfield width at compile time";_tag_fat(_tmp56D,sizeof(char),47U);});Cyc_Tcutil_warn(_tmp82B,_tmp82A,_tag_fat(_tmp56C,sizeof(void*),0U));});});
# 3078
if((int)i < 0)
# 3081
({void*_tmp56E=0U;({unsigned _tmp82D=loc;struct _fat_ptr _tmp82C=({const char*_tmp56F="bitfield has negative width";_tag_fat(_tmp56F,sizeof(char),28U);});Cyc_Tcutil_terr(_tmp82D,_tmp82C,_tag_fat(_tmp56E,sizeof(void*),0U));});});
# 3078
w=i;}}{
# 3084
void*_stmttmp6D=({Cyc_Tcutil_compress(field_type);});void*_tmp570=_stmttmp6D;enum Cyc_Absyn_Size_of _tmp571;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp570)->tag == 0U){if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp570)->f1)->tag == 1U){_LL4: _tmp571=((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp570)->f1)->f2;_LL5: {enum Cyc_Absyn_Size_of b=_tmp571;
# 3087
{enum Cyc_Absyn_Size_of _tmp572=b;switch(_tmp572){case Cyc_Absyn_Char_sz: _LL9: _LLA:
 if(w > (unsigned)8)({void*_tmp573=0U;({unsigned _tmp82F=loc;struct _fat_ptr _tmp82E=({const char*_tmp574="bitfield larger than type";_tag_fat(_tmp574,sizeof(char),26U);});Cyc_Tcutil_warn(_tmp82F,_tmp82E,_tag_fat(_tmp573,sizeof(void*),0U));});});goto _LL8;case Cyc_Absyn_Short_sz: _LLB: _LLC:
 if(w > (unsigned)16)({void*_tmp575=0U;({unsigned _tmp831=loc;struct _fat_ptr _tmp830=({const char*_tmp576="bitfield larger than type";_tag_fat(_tmp576,sizeof(char),26U);});Cyc_Tcutil_warn(_tmp831,_tmp830,_tag_fat(_tmp575,sizeof(void*),0U));});});goto _LL8;case Cyc_Absyn_Long_sz: _LLD: _LLE:
 goto _LL10;case Cyc_Absyn_Int_sz: _LLF: _LL10:
 if(w > (unsigned)32)({void*_tmp577=0U;({unsigned _tmp833=loc;struct _fat_ptr _tmp832=({const char*_tmp578="bitfield larger than type";_tag_fat(_tmp578,sizeof(char),26U);});Cyc_Tcutil_warn(_tmp833,_tmp832,_tag_fat(_tmp577,sizeof(void*),0U));});});goto _LL8;case Cyc_Absyn_LongLong_sz: _LL11: _LL12:
 goto _LL14;default: _LL13: _LL14:
 if(w > (unsigned)64)({void*_tmp579=0U;({unsigned _tmp835=loc;struct _fat_ptr _tmp834=({const char*_tmp57A="bitfield larger than type";_tag_fat(_tmp57A,sizeof(char),26U);});Cyc_Tcutil_warn(_tmp835,_tmp834,_tag_fat(_tmp579,sizeof(void*),0U));});});goto _LL8;}_LL8:;}
# 3095
goto _LL3;}}else{goto _LL6;}}else{_LL6: _LL7:
# 3097
({struct Cyc_String_pa_PrintArg_struct _tmp57D=({struct Cyc_String_pa_PrintArg_struct _tmp6E6;_tmp6E6.tag=0U,_tmp6E6.f1=(struct _fat_ptr)((struct _fat_ptr)*fn);_tmp6E6;});struct Cyc_String_pa_PrintArg_struct _tmp57E=({struct Cyc_String_pa_PrintArg_struct _tmp6E5;_tmp6E5.tag=0U,({
struct _fat_ptr _tmp836=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(field_type);}));_tmp6E5.f1=_tmp836;});_tmp6E5;});void*_tmp57B[2U];_tmp57B[0]=& _tmp57D,_tmp57B[1]=& _tmp57E;({unsigned _tmp838=loc;struct _fat_ptr _tmp837=({const char*_tmp57C="bitfield %s must have integral type but has type %s";_tag_fat(_tmp57C,sizeof(char),52U);});Cyc_Tcutil_terr(_tmp838,_tmp837,_tag_fat(_tmp57B,sizeof(void*),2U));});});
goto _LL3;}_LL3:;}}}
# 3070
int Cyc_Tcutil_extract_const_from_typedef(unsigned loc,int declared_const,void*t){
# 3107
void*_tmp580=t;void*_tmp582;struct Cyc_Absyn_Typedefdecl*_tmp581;if(((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp580)->tag == 8U){_LL1: _tmp581=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp580)->f3;_tmp582=(void*)((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp580)->f4;_LL2: {struct Cyc_Absyn_Typedefdecl*td=_tmp581;void*tdopt=_tmp582;
# 3109
if((((struct Cyc_Absyn_Typedefdecl*)_check_null(td))->tq).real_const ||(td->tq).print_const){
if(declared_const)({void*_tmp583=0U;({unsigned _tmp83A=loc;struct _fat_ptr _tmp839=({const char*_tmp584="extra const";_tag_fat(_tmp584,sizeof(char),12U);});Cyc_Tcutil_warn(_tmp83A,_tmp839,_tag_fat(_tmp583,sizeof(void*),0U));});});return 1;}
# 3109
if((unsigned)tdopt)
# 3115
return({Cyc_Tcutil_extract_const_from_typedef(loc,declared_const,tdopt);});else{
return declared_const;}}}else{_LL3: _LL4:
 return declared_const;}_LL0:;}
# 3121
void Cyc_Tcutil_add_tvar_identity(struct Cyc_Absyn_Tvar*tv){
if(tv->identity == - 1)
({int _tmp83B=({Cyc_Tcutil_new_tvar_id();});tv->identity=_tmp83B;});}
# 3125
void Cyc_Tcutil_add_tvar_identities(struct Cyc_List_List*tvs){
({({void(*_tmp83C)(void(*f)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*x)=({void(*_tmp587)(void(*f)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*x)=(void(*)(void(*f)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*x))Cyc_List_iter;_tmp587;});_tmp83C(Cyc_Tcutil_add_tvar_identity,tvs);});});}
# 3131
static void Cyc_Tcutil_check_unique_unsorted(int(*cmp)(void*,void*),struct Cyc_List_List*vs,unsigned loc,struct _fat_ptr(*a2string)(void*),struct _fat_ptr msg){
# 3134
for(0;vs != 0;vs=vs->tl){
struct Cyc_List_List*vs2=vs->tl;for(0;vs2 != 0;vs2=vs2->tl){
if(({cmp(vs->hd,vs2->hd);})== 0)
({struct Cyc_String_pa_PrintArg_struct _tmp58B=({struct Cyc_String_pa_PrintArg_struct _tmp6E8;_tmp6E8.tag=0U,_tmp6E8.f1=(struct _fat_ptr)((struct _fat_ptr)msg);_tmp6E8;});struct Cyc_String_pa_PrintArg_struct _tmp58C=({struct Cyc_String_pa_PrintArg_struct _tmp6E7;_tmp6E7.tag=0U,({struct _fat_ptr _tmp83D=(struct _fat_ptr)((struct _fat_ptr)({a2string(vs->hd);}));_tmp6E7.f1=_tmp83D;});_tmp6E7;});void*_tmp589[2U];_tmp589[0]=& _tmp58B,_tmp589[1]=& _tmp58C;({unsigned _tmp83F=loc;struct _fat_ptr _tmp83E=({const char*_tmp58A="%s: %s";_tag_fat(_tmp58A,sizeof(char),7U);});Cyc_Tcutil_terr(_tmp83F,_tmp83E,_tag_fat(_tmp589,sizeof(void*),2U));});});}}}
# 3140
static struct _fat_ptr Cyc_Tcutil_strptr2string(struct _fat_ptr*s){
return*s;}
# 3144
void Cyc_Tcutil_check_unique_vars(struct Cyc_List_List*vs,unsigned loc,struct _fat_ptr msg){
({({void(*_tmp842)(int(*cmp)(struct _fat_ptr*,struct _fat_ptr*),struct Cyc_List_List*vs,unsigned loc,struct _fat_ptr(*a2string)(struct _fat_ptr*),struct _fat_ptr msg)=({void(*_tmp58F)(int(*cmp)(struct _fat_ptr*,struct _fat_ptr*),struct Cyc_List_List*vs,unsigned loc,struct _fat_ptr(*a2string)(struct _fat_ptr*),struct _fat_ptr msg)=(void(*)(int(*cmp)(struct _fat_ptr*,struct _fat_ptr*),struct Cyc_List_List*vs,unsigned loc,struct _fat_ptr(*a2string)(struct _fat_ptr*),struct _fat_ptr msg))Cyc_Tcutil_check_unique_unsorted;_tmp58F;});struct Cyc_List_List*_tmp841=vs;unsigned _tmp840=loc;_tmp842(Cyc_strptrcmp,_tmp841,_tmp840,Cyc_Tcutil_strptr2string,msg);});});}
# 3148
void Cyc_Tcutil_check_unique_tvars(unsigned loc,struct Cyc_List_List*tvs){
({({void(*_tmp845)(int(*cmp)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_List_List*vs,unsigned loc,struct _fat_ptr(*a2string)(struct Cyc_Absyn_Tvar*),struct _fat_ptr msg)=({void(*_tmp591)(int(*cmp)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_List_List*vs,unsigned loc,struct _fat_ptr(*a2string)(struct Cyc_Absyn_Tvar*),struct _fat_ptr msg)=(void(*)(int(*cmp)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_List_List*vs,unsigned loc,struct _fat_ptr(*a2string)(struct Cyc_Absyn_Tvar*),struct _fat_ptr msg))Cyc_Tcutil_check_unique_unsorted;_tmp591;});struct Cyc_List_List*_tmp844=tvs;unsigned _tmp843=loc;_tmp845(Cyc_Absyn_tvar_cmp,_tmp844,_tmp843,Cyc_Absynpp_tvar2string,({const char*_tmp592="duplicate type variable";_tag_fat(_tmp592,sizeof(char),24U);}));});});}struct _tuple29{struct Cyc_Absyn_Aggrfield*f1;int f2;};struct _tuple30{struct Cyc_List_List*f1;void*f2;};struct _tuple31{struct Cyc_Absyn_Aggrfield*f1;void*f2;};
# 3163 "tcutil.cyc"
struct Cyc_List_List*Cyc_Tcutil_resolve_aggregate_designators(struct _RegionHandle*rgn,unsigned loc,struct Cyc_List_List*des,enum Cyc_Absyn_AggrKind aggr_kind,struct Cyc_List_List*sdfields){
# 3167
struct _RegionHandle _tmp594=_new_region("temp");struct _RegionHandle*temp=& _tmp594;_push_region(temp);
# 3171
{struct Cyc_List_List*fields=0;
{struct Cyc_List_List*sd_fields=sdfields;for(0;sd_fields != 0;sd_fields=sd_fields->tl){
if(({({struct _fat_ptr _tmp846=(struct _fat_ptr)*((struct Cyc_Absyn_Aggrfield*)sd_fields->hd)->name;Cyc_strcmp(_tmp846,({const char*_tmp595="";_tag_fat(_tmp595,sizeof(char),1U);}));});})!= 0)
fields=({struct Cyc_List_List*_tmp597=_region_malloc(temp,sizeof(*_tmp597));({struct _tuple29*_tmp847=({struct _tuple29*_tmp596=_region_malloc(temp,sizeof(*_tmp596));_tmp596->f1=(struct Cyc_Absyn_Aggrfield*)sd_fields->hd,_tmp596->f2=0;_tmp596;});_tmp597->hd=_tmp847;}),_tmp597->tl=fields;_tmp597;});}}
# 3176
fields=({Cyc_List_imp_rev(fields);});{
# 3178
struct _fat_ptr aggr_str=(int)aggr_kind == (int)0U?({const char*_tmp5B7="struct";_tag_fat(_tmp5B7,sizeof(char),7U);}):({const char*_tmp5B8="union";_tag_fat(_tmp5B8,sizeof(char),6U);});
# 3181
struct Cyc_List_List*ans=0;
for(0;des != 0;des=des->tl){
struct _tuple30*_stmttmp6E=(struct _tuple30*)des->hd;void*_tmp599;struct Cyc_List_List*_tmp598;_LL1: _tmp598=_stmttmp6E->f1;_tmp599=_stmttmp6E->f2;_LL2: {struct Cyc_List_List*dl=_tmp598;void*a=_tmp599;
if(dl == 0){
# 3186
struct Cyc_List_List*fields2=fields;
for(0;fields2 != 0;fields2=fields2->tl){
if(!(*((struct _tuple29*)fields2->hd)).f2){
(*((struct _tuple29*)fields2->hd)).f2=1;
({struct Cyc_List_List*_tmp849=({struct Cyc_List_List*_tmp59B=_cycalloc(sizeof(*_tmp59B));({void*_tmp848=(void*)({struct Cyc_Absyn_FieldName_Absyn_Designator_struct*_tmp59A=_cycalloc(sizeof(*_tmp59A));_tmp59A->tag=1U,_tmp59A->f1=((*((struct _tuple29*)fields2->hd)).f1)->name;_tmp59A;});_tmp59B->hd=_tmp848;}),_tmp59B->tl=0;_tmp59B;});(*((struct _tuple30*)des->hd)).f1=_tmp849;});
ans=({struct Cyc_List_List*_tmp59D=_region_malloc(rgn,sizeof(*_tmp59D));({struct _tuple31*_tmp84A=({struct _tuple31*_tmp59C=_region_malloc(rgn,sizeof(*_tmp59C));_tmp59C->f1=(*((struct _tuple29*)fields2->hd)).f1,_tmp59C->f2=a;_tmp59C;});_tmp59D->hd=_tmp84A;}),_tmp59D->tl=ans;_tmp59D;});
break;}}
# 3187
if(fields2 == 0)
# 3195
({struct Cyc_String_pa_PrintArg_struct _tmp5A0=({struct Cyc_String_pa_PrintArg_struct _tmp6E9;_tmp6E9.tag=0U,_tmp6E9.f1=(struct _fat_ptr)((struct _fat_ptr)aggr_str);_tmp6E9;});void*_tmp59E[1U];_tmp59E[0]=& _tmp5A0;({unsigned _tmp84C=loc;struct _fat_ptr _tmp84B=({const char*_tmp59F="too many arguments to %s";_tag_fat(_tmp59F,sizeof(char),25U);});Cyc_Tcutil_terr(_tmp84C,_tmp84B,_tag_fat(_tmp59E,sizeof(void*),1U));});});}else{
if(dl->tl != 0)
# 3198
({void*_tmp5A1=0U;({unsigned _tmp84E=loc;struct _fat_ptr _tmp84D=({const char*_tmp5A2="multiple designators are not yet supported";_tag_fat(_tmp5A2,sizeof(char),43U);});Cyc_Tcutil_terr(_tmp84E,_tmp84D,_tag_fat(_tmp5A1,sizeof(void*),0U));});});else{
# 3201
void*_stmttmp6F=(void*)dl->hd;void*_tmp5A3=_stmttmp6F;struct _fat_ptr*_tmp5A4;if(((struct Cyc_Absyn_ArrayElement_Absyn_Designator_struct*)_tmp5A3)->tag == 0U){_LL4: _LL5:
# 3203
({struct Cyc_String_pa_PrintArg_struct _tmp5A7=({struct Cyc_String_pa_PrintArg_struct _tmp6EA;_tmp6EA.tag=0U,_tmp6EA.f1=(struct _fat_ptr)((struct _fat_ptr)aggr_str);_tmp6EA;});void*_tmp5A5[1U];_tmp5A5[0]=& _tmp5A7;({unsigned _tmp850=loc;struct _fat_ptr _tmp84F=({const char*_tmp5A6="array designator used in argument to %s";_tag_fat(_tmp5A6,sizeof(char),40U);});Cyc_Tcutil_terr(_tmp850,_tmp84F,_tag_fat(_tmp5A5,sizeof(void*),1U));});});
goto _LL3;}else{_LL6: _tmp5A4=((struct Cyc_Absyn_FieldName_Absyn_Designator_struct*)_tmp5A3)->f1;_LL7: {struct _fat_ptr*v=_tmp5A4;
# 3206
struct Cyc_List_List*fields2=fields;
for(0;fields2 != 0;fields2=fields2->tl){
if(({Cyc_strptrcmp(v,((*((struct _tuple29*)fields2->hd)).f1)->name);})== 0){
if((*((struct _tuple29*)fields2->hd)).f2)
({struct Cyc_String_pa_PrintArg_struct _tmp5AA=({struct Cyc_String_pa_PrintArg_struct _tmp6EB;_tmp6EB.tag=0U,_tmp6EB.f1=(struct _fat_ptr)((struct _fat_ptr)*v);_tmp6EB;});void*_tmp5A8[1U];_tmp5A8[0]=& _tmp5AA;({unsigned _tmp852=loc;struct _fat_ptr _tmp851=({const char*_tmp5A9="member %s has already been used as an argument";_tag_fat(_tmp5A9,sizeof(char),47U);});Cyc_Tcutil_terr(_tmp852,_tmp851,_tag_fat(_tmp5A8,sizeof(void*),1U));});});
# 3209
(*((struct _tuple29*)fields2->hd)).f2=1;
# 3212
ans=({struct Cyc_List_List*_tmp5AC=_region_malloc(rgn,sizeof(*_tmp5AC));({struct _tuple31*_tmp853=({struct _tuple31*_tmp5AB=_region_malloc(rgn,sizeof(*_tmp5AB));_tmp5AB->f1=(*((struct _tuple29*)fields2->hd)).f1,_tmp5AB->f2=a;_tmp5AB;});_tmp5AC->hd=_tmp853;}),_tmp5AC->tl=ans;_tmp5AC;});
break;}}
# 3207
if(fields2 == 0)
# 3216
({struct Cyc_String_pa_PrintArg_struct _tmp5AF=({struct Cyc_String_pa_PrintArg_struct _tmp6EC;_tmp6EC.tag=0U,_tmp6EC.f1=(struct _fat_ptr)((struct _fat_ptr)*v);_tmp6EC;});void*_tmp5AD[1U];_tmp5AD[0]=& _tmp5AF;({unsigned _tmp855=loc;struct _fat_ptr _tmp854=({const char*_tmp5AE="bad field designator %s";_tag_fat(_tmp5AE,sizeof(char),24U);});Cyc_Tcutil_terr(_tmp855,_tmp854,_tag_fat(_tmp5AD,sizeof(void*),1U));});});
# 3207
goto _LL3;}}_LL3:;}}}}
# 3220
if((int)aggr_kind == (int)0U)
# 3222
for(0;fields != 0;fields=fields->tl){
if(!(*((struct _tuple29*)fields->hd)).f2){
({void*_tmp5B0=0U;({unsigned _tmp857=loc;struct _fat_ptr _tmp856=({const char*_tmp5B1="too few arguments to struct";_tag_fat(_tmp5B1,sizeof(char),28U);});Cyc_Tcutil_terr(_tmp857,_tmp856,_tag_fat(_tmp5B0,sizeof(void*),0U));});});
break;}}else{
# 3229
int found=0;
for(0;fields != 0;fields=fields->tl){
if((*((struct _tuple29*)fields->hd)).f2){
if(found)({void*_tmp5B2=0U;({unsigned _tmp859=loc;struct _fat_ptr _tmp858=({const char*_tmp5B3="only one member of a union is allowed";_tag_fat(_tmp5B3,sizeof(char),38U);});Cyc_Tcutil_terr(_tmp859,_tmp858,_tag_fat(_tmp5B2,sizeof(void*),0U));});});found=1;}}
# 3230
if(!found)
# 3235
({void*_tmp5B4=0U;({unsigned _tmp85B=loc;struct _fat_ptr _tmp85A=({const char*_tmp5B5="missing member for union";_tag_fat(_tmp5B5,sizeof(char),25U);});Cyc_Tcutil_terr(_tmp85B,_tmp85A,_tag_fat(_tmp5B4,sizeof(void*),0U));});});}{
# 3238
struct Cyc_List_List*_tmp5B6=({Cyc_List_imp_rev(ans);});_npop_handler(0U);return _tmp5B6;}}}
# 3171
;_pop_region();}
# 3163
int Cyc_Tcutil_is_zero_ptr_deref(struct Cyc_Absyn_Exp*e1,void**ptr_type,int*is_fat,void**elt_type){
# 3246
void*_stmttmp70=e1->r;void*_tmp5BA=_stmttmp70;struct Cyc_Absyn_Exp*_tmp5BB;struct Cyc_Absyn_Exp*_tmp5BC;struct Cyc_Absyn_Exp*_tmp5BD;struct Cyc_Absyn_Exp*_tmp5BE;struct Cyc_Absyn_Exp*_tmp5BF;struct Cyc_Absyn_Exp*_tmp5C0;switch(*((int*)_tmp5BA)){case 14U: _LL1: _LL2:
({struct Cyc_String_pa_PrintArg_struct _tmp5C4=({struct Cyc_String_pa_PrintArg_struct _tmp6ED;_tmp6ED.tag=0U,({struct _fat_ptr _tmp85C=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_exp2string(e1);}));_tmp6ED.f1=_tmp85C;});_tmp6ED;});void*_tmp5C1[1U];_tmp5C1[0]=& _tmp5C4;({int(*_tmp85E)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp5C3)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Tcutil_impos;_tmp5C3;});struct _fat_ptr _tmp85D=({const char*_tmp5C2="we have a cast in a lhs:  %s";_tag_fat(_tmp5C2,sizeof(char),29U);});_tmp85E(_tmp85D,_tag_fat(_tmp5C1,sizeof(void*),1U));});});case 20U: _LL3: _tmp5C0=((struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_tmp5BA)->f1;_LL4: {struct Cyc_Absyn_Exp*e1a=_tmp5C0;
_tmp5BF=e1a;goto _LL6;}case 23U: _LL5: _tmp5BF=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp5BA)->f1;_LL6: {struct Cyc_Absyn_Exp*e1a=_tmp5BF;
# 3250
return({Cyc_Tcutil_is_zero_ptr_type((void*)_check_null(e1a->topt),ptr_type,is_fat,elt_type);});}case 22U: _LL7: _tmp5BE=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp5BA)->f1;_LL8: {struct Cyc_Absyn_Exp*e1a=_tmp5BE;
_tmp5BD=e1a;goto _LLA;}case 21U: _LL9: _tmp5BD=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp5BA)->f1;_LLA: {struct Cyc_Absyn_Exp*e1a=_tmp5BD;
# 3254
if(({Cyc_Tcutil_is_zero_ptr_type((void*)_check_null(e1a->topt),ptr_type,is_fat,elt_type);}))
({struct Cyc_String_pa_PrintArg_struct _tmp5C8=({struct Cyc_String_pa_PrintArg_struct _tmp6EE;_tmp6EE.tag=0U,({
struct _fat_ptr _tmp85F=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_exp2string(e1);}));_tmp6EE.f1=_tmp85F;});_tmp6EE;});void*_tmp5C5[1U];_tmp5C5[0]=& _tmp5C8;({int(*_tmp861)(struct _fat_ptr fmt,struct _fat_ptr ap)=({
# 3255
int(*_tmp5C7)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Tcutil_impos;_tmp5C7;});struct _fat_ptr _tmp860=({const char*_tmp5C6="found zero pointer aggregate member assignment: %s";_tag_fat(_tmp5C6,sizeof(char),51U);});_tmp861(_tmp860,_tag_fat(_tmp5C5,sizeof(void*),1U));});});
# 3254
return 0;}case 13U: _LLB: _tmp5BC=((struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*)_tmp5BA)->f1;_LLC: {struct Cyc_Absyn_Exp*e1a=_tmp5BC;
# 3258
_tmp5BB=e1a;goto _LLE;}case 12U: _LLD: _tmp5BB=((struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*)_tmp5BA)->f1;_LLE: {struct Cyc_Absyn_Exp*e1a=_tmp5BB;
# 3260
if(({Cyc_Tcutil_is_zero_ptr_type((void*)_check_null(e1a->topt),ptr_type,is_fat,elt_type);}))
({struct Cyc_String_pa_PrintArg_struct _tmp5CC=({struct Cyc_String_pa_PrintArg_struct _tmp6EF;_tmp6EF.tag=0U,({
struct _fat_ptr _tmp862=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_exp2string(e1);}));_tmp6EF.f1=_tmp862;});_tmp6EF;});void*_tmp5C9[1U];_tmp5C9[0]=& _tmp5CC;({int(*_tmp864)(struct _fat_ptr fmt,struct _fat_ptr ap)=({
# 3261
int(*_tmp5CB)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Tcutil_impos;_tmp5CB;});struct _fat_ptr _tmp863=({const char*_tmp5CA="found zero pointer instantiate/noinstantiate: %s";_tag_fat(_tmp5CA,sizeof(char),49U);});_tmp864(_tmp863,_tag_fat(_tmp5C9,sizeof(void*),1U));});});
# 3260
return 0;}case 1U: _LLF: _LL10:
# 3264
 return 0;default: _LL11: _LL12:
({struct Cyc_String_pa_PrintArg_struct _tmp5D0=({struct Cyc_String_pa_PrintArg_struct _tmp6F0;_tmp6F0.tag=0U,({struct _fat_ptr _tmp865=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_exp2string(e1);}));_tmp6F0.f1=_tmp865;});_tmp6F0;});void*_tmp5CD[1U];_tmp5CD[0]=& _tmp5D0;({int(*_tmp867)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp5CF)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Tcutil_impos;_tmp5CF;});struct _fat_ptr _tmp866=({const char*_tmp5CE="found bad lhs in is_zero_ptr_deref: %s";_tag_fat(_tmp5CE,sizeof(char),39U);});_tmp867(_tmp866,_tag_fat(_tmp5CD,sizeof(void*),1U));});});}_LL0:;}
# 3163
int Cyc_Tcutil_is_noalias_region(void*r,int must_be_unique){
# 3278
void*_stmttmp71=({Cyc_Tcutil_compress(r);});void*_tmp5D2=_stmttmp71;struct Cyc_Absyn_Tvar*_tmp5D3;enum Cyc_Absyn_AliasQual _tmp5D5;enum Cyc_Absyn_KindQual _tmp5D4;switch(*((int*)_tmp5D2)){case 0U: switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp5D2)->f1)){case 8U: _LL1: _LL2:
 return !must_be_unique;case 7U: _LL3: _LL4:
 return 1;default: goto _LL9;}case 8U: if(((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp5D2)->f3 != 0){if(((struct Cyc_Absyn_Typedefdecl*)((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp5D2)->f3)->kind != 0){if(((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp5D2)->f4 == 0){_LL5: _tmp5D4=((struct Cyc_Absyn_Kind*)((((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp5D2)->f3)->kind)->v)->kind;_tmp5D5=((struct Cyc_Absyn_Kind*)((((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp5D2)->f3)->kind)->v)->aliasqual;_LL6: {enum Cyc_Absyn_KindQual k=_tmp5D4;enum Cyc_Absyn_AliasQual a=_tmp5D5;
# 3282
return(int)k == (int)4U &&((int)a == (int)1U ||(int)a == (int)2U && !must_be_unique);}}else{goto _LL9;}}else{goto _LL9;}}else{goto _LL9;}case 2U: _LL7: _tmp5D3=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp5D2)->f1;_LL8: {struct Cyc_Absyn_Tvar*tv=_tmp5D3;
# 3286
struct Cyc_Absyn_Kind*_stmttmp72=({Cyc_Tcutil_tvar_kind(tv,& Cyc_Tcutil_rk);});enum Cyc_Absyn_AliasQual _tmp5D7;enum Cyc_Absyn_KindQual _tmp5D6;_LLC: _tmp5D6=_stmttmp72->kind;_tmp5D7=_stmttmp72->aliasqual;_LLD: {enum Cyc_Absyn_KindQual k=_tmp5D6;enum Cyc_Absyn_AliasQual a=_tmp5D7;
if((int)k == (int)4U &&((int)a == (int)1U ||(int)a == (int)2U && !must_be_unique)){
void*_stmttmp73=({Cyc_Absyn_compress_kb(tv->kind);});void*_tmp5D8=_stmttmp73;struct Cyc_Core_Opt**_tmp5D9;if(((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp5D8)->tag == 2U){if(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp5D8)->f2)->kind == Cyc_Absyn_RgnKind){if(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp5D8)->f2)->aliasqual == Cyc_Absyn_Top){_LLF: _tmp5D9=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp5D8)->f1;_LL10: {struct Cyc_Core_Opt**x=_tmp5D9;
# 3290
({struct Cyc_Core_Opt*_tmp869=({struct Cyc_Core_Opt*_tmp5DB=_cycalloc(sizeof(*_tmp5DB));({void*_tmp868=(void*)({struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*_tmp5DA=_cycalloc(sizeof(*_tmp5DA));_tmp5DA->tag=2U,_tmp5DA->f1=0,_tmp5DA->f2=& Cyc_Tcutil_rk;_tmp5DA;});_tmp5DB->v=_tmp868;});_tmp5DB;});*x=_tmp869;});
return 0;}}else{goto _LL11;}}else{goto _LL11;}}else{_LL11: _LL12:
 return 1;}_LLE:;}
# 3287
return 0;}}default: _LL9: _LLA:
# 3296
 return 0;}_LL0:;}
# 3163
int Cyc_Tcutil_is_noalias_pointer(void*t,int must_be_unique){
# 3303
void*_stmttmp74=({Cyc_Tcutil_compress(t);});void*_tmp5DD=_stmttmp74;struct Cyc_Absyn_Tvar*_tmp5DE;void*_tmp5DF;switch(*((int*)_tmp5DD)){case 3U: _LL1: _tmp5DF=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp5DD)->f1).ptr_atts).rgn;_LL2: {void*r=_tmp5DF;
# 3305
return({Cyc_Tcutil_is_noalias_region(r,must_be_unique);});}case 2U: _LL3: _tmp5DE=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp5DD)->f1;_LL4: {struct Cyc_Absyn_Tvar*tv=_tmp5DE;
# 3307
struct Cyc_Absyn_Kind*_stmttmp75=({Cyc_Tcutil_tvar_kind(tv,& Cyc_Tcutil_bk);});enum Cyc_Absyn_AliasQual _tmp5E1;enum Cyc_Absyn_KindQual _tmp5E0;_LL8: _tmp5E0=_stmttmp75->kind;_tmp5E1=_stmttmp75->aliasqual;_LL9: {enum Cyc_Absyn_KindQual k=_tmp5E0;enum Cyc_Absyn_AliasQual a=_tmp5E1;
enum Cyc_Absyn_KindQual _tmp5E2=k;switch(_tmp5E2){case Cyc_Absyn_BoxKind: _LLB: _LLC:
 goto _LLE;case Cyc_Absyn_AnyKind: _LLD: _LLE: goto _LL10;case Cyc_Absyn_MemKind: _LLF: _LL10:
 if((int)a == (int)1U ||(int)a == (int)2U && !must_be_unique){
void*_stmttmp76=({Cyc_Absyn_compress_kb(tv->kind);});void*_tmp5E3=_stmttmp76;enum Cyc_Absyn_KindQual _tmp5E5;struct Cyc_Core_Opt**_tmp5E4;if(((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp5E3)->tag == 2U){if(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp5E3)->f2)->aliasqual == Cyc_Absyn_Top){_LL14: _tmp5E4=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp5E3)->f1;_tmp5E5=(((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp5E3)->f2)->kind;_LL15: {struct Cyc_Core_Opt**x=_tmp5E4;enum Cyc_Absyn_KindQual k=_tmp5E5;
# 3313
({struct Cyc_Core_Opt*_tmp86C=({struct Cyc_Core_Opt*_tmp5E8=_cycalloc(sizeof(*_tmp5E8));({void*_tmp86B=(void*)({struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*_tmp5E7=_cycalloc(sizeof(*_tmp5E7));_tmp5E7->tag=2U,_tmp5E7->f1=0,({struct Cyc_Absyn_Kind*_tmp86A=({struct Cyc_Absyn_Kind*_tmp5E6=_cycalloc(sizeof(*_tmp5E6));_tmp5E6->kind=k,_tmp5E6->aliasqual=Cyc_Absyn_Aliasable;_tmp5E6;});_tmp5E7->f2=_tmp86A;});_tmp5E7;});_tmp5E8->v=_tmp86B;});_tmp5E8;});*x=_tmp86C;});
return 0;}}else{goto _LL16;}}else{_LL16: _LL17:
# 3317
 return 1;}_LL13:;}
# 3310
return 0;default: _LL11: _LL12:
# 3321
 return 0;}_LLA:;}}default: _LL5: _LL6:
# 3323
 return 0;}_LL0:;}
# 3163
int Cyc_Tcutil_is_noalias_pointer_or_aggr(void*t0){
# 3327
void*t=({Cyc_Tcutil_compress(t0);});
if(({Cyc_Tcutil_is_noalias_pointer(t,0);}))return 1;{void*_tmp5EA=t;struct Cyc_List_List*_tmp5EB;struct Cyc_List_List*_tmp5ED;union Cyc_Absyn_DatatypeFieldInfo _tmp5EC;struct Cyc_List_List*_tmp5EF;union Cyc_Absyn_DatatypeInfo _tmp5EE;struct Cyc_List_List*_tmp5F1;struct Cyc_Absyn_Aggrdecl**_tmp5F0;struct Cyc_List_List*_tmp5F2;switch(*((int*)_tmp5EA)){case 6U: _LL1: _tmp5F2=((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp5EA)->f1;_LL2: {struct Cyc_List_List*qts=_tmp5F2;
# 3331
while(qts != 0){
if(({Cyc_Tcutil_is_noalias_pointer_or_aggr((*((struct _tuple12*)qts->hd)).f2);}))return 1;qts=qts->tl;}
# 3335
return 0;}case 0U: switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp5EA)->f1)){case 22U: if(((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp5EA)->f1)->f1).KnownAggr).tag == 2){_LL3: _tmp5F0=((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp5EA)->f1)->f1).KnownAggr).val;_tmp5F1=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp5EA)->f2;_LL4: {struct Cyc_Absyn_Aggrdecl**adp=_tmp5F0;struct Cyc_List_List*ts=_tmp5F1;
# 3337
if((*adp)->impl == 0)return 0;else{
# 3339
struct Cyc_List_List*inst=({Cyc_List_zip((*adp)->tvs,ts);});
struct Cyc_List_List*x=((struct Cyc_Absyn_AggrdeclImpl*)_check_null((*adp)->impl))->fields;
void*t;
while(x != 0){
t=inst == 0?((struct Cyc_Absyn_Aggrfield*)x->hd)->type:({Cyc_Tcutil_substitute(inst,((struct Cyc_Absyn_Aggrfield*)x->hd)->type);});
if(({Cyc_Tcutil_is_noalias_pointer_or_aggr(t);}))return 1;x=x->tl;}
# 3347
return 0;}}}else{_LL7: _LL8:
# 3357
 return 0;}case 20U: _LL9: _tmp5EE=((struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp5EA)->f1)->f1;_tmp5EF=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp5EA)->f2;_LLA: {union Cyc_Absyn_DatatypeInfo tinfo=_tmp5EE;struct Cyc_List_List*ts=_tmp5EF;
# 3359
union Cyc_Absyn_DatatypeInfo _tmp5F3=tinfo;struct Cyc_Core_Opt*_tmp5F5;struct Cyc_List_List*_tmp5F4;int _tmp5F7;struct _tuple1*_tmp5F6;if((_tmp5F3.UnknownDatatype).tag == 1){_LL10: _tmp5F6=((_tmp5F3.UnknownDatatype).val).name;_tmp5F7=((_tmp5F3.UnknownDatatype).val).is_extensible;_LL11: {struct _tuple1*nm=_tmp5F6;int isxt=_tmp5F7;
# 3362
return 0;}}else{_LL12: _tmp5F4=(*(_tmp5F3.KnownDatatype).val)->tvs;_tmp5F5=(*(_tmp5F3.KnownDatatype).val)->fields;_LL13: {struct Cyc_List_List*tvs=_tmp5F4;struct Cyc_Core_Opt*flds=_tmp5F5;
# 3365
return 0;}}_LLF:;}case 21U: _LLB: _tmp5EC=((struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp5EA)->f1)->f1;_tmp5ED=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp5EA)->f2;_LLC: {union Cyc_Absyn_DatatypeFieldInfo tinfo=_tmp5EC;struct Cyc_List_List*ts=_tmp5ED;
# 3368
union Cyc_Absyn_DatatypeFieldInfo _tmp5F8=tinfo;struct Cyc_Absyn_Datatypefield*_tmp5FA;struct Cyc_Absyn_Datatypedecl*_tmp5F9;if((_tmp5F8.UnknownDatatypefield).tag == 1){_LL15: _LL16:
# 3371
 return 0;}else{_LL17: _tmp5F9=((_tmp5F8.KnownDatatypefield).val).f1;_tmp5FA=((_tmp5F8.KnownDatatypefield).val).f2;_LL18: {struct Cyc_Absyn_Datatypedecl*td=_tmp5F9;struct Cyc_Absyn_Datatypefield*fld=_tmp5FA;
# 3373
struct Cyc_List_List*inst=({Cyc_List_zip(td->tvs,ts);});
struct Cyc_List_List*typs=fld->typs;
while(typs != 0){
t=inst == 0?(*((struct _tuple12*)typs->hd)).f2:({Cyc_Tcutil_substitute(inst,(*((struct _tuple12*)typs->hd)).f2);});
if(({Cyc_Tcutil_is_noalias_pointer_or_aggr(t);}))return 1;typs=typs->tl;}
# 3380
return 0;}}_LL14:;}default: goto _LLD;}case 7U: _LL5: _tmp5EB=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp5EA)->f2;_LL6: {struct Cyc_List_List*x=_tmp5EB;
# 3350
while(x != 0){
if(({Cyc_Tcutil_is_noalias_pointer_or_aggr(((struct Cyc_Absyn_Aggrfield*)x->hd)->type);}))return 1;x=x->tl;}
# 3354
return 0;}default: _LLD: _LLE:
# 3382
 return 0;}_LL0:;}}
# 3163
int Cyc_Tcutil_is_noalias_path(struct Cyc_Absyn_Exp*e){
# 3390
void*_stmttmp77=e->r;void*_tmp5FC=_stmttmp77;struct Cyc_Absyn_Stmt*_tmp5FD;struct Cyc_Absyn_Exp*_tmp5FE;struct Cyc_Absyn_Exp*_tmp5FF;struct Cyc_Absyn_Exp*_tmp601;struct Cyc_Absyn_Exp*_tmp600;struct Cyc_Absyn_Exp*_tmp603;struct Cyc_Absyn_Exp*_tmp602;struct _fat_ptr*_tmp605;struct Cyc_Absyn_Exp*_tmp604;struct Cyc_Absyn_Exp*_tmp606;struct Cyc_Absyn_Exp*_tmp607;switch(*((int*)_tmp5FC)){case 1U: if(((struct Cyc_Absyn_Global_b_Absyn_Binding_struct*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp5FC)->f1)->tag == 1U){_LL1: _LL2:
 return 0;}else{goto _LL13;}case 22U: _LL3: _tmp607=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp5FC)->f1;_LL4: {struct Cyc_Absyn_Exp*e1=_tmp607;
_tmp606=e1;goto _LL6;}case 20U: _LL5: _tmp606=((struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_tmp5FC)->f1;_LL6: {struct Cyc_Absyn_Exp*e1=_tmp606;
# 3394
return({Cyc_Tcutil_is_noalias_pointer((void*)_check_null(e1->topt),1);})&&({Cyc_Tcutil_is_noalias_path(e1);});}case 21U: _LL7: _tmp604=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp5FC)->f1;_tmp605=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp5FC)->f2;_LL8: {struct Cyc_Absyn_Exp*e1=_tmp604;struct _fat_ptr*f=_tmp605;
return({Cyc_Tcutil_is_noalias_path(e1);});}case 23U: _LL9: _tmp602=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp5FC)->f1;_tmp603=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp5FC)->f2;_LLA: {struct Cyc_Absyn_Exp*e1=_tmp602;struct Cyc_Absyn_Exp*e2=_tmp603;
# 3397
void*_stmttmp78=({Cyc_Tcutil_compress((void*)_check_null(e1->topt));});void*_tmp608=_stmttmp78;if(((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp608)->tag == 6U){_LL16: _LL17:
 return({Cyc_Tcutil_is_noalias_path(e1);});}else{_LL18: _LL19:
 return 0;}_LL15:;}case 6U: _LLB: _tmp600=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp5FC)->f2;_tmp601=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp5FC)->f3;_LLC: {struct Cyc_Absyn_Exp*e1=_tmp600;struct Cyc_Absyn_Exp*e2=_tmp601;
# 3402
return({Cyc_Tcutil_is_noalias_path(e1);})&&({Cyc_Tcutil_is_noalias_path(e2);});}case 9U: _LLD: _tmp5FF=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp5FC)->f2;_LLE: {struct Cyc_Absyn_Exp*e2=_tmp5FF;
_tmp5FE=e2;goto _LL10;}case 14U: _LLF: _tmp5FE=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp5FC)->f2;_LL10: {struct Cyc_Absyn_Exp*e2=_tmp5FE;
return({Cyc_Tcutil_is_noalias_path(e2);});}case 38U: _LL11: _tmp5FD=((struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct*)_tmp5FC)->f1;_LL12: {struct Cyc_Absyn_Stmt*s=_tmp5FD;
# 3406
while(1){
void*_stmttmp79=s->r;void*_tmp609=_stmttmp79;struct Cyc_Absyn_Exp*_tmp60A;struct Cyc_Absyn_Stmt*_tmp60C;struct Cyc_Absyn_Decl*_tmp60B;struct Cyc_Absyn_Stmt*_tmp60E;struct Cyc_Absyn_Stmt*_tmp60D;switch(*((int*)_tmp609)){case 2U: _LL1B: _tmp60D=((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_tmp609)->f1;_tmp60E=((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_tmp609)->f2;_LL1C: {struct Cyc_Absyn_Stmt*s1=_tmp60D;struct Cyc_Absyn_Stmt*s2=_tmp60E;
s=s2;goto _LL1A;}case 12U: _LL1D: _tmp60B=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp609)->f1;_tmp60C=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp609)->f2;_LL1E: {struct Cyc_Absyn_Decl*d=_tmp60B;struct Cyc_Absyn_Stmt*s1=_tmp60C;
s=s1;goto _LL1A;}case 1U: _LL1F: _tmp60A=((struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct*)_tmp609)->f1;_LL20: {struct Cyc_Absyn_Exp*e=_tmp60A;
return({Cyc_Tcutil_is_noalias_path(e);});}default: _LL21: _LL22:
({void*_tmp60F=0U;({int(*_tmp86E)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp611)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Tcutil_impos;_tmp611;});struct _fat_ptr _tmp86D=({const char*_tmp610="is_noalias_stmt_exp: ill-formed StmtExp";_tag_fat(_tmp610,sizeof(char),40U);});_tmp86E(_tmp86D,_tag_fat(_tmp60F,sizeof(void*),0U));});});}_LL1A:;}}default: _LL13: _LL14:
# 3414
 return 1;}_LL0:;}
# 3431 "tcutil.cyc"
struct _tuple14 Cyc_Tcutil_addressof_props(struct Cyc_Absyn_Exp*e){
# 3433
struct _tuple14 bogus_ans=({struct _tuple14 _tmp6FD;_tmp6FD.f1=0,_tmp6FD.f2=Cyc_Absyn_heap_rgn_type;_tmp6FD;});
void*_stmttmp7A=e->r;void*_tmp613=_stmttmp7A;struct Cyc_Absyn_Exp*_tmp615;struct Cyc_Absyn_Exp*_tmp614;struct Cyc_Absyn_Exp*_tmp616;int _tmp619;struct _fat_ptr*_tmp618;struct Cyc_Absyn_Exp*_tmp617;int _tmp61C;struct _fat_ptr*_tmp61B;struct Cyc_Absyn_Exp*_tmp61A;void*_tmp61D;switch(*((int*)_tmp613)){case 1U: _LL1: _tmp61D=(void*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp613)->f1;_LL2: {void*x=_tmp61D;
# 3437
void*_tmp61E=x;struct Cyc_Absyn_Vardecl*_tmp61F;struct Cyc_Absyn_Vardecl*_tmp620;struct Cyc_Absyn_Vardecl*_tmp621;struct Cyc_Absyn_Vardecl*_tmp622;switch(*((int*)_tmp61E)){case 0U: _LLE: _LLF:
 goto _LL11;case 2U: _LL10: _LL11:
 return bogus_ans;case 1U: _LL12: _tmp622=((struct Cyc_Absyn_Global_b_Absyn_Binding_struct*)_tmp61E)->f1;_LL13: {struct Cyc_Absyn_Vardecl*vd=_tmp622;
# 3441
void*_stmttmp7B=({Cyc_Tcutil_compress((void*)_check_null(e->topt));});void*_tmp623=_stmttmp7B;if(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp623)->tag == 4U){_LL1B: _LL1C:
# 3443
 return({struct _tuple14 _tmp6F1;_tmp6F1.f1=1,_tmp6F1.f2=Cyc_Absyn_heap_rgn_type;_tmp6F1;});}else{_LL1D: _LL1E:
 return({struct _tuple14 _tmp6F2;_tmp6F2.f1=(vd->tq).real_const,_tmp6F2.f2=Cyc_Absyn_heap_rgn_type;_tmp6F2;});}_LL1A:;}case 4U: _LL14: _tmp621=((struct Cyc_Absyn_Local_b_Absyn_Binding_struct*)_tmp61E)->f1;_LL15: {struct Cyc_Absyn_Vardecl*vd=_tmp621;
# 3447
void*_stmttmp7C=({Cyc_Tcutil_compress((void*)_check_null(e->topt));});void*_tmp624=_stmttmp7C;if(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp624)->tag == 4U){_LL20: _LL21:
 return({struct _tuple14 _tmp6F3;_tmp6F3.f1=1,_tmp6F3.f2=(void*)_check_null(vd->rgn);_tmp6F3;});}else{_LL22: _LL23:
# 3450
 vd->escapes=1;
return({struct _tuple14 _tmp6F4;_tmp6F4.f1=(vd->tq).real_const,_tmp6F4.f2=(void*)_check_null(vd->rgn);_tmp6F4;});}_LL1F:;}case 5U: _LL16: _tmp620=((struct Cyc_Absyn_Pat_b_Absyn_Binding_struct*)_tmp61E)->f1;_LL17: {struct Cyc_Absyn_Vardecl*vd=_tmp620;
# 3453
_tmp61F=vd;goto _LL19;}default: _LL18: _tmp61F=((struct Cyc_Absyn_Param_b_Absyn_Binding_struct*)_tmp61E)->f1;_LL19: {struct Cyc_Absyn_Vardecl*vd=_tmp61F;
# 3455
vd->escapes=1;
return({struct _tuple14 _tmp6F5;_tmp6F5.f1=(vd->tq).real_const,_tmp6F5.f2=(void*)_check_null(vd->rgn);_tmp6F5;});}}_LLD:;}case 21U: _LL3: _tmp61A=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp613)->f1;_tmp61B=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp613)->f2;_tmp61C=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp613)->f3;_LL4: {struct Cyc_Absyn_Exp*e1=_tmp61A;struct _fat_ptr*f=_tmp61B;int is_tagged=_tmp61C;
# 3460
if(is_tagged)return bogus_ans;{void*_stmttmp7D=({Cyc_Tcutil_compress((void*)_check_null(e1->topt));});void*_tmp625=_stmttmp7D;struct Cyc_Absyn_Aggrdecl*_tmp626;struct Cyc_List_List*_tmp627;switch(*((int*)_tmp625)){case 7U: _LL25: _tmp627=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp625)->f2;_LL26: {struct Cyc_List_List*fs=_tmp627;
# 3465
struct Cyc_Absyn_Aggrfield*finfo=({Cyc_Absyn_lookup_field(fs,f);});
if(finfo != 0 && finfo->width == 0){
struct _tuple14 _stmttmp7E=({Cyc_Tcutil_addressof_props(e1);});void*_tmp629;int _tmp628;_LL2C: _tmp628=_stmttmp7E.f1;_tmp629=_stmttmp7E.f2;_LL2D: {int c=_tmp628;void*t=_tmp629;
return({struct _tuple14 _tmp6F6;_tmp6F6.f1=(finfo->tq).real_const || c,_tmp6F6.f2=t;_tmp6F6;});}}
# 3466
return bogus_ans;}case 0U: if(((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp625)->f1)->tag == 22U){if(((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp625)->f1)->f1).KnownAggr).tag == 2){_LL27: _tmp626=*((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp625)->f1)->f1).KnownAggr).val;_LL28: {struct Cyc_Absyn_Aggrdecl*ad=_tmp626;
# 3472
struct Cyc_Absyn_Aggrfield*finfo=({Cyc_Absyn_lookup_decl_field(ad,f);});
if(finfo != 0 && finfo->width == 0){
struct _tuple14 _stmttmp7F=({Cyc_Tcutil_addressof_props(e1);});void*_tmp62B;int _tmp62A;_LL2F: _tmp62A=_stmttmp7F.f1;_tmp62B=_stmttmp7F.f2;_LL30: {int c=_tmp62A;void*t=_tmp62B;
return({struct _tuple14 _tmp6F7;_tmp6F7.f1=(finfo->tq).real_const || c,_tmp6F7.f2=t;_tmp6F7;});}}
# 3473
return bogus_ans;}}else{goto _LL29;}}else{goto _LL29;}default: _LL29: _LL2A:
# 3478
 return bogus_ans;}_LL24:;}}case 22U: _LL5: _tmp617=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp613)->f1;_tmp618=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp613)->f2;_tmp619=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp613)->f3;_LL6: {struct Cyc_Absyn_Exp*e1=_tmp617;struct _fat_ptr*f=_tmp618;int is_tagged=_tmp619;
# 3482
if(is_tagged)return bogus_ans;{void*_stmttmp80=({Cyc_Tcutil_compress((void*)_check_null(e1->topt));});void*_tmp62C=_stmttmp80;void*_tmp62E;void*_tmp62D;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp62C)->tag == 3U){_LL32: _tmp62D=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp62C)->f1).elt_type;_tmp62E=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp62C)->f1).ptr_atts).rgn;_LL33: {void*t1=_tmp62D;void*r=_tmp62E;
# 3487
struct Cyc_Absyn_Aggrfield*finfo;
{void*_stmttmp81=({Cyc_Tcutil_compress(t1);});void*_tmp62F=_stmttmp81;struct Cyc_Absyn_Aggrdecl*_tmp630;struct Cyc_List_List*_tmp631;switch(*((int*)_tmp62F)){case 7U: _LL37: _tmp631=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp62F)->f2;_LL38: {struct Cyc_List_List*fs=_tmp631;
# 3490
finfo=({Cyc_Absyn_lookup_field(fs,f);});goto _LL36;}case 0U: if(((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp62F)->f1)->tag == 22U){if(((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp62F)->f1)->f1).KnownAggr).tag == 2){_LL39: _tmp630=*((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp62F)->f1)->f1).KnownAggr).val;_LL3A: {struct Cyc_Absyn_Aggrdecl*ad=_tmp630;
# 3492
finfo=({Cyc_Absyn_lookup_decl_field(ad,f);});goto _LL36;}}else{goto _LL3B;}}else{goto _LL3B;}default: _LL3B: _LL3C:
 return bogus_ans;}_LL36:;}
# 3495
if(finfo != 0 && finfo->width == 0)
return({struct _tuple14 _tmp6F8;_tmp6F8.f1=(finfo->tq).real_const,_tmp6F8.f2=r;_tmp6F8;});
# 3495
return bogus_ans;}}else{_LL34: _LL35:
# 3498
 return bogus_ans;}_LL31:;}}case 20U: _LL7: _tmp616=((struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_tmp613)->f1;_LL8: {struct Cyc_Absyn_Exp*e1=_tmp616;
# 3502
void*_stmttmp82=({Cyc_Tcutil_compress((void*)_check_null(e1->topt));});void*_tmp632=_stmttmp82;void*_tmp634;struct Cyc_Absyn_Tqual _tmp633;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp632)->tag == 3U){_LL3E: _tmp633=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp632)->f1).elt_tq;_tmp634=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp632)->f1).ptr_atts).rgn;_LL3F: {struct Cyc_Absyn_Tqual tq=_tmp633;void*r=_tmp634;
# 3504
return({struct _tuple14 _tmp6F9;_tmp6F9.f1=tq.real_const,_tmp6F9.f2=r;_tmp6F9;});}}else{_LL40: _LL41:
 return bogus_ans;}_LL3D:;}case 23U: _LL9: _tmp614=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp613)->f1;_tmp615=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp613)->f2;_LLA: {struct Cyc_Absyn_Exp*e1=_tmp614;struct Cyc_Absyn_Exp*e2=_tmp615;
# 3510
void*t=({Cyc_Tcutil_compress((void*)_check_null(e1->topt));});
void*_tmp635=t;struct Cyc_Absyn_Tqual _tmp636;void*_tmp638;struct Cyc_Absyn_Tqual _tmp637;struct Cyc_List_List*_tmp639;switch(*((int*)_tmp635)){case 6U: _LL43: _tmp639=((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp635)->f1;_LL44: {struct Cyc_List_List*ts=_tmp639;
# 3514
struct _tuple13 _stmttmp83=({Cyc_Evexp_eval_const_uint_exp(e2);});int _tmp63B;unsigned _tmp63A;_LL4C: _tmp63A=_stmttmp83.f1;_tmp63B=_stmttmp83.f2;_LL4D: {unsigned i=_tmp63A;int known=_tmp63B;
if(!known)
return bogus_ans;{
# 3515
struct _tuple12*finfo=({Cyc_Absyn_lookup_tuple_field(ts,(int)i);});
# 3518
if(finfo != 0)
return({struct _tuple14 _tmp6FA;_tmp6FA.f1=((*finfo).f1).real_const,({void*_tmp86F=({Cyc_Tcutil_addressof_props(e1);}).f2;_tmp6FA.f2=_tmp86F;});_tmp6FA;});
# 3518
return bogus_ans;}}}case 3U: _LL45: _tmp637=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp635)->f1).elt_tq;_tmp638=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp635)->f1).ptr_atts).rgn;_LL46: {struct Cyc_Absyn_Tqual tq=_tmp637;void*r=_tmp638;
# 3522
return({struct _tuple14 _tmp6FB;_tmp6FB.f1=tq.real_const,_tmp6FB.f2=r;_tmp6FB;});}case 4U: _LL47: _tmp636=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp635)->f1).tq;_LL48: {struct Cyc_Absyn_Tqual tq=_tmp636;
# 3528
return({struct _tuple14 _tmp6FC;_tmp6FC.f1=tq.real_const,({void*_tmp870=({Cyc_Tcutil_addressof_props(e1);}).f2;_tmp6FC.f2=_tmp870;});_tmp6FC;});}default: _LL49: _LL4A:
 return bogus_ans;}_LL42:;}default: _LLB: _LLC:
# 3532
({void*_tmp63C=0U;({unsigned _tmp872=e->loc;struct _fat_ptr _tmp871=({const char*_tmp63D="unary & applied to non-lvalue";_tag_fat(_tmp63D,sizeof(char),30U);});Cyc_Tcutil_terr(_tmp872,_tmp871,_tag_fat(_tmp63C,sizeof(void*),0U));});});
return bogus_ans;}_LL0:;}
# 3539
void Cyc_Tcutil_check_bound(unsigned loc,unsigned i,void*b,int do_warn){
struct Cyc_Absyn_Exp*eopt=({struct Cyc_Absyn_Exp*(*_tmp649)(void*def,void*b)=Cyc_Tcutil_get_bounds_exp;void*_tmp64A=({Cyc_Absyn_bounds_one();});void*_tmp64B=b;_tmp649(_tmp64A,_tmp64B);});
if(eopt == 0)return;{struct Cyc_Absyn_Exp*e=eopt;
# 3544
struct _tuple13 _stmttmp84=({Cyc_Evexp_eval_const_uint_exp(e);});int _tmp640;unsigned _tmp63F;_LL1: _tmp63F=_stmttmp84.f1;_tmp640=_stmttmp84.f2;_LL2: {unsigned j=_tmp63F;int known=_tmp640;
if(known && j <= i){
if(do_warn)
({struct Cyc_Int_pa_PrintArg_struct _tmp643=({struct Cyc_Int_pa_PrintArg_struct _tmp6FF;_tmp6FF.tag=1U,_tmp6FF.f1=(unsigned long)((int)j);_tmp6FF;});struct Cyc_Int_pa_PrintArg_struct _tmp644=({struct Cyc_Int_pa_PrintArg_struct _tmp6FE;_tmp6FE.tag=1U,_tmp6FE.f1=(unsigned long)((int)i);_tmp6FE;});void*_tmp641[2U];_tmp641[0]=& _tmp643,_tmp641[1]=& _tmp644;({unsigned _tmp874=loc;struct _fat_ptr _tmp873=({const char*_tmp642="a dereference will be out of bounds: %d <= %d";_tag_fat(_tmp642,sizeof(char),46U);});Cyc_Tcutil_warn(_tmp874,_tmp873,_tag_fat(_tmp641,sizeof(void*),2U));});});else{
# 3549
({struct Cyc_Int_pa_PrintArg_struct _tmp647=({struct Cyc_Int_pa_PrintArg_struct _tmp701;_tmp701.tag=1U,_tmp701.f1=(unsigned long)((int)j);_tmp701;});struct Cyc_Int_pa_PrintArg_struct _tmp648=({struct Cyc_Int_pa_PrintArg_struct _tmp700;_tmp700.tag=1U,_tmp700.f1=(unsigned long)((int)i);_tmp700;});void*_tmp645[2U];_tmp645[0]=& _tmp647,_tmp645[1]=& _tmp648;({unsigned _tmp876=loc;struct _fat_ptr _tmp875=({const char*_tmp646="dereference is out of bounds: %d <= %d";_tag_fat(_tmp646,sizeof(char),39U);});Cyc_Tcutil_terr(_tmp876,_tmp875,_tag_fat(_tmp645,sizeof(void*),2U));});});}}
# 3545
return;}}}
# 3553
void Cyc_Tcutil_check_nonzero_bound(unsigned loc,void*b){
({Cyc_Tcutil_check_bound(loc,0U,b,0);});}
# 3562
static int Cyc_Tcutil_cnst_exp(int var_okay,struct Cyc_Absyn_Exp*e){
void*_stmttmp85=e->r;void*_tmp64E=_stmttmp85;struct Cyc_List_List*_tmp64F;struct Cyc_List_List*_tmp650;struct Cyc_List_List*_tmp652;enum Cyc_Absyn_Primop _tmp651;struct Cyc_List_List*_tmp653;struct Cyc_List_List*_tmp654;struct Cyc_List_List*_tmp655;struct Cyc_Absyn_Exp*_tmp656;struct Cyc_Absyn_Exp*_tmp658;struct Cyc_Absyn_Exp*_tmp657;struct Cyc_Absyn_Exp*_tmp659;struct Cyc_Absyn_Exp*_tmp65B;void*_tmp65A;struct Cyc_Absyn_Exp*_tmp65D;void*_tmp65C;struct Cyc_Absyn_Exp*_tmp65E;struct Cyc_Absyn_Exp*_tmp65F;struct Cyc_Absyn_Exp*_tmp660;struct Cyc_Absyn_Exp*_tmp662;struct Cyc_Absyn_Exp*_tmp661;struct Cyc_Absyn_Exp*_tmp665;struct Cyc_Absyn_Exp*_tmp664;struct Cyc_Absyn_Exp*_tmp663;void*_tmp666;switch(*((int*)_tmp64E)){case 0U: _LL1: _LL2:
 goto _LL4;case 2U: _LL3: _LL4:
 goto _LL6;case 17U: _LL5: _LL6:
 goto _LL8;case 18U: _LL7: _LL8:
 goto _LLA;case 19U: _LL9: _LLA:
 goto _LLC;case 32U: _LLB: _LLC:
 goto _LLE;case 33U: _LLD: _LLE:
 return 1;case 1U: _LLF: _tmp666=(void*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp64E)->f1;_LL10: {void*b=_tmp666;
# 3574
void*_tmp667=b;struct Cyc_Absyn_Vardecl*_tmp668;struct Cyc_Absyn_Vardecl*_tmp669;switch(*((int*)_tmp667)){case 2U: _LL34: _LL35:
 return 1;case 1U: _LL36: _tmp669=((struct Cyc_Absyn_Global_b_Absyn_Binding_struct*)_tmp667)->f1;_LL37: {struct Cyc_Absyn_Vardecl*vd=_tmp669;
# 3577
void*_stmttmp86=({Cyc_Tcutil_compress(vd->type);});void*_tmp66A=_stmttmp86;switch(*((int*)_tmp66A)){case 4U: _LL3F: _LL40:
 goto _LL42;case 5U: _LL41: _LL42:
 return 1;default: _LL43: _LL44:
 return var_okay;}_LL3E:;}case 4U: _LL38: _tmp668=((struct Cyc_Absyn_Local_b_Absyn_Binding_struct*)_tmp667)->f1;_LL39: {struct Cyc_Absyn_Vardecl*vd=_tmp668;
# 3584
if((int)vd->sc == (int)Cyc_Absyn_Static){
void*_stmttmp87=({Cyc_Tcutil_compress(vd->type);});void*_tmp66B=_stmttmp87;switch(*((int*)_tmp66B)){case 4U: _LL46: _LL47:
 goto _LL49;case 5U: _LL48: _LL49:
 return 1;default: _LL4A: _LL4B:
 return var_okay;}_LL45:;}else{
# 3591
return var_okay;}}case 0U: _LL3A: _LL3B:
 return 0;default: _LL3C: _LL3D:
 return var_okay;}_LL33:;}case 6U: _LL11: _tmp663=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp64E)->f1;_tmp664=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp64E)->f2;_tmp665=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp64E)->f3;_LL12: {struct Cyc_Absyn_Exp*e1=_tmp663;struct Cyc_Absyn_Exp*e2=_tmp664;struct Cyc_Absyn_Exp*e3=_tmp665;
# 3597
return(({Cyc_Tcutil_cnst_exp(0,e1);})&&({Cyc_Tcutil_cnst_exp(0,e2);}))&&({Cyc_Tcutil_cnst_exp(0,e3);});}case 9U: _LL13: _tmp661=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp64E)->f1;_tmp662=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp64E)->f2;_LL14: {struct Cyc_Absyn_Exp*e1=_tmp661;struct Cyc_Absyn_Exp*e2=_tmp662;
# 3601
return({Cyc_Tcutil_cnst_exp(0,e1);})&&({Cyc_Tcutil_cnst_exp(0,e2);});}case 42U: _LL15: _tmp660=((struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct*)_tmp64E)->f1;_LL16: {struct Cyc_Absyn_Exp*e2=_tmp660;
_tmp65F=e2;goto _LL18;}case 12U: _LL17: _tmp65F=((struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*)_tmp64E)->f1;_LL18: {struct Cyc_Absyn_Exp*e2=_tmp65F;
_tmp65E=e2;goto _LL1A;}case 13U: _LL19: _tmp65E=((struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*)_tmp64E)->f1;_LL1A: {struct Cyc_Absyn_Exp*e2=_tmp65E;
# 3605
return({Cyc_Tcutil_cnst_exp(var_okay,e2);});}case 14U: if(((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp64E)->f4 == Cyc_Absyn_No_coercion){_LL1B: _tmp65C=(void*)((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp64E)->f1;_tmp65D=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp64E)->f2;_LL1C: {void*t=_tmp65C;struct Cyc_Absyn_Exp*e2=_tmp65D;
# 3607
return({Cyc_Tcutil_cnst_exp(var_okay,e2);});}}else{_LL1D: _tmp65A=(void*)((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp64E)->f1;_tmp65B=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp64E)->f2;_LL1E: {void*t=_tmp65A;struct Cyc_Absyn_Exp*e2=_tmp65B;
# 3610
return({Cyc_Tcutil_cnst_exp(var_okay,e2);});}}case 15U: _LL1F: _tmp659=((struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct*)_tmp64E)->f1;_LL20: {struct Cyc_Absyn_Exp*e2=_tmp659;
# 3612
return({Cyc_Tcutil_cnst_exp(1,e2);});}case 27U: _LL21: _tmp657=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmp64E)->f2;_tmp658=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmp64E)->f3;_LL22: {struct Cyc_Absyn_Exp*e1=_tmp657;struct Cyc_Absyn_Exp*e2=_tmp658;
# 3614
return({Cyc_Tcutil_cnst_exp(0,e1);})&&({Cyc_Tcutil_cnst_exp(0,e2);});}case 28U: _LL23: _tmp656=((struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*)_tmp64E)->f1;_LL24: {struct Cyc_Absyn_Exp*e=_tmp656;
# 3616
return({Cyc_Tcutil_cnst_exp(0,e);});}case 26U: _LL25: _tmp655=((struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*)_tmp64E)->f1;_LL26: {struct Cyc_List_List*des=_tmp655;
_tmp654=des;goto _LL28;}case 30U: _LL27: _tmp654=((struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct*)_tmp64E)->f2;_LL28: {struct Cyc_List_List*des=_tmp654;
_tmp653=des;goto _LL2A;}case 29U: _LL29: _tmp653=((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_tmp64E)->f3;_LL2A: {struct Cyc_List_List*des=_tmp653;
# 3620
for(0;des != 0;des=des->tl){
if(!({Cyc_Tcutil_cnst_exp(0,(*((struct _tuple16*)des->hd)).f2);}))
return 0;}
# 3620
return 1;}case 3U: _LL2B: _tmp651=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp64E)->f1;_tmp652=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp64E)->f2;_LL2C: {enum Cyc_Absyn_Primop p=_tmp651;struct Cyc_List_List*es=_tmp652;
# 3625
_tmp650=es;goto _LL2E;}case 24U: _LL2D: _tmp650=((struct Cyc_Absyn_Tuple_e_Absyn_Raw_exp_struct*)_tmp64E)->f1;_LL2E: {struct Cyc_List_List*es=_tmp650;
_tmp64F=es;goto _LL30;}case 31U: _LL2F: _tmp64F=((struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*)_tmp64E)->f1;_LL30: {struct Cyc_List_List*es=_tmp64F;
# 3628
for(0;es != 0;es=es->tl){
if(!({Cyc_Tcutil_cnst_exp(0,(struct Cyc_Absyn_Exp*)es->hd);}))
return 0;}
# 3628
return 1;}default: _LL31: _LL32:
# 3632
 return 0;}_LL0:;}
# 3562
int Cyc_Tcutil_is_const_exp(struct Cyc_Absyn_Exp*e){
# 3636
return({Cyc_Tcutil_cnst_exp(0,e);});}
# 3562
static int Cyc_Tcutil_fields_zeroable(struct Cyc_List_List*,struct Cyc_List_List*,struct Cyc_List_List*);
# 3640
int Cyc_Tcutil_zeroable_type(void*t){
void*_stmttmp88=({Cyc_Tcutil_compress(t);});void*_tmp66E=_stmttmp88;struct Cyc_List_List*_tmp66F;struct Cyc_List_List*_tmp670;void*_tmp671;void*_tmp672;struct Cyc_List_List*_tmp674;void*_tmp673;switch(*((int*)_tmp66E)){case 0U: _LL1: _tmp673=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp66E)->f1;_tmp674=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp66E)->f2;_LL2: {void*c=_tmp673;struct Cyc_List_List*ts=_tmp674;
# 3643
void*_tmp675=c;union Cyc_Absyn_AggrInfo _tmp676;struct Cyc_List_List*_tmp677;struct Cyc_Absyn_Enumdecl*_tmp678;switch(*((int*)_tmp675)){case 0U: _LLE: _LLF:
 goto _LL11;case 1U: _LL10: _LL11:
 goto _LL13;case 2U: _LL12: _LL13:
 return 1;case 17U: _LL14: _tmp678=((struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct*)_tmp675)->f2;_LL15: {struct Cyc_Absyn_Enumdecl*edo=_tmp678;
# 3648
if(edo == 0 || edo->fields == 0)
return 0;
# 3648
_tmp677=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(edo->fields))->v;goto _LL17;}case 18U: _LL16: _tmp677=((struct Cyc_Absyn_AnonEnumCon_Absyn_TyCon_struct*)_tmp675)->f1;_LL17: {struct Cyc_List_List*fs=_tmp677;
# 3652
{struct Cyc_List_List*fs2=fs;for(0;fs2 != 0;fs2=fs2->tl){
if(((struct Cyc_Absyn_Enumfield*)fs2->hd)->tag == 0)
return fs2 == fs;{
# 3653
struct _tuple13 _stmttmp89=({Cyc_Evexp_eval_const_uint_exp((struct Cyc_Absyn_Exp*)_check_null(((struct Cyc_Absyn_Enumfield*)fs2->hd)->tag));});int _tmp67A;unsigned _tmp679;_LL1D: _tmp679=_stmttmp89.f1;_tmp67A=_stmttmp89.f2;_LL1E: {unsigned i=_tmp679;int known=_tmp67A;
# 3656
if(known && i == (unsigned)0)
return 1;}}}}
# 3659
return 0;}case 22U: _LL18: _tmp676=((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)_tmp675)->f1;_LL19: {union Cyc_Absyn_AggrInfo info=_tmp676;
# 3662
struct Cyc_Absyn_Aggrdecl*ad=({Cyc_Absyn_get_known_aggrdecl(info);});
if(ad->impl == 0)return 0;if(((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->exist_vars != 0)
return 0;
# 3663
if(
# 3665
(int)ad->kind == (int)Cyc_Absyn_UnionA &&((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->tagged)return 0;
# 3663
return({Cyc_Tcutil_fields_zeroable(ad->tvs,ts,((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->fields);});}default: _LL1A: _LL1B:
# 3667
 return 0;}_LLD:;}case 3U: _LL3: _tmp672=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp66E)->f1).ptr_atts).nullable;_LL4: {void*n=_tmp672;
# 3670
return({Cyc_Tcutil_force_type2bool(1,n);});}case 4U: _LL5: _tmp671=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp66E)->f1).elt_type;_LL6: {void*t=_tmp671;
return({Cyc_Tcutil_zeroable_type(t);});}case 6U: _LL7: _tmp670=((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp66E)->f1;_LL8: {struct Cyc_List_List*tqs=_tmp670;
# 3673
for(0;tqs != 0;tqs=tqs->tl){
if(!({Cyc_Tcutil_zeroable_type((*((struct _tuple12*)tqs->hd)).f2);}))return 0;}
# 3673
return 1;}case 7U: _LL9: _tmp66F=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp66E)->f2;_LLA: {struct Cyc_List_List*fs=_tmp66F;
# 3676
return({Cyc_Tcutil_fields_zeroable(0,0,fs);});}default: _LLB: _LLC:
 return 0;}_LL0:;}
# 3680
static int Cyc_Tcutil_fields_zeroable(struct Cyc_List_List*tvs,struct Cyc_List_List*ts,struct Cyc_List_List*fs){
# 3682
struct _RegionHandle _tmp67C=_new_region("rgn");struct _RegionHandle*rgn=& _tmp67C;_push_region(rgn);
{struct Cyc_List_List*inst=({Cyc_List_rzip(rgn,rgn,tvs,ts);});
for(0;fs != 0;fs=fs->tl){
void*t=((struct Cyc_Absyn_Aggrfield*)fs->hd)->type;
if(({Cyc_Tcutil_zeroable_type(t);}))continue;t=({Cyc_Tcutil_rsubstitute(rgn,inst,((struct Cyc_Absyn_Aggrfield*)fs->hd)->type);});
# 3688
if(!({Cyc_Tcutil_zeroable_type(t);})){int _tmp67D=0;_npop_handler(0U);return _tmp67D;}}{
# 3690
int _tmp67E=1;_npop_handler(0U);return _tmp67E;}}
# 3683
;_pop_region();}
# 3694
void Cyc_Tcutil_check_no_qual(unsigned loc,void*t){
void*_tmp680=t;struct Cyc_Absyn_Typedefdecl*_tmp681;if(((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp680)->tag == 8U){_LL1: _tmp681=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp680)->f3;_LL2: {struct Cyc_Absyn_Typedefdecl*tdopt=_tmp681;
# 3697
if(tdopt != 0){
struct Cyc_Absyn_Tqual tq=tdopt->tq;
if(((tq.print_const || tq.q_volatile)|| tq.q_restrict)|| tq.real_const)
({struct Cyc_String_pa_PrintArg_struct _tmp684=({struct Cyc_String_pa_PrintArg_struct _tmp702;_tmp702.tag=0U,({struct _fat_ptr _tmp877=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t);}));_tmp702.f1=_tmp877;});_tmp702;});void*_tmp682[1U];_tmp682[0]=& _tmp684;({unsigned _tmp879=loc;struct _fat_ptr _tmp878=({const char*_tmp683="qualifier within typedef type %s is ignored";_tag_fat(_tmp683,sizeof(char),44U);});Cyc_Tcutil_warn(_tmp879,_tmp878,_tag_fat(_tmp682,sizeof(void*),1U));});});}
# 3697
goto _LL0;}}else{_LL3: _LL4:
# 3703
 goto _LL0;}_LL0:;}
# 3694
struct Cyc_List_List*Cyc_Tcutil_transfer_fn_type_atts(void*t,struct Cyc_List_List*atts){
# 3710
void*_stmttmp8A=({Cyc_Tcutil_compress(t);});void*_tmp686=_stmttmp8A;struct Cyc_List_List**_tmp687;if(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp686)->tag == 5U){_LL1: _tmp687=(struct Cyc_List_List**)&(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp686)->f1).attributes;_LL2: {struct Cyc_List_List**fnatts=_tmp687;
# 3712
struct Cyc_List_List*res_atts=0;
for(0;atts != 0;atts=atts->tl){
if(({Cyc_Absyn_fntype_att((void*)atts->hd);})){
if(!({Cyc_List_mem(Cyc_Absyn_attribute_cmp,*fnatts,(void*)atts->hd);}))
({struct Cyc_List_List*_tmp87A=({struct Cyc_List_List*_tmp688=_cycalloc(sizeof(*_tmp688));_tmp688->hd=(void*)atts->hd,_tmp688->tl=*fnatts;_tmp688;});*fnatts=_tmp87A;});}else{
# 3719
res_atts=({struct Cyc_List_List*_tmp689=_cycalloc(sizeof(*_tmp689));_tmp689->hd=(void*)atts->hd,_tmp689->tl=res_atts;_tmp689;});}}
return res_atts;}}else{_LL3: _LL4:
({void*_tmp68A=0U;({int(*_tmp87C)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp68C)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Tcutil_impos;_tmp68C;});struct _fat_ptr _tmp87B=({const char*_tmp68B="transfer_fn_type_atts";_tag_fat(_tmp68B,sizeof(char),22U);});_tmp87C(_tmp87B,_tag_fat(_tmp68A,sizeof(void*),0U));});});}_LL0:;}
# 3694
struct Cyc_Absyn_Exp*Cyc_Tcutil_get_type_bound(void*t){
# 3727
void*_stmttmp8B=({Cyc_Tcutil_compress(t);});void*_tmp68E=_stmttmp8B;struct Cyc_Absyn_Exp*_tmp68F;struct Cyc_Absyn_PtrInfo _tmp690;switch(*((int*)_tmp68E)){case 3U: _LL1: _tmp690=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp68E)->f1;_LL2: {struct Cyc_Absyn_PtrInfo pi=_tmp690;
return({struct Cyc_Absyn_Exp*(*_tmp691)(void*def,void*b)=Cyc_Tcutil_get_bounds_exp;void*_tmp692=({Cyc_Absyn_bounds_one();});void*_tmp693=(pi.ptr_atts).bounds;_tmp691(_tmp692,_tmp693);});}case 4U: _LL3: _tmp68F=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp68E)->f1).num_elts;_LL4: {struct Cyc_Absyn_Exp*e=_tmp68F;
return e;}default: _LL5: _LL6:
 return 0;}_LL0:;}
# 3736
struct Cyc_Absyn_Vardecl*Cyc_Tcutil_nonesc_vardecl(void*b){
void*_tmp695=b;struct Cyc_Absyn_Vardecl*_tmp696;struct Cyc_Absyn_Vardecl*_tmp697;struct Cyc_Absyn_Vardecl*_tmp698;struct Cyc_Absyn_Vardecl*_tmp699;switch(*((int*)_tmp695)){case 5U: _LL1: _tmp699=((struct Cyc_Absyn_Pat_b_Absyn_Binding_struct*)_tmp695)->f1;_LL2: {struct Cyc_Absyn_Vardecl*x=_tmp699;
_tmp698=x;goto _LL4;}case 4U: _LL3: _tmp698=((struct Cyc_Absyn_Local_b_Absyn_Binding_struct*)_tmp695)->f1;_LL4: {struct Cyc_Absyn_Vardecl*x=_tmp698;
_tmp697=x;goto _LL6;}case 3U: _LL5: _tmp697=((struct Cyc_Absyn_Param_b_Absyn_Binding_struct*)_tmp695)->f1;_LL6: {struct Cyc_Absyn_Vardecl*x=_tmp697;
_tmp696=x;goto _LL8;}case 1U: _LL7: _tmp696=((struct Cyc_Absyn_Global_b_Absyn_Binding_struct*)_tmp695)->f1;_LL8: {struct Cyc_Absyn_Vardecl*x=_tmp696;
# 3742
if(!x->escapes)
return x;
# 3742
return 0;}default: _LL9: _LLA:
# 3745
 return 0;}_LL0:;}
# 3750
struct Cyc_List_List*Cyc_Tcutil_filter_nulls(struct Cyc_List_List*l){
struct Cyc_List_List*res=0;
{struct Cyc_List_List*x=l;for(0;x != 0;x=x->tl){
if((void**)x->hd != 0)res=({struct Cyc_List_List*_tmp69B=_cycalloc(sizeof(*_tmp69B));_tmp69B->hd=*((void**)_check_null((void**)x->hd)),_tmp69B->tl=res;_tmp69B;});}}
# 3752
return res;}
# 3750
void*Cyc_Tcutil_promote_array(void*t,void*rgn,int convert_tag){
# 3758
void*_stmttmp8C=({Cyc_Tcutil_compress(t);});void*_tmp69D=_stmttmp8C;unsigned _tmp6A2;void*_tmp6A1;struct Cyc_Absyn_Exp*_tmp6A0;struct Cyc_Absyn_Tqual _tmp69F;void*_tmp69E;if(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp69D)->tag == 4U){_LL1: _tmp69E=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp69D)->f1).elt_type;_tmp69F=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp69D)->f1).tq;_tmp6A0=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp69D)->f1).num_elts;_tmp6A1=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp69D)->f1).zero_term;_tmp6A2=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp69D)->f1).zt_loc;_LL2: {void*et=_tmp69E;struct Cyc_Absyn_Tqual tq=_tmp69F;struct Cyc_Absyn_Exp*eopt=_tmp6A0;void*zt=_tmp6A1;unsigned ztl=_tmp6A2;
# 3760
void*b;
if(eopt == 0)
b=Cyc_Absyn_fat_bound_type;else{
# 3764
if(convert_tag){
if(eopt->topt == 0)
({void*_tmp6A3=0U;({int(*_tmp87E)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp6A5)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Tcutil_impos;_tmp6A5;});struct _fat_ptr _tmp87D=({const char*_tmp6A4="cannot convert tag without type!";_tag_fat(_tmp6A4,sizeof(char),33U);});_tmp87E(_tmp87D,_tag_fat(_tmp6A3,sizeof(void*),0U));});});{
# 3765
void*_stmttmp8D=({Cyc_Tcutil_compress((void*)_check_null(eopt->topt));});void*_tmp6A6=_stmttmp8D;void*_tmp6A7;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp6A6)->tag == 0U){if(((struct Cyc_Absyn_TagCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp6A6)->f1)->tag == 5U){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp6A6)->f2 != 0){_LL6: _tmp6A7=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp6A6)->f2)->hd;_LL7: {void*t=_tmp6A7;
# 3770
b=({void*(*_tmp6A8)(struct Cyc_Absyn_Exp*)=Cyc_Absyn_thin_bounds_exp;struct Cyc_Absyn_Exp*_tmp6A9=({Cyc_Absyn_valueof_exp(t,0U);});_tmp6A8(_tmp6A9);});
goto _LL5;}}else{goto _LL8;}}else{goto _LL8;}}else{_LL8: _LL9:
# 3773
 b=({Cyc_Tcutil_is_const_exp(eopt);})?({Cyc_Absyn_thin_bounds_exp(eopt);}): Cyc_Absyn_fat_bound_type;}_LL5:;}}else{
# 3777
b=({Cyc_Absyn_thin_bounds_exp(eopt);});}}
# 3779
return({Cyc_Absyn_atb_type(et,rgn,tq,b,zt);});}}else{_LL3: _LL4:
 return t;}_LL0:;}
# 3750
int Cyc_Tcutil_is_tagged_union_and_needs_check(struct Cyc_Absyn_Exp*e){
# 3786
void*_stmttmp8E=e->r;void*_tmp6AB=_stmttmp8E;int _tmp6AD;int _tmp6AC;int _tmp6AF;int _tmp6AE;switch(*((int*)_tmp6AB)){case 21U: _LL1: _tmp6AE=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp6AB)->f3;_tmp6AF=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp6AB)->f4;_LL2: {int is_tagged=_tmp6AE;int is_read=_tmp6AF;
# 3789
_tmp6AC=is_tagged;_tmp6AD=is_read;goto _LL4;}case 22U: _LL3: _tmp6AC=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp6AB)->f3;_tmp6AD=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp6AB)->f4;_LL4: {int is_tagged=_tmp6AC;int is_read=_tmp6AD;
# 3791
return is_tagged && is_read;}default: _LL5: _LL6:
 return 0;}_LL0:;}

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
 struct Cyc___cycFILE;
# 53 "cycboot.h"
extern struct Cyc___cycFILE*Cyc_stderr;struct Cyc_String_pa_PrintArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Int_pa_PrintArg_struct{int tag;unsigned long f1;};struct Cyc_Double_pa_PrintArg_struct{int tag;double f1;};struct Cyc_LongDouble_pa_PrintArg_struct{int tag;long double f1;};struct Cyc_ShortPtr_pa_PrintArg_struct{int tag;short*f1;};struct Cyc_IntPtr_pa_PrintArg_struct{int tag;unsigned long*f1;};
# 73
struct _fat_ptr Cyc_aprintf(struct _fat_ptr,struct _fat_ptr);
# 100
int Cyc_fprintf(struct Cyc___cycFILE*,struct _fat_ptr,struct _fat_ptr);struct Cyc_ShortPtr_sa_ScanfArg_struct{int tag;short*f1;};struct Cyc_UShortPtr_sa_ScanfArg_struct{int tag;unsigned short*f1;};struct Cyc_IntPtr_sa_ScanfArg_struct{int tag;int*f1;};struct Cyc_UIntPtr_sa_ScanfArg_struct{int tag;unsigned*f1;};struct Cyc_StringPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_DoublePtr_sa_ScanfArg_struct{int tag;double*f1;};struct Cyc_FloatPtr_sa_ScanfArg_struct{int tag;float*f1;};struct Cyc_CharPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};extern char Cyc_FileCloseError[15U];struct Cyc_FileCloseError_exn_struct{char*tag;};extern char Cyc_FileOpenError[14U];struct Cyc_FileOpenError_exn_struct{char*tag;struct _fat_ptr f1;};struct Cyc_Core_Opt{void*v;};extern char Cyc_Core_Invalid_argument[17U];struct Cyc_Core_Invalid_argument_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Failure[8U];struct Cyc_Core_Failure_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Impossible[11U];struct Cyc_Core_Impossible_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Not_found[10U];struct Cyc_Core_Not_found_exn_struct{char*tag;};extern char Cyc_Core_Unreachable[12U];struct Cyc_Core_Unreachable_exn_struct{char*tag;struct _fat_ptr f1;};
# 165 "core.h"
extern struct _RegionHandle*Cyc_Core_heap_region;
# 168
extern struct _RegionHandle*Cyc_Core_unique_region;struct Cyc_Core_DynamicRegion;struct Cyc_Core_NewDynamicRegion{struct Cyc_Core_DynamicRegion*key;};struct Cyc_Core_ThinRes{void*arr;unsigned nelts;};struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};
# 54 "list.h"
struct Cyc_List_List*Cyc_List_list(struct _fat_ptr);extern char Cyc_List_List_mismatch[14U];struct Cyc_List_List_mismatch_exn_struct{char*tag;};extern char Cyc_List_Nth[4U];struct Cyc_List_Nth_exn_struct{char*tag;};struct Cyc_Iter_Iter{void*env;int(*next)(void*env,void*dest);};
# 37 "iter.h"
int Cyc_Iter_next(struct Cyc_Iter_Iter,void*);struct Cyc_Dict_T;struct Cyc_Dict_Dict{int(*rel)(void*,void*);struct _RegionHandle*r;const struct Cyc_Dict_T*t;};extern char Cyc_Dict_Present[8U];struct Cyc_Dict_Present_exn_struct{char*tag;};extern char Cyc_Dict_Absent[7U];struct Cyc_Dict_Absent_exn_struct{char*tag;};
# 62 "dict.h"
struct Cyc_Dict_Dict Cyc_Dict_empty(int(*cmp)(void*,void*));
# 83
int Cyc_Dict_member(struct Cyc_Dict_Dict d,void*k);
# 87
struct Cyc_Dict_Dict Cyc_Dict_insert(struct Cyc_Dict_Dict d,void*k,void*v);
# 110
void*Cyc_Dict_lookup(struct Cyc_Dict_Dict d,void*k);struct _tuple0{void*f1;void*f2;};
# 229 "dict.h"
struct _tuple0*Cyc_Dict_rchoose(struct _RegionHandle*,struct Cyc_Dict_Dict d);
# 283
struct Cyc_Iter_Iter Cyc_Dict_make_iter(struct _RegionHandle*rgn,struct Cyc_Dict_Dict d);struct Cyc_Position_Error;struct Cyc_Relations_Reln;struct _union_Nmspace_Rel_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Abs_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_C_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Loc_n{int tag;int val;};union Cyc_Absyn_Nmspace{struct _union_Nmspace_Rel_n Rel_n;struct _union_Nmspace_Abs_n Abs_n;struct _union_Nmspace_C_n C_n;struct _union_Nmspace_Loc_n Loc_n;};struct _tuple1{union Cyc_Absyn_Nmspace f1;struct _fat_ptr*f2;};
# 184 "absyn.h"
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
enum Cyc_Absyn_KindQual{Cyc_Absyn_AnyKind =0U,Cyc_Absyn_MemKind =1U,Cyc_Absyn_BoxKind =2U,Cyc_Absyn_XRgnKind =3U,Cyc_Absyn_RgnKind =4U,Cyc_Absyn_EffKind =5U,Cyc_Absyn_IntKind =6U,Cyc_Absyn_BoolKind =7U,Cyc_Absyn_PtrBndKind =8U};struct Cyc_Absyn_Kind{enum Cyc_Absyn_KindQual kind;enum Cyc_Absyn_AliasQual aliasqual;};struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct{int tag;struct Cyc_Absyn_Kind*f1;};struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct{int tag;struct Cyc_Core_Opt*f1;};struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct{int tag;struct Cyc_Core_Opt*f1;struct Cyc_Absyn_Kind*f2;};struct Cyc_Absyn_Tvar{struct _fat_ptr*name;int identity;void*kind;};struct Cyc_Absyn_PtrLoc{unsigned ptr_loc;unsigned rgn_loc;unsigned zt_loc;};struct Cyc_Absyn_PtrAtts{void*rgn;void*nullable;void*bounds;void*zero_term;struct Cyc_Absyn_PtrLoc*ptrloc;};struct Cyc_Absyn_PtrInfo{void*elt_type;struct Cyc_Absyn_Tqual elt_tq;struct Cyc_Absyn_PtrAtts ptr_atts;};struct Cyc_Absyn_VarargInfo{struct _fat_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;int inject;};struct Cyc_Absyn_Star_Absyn_Cap_struct{int tag;};struct Cyc_Absyn_Bot_Absyn_Cap_struct{int tag;};struct Cyc_Absyn_Nat_Absyn_Cap_struct{int tag;int f1;int f2;};struct Cyc_Absyn_RgnEffect{void*name;struct Cyc_List_List*caps;void*parent;};struct Cyc_Absyn_FnInfo{struct Cyc_List_List*tvars;void*effect;struct Cyc_Absyn_Tqual ret_tqual;void*ret_type;struct Cyc_List_List*args;int c_varargs;struct Cyc_Absyn_VarargInfo*cyc_varargs;struct Cyc_List_List*rgn_po;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*requires_clause;struct Cyc_List_List*requires_relns;struct Cyc_Absyn_Exp*ensures_clause;struct Cyc_List_List*ensures_relns;struct Cyc_List_List*ieffect;struct Cyc_List_List*oeffect;struct Cyc_List_List*throws;int reentrant;};struct Cyc_Absyn_UnknownDatatypeInfo{struct _tuple1*name;int is_extensible;};struct _union_DatatypeInfo_UnknownDatatype{int tag;struct Cyc_Absyn_UnknownDatatypeInfo val;};struct _union_DatatypeInfo_KnownDatatype{int tag;struct Cyc_Absyn_Datatypedecl**val;};union Cyc_Absyn_DatatypeInfo{struct _union_DatatypeInfo_UnknownDatatype UnknownDatatype;struct _union_DatatypeInfo_KnownDatatype KnownDatatype;};struct Cyc_Absyn_UnknownDatatypeFieldInfo{struct _tuple1*datatype_name;struct _tuple1*field_name;int is_extensible;};struct _union_DatatypeFieldInfo_UnknownDatatypefield{int tag;struct Cyc_Absyn_UnknownDatatypeFieldInfo val;};struct _tuple2{struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;};struct _union_DatatypeFieldInfo_KnownDatatypefield{int tag;struct _tuple2 val;};union Cyc_Absyn_DatatypeFieldInfo{struct _union_DatatypeFieldInfo_UnknownDatatypefield UnknownDatatypefield;struct _union_DatatypeFieldInfo_KnownDatatypefield KnownDatatypefield;};struct _tuple3{enum Cyc_Absyn_AggrKind f1;struct _tuple1*f2;struct Cyc_Core_Opt*f3;};struct _union_AggrInfo_UnknownAggr{int tag;struct _tuple3 val;};struct _union_AggrInfo_KnownAggr{int tag;struct Cyc_Absyn_Aggrdecl**val;};union Cyc_Absyn_AggrInfo{struct _union_AggrInfo_UnknownAggr UnknownAggr;struct _union_AggrInfo_KnownAggr KnownAggr;};struct Cyc_Absyn_ArrayInfo{void*elt_type;struct Cyc_Absyn_Tqual tq;struct Cyc_Absyn_Exp*num_elts;void*zero_term;unsigned zt_loc;};struct Cyc_Absyn_Aggr_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Enum_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;};struct Cyc_Absyn_Datatype_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;};struct Cyc_Absyn_TypeDecl{void*r;unsigned loc;};struct Cyc_Absyn_VoidCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_IntCon_Absyn_TyCon_struct{int tag;enum Cyc_Absyn_Sign f1;enum Cyc_Absyn_Size_of f2;};struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct{int tag;int f1;};struct Cyc_Absyn_RgnHandleCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_XRgnHandleCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_TagCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_HeapCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_UniqueCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_RefCntCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_AccessCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_CAccessCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_JoinCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_RgnsCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_TrueCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_FalseCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_ThinCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_FatCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct{int tag;struct _tuple1*f1;struct Cyc_Absyn_Enumdecl*f2;};struct Cyc_Absyn_AnonEnumCon_Absyn_TyCon_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_BuiltinCon_Absyn_TyCon_struct{int tag;struct _fat_ptr f1;struct Cyc_Absyn_Kind*f2;};struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_DatatypeInfo f1;};struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_DatatypeFieldInfo f1;};struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_AggrInfo f1;};struct Cyc_Absyn_AppType_Absyn_Type_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Evar_Absyn_Type_struct{int tag;struct Cyc_Core_Opt*f1;void*f2;int f3;struct Cyc_Core_Opt*f4;};struct Cyc_Absyn_VarType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Tvar*f1;};struct Cyc_Absyn_PointerType_Absyn_Type_struct{int tag;struct Cyc_Absyn_PtrInfo f1;};struct Cyc_Absyn_ArrayType_Absyn_Type_struct{int tag;struct Cyc_Absyn_ArrayInfo f1;};struct Cyc_Absyn_FnType_Absyn_Type_struct{int tag;struct Cyc_Absyn_FnInfo f1;};struct Cyc_Absyn_TupleType_Absyn_Type_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct{int tag;enum Cyc_Absyn_AggrKind f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_TypedefType_Absyn_Type_struct{int tag;struct _tuple1*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_Typedefdecl*f3;void*f4;};struct Cyc_Absyn_ValueofType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct{int tag;struct Cyc_Absyn_TypeDecl*f1;void**f2;};struct Cyc_Absyn_TypeofType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_NoTypes_Absyn_Funcparams_struct{int tag;struct Cyc_List_List*f1;unsigned f2;};struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct{int tag;struct Cyc_List_List*f1;int f2;struct Cyc_Absyn_VarargInfo*f3;void*f4;struct Cyc_List_List*f5;struct Cyc_Absyn_Exp*f6;struct Cyc_Absyn_Exp*f7;struct Cyc_List_List*f8;struct Cyc_List_List*f9;struct Cyc_List_List*f10;int f11;};
# 478 "absyn.h"
enum Cyc_Absyn_Format_Type{Cyc_Absyn_Printf_ft =0U,Cyc_Absyn_Scanf_ft =1U};struct Cyc_Absyn_Regparm_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Stdcall_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Cdecl_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Fastcall_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Noreturn_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Const_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Aligned_att_Absyn_Attribute_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Packed_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Section_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Nocommon_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Shared_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Unused_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Weak_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Dllimport_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Dllexport_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_instrument_function_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Constructor_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Destructor_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_check_memory_usage_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Format_att_Absyn_Attribute_struct{int tag;enum Cyc_Absyn_Format_Type f1;int f2;int f3;};struct Cyc_Absyn_Initializes_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Noliveunique_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Consume_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Pure_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Mode_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Alias_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Always_inline_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_throw_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Non_null_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Deprecated_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_VectorSize_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Carray_mod_Absyn_Type_modifier_struct{int tag;void*f1;unsigned f2;};struct Cyc_Absyn_ConstArray_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;unsigned f3;};struct Cyc_Absyn_Pointer_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_Absyn_PtrAtts f1;struct Cyc_Absyn_Tqual f2;};struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct{int tag;void*f1;};struct Cyc_Absyn_TypeParams_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_List_List*f1;unsigned f2;int f3;};struct Cyc_Absyn_Attributes_mod_Absyn_Type_modifier_struct{int tag;unsigned f1;struct Cyc_List_List*f2;};struct _union_Cnst_Null_c{int tag;int val;};struct _tuple4{enum Cyc_Absyn_Sign f1;char f2;};struct _union_Cnst_Char_c{int tag;struct _tuple4 val;};struct _union_Cnst_Wchar_c{int tag;struct _fat_ptr val;};struct _tuple5{enum Cyc_Absyn_Sign f1;short f2;};struct _union_Cnst_Short_c{int tag;struct _tuple5 val;};struct _tuple6{enum Cyc_Absyn_Sign f1;int f2;};struct _union_Cnst_Int_c{int tag;struct _tuple6 val;};struct _tuple7{enum Cyc_Absyn_Sign f1;long long f2;};struct _union_Cnst_LongLong_c{int tag;struct _tuple7 val;};struct _tuple8{struct _fat_ptr f1;int f2;};struct _union_Cnst_Float_c{int tag;struct _tuple8 val;};struct _union_Cnst_String_c{int tag;struct _fat_ptr val;};struct _union_Cnst_Wstring_c{int tag;struct _fat_ptr val;};union Cyc_Absyn_Cnst{struct _union_Cnst_Null_c Null_c;struct _union_Cnst_Char_c Char_c;struct _union_Cnst_Wchar_c Wchar_c;struct _union_Cnst_Short_c Short_c;struct _union_Cnst_Int_c Int_c;struct _union_Cnst_LongLong_c LongLong_c;struct _union_Cnst_Float_c Float_c;struct _union_Cnst_String_c String_c;struct _union_Cnst_Wstring_c Wstring_c;};
# 574
enum Cyc_Absyn_Primop{Cyc_Absyn_Plus =0U,Cyc_Absyn_Times =1U,Cyc_Absyn_Minus =2U,Cyc_Absyn_Div =3U,Cyc_Absyn_Mod =4U,Cyc_Absyn_Eq =5U,Cyc_Absyn_Neq =6U,Cyc_Absyn_Gt =7U,Cyc_Absyn_Lt =8U,Cyc_Absyn_Gte =9U,Cyc_Absyn_Lte =10U,Cyc_Absyn_Not =11U,Cyc_Absyn_Bitnot =12U,Cyc_Absyn_Bitand =13U,Cyc_Absyn_Bitor =14U,Cyc_Absyn_Bitxor =15U,Cyc_Absyn_Bitlshift =16U,Cyc_Absyn_Bitlrshift =17U,Cyc_Absyn_Numelts =18U};
# 581
enum Cyc_Absyn_Incrementor{Cyc_Absyn_PreInc =0U,Cyc_Absyn_PostInc =1U,Cyc_Absyn_PreDec =2U,Cyc_Absyn_PostDec =3U};struct Cyc_Absyn_VarargCallInfo{int num_varargs;struct Cyc_List_List*injectors;struct Cyc_Absyn_VarargInfo*vai;};struct Cyc_Absyn_StructField_Absyn_OffsetofField_struct{int tag;struct _fat_ptr*f1;};struct Cyc_Absyn_TupleIndex_Absyn_OffsetofField_struct{int tag;unsigned f1;};
# 599
enum Cyc_Absyn_Coercion{Cyc_Absyn_Unknown_coercion =0U,Cyc_Absyn_No_coercion =1U,Cyc_Absyn_Null_to_NonNull =2U,Cyc_Absyn_Other_coercion =3U};struct Cyc_Absyn_MallocInfo{int is_calloc;struct Cyc_Absyn_Exp*rgn;void**elt_type;struct Cyc_Absyn_Exp*num_elts;int fat_result;int inline_call;};struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct{int tag;union Cyc_Absyn_Cnst f1;};struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Pragma_e_Absyn_Raw_exp_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct{int tag;enum Cyc_Absyn_Primop f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;enum Cyc_Absyn_Incrementor f2;};struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct _tuple9{struct Cyc_List_List*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_VarargCallInfo*f3;int f4;struct _tuple9*f5;};struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;int f2;};struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Exp*f2;int f3;enum Cyc_Absyn_Coercion f4;};struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;int f3;int f4;};struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;int f3;int f4;};struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Tuple_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;};struct _tuple10{struct _fat_ptr*f1;struct Cyc_Absyn_Tqual f2;void*f3;};struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct{int tag;struct _tuple10*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;int f4;};struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;int f3;};struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct{int tag;struct _tuple1*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct Cyc_Absyn_Aggrdecl*f4;};struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Datatypedecl*f2;struct Cyc_Absyn_Datatypefield*f3;};struct Cyc_Absyn_Enum_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_Spawn_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;struct _tuple9*f3;};struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_MallocInfo f1;};struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Core_Opt*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Stmt*f1;};struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;};struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct{int tag;int f1;struct _fat_ptr f2;struct Cyc_List_List*f3;struct Cyc_List_List*f4;struct Cyc_List_List*f5;};struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Exp{void*topt;void*r;unsigned loc;void*annot;};struct Cyc_Absyn_Skip_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;struct Cyc_Absyn_Stmt*f3;};struct _tuple11{struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct{int tag;struct _tuple11 f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Break_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Continue_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Goto_s_Absyn_Raw_stmt_struct{int tag;struct _fat_ptr*f1;};struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _tuple11 f2;struct _tuple11 f3;struct Cyc_Absyn_Stmt*f4;};struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;void*f3;};struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Switch_clause**f2;};struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Decl*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct{int tag;struct _fat_ptr*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct _tuple11 f2;};struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_List_List*f2;void*f3;};struct Cyc_Absyn_Stmt{void*r;unsigned loc;void*annot;};struct Cyc_Absyn_Wild_p_Absyn_Raw_pat_struct{int tag;};struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_AliasVar_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_TagInt_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Tuple_p_Absyn_Raw_pat_struct{int tag;struct Cyc_List_List*f1;int f2;};struct Cyc_Absyn_Pointer_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Pat*f1;};struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct{int tag;union Cyc_Absyn_AggrInfo*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;int f4;};struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;struct Cyc_List_List*f3;int f4;};struct Cyc_Absyn_Null_p_Absyn_Raw_pat_struct{int tag;};struct Cyc_Absyn_Int_p_Absyn_Raw_pat_struct{int tag;enum Cyc_Absyn_Sign f1;int f2;};struct Cyc_Absyn_Char_p_Absyn_Raw_pat_struct{int tag;char f1;};struct Cyc_Absyn_Float_p_Absyn_Raw_pat_struct{int tag;struct _fat_ptr f1;int f2;};struct Cyc_Absyn_Enum_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_AnonEnum_p_Absyn_Raw_pat_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_UnknownId_p_Absyn_Raw_pat_struct{int tag;struct _tuple1*f1;};struct Cyc_Absyn_UnknownCall_p_Absyn_Raw_pat_struct{int tag;struct _tuple1*f1;struct Cyc_List_List*f2;int f3;};struct Cyc_Absyn_Exp_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Pat{void*r;void*topt;unsigned loc;};struct Cyc_Absyn_Switch_clause{struct Cyc_Absyn_Pat*pattern;struct Cyc_Core_Opt*pat_vars;struct Cyc_Absyn_Exp*where_clause;struct Cyc_Absyn_Stmt*body;unsigned loc;};struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct{int tag;struct _tuple1*f1;};struct Cyc_Absyn_Global_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Funname_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Param_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Local_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Pat_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Vardecl{enum Cyc_Absyn_Scope sc;struct _tuple1*name;unsigned varloc;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*initializer;void*rgn;struct Cyc_List_List*attributes;int escapes;};struct Cyc_Absyn_Fndecl{enum Cyc_Absyn_Scope sc;int is_inline;struct _tuple1*name;struct Cyc_Absyn_Stmt*body;struct Cyc_Absyn_FnInfo i;void*cached_type;struct Cyc_Core_Opt*param_vardecls;struct Cyc_Absyn_Vardecl*fn_vardecl;};struct Cyc_Absyn_Aggrfield{struct _fat_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*width;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*requires_clause;};struct Cyc_Absyn_AggrdeclImpl{struct Cyc_List_List*exist_vars;struct Cyc_List_List*rgn_po;struct Cyc_List_List*fields;int tagged;};struct Cyc_Absyn_Aggrdecl{enum Cyc_Absyn_AggrKind kind;enum Cyc_Absyn_Scope sc;struct _tuple1*name;struct Cyc_List_List*tvs;struct Cyc_Absyn_AggrdeclImpl*impl;struct Cyc_List_List*attributes;int expected_mem_kind;};struct Cyc_Absyn_Datatypefield{struct _tuple1*name;struct Cyc_List_List*typs;unsigned loc;enum Cyc_Absyn_Scope sc;};struct Cyc_Absyn_Datatypedecl{enum Cyc_Absyn_Scope sc;struct _tuple1*name;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*fields;int is_extensible;};struct Cyc_Absyn_Enumfield{struct _tuple1*name;struct Cyc_Absyn_Exp*tag;unsigned loc;};struct Cyc_Absyn_Enumdecl{enum Cyc_Absyn_Scope sc;struct _tuple1*name;struct Cyc_Core_Opt*fields;};struct Cyc_Absyn_Typedefdecl{struct _tuple1*name;struct Cyc_Absyn_Tqual tq;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*kind;void*defn;struct Cyc_List_List*atts;int extern_c;};struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Pat*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;void*f4;};struct Cyc_Absyn_Letv_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;};struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;};struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Typedefdecl*f1;};struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct{int tag;struct _fat_ptr*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct{int tag;struct _tuple1*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_ExternC_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;};struct _tuple12{unsigned f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct _tuple12*f4;};struct Cyc_Absyn_Porton_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Portoff_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Tempeston_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Tempestoff_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Decl{void*r;unsigned loc;};struct Cyc_Absyn_ArrayElement_Absyn_Designator_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_FieldName_Absyn_Designator_struct{int tag;struct _fat_ptr*f1;};extern char Cyc_Absyn_EmptyAnnot[11U];struct Cyc_Absyn_EmptyAnnot_Absyn_AbsynAnnot_struct{char*tag;};
# 956 "absyn.h"
int Cyc_Absyn_tvar_cmp(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*);
# 984
extern void*Cyc_Absyn_uint_type;
# 991
extern void*Cyc_Absyn_heap_rgn_type;extern void*Cyc_Absyn_refcnt_rgn_type;
# 993
extern void*Cyc_Absyn_empty_effect;
# 997
void*Cyc_Absyn_var_type(struct Cyc_Absyn_Tvar*);void*Cyc_Absyn_access_eff(void*);void*Cyc_Absyn_join_eff(struct Cyc_List_List*);struct Cyc_RgnOrder_RgnPO;
# 37 "rgnorder.h"
struct Cyc_RgnOrder_RgnPO*Cyc_RgnOrder_add_outlives_constraint(struct Cyc_RgnOrder_RgnPO*,void*eff,void*rgn,unsigned);
# 40
int Cyc_RgnOrder_effect_outlives(struct Cyc_RgnOrder_RgnPO*,void*eff,void*rgn);
# 42
int Cyc_RgnOrder_eff_outlives_eff(struct Cyc_RgnOrder_RgnPO*,void*eff1,void*eff2);
# 40 "warn.h"
void*Cyc_Warn_impos(struct _fat_ptr fmt,struct _fat_ptr ap);struct Cyc_Warn_String_Warn_Warg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Warn_Exp_Warn_Warg_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Warn_Typ_Warn_Warg_struct{int tag;void*f1;};struct Cyc_Warn_Qvar_Warn_Warg_struct{int tag;struct _tuple1*f1;};
# 29 "unify.h"
int Cyc_Unify_unify(void*,void*);struct _union_RelnOp_RConst{int tag;unsigned val;};struct _union_RelnOp_RVar{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RNumelts{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RType{int tag;void*val;};struct _union_RelnOp_RParam{int tag;unsigned val;};struct _union_RelnOp_RParamNumelts{int tag;unsigned val;};struct _union_RelnOp_RReturn{int tag;unsigned val;};union Cyc_Relations_RelnOp{struct _union_RelnOp_RConst RConst;struct _union_RelnOp_RVar RVar;struct _union_RelnOp_RNumelts RNumelts;struct _union_RelnOp_RType RType;struct _union_RelnOp_RParam RParam;struct _union_RelnOp_RParamNumelts RParamNumelts;struct _union_RelnOp_RReturn RReturn;};
# 50 "relations-ap.h"
enum Cyc_Relations_Relation{Cyc_Relations_Req =0U,Cyc_Relations_Rneq =1U,Cyc_Relations_Rlte =2U,Cyc_Relations_Rlt =3U};struct Cyc_Relations_Reln{union Cyc_Relations_RelnOp rop1;enum Cyc_Relations_Relation relation;union Cyc_Relations_RelnOp rop2;};extern char Cyc_Tcenv_Env_error[10U];struct Cyc_Tcenv_Env_error_exn_struct{char*tag;};struct Cyc_Tcenv_Genv{struct Cyc_Dict_Dict aggrdecls;struct Cyc_Dict_Dict datatypedecls;struct Cyc_Dict_Dict enumdecls;struct Cyc_Dict_Dict typedefs;struct Cyc_Dict_Dict ordinaries;};struct Cyc_Tcenv_Fenv;struct Cyc_Tcenv_Tenv{struct Cyc_List_List*ns;struct Cyc_Tcenv_Genv*ae;struct Cyc_Tcenv_Fenv*le;int allow_valueof: 1;int in_extern_c_include: 1;int in_tempest: 1;int tempest_generalize: 1;};
# 90 "tcenv.h"
enum Cyc_Tcenv_NewStatus{Cyc_Tcenv_NoneNew =0U,Cyc_Tcenv_InNew =1U,Cyc_Tcenv_InNewAggr =2U};
# 31 "tcutil.h"
void Cyc_Tcutil_terr(unsigned,struct _fat_ptr,struct _fat_ptr);
# 108
struct Cyc_Absyn_Kind*Cyc_Tcutil_tvar_kind(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Kind*def);
struct Cyc_Absyn_Kind*Cyc_Tcutil_type_kind(void*);
void*Cyc_Tcutil_compress(void*);
# 141
extern struct Cyc_Absyn_Kind Cyc_Tcutil_bk;
# 257
void*Cyc_Tcutil_normalize_effect(void*e);struct Cyc_PP_Ppstate;struct Cyc_PP_Out;struct Cyc_PP_Doc;struct Cyc_Absynpp_Params{int expand_typedefs;int qvar_to_Cids;int add_cyc_prefix;int to_VC;int decls_first;int rewrite_temp_tvars;int print_all_tvars;int print_all_kinds;int print_all_effects;int print_using_stmts;int print_externC_stmts;int print_full_evars;int print_zeroterm;int generate_line_directives;int use_curr_namespace;struct Cyc_List_List*curr_namespace;};
# 62 "absynpp.h"
struct _fat_ptr Cyc_Absynpp_typ2string(void*);
# 64
struct _fat_ptr Cyc_Absynpp_kind2string(struct Cyc_Absyn_Kind*);struct Cyc_RgnOrder_RgnPO{struct Cyc_Dict_Dict d;void*these_outlive_heap;void*these_outlive_unique;struct Cyc_Absyn_Tvar*youngest;void*opened_regions;};
# 68 "rgnorder.cyc"
static int Cyc_RgnOrder_valid_constraint(void*eff,void*rgn){
struct Cyc_Absyn_Kind*_stmttmp0=({Cyc_Tcutil_type_kind(rgn);});enum Cyc_Absyn_AliasQual _tmp1;enum Cyc_Absyn_KindQual _tmp0;_LL1: _tmp0=_stmttmp0->kind;_tmp1=_stmttmp0->aliasqual;_LL2: {enum Cyc_Absyn_KindQual k=_tmp0;enum Cyc_Absyn_AliasQual a=_tmp1;
if((int)k != (int)4U){
# 74
if((int)k == (int)3U)return 0;else{
# 76
({struct Cyc_String_pa_PrintArg_struct _tmp5=({struct Cyc_String_pa_PrintArg_struct _tmpE7;_tmpE7.tag=0U,({
struct _fat_ptr _tmpF2=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(rgn);}));_tmpE7.f1=_tmpF2;});_tmpE7;});void*_tmp2[1U];_tmp2[0]=& _tmp5;({int(*_tmpF4)(struct _fat_ptr fmt,struct _fat_ptr ap)=({
# 76
int(*_tmp4)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp4;});struct _fat_ptr _tmpF3=({const char*_tmp3="bad region type |%s| passed to add_outlives_constraint";_tag_fat(_tmp3,sizeof(char),55U);});_tmpF4(_tmpF3,_tag_fat(_tmp2,sizeof(void*),1U));});});}}{
# 70
void*_stmttmp1=({Cyc_Tcutil_compress(eff);});void*_tmp6=_stmttmp1;void*_tmp7;void*_tmp8;struct Cyc_List_List*_tmp9;switch(*((int*)_tmp6)){case 0U: switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp6)->f1)){case 11U: _LL4: _tmp9=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp6)->f2;_LL5: {struct Cyc_List_List*es=_tmp9;
# 99 "rgnorder.cyc"
for(0;es != 0;es=es->tl){
if(!({Cyc_RgnOrder_valid_constraint((void*)es->hd,rgn);}))return 0;}
# 99
return 1;}case 12U: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp6)->f2 != 0){_LLA: _tmp8=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp6)->f2)->hd;_LLB: {void*t=_tmp8;
# 108
struct Cyc_Absyn_Kind*_stmttmp2=({Cyc_Tcutil_type_kind(t);});enum Cyc_Absyn_AliasQual _tmpB;enum Cyc_Absyn_KindQual _tmpA;_LL13: _tmpA=_stmttmp2->kind;_tmpB=_stmttmp2->aliasqual;_LL14: {enum Cyc_Absyn_KindQual tk=_tmpA;enum Cyc_Absyn_AliasQual ta=_tmpB;
return(int)a == (int)0U ||(int)ta == (int)a;}}}else{goto _LL10;}case 9U: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp6)->f2 != 0){_LLC: _tmp7=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp6)->f2)->hd;_LLD: {void*r=_tmp7;
# 113
struct Cyc_Absyn_Kind*_stmttmp3=({Cyc_Tcutil_type_kind(r);});enum Cyc_Absyn_AliasQual _tmpD;enum Cyc_Absyn_KindQual _tmpC;_LL16: _tmpC=_stmttmp3->kind;_tmpD=_stmttmp3->aliasqual;_LL17: {enum Cyc_Absyn_KindQual rk=_tmpC;enum Cyc_Absyn_AliasQual ra=_tmpD;
return(int)k == (int)4U &&(
(int)a == (int)0U ||(int)ra == (int)a);}}}else{goto _LL10;}case 10U: _LLE: _LLF:
# 121
 return 1;default: goto _LL10;}case 1U: _LL6: _LL7:
# 103
 goto _LL9;case 2U: _LL8: _LL9:
 return 1;default: _LL10: _LL11:
# 124
({struct Cyc_String_pa_PrintArg_struct _tmp11=({struct Cyc_String_pa_PrintArg_struct _tmpE8;_tmpE8.tag=0U,({
struct _fat_ptr _tmpF5=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(eff);}));_tmpE8.f1=_tmpF5;});_tmpE8;});void*_tmpE[1U];_tmpE[0]=& _tmp11;({int(*_tmpF7)(struct _fat_ptr fmt,struct _fat_ptr ap)=({
# 124
int(*_tmp10)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp10;});struct _fat_ptr _tmpF6=({const char*_tmpF="bad effect |%s| passed to add_outlives_constraint";_tag_fat(_tmpF,sizeof(char),50U);});_tmpF7(_tmpF6,_tag_fat(_tmpE,sizeof(void*),1U));});});}_LL3:;}}}
# 68 "rgnorder.cyc"
struct Cyc_RgnOrder_RgnPO*Cyc_RgnOrder_add_outlives_constraint(struct Cyc_RgnOrder_RgnPO*po,void*eff,void*rgn,unsigned loc){
# 135 "rgnorder.cyc"
eff=({Cyc_Tcutil_normalize_effect(eff);});{
struct Cyc_RgnOrder_RgnPO*ans=({struct Cyc_RgnOrder_RgnPO*_tmp30=_cycalloc(sizeof(*_tmp30));*_tmp30=*po;_tmp30;});
# 138
if(!({Cyc_RgnOrder_valid_constraint(eff,rgn);})){
# 140
({struct Cyc_String_pa_PrintArg_struct _tmp15=({struct Cyc_String_pa_PrintArg_struct _tmpEA;_tmpEA.tag=0U,({struct _fat_ptr _tmpF8=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(eff);}));_tmpEA.f1=_tmpF8;});_tmpEA;});struct Cyc_String_pa_PrintArg_struct _tmp16=({struct Cyc_String_pa_PrintArg_struct _tmpE9;_tmpE9.tag=0U,({struct _fat_ptr _tmpF9=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(rgn);}));_tmpE9.f1=_tmpF9;});_tmpE9;});void*_tmp13[2U];_tmp13[0]=& _tmp15,_tmp13[1]=& _tmp16;({unsigned _tmpFB=loc;struct _fat_ptr _tmpFA=({const char*_tmp14="Invalid region ordering constraint; kind mismatch (effect = %s , rgn = %s";_tag_fat(_tmp14,sizeof(char),74U);});Cyc_Tcutil_terr(_tmpFB,_tmpFA,_tag_fat(_tmp13,sizeof(void*),2U));});});
return ans;}{
# 138
void*_stmttmp4=({Cyc_Tcutil_compress(rgn);});void*_tmp17=_stmttmp4;struct Cyc_Absyn_Tvar*_tmp18;switch(*((int*)_tmp17)){case 2U: _LL1: _tmp18=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp17)->f1;_LL2: {struct Cyc_Absyn_Tvar*tv=_tmp18;
# 147
struct Cyc_Dict_Dict d=po->d;
if(({({int(*_tmpFD)(struct Cyc_Dict_Dict d,struct Cyc_Absyn_Tvar*k)=({int(*_tmp19)(struct Cyc_Dict_Dict d,struct Cyc_Absyn_Tvar*k)=(int(*)(struct Cyc_Dict_Dict d,struct Cyc_Absyn_Tvar*k))Cyc_Dict_member;_tmp19;});struct Cyc_Dict_Dict _tmpFC=d;_tmpFD(_tmpFC,tv);});})){
void*old=({({void*(*_tmpFF)(struct Cyc_Dict_Dict d,struct Cyc_Absyn_Tvar*k)=({void*(*_tmp22)(struct Cyc_Dict_Dict d,struct Cyc_Absyn_Tvar*k)=(void*(*)(struct Cyc_Dict_Dict d,struct Cyc_Absyn_Tvar*k))Cyc_Dict_lookup;_tmp22;});struct Cyc_Dict_Dict _tmpFE=d;_tmpFF(_tmpFE,tv);});});
d=({struct Cyc_Dict_Dict(*_tmp1A)(struct Cyc_Dict_Dict d,struct Cyc_Absyn_Tvar*k,void*v)=({struct Cyc_Dict_Dict(*_tmp1B)(struct Cyc_Dict_Dict d,struct Cyc_Absyn_Tvar*k,void*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct Cyc_Absyn_Tvar*k,void*v))Cyc_Dict_insert;_tmp1B;});struct Cyc_Dict_Dict _tmp1C=d;struct Cyc_Absyn_Tvar*_tmp1D=tv;void*_tmp1E=({void*(*_tmp1F)(struct Cyc_List_List*)=Cyc_Absyn_join_eff;struct Cyc_List_List*_tmp20=({void*_tmp21[2U];_tmp21[0]=eff,_tmp21[1]=old;Cyc_List_list(_tag_fat(_tmp21,sizeof(void*),2U));});_tmp1F(_tmp20);});_tmp1A(_tmp1C,_tmp1D,_tmp1E);});}else{
# 152
d=({({struct Cyc_Dict_Dict(*_tmp102)(struct Cyc_Dict_Dict d,struct Cyc_Absyn_Tvar*k,void*v)=({struct Cyc_Dict_Dict(*_tmp23)(struct Cyc_Dict_Dict d,struct Cyc_Absyn_Tvar*k,void*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct Cyc_Absyn_Tvar*k,void*v))Cyc_Dict_insert;_tmp23;});struct Cyc_Dict_Dict _tmp101=d;struct Cyc_Absyn_Tvar*_tmp100=tv;_tmp102(_tmp101,_tmp100,eff);});});}
ans->d=d;
return ans;}case 1U: _LL3: _LL4:
# 156
({Cyc_Unify_unify(rgn,Cyc_Absyn_heap_rgn_type);});
goto _LL6;case 0U: switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp17)->f1)){case 6U: _LL5: _LL6:
# 159
({void*_tmp103=({void*(*_tmp24)(struct Cyc_List_List*)=Cyc_Absyn_join_eff;struct Cyc_List_List*_tmp25=({void*_tmp26[2U];_tmp26[0]=eff,_tmp26[1]=po->these_outlive_heap;Cyc_List_list(_tag_fat(_tmp26,sizeof(void*),2U));});_tmp24(_tmp25);});ans->these_outlive_heap=_tmp103;});
return ans;case 7U: _LL7: _LL8:
# 162
({void*_tmp104=({void*(*_tmp27)(struct Cyc_List_List*)=Cyc_Absyn_join_eff;struct Cyc_List_List*_tmp28=({void*_tmp29[2U];_tmp29[0]=eff,_tmp29[1]=po->these_outlive_unique;Cyc_List_list(_tag_fat(_tmp29,sizeof(void*),2U));});_tmp27(_tmp28);});ans->these_outlive_unique=_tmp104;});
return ans;case 8U: _LL9: _LLA:
# 165
({void*_tmp2A=0U;({int(*_tmp106)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp2C)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp2C;});struct _fat_ptr _tmp105=({const char*_tmp2B="RgnOrder::add_outlives_constraint can't outlive RC for now";_tag_fat(_tmp2B,sizeof(char),59U);});_tmp106(_tmp105,_tag_fat(_tmp2A,sizeof(void*),0U));});});default: goto _LLB;}default: _LLB: _LLC:
({void*_tmp2D=0U;({int(*_tmp108)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp2F)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp2F;});struct _fat_ptr _tmp107=({const char*_tmp2E="RgnOrder::add_outlives_constraint passed a bad region";_tag_fat(_tmp2E,sizeof(char),54U);});_tmp108(_tmp107,_tag_fat(_tmp2D,sizeof(void*),0U));});});}_LL0:;}}}
# 68 "rgnorder.cyc"
struct Cyc_RgnOrder_RgnPO*Cyc_RgnOrder_add_outlives_constraint_special(struct Cyc_RgnOrder_RgnPO*po,void*eff0,void*rgn){
# 171 "rgnorder.cyc"
eff0=({Cyc_Tcutil_normalize_effect(eff0);});{
struct Cyc_RgnOrder_RgnPO*ans=({struct Cyc_RgnOrder_RgnPO*_tmp42=_cycalloc(sizeof(*_tmp42));*_tmp42=*po;_tmp42;});
# 174
void*eff=({Cyc_Absyn_access_eff(eff0);});
# 176
{void*_stmttmp5=({Cyc_Tcutil_compress(rgn);});void*_tmp32=_stmttmp5;struct Cyc_Absyn_Tvar*_tmp33;if(((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp32)->tag == 2U){_LL1: _tmp33=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp32)->f1;_LL2: {struct Cyc_Absyn_Tvar*tv=_tmp33;
# 179
struct Cyc_Dict_Dict d=po->d;
if(({({int(*_tmp10A)(struct Cyc_Dict_Dict d,struct Cyc_Absyn_Tvar*k)=({int(*_tmp34)(struct Cyc_Dict_Dict d,struct Cyc_Absyn_Tvar*k)=(int(*)(struct Cyc_Dict_Dict d,struct Cyc_Absyn_Tvar*k))Cyc_Dict_member;_tmp34;});struct Cyc_Dict_Dict _tmp109=d;_tmp10A(_tmp109,tv);});})){
void*old=({({void*(*_tmp10C)(struct Cyc_Dict_Dict d,struct Cyc_Absyn_Tvar*k)=({void*(*_tmp3D)(struct Cyc_Dict_Dict d,struct Cyc_Absyn_Tvar*k)=(void*(*)(struct Cyc_Dict_Dict d,struct Cyc_Absyn_Tvar*k))Cyc_Dict_lookup;_tmp3D;});struct Cyc_Dict_Dict _tmp10B=d;_tmp10C(_tmp10B,tv);});});
d=({struct Cyc_Dict_Dict(*_tmp35)(struct Cyc_Dict_Dict d,struct Cyc_Absyn_Tvar*k,void*v)=({struct Cyc_Dict_Dict(*_tmp36)(struct Cyc_Dict_Dict d,struct Cyc_Absyn_Tvar*k,void*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct Cyc_Absyn_Tvar*k,void*v))Cyc_Dict_insert;_tmp36;});struct Cyc_Dict_Dict _tmp37=d;struct Cyc_Absyn_Tvar*_tmp38=tv;void*_tmp39=({void*(*_tmp3A)(struct Cyc_List_List*)=Cyc_Absyn_join_eff;struct Cyc_List_List*_tmp3B=({void*_tmp3C[2U];_tmp3C[0]=eff,_tmp3C[1]=old;Cyc_List_list(_tag_fat(_tmp3C,sizeof(void*),2U));});_tmp3A(_tmp3B);});_tmp35(_tmp37,_tmp38,_tmp39);});}else{
# 184
d=({({struct Cyc_Dict_Dict(*_tmp10F)(struct Cyc_Dict_Dict d,struct Cyc_Absyn_Tvar*k,void*v)=({struct Cyc_Dict_Dict(*_tmp3E)(struct Cyc_Dict_Dict d,struct Cyc_Absyn_Tvar*k,void*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct Cyc_Absyn_Tvar*k,void*v))Cyc_Dict_insert;_tmp3E;});struct Cyc_Dict_Dict _tmp10E=d;struct Cyc_Absyn_Tvar*_tmp10D=tv;_tmp10F(_tmp10E,_tmp10D,eff);});});}
ans->d=d;
goto _LL0;}}else{_LL3: _LL4:
({void*_tmp3F=0U;({int(*_tmp111)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp41)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp41;});struct _fat_ptr _tmp110=({const char*_tmp40="add_outlives_constraint_special";_tag_fat(_tmp40,sizeof(char),32U);});_tmp111(_tmp110,_tag_fat(_tmp3F,sizeof(void*),0U));});});}_LL0:;}
# 191
return ans;}}
# 68 "rgnorder.cyc"
struct Cyc_RgnOrder_RgnPO*Cyc_RgnOrder_add_youngest(struct Cyc_RgnOrder_RgnPO*po,struct Cyc_Absyn_Tvar*rgn,int opened){
# 196 "rgnorder.cyc"
if(!opened &&({({int(*_tmp113)(struct Cyc_Dict_Dict d,struct Cyc_Absyn_Tvar*k)=({int(*_tmp44)(struct Cyc_Dict_Dict d,struct Cyc_Absyn_Tvar*k)=(int(*)(struct Cyc_Dict_Dict d,struct Cyc_Absyn_Tvar*k))Cyc_Dict_member;_tmp44;});struct Cyc_Dict_Dict _tmp112=po->d;_tmp113(_tmp112,rgn);});}))
({void*_tmp45=0U;({int(*_tmp115)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp47)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp47;});struct _fat_ptr _tmp114=({const char*_tmp46="RgnOrder::add_youngest: repeated region";_tag_fat(_tmp46,sizeof(char),40U);});_tmp115(_tmp114,_tag_fat(_tmp45,sizeof(void*),0U));});});{
# 196
struct Cyc_RgnOrder_RgnPO*ans=({struct Cyc_RgnOrder_RgnPO*_tmp54=_cycalloc(sizeof(*_tmp54));*_tmp54=*po;_tmp54;});
# 199
if(opened){
# 201
({struct Cyc_Dict_Dict _tmp119=({({struct Cyc_Dict_Dict(*_tmp118)(struct Cyc_Dict_Dict d,struct Cyc_Absyn_Tvar*k,void*v)=({struct Cyc_Dict_Dict(*_tmp48)(struct Cyc_Dict_Dict d,struct Cyc_Absyn_Tvar*k,void*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct Cyc_Absyn_Tvar*k,void*v))Cyc_Dict_insert;_tmp48;});struct Cyc_Dict_Dict _tmp117=po->d;struct Cyc_Absyn_Tvar*_tmp116=rgn;_tmp118(_tmp117,_tmp116,Cyc_Absyn_empty_effect);});});ans->d=_tmp119;});
({void*_tmp11B=({void*(*_tmp49)(struct Cyc_List_List*)=Cyc_Absyn_join_eff;struct Cyc_List_List*_tmp4A=({void*_tmp4B[2U];({void*_tmp11A=({void*(*_tmp4C)(void*)=Cyc_Absyn_access_eff;void*_tmp4D=({Cyc_Absyn_var_type(rgn);});_tmp4C(_tmp4D);});_tmp4B[0]=_tmp11A;}),_tmp4B[1]=ans->opened_regions;Cyc_List_list(_tag_fat(_tmp4B,sizeof(void*),2U));});_tmp49(_tmp4A);});ans->opened_regions=_tmp11B;});}else{
# 206
void*eff=({void*(*_tmp4F)(struct Cyc_List_List*)=Cyc_Absyn_join_eff;struct Cyc_List_List*_tmp50=({void*_tmp51[2U];({void*_tmp11C=({void*(*_tmp52)(void*)=Cyc_Absyn_access_eff;void*_tmp53=({Cyc_Absyn_var_type(po->youngest);});_tmp52(_tmp53);});_tmp51[0]=_tmp11C;}),_tmp51[1]=ans->opened_regions;Cyc_List_list(_tag_fat(_tmp51,sizeof(void*),2U));});_tmp4F(_tmp50);});
# 208
({struct Cyc_Dict_Dict _tmp120=({({struct Cyc_Dict_Dict(*_tmp11F)(struct Cyc_Dict_Dict d,struct Cyc_Absyn_Tvar*k,void*v)=({struct Cyc_Dict_Dict(*_tmp4E)(struct Cyc_Dict_Dict d,struct Cyc_Absyn_Tvar*k,void*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct Cyc_Absyn_Tvar*k,void*v))Cyc_Dict_insert;_tmp4E;});struct Cyc_Dict_Dict _tmp11E=po->d;struct Cyc_Absyn_Tvar*_tmp11D=rgn;_tmp11F(_tmp11E,_tmp11D,eff);});});ans->d=_tmp120;});
ans->youngest=rgn;}
# 211
return ans;}}
# 68 "rgnorder.cyc"
struct Cyc_RgnOrder_RgnPO*Cyc_RgnOrder_add_unordered(struct Cyc_RgnOrder_RgnPO*po,struct Cyc_Absyn_Tvar*rgn){
# 215 "rgnorder.cyc"
if(({({int(*_tmp122)(struct Cyc_Dict_Dict d,struct Cyc_Absyn_Tvar*k)=({int(*_tmp56)(struct Cyc_Dict_Dict d,struct Cyc_Absyn_Tvar*k)=(int(*)(struct Cyc_Dict_Dict d,struct Cyc_Absyn_Tvar*k))Cyc_Dict_member;_tmp56;});struct Cyc_Dict_Dict _tmp121=po->d;_tmp122(_tmp121,rgn);});}))
({void*_tmp57=0U;({int(*_tmp124)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp59)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp59;});struct _fat_ptr _tmp123=({const char*_tmp58="RgnOrder::add_unordered: repeated region";_tag_fat(_tmp58,sizeof(char),41U);});_tmp124(_tmp123,_tag_fat(_tmp57,sizeof(void*),0U));});});{
# 215
struct Cyc_RgnOrder_RgnPO*ans=({struct Cyc_RgnOrder_RgnPO*_tmp5B=_cycalloc(sizeof(*_tmp5B));*_tmp5B=*po;_tmp5B;});
# 218
({struct Cyc_Dict_Dict _tmp128=({({struct Cyc_Dict_Dict(*_tmp127)(struct Cyc_Dict_Dict d,struct Cyc_Absyn_Tvar*k,void*v)=({struct Cyc_Dict_Dict(*_tmp5A)(struct Cyc_Dict_Dict d,struct Cyc_Absyn_Tvar*k,void*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct Cyc_Absyn_Tvar*k,void*v))Cyc_Dict_insert;_tmp5A;});struct Cyc_Dict_Dict _tmp126=ans->d;struct Cyc_Absyn_Tvar*_tmp125=rgn;_tmp127(_tmp126,_tmp125,Cyc_Absyn_empty_effect);});});ans->d=_tmp128;});
return ans;}}
# 68 "rgnorder.cyc"
struct Cyc_RgnOrder_RgnPO*Cyc_RgnOrder_initial_fn_po(struct Cyc_List_List*tvs,struct Cyc_List_List*po,void*effect,struct Cyc_Absyn_Tvar*fst_rgn,unsigned loc){
# 225 "rgnorder.cyc"
struct Cyc_Dict_Dict d=({({struct Cyc_Dict_Dict(*_tmp64)(int(*cmp)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*))=(struct Cyc_Dict_Dict(*)(int(*cmp)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*)))Cyc_Dict_empty;_tmp64;})(Cyc_Absyn_tvar_cmp);});
{struct Cyc_List_List*tvs2=tvs;for(0;tvs2 != 0;tvs2=tvs2->tl){
if((int)({Cyc_Tcutil_tvar_kind((struct Cyc_Absyn_Tvar*)tvs2->hd,& Cyc_Tcutil_bk);})->kind == (int)Cyc_Absyn_RgnKind)
# 230
d=({({struct Cyc_Dict_Dict(*_tmp12B)(struct Cyc_Dict_Dict d,struct Cyc_Absyn_Tvar*k,void*v)=({struct Cyc_Dict_Dict(*_tmp5D)(struct Cyc_Dict_Dict d,struct Cyc_Absyn_Tvar*k,void*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct Cyc_Absyn_Tvar*k,void*v))Cyc_Dict_insert;_tmp5D;});struct Cyc_Dict_Dict _tmp12A=d;struct Cyc_Absyn_Tvar*_tmp129=(struct Cyc_Absyn_Tvar*)tvs2->hd;_tmp12B(_tmp12A,_tmp129,Cyc_Absyn_empty_effect);});});}}
# 226
if(!({int(*_tmp5E)(void*eff,void*rgn)=Cyc_RgnOrder_valid_constraint;void*_tmp5F=effect;void*_tmp60=({Cyc_Absyn_var_type(fst_rgn);});_tmp5E(_tmp5F,_tmp60);}))
# 236
return({struct Cyc_RgnOrder_RgnPO*_tmp61=_cycalloc(sizeof(*_tmp61));_tmp61->d=d,_tmp61->these_outlive_heap=Cyc_Absyn_empty_effect,_tmp61->these_outlive_unique=Cyc_Absyn_empty_effect,_tmp61->youngest=fst_rgn,_tmp61->opened_regions=Cyc_Absyn_empty_effect;_tmp61;});
# 226
d=({({struct Cyc_Dict_Dict(*_tmp12E)(struct Cyc_Dict_Dict d,struct Cyc_Absyn_Tvar*k,void*v)=({
# 241
struct Cyc_Dict_Dict(*_tmp62)(struct Cyc_Dict_Dict d,struct Cyc_Absyn_Tvar*k,void*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct Cyc_Absyn_Tvar*k,void*v))Cyc_Dict_insert;_tmp62;});struct Cyc_Dict_Dict _tmp12D=d;struct Cyc_Absyn_Tvar*_tmp12C=fst_rgn;_tmp12E(_tmp12D,_tmp12C,effect);});});{
# 243
struct Cyc_RgnOrder_RgnPO*ans=({struct Cyc_RgnOrder_RgnPO*_tmp63=_cycalloc(sizeof(*_tmp63));_tmp63->d=d,_tmp63->these_outlive_heap=Cyc_Absyn_empty_effect,_tmp63->these_outlive_unique=Cyc_Absyn_empty_effect,_tmp63->youngest=fst_rgn,_tmp63->opened_regions=Cyc_Absyn_empty_effect;_tmp63;});
for(0;po != 0;po=po->tl){
ans=({Cyc_RgnOrder_add_outlives_constraint(ans,(*((struct _tuple0*)po->hd)).f1,(*((struct _tuple0*)po->hd)).f2,loc);});}
# 247
return ans;}}
# 251
static int Cyc_RgnOrder_contains_rgnseff(struct Cyc_Absyn_Tvar*rgns_of_var,void*eff){
void*_stmttmp6=({void*(*_tmp6B)(void*e)=Cyc_Tcutil_normalize_effect;void*_tmp6C=({Cyc_Tcutil_compress(eff);});_tmp6B(_tmp6C);});void*_tmp66=_stmttmp6;struct Cyc_List_List*_tmp67;void*_tmp68;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp66)->tag == 0U)switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp66)->f1)){case 12U: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp66)->f2 != 0){_LL1: _tmp68=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp66)->f2)->hd;_LL2: {void*t=_tmp68;
# 254
void*_stmttmp7=({Cyc_Tcutil_compress(t);});void*_tmp69=_stmttmp7;struct Cyc_Absyn_Tvar*_tmp6A;if(((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp69)->tag == 2U){_LL8: _tmp6A=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp69)->f1;_LL9: {struct Cyc_Absyn_Tvar*tv=_tmp6A;
return({Cyc_Absyn_tvar_cmp(tv,rgns_of_var);})== 0;}}else{_LLA: _LLB:
 return 0;}_LL7:;}}else{goto _LL5;}case 11U: _LL3: _tmp67=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp66)->f2;_LL4: {struct Cyc_List_List*effs=_tmp67;
# 259
for(0;effs != 0;effs=effs->tl){
if(({Cyc_RgnOrder_contains_rgnseff(rgns_of_var,(void*)effs->hd);}))
return 1;}
# 259
return 0;}default: goto _LL5;}else{_LL5: _LL6:
# 263
 return 0;}_LL0:;}struct Cyc_RgnOrder_OutlivesEnv{struct _RegionHandle*r;struct Cyc_List_List*seen;struct Cyc_List_List*todo;};
# 273
static void Cyc_RgnOrder_add_to_search(struct Cyc_RgnOrder_OutlivesEnv*env,void*eff){
void*_stmttmp8=({Cyc_Tcutil_compress(eff);});void*_tmp6E=_stmttmp8;struct Cyc_List_List*_tmp6F;struct Cyc_Absyn_Tvar*_tmp70;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp6E)->tag == 0U)switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp6E)->f1)){case 9U: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp6E)->f2 != 0){if(((struct Cyc_Absyn_VarType_Absyn_Type_struct*)((struct Cyc_List_List*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp6E)->f2)->hd)->tag == 2U){_LL1: _tmp70=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp6E)->f2)->hd)->f1;_LL2: {struct Cyc_Absyn_Tvar*tv=_tmp70;
# 276
{struct Cyc_List_List*seen=env->seen;for(0;seen != 0;seen=seen->tl){
if(({Cyc_Absyn_tvar_cmp(tv,(struct Cyc_Absyn_Tvar*)seen->hd);})== 0)
return;}}
# 276
({struct Cyc_List_List*_tmp12F=({struct Cyc_List_List*_tmp71=_region_malloc(env->r,sizeof(*_tmp71));
# 279
_tmp71->hd=tv,_tmp71->tl=env->seen;_tmp71;});
# 276
env->seen=_tmp12F;});
# 280
({struct Cyc_List_List*_tmp130=({struct Cyc_List_List*_tmp72=_region_malloc(env->r,sizeof(*_tmp72));_tmp72->hd=tv,_tmp72->tl=env->todo;_tmp72;});env->todo=_tmp130;});
return;}}else{goto _LL5;}}else{goto _LL5;}case 11U: _LL3: _tmp6F=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp6E)->f2;_LL4: {struct Cyc_List_List*effs=_tmp6F;
# 283
for(0;effs != 0;effs=effs->tl){
({Cyc_RgnOrder_add_to_search(env,(void*)effs->hd);});}
return;}default: goto _LL5;}else{_LL5: _LL6:
# 290
 return;}_LL0:;}
# 294
static struct Cyc_RgnOrder_OutlivesEnv Cyc_RgnOrder_initial_env(struct _RegionHandle*listrgn,struct Cyc_RgnOrder_RgnPO*po,void*rgn){
# 299
struct Cyc_RgnOrder_OutlivesEnv ans=({struct Cyc_RgnOrder_OutlivesEnv _tmpEC;_tmpEC.r=listrgn,_tmpEC.seen=0,_tmpEC.todo=0;_tmpEC;});
void*r=({Cyc_Tcutil_compress(rgn);});
struct Cyc_Absyn_Kind*_stmttmp9=({Cyc_Tcutil_type_kind(r);});enum Cyc_Absyn_AliasQual _tmp75;enum Cyc_Absyn_KindQual _tmp74;_LL1: _tmp74=_stmttmp9->kind;_tmp75=_stmttmp9->aliasqual;_LL2: {enum Cyc_Absyn_KindQual k=_tmp74;enum Cyc_Absyn_AliasQual a=_tmp75;
if((int)k == (int)4U){
# 304
{enum Cyc_Absyn_AliasQual _tmp76=a;switch(_tmp76){case Cyc_Absyn_Aliasable: _LL4: _LL5:
# 307
({Cyc_RgnOrder_add_to_search(& ans,po->these_outlive_unique);});
({Cyc_RgnOrder_add_to_search(& ans,po->these_outlive_heap);});
goto _LL3;case Cyc_Absyn_Unique: _LL6: _LL7:
# 311
({Cyc_RgnOrder_add_to_search(& ans,po->these_outlive_unique);});goto _LL3;case Cyc_Absyn_Top: _LL8: _LL9:
 goto _LLB;default: _LLA: _LLB:
# 316
 goto _LL3;}_LL3:;}{
# 318
void*_tmp77=r;struct Cyc_Absyn_Tvar*_tmp78;if(((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp77)->tag == 2U){_LLD: _tmp78=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp77)->f1;_LLE: {struct Cyc_Absyn_Tvar*tv=_tmp78;
# 321
({struct Cyc_List_List*_tmp131=({struct Cyc_List_List*_tmp79=_region_malloc(listrgn,sizeof(*_tmp79));_tmp79->hd=tv,_tmp79->tl=ans.seen;_tmp79;});ans.seen=_tmp131;});
({struct Cyc_List_List*_tmp132=({struct Cyc_List_List*_tmp7A=_region_malloc(listrgn,sizeof(*_tmp7A));_tmp7A->hd=tv,_tmp7A->tl=ans.todo;_tmp7A;});ans.todo=_tmp132;});
return ans;}}else{_LLF: _LL10:
 return ans;}_LLC:;}}else{
# 327
if((int)k == (int)3U)return ans;else{
# 329
({void*_tmp7B=0U;({int(*_tmp136)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp81)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp81;});struct _fat_ptr _tmp135=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp7E=({struct Cyc_String_pa_PrintArg_struct _tmpEB;_tmpEB.tag=0U,({struct _fat_ptr _tmp133=(struct _fat_ptr)((struct _fat_ptr)({struct _fat_ptr(*_tmp7F)(struct Cyc_Absyn_Kind*)=Cyc_Absynpp_kind2string;struct Cyc_Absyn_Kind*_tmp80=({Cyc_Tcutil_type_kind(r);});_tmp7F(_tmp80);}));_tmpEB.f1=_tmp133;});_tmpEB;});void*_tmp7C[1U];_tmp7C[0]=& _tmp7E;({struct _fat_ptr _tmp134=({const char*_tmp7D="RgnOrder: rgn not of correct kind: expected RgnKind but found %s ";_tag_fat(_tmp7D,sizeof(char),66U);});Cyc_aprintf(_tmp134,_tag_fat(_tmp7C,sizeof(void*),1U));});});_tmp136(_tmp135,_tag_fat(_tmp7B,sizeof(void*),0U));});});}}}}
# 335
static int Cyc_RgnOrder_atomic_effect_outlives(struct Cyc_RgnOrder_RgnPO*po,void*eff,void*rgn){
# 337
struct _RegionHandle _tmp83=_new_region("listrgn");struct _RegionHandle*listrgn=& _tmp83;_push_region(listrgn);
{void*_stmttmpA=({Cyc_Tcutil_compress(eff);});void*_tmp84=_stmttmpA;void*_tmp85;void*_tmp86;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp84)->tag == 0U)switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp84)->f1)){case 9U: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp84)->f2 != 0){_LL1: _tmp86=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp84)->f2)->hd;_LL2: {void*vt=_tmp86;
# 341
void*_stmttmpB=({Cyc_Tcutil_compress(vt);});void*_tmp87=_stmttmpB;struct Cyc_Absyn_Tvar*_tmp88;switch(*((int*)_tmp87)){case 0U: switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp87)->f1)){case 7U: _LL8: _LL9: {
# 345
struct Cyc_Absyn_Kind*_stmttmpC=({struct Cyc_Absyn_Kind*(*_tmp8C)(void*)=Cyc_Tcutil_type_kind;void*_tmp8D=({Cyc_Tcutil_compress(rgn);});_tmp8C(_tmp8D);});enum Cyc_Absyn_AliasQual _tmp8A;enum Cyc_Absyn_KindQual _tmp89;_LL13: _tmp89=_stmttmpC->kind;_tmp8A=_stmttmpC->aliasqual;_LL14: {enum Cyc_Absyn_KindQual k=_tmp89;enum Cyc_Absyn_AliasQual a=_tmp8A;
int _tmp8B=(int)k == (int)4U &&(int)a != (int)Cyc_Absyn_Top;_npop_handler(0U);return _tmp8B;}}case 8U: _LLA: _LLB:
# 350
 if(({void*_tmp137=({Cyc_Tcutil_compress(rgn);});_tmp137 == Cyc_Absyn_refcnt_rgn_type;})){int _tmp8E=1;_npop_handler(0U);return _tmp8E;}goto _LLD;case 6U: _LLC: _LLD: {
# 354
struct Cyc_Absyn_Kind*_stmttmpD=({struct Cyc_Absyn_Kind*(*_tmp92)(void*)=Cyc_Tcutil_type_kind;void*_tmp93=({Cyc_Tcutil_compress(rgn);});_tmp92(_tmp93);});enum Cyc_Absyn_AliasQual _tmp90;enum Cyc_Absyn_KindQual _tmp8F;_LL16: _tmp8F=_stmttmpD->kind;_tmp90=_stmttmpD->aliasqual;_LL17: {enum Cyc_Absyn_KindQual k=_tmp8F;enum Cyc_Absyn_AliasQual a=_tmp90;
int _tmp91=(int)k == (int)4U &&(int)a == (int)Cyc_Absyn_Aliasable;_npop_handler(0U);return _tmp91;}}default: goto _LL10;}case 2U: _LLE: _tmp88=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp87)->f1;_LLF: {struct Cyc_Absyn_Tvar*tv=_tmp88;
# 357
if(po == 0)
({void*_tmp94=0U;({int(*_tmp139)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp96)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp96;});struct _fat_ptr _tmp138=({const char*_tmp95="RgnOrder: tvar without a partial-order";_tag_fat(_tmp95,sizeof(char),39U);});_tmp139(_tmp138,_tag_fat(_tmp94,sizeof(void*),0U));});});{
# 357
struct Cyc_RgnOrder_OutlivesEnv env=({Cyc_RgnOrder_initial_env(listrgn,po,rgn);});
# 370 "rgnorder.cyc"
while(env.todo != 0){
# 372
struct Cyc_Absyn_Tvar*next=(struct Cyc_Absyn_Tvar*)((struct Cyc_List_List*)_check_null(env.todo))->hd;
env.todo=((struct Cyc_List_List*)_check_null(env.todo))->tl;
if(({Cyc_Absyn_tvar_cmp(next,tv);})== 0){
int _tmp97=1;_npop_handler(0U);return _tmp97;}
# 374
if(({({int(*_tmp13B)(struct Cyc_Dict_Dict d,struct Cyc_Absyn_Tvar*k)=({
# 380
int(*_tmp98)(struct Cyc_Dict_Dict d,struct Cyc_Absyn_Tvar*k)=(int(*)(struct Cyc_Dict_Dict d,struct Cyc_Absyn_Tvar*k))Cyc_Dict_member;_tmp98;});struct Cyc_Dict_Dict _tmp13A=po->d;_tmp13B(_tmp13A,next);});}))
({void(*_tmp99)(struct Cyc_RgnOrder_OutlivesEnv*env,void*eff)=Cyc_RgnOrder_add_to_search;struct Cyc_RgnOrder_OutlivesEnv*_tmp9A=& env;void*_tmp9B=({({void*(*_tmp13D)(struct Cyc_Dict_Dict d,struct Cyc_Absyn_Tvar*k)=({void*(*_tmp9C)(struct Cyc_Dict_Dict d,struct Cyc_Absyn_Tvar*k)=(void*(*)(struct Cyc_Dict_Dict d,struct Cyc_Absyn_Tvar*k))Cyc_Dict_lookup;_tmp9C;});struct Cyc_Dict_Dict _tmp13C=po->d;_tmp13D(_tmp13C,next);});});_tmp99(_tmp9A,_tmp9B);});else{
# 383
int _tmp9D=0;_npop_handler(0U);return _tmp9D;}}{
# 386
int _tmp9E=0;_npop_handler(0U);return _tmp9E;}}}default: _LL10: _LL11: {
int _tmp9F=0;_npop_handler(0U);return _tmp9F;}}_LL7:;}}else{goto _LL5;}case 12U: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp84)->f2 != 0){_LL3: _tmp85=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp84)->f2)->hd;_LL4: {void*vt=_tmp85;
# 390
void*_stmttmpE=({Cyc_Tcutil_compress(vt);});void*_tmpA0=_stmttmpE;struct Cyc_Absyn_Tvar*_tmpA1;if(((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmpA0)->tag == 2U){_LL19: _tmpA1=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmpA0)->f1;_LL1A: {struct Cyc_Absyn_Tvar*tv=_tmpA1;
# 393
if(po == 0)
({void*_tmpA2=0U;({int(*_tmp13F)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmpA4)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmpA4;});struct _fat_ptr _tmp13E=({const char*_tmpA3="RgnOrder: tvar without a partial-order";_tag_fat(_tmpA3,sizeof(char),39U);});_tmp13F(_tmp13E,_tag_fat(_tmpA2,sizeof(void*),0U));});});{
# 393
struct Cyc_RgnOrder_OutlivesEnv env=({Cyc_RgnOrder_initial_env(listrgn,po,rgn);});
# 399
struct Cyc_Absyn_Kind*_stmttmpF=({Cyc_Tcutil_type_kind(rgn);});enum Cyc_Absyn_AliasQual _tmpA6;enum Cyc_Absyn_KindQual _tmpA5;_LL1E: _tmpA5=_stmttmpF->kind;_tmpA6=_stmttmpF->aliasqual;_LL1F: {enum Cyc_Absyn_KindQual k=_tmpA5;enum Cyc_Absyn_AliasQual a=_tmpA6;
# 401
if((int)k == (int)4U){
if((int)a == (int)0U){
if(({Cyc_RgnOrder_contains_rgnseff(tv,po->these_outlive_heap);})||({Cyc_RgnOrder_contains_rgnseff(tv,po->these_outlive_unique);})){
# 405
int _tmpA7=1;_npop_handler(0U);return _tmpA7;}}else{
# 407
if((int)a == (int)1U){
if(({Cyc_RgnOrder_contains_rgnseff(tv,po->these_outlive_unique);})){
int _tmpA8=1;_npop_handler(0U);return _tmpA8;}}}}
# 401
while(env.todo != 0){
# 413
struct Cyc_Absyn_Tvar*next=(struct Cyc_Absyn_Tvar*)((struct Cyc_List_List*)_check_null(env.todo))->hd;
env.todo=((struct Cyc_List_List*)_check_null(env.todo))->tl;
if(({({int(*_tmp141)(struct Cyc_Dict_Dict d,struct Cyc_Absyn_Tvar*k)=({int(*_tmpA9)(struct Cyc_Dict_Dict d,struct Cyc_Absyn_Tvar*k)=(int(*)(struct Cyc_Dict_Dict d,struct Cyc_Absyn_Tvar*k))Cyc_Dict_member;_tmpA9;});struct Cyc_Dict_Dict _tmp140=po->d;_tmp141(_tmp140,next);});})){
void*next_eff=({({void*(*_tmp143)(struct Cyc_Dict_Dict d,struct Cyc_Absyn_Tvar*k)=({void*(*_tmpAB)(struct Cyc_Dict_Dict d,struct Cyc_Absyn_Tvar*k)=(void*(*)(struct Cyc_Dict_Dict d,struct Cyc_Absyn_Tvar*k))Cyc_Dict_lookup;_tmpAB;});struct Cyc_Dict_Dict _tmp142=po->d;_tmp143(_tmp142,next);});});
if(({Cyc_RgnOrder_contains_rgnseff(tv,next_eff);})){
int _tmpAA=1;_npop_handler(0U);return _tmpAA;}
# 417
({Cyc_RgnOrder_add_to_search(& env,next_eff);});}else{
# 420
({void*_tmpAC=0U;({int(*_tmp145)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmpAE)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmpAE;});struct _fat_ptr _tmp144=({const char*_tmpAD="RgnOrder: type variable not in dict";_tag_fat(_tmpAD,sizeof(char),36U);});_tmp145(_tmp144,_tag_fat(_tmpAC,sizeof(void*),0U));});});}}{
# 422
int _tmpAF=0;_npop_handler(0U);return _tmpAF;}}}}}else{_LL1B: _LL1C: {
int _tmpB0=0;_npop_handler(0U);return _tmpB0;}}_LL18:;}}else{goto _LL5;}default: goto _LL5;}else{_LL5: _LL6: {
# 428
int _tmpB1=0;_npop_handler(0U);return _tmpB1;}}_LL0:;}
# 338 "rgnorder.cyc"
;_pop_region();}
# 335
int Cyc_RgnOrder_effect_outlives(struct Cyc_RgnOrder_RgnPO*po,void*eff,void*rgn){
# 436 "rgnorder.cyc"
eff=({Cyc_Tcutil_normalize_effect(eff);});{
void*_stmttmp10=({Cyc_Tcutil_compress(eff);});void*_tmpB3=_stmttmp10;struct Cyc_List_List*_tmpB4;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpB3)->tag == 0U){if(((struct Cyc_Absyn_JoinCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpB3)->f1)->tag == 11U){_LL1: _tmpB4=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpB3)->f2;_LL2: {struct Cyc_List_List*effs=_tmpB4;
# 439
for(0;effs != 0;effs=effs->tl){
if(!({Cyc_RgnOrder_effect_outlives(po,(void*)effs->hd,rgn);}))
return 0;}
# 439
return 1;}}else{goto _LL3;}}else{_LL3: _LL4:
# 443
 return({Cyc_RgnOrder_atomic_effect_outlives(po,eff,rgn);});}_LL0:;}}
# 447
static void Cyc_RgnOrder_pin_effect(void*eff,void*bd){
eff=({Cyc_Tcutil_normalize_effect(eff);});{
void*_stmttmp11=({Cyc_Tcutil_compress(eff);});void*_tmpB6=_stmttmp11;void*_tmpB7;void*_tmpB8;struct Cyc_List_List*_tmpB9;switch(*((int*)_tmpB6)){case 0U: switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpB6)->f1)){case 11U: _LL1: _tmpB9=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpB6)->f2;_LL2: {struct Cyc_List_List*effs=_tmpB9;
# 451
for(0;effs != 0;effs=effs->tl){
({Cyc_RgnOrder_pin_effect((void*)effs->hd,bd);});}
return;}case 12U: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpB6)->f2 != 0){_LL3: _tmpB8=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpB6)->f2)->hd;_LL4: {void*t=_tmpB8;
# 455
void*_stmttmp12=({Cyc_Tcutil_compress(t);});void*_tmpBA=_stmttmp12;if(((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmpBA)->tag == 1U){_LLC: _LLD:
({Cyc_Unify_unify(t,Cyc_Absyn_uint_type);});return;}else{_LLE: _LLF:
 return;}_LLB:;}}else{goto _LL9;}case 9U: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpB6)->f2 != 0){_LL5: _tmpB7=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpB6)->f2)->hd;_LL6: {void*r=_tmpB7;
# 460
void*_stmttmp13=({Cyc_Tcutil_compress(r);});void*_tmpBB=_stmttmp13;if(((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmpBB)->tag == 1U){_LL11: _LL12:
({Cyc_Unify_unify(r,bd);});return;}else{_LL13: _LL14:
 return;}_LL10:;}}else{goto _LL9;}default: goto _LL9;}case 1U: _LL7: _LL8:
# 464
({Cyc_Unify_unify(eff,Cyc_Absyn_empty_effect);});return;default: _LL9: _LLA:
# 468
 return;}_LL0:;}}
# 447
int Cyc_RgnOrder_rgn_outlives_rgn(struct Cyc_RgnOrder_RgnPO*po,void*rgn1,void*rgn2){
# 475
return({int(*_tmpBD)(struct Cyc_RgnOrder_RgnPO*po,void*eff,void*rgn)=Cyc_RgnOrder_effect_outlives;struct Cyc_RgnOrder_RgnPO*_tmpBE=po;void*_tmpBF=({Cyc_Absyn_access_eff(rgn1);});void*_tmpC0=rgn2;_tmpBD(_tmpBE,_tmpBF,_tmpC0);});}
# 478
static int Cyc_RgnOrder_eff_outlives_atomic_eff(struct Cyc_RgnOrder_RgnPO*po,void*eff1,void*eff2){
# 480
eff2=({void*(*_tmpC2)(void*)=Cyc_Tcutil_compress;void*_tmpC3=({Cyc_Tcutil_normalize_effect(eff2);});_tmpC2(_tmpC3);});{
void*_tmpC4=eff2;void*_tmpC5;void*_tmpC6;struct Cyc_List_List*_tmpC7;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpC4)->tag == 0U)switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpC4)->f1)){case 11U: _LL1: _tmpC7=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpC4)->f2;_LL2: {struct Cyc_List_List*effs=_tmpC7;
# 483
for(0;effs != 0;effs=effs->tl){
if(({Cyc_RgnOrder_eff_outlives_atomic_eff(po,eff1,(void*)effs->hd);}))
return 1;}
# 483
return 0;}case 9U: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpC4)->f2 != 0){_LL3: _tmpC6=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpC4)->f2)->hd;_LL4: {void*vt=_tmpC6;
# 487
return({Cyc_RgnOrder_effect_outlives(po,eff1,vt);});}}else{goto _LL7;}case 12U: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpC4)->f2 != 0){_LL5: _tmpC5=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpC4)->f2)->hd;_LL6: {void*vt1=_tmpC5;
# 489
void*_tmpC8=eff1;void*_tmpC9;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpC8)->tag == 0U){if(((struct Cyc_Absyn_RgnsCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpC8)->f1)->tag == 12U){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpC8)->f2 != 0){_LLA: _tmpC9=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpC8)->f2)->hd;_LLB: {void*vt2=_tmpC9;
# 491
{struct _tuple0 _stmttmp14=({struct _tuple0 _tmpED;({void*_tmp147=({Cyc_Tcutil_compress(vt1);});_tmpED.f1=_tmp147;}),({void*_tmp146=({Cyc_Tcutil_compress(vt2);});_tmpED.f2=_tmp146;});_tmpED;});struct _tuple0 _tmpCA=_stmttmp14;struct Cyc_Absyn_Tvar*_tmpCC;struct Cyc_Absyn_Tvar*_tmpCB;if(((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmpCA.f1)->tag == 2U){if(((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmpCA.f2)->tag == 2U){_LLF: _tmpCB=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmpCA.f1)->f1;_tmpCC=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmpCA.f2)->f1;_LL10: {struct Cyc_Absyn_Tvar*tv1=_tmpCB;struct Cyc_Absyn_Tvar*tv2=_tmpCC;
return({Cyc_Absyn_tvar_cmp(tv1,tv2);})== 0;}}else{goto _LL11;}}else{_LL11: _LL12:
 goto _LLE;}_LLE:;}
# 495
goto _LLD;}}else{goto _LLC;}}else{goto _LLC;}}else{_LLC: _LLD:
 return eff1 == Cyc_Absyn_heap_rgn_type;}_LL9:;}}else{goto _LL7;}default: goto _LL7;}else{_LL7: _LL8:
# 502
 return eff1 == Cyc_Absyn_heap_rgn_type;}_LL0:;}}
# 478
int Cyc_RgnOrder_eff_outlives_eff(struct Cyc_RgnOrder_RgnPO*po,void*eff1,void*eff2){
# 507
eff1=({void*(*_tmpCE)(void*)=Cyc_Tcutil_compress;void*_tmpCF=({Cyc_Tcutil_normalize_effect(eff1);});_tmpCE(_tmpCF);});{
void*_tmpD0=eff1;struct Cyc_List_List*_tmpD1;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpD0)->tag == 0U){if(((struct Cyc_Absyn_JoinCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpD0)->f1)->tag == 11U){_LL1: _tmpD1=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpD0)->f2;_LL2: {struct Cyc_List_List*effs=_tmpD1;
# 510
for(0;effs != 0;effs=effs->tl){
if(!({Cyc_RgnOrder_eff_outlives_eff(po,(void*)effs->hd,eff2);}))
return 0;}
# 510
return 1;}}else{goto _LL3;}}else{_LL3: _LL4:
# 514
 return({Cyc_RgnOrder_eff_outlives_atomic_eff(po,eff1,eff2);});}_LL0:;}}
# 478
int Cyc_RgnOrder_satisfies_constraints(struct Cyc_RgnOrder_RgnPO*po,struct Cyc_List_List*constraints,void*default_bound,int do_pin){
# 532 "rgnorder.cyc"
{struct Cyc_List_List*cs=constraints;for(0;cs != 0;cs=cs->tl){
struct _tuple0*_stmttmp15=(struct _tuple0*)cs->hd;void*_tmpD3;_LL1: _tmpD3=_stmttmp15->f2;_LL2: {void*bd=_tmpD3;
void*_stmttmp16=({Cyc_Tcutil_compress(bd);});void*_tmpD4=_stmttmp16;if(((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmpD4)->tag == 1U){_LL4: _LL5:
# 536
 if(do_pin)
({Cyc_Unify_unify(bd,default_bound);});
# 536
goto _LL3;}else{_LL6: _LL7:
# 539
 goto _LL3;}_LL3:;}}}
# 542
{struct Cyc_List_List*cs=constraints;for(0;cs != 0;cs=cs->tl){
struct _tuple0*_stmttmp17=(struct _tuple0*)cs->hd;void*_tmpD6;void*_tmpD5;_LL9: _tmpD5=_stmttmp17->f1;_tmpD6=_stmttmp17->f2;_LLA: {void*eff=_tmpD5;void*bd=_tmpD6;
if(do_pin)
({Cyc_RgnOrder_pin_effect(eff,bd);});
# 544
if(!({Cyc_RgnOrder_effect_outlives(po,eff,bd);}))
# 547
return 0;}}}
# 549
return 1;}struct _tuple13{struct Cyc_Absyn_Tvar*f1;void*f2;};
# 553
void Cyc_RgnOrder_print_region_po(struct Cyc_RgnOrder_RgnPO*po){
struct Cyc_Iter_Iter iter=({Cyc_Dict_make_iter(Cyc_Core_heap_region,po->d);});
struct _tuple13 elem=*({({struct _tuple13*(*_tmp149)(struct _RegionHandle*,struct Cyc_Dict_Dict d)=({struct _tuple13*(*_tmpE5)(struct _RegionHandle*,struct Cyc_Dict_Dict d)=(struct _tuple13*(*)(struct _RegionHandle*,struct Cyc_Dict_Dict d))Cyc_Dict_rchoose;_tmpE5;});struct _RegionHandle*_tmp148=Cyc_Core_heap_region;_tmp149(_tmp148,po->d);});});
({void*_tmpD8=0U;({struct Cyc___cycFILE*_tmp14B=Cyc_stderr;struct _fat_ptr _tmp14A=({const char*_tmpD9="region po:\n";_tag_fat(_tmpD9,sizeof(char),12U);});Cyc_fprintf(_tmp14B,_tmp14A,_tag_fat(_tmpD8,sizeof(void*),0U));});});
while(({({int(*_tmp14C)(struct Cyc_Iter_Iter,struct _tuple13*)=({int(*_tmpDA)(struct Cyc_Iter_Iter,struct _tuple13*)=(int(*)(struct Cyc_Iter_Iter,struct _tuple13*))Cyc_Iter_next;_tmpDA;});_tmp14C(iter,& elem);});})){
({struct Cyc_String_pa_PrintArg_struct _tmpDD=({struct Cyc_String_pa_PrintArg_struct _tmpEF;_tmpEF.tag=0U,_tmpEF.f1=(struct _fat_ptr)((struct _fat_ptr)*(elem.f1)->name);_tmpEF;});struct Cyc_String_pa_PrintArg_struct _tmpDE=({struct Cyc_String_pa_PrintArg_struct _tmpEE;_tmpEE.tag=0U,({
struct _fat_ptr _tmp14D=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(elem.f2);}));_tmpEE.f1=_tmp14D;});_tmpEE;});void*_tmpDB[2U];_tmpDB[0]=& _tmpDD,_tmpDB[1]=& _tmpDE;({struct Cyc___cycFILE*_tmp14F=Cyc_stderr;struct _fat_ptr _tmp14E=({const char*_tmpDC="  %s outlived by %s\n";_tag_fat(_tmpDC,sizeof(char),21U);});Cyc_fprintf(_tmp14F,_tmp14E,_tag_fat(_tmpDB,sizeof(void*),2U));});});}
({struct Cyc_String_pa_PrintArg_struct _tmpE1=({struct Cyc_String_pa_PrintArg_struct _tmpF0;_tmpF0.tag=0U,({
struct _fat_ptr _tmp150=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(po->these_outlive_heap);}));_tmpF0.f1=_tmp150;});_tmpF0;});void*_tmpDF[1U];_tmpDF[0]=& _tmpE1;({struct Cyc___cycFILE*_tmp152=Cyc_stderr;struct _fat_ptr _tmp151=({const char*_tmpE0="  these outlive heap: %s\n";_tag_fat(_tmpE0,sizeof(char),26U);});Cyc_fprintf(_tmp152,_tmp151,_tag_fat(_tmpDF,sizeof(void*),1U));});});
({struct Cyc_String_pa_PrintArg_struct _tmpE4=({struct Cyc_String_pa_PrintArg_struct _tmpF1;_tmpF1.tag=0U,({
struct _fat_ptr _tmp153=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(po->these_outlive_unique);}));_tmpF1.f1=_tmp153;});_tmpF1;});void*_tmpE2[1U];_tmpE2[0]=& _tmpE4;({struct Cyc___cycFILE*_tmp155=Cyc_stderr;struct _fat_ptr _tmp154=({const char*_tmpE3="  these outlive unique: %s\n";_tag_fat(_tmpE3,sizeof(char),28U);});Cyc_fprintf(_tmp155,_tmp154,_tag_fat(_tmpE2,sizeof(void*),1U));});});}

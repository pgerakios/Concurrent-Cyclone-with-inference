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
 struct Cyc_Core_Opt{void*v;};struct _tuple0{void*f1;void*f2;};
# 108 "core.h"
void*Cyc_Core_fst(struct _tuple0*);extern char Cyc_Core_Invalid_argument[17U];struct Cyc_Core_Invalid_argument_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Failure[8U];struct Cyc_Core_Failure_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Impossible[11U];struct Cyc_Core_Impossible_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Not_found[10U];struct Cyc_Core_Not_found_exn_struct{char*tag;};extern char Cyc_Core_Unreachable[12U];struct Cyc_Core_Unreachable_exn_struct{char*tag;struct _fat_ptr f1;};
# 165
extern struct _RegionHandle*Cyc_Core_heap_region;
# 168
extern struct _RegionHandle*Cyc_Core_unique_region;struct Cyc_Core_DynamicRegion;struct Cyc_Core_NewDynamicRegion{struct Cyc_Core_DynamicRegion*key;};struct Cyc_Core_ThinRes{void*arr;unsigned nelts;};struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};
# 61 "list.h"
int Cyc_List_length(struct Cyc_List_List*x);
# 70
struct Cyc_List_List*Cyc_List_copy(struct Cyc_List_List*x);
# 76
struct Cyc_List_List*Cyc_List_map(void*(*f)(void*),struct Cyc_List_List*x);extern char Cyc_List_List_mismatch[14U];struct Cyc_List_List_mismatch_exn_struct{char*tag;};
# 135
void Cyc_List_iter_c(void(*f)(void*,void*),void*env,struct Cyc_List_List*x);
# 178
struct Cyc_List_List*Cyc_List_imp_rev(struct Cyc_List_List*x);
# 195
struct Cyc_List_List*Cyc_List_imp_append(struct Cyc_List_List*x,struct Cyc_List_List*y);extern char Cyc_List_Nth[4U];struct Cyc_List_Nth_exn_struct{char*tag;};
# 242
void*Cyc_List_nth(struct Cyc_List_List*x,int n);
# 322
int Cyc_List_mem(int(*compare)(void*,void*),struct Cyc_List_List*l,void*x);struct Cyc___cycFILE;struct Cyc_String_pa_PrintArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Int_pa_PrintArg_struct{int tag;unsigned long f1;};struct Cyc_Double_pa_PrintArg_struct{int tag;double f1;};struct Cyc_LongDouble_pa_PrintArg_struct{int tag;long double f1;};struct Cyc_ShortPtr_pa_PrintArg_struct{int tag;short*f1;};struct Cyc_IntPtr_pa_PrintArg_struct{int tag;unsigned long*f1;};struct Cyc_ShortPtr_sa_ScanfArg_struct{int tag;short*f1;};struct Cyc_UShortPtr_sa_ScanfArg_struct{int tag;unsigned short*f1;};struct Cyc_IntPtr_sa_ScanfArg_struct{int tag;int*f1;};struct Cyc_UIntPtr_sa_ScanfArg_struct{int tag;unsigned*f1;};struct Cyc_StringPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_DoublePtr_sa_ScanfArg_struct{int tag;double*f1;};struct Cyc_FloatPtr_sa_ScanfArg_struct{int tag;float*f1;};struct Cyc_CharPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};extern char Cyc_FileCloseError[15U];struct Cyc_FileCloseError_exn_struct{char*tag;};extern char Cyc_FileOpenError[14U];struct Cyc_FileOpenError_exn_struct{char*tag;struct _fat_ptr f1;};
# 49 "string.h"
int Cyc_strcmp(struct _fat_ptr s1,struct _fat_ptr s2);
int Cyc_strptrcmp(struct _fat_ptr*s1,struct _fat_ptr*s2);struct Cyc_PP_Ppstate;struct Cyc_PP_Out;struct Cyc_PP_Doc;struct Cyc_Position_Error;struct Cyc_Iter_Iter{void*env;int(*next)(void*env,void*dest);};struct Cyc_Dict_T;struct Cyc_Dict_Dict{int(*rel)(void*,void*);struct _RegionHandle*r;const struct Cyc_Dict_T*t;};extern char Cyc_Dict_Present[8U];struct Cyc_Dict_Present_exn_struct{char*tag;};extern char Cyc_Dict_Absent[7U];struct Cyc_Dict_Absent_exn_struct{char*tag;};struct Cyc_Relations_Reln;struct _union_Nmspace_Rel_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Abs_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_C_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Loc_n{int tag;int val;};union Cyc_Absyn_Nmspace{struct _union_Nmspace_Rel_n Rel_n;struct _union_Nmspace_Abs_n Abs_n;struct _union_Nmspace_C_n C_n;struct _union_Nmspace_Loc_n Loc_n;};struct _tuple1{union Cyc_Absyn_Nmspace f1;struct _fat_ptr*f2;};
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
enum Cyc_Absyn_KindQual{Cyc_Absyn_AnyKind =0U,Cyc_Absyn_MemKind =1U,Cyc_Absyn_BoxKind =2U,Cyc_Absyn_XRgnKind =3U,Cyc_Absyn_RgnKind =4U,Cyc_Absyn_EffKind =5U,Cyc_Absyn_IntKind =6U,Cyc_Absyn_BoolKind =7U,Cyc_Absyn_PtrBndKind =8U};struct Cyc_Absyn_Kind{enum Cyc_Absyn_KindQual kind;enum Cyc_Absyn_AliasQual aliasqual;};struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct{int tag;struct Cyc_Absyn_Kind*f1;};struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct{int tag;struct Cyc_Core_Opt*f1;};struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct{int tag;struct Cyc_Core_Opt*f1;struct Cyc_Absyn_Kind*f2;};struct Cyc_Absyn_Tvar{struct _fat_ptr*name;int identity;void*kind;};struct Cyc_Absyn_PtrLoc{unsigned ptr_loc;unsigned rgn_loc;unsigned zt_loc;};struct Cyc_Absyn_PtrAtts{void*rgn;void*nullable;void*bounds;void*zero_term;struct Cyc_Absyn_PtrLoc*ptrloc;};struct Cyc_Absyn_PtrInfo{void*elt_type;struct Cyc_Absyn_Tqual elt_tq;struct Cyc_Absyn_PtrAtts ptr_atts;};struct Cyc_Absyn_VarargInfo{struct _fat_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;int inject;};struct Cyc_Absyn_Star_Absyn_Cap_struct{int tag;};struct Cyc_Absyn_Bot_Absyn_Cap_struct{int tag;};struct Cyc_Absyn_Nat_Absyn_Cap_struct{int tag;int f1;int f2;};struct Cyc_Absyn_RgnEffect{void*name;struct Cyc_List_List*caps;void*parent;};struct Cyc_Absyn_FnInfo{struct Cyc_List_List*tvars;void*effect;struct Cyc_Absyn_Tqual ret_tqual;void*ret_type;struct Cyc_List_List*args;int c_varargs;struct Cyc_Absyn_VarargInfo*cyc_varargs;struct Cyc_List_List*rgn_po;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*requires_clause;struct Cyc_List_List*requires_relns;struct Cyc_Absyn_Exp*ensures_clause;struct Cyc_List_List*ensures_relns;struct Cyc_List_List*ieffect;struct Cyc_List_List*oeffect;struct Cyc_List_List*throws;int reentrant;};struct Cyc_Absyn_UnknownDatatypeInfo{struct _tuple1*name;int is_extensible;};struct _union_DatatypeInfo_UnknownDatatype{int tag;struct Cyc_Absyn_UnknownDatatypeInfo val;};struct _union_DatatypeInfo_KnownDatatype{int tag;struct Cyc_Absyn_Datatypedecl**val;};union Cyc_Absyn_DatatypeInfo{struct _union_DatatypeInfo_UnknownDatatype UnknownDatatype;struct _union_DatatypeInfo_KnownDatatype KnownDatatype;};
# 347
union Cyc_Absyn_DatatypeInfo Cyc_Absyn_KnownDatatype(struct Cyc_Absyn_Datatypedecl**);struct Cyc_Absyn_UnknownDatatypeFieldInfo{struct _tuple1*datatype_name;struct _tuple1*field_name;int is_extensible;};struct _union_DatatypeFieldInfo_UnknownDatatypefield{int tag;struct Cyc_Absyn_UnknownDatatypeFieldInfo val;};struct _tuple2{struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;};struct _union_DatatypeFieldInfo_KnownDatatypefield{int tag;struct _tuple2 val;};union Cyc_Absyn_DatatypeFieldInfo{struct _union_DatatypeFieldInfo_UnknownDatatypefield UnknownDatatypefield;struct _union_DatatypeFieldInfo_KnownDatatypefield KnownDatatypefield;};
# 360
union Cyc_Absyn_DatatypeFieldInfo Cyc_Absyn_KnownDatatypefield(struct Cyc_Absyn_Datatypedecl*,struct Cyc_Absyn_Datatypefield*);struct _tuple3{enum Cyc_Absyn_AggrKind f1;struct _tuple1*f2;struct Cyc_Core_Opt*f3;};struct _union_AggrInfo_UnknownAggr{int tag;struct _tuple3 val;};struct _union_AggrInfo_KnownAggr{int tag;struct Cyc_Absyn_Aggrdecl**val;};union Cyc_Absyn_AggrInfo{struct _union_AggrInfo_UnknownAggr UnknownAggr;struct _union_AggrInfo_KnownAggr KnownAggr;};
# 367
union Cyc_Absyn_AggrInfo Cyc_Absyn_KnownAggr(struct Cyc_Absyn_Aggrdecl**);struct Cyc_Absyn_ArrayInfo{void*elt_type;struct Cyc_Absyn_Tqual tq;struct Cyc_Absyn_Exp*num_elts;void*zero_term;unsigned zt_loc;};struct Cyc_Absyn_Aggr_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Enum_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;};struct Cyc_Absyn_Datatype_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;};struct Cyc_Absyn_TypeDecl{void*r;unsigned loc;};struct Cyc_Absyn_VoidCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_IntCon_Absyn_TyCon_struct{int tag;enum Cyc_Absyn_Sign f1;enum Cyc_Absyn_Size_of f2;};struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct{int tag;int f1;};struct Cyc_Absyn_RgnHandleCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_XRgnHandleCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_TagCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_HeapCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_UniqueCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_RefCntCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_AccessCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_CAccessCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_JoinCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_RgnsCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_TrueCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_FalseCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_ThinCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_FatCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct{int tag;struct _tuple1*f1;struct Cyc_Absyn_Enumdecl*f2;};struct Cyc_Absyn_AnonEnumCon_Absyn_TyCon_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_BuiltinCon_Absyn_TyCon_struct{int tag;struct _fat_ptr f1;struct Cyc_Absyn_Kind*f2;};struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_DatatypeInfo f1;};struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_DatatypeFieldInfo f1;};struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_AggrInfo f1;};struct Cyc_Absyn_AppType_Absyn_Type_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Evar_Absyn_Type_struct{int tag;struct Cyc_Core_Opt*f1;void*f2;int f3;struct Cyc_Core_Opt*f4;};struct Cyc_Absyn_VarType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Tvar*f1;};struct Cyc_Absyn_PointerType_Absyn_Type_struct{int tag;struct Cyc_Absyn_PtrInfo f1;};struct Cyc_Absyn_ArrayType_Absyn_Type_struct{int tag;struct Cyc_Absyn_ArrayInfo f1;};struct Cyc_Absyn_FnType_Absyn_Type_struct{int tag;struct Cyc_Absyn_FnInfo f1;};struct Cyc_Absyn_TupleType_Absyn_Type_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct{int tag;enum Cyc_Absyn_AggrKind f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_TypedefType_Absyn_Type_struct{int tag;struct _tuple1*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_Typedefdecl*f3;void*f4;};struct Cyc_Absyn_ValueofType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct{int tag;struct Cyc_Absyn_TypeDecl*f1;void**f2;};struct Cyc_Absyn_TypeofType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_NoTypes_Absyn_Funcparams_struct{int tag;struct Cyc_List_List*f1;unsigned f2;};struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct{int tag;struct Cyc_List_List*f1;int f2;struct Cyc_Absyn_VarargInfo*f3;void*f4;struct Cyc_List_List*f5;struct Cyc_Absyn_Exp*f6;struct Cyc_Absyn_Exp*f7;struct Cyc_List_List*f8;struct Cyc_List_List*f9;struct Cyc_List_List*f10;int f11;};
# 478 "absyn.h"
enum Cyc_Absyn_Format_Type{Cyc_Absyn_Printf_ft =0U,Cyc_Absyn_Scanf_ft =1U};struct Cyc_Absyn_Regparm_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Stdcall_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Cdecl_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Fastcall_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Noreturn_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Const_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Aligned_att_Absyn_Attribute_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Packed_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Section_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Nocommon_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Shared_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Unused_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Weak_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Dllimport_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Dllexport_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_instrument_function_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Constructor_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Destructor_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_check_memory_usage_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Format_att_Absyn_Attribute_struct{int tag;enum Cyc_Absyn_Format_Type f1;int f2;int f3;};struct Cyc_Absyn_Initializes_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Noliveunique_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Consume_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Pure_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Mode_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Alias_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Always_inline_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_throw_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Non_null_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Deprecated_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_VectorSize_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Carray_mod_Absyn_Type_modifier_struct{int tag;void*f1;unsigned f2;};struct Cyc_Absyn_ConstArray_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;unsigned f3;};struct Cyc_Absyn_Pointer_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_Absyn_PtrAtts f1;struct Cyc_Absyn_Tqual f2;};struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct{int tag;void*f1;};struct Cyc_Absyn_TypeParams_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_List_List*f1;unsigned f2;int f3;};struct Cyc_Absyn_Attributes_mod_Absyn_Type_modifier_struct{int tag;unsigned f1;struct Cyc_List_List*f2;};struct _union_Cnst_Null_c{int tag;int val;};struct _tuple4{enum Cyc_Absyn_Sign f1;char f2;};struct _union_Cnst_Char_c{int tag;struct _tuple4 val;};struct _union_Cnst_Wchar_c{int tag;struct _fat_ptr val;};struct _tuple5{enum Cyc_Absyn_Sign f1;short f2;};struct _union_Cnst_Short_c{int tag;struct _tuple5 val;};struct _tuple6{enum Cyc_Absyn_Sign f1;int f2;};struct _union_Cnst_Int_c{int tag;struct _tuple6 val;};struct _tuple7{enum Cyc_Absyn_Sign f1;long long f2;};struct _union_Cnst_LongLong_c{int tag;struct _tuple7 val;};struct _tuple8{struct _fat_ptr f1;int f2;};struct _union_Cnst_Float_c{int tag;struct _tuple8 val;};struct _union_Cnst_String_c{int tag;struct _fat_ptr val;};struct _union_Cnst_Wstring_c{int tag;struct _fat_ptr val;};union Cyc_Absyn_Cnst{struct _union_Cnst_Null_c Null_c;struct _union_Cnst_Char_c Char_c;struct _union_Cnst_Wchar_c Wchar_c;struct _union_Cnst_Short_c Short_c;struct _union_Cnst_Int_c Int_c;struct _union_Cnst_LongLong_c LongLong_c;struct _union_Cnst_Float_c Float_c;struct _union_Cnst_String_c String_c;struct _union_Cnst_Wstring_c Wstring_c;};
# 574
enum Cyc_Absyn_Primop{Cyc_Absyn_Plus =0U,Cyc_Absyn_Times =1U,Cyc_Absyn_Minus =2U,Cyc_Absyn_Div =3U,Cyc_Absyn_Mod =4U,Cyc_Absyn_Eq =5U,Cyc_Absyn_Neq =6U,Cyc_Absyn_Gt =7U,Cyc_Absyn_Lt =8U,Cyc_Absyn_Gte =9U,Cyc_Absyn_Lte =10U,Cyc_Absyn_Not =11U,Cyc_Absyn_Bitnot =12U,Cyc_Absyn_Bitand =13U,Cyc_Absyn_Bitor =14U,Cyc_Absyn_Bitxor =15U,Cyc_Absyn_Bitlshift =16U,Cyc_Absyn_Bitlrshift =17U,Cyc_Absyn_Numelts =18U};
# 581
enum Cyc_Absyn_Incrementor{Cyc_Absyn_PreInc =0U,Cyc_Absyn_PostInc =1U,Cyc_Absyn_PreDec =2U,Cyc_Absyn_PostDec =3U};struct Cyc_Absyn_VarargCallInfo{int num_varargs;struct Cyc_List_List*injectors;struct Cyc_Absyn_VarargInfo*vai;};struct Cyc_Absyn_StructField_Absyn_OffsetofField_struct{int tag;struct _fat_ptr*f1;};struct Cyc_Absyn_TupleIndex_Absyn_OffsetofField_struct{int tag;unsigned f1;};
# 599
enum Cyc_Absyn_Coercion{Cyc_Absyn_Unknown_coercion =0U,Cyc_Absyn_No_coercion =1U,Cyc_Absyn_Null_to_NonNull =2U,Cyc_Absyn_Other_coercion =3U};struct Cyc_Absyn_MallocInfo{int is_calloc;struct Cyc_Absyn_Exp*rgn;void**elt_type;struct Cyc_Absyn_Exp*num_elts;int fat_result;int inline_call;};struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct{int tag;union Cyc_Absyn_Cnst f1;};struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Pragma_e_Absyn_Raw_exp_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct{int tag;enum Cyc_Absyn_Primop f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;enum Cyc_Absyn_Incrementor f2;};struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct _tuple9{struct Cyc_List_List*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_VarargCallInfo*f3;int f4;struct _tuple9*f5;};struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;int f2;};struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Exp*f2;int f3;enum Cyc_Absyn_Coercion f4;};struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;int f3;int f4;};struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;int f3;int f4;};struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Tuple_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;};struct _tuple10{struct _fat_ptr*f1;struct Cyc_Absyn_Tqual f2;void*f3;};struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct{int tag;struct _tuple10*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;int f4;};struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;int f3;};struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct{int tag;struct _tuple1*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct Cyc_Absyn_Aggrdecl*f4;};struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Datatypedecl*f2;struct Cyc_Absyn_Datatypefield*f3;};struct Cyc_Absyn_Enum_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_Spawn_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;struct _tuple9*f3;};struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_MallocInfo f1;};struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Core_Opt*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Stmt*f1;};struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;};struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct{int tag;int f1;struct _fat_ptr f2;struct Cyc_List_List*f3;struct Cyc_List_List*f4;struct Cyc_List_List*f5;};struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Exp{void*topt;void*r;unsigned loc;void*annot;};struct Cyc_Absyn_Skip_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;struct Cyc_Absyn_Stmt*f3;};struct _tuple11{struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct{int tag;struct _tuple11 f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Break_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Continue_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Goto_s_Absyn_Raw_stmt_struct{int tag;struct _fat_ptr*f1;};struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _tuple11 f2;struct _tuple11 f3;struct Cyc_Absyn_Stmt*f4;};struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;void*f3;};struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Switch_clause**f2;};struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Decl*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct{int tag;struct _fat_ptr*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct _tuple11 f2;};struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_List_List*f2;void*f3;};struct Cyc_Absyn_Stmt{void*r;unsigned loc;void*annot;};struct Cyc_Absyn_Wild_p_Absyn_Raw_pat_struct{int tag;};struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_AliasVar_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_TagInt_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Tuple_p_Absyn_Raw_pat_struct{int tag;struct Cyc_List_List*f1;int f2;};struct Cyc_Absyn_Pointer_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Pat*f1;};struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct{int tag;union Cyc_Absyn_AggrInfo*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;int f4;};struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;struct Cyc_List_List*f3;int f4;};struct Cyc_Absyn_Null_p_Absyn_Raw_pat_struct{int tag;};struct Cyc_Absyn_Int_p_Absyn_Raw_pat_struct{int tag;enum Cyc_Absyn_Sign f1;int f2;};struct Cyc_Absyn_Char_p_Absyn_Raw_pat_struct{int tag;char f1;};struct Cyc_Absyn_Float_p_Absyn_Raw_pat_struct{int tag;struct _fat_ptr f1;int f2;};struct Cyc_Absyn_Enum_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_AnonEnum_p_Absyn_Raw_pat_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_UnknownId_p_Absyn_Raw_pat_struct{int tag;struct _tuple1*f1;};struct Cyc_Absyn_UnknownCall_p_Absyn_Raw_pat_struct{int tag;struct _tuple1*f1;struct Cyc_List_List*f2;int f3;};struct Cyc_Absyn_Exp_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Pat{void*r;void*topt;unsigned loc;};struct Cyc_Absyn_Switch_clause{struct Cyc_Absyn_Pat*pattern;struct Cyc_Core_Opt*pat_vars;struct Cyc_Absyn_Exp*where_clause;struct Cyc_Absyn_Stmt*body;unsigned loc;};struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct{int tag;struct _tuple1*f1;};struct Cyc_Absyn_Global_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Funname_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Param_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Local_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Pat_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Vardecl{enum Cyc_Absyn_Scope sc;struct _tuple1*name;unsigned varloc;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*initializer;void*rgn;struct Cyc_List_List*attributes;int escapes;};struct Cyc_Absyn_Fndecl{enum Cyc_Absyn_Scope sc;int is_inline;struct _tuple1*name;struct Cyc_Absyn_Stmt*body;struct Cyc_Absyn_FnInfo i;void*cached_type;struct Cyc_Core_Opt*param_vardecls;struct Cyc_Absyn_Vardecl*fn_vardecl;};struct Cyc_Absyn_Aggrfield{struct _fat_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*width;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*requires_clause;};struct Cyc_Absyn_AggrdeclImpl{struct Cyc_List_List*exist_vars;struct Cyc_List_List*rgn_po;struct Cyc_List_List*fields;int tagged;};struct Cyc_Absyn_Aggrdecl{enum Cyc_Absyn_AggrKind kind;enum Cyc_Absyn_Scope sc;struct _tuple1*name;struct Cyc_List_List*tvs;struct Cyc_Absyn_AggrdeclImpl*impl;struct Cyc_List_List*attributes;int expected_mem_kind;};struct Cyc_Absyn_Datatypefield{struct _tuple1*name;struct Cyc_List_List*typs;unsigned loc;enum Cyc_Absyn_Scope sc;};struct Cyc_Absyn_Datatypedecl{enum Cyc_Absyn_Scope sc;struct _tuple1*name;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*fields;int is_extensible;};struct Cyc_Absyn_Enumfield{struct _tuple1*name;struct Cyc_Absyn_Exp*tag;unsigned loc;};struct Cyc_Absyn_Enumdecl{enum Cyc_Absyn_Scope sc;struct _tuple1*name;struct Cyc_Core_Opt*fields;};struct Cyc_Absyn_Typedefdecl{struct _tuple1*name;struct Cyc_Absyn_Tqual tq;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*kind;void*defn;struct Cyc_List_List*atts;int extern_c;};struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Pat*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;void*f4;};struct Cyc_Absyn_Letv_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;};struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;};struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Typedefdecl*f1;};struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct{int tag;struct _fat_ptr*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct{int tag;struct _tuple1*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_ExternC_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;};struct _tuple12{unsigned f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct _tuple12*f4;};struct Cyc_Absyn_Porton_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Portoff_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Tempeston_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Tempestoff_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Decl{void*r;unsigned loc;};struct Cyc_Absyn_ArrayElement_Absyn_Designator_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_FieldName_Absyn_Designator_struct{int tag;struct _fat_ptr*f1;};extern char Cyc_Absyn_EmptyAnnot[11U];struct Cyc_Absyn_EmptyAnnot_Absyn_AbsynAnnot_struct{char*tag;};
# 954 "absyn.h"
int Cyc_Absyn_qvar_cmp(struct _tuple1*,struct _tuple1*);
# 956
int Cyc_Absyn_tvar_cmp(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*);
# 968
void*Cyc_Absyn_compress_kb(void*);
# 974
int Cyc_Absyn_type2bool(int def,void*);
# 979
void*Cyc_Absyn_new_evar(struct Cyc_Core_Opt*k,struct Cyc_Core_Opt*tenv);
# 991
extern void*Cyc_Absyn_heap_rgn_type;extern void*Cyc_Absyn_unique_rgn_type;
# 993
extern void*Cyc_Absyn_empty_effect;
# 995
extern void*Cyc_Absyn_true_type;extern void*Cyc_Absyn_false_type;
# 997
void*Cyc_Absyn_var_type(struct Cyc_Absyn_Tvar*);void*Cyc_Absyn_access_eff(void*);void*Cyc_Absyn_join_eff(struct Cyc_List_List*);void*Cyc_Absyn_regionsof_eff(void*);
# 1016
struct _tuple1*Cyc_Absyn_datatype_print_arg_qvar();
struct _tuple1*Cyc_Absyn_datatype_scanf_arg_qvar();
# 1026
void*Cyc_Absyn_bounds_one();
# 1068
struct Cyc_Absyn_Exp*Cyc_Absyn_uint_exp(unsigned,unsigned);
# 1213
struct _fat_ptr Cyc_Absyn_attribute2string(void*);
# 1215
int Cyc_Absyn_fntype_att(void*);
# 1288
int Cyc_Absyn_is_xrgn(void*r);
# 1331
void*Cyc_Absyn_rgneffect2type(struct Cyc_Absyn_RgnEffect*re);
# 1338
struct Cyc_Absyn_RgnEffect*Cyc_Absyn_find_rgneffect(struct Cyc_Absyn_Tvar*x,struct Cyc_List_List*l);
# 1363
void*Cyc_Absyn_i_check_valid_effect(unsigned loc,void*(*cb)(unsigned,void*,void*,struct Cyc_Absyn_Kind*,void*,int,int),void*te,void*new_cvtenv,struct Cyc_List_List*ieff,struct Cyc_List_List*oeff);struct Cyc_Absynpp_Params{int expand_typedefs;int qvar_to_Cids;int add_cyc_prefix;int to_VC;int decls_first;int rewrite_temp_tvars;int print_all_tvars;int print_all_kinds;int print_all_effects;int print_using_stmts;int print_externC_stmts;int print_full_evars;int print_zeroterm;int generate_line_directives;int use_curr_namespace;struct Cyc_List_List*curr_namespace;};
# 62 "absynpp.h"
struct _fat_ptr Cyc_Absynpp_typ2string(void*);
# 64
struct _fat_ptr Cyc_Absynpp_kind2string(struct Cyc_Absyn_Kind*);
struct _fat_ptr Cyc_Absynpp_kindbound2string(void*);
# 67
struct _fat_ptr Cyc_Absynpp_exp2string(struct Cyc_Absyn_Exp*);
# 69
struct _fat_ptr Cyc_Absynpp_qvar2string(struct _tuple1*);
# 74
struct _fat_ptr Cyc_Absynpp_tvar2string(struct Cyc_Absyn_Tvar*);
# 29 "unify.h"
int Cyc_Unify_unify(void*,void*);struct _union_RelnOp_RConst{int tag;unsigned val;};struct _union_RelnOp_RVar{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RNumelts{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RType{int tag;void*val;};struct _union_RelnOp_RParam{int tag;unsigned val;};struct _union_RelnOp_RParamNumelts{int tag;unsigned val;};struct _union_RelnOp_RReturn{int tag;unsigned val;};union Cyc_Relations_RelnOp{struct _union_RelnOp_RConst RConst;struct _union_RelnOp_RVar RVar;struct _union_RelnOp_RNumelts RNumelts;struct _union_RelnOp_RType RType;struct _union_RelnOp_RParam RParam;struct _union_RelnOp_RParamNumelts RParamNumelts;struct _union_RelnOp_RReturn RReturn;};
# 50 "relations-ap.h"
enum Cyc_Relations_Relation{Cyc_Relations_Req =0U,Cyc_Relations_Rneq =1U,Cyc_Relations_Rlte =2U,Cyc_Relations_Rlt =3U};struct Cyc_Relations_Reln{union Cyc_Relations_RelnOp rop1;enum Cyc_Relations_Relation relation;union Cyc_Relations_RelnOp rop2;};
# 84
struct Cyc_List_List*Cyc_Relations_exp2relns(struct _RegionHandle*r,struct Cyc_Absyn_Exp*e);
# 129
int Cyc_Relations_consistent_relations(struct Cyc_List_List*rlns);struct Cyc_RgnOrder_RgnPO;extern char Cyc_Tcenv_Env_error[10U];struct Cyc_Tcenv_Env_error_exn_struct{char*tag;};struct Cyc_Tcenv_Genv{struct Cyc_Dict_Dict aggrdecls;struct Cyc_Dict_Dict datatypedecls;struct Cyc_Dict_Dict enumdecls;struct Cyc_Dict_Dict typedefs;struct Cyc_Dict_Dict ordinaries;};struct Cyc_Tcenv_Fenv;struct Cyc_Tcenv_Tenv{struct Cyc_List_List*ns;struct Cyc_Tcenv_Genv*ae;struct Cyc_Tcenv_Fenv*le;int allow_valueof: 1;int in_extern_c_include: 1;int in_tempest: 1;int tempest_generalize: 1;};
# 73 "tcenv.h"
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_new_bogus_fenv_in_env(struct Cyc_Tcenv_Tenv*te,void*ret_type,struct Cyc_List_List*args);
# 78
struct Cyc_Absyn_Aggrdecl**Cyc_Tcenv_lookup_aggrdecl(struct Cyc_Tcenv_Tenv*,unsigned,struct _tuple1*);
struct Cyc_Absyn_Datatypedecl**Cyc_Tcenv_lookup_datatypedecl(struct Cyc_Tcenv_Tenv*,unsigned,struct _tuple1*);
# 81
struct Cyc_Absyn_Enumdecl**Cyc_Tcenv_lookup_enumdecl(struct Cyc_Tcenv_Tenv*,unsigned,struct _tuple1*);
struct Cyc_Absyn_Typedefdecl*Cyc_Tcenv_lookup_typedefdecl(struct Cyc_Tcenv_Tenv*,unsigned,struct _tuple1*);
# 84
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_allow_valueof(struct Cyc_Tcenv_Tenv*);
# 90
enum Cyc_Tcenv_NewStatus{Cyc_Tcenv_NoneNew =0U,Cyc_Tcenv_InNew =1U,Cyc_Tcenv_InNewAggr =2U};
# 100
struct Cyc_List_List*Cyc_Tcenv_lookup_type_vars(struct Cyc_Tcenv_Tenv*);
# 30 "tcutil.h"
void*Cyc_Tcutil_impos(struct _fat_ptr,struct _fat_ptr);
void Cyc_Tcutil_terr(unsigned,struct _fat_ptr,struct _fat_ptr);
void Cyc_Tcutil_warn(unsigned,struct _fat_ptr,struct _fat_ptr);
# 37
int Cyc_Tcutil_is_pointer_effect(void*t);
# 40
int Cyc_Tcutil_is_char_type(void*);
# 46
int Cyc_Tcutil_is_function_type(void*);
# 48
int Cyc_Tcutil_is_array_type(void*);
# 67
void*Cyc_Tcutil_pointer_elt_type(void*);
# 75
struct Cyc_Absyn_Exp*Cyc_Tcutil_get_bounds_exp(void*def,void*b);
# 87
struct Cyc_Absyn_Exp*Cyc_Tcutil_get_bounds_exp(void*def,void*b);
# 90
int Cyc_Tcutil_is_integral(struct Cyc_Absyn_Exp*);
# 97
void*Cyc_Tcutil_copy_type(void*);
# 103
int Cyc_Tcutil_kind_leq(struct Cyc_Absyn_Kind*,struct Cyc_Absyn_Kind*);
# 108
struct Cyc_Absyn_Kind*Cyc_Tcutil_tvar_kind(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Kind*def);
struct Cyc_Absyn_Kind*Cyc_Tcutil_type_kind(void*);
void*Cyc_Tcutil_compress(void*);
# 112
int Cyc_Tcutil_coerce_uint_type(struct Cyc_Absyn_Exp*);
# 138
extern struct Cyc_Absyn_Kind Cyc_Tcutil_xrk;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_rk;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_ak;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_bk;
# 143
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
# 155
extern struct Cyc_Absyn_Kind Cyc_Tcutil_ubk;
# 177
struct Cyc_Core_Opt*Cyc_Tcutil_kind_to_opt(struct Cyc_Absyn_Kind*k);
# 189
void*Cyc_Tcutil_substitute(struct Cyc_List_List*,void*);
# 206
void*Cyc_Tcutil_fndecl2type(struct Cyc_Absyn_Fndecl*);
# 216
void Cyc_Tcutil_check_bitfield(unsigned,void*field_typ,struct Cyc_Absyn_Exp*width,struct _fat_ptr*fn);
# 218
void Cyc_Tcutil_check_unique_vars(struct Cyc_List_List*,unsigned,struct _fat_ptr err_msg);
void Cyc_Tcutil_check_unique_tvars(unsigned,struct Cyc_List_List*);
# 261
struct Cyc_Absyn_Tvar*Cyc_Tcutil_new_tvar(void*);
# 263
int Cyc_Tcutil_new_tvar_id();
# 278
int Cyc_Tcutil_is_const_exp(struct Cyc_Absyn_Exp*);
# 285
int Cyc_Tcutil_extract_const_from_typedef(unsigned,int declared_const,void*);
# 292
void Cyc_Tcutil_check_no_qual(unsigned,void*);
# 303
void*Cyc_Tcutil_promote_array(void*t,void*rgn,int convert_tag);
# 310
int Cyc_Tcutil_force_type2bool(int desired,void*);
# 318
int Cyc_Tcutil_admits_zero(void*);
void Cyc_Tcutil_replace_rops(struct Cyc_List_List*,struct Cyc_Relations_Reln*);
# 321
struct Cyc_Core_Opt*Cyc_Tcutil_kind_to_bound_opt(struct Cyc_Absyn_Kind*k);
int Cyc_Tcutil_fast_tvar_cmp(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*);struct _tuple13{unsigned f1;int f2;};
# 28 "evexp.h"
struct _tuple13 Cyc_Evexp_eval_const_uint_exp(struct Cyc_Absyn_Exp*e);
# 28 "tcexp.h"
void*Cyc_Tcexp_tcExp(struct Cyc_Tcenv_Tenv*,void**,struct Cyc_Absyn_Exp*);
# 40 "tc.h"
void Cyc_Tc_tcAggrdecl(struct Cyc_Tcenv_Tenv*,unsigned,struct Cyc_Absyn_Aggrdecl*);
void Cyc_Tc_tcDatatypedecl(struct Cyc_Tcenv_Tenv*,unsigned,struct Cyc_Absyn_Datatypedecl*);
void Cyc_Tc_tcEnumdecl(struct Cyc_Tcenv_Tenv*,unsigned,struct Cyc_Absyn_Enumdecl*);
# 24 "cyclone.h"
extern int Cyc_Cyclone_tovc_r;
# 26
enum Cyc_Cyclone_C_Compilers{Cyc_Cyclone_Gcc_c =0U,Cyc_Cyclone_Vc_c =1U};
# 34 "tctyp.h"
void Cyc_Tctyp_check_valid_toplevel_type(unsigned,struct Cyc_Tcenv_Tenv*,void*);struct Cyc_Tctyp_CVTEnv{struct _RegionHandle*r;struct Cyc_List_List*kind_env;int fn_result;int generalize_evars;struct Cyc_List_List*free_vars;struct Cyc_List_List*free_evars;};
# 57 "tctyp.cyc"
static struct _fat_ptr*Cyc_Tctyp_fst_fdarg(struct _tuple10*t){return(struct _fat_ptr*)_check_null((*t).f1);}struct _tuple14{void*f1;int f2;};
# 63
static struct Cyc_List_List*Cyc_Tctyp_add_free_evar(struct _RegionHandle*r,struct Cyc_List_List*es,void*e,int b){
# 65
void*_stmttmp0=({Cyc_Tcutil_compress(e);});void*_tmp1=_stmttmp0;int _tmp2;if(((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp1)->tag == 1U){_LL1: _tmp2=((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp1)->f3;_LL2: {int i=_tmp2;
# 67
{struct Cyc_List_List*es2=es;for(0;es2 != 0;es2=es2->tl){
struct _tuple14*_stmttmp1=(struct _tuple14*)es2->hd;int*_tmp4;void*_tmp3;_LL6: _tmp3=_stmttmp1->f1;_tmp4=(int*)& _stmttmp1->f2;_LL7: {void*t=_tmp3;int*b2=_tmp4;
void*_stmttmp2=({Cyc_Tcutil_compress(t);});void*_tmp5=_stmttmp2;int _tmp6;if(((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp5)->tag == 1U){_LL9: _tmp6=((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp5)->f3;_LLA: {int j=_tmp6;
# 71
if(i == j){
if(b != *b2)*b2=1;return es;}
# 71
goto _LL8;}}else{_LLB: _LLC:
# 76
 goto _LL8;}_LL8:;}}}
# 79
return({struct Cyc_List_List*_tmp8=_region_malloc(r,sizeof(*_tmp8));({struct _tuple14*_tmp2B1=({struct _tuple14*_tmp7=_region_malloc(r,sizeof(*_tmp7));_tmp7->f1=e,_tmp7->f2=b;_tmp7;});_tmp8->hd=_tmp2B1;}),_tmp8->tl=es;_tmp8;});}}else{_LL3: _LL4:
 return es;}_LL0:;}
# 85
static struct Cyc_List_List*Cyc_Tctyp_add_bound_tvar(struct Cyc_List_List*tvs,struct Cyc_Absyn_Tvar*tv){
if(tv->identity == - 1)
({struct Cyc_String_pa_PrintArg_struct _tmpD=({struct Cyc_String_pa_PrintArg_struct _tmp277;_tmp277.tag=0U,({struct _fat_ptr _tmp2B2=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_tvar2string(tv);}));_tmp277.f1=_tmp2B2;});_tmp277;});void*_tmpA[1U];_tmpA[0]=& _tmpD;({int(*_tmp2B4)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmpC)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmpC;});struct _fat_ptr _tmp2B3=({const char*_tmpB="bound tvar id for %s is NULL";_tag_fat(_tmpB,sizeof(char),29U);});_tmp2B4(_tmp2B3,_tag_fat(_tmpA,sizeof(void*),1U));});});
# 86
return({struct Cyc_List_List*_tmpE=_cycalloc(sizeof(*_tmpE));
# 88
_tmpE->hd=tv,_tmpE->tl=tvs;_tmpE;});}
# 90
static struct Cyc_List_List*Cyc_Tctyp_remove_bound_tvars(struct _RegionHandle*rgn,struct Cyc_List_List*tvs,struct Cyc_List_List*btvs){
# 93
struct Cyc_List_List*r=0;
for(0;tvs != 0;tvs=tvs->tl){
int present=0;
{struct Cyc_List_List*b=btvs;for(0;b != 0;b=b->tl){
if(((struct Cyc_Absyn_Tvar*)tvs->hd)->identity == ((struct Cyc_Absyn_Tvar*)b->hd)->identity){
present=1;
break;}}}
# 96
if(!present)
# 102
r=({struct Cyc_List_List*_tmp10=_region_malloc(rgn,sizeof(*_tmp10));_tmp10->hd=(struct Cyc_Absyn_Tvar*)tvs->hd,_tmp10->tl=r;_tmp10;});}
# 104
r=({Cyc_List_imp_rev(r);});
return r;}struct _tuple15{struct Cyc_Absyn_Tvar*f1;int f2;};
# 107
static struct Cyc_List_List*Cyc_Tctyp_remove_bound_tvars_bool(struct _RegionHandle*r,struct Cyc_List_List*tvs,struct Cyc_List_List*btvs){
# 110
struct Cyc_List_List*res=0;
for(0;tvs != 0;tvs=tvs->tl){
struct _tuple15 _stmttmp3=*((struct _tuple15*)tvs->hd);int _tmp13;struct Cyc_Absyn_Tvar*_tmp12;_LL1: _tmp12=_stmttmp3.f1;_tmp13=_stmttmp3.f2;_LL2: {struct Cyc_Absyn_Tvar*tv=_tmp12;int b=_tmp13;
int present=0;
{struct Cyc_List_List*b=btvs;for(0;b != 0;b=b->tl){
if(tv->identity == ((struct Cyc_Absyn_Tvar*)b->hd)->identity){
present=1;
break;}}}
# 114
if(!present)
# 120
res=({struct Cyc_List_List*_tmp14=_region_malloc(r,sizeof(*_tmp14));_tmp14->hd=(struct _tuple15*)tvs->hd,_tmp14->tl=res;_tmp14;});}}
# 122
res=({Cyc_List_imp_rev(res);});
return res;}
# 130
static void Cyc_Tctyp_check_free_evars(struct Cyc_List_List*free_evars,void*in_typ,unsigned loc){
# 132
for(0;free_evars != 0;free_evars=free_evars->tl){
void*e=(void*)free_evars->hd;
{void*_stmttmp4=({Cyc_Tcutil_compress(e);});void*_tmp16=_stmttmp4;if(((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp16)->tag == 1U){_LL1: _LL2:
 goto _LL0;}else{_LL3: _LL4:
 continue;}_LL0:;}{
# 138
struct Cyc_Absyn_Kind*_stmttmp5=({Cyc_Tcutil_type_kind(e);});struct Cyc_Absyn_Kind*_tmp17=_stmttmp5;switch(((struct Cyc_Absyn_Kind*)_tmp17)->kind){case Cyc_Absyn_RgnKind: switch(((struct Cyc_Absyn_Kind*)_tmp17)->aliasqual){case Cyc_Absyn_Unique: _LL6: _LL7:
# 140
 if(!({Cyc_Unify_unify(e,Cyc_Absyn_unique_rgn_type);}))
({void*_tmp18=0U;({int(*_tmp2B6)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp1A)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp1A;});struct _fat_ptr _tmp2B5=({const char*_tmp19="can't unify evar with unique region!";_tag_fat(_tmp19,sizeof(char),37U);});_tmp2B6(_tmp2B5,_tag_fat(_tmp18,sizeof(void*),0U));});});
# 140
goto _LL5;case Cyc_Absyn_Aliasable: _LL8: _LL9:
# 143
 goto _LLB;case Cyc_Absyn_Top: _LLA: _LLB:
# 145
 if(!({Cyc_Unify_unify(e,Cyc_Absyn_heap_rgn_type);}))({void*_tmp1B=0U;({int(*_tmp2B8)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp1D)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp1D;});struct _fat_ptr _tmp2B7=({const char*_tmp1C="can't unify evar with heap!";_tag_fat(_tmp1C,sizeof(char),28U);});_tmp2B8(_tmp2B7,_tag_fat(_tmp1B,sizeof(void*),0U));});});goto _LL5;default: goto _LL12;}case Cyc_Absyn_EffKind: _LLC: _LLD:
# 148
 if(!({Cyc_Unify_unify(e,Cyc_Absyn_empty_effect);}))({void*_tmp1E=0U;({int(*_tmp2BA)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp20)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp20;});struct _fat_ptr _tmp2B9=({const char*_tmp1F="can't unify evar with {}!";_tag_fat(_tmp1F,sizeof(char),26U);});_tmp2BA(_tmp2B9,_tag_fat(_tmp1E,sizeof(void*),0U));});});goto _LL5;case Cyc_Absyn_BoolKind: _LLE: _LLF:
# 151
 if(!({Cyc_Unify_unify(e,Cyc_Absyn_false_type);}))({struct Cyc_String_pa_PrintArg_struct _tmp24=({struct Cyc_String_pa_PrintArg_struct _tmp278;_tmp278.tag=0U,({
struct _fat_ptr _tmp2BB=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(e);}));_tmp278.f1=_tmp2BB;});_tmp278;});void*_tmp21[1U];_tmp21[0]=& _tmp24;({int(*_tmp2BD)(struct _fat_ptr,struct _fat_ptr)=({
# 151
int(*_tmp23)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp23;});struct _fat_ptr _tmp2BC=({const char*_tmp22="can't unify evar %s with @false!";_tag_fat(_tmp22,sizeof(char),33U);});_tmp2BD(_tmp2BC,_tag_fat(_tmp21,sizeof(void*),1U));});});goto _LL5;case Cyc_Absyn_PtrBndKind: _LL10: _LL11:
# 155
 if(!({int(*_tmp25)(void*,void*)=Cyc_Unify_unify;void*_tmp26=e;void*_tmp27=({Cyc_Absyn_bounds_one();});_tmp25(_tmp26,_tmp27);}))
({void*_tmp28=0U;({int(*_tmp2BF)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp2A)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp2A;});struct _fat_ptr _tmp2BE=({const char*_tmp29="can't unify evar with bounds_one!";_tag_fat(_tmp29,sizeof(char),34U);});_tmp2BF(_tmp2BE,_tag_fat(_tmp28,sizeof(void*),0U));});});
# 155
goto _LL5;default: _LL12: _LL13:
# 159
({struct Cyc_String_pa_PrintArg_struct _tmp2D=({struct Cyc_String_pa_PrintArg_struct _tmp27A;_tmp27A.tag=0U,({
struct _fat_ptr _tmp2C0=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(e);}));_tmp27A.f1=_tmp2C0;});_tmp27A;});struct Cyc_String_pa_PrintArg_struct _tmp2E=({struct Cyc_String_pa_PrintArg_struct _tmp279;_tmp279.tag=0U,({struct _fat_ptr _tmp2C1=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(in_typ);}));_tmp279.f1=_tmp2C1;});_tmp279;});void*_tmp2B[2U];_tmp2B[0]=& _tmp2D,_tmp2B[1]=& _tmp2E;({unsigned _tmp2C3=loc;struct _fat_ptr _tmp2C2=({const char*_tmp2C="hidden type variable %s isn't abstracted in type %s";_tag_fat(_tmp2C,sizeof(char),52U);});Cyc_Tcutil_terr(_tmp2C3,_tmp2C2,_tag_fat(_tmp2B,sizeof(void*),2U));});});
goto _LL5;}_LL5:;}}}
# 170
static int Cyc_Tctyp_constrain_kinds(void*c1,void*c2){
c1=({Cyc_Absyn_compress_kb(c1);});
c2=({Cyc_Absyn_compress_kb(c2);});
if(c1 == c2)return 1;{struct _tuple0 _stmttmp6=({struct _tuple0 _tmp27B;_tmp27B.f1=c1,_tmp27B.f2=c2;_tmp27B;});struct _tuple0 _tmp30=_stmttmp6;struct Cyc_Absyn_Kind*_tmp34;struct Cyc_Core_Opt**_tmp33;struct Cyc_Absyn_Kind*_tmp32;struct Cyc_Core_Opt**_tmp31;struct Cyc_Absyn_Kind*_tmp37;struct Cyc_Absyn_Kind*_tmp36;struct Cyc_Core_Opt**_tmp35;struct Cyc_Core_Opt**_tmp38;struct Cyc_Core_Opt**_tmp39;struct Cyc_Absyn_Kind*_tmp3C;struct Cyc_Core_Opt**_tmp3B;struct Cyc_Absyn_Kind*_tmp3A;struct Cyc_Absyn_Kind*_tmp3E;struct Cyc_Absyn_Kind*_tmp3D;if(((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp30.f1)->tag == 0U)switch(*((int*)_tmp30.f2)){case 0U: _LL1: _tmp3D=((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp30.f1)->f1;_tmp3E=((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp30.f2)->f1;_LL2: {struct Cyc_Absyn_Kind*k1=_tmp3D;struct Cyc_Absyn_Kind*k2=_tmp3E;
# 175
return k1 == k2;}case 1U: goto _LL3;default: _LL9: _tmp3A=((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp30.f1)->f1;_tmp3B=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp30.f2)->f1;_tmp3C=((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp30.f2)->f2;_LLA: {struct Cyc_Absyn_Kind*k1=_tmp3A;struct Cyc_Core_Opt**f=_tmp3B;struct Cyc_Absyn_Kind*k2=_tmp3C;
# 183
if(!({Cyc_Tcutil_kind_leq(k1,k2);}))return 0;({struct Cyc_Core_Opt*_tmp2C4=({struct Cyc_Core_Opt*_tmp42=_cycalloc(sizeof(*_tmp42));_tmp42->v=c1;_tmp42;});*f=_tmp2C4;});
# 185
return 1;}}else{if(((struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*)_tmp30.f2)->tag == 1U){_LL3: _tmp39=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*)_tmp30.f2)->f1;_LL4: {struct Cyc_Core_Opt**f=_tmp39;
# 176
({struct Cyc_Core_Opt*_tmp2C5=({struct Cyc_Core_Opt*_tmp3F=_cycalloc(sizeof(*_tmp3F));_tmp3F->v=c1;_tmp3F;});*f=_tmp2C5;});return 1;}}else{if(((struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*)_tmp30.f1)->tag == 1U){_LL5: _tmp38=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*)_tmp30.f1)->f1;_LL6: {struct Cyc_Core_Opt**f=_tmp38;
({struct Cyc_Core_Opt*_tmp2C6=({struct Cyc_Core_Opt*_tmp40=_cycalloc(sizeof(*_tmp40));_tmp40->v=c2;_tmp40;});*f=_tmp2C6;});return 1;}}else{if(((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp30.f2)->tag == 0U){_LL7: _tmp35=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp30.f1)->f1;_tmp36=((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp30.f1)->f2;_tmp37=((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp30.f2)->f1;_LL8: {struct Cyc_Core_Opt**f=_tmp35;struct Cyc_Absyn_Kind*k1=_tmp36;struct Cyc_Absyn_Kind*k2=_tmp37;
# 179
if(!({Cyc_Tcutil_kind_leq(k2,k1);}))return 0;({struct Cyc_Core_Opt*_tmp2C7=({struct Cyc_Core_Opt*_tmp41=_cycalloc(sizeof(*_tmp41));_tmp41->v=c2;_tmp41;});*f=_tmp2C7;});
# 181
return 1;}}else{_LLB: _tmp31=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp30.f1)->f1;_tmp32=((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp30.f1)->f2;_tmp33=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp30.f2)->f1;_tmp34=((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp30.f2)->f2;_LLC: {struct Cyc_Core_Opt**f1=_tmp31;struct Cyc_Absyn_Kind*k1=_tmp32;struct Cyc_Core_Opt**f2=_tmp33;struct Cyc_Absyn_Kind*k2=_tmp34;
# 187
if(({Cyc_Tcutil_kind_leq(k1,k2);})){
# 189
({struct Cyc_Core_Opt*_tmp2C8=({struct Cyc_Core_Opt*_tmp43=_cycalloc(sizeof(*_tmp43));_tmp43->v=c1;_tmp43;});*f2=_tmp2C8;});
return 1;}else{
# 192
if(({Cyc_Tcutil_kind_leq(k2,k1);})){
# 194
({struct Cyc_Core_Opt*_tmp2C9=({struct Cyc_Core_Opt*_tmp44=_cycalloc(sizeof(*_tmp44));_tmp44->v=c2;_tmp44;});*f1=_tmp2C9;});
return 1;}else{
# 197
return 0;}}}}}}}_LL0:;}}
# 206
static struct Cyc_List_List*Cyc_Tctyp_add_free_tvar(unsigned loc,struct Cyc_List_List*tvs,struct Cyc_Absyn_Tvar*tv){
# 209
{struct Cyc_List_List*tvs2=tvs;for(0;tvs2 != 0;tvs2=tvs2->tl){
if(({Cyc_strptrcmp(((struct Cyc_Absyn_Tvar*)tvs2->hd)->name,tv->name);})== 0){
void*k1=((struct Cyc_Absyn_Tvar*)tvs2->hd)->kind;
void*k2=tv->kind;
if(!({Cyc_Tctyp_constrain_kinds(k1,k2);}))
({struct Cyc_String_pa_PrintArg_struct _tmp48=({struct Cyc_String_pa_PrintArg_struct _tmp27E;_tmp27E.tag=0U,_tmp27E.f1=(struct _fat_ptr)((struct _fat_ptr)*tv->name);_tmp27E;});struct Cyc_String_pa_PrintArg_struct _tmp49=({struct Cyc_String_pa_PrintArg_struct _tmp27D;_tmp27D.tag=0U,({
struct _fat_ptr _tmp2CA=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_kindbound2string(k1);}));_tmp27D.f1=_tmp2CA;});_tmp27D;});struct Cyc_String_pa_PrintArg_struct _tmp4A=({struct Cyc_String_pa_PrintArg_struct _tmp27C;_tmp27C.tag=0U,({struct _fat_ptr _tmp2CB=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_kindbound2string(k2);}));_tmp27C.f1=_tmp2CB;});_tmp27C;});void*_tmp46[3U];_tmp46[0]=& _tmp48,_tmp46[1]=& _tmp49,_tmp46[2]=& _tmp4A;({unsigned _tmp2CD=loc;struct _fat_ptr _tmp2CC=({const char*_tmp47="type variable %s is used with inconsistent kinds %s and %s";_tag_fat(_tmp47,sizeof(char),59U);});Cyc_Tcutil_terr(_tmp2CD,_tmp2CC,_tag_fat(_tmp46,sizeof(void*),3U));});});
# 213
if(tv->identity == - 1)
# 217
tv->identity=((struct Cyc_Absyn_Tvar*)tvs2->hd)->identity;else{
if(tv->identity != ((struct Cyc_Absyn_Tvar*)tvs2->hd)->identity)
({void*_tmp4B=0U;({int(*_tmp2CF)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp4D)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp4D;});struct _fat_ptr _tmp2CE=({const char*_tmp4C="same type variable has different identity!";_tag_fat(_tmp4C,sizeof(char),43U);});_tmp2CF(_tmp2CE,_tag_fat(_tmp4B,sizeof(void*),0U));});});}
# 213
return tvs;}}}
# 223
({int _tmp2D0=({Cyc_Tcutil_new_tvar_id();});tv->identity=_tmp2D0;});
return({struct Cyc_List_List*_tmp4E=_cycalloc(sizeof(*_tmp4E));_tmp4E->hd=tv,_tmp4E->tl=tvs;_tmp4E;});}
# 229
static struct Cyc_List_List*Cyc_Tctyp_fast_add_free_tvar(struct Cyc_List_List*tvs,struct Cyc_Absyn_Tvar*tv){
if(tv->identity == - 1)
({void*_tmp50=0U;({int(*_tmp2D2)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp52)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp52;});struct _fat_ptr _tmp2D1=({const char*_tmp51="fast_add_free_tvar: bad identity in tv";_tag_fat(_tmp51,sizeof(char),39U);});_tmp2D2(_tmp2D1,_tag_fat(_tmp50,sizeof(void*),0U));});});
# 230
{
# 232
struct Cyc_List_List*tvs2=tvs;
# 230
for(0;tvs2 != 0;tvs2=tvs2->tl){
# 233
struct Cyc_Absyn_Tvar*tv2=(struct Cyc_Absyn_Tvar*)tvs2->hd;
if(tv2->identity == - 1)
({void*_tmp53=0U;({int(*_tmp2D4)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp55)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp55;});struct _fat_ptr _tmp2D3=({const char*_tmp54="fast_add_free_tvar: bad identity in tvs2";_tag_fat(_tmp54,sizeof(char),41U);});_tmp2D4(_tmp2D3,_tag_fat(_tmp53,sizeof(void*),0U));});});
# 234
if(tv2->identity == tv->identity)
# 237
return tvs;}}
# 239
return({struct Cyc_List_List*_tmp56=_cycalloc(sizeof(*_tmp56));_tmp56->hd=tv,_tmp56->tl=tvs;_tmp56;});}
# 245
static struct Cyc_List_List*Cyc_Tctyp_fast_add_free_tvar_bool(struct _RegionHandle*r,struct Cyc_List_List*tvs,struct Cyc_Absyn_Tvar*tv,int b){
# 248
if(tv->identity == - 1)
({void*_tmp58=0U;({int(*_tmp2D6)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp5A)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp5A;});struct _fat_ptr _tmp2D5=({const char*_tmp59="fast_add_free_tvar_bool: bad identity in tv";_tag_fat(_tmp59,sizeof(char),44U);});_tmp2D6(_tmp2D5,_tag_fat(_tmp58,sizeof(void*),0U));});});
# 248
{
# 250
struct Cyc_List_List*tvs2=tvs;
# 248
for(0;tvs2 != 0;tvs2=tvs2->tl){
# 251
struct _tuple15*_stmttmp7=(struct _tuple15*)tvs2->hd;int*_tmp5C;struct Cyc_Absyn_Tvar*_tmp5B;_LL1: _tmp5B=_stmttmp7->f1;_tmp5C=(int*)& _stmttmp7->f2;_LL2: {struct Cyc_Absyn_Tvar*tv2=_tmp5B;int*b2=_tmp5C;
if(tv2->identity == - 1)
({void*_tmp5D=0U;({int(*_tmp2D8)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp5F)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp5F;});struct _fat_ptr _tmp2D7=({const char*_tmp5E="fast_add_free_tvar_bool: bad identity in tvs2";_tag_fat(_tmp5E,sizeof(char),46U);});_tmp2D8(_tmp2D7,_tag_fat(_tmp5D,sizeof(void*),0U));});});
# 252
if(tv2->identity == tv->identity){
# 255
*b2=*b2 || b;
return tvs;}}}}
# 259
return({struct Cyc_List_List*_tmp61=_region_malloc(r,sizeof(*_tmp61));({struct _tuple15*_tmp2D9=({struct _tuple15*_tmp60=_region_malloc(r,sizeof(*_tmp60));_tmp60->f1=tv,_tmp60->f2=b;_tmp60;});_tmp61->hd=_tmp2D9;}),_tmp61->tl=tvs;_tmp61;});}
# 262
static int Cyc_Tctyp_typedef_tvar_is_ptr_rgn(struct Cyc_Absyn_Tvar*tvar,struct Cyc_Absyn_Typedefdecl*td){
# 264
if(td != 0){
# 266
if(td->defn != 0){
# 268
void*_stmttmp8=({Cyc_Tcutil_compress((void*)_check_null(td->defn));});void*_tmp63=_stmttmp8;void*_tmp64;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp63)->tag == 3U){_LL1: _tmp64=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp63)->f1).ptr_atts).rgn;_LL2: {void*r=_tmp64;
# 271
{void*_stmttmp9=({Cyc_Tcutil_compress(r);});void*_tmp65=_stmttmp9;struct Cyc_Absyn_Tvar*_tmp66;if(((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp65)->tag == 2U){_LL6: _tmp66=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp65)->f1;_LL7: {struct Cyc_Absyn_Tvar*tv=_tmp66;
# 273
return({Cyc_Absyn_tvar_cmp(tvar,tv);})== 0;}}else{_LL8: _LL9:
 goto _LL5;}_LL5:;}
# 276
goto _LL0;}}else{_LL3: _LL4:
 goto _LL0;}_LL0:;}}else{
# 281
return 1;}
return 0;}
# 285
static struct Cyc_Absyn_Kind*Cyc_Tctyp_tvar_inst_kind(struct Cyc_Absyn_Tvar*tvar,struct Cyc_Absyn_Kind*def_kind,struct Cyc_Absyn_Kind*expected_kind,struct Cyc_Absyn_Typedefdecl*td){
# 288
void*_stmttmpA=({Cyc_Absyn_compress_kb(tvar->kind);});void*_tmp68=_stmttmpA;switch(*((int*)_tmp68)){case 2U: if(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp68)->f2)->kind == Cyc_Absyn_RgnKind){if(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp68)->f2)->aliasqual == Cyc_Absyn_Top){_LL1: _LL2:
 goto _LL4;}else{goto _LL5;}}else{goto _LL5;}case 0U: if(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp68)->f1)->kind == Cyc_Absyn_RgnKind){if(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp68)->f1)->aliasqual == Cyc_Absyn_Top){_LL3: _LL4:
# 297
 if((((int)expected_kind->kind == (int)Cyc_Absyn_BoxKind ||(int)expected_kind->kind == (int)Cyc_Absyn_MemKind)||(int)expected_kind->kind == (int)Cyc_Absyn_AnyKind)&&({Cyc_Tctyp_typedef_tvar_is_ptr_rgn(tvar,td);})){
# 301
if((int)expected_kind->aliasqual == (int)Cyc_Absyn_Aliasable)
return& Cyc_Tcutil_rk;else{
if((int)expected_kind->aliasqual == (int)Cyc_Absyn_Unique)
return& Cyc_Tcutil_urk;}}
# 297
return& Cyc_Tcutil_trk;}else{goto _LL5;}}else{goto _LL5;}default: _LL5: _LL6:
# 307
 return({Cyc_Tcutil_tvar_kind(tvar,def_kind);});}_LL0:;}
# 311
static void Cyc_Tctyp_check_field_atts(unsigned loc,struct _fat_ptr*fn,struct Cyc_List_List*atts){
for(0;atts != 0;atts=atts->tl){
void*_stmttmpB=(void*)atts->hd;void*_tmp6A=_stmttmpB;switch(*((int*)_tmp6A)){case 7U: _LL1: _LL2:
 goto _LL4;case 6U: _LL3: _LL4:
 continue;default: _LL5: _LL6:
({struct Cyc_String_pa_PrintArg_struct _tmp6D=({struct Cyc_String_pa_PrintArg_struct _tmp280;_tmp280.tag=0U,({
struct _fat_ptr _tmp2DA=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_attribute2string((void*)atts->hd);}));_tmp280.f1=_tmp2DA;});_tmp280;});struct Cyc_String_pa_PrintArg_struct _tmp6E=({struct Cyc_String_pa_PrintArg_struct _tmp27F;_tmp27F.tag=0U,_tmp27F.f1=(struct _fat_ptr)((struct _fat_ptr)*fn);_tmp27F;});void*_tmp6B[2U];_tmp6B[0]=& _tmp6D,_tmp6B[1]=& _tmp6E;({unsigned _tmp2DC=loc;struct _fat_ptr _tmp2DB=({const char*_tmp6C="bad attribute %s on member %s";_tag_fat(_tmp6C,sizeof(char),30U);});Cyc_Tcutil_terr(_tmp2DC,_tmp2DB,_tag_fat(_tmp6B,sizeof(void*),2U));});});}_LL0:;}}
# 311
static struct Cyc_Tctyp_CVTEnv Cyc_Tctyp_i_check_valid_type_level_exp(struct Cyc_Absyn_Exp*,struct Cyc_Tcenv_Tenv*,struct Cyc_Tctyp_CVTEnv);struct _tuple16{struct Cyc_Tctyp_CVTEnv f1;struct Cyc_List_List*f2;};
# 326
static struct _tuple16 Cyc_Tctyp_check_clause(unsigned loc,struct Cyc_Tcenv_Tenv*te,struct Cyc_Tctyp_CVTEnv cvtenv,struct _fat_ptr clause_name,struct Cyc_Absyn_Exp*clause){
# 328
struct Cyc_List_List*relns=0;
if(clause != 0){
({Cyc_Tcexp_tcExp(te,0,clause);});
if(!({Cyc_Tcutil_is_integral(clause);}))
({struct Cyc_String_pa_PrintArg_struct _tmp72=({struct Cyc_String_pa_PrintArg_struct _tmp282;_tmp282.tag=0U,_tmp282.f1=(struct _fat_ptr)((struct _fat_ptr)clause_name);_tmp282;});struct Cyc_String_pa_PrintArg_struct _tmp73=({struct Cyc_String_pa_PrintArg_struct _tmp281;_tmp281.tag=0U,({
struct _fat_ptr _tmp2DD=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string((void*)_check_null(clause->topt));}));_tmp281.f1=_tmp2DD;});_tmp281;});void*_tmp70[2U];_tmp70[0]=& _tmp72,_tmp70[1]=& _tmp73;({unsigned _tmp2DF=loc;struct _fat_ptr _tmp2DE=({const char*_tmp71="%s clause has type %s instead of integral type";_tag_fat(_tmp71,sizeof(char),47U);});Cyc_Tcutil_terr(_tmp2DF,_tmp2DE,_tag_fat(_tmp70,sizeof(void*),2U));});});
# 331
cvtenv=({Cyc_Tctyp_i_check_valid_type_level_exp(clause,te,cvtenv);});
# 335
relns=({Cyc_Relations_exp2relns(Cyc_Core_heap_region,clause);});
if(!({Cyc_Relations_consistent_relations(relns);}))
({struct Cyc_String_pa_PrintArg_struct _tmp76=({struct Cyc_String_pa_PrintArg_struct _tmp284;_tmp284.tag=0U,_tmp284.f1=(struct _fat_ptr)((struct _fat_ptr)clause_name);_tmp284;});struct Cyc_String_pa_PrintArg_struct _tmp77=({struct Cyc_String_pa_PrintArg_struct _tmp283;_tmp283.tag=0U,({
struct _fat_ptr _tmp2E0=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_exp2string(clause);}));_tmp283.f1=_tmp2E0;});_tmp283;});void*_tmp74[2U];_tmp74[0]=& _tmp76,_tmp74[1]=& _tmp77;({unsigned _tmp2E2=clause->loc;struct _fat_ptr _tmp2E1=({const char*_tmp75="%s clause '%s' may be unsatisfiable";_tag_fat(_tmp75,sizeof(char),36U);});Cyc_Tcutil_terr(_tmp2E2,_tmp2E1,_tag_fat(_tmp74,sizeof(void*),2U));});});}
# 329
return({struct _tuple16 _tmp285;_tmp285.f1=cvtenv,_tmp285.f2=relns;_tmp285;});}
# 326
static struct Cyc_Tctyp_CVTEnv Cyc_Tctyp_i_check_valid_type(unsigned,struct Cyc_Tcenv_Tenv*,struct Cyc_Tctyp_CVTEnv,struct Cyc_Absyn_Kind*expected_kind,void*t,int put_in_effect,int allow_abs_aggr);
# 376 "tctyp.cyc"
static struct Cyc_Tctyp_CVTEnv Cyc_Tctyp_i_check_valid_aggr(unsigned loc,struct Cyc_Tcenv_Tenv*te,struct Cyc_Tctyp_CVTEnv cvtenv,struct Cyc_Absyn_Kind*expected_kind,union Cyc_Absyn_AggrInfo*info,struct Cyc_List_List**targs,int allow_abs_aggr){
# 381
{union Cyc_Absyn_AggrInfo _stmttmpC=*info;union Cyc_Absyn_AggrInfo _tmp79=_stmttmpC;struct Cyc_Absyn_Aggrdecl*_tmp7A;struct Cyc_Core_Opt*_tmp7D;struct _tuple1*_tmp7C;enum Cyc_Absyn_AggrKind _tmp7B;if((_tmp79.UnknownAggr).tag == 1){_LL1: _tmp7B=((_tmp79.UnknownAggr).val).f1;_tmp7C=((_tmp79.UnknownAggr).val).f2;_tmp7D=((_tmp79.UnknownAggr).val).f3;_LL2: {enum Cyc_Absyn_AggrKind k=_tmp7B;struct _tuple1*n=_tmp7C;struct Cyc_Core_Opt*tgd=_tmp7D;
# 383
struct Cyc_Absyn_Aggrdecl**adp;
{struct _handler_cons _tmp7E;_push_handler(& _tmp7E);{int _tmp80=0;if(setjmp(_tmp7E.handler))_tmp80=1;if(!_tmp80){
adp=({Cyc_Tcenv_lookup_aggrdecl(te,loc,n);});{
struct Cyc_Absyn_Aggrdecl*ad=*adp;
if((int)ad->kind != (int)k){
if((int)ad->kind == (int)Cyc_Absyn_StructA)
({struct Cyc_String_pa_PrintArg_struct _tmp83=({struct Cyc_String_pa_PrintArg_struct _tmp287;_tmp287.tag=0U,({
struct _fat_ptr _tmp2E3=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(n);}));_tmp287.f1=_tmp2E3;});_tmp287;});struct Cyc_String_pa_PrintArg_struct _tmp84=({struct Cyc_String_pa_PrintArg_struct _tmp286;_tmp286.tag=0U,({struct _fat_ptr _tmp2E4=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(n);}));_tmp286.f1=_tmp2E4;});_tmp286;});void*_tmp81[2U];_tmp81[0]=& _tmp83,_tmp81[1]=& _tmp84;({unsigned _tmp2E6=loc;struct _fat_ptr _tmp2E5=({const char*_tmp82="expecting struct %s instead of union %s";_tag_fat(_tmp82,sizeof(char),40U);});Cyc_Tcutil_terr(_tmp2E6,_tmp2E5,_tag_fat(_tmp81,sizeof(void*),2U));});});else{
# 392
({struct Cyc_String_pa_PrintArg_struct _tmp87=({struct Cyc_String_pa_PrintArg_struct _tmp289;_tmp289.tag=0U,({
struct _fat_ptr _tmp2E7=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(n);}));_tmp289.f1=_tmp2E7;});_tmp289;});struct Cyc_String_pa_PrintArg_struct _tmp88=({struct Cyc_String_pa_PrintArg_struct _tmp288;_tmp288.tag=0U,({struct _fat_ptr _tmp2E8=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(n);}));_tmp288.f1=_tmp2E8;});_tmp288;});void*_tmp85[2U];_tmp85[0]=& _tmp87,_tmp85[1]=& _tmp88;({unsigned _tmp2EA=loc;struct _fat_ptr _tmp2E9=({const char*_tmp86="expecting union %s instead of struct %s";_tag_fat(_tmp86,sizeof(char),40U);});Cyc_Tcutil_terr(_tmp2EA,_tmp2E9,_tag_fat(_tmp85,sizeof(void*),2U));});});}}
# 387
if(
# 395
(unsigned)tgd &&(int)tgd->v){
if(!((unsigned)ad->impl)|| !((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->tagged)
({struct Cyc_String_pa_PrintArg_struct _tmp8B=({struct Cyc_String_pa_PrintArg_struct _tmp28A;_tmp28A.tag=0U,({
struct _fat_ptr _tmp2EB=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(n);}));_tmp28A.f1=_tmp2EB;});_tmp28A;});void*_tmp89[1U];_tmp89[0]=& _tmp8B;({unsigned _tmp2ED=loc;struct _fat_ptr _tmp2EC=({const char*_tmp8A="@tagged qualfiers don't agree on union %s";_tag_fat(_tmp8A,sizeof(char),42U);});Cyc_Tcutil_terr(_tmp2ED,_tmp2EC,_tag_fat(_tmp89,sizeof(void*),1U));});});}
# 387
({union Cyc_Absyn_AggrInfo _tmp2EE=({Cyc_Absyn_KnownAggr(adp);});*info=_tmp2EE;});}
# 385
;_pop_handler();}else{void*_tmp7F=(void*)Cyc_Core_get_exn_thrown();void*_tmp8C=_tmp7F;void*_tmp8D;if(((struct Cyc_Dict_Absent_exn_struct*)_tmp8C)->tag == Cyc_Dict_Absent){_LL6: _LL7: {
# 405
struct Cyc_Absyn_Aggrdecl*ad=({struct Cyc_Absyn_Aggrdecl*_tmp91=_cycalloc(sizeof(*_tmp91));_tmp91->kind=k,_tmp91->sc=Cyc_Absyn_Extern,_tmp91->name=n,_tmp91->tvs=0,_tmp91->impl=0,_tmp91->attributes=0,_tmp91->expected_mem_kind=0;_tmp91;});
({Cyc_Tc_tcAggrdecl(te,loc,ad);});
adp=({Cyc_Tcenv_lookup_aggrdecl(te,loc,n);});
({union Cyc_Absyn_AggrInfo _tmp2EF=({Cyc_Absyn_KnownAggr(adp);});*info=_tmp2EF;});
# 410
if(*targs != 0){
({struct Cyc_String_pa_PrintArg_struct _tmp90=({struct Cyc_String_pa_PrintArg_struct _tmp28B;_tmp28B.tag=0U,({struct _fat_ptr _tmp2F0=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(n);}));_tmp28B.f1=_tmp2F0;});_tmp28B;});void*_tmp8E[1U];_tmp8E[0]=& _tmp90;({unsigned _tmp2F2=loc;struct _fat_ptr _tmp2F1=({const char*_tmp8F="declare parameterized type %s before using";_tag_fat(_tmp8F,sizeof(char),43U);});Cyc_Tcutil_terr(_tmp2F2,_tmp2F1,_tag_fat(_tmp8E,sizeof(void*),1U));});});
return cvtenv;}
# 410
goto _LL5;}}else{_LL8: _tmp8D=_tmp8C;_LL9: {void*exn=_tmp8D;(int)_rethrow(exn);}}_LL5:;}}}
# 416
_tmp7A=*adp;goto _LL4;}}else{_LL3: _tmp7A=*(_tmp79.KnownAggr).val;_LL4: {struct Cyc_Absyn_Aggrdecl*ad=_tmp7A;
# 418
struct Cyc_List_List*tvs=ad->tvs;
struct Cyc_List_List*ts=*targs;
for(0;ts != 0 && tvs != 0;(ts=ts->tl,tvs=tvs->tl)){
struct Cyc_Absyn_Tvar*tv=(struct Cyc_Absyn_Tvar*)tvs->hd;
void*t=(void*)ts->hd;
# 426
{struct _tuple0 _stmttmpD=({struct _tuple0 _tmp28C;({void*_tmp2F3=({Cyc_Absyn_compress_kb(tv->kind);});_tmp28C.f1=_tmp2F3;}),_tmp28C.f2=t;_tmp28C;});struct _tuple0 _tmp92=_stmttmpD;struct Cyc_Absyn_Tvar*_tmp93;if(((struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*)_tmp92.f1)->tag == 1U){if(((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp92.f2)->tag == 2U){_LLB: _tmp93=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp92.f2)->f1;_LLC: {struct Cyc_Absyn_Tvar*tv2=_tmp93;
# 428
({struct Cyc_List_List*_tmp2F4=({Cyc_Tctyp_add_free_tvar(loc,cvtenv.kind_env,tv2);});cvtenv.kind_env=_tmp2F4;});
({struct Cyc_List_List*_tmp2F5=({Cyc_Tctyp_fast_add_free_tvar_bool(cvtenv.r,cvtenv.free_vars,tv2,1);});cvtenv.free_vars=_tmp2F5;});
# 431
continue;}}else{goto _LLD;}}else{_LLD: _LLE:
 goto _LLA;}_LLA:;}{
# 434
struct Cyc_Absyn_Kind*k=({Cyc_Tcutil_tvar_kind((struct Cyc_Absyn_Tvar*)tvs->hd,& Cyc_Tcutil_bk);});
cvtenv=({Cyc_Tctyp_i_check_valid_type(loc,te,cvtenv,k,(void*)ts->hd,1,allow_abs_aggr);});
({Cyc_Tcutil_check_no_qual(loc,(void*)ts->hd);});}}
# 438
if(ts != 0)
({struct Cyc_String_pa_PrintArg_struct _tmp96=({struct Cyc_String_pa_PrintArg_struct _tmp28D;_tmp28D.tag=0U,({struct _fat_ptr _tmp2F6=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(ad->name);}));_tmp28D.f1=_tmp2F6;});_tmp28D;});void*_tmp94[1U];_tmp94[0]=& _tmp96;({unsigned _tmp2F8=loc;struct _fat_ptr _tmp2F7=({const char*_tmp95="too many parameters for type %s";_tag_fat(_tmp95,sizeof(char),32U);});Cyc_Tcutil_terr(_tmp2F8,_tmp2F7,_tag_fat(_tmp94,sizeof(void*),1U));});});
# 438
if(tvs != 0){
# 442
struct Cyc_List_List*hidden_ts=0;
for(0;tvs != 0;tvs=tvs->tl){
struct Cyc_Absyn_Kind*k=({Cyc_Tctyp_tvar_inst_kind((struct Cyc_Absyn_Tvar*)tvs->hd,& Cyc_Tcutil_bk,expected_kind,0);});
void*e=({Cyc_Absyn_new_evar(0,0);});
hidden_ts=({struct Cyc_List_List*_tmp97=_cycalloc(sizeof(*_tmp97));_tmp97->hd=e,_tmp97->tl=hidden_ts;_tmp97;});
cvtenv=({Cyc_Tctyp_i_check_valid_type(loc,te,cvtenv,k,e,1,allow_abs_aggr);});}
# 449
({struct Cyc_List_List*_tmp2F9=({struct Cyc_List_List*(*_tmp98)(struct Cyc_List_List*x,struct Cyc_List_List*y)=Cyc_List_imp_append;struct Cyc_List_List*_tmp99=*targs;struct Cyc_List_List*_tmp9A=({Cyc_List_imp_rev(hidden_ts);});_tmp98(_tmp99,_tmp9A);});*targs=_tmp2F9;});}
# 438
if(
# 451
(allow_abs_aggr && ad->impl == 0)&& !({Cyc_Tcutil_kind_leq(& Cyc_Tcutil_ak,expected_kind);}))
# 456
ad->expected_mem_kind=1;}}_LL0:;}
# 459
return cvtenv;}
# 464
static struct Cyc_Tctyp_CVTEnv Cyc_Tctyp_i_check_valid_datatype(unsigned loc,struct Cyc_Tcenv_Tenv*te,struct Cyc_Tctyp_CVTEnv cvtenv,struct Cyc_Absyn_Kind*expected_kind,union Cyc_Absyn_DatatypeInfo*info,struct Cyc_List_List**targsp,int allow_abs_aggr){
# 469
struct Cyc_List_List*targs=*targsp;
{union Cyc_Absyn_DatatypeInfo _stmttmpE=*info;union Cyc_Absyn_DatatypeInfo _tmp9C=_stmttmpE;struct Cyc_Absyn_Datatypedecl*_tmp9D;int _tmp9F;struct _tuple1*_tmp9E;if((_tmp9C.UnknownDatatype).tag == 1){_LL1: _tmp9E=((_tmp9C.UnknownDatatype).val).name;_tmp9F=((_tmp9C.UnknownDatatype).val).is_extensible;_LL2: {struct _tuple1*n=_tmp9E;int is_x=_tmp9F;
# 472
struct Cyc_Absyn_Datatypedecl**tudp;
{struct _handler_cons _tmpA0;_push_handler(& _tmpA0);{int _tmpA2=0;if(setjmp(_tmpA0.handler))_tmpA2=1;if(!_tmpA2){tudp=({Cyc_Tcenv_lookup_datatypedecl(te,loc,n);});;_pop_handler();}else{void*_tmpA1=(void*)Cyc_Core_get_exn_thrown();void*_tmpA3=_tmpA1;void*_tmpA4;if(((struct Cyc_Dict_Absent_exn_struct*)_tmpA3)->tag == Cyc_Dict_Absent){_LL6: _LL7: {
# 476
struct Cyc_Absyn_Datatypedecl*tud=({struct Cyc_Absyn_Datatypedecl*_tmpA8=_cycalloc(sizeof(*_tmpA8));_tmpA8->sc=Cyc_Absyn_Extern,_tmpA8->name=n,_tmpA8->tvs=0,_tmpA8->fields=0,_tmpA8->is_extensible=is_x;_tmpA8;});
({Cyc_Tc_tcDatatypedecl(te,loc,tud);});
tudp=({Cyc_Tcenv_lookup_datatypedecl(te,loc,n);});
# 480
if(targs != 0){
({struct Cyc_String_pa_PrintArg_struct _tmpA7=({struct Cyc_String_pa_PrintArg_struct _tmp28E;_tmp28E.tag=0U,({
struct _fat_ptr _tmp2FA=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(n);}));_tmp28E.f1=_tmp2FA;});_tmp28E;});void*_tmpA5[1U];_tmpA5[0]=& _tmpA7;({unsigned _tmp2FC=loc;struct _fat_ptr _tmp2FB=({const char*_tmpA6="declare parameterized datatype %s before using";_tag_fat(_tmpA6,sizeof(char),47U);});Cyc_Tcutil_terr(_tmp2FC,_tmp2FB,_tag_fat(_tmpA5,sizeof(void*),1U));});});
return cvtenv;}
# 480
goto _LL5;}}else{_LL8: _tmpA4=_tmpA3;_LL9: {void*exn=_tmpA4;(int)_rethrow(exn);}}_LL5:;}}}
# 489
if(is_x && !(*tudp)->is_extensible)
({struct Cyc_String_pa_PrintArg_struct _tmpAB=({struct Cyc_String_pa_PrintArg_struct _tmp28F;_tmp28F.tag=0U,({struct _fat_ptr _tmp2FD=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(n);}));_tmp28F.f1=_tmp2FD;});_tmp28F;});void*_tmpA9[1U];_tmpA9[0]=& _tmpAB;({unsigned _tmp2FF=loc;struct _fat_ptr _tmp2FE=({const char*_tmpAA="datatype %s was not declared @extensible";_tag_fat(_tmpAA,sizeof(char),41U);});Cyc_Tcutil_terr(_tmp2FF,_tmp2FE,_tag_fat(_tmpA9,sizeof(void*),1U));});});
# 489
({union Cyc_Absyn_DatatypeInfo _tmp300=({Cyc_Absyn_KnownDatatype(tudp);});*info=_tmp300;});
# 492
_tmp9D=*tudp;goto _LL4;}}else{_LL3: _tmp9D=*(_tmp9C.KnownDatatype).val;_LL4: {struct Cyc_Absyn_Datatypedecl*tud=_tmp9D;
# 495
struct Cyc_List_List*tvs=tud->tvs;
for(0;targs != 0 && tvs != 0;(targs=targs->tl,tvs=tvs->tl)){
void*t=(void*)targs->hd;
struct Cyc_Absyn_Tvar*tv=(struct Cyc_Absyn_Tvar*)tvs->hd;
# 501
{struct _tuple0 _stmttmpF=({struct _tuple0 _tmp290;({void*_tmp301=({Cyc_Absyn_compress_kb(tv->kind);});_tmp290.f1=_tmp301;}),_tmp290.f2=t;_tmp290;});struct _tuple0 _tmpAC=_stmttmpF;struct Cyc_Absyn_Tvar*_tmpAD;if(((struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*)_tmpAC.f1)->tag == 1U){if(((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmpAC.f2)->tag == 2U){_LLB: _tmpAD=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmpAC.f2)->f1;_LLC: {struct Cyc_Absyn_Tvar*tv2=_tmpAD;
# 503
({struct Cyc_List_List*_tmp302=({Cyc_Tctyp_add_free_tvar(loc,cvtenv.kind_env,tv2);});cvtenv.kind_env=_tmp302;});
({struct Cyc_List_List*_tmp303=({Cyc_Tctyp_fast_add_free_tvar_bool(cvtenv.r,cvtenv.free_vars,tv2,1);});cvtenv.free_vars=_tmp303;});
# 506
continue;}}else{goto _LLD;}}else{_LLD: _LLE:
 goto _LLA;}_LLA:;}{
# 509
struct Cyc_Absyn_Kind*k=({Cyc_Tcutil_tvar_kind(tv,& Cyc_Tcutil_bk);});
cvtenv=({Cyc_Tctyp_i_check_valid_type(loc,te,cvtenv,k,t,1,allow_abs_aggr);});
({Cyc_Tcutil_check_no_qual(loc,t);});}}
# 513
if(targs != 0)
({struct Cyc_String_pa_PrintArg_struct _tmpB0=({struct Cyc_String_pa_PrintArg_struct _tmp291;_tmp291.tag=0U,({
struct _fat_ptr _tmp304=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(tud->name);}));_tmp291.f1=_tmp304;});_tmp291;});void*_tmpAE[1U];_tmpAE[0]=& _tmpB0;({unsigned _tmp306=loc;struct _fat_ptr _tmp305=({const char*_tmpAF="too many type arguments for datatype %s";_tag_fat(_tmpAF,sizeof(char),40U);});Cyc_Tcutil_terr(_tmp306,_tmp305,_tag_fat(_tmpAE,sizeof(void*),1U));});});
# 513
if(tvs != 0){
# 518
struct Cyc_List_List*hidden_ts=0;
for(0;tvs != 0;tvs=tvs->tl){
struct Cyc_Absyn_Kind*k1=({Cyc_Tctyp_tvar_inst_kind((struct Cyc_Absyn_Tvar*)tvs->hd,& Cyc_Tcutil_bk,expected_kind,0);});
void*e=({Cyc_Absyn_new_evar(0,0);});
hidden_ts=({struct Cyc_List_List*_tmpB1=_cycalloc(sizeof(*_tmpB1));_tmpB1->hd=e,_tmpB1->tl=hidden_ts;_tmpB1;});
cvtenv=({Cyc_Tctyp_i_check_valid_type(loc,te,cvtenv,k1,e,1,allow_abs_aggr);});}
# 525
({struct Cyc_List_List*_tmp307=({struct Cyc_List_List*(*_tmpB2)(struct Cyc_List_List*x,struct Cyc_List_List*y)=Cyc_List_imp_append;struct Cyc_List_List*_tmpB3=*targsp;struct Cyc_List_List*_tmpB4=({Cyc_List_imp_rev(hidden_ts);});_tmpB2(_tmpB3,_tmpB4);});*targsp=_tmp307;});}
# 513
goto _LL0;}}_LL0:;}
# 529
return cvtenv;}
# 534
static struct Cyc_Tctyp_CVTEnv Cyc_Tctyp_i_check_valid_datatype_field(unsigned loc,struct Cyc_Tcenv_Tenv*te,struct Cyc_Tctyp_CVTEnv cvtenv,struct Cyc_Absyn_Kind*expected_kind,union Cyc_Absyn_DatatypeFieldInfo*info,struct Cyc_List_List*targs,int allow_abs_aggr){
# 539
{union Cyc_Absyn_DatatypeFieldInfo _stmttmp10=*info;union Cyc_Absyn_DatatypeFieldInfo _tmpB6=_stmttmp10;struct Cyc_Absyn_Datatypefield*_tmpB8;struct Cyc_Absyn_Datatypedecl*_tmpB7;int _tmpBB;struct _tuple1*_tmpBA;struct _tuple1*_tmpB9;if((_tmpB6.UnknownDatatypefield).tag == 1){_LL1: _tmpB9=((_tmpB6.UnknownDatatypefield).val).datatype_name;_tmpBA=((_tmpB6.UnknownDatatypefield).val).field_name;_tmpBB=((_tmpB6.UnknownDatatypefield).val).is_extensible;_LL2: {struct _tuple1*tname=_tmpB9;struct _tuple1*fname=_tmpBA;int is_x=_tmpBB;
# 542
struct Cyc_Absyn_Datatypedecl*tud=*({Cyc_Tcenv_lookup_datatypedecl(te,loc,tname);});
struct Cyc_Absyn_Datatypefield*tuf;
# 547
{struct Cyc_List_List*fs=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(tud->fields))->v;for(0;1;fs=fs->tl){
if(fs == 0)({void*_tmpBC=0U;({int(*_tmp309)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmpBE)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmpBE;});struct _fat_ptr _tmp308=({const char*_tmpBD="Tcutil found a bad datatypefield";_tag_fat(_tmpBD,sizeof(char),33U);});_tmp309(_tmp308,_tag_fat(_tmpBC,sizeof(void*),0U));});});if(({Cyc_Absyn_qvar_cmp(((struct Cyc_Absyn_Datatypefield*)fs->hd)->name,fname);})== 0){
# 550
tuf=(struct Cyc_Absyn_Datatypefield*)fs->hd;
break;}}}
# 554
({union Cyc_Absyn_DatatypeFieldInfo _tmp30A=({Cyc_Absyn_KnownDatatypefield(tud,tuf);});*info=_tmp30A;});
_tmpB7=tud;_tmpB8=tuf;goto _LL4;}}else{_LL3: _tmpB7=((_tmpB6.KnownDatatypefield).val).f1;_tmpB8=((_tmpB6.KnownDatatypefield).val).f2;_LL4: {struct Cyc_Absyn_Datatypedecl*tud=_tmpB7;struct Cyc_Absyn_Datatypefield*tuf=_tmpB8;
# 558
struct Cyc_List_List*tvs=tud->tvs;
for(0;targs != 0 && tvs != 0;(targs=targs->tl,tvs=tvs->tl)){
void*t=(void*)targs->hd;
struct Cyc_Absyn_Tvar*tv=(struct Cyc_Absyn_Tvar*)tvs->hd;
# 564
{struct _tuple0 _stmttmp11=({struct _tuple0 _tmp292;({void*_tmp30B=({Cyc_Absyn_compress_kb(tv->kind);});_tmp292.f1=_tmp30B;}),_tmp292.f2=t;_tmp292;});struct _tuple0 _tmpBF=_stmttmp11;struct Cyc_Absyn_Tvar*_tmpC0;if(((struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*)_tmpBF.f1)->tag == 1U){if(((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmpBF.f2)->tag == 2U){_LL6: _tmpC0=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmpBF.f2)->f1;_LL7: {struct Cyc_Absyn_Tvar*tv2=_tmpC0;
# 566
({struct Cyc_List_List*_tmp30C=({Cyc_Tctyp_add_free_tvar(loc,cvtenv.kind_env,tv2);});cvtenv.kind_env=_tmp30C;});
({struct Cyc_List_List*_tmp30D=({Cyc_Tctyp_fast_add_free_tvar_bool(cvtenv.r,cvtenv.free_vars,tv2,1);});cvtenv.free_vars=_tmp30D;});
# 569
continue;}}else{goto _LL8;}}else{_LL8: _LL9:
 goto _LL5;}_LL5:;}{
# 572
struct Cyc_Absyn_Kind*k=({Cyc_Tcutil_tvar_kind(tv,& Cyc_Tcutil_bk);});
cvtenv=({Cyc_Tctyp_i_check_valid_type(loc,te,cvtenv,k,t,1,allow_abs_aggr);});
({Cyc_Tcutil_check_no_qual(loc,t);});}}
# 576
if(targs != 0)
({struct Cyc_String_pa_PrintArg_struct _tmpC3=({struct Cyc_String_pa_PrintArg_struct _tmp294;_tmp294.tag=0U,({
struct _fat_ptr _tmp30E=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(tud->name);}));_tmp294.f1=_tmp30E;});_tmp294;});struct Cyc_String_pa_PrintArg_struct _tmpC4=({struct Cyc_String_pa_PrintArg_struct _tmp293;_tmp293.tag=0U,({struct _fat_ptr _tmp30F=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(tuf->name);}));_tmp293.f1=_tmp30F;});_tmp293;});void*_tmpC1[2U];_tmpC1[0]=& _tmpC3,_tmpC1[1]=& _tmpC4;({unsigned _tmp311=loc;struct _fat_ptr _tmp310=({const char*_tmpC2="too many type arguments for datatype %s.%s";_tag_fat(_tmpC2,sizeof(char),43U);});Cyc_Tcutil_terr(_tmp311,_tmp310,_tag_fat(_tmpC1,sizeof(void*),2U));});});
# 576
if(tvs != 0)
# 580
({struct Cyc_String_pa_PrintArg_struct _tmpC7=({struct Cyc_String_pa_PrintArg_struct _tmp296;_tmp296.tag=0U,({
struct _fat_ptr _tmp312=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(tud->name);}));_tmp296.f1=_tmp312;});_tmp296;});struct Cyc_String_pa_PrintArg_struct _tmpC8=({struct Cyc_String_pa_PrintArg_struct _tmp295;_tmp295.tag=0U,({struct _fat_ptr _tmp313=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(tuf->name);}));_tmp295.f1=_tmp313;});_tmp295;});void*_tmpC5[2U];_tmpC5[0]=& _tmpC7,_tmpC5[1]=& _tmpC8;({unsigned _tmp315=loc;struct _fat_ptr _tmp314=({const char*_tmpC6="too few type arguments for datatype %s.%s";_tag_fat(_tmpC6,sizeof(char),42U);});Cyc_Tcutil_terr(_tmp315,_tmp314,_tag_fat(_tmpC5,sizeof(void*),2U));});});
# 576
goto _LL0;}}_LL0:;}
# 584
return cvtenv;}
# 588
static struct Cyc_Tctyp_CVTEnv Cyc_Tctyp_i_check_valid_type_app(unsigned loc,struct Cyc_Tcenv_Tenv*te,struct Cyc_Tctyp_CVTEnv cvtenv,struct Cyc_Absyn_Kind*expected_kind,void*c,struct Cyc_List_List**targsp,int put_in_effect,int allow_abs_aggr){
# 592
struct Cyc_List_List*ts=*targsp;
{void*_tmpCA=c;union Cyc_Absyn_DatatypeFieldInfo*_tmpCB;union Cyc_Absyn_DatatypeInfo*_tmpCC;union Cyc_Absyn_AggrInfo*_tmpCD;struct Cyc_List_List*_tmpCE;struct Cyc_Absyn_Enumdecl**_tmpD0;struct _tuple1*_tmpCF;switch(*((int*)_tmpCA)){case 1U: _LL1: _LL2:
# 595
 goto _LL4;case 2U: _LL3: _LL4: goto _LL6;case 0U: _LL5: _LL6: goto _LL8;case 8U: _LL7: _LL8:
 goto _LLA;case 7U: _LL9: _LLA: goto _LLC;case 6U: _LLB: _LLC: goto _LLE;case 14U: _LLD: _LLE:
 goto _LL10;case 13U: _LLF: _LL10: goto _LL12;case 16U: _LL11: _LL12: goto _LL14;case 19U: _LL13: _LL14:
# 599
 if(ts != 0)
({struct Cyc_String_pa_PrintArg_struct _tmpD4=({struct Cyc_String_pa_PrintArg_struct _tmp297;_tmp297.tag=0U,({struct _fat_ptr _tmp316=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string((void*)({struct Cyc_Absyn_AppType_Absyn_Type_struct*_tmpD5=_cycalloc(sizeof(*_tmpD5));_tmpD5->tag=0U,_tmpD5->f1=c,_tmpD5->f2=ts;_tmpD5;}));}));_tmp297.f1=_tmp316;});_tmp297;});void*_tmpD1[1U];_tmpD1[0]=& _tmpD4;({int(*_tmp318)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmpD3)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmpD3;});struct _fat_ptr _tmp317=({const char*_tmpD2="%s applied to argument(s)";_tag_fat(_tmpD2,sizeof(char),26U);});_tmp318(_tmp317,_tag_fat(_tmpD1,sizeof(void*),1U));});});
# 599
goto _LL0;case 11U: _LL15: _LL16:
# 603
 for(0;ts != 0;ts=ts->tl){
cvtenv=({Cyc_Tctyp_i_check_valid_type(loc,te,cvtenv,& Cyc_Tcutil_ek,(void*)ts->hd,1,1);});}
goto _LL0;case 5U: _LL17: _LL18:
# 607
 if(({Cyc_List_length(ts);})!= 1)
({void*_tmpD6=0U;({int(*_tmp31A)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmpD8)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmpD8;});struct _fat_ptr _tmp319=({const char*_tmpD7="tag_t applied to wrong number of arguments";_tag_fat(_tmpD7,sizeof(char),43U);});_tmp31A(_tmp319,_tag_fat(_tmpD6,sizeof(void*),0U));});});
# 607
cvtenv=({Cyc_Tctyp_i_check_valid_type(loc,te,cvtenv,& Cyc_Tcutil_ik,(void*)((struct Cyc_List_List*)_check_null(ts))->hd,0,1);});
# 610
goto _LL0;case 17U: _LL19: _tmpCF=((struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct*)_tmpCA)->f1;_tmpD0=(struct Cyc_Absyn_Enumdecl**)&((struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct*)_tmpCA)->f2;_LL1A: {struct _tuple1*n=_tmpCF;struct Cyc_Absyn_Enumdecl**edp=_tmpD0;
# 612
if(ts != 0)({void*_tmpD9=0U;({int(*_tmp31C)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmpDB)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmpDB;});struct _fat_ptr _tmp31B=({const char*_tmpDA="enum applied to argument(s)";_tag_fat(_tmpDA,sizeof(char),28U);});_tmp31C(_tmp31B,_tag_fat(_tmpD9,sizeof(void*),0U));});});if(
*edp == 0 ||((struct Cyc_Absyn_Enumdecl*)_check_null(*edp))->fields == 0){
struct _handler_cons _tmpDC;_push_handler(& _tmpDC);{int _tmpDE=0;if(setjmp(_tmpDC.handler))_tmpDE=1;if(!_tmpDE){
{struct Cyc_Absyn_Enumdecl**ed=({Cyc_Tcenv_lookup_enumdecl(te,loc,n);});
*edp=*ed;}
# 615
;_pop_handler();}else{void*_tmpDD=(void*)Cyc_Core_get_exn_thrown();void*_tmpDF=_tmpDD;void*_tmpE0;if(((struct Cyc_Dict_Absent_exn_struct*)_tmpDF)->tag == Cyc_Dict_Absent){_LL30: _LL31: {
# 619
struct Cyc_Absyn_Enumdecl*ed_orig=({struct Cyc_Absyn_Enumdecl*_tmpE1=_cycalloc(sizeof(*_tmpE1));_tmpE1->sc=Cyc_Absyn_Extern,_tmpE1->name=n,_tmpE1->fields=0;_tmpE1;});
({Cyc_Tc_tcEnumdecl(te,loc,ed_orig);});{
struct Cyc_Absyn_Enumdecl**ed=({Cyc_Tcenv_lookup_enumdecl(te,loc,n);});
*edp=*ed;
goto _LL2F;}}}else{_LL32: _tmpE0=_tmpDF;_LL33: {void*exn=_tmpE0;(int)_rethrow(exn);}}_LL2F:;}}}
# 612
goto _LL0;}case 18U: _LL1B: _tmpCE=((struct Cyc_Absyn_AnonEnumCon_Absyn_TyCon_struct*)_tmpCA)->f1;_LL1C: {struct Cyc_List_List*fs=_tmpCE;
# 627
if(ts != 0)({void*_tmpE2=0U;({int(*_tmp31E)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmpE4)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmpE4;});struct _fat_ptr _tmp31D=({const char*_tmpE3="enum applied to argument(s)";_tag_fat(_tmpE3,sizeof(char),28U);});_tmp31E(_tmp31D,_tag_fat(_tmpE2,sizeof(void*),0U));});});{struct Cyc_List_List*prev_fields=0;
# 630
unsigned tag_count=0U;
for(0;fs != 0;fs=fs->tl){
struct Cyc_Absyn_Enumfield*f=(struct Cyc_Absyn_Enumfield*)fs->hd;
if(({({int(*_tmp320)(int(*compare)(struct _fat_ptr*,struct _fat_ptr*),struct Cyc_List_List*l,struct _fat_ptr*x)=({int(*_tmpE5)(int(*compare)(struct _fat_ptr*,struct _fat_ptr*),struct Cyc_List_List*l,struct _fat_ptr*x)=(int(*)(int(*compare)(struct _fat_ptr*,struct _fat_ptr*),struct Cyc_List_List*l,struct _fat_ptr*x))Cyc_List_mem;_tmpE5;});struct Cyc_List_List*_tmp31F=prev_fields;_tmp320(Cyc_strptrcmp,_tmp31F,(*f->name).f2);});}))
({struct Cyc_String_pa_PrintArg_struct _tmpE8=({struct Cyc_String_pa_PrintArg_struct _tmp298;_tmp298.tag=0U,_tmp298.f1=(struct _fat_ptr)((struct _fat_ptr)*(*f->name).f2);_tmp298;});void*_tmpE6[1U];_tmpE6[0]=& _tmpE8;({unsigned _tmp322=f->loc;struct _fat_ptr _tmp321=({const char*_tmpE7="duplicate enum field name %s";_tag_fat(_tmpE7,sizeof(char),29U);});Cyc_Tcutil_terr(_tmp322,_tmp321,_tag_fat(_tmpE6,sizeof(void*),1U));});});else{
# 636
prev_fields=({struct Cyc_List_List*_tmpE9=_cycalloc(sizeof(*_tmpE9));_tmpE9->hd=(*f->name).f2,_tmpE9->tl=prev_fields;_tmpE9;});}
if(f->tag == 0)
({struct Cyc_Absyn_Exp*_tmp323=({Cyc_Absyn_uint_exp(tag_count,f->loc);});f->tag=_tmp323;});else{
if(!({Cyc_Tcutil_is_const_exp((struct Cyc_Absyn_Exp*)_check_null(f->tag));}))
({struct Cyc_String_pa_PrintArg_struct _tmpEC=({struct Cyc_String_pa_PrintArg_struct _tmp299;_tmp299.tag=0U,_tmp299.f1=(struct _fat_ptr)((struct _fat_ptr)*(*f->name).f2);_tmp299;});void*_tmpEA[1U];_tmpEA[0]=& _tmpEC;({unsigned _tmp325=loc;struct _fat_ptr _tmp324=({const char*_tmpEB="enum field %s: expression is not constant";_tag_fat(_tmpEB,sizeof(char),42U);});Cyc_Tcutil_terr(_tmp325,_tmp324,_tag_fat(_tmpEA,sizeof(void*),1U));});});}{
# 637
unsigned t1=({Cyc_Evexp_eval_const_uint_exp((struct Cyc_Absyn_Exp*)_check_null(f->tag));}).f1;
# 642
tag_count=t1 + (unsigned)1;}}
# 644
goto _LL0;}}case 12U: _LL1D: _LL1E:
# 646
 if(({Cyc_List_length(ts);})!= 1)({void*_tmpED=0U;({int(*_tmp327)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmpEF)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmpEF;});struct _fat_ptr _tmp326=({const char*_tmpEE="regions has wrong number of arguments";_tag_fat(_tmpEE,sizeof(char),38U);});_tmp327(_tmp326,_tag_fat(_tmpED,sizeof(void*),0U));});});cvtenv=({Cyc_Tctyp_i_check_valid_type(loc,te,cvtenv,& Cyc_Tcutil_tak,(void*)((struct Cyc_List_List*)_check_null(ts))->hd,1,1);});
goto _LL0;case 4U: _LL1F: _LL20:
# 649
 if(({Cyc_List_length(ts);})!= 1)({void*_tmpF0=0U;({int(*_tmp329)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmpF2)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmpF2;});struct _fat_ptr _tmp328=({const char*_tmpF1="xregion_t has wrong number of arguments";_tag_fat(_tmpF1,sizeof(char),40U);});_tmp329(_tmp328,_tag_fat(_tmpF0,sizeof(void*),0U));});});cvtenv=({Cyc_Tctyp_i_check_valid_type(loc,te,cvtenv,& Cyc_Tcutil_xrk,(void*)((struct Cyc_List_List*)_check_null(ts))->hd,1,1);});
# 651
goto _LL0;case 3U: _LL21: _LL22:
# 653
 if(({Cyc_List_length(ts);})!= 1)({void*_tmpF3=0U;({int(*_tmp32B)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmpF5)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmpF5;});struct _fat_ptr _tmp32A=({const char*_tmpF4="region_t has wrong number of arguments";_tag_fat(_tmpF4,sizeof(char),39U);});_tmp32B(_tmp32A,_tag_fat(_tmpF3,sizeof(void*),0U));});});{struct Cyc_Absyn_Kind*kind=& Cyc_Tcutil_trk;
# 655
if(({int(*_tmpF6)(void*r)=Cyc_Absyn_is_xrgn;void*_tmpF7=({Cyc_Tcutil_compress((void*)((struct Cyc_List_List*)_check_null(ts))->hd);});_tmpF6(_tmpF7);}))kind=& Cyc_Tcutil_xrk;cvtenv=({Cyc_Tctyp_i_check_valid_type(loc,te,cvtenv,kind,(void*)ts->hd,1,1);});
# 657
goto _LL0;}case 15U: _LL23: _LL24:
# 659
 if(({Cyc_List_length(ts);})!= 1)({void*_tmpF8=0U;({int(*_tmp32D)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmpFA)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmpFA;});struct _fat_ptr _tmp32C=({const char*_tmpF9="@thin has wrong number of arguments";_tag_fat(_tmpF9,sizeof(char),36U);});_tmp32D(_tmp32C,_tag_fat(_tmpF8,sizeof(void*),0U));});});cvtenv=({Cyc_Tctyp_i_check_valid_type(loc,te,cvtenv,& Cyc_Tcutil_ik,(void*)((struct Cyc_List_List*)_check_null(ts))->hd,0,1);});
# 661
goto _LL0;case 10U: _LL25: _LL26: {
# 663
int len=({Cyc_List_length(ts);});
int expected=2;
if(len < expected - 1 || len > expected)
({void*_tmpFB=0U;({int(*_tmp32F)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmpFD)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmpFD;});struct _fat_ptr _tmp32E=({const char*_tmpFC="caccess has wrong number of arguments";_tag_fat(_tmpFC,sizeof(char),38U);});_tmp32F(_tmp32E,_tag_fat(_tmpFB,sizeof(void*),0U));});});
# 665
cvtenv=({Cyc_Tctyp_i_check_valid_type(loc,te,cvtenv,& Cyc_Tcutil_xrk,(void*)((struct Cyc_List_List*)_check_null(ts))->hd,1,1);});{
# 668
struct Cyc_List_List*iter=ts->tl;
{int i=2;for(0;i < expected;(i ++,iter=iter->tl)){
cvtenv=({Cyc_Tctyp_i_check_valid_type(loc,te,cvtenv,& Cyc_Tcutil_bk,(void*)((struct Cyc_List_List*)_check_null(iter))->hd,1,1);});}}
if(iter != 0)
cvtenv=({Cyc_Tctyp_i_check_valid_type(loc,te,cvtenv,& Cyc_Tcutil_xrk,(void*)iter->hd,1,1);});
# 671
goto _LL0;}}case 9U: _LL27: _LL28:
# 675
 if(({Cyc_List_length(ts);})!= 1)({void*_tmpFE=0U;({int(*_tmp331)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp100)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp100;});struct _fat_ptr _tmp330=({const char*_tmpFF="access has wrong number of arguments";_tag_fat(_tmpFF,sizeof(char),37U);});_tmp331(_tmp330,_tag_fat(_tmpFE,sizeof(void*),0U));});});cvtenv=({Cyc_Tctyp_i_check_valid_type(loc,te,cvtenv,& Cyc_Tcutil_trk,(void*)((struct Cyc_List_List*)_check_null(ts))->hd,1,1);});
goto _LL0;case 22U: _LL29: _tmpCD=(union Cyc_Absyn_AggrInfo*)&((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)_tmpCA)->f1;_LL2A: {union Cyc_Absyn_AggrInfo*info=_tmpCD;
# 678
cvtenv=({Cyc_Tctyp_i_check_valid_aggr(loc,te,cvtenv,expected_kind,info,targsp,allow_abs_aggr);});
# 680
goto _LL0;}case 20U: _LL2B: _tmpCC=(union Cyc_Absyn_DatatypeInfo*)&((struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*)_tmpCA)->f1;_LL2C: {union Cyc_Absyn_DatatypeInfo*info=_tmpCC;
# 682
cvtenv=({Cyc_Tctyp_i_check_valid_datatype(loc,te,cvtenv,expected_kind,info,targsp,allow_abs_aggr);});
# 684
goto _LL0;}default: _LL2D: _tmpCB=(union Cyc_Absyn_DatatypeFieldInfo*)&((struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)_tmpCA)->f1;_LL2E: {union Cyc_Absyn_DatatypeFieldInfo*info=_tmpCB;
# 686
cvtenv=({Cyc_Tctyp_i_check_valid_datatype_field(loc,te,cvtenv,expected_kind,info,ts,allow_abs_aggr);});
# 688
goto _LL0;}}_LL0:;}
# 690
return cvtenv;}
# 695
static struct Cyc_Tctyp_CVTEnv*Cyc_Tctyp_i_check_valid_type_wrapper(unsigned loc,struct Cyc_Tcenv_Tenv*te,struct Cyc_Tctyp_CVTEnv*cvtenv,struct Cyc_Absyn_Kind*expected_kind,void*t,int put_in_effect,int allow_abs_aggr){
# 701
return({struct Cyc_Tctyp_CVTEnv*_tmp102=_cycalloc(sizeof(*_tmp102));({
struct Cyc_Tctyp_CVTEnv _tmp332=({Cyc_Tctyp_i_check_valid_type(loc,te,*cvtenv,expected_kind,t,put_in_effect,allow_abs_aggr);});*_tmp102=_tmp332;});_tmp102;});}struct _tuple17{enum Cyc_Absyn_Format_Type f1;void*f2;};struct _tuple18{struct Cyc_Absyn_Tqual f1;void*f2;};struct _tuple19{struct Cyc_Absyn_Tvar*f1;void*f2;};
# 707
static struct Cyc_Tctyp_CVTEnv Cyc_Tctyp_i_check_valid_type(unsigned loc,struct Cyc_Tcenv_Tenv*te,struct Cyc_Tctyp_CVTEnv cvtenv,struct Cyc_Absyn_Kind*expected_kind,void*t,int put_in_effect,int allow_abs_aggr){
# 716
void*c=({Cyc_Tcutil_compress(t);});
{void*_tmp104=c;void**_tmp108;struct Cyc_Absyn_Typedefdecl**_tmp107;struct Cyc_List_List**_tmp106;struct _tuple1*_tmp105;struct Cyc_List_List*_tmp10A;enum Cyc_Absyn_AggrKind _tmp109;struct Cyc_List_List*_tmp10B;int _tmp11C;struct Cyc_List_List*_tmp11B;struct Cyc_List_List*_tmp11A;struct Cyc_List_List*_tmp119;struct Cyc_List_List**_tmp118;struct Cyc_Absyn_Exp*_tmp117;struct Cyc_List_List**_tmp116;struct Cyc_Absyn_Exp*_tmp115;struct Cyc_List_List*_tmp114;struct Cyc_List_List*_tmp113;struct Cyc_Absyn_VarargInfo*_tmp112;int _tmp111;struct Cyc_List_List*_tmp110;void*_tmp10F;struct Cyc_Absyn_Tqual*_tmp10E;void**_tmp10D;struct Cyc_List_List**_tmp10C;unsigned _tmp121;void*_tmp120;struct Cyc_Absyn_Exp**_tmp11F;struct Cyc_Absyn_Tqual*_tmp11E;void*_tmp11D;struct Cyc_Absyn_Exp*_tmp122;struct Cyc_Absyn_Exp*_tmp123;void*_tmp129;void*_tmp128;void*_tmp127;void*_tmp126;struct Cyc_Absyn_Tqual*_tmp125;void*_tmp124;void***_tmp12B;void*_tmp12A;struct Cyc_Absyn_Tvar*_tmp12C;void**_tmp12E;struct Cyc_Core_Opt**_tmp12D;struct Cyc_List_List**_tmp130;void*_tmp12F;switch(*((int*)_tmp104)){case 0U: _LL1: _tmp12F=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp104)->f1;_tmp130=(struct Cyc_List_List**)&((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp104)->f2;_LL2: {void*c=_tmp12F;struct Cyc_List_List**targsp=_tmp130;
# 720
cvtenv=({Cyc_Tctyp_i_check_valid_type_app(loc,te,cvtenv,expected_kind,c,targsp,put_in_effect,allow_abs_aggr);});
# 722
goto _LL0;}case 1U: _LL3: _tmp12D=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp104)->f1;_tmp12E=(void**)&((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp104)->f2;_LL4: {struct Cyc_Core_Opt**k=_tmp12D;void**ref=_tmp12E;
# 725
if(*k == 0 ||
({Cyc_Tcutil_kind_leq(expected_kind,(struct Cyc_Absyn_Kind*)((struct Cyc_Core_Opt*)_check_null(*k))->v);})&& !({Cyc_Tcutil_kind_leq((struct Cyc_Absyn_Kind*)((struct Cyc_Core_Opt*)_check_null(*k))->v,expected_kind);}))
({struct Cyc_Core_Opt*_tmp333=({Cyc_Tcutil_kind_to_opt(expected_kind);});*k=_tmp333;});
# 725
if(
# 728
((cvtenv.fn_result && cvtenv.generalize_evars)&&(int)expected_kind->kind == (int)Cyc_Absyn_RgnKind)&& !te->tempest_generalize){
# 731
if((int)expected_kind->aliasqual == (int)Cyc_Absyn_Unique)
*ref=Cyc_Absyn_unique_rgn_type;else{
# 734
*ref=Cyc_Absyn_heap_rgn_type;}}else{
# 736
if((cvtenv.generalize_evars &&(int)expected_kind->kind != (int)Cyc_Absyn_BoolKind)&&(int)expected_kind->kind != (int)Cyc_Absyn_PtrBndKind){
# 740
struct Cyc_Absyn_Tvar*v=({Cyc_Tcutil_new_tvar((void*)({struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*_tmp131=_cycalloc(sizeof(*_tmp131));_tmp131->tag=2U,_tmp131->f1=0,_tmp131->f2=expected_kind;_tmp131;}));});
({void*_tmp334=({Cyc_Absyn_var_type(v);});*ref=_tmp334;});
_tmp12C=v;goto _LL6;}else{
# 744
({struct Cyc_List_List*_tmp335=({Cyc_Tctyp_add_free_evar(cvtenv.r,cvtenv.free_evars,t,put_in_effect);});cvtenv.free_evars=_tmp335;});}}
goto _LL0;}case 2U: _LL5: _tmp12C=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp104)->f1;_LL6: {struct Cyc_Absyn_Tvar*v=_tmp12C;
# 748
{void*_stmttmp12=({Cyc_Absyn_compress_kb(v->kind);});void*_tmp132=_stmttmp12;struct Cyc_Core_Opt**_tmp133;if(((struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*)_tmp132)->tag == 1U){_LL1A: _tmp133=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*)_tmp132)->f1;_LL1B: {struct Cyc_Core_Opt**f=_tmp133;
# 752
{struct Cyc_Absyn_Kind*_tmp134=expected_kind;enum Cyc_Absyn_AliasQual _tmp135;if(((struct Cyc_Absyn_Kind*)_tmp134)->kind == Cyc_Absyn_RgnKind){_LL1F: _tmp135=_tmp134->aliasqual;_LL20: {enum Cyc_Absyn_AliasQual Alisable=_tmp135;
# 755
{struct Cyc_List_List*tvs2=cvtenv.kind_env;for(0;tvs2 != 0;tvs2=tvs2->tl){
if(({Cyc_strptrcmp(((struct Cyc_Absyn_Tvar*)tvs2->hd)->name,v->name);})== 0){
void*_stmttmp13=((struct Cyc_Absyn_Tvar*)tvs2->hd)->kind;void*_tmp136=_stmttmp13;if(((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp136)->tag == 0U){if(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp136)->f1)->kind == Cyc_Absyn_XRgnKind){_LL24: _LL25:
# 760
 expected_kind=& Cyc_Tcutil_xrk;
goto DONE;}else{goto _LL26;}}else{_LL26: _LL27:
 goto _LL23;}_LL23:;}}}
# 755
goto _LL1E;}}else{_LL21: _LL22:
# 765
 goto _LL1E;}_LL1E:;}
# 767
DONE:({struct Cyc_Core_Opt*_tmp337=({struct Cyc_Core_Opt*_tmp138=_cycalloc(sizeof(*_tmp138));({void*_tmp336=(void*)({struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*_tmp137=_cycalloc(sizeof(*_tmp137));_tmp137->tag=2U,_tmp137->f1=0,_tmp137->f2=expected_kind;_tmp137;});_tmp138->v=_tmp336;});_tmp138;});*f=_tmp337;});goto _LL19;}}else{_LL1C: _LL1D:
 goto _LL19;}_LL19:;}
# 772
({struct Cyc_List_List*_tmp338=({Cyc_Tctyp_add_free_tvar(loc,cvtenv.kind_env,v);});cvtenv.kind_env=_tmp338;});
# 775
({struct Cyc_List_List*_tmp339=({Cyc_Tctyp_fast_add_free_tvar_bool(cvtenv.r,cvtenv.free_vars,v,put_in_effect);});cvtenv.free_vars=_tmp339;});
# 778
{void*_stmttmp14=({Cyc_Absyn_compress_kb(v->kind);});void*_tmp139=_stmttmp14;struct Cyc_Absyn_Kind*_tmp13B;struct Cyc_Core_Opt**_tmp13A;if(((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp139)->tag == 2U){_LL29: _tmp13A=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp139)->f1;_tmp13B=((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp139)->f2;_LL2A: {struct Cyc_Core_Opt**f=_tmp13A;struct Cyc_Absyn_Kind*k=_tmp13B;
# 781
if(({Cyc_Tcutil_kind_leq(expected_kind,k);}))
({struct Cyc_Core_Opt*_tmp33B=({struct Cyc_Core_Opt*_tmp13D=_cycalloc(sizeof(*_tmp13D));({void*_tmp33A=(void*)({struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*_tmp13C=_cycalloc(sizeof(*_tmp13C));_tmp13C->tag=2U,_tmp13C->f1=0,_tmp13C->f2=expected_kind;_tmp13C;});_tmp13D->v=_tmp33A;});_tmp13D;});*f=_tmp33B;});
# 781
goto _LL28;}}else{_LL2B: _LL2C:
# 784
 goto _LL28;}_LL28:;}
# 786
goto _LL0;}case 10U: _LL7: _tmp12A=(((struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_tmp104)->f1)->r;_tmp12B=(void***)&((struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_tmp104)->f2;_LL8: {void*td=_tmp12A;void***topt=_tmp12B;
# 792
void*new_typ=({void*(*_tmp143)(void*)=Cyc_Tcutil_copy_type;void*_tmp144=({Cyc_Tcutil_compress(t);});_tmp143(_tmp144);});
{void*_tmp13E=td;struct Cyc_Absyn_Datatypedecl*_tmp13F;struct Cyc_Absyn_Enumdecl*_tmp140;struct Cyc_Absyn_Aggrdecl*_tmp141;switch(*((int*)_tmp13E)){case 0U: _LL2E: _tmp141=((struct Cyc_Absyn_Aggr_td_Absyn_Raw_typedecl_struct*)_tmp13E)->f1;_LL2F: {struct Cyc_Absyn_Aggrdecl*ad=_tmp141;
# 795
if(te->in_extern_c_include)
ad->sc=Cyc_Absyn_ExternC;
# 795
({Cyc_Tc_tcAggrdecl(te,loc,ad);});
# 797
goto _LL2D;}case 1U: _LL30: _tmp140=((struct Cyc_Absyn_Enum_td_Absyn_Raw_typedecl_struct*)_tmp13E)->f1;_LL31: {struct Cyc_Absyn_Enumdecl*ed=_tmp140;
# 799
if(te->in_extern_c_include)
ed->sc=Cyc_Absyn_ExternC;
# 799
({Cyc_Tc_tcEnumdecl(te,loc,ed);});
# 801
goto _LL2D;}default: _LL32: _tmp13F=((struct Cyc_Absyn_Datatype_td_Absyn_Raw_typedecl_struct*)_tmp13E)->f1;_LL33: {struct Cyc_Absyn_Datatypedecl*dd=_tmp13F;
# 803
({Cyc_Tc_tcDatatypedecl(te,loc,dd);});goto _LL2D;}}_LL2D:;}
# 805
({void**_tmp33C=({void**_tmp142=_cycalloc(sizeof(*_tmp142));*_tmp142=new_typ;_tmp142;});*topt=_tmp33C;});
return({Cyc_Tctyp_i_check_valid_type(loc,te,cvtenv,expected_kind,new_typ,put_in_effect,allow_abs_aggr);});}case 3U: _LL9: _tmp124=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp104)->f1).elt_type;_tmp125=(struct Cyc_Absyn_Tqual*)&(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp104)->f1).elt_tq;_tmp126=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp104)->f1).ptr_atts).rgn;_tmp127=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp104)->f1).ptr_atts).nullable;_tmp128=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp104)->f1).ptr_atts).bounds;_tmp129=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp104)->f1).ptr_atts).zero_term;_LLA: {void*t1=_tmp124;struct Cyc_Absyn_Tqual*tqp=_tmp125;void*rgn_type=_tmp126;void*nullable=_tmp127;void*b=_tmp128;void*zt=_tmp129;
# 811
int is_zero_terminated;
# 813
cvtenv=({Cyc_Tctyp_i_check_valid_type(loc,te,cvtenv,& Cyc_Tcutil_tak,t1,1,1);});
({int _tmp33D=({Cyc_Tcutil_extract_const_from_typedef(loc,tqp->print_const,t1);});tqp->real_const=_tmp33D;});
{
struct Cyc_Absyn_Kind*k;
{enum Cyc_Absyn_AliasQual _stmttmp15=expected_kind->aliasqual;enum Cyc_Absyn_AliasQual _tmp145=_stmttmp15;switch(_tmp145){case Cyc_Absyn_Aliasable: _LL35: _LL36:
# 819
 if(!({Cyc_Tcutil_is_pointer_effect(t);})){
# 821
if(({Cyc_Absyn_is_xrgn(rgn_type);}))k=& Cyc_Tcutil_xrk;else{
k=& Cyc_Tcutil_rk;}}else{
# 825
k=& Cyc_Tcutil_ek;}
goto _LL34;case Cyc_Absyn_Unique: _LL37: _LL38:
 k=& Cyc_Tcutil_urk;goto _LL34;case Cyc_Absyn_Top: _LL39: _LL3A:
 goto _LL3C;default: _LL3B: _LL3C:
# 830
 if(!({Cyc_Tcutil_is_pointer_effect(t);})){
# 832
if(({Cyc_Absyn_is_xrgn(rgn_type);}))k=& Cyc_Tcutil_xrk;else{
k=& Cyc_Tcutil_trk;}}else{
# 836
k=& Cyc_Tcutil_ek;}}_LL34:;}
# 838
cvtenv=({Cyc_Tctyp_i_check_valid_type(loc,te,cvtenv,k,rgn_type,1,1);});}
# 843
cvtenv=({Cyc_Tctyp_i_check_valid_type(loc,te,cvtenv,& Cyc_Tcutil_boolk,zt,0,1);});
cvtenv=({Cyc_Tctyp_i_check_valid_type(loc,te,cvtenv,& Cyc_Tcutil_boolk,nullable,0,1);});
({int(*_tmp146)(void*,void*)=Cyc_Unify_unify;void*_tmp147=zt;void*_tmp148=({Cyc_Tcutil_is_char_type(t1);})?Cyc_Absyn_true_type: Cyc_Absyn_false_type;_tmp146(_tmp147,_tmp148);});
is_zero_terminated=({Cyc_Absyn_type2bool(0,zt);});
if(is_zero_terminated){
# 849
if(!({Cyc_Tcutil_admits_zero(t1);}))
({struct Cyc_String_pa_PrintArg_struct _tmp14B=({struct Cyc_String_pa_PrintArg_struct _tmp29A;_tmp29A.tag=0U,({
struct _fat_ptr _tmp33E=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t1);}));_tmp29A.f1=_tmp33E;});_tmp29A;});void*_tmp149[1U];_tmp149[0]=& _tmp14B;({unsigned _tmp340=loc;struct _fat_ptr _tmp33F=({const char*_tmp14A="cannot have a pointer to zero-terminated %s elements";_tag_fat(_tmp14A,sizeof(char),53U);});Cyc_Tcutil_terr(_tmp340,_tmp33F,_tag_fat(_tmp149,sizeof(void*),1U));});});}
# 847
cvtenv=({Cyc_Tctyp_i_check_valid_type(loc,te,cvtenv,& Cyc_Tcutil_ptrbk,b,0,allow_abs_aggr);});{
# 855
struct Cyc_Absyn_Exp*e=({struct Cyc_Absyn_Exp*(*_tmp150)(void*def,void*b)=Cyc_Tcutil_get_bounds_exp;void*_tmp151=({Cyc_Absyn_bounds_one();});void*_tmp152=b;_tmp150(_tmp151,_tmp152);});
if(e != 0){
struct _tuple13 _stmttmp16=({Cyc_Evexp_eval_const_uint_exp(e);});int _tmp14D;unsigned _tmp14C;_LL3E: _tmp14C=_stmttmp16.f1;_tmp14D=_stmttmp16.f2;_LL3F: {unsigned sz=_tmp14C;int known=_tmp14D;
if(is_zero_terminated &&(!known || sz < (unsigned)1))
({void*_tmp14E=0U;({unsigned _tmp342=loc;struct _fat_ptr _tmp341=({const char*_tmp14F="zero-terminated pointer cannot point to empty sequence";_tag_fat(_tmp14F,sizeof(char),55U);});Cyc_Tcutil_terr(_tmp342,_tmp341,_tag_fat(_tmp14E,sizeof(void*),0U));});});}}
# 856
goto _LL0;}}case 9U: _LLB: _tmp123=((struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_tmp104)->f1;_LLC: {struct Cyc_Absyn_Exp*e=_tmp123;
# 866
({void*(*_tmp153)(struct Cyc_Tcenv_Tenv*,void**,struct Cyc_Absyn_Exp*)=Cyc_Tcexp_tcExp;struct Cyc_Tcenv_Tenv*_tmp154=({Cyc_Tcenv_allow_valueof(te);});void**_tmp155=0;struct Cyc_Absyn_Exp*_tmp156=e;_tmp153(_tmp154,_tmp155,_tmp156);});
if(!({Cyc_Tcutil_coerce_uint_type(e);}))
({void*_tmp157=0U;({unsigned _tmp344=loc;struct _fat_ptr _tmp343=({const char*_tmp158="valueof_t requires an int expression";_tag_fat(_tmp158,sizeof(char),37U);});Cyc_Tcutil_terr(_tmp344,_tmp343,_tag_fat(_tmp157,sizeof(void*),0U));});});
# 867
cvtenv=({Cyc_Tctyp_i_check_valid_type_level_exp(e,te,cvtenv);});
# 870
goto _LL0;}case 11U: _LLD: _tmp122=((struct Cyc_Absyn_TypeofType_Absyn_Type_struct*)_tmp104)->f1;_LLE: {struct Cyc_Absyn_Exp*e=_tmp122;
# 875
({void*(*_tmp159)(struct Cyc_Tcenv_Tenv*,void**,struct Cyc_Absyn_Exp*)=Cyc_Tcexp_tcExp;struct Cyc_Tcenv_Tenv*_tmp15A=({Cyc_Tcenv_allow_valueof(te);});void**_tmp15B=0;struct Cyc_Absyn_Exp*_tmp15C=e;_tmp159(_tmp15A,_tmp15B,_tmp15C);});
goto _LL0;}case 4U: _LLF: _tmp11D=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp104)->f1).elt_type;_tmp11E=(struct Cyc_Absyn_Tqual*)&(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp104)->f1).tq;_tmp11F=(struct Cyc_Absyn_Exp**)&(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp104)->f1).num_elts;_tmp120=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp104)->f1).zero_term;_tmp121=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp104)->f1).zt_loc;_LL10: {void*t1=_tmp11D;struct Cyc_Absyn_Tqual*tqp=_tmp11E;struct Cyc_Absyn_Exp**eptr=_tmp11F;void*zt=_tmp120;unsigned ztl=_tmp121;
# 880
struct Cyc_Absyn_Exp*e=*eptr;
cvtenv=({Cyc_Tctyp_i_check_valid_type(loc,te,cvtenv,& Cyc_Tcutil_tmk,t1,1,allow_abs_aggr);});
({int _tmp345=({Cyc_Tcutil_extract_const_from_typedef(loc,tqp->print_const,t1);});tqp->real_const=_tmp345;});{
# 884
int is_zero_terminated=({Cyc_Tcutil_force_type2bool(0,zt);});
if(is_zero_terminated){
# 887
if(!({Cyc_Tcutil_admits_zero(t1);}))
({struct Cyc_String_pa_PrintArg_struct _tmp15F=({struct Cyc_String_pa_PrintArg_struct _tmp29B;_tmp29B.tag=0U,({
struct _fat_ptr _tmp346=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t1);}));_tmp29B.f1=_tmp346;});_tmp29B;});void*_tmp15D[1U];_tmp15D[0]=& _tmp15F;({unsigned _tmp348=loc;struct _fat_ptr _tmp347=({const char*_tmp15E="cannot have a zero-terminated array of %s elements";_tag_fat(_tmp15E,sizeof(char),51U);});Cyc_Tcutil_terr(_tmp348,_tmp347,_tag_fat(_tmp15D,sizeof(void*),1U));});});}
# 885
if(e == 0){
# 895
if(is_zero_terminated)
# 897
({struct Cyc_Absyn_Exp*_tmp349=e=({Cyc_Absyn_uint_exp(1U,0U);});*eptr=_tmp349;});else{
# 900
({void*_tmp160=0U;({unsigned _tmp34B=loc;struct _fat_ptr _tmp34A=({const char*_tmp161="array bound defaults to 1 here";_tag_fat(_tmp161,sizeof(char),31U);});Cyc_Tcutil_warn(_tmp34B,_tmp34A,_tag_fat(_tmp160,sizeof(void*),0U));});});
({struct Cyc_Absyn_Exp*_tmp34C=e=({Cyc_Absyn_uint_exp(1U,0U);});*eptr=_tmp34C;});}}
# 885
({void*(*_tmp162)(struct Cyc_Tcenv_Tenv*,void**,struct Cyc_Absyn_Exp*)=Cyc_Tcexp_tcExp;struct Cyc_Tcenv_Tenv*_tmp163=({Cyc_Tcenv_allow_valueof(te);});void**_tmp164=0;struct Cyc_Absyn_Exp*_tmp165=e;_tmp162(_tmp163,_tmp164,_tmp165);});
# 905
if(!({Cyc_Tcutil_coerce_uint_type(e);}))
({void*_tmp166=0U;({unsigned _tmp34E=loc;struct _fat_ptr _tmp34D=({const char*_tmp167="array bounds expression is not an unsigned int";_tag_fat(_tmp167,sizeof(char),47U);});Cyc_Tcutil_terr(_tmp34E,_tmp34D,_tag_fat(_tmp166,sizeof(void*),0U));});});
# 905
cvtenv=({Cyc_Tctyp_i_check_valid_type_level_exp(e,te,cvtenv);});{
# 912
struct _tuple13 _stmttmp17=({Cyc_Evexp_eval_const_uint_exp(e);});int _tmp169;unsigned _tmp168;_LL41: _tmp168=_stmttmp17.f1;_tmp169=_stmttmp17.f2;_LL42: {unsigned sz=_tmp168;int known=_tmp169;
# 914
if((is_zero_terminated && known)&& sz < (unsigned)1)
({void*_tmp16A=0U;({unsigned _tmp350=loc;struct _fat_ptr _tmp34F=({const char*_tmp16B="zero terminated array cannot have zero elements";_tag_fat(_tmp16B,sizeof(char),48U);});Cyc_Tcutil_warn(_tmp350,_tmp34F,_tag_fat(_tmp16A,sizeof(void*),0U));});});
# 914
if(
# 917
(known && sz < (unsigned)1)&& Cyc_Cyclone_tovc_r){
({void*_tmp16C=0U;({unsigned _tmp352=loc;struct _fat_ptr _tmp351=({const char*_tmp16D="arrays with 0 elements are not supported except with gcc -- changing to 1.";_tag_fat(_tmp16D,sizeof(char),75U);});Cyc_Tcutil_warn(_tmp352,_tmp351,_tag_fat(_tmp16C,sizeof(void*),0U));});});
({struct Cyc_Absyn_Exp*_tmp353=({Cyc_Absyn_uint_exp(1U,0U);});*eptr=_tmp353;});}
# 914
goto _LL0;}}}}case 5U: _LL11: _tmp10C=(struct Cyc_List_List**)&(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp104)->f1).tvars;_tmp10D=(void**)&(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp104)->f1).effect;_tmp10E=(struct Cyc_Absyn_Tqual*)&(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp104)->f1).ret_tqual;_tmp10F=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp104)->f1).ret_type;_tmp110=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp104)->f1).args;_tmp111=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp104)->f1).c_varargs;_tmp112=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp104)->f1).cyc_varargs;_tmp113=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp104)->f1).rgn_po;_tmp114=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp104)->f1).attributes;_tmp115=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp104)->f1).requires_clause;_tmp116=(struct Cyc_List_List**)&(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp104)->f1).requires_relns;_tmp117=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp104)->f1).ensures_clause;_tmp118=(struct Cyc_List_List**)&(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp104)->f1).ensures_relns;_tmp119=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp104)->f1).ieffect;_tmp11A=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp104)->f1).oeffect;_tmp11B=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp104)->f1).throws;_tmp11C=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp104)->f1).reentrant;_LL12: {struct Cyc_List_List**btvs=_tmp10C;void**eff=_tmp10D;struct Cyc_Absyn_Tqual*rtq=_tmp10E;void*tr=_tmp10F;struct Cyc_List_List*args=_tmp110;int c_vararg=_tmp111;struct Cyc_Absyn_VarargInfo*cyc_vararg=_tmp112;struct Cyc_List_List*rgn_po=_tmp113;struct Cyc_List_List*atts=_tmp114;struct Cyc_Absyn_Exp*req=_tmp115;struct Cyc_List_List**req_relns=_tmp116;struct Cyc_Absyn_Exp*ens=_tmp117;struct Cyc_List_List**ens_relns=_tmp118;struct Cyc_List_List*ieff=_tmp119;struct Cyc_List_List*oeff=_tmp11A;struct Cyc_List_List*throws=_tmp11B;int reentrant=_tmp11C;
# 930
int num_convs=0;
int seen_cdecl=0;
int seen_stdcall=0;
int seen_fastcall=0;
int seen_format=0;
enum Cyc_Absyn_Format_Type ft=0U;
int fmt_desc_arg=-1;
int fmt_arg_start=-1;
for(0;atts != 0;atts=atts->tl){
# 940
if(!({Cyc_Absyn_fntype_att((void*)atts->hd);}))
({struct Cyc_String_pa_PrintArg_struct _tmp170=({struct Cyc_String_pa_PrintArg_struct _tmp29C;_tmp29C.tag=0U,({struct _fat_ptr _tmp354=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_attribute2string((void*)atts->hd);}));_tmp29C.f1=_tmp354;});_tmp29C;});void*_tmp16E[1U];_tmp16E[0]=& _tmp170;({unsigned _tmp356=loc;struct _fat_ptr _tmp355=({const char*_tmp16F="bad function type attribute %s";_tag_fat(_tmp16F,sizeof(char),31U);});Cyc_Tcutil_terr(_tmp356,_tmp355,_tag_fat(_tmp16E,sizeof(void*),1U));});});{
# 940
void*_stmttmp18=(void*)atts->hd;void*_tmp171=_stmttmp18;int _tmp174;int _tmp173;enum Cyc_Absyn_Format_Type _tmp172;switch(*((int*)_tmp171)){case 1U: _LL44: _LL45:
# 945
 if(!seen_stdcall){seen_stdcall=1;++ num_convs;}goto _LL43;case 2U: _LL46: _LL47:
# 947
 if(!seen_cdecl){seen_cdecl=1;++ num_convs;}goto _LL43;case 3U: _LL48: _LL49:
# 949
 if(!seen_fastcall){seen_fastcall=1;++ num_convs;}goto _LL43;case 19U: _LL4A: _tmp172=((struct Cyc_Absyn_Format_att_Absyn_Attribute_struct*)_tmp171)->f1;_tmp173=((struct Cyc_Absyn_Format_att_Absyn_Attribute_struct*)_tmp171)->f2;_tmp174=((struct Cyc_Absyn_Format_att_Absyn_Attribute_struct*)_tmp171)->f3;_LL4B: {enum Cyc_Absyn_Format_Type fmttype=_tmp172;int i=_tmp173;int j=_tmp174;
# 951
if(!seen_format){
# 953
seen_format=1;
ft=fmttype;
fmt_desc_arg=i;
fmt_arg_start=j;}else{
({void*_tmp175=0U;({unsigned _tmp358=loc;struct _fat_ptr _tmp357=({const char*_tmp176="function can't have multiple format attributes";_tag_fat(_tmp176,sizeof(char),47U);});Cyc_Tcutil_terr(_tmp358,_tmp357,_tag_fat(_tmp175,sizeof(void*),0U));});});}
goto _LL43;}default: _LL4C: _LL4D:
 goto _LL43;}_LL43:;}}
# 962
if(num_convs > 1)({void*_tmp177=0U;({unsigned _tmp35A=loc;struct _fat_ptr _tmp359=({const char*_tmp178="function can't have multiple calling conventions";_tag_fat(_tmp178,sizeof(char),49U);});Cyc_Tcutil_terr(_tmp35A,_tmp359,_tag_fat(_tmp177,sizeof(void*),0U));});});({Cyc_Tcutil_check_unique_tvars(loc,*btvs);});
# 967
{struct Cyc_List_List*b=*btvs;for(0;b != 0;b=b->tl){
# 969
({int _tmp35B=({Cyc_Tcutil_new_tvar_id();});((struct Cyc_Absyn_Tvar*)b->hd)->identity=_tmp35B;});
({struct Cyc_List_List*_tmp35C=({Cyc_Tctyp_add_bound_tvar(cvtenv.kind_env,(struct Cyc_Absyn_Tvar*)b->hd);});cvtenv.kind_env=_tmp35C;});{
void*_stmttmp19=({Cyc_Absyn_compress_kb(((struct Cyc_Absyn_Tvar*)b->hd)->kind);});void*_tmp179=_stmttmp19;if(((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp179)->tag == 0U){if(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp179)->f1)->kind == Cyc_Absyn_MemKind){_LL4F: _LL50:
# 974
({struct Cyc_String_pa_PrintArg_struct _tmp17C=({struct Cyc_String_pa_PrintArg_struct _tmp29D;_tmp29D.tag=0U,_tmp29D.f1=(struct _fat_ptr)((struct _fat_ptr)*((struct Cyc_Absyn_Tvar*)b->hd)->name);_tmp29D;});void*_tmp17A[1U];_tmp17A[0]=& _tmp17C;({unsigned _tmp35E=loc;struct _fat_ptr _tmp35D=({const char*_tmp17B="function attempts to abstract Mem type variable %s";_tag_fat(_tmp17B,sizeof(char),51U);});Cyc_Tcutil_terr(_tmp35E,_tmp35D,_tag_fat(_tmp17A,sizeof(void*),1U));});});goto _LL4E;}else{goto _LL51;}}else{_LL51: _LL52:
 goto _LL4E;}_LL4E:;}}}{
# 981
struct Cyc_Tctyp_CVTEnv new_cvtenv=({struct Cyc_Tctyp_CVTEnv _tmp2A2;_tmp2A2.r=Cyc_Core_heap_region,_tmp2A2.kind_env=cvtenv.kind_env,_tmp2A2.fn_result=1,_tmp2A2.generalize_evars=cvtenv.generalize_evars,_tmp2A2.free_vars=0,_tmp2A2.free_evars=0;_tmp2A2;});
# 983
new_cvtenv=*({({struct Cyc_Tctyp_CVTEnv*(*_tmp363)(unsigned loc,struct Cyc_Tctyp_CVTEnv*(*cb)(unsigned,struct Cyc_Tcenv_Tenv*,struct Cyc_Tctyp_CVTEnv*,struct Cyc_Absyn_Kind*,void*,int,int),struct Cyc_Tcenv_Tenv*te,struct Cyc_Tctyp_CVTEnv*new_cvtenv,struct Cyc_List_List*ieff,struct Cyc_List_List*oeff)=({
struct Cyc_Tctyp_CVTEnv*(*_tmp17D)(unsigned loc,struct Cyc_Tctyp_CVTEnv*(*cb)(unsigned,struct Cyc_Tcenv_Tenv*,struct Cyc_Tctyp_CVTEnv*,struct Cyc_Absyn_Kind*,void*,int,int),struct Cyc_Tcenv_Tenv*te,struct Cyc_Tctyp_CVTEnv*new_cvtenv,struct Cyc_List_List*ieff,struct Cyc_List_List*oeff)=(struct Cyc_Tctyp_CVTEnv*(*)(unsigned loc,struct Cyc_Tctyp_CVTEnv*(*cb)(unsigned,struct Cyc_Tcenv_Tenv*,struct Cyc_Tctyp_CVTEnv*,struct Cyc_Absyn_Kind*,void*,int,int),struct Cyc_Tcenv_Tenv*te,struct Cyc_Tctyp_CVTEnv*new_cvtenv,struct Cyc_List_List*ieff,struct Cyc_List_List*oeff))Cyc_Absyn_i_check_valid_effect;_tmp17D;});unsigned _tmp362=loc;struct Cyc_Tcenv_Tenv*_tmp361=te;struct Cyc_Tctyp_CVTEnv*_tmp360=({struct Cyc_Tctyp_CVTEnv*_tmp17E=_cycalloc(sizeof(*_tmp17E));*_tmp17E=new_cvtenv;_tmp17E;});struct Cyc_List_List*_tmp35F=ieff;_tmp363(_tmp362,Cyc_Tctyp_i_check_valid_type_wrapper,_tmp361,_tmp360,_tmp35F,oeff);});});
# 986
new_cvtenv=({Cyc_Tctyp_i_check_valid_type(loc,te,new_cvtenv,& Cyc_Tcutil_tmk,tr,1,1);});
# 988
({int _tmp364=({Cyc_Tcutil_extract_const_from_typedef(loc,rtq->print_const,tr);});rtq->real_const=_tmp364;});
new_cvtenv.fn_result=0;
# 992
{struct Cyc_List_List*a=args;for(0;a != 0;a=a->tl){
struct _tuple10*trip=(struct _tuple10*)a->hd;
void*t=(*trip).f3;
new_cvtenv=({Cyc_Tctyp_i_check_valid_type(loc,te,new_cvtenv,& Cyc_Tcutil_tmk,t,1,1);});{
int ec=({Cyc_Tcutil_extract_const_from_typedef(loc,((*trip).f2).print_const,t);});
((*trip).f2).real_const=ec;
# 1000
if(({Cyc_Tcutil_is_array_type(t);})){
# 1002
void*ptr_rgn=({Cyc_Absyn_new_evar(0,0);});
# 1004
new_cvtenv=({Cyc_Tctyp_i_check_valid_type(loc,te,new_cvtenv,& Cyc_Tcutil_rk,ptr_rgn,1,1);});
({void*_tmp365=({Cyc_Tcutil_promote_array(t,ptr_rgn,0);});(*trip).f3=_tmp365;});}}}}
# 1010
if(cyc_vararg != 0){
if(c_vararg)({void*_tmp17F=0U;({int(*_tmp367)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp181)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp181;});struct _fat_ptr _tmp366=({const char*_tmp180="both c_vararg and cyc_vararg";_tag_fat(_tmp180,sizeof(char),29U);});_tmp367(_tmp366,_tag_fat(_tmp17F,sizeof(void*),0U));});});{struct Cyc_Absyn_VarargInfo _stmttmp1A=*cyc_vararg;int _tmp183;void*_tmp182;_LL54: _tmp182=_stmttmp1A.type;_tmp183=_stmttmp1A.inject;_LL55: {void*vt=_tmp182;int vi=_tmp183;
# 1013
new_cvtenv=({Cyc_Tctyp_i_check_valid_type(loc,te,new_cvtenv,& Cyc_Tcutil_tmk,vt,1,1);});
({int _tmp368=({Cyc_Tcutil_extract_const_from_typedef(loc,(cyc_vararg->tq).print_const,vt);});(cyc_vararg->tq).real_const=_tmp368;});
# 1016
if(vi){
void*_stmttmp1B=({Cyc_Tcutil_compress(vt);});void*_tmp184=_stmttmp1B;void*_tmp187;void*_tmp186;void*_tmp185;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp184)->tag == 3U){_LL57: _tmp185=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp184)->f1).elt_type;_tmp186=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp184)->f1).ptr_atts).bounds;_tmp187=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp184)->f1).ptr_atts).zero_term;_LL58: {void*et=_tmp185;void*bs=_tmp186;void*zt=_tmp187;
# 1019
{void*_stmttmp1C=({Cyc_Tcutil_compress(et);});void*_tmp188=_stmttmp1C;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp188)->tag == 0U){if(((struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp188)->f1)->tag == 20U){_LL5C: _LL5D:
# 1021
 if(({Cyc_Tcutil_force_type2bool(0,zt);}))
({void*_tmp189=0U;({unsigned _tmp36A=loc;struct _fat_ptr _tmp369=({const char*_tmp18A="can't inject into a zeroterm pointer";_tag_fat(_tmp18A,sizeof(char),37U);});Cyc_Tcutil_terr(_tmp36A,_tmp369,_tag_fat(_tmp189,sizeof(void*),0U));});});
# 1021
if(!({int(*_tmp18B)(void*,void*)=Cyc_Unify_unify;void*_tmp18C=({Cyc_Absyn_bounds_one();});void*_tmp18D=bs;_tmp18B(_tmp18C,_tmp18D);}))
# 1024
({void*_tmp18E=0U;({unsigned _tmp36C=loc;struct _fat_ptr _tmp36B=({const char*_tmp18F="can't inject into a fat pointer to datatype";_tag_fat(_tmp18F,sizeof(char),44U);});Cyc_Tcutil_terr(_tmp36C,_tmp36B,_tag_fat(_tmp18E,sizeof(void*),0U));});});
# 1021
goto _LL5B;}else{goto _LL5E;}}else{_LL5E: _LL5F:
# 1026
({void*_tmp190=0U;({unsigned _tmp36E=loc;struct _fat_ptr _tmp36D=({const char*_tmp191="can't inject a non-datatype type";_tag_fat(_tmp191,sizeof(char),33U);});Cyc_Tcutil_terr(_tmp36E,_tmp36D,_tag_fat(_tmp190,sizeof(void*),0U));});});goto _LL5B;}_LL5B:;}
# 1028
goto _LL56;}}else{_LL59: _LL5A:
({void*_tmp192=0U;({unsigned _tmp370=loc;struct _fat_ptr _tmp36F=({const char*_tmp193="expecting a datatype pointer type";_tag_fat(_tmp193,sizeof(char),34U);});Cyc_Tcutil_terr(_tmp370,_tmp36F,_tag_fat(_tmp192,sizeof(void*),0U));});});goto _LL56;}_LL56:;}}}}
# 1010
if(seen_format){
# 1035
int num_args=({Cyc_List_length(args);});
if((((fmt_desc_arg < 0 || fmt_desc_arg > num_args)|| fmt_arg_start < 0)||
# 1038
(cyc_vararg == 0 && !c_vararg)&& fmt_arg_start != 0)||
(cyc_vararg != 0 || c_vararg)&& fmt_arg_start != num_args + 1)
# 1041
({void*_tmp194=0U;({unsigned _tmp372=loc;struct _fat_ptr _tmp371=({const char*_tmp195="bad format descriptor";_tag_fat(_tmp195,sizeof(char),22U);});Cyc_Tcutil_terr(_tmp372,_tmp371,_tag_fat(_tmp194,sizeof(void*),0U));});});else{
# 1044
struct _tuple10 _stmttmp1D=*({({struct _tuple10*(*_tmp374)(struct Cyc_List_List*x,int n)=({struct _tuple10*(*_tmp1B3)(struct Cyc_List_List*x,int n)=(struct _tuple10*(*)(struct Cyc_List_List*x,int n))Cyc_List_nth;_tmp1B3;});struct Cyc_List_List*_tmp373=args;_tmp374(_tmp373,fmt_desc_arg - 1);});});void*_tmp196;_LL61: _tmp196=_stmttmp1D.f3;_LL62: {void*t=_tmp196;
# 1046
{void*_stmttmp1E=({Cyc_Tcutil_compress(t);});void*_tmp197=_stmttmp1E;void*_tmp19A;void*_tmp199;void*_tmp198;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp197)->tag == 3U){_LL64: _tmp198=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp197)->f1).elt_type;_tmp199=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp197)->f1).ptr_atts).bounds;_tmp19A=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp197)->f1).ptr_atts).zero_term;_LL65: {void*et=_tmp198;void*b=_tmp199;void*zt=_tmp19A;
# 1049
if(!({Cyc_Tcutil_is_char_type(et);}))
({void*_tmp19B=0U;({unsigned _tmp376=loc;struct _fat_ptr _tmp375=({const char*_tmp19C="format descriptor is not a string";_tag_fat(_tmp19C,sizeof(char),34U);});Cyc_Tcutil_terr(_tmp376,_tmp375,_tag_fat(_tmp19B,sizeof(void*),0U));});});else{
# 1052
struct Cyc_Absyn_Exp*e=({struct Cyc_Absyn_Exp*(*_tmp1A1)(void*def,void*b)=Cyc_Tcutil_get_bounds_exp;void*_tmp1A2=({Cyc_Absyn_bounds_one();});void*_tmp1A3=b;_tmp1A1(_tmp1A2,_tmp1A3);});
if(e == 0 && c_vararg)
({void*_tmp19D=0U;({unsigned _tmp378=loc;struct _fat_ptr _tmp377=({const char*_tmp19E="format descriptor is not a char * type";_tag_fat(_tmp19E,sizeof(char),39U);});Cyc_Tcutil_terr(_tmp378,_tmp377,_tag_fat(_tmp19D,sizeof(void*),0U));});});else{
if(e != 0 && !c_vararg)
({void*_tmp19F=0U;({unsigned _tmp37A=loc;struct _fat_ptr _tmp379=({const char*_tmp1A0="format descriptor is not a char ? type";_tag_fat(_tmp1A0,sizeof(char),39U);});Cyc_Tcutil_terr(_tmp37A,_tmp379,_tag_fat(_tmp19F,sizeof(void*),0U));});});}}
# 1058
goto _LL63;}}else{_LL66: _LL67:
({void*_tmp1A4=0U;({unsigned _tmp37C=loc;struct _fat_ptr _tmp37B=({const char*_tmp1A5="format descriptor is not a string type";_tag_fat(_tmp1A5,sizeof(char),39U);});Cyc_Tcutil_terr(_tmp37C,_tmp37B,_tag_fat(_tmp1A4,sizeof(void*),0U));});});goto _LL63;}_LL63:;}
# 1061
if(fmt_arg_start != 0 && !c_vararg){
# 1065
int problem;
{struct _tuple17 _stmttmp1F=({struct _tuple17 _tmp29E;_tmp29E.f1=ft,({void*_tmp37D=({void*(*_tmp1AF)(void*)=Cyc_Tcutil_compress;void*_tmp1B0=({Cyc_Tcutil_pointer_elt_type(((struct Cyc_Absyn_VarargInfo*)_check_null(cyc_vararg))->type);});_tmp1AF(_tmp1B0);});_tmp29E.f2=_tmp37D;});_tmp29E;});struct _tuple17 _tmp1A6=_stmttmp1F;struct Cyc_Absyn_Datatypedecl*_tmp1A7;struct Cyc_Absyn_Datatypedecl*_tmp1A8;switch(_tmp1A6.f1){case Cyc_Absyn_Printf_ft: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp1A6.f2)->tag == 0U){if(((struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp1A6.f2)->f1)->tag == 20U){if(((((struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp1A6.f2)->f1)->f1).KnownDatatype).tag == 2){_LL69: _tmp1A8=*((((struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp1A6.f2)->f1)->f1).KnownDatatype).val;_LL6A: {struct Cyc_Absyn_Datatypedecl*tud=_tmp1A8;
# 1068
problem=({int(*_tmp1A9)(struct _tuple1*,struct _tuple1*)=Cyc_Absyn_qvar_cmp;struct _tuple1*_tmp1AA=tud->name;struct _tuple1*_tmp1AB=({Cyc_Absyn_datatype_print_arg_qvar();});_tmp1A9(_tmp1AA,_tmp1AB);})!= 0;
goto _LL68;}}else{goto _LL6D;}}else{goto _LL6D;}}else{goto _LL6D;}case Cyc_Absyn_Scanf_ft: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp1A6.f2)->tag == 0U){if(((struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp1A6.f2)->f1)->tag == 20U){if(((((struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp1A6.f2)->f1)->f1).KnownDatatype).tag == 2){_LL6B: _tmp1A7=*((((struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp1A6.f2)->f1)->f1).KnownDatatype).val;_LL6C: {struct Cyc_Absyn_Datatypedecl*tud=_tmp1A7;
# 1071
problem=({int(*_tmp1AC)(struct _tuple1*,struct _tuple1*)=Cyc_Absyn_qvar_cmp;struct _tuple1*_tmp1AD=tud->name;struct _tuple1*_tmp1AE=({Cyc_Absyn_datatype_scanf_arg_qvar();});_tmp1AC(_tmp1AD,_tmp1AE);})!= 0;
goto _LL68;}}else{goto _LL6D;}}else{goto _LL6D;}}else{goto _LL6D;}default: _LL6D: _LL6E:
# 1074
 problem=1;
goto _LL68;}_LL68:;}
# 1077
if(problem)
({void*_tmp1B1=0U;({unsigned _tmp37F=loc;struct _fat_ptr _tmp37E=({const char*_tmp1B2="format attribute and vararg types don't match";_tag_fat(_tmp1B2,sizeof(char),46U);});Cyc_Tcutil_terr(_tmp37F,_tmp37E,_tag_fat(_tmp1B1,sizeof(void*),0U));});});}}}}
# 1010
{
# 1085
struct Cyc_List_List*rpo=rgn_po;
# 1010
for(0;rpo != 0;rpo=rpo->tl){
# 1087
struct _tuple0*_stmttmp20=(struct _tuple0*)rpo->hd;void*_tmp1B5;void*_tmp1B4;_LL70: _tmp1B4=_stmttmp20->f1;_tmp1B5=_stmttmp20->f2;_LL71: {void*r1=_tmp1B4;void*r2=_tmp1B5;
new_cvtenv=({Cyc_Tctyp_i_check_valid_type(loc,te,new_cvtenv,& Cyc_Tcutil_ek,r1,1,1);});
new_cvtenv=({Cyc_Tctyp_i_check_valid_type(loc,te,new_cvtenv,& Cyc_Tcutil_trk,r2,1,1);});}}}{
# 1094
struct Cyc_Tcenv_Tenv*ftenv=({Cyc_Tcenv_new_bogus_fenv_in_env(te,tr,args);});
# 1096
struct _tuple16 _stmttmp21=({({unsigned _tmp383=loc;struct Cyc_Tcenv_Tenv*_tmp382=ftenv;struct Cyc_Tctyp_CVTEnv _tmp381=new_cvtenv;struct _fat_ptr _tmp380=({const char*_tmp1F2="@requires";_tag_fat(_tmp1F2,sizeof(char),10U);});Cyc_Tctyp_check_clause(_tmp383,_tmp382,_tmp381,_tmp380,req);});});struct Cyc_List_List*_tmp1B7;struct Cyc_Tctyp_CVTEnv _tmp1B6;_LL73: _tmp1B6=_stmttmp21.f1;_tmp1B7=_stmttmp21.f2;_LL74: {struct Cyc_Tctyp_CVTEnv nenv=_tmp1B6;struct Cyc_List_List*req_rs=_tmp1B7;
new_cvtenv=nenv;
*req_relns=req_rs;
({({void(*_tmp385)(void(*f)(struct Cyc_List_List*,struct Cyc_Relations_Reln*),struct Cyc_List_List*env,struct Cyc_List_List*x)=({void(*_tmp1B8)(void(*f)(struct Cyc_List_List*,struct Cyc_Relations_Reln*),struct Cyc_List_List*env,struct Cyc_List_List*x)=(void(*)(void(*f)(struct Cyc_List_List*,struct Cyc_Relations_Reln*),struct Cyc_List_List*env,struct Cyc_List_List*x))Cyc_List_iter_c;_tmp1B8;});struct Cyc_List_List*_tmp384=args;_tmp385(Cyc_Tcutil_replace_rops,_tmp384,req_rs);});});{
struct _tuple16 _stmttmp22=({({unsigned _tmp389=loc;struct Cyc_Tcenv_Tenv*_tmp388=ftenv;struct Cyc_Tctyp_CVTEnv _tmp387=new_cvtenv;struct _fat_ptr _tmp386=({const char*_tmp1F1="@ensures";_tag_fat(_tmp1F1,sizeof(char),9U);});Cyc_Tctyp_check_clause(_tmp389,_tmp388,_tmp387,_tmp386,ens);});});struct Cyc_List_List*_tmp1BA;struct Cyc_Tctyp_CVTEnv _tmp1B9;_LL76: _tmp1B9=_stmttmp22.f1;_tmp1BA=_stmttmp22.f2;_LL77: {struct Cyc_Tctyp_CVTEnv nenv=_tmp1B9;struct Cyc_List_List*ens_rs=_tmp1BA;
new_cvtenv=nenv;
*ens_relns=ens_rs;
({({void(*_tmp38B)(void(*f)(struct Cyc_List_List*,struct Cyc_Relations_Reln*),struct Cyc_List_List*env,struct Cyc_List_List*x)=({void(*_tmp1BB)(void(*f)(struct Cyc_List_List*,struct Cyc_Relations_Reln*),struct Cyc_List_List*env,struct Cyc_List_List*x)=(void(*)(void(*f)(struct Cyc_List_List*,struct Cyc_Relations_Reln*),struct Cyc_List_List*env,struct Cyc_List_List*x))Cyc_List_iter_c;_tmp1BB;});struct Cyc_List_List*_tmp38A=args;_tmp38B(Cyc_Tcutil_replace_rops,_tmp38A,ens_rs);});});
# 1106
if(*eff != 0)
# 1108
new_cvtenv=({Cyc_Tctyp_i_check_valid_type(loc,te,new_cvtenv,& Cyc_Tcutil_ek,(void*)_check_null(*eff),1,1);});else{
# 1113
struct Cyc_List_List*effect=0;
# 1118
{struct Cyc_List_List*tvs=new_cvtenv.free_vars;for(0;tvs != 0;tvs=tvs->tl){
# 1120
struct _tuple15 _stmttmp23=*((struct _tuple15*)tvs->hd);int _tmp1BD;struct Cyc_Absyn_Tvar*_tmp1BC;_LL79: _tmp1BC=_stmttmp23.f1;_tmp1BD=_stmttmp23.f2;_LL7A: {struct Cyc_Absyn_Tvar*tv=_tmp1BC;int put_in_eff=_tmp1BD;
if(!put_in_eff)continue;{void*_stmttmp24=({Cyc_Absyn_compress_kb(tv->kind);});void*_tmp1BE=_stmttmp24;struct Cyc_Core_Opt**_tmp1BF;struct Cyc_Absyn_Kind*_tmp1C0;struct Cyc_Absyn_Kind*_tmp1C1;struct Cyc_Core_Opt**_tmp1C2;struct Cyc_Absyn_Kind*_tmp1C4;struct Cyc_Core_Opt**_tmp1C3;switch(*((int*)_tmp1BE)){case 2U: _LL7C: _tmp1C3=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp1BE)->f1;_tmp1C4=((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp1BE)->f2;if((int)_tmp1C4->kind == (int)Cyc_Absyn_RgnKind){_LL7D: {struct Cyc_Core_Opt**f=_tmp1C3;struct Cyc_Absyn_Kind*r=_tmp1C4;
# 1126
if((int)r->aliasqual == (int)Cyc_Absyn_Top){
# 1128
({struct Cyc_Core_Opt*_tmp38C=({Cyc_Tcutil_kind_to_bound_opt(& Cyc_Tcutil_rk);});*f=_tmp38C;});
_tmp1C1=r;goto _LL7F;}
# 1126
({struct Cyc_Core_Opt*_tmp38D=({Cyc_Tcutil_kind_to_bound_opt(r);});*f=_tmp38D;});
# 1132
_tmp1C1=r;goto _LL7F;}}else{switch(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp1BE)->f2)->kind){case Cyc_Absyn_BoolKind: _LL82: _LL83:
# 1153
 goto _LL85;case Cyc_Absyn_PtrBndKind: _LL84: _LL85:
 goto _LL87;case Cyc_Absyn_IntKind: _LL86: _LL87:
 goto _LL89;case Cyc_Absyn_EffKind: _LL8E: _tmp1C2=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp1BE)->f1;_LL8F: {struct Cyc_Core_Opt**f=_tmp1C2;
# 1160
({struct Cyc_Core_Opt*_tmp38E=({Cyc_Tcutil_kind_to_bound_opt(& Cyc_Tcutil_ek);});*f=_tmp38E;});goto _LL91;}default: goto _LL94;}}case 0U: _LL7E: _tmp1C1=((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp1BE)->f1;if((int)_tmp1C1->kind == (int)Cyc_Absyn_RgnKind){_LL7F: {struct Cyc_Absyn_Kind*r=_tmp1C1;
# 1135
effect=({struct Cyc_List_List*_tmp1C7=_cycalloc(sizeof(*_tmp1C7));({void*_tmp38F=({void*(*_tmp1C5)(void*)=Cyc_Absyn_access_eff;void*_tmp1C6=({Cyc_Absyn_var_type(tv);});_tmp1C5(_tmp1C6);});_tmp1C7->hd=_tmp38F;}),_tmp1C7->tl=effect;_tmp1C7;});
goto _LL7B;}}else{_LL80: _tmp1C0=((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp1BE)->f1;if((int)_tmp1C0->kind == (int)Cyc_Absyn_XRgnKind){_LL81: {struct Cyc_Absyn_Kind*r=_tmp1C0;
# 1139
struct Cyc_Absyn_RgnEffect*ef=({Cyc_Absyn_find_rgneffect(tv,ieff);});
if(ef == 0){
# 1142
void*formtyp=({void*(*_tmp1CC)(struct Cyc_List_List*)=Cyc_Absyn_join_eff;struct Cyc_List_List*_tmp1CD=({({struct Cyc_List_List*(*_tmp390)(void*(*f)(struct Cyc_Absyn_RgnEffect*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp1CE)(void*(*f)(struct Cyc_Absyn_RgnEffect*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(void*(*f)(struct Cyc_Absyn_RgnEffect*),struct Cyc_List_List*x))Cyc_List_map;_tmp1CE;});_tmp390(Cyc_Absyn_rgneffect2type,ieff);});});_tmp1CC(_tmp1CD);});
({struct Cyc_String_pa_PrintArg_struct _tmp1CA=({struct Cyc_String_pa_PrintArg_struct _tmp2A0;_tmp2A0.tag=0U,({
struct _fat_ptr _tmp391=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_tvar2string(tv);}));_tmp2A0.f1=_tmp391;});_tmp2A0;});struct Cyc_String_pa_PrintArg_struct _tmp1CB=({struct Cyc_String_pa_PrintArg_struct _tmp29F;_tmp29F.tag=0U,({struct _fat_ptr _tmp392=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(formtyp);}));_tmp29F.f1=_tmp392;});_tmp29F;});void*_tmp1C8[2U];_tmp1C8[0]=& _tmp1CA,_tmp1C8[1]=& _tmp1CB;({unsigned _tmp394=loc;struct _fat_ptr _tmp393=({const char*_tmp1C9="Type variable %s could not be found in @ieffect %s";_tag_fat(_tmp1C9,sizeof(char),51U);});Cyc_Tcutil_terr(_tmp394,_tmp393,_tag_fat(_tmp1C8,sizeof(void*),2U));});});}else{
# 1150
effect=({struct Cyc_List_List*_tmp1CF=_cycalloc(sizeof(*_tmp1CF));({void*_tmp395=({Cyc_Absyn_rgneffect2type(ef);});_tmp1CF->hd=_tmp395;}),_tmp1CF->tl=effect;_tmp1CF;});}
# 1152
goto _LL7B;}}else{switch(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp1BE)->f1)->kind){case Cyc_Absyn_BoolKind: _LL88: _LL89:
# 1156
 goto _LL8B;case Cyc_Absyn_PtrBndKind: _LL8A: _LL8B:
 goto _LL8D;case Cyc_Absyn_IntKind: _LL8C: _LL8D:
 goto _LL7B;case Cyc_Absyn_EffKind: _LL90: _LL91:
# 1162
 effect=({struct Cyc_List_List*_tmp1D0=_cycalloc(sizeof(*_tmp1D0));({void*_tmp396=({Cyc_Absyn_var_type(tv);});_tmp1D0->hd=_tmp396;}),_tmp1D0->tl=effect;_tmp1D0;});goto _LL7B;default: _LL94: _LL95:
# 1167
 effect=({struct Cyc_List_List*_tmp1D5=_cycalloc(sizeof(*_tmp1D5));({void*_tmp397=({void*(*_tmp1D3)(void*)=Cyc_Absyn_regionsof_eff;void*_tmp1D4=({Cyc_Absyn_var_type(tv);});_tmp1D3(_tmp1D4);});_tmp1D5->hd=_tmp397;}),_tmp1D5->tl=effect;_tmp1D5;});goto _LL7B;}}}default: _LL92: _tmp1BF=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*)_tmp1BE)->f1;_LL93: {struct Cyc_Core_Opt**f=_tmp1BF;
# 1164
({struct Cyc_Core_Opt*_tmp399=({struct Cyc_Core_Opt*_tmp1D2=_cycalloc(sizeof(*_tmp1D2));({void*_tmp398=(void*)({struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*_tmp1D1=_cycalloc(sizeof(*_tmp1D1));_tmp1D1->tag=2U,_tmp1D1->f1=0,_tmp1D1->f2=& Cyc_Tcutil_ak;_tmp1D1;});_tmp1D2->v=_tmp398;});_tmp1D2;});*f=_tmp399;});goto _LL95;}}_LL7B:;}}}}
# 1171
{struct Cyc_List_List*ts=new_cvtenv.free_evars;for(0;ts != 0;ts=ts->tl){
# 1173
struct _tuple14 _stmttmp25=*((struct _tuple14*)ts->hd);int _tmp1D7;void*_tmp1D6;_LL97: _tmp1D6=_stmttmp25.f1;_tmp1D7=_stmttmp25.f2;_LL98: {void*tv=_tmp1D6;int put_in_eff=_tmp1D7;
if(!put_in_eff)continue;{struct Cyc_Absyn_Kind*_stmttmp26=({Cyc_Tcutil_type_kind(tv);});struct Cyc_Absyn_Kind*_tmp1D8=_stmttmp26;switch(((struct Cyc_Absyn_Kind*)_tmp1D8)->kind){case Cyc_Absyn_RgnKind: _LL9A: _LL9B:
# 1177
 effect=({struct Cyc_List_List*_tmp1D9=_cycalloc(sizeof(*_tmp1D9));({void*_tmp39A=({Cyc_Absyn_access_eff(tv);});_tmp1D9->hd=_tmp39A;}),_tmp1D9->tl=effect;_tmp1D9;});goto _LL99;case Cyc_Absyn_EffKind: _LL9C: _LL9D:
 effect=({struct Cyc_List_List*_tmp1DA=_cycalloc(sizeof(*_tmp1DA));_tmp1DA->hd=tv,_tmp1DA->tl=effect;_tmp1DA;});goto _LL99;case Cyc_Absyn_IntKind: _LL9E: _LL9F:
 goto _LL99;default: _LLA0: _LLA1:
 effect=({struct Cyc_List_List*_tmp1DB=_cycalloc(sizeof(*_tmp1DB));({void*_tmp39B=({Cyc_Absyn_regionsof_eff(tv);});_tmp1DB->hd=_tmp39B;}),_tmp1DB->tl=effect;_tmp1DB;});goto _LL99;}_LL99:;}}}}
# 1183
({void*_tmp39C=({Cyc_Absyn_join_eff(effect);});*eff=_tmp39C;});}
# 1189
if(*btvs != 0){
struct Cyc_List_List*bs=*btvs;for(0;bs != 0;bs=bs->tl){
# 1192
void*_stmttmp27=({Cyc_Absyn_compress_kb(((struct Cyc_Absyn_Tvar*)bs->hd)->kind);});void*_tmp1DC=_stmttmp27;struct Cyc_Absyn_Kind*_tmp1DE;struct Cyc_Core_Opt**_tmp1DD;struct Cyc_Core_Opt**_tmp1DF;struct Cyc_Core_Opt**_tmp1E0;struct Cyc_Core_Opt**_tmp1E1;struct Cyc_Core_Opt**_tmp1E2;struct Cyc_Core_Opt**_tmp1E3;struct Cyc_Core_Opt**_tmp1E4;struct Cyc_Core_Opt**_tmp1E5;struct Cyc_Core_Opt**_tmp1E6;struct Cyc_Core_Opt**_tmp1E7;switch(*((int*)_tmp1DC)){case 1U: _LLA3: _tmp1E7=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*)_tmp1DC)->f1;_LLA4: {struct Cyc_Core_Opt**f=_tmp1E7;
# 1195
({struct Cyc_String_pa_PrintArg_struct _tmp1EA=({struct Cyc_String_pa_PrintArg_struct _tmp2A1;_tmp2A1.tag=0U,_tmp2A1.f1=(struct _fat_ptr)((struct _fat_ptr)*((struct Cyc_Absyn_Tvar*)bs->hd)->name);_tmp2A1;});void*_tmp1E8[1U];_tmp1E8[0]=& _tmp1EA;({unsigned _tmp39E=loc;struct _fat_ptr _tmp39D=({const char*_tmp1E9="Type variable %s unconstrained, assuming boxed";_tag_fat(_tmp1E9,sizeof(char),47U);});Cyc_Tcutil_warn(_tmp39E,_tmp39D,_tag_fat(_tmp1E8,sizeof(void*),1U));});});
# 1197
_tmp1E6=f;goto _LLA6;}case 2U: switch(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp1DC)->f2)->kind){case Cyc_Absyn_BoxKind: if(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp1DC)->f2)->aliasqual == Cyc_Absyn_Top){_LLA5: _tmp1E6=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp1DC)->f1;_LLA6: {struct Cyc_Core_Opt**f=_tmp1E6;
_tmp1E5=f;goto _LLA8;}}else{goto _LLB5;}case Cyc_Absyn_MemKind: switch(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp1DC)->f2)->aliasqual){case Cyc_Absyn_Top: _LLA7: _tmp1E5=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp1DC)->f1;_LLA8: {struct Cyc_Core_Opt**f=_tmp1E5;
_tmp1E4=f;goto _LLAA;}case Cyc_Absyn_Aliasable: _LLA9: _tmp1E4=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp1DC)->f1;_LLAA: {struct Cyc_Core_Opt**f=_tmp1E4;
_tmp1E2=f;goto _LLAC;}case Cyc_Absyn_Unique: _LLAF: _tmp1E3=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp1DC)->f1;_LLB0: {struct Cyc_Core_Opt**f=_tmp1E3;
# 1203
_tmp1E0=f;goto _LLB2;}default: goto _LLB5;}case Cyc_Absyn_AnyKind: switch(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp1DC)->f2)->aliasqual){case Cyc_Absyn_Top: _LLAB: _tmp1E2=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp1DC)->f1;_LLAC: {struct Cyc_Core_Opt**f=_tmp1E2;
# 1201
_tmp1E1=f;goto _LLAE;}case Cyc_Absyn_Aliasable: _LLAD: _tmp1E1=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp1DC)->f1;_LLAE: {struct Cyc_Core_Opt**f=_tmp1E1;
({struct Cyc_Core_Opt*_tmp39F=({Cyc_Tcutil_kind_to_bound_opt(& Cyc_Tcutil_bk);});*f=_tmp39F;});goto _LLA2;}case Cyc_Absyn_Unique: _LLB1: _tmp1E0=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp1DC)->f1;_LLB2: {struct Cyc_Core_Opt**f=_tmp1E0;
# 1204
({struct Cyc_Core_Opt*_tmp3A0=({Cyc_Tcutil_kind_to_bound_opt(& Cyc_Tcutil_ubk);});*f=_tmp3A0;});goto _LLA2;}default: goto _LLB5;}case Cyc_Absyn_RgnKind: if(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp1DC)->f2)->aliasqual == Cyc_Absyn_Top){_LLB3: _tmp1DF=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp1DC)->f1;_LLB4: {struct Cyc_Core_Opt**f=_tmp1DF;
({struct Cyc_Core_Opt*_tmp3A1=({Cyc_Tcutil_kind_to_bound_opt(& Cyc_Tcutil_rk);});*f=_tmp3A1;});goto _LLA2;}}else{goto _LLB5;}default: _LLB5: _tmp1DD=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp1DC)->f1;_tmp1DE=((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp1DC)->f2;_LLB6: {struct Cyc_Core_Opt**f=_tmp1DD;struct Cyc_Absyn_Kind*k=_tmp1DE;
({struct Cyc_Core_Opt*_tmp3A2=({Cyc_Tcutil_kind_to_bound_opt(k);});*f=_tmp3A2;});goto _LLA2;}}default: if(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp1DC)->f1)->kind == Cyc_Absyn_MemKind){_LLB7: _LLB8:
({void*_tmp1EB=0U;({unsigned _tmp3A4=loc;struct _fat_ptr _tmp3A3=({const char*_tmp1EC="functions can't abstract M types";_tag_fat(_tmp1EC,sizeof(char),33U);});Cyc_Tcutil_terr(_tmp3A4,_tmp3A3,_tag_fat(_tmp1EB,sizeof(void*),0U));});});goto _LLA2;}else{_LLB9: _LLBA:
 goto _LLA2;}}_LLA2:;}}
# 1189
({struct Cyc_List_List*_tmp3A5=({Cyc_Tctyp_remove_bound_tvars(Cyc_Core_heap_region,new_cvtenv.kind_env,*btvs);});cvtenv.kind_env=_tmp3A5;});
# 1213
({struct Cyc_List_List*_tmp3A6=({Cyc_Tctyp_remove_bound_tvars_bool(new_cvtenv.r,new_cvtenv.free_vars,*btvs);});new_cvtenv.free_vars=_tmp3A6;});
# 1215
{struct Cyc_List_List*tvs=new_cvtenv.free_vars;for(0;tvs != 0;tvs=tvs->tl){
# 1217
struct _tuple15 _stmttmp28=*((struct _tuple15*)tvs->hd);int _tmp1EE;struct Cyc_Absyn_Tvar*_tmp1ED;_LLBC: _tmp1ED=_stmttmp28.f1;_tmp1EE=_stmttmp28.f2;_LLBD: {struct Cyc_Absyn_Tvar*t=_tmp1ED;int b=_tmp1EE;
({struct Cyc_List_List*_tmp3A7=({Cyc_Tctyp_fast_add_free_tvar_bool(cvtenv.r,cvtenv.free_vars,t,b);});cvtenv.free_vars=_tmp3A7;});}}}
# 1221
{struct Cyc_List_List*evs=new_cvtenv.free_evars;for(0;evs != 0;evs=evs->tl){
# 1223
struct _tuple14 _stmttmp29=*((struct _tuple14*)evs->hd);int _tmp1F0;void*_tmp1EF;_LLBF: _tmp1EF=_stmttmp29.f1;_tmp1F0=_stmttmp29.f2;_LLC0: {void*e=_tmp1EF;int b=_tmp1F0;
({struct Cyc_List_List*_tmp3A8=({Cyc_Tctyp_add_free_evar(cvtenv.r,cvtenv.free_evars,e,b);});cvtenv.free_evars=_tmp3A8;});}}}
# 1226
goto _LL0;}}}}}}case 6U: _LL13: _tmp10B=((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp104)->f1;_LL14: {struct Cyc_List_List*tq_ts=_tmp10B;
# 1229
for(0;tq_ts != 0;tq_ts=tq_ts->tl){
struct _tuple18*p=(struct _tuple18*)tq_ts->hd;
cvtenv=({Cyc_Tctyp_i_check_valid_type(loc,te,cvtenv,& Cyc_Tcutil_tmk,(*p).f2,1,0);});
({int _tmp3A9=({Cyc_Tcutil_extract_const_from_typedef(loc,((*p).f1).print_const,(*p).f2);});((*p).f1).real_const=_tmp3A9;});}
# 1235
goto _LL0;}case 7U: _LL15: _tmp109=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp104)->f1;_tmp10A=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp104)->f2;_LL16: {enum Cyc_Absyn_AggrKind k=_tmp109;struct Cyc_List_List*fs=_tmp10A;
# 1239
struct Cyc_List_List*prev_fields=0;
for(0;fs != 0;fs=fs->tl){
struct Cyc_Absyn_Aggrfield*_stmttmp2A=(struct Cyc_Absyn_Aggrfield*)fs->hd;struct Cyc_Absyn_Exp*_tmp1F8;struct Cyc_List_List*_tmp1F7;struct Cyc_Absyn_Exp*_tmp1F6;void*_tmp1F5;struct Cyc_Absyn_Tqual*_tmp1F4;struct _fat_ptr*_tmp1F3;_LLC2: _tmp1F3=_stmttmp2A->name;_tmp1F4=(struct Cyc_Absyn_Tqual*)& _stmttmp2A->tq;_tmp1F5=_stmttmp2A->type;_tmp1F6=_stmttmp2A->width;_tmp1F7=_stmttmp2A->attributes;_tmp1F8=_stmttmp2A->requires_clause;_LLC3: {struct _fat_ptr*fn=_tmp1F3;struct Cyc_Absyn_Tqual*tqp=_tmp1F4;void*t=_tmp1F5;struct Cyc_Absyn_Exp*width=_tmp1F6;struct Cyc_List_List*atts=_tmp1F7;struct Cyc_Absyn_Exp*requires_clause=_tmp1F8;
if(({({int(*_tmp3AB)(int(*compare)(struct _fat_ptr*,struct _fat_ptr*),struct Cyc_List_List*l,struct _fat_ptr*x)=({int(*_tmp1F9)(int(*compare)(struct _fat_ptr*,struct _fat_ptr*),struct Cyc_List_List*l,struct _fat_ptr*x)=(int(*)(int(*compare)(struct _fat_ptr*,struct _fat_ptr*),struct Cyc_List_List*l,struct _fat_ptr*x))Cyc_List_mem;_tmp1F9;});struct Cyc_List_List*_tmp3AA=prev_fields;_tmp3AB(Cyc_strptrcmp,_tmp3AA,fn);});}))
({struct Cyc_String_pa_PrintArg_struct _tmp1FC=({struct Cyc_String_pa_PrintArg_struct _tmp2A3;_tmp2A3.tag=0U,_tmp2A3.f1=(struct _fat_ptr)((struct _fat_ptr)*fn);_tmp2A3;});void*_tmp1FA[1U];_tmp1FA[0]=& _tmp1FC;({unsigned _tmp3AD=loc;struct _fat_ptr _tmp3AC=({const char*_tmp1FB="duplicate field %s";_tag_fat(_tmp1FB,sizeof(char),19U);});Cyc_Tcutil_terr(_tmp3AD,_tmp3AC,_tag_fat(_tmp1FA,sizeof(void*),1U));});});
# 1242
if(({({struct _fat_ptr _tmp3AE=(struct _fat_ptr)*fn;Cyc_strcmp(_tmp3AE,({const char*_tmp1FD="";_tag_fat(_tmp1FD,sizeof(char),1U);}));});})!= 0)
# 1245
prev_fields=({struct Cyc_List_List*_tmp1FE=_cycalloc(sizeof(*_tmp1FE));_tmp1FE->hd=fn,_tmp1FE->tl=prev_fields;_tmp1FE;});
# 1242
cvtenv=({Cyc_Tctyp_i_check_valid_type(loc,te,cvtenv,& Cyc_Tcutil_tmk,t,1,0);});
# 1247
({int _tmp3AF=({Cyc_Tcutil_extract_const_from_typedef(loc,tqp->print_const,t);});tqp->real_const=_tmp3AF;});
({Cyc_Tcutil_check_bitfield(loc,t,width,fn);});
({Cyc_Tctyp_check_field_atts(loc,fn,atts);});
if(requires_clause != 0){
# 1252
if((int)k != (int)1U)
({void*_tmp1FF=0U;({unsigned _tmp3B1=loc;struct _fat_ptr _tmp3B0=({const char*_tmp200="@requires clause is only allowed on union members";_tag_fat(_tmp200,sizeof(char),50U);});Cyc_Tcutil_terr(_tmp3B1,_tmp3B0,_tag_fat(_tmp1FF,sizeof(void*),0U));});});
# 1252
({void*(*_tmp201)(struct Cyc_Tcenv_Tenv*,void**,struct Cyc_Absyn_Exp*)=Cyc_Tcexp_tcExp;struct Cyc_Tcenv_Tenv*_tmp202=({Cyc_Tcenv_allow_valueof(te);});void**_tmp203=0;struct Cyc_Absyn_Exp*_tmp204=requires_clause;_tmp201(_tmp202,_tmp203,_tmp204);});
# 1255
if(!({Cyc_Tcutil_is_integral(requires_clause);}))
({struct Cyc_String_pa_PrintArg_struct _tmp207=({struct Cyc_String_pa_PrintArg_struct _tmp2A4;_tmp2A4.tag=0U,({
struct _fat_ptr _tmp3B2=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string((void*)_check_null(requires_clause->topt));}));_tmp2A4.f1=_tmp3B2;});_tmp2A4;});void*_tmp205[1U];_tmp205[0]=& _tmp207;({unsigned _tmp3B4=loc;struct _fat_ptr _tmp3B3=({const char*_tmp206="@requires clause has type %s instead of integral type";_tag_fat(_tmp206,sizeof(char),54U);});Cyc_Tcutil_terr(_tmp3B4,_tmp3B3,_tag_fat(_tmp205,sizeof(void*),1U));});});
# 1255
cvtenv=({Cyc_Tctyp_i_check_valid_type_level_exp(requires_clause,te,cvtenv);});}}}
# 1261
goto _LL0;}default: _LL17: _tmp105=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp104)->f1;_tmp106=(struct Cyc_List_List**)&((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp104)->f2;_tmp107=(struct Cyc_Absyn_Typedefdecl**)&((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp104)->f3;_tmp108=(void**)&((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp104)->f4;_LL18: {struct _tuple1*tdn=_tmp105;struct Cyc_List_List**targs_ref=_tmp106;struct Cyc_Absyn_Typedefdecl**tdopt=_tmp107;void**toptp=_tmp108;
# 1264
struct Cyc_List_List*targs=*targs_ref;
# 1266
struct Cyc_Absyn_Typedefdecl*td;
{struct _handler_cons _tmp208;_push_handler(& _tmp208);{int _tmp20A=0;if(setjmp(_tmp208.handler))_tmp20A=1;if(!_tmp20A){td=({Cyc_Tcenv_lookup_typedefdecl(te,loc,tdn);});;_pop_handler();}else{void*_tmp209=(void*)Cyc_Core_get_exn_thrown();void*_tmp20B=_tmp209;void*_tmp20C;if(((struct Cyc_Dict_Absent_exn_struct*)_tmp20B)->tag == Cyc_Dict_Absent){_LLC5: _LLC6:
# 1269
({struct Cyc_String_pa_PrintArg_struct _tmp20F=({struct Cyc_String_pa_PrintArg_struct _tmp2A5;_tmp2A5.tag=0U,({struct _fat_ptr _tmp3B5=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(tdn);}));_tmp2A5.f1=_tmp3B5;});_tmp2A5;});void*_tmp20D[1U];_tmp20D[0]=& _tmp20F;({unsigned _tmp3B7=loc;struct _fat_ptr _tmp3B6=({const char*_tmp20E="unbound typedef name %s";_tag_fat(_tmp20E,sizeof(char),24U);});Cyc_Tcutil_terr(_tmp3B7,_tmp3B6,_tag_fat(_tmp20D,sizeof(void*),1U));});});
return cvtenv;}else{_LLC7: _tmp20C=_tmp20B;_LLC8: {void*exn=_tmp20C;(int)_rethrow(exn);}}_LLC4:;}}}
# 1272
*tdopt=td;{
struct Cyc_List_List*tvs=td->tvs;
struct Cyc_List_List*ts=targs;
struct Cyc_List_List*inst=0;
# 1277
for(0;ts != 0 && tvs != 0;(ts=ts->tl,tvs=tvs->tl)){
struct Cyc_Absyn_Kind*k=({Cyc_Tctyp_tvar_inst_kind((struct Cyc_Absyn_Tvar*)tvs->hd,& Cyc_Tcutil_tbk,expected_kind,td);});
# 1282
cvtenv=({Cyc_Tctyp_i_check_valid_type(loc,te,cvtenv,k,(void*)ts->hd,1,allow_abs_aggr);});
({Cyc_Tcutil_check_no_qual(loc,(void*)ts->hd);});
inst=({struct Cyc_List_List*_tmp211=_cycalloc(sizeof(*_tmp211));({struct _tuple19*_tmp3B8=({struct _tuple19*_tmp210=_cycalloc(sizeof(*_tmp210));_tmp210->f1=(struct Cyc_Absyn_Tvar*)tvs->hd,_tmp210->f2=(void*)ts->hd;_tmp210;});_tmp211->hd=_tmp3B8;}),_tmp211->tl=inst;_tmp211;});}
# 1286
if(ts != 0)
({struct Cyc_String_pa_PrintArg_struct _tmp214=({struct Cyc_String_pa_PrintArg_struct _tmp2A6;_tmp2A6.tag=0U,({struct _fat_ptr _tmp3B9=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(tdn);}));_tmp2A6.f1=_tmp3B9;});_tmp2A6;});void*_tmp212[1U];_tmp212[0]=& _tmp214;({unsigned _tmp3BB=loc;struct _fat_ptr _tmp3BA=({const char*_tmp213="too many parameters for typedef %s";_tag_fat(_tmp213,sizeof(char),35U);});Cyc_Tcutil_terr(_tmp3BB,_tmp3BA,_tag_fat(_tmp212,sizeof(void*),1U));});});
# 1286
if(tvs != 0){
# 1289
struct Cyc_List_List*hidden_ts=0;
# 1291
for(0;tvs != 0;tvs=tvs->tl){
struct Cyc_Absyn_Kind*k=({Cyc_Tctyp_tvar_inst_kind((struct Cyc_Absyn_Tvar*)tvs->hd,& Cyc_Tcutil_bk,expected_kind,td);});
void*e=({Cyc_Absyn_new_evar(0,0);});
hidden_ts=({struct Cyc_List_List*_tmp215=_cycalloc(sizeof(*_tmp215));_tmp215->hd=e,_tmp215->tl=hidden_ts;_tmp215;});
cvtenv=({Cyc_Tctyp_i_check_valid_type(loc,te,cvtenv,k,e,1,allow_abs_aggr);});
inst=({struct Cyc_List_List*_tmp217=_cycalloc(sizeof(*_tmp217));({struct _tuple19*_tmp3BC=({struct _tuple19*_tmp216=_cycalloc(sizeof(*_tmp216));_tmp216->f1=(struct Cyc_Absyn_Tvar*)tvs->hd,_tmp216->f2=e;_tmp216;});_tmp217->hd=_tmp3BC;}),_tmp217->tl=inst;_tmp217;});}
# 1299
({struct Cyc_List_List*_tmp3BD=({struct Cyc_List_List*(*_tmp218)(struct Cyc_List_List*x,struct Cyc_List_List*y)=Cyc_List_imp_append;struct Cyc_List_List*_tmp219=targs;struct Cyc_List_List*_tmp21A=({Cyc_List_imp_rev(hidden_ts);});_tmp218(_tmp219,_tmp21A);});*targs_ref=_tmp3BD;});}
# 1286
if(td->defn != 0){
# 1302
void*new_typ=
inst == 0?(void*)_check_null(td->defn):({Cyc_Tcutil_substitute(inst,(void*)_check_null(td->defn));});
# 1305
*toptp=new_typ;}
# 1286
goto _LL0;}}}_LL0:;}
# 1310
if(!({int(*_tmp21B)(struct Cyc_Absyn_Kind*,struct Cyc_Absyn_Kind*)=Cyc_Tcutil_kind_leq;struct Cyc_Absyn_Kind*_tmp21C=({Cyc_Tcutil_type_kind(t);});struct Cyc_Absyn_Kind*_tmp21D=expected_kind;_tmp21B(_tmp21C,_tmp21D);}))
({struct Cyc_String_pa_PrintArg_struct _tmp220=({struct Cyc_String_pa_PrintArg_struct _tmp2A9;_tmp2A9.tag=0U,({
struct _fat_ptr _tmp3BE=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t);}));_tmp2A9.f1=_tmp3BE;});_tmp2A9;});struct Cyc_String_pa_PrintArg_struct _tmp221=({struct Cyc_String_pa_PrintArg_struct _tmp2A8;_tmp2A8.tag=0U,({struct _fat_ptr _tmp3BF=(struct _fat_ptr)((struct _fat_ptr)({struct _fat_ptr(*_tmp223)(struct Cyc_Absyn_Kind*)=Cyc_Absynpp_kind2string;struct Cyc_Absyn_Kind*_tmp224=({Cyc_Tcutil_type_kind(t);});_tmp223(_tmp224);}));_tmp2A8.f1=_tmp3BF;});_tmp2A8;});struct Cyc_String_pa_PrintArg_struct _tmp222=({struct Cyc_String_pa_PrintArg_struct _tmp2A7;_tmp2A7.tag=0U,({struct _fat_ptr _tmp3C0=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_kind2string(expected_kind);}));_tmp2A7.f1=_tmp3C0;});_tmp2A7;});void*_tmp21E[3U];_tmp21E[0]=& _tmp220,_tmp21E[1]=& _tmp221,_tmp21E[2]=& _tmp222;({unsigned _tmp3C2=loc;struct _fat_ptr _tmp3C1=({const char*_tmp21F="type %s has kind %s but as used here needs kind %s";_tag_fat(_tmp21F,sizeof(char),51U);});Cyc_Tcutil_terr(_tmp3C2,_tmp3C1,_tag_fat(_tmp21E,sizeof(void*),3U));});});
# 1310
return cvtenv;}
# 1323
static struct Cyc_Tctyp_CVTEnv Cyc_Tctyp_i_check_valid_type_level_exp(struct Cyc_Absyn_Exp*e,struct Cyc_Tcenv_Tenv*te,struct Cyc_Tctyp_CVTEnv cvtenv){
{void*_stmttmp2B=e->r;void*_tmp226=_stmttmp2B;struct Cyc_Absyn_Exp*_tmp227;struct Cyc_Absyn_Exp*_tmp228;void*_tmp229;void*_tmp22A;void*_tmp22B;struct Cyc_Absyn_Exp*_tmp22D;void*_tmp22C;struct Cyc_Absyn_Exp*_tmp22F;struct Cyc_Absyn_Exp*_tmp22E;struct Cyc_Absyn_Exp*_tmp231;struct Cyc_Absyn_Exp*_tmp230;struct Cyc_Absyn_Exp*_tmp233;struct Cyc_Absyn_Exp*_tmp232;struct Cyc_Absyn_Exp*_tmp236;struct Cyc_Absyn_Exp*_tmp235;struct Cyc_Absyn_Exp*_tmp234;struct Cyc_List_List*_tmp237;switch(*((int*)_tmp226)){case 0U: _LL1: _LL2:
 goto _LL4;case 32U: _LL3: _LL4:
 goto _LL6;case 33U: _LL5: _LL6:
 goto _LL8;case 2U: _LL7: _LL8:
 goto _LLA;case 1U: _LL9: _LLA:
 goto _LL0;case 3U: _LLB: _tmp237=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp226)->f2;_LLC: {struct Cyc_List_List*es=_tmp237;
# 1331
for(0;es != 0;es=es->tl){
cvtenv=({Cyc_Tctyp_i_check_valid_type_level_exp((struct Cyc_Absyn_Exp*)es->hd,te,cvtenv);});}
goto _LL0;}case 6U: _LLD: _tmp234=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp226)->f1;_tmp235=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp226)->f2;_tmp236=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp226)->f3;_LLE: {struct Cyc_Absyn_Exp*e1=_tmp234;struct Cyc_Absyn_Exp*e2=_tmp235;struct Cyc_Absyn_Exp*e3=_tmp236;
# 1335
cvtenv=({Cyc_Tctyp_i_check_valid_type_level_exp(e1,te,cvtenv);});
cvtenv=({Cyc_Tctyp_i_check_valid_type_level_exp(e2,te,cvtenv);});
cvtenv=({Cyc_Tctyp_i_check_valid_type_level_exp(e3,te,cvtenv);});
goto _LL0;}case 7U: _LLF: _tmp232=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp226)->f1;_tmp233=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp226)->f2;_LL10: {struct Cyc_Absyn_Exp*e1=_tmp232;struct Cyc_Absyn_Exp*e2=_tmp233;
_tmp230=e1;_tmp231=e2;goto _LL12;}case 8U: _LL11: _tmp230=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp226)->f1;_tmp231=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp226)->f2;_LL12: {struct Cyc_Absyn_Exp*e1=_tmp230;struct Cyc_Absyn_Exp*e2=_tmp231;
_tmp22E=e1;_tmp22F=e2;goto _LL14;}case 9U: _LL13: _tmp22E=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp226)->f1;_tmp22F=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp226)->f2;_LL14: {struct Cyc_Absyn_Exp*e1=_tmp22E;struct Cyc_Absyn_Exp*e2=_tmp22F;
# 1342
cvtenv=({Cyc_Tctyp_i_check_valid_type_level_exp(e1,te,cvtenv);});
cvtenv=({Cyc_Tctyp_i_check_valid_type_level_exp(e2,te,cvtenv);});
goto _LL0;}case 14U: _LL15: _tmp22C=(void*)((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp226)->f1;_tmp22D=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp226)->f2;_LL16: {void*t=_tmp22C;struct Cyc_Absyn_Exp*e1=_tmp22D;
# 1346
cvtenv=({Cyc_Tctyp_i_check_valid_type_level_exp(e1,te,cvtenv);});
cvtenv=({Cyc_Tctyp_i_check_valid_type(e->loc,te,cvtenv,& Cyc_Tcutil_tak,t,1,0);});
goto _LL0;}case 19U: _LL17: _tmp22B=(void*)((struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct*)_tmp226)->f1;_LL18: {void*t=_tmp22B;
_tmp22A=t;goto _LL1A;}case 17U: _LL19: _tmp22A=(void*)((struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct*)_tmp226)->f1;_LL1A: {void*t=_tmp22A;
# 1351
cvtenv=({Cyc_Tctyp_i_check_valid_type(e->loc,te,cvtenv,& Cyc_Tcutil_tak,t,1,0);});
goto _LL0;}case 40U: _LL1B: _tmp229=(void*)((struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct*)_tmp226)->f1;_LL1C: {void*t=_tmp229;
# 1354
cvtenv=({Cyc_Tctyp_i_check_valid_type(e->loc,te,cvtenv,& Cyc_Tcutil_ik,t,1,0);});
goto _LL0;}case 18U: _LL1D: _tmp228=((struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct*)_tmp226)->f1;_LL1E: {struct Cyc_Absyn_Exp*e=_tmp228;
# 1357
cvtenv=({Cyc_Tctyp_i_check_valid_type_level_exp(e,te,cvtenv);});
goto _LL0;}case 42U: _LL1F: _tmp227=((struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct*)_tmp226)->f1;_LL20: {struct Cyc_Absyn_Exp*e=_tmp227;
# 1360
cvtenv=({Cyc_Tctyp_i_check_valid_type_level_exp(e,te,cvtenv);});
goto _LL0;}default: _LL21: _LL22:
# 1363
({void*_tmp238=0U;({int(*_tmp3C4)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp23A)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp23A;});struct _fat_ptr _tmp3C3=({const char*_tmp239="non-type-level-expression in Tcutil::i_check_valid_type_level_exp";_tag_fat(_tmp239,sizeof(char),66U);});_tmp3C4(_tmp3C3,_tag_fat(_tmp238,sizeof(void*),0U));});});}_LL0:;}
# 1365
return cvtenv;}
# 1368
static struct Cyc_Tctyp_CVTEnv Cyc_Tctyp_check_valid_type(unsigned loc,struct Cyc_Tcenv_Tenv*te,struct Cyc_Tctyp_CVTEnv cvt,struct Cyc_Absyn_Kind*expected_kind,int allow_abs_aggr,void*t){
# 1373
struct Cyc_List_List*kind_env=cvt.kind_env;
# 1381
cvt=({Cyc_Tctyp_i_check_valid_type(loc,te,cvt,expected_kind,t,1,allow_abs_aggr);});
# 1383
{struct Cyc_List_List*vs=cvt.free_vars;for(0;vs != 0;vs=vs->tl){
({struct Cyc_List_List*_tmp3C5=({Cyc_Tctyp_fast_add_free_tvar(kind_env,(*((struct _tuple15*)vs->hd)).f1);});cvt.kind_env=_tmp3C5;});}}
# 1389
{struct Cyc_List_List*evs=cvt.free_evars;for(0;evs != 0;evs=evs->tl){
struct _tuple14 _stmttmp2C=*((struct _tuple14*)evs->hd);void*_tmp23C;_LL1: _tmp23C=_stmttmp2C.f1;_LL2: {void*e=_tmp23C;
void*_stmttmp2D=({Cyc_Tcutil_compress(e);});void*_tmp23D=_stmttmp2D;struct Cyc_Core_Opt**_tmp23E;if(((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp23D)->tag == 1U){_LL4: _tmp23E=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp23D)->f4;_LL5: {struct Cyc_Core_Opt**s=_tmp23E;
# 1393
if(*s == 0)
({struct Cyc_Core_Opt*_tmp3C6=({struct Cyc_Core_Opt*_tmp23F=_cycalloc(sizeof(*_tmp23F));_tmp23F->v=kind_env;_tmp23F;});*s=_tmp3C6;});else{
# 1397
struct Cyc_List_List*tvs=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(*s))->v;
struct Cyc_List_List*result=0;
for(0;tvs != 0;tvs=tvs->tl){
if(({({int(*_tmp3C8)(int(*compare)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_List_List*l,struct Cyc_Absyn_Tvar*x)=({int(*_tmp240)(int(*compare)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_List_List*l,struct Cyc_Absyn_Tvar*x)=(int(*)(int(*compare)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_List_List*l,struct Cyc_Absyn_Tvar*x))Cyc_List_mem;_tmp240;});struct Cyc_List_List*_tmp3C7=kind_env;_tmp3C8(Cyc_Tcutil_fast_tvar_cmp,_tmp3C7,(struct Cyc_Absyn_Tvar*)tvs->hd);});}))
result=({struct Cyc_List_List*_tmp241=_cycalloc(sizeof(*_tmp241));_tmp241->hd=(struct Cyc_Absyn_Tvar*)tvs->hd,_tmp241->tl=result;_tmp241;});}
# 1399
({struct Cyc_Core_Opt*_tmp3C9=({struct Cyc_Core_Opt*_tmp242=_cycalloc(sizeof(*_tmp242));_tmp242->v=result;_tmp242;});*s=_tmp3C9;});}
# 1404
goto _LL3;}}else{_LL6: _LL7:
 goto _LL3;}_LL3:;}}}
# 1408
return cvt;}
# 1415
void Cyc_Tctyp_check_valid_toplevel_type(unsigned loc,struct Cyc_Tcenv_Tenv*te,void*t){
# 1417
int generalize_evars=({Cyc_Tcutil_is_function_type(t);});
if(te->in_tempest && !te->tempest_generalize)generalize_evars=0;{struct Cyc_List_List*te_vars=({Cyc_Tcenv_lookup_type_vars(te);});
# 1421
struct Cyc_Absyn_Kind*expected_kind=({Cyc_Tcutil_is_function_type(t);})?& Cyc_Tcutil_tak:& Cyc_Tcutil_tmk;
struct Cyc_Tctyp_CVTEnv cvt=({({unsigned _tmp3CD=loc;struct Cyc_Tcenv_Tenv*_tmp3CC=te;struct Cyc_Tctyp_CVTEnv _tmp3CB=({struct Cyc_Tctyp_CVTEnv _tmp2AD;_tmp2AD.r=Cyc_Core_heap_region,_tmp2AD.kind_env=te_vars,_tmp2AD.fn_result=0,_tmp2AD.generalize_evars=generalize_evars,_tmp2AD.free_vars=0,_tmp2AD.free_evars=0;_tmp2AD;});struct Cyc_Absyn_Kind*_tmp3CA=expected_kind;Cyc_Tctyp_check_valid_type(_tmp3CD,_tmp3CC,_tmp3CB,_tmp3CA,1,t);});});
# 1425
struct Cyc_List_List*free_tvars=({({struct Cyc_List_List*(*_tmp3CF)(struct Cyc_Absyn_Tvar*(*f)(struct _tuple15*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp25B)(struct Cyc_Absyn_Tvar*(*f)(struct _tuple15*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_Tvar*(*f)(struct _tuple15*),struct Cyc_List_List*x))Cyc_List_map;_tmp25B;});struct Cyc_Absyn_Tvar*(*_tmp3CE)(struct _tuple15*)=({struct Cyc_Absyn_Tvar*(*_tmp25C)(struct _tuple15*)=(struct Cyc_Absyn_Tvar*(*)(struct _tuple15*))Cyc_Core_fst;_tmp25C;});_tmp3CF(_tmp3CE,cvt.free_vars);});});
struct Cyc_List_List*free_evars=({({struct Cyc_List_List*(*_tmp3D1)(void*(*f)(struct _tuple14*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp259)(void*(*f)(struct _tuple14*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(void*(*f)(struct _tuple14*),struct Cyc_List_List*x))Cyc_List_map;_tmp259;});void*(*_tmp3D0)(struct _tuple14*)=({void*(*_tmp25A)(struct _tuple14*)=(void*(*)(struct _tuple14*))Cyc_Core_fst;_tmp25A;});_tmp3D1(_tmp3D0,cvt.free_evars);});});
# 1429
if(te_vars != 0){
# 1431
struct Cyc_List_List*res=0;
# 1433
{struct Cyc_List_List*fs=free_tvars;for(0;(unsigned)fs;fs=fs->tl){
# 1435
struct Cyc_Absyn_Tvar*f=(struct Cyc_Absyn_Tvar*)fs->hd;
int found=0;
{struct Cyc_List_List*ts=te_vars;for(0;(unsigned)ts;ts=ts->tl){
if(({Cyc_Absyn_tvar_cmp(f,(struct Cyc_Absyn_Tvar*)ts->hd);})== 0){found=1;break;}}}
# 1437
if(!found)
# 1439
res=({struct Cyc_List_List*_tmp244=_cycalloc(sizeof(*_tmp244));_tmp244->hd=(struct Cyc_Absyn_Tvar*)fs->hd,_tmp244->tl=res;_tmp244;});}}
# 1441
free_tvars=({Cyc_List_imp_rev(res);});}
# 1429
{
# 1447
struct Cyc_List_List*x=free_tvars;
# 1429
for(0;x != 0;x=x->tl){
# 1448
void*_stmttmp2E=({Cyc_Absyn_compress_kb(((struct Cyc_Absyn_Tvar*)x->hd)->kind);});void*_tmp245=_stmttmp2E;enum Cyc_Absyn_AliasQual _tmp246;struct Cyc_Absyn_Kind*_tmp248;struct Cyc_Core_Opt**_tmp247;struct Cyc_Core_Opt**_tmp249;struct Cyc_Core_Opt**_tmp24A;struct Cyc_Core_Opt**_tmp24B;struct Cyc_Core_Opt**_tmp24C;struct Cyc_Core_Opt**_tmp24D;struct Cyc_Core_Opt**_tmp24E;switch(*((int*)_tmp245)){case 1U: _LL1: _tmp24E=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*)_tmp245)->f1;_LL2: {struct Cyc_Core_Opt**f=_tmp24E;
_tmp24D=f;goto _LL4;}case 2U: switch(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp245)->f2)->kind){case Cyc_Absyn_BoxKind: if(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp245)->f2)->aliasqual == Cyc_Absyn_Top){_LL3: _tmp24D=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp245)->f1;_LL4: {struct Cyc_Core_Opt**f=_tmp24D;
_tmp24C=f;goto _LL6;}}else{goto _LLD;}case Cyc_Absyn_MemKind: switch(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp245)->f2)->aliasqual){case Cyc_Absyn_Top: _LL5: _tmp24C=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp245)->f1;_LL6: {struct Cyc_Core_Opt**f=_tmp24C;
_tmp24B=f;goto _LL8;}case Cyc_Absyn_Aliasable: _LL7: _tmp24B=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp245)->f1;_LL8: {struct Cyc_Core_Opt**f=_tmp24B;
# 1453
({struct Cyc_Core_Opt*_tmp3D2=({Cyc_Tcutil_kind_to_bound_opt(& Cyc_Tcutil_bk);});*f=_tmp3D2;});goto _LL0;}case Cyc_Absyn_Unique: _LL9: _tmp24A=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp245)->f1;_LLA: {struct Cyc_Core_Opt**f=_tmp24A;
# 1455
({struct Cyc_Core_Opt*_tmp3D3=({Cyc_Tcutil_kind_to_bound_opt(& Cyc_Tcutil_ubk);});*f=_tmp3D3;});goto _LL0;}default: goto _LLD;}case Cyc_Absyn_RgnKind: if(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp245)->f2)->aliasqual == Cyc_Absyn_Top){_LLB: _tmp249=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp245)->f1;_LLC: {struct Cyc_Core_Opt**f=_tmp249;
# 1457
({struct Cyc_Core_Opt*_tmp3D4=({Cyc_Tcutil_kind_to_bound_opt(& Cyc_Tcutil_rk);});*f=_tmp3D4;});goto _LL0;}}else{goto _LLD;}default: _LLD: _tmp247=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp245)->f1;_tmp248=((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp245)->f2;_LLE: {struct Cyc_Core_Opt**f=_tmp247;struct Cyc_Absyn_Kind*k=_tmp248;
# 1459
({struct Cyc_Core_Opt*_tmp3D5=({Cyc_Tcutil_kind_to_bound_opt(k);});*f=_tmp3D5;});goto _LL0;}}default: if(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp245)->f1)->kind == Cyc_Absyn_MemKind){_LLF: _tmp246=(((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp245)->f1)->aliasqual;_LL10: {enum Cyc_Absyn_AliasQual a=_tmp246;
# 1461
({struct Cyc_String_pa_PrintArg_struct _tmp251=({struct Cyc_String_pa_PrintArg_struct _tmp2AB;_tmp2AB.tag=0U,({
struct _fat_ptr _tmp3D6=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_tvar2string((struct Cyc_Absyn_Tvar*)x->hd);}));_tmp2AB.f1=_tmp3D6;});_tmp2AB;});struct Cyc_String_pa_PrintArg_struct _tmp252=({struct Cyc_String_pa_PrintArg_struct _tmp2AA;_tmp2AA.tag=0U,({struct _fat_ptr _tmp3D7=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_kind2string(({struct Cyc_Absyn_Kind*_tmp253=_cycalloc(sizeof(*_tmp253));_tmp253->kind=Cyc_Absyn_MemKind,_tmp253->aliasqual=a;_tmp253;}));}));_tmp2AA.f1=_tmp3D7;});_tmp2AA;});void*_tmp24F[2U];_tmp24F[0]=& _tmp251,_tmp24F[1]=& _tmp252;({unsigned _tmp3D9=loc;struct _fat_ptr _tmp3D8=({const char*_tmp250="type variable %s cannot have kind %s";_tag_fat(_tmp250,sizeof(char),37U);});Cyc_Tcutil_terr(_tmp3D9,_tmp3D8,_tag_fat(_tmp24F,sizeof(void*),2U));});});
goto _LL0;}}else{_LL11: _LL12:
 goto _LL0;}}_LL0:;}}
# 1468
if(free_tvars != 0 || free_evars != 0){
{void*_stmttmp2F=({Cyc_Tcutil_compress(t);});void*_tmp254=_stmttmp2F;struct Cyc_List_List**_tmp255;if(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp254)->tag == 5U){_LL14: _tmp255=(struct Cyc_List_List**)&(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp254)->f1).tvars;_LL15: {struct Cyc_List_List**btvs=_tmp255;
# 1471
if(*btvs == 0){
# 1473
({struct Cyc_List_List*_tmp3DA=({Cyc_List_copy(free_tvars);});*btvs=_tmp3DA;});
free_tvars=0;}
# 1471
goto _LL13;}}else{_LL16: _LL17:
# 1477
 goto _LL13;}_LL13:;}
# 1479
if(free_tvars != 0)
({struct Cyc_String_pa_PrintArg_struct _tmp258=({struct Cyc_String_pa_PrintArg_struct _tmp2AC;_tmp2AC.tag=0U,_tmp2AC.f1=(struct _fat_ptr)((struct _fat_ptr)*((struct Cyc_Absyn_Tvar*)free_tvars->hd)->name);_tmp2AC;});void*_tmp256[1U];_tmp256[0]=& _tmp258;({unsigned _tmp3DC=loc;struct _fat_ptr _tmp3DB=({const char*_tmp257="unbound type variable %s";_tag_fat(_tmp257,sizeof(char),25U);});Cyc_Tcutil_terr(_tmp3DC,_tmp3DB,_tag_fat(_tmp256,sizeof(void*),1U));});});
# 1479
if(
# 1481
!({Cyc_Tcutil_is_function_type(t);})|| !te->in_tempest)
({Cyc_Tctyp_check_free_evars(free_evars,t,loc);});}}}
# 1490
void Cyc_Tctyp_check_fndecl_valid_type(unsigned loc,struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Fndecl*fd){
void*t=({Cyc_Tcutil_fndecl2type(fd);});
# 1493
({Cyc_Tctyp_check_valid_toplevel_type(loc,te,t);});
{void*_stmttmp30=({Cyc_Tcutil_compress(t);});void*_tmp25E=_stmttmp30;struct Cyc_Absyn_FnInfo _tmp25F;if(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp25E)->tag == 5U){_LL1: _tmp25F=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp25E)->f1;_LL2: {struct Cyc_Absyn_FnInfo i=_tmp25F;
# 1496
struct Cyc_List_List*atts=(fd->i).attributes;
fd->i=i;
(fd->i).attributes=atts;
({int _tmp3DD=({Cyc_Tcutil_extract_const_from_typedef(loc,(i.ret_tqual).print_const,i.ret_type);});((fd->i).ret_tqual).real_const=_tmp3DD;});
# 1501
goto _LL0;}}else{_LL3: _LL4:
({void*_tmp260=0U;({int(*_tmp3DF)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp262)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp262;});struct _fat_ptr _tmp3DE=({const char*_tmp261="check_fndecl_valid_type: not a FnType";_tag_fat(_tmp261,sizeof(char),38U);});_tmp3DF(_tmp3DE,_tag_fat(_tmp260,sizeof(void*),0U));});});}_LL0:;}
# 1504
({void(*_tmp263)(struct Cyc_List_List*,unsigned,struct _fat_ptr err_msg)=Cyc_Tcutil_check_unique_vars;struct Cyc_List_List*_tmp264=({({struct Cyc_List_List*(*_tmp3E0)(struct _fat_ptr*(*f)(struct _tuple10*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp265)(struct _fat_ptr*(*f)(struct _tuple10*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct _fat_ptr*(*f)(struct _tuple10*),struct Cyc_List_List*x))Cyc_List_map;_tmp265;});_tmp3E0(Cyc_Tctyp_fst_fdarg,(fd->i).args);});});unsigned _tmp266=loc;struct _fat_ptr _tmp267=({const char*_tmp268="function declaration has repeated parameter";_tag_fat(_tmp268,sizeof(char),44U);});_tmp263(_tmp264,_tmp266,_tmp267);});
# 1507
fd->cached_type=t;}
# 1512
void Cyc_Tctyp_check_type(unsigned loc,struct Cyc_Tcenv_Tenv*te,struct Cyc_List_List*bound_tvars,struct Cyc_Absyn_Kind*expected_kind,int allow_evars,int allow_abs_aggr,void*t){
# 1516
struct Cyc_Tctyp_CVTEnv cvt=({({unsigned _tmp3E5=loc;struct Cyc_Tcenv_Tenv*_tmp3E4=te;struct Cyc_Tctyp_CVTEnv _tmp3E3=({struct Cyc_Tctyp_CVTEnv _tmp2B0;_tmp2B0.r=Cyc_Core_heap_region,_tmp2B0.kind_env=bound_tvars,_tmp2B0.fn_result=0,_tmp2B0.generalize_evars=0,_tmp2B0.free_vars=0,_tmp2B0.free_evars=0;_tmp2B0;});struct Cyc_Absyn_Kind*_tmp3E2=expected_kind;int _tmp3E1=allow_abs_aggr;Cyc_Tctyp_check_valid_type(_tmp3E5,_tmp3E4,_tmp3E3,_tmp3E2,_tmp3E1,t);});});
# 1520
struct Cyc_List_List*free_tvars=({struct Cyc_List_List*(*_tmp270)(struct _RegionHandle*rgn,struct Cyc_List_List*tvs,struct Cyc_List_List*btvs)=Cyc_Tctyp_remove_bound_tvars;struct _RegionHandle*_tmp271=Cyc_Core_heap_region;struct Cyc_List_List*_tmp272=({({struct Cyc_List_List*(*_tmp3E7)(struct Cyc_Absyn_Tvar*(*f)(struct _tuple15*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp273)(struct Cyc_Absyn_Tvar*(*f)(struct _tuple15*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_Tvar*(*f)(struct _tuple15*),struct Cyc_List_List*x))Cyc_List_map;_tmp273;});struct Cyc_Absyn_Tvar*(*_tmp3E6)(struct _tuple15*)=({struct Cyc_Absyn_Tvar*(*_tmp274)(struct _tuple15*)=(struct Cyc_Absyn_Tvar*(*)(struct _tuple15*))Cyc_Core_fst;_tmp274;});_tmp3E7(_tmp3E6,cvt.free_vars);});});struct Cyc_List_List*_tmp275=bound_tvars;_tmp270(_tmp271,_tmp272,_tmp275);});
# 1522
struct Cyc_List_List*free_evars=({({struct Cyc_List_List*(*_tmp3E9)(void*(*f)(struct _tuple14*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp26E)(void*(*f)(struct _tuple14*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(void*(*f)(struct _tuple14*),struct Cyc_List_List*x))Cyc_List_map;_tmp26E;});void*(*_tmp3E8)(struct _tuple14*)=({void*(*_tmp26F)(struct _tuple14*)=(void*(*)(struct _tuple14*))Cyc_Core_fst;_tmp26F;});_tmp3E9(_tmp3E8,cvt.free_evars);});});
{struct Cyc_List_List*fs=free_tvars;for(0;fs != 0;fs=fs->tl){
({struct Cyc_String_pa_PrintArg_struct _tmp26C=({struct Cyc_String_pa_PrintArg_struct _tmp2AF;_tmp2AF.tag=0U,_tmp2AF.f1=(struct _fat_ptr)((struct _fat_ptr)*((struct Cyc_Absyn_Tvar*)fs->hd)->name);_tmp2AF;});struct Cyc_String_pa_PrintArg_struct _tmp26D=({struct Cyc_String_pa_PrintArg_struct _tmp2AE;_tmp2AE.tag=0U,({struct _fat_ptr _tmp3EA=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t);}));_tmp2AE.f1=_tmp3EA;});_tmp2AE;});void*_tmp26A[2U];_tmp26A[0]=& _tmp26C,_tmp26A[1]=& _tmp26D;({unsigned _tmp3EC=loc;struct _fat_ptr _tmp3EB=({const char*_tmp26B="unbound type variable %s in type %s";_tag_fat(_tmp26B,sizeof(char),36U);});Cyc_Tcutil_terr(_tmp3EC,_tmp3EB,_tag_fat(_tmp26A,sizeof(void*),2U));});});}}
if(!allow_evars)
({Cyc_Tctyp_check_free_evars(free_evars,t,loc);});}

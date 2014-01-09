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
 struct Cyc_Core_Opt{void*v;};extern char Cyc_Core_Invalid_argument[17U];struct Cyc_Core_Invalid_argument_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Failure[8U];struct Cyc_Core_Failure_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Impossible[11U];struct Cyc_Core_Impossible_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Not_found[10U];struct Cyc_Core_Not_found_exn_struct{char*tag;};extern char Cyc_Core_Unreachable[12U];struct Cyc_Core_Unreachable_exn_struct{char*tag;struct _fat_ptr f1;};
# 168 "core.h"
extern struct _RegionHandle*Cyc_Core_unique_region;struct Cyc_Core_DynamicRegion;struct Cyc_Core_NewDynamicRegion{struct Cyc_Core_DynamicRegion*key;};struct Cyc_Core_ThinRes{void*arr;unsigned nelts;};struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};extern char Cyc_List_List_mismatch[14U];struct Cyc_List_List_mismatch_exn_struct{char*tag;};
# 172 "list.h"
struct Cyc_List_List*Cyc_List_rev(struct Cyc_List_List*x);
# 178
struct Cyc_List_List*Cyc_List_imp_rev(struct Cyc_List_List*x);
# 184
struct Cyc_List_List*Cyc_List_append(struct Cyc_List_List*x,struct Cyc_List_List*y);extern char Cyc_List_Nth[4U];struct Cyc_List_Nth_exn_struct{char*tag;};
# 322
int Cyc_List_mem(int(*compare)(void*,void*),struct Cyc_List_List*l,void*x);struct Cyc___cycFILE;struct Cyc_String_pa_PrintArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Int_pa_PrintArg_struct{int tag;unsigned long f1;};struct Cyc_Double_pa_PrintArg_struct{int tag;double f1;};struct Cyc_LongDouble_pa_PrintArg_struct{int tag;long double f1;};struct Cyc_ShortPtr_pa_PrintArg_struct{int tag;short*f1;};struct Cyc_IntPtr_pa_PrintArg_struct{int tag;unsigned long*f1;};struct Cyc_ShortPtr_sa_ScanfArg_struct{int tag;short*f1;};struct Cyc_UShortPtr_sa_ScanfArg_struct{int tag;unsigned short*f1;};struct Cyc_IntPtr_sa_ScanfArg_struct{int tag;int*f1;};struct Cyc_UIntPtr_sa_ScanfArg_struct{int tag;unsigned*f1;};struct Cyc_StringPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_DoublePtr_sa_ScanfArg_struct{int tag;double*f1;};struct Cyc_FloatPtr_sa_ScanfArg_struct{int tag;float*f1;};struct Cyc_CharPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};extern char Cyc_FileCloseError[15U];struct Cyc_FileCloseError_exn_struct{char*tag;};extern char Cyc_FileOpenError[14U];struct Cyc_FileOpenError_exn_struct{char*tag;struct _fat_ptr f1;};
# 50 "string.h"
int Cyc_strptrcmp(struct _fat_ptr*s1,struct _fat_ptr*s2);
# 66
struct _fat_ptr Cyc_str_sepstr(struct Cyc_List_List*,struct _fat_ptr);struct Cyc_Iter_Iter{void*env;int(*next)(void*env,void*dest);};struct Cyc_Dict_T;struct Cyc_Dict_Dict{int(*rel)(void*,void*);struct _RegionHandle*r;const struct Cyc_Dict_T*t;};extern char Cyc_Dict_Present[8U];struct Cyc_Dict_Present_exn_struct{char*tag;};extern char Cyc_Dict_Absent[7U];struct Cyc_Dict_Absent_exn_struct{char*tag;};
# 62 "dict.h"
struct Cyc_Dict_Dict Cyc_Dict_empty(int(*cmp)(void*,void*));
# 83
int Cyc_Dict_member(struct Cyc_Dict_Dict d,void*k);
# 87
struct Cyc_Dict_Dict Cyc_Dict_insert(struct Cyc_Dict_Dict d,void*k,void*v);
# 101
struct Cyc_Dict_Dict Cyc_Dict_singleton(int(*cmp)(void*,void*),void*k,void*v);
# 110
void*Cyc_Dict_lookup(struct Cyc_Dict_Dict d,void*k);
# 149 "dict.h"
void Cyc_Dict_iter_c(void(*f)(void*,void*,void*),void*env,struct Cyc_Dict_Dict d);
# 257
struct Cyc_Dict_Dict Cyc_Dict_difference(struct Cyc_Dict_Dict d1,struct Cyc_Dict_Dict d2);struct Cyc_Set_Set;extern char Cyc_Set_Absent[7U];struct Cyc_Set_Absent_exn_struct{char*tag;};struct Cyc_Position_Error;struct Cyc_Relations_Reln;struct _union_Nmspace_Rel_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Abs_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_C_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Loc_n{int tag;int val;};union Cyc_Absyn_Nmspace{struct _union_Nmspace_Rel_n Rel_n;struct _union_Nmspace_Abs_n Abs_n;struct _union_Nmspace_C_n C_n;struct _union_Nmspace_Loc_n Loc_n;};
# 110 "absyn.h"
extern union Cyc_Absyn_Nmspace Cyc_Absyn_Loc_n;
union Cyc_Absyn_Nmspace Cyc_Absyn_Rel_n(struct Cyc_List_List*);
# 113
union Cyc_Absyn_Nmspace Cyc_Absyn_Abs_n(struct Cyc_List_List*ns,int C_scope);struct _tuple0{union Cyc_Absyn_Nmspace f1;struct _fat_ptr*f2;};
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
enum Cyc_Absyn_KindQual{Cyc_Absyn_AnyKind =0U,Cyc_Absyn_MemKind =1U,Cyc_Absyn_BoxKind =2U,Cyc_Absyn_XRgnKind =3U,Cyc_Absyn_RgnKind =4U,Cyc_Absyn_EffKind =5U,Cyc_Absyn_IntKind =6U,Cyc_Absyn_BoolKind =7U,Cyc_Absyn_PtrBndKind =8U};struct Cyc_Absyn_Kind{enum Cyc_Absyn_KindQual kind;enum Cyc_Absyn_AliasQual aliasqual;};struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct{int tag;struct Cyc_Absyn_Kind*f1;};struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct{int tag;struct Cyc_Core_Opt*f1;};struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct{int tag;struct Cyc_Core_Opt*f1;struct Cyc_Absyn_Kind*f2;};struct Cyc_Absyn_Tvar{struct _fat_ptr*name;int identity;void*kind;};struct Cyc_Absyn_PtrLoc{unsigned ptr_loc;unsigned rgn_loc;unsigned zt_loc;};struct Cyc_Absyn_PtrAtts{void*rgn;void*nullable;void*bounds;void*zero_term;struct Cyc_Absyn_PtrLoc*ptrloc;};struct Cyc_Absyn_PtrInfo{void*elt_type;struct Cyc_Absyn_Tqual elt_tq;struct Cyc_Absyn_PtrAtts ptr_atts;};struct Cyc_Absyn_VarargInfo{struct _fat_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;int inject;};struct Cyc_Absyn_Star_Absyn_Cap_struct{int tag;};struct Cyc_Absyn_Bot_Absyn_Cap_struct{int tag;};struct Cyc_Absyn_Nat_Absyn_Cap_struct{int tag;int f1;int f2;};struct Cyc_Absyn_RgnEffect{void*name;struct Cyc_List_List*caps;void*parent;};struct Cyc_Absyn_FnInfo{struct Cyc_List_List*tvars;void*effect;struct Cyc_Absyn_Tqual ret_tqual;void*ret_type;struct Cyc_List_List*args;int c_varargs;struct Cyc_Absyn_VarargInfo*cyc_varargs;struct Cyc_List_List*rgn_po;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*requires_clause;struct Cyc_List_List*requires_relns;struct Cyc_Absyn_Exp*ensures_clause;struct Cyc_List_List*ensures_relns;struct Cyc_List_List*ieffect;struct Cyc_List_List*oeffect;struct Cyc_List_List*throws;int reentrant;};struct Cyc_Absyn_UnknownDatatypeInfo{struct _tuple0*name;int is_extensible;};struct _union_DatatypeInfo_UnknownDatatype{int tag;struct Cyc_Absyn_UnknownDatatypeInfo val;};struct _union_DatatypeInfo_KnownDatatype{int tag;struct Cyc_Absyn_Datatypedecl**val;};union Cyc_Absyn_DatatypeInfo{struct _union_DatatypeInfo_UnknownDatatype UnknownDatatype;struct _union_DatatypeInfo_KnownDatatype KnownDatatype;};
# 346
union Cyc_Absyn_DatatypeInfo Cyc_Absyn_UnknownDatatype(struct Cyc_Absyn_UnknownDatatypeInfo);struct Cyc_Absyn_UnknownDatatypeFieldInfo{struct _tuple0*datatype_name;struct _tuple0*field_name;int is_extensible;};struct _union_DatatypeFieldInfo_UnknownDatatypefield{int tag;struct Cyc_Absyn_UnknownDatatypeFieldInfo val;};struct _tuple1{struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;};struct _union_DatatypeFieldInfo_KnownDatatypefield{int tag;struct _tuple1 val;};union Cyc_Absyn_DatatypeFieldInfo{struct _union_DatatypeFieldInfo_UnknownDatatypefield UnknownDatatypefield;struct _union_DatatypeFieldInfo_KnownDatatypefield KnownDatatypefield;};
# 359
union Cyc_Absyn_DatatypeFieldInfo Cyc_Absyn_UnknownDatatypefield(struct Cyc_Absyn_UnknownDatatypeFieldInfo);struct _tuple2{enum Cyc_Absyn_AggrKind f1;struct _tuple0*f2;struct Cyc_Core_Opt*f3;};struct _union_AggrInfo_UnknownAggr{int tag;struct _tuple2 val;};struct _union_AggrInfo_KnownAggr{int tag;struct Cyc_Absyn_Aggrdecl**val;};union Cyc_Absyn_AggrInfo{struct _union_AggrInfo_UnknownAggr UnknownAggr;struct _union_AggrInfo_KnownAggr KnownAggr;};
# 366
union Cyc_Absyn_AggrInfo Cyc_Absyn_UnknownAggr(enum Cyc_Absyn_AggrKind,struct _tuple0*,struct Cyc_Core_Opt*);
union Cyc_Absyn_AggrInfo Cyc_Absyn_KnownAggr(struct Cyc_Absyn_Aggrdecl**);struct Cyc_Absyn_ArrayInfo{void*elt_type;struct Cyc_Absyn_Tqual tq;struct Cyc_Absyn_Exp*num_elts;void*zero_term;unsigned zt_loc;};struct Cyc_Absyn_Aggr_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Enum_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;};struct Cyc_Absyn_Datatype_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;};struct Cyc_Absyn_TypeDecl{void*r;unsigned loc;};struct Cyc_Absyn_VoidCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_IntCon_Absyn_TyCon_struct{int tag;enum Cyc_Absyn_Sign f1;enum Cyc_Absyn_Size_of f2;};struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct{int tag;int f1;};struct Cyc_Absyn_RgnHandleCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_XRgnHandleCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_TagCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_HeapCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_UniqueCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_RefCntCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_AccessCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_CAccessCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_JoinCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_RgnsCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_TrueCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_FalseCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_ThinCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_FatCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct{int tag;struct _tuple0*f1;struct Cyc_Absyn_Enumdecl*f2;};struct Cyc_Absyn_AnonEnumCon_Absyn_TyCon_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_BuiltinCon_Absyn_TyCon_struct{int tag;struct _fat_ptr f1;struct Cyc_Absyn_Kind*f2;};struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_DatatypeInfo f1;};struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_DatatypeFieldInfo f1;};struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_AggrInfo f1;};struct Cyc_Absyn_AppType_Absyn_Type_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Evar_Absyn_Type_struct{int tag;struct Cyc_Core_Opt*f1;void*f2;int f3;struct Cyc_Core_Opt*f4;};struct Cyc_Absyn_VarType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Tvar*f1;};struct Cyc_Absyn_PointerType_Absyn_Type_struct{int tag;struct Cyc_Absyn_PtrInfo f1;};struct Cyc_Absyn_ArrayType_Absyn_Type_struct{int tag;struct Cyc_Absyn_ArrayInfo f1;};struct Cyc_Absyn_FnType_Absyn_Type_struct{int tag;struct Cyc_Absyn_FnInfo f1;};struct Cyc_Absyn_TupleType_Absyn_Type_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct{int tag;enum Cyc_Absyn_AggrKind f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_TypedefType_Absyn_Type_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_Typedefdecl*f3;void*f4;};struct Cyc_Absyn_ValueofType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct{int tag;struct Cyc_Absyn_TypeDecl*f1;void**f2;};struct Cyc_Absyn_TypeofType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_NoTypes_Absyn_Funcparams_struct{int tag;struct Cyc_List_List*f1;unsigned f2;};struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct{int tag;struct Cyc_List_List*f1;int f2;struct Cyc_Absyn_VarargInfo*f3;void*f4;struct Cyc_List_List*f5;struct Cyc_Absyn_Exp*f6;struct Cyc_Absyn_Exp*f7;struct Cyc_List_List*f8;struct Cyc_List_List*f9;struct Cyc_List_List*f10;int f11;};
# 478 "absyn.h"
enum Cyc_Absyn_Format_Type{Cyc_Absyn_Printf_ft =0U,Cyc_Absyn_Scanf_ft =1U};struct Cyc_Absyn_Regparm_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Stdcall_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Cdecl_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Fastcall_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Noreturn_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Const_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Aligned_att_Absyn_Attribute_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Packed_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Section_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Nocommon_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Shared_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Unused_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Weak_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Dllimport_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Dllexport_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_instrument_function_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Constructor_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Destructor_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_check_memory_usage_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Format_att_Absyn_Attribute_struct{int tag;enum Cyc_Absyn_Format_Type f1;int f2;int f3;};struct Cyc_Absyn_Initializes_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Noliveunique_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Consume_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Pure_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Mode_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Alias_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Always_inline_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_throw_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Non_null_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Deprecated_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_VectorSize_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Carray_mod_Absyn_Type_modifier_struct{int tag;void*f1;unsigned f2;};struct Cyc_Absyn_ConstArray_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;unsigned f3;};struct Cyc_Absyn_Pointer_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_Absyn_PtrAtts f1;struct Cyc_Absyn_Tqual f2;};struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct{int tag;void*f1;};struct Cyc_Absyn_TypeParams_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_List_List*f1;unsigned f2;int f3;};struct Cyc_Absyn_Attributes_mod_Absyn_Type_modifier_struct{int tag;unsigned f1;struct Cyc_List_List*f2;};struct _union_Cnst_Null_c{int tag;int val;};struct _tuple3{enum Cyc_Absyn_Sign f1;char f2;};struct _union_Cnst_Char_c{int tag;struct _tuple3 val;};struct _union_Cnst_Wchar_c{int tag;struct _fat_ptr val;};struct _tuple4{enum Cyc_Absyn_Sign f1;short f2;};struct _union_Cnst_Short_c{int tag;struct _tuple4 val;};struct _tuple5{enum Cyc_Absyn_Sign f1;int f2;};struct _union_Cnst_Int_c{int tag;struct _tuple5 val;};struct _tuple6{enum Cyc_Absyn_Sign f1;long long f2;};struct _union_Cnst_LongLong_c{int tag;struct _tuple6 val;};struct _tuple7{struct _fat_ptr f1;int f2;};struct _union_Cnst_Float_c{int tag;struct _tuple7 val;};struct _union_Cnst_String_c{int tag;struct _fat_ptr val;};struct _union_Cnst_Wstring_c{int tag;struct _fat_ptr val;};union Cyc_Absyn_Cnst{struct _union_Cnst_Null_c Null_c;struct _union_Cnst_Char_c Char_c;struct _union_Cnst_Wchar_c Wchar_c;struct _union_Cnst_Short_c Short_c;struct _union_Cnst_Int_c Int_c;struct _union_Cnst_LongLong_c LongLong_c;struct _union_Cnst_Float_c Float_c;struct _union_Cnst_String_c String_c;struct _union_Cnst_Wstring_c Wstring_c;};
# 574
enum Cyc_Absyn_Primop{Cyc_Absyn_Plus =0U,Cyc_Absyn_Times =1U,Cyc_Absyn_Minus =2U,Cyc_Absyn_Div =3U,Cyc_Absyn_Mod =4U,Cyc_Absyn_Eq =5U,Cyc_Absyn_Neq =6U,Cyc_Absyn_Gt =7U,Cyc_Absyn_Lt =8U,Cyc_Absyn_Gte =9U,Cyc_Absyn_Lte =10U,Cyc_Absyn_Not =11U,Cyc_Absyn_Bitnot =12U,Cyc_Absyn_Bitand =13U,Cyc_Absyn_Bitor =14U,Cyc_Absyn_Bitxor =15U,Cyc_Absyn_Bitlshift =16U,Cyc_Absyn_Bitlrshift =17U,Cyc_Absyn_Numelts =18U};
# 581
enum Cyc_Absyn_Incrementor{Cyc_Absyn_PreInc =0U,Cyc_Absyn_PostInc =1U,Cyc_Absyn_PreDec =2U,Cyc_Absyn_PostDec =3U};struct Cyc_Absyn_VarargCallInfo{int num_varargs;struct Cyc_List_List*injectors;struct Cyc_Absyn_VarargInfo*vai;};struct Cyc_Absyn_StructField_Absyn_OffsetofField_struct{int tag;struct _fat_ptr*f1;};struct Cyc_Absyn_TupleIndex_Absyn_OffsetofField_struct{int tag;unsigned f1;};
# 599
enum Cyc_Absyn_Coercion{Cyc_Absyn_Unknown_coercion =0U,Cyc_Absyn_No_coercion =1U,Cyc_Absyn_Null_to_NonNull =2U,Cyc_Absyn_Other_coercion =3U};struct Cyc_Absyn_MallocInfo{int is_calloc;struct Cyc_Absyn_Exp*rgn;void**elt_type;struct Cyc_Absyn_Exp*num_elts;int fat_result;int inline_call;};struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct{int tag;union Cyc_Absyn_Cnst f1;};struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Pragma_e_Absyn_Raw_exp_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct{int tag;enum Cyc_Absyn_Primop f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;enum Cyc_Absyn_Incrementor f2;};struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct _tuple8{struct Cyc_List_List*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_VarargCallInfo*f3;int f4;struct _tuple8*f5;};struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;int f2;};struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Exp*f2;int f3;enum Cyc_Absyn_Coercion f4;};struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;int f3;int f4;};struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;int f3;int f4;};struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Tuple_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;};struct _tuple9{struct _fat_ptr*f1;struct Cyc_Absyn_Tqual f2;void*f3;};struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct{int tag;struct _tuple9*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;int f4;};struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;int f3;};struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct Cyc_Absyn_Aggrdecl*f4;};struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Datatypedecl*f2;struct Cyc_Absyn_Datatypefield*f3;};struct Cyc_Absyn_Enum_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_Spawn_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;struct _tuple8*f3;};struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_MallocInfo f1;};struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Core_Opt*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Stmt*f1;};struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;};struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct{int tag;int f1;struct _fat_ptr f2;struct Cyc_List_List*f3;struct Cyc_List_List*f4;struct Cyc_List_List*f5;};struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Exp{void*topt;void*r;unsigned loc;void*annot;};struct Cyc_Absyn_Skip_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;struct Cyc_Absyn_Stmt*f3;};struct _tuple10{struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct{int tag;struct _tuple10 f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Break_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Continue_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Goto_s_Absyn_Raw_stmt_struct{int tag;struct _fat_ptr*f1;};struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _tuple10 f2;struct _tuple10 f3;struct Cyc_Absyn_Stmt*f4;};struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;void*f3;};struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Switch_clause**f2;};struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Decl*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct{int tag;struct _fat_ptr*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct _tuple10 f2;};struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_List_List*f2;void*f3;};struct Cyc_Absyn_Stmt{void*r;unsigned loc;void*annot;};struct Cyc_Absyn_Wild_p_Absyn_Raw_pat_struct{int tag;};struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_AliasVar_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_TagInt_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Tuple_p_Absyn_Raw_pat_struct{int tag;struct Cyc_List_List*f1;int f2;};struct Cyc_Absyn_Pointer_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Pat*f1;};struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct{int tag;union Cyc_Absyn_AggrInfo*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;int f4;};struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;struct Cyc_List_List*f3;int f4;};struct Cyc_Absyn_Null_p_Absyn_Raw_pat_struct{int tag;};struct Cyc_Absyn_Int_p_Absyn_Raw_pat_struct{int tag;enum Cyc_Absyn_Sign f1;int f2;};struct Cyc_Absyn_Char_p_Absyn_Raw_pat_struct{int tag;char f1;};struct Cyc_Absyn_Float_p_Absyn_Raw_pat_struct{int tag;struct _fat_ptr f1;int f2;};struct Cyc_Absyn_Enum_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_AnonEnum_p_Absyn_Raw_pat_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_UnknownId_p_Absyn_Raw_pat_struct{int tag;struct _tuple0*f1;};struct Cyc_Absyn_UnknownCall_p_Absyn_Raw_pat_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;int f3;};struct Cyc_Absyn_Exp_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Exp*f1;};
# 771 "absyn.h"
extern struct Cyc_Absyn_Wild_p_Absyn_Raw_pat_struct Cyc_Absyn_Wild_p_val;struct Cyc_Absyn_Pat{void*r;void*topt;unsigned loc;};struct Cyc_Absyn_Switch_clause{struct Cyc_Absyn_Pat*pattern;struct Cyc_Core_Opt*pat_vars;struct Cyc_Absyn_Exp*where_clause;struct Cyc_Absyn_Stmt*body;unsigned loc;};struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct{int tag;struct _tuple0*f1;};struct Cyc_Absyn_Global_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Funname_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Param_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Local_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Pat_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Vardecl{enum Cyc_Absyn_Scope sc;struct _tuple0*name;unsigned varloc;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*initializer;void*rgn;struct Cyc_List_List*attributes;int escapes;};struct Cyc_Absyn_Fndecl{enum Cyc_Absyn_Scope sc;int is_inline;struct _tuple0*name;struct Cyc_Absyn_Stmt*body;struct Cyc_Absyn_FnInfo i;void*cached_type;struct Cyc_Core_Opt*param_vardecls;struct Cyc_Absyn_Vardecl*fn_vardecl;};struct Cyc_Absyn_Aggrfield{struct _fat_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*width;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*requires_clause;};struct Cyc_Absyn_AggrdeclImpl{struct Cyc_List_List*exist_vars;struct Cyc_List_List*rgn_po;struct Cyc_List_List*fields;int tagged;};struct Cyc_Absyn_Aggrdecl{enum Cyc_Absyn_AggrKind kind;enum Cyc_Absyn_Scope sc;struct _tuple0*name;struct Cyc_List_List*tvs;struct Cyc_Absyn_AggrdeclImpl*impl;struct Cyc_List_List*attributes;int expected_mem_kind;};struct Cyc_Absyn_Datatypefield{struct _tuple0*name;struct Cyc_List_List*typs;unsigned loc;enum Cyc_Absyn_Scope sc;};struct Cyc_Absyn_Datatypedecl{enum Cyc_Absyn_Scope sc;struct _tuple0*name;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*fields;int is_extensible;};struct Cyc_Absyn_Enumfield{struct _tuple0*name;struct Cyc_Absyn_Exp*tag;unsigned loc;};struct Cyc_Absyn_Enumdecl{enum Cyc_Absyn_Scope sc;struct _tuple0*name;struct Cyc_Core_Opt*fields;};struct Cyc_Absyn_Typedefdecl{struct _tuple0*name;struct Cyc_Absyn_Tqual tq;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*kind;void*defn;struct Cyc_List_List*atts;int extern_c;};struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Pat*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;void*f4;};struct Cyc_Absyn_Letv_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;};struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;};struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Typedefdecl*f1;};struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct{int tag;struct _fat_ptr*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_ExternC_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;};struct _tuple11{unsigned f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct _tuple11*f4;};struct Cyc_Absyn_Porton_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Portoff_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Tempeston_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Tempestoff_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Decl{void*r;unsigned loc;};struct Cyc_Absyn_ArrayElement_Absyn_Designator_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_FieldName_Absyn_Designator_struct{int tag;struct _fat_ptr*f1;};extern char Cyc_Absyn_EmptyAnnot[11U];struct Cyc_Absyn_EmptyAnnot_Absyn_AbsynAnnot_struct{char*tag;};
# 954
int Cyc_Absyn_qvar_cmp(struct _tuple0*,struct _tuple0*);
int Cyc_Absyn_varlist_cmp(struct Cyc_List_List*,struct Cyc_List_List*);
# 991
extern void*Cyc_Absyn_heap_rgn_type;
# 995
extern void*Cyc_Absyn_false_type;
# 997
extern void*Cyc_Absyn_void_type;
# 1013
struct Cyc_Absyn_Datatypedecl*Cyc_Absyn_exn_tud();
# 1042
void*Cyc_Absyn_fatptr_type(void*t,void*rgn,struct Cyc_Absyn_Tqual,void*zeroterm);
# 1149
struct Cyc_Absyn_Pat*Cyc_Absyn_new_pat(void*,unsigned);
# 1158
struct Cyc_Absyn_Vardecl*Cyc_Absyn_new_vardecl(unsigned varloc,struct _tuple0*,void*,struct Cyc_Absyn_Exp*init);
# 1346
void Cyc_Absyn_resolve_effects(unsigned loc,struct Cyc_List_List*in,struct Cyc_List_List*out);
# 1377
void Cyc_Absyn_resolve_throws(unsigned loc,struct Cyc_List_List*throws,void(*check)(unsigned loc,void*,struct _tuple0*),void*p);
# 35 "warn.h"
void Cyc_Warn_err(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap);
# 40
void*Cyc_Warn_impos(struct _fat_ptr fmt,struct _fat_ptr ap);struct Cyc_Warn_String_Warn_Warg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Warn_Exp_Warn_Warg_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Warn_Typ_Warn_Warg_struct{int tag;void*f1;};struct Cyc_Warn_Qvar_Warn_Warg_struct{int tag;struct _tuple0*f1;};
# 58
void Cyc_Warn_err2(unsigned,struct _fat_ptr);
# 60
void Cyc_Warn_warn2(unsigned,struct _fat_ptr);
# 29 "binding.h"
extern int Cyc_Binding_warn_override;struct Cyc_Binding_Namespace_Binding_NSDirective_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Binding_Using_Binding_NSDirective_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Binding_NSCtxt{struct Cyc_List_List*curr_ns;struct Cyc_List_List*availables;struct Cyc_Dict_Dict ns_data;};
# 46
struct Cyc_Binding_NSCtxt*Cyc_Binding_mt_nsctxt(void*,void*(*mkdata)(void*));
void Cyc_Binding_enter_ns(struct Cyc_Binding_NSCtxt*,struct _fat_ptr*,void*,void*(*mkdata)(void*));
struct Cyc_List_List*Cyc_Binding_enter_using(unsigned loc,struct Cyc_Binding_NSCtxt*ctxt,struct _tuple0*usename);
void Cyc_Binding_leave_ns(struct Cyc_Binding_NSCtxt*ctxt);
void Cyc_Binding_leave_using(struct Cyc_Binding_NSCtxt*ctxt);
struct Cyc_List_List*Cyc_Binding_resolve_rel_ns(unsigned,struct Cyc_Binding_NSCtxt*,struct Cyc_List_List*);
# 34 "cifc.h"
void Cyc_Cifc_user_overrides(unsigned,struct Cyc_List_List*ds,struct Cyc_List_List*ovrs);struct Cyc_PP_Ppstate;struct Cyc_PP_Out;struct Cyc_PP_Doc;struct Cyc_Absynpp_Params{int expand_typedefs;int qvar_to_Cids;int add_cyc_prefix;int to_VC;int decls_first;int rewrite_temp_tvars;int print_all_tvars;int print_all_kinds;int print_all_effects;int print_using_stmts;int print_externC_stmts;int print_full_evars;int print_zeroterm;int generate_line_directives;int use_curr_namespace;struct Cyc_List_List*curr_namespace;};char Cyc_Binding_BindingError[13U]="BindingError";struct Cyc_Binding_BindingError_exn_struct{char*tag;};
# 45 "binding.cyc"
int Cyc_Binding_warn_override=0;
# 59
struct Cyc_Binding_NSCtxt*Cyc_Binding_mt_nsctxt(void*env,void*(*mkdata)(void*)){
void*data=({mkdata(env);});
return({struct Cyc_Binding_NSCtxt*_tmp3=_cycalloc(sizeof(*_tmp3));_tmp3->curr_ns=0,({
struct Cyc_List_List*_tmp34B=({struct Cyc_List_List*_tmp1=_cycalloc(sizeof(*_tmp1));({void*_tmp34A=(void*)({struct Cyc_Binding_Namespace_Binding_NSDirective_struct*_tmp0=_cycalloc(sizeof(*_tmp0));_tmp0->tag=0U,_tmp0->f1=0;_tmp0;});_tmp1->hd=_tmp34A;}),_tmp1->tl=0;_tmp1;});_tmp3->availables=_tmp34B;}),({
struct Cyc_Dict_Dict _tmp349=({({struct Cyc_Dict_Dict(*_tmp348)(int(*cmp)(struct Cyc_List_List*,struct Cyc_List_List*),struct Cyc_List_List*k,void*v)=({struct Cyc_Dict_Dict(*_tmp2)(int(*cmp)(struct Cyc_List_List*,struct Cyc_List_List*),struct Cyc_List_List*k,void*v)=(struct Cyc_Dict_Dict(*)(int(*cmp)(struct Cyc_List_List*,struct Cyc_List_List*),struct Cyc_List_List*k,void*v))Cyc_Dict_singleton;_tmp2;});_tmp348(Cyc_Absyn_varlist_cmp,0,data);});});_tmp3->ns_data=_tmp349;});_tmp3;});}
# 65
void*Cyc_Binding_get_ns_data(struct Cyc_Binding_NSCtxt*ctxt,union Cyc_Absyn_Nmspace abs_ns){
union Cyc_Absyn_Nmspace _tmp5=abs_ns;struct Cyc_List_List*_tmp6;struct Cyc_List_List*_tmp7;switch((_tmp5.Abs_n).tag){case 3U: _LL1: _tmp7=(_tmp5.C_n).val;_LL2: {struct Cyc_List_List*vs=_tmp7;
_tmp6=vs;goto _LL4;}case 2U: _LL3: _tmp6=(_tmp5.Abs_n).val;_LL4: {struct Cyc_List_List*vs=_tmp6;
return({({void*(*_tmp34D)(struct Cyc_Dict_Dict d,struct Cyc_List_List*k)=({void*(*_tmp8)(struct Cyc_Dict_Dict d,struct Cyc_List_List*k)=(void*(*)(struct Cyc_Dict_Dict d,struct Cyc_List_List*k))Cyc_Dict_lookup;_tmp8;});struct Cyc_Dict_Dict _tmp34C=ctxt->ns_data;_tmp34D(_tmp34C,vs);});});}default: _LL5: _LL6:
({void*_tmp9=0U;({struct _fat_ptr _tmp34E=({const char*_tmpA="Binding:get_ns_data: relative ns";_tag_fat(_tmpA,sizeof(char),33U);});Cyc_Warn_impos(_tmp34E,_tag_fat(_tmp9,sizeof(void*),0U));});});}_LL0:;}
# 65
struct Cyc_List_List*Cyc_Binding_resolve_rel_ns(unsigned loc,struct Cyc_Binding_NSCtxt*ctxt,struct Cyc_List_List*rel_ns){
# 86 "binding.cyc"
struct Cyc_List_List*fullname=({Cyc_List_append(ctxt->curr_ns,rel_ns);});
if(({({int(*_tmp350)(struct Cyc_Dict_Dict d,struct Cyc_List_List*k)=({int(*_tmpC)(struct Cyc_Dict_Dict d,struct Cyc_List_List*k)=(int(*)(struct Cyc_Dict_Dict d,struct Cyc_List_List*k))Cyc_Dict_member;_tmpC;});struct Cyc_Dict_Dict _tmp34F=ctxt->ns_data;_tmp350(_tmp34F,fullname);});}))
return fullname;
# 87
{
# 89
struct Cyc_List_List*as=ctxt->availables;
# 87
for(0;as != 0;as=as->tl){
# 90
void*_stmttmp0=(void*)as->hd;void*_tmpD=_stmttmp0;struct Cyc_List_List*_tmpE;struct Cyc_List_List*_tmpF;if(((struct Cyc_Binding_Using_Binding_NSDirective_struct*)_tmpD)->tag == 1U){_LL1: _tmpF=((struct Cyc_Binding_Using_Binding_NSDirective_struct*)_tmpD)->f1;_LL2: {struct Cyc_List_List*ns=_tmpF;
_tmpE=ns;goto _LL4;}}else{_LL3: _tmpE=((struct Cyc_Binding_Namespace_Binding_NSDirective_struct*)_tmpD)->f1;_LL4: {struct Cyc_List_List*ns=_tmpE;
# 93
struct Cyc_List_List*fullname=({Cyc_List_append(ns,rel_ns);});
if(({({int(*_tmp352)(struct Cyc_Dict_Dict d,struct Cyc_List_List*k)=({int(*_tmp10)(struct Cyc_Dict_Dict d,struct Cyc_List_List*k)=(int(*)(struct Cyc_Dict_Dict d,struct Cyc_List_List*k))Cyc_Dict_member;_tmp10;});struct Cyc_Dict_Dict _tmp351=ctxt->ns_data;_tmp352(_tmp351,fullname);});}))
return fullname;
# 94
goto _LL0;}}_LL0:;}}
# 98
({struct Cyc_Warn_String_Warn_Warg_struct _tmp12=({struct Cyc_Warn_String_Warn_Warg_struct _tmp317;_tmp317.tag=0U,({struct _fat_ptr _tmp353=({const char*_tmp17="namespace ";_tag_fat(_tmp17,sizeof(char),11U);});_tmp317.f1=_tmp353;});_tmp317;});struct Cyc_Warn_String_Warn_Warg_struct _tmp13=({struct Cyc_Warn_String_Warn_Warg_struct _tmp316;_tmp316.tag=0U,({struct _fat_ptr _tmp355=(struct _fat_ptr)({({struct Cyc_List_List*_tmp354=rel_ns;Cyc_str_sepstr(_tmp354,({const char*_tmp16="::";_tag_fat(_tmp16,sizeof(char),3U);}));});});_tmp316.f1=_tmp355;});_tmp316;});struct Cyc_Warn_String_Warn_Warg_struct _tmp14=({struct Cyc_Warn_String_Warn_Warg_struct _tmp315;_tmp315.tag=0U,({struct _fat_ptr _tmp356=({const char*_tmp15=" not found";_tag_fat(_tmp15,sizeof(char),11U);});_tmp315.f1=_tmp356;});_tmp315;});void*_tmp11[3U];_tmp11[0]=& _tmp12,_tmp11[1]=& _tmp13,_tmp11[2]=& _tmp14;({unsigned _tmp357=loc;Cyc_Warn_err2(_tmp357,_tag_fat(_tmp11,sizeof(void*),3U));});});
(int)_throw(({struct Cyc_Binding_BindingError_exn_struct*_tmp18=_cycalloc(sizeof(*_tmp18));_tmp18->tag=Cyc_Binding_BindingError;_tmp18;}));}
# 103
void*Cyc_Binding_resolve_lookup(unsigned loc,struct Cyc_Binding_NSCtxt*ctxt,struct _tuple0*qv,void*(*lookup)(void*,struct _fat_ptr*)){
struct _fat_ptr*_tmp1B;union Cyc_Absyn_Nmspace _tmp1A;_LL1: _tmp1A=qv->f1;_tmp1B=qv->f2;_LL2: {union Cyc_Absyn_Nmspace ns=_tmp1A;struct _fat_ptr*v=_tmp1B;
union Cyc_Absyn_Nmspace _tmp1C=ns;struct Cyc_List_List*_tmp1D;struct Cyc_List_List*_tmp1E;struct Cyc_List_List*_tmp1F;switch((_tmp1C.Abs_n).tag){case 1U: if((_tmp1C.Rel_n).val == 0){_LL4: _LL5:
# 107
{struct _handler_cons _tmp20;_push_handler(& _tmp20);{int _tmp22=0;if(setjmp(_tmp20.handler))_tmp22=1;if(!_tmp22){{void*_tmp27=({void*(*_tmp23)(void*,struct _fat_ptr*)=lookup;void*_tmp24=({({void*(*_tmp359)(struct Cyc_Dict_Dict d,struct Cyc_List_List*k)=({void*(*_tmp25)(struct Cyc_Dict_Dict d,struct Cyc_List_List*k)=(void*(*)(struct Cyc_Dict_Dict d,struct Cyc_List_List*k))Cyc_Dict_lookup;_tmp25;});struct Cyc_Dict_Dict _tmp358=ctxt->ns_data;_tmp359(_tmp358,ctxt->curr_ns);});});struct _fat_ptr*_tmp26=v;_tmp23(_tmp24,_tmp26);});_npop_handler(0U);return _tmp27;};_pop_handler();}else{void*_tmp21=(void*)Cyc_Core_get_exn_thrown();void*_tmp28=_tmp21;void*_tmp29;if(((struct Cyc_Dict_Absent_exn_struct*)_tmp28)->tag == Cyc_Dict_Absent){_LLF: _LL10:
 goto _LLE;}else{_LL11: _tmp29=_tmp28;_LL12: {void*exn=_tmp29;(int)_rethrow(exn);}}_LLE:;}}}
{struct Cyc_List_List*as=ctxt->availables;for(0;as != 0;as=as->tl){
void*_stmttmp1=(void*)as->hd;void*_tmp2A=_stmttmp1;struct Cyc_List_List*_tmp2B;struct Cyc_List_List*_tmp2C;if(((struct Cyc_Binding_Using_Binding_NSDirective_struct*)_tmp2A)->tag == 1U){_LL14: _tmp2C=((struct Cyc_Binding_Using_Binding_NSDirective_struct*)_tmp2A)->f1;_LL15: {struct Cyc_List_List*ns=_tmp2C;
_tmp2B=ns;goto _LL17;}}else{_LL16: _tmp2B=((struct Cyc_Binding_Namespace_Binding_NSDirective_struct*)_tmp2A)->f1;_LL17: {struct Cyc_List_List*ns=_tmp2B;
# 113
{struct _handler_cons _tmp2D;_push_handler(& _tmp2D);{int _tmp2F=0;if(setjmp(_tmp2D.handler))_tmp2F=1;if(!_tmp2F){{void*_tmp34=({void*(*_tmp30)(void*,struct _fat_ptr*)=lookup;void*_tmp31=({({void*(*_tmp35B)(struct Cyc_Dict_Dict d,struct Cyc_List_List*k)=({void*(*_tmp32)(struct Cyc_Dict_Dict d,struct Cyc_List_List*k)=(void*(*)(struct Cyc_Dict_Dict d,struct Cyc_List_List*k))Cyc_Dict_lookup;_tmp32;});struct Cyc_Dict_Dict _tmp35A=ctxt->ns_data;_tmp35B(_tmp35A,ns);});});struct _fat_ptr*_tmp33=v;_tmp30(_tmp31,_tmp33);});_npop_handler(0U);return _tmp34;};_pop_handler();}else{void*_tmp2E=(void*)Cyc_Core_get_exn_thrown();void*_tmp35=_tmp2E;void*_tmp36;if(((struct Cyc_Dict_Absent_exn_struct*)_tmp35)->tag == Cyc_Dict_Absent){_LL19: _LL1A:
 goto _LL18;}else{_LL1B: _tmp36=_tmp35;_LL1C: {void*exn=_tmp36;(int)_rethrow(exn);}}_LL18:;}}}
goto _LL13;}}_LL13:;}}
# 117
(int)_throw(({struct Cyc_Binding_BindingError_exn_struct*_tmp37=_cycalloc(sizeof(*_tmp37));_tmp37->tag=Cyc_Binding_BindingError;_tmp37;}));}else{_LL6: _tmp1F=(_tmp1C.Rel_n).val;_LL7: {struct Cyc_List_List*ns=_tmp1F;
# 119
struct _handler_cons _tmp38;_push_handler(& _tmp38);{int _tmp3A=0;if(setjmp(_tmp38.handler))_tmp3A=1;if(!_tmp3A){
{struct Cyc_List_List*abs_ns=({Cyc_Binding_resolve_rel_ns(loc,ctxt,ns);});
void*_tmp3F=({void*(*_tmp3B)(void*,struct _fat_ptr*)=lookup;void*_tmp3C=({({void*(*_tmp35D)(struct Cyc_Dict_Dict d,struct Cyc_List_List*k)=({void*(*_tmp3D)(struct Cyc_Dict_Dict d,struct Cyc_List_List*k)=(void*(*)(struct Cyc_Dict_Dict d,struct Cyc_List_List*k))Cyc_Dict_lookup;_tmp3D;});struct Cyc_Dict_Dict _tmp35C=ctxt->ns_data;_tmp35D(_tmp35C,abs_ns);});});struct _fat_ptr*_tmp3E=v;_tmp3B(_tmp3C,_tmp3E);});_npop_handler(0U);return _tmp3F;}
# 120
;_pop_handler();}else{void*_tmp39=(void*)Cyc_Core_get_exn_thrown();void*_tmp40=_tmp39;void*_tmp41;if(((struct Cyc_Dict_Absent_exn_struct*)_tmp40)->tag == Cyc_Dict_Absent){_LL1E: _LL1F:
# 122
(int)_throw(({struct Cyc_Binding_BindingError_exn_struct*_tmp42=_cycalloc(sizeof(*_tmp42));_tmp42->tag=Cyc_Binding_BindingError;_tmp42;}));}else{_LL20: _tmp41=_tmp40;_LL21: {void*exn=_tmp41;(int)_rethrow(exn);}}_LL1D:;}}}}case 3U: _LL8: _tmp1E=(_tmp1C.C_n).val;_LL9: {struct Cyc_List_List*ns=_tmp1E;
_tmp1D=ns;goto _LLB;}case 2U: _LLA: _tmp1D=(_tmp1C.Abs_n).val;_LLB: {struct Cyc_List_List*ns=_tmp1D;
# 125
struct _handler_cons _tmp43;_push_handler(& _tmp43);{int _tmp45=0;if(setjmp(_tmp43.handler))_tmp45=1;if(!_tmp45){{void*_tmp4A=({void*(*_tmp46)(void*,struct _fat_ptr*)=lookup;void*_tmp47=({({void*(*_tmp35F)(struct Cyc_Dict_Dict d,struct Cyc_List_List*k)=({void*(*_tmp48)(struct Cyc_Dict_Dict d,struct Cyc_List_List*k)=(void*(*)(struct Cyc_Dict_Dict d,struct Cyc_List_List*k))Cyc_Dict_lookup;_tmp48;});struct Cyc_Dict_Dict _tmp35E=ctxt->ns_data;_tmp35F(_tmp35E,ns);});});struct _fat_ptr*_tmp49=v;_tmp46(_tmp47,_tmp49);});_npop_handler(0U);return _tmp4A;};_pop_handler();}else{void*_tmp44=(void*)Cyc_Core_get_exn_thrown();void*_tmp4B=_tmp44;void*_tmp4C;if(((struct Cyc_Dict_Absent_exn_struct*)_tmp4B)->tag == Cyc_Dict_Absent){_LL23: _LL24:
(int)_throw(({struct Cyc_Binding_BindingError_exn_struct*_tmp4D=_cycalloc(sizeof(*_tmp4D));_tmp4D->tag=Cyc_Binding_BindingError;_tmp4D;}));}else{_LL25: _tmp4C=_tmp4B;_LL26: {void*exn=_tmp4C;(int)_rethrow(exn);}}_LL22:;}}}default: _LLC: _LLD:
({void*_tmp4E=0U;({struct _fat_ptr _tmp360=({const char*_tmp4F="lookup local in global";_tag_fat(_tmp4F,sizeof(char),23U);});Cyc_Warn_impos(_tmp360,_tag_fat(_tmp4E,sizeof(void*),0U));});});}_LL3:;}}
# 131
void Cyc_Binding_enter_ns(struct Cyc_Binding_NSCtxt*ctxt,struct _fat_ptr*subname,void*env,void*(*mkdata)(void*)){
struct Cyc_List_List*ns=ctxt->curr_ns;
struct Cyc_List_List*ns2=({({struct Cyc_List_List*_tmp361=ns;Cyc_List_append(_tmp361,({struct Cyc_List_List*_tmp59=_cycalloc(sizeof(*_tmp59));_tmp59->hd=subname,_tmp59->tl=0;_tmp59;}));});});
if(!({({int(*_tmp363)(struct Cyc_Dict_Dict d,struct Cyc_List_List*k)=({int(*_tmp51)(struct Cyc_Dict_Dict d,struct Cyc_List_List*k)=(int(*)(struct Cyc_Dict_Dict d,struct Cyc_List_List*k))Cyc_Dict_member;_tmp51;});struct Cyc_Dict_Dict _tmp362=ctxt->ns_data;_tmp363(_tmp362,ns2);});}))
({struct Cyc_Dict_Dict _tmp364=({struct Cyc_Dict_Dict(*_tmp52)(struct Cyc_Dict_Dict d,struct Cyc_List_List*k,void*v)=({struct Cyc_Dict_Dict(*_tmp53)(struct Cyc_Dict_Dict d,struct Cyc_List_List*k,void*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct Cyc_List_List*k,void*v))Cyc_Dict_insert;_tmp53;});struct Cyc_Dict_Dict _tmp54=ctxt->ns_data;struct Cyc_List_List*_tmp55=ns2;void*_tmp56=({mkdata(env);});_tmp52(_tmp54,_tmp55,_tmp56);});ctxt->ns_data=_tmp364;});
# 134
ctxt->curr_ns=ns2;
# 137
({struct Cyc_List_List*_tmp366=({struct Cyc_List_List*_tmp58=_cycalloc(sizeof(*_tmp58));({void*_tmp365=(void*)({struct Cyc_Binding_Namespace_Binding_NSDirective_struct*_tmp57=_cycalloc(sizeof(*_tmp57));_tmp57->tag=0U,_tmp57->f1=ns2;_tmp57;});_tmp58->hd=_tmp365;}),_tmp58->tl=ctxt->availables;_tmp58;});ctxt->availables=_tmp366;});}
# 139
void Cyc_Binding_leave_ns(struct Cyc_Binding_NSCtxt*ctxt){
if(ctxt->availables == 0)
({void*_tmp5B=0U;({struct _fat_ptr _tmp367=({const char*_tmp5C="leaving topmost namespace";_tag_fat(_tmp5C,sizeof(char),26U);});Cyc_Warn_impos(_tmp367,_tag_fat(_tmp5B,sizeof(void*),0U));});});{
# 140
void*_stmttmp2=(void*)((struct Cyc_List_List*)_check_null(ctxt->availables))->hd;void*_tmp5D=_stmttmp2;if(((struct Cyc_Binding_Namespace_Binding_NSDirective_struct*)_tmp5D)->tag == 0U){_LL1: _LL2:
# 144
 ctxt->availables=((struct Cyc_List_List*)_check_null(ctxt->availables))->tl;
({struct Cyc_List_List*_tmp368=({struct Cyc_List_List*(*_tmp5E)(struct Cyc_List_List*x)=Cyc_List_rev;struct Cyc_List_List*_tmp5F=((struct Cyc_List_List*)_check_null(({Cyc_List_rev(ctxt->curr_ns);})))->tl;_tmp5E(_tmp5F);});ctxt->curr_ns=_tmp368;});
goto _LL0;}else{_LL3: _LL4:
# 148
({void*_tmp60=0U;({struct _fat_ptr _tmp369=({const char*_tmp61="leaving using as namespace";_tag_fat(_tmp61,sizeof(char),27U);});Cyc_Warn_impos(_tmp369,_tag_fat(_tmp60,sizeof(void*),0U));});});}_LL0:;}}
# 139
struct Cyc_List_List*Cyc_Binding_enter_using(unsigned loc,struct Cyc_Binding_NSCtxt*ctxt,struct _tuple0*usename){
# 152
struct _fat_ptr*_tmp64;union Cyc_Absyn_Nmspace _tmp63;_LL1: _tmp63=usename->f1;_tmp64=usename->f2;_LL2: {union Cyc_Absyn_Nmspace nsl=_tmp63;struct _fat_ptr*v=_tmp64;
struct Cyc_List_List*ns;
{union Cyc_Absyn_Nmspace _tmp65=nsl;struct Cyc_List_List*_tmp66;struct Cyc_List_List*_tmp67;switch((_tmp65.Loc_n).tag){case 1U: _LL4: _tmp67=(_tmp65.Rel_n).val;_LL5: {struct Cyc_List_List*vs=_tmp67;
# 156
ns=({struct Cyc_List_List*(*_tmp68)(unsigned loc,struct Cyc_Binding_NSCtxt*ctxt,struct Cyc_List_List*rel_ns)=Cyc_Binding_resolve_rel_ns;unsigned _tmp69=loc;struct Cyc_Binding_NSCtxt*_tmp6A=ctxt;struct Cyc_List_List*_tmp6B=({({struct Cyc_List_List*_tmp36A=vs;Cyc_List_append(_tmp36A,({struct Cyc_List_List*_tmp6C=_cycalloc(sizeof(*_tmp6C));_tmp6C->hd=v,_tmp6C->tl=0;_tmp6C;}));});});_tmp68(_tmp69,_tmp6A,_tmp6B);});{
struct Cyc_List_List*abs_vs=({struct Cyc_List_List*(*_tmp6D)(struct Cyc_List_List*x)=Cyc_List_rev;struct Cyc_List_List*_tmp6E=((struct Cyc_List_List*)_check_null(({Cyc_List_rev(ns);})))->tl;_tmp6D(_tmp6E);});
({union Cyc_Absyn_Nmspace _tmp36B=({Cyc_Absyn_Abs_n(abs_vs,0);});(*usename).f1=_tmp36B;});
goto _LL3;}}case 2U: _LL6: _tmp66=(_tmp65.Abs_n).val;_LL7: {struct Cyc_List_List*vs=_tmp66;
# 161
ns=({({struct Cyc_List_List*_tmp36C=vs;Cyc_List_append(_tmp36C,({struct Cyc_List_List*_tmp6F=_cycalloc(sizeof(*_tmp6F));_tmp6F->hd=v,_tmp6F->tl=0;_tmp6F;}));});});
goto _LL3;}case 4U: _LL8: _LL9:
({void*_tmp70=0U;({struct _fat_ptr _tmp36D=({const char*_tmp71="enter_using, Loc_n";_tag_fat(_tmp71,sizeof(char),19U);});Cyc_Warn_impos(_tmp36D,_tag_fat(_tmp70,sizeof(void*),0U));});});default: _LLA: _LLB:
({void*_tmp72=0U;({struct _fat_ptr _tmp36E=({const char*_tmp73="enter_using, C_n";_tag_fat(_tmp73,sizeof(char),17U);});Cyc_Warn_impos(_tmp36E,_tag_fat(_tmp72,sizeof(void*),0U));});});}_LL3:;}
# 166
({struct Cyc_List_List*_tmp370=({struct Cyc_List_List*_tmp75=_cycalloc(sizeof(*_tmp75));({void*_tmp36F=(void*)({struct Cyc_Binding_Using_Binding_NSDirective_struct*_tmp74=_cycalloc(sizeof(*_tmp74));_tmp74->tag=1U,_tmp74->f1=ns;_tmp74;});_tmp75->hd=_tmp36F;}),_tmp75->tl=ctxt->availables;_tmp75;});ctxt->availables=_tmp370;});
return ns;}}
# 169
void Cyc_Binding_leave_using(struct Cyc_Binding_NSCtxt*ctxt){
if(ctxt->availables == 0)
({void*_tmp77=0U;({struct _fat_ptr _tmp371=({const char*_tmp78="leaving topmost namespace as a using";_tag_fat(_tmp78,sizeof(char),37U);});Cyc_Warn_impos(_tmp371,_tag_fat(_tmp77,sizeof(void*),0U));});});{
# 170
void*_stmttmp3=(void*)((struct Cyc_List_List*)_check_null(ctxt->availables))->hd;void*_tmp79=_stmttmp3;if(((struct Cyc_Binding_Using_Binding_NSDirective_struct*)_tmp79)->tag == 1U){_LL1: _LL2:
# 173
 ctxt->availables=((struct Cyc_List_List*)_check_null(ctxt->availables))->tl;goto _LL0;}else{_LL3: _LL4:
({void*_tmp7A=0U;({struct _fat_ptr _tmp372=({const char*_tmp7B="leaving namespace as using";_tag_fat(_tmp7B,sizeof(char),27U);});Cyc_Warn_impos(_tmp372,_tag_fat(_tmp7A,sizeof(void*),0U));});});}_LL0:;}}struct Cyc_Binding_VarRes_Binding_Resolved_struct{int tag;void*f1;};struct Cyc_Binding_AggrRes_Binding_Resolved_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Binding_DatatypeRes_Binding_Resolved_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;};struct Cyc_Binding_EnumRes_Binding_Resolved_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Binding_AnonEnumRes_Binding_Resolved_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Binding_ResolveNSEnv{struct Cyc_Dict_Dict aggrdecls;struct Cyc_Dict_Dict datatypedecls;struct Cyc_Dict_Dict enumdecls;struct Cyc_Dict_Dict typedefs;struct Cyc_Dict_Dict ordinaries;};struct Cyc_Binding_Env{int in_cinclude;struct Cyc_Binding_NSCtxt*ns;struct Cyc_Dict_Dict*local_vars;};
# 213 "binding.cyc"
inline static int Cyc_Binding_in_cinclude(struct Cyc_Binding_Env*env){
return env->in_cinclude;}
# 216
inline static int Cyc_Binding_at_toplevel(struct Cyc_Binding_Env*env){
return env->local_vars == 0;}
# 219
static struct Cyc_Binding_ResolveNSEnv*Cyc_Binding_mt_renv(int ignore){
return({struct Cyc_Binding_ResolveNSEnv*_tmp84=_cycalloc(sizeof(*_tmp84));({struct Cyc_Dict_Dict _tmp377=({({struct Cyc_Dict_Dict(*_tmp7F)(int(*cmp)(struct _fat_ptr*,struct _fat_ptr*))=(struct Cyc_Dict_Dict(*)(int(*cmp)(struct _fat_ptr*,struct _fat_ptr*)))Cyc_Dict_empty;_tmp7F;})(Cyc_strptrcmp);});_tmp84->aggrdecls=_tmp377;}),({
struct Cyc_Dict_Dict _tmp376=({({struct Cyc_Dict_Dict(*_tmp80)(int(*cmp)(struct _fat_ptr*,struct _fat_ptr*))=(struct Cyc_Dict_Dict(*)(int(*cmp)(struct _fat_ptr*,struct _fat_ptr*)))Cyc_Dict_empty;_tmp80;})(Cyc_strptrcmp);});_tmp84->datatypedecls=_tmp376;}),({
struct Cyc_Dict_Dict _tmp375=({({struct Cyc_Dict_Dict(*_tmp81)(int(*cmp)(struct _fat_ptr*,struct _fat_ptr*))=(struct Cyc_Dict_Dict(*)(int(*cmp)(struct _fat_ptr*,struct _fat_ptr*)))Cyc_Dict_empty;_tmp81;})(Cyc_strptrcmp);});_tmp84->enumdecls=_tmp375;}),({
struct Cyc_Dict_Dict _tmp374=({({struct Cyc_Dict_Dict(*_tmp82)(int(*cmp)(struct _fat_ptr*,struct _fat_ptr*))=(struct Cyc_Dict_Dict(*)(int(*cmp)(struct _fat_ptr*,struct _fat_ptr*)))Cyc_Dict_empty;_tmp82;})(Cyc_strptrcmp);});_tmp84->typedefs=_tmp374;}),({
struct Cyc_Dict_Dict _tmp373=({({struct Cyc_Dict_Dict(*_tmp83)(int(*cmp)(struct _fat_ptr*,struct _fat_ptr*))=(struct Cyc_Dict_Dict(*)(int(*cmp)(struct _fat_ptr*,struct _fat_ptr*)))Cyc_Dict_empty;_tmp83;})(Cyc_strptrcmp);});_tmp84->ordinaries=_tmp373;});_tmp84;});}
# 227
static struct Cyc_Absyn_Aggrdecl*Cyc_Binding_lookup_aggrdecl(struct Cyc_Binding_ResolveNSEnv*renv,struct _fat_ptr*v){
return({({struct Cyc_Absyn_Aggrdecl*(*_tmp379)(struct Cyc_Dict_Dict d,struct _fat_ptr*k)=({struct Cyc_Absyn_Aggrdecl*(*_tmp86)(struct Cyc_Dict_Dict d,struct _fat_ptr*k)=(struct Cyc_Absyn_Aggrdecl*(*)(struct Cyc_Dict_Dict d,struct _fat_ptr*k))Cyc_Dict_lookup;_tmp86;});struct Cyc_Dict_Dict _tmp378=renv->aggrdecls;_tmp379(_tmp378,v);});});}
# 230
static struct Cyc_List_List*Cyc_Binding_lookup_datatypedecl(struct Cyc_Binding_ResolveNSEnv*renv,struct _fat_ptr*v){
return({({struct Cyc_List_List*(*_tmp37B)(struct Cyc_Dict_Dict d,struct _fat_ptr*k)=({struct Cyc_List_List*(*_tmp88)(struct Cyc_Dict_Dict d,struct _fat_ptr*k)=(struct Cyc_List_List*(*)(struct Cyc_Dict_Dict d,struct _fat_ptr*k))Cyc_Dict_lookup;_tmp88;});struct Cyc_Dict_Dict _tmp37A=renv->datatypedecls;_tmp37B(_tmp37A,v);});});}
# 233
static struct Cyc_Absyn_Enumdecl*Cyc_Binding_lookup_enumdecl(struct Cyc_Binding_ResolveNSEnv*renv,struct _fat_ptr*v){
return({({struct Cyc_Absyn_Enumdecl*(*_tmp37D)(struct Cyc_Dict_Dict d,struct _fat_ptr*k)=({struct Cyc_Absyn_Enumdecl*(*_tmp8A)(struct Cyc_Dict_Dict d,struct _fat_ptr*k)=(struct Cyc_Absyn_Enumdecl*(*)(struct Cyc_Dict_Dict d,struct _fat_ptr*k))Cyc_Dict_lookup;_tmp8A;});struct Cyc_Dict_Dict _tmp37C=renv->enumdecls;_tmp37D(_tmp37C,v);});});}
# 236
static struct Cyc_Absyn_Typedefdecl*Cyc_Binding_lookup_typedefdecl(struct Cyc_Binding_ResolveNSEnv*renv,struct _fat_ptr*v){
return({({struct Cyc_Absyn_Typedefdecl*(*_tmp37F)(struct Cyc_Dict_Dict d,struct _fat_ptr*k)=({struct Cyc_Absyn_Typedefdecl*(*_tmp8C)(struct Cyc_Dict_Dict d,struct _fat_ptr*k)=(struct Cyc_Absyn_Typedefdecl*(*)(struct Cyc_Dict_Dict d,struct _fat_ptr*k))Cyc_Dict_lookup;_tmp8C;});struct Cyc_Dict_Dict _tmp37E=renv->typedefs;_tmp37F(_tmp37E,v);});});}
# 239
static void*Cyc_Binding_lookup_ordinary_global(struct Cyc_Binding_ResolveNSEnv*renv,struct _fat_ptr*v){
return({({void*(*_tmp381)(struct Cyc_Dict_Dict d,struct _fat_ptr*k)=({void*(*_tmp8E)(struct Cyc_Dict_Dict d,struct _fat_ptr*k)=(void*(*)(struct Cyc_Dict_Dict d,struct _fat_ptr*k))Cyc_Dict_lookup;_tmp8E;});struct Cyc_Dict_Dict _tmp380=renv->ordinaries;_tmp381(_tmp380,v);});});}
# 242
static void*Cyc_Binding_lookup_ordinary(unsigned loc,struct Cyc_Binding_Env*env,struct _tuple0*qv){
struct _fat_ptr*_tmp91;union Cyc_Absyn_Nmspace _tmp90;_LL1: _tmp90=qv->f1;_tmp91=qv->f2;_LL2: {union Cyc_Absyn_Nmspace nsl=_tmp90;struct _fat_ptr*v=_tmp91;
union Cyc_Absyn_Nmspace _tmp92=nsl;switch((_tmp92.Rel_n).tag){case 4U: _LL4: _LL5:
# 246
 if(({Cyc_Binding_at_toplevel(env);})|| !({({int(*_tmp383)(struct Cyc_Dict_Dict d,struct _fat_ptr*k)=({int(*_tmp93)(struct Cyc_Dict_Dict d,struct _fat_ptr*k)=(int(*)(struct Cyc_Dict_Dict d,struct _fat_ptr*k))Cyc_Dict_member;_tmp93;});struct Cyc_Dict_Dict _tmp382=*((struct Cyc_Dict_Dict*)_check_null(env->local_vars));_tmp383(_tmp382,v);});}))
(int)_throw(({struct Cyc_Binding_BindingError_exn_struct*_tmp94=_cycalloc(sizeof(*_tmp94));_tmp94->tag=Cyc_Binding_BindingError;_tmp94;}));
# 246
return({({void*(*_tmp385)(struct Cyc_Dict_Dict d,struct _fat_ptr*k)=({
# 248
void*(*_tmp95)(struct Cyc_Dict_Dict d,struct _fat_ptr*k)=(void*(*)(struct Cyc_Dict_Dict d,struct _fat_ptr*k))Cyc_Dict_lookup;_tmp95;});struct Cyc_Dict_Dict _tmp384=*((struct Cyc_Dict_Dict*)_check_null(env->local_vars));_tmp385(_tmp384,v);});});case 1U: if((_tmp92.Rel_n).val == 0){_LL6: _LL7:
# 250
 if(!({Cyc_Binding_at_toplevel(env);})&&({({int(*_tmp387)(struct Cyc_Dict_Dict d,struct _fat_ptr*k)=({int(*_tmp96)(struct Cyc_Dict_Dict d,struct _fat_ptr*k)=(int(*)(struct Cyc_Dict_Dict d,struct _fat_ptr*k))Cyc_Dict_member;_tmp96;});struct Cyc_Dict_Dict _tmp386=*((struct Cyc_Dict_Dict*)_check_null(env->local_vars));_tmp387(_tmp386,v);});}))
return({({void*(*_tmp389)(struct Cyc_Dict_Dict d,struct _fat_ptr*k)=({void*(*_tmp97)(struct Cyc_Dict_Dict d,struct _fat_ptr*k)=(void*(*)(struct Cyc_Dict_Dict d,struct _fat_ptr*k))Cyc_Dict_lookup;_tmp97;});struct Cyc_Dict_Dict _tmp388=*((struct Cyc_Dict_Dict*)_check_null(env->local_vars));_tmp389(_tmp388,v);});});
# 250
goto _LL9;}else{goto _LL8;}default: _LL8: _LL9:
# 254
 return({({void*(*_tmp38C)(unsigned loc,struct Cyc_Binding_NSCtxt*ctxt,struct _tuple0*qv,void*(*lookup)(struct Cyc_Binding_ResolveNSEnv*,struct _fat_ptr*))=({void*(*_tmp98)(unsigned loc,struct Cyc_Binding_NSCtxt*ctxt,struct _tuple0*qv,void*(*lookup)(struct Cyc_Binding_ResolveNSEnv*,struct _fat_ptr*))=(void*(*)(unsigned loc,struct Cyc_Binding_NSCtxt*ctxt,struct _tuple0*qv,void*(*lookup)(struct Cyc_Binding_ResolveNSEnv*,struct _fat_ptr*)))Cyc_Binding_resolve_lookup;_tmp98;});unsigned _tmp38B=loc;struct Cyc_Binding_NSCtxt*_tmp38A=env->ns;_tmp38C(_tmp38B,_tmp38A,qv,Cyc_Binding_lookup_ordinary_global);});});}_LL3:;}}
# 242
void Cyc_Binding_resolve_decl(struct Cyc_Binding_Env*,struct Cyc_Absyn_Decl*);
# 259
void Cyc_Binding_resolve_decls(struct Cyc_Binding_Env*,struct Cyc_List_List*);
void Cyc_Binding_resolve_stmt(struct Cyc_Binding_Env*,struct Cyc_Absyn_Stmt*);
void Cyc_Binding_resolve_pat(struct Cyc_Binding_Env*,struct Cyc_Absyn_Pat*);
void Cyc_Binding_resolve_exp(struct Cyc_Binding_Env*,struct Cyc_Absyn_Exp*);
void Cyc_Binding_resolve_type(unsigned,struct Cyc_Binding_Env*,void*);
void Cyc_Binding_resolve_rgnpo(unsigned,struct Cyc_Binding_Env*,struct Cyc_List_List*);
void Cyc_Binding_resolve_scs(struct Cyc_Binding_Env*,struct Cyc_List_List*);
void Cyc_Binding_resolve_aggrfields(unsigned,struct Cyc_Binding_Env*,struct Cyc_List_List*);
void Cyc_Binding_resolve_function_stuff(unsigned loc,struct Cyc_Binding_Env*env,struct Cyc_Absyn_FnInfo i);
void Cyc_Binding_resolve_asm_iolist(struct Cyc_Binding_Env*,struct Cyc_List_List*l);
struct Cyc_List_List*Cyc_Binding_get_fun_vardecls(int,unsigned,struct Cyc_Binding_Env*,struct Cyc_List_List*,struct Cyc_Absyn_VarargInfo*);
# 274
void Cyc_Binding_absolutize_decl(unsigned loc,struct Cyc_Binding_Env*env,struct _tuple0*qv,enum Cyc_Absyn_Scope sc){
union Cyc_Absyn_Nmspace _stmttmp4=(*qv).f1;union Cyc_Absyn_Nmspace _tmp9A=_stmttmp4;switch((_tmp9A.Abs_n).tag){case 1U: if((_tmp9A.Rel_n).val == 0){_LL1: _LL2:
# 277
 if(({Cyc_Binding_at_toplevel(env);}))
({union Cyc_Absyn_Nmspace _tmp38D=({union Cyc_Absyn_Nmspace(*_tmp9B)(struct Cyc_List_List*ns,int C_scope)=Cyc_Absyn_Abs_n;struct Cyc_List_List*_tmp9C=(env->ns)->curr_ns;int _tmp9D=({Cyc_Binding_in_cinclude(env);})||(int)sc == (int)Cyc_Absyn_ExternC;_tmp9B(_tmp9C,_tmp9D);});(*qv).f1=_tmp38D;});else{
# 280
(*qv).f1=Cyc_Absyn_Loc_n;}
goto _LL0;}else{goto _LL7;}case 4U: _LL3: _LL4:
 goto _LL0;case 2U: _LL5: _LL6:
# 284
 if(!({Cyc_Binding_at_toplevel(env);}))
goto _LL8;
# 284
goto _LL0;default: _LL7: _LL8:
# 287
({struct Cyc_Warn_String_Warn_Warg_struct _tmp9F=({struct Cyc_Warn_String_Warn_Warg_struct _tmp31A;_tmp31A.tag=0U,({struct _fat_ptr _tmp38E=({const char*_tmpA3="qualified names in declarations unimplemented (";_tag_fat(_tmpA3,sizeof(char),48U);});_tmp31A.f1=_tmp38E;});_tmp31A;});struct Cyc_Warn_Qvar_Warn_Warg_struct _tmpA0=({struct Cyc_Warn_Qvar_Warn_Warg_struct _tmp319;_tmp319.tag=3U,_tmp319.f1=qv;_tmp319;});struct Cyc_Warn_String_Warn_Warg_struct _tmpA1=({struct Cyc_Warn_String_Warn_Warg_struct _tmp318;_tmp318.tag=0U,({struct _fat_ptr _tmp38F=({const char*_tmpA2=")";_tag_fat(_tmpA2,sizeof(char),2U);});_tmp318.f1=_tmp38F;});_tmp318;});void*_tmp9E[3U];_tmp9E[0]=& _tmp9F,_tmp9E[1]=& _tmpA0,_tmp9E[2]=& _tmpA1;({unsigned _tmp390=loc;Cyc_Warn_err2(_tmp390,_tag_fat(_tmp9E,sizeof(void*),3U));});});}_LL0:;}
# 293
void Cyc_Binding_absolutize_topdecl(unsigned loc,struct Cyc_Binding_Env*env,struct _tuple0*qv,enum Cyc_Absyn_Scope sc){
struct Cyc_Dict_Dict*old_locals=env->local_vars;
env->local_vars=0;
({Cyc_Binding_absolutize_decl(loc,env,qv,sc);});
env->local_vars=old_locals;}
# 300
void Cyc_Binding_check_warn_override(unsigned loc,struct Cyc_Binding_Env*env,struct _tuple0*q){
struct _fat_ptr hides_what;
struct _handler_cons _tmpA6;_push_handler(& _tmpA6);{int _tmpA8=0;if(setjmp(_tmpA6.handler))_tmpA8=1;if(!_tmpA8){
{void*_stmttmp5=({Cyc_Binding_lookup_ordinary(loc,env,q);});void*_tmpA9=_stmttmp5;void*_tmpAA;switch(*((int*)_tmpA9)){case 0U: _LL1: _tmpAA=(void*)((struct Cyc_Binding_VarRes_Binding_Resolved_struct*)_tmpA9)->f1;_LL2: {void*b=_tmpAA;
# 305
if(({Cyc_Binding_at_toplevel(env);})){
_npop_handler(0U);return;}
# 305
{void*_tmpAB=b;switch(*((int*)_tmpAB)){case 1U: _LLC: _LLD:
# 308
 hides_what=({const char*_tmpAC="global variable";_tag_fat(_tmpAC,sizeof(char),16U);});goto _LLB;case 4U: _LLE: _LLF:
 hides_what=({const char*_tmpAD="local variable";_tag_fat(_tmpAD,sizeof(char),15U);});goto _LLB;case 3U: _LL10: _LL11:
 hides_what=({const char*_tmpAE="parameter";_tag_fat(_tmpAE,sizeof(char),10U);});goto _LLB;case 5U: _LL12: _LL13:
 hides_what=({const char*_tmpAF="pattern variable";_tag_fat(_tmpAF,sizeof(char),17U);});goto _LLB;case 2U: _LL14: _LL15:
 hides_what=({const char*_tmpB0="function";_tag_fat(_tmpB0,sizeof(char),9U);});goto _LLB;default: _LL16: _LL17:
({void*_tmpB1=0U;({int(*_tmp392)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmpB3)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmpB3;});struct _fat_ptr _tmp391=({const char*_tmpB2="shadowing free variable!";_tag_fat(_tmpB2,sizeof(char),25U);});_tmp392(_tmp391,_tag_fat(_tmpB1,sizeof(void*),0U));});});}_LLB:;}
# 315
goto _LL0;}case 1U: _LL3: _LL4:
 hides_what=({const char*_tmpB4="struct constructor";_tag_fat(_tmpB4,sizeof(char),19U);});goto _LL0;case 2U: _LL5: _LL6:
 hides_what=({const char*_tmpB5="datatype constructor";_tag_fat(_tmpB5,sizeof(char),21U);});goto _LL0;case 3U: _LL7: _LL8:
 goto _LLA;default: _LL9: _LLA:
 hides_what=({const char*_tmpB6="enum tag";_tag_fat(_tmpB6,sizeof(char),9U);});goto _LL0;}_LL0:;}
# 321
({struct Cyc_Warn_String_Warn_Warg_struct _tmpB8=({struct Cyc_Warn_String_Warn_Warg_struct _tmp31C;_tmp31C.tag=0U,({struct _fat_ptr _tmp393=({const char*_tmpBA="declaration hides ";_tag_fat(_tmpBA,sizeof(char),19U);});_tmp31C.f1=_tmp393;});_tmp31C;});struct Cyc_Warn_String_Warn_Warg_struct _tmpB9=({struct Cyc_Warn_String_Warn_Warg_struct _tmp31B;_tmp31B.tag=0U,_tmp31B.f1=hides_what;_tmp31B;});void*_tmpB7[2U];_tmpB7[0]=& _tmpB8,_tmpB7[1]=& _tmpB9;({unsigned _tmp394=loc;Cyc_Warn_warn2(_tmp394,_tag_fat(_tmpB7,sizeof(void*),2U));});});
_npop_handler(0U);return;
# 303
;_pop_handler();}else{void*_tmpA7=(void*)Cyc_Core_get_exn_thrown();void*_tmpBB=_tmpA7;void*_tmpBC;if(((struct Cyc_Binding_BindingError_exn_struct*)_tmpBB)->tag == Cyc_Binding_BindingError){_LL19: _LL1A:
# 323
 return;}else{_LL1B: _tmpBC=_tmpBB;_LL1C: {void*exn=_tmpBC;(int)_rethrow(exn);}}_LL18:;}}}
# 326
void Cyc_Binding_resolve_and_add_vardecl(unsigned loc,struct Cyc_Binding_Env*env,struct Cyc_Absyn_Vardecl*vd){
({Cyc_Binding_absolutize_decl(loc,env,vd->name,vd->sc);});{
struct _tuple0*_stmttmp6=vd->name;struct _fat_ptr*_tmpBF;union Cyc_Absyn_Nmspace _tmpBE;_LL1: _tmpBE=_stmttmp6->f1;_tmpBF=_stmttmp6->f2;_LL2: {union Cyc_Absyn_Nmspace decl_ns=_tmpBE;struct _fat_ptr*decl_name=_tmpBF;
({Cyc_Binding_resolve_type(loc,env,vd->type);});
if(Cyc_Binding_warn_override)
({Cyc_Binding_check_warn_override(loc,env,vd->name);});
# 330
if(!({Cyc_Binding_at_toplevel(env);}))
# 337
({struct Cyc_Dict_Dict _tmp399=({({struct Cyc_Dict_Dict(*_tmp398)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,void*v)=({struct Cyc_Dict_Dict(*_tmpC0)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,void*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,void*v))Cyc_Dict_insert;_tmpC0;});struct Cyc_Dict_Dict _tmp397=*((struct Cyc_Dict_Dict*)_check_null(env->local_vars));struct _fat_ptr*_tmp396=decl_name;_tmp398(_tmp397,_tmp396,(void*)({struct Cyc_Binding_VarRes_Binding_Resolved_struct*_tmpC2=_cycalloc(sizeof(*_tmpC2));_tmpC2->tag=0U,({void*_tmp395=(void*)({struct Cyc_Absyn_Local_b_Absyn_Binding_struct*_tmpC1=_cycalloc(sizeof(*_tmpC1));_tmpC1->tag=4U,_tmpC1->f1=vd;_tmpC1;});_tmpC2->f1=_tmp395;});_tmpC2;}));});});*((struct Cyc_Dict_Dict*)_check_null(env->local_vars))=_tmp399;});else{
# 341
struct Cyc_Binding_ResolveNSEnv*decl_ns_data=({({struct Cyc_Binding_ResolveNSEnv*(*_tmp39B)(struct Cyc_Binding_NSCtxt*ctxt,union Cyc_Absyn_Nmspace abs_ns)=({struct Cyc_Binding_ResolveNSEnv*(*_tmpC6)(struct Cyc_Binding_NSCtxt*ctxt,union Cyc_Absyn_Nmspace abs_ns)=(struct Cyc_Binding_ResolveNSEnv*(*)(struct Cyc_Binding_NSCtxt*ctxt,union Cyc_Absyn_Nmspace abs_ns))Cyc_Binding_get_ns_data;_tmpC6;});struct Cyc_Binding_NSCtxt*_tmp39A=env->ns;_tmp39B(_tmp39A,decl_ns);});});
({struct Cyc_Dict_Dict _tmp3A0=({({struct Cyc_Dict_Dict(*_tmp39F)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,void*v)=({struct Cyc_Dict_Dict(*_tmpC3)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,void*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,void*v))Cyc_Dict_insert;_tmpC3;});struct Cyc_Dict_Dict _tmp39E=decl_ns_data->ordinaries;struct _fat_ptr*_tmp39D=decl_name;_tmp39F(_tmp39E,_tmp39D,(void*)({struct Cyc_Binding_VarRes_Binding_Resolved_struct*_tmpC5=_cycalloc(sizeof(*_tmpC5));_tmpC5->tag=0U,({void*_tmp39C=(void*)({struct Cyc_Absyn_Global_b_Absyn_Binding_struct*_tmpC4=_cycalloc(sizeof(*_tmpC4));_tmpC4->tag=1U,_tmpC4->f1=vd;_tmpC4;});_tmpC5->f1=_tmp39C;});_tmpC5;}));});});decl_ns_data->ordinaries=_tmp3A0;});}}}}
# 348
void Cyc_Binding_resolve_stmt(struct Cyc_Binding_Env*env,struct Cyc_Absyn_Stmt*s){
void*_stmttmp7=s->r;void*_tmpC8=_stmttmp7;struct Cyc_List_List*_tmpCA;struct Cyc_Absyn_Stmt*_tmpC9;struct Cyc_List_List*_tmpCC;struct Cyc_Absyn_Exp*_tmpCB;struct Cyc_Absyn_Stmt*_tmpCE;struct Cyc_Absyn_Decl*_tmpCD;struct Cyc_Absyn_Stmt*_tmpCF;struct Cyc_List_List*_tmpD0;struct Cyc_Absyn_Stmt*_tmpD4;struct Cyc_Absyn_Exp*_tmpD3;struct Cyc_Absyn_Exp*_tmpD2;struct Cyc_Absyn_Exp*_tmpD1;struct Cyc_Absyn_Stmt*_tmpD6;struct Cyc_Absyn_Exp*_tmpD5;struct Cyc_Absyn_Exp*_tmpD8;struct Cyc_Absyn_Stmt*_tmpD7;struct Cyc_Absyn_Stmt*_tmpDB;struct Cyc_Absyn_Stmt*_tmpDA;struct Cyc_Absyn_Exp*_tmpD9;struct Cyc_Absyn_Exp*_tmpDC;struct Cyc_Absyn_Stmt*_tmpDE;struct Cyc_Absyn_Stmt*_tmpDD;struct Cyc_Absyn_Exp*_tmpDF;switch(*((int*)_tmpC8)){case 0U: _LL1: _LL2:
 return;case 1U: _LL3: _tmpDF=((struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct*)_tmpC8)->f1;_LL4: {struct Cyc_Absyn_Exp*e=_tmpDF;
({Cyc_Binding_resolve_exp(env,e);});return;}case 2U: _LL5: _tmpDD=((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_tmpC8)->f1;_tmpDE=((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_tmpC8)->f2;_LL6: {struct Cyc_Absyn_Stmt*s1=_tmpDD;struct Cyc_Absyn_Stmt*s2=_tmpDE;
({Cyc_Binding_resolve_stmt(env,s1);});({Cyc_Binding_resolve_stmt(env,s2);});return;}case 3U: _LL7: _tmpDC=((struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct*)_tmpC8)->f1;_LL8: {struct Cyc_Absyn_Exp*eopt=_tmpDC;
if((unsigned)eopt)({Cyc_Binding_resolve_exp(env,eopt);});return;}case 4U: _LL9: _tmpD9=((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmpC8)->f1;_tmpDA=((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmpC8)->f2;_tmpDB=((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmpC8)->f3;_LLA: {struct Cyc_Absyn_Exp*e=_tmpD9;struct Cyc_Absyn_Stmt*s1=_tmpDA;struct Cyc_Absyn_Stmt*s2=_tmpDB;
# 355
({Cyc_Binding_resolve_exp(env,e);});({Cyc_Binding_resolve_stmt(env,s1);});({Cyc_Binding_resolve_stmt(env,s2);});return;}case 14U: _LLB: _tmpD7=((struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct*)_tmpC8)->f1;_tmpD8=(((struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct*)_tmpC8)->f2).f1;_LLC: {struct Cyc_Absyn_Stmt*s1=_tmpD7;struct Cyc_Absyn_Exp*e=_tmpD8;
_tmpD5=e;_tmpD6=s1;goto _LLE;}case 5U: _LLD: _tmpD5=(((struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct*)_tmpC8)->f1).f1;_tmpD6=((struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct*)_tmpC8)->f2;_LLE: {struct Cyc_Absyn_Exp*e=_tmpD5;struct Cyc_Absyn_Stmt*s1=_tmpD6;
({Cyc_Binding_resolve_exp(env,e);});({Cyc_Binding_resolve_stmt(env,s1);});return;}case 9U: _LLF: _tmpD1=((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmpC8)->f1;_tmpD2=(((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmpC8)->f2).f1;_tmpD3=(((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmpC8)->f3).f1;_tmpD4=((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmpC8)->f4;_LL10: {struct Cyc_Absyn_Exp*e1=_tmpD1;struct Cyc_Absyn_Exp*e2=_tmpD2;struct Cyc_Absyn_Exp*e3=_tmpD3;struct Cyc_Absyn_Stmt*s1=_tmpD4;
# 359
({Cyc_Binding_resolve_exp(env,e1);});({Cyc_Binding_resolve_exp(env,e2);});({Cyc_Binding_resolve_exp(env,e3);});
({Cyc_Binding_resolve_stmt(env,s1);});
return;}case 6U: _LL11: _LL12:
 goto _LL14;case 7U: _LL13: _LL14:
 goto _LL16;case 8U: _LL15: _LL16:
 return;case 11U: _LL17: _tmpD0=((struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct*)_tmpC8)->f1;_LL18: {struct Cyc_List_List*es=_tmpD0;
# 366
for(0;es != 0;es=es->tl){
({Cyc_Binding_resolve_exp(env,(struct Cyc_Absyn_Exp*)es->hd);});}
return;}case 13U: _LL19: _tmpCF=((struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct*)_tmpC8)->f2;_LL1A: {struct Cyc_Absyn_Stmt*s1=_tmpCF;
({Cyc_Binding_resolve_stmt(env,s1);});return;}case 12U: _LL1B: _tmpCD=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmpC8)->f1;_tmpCE=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmpC8)->f2;_LL1C: {struct Cyc_Absyn_Decl*d=_tmpCD;struct Cyc_Absyn_Stmt*s1=_tmpCE;
# 371
struct Cyc_Dict_Dict old_locals=*((struct Cyc_Dict_Dict*)_check_null(env->local_vars));
({Cyc_Binding_resolve_decl(env,d);});
({Cyc_Binding_resolve_stmt(env,s1);});
*((struct Cyc_Dict_Dict*)_check_null(env->local_vars))=old_locals;
return;}case 10U: _LL1D: _tmpCB=((struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct*)_tmpC8)->f1;_tmpCC=((struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct*)_tmpC8)->f2;_LL1E: {struct Cyc_Absyn_Exp*e=_tmpCB;struct Cyc_List_List*scs=_tmpCC;
({Cyc_Binding_resolve_exp(env,e);});({Cyc_Binding_resolve_scs(env,scs);});return;}default: _LL1F: _tmpC9=((struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct*)_tmpC8)->f1;_tmpCA=((struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct*)_tmpC8)->f2;_LL20: {struct Cyc_Absyn_Stmt*s1=_tmpC9;struct Cyc_List_List*scs=_tmpCA;
({Cyc_Binding_resolve_stmt(env,s1);});({Cyc_Binding_resolve_scs(env,scs);});return;}}_LL0:;}struct _tuple12{struct Cyc_List_List*f1;struct Cyc_Absyn_Exp*f2;};
# 380
void Cyc_Binding_resolve_exp(struct Cyc_Binding_Env*env,struct Cyc_Absyn_Exp*e){
void*_stmttmp8=e->r;void*_tmpE1=_stmttmp8;struct Cyc_List_List*_tmpE6;struct Cyc_List_List*_tmpE5;struct Cyc_List_List*_tmpE4;struct _fat_ptr _tmpE3;int _tmpE2;struct Cyc_List_List*_tmpE8;void*_tmpE7;struct Cyc_Absyn_Datatypefield*_tmpEB;struct Cyc_Absyn_Datatypedecl*_tmpEA;struct Cyc_List_List*_tmpE9;struct Cyc_Absyn_Enumfield*_tmpED;struct Cyc_Absyn_Enumdecl*_tmpEC;struct Cyc_Absyn_Enumfield*_tmpEF;void*_tmpEE;struct Cyc_List_List*_tmpF0;struct Cyc_List_List*_tmpF2;void*_tmpF1;void*_tmpF3;void*_tmpF4;void*_tmpF5;struct Cyc_Absyn_Stmt*_tmpF6;struct Cyc_Absyn_Exp*_tmpF8;void*_tmpF7;struct Cyc_List_List*_tmpFA;struct Cyc_Absyn_Exp*_tmpF9;struct Cyc_Absyn_Exp*_tmpFD;struct Cyc_Absyn_Exp*_tmpFC;struct Cyc_Absyn_Exp*_tmpFB;struct Cyc_Absyn_Exp*_tmpFF;struct Cyc_Absyn_Exp*_tmpFE;struct Cyc_Absyn_Exp*_tmp102;void**_tmp101;struct Cyc_Absyn_Exp*_tmp100;struct Cyc_Absyn_Exp*_tmp104;struct Cyc_Absyn_Exp*_tmp103;struct Cyc_Absyn_Exp*_tmp106;struct Cyc_Absyn_Exp*_tmp105;struct Cyc_Absyn_Exp*_tmp108;struct Cyc_Absyn_Exp*_tmp107;struct Cyc_Absyn_Exp*_tmp10A;struct Cyc_Absyn_Exp*_tmp109;struct Cyc_Absyn_Exp*_tmp10C;struct Cyc_Absyn_Exp*_tmp10B;struct Cyc_Absyn_Exp*_tmp10E;struct Cyc_Absyn_Exp*_tmp10D;struct Cyc_Absyn_Exp*_tmp110;struct Cyc_Absyn_Exp*_tmp10F;struct Cyc_Absyn_Exp*_tmp111;struct Cyc_Absyn_Exp*_tmp112;struct Cyc_Absyn_Exp*_tmp113;struct Cyc_Absyn_Exp*_tmp114;struct Cyc_Absyn_Exp*_tmp115;struct Cyc_Absyn_Exp*_tmp116;struct Cyc_Absyn_Exp*_tmp117;struct Cyc_Absyn_Exp*_tmp118;struct Cyc_Absyn_Exp*_tmp119;struct Cyc_Absyn_Exp*_tmp11A;struct Cyc_List_List*_tmp11B;struct Cyc_List_List*_tmp11C;void*_tmp11E;struct Cyc_Absyn_Exp*_tmp11D;struct Cyc_List_List*_tmp11F;struct Cyc_Absyn_Exp*_tmp122;struct Cyc_Absyn_Exp*_tmp121;struct Cyc_Absyn_Vardecl*_tmp120;struct Cyc_Absyn_Aggrdecl**_tmp126;struct Cyc_List_List*_tmp125;struct Cyc_List_List*_tmp124;struct _tuple0**_tmp123;int*_tmp129;struct Cyc_List_List*_tmp128;struct Cyc_Absyn_Exp*_tmp127;void**_tmp12A;switch(*((int*)_tmpE1)){case 1U: _LL1: _tmp12A=(void**)&((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_LL2: {void**b=_tmp12A;
# 383
void*_stmttmp9=*b;void*_tmp12B=_stmttmp9;struct _tuple0*_tmp12C;if(((struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct*)_tmp12B)->tag == 0U){_LL58: _tmp12C=((struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct*)_tmp12B)->f1;_LL59: {struct _tuple0*q=_tmp12C;
# 385
struct _handler_cons _tmp12D;_push_handler(& _tmp12D);{int _tmp12F=0;if(setjmp(_tmp12D.handler))_tmp12F=1;if(!_tmp12F){{void*_stmttmpA=({Cyc_Binding_lookup_ordinary(e->loc,env,q);});void*_tmp130=_stmttmpA;struct Cyc_Absyn_Aggrdecl*_tmp131;struct Cyc_Absyn_Enumfield*_tmp133;void*_tmp132;struct Cyc_Absyn_Enumfield*_tmp135;struct Cyc_Absyn_Enumdecl*_tmp134;struct Cyc_Absyn_Datatypefield*_tmp137;struct Cyc_Absyn_Datatypedecl*_tmp136;void*_tmp138;switch(*((int*)_tmp130)){case 0U: _LL5D: _tmp138=(void*)((struct Cyc_Binding_VarRes_Binding_Resolved_struct*)_tmp130)->f1;_LL5E: {void*bnd=_tmp138;
*b=bnd;_npop_handler(0U);return;}case 2U: _LL5F: _tmp136=((struct Cyc_Binding_DatatypeRes_Binding_Resolved_struct*)_tmp130)->f1;_tmp137=((struct Cyc_Binding_DatatypeRes_Binding_Resolved_struct*)_tmp130)->f2;_LL60: {struct Cyc_Absyn_Datatypedecl*tud=_tmp136;struct Cyc_Absyn_Datatypefield*tuf=_tmp137;
({void*_tmp3A1=(void*)({struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*_tmp139=_cycalloc(sizeof(*_tmp139));_tmp139->tag=31U,_tmp139->f1=0,_tmp139->f2=tud,_tmp139->f3=tuf;_tmp139;});e->r=_tmp3A1;});_npop_handler(0U);return;}case 3U: _LL61: _tmp134=((struct Cyc_Binding_EnumRes_Binding_Resolved_struct*)_tmp130)->f1;_tmp135=((struct Cyc_Binding_EnumRes_Binding_Resolved_struct*)_tmp130)->f2;_LL62: {struct Cyc_Absyn_Enumdecl*ed=_tmp134;struct Cyc_Absyn_Enumfield*ef=_tmp135;
({void*_tmp3A2=(void*)({struct Cyc_Absyn_Enum_e_Absyn_Raw_exp_struct*_tmp13A=_cycalloc(sizeof(*_tmp13A));_tmp13A->tag=32U,_tmp13A->f1=ed,_tmp13A->f2=ef;_tmp13A;});e->r=_tmp3A2;});_npop_handler(0U);return;}case 4U: _LL63: _tmp132=(void*)((struct Cyc_Binding_AnonEnumRes_Binding_Resolved_struct*)_tmp130)->f1;_tmp133=((struct Cyc_Binding_AnonEnumRes_Binding_Resolved_struct*)_tmp130)->f2;_LL64: {void*t=_tmp132;struct Cyc_Absyn_Enumfield*ef=_tmp133;
({void*_tmp3A3=(void*)({struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct*_tmp13B=_cycalloc(sizeof(*_tmp13B));_tmp13B->tag=33U,_tmp13B->f1=t,_tmp13B->f2=ef;_tmp13B;});e->r=_tmp3A3;});_npop_handler(0U);return;}default: _LL65: _tmp131=((struct Cyc_Binding_AggrRes_Binding_Resolved_struct*)_tmp130)->f1;_LL66: {struct Cyc_Absyn_Aggrdecl*ad=_tmp131;
({struct Cyc_Warn_String_Warn_Warg_struct _tmp13D=({struct Cyc_Warn_String_Warn_Warg_struct _tmp31E;_tmp31E.tag=0U,({struct _fat_ptr _tmp3A4=({const char*_tmp13F="bad occurrence of type name ";_tag_fat(_tmp13F,sizeof(char),29U);});_tmp31E.f1=_tmp3A4;});_tmp31E;});struct Cyc_Warn_Qvar_Warn_Warg_struct _tmp13E=({struct Cyc_Warn_Qvar_Warn_Warg_struct _tmp31D;_tmp31D.tag=3U,_tmp31D.f1=q;_tmp31D;});void*_tmp13C[2U];_tmp13C[0]=& _tmp13D,_tmp13C[1]=& _tmp13E;({unsigned _tmp3A5=e->loc;Cyc_Warn_err2(_tmp3A5,_tag_fat(_tmp13C,sizeof(void*),2U));});});_npop_handler(0U);return;}}_LL5C:;}
# 385
;_pop_handler();}else{void*_tmp12E=(void*)Cyc_Core_get_exn_thrown();void*_tmp140=_tmp12E;void*_tmp141;if(((struct Cyc_Binding_BindingError_exn_struct*)_tmp140)->tag == Cyc_Binding_BindingError){_LL68: _LL69:
# 393
({struct Cyc_Warn_String_Warn_Warg_struct _tmp143=({struct Cyc_Warn_String_Warn_Warg_struct _tmp320;_tmp320.tag=0U,({struct _fat_ptr _tmp3A6=({const char*_tmp145="undeclared identifier ";_tag_fat(_tmp145,sizeof(char),23U);});_tmp320.f1=_tmp3A6;});_tmp320;});struct Cyc_Warn_Qvar_Warn_Warg_struct _tmp144=({struct Cyc_Warn_Qvar_Warn_Warg_struct _tmp31F;_tmp31F.tag=3U,_tmp31F.f1=q;_tmp31F;});void*_tmp142[2U];_tmp142[0]=& _tmp143,_tmp142[1]=& _tmp144;({unsigned _tmp3A7=e->loc;Cyc_Warn_err2(_tmp3A7,_tag_fat(_tmp142,sizeof(void*),2U));});});return;}else{_LL6A: _tmp141=_tmp140;_LL6B: {void*exn=_tmp141;(int)_rethrow(exn);}}_LL67:;}}}}else{_LL5A: _LL5B:
# 395
 return;}_LL57:;}case 10U: _LL3: _tmp127=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_tmp128=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmpE1)->f2;_tmp129=(int*)&((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmpE1)->f4;_LL4: {struct Cyc_Absyn_Exp*e1=_tmp127;struct Cyc_List_List*es=_tmp128;int*b=_tmp129;
# 398
*b=1;
# 400
{struct Cyc_List_List*es2=es;for(0;es2 != 0;es2=es2->tl){
({Cyc_Binding_resolve_exp(env,(struct Cyc_Absyn_Exp*)es2->hd);});}}{
void*_stmttmpB=e1->r;void*_tmp146=_stmttmpB;void**_tmp147;if(((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp146)->tag == 1U){_LL6D: _tmp147=(void**)&((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp146)->f1;_LL6E: {void**b=_tmp147;
# 404
void*_stmttmpC=*b;void*_tmp148=_stmttmpC;struct _tuple0*_tmp149;if(((struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct*)_tmp148)->tag == 0U){_LL72: _tmp149=((struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct*)_tmp148)->f1;_LL73: {struct _tuple0*q=_tmp149;
# 406
struct _handler_cons _tmp14A;_push_handler(& _tmp14A);{int _tmp14C=0;if(setjmp(_tmp14A.handler))_tmp14C=1;if(!_tmp14C){{void*_stmttmpD=({Cyc_Binding_lookup_ordinary(e1->loc,env,q);});void*_tmp14D=_stmttmpD;struct Cyc_Absyn_Datatypefield*_tmp14F;struct Cyc_Absyn_Datatypedecl*_tmp14E;struct Cyc_Absyn_Aggrdecl*_tmp150;void*_tmp151;switch(*((int*)_tmp14D)){case 0U: _LL77: _tmp151=(void*)((struct Cyc_Binding_VarRes_Binding_Resolved_struct*)_tmp14D)->f1;_LL78: {void*bnd=_tmp151;
# 408
*b=bnd;_npop_handler(0U);return;}case 1U: _LL79: _tmp150=((struct Cyc_Binding_AggrRes_Binding_Resolved_struct*)_tmp14D)->f1;_LL7A: {struct Cyc_Absyn_Aggrdecl*ad=_tmp150;
# 410
struct Cyc_List_List*dles=0;
for(0;es != 0;es=es->tl){
dles=({struct Cyc_List_List*_tmp153=_cycalloc(sizeof(*_tmp153));({struct _tuple12*_tmp3A8=({struct _tuple12*_tmp152=_cycalloc(sizeof(*_tmp152));_tmp152->f1=0,_tmp152->f2=(struct Cyc_Absyn_Exp*)es->hd;_tmp152;});_tmp153->hd=_tmp3A8;}),_tmp153->tl=dles;_tmp153;});}
({void*_tmp3AA=(void*)({struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*_tmp154=_cycalloc(sizeof(*_tmp154));_tmp154->tag=29U,_tmp154->f1=ad->name,_tmp154->f2=0,({struct Cyc_List_List*_tmp3A9=({Cyc_List_imp_rev(dles);});_tmp154->f3=_tmp3A9;}),_tmp154->f4=ad;_tmp154;});e->r=_tmp3AA;});
_npop_handler(0U);return;}case 2U: _LL7B: _tmp14E=((struct Cyc_Binding_DatatypeRes_Binding_Resolved_struct*)_tmp14D)->f1;_tmp14F=((struct Cyc_Binding_DatatypeRes_Binding_Resolved_struct*)_tmp14D)->f2;_LL7C: {struct Cyc_Absyn_Datatypedecl*tud=_tmp14E;struct Cyc_Absyn_Datatypefield*tuf=_tmp14F;
# 416
if(tuf->typs == 0)
# 418
({struct Cyc_Warn_Qvar_Warn_Warg_struct _tmp156=({struct Cyc_Warn_Qvar_Warn_Warg_struct _tmp322;_tmp322.tag=3U,_tmp322.f1=tuf->name;_tmp322;});struct Cyc_Warn_String_Warn_Warg_struct _tmp157=({struct Cyc_Warn_String_Warn_Warg_struct _tmp321;_tmp321.tag=0U,({struct _fat_ptr _tmp3AB=({const char*_tmp158=" is a constant, not a function";_tag_fat(_tmp158,sizeof(char),31U);});_tmp321.f1=_tmp3AB;});_tmp321;});void*_tmp155[2U];_tmp155[0]=& _tmp156,_tmp155[1]=& _tmp157;({unsigned _tmp3AC=e->loc;Cyc_Warn_err2(_tmp3AC,_tag_fat(_tmp155,sizeof(void*),2U));});});
# 416
({void*_tmp3AD=(void*)({struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*_tmp159=_cycalloc(sizeof(*_tmp159));
# 419
_tmp159->tag=31U,_tmp159->f1=es,_tmp159->f2=tud,_tmp159->f3=tuf;_tmp159;});
# 416
e->r=_tmp3AD;});
# 420
_npop_handler(0U);return;}case 4U: _LL7D: _LL7E:
 goto _LL80;default: _LL7F: _LL80:
# 423
({struct Cyc_Warn_Qvar_Warn_Warg_struct _tmp15B=({struct Cyc_Warn_Qvar_Warn_Warg_struct _tmp324;_tmp324.tag=3U,_tmp324.f1=q;_tmp324;});struct Cyc_Warn_String_Warn_Warg_struct _tmp15C=({struct Cyc_Warn_String_Warn_Warg_struct _tmp323;_tmp323.tag=0U,({struct _fat_ptr _tmp3AE=({const char*_tmp15D=" is an enum constructor, not a function";_tag_fat(_tmp15D,sizeof(char),40U);});_tmp323.f1=_tmp3AE;});_tmp323;});void*_tmp15A[2U];_tmp15A[0]=& _tmp15B,_tmp15A[1]=& _tmp15C;({unsigned _tmp3AF=e->loc;Cyc_Warn_err2(_tmp3AF,_tag_fat(_tmp15A,sizeof(void*),2U));});});_npop_handler(0U);return;}_LL76:;}
# 406
;_pop_handler();}else{void*_tmp14B=(void*)Cyc_Core_get_exn_thrown();void*_tmp15E=_tmp14B;void*_tmp15F;if(((struct Cyc_Binding_BindingError_exn_struct*)_tmp15E)->tag == Cyc_Binding_BindingError){_LL82: _LL83:
# 425
({struct Cyc_Warn_String_Warn_Warg_struct _tmp161=({struct Cyc_Warn_String_Warn_Warg_struct _tmp326;_tmp326.tag=0U,({struct _fat_ptr _tmp3B0=({const char*_tmp163="undeclared identifier ";_tag_fat(_tmp163,sizeof(char),23U);});_tmp326.f1=_tmp3B0;});_tmp326;});struct Cyc_Warn_Qvar_Warn_Warg_struct _tmp162=({struct Cyc_Warn_Qvar_Warn_Warg_struct _tmp325;_tmp325.tag=3U,_tmp325.f1=q;_tmp325;});void*_tmp160[2U];_tmp160[0]=& _tmp161,_tmp160[1]=& _tmp162;({unsigned _tmp3B1=e->loc;Cyc_Warn_err2(_tmp3B1,_tag_fat(_tmp160,sizeof(void*),2U));});});return;}else{_LL84: _tmp15F=_tmp15E;_LL85: {void*exn=_tmp15F;(int)_rethrow(exn);}}_LL81:;}}}}else{_LL74: _LL75:
# 427
 return;}_LL71:;}}else{_LL6F: _LL70:
# 429
({Cyc_Binding_resolve_exp(env,e1);});return;}_LL6C:;}}case 29U: _LL5: _tmp123=(struct _tuple0**)&((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_tmp124=((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_tmpE1)->f2;_tmp125=((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_tmpE1)->f3;_tmp126=(struct Cyc_Absyn_Aggrdecl**)&((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_tmpE1)->f4;_LL6: {struct _tuple0**tn=_tmp123;struct Cyc_List_List*ts=_tmp124;struct Cyc_List_List*dles=_tmp125;struct Cyc_Absyn_Aggrdecl**adopt=_tmp126;
# 433
for(0;dles != 0;dles=dles->tl){
({Cyc_Binding_resolve_exp(env,(*((struct _tuple12*)dles->hd)).f2);});}{
struct _handler_cons _tmp164;_push_handler(& _tmp164);{int _tmp166=0;if(setjmp(_tmp164.handler))_tmp166=1;if(!_tmp166){
({struct Cyc_Absyn_Aggrdecl*_tmp3B5=({({struct Cyc_Absyn_Aggrdecl*(*_tmp3B4)(unsigned loc,struct Cyc_Binding_NSCtxt*ctxt,struct _tuple0*qv,struct Cyc_Absyn_Aggrdecl*(*lookup)(struct Cyc_Binding_ResolveNSEnv*,struct _fat_ptr*))=({struct Cyc_Absyn_Aggrdecl*(*_tmp167)(unsigned loc,struct Cyc_Binding_NSCtxt*ctxt,struct _tuple0*qv,struct Cyc_Absyn_Aggrdecl*(*lookup)(struct Cyc_Binding_ResolveNSEnv*,struct _fat_ptr*))=(struct Cyc_Absyn_Aggrdecl*(*)(unsigned loc,struct Cyc_Binding_NSCtxt*ctxt,struct _tuple0*qv,struct Cyc_Absyn_Aggrdecl*(*lookup)(struct Cyc_Binding_ResolveNSEnv*,struct _fat_ptr*)))Cyc_Binding_resolve_lookup;_tmp167;});unsigned _tmp3B3=e->loc;struct Cyc_Binding_NSCtxt*_tmp3B2=env->ns;_tmp3B4(_tmp3B3,_tmp3B2,*tn,Cyc_Binding_lookup_aggrdecl);});});*adopt=_tmp3B5;});
*tn=((struct Cyc_Absyn_Aggrdecl*)_check_null(*adopt))->name;
_npop_handler(0U);return;
# 436
;_pop_handler();}else{void*_tmp165=(void*)Cyc_Core_get_exn_thrown();void*_tmp168=_tmp165;void*_tmp169;if(((struct Cyc_Binding_BindingError_exn_struct*)_tmp168)->tag == Cyc_Binding_BindingError){_LL87: _LL88:
# 440
({struct Cyc_Warn_String_Warn_Warg_struct _tmp16B=({struct Cyc_Warn_String_Warn_Warg_struct _tmp328;_tmp328.tag=0U,({struct _fat_ptr _tmp3B6=({const char*_tmp16D="unbound struct/union name ";_tag_fat(_tmp16D,sizeof(char),27U);});_tmp328.f1=_tmp3B6;});_tmp328;});struct Cyc_Warn_Qvar_Warn_Warg_struct _tmp16C=({struct Cyc_Warn_Qvar_Warn_Warg_struct _tmp327;_tmp327.tag=3U,_tmp327.f1=*tn;_tmp327;});void*_tmp16A[2U];_tmp16A[0]=& _tmp16B,_tmp16A[1]=& _tmp16C;({unsigned _tmp3B7=e->loc;Cyc_Warn_err2(_tmp3B7,_tag_fat(_tmp16A,sizeof(void*),2U));});});
return;}else{_LL89: _tmp169=_tmp168;_LL8A: {void*exn=_tmp169;(int)_rethrow(exn);}}_LL86:;}}}}case 27U: _LL7: _tmp120=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_tmp121=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmpE1)->f2;_tmp122=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmpE1)->f3;_LL8: {struct Cyc_Absyn_Vardecl*vd=_tmp120;struct Cyc_Absyn_Exp*e1=_tmp121;struct Cyc_Absyn_Exp*e2=_tmp122;
# 445
({Cyc_Binding_resolve_exp(env,e1);});
if(({Cyc_Binding_at_toplevel(env);})){
({struct Cyc_Dict_Dict*_tmp3B9=({struct Cyc_Dict_Dict*_tmp16F=_cycalloc(sizeof(*_tmp16F));({struct Cyc_Dict_Dict _tmp3B8=({({struct Cyc_Dict_Dict(*_tmp16E)(int(*cmp)(struct _fat_ptr*,struct _fat_ptr*))=(struct Cyc_Dict_Dict(*)(int(*cmp)(struct _fat_ptr*,struct _fat_ptr*)))Cyc_Dict_empty;_tmp16E;})(Cyc_strptrcmp);});*_tmp16F=_tmp3B8;});_tmp16F;});env->local_vars=_tmp3B9;});
({Cyc_Binding_resolve_and_add_vardecl(e->loc,env,vd);});
({Cyc_Binding_resolve_exp(env,e2);});
env->local_vars=0;
return;}{
# 446
struct Cyc_Dict_Dict old_locals=*((struct Cyc_Dict_Dict*)_check_null(env->local_vars));
# 454
({Cyc_Binding_resolve_and_add_vardecl(e->loc,env,vd);});
({Cyc_Binding_resolve_exp(env,e2);});
*((struct Cyc_Dict_Dict*)_check_null(env->local_vars))=old_locals;
return;}}case 37U: _LL9: _tmp11F=((struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*)_tmpE1)->f2;_LLA: {struct Cyc_List_List*dles=_tmp11F;
# 461
for(0;dles != 0;dles=dles->tl){
({Cyc_Binding_resolve_exp(env,(*((struct _tuple12*)dles->hd)).f2);});}
return;}case 28U: _LLB: _tmp11D=((struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_tmp11E=(void*)((struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*)_tmpE1)->f2;_LLC: {struct Cyc_Absyn_Exp*e1=_tmp11D;void*t=_tmp11E;
# 467
({Cyc_Binding_resolve_exp(env,e1);});
({Cyc_Binding_resolve_type(e->loc,env,t);});
return;}case 2U: _LLD: _LLE:
# 471
 goto _LL10;case 0U: _LLF: _LL10:
 return;case 24U: _LL11: _tmp11C=((struct Cyc_Absyn_Tuple_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_LL12: {struct Cyc_List_List*es=_tmp11C;
# 474
_tmp11B=es;goto _LL14;}case 3U: _LL13: _tmp11B=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmpE1)->f2;_LL14: {struct Cyc_List_List*es=_tmp11B;
# 476
for(0;es != 0;es=es->tl){
({Cyc_Binding_resolve_exp(env,(struct Cyc_Absyn_Exp*)es->hd);});}
return;}case 42U: _LL15: _tmp11A=((struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_LL16: {struct Cyc_Absyn_Exp*e1=_tmp11A;
# 480
_tmp119=e1;goto _LL18;}case 39U: _LL17: _tmp119=((struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_LL18: {struct Cyc_Absyn_Exp*e1=_tmp119;
_tmp118=e1;goto _LL1A;}case 12U: _LL19: _tmp118=((struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_LL1A: {struct Cyc_Absyn_Exp*e1=_tmp118;
_tmp117=e1;goto _LL1C;}case 18U: _LL1B: _tmp117=((struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_LL1C: {struct Cyc_Absyn_Exp*e1=_tmp117;
_tmp116=e1;goto _LL1E;}case 11U: _LL1D: _tmp116=((struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_LL1E: {struct Cyc_Absyn_Exp*e1=_tmp116;
_tmp115=e1;goto _LL20;}case 5U: _LL1F: _tmp115=((struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_LL20: {struct Cyc_Absyn_Exp*e1=_tmp115;
_tmp114=e1;goto _LL22;}case 22U: _LL21: _tmp114=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_LL22: {struct Cyc_Absyn_Exp*e1=_tmp114;
_tmp113=e1;goto _LL24;}case 21U: _LL23: _tmp113=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_LL24: {struct Cyc_Absyn_Exp*e1=_tmp113;
_tmp112=e1;goto _LL26;}case 15U: _LL25: _tmp112=((struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_LL26: {struct Cyc_Absyn_Exp*e1=_tmp112;
_tmp111=e1;goto _LL28;}case 20U: _LL27: _tmp111=((struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_LL28: {struct Cyc_Absyn_Exp*e1=_tmp111;
({Cyc_Binding_resolve_exp(env,e1);});return;}case 36U: _LL29: _tmp10F=((struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_tmp110=((struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct*)_tmpE1)->f2;_LL2A: {struct Cyc_Absyn_Exp*e1=_tmp10F;struct Cyc_Absyn_Exp*e2=_tmp110;
# 491
_tmp10D=e1;_tmp10E=e2;goto _LL2C;}case 9U: _LL2B: _tmp10D=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_tmp10E=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmpE1)->f2;_LL2C: {struct Cyc_Absyn_Exp*e1=_tmp10D;struct Cyc_Absyn_Exp*e2=_tmp10E;
_tmp10B=e1;_tmp10C=e2;goto _LL2E;}case 4U: _LL2D: _tmp10B=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_tmp10C=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmpE1)->f3;_LL2E: {struct Cyc_Absyn_Exp*e1=_tmp10B;struct Cyc_Absyn_Exp*e2=_tmp10C;
_tmp109=e1;_tmp10A=e2;goto _LL30;}case 23U: _LL2F: _tmp109=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_tmp10A=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmpE1)->f2;_LL30: {struct Cyc_Absyn_Exp*e1=_tmp109;struct Cyc_Absyn_Exp*e2=_tmp10A;
_tmp107=e1;_tmp108=e2;goto _LL32;}case 7U: _LL31: _tmp107=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_tmp108=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmpE1)->f2;_LL32: {struct Cyc_Absyn_Exp*e1=_tmp107;struct Cyc_Absyn_Exp*e2=_tmp108;
_tmp105=e1;_tmp106=e2;goto _LL34;}case 8U: _LL33: _tmp105=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_tmp106=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmpE1)->f2;_LL34: {struct Cyc_Absyn_Exp*e1=_tmp105;struct Cyc_Absyn_Exp*e2=_tmp106;
({Cyc_Binding_resolve_exp(env,e1);});({Cyc_Binding_resolve_exp(env,e2);});return;}case 34U: _LL35: _tmp103=((struct Cyc_Absyn_Spawn_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_tmp104=((struct Cyc_Absyn_Spawn_e_Absyn_Raw_exp_struct*)_tmpE1)->f2;_LL36: {struct Cyc_Absyn_Exp*e1=_tmp103;struct Cyc_Absyn_Exp*e2=_tmp104;
# 499
({Cyc_Binding_resolve_exp(env,e1);});
({Cyc_Binding_resolve_exp(env,e2);});
return;}case 35U: _LL37: _tmp100=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmpE1)->f1).rgn;_tmp101=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmpE1)->f1).elt_type;_tmp102=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmpE1)->f1).num_elts;_LL38: {struct Cyc_Absyn_Exp*eo=_tmp100;void**to=_tmp101;struct Cyc_Absyn_Exp*e1=_tmp102;
# 503
if(eo != 0)({Cyc_Binding_resolve_exp(env,eo);});if(to != 0)
({Cyc_Binding_resolve_type(e->loc,env,*to);});
# 503
({Cyc_Binding_resolve_exp(env,e1);});
# 506
return;}case 16U: _LL39: _tmpFE=((struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_tmpFF=((struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct*)_tmpE1)->f2;_LL3A: {struct Cyc_Absyn_Exp*eo=_tmpFE;struct Cyc_Absyn_Exp*e2=_tmpFF;
# 509
if(eo != 0)({Cyc_Binding_resolve_exp(env,eo);});({Cyc_Binding_resolve_exp(env,e2);});
# 511
return;}case 6U: _LL3B: _tmpFB=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_tmpFC=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmpE1)->f2;_tmpFD=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmpE1)->f3;_LL3C: {struct Cyc_Absyn_Exp*e1=_tmpFB;struct Cyc_Absyn_Exp*e2=_tmpFC;struct Cyc_Absyn_Exp*e3=_tmpFD;
# 514
({Cyc_Binding_resolve_exp(env,e1);});({Cyc_Binding_resolve_exp(env,e2);});({Cyc_Binding_resolve_exp(env,e3);});return;}case 13U: _LL3D: _tmpF9=((struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_tmpFA=((struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*)_tmpE1)->f2;_LL3E: {struct Cyc_Absyn_Exp*e1=_tmpF9;struct Cyc_List_List*ts=_tmpFA;
# 517
({Cyc_Binding_resolve_exp(env,e1);});
for(0;ts != 0;ts=ts->tl){
({Cyc_Binding_resolve_type(e->loc,env,(void*)ts->hd);});}
return;}case 14U: _LL3F: _tmpF7=(void*)((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_tmpF8=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmpE1)->f2;_LL40: {void*t=_tmpF7;struct Cyc_Absyn_Exp*e1=_tmpF8;
# 523
({Cyc_Binding_resolve_exp(env,e1);});({Cyc_Binding_resolve_type(e->loc,env,t);});return;}case 38U: _LL41: _tmpF6=((struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_LL42: {struct Cyc_Absyn_Stmt*s=_tmpF6;
# 525
({Cyc_Binding_resolve_stmt(env,s);});return;}case 40U: _LL43: _tmpF5=(void*)((struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_LL44: {void*t=_tmpF5;
# 527
_tmpF4=t;goto _LL46;}case 19U: _LL45: _tmpF4=(void*)((struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_LL46: {void*t=_tmpF4;
_tmpF3=t;goto _LL48;}case 17U: _LL47: _tmpF3=(void*)((struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_LL48: {void*t=_tmpF3;
({Cyc_Binding_resolve_type(e->loc,env,t);});return;}case 25U: _LL49: _tmpF1=(((struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct*)_tmpE1)->f1)->f3;_tmpF2=((struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct*)_tmpE1)->f2;_LL4A: {void*t=_tmpF1;struct Cyc_List_List*dles=_tmpF2;
# 532
({Cyc_Binding_resolve_type(e->loc,env,t);});
_tmpF0=dles;goto _LL4C;}case 26U: _LL4B: _tmpF0=((struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_LL4C: {struct Cyc_List_List*dles=_tmpF0;
# 536
for(0;dles != 0;dles=dles->tl){
({Cyc_Binding_resolve_exp(env,(*((struct _tuple12*)dles->hd)).f2);});}
return;}case 33U: _LL4D: _tmpEE=(void*)((struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_tmpEF=((struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct*)_tmpE1)->f2;_LL4E: {void*t=_tmpEE;struct Cyc_Absyn_Enumfield*ef=_tmpEF;
# 541
({Cyc_Binding_resolve_type(e->loc,env,t);});return;}case 32U: _LL4F: _tmpEC=((struct Cyc_Absyn_Enum_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_tmpED=((struct Cyc_Absyn_Enum_e_Absyn_Raw_exp_struct*)_tmpE1)->f2;_LL50: {struct Cyc_Absyn_Enumdecl*ed=_tmpEC;struct Cyc_Absyn_Enumfield*ef=_tmpED;
return;}case 31U: _LL51: _tmpE9=((struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_tmpEA=((struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*)_tmpE1)->f2;_tmpEB=((struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*)_tmpE1)->f3;_LL52: {struct Cyc_List_List*es=_tmpE9;struct Cyc_Absyn_Datatypedecl*tud=_tmpEA;struct Cyc_Absyn_Datatypefield*tuf=_tmpEB;
# 544
for(0;es != 0;es=es->tl){
({Cyc_Binding_resolve_exp(env,(struct Cyc_Absyn_Exp*)es->hd);});}
return;}case 30U: _LL53: _tmpE7=(void*)((struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_tmpE8=((struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct*)_tmpE1)->f2;_LL54: {void*t=_tmpE7;struct Cyc_List_List*dles=_tmpE8;
# 548
({Cyc_Binding_resolve_type(e->loc,env,t);});
for(0;dles != 0;dles=dles->tl){
({Cyc_Binding_resolve_exp(env,(*((struct _tuple12*)dles->hd)).f2);});}
return;}default: _LL55: _tmpE2=((struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct*)_tmpE1)->f1;_tmpE3=((struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct*)_tmpE1)->f2;_tmpE4=((struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct*)_tmpE1)->f3;_tmpE5=((struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct*)_tmpE1)->f4;_tmpE6=((struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct*)_tmpE1)->f5;_LL56: {int v=_tmpE2;struct _fat_ptr t=_tmpE3;struct Cyc_List_List*o=_tmpE4;struct Cyc_List_List*i=_tmpE5;struct Cyc_List_List*c=_tmpE6;
# 554
({Cyc_Binding_resolve_asm_iolist(env,o);});
({Cyc_Binding_resolve_asm_iolist(env,i);});
goto _LL0;}}_LL0:;}struct _tuple13{struct _fat_ptr f1;struct Cyc_Absyn_Exp*f2;};
# 560
void Cyc_Binding_resolve_asm_iolist(struct Cyc_Binding_Env*env,struct Cyc_List_List*l){
while((unsigned)l){
struct _tuple13*_stmttmpE=(struct _tuple13*)l->hd;struct Cyc_Absyn_Exp*_tmp171;_LL1: _tmp171=_stmttmpE->f2;_LL2: {struct Cyc_Absyn_Exp*e=_tmp171;
({Cyc_Binding_resolve_exp(env,e);});
l=l->tl;}}}
# 568
void Cyc_Binding_resolve_scs(struct Cyc_Binding_Env*env,struct Cyc_List_List*scs){
struct Cyc_Dict_Dict old_locals=*((struct Cyc_Dict_Dict*)_check_null(env->local_vars));
for(0;scs != 0;scs=scs->tl){
struct Cyc_Absyn_Switch_clause*_stmttmpF=(struct Cyc_Absyn_Switch_clause*)scs->hd;struct Cyc_Absyn_Stmt*_tmp175;struct Cyc_Absyn_Exp*_tmp174;struct Cyc_Absyn_Pat*_tmp173;_LL1: _tmp173=_stmttmpF->pattern;_tmp174=_stmttmpF->where_clause;_tmp175=_stmttmpF->body;_LL2: {struct Cyc_Absyn_Pat*pattern=_tmp173;struct Cyc_Absyn_Exp*where_clause=_tmp174;struct Cyc_Absyn_Stmt*body=_tmp175;
({Cyc_Binding_resolve_pat(env,pattern);});
if(where_clause != 0)
({Cyc_Binding_resolve_exp(env,where_clause);});
# 573
({Cyc_Binding_resolve_stmt(env,body);});
# 576
*((struct Cyc_Dict_Dict*)_check_null(env->local_vars))=old_locals;}}
# 578
return;}
# 580
void Cyc_Binding_resolve_aggrfields(unsigned loc,struct Cyc_Binding_Env*env,struct Cyc_List_List*fs){
for(0;fs != 0;fs=fs->tl){
struct Cyc_Absyn_Aggrfield*_stmttmp10=(struct Cyc_Absyn_Aggrfield*)fs->hd;struct Cyc_Absyn_Exp*_tmp17C;struct Cyc_List_List*_tmp17B;struct Cyc_Absyn_Exp*_tmp17A;void*_tmp179;struct Cyc_Absyn_Tqual _tmp178;struct _fat_ptr*_tmp177;_LL1: _tmp177=_stmttmp10->name;_tmp178=_stmttmp10->tq;_tmp179=_stmttmp10->type;_tmp17A=_stmttmp10->width;_tmp17B=_stmttmp10->attributes;_tmp17C=_stmttmp10->requires_clause;_LL2: {struct _fat_ptr*fn=_tmp177;struct Cyc_Absyn_Tqual tq=_tmp178;void*t=_tmp179;struct Cyc_Absyn_Exp*width=_tmp17A;struct Cyc_List_List*atts=_tmp17B;struct Cyc_Absyn_Exp*requires_clause=_tmp17C;
({Cyc_Binding_resolve_type(loc,env,t);});
if(width != 0)
({Cyc_Binding_resolve_exp(env,width);});
# 584
if(requires_clause != 0)
# 587
({Cyc_Binding_resolve_exp(env,requires_clause);});}}
# 589
return;}
# 592
struct Cyc_List_List*Cyc_Binding_get_fun_vardecls(int need_va_name,unsigned loc,struct Cyc_Binding_Env*env,struct Cyc_List_List*args,struct Cyc_Absyn_VarargInfo*vai){
# 598
struct Cyc_List_List*vds=0;
for(0;args != 0;args=args->tl){
struct _tuple9*_stmttmp11=(struct _tuple9*)args->hd;void*_tmp180;struct Cyc_Absyn_Tqual _tmp17F;struct _fat_ptr*_tmp17E;_LL1: _tmp17E=_stmttmp11->f1;_tmp17F=_stmttmp11->f2;_tmp180=_stmttmp11->f3;_LL2: {struct _fat_ptr*a=_tmp17E;struct Cyc_Absyn_Tqual tq=_tmp17F;void*t=_tmp180;
if(a == 0)
continue;{
# 601
struct Cyc_Absyn_Vardecl*vd=({({struct _tuple0*_tmp3BA=({struct _tuple0*_tmp182=_cycalloc(sizeof(*_tmp182));
# 603
_tmp182->f1=Cyc_Absyn_Loc_n,_tmp182->f2=a;_tmp182;});Cyc_Absyn_new_vardecl(0U,_tmp3BA,t,0);});});
vd->tq=tq;
vds=({struct Cyc_List_List*_tmp181=_cycalloc(sizeof(*_tmp181));_tmp181->hd=vd,_tmp181->tl=vds;_tmp181;});}}}
# 607
if(vai != 0){
struct Cyc_Absyn_VarargInfo _stmttmp12=*vai;int _tmp186;void*_tmp185;struct Cyc_Absyn_Tqual _tmp184;struct _fat_ptr*_tmp183;_LL4: _tmp183=_stmttmp12.name;_tmp184=_stmttmp12.tq;_tmp185=_stmttmp12.type;_tmp186=_stmttmp12.inject;_LL5: {struct _fat_ptr*v=_tmp183;struct Cyc_Absyn_Tqual tq=_tmp184;void*t=_tmp185;int i=_tmp186;
if(v == 0){
if(need_va_name)
({struct Cyc_Warn_String_Warn_Warg_struct _tmp188=({struct Cyc_Warn_String_Warn_Warg_struct _tmp329;_tmp329.tag=0U,({struct _fat_ptr _tmp3BB=({const char*_tmp189="missing name for vararg";_tag_fat(_tmp189,sizeof(char),24U);});_tmp329.f1=_tmp3BB;});_tmp329;});void*_tmp187[1U];_tmp187[0]=& _tmp188;({unsigned _tmp3BC=loc;Cyc_Warn_err2(_tmp3BC,_tag_fat(_tmp187,sizeof(void*),1U));});});}else{
# 614
void*typ=({Cyc_Absyn_fatptr_type(t,Cyc_Absyn_heap_rgn_type,tq,Cyc_Absyn_false_type);});
struct Cyc_Absyn_Vardecl*vd=({({struct _tuple0*_tmp3BD=({struct _tuple0*_tmp18B=_cycalloc(sizeof(*_tmp18B));_tmp18B->f1=Cyc_Absyn_Loc_n,_tmp18B->f2=v;_tmp18B;});Cyc_Absyn_new_vardecl(0U,_tmp3BD,typ,0);});});
vds=({struct Cyc_List_List*_tmp18A=_cycalloc(sizeof(*_tmp18A));_tmp18A->hd=vd,_tmp18A->tl=vds;_tmp18A;});}}}
# 607
vds=({Cyc_List_imp_rev(vds);});
# 620
return vds;}
# 623
void Cyc_Binding_resolve_qvar_throws(unsigned loc,struct Cyc_Binding_Env*env,struct _tuple0*q){
# 625
struct _handler_cons _tmp18D;_push_handler(& _tmp18D);{int _tmp18F=0;if(setjmp(_tmp18D.handler))_tmp18F=1;if(!_tmp18F){{void*_stmttmp13=({Cyc_Binding_lookup_ordinary(loc,env,q);});void*_tmp190=_stmttmp13;struct Cyc_Absyn_Datatypefield*_tmp192;struct Cyc_Absyn_Datatypedecl*_tmp191;if(((struct Cyc_Binding_DatatypeRes_Binding_Resolved_struct*)_tmp190)->tag == 2U){_LL1: _tmp191=((struct Cyc_Binding_DatatypeRes_Binding_Resolved_struct*)_tmp190)->f1;_tmp192=((struct Cyc_Binding_DatatypeRes_Binding_Resolved_struct*)_tmp190)->f2;_LL2: {struct Cyc_Absyn_Datatypedecl*tud=_tmp191;struct Cyc_Absyn_Datatypefield*tuf=_tmp192;
# 627
_npop_handler(0U);return;}}else{_LL3: _LL4:
# 629
({struct Cyc_Warn_String_Warn_Warg_struct _tmp194=({struct Cyc_Warn_String_Warn_Warg_struct _tmp32B;_tmp32B.tag=0U,({struct _fat_ptr _tmp3BE=({const char*_tmp196="bad occurrence of  name ";_tag_fat(_tmp196,sizeof(char),25U);});_tmp32B.f1=_tmp3BE;});_tmp32B;});struct Cyc_Warn_Qvar_Warn_Warg_struct _tmp195=({struct Cyc_Warn_Qvar_Warn_Warg_struct _tmp32A;_tmp32A.tag=3U,_tmp32A.f1=q;_tmp32A;});void*_tmp193[2U];_tmp193[0]=& _tmp194,_tmp193[1]=& _tmp195;({unsigned _tmp3BF=loc;Cyc_Warn_err2(_tmp3BF,_tag_fat(_tmp193,sizeof(void*),2U));});});_npop_handler(0U);return;}_LL0:;}
# 625
;_pop_handler();}else{void*_tmp18E=(void*)Cyc_Core_get_exn_thrown();void*_tmp197=_tmp18E;void*_tmp198;if(((struct Cyc_Binding_BindingError_exn_struct*)_tmp197)->tag == Cyc_Binding_BindingError){_LL6: _LL7:
# 634
({struct Cyc_Warn_String_Warn_Warg_struct _tmp19A=({struct Cyc_Warn_String_Warn_Warg_struct _tmp32D;_tmp32D.tag=0U,({struct _fat_ptr _tmp3C0=({const char*_tmp19C="undeclared identifier ";_tag_fat(_tmp19C,sizeof(char),23U);});_tmp32D.f1=_tmp3C0;});_tmp32D;});struct Cyc_Warn_Qvar_Warn_Warg_struct _tmp19B=({struct Cyc_Warn_Qvar_Warn_Warg_struct _tmp32C;_tmp32C.tag=3U,_tmp32C.f1=q;_tmp32C;});void*_tmp199[2U];_tmp199[0]=& _tmp19A,_tmp199[1]=& _tmp19B;({unsigned _tmp3C1=loc;Cyc_Warn_err2(_tmp3C1,_tag_fat(_tmp199,sizeof(void*),2U));});});
return;}else{_LL8: _tmp198=_tmp197;_LL9: {void*exn=_tmp198;(int)_rethrow(exn);}}_LL5:;}}}
# 639
void Cyc_Binding_resolve_function_stuff(unsigned loc,struct Cyc_Binding_Env*env,struct Cyc_Absyn_FnInfo i){
# 642
({Cyc_Absyn_resolve_effects(loc,i.ieffect,i.oeffect);});
({({void(*_tmp3C4)(unsigned loc,struct Cyc_List_List*throws,void(*check)(unsigned loc,struct Cyc_Binding_Env*,struct _tuple0*),struct Cyc_Binding_Env*p)=({void(*_tmp19E)(unsigned loc,struct Cyc_List_List*throws,void(*check)(unsigned loc,struct Cyc_Binding_Env*,struct _tuple0*),struct Cyc_Binding_Env*p)=(void(*)(unsigned loc,struct Cyc_List_List*throws,void(*check)(unsigned loc,struct Cyc_Binding_Env*,struct _tuple0*),struct Cyc_Binding_Env*p))Cyc_Absyn_resolve_throws;_tmp19E;});unsigned _tmp3C3=loc;struct Cyc_List_List*_tmp3C2=i.throws;_tmp3C4(_tmp3C3,_tmp3C2,Cyc_Binding_resolve_qvar_throws,env);});});
# 645
if(i.effect != 0)
# 647
({Cyc_Binding_resolve_type(loc,env,i.effect);});
# 645
({Cyc_Binding_resolve_type(loc,env,i.ret_type);});
# 650
{struct Cyc_List_List*args=i.args;for(0;args != 0;args=args->tl){
({Cyc_Binding_resolve_type(loc,env,(*((struct _tuple9*)args->hd)).f3);});}}
if(i.cyc_varargs != 0)
({Cyc_Binding_resolve_type(loc,env,(i.cyc_varargs)->type);});
# 652
({Cyc_Binding_resolve_rgnpo(loc,env,i.rgn_po);});
# 657
if(i.requires_clause != 0)
({Cyc_Binding_resolve_exp(env,i.requires_clause);});
# 657
if(i.ensures_clause != 0){
# 661
struct Cyc_Dict_Dict locs=*((struct Cyc_Dict_Dict*)_check_null(env->local_vars));
struct _fat_ptr*v=({struct _fat_ptr*_tmp1A4=_cycalloc(sizeof(*_tmp1A4));({struct _fat_ptr _tmp3C5=({const char*_tmp1A3="return_value";_tag_fat(_tmp1A3,sizeof(char),13U);});*_tmp1A4=_tmp3C5;});_tmp1A4;});
struct Cyc_Absyn_Vardecl*vd=({({struct _tuple0*_tmp3C6=({struct _tuple0*_tmp1A2=_cycalloc(sizeof(*_tmp1A2));_tmp1A2->f1=Cyc_Absyn_Loc_n,_tmp1A2->f2=v;_tmp1A2;});Cyc_Absyn_new_vardecl(0U,_tmp3C6,i.ret_type,0);});});
({struct Cyc_Dict_Dict _tmp3CB=({({struct Cyc_Dict_Dict(*_tmp3CA)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,void*v)=({struct Cyc_Dict_Dict(*_tmp19F)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,void*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,void*v))Cyc_Dict_insert;_tmp19F;});struct Cyc_Dict_Dict _tmp3C9=*((struct Cyc_Dict_Dict*)_check_null(env->local_vars));struct _fat_ptr*_tmp3C8=v;_tmp3CA(_tmp3C9,_tmp3C8,(void*)({struct Cyc_Binding_VarRes_Binding_Resolved_struct*_tmp1A1=_cycalloc(sizeof(*_tmp1A1));_tmp1A1->tag=0U,({void*_tmp3C7=(void*)({struct Cyc_Absyn_Local_b_Absyn_Binding_struct*_tmp1A0=_cycalloc(sizeof(*_tmp1A0));_tmp1A0->tag=4U,_tmp1A0->f1=vd;_tmp1A0;});_tmp1A1->f1=_tmp3C7;});_tmp1A1;}));});});*((struct Cyc_Dict_Dict*)_check_null(env->local_vars))=_tmp3CB;});
# 666
({Cyc_Binding_resolve_exp(env,i.ensures_clause);});
*((struct Cyc_Dict_Dict*)_check_null(env->local_vars))=locs;}}struct _tuple14{struct Cyc_Absyn_Tqual f1;void*f2;};
# 676
void Cyc_Binding_resolve_type(unsigned loc,struct Cyc_Binding_Env*env,void*t){
void*_tmp1A6=t;struct Cyc_Absyn_Exp*_tmp1A7;struct Cyc_Absyn_Exp*_tmp1A8;struct Cyc_Absyn_FnInfo _tmp1A9;struct Cyc_Absyn_Exp*_tmp1AB;void*_tmp1AA;struct Cyc_List_List*_tmp1AC;void*_tmp1AF;void*_tmp1AE;void*_tmp1AD;struct Cyc_Absyn_Tvar*_tmp1B0;void*_tmp1B1;struct Cyc_List_List*_tmp1B2;void***_tmp1B4;struct Cyc_Absyn_TypeDecl*_tmp1B3;struct Cyc_Absyn_Typedefdecl**_tmp1B7;struct Cyc_List_List*_tmp1B6;struct _tuple0**_tmp1B5;struct Cyc_List_List*_tmp1B9;void*_tmp1B8;struct Cyc_List_List*_tmp1BA;struct Cyc_Absyn_Enumdecl*_tmp1BC;struct _tuple0**_tmp1BB;struct Cyc_List_List*_tmp1BE;union Cyc_Absyn_AggrInfo*_tmp1BD;struct Cyc_List_List*_tmp1C0;union Cyc_Absyn_DatatypeFieldInfo*_tmp1BF;struct Cyc_List_List*_tmp1C2;union Cyc_Absyn_DatatypeInfo*_tmp1C1;switch(*((int*)_tmp1A6)){case 0U: switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp1A6)->f1)){case 20U: _LL1: _tmp1C1=(union Cyc_Absyn_DatatypeInfo*)&((struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp1A6)->f1)->f1;_tmp1C2=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp1A6)->f2;_LL2: {union Cyc_Absyn_DatatypeInfo*info=_tmp1C1;struct Cyc_List_List*targs=_tmp1C2;
# 679
for(0;targs != 0;targs=targs->tl){
({Cyc_Binding_resolve_type(loc,env,(void*)targs->hd);});}{
union Cyc_Absyn_DatatypeInfo _stmttmp14=*info;union Cyc_Absyn_DatatypeInfo _tmp1C3=_stmttmp14;int _tmp1C5;struct _tuple0*_tmp1C4;if((_tmp1C3.UnknownDatatype).tag == 1){_LL26: _tmp1C4=((_tmp1C3.UnknownDatatype).val).name;_tmp1C5=((_tmp1C3.UnknownDatatype).val).is_extensible;_LL27: {struct _tuple0*qv=_tmp1C4;int b=_tmp1C5;
# 683
struct _handler_cons _tmp1C6;_push_handler(& _tmp1C6);{int _tmp1C8=0;if(setjmp(_tmp1C6.handler))_tmp1C8=1;if(!_tmp1C8){
{struct Cyc_Absyn_Datatypedecl*tud=(struct Cyc_Absyn_Datatypedecl*)({({struct Cyc_List_List*(*_tmp3CE)(unsigned loc,struct Cyc_Binding_NSCtxt*ctxt,struct _tuple0*qv,struct Cyc_List_List*(*lookup)(struct Cyc_Binding_ResolveNSEnv*,struct _fat_ptr*))=({struct Cyc_List_List*(*_tmp1C9)(unsigned loc,struct Cyc_Binding_NSCtxt*ctxt,struct _tuple0*qv,struct Cyc_List_List*(*lookup)(struct Cyc_Binding_ResolveNSEnv*,struct _fat_ptr*))=(struct Cyc_List_List*(*)(unsigned loc,struct Cyc_Binding_NSCtxt*ctxt,struct _tuple0*qv,struct Cyc_List_List*(*lookup)(struct Cyc_Binding_ResolveNSEnv*,struct _fat_ptr*)))Cyc_Binding_resolve_lookup;_tmp1C9;});unsigned _tmp3CD=loc;struct Cyc_Binding_NSCtxt*_tmp3CC=env->ns;_tmp3CE(_tmp3CD,_tmp3CC,qv,Cyc_Binding_lookup_datatypedecl);});})->hd;
({union Cyc_Absyn_DatatypeInfo _tmp3CF=({Cyc_Absyn_UnknownDatatype(({struct Cyc_Absyn_UnknownDatatypeInfo _tmp32E;_tmp32E.name=tud->name,_tmp32E.is_extensible=b;_tmp32E;}));});*info=_tmp3CF;});
_npop_handler(0U);return;}
# 684
;_pop_handler();}else{void*_tmp1C7=(void*)Cyc_Core_get_exn_thrown();void*_tmp1CA=_tmp1C7;void*_tmp1CB;if(((struct Cyc_Binding_BindingError_exn_struct*)_tmp1CA)->tag == Cyc_Binding_BindingError){_LL2B: _LL2C:
# 688
({Cyc_Binding_absolutize_topdecl(loc,env,qv,Cyc_Absyn_Public);});return;}else{_LL2D: _tmp1CB=_tmp1CA;_LL2E: {void*exn=_tmp1CB;(int)_rethrow(exn);}}_LL2A:;}}}}else{_LL28: _LL29:
 return;}_LL25:;}}case 21U: _LL3: _tmp1BF=(union Cyc_Absyn_DatatypeFieldInfo*)&((struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp1A6)->f1)->f1;_tmp1C0=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp1A6)->f2;_LL4: {union Cyc_Absyn_DatatypeFieldInfo*info=_tmp1BF;struct Cyc_List_List*targs=_tmp1C0;
# 692
for(0;targs != 0;targs=targs->tl){
({Cyc_Binding_resolve_type(loc,env,(void*)targs->hd);});}{
union Cyc_Absyn_DatatypeFieldInfo _stmttmp15=*info;union Cyc_Absyn_DatatypeFieldInfo _tmp1CC=_stmttmp15;int _tmp1CF;struct _tuple0*_tmp1CE;struct _tuple0*_tmp1CD;if((_tmp1CC.UnknownDatatypefield).tag == 1){_LL30: _tmp1CD=((_tmp1CC.UnknownDatatypefield).val).datatype_name;_tmp1CE=((_tmp1CC.UnknownDatatypefield).val).field_name;_tmp1CF=((_tmp1CC.UnknownDatatypefield).val).is_extensible;_LL31: {struct _tuple0*qvd=_tmp1CD;struct _tuple0*qvf=_tmp1CE;int b=_tmp1CF;
# 698
{struct _handler_cons _tmp1D0;_push_handler(& _tmp1D0);{int _tmp1D2=0;if(setjmp(_tmp1D0.handler))_tmp1D2=1;if(!_tmp1D2){
{void*_stmttmp16=({Cyc_Binding_lookup_ordinary(loc,env,qvf);});void*_tmp1D3=_stmttmp16;struct Cyc_Absyn_Datatypefield*_tmp1D5;struct Cyc_Absyn_Datatypedecl*_tmp1D4;if(((struct Cyc_Binding_DatatypeRes_Binding_Resolved_struct*)_tmp1D3)->tag == 2U){_LL35: _tmp1D4=((struct Cyc_Binding_DatatypeRes_Binding_Resolved_struct*)_tmp1D3)->f1;_tmp1D5=((struct Cyc_Binding_DatatypeRes_Binding_Resolved_struct*)_tmp1D3)->f2;_LL36: {struct Cyc_Absyn_Datatypedecl*tud=_tmp1D4;struct Cyc_Absyn_Datatypefield*tuf=_tmp1D5;
# 701
struct Cyc_Absyn_Datatypedecl*tud2=(struct Cyc_Absyn_Datatypedecl*)({({struct Cyc_List_List*(*_tmp3D2)(unsigned loc,struct Cyc_Binding_NSCtxt*ctxt,struct _tuple0*qv,struct Cyc_List_List*(*lookup)(struct Cyc_Binding_ResolveNSEnv*,struct _fat_ptr*))=({struct Cyc_List_List*(*_tmp1E0)(unsigned loc,struct Cyc_Binding_NSCtxt*ctxt,struct _tuple0*qv,struct Cyc_List_List*(*lookup)(struct Cyc_Binding_ResolveNSEnv*,struct _fat_ptr*))=(struct Cyc_List_List*(*)(unsigned loc,struct Cyc_Binding_NSCtxt*ctxt,struct _tuple0*qv,struct Cyc_List_List*(*lookup)(struct Cyc_Binding_ResolveNSEnv*,struct _fat_ptr*)))Cyc_Binding_resolve_lookup;_tmp1E0;});unsigned _tmp3D1=loc;struct Cyc_Binding_NSCtxt*_tmp3D0=env->ns;_tmp3D2(_tmp3D1,_tmp3D0,qvd,Cyc_Binding_lookup_datatypedecl);});})->hd;
if(({Cyc_Absyn_qvar_cmp(tud->name,tud2->name);})!= 0){
({struct Cyc_Warn_Qvar_Warn_Warg_struct _tmp1D7=({struct Cyc_Warn_Qvar_Warn_Warg_struct _tmp334;_tmp334.tag=3U,_tmp334.f1=tuf->name;_tmp334;});struct Cyc_Warn_String_Warn_Warg_struct _tmp1D8=({struct Cyc_Warn_String_Warn_Warg_struct _tmp333;_tmp333.tag=0U,({struct _fat_ptr _tmp3D3=({const char*_tmp1DF=" is a variant of ";_tag_fat(_tmp1DF,sizeof(char),18U);});_tmp333.f1=_tmp3D3;});_tmp333;});struct Cyc_Warn_Qvar_Warn_Warg_struct _tmp1D9=({struct Cyc_Warn_Qvar_Warn_Warg_struct _tmp332;_tmp332.tag=3U,_tmp332.f1=tud2->name;_tmp332;});struct Cyc_Warn_String_Warn_Warg_struct _tmp1DA=({struct Cyc_Warn_String_Warn_Warg_struct _tmp331;_tmp331.tag=0U,({struct _fat_ptr _tmp3D4=({const char*_tmp1DE=" not ";_tag_fat(_tmp1DE,sizeof(char),6U);});_tmp331.f1=_tmp3D4;});_tmp331;});struct Cyc_Warn_Qvar_Warn_Warg_struct _tmp1DB=({struct Cyc_Warn_Qvar_Warn_Warg_struct _tmp330;_tmp330.tag=3U,_tmp330.f1=tud->name;_tmp330;});struct Cyc_Warn_String_Warn_Warg_struct _tmp1DC=({struct Cyc_Warn_String_Warn_Warg_struct _tmp32F;_tmp32F.tag=0U,({
struct _fat_ptr _tmp3D5=({const char*_tmp1DD=" (shadowing not yet implemented properly)";_tag_fat(_tmp1DD,sizeof(char),42U);});_tmp32F.f1=_tmp3D5;});_tmp32F;});void*_tmp1D6[6U];_tmp1D6[0]=& _tmp1D7,_tmp1D6[1]=& _tmp1D8,_tmp1D6[2]=& _tmp1D9,_tmp1D6[3]=& _tmp1DA,_tmp1D6[4]=& _tmp1DB,_tmp1D6[5]=& _tmp1DC;({unsigned _tmp3D6=loc;Cyc_Warn_err2(_tmp3D6,_tag_fat(_tmp1D6,sizeof(void*),6U));});});
_npop_handler(0U);return;}
# 702
({union Cyc_Absyn_DatatypeFieldInfo _tmp3D7=({Cyc_Absyn_UnknownDatatypefield(({struct Cyc_Absyn_UnknownDatatypeFieldInfo _tmp335;_tmp335.datatype_name=tud->name,_tmp335.field_name=tuf->name,_tmp335.is_extensible=b;_tmp335;}));});*info=_tmp3D7;});
# 709
_npop_handler(0U);return;}}else{_LL37: _LL38:
 goto _LL34;}_LL34:;}
# 699
;_pop_handler();}else{void*_tmp1D1=(void*)Cyc_Core_get_exn_thrown();void*_tmp1E1=_tmp1D1;void*_tmp1E2;if(((struct Cyc_Binding_BindingError_exn_struct*)_tmp1E1)->tag == Cyc_Binding_BindingError){_LL3A: _LL3B:
# 712
 goto _LL39;}else{_LL3C: _tmp1E2=_tmp1E1;_LL3D: {void*exn=_tmp1E2;(int)_rethrow(exn);}}_LL39:;}}}
({struct Cyc_Warn_Qvar_Warn_Warg_struct _tmp1E4=({struct Cyc_Warn_Qvar_Warn_Warg_struct _tmp337;_tmp337.tag=3U,_tmp337.f1=qvf;_tmp337;});struct Cyc_Warn_String_Warn_Warg_struct _tmp1E5=({struct Cyc_Warn_String_Warn_Warg_struct _tmp336;_tmp336.tag=0U,({struct _fat_ptr _tmp3D8=({const char*_tmp1E6=" is not a datatype field";_tag_fat(_tmp1E6,sizeof(char),25U);});_tmp336.f1=_tmp3D8;});_tmp336;});void*_tmp1E3[2U];_tmp1E3[0]=& _tmp1E4,_tmp1E3[1]=& _tmp1E5;({unsigned _tmp3D9=loc;Cyc_Warn_err2(_tmp3D9,_tag_fat(_tmp1E3,sizeof(void*),2U));});});return;}}else{_LL32: _LL33:
 return;}_LL2F:;}}case 22U: _LL5: _tmp1BD=(union Cyc_Absyn_AggrInfo*)&((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp1A6)->f1)->f1;_tmp1BE=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp1A6)->f2;_LL6: {union Cyc_Absyn_AggrInfo*info=_tmp1BD;struct Cyc_List_List*targs=_tmp1BE;
# 718
for(0;targs != 0;targs=targs->tl){
({Cyc_Binding_resolve_type(loc,env,(void*)targs->hd);});}{
union Cyc_Absyn_AggrInfo _stmttmp17=*info;union Cyc_Absyn_AggrInfo _tmp1E7=_stmttmp17;struct Cyc_Core_Opt*_tmp1EA;struct _tuple0*_tmp1E9;enum Cyc_Absyn_AggrKind _tmp1E8;if((_tmp1E7.UnknownAggr).tag == 1){_LL3F: _tmp1E8=((_tmp1E7.UnknownAggr).val).f1;_tmp1E9=((_tmp1E7.UnknownAggr).val).f2;_tmp1EA=((_tmp1E7.UnknownAggr).val).f3;_LL40: {enum Cyc_Absyn_AggrKind ak=_tmp1E8;struct _tuple0*qv=_tmp1E9;struct Cyc_Core_Opt*bo=_tmp1EA;
# 722
struct _handler_cons _tmp1EB;_push_handler(& _tmp1EB);{int _tmp1ED=0;if(setjmp(_tmp1EB.handler))_tmp1ED=1;if(!_tmp1ED){
{struct Cyc_Absyn_Aggrdecl*ad=({({struct Cyc_Absyn_Aggrdecl*(*_tmp3DC)(unsigned loc,struct Cyc_Binding_NSCtxt*ctxt,struct _tuple0*qv,struct Cyc_Absyn_Aggrdecl*(*lookup)(struct Cyc_Binding_ResolveNSEnv*,struct _fat_ptr*))=({struct Cyc_Absyn_Aggrdecl*(*_tmp1F4)(unsigned loc,struct Cyc_Binding_NSCtxt*ctxt,struct _tuple0*qv,struct Cyc_Absyn_Aggrdecl*(*lookup)(struct Cyc_Binding_ResolveNSEnv*,struct _fat_ptr*))=(struct Cyc_Absyn_Aggrdecl*(*)(unsigned loc,struct Cyc_Binding_NSCtxt*ctxt,struct _tuple0*qv,struct Cyc_Absyn_Aggrdecl*(*lookup)(struct Cyc_Binding_ResolveNSEnv*,struct _fat_ptr*)))Cyc_Binding_resolve_lookup;_tmp1F4;});unsigned _tmp3DB=loc;struct Cyc_Binding_NSCtxt*_tmp3DA=env->ns;_tmp3DC(_tmp3DB,_tmp3DA,qv,Cyc_Binding_lookup_aggrdecl);});});
if((int)ad->kind != (int)ak)
({struct Cyc_Warn_String_Warn_Warg_struct _tmp1EF=({struct Cyc_Warn_String_Warn_Warg_struct _tmp338;_tmp338.tag=0U,({struct _fat_ptr _tmp3DD=({const char*_tmp1F0="struct vs. union mismatch with earlier declaration";_tag_fat(_tmp1F0,sizeof(char),51U);});_tmp338.f1=_tmp3DD;});_tmp338;});void*_tmp1EE[1U];_tmp1EE[0]=& _tmp1EF;({unsigned _tmp3DE=loc;Cyc_Warn_err2(_tmp3DE,_tag_fat(_tmp1EE,sizeof(void*),1U));});});
# 724
if(
# 726
(((int)ak == (int)1U && bo != 0)&& ad->impl != 0)&&((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->tagged != (int)bo->v)
# 728
({struct Cyc_Warn_String_Warn_Warg_struct _tmp1F2=({struct Cyc_Warn_String_Warn_Warg_struct _tmp339;_tmp339.tag=0U,({struct _fat_ptr _tmp3DF=({const char*_tmp1F3="@tagged mismatch with earlier declaration";_tag_fat(_tmp1F3,sizeof(char),42U);});_tmp339.f1=_tmp3DF;});_tmp339;});void*_tmp1F1[1U];_tmp1F1[0]=& _tmp1F2;({unsigned _tmp3E0=loc;Cyc_Warn_err2(_tmp3E0,_tag_fat(_tmp1F1,sizeof(void*),1U));});});
# 724
({union Cyc_Absyn_AggrInfo _tmp3E1=({Cyc_Absyn_UnknownAggr(ak,ad->name,bo);});*info=_tmp3E1;});
# 730
_npop_handler(0U);return;}
# 723
;_pop_handler();}else{void*_tmp1EC=(void*)Cyc_Core_get_exn_thrown();void*_tmp1F5=_tmp1EC;void*_tmp1F6;if(((struct Cyc_Binding_BindingError_exn_struct*)_tmp1F5)->tag == Cyc_Binding_BindingError){_LL44: _LL45:
# 732
({Cyc_Binding_absolutize_topdecl(loc,env,qv,Cyc_Absyn_Public);});return;}else{_LL46: _tmp1F6=_tmp1F5;_LL47: {void*exn=_tmp1F6;(int)_rethrow(exn);}}_LL43:;}}}}else{_LL41: _LL42:
 return;}_LL3E:;}}case 17U: _LL7: _tmp1BB=(struct _tuple0**)&((struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp1A6)->f1)->f1;_tmp1BC=((struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp1A6)->f1)->f2;_LL8: {struct _tuple0**qv=_tmp1BB;struct Cyc_Absyn_Enumdecl*edo=_tmp1BC;
# 736
if(edo != 0)
return;{
# 736
struct _handler_cons _tmp1F7;_push_handler(& _tmp1F7);{int _tmp1F9=0;if(setjmp(_tmp1F7.handler))_tmp1F9=1;if(!_tmp1F9){
# 739
{struct Cyc_Absyn_Enumdecl*ed=({({struct Cyc_Absyn_Enumdecl*(*_tmp3E4)(unsigned loc,struct Cyc_Binding_NSCtxt*ctxt,struct _tuple0*qv,struct Cyc_Absyn_Enumdecl*(*lookup)(struct Cyc_Binding_ResolveNSEnv*,struct _fat_ptr*))=({struct Cyc_Absyn_Enumdecl*(*_tmp1FA)(unsigned loc,struct Cyc_Binding_NSCtxt*ctxt,struct _tuple0*qv,struct Cyc_Absyn_Enumdecl*(*lookup)(struct Cyc_Binding_ResolveNSEnv*,struct _fat_ptr*))=(struct Cyc_Absyn_Enumdecl*(*)(unsigned loc,struct Cyc_Binding_NSCtxt*ctxt,struct _tuple0*qv,struct Cyc_Absyn_Enumdecl*(*lookup)(struct Cyc_Binding_ResolveNSEnv*,struct _fat_ptr*)))Cyc_Binding_resolve_lookup;_tmp1FA;});unsigned _tmp3E3=loc;struct Cyc_Binding_NSCtxt*_tmp3E2=env->ns;_tmp3E4(_tmp3E3,_tmp3E2,*qv,Cyc_Binding_lookup_enumdecl);});});
*qv=ed->name;
_npop_handler(0U);return;}
# 739
;_pop_handler();}else{void*_tmp1F8=(void*)Cyc_Core_get_exn_thrown();void*_tmp1FB=_tmp1F8;void*_tmp1FC;if(((struct Cyc_Binding_BindingError_exn_struct*)_tmp1FB)->tag == Cyc_Binding_BindingError){_LL49: _LL4A:
# 743
({Cyc_Binding_absolutize_topdecl(loc,env,*qv,Cyc_Absyn_Public);});return;}else{_LL4B: _tmp1FC=_tmp1FB;_LL4C: {void*exn=_tmp1FC;(int)_rethrow(exn);}}_LL48:;}}}}case 18U: _LLF: _tmp1BA=((struct Cyc_Absyn_AnonEnumCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp1A6)->f1)->f1;_LL10: {struct Cyc_List_List*fs=_tmp1BA;
# 781
struct Cyc_Binding_ResolveNSEnv*decl_ns_data=({struct Cyc_Binding_ResolveNSEnv*(*_tmp213)(struct Cyc_Binding_NSCtxt*ctxt,union Cyc_Absyn_Nmspace abs_ns)=({struct Cyc_Binding_ResolveNSEnv*(*_tmp214)(struct Cyc_Binding_NSCtxt*ctxt,union Cyc_Absyn_Nmspace abs_ns)=(struct Cyc_Binding_ResolveNSEnv*(*)(struct Cyc_Binding_NSCtxt*ctxt,union Cyc_Absyn_Nmspace abs_ns))Cyc_Binding_get_ns_data;_tmp214;});struct Cyc_Binding_NSCtxt*_tmp215=env->ns;union Cyc_Absyn_Nmspace _tmp216=({union Cyc_Absyn_Nmspace(*_tmp217)(struct Cyc_List_List*ns,int C_scope)=Cyc_Absyn_Abs_n;struct Cyc_List_List*_tmp218=(env->ns)->curr_ns;int _tmp219=({Cyc_Binding_in_cinclude(env);});_tmp217(_tmp218,_tmp219);});_tmp213(_tmp215,_tmp216);});
# 783
for(0;fs != 0;fs=fs->tl){
struct Cyc_Absyn_Enumfield*f=(struct Cyc_Absyn_Enumfield*)fs->hd;
if(f->tag != 0)
({Cyc_Binding_resolve_exp(env,(struct Cyc_Absyn_Exp*)_check_null(f->tag));});
# 785
({struct Cyc_Dict_Dict _tmp3E8=({({struct Cyc_Dict_Dict(*_tmp3E7)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,void*v)=({
# 787
struct Cyc_Dict_Dict(*_tmp211)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,void*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,void*v))Cyc_Dict_insert;_tmp211;});struct Cyc_Dict_Dict _tmp3E6=decl_ns_data->ordinaries;struct _fat_ptr*_tmp3E5=(*f->name).f2;_tmp3E7(_tmp3E6,_tmp3E5,(void*)({struct Cyc_Binding_AnonEnumRes_Binding_Resolved_struct*_tmp212=_cycalloc(sizeof(*_tmp212));_tmp212->tag=4U,_tmp212->f1=t,_tmp212->f2=f;_tmp212;}));});});
# 785
decl_ns_data->ordinaries=_tmp3E8;});}
# 789
return;}default: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp1A6)->f2 == 0){_LL15: _LL16:
# 793
 return;}else{_LL17: _tmp1B8=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp1A6)->f1;_tmp1B9=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp1A6)->f2;_LL18: {void*c=_tmp1B8;struct Cyc_List_List*ts=_tmp1B9;
# 795
for(0;ts != 0;ts=ts->tl){
({Cyc_Binding_resolve_type(loc,env,(void*)ts->hd);});}
return;}}}case 8U: _LL9: _tmp1B5=(struct _tuple0**)&((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp1A6)->f1;_tmp1B6=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp1A6)->f2;_tmp1B7=(struct Cyc_Absyn_Typedefdecl**)&((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp1A6)->f3;_LLA: {struct _tuple0**tdn=_tmp1B5;struct Cyc_List_List*targs=_tmp1B6;struct Cyc_Absyn_Typedefdecl**tdo=_tmp1B7;
# 746
for(0;targs != 0;targs=targs->tl){
({Cyc_Binding_resolve_type(loc,env,(void*)targs->hd);});}
{struct _handler_cons _tmp1FD;_push_handler(& _tmp1FD);{int _tmp1FF=0;if(setjmp(_tmp1FD.handler))_tmp1FF=1;if(!_tmp1FF){
{struct Cyc_Absyn_Typedefdecl*td=({({struct Cyc_Absyn_Typedefdecl*(*_tmp3EB)(unsigned loc,struct Cyc_Binding_NSCtxt*ctxt,struct _tuple0*qv,struct Cyc_Absyn_Typedefdecl*(*lookup)(struct Cyc_Binding_ResolveNSEnv*,struct _fat_ptr*))=({struct Cyc_Absyn_Typedefdecl*(*_tmp200)(unsigned loc,struct Cyc_Binding_NSCtxt*ctxt,struct _tuple0*qv,struct Cyc_Absyn_Typedefdecl*(*lookup)(struct Cyc_Binding_ResolveNSEnv*,struct _fat_ptr*))=(struct Cyc_Absyn_Typedefdecl*(*)(unsigned loc,struct Cyc_Binding_NSCtxt*ctxt,struct _tuple0*qv,struct Cyc_Absyn_Typedefdecl*(*lookup)(struct Cyc_Binding_ResolveNSEnv*,struct _fat_ptr*)))Cyc_Binding_resolve_lookup;_tmp200;});unsigned _tmp3EA=loc;struct Cyc_Binding_NSCtxt*_tmp3E9=env->ns;_tmp3EB(_tmp3EA,_tmp3E9,*tdn,Cyc_Binding_lookup_typedefdecl);});});
# 751
*tdn=td->name;
_npop_handler(0U);return;}
# 749
;_pop_handler();}else{void*_tmp1FE=(void*)Cyc_Core_get_exn_thrown();void*_tmp201=_tmp1FE;void*_tmp202;if(((struct Cyc_Binding_BindingError_exn_struct*)_tmp201)->tag == Cyc_Binding_BindingError){_LL4E: _LL4F:
# 753
 goto _LL4D;}else{_LL50: _tmp202=_tmp201;_LL51: {void*exn=_tmp202;(int)_rethrow(exn);}}_LL4D:;}}}
({struct Cyc_Warn_String_Warn_Warg_struct _tmp204=({struct Cyc_Warn_String_Warn_Warg_struct _tmp33B;_tmp33B.tag=0U,({struct _fat_ptr _tmp3EC=({const char*_tmp206="unbound typdef name ";_tag_fat(_tmp206,sizeof(char),21U);});_tmp33B.f1=_tmp3EC;});_tmp33B;});struct Cyc_Warn_Qvar_Warn_Warg_struct _tmp205=({struct Cyc_Warn_Qvar_Warn_Warg_struct _tmp33A;_tmp33A.tag=3U,_tmp33A.f1=*tdn;_tmp33A;});void*_tmp203[2U];_tmp203[0]=& _tmp204,_tmp203[1]=& _tmp205;({unsigned _tmp3ED=loc;Cyc_Warn_err2(_tmp3ED,_tag_fat(_tmp203,sizeof(void*),2U));});});
return;}case 10U: _LLB: _tmp1B3=((struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_tmp1A6)->f1;_tmp1B4=(void***)&((struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_tmp1A6)->f2;_LLC: {struct Cyc_Absyn_TypeDecl*td=_tmp1B3;void***to=_tmp1B4;
# 759
struct Cyc_Dict_Dict*old_locals=env->local_vars;
env->local_vars=0;
{void*_stmttmp18=td->r;void*_tmp207=_stmttmp18;struct Cyc_Absyn_Datatypedecl*_tmp208;struct Cyc_Absyn_Enumdecl*_tmp209;struct Cyc_Absyn_Aggrdecl*_tmp20A;switch(*((int*)_tmp207)){case 0U: _LL53: _tmp20A=((struct Cyc_Absyn_Aggr_td_Absyn_Raw_typedecl_struct*)_tmp207)->f1;_LL54: {struct Cyc_Absyn_Aggrdecl*ad=_tmp20A;
# 763
({({struct Cyc_Binding_Env*_tmp3EF=env;Cyc_Binding_resolve_decl(_tmp3EF,({struct Cyc_Absyn_Decl*_tmp20C=_cycalloc(sizeof(*_tmp20C));({void*_tmp3EE=(void*)({struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct*_tmp20B=_cycalloc(sizeof(*_tmp20B));_tmp20B->tag=5U,_tmp20B->f1=ad;_tmp20B;});_tmp20C->r=_tmp3EE;}),_tmp20C->loc=td->loc;_tmp20C;}));});});goto _LL52;}case 1U: _LL55: _tmp209=((struct Cyc_Absyn_Enum_td_Absyn_Raw_typedecl_struct*)_tmp207)->f1;_LL56: {struct Cyc_Absyn_Enumdecl*ed=_tmp209;
# 765
({({struct Cyc_Binding_Env*_tmp3F1=env;Cyc_Binding_resolve_decl(_tmp3F1,({struct Cyc_Absyn_Decl*_tmp20E=_cycalloc(sizeof(*_tmp20E));({void*_tmp3F0=(void*)({struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct*_tmp20D=_cycalloc(sizeof(*_tmp20D));_tmp20D->tag=7U,_tmp20D->f1=ed;_tmp20D;});_tmp20E->r=_tmp3F0;}),_tmp20E->loc=td->loc;_tmp20E;}));});});goto _LL52;}default: _LL57: _tmp208=((struct Cyc_Absyn_Datatype_td_Absyn_Raw_typedecl_struct*)_tmp207)->f1;_LL58: {struct Cyc_Absyn_Datatypedecl*tud=_tmp208;
# 767
({({struct Cyc_Binding_Env*_tmp3F3=env;Cyc_Binding_resolve_decl(_tmp3F3,({struct Cyc_Absyn_Decl*_tmp210=_cycalloc(sizeof(*_tmp210));({void*_tmp3F2=(void*)({struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct*_tmp20F=_cycalloc(sizeof(*_tmp20F));_tmp20F->tag=6U,_tmp20F->f1=tud;_tmp20F;});_tmp210->r=_tmp3F2;}),_tmp210->loc=td->loc;_tmp210;}));});});goto _LL52;}}_LL52:;}
# 769
env->local_vars=old_locals;
return;}case 7U: _LLD: _tmp1B2=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp1A6)->f2;_LLE: {struct Cyc_List_List*fs=_tmp1B2;
# 776
({Cyc_Binding_resolve_aggrfields(loc,env,fs);});
return;}case 1U: _LL11: _tmp1B1=(void*)((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp1A6)->f2;_LL12: {void*to=_tmp1B1;
# 790
if(to != 0)({Cyc_Binding_resolve_type(loc,env,to);});return;}case 2U: _LL13: _tmp1B0=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp1A6)->f1;_LL14: {struct Cyc_Absyn_Tvar*tv=_tmp1B0;
# 792
return;}case 3U: _LL19: _tmp1AD=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp1A6)->f1).elt_type;_tmp1AE=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp1A6)->f1).ptr_atts).rgn;_tmp1AF=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp1A6)->f1).ptr_atts).bounds;_LL1A: {void*t1=_tmp1AD;void*t2=_tmp1AE;void*bds=_tmp1AF;
# 800
({Cyc_Binding_resolve_type(loc,env,t1);});
({Cyc_Binding_resolve_type(loc,env,t2);});
({Cyc_Binding_resolve_type(loc,env,bds);});
return;}case 6U: _LL1B: _tmp1AC=((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp1A6)->f1;_LL1C: {struct Cyc_List_List*tqts=_tmp1AC;
# 806
for(0;tqts != 0;tqts=tqts->tl){
({Cyc_Binding_resolve_type(loc,env,(*((struct _tuple14*)tqts->hd)).f2);});}
return;}case 4U: _LL1D: _tmp1AA=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp1A6)->f1).elt_type;_tmp1AB=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp1A6)->f1).num_elts;_LL1E: {void*t1=_tmp1AA;struct Cyc_Absyn_Exp*eo=_tmp1AB;
# 810
({Cyc_Binding_resolve_type(loc,env,t1);});
if(eo != 0)({Cyc_Binding_resolve_exp(env,eo);});return;}case 5U: _LL1F: _tmp1A9=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1A6)->f1;_LL20: {struct Cyc_Absyn_FnInfo i=_tmp1A9;
# 817
struct Cyc_List_List*vds=({Cyc_Binding_get_fun_vardecls(0,loc,env,i.args,i.cyc_varargs);});
# 819
struct Cyc_Dict_Dict*old_locals=env->local_vars;
if(old_locals != 0)
({struct Cyc_Dict_Dict*_tmp3F4=({struct Cyc_Dict_Dict*_tmp21A=_cycalloc(sizeof(*_tmp21A));*_tmp21A=*((struct Cyc_Dict_Dict*)_check_null(env->local_vars));_tmp21A;});env->local_vars=_tmp3F4;});else{
# 823
({struct Cyc_Dict_Dict*_tmp3F6=({struct Cyc_Dict_Dict*_tmp21C=_cycalloc(sizeof(*_tmp21C));({struct Cyc_Dict_Dict _tmp3F5=({({struct Cyc_Dict_Dict(*_tmp21B)(int(*cmp)(struct _fat_ptr*,struct _fat_ptr*))=(struct Cyc_Dict_Dict(*)(int(*cmp)(struct _fat_ptr*,struct _fat_ptr*)))Cyc_Dict_empty;_tmp21B;})(Cyc_strptrcmp);});*_tmp21C=_tmp3F5;});_tmp21C;});env->local_vars=_tmp3F6;});}
{struct Cyc_List_List*vds1=vds;for(0;vds1 != 0;vds1=vds1->tl){
({struct Cyc_Dict_Dict _tmp3FB=({({struct Cyc_Dict_Dict(*_tmp3FA)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,void*v)=({struct Cyc_Dict_Dict(*_tmp21D)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,void*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,void*v))Cyc_Dict_insert;_tmp21D;});struct Cyc_Dict_Dict _tmp3F9=*((struct Cyc_Dict_Dict*)_check_null(env->local_vars));struct _fat_ptr*_tmp3F8=(*((struct Cyc_Absyn_Vardecl*)vds1->hd)->name).f2;_tmp3FA(_tmp3F9,_tmp3F8,(void*)({struct Cyc_Binding_VarRes_Binding_Resolved_struct*_tmp21F=_cycalloc(sizeof(*_tmp21F));_tmp21F->tag=0U,({void*_tmp3F7=(void*)({struct Cyc_Absyn_Param_b_Absyn_Binding_struct*_tmp21E=_cycalloc(sizeof(*_tmp21E));_tmp21E->tag=3U,_tmp21E->f1=(struct Cyc_Absyn_Vardecl*)vds1->hd;_tmp21E;});_tmp21F->f1=_tmp3F7;});_tmp21F;}));});});*((struct Cyc_Dict_Dict*)_check_null(env->local_vars))=_tmp3FB;});}}
# 828
({Cyc_Binding_resolve_function_stuff(loc,env,i);});
env->local_vars=old_locals;
return;}case 9U: _LL21: _tmp1A8=((struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_tmp1A6)->f1;_LL22: {struct Cyc_Absyn_Exp*e=_tmp1A8;
# 832
_tmp1A7=e;goto _LL24;}default: _LL23: _tmp1A7=((struct Cyc_Absyn_TypeofType_Absyn_Type_struct*)_tmp1A6)->f1;_LL24: {struct Cyc_Absyn_Exp*e=_tmp1A7;
({Cyc_Binding_resolve_exp(env,e);});return;}}_LL0:;}struct _tuple15{struct Cyc_List_List*f1;struct Cyc_Absyn_Pat*f2;};
# 837
void Cyc_Binding_resolve_pat(struct Cyc_Binding_Env*env,struct Cyc_Absyn_Pat*p){
unsigned _tmp222;void*_tmp221;_LL1: _tmp221=p->r;_tmp222=p->loc;_LL2: {void*r=_tmp221;unsigned loc=_tmp222;
void*_tmp223=r;struct Cyc_List_List*_tmp224;struct Cyc_Absyn_Exp*_tmp225;struct Cyc_List_List*_tmp226;struct Cyc_Absyn_Pat*_tmp227;struct Cyc_Absyn_Vardecl*_tmp228;struct Cyc_Absyn_Vardecl*_tmp229;struct Cyc_Absyn_Pat*_tmp22B;struct Cyc_Absyn_Vardecl*_tmp22A;struct Cyc_Absyn_Pat*_tmp22D;struct Cyc_Absyn_Vardecl*_tmp22C;struct Cyc_List_List*_tmp22E;struct Cyc_List_List*_tmp22F;int _tmp233;struct Cyc_List_List*_tmp232;struct Cyc_List_List*_tmp231;struct _tuple0*_tmp230;int _tmp236;struct Cyc_List_List*_tmp235;struct _tuple0*_tmp234;struct _tuple0*_tmp237;switch(*((int*)_tmp223)){case 15U: _LL4: _tmp237=((struct Cyc_Absyn_UnknownId_p_Absyn_Raw_pat_struct*)_tmp223)->f1;_LL5: {struct _tuple0*qv=_tmp237;
# 841
{struct _handler_cons _tmp238;_push_handler(& _tmp238);{int _tmp23A=0;if(setjmp(_tmp238.handler))_tmp23A=1;if(!_tmp23A){{void*_stmttmp19=({Cyc_Binding_lookup_ordinary(loc,env,qv);});void*_tmp23B=_stmttmp19;struct Cyc_Absyn_Enumfield*_tmp23D;void*_tmp23C;struct Cyc_Absyn_Enumfield*_tmp23F;struct Cyc_Absyn_Enumdecl*_tmp23E;struct Cyc_Absyn_Datatypefield*_tmp241;struct Cyc_Absyn_Datatypedecl*_tmp240;switch(*((int*)_tmp23B)){case 0U: _LL2D: _LL2E:
 goto _LL2C;case 1U: _LL2F: _LL30:
# 844
({struct Cyc_Warn_String_Warn_Warg_struct _tmp243=({struct Cyc_Warn_String_Warn_Warg_struct _tmp33C;_tmp33C.tag=0U,({struct _fat_ptr _tmp3FC=({const char*_tmp244="struct tag used without arguments in pattern";_tag_fat(_tmp244,sizeof(char),45U);});_tmp33C.f1=_tmp3FC;});_tmp33C;});void*_tmp242[1U];_tmp242[0]=& _tmp243;({unsigned _tmp3FD=loc;Cyc_Warn_err2(_tmp3FD,_tag_fat(_tmp242,sizeof(void*),1U));});});_npop_handler(0U);return;case 2U: _LL31: _tmp240=((struct Cyc_Binding_DatatypeRes_Binding_Resolved_struct*)_tmp23B)->f1;_tmp241=((struct Cyc_Binding_DatatypeRes_Binding_Resolved_struct*)_tmp23B)->f2;_LL32: {struct Cyc_Absyn_Datatypedecl*tud=_tmp240;struct Cyc_Absyn_Datatypefield*tuf=_tmp241;
# 846
({void*_tmp3FE=(void*)({struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct*_tmp245=_cycalloc(sizeof(*_tmp245));_tmp245->tag=8U,_tmp245->f1=tud,_tmp245->f2=tuf,_tmp245->f3=0,_tmp245->f4=0;_tmp245;});p->r=_tmp3FE;});_npop_handler(0U);return;}case 3U: _LL33: _tmp23E=((struct Cyc_Binding_EnumRes_Binding_Resolved_struct*)_tmp23B)->f1;_tmp23F=((struct Cyc_Binding_EnumRes_Binding_Resolved_struct*)_tmp23B)->f2;_LL34: {struct Cyc_Absyn_Enumdecl*ed=_tmp23E;struct Cyc_Absyn_Enumfield*ef=_tmp23F;
# 848
({void*_tmp3FF=(void*)({struct Cyc_Absyn_Enum_p_Absyn_Raw_pat_struct*_tmp246=_cycalloc(sizeof(*_tmp246));_tmp246->tag=13U,_tmp246->f1=ed,_tmp246->f2=ef;_tmp246;});p->r=_tmp3FF;});_npop_handler(0U);return;}default: _LL35: _tmp23C=(void*)((struct Cyc_Binding_AnonEnumRes_Binding_Resolved_struct*)_tmp23B)->f1;_tmp23D=((struct Cyc_Binding_AnonEnumRes_Binding_Resolved_struct*)_tmp23B)->f2;_LL36: {void*t=_tmp23C;struct Cyc_Absyn_Enumfield*ef=_tmp23D;
# 850
({void*_tmp400=(void*)({struct Cyc_Absyn_AnonEnum_p_Absyn_Raw_pat_struct*_tmp247=_cycalloc(sizeof(*_tmp247));_tmp247->tag=14U,_tmp247->f1=t,_tmp247->f2=ef;_tmp247;});p->r=_tmp400;});_npop_handler(0U);return;}}_LL2C:;}
# 841
;_pop_handler();}else{void*_tmp239=(void*)Cyc_Core_get_exn_thrown();void*_tmp248=_tmp239;void*_tmp249;if(((struct Cyc_Binding_BindingError_exn_struct*)_tmp248)->tag == Cyc_Binding_BindingError){_LL38: _LL39:
# 851
 goto _LL37;}else{_LL3A: _tmp249=_tmp248;_LL3B: {void*exn=_tmp249;(int)_rethrow(exn);}}_LL37:;}}}{
struct Cyc_Absyn_Vardecl*vd=({Cyc_Absyn_new_vardecl(0U,qv,Cyc_Absyn_void_type,0);});
({Cyc_Binding_resolve_and_add_vardecl(loc,env,vd);});
({void*_tmp402=(void*)({struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct*_tmp24A=_cycalloc(sizeof(*_tmp24A));_tmp24A->tag=1U,_tmp24A->f1=vd,({struct Cyc_Absyn_Pat*_tmp401=({Cyc_Absyn_new_pat((void*)& Cyc_Absyn_Wild_p_val,0U);});_tmp24A->f2=_tmp401;});_tmp24A;});p->r=_tmp402;});
return;}}case 16U: _LL6: _tmp234=((struct Cyc_Absyn_UnknownCall_p_Absyn_Raw_pat_struct*)_tmp223)->f1;_tmp235=((struct Cyc_Absyn_UnknownCall_p_Absyn_Raw_pat_struct*)_tmp223)->f2;_tmp236=((struct Cyc_Absyn_UnknownCall_p_Absyn_Raw_pat_struct*)_tmp223)->f3;_LL7: {struct _tuple0*qv=_tmp234;struct Cyc_List_List*ps=_tmp235;int dots=_tmp236;
# 858
{struct Cyc_List_List*ps2=ps;for(0;ps2 != 0;ps2=ps2->tl){
({Cyc_Binding_resolve_pat(env,(struct Cyc_Absyn_Pat*)ps2->hd);});}}
{struct _handler_cons _tmp24B;_push_handler(& _tmp24B);{int _tmp24D=0;if(setjmp(_tmp24B.handler))_tmp24D=1;if(!_tmp24D){{void*_stmttmp1A=({Cyc_Binding_lookup_ordinary(loc,env,qv);});void*_tmp24E=_stmttmp1A;struct Cyc_Absyn_Datatypefield*_tmp250;struct Cyc_Absyn_Datatypedecl*_tmp24F;struct Cyc_Absyn_Aggrdecl*_tmp251;switch(*((int*)_tmp24E)){case 0U: _LL3D: _LL3E:
 goto _LL3C;case 1U: _LL3F: _tmp251=((struct Cyc_Binding_AggrRes_Binding_Resolved_struct*)_tmp24E)->f1;_LL40: {struct Cyc_Absyn_Aggrdecl*ad=_tmp251;
# 863
struct Cyc_List_List*new_ps=0;
for(0;ps != 0;ps=ps->tl){
new_ps=({struct Cyc_List_List*_tmp253=_cycalloc(sizeof(*_tmp253));({struct _tuple15*_tmp403=({struct _tuple15*_tmp252=_cycalloc(sizeof(*_tmp252));_tmp252->f1=0,_tmp252->f2=(struct Cyc_Absyn_Pat*)ps->hd;_tmp252;});_tmp253->hd=_tmp403;}),_tmp253->tl=new_ps;_tmp253;});}
({void*_tmp407=(void*)({struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*_tmp256=_cycalloc(sizeof(*_tmp256));_tmp256->tag=7U,({union Cyc_Absyn_AggrInfo*_tmp406=({union Cyc_Absyn_AggrInfo*_tmp255=_cycalloc(sizeof(*_tmp255));({union Cyc_Absyn_AggrInfo _tmp405=({Cyc_Absyn_KnownAggr(({struct Cyc_Absyn_Aggrdecl**_tmp254=_cycalloc(sizeof(*_tmp254));*_tmp254=ad;_tmp254;}));});*_tmp255=_tmp405;});_tmp255;});_tmp256->f1=_tmp406;}),_tmp256->f2=0,({struct Cyc_List_List*_tmp404=({Cyc_List_imp_rev(new_ps);});_tmp256->f3=_tmp404;}),_tmp256->f4=dots;_tmp256;});p->r=_tmp407;});
_npop_handler(0U);return;}case 2U: _LL41: _tmp24F=((struct Cyc_Binding_DatatypeRes_Binding_Resolved_struct*)_tmp24E)->f1;_tmp250=((struct Cyc_Binding_DatatypeRes_Binding_Resolved_struct*)_tmp24E)->f2;_LL42: {struct Cyc_Absyn_Datatypedecl*tud=_tmp24F;struct Cyc_Absyn_Datatypefield*tuf=_tmp250;
# 869
({void*_tmp408=(void*)({struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct*_tmp257=_cycalloc(sizeof(*_tmp257));_tmp257->tag=8U,_tmp257->f1=tud,_tmp257->f2=tuf,_tmp257->f3=ps,_tmp257->f4=dots;_tmp257;});p->r=_tmp408;});_npop_handler(0U);return;}case 3U: _LL43: _LL44:
 goto _LL46;default: _LL45: _LL46:
# 872
({struct Cyc_Warn_String_Warn_Warg_struct _tmp259=({struct Cyc_Warn_String_Warn_Warg_struct _tmp33D;_tmp33D.tag=0U,({struct _fat_ptr _tmp409=({const char*_tmp25A="enum tag used with arguments in pattern";_tag_fat(_tmp25A,sizeof(char),40U);});_tmp33D.f1=_tmp409;});_tmp33D;});void*_tmp258[1U];_tmp258[0]=& _tmp259;({unsigned _tmp40A=loc;Cyc_Warn_err2(_tmp40A,_tag_fat(_tmp258,sizeof(void*),1U));});});_npop_handler(0U);return;}_LL3C:;}
# 860
;_pop_handler();}else{void*_tmp24C=(void*)Cyc_Core_get_exn_thrown();void*_tmp25B=_tmp24C;void*_tmp25C;if(((struct Cyc_Binding_BindingError_exn_struct*)_tmp25B)->tag == Cyc_Binding_BindingError){_LL48: _LL49:
# 873
 goto _LL47;}else{_LL4A: _tmp25C=_tmp25B;_LL4B: {void*exn=_tmp25C;(int)_rethrow(exn);}}_LL47:;}}}
({struct Cyc_Warn_Qvar_Warn_Warg_struct _tmp25E=({struct Cyc_Warn_Qvar_Warn_Warg_struct _tmp33F;_tmp33F.tag=3U,_tmp33F.f1=qv;_tmp33F;});struct Cyc_Warn_String_Warn_Warg_struct _tmp25F=({struct Cyc_Warn_String_Warn_Warg_struct _tmp33E;_tmp33E.tag=0U,({struct _fat_ptr _tmp40B=({const char*_tmp260=" is not a constructor in pattern";_tag_fat(_tmp260,sizeof(char),33U);});_tmp33E.f1=_tmp40B;});_tmp33E;});void*_tmp25D[2U];_tmp25D[0]=& _tmp25E,_tmp25D[1]=& _tmp25F;({unsigned _tmp40C=loc;Cyc_Warn_err2(_tmp40C,_tag_fat(_tmp25D,sizeof(void*),2U));});});return;}case 7U: if(((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp223)->f1 != 0){if((((union Cyc_Absyn_AggrInfo*)((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp223)->f1)->UnknownAggr).tag == 1){_LL8: _tmp230=(((((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp223)->f1)->UnknownAggr).val).f2;_tmp231=((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp223)->f2;_tmp232=((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp223)->f3;_tmp233=((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp223)->f4;_LL9: {struct _tuple0*n=_tmp230;struct Cyc_List_List*exist_ts=_tmp231;struct Cyc_List_List*dps=_tmp232;int dots=_tmp233;
# 877
{struct Cyc_List_List*dps2=dps;for(0;dps2 != 0;dps2=dps2->tl){
({Cyc_Binding_resolve_pat(env,(*((struct _tuple15*)dps2->hd)).f2);});}}
{struct _handler_cons _tmp261;_push_handler(& _tmp261);{int _tmp263=0;if(setjmp(_tmp261.handler))_tmp263=1;if(!_tmp263){
{struct Cyc_Absyn_Aggrdecl*ad=({({struct Cyc_Absyn_Aggrdecl*(*_tmp40F)(unsigned loc,struct Cyc_Binding_NSCtxt*ctxt,struct _tuple0*qv,struct Cyc_Absyn_Aggrdecl*(*lookup)(struct Cyc_Binding_ResolveNSEnv*,struct _fat_ptr*))=({struct Cyc_Absyn_Aggrdecl*(*_tmp267)(unsigned loc,struct Cyc_Binding_NSCtxt*ctxt,struct _tuple0*qv,struct Cyc_Absyn_Aggrdecl*(*lookup)(struct Cyc_Binding_ResolveNSEnv*,struct _fat_ptr*))=(struct Cyc_Absyn_Aggrdecl*(*)(unsigned loc,struct Cyc_Binding_NSCtxt*ctxt,struct _tuple0*qv,struct Cyc_Absyn_Aggrdecl*(*lookup)(struct Cyc_Binding_ResolveNSEnv*,struct _fat_ptr*)))Cyc_Binding_resolve_lookup;_tmp267;});unsigned _tmp40E=loc;struct Cyc_Binding_NSCtxt*_tmp40D=env->ns;_tmp40F(_tmp40E,_tmp40D,n,Cyc_Binding_lookup_aggrdecl);});});
({void*_tmp412=(void*)({struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*_tmp266=_cycalloc(sizeof(*_tmp266));_tmp266->tag=7U,({union Cyc_Absyn_AggrInfo*_tmp411=({union Cyc_Absyn_AggrInfo*_tmp265=_cycalloc(sizeof(*_tmp265));({union Cyc_Absyn_AggrInfo _tmp410=({Cyc_Absyn_KnownAggr(({struct Cyc_Absyn_Aggrdecl**_tmp264=_cycalloc(sizeof(*_tmp264));*_tmp264=ad;_tmp264;}));});*_tmp265=_tmp410;});_tmp265;});_tmp266->f1=_tmp411;}),_tmp266->f2=exist_ts,_tmp266->f3=dps,_tmp266->f4=dots;_tmp266;});p->r=_tmp412;});}
# 880
;_pop_handler();}else{void*_tmp262=(void*)Cyc_Core_get_exn_thrown();void*_tmp268=_tmp262;void*_tmp269;if(((struct Cyc_Binding_BindingError_exn_struct*)_tmp268)->tag == Cyc_Binding_BindingError){_LL4D: _LL4E:
# 883
({struct Cyc_Warn_String_Warn_Warg_struct _tmp26B=({struct Cyc_Warn_String_Warn_Warg_struct _tmp340;_tmp340.tag=0U,({struct _fat_ptr _tmp413=({const char*_tmp26C="non-aggregate name has designator patterns";_tag_fat(_tmp26C,sizeof(char),43U);});_tmp340.f1=_tmp413;});_tmp340;});void*_tmp26A[1U];_tmp26A[0]=& _tmp26B;({unsigned _tmp414=loc;Cyc_Warn_err2(_tmp414,_tag_fat(_tmp26A,sizeof(void*),1U));});});return;}else{_LL4F: _tmp269=_tmp268;_LL50: {void*exn=_tmp269;(int)_rethrow(exn);}}_LL4C:;}}}
# 885
return;}}else{_LL24: _tmp22F=((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp223)->f3;_LL25: {struct Cyc_List_List*dps=_tmp22F;
# 914
for(0;dps != 0;dps=dps->tl){
({Cyc_Binding_resolve_pat(env,(*((struct _tuple15*)dps->hd)).f2);});}
return;}}}else{_LL22: _tmp22E=((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp223)->f3;_LL23: {struct Cyc_List_List*dps=_tmp22E;
# 912
_tmp22F=dps;goto _LL25;}}case 0U: _LLA: _LLB:
# 887
 return;case 3U: _LLC: _tmp22C=((struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct*)_tmp223)->f1;_tmp22D=((struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct*)_tmp223)->f2;_LLD: {struct Cyc_Absyn_Vardecl*vd=_tmp22C;struct Cyc_Absyn_Pat*p2=_tmp22D;
# 889
_tmp22A=vd;_tmp22B=p2;goto _LLF;}case 1U: _LLE: _tmp22A=((struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct*)_tmp223)->f1;_tmp22B=((struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct*)_tmp223)->f2;_LLF: {struct Cyc_Absyn_Vardecl*vd=_tmp22A;struct Cyc_Absyn_Pat*p2=_tmp22B;
# 891
({Cyc_Binding_resolve_pat(env,p2);});
_tmp229=vd;goto _LL11;}case 4U: _LL10: _tmp229=((struct Cyc_Absyn_TagInt_p_Absyn_Raw_pat_struct*)_tmp223)->f2;_LL11: {struct Cyc_Absyn_Vardecl*vd=_tmp229;
_tmp228=vd;goto _LL13;}case 2U: _LL12: _tmp228=((struct Cyc_Absyn_AliasVar_p_Absyn_Raw_pat_struct*)_tmp223)->f2;_LL13: {struct Cyc_Absyn_Vardecl*vd=_tmp228;
# 895
({Cyc_Binding_resolve_and_add_vardecl(loc,env,vd);});goto _LL3;}case 6U: _LL14: _tmp227=((struct Cyc_Absyn_Pointer_p_Absyn_Raw_pat_struct*)_tmp223)->f1;_LL15: {struct Cyc_Absyn_Pat*p2=_tmp227;
# 897
({Cyc_Binding_resolve_pat(env,p2);});return;}case 5U: _LL16: _tmp226=((struct Cyc_Absyn_Tuple_p_Absyn_Raw_pat_struct*)_tmp223)->f1;_LL17: {struct Cyc_List_List*ps=_tmp226;
# 899
for(0;ps != 0;ps=ps->tl){
({Cyc_Binding_resolve_pat(env,(struct Cyc_Absyn_Pat*)ps->hd);});}
goto _LL3;}case 17U: _LL18: _tmp225=((struct Cyc_Absyn_Exp_p_Absyn_Raw_pat_struct*)_tmp223)->f1;_LL19: {struct Cyc_Absyn_Exp*e=_tmp225;
# 903
({Cyc_Binding_resolve_exp(env,e);});return;}case 9U: _LL1A: _LL1B:
# 905
 goto _LL1D;case 10U: _LL1C: _LL1D:
 goto _LL1F;case 11U: _LL1E: _LL1F:
 goto _LL21;case 12U: _LL20: _LL21:
 return;case 8U: _LL26: _tmp224=((struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct*)_tmp223)->f3;_LL27: {struct Cyc_List_List*ps=_tmp224;
# 918
for(0;ps != 0;ps=ps->tl){
({Cyc_Binding_resolve_pat(env,(struct Cyc_Absyn_Pat*)ps->hd);});}
return;}case 13U: _LL28: _LL29:
 goto _LL2B;default: _LL2A: _LL2B:
 return;}_LL3:;}}struct _tuple16{void*f1;void*f2;};
# 926
void Cyc_Binding_resolve_rgnpo(unsigned loc,struct Cyc_Binding_Env*env,struct Cyc_List_List*po){
for(0;po != 0;po=po->tl){
({Cyc_Binding_resolve_type(loc,env,(*((struct _tuple16*)po->hd)).f1);});
({Cyc_Binding_resolve_type(loc,env,(*((struct _tuple16*)po->hd)).f2);});}}struct _tuple17{struct Cyc_List_List**f1;struct Cyc_Dict_Dict*f2;struct Cyc_Binding_Env*f3;struct _tuple11*f4;};struct _tuple18{unsigned f1;struct _tuple0*f2;int f3;};
# 935
void Cyc_Binding_export_all_symbols(struct _tuple17*cenv,struct _fat_ptr*name,void*res){
# 937
struct Cyc_List_List*_tmp273;unsigned _tmp272;struct Cyc_Binding_Env*_tmp271;struct Cyc_Dict_Dict*_tmp270;struct Cyc_List_List**_tmp26F;_LL1: _tmp26F=cenv->f1;_tmp270=cenv->f2;_tmp271=cenv->f3;_tmp272=(cenv->f4)->f1;_tmp273=(cenv->f4)->f2;_LL2: {struct Cyc_List_List**exlist_ptr=_tmp26F;struct Cyc_Dict_Dict*out_dict=_tmp270;struct Cyc_Binding_Env*env=_tmp271;unsigned wcloc=_tmp272;struct Cyc_List_List*hidelist=_tmp273;
void*_tmp274=res;struct Cyc_Absyn_Datatypefield*_tmp276;struct Cyc_Absyn_Datatypedecl*_tmp275;struct Cyc_Absyn_Enumfield*_tmp278;void*_tmp277;struct Cyc_Absyn_Aggrdecl*_tmp279;struct Cyc_Absyn_Enumfield*_tmp27B;struct Cyc_Absyn_Enumdecl*_tmp27A;void*_tmp27C;switch(*((int*)_tmp274)){case 0U: _LL4: _tmp27C=(void*)((struct Cyc_Binding_VarRes_Binding_Resolved_struct*)_tmp274)->f1;_LL5: {void*bnd=_tmp27C;
# 940
struct _tuple0*qv=({struct _tuple0*_tmp281=_cycalloc(sizeof(*_tmp281));({union Cyc_Absyn_Nmspace _tmp415=({Cyc_Absyn_Rel_n(0);});_tmp281->f1=_tmp415;}),_tmp281->f2=name;_tmp281;});
if(({({int(*_tmp417)(int(*compare)(struct _tuple0*,struct _tuple0*),struct Cyc_List_List*l,struct _tuple0*x)=({int(*_tmp27D)(int(*compare)(struct _tuple0*,struct _tuple0*),struct Cyc_List_List*l,struct _tuple0*x)=(int(*)(int(*compare)(struct _tuple0*,struct _tuple0*),struct Cyc_List_List*l,struct _tuple0*x))Cyc_List_mem;_tmp27D;});struct Cyc_List_List*_tmp416=hidelist;_tmp417(Cyc_Absyn_qvar_cmp,_tmp416,qv);});}))
# 943
return;
# 941
({Cyc_Binding_absolutize_decl(wcloc,env,qv,Cyc_Absyn_ExternC);});{
# 946
struct _tuple18*ex_sym=({struct _tuple18*_tmp280=_cycalloc(sizeof(*_tmp280));_tmp280->f1=wcloc,_tmp280->f2=qv,_tmp280->f3=0;_tmp280;});
({struct Cyc_List_List*_tmp418=({struct Cyc_List_List*_tmp27E=_cycalloc(sizeof(*_tmp27E));_tmp27E->hd=ex_sym,_tmp27E->tl=*exlist_ptr;_tmp27E;});*exlist_ptr=_tmp418;});
({struct Cyc_Dict_Dict _tmp41C=({({struct Cyc_Dict_Dict(*_tmp41B)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,void*v)=({struct Cyc_Dict_Dict(*_tmp27F)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,void*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,void*v))Cyc_Dict_insert;_tmp27F;});struct Cyc_Dict_Dict _tmp41A=*out_dict;struct _fat_ptr*_tmp419=name;_tmp41B(_tmp41A,_tmp419,res);});});*out_dict=_tmp41C;});
goto _LL3;}}case 3U: _LL6: _tmp27A=((struct Cyc_Binding_EnumRes_Binding_Resolved_struct*)_tmp274)->f1;_tmp27B=((struct Cyc_Binding_EnumRes_Binding_Resolved_struct*)_tmp274)->f2;_LL7: {struct Cyc_Absyn_Enumdecl*ed=_tmp27A;struct Cyc_Absyn_Enumfield*ef=_tmp27B;
# 951
({struct Cyc_Dict_Dict _tmp420=({({struct Cyc_Dict_Dict(*_tmp41F)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,void*v)=({struct Cyc_Dict_Dict(*_tmp282)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,void*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,void*v))Cyc_Dict_insert;_tmp282;});struct Cyc_Dict_Dict _tmp41E=*out_dict;struct _fat_ptr*_tmp41D=name;_tmp41F(_tmp41E,_tmp41D,res);});});*out_dict=_tmp420;});goto _LL3;}case 1U: _LL8: _tmp279=((struct Cyc_Binding_AggrRes_Binding_Resolved_struct*)_tmp274)->f1;_LL9: {struct Cyc_Absyn_Aggrdecl*ad=_tmp279;
# 953
goto _LL3;}case 4U: _LLA: _tmp277=(void*)((struct Cyc_Binding_AnonEnumRes_Binding_Resolved_struct*)_tmp274)->f1;_tmp278=((struct Cyc_Binding_AnonEnumRes_Binding_Resolved_struct*)_tmp274)->f2;_LLB: {void*t=_tmp277;struct Cyc_Absyn_Enumfield*ef=_tmp278;
goto _LL3;}default: _LLC: _tmp275=((struct Cyc_Binding_DatatypeRes_Binding_Resolved_struct*)_tmp274)->f1;_tmp276=((struct Cyc_Binding_DatatypeRes_Binding_Resolved_struct*)_tmp274)->f2;_LLD: {struct Cyc_Absyn_Datatypedecl*dd=_tmp275;struct Cyc_Absyn_Datatypefield*df=_tmp276;
# 956
({void*_tmp283=0U;({unsigned _tmp422=wcloc;struct _fat_ptr _tmp421=({const char*_tmp284="Unexpected binding from extern C Include";_tag_fat(_tmp284,sizeof(char),41U);});Cyc_Warn_err(_tmp422,_tmp421,_tag_fat(_tmp283,sizeof(void*),0U));});});
goto _LL3;}}_LL3:;}}
# 969 "binding.cyc"
void Cyc_Binding_resolve_decl(struct Cyc_Binding_Env*env,struct Cyc_Absyn_Decl*d){
# 972
unsigned loc=d->loc;
void*_stmttmp1B=d->r;void*_tmp286=_stmttmp1B;struct _tuple11*_tmp28A;struct Cyc_List_List**_tmp289;struct Cyc_List_List*_tmp288;struct Cyc_List_List*_tmp287;struct Cyc_List_List*_tmp28B;struct Cyc_List_List*_tmp28D;struct _tuple0*_tmp28C;struct Cyc_List_List*_tmp28F;struct _fat_ptr*_tmp28E;struct Cyc_Absyn_Datatypedecl*_tmp290;struct Cyc_Absyn_Enumdecl*_tmp291;struct Cyc_Absyn_Aggrdecl*_tmp292;struct Cyc_Absyn_Typedefdecl*_tmp293;struct Cyc_Absyn_Exp*_tmp296;struct Cyc_Absyn_Vardecl*_tmp295;struct Cyc_Absyn_Tvar*_tmp294;struct Cyc_List_List*_tmp297;struct Cyc_Absyn_Exp*_tmp299;struct Cyc_Absyn_Pat*_tmp298;struct Cyc_Absyn_Fndecl*_tmp29A;struct Cyc_Absyn_Vardecl*_tmp29B;switch(*((int*)_tmp286)){case 0U: _LL1: _tmp29B=((struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_tmp286)->f1;_LL2: {struct Cyc_Absyn_Vardecl*vd=_tmp29B;
# 976
({Cyc_Binding_resolve_and_add_vardecl(loc,env,vd);});
# 981
if(vd->initializer != 0 && !({Cyc_Binding_in_cinclude(env);}))
({Cyc_Binding_resolve_exp(env,(struct Cyc_Absyn_Exp*)_check_null(vd->initializer));});
# 981
goto _LL0;}case 1U: _LL3: _tmp29A=((struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*)_tmp286)->f1;_LL4: {struct Cyc_Absyn_Fndecl*fd=_tmp29A;
# 986
({Cyc_Binding_absolutize_decl(loc,env,fd->name,fd->sc);});{
struct _tuple0*_stmttmp1C=fd->name;struct _fat_ptr*_tmp29D;union Cyc_Absyn_Nmspace _tmp29C;_LL24: _tmp29C=_stmttmp1C->f1;_tmp29D=_stmttmp1C->f2;_LL25: {union Cyc_Absyn_Nmspace decl_ns=_tmp29C;struct _fat_ptr*decl_name=_tmp29D;
# 989
struct Cyc_List_List*vds=({Cyc_Binding_get_fun_vardecls(1,loc,env,(fd->i).args,(fd->i).cyc_varargs);});
({struct Cyc_Core_Opt*_tmp423=({struct Cyc_Core_Opt*_tmp29E=_cycalloc(sizeof(*_tmp29E));_tmp29E->v=vds;_tmp29E;});fd->param_vardecls=_tmp423;});
# 992
if(Cyc_Binding_warn_override)
({Cyc_Binding_check_warn_override(loc,env,fd->name);});{
# 992
struct Cyc_Dict_Dict*old_locals=env->local_vars;
# 996
if(old_locals != 0)
({struct Cyc_Dict_Dict*_tmp424=({struct Cyc_Dict_Dict*_tmp29F=_cycalloc(sizeof(*_tmp29F));*_tmp29F=*((struct Cyc_Dict_Dict*)_check_null(env->local_vars));_tmp29F;});env->local_vars=_tmp424;});else{
# 999
({struct Cyc_Dict_Dict*_tmp426=({struct Cyc_Dict_Dict*_tmp2A1=_cycalloc(sizeof(*_tmp2A1));({struct Cyc_Dict_Dict _tmp425=({({struct Cyc_Dict_Dict(*_tmp2A0)(int(*cmp)(struct _fat_ptr*,struct _fat_ptr*))=(struct Cyc_Dict_Dict(*)(int(*cmp)(struct _fat_ptr*,struct _fat_ptr*)))Cyc_Dict_empty;_tmp2A0;})(Cyc_strptrcmp);});*_tmp2A1=_tmp425;});_tmp2A1;});env->local_vars=_tmp426;});}
{struct Cyc_List_List*vds1=vds;for(0;vds1 != 0;vds1=vds1->tl){
({struct Cyc_Dict_Dict _tmp42B=({({struct Cyc_Dict_Dict(*_tmp42A)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,void*v)=({struct Cyc_Dict_Dict(*_tmp2A2)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,void*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,void*v))Cyc_Dict_insert;_tmp2A2;});struct Cyc_Dict_Dict _tmp429=*((struct Cyc_Dict_Dict*)_check_null(env->local_vars));struct _fat_ptr*_tmp428=(*((struct Cyc_Absyn_Vardecl*)vds1->hd)->name).f2;_tmp42A(_tmp429,_tmp428,(void*)({struct Cyc_Binding_VarRes_Binding_Resolved_struct*_tmp2A4=_cycalloc(sizeof(*_tmp2A4));_tmp2A4->tag=0U,({void*_tmp427=(void*)({struct Cyc_Absyn_Param_b_Absyn_Binding_struct*_tmp2A3=_cycalloc(sizeof(*_tmp2A3));_tmp2A3->tag=3U,_tmp2A3->f1=(struct Cyc_Absyn_Vardecl*)vds1->hd;_tmp2A3;});_tmp2A4->f1=_tmp427;});_tmp2A4;}));});});*((struct Cyc_Dict_Dict*)_check_null(env->local_vars))=_tmp42B;});}}
# 1004
({Cyc_Binding_resolve_function_stuff(loc,env,fd->i);});
# 1006
if(old_locals != 0){
# 1008
({struct Cyc_Dict_Dict _tmp430=({({struct Cyc_Dict_Dict(*_tmp42F)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,void*v)=({struct Cyc_Dict_Dict(*_tmp2A5)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,void*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,void*v))Cyc_Dict_insert;_tmp2A5;});struct Cyc_Dict_Dict _tmp42E=*old_locals;struct _fat_ptr*_tmp42D=decl_name;_tmp42F(_tmp42E,_tmp42D,(void*)({struct Cyc_Binding_VarRes_Binding_Resolved_struct*_tmp2A7=_cycalloc(sizeof(*_tmp2A7));_tmp2A7->tag=0U,({void*_tmp42C=(void*)({struct Cyc_Absyn_Funname_b_Absyn_Binding_struct*_tmp2A6=_cycalloc(sizeof(*_tmp2A6));_tmp2A6->tag=2U,_tmp2A6->f1=fd;_tmp2A6;});_tmp2A7->f1=_tmp42C;});_tmp2A7;}));});});*old_locals=_tmp430;});
# 1010
({struct Cyc_Dict_Dict _tmp435=({({struct Cyc_Dict_Dict(*_tmp434)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,void*v)=({struct Cyc_Dict_Dict(*_tmp2A8)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,void*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,void*v))Cyc_Dict_insert;_tmp2A8;});struct Cyc_Dict_Dict _tmp433=*((struct Cyc_Dict_Dict*)_check_null(env->local_vars));struct _fat_ptr*_tmp432=decl_name;_tmp434(_tmp433,_tmp432,(void*)({struct Cyc_Binding_VarRes_Binding_Resolved_struct*_tmp2AA=_cycalloc(sizeof(*_tmp2AA));_tmp2AA->tag=0U,({void*_tmp431=(void*)({struct Cyc_Absyn_Funname_b_Absyn_Binding_struct*_tmp2A9=_cycalloc(sizeof(*_tmp2A9));_tmp2A9->tag=2U,_tmp2A9->f1=fd;_tmp2A9;});_tmp2AA->f1=_tmp431;});_tmp2AA;}));});});*((struct Cyc_Dict_Dict*)_check_null(env->local_vars))=_tmp435;});}else{
# 1013
struct Cyc_Binding_ResolveNSEnv*decl_ns_data=({({struct Cyc_Binding_ResolveNSEnv*(*_tmp437)(struct Cyc_Binding_NSCtxt*ctxt,union Cyc_Absyn_Nmspace abs_ns)=({struct Cyc_Binding_ResolveNSEnv*(*_tmp2AE)(struct Cyc_Binding_NSCtxt*ctxt,union Cyc_Absyn_Nmspace abs_ns)=(struct Cyc_Binding_ResolveNSEnv*(*)(struct Cyc_Binding_NSCtxt*ctxt,union Cyc_Absyn_Nmspace abs_ns))Cyc_Binding_get_ns_data;_tmp2AE;});struct Cyc_Binding_NSCtxt*_tmp436=env->ns;_tmp437(_tmp436,decl_ns);});});
({struct Cyc_Dict_Dict _tmp43C=({({struct Cyc_Dict_Dict(*_tmp43B)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,void*v)=({struct Cyc_Dict_Dict(*_tmp2AB)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,void*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,void*v))Cyc_Dict_insert;_tmp2AB;});struct Cyc_Dict_Dict _tmp43A=decl_ns_data->ordinaries;struct _fat_ptr*_tmp439=decl_name;_tmp43B(_tmp43A,_tmp439,(void*)({struct Cyc_Binding_VarRes_Binding_Resolved_struct*_tmp2AD=_cycalloc(sizeof(*_tmp2AD));_tmp2AD->tag=0U,({void*_tmp438=(void*)({struct Cyc_Absyn_Funname_b_Absyn_Binding_struct*_tmp2AC=_cycalloc(sizeof(*_tmp2AC));_tmp2AC->tag=2U,_tmp2AC->f1=fd;_tmp2AC;});_tmp2AD->f1=_tmp438;});_tmp2AD;}));});});decl_ns_data->ordinaries=_tmp43C;});}
# 1019
if(!({Cyc_Binding_in_cinclude(env);}))
({Cyc_Binding_resolve_stmt(env,fd->body);});
# 1019
env->local_vars=old_locals;
# 1023
goto _LL0;}}}}case 2U: _LL5: _tmp298=((struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct*)_tmp286)->f1;_tmp299=((struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct*)_tmp286)->f3;_LL6: {struct Cyc_Absyn_Pat*p=_tmp298;struct Cyc_Absyn_Exp*exp=_tmp299;
# 1026
if(({Cyc_Binding_at_toplevel(env);})){
({struct Cyc_Warn_String_Warn_Warg_struct _tmp2B0=({struct Cyc_Warn_String_Warn_Warg_struct _tmp341;_tmp341.tag=0U,({struct _fat_ptr _tmp43D=({const char*_tmp2B1="let not allowed at top-level";_tag_fat(_tmp2B1,sizeof(char),29U);});_tmp341.f1=_tmp43D;});_tmp341;});void*_tmp2AF[1U];_tmp2AF[0]=& _tmp2B0;({unsigned _tmp43E=loc;Cyc_Warn_err2(_tmp43E,_tag_fat(_tmp2AF,sizeof(void*),1U));});});
goto _LL0;}
# 1026
({Cyc_Binding_resolve_exp(env,exp);});
# 1031
({Cyc_Binding_resolve_pat(env,p);});
goto _LL0;}case 3U: _LL7: _tmp297=((struct Cyc_Absyn_Letv_d_Absyn_Raw_decl_struct*)_tmp286)->f1;_LL8: {struct Cyc_List_List*vds=_tmp297;
# 1035
for(0;vds != 0;vds=vds->tl){
({Cyc_Binding_resolve_and_add_vardecl(loc,env,(struct Cyc_Absyn_Vardecl*)vds->hd);});}
goto _LL0;}case 4U: _LL9: _tmp294=((struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*)_tmp286)->f1;_tmp295=((struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*)_tmp286)->f2;_tmp296=((struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*)_tmp286)->f3;_LLA: {struct Cyc_Absyn_Tvar*tv=_tmp294;struct Cyc_Absyn_Vardecl*vd=_tmp295;struct Cyc_Absyn_Exp*open_exp_opt=_tmp296;
# 1040
if(open_exp_opt != 0)
({Cyc_Binding_resolve_exp(env,open_exp_opt);});
# 1040
({Cyc_Binding_resolve_and_add_vardecl(loc,env,vd);});
# 1043
goto _LL0;}case 8U: _LLB: _tmp293=((struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct*)_tmp286)->f1;_LLC: {struct Cyc_Absyn_Typedefdecl*td=_tmp293;
# 1046
if(!({Cyc_Binding_at_toplevel(env);})){
({struct Cyc_Warn_String_Warn_Warg_struct _tmp2B3=({struct Cyc_Warn_String_Warn_Warg_struct _tmp342;_tmp342.tag=0U,({struct _fat_ptr _tmp43F=({const char*_tmp2B4="nested type definitions are not yet supported";_tag_fat(_tmp2B4,sizeof(char),46U);});_tmp342.f1=_tmp43F;});_tmp342;});void*_tmp2B2[1U];_tmp2B2[0]=& _tmp2B3;({unsigned _tmp440=loc;Cyc_Warn_err2(_tmp440,_tag_fat(_tmp2B2,sizeof(void*),1U));});});
goto _LL0;}
# 1046
({Cyc_Binding_absolutize_decl(loc,env,td->name,td->extern_c?Cyc_Absyn_ExternC: Cyc_Absyn_Public);});{
# 1051
struct _tuple0*_stmttmp1D=td->name;struct _fat_ptr*_tmp2B6;union Cyc_Absyn_Nmspace _tmp2B5;_LL27: _tmp2B5=_stmttmp1D->f1;_tmp2B6=_stmttmp1D->f2;_LL28: {union Cyc_Absyn_Nmspace decl_ns=_tmp2B5;struct _fat_ptr*decl_name=_tmp2B6;
# 1053
struct Cyc_Binding_ResolveNSEnv*decl_ns_data=({({struct Cyc_Binding_ResolveNSEnv*(*_tmp442)(struct Cyc_Binding_NSCtxt*ctxt,union Cyc_Absyn_Nmspace abs_ns)=({struct Cyc_Binding_ResolveNSEnv*(*_tmp2B8)(struct Cyc_Binding_NSCtxt*ctxt,union Cyc_Absyn_Nmspace abs_ns)=(struct Cyc_Binding_ResolveNSEnv*(*)(struct Cyc_Binding_NSCtxt*ctxt,union Cyc_Absyn_Nmspace abs_ns))Cyc_Binding_get_ns_data;_tmp2B8;});struct Cyc_Binding_NSCtxt*_tmp441=env->ns;_tmp442(_tmp441,decl_ns);});});
if(td->defn != 0)
({Cyc_Binding_resolve_type(loc,env,(void*)_check_null(td->defn));});
# 1054
({struct Cyc_Dict_Dict _tmp446=({({struct Cyc_Dict_Dict(*_tmp445)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,struct Cyc_Absyn_Typedefdecl*v)=({
# 1057
struct Cyc_Dict_Dict(*_tmp2B7)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,struct Cyc_Absyn_Typedefdecl*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,struct Cyc_Absyn_Typedefdecl*v))Cyc_Dict_insert;_tmp2B7;});struct Cyc_Dict_Dict _tmp444=decl_ns_data->typedefs;struct _fat_ptr*_tmp443=decl_name;_tmp445(_tmp444,_tmp443,td);});});
# 1054
decl_ns_data->typedefs=_tmp446;});
# 1059
goto _LL0;}}}case 5U: _LLD: _tmp292=((struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct*)_tmp286)->f1;_LLE: {struct Cyc_Absyn_Aggrdecl*ad=_tmp292;
# 1062
if(!({Cyc_Binding_at_toplevel(env);})){
({struct Cyc_Warn_String_Warn_Warg_struct _tmp2BA=({struct Cyc_Warn_String_Warn_Warg_struct _tmp343;_tmp343.tag=0U,({struct _fat_ptr _tmp447=({const char*_tmp2BB="nested type definitions are not yet supported";_tag_fat(_tmp2BB,sizeof(char),46U);});_tmp343.f1=_tmp447;});_tmp343;});void*_tmp2B9[1U];_tmp2B9[0]=& _tmp2BA;({unsigned _tmp448=loc;Cyc_Warn_err2(_tmp448,_tag_fat(_tmp2B9,sizeof(void*),1U));});});
goto _LL0;}
# 1062
({Cyc_Binding_absolutize_decl(loc,env,ad->name,ad->sc);});{
# 1067
struct _tuple0*_stmttmp1E=ad->name;struct _fat_ptr*_tmp2BD;union Cyc_Absyn_Nmspace _tmp2BC;_LL2A: _tmp2BC=_stmttmp1E->f1;_tmp2BD=_stmttmp1E->f2;_LL2B: {union Cyc_Absyn_Nmspace decl_ns=_tmp2BC;struct _fat_ptr*decl_name=_tmp2BD;
# 1069
struct Cyc_Binding_ResolveNSEnv*decl_ns_data=({({struct Cyc_Binding_ResolveNSEnv*(*_tmp44A)(struct Cyc_Binding_NSCtxt*ctxt,union Cyc_Absyn_Nmspace abs_ns)=({struct Cyc_Binding_ResolveNSEnv*(*_tmp2C4)(struct Cyc_Binding_NSCtxt*ctxt,union Cyc_Absyn_Nmspace abs_ns)=(struct Cyc_Binding_ResolveNSEnv*(*)(struct Cyc_Binding_NSCtxt*ctxt,union Cyc_Absyn_Nmspace abs_ns))Cyc_Binding_get_ns_data;_tmp2C4;});struct Cyc_Binding_NSCtxt*_tmp449=env->ns;_tmp44A(_tmp449,decl_ns);});});
# 1071
if(({({int(*_tmp44C)(struct Cyc_Dict_Dict d,struct _fat_ptr*k)=({int(*_tmp2BE)(struct Cyc_Dict_Dict d,struct _fat_ptr*k)=(int(*)(struct Cyc_Dict_Dict d,struct _fat_ptr*k))Cyc_Dict_member;_tmp2BE;});struct Cyc_Dict_Dict _tmp44B=decl_ns_data->aggrdecls;_tmp44C(_tmp44B,decl_name);});})&& ad->impl == 0)
goto _LL0;
# 1071
({struct Cyc_Dict_Dict _tmp450=({({struct Cyc_Dict_Dict(*_tmp44F)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,struct Cyc_Absyn_Aggrdecl*v)=({
# 1074
struct Cyc_Dict_Dict(*_tmp2BF)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,struct Cyc_Absyn_Aggrdecl*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,struct Cyc_Absyn_Aggrdecl*v))Cyc_Dict_insert;_tmp2BF;});struct Cyc_Dict_Dict _tmp44E=decl_ns_data->aggrdecls;struct _fat_ptr*_tmp44D=decl_name;_tmp44F(_tmp44E,_tmp44D,ad);});});
# 1071
decl_ns_data->aggrdecls=_tmp450;});
# 1076
({struct Cyc_Dict_Dict _tmp454=({({struct Cyc_Dict_Dict(*_tmp453)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,void*v)=({struct Cyc_Dict_Dict(*_tmp2C0)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,void*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,void*v))Cyc_Dict_insert;_tmp2C0;});struct Cyc_Dict_Dict _tmp452=decl_ns_data->ordinaries;struct _fat_ptr*_tmp451=decl_name;_tmp453(_tmp452,_tmp451,(void*)({struct Cyc_Binding_AggrRes_Binding_Resolved_struct*_tmp2C1=_cycalloc(sizeof(*_tmp2C1));_tmp2C1->tag=1U,_tmp2C1->f1=ad;_tmp2C1;}));});});decl_ns_data->ordinaries=_tmp454;});
# 1078
if(ad->impl != 0){
struct Cyc_Absyn_AggrdeclImpl*_stmttmp1F=(struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl);struct Cyc_List_List*_tmp2C3;struct Cyc_List_List*_tmp2C2;_LL2D: _tmp2C2=_stmttmp1F->rgn_po;_tmp2C3=_stmttmp1F->fields;_LL2E: {struct Cyc_List_List*rpo=_tmp2C2;struct Cyc_List_List*fs=_tmp2C3;
({Cyc_Binding_resolve_rgnpo(loc,env,rpo);});
({Cyc_Binding_resolve_aggrfields(loc,env,fs);});}}
# 1078
goto _LL0;}}}case 7U: _LLF: _tmp291=((struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct*)_tmp286)->f1;_LL10: {struct Cyc_Absyn_Enumdecl*ed=_tmp291;
# 1086
if(!({Cyc_Binding_at_toplevel(env);})){
({struct Cyc_Warn_String_Warn_Warg_struct _tmp2C6=({struct Cyc_Warn_String_Warn_Warg_struct _tmp344;_tmp344.tag=0U,({struct _fat_ptr _tmp455=({const char*_tmp2C7="nested type definitions are not yet supported";_tag_fat(_tmp2C7,sizeof(char),46U);});_tmp344.f1=_tmp455;});_tmp344;});void*_tmp2C5[1U];_tmp2C5[0]=& _tmp2C6;({unsigned _tmp456=loc;Cyc_Warn_err2(_tmp456,_tag_fat(_tmp2C5,sizeof(void*),1U));});});
goto _LL0;}
# 1086
({Cyc_Binding_absolutize_decl(loc,env,ed->name,ed->sc);});{
# 1091
struct _tuple0*_stmttmp20=ed->name;struct _fat_ptr*_tmp2C9;union Cyc_Absyn_Nmspace _tmp2C8;_LL30: _tmp2C8=_stmttmp20->f1;_tmp2C9=_stmttmp20->f2;_LL31: {union Cyc_Absyn_Nmspace decl_ns=_tmp2C8;struct _fat_ptr*decl_name=_tmp2C9;
# 1093
struct Cyc_Binding_ResolveNSEnv*decl_ns_data=({({struct Cyc_Binding_ResolveNSEnv*(*_tmp458)(struct Cyc_Binding_NSCtxt*ctxt,union Cyc_Absyn_Nmspace abs_ns)=({struct Cyc_Binding_ResolveNSEnv*(*_tmp2CE)(struct Cyc_Binding_NSCtxt*ctxt,union Cyc_Absyn_Nmspace abs_ns)=(struct Cyc_Binding_ResolveNSEnv*(*)(struct Cyc_Binding_NSCtxt*ctxt,union Cyc_Absyn_Nmspace abs_ns))Cyc_Binding_get_ns_data;_tmp2CE;});struct Cyc_Binding_NSCtxt*_tmp457=env->ns;_tmp458(_tmp457,decl_ns);});});
# 1095
if(({({int(*_tmp45A)(struct Cyc_Dict_Dict d,struct _fat_ptr*k)=({int(*_tmp2CA)(struct Cyc_Dict_Dict d,struct _fat_ptr*k)=(int(*)(struct Cyc_Dict_Dict d,struct _fat_ptr*k))Cyc_Dict_member;_tmp2CA;});struct Cyc_Dict_Dict _tmp459=decl_ns_data->enumdecls;_tmp45A(_tmp459,decl_name);});})&& ed->fields == 0)
goto _LL0;
# 1095
({struct Cyc_Dict_Dict _tmp45E=({({struct Cyc_Dict_Dict(*_tmp45D)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,struct Cyc_Absyn_Enumdecl*v)=({
# 1099
struct Cyc_Dict_Dict(*_tmp2CB)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,struct Cyc_Absyn_Enumdecl*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,struct Cyc_Absyn_Enumdecl*v))Cyc_Dict_insert;_tmp2CB;});struct Cyc_Dict_Dict _tmp45C=decl_ns_data->enumdecls;struct _fat_ptr*_tmp45B=decl_name;_tmp45D(_tmp45C,_tmp45B,ed);});});
# 1095
decl_ns_data->enumdecls=_tmp45E;});
# 1101
if(ed->fields != 0){
# 1103
struct Cyc_List_List*fs=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(ed->fields))->v;for(0;fs != 0;fs=fs->tl){
struct Cyc_Absyn_Enumfield*f=(struct Cyc_Absyn_Enumfield*)fs->hd;
({Cyc_Binding_absolutize_decl(f->loc,env,f->name,ed->sc);});
if(f->tag != 0)
({Cyc_Binding_resolve_exp(env,(struct Cyc_Absyn_Exp*)_check_null(f->tag));});
# 1106
({struct Cyc_Dict_Dict _tmp462=({({struct Cyc_Dict_Dict(*_tmp461)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,void*v)=({
# 1108
struct Cyc_Dict_Dict(*_tmp2CC)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,void*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,void*v))Cyc_Dict_insert;_tmp2CC;});struct Cyc_Dict_Dict _tmp460=decl_ns_data->ordinaries;struct _fat_ptr*_tmp45F=(*f->name).f2;_tmp461(_tmp460,_tmp45F,(void*)({struct Cyc_Binding_EnumRes_Binding_Resolved_struct*_tmp2CD=_cycalloc(sizeof(*_tmp2CD));_tmp2CD->tag=3U,_tmp2CD->f1=ed,_tmp2CD->f2=f;_tmp2CD;}));});});
# 1106
decl_ns_data->ordinaries=_tmp462;});}}
# 1101
goto _LL0;}}}case 6U: _LL11: _tmp290=((struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct*)_tmp286)->f1;_LL12: {struct Cyc_Absyn_Datatypedecl*tud=_tmp290;
# 1116
{struct _handler_cons _tmp2CF;_push_handler(& _tmp2CF);{int _tmp2D1=0;if(setjmp(_tmp2CF.handler))_tmp2D1=1;if(!_tmp2D1){
{struct Cyc_List_List*decls=({({struct Cyc_List_List*(*_tmp465)(unsigned loc,struct Cyc_Binding_NSCtxt*ctxt,struct _tuple0*qv,struct Cyc_List_List*(*lookup)(struct Cyc_Binding_ResolveNSEnv*,struct _fat_ptr*))=({struct Cyc_List_List*(*_tmp2D8)(unsigned loc,struct Cyc_Binding_NSCtxt*ctxt,struct _tuple0*qv,struct Cyc_List_List*(*lookup)(struct Cyc_Binding_ResolveNSEnv*,struct _fat_ptr*))=(struct Cyc_List_List*(*)(unsigned loc,struct Cyc_Binding_NSCtxt*ctxt,struct _tuple0*qv,struct Cyc_List_List*(*lookup)(struct Cyc_Binding_ResolveNSEnv*,struct _fat_ptr*)))Cyc_Binding_resolve_lookup;_tmp2D8;});unsigned _tmp464=loc;struct Cyc_Binding_NSCtxt*_tmp463=env->ns;_tmp465(_tmp464,_tmp463,tud->name,Cyc_Binding_lookup_datatypedecl);});});
struct Cyc_Absyn_Datatypedecl*last_decl=(struct Cyc_Absyn_Datatypedecl*)decls->hd;
if(!last_decl->is_extensible)
(int)_throw(({struct Cyc_Binding_BindingError_exn_struct*_tmp2D2=_cycalloc(sizeof(*_tmp2D2));_tmp2D2->tag=Cyc_Binding_BindingError;_tmp2D2;}));
# 1119
tud->name=last_decl->name;
# 1122
tud->is_extensible=1;{
struct _tuple0*_stmttmp21=tud->name;struct _fat_ptr*_tmp2D4;union Cyc_Absyn_Nmspace _tmp2D3;_LL33: _tmp2D3=_stmttmp21->f1;_tmp2D4=_stmttmp21->f2;_LL34: {union Cyc_Absyn_Nmspace decl_ns=_tmp2D3;struct _fat_ptr*decl_name=_tmp2D4;
struct Cyc_Binding_ResolveNSEnv*decl_ns_data=({({struct Cyc_Binding_ResolveNSEnv*(*_tmp467)(struct Cyc_Binding_NSCtxt*ctxt,union Cyc_Absyn_Nmspace abs_ns)=({struct Cyc_Binding_ResolveNSEnv*(*_tmp2D7)(struct Cyc_Binding_NSCtxt*ctxt,union Cyc_Absyn_Nmspace abs_ns)=(struct Cyc_Binding_ResolveNSEnv*(*)(struct Cyc_Binding_NSCtxt*ctxt,union Cyc_Absyn_Nmspace abs_ns))Cyc_Binding_get_ns_data;_tmp2D7;});struct Cyc_Binding_NSCtxt*_tmp466=env->ns;_tmp467(_tmp466,decl_ns);});});
({struct Cyc_Dict_Dict _tmp46B=({({struct Cyc_Dict_Dict(*_tmp46A)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,struct Cyc_List_List*v)=({struct Cyc_Dict_Dict(*_tmp2D5)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,struct Cyc_List_List*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,struct Cyc_List_List*v))Cyc_Dict_insert;_tmp2D5;});struct Cyc_Dict_Dict _tmp469=decl_ns_data->datatypedecls;struct _fat_ptr*_tmp468=decl_name;_tmp46A(_tmp469,_tmp468,({struct Cyc_List_List*_tmp2D6=_cycalloc(sizeof(*_tmp2D6));_tmp2D6->hd=tud,_tmp2D6->tl=decls;_tmp2D6;}));});});decl_ns_data->datatypedecls=_tmp46B;});}}}
# 1117
;_pop_handler();}else{void*_tmp2D0=(void*)Cyc_Core_get_exn_thrown();void*_tmp2D9=_tmp2D0;void*_tmp2DA;if(((struct Cyc_Binding_BindingError_exn_struct*)_tmp2D9)->tag == Cyc_Binding_BindingError){_LL36: _LL37:
# 1129
({Cyc_Binding_absolutize_topdecl(loc,env,tud->name,tud->sc);});{
struct _tuple0*_stmttmp22=tud->name;struct _fat_ptr*_tmp2DC;union Cyc_Absyn_Nmspace _tmp2DB;_LL3B: _tmp2DB=_stmttmp22->f1;_tmp2DC=_stmttmp22->f2;_LL3C: {union Cyc_Absyn_Nmspace decl_ns=_tmp2DB;struct _fat_ptr*decl_name=_tmp2DC;
struct Cyc_Binding_ResolveNSEnv*decl_ns_data=({({struct Cyc_Binding_ResolveNSEnv*(*_tmp46D)(struct Cyc_Binding_NSCtxt*ctxt,union Cyc_Absyn_Nmspace abs_ns)=({struct Cyc_Binding_ResolveNSEnv*(*_tmp2E0)(struct Cyc_Binding_NSCtxt*ctxt,union Cyc_Absyn_Nmspace abs_ns)=(struct Cyc_Binding_ResolveNSEnv*(*)(struct Cyc_Binding_NSCtxt*ctxt,union Cyc_Absyn_Nmspace abs_ns))Cyc_Binding_get_ns_data;_tmp2E0;});struct Cyc_Binding_NSCtxt*_tmp46C=env->ns;_tmp46D(_tmp46C,decl_ns);});});
# 1133
if(({({int(*_tmp46F)(struct Cyc_Dict_Dict d,struct _fat_ptr*k)=({int(*_tmp2DD)(struct Cyc_Dict_Dict d,struct _fat_ptr*k)=(int(*)(struct Cyc_Dict_Dict d,struct _fat_ptr*k))Cyc_Dict_member;_tmp2DD;});struct Cyc_Dict_Dict _tmp46E=decl_ns_data->datatypedecls;_tmp46F(_tmp46E,decl_name);});})&& tud->fields == 0)
# 1135
goto _LL35;
# 1133
({struct Cyc_Dict_Dict _tmp473=({({struct Cyc_Dict_Dict(*_tmp472)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,struct Cyc_List_List*v)=({
# 1136
struct Cyc_Dict_Dict(*_tmp2DE)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,struct Cyc_List_List*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,struct Cyc_List_List*v))Cyc_Dict_insert;_tmp2DE;});struct Cyc_Dict_Dict _tmp471=decl_ns_data->datatypedecls;struct _fat_ptr*_tmp470=decl_name;_tmp472(_tmp471,_tmp470,({struct Cyc_List_List*_tmp2DF=_cycalloc(sizeof(*_tmp2DF));_tmp2DF->hd=tud,_tmp2DF->tl=0;_tmp2DF;}));});});
# 1133
decl_ns_data->datatypedecls=_tmp473;});
# 1138
goto _LL35;}}}else{_LL38: _tmp2DA=_tmp2D9;_LL39: {void*exn=_tmp2DA;(int)_rethrow(exn);}}_LL35:;}}}{
# 1141
struct _tuple0*_stmttmp23=tud->name;struct _fat_ptr*_tmp2E2;union Cyc_Absyn_Nmspace _tmp2E1;_LL3E: _tmp2E1=_stmttmp23->f1;_tmp2E2=_stmttmp23->f2;_LL3F: {union Cyc_Absyn_Nmspace decl_ns=_tmp2E1;struct _fat_ptr*decl_name=_tmp2E2;
if(tud->fields != 0){
struct Cyc_List_List*fs=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(tud->fields))->v;for(0;fs != 0;fs=fs->tl){
struct Cyc_Absyn_Datatypefield*f=(struct Cyc_Absyn_Datatypefield*)fs->hd;
{struct Cyc_List_List*tqts=f->typs;for(0;tqts != 0;tqts=tqts->tl){
({Cyc_Binding_resolve_type(loc,env,(*((struct _tuple14*)tqts->hd)).f2);});}}
{union Cyc_Absyn_Nmspace _stmttmp24=(*f->name).f1;union Cyc_Absyn_Nmspace _tmp2E3=_stmttmp24;switch((_tmp2E3.Abs_n).tag){case 1U: if((_tmp2E3.Rel_n).val == 0){_LL41: _LL42:
# 1149
 if(tud->is_extensible)
({union Cyc_Absyn_Nmspace _tmp474=({Cyc_Absyn_Abs_n((env->ns)->curr_ns,0);});(*f->name).f1=_tmp474;});else{
# 1152
(*f->name).f1=(*tud->name).f1;}
goto _LL40;}else{_LL43: _LL44:
# 1155
({struct Cyc_Warn_String_Warn_Warg_struct _tmp2E5=({struct Cyc_Warn_String_Warn_Warg_struct _tmp345;_tmp345.tag=0U,({struct _fat_ptr _tmp475=({const char*_tmp2E6="qualified datatype field declaratations not allowed";_tag_fat(_tmp2E6,sizeof(char),52U);});_tmp345.f1=_tmp475;});_tmp345;});void*_tmp2E4[1U];_tmp2E4[0]=& _tmp2E5;({unsigned _tmp476=f->loc;Cyc_Warn_err2(_tmp476,_tag_fat(_tmp2E4,sizeof(void*),1U));});});
return;}case 2U: _LL45: _LL46:
 goto _LL40;default: _LL47: _LL48:
({void*_tmp2E7=0U;({int(*_tmp478)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp2E9)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp2E9;});struct _fat_ptr _tmp477=({const char*_tmp2E8="datatype field Loc_n or C_n";_tag_fat(_tmp2E8,sizeof(char),28U);});_tmp478(_tmp477,_tag_fat(_tmp2E7,sizeof(void*),0U));});});}_LL40:;}{
# 1160
struct Cyc_Binding_ResolveNSEnv*ns_data=({({struct Cyc_Binding_ResolveNSEnv*(*_tmp47A)(struct Cyc_Binding_NSCtxt*ctxt,union Cyc_Absyn_Nmspace abs_ns)=({struct Cyc_Binding_ResolveNSEnv*(*_tmp2EC)(struct Cyc_Binding_NSCtxt*ctxt,union Cyc_Absyn_Nmspace abs_ns)=(struct Cyc_Binding_ResolveNSEnv*(*)(struct Cyc_Binding_NSCtxt*ctxt,union Cyc_Absyn_Nmspace abs_ns))Cyc_Binding_get_ns_data;_tmp2EC;});struct Cyc_Binding_NSCtxt*_tmp479=env->ns;_tmp47A(_tmp479,(*f->name).f1);});});
({struct Cyc_Dict_Dict _tmp47E=({({struct Cyc_Dict_Dict(*_tmp47D)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,void*v)=({struct Cyc_Dict_Dict(*_tmp2EA)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,void*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,void*v))Cyc_Dict_insert;_tmp2EA;});struct Cyc_Dict_Dict _tmp47C=ns_data->ordinaries;struct _fat_ptr*_tmp47B=(*f->name).f2;_tmp47D(_tmp47C,_tmp47B,(void*)({struct Cyc_Binding_DatatypeRes_Binding_Resolved_struct*_tmp2EB=_cycalloc(sizeof(*_tmp2EB));_tmp2EB->tag=2U,_tmp2EB->f1=tud,_tmp2EB->f2=f;_tmp2EB;}));});});ns_data->ordinaries=_tmp47E;});}}}
# 1142
goto _LL0;}}}case 9U: _LL13: _tmp28E=((struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct*)_tmp286)->f1;_tmp28F=((struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct*)_tmp286)->f2;_LL14: {struct _fat_ptr*v=_tmp28E;struct Cyc_List_List*ds2=_tmp28F;
# 1168
({({void(*_tmp480)(struct Cyc_Binding_NSCtxt*ctxt,struct _fat_ptr*subname,int env,struct Cyc_Binding_ResolveNSEnv*(*mkdata)(int))=({void(*_tmp2ED)(struct Cyc_Binding_NSCtxt*ctxt,struct _fat_ptr*subname,int env,struct Cyc_Binding_ResolveNSEnv*(*mkdata)(int))=(void(*)(struct Cyc_Binding_NSCtxt*ctxt,struct _fat_ptr*subname,int env,struct Cyc_Binding_ResolveNSEnv*(*mkdata)(int)))Cyc_Binding_enter_ns;_tmp2ED;});struct Cyc_Binding_NSCtxt*_tmp47F=env->ns;_tmp480(_tmp47F,v,1,Cyc_Binding_mt_renv);});});
({Cyc_Binding_resolve_decls(env,ds2);});
({Cyc_Binding_leave_ns(env->ns);});
goto _LL0;}case 10U: _LL15: _tmp28C=((struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct*)_tmp286)->f1;_tmp28D=((struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct*)_tmp286)->f2;_LL16: {struct _tuple0*qv=_tmp28C;struct Cyc_List_List*ds2=_tmp28D;
# 1173
({Cyc_Binding_enter_using(d->loc,env->ns,qv);});
({Cyc_Binding_resolve_decls(env,ds2);});
({Cyc_Binding_leave_using(env->ns);});
goto _LL0;}case 11U: _LL17: _tmp28B=((struct Cyc_Absyn_ExternC_d_Absyn_Raw_decl_struct*)_tmp286)->f1;_LL18: {struct Cyc_List_List*ds2=_tmp28B;
# 1179
int old=env->in_cinclude;
env->in_cinclude=1;
({Cyc_Binding_resolve_decls(env,ds2);});
env->in_cinclude=old;
goto _LL0;}case 12U: _LL19: _tmp287=((struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct*)_tmp286)->f1;_tmp288=((struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct*)_tmp286)->f2;_tmp289=(struct Cyc_List_List**)&((struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct*)_tmp286)->f3;_tmp28A=((struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct*)_tmp286)->f4;_LL1A: {struct Cyc_List_List*ds2=_tmp287;struct Cyc_List_List*ovrs=_tmp288;struct Cyc_List_List**exports=_tmp289;struct _tuple11*hides=_tmp28A;
# 1190
{struct Cyc_List_List*exs=*exports;for(0;exs != 0;exs=exs->tl){
struct _tuple18*_stmttmp25=(struct _tuple18*)exs->hd;struct _tuple0*_tmp2EF;unsigned _tmp2EE;_LL4A: _tmp2EE=_stmttmp25->f1;_tmp2EF=_stmttmp25->f2;_LL4B: {unsigned loc=_tmp2EE;struct _tuple0*qv=_tmp2EF;
({Cyc_Binding_absolutize_decl(loc,env,qv,Cyc_Absyn_ExternC);});}}}
# 1198
if(!({Cyc_Binding_at_toplevel(env);})){
({struct Cyc_Warn_String_Warn_Warg_struct _tmp2F1=({struct Cyc_Warn_String_Warn_Warg_struct _tmp346;_tmp346.tag=0U,({struct _fat_ptr _tmp481=({const char*_tmp2F2="extern \"C include\" not at toplevel";_tag_fat(_tmp2F2,sizeof(char),35U);});_tmp346.f1=_tmp481;});_tmp346;});void*_tmp2F0[1U];_tmp2F0[0]=& _tmp2F1;({unsigned _tmp482=loc;Cyc_Warn_err2(_tmp482,_tag_fat(_tmp2F0,sizeof(void*),1U));});});
goto _LL0;}{
# 1198
struct Cyc_Binding_ResolveNSEnv*data=({({struct Cyc_Binding_ResolveNSEnv*(*_tmp484)(struct Cyc_Dict_Dict d,struct Cyc_List_List*k)=({
# 1202
struct Cyc_Binding_ResolveNSEnv*(*_tmp30A)(struct Cyc_Dict_Dict d,struct Cyc_List_List*k)=(struct Cyc_Binding_ResolveNSEnv*(*)(struct Cyc_Dict_Dict d,struct Cyc_List_List*k))Cyc_Dict_lookup;_tmp30A;});struct Cyc_Dict_Dict _tmp483=(env->ns)->ns_data;_tmp484(_tmp483,(env->ns)->curr_ns);});});
struct Cyc_Dict_Dict old_dict=data->ordinaries;
int old=env->in_cinclude;
env->in_cinclude=1;
({Cyc_Binding_resolve_decls(env,ds2);});
({Cyc_Binding_resolve_decls(env,ovrs);});{
struct Cyc_Dict_Dict new_dict=data->ordinaries;
struct Cyc_Dict_Dict out_dict=old_dict;
if((*hides).f1 > (unsigned)0){
if((unsigned)*exports)
({void*_tmp2F3=0U;({unsigned _tmp486=(*hides).f1;struct _fat_ptr _tmp485=({const char*_tmp2F4="export wildcard expects empty export list";_tag_fat(_tmp2F4,sizeof(char),42U);});Cyc_Warn_err(_tmp486,_tmp485,_tag_fat(_tmp2F3,sizeof(void*),0U));});});
# 1211
env->in_cinclude=old;
# 1214
({void(*_tmp2F5)(void(*f)(struct _tuple17*,struct _fat_ptr*,void*),struct _tuple17*env,struct Cyc_Dict_Dict d)=({void(*_tmp2F6)(void(*f)(struct _tuple17*,struct _fat_ptr*,void*),struct _tuple17*env,struct Cyc_Dict_Dict d)=(void(*)(void(*f)(struct _tuple17*,struct _fat_ptr*,void*),struct _tuple17*env,struct Cyc_Dict_Dict d))Cyc_Dict_iter_c;_tmp2F6;});void(*_tmp2F7)(struct _tuple17*cenv,struct _fat_ptr*name,void*res)=Cyc_Binding_export_all_symbols;struct _tuple17*_tmp2F8=({struct _tuple17*_tmp2F9=_cycalloc(sizeof(*_tmp2F9));_tmp2F9->f1=exports,_tmp2F9->f2=& out_dict,_tmp2F9->f3=env,_tmp2F9->f4=hides;_tmp2F9;});struct Cyc_Dict_Dict _tmp2FA=({Cyc_Dict_difference(new_dict,old_dict);});_tmp2F5(_tmp2F7,_tmp2F8,_tmp2FA);});}else{
# 1218
struct Cyc_List_List*exs=*exports;for(0;exs != 0;exs=exs->tl){
struct _tuple18*_stmttmp26=(struct _tuple18*)exs->hd;struct _fat_ptr*_tmp2FC;unsigned _tmp2FB;_LL4D: _tmp2FB=_stmttmp26->f1;_tmp2FC=(_stmttmp26->f2)->f2;_LL4E: {unsigned loc=_tmp2FB;struct _fat_ptr*v=_tmp2FC;
if(!({({int(*_tmp48F)(struct Cyc_Dict_Dict d,struct _fat_ptr*k)=({int(*_tmp2FD)(struct Cyc_Dict_Dict d,struct _fat_ptr*k)=(int(*)(struct Cyc_Dict_Dict d,struct _fat_ptr*k))Cyc_Dict_member;_tmp2FD;});struct Cyc_Dict_Dict _tmp48E=new_dict;_tmp48F(_tmp48E,v);});})||
({({int(*_tmp48D)(struct Cyc_Dict_Dict d,struct _fat_ptr*k)=({int(*_tmp2FE)(struct Cyc_Dict_Dict d,struct _fat_ptr*k)=(int(*)(struct Cyc_Dict_Dict d,struct _fat_ptr*k))Cyc_Dict_member;_tmp2FE;});struct Cyc_Dict_Dict _tmp48C=old_dict;_tmp48D(_tmp48C,v);});})&&({
void*_tmp48B=({({void*(*_tmp488)(struct Cyc_Dict_Dict d,struct _fat_ptr*k)=({void*(*_tmp2FF)(struct Cyc_Dict_Dict d,struct _fat_ptr*k)=(void*(*)(struct Cyc_Dict_Dict d,struct _fat_ptr*k))Cyc_Dict_lookup;_tmp2FF;});struct Cyc_Dict_Dict _tmp487=old_dict;_tmp488(_tmp487,v);});});_tmp48B == ({({void*(*_tmp48A)(struct Cyc_Dict_Dict d,struct _fat_ptr*k)=({void*(*_tmp300)(struct Cyc_Dict_Dict d,struct _fat_ptr*k)=(void*(*)(struct Cyc_Dict_Dict d,struct _fat_ptr*k))Cyc_Dict_lookup;_tmp300;});struct Cyc_Dict_Dict _tmp489=new_dict;_tmp48A(_tmp489,v);});});}))
({struct Cyc_String_pa_PrintArg_struct _tmp303=({struct Cyc_String_pa_PrintArg_struct _tmp347;_tmp347.tag=0U,_tmp347.f1=(struct _fat_ptr)((struct _fat_ptr)*v);_tmp347;});void*_tmp301[1U];_tmp301[0]=& _tmp303;({unsigned _tmp491=loc;struct _fat_ptr _tmp490=({const char*_tmp302="%s is exported but not defined";_tag_fat(_tmp302,sizeof(char),31U);});Cyc_Warn_err(_tmp491,_tmp490,_tag_fat(_tmp301,sizeof(void*),1U));});});
# 1220
out_dict=({struct Cyc_Dict_Dict(*_tmp304)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,void*v)=({
# 1224
struct Cyc_Dict_Dict(*_tmp305)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,void*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,void*v))Cyc_Dict_insert;_tmp305;});struct Cyc_Dict_Dict _tmp306=out_dict;struct _fat_ptr*_tmp307=v;void*_tmp308=({({void*(*_tmp493)(struct Cyc_Dict_Dict d,struct _fat_ptr*k)=({void*(*_tmp309)(struct Cyc_Dict_Dict d,struct _fat_ptr*k)=(void*(*)(struct Cyc_Dict_Dict d,struct _fat_ptr*k))Cyc_Dict_lookup;_tmp309;});struct Cyc_Dict_Dict _tmp492=new_dict;_tmp493(_tmp492,v);});});_tmp304(_tmp306,_tmp307,_tmp308);});}}}
# 1227
data->ordinaries=out_dict;
env->in_cinclude=old;
# 1231
({Cyc_Cifc_user_overrides(loc,ds2,ovrs);});
goto _LL0;}}}case 13U: _LL1B: _LL1C:
# 1234
 goto _LL1E;case 14U: _LL1D: _LL1E:
 goto _LL20;case 15U: _LL1F: _LL20:
 goto _LL22;default: _LL21: _LL22:
 goto _LL0;}_LL0:;}
# 1241
void Cyc_Binding_resolve_decls(struct Cyc_Binding_Env*env,struct Cyc_List_List*tds){
for(0;tds != 0;tds=tds->tl){
({Cyc_Binding_resolve_decl(env,(struct Cyc_Absyn_Decl*)tds->hd);});}}
# 1246
void Cyc_Binding_resolve_all(struct Cyc_List_List*tds){
struct Cyc_Binding_Env*env=({struct Cyc_Binding_Env*_tmp313=_cycalloc(sizeof(*_tmp313));_tmp313->in_cinclude=0,({struct Cyc_Binding_NSCtxt*_tmp494=({({struct Cyc_Binding_NSCtxt*(*_tmp312)(int env,struct Cyc_Binding_ResolveNSEnv*(*mkdata)(int))=(struct Cyc_Binding_NSCtxt*(*)(int env,struct Cyc_Binding_ResolveNSEnv*(*mkdata)(int)))Cyc_Binding_mt_nsctxt;_tmp312;})(1,Cyc_Binding_mt_renv);});_tmp313->ns=_tmp494;}),_tmp313->local_vars=0;_tmp313;});
({void(*_tmp30D)(struct Cyc_Binding_Env*env,struct Cyc_Absyn_Decl*d)=Cyc_Binding_resolve_decl;struct Cyc_Binding_Env*_tmp30E=env;struct Cyc_Absyn_Decl*_tmp30F=({struct Cyc_Absyn_Decl*_tmp311=_cycalloc(sizeof(*_tmp311));({void*_tmp496=(void*)({struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct*_tmp310=_cycalloc(sizeof(*_tmp310));_tmp310->tag=6U,({struct Cyc_Absyn_Datatypedecl*_tmp495=({Cyc_Absyn_exn_tud();});_tmp310->f1=_tmp495;});_tmp310;});_tmp311->r=_tmp496;}),_tmp311->loc=0U;_tmp311;});_tmp30D(_tmp30E,_tmp30F);});
({Cyc_Binding_resolve_decls(env,tds);});}

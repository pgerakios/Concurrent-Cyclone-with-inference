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
# 168
extern struct _RegionHandle*Cyc_Core_unique_region;struct Cyc_Core_DynamicRegion;struct Cyc_Core_NewDynamicRegion{struct Cyc_Core_DynamicRegion*key;};struct Cyc_Core_ThinRes{void*arr;unsigned nelts;};struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};
# 61 "list.h"
int Cyc_List_length(struct Cyc_List_List*x);extern char Cyc_List_List_mismatch[14U];struct Cyc_List_List_mismatch_exn_struct{char*tag;};extern char Cyc_List_Nth[4U];struct Cyc_List_Nth_exn_struct{char*tag;};struct Cyc___cycFILE;struct Cyc_String_pa_PrintArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Int_pa_PrintArg_struct{int tag;unsigned long f1;};struct Cyc_Double_pa_PrintArg_struct{int tag;double f1;};struct Cyc_LongDouble_pa_PrintArg_struct{int tag;long double f1;};struct Cyc_ShortPtr_pa_PrintArg_struct{int tag;short*f1;};struct Cyc_IntPtr_pa_PrintArg_struct{int tag;unsigned long*f1;};
# 100 "cycboot.h"
int Cyc_fprintf(struct Cyc___cycFILE*,struct _fat_ptr,struct _fat_ptr);struct Cyc_ShortPtr_sa_ScanfArg_struct{int tag;short*f1;};struct Cyc_UShortPtr_sa_ScanfArg_struct{int tag;unsigned short*f1;};struct Cyc_IntPtr_sa_ScanfArg_struct{int tag;int*f1;};struct Cyc_UIntPtr_sa_ScanfArg_struct{int tag;unsigned*f1;};struct Cyc_StringPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_DoublePtr_sa_ScanfArg_struct{int tag;double*f1;};struct Cyc_FloatPtr_sa_ScanfArg_struct{int tag;float*f1;};struct Cyc_CharPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};extern char Cyc_FileCloseError[15U];struct Cyc_FileCloseError_exn_struct{char*tag;};extern char Cyc_FileOpenError[14U];struct Cyc_FileOpenError_exn_struct{char*tag;struct _fat_ptr f1;};struct Cyc_Iter_Iter{void*env;int(*next)(void*env,void*dest);};struct Cyc_Set_Set;
# 51 "set.h"
struct Cyc_Set_Set*Cyc_Set_empty(int(*cmp)(void*,void*));
# 63
struct Cyc_Set_Set*Cyc_Set_insert(struct Cyc_Set_Set*s,void*elt);
# 75
struct Cyc_Set_Set*Cyc_Set_union_two(struct Cyc_Set_Set*s1,struct Cyc_Set_Set*s2);
# 82
struct Cyc_Set_Set*Cyc_Set_diff(struct Cyc_Set_Set*s1,struct Cyc_Set_Set*s2);
# 85
struct Cyc_Set_Set*Cyc_Set_delete(struct Cyc_Set_Set*s,void*elt);
# 97
int Cyc_Set_is_empty(struct Cyc_Set_Set*s);
# 100
int Cyc_Set_member(struct Cyc_Set_Set*s,void*elt);extern char Cyc_Set_Absent[7U];struct Cyc_Set_Absent_exn_struct{char*tag;};
# 137
void*Cyc_Set_choose(struct Cyc_Set_Set*s);struct Cyc_Dict_T;struct Cyc_Dict_Dict{int(*rel)(void*,void*);struct _RegionHandle*r;const struct Cyc_Dict_T*t;};extern char Cyc_Dict_Present[8U];struct Cyc_Dict_Present_exn_struct{char*tag;};extern char Cyc_Dict_Absent[7U];struct Cyc_Dict_Absent_exn_struct{char*tag;};
# 62 "dict.h"
struct Cyc_Dict_Dict Cyc_Dict_empty(int(*cmp)(void*,void*));
# 83
int Cyc_Dict_member(struct Cyc_Dict_Dict d,void*k);
# 87
struct Cyc_Dict_Dict Cyc_Dict_insert(struct Cyc_Dict_Dict d,void*k,void*v);
# 110
void*Cyc_Dict_lookup(struct Cyc_Dict_Dict d,void*k);
# 122 "dict.h"
void**Cyc_Dict_lookup_opt(struct Cyc_Dict_Dict d,void*k);
# 233
struct Cyc_List_List*Cyc_Dict_to_list(struct Cyc_Dict_Dict d);
# 270
struct Cyc_Dict_Dict Cyc_Dict_delete(struct Cyc_Dict_Dict,void*);
# 50 "graph.h"
struct Cyc_Dict_Dict Cyc_Graph_empty(int(*cmp)(void*,void*));
# 53
struct Cyc_Dict_Dict Cyc_Graph_add_node(struct Cyc_Dict_Dict g,void*s);
# 56
struct Cyc_Dict_Dict Cyc_Graph_add_edge(struct Cyc_Dict_Dict g,void*s,void*t);
# 59
struct Cyc_Dict_Dict Cyc_Graph_add_edges(struct Cyc_Dict_Dict g,void*s,struct Cyc_Set_Set*T);
# 69
int Cyc_Graph_is_edge(struct Cyc_Dict_Dict g,void*s,void*t);
# 72
struct Cyc_Set_Set*Cyc_Graph_get_targets(struct Cyc_Dict_Dict,void*s);
# 27 "graph.cyc"
struct Cyc_Dict_Dict Cyc_Graph_empty(int(*cmp)(void*,void*)){
return({Cyc_Dict_empty(cmp);});}
# 34
struct Cyc_Set_Set*Cyc_Graph_get_targets(struct Cyc_Dict_Dict g,void*source){
struct Cyc_Set_Set**_stmttmp0=({({struct Cyc_Set_Set**(*_tmp8F)(struct Cyc_Dict_Dict d,void*k)=({struct Cyc_Set_Set**(*_tmp3)(struct Cyc_Dict_Dict d,void*k)=(struct Cyc_Set_Set**(*)(struct Cyc_Dict_Dict d,void*k))Cyc_Dict_lookup_opt;_tmp3;});struct Cyc_Dict_Dict _tmp8E=g;_tmp8F(_tmp8E,source);});});struct Cyc_Set_Set**_tmp1=_stmttmp0;struct Cyc_Set_Set*_tmp2;if(_tmp1 == 0){_LL1: _LL2:
 return({Cyc_Set_empty(g.rel);});}else{_LL3: _tmp2=*_tmp1;_LL4: {struct Cyc_Set_Set*targets=_tmp2;
return targets;}}_LL0:;}
# 45
struct Cyc_Dict_Dict Cyc_Graph_add_node(struct Cyc_Dict_Dict g,void*source){
if(({Cyc_Dict_member(g,source);}))return g;else{
return({struct Cyc_Dict_Dict(*_tmp5)(struct Cyc_Dict_Dict d,void*k,struct Cyc_Set_Set*v)=({struct Cyc_Dict_Dict(*_tmp6)(struct Cyc_Dict_Dict d,void*k,struct Cyc_Set_Set*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,void*k,struct Cyc_Set_Set*v))Cyc_Dict_insert;_tmp6;});struct Cyc_Dict_Dict _tmp7=g;void*_tmp8=source;struct Cyc_Set_Set*_tmp9=({Cyc_Set_empty(g.rel);});_tmp5(_tmp7,_tmp8,_tmp9);});}}
# 53
struct Cyc_Dict_Dict Cyc_Graph_add_edge(struct Cyc_Dict_Dict g,void*source,void*target){
struct Cyc_Set_Set*sourceTargets=({Cyc_Graph_get_targets(g,source);});
return({struct Cyc_Dict_Dict(*_tmpB)(struct Cyc_Dict_Dict d,void*k,struct Cyc_Set_Set*v)=({struct Cyc_Dict_Dict(*_tmpC)(struct Cyc_Dict_Dict d,void*k,struct Cyc_Set_Set*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,void*k,struct Cyc_Set_Set*v))Cyc_Dict_insert;_tmpC;});struct Cyc_Dict_Dict _tmpD=g;void*_tmpE=source;struct Cyc_Set_Set*_tmpF=({Cyc_Set_insert(sourceTargets,target);});_tmpB(_tmpD,_tmpE,_tmpF);});}
# 61
struct Cyc_Dict_Dict Cyc_Graph_add_edges(struct Cyc_Dict_Dict g,void*source,struct Cyc_Set_Set*targets){
struct Cyc_Set_Set*sourceTargets=({Cyc_Graph_get_targets(g,source);});
return({struct Cyc_Dict_Dict(*_tmp11)(struct Cyc_Dict_Dict d,void*k,struct Cyc_Set_Set*v)=({struct Cyc_Dict_Dict(*_tmp12)(struct Cyc_Dict_Dict d,void*k,struct Cyc_Set_Set*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,void*k,struct Cyc_Set_Set*v))Cyc_Dict_insert;_tmp12;});struct Cyc_Dict_Dict _tmp13=g;void*_tmp14=source;struct Cyc_Set_Set*_tmp15=({Cyc_Set_union_two(sourceTargets,targets);});_tmp11(_tmp13,_tmp14,_tmp15);});}
# 69
struct Cyc_Dict_Dict Cyc_Graph_remove_edge(struct Cyc_Dict_Dict g,void*source,void*target){
struct Cyc_Set_Set*sourceTargets=({Cyc_Graph_get_targets(g,source);});
return({struct Cyc_Dict_Dict(*_tmp17)(struct Cyc_Dict_Dict d,void*k,struct Cyc_Set_Set*v)=({struct Cyc_Dict_Dict(*_tmp18)(struct Cyc_Dict_Dict d,void*k,struct Cyc_Set_Set*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,void*k,struct Cyc_Set_Set*v))Cyc_Dict_insert;_tmp18;});struct Cyc_Dict_Dict _tmp19=g;void*_tmp1A=source;struct Cyc_Set_Set*_tmp1B=({Cyc_Set_delete(sourceTargets,target);});_tmp17(_tmp19,_tmp1A,_tmp1B);});}
# 77
struct Cyc_Dict_Dict Cyc_Graph_remove_edges(struct Cyc_Dict_Dict g,void*source,struct Cyc_Set_Set*targets){
struct Cyc_Set_Set*sourceTargets=({Cyc_Graph_get_targets(g,source);});
return({struct Cyc_Dict_Dict(*_tmp1D)(struct Cyc_Dict_Dict d,void*k,struct Cyc_Set_Set*v)=({struct Cyc_Dict_Dict(*_tmp1E)(struct Cyc_Dict_Dict d,void*k,struct Cyc_Set_Set*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,void*k,struct Cyc_Set_Set*v))Cyc_Dict_insert;_tmp1E;});struct Cyc_Dict_Dict _tmp1F=g;void*_tmp20=source;struct Cyc_Set_Set*_tmp21=({Cyc_Set_diff(sourceTargets,targets);});_tmp1D(_tmp1F,_tmp20,_tmp21);});}
# 87
static struct Cyc_List_List*Cyc_Graph_to_list(struct Cyc_Set_Set*ts){
struct Cyc_List_List*result=0;
while(!({Cyc_Set_is_empty(ts);})){
void*z=({Cyc_Set_choose(ts);});
ts=({Cyc_Set_delete(ts,z);});
result=({struct Cyc_List_List*_tmp23=_cycalloc(sizeof(*_tmp23));_tmp23->hd=z,_tmp23->tl=result;_tmp23;});}
# 94
return result;}
# 100
int Cyc_Graph_is_edge(struct Cyc_Dict_Dict g,void*source,void*target){
struct Cyc_Set_Set**_stmttmp1=({({struct Cyc_Set_Set**(*_tmp91)(struct Cyc_Dict_Dict d,void*k)=({struct Cyc_Set_Set**(*_tmp27)(struct Cyc_Dict_Dict d,void*k)=(struct Cyc_Set_Set**(*)(struct Cyc_Dict_Dict d,void*k))Cyc_Dict_lookup_opt;_tmp27;});struct Cyc_Dict_Dict _tmp90=g;_tmp91(_tmp90,source);});});struct Cyc_Set_Set**_tmp25=_stmttmp1;struct Cyc_Set_Set*_tmp26;if(_tmp25 == 0){_LL1: _LL2:
# 103
 return 0;}else{_LL3: _tmp26=*_tmp25;_LL4: {struct Cyc_Set_Set*sourceTargets=_tmp26;
# 105
return({Cyc_Set_member(sourceTargets,target);});}}_LL0:;}struct _tuple0{void*f1;struct Cyc_Set_Set*f2;};
# 112
void Cyc_Graph_print(struct Cyc___cycFILE*f,struct Cyc_Dict_Dict g,void(*nodeprint)(struct Cyc___cycFILE*,void*)){
({void*_tmp29=0U;({struct Cyc___cycFILE*_tmp93=f;struct _fat_ptr _tmp92=({const char*_tmp2A="digraph {\n";_tag_fat(_tmp2A,sizeof(char),11U);});Cyc_fprintf(_tmp93,_tmp92,_tag_fat(_tmp29,sizeof(void*),0U));});});{
struct Cyc_List_List*edges=({Cyc_Dict_to_list(g);});
for(0;(unsigned)edges;edges=edges->tl){
struct _tuple0*_stmttmp2=(struct _tuple0*)edges->hd;struct Cyc_Set_Set*_tmp2C;void*_tmp2B;_LL1: _tmp2B=_stmttmp2->f1;_tmp2C=_stmttmp2->f2;_LL2: {void*s=_tmp2B;struct Cyc_Set_Set*ts=_tmp2C;
struct Cyc_List_List*tslist=({Cyc_Graph_to_list(ts);});
if((unsigned)tslist)
for(0;(unsigned)tslist;tslist=tslist->tl){
({nodeprint(f,s);});
({void*_tmp2D=0U;({struct Cyc___cycFILE*_tmp95=f;struct _fat_ptr _tmp94=({const char*_tmp2E=" -> ";_tag_fat(_tmp2E,sizeof(char),5U);});Cyc_fprintf(_tmp95,_tmp94,_tag_fat(_tmp2D,sizeof(void*),0U));});});
({nodeprint(f,tslist->hd);});
({void*_tmp2F=0U;({struct Cyc___cycFILE*_tmp97=f;struct _fat_ptr _tmp96=({const char*_tmp30=";\n";_tag_fat(_tmp30,sizeof(char),3U);});Cyc_fprintf(_tmp97,_tmp96,_tag_fat(_tmp2F,sizeof(void*),0U));});});}else{
# 126
({void*_tmp31=0U;({struct Cyc___cycFILE*_tmp99=f;struct _fat_ptr _tmp98=({const char*_tmp32="node ";_tag_fat(_tmp32,sizeof(char),6U);});Cyc_fprintf(_tmp99,_tmp98,_tag_fat(_tmp31,sizeof(void*),0U));});});
({nodeprint(f,s);});
({void*_tmp33=0U;({struct Cyc___cycFILE*_tmp9B=f;struct _fat_ptr _tmp9A=({const char*_tmp34="; // no targets\n";_tag_fat(_tmp34,sizeof(char),17U);});Cyc_fprintf(_tmp9B,_tmp9A,_tag_fat(_tmp33,sizeof(void*),0U));});});}}}
# 131
({void*_tmp35=0U;({struct Cyc___cycFILE*_tmp9D=f;struct _fat_ptr _tmp9C=({const char*_tmp36="}\n";_tag_fat(_tmp36,sizeof(char),3U);});Cyc_fprintf(_tmp9D,_tmp9C,_tag_fat(_tmp35,sizeof(void*),0U));});});}}
# 141 "graph.cyc"
static struct Cyc_List_List*Cyc_Graph_sourcesOf(struct Cyc_Dict_Dict g,void*node){
struct Cyc_List_List*result=0;
# 144
{struct Cyc_List_List*edges=({Cyc_Dict_to_list(g);});for(0;(unsigned)edges;edges=edges->tl){
struct _tuple0*_stmttmp3=(struct _tuple0*)edges->hd;struct Cyc_Set_Set*_tmp39;void*_tmp38;_LL1: _tmp38=_stmttmp3->f1;_tmp39=_stmttmp3->f2;_LL2: {void*s=_tmp38;struct Cyc_Set_Set*ts=_tmp39;
if(({Cyc_Set_member(ts,node);}))result=({struct Cyc_List_List*_tmp3A=_cycalloc(sizeof(*_tmp3A));_tmp3A->hd=s,_tmp3A->tl=result;_tmp3A;});}}}
# 148
return result;}struct _tuple1{struct Cyc_Set_Set*f1;struct Cyc_Dict_Dict f2;};
# 158
static struct _tuple1 Cyc_Graph_divideGraph(struct Cyc_Dict_Dict g,void*source){
# 160
struct Cyc_Set_Set**_stmttmp4=({({struct Cyc_Set_Set**(*_tmp9F)(struct Cyc_Dict_Dict d,void*k)=({struct Cyc_Set_Set**(*_tmp3E)(struct Cyc_Dict_Dict d,void*k)=(struct Cyc_Set_Set**(*)(struct Cyc_Dict_Dict d,void*k))Cyc_Dict_lookup_opt;_tmp3E;});struct Cyc_Dict_Dict _tmp9E=g;_tmp9F(_tmp9E,source);});});struct Cyc_Set_Set**_tmp3C=_stmttmp4;struct Cyc_Set_Set*_tmp3D;if(_tmp3C == 0){_LL1: _LL2:
 return({struct _tuple1 _tmp8C;({struct Cyc_Set_Set*_tmpA0=({Cyc_Set_empty(g.rel);});_tmp8C.f1=_tmpA0;}),_tmp8C.f2=g;_tmp8C;});}else{_LL3: _tmp3D=*_tmp3C;_LL4: {struct Cyc_Set_Set*sourceTargets=_tmp3D;
return({struct _tuple1 _tmp8D;_tmp8D.f1=sourceTargets,({struct Cyc_Dict_Dict _tmpA1=({Cyc_Dict_delete(g,source);});_tmp8D.f2=_tmpA1;});_tmp8D;});}}_LL0:;}
# 169
static struct Cyc_Dict_Dict Cyc_Graph_add_edgeTc(struct Cyc_Dict_Dict g,void*source,void*target){
struct Cyc_Set_Set*targetTargets=({Cyc_Graph_get_targets(g,target);});
struct _tuple1 _stmttmp5=({Cyc_Graph_divideGraph(g,source);});struct Cyc_Dict_Dict _tmp41;struct Cyc_Set_Set*_tmp40;_LL1: _tmp40=_stmttmp5.f1;_tmp41=_stmttmp5.f2;_LL2: {struct Cyc_Set_Set*sourceTargets=_tmp40;struct Cyc_Dict_Dict graphWithoutSource=_tmp41;
struct Cyc_List_List*sourceSources=({Cyc_Graph_sourcesOf(graphWithoutSource,source);});
struct Cyc_Set_Set*newSourceTargets=({struct Cyc_Set_Set*(*_tmp43)(struct Cyc_Set_Set*s,void*elt)=Cyc_Set_insert;struct Cyc_Set_Set*_tmp44=({Cyc_Set_union_two(sourceTargets,targetTargets);});void*_tmp45=target;_tmp43(_tmp44,_tmp45);});
struct Cyc_Dict_Dict result=({({struct Cyc_Dict_Dict(*_tmpA4)(struct Cyc_Dict_Dict d,void*k,struct Cyc_Set_Set*v)=({struct Cyc_Dict_Dict(*_tmp42)(struct Cyc_Dict_Dict d,void*k,struct Cyc_Set_Set*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,void*k,struct Cyc_Set_Set*v))Cyc_Dict_insert;_tmp42;});struct Cyc_Dict_Dict _tmpA3=graphWithoutSource;void*_tmpA2=source;_tmpA4(_tmpA3,_tmpA2,newSourceTargets);});});
{struct Cyc_List_List*s=sourceSources;for(0;(unsigned)s;s=s->tl){
result=({Cyc_Graph_add_edges(result,s->hd,newSourceTargets);});}}
# 178
return result;}}
# 180
struct Cyc_Dict_Dict Cyc_Graph_tc_slow(struct Cyc_Dict_Dict g){
struct Cyc_Dict_Dict result=({Cyc_Graph_empty(g.rel);});
struct Cyc_List_List*edges=({Cyc_Dict_to_list(g);});
for(0;(unsigned)edges;edges=edges->tl){
struct _tuple0*_stmttmp6=(struct _tuple0*)edges->hd;struct Cyc_Set_Set*_tmp48;void*_tmp47;_LL1: _tmp47=_stmttmp6->f1;_tmp48=_stmttmp6->f2;_LL2: {void*source=_tmp47;struct Cyc_Set_Set*targets=_tmp48;
struct Cyc_List_List*tslist=({Cyc_Graph_to_list(targets);});
if((unsigned)tslist){
struct Cyc_List_List*t=tslist;for(0;(unsigned)t;t=t->tl){
result=({Cyc_Graph_add_edgeTc(result,source,t->hd);});}}else{
result=({Cyc_Graph_add_node(result,source);});}}}
# 191
return result;}
# 198
struct Cyc_Dict_Dict Cyc_Graph_tkernel(struct Cyc_Dict_Dict g){
int(*cmp)(void*,void*)=g.rel;
struct Cyc_Dict_Dict closure=({Cyc_Graph_empty(cmp);});
struct Cyc_Dict_Dict result=({Cyc_Graph_empty(cmp);});
{struct Cyc_List_List*edges=({Cyc_Dict_to_list(g);});for(0;(unsigned)edges;edges=edges->tl){
struct _tuple0*_stmttmp7=(struct _tuple0*)edges->hd;struct Cyc_Set_Set*_tmp4B;void*_tmp4A;_LL1: _tmp4A=_stmttmp7->f1;_tmp4B=_stmttmp7->f2;_LL2: {void*source=_tmp4A;struct Cyc_Set_Set*targets=_tmp4B;
while(!({Cyc_Set_is_empty(targets);})){
void*target=({Cyc_Set_choose(targets);});
targets=({Cyc_Set_delete(targets,target);});
if(({Cyc_Graph_is_edge(closure,source,target);}))continue;closure=({Cyc_Graph_add_edgeTc(closure,source,target);});
# 209
result=({Cyc_Graph_add_edge(result,source,target);});}}}}
# 212
return result;}struct Cyc_Graph_nodestate{void*root;int C;int visitindex;};struct Cyc_Graph_componentstate{struct Cyc_Set_Set*Succ;struct Cyc_Set_Set*nodes;};struct Cyc_Graph_tcstate{struct Cyc_Set_Set*visited;int visitindex;struct Cyc_Dict_Dict ns;struct Cyc_Dict_Dict cs;int Cindex;struct Cyc_List_List*nstack;struct Cyc_List_List*cstack;};
# 239
static void Cyc_Graph_comp_tc(struct Cyc_Graph_tcstate*ts,struct Cyc_Dict_Dict g,void*v){
int(*cmp)(void*,void*)=g.rel;
({struct Cyc_Set_Set*_tmpA5=({Cyc_Set_insert(ts->visited,v);});ts->visited=_tmpA5;});{
struct Cyc_Graph_nodestate*nsv=({struct Cyc_Graph_nodestate*_tmp61=_cycalloc(sizeof(*_tmp61));_tmp61->root=v,_tmp61->C=0,_tmp61->visitindex=ts->visitindex ++;_tmp61;});
({struct Cyc_Dict_Dict _tmpA9=({({struct Cyc_Dict_Dict(*_tmpA8)(struct Cyc_Dict_Dict d,void*k,struct Cyc_Graph_nodestate*v)=({struct Cyc_Dict_Dict(*_tmp4D)(struct Cyc_Dict_Dict d,void*k,struct Cyc_Graph_nodestate*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,void*k,struct Cyc_Graph_nodestate*v))Cyc_Dict_insert;_tmp4D;});struct Cyc_Dict_Dict _tmpA7=ts->ns;void*_tmpA6=v;_tmpA8(_tmpA7,_tmpA6,nsv);});});ts->ns=_tmpA9;});
({struct Cyc_List_List*_tmpAA=({struct Cyc_List_List*_tmp4E=_cycalloc(sizeof(*_tmp4E));_tmp4E->hd=v,_tmp4E->tl=ts->nstack;_tmp4E;});ts->nstack=_tmpAA;});{
int hsaved=({Cyc_List_length(ts->cstack);});
struct Cyc_Set_Set*targets=({Cyc_Graph_get_targets(g,v);});
while(!({Cyc_Set_is_empty(targets);})){
void*w=({Cyc_Set_choose(targets);});
targets=({Cyc_Set_delete(targets,w);});
if(({cmp(v,w);})== 0)continue;{int is_forward_edge;
# 252
struct Cyc_Graph_nodestate*nsw;
if(({Cyc_Set_member(ts->visited,w);})){
nsw=({({struct Cyc_Graph_nodestate*(*_tmpAC)(struct Cyc_Dict_Dict d,void*k)=({struct Cyc_Graph_nodestate*(*_tmp4F)(struct Cyc_Dict_Dict d,void*k)=(struct Cyc_Graph_nodestate*(*)(struct Cyc_Dict_Dict d,void*k))Cyc_Dict_lookup;_tmp4F;});struct Cyc_Dict_Dict _tmpAB=ts->ns;_tmpAC(_tmpAB,w);});});
is_forward_edge=nsw->visitindex < nsv->visitindex?0: 1;}else{
# 258
is_forward_edge=0;
({Cyc_Graph_comp_tc(ts,g,w);});
nsw=({({struct Cyc_Graph_nodestate*(*_tmpAE)(struct Cyc_Dict_Dict d,void*k)=({struct Cyc_Graph_nodestate*(*_tmp50)(struct Cyc_Dict_Dict d,void*k)=(struct Cyc_Graph_nodestate*(*)(struct Cyc_Dict_Dict d,void*k))Cyc_Dict_lookup;_tmp50;});struct Cyc_Dict_Dict _tmpAD=ts->ns;_tmpAE(_tmpAD,w);});});}{
# 262
int Cw=nsw->C;
if(Cw == 0){
struct Cyc_Graph_nodestate*nsrootv=({({struct Cyc_Graph_nodestate*(*_tmpB0)(struct Cyc_Dict_Dict d,void*k)=({struct Cyc_Graph_nodestate*(*_tmp52)(struct Cyc_Dict_Dict d,void*k)=(struct Cyc_Graph_nodestate*(*)(struct Cyc_Dict_Dict d,void*k))Cyc_Dict_lookup;_tmp52;});struct Cyc_Dict_Dict _tmpAF=ts->ns;_tmpB0(_tmpAF,nsv->root);});});
struct Cyc_Graph_nodestate*nsrootw=({({struct Cyc_Graph_nodestate*(*_tmpB2)(struct Cyc_Dict_Dict d,void*k)=({struct Cyc_Graph_nodestate*(*_tmp51)(struct Cyc_Dict_Dict d,void*k)=(struct Cyc_Graph_nodestate*(*)(struct Cyc_Dict_Dict d,void*k))Cyc_Dict_lookup;_tmp51;});struct Cyc_Dict_Dict _tmpB1=ts->ns;_tmpB2(_tmpB1,nsw->root);});});
# 267
if(nsrootv->visitindex > nsrootw->visitindex)
nsv->root=nsw->root;}else{
# 271
if(!is_forward_edge)
({struct Cyc_List_List*_tmpB3=({struct Cyc_List_List*_tmp53=_cycalloc(sizeof(*_tmp53));_tmp53->hd=(void*)Cw,_tmp53->tl=ts->cstack;_tmp53;});ts->cstack=_tmpB3;});}}}}
# 274
if(({cmp(nsv->root,v);})!= 0)
return;{
# 274
int Cnew=ts->Cindex ++;
# 278
struct Cyc_Graph_componentstate*csCnew=({struct Cyc_Graph_componentstate*_tmp60=_cycalloc(sizeof(*_tmp60));({struct Cyc_Set_Set*_tmpB5=({({struct Cyc_Set_Set*(*_tmp5F)(int(*cmp)(int,int))=(struct Cyc_Set_Set*(*)(int(*cmp)(int,int)))Cyc_Set_empty;_tmp5F;})(Cyc_Core_intcmp);});_tmp60->Succ=_tmpB5;}),({
struct Cyc_Set_Set*_tmpB4=({Cyc_Set_empty(cmp);});_tmp60->nodes=_tmpB4;});_tmp60;});
({struct Cyc_Dict_Dict _tmpB9=({({struct Cyc_Dict_Dict(*_tmpB8)(struct Cyc_Dict_Dict d,int k,struct Cyc_Graph_componentstate*v)=({struct Cyc_Dict_Dict(*_tmp54)(struct Cyc_Dict_Dict d,int k,struct Cyc_Graph_componentstate*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,int k,struct Cyc_Graph_componentstate*v))Cyc_Dict_insert;_tmp54;});struct Cyc_Dict_Dict _tmpB7=ts->cs;int _tmpB6=Cnew;_tmpB8(_tmpB7,_tmpB6,csCnew);});});ts->cs=_tmpB9;});
{struct Cyc_List_List*_stmttmp8=ts->nstack;struct Cyc_List_List*_tmp55=_stmttmp8;void*_tmp56;if(_tmp55 == 0){_LL1: _LL2:
 goto _LL0;}else{_LL3: _tmp56=(void*)_tmp55->hd;_LL4: {void*top=_tmp56;
# 284
if(({cmp(top,v);})!= 0 ||({Cyc_Graph_is_edge(g,v,v);}))
# 286
({struct Cyc_Set_Set*_tmpBC=({({struct Cyc_Set_Set*(*_tmpBB)(struct Cyc_Set_Set*s,int elt)=({struct Cyc_Set_Set*(*_tmp57)(struct Cyc_Set_Set*s,int elt)=(struct Cyc_Set_Set*(*)(struct Cyc_Set_Set*s,int elt))Cyc_Set_insert;_tmp57;});struct Cyc_Set_Set*_tmpBA=csCnew->Succ;_tmpBB(_tmpBA,Cnew);});});csCnew->Succ=_tmpBC;});
# 284
goto _LL0;}}_LL0:;}
# 290
{int hnow=({Cyc_List_length(ts->cstack);});for(0;hnow > hsaved;-- hnow){
int X=(int)((struct Cyc_List_List*)_check_null(ts->cstack))->hd;
ts->cstack=((struct Cyc_List_List*)_check_null(ts->cstack))->tl;
if(!({({int(*_tmpBE)(struct Cyc_Set_Set*s,int elt)=({int(*_tmp58)(struct Cyc_Set_Set*s,int elt)=(int(*)(struct Cyc_Set_Set*s,int elt))Cyc_Set_member;_tmp58;});struct Cyc_Set_Set*_tmpBD=csCnew->Succ;_tmpBE(_tmpBD,X);});})){
struct Cyc_Set_Set*s=({({struct Cyc_Set_Set*(*_tmpC0)(struct Cyc_Set_Set*s,int elt)=({struct Cyc_Set_Set*(*_tmp5D)(struct Cyc_Set_Set*s,int elt)=(struct Cyc_Set_Set*(*)(struct Cyc_Set_Set*s,int elt))Cyc_Set_insert;_tmp5D;});struct Cyc_Set_Set*_tmpBF=csCnew->Succ;_tmpC0(_tmpBF,X);});});
s=({struct Cyc_Set_Set*(*_tmp59)(struct Cyc_Set_Set*s1,struct Cyc_Set_Set*s2)=Cyc_Set_union_two;struct Cyc_Set_Set*_tmp5A=s;struct Cyc_Set_Set*_tmp5B=({({struct Cyc_Graph_componentstate*(*_tmpC2)(struct Cyc_Dict_Dict d,int k)=({struct Cyc_Graph_componentstate*(*_tmp5C)(struct Cyc_Dict_Dict d,int k)=(struct Cyc_Graph_componentstate*(*)(struct Cyc_Dict_Dict d,int k))Cyc_Dict_lookup;_tmp5C;});struct Cyc_Dict_Dict _tmpC1=ts->cs;_tmpC2(_tmpC1,X);});})->Succ;_tmp59(_tmp5A,_tmp5B);});
csCnew->Succ=s;}}}
# 299
while((unsigned)ts->nstack){
void*w=((struct Cyc_List_List*)_check_null(ts->nstack))->hd;
ts->nstack=((struct Cyc_List_List*)_check_null(ts->nstack))->tl;{
struct Cyc_Graph_nodestate*nsw=({({struct Cyc_Graph_nodestate*(*_tmpC4)(struct Cyc_Dict_Dict d,void*k)=({struct Cyc_Graph_nodestate*(*_tmp5E)(struct Cyc_Dict_Dict d,void*k)=(struct Cyc_Graph_nodestate*(*)(struct Cyc_Dict_Dict d,void*k))Cyc_Dict_lookup;_tmp5E;});struct Cyc_Dict_Dict _tmpC3=ts->ns;_tmpC4(_tmpC3,w);});});
nsw->C=Cnew;
({struct Cyc_Set_Set*_tmpC5=({Cyc_Set_insert(csCnew->nodes,w);});csCnew->nodes=_tmpC5;});
if(({cmp(w,v);})== 0)break;}}}}}}
# 308
struct Cyc_Dict_Dict Cyc_Graph_tc(struct Cyc_Dict_Dict g){
int(*cmp)(void*,void*)=g.rel;
struct Cyc_Graph_tcstate*ts=({struct Cyc_Graph_tcstate*_tmp6D=_cycalloc(sizeof(*_tmp6D));
({
struct Cyc_Set_Set*_tmpC8=({Cyc_Set_empty(cmp);});_tmp6D->visited=_tmpC8;}),_tmp6D->visitindex=1,({
# 314
struct Cyc_Dict_Dict _tmpC7=({Cyc_Dict_empty(cmp);});_tmp6D->ns=_tmpC7;}),({
struct Cyc_Dict_Dict _tmpC6=({({struct Cyc_Dict_Dict(*_tmp6C)(int(*cmp)(int,int))=(struct Cyc_Dict_Dict(*)(int(*cmp)(int,int)))Cyc_Dict_empty;_tmp6C;})(Cyc_Core_intcmp);});_tmp6D->cs=_tmpC6;}),_tmp6D->Cindex=1,_tmp6D->nstack=0,_tmp6D->cstack=0;_tmp6D;});
# 320
{struct Cyc_List_List*edges=({Cyc_Dict_to_list(g);});for(0;(unsigned)edges;edges=edges->tl){
struct _tuple0*_stmttmp9=(struct _tuple0*)edges->hd;struct Cyc_Set_Set*_tmp64;void*_tmp63;_LL1: _tmp63=_stmttmp9->f1;_tmp64=_stmttmp9->f2;_LL2: {void*source=_tmp63;struct Cyc_Set_Set*targets=_tmp64;
if(!({Cyc_Set_member(ts->visited,source);}))({Cyc_Graph_comp_tc(ts,g,source);});}}}{
# 325
struct Cyc_Dict_Dict result=({Cyc_Graph_empty(cmp);});
{int C=1;for(0;C < ts->Cindex;++ C){
struct Cyc_Graph_componentstate*cs=({({struct Cyc_Graph_componentstate*(*_tmpCA)(struct Cyc_Dict_Dict d,int k)=({struct Cyc_Graph_componentstate*(*_tmp6B)(struct Cyc_Dict_Dict d,int k)=(struct Cyc_Graph_componentstate*(*)(struct Cyc_Dict_Dict d,int k))Cyc_Dict_lookup;_tmp6B;});struct Cyc_Dict_Dict _tmpC9=ts->cs;_tmpCA(_tmpC9,C);});});
struct Cyc_Set_Set*targets=({Cyc_Set_empty(cmp);});
struct Cyc_Set_Set*Succ=cs->Succ;
while(!({Cyc_Set_is_empty(Succ);})){
int C2=({({int(*_tmpCB)(struct Cyc_Set_Set*s)=({int(*_tmp6A)(struct Cyc_Set_Set*s)=(int(*)(struct Cyc_Set_Set*s))Cyc_Set_choose;_tmp6A;});_tmpCB(Succ);});});
Succ=({({struct Cyc_Set_Set*(*_tmpCD)(struct Cyc_Set_Set*s,int elt)=({struct Cyc_Set_Set*(*_tmp65)(struct Cyc_Set_Set*s,int elt)=(struct Cyc_Set_Set*(*)(struct Cyc_Set_Set*s,int elt))Cyc_Set_delete;_tmp65;});struct Cyc_Set_Set*_tmpCC=Succ;_tmpCD(_tmpCC,C2);});});
targets=({struct Cyc_Set_Set*(*_tmp66)(struct Cyc_Set_Set*s1,struct Cyc_Set_Set*s2)=Cyc_Set_union_two;struct Cyc_Set_Set*_tmp67=targets;struct Cyc_Set_Set*_tmp68=({({struct Cyc_Graph_componentstate*(*_tmpCF)(struct Cyc_Dict_Dict d,int k)=({
struct Cyc_Graph_componentstate*(*_tmp69)(struct Cyc_Dict_Dict d,int k)=(struct Cyc_Graph_componentstate*(*)(struct Cyc_Dict_Dict d,int k))Cyc_Dict_lookup;_tmp69;});struct Cyc_Dict_Dict _tmpCE=ts->cs;_tmpCF(_tmpCE,C2);});})->nodes;_tmp66(_tmp67,_tmp68);});}{
# 336
struct Cyc_Set_Set*nodes=cs->nodes;
while(!({Cyc_Set_is_empty(nodes);})){
void*v=({Cyc_Set_choose(nodes);});
nodes=({Cyc_Set_delete(nodes,v);});
result=({Cyc_Graph_add_edges(result,v,targets);});}}}}
# 343
return result;}}
# 357 "graph.cyc"
static void Cyc_Graph_traverse(struct Cyc_Dict_Dict input,struct Cyc_Dict_Dict*output,struct Cyc_Dict_Dict*N,struct Cyc_List_List**S,void*x,unsigned k){
# 360
({struct Cyc_List_List*_tmpD0=({struct Cyc_List_List*_tmp6F=_cycalloc(sizeof(*_tmp6F));_tmp6F->hd=x,_tmp6F->tl=*S;_tmp6F;});*S=_tmpD0;});
({struct Cyc_Dict_Dict _tmpD4=({({struct Cyc_Dict_Dict(*_tmpD3)(struct Cyc_Dict_Dict d,void*k,unsigned v)=({struct Cyc_Dict_Dict(*_tmp70)(struct Cyc_Dict_Dict d,void*k,unsigned v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,void*k,unsigned v))Cyc_Dict_insert;_tmp70;});struct Cyc_Dict_Dict _tmpD2=*N;void*_tmpD1=x;_tmpD3(_tmpD2,_tmpD1,k);});});*N=_tmpD4;});
{struct Cyc_Set_Set*targets=({Cyc_Graph_get_targets(input,x);});for(0;!({Cyc_Set_is_empty(targets);});0){
void*y=({Cyc_Set_choose(targets);});
targets=({Cyc_Set_delete(targets,y);});
if(({({unsigned(*_tmpD6)(struct Cyc_Dict_Dict d,void*k)=({unsigned(*_tmp71)(struct Cyc_Dict_Dict d,void*k)=(unsigned(*)(struct Cyc_Dict_Dict d,void*k))Cyc_Dict_lookup;_tmp71;});struct Cyc_Dict_Dict _tmpD5=*N;_tmpD6(_tmpD5,y);});})== (unsigned)0)
({Cyc_Graph_traverse(input,output,N,S,y,k + (unsigned)1);});{
# 365
unsigned Ny=({({unsigned(*_tmpD8)(struct Cyc_Dict_Dict d,void*k)=({
# 367
unsigned(*_tmp74)(struct Cyc_Dict_Dict d,void*k)=(unsigned(*)(struct Cyc_Dict_Dict d,void*k))Cyc_Dict_lookup;_tmp74;});struct Cyc_Dict_Dict _tmpD7=*N;_tmpD8(_tmpD7,y);});});
if(({unsigned _tmpDB=Ny;_tmpDB < ({({unsigned(*_tmpDA)(struct Cyc_Dict_Dict d,void*k)=({unsigned(*_tmp72)(struct Cyc_Dict_Dict d,void*k)=(unsigned(*)(struct Cyc_Dict_Dict d,void*k))Cyc_Dict_lookup;_tmp72;});struct Cyc_Dict_Dict _tmpD9=*N;_tmpDA(_tmpD9,x);});});}))
({struct Cyc_Dict_Dict _tmpDF=({({struct Cyc_Dict_Dict(*_tmpDE)(struct Cyc_Dict_Dict d,void*k,unsigned v)=({struct Cyc_Dict_Dict(*_tmp73)(struct Cyc_Dict_Dict d,void*k,unsigned v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,void*k,unsigned v))Cyc_Dict_insert;_tmp73;});struct Cyc_Dict_Dict _tmpDD=*N;void*_tmpDC=x;_tmpDE(_tmpDD,_tmpDC,Ny);});});*N=_tmpDF;});}}}
# 371
if(({unsigned _tmpE2=({({unsigned(*_tmpE1)(struct Cyc_Dict_Dict d,void*k)=({unsigned(*_tmp75)(struct Cyc_Dict_Dict d,void*k)=(unsigned(*)(struct Cyc_Dict_Dict d,void*k))Cyc_Dict_lookup;_tmp75;});struct Cyc_Dict_Dict _tmpE0=*N;_tmpE1(_tmpE0,x);});});_tmpE2 == k;})){
int(*cmp)(void*,void*)=(*output).rel;
struct Cyc_Set_Set*component=({Cyc_Set_empty(cmp);});
{struct Cyc_List_List*s=*S;for(0;(unsigned)s;s=s->tl){
void*top=s->hd;
component=({Cyc_Set_insert(component,top);});
if(({cmp(top,x);})== 0)break;}}
# 379
for(0;(unsigned)*S;*S=((struct Cyc_List_List*)_check_null(*S))->tl){
void*top=((struct Cyc_List_List*)_check_null(*S))->hd;
({struct Cyc_Dict_Dict _tmpE5=({({struct Cyc_Dict_Dict(*_tmpE4)(struct Cyc_Dict_Dict d,void*k,unsigned v)=({struct Cyc_Dict_Dict(*_tmp76)(struct Cyc_Dict_Dict d,void*k,unsigned v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,void*k,unsigned v))Cyc_Dict_insert;_tmp76;});struct Cyc_Dict_Dict _tmpE3=*N;_tmpE4(_tmpE3,top,- 1U);});});*N=_tmpE5;});
({struct Cyc_Dict_Dict _tmpE6=({Cyc_Graph_add_node(*output,top);});*output=_tmpE6;});
({struct Cyc_Dict_Dict _tmpE7=({Cyc_Graph_add_edges(*output,top,component);});*output=_tmpE7;});
if(({cmp(top,x);})== 0){
*S=((struct Cyc_List_List*)_check_null(*S))->tl;
break;}}}}
# 391
struct Cyc_Dict_Dict Cyc_Graph_scc(struct Cyc_Dict_Dict input){
int(*cmp)(void*,void*)=input.rel;
struct Cyc_List_List*edges=({Cyc_Dict_to_list(input);});
struct Cyc_Dict_Dict output=({Cyc_Graph_empty(cmp);});
struct Cyc_List_List*S=0;
struct Cyc_Dict_Dict N=({Cyc_Dict_empty(cmp);});
{struct Cyc_List_List*e=edges;for(0;(unsigned)e;e=e->tl){
struct _tuple0*_stmttmpA=(struct _tuple0*)e->hd;void*_tmp78;_LL1: _tmp78=_stmttmpA->f1;_LL2: {void*x=_tmp78;
N=({({struct Cyc_Dict_Dict(*_tmpE9)(struct Cyc_Dict_Dict d,void*k,unsigned v)=({struct Cyc_Dict_Dict(*_tmp79)(struct Cyc_Dict_Dict d,void*k,unsigned v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,void*k,unsigned v))Cyc_Dict_insert;_tmp79;});struct Cyc_Dict_Dict _tmpE8=N;_tmpE9(_tmpE8,x,0U);});});}}}
# 401
{struct Cyc_List_List*e=edges;for(0;(unsigned)e;e=e->tl){
struct _tuple0*_stmttmpB=(struct _tuple0*)e->hd;void*_tmp7A;_LL4: _tmp7A=_stmttmpB->f1;_LL5: {void*x=_tmp7A;
if(({({unsigned(*_tmpEB)(struct Cyc_Dict_Dict d,void*k)=({unsigned(*_tmp7B)(struct Cyc_Dict_Dict d,void*k)=(unsigned(*)(struct Cyc_Dict_Dict d,void*k))Cyc_Dict_lookup;_tmp7B;});struct Cyc_Dict_Dict _tmpEA=N;_tmpEB(_tmpEA,x);});})== (unsigned)0)
({Cyc_Graph_traverse(input,& output,& N,& S,x,1U);});}}}
# 406
return output;}
# 411
static void Cyc_Graph_tsort0(struct Cyc_Dict_Dict input,struct Cyc_List_List**output,struct Cyc_Dict_Dict*N,struct Cyc_List_List**S,void*x,unsigned k){
# 414
({struct Cyc_List_List*_tmpEC=({struct Cyc_List_List*_tmp7D=_cycalloc(sizeof(*_tmp7D));_tmp7D->hd=x,_tmp7D->tl=*S;_tmp7D;});*S=_tmpEC;});
({struct Cyc_Dict_Dict _tmpF0=({({struct Cyc_Dict_Dict(*_tmpEF)(struct Cyc_Dict_Dict d,void*k,unsigned v)=({struct Cyc_Dict_Dict(*_tmp7E)(struct Cyc_Dict_Dict d,void*k,unsigned v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,void*k,unsigned v))Cyc_Dict_insert;_tmp7E;});struct Cyc_Dict_Dict _tmpEE=*N;void*_tmpED=x;_tmpEF(_tmpEE,_tmpED,k);});});*N=_tmpF0;});
{struct Cyc_Set_Set*targets=({Cyc_Graph_get_targets(input,x);});for(0;!({Cyc_Set_is_empty(targets);});0){
void*y=({Cyc_Set_choose(targets);});
targets=({Cyc_Set_delete(targets,y);});
if(({({unsigned(*_tmpF2)(struct Cyc_Dict_Dict d,void*k)=({unsigned(*_tmp7F)(struct Cyc_Dict_Dict d,void*k)=(unsigned(*)(struct Cyc_Dict_Dict d,void*k))Cyc_Dict_lookup;_tmp7F;});struct Cyc_Dict_Dict _tmpF1=*N;_tmpF2(_tmpF1,y);});})== (unsigned)0)
({Cyc_Graph_tsort0(input,output,N,S,y,k + (unsigned)1);});{
# 419
unsigned Ny=({({unsigned(*_tmpF4)(struct Cyc_Dict_Dict d,void*k)=({
# 421
unsigned(*_tmp82)(struct Cyc_Dict_Dict d,void*k)=(unsigned(*)(struct Cyc_Dict_Dict d,void*k))Cyc_Dict_lookup;_tmp82;});struct Cyc_Dict_Dict _tmpF3=*N;_tmpF4(_tmpF3,y);});});
if(({unsigned _tmpF7=Ny;_tmpF7 < ({({unsigned(*_tmpF6)(struct Cyc_Dict_Dict d,void*k)=({unsigned(*_tmp80)(struct Cyc_Dict_Dict d,void*k)=(unsigned(*)(struct Cyc_Dict_Dict d,void*k))Cyc_Dict_lookup;_tmp80;});struct Cyc_Dict_Dict _tmpF5=*N;_tmpF6(_tmpF5,x);});});}))
({struct Cyc_Dict_Dict _tmpFB=({({struct Cyc_Dict_Dict(*_tmpFA)(struct Cyc_Dict_Dict d,void*k,unsigned v)=({struct Cyc_Dict_Dict(*_tmp81)(struct Cyc_Dict_Dict d,void*k,unsigned v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,void*k,unsigned v))Cyc_Dict_insert;_tmp81;});struct Cyc_Dict_Dict _tmpF9=*N;void*_tmpF8=x;_tmpFA(_tmpF9,_tmpF8,Ny);});});*N=_tmpFB;});}}}
# 425
if(({unsigned _tmpFE=({({unsigned(*_tmpFD)(struct Cyc_Dict_Dict d,void*k)=({unsigned(*_tmp83)(struct Cyc_Dict_Dict d,void*k)=(unsigned(*)(struct Cyc_Dict_Dict d,void*k))Cyc_Dict_lookup;_tmp83;});struct Cyc_Dict_Dict _tmpFC=*N;_tmpFD(_tmpFC,x);});});_tmpFE == k;})){
int(*cmp)(void*,void*)=input.rel;
struct Cyc_Set_Set*component=({Cyc_Set_empty(cmp);});
{struct Cyc_List_List*s=*S;for(0;(unsigned)s;s=s->tl){
void*top=s->hd;
component=({Cyc_Set_insert(component,top);});
if(({cmp(top,x);})== 0)break;}}
# 433
for(0;(unsigned)*S;*S=((struct Cyc_List_List*)_check_null(*S))->tl){
void*top=((struct Cyc_List_List*)_check_null(*S))->hd;
({struct Cyc_Dict_Dict _tmp101=({({struct Cyc_Dict_Dict(*_tmp100)(struct Cyc_Dict_Dict d,void*k,unsigned v)=({struct Cyc_Dict_Dict(*_tmp84)(struct Cyc_Dict_Dict d,void*k,unsigned v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,void*k,unsigned v))Cyc_Dict_insert;_tmp84;});struct Cyc_Dict_Dict _tmpFF=*N;_tmp100(_tmpFF,top,- 1U);});});*N=_tmp101;});
({struct Cyc_List_List*_tmp102=({struct Cyc_List_List*_tmp85=_cycalloc(sizeof(*_tmp85));_tmp85->hd=top,_tmp85->tl=*output;_tmp85;});*output=_tmp102;});
if(({cmp(top,x);})== 0){
*S=((struct Cyc_List_List*)_check_null(*S))->tl;
break;}}}}
# 444
struct Cyc_List_List*Cyc_Graph_tsort(struct Cyc_Dict_Dict input){
int(*cmp)(void*,void*)=input.rel;
struct Cyc_List_List*edges=({Cyc_Dict_to_list(input);});
struct Cyc_List_List*output=0;
struct Cyc_List_List*S=0;
struct Cyc_Dict_Dict N=({Cyc_Dict_empty(cmp);});
{struct Cyc_List_List*e=edges;for(0;(unsigned)e;e=e->tl){
struct _tuple0*_stmttmpC=(struct _tuple0*)e->hd;void*_tmp87;_LL1: _tmp87=_stmttmpC->f1;_LL2: {void*x=_tmp87;
N=({({struct Cyc_Dict_Dict(*_tmp104)(struct Cyc_Dict_Dict d,void*k,unsigned v)=({struct Cyc_Dict_Dict(*_tmp88)(struct Cyc_Dict_Dict d,void*k,unsigned v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,void*k,unsigned v))Cyc_Dict_insert;_tmp88;});struct Cyc_Dict_Dict _tmp103=N;_tmp104(_tmp103,x,0U);});});}}}
# 454
{struct Cyc_List_List*e=edges;for(0;(unsigned)e;e=e->tl){
struct _tuple0*_stmttmpD=(struct _tuple0*)e->hd;void*_tmp89;_LL4: _tmp89=_stmttmpD->f1;_LL5: {void*x=_tmp89;
if(({({unsigned(*_tmp106)(struct Cyc_Dict_Dict d,void*k)=({unsigned(*_tmp8A)(struct Cyc_Dict_Dict d,void*k)=(unsigned(*)(struct Cyc_Dict_Dict d,void*k))Cyc_Dict_lookup;_tmp8A;});struct Cyc_Dict_Dict _tmp105=N;_tmp106(_tmp105,x);});})== (unsigned)0)
({Cyc_Graph_tsort0(input,& output,& N,& S,x,1U);});}}}
# 459
return output;}

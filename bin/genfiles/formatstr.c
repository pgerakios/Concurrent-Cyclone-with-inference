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
# 178 "list.h"
struct Cyc_List_List*Cyc_List_imp_rev(struct Cyc_List_List*x);extern char Cyc_List_Nth[4U];struct Cyc_List_Nth_exn_struct{char*tag;};struct Cyc___cycFILE;struct Cyc_String_pa_PrintArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Int_pa_PrintArg_struct{int tag;unsigned long f1;};struct Cyc_Double_pa_PrintArg_struct{int tag;double f1;};struct Cyc_LongDouble_pa_PrintArg_struct{int tag;long double f1;};struct Cyc_ShortPtr_pa_PrintArg_struct{int tag;short*f1;};struct Cyc_IntPtr_pa_PrintArg_struct{int tag;unsigned long*f1;};struct Cyc_ShortPtr_sa_ScanfArg_struct{int tag;short*f1;};struct Cyc_UShortPtr_sa_ScanfArg_struct{int tag;unsigned short*f1;};struct Cyc_IntPtr_sa_ScanfArg_struct{int tag;int*f1;};struct Cyc_UIntPtr_sa_ScanfArg_struct{int tag;unsigned*f1;};struct Cyc_StringPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_DoublePtr_sa_ScanfArg_struct{int tag;double*f1;};struct Cyc_FloatPtr_sa_ScanfArg_struct{int tag;float*f1;};struct Cyc_CharPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};extern char Cyc_FileCloseError[15U];struct Cyc_FileCloseError_exn_struct{char*tag;};extern char Cyc_FileOpenError[14U];struct Cyc_FileOpenError_exn_struct{char*tag;struct _fat_ptr f1;};
# 290 "cycboot.h"
int isdigit(int);
# 38 "string.h"
unsigned long Cyc_strlen(struct _fat_ptr s);
# 136 "string.h"
struct _fat_ptr Cyc_implode(struct Cyc_List_List*c);struct Cyc_Position_Error;struct Cyc_Iter_Iter{void*env;int(*next)(void*env,void*dest);};struct Cyc_Dict_T;struct Cyc_Dict_Dict{int(*rel)(void*,void*);struct _RegionHandle*r;const struct Cyc_Dict_T*t;};extern char Cyc_Dict_Present[8U];struct Cyc_Dict_Present_exn_struct{char*tag;};extern char Cyc_Dict_Absent[7U];struct Cyc_Dict_Absent_exn_struct{char*tag;};struct Cyc_Relations_Reln;struct _union_Nmspace_Rel_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Abs_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_C_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Loc_n{int tag;int val;};union Cyc_Absyn_Nmspace{struct _union_Nmspace_Rel_n Rel_n;struct _union_Nmspace_Abs_n Abs_n;struct _union_Nmspace_C_n C_n;struct _union_Nmspace_Loc_n Loc_n;};struct _tuple0{union Cyc_Absyn_Nmspace f1;struct _fat_ptr*f2;};
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
enum Cyc_Absyn_KindQual{Cyc_Absyn_AnyKind =0U,Cyc_Absyn_MemKind =1U,Cyc_Absyn_BoxKind =2U,Cyc_Absyn_XRgnKind =3U,Cyc_Absyn_RgnKind =4U,Cyc_Absyn_EffKind =5U,Cyc_Absyn_IntKind =6U,Cyc_Absyn_BoolKind =7U,Cyc_Absyn_PtrBndKind =8U};struct Cyc_Absyn_Kind{enum Cyc_Absyn_KindQual kind;enum Cyc_Absyn_AliasQual aliasqual;};struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct{int tag;struct Cyc_Absyn_Kind*f1;};struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct{int tag;struct Cyc_Core_Opt*f1;};struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct{int tag;struct Cyc_Core_Opt*f1;struct Cyc_Absyn_Kind*f2;};struct Cyc_Absyn_Tvar{struct _fat_ptr*name;int identity;void*kind;};struct Cyc_Absyn_PtrLoc{unsigned ptr_loc;unsigned rgn_loc;unsigned zt_loc;};struct Cyc_Absyn_PtrAtts{void*rgn;void*nullable;void*bounds;void*zero_term;struct Cyc_Absyn_PtrLoc*ptrloc;};struct Cyc_Absyn_PtrInfo{void*elt_type;struct Cyc_Absyn_Tqual elt_tq;struct Cyc_Absyn_PtrAtts ptr_atts;};struct Cyc_Absyn_VarargInfo{struct _fat_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;int inject;};struct Cyc_Absyn_Star_Absyn_Cap_struct{int tag;};struct Cyc_Absyn_Bot_Absyn_Cap_struct{int tag;};struct Cyc_Absyn_Nat_Absyn_Cap_struct{int tag;int f1;int f2;};struct Cyc_Absyn_RgnEffect{void*name;struct Cyc_List_List*caps;void*parent;};struct Cyc_Absyn_FnInfo{struct Cyc_List_List*tvars;void*effect;struct Cyc_Absyn_Tqual ret_tqual;void*ret_type;struct Cyc_List_List*args;int c_varargs;struct Cyc_Absyn_VarargInfo*cyc_varargs;struct Cyc_List_List*rgn_po;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*requires_clause;struct Cyc_List_List*requires_relns;struct Cyc_Absyn_Exp*ensures_clause;struct Cyc_List_List*ensures_relns;struct Cyc_List_List*ieffect;struct Cyc_List_List*oeffect;struct Cyc_List_List*throws;int reentrant;};struct Cyc_Absyn_UnknownDatatypeInfo{struct _tuple0*name;int is_extensible;};struct _union_DatatypeInfo_UnknownDatatype{int tag;struct Cyc_Absyn_UnknownDatatypeInfo val;};struct _union_DatatypeInfo_KnownDatatype{int tag;struct Cyc_Absyn_Datatypedecl**val;};union Cyc_Absyn_DatatypeInfo{struct _union_DatatypeInfo_UnknownDatatype UnknownDatatype;struct _union_DatatypeInfo_KnownDatatype KnownDatatype;};struct Cyc_Absyn_UnknownDatatypeFieldInfo{struct _tuple0*datatype_name;struct _tuple0*field_name;int is_extensible;};struct _union_DatatypeFieldInfo_UnknownDatatypefield{int tag;struct Cyc_Absyn_UnknownDatatypeFieldInfo val;};struct _tuple1{struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;};struct _union_DatatypeFieldInfo_KnownDatatypefield{int tag;struct _tuple1 val;};union Cyc_Absyn_DatatypeFieldInfo{struct _union_DatatypeFieldInfo_UnknownDatatypefield UnknownDatatypefield;struct _union_DatatypeFieldInfo_KnownDatatypefield KnownDatatypefield;};struct _tuple2{enum Cyc_Absyn_AggrKind f1;struct _tuple0*f2;struct Cyc_Core_Opt*f3;};struct _union_AggrInfo_UnknownAggr{int tag;struct _tuple2 val;};struct _union_AggrInfo_KnownAggr{int tag;struct Cyc_Absyn_Aggrdecl**val;};union Cyc_Absyn_AggrInfo{struct _union_AggrInfo_UnknownAggr UnknownAggr;struct _union_AggrInfo_KnownAggr KnownAggr;};struct Cyc_Absyn_ArrayInfo{void*elt_type;struct Cyc_Absyn_Tqual tq;struct Cyc_Absyn_Exp*num_elts;void*zero_term;unsigned zt_loc;};struct Cyc_Absyn_Aggr_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Enum_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;};struct Cyc_Absyn_Datatype_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;};struct Cyc_Absyn_TypeDecl{void*r;unsigned loc;};struct Cyc_Absyn_VoidCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_IntCon_Absyn_TyCon_struct{int tag;enum Cyc_Absyn_Sign f1;enum Cyc_Absyn_Size_of f2;};struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct{int tag;int f1;};struct Cyc_Absyn_RgnHandleCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_XRgnHandleCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_TagCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_HeapCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_UniqueCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_RefCntCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_AccessCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_CAccessCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_JoinCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_RgnsCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_TrueCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_FalseCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_ThinCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_FatCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct{int tag;struct _tuple0*f1;struct Cyc_Absyn_Enumdecl*f2;};struct Cyc_Absyn_AnonEnumCon_Absyn_TyCon_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_BuiltinCon_Absyn_TyCon_struct{int tag;struct _fat_ptr f1;struct Cyc_Absyn_Kind*f2;};struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_DatatypeInfo f1;};struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_DatatypeFieldInfo f1;};struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_AggrInfo f1;};struct Cyc_Absyn_AppType_Absyn_Type_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Evar_Absyn_Type_struct{int tag;struct Cyc_Core_Opt*f1;void*f2;int f3;struct Cyc_Core_Opt*f4;};struct Cyc_Absyn_VarType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Tvar*f1;};struct Cyc_Absyn_PointerType_Absyn_Type_struct{int tag;struct Cyc_Absyn_PtrInfo f1;};struct Cyc_Absyn_ArrayType_Absyn_Type_struct{int tag;struct Cyc_Absyn_ArrayInfo f1;};struct Cyc_Absyn_FnType_Absyn_Type_struct{int tag;struct Cyc_Absyn_FnInfo f1;};struct Cyc_Absyn_TupleType_Absyn_Type_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct{int tag;enum Cyc_Absyn_AggrKind f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_TypedefType_Absyn_Type_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_Typedefdecl*f3;void*f4;};struct Cyc_Absyn_ValueofType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct{int tag;struct Cyc_Absyn_TypeDecl*f1;void**f2;};struct Cyc_Absyn_TypeofType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_NoTypes_Absyn_Funcparams_struct{int tag;struct Cyc_List_List*f1;unsigned f2;};struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct{int tag;struct Cyc_List_List*f1;int f2;struct Cyc_Absyn_VarargInfo*f3;void*f4;struct Cyc_List_List*f5;struct Cyc_Absyn_Exp*f6;struct Cyc_Absyn_Exp*f7;struct Cyc_List_List*f8;struct Cyc_List_List*f9;struct Cyc_List_List*f10;int f11;};
# 478 "absyn.h"
enum Cyc_Absyn_Format_Type{Cyc_Absyn_Printf_ft =0U,Cyc_Absyn_Scanf_ft =1U};struct Cyc_Absyn_Regparm_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Stdcall_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Cdecl_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Fastcall_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Noreturn_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Const_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Aligned_att_Absyn_Attribute_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Packed_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Section_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Nocommon_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Shared_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Unused_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Weak_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Dllimport_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Dllexport_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_instrument_function_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Constructor_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Destructor_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_check_memory_usage_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Format_att_Absyn_Attribute_struct{int tag;enum Cyc_Absyn_Format_Type f1;int f2;int f3;};struct Cyc_Absyn_Initializes_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Noliveunique_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Consume_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Pure_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Mode_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Alias_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Always_inline_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_throw_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Non_null_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Deprecated_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_VectorSize_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Carray_mod_Absyn_Type_modifier_struct{int tag;void*f1;unsigned f2;};struct Cyc_Absyn_ConstArray_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;unsigned f3;};struct Cyc_Absyn_Pointer_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_Absyn_PtrAtts f1;struct Cyc_Absyn_Tqual f2;};struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct{int tag;void*f1;};struct Cyc_Absyn_TypeParams_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_List_List*f1;unsigned f2;int f3;};struct Cyc_Absyn_Attributes_mod_Absyn_Type_modifier_struct{int tag;unsigned f1;struct Cyc_List_List*f2;};struct _union_Cnst_Null_c{int tag;int val;};struct _tuple3{enum Cyc_Absyn_Sign f1;char f2;};struct _union_Cnst_Char_c{int tag;struct _tuple3 val;};struct _union_Cnst_Wchar_c{int tag;struct _fat_ptr val;};struct _tuple4{enum Cyc_Absyn_Sign f1;short f2;};struct _union_Cnst_Short_c{int tag;struct _tuple4 val;};struct _tuple5{enum Cyc_Absyn_Sign f1;int f2;};struct _union_Cnst_Int_c{int tag;struct _tuple5 val;};struct _tuple6{enum Cyc_Absyn_Sign f1;long long f2;};struct _union_Cnst_LongLong_c{int tag;struct _tuple6 val;};struct _tuple7{struct _fat_ptr f1;int f2;};struct _union_Cnst_Float_c{int tag;struct _tuple7 val;};struct _union_Cnst_String_c{int tag;struct _fat_ptr val;};struct _union_Cnst_Wstring_c{int tag;struct _fat_ptr val;};union Cyc_Absyn_Cnst{struct _union_Cnst_Null_c Null_c;struct _union_Cnst_Char_c Char_c;struct _union_Cnst_Wchar_c Wchar_c;struct _union_Cnst_Short_c Short_c;struct _union_Cnst_Int_c Int_c;struct _union_Cnst_LongLong_c LongLong_c;struct _union_Cnst_Float_c Float_c;struct _union_Cnst_String_c String_c;struct _union_Cnst_Wstring_c Wstring_c;};
# 574
enum Cyc_Absyn_Primop{Cyc_Absyn_Plus =0U,Cyc_Absyn_Times =1U,Cyc_Absyn_Minus =2U,Cyc_Absyn_Div =3U,Cyc_Absyn_Mod =4U,Cyc_Absyn_Eq =5U,Cyc_Absyn_Neq =6U,Cyc_Absyn_Gt =7U,Cyc_Absyn_Lt =8U,Cyc_Absyn_Gte =9U,Cyc_Absyn_Lte =10U,Cyc_Absyn_Not =11U,Cyc_Absyn_Bitnot =12U,Cyc_Absyn_Bitand =13U,Cyc_Absyn_Bitor =14U,Cyc_Absyn_Bitxor =15U,Cyc_Absyn_Bitlshift =16U,Cyc_Absyn_Bitlrshift =17U,Cyc_Absyn_Numelts =18U};
# 581
enum Cyc_Absyn_Incrementor{Cyc_Absyn_PreInc =0U,Cyc_Absyn_PostInc =1U,Cyc_Absyn_PreDec =2U,Cyc_Absyn_PostDec =3U};struct Cyc_Absyn_VarargCallInfo{int num_varargs;struct Cyc_List_List*injectors;struct Cyc_Absyn_VarargInfo*vai;};struct Cyc_Absyn_StructField_Absyn_OffsetofField_struct{int tag;struct _fat_ptr*f1;};struct Cyc_Absyn_TupleIndex_Absyn_OffsetofField_struct{int tag;unsigned f1;};
# 599
enum Cyc_Absyn_Coercion{Cyc_Absyn_Unknown_coercion =0U,Cyc_Absyn_No_coercion =1U,Cyc_Absyn_Null_to_NonNull =2U,Cyc_Absyn_Other_coercion =3U};struct Cyc_Absyn_MallocInfo{int is_calloc;struct Cyc_Absyn_Exp*rgn;void**elt_type;struct Cyc_Absyn_Exp*num_elts;int fat_result;int inline_call;};struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct{int tag;union Cyc_Absyn_Cnst f1;};struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Pragma_e_Absyn_Raw_exp_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct{int tag;enum Cyc_Absyn_Primop f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;enum Cyc_Absyn_Incrementor f2;};struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct _tuple8{struct Cyc_List_List*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_VarargCallInfo*f3;int f4;struct _tuple8*f5;};struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;int f2;};struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Exp*f2;int f3;enum Cyc_Absyn_Coercion f4;};struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;int f3;int f4;};struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;int f3;int f4;};struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Tuple_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;};struct _tuple9{struct _fat_ptr*f1;struct Cyc_Absyn_Tqual f2;void*f3;};struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct{int tag;struct _tuple9*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;int f4;};struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;int f3;};struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct Cyc_Absyn_Aggrdecl*f4;};struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Datatypedecl*f2;struct Cyc_Absyn_Datatypefield*f3;};struct Cyc_Absyn_Enum_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_Spawn_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;struct _tuple8*f3;};struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_MallocInfo f1;};struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Core_Opt*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Stmt*f1;};struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;};struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct{int tag;int f1;struct _fat_ptr f2;struct Cyc_List_List*f3;struct Cyc_List_List*f4;struct Cyc_List_List*f5;};struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Exp{void*topt;void*r;unsigned loc;void*annot;};struct Cyc_Absyn_Skip_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;struct Cyc_Absyn_Stmt*f3;};struct _tuple10{struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct{int tag;struct _tuple10 f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Break_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Continue_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Goto_s_Absyn_Raw_stmt_struct{int tag;struct _fat_ptr*f1;};struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _tuple10 f2;struct _tuple10 f3;struct Cyc_Absyn_Stmt*f4;};struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;void*f3;};struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Switch_clause**f2;};struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Decl*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct{int tag;struct _fat_ptr*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct _tuple10 f2;};struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_List_List*f2;void*f3;};struct Cyc_Absyn_Stmt{void*r;unsigned loc;void*annot;};struct Cyc_Absyn_Wild_p_Absyn_Raw_pat_struct{int tag;};struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_AliasVar_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_TagInt_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Tuple_p_Absyn_Raw_pat_struct{int tag;struct Cyc_List_List*f1;int f2;};struct Cyc_Absyn_Pointer_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Pat*f1;};struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct{int tag;union Cyc_Absyn_AggrInfo*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;int f4;};struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;struct Cyc_List_List*f3;int f4;};struct Cyc_Absyn_Null_p_Absyn_Raw_pat_struct{int tag;};struct Cyc_Absyn_Int_p_Absyn_Raw_pat_struct{int tag;enum Cyc_Absyn_Sign f1;int f2;};struct Cyc_Absyn_Char_p_Absyn_Raw_pat_struct{int tag;char f1;};struct Cyc_Absyn_Float_p_Absyn_Raw_pat_struct{int tag;struct _fat_ptr f1;int f2;};struct Cyc_Absyn_Enum_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_AnonEnum_p_Absyn_Raw_pat_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_UnknownId_p_Absyn_Raw_pat_struct{int tag;struct _tuple0*f1;};struct Cyc_Absyn_UnknownCall_p_Absyn_Raw_pat_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;int f3;};struct Cyc_Absyn_Exp_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Pat{void*r;void*topt;unsigned loc;};struct Cyc_Absyn_Switch_clause{struct Cyc_Absyn_Pat*pattern;struct Cyc_Core_Opt*pat_vars;struct Cyc_Absyn_Exp*where_clause;struct Cyc_Absyn_Stmt*body;unsigned loc;};struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct{int tag;struct _tuple0*f1;};struct Cyc_Absyn_Global_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Funname_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Param_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Local_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Pat_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Vardecl{enum Cyc_Absyn_Scope sc;struct _tuple0*name;unsigned varloc;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*initializer;void*rgn;struct Cyc_List_List*attributes;int escapes;};struct Cyc_Absyn_Fndecl{enum Cyc_Absyn_Scope sc;int is_inline;struct _tuple0*name;struct Cyc_Absyn_Stmt*body;struct Cyc_Absyn_FnInfo i;void*cached_type;struct Cyc_Core_Opt*param_vardecls;struct Cyc_Absyn_Vardecl*fn_vardecl;};struct Cyc_Absyn_Aggrfield{struct _fat_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*width;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*requires_clause;};struct Cyc_Absyn_AggrdeclImpl{struct Cyc_List_List*exist_vars;struct Cyc_List_List*rgn_po;struct Cyc_List_List*fields;int tagged;};struct Cyc_Absyn_Aggrdecl{enum Cyc_Absyn_AggrKind kind;enum Cyc_Absyn_Scope sc;struct _tuple0*name;struct Cyc_List_List*tvs;struct Cyc_Absyn_AggrdeclImpl*impl;struct Cyc_List_List*attributes;int expected_mem_kind;};struct Cyc_Absyn_Datatypefield{struct _tuple0*name;struct Cyc_List_List*typs;unsigned loc;enum Cyc_Absyn_Scope sc;};struct Cyc_Absyn_Datatypedecl{enum Cyc_Absyn_Scope sc;struct _tuple0*name;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*fields;int is_extensible;};struct Cyc_Absyn_Enumfield{struct _tuple0*name;struct Cyc_Absyn_Exp*tag;unsigned loc;};struct Cyc_Absyn_Enumdecl{enum Cyc_Absyn_Scope sc;struct _tuple0*name;struct Cyc_Core_Opt*fields;};struct Cyc_Absyn_Typedefdecl{struct _tuple0*name;struct Cyc_Absyn_Tqual tq;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*kind;void*defn;struct Cyc_List_List*atts;int extern_c;};struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Pat*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;void*f4;};struct Cyc_Absyn_Letv_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;};struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;};struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Typedefdecl*f1;};struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct{int tag;struct _fat_ptr*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_ExternC_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;};struct _tuple11{unsigned f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct _tuple11*f4;};struct Cyc_Absyn_Porton_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Portoff_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Tempeston_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Tempestoff_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Decl{void*r;unsigned loc;};struct Cyc_Absyn_ArrayElement_Absyn_Designator_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_FieldName_Absyn_Designator_struct{int tag;struct _fat_ptr*f1;};extern char Cyc_Absyn_EmptyAnnot[11U];struct Cyc_Absyn_EmptyAnnot_Absyn_AbsynAnnot_struct{char*tag;};
# 962 "absyn.h"
struct Cyc_Absyn_Tqual Cyc_Absyn_const_tqual(unsigned);
struct Cyc_Absyn_Tqual Cyc_Absyn_empty_tqual(unsigned);
# 979
void*Cyc_Absyn_new_evar(struct Cyc_Core_Opt*k,struct Cyc_Core_Opt*tenv);
# 984
extern void*Cyc_Absyn_char_type;extern void*Cyc_Absyn_uchar_type;extern void*Cyc_Absyn_ushort_type;extern void*Cyc_Absyn_uint_type;extern void*Cyc_Absyn_ulong_type;
# 986
extern void*Cyc_Absyn_schar_type;extern void*Cyc_Absyn_sshort_type;extern void*Cyc_Absyn_sint_type;extern void*Cyc_Absyn_slong_type;
# 988
extern void*Cyc_Absyn_float_type;extern void*Cyc_Absyn_double_type;extern void*Cyc_Absyn_long_double_type;
# 995
extern void*Cyc_Absyn_true_type;extern void*Cyc_Absyn_false_type;
# 1038
void*Cyc_Absyn_at_type(void*t,void*rgn,struct Cyc_Absyn_Tqual,void*zero_term);
# 1042
void*Cyc_Absyn_fatptr_type(void*t,void*rgn,struct Cyc_Absyn_Tqual,void*zeroterm);struct _union_RelnOp_RConst{int tag;unsigned val;};struct _union_RelnOp_RVar{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RNumelts{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RType{int tag;void*val;};struct _union_RelnOp_RParam{int tag;unsigned val;};struct _union_RelnOp_RParamNumelts{int tag;unsigned val;};struct _union_RelnOp_RReturn{int tag;unsigned val;};union Cyc_Relations_RelnOp{struct _union_RelnOp_RConst RConst;struct _union_RelnOp_RVar RVar;struct _union_RelnOp_RNumelts RNumelts;struct _union_RelnOp_RType RType;struct _union_RelnOp_RParam RParam;struct _union_RelnOp_RParamNumelts RParamNumelts;struct _union_RelnOp_RReturn RReturn;};
# 50 "relations-ap.h"
enum Cyc_Relations_Relation{Cyc_Relations_Req =0U,Cyc_Relations_Rneq =1U,Cyc_Relations_Rlte =2U,Cyc_Relations_Rlt =3U};struct Cyc_Relations_Reln{union Cyc_Relations_RelnOp rop1;enum Cyc_Relations_Relation relation;union Cyc_Relations_RelnOp rop2;};struct Cyc_RgnOrder_RgnPO;extern char Cyc_Tcenv_Env_error[10U];struct Cyc_Tcenv_Env_error_exn_struct{char*tag;};struct Cyc_Tcenv_Genv{struct Cyc_Dict_Dict aggrdecls;struct Cyc_Dict_Dict datatypedecls;struct Cyc_Dict_Dict enumdecls;struct Cyc_Dict_Dict typedefs;struct Cyc_Dict_Dict ordinaries;};struct Cyc_Tcenv_Fenv;struct Cyc_Tcenv_Tenv{struct Cyc_List_List*ns;struct Cyc_Tcenv_Genv*ae;struct Cyc_Tcenv_Fenv*le;int allow_valueof: 1;int in_extern_c_include: 1;int in_tempest: 1;int tempest_generalize: 1;};
# 90 "tcenv.h"
enum Cyc_Tcenv_NewStatus{Cyc_Tcenv_NoneNew =0U,Cyc_Tcenv_InNew =1U,Cyc_Tcenv_InNewAggr =2U};
# 100
struct Cyc_List_List*Cyc_Tcenv_lookup_type_vars(struct Cyc_Tcenv_Tenv*);
# 31 "tcutil.h"
void Cyc_Tcutil_terr(unsigned,struct _fat_ptr,struct _fat_ptr);
# 158
extern struct Cyc_Core_Opt Cyc_Tcutil_rko;struct _tuple12{struct Cyc_List_List*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct Cyc_List_List*f4;char f5;int f6;};
# 38 "formatstr.cyc"
struct Cyc_Core_Opt*Cyc_Formatstr_parse_conversionspecification(struct _RegionHandle*r,struct _fat_ptr s,int i){
# 40
unsigned long len=({Cyc_strlen((struct _fat_ptr)s);});
if(i < 0 ||(unsigned long)i >= len)return 0;{struct Cyc_List_List*flags=0;
# 45
char c=' ';
for(0;(unsigned long)i < len;++ i){
c=*((const char*)_check_fat_subscript(s,sizeof(char),i));
{char _tmp0=c;switch(_tmp0){case 43U: _LL1: _LL2:
 goto _LL4;case 45U: _LL3: _LL4:
 goto _LL6;case 32U: _LL5: _LL6:
 goto _LL8;case 35U: _LL7: _LL8:
 goto _LLA;case 48U: _LL9: _LLA:
 flags=({struct Cyc_List_List*_tmp1=_region_malloc(r,sizeof(*_tmp1));_tmp1->hd=(void*)((int)c),_tmp1->tl=flags;_tmp1;});continue;default: _LLB: _LLC:
 goto _LL0;}_LL0:;}
# 56
break;}
# 58
if((unsigned long)i >= len)return 0;flags=({Cyc_List_imp_rev(flags);});{
# 62
struct Cyc_List_List*width=0;
c=*((const char*)_check_fat_subscript(s,sizeof(char),i));
if((int)c == (int)'*'){
width=({struct Cyc_List_List*_tmp2=_region_malloc(r,sizeof(*_tmp2));_tmp2->hd=(void*)((int)c),_tmp2->tl=width;_tmp2;});
++ i;}else{
# 68
for(0;(unsigned long)i < len;++ i){
c=*((const char*)_check_fat_subscript(s,sizeof(char),i));
if(({isdigit((int)c);}))width=({struct Cyc_List_List*_tmp3=_region_malloc(r,sizeof(*_tmp3));_tmp3->hd=(void*)((int)c),_tmp3->tl=width;_tmp3;});else{
break;}}}
# 74
if((unsigned long)i >= len)return 0;width=({Cyc_List_imp_rev(width);});{
# 78
struct Cyc_List_List*precision=0;
c=*((const char*)_check_fat_subscript(s,sizeof(char),i));
if((int)c == (int)'.'){
precision=({struct Cyc_List_List*_tmp4=_region_malloc(r,sizeof(*_tmp4));_tmp4->hd=(void*)((int)c),_tmp4->tl=precision;_tmp4;});
if((unsigned long)++ i >= len)return 0;c=*((const char*)_check_fat_subscript(s,sizeof(char),i));
# 84
if((int)c == (int)'*'){
precision=({struct Cyc_List_List*_tmp5=_region_malloc(r,sizeof(*_tmp5));_tmp5->hd=(void*)((int)c),_tmp5->tl=precision;_tmp5;});
++ i;}else{
# 88
for(0;(unsigned long)i < len;++ i){
c=*((const char*)_check_fat_subscript(s,sizeof(char),i));
if(({isdigit((int)c);}))precision=({struct Cyc_List_List*_tmp6=_region_malloc(r,sizeof(*_tmp6));_tmp6->hd=(void*)((int)c),_tmp6->tl=precision;_tmp6;});else{
break;}}}}
# 80
if((unsigned long)i >= len)
# 94
return 0;
# 80
precision=({Cyc_List_imp_rev(precision);});{
# 99
struct Cyc_List_List*lenmod=0;
c=*((const char*)_check_fat_subscript(s,sizeof(char),i));
{char _tmp7=c;switch(_tmp7){case 104U: _LLE: _LLF:
# 103
 lenmod=({struct Cyc_List_List*_tmp8=_region_malloc(r,sizeof(*_tmp8));_tmp8->hd=(void*)((int)c),_tmp8->tl=lenmod;_tmp8;});
if((unsigned long)++ i >= len)return 0;c=*((const char*)_check_fat_subscript(s,sizeof(char),i));
# 106
if((int)c == (int)'h'){lenmod=({struct Cyc_List_List*_tmp9=_region_malloc(r,sizeof(*_tmp9));_tmp9->hd=(void*)((int)c),_tmp9->tl=lenmod;_tmp9;});++ i;}goto _LLD;case 108U: _LL10: _LL11:
# 109
 lenmod=({struct Cyc_List_List*_tmpA=_region_malloc(r,sizeof(*_tmpA));_tmpA->hd=(void*)((int)c),_tmpA->tl=lenmod;_tmpA;});
if((unsigned long)++ i >= len)return 0;c=*((const char*)_check_fat_subscript(s,sizeof(char),i));
# 112
if((int)c == (int)'l'){lenmod=({struct Cyc_List_List*_tmpB=_region_malloc(r,sizeof(*_tmpB));_tmpB->hd=(void*)((int)c),_tmpB->tl=lenmod;_tmpB;});++ i;}goto _LLD;case 106U: _LL12: _LL13:
# 114
 goto _LL15;case 122U: _LL14: _LL15:
 goto _LL17;case 116U: _LL16: _LL17:
 goto _LL19;case 76U: _LL18: _LL19:
# 118
 lenmod=({struct Cyc_List_List*_tmpC=_region_malloc(r,sizeof(*_tmpC));_tmpC->hd=(void*)((int)c),_tmpC->tl=lenmod;_tmpC;});
++ i;
goto _LLD;default: _LL1A: _LL1B:
 goto _LLD;}_LLD:;}
# 123
if((unsigned long)i >= len)return 0;lenmod=({Cyc_List_imp_rev(lenmod);});
# 127
c=*((const char*)_check_fat_subscript(s,sizeof(char),i));
{char _tmpD=c;switch(_tmpD){case 100U: _LL1D: _LL1E:
 goto _LL20;case 105U: _LL1F: _LL20:
 goto _LL22;case 111U: _LL21: _LL22:
 goto _LL24;case 117U: _LL23: _LL24:
 goto _LL26;case 120U: _LL25: _LL26:
 goto _LL28;case 88U: _LL27: _LL28:
 goto _LL2A;case 102U: _LL29: _LL2A:
 goto _LL2C;case 70U: _LL2B: _LL2C:
 goto _LL2E;case 101U: _LL2D: _LL2E:
 goto _LL30;case 69U: _LL2F: _LL30:
 goto _LL32;case 103U: _LL31: _LL32:
 goto _LL34;case 71U: _LL33: _LL34:
 goto _LL36;case 97U: _LL35: _LL36:
 goto _LL38;case 65U: _LL37: _LL38:
 goto _LL3A;case 99U: _LL39: _LL3A:
 goto _LL3C;case 115U: _LL3B: _LL3C:
 goto _LL3E;case 112U: _LL3D: _LL3E:
 goto _LL40;case 110U: _LL3F: _LL40:
 goto _LL42;case 37U: _LL41: _LL42:
 goto _LL1C;default: _LL43: _LL44:
 return 0;}_LL1C:;}
# 159 "formatstr.cyc"
return({struct Cyc_Core_Opt*_tmpF=_region_malloc(r,sizeof(*_tmpF));({struct _tuple12*_tmp146=({struct _tuple12*_tmpE=_region_malloc(r,sizeof(*_tmpE));_tmpE->f1=flags,_tmpE->f2=width,_tmpE->f3=precision,_tmpE->f4=lenmod,_tmpE->f5=c,_tmpE->f6=i + 1;_tmpE;});_tmpF->v=_tmp146;});_tmpF;});}}}}}
# 162
struct Cyc_List_List*Cyc_Formatstr_get_format_types(struct Cyc_Tcenv_Tenv*te,struct _fat_ptr s,int isCproto,unsigned loc){
# 165
unsigned long len=({Cyc_strlen((struct _fat_ptr)s);});
struct Cyc_List_List*typs=0;
int i;
struct _RegionHandle _tmp11=_new_region("temp");struct _RegionHandle*temp=& _tmp11;_push_region(temp);
for(i=0;(unsigned long)i < len;++ i){
if((int)*((const char*)_check_fat_subscript(s,sizeof(char),i))!= (int)'%')continue;{struct Cyc_Core_Opt*cs=({Cyc_Formatstr_parse_conversionspecification(temp,s,i + 1);});
# 172
if(cs == 0){
({void*_tmp12=0U;({unsigned _tmp148=loc;struct _fat_ptr _tmp147=({const char*_tmp13="bad format string";_tag_fat(_tmp13,sizeof(char),18U);});Cyc_Tcutil_terr(_tmp148,_tmp147,_tag_fat(_tmp12,sizeof(void*),0U));});});{
struct Cyc_List_List*_tmp14=0;_npop_handler(0U);return _tmp14;}}{
# 172
struct _tuple12*_stmttmp0=(struct _tuple12*)cs->v;int _tmp1A;char _tmp19;struct Cyc_List_List*_tmp18;struct Cyc_List_List*_tmp17;struct Cyc_List_List*_tmp16;struct Cyc_List_List*_tmp15;_LL1: _tmp15=_stmttmp0->f1;_tmp16=_stmttmp0->f2;_tmp17=_stmttmp0->f3;_tmp18=_stmttmp0->f4;_tmp19=_stmttmp0->f5;_tmp1A=_stmttmp0->f6;_LL2: {struct Cyc_List_List*flags=_tmp15;struct Cyc_List_List*width=_tmp16;struct Cyc_List_List*precision=_tmp17;struct Cyc_List_List*lenmod=_tmp18;char c=_tmp19;int j=_tmp1A;
# 177
i=j - 1;
{struct Cyc_List_List*_tmp1B=lenmod;int _tmp1C;if(_tmp1B != 0){if(((struct Cyc_List_List*)_tmp1B)->tl == 0){_LL4: _tmp1C=(int)_tmp1B->hd;if(
(_tmp1C == (int)'j' || _tmp1C == (int)'z')|| _tmp1C == (int)'t'){_LL5: {int x=_tmp1C;
# 182
({struct Cyc_Int_pa_PrintArg_struct _tmp1F=({struct Cyc_Int_pa_PrintArg_struct _tmp127;_tmp127.tag=1U,_tmp127.f1=(unsigned long)x;_tmp127;});void*_tmp1D[1U];_tmp1D[0]=& _tmp1F;({unsigned _tmp14A=loc;struct _fat_ptr _tmp149=({const char*_tmp1E="length modifier '%c' is not supported";_tag_fat(_tmp1E,sizeof(char),38U);});Cyc_Tcutil_terr(_tmp14A,_tmp149,_tag_fat(_tmp1D,sizeof(void*),1U));});});{
struct Cyc_List_List*_tmp20=0;_npop_handler(0U);return _tmp20;}}}else{goto _LL6;}}else{goto _LL6;}}else{_LL6: _LL7:
 goto _LL3;}_LL3:;}
# 186
{struct Cyc_List_List*_tmp21=width;int _tmp22;if(_tmp21 != 0){if(((struct Cyc_List_List*)_tmp21)->tl == 0){_LL9: _tmp22=(int)_tmp21->hd;if(_tmp22 == (int)'*'){_LLA: {int x=_tmp22;
typs=({struct Cyc_List_List*_tmp23=_cycalloc(sizeof(*_tmp23));_tmp23->hd=Cyc_Absyn_sint_type,_tmp23->tl=typs;_tmp23;});goto _LL8;}}else{goto _LLB;}}else{goto _LLB;}}else{_LLB: _LLC:
 goto _LL8;}_LL8:;}
# 190
{struct Cyc_List_List*_tmp24=precision;int _tmp26;int _tmp25;if(_tmp24 != 0){if(((struct Cyc_List_List*)_tmp24)->tl != 0){if(((struct Cyc_List_List*)((struct Cyc_List_List*)_tmp24)->tl)->tl == 0){_LLE: _tmp25=(int)_tmp24->hd;_tmp26=(int)(_tmp24->tl)->hd;if(
_tmp25 == (int)'.' && _tmp26 == (int)'*'){_LLF: {int x=_tmp25;int y=_tmp26;
typs=({struct Cyc_List_List*_tmp27=_cycalloc(sizeof(*_tmp27));_tmp27->hd=Cyc_Absyn_sint_type,_tmp27->tl=typs;_tmp27;});goto _LLD;}}else{goto _LL10;}}else{goto _LL10;}}else{goto _LL10;}}else{_LL10: _LL11:
 goto _LLD;}_LLD:;}{
# 195
void*t;
char _tmp28=c;switch(_tmp28){case 100U: _LL13: _LL14:
 goto _LL16;case 105U: _LL15: _LL16:
# 199
{struct Cyc_List_List*f=flags;for(0;f != 0;f=f->tl){
if((int)f->hd == (int)'#'){
({struct Cyc_Int_pa_PrintArg_struct _tmp2B=({struct Cyc_Int_pa_PrintArg_struct _tmp128;_tmp128.tag=1U,_tmp128.f1=(unsigned long)((int)c);_tmp128;});void*_tmp29[1U];_tmp29[0]=& _tmp2B;({unsigned _tmp14C=loc;struct _fat_ptr _tmp14B=({const char*_tmp2A="flag '#' is not valid with %%%c";_tag_fat(_tmp2A,sizeof(char),32U);});Cyc_Tcutil_terr(_tmp14C,_tmp14B,_tag_fat(_tmp29,sizeof(void*),1U));});});{
struct Cyc_List_List*_tmp2C=0;_npop_handler(0U);return _tmp2C;}}}}
# 199
{struct Cyc_List_List*_tmp2D=lenmod;int _tmp2F;int _tmp2E;int _tmp30;int _tmp31;if(_tmp2D == 0){_LL3C: _LL3D:
# 205
 t=Cyc_Absyn_sint_type;goto _LL3B;}else{if(((struct Cyc_List_List*)_tmp2D)->tl == 0){_LL3E: _tmp31=(int)_tmp2D->hd;if(_tmp31 == (int)'l'){_LL3F: {int x=_tmp31;
t=Cyc_Absyn_slong_type;goto _LL3B;}}else{_LL40: _tmp30=(int)_tmp2D->hd;if(_tmp30 == (int)'h'){_LL41: {int x=_tmp30;
t=Cyc_Absyn_sshort_type;goto _LL3B;}}else{goto _LL44;}}}else{if(((struct Cyc_List_List*)((struct Cyc_List_List*)_tmp2D)->tl)->tl == 0){_LL42: _tmp2E=(int)_tmp2D->hd;_tmp2F=(int)(_tmp2D->tl)->hd;if(
_tmp2E == (int)'h' && _tmp2F == (int)'h'){_LL43: {int x=_tmp2E;int y=_tmp2F;
t=Cyc_Absyn_schar_type;goto _LL3B;}}else{goto _LL44;}}else{_LL44: _LL45:
# 211
({struct Cyc_String_pa_PrintArg_struct _tmp34=({struct Cyc_String_pa_PrintArg_struct _tmp12A;_tmp12A.tag=0U,({
struct _fat_ptr _tmp14D=(struct _fat_ptr)((struct _fat_ptr)({Cyc_implode(lenmod);}));_tmp12A.f1=_tmp14D;});_tmp12A;});struct Cyc_Int_pa_PrintArg_struct _tmp35=({struct Cyc_Int_pa_PrintArg_struct _tmp129;_tmp129.tag=1U,_tmp129.f1=(unsigned long)((int)c);_tmp129;});void*_tmp32[2U];_tmp32[0]=& _tmp34,_tmp32[1]=& _tmp35;({unsigned _tmp14F=loc;struct _fat_ptr _tmp14E=({const char*_tmp33="length modifier '%s' is not allowed with %%%c";_tag_fat(_tmp33,sizeof(char),46U);});Cyc_Tcutil_terr(_tmp14F,_tmp14E,_tag_fat(_tmp32,sizeof(void*),2U));});});{
struct Cyc_List_List*_tmp36=0;_npop_handler(0U);return _tmp36;}}}}_LL3B:;}
# 215
typs=({struct Cyc_List_List*_tmp37=_cycalloc(sizeof(*_tmp37));_tmp37->hd=t,_tmp37->tl=typs;_tmp37;});
goto _LL12;case 117U: _LL17: _LL18:
# 218
{struct Cyc_List_List*f=flags;for(0;f != 0;f=f->tl){
if((int)f->hd == (int)'#'){
({void*_tmp38=0U;({unsigned _tmp151=loc;struct _fat_ptr _tmp150=({const char*_tmp39="Flag '#' not valid with %%u";_tag_fat(_tmp39,sizeof(char),28U);});Cyc_Tcutil_terr(_tmp151,_tmp150,_tag_fat(_tmp38,sizeof(void*),0U));});});{
struct Cyc_List_List*_tmp3A=0;_npop_handler(0U);return _tmp3A;}}}}
# 218
goto _LL1A;case 111U: _LL19: _LL1A:
# 224
 goto _LL1C;case 120U: _LL1B: _LL1C:
 goto _LL1E;case 88U: _LL1D: _LL1E:
# 227
{struct Cyc_List_List*_tmp3B=lenmod;int _tmp3D;int _tmp3C;int _tmp3E;int _tmp3F;if(_tmp3B == 0){_LL47: _LL48:
 t=Cyc_Absyn_uint_type;goto _LL46;}else{if(((struct Cyc_List_List*)_tmp3B)->tl == 0){_LL49: _tmp3F=(int)_tmp3B->hd;if(_tmp3F == (int)'l'){_LL4A: {int x=_tmp3F;
t=Cyc_Absyn_ulong_type;goto _LL46;}}else{_LL4B: _tmp3E=(int)_tmp3B->hd;if(_tmp3E == (int)'h'){_LL4C: {int x=_tmp3E;
t=Cyc_Absyn_ushort_type;goto _LL46;}}else{goto _LL4F;}}}else{if(((struct Cyc_List_List*)((struct Cyc_List_List*)_tmp3B)->tl)->tl == 0){_LL4D: _tmp3C=(int)_tmp3B->hd;_tmp3D=(int)(_tmp3B->tl)->hd;if(
_tmp3C == (int)'h' && _tmp3D == (int)'h'){_LL4E: {int x=_tmp3C;int y=_tmp3D;
t=Cyc_Absyn_uchar_type;goto _LL46;}}else{goto _LL4F;}}else{_LL4F: _LL50:
# 235
({struct Cyc_String_pa_PrintArg_struct _tmp42=({struct Cyc_String_pa_PrintArg_struct _tmp12C;_tmp12C.tag=0U,({
struct _fat_ptr _tmp152=(struct _fat_ptr)((struct _fat_ptr)({Cyc_implode(lenmod);}));_tmp12C.f1=_tmp152;});_tmp12C;});struct Cyc_Int_pa_PrintArg_struct _tmp43=({struct Cyc_Int_pa_PrintArg_struct _tmp12B;_tmp12B.tag=1U,_tmp12B.f1=(unsigned long)((int)c);_tmp12B;});void*_tmp40[2U];_tmp40[0]=& _tmp42,_tmp40[1]=& _tmp43;({unsigned _tmp154=loc;struct _fat_ptr _tmp153=({const char*_tmp41="length modifier '%s' is not allowed with %%%c";_tag_fat(_tmp41,sizeof(char),46U);});Cyc_Tcutil_terr(_tmp154,_tmp153,_tag_fat(_tmp40,sizeof(void*),2U));});});{
struct Cyc_List_List*_tmp44=0;_npop_handler(0U);return _tmp44;}}}}_LL46:;}
# 239
typs=({struct Cyc_List_List*_tmp45=_cycalloc(sizeof(*_tmp45));_tmp45->hd=t,_tmp45->tl=typs;_tmp45;});
goto _LL12;case 102U: _LL1F: _LL20:
 goto _LL22;case 70U: _LL21: _LL22:
 goto _LL24;case 101U: _LL23: _LL24:
 goto _LL26;case 69U: _LL25: _LL26:
 goto _LL28;case 103U: _LL27: _LL28:
 goto _LL2A;case 71U: _LL29: _LL2A:
 goto _LL2C;case 97U: _LL2B: _LL2C:
 goto _LL2E;case 65U: _LL2D: _LL2E:
# 255
{struct Cyc_List_List*_tmp46=lenmod;int _tmp47;if(_tmp46 == 0){_LL52: _LL53:
# 257
 typs=({struct Cyc_List_List*_tmp48=_cycalloc(sizeof(*_tmp48));_tmp48->hd=Cyc_Absyn_double_type,_tmp48->tl=typs;_tmp48;});
goto _LL51;}else{if(((struct Cyc_List_List*)_tmp46)->tl == 0){_LL54: _tmp47=(int)_tmp46->hd;if(_tmp47 == (int)'l'){_LL55: {int x=_tmp47;
# 260
typs=({struct Cyc_List_List*_tmp49=_cycalloc(sizeof(*_tmp49));_tmp49->hd=Cyc_Absyn_long_double_type,_tmp49->tl=typs;_tmp49;});goto _LL51;}}else{goto _LL56;}}else{_LL56: _LL57:
# 262
({struct Cyc_String_pa_PrintArg_struct _tmp4C=({struct Cyc_String_pa_PrintArg_struct _tmp12E;_tmp12E.tag=0U,({
struct _fat_ptr _tmp155=(struct _fat_ptr)((struct _fat_ptr)({Cyc_implode(lenmod);}));_tmp12E.f1=_tmp155;});_tmp12E;});struct Cyc_Int_pa_PrintArg_struct _tmp4D=({struct Cyc_Int_pa_PrintArg_struct _tmp12D;_tmp12D.tag=1U,_tmp12D.f1=(unsigned long)((int)c);_tmp12D;});void*_tmp4A[2U];_tmp4A[0]=& _tmp4C,_tmp4A[1]=& _tmp4D;({unsigned _tmp157=loc;struct _fat_ptr _tmp156=({const char*_tmp4B="length modifier '%s' is not allowed with %%%c";_tag_fat(_tmp4B,sizeof(char),46U);});Cyc_Tcutil_terr(_tmp157,_tmp156,_tag_fat(_tmp4A,sizeof(void*),2U));});});{
struct Cyc_List_List*_tmp4E=0;_npop_handler(0U);return _tmp4E;}}}_LL51:;}
# 266
goto _LL12;case 99U: _LL2F: _LL30:
# 268
{struct Cyc_List_List*f=flags;for(0;f != 0;f=f->tl){
if((int)f->hd == (int)'#' ||(int)f->hd == (int)'0'){
({struct Cyc_Int_pa_PrintArg_struct _tmp51=({struct Cyc_Int_pa_PrintArg_struct _tmp12F;_tmp12F.tag=1U,_tmp12F.f1=(unsigned long)((int)f->hd);_tmp12F;});void*_tmp4F[1U];_tmp4F[0]=& _tmp51;({unsigned _tmp159=loc;struct _fat_ptr _tmp158=({const char*_tmp50="flag '%c' not allowed with %%c";_tag_fat(_tmp50,sizeof(char),31U);});Cyc_Tcutil_terr(_tmp159,_tmp158,_tag_fat(_tmp4F,sizeof(void*),1U));});});{
struct Cyc_List_List*_tmp52=0;_npop_handler(0U);return _tmp52;}}}}
# 268
if(lenmod != 0){
# 276
({struct Cyc_String_pa_PrintArg_struct _tmp55=({struct Cyc_String_pa_PrintArg_struct _tmp130;_tmp130.tag=0U,({
struct _fat_ptr _tmp15A=(struct _fat_ptr)((struct _fat_ptr)({Cyc_implode(lenmod);}));_tmp130.f1=_tmp15A;});_tmp130;});void*_tmp53[1U];_tmp53[0]=& _tmp55;({unsigned _tmp15C=loc;struct _fat_ptr _tmp15B=({const char*_tmp54="length modifier '%s' not allowed with %%c";_tag_fat(_tmp54,sizeof(char),42U);});Cyc_Tcutil_terr(_tmp15C,_tmp15B,_tag_fat(_tmp53,sizeof(void*),1U));});});{
struct Cyc_List_List*_tmp56=0;_npop_handler(0U);return _tmp56;}}
# 268
if(precision != 0){
# 281
({struct Cyc_String_pa_PrintArg_struct _tmp59=({struct Cyc_String_pa_PrintArg_struct _tmp131;_tmp131.tag=0U,({
struct _fat_ptr _tmp15D=(struct _fat_ptr)((struct _fat_ptr)({Cyc_implode(precision);}));_tmp131.f1=_tmp15D;});_tmp131;});void*_tmp57[1U];_tmp57[0]=& _tmp59;({unsigned _tmp15F=loc;struct _fat_ptr _tmp15E=({const char*_tmp58="precision '%s' not allowed with %%c";_tag_fat(_tmp58,sizeof(char),36U);});Cyc_Tcutil_terr(_tmp15F,_tmp15E,_tag_fat(_tmp57,sizeof(void*),1U));});});{
struct Cyc_List_List*_tmp5A=0;_npop_handler(0U);return _tmp5A;}}
# 268
typs=({struct Cyc_List_List*_tmp5B=_cycalloc(sizeof(*_tmp5B));
# 285
_tmp5B->hd=Cyc_Absyn_sint_type,_tmp5B->tl=typs;_tmp5B;});
goto _LL12;case 115U: _LL31: _LL32:
# 289
{struct Cyc_List_List*f=flags;for(0;f != 0;f=f->tl){
if((int)f->hd != (int)'-'){
({void*_tmp5C=0U;({unsigned _tmp161=loc;struct _fat_ptr _tmp160=({const char*_tmp5D="a flag not allowed with %%s";_tag_fat(_tmp5D,sizeof(char),28U);});Cyc_Tcutil_terr(_tmp161,_tmp160,_tag_fat(_tmp5C,sizeof(void*),0U));});});{
struct Cyc_List_List*_tmp5E=0;_npop_handler(0U);return _tmp5E;}}}}
# 289
if(lenmod != 0){
# 297
({void*_tmp5F=0U;({unsigned _tmp163=loc;struct _fat_ptr _tmp162=({const char*_tmp60="length modifiers not allowed with %%s";_tag_fat(_tmp60,sizeof(char),38U);});Cyc_Tcutil_terr(_tmp163,_tmp162,_tag_fat(_tmp5F,sizeof(void*),0U));});});{
struct Cyc_List_List*_tmp61=0;_npop_handler(0U);return _tmp61;}}{
# 289
void*ptr;
# 303
if(!isCproto)
ptr=({void*(*_tmp62)(void*t,void*rgn,struct Cyc_Absyn_Tqual,void*zeroterm)=Cyc_Absyn_fatptr_type;void*_tmp63=Cyc_Absyn_char_type;void*_tmp64=({void*(*_tmp65)(struct Cyc_Core_Opt*k,struct Cyc_Core_Opt*tenv)=Cyc_Absyn_new_evar;struct Cyc_Core_Opt*_tmp66=& Cyc_Tcutil_rko;struct Cyc_Core_Opt*_tmp67=({struct Cyc_Core_Opt*_tmp68=_cycalloc(sizeof(*_tmp68));({
# 306
struct Cyc_List_List*_tmp164=({Cyc_Tcenv_lookup_type_vars(te);});_tmp68->v=_tmp164;});_tmp68;});_tmp65(_tmp66,_tmp67);});struct Cyc_Absyn_Tqual _tmp69=({Cyc_Absyn_const_tqual(0U);});void*_tmp6A=Cyc_Absyn_false_type;_tmp62(_tmp63,_tmp64,_tmp69,_tmp6A);});else{
# 309
ptr=({void*(*_tmp6B)(void*t,void*rgn,struct Cyc_Absyn_Tqual,void*zero_term)=Cyc_Absyn_at_type;void*_tmp6C=Cyc_Absyn_char_type;void*_tmp6D=({void*(*_tmp6E)(struct Cyc_Core_Opt*k,struct Cyc_Core_Opt*tenv)=Cyc_Absyn_new_evar;struct Cyc_Core_Opt*_tmp6F=& Cyc_Tcutil_rko;struct Cyc_Core_Opt*_tmp70=({struct Cyc_Core_Opt*_tmp71=_cycalloc(sizeof(*_tmp71));({
# 311
struct Cyc_List_List*_tmp165=({Cyc_Tcenv_lookup_type_vars(te);});_tmp71->v=_tmp165;});_tmp71;});_tmp6E(_tmp6F,_tmp70);});struct Cyc_Absyn_Tqual _tmp72=({Cyc_Absyn_const_tqual(0U);});void*_tmp73=Cyc_Absyn_true_type;_tmp6B(_tmp6C,_tmp6D,_tmp72,_tmp73);});}
# 313
typs=({struct Cyc_List_List*_tmp74=_cycalloc(sizeof(*_tmp74));_tmp74->hd=ptr,_tmp74->tl=typs;_tmp74;});
goto _LL12;}case 112U: _LL33: _LL34:
# 317
 typs=({struct Cyc_List_List*_tmp75=_cycalloc(sizeof(*_tmp75));_tmp75->hd=Cyc_Absyn_uint_type,_tmp75->tl=typs;_tmp75;});
goto _LL12;case 110U: _LL35: _LL36:
# 320
{struct Cyc_List_List*f=flags;for(0;f != 0;f=f->tl){
if((int)f->hd == (int)'#' ||(int)f->hd == (int)'0'){
({struct Cyc_Int_pa_PrintArg_struct _tmp78=({struct Cyc_Int_pa_PrintArg_struct _tmp132;_tmp132.tag=1U,_tmp132.f1=(unsigned long)((int)f->hd);_tmp132;});void*_tmp76[1U];_tmp76[0]=& _tmp78;({unsigned _tmp167=loc;struct _fat_ptr _tmp166=({const char*_tmp77="flag '%c' not allowed with %%n";_tag_fat(_tmp77,sizeof(char),31U);});Cyc_Tcutil_terr(_tmp167,_tmp166,_tag_fat(_tmp76,sizeof(void*),1U));});});{
struct Cyc_List_List*_tmp79=0;_npop_handler(0U);return _tmp79;}}}}
# 320
if(precision != 0){
# 326
({struct Cyc_String_pa_PrintArg_struct _tmp7C=({struct Cyc_String_pa_PrintArg_struct _tmp133;_tmp133.tag=0U,({
struct _fat_ptr _tmp168=(struct _fat_ptr)((struct _fat_ptr)({Cyc_implode(precision);}));_tmp133.f1=_tmp168;});_tmp133;});void*_tmp7A[1U];_tmp7A[0]=& _tmp7C;({unsigned _tmp16A=loc;struct _fat_ptr _tmp169=({const char*_tmp7B="precision '%s' not allowed with %%n";_tag_fat(_tmp7B,sizeof(char),36U);});Cyc_Tcutil_terr(_tmp16A,_tmp169,_tag_fat(_tmp7A,sizeof(void*),1U));});});{
struct Cyc_List_List*_tmp7D=0;_npop_handler(0U);return _tmp7D;}}{
# 320
struct Cyc_Core_Opt*tvs=({struct Cyc_Core_Opt*_tmp8E=_cycalloc(sizeof(*_tmp8E));({
# 330
struct Cyc_List_List*_tmp16B=({Cyc_Tcenv_lookup_type_vars(te);});_tmp8E->v=_tmp16B;});_tmp8E;});
{struct Cyc_List_List*_tmp7E=lenmod;int _tmp80;int _tmp7F;int _tmp81;int _tmp82;if(_tmp7E == 0){_LL59: _LL5A:
 t=Cyc_Absyn_sint_type;goto _LL58;}else{if(((struct Cyc_List_List*)_tmp7E)->tl == 0){_LL5B: _tmp82=(int)_tmp7E->hd;if(_tmp82 == (int)'l'){_LL5C: {int x=_tmp82;
# 334
t=Cyc_Absyn_ulong_type;goto _LL58;}}else{_LL5D: _tmp81=(int)_tmp7E->hd;if(_tmp81 == (int)'h'){_LL5E: {int x=_tmp81;
t=Cyc_Absyn_sshort_type;goto _LL58;}}else{goto _LL61;}}}else{if(((struct Cyc_List_List*)((struct Cyc_List_List*)_tmp7E)->tl)->tl == 0){_LL5F: _tmp7F=(int)_tmp7E->hd;_tmp80=(int)(_tmp7E->tl)->hd;if(
_tmp7F == (int)'h' && _tmp80 == (int)'h'){_LL60: {int x=_tmp7F;int y=_tmp80;
t=Cyc_Absyn_schar_type;goto _LL58;}}else{goto _LL61;}}else{_LL61: _LL62:
# 339
({struct Cyc_String_pa_PrintArg_struct _tmp85=({struct Cyc_String_pa_PrintArg_struct _tmp135;_tmp135.tag=0U,({
struct _fat_ptr _tmp16C=(struct _fat_ptr)((struct _fat_ptr)({Cyc_implode(lenmod);}));_tmp135.f1=_tmp16C;});_tmp135;});struct Cyc_Int_pa_PrintArg_struct _tmp86=({struct Cyc_Int_pa_PrintArg_struct _tmp134;_tmp134.tag=1U,_tmp134.f1=(unsigned long)((int)c);_tmp134;});void*_tmp83[2U];_tmp83[0]=& _tmp85,_tmp83[1]=& _tmp86;({unsigned _tmp16E=loc;struct _fat_ptr _tmp16D=({const char*_tmp84="length modifier '%s' is not allowed with %%%c";_tag_fat(_tmp84,sizeof(char),46U);});Cyc_Tcutil_terr(_tmp16E,_tmp16D,_tag_fat(_tmp83,sizeof(void*),2U));});});{
struct Cyc_List_List*_tmp87=0;_npop_handler(0U);return _tmp87;}}}}_LL58:;}
# 343
t=({void*(*_tmp88)(void*t,void*rgn,struct Cyc_Absyn_Tqual,void*zero_term)=Cyc_Absyn_at_type;void*_tmp89=t;void*_tmp8A=({Cyc_Absyn_new_evar(& Cyc_Tcutil_rko,tvs);});struct Cyc_Absyn_Tqual _tmp8B=({Cyc_Absyn_empty_tqual(0U);});void*_tmp8C=Cyc_Absyn_false_type;_tmp88(_tmp89,_tmp8A,_tmp8B,_tmp8C);});
typs=({struct Cyc_List_List*_tmp8D=_cycalloc(sizeof(*_tmp8D));_tmp8D->hd=t,_tmp8D->tl=typs;_tmp8D;});
goto _LL12;}case 37U: _LL37: _LL38:
# 347
 if(flags != 0){
({struct Cyc_String_pa_PrintArg_struct _tmp91=({struct Cyc_String_pa_PrintArg_struct _tmp136;_tmp136.tag=0U,({struct _fat_ptr _tmp16F=(struct _fat_ptr)((struct _fat_ptr)({Cyc_implode(flags);}));_tmp136.f1=_tmp16F;});_tmp136;});void*_tmp8F[1U];_tmp8F[0]=& _tmp91;({unsigned _tmp171=loc;struct _fat_ptr _tmp170=({const char*_tmp90="flags '%s' not allowed with %%%%";_tag_fat(_tmp90,sizeof(char),33U);});Cyc_Tcutil_terr(_tmp171,_tmp170,_tag_fat(_tmp8F,sizeof(void*),1U));});});{
struct Cyc_List_List*_tmp92=0;_npop_handler(0U);return _tmp92;}}
# 347
if(width != 0){
# 352
({struct Cyc_String_pa_PrintArg_struct _tmp95=({struct Cyc_String_pa_PrintArg_struct _tmp137;_tmp137.tag=0U,({struct _fat_ptr _tmp172=(struct _fat_ptr)((struct _fat_ptr)({Cyc_implode(width);}));_tmp137.f1=_tmp172;});_tmp137;});void*_tmp93[1U];_tmp93[0]=& _tmp95;({unsigned _tmp174=loc;struct _fat_ptr _tmp173=({const char*_tmp94="width '%s' not allowed with %%%%";_tag_fat(_tmp94,sizeof(char),33U);});Cyc_Tcutil_terr(_tmp174,_tmp173,_tag_fat(_tmp93,sizeof(void*),1U));});});{
struct Cyc_List_List*_tmp96=0;_npop_handler(0U);return _tmp96;}}
# 347
if(precision != 0){
# 356
({struct Cyc_String_pa_PrintArg_struct _tmp99=({struct Cyc_String_pa_PrintArg_struct _tmp138;_tmp138.tag=0U,({struct _fat_ptr _tmp175=(struct _fat_ptr)((struct _fat_ptr)({Cyc_implode(precision);}));_tmp138.f1=_tmp175;});_tmp138;});void*_tmp97[1U];_tmp97[0]=& _tmp99;({unsigned _tmp177=loc;struct _fat_ptr _tmp176=({const char*_tmp98="precision '%s' not allowed with %%%%";_tag_fat(_tmp98,sizeof(char),37U);});Cyc_Tcutil_terr(_tmp177,_tmp176,_tag_fat(_tmp97,sizeof(void*),1U));});});{
struct Cyc_List_List*_tmp9A=0;_npop_handler(0U);return _tmp9A;}}
# 347
if(lenmod != 0){
# 360
({struct Cyc_String_pa_PrintArg_struct _tmp9D=({struct Cyc_String_pa_PrintArg_struct _tmp139;_tmp139.tag=0U,({struct _fat_ptr _tmp178=(struct _fat_ptr)((struct _fat_ptr)({Cyc_implode(lenmod);}));_tmp139.f1=_tmp178;});_tmp139;});void*_tmp9B[1U];_tmp9B[0]=& _tmp9D;({unsigned _tmp17A=loc;struct _fat_ptr _tmp179=({const char*_tmp9C="length modifier '%s' not allowed with %%%%";_tag_fat(_tmp9C,sizeof(char),43U);});Cyc_Tcutil_terr(_tmp17A,_tmp179,_tag_fat(_tmp9B,sizeof(void*),1U));});});{
struct Cyc_List_List*_tmp9E=0;_npop_handler(0U);return _tmp9E;}}
# 347
goto _LL12;default: _LL39: _LL3A: {
# 365
struct Cyc_List_List*_tmp9F=0;_npop_handler(0U);return _tmp9F;}}_LL12:;}}}}}{
# 368
struct Cyc_List_List*_tmpA0=({Cyc_List_imp_rev(typs);});_npop_handler(0U);return _tmpA0;}
# 169
;_pop_region();}struct _tuple13{int f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;char f4;int f5;};
# 380 "formatstr.cyc"
struct Cyc_Core_Opt*Cyc_Formatstr_parse_inputformat(struct _RegionHandle*r,struct _fat_ptr s,int i){
unsigned long len=({Cyc_strlen((struct _fat_ptr)s);});
if(i < 0 ||(unsigned long)i >= len)return 0;{int suppress=0;
# 385
char c=*((const char*)_check_fat_subscript(s,sizeof(char),i));
if((int)c == (int)'*'){
suppress=1;
++ i;
if((unsigned long)i >= len)return 0;}{
# 386
struct Cyc_List_List*width=0;
# 393
for(0;(unsigned long)i < len;++ i){
c=*((const char*)_check_fat_subscript(s,sizeof(char),i));
if(({isdigit((int)c);}))width=({struct Cyc_List_List*_tmpA2=_region_malloc(r,sizeof(*_tmpA2));_tmpA2->hd=(void*)((int)c),_tmpA2->tl=width;_tmpA2;});else{
break;}}
# 398
if((unsigned long)i >= len)return 0;width=({Cyc_List_imp_rev(width);});{
# 403
struct Cyc_List_List*lenmod=0;
c=*((const char*)_check_fat_subscript(s,sizeof(char),i));
{char _tmpA3=c;switch(_tmpA3){case 104U: _LL1: _LL2:
# 407
 lenmod=({struct Cyc_List_List*_tmpA4=_region_malloc(r,sizeof(*_tmpA4));_tmpA4->hd=(void*)((int)c),_tmpA4->tl=lenmod;_tmpA4;});
++ i;
if((unsigned long)i >= len)return 0;c=*((const char*)_check_fat_subscript(s,sizeof(char),i));
# 411
if((int)c == (int)'h'){lenmod=({struct Cyc_List_List*_tmpA5=_region_malloc(r,sizeof(*_tmpA5));_tmpA5->hd=(void*)((int)c),_tmpA5->tl=lenmod;_tmpA5;});++ i;}goto _LL0;case 108U: _LL3: _LL4:
# 414
 lenmod=({struct Cyc_List_List*_tmpA6=_region_malloc(r,sizeof(*_tmpA6));_tmpA6->hd=(void*)((int)c),_tmpA6->tl=lenmod;_tmpA6;});
++ i;
if((unsigned long)i >= len)return 0;c=*((const char*)_check_fat_subscript(s,sizeof(char),i));
# 418
if((int)c == (int)'l'){lenmod=({struct Cyc_List_List*_tmpA7=_region_malloc(r,sizeof(*_tmpA7));_tmpA7->hd=(void*)((int)c),_tmpA7->tl=lenmod;_tmpA7;});++ i;}goto _LL0;case 106U: _LL5: _LL6:
# 420
 goto _LL8;case 122U: _LL7: _LL8:
 goto _LLA;case 116U: _LL9: _LLA:
 goto _LLC;case 76U: _LLB: _LLC:
# 424
 lenmod=({struct Cyc_List_List*_tmpA8=_region_malloc(r,sizeof(*_tmpA8));_tmpA8->hd=(void*)((int)c),_tmpA8->tl=lenmod;_tmpA8;});
++ i;
goto _LL0;default: _LLD: _LLE:
 goto _LL0;}_LL0:;}
# 429
if((unsigned long)i >= len)return 0;lenmod=({Cyc_List_imp_rev(lenmod);});
# 433
c=*((const char*)_check_fat_subscript(s,sizeof(char),i));
{char _tmpA9=c;switch(_tmpA9){case 100U: _LL10: _LL11:
 goto _LL13;case 105U: _LL12: _LL13:
 goto _LL15;case 111U: _LL14: _LL15:
 goto _LL17;case 117U: _LL16: _LL17:
 goto _LL19;case 120U: _LL18: _LL19:
 goto _LL1B;case 88U: _LL1A: _LL1B:
 goto _LL1D;case 102U: _LL1C: _LL1D:
 goto _LL1F;case 70U: _LL1E: _LL1F:
 goto _LL21;case 101U: _LL20: _LL21:
 goto _LL23;case 69U: _LL22: _LL23:
 goto _LL25;case 103U: _LL24: _LL25:
 goto _LL27;case 71U: _LL26: _LL27:
 goto _LL29;case 97U: _LL28: _LL29:
 goto _LL2B;case 65U: _LL2A: _LL2B:
 goto _LL2D;case 99U: _LL2C: _LL2D:
 goto _LL2F;case 115U: _LL2E: _LL2F:
 goto _LL31;case 112U: _LL30: _LL31:
 goto _LL33;case 110U: _LL32: _LL33:
 goto _LL35;case 37U: _LL34: _LL35:
 goto _LLF;default: _LL36: _LL37:
 return 0;}_LLF:;}
# 456
return({struct Cyc_Core_Opt*_tmpAB=_region_malloc(r,sizeof(*_tmpAB));({struct _tuple13*_tmp17B=({struct _tuple13*_tmpAA=_region_malloc(r,sizeof(*_tmpAA));_tmpAA->f1=suppress,_tmpAA->f2=width,_tmpAA->f3=lenmod,_tmpAA->f4=c,_tmpAA->f5=i + 1;_tmpAA;});_tmpAB->v=_tmp17B;});_tmpAB;});}}}}
# 458
struct Cyc_List_List*Cyc_Formatstr_get_scanf_types(struct Cyc_Tcenv_Tenv*te,struct _fat_ptr s,int isCproto,unsigned loc){
# 461
unsigned long len=({Cyc_strlen((struct _fat_ptr)s);});
struct Cyc_List_List*typs=0;
int i;
{struct _RegionHandle _tmpAD=_new_region("temp");struct _RegionHandle*temp=& _tmpAD;_push_region(temp);
for(i=0;(unsigned long)i < len;++ i){
if((int)*((const char*)_check_fat_subscript(s,sizeof(char),i))!= (int)'%')continue;{struct Cyc_Core_Opt*x=({Cyc_Formatstr_parse_inputformat(temp,s,i + 1);});
# 468
if(x == 0){
({void*_tmpAE=0U;({unsigned _tmp17D=loc;struct _fat_ptr _tmp17C=({const char*_tmpAF="bad format string";_tag_fat(_tmpAF,sizeof(char),18U);});Cyc_Tcutil_terr(_tmp17D,_tmp17C,_tag_fat(_tmpAE,sizeof(void*),0U));});});{
struct Cyc_List_List*_tmpB0=0;_npop_handler(0U);return _tmpB0;}}{
# 468
struct _tuple13*_stmttmp1=(struct _tuple13*)x->v;int _tmpB5;char _tmpB4;struct Cyc_List_List*_tmpB3;struct Cyc_List_List*_tmpB2;int _tmpB1;_LL1: _tmpB1=_stmttmp1->f1;_tmpB2=_stmttmp1->f2;_tmpB3=_stmttmp1->f3;_tmpB4=_stmttmp1->f4;_tmpB5=_stmttmp1->f5;_LL2: {int suppress=_tmpB1;struct Cyc_List_List*width=_tmpB2;struct Cyc_List_List*lenmod=_tmpB3;char c=_tmpB4;int j=_tmpB5;
# 473
i=j - 1;
{struct Cyc_List_List*_tmpB6=lenmod;int _tmpB7;if(_tmpB6 != 0){if(((struct Cyc_List_List*)_tmpB6)->tl == 0){_LL4: _tmpB7=(int)_tmpB6->hd;if(
(_tmpB7 == (int)'j' || _tmpB7 == (int)'z')|| _tmpB7 == (int)'t'){_LL5: {int x=_tmpB7;
# 477
({struct Cyc_Int_pa_PrintArg_struct _tmpBA=({struct Cyc_Int_pa_PrintArg_struct _tmp13A;_tmp13A.tag=1U,_tmp13A.f1=(unsigned long)x;_tmp13A;});void*_tmpB8[1U];_tmpB8[0]=& _tmpBA;({unsigned _tmp17F=loc;struct _fat_ptr _tmp17E=({const char*_tmpB9="length modifier '%c' is not supported";_tag_fat(_tmpB9,sizeof(char),38U);});Cyc_Tcutil_terr(_tmp17F,_tmp17E,_tag_fat(_tmpB8,sizeof(void*),1U));});});{
struct Cyc_List_List*_tmpBB=0;_npop_handler(0U);return _tmpBB;}}}else{goto _LL6;}}else{goto _LL6;}}else{_LL6: _LL7:
 goto _LL3;}_LL3:;}
# 481
if(suppress)continue;{void*t;
# 483
char _tmpBC=c;switch(_tmpBC){case 100U: _LL9: _LLA:
 goto _LLC;case 105U: _LLB: _LLC: {
# 486
struct Cyc_Core_Opt*tvs=({struct Cyc_Core_Opt*_tmpCD=_cycalloc(sizeof(*_tmpCD));({struct Cyc_List_List*_tmp180=({Cyc_Tcenv_lookup_type_vars(te);});_tmpCD->v=_tmp180;});_tmpCD;});
{struct Cyc_List_List*_tmpBD=lenmod;int _tmpBF;int _tmpBE;int _tmpC0;int _tmpC1;if(_tmpBD == 0){_LL34: _LL35:
 t=Cyc_Absyn_sint_type;goto _LL33;}else{if(((struct Cyc_List_List*)_tmpBD)->tl == 0){_LL36: _tmpC1=(int)_tmpBD->hd;if(_tmpC1 == (int)'l'){_LL37: {int x=_tmpC1;
t=Cyc_Absyn_slong_type;goto _LL33;}}else{_LL38: _tmpC0=(int)_tmpBD->hd;if(_tmpC0 == (int)'h'){_LL39: {int x=_tmpC0;
t=Cyc_Absyn_sshort_type;goto _LL33;}}else{goto _LL3C;}}}else{if(((struct Cyc_List_List*)((struct Cyc_List_List*)_tmpBD)->tl)->tl == 0){_LL3A: _tmpBE=(int)_tmpBD->hd;_tmpBF=(int)(_tmpBD->tl)->hd;if(
_tmpBE == (int)'h' && _tmpBF == (int)'h'){_LL3B: {int x=_tmpBE;int y=_tmpBF;t=Cyc_Absyn_schar_type;goto _LL33;}}else{goto _LL3C;}}else{_LL3C: _LL3D:
# 493
({struct Cyc_String_pa_PrintArg_struct _tmpC4=({struct Cyc_String_pa_PrintArg_struct _tmp13C;_tmp13C.tag=0U,({
struct _fat_ptr _tmp181=(struct _fat_ptr)((struct _fat_ptr)({Cyc_implode(lenmod);}));_tmp13C.f1=_tmp181;});_tmp13C;});struct Cyc_Int_pa_PrintArg_struct _tmpC5=({struct Cyc_Int_pa_PrintArg_struct _tmp13B;_tmp13B.tag=1U,_tmp13B.f1=(unsigned long)((int)c);_tmp13B;});void*_tmpC2[2U];_tmpC2[0]=& _tmpC4,_tmpC2[1]=& _tmpC5;({unsigned _tmp183=loc;struct _fat_ptr _tmp182=({const char*_tmpC3="length modifier '%s' is not allowed with %%%c";_tag_fat(_tmpC3,sizeof(char),46U);});Cyc_Tcutil_terr(_tmp183,_tmp182,_tag_fat(_tmpC2,sizeof(void*),2U));});});{
struct Cyc_List_List*_tmpC6=0;_npop_handler(0U);return _tmpC6;}}}}_LL33:;}
# 497
t=({void*(*_tmpC7)(void*t,void*rgn,struct Cyc_Absyn_Tqual,void*zero_term)=Cyc_Absyn_at_type;void*_tmpC8=t;void*_tmpC9=({Cyc_Absyn_new_evar(& Cyc_Tcutil_rko,tvs);});struct Cyc_Absyn_Tqual _tmpCA=({Cyc_Absyn_empty_tqual(0U);});void*_tmpCB=Cyc_Absyn_false_type;_tmpC7(_tmpC8,_tmpC9,_tmpCA,_tmpCB);});
typs=({struct Cyc_List_List*_tmpCC=_cycalloc(sizeof(*_tmpCC));_tmpCC->hd=t,_tmpCC->tl=typs;_tmpCC;});
goto _LL8;}case 117U: _LLD: _LLE:
 goto _LL10;case 111U: _LLF: _LL10:
 goto _LL12;case 120U: _LL11: _LL12:
 goto _LL14;case 88U: _LL13: _LL14: {
# 504
struct Cyc_Core_Opt*tvs=({struct Cyc_Core_Opt*_tmpDE=_cycalloc(sizeof(*_tmpDE));({struct Cyc_List_List*_tmp184=({Cyc_Tcenv_lookup_type_vars(te);});_tmpDE->v=_tmp184;});_tmpDE;});
{struct Cyc_List_List*_tmpCE=lenmod;int _tmpD0;int _tmpCF;int _tmpD1;int _tmpD2;if(_tmpCE == 0){_LL3F: _LL40:
 t=Cyc_Absyn_uint_type;goto _LL3E;}else{if(((struct Cyc_List_List*)_tmpCE)->tl == 0){_LL41: _tmpD2=(int)_tmpCE->hd;if(_tmpD2 == (int)'l'){_LL42: {int x=_tmpD2;
t=Cyc_Absyn_ulong_type;goto _LL3E;}}else{_LL43: _tmpD1=(int)_tmpCE->hd;if(_tmpD1 == (int)'h'){_LL44: {int x=_tmpD1;
t=Cyc_Absyn_ushort_type;goto _LL3E;}}else{goto _LL47;}}}else{if(((struct Cyc_List_List*)((struct Cyc_List_List*)_tmpCE)->tl)->tl == 0){_LL45: _tmpCF=(int)_tmpCE->hd;_tmpD0=(int)(_tmpCE->tl)->hd;if(
_tmpCF == (int)'h' && _tmpD0 == (int)'h'){_LL46: {int x=_tmpCF;int y=_tmpD0;t=Cyc_Absyn_uchar_type;goto _LL3E;}}else{goto _LL47;}}else{_LL47: _LL48:
# 511
({struct Cyc_String_pa_PrintArg_struct _tmpD5=({struct Cyc_String_pa_PrintArg_struct _tmp13E;_tmp13E.tag=0U,({
struct _fat_ptr _tmp185=(struct _fat_ptr)((struct _fat_ptr)({Cyc_implode(lenmod);}));_tmp13E.f1=_tmp185;});_tmp13E;});struct Cyc_Int_pa_PrintArg_struct _tmpD6=({struct Cyc_Int_pa_PrintArg_struct _tmp13D;_tmp13D.tag=1U,_tmp13D.f1=(unsigned long)((int)c);_tmp13D;});void*_tmpD3[2U];_tmpD3[0]=& _tmpD5,_tmpD3[1]=& _tmpD6;({unsigned _tmp187=loc;struct _fat_ptr _tmp186=({const char*_tmpD4="length modifier '%s' is not allowed with %%%c";_tag_fat(_tmpD4,sizeof(char),46U);});Cyc_Tcutil_terr(_tmp187,_tmp186,_tag_fat(_tmpD3,sizeof(void*),2U));});});{
struct Cyc_List_List*_tmpD7=0;_npop_handler(0U);return _tmpD7;}}}}_LL3E:;}
# 515
t=({void*(*_tmpD8)(void*t,void*rgn,struct Cyc_Absyn_Tqual,void*zero_term)=Cyc_Absyn_at_type;void*_tmpD9=t;void*_tmpDA=({Cyc_Absyn_new_evar(& Cyc_Tcutil_rko,tvs);});struct Cyc_Absyn_Tqual _tmpDB=({Cyc_Absyn_empty_tqual(0U);});void*_tmpDC=Cyc_Absyn_false_type;_tmpD8(_tmpD9,_tmpDA,_tmpDB,_tmpDC);});
typs=({struct Cyc_List_List*_tmpDD=_cycalloc(sizeof(*_tmpDD));_tmpDD->hd=t,_tmpDD->tl=typs;_tmpDD;});
goto _LL8;}case 102U: _LL15: _LL16:
 goto _LL18;case 70U: _LL17: _LL18:
 goto _LL1A;case 101U: _LL19: _LL1A:
 goto _LL1C;case 69U: _LL1B: _LL1C:
 goto _LL1E;case 103U: _LL1D: _LL1E:
 goto _LL20;case 71U: _LL1F: _LL20:
 goto _LL22;case 97U: _LL21: _LL22:
 goto _LL24;case 65U: _LL23: _LL24: {
# 526
struct Cyc_Core_Opt*tvs=({struct Cyc_Core_Opt*_tmpEC=_cycalloc(sizeof(*_tmpEC));({struct Cyc_List_List*_tmp188=({Cyc_Tcenv_lookup_type_vars(te);});_tmpEC->v=_tmp188;});_tmpEC;});
{struct Cyc_List_List*_tmpDF=lenmod;int _tmpE0;if(_tmpDF == 0){_LL4A: _LL4B:
 t=Cyc_Absyn_float_type;goto _LL49;}else{if(((struct Cyc_List_List*)_tmpDF)->tl == 0){_LL4C: _tmpE0=(int)_tmpDF->hd;if(_tmpE0 == (int)'l'){_LL4D: {int x=_tmpE0;
# 530
t=Cyc_Absyn_double_type;goto _LL49;}}else{goto _LL4E;}}else{_LL4E: _LL4F:
# 532
({struct Cyc_String_pa_PrintArg_struct _tmpE3=({struct Cyc_String_pa_PrintArg_struct _tmp140;_tmp140.tag=0U,({
struct _fat_ptr _tmp189=(struct _fat_ptr)((struct _fat_ptr)({Cyc_implode(lenmod);}));_tmp140.f1=_tmp189;});_tmp140;});struct Cyc_Int_pa_PrintArg_struct _tmpE4=({struct Cyc_Int_pa_PrintArg_struct _tmp13F;_tmp13F.tag=1U,_tmp13F.f1=(unsigned long)((int)c);_tmp13F;});void*_tmpE1[2U];_tmpE1[0]=& _tmpE3,_tmpE1[1]=& _tmpE4;({unsigned _tmp18B=loc;struct _fat_ptr _tmp18A=({const char*_tmpE2="length modifier '%s' is not allowed with %%%c";_tag_fat(_tmpE2,sizeof(char),46U);});Cyc_Tcutil_terr(_tmp18B,_tmp18A,_tag_fat(_tmpE1,sizeof(void*),2U));});});{
struct Cyc_List_List*_tmpE5=0;_npop_handler(0U);return _tmpE5;}}}_LL49:;}
# 536
t=({void*(*_tmpE6)(void*t,void*rgn,struct Cyc_Absyn_Tqual,void*zero_term)=Cyc_Absyn_at_type;void*_tmpE7=t;void*_tmpE8=({Cyc_Absyn_new_evar(& Cyc_Tcutil_rko,tvs);});struct Cyc_Absyn_Tqual _tmpE9=({Cyc_Absyn_empty_tqual(0U);});void*_tmpEA=Cyc_Absyn_false_type;_tmpE6(_tmpE7,_tmpE8,_tmpE9,_tmpEA);});
typs=({struct Cyc_List_List*_tmpEB=_cycalloc(sizeof(*_tmpEB));_tmpEB->hd=t,_tmpEB->tl=typs;_tmpEB;});
goto _LL8;}case 99U: _LL25: _LL26: {
# 541
struct Cyc_Core_Opt*tvs=({struct Cyc_Core_Opt*_tmpF8=_cycalloc(sizeof(*_tmpF8));({struct Cyc_List_List*_tmp18C=({Cyc_Tcenv_lookup_type_vars(te);});_tmpF8->v=_tmp18C;});_tmpF8;});
void*ptr;
if(!isCproto)
ptr=({void*(*_tmpED)(void*t,void*rgn,struct Cyc_Absyn_Tqual,void*zeroterm)=Cyc_Absyn_fatptr_type;void*_tmpEE=Cyc_Absyn_char_type;void*_tmpEF=({Cyc_Absyn_new_evar(& Cyc_Tcutil_rko,tvs);});struct Cyc_Absyn_Tqual _tmpF0=({Cyc_Absyn_empty_tqual(0U);});void*_tmpF1=Cyc_Absyn_false_type;_tmpED(_tmpEE,_tmpEF,_tmpF0,_tmpF1);});else{
# 548
ptr=({void*(*_tmpF2)(void*t,void*rgn,struct Cyc_Absyn_Tqual,void*zero_term)=Cyc_Absyn_at_type;void*_tmpF3=Cyc_Absyn_char_type;void*_tmpF4=({Cyc_Absyn_new_evar(& Cyc_Tcutil_rko,tvs);});struct Cyc_Absyn_Tqual _tmpF5=({Cyc_Absyn_empty_tqual(0U);});void*_tmpF6=Cyc_Absyn_false_type;_tmpF2(_tmpF3,_tmpF4,_tmpF5,_tmpF6);});}
# 551
typs=({struct Cyc_List_List*_tmpF7=_cycalloc(sizeof(*_tmpF7));_tmpF7->hd=ptr,_tmpF7->tl=typs;_tmpF7;});
goto _LL8;}case 115U: _LL27: _LL28: {
# 554
struct Cyc_Core_Opt*tvs=({struct Cyc_Core_Opt*_tmp104=_cycalloc(sizeof(*_tmp104));({struct Cyc_List_List*_tmp18D=({Cyc_Tcenv_lookup_type_vars(te);});_tmp104->v=_tmp18D;});_tmp104;});
# 556
void*ptr;
if(!isCproto)
ptr=({void*(*_tmpF9)(void*t,void*rgn,struct Cyc_Absyn_Tqual,void*zeroterm)=Cyc_Absyn_fatptr_type;void*_tmpFA=Cyc_Absyn_char_type;void*_tmpFB=({Cyc_Absyn_new_evar(& Cyc_Tcutil_rko,tvs);});struct Cyc_Absyn_Tqual _tmpFC=({Cyc_Absyn_empty_tqual(0U);});void*_tmpFD=Cyc_Absyn_false_type;_tmpF9(_tmpFA,_tmpFB,_tmpFC,_tmpFD);});else{
# 561
ptr=({void*(*_tmpFE)(void*t,void*rgn,struct Cyc_Absyn_Tqual,void*zero_term)=Cyc_Absyn_at_type;void*_tmpFF=Cyc_Absyn_char_type;void*_tmp100=({Cyc_Absyn_new_evar(& Cyc_Tcutil_rko,tvs);});struct Cyc_Absyn_Tqual _tmp101=({Cyc_Absyn_empty_tqual(0U);});void*_tmp102=Cyc_Absyn_true_type;_tmpFE(_tmpFF,_tmp100,_tmp101,_tmp102);});}
# 563
typs=({struct Cyc_List_List*_tmp103=_cycalloc(sizeof(*_tmp103));_tmp103->hd=ptr,_tmp103->tl=typs;_tmp103;});
goto _LL8;}case 91U: _LL29: _LL2A:
 goto _LL2C;case 112U: _LL2B: _LL2C:
# 567
({struct Cyc_Int_pa_PrintArg_struct _tmp107=({struct Cyc_Int_pa_PrintArg_struct _tmp141;_tmp141.tag=1U,_tmp141.f1=(unsigned long)((int)c);_tmp141;});void*_tmp105[1U];_tmp105[0]=& _tmp107;({unsigned _tmp18F=loc;struct _fat_ptr _tmp18E=({const char*_tmp106="%%%c is not supported";_tag_fat(_tmp106,sizeof(char),22U);});Cyc_Tcutil_terr(_tmp18F,_tmp18E,_tag_fat(_tmp105,sizeof(void*),1U));});});{
struct Cyc_List_List*_tmp108=0;_npop_handler(0U);return _tmp108;}case 110U: _LL2D: _LL2E: {
# 570
struct Cyc_Core_Opt*tvs=({struct Cyc_Core_Opt*_tmp119=_cycalloc(sizeof(*_tmp119));({struct Cyc_List_List*_tmp190=({Cyc_Tcenv_lookup_type_vars(te);});_tmp119->v=_tmp190;});_tmp119;});
{struct Cyc_List_List*_tmp109=lenmod;int _tmp10B;int _tmp10A;int _tmp10C;int _tmp10D;if(_tmp109 == 0){_LL51: _LL52:
 t=Cyc_Absyn_sint_type;goto _LL50;}else{if(((struct Cyc_List_List*)_tmp109)->tl == 0){_LL53: _tmp10D=(int)_tmp109->hd;if(_tmp10D == (int)'l'){_LL54: {int x=_tmp10D;
t=Cyc_Absyn_ulong_type;goto _LL50;}}else{_LL55: _tmp10C=(int)_tmp109->hd;if(_tmp10C == (int)'h'){_LL56: {int x=_tmp10C;
t=Cyc_Absyn_sshort_type;goto _LL50;}}else{goto _LL59;}}}else{if(((struct Cyc_List_List*)((struct Cyc_List_List*)_tmp109)->tl)->tl == 0){_LL57: _tmp10A=(int)_tmp109->hd;_tmp10B=(int)(_tmp109->tl)->hd;if(
_tmp10A == (int)'h' && _tmp10B == (int)'h'){_LL58: {int x=_tmp10A;int y=_tmp10B;t=Cyc_Absyn_schar_type;goto _LL50;}}else{goto _LL59;}}else{_LL59: _LL5A:
# 577
({struct Cyc_String_pa_PrintArg_struct _tmp110=({struct Cyc_String_pa_PrintArg_struct _tmp143;_tmp143.tag=0U,({
struct _fat_ptr _tmp191=(struct _fat_ptr)((struct _fat_ptr)({Cyc_implode(lenmod);}));_tmp143.f1=_tmp191;});_tmp143;});struct Cyc_Int_pa_PrintArg_struct _tmp111=({struct Cyc_Int_pa_PrintArg_struct _tmp142;_tmp142.tag=1U,_tmp142.f1=(unsigned long)((int)c);_tmp142;});void*_tmp10E[2U];_tmp10E[0]=& _tmp110,_tmp10E[1]=& _tmp111;({unsigned _tmp193=loc;struct _fat_ptr _tmp192=({const char*_tmp10F="length modifier '%s' is not allowed with %%%c";_tag_fat(_tmp10F,sizeof(char),46U);});Cyc_Tcutil_terr(_tmp193,_tmp192,_tag_fat(_tmp10E,sizeof(void*),2U));});});{
struct Cyc_List_List*_tmp112=0;_npop_handler(0U);return _tmp112;}}}}_LL50:;}
# 581
t=({void*(*_tmp113)(void*t,void*rgn,struct Cyc_Absyn_Tqual,void*zero_term)=Cyc_Absyn_at_type;void*_tmp114=t;void*_tmp115=({Cyc_Absyn_new_evar(& Cyc_Tcutil_rko,tvs);});struct Cyc_Absyn_Tqual _tmp116=({Cyc_Absyn_empty_tqual(0U);});void*_tmp117=Cyc_Absyn_false_type;_tmp113(_tmp114,_tmp115,_tmp116,_tmp117);});
typs=({struct Cyc_List_List*_tmp118=_cycalloc(sizeof(*_tmp118));_tmp118->hd=t,_tmp118->tl=typs;_tmp118;});
goto _LL8;}case 37U: _LL2F: _LL30:
# 585
 if(suppress){
({void*_tmp11A=0U;({unsigned _tmp195=loc;struct _fat_ptr _tmp194=({const char*_tmp11B="Assignment suppression (*) is not allowed with %%%%";_tag_fat(_tmp11B,sizeof(char),52U);});Cyc_Tcutil_terr(_tmp195,_tmp194,_tag_fat(_tmp11A,sizeof(void*),0U));});});{
struct Cyc_List_List*_tmp11C=0;_npop_handler(0U);return _tmp11C;}}
# 585
if(width != 0){
# 590
({struct Cyc_String_pa_PrintArg_struct _tmp11F=({struct Cyc_String_pa_PrintArg_struct _tmp144;_tmp144.tag=0U,({struct _fat_ptr _tmp196=(struct _fat_ptr)((struct _fat_ptr)({Cyc_implode(width);}));_tmp144.f1=_tmp196;});_tmp144;});void*_tmp11D[1U];_tmp11D[0]=& _tmp11F;({unsigned _tmp198=loc;struct _fat_ptr _tmp197=({const char*_tmp11E="width '%s' not allowed with %%%%";_tag_fat(_tmp11E,sizeof(char),33U);});Cyc_Tcutil_terr(_tmp198,_tmp197,_tag_fat(_tmp11D,sizeof(void*),1U));});});{
struct Cyc_List_List*_tmp120=0;_npop_handler(0U);return _tmp120;}}
# 585
if(lenmod != 0){
# 594
({struct Cyc_String_pa_PrintArg_struct _tmp123=({struct Cyc_String_pa_PrintArg_struct _tmp145;_tmp145.tag=0U,({struct _fat_ptr _tmp199=(struct _fat_ptr)((struct _fat_ptr)({Cyc_implode(lenmod);}));_tmp145.f1=_tmp199;});_tmp145;});void*_tmp121[1U];_tmp121[0]=& _tmp123;({unsigned _tmp19B=loc;struct _fat_ptr _tmp19A=({const char*_tmp122="length modifier '%s' not allowed with %%%%";_tag_fat(_tmp122,sizeof(char),43U);});Cyc_Tcutil_terr(_tmp19B,_tmp19A,_tag_fat(_tmp121,sizeof(void*),1U));});});{
struct Cyc_List_List*_tmp124=0;_npop_handler(0U);return _tmp124;}}
# 585
goto _LL8;default: _LL31: _LL32: {
# 599
struct Cyc_List_List*_tmp125=0;_npop_handler(0U);return _tmp125;}}_LL8:;}}}}}
# 465
;_pop_region();}
# 603
return({Cyc_List_imp_rev(typs);});}

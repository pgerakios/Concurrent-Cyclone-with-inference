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
# 165 "core.h"
extern struct _RegionHandle*Cyc_Core_heap_region;
# 168
extern struct _RegionHandle*Cyc_Core_unique_region;struct Cyc_Core_DynamicRegion;struct Cyc_Core_NewDynamicRegion{struct Cyc_Core_DynamicRegion*key;};struct Cyc_Core_ThinRes{void*arr;unsigned nelts;};struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};extern char Cyc_List_List_mismatch[14U];struct Cyc_List_List_mismatch_exn_struct{char*tag;};extern char Cyc_List_Nth[4U];struct Cyc_List_Nth_exn_struct{char*tag;};struct Cyc_Iter_Iter{void*env;int(*next)(void*env,void*dest);};struct Cyc_Set_Set;extern char Cyc_Set_Absent[7U];struct Cyc_Set_Absent_exn_struct{char*tag;};struct Cyc___cycFILE;
# 53 "cycboot.h"
extern struct Cyc___cycFILE*Cyc_stderr;struct Cyc_String_pa_PrintArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Int_pa_PrintArg_struct{int tag;unsigned long f1;};struct Cyc_Double_pa_PrintArg_struct{int tag;double f1;};struct Cyc_LongDouble_pa_PrintArg_struct{int tag;long double f1;};struct Cyc_ShortPtr_pa_PrintArg_struct{int tag;short*f1;};struct Cyc_IntPtr_pa_PrintArg_struct{int tag;unsigned long*f1;};
# 100
int Cyc_fprintf(struct Cyc___cycFILE*,struct _fat_ptr,struct _fat_ptr);struct Cyc_ShortPtr_sa_ScanfArg_struct{int tag;short*f1;};struct Cyc_UShortPtr_sa_ScanfArg_struct{int tag;unsigned short*f1;};struct Cyc_IntPtr_sa_ScanfArg_struct{int tag;int*f1;};struct Cyc_UIntPtr_sa_ScanfArg_struct{int tag;unsigned*f1;};struct Cyc_StringPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_DoublePtr_sa_ScanfArg_struct{int tag;double*f1;};struct Cyc_FloatPtr_sa_ScanfArg_struct{int tag;float*f1;};struct Cyc_CharPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};extern char Cyc_FileCloseError[15U];struct Cyc_FileCloseError_exn_struct{char*tag;};extern char Cyc_FileOpenError[14U];struct Cyc_FileOpenError_exn_struct{char*tag;struct _fat_ptr f1;};struct Cyc_Dict_T;struct Cyc_Dict_Dict{int(*rel)(void*,void*);struct _RegionHandle*r;const struct Cyc_Dict_T*t;};extern char Cyc_Dict_Present[8U];struct Cyc_Dict_Present_exn_struct{char*tag;};extern char Cyc_Dict_Absent[7U];struct Cyc_Dict_Absent_exn_struct{char*tag;};struct Cyc_Position_Error;struct Cyc_Relations_Reln;struct _union_Nmspace_Rel_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Abs_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_C_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Loc_n{int tag;int val;};union Cyc_Absyn_Nmspace{struct _union_Nmspace_Rel_n Rel_n;struct _union_Nmspace_Abs_n Abs_n;struct _union_Nmspace_C_n C_n;struct _union_Nmspace_Loc_n Loc_n;};struct _tuple0{union Cyc_Absyn_Nmspace f1;struct _fat_ptr*f2;};
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
# 1026 "absyn.h"
void*Cyc_Absyn_bounds_one();
# 1068
struct Cyc_Absyn_Exp*Cyc_Absyn_uint_exp(unsigned,unsigned);
# 1285
int Cyc_Absyn_get_debug();struct _union_RelnOp_RConst{int tag;unsigned val;};struct _union_RelnOp_RVar{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RNumelts{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RType{int tag;void*val;};struct _union_RelnOp_RParam{int tag;unsigned val;};struct _union_RelnOp_RParamNumelts{int tag;unsigned val;};struct _union_RelnOp_RReturn{int tag;unsigned val;};union Cyc_Relations_RelnOp{struct _union_RelnOp_RConst RConst;struct _union_RelnOp_RVar RVar;struct _union_RelnOp_RNumelts RNumelts;struct _union_RelnOp_RType RType;struct _union_RelnOp_RParam RParam;struct _union_RelnOp_RParamNumelts RParamNumelts;struct _union_RelnOp_RReturn RReturn;};
# 41 "relations-ap.h"
union Cyc_Relations_RelnOp Cyc_Relations_RConst(unsigned);union Cyc_Relations_RelnOp Cyc_Relations_RNumelts(struct Cyc_Absyn_Vardecl*);
# 50
enum Cyc_Relations_Relation{Cyc_Relations_Req =0U,Cyc_Relations_Rneq =1U,Cyc_Relations_Rlte =2U,Cyc_Relations_Rlt =3U};struct Cyc_Relations_Reln{union Cyc_Relations_RelnOp rop1;enum Cyc_Relations_Relation relation;union Cyc_Relations_RelnOp rop2;};
# 76
int Cyc_Relations_exp2relnop(struct Cyc_Absyn_Exp*e,union Cyc_Relations_RelnOp*p);
# 87
struct Cyc_List_List*Cyc_Relations_add_relation(struct _RegionHandle*rgn,union Cyc_Relations_RelnOp rop1,enum Cyc_Relations_Relation r,union Cyc_Relations_RelnOp rop2,struct Cyc_List_List*relns);
# 118
void Cyc_Relations_print_relns(struct Cyc___cycFILE*,struct Cyc_List_List*);
# 129
int Cyc_Relations_consistent_relations(struct Cyc_List_List*rlns);
# 41 "cf_flowinfo.h"
int Cyc_CfFlowInfo_anal_error;struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_CfFlowInfo_MallocPt_CfFlowInfo_Root_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;};struct Cyc_CfFlowInfo_InitParam_CfFlowInfo_Root_struct{int tag;int f1;void*f2;};struct Cyc_CfFlowInfo_Dot_CfFlowInfo_PathCon_struct{int tag;int f1;};struct Cyc_CfFlowInfo_Star_CfFlowInfo_PathCon_struct{int tag;};struct Cyc_CfFlowInfo_Place{void*root;struct Cyc_List_List*path;};
# 74
enum Cyc_CfFlowInfo_InitLevel{Cyc_CfFlowInfo_NoneIL =0U,Cyc_CfFlowInfo_AllIL =1U};extern char Cyc_CfFlowInfo_IsZero[7U];struct Cyc_CfFlowInfo_IsZero_Absyn_AbsynAnnot_struct{char*tag;};extern char Cyc_CfFlowInfo_NotZero[8U];struct Cyc_CfFlowInfo_NotZero_Absyn_AbsynAnnot_struct{char*tag;struct Cyc_List_List*f1;};extern char Cyc_CfFlowInfo_UnknownZ[9U];struct Cyc_CfFlowInfo_UnknownZ_Absyn_AbsynAnnot_struct{char*tag;struct Cyc_List_List*f1;};struct _union_AbsLVal_PlaceL{int tag;struct Cyc_CfFlowInfo_Place*val;};struct _union_AbsLVal_UnknownL{int tag;int val;};union Cyc_CfFlowInfo_AbsLVal{struct _union_AbsLVal_PlaceL PlaceL;struct _union_AbsLVal_UnknownL UnknownL;};struct Cyc_CfFlowInfo_UnionRInfo{int is_union;int fieldnum;};struct Cyc_CfFlowInfo_Zero_CfFlowInfo_AbsRVal_struct{int tag;};struct Cyc_CfFlowInfo_NotZeroAll_CfFlowInfo_AbsRVal_struct{int tag;};struct Cyc_CfFlowInfo_UnknownR_CfFlowInfo_AbsRVal_struct{int tag;enum Cyc_CfFlowInfo_InitLevel f1;};struct Cyc_CfFlowInfo_Esc_CfFlowInfo_AbsRVal_struct{int tag;enum Cyc_CfFlowInfo_InitLevel f1;};struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct{int tag;struct Cyc_CfFlowInfo_Place*f1;};struct Cyc_CfFlowInfo_UniquePtr_CfFlowInfo_AbsRVal_struct{int tag;void*f1;};struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct{int tag;struct Cyc_CfFlowInfo_UnionRInfo f1;struct _fat_ptr f2;};struct Cyc_CfFlowInfo_Consumed_CfFlowInfo_AbsRVal_struct{int tag;struct Cyc_Absyn_Exp*f1;int f2;void*f3;};struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct{int tag;struct Cyc_Absyn_Vardecl*f1;void*f2;};struct _union_FlowInfo_BottomFL{int tag;int val;};struct _tuple12{struct Cyc_Dict_Dict f1;struct Cyc_List_List*f2;};struct _union_FlowInfo_ReachableFL{int tag;struct _tuple12 val;};union Cyc_CfFlowInfo_FlowInfo{struct _union_FlowInfo_BottomFL BottomFL;struct _union_FlowInfo_ReachableFL ReachableFL;};struct Cyc_CfFlowInfo_FlowEnv{void*zero;void*notzeroall;void*unknown_none;void*unknown_all;void*esc_none;void*esc_all;struct Cyc_Dict_Dict mt_flowdict;struct Cyc_CfFlowInfo_Place*dummy_place;};struct Cyc_RgnOrder_RgnPO;extern char Cyc_Tcenv_Env_error[10U];struct Cyc_Tcenv_Env_error_exn_struct{char*tag;};struct Cyc_Tcenv_Genv{struct Cyc_Dict_Dict aggrdecls;struct Cyc_Dict_Dict datatypedecls;struct Cyc_Dict_Dict enumdecls;struct Cyc_Dict_Dict typedefs;struct Cyc_Dict_Dict ordinaries;};struct Cyc_Tcenv_Fenv;struct Cyc_Tcenv_Tenv{struct Cyc_List_List*ns;struct Cyc_Tcenv_Genv*ae;struct Cyc_Tcenv_Fenv*le;int allow_valueof: 1;int in_extern_c_include: 1;int in_tempest: 1;int tempest_generalize: 1;};
# 90 "tcenv.h"
enum Cyc_Tcenv_NewStatus{Cyc_Tcenv_NoneNew =0U,Cyc_Tcenv_InNew =1U,Cyc_Tcenv_InNewAggr =2U};
# 32 "tcutil.h"
void Cyc_Tcutil_warn(unsigned,struct _fat_ptr,struct _fat_ptr);
# 53
int Cyc_Tcutil_is_zeroterm_pointer_type(void*);
# 75
struct Cyc_Absyn_Exp*Cyc_Tcutil_get_bounds_exp(void*def,void*b);
# 78
struct Cyc_Absyn_Exp*Cyc_Tcutil_get_type_bound(void*);
# 87
struct Cyc_Absyn_Exp*Cyc_Tcutil_get_bounds_exp(void*def,void*b);
# 110
void*Cyc_Tcutil_compress(void*);
# 296
struct Cyc_Absyn_Vardecl*Cyc_Tcutil_nonesc_vardecl(void*);
# 310
int Cyc_Tcutil_force_type2bool(int desired,void*);
# 32 "evexp.h"
int Cyc_Evexp_c_can_eval(struct Cyc_Absyn_Exp*e);
# 42 "evexp.h"
int Cyc_Evexp_lte_const_exp(struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2);
# 35 "warn.h"
void Cyc_Warn_err(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap);
# 46
void*Cyc_Warn_impos_loc(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap);struct Cyc_Warn_String_Warn_Warg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Warn_Exp_Warn_Warg_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Warn_Typ_Warn_Warg_struct{int tag;void*f1;};struct Cyc_Warn_Qvar_Warn_Warg_struct{int tag;struct _tuple0*f1;};struct Cyc_Hashtable_Table;struct Cyc_JumpAnalysis_Jump_Anal_Result{struct Cyc_Hashtable_Table*pop_tables;struct Cyc_Hashtable_Table*succ_tables;struct Cyc_Hashtable_Table*pat_pop_tables;};extern char Cyc_InsertChecks_FatBound[9U];struct Cyc_InsertChecks_FatBound_Absyn_AbsynAnnot_struct{char*tag;};extern char Cyc_InsertChecks_NoCheck[8U];struct Cyc_InsertChecks_NoCheck_Absyn_AbsynAnnot_struct{char*tag;};extern char Cyc_InsertChecks_NullAndFatBound[16U];struct Cyc_InsertChecks_NullAndFatBound_Absyn_AbsynAnnot_struct{char*tag;};extern char Cyc_InsertChecks_NullAndThinBound[17U];struct Cyc_InsertChecks_NullAndThinBound_Absyn_AbsynAnnot_struct{char*tag;struct Cyc_Absyn_Exp*f1;};extern char Cyc_InsertChecks_NullOnly[9U];struct Cyc_InsertChecks_NullOnly_Absyn_AbsynAnnot_struct{char*tag;};extern char Cyc_InsertChecks_ThinBound[10U];struct Cyc_InsertChecks_ThinBound_Absyn_AbsynAnnot_struct{char*tag;struct Cyc_Absyn_Exp*f1;};
# 35 "insert_checks.h"
extern struct Cyc_InsertChecks_NoCheck_Absyn_AbsynAnnot_struct Cyc_InsertChecks_NoCheck_val;
extern struct Cyc_InsertChecks_NullOnly_Absyn_AbsynAnnot_struct Cyc_InsertChecks_NullOnly_val;
extern struct Cyc_InsertChecks_NullAndFatBound_Absyn_AbsynAnnot_struct Cyc_InsertChecks_NullAndFatBound_val;
extern struct Cyc_InsertChecks_FatBound_Absyn_AbsynAnnot_struct Cyc_InsertChecks_FatBound_val;
# 40
extern int Cyc_InsertChecks_warn_bounds_checks;
extern int Cyc_InsertChecks_warn_all_null_deref;struct Cyc_PP_Ppstate;struct Cyc_PP_Out;struct Cyc_PP_Doc;struct Cyc_Absynpp_Params{int expand_typedefs;int qvar_to_Cids;int add_cyc_prefix;int to_VC;int decls_first;int rewrite_temp_tvars;int print_all_tvars;int print_all_kinds;int print_all_effects;int print_using_stmts;int print_externC_stmts;int print_full_evars;int print_zeroterm;int generate_line_directives;int use_curr_namespace;struct Cyc_List_List*curr_namespace;};
# 62 "absynpp.h"
struct _fat_ptr Cyc_Absynpp_typ2string(void*);
# 67
struct _fat_ptr Cyc_Absynpp_exp2string(struct Cyc_Absyn_Exp*);char Cyc_InsertChecks_FatBound[9U]="FatBound";char Cyc_InsertChecks_NoCheck[8U]="NoCheck";char Cyc_InsertChecks_NullAndFatBound[16U]="NullAndFatBound";char Cyc_InsertChecks_NullAndThinBound[17U]="NullAndThinBound";char Cyc_InsertChecks_NullOnly[9U]="NullOnly";char Cyc_InsertChecks_ThinBound[10U]="ThinBound";
# 62 "insert_checks.cyc"
struct Cyc_InsertChecks_NoCheck_Absyn_AbsynAnnot_struct Cyc_InsertChecks_NoCheck_val={Cyc_InsertChecks_NoCheck};
struct Cyc_InsertChecks_NullOnly_Absyn_AbsynAnnot_struct Cyc_InsertChecks_NullOnly_val={Cyc_InsertChecks_NullOnly};
struct Cyc_InsertChecks_NullAndFatBound_Absyn_AbsynAnnot_struct Cyc_InsertChecks_NullAndFatBound_val={Cyc_InsertChecks_NullAndFatBound};
struct Cyc_InsertChecks_FatBound_Absyn_AbsynAnnot_struct Cyc_InsertChecks_FatBound_val={Cyc_InsertChecks_FatBound};
# 68
int Cyc_InsertChecks_warn_bounds_checks=0;
int Cyc_InsertChecks_warn_all_null_deref=0;
unsigned Cyc_InsertChecks_total_bounds_checks=0U;
unsigned Cyc_InsertChecks_bounds_checks_eliminated=0U;
# 73
static struct Cyc_Absyn_Exp*Cyc_InsertChecks_shared_zero_exp(){
static struct Cyc_Absyn_Exp*ans=0;
if(!((unsigned)ans))
ans=({Cyc_Absyn_uint_exp(0U,0U);});
# 75
return(struct Cyc_Absyn_Exp*)_check_null(ans);}struct Cyc_InsertChecks_Env{int in_sizeof: 1;int at_toplevel: 1;struct Cyc_Absyn_Fndecl*fd;};
# 86
static struct Cyc_InsertChecks_Env Cyc_InsertChecks_toplevel_env(){
return({struct Cyc_InsertChecks_Env _tmpCB;_tmpCB.in_sizeof=0,_tmpCB.at_toplevel=1,_tmpCB.fd=0;_tmpCB;});}
static struct Cyc_InsertChecks_Env Cyc_InsertChecks_fn_body_env(struct Cyc_Absyn_Fndecl*fd){
return({struct Cyc_InsertChecks_Env _tmpCC;_tmpCC.in_sizeof=0,_tmpCC.at_toplevel=0,_tmpCC.fd=fd;_tmpCC;});}
# 91
static struct Cyc_InsertChecks_Env Cyc_InsertChecks_enter_sizeof(struct Cyc_InsertChecks_Env nv){
return({struct Cyc_InsertChecks_Env _tmpCD;_tmpCD.in_sizeof=1,_tmpCD.at_toplevel=nv.at_toplevel,_tmpCD.fd=0;_tmpCD;});}
# 96
static int Cyc_InsertChecks_need_null_check(struct Cyc_InsertChecks_Env nv,struct Cyc_Absyn_Exp*e){
if(nv.in_sizeof)
return 0;
# 97
{void*_stmttmp0=({Cyc_Tcutil_compress((void*)_check_null(e->topt));});void*_tmp4=_stmttmp0;void*_tmp7;void*_tmp6;void*_tmp5;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp4)->tag == 3U){_LL1: _tmp5=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp4)->f1).elt_type;_tmp6=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp4)->f1).ptr_atts).rgn;_tmp7=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp4)->f1).ptr_atts).nullable;_LL2: {void*tp=_tmp5;void*rgn=_tmp6;void*n=_tmp7;
# 102
if(!({Cyc_Tcutil_force_type2bool(0,n);}))
return 0;
# 102
goto _LL0;}}else{_LL3: _LL4:
# 112
({void*_tmp8=0U;({int(*_tmpD5)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmpA)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos_loc;_tmpA;});unsigned _tmpD4=e->loc;struct _fat_ptr _tmpD3=({const char*_tmp9="need_null_check: non-pointer type";_tag_fat(_tmp9,sizeof(char),34U);});_tmpD5(_tmpD4,_tmpD3,_tag_fat(_tmp8,sizeof(void*),0U));});});}_LL0:;}
# 114
{void*_stmttmp1=e->annot;void*_tmpB=_stmttmp1;if(((struct Cyc_CfFlowInfo_UnknownZ_Absyn_AbsynAnnot_struct*)_tmpB)->tag == Cyc_CfFlowInfo_UnknownZ){_LL6: _LL7:
 goto _LL5;}else{if(((struct Cyc_CfFlowInfo_NotZero_Absyn_AbsynAnnot_struct*)_tmpB)->tag == Cyc_CfFlowInfo_NotZero){_LL8: _LL9:
 return 0;}else{if(((struct Cyc_CfFlowInfo_IsZero_Absyn_AbsynAnnot_struct*)_tmpB)->tag == Cyc_CfFlowInfo_IsZero){_LLA: _LLB:
# 118
({void*_tmpC=0U;({unsigned _tmpD7=e->loc;struct _fat_ptr _tmpD6=({const char*_tmpD="NULL pointer check will definitely fail";_tag_fat(_tmpD,sizeof(char),40U);});Cyc_Warn_err(_tmpD7,_tmpD6,_tag_fat(_tmpC,sizeof(void*),0U));});});
return 0;}else{if(((struct Cyc_Absyn_EmptyAnnot_Absyn_AbsynAnnot_struct*)_tmpB)->tag == Cyc_Absyn_EmptyAnnot){_LLC: _LLD:
# 121
 return 0;}else{_LLE: _LLF:
({void*_tmpE=0U;({int(*_tmpDA)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp10)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos_loc;_tmp10;});unsigned _tmpD9=e->loc;struct _fat_ptr _tmpD8=({const char*_tmpF="need_null_check: unexpected annotation";_tag_fat(_tmpF,sizeof(char),39U);});_tmpDA(_tmpD9,_tmpD8,_tag_fat(_tmpE,sizeof(void*),0U));});});}}}}_LL5:;}
# 124
if(nv.at_toplevel){
({void*_tmp11=0U;({unsigned _tmpDC=e->loc;struct _fat_ptr _tmpDB=({const char*_tmp12="cannot perform NULL-pointer check at toplevel";_tag_fat(_tmp12,sizeof(char),46U);});Cyc_Warn_err(_tmpDC,_tmpDB,_tag_fat(_tmp11,sizeof(void*),0U));});});
return 0;}
# 124
if(Cyc_InsertChecks_warn_all_null_deref)
# 129
({void*_tmp13=0U;({unsigned _tmpDE=e->loc;struct _fat_ptr _tmpDD=({const char*_tmp14="inserted null check";_tag_fat(_tmp14,sizeof(char),20U);});Cyc_Tcutil_warn(_tmpDE,_tmpDD,_tag_fat(_tmp13,sizeof(void*),0U));});});
# 124
return 1;}
# 133
static struct Cyc_List_List*Cyc_InsertChecks_get_relns(struct Cyc_Absyn_Exp*e){
void*_stmttmp2=e->annot;void*_tmp16=_stmttmp2;struct Cyc_List_List*_tmp17;struct Cyc_List_List*_tmp18;if(((struct Cyc_CfFlowInfo_UnknownZ_Absyn_AbsynAnnot_struct*)_tmp16)->tag == Cyc_CfFlowInfo_UnknownZ){_LL1: _tmp18=((struct Cyc_CfFlowInfo_UnknownZ_Absyn_AbsynAnnot_struct*)_tmp16)->f1;_LL2: {struct Cyc_List_List*r=_tmp18;
return r;}}else{if(((struct Cyc_CfFlowInfo_NotZero_Absyn_AbsynAnnot_struct*)_tmp16)->tag == Cyc_CfFlowInfo_NotZero){_LL3: _tmp17=((struct Cyc_CfFlowInfo_NotZero_Absyn_AbsynAnnot_struct*)_tmp16)->f1;_LL4: {struct Cyc_List_List*r=_tmp17;
return r;}}else{if(((struct Cyc_CfFlowInfo_IsZero_Absyn_AbsynAnnot_struct*)_tmp16)->tag == Cyc_CfFlowInfo_IsZero){_LL5: _LL6:
 return 0;}else{if(((struct Cyc_Absyn_EmptyAnnot_Absyn_AbsynAnnot_struct*)_tmp16)->tag == Cyc_Absyn_EmptyAnnot){_LL7: _LL8:
 return 0;}else{_LL9: _LLA:
({void*_tmp19=0U;({int(*_tmpE1)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp1B)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos_loc;_tmp1B;});unsigned _tmpE0=e->loc;struct _fat_ptr _tmpDF=({const char*_tmp1A="get_relns: unexpected annotation";_tag_fat(_tmp1A,sizeof(char),33U);});_tmpE1(_tmpE0,_tmpDF,_tag_fat(_tmp19,sizeof(void*),0U));});});}}}}_LL0:;}
# 146
static int Cyc_InsertChecks_need_bounds_check(struct Cyc_InsertChecks_Env nv,struct Cyc_List_List*relns,struct Cyc_Absyn_Exp*a,struct Cyc_Absyn_Exp*i){
if(nv.in_sizeof)
return 0;
# 147
if(nv.at_toplevel)
# 150
({void*_tmp1D=0U;({int(*_tmpE4)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp1F)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos_loc;_tmp1F;});unsigned _tmpE3=a->loc;struct _fat_ptr _tmpE2=({const char*_tmp1E="InsertChecks: memory read at top-level";_tag_fat(_tmp1E,sizeof(char),39U);});_tmpE4(_tmpE3,_tmpE2,_tag_fat(_tmp1D,sizeof(void*),0U));});});
# 147
++ Cyc_InsertChecks_total_bounds_checks;{
# 158
int bd_valid=0;union Cyc_Relations_RelnOp rop_bd=({Cyc_Relations_RConst(0U);});
int a_valid=0;union Cyc_Relations_RelnOp rop_a=({Cyc_Relations_RConst(0U);});
int i_valid=0;union Cyc_Relations_RelnOp rop_i=({Cyc_Relations_RConst(0U);});
int z_valid=0;union Cyc_Relations_RelnOp rop_z=({Cyc_Relations_RConst(0U);});
# 164
struct Cyc_Absyn_Exp*bound=({Cyc_Tcutil_get_type_bound((void*)_check_null(a->topt));});
if(bound != 0)
bd_valid=({Cyc_Relations_exp2relnop(bound,& rop_bd);});
# 165
{void*_stmttmp3=a->r;void*_tmp20=_stmttmp3;void*_tmp21;if(((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp20)->tag == 1U){_LL1: _tmp21=(void*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp20)->f1;_LL2: {void*b=_tmp21;
# 170
struct Cyc_Absyn_Vardecl*x=({Cyc_Tcutil_nonesc_vardecl(b);});
if(x != 0){
a_valid=1;
rop_a=({Cyc_Relations_RNumelts(x);});}
# 171
goto _LL0;}}else{_LL3: _LL4:
# 176
 goto _LL0;}_LL0:;}
# 179
i_valid=({Cyc_Relations_exp2relnop(i,& rop_i);});
# 181
if(!i_valid){
inner_loop: {
void*_stmttmp4=i->r;void*_tmp22=_stmttmp4;struct Cyc_Absyn_Exp*_tmp24;struct Cyc_Absyn_Exp*_tmp23;struct Cyc_Absyn_Exp*_tmp26;void*_tmp25;switch(*((int*)_tmp22)){case 14U: _LL6: _tmp25=(void*)((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp22)->f1;_tmp26=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp22)->f2;_LL7: {void*t=_tmp25;struct Cyc_Absyn_Exp*e2=_tmp26;
i=e2;goto inner_loop;}case 3U: if(((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp22)->f1 == Cyc_Absyn_Mod){if(((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp22)->f2 != 0){if(((struct Cyc_List_List*)((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp22)->f2)->tl != 0){_LL8: _tmp23=(struct Cyc_Absyn_Exp*)(((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp22)->f2)->hd;_tmp24=(struct Cyc_Absyn_Exp*)((((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp22)->f2)->tl)->hd;_LL9: {struct Cyc_Absyn_Exp*e1=_tmp23;struct Cyc_Absyn_Exp*e2=_tmp24;
# 186
z_valid=({Cyc_Relations_exp2relnop(e2,& rop_z);});goto _LL5;}}else{goto _LLA;}}else{goto _LLA;}}else{goto _LLA;}default: _LLA: _LLB:
 goto _LL5;}_LL5:;}}{
# 181
union Cyc_Relations_RelnOp rop_left;
# 196
if(!bd_valid && !a_valid)
return 1;
# 196
if(
# 198
bd_valid && a_valid){
relns=({Cyc_Relations_add_relation(Cyc_Core_heap_region,rop_bd,Cyc_Relations_Rlte,rop_a,relns);});
rop_left=rop_a;}else{
if(a_valid)
rop_left=rop_a;else{
# 204
rop_left=rop_bd;}}
# 206
if(i_valid)relns=({Cyc_Relations_add_relation(Cyc_Core_heap_region,rop_left,Cyc_Relations_Rlte,rop_i,relns);});else{
if(z_valid)relns=({Cyc_Relations_add_relation(Cyc_Core_heap_region,rop_left,Cyc_Relations_Rlt,rop_z,relns);});else{
return 1;}}{
# 213
int ans=({Cyc_Relations_consistent_relations(relns);});
# 217
if(!ans)
++ Cyc_InsertChecks_bounds_checks_eliminated;
# 217
if(
# 219
Cyc_InsertChecks_warn_bounds_checks && ans)
({void*_tmp27=0U;({unsigned _tmpE6=a->loc;struct _fat_ptr _tmpE5=({const char*_tmp28="inserted bounds check (location is pointer expression)";_tag_fat(_tmp28,sizeof(char),55U);});Cyc_Tcutil_warn(_tmpE6,_tmpE5,_tag_fat(_tmp27,sizeof(void*),0U));});});
# 217
return ans;}}}}
# 146
static void Cyc_InsertChecks_do_stmt(struct Cyc_InsertChecks_Env,struct Cyc_Absyn_Stmt*);
# 225
static void Cyc_InsertChecks_do_exp(struct Cyc_InsertChecks_Env,struct Cyc_Absyn_Exp*);
# 227
static void Cyc_InsertChecks_do_exp_opt(struct Cyc_InsertChecks_Env nv,struct Cyc_Absyn_Exp*eo){
if(eo != 0)
({Cyc_InsertChecks_do_exp(nv,eo);});}struct _tuple13{void*f1;void*f2;};struct _tuple14{struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct _tuple15{struct Cyc_List_List*f1;struct Cyc_Absyn_Exp*f2;};
# 232
static void Cyc_InsertChecks_do_exp(struct Cyc_InsertChecks_Env nv,struct Cyc_Absyn_Exp*e){
void*_stmttmp5=e->r;void*_tmp2B=_stmttmp5;struct Cyc_Absyn_Stmt*_tmp2C;struct Cyc_List_List*_tmp2D;struct Cyc_List_List*_tmp2E;struct Cyc_List_List*_tmp2F;struct Cyc_Absyn_Exp*_tmp31;struct Cyc_Absyn_Exp*_tmp30;struct Cyc_Absyn_Exp*_tmp33;struct Cyc_Absyn_Exp*_tmp32;struct Cyc_Absyn_Exp*_tmp34;struct Cyc_Absyn_Exp*_tmp35;struct Cyc_Absyn_Exp*_tmp36;struct Cyc_Absyn_Exp*_tmp37;struct Cyc_Absyn_Exp*_tmp38;struct Cyc_Absyn_Exp*_tmp39;struct Cyc_Absyn_Exp*_tmp3A;struct Cyc_Absyn_Exp*_tmp3B;struct Cyc_Absyn_Exp*_tmp3D;struct Cyc_Absyn_Exp*_tmp3C;struct Cyc_Absyn_Exp*_tmp3F;struct Cyc_Absyn_Exp*_tmp3E;struct Cyc_Absyn_Exp*_tmp40;struct Cyc_Absyn_Exp*_tmp42;struct Cyc_Absyn_Exp*_tmp41;struct Cyc_Absyn_Exp*_tmp43;struct Cyc_Absyn_Exp*_tmp44;enum Cyc_Absyn_Coercion _tmp48;int _tmp47;struct Cyc_Absyn_Exp*_tmp46;void*_tmp45;struct Cyc_List_List*_tmp4A;struct Cyc_Absyn_Exp*_tmp49;enum Cyc_Absyn_Incrementor _tmp4C;struct Cyc_Absyn_Exp*_tmp4B;struct Cyc_Absyn_Exp*_tmp4F;struct Cyc_Absyn_Exp*_tmp4E;struct Cyc_Absyn_Exp*_tmp4D;struct Cyc_Absyn_Exp*_tmp51;struct Cyc_Absyn_Exp*_tmp50;struct Cyc_Absyn_Exp*_tmp53;struct Cyc_Absyn_Exp*_tmp52;struct Cyc_Absyn_Exp*_tmp55;struct Cyc_Absyn_Exp*_tmp54;struct Cyc_Absyn_Exp*_tmp57;struct Cyc_Absyn_Exp*_tmp56;struct Cyc_Absyn_Exp*_tmp59;struct Cyc_Absyn_Exp*_tmp58;struct Cyc_List_List*_tmp5A;struct Cyc_List_List*_tmp5B;struct Cyc_List_List*_tmp5C;switch(*((int*)_tmp2B)){case 41U: _LL1: _LL2:
 goto _LL4;case 0U: _LL3: _LL4:
 goto _LL6;case 1U: _LL5: _LL6:
 goto _LL8;case 2U: _LL7: _LL8:
 e->annot=(void*)& Cyc_InsertChecks_NoCheck_val;goto _LL0;case 31U: _LL9: _tmp5C=((struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*)_tmp2B)->f1;_LLA: {struct Cyc_List_List*es=_tmp5C;
# 239
_tmp5B=es;goto _LLC;}case 24U: _LLB: _tmp5B=((struct Cyc_Absyn_Tuple_e_Absyn_Raw_exp_struct*)_tmp2B)->f1;_LLC: {struct Cyc_List_List*es=_tmp5B;
_tmp5A=es;goto _LLE;}case 3U: _LLD: _tmp5A=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp2B)->f2;_LLE: {struct Cyc_List_List*es=_tmp5A;
# 242
e->annot=(void*)& Cyc_InsertChecks_NoCheck_val;
for(0;es != 0;es=es->tl){
({Cyc_InsertChecks_do_exp(nv,(struct Cyc_Absyn_Exp*)es->hd);});}
goto _LL0;}case 4U: _LLF: _tmp58=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp2B)->f1;_tmp59=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp2B)->f3;_LL10: {struct Cyc_Absyn_Exp*e1=_tmp58;struct Cyc_Absyn_Exp*e2=_tmp59;
# 249
_tmp56=e1;_tmp57=e2;goto _LL12;}case 34U: _LL11: _tmp56=((struct Cyc_Absyn_Spawn_e_Absyn_Raw_exp_struct*)_tmp2B)->f1;_tmp57=((struct Cyc_Absyn_Spawn_e_Absyn_Raw_exp_struct*)_tmp2B)->f2;_LL12: {struct Cyc_Absyn_Exp*e1=_tmp56;struct Cyc_Absyn_Exp*e2=_tmp57;
_tmp54=e1;_tmp55=e2;goto _LL14;}case 9U: _LL13: _tmp54=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp2B)->f1;_tmp55=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp2B)->f2;_LL14: {struct Cyc_Absyn_Exp*e1=_tmp54;struct Cyc_Absyn_Exp*e2=_tmp55;
_tmp52=e1;_tmp53=e2;goto _LL16;}case 7U: _LL15: _tmp52=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp2B)->f1;_tmp53=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp2B)->f2;_LL16: {struct Cyc_Absyn_Exp*e1=_tmp52;struct Cyc_Absyn_Exp*e2=_tmp53;
_tmp50=e1;_tmp51=e2;goto _LL18;}case 8U: _LL17: _tmp50=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp2B)->f1;_tmp51=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp2B)->f2;_LL18: {struct Cyc_Absyn_Exp*e1=_tmp50;struct Cyc_Absyn_Exp*e2=_tmp51;
# 254
e->annot=(void*)& Cyc_InsertChecks_NoCheck_val;
({Cyc_InsertChecks_do_exp(nv,e1);});
({Cyc_InsertChecks_do_exp(nv,e2);});
goto _LL0;}case 6U: _LL19: _tmp4D=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp2B)->f1;_tmp4E=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp2B)->f2;_tmp4F=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp2B)->f3;_LL1A: {struct Cyc_Absyn_Exp*e1=_tmp4D;struct Cyc_Absyn_Exp*e2=_tmp4E;struct Cyc_Absyn_Exp*e3=_tmp4F;
# 259
e->annot=(void*)& Cyc_InsertChecks_NoCheck_val;
({Cyc_InsertChecks_do_exp(nv,e1);});
({Cyc_InsertChecks_do_exp(nv,e2);});
({Cyc_InsertChecks_do_exp(nv,e3);});
goto _LL0;}case 5U: _LL1B: _tmp4B=((struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*)_tmp2B)->f1;_tmp4C=((struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*)_tmp2B)->f2;_LL1C: {struct Cyc_Absyn_Exp*e2=_tmp4B;enum Cyc_Absyn_Incrementor incr=_tmp4C;
# 270
if((({Cyc_Tcutil_get_type_bound((void*)_check_null(e2->topt));})!= 0 &&({Cyc_Tcutil_is_zeroterm_pointer_type((void*)_check_null(e2->topt));}))&&({Cyc_InsertChecks_need_null_check(nv,e2);}))
# 273
e->annot=(void*)& Cyc_InsertChecks_NullOnly_val;else{
# 275
e->annot=(void*)& Cyc_InsertChecks_NoCheck_val;}
({Cyc_InsertChecks_do_exp(nv,e2);});
goto _LL0;}case 10U: _LL1D: _tmp49=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp2B)->f1;_tmp4A=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp2B)->f2;_LL1E: {struct Cyc_Absyn_Exp*e1=_tmp49;struct Cyc_List_List*es=_tmp4A;
# 280
if(({Cyc_InsertChecks_need_null_check(nv,e1);}))
e->annot=(void*)& Cyc_InsertChecks_NullOnly_val;else{
# 283
e->annot=(void*)& Cyc_InsertChecks_NoCheck_val;}
({Cyc_InsertChecks_do_exp(nv,e1);});
for(0;es != 0;es=es->tl){
({Cyc_InsertChecks_do_exp(nv,(struct Cyc_Absyn_Exp*)es->hd);});}
goto _LL0;}case 14U: _LL1F: _tmp45=(void*)((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp2B)->f1;_tmp46=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp2B)->f2;_tmp47=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp2B)->f3;_tmp48=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp2B)->f4;_LL20: {void*t=_tmp45;struct Cyc_Absyn_Exp*e1=_tmp46;int user_inserted=_tmp47;enum Cyc_Absyn_Coercion coercion=_tmp48;
# 292
{struct _tuple13 _stmttmp6=({struct _tuple13 _tmpD2;({void*_tmpE8=({Cyc_Tcutil_compress((void*)_check_null(e1->topt));});_tmpD2.f1=_tmpE8;}),({void*_tmpE7=({Cyc_Tcutil_compress(t);});_tmpD2.f2=_tmpE7;});_tmpD2;});struct _tuple13 _tmp5D=_stmttmp6;struct Cyc_Absyn_PtrInfo _tmp5F;struct Cyc_Absyn_PtrInfo _tmp5E;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp5D.f1)->tag == 3U){if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp5D.f2)->tag == 3U){_LL58: _tmp5E=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp5D.f1)->f1;_tmp5F=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp5D.f2)->f1;_LL59: {struct Cyc_Absyn_PtrInfo p1=_tmp5E;struct Cyc_Absyn_PtrInfo p2=_tmp5F;
# 294
int n1=({Cyc_Tcutil_force_type2bool(0,(p1.ptr_atts).nullable);});
int n2=({Cyc_Tcutil_force_type2bool(0,(p2.ptr_atts).nullable);});
struct Cyc_Absyn_Exp*b1=({struct Cyc_Absyn_Exp*(*_tmp7D)(void*def,void*b)=Cyc_Tcutil_get_bounds_exp;void*_tmp7E=({Cyc_Absyn_bounds_one();});void*_tmp7F=(p1.ptr_atts).bounds;_tmp7D(_tmp7E,_tmp7F);});
struct Cyc_Absyn_Exp*b2=({struct Cyc_Absyn_Exp*(*_tmp7A)(void*def,void*b)=Cyc_Tcutil_get_bounds_exp;void*_tmp7B=({Cyc_Absyn_bounds_one();});void*_tmp7C=(p2.ptr_atts).bounds;_tmp7A(_tmp7B,_tmp7C);});
if((n1 && !n2)&&(int)coercion != (int)2U)
({void*_tmp60=0U;({int(*_tmpEB)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp62)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos_loc;_tmp62;});unsigned _tmpEA=e->loc;struct _fat_ptr _tmpE9=({const char*_tmp61="null-check conversion mis-classified";_tag_fat(_tmp61,sizeof(char),37U);});_tmpEB(_tmpEA,_tmpE9,_tag_fat(_tmp60,sizeof(void*),0U));});});else{
if(((unsigned)b1 && !((unsigned)b2))&&(int)coercion == (int)2U)
({void*_tmp63=0U;({int(*_tmpEE)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp65)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos_loc;_tmp65;});unsigned _tmpED=e->loc;struct _fat_ptr _tmpEC=({const char*_tmp64="conversion mis-classified as null-check";_tag_fat(_tmp64,sizeof(char),40U);});_tmpEE(_tmpED,_tmpEC,_tag_fat(_tmp63,sizeof(void*),0U));});});}
# 298
{struct _tuple14 _stmttmp7=({struct _tuple14 _tmpD1;_tmpD1.f1=b1,_tmpD1.f2=b2;_tmpD1;});struct _tuple14 _tmp66=_stmttmp7;if(_tmp66.f1 != 0){if(_tmp66.f2 != 0){_LL5D: _LL5E:
# 304
 if(!({({struct Cyc_Absyn_Exp*_tmpEF=(struct Cyc_Absyn_Exp*)_check_null(b2);Cyc_Evexp_lte_const_exp(_tmpEF,(struct Cyc_Absyn_Exp*)_check_null(b1));});}))
({void*_tmp67=0U;({int(*_tmpF2)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp69)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos_loc;_tmp69;});unsigned _tmpF1=e->loc;struct _fat_ptr _tmpF0=({const char*_tmp68="InsertChecks: cast to maybe-shorter pointer type";_tag_fat(_tmp68,sizeof(char),49U);});_tmpF2(_tmpF1,_tmpF0,_tag_fat(_tmp67,sizeof(void*),0U));});});
# 304
if(
# 306
(n1 && !n2)&&({Cyc_InsertChecks_need_null_check(nv,e1);})){
if(!user_inserted)
({void*_tmp6A=0U;({unsigned _tmpF4=e->loc;struct _fat_ptr _tmpF3=({const char*_tmp6B="inserted null check due to implicit cast from * to @ type";_tag_fat(_tmp6B,sizeof(char),58U);});Cyc_Tcutil_warn(_tmpF4,_tmpF3,_tag_fat(_tmp6A,sizeof(void*),0U));});});
# 307
e->annot=(void*)& Cyc_InsertChecks_NullOnly_val;}else{
# 312
e->annot=(void*)& Cyc_InsertChecks_NoCheck_val;}
goto _LL5C;}else{_LL5F: _LL60:
# 316
 if(!({Cyc_Evexp_c_can_eval((struct Cyc_Absyn_Exp*)_check_null(b1));})){
if(({Cyc_Absyn_get_debug();}))
({struct Cyc_String_pa_PrintArg_struct _tmp6E=({struct Cyc_String_pa_PrintArg_struct _tmpD0;_tmpD0.tag=0U,({
struct _fat_ptr _tmpF5=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_exp2string(b1);}));_tmpD0.f1=_tmpF5;});_tmpD0;});struct Cyc_String_pa_PrintArg_struct _tmp6F=({struct Cyc_String_pa_PrintArg_struct _tmpCF;_tmpCF.tag=0U,({struct _fat_ptr _tmpF6=(struct _fat_ptr)((struct _fat_ptr)(b2 != 0?({Cyc_Absynpp_exp2string(b2);}):(struct _fat_ptr)({const char*_tmp71="nil";_tag_fat(_tmp71,sizeof(char),4U);})));_tmpCF.f1=_tmpF6;});_tmpCF;});struct Cyc_String_pa_PrintArg_struct _tmp70=({struct Cyc_String_pa_PrintArg_struct _tmpCE;_tmpCE.tag=0U,({struct _fat_ptr _tmpF7=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t);}));_tmpCE.f1=_tmpF7;});_tmpCE;});void*_tmp6C[3U];_tmp6C[0]=& _tmp6E,_tmp6C[1]=& _tmp6F,_tmp6C[2]=& _tmp70;({unsigned _tmpF9=e->loc;struct _fat_ptr _tmpF8=({const char*_tmp6D="cannot perform coercion: numelts is statically unknown. (b1=\"%s\", b2=\"%s\" t=\"%s\").";_tag_fat(_tmp6D,sizeof(char),83U);});Cyc_Warn_err(_tmpF9,_tmpF8,_tag_fat(_tmp6C,sizeof(void*),3U));});});else{
# 321
({void*_tmp72=0U;({unsigned _tmpFB=e->loc;struct _fat_ptr _tmpFA=({const char*_tmp73="cannot perform coercion: numelts is statically unknown.";_tag_fat(_tmp73,sizeof(char),56U);});Cyc_Warn_err(_tmpFB,_tmpFA,_tag_fat(_tmp72,sizeof(void*),0U));});});}}
# 316
e->annot=(void*)& Cyc_InsertChecks_NoCheck_val;
# 324
goto _LL5C;}}else{if(_tmp66.f2 != 0){_LL61: _LL62:
# 326
 if(nv.at_toplevel)
({void*_tmp74=0U;({unsigned _tmpFD=e->loc;struct _fat_ptr _tmpFC=({const char*_tmp75="cannot coerce fat-pointer to thin-pointer at toplevel";_tag_fat(_tmp75,sizeof(char),54U);});Cyc_Warn_err(_tmpFD,_tmpFC,_tag_fat(_tmp74,sizeof(void*),0U));});});
# 326
if(!({Cyc_Evexp_c_can_eval((struct Cyc_Absyn_Exp*)_check_null(b2));}))
# 330
({void*_tmp76=0U;({unsigned _tmpFF=e->loc;struct _fat_ptr _tmpFE=({const char*_tmp77="cannot perform coercion: numelts is statically unknown";_tag_fat(_tmp77,sizeof(char),55U);});Cyc_Warn_err(_tmpFF,_tmpFE,_tag_fat(_tmp76,sizeof(void*),0U));});});
# 326
++ Cyc_InsertChecks_total_bounds_checks;{
# 334
int bds_chk=0;
if(!n2 &&({Cyc_InsertChecks_need_null_check(nv,e1);})){
if(!user_inserted)
({void*_tmp78=0U;({unsigned _tmp101=e->loc;struct _fat_ptr _tmp100=({const char*_tmp79="inserted null check due to implicit cast from *@fat (?) to @ type";_tag_fat(_tmp79,sizeof(char),66U);});Cyc_Tcutil_warn(_tmp101,_tmp100,_tag_fat(_tmp78,sizeof(void*),0U));});});
# 336
e->annot=
# 339
bds_chk?(void*)& Cyc_InsertChecks_NullAndFatBound_val:(void*)& Cyc_InsertChecks_NullOnly_val;}else{
# 341
e->annot=bds_chk?(void*)& Cyc_InsertChecks_FatBound_val:(void*)& Cyc_InsertChecks_NoCheck_val;}
goto _LL5C;}}else{_LL63: _LL64:
 e->annot=(void*)& Cyc_InsertChecks_NoCheck_val;goto _LL5C;}}_LL5C:;}
# 345
goto _LL57;}}else{goto _LL5A;}}else{_LL5A: _LL5B:
 e->annot=(void*)& Cyc_InsertChecks_NoCheck_val;goto _LL57;}_LL57:;}
# 348
({Cyc_InsertChecks_do_exp(nv,e1);});
goto _LL0;}case 22U: _LL21: _tmp44=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp2B)->f1;_LL22: {struct Cyc_Absyn_Exp*e1=_tmp44;
# 351
_tmp43=e1;goto _LL24;}case 20U: _LL23: _tmp43=((struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_tmp2B)->f1;_LL24: {struct Cyc_Absyn_Exp*e1=_tmp43;
_tmp41=e1;_tmp42=({Cyc_InsertChecks_shared_zero_exp();});goto _LL26;}case 23U: _LL25: _tmp41=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp2B)->f1;_tmp42=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp2B)->f2;_LL26: {struct Cyc_Absyn_Exp*e1=_tmp41;struct Cyc_Absyn_Exp*e2=_tmp42;
# 354
{void*_stmttmp8=({Cyc_Tcutil_compress((void*)_check_null(e1->topt));});void*_tmp80=_stmttmp8;void*_tmp86;void*_tmp85;void*_tmp84;void*_tmp83;struct Cyc_Absyn_Tqual _tmp82;void*_tmp81;switch(*((int*)_tmp80)){case 6U: _LL66: _LL67:
 e->annot=(void*)& Cyc_InsertChecks_NoCheck_val;goto _LL65;case 3U: _LL68: _tmp81=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp80)->f1).elt_type;_tmp82=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp80)->f1).elt_tq;_tmp83=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp80)->f1).ptr_atts).rgn;_tmp84=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp80)->f1).ptr_atts).nullable;_tmp85=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp80)->f1).ptr_atts).bounds;_tmp86=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp80)->f1).ptr_atts).zero_term;_LL69: {void*ta=_tmp81;struct Cyc_Absyn_Tqual tq=_tmp82;void*rgn=_tmp83;void*n=_tmp84;void*b=_tmp85;void*zt=_tmp86;
# 357
int bds_ck=({int(*_tmp95)(struct Cyc_InsertChecks_Env nv,struct Cyc_List_List*relns,struct Cyc_Absyn_Exp*a,struct Cyc_Absyn_Exp*i)=Cyc_InsertChecks_need_bounds_check;struct Cyc_InsertChecks_Env _tmp96=nv;struct Cyc_List_List*_tmp97=({Cyc_InsertChecks_get_relns(e1);});struct Cyc_Absyn_Exp*_tmp98=e1;struct Cyc_Absyn_Exp*_tmp99=e2;_tmp95(_tmp96,_tmp97,_tmp98,_tmp99);});
struct Cyc_Absyn_Exp*bd=({struct Cyc_Absyn_Exp*(*_tmp92)(void*def,void*b)=Cyc_Tcutil_get_bounds_exp;void*_tmp93=({Cyc_Absyn_bounds_one();});void*_tmp94=b;_tmp92(_tmp93,_tmp94);});
int fat_ptr=bd == 0;
int null_ck;
# 363
if(fat_ptr && !bds_ck)null_ck=0;else{
null_ck=({Cyc_InsertChecks_need_null_check(nv,e1);});}
if(bds_ck){
if((unsigned)bd){
if(!({Cyc_Evexp_c_can_eval(bd);})){
({void*_tmp87=0U;({unsigned _tmp103=e->loc;struct _fat_ptr _tmp102=({const char*_tmp88="cannot determine subscript is in bounds";_tag_fat(_tmp88,sizeof(char),40U);});Cyc_Warn_err(_tmp103,_tmp102,_tag_fat(_tmp87,sizeof(void*),0U));});});
({void*_tmp89=0U;({struct Cyc___cycFILE*_tmp105=Cyc_stderr;struct _fat_ptr _tmp104=({const char*_tmp8A="[all that I can prove is ";_tag_fat(_tmp8A,sizeof(char),26U);});Cyc_fprintf(_tmp105,_tmp104,_tag_fat(_tmp89,sizeof(void*),0U));});});
({void(*_tmp8B)(struct Cyc___cycFILE*,struct Cyc_List_List*)=Cyc_Relations_print_relns;struct Cyc___cycFILE*_tmp8C=Cyc_stderr;struct Cyc_List_List*_tmp8D=({Cyc_InsertChecks_get_relns(e1);});_tmp8B(_tmp8C,_tmp8D);});
({void*_tmp8E=0U;({struct Cyc___cycFILE*_tmp107=Cyc_stderr;struct _fat_ptr _tmp106=({const char*_tmp8F="]\n";_tag_fat(_tmp8F,sizeof(char),3U);});Cyc_fprintf(_tmp107,_tmp106,_tag_fat(_tmp8E,sizeof(void*),0U));});});}
# 367
({void*_tmp108=
# 374
null_ck?(void*)({struct Cyc_InsertChecks_NullAndThinBound_Absyn_AbsynAnnot_struct*_tmp90=_cycalloc(sizeof(*_tmp90));_tmp90->tag=Cyc_InsertChecks_NullAndThinBound,_tmp90->f1=bd;_tmp90;}):(void*)({struct Cyc_InsertChecks_ThinBound_Absyn_AbsynAnnot_struct*_tmp91=_cycalloc(sizeof(*_tmp91));_tmp91->tag=Cyc_InsertChecks_ThinBound,_tmp91->f1=bd;_tmp91;});
# 367
e->annot=_tmp108;});}else{
# 376
e->annot=null_ck?(void*)& Cyc_InsertChecks_NullAndFatBound_val:(void*)& Cyc_InsertChecks_FatBound_val;}}else{
if(null_ck)
e->annot=(void*)& Cyc_InsertChecks_NullOnly_val;else{
# 380
e->annot=(void*)& Cyc_InsertChecks_NoCheck_val;}}
goto _LL65;}default: _LL6A: _LL6B:
({void*_tmp9A=0U;({int(*_tmp10B)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp9C)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos_loc;_tmp9C;});unsigned _tmp10A=e->loc;struct _fat_ptr _tmp109=({const char*_tmp9B="InsertChecks: non-pointer-type on dereference";_tag_fat(_tmp9B,sizeof(char),46U);});_tmp10B(_tmp10A,_tmp109,_tag_fat(_tmp9A,sizeof(void*),0U));});});}_LL65:;}
# 384
({Cyc_InsertChecks_do_exp(nv,e1);});
({Cyc_InsertChecks_do_exp(nv,e2);});
goto _LL0;}case 18U: _LL27: _tmp40=((struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct*)_tmp2B)->f1;_LL28: {struct Cyc_Absyn_Exp*e1=_tmp40;
# 389
e->annot=(void*)& Cyc_InsertChecks_NoCheck_val;
({void(*_tmp9D)(struct Cyc_InsertChecks_Env nv,struct Cyc_Absyn_Exp*e)=Cyc_InsertChecks_do_exp;struct Cyc_InsertChecks_Env _tmp9E=({Cyc_InsertChecks_enter_sizeof(nv);});struct Cyc_Absyn_Exp*_tmp9F=e1;_tmp9D(_tmp9E,_tmp9F);});
goto _LL0;}case 16U: _LL29: _tmp3E=((struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct*)_tmp2B)->f1;_tmp3F=((struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct*)_tmp2B)->f2;_LL2A: {struct Cyc_Absyn_Exp*rgnopt=_tmp3E;struct Cyc_Absyn_Exp*e1=_tmp3F;
# 394
e->annot=(void*)& Cyc_InsertChecks_NoCheck_val;
({Cyc_InsertChecks_do_exp_opt(nv,rgnopt);});
({Cyc_InsertChecks_do_exp(nv,e1);});
goto _LL0;}case 35U: _LL2B: _tmp3C=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp2B)->f1).rgn;_tmp3D=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp2B)->f1).num_elts;_LL2C: {struct Cyc_Absyn_Exp*rgnopt=_tmp3C;struct Cyc_Absyn_Exp*sizeexp=_tmp3D;
# 401
e->annot=(void*)& Cyc_InsertChecks_NoCheck_val;
({Cyc_InsertChecks_do_exp_opt(nv,rgnopt);});
({Cyc_InsertChecks_do_exp(nv,sizeexp);});
goto _LL0;}case 42U: _LL2D: _tmp3B=((struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct*)_tmp2B)->f1;_LL2E: {struct Cyc_Absyn_Exp*e1=_tmp3B;
# 406
_tmp3A=e1;goto _LL30;}case 11U: _LL2F: _tmp3A=((struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct*)_tmp2B)->f1;_LL30: {struct Cyc_Absyn_Exp*e1=_tmp3A;
_tmp39=e1;goto _LL32;}case 12U: _LL31: _tmp39=((struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*)_tmp2B)->f1;_LL32: {struct Cyc_Absyn_Exp*e1=_tmp39;
_tmp38=e1;goto _LL34;}case 13U: _LL33: _tmp38=((struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*)_tmp2B)->f1;_LL34: {struct Cyc_Absyn_Exp*e1=_tmp38;
_tmp37=e1;goto _LL36;}case 15U: _LL35: _tmp37=((struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct*)_tmp2B)->f1;_LL36: {struct Cyc_Absyn_Exp*e1=_tmp37;
# 411
e->annot=(void*)& Cyc_InsertChecks_NoCheck_val;
({Cyc_InsertChecks_do_exp(nv,e1);});
goto _LL0;}case 32U: _LL37: _LL38:
 goto _LL3A;case 33U: _LL39: _LL3A:
 goto _LL3C;case 17U: _LL3B: _LL3C:
 goto _LL3E;case 19U: _LL3D: _LL3E:
 e->annot=(void*)& Cyc_InsertChecks_NoCheck_val;goto _LL0;case 21U: _LL3F: _tmp36=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp2B)->f1;_LL40: {struct Cyc_Absyn_Exp*e1=_tmp36;
_tmp35=e1;goto _LL42;}case 39U: _LL41: _tmp35=((struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct*)_tmp2B)->f1;_LL42: {struct Cyc_Absyn_Exp*e1=_tmp35;
_tmp34=e1;goto _LL44;}case 28U: _LL43: _tmp34=((struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*)_tmp2B)->f1;_LL44: {struct Cyc_Absyn_Exp*e1=_tmp34;
# 421
e->annot=(void*)& Cyc_InsertChecks_NoCheck_val;
({Cyc_InsertChecks_do_exp(nv,e1);});
goto _LL0;}case 27U: _LL45: _tmp32=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmp2B)->f2;_tmp33=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmp2B)->f3;_LL46: {struct Cyc_Absyn_Exp*e1=_tmp32;struct Cyc_Absyn_Exp*e2=_tmp33;
_tmp30=e1;_tmp31=e2;goto _LL48;}case 36U: _LL47: _tmp30=((struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct*)_tmp2B)->f1;_tmp31=((struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct*)_tmp2B)->f2;_LL48: {struct Cyc_Absyn_Exp*e1=_tmp30;struct Cyc_Absyn_Exp*e2=_tmp31;
# 426
e->annot=(void*)& Cyc_InsertChecks_NoCheck_val;
({Cyc_InsertChecks_do_exp(nv,e1);});
({Cyc_InsertChecks_do_exp(nv,e2);});
goto _LL0;}case 26U: _LL49: _tmp2F=((struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*)_tmp2B)->f1;_LL4A: {struct Cyc_List_List*dles=_tmp2F;
_tmp2E=dles;goto _LL4C;}case 29U: _LL4B: _tmp2E=((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_tmp2B)->f3;_LL4C: {struct Cyc_List_List*dles=_tmp2E;
_tmp2D=dles;goto _LL4E;}case 30U: _LL4D: _tmp2D=((struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct*)_tmp2B)->f2;_LL4E: {struct Cyc_List_List*dles=_tmp2D;
# 433
e->annot=(void*)& Cyc_InsertChecks_NoCheck_val;
for(0;dles != 0;dles=dles->tl){
({Cyc_InsertChecks_do_exp(nv,(*((struct _tuple15*)dles->hd)).f2);});}
goto _LL0;}case 38U: _LL4F: _tmp2C=((struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct*)_tmp2B)->f1;_LL50: {struct Cyc_Absyn_Stmt*s=_tmp2C;
e->annot=(void*)& Cyc_InsertChecks_NoCheck_val;({Cyc_InsertChecks_do_stmt(nv,s);});goto _LL0;}case 37U: _LL51: _LL52:
# 439
 goto _LL54;case 25U: _LL53: _LL54:
 goto _LL56;default: _LL55: _LL56:
({void*_tmpA0=0U;({int(*_tmp10E)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmpA2)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos_loc;_tmpA2;});unsigned _tmp10D=e->loc;struct _fat_ptr _tmp10C=({const char*_tmpA1="InsertChecks, unexpected exp form";_tag_fat(_tmpA1,sizeof(char),34U);});_tmp10E(_tmp10D,_tmp10C,_tag_fat(_tmpA0,sizeof(void*),0U));});});}_LL0:;}
# 445
static void Cyc_InsertChecks_do_decl(struct Cyc_InsertChecks_Env nv,struct Cyc_Absyn_Decl*d){
void*_stmttmp9=d->r;void*_tmpA4=_stmttmp9;struct Cyc_List_List*_tmpA5;struct Cyc_List_List*_tmpA6;struct Cyc_List_List*_tmpA7;struct Cyc_Absyn_Exp*_tmpA8;struct Cyc_Absyn_Exp*_tmpA9;struct Cyc_Absyn_Fndecl*_tmpAA;struct Cyc_Absyn_Vardecl*_tmpAB;switch(*((int*)_tmpA4)){case 0U: _LL1: _tmpAB=((struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_tmpA4)->f1;_LL2: {struct Cyc_Absyn_Vardecl*vd=_tmpAB;
({Cyc_InsertChecks_do_exp_opt(nv,vd->initializer);});goto _LL0;}case 1U: _LL3: _tmpAA=((struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*)_tmpA4)->f1;_LL4: {struct Cyc_Absyn_Fndecl*fd=_tmpAA;
({void(*_tmpAC)(struct Cyc_InsertChecks_Env,struct Cyc_Absyn_Stmt*)=Cyc_InsertChecks_do_stmt;struct Cyc_InsertChecks_Env _tmpAD=({Cyc_InsertChecks_fn_body_env(fd);});struct Cyc_Absyn_Stmt*_tmpAE=fd->body;_tmpAC(_tmpAD,_tmpAE);});goto _LL0;}case 2U: _LL5: _tmpA9=((struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct*)_tmpA4)->f3;_LL6: {struct Cyc_Absyn_Exp*e=_tmpA9;
({Cyc_InsertChecks_do_exp(nv,e);});goto _LL0;}case 3U: _LL7: _LL8:
 goto _LL0;case 4U: _LL9: _tmpA8=((struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*)_tmpA4)->f3;_LLA: {struct Cyc_Absyn_Exp*eo=_tmpA8;
({Cyc_InsertChecks_do_exp_opt(nv,eo);});goto _LL0;}case 5U: _LLB: _LLC:
 goto _LLE;case 6U: _LLD: _LLE:
 goto _LL10;case 7U: _LLF: _LL10:
 goto _LL12;case 8U: _LL11: _LL12:
 goto _LL14;case 13U: _LL13: _LL14:
 goto _LL16;case 14U: _LL15: _LL16:
 goto _LL18;case 15U: _LL17: _LL18:
 goto _LL1A;case 16U: _LL19: _LL1A:
 goto _LL1C;case 12U: _LL1B: _LL1C:
 goto _LL0;case 11U: _LL1D: _tmpA7=((struct Cyc_Absyn_ExternC_d_Absyn_Raw_decl_struct*)_tmpA4)->f1;_LL1E: {struct Cyc_List_List*ds=_tmpA7;
_tmpA6=ds;goto _LL20;}case 10U: _LL1F: _tmpA6=((struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct*)_tmpA4)->f2;_LL20: {struct Cyc_List_List*ds=_tmpA6;
_tmpA5=ds;goto _LL22;}default: _LL21: _tmpA5=((struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct*)_tmpA4)->f2;_LL22: {struct Cyc_List_List*ds=_tmpA5;
# 464
for(0;ds != 0;ds=ds->tl){
({Cyc_InsertChecks_do_decl(nv,(struct Cyc_Absyn_Decl*)ds->hd);});}}}_LL0:;}
# 469
static void Cyc_InsertChecks_do_scs(struct Cyc_InsertChecks_Env nv,struct Cyc_List_List*scs){
# 472
for(0;scs != 0;scs=scs->tl){
if(((struct Cyc_Absyn_Switch_clause*)scs->hd)->where_clause != 0)
({Cyc_InsertChecks_do_exp(nv,(struct Cyc_Absyn_Exp*)_check_null(((struct Cyc_Absyn_Switch_clause*)scs->hd)->where_clause));});
# 473
({Cyc_InsertChecks_do_stmt(nv,((struct Cyc_Absyn_Switch_clause*)scs->hd)->body);});}}
# 479
static void Cyc_InsertChecks_do_stmt(struct Cyc_InsertChecks_Env nv,struct Cyc_Absyn_Stmt*s){
void*_stmttmpA=s->r;void*_tmpB1=_stmttmpA;struct Cyc_List_List*_tmpB3;struct Cyc_Absyn_Stmt*_tmpB2;struct Cyc_List_List*_tmpB5;struct Cyc_Absyn_Exp*_tmpB4;struct Cyc_Absyn_Stmt*_tmpB7;struct Cyc_Absyn_Decl*_tmpB6;struct Cyc_Absyn_Stmt*_tmpB8;struct Cyc_List_List*_tmpB9;struct Cyc_Absyn_Stmt*_tmpBD;struct Cyc_Absyn_Exp*_tmpBC;struct Cyc_Absyn_Exp*_tmpBB;struct Cyc_Absyn_Exp*_tmpBA;struct Cyc_Absyn_Stmt*_tmpBF;struct Cyc_Absyn_Exp*_tmpBE;struct Cyc_Absyn_Exp*_tmpC1;struct Cyc_Absyn_Stmt*_tmpC0;struct Cyc_Absyn_Stmt*_tmpC4;struct Cyc_Absyn_Stmt*_tmpC3;struct Cyc_Absyn_Exp*_tmpC2;struct Cyc_Absyn_Exp*_tmpC5;struct Cyc_Absyn_Stmt*_tmpC7;struct Cyc_Absyn_Stmt*_tmpC6;struct Cyc_Absyn_Exp*_tmpC8;switch(*((int*)_tmpB1)){case 0U: _LL1: _LL2:
 goto _LL0;case 1U: _LL3: _tmpC8=((struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct*)_tmpB1)->f1;_LL4: {struct Cyc_Absyn_Exp*e=_tmpC8;
({Cyc_InsertChecks_do_exp(nv,e);});goto _LL0;}case 2U: _LL5: _tmpC6=((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_tmpB1)->f1;_tmpC7=((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_tmpB1)->f2;_LL6: {struct Cyc_Absyn_Stmt*s1=_tmpC6;struct Cyc_Absyn_Stmt*s2=_tmpC7;
({Cyc_InsertChecks_do_stmt(nv,s1);});({Cyc_InsertChecks_do_stmt(nv,s2);});goto _LL0;}case 3U: _LL7: _tmpC5=((struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct*)_tmpB1)->f1;_LL8: {struct Cyc_Absyn_Exp*eopt=_tmpC5;
if((unsigned)eopt)({Cyc_InsertChecks_do_exp(nv,eopt);});goto _LL0;}case 4U: _LL9: _tmpC2=((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmpB1)->f1;_tmpC3=((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmpB1)->f2;_tmpC4=((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmpB1)->f3;_LLA: {struct Cyc_Absyn_Exp*e=_tmpC2;struct Cyc_Absyn_Stmt*s1=_tmpC3;struct Cyc_Absyn_Stmt*s2=_tmpC4;
# 486
({Cyc_InsertChecks_do_exp(nv,e);});({Cyc_InsertChecks_do_stmt(nv,s1);});({Cyc_InsertChecks_do_stmt(nv,s2);});goto _LL0;}case 14U: _LLB: _tmpC0=((struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct*)_tmpB1)->f1;_tmpC1=(((struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct*)_tmpB1)->f2).f1;_LLC: {struct Cyc_Absyn_Stmt*s1=_tmpC0;struct Cyc_Absyn_Exp*e=_tmpC1;
_tmpBE=e;_tmpBF=s1;goto _LLE;}case 5U: _LLD: _tmpBE=(((struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct*)_tmpB1)->f1).f1;_tmpBF=((struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct*)_tmpB1)->f2;_LLE: {struct Cyc_Absyn_Exp*e=_tmpBE;struct Cyc_Absyn_Stmt*s1=_tmpBF;
({Cyc_InsertChecks_do_exp(nv,e);});({Cyc_InsertChecks_do_stmt(nv,s1);});goto _LL0;}case 9U: _LLF: _tmpBA=((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmpB1)->f1;_tmpBB=(((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmpB1)->f2).f1;_tmpBC=(((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmpB1)->f3).f1;_tmpBD=((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmpB1)->f4;_LL10: {struct Cyc_Absyn_Exp*e1=_tmpBA;struct Cyc_Absyn_Exp*e2=_tmpBB;struct Cyc_Absyn_Exp*e3=_tmpBC;struct Cyc_Absyn_Stmt*s1=_tmpBD;
# 490
({Cyc_InsertChecks_do_exp(nv,e1);});({Cyc_InsertChecks_do_exp(nv,e2);});({Cyc_InsertChecks_do_exp(nv,e3);});({Cyc_InsertChecks_do_stmt(nv,s1);});goto _LL0;}case 6U: _LL11: _LL12:
 goto _LL14;case 7U: _LL13: _LL14:
 goto _LL16;case 8U: _LL15: _LL16:
 return;case 11U: _LL17: _tmpB9=((struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct*)_tmpB1)->f1;_LL18: {struct Cyc_List_List*es=_tmpB9;
# 495
for(0;es != 0;es=es->tl){
({Cyc_InsertChecks_do_exp(nv,(struct Cyc_Absyn_Exp*)es->hd);});}
goto _LL0;}case 13U: _LL19: _tmpB8=((struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct*)_tmpB1)->f2;_LL1A: {struct Cyc_Absyn_Stmt*s1=_tmpB8;
({Cyc_InsertChecks_do_stmt(nv,s1);});goto _LL0;}case 12U: _LL1B: _tmpB6=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmpB1)->f1;_tmpB7=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmpB1)->f2;_LL1C: {struct Cyc_Absyn_Decl*d=_tmpB6;struct Cyc_Absyn_Stmt*s1=_tmpB7;
({Cyc_InsertChecks_do_decl(nv,d);});({Cyc_InsertChecks_do_stmt(nv,s1);});goto _LL0;}case 10U: _LL1D: _tmpB4=((struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct*)_tmpB1)->f1;_tmpB5=((struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct*)_tmpB1)->f2;_LL1E: {struct Cyc_Absyn_Exp*e=_tmpB4;struct Cyc_List_List*scs=_tmpB5;
({Cyc_InsertChecks_do_exp(nv,e);});({Cyc_InsertChecks_do_scs(nv,scs);});goto _LL0;}default: _LL1F: _tmpB2=((struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct*)_tmpB1)->f1;_tmpB3=((struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct*)_tmpB1)->f2;_LL20: {struct Cyc_Absyn_Stmt*s1=_tmpB2;struct Cyc_List_List*scs=_tmpB3;
({Cyc_InsertChecks_do_stmt(nv,s1);});({Cyc_InsertChecks_do_scs(nv,scs);});goto _LL0;}}_LL0:;}
# 505
void Cyc_InsertChecks_insert_checks(struct Cyc_List_List*tds,struct Cyc_JumpAnalysis_Jump_Anal_Result*tables){
# 507
struct Cyc_InsertChecks_Env nv=({Cyc_InsertChecks_toplevel_env();});
for(0;tds != 0;tds=tds->tl){
({Cyc_InsertChecks_do_decl(nv,(struct Cyc_Absyn_Decl*)tds->hd);});}}
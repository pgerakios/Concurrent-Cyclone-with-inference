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
extern struct _RegionHandle*Cyc_Core_unique_region;struct Cyc_Core_DynamicRegion;struct Cyc_Core_NewDynamicRegion{struct Cyc_Core_DynamicRegion*key;};struct Cyc_Core_ThinRes{void*arr;unsigned nelts;};struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};extern char Cyc_List_List_mismatch[14U];struct Cyc_List_List_mismatch_exn_struct{char*tag;};extern char Cyc_List_Nth[4U];struct Cyc_List_Nth_exn_struct{char*tag;};struct Cyc_Position_Error;struct Cyc___cycFILE;struct Cyc_String_pa_PrintArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Int_pa_PrintArg_struct{int tag;unsigned long f1;};struct Cyc_Double_pa_PrintArg_struct{int tag;double f1;};struct Cyc_LongDouble_pa_PrintArg_struct{int tag;long double f1;};struct Cyc_ShortPtr_pa_PrintArg_struct{int tag;short*f1;};struct Cyc_IntPtr_pa_PrintArg_struct{int tag;unsigned long*f1;};
# 100 "cycboot.h"
int Cyc_fprintf(struct Cyc___cycFILE*,struct _fat_ptr,struct _fat_ptr);struct Cyc_ShortPtr_sa_ScanfArg_struct{int tag;short*f1;};struct Cyc_UShortPtr_sa_ScanfArg_struct{int tag;unsigned short*f1;};struct Cyc_IntPtr_sa_ScanfArg_struct{int tag;int*f1;};struct Cyc_UIntPtr_sa_ScanfArg_struct{int tag;unsigned*f1;};struct Cyc_StringPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_DoublePtr_sa_ScanfArg_struct{int tag;double*f1;};struct Cyc_FloatPtr_sa_ScanfArg_struct{int tag;float*f1;};struct Cyc_CharPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};extern char Cyc_FileCloseError[15U];struct Cyc_FileCloseError_exn_struct{char*tag;};extern char Cyc_FileOpenError[14U];struct Cyc_FileOpenError_exn_struct{char*tag;struct _fat_ptr f1;};struct Cyc_Iter_Iter{void*env;int(*next)(void*env,void*dest);};struct Cyc_Dict_T;struct Cyc_Dict_Dict{int(*rel)(void*,void*);struct _RegionHandle*r;const struct Cyc_Dict_T*t;};extern char Cyc_Dict_Present[8U];struct Cyc_Dict_Present_exn_struct{char*tag;};extern char Cyc_Dict_Absent[7U];struct Cyc_Dict_Absent_exn_struct{char*tag;};struct Cyc_Relations_Reln;struct _union_Nmspace_Rel_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Abs_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_C_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Loc_n{int tag;int val;};union Cyc_Absyn_Nmspace{struct _union_Nmspace_Rel_n Rel_n;struct _union_Nmspace_Abs_n Abs_n;struct _union_Nmspace_C_n C_n;struct _union_Nmspace_Loc_n Loc_n;};struct _tuple0{union Cyc_Absyn_Nmspace f1;struct _fat_ptr*f2;};
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
# 954 "absyn.h"
int Cyc_Absyn_qvar_cmp(struct _tuple0*,struct _tuple0*);struct Cyc_Set_Set;
# 51 "set.h"
struct Cyc_Set_Set*Cyc_Set_empty(int(*cmp)(void*,void*));
# 63
struct Cyc_Set_Set*Cyc_Set_insert(struct Cyc_Set_Set*s,void*elt);extern char Cyc_Set_Absent[7U];struct Cyc_Set_Absent_exn_struct{char*tag;};
# 46 "graph.h"
void Cyc_Graph_print(struct Cyc___cycFILE*f,struct Cyc_Dict_Dict g,void(*nodeprint)(struct Cyc___cycFILE*,void*));
# 50
struct Cyc_Dict_Dict Cyc_Graph_empty(int(*cmp)(void*,void*));
# 53
struct Cyc_Dict_Dict Cyc_Graph_add_node(struct Cyc_Dict_Dict g,void*s);
# 59
struct Cyc_Dict_Dict Cyc_Graph_add_edges(struct Cyc_Dict_Dict g,void*s,struct Cyc_Set_Set*T);struct Cyc_PP_Ppstate;struct Cyc_PP_Out;struct Cyc_PP_Doc;struct Cyc_Absynpp_Params{int expand_typedefs;int qvar_to_Cids;int add_cyc_prefix;int to_VC;int decls_first;int rewrite_temp_tvars;int print_all_tvars;int print_all_kinds;int print_all_effects;int print_using_stmts;int print_externC_stmts;int print_full_evars;int print_zeroterm;int generate_line_directives;int use_curr_namespace;struct Cyc_List_List*curr_namespace;};
# 69 "absynpp.h"
struct _fat_ptr Cyc_Absynpp_qvar2string(struct _tuple0*);struct Cyc_Hashtable_Table;
# 39 "hashtable.h"
struct Cyc_Hashtable_Table*Cyc_Hashtable_create(int sz,int(*cmp)(void*,void*),int(*hash)(void*));
# 50
void Cyc_Hashtable_insert(struct Cyc_Hashtable_Table*t,void*key,void*val);
# 52
void*Cyc_Hashtable_lookup(struct Cyc_Hashtable_Table*t,void*key);
# 74
int Cyc_Hashtable_hash_string(struct _fat_ptr s);
# 47 "callgraph.cyc"
static struct Cyc_Set_Set*Cyc_Callgraph_cg_stmt(struct Cyc_Hashtable_Table*fds,struct Cyc_Absyn_Stmt*,struct Cyc_Set_Set*s);
static struct Cyc_Set_Set*Cyc_Callgraph_cg_exp(struct Cyc_Hashtable_Table*fds,struct Cyc_Absyn_Exp*,struct Cyc_Set_Set*s);
# 50
static struct Cyc_Set_Set*Cyc_Callgraph_cg_exps(struct Cyc_Hashtable_Table*fds,struct Cyc_List_List*es,struct Cyc_Set_Set*s){
for(0;es != 0;es=es->tl){
s=({Cyc_Callgraph_cg_exp(fds,(struct Cyc_Absyn_Exp*)es->hd,s);});}
return s;}struct _tuple12{struct Cyc_List_List*f1;struct Cyc_Absyn_Exp*f2;};
# 56
static struct Cyc_Set_Set*Cyc_Callgraph_cg_exp(struct Cyc_Hashtable_Table*fds,struct Cyc_Absyn_Exp*e,struct Cyc_Set_Set*s){
void*_stmttmp0=e->r;void*_tmp1=_stmttmp0;struct Cyc_Absyn_Stmt*_tmp2;struct Cyc_List_List*_tmp3;struct Cyc_List_List*_tmp4;struct Cyc_List_List*_tmp5;struct Cyc_List_List*_tmp6;struct Cyc_List_List*_tmp7;struct Cyc_List_List*_tmp8;struct Cyc_List_List*_tmp9;struct Cyc_Absyn_Exp*_tmpB;struct Cyc_Absyn_Exp*_tmpA;struct Cyc_Absyn_MallocInfo _tmpC;struct Cyc_List_List*_tmpE;struct Cyc_Absyn_Exp*_tmpD;struct Cyc_Absyn_Exp*_tmp11;struct Cyc_Absyn_Exp*_tmp10;struct Cyc_Absyn_Exp*_tmpF;struct Cyc_Absyn_Exp*_tmp12;struct Cyc_Absyn_Exp*_tmp13;struct Cyc_Absyn_Exp*_tmp14;struct Cyc_Absyn_Exp*_tmp15;struct Cyc_Absyn_Exp*_tmp16;struct Cyc_Absyn_Exp*_tmp17;struct Cyc_Absyn_Exp*_tmp18;struct Cyc_Absyn_Exp*_tmp19;struct Cyc_Absyn_Exp*_tmp1A;struct Cyc_Absyn_Exp*_tmp1B;struct Cyc_Absyn_Exp*_tmp1C;struct Cyc_Absyn_Exp*_tmp1E;struct Cyc_Absyn_Exp*_tmp1D;struct Cyc_Absyn_Exp*_tmp20;struct Cyc_Absyn_Exp*_tmp1F;struct Cyc_Absyn_Exp*_tmp22;struct Cyc_Absyn_Exp*_tmp21;struct Cyc_Absyn_Exp*_tmp24;struct Cyc_Absyn_Exp*_tmp23;struct Cyc_Absyn_Exp*_tmp26;struct Cyc_Absyn_Exp*_tmp25;struct Cyc_Absyn_Exp*_tmp28;struct Cyc_Absyn_Exp*_tmp27;struct Cyc_Absyn_Exp*_tmp2A;struct Cyc_Absyn_Exp*_tmp29;struct Cyc_Absyn_Exp*_tmp2C;struct Cyc_Absyn_Exp*_tmp2B;struct Cyc_List_List*_tmp2D;struct Cyc_Absyn_Vardecl*_tmp2E;struct Cyc_Absyn_Fndecl*_tmp2F;switch(*((int*)_tmp1)){case 1U: switch(*((int*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp1)->f1)){case 2U: _LL1: _tmp2F=((struct Cyc_Absyn_Funname_b_Absyn_Binding_struct*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp1)->f1)->f1;_LL2: {struct Cyc_Absyn_Fndecl*fd=_tmp2F;
# 59
struct _handler_cons _tmp30;_push_handler(& _tmp30);{int _tmp32=0;if(setjmp(_tmp30.handler))_tmp32=1;if(!_tmp32){
# 62
{struct Cyc_Set_Set*_tmp38=({struct Cyc_Set_Set*(*_tmp33)(struct Cyc_Set_Set*s,struct Cyc_Absyn_Fndecl*elt)=({struct Cyc_Set_Set*(*_tmp34)(struct Cyc_Set_Set*s,struct Cyc_Absyn_Fndecl*elt)=(struct Cyc_Set_Set*(*)(struct Cyc_Set_Set*s,struct Cyc_Absyn_Fndecl*elt))Cyc_Set_insert;_tmp34;});struct Cyc_Set_Set*_tmp35=s;struct Cyc_Absyn_Fndecl*_tmp36=({({struct Cyc_Absyn_Fndecl*(*_tmpC2)(struct Cyc_Hashtable_Table*t,struct _tuple0*key)=({struct Cyc_Absyn_Fndecl*(*_tmp37)(struct Cyc_Hashtable_Table*t,struct _tuple0*key)=(struct Cyc_Absyn_Fndecl*(*)(struct Cyc_Hashtable_Table*t,struct _tuple0*key))Cyc_Hashtable_lookup;_tmp37;});struct Cyc_Hashtable_Table*_tmpC1=fds;_tmpC2(_tmpC1,fd->name);});});_tmp33(_tmp35,_tmp36);});_npop_handler(0U);return _tmp38;};_pop_handler();}else{void*_tmp31=(void*)Cyc_Core_get_exn_thrown();void*_tmp39=_tmp31;void*_tmp3A;if(((struct Cyc_Core_Not_found_exn_struct*)_tmp39)->tag == Cyc_Core_Not_found){_LL5C: _LL5D:
# 64
 return s;}else{_LL5E: _tmp3A=_tmp39;_LL5F: {void*exn=_tmp3A;(int)_rethrow(exn);}}_LL5B:;}}}case 1U: _LL3: _tmp2E=((struct Cyc_Absyn_Global_b_Absyn_Binding_struct*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp1)->f1)->f1;_LL4: {struct Cyc_Absyn_Vardecl*vd=_tmp2E;
# 67
struct _handler_cons _tmp3B;_push_handler(& _tmp3B);{int _tmp3D=0;if(setjmp(_tmp3B.handler))_tmp3D=1;if(!_tmp3D){
{struct Cyc_Set_Set*_tmp43=({struct Cyc_Set_Set*(*_tmp3E)(struct Cyc_Set_Set*s,struct Cyc_Absyn_Fndecl*elt)=({struct Cyc_Set_Set*(*_tmp3F)(struct Cyc_Set_Set*s,struct Cyc_Absyn_Fndecl*elt)=(struct Cyc_Set_Set*(*)(struct Cyc_Set_Set*s,struct Cyc_Absyn_Fndecl*elt))Cyc_Set_insert;_tmp3F;});struct Cyc_Set_Set*_tmp40=s;struct Cyc_Absyn_Fndecl*_tmp41=({({struct Cyc_Absyn_Fndecl*(*_tmpC4)(struct Cyc_Hashtable_Table*t,struct _tuple0*key)=({struct Cyc_Absyn_Fndecl*(*_tmp42)(struct Cyc_Hashtable_Table*t,struct _tuple0*key)=(struct Cyc_Absyn_Fndecl*(*)(struct Cyc_Hashtable_Table*t,struct _tuple0*key))Cyc_Hashtable_lookup;_tmp42;});struct Cyc_Hashtable_Table*_tmpC3=fds;_tmpC4(_tmpC3,vd->name);});});_tmp3E(_tmp40,_tmp41);});_npop_handler(0U);return _tmp43;};_pop_handler();}else{void*_tmp3C=(void*)Cyc_Core_get_exn_thrown();void*_tmp44=_tmp3C;void*_tmp45;if(((struct Cyc_Core_Not_found_exn_struct*)_tmp44)->tag == Cyc_Core_Not_found){_LL61: _LL62:
# 70
 return s;}else{_LL63: _tmp45=_tmp44;_LL64: {void*exn=_tmp45;(int)_rethrow(exn);}}_LL60:;}}}default: _LL5: _LL6:
# 72
 goto _LL8;}case 0U: _LL7: _LL8: goto _LLA;case 2U: _LL9: _LLA: goto _LLC;case 17U: _LLB: _LLC:
 goto _LLE;case 18U: _LLD: _LLE: goto _LL10;case 19U: _LLF: _LL10: goto _LL12;case 32U: _LL11: _LL12:
 goto _LL14;case 33U: _LL13: _LL14: goto _LL16;case 40U: _LL15: _LL16: goto _LL18;case 41U: _LL17: _LL18:
 goto _LL1A;case 42U: _LL19: _LL1A: return s;case 3U: _LL1B: _tmp2D=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp1)->f2;_LL1C: {struct Cyc_List_List*es=_tmp2D;
return({Cyc_Callgraph_cg_exps(fds,es,s);});}case 7U: _LL1D: _tmp2B=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp1)->f1;_tmp2C=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp1)->f2;_LL1E: {struct Cyc_Absyn_Exp*e1=_tmp2B;struct Cyc_Absyn_Exp*e2=_tmp2C;
_tmp29=e1;_tmp2A=e2;goto _LL20;}case 8U: _LL1F: _tmp29=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp1)->f1;_tmp2A=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp1)->f2;_LL20: {struct Cyc_Absyn_Exp*e1=_tmp29;struct Cyc_Absyn_Exp*e2=_tmp2A;
_tmp27=e1;_tmp28=e2;goto _LL22;}case 34U: _LL21: _tmp27=((struct Cyc_Absyn_Spawn_e_Absyn_Raw_exp_struct*)_tmp1)->f1;_tmp28=((struct Cyc_Absyn_Spawn_e_Absyn_Raw_exp_struct*)_tmp1)->f2;_LL22: {struct Cyc_Absyn_Exp*e1=_tmp27;struct Cyc_Absyn_Exp*e2=_tmp28;
_tmp25=e1;_tmp26=e2;goto _LL24;}case 9U: _LL23: _tmp25=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp1)->f1;_tmp26=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp1)->f2;_LL24: {struct Cyc_Absyn_Exp*e1=_tmp25;struct Cyc_Absyn_Exp*e2=_tmp26;
_tmp23=e1;_tmp24=e2;goto _LL26;}case 36U: _LL25: _tmp23=((struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct*)_tmp1)->f1;_tmp24=((struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct*)_tmp1)->f2;_LL26: {struct Cyc_Absyn_Exp*e1=_tmp23;struct Cyc_Absyn_Exp*e2=_tmp24;
_tmp21=e1;_tmp22=e2;goto _LL28;}case 23U: _LL27: _tmp21=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp1)->f1;_tmp22=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp1)->f2;_LL28: {struct Cyc_Absyn_Exp*e1=_tmp21;struct Cyc_Absyn_Exp*e2=_tmp22;
_tmp1F=e1;_tmp20=e2;goto _LL2A;}case 27U: _LL29: _tmp1F=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmp1)->f2;_tmp20=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmp1)->f3;_LL2A: {struct Cyc_Absyn_Exp*e1=_tmp1F;struct Cyc_Absyn_Exp*e2=_tmp20;
_tmp1D=e1;_tmp1E=e2;goto _LL2C;}case 4U: _LL2B: _tmp1D=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp1)->f1;_tmp1E=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp1)->f3;_LL2C: {struct Cyc_Absyn_Exp*e1=_tmp1D;struct Cyc_Absyn_Exp*e2=_tmp1E;
return({struct Cyc_Set_Set*(*_tmp46)(struct Cyc_Hashtable_Table*fds,struct Cyc_Absyn_Exp*e,struct Cyc_Set_Set*s)=Cyc_Callgraph_cg_exp;struct Cyc_Hashtable_Table*_tmp47=fds;struct Cyc_Absyn_Exp*_tmp48=e2;struct Cyc_Set_Set*_tmp49=({Cyc_Callgraph_cg_exp(fds,e1,s);});_tmp46(_tmp47,_tmp48,_tmp49);});}case 11U: _LL2D: _tmp1C=((struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct*)_tmp1)->f1;_LL2E: {struct Cyc_Absyn_Exp*e=_tmp1C;
_tmp1B=e;goto _LL30;}case 12U: _LL2F: _tmp1B=((struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*)_tmp1)->f1;_LL30: {struct Cyc_Absyn_Exp*e=_tmp1B;
_tmp1A=e;goto _LL32;}case 13U: _LL31: _tmp1A=((struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*)_tmp1)->f1;_LL32: {struct Cyc_Absyn_Exp*e=_tmp1A;
_tmp19=e;goto _LL34;}case 14U: _LL33: _tmp19=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp1)->f2;_LL34: {struct Cyc_Absyn_Exp*e=_tmp19;
_tmp18=e;goto _LL36;}case 15U: _LL35: _tmp18=((struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct*)_tmp1)->f1;_LL36: {struct Cyc_Absyn_Exp*e=_tmp18;
_tmp17=e;goto _LL38;}case 20U: _LL37: _tmp17=((struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_tmp1)->f1;_LL38: {struct Cyc_Absyn_Exp*e=_tmp17;
_tmp16=e;goto _LL3A;}case 21U: _LL39: _tmp16=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp1)->f1;_LL3A: {struct Cyc_Absyn_Exp*e=_tmp16;
_tmp15=e;goto _LL3C;}case 22U: _LL3B: _tmp15=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp1)->f1;_LL3C: {struct Cyc_Absyn_Exp*e=_tmp15;
_tmp14=e;goto _LL3E;}case 39U: _LL3D: _tmp14=((struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct*)_tmp1)->f1;_LL3E: {struct Cyc_Absyn_Exp*e=_tmp14;
_tmp13=e;goto _LL40;}case 28U: _LL3F: _tmp13=((struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*)_tmp1)->f1;_LL40: {struct Cyc_Absyn_Exp*e=_tmp13;
_tmp12=e;goto _LL42;}case 5U: _LL41: _tmp12=((struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*)_tmp1)->f1;_LL42: {struct Cyc_Absyn_Exp*e=_tmp12;
return({Cyc_Callgraph_cg_exp(fds,e,s);});}case 6U: _LL43: _tmpF=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp1)->f1;_tmp10=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp1)->f2;_tmp11=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp1)->f3;_LL44: {struct Cyc_Absyn_Exp*e1=_tmpF;struct Cyc_Absyn_Exp*e2=_tmp10;struct Cyc_Absyn_Exp*e3=_tmp11;
# 97
return({struct Cyc_Set_Set*(*_tmp4A)(struct Cyc_Hashtable_Table*fds,struct Cyc_Absyn_Exp*e,struct Cyc_Set_Set*s)=Cyc_Callgraph_cg_exp;struct Cyc_Hashtable_Table*_tmp4B=fds;struct Cyc_Absyn_Exp*_tmp4C=e3;struct Cyc_Set_Set*_tmp4D=({struct Cyc_Set_Set*(*_tmp4E)(struct Cyc_Hashtable_Table*fds,struct Cyc_Absyn_Exp*e,struct Cyc_Set_Set*s)=Cyc_Callgraph_cg_exp;struct Cyc_Hashtable_Table*_tmp4F=fds;struct Cyc_Absyn_Exp*_tmp50=e2;struct Cyc_Set_Set*_tmp51=({Cyc_Callgraph_cg_exp(fds,e1,s);});_tmp4E(_tmp4F,_tmp50,_tmp51);});_tmp4A(_tmp4B,_tmp4C,_tmp4D);});}case 10U: _LL45: _tmpD=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp1)->f1;_tmpE=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp1)->f2;_LL46: {struct Cyc_Absyn_Exp*e=_tmpD;struct Cyc_List_List*es=_tmpE;
return({struct Cyc_Set_Set*(*_tmp52)(struct Cyc_Hashtable_Table*fds,struct Cyc_List_List*es,struct Cyc_Set_Set*s)=Cyc_Callgraph_cg_exps;struct Cyc_Hashtable_Table*_tmp53=fds;struct Cyc_List_List*_tmp54=es;struct Cyc_Set_Set*_tmp55=({Cyc_Callgraph_cg_exp(fds,e,s);});_tmp52(_tmp53,_tmp54,_tmp55);});}case 35U: _LL47: _tmpC=((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp1)->f1;_LL48: {struct Cyc_Absyn_MallocInfo mi=_tmpC;
# 100
_tmpA=mi.rgn;_tmpB=mi.num_elts;goto _LL4A;}case 16U: _LL49: _tmpA=((struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct*)_tmp1)->f1;_tmpB=((struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct*)_tmp1)->f2;_LL4A: {struct Cyc_Absyn_Exp*eopt=_tmpA;struct Cyc_Absyn_Exp*e=_tmpB;
# 102
return({struct Cyc_Set_Set*(*_tmp56)(struct Cyc_Hashtable_Table*fds,struct Cyc_Absyn_Exp*e,struct Cyc_Set_Set*s)=Cyc_Callgraph_cg_exp;struct Cyc_Hashtable_Table*_tmp57=fds;struct Cyc_Absyn_Exp*_tmp58=e;struct Cyc_Set_Set*_tmp59=eopt == 0?s:({Cyc_Callgraph_cg_exp(fds,eopt,s);});_tmp56(_tmp57,_tmp58,_tmp59);});}case 31U: _LL4B: _tmp9=((struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*)_tmp1)->f1;_LL4C: {struct Cyc_List_List*es=_tmp9;
_tmp8=es;goto _LL4E;}case 24U: _LL4D: _tmp8=((struct Cyc_Absyn_Tuple_e_Absyn_Raw_exp_struct*)_tmp1)->f1;_LL4E: {struct Cyc_List_List*es=_tmp8;
return({Cyc_Callgraph_cg_exps(fds,es,s);});}case 37U: _LL4F: _tmp7=((struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*)_tmp1)->f2;_LL50: {struct Cyc_List_List*dles=_tmp7;
_tmp6=dles;goto _LL52;}case 25U: _LL51: _tmp6=((struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct*)_tmp1)->f2;_LL52: {struct Cyc_List_List*dles=_tmp6;
_tmp5=dles;goto _LL54;}case 29U: _LL53: _tmp5=((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_tmp1)->f3;_LL54: {struct Cyc_List_List*dles=_tmp5;
_tmp4=dles;goto _LL56;}case 30U: _LL55: _tmp4=((struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct*)_tmp1)->f2;_LL56: {struct Cyc_List_List*dles=_tmp4;
_tmp3=dles;goto _LL58;}case 26U: _LL57: _tmp3=((struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*)_tmp1)->f1;_LL58: {struct Cyc_List_List*dles=_tmp3;
# 110
for(0;dles != 0;dles=dles->tl){
struct _tuple12*_stmttmp1=(struct _tuple12*)dles->hd;struct Cyc_Absyn_Exp*_tmp5A;_LL66: _tmp5A=_stmttmp1->f2;_LL67: {struct Cyc_Absyn_Exp*e=_tmp5A;
s=({Cyc_Callgraph_cg_exp(fds,e,s);});}}
# 114
return s;}default: _LL59: _tmp2=((struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct*)_tmp1)->f1;_LL5A: {struct Cyc_Absyn_Stmt*stmt=_tmp2;
return({Cyc_Callgraph_cg_stmt(fds,stmt,s);});}}_LL0:;}
# 119
static struct Cyc_Set_Set*Cyc_Callgraph_cg_decl(struct Cyc_Hashtable_Table*fds,struct Cyc_Absyn_Decl*d,struct Cyc_Set_Set*s){
void*_stmttmp2=d->r;void*_tmp5C=_stmttmp2;struct Cyc_Absyn_Exp*_tmp5E;struct Cyc_Absyn_Vardecl*_tmp5D;struct Cyc_List_List*_tmp5F;struct Cyc_Absyn_Exp*_tmp60;struct Cyc_Absyn_Vardecl*_tmp61;switch(*((int*)_tmp5C)){case 0U: _LL1: _tmp61=((struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_tmp5C)->f1;_LL2: {struct Cyc_Absyn_Vardecl*vd=_tmp61;
# 122
struct Cyc_Absyn_Exp*eopt=vd->initializer;
return eopt == 0?s:({Cyc_Callgraph_cg_exp(fds,eopt,s);});}case 2U: _LL3: _tmp60=((struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct*)_tmp5C)->f3;_LL4: {struct Cyc_Absyn_Exp*e=_tmp60;
return({Cyc_Callgraph_cg_exp(fds,e,s);});}case 3U: _LL5: _tmp5F=((struct Cyc_Absyn_Letv_d_Absyn_Raw_decl_struct*)_tmp5C)->f1;_LL6: {struct Cyc_List_List*vds=_tmp5F;
# 126
for(0;vds != 0;vds=vds->tl){
struct Cyc_Absyn_Exp*eopt=((struct Cyc_Absyn_Vardecl*)vds->hd)->initializer;
s=eopt == 0?s:({Cyc_Callgraph_cg_exp(fds,eopt,s);});}
# 130
return s;}case 4U: _LL7: _tmp5D=((struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*)_tmp5C)->f2;_tmp5E=((struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*)_tmp5C)->f3;_LL8: {struct Cyc_Absyn_Vardecl*vd=_tmp5D;struct Cyc_Absyn_Exp*eopt=_tmp5E;
# 132
s=vd->initializer == 0?s:({Cyc_Callgraph_cg_exp(fds,(struct Cyc_Absyn_Exp*)_check_null(vd->initializer),s);});
return eopt == 0?s:({Cyc_Callgraph_cg_exp(fds,eopt,s);});}default: _LL9: _LLA:
 return s;}_LL0:;}
# 138
static struct Cyc_Set_Set*Cyc_Callgraph_cg_stmt(struct Cyc_Hashtable_Table*fds,struct Cyc_Absyn_Stmt*stmt,struct Cyc_Set_Set*s){
void*_stmttmp3=stmt->r;void*_tmp63=_stmttmp3;struct Cyc_Absyn_Stmt*_tmp64;struct Cyc_List_List*_tmp66;struct Cyc_Absyn_Stmt*_tmp65;struct Cyc_Absyn_Exp*_tmp68;struct Cyc_Absyn_Stmt*_tmp67;struct Cyc_Absyn_Stmt*_tmp6A;struct Cyc_Absyn_Decl*_tmp69;struct Cyc_List_List*_tmp6B;struct Cyc_List_List*_tmp6D;struct Cyc_Absyn_Exp*_tmp6C;struct Cyc_Absyn_Stmt*_tmp71;struct Cyc_Absyn_Exp*_tmp70;struct Cyc_Absyn_Exp*_tmp6F;struct Cyc_Absyn_Exp*_tmp6E;struct Cyc_Absyn_Stmt*_tmp73;struct Cyc_Absyn_Exp*_tmp72;struct Cyc_Absyn_Stmt*_tmp76;struct Cyc_Absyn_Stmt*_tmp75;struct Cyc_Absyn_Exp*_tmp74;struct Cyc_Absyn_Exp*_tmp77;struct Cyc_Absyn_Stmt*_tmp79;struct Cyc_Absyn_Stmt*_tmp78;struct Cyc_Absyn_Exp*_tmp7A;switch(*((int*)_tmp63)){case 6U: _LL1: _LL2:
 goto _LL4;case 7U: _LL3: _LL4:
 goto _LL6;case 8U: _LL5: _LL6:
 goto _LL8;case 0U: _LL7: _LL8:
 return s;case 1U: _LL9: _tmp7A=((struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct*)_tmp63)->f1;_LLA: {struct Cyc_Absyn_Exp*e=_tmp7A;
return({Cyc_Callgraph_cg_exp(fds,e,s);});}case 2U: _LLB: _tmp78=((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_tmp63)->f1;_tmp79=((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_tmp63)->f2;_LLC: {struct Cyc_Absyn_Stmt*s1=_tmp78;struct Cyc_Absyn_Stmt*s2=_tmp79;
return({struct Cyc_Set_Set*(*_tmp7B)(struct Cyc_Hashtable_Table*fds,struct Cyc_Absyn_Stmt*stmt,struct Cyc_Set_Set*s)=Cyc_Callgraph_cg_stmt;struct Cyc_Hashtable_Table*_tmp7C=fds;struct Cyc_Absyn_Stmt*_tmp7D=s2;struct Cyc_Set_Set*_tmp7E=({Cyc_Callgraph_cg_stmt(fds,s1,s);});_tmp7B(_tmp7C,_tmp7D,_tmp7E);});}case 3U: _LLD: _tmp77=((struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct*)_tmp63)->f1;_LLE: {struct Cyc_Absyn_Exp*eopt=_tmp77;
return eopt != 0?({Cyc_Callgraph_cg_exp(fds,eopt,s);}): s;}case 4U: _LLF: _tmp74=((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmp63)->f1;_tmp75=((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmp63)->f2;_tmp76=((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmp63)->f3;_LL10: {struct Cyc_Absyn_Exp*e=_tmp74;struct Cyc_Absyn_Stmt*s1=_tmp75;struct Cyc_Absyn_Stmt*s2=_tmp76;
# 148
return({struct Cyc_Set_Set*(*_tmp7F)(struct Cyc_Hashtable_Table*fds,struct Cyc_Absyn_Stmt*stmt,struct Cyc_Set_Set*s)=Cyc_Callgraph_cg_stmt;struct Cyc_Hashtable_Table*_tmp80=fds;struct Cyc_Absyn_Stmt*_tmp81=s2;struct Cyc_Set_Set*_tmp82=({struct Cyc_Set_Set*(*_tmp83)(struct Cyc_Hashtable_Table*fds,struct Cyc_Absyn_Stmt*stmt,struct Cyc_Set_Set*s)=Cyc_Callgraph_cg_stmt;struct Cyc_Hashtable_Table*_tmp84=fds;struct Cyc_Absyn_Stmt*_tmp85=s1;struct Cyc_Set_Set*_tmp86=({Cyc_Callgraph_cg_exp(fds,e,s);});_tmp83(_tmp84,_tmp85,_tmp86);});_tmp7F(_tmp80,_tmp81,_tmp82);});}case 5U: _LL11: _tmp72=(((struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct*)_tmp63)->f1).f1;_tmp73=((struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct*)_tmp63)->f2;_LL12: {struct Cyc_Absyn_Exp*e=_tmp72;struct Cyc_Absyn_Stmt*s1=_tmp73;
return({struct Cyc_Set_Set*(*_tmp87)(struct Cyc_Hashtable_Table*fds,struct Cyc_Absyn_Stmt*stmt,struct Cyc_Set_Set*s)=Cyc_Callgraph_cg_stmt;struct Cyc_Hashtable_Table*_tmp88=fds;struct Cyc_Absyn_Stmt*_tmp89=s1;struct Cyc_Set_Set*_tmp8A=({Cyc_Callgraph_cg_exp(fds,e,s);});_tmp87(_tmp88,_tmp89,_tmp8A);});}case 9U: _LL13: _tmp6E=((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp63)->f1;_tmp6F=(((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp63)->f2).f1;_tmp70=(((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp63)->f3).f1;_tmp71=((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp63)->f4;_LL14: {struct Cyc_Absyn_Exp*e1=_tmp6E;struct Cyc_Absyn_Exp*e2=_tmp6F;struct Cyc_Absyn_Exp*e3=_tmp70;struct Cyc_Absyn_Stmt*s1=_tmp71;
# 151
return({struct Cyc_Set_Set*(*_tmp8B)(struct Cyc_Hashtable_Table*fds,struct Cyc_Absyn_Stmt*stmt,struct Cyc_Set_Set*s)=Cyc_Callgraph_cg_stmt;struct Cyc_Hashtable_Table*_tmp8C=fds;struct Cyc_Absyn_Stmt*_tmp8D=s1;struct Cyc_Set_Set*_tmp8E=({struct Cyc_Set_Set*(*_tmp8F)(struct Cyc_Hashtable_Table*fds,struct Cyc_Absyn_Exp*e,struct Cyc_Set_Set*s)=Cyc_Callgraph_cg_exp;struct Cyc_Hashtable_Table*_tmp90=fds;struct Cyc_Absyn_Exp*_tmp91=e3;struct Cyc_Set_Set*_tmp92=({struct Cyc_Set_Set*(*_tmp93)(struct Cyc_Hashtable_Table*fds,struct Cyc_Absyn_Exp*e,struct Cyc_Set_Set*s)=Cyc_Callgraph_cg_exp;struct Cyc_Hashtable_Table*_tmp94=fds;struct Cyc_Absyn_Exp*_tmp95=e2;struct Cyc_Set_Set*_tmp96=({Cyc_Callgraph_cg_exp(fds,e1,s);});_tmp93(_tmp94,_tmp95,_tmp96);});_tmp8F(_tmp90,_tmp91,_tmp92);});_tmp8B(_tmp8C,_tmp8D,_tmp8E);});}case 10U: _LL15: _tmp6C=((struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct*)_tmp63)->f1;_tmp6D=((struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct*)_tmp63)->f2;_LL16: {struct Cyc_Absyn_Exp*e=_tmp6C;struct Cyc_List_List*scs=_tmp6D;
# 153
s=({Cyc_Callgraph_cg_exp(fds,e,s);});
for(0;scs != 0;scs=scs->tl){
# 156
s=({Cyc_Callgraph_cg_stmt(fds,((struct Cyc_Absyn_Switch_clause*)scs->hd)->body,s);});}
return s;}case 11U: _LL17: _tmp6B=((struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct*)_tmp63)->f1;_LL18: {struct Cyc_List_List*es=_tmp6B;
return({Cyc_Callgraph_cg_exps(fds,es,s);});}case 12U: _LL19: _tmp69=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp63)->f1;_tmp6A=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp63)->f2;_LL1A: {struct Cyc_Absyn_Decl*d=_tmp69;struct Cyc_Absyn_Stmt*s1=_tmp6A;
return({struct Cyc_Set_Set*(*_tmp97)(struct Cyc_Hashtable_Table*fds,struct Cyc_Absyn_Stmt*stmt,struct Cyc_Set_Set*s)=Cyc_Callgraph_cg_stmt;struct Cyc_Hashtable_Table*_tmp98=fds;struct Cyc_Absyn_Stmt*_tmp99=s1;struct Cyc_Set_Set*_tmp9A=({Cyc_Callgraph_cg_decl(fds,d,s);});_tmp97(_tmp98,_tmp99,_tmp9A);});}case 14U: _LL1B: _tmp67=((struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct*)_tmp63)->f1;_tmp68=(((struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct*)_tmp63)->f2).f1;_LL1C: {struct Cyc_Absyn_Stmt*s1=_tmp67;struct Cyc_Absyn_Exp*e=_tmp68;
return({struct Cyc_Set_Set*(*_tmp9B)(struct Cyc_Hashtable_Table*fds,struct Cyc_Absyn_Exp*e,struct Cyc_Set_Set*s)=Cyc_Callgraph_cg_exp;struct Cyc_Hashtable_Table*_tmp9C=fds;struct Cyc_Absyn_Exp*_tmp9D=e;struct Cyc_Set_Set*_tmp9E=({Cyc_Callgraph_cg_stmt(fds,s1,s);});_tmp9B(_tmp9C,_tmp9D,_tmp9E);});}case 15U: _LL1D: _tmp65=((struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct*)_tmp63)->f1;_tmp66=((struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct*)_tmp63)->f2;_LL1E: {struct Cyc_Absyn_Stmt*s1=_tmp65;struct Cyc_List_List*scs=_tmp66;
# 162
s=({Cyc_Callgraph_cg_stmt(fds,s1,s);});
for(0;scs != 0;scs=scs->tl){
# 165
s=({Cyc_Callgraph_cg_stmt(fds,((struct Cyc_Absyn_Switch_clause*)scs->hd)->body,s);});}
return s;}default: _LL1F: _tmp64=((struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct*)_tmp63)->f2;_LL20: {struct Cyc_Absyn_Stmt*s1=_tmp64;
return({Cyc_Callgraph_cg_stmt(fds,s1,s);});}}_LL0:;}
# 171
static int Cyc_Callgraph_fndecl_cmp(struct Cyc_Absyn_Fndecl*fd1,struct Cyc_Absyn_Fndecl*fd2){return(int)fd1 - (int)fd2;}
# 173
static struct Cyc_Dict_Dict Cyc_Callgraph_cg_topdecls(struct Cyc_Hashtable_Table*fds,struct Cyc_Dict_Dict cg,struct Cyc_List_List*ds){
struct Cyc_Set_Set*mt=({({struct Cyc_Set_Set*(*_tmpAB)(int(*cmp)(struct Cyc_Absyn_Fndecl*,struct Cyc_Absyn_Fndecl*))=(struct Cyc_Set_Set*(*)(int(*cmp)(struct Cyc_Absyn_Fndecl*,struct Cyc_Absyn_Fndecl*)))Cyc_Set_empty;_tmpAB;})(Cyc_Callgraph_fndecl_cmp);});
{struct Cyc_List_List*_tmpA1=ds;struct Cyc_List_List*ds=_tmpA1;for(0;ds != 0;ds=ds->tl){
void*_stmttmp4=((struct Cyc_Absyn_Decl*)ds->hd)->r;void*_tmpA2=_stmttmp4;struct Cyc_List_List*_tmpA3;struct Cyc_List_List*_tmpA4;struct Cyc_Absyn_Fndecl*_tmpA5;switch(*((int*)_tmpA2)){case 1U: _LL1: _tmpA5=((struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*)_tmpA2)->f1;_LL2: {struct Cyc_Absyn_Fndecl*fd=_tmpA5;
# 178
cg=({struct Cyc_Dict_Dict(*_tmpA6)(struct Cyc_Dict_Dict g,struct Cyc_Absyn_Fndecl*s,struct Cyc_Set_Set*T)=({struct Cyc_Dict_Dict(*_tmpA7)(struct Cyc_Dict_Dict g,struct Cyc_Absyn_Fndecl*s,struct Cyc_Set_Set*T)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict g,struct Cyc_Absyn_Fndecl*s,struct Cyc_Set_Set*T))Cyc_Graph_add_edges;_tmpA7;});struct Cyc_Dict_Dict _tmpA8=cg;struct Cyc_Absyn_Fndecl*_tmpA9=fd;struct Cyc_Set_Set*_tmpAA=({Cyc_Callgraph_cg_stmt(fds,fd->body,mt);});_tmpA6(_tmpA8,_tmpA9,_tmpAA);});
goto _LL0;}case 10U: _LL3: _tmpA4=((struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct*)_tmpA2)->f2;_LL4: {struct Cyc_List_List*ds=_tmpA4;
_tmpA3=ds;goto _LL6;}case 9U: _LL5: _tmpA3=((struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct*)_tmpA2)->f2;_LL6: {struct Cyc_List_List*ds=_tmpA3;
cg=({Cyc_Callgraph_cg_topdecls(fds,cg,ds);});goto _LL0;}default: _LL7: _LL8:
 goto _LL0;}_LL0:;}}
# 185
return cg;}
# 189
static struct Cyc_Dict_Dict Cyc_Callgraph_enter_fndecls(struct Cyc_Hashtable_Table*fds,struct Cyc_Dict_Dict cg,struct Cyc_List_List*ds){
# 191
{struct Cyc_List_List*_tmpAD=ds;struct Cyc_List_List*ds=_tmpAD;for(0;ds != 0;ds=ds->tl){
void*_stmttmp5=((struct Cyc_Absyn_Decl*)ds->hd)->r;void*_tmpAE=_stmttmp5;struct Cyc_List_List*_tmpAF;struct Cyc_List_List*_tmpB0;struct Cyc_Absyn_Fndecl*_tmpB1;switch(*((int*)_tmpAE)){case 1U: _LL1: _tmpB1=((struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*)_tmpAE)->f1;_LL2: {struct Cyc_Absyn_Fndecl*f=_tmpB1;
# 194
({({void(*_tmpC7)(struct Cyc_Hashtable_Table*t,struct _tuple0*key,struct Cyc_Absyn_Fndecl*val)=({void(*_tmpB2)(struct Cyc_Hashtable_Table*t,struct _tuple0*key,struct Cyc_Absyn_Fndecl*val)=(void(*)(struct Cyc_Hashtable_Table*t,struct _tuple0*key,struct Cyc_Absyn_Fndecl*val))Cyc_Hashtable_insert;_tmpB2;});struct Cyc_Hashtable_Table*_tmpC6=fds;struct _tuple0*_tmpC5=f->name;_tmpC7(_tmpC6,_tmpC5,f);});});
cg=({({struct Cyc_Dict_Dict(*_tmpC9)(struct Cyc_Dict_Dict g,struct Cyc_Absyn_Fndecl*s)=({struct Cyc_Dict_Dict(*_tmpB3)(struct Cyc_Dict_Dict g,struct Cyc_Absyn_Fndecl*s)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict g,struct Cyc_Absyn_Fndecl*s))Cyc_Graph_add_node;_tmpB3;});struct Cyc_Dict_Dict _tmpC8=cg;_tmpC9(_tmpC8,f);});});
goto _LL0;}case 10U: _LL3: _tmpB0=((struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct*)_tmpAE)->f2;_LL4: {struct Cyc_List_List*ds=_tmpB0;
_tmpAF=ds;goto _LL6;}case 9U: _LL5: _tmpAF=((struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct*)_tmpAE)->f2;_LL6: {struct Cyc_List_List*ds=_tmpAF;
# 199
cg=({Cyc_Callgraph_enter_fndecls(fds,cg,ds);});goto _LL0;}default: _LL7: _LL8:
 goto _LL0;}_LL0:;}}
# 203
return cg;}
# 206
static int Cyc_Callgraph_hash_qvar(struct _tuple0*q){return({Cyc_Hashtable_hash_string(*(*q).f2);});}
static int Cyc_Callgraph_hash_fndecl(struct Cyc_Absyn_Fndecl*fd){return(int)fd;}struct Cyc_Dict_Dict Cyc_Callgraph_compute_callgraph(struct Cyc_List_List*ds){
# 211
struct Cyc_Hashtable_Table*fd=({({struct Cyc_Hashtable_Table*(*_tmpB8)(int sz,int(*cmp)(struct _tuple0*,struct _tuple0*),int(*hash)(struct _tuple0*))=(struct Cyc_Hashtable_Table*(*)(int sz,int(*cmp)(struct _tuple0*,struct _tuple0*),int(*hash)(struct _tuple0*)))Cyc_Hashtable_create;_tmpB8;})(51,Cyc_Absyn_qvar_cmp,Cyc_Callgraph_hash_qvar);});
struct Cyc_Dict_Dict cg=({({struct Cyc_Dict_Dict(*_tmpB7)(int(*cmp)(struct Cyc_Absyn_Fndecl*,struct Cyc_Absyn_Fndecl*))=(struct Cyc_Dict_Dict(*)(int(*cmp)(struct Cyc_Absyn_Fndecl*,struct Cyc_Absyn_Fndecl*)))Cyc_Graph_empty;_tmpB7;})(Cyc_Callgraph_fndecl_cmp);});
cg=({Cyc_Callgraph_enter_fndecls(fd,cg,ds);});
# 215
cg=({Cyc_Callgraph_cg_topdecls(fd,cg,ds);});
return cg;}
# 219
static void Cyc_Callgraph_print_fndecl(struct Cyc___cycFILE*f,struct Cyc_Absyn_Fndecl*fd){
({struct Cyc_String_pa_PrintArg_struct _tmpBC=({struct Cyc_String_pa_PrintArg_struct _tmpC0;_tmpC0.tag=0U,({struct _fat_ptr _tmpCA=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(fd->name);}));_tmpC0.f1=_tmpCA;});_tmpC0;});void*_tmpBA[1U];_tmpBA[0]=& _tmpBC;({struct Cyc___cycFILE*_tmpCC=f;struct _fat_ptr _tmpCB=({const char*_tmpBB="%s ";_tag_fat(_tmpBB,sizeof(char),4U);});Cyc_fprintf(_tmpCC,_tmpCB,_tag_fat(_tmpBA,sizeof(void*),1U));});});}
# 223
void Cyc_Callgraph_print_callgraph(struct Cyc___cycFILE*f,struct Cyc_Dict_Dict cg){
({({void(*_tmpCE)(struct Cyc___cycFILE*f,struct Cyc_Dict_Dict g,void(*nodeprint)(struct Cyc___cycFILE*,struct Cyc_Absyn_Fndecl*))=({void(*_tmpBE)(struct Cyc___cycFILE*f,struct Cyc_Dict_Dict g,void(*nodeprint)(struct Cyc___cycFILE*,struct Cyc_Absyn_Fndecl*))=(void(*)(struct Cyc___cycFILE*f,struct Cyc_Dict_Dict g,void(*nodeprint)(struct Cyc___cycFILE*,struct Cyc_Absyn_Fndecl*)))Cyc_Graph_print;_tmpBE;});struct Cyc___cycFILE*_tmpCD=f;_tmpCE(_tmpCD,cg,Cyc_Callgraph_print_fndecl);});});}

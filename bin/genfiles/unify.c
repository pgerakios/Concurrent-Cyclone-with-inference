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
extern struct _RegionHandle*Cyc_Core_unique_region;struct Cyc_Core_DynamicRegion;struct Cyc_Core_NewDynamicRegion{struct Cyc_Core_DynamicRegion*key;};struct Cyc_Core_ThinRes{void*arr;unsigned nelts;};struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};extern char Cyc_List_List_mismatch[14U];struct Cyc_List_List_mismatch_exn_struct{char*tag;};
# 190 "list.h"
struct Cyc_List_List*Cyc_List_rappend(struct _RegionHandle*,struct Cyc_List_List*x,struct Cyc_List_List*y);extern char Cyc_List_Nth[4U];struct Cyc_List_Nth_exn_struct{char*tag;};
# 322
int Cyc_List_mem(int(*compare)(void*,void*),struct Cyc_List_List*l,void*x);struct Cyc___cycFILE;
# 51 "cycboot.h"
extern struct Cyc___cycFILE*Cyc_stdout;
# 53
extern struct Cyc___cycFILE*Cyc_stderr;struct Cyc_String_pa_PrintArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Int_pa_PrintArg_struct{int tag;unsigned long f1;};struct Cyc_Double_pa_PrintArg_struct{int tag;double f1;};struct Cyc_LongDouble_pa_PrintArg_struct{int tag;long double f1;};struct Cyc_ShortPtr_pa_PrintArg_struct{int tag;short*f1;};struct Cyc_IntPtr_pa_PrintArg_struct{int tag;unsigned long*f1;};
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
int Cyc_strptrcmp(struct _fat_ptr*s1,struct _fat_ptr*s2);struct Cyc_PP_Ppstate;struct Cyc_PP_Out;struct Cyc_PP_Doc;struct Cyc_Position_Error;
# 47 "position.h"
extern int Cyc_Position_num_errors;
extern int Cyc_Position_max_errors;struct Cyc_Iter_Iter{void*env;int(*next)(void*env,void*dest);};struct Cyc_Dict_T;struct Cyc_Dict_Dict{int(*rel)(void*,void*);struct _RegionHandle*r;const struct Cyc_Dict_T*t;};extern char Cyc_Dict_Present[8U];struct Cyc_Dict_Present_exn_struct{char*tag;};extern char Cyc_Dict_Absent[7U];struct Cyc_Dict_Absent_exn_struct{char*tag;};struct Cyc_Relations_Reln;struct _union_Nmspace_Rel_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Abs_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_C_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Loc_n{int tag;int val;};union Cyc_Absyn_Nmspace{struct _union_Nmspace_Rel_n Rel_n;struct _union_Nmspace_Abs_n Abs_n;struct _union_Nmspace_C_n C_n;struct _union_Nmspace_Loc_n Loc_n;};struct _tuple0{union Cyc_Absyn_Nmspace f1;struct _fat_ptr*f2;};
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
# 968 "absyn.h"
void*Cyc_Absyn_compress_kb(void*);
# 997
void*Cyc_Absyn_var_type(struct Cyc_Absyn_Tvar*);
# 1026
void*Cyc_Absyn_bounds_one();
# 1285
int Cyc_Absyn_get_debug();
# 1344
int Cyc_Absyn_unify_rgneffects(struct Cyc_List_List*l1,struct Cyc_List_List*l2,void(*uit)(void*,void*));
# 1381
struct _fat_ptr Cyc_Absyn_throws2string(struct Cyc_List_List*t);
# 1383
int Cyc_Absyn_equals_throws(struct Cyc_List_List*t1,struct Cyc_List_List*t2);struct Cyc_Absynpp_Params{int expand_typedefs;int qvar_to_Cids;int add_cyc_prefix;int to_VC;int decls_first;int rewrite_temp_tvars;int print_all_tvars;int print_all_kinds;int print_all_effects;int print_using_stmts;int print_externC_stmts;int print_full_evars;int print_zeroterm;int generate_line_directives;int use_curr_namespace;struct Cyc_List_List*curr_namespace;};
# 53 "absynpp.h"
void Cyc_Absynpp_set_params(struct Cyc_Absynpp_Params*fs);
# 55
extern struct Cyc_Absynpp_Params Cyc_Absynpp_tc_params_r;
# 62
struct _fat_ptr Cyc_Absynpp_typ2string(void*);
# 64
struct _fat_ptr Cyc_Absynpp_kind2string(struct Cyc_Absyn_Kind*);
# 74
struct _fat_ptr Cyc_Absynpp_tvar2string(struct Cyc_Absyn_Tvar*);
struct _fat_ptr Cyc_Absynpp_eff2string(void*t);
# 41 "evexp.h"
int Cyc_Evexp_same_const_exp(struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2);struct _union_RelnOp_RConst{int tag;unsigned val;};struct _union_RelnOp_RVar{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RNumelts{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RType{int tag;void*val;};struct _union_RelnOp_RParam{int tag;unsigned val;};struct _union_RelnOp_RParamNumelts{int tag;unsigned val;};struct _union_RelnOp_RReturn{int tag;unsigned val;};union Cyc_Relations_RelnOp{struct _union_RelnOp_RConst RConst;struct _union_RelnOp_RVar RVar;struct _union_RelnOp_RNumelts RNumelts;struct _union_RelnOp_RType RType;struct _union_RelnOp_RParam RParam;struct _union_RelnOp_RParamNumelts RParamNumelts;struct _union_RelnOp_RReturn RReturn;};
# 50 "relations-ap.h"
enum Cyc_Relations_Relation{Cyc_Relations_Req =0U,Cyc_Relations_Rneq =1U,Cyc_Relations_Rlte =2U,Cyc_Relations_Rlt =3U};struct Cyc_Relations_Reln{union Cyc_Relations_RelnOp rop1;enum Cyc_Relations_Relation relation;union Cyc_Relations_RelnOp rop2;};
# 131
int Cyc_Relations_check_logical_implication(struct Cyc_List_List*r1,struct Cyc_List_List*r2);struct Cyc_RgnOrder_RgnPO;extern char Cyc_Tcenv_Env_error[10U];struct Cyc_Tcenv_Env_error_exn_struct{char*tag;};struct Cyc_Tcenv_Genv{struct Cyc_Dict_Dict aggrdecls;struct Cyc_Dict_Dict datatypedecls;struct Cyc_Dict_Dict enumdecls;struct Cyc_Dict_Dict typedefs;struct Cyc_Dict_Dict ordinaries;};struct Cyc_Tcenv_Fenv;struct Cyc_Tcenv_Tenv{struct Cyc_List_List*ns;struct Cyc_Tcenv_Genv*ae;struct Cyc_Tcenv_Fenv*le;int allow_valueof: 1;int in_extern_c_include: 1;int in_tempest: 1;int tempest_generalize: 1;};
# 90 "tcenv.h"
enum Cyc_Tcenv_NewStatus{Cyc_Tcenv_NoneNew =0U,Cyc_Tcenv_InNew =1U,Cyc_Tcenv_InNewAggr =2U};
# 30 "tcutil.h"
void*Cyc_Tcutil_impos(struct _fat_ptr,struct _fat_ptr);
# 103
int Cyc_Tcutil_kind_leq(struct Cyc_Absyn_Kind*,struct Cyc_Absyn_Kind*);
# 108
struct Cyc_Absyn_Kind*Cyc_Tcutil_tvar_kind(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Kind*def);
struct Cyc_Absyn_Kind*Cyc_Tcutil_type_kind(void*);
void*Cyc_Tcutil_compress(void*);
# 141
extern struct Cyc_Absyn_Kind Cyc_Tcutil_bk;
# 191
void*Cyc_Tcutil_rsubstitute(struct _RegionHandle*,struct Cyc_List_List*,void*);
# 200
int Cyc_Tcutil_subset_effect(int may_constrain_evars,void*e1,void*e2);
# 257
void*Cyc_Tcutil_normalize_effect(void*e);
# 271
int Cyc_Tcutil_same_atts(struct Cyc_List_List*,struct Cyc_List_List*);
# 275
int Cyc_Tcutil_equiv_fn_atts(struct Cyc_List_List*a1,struct Cyc_List_List*a2);
# 322
int Cyc_Tcutil_fast_tvar_cmp(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*);
# 326
int Cyc_Tcutil_same_rgn_po(struct Cyc_List_List*,struct Cyc_List_List*);
int Cyc_Tcutil_tycon_cmp(void*,void*);
# 28 "unify.h"
int Cyc_Unify_unify_kindbound(void*,void*);
int Cyc_Unify_unify(void*,void*);
# 32
void Cyc_Unify_occurs(void*evar,struct _RegionHandle*r,struct Cyc_List_List*env,void*t);char Cyc_Unify_Unify[6U]="Unify";struct Cyc_Unify_Unify_exn_struct{char*tag;};
# 34 "unify.cyc"
struct Cyc_Unify_Unify_exn_struct Cyc_Unify_Unify_val={Cyc_Unify_Unify};struct _tuple12{void*f1;void*f2;};
# 37
static struct _tuple12 Cyc_Unify_ts_failure={0,0};struct _tuple13{int f1;int f2;};
static struct _tuple13 Cyc_Unify_tqs_const={0,0};
static struct _fat_ptr Cyc_Unify_failure_reason={(void*)0,(void*)0,(void*)(0 + 0)};
# 41
static int Cyc_Unify_checking_pointer_effect=0;
# 43
static void Cyc_Unify_fail_because(struct _fat_ptr reason){
Cyc_Unify_failure_reason=reason;
(int)_throw(& Cyc_Unify_Unify_val);}
# 51
void Cyc_Unify_explain_failure(){
if(Cyc_Position_num_errors >= Cyc_Position_max_errors)
return;
# 52
({Cyc_fflush(Cyc_stderr);});
# 57
if(({({struct _fat_ptr _tmp16C=({const char*_tmp1="(qualifiers don't match)";_tag_fat(_tmp1,sizeof(char),25U);});Cyc_strcmp(_tmp16C,(struct _fat_ptr)Cyc_Unify_failure_reason);});})== 0){
({struct Cyc_String_pa_PrintArg_struct _tmp4=({struct Cyc_String_pa_PrintArg_struct _tmp143;_tmp143.tag=0U,_tmp143.f1=(struct _fat_ptr)((struct _fat_ptr)Cyc_Unify_failure_reason);_tmp143;});void*_tmp2[1U];_tmp2[0]=& _tmp4;({struct Cyc___cycFILE*_tmp16E=Cyc_stderr;struct _fat_ptr _tmp16D=({const char*_tmp3="  %s\n";_tag_fat(_tmp3,sizeof(char),6U);});Cyc_fprintf(_tmp16E,_tmp16D,_tag_fat(_tmp2,sizeof(void*),1U));});});
return;}
# 57
if(({({struct _fat_ptr _tmp16F=({const char*_tmp5="(function effects do not match)";_tag_fat(_tmp5,sizeof(char),32U);});Cyc_strcmp(_tmp16F,(struct _fat_ptr)Cyc_Unify_failure_reason);});})== 0){
# 63
struct Cyc_Absynpp_Params p=Cyc_Absynpp_tc_params_r;
p.print_all_effects=1;
({Cyc_Absynpp_set_params(& p);});}{
# 57
void*_tmp7;void*_tmp6;_LL1: _tmp6=Cyc_Unify_ts_failure.f1;_tmp7=Cyc_Unify_ts_failure.f2;_LL2: {void*t1f=_tmp6;void*t2f=_tmp7;
# 68
struct _fat_ptr s1=(unsigned)t1f?({Cyc_Absynpp_typ2string(t1f);}):({const char*_tmp28="<?>";_tag_fat(_tmp28,sizeof(char),4U);});
struct _fat_ptr s2=(unsigned)t2f?({Cyc_Absynpp_typ2string(t2f);}):({const char*_tmp27="<?>";_tag_fat(_tmp27,sizeof(char),4U);});
int pos=2;
({struct Cyc_String_pa_PrintArg_struct _tmpA=({struct Cyc_String_pa_PrintArg_struct _tmp144;_tmp144.tag=0U,_tmp144.f1=(struct _fat_ptr)((struct _fat_ptr)s1);_tmp144;});void*_tmp8[1U];_tmp8[0]=& _tmpA;({struct Cyc___cycFILE*_tmp171=Cyc_stderr;struct _fat_ptr _tmp170=({const char*_tmp9="  %s";_tag_fat(_tmp9,sizeof(char),5U);});Cyc_fprintf(_tmp171,_tmp170,_tag_fat(_tmp8,sizeof(void*),1U));});});
pos +=_get_fat_size(s1,sizeof(char));
if(pos + 5 >= 80){
({void*_tmpB=0U;({struct Cyc___cycFILE*_tmp173=Cyc_stderr;struct _fat_ptr _tmp172=({const char*_tmpC="\n\t";_tag_fat(_tmpC,sizeof(char),3U);});Cyc_fprintf(_tmp173,_tmp172,_tag_fat(_tmpB,sizeof(void*),0U));});});
pos=8;}else{
# 77
({void*_tmpD=0U;({struct Cyc___cycFILE*_tmp175=Cyc_stderr;struct _fat_ptr _tmp174=({const char*_tmpE=" ";_tag_fat(_tmpE,sizeof(char),2U);});Cyc_fprintf(_tmp175,_tmp174,_tag_fat(_tmpD,sizeof(void*),0U));});});
++ pos;}
# 80
({void*_tmpF=0U;({struct Cyc___cycFILE*_tmp177=Cyc_stderr;struct _fat_ptr _tmp176=({const char*_tmp10="and ";_tag_fat(_tmp10,sizeof(char),5U);});Cyc_fprintf(_tmp177,_tmp176,_tag_fat(_tmpF,sizeof(void*),0U));});});
pos +=4;
if((unsigned)pos + _get_fat_size(s2,sizeof(char))>= (unsigned)80){
({void*_tmp11=0U;({struct Cyc___cycFILE*_tmp179=Cyc_stderr;struct _fat_ptr _tmp178=({const char*_tmp12="\n\t";_tag_fat(_tmp12,sizeof(char),3U);});Cyc_fprintf(_tmp179,_tmp178,_tag_fat(_tmp11,sizeof(void*),0U));});});
pos=8;}
# 82
({struct Cyc_String_pa_PrintArg_struct _tmp15=({struct Cyc_String_pa_PrintArg_struct _tmp145;_tmp145.tag=0U,_tmp145.f1=(struct _fat_ptr)((struct _fat_ptr)s2);_tmp145;});void*_tmp13[1U];_tmp13[0]=& _tmp15;({struct Cyc___cycFILE*_tmp17B=Cyc_stderr;struct _fat_ptr _tmp17A=({const char*_tmp14="%s ";_tag_fat(_tmp14,sizeof(char),4U);});Cyc_fprintf(_tmp17B,_tmp17A,_tag_fat(_tmp13,sizeof(void*),1U));});});
# 87
pos +=_get_fat_size(s2,sizeof(char))+ (unsigned)1;
if(pos + 17 >= 80){
({void*_tmp16=0U;({struct Cyc___cycFILE*_tmp17D=Cyc_stderr;struct _fat_ptr _tmp17C=({const char*_tmp17="\n\t";_tag_fat(_tmp17,sizeof(char),3U);});Cyc_fprintf(_tmp17D,_tmp17C,_tag_fat(_tmp16,sizeof(void*),0U));});});
pos=8;}
# 88
({struct Cyc_String_pa_PrintArg_struct _tmp1A=({struct Cyc_String_pa_PrintArg_struct _tmp147;_tmp147.tag=0U,({
# 92
struct _fat_ptr _tmp17E=(struct _fat_ptr)((struct _fat_ptr)({struct _fat_ptr(*_tmp1E)(struct Cyc_Absyn_Kind*)=Cyc_Absynpp_kind2string;struct Cyc_Absyn_Kind*_tmp1F=({Cyc_Tcutil_type_kind((void*)_check_null(t1f));});_tmp1E(_tmp1F);}));_tmp147.f1=_tmp17E;});_tmp147;});struct Cyc_String_pa_PrintArg_struct _tmp1B=({struct Cyc_String_pa_PrintArg_struct _tmp146;_tmp146.tag=0U,({
struct _fat_ptr _tmp17F=(struct _fat_ptr)((struct _fat_ptr)({struct _fat_ptr(*_tmp1C)(struct Cyc_Absyn_Kind*)=Cyc_Absynpp_kind2string;struct Cyc_Absyn_Kind*_tmp1D=({Cyc_Tcutil_type_kind((void*)_check_null(t2f));});_tmp1C(_tmp1D);}));_tmp146.f1=_tmp17F;});_tmp146;});void*_tmp18[2U];_tmp18[0]=& _tmp1A,_tmp18[1]=& _tmp1B;({struct Cyc___cycFILE*_tmp181=Cyc_stderr;struct _fat_ptr _tmp180=({const char*_tmp19="are incompatible (kinds k1 = %s k2 = %s).";_tag_fat(_tmp19,sizeof(char),42U);});Cyc_fprintf(_tmp181,_tmp180,_tag_fat(_tmp18,sizeof(void*),2U));});});
pos +=17;
if(({char*_tmp182=(char*)Cyc_Unify_failure_reason.curr;_tmp182 != (char*)(_tag_fat(0,0,0)).curr;})){
if(({unsigned long _tmp183=(unsigned long)pos;_tmp183 + ({Cyc_strlen((struct _fat_ptr)Cyc_Unify_failure_reason);});})>= (unsigned long)80)
({void*_tmp20=0U;({struct Cyc___cycFILE*_tmp185=Cyc_stderr;struct _fat_ptr _tmp184=({const char*_tmp21="\n\t";_tag_fat(_tmp21,sizeof(char),3U);});Cyc_fprintf(_tmp185,_tmp184,_tag_fat(_tmp20,sizeof(void*),0U));});});
# 99
({struct Cyc_String_pa_PrintArg_struct _tmp24=({struct Cyc_String_pa_PrintArg_struct _tmp148;_tmp148.tag=0U,_tmp148.f1=(struct _fat_ptr)((struct _fat_ptr)Cyc_Unify_failure_reason);_tmp148;});void*_tmp22[1U];_tmp22[0]=& _tmp24;({struct Cyc___cycFILE*_tmp187=Cyc_stderr;struct _fat_ptr _tmp186=({const char*_tmp23="%s";_tag_fat(_tmp23,sizeof(char),3U);});Cyc_fprintf(_tmp187,_tmp186,_tag_fat(_tmp22,sizeof(void*),1U));});});}
# 95
({void*_tmp25=0U;({struct Cyc___cycFILE*_tmp189=Cyc_stderr;struct _fat_ptr _tmp188=({const char*_tmp26="\n";_tag_fat(_tmp26,sizeof(char),2U);});Cyc_fprintf(_tmp189,_tmp188,_tag_fat(_tmp25,sizeof(void*),0U));});});
# 102
({Cyc_fflush(Cyc_stderr);});}}}
# 107
static int Cyc_Unify_check_logical_equivalence(struct Cyc_List_List*r1,struct Cyc_List_List*r2){
if(r1 == r2)return 1;return
({Cyc_Relations_check_logical_implication(r1,r2);})&&({Cyc_Relations_check_logical_implication(r2,r1);});}
# 107
int Cyc_Unify_unify_kindbound(void*kb1,void*kb2){
# 114
struct _tuple12 _stmttmp0=({struct _tuple12 _tmp149;({void*_tmp18B=({Cyc_Absyn_compress_kb(kb1);});_tmp149.f1=_tmp18B;}),({void*_tmp18A=({Cyc_Absyn_compress_kb(kb2);});_tmp149.f2=_tmp18A;});_tmp149;});struct _tuple12 _tmp2B=_stmttmp0;void*_tmp2D;struct Cyc_Core_Opt**_tmp2C;struct Cyc_Core_Opt**_tmp2F;void*_tmp2E;struct Cyc_Absyn_Kind*_tmp33;struct Cyc_Core_Opt**_tmp32;struct Cyc_Absyn_Kind*_tmp31;struct Cyc_Core_Opt**_tmp30;struct Cyc_Absyn_Kind*_tmp36;struct Cyc_Absyn_Kind*_tmp35;struct Cyc_Core_Opt**_tmp34;struct Cyc_Absyn_Kind*_tmp39;struct Cyc_Core_Opt**_tmp38;struct Cyc_Absyn_Kind*_tmp37;struct Cyc_Absyn_Kind*_tmp3B;struct Cyc_Absyn_Kind*_tmp3A;switch(*((int*)_tmp2B.f1)){case 0U: switch(*((int*)_tmp2B.f2)){case 0U: _LL1: _tmp3A=((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp2B.f1)->f1;_tmp3B=((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp2B.f2)->f1;_LL2: {struct Cyc_Absyn_Kind*k1=_tmp3A;struct Cyc_Absyn_Kind*k2=_tmp3B;
return k1 == k2;}case 2U: _LL5: _tmp37=((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp2B.f1)->f1;_tmp38=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp2B.f2)->f1;_tmp39=((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp2B.f2)->f2;_LL6: {struct Cyc_Absyn_Kind*k1=_tmp37;struct Cyc_Core_Opt**x=_tmp38;struct Cyc_Absyn_Kind*k2=_tmp39;
# 122
if(({Cyc_Tcutil_kind_leq(k1,k2);})){
({struct Cyc_Core_Opt*_tmp18C=({struct Cyc_Core_Opt*_tmp3D=_cycalloc(sizeof(*_tmp3D));_tmp3D->v=kb1;_tmp3D;});*x=_tmp18C;});
return 1;}else{
return 0;}}default: goto _LLB;}case 2U: switch(*((int*)_tmp2B.f2)){case 0U: _LL3: _tmp34=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp2B.f1)->f1;_tmp35=((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp2B.f1)->f2;_tmp36=((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp2B.f2)->f1;_LL4: {struct Cyc_Core_Opt**x=_tmp34;struct Cyc_Absyn_Kind*k2=_tmp35;struct Cyc_Absyn_Kind*k1=_tmp36;
# 117
if(({Cyc_Tcutil_kind_leq(k1,k2);})){
({struct Cyc_Core_Opt*_tmp18D=({struct Cyc_Core_Opt*_tmp3C=_cycalloc(sizeof(*_tmp3C));_tmp3C->v=kb2;_tmp3C;});*x=_tmp18D;});
return 1;}else{
return 0;}}case 2U: _LL7: _tmp30=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp2B.f1)->f1;_tmp31=((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp2B.f1)->f2;_tmp32=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp2B.f2)->f1;_tmp33=((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp2B.f2)->f2;_LL8: {struct Cyc_Core_Opt**x=_tmp30;struct Cyc_Absyn_Kind*k1=_tmp31;struct Cyc_Core_Opt**y=_tmp32;struct Cyc_Absyn_Kind*k2=_tmp33;
# 127
if(({Cyc_Tcutil_kind_leq(k1,k2);})){
({struct Cyc_Core_Opt*_tmp18E=({struct Cyc_Core_Opt*_tmp3E=_cycalloc(sizeof(*_tmp3E));_tmp3E->v=kb1;_tmp3E;});*y=_tmp18E;});
return 1;}else{
if(({Cyc_Tcutil_kind_leq(k2,k1);})){
({struct Cyc_Core_Opt*_tmp18F=({struct Cyc_Core_Opt*_tmp3F=_cycalloc(sizeof(*_tmp3F));_tmp3F->v=kb2;_tmp3F;});*x=_tmp18F;});
return 1;}else{
return 0;}}}default: _LLB: _tmp2E=_tmp2B.f1;_tmp2F=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*)_tmp2B.f2)->f1;_LLC: {void*y=_tmp2E;struct Cyc_Core_Opt**x=_tmp2F;
# 136
({struct Cyc_Core_Opt*_tmp190=({struct Cyc_Core_Opt*_tmp40=_cycalloc(sizeof(*_tmp40));_tmp40->v=y;_tmp40;});*x=_tmp190;});
return 1;}}default: _LL9: _tmp2C=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*)_tmp2B.f1)->f1;_tmp2D=_tmp2B.f2;_LLA: {struct Cyc_Core_Opt**x=_tmp2C;void*y=_tmp2D;
# 134
_tmp2E=y;_tmp2F=x;goto _LLC;}}_LL0:;}struct _tuple14{struct Cyc_Absyn_Tqual f1;void*f2;};
# 143
void Cyc_Unify_occurs(void*evar,struct _RegionHandle*r,struct Cyc_List_List*env,void*t){
t=({Cyc_Tcutil_compress(t);});{
void*_tmp42=t;struct Cyc_List_List*_tmp43;struct Cyc_List_List*_tmp44;struct Cyc_List_List*_tmp45;struct Cyc_List_List*_tmp46;struct Cyc_List_List*_tmp4D;struct Cyc_Absyn_VarargInfo*_tmp4C;struct Cyc_List_List*_tmp4B;void*_tmp4A;struct Cyc_Absyn_Tqual _tmp49;void*_tmp48;struct Cyc_List_List*_tmp47;void*_tmp4F;void*_tmp4E;struct Cyc_Absyn_PtrInfo _tmp50;struct Cyc_Core_Opt**_tmp52;void*_tmp51;struct Cyc_Absyn_Tvar*_tmp53;switch(*((int*)_tmp42)){case 2U: _LL1: _tmp53=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp42)->f1;_LL2: {struct Cyc_Absyn_Tvar*tv=_tmp53;
# 147
if(!({({int(*_tmp192)(int(*compare)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_List_List*l,struct Cyc_Absyn_Tvar*x)=({int(*_tmp54)(int(*compare)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_List_List*l,struct Cyc_Absyn_Tvar*x)=(int(*)(int(*compare)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_List_List*l,struct Cyc_Absyn_Tvar*x))Cyc_List_mem;_tmp54;});struct Cyc_List_List*_tmp191=env;_tmp192(Cyc_Tcutil_fast_tvar_cmp,_tmp191,tv);});}))
({Cyc_Unify_fail_because(({const char*_tmp55="(type variable would escape scope)";_tag_fat(_tmp55,sizeof(char),35U);}));});
# 147
goto _LL0;}case 1U: _LL3: _tmp51=(void*)((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp42)->f2;_tmp52=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp42)->f4;_LL4: {void*rg=_tmp51;struct Cyc_Core_Opt**sopt=_tmp52;
# 151
if(t == evar)
({Cyc_Unify_fail_because(({const char*_tmp56="(occurs check)";_tag_fat(_tmp56,sizeof(char),15U);}));});else{
if(rg != 0)({Cyc_Unify_occurs(evar,r,env,rg);});else{
# 156
int problem=0;
{struct Cyc_List_List*s=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(*sopt))->v;for(0;s != 0 && !problem;s=s->tl){
if(!({({int(*_tmp194)(int(*compare)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_List_List*l,struct Cyc_Absyn_Tvar*x)=({int(*_tmp57)(int(*compare)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_List_List*l,struct Cyc_Absyn_Tvar*x)=(int(*)(int(*compare)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_List_List*l,struct Cyc_Absyn_Tvar*x))Cyc_List_mem;_tmp57;});struct Cyc_List_List*_tmp193=env;_tmp194(Cyc_Tcutil_fast_tvar_cmp,_tmp193,(struct Cyc_Absyn_Tvar*)s->hd);});}))
problem=1;}}
# 157
if(problem){
# 162
struct Cyc_List_List*result=0;
{struct Cyc_List_List*s=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(*sopt))->v;for(0;s != 0;s=s->tl){
if(({({int(*_tmp196)(int(*compare)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_List_List*l,struct Cyc_Absyn_Tvar*x)=({int(*_tmp58)(int(*compare)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_List_List*l,struct Cyc_Absyn_Tvar*x)=(int(*)(int(*compare)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_List_List*l,struct Cyc_Absyn_Tvar*x))Cyc_List_mem;_tmp58;});struct Cyc_List_List*_tmp195=env;_tmp196(Cyc_Tcutil_fast_tvar_cmp,_tmp195,(struct Cyc_Absyn_Tvar*)s->hd);});}))
result=({struct Cyc_List_List*_tmp59=_cycalloc(sizeof(*_tmp59));_tmp59->hd=(struct Cyc_Absyn_Tvar*)s->hd,_tmp59->tl=result;_tmp59;});}}
# 163
({struct Cyc_Core_Opt*_tmp197=({struct Cyc_Core_Opt*_tmp5A=_cycalloc(sizeof(*_tmp5A));_tmp5A->v=result;_tmp5A;});*sopt=_tmp197;});}}}
# 169
goto _LL0;}case 3U: _LL5: _tmp50=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp42)->f1;_LL6: {struct Cyc_Absyn_PtrInfo pinfo=_tmp50;
# 171
({Cyc_Unify_occurs(evar,r,env,pinfo.elt_type);});
({Cyc_Unify_occurs(evar,r,env,(pinfo.ptr_atts).rgn);});
({Cyc_Unify_occurs(evar,r,env,(pinfo.ptr_atts).nullable);});
({Cyc_Unify_occurs(evar,r,env,(pinfo.ptr_atts).bounds);});
({Cyc_Unify_occurs(evar,r,env,(pinfo.ptr_atts).zero_term);});
goto _LL0;}case 4U: _LL7: _tmp4E=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp42)->f1).elt_type;_tmp4F=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp42)->f1).zero_term;_LL8: {void*t2=_tmp4E;void*zt=_tmp4F;
# 179
({Cyc_Unify_occurs(evar,r,env,t2);});
({Cyc_Unify_occurs(evar,r,env,zt);});
goto _LL0;}case 5U: _LL9: _tmp47=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp42)->f1).tvars;_tmp48=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp42)->f1).effect;_tmp49=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp42)->f1).ret_tqual;_tmp4A=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp42)->f1).ret_type;_tmp4B=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp42)->f1).args;_tmp4C=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp42)->f1).cyc_varargs;_tmp4D=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp42)->f1).rgn_po;_LLA: {struct Cyc_List_List*tvs=_tmp47;void*eff=_tmp48;struct Cyc_Absyn_Tqual rt_tq=_tmp49;void*rt=_tmp4A;struct Cyc_List_List*args=_tmp4B;struct Cyc_Absyn_VarargInfo*cyc_varargs=_tmp4C;struct Cyc_List_List*rgn_po=_tmp4D;
# 184
env=({Cyc_List_rappend(r,tvs,env);});
if(eff != 0)({Cyc_Unify_occurs(evar,r,env,eff);});({Cyc_Unify_occurs(evar,r,env,rt);});
# 187
for(0;args != 0;args=args->tl){
({Cyc_Unify_occurs(evar,r,env,(*((struct _tuple9*)args->hd)).f3);});}
if(cyc_varargs != 0)
({Cyc_Unify_occurs(evar,r,env,cyc_varargs->type);});
# 189
for(0;rgn_po != 0;rgn_po=rgn_po->tl){
# 192
struct _tuple12*_stmttmp1=(struct _tuple12*)rgn_po->hd;void*_tmp5C;void*_tmp5B;_LL16: _tmp5B=_stmttmp1->f1;_tmp5C=_stmttmp1->f2;_LL17: {void*r1=_tmp5B;void*r2=_tmp5C;
({Cyc_Unify_occurs(evar,r,env,r1);});
({Cyc_Unify_occurs(evar,r,env,r2);});}}
# 196
goto _LL0;}case 6U: _LLB: _tmp46=((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp42)->f1;_LLC: {struct Cyc_List_List*args=_tmp46;
# 198
for(0;args != 0;args=args->tl){
({Cyc_Unify_occurs(evar,r,env,(*((struct _tuple14*)args->hd)).f2);});}
goto _LL0;}case 7U: _LLD: _tmp45=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp42)->f2;_LLE: {struct Cyc_List_List*fs=_tmp45;
# 203
for(0;fs != 0;fs=fs->tl){
({Cyc_Unify_occurs(evar,r,env,((struct Cyc_Absyn_Aggrfield*)fs->hd)->type);});}
goto _LL0;}case 8U: _LLF: _tmp44=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp42)->f2;_LL10: {struct Cyc_List_List*ts=_tmp44;
_tmp43=ts;goto _LL12;}case 0U: _LL11: _tmp43=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp42)->f2;_LL12: {struct Cyc_List_List*ts=_tmp43;
# 208
for(0;ts != 0;ts=ts->tl){
({Cyc_Unify_occurs(evar,r,env,(void*)ts->hd);});}
goto _LL0;}default: _LL13: _LL14:
# 213
 goto _LL0;}_LL0:;}}
# 143
static void Cyc_Unify_unify_it(void*t1,void*t2);
# 220
int Cyc_Unify_unify(void*t1,void*t2){
struct _handler_cons _tmp5E;_push_handler(& _tmp5E);{int _tmp60=0;if(setjmp(_tmp5E.handler))_tmp60=1;if(!_tmp60){
({Cyc_Unify_unify_it(t1,t2);});{
int _tmp61=1;_npop_handler(0U);return _tmp61;}
# 222
;_pop_handler();}else{void*_tmp5F=(void*)Cyc_Core_get_exn_thrown();void*_tmp62=_tmp5F;void*_tmp63;if(((struct Cyc_Unify_Unify_exn_struct*)_tmp62)->tag == Cyc_Unify_Unify){_LL1: _LL2:
# 224
 return 0;}else{_LL3: _tmp63=_tmp62;_LL4: {void*exn=_tmp63;(int)_rethrow(exn);}}_LL0:;}}}
# 228
static void Cyc_Unify_unify_list(struct Cyc_List_List*t1,struct Cyc_List_List*t2){
for(0;t1 != 0 && t2 != 0;(t1=t1->tl,t2=t2->tl)){
({Cyc_Unify_unify_it((void*)t1->hd,(void*)t2->hd);});}
if(t1 != 0 || t2 != 0)
(int)_throw(& Cyc_Unify_Unify_val);}
# 236
static void Cyc_Unify_unify_tqual(struct Cyc_Absyn_Tqual tq1,void*t1,struct Cyc_Absyn_Tqual tq2,void*t2){
if(tq1.print_const && !tq1.real_const)
({void*_tmp66=0U;({int(*_tmp199)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp68)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp68;});struct _fat_ptr _tmp198=({const char*_tmp67="tq1 real_const not set.";_tag_fat(_tmp67,sizeof(char),24U);});_tmp199(_tmp198,_tag_fat(_tmp66,sizeof(void*),0U));});});
# 237
if(
# 239
tq2.print_const && !tq2.real_const)
({void*_tmp69=0U;({int(*_tmp19B)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp6B)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp6B;});struct _fat_ptr _tmp19A=({const char*_tmp6A="tq2 real_const not set.";_tag_fat(_tmp6A,sizeof(char),24U);});_tmp19B(_tmp19A,_tag_fat(_tmp69,sizeof(void*),0U));});});
# 237
if(
# 242
(tq1.real_const != tq2.real_const || tq1.q_volatile != tq2.q_volatile)|| tq1.q_restrict != tq2.q_restrict){
# 245
Cyc_Unify_ts_failure=({struct _tuple12 _tmp14A;_tmp14A.f1=t1,_tmp14A.f2=t2;_tmp14A;});
Cyc_Unify_tqs_const=({struct _tuple13 _tmp14B;_tmp14B.f1=tq1.real_const,_tmp14B.f2=tq2.real_const;_tmp14B;});
Cyc_Unify_failure_reason=({const char*_tmp6C="(qualifiers don't match)";_tag_fat(_tmp6C,sizeof(char),25U);});
(int)_throw(& Cyc_Unify_Unify_val);}
# 237
Cyc_Unify_tqs_const=({struct _tuple13 _tmp14C;_tmp14C.f1=0,_tmp14C.f2=0;_tmp14C;});}
# 265 "unify.cyc"
static int Cyc_Unify_unify_effect(void*e1,void*e2){
e1=({Cyc_Tcutil_normalize_effect(e1);});
e2=({Cyc_Tcutil_normalize_effect(e2);});{
int b1;int b2;int b3;int b4;
if((b1=({Cyc_Tcutil_subset_effect(0,e1,e2);}))&&(b2=({Cyc_Tcutil_subset_effect(0,e2,e1);})))
return 1;
# 269
if(
# 271
(b3=({Cyc_Tcutil_subset_effect(1,e1,e2);}))&&(b4=({Cyc_Tcutil_subset_effect(1,e2,e1);})))
return 1;
# 269
if(({Cyc_Absyn_get_debug();})){
# 276
({struct Cyc_String_pa_PrintArg_struct _tmp70=({struct Cyc_String_pa_PrintArg_struct _tmp152;_tmp152.tag=0U,({
struct _fat_ptr _tmp19C=(struct _fat_ptr)(b1?({const char*_tmp7C="true";_tag_fat(_tmp7C,sizeof(char),5U);}):({const char*_tmp7D="false";_tag_fat(_tmp7D,sizeof(char),6U);}));_tmp152.f1=_tmp19C;});_tmp152;});struct Cyc_String_pa_PrintArg_struct _tmp71=({struct Cyc_String_pa_PrintArg_struct _tmp151;_tmp151.tag=0U,({struct _fat_ptr _tmp19D=(struct _fat_ptr)(b1?({const char*_tmp7A="true";_tag_fat(_tmp7A,sizeof(char),5U);}):({const char*_tmp7B="false";_tag_fat(_tmp7B,sizeof(char),6U);}));_tmp151.f1=_tmp19D;});_tmp151;});struct Cyc_String_pa_PrintArg_struct _tmp72=({struct Cyc_String_pa_PrintArg_struct _tmp150;_tmp150.tag=0U,({struct _fat_ptr _tmp19E=(struct _fat_ptr)(b1?({const char*_tmp78="true";_tag_fat(_tmp78,sizeof(char),5U);}):({const char*_tmp79="false";_tag_fat(_tmp79,sizeof(char),6U);}));_tmp150.f1=_tmp19E;});_tmp150;});struct Cyc_String_pa_PrintArg_struct _tmp73=({struct Cyc_String_pa_PrintArg_struct _tmp14F;_tmp14F.tag=0U,({struct _fat_ptr _tmp19F=(struct _fat_ptr)(b1?({const char*_tmp76="true";_tag_fat(_tmp76,sizeof(char),5U);}):({const char*_tmp77="false";_tag_fat(_tmp77,sizeof(char),6U);}));_tmp14F.f1=_tmp19F;});_tmp14F;});struct Cyc_String_pa_PrintArg_struct _tmp74=({struct Cyc_String_pa_PrintArg_struct _tmp14E;_tmp14E.tag=0U,({struct _fat_ptr _tmp1A0=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_eff2string(e1);}));_tmp14E.f1=_tmp1A0;});_tmp14E;});struct Cyc_String_pa_PrintArg_struct _tmp75=({struct Cyc_String_pa_PrintArg_struct _tmp14D;_tmp14D.tag=0U,({struct _fat_ptr _tmp1A1=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_eff2string(e2);}));_tmp14D.f1=_tmp1A1;});_tmp14D;});void*_tmp6E[6U];_tmp6E[0]=& _tmp70,_tmp6E[1]=& _tmp71,_tmp6E[2]=& _tmp72,_tmp6E[3]=& _tmp73,_tmp6E[4]=& _tmp74,_tmp6E[5]=& _tmp75;({struct Cyc___cycFILE*_tmp1A3=Cyc_stdout;struct _fat_ptr _tmp1A2=({const char*_tmp6F="\nDebug:: unify_effect:  returning false ... e1 <f= e2 =  %s && e2 >f= e1 = %s |e1 <t= e2 =  %s && e2 >t= e1 = %s | e1 = %s   , e2 = %s\n";_tag_fat(_tmp6F,sizeof(char),136U);});Cyc_fprintf(_tmp1A3,_tmp1A2,_tag_fat(_tmp6E,sizeof(void*),6U));});});
({Cyc_fflush(Cyc_stdout);});}
# 269
return 0;}}
# 284
static int Cyc_Unify_unify_const_exp_opt(struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2){
if(e1 == 0 && e2 == 0)
return 1;
# 285
if(
# 287
e1 == 0 || e2 == 0)
return 0;
# 285
return({Cyc_Evexp_same_const_exp(e1,e2);});}struct _tuple15{struct Cyc_Absyn_Tvar*f1;void*f2;};struct _tuple16{struct Cyc_Absyn_VarargInfo*f1;struct Cyc_Absyn_VarargInfo*f2;};
# 292
static void Cyc_Unify_unify_it(void*t1,void*t2){
Cyc_Unify_ts_failure=({struct _tuple12 _tmp153;_tmp153.f1=t1,_tmp153.f2=t2;_tmp153;});
Cyc_Unify_failure_reason=_tag_fat(0,0,0);
t1=({Cyc_Tcutil_compress(t1);});
t2=({Cyc_Tcutil_compress(t2);});
if(t1 == t2)return;{void*_tmp80=t1;struct Cyc_Core_Opt*_tmp83;void**_tmp82;struct Cyc_Core_Opt**_tmp81;if(((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp80)->tag == 1U){_LL1: _tmp81=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp80)->f1;_tmp82=(void**)&((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp80)->f2;_tmp83=((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp80)->f4;_LL2: {struct Cyc_Core_Opt**kind11=_tmp81;void**ref1_ref=_tmp82;struct Cyc_Core_Opt*s1opt=_tmp83;
# 304
struct Cyc_Core_Opt*kind1=*kind11;
({Cyc_Unify_occurs(t1,Cyc_Core_heap_region,(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(s1opt))->v,t2);});{
struct Cyc_Absyn_Kind*kind2=({Cyc_Tcutil_type_kind(t2);});
# 320 "unify.cyc"
if(({Cyc_Tcutil_kind_leq(kind2,(struct Cyc_Absyn_Kind*)((struct Cyc_Core_Opt*)_check_null(kind1))->v);})){
*ref1_ref=t2;
return;}{
# 320
void*_tmp84=t2;struct Cyc_Absyn_PtrInfo _tmp85;struct Cyc_Core_Opt*_tmp87;void**_tmp86;switch(*((int*)_tmp84)){case 1U: _LL6: _tmp86=(void**)&((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp84)->f2;_tmp87=((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp84)->f4;_LL7: {void**ref2_ref=_tmp86;struct Cyc_Core_Opt*s2opt=_tmp87;
# 351 "unify.cyc"
struct Cyc_List_List*s1=(struct Cyc_List_List*)s1opt->v;
{struct Cyc_List_List*s2=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(s2opt))->v;for(0;s2 != 0;s2=s2->tl){
if(!({({int(*_tmp1A5)(int(*compare)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_List_List*l,struct Cyc_Absyn_Tvar*x)=({int(*_tmp88)(int(*compare)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_List_List*l,struct Cyc_Absyn_Tvar*x)=(int(*)(int(*compare)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_List_List*l,struct Cyc_Absyn_Tvar*x))Cyc_List_mem;_tmp88;});struct Cyc_List_List*_tmp1A4=s1;_tmp1A5(Cyc_Tcutil_fast_tvar_cmp,_tmp1A4,(struct Cyc_Absyn_Tvar*)s2->hd);});}))
({Cyc_Unify_fail_because(({const char*_tmp89="(type variable would escape scope)";_tag_fat(_tmp89,sizeof(char),35U);}));});}}
# 352
if(!({Cyc_Tcutil_kind_leq((struct Cyc_Absyn_Kind*)kind1->v,kind2);}))
# 356
({void(*_tmp8A)(struct _fat_ptr reason)=Cyc_Unify_fail_because;struct _fat_ptr _tmp8B=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp8E=({struct Cyc_String_pa_PrintArg_struct _tmp155;_tmp155.tag=0U,({
struct _fat_ptr _tmp1A6=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_kind2string((struct Cyc_Absyn_Kind*)kind1->v);}));_tmp155.f1=_tmp1A6;});_tmp155;});struct Cyc_String_pa_PrintArg_struct _tmp8F=({struct Cyc_String_pa_PrintArg_struct _tmp154;_tmp154.tag=0U,({
struct _fat_ptr _tmp1A7=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_kind2string(kind2);}));_tmp154.f1=_tmp1A7;});_tmp154;});void*_tmp8C[2U];_tmp8C[0]=& _tmp8E,_tmp8C[1]=& _tmp8F;({struct _fat_ptr _tmp1A8=({const char*_tmp8D="(kinds are incompatible k1=%s k2=%s).";_tag_fat(_tmp8D,sizeof(char),38U);});Cyc_aprintf(_tmp1A8,_tag_fat(_tmp8C,sizeof(void*),2U));});});_tmp8A(_tmp8B);});
# 352
*ref2_ref=t1;
# 361
return;}case 3U: _LL8: _tmp85=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp84)->f1;if((int)((struct Cyc_Absyn_Kind*)kind1->v)->kind == (int)Cyc_Absyn_BoxKind){_LL9: {struct Cyc_Absyn_PtrInfo pinfo=_tmp85;
# 366
void*c=({Cyc_Tcutil_compress((pinfo.ptr_atts).bounds);});
void*_tmp90=c;if(((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp90)->tag == 1U){_LLD: _LLE:
# 369
({int(*_tmp91)(void*t1,void*t2)=Cyc_Unify_unify;void*_tmp92=c;void*_tmp93=({Cyc_Absyn_bounds_one();});_tmp91(_tmp92,_tmp93);});
*ref1_ref=t2;
return;}else{_LLF: _LL10:
# 373
({void(*_tmp94)(struct _fat_ptr reason)=Cyc_Unify_fail_because;struct _fat_ptr _tmp95=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp98=({struct Cyc_String_pa_PrintArg_struct _tmp157;_tmp157.tag=0U,({
struct _fat_ptr _tmp1A9=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_kind2string((struct Cyc_Absyn_Kind*)kind1->v);}));_tmp157.f1=_tmp1A9;});_tmp157;});struct Cyc_String_pa_PrintArg_struct _tmp99=({struct Cyc_String_pa_PrintArg_struct _tmp156;_tmp156.tag=0U,({
struct _fat_ptr _tmp1AA=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_kind2string(kind2);}));_tmp156.f1=_tmp1AA;});_tmp156;});void*_tmp96[2U];_tmp96[0]=& _tmp98,_tmp96[1]=& _tmp99;({struct _fat_ptr _tmp1AB=({const char*_tmp97="(kinds are incompatible k1=%s k2=%s )..";_tag_fat(_tmp97,sizeof(char),40U);});Cyc_aprintf(_tmp1AB,_tag_fat(_tmp96,sizeof(void*),2U));});});_tmp94(_tmp95);});}_LLC:;}}else{goto _LLA;}default: _LLA: _LLB:
# 380
({void(*_tmp9A)(struct _fat_ptr reason)=Cyc_Unify_fail_because;struct _fat_ptr _tmp9B=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp9E=({struct Cyc_String_pa_PrintArg_struct _tmp15A;_tmp15A.tag=0U,({
struct _fat_ptr _tmp1AC=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_kind2string((struct Cyc_Absyn_Kind*)kind1->v);}));_tmp15A.f1=_tmp1AC;});_tmp15A;});struct Cyc_String_pa_PrintArg_struct _tmp9F=({struct Cyc_String_pa_PrintArg_struct _tmp159;_tmp159.tag=0U,({
struct _fat_ptr _tmp1AD=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_kind2string(kind2);}));_tmp159.f1=_tmp1AD;});_tmp159;});struct Cyc_String_pa_PrintArg_struct _tmpA0=({struct Cyc_String_pa_PrintArg_struct _tmp158;_tmp158.tag=0U,({
struct _fat_ptr _tmp1AE=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t2);}));_tmp158.f1=_tmp1AE;});_tmp158;});void*_tmp9C[3U];_tmp9C[0]=& _tmp9E,_tmp9C[1]=& _tmp9F,_tmp9C[2]=& _tmpA0;({struct _fat_ptr _tmp1AF=({const char*_tmp9D="(kinds are incompatible k1=%s k2=%s t2=%s)...";_tag_fat(_tmp9D,sizeof(char),46U);});Cyc_aprintf(_tmp1AF,_tag_fat(_tmp9C,sizeof(void*),3U));});});_tmp9A(_tmp9B);});}_LL5:;}}}}else{_LL3: _LL4:
# 388
 goto _LL0;}_LL0:;}{
# 392
struct _tuple12 _stmttmp2=({struct _tuple12 _tmp16B;_tmp16B.f1=t2,_tmp16B.f2=t1;_tmp16B;});struct _tuple12 _tmpA1=_stmttmp2;struct Cyc_Absyn_Typedefdecl*_tmpA5;struct Cyc_List_List*_tmpA4;struct Cyc_Absyn_Typedefdecl*_tmpA3;struct Cyc_List_List*_tmpA2;struct Cyc_List_List*_tmpA9;enum Cyc_Absyn_AggrKind _tmpA8;struct Cyc_List_List*_tmpA7;enum Cyc_Absyn_AggrKind _tmpA6;struct Cyc_List_List*_tmpAB;struct Cyc_List_List*_tmpAA;int _tmpCD;struct Cyc_List_List*_tmpCC;struct Cyc_List_List*_tmpCB;struct Cyc_List_List*_tmpCA;struct Cyc_List_List*_tmpC9;struct Cyc_Absyn_Exp*_tmpC8;struct Cyc_List_List*_tmpC7;struct Cyc_Absyn_Exp*_tmpC6;struct Cyc_List_List*_tmpC5;struct Cyc_List_List*_tmpC4;struct Cyc_Absyn_VarargInfo*_tmpC3;int _tmpC2;struct Cyc_List_List*_tmpC1;void*_tmpC0;struct Cyc_Absyn_Tqual _tmpBF;void*_tmpBE;struct Cyc_List_List*_tmpBD;int _tmpBC;struct Cyc_List_List*_tmpBB;struct Cyc_List_List*_tmpBA;struct Cyc_List_List*_tmpB9;struct Cyc_List_List*_tmpB8;struct Cyc_Absyn_Exp*_tmpB7;struct Cyc_List_List*_tmpB6;struct Cyc_Absyn_Exp*_tmpB5;struct Cyc_List_List*_tmpB4;struct Cyc_List_List*_tmpB3;struct Cyc_Absyn_VarargInfo*_tmpB2;int _tmpB1;struct Cyc_List_List*_tmpB0;void*_tmpAF;struct Cyc_Absyn_Tqual _tmpAE;void*_tmpAD;struct Cyc_List_List*_tmpAC;void*_tmpD5;struct Cyc_Absyn_Exp*_tmpD4;struct Cyc_Absyn_Tqual _tmpD3;void*_tmpD2;void*_tmpD1;struct Cyc_Absyn_Exp*_tmpD0;struct Cyc_Absyn_Tqual _tmpCF;void*_tmpCE;struct Cyc_Absyn_Exp*_tmpD7;struct Cyc_Absyn_Exp*_tmpD6;void*_tmpE3;void*_tmpE2;void*_tmpE1;void*_tmpE0;struct Cyc_Absyn_Tqual _tmpDF;void*_tmpDE;void*_tmpDD;void*_tmpDC;void*_tmpDB;void*_tmpDA;struct Cyc_Absyn_Tqual _tmpD9;void*_tmpD8;struct Cyc_Absyn_Tvar*_tmpE5;struct Cyc_Absyn_Tvar*_tmpE4;struct Cyc_List_List*_tmpE9;void*_tmpE8;struct Cyc_List_List*_tmpE7;void*_tmpE6;switch(*((int*)_tmpA1.f1)){case 1U: _LL12: _LL13:
# 395
({Cyc_Unify_unify_it(t2,t1);});return;case 0U: if(((struct Cyc_Absyn_JoinCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpA1.f1)->f1)->tag == 11U){_LL14: _LL15:
# 397
 goto _LL17;}else{if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpA1.f2)->tag == 0U){if(((struct Cyc_Absyn_JoinCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpA1.f2)->f1)->tag == 11U)goto _LL16;else{if(((struct Cyc_Absyn_AccessCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpA1.f1)->f1)->tag == 9U)goto _LL18;else{if(((struct Cyc_Absyn_AccessCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpA1.f2)->f1)->tag == 9U)goto _LL1A;else{if(((struct Cyc_Absyn_RgnsCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpA1.f1)->f1)->tag == 12U)goto _LL1C;else{if(((struct Cyc_Absyn_RgnsCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpA1.f2)->f1)->tag == 12U)goto _LL1E;else{_LL20: _tmpE6=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpA1.f1)->f1;_tmpE7=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpA1.f1)->f2;_tmpE8=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpA1.f2)->f1;_tmpE9=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpA1.f2)->f2;_LL21: {void*c1=_tmpE6;struct Cyc_List_List*ts1=_tmpE7;void*c2=_tmpE8;struct Cyc_List_List*ts2=_tmpE9;
# 411
if(({Cyc_Tcutil_tycon_cmp(c1,c2);})!= 0)
({Cyc_Unify_fail_because(({const char*_tmpEB="(different type constructors)";_tag_fat(_tmpEB,sizeof(char),30U);}));});
# 411
({Cyc_Unify_unify_list(ts1,ts2);});
# 414
return;}}}}}}}else{switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpA1.f1)->f1)){case 9U: _LL18: _LL19:
# 399
 goto _LL1B;case 12U: _LL1C: _LL1D:
# 401
 goto _LL1F;default: goto _LL32;}}}default: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpA1.f2)->tag == 0U)switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpA1.f2)->f1)){case 11U: _LL16: _LL17:
# 398
 goto _LL19;case 9U: _LL1A: _LL1B:
# 400
 goto _LL1D;case 12U: _LL1E: _LL1F:
# 403
 if(!({Cyc_Unify_unify_effect(t1,t2);})){
# 405
if(!Cyc_Unify_checking_pointer_effect)({Cyc_Unify_fail_because(({const char*_tmpEA="(effects don't unify)";_tag_fat(_tmpEA,sizeof(char),22U);}));});else{
(int)_throw(& Cyc_Unify_Unify_val);}}
# 403
return;default: switch(*((int*)_tmpA1.f1)){case 2U: goto _LL32;case 3U: goto _LL32;case 9U: goto _LL32;case 4U: goto _LL32;case 5U: goto _LL32;case 6U: goto _LL32;case 7U: goto _LL32;case 8U: goto _LL32;default: goto _LL32;}}else{switch(*((int*)_tmpA1.f1)){case 2U: if(((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmpA1.f2)->tag == 2U){_LL22: _tmpE4=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmpA1.f1)->f1;_tmpE5=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmpA1.f2)->f1;_LL23: {struct Cyc_Absyn_Tvar*tv2=_tmpE4;struct Cyc_Absyn_Tvar*tv1=_tmpE5;
# 418
if(tv2->identity != tv1->identity)
({Cyc_Unify_fail_because(({const char*_tmpEC="(variable types are not the same)";_tag_fat(_tmpEC,sizeof(char),34U);}));});
# 418
return;}}else{goto _LL32;}case 3U: if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmpA1.f2)->tag == 3U){_LL24: _tmpD8=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmpA1.f1)->f1).elt_type;_tmpD9=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmpA1.f1)->f1).elt_tq;_tmpDA=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmpA1.f1)->f1).ptr_atts).rgn;_tmpDB=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmpA1.f1)->f1).ptr_atts).nullable;_tmpDC=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmpA1.f1)->f1).ptr_atts).bounds;_tmpDD=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmpA1.f1)->f1).ptr_atts).zero_term;_tmpDE=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmpA1.f2)->f1).elt_type;_tmpDF=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmpA1.f2)->f1).elt_tq;_tmpE0=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmpA1.f2)->f1).ptr_atts).rgn;_tmpE1=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmpA1.f2)->f1).ptr_atts).nullable;_tmpE2=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmpA1.f2)->f1).ptr_atts).bounds;_tmpE3=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmpA1.f2)->f1).ptr_atts).zero_term;_LL25: {void*t2a=_tmpD8;struct Cyc_Absyn_Tqual tqual2a=_tmpD9;void*rgn2=_tmpDA;void*null2a=_tmpDB;void*b2=_tmpDC;void*zt2=_tmpDD;void*t1a=_tmpDE;struct Cyc_Absyn_Tqual tqual1a=_tmpDF;void*rgn1=_tmpE0;void*null1a=_tmpE1;void*b1=_tmpE2;void*zt1=_tmpE3;
# 424
({Cyc_Unify_unify_it(t1a,t2a);});
Cyc_Unify_checking_pointer_effect=1;
{struct _handler_cons _tmpED;_push_handler(& _tmpED);{int _tmpEF=0;if(setjmp(_tmpED.handler))_tmpEF=1;if(!_tmpEF){
# 434
({Cyc_Unify_unify_it(rgn2,rgn1);});;_pop_handler();}else{void*_tmpEE=(void*)Cyc_Core_get_exn_thrown();void*_tmpF0=_tmpEE;void*_tmpF1;_LL35: _tmpF1=_tmpF0;_LL36: {void*e=_tmpF1;
# 439
Cyc_Unify_checking_pointer_effect=0;
(int)_throw(e);}_LL34:;}}}
# 442
Cyc_Unify_checking_pointer_effect=0;{
struct _fat_ptr orig_failure=Cyc_Unify_failure_reason;
if(!({Cyc_Unify_unify(zt1,zt2);})){
Cyc_Unify_ts_failure=({struct _tuple12 _tmp15B;_tmp15B.f1=t1,_tmp15B.f2=t2;_tmp15B;});
({Cyc_Unify_fail_because(({const char*_tmpF2="(not both zero terminated)";_tag_fat(_tmpF2,sizeof(char),27U);}));});}
# 444
({Cyc_Unify_unify_tqual(tqual1a,t1a,tqual2a,t2a);});
# 449
if(!({Cyc_Unify_unify(b1,b2);})){
Cyc_Unify_ts_failure=({struct _tuple12 _tmp15C;_tmp15C.f1=t1,_tmp15C.f2=t2;_tmp15C;});
({Cyc_Unify_fail_because(({const char*_tmpF3="(different pointer bounds)";_tag_fat(_tmpF3,sizeof(char),27U);}));});}{
# 449
void*_stmttmp3=({Cyc_Tcutil_compress(b1);});void*_tmpF4=_stmttmp3;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpF4)->tag == 0U){if(((struct Cyc_Absyn_FatCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpF4)->f1)->tag == 16U){_LL3A: _LL3B:
# 456
 Cyc_Unify_failure_reason=orig_failure;
return;}else{goto _LL3C;}}else{_LL3C: _LL3D:
# 459
 Cyc_Unify_failure_reason=({const char*_tmpF5="(incompatible pointer types)";_tag_fat(_tmpF5,sizeof(char),29U);});
({Cyc_Unify_unify_it(null1a,null2a);});
return;}_LL39:;}}}}else{goto _LL32;}case 9U: if(((struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_tmpA1.f2)->tag == 9U){_LL26: _tmpD6=((struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_tmpA1.f1)->f1;_tmpD7=((struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_tmpA1.f2)->f1;_LL27: {struct Cyc_Absyn_Exp*e1=_tmpD6;struct Cyc_Absyn_Exp*e2=_tmpD7;
# 465
if(!({Cyc_Evexp_same_const_exp(e1,e2);}))
({Cyc_Unify_fail_because(({const char*_tmpF6="(cannot prove expressions are the same)";_tag_fat(_tmpF6,sizeof(char),40U);}));});
# 465
return;}}else{goto _LL32;}case 4U: if(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmpA1.f2)->tag == 4U){_LL28: _tmpCE=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmpA1.f1)->f1).elt_type;_tmpCF=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmpA1.f1)->f1).tq;_tmpD0=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmpA1.f1)->f1).num_elts;_tmpD1=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmpA1.f1)->f1).zero_term;_tmpD2=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmpA1.f2)->f1).elt_type;_tmpD3=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmpA1.f2)->f1).tq;_tmpD4=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmpA1.f2)->f1).num_elts;_tmpD5=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmpA1.f2)->f1).zero_term;_LL29: {void*t2a=_tmpCE;struct Cyc_Absyn_Tqual tq2a=_tmpCF;struct Cyc_Absyn_Exp*e1=_tmpD0;void*zt1=_tmpD1;void*t1a=_tmpD2;struct Cyc_Absyn_Tqual tq1a=_tmpD3;struct Cyc_Absyn_Exp*e2=_tmpD4;void*zt2=_tmpD5;
# 471
({Cyc_Unify_unify_it(t1a,t2a);});
({Cyc_Unify_unify_tqual(tq1a,t1a,tq2a,t2a);});
Cyc_Unify_failure_reason=({const char*_tmpF7="(not both zero terminated)";_tag_fat(_tmpF7,sizeof(char),27U);});
({Cyc_Unify_unify_it(zt1,zt2);});
if(!({Cyc_Unify_unify_const_exp_opt(e1,e2);}))
({Cyc_Unify_fail_because(({const char*_tmpF8="(different array sizes)";_tag_fat(_tmpF8,sizeof(char),24U);}));});
# 475
return;}}else{goto _LL32;}case 5U: if(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmpA1.f2)->tag == 5U){_LL2A: _tmpAC=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmpA1.f1)->f1).tvars;_tmpAD=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmpA1.f1)->f1).effect;_tmpAE=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmpA1.f1)->f1).ret_tqual;_tmpAF=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmpA1.f1)->f1).ret_type;_tmpB0=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmpA1.f1)->f1).args;_tmpB1=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmpA1.f1)->f1).c_varargs;_tmpB2=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmpA1.f1)->f1).cyc_varargs;_tmpB3=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmpA1.f1)->f1).rgn_po;_tmpB4=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmpA1.f1)->f1).attributes;_tmpB5=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmpA1.f1)->f1).requires_clause;_tmpB6=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmpA1.f1)->f1).requires_relns;_tmpB7=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmpA1.f1)->f1).ensures_clause;_tmpB8=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmpA1.f1)->f1).ensures_relns;_tmpB9=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmpA1.f1)->f1).ieffect;_tmpBA=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmpA1.f1)->f1).oeffect;_tmpBB=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmpA1.f1)->f1).throws;_tmpBC=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmpA1.f1)->f1).reentrant;_tmpBD=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmpA1.f2)->f1).tvars;_tmpBE=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmpA1.f2)->f1).effect;_tmpBF=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmpA1.f2)->f1).ret_tqual;_tmpC0=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmpA1.f2)->f1).ret_type;_tmpC1=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmpA1.f2)->f1).args;_tmpC2=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmpA1.f2)->f1).c_varargs;_tmpC3=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmpA1.f2)->f1).cyc_varargs;_tmpC4=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmpA1.f2)->f1).rgn_po;_tmpC5=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmpA1.f2)->f1).attributes;_tmpC6=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmpA1.f2)->f1).requires_clause;_tmpC7=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmpA1.f2)->f1).requires_relns;_tmpC8=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmpA1.f2)->f1).ensures_clause;_tmpC9=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmpA1.f2)->f1).ensures_relns;_tmpCA=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmpA1.f2)->f1).ieffect;_tmpCB=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmpA1.f2)->f1).oeffect;_tmpCC=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmpA1.f2)->f1).throws;_tmpCD=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmpA1.f2)->f1).reentrant;_LL2B: {struct Cyc_List_List*tvs2=_tmpAC;void*eff2=_tmpAD;struct Cyc_Absyn_Tqual rt_tq2=_tmpAE;void*rt2=_tmpAF;struct Cyc_List_List*args2=_tmpB0;int c_vararg2=_tmpB1;struct Cyc_Absyn_VarargInfo*cyc_vararg2=_tmpB2;struct Cyc_List_List*rpo2=_tmpB3;struct Cyc_List_List*atts2=_tmpB4;struct Cyc_Absyn_Exp*req1=_tmpB5;struct Cyc_List_List*req_relns1=_tmpB6;struct Cyc_Absyn_Exp*ens1=_tmpB7;struct Cyc_List_List*ens_relns1=_tmpB8;struct Cyc_List_List*ieff1=_tmpB9;struct Cyc_List_List*oeff1=_tmpBA;struct Cyc_List_List*throws1=_tmpBB;int reentrant1=_tmpBC;struct Cyc_List_List*tvs1=_tmpBD;void*eff1=_tmpBE;struct Cyc_Absyn_Tqual rt_tq1=_tmpBF;void*rt1=_tmpC0;struct Cyc_List_List*args1=_tmpC1;int c_vararg1=_tmpC2;struct Cyc_Absyn_VarargInfo*cyc_vararg1=_tmpC3;struct Cyc_List_List*rpo1=_tmpC4;struct Cyc_List_List*atts1=_tmpC5;struct Cyc_Absyn_Exp*req2=_tmpC6;struct Cyc_List_List*req_relns2=_tmpC7;struct Cyc_Absyn_Exp*ens2=_tmpC8;struct Cyc_List_List*ens_relns2=_tmpC9;struct Cyc_List_List*ieff2=_tmpCA;struct Cyc_List_List*oeff2=_tmpCB;struct Cyc_List_List*throws2=_tmpCC;int reentrant2=_tmpCD;
# 485
if(reentrant1 != reentrant2)
# 487
({Cyc_Unify_fail_because(({const char*_tmpF9="(one of the functions is re-entrant and the other is not)";_tag_fat(_tmpF9,sizeof(char),58U);}));});
# 485
{
# 489
struct _RegionHandle _tmpFA=_new_region("rgn");struct _RegionHandle*rgn=& _tmpFA;_push_region(rgn);
# 491
{struct Cyc_List_List*inst=0;
while(tvs1 != 0){
if(tvs2 == 0)
({Cyc_Unify_fail_because(({const char*_tmpFB="(second function type has too few type variables)";_tag_fat(_tmpFB,sizeof(char),50U);}));});{
# 493
void*kb1=((struct Cyc_Absyn_Tvar*)tvs1->hd)->kind;
# 496
void*kb2=((struct Cyc_Absyn_Tvar*)tvs2->hd)->kind;
if(!({Cyc_Unify_unify_kindbound(kb1,kb2);}))
({void(*_tmpFC)(struct _fat_ptr reason)=Cyc_Unify_fail_because;struct _fat_ptr _tmpFD=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp100=({struct Cyc_String_pa_PrintArg_struct _tmp15F;_tmp15F.tag=0U,({
struct _fat_ptr _tmp1B0=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_tvar2string((struct Cyc_Absyn_Tvar*)tvs1->hd);}));_tmp15F.f1=_tmp1B0;});_tmp15F;});struct Cyc_String_pa_PrintArg_struct _tmp101=({struct Cyc_String_pa_PrintArg_struct _tmp15E;_tmp15E.tag=0U,({
struct _fat_ptr _tmp1B1=(struct _fat_ptr)((struct _fat_ptr)({struct _fat_ptr(*_tmp105)(struct Cyc_Absyn_Kind*)=Cyc_Absynpp_kind2string;struct Cyc_Absyn_Kind*_tmp106=({Cyc_Tcutil_tvar_kind((struct Cyc_Absyn_Tvar*)tvs1->hd,& Cyc_Tcutil_bk);});_tmp105(_tmp106);}));_tmp15E.f1=_tmp1B1;});_tmp15E;});struct Cyc_String_pa_PrintArg_struct _tmp102=({struct Cyc_String_pa_PrintArg_struct _tmp15D;_tmp15D.tag=0U,({
struct _fat_ptr _tmp1B2=(struct _fat_ptr)((struct _fat_ptr)({struct _fat_ptr(*_tmp103)(struct Cyc_Absyn_Kind*)=Cyc_Absynpp_kind2string;struct Cyc_Absyn_Kind*_tmp104=({Cyc_Tcutil_tvar_kind((struct Cyc_Absyn_Tvar*)tvs2->hd,& Cyc_Tcutil_bk);});_tmp103(_tmp104);}));_tmp15D.f1=_tmp1B2;});_tmp15D;});void*_tmpFE[3U];_tmpFE[0]=& _tmp100,_tmpFE[1]=& _tmp101,_tmpFE[2]=& _tmp102;({struct _fat_ptr _tmp1B3=({const char*_tmpFF="(type var %s has different kinds %s and %s)";_tag_fat(_tmpFF,sizeof(char),44U);});Cyc_aprintf(_tmp1B3,_tag_fat(_tmpFE,sizeof(void*),3U));});});_tmpFC(_tmpFD);});
# 497
inst=({struct Cyc_List_List*_tmp108=_region_malloc(rgn,sizeof(*_tmp108));
# 502
({struct _tuple15*_tmp1B5=({struct _tuple15*_tmp107=_region_malloc(rgn,sizeof(*_tmp107));_tmp107->f1=(struct Cyc_Absyn_Tvar*)tvs2->hd,({void*_tmp1B4=({Cyc_Absyn_var_type((struct Cyc_Absyn_Tvar*)tvs1->hd);});_tmp107->f2=_tmp1B4;});_tmp107;});_tmp108->hd=_tmp1B5;}),_tmp108->tl=inst;_tmp108;});
tvs1=tvs1->tl;
tvs2=tvs2->tl;}}
# 506
if(tvs2 != 0)
({Cyc_Unify_fail_because(({const char*_tmp109="(second function type has too many type variables)";_tag_fat(_tmp109,sizeof(char),51U);}));});
# 506
if(inst != 0){
# 509
({void(*_tmp10A)(void*t1,void*t2)=Cyc_Unify_unify_it;void*_tmp10B=(void*)({struct Cyc_Absyn_FnType_Absyn_Type_struct*_tmp10C=_cycalloc(sizeof(*_tmp10C));_tmp10C->tag=5U,(_tmp10C->f1).tvars=0,(_tmp10C->f1).effect=eff1,(_tmp10C->f1).ret_tqual=rt_tq1,(_tmp10C->f1).ret_type=rt1,(_tmp10C->f1).args=args1,(_tmp10C->f1).c_varargs=c_vararg1,(_tmp10C->f1).cyc_varargs=cyc_vararg1,(_tmp10C->f1).rgn_po=rpo1,(_tmp10C->f1).attributes=atts1,(_tmp10C->f1).requires_clause=req1,(_tmp10C->f1).requires_relns=req_relns1,(_tmp10C->f1).ensures_clause=ens1,(_tmp10C->f1).ensures_relns=ens_relns1,(_tmp10C->f1).ieffect=ieff1,(_tmp10C->f1).oeffect=oeff1,(_tmp10C->f1).throws=throws1,(_tmp10C->f1).reentrant=reentrant1;_tmp10C;});void*_tmp10D=({({struct _RegionHandle*_tmp1B7=rgn;struct Cyc_List_List*_tmp1B6=inst;Cyc_Tcutil_rsubstitute(_tmp1B7,_tmp1B6,(void*)({struct Cyc_Absyn_FnType_Absyn_Type_struct*_tmp10E=_cycalloc(sizeof(*_tmp10E));_tmp10E->tag=5U,(_tmp10E->f1).tvars=0,(_tmp10E->f1).effect=eff2,(_tmp10E->f1).ret_tqual=rt_tq2,(_tmp10E->f1).ret_type=rt2,(_tmp10E->f1).args=args2,(_tmp10E->f1).c_varargs=c_vararg2,(_tmp10E->f1).cyc_varargs=cyc_vararg2,(_tmp10E->f1).rgn_po=rpo2,(_tmp10E->f1).attributes=atts2,(_tmp10E->f1).requires_clause=req2,(_tmp10E->f1).requires_relns=req_relns2,(_tmp10E->f1).ensures_clause=ens2,(_tmp10E->f1).ensures_relns=ens_relns2,(_tmp10E->f1).ieffect=ieff1,(_tmp10E->f1).oeffect=oeff2,(_tmp10E->f1).throws=throws2,(_tmp10E->f1).reentrant=reentrant2;_tmp10E;}));});});_tmp10A(_tmp10B,_tmp10D);});
# 518
_npop_handler(0U);return;}}
# 491
;_pop_region();}
# 521
({Cyc_Unify_unify_it(rt1,rt2);});
({Cyc_Unify_unify_tqual(rt_tq1,rt1,rt_tq2,rt2);});
for(0;args1 != 0 && args2 != 0;(args1=args1->tl,args2=args2->tl)){
struct _tuple9 _stmttmp4=*((struct _tuple9*)args1->hd);void*_tmp110;struct Cyc_Absyn_Tqual _tmp10F;_LL3F: _tmp10F=_stmttmp4.f2;_tmp110=_stmttmp4.f3;_LL40: {struct Cyc_Absyn_Tqual tqa=_tmp10F;void*ta=_tmp110;
struct _tuple9 _stmttmp5=*((struct _tuple9*)args2->hd);void*_tmp112;struct Cyc_Absyn_Tqual _tmp111;_LL42: _tmp111=_stmttmp5.f2;_tmp112=_stmttmp5.f3;_LL43: {struct Cyc_Absyn_Tqual tqb=_tmp111;void*tb=_tmp112;
({Cyc_Unify_unify_it(ta,tb);});
({Cyc_Unify_unify_tqual(tqa,ta,tqb,tb);});}}}
# 529
Cyc_Unify_ts_failure=({struct _tuple12 _tmp160;_tmp160.f1=t1,_tmp160.f2=t2;_tmp160;});
if(args1 != 0 || args2 != 0)
({Cyc_Unify_fail_because(({const char*_tmp113="(function types have different number of arguments)";_tag_fat(_tmp113,sizeof(char),52U);}));});
# 530
if(c_vararg1 != c_vararg2)
# 533
({Cyc_Unify_fail_because(({const char*_tmp114="(only one function type takes C varargs)";_tag_fat(_tmp114,sizeof(char),41U);}));});
# 530
{struct _tuple16 _stmttmp6=({struct _tuple16 _tmp161;_tmp161.f1=cyc_vararg1,_tmp161.f2=cyc_vararg2;_tmp161;});struct _tuple16 _tmp115=_stmttmp6;int _tmp11D;void*_tmp11C;struct Cyc_Absyn_Tqual _tmp11B;struct _fat_ptr*_tmp11A;int _tmp119;void*_tmp118;struct Cyc_Absyn_Tqual _tmp117;struct _fat_ptr*_tmp116;if(_tmp115.f1 == 0){if(_tmp115.f2 == 0){_LL45: _LL46:
# 536
 goto _LL44;}else{_LL47: _LL48:
 goto _LL4A;}}else{if(_tmp115.f2 == 0){_LL49: _LL4A:
({Cyc_Unify_fail_because(({const char*_tmp11E="(only one function type takes varargs)";_tag_fat(_tmp11E,sizeof(char),39U);}));});}else{_LL4B: _tmp116=(_tmp115.f1)->name;_tmp117=(_tmp115.f1)->tq;_tmp118=(_tmp115.f1)->type;_tmp119=(_tmp115.f1)->inject;_tmp11A=(_tmp115.f2)->name;_tmp11B=(_tmp115.f2)->tq;_tmp11C=(_tmp115.f2)->type;_tmp11D=(_tmp115.f2)->inject;_LL4C: {struct _fat_ptr*n1=_tmp116;struct Cyc_Absyn_Tqual tq1=_tmp117;void*tp1=_tmp118;int i1=_tmp119;struct _fat_ptr*n2=_tmp11A;struct Cyc_Absyn_Tqual tq2=_tmp11B;void*tp2=_tmp11C;int i2=_tmp11D;
# 540
({Cyc_Unify_unify_it(tp1,tp2);});
({Cyc_Unify_unify_tqual(tq1,tp1,tq2,tp2);});
if(i1 != i2)
({Cyc_Unify_fail_because(({const char*_tmp11F="(only one function type injects varargs)";_tag_fat(_tmp11F,sizeof(char),41U);}));});}}}_LL44:;}
# 546
if((ieff1 != 0 && ieff2 == 0 ||
 ieff2 != 0 && ieff1 == 0)|| !({Cyc_Absyn_unify_rgneffects(ieff1,ieff2,Cyc_Unify_unify_it);}))
# 550
({Cyc_Unify_fail_because(({const char*_tmp120="(Function types have different @ieffect annotations)";_tag_fat(_tmp120,sizeof(char),53U);}));});
# 546
if(
# 552
(oeff1 != 0 && oeff2 == 0 ||
 oeff2 != 0 && oeff1 == 0)|| !({Cyc_Absyn_unify_rgneffects(oeff1,oeff2,Cyc_Unify_unify_it);}))
# 556
({Cyc_Unify_fail_because(({const char*_tmp121="(Function types have different @oeffect annotations)";_tag_fat(_tmp121,sizeof(char),53U);}));});
# 546
if(!({Cyc_Absyn_equals_throws(throws1,throws2);}))
# 563
({void(*_tmp122)(struct _fat_ptr reason)=Cyc_Unify_fail_because;struct _fat_ptr _tmp123=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp126=({struct Cyc_String_pa_PrintArg_struct _tmp165;_tmp165.tag=0U,({
struct _fat_ptr _tmp1B8=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t1);}));_tmp165.f1=_tmp1B8;});_tmp165;});struct Cyc_String_pa_PrintArg_struct _tmp127=({struct Cyc_String_pa_PrintArg_struct _tmp164;_tmp164.tag=0U,({
struct _fat_ptr _tmp1B9=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_throws2string(throws1);}));_tmp164.f1=_tmp1B9;});_tmp164;});struct Cyc_String_pa_PrintArg_struct _tmp128=({struct Cyc_String_pa_PrintArg_struct _tmp163;_tmp163.tag=0U,({
struct _fat_ptr _tmp1BA=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t2);}));_tmp163.f1=_tmp1BA;});_tmp163;});struct Cyc_String_pa_PrintArg_struct _tmp129=({struct Cyc_String_pa_PrintArg_struct _tmp162;_tmp162.tag=0U,({
struct _fat_ptr _tmp1BB=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_throws2string(throws2);}));_tmp162.f1=_tmp1BB;});_tmp162;});void*_tmp124[4U];_tmp124[0]=& _tmp126,_tmp124[1]=& _tmp127,_tmp124[2]=& _tmp128,_tmp124[3]=& _tmp129;({struct _fat_ptr _tmp1BC=({const char*_tmp125="(Function types have different @throws annotations): %s (%s) AND %s (%s)";_tag_fat(_tmp125,sizeof(char),73U);});Cyc_aprintf(_tmp1BC,_tag_fat(_tmp124,sizeof(void*),4U));});});_tmp122(_tmp123);});
# 546
if(reentrant1 != reentrant2)
# 570
({Cyc_Unify_fail_because(({const char*_tmp12A="(one of the functions is re-entrant and the other is not)";_tag_fat(_tmp12A,sizeof(char),58U);}));});{
# 546
int bad_effect;
# 577
if(eff1 == 0 && eff2 == 0)
bad_effect=0;else{
if(eff1 == 0 || eff2 == 0)
bad_effect=1;else{
# 582
bad_effect=!({Cyc_Unify_unify_effect(eff1,eff2);});}}
Cyc_Unify_ts_failure=({struct _tuple12 _tmp166;_tmp166.f1=t1,_tmp166.f2=t2;_tmp166;});
if(bad_effect)
({void(*_tmp12B)(struct _fat_ptr reason)=Cyc_Unify_fail_because;struct _fat_ptr _tmp12C=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp12F=({struct Cyc_String_pa_PrintArg_struct _tmp168;_tmp168.tag=0U,({
struct _fat_ptr _tmp1BD=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string((void*)_check_null(eff1));}));_tmp168.f1=_tmp1BD;});_tmp168;});struct Cyc_String_pa_PrintArg_struct _tmp130=({struct Cyc_String_pa_PrintArg_struct _tmp167;_tmp167.tag=0U,({struct _fat_ptr _tmp1BE=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string((void*)_check_null(eff2));}));_tmp167.f1=_tmp1BE;});_tmp167;});void*_tmp12D[2U];_tmp12D[0]=& _tmp12F,_tmp12D[1]=& _tmp130;({struct _fat_ptr _tmp1BF=({const char*_tmp12E="(function effects do not match: effect 1 = %s  , effect 2 =  %s)";_tag_fat(_tmp12E,sizeof(char),65U);});Cyc_aprintf(_tmp1BF,_tag_fat(_tmp12D,sizeof(void*),2U));});});_tmp12B(_tmp12C);});
# 584
if(!({Cyc_Tcutil_equiv_fn_atts(atts2,atts1);}))
# 588
({Cyc_Unify_fail_because(({const char*_tmp131="(function types have different attributes)";_tag_fat(_tmp131,sizeof(char),43U);}));});
# 584
if(!({Cyc_Tcutil_same_rgn_po(rpo2,rpo1);}))
# 590
({Cyc_Unify_fail_because(({const char*_tmp132="(function types have different region lifetime orderings)";_tag_fat(_tmp132,sizeof(char),58U);}));});
# 584
if(!({Cyc_Unify_check_logical_equivalence(req_relns1,req_relns2);}))
# 592
({Cyc_Unify_fail_because(({const char*_tmp133="(@requires clauses not equivalent)";_tag_fat(_tmp133,sizeof(char),35U);}));});
# 584
if(!({Cyc_Unify_check_logical_equivalence(ens_relns1,ens_relns2);}))
# 594
({Cyc_Unify_fail_because(({const char*_tmp134="(@ensures clauses not equivalent)";_tag_fat(_tmp134,sizeof(char),34U);}));});
# 584
return;}}}else{goto _LL32;}case 6U: if(((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmpA1.f2)->tag == 6U){_LL2C: _tmpAA=((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmpA1.f1)->f1;_tmpAB=((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmpA1.f2)->f1;_LL2D: {struct Cyc_List_List*ts2=_tmpAA;struct Cyc_List_List*ts1=_tmpAB;
# 598
for(0;ts1 != 0 && ts2 != 0;(ts1=ts1->tl,ts2=ts2->tl)){
struct _tuple14 _stmttmp7=*((struct _tuple14*)ts1->hd);void*_tmp136;struct Cyc_Absyn_Tqual _tmp135;_LL4E: _tmp135=_stmttmp7.f1;_tmp136=_stmttmp7.f2;_LL4F: {struct Cyc_Absyn_Tqual tqa=_tmp135;void*ta=_tmp136;
struct _tuple14 _stmttmp8=*((struct _tuple14*)ts2->hd);void*_tmp138;struct Cyc_Absyn_Tqual _tmp137;_LL51: _tmp137=_stmttmp8.f1;_tmp138=_stmttmp8.f2;_LL52: {struct Cyc_Absyn_Tqual tqb=_tmp137;void*tb=_tmp138;
({Cyc_Unify_unify_it(ta,tb);});
({Cyc_Unify_unify_tqual(tqa,ta,tqb,tb);});}}}
# 604
if(ts1 == 0 && ts2 == 0)
return;
# 604
Cyc_Unify_ts_failure=({struct _tuple12 _tmp169;_tmp169.f1=t1,_tmp169.f2=t2;_tmp169;});
# 607
({Cyc_Unify_fail_because(({const char*_tmp139="(tuple types have different numbers of components)";_tag_fat(_tmp139,sizeof(char),51U);}));});}}else{goto _LL32;}case 7U: if(((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmpA1.f2)->tag == 7U){_LL2E: _tmpA6=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmpA1.f1)->f1;_tmpA7=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmpA1.f1)->f2;_tmpA8=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmpA1.f2)->f1;_tmpA9=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmpA1.f2)->f2;_LL2F: {enum Cyc_Absyn_AggrKind k2=_tmpA6;struct Cyc_List_List*fs2=_tmpA7;enum Cyc_Absyn_AggrKind k1=_tmpA8;struct Cyc_List_List*fs1=_tmpA9;
# 610
if((int)k1 != (int)k2)
({Cyc_Unify_fail_because(({const char*_tmp13A="(struct and union type)";_tag_fat(_tmp13A,sizeof(char),24U);}));});
# 610
for(0;
# 612
fs1 != 0 && fs2 != 0;(fs1=fs1->tl,fs2=fs2->tl)){
struct Cyc_Absyn_Aggrfield*f1=(struct Cyc_Absyn_Aggrfield*)fs1->hd;
struct Cyc_Absyn_Aggrfield*f2=(struct Cyc_Absyn_Aggrfield*)fs2->hd;
if(({Cyc_strptrcmp(f1->name,f2->name);})!= 0)
({Cyc_Unify_fail_because(({const char*_tmp13B="(different member names)";_tag_fat(_tmp13B,sizeof(char),25U);}));});
# 615
({Cyc_Unify_unify_it(f1->type,f2->type);});
# 618
({Cyc_Unify_unify_tqual(f1->tq,f1->type,f2->tq,f2->type);});
Cyc_Unify_ts_failure=({struct _tuple12 _tmp16A;_tmp16A.f1=t1,_tmp16A.f2=t2;_tmp16A;});
if(!({Cyc_Tcutil_same_atts(f1->attributes,f2->attributes);}))
({Cyc_Unify_fail_because(({const char*_tmp13C="(different attributes on member)";_tag_fat(_tmp13C,sizeof(char),33U);}));});
# 620
if(!({Cyc_Unify_unify_const_exp_opt(f1->width,f2->width);}))
# 623
({Cyc_Unify_fail_because(({const char*_tmp13D="(different bitfield widths on member)";_tag_fat(_tmp13D,sizeof(char),38U);}));});
# 620
if(!({Cyc_Unify_unify_const_exp_opt(f1->requires_clause,f2->requires_clause);}))
# 625
({Cyc_Unify_fail_because(({const char*_tmp13E="(different @requires clauses on member)";_tag_fat(_tmp13E,sizeof(char),40U);}));});}
# 627
if(fs1 != 0 || fs2 != 0)
({Cyc_Unify_fail_because(({const char*_tmp13F="(different number of members)";_tag_fat(_tmp13F,sizeof(char),30U);}));});
# 627
return;}}else{goto _LL32;}case 8U: if(((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmpA1.f2)->tag == 8U){_LL30: _tmpA2=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmpA1.f1)->f2;_tmpA3=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmpA1.f1)->f3;_tmpA4=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmpA1.f2)->f2;_tmpA5=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmpA1.f2)->f3;_LL31: {struct Cyc_List_List*ts1=_tmpA2;struct Cyc_Absyn_Typedefdecl*td1=_tmpA3;struct Cyc_List_List*ts2=_tmpA4;struct Cyc_Absyn_Typedefdecl*td2=_tmpA5;
# 632
if(td1 != td2)
({Cyc_Unify_fail_because(({const char*_tmp140="(different abstract typedefs)";_tag_fat(_tmp140,sizeof(char),30U);}));});
# 632
Cyc_Unify_failure_reason=({const char*_tmp141="(type parameters to typedef differ)";_tag_fat(_tmp141,sizeof(char),36U);});
# 635
({Cyc_Unify_unify_list(ts1,ts2);});
return;}}else{goto _LL32;}default: _LL32: _LL33:
(int)_throw(& Cyc_Unify_Unify_val);}}}_LL11:;}}

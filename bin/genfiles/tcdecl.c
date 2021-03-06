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
int Cyc_fprintf(struct Cyc___cycFILE*,struct _fat_ptr,struct _fat_ptr);struct Cyc_ShortPtr_sa_ScanfArg_struct{int tag;short*f1;};struct Cyc_UShortPtr_sa_ScanfArg_struct{int tag;unsigned short*f1;};struct Cyc_IntPtr_sa_ScanfArg_struct{int tag;int*f1;};struct Cyc_UIntPtr_sa_ScanfArg_struct{int tag;unsigned*f1;};struct Cyc_StringPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_DoublePtr_sa_ScanfArg_struct{int tag;double*f1;};struct Cyc_FloatPtr_sa_ScanfArg_struct{int tag;float*f1;};struct Cyc_CharPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};
# 232 "cycboot.h"
struct _fat_ptr Cyc_vrprintf(struct _RegionHandle*,struct _fat_ptr,struct _fat_ptr);extern char Cyc_FileCloseError[15U];struct Cyc_FileCloseError_exn_struct{char*tag;};extern char Cyc_FileOpenError[14U];struct Cyc_FileOpenError_exn_struct{char*tag;struct _fat_ptr f1;};struct Cyc_Core_Opt{void*v;};extern char Cyc_Core_Invalid_argument[17U];struct Cyc_Core_Invalid_argument_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Failure[8U];struct Cyc_Core_Failure_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Impossible[11U];struct Cyc_Core_Impossible_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Not_found[10U];struct Cyc_Core_Not_found_exn_struct{char*tag;};extern char Cyc_Core_Unreachable[12U];struct Cyc_Core_Unreachable_exn_struct{char*tag;struct _fat_ptr f1;};
# 165 "core.h"
extern struct _RegionHandle*Cyc_Core_heap_region;
# 168
extern struct _RegionHandle*Cyc_Core_unique_region;struct Cyc_Core_DynamicRegion;struct Cyc_Core_NewDynamicRegion{struct Cyc_Core_DynamicRegion*key;};struct Cyc_Core_ThinRes{void*arr;unsigned nelts;};struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};
# 61 "list.h"
int Cyc_List_length(struct Cyc_List_List*x);
# 83
struct Cyc_List_List*Cyc_List_map_c(void*(*f)(void*,void*),void*env,struct Cyc_List_List*x);extern char Cyc_List_List_mismatch[14U];struct Cyc_List_List_mismatch_exn_struct{char*tag;};
# 184
struct Cyc_List_List*Cyc_List_append(struct Cyc_List_List*x,struct Cyc_List_List*y);extern char Cyc_List_Nth[4U];struct Cyc_List_Nth_exn_struct{char*tag;};struct Cyc_Iter_Iter{void*env;int(*next)(void*env,void*dest);};struct Cyc_Dict_T;struct Cyc_Dict_Dict{int(*rel)(void*,void*);struct _RegionHandle*r;const struct Cyc_Dict_T*t;};extern char Cyc_Dict_Present[8U];struct Cyc_Dict_Present_exn_struct{char*tag;};extern char Cyc_Dict_Absent[7U];struct Cyc_Dict_Absent_exn_struct{char*tag;};
# 38 "string.h"
unsigned long Cyc_strlen(struct _fat_ptr s);
# 50 "string.h"
int Cyc_strptrcmp(struct _fat_ptr*s1,struct _fat_ptr*s2);
# 62
struct _fat_ptr Cyc_strconcat(struct _fat_ptr,struct _fat_ptr);struct Cyc_Position_Error;
# 43 "position.h"
struct Cyc_Position_Error*Cyc_Position_mk_err(unsigned,struct _fat_ptr);
# 49
void Cyc_Position_post_error(struct Cyc_Position_Error*);struct Cyc_Relations_Reln;struct _union_Nmspace_Rel_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Abs_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_C_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Loc_n{int tag;int val;};union Cyc_Absyn_Nmspace{struct _union_Nmspace_Rel_n Rel_n;struct _union_Nmspace_Abs_n Abs_n;struct _union_Nmspace_C_n C_n;struct _union_Nmspace_Loc_n Loc_n;};struct _tuple0{union Cyc_Absyn_Nmspace f1;struct _fat_ptr*f2;};
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
int Cyc_Absyn_qvar_cmp(struct _tuple0*,struct _tuple0*);
# 963
struct Cyc_Absyn_Tqual Cyc_Absyn_empty_tqual(unsigned);
# 965
int Cyc_Absyn_equal_tqual(struct Cyc_Absyn_Tqual,struct Cyc_Absyn_Tqual);
# 969
struct Cyc_Absyn_Kind*Cyc_Absyn_force_kb(void*);
# 1213
struct _fat_ptr Cyc_Absyn_attribute2string(void*);
# 27 "unify.h"
void Cyc_Unify_explain_failure();
int Cyc_Unify_unify_kindbound(void*,void*);
int Cyc_Unify_unify(void*,void*);struct _union_RelnOp_RConst{int tag;unsigned val;};struct _union_RelnOp_RVar{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RNumelts{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RType{int tag;void*val;};struct _union_RelnOp_RParam{int tag;unsigned val;};struct _union_RelnOp_RParamNumelts{int tag;unsigned val;};struct _union_RelnOp_RReturn{int tag;unsigned val;};union Cyc_Relations_RelnOp{struct _union_RelnOp_RConst RConst;struct _union_RelnOp_RVar RVar;struct _union_RelnOp_RNumelts RNumelts;struct _union_RelnOp_RType RType;struct _union_RelnOp_RParam RParam;struct _union_RelnOp_RParamNumelts RParamNumelts;struct _union_RelnOp_RReturn RReturn;};
# 50 "relations-ap.h"
enum Cyc_Relations_Relation{Cyc_Relations_Req =0U,Cyc_Relations_Rneq =1U,Cyc_Relations_Rlte =2U,Cyc_Relations_Rlt =3U};struct Cyc_Relations_Reln{union Cyc_Relations_RelnOp rop1;enum Cyc_Relations_Relation relation;union Cyc_Relations_RelnOp rop2;};struct Cyc_RgnOrder_RgnPO;extern char Cyc_Tcenv_Env_error[10U];struct Cyc_Tcenv_Env_error_exn_struct{char*tag;};struct Cyc_Tcenv_Genv{struct Cyc_Dict_Dict aggrdecls;struct Cyc_Dict_Dict datatypedecls;struct Cyc_Dict_Dict enumdecls;struct Cyc_Dict_Dict typedefs;struct Cyc_Dict_Dict ordinaries;};struct Cyc_Tcenv_Fenv;struct Cyc_Tcenv_Tenv{struct Cyc_List_List*ns;struct Cyc_Tcenv_Genv*ae;struct Cyc_Tcenv_Fenv*le;int allow_valueof: 1;int in_extern_c_include: 1;int in_tempest: 1;int tempest_generalize: 1;};
# 90 "tcenv.h"
enum Cyc_Tcenv_NewStatus{Cyc_Tcenv_NoneNew =0U,Cyc_Tcenv_InNew =1U,Cyc_Tcenv_InNewAggr =2U};
# 189 "tcutil.h"
void*Cyc_Tcutil_substitute(struct Cyc_List_List*,void*);
# 271
int Cyc_Tcutil_same_atts(struct Cyc_List_List*,struct Cyc_List_List*);struct Cyc_PP_Ppstate;struct Cyc_PP_Out;struct Cyc_PP_Doc;struct Cyc_Absynpp_Params{int expand_typedefs;int qvar_to_Cids;int add_cyc_prefix;int to_VC;int decls_first;int rewrite_temp_tvars;int print_all_tvars;int print_all_kinds;int print_all_effects;int print_using_stmts;int print_externC_stmts;int print_full_evars;int print_zeroterm;int generate_line_directives;int use_curr_namespace;struct Cyc_List_List*curr_namespace;};
# 62 "absynpp.h"
struct _fat_ptr Cyc_Absynpp_typ2string(void*);
# 64
struct _fat_ptr Cyc_Absynpp_kind2string(struct Cyc_Absyn_Kind*);
# 67
struct _fat_ptr Cyc_Absynpp_exp2string(struct Cyc_Absyn_Exp*);
# 69
struct _fat_ptr Cyc_Absynpp_qvar2string(struct _tuple0*);
# 73
struct _fat_ptr Cyc_Absynpp_scope2string(enum Cyc_Absyn_Scope sc);extern char Cyc_Tcdecl_Incompatible[13U];struct Cyc_Tcdecl_Incompatible_exn_struct{char*tag;};struct Cyc_Tcdecl_Xdatatypefielddecl{struct Cyc_Absyn_Datatypedecl*base;struct Cyc_Absyn_Datatypefield*field;};
# 38 "tcdecl.h"
void Cyc_Tcdecl_merr(unsigned loc,struct _fat_ptr*msg1,struct _fat_ptr fmt,struct _fat_ptr ap);
# 61 "tcdecl.h"
struct Cyc_Absyn_Vardecl*Cyc_Tcdecl_merge_vardecl(struct Cyc_Absyn_Vardecl*,struct Cyc_Absyn_Vardecl*,unsigned,struct _fat_ptr*msg);
# 74
struct Cyc_List_List*Cyc_Tcdecl_sort_xdatatype_fields(struct Cyc_List_List*f,int*res,struct _fat_ptr*v,unsigned,struct _fat_ptr*msg);char Cyc_Tcdecl_Incompatible[13U]="Incompatible";
# 36 "tcdecl.cyc"
struct Cyc_Tcdecl_Incompatible_exn_struct Cyc_Tcdecl_Incompatible_val={Cyc_Tcdecl_Incompatible};
# 38
void Cyc_Tcdecl_merr(unsigned loc,struct _fat_ptr*msg1,struct _fat_ptr fmt,struct _fat_ptr ap){
# 42
if(msg1 == 0)
(int)_throw(& Cyc_Tcdecl_Incompatible_val);{
# 42
struct _fat_ptr fmt2;
# 45
if(({Cyc_strlen((struct _fat_ptr)*msg1);})== (unsigned long)0)
fmt2=(struct _fat_ptr)({({struct _fat_ptr _tmp1D0=({const char*_tmp0="%s";_tag_fat(_tmp0,sizeof(char),3U);});Cyc_strconcat(_tmp1D0,(struct _fat_ptr)fmt);});});else{
# 48
fmt2=(struct _fat_ptr)({({struct _fat_ptr _tmp1D1=({const char*_tmp1="%s ";_tag_fat(_tmp1,sizeof(char),4U);});Cyc_strconcat(_tmp1D1,(struct _fat_ptr)fmt);});});}{
struct _fat_ptr ap2=({unsigned _tmp9=_get_fat_size(ap,sizeof(void*))+ (unsigned)1;void**_tmp8=_cycalloc(_check_times(_tmp9,sizeof(void*)));({{unsigned _tmp167=_get_fat_size(ap,sizeof(void*))+ (unsigned)1;unsigned i;for(i=0;i < _tmp167;++ i){
i == (unsigned)0?({void*_tmp1D2=(void*)({struct Cyc_String_pa_PrintArg_struct*_tmp7=_cycalloc(sizeof(*_tmp7));_tmp7->tag=0U,_tmp7->f1=(struct _fat_ptr)((struct _fat_ptr)*msg1);_tmp7;});_tmp8[i]=_tmp1D2;}):(_tmp8[i]=*((void**)_check_fat_subscript(ap,sizeof(void*),(int)(i - (unsigned)1))));}}0;});_tag_fat(_tmp8,sizeof(void*),_tmp9);});
({void(*_tmp2)(struct Cyc_Position_Error*)=Cyc_Position_post_error;struct Cyc_Position_Error*_tmp3=({struct Cyc_Position_Error*(*_tmp4)(unsigned,struct _fat_ptr)=Cyc_Position_mk_err;unsigned _tmp5=loc;struct _fat_ptr _tmp6=(struct _fat_ptr)({Cyc_vrprintf(Cyc_Core_heap_region,fmt2,ap2);});_tmp4(_tmp5,_tmp6);});_tmp2(_tmp3);});}}}
# 56
static void Cyc_Tcdecl_merge_scope_err(enum Cyc_Absyn_Scope s0,enum Cyc_Absyn_Scope s1,struct _fat_ptr t,struct _fat_ptr v,unsigned loc,struct _fat_ptr*msg){
# 58
struct _fat_ptr s0_str=(int)s0 == (int)2U?({const char*_tmp12="public ";_tag_fat(_tmp12,sizeof(char),8U);}):({Cyc_Absynpp_scope2string(s0);});
struct _fat_ptr s1_str=(int)s1 == (int)2U?({const char*_tmp11="public ";_tag_fat(_tmp11,sizeof(char),8U);}):({Cyc_Absynpp_scope2string(s1);});
({struct Cyc_String_pa_PrintArg_struct _tmpD=({struct Cyc_String_pa_PrintArg_struct _tmp16B;_tmp16B.tag=0U,_tmp16B.f1=(struct _fat_ptr)((struct _fat_ptr)t);_tmp16B;});struct Cyc_String_pa_PrintArg_struct _tmpE=({struct Cyc_String_pa_PrintArg_struct _tmp16A;_tmp16A.tag=0U,_tmp16A.f1=(struct _fat_ptr)((struct _fat_ptr)v);_tmp16A;});struct Cyc_String_pa_PrintArg_struct _tmpF=({struct Cyc_String_pa_PrintArg_struct _tmp169;_tmp169.tag=0U,_tmp169.f1=(struct _fat_ptr)((struct _fat_ptr)s1_str);_tmp169;});struct Cyc_String_pa_PrintArg_struct _tmp10=({struct Cyc_String_pa_PrintArg_struct _tmp168;_tmp168.tag=0U,_tmp168.f1=(struct _fat_ptr)((struct _fat_ptr)s0_str);_tmp168;});void*_tmpB[4U];_tmpB[0]=& _tmpD,_tmpB[1]=& _tmpE,_tmpB[2]=& _tmpF,_tmpB[3]=& _tmp10;({unsigned _tmp1D5=loc;struct _fat_ptr*_tmp1D4=msg;struct _fat_ptr _tmp1D3=({const char*_tmpC="%s %s is %sbut expected scope is %s";_tag_fat(_tmpC,sizeof(char),36U);});Cyc_Tcdecl_merr(_tmp1D5,_tmp1D4,_tmp1D3,_tag_fat(_tmpB,sizeof(void*),4U));});});}struct _tuple12{enum Cyc_Absyn_Scope f1;int f2;};struct _tuple13{enum Cyc_Absyn_Scope f1;enum Cyc_Absyn_Scope f2;};
# 64
struct _tuple12 Cyc_Tcdecl_merge_scope(enum Cyc_Absyn_Scope s0,enum Cyc_Absyn_Scope s1,struct _fat_ptr t,struct _fat_ptr v,unsigned loc,struct _fat_ptr*msg,int externCmerge){
# 66
{struct _tuple13 _stmttmp0=({struct _tuple13 _tmp16E;_tmp16E.f1=s0,_tmp16E.f2=s1;_tmp16E;});struct _tuple13 _tmp14=_stmttmp0;switch(_tmp14.f1){case Cyc_Absyn_ExternC: switch(_tmp14.f2){case Cyc_Absyn_ExternC: _LL1: _LL2:
# 68
 goto _LL0;case Cyc_Absyn_Public: _LL3: _LL4:
# 70
 goto _LL6;case Cyc_Absyn_Extern: _LL5: _LL6:
 goto _LL8;default: _LLB: _LLC:
# 74
 goto _LLE;}case Cyc_Absyn_Public: switch(_tmp14.f2){case Cyc_Absyn_ExternC: _LL7: _LL8:
# 72
 goto _LLA;case Cyc_Absyn_Extern: goto _LLF;case Cyc_Absyn_Public: _LL15: _LL16:
# 81
 goto _LL18;default: goto _LL1B;}case Cyc_Absyn_Extern: switch(_tmp14.f2){case Cyc_Absyn_ExternC: _LL9: _LLA:
# 73
 if(externCmerge)goto _LL0;else{goto _LLC;}case Cyc_Absyn_Extern: goto _LLF;default: _LL11: _LL12:
# 78
 goto _LL0;}default: switch(_tmp14.f2){case Cyc_Absyn_ExternC: _LLD: _LLE:
# 75
({Cyc_Tcdecl_merge_scope_err(s0,s1,t,v,loc,msg);});return({struct _tuple12 _tmp16C;_tmp16C.f1=s1,_tmp16C.f2=0;_tmp16C;});case Cyc_Absyn_Extern: _LLF: _LL10:
# 77
 s1=s0;goto _LL0;default: switch(_tmp14.f1){case Cyc_Absyn_Static: if(_tmp14.f2 == Cyc_Absyn_Static){_LL13: _LL14:
# 80
 goto _LL16;}else{goto _LL1B;}case Cyc_Absyn_Abstract: if(_tmp14.f2 == Cyc_Absyn_Abstract){_LL17: _LL18:
# 82
 goto _LL1A;}else{goto _LL1B;}case Cyc_Absyn_Register: if(_tmp14.f2 == Cyc_Absyn_Register){_LL19: _LL1A:
 goto _LL0;}else{goto _LL1B;}default: _LL1B: _LL1C:
({Cyc_Tcdecl_merge_scope_err(s0,s1,t,v,loc,msg);});return({struct _tuple12 _tmp16D;_tmp16D.f1=s1,_tmp16D.f2=0;_tmp16D;});}}}_LL0:;}
# 86
return({struct _tuple12 _tmp16F;_tmp16F.f1=s1,_tmp16F.f2=1;_tmp16F;});}
# 89
static int Cyc_Tcdecl_check_type(void*t0,void*t1){
# 91
return({Cyc_Unify_unify(t0,t1);});}
# 95
static unsigned Cyc_Tcdecl_get_uint_const_value(struct Cyc_Absyn_Exp*e){
void*_stmttmp1=e->r;void*_tmp17=_stmttmp1;int _tmp18;if(((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp17)->tag == 0U){if(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp17)->f1).Int_c).tag == 5){_LL1: _tmp18=(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp17)->f1).Int_c).val).f2;_LL2: {int x=_tmp18;
return(unsigned)x;}}else{goto _LL3;}}else{_LL3: _LL4:
(int)_throw(({struct Cyc_Core_Invalid_argument_exn_struct*_tmp1A=_cycalloc(sizeof(*_tmp1A));_tmp1A->tag=Cyc_Core_Invalid_argument,({struct _fat_ptr _tmp1D6=({const char*_tmp19="Tcdecl::get_uint_const_value";_tag_fat(_tmp19,sizeof(char),29U);});_tmp1A->f1=_tmp1D6;});_tmp1A;}));}_LL0:;}struct _tuple14{struct Cyc_Absyn_Tvar*f1;void*f2;};
# 102
static struct Cyc_List_List*Cyc_Tcdecl_build_tvs_map(struct Cyc_List_List*tvs0,struct Cyc_List_List*tvs1){
# 104
struct Cyc_List_List*inst=0;
for(0;tvs0 != 0;(tvs0=tvs0->tl,tvs1=tvs1->tl)){
inst=({struct Cyc_List_List*_tmp1E=_cycalloc(sizeof(*_tmp1E));({struct _tuple14*_tmp1D8=({struct _tuple14*_tmp1D=_cycalloc(sizeof(*_tmp1D));_tmp1D->f1=(struct Cyc_Absyn_Tvar*)((struct Cyc_List_List*)_check_null(tvs1))->hd,({void*_tmp1D7=(void*)({struct Cyc_Absyn_VarType_Absyn_Type_struct*_tmp1C=_cycalloc(sizeof(*_tmp1C));_tmp1C->tag=2U,_tmp1C->f1=(struct Cyc_Absyn_Tvar*)tvs0->hd;_tmp1C;});_tmp1D->f2=_tmp1D7;});_tmp1D;});_tmp1E->hd=_tmp1D8;}),_tmp1E->tl=inst;_tmp1E;});}
return inst;}
# 110
static int Cyc_Tcdecl_check_tvs(struct Cyc_List_List*tvs0,struct Cyc_List_List*tvs1,struct _fat_ptr t,struct _fat_ptr v,unsigned loc,struct _fat_ptr*msg){
# 113
if(({int _tmp1D9=({Cyc_List_length(tvs0);});_tmp1D9 != ({Cyc_List_length(tvs1);});})){
({struct Cyc_String_pa_PrintArg_struct _tmp22=({struct Cyc_String_pa_PrintArg_struct _tmp171;_tmp171.tag=0U,_tmp171.f1=(struct _fat_ptr)((struct _fat_ptr)t);_tmp171;});struct Cyc_String_pa_PrintArg_struct _tmp23=({struct Cyc_String_pa_PrintArg_struct _tmp170;_tmp170.tag=0U,_tmp170.f1=(struct _fat_ptr)((struct _fat_ptr)v);_tmp170;});void*_tmp20[2U];_tmp20[0]=& _tmp22,_tmp20[1]=& _tmp23;({unsigned _tmp1DC=loc;struct _fat_ptr*_tmp1DB=msg;struct _fat_ptr _tmp1DA=({const char*_tmp21="%s %s has a different number of type parameters";_tag_fat(_tmp21,sizeof(char),48U);});Cyc_Tcdecl_merr(_tmp1DC,_tmp1DB,_tmp1DA,_tag_fat(_tmp20,sizeof(void*),2U));});});
return 0;}{
# 113
struct Cyc_List_List*x0=tvs0;
# 118
struct Cyc_List_List*x1=tvs1;
for(0;x0 != 0;(x0=x0->tl,x1=x1->tl)){
({Cyc_Unify_unify_kindbound(((struct Cyc_Absyn_Tvar*)x0->hd)->kind,((struct Cyc_Absyn_Tvar*)((struct Cyc_List_List*)_check_null(x1))->hd)->kind);});{
struct Cyc_Absyn_Kind*c0=({Cyc_Absyn_force_kb(((struct Cyc_Absyn_Tvar*)x0->hd)->kind);});
struct Cyc_Absyn_Kind*c1=({Cyc_Absyn_force_kb(((struct Cyc_Absyn_Tvar*)x1->hd)->kind);});
if(c0 == c1)continue;({struct Cyc_String_pa_PrintArg_struct _tmp26=({struct Cyc_String_pa_PrintArg_struct _tmp176;_tmp176.tag=0U,_tmp176.f1=(struct _fat_ptr)((struct _fat_ptr)t);_tmp176;});struct Cyc_String_pa_PrintArg_struct _tmp27=({struct Cyc_String_pa_PrintArg_struct _tmp175;_tmp175.tag=0U,_tmp175.f1=(struct _fat_ptr)((struct _fat_ptr)v);_tmp175;});struct Cyc_String_pa_PrintArg_struct _tmp28=({struct Cyc_String_pa_PrintArg_struct _tmp174;_tmp174.tag=0U,({
# 125
struct _fat_ptr _tmp1DD=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_kind2string(c0);}));_tmp174.f1=_tmp1DD;});_tmp174;});struct Cyc_String_pa_PrintArg_struct _tmp29=({struct Cyc_String_pa_PrintArg_struct _tmp173;_tmp173.tag=0U,_tmp173.f1=(struct _fat_ptr)((struct _fat_ptr)*((struct Cyc_Absyn_Tvar*)x0->hd)->name);_tmp173;});struct Cyc_String_pa_PrintArg_struct _tmp2A=({struct Cyc_String_pa_PrintArg_struct _tmp172;_tmp172.tag=0U,({struct _fat_ptr _tmp1DE=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_kind2string(c1);}));_tmp172.f1=_tmp1DE;});_tmp172;});void*_tmp24[5U];_tmp24[0]=& _tmp26,_tmp24[1]=& _tmp27,_tmp24[2]=& _tmp28,_tmp24[3]=& _tmp29,_tmp24[4]=& _tmp2A;({unsigned _tmp1E1=loc;struct _fat_ptr*_tmp1E0=msg;struct _fat_ptr _tmp1DF=({const char*_tmp25="%s %s has a different kind (%s) for type parameter %s (%s)";_tag_fat(_tmp25,sizeof(char),59U);});Cyc_Tcdecl_merr(_tmp1E1,_tmp1E0,_tmp1DF,_tag_fat(_tmp24,sizeof(void*),5U));});});
return 0;}}
# 128
return 1;}}
# 131
static int Cyc_Tcdecl_check_atts(struct Cyc_List_List*atts0,struct Cyc_List_List*atts1,struct _fat_ptr t,struct _fat_ptr v,unsigned loc,struct _fat_ptr*msg){
# 133
if(!({Cyc_Tcutil_same_atts(atts0,atts1);})){
({struct Cyc_String_pa_PrintArg_struct _tmp2E=({struct Cyc_String_pa_PrintArg_struct _tmp178;_tmp178.tag=0U,_tmp178.f1=(struct _fat_ptr)((struct _fat_ptr)t);_tmp178;});struct Cyc_String_pa_PrintArg_struct _tmp2F=({struct Cyc_String_pa_PrintArg_struct _tmp177;_tmp177.tag=0U,_tmp177.f1=(struct _fat_ptr)((struct _fat_ptr)v);_tmp177;});void*_tmp2C[2U];_tmp2C[0]=& _tmp2E,_tmp2C[1]=& _tmp2F;({unsigned _tmp1E4=loc;struct _fat_ptr*_tmp1E3=msg;struct _fat_ptr _tmp1E2=({const char*_tmp2D="%s %s has different attributes";_tag_fat(_tmp2D,sizeof(char),31U);});Cyc_Tcdecl_merr(_tmp1E4,_tmp1E3,_tmp1E2,_tag_fat(_tmp2C,sizeof(void*),2U));});});
return 0;}
# 133
return 1;}struct _tuple15{struct Cyc_Absyn_AggrdeclImpl*f1;struct Cyc_Absyn_AggrdeclImpl*f2;};struct _tuple16{void*f1;void*f2;};
# 141
struct Cyc_Absyn_Aggrdecl*Cyc_Tcdecl_merge_aggrdecl(struct Cyc_Absyn_Aggrdecl*d0,struct Cyc_Absyn_Aggrdecl*d1,unsigned loc,struct _fat_ptr*msg){
# 143
struct _fat_ptr v=({Cyc_Absynpp_qvar2string(d0->name);});
int res=1;
# 149
if(!((int)d0->kind == (int)d1->kind))return 0;if(!({({struct Cyc_List_List*_tmp1E9=d0->tvs;struct Cyc_List_List*_tmp1E8=d1->tvs;struct _fat_ptr _tmp1E7=({const char*_tmp31="type";_tag_fat(_tmp31,sizeof(char),5U);});struct _fat_ptr _tmp1E6=v;unsigned _tmp1E5=loc;Cyc_Tcdecl_check_tvs(_tmp1E9,_tmp1E8,_tmp1E7,_tmp1E6,_tmp1E5,msg);});}))
# 152
return 0;{
# 149
struct _tuple12 _stmttmp2=({({enum Cyc_Absyn_Scope _tmp1EE=d0->sc;enum Cyc_Absyn_Scope _tmp1ED=d1->sc;struct _fat_ptr _tmp1EC=({const char*_tmp83="type";_tag_fat(_tmp83,sizeof(char),5U);});struct _fat_ptr _tmp1EB=v;unsigned _tmp1EA=loc;Cyc_Tcdecl_merge_scope(_tmp1EE,_tmp1ED,_tmp1EC,_tmp1EB,_tmp1EA,msg,1);});});int _tmp33;enum Cyc_Absyn_Scope _tmp32;_LL1: _tmp32=_stmttmp2.f1;_tmp33=_stmttmp2.f2;_LL2: {enum Cyc_Absyn_Scope new_scope=_tmp32;int res_scope=_tmp33;
# 157
if(!res_scope)res=0;if(!({({struct Cyc_List_List*_tmp1F3=d0->attributes;struct Cyc_List_List*_tmp1F2=d1->attributes;struct _fat_ptr _tmp1F1=({const char*_tmp34="type";_tag_fat(_tmp34,sizeof(char),5U);});struct _fat_ptr _tmp1F0=v;unsigned _tmp1EF=loc;Cyc_Tcdecl_check_atts(_tmp1F3,_tmp1F2,_tmp1F1,_tmp1F0,_tmp1EF,msg);});}))
# 160
res=0;{
# 157
struct Cyc_Absyn_Aggrdecl*d2;
# 164
{struct _tuple15 _stmttmp3=({struct _tuple15 _tmp193;_tmp193.f1=d0->impl,_tmp193.f2=d1->impl;_tmp193;});struct _tuple15 _tmp35=_stmttmp3;int _tmp3D;struct Cyc_List_List*_tmp3C;struct Cyc_List_List*_tmp3B;struct Cyc_List_List*_tmp3A;int _tmp39;struct Cyc_List_List*_tmp38;struct Cyc_List_List*_tmp37;struct Cyc_List_List*_tmp36;if(_tmp35.f2 == 0){_LL4: _LL5:
 d2=d0;goto _LL3;}else{if(_tmp35.f1 == 0){_LL6: _LL7:
 d2=d1;goto _LL3;}else{_LL8: _tmp36=(_tmp35.f1)->exist_vars;_tmp37=(_tmp35.f1)->rgn_po;_tmp38=(_tmp35.f1)->fields;_tmp39=(_tmp35.f1)->tagged;_tmp3A=(_tmp35.f2)->exist_vars;_tmp3B=(_tmp35.f2)->rgn_po;_tmp3C=(_tmp35.f2)->fields;_tmp3D=(_tmp35.f2)->tagged;_LL9: {struct Cyc_List_List*exist_vars0=_tmp36;struct Cyc_List_List*rgn_po0=_tmp37;struct Cyc_List_List*f0s=_tmp38;int tagged0=_tmp39;struct Cyc_List_List*exist_vars1=_tmp3A;struct Cyc_List_List*rgn_po1=_tmp3B;struct Cyc_List_List*f1s=_tmp3C;int tagged1=_tmp3D;
# 171
if(!({({struct Cyc_List_List*_tmp1F8=exist_vars0;struct Cyc_List_List*_tmp1F7=exist_vars1;struct _fat_ptr _tmp1F6=({const char*_tmp3E="type";_tag_fat(_tmp3E,sizeof(char),5U);});struct _fat_ptr _tmp1F5=v;unsigned _tmp1F4=loc;Cyc_Tcdecl_check_tvs(_tmp1F8,_tmp1F7,_tmp1F6,_tmp1F5,_tmp1F4,msg);});}))
return 0;{
# 171
struct Cyc_List_List*inst=({struct Cyc_List_List*(*_tmp7F)(struct Cyc_List_List*tvs0,struct Cyc_List_List*tvs1)=Cyc_Tcdecl_build_tvs_map;struct Cyc_List_List*_tmp80=({Cyc_List_append(d0->tvs,exist_vars0);});struct Cyc_List_List*_tmp81=({Cyc_List_append(d1->tvs,exist_vars1);});_tmp7F(_tmp80,_tmp81);});
# 180
for(0;rgn_po0 != 0 && rgn_po1 != 0;(
rgn_po0=rgn_po0->tl,rgn_po1=rgn_po1->tl)){
({Cyc_Tcdecl_check_type((*((struct _tuple16*)rgn_po0->hd)).f1,(*((struct _tuple16*)rgn_po1->hd)).f1);});
({Cyc_Tcdecl_check_type((*((struct _tuple16*)rgn_po0->hd)).f2,(*((struct _tuple16*)rgn_po1->hd)).f2);});}
# 187
for(0;f0s != 0 && f1s != 0;(f0s=f0s->tl,f1s=f1s->tl)){
struct Cyc_Absyn_Aggrfield*_stmttmp4=(struct Cyc_Absyn_Aggrfield*)f0s->hd;struct Cyc_Absyn_Exp*_tmp44;struct Cyc_List_List*_tmp43;struct Cyc_Absyn_Exp*_tmp42;void*_tmp41;struct Cyc_Absyn_Tqual _tmp40;struct _fat_ptr*_tmp3F;_LLB: _tmp3F=_stmttmp4->name;_tmp40=_stmttmp4->tq;_tmp41=_stmttmp4->type;_tmp42=_stmttmp4->width;_tmp43=_stmttmp4->attributes;_tmp44=_stmttmp4->requires_clause;_LLC: {struct _fat_ptr*fn0=_tmp3F;struct Cyc_Absyn_Tqual tq0=_tmp40;void*t0=_tmp41;struct Cyc_Absyn_Exp*width0=_tmp42;struct Cyc_List_List*atts0=_tmp43;struct Cyc_Absyn_Exp*req0=_tmp44;
struct Cyc_Absyn_Aggrfield*_stmttmp5=(struct Cyc_Absyn_Aggrfield*)f1s->hd;struct Cyc_Absyn_Exp*_tmp4A;struct Cyc_List_List*_tmp49;struct Cyc_Absyn_Exp*_tmp48;void*_tmp47;struct Cyc_Absyn_Tqual _tmp46;struct _fat_ptr*_tmp45;_LLE: _tmp45=_stmttmp5->name;_tmp46=_stmttmp5->tq;_tmp47=_stmttmp5->type;_tmp48=_stmttmp5->width;_tmp49=_stmttmp5->attributes;_tmp4A=_stmttmp5->requires_clause;_LLF: {struct _fat_ptr*fn1=_tmp45;struct Cyc_Absyn_Tqual tq1=_tmp46;void*t1=_tmp47;struct Cyc_Absyn_Exp*width1=_tmp48;struct Cyc_List_List*atts1=_tmp49;struct Cyc_Absyn_Exp*req1=_tmp4A;
# 191
if(({Cyc_strptrcmp(fn0,fn1);})!= 0){
({struct Cyc_String_pa_PrintArg_struct _tmp4D=({struct Cyc_String_pa_PrintArg_struct _tmp17C;_tmp17C.tag=0U,({struct _fat_ptr _tmp1F9=(struct _fat_ptr)({const char*_tmp51="type";_tag_fat(_tmp51,sizeof(char),5U);});_tmp17C.f1=_tmp1F9;});_tmp17C;});struct Cyc_String_pa_PrintArg_struct _tmp4E=({struct Cyc_String_pa_PrintArg_struct _tmp17B;_tmp17B.tag=0U,_tmp17B.f1=(struct _fat_ptr)((struct _fat_ptr)v);_tmp17B;});struct Cyc_String_pa_PrintArg_struct _tmp4F=({struct Cyc_String_pa_PrintArg_struct _tmp17A;_tmp17A.tag=0U,_tmp17A.f1=(struct _fat_ptr)((struct _fat_ptr)*fn0);_tmp17A;});struct Cyc_String_pa_PrintArg_struct _tmp50=({struct Cyc_String_pa_PrintArg_struct _tmp179;_tmp179.tag=0U,_tmp179.f1=(struct _fat_ptr)((struct _fat_ptr)*fn1);_tmp179;});void*_tmp4B[4U];_tmp4B[0]=& _tmp4D,_tmp4B[1]=& _tmp4E,_tmp4B[2]=& _tmp4F,_tmp4B[3]=& _tmp50;({unsigned _tmp1FC=loc;struct _fat_ptr*_tmp1FB=msg;struct _fat_ptr _tmp1FA=({const char*_tmp4C="%s %s : field name mismatch %s != %s";_tag_fat(_tmp4C,sizeof(char),37U);});Cyc_Tcdecl_merr(_tmp1FC,_tmp1FB,_tmp1FA,_tag_fat(_tmp4B,sizeof(void*),4U));});});
# 194
return 0;}
# 191
if(!({Cyc_Tcutil_same_atts(atts0,atts1);})){
# 197
({struct Cyc_String_pa_PrintArg_struct _tmp54=({struct Cyc_String_pa_PrintArg_struct _tmp17F;_tmp17F.tag=0U,({struct _fat_ptr _tmp1FD=(struct _fat_ptr)({const char*_tmp57="type";_tag_fat(_tmp57,sizeof(char),5U);});_tmp17F.f1=_tmp1FD;});_tmp17F;});struct Cyc_String_pa_PrintArg_struct _tmp55=({struct Cyc_String_pa_PrintArg_struct _tmp17E;_tmp17E.tag=0U,_tmp17E.f1=(struct _fat_ptr)((struct _fat_ptr)v);_tmp17E;});struct Cyc_String_pa_PrintArg_struct _tmp56=({struct Cyc_String_pa_PrintArg_struct _tmp17D;_tmp17D.tag=0U,_tmp17D.f1=(struct _fat_ptr)((struct _fat_ptr)*fn0);_tmp17D;});void*_tmp52[3U];_tmp52[0]=& _tmp54,_tmp52[1]=& _tmp55,_tmp52[2]=& _tmp56;({unsigned _tmp200=loc;struct _fat_ptr*_tmp1FF=msg;struct _fat_ptr _tmp1FE=({const char*_tmp53="%s %s : attribute mismatch on field %s";_tag_fat(_tmp53,sizeof(char),39U);});Cyc_Tcdecl_merr(_tmp200,_tmp1FF,_tmp1FE,_tag_fat(_tmp52,sizeof(void*),3U));});});
res=0;}
# 191
if(!({Cyc_Absyn_equal_tqual(tq0,tq1);})){
# 201
({struct Cyc_String_pa_PrintArg_struct _tmp5A=({struct Cyc_String_pa_PrintArg_struct _tmp182;_tmp182.tag=0U,({struct _fat_ptr _tmp201=(struct _fat_ptr)({const char*_tmp5D="type";_tag_fat(_tmp5D,sizeof(char),5U);});_tmp182.f1=_tmp201;});_tmp182;});struct Cyc_String_pa_PrintArg_struct _tmp5B=({struct Cyc_String_pa_PrintArg_struct _tmp181;_tmp181.tag=0U,_tmp181.f1=(struct _fat_ptr)((struct _fat_ptr)v);_tmp181;});struct Cyc_String_pa_PrintArg_struct _tmp5C=({struct Cyc_String_pa_PrintArg_struct _tmp180;_tmp180.tag=0U,_tmp180.f1=(struct _fat_ptr)((struct _fat_ptr)*fn0);_tmp180;});void*_tmp58[3U];_tmp58[0]=& _tmp5A,_tmp58[1]=& _tmp5B,_tmp58[2]=& _tmp5C;({unsigned _tmp204=loc;struct _fat_ptr*_tmp203=msg;struct _fat_ptr _tmp202=({const char*_tmp59="%s %s : qualifier mismatch on field %s";_tag_fat(_tmp59,sizeof(char),39U);});Cyc_Tcdecl_merr(_tmp204,_tmp203,_tmp202,_tag_fat(_tmp58,sizeof(void*),3U));});});
res=0;}
# 191
if(
# 204
((width0 != 0 && width1 != 0)&&({
unsigned _tmp205=({Cyc_Tcdecl_get_uint_const_value(width0);});_tmp205 != ({Cyc_Tcdecl_get_uint_const_value(width1);});})||
# 207
 width0 == 0 && width1 != 0)||
 width0 != 0 && width1 == 0){
({struct Cyc_String_pa_PrintArg_struct _tmp60=({struct Cyc_String_pa_PrintArg_struct _tmp185;_tmp185.tag=0U,({struct _fat_ptr _tmp206=(struct _fat_ptr)({const char*_tmp63="type";_tag_fat(_tmp63,sizeof(char),5U);});_tmp185.f1=_tmp206;});_tmp185;});struct Cyc_String_pa_PrintArg_struct _tmp61=({struct Cyc_String_pa_PrintArg_struct _tmp184;_tmp184.tag=0U,_tmp184.f1=(struct _fat_ptr)((struct _fat_ptr)v);_tmp184;});struct Cyc_String_pa_PrintArg_struct _tmp62=({struct Cyc_String_pa_PrintArg_struct _tmp183;_tmp183.tag=0U,_tmp183.f1=(struct _fat_ptr)((struct _fat_ptr)*fn0);_tmp183;});void*_tmp5E[3U];_tmp5E[0]=& _tmp60,_tmp5E[1]=& _tmp61,_tmp5E[2]=& _tmp62;({unsigned _tmp209=loc;struct _fat_ptr*_tmp208=msg;struct _fat_ptr _tmp207=({const char*_tmp5F="%s %s : bitfield mismatch on field %s";_tag_fat(_tmp5F,sizeof(char),38U);});Cyc_Tcdecl_merr(_tmp209,_tmp208,_tmp207,_tag_fat(_tmp5E,sizeof(void*),3U));});});
res=0;}
# 191
if(
# 212
req0 != 0 && req1 != 0){
void*subst_w1=({({struct Cyc_List_List*_tmp20A=inst;Cyc_Tcutil_substitute(_tmp20A,(void*)({struct Cyc_Absyn_ValueofType_Absyn_Type_struct*_tmp6A=_cycalloc(sizeof(*_tmp6A));_tmp6A->tag=9U,_tmp6A->f1=req1;_tmp6A;}));});});
if(!({({void*_tmp20B=(void*)({struct Cyc_Absyn_ValueofType_Absyn_Type_struct*_tmp64=_cycalloc(sizeof(*_tmp64));_tmp64->tag=9U,_tmp64->f1=req0;_tmp64;});Cyc_Tcdecl_check_type(_tmp20B,subst_w1);});})){
({struct Cyc_String_pa_PrintArg_struct _tmp67=({struct Cyc_String_pa_PrintArg_struct _tmp188;_tmp188.tag=0U,_tmp188.f1=(struct _fat_ptr)((struct _fat_ptr)v);_tmp188;});struct Cyc_String_pa_PrintArg_struct _tmp68=({struct Cyc_String_pa_PrintArg_struct _tmp187;_tmp187.tag=0U,({
struct _fat_ptr _tmp20C=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_exp2string(req0);}));_tmp187.f1=_tmp20C;});_tmp187;});struct Cyc_String_pa_PrintArg_struct _tmp69=({struct Cyc_String_pa_PrintArg_struct _tmp186;_tmp186.tag=0U,({struct _fat_ptr _tmp20D=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_exp2string(req1);}));_tmp186.f1=_tmp20D;});_tmp186;});void*_tmp65[3U];_tmp65[0]=& _tmp67,_tmp65[1]=& _tmp68,_tmp65[2]=& _tmp69;({unsigned _tmp210=loc;struct _fat_ptr*_tmp20F=msg;struct _fat_ptr _tmp20E=({const char*_tmp66="type %s : mismatch on @requires clauses: %s != %s";_tag_fat(_tmp66,sizeof(char),50U);});Cyc_Tcdecl_merr(_tmp210,_tmp20F,_tmp20E,_tag_fat(_tmp65,sizeof(void*),3U));});});
res=0;}}else{
# 219
if(req0 == 0 && req1 != 0 ||
 req0 != 0 && req1 == 0){
({struct Cyc_String_pa_PrintArg_struct _tmp6D=({struct Cyc_String_pa_PrintArg_struct _tmp189;_tmp189.tag=0U,_tmp189.f1=(struct _fat_ptr)((struct _fat_ptr)v);_tmp189;});void*_tmp6B[1U];_tmp6B[0]=& _tmp6D;({unsigned _tmp213=loc;struct _fat_ptr*_tmp212=msg;struct _fat_ptr _tmp211=({const char*_tmp6C="type %s : mismatch on @requires clauses";_tag_fat(_tmp6C,sizeof(char),40U);});Cyc_Tcdecl_merr(_tmp213,_tmp212,_tmp211,_tag_fat(_tmp6B,sizeof(void*),1U));});});
res=0;}}{
# 191
void*subst_t1=({Cyc_Tcutil_substitute(inst,t1);});
# 226
if(!({Cyc_Tcdecl_check_type(t0,subst_t1);})){
({struct Cyc_String_pa_PrintArg_struct _tmp70=({struct Cyc_String_pa_PrintArg_struct _tmp18D;_tmp18D.tag=0U,_tmp18D.f1=(struct _fat_ptr)((struct _fat_ptr)v);_tmp18D;});struct Cyc_String_pa_PrintArg_struct _tmp71=({struct Cyc_String_pa_PrintArg_struct _tmp18C;_tmp18C.tag=0U,_tmp18C.f1=(struct _fat_ptr)((struct _fat_ptr)*fn0);_tmp18C;});struct Cyc_String_pa_PrintArg_struct _tmp72=({struct Cyc_String_pa_PrintArg_struct _tmp18B;_tmp18B.tag=0U,({
struct _fat_ptr _tmp214=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t0);}));_tmp18B.f1=_tmp214;});_tmp18B;});struct Cyc_String_pa_PrintArg_struct _tmp73=({struct Cyc_String_pa_PrintArg_struct _tmp18A;_tmp18A.tag=0U,({struct _fat_ptr _tmp215=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(subst_t1);}));_tmp18A.f1=_tmp215;});_tmp18A;});void*_tmp6E[4U];_tmp6E[0]=& _tmp70,_tmp6E[1]=& _tmp71,_tmp6E[2]=& _tmp72,_tmp6E[3]=& _tmp73;({unsigned _tmp218=loc;struct _fat_ptr*_tmp217=msg;struct _fat_ptr _tmp216=({const char*_tmp6F="type %s : type mismatch on field %s: %s != %s";_tag_fat(_tmp6F,sizeof(char),46U);});Cyc_Tcdecl_merr(_tmp218,_tmp217,_tmp216,_tag_fat(_tmp6E,sizeof(void*),4U));});});
({Cyc_Unify_explain_failure();});
res=0;}}}}}
# 234
if(f0s != 0){
({struct Cyc_String_pa_PrintArg_struct _tmp76=({struct Cyc_String_pa_PrintArg_struct _tmp18F;_tmp18F.tag=0U,_tmp18F.f1=(struct _fat_ptr)((struct _fat_ptr)v);_tmp18F;});struct Cyc_String_pa_PrintArg_struct _tmp77=({struct Cyc_String_pa_PrintArg_struct _tmp18E;_tmp18E.tag=0U,_tmp18E.f1=(struct _fat_ptr)((struct _fat_ptr)*((struct Cyc_Absyn_Aggrfield*)f0s->hd)->name);_tmp18E;});void*_tmp74[2U];_tmp74[0]=& _tmp76,_tmp74[1]=& _tmp77;({unsigned _tmp21B=loc;struct _fat_ptr*_tmp21A=msg;struct _fat_ptr _tmp219=({const char*_tmp75="type %s is missing field %s";_tag_fat(_tmp75,sizeof(char),28U);});Cyc_Tcdecl_merr(_tmp21B,_tmp21A,_tmp219,_tag_fat(_tmp74,sizeof(void*),2U));});});
res=0;}
# 234
if(f1s != 0){
# 239
({struct Cyc_String_pa_PrintArg_struct _tmp7A=({struct Cyc_String_pa_PrintArg_struct _tmp191;_tmp191.tag=0U,_tmp191.f1=(struct _fat_ptr)((struct _fat_ptr)v);_tmp191;});struct Cyc_String_pa_PrintArg_struct _tmp7B=({struct Cyc_String_pa_PrintArg_struct _tmp190;_tmp190.tag=0U,_tmp190.f1=(struct _fat_ptr)((struct _fat_ptr)*((struct Cyc_Absyn_Aggrfield*)f1s->hd)->name);_tmp190;});void*_tmp78[2U];_tmp78[0]=& _tmp7A,_tmp78[1]=& _tmp7B;({unsigned _tmp21E=loc;struct _fat_ptr*_tmp21D=msg;struct _fat_ptr _tmp21C=({const char*_tmp79="type %s has extra field %s";_tag_fat(_tmp79,sizeof(char),27U);});Cyc_Tcdecl_merr(_tmp21E,_tmp21D,_tmp21C,_tag_fat(_tmp78,sizeof(void*),2U));});});
res=0;}
# 234
if(tagged0 != tagged1){
# 244
({struct Cyc_String_pa_PrintArg_struct _tmp7E=({struct Cyc_String_pa_PrintArg_struct _tmp192;_tmp192.tag=0U,_tmp192.f1=(struct _fat_ptr)((struct _fat_ptr)v);_tmp192;});void*_tmp7C[1U];_tmp7C[0]=& _tmp7E;({unsigned _tmp221=loc;struct _fat_ptr*_tmp220=msg;struct _fat_ptr _tmp21F=({const char*_tmp7D="%s : previous declaration disagrees with @tagged qualifier";_tag_fat(_tmp7D,sizeof(char),59U);});Cyc_Tcdecl_merr(_tmp221,_tmp220,_tmp21F,_tag_fat(_tmp7C,sizeof(void*),1U));});});
res=0;}
# 234
d2=d0;
# 249
goto _LL3;}}}}_LL3:;}
# 252
if(!res)return 0;if((int)new_scope == (int)d2->sc)
# 255
return d2;else{
# 257
d2=({struct Cyc_Absyn_Aggrdecl*_tmp82=_cycalloc(sizeof(*_tmp82));*_tmp82=*d2;_tmp82;});
d2->sc=new_scope;
return d2;}}}}}
# 263
inline static struct _fat_ptr Cyc_Tcdecl_is_x2string(int is_x){
return is_x?({const char*_tmp85="@extensible datatype";_tag_fat(_tmp85,sizeof(char),21U);}):({const char*_tmp86="datatype";_tag_fat(_tmp86,sizeof(char),9U);});}struct _tuple17{struct Cyc_Absyn_Tqual f1;void*f2;};
# 270
static struct Cyc_Absyn_Datatypefield*Cyc_Tcdecl_merge_datatypefield(struct Cyc_Absyn_Datatypefield*f0,struct Cyc_Absyn_Datatypefield*f1,struct Cyc_List_List*inst,struct _fat_ptr t,struct _fat_ptr v,struct _fat_ptr*msg){
# 273
unsigned loc=f1->loc;
# 276
if(({Cyc_strptrcmp((*f0->name).f2,(*f1->name).f2);})!= 0){
({struct Cyc_String_pa_PrintArg_struct _tmp8A=({struct Cyc_String_pa_PrintArg_struct _tmp197;_tmp197.tag=0U,_tmp197.f1=(struct _fat_ptr)((struct _fat_ptr)t);_tmp197;});struct Cyc_String_pa_PrintArg_struct _tmp8B=({struct Cyc_String_pa_PrintArg_struct _tmp196;_tmp196.tag=0U,_tmp196.f1=(struct _fat_ptr)((struct _fat_ptr)v);_tmp196;});struct Cyc_String_pa_PrintArg_struct _tmp8C=({struct Cyc_String_pa_PrintArg_struct _tmp195;_tmp195.tag=0U,_tmp195.f1=(struct _fat_ptr)((struct _fat_ptr)*(*f1->name).f2);_tmp195;});struct Cyc_String_pa_PrintArg_struct _tmp8D=({struct Cyc_String_pa_PrintArg_struct _tmp194;_tmp194.tag=0U,_tmp194.f1=(struct _fat_ptr)((struct _fat_ptr)*(*f0->name).f2);_tmp194;});void*_tmp88[4U];_tmp88[0]=& _tmp8A,_tmp88[1]=& _tmp8B,_tmp88[2]=& _tmp8C,_tmp88[3]=& _tmp8D;({unsigned _tmp224=loc;struct _fat_ptr*_tmp223=msg;struct _fat_ptr _tmp222=({const char*_tmp89="%s %s: field name mismatch %s != %s";_tag_fat(_tmp89,sizeof(char),36U);});Cyc_Tcdecl_merr(_tmp224,_tmp223,_tmp222,_tag_fat(_tmp88,sizeof(void*),4U));});});
# 279
return 0;}{
# 276
struct _fat_ptr name=*(*f0->name).f2;
# 285
struct _tuple12 _stmttmp6=({struct _tuple12(*_tmpA2)(enum Cyc_Absyn_Scope s0,enum Cyc_Absyn_Scope s1,struct _fat_ptr t,struct _fat_ptr v,unsigned loc,struct _fat_ptr*msg,int externCmerge)=Cyc_Tcdecl_merge_scope;enum Cyc_Absyn_Scope _tmpA3=f0->sc;enum Cyc_Absyn_Scope _tmpA4=f1->sc;struct _fat_ptr _tmpA5=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmpA8=({struct Cyc_String_pa_PrintArg_struct _tmp1A4;_tmp1A4.tag=0U,_tmp1A4.f1=(struct _fat_ptr)((struct _fat_ptr)t);_tmp1A4;});struct Cyc_String_pa_PrintArg_struct _tmpA9=({struct Cyc_String_pa_PrintArg_struct _tmp1A3;_tmp1A3.tag=0U,_tmp1A3.f1=(struct _fat_ptr)((struct _fat_ptr)v);_tmp1A3;});void*_tmpA6[2U];_tmpA6[0]=& _tmpA8,_tmpA6[1]=& _tmpA9;({struct _fat_ptr _tmp225=({const char*_tmpA7="in %s %s, field";_tag_fat(_tmpA7,sizeof(char),16U);});Cyc_aprintf(_tmp225,_tag_fat(_tmpA6,sizeof(void*),2U));});});struct _fat_ptr _tmpAA=name;unsigned _tmpAB=loc;struct _fat_ptr*_tmpAC=msg;int _tmpAD=0;_tmpA2(_tmpA3,_tmpA4,_tmpA5,_tmpAA,_tmpAB,_tmpAC,_tmpAD);});int _tmp8F;enum Cyc_Absyn_Scope _tmp8E;_LL1: _tmp8E=_stmttmp6.f1;_tmp8F=_stmttmp6.f2;_LL2: {enum Cyc_Absyn_Scope new_scope=_tmp8E;int res=_tmp8F;
# 289
struct Cyc_List_List*typs0=f0->typs;
struct Cyc_List_List*typs1=f1->typs;
if(({int _tmp226=({Cyc_List_length(typs0);});_tmp226 != ({Cyc_List_length(typs1);});})){
({struct Cyc_String_pa_PrintArg_struct _tmp92=({struct Cyc_String_pa_PrintArg_struct _tmp19A;_tmp19A.tag=0U,_tmp19A.f1=(struct _fat_ptr)((struct _fat_ptr)t);_tmp19A;});struct Cyc_String_pa_PrintArg_struct _tmp93=({struct Cyc_String_pa_PrintArg_struct _tmp199;_tmp199.tag=0U,_tmp199.f1=(struct _fat_ptr)((struct _fat_ptr)v);_tmp199;});struct Cyc_String_pa_PrintArg_struct _tmp94=({struct Cyc_String_pa_PrintArg_struct _tmp198;_tmp198.tag=0U,_tmp198.f1=(struct _fat_ptr)((struct _fat_ptr)name);_tmp198;});void*_tmp90[3U];_tmp90[0]=& _tmp92,_tmp90[1]=& _tmp93,_tmp90[2]=& _tmp94;({unsigned _tmp229=loc;struct _fat_ptr*_tmp228=msg;struct _fat_ptr _tmp227=({const char*_tmp91="%s %s, field %s: parameter number mismatch";_tag_fat(_tmp91,sizeof(char),43U);});Cyc_Tcdecl_merr(_tmp229,_tmp228,_tmp227,_tag_fat(_tmp90,sizeof(void*),3U));});});
res=0;}
# 291
for(0;
# 296
typs0 != 0 && typs1 != 0;(typs0=typs0->tl,typs1=typs1->tl)){
# 298
if(!({Cyc_Absyn_equal_tqual((*((struct _tuple17*)typs0->hd)).f1,(*((struct _tuple17*)typs1->hd)).f1);})){
({struct Cyc_String_pa_PrintArg_struct _tmp97=({struct Cyc_String_pa_PrintArg_struct _tmp19D;_tmp19D.tag=0U,_tmp19D.f1=(struct _fat_ptr)((struct _fat_ptr)t);_tmp19D;});struct Cyc_String_pa_PrintArg_struct _tmp98=({struct Cyc_String_pa_PrintArg_struct _tmp19C;_tmp19C.tag=0U,_tmp19C.f1=(struct _fat_ptr)((struct _fat_ptr)v);_tmp19C;});struct Cyc_String_pa_PrintArg_struct _tmp99=({struct Cyc_String_pa_PrintArg_struct _tmp19B;_tmp19B.tag=0U,_tmp19B.f1=(struct _fat_ptr)((struct _fat_ptr)name);_tmp19B;});void*_tmp95[3U];_tmp95[0]=& _tmp97,_tmp95[1]=& _tmp98,_tmp95[2]=& _tmp99;({unsigned _tmp22C=loc;struct _fat_ptr*_tmp22B=msg;struct _fat_ptr _tmp22A=({const char*_tmp96="%s %s, field %s: parameter qualifier";_tag_fat(_tmp96,sizeof(char),37U);});Cyc_Tcdecl_merr(_tmp22C,_tmp22B,_tmp22A,_tag_fat(_tmp95,sizeof(void*),3U));});});
res=0;}{
# 298
void*subst_t1=({Cyc_Tcutil_substitute(inst,(*((struct _tuple17*)typs1->hd)).f2);});
# 304
if(!({Cyc_Tcdecl_check_type((*((struct _tuple17*)typs0->hd)).f2,subst_t1);})){
({struct Cyc_String_pa_PrintArg_struct _tmp9C=({struct Cyc_String_pa_PrintArg_struct _tmp1A2;_tmp1A2.tag=0U,_tmp1A2.f1=(struct _fat_ptr)((struct _fat_ptr)t);_tmp1A2;});struct Cyc_String_pa_PrintArg_struct _tmp9D=({struct Cyc_String_pa_PrintArg_struct _tmp1A1;_tmp1A1.tag=0U,_tmp1A1.f1=(struct _fat_ptr)((struct _fat_ptr)v);_tmp1A1;});struct Cyc_String_pa_PrintArg_struct _tmp9E=({struct Cyc_String_pa_PrintArg_struct _tmp1A0;_tmp1A0.tag=0U,_tmp1A0.f1=(struct _fat_ptr)((struct _fat_ptr)name);_tmp1A0;});struct Cyc_String_pa_PrintArg_struct _tmp9F=({struct Cyc_String_pa_PrintArg_struct _tmp19F;_tmp19F.tag=0U,({
struct _fat_ptr _tmp22D=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string((*((struct _tuple17*)typs0->hd)).f2);}));_tmp19F.f1=_tmp22D;});_tmp19F;});struct Cyc_String_pa_PrintArg_struct _tmpA0=({struct Cyc_String_pa_PrintArg_struct _tmp19E;_tmp19E.tag=0U,({struct _fat_ptr _tmp22E=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(subst_t1);}));_tmp19E.f1=_tmp22E;});_tmp19E;});void*_tmp9A[5U];_tmp9A[0]=& _tmp9C,_tmp9A[1]=& _tmp9D,_tmp9A[2]=& _tmp9E,_tmp9A[3]=& _tmp9F,_tmp9A[4]=& _tmpA0;({unsigned _tmp231=loc;struct _fat_ptr*_tmp230=msg;struct _fat_ptr _tmp22F=({const char*_tmp9B="%s %s, field %s: parameter type mismatch %s != %s";_tag_fat(_tmp9B,sizeof(char),50U);});Cyc_Tcdecl_merr(_tmp231,_tmp230,_tmp22F,_tag_fat(_tmp9A,sizeof(void*),5U));});});
({Cyc_Unify_explain_failure();});
res=0;}}}
# 312
if(!res)return 0;if((int)f0->sc != (int)new_scope){
# 314
struct Cyc_Absyn_Datatypefield*f2=({struct Cyc_Absyn_Datatypefield*_tmpA1=_cycalloc(sizeof(*_tmpA1));*_tmpA1=*f0;_tmpA1;});
f2->sc=new_scope;
return f2;}else{
return f0;}}}}
# 321
static struct _tuple17*Cyc_Tcdecl_substitute_datatypefield_f2(struct Cyc_List_List*inst,struct _tuple17*x){
# 323
void*_tmpB0;struct Cyc_Absyn_Tqual _tmpAF;_LL1: _tmpAF=x->f1;_tmpB0=x->f2;_LL2: {struct Cyc_Absyn_Tqual tq=_tmpAF;void*t=_tmpB0;
return({struct _tuple17*_tmpB1=_cycalloc(sizeof(*_tmpB1));_tmpB1->f1=tq,({void*_tmp232=({Cyc_Tcutil_substitute(inst,t);});_tmpB1->f2=_tmp232;});_tmpB1;});}}
# 329
static struct Cyc_Absyn_Datatypefield*Cyc_Tcdecl_substitute_datatypefield(struct Cyc_List_List*inst1,struct Cyc_Absyn_Datatypefield*f1){
# 331
struct Cyc_Absyn_Datatypefield*f0=({struct Cyc_Absyn_Datatypefield*_tmpB4=_cycalloc(sizeof(*_tmpB4));*_tmpB4=*f1;_tmpB4;});
({struct Cyc_List_List*_tmp235=({({struct Cyc_List_List*(*_tmp234)(struct _tuple17*(*f)(struct Cyc_List_List*,struct _tuple17*),struct Cyc_List_List*env,struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmpB3)(struct _tuple17*(*f)(struct Cyc_List_List*,struct _tuple17*),struct Cyc_List_List*env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct _tuple17*(*f)(struct Cyc_List_List*,struct _tuple17*),struct Cyc_List_List*env,struct Cyc_List_List*x))Cyc_List_map_c;_tmpB3;});struct Cyc_List_List*_tmp233=inst1;_tmp234(Cyc_Tcdecl_substitute_datatypefield_f2,_tmp233,f1->typs);});});f0->typs=_tmp235;});
# 334
return f0;}
# 342
static struct Cyc_List_List*Cyc_Tcdecl_merge_xdatatype_fields(struct Cyc_List_List*f0s,struct Cyc_List_List*f1s,struct Cyc_List_List*inst,struct Cyc_List_List*tvs0,struct Cyc_List_List*tvs1,int*res,int*incl,struct _fat_ptr t,struct _fat_ptr v,unsigned loc,struct _fat_ptr*msg){
# 350
struct Cyc_List_List**f2sp=({struct Cyc_List_List**_tmpB9=_cycalloc(sizeof(*_tmpB9));*_tmpB9=0;_tmpB9;});
struct Cyc_List_List**next=f2sp;
int cmp=-1;
# 354
for(0;f0s != 0 && f1s != 0;f1s=f1s->tl){
while(f0s != 0 &&(cmp=({Cyc_Absyn_qvar_cmp(((struct Cyc_Absyn_Datatypefield*)f0s->hd)->name,((struct Cyc_Absyn_Datatypefield*)f1s->hd)->name);}))< 0){
struct Cyc_List_List*node=({struct Cyc_List_List*_tmpB6=_cycalloc(sizeof(*_tmpB6));_tmpB6->hd=(struct Cyc_Absyn_Datatypefield*)f0s->hd,_tmpB6->tl=0;_tmpB6;});
*next=node;
next=& node->tl;
f0s=f0s->tl;}
# 361
if(f0s == 0 || cmp > 0){
# 363
*incl=0;{
struct Cyc_List_List*node=({struct Cyc_List_List*_tmpB7=_cycalloc(sizeof(*_tmpB7));({struct Cyc_Absyn_Datatypefield*_tmp236=({Cyc_Tcdecl_substitute_datatypefield(inst,(struct Cyc_Absyn_Datatypefield*)f1s->hd);});_tmpB7->hd=_tmp236;}),_tmpB7->tl=0;_tmpB7;});
*next=node;
next=& node->tl;}}else{
# 369
struct Cyc_Absyn_Datatypefield*f2=({Cyc_Tcdecl_merge_datatypefield((struct Cyc_Absyn_Datatypefield*)f0s->hd,(struct Cyc_Absyn_Datatypefield*)f1s->hd,inst,t,v,msg);});
if(f2 != 0){
if(f2 != (struct Cyc_Absyn_Datatypefield*)f0s->hd)*incl=0;{struct Cyc_List_List*node=({struct Cyc_List_List*_tmpB8=_cycalloc(sizeof(*_tmpB8));
_tmpB8->hd=f2,_tmpB8->tl=0;_tmpB8;});
*next=node;
next=& node->tl;}}else{
# 376
*res=0;}
# 378
f0s=f0s->tl;}}
# 383
if(f1s != 0){
*incl=0;
*next=f1s;}else{
# 387
*next=f0s;}
# 390
return*f2sp;}
# 394
static struct _tuple8 Cyc_Tcdecl_split(struct Cyc_List_List*f){
if(f == 0)return({struct _tuple8 _tmp1A5;_tmp1A5.f1=0,_tmp1A5.f2=0;_tmp1A5;});if(f->tl == 0)
return({struct _tuple8 _tmp1A6;_tmp1A6.f1=f,_tmp1A6.f2=0;_tmp1A6;});{
# 395
struct _tuple8 _stmttmp7=({Cyc_Tcdecl_split(((struct Cyc_List_List*)_check_null(f->tl))->tl);});struct Cyc_List_List*_tmpBC;struct Cyc_List_List*_tmpBB;_LL1: _tmpBB=_stmttmp7.f1;_tmpBC=_stmttmp7.f2;_LL2: {struct Cyc_List_List*f1=_tmpBB;struct Cyc_List_List*f2=_tmpBC;
# 398
return({struct _tuple8 _tmp1A7;({struct Cyc_List_List*_tmp238=({struct Cyc_List_List*_tmpBD=_cycalloc(sizeof(*_tmpBD));_tmpBD->hd=f->hd,_tmpBD->tl=f1;_tmpBD;});_tmp1A7.f1=_tmp238;}),({struct Cyc_List_List*_tmp237=({struct Cyc_List_List*_tmpBE=_cycalloc(sizeof(*_tmpBE));_tmpBE->hd=((struct Cyc_List_List*)_check_null(f->tl))->hd,_tmpBE->tl=f2;_tmpBE;});_tmp1A7.f2=_tmp237;});_tmp1A7;});}}}
# 403
struct Cyc_List_List*Cyc_Tcdecl_sort_xdatatype_fields(struct Cyc_List_List*f,int*res,struct _fat_ptr*v,unsigned loc,struct _fat_ptr*msg){
# 406
struct _tuple8 _stmttmp8=({Cyc_Tcdecl_split(f);});struct Cyc_List_List*_tmpC1;struct Cyc_List_List*_tmpC0;_LL1: _tmpC0=_stmttmp8.f1;_tmpC1=_stmttmp8.f2;_LL2: {struct Cyc_List_List*f1=_tmpC0;struct Cyc_List_List*f2=_tmpC1;
if(f1 != 0 && f1->tl != 0)
f1=({Cyc_Tcdecl_sort_xdatatype_fields(f1,res,v,loc,msg);});
# 407
if(
# 409
f2 != 0 && f2->tl != 0)
f2=({Cyc_Tcdecl_sort_xdatatype_fields(f2,res,v,loc,msg);});
# 407
return({({struct Cyc_List_List*_tmp23F=f1;struct Cyc_List_List*_tmp23E=f2;int*_tmp23D=res;int*_tmp23C=({int*_tmpC2=_cycalloc_atomic(sizeof(*_tmpC2));*_tmpC2=1;_tmpC2;});struct _fat_ptr _tmp23B=({const char*_tmpC3="@extensible datatype";_tag_fat(_tmpC3,sizeof(char),21U);});struct _fat_ptr _tmp23A=*v;unsigned _tmp239=loc;Cyc_Tcdecl_merge_xdatatype_fields(_tmp23F,_tmp23E,0,0,0,_tmp23D,_tmp23C,_tmp23B,_tmp23A,_tmp239,msg);});});}}struct _tuple18{struct Cyc_Core_Opt*f1;struct Cyc_Core_Opt*f2;};
# 415
struct Cyc_Absyn_Datatypedecl*Cyc_Tcdecl_merge_datatypedecl(struct Cyc_Absyn_Datatypedecl*d0,struct Cyc_Absyn_Datatypedecl*d1,unsigned loc,struct _fat_ptr*msg){
# 417
struct _fat_ptr v=({Cyc_Absynpp_qvar2string(d0->name);});
struct _fat_ptr t=({const char*_tmpDC="datatype";_tag_fat(_tmpDC,sizeof(char),9U);});
int res=1;
# 424
if(d0->is_extensible != d1->is_extensible){
({struct Cyc_String_pa_PrintArg_struct _tmpC7=({struct Cyc_String_pa_PrintArg_struct _tmp1AA;_tmp1AA.tag=0U,_tmp1AA.f1=(struct _fat_ptr)((struct _fat_ptr)v);_tmp1AA;});struct Cyc_String_pa_PrintArg_struct _tmpC8=({struct Cyc_String_pa_PrintArg_struct _tmp1A9;_tmp1A9.tag=0U,({
struct _fat_ptr _tmp240=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Tcdecl_is_x2string(d0->is_extensible);}));_tmp1A9.f1=_tmp240;});_tmp1A9;});struct Cyc_String_pa_PrintArg_struct _tmpC9=({struct Cyc_String_pa_PrintArg_struct _tmp1A8;_tmp1A8.tag=0U,({struct _fat_ptr _tmp241=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Tcdecl_is_x2string(d1->is_extensible);}));_tmp1A8.f1=_tmp241;});_tmp1A8;});void*_tmpC5[3U];_tmpC5[0]=& _tmpC7,_tmpC5[1]=& _tmpC8,_tmpC5[2]=& _tmpC9;({unsigned _tmp244=loc;struct _fat_ptr*_tmp243=msg;struct _fat_ptr _tmp242=({const char*_tmpC6="expected %s to be a %s instead of a %s";_tag_fat(_tmpC6,sizeof(char),39U);});Cyc_Tcdecl_merr(_tmp244,_tmp243,_tmp242,_tag_fat(_tmpC5,sizeof(void*),3U));});});
res=0;}else{
# 429
t=({Cyc_Tcdecl_is_x2string(d0->is_extensible);});}
# 432
if(!({Cyc_Tcdecl_check_tvs(d0->tvs,d1->tvs,t,v,loc,msg);}))return 0;{struct _tuple12 _stmttmp9=({Cyc_Tcdecl_merge_scope(d0->sc,d1->sc,t,v,loc,msg,0);});int _tmpCB;enum Cyc_Absyn_Scope _tmpCA;_LL1: _tmpCA=_stmttmp9.f1;_tmpCB=_stmttmp9.f2;_LL2: {enum Cyc_Absyn_Scope new_scope=_tmpCA;int res_scope=_tmpCB;
# 436
if(!res_scope)res=0;{struct Cyc_Absyn_Datatypedecl*d2;
# 441
{struct _tuple18 _stmttmpA=({struct _tuple18 _tmp1B1;_tmp1B1.f1=d0->fields,_tmp1B1.f2=d1->fields;_tmp1B1;});struct _tuple18 _tmpCC=_stmttmpA;struct Cyc_List_List*_tmpCE;struct Cyc_List_List*_tmpCD;if(_tmpCC.f2 == 0){_LL4: _LL5:
 d2=d0;goto _LL3;}else{if(_tmpCC.f1 == 0){_LL6: _LL7:
 d2=d1;goto _LL3;}else{_LL8: _tmpCD=(struct Cyc_List_List*)(_tmpCC.f1)->v;_tmpCE=(struct Cyc_List_List*)(_tmpCC.f2)->v;_LL9: {struct Cyc_List_List*f0s=_tmpCD;struct Cyc_List_List*f1s=_tmpCE;
# 455 "tcdecl.cyc"
struct Cyc_List_List*inst=({Cyc_Tcdecl_build_tvs_map(d0->tvs,d1->tvs);});
# 458
if(d0->is_extensible){
# 462
int incl=1;
struct Cyc_List_List*f2s=({Cyc_Tcdecl_merge_xdatatype_fields(f0s,f1s,inst,d0->tvs,d1->tvs,& res,& incl,t,v,loc,msg);});
# 466
if(incl)
d2=d0;else{
# 469
d2=({struct Cyc_Absyn_Datatypedecl*_tmpCF=_cycalloc(sizeof(*_tmpCF));*_tmpCF=*d0;_tmpCF;});
d2->sc=new_scope;
({struct Cyc_Core_Opt*_tmp245=({struct Cyc_Core_Opt*_tmpD0=_cycalloc(sizeof(*_tmpD0));_tmpD0->v=f2s;_tmpD0;});d2->fields=_tmp245;});}}else{
# 475
for(0;f0s != 0 && f1s != 0;(f0s=f0s->tl,f1s=f1s->tl)){
({Cyc_Tcdecl_merge_datatypefield((struct Cyc_Absyn_Datatypefield*)f0s->hd,(struct Cyc_Absyn_Datatypefield*)f1s->hd,inst,t,v,msg);});}
# 478
if(f0s != 0){
({struct Cyc_String_pa_PrintArg_struct _tmpD3=({struct Cyc_String_pa_PrintArg_struct _tmp1AD;_tmp1AD.tag=0U,_tmp1AD.f1=(struct _fat_ptr)((struct _fat_ptr)t);_tmp1AD;});struct Cyc_String_pa_PrintArg_struct _tmpD4=({struct Cyc_String_pa_PrintArg_struct _tmp1AC;_tmp1AC.tag=0U,_tmp1AC.f1=(struct _fat_ptr)((struct _fat_ptr)v);_tmp1AC;});struct Cyc_String_pa_PrintArg_struct _tmpD5=({struct Cyc_String_pa_PrintArg_struct _tmp1AB;_tmp1AB.tag=0U,_tmp1AB.f1=(struct _fat_ptr)((struct _fat_ptr)*(*((struct Cyc_Absyn_Datatypefield*)f0s->hd)->name).f2);_tmp1AB;});void*_tmpD1[3U];_tmpD1[0]=& _tmpD3,_tmpD1[1]=& _tmpD4,_tmpD1[2]=& _tmpD5;({unsigned _tmp248=loc;struct _fat_ptr*_tmp247=msg;struct _fat_ptr _tmp246=({const char*_tmpD2="%s %s is missing field %s";_tag_fat(_tmpD2,sizeof(char),26U);});Cyc_Tcdecl_merr(_tmp248,_tmp247,_tmp246,_tag_fat(_tmpD1,sizeof(void*),3U));});});
res=0;}
# 478
if(f1s != 0){
# 483
({struct Cyc_String_pa_PrintArg_struct _tmpD8=({struct Cyc_String_pa_PrintArg_struct _tmp1B0;_tmp1B0.tag=0U,_tmp1B0.f1=(struct _fat_ptr)((struct _fat_ptr)t);_tmp1B0;});struct Cyc_String_pa_PrintArg_struct _tmpD9=({struct Cyc_String_pa_PrintArg_struct _tmp1AF;_tmp1AF.tag=0U,_tmp1AF.f1=(struct _fat_ptr)((struct _fat_ptr)v);_tmp1AF;});struct Cyc_String_pa_PrintArg_struct _tmpDA=({struct Cyc_String_pa_PrintArg_struct _tmp1AE;_tmp1AE.tag=0U,_tmp1AE.f1=(struct _fat_ptr)((struct _fat_ptr)*(*((struct Cyc_Absyn_Datatypefield*)f1s->hd)->name).f2);_tmp1AE;});void*_tmpD6[3U];_tmpD6[0]=& _tmpD8,_tmpD6[1]=& _tmpD9,_tmpD6[2]=& _tmpDA;({unsigned _tmp24B=loc;struct _fat_ptr*_tmp24A=msg;struct _fat_ptr _tmp249=({const char*_tmpD7="%s %s has extra field %s";_tag_fat(_tmpD7,sizeof(char),25U);});Cyc_Tcdecl_merr(_tmp24B,_tmp24A,_tmp249,_tag_fat(_tmpD6,sizeof(void*),3U));});});
res=0;}
# 478
d2=d0;}
# 488
goto _LL3;}}}_LL3:;}
# 491
if(!res)return 0;if((int)new_scope == (int)d2->sc)
# 494
return d2;else{
# 496
d2=({struct Cyc_Absyn_Datatypedecl*_tmpDB=_cycalloc(sizeof(*_tmpDB));*_tmpDB=*d2;_tmpDB;});
d2->sc=new_scope;
return d2;}}}}}
# 502
struct Cyc_Absyn_Enumdecl*Cyc_Tcdecl_merge_enumdecl(struct Cyc_Absyn_Enumdecl*d0,struct Cyc_Absyn_Enumdecl*d1,unsigned loc,struct _fat_ptr*msg){
# 504
struct _fat_ptr v=({Cyc_Absynpp_qvar2string(d0->name);});
int res=1;
# 508
struct _tuple12 _stmttmpB=({({enum Cyc_Absyn_Scope _tmp250=d0->sc;enum Cyc_Absyn_Scope _tmp24F=d1->sc;struct _fat_ptr _tmp24E=({const char*_tmpF6="enum";_tag_fat(_tmpF6,sizeof(char),5U);});struct _fat_ptr _tmp24D=v;unsigned _tmp24C=loc;Cyc_Tcdecl_merge_scope(_tmp250,_tmp24F,_tmp24E,_tmp24D,_tmp24C,msg,1);});});int _tmpDF;enum Cyc_Absyn_Scope _tmpDE;_LL1: _tmpDE=_stmttmpB.f1;_tmpDF=_stmttmpB.f2;_LL2: {enum Cyc_Absyn_Scope new_scope=_tmpDE;int res_scope=_tmpDF;
if(!res_scope)res=0;{struct Cyc_Absyn_Enumdecl*d2;
# 514
{struct _tuple18 _stmttmpC=({struct _tuple18 _tmp1B8;_tmp1B8.f1=d0->fields,_tmp1B8.f2=d1->fields;_tmp1B8;});struct _tuple18 _tmpE0=_stmttmpC;struct Cyc_List_List*_tmpE2;struct Cyc_List_List*_tmpE1;if(_tmpE0.f2 == 0){_LL4: _LL5:
 d2=d0;goto _LL3;}else{if(_tmpE0.f1 == 0){_LL6: _LL7:
 d2=d1;goto _LL3;}else{_LL8: _tmpE1=(struct Cyc_List_List*)(_tmpE0.f1)->v;_tmpE2=(struct Cyc_List_List*)(_tmpE0.f2)->v;_LL9: {struct Cyc_List_List*f0s=_tmpE1;struct Cyc_List_List*f1s=_tmpE2;
# 519
for(0;f0s != 0 && f1s != 0;(f0s=f0s->tl,f1s=f1s->tl)){
struct Cyc_Absyn_Enumfield*_stmttmpD=(struct Cyc_Absyn_Enumfield*)f0s->hd;unsigned _tmpE5;struct Cyc_Absyn_Exp*_tmpE4;struct _fat_ptr*_tmpE3;_LLB: _tmpE3=(_stmttmpD->name)->f2;_tmpE4=_stmttmpD->tag;_tmpE5=_stmttmpD->loc;_LLC: {struct _fat_ptr*fname0=_tmpE3;struct Cyc_Absyn_Exp*tag0=_tmpE4;unsigned loc0=_tmpE5;
struct Cyc_Absyn_Enumfield*_stmttmpE=(struct Cyc_Absyn_Enumfield*)f1s->hd;unsigned _tmpE8;struct Cyc_Absyn_Exp*_tmpE7;struct _fat_ptr*_tmpE6;_LLE: _tmpE6=(_stmttmpE->name)->f2;_tmpE7=_stmttmpE->tag;_tmpE8=_stmttmpE->loc;_LLF: {struct _fat_ptr*fname1=_tmpE6;struct Cyc_Absyn_Exp*tag1=_tmpE7;unsigned loc1=_tmpE8;
# 523
if(({Cyc_strptrcmp(fname0,fname1);})!= 0){
({struct Cyc_String_pa_PrintArg_struct _tmpEB=({struct Cyc_String_pa_PrintArg_struct _tmp1B4;_tmp1B4.tag=0U,_tmp1B4.f1=(struct _fat_ptr)((struct _fat_ptr)v);_tmp1B4;});struct Cyc_String_pa_PrintArg_struct _tmpEC=({struct Cyc_String_pa_PrintArg_struct _tmp1B3;_tmp1B3.tag=0U,_tmp1B3.f1=(struct _fat_ptr)((struct _fat_ptr)*fname0);_tmp1B3;});struct Cyc_String_pa_PrintArg_struct _tmpED=({struct Cyc_String_pa_PrintArg_struct _tmp1B2;_tmp1B2.tag=0U,_tmp1B2.f1=(struct _fat_ptr)((struct _fat_ptr)*fname1);_tmp1B2;});void*_tmpE9[3U];_tmpE9[0]=& _tmpEB,_tmpE9[1]=& _tmpEC,_tmpE9[2]=& _tmpED;({unsigned _tmp253=loc;struct _fat_ptr*_tmp252=msg;struct _fat_ptr _tmp251=({const char*_tmpEA="enum %s: field name mismatch %s != %s";_tag_fat(_tmpEA,sizeof(char),38U);});Cyc_Tcdecl_merr(_tmp253,_tmp252,_tmp251,_tag_fat(_tmpE9,sizeof(void*),3U));});});
res=0;}
# 523
if(({
# 527
unsigned _tmp254=({Cyc_Tcdecl_get_uint_const_value((struct Cyc_Absyn_Exp*)_check_null(tag0));});_tmp254 != ({Cyc_Tcdecl_get_uint_const_value((struct Cyc_Absyn_Exp*)_check_null(tag1));});})){
({struct Cyc_String_pa_PrintArg_struct _tmpF0=({struct Cyc_String_pa_PrintArg_struct _tmp1B6;_tmp1B6.tag=0U,_tmp1B6.f1=(struct _fat_ptr)((struct _fat_ptr)v);_tmp1B6;});struct Cyc_String_pa_PrintArg_struct _tmpF1=({struct Cyc_String_pa_PrintArg_struct _tmp1B5;_tmp1B5.tag=0U,_tmp1B5.f1=(struct _fat_ptr)((struct _fat_ptr)*fname1);_tmp1B5;});void*_tmpEE[2U];_tmpEE[0]=& _tmpF0,_tmpEE[1]=& _tmpF1;({unsigned _tmp257=loc;struct _fat_ptr*_tmp256=msg;struct _fat_ptr _tmp255=({const char*_tmpEF="enum %s, field %s, value mismatch";_tag_fat(_tmpEF,sizeof(char),34U);});Cyc_Tcdecl_merr(_tmp257,_tmp256,_tmp255,_tag_fat(_tmpEE,sizeof(void*),2U));});});
res=0;}}}}
# 532
if(f0s != 0 || f1s != 0){
({struct Cyc_String_pa_PrintArg_struct _tmpF4=({struct Cyc_String_pa_PrintArg_struct _tmp1B7;_tmp1B7.tag=0U,_tmp1B7.f1=(struct _fat_ptr)((struct _fat_ptr)v);_tmp1B7;});void*_tmpF2[1U];_tmpF2[0]=& _tmpF4;({unsigned _tmp25A=loc;struct _fat_ptr*_tmp259=msg;struct _fat_ptr _tmp258=({const char*_tmpF3="enum %s declarations: number of fields differs";_tag_fat(_tmpF3,sizeof(char),47U);});Cyc_Tcdecl_merr(_tmp25A,_tmp259,_tmp258,_tag_fat(_tmpF2,sizeof(void*),1U));});});
res=0;}
# 532
d2=d0;
# 537
goto _LL3;}}}_LL3:;}
# 540
if(!res)return 0;if((int)d2->sc == (int)new_scope)
# 543
return d2;else{
# 545
d2=({struct Cyc_Absyn_Enumdecl*_tmpF5=_cycalloc(sizeof(*_tmpF5));*_tmpF5=*d2;_tmpF5;});
d2->sc=new_scope;
return d2;}}}}
# 552
static struct _tuple12 Cyc_Tcdecl_check_var_or_fn_decl(enum Cyc_Absyn_Scope sc0,void*t0,struct Cyc_Absyn_Tqual tq0,struct Cyc_List_List*atts0,enum Cyc_Absyn_Scope sc1,void*t1,struct Cyc_Absyn_Tqual tq1,struct Cyc_List_List*atts1,struct _fat_ptr t,struct _fat_ptr v,unsigned loc,struct _fat_ptr*msg){
# 555
int res=1;
# 558
struct _tuple12 _stmttmpF=({Cyc_Tcdecl_merge_scope(sc0,sc1,t,v,loc,msg,0);});int _tmpF9;enum Cyc_Absyn_Scope _tmpF8;_LL1: _tmpF8=_stmttmpF.f1;_tmpF9=_stmttmpF.f2;_LL2: {enum Cyc_Absyn_Scope new_scope=_tmpF8;int res_scope=_tmpF9;
if(!res_scope)res=0;if(!({Cyc_Tcdecl_check_type(t0,t1);})){
# 563
struct _fat_ptr s1=({Cyc_Absynpp_typ2string(t0);});
struct _fat_ptr s2=({Cyc_Absynpp_typ2string(t1);});
if(({unsigned long _tmp25E=({unsigned long _tmp25D=({unsigned long _tmp25C=({unsigned long _tmp25B=({Cyc_strlen((struct _fat_ptr)s1);});_tmp25B + ({Cyc_strlen((struct _fat_ptr)s2);});});_tmp25C + ({Cyc_strlen((struct _fat_ptr)t);});});_tmp25D + ({Cyc_strlen((struct _fat_ptr)v);});});_tmp25E + ({Cyc_strlen(({const char*_tmpFA="  was declared  instead of ";_tag_fat(_tmpFA,sizeof(char),28U);}));});})> (unsigned long)70)
# 567
({struct Cyc_String_pa_PrintArg_struct _tmpFD=({struct Cyc_String_pa_PrintArg_struct _tmp1BC;_tmp1BC.tag=0U,_tmp1BC.f1=(struct _fat_ptr)((struct _fat_ptr)t);_tmp1BC;});struct Cyc_String_pa_PrintArg_struct _tmpFE=({struct Cyc_String_pa_PrintArg_struct _tmp1BB;_tmp1BB.tag=0U,_tmp1BB.f1=(struct _fat_ptr)((struct _fat_ptr)v);_tmp1BB;});struct Cyc_String_pa_PrintArg_struct _tmpFF=({struct Cyc_String_pa_PrintArg_struct _tmp1BA;_tmp1BA.tag=0U,_tmp1BA.f1=(struct _fat_ptr)((struct _fat_ptr)s2);_tmp1BA;});struct Cyc_String_pa_PrintArg_struct _tmp100=({struct Cyc_String_pa_PrintArg_struct _tmp1B9;_tmp1B9.tag=0U,_tmp1B9.f1=(struct _fat_ptr)((struct _fat_ptr)s1);_tmp1B9;});void*_tmpFB[4U];_tmpFB[0]=& _tmpFD,_tmpFB[1]=& _tmpFE,_tmpFB[2]=& _tmpFF,_tmpFB[3]=& _tmp100;({unsigned _tmp261=loc;struct _fat_ptr*_tmp260=msg;struct _fat_ptr _tmp25F=({const char*_tmpFC="%s %s was declared \n\t%s\n instead of \n\t%s";_tag_fat(_tmpFC,sizeof(char),41U);});Cyc_Tcdecl_merr(_tmp261,_tmp260,_tmp25F,_tag_fat(_tmpFB,sizeof(void*),4U));});});else{
# 570
({struct Cyc_String_pa_PrintArg_struct _tmp103=({struct Cyc_String_pa_PrintArg_struct _tmp1C0;_tmp1C0.tag=0U,_tmp1C0.f1=(struct _fat_ptr)((struct _fat_ptr)t);_tmp1C0;});struct Cyc_String_pa_PrintArg_struct _tmp104=({struct Cyc_String_pa_PrintArg_struct _tmp1BF;_tmp1BF.tag=0U,_tmp1BF.f1=(struct _fat_ptr)((struct _fat_ptr)v);_tmp1BF;});struct Cyc_String_pa_PrintArg_struct _tmp105=({struct Cyc_String_pa_PrintArg_struct _tmp1BE;_tmp1BE.tag=0U,_tmp1BE.f1=(struct _fat_ptr)((struct _fat_ptr)s2);_tmp1BE;});struct Cyc_String_pa_PrintArg_struct _tmp106=({struct Cyc_String_pa_PrintArg_struct _tmp1BD;_tmp1BD.tag=0U,_tmp1BD.f1=(struct _fat_ptr)((struct _fat_ptr)s1);_tmp1BD;});void*_tmp101[4U];_tmp101[0]=& _tmp103,_tmp101[1]=& _tmp104,_tmp101[2]=& _tmp105,_tmp101[3]=& _tmp106;({unsigned _tmp264=loc;struct _fat_ptr*_tmp263=msg;struct _fat_ptr _tmp262=({const char*_tmp102="%s %s was declared %s instead of %s";_tag_fat(_tmp102,sizeof(char),36U);});Cyc_Tcdecl_merr(_tmp264,_tmp263,_tmp262,_tag_fat(_tmp101,sizeof(void*),4U));});});}
# 572
({Cyc_Unify_explain_failure();});
res=0;}
# 559
if(!({Cyc_Absyn_equal_tqual(tq0,tq1);})){
# 578
({struct Cyc_String_pa_PrintArg_struct _tmp109=({struct Cyc_String_pa_PrintArg_struct _tmp1C2;_tmp1C2.tag=0U,_tmp1C2.f1=(struct _fat_ptr)((struct _fat_ptr)t);_tmp1C2;});struct Cyc_String_pa_PrintArg_struct _tmp10A=({struct Cyc_String_pa_PrintArg_struct _tmp1C1;_tmp1C1.tag=0U,_tmp1C1.f1=(struct _fat_ptr)((struct _fat_ptr)v);_tmp1C1;});void*_tmp107[2U];_tmp107[0]=& _tmp109,_tmp107[1]=& _tmp10A;({unsigned _tmp267=loc;struct _fat_ptr*_tmp266=msg;struct _fat_ptr _tmp265=({const char*_tmp108="%s %s has different type qualifiers";_tag_fat(_tmp108,sizeof(char),36U);});Cyc_Tcdecl_merr(_tmp267,_tmp266,_tmp265,_tag_fat(_tmp107,sizeof(void*),2U));});});
res=0;}
# 559
if(!({Cyc_Tcutil_same_atts(atts0,atts1);})){
# 584
({struct Cyc_String_pa_PrintArg_struct _tmp10D=({struct Cyc_String_pa_PrintArg_struct _tmp1C4;_tmp1C4.tag=0U,_tmp1C4.f1=(struct _fat_ptr)((struct _fat_ptr)t);_tmp1C4;});struct Cyc_String_pa_PrintArg_struct _tmp10E=({struct Cyc_String_pa_PrintArg_struct _tmp1C3;_tmp1C3.tag=0U,_tmp1C3.f1=(struct _fat_ptr)((struct _fat_ptr)v);_tmp1C3;});void*_tmp10B[2U];_tmp10B[0]=& _tmp10D,_tmp10B[1]=& _tmp10E;({unsigned _tmp26A=loc;struct _fat_ptr*_tmp269=msg;struct _fat_ptr _tmp268=({const char*_tmp10C="%s %s has different attributes";_tag_fat(_tmp10C,sizeof(char),31U);});Cyc_Tcdecl_merr(_tmp26A,_tmp269,_tmp268,_tag_fat(_tmp10B,sizeof(void*),2U));});});
({void*_tmp10F=0U;({struct Cyc___cycFILE*_tmp26C=Cyc_stderr;struct _fat_ptr _tmp26B=({const char*_tmp110="\tprevious attributes: ";_tag_fat(_tmp110,sizeof(char),23U);});Cyc_fprintf(_tmp26C,_tmp26B,_tag_fat(_tmp10F,sizeof(void*),0U));});});
for(0;atts0 != 0;atts0=atts0->tl){
({struct Cyc_String_pa_PrintArg_struct _tmp113=({struct Cyc_String_pa_PrintArg_struct _tmp1C5;_tmp1C5.tag=0U,({struct _fat_ptr _tmp26D=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_attribute2string((void*)atts0->hd);}));_tmp1C5.f1=_tmp26D;});_tmp1C5;});void*_tmp111[1U];_tmp111[0]=& _tmp113;({struct Cyc___cycFILE*_tmp26F=Cyc_stderr;struct _fat_ptr _tmp26E=({const char*_tmp112="%s ";_tag_fat(_tmp112,sizeof(char),4U);});Cyc_fprintf(_tmp26F,_tmp26E,_tag_fat(_tmp111,sizeof(void*),1U));});});}
({void*_tmp114=0U;({struct Cyc___cycFILE*_tmp271=Cyc_stderr;struct _fat_ptr _tmp270=({const char*_tmp115="\n\tcurrent attributes: ";_tag_fat(_tmp115,sizeof(char),23U);});Cyc_fprintf(_tmp271,_tmp270,_tag_fat(_tmp114,sizeof(void*),0U));});});
for(0;atts1 != 0;atts1=atts1->tl){
({struct Cyc_String_pa_PrintArg_struct _tmp118=({struct Cyc_String_pa_PrintArg_struct _tmp1C6;_tmp1C6.tag=0U,({struct _fat_ptr _tmp272=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_attribute2string((void*)atts1->hd);}));_tmp1C6.f1=_tmp272;});_tmp1C6;});void*_tmp116[1U];_tmp116[0]=& _tmp118;({struct Cyc___cycFILE*_tmp274=Cyc_stderr;struct _fat_ptr _tmp273=({const char*_tmp117="%s ";_tag_fat(_tmp117,sizeof(char),4U);});Cyc_fprintf(_tmp274,_tmp273,_tag_fat(_tmp116,sizeof(void*),1U));});});}
({void*_tmp119=0U;({struct Cyc___cycFILE*_tmp276=Cyc_stderr;struct _fat_ptr _tmp275=({const char*_tmp11A="\n";_tag_fat(_tmp11A,sizeof(char),2U);});Cyc_fprintf(_tmp276,_tmp275,_tag_fat(_tmp119,sizeof(void*),0U));});});
res=0;}
# 559
return({struct _tuple12 _tmp1C7;_tmp1C7.f1=new_scope,_tmp1C7.f2=res;_tmp1C7;});}}
# 598
struct Cyc_Absyn_Vardecl*Cyc_Tcdecl_merge_vardecl(struct Cyc_Absyn_Vardecl*d0,struct Cyc_Absyn_Vardecl*d1,unsigned loc,struct _fat_ptr*msg){
# 600
struct _fat_ptr v=({Cyc_Absynpp_qvar2string(d0->name);});
# 603
struct _tuple12 _stmttmp10=({({enum Cyc_Absyn_Scope _tmp281=d0->sc;void*_tmp280=d0->type;struct Cyc_Absyn_Tqual _tmp27F=d0->tq;struct Cyc_List_List*_tmp27E=d0->attributes;enum Cyc_Absyn_Scope _tmp27D=d1->sc;void*_tmp27C=d1->type;struct Cyc_Absyn_Tqual _tmp27B=d1->tq;struct Cyc_List_List*_tmp27A=d1->attributes;struct _fat_ptr _tmp279=({const char*_tmp11F="variable";_tag_fat(_tmp11F,sizeof(char),9U);});struct _fat_ptr _tmp278=v;unsigned _tmp277=loc;Cyc_Tcdecl_check_var_or_fn_decl(_tmp281,_tmp280,_tmp27F,_tmp27E,_tmp27D,_tmp27C,_tmp27B,_tmp27A,_tmp279,_tmp278,_tmp277,msg);});});int _tmp11D;enum Cyc_Absyn_Scope _tmp11C;_LL1: _tmp11C=_stmttmp10.f1;_tmp11D=_stmttmp10.f2;_LL2: {enum Cyc_Absyn_Scope new_scope=_tmp11C;int res=_tmp11D;
# 607
if(!res)return 0;if((int)d0->sc == (int)new_scope)
# 610
return d0;else{
# 612
struct Cyc_Absyn_Vardecl*d2=({struct Cyc_Absyn_Vardecl*_tmp11E=_cycalloc(sizeof(*_tmp11E));*_tmp11E=*d0;_tmp11E;});
d2->sc=new_scope;
return d2;}}}
# 618
struct Cyc_Absyn_Typedefdecl*Cyc_Tcdecl_merge_typedefdecl(struct Cyc_Absyn_Typedefdecl*d0,struct Cyc_Absyn_Typedefdecl*d1,unsigned loc,struct _fat_ptr*msg){
# 620
struct _fat_ptr v=({Cyc_Absynpp_qvar2string(d0->name);});
# 623
if(!({({struct Cyc_List_List*_tmp286=d0->tvs;struct Cyc_List_List*_tmp285=d1->tvs;struct _fat_ptr _tmp284=({const char*_tmp121="typedef";_tag_fat(_tmp121,sizeof(char),8U);});struct _fat_ptr _tmp283=v;unsigned _tmp282=loc;Cyc_Tcdecl_check_tvs(_tmp286,_tmp285,_tmp284,_tmp283,_tmp282,msg);});}))return 0;{struct Cyc_List_List*inst=({Cyc_Tcdecl_build_tvs_map(d0->tvs,d1->tvs);});
# 629
if(d0->defn != 0 && d1->defn != 0){
void*subst_defn1=({Cyc_Tcutil_substitute(inst,(void*)_check_null(d1->defn));});
if(!({Cyc_Tcdecl_check_type((void*)_check_null(d0->defn),subst_defn1);})){
({struct Cyc_String_pa_PrintArg_struct _tmp124=({struct Cyc_String_pa_PrintArg_struct _tmp1CA;_tmp1CA.tag=0U,_tmp1CA.f1=(struct _fat_ptr)((struct _fat_ptr)v);_tmp1CA;});struct Cyc_String_pa_PrintArg_struct _tmp125=({struct Cyc_String_pa_PrintArg_struct _tmp1C9;_tmp1C9.tag=0U,({
struct _fat_ptr _tmp287=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(subst_defn1);}));_tmp1C9.f1=_tmp287;});_tmp1C9;});struct Cyc_String_pa_PrintArg_struct _tmp126=({struct Cyc_String_pa_PrintArg_struct _tmp1C8;_tmp1C8.tag=0U,({struct _fat_ptr _tmp288=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string((void*)_check_null(d0->defn));}));_tmp1C8.f1=_tmp288;});_tmp1C8;});void*_tmp122[3U];_tmp122[0]=& _tmp124,_tmp122[1]=& _tmp125,_tmp122[2]=& _tmp126;({unsigned _tmp28B=loc;struct _fat_ptr*_tmp28A=msg;struct _fat_ptr _tmp289=({const char*_tmp123="typedef %s does not refer to the same type: %s != %s";_tag_fat(_tmp123,sizeof(char),53U);});Cyc_Tcdecl_merr(_tmp28B,_tmp28A,_tmp289,_tag_fat(_tmp122,sizeof(void*),3U));});});
return 0;}}
# 629
return d0;}}
# 640
void*Cyc_Tcdecl_merge_binding(void*b0,void*b1,unsigned loc,struct _fat_ptr*msg){
# 642
struct _tuple16 _stmttmp11=({struct _tuple16 _tmp1CC;_tmp1CC.f1=b0,_tmp1CC.f2=b1;_tmp1CC;});struct _tuple16 _tmp128=_stmttmp11;struct Cyc_Absyn_Vardecl*_tmp12A;struct Cyc_Absyn_Fndecl*_tmp129;struct Cyc_Absyn_Fndecl*_tmp12B;struct Cyc_Absyn_Fndecl*_tmp12D;struct Cyc_Absyn_Vardecl*_tmp12C;struct Cyc_Absyn_Vardecl*_tmp12F;struct Cyc_Absyn_Vardecl*_tmp12E;switch(*((int*)_tmp128.f1)){case 1U: switch(*((int*)_tmp128.f2)){case 1U: _LL1: _tmp12E=((struct Cyc_Absyn_Global_b_Absyn_Binding_struct*)_tmp128.f1)->f1;_tmp12F=((struct Cyc_Absyn_Global_b_Absyn_Binding_struct*)_tmp128.f2)->f1;_LL2: {struct Cyc_Absyn_Vardecl*d0=_tmp12E;struct Cyc_Absyn_Vardecl*d1=_tmp12F;
# 644
struct Cyc_Absyn_Vardecl*d2=({Cyc_Tcdecl_merge_vardecl(d0,d1,loc,msg);});
if(d2 == 0)return 0;if(d2 == d0)
return b0;
# 645
if(d2 == d1)
# 647
return b1;
# 645
return(void*)({struct Cyc_Absyn_Global_b_Absyn_Binding_struct*_tmp130=_cycalloc(sizeof(*_tmp130));
# 648
_tmp130->tag=1U,_tmp130->f1=d2;_tmp130;});}case 2U: _LL3: _tmp12C=((struct Cyc_Absyn_Global_b_Absyn_Binding_struct*)_tmp128.f1)->f1;_tmp12D=((struct Cyc_Absyn_Funname_b_Absyn_Binding_struct*)_tmp128.f2)->f1;_LL4: {struct Cyc_Absyn_Vardecl*d0=_tmp12C;struct Cyc_Absyn_Fndecl*d1=_tmp12D;
# 650
struct _tuple12 _stmttmp12=({struct _tuple12(*_tmp132)(enum Cyc_Absyn_Scope sc0,void*t0,struct Cyc_Absyn_Tqual tq0,struct Cyc_List_List*atts0,enum Cyc_Absyn_Scope sc1,void*t1,struct Cyc_Absyn_Tqual tq1,struct Cyc_List_List*atts1,struct _fat_ptr t,struct _fat_ptr v,unsigned loc,struct _fat_ptr*msg)=Cyc_Tcdecl_check_var_or_fn_decl;enum Cyc_Absyn_Scope _tmp133=d0->sc;void*_tmp134=d0->type;struct Cyc_Absyn_Tqual _tmp135=d0->tq;struct Cyc_List_List*_tmp136=d0->attributes;enum Cyc_Absyn_Scope _tmp137=d1->sc;void*_tmp138=(void*)_check_null(d1->cached_type);struct Cyc_Absyn_Tqual _tmp139=({Cyc_Absyn_empty_tqual(0U);});struct Cyc_List_List*_tmp13A=(d1->i).attributes;struct _fat_ptr _tmp13B=({const char*_tmp13C="function";_tag_fat(_tmp13C,sizeof(char),9U);});struct _fat_ptr _tmp13D=({Cyc_Absynpp_qvar2string(d0->name);});unsigned _tmp13E=loc;struct _fat_ptr*_tmp13F=msg;_tmp132(_tmp133,_tmp134,_tmp135,_tmp136,_tmp137,_tmp138,_tmp139,_tmp13A,_tmp13B,_tmp13D,_tmp13E,_tmp13F);});int _tmp131;_LLC: _tmp131=_stmttmp12.f2;_LLD: {int res=_tmp131;
# 654
if(res)return b1;return 0;}}default: goto _LL9;}case 2U: switch(*((int*)_tmp128.f2)){case 2U: _LL5: _tmp12B=((struct Cyc_Absyn_Funname_b_Absyn_Binding_struct*)_tmp128.f2)->f1;_LL6: {struct Cyc_Absyn_Fndecl*d1=_tmp12B;
# 657
({struct Cyc_String_pa_PrintArg_struct _tmp142=({struct Cyc_String_pa_PrintArg_struct _tmp1CB;_tmp1CB.tag=0U,({struct _fat_ptr _tmp28C=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(d1->name);}));_tmp1CB.f1=_tmp28C;});_tmp1CB;});void*_tmp140[1U];_tmp140[0]=& _tmp142;({unsigned _tmp28F=loc;struct _fat_ptr*_tmp28E=msg;struct _fat_ptr _tmp28D=({const char*_tmp141="redefinition of function %s";_tag_fat(_tmp141,sizeof(char),28U);});Cyc_Tcdecl_merr(_tmp28F,_tmp28E,_tmp28D,_tag_fat(_tmp140,sizeof(void*),1U));});});
return 0;}case 1U: _LL7: _tmp129=((struct Cyc_Absyn_Funname_b_Absyn_Binding_struct*)_tmp128.f1)->f1;_tmp12A=((struct Cyc_Absyn_Global_b_Absyn_Binding_struct*)_tmp128.f2)->f1;_LL8: {struct Cyc_Absyn_Fndecl*d0=_tmp129;struct Cyc_Absyn_Vardecl*d1=_tmp12A;
# 660
struct _tuple12 _stmttmp13=({struct _tuple12(*_tmp144)(enum Cyc_Absyn_Scope sc0,void*t0,struct Cyc_Absyn_Tqual tq0,struct Cyc_List_List*atts0,enum Cyc_Absyn_Scope sc1,void*t1,struct Cyc_Absyn_Tqual tq1,struct Cyc_List_List*atts1,struct _fat_ptr t,struct _fat_ptr v,unsigned loc,struct _fat_ptr*msg)=Cyc_Tcdecl_check_var_or_fn_decl;enum Cyc_Absyn_Scope _tmp145=d0->sc;void*_tmp146=(void*)_check_null(d0->cached_type);struct Cyc_Absyn_Tqual _tmp147=({Cyc_Absyn_empty_tqual(0U);});struct Cyc_List_List*_tmp148=(d0->i).attributes;enum Cyc_Absyn_Scope _tmp149=d1->sc;void*_tmp14A=d1->type;struct Cyc_Absyn_Tqual _tmp14B=d1->tq;struct Cyc_List_List*_tmp14C=d1->attributes;struct _fat_ptr _tmp14D=({const char*_tmp14E="variable";_tag_fat(_tmp14E,sizeof(char),9U);});struct _fat_ptr _tmp14F=({Cyc_Absynpp_qvar2string(d0->name);});unsigned _tmp150=loc;struct _fat_ptr*_tmp151=msg;_tmp144(_tmp145,_tmp146,_tmp147,_tmp148,_tmp149,_tmp14A,_tmp14B,_tmp14C,_tmp14D,_tmp14F,_tmp150,_tmp151);});int _tmp143;_LLF: _tmp143=_stmttmp13.f2;_LL10: {int res=_tmp143;
# 664
if(res)return b0;return 0;}}default: goto _LL9;}default: _LL9: _LLA:
# 666
 return 0;}_LL0:;}
# 670
struct Cyc_Tcdecl_Xdatatypefielddecl*Cyc_Tcdecl_merge_xdatatypefielddecl(struct Cyc_Tcdecl_Xdatatypefielddecl*d0,struct Cyc_Tcdecl_Xdatatypefielddecl*d1,unsigned loc,struct _fat_ptr*msg){
# 673
struct Cyc_Absyn_Datatypefield*_tmp154;struct Cyc_Absyn_Datatypedecl*_tmp153;_LL1: _tmp153=d0->base;_tmp154=d0->field;_LL2: {struct Cyc_Absyn_Datatypedecl*b0=_tmp153;struct Cyc_Absyn_Datatypefield*f0=_tmp154;
struct Cyc_Absyn_Datatypefield*_tmp156;struct Cyc_Absyn_Datatypedecl*_tmp155;_LL4: _tmp155=d1->base;_tmp156=d1->field;_LL5: {struct Cyc_Absyn_Datatypedecl*b1=_tmp155;struct Cyc_Absyn_Datatypefield*f1=_tmp156;
struct _fat_ptr v=({Cyc_Absynpp_qvar2string(f0->name);});
# 678
if(({Cyc_Absyn_qvar_cmp(b0->name,b1->name);})!= 0){
({struct Cyc_String_pa_PrintArg_struct _tmp159=({struct Cyc_String_pa_PrintArg_struct _tmp1CF;_tmp1CF.tag=0U,({
struct _fat_ptr _tmp290=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(b0->name);}));_tmp1CF.f1=_tmp290;});_tmp1CF;});struct Cyc_String_pa_PrintArg_struct _tmp15A=({struct Cyc_String_pa_PrintArg_struct _tmp1CE;_tmp1CE.tag=0U,({struct _fat_ptr _tmp291=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(b1->name);}));_tmp1CE.f1=_tmp291;});_tmp1CE;});struct Cyc_String_pa_PrintArg_struct _tmp15B=({struct Cyc_String_pa_PrintArg_struct _tmp1CD;_tmp1CD.tag=0U,_tmp1CD.f1=(struct _fat_ptr)((struct _fat_ptr)v);_tmp1CD;});void*_tmp157[3U];_tmp157[0]=& _tmp159,_tmp157[1]=& _tmp15A,_tmp157[2]=& _tmp15B;({unsigned _tmp294=loc;struct _fat_ptr*_tmp293=msg;struct _fat_ptr _tmp292=({const char*_tmp158="@extensible datatype %s and %s have both a field named %s";_tag_fat(_tmp158,sizeof(char),58U);});Cyc_Tcdecl_merr(_tmp294,_tmp293,_tmp292,_tag_fat(_tmp157,sizeof(void*),3U));});});
return 0;}
# 678
if(!({int(*_tmp15C)(struct Cyc_List_List*tvs0,struct Cyc_List_List*tvs1,struct _fat_ptr t,struct _fat_ptr v,unsigned loc,struct _fat_ptr*msg)=Cyc_Tcdecl_check_tvs;struct Cyc_List_List*_tmp15D=b0->tvs;struct Cyc_List_List*_tmp15E=b1->tvs;struct _fat_ptr _tmp15F=({const char*_tmp160="@extensible datatype";_tag_fat(_tmp160,sizeof(char),21U);});struct _fat_ptr _tmp161=({Cyc_Absynpp_qvar2string(b0->name);});unsigned _tmp162=loc;struct _fat_ptr*_tmp163=msg;_tmp15C(_tmp15D,_tmp15E,_tmp15F,_tmp161,_tmp162,_tmp163);}))
# 686
return 0;{
# 678
struct Cyc_List_List*inst1=({Cyc_Tcdecl_build_tvs_map(b0->tvs,b1->tvs);});
# 691
struct Cyc_Absyn_Datatypefield*f2=({({struct Cyc_Absyn_Datatypefield*_tmp299=f0;struct Cyc_Absyn_Datatypefield*_tmp298=f1;struct Cyc_List_List*_tmp297=inst1;struct _fat_ptr _tmp296=({const char*_tmp165="@extensible datatype field";_tag_fat(_tmp165,sizeof(char),27U);});struct _fat_ptr _tmp295=v;Cyc_Tcdecl_merge_datatypefield(_tmp299,_tmp298,_tmp297,_tmp296,_tmp295,msg);});});
if(f2 == 0)return 0;if(f2 == f0)
return d0;
# 692
return({struct Cyc_Tcdecl_Xdatatypefielddecl*_tmp164=_cycalloc(sizeof(*_tmp164));
# 694
_tmp164->base=b0,_tmp164->field=f2;_tmp164;});}}}}

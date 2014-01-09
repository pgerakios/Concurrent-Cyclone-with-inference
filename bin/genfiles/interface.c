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
 struct Cyc___cycFILE;struct Cyc_String_pa_PrintArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Int_pa_PrintArg_struct{int tag;unsigned long f1;};struct Cyc_Double_pa_PrintArg_struct{int tag;double f1;};struct Cyc_LongDouble_pa_PrintArg_struct{int tag;long double f1;};struct Cyc_ShortPtr_pa_PrintArg_struct{int tag;short*f1;};struct Cyc_IntPtr_pa_PrintArg_struct{int tag;unsigned long*f1;};
# 73 "cycboot.h"
struct _fat_ptr Cyc_aprintf(struct _fat_ptr,struct _fat_ptr);
# 100
int Cyc_fprintf(struct Cyc___cycFILE*,struct _fat_ptr,struct _fat_ptr);struct Cyc_ShortPtr_sa_ScanfArg_struct{int tag;short*f1;};struct Cyc_UShortPtr_sa_ScanfArg_struct{int tag;unsigned short*f1;};struct Cyc_IntPtr_sa_ScanfArg_struct{int tag;int*f1;};struct Cyc_UIntPtr_sa_ScanfArg_struct{int tag;unsigned*f1;};struct Cyc_StringPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_DoublePtr_sa_ScanfArg_struct{int tag;double*f1;};struct Cyc_FloatPtr_sa_ScanfArg_struct{int tag;float*f1;};struct Cyc_CharPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};extern char Cyc_FileCloseError[15U];struct Cyc_FileCloseError_exn_struct{char*tag;};extern char Cyc_FileOpenError[14U];struct Cyc_FileOpenError_exn_struct{char*tag;struct _fat_ptr f1;};struct Cyc_Core_Opt{void*v;};
# 117 "core.h"
void*Cyc_Core_identity(void*);extern char Cyc_Core_Invalid_argument[17U];struct Cyc_Core_Invalid_argument_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Failure[8U];struct Cyc_Core_Failure_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Impossible[11U];struct Cyc_Core_Impossible_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Not_found[10U];struct Cyc_Core_Not_found_exn_struct{char*tag;};extern char Cyc_Core_Unreachable[12U];struct Cyc_Core_Unreachable_exn_struct{char*tag;struct _fat_ptr f1;};
# 168
extern struct _RegionHandle*Cyc_Core_unique_region;struct Cyc_Core_DynamicRegion;struct Cyc_Core_NewDynamicRegion{struct Cyc_Core_DynamicRegion*key;};struct Cyc_Core_ThinRes{void*arr;unsigned nelts;};struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};
# 76 "list.h"
struct Cyc_List_List*Cyc_List_map(void*(*f)(void*),struct Cyc_List_List*x);extern char Cyc_List_List_mismatch[14U];struct Cyc_List_List_mismatch_exn_struct{char*tag;};
# 145
void*Cyc_List_fold_left(void*(*f)(void*,void*),void*accum,struct Cyc_List_List*x);
# 172
struct Cyc_List_List*Cyc_List_rev(struct Cyc_List_List*x);
# 178
struct Cyc_List_List*Cyc_List_imp_rev(struct Cyc_List_List*x);
# 184
struct Cyc_List_List*Cyc_List_append(struct Cyc_List_List*x,struct Cyc_List_List*y);extern char Cyc_List_Nth[4U];struct Cyc_List_Nth_exn_struct{char*tag;};struct Cyc_Iter_Iter{void*env;int(*next)(void*env,void*dest);};struct Cyc_Dict_T;struct Cyc_Dict_Dict{int(*rel)(void*,void*);struct _RegionHandle*r;const struct Cyc_Dict_T*t;};extern char Cyc_Dict_Present[8U];struct Cyc_Dict_Present_exn_struct{char*tag;};extern char Cyc_Dict_Absent[7U];struct Cyc_Dict_Absent_exn_struct{char*tag;};
# 62 "dict.h"
struct Cyc_Dict_Dict Cyc_Dict_empty(int(*cmp)(void*,void*));
# 83
int Cyc_Dict_member(struct Cyc_Dict_Dict d,void*k);
# 87
struct Cyc_Dict_Dict Cyc_Dict_insert(struct Cyc_Dict_Dict d,void*k,void*v);
# 110
void*Cyc_Dict_lookup(struct Cyc_Dict_Dict d,void*k);
# 131 "dict.h"
void*Cyc_Dict_fold(void*(*f)(void*,void*,void*),struct Cyc_Dict_Dict d,void*accum);
# 149
void Cyc_Dict_iter_c(void(*f)(void*,void*,void*),void*env,struct Cyc_Dict_Dict d);struct Cyc_Position_Error;struct Cyc_Relations_Reln;struct _union_Nmspace_Rel_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Abs_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_C_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Loc_n{int tag;int val;};union Cyc_Absyn_Nmspace{struct _union_Nmspace_Rel_n Rel_n;struct _union_Nmspace_Abs_n Abs_n;struct _union_Nmspace_C_n C_n;struct _union_Nmspace_Loc_n Loc_n;};
# 111 "absyn.h"
union Cyc_Absyn_Nmspace Cyc_Absyn_Rel_n(struct Cyc_List_List*);struct _tuple0{union Cyc_Absyn_Nmspace f1;struct _fat_ptr*f2;};
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
# 1012
extern struct _tuple0*Cyc_Absyn_exn_name;
struct Cyc_Absyn_Datatypedecl*Cyc_Absyn_exn_tud();
# 1153
struct Cyc_Absyn_Decl*Cyc_Absyn_new_decl(void*,unsigned);
# 1158
struct Cyc_Absyn_Vardecl*Cyc_Absyn_new_vardecl(unsigned varloc,struct _tuple0*,void*,struct Cyc_Absyn_Exp*init);
# 1225
struct Cyc_Absyn_Aggrdecl*Cyc_Absyn_get_known_aggrdecl(union Cyc_Absyn_AggrInfo);struct Cyc_RgnOrder_RgnPO;extern char Cyc_Tcenv_Env_error[10U];struct Cyc_Tcenv_Env_error_exn_struct{char*tag;};struct Cyc_Tcenv_Genv{struct Cyc_Dict_Dict aggrdecls;struct Cyc_Dict_Dict datatypedecls;struct Cyc_Dict_Dict enumdecls;struct Cyc_Dict_Dict typedefs;struct Cyc_Dict_Dict ordinaries;};struct Cyc_Tcenv_Fenv;struct Cyc_Tcenv_Tenv{struct Cyc_List_List*ns;struct Cyc_Tcenv_Genv*ae;struct Cyc_Tcenv_Fenv*le;int allow_valueof: 1;int in_extern_c_include: 1;int in_tempest: 1;int tempest_generalize: 1;};
# 68 "tcenv.h"
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_tc_init();
# 90
enum Cyc_Tcenv_NewStatus{Cyc_Tcenv_NoneNew =0U,Cyc_Tcenv_InNew =1U,Cyc_Tcenv_InNewAggr =2U};struct Cyc_Interface_I;
# 36 "interface.h"
struct Cyc_Interface_I*Cyc_Interface_empty();struct _tuple12{struct _fat_ptr f1;struct _fat_ptr f2;};
# 66 "interface.h"
struct Cyc_Interface_I*Cyc_Interface_merge(struct Cyc_Interface_I*i1,struct Cyc_Interface_I*i2,struct _tuple12*info);
# 75
void Cyc_Interface_print(struct Cyc_Interface_I*,struct Cyc___cycFILE*);
# 78
struct Cyc_Interface_I*Cyc_Interface_parse(struct Cyc___cycFILE*);
# 50 "string.h"
int Cyc_strptrcmp(struct _fat_ptr*s1,struct _fat_ptr*s2);struct Cyc_PP_Ppstate;struct Cyc_PP_Out;struct Cyc_PP_Doc;struct Cyc_Absynpp_Params{int expand_typedefs;int qvar_to_Cids;int add_cyc_prefix;int to_VC;int decls_first;int rewrite_temp_tvars;int print_all_tvars;int print_all_kinds;int print_all_effects;int print_using_stmts;int print_externC_stmts;int print_full_evars;int print_zeroterm;int generate_line_directives;int use_curr_namespace;struct Cyc_List_List*curr_namespace;};
# 53 "absynpp.h"
void Cyc_Absynpp_set_params(struct Cyc_Absynpp_Params*fs);
# 55
extern struct Cyc_Absynpp_Params Cyc_Absynpp_cyci_params_r;
# 57
void Cyc_Absynpp_decllist2file(struct Cyc_List_List*tdl,struct Cyc___cycFILE*f);
# 69
struct _fat_ptr Cyc_Absynpp_qvar2string(struct _tuple0*);
# 33 "tc.h"
void Cyc_Tc_tc(struct Cyc_Tcenv_Tenv*te,int var_default_init,struct Cyc_List_List*ds,int);extern char Cyc_Lexing_Error[6U];struct Cyc_Lexing_Error_exn_struct{char*tag;struct _fat_ptr f1;};struct Cyc_Lexing_lexbuf{void(*refill_buff)(struct Cyc_Lexing_lexbuf*);void*refill_state;struct _fat_ptr lex_buffer;int lex_buffer_len;int lex_abs_pos;int lex_start_pos;int lex_curr_pos;int lex_last_pos;int lex_last_action;int lex_eof_reached;};struct Cyc_Lexing_function_lexbuf_state{int(*read_fun)(struct _fat_ptr,int,void*);void*read_fun_state;};struct Cyc_Lexing_lex_tables{struct _fat_ptr lex_base;struct _fat_ptr lex_backtrk;struct _fat_ptr lex_default;struct _fat_ptr lex_trans;struct _fat_ptr lex_check;};
# 28 "parse.h"
struct Cyc_List_List*Cyc_Parse_parse_file(struct Cyc___cycFILE*f,struct _fat_ptr name);extern char Cyc_Parse_Exit[5U];struct Cyc_Parse_Exit_exn_struct{char*tag;};struct Cyc_FlatList{struct Cyc_FlatList*tl;void*hd[0U] __attribute__((aligned )) ;};struct Cyc_Type_specifier{int Signed_spec: 1;int Unsigned_spec: 1;int Short_spec: 1;int Long_spec: 1;int Long_Long_spec: 1;int Valid_type_spec: 1;void*Type_spec;unsigned loc;};struct Cyc_Declarator{struct _tuple0*id;struct Cyc_List_List*tms;};struct _tuple14{struct Cyc_Declarator f1;struct Cyc_Absyn_Exp*f2;};struct _tuple13{struct _tuple13*tl;struct _tuple14 hd  __attribute__((aligned )) ;};
# 52
enum Cyc_Storage_class{Cyc_Typedef_sc =0U,Cyc_Extern_sc =1U,Cyc_ExternC_sc =2U,Cyc_Static_sc =3U,Cyc_Auto_sc =4U,Cyc_Register_sc =5U,Cyc_Abstract_sc =6U};struct Cyc_Declaration_spec{enum Cyc_Storage_class*sc;struct Cyc_Absyn_Tqual tq;struct Cyc_Type_specifier type_specs;int is_inline;struct Cyc_List_List*attributes;};struct Cyc_Abstractdeclarator{struct Cyc_List_List*tms;};struct Cyc_Numelts_ptrqual_Pointer_qual_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Region_ptrqual_Pointer_qual_struct{int tag;void*f1;};struct Cyc_Thin_ptrqual_Pointer_qual_struct{int tag;};struct Cyc_Fat_ptrqual_Pointer_qual_struct{int tag;};struct Cyc_Zeroterm_ptrqual_Pointer_qual_struct{int tag;};struct Cyc_Nozeroterm_ptrqual_Pointer_qual_struct{int tag;};struct Cyc_Notnull_ptrqual_Pointer_qual_struct{int tag;};struct Cyc_Nullable_ptrqual_Pointer_qual_struct{int tag;};struct _union_YYSTYPE_Int_tok{int tag;union Cyc_Absyn_Cnst val;};struct _union_YYSTYPE_Char_tok{int tag;char val;};struct _union_YYSTYPE_String_tok{int tag;struct _fat_ptr val;};struct _union_YYSTYPE_Stringopt_tok{int tag;struct Cyc_Core_Opt*val;};struct _union_YYSTYPE_QualId_tok{int tag;struct _tuple0*val;};struct _tuple15{int f1;struct _fat_ptr f2;};struct _union_YYSTYPE_Asm_tok{int tag;struct _tuple15 val;};struct _union_YYSTYPE_Exp_tok{int tag;struct Cyc_Absyn_Exp*val;};struct _union_YYSTYPE_Stmt_tok{int tag;struct Cyc_Absyn_Stmt*val;};struct _tuple16{unsigned f1;void*f2;void*f3;};struct _union_YYSTYPE_YY1{int tag;struct _tuple16*val;};struct _union_YYSTYPE_YY2{int tag;void*val;};struct _union_YYSTYPE_YY3{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY4{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY5{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY6{int tag;enum Cyc_Absyn_Primop val;};struct _union_YYSTYPE_YY7{int tag;struct Cyc_Core_Opt*val;};struct _union_YYSTYPE_YY8{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY9{int tag;struct Cyc_Absyn_Pat*val;};struct _tuple17{struct Cyc_List_List*f1;int f2;};struct _union_YYSTYPE_YY10{int tag;struct _tuple17*val;};struct _union_YYSTYPE_YY11{int tag;struct Cyc_List_List*val;};struct _tuple18{struct Cyc_List_List*f1;struct Cyc_Absyn_Pat*f2;};struct _union_YYSTYPE_YY12{int tag;struct _tuple18*val;};struct _union_YYSTYPE_YY13{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY14{int tag;struct _tuple17*val;};struct _union_YYSTYPE_YY15{int tag;struct Cyc_Absyn_Fndecl*val;};struct _union_YYSTYPE_YY16{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY17{int tag;struct Cyc_Declaration_spec val;};struct _union_YYSTYPE_YY18{int tag;struct _tuple14 val;};struct _union_YYSTYPE_YY19{int tag;struct _tuple13*val;};struct _union_YYSTYPE_YY20{int tag;enum Cyc_Storage_class*val;};struct _union_YYSTYPE_YY21{int tag;struct Cyc_Type_specifier val;};struct _union_YYSTYPE_YY22{int tag;enum Cyc_Absyn_AggrKind val;};struct _union_YYSTYPE_YY23{int tag;struct Cyc_Absyn_Tqual val;};struct _union_YYSTYPE_YY24{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY25{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY26{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY27{int tag;struct Cyc_Declarator val;};struct _tuple19{struct Cyc_Declarator f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;};struct _union_YYSTYPE_YY28{int tag;struct _tuple19*val;};struct _union_YYSTYPE_YY29{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY30{int tag;struct Cyc_Abstractdeclarator val;};struct _union_YYSTYPE_YY31{int tag;int val;};struct _union_YYSTYPE_YY32{int tag;enum Cyc_Absyn_Scope val;};struct _union_YYSTYPE_YY33{int tag;struct Cyc_Absyn_Datatypefield*val;};struct _union_YYSTYPE_YY34{int tag;struct Cyc_List_List*val;};struct _tuple20{struct Cyc_Absyn_Tqual f1;struct Cyc_Type_specifier f2;struct Cyc_List_List*f3;};struct _union_YYSTYPE_YY35{int tag;struct _tuple20 val;};struct _union_YYSTYPE_YY36{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY37{int tag;struct _tuple9*val;};struct _union_YYSTYPE_YY38{int tag;struct Cyc_List_List*val;};struct _tuple21{struct Cyc_List_List*f1;int f2;struct Cyc_Absyn_VarargInfo*f3;void*f4;struct Cyc_List_List*f5;};struct _union_YYSTYPE_YY39{int tag;struct _tuple21*val;};struct _union_YYSTYPE_YY40{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY41{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY42{int tag;void*val;};struct _union_YYSTYPE_YY43{int tag;struct Cyc_Absyn_Kind*val;};struct _union_YYSTYPE_YY44{int tag;void*val;};struct _union_YYSTYPE_YY45{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY46{int tag;void*val;};struct _union_YYSTYPE_YY47{int tag;struct Cyc_Absyn_Enumfield*val;};struct _union_YYSTYPE_YY48{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY49{int tag;void*val;};struct _union_YYSTYPE_YY50{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY51{int tag;void*val;};struct _union_YYSTYPE_YY52{int tag;struct Cyc_List_List*val;};struct _tuple22{struct Cyc_List_List*f1;unsigned f2;};struct _union_YYSTYPE_YY53{int tag;struct _tuple22*val;};struct _union_YYSTYPE_YY54{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY55{int tag;void*val;};struct _union_YYSTYPE_YY56{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY57{int tag;struct Cyc_Absyn_Exp*val;};struct _union_YYSTYPE_YY58{int tag;void*val;};struct _tuple23{struct Cyc_List_List*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;};struct _union_YYSTYPE_YY59{int tag;struct _tuple23*val;};struct _union_YYSTYPE_YY60{int tag;struct _tuple8*val;};struct _union_YYSTYPE_YY61{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY62{int tag;struct Cyc_List_List*val;};struct _tuple24{struct _fat_ptr f1;struct Cyc_Absyn_Exp*f2;};struct _union_YYSTYPE_YY63{int tag;struct _tuple24*val;};struct _union_YYSTYPE_YY64{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY65{int tag;int val;};struct _union_YYSTYPE_YY66{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YYINITIALSVAL{int tag;int val;};union Cyc_YYSTYPE{struct _union_YYSTYPE_Int_tok Int_tok;struct _union_YYSTYPE_Char_tok Char_tok;struct _union_YYSTYPE_String_tok String_tok;struct _union_YYSTYPE_Stringopt_tok Stringopt_tok;struct _union_YYSTYPE_QualId_tok QualId_tok;struct _union_YYSTYPE_Asm_tok Asm_tok;struct _union_YYSTYPE_Exp_tok Exp_tok;struct _union_YYSTYPE_Stmt_tok Stmt_tok;struct _union_YYSTYPE_YY1 YY1;struct _union_YYSTYPE_YY2 YY2;struct _union_YYSTYPE_YY3 YY3;struct _union_YYSTYPE_YY4 YY4;struct _union_YYSTYPE_YY5 YY5;struct _union_YYSTYPE_YY6 YY6;struct _union_YYSTYPE_YY7 YY7;struct _union_YYSTYPE_YY8 YY8;struct _union_YYSTYPE_YY9 YY9;struct _union_YYSTYPE_YY10 YY10;struct _union_YYSTYPE_YY11 YY11;struct _union_YYSTYPE_YY12 YY12;struct _union_YYSTYPE_YY13 YY13;struct _union_YYSTYPE_YY14 YY14;struct _union_YYSTYPE_YY15 YY15;struct _union_YYSTYPE_YY16 YY16;struct _union_YYSTYPE_YY17 YY17;struct _union_YYSTYPE_YY18 YY18;struct _union_YYSTYPE_YY19 YY19;struct _union_YYSTYPE_YY20 YY20;struct _union_YYSTYPE_YY21 YY21;struct _union_YYSTYPE_YY22 YY22;struct _union_YYSTYPE_YY23 YY23;struct _union_YYSTYPE_YY24 YY24;struct _union_YYSTYPE_YY25 YY25;struct _union_YYSTYPE_YY26 YY26;struct _union_YYSTYPE_YY27 YY27;struct _union_YYSTYPE_YY28 YY28;struct _union_YYSTYPE_YY29 YY29;struct _union_YYSTYPE_YY30 YY30;struct _union_YYSTYPE_YY31 YY31;struct _union_YYSTYPE_YY32 YY32;struct _union_YYSTYPE_YY33 YY33;struct _union_YYSTYPE_YY34 YY34;struct _union_YYSTYPE_YY35 YY35;struct _union_YYSTYPE_YY36 YY36;struct _union_YYSTYPE_YY37 YY37;struct _union_YYSTYPE_YY38 YY38;struct _union_YYSTYPE_YY39 YY39;struct _union_YYSTYPE_YY40 YY40;struct _union_YYSTYPE_YY41 YY41;struct _union_YYSTYPE_YY42 YY42;struct _union_YYSTYPE_YY43 YY43;struct _union_YYSTYPE_YY44 YY44;struct _union_YYSTYPE_YY45 YY45;struct _union_YYSTYPE_YY46 YY46;struct _union_YYSTYPE_YY47 YY47;struct _union_YYSTYPE_YY48 YY48;struct _union_YYSTYPE_YY49 YY49;struct _union_YYSTYPE_YY50 YY50;struct _union_YYSTYPE_YY51 YY51;struct _union_YYSTYPE_YY52 YY52;struct _union_YYSTYPE_YY53 YY53;struct _union_YYSTYPE_YY54 YY54;struct _union_YYSTYPE_YY55 YY55;struct _union_YYSTYPE_YY56 YY56;struct _union_YYSTYPE_YY57 YY57;struct _union_YYSTYPE_YY58 YY58;struct _union_YYSTYPE_YY59 YY59;struct _union_YYSTYPE_YY60 YY60;struct _union_YYSTYPE_YY61 YY61;struct _union_YYSTYPE_YY62 YY62;struct _union_YYSTYPE_YY63 YY63;struct _union_YYSTYPE_YY64 YY64;struct _union_YYSTYPE_YY65 YY65;struct _union_YYSTYPE_YY66 YY66;struct _union_YYSTYPE_YYINITIALSVAL YYINITIALSVAL;};struct Cyc_Yyltype{int timestamp;int first_line;int first_column;int last_line;int last_column;};struct _union_RelnOp_RConst{int tag;unsigned val;};struct _union_RelnOp_RVar{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RNumelts{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RType{int tag;void*val;};struct _union_RelnOp_RParam{int tag;unsigned val;};struct _union_RelnOp_RParamNumelts{int tag;unsigned val;};struct _union_RelnOp_RReturn{int tag;unsigned val;};union Cyc_Relations_RelnOp{struct _union_RelnOp_RConst RConst;struct _union_RelnOp_RVar RVar;struct _union_RelnOp_RNumelts RNumelts;struct _union_RelnOp_RType RType;struct _union_RelnOp_RParam RParam;struct _union_RelnOp_RParamNumelts RParamNumelts;struct _union_RelnOp_RReturn RReturn;};
# 50 "relations-ap.h"
enum Cyc_Relations_Relation{Cyc_Relations_Req =0U,Cyc_Relations_Rneq =1U,Cyc_Relations_Rlte =2U,Cyc_Relations_Rlt =3U};struct Cyc_Relations_Reln{union Cyc_Relations_RelnOp rop1;enum Cyc_Relations_Relation relation;union Cyc_Relations_RelnOp rop2;};
# 31 "tcutil.h"
void Cyc_Tcutil_terr(unsigned,struct _fat_ptr,struct _fat_ptr);
# 46
int Cyc_Tcutil_is_function_type(void*);
# 110
void*Cyc_Tcutil_compress(void*);
# 141
extern struct Cyc_Absyn_Kind Cyc_Tcutil_bk;
# 178
void*Cyc_Tcutil_kind_to_bound(struct Cyc_Absyn_Kind*k);extern char Cyc_Tcdecl_Incompatible[13U];struct Cyc_Tcdecl_Incompatible_exn_struct{char*tag;};struct Cyc_Tcdecl_Xdatatypefielddecl{struct Cyc_Absyn_Datatypedecl*base;struct Cyc_Absyn_Datatypefield*field;};
# 38 "tcdecl.h"
void Cyc_Tcdecl_merr(unsigned loc,struct _fat_ptr*msg1,struct _fat_ptr fmt,struct _fat_ptr ap);
# 54 "tcdecl.h"
struct Cyc_Absyn_Aggrdecl*Cyc_Tcdecl_merge_aggrdecl(struct Cyc_Absyn_Aggrdecl*,struct Cyc_Absyn_Aggrdecl*,unsigned,struct _fat_ptr*msg);
# 57
struct Cyc_Absyn_Datatypedecl*Cyc_Tcdecl_merge_datatypedecl(struct Cyc_Absyn_Datatypedecl*,struct Cyc_Absyn_Datatypedecl*,unsigned,struct _fat_ptr*msg);
# 59
struct Cyc_Absyn_Enumdecl*Cyc_Tcdecl_merge_enumdecl(struct Cyc_Absyn_Enumdecl*,struct Cyc_Absyn_Enumdecl*,unsigned,struct _fat_ptr*msg);
# 61
struct Cyc_Absyn_Vardecl*Cyc_Tcdecl_merge_vardecl(struct Cyc_Absyn_Vardecl*,struct Cyc_Absyn_Vardecl*,unsigned,struct _fat_ptr*msg);
# 63
struct Cyc_Absyn_Typedefdecl*Cyc_Tcdecl_merge_typedefdecl(struct Cyc_Absyn_Typedefdecl*,struct Cyc_Absyn_Typedefdecl*,unsigned,struct _fat_ptr*msg);
# 68
struct Cyc_Tcdecl_Xdatatypefielddecl*Cyc_Tcdecl_merge_xdatatypefielddecl(struct Cyc_Tcdecl_Xdatatypefielddecl*,struct Cyc_Tcdecl_Xdatatypefielddecl*,unsigned,struct _fat_ptr*msg);
# 30 "binding.h"
void Cyc_Binding_resolve_all(struct Cyc_List_List*tds);struct Cyc_Binding_Namespace_Binding_NSDirective_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Binding_Using_Binding_NSDirective_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Binding_NSCtxt{struct Cyc_List_List*curr_ns;struct Cyc_List_List*availables;struct Cyc_Dict_Dict ns_data;};
# 45 "interface.cyc"
void Cyc_Lex_lex_init();struct Cyc_Interface_Ienv{struct Cyc_Dict_Dict aggrdecls;struct Cyc_Dict_Dict datatypedecls;struct Cyc_Dict_Dict enumdecls;struct Cyc_Dict_Dict typedefdecls;struct Cyc_Dict_Dict vardecls;struct Cyc_Dict_Dict xdatatypefielddecls;};struct Cyc_Interface_I{struct Cyc_Interface_Ienv*imports;struct Cyc_Interface_Ienv*exports;struct Cyc_List_List*tds;};
# 73
static struct Cyc_Interface_Ienv*Cyc_Interface_new_ienv(){
return({struct Cyc_Interface_Ienv*_tmp6=_cycalloc(sizeof(*_tmp6));({
struct Cyc_Dict_Dict _tmp2DC=({({struct Cyc_Dict_Dict(*_tmp0)(int(*cmp)(struct _tuple0*,struct _tuple0*))=(struct Cyc_Dict_Dict(*)(int(*cmp)(struct _tuple0*,struct _tuple0*)))Cyc_Dict_empty;_tmp0;})(Cyc_Absyn_qvar_cmp);});_tmp6->aggrdecls=_tmp2DC;}),({
struct Cyc_Dict_Dict _tmp2DB=({({struct Cyc_Dict_Dict(*_tmp1)(int(*cmp)(struct _tuple0*,struct _tuple0*))=(struct Cyc_Dict_Dict(*)(int(*cmp)(struct _tuple0*,struct _tuple0*)))Cyc_Dict_empty;_tmp1;})(Cyc_Absyn_qvar_cmp);});_tmp6->datatypedecls=_tmp2DB;}),({
struct Cyc_Dict_Dict _tmp2DA=({({struct Cyc_Dict_Dict(*_tmp2)(int(*cmp)(struct _tuple0*,struct _tuple0*))=(struct Cyc_Dict_Dict(*)(int(*cmp)(struct _tuple0*,struct _tuple0*)))Cyc_Dict_empty;_tmp2;})(Cyc_Absyn_qvar_cmp);});_tmp6->enumdecls=_tmp2DA;}),({
struct Cyc_Dict_Dict _tmp2D9=({({struct Cyc_Dict_Dict(*_tmp3)(int(*cmp)(struct _tuple0*,struct _tuple0*))=(struct Cyc_Dict_Dict(*)(int(*cmp)(struct _tuple0*,struct _tuple0*)))Cyc_Dict_empty;_tmp3;})(Cyc_Absyn_qvar_cmp);});_tmp6->typedefdecls=_tmp2D9;}),({
struct Cyc_Dict_Dict _tmp2D8=({({struct Cyc_Dict_Dict(*_tmp4)(int(*cmp)(struct _tuple0*,struct _tuple0*))=(struct Cyc_Dict_Dict(*)(int(*cmp)(struct _tuple0*,struct _tuple0*)))Cyc_Dict_empty;_tmp4;})(Cyc_Absyn_qvar_cmp);});_tmp6->vardecls=_tmp2D8;}),({
struct Cyc_Dict_Dict _tmp2D7=({({struct Cyc_Dict_Dict(*_tmp5)(int(*cmp)(struct _tuple0*,struct _tuple0*))=(struct Cyc_Dict_Dict(*)(int(*cmp)(struct _tuple0*,struct _tuple0*)))Cyc_Dict_empty;_tmp5;})(Cyc_Absyn_qvar_cmp);});_tmp6->xdatatypefielddecls=_tmp2D7;});_tmp6;});}
# 73
struct Cyc_Interface_I*Cyc_Interface_empty(){
# 84
return({struct Cyc_Interface_I*_tmp8=_cycalloc(sizeof(*_tmp8));({struct Cyc_Interface_Ienv*_tmp2DE=({Cyc_Interface_new_ienv();});_tmp8->imports=_tmp2DE;}),({struct Cyc_Interface_Ienv*_tmp2DD=({Cyc_Interface_new_ienv();});_tmp8->exports=_tmp2DD;}),_tmp8->tds=0;_tmp8;});}
# 73
struct Cyc_Interface_I*Cyc_Interface_final(){
# 89
struct Cyc_Interface_I*i=({Cyc_Interface_empty();});
# 91
struct Cyc_Absyn_Datatypedecl*exn_d=({struct Cyc_Absyn_Datatypedecl*_tmpE=_cycalloc(sizeof(*_tmpE));({struct Cyc_Absyn_Datatypedecl _tmp2DF=*({Cyc_Absyn_exn_tud();});*_tmpE=_tmp2DF;});_tmpE;});
exn_d->sc=Cyc_Absyn_Public;
({struct Cyc_Dict_Dict _tmp2E3=({({struct Cyc_Dict_Dict(*_tmp2E2)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Datatypedecl*v)=({struct Cyc_Dict_Dict(*_tmpA)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Datatypedecl*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Datatypedecl*v))Cyc_Dict_insert;_tmpA;});struct Cyc_Dict_Dict _tmp2E1=(i->exports)->datatypedecls;struct _tuple0*_tmp2E0=Cyc_Absyn_exn_name;_tmp2E2(_tmp2E1,_tmp2E0,exn_d);});});(i->exports)->datatypedecls=_tmp2E3;});
# 95
{struct Cyc_List_List*tufs=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(({Cyc_Absyn_exn_tud();})->fields))->v;for(0;tufs != 0;tufs=tufs->tl){
struct Cyc_Absyn_Datatypefield*exn_f=({struct Cyc_Absyn_Datatypefield*_tmpD=_cycalloc(sizeof(*_tmpD));*_tmpD=*((struct Cyc_Absyn_Datatypefield*)tufs->hd);_tmpD;});
exn_f->sc=Cyc_Absyn_Public;{
struct Cyc_Tcdecl_Xdatatypefielddecl*exn_fd=({struct Cyc_Tcdecl_Xdatatypefielddecl*_tmpC=_cycalloc(sizeof(*_tmpC));_tmpC->base=exn_d,_tmpC->field=exn_f;_tmpC;});
({struct Cyc_Dict_Dict _tmp2E7=({({struct Cyc_Dict_Dict(*_tmp2E6)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Tcdecl_Xdatatypefielddecl*v)=({struct Cyc_Dict_Dict(*_tmpB)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Tcdecl_Xdatatypefielddecl*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Tcdecl_Xdatatypefielddecl*v))Cyc_Dict_insert;_tmpB;});struct Cyc_Dict_Dict _tmp2E5=(i->exports)->xdatatypefielddecls;struct _tuple0*_tmp2E4=((struct Cyc_Absyn_Datatypefield*)tufs->hd)->name;_tmp2E6(_tmp2E5,_tmp2E4,exn_fd);});});(i->exports)->xdatatypefielddecls=_tmp2E7;});}}}
# 120 "interface.cyc"
i->imports=i->exports;
return i;}
# 140 "interface.cyc"
static void Cyc_Interface_err(struct _fat_ptr msg){
({void*_tmp10=0U;({struct _fat_ptr _tmp2E8=msg;Cyc_Tcutil_terr(0U,_tmp2E8,_tag_fat(_tmp10,sizeof(void*),0U));});});}
# 143
static void*Cyc_Interface_invalid_arg(struct _fat_ptr s){
(int)_throw(({struct Cyc_Core_Invalid_argument_exn_struct*_tmp12=_cycalloc(sizeof(*_tmp12));_tmp12->tag=Cyc_Core_Invalid_argument,_tmp12->f1=s;_tmp12;}));}
# 146
static void Cyc_Interface_fields_err(struct _fat_ptr sc,struct _fat_ptr t,struct _tuple0*n){
({void(*_tmp14)(struct _fat_ptr msg)=Cyc_Interface_err;struct _fat_ptr _tmp15=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp18=({struct Cyc_String_pa_PrintArg_struct _tmp2A6;_tmp2A6.tag=0U,_tmp2A6.f1=(struct _fat_ptr)((struct _fat_ptr)sc);_tmp2A6;});struct Cyc_String_pa_PrintArg_struct _tmp19=({struct Cyc_String_pa_PrintArg_struct _tmp2A5;_tmp2A5.tag=0U,_tmp2A5.f1=(struct _fat_ptr)((struct _fat_ptr)t);_tmp2A5;});struct Cyc_String_pa_PrintArg_struct _tmp1A=({struct Cyc_String_pa_PrintArg_struct _tmp2A4;_tmp2A4.tag=0U,({
struct _fat_ptr _tmp2E9=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(n);}));_tmp2A4.f1=_tmp2E9;});_tmp2A4;});void*_tmp16[3U];_tmp16[0]=& _tmp18,_tmp16[1]=& _tmp19,_tmp16[2]=& _tmp1A;({struct _fat_ptr _tmp2EA=({const char*_tmp17="fields of %s %s %s have never been defined";_tag_fat(_tmp17,sizeof(char),43U);});Cyc_aprintf(_tmp2EA,_tag_fat(_tmp16,sizeof(void*),3U));});});_tmp14(_tmp15);});}
# 150
static void Cyc_Interface_body_err(struct _fat_ptr sc,struct _tuple0*n){
({void(*_tmp1C)(struct _fat_ptr msg)=Cyc_Interface_err;struct _fat_ptr _tmp1D=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp20=({struct Cyc_String_pa_PrintArg_struct _tmp2A8;_tmp2A8.tag=0U,_tmp2A8.f1=(struct _fat_ptr)((struct _fat_ptr)sc);_tmp2A8;});struct Cyc_String_pa_PrintArg_struct _tmp21=({struct Cyc_String_pa_PrintArg_struct _tmp2A7;_tmp2A7.tag=0U,({
struct _fat_ptr _tmp2EB=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(n);}));_tmp2A7.f1=_tmp2EB;});_tmp2A7;});void*_tmp1E[2U];_tmp1E[0]=& _tmp20,_tmp1E[1]=& _tmp21;({struct _fat_ptr _tmp2EC=({const char*_tmp1F="the body of %s function %s has never been defined";_tag_fat(_tmp1F,sizeof(char),50U);});Cyc_aprintf(_tmp2EC,_tag_fat(_tmp1E,sizeof(void*),2U));});});_tmp1C(_tmp1D);});}
# 156
static void Cyc_Interface_static_err(struct _fat_ptr obj1,struct _tuple0*name1,struct _fat_ptr obj2,struct _tuple0*name2){
if(({char*_tmp2ED=(char*)obj1.curr;_tmp2ED != (char*)(_tag_fat(0,0,0)).curr;}))
({void(*_tmp23)(struct _fat_ptr msg)=Cyc_Interface_err;struct _fat_ptr _tmp24=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp27=({struct Cyc_String_pa_PrintArg_struct _tmp2AC;_tmp2AC.tag=0U,_tmp2AC.f1=(struct _fat_ptr)((struct _fat_ptr)obj1);_tmp2AC;});struct Cyc_String_pa_PrintArg_struct _tmp28=({struct Cyc_String_pa_PrintArg_struct _tmp2AB;_tmp2AB.tag=0U,({
struct _fat_ptr _tmp2EE=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(name1);}));_tmp2AB.f1=_tmp2EE;});_tmp2AB;});struct Cyc_String_pa_PrintArg_struct _tmp29=({struct Cyc_String_pa_PrintArg_struct _tmp2AA;_tmp2AA.tag=0U,_tmp2AA.f1=(struct _fat_ptr)((struct _fat_ptr)obj2);_tmp2AA;});struct Cyc_String_pa_PrintArg_struct _tmp2A=({struct Cyc_String_pa_PrintArg_struct _tmp2A9;_tmp2A9.tag=0U,({
struct _fat_ptr _tmp2EF=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(name2);}));_tmp2A9.f1=_tmp2EF;});_tmp2A9;});void*_tmp25[4U];_tmp25[0]=& _tmp27,_tmp25[1]=& _tmp28,_tmp25[2]=& _tmp29,_tmp25[3]=& _tmp2A;({struct _fat_ptr _tmp2F0=({const char*_tmp26="declaration of %s %s relies on static %s %s";_tag_fat(_tmp26,sizeof(char),44U);});Cyc_aprintf(_tmp2F0,_tag_fat(_tmp25,sizeof(void*),4U));});});_tmp23(_tmp24);});}
# 164
static void Cyc_Interface_abstract_err(struct _fat_ptr obj1,struct _tuple0*name1,struct _fat_ptr obj2,struct _tuple0*name2){
if(({char*_tmp2F1=(char*)obj1.curr;_tmp2F1 != (char*)(_tag_fat(0,0,0)).curr;}))
({void(*_tmp2C)(struct _fat_ptr msg)=Cyc_Interface_err;struct _fat_ptr _tmp2D=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp30=({struct Cyc_String_pa_PrintArg_struct _tmp2B0;_tmp2B0.tag=0U,_tmp2B0.f1=(struct _fat_ptr)((struct _fat_ptr)obj1);_tmp2B0;});struct Cyc_String_pa_PrintArg_struct _tmp31=({struct Cyc_String_pa_PrintArg_struct _tmp2AF;_tmp2AF.tag=0U,({
struct _fat_ptr _tmp2F2=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(name1);}));_tmp2AF.f1=_tmp2F2;});_tmp2AF;});struct Cyc_String_pa_PrintArg_struct _tmp32=({struct Cyc_String_pa_PrintArg_struct _tmp2AE;_tmp2AE.tag=0U,_tmp2AE.f1=(struct _fat_ptr)((struct _fat_ptr)obj2);_tmp2AE;});struct Cyc_String_pa_PrintArg_struct _tmp33=({struct Cyc_String_pa_PrintArg_struct _tmp2AD;_tmp2AD.tag=0U,({
struct _fat_ptr _tmp2F3=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(name2);}));_tmp2AD.f1=_tmp2F3;});_tmp2AD;});void*_tmp2E[4U];_tmp2E[0]=& _tmp30,_tmp2E[1]=& _tmp31,_tmp2E[2]=& _tmp32,_tmp2E[3]=& _tmp33;({struct _fat_ptr _tmp2F4=({const char*_tmp2F="declaration of %s %s relies on fields of abstract %s %s";_tag_fat(_tmp2F,sizeof(char),56U);});Cyc_aprintf(_tmp2F4,_tag_fat(_tmp2E,sizeof(void*),4U));});});_tmp2C(_tmp2D);});}struct Cyc_Interface_Seen{struct Cyc_Dict_Dict aggrs;struct Cyc_Dict_Dict datatypes;};
# 181
static struct Cyc_Interface_Seen*Cyc_Interface_new_seen(){
return({struct Cyc_Interface_Seen*_tmp37=_cycalloc(sizeof(*_tmp37));({struct Cyc_Dict_Dict _tmp2F6=({({struct Cyc_Dict_Dict(*_tmp35)(int(*cmp)(struct _tuple0*,struct _tuple0*))=(struct Cyc_Dict_Dict(*)(int(*cmp)(struct _tuple0*,struct _tuple0*)))Cyc_Dict_empty;_tmp35;})(Cyc_Absyn_qvar_cmp);});_tmp37->aggrs=_tmp2F6;}),({
struct Cyc_Dict_Dict _tmp2F5=({({struct Cyc_Dict_Dict(*_tmp36)(int(*cmp)(struct _tuple0*,struct _tuple0*))=(struct Cyc_Dict_Dict(*)(int(*cmp)(struct _tuple0*,struct _tuple0*)))Cyc_Dict_empty;_tmp36;})(Cyc_Absyn_qvar_cmp);});_tmp37->datatypes=_tmp2F5;});_tmp37;});}
# 181
static int Cyc_Interface_check_public_type(struct Cyc_Tcenv_Genv*ae,struct Cyc_Interface_Seen*seen,struct _fat_ptr obj,struct _tuple0*name,void*t);
# 190
static int Cyc_Interface_check_public_type_list(struct Cyc_Tcenv_Genv*ae,struct Cyc_Interface_Seen*seen,struct _fat_ptr obj,struct _tuple0*name,void*(*f)(void*),struct Cyc_List_List*l){
# 192
int res=1;
for(0;l != 0;l=l->tl){
if(!({int(*_tmp39)(struct Cyc_Tcenv_Genv*ae,struct Cyc_Interface_Seen*seen,struct _fat_ptr obj,struct _tuple0*name,void*t)=Cyc_Interface_check_public_type;struct Cyc_Tcenv_Genv*_tmp3A=ae;struct Cyc_Interface_Seen*_tmp3B=seen;struct _fat_ptr _tmp3C=obj;struct _tuple0*_tmp3D=name;void*_tmp3E=({f(l->hd);});_tmp39(_tmp3A,_tmp3B,_tmp3C,_tmp3D,_tmp3E);}))
res=0;}
# 193
return res;}
# 199
static int Cyc_Interface_check_public_aggrdecl(struct Cyc_Tcenv_Genv*ae,struct Cyc_Interface_Seen*seen,struct Cyc_Absyn_Aggrdecl*d){
{struct _handler_cons _tmp40;_push_handler(& _tmp40);{int _tmp42=0;if(setjmp(_tmp40.handler))_tmp42=1;if(!_tmp42){{int _tmp44=({({int(*_tmp2F8)(struct Cyc_Dict_Dict d,struct _tuple0*k)=({int(*_tmp43)(struct Cyc_Dict_Dict d,struct _tuple0*k)=(int(*)(struct Cyc_Dict_Dict d,struct _tuple0*k))Cyc_Dict_lookup;_tmp43;});struct Cyc_Dict_Dict _tmp2F7=seen->aggrs;_tmp2F8(_tmp2F7,d->name);});});_npop_handler(0U);return _tmp44;};_pop_handler();}else{void*_tmp41=(void*)Cyc_Core_get_exn_thrown();void*_tmp45=_tmp41;void*_tmp46;if(((struct Cyc_Dict_Absent_exn_struct*)_tmp45)->tag == Cyc_Dict_Absent){_LL1: _LL2:
 goto _LL0;}else{_LL3: _tmp46=_tmp45;_LL4: {void*exn=_tmp46;(int)_rethrow(exn);}}_LL0:;}}}{
int res=1;
({struct Cyc_Dict_Dict _tmp2FC=({({struct Cyc_Dict_Dict(*_tmp2FB)(struct Cyc_Dict_Dict d,struct _tuple0*k,int v)=({struct Cyc_Dict_Dict(*_tmp47)(struct Cyc_Dict_Dict d,struct _tuple0*k,int v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _tuple0*k,int v))Cyc_Dict_insert;_tmp47;});struct Cyc_Dict_Dict _tmp2FA=seen->aggrs;struct _tuple0*_tmp2F9=d->name;_tmp2FB(_tmp2FA,_tmp2F9,res);});});seen->aggrs=_tmp2FC;});
if(d->impl != 0){
struct Cyc_List_List*fs=((struct Cyc_Absyn_AggrdeclImpl*)_check_null(d->impl))->fields;for(0;fs != 0;fs=fs->tl){
if(!({({struct Cyc_Tcenv_Genv*_tmp300=ae;struct Cyc_Interface_Seen*_tmp2FF=seen;struct _fat_ptr _tmp2FE=({const char*_tmp48="type";_tag_fat(_tmp48,sizeof(char),5U);});struct _tuple0*_tmp2FD=d->name;Cyc_Interface_check_public_type(_tmp300,_tmp2FF,_tmp2FE,_tmp2FD,((struct Cyc_Absyn_Aggrfield*)fs->hd)->type);});}))
res=0;}}
# 204
({struct Cyc_Dict_Dict _tmp304=({({struct Cyc_Dict_Dict(*_tmp303)(struct Cyc_Dict_Dict d,struct _tuple0*k,int v)=({
# 209
struct Cyc_Dict_Dict(*_tmp49)(struct Cyc_Dict_Dict d,struct _tuple0*k,int v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _tuple0*k,int v))Cyc_Dict_insert;_tmp49;});struct Cyc_Dict_Dict _tmp302=seen->aggrs;struct _tuple0*_tmp301=d->name;_tmp303(_tmp302,_tmp301,res);});});
# 204
seen->aggrs=_tmp304;});
# 210
return res;}}struct _tuple25{struct Cyc_Absyn_Tqual f1;void*f2;};
# 213
static void*Cyc_Interface_get_type1(struct _tuple25*x){
return(*x).f2;}
# 216
static void*Cyc_Interface_get_type2(struct _tuple9*x){
return(*x).f3;}
# 220
static int Cyc_Interface_check_public_datatypedecl(struct Cyc_Tcenv_Genv*ae,struct Cyc_Interface_Seen*seen,struct Cyc_Absyn_Datatypedecl*d){
{struct _handler_cons _tmp4D;_push_handler(& _tmp4D);{int _tmp4F=0;if(setjmp(_tmp4D.handler))_tmp4F=1;if(!_tmp4F){{int _tmp51=({({int(*_tmp306)(struct Cyc_Dict_Dict d,struct _tuple0*k)=({int(*_tmp50)(struct Cyc_Dict_Dict d,struct _tuple0*k)=(int(*)(struct Cyc_Dict_Dict d,struct _tuple0*k))Cyc_Dict_lookup;_tmp50;});struct Cyc_Dict_Dict _tmp305=seen->datatypes;_tmp306(_tmp305,d->name);});});_npop_handler(0U);return _tmp51;};_pop_handler();}else{void*_tmp4E=(void*)Cyc_Core_get_exn_thrown();void*_tmp52=_tmp4E;void*_tmp53;if(((struct Cyc_Dict_Absent_exn_struct*)_tmp52)->tag == Cyc_Dict_Absent){_LL1: _LL2:
 goto _LL0;}else{_LL3: _tmp53=_tmp52;_LL4: {void*exn=_tmp53;(int)_rethrow(exn);}}_LL0:;}}}{
int res=1;
({struct Cyc_Dict_Dict _tmp30A=({({struct Cyc_Dict_Dict(*_tmp309)(struct Cyc_Dict_Dict d,struct _tuple0*k,int v)=({struct Cyc_Dict_Dict(*_tmp54)(struct Cyc_Dict_Dict d,struct _tuple0*k,int v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _tuple0*k,int v))Cyc_Dict_insert;_tmp54;});struct Cyc_Dict_Dict _tmp308=seen->datatypes;struct _tuple0*_tmp307=d->name;_tmp309(_tmp308,_tmp307,res);});});seen->datatypes=_tmp30A;});
if(d->fields != 0){
struct Cyc_List_List*fs=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(d->fields))->v;for(0;fs != 0;fs=fs->tl){
if(!({({int(*_tmp30F)(struct Cyc_Tcenv_Genv*ae,struct Cyc_Interface_Seen*seen,struct _fat_ptr obj,struct _tuple0*name,void*(*f)(struct _tuple25*),struct Cyc_List_List*l)=({int(*_tmp55)(struct Cyc_Tcenv_Genv*ae,struct Cyc_Interface_Seen*seen,struct _fat_ptr obj,struct _tuple0*name,void*(*f)(struct _tuple25*),struct Cyc_List_List*l)=(int(*)(struct Cyc_Tcenv_Genv*ae,struct Cyc_Interface_Seen*seen,struct _fat_ptr obj,struct _tuple0*name,void*(*f)(struct _tuple25*),struct Cyc_List_List*l))Cyc_Interface_check_public_type_list;_tmp55;});struct Cyc_Tcenv_Genv*_tmp30E=ae;struct Cyc_Interface_Seen*_tmp30D=seen;struct _fat_ptr _tmp30C=({const char*_tmp56="datatype";_tag_fat(_tmp56,sizeof(char),9U);});struct _tuple0*_tmp30B=d->name;_tmp30F(_tmp30E,_tmp30D,_tmp30C,_tmp30B,Cyc_Interface_get_type1,((struct Cyc_Absyn_Datatypefield*)fs->hd)->typs);});}))
# 229
res=0;}}
# 225
({struct Cyc_Dict_Dict _tmp313=({({struct Cyc_Dict_Dict(*_tmp312)(struct Cyc_Dict_Dict d,struct _tuple0*k,int v)=({
# 231
struct Cyc_Dict_Dict(*_tmp57)(struct Cyc_Dict_Dict d,struct _tuple0*k,int v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _tuple0*k,int v))Cyc_Dict_insert;_tmp57;});struct Cyc_Dict_Dict _tmp311=seen->datatypes;struct _tuple0*_tmp310=d->name;_tmp312(_tmp311,_tmp310,res);});});
# 225
seen->datatypes=_tmp313;});
# 232
return res;}}
# 235
static int Cyc_Interface_check_public_enumdecl(struct Cyc_Tcenv_Genv*ae,struct Cyc_Interface_Seen*seen,struct Cyc_Absyn_Enumdecl*d){
return 1;}
# 239
static int Cyc_Interface_check_public_typedefdecl(struct Cyc_Tcenv_Genv*ae,struct Cyc_Interface_Seen*seen,struct Cyc_Absyn_Typedefdecl*d){
if(d->defn != 0)
return({({struct Cyc_Tcenv_Genv*_tmp317=ae;struct Cyc_Interface_Seen*_tmp316=seen;struct _fat_ptr _tmp315=_tag_fat(0,0,0);struct _tuple0*_tmp314=d->name;Cyc_Interface_check_public_type(_tmp317,_tmp316,_tmp315,_tmp314,(void*)_check_null(d->defn));});});else{
return 1;}}
# 246
static int Cyc_Interface_check_public_vardecl(struct Cyc_Tcenv_Genv*ae,struct Cyc_Interface_Seen*seen,struct Cyc_Absyn_Vardecl*d){
return({({struct Cyc_Tcenv_Genv*_tmp31B=ae;struct Cyc_Interface_Seen*_tmp31A=seen;struct _fat_ptr _tmp319=({const char*_tmp5B="variable";_tag_fat(_tmp5B,sizeof(char),9U);});struct _tuple0*_tmp318=d->name;Cyc_Interface_check_public_type(_tmp31B,_tmp31A,_tmp319,_tmp318,d->type);});});}
# 250
static int Cyc_Interface_check_public_type(struct Cyc_Tcenv_Genv*ae,struct Cyc_Interface_Seen*seen,struct _fat_ptr obj,struct _tuple0*name,void*t){
void*_stmttmp0=({Cyc_Tcutil_compress(t);});void*_tmp5D=_stmttmp0;struct Cyc_List_List*_tmp60;struct Cyc_Absyn_Datatypefield*_tmp5F;struct Cyc_Absyn_Datatypedecl*_tmp5E;struct Cyc_List_List*_tmp62;struct Cyc_Absyn_Datatypedecl*_tmp61;struct _tuple0*_tmp63;struct Cyc_List_List*_tmp65;union Cyc_Absyn_AggrInfo _tmp64;struct Cyc_List_List*_tmp66;struct Cyc_Absyn_VarargInfo*_tmp69;struct Cyc_List_List*_tmp68;void*_tmp67;void*_tmp6A;void*_tmp6B;void*_tmp6C;switch(*((int*)_tmp5D)){case 3U: _LL1: _tmp6C=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp5D)->f1).elt_type;_LL2: {void*t=_tmp6C;
_tmp6B=t;goto _LL4;}case 4U: _LL3: _tmp6B=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp5D)->f1).elt_type;_LL4: {void*t=_tmp6B;
_tmp6A=t;goto _LL6;}case 8U: _LL5: _tmp6A=(void*)((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp5D)->f4;if(_tmp6A != 0){_LL6: {void*t=_tmp6A;
# 255
return({Cyc_Interface_check_public_type(ae,seen,obj,name,t);});}}else{goto _LL13;}case 5U: _LL7: _tmp67=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp5D)->f1).ret_type;_tmp68=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp5D)->f1).args;_tmp69=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp5D)->f1).cyc_varargs;_LL8: {void*ret=_tmp67;struct Cyc_List_List*args=_tmp68;struct Cyc_Absyn_VarargInfo*cyc_varargs=_tmp69;
# 259
int b=({({int(*_tmp320)(struct Cyc_Tcenv_Genv*ae,struct Cyc_Interface_Seen*seen,struct _fat_ptr obj,struct _tuple0*name,void*(*f)(struct _tuple9*),struct Cyc_List_List*l)=({int(*_tmp6E)(struct Cyc_Tcenv_Genv*ae,struct Cyc_Interface_Seen*seen,struct _fat_ptr obj,struct _tuple0*name,void*(*f)(struct _tuple9*),struct Cyc_List_List*l)=(int(*)(struct Cyc_Tcenv_Genv*ae,struct Cyc_Interface_Seen*seen,struct _fat_ptr obj,struct _tuple0*name,void*(*f)(struct _tuple9*),struct Cyc_List_List*l))Cyc_Interface_check_public_type_list;_tmp6E;});struct Cyc_Tcenv_Genv*_tmp31F=ae;struct Cyc_Interface_Seen*_tmp31E=seen;struct _fat_ptr _tmp31D=obj;struct _tuple0*_tmp31C=name;_tmp320(_tmp31F,_tmp31E,_tmp31D,_tmp31C,Cyc_Interface_get_type2,args);});})&&({Cyc_Interface_check_public_type(ae,seen,obj,name,ret);});
# 261
if(cyc_varargs != 0){
struct Cyc_Absyn_VarargInfo _stmttmp1=*cyc_varargs;void*_tmp6D;_LL16: _tmp6D=_stmttmp1.type;_LL17: {void*vt=_tmp6D;
b=({Cyc_Interface_check_public_type(ae,seen,obj,name,vt);});}}
# 261
return b;}case 6U: _LL9: _tmp66=((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp5D)->f1;_LLA: {struct Cyc_List_List*lt=_tmp66;
# 268
return({({int(*_tmp325)(struct Cyc_Tcenv_Genv*ae,struct Cyc_Interface_Seen*seen,struct _fat_ptr obj,struct _tuple0*name,void*(*f)(struct _tuple25*),struct Cyc_List_List*l)=({int(*_tmp6F)(struct Cyc_Tcenv_Genv*ae,struct Cyc_Interface_Seen*seen,struct _fat_ptr obj,struct _tuple0*name,void*(*f)(struct _tuple25*),struct Cyc_List_List*l)=(int(*)(struct Cyc_Tcenv_Genv*ae,struct Cyc_Interface_Seen*seen,struct _fat_ptr obj,struct _tuple0*name,void*(*f)(struct _tuple25*),struct Cyc_List_List*l))Cyc_Interface_check_public_type_list;_tmp6F;});struct Cyc_Tcenv_Genv*_tmp324=ae;struct Cyc_Interface_Seen*_tmp323=seen;struct _fat_ptr _tmp322=obj;struct _tuple0*_tmp321=name;_tmp325(_tmp324,_tmp323,_tmp322,_tmp321,Cyc_Interface_get_type1,lt);});});}case 0U: switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp5D)->f1)){case 22U: _LLB: _tmp64=((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp5D)->f1)->f1;_tmp65=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp5D)->f2;_LLC: {union Cyc_Absyn_AggrInfo info=_tmp64;struct Cyc_List_List*targs=_tmp65;
# 271
struct Cyc_Absyn_Aggrdecl*ad=({Cyc_Absyn_get_known_aggrdecl(info);});
if((int)ad->sc == (int)Cyc_Absyn_Static){
({({struct _fat_ptr _tmp328=obj;struct _tuple0*_tmp327=name;struct _fat_ptr _tmp326=({const char*_tmp70="type";_tag_fat(_tmp70,sizeof(char),5U);});Cyc_Interface_static_err(_tmp328,_tmp327,_tmp326,ad->name);});});
return 0;}
# 272
return
# 276
({Cyc_Interface_check_public_type_list(ae,seen,obj,name,Cyc_Core_identity,targs);})&&({Cyc_Interface_check_public_aggrdecl(ae,seen,ad);});}case 17U: _LLD: _tmp63=((struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp5D)->f1)->f1;_LLE: {struct _tuple0*name=_tmp63;
# 280
struct Cyc_Absyn_Enumdecl*ed;
{struct _handler_cons _tmp71;_push_handler(& _tmp71);{int _tmp73=0;if(setjmp(_tmp71.handler))_tmp73=1;if(!_tmp73){ed=*({({struct Cyc_Absyn_Enumdecl**(*_tmp32A)(struct Cyc_Dict_Dict d,struct _tuple0*k)=({struct Cyc_Absyn_Enumdecl**(*_tmp74)(struct Cyc_Dict_Dict d,struct _tuple0*k)=(struct Cyc_Absyn_Enumdecl**(*)(struct Cyc_Dict_Dict d,struct _tuple0*k))Cyc_Dict_lookup;_tmp74;});struct Cyc_Dict_Dict _tmp329=ae->enumdecls;_tmp32A(_tmp329,name);});});;_pop_handler();}else{void*_tmp72=(void*)Cyc_Core_get_exn_thrown();void*_tmp75=_tmp72;void*_tmp76;if(((struct Cyc_Dict_Absent_exn_struct*)_tmp75)->tag == Cyc_Dict_Absent){_LL19: _LL1A:
# 283
({int(*_tmp77)(struct _fat_ptr s)=({int(*_tmp78)(struct _fat_ptr s)=(int(*)(struct _fat_ptr s))Cyc_Interface_invalid_arg;_tmp78;});struct _fat_ptr _tmp79=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp7C=({struct Cyc_String_pa_PrintArg_struct _tmp2B1;_tmp2B1.tag=0U,({
struct _fat_ptr _tmp32B=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(name);}));_tmp2B1.f1=_tmp32B;});_tmp2B1;});void*_tmp7A[1U];_tmp7A[0]=& _tmp7C;({struct _fat_ptr _tmp32C=({const char*_tmp7B="check_public_type (can't find enum %s)";_tag_fat(_tmp7B,sizeof(char),39U);});Cyc_aprintf(_tmp32C,_tag_fat(_tmp7A,sizeof(void*),1U));});});_tmp77(_tmp79);});}else{_LL1B: _tmp76=_tmp75;_LL1C: {void*exn=_tmp76;(int)_rethrow(exn);}}_LL18:;}}}
# 286
if((int)ed->sc == (int)Cyc_Absyn_Static){
({({struct _fat_ptr _tmp32F=obj;struct _tuple0*_tmp32E=name;struct _fat_ptr _tmp32D=({const char*_tmp7D="enum";_tag_fat(_tmp7D,sizeof(char),5U);});Cyc_Interface_static_err(_tmp32F,_tmp32E,_tmp32D,ed->name);});});
return 0;}
# 286
return 1;}case 20U: if(((((struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp5D)->f1)->f1).KnownDatatype).tag == 2){_LLF: _tmp61=*((((struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp5D)->f1)->f1).KnownDatatype).val;_tmp62=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp5D)->f2;_LL10: {struct Cyc_Absyn_Datatypedecl*tud0=_tmp61;struct Cyc_List_List*targs=_tmp62;
# 293
struct Cyc_Absyn_Datatypedecl*tud;
{struct _handler_cons _tmp7E;_push_handler(& _tmp7E);{int _tmp80=0;if(setjmp(_tmp7E.handler))_tmp80=1;if(!_tmp80){tud=*({({struct Cyc_Absyn_Datatypedecl**(*_tmp331)(struct Cyc_Dict_Dict d,struct _tuple0*k)=({struct Cyc_Absyn_Datatypedecl**(*_tmp81)(struct Cyc_Dict_Dict d,struct _tuple0*k)=(struct Cyc_Absyn_Datatypedecl**(*)(struct Cyc_Dict_Dict d,struct _tuple0*k))Cyc_Dict_lookup;_tmp81;});struct Cyc_Dict_Dict _tmp330=ae->datatypedecls;_tmp331(_tmp330,tud0->name);});});;_pop_handler();}else{void*_tmp7F=(void*)Cyc_Core_get_exn_thrown();void*_tmp82=_tmp7F;void*_tmp83;if(((struct Cyc_Dict_Absent_exn_struct*)_tmp82)->tag == Cyc_Dict_Absent){_LL1E: _LL1F:
# 296
({int(*_tmp84)(struct _fat_ptr s)=({int(*_tmp85)(struct _fat_ptr s)=(int(*)(struct _fat_ptr s))Cyc_Interface_invalid_arg;_tmp85;});struct _fat_ptr _tmp86=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp89=({struct Cyc_String_pa_PrintArg_struct _tmp2B2;_tmp2B2.tag=0U,({
struct _fat_ptr _tmp332=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(tud0->name);}));_tmp2B2.f1=_tmp332;});_tmp2B2;});void*_tmp87[1U];_tmp87[0]=& _tmp89;({struct _fat_ptr _tmp333=({const char*_tmp88="check_public_type (can't find datatype %s)";_tag_fat(_tmp88,sizeof(char),43U);});Cyc_aprintf(_tmp333,_tag_fat(_tmp87,sizeof(void*),1U));});});_tmp84(_tmp86);});}else{_LL20: _tmp83=_tmp82;_LL21: {void*exn=_tmp83;(int)_rethrow(exn);}}_LL1D:;}}}
# 299
if((int)tud->sc == (int)Cyc_Absyn_Static){
({({struct _fat_ptr _tmp336=obj;struct _tuple0*_tmp335=name;struct _fat_ptr _tmp334=({const char*_tmp8A="datatype";_tag_fat(_tmp8A,sizeof(char),9U);});Cyc_Interface_static_err(_tmp336,_tmp335,_tmp334,tud->name);});});
return 0;}
# 299
return
# 303
({Cyc_Interface_check_public_type_list(ae,seen,obj,name,Cyc_Core_identity,targs);})&&({Cyc_Interface_check_public_datatypedecl(ae,seen,tud);});}}else{goto _LL13;}case 21U: if(((((struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp5D)->f1)->f1).KnownDatatypefield).tag == 2){_LL11: _tmp5E=(((((struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp5D)->f1)->f1).KnownDatatypefield).val).f1;_tmp5F=(((((struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp5D)->f1)->f1).KnownDatatypefield).val).f2;_tmp60=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp5D)->f2;_LL12: {struct Cyc_Absyn_Datatypedecl*tud0=_tmp5E;struct Cyc_Absyn_Datatypefield*tuf0=_tmp5F;struct Cyc_List_List*targs=_tmp60;
# 307
struct Cyc_Absyn_Datatypedecl*tud;
{struct _handler_cons _tmp8B;_push_handler(& _tmp8B);{int _tmp8D=0;if(setjmp(_tmp8B.handler))_tmp8D=1;if(!_tmp8D){tud=*({({struct Cyc_Absyn_Datatypedecl**(*_tmp338)(struct Cyc_Dict_Dict d,struct _tuple0*k)=({struct Cyc_Absyn_Datatypedecl**(*_tmp8E)(struct Cyc_Dict_Dict d,struct _tuple0*k)=(struct Cyc_Absyn_Datatypedecl**(*)(struct Cyc_Dict_Dict d,struct _tuple0*k))Cyc_Dict_lookup;_tmp8E;});struct Cyc_Dict_Dict _tmp337=ae->datatypedecls;_tmp338(_tmp337,tud0->name);});});;_pop_handler();}else{void*_tmp8C=(void*)Cyc_Core_get_exn_thrown();void*_tmp8F=_tmp8C;void*_tmp90;if(((struct Cyc_Dict_Absent_exn_struct*)_tmp8F)->tag == Cyc_Dict_Absent){_LL23: _LL24:
# 310
({int(*_tmp91)(struct _fat_ptr s)=({int(*_tmp92)(struct _fat_ptr s)=(int(*)(struct _fat_ptr s))Cyc_Interface_invalid_arg;_tmp92;});struct _fat_ptr _tmp93=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp96=({struct Cyc_String_pa_PrintArg_struct _tmp2B3;_tmp2B3.tag=0U,({
struct _fat_ptr _tmp339=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(tud0->name);}));_tmp2B3.f1=_tmp339;});_tmp2B3;});void*_tmp94[1U];_tmp94[0]=& _tmp96;({struct _fat_ptr _tmp33A=({const char*_tmp95="check_public_type (can't find datatype %s and search its fields)";_tag_fat(_tmp95,sizeof(char),65U);});Cyc_aprintf(_tmp33A,_tag_fat(_tmp94,sizeof(void*),1U));});});_tmp91(_tmp93);});}else{_LL25: _tmp90=_tmp8F;_LL26: {void*exn=_tmp90;(int)_rethrow(exn);}}_LL22:;}}}
# 313
if(tud->fields == 0)
({int(*_tmp97)(struct _fat_ptr s)=({int(*_tmp98)(struct _fat_ptr s)=(int(*)(struct _fat_ptr s))Cyc_Interface_invalid_arg;_tmp98;});struct _fat_ptr _tmp99=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp9C=({struct Cyc_String_pa_PrintArg_struct _tmp2B4;_tmp2B4.tag=0U,({
struct _fat_ptr _tmp33B=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(tud->name);}));_tmp2B4.f1=_tmp33B;});_tmp2B4;});void*_tmp9A[1U];_tmp9A[0]=& _tmp9C;({struct _fat_ptr _tmp33C=({const char*_tmp9B="check_public_type (datatype %s has no fields)";_tag_fat(_tmp9B,sizeof(char),46U);});Cyc_aprintf(_tmp33C,_tag_fat(_tmp9A,sizeof(void*),1U));});});_tmp97(_tmp99);});{
# 313
struct Cyc_Absyn_Datatypefield*tuf1=0;
# 318
{struct Cyc_List_List*fs=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(tud->fields))->v;for(0;fs != 0;fs=fs->tl){
if(({Cyc_strptrcmp((*tuf0->name).f2,(*((struct Cyc_Absyn_Datatypefield*)fs->hd)->name).f2);})== 0){
tuf1=(struct Cyc_Absyn_Datatypefield*)fs->hd;
break;}}}
# 318
if(tuf1 == 0)
# 325
({int(*_tmp9D)(struct _fat_ptr s)=({int(*_tmp9E)(struct _fat_ptr s)=(int(*)(struct _fat_ptr s))Cyc_Interface_invalid_arg;_tmp9E;});struct _fat_ptr _tmp9F=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmpA2=({struct Cyc_String_pa_PrintArg_struct _tmp2B5;_tmp2B5.tag=0U,({
struct _fat_ptr _tmp33D=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(tuf0->name);}));_tmp2B5.f1=_tmp33D;});_tmp2B5;});void*_tmpA0[1U];_tmpA0[0]=& _tmpA2;({struct _fat_ptr _tmp33E=({const char*_tmpA1="check_public_type (can't find datatypefield %s)";_tag_fat(_tmpA1,sizeof(char),48U);});Cyc_aprintf(_tmp33E,_tag_fat(_tmpA0,sizeof(void*),1U));});});_tmp9D(_tmp9F);});{
# 318
struct Cyc_Absyn_Datatypefield*tuf=tuf1;
# 329
if((int)tud->sc == (int)Cyc_Absyn_Static){
({({struct _fat_ptr _tmp341=obj;struct _tuple0*_tmp340=name;struct _fat_ptr _tmp33F=({const char*_tmpA3="datatype";_tag_fat(_tmpA3,sizeof(char),9U);});Cyc_Interface_static_err(_tmp341,_tmp340,_tmp33F,tud->name);});});
return 0;}
# 329
if((int)tud->sc == (int)Cyc_Absyn_Abstract){
# 334
({({struct _fat_ptr _tmp344=obj;struct _tuple0*_tmp343=name;struct _fat_ptr _tmp342=({const char*_tmpA4="datatype";_tag_fat(_tmpA4,sizeof(char),9U);});Cyc_Interface_abstract_err(_tmp344,_tmp343,_tmp342,tud->name);});});
return 0;}
# 329
if((int)tuf->sc == (int)Cyc_Absyn_Static){
# 338
({void(*_tmpA5)(struct _fat_ptr obj1,struct _tuple0*name1,struct _fat_ptr obj2,struct _tuple0*name2)=Cyc_Interface_static_err;struct _fat_ptr _tmpA6=obj;struct _tuple0*_tmpA7=name;struct _fat_ptr _tmpA8=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmpAB=({struct Cyc_String_pa_PrintArg_struct _tmp2B6;_tmp2B6.tag=0U,({
struct _fat_ptr _tmp345=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(tuf->name);}));_tmp2B6.f1=_tmp345;});_tmp2B6;});void*_tmpA9[1U];_tmpA9[0]=& _tmpAB;({struct _fat_ptr _tmp346=({const char*_tmpAA="field %s of";_tag_fat(_tmpAA,sizeof(char),12U);});Cyc_aprintf(_tmp346,_tag_fat(_tmpA9,sizeof(void*),1U));});});struct _tuple0*_tmpAC=tud->name;_tmpA5(_tmpA6,_tmpA7,_tmpA8,_tmpAC);});
return 0;}
# 329
return
# 342
({Cyc_Interface_check_public_type_list(ae,seen,obj,name,Cyc_Core_identity,targs);})&&({Cyc_Interface_check_public_datatypedecl(ae,seen,tud);});}}}}else{goto _LL13;}default: goto _LL13;}default: _LL13: _LL14:
# 345
 return 1;}_LL0:;}struct _tuple26{struct Cyc_Interface_Ienv*f1;struct Cyc_Interface_Ienv*f2;int f3;struct Cyc_Tcenv_Genv*f4;struct Cyc_Interface_Seen*f5;struct Cyc_Interface_I*f6;};
# 351
static void Cyc_Interface_extract_aggrdecl(struct _tuple26*env,struct _tuple0*x,struct Cyc_Absyn_Aggrdecl**dp){
# 353
struct Cyc_Interface_Seen*_tmpB2;struct Cyc_Tcenv_Genv*_tmpB1;int _tmpB0;struct Cyc_Interface_Ienv*_tmpAF;struct Cyc_Interface_Ienv*_tmpAE;_LL1: _tmpAE=env->f1;_tmpAF=env->f2;_tmpB0=env->f3;_tmpB1=env->f4;_tmpB2=env->f5;_LL2: {struct Cyc_Interface_Ienv*imp=_tmpAE;struct Cyc_Interface_Ienv*exp=_tmpAF;int check_complete_defs=_tmpB0;struct Cyc_Tcenv_Genv*ae=_tmpB1;struct Cyc_Interface_Seen*seen=_tmpB2;
struct Cyc_Absyn_Aggrdecl*d=*dp;
enum Cyc_Absyn_Scope _stmttmp2=d->sc;enum Cyc_Absyn_Scope _tmpB3=_stmttmp2;switch(_tmpB3){case Cyc_Absyn_Static: _LL4: _LL5:
# 357
 if(check_complete_defs && d->impl == 0)
({({struct _fat_ptr _tmp348=({const char*_tmpB4="static";_tag_fat(_tmpB4,sizeof(char),7U);});struct _fat_ptr _tmp347=({const char*_tmpB5="struct/union";_tag_fat(_tmpB5,sizeof(char),13U);});Cyc_Interface_fields_err(_tmp348,_tmp347,d->name);});});
# 357
goto _LL3;case Cyc_Absyn_Abstract: _LL6: _LL7:
# 361
 if(d->impl == 0){
if(check_complete_defs)
({({struct _fat_ptr _tmp34A=({const char*_tmpB6="abstract";_tag_fat(_tmpB6,sizeof(char),9U);});struct _fat_ptr _tmp349=({const char*_tmpB7="struct/union";_tag_fat(_tmpB7,sizeof(char),13U);});Cyc_Interface_fields_err(_tmp34A,_tmp349,d->name);});});}else{
# 365
d=({struct Cyc_Absyn_Aggrdecl*_tmpB8=_cycalloc(sizeof(*_tmpB8));*_tmpB8=*d;_tmpB8;});
d->impl=0;}
# 368
if(({Cyc_Interface_check_public_aggrdecl(ae,seen,d);}))
({struct Cyc_Dict_Dict _tmp34E=({({struct Cyc_Dict_Dict(*_tmp34D)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Aggrdecl*v)=({struct Cyc_Dict_Dict(*_tmpB9)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Aggrdecl*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Aggrdecl*v))Cyc_Dict_insert;_tmpB9;});struct Cyc_Dict_Dict _tmp34C=exp->aggrdecls;struct _tuple0*_tmp34B=x;_tmp34D(_tmp34C,_tmp34B,d);});});exp->aggrdecls=_tmp34E;});
# 368
goto _LL3;case Cyc_Absyn_Public: _LL8: _LL9:
# 372
 if(d->impl == 0){
({({struct _fat_ptr _tmp350=({const char*_tmpBA="public";_tag_fat(_tmpBA,sizeof(char),7U);});struct _fat_ptr _tmp34F=({const char*_tmpBB="struct/union";_tag_fat(_tmpBB,sizeof(char),13U);});Cyc_Interface_fields_err(_tmp350,_tmp34F,d->name);});});
d=({struct Cyc_Absyn_Aggrdecl*_tmpBC=_cycalloc(sizeof(*_tmpBC));*_tmpBC=*d;_tmpBC;});
d->sc=Cyc_Absyn_Abstract;}
# 372
if(({Cyc_Interface_check_public_aggrdecl(ae,seen,d);}))
# 378
({struct Cyc_Dict_Dict _tmp354=({({struct Cyc_Dict_Dict(*_tmp353)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Aggrdecl*v)=({struct Cyc_Dict_Dict(*_tmpBD)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Aggrdecl*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Aggrdecl*v))Cyc_Dict_insert;_tmpBD;});struct Cyc_Dict_Dict _tmp352=exp->aggrdecls;struct _tuple0*_tmp351=x;_tmp353(_tmp352,_tmp351,d);});});exp->aggrdecls=_tmp354;});
# 372
goto _LL3;case Cyc_Absyn_ExternC: _LLA: _LLB:
# 380
 goto _LLD;case Cyc_Absyn_Extern: _LLC: _LLD:
# 382
 if(({Cyc_Interface_check_public_aggrdecl(ae,seen,d);}))
({struct Cyc_Dict_Dict _tmp358=({({struct Cyc_Dict_Dict(*_tmp357)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Aggrdecl*v)=({struct Cyc_Dict_Dict(*_tmpBE)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Aggrdecl*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Aggrdecl*v))Cyc_Dict_insert;_tmpBE;});struct Cyc_Dict_Dict _tmp356=imp->aggrdecls;struct _tuple0*_tmp355=x;_tmp357(_tmp356,_tmp355,d);});});imp->aggrdecls=_tmp358;});
# 382
goto _LL3;case Cyc_Absyn_Register: _LLE: _LLF:
# 385
 goto _LL11;default: _LL10: _LL11:
# 387
({({int(*_tmp359)(struct _fat_ptr s)=({int(*_tmpBF)(struct _fat_ptr s)=(int(*)(struct _fat_ptr s))Cyc_Interface_invalid_arg;_tmpBF;});_tmp359(({const char*_tmpC0="add_aggrdecl";_tag_fat(_tmpC0,sizeof(char),13U);}));});});
goto _LL3;}_LL3:;}}
# 391
static void Cyc_Interface_extract_enumdecl(struct _tuple26*env,struct _tuple0*x,struct Cyc_Absyn_Enumdecl**dp){
# 393
struct Cyc_Interface_Seen*_tmpC6;struct Cyc_Tcenv_Genv*_tmpC5;int _tmpC4;struct Cyc_Interface_Ienv*_tmpC3;struct Cyc_Interface_Ienv*_tmpC2;_LL1: _tmpC2=env->f1;_tmpC3=env->f2;_tmpC4=env->f3;_tmpC5=env->f4;_tmpC6=env->f5;_LL2: {struct Cyc_Interface_Ienv*imp=_tmpC2;struct Cyc_Interface_Ienv*exp=_tmpC3;int check_complete_defs=_tmpC4;struct Cyc_Tcenv_Genv*ae=_tmpC5;struct Cyc_Interface_Seen*seen=_tmpC6;
struct Cyc_Absyn_Enumdecl*d=*dp;
enum Cyc_Absyn_Scope _stmttmp3=d->sc;enum Cyc_Absyn_Scope _tmpC7=_stmttmp3;switch(_tmpC7){case Cyc_Absyn_Static: _LL4: _LL5:
# 397
 if(check_complete_defs && d->fields == 0)
({({struct _fat_ptr _tmp35B=({const char*_tmpC8="static";_tag_fat(_tmpC8,sizeof(char),7U);});struct _fat_ptr _tmp35A=({const char*_tmpC9="enum";_tag_fat(_tmpC9,sizeof(char),5U);});Cyc_Interface_fields_err(_tmp35B,_tmp35A,d->name);});});
# 397
goto _LL3;case Cyc_Absyn_Abstract: _LL6: _LL7:
# 401
 if(d->fields == 0){
if(check_complete_defs)
({({struct _fat_ptr _tmp35D=({const char*_tmpCA="abstract";_tag_fat(_tmpCA,sizeof(char),9U);});struct _fat_ptr _tmp35C=({const char*_tmpCB="enum";_tag_fat(_tmpCB,sizeof(char),5U);});Cyc_Interface_fields_err(_tmp35D,_tmp35C,d->name);});});}else{
# 405
d=({struct Cyc_Absyn_Enumdecl*_tmpCC=_cycalloc(sizeof(*_tmpCC));*_tmpCC=*d;_tmpCC;});
d->fields=0;}
# 408
if(({Cyc_Interface_check_public_enumdecl(ae,seen,d);}))
({struct Cyc_Dict_Dict _tmp361=({({struct Cyc_Dict_Dict(*_tmp360)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Enumdecl*v)=({struct Cyc_Dict_Dict(*_tmpCD)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Enumdecl*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Enumdecl*v))Cyc_Dict_insert;_tmpCD;});struct Cyc_Dict_Dict _tmp35F=exp->enumdecls;struct _tuple0*_tmp35E=x;_tmp360(_tmp35F,_tmp35E,d);});});exp->enumdecls=_tmp361;});
# 408
goto _LL3;case Cyc_Absyn_Public: _LL8: _LL9:
# 412
 if(d->fields == 0){
({({struct _fat_ptr _tmp363=({const char*_tmpCE="public";_tag_fat(_tmpCE,sizeof(char),7U);});struct _fat_ptr _tmp362=({const char*_tmpCF="enum";_tag_fat(_tmpCF,sizeof(char),5U);});Cyc_Interface_fields_err(_tmp363,_tmp362,d->name);});});
d=({struct Cyc_Absyn_Enumdecl*_tmpD0=_cycalloc(sizeof(*_tmpD0));*_tmpD0=*d;_tmpD0;});
d->sc=Cyc_Absyn_Abstract;}
# 412
if(({Cyc_Interface_check_public_enumdecl(ae,seen,d);}))
# 418
({struct Cyc_Dict_Dict _tmp367=({({struct Cyc_Dict_Dict(*_tmp366)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Enumdecl*v)=({struct Cyc_Dict_Dict(*_tmpD1)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Enumdecl*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Enumdecl*v))Cyc_Dict_insert;_tmpD1;});struct Cyc_Dict_Dict _tmp365=exp->enumdecls;struct _tuple0*_tmp364=x;_tmp366(_tmp365,_tmp364,d);});});exp->enumdecls=_tmp367;});
# 412
goto _LL3;case Cyc_Absyn_ExternC: _LLA: _LLB:
# 420
 goto _LLD;case Cyc_Absyn_Extern: _LLC: _LLD:
# 422
 if(({Cyc_Interface_check_public_enumdecl(ae,seen,d);}))
({struct Cyc_Dict_Dict _tmp36B=({({struct Cyc_Dict_Dict(*_tmp36A)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Enumdecl*v)=({struct Cyc_Dict_Dict(*_tmpD2)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Enumdecl*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Enumdecl*v))Cyc_Dict_insert;_tmpD2;});struct Cyc_Dict_Dict _tmp369=imp->enumdecls;struct _tuple0*_tmp368=x;_tmp36A(_tmp369,_tmp368,d);});});imp->enumdecls=_tmp36B;});
# 422
goto _LL3;case Cyc_Absyn_Register: _LLE: _LLF:
# 425
 goto _LL11;default: _LL10: _LL11:
# 427
({({int(*_tmp36C)(struct _fat_ptr s)=({int(*_tmpD3)(struct _fat_ptr s)=(int(*)(struct _fat_ptr s))Cyc_Interface_invalid_arg;_tmpD3;});_tmp36C(({const char*_tmpD4="add_enumdecl";_tag_fat(_tmpD4,sizeof(char),13U);}));});});
goto _LL3;}_LL3:;}}
# 433
static void Cyc_Interface_extract_xdatatypefielddecl(struct Cyc_Interface_I*i,struct Cyc_Absyn_Datatypedecl*d,struct Cyc_Absyn_Datatypefield*f){
struct Cyc_Interface_Ienv*env;
{enum Cyc_Absyn_Scope _stmttmp4=f->sc;enum Cyc_Absyn_Scope _tmpD6=_stmttmp4;switch(_tmpD6){case Cyc_Absyn_Static: _LL1: _LL2:
 return;case Cyc_Absyn_Extern: _LL3: _LL4:
 env=i->imports;goto _LL0;case Cyc_Absyn_Public: _LL5: _LL6:
 env=i->exports;goto _LL0;default: _LL7: _LL8:
({({int(*_tmp36D)(struct _fat_ptr s)=({int(*_tmpD7)(struct _fat_ptr s)=(int(*)(struct _fat_ptr s))Cyc_Interface_invalid_arg;_tmpD7;});_tmp36D(({const char*_tmpD8="add_xdatatypefielddecl";_tag_fat(_tmpD8,sizeof(char),23U);}));});});}_LL0:;}
# 442
({struct Cyc_Dict_Dict _tmp371=({({struct Cyc_Dict_Dict(*_tmp370)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Tcdecl_Xdatatypefielddecl*v)=({
struct Cyc_Dict_Dict(*_tmpD9)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Tcdecl_Xdatatypefielddecl*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Tcdecl_Xdatatypefielddecl*v))Cyc_Dict_insert;_tmpD9;});struct Cyc_Dict_Dict _tmp36F=env->xdatatypefielddecls;struct _tuple0*_tmp36E=f->name;_tmp370(_tmp36F,_tmp36E,({struct Cyc_Tcdecl_Xdatatypefielddecl*_tmpDA=_cycalloc(sizeof(*_tmpDA));_tmpDA->base=d,_tmpDA->field=f;_tmpDA;}));});});
# 442
env->xdatatypefielddecls=_tmp371;});}
# 447
static void Cyc_Interface_extract_datatypedecl(struct _tuple26*env,struct _tuple0*x,struct Cyc_Absyn_Datatypedecl**dp){
# 449
struct Cyc_Interface_I*_tmpE1;struct Cyc_Interface_Seen*_tmpE0;struct Cyc_Tcenv_Genv*_tmpDF;int _tmpDE;struct Cyc_Interface_Ienv*_tmpDD;struct Cyc_Interface_Ienv*_tmpDC;_LL1: _tmpDC=env->f1;_tmpDD=env->f2;_tmpDE=env->f3;_tmpDF=env->f4;_tmpE0=env->f5;_tmpE1=env->f6;_LL2: {struct Cyc_Interface_Ienv*imp=_tmpDC;struct Cyc_Interface_Ienv*exp=_tmpDD;int check_complete_defs=_tmpDE;struct Cyc_Tcenv_Genv*ae=_tmpDF;struct Cyc_Interface_Seen*seen=_tmpE0;struct Cyc_Interface_I*i=_tmpE1;
struct Cyc_Absyn_Datatypedecl*d=*dp;
# 452
enum Cyc_Absyn_Scope _stmttmp5=d->sc;enum Cyc_Absyn_Scope _tmpE2=_stmttmp5;switch(_tmpE2){case Cyc_Absyn_Static: _LL4: _LL5:
# 454
 if((!d->is_extensible && check_complete_defs)&& d->fields == 0)
({({struct _fat_ptr _tmp373=({const char*_tmpE3="static";_tag_fat(_tmpE3,sizeof(char),7U);});struct _fat_ptr _tmp372=({const char*_tmpE4="datatype";_tag_fat(_tmpE4,sizeof(char),9U);});Cyc_Interface_fields_err(_tmp373,_tmp372,d->name);});});
# 454
goto _LL3;case Cyc_Absyn_Abstract: _LL6: _LL7:
# 459
 if(d->fields == 0){
if(!d->is_extensible && check_complete_defs)
({({struct _fat_ptr _tmp375=({const char*_tmpE5="abstract";_tag_fat(_tmpE5,sizeof(char),9U);});struct _fat_ptr _tmp374=({const char*_tmpE6="datatype";_tag_fat(_tmpE6,sizeof(char),9U);});Cyc_Interface_fields_err(_tmp375,_tmp374,d->name);});});}else{
# 464
d=({struct Cyc_Absyn_Datatypedecl*_tmpE7=_cycalloc(sizeof(*_tmpE7));*_tmpE7=*d;_tmpE7;});
d->fields=0;}
# 467
if(({Cyc_Interface_check_public_datatypedecl(ae,seen,d);}))
({struct Cyc_Dict_Dict _tmp379=({({struct Cyc_Dict_Dict(*_tmp378)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Datatypedecl*v)=({struct Cyc_Dict_Dict(*_tmpE8)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Datatypedecl*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Datatypedecl*v))Cyc_Dict_insert;_tmpE8;});struct Cyc_Dict_Dict _tmp377=exp->datatypedecls;struct _tuple0*_tmp376=x;_tmp378(_tmp377,_tmp376,d);});});exp->datatypedecls=_tmp379;});
# 467
goto _LL3;case Cyc_Absyn_Public: _LL8: _LL9:
# 471
 d=({struct Cyc_Absyn_Datatypedecl*_tmpE9=_cycalloc(sizeof(*_tmpE9));*_tmpE9=*d;_tmpE9;});
if(!d->is_extensible && d->fields == 0){
({({struct _fat_ptr _tmp37B=({const char*_tmpEA="public";_tag_fat(_tmpEA,sizeof(char),7U);});struct _fat_ptr _tmp37A=({const char*_tmpEB="datatype";_tag_fat(_tmpEB,sizeof(char),9U);});Cyc_Interface_fields_err(_tmp37B,_tmp37A,d->name);});});
d->sc=Cyc_Absyn_Abstract;}
# 472
if(({Cyc_Interface_check_public_datatypedecl(ae,seen,d);})){
# 477
if(d->is_extensible && d->fields != 0){
struct Cyc_List_List*fields=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(d->fields))->v;
d->fields=0;{
struct Cyc_List_List*f=fields;for(0;f != 0;f=f->tl){
({Cyc_Interface_extract_xdatatypefielddecl(i,d,(struct Cyc_Absyn_Datatypefield*)f->hd);});}}}
# 477
({struct Cyc_Dict_Dict _tmp37F=({({struct Cyc_Dict_Dict(*_tmp37E)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Datatypedecl*v)=({
# 484
struct Cyc_Dict_Dict(*_tmpEC)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Datatypedecl*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Datatypedecl*v))Cyc_Dict_insert;_tmpEC;});struct Cyc_Dict_Dict _tmp37D=exp->datatypedecls;struct _tuple0*_tmp37C=x;_tmp37E(_tmp37D,_tmp37C,d);});});
# 477
exp->datatypedecls=_tmp37F;});}
# 472
goto _LL3;case Cyc_Absyn_ExternC: _LLA: _LLB:
# 487
({({int(*_tmp380)(struct _fat_ptr s)=({int(*_tmpED)(struct _fat_ptr s)=(int(*)(struct _fat_ptr s))Cyc_Interface_invalid_arg;_tmpED;});_tmp380(({const char*_tmpEE="extract_datatypedecl";_tag_fat(_tmpEE,sizeof(char),21U);}));});});case Cyc_Absyn_Extern: _LLC: _LLD:
# 489
 if(({Cyc_Interface_check_public_datatypedecl(ae,seen,d);})){
if(d->is_extensible && d->fields != 0){
d=({struct Cyc_Absyn_Datatypedecl*_tmpEF=_cycalloc(sizeof(*_tmpEF));*_tmpEF=*d;_tmpEF;});{
struct Cyc_List_List*fields=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(d->fields))->v;
d->fields=0;{
struct Cyc_List_List*f=fields;for(0;f != 0;f=f->tl){
({Cyc_Interface_extract_xdatatypefielddecl(i,d,(struct Cyc_Absyn_Datatypefield*)f->hd);});}}}}
# 490
({struct Cyc_Dict_Dict _tmp384=({({struct Cyc_Dict_Dict(*_tmp383)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Datatypedecl*v)=({
# 498
struct Cyc_Dict_Dict(*_tmpF0)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Datatypedecl*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Datatypedecl*v))Cyc_Dict_insert;_tmpF0;});struct Cyc_Dict_Dict _tmp382=imp->datatypedecls;struct _tuple0*_tmp381=x;_tmp383(_tmp382,_tmp381,d);});});
# 490
imp->datatypedecls=_tmp384;});}
# 489
goto _LL3;case Cyc_Absyn_Register: _LLE: _LLF:
# 501
 goto _LL11;default: _LL10: _LL11:
# 503
({({int(*_tmp385)(struct _fat_ptr s)=({int(*_tmpF1)(struct _fat_ptr s)=(int(*)(struct _fat_ptr s))Cyc_Interface_invalid_arg;_tmpF1;});_tmp385(({const char*_tmpF2="add_datatypedecl";_tag_fat(_tmpF2,sizeof(char),17U);}));});});
goto _LL3;}_LL3:;}}
# 508
static void Cyc_Interface_extract_typedef(struct _tuple26*env,struct _tuple0*x,struct Cyc_Absyn_Typedefdecl*d){
# 510
struct Cyc_Interface_Seen*_tmpF6;struct Cyc_Tcenv_Genv*_tmpF5;struct Cyc_Interface_Ienv*_tmpF4;_LL1: _tmpF4=env->f2;_tmpF5=env->f4;_tmpF6=env->f5;_LL2: {struct Cyc_Interface_Ienv*exp=_tmpF4;struct Cyc_Tcenv_Genv*ae=_tmpF5;struct Cyc_Interface_Seen*seen=_tmpF6;
if(({Cyc_Interface_check_public_typedefdecl(ae,seen,d);}))
({struct Cyc_Dict_Dict _tmp389=({({struct Cyc_Dict_Dict(*_tmp388)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Typedefdecl*v)=({struct Cyc_Dict_Dict(*_tmpF7)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Typedefdecl*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Typedefdecl*v))Cyc_Dict_insert;_tmpF7;});struct Cyc_Dict_Dict _tmp387=exp->typedefdecls;struct _tuple0*_tmp386=x;_tmp388(_tmp387,_tmp386,d);});});exp->typedefdecls=_tmp389;});}}struct _tuple27{void*f1;int f2;};
# 515
static void Cyc_Interface_extract_ordinarie(struct _tuple26*env,struct _tuple0*x,struct _tuple27*v){
# 517
struct Cyc_Interface_Seen*_tmpFD;struct Cyc_Tcenv_Genv*_tmpFC;int _tmpFB;struct Cyc_Interface_Ienv*_tmpFA;struct Cyc_Interface_Ienv*_tmpF9;_LL1: _tmpF9=env->f1;_tmpFA=env->f2;_tmpFB=env->f3;_tmpFC=env->f4;_tmpFD=env->f5;_LL2: {struct Cyc_Interface_Ienv*imp=_tmpF9;struct Cyc_Interface_Ienv*exp=_tmpFA;int check_complete_defs=_tmpFB;struct Cyc_Tcenv_Genv*ae=_tmpFC;struct Cyc_Interface_Seen*seen=_tmpFD;
# 519
void*_stmttmp6=(*v).f1;void*_tmpFE=_stmttmp6;struct Cyc_Absyn_Vardecl*_tmpFF;struct Cyc_Absyn_Fndecl*_tmp100;switch(*((int*)_tmpFE)){case 2U: _LL4: _tmp100=((struct Cyc_Absyn_Funname_b_Absyn_Binding_struct*)_tmpFE)->f1;_LL5: {struct Cyc_Absyn_Fndecl*fd=_tmp100;
# 521
struct Cyc_Absyn_Vardecl*vd=({struct Cyc_Absyn_Vardecl*_tmp101=_cycalloc(sizeof(*_tmp101));_tmp101->sc=fd->sc,_tmp101->name=fd->name,_tmp101->varloc=0U,({
# 525
struct Cyc_Absyn_Tqual _tmp38A=({Cyc_Absyn_empty_tqual(0U);});_tmp101->tq=_tmp38A;}),_tmp101->type=(void*)_check_null(fd->cached_type),_tmp101->initializer=0,_tmp101->rgn=0,_tmp101->attributes=0,_tmp101->escapes=0;_tmp101;});
# 532
check_complete_defs=0;
_tmpFF=vd;goto _LL7;}case 1U: _LL6: _tmpFF=((struct Cyc_Absyn_Global_b_Absyn_Binding_struct*)_tmpFE)->f1;_LL7: {struct Cyc_Absyn_Vardecl*d=_tmpFF;
# 535
if(d->initializer != 0){
d=({struct Cyc_Absyn_Vardecl*_tmp102=_cycalloc(sizeof(*_tmp102));*_tmp102=*d;_tmp102;});
d->initializer=0;}
# 535
{enum Cyc_Absyn_Scope _stmttmp7=d->sc;enum Cyc_Absyn_Scope _tmp103=_stmttmp7;switch(_tmp103){case Cyc_Absyn_Static: _LLB: _LLC:
# 541
 if(check_complete_defs &&({Cyc_Tcutil_is_function_type(d->type);}))
({({struct _fat_ptr _tmp38B=({const char*_tmp104="static";_tag_fat(_tmp104,sizeof(char),7U);});Cyc_Interface_body_err(_tmp38B,d->name);});});
# 541
goto _LLA;case Cyc_Absyn_Register: _LLD: _LLE:
# 545
 goto _LL10;case Cyc_Absyn_Abstract: _LLF: _LL10:
({({int(*_tmp38C)(struct _fat_ptr s)=({int(*_tmp105)(struct _fat_ptr s)=(int(*)(struct _fat_ptr s))Cyc_Interface_invalid_arg;_tmp105;});_tmp38C(({const char*_tmp106="extract_ordinarie";_tag_fat(_tmp106,sizeof(char),18U);}));});});case Cyc_Absyn_Public: _LL11: _LL12:
# 548
 if(check_complete_defs &&({Cyc_Tcutil_is_function_type(d->type);}))
({({struct _fat_ptr _tmp38D=({const char*_tmp107="public";_tag_fat(_tmp107,sizeof(char),7U);});Cyc_Interface_body_err(_tmp38D,d->name);});});
# 548
if(({Cyc_Interface_check_public_vardecl(ae,seen,d);}))
# 552
({struct Cyc_Dict_Dict _tmp391=({({struct Cyc_Dict_Dict(*_tmp390)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Vardecl*v)=({struct Cyc_Dict_Dict(*_tmp108)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Vardecl*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Vardecl*v))Cyc_Dict_insert;_tmp108;});struct Cyc_Dict_Dict _tmp38F=exp->vardecls;struct _tuple0*_tmp38E=x;_tmp390(_tmp38F,_tmp38E,d);});});exp->vardecls=_tmp391;});
# 548
goto _LLA;case Cyc_Absyn_ExternC: _LL13: _LL14:
# 554
 goto _LL16;case Cyc_Absyn_Extern: _LL15: _LL16:
 goto _LL18;default: _LL17: _LL18:
# 557
 if(({Cyc_Interface_check_public_vardecl(ae,seen,d);}))
({struct Cyc_Dict_Dict _tmp395=({({struct Cyc_Dict_Dict(*_tmp394)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Vardecl*v)=({struct Cyc_Dict_Dict(*_tmp109)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Vardecl*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Vardecl*v))Cyc_Dict_insert;_tmp109;});struct Cyc_Dict_Dict _tmp393=imp->vardecls;struct _tuple0*_tmp392=x;_tmp394(_tmp393,_tmp392,d);});});imp->vardecls=_tmp395;});
# 557
goto _LLA;}_LLA:;}
# 561
goto _LL3;}default: _LL8: _LL9:
 goto _LL3;}_LL3:;}}struct _tuple28{void*f1;void*f2;};
# 566
static struct Cyc_List_List*Cyc_Interface_remove_decl_from_list(struct Cyc_List_List*l,struct Cyc_Absyn_Decl*d){
if(l == 0)return 0;{struct _tuple28 _stmttmp8=({struct _tuple28 _tmp2B7;_tmp2B7.f1=d->r,_tmp2B7.f2=((struct Cyc_Absyn_Decl*)l->hd)->r;_tmp2B7;});struct _tuple28 _tmp10B=_stmttmp8;struct Cyc_Absyn_Vardecl*_tmp10D;struct Cyc_Absyn_Vardecl**_tmp10C;struct Cyc_Absyn_Enumdecl*_tmp10F;struct Cyc_Absyn_Enumdecl*_tmp10E;struct Cyc_Absyn_Typedefdecl*_tmp111;struct Cyc_Absyn_Typedefdecl*_tmp110;switch(*((int*)_tmp10B.f1)){case 8U: if(((struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct*)_tmp10B.f2)->tag == 8U){_LL1: _tmp110=((struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct*)_tmp10B.f1)->f1;_tmp111=((struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct*)_tmp10B.f2)->f1;_LL2: {struct Cyc_Absyn_Typedefdecl*a1=_tmp110;struct Cyc_Absyn_Typedefdecl*a2=_tmp111;
# 570
if(({Cyc_Absyn_qvar_cmp(a1->name,a2->name);})!= 0)goto _LL0;return({Cyc_Interface_remove_decl_from_list(l->tl,d);});}}else{goto _LL7;}case 7U: if(((struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct*)_tmp10B.f2)->tag == 7U){_LL3: _tmp10E=((struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct*)_tmp10B.f1)->f1;_tmp10F=((struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct*)_tmp10B.f2)->f1;_LL4: {struct Cyc_Absyn_Enumdecl*a1=_tmp10E;struct Cyc_Absyn_Enumdecl*a2=_tmp10F;
# 573
if(({Cyc_Absyn_qvar_cmp(a1->name,a2->name);})!= 0)goto _LL0;if((int)a1->sc == (int)Cyc_Absyn_Extern)
a1->sc=a2->sc;
# 573
return({Cyc_Interface_remove_decl_from_list(l->tl,d);});
# 576
goto _LL0;}}else{goto _LL7;}case 0U: if(((struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_tmp10B.f2)->tag == 0U){_LL5: _tmp10C=(struct Cyc_Absyn_Vardecl**)&((struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_tmp10B.f1)->f1;_tmp10D=((struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_tmp10B.f2)->f1;_LL6: {struct Cyc_Absyn_Vardecl**vd1=_tmp10C;struct Cyc_Absyn_Vardecl*vd2=_tmp10D;
# 578
if(({Cyc_Absyn_qvar_cmp((*vd1)->name,vd2->name);})!= 0)goto _LL0;if((int)(*vd1)->sc == (int)Cyc_Absyn_Extern)
({struct Cyc_Absyn_Vardecl*_tmp396=({struct Cyc_Absyn_Vardecl*_tmp112=_cycalloc(sizeof(*_tmp112));*_tmp112=*vd2;_tmp112;});*vd1=_tmp396;});
# 578
return({Cyc_Interface_remove_decl_from_list(l->tl,d);});}}else{goto _LL7;}default: _LL7: _LL8:
# 582
 goto _LL0;}_LL0:;}
# 584
return({struct Cyc_List_List*_tmp113=_cycalloc(sizeof(*_tmp113));_tmp113->hd=(struct Cyc_Absyn_Decl*)l->hd,({struct Cyc_List_List*_tmp397=({Cyc_Interface_remove_decl_from_list(l->tl,d);});_tmp113->tl=_tmp397;});_tmp113;});}
# 587
static struct Cyc_List_List*Cyc_Interface_uniqify_decl_list(struct Cyc_List_List*accum,struct Cyc_Absyn_Decl*d){
if(accum == 0)return({struct Cyc_List_List*_tmp115=_cycalloc(sizeof(*_tmp115));_tmp115->hd=d,_tmp115->tl=0;_tmp115;});{struct Cyc_List_List*l;
# 590
for(l=accum;l != 0;l=l->tl){
struct _tuple28 _stmttmp9=({struct _tuple28 _tmp2B8;_tmp2B8.f1=d->r,_tmp2B8.f2=((struct Cyc_Absyn_Decl*)l->hd)->r;_tmp2B8;});struct _tuple28 _tmp116=_stmttmp9;struct Cyc_List_List*_tmp11A;struct _fat_ptr*_tmp119;struct Cyc_List_List**_tmp118;struct _fat_ptr*_tmp117;struct Cyc_Absyn_Enumdecl*_tmp11C;struct Cyc_Absyn_Enumdecl*_tmp11B;struct Cyc_Absyn_Typedefdecl*_tmp11E;struct Cyc_Absyn_Typedefdecl*_tmp11D;struct Cyc_Absyn_Vardecl**_tmp120;struct Cyc_Absyn_Vardecl*_tmp11F;switch(*((int*)_tmp116.f1)){case 0U: if(((struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_tmp116.f2)->tag == 0U){_LL1: _tmp11F=((struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_tmp116.f1)->f1;_tmp120=(struct Cyc_Absyn_Vardecl**)&((struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_tmp116.f2)->f1;_LL2: {struct Cyc_Absyn_Vardecl*vd1=_tmp11F;struct Cyc_Absyn_Vardecl**vd2=_tmp120;
# 593
if(({Cyc_Absyn_qvar_cmp(vd1->name,(*vd2)->name);})!= 0)goto _LL0;if((int)(*vd2)->sc == (int)Cyc_Absyn_Extern)
({struct Cyc_Absyn_Vardecl*_tmp398=({struct Cyc_Absyn_Vardecl*_tmp121=_cycalloc(sizeof(*_tmp121));*_tmp121=*vd1;_tmp121;});*vd2=_tmp398;});
# 593
return accum;}}else{goto _LL9;}case 8U: if(((struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct*)_tmp116.f2)->tag == 8U){_LL3: _tmp11D=((struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct*)_tmp116.f1)->f1;_tmp11E=((struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct*)_tmp116.f2)->f1;_LL4: {struct Cyc_Absyn_Typedefdecl*a1=_tmp11D;struct Cyc_Absyn_Typedefdecl*a2=_tmp11E;
# 597
if(({Cyc_Absyn_qvar_cmp(a1->name,a2->name);})!= 0)goto _LL0;return accum;}}else{goto _LL9;}case 7U: if(((struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct*)_tmp116.f2)->tag == 7U){_LL5: _tmp11B=((struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct*)_tmp116.f1)->f1;_tmp11C=((struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct*)_tmp116.f2)->f1;_LL6: {struct Cyc_Absyn_Enumdecl*a1=_tmp11B;struct Cyc_Absyn_Enumdecl*a2=_tmp11C;
# 600
if(({Cyc_Absyn_qvar_cmp(a1->name,a2->name);})!= 0)goto _LL0;return accum;}}else{goto _LL9;}case 9U: if(((struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct*)_tmp116.f2)->tag == 9U){_LL7: _tmp117=((struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct*)_tmp116.f1)->f1;_tmp118=(struct Cyc_List_List**)&((struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct*)_tmp116.f1)->f2;_tmp119=((struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct*)_tmp116.f2)->f1;_tmp11A=((struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct*)_tmp116.f2)->f2;_LL8: {struct _fat_ptr*a1=_tmp117;struct Cyc_List_List**b1=_tmp118;struct _fat_ptr*a2=_tmp119;struct Cyc_List_List*b2=_tmp11A;
# 603
if(({Cyc_strptrcmp(a1,a2);})!= 0)goto _LL0;{struct Cyc_List_List*dl=b2;
# 605
for(0;dl != 0;dl=dl->tl){
({struct Cyc_List_List*_tmp399=({Cyc_Interface_remove_decl_from_list(*b1,(struct Cyc_Absyn_Decl*)dl->hd);});*b1=_tmp399;});}
# 608
goto _LL0;}}}else{goto _LL9;}default: _LL9: _LLA:
# 610
 goto _LL0;}_LL0:;}
# 613
return({struct Cyc_List_List*_tmp122=_cycalloc(sizeof(*_tmp122));_tmp122->hd=d,_tmp122->tl=accum;_tmp122;});}}
# 616
static struct Cyc_List_List*Cyc_Interface_filterstatics(struct Cyc_List_List*accum,struct Cyc_Absyn_Decl*d){
{void*_stmttmpA=d->r;void*_tmp124=_stmttmpA;struct Cyc_List_List*_tmp125;struct Cyc_List_List*_tmp127;struct _fat_ptr*_tmp126;struct Cyc_Absyn_Typedefdecl*_tmp128;struct Cyc_Absyn_Enumdecl*_tmp129;struct Cyc_Absyn_Datatypedecl*_tmp12A;struct Cyc_Absyn_Aggrdecl*_tmp12B;struct Cyc_Absyn_Fndecl*_tmp12C;struct Cyc_Absyn_Vardecl*_tmp12D;switch(*((int*)_tmp124)){case 0U: _LL1: _tmp12D=((struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_tmp124)->f1;_LL2: {struct Cyc_Absyn_Vardecl*vd=_tmp12D;
# 619
if((int)vd->sc == (int)Cyc_Absyn_ExternC)return accum;if((int)vd->sc == (int)Cyc_Absyn_Static)
return accum;{
# 619
struct Cyc_Absyn_Vardecl*nvd=({struct Cyc_Absyn_Vardecl*_tmp130=_cycalloc(sizeof(*_tmp130));*_tmp130=*vd;_tmp130;});
# 622
nvd->initializer=0;
if(({Cyc_Tcutil_is_function_type(nvd->type);})&&(int)nvd->sc != (int)Cyc_Absyn_Extern)
nvd->sc=Cyc_Absyn_Extern;
# 623
return({struct Cyc_List_List*_tmp12F=_cycalloc(sizeof(*_tmp12F));
# 625
({struct Cyc_Absyn_Decl*_tmp39A=({Cyc_Absyn_new_decl((void*)({struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*_tmp12E=_cycalloc(sizeof(*_tmp12E));_tmp12E->tag=0U,_tmp12E->f1=nvd;_tmp12E;}),0U);});_tmp12F->hd=_tmp39A;}),_tmp12F->tl=accum;_tmp12F;});}}case 1U: _LL3: _tmp12C=((struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*)_tmp124)->f1;_LL4: {struct Cyc_Absyn_Fndecl*fd=_tmp12C;
# 627
if((int)fd->sc == (int)Cyc_Absyn_Static)return accum;if((int)fd->sc == (int)Cyc_Absyn_ExternC)
return accum;{
# 627
struct Cyc_Absyn_Vardecl*vd=({Cyc_Absyn_new_vardecl(0U,fd->name,(void*)_check_null(fd->cached_type),0);});
# 630
vd->sc=fd->sc;
return({struct Cyc_List_List*_tmp132=_cycalloc(sizeof(*_tmp132));({struct Cyc_Absyn_Decl*_tmp39B=({Cyc_Absyn_new_decl((void*)({struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*_tmp131=_cycalloc(sizeof(*_tmp131));_tmp131->tag=0U,_tmp131->f1=vd;_tmp131;}),0U);});_tmp132->hd=_tmp39B;}),_tmp132->tl=accum;_tmp132;});}}case 5U: _LL5: _tmp12B=((struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct*)_tmp124)->f1;_LL6: {struct Cyc_Absyn_Aggrdecl*a=_tmp12B;
# 634
if((int)a->sc == (int)Cyc_Absyn_ExternC)return accum;goto _LL0;}case 6U: _LL7: _tmp12A=((struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct*)_tmp124)->f1;_LL8: {struct Cyc_Absyn_Datatypedecl*a=_tmp12A;
# 638
if((int)a->sc == (int)Cyc_Absyn_ExternC)return accum;goto _LL0;}case 7U: _LL9: _tmp129=((struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct*)_tmp124)->f1;_LLA: {struct Cyc_Absyn_Enumdecl*a=_tmp129;
# 641
if((int)a->sc == (int)Cyc_Absyn_Static)return accum;if((int)a->sc == (int)Cyc_Absyn_ExternC)
return accum;
# 641
goto _LL0;}case 8U: _LLB: _tmp128=((struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct*)_tmp124)->f1;_LLC: {struct Cyc_Absyn_Typedefdecl*a=_tmp128;
# 645
goto _LL0;}case 9U: _LLD: _tmp126=((struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct*)_tmp124)->f1;_tmp127=((struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct*)_tmp124)->f2;_LLE: {struct _fat_ptr*a=_tmp126;struct Cyc_List_List*b=_tmp127;
# 647
struct Cyc_List_List*l=({struct Cyc_List_List*(*_tmp135)(struct Cyc_List_List*x)=Cyc_List_rev;struct Cyc_List_List*_tmp136=({({struct Cyc_List_List*(*_tmp39C)(struct Cyc_List_List*(*f)(struct Cyc_List_List*,struct Cyc_Absyn_Decl*),struct Cyc_List_List*accum,struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp137)(struct Cyc_List_List*(*f)(struct Cyc_List_List*,struct Cyc_Absyn_Decl*),struct Cyc_List_List*accum,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_List_List*(*f)(struct Cyc_List_List*,struct Cyc_Absyn_Decl*),struct Cyc_List_List*accum,struct Cyc_List_List*x))Cyc_List_fold_left;_tmp137;});_tmp39C(Cyc_Interface_filterstatics,0,b);});});_tmp135(_tmp136);});
return({struct Cyc_List_List*_tmp134=_cycalloc(sizeof(*_tmp134));({struct Cyc_Absyn_Decl*_tmp39D=({Cyc_Absyn_new_decl((void*)({struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct*_tmp133=_cycalloc(sizeof(*_tmp133));_tmp133->tag=9U,_tmp133->f1=a,_tmp133->f2=l;_tmp133;}),0U);});_tmp134->hd=_tmp39D;}),_tmp134->tl=accum;_tmp134;});}case 10U: _LLF: _tmp125=((struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct*)_tmp124)->f2;_LL10: {struct Cyc_List_List*b=_tmp125;
# 650
return({struct Cyc_List_List*(*_tmp138)(struct Cyc_List_List*x,struct Cyc_List_List*y)=Cyc_List_append;struct Cyc_List_List*_tmp139=({({struct Cyc_List_List*(*_tmp39E)(struct Cyc_List_List*(*f)(struct Cyc_List_List*,struct Cyc_Absyn_Decl*),struct Cyc_List_List*accum,struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp13A)(struct Cyc_List_List*(*f)(struct Cyc_List_List*,struct Cyc_Absyn_Decl*),struct Cyc_List_List*accum,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_List_List*(*f)(struct Cyc_List_List*,struct Cyc_Absyn_Decl*),struct Cyc_List_List*accum,struct Cyc_List_List*x))Cyc_List_fold_left;_tmp13A;});_tmp39E(Cyc_Interface_filterstatics,0,b);});});struct Cyc_List_List*_tmp13B=accum;_tmp138(_tmp139,_tmp13B);});}case 2U: _LL11: _LL12:
 goto _LL14;case 3U: _LL13: _LL14:
 goto _LL16;case 4U: _LL15: _LL16:
 goto _LL18;case 11U: _LL17: _LL18:
 goto _LL1A;case 12U: _LL19: _LL1A:
 goto _LL1C;case 13U: _LL1B: _LL1C:
 goto _LL1E;case 14U: _LL1D: _LL1E:
 goto _LL20;case 15U: _LL1F: _LL20:
 goto _LL22;default: _LL21: _LL22:
# 660
 return accum;}_LL0:;}
# 662
return({struct Cyc_List_List*_tmp13D=_cycalloc(sizeof(*_tmp13D));({struct Cyc_Absyn_Decl*_tmp39F=({struct Cyc_Absyn_Decl*_tmp13C=_cycalloc(sizeof(*_tmp13C));*_tmp13C=*d;_tmp13C;});_tmp13D->hd=_tmp39F;}),_tmp13D->tl=accum;_tmp13D;});}struct _tuple29{struct Cyc_Interface_I*f1;int f2;struct Cyc_Tcenv_Genv*f3;struct Cyc_Interface_Seen*f4;};
# 665
static void Cyc_Interface_extract_f(struct _tuple29*env_f){
struct Cyc_Interface_Seen*_tmp142;struct Cyc_Tcenv_Genv*_tmp141;int _tmp140;struct Cyc_Interface_I*_tmp13F;_LL1: _tmp13F=env_f->f1;_tmp140=env_f->f2;_tmp141=env_f->f3;_tmp142=env_f->f4;_LL2: {struct Cyc_Interface_I*i=_tmp13F;int check_complete_defs=_tmp140;struct Cyc_Tcenv_Genv*ae=_tmp141;struct Cyc_Interface_Seen*seen=_tmp142;
struct _tuple26 env=({struct _tuple26 _tmp2B9;_tmp2B9.f1=i->imports,_tmp2B9.f2=i->exports,_tmp2B9.f3=check_complete_defs,_tmp2B9.f4=ae,_tmp2B9.f5=seen,_tmp2B9.f6=i;_tmp2B9;});
({({void(*_tmp3A0)(void(*f)(struct _tuple26*,struct _tuple0*,struct Cyc_Absyn_Aggrdecl**),struct _tuple26*env,struct Cyc_Dict_Dict d)=({void(*_tmp143)(void(*f)(struct _tuple26*,struct _tuple0*,struct Cyc_Absyn_Aggrdecl**),struct _tuple26*env,struct Cyc_Dict_Dict d)=(void(*)(void(*f)(struct _tuple26*,struct _tuple0*,struct Cyc_Absyn_Aggrdecl**),struct _tuple26*env,struct Cyc_Dict_Dict d))Cyc_Dict_iter_c;_tmp143;});_tmp3A0(Cyc_Interface_extract_aggrdecl,& env,ae->aggrdecls);});});
({({void(*_tmp3A1)(void(*f)(struct _tuple26*,struct _tuple0*,struct Cyc_Absyn_Datatypedecl**),struct _tuple26*env,struct Cyc_Dict_Dict d)=({void(*_tmp144)(void(*f)(struct _tuple26*,struct _tuple0*,struct Cyc_Absyn_Datatypedecl**),struct _tuple26*env,struct Cyc_Dict_Dict d)=(void(*)(void(*f)(struct _tuple26*,struct _tuple0*,struct Cyc_Absyn_Datatypedecl**),struct _tuple26*env,struct Cyc_Dict_Dict d))Cyc_Dict_iter_c;_tmp144;});_tmp3A1(Cyc_Interface_extract_datatypedecl,& env,ae->datatypedecls);});});
({({void(*_tmp3A2)(void(*f)(struct _tuple26*,struct _tuple0*,struct Cyc_Absyn_Enumdecl**),struct _tuple26*env,struct Cyc_Dict_Dict d)=({void(*_tmp145)(void(*f)(struct _tuple26*,struct _tuple0*,struct Cyc_Absyn_Enumdecl**),struct _tuple26*env,struct Cyc_Dict_Dict d)=(void(*)(void(*f)(struct _tuple26*,struct _tuple0*,struct Cyc_Absyn_Enumdecl**),struct _tuple26*env,struct Cyc_Dict_Dict d))Cyc_Dict_iter_c;_tmp145;});_tmp3A2(Cyc_Interface_extract_enumdecl,& env,ae->enumdecls);});});
({({void(*_tmp3A3)(void(*f)(struct _tuple26*,struct _tuple0*,struct Cyc_Absyn_Typedefdecl*),struct _tuple26*env,struct Cyc_Dict_Dict d)=({void(*_tmp146)(void(*f)(struct _tuple26*,struct _tuple0*,struct Cyc_Absyn_Typedefdecl*),struct _tuple26*env,struct Cyc_Dict_Dict d)=(void(*)(void(*f)(struct _tuple26*,struct _tuple0*,struct Cyc_Absyn_Typedefdecl*),struct _tuple26*env,struct Cyc_Dict_Dict d))Cyc_Dict_iter_c;_tmp146;});_tmp3A3(Cyc_Interface_extract_typedef,& env,ae->typedefs);});});
({({void(*_tmp3A4)(void(*f)(struct _tuple26*,struct _tuple0*,struct _tuple27*),struct _tuple26*env,struct Cyc_Dict_Dict d)=({void(*_tmp147)(void(*f)(struct _tuple26*,struct _tuple0*,struct _tuple27*),struct _tuple26*env,struct Cyc_Dict_Dict d)=(void(*)(void(*f)(struct _tuple26*,struct _tuple0*,struct _tuple27*),struct _tuple26*env,struct Cyc_Dict_Dict d))Cyc_Dict_iter_c;_tmp147;});_tmp3A4(Cyc_Interface_extract_ordinarie,& env,ae->ordinaries);});});}}
# 675
static struct Cyc_Interface_I*Cyc_Interface_gen_extract(struct Cyc_Tcenv_Genv*ae,int check_complete_defs,struct Cyc_List_List*tds){
struct _tuple29 env=({struct _tuple29 _tmp2BA;({
struct Cyc_Interface_I*_tmp3A6=({Cyc_Interface_empty();});_tmp2BA.f1=_tmp3A6;}),_tmp2BA.f2=check_complete_defs,_tmp2BA.f3=ae,({struct Cyc_Interface_Seen*_tmp3A5=({Cyc_Interface_new_seen();});_tmp2BA.f4=_tmp3A5;});_tmp2BA;});
({Cyc_Interface_extract_f(& env);});{
struct Cyc_Interface_I*i=env.f1;
({struct Cyc_List_List*_tmp3A8=({struct Cyc_List_List*(*_tmp149)(struct Cyc_List_List*x)=Cyc_List_rev;struct Cyc_List_List*_tmp14A=({({struct Cyc_List_List*(*_tmp3A7)(struct Cyc_List_List*(*f)(struct Cyc_List_List*,struct Cyc_Absyn_Decl*),struct Cyc_List_List*accum,struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp14B)(struct Cyc_List_List*(*f)(struct Cyc_List_List*,struct Cyc_Absyn_Decl*),struct Cyc_List_List*accum,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_List_List*(*f)(struct Cyc_List_List*,struct Cyc_Absyn_Decl*),struct Cyc_List_List*accum,struct Cyc_List_List*x))Cyc_List_fold_left;_tmp14B;});_tmp3A7(Cyc_Interface_filterstatics,0,tds);});});_tmp149(_tmp14A);});i->tds=_tmp3A8;});
({struct Cyc_List_List*_tmp3AA=({struct Cyc_List_List*(*_tmp14C)(struct Cyc_List_List*x)=Cyc_List_rev;struct Cyc_List_List*_tmp14D=({({struct Cyc_List_List*(*_tmp3A9)(struct Cyc_List_List*(*f)(struct Cyc_List_List*,struct Cyc_Absyn_Decl*),struct Cyc_List_List*accum,struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp14E)(struct Cyc_List_List*(*f)(struct Cyc_List_List*,struct Cyc_Absyn_Decl*),struct Cyc_List_List*accum,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_List_List*(*f)(struct Cyc_List_List*,struct Cyc_Absyn_Decl*),struct Cyc_List_List*accum,struct Cyc_List_List*x))Cyc_List_fold_left;_tmp14E;});_tmp3A9(Cyc_Interface_uniqify_decl_list,0,i->tds);});});_tmp14C(_tmp14D);});i->tds=_tmp3AA;});
return i;}}
# 675
struct Cyc_Interface_I*Cyc_Interface_extract(struct Cyc_Tcenv_Genv*ae,struct Cyc_List_List*tds){
# 686
return({Cyc_Interface_gen_extract(ae,1,tds);});}
# 691
inline static void Cyc_Interface_check_err(struct _fat_ptr*msg1,struct _fat_ptr msg2){
({struct Cyc_String_pa_PrintArg_struct _tmp153=({struct Cyc_String_pa_PrintArg_struct _tmp2BB;_tmp2BB.tag=0U,_tmp2BB.f1=(struct _fat_ptr)((struct _fat_ptr)msg2);_tmp2BB;});void*_tmp151[1U];_tmp151[0]=& _tmp153;({struct _fat_ptr*_tmp3AC=msg1;struct _fat_ptr _tmp3AB=({const char*_tmp152="%s";_tag_fat(_tmp152,sizeof(char),3U);});Cyc_Tcdecl_merr(0U,_tmp3AC,_tmp3AB,_tag_fat(_tmp151,sizeof(void*),1U));});});}struct _tuple30{int f1;struct Cyc_Dict_Dict f2;int(*f3)(void*,void*,struct _fat_ptr*);struct _fat_ptr f4;struct _fat_ptr*f5;};
# 695
static void Cyc_Interface_incl_dict_f(struct _tuple30*env,struct _tuple0*x,void*y1){
# 702
struct _fat_ptr*_tmp159;struct _fat_ptr _tmp158;int(*_tmp157)(void*,void*,struct _fat_ptr*);struct Cyc_Dict_Dict _tmp156;int*_tmp155;_LL1: _tmp155=(int*)& env->f1;_tmp156=env->f2;_tmp157=env->f3;_tmp158=env->f4;_tmp159=env->f5;_LL2: {int*res=_tmp155;struct Cyc_Dict_Dict dic2=_tmp156;int(*incl_f)(void*,void*,struct _fat_ptr*)=_tmp157;struct _fat_ptr t=_tmp158;struct _fat_ptr*msg=_tmp159;
struct _handler_cons _tmp15A;_push_handler(& _tmp15A);{int _tmp15C=0;if(setjmp(_tmp15A.handler))_tmp15C=1;if(!_tmp15C){
{void*y2=({({void*(*_tmp3AE)(struct Cyc_Dict_Dict d,struct _tuple0*k)=({void*(*_tmp15D)(struct Cyc_Dict_Dict d,struct _tuple0*k)=(void*(*)(struct Cyc_Dict_Dict d,struct _tuple0*k))Cyc_Dict_lookup;_tmp15D;});struct Cyc_Dict_Dict _tmp3AD=dic2;_tmp3AE(_tmp3AD,x);});});
if(!({incl_f(y1,y2,msg);}))*res=0;}
# 704
;_pop_handler();}else{void*_tmp15B=(void*)Cyc_Core_get_exn_thrown();void*_tmp15E=_tmp15B;void*_tmp15F;if(((struct Cyc_Dict_Absent_exn_struct*)_tmp15E)->tag == Cyc_Dict_Absent){_LL4: _LL5:
# 708
({void(*_tmp160)(struct _fat_ptr*msg1,struct _fat_ptr msg2)=Cyc_Interface_check_err;struct _fat_ptr*_tmp161=msg;struct _fat_ptr _tmp162=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp165=({struct Cyc_String_pa_PrintArg_struct _tmp2BD;_tmp2BD.tag=0U,_tmp2BD.f1=(struct _fat_ptr)((struct _fat_ptr)t);_tmp2BD;});struct Cyc_String_pa_PrintArg_struct _tmp166=({struct Cyc_String_pa_PrintArg_struct _tmp2BC;_tmp2BC.tag=0U,({struct _fat_ptr _tmp3AF=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(x);}));_tmp2BC.f1=_tmp3AF;});_tmp2BC;});void*_tmp163[2U];_tmp163[0]=& _tmp165,_tmp163[1]=& _tmp166;({struct _fat_ptr _tmp3B0=({const char*_tmp164="%s %s is missing";_tag_fat(_tmp164,sizeof(char),17U);});Cyc_aprintf(_tmp3B0,_tag_fat(_tmp163,sizeof(void*),2U));});});_tmp160(_tmp161,_tmp162);});
*res=0;
goto _LL3;}else{_LL6: _tmp15F=_tmp15E;_LL7: {void*exn=_tmp15F;(int)_rethrow(exn);}}_LL3:;}}}}
# 714
static int Cyc_Interface_incl_dict(struct Cyc_Dict_Dict dic1,struct Cyc_Dict_Dict dic2,int(*incl_f)(void*,void*,struct _fat_ptr*),struct _fat_ptr t,struct _fat_ptr*msg){
# 719
struct _tuple30 env=({struct _tuple30 _tmp2BE;_tmp2BE.f1=1,_tmp2BE.f2=dic2,_tmp2BE.f3=incl_f,_tmp2BE.f4=t,_tmp2BE.f5=msg;_tmp2BE;});
({({void(*_tmp3B1)(void(*f)(struct _tuple30*,struct _tuple0*,void*),struct _tuple30*env,struct Cyc_Dict_Dict d)=({void(*_tmp168)(void(*f)(struct _tuple30*,struct _tuple0*,void*),struct _tuple30*env,struct Cyc_Dict_Dict d)=(void(*)(void(*f)(struct _tuple30*,struct _tuple0*,void*),struct _tuple30*env,struct Cyc_Dict_Dict d))Cyc_Dict_iter_c;_tmp168;});_tmp3B1(Cyc_Interface_incl_dict_f,& env,dic1);});});
return env.f1;}
# 736 "interface.cyc"
static int Cyc_Interface_incl_aggrdecl(struct Cyc_Absyn_Aggrdecl*d0,struct Cyc_Absyn_Aggrdecl*d1,struct _fat_ptr*msg){struct Cyc_Absyn_Aggrdecl*d=({Cyc_Tcdecl_merge_aggrdecl(d0,d1,0U,msg);});if(d == 0)return 0;if(d0 != d){({void(*_tmp16A)(struct _fat_ptr*msg1,struct _fat_ptr msg2)=Cyc_Interface_check_err;struct _fat_ptr*_tmp16B=msg;struct _fat_ptr _tmp16C=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp16F=({struct Cyc_String_pa_PrintArg_struct _tmp2BF;_tmp2BF.tag=0U,({struct _fat_ptr _tmp3B2=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(d1->name);}));_tmp2BF.f1=_tmp3B2;});_tmp2BF;});void*_tmp16D[1U];_tmp16D[0]=& _tmp16F;({struct _fat_ptr _tmp3B3=({const char*_tmp16E="declaration of type %s discloses too much information";_tag_fat(_tmp16E,sizeof(char),54U);});Cyc_aprintf(_tmp3B3,_tag_fat(_tmp16D,sizeof(void*),1U));});});_tmp16A(_tmp16B,_tmp16C);});return 0;}return 1;}
# 738
static int Cyc_Interface_incl_datatypedecl(struct Cyc_Absyn_Datatypedecl*d0,struct Cyc_Absyn_Datatypedecl*d1,struct _fat_ptr*msg){struct Cyc_Absyn_Datatypedecl*d=({Cyc_Tcdecl_merge_datatypedecl(d0,d1,0U,msg);});if(d == 0)return 0;if(d0 != d){({void(*_tmp171)(struct _fat_ptr*msg1,struct _fat_ptr msg2)=Cyc_Interface_check_err;struct _fat_ptr*_tmp172=msg;struct _fat_ptr _tmp173=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp176=({struct Cyc_String_pa_PrintArg_struct _tmp2C0;_tmp2C0.tag=0U,({struct _fat_ptr _tmp3B4=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(d1->name);}));_tmp2C0.f1=_tmp3B4;});_tmp2C0;});void*_tmp174[1U];_tmp174[0]=& _tmp176;({struct _fat_ptr _tmp3B5=({const char*_tmp175="declaration of datatype %s discloses too much information";_tag_fat(_tmp175,sizeof(char),58U);});Cyc_aprintf(_tmp3B5,_tag_fat(_tmp174,sizeof(void*),1U));});});_tmp171(_tmp172,_tmp173);});return 0;}return 1;}
# 740
static int Cyc_Interface_incl_enumdecl(struct Cyc_Absyn_Enumdecl*d0,struct Cyc_Absyn_Enumdecl*d1,struct _fat_ptr*msg){struct Cyc_Absyn_Enumdecl*d=({Cyc_Tcdecl_merge_enumdecl(d0,d1,0U,msg);});if(d == 0)return 0;if(d0 != d){({void(*_tmp178)(struct _fat_ptr*msg1,struct _fat_ptr msg2)=Cyc_Interface_check_err;struct _fat_ptr*_tmp179=msg;struct _fat_ptr _tmp17A=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp17D=({struct Cyc_String_pa_PrintArg_struct _tmp2C1;_tmp2C1.tag=0U,({struct _fat_ptr _tmp3B6=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(d1->name);}));_tmp2C1.f1=_tmp3B6;});_tmp2C1;});void*_tmp17B[1U];_tmp17B[0]=& _tmp17D;({struct _fat_ptr _tmp3B7=({const char*_tmp17C="declaration of enum %s discloses too much information";_tag_fat(_tmp17C,sizeof(char),54U);});Cyc_aprintf(_tmp3B7,_tag_fat(_tmp17B,sizeof(void*),1U));});});_tmp178(_tmp179,_tmp17A);});return 0;}return 1;}
# 742
static int Cyc_Interface_incl_vardecl(struct Cyc_Absyn_Vardecl*d0,struct Cyc_Absyn_Vardecl*d1,struct _fat_ptr*msg){struct Cyc_Absyn_Vardecl*d=({Cyc_Tcdecl_merge_vardecl(d0,d1,0U,msg);});if(d == 0)return 0;if(d0 != d){({void(*_tmp17F)(struct _fat_ptr*msg1,struct _fat_ptr msg2)=Cyc_Interface_check_err;struct _fat_ptr*_tmp180=msg;struct _fat_ptr _tmp181=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp184=({struct Cyc_String_pa_PrintArg_struct _tmp2C2;_tmp2C2.tag=0U,({struct _fat_ptr _tmp3B8=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(d1->name);}));_tmp2C2.f1=_tmp3B8;});_tmp2C2;});void*_tmp182[1U];_tmp182[0]=& _tmp184;({struct _fat_ptr _tmp3B9=({const char*_tmp183="declaration of variable %s discloses too much information";_tag_fat(_tmp183,sizeof(char),58U);});Cyc_aprintf(_tmp3B9,_tag_fat(_tmp182,sizeof(void*),1U));});});_tmp17F(_tmp180,_tmp181);});return 0;}return 1;}
# 744
static int Cyc_Interface_incl_typedefdecl(struct Cyc_Absyn_Typedefdecl*d0,struct Cyc_Absyn_Typedefdecl*d1,struct _fat_ptr*msg){struct Cyc_Absyn_Typedefdecl*d=({Cyc_Tcdecl_merge_typedefdecl(d0,d1,0U,msg);});if(d == 0)return 0;if(d0 != d){({void(*_tmp186)(struct _fat_ptr*msg1,struct _fat_ptr msg2)=Cyc_Interface_check_err;struct _fat_ptr*_tmp187=msg;struct _fat_ptr _tmp188=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp18B=({struct Cyc_String_pa_PrintArg_struct _tmp2C3;_tmp2C3.tag=0U,({struct _fat_ptr _tmp3BA=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(d1->name);}));_tmp2C3.f1=_tmp3BA;});_tmp2C3;});void*_tmp189[1U];_tmp189[0]=& _tmp18B;({struct _fat_ptr _tmp3BB=({const char*_tmp18A="declaration of typedef %s discloses too much information";_tag_fat(_tmp18A,sizeof(char),57U);});Cyc_aprintf(_tmp3BB,_tag_fat(_tmp189,sizeof(void*),1U));});});_tmp186(_tmp187,_tmp188);});return 0;}return 1;}
# 746
static int Cyc_Interface_incl_xdatatypefielddecl(struct Cyc_Tcdecl_Xdatatypefielddecl*d0,struct Cyc_Tcdecl_Xdatatypefielddecl*d1,struct _fat_ptr*msg){struct Cyc_Tcdecl_Xdatatypefielddecl*d=({Cyc_Tcdecl_merge_xdatatypefielddecl(d0,d1,0U,msg);});if(d == 0)return 0;if(d0 != d){({void(*_tmp18D)(struct _fat_ptr*msg1,struct _fat_ptr msg2)=Cyc_Interface_check_err;struct _fat_ptr*_tmp18E=msg;struct _fat_ptr _tmp18F=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp192=({struct Cyc_String_pa_PrintArg_struct _tmp2C4;_tmp2C4.tag=0U,({struct _fat_ptr _tmp3BC=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string((d1->field)->name);}));_tmp2C4.f1=_tmp3BC;});_tmp2C4;});void*_tmp190[1U];_tmp190[0]=& _tmp192;({struct _fat_ptr _tmp3BD=({const char*_tmp191="declaration of xdatatypefield %s discloses too much information";_tag_fat(_tmp191,sizeof(char),64U);});Cyc_aprintf(_tmp3BD,_tag_fat(_tmp190,sizeof(void*),1U));});});_tmp18D(_tmp18E,_tmp18F);});return 0;}return 1;}struct Cyc_Dict_Dict*Cyc_Interface_compat_merge_dict(struct Cyc_Dict_Dict dic1,struct Cyc_Dict_Dict dic2,struct Cyc_Dict_Dict excl,void*(*merge_f)(void*,void*,unsigned,struct _fat_ptr*),struct _fat_ptr t,struct _fat_ptr*msg);
# 760
static int Cyc_Interface_incl_ienv(struct Cyc_Interface_Ienv*ie1,struct Cyc_Interface_Ienv*ie2,struct _fat_ptr*msg){
int r1=({({int(*_tmp3C1)(struct Cyc_Dict_Dict dic1,struct Cyc_Dict_Dict dic2,int(*incl_f)(struct Cyc_Absyn_Aggrdecl*,struct Cyc_Absyn_Aggrdecl*,struct _fat_ptr*),struct _fat_ptr t,struct _fat_ptr*msg)=({int(*_tmp1A6)(struct Cyc_Dict_Dict dic1,struct Cyc_Dict_Dict dic2,int(*incl_f)(struct Cyc_Absyn_Aggrdecl*,struct Cyc_Absyn_Aggrdecl*,struct _fat_ptr*),struct _fat_ptr t,struct _fat_ptr*msg)=(int(*)(struct Cyc_Dict_Dict dic1,struct Cyc_Dict_Dict dic2,int(*incl_f)(struct Cyc_Absyn_Aggrdecl*,struct Cyc_Absyn_Aggrdecl*,struct _fat_ptr*),struct _fat_ptr t,struct _fat_ptr*msg))Cyc_Interface_incl_dict;_tmp1A6;});struct Cyc_Dict_Dict _tmp3C0=ie1->aggrdecls;struct Cyc_Dict_Dict _tmp3BF=ie2->aggrdecls;struct _fat_ptr _tmp3BE=({const char*_tmp1A7="type";_tag_fat(_tmp1A7,sizeof(char),5U);});_tmp3C1(_tmp3C0,_tmp3BF,Cyc_Interface_incl_aggrdecl,_tmp3BE,msg);});});
int r2=({({int(*_tmp3C5)(struct Cyc_Dict_Dict dic1,struct Cyc_Dict_Dict dic2,int(*incl_f)(struct Cyc_Absyn_Datatypedecl*,struct Cyc_Absyn_Datatypedecl*,struct _fat_ptr*),struct _fat_ptr t,struct _fat_ptr*msg)=({int(*_tmp1A4)(struct Cyc_Dict_Dict dic1,struct Cyc_Dict_Dict dic2,int(*incl_f)(struct Cyc_Absyn_Datatypedecl*,struct Cyc_Absyn_Datatypedecl*,struct _fat_ptr*),struct _fat_ptr t,struct _fat_ptr*msg)=(int(*)(struct Cyc_Dict_Dict dic1,struct Cyc_Dict_Dict dic2,int(*incl_f)(struct Cyc_Absyn_Datatypedecl*,struct Cyc_Absyn_Datatypedecl*,struct _fat_ptr*),struct _fat_ptr t,struct _fat_ptr*msg))Cyc_Interface_incl_dict;_tmp1A4;});struct Cyc_Dict_Dict _tmp3C4=ie1->datatypedecls;struct Cyc_Dict_Dict _tmp3C3=ie2->datatypedecls;struct _fat_ptr _tmp3C2=({const char*_tmp1A5="datatype";_tag_fat(_tmp1A5,sizeof(char),9U);});_tmp3C5(_tmp3C4,_tmp3C3,Cyc_Interface_incl_datatypedecl,_tmp3C2,msg);});});
int r3=({({int(*_tmp3C9)(struct Cyc_Dict_Dict dic1,struct Cyc_Dict_Dict dic2,int(*incl_f)(struct Cyc_Absyn_Enumdecl*,struct Cyc_Absyn_Enumdecl*,struct _fat_ptr*),struct _fat_ptr t,struct _fat_ptr*msg)=({int(*_tmp1A2)(struct Cyc_Dict_Dict dic1,struct Cyc_Dict_Dict dic2,int(*incl_f)(struct Cyc_Absyn_Enumdecl*,struct Cyc_Absyn_Enumdecl*,struct _fat_ptr*),struct _fat_ptr t,struct _fat_ptr*msg)=(int(*)(struct Cyc_Dict_Dict dic1,struct Cyc_Dict_Dict dic2,int(*incl_f)(struct Cyc_Absyn_Enumdecl*,struct Cyc_Absyn_Enumdecl*,struct _fat_ptr*),struct _fat_ptr t,struct _fat_ptr*msg))Cyc_Interface_incl_dict;_tmp1A2;});struct Cyc_Dict_Dict _tmp3C8=ie1->enumdecls;struct Cyc_Dict_Dict _tmp3C7=ie2->enumdecls;struct _fat_ptr _tmp3C6=({const char*_tmp1A3="enum";_tag_fat(_tmp1A3,sizeof(char),5U);});_tmp3C9(_tmp3C8,_tmp3C7,Cyc_Interface_incl_enumdecl,_tmp3C6,msg);});});
# 765
int r4=({struct Cyc_Dict_Dict*(*_tmp198)(struct Cyc_Dict_Dict dic1,struct Cyc_Dict_Dict dic2,struct Cyc_Dict_Dict excl,struct Cyc_Absyn_Typedefdecl*(*merge_f)(struct Cyc_Absyn_Typedefdecl*,struct Cyc_Absyn_Typedefdecl*,unsigned,struct _fat_ptr*),struct _fat_ptr t,struct _fat_ptr*msg)=({struct Cyc_Dict_Dict*(*_tmp199)(struct Cyc_Dict_Dict dic1,struct Cyc_Dict_Dict dic2,struct Cyc_Dict_Dict excl,struct Cyc_Absyn_Typedefdecl*(*merge_f)(struct Cyc_Absyn_Typedefdecl*,struct Cyc_Absyn_Typedefdecl*,unsigned,struct _fat_ptr*),struct _fat_ptr t,struct _fat_ptr*msg)=(struct Cyc_Dict_Dict*(*)(struct Cyc_Dict_Dict dic1,struct Cyc_Dict_Dict dic2,struct Cyc_Dict_Dict excl,struct Cyc_Absyn_Typedefdecl*(*merge_f)(struct Cyc_Absyn_Typedefdecl*,struct Cyc_Absyn_Typedefdecl*,unsigned,struct _fat_ptr*),struct _fat_ptr t,struct _fat_ptr*msg))Cyc_Interface_compat_merge_dict;_tmp199;});struct Cyc_Dict_Dict _tmp19A=ie1->typedefdecls;struct Cyc_Dict_Dict _tmp19B=ie2->typedefdecls;struct Cyc_Dict_Dict _tmp19C=({({
struct Cyc_Dict_Dict(*_tmp19D)(int(*cmp)(struct _tuple0*,struct _tuple0*))=(struct Cyc_Dict_Dict(*)(int(*cmp)(struct _tuple0*,struct _tuple0*)))Cyc_Dict_empty;_tmp19D;})(Cyc_Absyn_qvar_cmp);});struct Cyc_Absyn_Typedefdecl*(*_tmp19E)(struct Cyc_Absyn_Typedefdecl*,struct Cyc_Absyn_Typedefdecl*,unsigned,struct _fat_ptr*msg)=Cyc_Tcdecl_merge_typedefdecl;struct _fat_ptr _tmp19F=({const char*_tmp1A0="typedef";_tag_fat(_tmp1A0,sizeof(char),8U);});struct _fat_ptr*_tmp1A1=msg;_tmp198(_tmp19A,_tmp19B,_tmp19C,_tmp19E,_tmp19F,_tmp1A1);})!= 0;
# 768
int r5=({({int(*_tmp3CD)(struct Cyc_Dict_Dict dic1,struct Cyc_Dict_Dict dic2,int(*incl_f)(struct Cyc_Absyn_Vardecl*,struct Cyc_Absyn_Vardecl*,struct _fat_ptr*),struct _fat_ptr t,struct _fat_ptr*msg)=({int(*_tmp196)(struct Cyc_Dict_Dict dic1,struct Cyc_Dict_Dict dic2,int(*incl_f)(struct Cyc_Absyn_Vardecl*,struct Cyc_Absyn_Vardecl*,struct _fat_ptr*),struct _fat_ptr t,struct _fat_ptr*msg)=(int(*)(struct Cyc_Dict_Dict dic1,struct Cyc_Dict_Dict dic2,int(*incl_f)(struct Cyc_Absyn_Vardecl*,struct Cyc_Absyn_Vardecl*,struct _fat_ptr*),struct _fat_ptr t,struct _fat_ptr*msg))Cyc_Interface_incl_dict;_tmp196;});struct Cyc_Dict_Dict _tmp3CC=ie1->vardecls;struct Cyc_Dict_Dict _tmp3CB=ie2->vardecls;struct _fat_ptr _tmp3CA=({const char*_tmp197="variable";_tag_fat(_tmp197,sizeof(char),9U);});_tmp3CD(_tmp3CC,_tmp3CB,Cyc_Interface_incl_vardecl,_tmp3CA,msg);});});
int r6=({({int(*_tmp3D1)(struct Cyc_Dict_Dict dic1,struct Cyc_Dict_Dict dic2,int(*incl_f)(struct Cyc_Tcdecl_Xdatatypefielddecl*,struct Cyc_Tcdecl_Xdatatypefielddecl*,struct _fat_ptr*),struct _fat_ptr t,struct _fat_ptr*msg)=({int(*_tmp194)(struct Cyc_Dict_Dict dic1,struct Cyc_Dict_Dict dic2,int(*incl_f)(struct Cyc_Tcdecl_Xdatatypefielddecl*,struct Cyc_Tcdecl_Xdatatypefielddecl*,struct _fat_ptr*),struct _fat_ptr t,struct _fat_ptr*msg)=(int(*)(struct Cyc_Dict_Dict dic1,struct Cyc_Dict_Dict dic2,int(*incl_f)(struct Cyc_Tcdecl_Xdatatypefielddecl*,struct Cyc_Tcdecl_Xdatatypefielddecl*,struct _fat_ptr*),struct _fat_ptr t,struct _fat_ptr*msg))Cyc_Interface_incl_dict;_tmp194;});struct Cyc_Dict_Dict _tmp3D0=ie1->xdatatypefielddecls;struct Cyc_Dict_Dict _tmp3CF=ie2->xdatatypefielddecls;struct _fat_ptr _tmp3CE=({const char*_tmp195="xdatatypefield";_tag_fat(_tmp195,sizeof(char),15U);});_tmp3D1(_tmp3D0,_tmp3CF,Cyc_Interface_incl_xdatatypefielddecl,_tmp3CE,msg);});});
return((((r1 && r2)&& r3)&& r4)&& r5)&& r6;}
# 760
int Cyc_Interface_is_subinterface(struct Cyc_Interface_I*i1,struct Cyc_Interface_I*i2,struct _tuple12*info){
# 774
struct _handler_cons _tmp1A9;_push_handler(& _tmp1A9);{int _tmp1AB=0;if(setjmp(_tmp1A9.handler))_tmp1AB=1;if(!_tmp1AB){
{int res=1;
struct _fat_ptr*msg=0;
# 778
if(info != 0)
msg=({struct _fat_ptr*_tmp1B0=_cycalloc(sizeof(*_tmp1B0));({struct _fat_ptr _tmp3D3=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp1AE=({struct Cyc_String_pa_PrintArg_struct _tmp2C6;_tmp2C6.tag=0U,_tmp2C6.f1=(struct _fat_ptr)((struct _fat_ptr)(*info).f1);_tmp2C6;});struct Cyc_String_pa_PrintArg_struct _tmp1AF=({struct Cyc_String_pa_PrintArg_struct _tmp2C5;_tmp2C5.tag=0U,_tmp2C5.f1=(struct _fat_ptr)((struct _fat_ptr)(*info).f2);_tmp2C5;});void*_tmp1AC[2U];_tmp1AC[0]=& _tmp1AE,_tmp1AC[1]=& _tmp1AF;({struct _fat_ptr _tmp3D2=({const char*_tmp1AD="checking inclusion of %s exports into %s exports,";_tag_fat(_tmp1AD,sizeof(char),50U);});Cyc_aprintf(_tmp3D2,_tag_fat(_tmp1AC,sizeof(void*),2U));});});*_tmp1B0=_tmp3D3;});_tmp1B0;});
# 778
if(!({Cyc_Interface_incl_ienv(i1->exports,i2->exports,msg);}))
# 780
res=0;
# 778
if(info != 0)
# 783
msg=({struct _fat_ptr*_tmp1B5=_cycalloc(sizeof(*_tmp1B5));({struct _fat_ptr _tmp3D5=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp1B3=({struct Cyc_String_pa_PrintArg_struct _tmp2C8;_tmp2C8.tag=0U,_tmp2C8.f1=(struct _fat_ptr)((struct _fat_ptr)(*info).f2);_tmp2C8;});struct Cyc_String_pa_PrintArg_struct _tmp1B4=({struct Cyc_String_pa_PrintArg_struct _tmp2C7;_tmp2C7.tag=0U,_tmp2C7.f1=(struct _fat_ptr)((struct _fat_ptr)(*info).f1);_tmp2C7;});void*_tmp1B1[2U];_tmp1B1[0]=& _tmp1B3,_tmp1B1[1]=& _tmp1B4;({struct _fat_ptr _tmp3D4=({const char*_tmp1B2="checking inclusion of %s imports into %s imports,";_tag_fat(_tmp1B2,sizeof(char),50U);});Cyc_aprintf(_tmp3D4,_tag_fat(_tmp1B1,sizeof(void*),2U));});});*_tmp1B5=_tmp3D5;});_tmp1B5;});
# 778
if(!({Cyc_Interface_incl_ienv(i2->imports,i1->imports,msg);}))
# 784
res=0;{
# 778
int _tmp1B6=res;_npop_handler(0U);return _tmp1B6;}}
# 775
;_pop_handler();}else{void*_tmp1AA=(void*)Cyc_Core_get_exn_thrown();void*_tmp1B7=_tmp1AA;void*_tmp1B8;if(((struct Cyc_Tcdecl_Incompatible_exn_struct*)_tmp1B7)->tag == Cyc_Tcdecl_Incompatible){_LL1: _LL2:
# 788
 return 0;}else{_LL3: _tmp1B8=_tmp1B7;_LL4: {void*exn=_tmp1B8;(int)_rethrow(exn);}}_LL0:;}}}struct _tuple31{int f1;struct Cyc_Dict_Dict f2;struct Cyc_Dict_Dict f3;struct Cyc_Dict_Dict f4;void*(*f5)(void*,void*,unsigned,struct _fat_ptr*);struct _fat_ptr f6;struct _fat_ptr*f7;};
# 796
void Cyc_Interface_compat_merge_dict_f(struct _tuple31*env,struct _tuple0*x,void*y2){
# 803
struct _fat_ptr*_tmp1C0;struct _fat_ptr _tmp1BF;void*(*_tmp1BE)(void*,void*,unsigned,struct _fat_ptr*);struct Cyc_Dict_Dict _tmp1BD;struct Cyc_Dict_Dict _tmp1BC;struct Cyc_Dict_Dict*_tmp1BB;int*_tmp1BA;_LL1: _tmp1BA=(int*)& env->f1;_tmp1BB=(struct Cyc_Dict_Dict*)& env->f2;_tmp1BC=env->f3;_tmp1BD=env->f4;_tmp1BE=env->f5;_tmp1BF=env->f6;_tmp1C0=env->f7;_LL2: {int*res=_tmp1BA;struct Cyc_Dict_Dict*res_dic=_tmp1BB;struct Cyc_Dict_Dict dic1=_tmp1BC;struct Cyc_Dict_Dict excl=_tmp1BD;void*(*merge_f)(void*,void*,unsigned,struct _fat_ptr*)=_tmp1BE;struct _fat_ptr t=_tmp1BF;struct _fat_ptr*msg=_tmp1C0;
void*y;
{struct _handler_cons _tmp1C1;_push_handler(& _tmp1C1);{int _tmp1C3=0;if(setjmp(_tmp1C1.handler))_tmp1C3=1;if(!_tmp1C3){
{void*y1=({({void*(*_tmp3D7)(struct Cyc_Dict_Dict d,struct _tuple0*k)=({void*(*_tmp1C4)(struct Cyc_Dict_Dict d,struct _tuple0*k)=(void*(*)(struct Cyc_Dict_Dict d,struct _tuple0*k))Cyc_Dict_lookup;_tmp1C4;});struct Cyc_Dict_Dict _tmp3D6=dic1;_tmp3D7(_tmp3D6,x);});});
# 809
void*yt=({merge_f(y1,y2,0U,msg);});
if(!((unsigned)yt)){
*res=0;
_npop_handler(0U);return;}
# 810
y=yt;}
# 806
;_pop_handler();}else{void*_tmp1C2=(void*)Cyc_Core_get_exn_thrown();void*_tmp1C5=_tmp1C2;void*_tmp1C6;if(((struct Cyc_Dict_Absent_exn_struct*)_tmp1C5)->tag == Cyc_Dict_Absent){_LL4: _LL5:
# 817
 y=y2;
goto _LL3;}else{_LL6: _tmp1C6=_tmp1C5;_LL7: {void*exn=_tmp1C6;(int)_rethrow(exn);}}_LL3:;}}}{
# 821
struct _handler_cons _tmp1C7;_push_handler(& _tmp1C7);{int _tmp1C9=0;if(setjmp(_tmp1C7.handler))_tmp1C9=1;if(!_tmp1C9){
{void*ye=({({void*(*_tmp3D9)(struct Cyc_Dict_Dict d,struct _tuple0*k)=({void*(*_tmp1D1)(struct Cyc_Dict_Dict d,struct _tuple0*k)=(void*(*)(struct Cyc_Dict_Dict d,struct _tuple0*k))Cyc_Dict_lookup;_tmp1D1;});struct Cyc_Dict_Dict _tmp3D8=excl;_tmp3D9(_tmp3D8,x);});});
# 826
void*yt=({merge_f(ye,y,0U,msg);});
if(yt != ye){
if((unsigned)yt)
({void(*_tmp1CA)(struct _fat_ptr*msg1,struct _fat_ptr msg2)=Cyc_Interface_check_err;struct _fat_ptr*_tmp1CB=msg;struct _fat_ptr _tmp1CC=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp1CF=({struct Cyc_String_pa_PrintArg_struct _tmp2CA;_tmp2CA.tag=0U,_tmp2CA.f1=(struct _fat_ptr)((struct _fat_ptr)t);_tmp2CA;});struct Cyc_String_pa_PrintArg_struct _tmp1D0=({struct Cyc_String_pa_PrintArg_struct _tmp2C9;_tmp2C9.tag=0U,({
struct _fat_ptr _tmp3DA=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(x);}));_tmp2C9.f1=_tmp3DA;});_tmp2C9;});void*_tmp1CD[2U];_tmp1CD[0]=& _tmp1CF,_tmp1CD[1]=& _tmp1D0;({struct _fat_ptr _tmp3DB=({const char*_tmp1CE="abstract %s %s is being imported as non-abstract";_tag_fat(_tmp1CE,sizeof(char),49U);});Cyc_aprintf(_tmp3DB,_tag_fat(_tmp1CD,sizeof(void*),2U));});});_tmp1CA(_tmp1CB,_tmp1CC);});
# 828
*res=0;}}
# 822
;_pop_handler();}else{void*_tmp1C8=(void*)Cyc_Core_get_exn_thrown();void*_tmp1D2=_tmp1C8;void*_tmp1D3;if(((struct Cyc_Dict_Absent_exn_struct*)_tmp1D2)->tag == Cyc_Dict_Absent){_LL9: _LLA:
# 835
 if(*res)
({struct Cyc_Dict_Dict _tmp3DF=({({struct Cyc_Dict_Dict(*_tmp3DE)(struct Cyc_Dict_Dict d,struct _tuple0*k,void*v)=({struct Cyc_Dict_Dict(*_tmp1D4)(struct Cyc_Dict_Dict d,struct _tuple0*k,void*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _tuple0*k,void*v))Cyc_Dict_insert;_tmp1D4;});struct Cyc_Dict_Dict _tmp3DD=*res_dic;struct _tuple0*_tmp3DC=x;_tmp3DE(_tmp3DD,_tmp3DC,y);});});*res_dic=_tmp3DF;});
# 835
goto _LL8;}else{_LLB: _tmp1D3=_tmp1D2;_LLC: {void*exn=_tmp1D3;(int)_rethrow(exn);}}_LL8:;}}}}}
# 842
struct Cyc_Dict_Dict*Cyc_Interface_compat_merge_dict(struct Cyc_Dict_Dict dic1,struct Cyc_Dict_Dict dic2,struct Cyc_Dict_Dict excl,void*(*merge_f)(void*,void*,unsigned,struct _fat_ptr*),struct _fat_ptr t,struct _fat_ptr*msg){
# 849
struct _tuple31 env=({struct _tuple31 _tmp2CB;_tmp2CB.f1=1,_tmp2CB.f2=dic1,_tmp2CB.f3=dic1,_tmp2CB.f4=excl,_tmp2CB.f5=merge_f,_tmp2CB.f6=t,_tmp2CB.f7=msg;_tmp2CB;});
# 851
({({void(*_tmp3E0)(void(*f)(struct _tuple31*,struct _tuple0*,void*),struct _tuple31*env,struct Cyc_Dict_Dict d)=({void(*_tmp1D6)(void(*f)(struct _tuple31*,struct _tuple0*,void*),struct _tuple31*env,struct Cyc_Dict_Dict d)=(void(*)(void(*f)(struct _tuple31*,struct _tuple0*,void*),struct _tuple31*env,struct Cyc_Dict_Dict d))Cyc_Dict_iter_c;_tmp1D6;});_tmp3E0(Cyc_Interface_compat_merge_dict_f,& env,dic2);});});
# 853
if(env.f1)
return({struct Cyc_Dict_Dict*_tmp1D7=_cycalloc(sizeof(*_tmp1D7));*_tmp1D7=env.f2;_tmp1D7;});
# 853
return 0;}
# 863
struct Cyc_Interface_Ienv*Cyc_Interface_compat_merge_ienv(struct Cyc_Interface_Ienv*ie1,struct Cyc_Interface_Ienv*ie2,struct Cyc_Interface_Ienv*iexcl,struct _fat_ptr*msg){
struct Cyc_Dict_Dict*r1=({({struct Cyc_Dict_Dict*(*_tmp3E5)(struct Cyc_Dict_Dict dic1,struct Cyc_Dict_Dict dic2,struct Cyc_Dict_Dict excl,struct Cyc_Absyn_Aggrdecl*(*merge_f)(struct Cyc_Absyn_Aggrdecl*,struct Cyc_Absyn_Aggrdecl*,unsigned,struct _fat_ptr*),struct _fat_ptr t,struct _fat_ptr*msg)=({struct Cyc_Dict_Dict*(*_tmp1E4)(struct Cyc_Dict_Dict dic1,struct Cyc_Dict_Dict dic2,struct Cyc_Dict_Dict excl,struct Cyc_Absyn_Aggrdecl*(*merge_f)(struct Cyc_Absyn_Aggrdecl*,struct Cyc_Absyn_Aggrdecl*,unsigned,struct _fat_ptr*),struct _fat_ptr t,struct _fat_ptr*msg)=(struct Cyc_Dict_Dict*(*)(struct Cyc_Dict_Dict dic1,struct Cyc_Dict_Dict dic2,struct Cyc_Dict_Dict excl,struct Cyc_Absyn_Aggrdecl*(*merge_f)(struct Cyc_Absyn_Aggrdecl*,struct Cyc_Absyn_Aggrdecl*,unsigned,struct _fat_ptr*),struct _fat_ptr t,struct _fat_ptr*msg))Cyc_Interface_compat_merge_dict;_tmp1E4;});struct Cyc_Dict_Dict _tmp3E4=ie1->aggrdecls;struct Cyc_Dict_Dict _tmp3E3=ie2->aggrdecls;struct Cyc_Dict_Dict _tmp3E2=iexcl->aggrdecls;struct _fat_ptr _tmp3E1=({const char*_tmp1E5="type";_tag_fat(_tmp1E5,sizeof(char),5U);});_tmp3E5(_tmp3E4,_tmp3E3,_tmp3E2,Cyc_Tcdecl_merge_aggrdecl,_tmp3E1,msg);});});
struct Cyc_Dict_Dict*r2=({({struct Cyc_Dict_Dict*(*_tmp3EA)(struct Cyc_Dict_Dict dic1,struct Cyc_Dict_Dict dic2,struct Cyc_Dict_Dict excl,struct Cyc_Absyn_Datatypedecl*(*merge_f)(struct Cyc_Absyn_Datatypedecl*,struct Cyc_Absyn_Datatypedecl*,unsigned,struct _fat_ptr*),struct _fat_ptr t,struct _fat_ptr*msg)=({struct Cyc_Dict_Dict*(*_tmp1E2)(struct Cyc_Dict_Dict dic1,struct Cyc_Dict_Dict dic2,struct Cyc_Dict_Dict excl,struct Cyc_Absyn_Datatypedecl*(*merge_f)(struct Cyc_Absyn_Datatypedecl*,struct Cyc_Absyn_Datatypedecl*,unsigned,struct _fat_ptr*),struct _fat_ptr t,struct _fat_ptr*msg)=(struct Cyc_Dict_Dict*(*)(struct Cyc_Dict_Dict dic1,struct Cyc_Dict_Dict dic2,struct Cyc_Dict_Dict excl,struct Cyc_Absyn_Datatypedecl*(*merge_f)(struct Cyc_Absyn_Datatypedecl*,struct Cyc_Absyn_Datatypedecl*,unsigned,struct _fat_ptr*),struct _fat_ptr t,struct _fat_ptr*msg))Cyc_Interface_compat_merge_dict;_tmp1E2;});struct Cyc_Dict_Dict _tmp3E9=ie1->datatypedecls;struct Cyc_Dict_Dict _tmp3E8=ie2->datatypedecls;struct Cyc_Dict_Dict _tmp3E7=iexcl->datatypedecls;struct _fat_ptr _tmp3E6=({const char*_tmp1E3="datatype";_tag_fat(_tmp1E3,sizeof(char),9U);});_tmp3EA(_tmp3E9,_tmp3E8,_tmp3E7,Cyc_Tcdecl_merge_datatypedecl,_tmp3E6,msg);});});
struct Cyc_Dict_Dict*r3=({({struct Cyc_Dict_Dict*(*_tmp3EF)(struct Cyc_Dict_Dict dic1,struct Cyc_Dict_Dict dic2,struct Cyc_Dict_Dict excl,struct Cyc_Absyn_Enumdecl*(*merge_f)(struct Cyc_Absyn_Enumdecl*,struct Cyc_Absyn_Enumdecl*,unsigned,struct _fat_ptr*),struct _fat_ptr t,struct _fat_ptr*msg)=({struct Cyc_Dict_Dict*(*_tmp1E0)(struct Cyc_Dict_Dict dic1,struct Cyc_Dict_Dict dic2,struct Cyc_Dict_Dict excl,struct Cyc_Absyn_Enumdecl*(*merge_f)(struct Cyc_Absyn_Enumdecl*,struct Cyc_Absyn_Enumdecl*,unsigned,struct _fat_ptr*),struct _fat_ptr t,struct _fat_ptr*msg)=(struct Cyc_Dict_Dict*(*)(struct Cyc_Dict_Dict dic1,struct Cyc_Dict_Dict dic2,struct Cyc_Dict_Dict excl,struct Cyc_Absyn_Enumdecl*(*merge_f)(struct Cyc_Absyn_Enumdecl*,struct Cyc_Absyn_Enumdecl*,unsigned,struct _fat_ptr*),struct _fat_ptr t,struct _fat_ptr*msg))Cyc_Interface_compat_merge_dict;_tmp1E0;});struct Cyc_Dict_Dict _tmp3EE=ie1->enumdecls;struct Cyc_Dict_Dict _tmp3ED=ie2->enumdecls;struct Cyc_Dict_Dict _tmp3EC=iexcl->enumdecls;struct _fat_ptr _tmp3EB=({const char*_tmp1E1="enum";_tag_fat(_tmp1E1,sizeof(char),5U);});_tmp3EF(_tmp3EE,_tmp3ED,_tmp3EC,Cyc_Tcdecl_merge_enumdecl,_tmp3EB,msg);});});
struct Cyc_Dict_Dict*r4=({({struct Cyc_Dict_Dict*(*_tmp3F4)(struct Cyc_Dict_Dict dic1,struct Cyc_Dict_Dict dic2,struct Cyc_Dict_Dict excl,struct Cyc_Absyn_Typedefdecl*(*merge_f)(struct Cyc_Absyn_Typedefdecl*,struct Cyc_Absyn_Typedefdecl*,unsigned,struct _fat_ptr*),struct _fat_ptr t,struct _fat_ptr*msg)=({struct Cyc_Dict_Dict*(*_tmp1DE)(struct Cyc_Dict_Dict dic1,struct Cyc_Dict_Dict dic2,struct Cyc_Dict_Dict excl,struct Cyc_Absyn_Typedefdecl*(*merge_f)(struct Cyc_Absyn_Typedefdecl*,struct Cyc_Absyn_Typedefdecl*,unsigned,struct _fat_ptr*),struct _fat_ptr t,struct _fat_ptr*msg)=(struct Cyc_Dict_Dict*(*)(struct Cyc_Dict_Dict dic1,struct Cyc_Dict_Dict dic2,struct Cyc_Dict_Dict excl,struct Cyc_Absyn_Typedefdecl*(*merge_f)(struct Cyc_Absyn_Typedefdecl*,struct Cyc_Absyn_Typedefdecl*,unsigned,struct _fat_ptr*),struct _fat_ptr t,struct _fat_ptr*msg))Cyc_Interface_compat_merge_dict;_tmp1DE;});struct Cyc_Dict_Dict _tmp3F3=ie1->typedefdecls;struct Cyc_Dict_Dict _tmp3F2=ie2->typedefdecls;struct Cyc_Dict_Dict _tmp3F1=iexcl->typedefdecls;struct _fat_ptr _tmp3F0=({const char*_tmp1DF="typedef";_tag_fat(_tmp1DF,sizeof(char),8U);});_tmp3F4(_tmp3F3,_tmp3F2,_tmp3F1,Cyc_Tcdecl_merge_typedefdecl,_tmp3F0,msg);});});
struct Cyc_Dict_Dict*r5=({({struct Cyc_Dict_Dict*(*_tmp3F9)(struct Cyc_Dict_Dict dic1,struct Cyc_Dict_Dict dic2,struct Cyc_Dict_Dict excl,struct Cyc_Absyn_Vardecl*(*merge_f)(struct Cyc_Absyn_Vardecl*,struct Cyc_Absyn_Vardecl*,unsigned,struct _fat_ptr*),struct _fat_ptr t,struct _fat_ptr*msg)=({struct Cyc_Dict_Dict*(*_tmp1DC)(struct Cyc_Dict_Dict dic1,struct Cyc_Dict_Dict dic2,struct Cyc_Dict_Dict excl,struct Cyc_Absyn_Vardecl*(*merge_f)(struct Cyc_Absyn_Vardecl*,struct Cyc_Absyn_Vardecl*,unsigned,struct _fat_ptr*),struct _fat_ptr t,struct _fat_ptr*msg)=(struct Cyc_Dict_Dict*(*)(struct Cyc_Dict_Dict dic1,struct Cyc_Dict_Dict dic2,struct Cyc_Dict_Dict excl,struct Cyc_Absyn_Vardecl*(*merge_f)(struct Cyc_Absyn_Vardecl*,struct Cyc_Absyn_Vardecl*,unsigned,struct _fat_ptr*),struct _fat_ptr t,struct _fat_ptr*msg))Cyc_Interface_compat_merge_dict;_tmp1DC;});struct Cyc_Dict_Dict _tmp3F8=ie1->vardecls;struct Cyc_Dict_Dict _tmp3F7=ie2->vardecls;struct Cyc_Dict_Dict _tmp3F6=iexcl->vardecls;struct _fat_ptr _tmp3F5=({const char*_tmp1DD="variable";_tag_fat(_tmp1DD,sizeof(char),9U);});_tmp3F9(_tmp3F8,_tmp3F7,_tmp3F6,Cyc_Tcdecl_merge_vardecl,_tmp3F5,msg);});});
struct Cyc_Dict_Dict*r6=({({struct Cyc_Dict_Dict*(*_tmp3FE)(struct Cyc_Dict_Dict dic1,struct Cyc_Dict_Dict dic2,struct Cyc_Dict_Dict excl,struct Cyc_Tcdecl_Xdatatypefielddecl*(*merge_f)(struct Cyc_Tcdecl_Xdatatypefielddecl*,struct Cyc_Tcdecl_Xdatatypefielddecl*,unsigned,struct _fat_ptr*),struct _fat_ptr t,struct _fat_ptr*msg)=({struct Cyc_Dict_Dict*(*_tmp1DA)(struct Cyc_Dict_Dict dic1,struct Cyc_Dict_Dict dic2,struct Cyc_Dict_Dict excl,struct Cyc_Tcdecl_Xdatatypefielddecl*(*merge_f)(struct Cyc_Tcdecl_Xdatatypefielddecl*,struct Cyc_Tcdecl_Xdatatypefielddecl*,unsigned,struct _fat_ptr*),struct _fat_ptr t,struct _fat_ptr*msg)=(struct Cyc_Dict_Dict*(*)(struct Cyc_Dict_Dict dic1,struct Cyc_Dict_Dict dic2,struct Cyc_Dict_Dict excl,struct Cyc_Tcdecl_Xdatatypefielddecl*(*merge_f)(struct Cyc_Tcdecl_Xdatatypefielddecl*,struct Cyc_Tcdecl_Xdatatypefielddecl*,unsigned,struct _fat_ptr*),struct _fat_ptr t,struct _fat_ptr*msg))Cyc_Interface_compat_merge_dict;_tmp1DA;});struct Cyc_Dict_Dict _tmp3FD=ie1->xdatatypefielddecls;struct Cyc_Dict_Dict _tmp3FC=ie2->xdatatypefielddecls;struct Cyc_Dict_Dict _tmp3FB=iexcl->xdatatypefielddecls;struct _fat_ptr _tmp3FA=({const char*_tmp1DB="xdatatypefield";_tag_fat(_tmp1DB,sizeof(char),15U);});_tmp3FE(_tmp3FD,_tmp3FC,_tmp3FB,Cyc_Tcdecl_merge_xdatatypefielddecl,_tmp3FA,msg);});});
if(((((!((unsigned)r1)|| !((unsigned)r2))|| !((unsigned)r3))|| !((unsigned)r4))|| !((unsigned)r5))|| !((unsigned)r6))
return 0;
# 870
return({struct Cyc_Interface_Ienv*_tmp1D9=_cycalloc(sizeof(*_tmp1D9));
# 872
_tmp1D9->aggrdecls=*r1,_tmp1D9->datatypedecls=*r2,_tmp1D9->enumdecls=*r3,_tmp1D9->typedefdecls=*r4,_tmp1D9->vardecls=*r5,_tmp1D9->xdatatypefielddecls=*r6;_tmp1D9;});}struct _tuple32{int f1;struct Cyc_Dict_Dict f2;struct Cyc_Dict_Dict f3;struct _fat_ptr f4;struct _fat_ptr*f5;};
# 876
void Cyc_Interface_disj_merge_dict_f(struct _tuple32*env,struct _tuple0*x,void*y){
# 879
struct _fat_ptr*_tmp1EB;struct _fat_ptr _tmp1EA;struct Cyc_Dict_Dict _tmp1E9;struct Cyc_Dict_Dict*_tmp1E8;int*_tmp1E7;_LL1: _tmp1E7=(int*)& env->f1;_tmp1E8=(struct Cyc_Dict_Dict*)& env->f2;_tmp1E9=env->f3;_tmp1EA=env->f4;_tmp1EB=env->f5;_LL2: {int*res=_tmp1E7;struct Cyc_Dict_Dict*res_dic=_tmp1E8;struct Cyc_Dict_Dict dic1=_tmp1E9;struct _fat_ptr t=_tmp1EA;struct _fat_ptr*msg=_tmp1EB;
if(({({int(*_tmp400)(struct Cyc_Dict_Dict d,struct _tuple0*k)=({int(*_tmp1EC)(struct Cyc_Dict_Dict d,struct _tuple0*k)=(int(*)(struct Cyc_Dict_Dict d,struct _tuple0*k))Cyc_Dict_member;_tmp1EC;});struct Cyc_Dict_Dict _tmp3FF=dic1;_tmp400(_tmp3FF,x);});})){
({void(*_tmp1ED)(struct _fat_ptr*msg1,struct _fat_ptr msg2)=Cyc_Interface_check_err;struct _fat_ptr*_tmp1EE=msg;struct _fat_ptr _tmp1EF=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp1F2=({struct Cyc_String_pa_PrintArg_struct _tmp2CD;_tmp2CD.tag=0U,_tmp2CD.f1=(struct _fat_ptr)((struct _fat_ptr)t);_tmp2CD;});struct Cyc_String_pa_PrintArg_struct _tmp1F3=({struct Cyc_String_pa_PrintArg_struct _tmp2CC;_tmp2CC.tag=0U,({
struct _fat_ptr _tmp401=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(x);}));_tmp2CC.f1=_tmp401;});_tmp2CC;});void*_tmp1F0[2U];_tmp1F0[0]=& _tmp1F2,_tmp1F0[1]=& _tmp1F3;({struct _fat_ptr _tmp402=({const char*_tmp1F1="%s %s is exported more than once";_tag_fat(_tmp1F1,sizeof(char),33U);});Cyc_aprintf(_tmp402,_tag_fat(_tmp1F0,sizeof(void*),2U));});});_tmp1ED(_tmp1EE,_tmp1EF);});
*res=0;}else{
# 885
if(*res)({struct Cyc_Dict_Dict _tmp406=({({struct Cyc_Dict_Dict(*_tmp405)(struct Cyc_Dict_Dict d,struct _tuple0*k,void*v)=({struct Cyc_Dict_Dict(*_tmp1F4)(struct Cyc_Dict_Dict d,struct _tuple0*k,void*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _tuple0*k,void*v))Cyc_Dict_insert;_tmp1F4;});struct Cyc_Dict_Dict _tmp404=*res_dic;struct _tuple0*_tmp403=x;_tmp405(_tmp404,_tmp403,y);});});*res_dic=_tmp406;});}}}
# 889
struct Cyc_Dict_Dict*Cyc_Interface_disj_merge_dict(struct Cyc_Dict_Dict dic1,struct Cyc_Dict_Dict dic2,struct _fat_ptr t,struct _fat_ptr*msg){
# 893
struct _tuple32 env=({struct _tuple32 _tmp2CE;_tmp2CE.f1=1,_tmp2CE.f2=dic1,_tmp2CE.f3=dic1,_tmp2CE.f4=t,_tmp2CE.f5=msg;_tmp2CE;});
({({void(*_tmp407)(void(*f)(struct _tuple32*,struct _tuple0*,void*),struct _tuple32*env,struct Cyc_Dict_Dict d)=({void(*_tmp1F6)(void(*f)(struct _tuple32*,struct _tuple0*,void*),struct _tuple32*env,struct Cyc_Dict_Dict d)=(void(*)(void(*f)(struct _tuple32*,struct _tuple0*,void*),struct _tuple32*env,struct Cyc_Dict_Dict d))Cyc_Dict_iter_c;_tmp1F6;});_tmp407(Cyc_Interface_disj_merge_dict_f,& env,dic2);});});
if(env.f1)
return({struct Cyc_Dict_Dict*_tmp1F7=_cycalloc(sizeof(*_tmp1F7));*_tmp1F7=env.f2;_tmp1F7;});
# 895
return 0;}
# 900
struct Cyc_Interface_Ienv*Cyc_Interface_disj_merge_ienv(struct Cyc_Interface_Ienv*ie1,struct Cyc_Interface_Ienv*ie2,struct _fat_ptr*msg){
struct Cyc_Dict_Dict*r1=({struct Cyc_Dict_Dict*(*_tmp208)(struct Cyc_Dict_Dict dic1,struct Cyc_Dict_Dict dic2,struct Cyc_Dict_Dict excl,struct Cyc_Absyn_Aggrdecl*(*merge_f)(struct Cyc_Absyn_Aggrdecl*,struct Cyc_Absyn_Aggrdecl*,unsigned,struct _fat_ptr*),struct _fat_ptr t,struct _fat_ptr*msg)=({struct Cyc_Dict_Dict*(*_tmp209)(struct Cyc_Dict_Dict dic1,struct Cyc_Dict_Dict dic2,struct Cyc_Dict_Dict excl,struct Cyc_Absyn_Aggrdecl*(*merge_f)(struct Cyc_Absyn_Aggrdecl*,struct Cyc_Absyn_Aggrdecl*,unsigned,struct _fat_ptr*),struct _fat_ptr t,struct _fat_ptr*msg)=(struct Cyc_Dict_Dict*(*)(struct Cyc_Dict_Dict dic1,struct Cyc_Dict_Dict dic2,struct Cyc_Dict_Dict excl,struct Cyc_Absyn_Aggrdecl*(*merge_f)(struct Cyc_Absyn_Aggrdecl*,struct Cyc_Absyn_Aggrdecl*,unsigned,struct _fat_ptr*),struct _fat_ptr t,struct _fat_ptr*msg))Cyc_Interface_compat_merge_dict;_tmp209;});struct Cyc_Dict_Dict _tmp20A=ie1->aggrdecls;struct Cyc_Dict_Dict _tmp20B=ie2->aggrdecls;struct Cyc_Dict_Dict _tmp20C=({({
struct Cyc_Dict_Dict(*_tmp20D)(int(*cmp)(struct _tuple0*,struct _tuple0*))=(struct Cyc_Dict_Dict(*)(int(*cmp)(struct _tuple0*,struct _tuple0*)))Cyc_Dict_empty;_tmp20D;})(Cyc_Absyn_qvar_cmp);});struct Cyc_Absyn_Aggrdecl*(*_tmp20E)(struct Cyc_Absyn_Aggrdecl*,struct Cyc_Absyn_Aggrdecl*,unsigned,struct _fat_ptr*msg)=Cyc_Tcdecl_merge_aggrdecl;struct _fat_ptr _tmp20F=({const char*_tmp210="type";_tag_fat(_tmp210,sizeof(char),5U);});struct _fat_ptr*_tmp211=msg;_tmp208(_tmp20A,_tmp20B,_tmp20C,_tmp20E,_tmp20F,_tmp211);});
# 904
struct Cyc_Dict_Dict*r2=({({struct Cyc_Dict_Dict _tmp40A=ie1->datatypedecls;struct Cyc_Dict_Dict _tmp409=ie2->datatypedecls;struct _fat_ptr _tmp408=({const char*_tmp207="datatype";_tag_fat(_tmp207,sizeof(char),9U);});Cyc_Interface_disj_merge_dict(_tmp40A,_tmp409,_tmp408,msg);});});
struct Cyc_Dict_Dict*r3=({({struct Cyc_Dict_Dict _tmp40D=ie1->enumdecls;struct Cyc_Dict_Dict _tmp40C=ie2->enumdecls;struct _fat_ptr _tmp40B=({const char*_tmp206="enum";_tag_fat(_tmp206,sizeof(char),5U);});Cyc_Interface_disj_merge_dict(_tmp40D,_tmp40C,_tmp40B,msg);});});
# 907
struct Cyc_Dict_Dict*r4=({struct Cyc_Dict_Dict*(*_tmp1FC)(struct Cyc_Dict_Dict dic1,struct Cyc_Dict_Dict dic2,struct Cyc_Dict_Dict excl,struct Cyc_Absyn_Typedefdecl*(*merge_f)(struct Cyc_Absyn_Typedefdecl*,struct Cyc_Absyn_Typedefdecl*,unsigned,struct _fat_ptr*),struct _fat_ptr t,struct _fat_ptr*msg)=({struct Cyc_Dict_Dict*(*_tmp1FD)(struct Cyc_Dict_Dict dic1,struct Cyc_Dict_Dict dic2,struct Cyc_Dict_Dict excl,struct Cyc_Absyn_Typedefdecl*(*merge_f)(struct Cyc_Absyn_Typedefdecl*,struct Cyc_Absyn_Typedefdecl*,unsigned,struct _fat_ptr*),struct _fat_ptr t,struct _fat_ptr*msg)=(struct Cyc_Dict_Dict*(*)(struct Cyc_Dict_Dict dic1,struct Cyc_Dict_Dict dic2,struct Cyc_Dict_Dict excl,struct Cyc_Absyn_Typedefdecl*(*merge_f)(struct Cyc_Absyn_Typedefdecl*,struct Cyc_Absyn_Typedefdecl*,unsigned,struct _fat_ptr*),struct _fat_ptr t,struct _fat_ptr*msg))Cyc_Interface_compat_merge_dict;_tmp1FD;});struct Cyc_Dict_Dict _tmp1FE=ie1->typedefdecls;struct Cyc_Dict_Dict _tmp1FF=ie2->typedefdecls;struct Cyc_Dict_Dict _tmp200=({({
struct Cyc_Dict_Dict(*_tmp201)(int(*cmp)(struct _tuple0*,struct _tuple0*))=(struct Cyc_Dict_Dict(*)(int(*cmp)(struct _tuple0*,struct _tuple0*)))Cyc_Dict_empty;_tmp201;})(Cyc_Absyn_qvar_cmp);});struct Cyc_Absyn_Typedefdecl*(*_tmp202)(struct Cyc_Absyn_Typedefdecl*,struct Cyc_Absyn_Typedefdecl*,unsigned,struct _fat_ptr*msg)=Cyc_Tcdecl_merge_typedefdecl;struct _fat_ptr _tmp203=({const char*_tmp204="typedef";_tag_fat(_tmp204,sizeof(char),8U);});struct _fat_ptr*_tmp205=msg;_tmp1FC(_tmp1FE,_tmp1FF,_tmp200,_tmp202,_tmp203,_tmp205);});
# 910
struct Cyc_Dict_Dict*r5=({({struct Cyc_Dict_Dict _tmp410=ie1->vardecls;struct Cyc_Dict_Dict _tmp40F=ie2->vardecls;struct _fat_ptr _tmp40E=({const char*_tmp1FB="variable";_tag_fat(_tmp1FB,sizeof(char),9U);});Cyc_Interface_disj_merge_dict(_tmp410,_tmp40F,_tmp40E,msg);});});
struct Cyc_Dict_Dict*r6=({({struct Cyc_Dict_Dict _tmp413=ie1->xdatatypefielddecls;struct Cyc_Dict_Dict _tmp412=ie2->xdatatypefielddecls;struct _fat_ptr _tmp411=({const char*_tmp1FA="xdatatypefield";_tag_fat(_tmp1FA,sizeof(char),15U);});Cyc_Interface_disj_merge_dict(_tmp413,_tmp412,_tmp411,msg);});});
# 914
if(((((!((unsigned)r1)|| !((unsigned)r2))|| !((unsigned)r3))|| !((unsigned)r4))|| !((unsigned)r5))|| !((unsigned)r6))
return 0;
# 914
return({struct Cyc_Interface_Ienv*_tmp1F9=_cycalloc(sizeof(*_tmp1F9));
# 916
_tmp1F9->aggrdecls=*r1,_tmp1F9->datatypedecls=*r2,_tmp1F9->enumdecls=*r3,_tmp1F9->typedefdecls=*r4,_tmp1F9->vardecls=*r5,_tmp1F9->xdatatypefielddecls=*r6;_tmp1F9;});}
# 920
struct Cyc_Interface_I*Cyc_Interface_merge(struct Cyc_Interface_I*i1,struct Cyc_Interface_I*i2,struct _tuple12*info){
struct _handler_cons _tmp213;_push_handler(& _tmp213);{int _tmp215=0;if(setjmp(_tmp213.handler))_tmp215=1;if(!_tmp215){
{struct _fat_ptr*msg=0;
# 924
if(info != 0)
msg=({struct _fat_ptr*_tmp21A=_cycalloc(sizeof(*_tmp21A));({struct _fat_ptr _tmp415=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp218=({struct Cyc_String_pa_PrintArg_struct _tmp2D0;_tmp2D0.tag=0U,_tmp2D0.f1=(struct _fat_ptr)((struct _fat_ptr)(*info).f1);_tmp2D0;});struct Cyc_String_pa_PrintArg_struct _tmp219=({struct Cyc_String_pa_PrintArg_struct _tmp2CF;_tmp2CF.tag=0U,_tmp2CF.f1=(struct _fat_ptr)((struct _fat_ptr)(*info).f2);_tmp2CF;});void*_tmp216[2U];_tmp216[0]=& _tmp218,_tmp216[1]=& _tmp219;({struct _fat_ptr _tmp414=({const char*_tmp217="merging exports of %s and %s,";_tag_fat(_tmp217,sizeof(char),30U);});Cyc_aprintf(_tmp414,_tag_fat(_tmp216,sizeof(void*),2U));});});*_tmp21A=_tmp415;});_tmp21A;});{
# 924
struct Cyc_Interface_Ienv*exp=({Cyc_Interface_disj_merge_ienv(i1->exports,i2->exports,msg);});
# 929
if(exp == 0){struct Cyc_Interface_I*_tmp21B=0;_npop_handler(0U);return _tmp21B;}if(info != 0)
# 932
msg=({struct _fat_ptr*_tmp220=_cycalloc(sizeof(*_tmp220));({struct _fat_ptr _tmp417=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp21E=({struct Cyc_String_pa_PrintArg_struct _tmp2D2;_tmp2D2.tag=0U,_tmp2D2.f1=(struct _fat_ptr)((struct _fat_ptr)(*info).f1);_tmp2D2;});struct Cyc_String_pa_PrintArg_struct _tmp21F=({struct Cyc_String_pa_PrintArg_struct _tmp2D1;_tmp2D1.tag=0U,_tmp2D1.f1=(struct _fat_ptr)((struct _fat_ptr)(*info).f2);_tmp2D1;});void*_tmp21C[2U];_tmp21C[0]=& _tmp21E,_tmp21C[1]=& _tmp21F;({struct _fat_ptr _tmp416=({const char*_tmp21D="merging imports of %s and %s,";_tag_fat(_tmp21D,sizeof(char),30U);});Cyc_aprintf(_tmp416,_tag_fat(_tmp21C,sizeof(void*),2U));});});*_tmp220=_tmp417;});_tmp220;});{
# 929
struct Cyc_Interface_Ienv*imp=({Cyc_Interface_compat_merge_ienv(i1->imports,i2->imports,exp,msg);});
# 936
if(imp == 0){struct Cyc_Interface_I*_tmp221=0;_npop_handler(0U);return _tmp221;}{struct Cyc_List_List*newtds=0;struct Cyc_List_List*l=i2->tds;
# 939
for(0;l != 0;l=l->tl){
newtds=({struct Cyc_List_List*_tmp223=_cycalloc(sizeof(*_tmp223));({struct Cyc_Absyn_Decl*_tmp418=({struct Cyc_Absyn_Decl*_tmp222=_cycalloc(sizeof(*_tmp222));*_tmp222=*((struct Cyc_Absyn_Decl*)l->hd);_tmp222;});_tmp223->hd=_tmp418;}),_tmp223->tl=newtds;_tmp223;});}
# 942
newtds=({struct Cyc_List_List*(*_tmp224)(struct Cyc_List_List*x)=Cyc_List_rev;struct Cyc_List_List*_tmp225=({({struct Cyc_List_List*(*_tmp41A)(struct Cyc_List_List*(*f)(struct Cyc_List_List*,struct Cyc_Absyn_Decl*),struct Cyc_List_List*accum,struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp226)(struct Cyc_List_List*(*f)(struct Cyc_List_List*,struct Cyc_Absyn_Decl*),struct Cyc_List_List*accum,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_List_List*(*f)(struct Cyc_List_List*,struct Cyc_Absyn_Decl*),struct Cyc_List_List*accum,struct Cyc_List_List*x))Cyc_List_fold_left;_tmp226;});struct Cyc_List_List*_tmp419=newtds;_tmp41A(Cyc_Interface_uniqify_decl_list,_tmp419,i1->tds);});});_tmp224(_tmp225);});{
struct Cyc_Interface_I*_tmp228=({struct Cyc_Interface_I*_tmp227=_cycalloc(sizeof(*_tmp227));_tmp227->imports=imp,_tmp227->exports=exp,_tmp227->tds=newtds;_tmp227;});_npop_handler(0U);return _tmp228;}}}}}
# 922
;_pop_handler();}else{void*_tmp214=(void*)Cyc_Core_get_exn_thrown();void*_tmp229=_tmp214;void*_tmp22A;if(((struct Cyc_Tcdecl_Incompatible_exn_struct*)_tmp229)->tag == Cyc_Tcdecl_Incompatible){_LL1: _LL2:
# 944
 return 0;}else{_LL3: _tmp22A=_tmp229;_LL4: {void*exn=_tmp22A;(int)_rethrow(exn);}}_LL0:;}}}
# 947
struct Cyc_Interface_I*Cyc_Interface_merge_list(struct Cyc_List_List*li,struct Cyc_List_List*linfo){
if(li == 0)return({Cyc_Interface_empty();});{struct Cyc_Interface_I*curr_i=(struct Cyc_Interface_I*)li->hd;
# 951
struct _fat_ptr*curr_info=linfo != 0?(struct _fat_ptr*)linfo->hd: 0;
li=li->tl;
if(linfo != 0)linfo=linfo->tl;for(0;li != 0;li=li->tl){
# 956
struct Cyc_Interface_I*i=({({struct Cyc_Interface_I*_tmp41C=curr_i;struct Cyc_Interface_I*_tmp41B=(struct Cyc_Interface_I*)li->hd;Cyc_Interface_merge(_tmp41C,_tmp41B,curr_info != 0 && linfo != 0?({struct _tuple12*_tmp231=_cycalloc(sizeof(*_tmp231));_tmp231->f1=*curr_info,_tmp231->f2=*((struct _fat_ptr*)linfo->hd);_tmp231;}): 0);});});
# 958
if(i == 0)return 0;curr_i=i;
# 960
if(curr_info != 0)
curr_info=linfo != 0?({struct _fat_ptr*_tmp230=_cycalloc(sizeof(*_tmp230));({struct _fat_ptr _tmp41E=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp22E=({struct Cyc_String_pa_PrintArg_struct _tmp2D4;_tmp2D4.tag=0U,_tmp2D4.f1=(struct _fat_ptr)((struct _fat_ptr)*curr_info);_tmp2D4;});struct Cyc_String_pa_PrintArg_struct _tmp22F=({struct Cyc_String_pa_PrintArg_struct _tmp2D3;_tmp2D3.tag=0U,_tmp2D3.f1=(struct _fat_ptr)((struct _fat_ptr)*((struct _fat_ptr*)linfo->hd));_tmp2D3;});void*_tmp22C[2U];_tmp22C[0]=& _tmp22E,_tmp22C[1]=& _tmp22F;({struct _fat_ptr _tmp41D=({const char*_tmp22D="%s+%s";_tag_fat(_tmp22D,sizeof(char),6U);});Cyc_aprintf(_tmp41D,_tag_fat(_tmp22C,sizeof(void*),2U));});});*_tmp230=_tmp41E;});_tmp230;}): 0;
# 960
if(linfo != 0)
# 963
linfo=linfo->tl;}
# 965
return curr_i;}}
# 968
struct Cyc_Interface_I*Cyc_Interface_get_and_merge_list(struct Cyc_Interface_I*(*get)(void*),struct Cyc_List_List*la,struct Cyc_List_List*linfo){
if(la == 0)return({Cyc_Interface_empty();});{struct Cyc_Interface_I*curr_i=({get(la->hd);});
# 972
struct _fat_ptr*curr_info=linfo != 0?(struct _fat_ptr*)linfo->hd: 0;
la=la->tl;
if(linfo != 0)linfo=linfo->tl;for(0;la != 0;la=la->tl){
# 977
struct Cyc_Interface_I*i=({struct Cyc_Interface_I*(*_tmp238)(struct Cyc_Interface_I*i1,struct Cyc_Interface_I*i2,struct _tuple12*info)=Cyc_Interface_merge;struct Cyc_Interface_I*_tmp239=curr_i;struct Cyc_Interface_I*_tmp23A=({get(la->hd);});struct _tuple12*_tmp23B=
curr_info != 0 && linfo != 0?({struct _tuple12*_tmp23C=_cycalloc(sizeof(*_tmp23C));_tmp23C->f1=*curr_info,_tmp23C->f2=*((struct _fat_ptr*)linfo->hd);_tmp23C;}): 0;_tmp238(_tmp239,_tmp23A,_tmp23B);});
if(i == 0)return 0;curr_i=i;
# 981
if(curr_info != 0)
curr_info=linfo != 0?({struct _fat_ptr*_tmp237=_cycalloc(sizeof(*_tmp237));({struct _fat_ptr _tmp420=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp235=({struct Cyc_String_pa_PrintArg_struct _tmp2D6;_tmp2D6.tag=0U,_tmp2D6.f1=(struct _fat_ptr)((struct _fat_ptr)*curr_info);_tmp2D6;});struct Cyc_String_pa_PrintArg_struct _tmp236=({struct Cyc_String_pa_PrintArg_struct _tmp2D5;_tmp2D5.tag=0U,_tmp2D5.f1=(struct _fat_ptr)((struct _fat_ptr)*((struct _fat_ptr*)linfo->hd));_tmp2D5;});void*_tmp233[2U];_tmp233[0]=& _tmp235,_tmp233[1]=& _tmp236;({struct _fat_ptr _tmp41F=({const char*_tmp234="%s+%s";_tag_fat(_tmp234,sizeof(char),6U);});Cyc_aprintf(_tmp41F,_tag_fat(_tmp233,sizeof(void*),2U));});});*_tmp237=_tmp420;});_tmp237;}): 0;
# 981
if(linfo != 0)
# 984
linfo=linfo->tl;}
# 986
return curr_i;}}
# 992
static struct Cyc_List_List*Cyc_Interface_add_namespace(struct Cyc_List_List*tds){
struct Cyc_List_List*ans=0;
for(0;tds != 0;tds=tds->tl){
struct _tuple0*qv;
struct Cyc_Absyn_Decl*d=(struct Cyc_Absyn_Decl*)tds->hd;
{void*_stmttmpB=d->r;void*_tmp23E=_stmttmpB;struct Cyc_Absyn_Typedefdecl*_tmp23F;struct Cyc_Absyn_Enumdecl*_tmp240;struct Cyc_Absyn_Datatypedecl*_tmp241;struct Cyc_Absyn_Aggrdecl*_tmp242;struct Cyc_Absyn_Fndecl*_tmp243;struct Cyc_Absyn_Vardecl*_tmp244;switch(*((int*)_tmp23E)){case 0U: _LL1: _tmp244=((struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_tmp23E)->f1;_LL2: {struct Cyc_Absyn_Vardecl*vd=_tmp244;
qv=vd->name;goto _LL0;}case 1U: _LL3: _tmp243=((struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*)_tmp23E)->f1;_LL4: {struct Cyc_Absyn_Fndecl*fd=_tmp243;
qv=fd->name;goto _LL0;}case 5U: _LL5: _tmp242=((struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct*)_tmp23E)->f1;_LL6: {struct Cyc_Absyn_Aggrdecl*ad=_tmp242;
qv=ad->name;goto _LL0;}case 6U: _LL7: _tmp241=((struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct*)_tmp23E)->f1;_LL8: {struct Cyc_Absyn_Datatypedecl*dd=_tmp241;
qv=dd->name;goto _LL0;}case 7U: _LL9: _tmp240=((struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct*)_tmp23E)->f1;_LLA: {struct Cyc_Absyn_Enumdecl*ed=_tmp240;
qv=ed->name;goto _LL0;}case 8U: _LLB: _tmp23F=((struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct*)_tmp23E)->f1;_LLC: {struct Cyc_Absyn_Typedefdecl*td=_tmp23F;
qv=td->name;goto _LL0;}default: _LLD: _LLE:
({Cyc_Interface_err(({const char*_tmp245="bad decl form in Interface::add_namespace";_tag_fat(_tmp245,sizeof(char),42U);}));});return 0;}_LL0:;}{
# 1006
struct Cyc_List_List*vs;
{union Cyc_Absyn_Nmspace _stmttmpC=(*qv).f1;union Cyc_Absyn_Nmspace _tmp246=_stmttmpC;struct Cyc_List_List*_tmp247;struct Cyc_List_List*_tmp248;switch((_tmp246.C_n).tag){case 2U: _LL10: _tmp248=(_tmp246.Abs_n).val;_LL11: {struct Cyc_List_List*x=_tmp248;
vs=x;goto _LLF;}case 3U: _LL12: _tmp247=(_tmp246.C_n).val;_LL13: {struct Cyc_List_List*x=_tmp247;
vs=x;goto _LLF;}default: _LL14: _LL15:
({Cyc_Interface_err(({const char*_tmp249="bad namespace in Interface::add_namespace";_tag_fat(_tmp249,sizeof(char),42U);}));});return 0;}_LLF:;}
# 1012
vs=({Cyc_List_imp_rev(vs);});
({union Cyc_Absyn_Nmspace _tmp421=({Cyc_Absyn_Rel_n(0);});(*qv).f1=_tmp421;});
for(0;vs != 0;vs=vs->tl){
d=({({void*_tmp423=(void*)({struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct*_tmp24B=_cycalloc(sizeof(*_tmp24B));_tmp24B->tag=9U,_tmp24B->f1=(struct _fat_ptr*)vs->hd,({struct Cyc_List_List*_tmp422=({struct Cyc_List_List*_tmp24A=_cycalloc(sizeof(*_tmp24A));_tmp24A->hd=d,_tmp24A->tl=0;_tmp24A;});_tmp24B->f2=_tmp422;});_tmp24B;});Cyc_Absyn_new_decl(_tmp423,d->loc);});});}
ans=({struct Cyc_List_List*_tmp24C=_cycalloc(sizeof(*_tmp24C));_tmp24C->hd=d,_tmp24C->tl=ans;_tmp24C;});}}
# 1018
ans=({Cyc_List_imp_rev(ans);});
return ans;}
# 1022
static struct Cyc_List_List*Cyc_Interface_add_aggrdecl(struct _tuple0*x,struct Cyc_Absyn_Aggrdecl*d,struct Cyc_List_List*tds){
return({struct Cyc_List_List*_tmp24F=_cycalloc(sizeof(*_tmp24F));({struct Cyc_Absyn_Decl*_tmp424=({Cyc_Absyn_new_decl((void*)({struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct*_tmp24E=_cycalloc(sizeof(*_tmp24E));_tmp24E->tag=5U,_tmp24E->f1=d;_tmp24E;}),0U);});_tmp24F->hd=_tmp424;}),_tmp24F->tl=tds;_tmp24F;});}
# 1026
static struct Cyc_List_List*Cyc_Interface_add_aggrdecl_header(struct _tuple0*x,struct Cyc_Absyn_Aggrdecl*d,struct Cyc_List_List*tds){
# 1028
d=({struct Cyc_Absyn_Aggrdecl*_tmp251=_cycalloc(sizeof(*_tmp251));*_tmp251=*d;_tmp251;});
d->impl=0;
if((int)d->sc != (int)Cyc_Absyn_ExternC)d->sc=Cyc_Absyn_Extern;return({struct Cyc_List_List*_tmp253=_cycalloc(sizeof(*_tmp253));
({struct Cyc_Absyn_Decl*_tmp425=({Cyc_Absyn_new_decl((void*)({struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct*_tmp252=_cycalloc(sizeof(*_tmp252));_tmp252->tag=5U,_tmp252->f1=d;_tmp252;}),0U);});_tmp253->hd=_tmp425;}),_tmp253->tl=tds;_tmp253;});}
# 1034
static struct Cyc_List_List*Cyc_Interface_add_datatypedecl(struct _tuple0*x,struct Cyc_Absyn_Datatypedecl*d,struct Cyc_List_List*tds){
return({struct Cyc_List_List*_tmp256=_cycalloc(sizeof(*_tmp256));({struct Cyc_Absyn_Decl*_tmp426=({Cyc_Absyn_new_decl((void*)({struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct*_tmp255=_cycalloc(sizeof(*_tmp255));_tmp255->tag=6U,_tmp255->f1=d;_tmp255;}),0U);});_tmp256->hd=_tmp426;}),_tmp256->tl=tds;_tmp256;});}static char _tmp258[2U]="_";
# 1034
static struct _fat_ptr Cyc_Interface_us={_tmp258,_tmp258,_tmp258 + 2U};
# 1039
static struct _fat_ptr*Cyc_Interface_us_p=& Cyc_Interface_us;
# 1041
static struct _tuple25*Cyc_Interface_rewrite_datatypefield_type(struct _tuple25*x){
return({struct _tuple25*_tmp25B=_cycalloc(sizeof(*_tmp25B));({struct Cyc_Absyn_Tqual _tmp42A=({Cyc_Absyn_empty_tqual(0U);});_tmp25B->f1=_tmp42A;}),({
void*_tmp429=(void*)({struct Cyc_Absyn_VarType_Absyn_Type_struct*_tmp25A=_cycalloc(sizeof(*_tmp25A));_tmp25A->tag=2U,({struct Cyc_Absyn_Tvar*_tmp428=({struct Cyc_Absyn_Tvar*_tmp259=_cycalloc(sizeof(*_tmp259));_tmp259->name=Cyc_Interface_us_p,_tmp259->identity=- 1,({void*_tmp427=({Cyc_Tcutil_kind_to_bound(& Cyc_Tcutil_bk);});_tmp259->kind=_tmp427;});_tmp259;});_tmp25A->f1=_tmp428;});_tmp25A;});_tmp25B->f2=_tmp429;});_tmp25B;});}
# 1046
static struct Cyc_Absyn_Datatypefield*Cyc_Interface_rewrite_datatypefield(struct Cyc_Absyn_Datatypefield*f){
f=({struct Cyc_Absyn_Datatypefield*_tmp25D=_cycalloc(sizeof(*_tmp25D));*_tmp25D=*f;_tmp25D;});
({struct Cyc_List_List*_tmp42C=({({struct Cyc_List_List*(*_tmp42B)(struct _tuple25*(*f)(struct _tuple25*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp25E)(struct _tuple25*(*f)(struct _tuple25*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct _tuple25*(*f)(struct _tuple25*),struct Cyc_List_List*x))Cyc_List_map;_tmp25E;});_tmp42B(Cyc_Interface_rewrite_datatypefield_type,f->typs);});});f->typs=_tmp42C;});
return f;}
# 1052
static struct Cyc_List_List*Cyc_Interface_add_datatypedecl_header(struct _tuple0*x,struct Cyc_Absyn_Datatypedecl*d,struct Cyc_List_List*tds){
# 1054
d=({struct Cyc_Absyn_Datatypedecl*_tmp260=_cycalloc(sizeof(*_tmp260));*_tmp260=*d;_tmp260;});
# 1056
if(d->fields != 0)({struct Cyc_Core_Opt*_tmp42F=({struct Cyc_Core_Opt*_tmp262=_cycalloc(sizeof(*_tmp262));({struct Cyc_List_List*_tmp42E=({({struct Cyc_List_List*(*_tmp42D)(struct Cyc_Absyn_Datatypefield*(*f)(struct Cyc_Absyn_Datatypefield*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp261)(struct Cyc_Absyn_Datatypefield*(*f)(struct Cyc_Absyn_Datatypefield*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_Datatypefield*(*f)(struct Cyc_Absyn_Datatypefield*),struct Cyc_List_List*x))Cyc_List_map;_tmp261;});_tmp42D(Cyc_Interface_rewrite_datatypefield,(struct Cyc_List_List*)(d->fields)->v);});});_tmp262->v=_tmp42E;});_tmp262;});d->fields=_tmp42F;});if((int)d->sc != (int)Cyc_Absyn_ExternC)
d->sc=Cyc_Absyn_Extern;
# 1056
return({struct Cyc_List_List*_tmp264=_cycalloc(sizeof(*_tmp264));
# 1058
({struct Cyc_Absyn_Decl*_tmp430=({Cyc_Absyn_new_decl((void*)({struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct*_tmp263=_cycalloc(sizeof(*_tmp263));_tmp263->tag=6U,_tmp263->f1=d;_tmp263;}),0U);});_tmp264->hd=_tmp430;}),_tmp264->tl=tds;_tmp264;});}
# 1061
static struct Cyc_List_List*Cyc_Interface_add_enumdecl(struct _tuple0*x,struct Cyc_Absyn_Enumdecl*d,struct Cyc_List_List*tds){
return({struct Cyc_List_List*_tmp267=_cycalloc(sizeof(*_tmp267));({struct Cyc_Absyn_Decl*_tmp431=({Cyc_Absyn_new_decl((void*)({struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct*_tmp266=_cycalloc(sizeof(*_tmp266));_tmp266->tag=7U,_tmp266->f1=d;_tmp266;}),0U);});_tmp267->hd=_tmp431;}),_tmp267->tl=tds;_tmp267;});}
# 1065
static struct Cyc_List_List*Cyc_Interface_add_enumdecl_header(struct _tuple0*x,struct Cyc_Absyn_Enumdecl*d,struct Cyc_List_List*tds){
# 1067
d=({struct Cyc_Absyn_Enumdecl*_tmp269=_cycalloc(sizeof(*_tmp269));*_tmp269=*d;_tmp269;});
d->fields=0;
if((int)d->sc != (int)Cyc_Absyn_ExternC)d->sc=Cyc_Absyn_Extern;return({struct Cyc_List_List*_tmp26B=_cycalloc(sizeof(*_tmp26B));
({struct Cyc_Absyn_Decl*_tmp432=({Cyc_Absyn_new_decl((void*)({struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct*_tmp26A=_cycalloc(sizeof(*_tmp26A));_tmp26A->tag=7U,_tmp26A->f1=d;_tmp26A;}),0U);});_tmp26B->hd=_tmp432;}),_tmp26B->tl=tds;_tmp26B;});}
# 1073
static struct Cyc_List_List*Cyc_Interface_add_typedef(struct _tuple0*x,struct Cyc_Absyn_Typedefdecl*d,struct Cyc_List_List*tds){
# 1075
return({struct Cyc_List_List*_tmp26E=_cycalloc(sizeof(*_tmp26E));({struct Cyc_Absyn_Decl*_tmp433=({Cyc_Absyn_new_decl((void*)({struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct*_tmp26D=_cycalloc(sizeof(*_tmp26D));_tmp26D->tag=8U,_tmp26D->f1=d;_tmp26D;}),0U);});_tmp26E->hd=_tmp433;}),_tmp26E->tl=tds;_tmp26E;});}
# 1078
static struct Cyc_List_List*Cyc_Interface_add_vardecl(struct _tuple0*x,struct Cyc_Absyn_Vardecl*d,struct Cyc_List_List*tds){
return({struct Cyc_List_List*_tmp271=_cycalloc(sizeof(*_tmp271));({struct Cyc_Absyn_Decl*_tmp434=({Cyc_Absyn_new_decl((void*)({struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*_tmp270=_cycalloc(sizeof(*_tmp270));_tmp270->tag=0U,_tmp270->f1=d;_tmp270;}),0U);});_tmp271->hd=_tmp434;}),_tmp271->tl=tds;_tmp271;});}
# 1082
static struct Cyc_List_List*Cyc_Interface_add_xdatatypefielddecl(struct _tuple0*x,struct Cyc_Tcdecl_Xdatatypefielddecl*d,struct Cyc_List_List*tds){
# 1084
struct Cyc_Absyn_Datatypefield*_tmp274;struct Cyc_Absyn_Datatypedecl*_tmp273;_LL1: _tmp273=d->base;_tmp274=d->field;_LL2: {struct Cyc_Absyn_Datatypedecl*b=_tmp273;struct Cyc_Absyn_Datatypefield*f=_tmp274;
b=({struct Cyc_Absyn_Datatypedecl*_tmp275=_cycalloc(sizeof(*_tmp275));*_tmp275=*b;_tmp275;});
({struct Cyc_Core_Opt*_tmp436=({struct Cyc_Core_Opt*_tmp277=_cycalloc(sizeof(*_tmp277));({struct Cyc_List_List*_tmp435=({struct Cyc_List_List*_tmp276=_cycalloc(sizeof(*_tmp276));_tmp276->hd=f,_tmp276->tl=0;_tmp276;});_tmp277->v=_tmp435;});_tmp277;});b->fields=_tmp436;});
b->sc=Cyc_Absyn_Extern;
return({struct Cyc_List_List*_tmp279=_cycalloc(sizeof(*_tmp279));({struct Cyc_Absyn_Decl*_tmp437=({Cyc_Absyn_new_decl((void*)({struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct*_tmp278=_cycalloc(sizeof(*_tmp278));_tmp278->tag=6U,_tmp278->f1=b;_tmp278;}),0U);});_tmp279->hd=_tmp437;}),_tmp279->tl=tds;_tmp279;});}}
# 1091
static struct Cyc_List_List*Cyc_Interface_add_xdatatypefielddecl_header(struct _tuple0*x,struct Cyc_Tcdecl_Xdatatypefielddecl*d,struct Cyc_List_List*tds){
# 1093
struct Cyc_Absyn_Datatypefield*_tmp27C;struct Cyc_Absyn_Datatypedecl*_tmp27B;_LL1: _tmp27B=d->base;_tmp27C=d->field;_LL2: {struct Cyc_Absyn_Datatypedecl*b=_tmp27B;struct Cyc_Absyn_Datatypefield*f=_tmp27C;
b=({struct Cyc_Absyn_Datatypedecl*_tmp27D=_cycalloc(sizeof(*_tmp27D));*_tmp27D=*b;_tmp27D;});
f=({Cyc_Interface_rewrite_datatypefield(f);});
f->sc=Cyc_Absyn_Extern;
({struct Cyc_Core_Opt*_tmp439=({struct Cyc_Core_Opt*_tmp27F=_cycalloc(sizeof(*_tmp27F));({struct Cyc_List_List*_tmp438=({struct Cyc_List_List*_tmp27E=_cycalloc(sizeof(*_tmp27E));_tmp27E->hd=f,_tmp27E->tl=0;_tmp27E;});_tmp27F->v=_tmp438;});_tmp27F;});b->fields=_tmp439;});
b->sc=Cyc_Absyn_Extern;
return({struct Cyc_List_List*_tmp281=_cycalloc(sizeof(*_tmp281));({struct Cyc_Absyn_Decl*_tmp43A=({Cyc_Absyn_new_decl((void*)({struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct*_tmp280=_cycalloc(sizeof(*_tmp280));_tmp280->tag=6U,_tmp280->f1=b;_tmp280;}),0U);});_tmp281->hd=_tmp43A;}),_tmp281->tl=tds;_tmp281;});}}
# 1102
static void Cyc_Interface_print_ns_headers(struct Cyc___cycFILE*f,struct Cyc_Interface_Ienv*ie){
struct Cyc_List_List*tds=0;
tds=({({struct Cyc_List_List*(*_tmp43C)(struct Cyc_List_List*(*f)(struct _tuple0*,struct Cyc_Absyn_Aggrdecl*,struct Cyc_List_List*),struct Cyc_Dict_Dict d,struct Cyc_List_List*accum)=({struct Cyc_List_List*(*_tmp283)(struct Cyc_List_List*(*f)(struct _tuple0*,struct Cyc_Absyn_Aggrdecl*,struct Cyc_List_List*),struct Cyc_Dict_Dict d,struct Cyc_List_List*accum)=(struct Cyc_List_List*(*)(struct Cyc_List_List*(*f)(struct _tuple0*,struct Cyc_Absyn_Aggrdecl*,struct Cyc_List_List*),struct Cyc_Dict_Dict d,struct Cyc_List_List*accum))Cyc_Dict_fold;_tmp283;});struct Cyc_Dict_Dict _tmp43B=ie->aggrdecls;_tmp43C(Cyc_Interface_add_aggrdecl_header,_tmp43B,tds);});});
tds=({({struct Cyc_List_List*(*_tmp43E)(struct Cyc_List_List*(*f)(struct _tuple0*,struct Cyc_Absyn_Datatypedecl*,struct Cyc_List_List*),struct Cyc_Dict_Dict d,struct Cyc_List_List*accum)=({struct Cyc_List_List*(*_tmp284)(struct Cyc_List_List*(*f)(struct _tuple0*,struct Cyc_Absyn_Datatypedecl*,struct Cyc_List_List*),struct Cyc_Dict_Dict d,struct Cyc_List_List*accum)=(struct Cyc_List_List*(*)(struct Cyc_List_List*(*f)(struct _tuple0*,struct Cyc_Absyn_Datatypedecl*,struct Cyc_List_List*),struct Cyc_Dict_Dict d,struct Cyc_List_List*accum))Cyc_Dict_fold;_tmp284;});struct Cyc_Dict_Dict _tmp43D=ie->datatypedecls;_tmp43E(Cyc_Interface_add_datatypedecl_header,_tmp43D,tds);});});
tds=({({struct Cyc_List_List*(*_tmp440)(struct Cyc_List_List*(*f)(struct _tuple0*,struct Cyc_Absyn_Enumdecl*,struct Cyc_List_List*),struct Cyc_Dict_Dict d,struct Cyc_List_List*accum)=({struct Cyc_List_List*(*_tmp285)(struct Cyc_List_List*(*f)(struct _tuple0*,struct Cyc_Absyn_Enumdecl*,struct Cyc_List_List*),struct Cyc_Dict_Dict d,struct Cyc_List_List*accum)=(struct Cyc_List_List*(*)(struct Cyc_List_List*(*f)(struct _tuple0*,struct Cyc_Absyn_Enumdecl*,struct Cyc_List_List*),struct Cyc_Dict_Dict d,struct Cyc_List_List*accum))Cyc_Dict_fold;_tmp285;});struct Cyc_Dict_Dict _tmp43F=ie->enumdecls;_tmp440(Cyc_Interface_add_enumdecl_header,_tmp43F,tds);});});
# 1108
if(tds != 0){
tds=({Cyc_List_imp_rev(tds);});
tds=({Cyc_Interface_add_namespace(tds);});
({Cyc_Absynpp_decllist2file(tds,f);});}}
# 1115
static void Cyc_Interface_print_ns_xdatatypefielddecl_headers(struct Cyc___cycFILE*f,struct Cyc_Interface_Ienv*ie){
struct Cyc_List_List*tds=({({struct Cyc_List_List*(*_tmp441)(struct Cyc_List_List*(*f)(struct _tuple0*,struct Cyc_Tcdecl_Xdatatypefielddecl*,struct Cyc_List_List*),struct Cyc_Dict_Dict d,struct Cyc_List_List*accum)=({struct Cyc_List_List*(*_tmp287)(struct Cyc_List_List*(*f)(struct _tuple0*,struct Cyc_Tcdecl_Xdatatypefielddecl*,struct Cyc_List_List*),struct Cyc_Dict_Dict d,struct Cyc_List_List*accum)=(struct Cyc_List_List*(*)(struct Cyc_List_List*(*f)(struct _tuple0*,struct Cyc_Tcdecl_Xdatatypefielddecl*,struct Cyc_List_List*),struct Cyc_Dict_Dict d,struct Cyc_List_List*accum))Cyc_Dict_fold;_tmp287;});_tmp441(Cyc_Interface_add_xdatatypefielddecl_header,ie->xdatatypefielddecls,0);});});
if(tds != 0){
tds=({Cyc_List_imp_rev(tds);});
tds=({Cyc_Interface_add_namespace(tds);});
({Cyc_Absynpp_decllist2file(tds,f);});}}
# 1124
static void Cyc_Interface_print_ns_typedefs(struct Cyc___cycFILE*f,struct Cyc_Interface_Ienv*ie){
struct Cyc_List_List*tds=({({struct Cyc_List_List*(*_tmp442)(struct Cyc_List_List*(*f)(struct _tuple0*,struct Cyc_Absyn_Typedefdecl*,struct Cyc_List_List*),struct Cyc_Dict_Dict d,struct Cyc_List_List*accum)=({struct Cyc_List_List*(*_tmp289)(struct Cyc_List_List*(*f)(struct _tuple0*,struct Cyc_Absyn_Typedefdecl*,struct Cyc_List_List*),struct Cyc_Dict_Dict d,struct Cyc_List_List*accum)=(struct Cyc_List_List*(*)(struct Cyc_List_List*(*f)(struct _tuple0*,struct Cyc_Absyn_Typedefdecl*,struct Cyc_List_List*),struct Cyc_Dict_Dict d,struct Cyc_List_List*accum))Cyc_Dict_fold;_tmp289;});_tmp442(Cyc_Interface_add_typedef,ie->typedefdecls,0);});});
if(tds != 0){
tds=({Cyc_List_imp_rev(tds);});
tds=({Cyc_Interface_add_namespace(tds);});
({Cyc_Absynpp_decllist2file(tds,f);});}}
# 1133
static void Cyc_Interface_print_ns_decls(struct Cyc___cycFILE*f,struct Cyc_Interface_Ienv*ie){
struct Cyc_List_List*tds=0;
tds=({({struct Cyc_List_List*(*_tmp444)(struct Cyc_List_List*(*f)(struct _tuple0*,struct Cyc_Absyn_Aggrdecl*,struct Cyc_List_List*),struct Cyc_Dict_Dict d,struct Cyc_List_List*accum)=({struct Cyc_List_List*(*_tmp28B)(struct Cyc_List_List*(*f)(struct _tuple0*,struct Cyc_Absyn_Aggrdecl*,struct Cyc_List_List*),struct Cyc_Dict_Dict d,struct Cyc_List_List*accum)=(struct Cyc_List_List*(*)(struct Cyc_List_List*(*f)(struct _tuple0*,struct Cyc_Absyn_Aggrdecl*,struct Cyc_List_List*),struct Cyc_Dict_Dict d,struct Cyc_List_List*accum))Cyc_Dict_fold;_tmp28B;});struct Cyc_Dict_Dict _tmp443=ie->aggrdecls;_tmp444(Cyc_Interface_add_aggrdecl,_tmp443,tds);});});
tds=({({struct Cyc_List_List*(*_tmp446)(struct Cyc_List_List*(*f)(struct _tuple0*,struct Cyc_Absyn_Datatypedecl*,struct Cyc_List_List*),struct Cyc_Dict_Dict d,struct Cyc_List_List*accum)=({struct Cyc_List_List*(*_tmp28C)(struct Cyc_List_List*(*f)(struct _tuple0*,struct Cyc_Absyn_Datatypedecl*,struct Cyc_List_List*),struct Cyc_Dict_Dict d,struct Cyc_List_List*accum)=(struct Cyc_List_List*(*)(struct Cyc_List_List*(*f)(struct _tuple0*,struct Cyc_Absyn_Datatypedecl*,struct Cyc_List_List*),struct Cyc_Dict_Dict d,struct Cyc_List_List*accum))Cyc_Dict_fold;_tmp28C;});struct Cyc_Dict_Dict _tmp445=ie->datatypedecls;_tmp446(Cyc_Interface_add_datatypedecl,_tmp445,tds);});});
tds=({({struct Cyc_List_List*(*_tmp448)(struct Cyc_List_List*(*f)(struct _tuple0*,struct Cyc_Absyn_Enumdecl*,struct Cyc_List_List*),struct Cyc_Dict_Dict d,struct Cyc_List_List*accum)=({struct Cyc_List_List*(*_tmp28D)(struct Cyc_List_List*(*f)(struct _tuple0*,struct Cyc_Absyn_Enumdecl*,struct Cyc_List_List*),struct Cyc_Dict_Dict d,struct Cyc_List_List*accum)=(struct Cyc_List_List*(*)(struct Cyc_List_List*(*f)(struct _tuple0*,struct Cyc_Absyn_Enumdecl*,struct Cyc_List_List*),struct Cyc_Dict_Dict d,struct Cyc_List_List*accum))Cyc_Dict_fold;_tmp28D;});struct Cyc_Dict_Dict _tmp447=ie->enumdecls;_tmp448(Cyc_Interface_add_enumdecl,_tmp447,tds);});});
tds=({({struct Cyc_List_List*(*_tmp44A)(struct Cyc_List_List*(*f)(struct _tuple0*,struct Cyc_Absyn_Vardecl*,struct Cyc_List_List*),struct Cyc_Dict_Dict d,struct Cyc_List_List*accum)=({struct Cyc_List_List*(*_tmp28E)(struct Cyc_List_List*(*f)(struct _tuple0*,struct Cyc_Absyn_Vardecl*,struct Cyc_List_List*),struct Cyc_Dict_Dict d,struct Cyc_List_List*accum)=(struct Cyc_List_List*(*)(struct Cyc_List_List*(*f)(struct _tuple0*,struct Cyc_Absyn_Vardecl*,struct Cyc_List_List*),struct Cyc_Dict_Dict d,struct Cyc_List_List*accum))Cyc_Dict_fold;_tmp28E;});struct Cyc_Dict_Dict _tmp449=ie->vardecls;_tmp44A(Cyc_Interface_add_vardecl,_tmp449,tds);});});
tds=({({struct Cyc_List_List*(*_tmp44C)(struct Cyc_List_List*(*f)(struct _tuple0*,struct Cyc_Tcdecl_Xdatatypefielddecl*,struct Cyc_List_List*),struct Cyc_Dict_Dict d,struct Cyc_List_List*accum)=({struct Cyc_List_List*(*_tmp28F)(struct Cyc_List_List*(*f)(struct _tuple0*,struct Cyc_Tcdecl_Xdatatypefielddecl*,struct Cyc_List_List*),struct Cyc_Dict_Dict d,struct Cyc_List_List*accum)=(struct Cyc_List_List*(*)(struct Cyc_List_List*(*f)(struct _tuple0*,struct Cyc_Tcdecl_Xdatatypefielddecl*,struct Cyc_List_List*),struct Cyc_Dict_Dict d,struct Cyc_List_List*accum))Cyc_Dict_fold;_tmp28F;});struct Cyc_Dict_Dict _tmp44B=ie->xdatatypefielddecls;_tmp44C(Cyc_Interface_add_xdatatypefielddecl,_tmp44B,tds);});});
# 1141
if(tds != 0){
tds=({Cyc_List_imp_rev(tds);});
tds=({Cyc_Interface_add_namespace(tds);});
({Cyc_Absynpp_decllist2file(tds,f);});}}
# 1151
void Cyc_Interface_print(struct Cyc_Interface_I*i,struct Cyc___cycFILE*f){
({Cyc_Absynpp_set_params(& Cyc_Absynpp_cyci_params_r);});
({void*_tmp291=0U;({struct Cyc___cycFILE*_tmp44E=f;struct _fat_ptr _tmp44D=({const char*_tmp292="/****** needed (headers) ******/\n";_tag_fat(_tmp292,sizeof(char),34U);});Cyc_fprintf(_tmp44E,_tmp44D,_tag_fat(_tmp291,sizeof(void*),0U));});});
({Cyc_Interface_print_ns_headers(f,i->imports);});
# 1156
({void*_tmp293=0U;({struct Cyc___cycFILE*_tmp450=f;struct _fat_ptr _tmp44F=({const char*_tmp294="\n/****** provided (headers) ******/\n";_tag_fat(_tmp294,sizeof(char),37U);});Cyc_fprintf(_tmp450,_tmp44F,_tag_fat(_tmp293,sizeof(void*),0U));});});
({Cyc_Interface_print_ns_headers(f,i->exports);});
# 1161
({void*_tmp295=0U;({struct Cyc___cycFILE*_tmp452=f;struct _fat_ptr _tmp451=({const char*_tmp296="\n/****** needed (headers of xdatatypefielddecls) ******/\n";_tag_fat(_tmp296,sizeof(char),58U);});Cyc_fprintf(_tmp452,_tmp451,_tag_fat(_tmp295,sizeof(void*),0U));});});
({Cyc_Interface_print_ns_xdatatypefielddecl_headers(f,i->imports);});
# 1164
({void*_tmp297=0U;({struct Cyc___cycFILE*_tmp454=f;struct _fat_ptr _tmp453=({const char*_tmp298="\n/****** provided (headers of xdatatypefielddecls) ******/\n";_tag_fat(_tmp298,sizeof(char),60U);});Cyc_fprintf(_tmp454,_tmp453,_tag_fat(_tmp297,sizeof(void*),0U));});});
({Cyc_Interface_print_ns_xdatatypefielddecl_headers(f,i->exports);});
# 1168
({void*_tmp299=0U;({struct Cyc___cycFILE*_tmp456=f;struct _fat_ptr _tmp455=({const char*_tmp29A="\n/****** provided (typedefs) ******/\n";_tag_fat(_tmp29A,sizeof(char),38U);});Cyc_fprintf(_tmp456,_tmp455,_tag_fat(_tmp299,sizeof(void*),0U));});});
({Cyc_Interface_print_ns_typedefs(f,i->exports);});
# 1171
({void*_tmp29B=0U;({struct Cyc___cycFILE*_tmp458=f;struct _fat_ptr _tmp457=({const char*_tmp29C="\n/****** needed (declarations) ******/\n";_tag_fat(_tmp29C,sizeof(char),40U);});Cyc_fprintf(_tmp458,_tmp457,_tag_fat(_tmp29B,sizeof(void*),0U));});});
({Cyc_Interface_print_ns_decls(f,i->imports);});
# 1174
({void*_tmp29D=0U;({struct Cyc___cycFILE*_tmp45A=f;struct _fat_ptr _tmp459=({const char*_tmp29E="\n/****** provided (declarations) ******/\n";_tag_fat(_tmp29E,sizeof(char),42U);});Cyc_fprintf(_tmp45A,_tmp459,_tag_fat(_tmp29D,sizeof(void*),0U));});});
({Cyc_Interface_print_ns_decls(f,i->exports);});}
# 1151
struct Cyc_Interface_I*Cyc_Interface_parse(struct Cyc___cycFILE*f){
# 1182
({Cyc_Lex_lex_init();});{
struct Cyc_List_List*tds=({({struct Cyc___cycFILE*_tmp45B=f;Cyc_Parse_parse_file(_tmp45B,({const char*_tmp2A0="unknown file name";_tag_fat(_tmp2A0,sizeof(char),18U);}));});});
({Cyc_Binding_resolve_all(tds);});{
struct Cyc_Tcenv_Tenv*te=({Cyc_Tcenv_tc_init();});
({Cyc_Tc_tc(te,0,tds,0);});
return({Cyc_Interface_gen_extract(te->ae,0,tds);});}}}
# 1194
void Cyc_Interface_save(struct Cyc_Interface_I*i,struct Cyc___cycFILE*f){
({Cyc_Interface_print(i,f);});}
# 1194
struct Cyc_Interface_I*Cyc_Interface_load(struct Cyc___cycFILE*f){
# 1201
return({Cyc_Interface_parse(f);});}

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
int Cyc_List_length(struct Cyc_List_List*x);
# 76
struct Cyc_List_List*Cyc_List_map(void*(*f)(void*),struct Cyc_List_List*x);
# 83
struct Cyc_List_List*Cyc_List_map_c(void*(*f)(void*,void*),void*env,struct Cyc_List_List*x);extern char Cyc_List_List_mismatch[14U];struct Cyc_List_List_mismatch_exn_struct{char*tag;};
# 135
void Cyc_List_iter_c(void(*f)(void*,void*),void*env,struct Cyc_List_List*x);
# 172
struct Cyc_List_List*Cyc_List_rev(struct Cyc_List_List*x);
# 178
struct Cyc_List_List*Cyc_List_imp_rev(struct Cyc_List_List*x);
# 184
struct Cyc_List_List*Cyc_List_append(struct Cyc_List_List*x,struct Cyc_List_List*y);extern char Cyc_List_Nth[4U];struct Cyc_List_Nth_exn_struct{char*tag;};
# 261
int Cyc_List_exists_c(int(*pred)(void*,void*),void*env,struct Cyc_List_List*x);
# 265
void*Cyc_List_find_c(void*(*pred)(void*,void*),void*env,struct Cyc_List_List*x);
# 371
struct Cyc_List_List*Cyc_List_from_array(struct _fat_ptr arr);
# 394
struct Cyc_List_List*Cyc_List_filter_c(int(*f)(void*,void*),void*env,struct Cyc_List_List*x);
# 34 "position.h"
unsigned Cyc_Position_segment_join(unsigned,unsigned);struct Cyc_Position_Error;struct Cyc___cycFILE;struct Cyc_String_pa_PrintArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Int_pa_PrintArg_struct{int tag;unsigned long f1;};struct Cyc_Double_pa_PrintArg_struct{int tag;double f1;};struct Cyc_LongDouble_pa_PrintArg_struct{int tag;long double f1;};struct Cyc_ShortPtr_pa_PrintArg_struct{int tag;short*f1;};struct Cyc_IntPtr_pa_PrintArg_struct{int tag;unsigned long*f1;};
# 73 "cycboot.h"
struct _fat_ptr Cyc_aprintf(struct _fat_ptr,struct _fat_ptr);struct Cyc_ShortPtr_sa_ScanfArg_struct{int tag;short*f1;};struct Cyc_UShortPtr_sa_ScanfArg_struct{int tag;unsigned short*f1;};struct Cyc_IntPtr_sa_ScanfArg_struct{int tag;int*f1;};struct Cyc_UIntPtr_sa_ScanfArg_struct{int tag;unsigned*f1;};struct Cyc_StringPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_DoublePtr_sa_ScanfArg_struct{int tag;double*f1;};struct Cyc_FloatPtr_sa_ScanfArg_struct{int tag;float*f1;};struct Cyc_CharPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};
# 157 "cycboot.h"
int Cyc_printf(struct _fat_ptr,struct _fat_ptr);extern char Cyc_FileCloseError[15U];struct Cyc_FileCloseError_exn_struct{char*tag;};extern char Cyc_FileOpenError[14U];struct Cyc_FileOpenError_exn_struct{char*tag;struct _fat_ptr f1;};struct Cyc_Iter_Iter{void*env;int(*next)(void*env,void*dest);};struct Cyc_Dict_T;struct Cyc_Dict_Dict{int(*rel)(void*,void*);struct _RegionHandle*r;const struct Cyc_Dict_T*t;};extern char Cyc_Dict_Present[8U];struct Cyc_Dict_Present_exn_struct{char*tag;};extern char Cyc_Dict_Absent[7U];struct Cyc_Dict_Absent_exn_struct{char*tag;};struct Cyc_Relations_Reln;struct _union_Nmspace_Rel_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Abs_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_C_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Loc_n{int tag;int val;};union Cyc_Absyn_Nmspace{struct _union_Nmspace_Rel_n Rel_n;struct _union_Nmspace_Abs_n Abs_n;struct _union_Nmspace_C_n C_n;struct _union_Nmspace_Loc_n Loc_n;};
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
enum Cyc_Absyn_KindQual{Cyc_Absyn_AnyKind =0U,Cyc_Absyn_MemKind =1U,Cyc_Absyn_BoxKind =2U,Cyc_Absyn_XRgnKind =3U,Cyc_Absyn_RgnKind =4U,Cyc_Absyn_EffKind =5U,Cyc_Absyn_IntKind =6U,Cyc_Absyn_BoolKind =7U,Cyc_Absyn_PtrBndKind =8U};struct Cyc_Absyn_Kind{enum Cyc_Absyn_KindQual kind;enum Cyc_Absyn_AliasQual aliasqual;};struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct{int tag;struct Cyc_Absyn_Kind*f1;};struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct{int tag;struct Cyc_Core_Opt*f1;};struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct{int tag;struct Cyc_Core_Opt*f1;struct Cyc_Absyn_Kind*f2;};struct Cyc_Absyn_Tvar{struct _fat_ptr*name;int identity;void*kind;};struct Cyc_Absyn_PtrLoc{unsigned ptr_loc;unsigned rgn_loc;unsigned zt_loc;};struct Cyc_Absyn_PtrAtts{void*rgn;void*nullable;void*bounds;void*zero_term;struct Cyc_Absyn_PtrLoc*ptrloc;};struct Cyc_Absyn_PtrInfo{void*elt_type;struct Cyc_Absyn_Tqual elt_tq;struct Cyc_Absyn_PtrAtts ptr_atts;};struct Cyc_Absyn_VarargInfo{struct _fat_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;int inject;};struct Cyc_Absyn_Star_Absyn_Cap_struct{int tag;};struct Cyc_Absyn_Bot_Absyn_Cap_struct{int tag;};struct Cyc_Absyn_Nat_Absyn_Cap_struct{int tag;int f1;int f2;};struct Cyc_Absyn_RgnEffect{void*name;struct Cyc_List_List*caps;void*parent;};struct Cyc_Absyn_FnInfo{struct Cyc_List_List*tvars;void*effect;struct Cyc_Absyn_Tqual ret_tqual;void*ret_type;struct Cyc_List_List*args;int c_varargs;struct Cyc_Absyn_VarargInfo*cyc_varargs;struct Cyc_List_List*rgn_po;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*requires_clause;struct Cyc_List_List*requires_relns;struct Cyc_Absyn_Exp*ensures_clause;struct Cyc_List_List*ensures_relns;struct Cyc_List_List*ieffect;struct Cyc_List_List*oeffect;struct Cyc_List_List*throws;int reentrant;};struct Cyc_Absyn_UnknownDatatypeInfo{struct _tuple0*name;int is_extensible;};struct _union_DatatypeInfo_UnknownDatatype{int tag;struct Cyc_Absyn_UnknownDatatypeInfo val;};struct _union_DatatypeInfo_KnownDatatype{int tag;struct Cyc_Absyn_Datatypedecl**val;};union Cyc_Absyn_DatatypeInfo{struct _union_DatatypeInfo_UnknownDatatype UnknownDatatype;struct _union_DatatypeInfo_KnownDatatype KnownDatatype;};struct Cyc_Absyn_UnknownDatatypeFieldInfo{struct _tuple0*datatype_name;struct _tuple0*field_name;int is_extensible;};struct _union_DatatypeFieldInfo_UnknownDatatypefield{int tag;struct Cyc_Absyn_UnknownDatatypeFieldInfo val;};struct _tuple1{struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;};struct _union_DatatypeFieldInfo_KnownDatatypefield{int tag;struct _tuple1 val;};union Cyc_Absyn_DatatypeFieldInfo{struct _union_DatatypeFieldInfo_UnknownDatatypefield UnknownDatatypefield;struct _union_DatatypeFieldInfo_KnownDatatypefield KnownDatatypefield;};struct _tuple2{enum Cyc_Absyn_AggrKind f1;struct _tuple0*f2;struct Cyc_Core_Opt*f3;};struct _union_AggrInfo_UnknownAggr{int tag;struct _tuple2 val;};struct _union_AggrInfo_KnownAggr{int tag;struct Cyc_Absyn_Aggrdecl**val;};union Cyc_Absyn_AggrInfo{struct _union_AggrInfo_UnknownAggr UnknownAggr;struct _union_AggrInfo_KnownAggr KnownAggr;};
# 366
union Cyc_Absyn_AggrInfo Cyc_Absyn_UnknownAggr(enum Cyc_Absyn_AggrKind,struct _tuple0*,struct Cyc_Core_Opt*);struct Cyc_Absyn_ArrayInfo{void*elt_type;struct Cyc_Absyn_Tqual tq;struct Cyc_Absyn_Exp*num_elts;void*zero_term;unsigned zt_loc;};struct Cyc_Absyn_Aggr_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Enum_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;};struct Cyc_Absyn_Datatype_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;};struct Cyc_Absyn_TypeDecl{void*r;unsigned loc;};struct Cyc_Absyn_VoidCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_IntCon_Absyn_TyCon_struct{int tag;enum Cyc_Absyn_Sign f1;enum Cyc_Absyn_Size_of f2;};struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct{int tag;int f1;};struct Cyc_Absyn_RgnHandleCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_XRgnHandleCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_TagCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_HeapCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_UniqueCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_RefCntCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_AccessCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_CAccessCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_JoinCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_RgnsCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_TrueCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_FalseCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_ThinCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_FatCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct{int tag;struct _tuple0*f1;struct Cyc_Absyn_Enumdecl*f2;};struct Cyc_Absyn_AnonEnumCon_Absyn_TyCon_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_BuiltinCon_Absyn_TyCon_struct{int tag;struct _fat_ptr f1;struct Cyc_Absyn_Kind*f2;};struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_DatatypeInfo f1;};struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_DatatypeFieldInfo f1;};struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_AggrInfo f1;};struct Cyc_Absyn_AppType_Absyn_Type_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Evar_Absyn_Type_struct{int tag;struct Cyc_Core_Opt*f1;void*f2;int f3;struct Cyc_Core_Opt*f4;};struct Cyc_Absyn_VarType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Tvar*f1;};struct Cyc_Absyn_PointerType_Absyn_Type_struct{int tag;struct Cyc_Absyn_PtrInfo f1;};struct Cyc_Absyn_ArrayType_Absyn_Type_struct{int tag;struct Cyc_Absyn_ArrayInfo f1;};struct Cyc_Absyn_FnType_Absyn_Type_struct{int tag;struct Cyc_Absyn_FnInfo f1;};struct Cyc_Absyn_TupleType_Absyn_Type_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct{int tag;enum Cyc_Absyn_AggrKind f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_TypedefType_Absyn_Type_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_Typedefdecl*f3;void*f4;};struct Cyc_Absyn_ValueofType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct{int tag;struct Cyc_Absyn_TypeDecl*f1;void**f2;};struct Cyc_Absyn_TypeofType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_NoTypes_Absyn_Funcparams_struct{int tag;struct Cyc_List_List*f1;unsigned f2;};struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct{int tag;struct Cyc_List_List*f1;int f2;struct Cyc_Absyn_VarargInfo*f3;void*f4;struct Cyc_List_List*f5;struct Cyc_Absyn_Exp*f6;struct Cyc_Absyn_Exp*f7;struct Cyc_List_List*f8;struct Cyc_List_List*f9;struct Cyc_List_List*f10;int f11;};
# 478 "absyn.h"
enum Cyc_Absyn_Format_Type{Cyc_Absyn_Printf_ft =0U,Cyc_Absyn_Scanf_ft =1U};struct Cyc_Absyn_Regparm_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Stdcall_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Cdecl_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Fastcall_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Noreturn_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Const_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Aligned_att_Absyn_Attribute_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Packed_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Section_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Nocommon_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Shared_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Unused_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Weak_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Dllimport_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Dllexport_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_instrument_function_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Constructor_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Destructor_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_check_memory_usage_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Format_att_Absyn_Attribute_struct{int tag;enum Cyc_Absyn_Format_Type f1;int f2;int f3;};struct Cyc_Absyn_Initializes_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Noliveunique_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Consume_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Pure_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Mode_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Alias_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Always_inline_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_throw_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Non_null_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Deprecated_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_VectorSize_att_Absyn_Attribute_struct{int tag;int f1;};
# 518
extern struct Cyc_Absyn_Stdcall_att_Absyn_Attribute_struct Cyc_Absyn_Stdcall_att_val;
extern struct Cyc_Absyn_Cdecl_att_Absyn_Attribute_struct Cyc_Absyn_Cdecl_att_val;
extern struct Cyc_Absyn_Fastcall_att_Absyn_Attribute_struct Cyc_Absyn_Fastcall_att_val;
extern struct Cyc_Absyn_Noreturn_att_Absyn_Attribute_struct Cyc_Absyn_Noreturn_att_val;
extern struct Cyc_Absyn_Const_att_Absyn_Attribute_struct Cyc_Absyn_Const_att_val;
extern struct Cyc_Absyn_Packed_att_Absyn_Attribute_struct Cyc_Absyn_Packed_att_val;
# 525
extern struct Cyc_Absyn_Shared_att_Absyn_Attribute_struct Cyc_Absyn_Shared_att_val;
extern struct Cyc_Absyn_Unused_att_Absyn_Attribute_struct Cyc_Absyn_Unused_att_val;
extern struct Cyc_Absyn_Weak_att_Absyn_Attribute_struct Cyc_Absyn_Weak_att_val;
extern struct Cyc_Absyn_Dllimport_att_Absyn_Attribute_struct Cyc_Absyn_Dllimport_att_val;
extern struct Cyc_Absyn_Dllexport_att_Absyn_Attribute_struct Cyc_Absyn_Dllexport_att_val;
extern struct Cyc_Absyn_No_instrument_function_att_Absyn_Attribute_struct Cyc_Absyn_No_instrument_function_att_val;
extern struct Cyc_Absyn_Constructor_att_Absyn_Attribute_struct Cyc_Absyn_Constructor_att_val;
extern struct Cyc_Absyn_Destructor_att_Absyn_Attribute_struct Cyc_Absyn_Destructor_att_val;
extern struct Cyc_Absyn_No_check_memory_usage_att_Absyn_Attribute_struct Cyc_Absyn_No_check_memory_usage_att_val;
extern struct Cyc_Absyn_Pure_att_Absyn_Attribute_struct Cyc_Absyn_Pure_att_val;
extern struct Cyc_Absyn_Always_inline_att_Absyn_Attribute_struct Cyc_Absyn_Always_inline_att_val;
extern struct Cyc_Absyn_No_throw_att_Absyn_Attribute_struct Cyc_Absyn_No_throw_att_val;
extern struct Cyc_Absyn_Non_null_att_Absyn_Attribute_struct Cyc_Absyn_Non_null_att_val;
extern struct Cyc_Absyn_Deprecated_att_Absyn_Attribute_struct Cyc_Absyn_Deprecated_att_val;struct Cyc_Absyn_Carray_mod_Absyn_Type_modifier_struct{int tag;void*f1;unsigned f2;};struct Cyc_Absyn_ConstArray_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;unsigned f3;};struct Cyc_Absyn_Pointer_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_Absyn_PtrAtts f1;struct Cyc_Absyn_Tqual f2;};struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct{int tag;void*f1;};struct Cyc_Absyn_TypeParams_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_List_List*f1;unsigned f2;int f3;};struct Cyc_Absyn_Attributes_mod_Absyn_Type_modifier_struct{int tag;unsigned f1;struct Cyc_List_List*f2;};struct _union_Cnst_Null_c{int tag;int val;};struct _tuple3{enum Cyc_Absyn_Sign f1;char f2;};struct _union_Cnst_Char_c{int tag;struct _tuple3 val;};struct _union_Cnst_Wchar_c{int tag;struct _fat_ptr val;};struct _tuple4{enum Cyc_Absyn_Sign f1;short f2;};struct _union_Cnst_Short_c{int tag;struct _tuple4 val;};struct _tuple5{enum Cyc_Absyn_Sign f1;int f2;};struct _union_Cnst_Int_c{int tag;struct _tuple5 val;};struct _tuple6{enum Cyc_Absyn_Sign f1;long long f2;};struct _union_Cnst_LongLong_c{int tag;struct _tuple6 val;};struct _tuple7{struct _fat_ptr f1;int f2;};struct _union_Cnst_Float_c{int tag;struct _tuple7 val;};struct _union_Cnst_String_c{int tag;struct _fat_ptr val;};struct _union_Cnst_Wstring_c{int tag;struct _fat_ptr val;};union Cyc_Absyn_Cnst{struct _union_Cnst_Null_c Null_c;struct _union_Cnst_Char_c Char_c;struct _union_Cnst_Wchar_c Wchar_c;struct _union_Cnst_Short_c Short_c;struct _union_Cnst_Int_c Int_c;struct _union_Cnst_LongLong_c LongLong_c;struct _union_Cnst_Float_c Float_c;struct _union_Cnst_String_c String_c;struct _union_Cnst_Wstring_c Wstring_c;};
# 564
union Cyc_Absyn_Cnst Cyc_Absyn_Char_c(enum Cyc_Absyn_Sign,char);
union Cyc_Absyn_Cnst Cyc_Absyn_Wchar_c(struct _fat_ptr);
# 567
union Cyc_Absyn_Cnst Cyc_Absyn_Int_c(enum Cyc_Absyn_Sign,int);
# 569
union Cyc_Absyn_Cnst Cyc_Absyn_Float_c(struct _fat_ptr,int);
union Cyc_Absyn_Cnst Cyc_Absyn_String_c(struct _fat_ptr);
union Cyc_Absyn_Cnst Cyc_Absyn_Wstring_c(struct _fat_ptr);
# 574
enum Cyc_Absyn_Primop{Cyc_Absyn_Plus =0U,Cyc_Absyn_Times =1U,Cyc_Absyn_Minus =2U,Cyc_Absyn_Div =3U,Cyc_Absyn_Mod =4U,Cyc_Absyn_Eq =5U,Cyc_Absyn_Neq =6U,Cyc_Absyn_Gt =7U,Cyc_Absyn_Lt =8U,Cyc_Absyn_Gte =9U,Cyc_Absyn_Lte =10U,Cyc_Absyn_Not =11U,Cyc_Absyn_Bitnot =12U,Cyc_Absyn_Bitand =13U,Cyc_Absyn_Bitor =14U,Cyc_Absyn_Bitxor =15U,Cyc_Absyn_Bitlshift =16U,Cyc_Absyn_Bitlrshift =17U,Cyc_Absyn_Numelts =18U};
# 581
enum Cyc_Absyn_Incrementor{Cyc_Absyn_PreInc =0U,Cyc_Absyn_PostInc =1U,Cyc_Absyn_PreDec =2U,Cyc_Absyn_PostDec =3U};struct Cyc_Absyn_VarargCallInfo{int num_varargs;struct Cyc_List_List*injectors;struct Cyc_Absyn_VarargInfo*vai;};struct Cyc_Absyn_StructField_Absyn_OffsetofField_struct{int tag;struct _fat_ptr*f1;};struct Cyc_Absyn_TupleIndex_Absyn_OffsetofField_struct{int tag;unsigned f1;};
# 599
enum Cyc_Absyn_Coercion{Cyc_Absyn_Unknown_coercion =0U,Cyc_Absyn_No_coercion =1U,Cyc_Absyn_Null_to_NonNull =2U,Cyc_Absyn_Other_coercion =3U};struct Cyc_Absyn_MallocInfo{int is_calloc;struct Cyc_Absyn_Exp*rgn;void**elt_type;struct Cyc_Absyn_Exp*num_elts;int fat_result;int inline_call;};struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct{int tag;union Cyc_Absyn_Cnst f1;};struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Pragma_e_Absyn_Raw_exp_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct{int tag;enum Cyc_Absyn_Primop f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;enum Cyc_Absyn_Incrementor f2;};struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct _tuple8{struct Cyc_List_List*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_VarargCallInfo*f3;int f4;struct _tuple8*f5;};struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;int f2;};struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Exp*f2;int f3;enum Cyc_Absyn_Coercion f4;};struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;int f3;int f4;};struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;int f3;int f4;};struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Tuple_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;};struct _tuple9{struct _fat_ptr*f1;struct Cyc_Absyn_Tqual f2;void*f3;};struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct{int tag;struct _tuple9*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;int f4;};struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;int f3;};struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct Cyc_Absyn_Aggrdecl*f4;};struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Datatypedecl*f2;struct Cyc_Absyn_Datatypefield*f3;};struct Cyc_Absyn_Enum_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_Spawn_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;struct _tuple8*f3;};struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_MallocInfo f1;};struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Core_Opt*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Stmt*f1;};struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;};struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct{int tag;int f1;struct _fat_ptr f2;struct Cyc_List_List*f3;struct Cyc_List_List*f4;struct Cyc_List_List*f5;};struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Exp{void*topt;void*r;unsigned loc;void*annot;};struct Cyc_Absyn_Skip_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;struct Cyc_Absyn_Stmt*f3;};struct _tuple10{struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct{int tag;struct _tuple10 f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Break_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Continue_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Goto_s_Absyn_Raw_stmt_struct{int tag;struct _fat_ptr*f1;};struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _tuple10 f2;struct _tuple10 f3;struct Cyc_Absyn_Stmt*f4;};struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;void*f3;};struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Switch_clause**f2;};struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Decl*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct{int tag;struct _fat_ptr*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct _tuple10 f2;};struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_List_List*f2;void*f3;};
# 737 "absyn.h"
extern struct Cyc_Absyn_Skip_s_Absyn_Raw_stmt_struct Cyc_Absyn_Skip_s_val;struct Cyc_Absyn_Stmt{void*r;unsigned loc;void*annot;};struct Cyc_Absyn_Wild_p_Absyn_Raw_pat_struct{int tag;};struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_AliasVar_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_TagInt_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Tuple_p_Absyn_Raw_pat_struct{int tag;struct Cyc_List_List*f1;int f2;};struct Cyc_Absyn_Pointer_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Pat*f1;};struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct{int tag;union Cyc_Absyn_AggrInfo*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;int f4;};struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;struct Cyc_List_List*f3;int f4;};struct Cyc_Absyn_Null_p_Absyn_Raw_pat_struct{int tag;};struct Cyc_Absyn_Int_p_Absyn_Raw_pat_struct{int tag;enum Cyc_Absyn_Sign f1;int f2;};struct Cyc_Absyn_Char_p_Absyn_Raw_pat_struct{int tag;char f1;};struct Cyc_Absyn_Float_p_Absyn_Raw_pat_struct{int tag;struct _fat_ptr f1;int f2;};struct Cyc_Absyn_Enum_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_AnonEnum_p_Absyn_Raw_pat_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_UnknownId_p_Absyn_Raw_pat_struct{int tag;struct _tuple0*f1;};struct Cyc_Absyn_UnknownCall_p_Absyn_Raw_pat_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;int f3;};struct Cyc_Absyn_Exp_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Pat{void*r;void*topt;unsigned loc;};struct Cyc_Absyn_Switch_clause{struct Cyc_Absyn_Pat*pattern;struct Cyc_Core_Opt*pat_vars;struct Cyc_Absyn_Exp*where_clause;struct Cyc_Absyn_Stmt*body;unsigned loc;};struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct{int tag;struct _tuple0*f1;};struct Cyc_Absyn_Global_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Funname_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Param_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Local_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Pat_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Vardecl{enum Cyc_Absyn_Scope sc;struct _tuple0*name;unsigned varloc;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*initializer;void*rgn;struct Cyc_List_List*attributes;int escapes;};struct Cyc_Absyn_Fndecl{enum Cyc_Absyn_Scope sc;int is_inline;struct _tuple0*name;struct Cyc_Absyn_Stmt*body;struct Cyc_Absyn_FnInfo i;void*cached_type;struct Cyc_Core_Opt*param_vardecls;struct Cyc_Absyn_Vardecl*fn_vardecl;};struct Cyc_Absyn_Aggrfield{struct _fat_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*width;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*requires_clause;};struct Cyc_Absyn_AggrdeclImpl{struct Cyc_List_List*exist_vars;struct Cyc_List_List*rgn_po;struct Cyc_List_List*fields;int tagged;};struct Cyc_Absyn_Aggrdecl{enum Cyc_Absyn_AggrKind kind;enum Cyc_Absyn_Scope sc;struct _tuple0*name;struct Cyc_List_List*tvs;struct Cyc_Absyn_AggrdeclImpl*impl;struct Cyc_List_List*attributes;int expected_mem_kind;};struct Cyc_Absyn_Datatypefield{struct _tuple0*name;struct Cyc_List_List*typs;unsigned loc;enum Cyc_Absyn_Scope sc;};struct Cyc_Absyn_Datatypedecl{enum Cyc_Absyn_Scope sc;struct _tuple0*name;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*fields;int is_extensible;};struct Cyc_Absyn_Enumfield{struct _tuple0*name;struct Cyc_Absyn_Exp*tag;unsigned loc;};struct Cyc_Absyn_Enumdecl{enum Cyc_Absyn_Scope sc;struct _tuple0*name;struct Cyc_Core_Opt*fields;};struct Cyc_Absyn_Typedefdecl{struct _tuple0*name;struct Cyc_Absyn_Tqual tq;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*kind;void*defn;struct Cyc_List_List*atts;int extern_c;};struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Pat*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;void*f4;};struct Cyc_Absyn_Letv_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;};struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;};struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Typedefdecl*f1;};struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct{int tag;struct _fat_ptr*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_ExternC_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;};struct _tuple11{unsigned f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct _tuple11*f4;};struct Cyc_Absyn_Porton_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Portoff_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Tempeston_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Tempestoff_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Decl{void*r;unsigned loc;};struct Cyc_Absyn_ArrayElement_Absyn_Designator_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_FieldName_Absyn_Designator_struct{int tag;struct _fat_ptr*f1;};char Cyc_Absyn_EmptyAnnot[11U]="EmptyAnnot";struct Cyc_Absyn_EmptyAnnot_Absyn_AbsynAnnot_struct{char*tag;};
# 947
extern struct Cyc_Absyn_EmptyAnnot_Absyn_AbsynAnnot_struct Cyc_Absyn_EmptyAnnot_val;
# 954
int Cyc_Absyn_qvar_cmp(struct _tuple0*,struct _tuple0*);
# 962
struct Cyc_Absyn_Tqual Cyc_Absyn_const_tqual(unsigned);
struct Cyc_Absyn_Tqual Cyc_Absyn_empty_tqual(unsigned);
# 968
void*Cyc_Absyn_compress_kb(void*);
# 976
void*Cyc_Absyn_app_type(void*,struct _fat_ptr);
# 979
void*Cyc_Absyn_new_evar(struct Cyc_Core_Opt*k,struct Cyc_Core_Opt*tenv);
# 981
void*Cyc_Absyn_wildtyp(struct Cyc_Core_Opt*);
# 984
extern void*Cyc_Absyn_char_type;extern void*Cyc_Absyn_uchar_type;extern void*Cyc_Absyn_ushort_type;extern void*Cyc_Absyn_uint_type;extern void*Cyc_Absyn_ulong_type;extern void*Cyc_Absyn_ulonglong_type;
# 986
extern void*Cyc_Absyn_schar_type;extern void*Cyc_Absyn_sshort_type;extern void*Cyc_Absyn_sint_type;extern void*Cyc_Absyn_slong_type;extern void*Cyc_Absyn_slonglong_type;
# 988
extern void*Cyc_Absyn_float_type;extern void*Cyc_Absyn_double_type;extern void*Cyc_Absyn_long_double_type;
# 991
extern void*Cyc_Absyn_heap_rgn_type;extern void*Cyc_Absyn_unique_rgn_type;
# 995
extern void*Cyc_Absyn_true_type;extern void*Cyc_Absyn_false_type;
# 997
extern void*Cyc_Absyn_void_type;void*Cyc_Absyn_var_type(struct Cyc_Absyn_Tvar*);void*Cyc_Absyn_rgn_handle_type(void*);void*Cyc_Absyn_valueof_type(struct Cyc_Absyn_Exp*);void*Cyc_Absyn_join_eff(struct Cyc_List_List*);
# 1012
extern struct _tuple0*Cyc_Absyn_exn_name;
struct Cyc_Absyn_Datatypedecl*Cyc_Absyn_exn_tud();
# 1022
extern void*Cyc_Absyn_fat_bound_type;
void*Cyc_Absyn_thin_bounds_type(void*);
void*Cyc_Absyn_thin_bounds_exp(struct Cyc_Absyn_Exp*);
void*Cyc_Absyn_thin_bounds_int(unsigned);
void*Cyc_Absyn_bounds_one();
# 1028
void*Cyc_Absyn_pointer_type(struct Cyc_Absyn_PtrInfo);
# 1030
void*Cyc_Absyn_starb_type(void*t,void*rgn,struct Cyc_Absyn_Tqual,void*,void*zero_term);
# 1033
void*Cyc_Absyn_atb_type(void*t,void*rgn,struct Cyc_Absyn_Tqual tq,void*b,void*zero_term);
# 1036
void*Cyc_Absyn_star_type(void*t,void*rgn,struct Cyc_Absyn_Tqual,void*zero_term);
# 1038
void*Cyc_Absyn_at_type(void*t,void*rgn,struct Cyc_Absyn_Tqual,void*zero_term);
# 1053
void*Cyc_Absyn_datatype_type(union Cyc_Absyn_DatatypeInfo,struct Cyc_List_List*args);
# 1055
void*Cyc_Absyn_aggr_type(union Cyc_Absyn_AggrInfo,struct Cyc_List_List*args);
# 1058
struct Cyc_Absyn_Exp*Cyc_Absyn_new_exp(void*,unsigned);
# 1061
struct Cyc_Absyn_Exp*Cyc_Absyn_const_exp(union Cyc_Absyn_Cnst,unsigned);
# 1063
struct Cyc_Absyn_Exp*Cyc_Absyn_bool_exp(int,unsigned);
# 1066
struct Cyc_Absyn_Exp*Cyc_Absyn_int_exp(enum Cyc_Absyn_Sign,int,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_signed_int_exp(int,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_uint_exp(unsigned,unsigned);
# 1074
struct Cyc_Absyn_Exp*Cyc_Absyn_var_exp(struct _tuple0*,unsigned);
# 1078
struct Cyc_Absyn_Exp*Cyc_Absyn_primop_exp(enum Cyc_Absyn_Primop,struct Cyc_List_List*,unsigned);
# 1080
struct Cyc_Absyn_Exp*Cyc_Absyn_prim2_exp(enum Cyc_Absyn_Primop,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
# 1091
struct Cyc_Absyn_Exp*Cyc_Absyn_assignop_exp(struct Cyc_Absyn_Exp*,struct Cyc_Core_Opt*,struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_assign_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
# 1114
struct Cyc_Absyn_Exp*Cyc_Absyn_stmt_exp(struct Cyc_Absyn_Stmt*,unsigned);
# 1124
struct _tuple0*Cyc_Absyn_uniquergn_qvar();
# 1127
struct Cyc_Absyn_Stmt*Cyc_Absyn_new_stmt(void*,unsigned);
struct Cyc_Absyn_Stmt*Cyc_Absyn_skip_stmt(unsigned);
struct Cyc_Absyn_Stmt*Cyc_Absyn_exp_stmt(struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Stmt*Cyc_Absyn_seq_stmt(struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt*,unsigned);
struct Cyc_Absyn_Stmt*Cyc_Absyn_seq_stmts(struct Cyc_List_List*,unsigned);
# 1140
struct Cyc_Absyn_Stmt*Cyc_Absyn_decl_stmt(struct Cyc_Absyn_Decl*,struct Cyc_Absyn_Stmt*,unsigned);
# 1149
struct Cyc_Absyn_Pat*Cyc_Absyn_new_pat(void*,unsigned);
# 1153
struct Cyc_Absyn_Decl*Cyc_Absyn_new_decl(void*,unsigned);
# 1155
struct Cyc_Absyn_Decl*Cyc_Absyn_letv_decl(struct Cyc_List_List*,unsigned);
struct Cyc_Absyn_Decl*Cyc_Absyn_region_decl(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Vardecl*,struct Cyc_Absyn_Exp*open_exp,unsigned);
# 1158
struct Cyc_Absyn_Vardecl*Cyc_Absyn_new_vardecl(unsigned varloc,struct _tuple0*,void*,struct Cyc_Absyn_Exp*init);
# 1164
struct Cyc_Absyn_Decl*Cyc_Absyn_aggr_decl(enum Cyc_Absyn_AggrKind,enum Cyc_Absyn_Scope,struct _tuple0*,struct Cyc_List_List*ts,struct Cyc_Absyn_AggrdeclImpl*,struct Cyc_List_List*,unsigned);
# 1198
void*Cyc_Absyn_pointer_expand(void*,int fresh_evar);
# 1200
int Cyc_Absyn_is_lvalue(struct Cyc_Absyn_Exp*);
# 1203
struct Cyc_Absyn_Aggrfield*Cyc_Absyn_lookup_field(struct Cyc_List_List*,struct _fat_ptr*);
# 1211
struct _fat_ptr*Cyc_Absyn_decl_name(struct Cyc_Absyn_Decl*decl);
# 1219
int Cyc_Absyn_attribute_cmp(void*,void*);
# 1231
struct _tuple0*Cyc_Absyn_binding2qvar(void*);
# 1243
void Cyc_Absyn_visit_exp(int(*)(void*,struct Cyc_Absyn_Exp*),int(*)(void*,struct Cyc_Absyn_Stmt*),void*,struct Cyc_Absyn_Exp*);
# 1251
void Cyc_Absyn_visit_exp_pop(int(*)(void*,struct Cyc_Absyn_Exp*),void(*)(void*,struct Cyc_Absyn_Exp*),int(*)(void*,struct Cyc_Absyn_Stmt*),void(*)(void*,struct Cyc_Absyn_Stmt*),void*,struct Cyc_Absyn_Exp*);
# 1256
void Cyc_Absyn_visit_stmt_pop(int(*)(void*,struct Cyc_Absyn_Exp*),void(*)(void*,struct Cyc_Absyn_Exp*),int(*)(void*,struct Cyc_Absyn_Stmt*),void(*f)(void*,struct Cyc_Absyn_Stmt*),void*,struct Cyc_Absyn_Stmt*);
# 1278
int Cyc_Absyn_typ2int(void*t);
void*Cyc_Absyn_int2typ(int i);
struct Cyc_Absyn_Tvar*Cyc_Absyn_type2tvar(void*t);
# 1283
struct Cyc_List_List*Cyc_Absyn_add_qvar(struct Cyc_List_List*t,struct _tuple0*q);
# 1287
int Cyc_Absyn_is_xrgn_tvar(struct Cyc_Absyn_Tvar*tv);
int Cyc_Absyn_is_xrgn(void*r);
# 1290
struct _fat_ptr Cyc_Absyn_list2string(struct Cyc_List_List*l,struct _fat_ptr(*cb)(void*));
int Cyc_Absyn_xorptr(void*p1,void*p2);
# 1295
void*Cyc_Absyn_new_nat_cap(int i,int b);
void*Cyc_Absyn_bot_cap();
void*Cyc_Absyn_star_cap();
int Cyc_Absyn_is_bot_cap(void*c);
int Cyc_Absyn_is_star_cap(void*c);
# 1301
int Cyc_Absyn_is_nat_bar_cap(void*c);
# 1303
int Cyc_Absyn_get_nat_cap(void*c);
void*Cyc_Absyn_int2cap(int i);
int Cyc_Absyn_cap2int(void*c);
void*Cyc_Absyn_cap2type(void*c);
void*Cyc_Absyn_type2cap(void*c);
int Cyc_Absyn_equal_cap(void*c1,void*c2);
void*Cyc_Absyn_copy_cap(void*c);
struct _fat_ptr Cyc_Absyn_cap2string(void*c);
# 1314
int Cyc_Absyn_number_of_caps();
struct Cyc_List_List*Cyc_Absyn_caps(void*c1,void*c2);
void*Cyc_Absyn_rgn_cap(struct Cyc_List_List*c);
void*Cyc_Absyn_lock_cap(struct Cyc_List_List*c);
void*Cyc_Absyn_caps2type(struct Cyc_List_List*t);
int Cyc_Absyn_equal_caps(struct Cyc_List_List*c1,struct Cyc_List_List*c2);
struct Cyc_List_List*Cyc_Absyn_copy_caps(struct Cyc_List_List*c);
struct _fat_ptr Cyc_Absyn_caps2string(struct Cyc_List_List*c);struct _tuple12{int f1;int f2;int f3;};
# 1323
struct _tuple12 Cyc_Absyn_caps2tup(struct Cyc_List_List*);
# 1327
struct Cyc_Absyn_RgnEffect*Cyc_Absyn_new_rgneffect(void*t1,struct Cyc_List_List*c,void*t2);
# 1329
struct Cyc_List_List*Cyc_Absyn_rgneffect_rnames(struct Cyc_List_List*l,int);
struct Cyc_Absyn_RgnEffect*Cyc_Absyn_type2rgneffect(void*t);
void*Cyc_Absyn_rgneffect2type(struct Cyc_Absyn_RgnEffect*re);
# 1333
int Cyc_Absyn_rgneffect_cmp_name_tv(struct Cyc_Absyn_Tvar*x,struct Cyc_Absyn_RgnEffect*r2);
# 1337
struct Cyc_List_List*Cyc_Absyn_find_rgneffect_tail(struct Cyc_Absyn_Tvar*x,struct Cyc_List_List*l);
struct Cyc_Absyn_RgnEffect*Cyc_Absyn_find_rgneffect(struct Cyc_Absyn_Tvar*x,struct Cyc_List_List*l);
# 1340
int Cyc_Absyn_subset_rgneffect(struct Cyc_List_List*l,struct Cyc_List_List*l1);
int Cyc_Absyn_equal_rgneffect(struct Cyc_Absyn_RgnEffect*r1,struct Cyc_Absyn_RgnEffect*r2);
struct Cyc_List_List*Cyc_Absyn_filter_rgneffects(unsigned loc,void*t);
# 1347
struct _fat_ptr Cyc_Absyn_effect2string(struct Cyc_List_List*f);
struct _fat_ptr Cyc_Absyn_rgneffect2string(struct Cyc_Absyn_RgnEffect*r);
struct Cyc_Absyn_RgnEffect*Cyc_Absyn_copy_rgneffect(struct Cyc_Absyn_RgnEffect*eff);
# 1354
struct Cyc_Absyn_RgnEffect*Cyc_Absyn_rsubstitute_rgneffect(struct _RegionHandle*rgn,struct Cyc_List_List*inst,struct Cyc_Absyn_RgnEffect*eff);
# 1369
struct _tuple0*Cyc_Absyn_any_qvar();
# 1373
struct Cyc_List_List*Cyc_Absyn_throwsany();
int Cyc_Absyn_is_throwsany(struct Cyc_List_List*t);
# 1376
int Cyc_Absyn_is_nothrow(struct Cyc_List_List*t);
# 1379
int Cyc_Absyn_exists_throws(struct Cyc_List_List*throws,struct _tuple0*q);
# 1386
int Cyc_Absyn_is_exn_stmt(struct Cyc_Absyn_Stmt*s);
# 1401
extern const int Cyc_Absyn_reentrant;
# 1406
struct _fat_ptr Cyc_Absyn_typcon2string(void*t);
# 35 "warn.h"
void Cyc_Warn_err(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap);
# 40
void*Cyc_Warn_impos(struct _fat_ptr fmt,struct _fat_ptr ap);struct Cyc_Warn_String_Warn_Warg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Warn_Exp_Warn_Warg_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Warn_Typ_Warn_Warg_struct{int tag;void*f1;};struct Cyc_Warn_Qvar_Warn_Warg_struct{int tag;struct _tuple0*f1;};
# 38 "string.h"
unsigned long Cyc_strlen(struct _fat_ptr s);
# 49 "string.h"
int Cyc_strcmp(struct _fat_ptr s1,struct _fat_ptr s2);
int Cyc_strptrcmp(struct _fat_ptr*s1,struct _fat_ptr*s2);
# 52
int Cyc_zstrcmp(struct _fat_ptr,struct _fat_ptr);
# 109 "string.h"
struct _fat_ptr Cyc_substring(struct _fat_ptr,int ofs,unsigned long n);struct Cyc_Hashtable_Table;
# 47 "hashtable.h"
struct Cyc_Hashtable_Table*Cyc_Hashtable_rcreate(struct _RegionHandle*r,int sz,int(*cmp)(void*,void*),int(*hash)(void*));
# 50
void Cyc_Hashtable_insert(struct Cyc_Hashtable_Table*t,void*key,void*val);
# 56
void**Cyc_Hashtable_lookup_opt(struct Cyc_Hashtable_Table*t,void*key);struct _union_RelnOp_RConst{int tag;unsigned val;};struct _union_RelnOp_RVar{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RNumelts{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RType{int tag;void*val;};struct _union_RelnOp_RParam{int tag;unsigned val;};struct _union_RelnOp_RParamNumelts{int tag;unsigned val;};struct _union_RelnOp_RReturn{int tag;unsigned val;};union Cyc_Relations_RelnOp{struct _union_RelnOp_RConst RConst;struct _union_RelnOp_RVar RVar;struct _union_RelnOp_RNumelts RNumelts;struct _union_RelnOp_RType RType;struct _union_RelnOp_RParam RParam;struct _union_RelnOp_RParamNumelts RParamNumelts;struct _union_RelnOp_RReturn RReturn;};
# 50 "relations-ap.h"
enum Cyc_Relations_Relation{Cyc_Relations_Req =0U,Cyc_Relations_Rneq =1U,Cyc_Relations_Rlte =2U,Cyc_Relations_Rlt =3U};struct Cyc_Relations_Reln{union Cyc_Relations_RelnOp rop1;enum Cyc_Relations_Relation relation;union Cyc_Relations_RelnOp rop2;};struct Cyc_RgnOrder_RgnPO;extern char Cyc_Tcenv_Env_error[10U];struct Cyc_Tcenv_Env_error_exn_struct{char*tag;};struct Cyc_Tcenv_Genv{struct Cyc_Dict_Dict aggrdecls;struct Cyc_Dict_Dict datatypedecls;struct Cyc_Dict_Dict enumdecls;struct Cyc_Dict_Dict typedefs;struct Cyc_Dict_Dict ordinaries;};struct Cyc_Tcenv_Fenv;struct Cyc_Tcenv_Tenv{struct Cyc_List_List*ns;struct Cyc_Tcenv_Genv*ae;struct Cyc_Tcenv_Fenv*le;int allow_valueof: 1;int in_extern_c_include: 1;int in_tempest: 1;int tempest_generalize: 1;};
# 90 "tcenv.h"
enum Cyc_Tcenv_NewStatus{Cyc_Tcenv_NoneNew =0U,Cyc_Tcenv_InNew =1U,Cyc_Tcenv_InNewAggr =2U};
# 31 "tcutil.h"
void Cyc_Tcutil_terr(unsigned,struct _fat_ptr,struct _fat_ptr);
# 97
void*Cyc_Tcutil_copy_type(void*);
# 104
int Cyc_Tcutil_kind_eq(struct Cyc_Absyn_Kind*,struct Cyc_Absyn_Kind*);
# 110
void*Cyc_Tcutil_compress(void*);
# 138
extern struct Cyc_Absyn_Kind Cyc_Tcutil_xrk;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_rk;
# 141
extern struct Cyc_Absyn_Kind Cyc_Tcutil_bk;
# 170
extern struct Cyc_Core_Opt Cyc_Tcutil_tmko;
# 178
void*Cyc_Tcutil_kind_to_bound(struct Cyc_Absyn_Kind*k);
# 186
int Cyc_Tcutil_typecmp(void*,void*);
# 191
void*Cyc_Tcutil_rsubstitute(struct _RegionHandle*,struct Cyc_List_List*,void*);
# 219
void Cyc_Tcutil_check_unique_tvars(unsigned,struct Cyc_List_List*);
# 26 "cyclone.h"
enum Cyc_Cyclone_C_Compilers{Cyc_Cyclone_Gcc_c =0U,Cyc_Cyclone_Vc_c =1U};
# 32
extern enum Cyc_Cyclone_C_Compilers Cyc_Cyclone_c_compiler;
# 45 "evexp.h"
int Cyc_Evexp_const_exp_cmp(struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2);struct Cyc_Tcpat_TcPatResult{struct _tuple8*tvars_and_bounds_opt;struct Cyc_List_List*patvars;};struct Cyc_Tcpat_WhereTest_Tcpat_PatTest_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Tcpat_EqNull_Tcpat_PatTest_struct{int tag;};struct Cyc_Tcpat_NeqNull_Tcpat_PatTest_struct{int tag;};struct Cyc_Tcpat_EqEnum_Tcpat_PatTest_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Tcpat_EqAnonEnum_Tcpat_PatTest_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Tcpat_EqFloat_Tcpat_PatTest_struct{int tag;struct _fat_ptr f1;int f2;};struct Cyc_Tcpat_EqConst_Tcpat_PatTest_struct{int tag;unsigned f1;};struct Cyc_Tcpat_EqDatatypeTag_Tcpat_PatTest_struct{int tag;int f1;struct Cyc_Absyn_Datatypedecl*f2;struct Cyc_Absyn_Datatypefield*f3;};struct Cyc_Tcpat_EqTaggedUnion_Tcpat_PatTest_struct{int tag;struct _fat_ptr*f1;int f2;};struct Cyc_Tcpat_EqExtensibleDatatype_Tcpat_PatTest_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;};struct Cyc_Tcpat_Dummy_Tcpat_Access_struct{int tag;};struct Cyc_Tcpat_Deref_Tcpat_Access_struct{int tag;};struct Cyc_Tcpat_TupleField_Tcpat_Access_struct{int tag;unsigned f1;};struct Cyc_Tcpat_DatatypeField_Tcpat_Access_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;unsigned f3;};struct Cyc_Tcpat_AggrField_Tcpat_Access_struct{int tag;int f1;struct _fat_ptr*f2;};struct _union_PatOrWhere_pattern{int tag;struct Cyc_Absyn_Pat*val;};struct _union_PatOrWhere_where_clause{int tag;struct Cyc_Absyn_Exp*val;};union Cyc_Tcpat_PatOrWhere{struct _union_PatOrWhere_pattern pattern;struct _union_PatOrWhere_where_clause where_clause;};struct Cyc_Tcpat_PathNode{union Cyc_Tcpat_PatOrWhere orig_pat;void*access;};struct Cyc_Tcpat_Rhs{int used;unsigned pat_loc;struct Cyc_Absyn_Stmt*rhs;};struct Cyc_Tcpat_Failure_Tcpat_Decision_struct{int tag;void*f1;};struct Cyc_Tcpat_Success_Tcpat_Decision_struct{int tag;struct Cyc_Tcpat_Rhs*f1;};struct Cyc_Tcpat_SwitchDec_Tcpat_Decision_struct{int tag;struct Cyc_List_List*f1;struct Cyc_List_List*f2;void*f3;};
# 35 "absyn.cyc"
struct _fat_ptr Cyc_Absynpp_exp2string(struct Cyc_Absyn_Exp*e);
# 37
struct _fat_ptr Cyc_Absynpp_typ2string(void*);
struct _fat_ptr Cyc_Absynpp_qvar2string(struct _tuple0*);
struct _fat_ptr Cyc_Absynpp_kindbound2string(void*);
# 48
int Cyc_Cyclone_tovc_r=0;
# 50
enum Cyc_Cyclone_C_Compilers Cyc_Cyclone_c_compiler=Cyc_Cyclone_Gcc_c;
# 55
int Cyc_Flags_verbose=1;
int Cyc_Flags_warn_all_null_deref=1;
int Cyc_Flags_warn_bounds_checks=1;
int Cyc_Flags_warn_assert=1;
unsigned Cyc_Flags_max_vc_summary=500U;
int Cyc_Flags_allpaths=1;
int Cyc_Flags_better_widen=0;
# 69
static int Cyc_Absyn_strlist_cmp(struct Cyc_List_List*ss1,struct Cyc_List_List*ss2){
for(0;ss1 != 0;ss1=ss1->tl){
if(ss2 == 0)return 1;{int i=({Cyc_strptrcmp((struct _fat_ptr*)ss1->hd,(struct _fat_ptr*)ss2->hd);});
# 73
if(i != 0)return i;ss2=ss2->tl;}}
# 76
if(ss2 != 0)return - 1;return 0;}
# 79
int Cyc_Absyn_varlist_cmp(struct Cyc_List_List*vs1,struct Cyc_List_List*vs2){
if((int)vs1 == (int)vs2)return 0;return({Cyc_Absyn_strlist_cmp(vs1,vs2);});}struct _tuple13{union Cyc_Absyn_Nmspace f1;union Cyc_Absyn_Nmspace f2;};
# 83
int Cyc_Absyn_qvar_cmp(struct _tuple0*q1,struct _tuple0*q2){
if(q1 == q2)return 0;{struct _fat_ptr*_tmp3;union Cyc_Absyn_Nmspace _tmp2;_LL1: _tmp2=q1->f1;_tmp3=q1->f2;_LL2: {union Cyc_Absyn_Nmspace n1=_tmp2;struct _fat_ptr*v1=_tmp3;
# 86
struct _fat_ptr*_tmp5;union Cyc_Absyn_Nmspace _tmp4;_LL4: _tmp4=q2->f1;_tmp5=q2->f2;_LL5: {union Cyc_Absyn_Nmspace n2=_tmp4;struct _fat_ptr*v2=_tmp5;
int i=({Cyc_strptrcmp(v1,v2);});
if(i != 0)return i;{struct _tuple13 _stmttmp0=({struct _tuple13 _tmp607;_tmp607.f1=n1,_tmp607.f2=n2;_tmp607;});struct _tuple13 _tmp6=_stmttmp0;struct Cyc_List_List*_tmp8;struct Cyc_List_List*_tmp7;struct Cyc_List_List*_tmpA;struct Cyc_List_List*_tmp9;struct Cyc_List_List*_tmpC;struct Cyc_List_List*_tmpB;switch(((_tmp6.f1).Abs_n).tag){case 4U: if(((_tmp6.f2).Loc_n).tag == 4){_LL7: _LL8:
# 90
 return 0;}else{_LLF: _LL10:
# 95
 return - 1;}case 1U: switch(((_tmp6.f2).Loc_n).tag){case 1U: _LL9: _tmpB=((_tmp6.f1).Rel_n).val;_tmpC=((_tmp6.f2).Rel_n).val;_LLA: {struct Cyc_List_List*x1=_tmpB;struct Cyc_List_List*x2=_tmpC;
# 91
return({Cyc_Absyn_strlist_cmp(x1,x2);});}case 4U: goto _LL11;default: _LL13: _LL14:
# 97
 return - 1;}case 2U: switch(((_tmp6.f2).Rel_n).tag){case 2U: _LLB: _tmp9=((_tmp6.f1).Abs_n).val;_tmpA=((_tmp6.f2).Abs_n).val;_LLC: {struct Cyc_List_List*x1=_tmp9;struct Cyc_List_List*x2=_tmpA;
# 92
return({Cyc_Absyn_strlist_cmp(x1,x2);});}case 4U: goto _LL11;case 1U: goto _LL15;default: _LL17: _LL18:
# 99
 return - 1;}default: switch(((_tmp6.f2).Rel_n).tag){case 3U: _LLD: _tmp7=((_tmp6.f1).C_n).val;_tmp8=((_tmp6.f2).C_n).val;_LLE: {struct Cyc_List_List*x1=_tmp7;struct Cyc_List_List*x2=_tmp8;
# 93
return({Cyc_Absyn_strlist_cmp(x1,x2);});}case 4U: _LL11: _LL12:
# 96
 return 1;case 1U: _LL15: _LL16:
# 98
 return 1;default: _LL19: _LL1A:
# 100
 return 1;}}_LL6:;}}}}}
# 104
int Cyc_Absyn_tvar_cmp(struct Cyc_Absyn_Tvar*tv1,struct Cyc_Absyn_Tvar*tv2){
int i=({Cyc_strptrcmp(tv1->name,tv2->name);});
if(i != 0)return i;return tv1->identity - tv2->identity;}
# 104
union Cyc_Absyn_Nmspace Cyc_Absyn_Loc_n={.Loc_n={4,0}};
# 111
union Cyc_Absyn_Nmspace Cyc_Absyn_Abs_n(struct Cyc_List_List*x,int C_scope){
return C_scope?({union Cyc_Absyn_Nmspace _tmp608;(_tmp608.C_n).tag=3U,(_tmp608.C_n).val=x;_tmp608;}):({union Cyc_Absyn_Nmspace _tmp609;(_tmp609.Abs_n).tag=2U,(_tmp609.Abs_n).val=x;_tmp609;});}
# 111
union Cyc_Absyn_Nmspace Cyc_Absyn_Rel_n(struct Cyc_List_List*x){
# 114
return({union Cyc_Absyn_Nmspace _tmp60A;(_tmp60A.Rel_n).tag=1U,(_tmp60A.Rel_n).val=x;_tmp60A;});}
# 111
union Cyc_Absyn_Nmspace Cyc_Absyn_rel_ns_null={.Rel_n={1,0}};
# 117
int Cyc_Absyn_is_qvar_qualified(struct _tuple0*qv){
union Cyc_Absyn_Nmspace _stmttmp1=(*qv).f1;union Cyc_Absyn_Nmspace _tmp11=_stmttmp1;switch((_tmp11.Loc_n).tag){case 1U: if((_tmp11.Rel_n).val == 0){_LL1: _LL2:
 goto _LL4;}else{goto _LL7;}case 2U: if((_tmp11.Abs_n).val == 0){_LL3: _LL4:
 goto _LL6;}else{goto _LL7;}case 4U: _LL5: _LL6:
 return 0;default: _LL7: _LL8:
 return 1;}_LL0:;}
# 117
void*Cyc_Absyn_new_evar(struct Cyc_Core_Opt*k,struct Cyc_Core_Opt*env){
# 128
static int new_type_counter=0;
return(void*)({struct Cyc_Absyn_Evar_Absyn_Type_struct*_tmp13=_cycalloc(sizeof(*_tmp13));_tmp13->tag=1U,_tmp13->f1=k,_tmp13->f2=0,_tmp13->f3=new_type_counter ++,_tmp13->f4=env;_tmp13;});}
# 117
void*Cyc_Absyn_wildtyp(struct Cyc_Core_Opt*tenv){
# 132
return({Cyc_Absyn_new_evar(& Cyc_Tcutil_tmko,tenv);});}
# 117
struct Cyc_Absyn_Tqual Cyc_Absyn_empty_tqual(unsigned loc){
# 135
return({struct Cyc_Absyn_Tqual _tmp60B;_tmp60B.print_const=0,_tmp60B.q_volatile=0,_tmp60B.q_restrict=0,_tmp60B.real_const=0,_tmp60B.loc=loc;_tmp60B;});}
# 117
struct Cyc_Absyn_Tqual Cyc_Absyn_const_tqual(unsigned loc){
# 136
return({struct Cyc_Absyn_Tqual _tmp60C;_tmp60C.print_const=1,_tmp60C.q_volatile=0,_tmp60C.q_restrict=0,_tmp60C.real_const=1,_tmp60C.loc=loc;_tmp60C;});}
# 117
struct Cyc_Absyn_Tqual Cyc_Absyn_combine_tqual(struct Cyc_Absyn_Tqual x,struct Cyc_Absyn_Tqual y){
# 138
return({struct Cyc_Absyn_Tqual _tmp60D;_tmp60D.print_const=x.print_const || y.print_const,_tmp60D.q_volatile=
x.q_volatile || y.q_volatile,_tmp60D.q_restrict=
x.q_restrict || y.q_restrict,_tmp60D.real_const=
x.real_const || y.real_const,({
unsigned _tmp694=({Cyc_Position_segment_join(x.loc,y.loc);});_tmp60D.loc=_tmp694;});_tmp60D;});}
# 117
int Cyc_Absyn_equal_tqual(struct Cyc_Absyn_Tqual tq1,struct Cyc_Absyn_Tqual tq2){
# 145
return(tq1.real_const == tq2.real_const && tq1.q_volatile == tq2.q_volatile)&& tq1.q_restrict == tq2.q_restrict;}
# 117
struct Cyc_Absyn_EmptyAnnot_Absyn_AbsynAnnot_struct Cyc_Absyn_EmptyAnnot_val={Cyc_Absyn_EmptyAnnot};
# 152
union Cyc_Absyn_DatatypeInfo Cyc_Absyn_UnknownDatatype(struct Cyc_Absyn_UnknownDatatypeInfo udi){
return({union Cyc_Absyn_DatatypeInfo _tmp60E;(_tmp60E.UnknownDatatype).tag=1U,(_tmp60E.UnknownDatatype).val=udi;_tmp60E;});}
# 152
union Cyc_Absyn_DatatypeInfo Cyc_Absyn_KnownDatatype(struct Cyc_Absyn_Datatypedecl**d){
# 156
return({union Cyc_Absyn_DatatypeInfo _tmp60F;(_tmp60F.KnownDatatype).tag=2U,(_tmp60F.KnownDatatype).val=d;_tmp60F;});}
# 152
union Cyc_Absyn_DatatypeFieldInfo Cyc_Absyn_UnknownDatatypefield(struct Cyc_Absyn_UnknownDatatypeFieldInfo s){
# 159
return({union Cyc_Absyn_DatatypeFieldInfo _tmp610;(_tmp610.UnknownDatatypefield).tag=1U,(_tmp610.UnknownDatatypefield).val=s;_tmp610;});}
# 152
union Cyc_Absyn_DatatypeFieldInfo Cyc_Absyn_KnownDatatypefield(struct Cyc_Absyn_Datatypedecl*dd,struct Cyc_Absyn_Datatypefield*df){
# 162
return({union Cyc_Absyn_DatatypeFieldInfo _tmp611;(_tmp611.KnownDatatypefield).tag=2U,((_tmp611.KnownDatatypefield).val).f1=dd,((_tmp611.KnownDatatypefield).val).f2=df;_tmp611;});}
# 152
union Cyc_Absyn_AggrInfo Cyc_Absyn_UnknownAggr(enum Cyc_Absyn_AggrKind ak,struct _tuple0*n,struct Cyc_Core_Opt*tagged){
# 165
return({union Cyc_Absyn_AggrInfo _tmp612;(_tmp612.UnknownAggr).tag=1U,((_tmp612.UnknownAggr).val).f1=ak,((_tmp612.UnknownAggr).val).f2=n,((_tmp612.UnknownAggr).val).f3=tagged;_tmp612;});}
# 152
union Cyc_Absyn_AggrInfo Cyc_Absyn_KnownAggr(struct Cyc_Absyn_Aggrdecl**ad){
# 168
return({union Cyc_Absyn_AggrInfo _tmp613;(_tmp613.KnownAggr).tag=2U,(_tmp613.KnownAggr).val=ad;_tmp613;});}
# 152
void*Cyc_Absyn_compress_kb(void*k){
# 171
void*_tmp20=k;void**_tmp21;void**_tmp22;switch(*((int*)_tmp20)){case 0U: _LL1: _LL2:
 goto _LL4;case 1U: if(((struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*)_tmp20)->f1 == 0){_LL3: _LL4:
 goto _LL6;}else{_LL7: _tmp22=(void**)&(((struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*)_tmp20)->f1)->v;_LL8: {void**k2=_tmp22;
# 175
_tmp21=k2;goto _LLA;}}default: if(((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp20)->f1 == 0){_LL5: _LL6:
# 174
 return k;}else{_LL9: _tmp21=(void**)&(((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp20)->f1)->v;_LLA: {void**k2=_tmp21;
# 177
({void*_tmp695=({Cyc_Absyn_compress_kb(*k2);});*k2=_tmp695;});
return*k2;}}}_LL0:;}
# 152
struct Cyc_Absyn_Kind*Cyc_Absyn_force_kb(void*kb){
# 182
void*_stmttmp2=({Cyc_Absyn_compress_kb(kb);});void*_tmp24=_stmttmp2;struct Cyc_Absyn_Kind*_tmp26;struct Cyc_Core_Opt**_tmp25;struct Cyc_Core_Opt**_tmp27;struct Cyc_Absyn_Kind*_tmp28;switch(*((int*)_tmp24)){case 0U: _LL1: _tmp28=((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp24)->f1;_LL2: {struct Cyc_Absyn_Kind*k=_tmp28;
return k;}case 1U: _LL3: _tmp27=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*)_tmp24)->f1;_LL4: {struct Cyc_Core_Opt**f=_tmp27;
_tmp25=f;_tmp26=& Cyc_Tcutil_bk;goto _LL6;}default: _LL5: _tmp25=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp24)->f1;_tmp26=((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp24)->f2;_LL6: {struct Cyc_Core_Opt**f=_tmp25;struct Cyc_Absyn_Kind*k=_tmp26;
# 186
({struct Cyc_Core_Opt*_tmp697=({struct Cyc_Core_Opt*_tmp29=_cycalloc(sizeof(*_tmp29));({void*_tmp696=({Cyc_Tcutil_kind_to_bound(k);});_tmp29->v=_tmp696;});_tmp29;});*f=_tmp697;});
return k;}}_LL0:;}
# 152
void*Cyc_Absyn_app_type(void*c,struct _fat_ptr args){
# 193
return(void*)({struct Cyc_Absyn_AppType_Absyn_Type_struct*_tmp2B=_cycalloc(sizeof(*_tmp2B));_tmp2B->tag=0U,_tmp2B->f1=c,({struct Cyc_List_List*_tmp698=({Cyc_List_from_array(args);});_tmp2B->f2=_tmp698;});_tmp2B;});}
# 152
void*Cyc_Absyn_valueof_type(struct Cyc_Absyn_Exp*e){
# 196
return(void*)({struct Cyc_Absyn_ValueofType_Absyn_Type_struct*_tmp2D=_cycalloc(sizeof(*_tmp2D));_tmp2D->tag=9U,_tmp2D->f1=e;_tmp2D;});}
# 152
static struct Cyc_Absyn_VoidCon_Absyn_TyCon_struct Cyc_Absyn_void_type_cval={0U};
# 211 "absyn.cyc"
static struct Cyc_Absyn_AppType_Absyn_Type_struct Cyc_Absyn_void_type_tval={0U,(void*)& Cyc_Absyn_void_type_cval,0};void*Cyc_Absyn_void_type=(void*)& Cyc_Absyn_void_type_tval;
static struct Cyc_Absyn_HeapCon_Absyn_TyCon_struct Cyc_Absyn_heap_rgn_type_cval={6U};static struct Cyc_Absyn_AppType_Absyn_Type_struct Cyc_Absyn_heap_rgn_type_tval={0U,(void*)& Cyc_Absyn_heap_rgn_type_cval,0};void*Cyc_Absyn_heap_rgn_type=(void*)& Cyc_Absyn_heap_rgn_type_tval;
static struct Cyc_Absyn_UniqueCon_Absyn_TyCon_struct Cyc_Absyn_unique_rgn_type_cval={7U};static struct Cyc_Absyn_AppType_Absyn_Type_struct Cyc_Absyn_unique_rgn_type_tval={0U,(void*)& Cyc_Absyn_unique_rgn_type_cval,0};void*Cyc_Absyn_unique_rgn_type=(void*)& Cyc_Absyn_unique_rgn_type_tval;
static struct Cyc_Absyn_RefCntCon_Absyn_TyCon_struct Cyc_Absyn_refcnt_rgn_type_cval={8U};static struct Cyc_Absyn_AppType_Absyn_Type_struct Cyc_Absyn_refcnt_rgn_type_tval={0U,(void*)& Cyc_Absyn_refcnt_rgn_type_cval,0};void*Cyc_Absyn_refcnt_rgn_type=(void*)& Cyc_Absyn_refcnt_rgn_type_tval;
static struct Cyc_Absyn_TrueCon_Absyn_TyCon_struct Cyc_Absyn_true_type_cval={13U};static struct Cyc_Absyn_AppType_Absyn_Type_struct Cyc_Absyn_true_type_tval={0U,(void*)& Cyc_Absyn_true_type_cval,0};void*Cyc_Absyn_true_type=(void*)& Cyc_Absyn_true_type_tval;
static struct Cyc_Absyn_FalseCon_Absyn_TyCon_struct Cyc_Absyn_false_type_cval={14U};static struct Cyc_Absyn_AppType_Absyn_Type_struct Cyc_Absyn_false_type_tval={0U,(void*)& Cyc_Absyn_false_type_cval,0};void*Cyc_Absyn_false_type=(void*)& Cyc_Absyn_false_type_tval;
static struct Cyc_Absyn_FatCon_Absyn_TyCon_struct Cyc_Absyn_fat_bound_type_cval={16U};static struct Cyc_Absyn_AppType_Absyn_Type_struct Cyc_Absyn_fat_bound_type_tval={0U,(void*)& Cyc_Absyn_fat_bound_type_cval,0};void*Cyc_Absyn_fat_bound_type=(void*)& Cyc_Absyn_fat_bound_type_tval;
static struct Cyc_Absyn_IntCon_Absyn_TyCon_struct Cyc_Absyn_schar_type_cval={1U,Cyc_Absyn_Signed,Cyc_Absyn_Char_sz};static struct Cyc_Absyn_AppType_Absyn_Type_struct Cyc_Absyn_schar_type_tval={0U,(void*)& Cyc_Absyn_schar_type_cval,0};void*Cyc_Absyn_schar_type=(void*)& Cyc_Absyn_schar_type_tval;
static struct Cyc_Absyn_IntCon_Absyn_TyCon_struct Cyc_Absyn_uchar_type_cval={1U,Cyc_Absyn_Unsigned,Cyc_Absyn_Char_sz};static struct Cyc_Absyn_AppType_Absyn_Type_struct Cyc_Absyn_uchar_type_tval={0U,(void*)& Cyc_Absyn_uchar_type_cval,0};void*Cyc_Absyn_uchar_type=(void*)& Cyc_Absyn_uchar_type_tval;
static struct Cyc_Absyn_IntCon_Absyn_TyCon_struct Cyc_Absyn_char_type_cval={1U,Cyc_Absyn_None,Cyc_Absyn_Char_sz};static struct Cyc_Absyn_AppType_Absyn_Type_struct Cyc_Absyn_char_type_tval={0U,(void*)& Cyc_Absyn_char_type_cval,0};void*Cyc_Absyn_char_type=(void*)& Cyc_Absyn_char_type_tval;
static struct Cyc_Absyn_IntCon_Absyn_TyCon_struct Cyc_Absyn_sshort_type_cval={1U,Cyc_Absyn_Signed,Cyc_Absyn_Short_sz};static struct Cyc_Absyn_AppType_Absyn_Type_struct Cyc_Absyn_sshort_type_tval={0U,(void*)& Cyc_Absyn_sshort_type_cval,0};void*Cyc_Absyn_sshort_type=(void*)& Cyc_Absyn_sshort_type_tval;
static struct Cyc_Absyn_IntCon_Absyn_TyCon_struct Cyc_Absyn_ushort_type_cval={1U,Cyc_Absyn_Unsigned,Cyc_Absyn_Short_sz};static struct Cyc_Absyn_AppType_Absyn_Type_struct Cyc_Absyn_ushort_type_tval={0U,(void*)& Cyc_Absyn_ushort_type_cval,0};void*Cyc_Absyn_ushort_type=(void*)& Cyc_Absyn_ushort_type_tval;
static struct Cyc_Absyn_IntCon_Absyn_TyCon_struct Cyc_Absyn_nshort_type_cval={1U,Cyc_Absyn_None,Cyc_Absyn_Short_sz};static struct Cyc_Absyn_AppType_Absyn_Type_struct Cyc_Absyn_nshort_type_tval={0U,(void*)& Cyc_Absyn_nshort_type_cval,0};void*Cyc_Absyn_nshort_type=(void*)& Cyc_Absyn_nshort_type_tval;
static struct Cyc_Absyn_IntCon_Absyn_TyCon_struct Cyc_Absyn_sint_type_cval={1U,Cyc_Absyn_Signed,Cyc_Absyn_Int_sz};static struct Cyc_Absyn_AppType_Absyn_Type_struct Cyc_Absyn_sint_type_tval={0U,(void*)& Cyc_Absyn_sint_type_cval,0};void*Cyc_Absyn_sint_type=(void*)& Cyc_Absyn_sint_type_tval;
static struct Cyc_Absyn_IntCon_Absyn_TyCon_struct Cyc_Absyn_uint_type_cval={1U,Cyc_Absyn_Unsigned,Cyc_Absyn_Int_sz};static struct Cyc_Absyn_AppType_Absyn_Type_struct Cyc_Absyn_uint_type_tval={0U,(void*)& Cyc_Absyn_uint_type_cval,0};void*Cyc_Absyn_uint_type=(void*)& Cyc_Absyn_uint_type_tval;
static struct Cyc_Absyn_IntCon_Absyn_TyCon_struct Cyc_Absyn_nint_type_cval={1U,Cyc_Absyn_None,Cyc_Absyn_Int_sz};static struct Cyc_Absyn_AppType_Absyn_Type_struct Cyc_Absyn_nint_type_tval={0U,(void*)& Cyc_Absyn_nint_type_cval,0};void*Cyc_Absyn_nint_type=(void*)& Cyc_Absyn_nint_type_tval;
static struct Cyc_Absyn_IntCon_Absyn_TyCon_struct Cyc_Absyn_slong_type_cval={1U,Cyc_Absyn_Signed,Cyc_Absyn_Long_sz};static struct Cyc_Absyn_AppType_Absyn_Type_struct Cyc_Absyn_slong_type_tval={0U,(void*)& Cyc_Absyn_slong_type_cval,0};void*Cyc_Absyn_slong_type=(void*)& Cyc_Absyn_slong_type_tval;
static struct Cyc_Absyn_IntCon_Absyn_TyCon_struct Cyc_Absyn_ulong_type_cval={1U,Cyc_Absyn_Unsigned,Cyc_Absyn_Long_sz};static struct Cyc_Absyn_AppType_Absyn_Type_struct Cyc_Absyn_ulong_type_tval={0U,(void*)& Cyc_Absyn_ulong_type_cval,0};void*Cyc_Absyn_ulong_type=(void*)& Cyc_Absyn_ulong_type_tval;
static struct Cyc_Absyn_IntCon_Absyn_TyCon_struct Cyc_Absyn_nlong_type_cval={1U,Cyc_Absyn_None,Cyc_Absyn_Long_sz};static struct Cyc_Absyn_AppType_Absyn_Type_struct Cyc_Absyn_nlong_type_tval={0U,(void*)& Cyc_Absyn_nlong_type_cval,0};void*Cyc_Absyn_nlong_type=(void*)& Cyc_Absyn_nlong_type_tval;
static struct Cyc_Absyn_IntCon_Absyn_TyCon_struct Cyc_Absyn_slonglong_type_cval={1U,Cyc_Absyn_Signed,Cyc_Absyn_LongLong_sz};static struct Cyc_Absyn_AppType_Absyn_Type_struct Cyc_Absyn_slonglong_type_tval={0U,(void*)& Cyc_Absyn_slonglong_type_cval,0};void*Cyc_Absyn_slonglong_type=(void*)& Cyc_Absyn_slonglong_type_tval;
static struct Cyc_Absyn_IntCon_Absyn_TyCon_struct Cyc_Absyn_ulonglong_type_cval={1U,Cyc_Absyn_Unsigned,Cyc_Absyn_LongLong_sz};static struct Cyc_Absyn_AppType_Absyn_Type_struct Cyc_Absyn_ulonglong_type_tval={0U,(void*)& Cyc_Absyn_ulonglong_type_cval,0};void*Cyc_Absyn_ulonglong_type=(void*)& Cyc_Absyn_ulonglong_type_tval;
static struct Cyc_Absyn_IntCon_Absyn_TyCon_struct Cyc_Absyn_nlonglong_type_cval={1U,Cyc_Absyn_None,Cyc_Absyn_LongLong_sz};static struct Cyc_Absyn_AppType_Absyn_Type_struct Cyc_Absyn_nlonglong_type_tval={0U,(void*)& Cyc_Absyn_nlonglong_type_cval,0};void*Cyc_Absyn_nlonglong_type=(void*)& Cyc_Absyn_nlonglong_type_tval;
static struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct Cyc_Absyn_float_type_cval={2U,0};static struct Cyc_Absyn_AppType_Absyn_Type_struct Cyc_Absyn_float_type_tval={0U,(void*)& Cyc_Absyn_float_type_cval,0};void*Cyc_Absyn_float_type=(void*)& Cyc_Absyn_float_type_tval;
static struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct Cyc_Absyn_double_type_cval={2U,1};static struct Cyc_Absyn_AppType_Absyn_Type_struct Cyc_Absyn_double_type_tval={0U,(void*)& Cyc_Absyn_double_type_cval,0};void*Cyc_Absyn_double_type=(void*)& Cyc_Absyn_double_type_tval;
static struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct Cyc_Absyn_long_double_type_cval={2U,2};static struct Cyc_Absyn_AppType_Absyn_Type_struct Cyc_Absyn_long_double_type_tval={0U,(void*)& Cyc_Absyn_long_double_type_cval,0};void*Cyc_Absyn_long_double_type=(void*)& Cyc_Absyn_long_double_type_tval;
# 237
static struct Cyc_Absyn_JoinCon_Absyn_TyCon_struct Cyc_Absyn_empty_effect_cval={11U};static struct Cyc_Absyn_AppType_Absyn_Type_struct Cyc_Absyn_empty_effect_tval={0U,(void*)& Cyc_Absyn_empty_effect_cval,0};void*Cyc_Absyn_empty_effect=(void*)& Cyc_Absyn_empty_effect_tval;
# 239
static struct Cyc_Absyn_RgnHandleCon_Absyn_TyCon_struct Cyc_Absyn_RgnHandleCon_val={3U};
static struct Cyc_Absyn_TagCon_Absyn_TyCon_struct Cyc_Absyn_TagCon_val={5U};
static struct Cyc_Absyn_AccessCon_Absyn_TyCon_struct Cyc_Absyn_AccessCon_val={9U};
static struct Cyc_Absyn_RgnsCon_Absyn_TyCon_struct Cyc_Absyn_RgnsCon_val={12U};
static struct Cyc_Absyn_ThinCon_Absyn_TyCon_struct Cyc_Absyn_ThinCon_val={15U};
static struct Cyc_Absyn_JoinCon_Absyn_TyCon_struct Cyc_Absyn_JoinCon_val={11U};
# 246
void*Cyc_Absyn_rgn_handle_type(void*r){return({void*_tmp2F[1U];_tmp2F[0]=r;Cyc_Absyn_app_type((void*)& Cyc_Absyn_RgnHandleCon_val,_tag_fat(_tmp2F,sizeof(void*),1U));});}void*Cyc_Absyn_tag_type(void*t){
return({void*_tmp31[1U];_tmp31[0]=t;Cyc_Absyn_app_type((void*)& Cyc_Absyn_TagCon_val,_tag_fat(_tmp31,sizeof(void*),1U));});}
# 246
void*Cyc_Absyn_access_eff(void*r){
# 248
return({void*_tmp33[1U];_tmp33[0]=r;Cyc_Absyn_app_type((void*)& Cyc_Absyn_AccessCon_val,_tag_fat(_tmp33,sizeof(void*),1U));});}
# 246
void*Cyc_Absyn_regionsof_eff(void*t){
# 249
return({void*_tmp35[1U];_tmp35[0]=t;Cyc_Absyn_app_type((void*)& Cyc_Absyn_RgnsCon_val,_tag_fat(_tmp35,sizeof(void*),1U));});}
# 246
void*Cyc_Absyn_thin_bounds_type(void*t){
# 250
return({void*_tmp37[1U];_tmp37[0]=t;Cyc_Absyn_app_type((void*)& Cyc_Absyn_ThinCon_val,_tag_fat(_tmp37,sizeof(void*),1U));});}
# 246
void*Cyc_Absyn_join_eff(struct Cyc_List_List*ts){
# 251
return(void*)({struct Cyc_Absyn_AppType_Absyn_Type_struct*_tmp39=_cycalloc(sizeof(*_tmp39));_tmp39->tag=0U,_tmp39->f1=(void*)& Cyc_Absyn_empty_effect_cval,_tmp39->f2=ts;_tmp39;});}
# 246
void*Cyc_Absyn_enum_type(struct _tuple0*n,struct Cyc_Absyn_Enumdecl*d){
# 254
return({void*_tmp3B=0U;({void*_tmp699=(void*)({struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct*_tmp3C=_cycalloc(sizeof(*_tmp3C));_tmp3C->tag=17U,_tmp3C->f1=n,_tmp3C->f2=d;_tmp3C;});Cyc_Absyn_app_type(_tmp699,_tag_fat(_tmp3B,sizeof(void*),0U));});});}
# 246
void*Cyc_Absyn_anon_enum_type(struct Cyc_List_List*fs){
# 257
return({void*_tmp3E=0U;({void*_tmp69A=(void*)({struct Cyc_Absyn_AnonEnumCon_Absyn_TyCon_struct*_tmp3F=_cycalloc(sizeof(*_tmp3F));_tmp3F->tag=18U,_tmp3F->f1=fs;_tmp3F;});Cyc_Absyn_app_type(_tmp69A,_tag_fat(_tmp3E,sizeof(void*),0U));});});}
# 246
void*Cyc_Absyn_builtin_type(struct _fat_ptr s,struct Cyc_Absyn_Kind*k){
# 260
return({void*_tmp41=0U;({void*_tmp69B=(void*)({struct Cyc_Absyn_BuiltinCon_Absyn_TyCon_struct*_tmp42=_cycalloc(sizeof(*_tmp42));_tmp42->tag=19U,_tmp42->f1=s,_tmp42->f2=k;_tmp42;});Cyc_Absyn_app_type(_tmp69B,_tag_fat(_tmp41,sizeof(void*),0U));});});}
# 246
void*Cyc_Absyn_datatype_type(union Cyc_Absyn_DatatypeInfo di,struct Cyc_List_List*args){
# 263
return(void*)({struct Cyc_Absyn_AppType_Absyn_Type_struct*_tmp45=_cycalloc(sizeof(*_tmp45));_tmp45->tag=0U,({void*_tmp69C=(void*)({struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*_tmp44=_cycalloc(sizeof(*_tmp44));_tmp44->tag=20U,_tmp44->f1=di;_tmp44;});_tmp45->f1=_tmp69C;}),_tmp45->f2=args;_tmp45;});}
# 246
void*Cyc_Absyn_datatype_field_type(union Cyc_Absyn_DatatypeFieldInfo di,struct Cyc_List_List*args){
# 266
return(void*)({struct Cyc_Absyn_AppType_Absyn_Type_struct*_tmp48=_cycalloc(sizeof(*_tmp48));_tmp48->tag=0U,({void*_tmp69D=(void*)({struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*_tmp47=_cycalloc(sizeof(*_tmp47));_tmp47->tag=21U,_tmp47->f1=di;_tmp47;});_tmp48->f1=_tmp69D;}),_tmp48->f2=args;_tmp48;});}
# 246
void*Cyc_Absyn_aggr_type(union Cyc_Absyn_AggrInfo ai,struct Cyc_List_List*args){
# 269
return(void*)({struct Cyc_Absyn_AppType_Absyn_Type_struct*_tmp4B=_cycalloc(sizeof(*_tmp4B));_tmp4B->tag=0U,({void*_tmp69E=(void*)({struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*_tmp4A=_cycalloc(sizeof(*_tmp4A));_tmp4A->tag=22U,_tmp4A->f1=ai;_tmp4A;});_tmp4B->f1=_tmp69E;}),_tmp4B->f2=args;_tmp4B;});}
# 246
void*Cyc_Absyn_var_type(struct Cyc_Absyn_Tvar*x){
# 272
return(void*)({struct Cyc_Absyn_VarType_Absyn_Type_struct*_tmp4D=_cycalloc(sizeof(*_tmp4D));_tmp4D->tag=2U,_tmp4D->f1=x;_tmp4D;});}
# 246
void*Cyc_Absyn_gen_float_type(unsigned i){
# 275
unsigned _tmp4F=i;switch(_tmp4F){case 0U: _LL1: _LL2:
 return Cyc_Absyn_float_type;case 1U: _LL3: _LL4:
 return Cyc_Absyn_double_type;case 2U: _LL5: _LL6:
 return Cyc_Absyn_long_double_type;default: _LL7: _LL8:
({struct Cyc_Int_pa_PrintArg_struct _tmp53=({struct Cyc_Int_pa_PrintArg_struct _tmp614;_tmp614.tag=1U,_tmp614.f1=(unsigned long)((int)i);_tmp614;});void*_tmp50[1U];_tmp50[0]=& _tmp53;({int(*_tmp6A0)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp52)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp52;});struct _fat_ptr _tmp69F=({const char*_tmp51="gen_float_type(%d)";_tag_fat(_tmp51,sizeof(char),19U);});_tmp6A0(_tmp69F,_tag_fat(_tmp50,sizeof(void*),1U));});});}_LL0:;}
# 246
void*Cyc_Absyn_int_type(enum Cyc_Absyn_Sign sn,enum Cyc_Absyn_Size_of sz){
# 283
enum Cyc_Absyn_Sign _tmp55=sn;switch(_tmp55){case Cyc_Absyn_Signed: _LL1: _LL2: {
# 285
enum Cyc_Absyn_Size_of _tmp56=sz;switch(_tmp56){case Cyc_Absyn_Char_sz: _LLA: _LLB:
 return Cyc_Absyn_schar_type;case Cyc_Absyn_Short_sz: _LLC: _LLD:
 return Cyc_Absyn_sshort_type;case Cyc_Absyn_Int_sz: _LLE: _LLF:
 return Cyc_Absyn_sint_type;case Cyc_Absyn_Long_sz: _LL10: _LL11:
 return Cyc_Absyn_slong_type;case Cyc_Absyn_LongLong_sz: _LL12: _LL13:
 goto _LL15;default: _LL14: _LL15:
 return Cyc_Absyn_slonglong_type;}_LL9:;}case Cyc_Absyn_Unsigned: _LL3: _LL4: {
# 294
enum Cyc_Absyn_Size_of _tmp57=sz;switch(_tmp57){case Cyc_Absyn_Char_sz: _LL17: _LL18:
 return Cyc_Absyn_uchar_type;case Cyc_Absyn_Short_sz: _LL19: _LL1A:
 return Cyc_Absyn_ushort_type;case Cyc_Absyn_Int_sz: _LL1B: _LL1C:
 return Cyc_Absyn_uint_type;case Cyc_Absyn_Long_sz: _LL1D: _LL1E:
 return Cyc_Absyn_ulong_type;case Cyc_Absyn_LongLong_sz: _LL1F: _LL20:
 goto _LL22;default: _LL21: _LL22:
 return Cyc_Absyn_ulonglong_type;}_LL16:;}case Cyc_Absyn_None: _LL5: _LL6:
# 302
 goto _LL8;default: _LL7: _LL8: {
# 304
enum Cyc_Absyn_Size_of _tmp58=sz;switch(_tmp58){case Cyc_Absyn_Char_sz: _LL24: _LL25:
 return Cyc_Absyn_char_type;case Cyc_Absyn_Short_sz: _LL26: _LL27:
 return Cyc_Absyn_nshort_type;case Cyc_Absyn_Int_sz: _LL28: _LL29:
 return Cyc_Absyn_nint_type;case Cyc_Absyn_Long_sz: _LL2A: _LL2B:
 return Cyc_Absyn_nlong_type;case Cyc_Absyn_LongLong_sz: _LL2C: _LL2D:
 goto _LL2F;default: _LL2E: _LL2F:
 return Cyc_Absyn_nlonglong_type;}_LL23:;}}_LL0:;}
# 246
void*Cyc_Absyn_thin_bounds_exp(struct Cyc_Absyn_Exp*e){
# 316
return({void*(*_tmp5A)(void*t)=Cyc_Absyn_thin_bounds_type;void*_tmp5B=({Cyc_Absyn_valueof_type(e);});_tmp5A(_tmp5B);});}
# 246
void*Cyc_Absyn_thin_bounds_int(unsigned i){
# 319
struct Cyc_Absyn_Exp*e=({Cyc_Absyn_uint_exp(i,0U);});
e->topt=Cyc_Absyn_uint_type;
return({Cyc_Absyn_thin_bounds_exp(e);});}
# 246
void*Cyc_Absyn_bounds_one(){
# 324
static void*bone=0;
void*b=bone;
if(b == 0){
b=({Cyc_Absyn_thin_bounds_int(1U);});
bone=b;}
# 326
return b;}
# 336
extern int Wchar_t_unsigned;
extern int Sizeof_wchar_t;
# 339
void*Cyc_Absyn_wchar_type(){
int _tmp5F=Sizeof_wchar_t;switch(_tmp5F){case 1U: _LL1: _LL2:
# 350 "absyn.cyc"
 return Wchar_t_unsigned?Cyc_Absyn_uchar_type: Cyc_Absyn_schar_type;case 2U: _LL3: _LL4:
 return Wchar_t_unsigned?Cyc_Absyn_ushort_type: Cyc_Absyn_sshort_type;default: _LL5: _LL6:
# 354
 return Wchar_t_unsigned?Cyc_Absyn_uint_type: Cyc_Absyn_sint_type;}_LL0:;}static char _tmp61[4U]="exn";
# 339 "absyn.cyc"
static struct _fat_ptr Cyc_Absyn_exn_str={_tmp61,_tmp61,_tmp61 + 4U};
# 360 "absyn.cyc"
static struct _tuple0 Cyc_Absyn_exn_name_v={{.Abs_n={2,0}},& Cyc_Absyn_exn_str};
struct _tuple0*Cyc_Absyn_exn_name=& Cyc_Absyn_exn_name_v;static char _tmp68[15U]="Null_Exception";static char _tmp69[13U]="Array_bounds";static char _tmp6A[16U]="Match_Exception";static char _tmp6B[10U]="Bad_alloc";
# 363
struct Cyc_Absyn_Datatypedecl*Cyc_Absyn_exn_tud(){
static struct _fat_ptr builtin_exns[4U]={{_tmp68,_tmp68,_tmp68 + 15U},{_tmp69,_tmp69,_tmp69 + 13U},{_tmp6A,_tmp6A,_tmp6A + 16U},{_tmp6B,_tmp6B,_tmp6B + 10U}};
# 366
static struct Cyc_Absyn_Datatypedecl*tud_opt=0;
if(tud_opt == 0){
struct Cyc_List_List*tufs=0;
{int i=0;for(0;(unsigned)i < 4U;++ i){
tufs=({struct Cyc_List_List*_tmp65=_cycalloc(sizeof(*_tmp65));({struct Cyc_Absyn_Datatypefield*_tmp6A3=({struct Cyc_Absyn_Datatypefield*_tmp64=_cycalloc(sizeof(*_tmp64));({struct _tuple0*_tmp6A2=({struct _tuple0*_tmp63=_cycalloc(sizeof(*_tmp63));((_tmp63->f1).Abs_n).tag=2U,((_tmp63->f1).Abs_n).val=0,({
struct _fat_ptr*_tmp6A1=({struct _fat_ptr*_tmp62=_cycalloc(sizeof(*_tmp62));*_tmp62=*((struct _fat_ptr*)_check_known_subscript_notnull(builtin_exns,4U,sizeof(struct _fat_ptr),i));_tmp62;});_tmp63->f2=_tmp6A1;});_tmp63;});
# 370
_tmp64->name=_tmp6A2;}),_tmp64->typs=0,_tmp64->loc=0U,_tmp64->sc=Cyc_Absyn_Extern;_tmp64;});_tmp65->hd=_tmp6A3;}),_tmp65->tl=tufs;_tmp65;});}}
# 374
tud_opt=({struct Cyc_Absyn_Datatypedecl*_tmp67=_cycalloc(sizeof(*_tmp67));_tmp67->sc=Cyc_Absyn_Extern,_tmp67->name=Cyc_Absyn_exn_name,_tmp67->tvs=0,({struct Cyc_Core_Opt*_tmp6A4=({struct Cyc_Core_Opt*_tmp66=_cycalloc(sizeof(*_tmp66));_tmp66->v=tufs;_tmp66;});_tmp67->fields=_tmp6A4;}),_tmp67->is_extensible=1;_tmp67;});}
# 367
return(struct Cyc_Absyn_Datatypedecl*)_check_null(tud_opt);}
# 363
void*Cyc_Absyn_exn_type(){
# 380
static void*exn_typ=0;
static void*eopt=0;
if(exn_typ == 0){
eopt=({void*(*_tmp6D)(union Cyc_Absyn_DatatypeInfo di,struct Cyc_List_List*args)=Cyc_Absyn_datatype_type;union Cyc_Absyn_DatatypeInfo _tmp6E=({union Cyc_Absyn_DatatypeInfo _tmp615;(_tmp615.KnownDatatype).tag=2U,({struct Cyc_Absyn_Datatypedecl**_tmp6A6=({struct Cyc_Absyn_Datatypedecl**_tmp6F=_cycalloc(sizeof(*_tmp6F));({struct Cyc_Absyn_Datatypedecl*_tmp6A5=({Cyc_Absyn_exn_tud();});*_tmp6F=_tmp6A5;});_tmp6F;});(_tmp615.KnownDatatype).val=_tmp6A6;});_tmp615;});struct Cyc_List_List*_tmp70=0;_tmp6D(_tmp6E,_tmp70);});
exn_typ=({void*(*_tmp71)(void*t,void*rgn,struct Cyc_Absyn_Tqual,void*zero_term)=Cyc_Absyn_at_type;void*_tmp72=(void*)_check_null(eopt);void*_tmp73=Cyc_Absyn_heap_rgn_type;struct Cyc_Absyn_Tqual _tmp74=({Cyc_Absyn_empty_tqual(0U);});void*_tmp75=Cyc_Absyn_false_type;_tmp71(_tmp72,_tmp73,_tmp74,_tmp75);});}
# 382
return(void*)_check_null(exn_typ);}
# 363
struct _tuple0*Cyc_Absyn_datatype_print_arg_qvar(){
# 390
static struct _tuple0*q=0;
if(q == 0)
q=({struct _tuple0*_tmp79=_cycalloc(sizeof(*_tmp79));({union Cyc_Absyn_Nmspace _tmp6A9=({Cyc_Absyn_Abs_n(0,0);});_tmp79->f1=_tmp6A9;}),({struct _fat_ptr*_tmp6A8=({struct _fat_ptr*_tmp78=_cycalloc(sizeof(*_tmp78));({struct _fat_ptr _tmp6A7=({const char*_tmp77="PrintArg";_tag_fat(_tmp77,sizeof(char),9U);});*_tmp78=_tmp6A7;});_tmp78;});_tmp79->f2=_tmp6A8;});_tmp79;});
# 391
return(struct _tuple0*)_check_null(q);}
# 363
struct _tuple0*Cyc_Absyn_datatype_scanf_arg_qvar(){
# 396
static struct _tuple0*q=0;
if(q == 0)
q=({struct _tuple0*_tmp7D=_cycalloc(sizeof(*_tmp7D));({union Cyc_Absyn_Nmspace _tmp6AC=({Cyc_Absyn_Abs_n(0,0);});_tmp7D->f1=_tmp6AC;}),({struct _fat_ptr*_tmp6AB=({struct _fat_ptr*_tmp7C=_cycalloc(sizeof(*_tmp7C));({struct _fat_ptr _tmp6AA=({const char*_tmp7B="ScanfArg";_tag_fat(_tmp7B,sizeof(char),9U);});*_tmp7C=_tmp6AA;});_tmp7C;});_tmp7D->f2=_tmp6AB;});_tmp7D;});
# 397
return(struct _tuple0*)_check_null(q);}
# 363
struct _tuple0*Cyc_Absyn_uniquergn_qvar(){
# 408
static struct _tuple0*q=0;
if(q == 0)
q=({struct _tuple0*_tmp84=_cycalloc(sizeof(*_tmp84));({union Cyc_Absyn_Nmspace _tmp6B1=({Cyc_Absyn_Abs_n(({struct Cyc_List_List*_tmp81=_cycalloc(sizeof(*_tmp81));({struct _fat_ptr*_tmp6B0=({struct _fat_ptr*_tmp80=_cycalloc(sizeof(*_tmp80));({struct _fat_ptr _tmp6AF=({const char*_tmp7F="Core";_tag_fat(_tmp7F,sizeof(char),5U);});*_tmp80=_tmp6AF;});_tmp80;});_tmp81->hd=_tmp6B0;}),_tmp81->tl=0;_tmp81;}),0);});_tmp84->f1=_tmp6B1;}),({struct _fat_ptr*_tmp6AE=({struct _fat_ptr*_tmp83=_cycalloc(sizeof(*_tmp83));({struct _fat_ptr _tmp6AD=({const char*_tmp82="unique_region";_tag_fat(_tmp82,sizeof(char),14U);});*_tmp83=_tmp6AD;});_tmp83;});_tmp84->f2=_tmp6AE;});_tmp84;});
# 409
return(struct _tuple0*)_check_null(q);}
# 363
struct Cyc_Absyn_Exp*Cyc_Absyn_uniquergn_exp(){
# 414
static struct Cyc_Absyn_Exp*e=0;
if(e == 0){
void*t=({Cyc_Absyn_rgn_handle_type(Cyc_Absyn_unique_rgn_type);});
e=({struct Cyc_Absyn_Exp*_tmp89=_cycalloc(sizeof(*_tmp89));_tmp89->topt=t,_tmp89->loc=0U,_tmp89->annot=(void*)& Cyc_Absyn_EmptyAnnot_val,({
void*_tmp6B6=(void*)({struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*_tmp88=_cycalloc(sizeof(*_tmp88));_tmp88->tag=1U,({void*_tmp6B5=(void*)({struct Cyc_Absyn_Global_b_Absyn_Binding_struct*_tmp87=_cycalloc(sizeof(*_tmp87));_tmp87->tag=1U,({struct Cyc_Absyn_Vardecl*_tmp6B4=({struct Cyc_Absyn_Vardecl*_tmp86=_cycalloc(sizeof(*_tmp86));_tmp86->sc=Cyc_Absyn_Extern,({struct _tuple0*_tmp6B3=({Cyc_Absyn_uniquergn_qvar();});_tmp86->name=_tmp6B3;}),_tmp86->varloc=0U,({
struct Cyc_Absyn_Tqual _tmp6B2=({Cyc_Absyn_empty_tqual(0U);});_tmp86->tq=_tmp6B2;}),_tmp86->type=t,_tmp86->initializer=0,_tmp86->rgn=0,_tmp86->attributes=0,_tmp86->escapes=0;_tmp86;});
# 418
_tmp87->f1=_tmp6B4;});_tmp87;});_tmp88->f1=_tmp6B5;});_tmp88;});_tmp89->r=_tmp6B6;});_tmp89;});}
# 415
return(struct Cyc_Absyn_Exp*)_check_null(e);}
# 363
void*Cyc_Absyn_pointer_type(struct Cyc_Absyn_PtrInfo s){
# 427
return(void*)({struct Cyc_Absyn_PointerType_Absyn_Type_struct*_tmp8B=_cycalloc(sizeof(*_tmp8B));_tmp8B->tag=3U,_tmp8B->f1=s;_tmp8B;});}
# 363
void*Cyc_Absyn_fatptr_type(void*t,void*r,struct Cyc_Absyn_Tqual tq,void*zt){
# 431
return({Cyc_Absyn_pointer_type(({struct Cyc_Absyn_PtrInfo _tmp616;_tmp616.elt_type=t,_tmp616.elt_tq=tq,(_tmp616.ptr_atts).rgn=r,(_tmp616.ptr_atts).nullable=Cyc_Absyn_true_type,(_tmp616.ptr_atts).bounds=Cyc_Absyn_fat_bound_type,(_tmp616.ptr_atts).zero_term=zt,(_tmp616.ptr_atts).ptrloc=0;_tmp616;}));});}
# 363
void*Cyc_Absyn_starb_type(void*t,void*r,struct Cyc_Absyn_Tqual tq,void*b,void*zt){
# 436
return({Cyc_Absyn_pointer_type(({struct Cyc_Absyn_PtrInfo _tmp617;_tmp617.elt_type=t,_tmp617.elt_tq=tq,(_tmp617.ptr_atts).rgn=r,(_tmp617.ptr_atts).nullable=Cyc_Absyn_true_type,(_tmp617.ptr_atts).bounds=b,(_tmp617.ptr_atts).zero_term=zt,(_tmp617.ptr_atts).ptrloc=0;_tmp617;}));});}
# 363
void*Cyc_Absyn_atb_type(void*t,void*r,struct Cyc_Absyn_Tqual tq,void*b,void*zt){
# 441
return({Cyc_Absyn_pointer_type(({struct Cyc_Absyn_PtrInfo _tmp618;_tmp618.elt_type=t,_tmp618.elt_tq=tq,(_tmp618.ptr_atts).rgn=r,(_tmp618.ptr_atts).nullable=Cyc_Absyn_false_type,(_tmp618.ptr_atts).bounds=b,(_tmp618.ptr_atts).zero_term=zt,(_tmp618.ptr_atts).ptrloc=0;_tmp618;}));});}
# 363
void*Cyc_Absyn_star_type(void*t,void*r,struct Cyc_Absyn_Tqual tq,void*zeroterm){
# 446
return({void*(*_tmp90)(void*t,void*r,struct Cyc_Absyn_Tqual tq,void*b,void*zt)=Cyc_Absyn_starb_type;void*_tmp91=t;void*_tmp92=r;struct Cyc_Absyn_Tqual _tmp93=tq;void*_tmp94=({Cyc_Absyn_bounds_one();});void*_tmp95=zeroterm;_tmp90(_tmp91,_tmp92,_tmp93,_tmp94,_tmp95);});}
# 363
void*Cyc_Absyn_cstar_type(void*t,struct Cyc_Absyn_Tqual tq){
# 449
return({Cyc_Absyn_star_type(t,Cyc_Absyn_heap_rgn_type,tq,Cyc_Absyn_false_type);});}
# 363
void*Cyc_Absyn_at_type(void*t,void*r,struct Cyc_Absyn_Tqual tq,void*zeroterm){
# 452
return({void*(*_tmp98)(void*t,void*r,struct Cyc_Absyn_Tqual tq,void*b,void*zt)=Cyc_Absyn_atb_type;void*_tmp99=t;void*_tmp9A=r;struct Cyc_Absyn_Tqual _tmp9B=tq;void*_tmp9C=({Cyc_Absyn_bounds_one();});void*_tmp9D=zeroterm;_tmp98(_tmp99,_tmp9A,_tmp9B,_tmp9C,_tmp9D);});}
# 363
void*Cyc_Absyn_string_type(void*rgn){
# 455
return({void*(*_tmp9F)(void*t,void*r,struct Cyc_Absyn_Tqual tq,void*b,void*zt)=Cyc_Absyn_starb_type;void*_tmpA0=Cyc_Absyn_char_type;void*_tmpA1=rgn;struct Cyc_Absyn_Tqual _tmpA2=({Cyc_Absyn_empty_tqual(0U);});void*_tmpA3=Cyc_Absyn_fat_bound_type;void*_tmpA4=Cyc_Absyn_true_type;_tmp9F(_tmpA0,_tmpA1,_tmpA2,_tmpA3,_tmpA4);});}
# 363
void*Cyc_Absyn_const_string_type(void*rgn){
# 458
return({void*(*_tmpA6)(void*t,void*r,struct Cyc_Absyn_Tqual tq,void*b,void*zt)=Cyc_Absyn_starb_type;void*_tmpA7=Cyc_Absyn_char_type;void*_tmpA8=rgn;struct Cyc_Absyn_Tqual _tmpA9=({Cyc_Absyn_const_tqual(0U);});void*_tmpAA=Cyc_Absyn_fat_bound_type;void*_tmpAB=Cyc_Absyn_true_type;_tmpA6(_tmpA7,_tmpA8,_tmpA9,_tmpAA,_tmpAB);});}
# 363
void*Cyc_Absyn_array_type(void*elt_type,struct Cyc_Absyn_Tqual tq,struct Cyc_Absyn_Exp*num_elts,void*zero_term,unsigned ztloc){
# 463
return(void*)({struct Cyc_Absyn_ArrayType_Absyn_Type_struct*_tmpAD=_cycalloc(sizeof(*_tmpAD));_tmpAD->tag=4U,(_tmpAD->f1).elt_type=elt_type,(_tmpAD->f1).tq=tq,(_tmpAD->f1).num_elts=num_elts,(_tmpAD->f1).zero_term=zero_term,(_tmpAD->f1).zt_loc=ztloc;_tmpAD;});}
# 363
void*Cyc_Absyn_typeof_type(struct Cyc_Absyn_Exp*e){
# 467
return(void*)({struct Cyc_Absyn_TypeofType_Absyn_Type_struct*_tmpAF=_cycalloc(sizeof(*_tmpAF));_tmpAF->tag=11U,_tmpAF->f1=e;_tmpAF;});}
# 363
void*Cyc_Absyn_typedef_type(struct _tuple0*n,struct Cyc_List_List*args,struct Cyc_Absyn_Typedefdecl*d,void*defn){
# 473
return(void*)({struct Cyc_Absyn_TypedefType_Absyn_Type_struct*_tmpB1=_cycalloc(sizeof(*_tmpB1));_tmpB1->tag=8U,_tmpB1->f1=n,_tmpB1->f2=args,_tmpB1->f3=d,_tmpB1->f4=defn;_tmpB1;});}
# 479
static void*Cyc_Absyn_aggregate_type(enum Cyc_Absyn_AggrKind k,struct _fat_ptr*name){
return(void*)({struct Cyc_Absyn_AppType_Absyn_Type_struct*_tmpB5=_cycalloc(sizeof(*_tmpB5));_tmpB5->tag=0U,({void*_tmp6B9=(void*)({struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*_tmpB4=_cycalloc(sizeof(*_tmpB4));_tmpB4->tag=22U,({union Cyc_Absyn_AggrInfo _tmp6B8=({({enum Cyc_Absyn_AggrKind _tmp6B7=k;Cyc_Absyn_UnknownAggr(_tmp6B7,({struct _tuple0*_tmpB3=_cycalloc(sizeof(*_tmpB3));_tmpB3->f1=Cyc_Absyn_rel_ns_null,_tmpB3->f2=name;_tmpB3;}),0);});});_tmpB4->f1=_tmp6B8;});_tmpB4;});_tmpB5->f1=_tmp6B9;}),_tmpB5->f2=0;_tmpB5;});}
# 479
void*Cyc_Absyn_strct(struct _fat_ptr*name){
# 482
return({Cyc_Absyn_aggregate_type(Cyc_Absyn_StructA,name);});}
# 479
void*Cyc_Absyn_union_typ(struct _fat_ptr*name){
# 483
return({Cyc_Absyn_aggregate_type(Cyc_Absyn_UnionA,name);});}
# 479
void*Cyc_Absyn_strctq(struct _tuple0*name){
# 486
return({void*(*_tmpB9)(union Cyc_Absyn_AggrInfo ai,struct Cyc_List_List*args)=Cyc_Absyn_aggr_type;union Cyc_Absyn_AggrInfo _tmpBA=({Cyc_Absyn_UnknownAggr(Cyc_Absyn_StructA,name,0);});struct Cyc_List_List*_tmpBB=0;_tmpB9(_tmpBA,_tmpBB);});}
# 479
void*Cyc_Absyn_unionq_type(struct _tuple0*name){
# 489
return({void*(*_tmpBD)(union Cyc_Absyn_AggrInfo ai,struct Cyc_List_List*args)=Cyc_Absyn_aggr_type;union Cyc_Absyn_AggrInfo _tmpBE=({Cyc_Absyn_UnknownAggr(Cyc_Absyn_UnionA,name,0);});struct Cyc_List_List*_tmpBF=0;_tmpBD(_tmpBE,_tmpBF);});}
# 479
union Cyc_Absyn_Cnst Cyc_Absyn_Char_c(enum Cyc_Absyn_Sign sn,char c){
# 492
return({union Cyc_Absyn_Cnst _tmp619;(_tmp619.Char_c).tag=2U,((_tmp619.Char_c).val).f1=sn,((_tmp619.Char_c).val).f2=c;_tmp619;});}
# 479
union Cyc_Absyn_Cnst Cyc_Absyn_Wchar_c(struct _fat_ptr s){
# 493
return({union Cyc_Absyn_Cnst _tmp61A;(_tmp61A.Wchar_c).tag=3U,(_tmp61A.Wchar_c).val=s;_tmp61A;});}
# 479
union Cyc_Absyn_Cnst Cyc_Absyn_Short_c(enum Cyc_Absyn_Sign sn,short s){
# 494
return({union Cyc_Absyn_Cnst _tmp61B;(_tmp61B.Short_c).tag=4U,((_tmp61B.Short_c).val).f1=sn,((_tmp61B.Short_c).val).f2=s;_tmp61B;});}
# 479
union Cyc_Absyn_Cnst Cyc_Absyn_Int_c(enum Cyc_Absyn_Sign sn,int i){
# 495
return({union Cyc_Absyn_Cnst _tmp61C;(_tmp61C.Int_c).tag=5U,((_tmp61C.Int_c).val).f1=sn,((_tmp61C.Int_c).val).f2=i;_tmp61C;});}
# 479
union Cyc_Absyn_Cnst Cyc_Absyn_LongLong_c(enum Cyc_Absyn_Sign sn,long long l){
# 496
return({union Cyc_Absyn_Cnst _tmp61D;(_tmp61D.LongLong_c).tag=6U,((_tmp61D.LongLong_c).val).f1=sn,((_tmp61D.LongLong_c).val).f2=l;_tmp61D;});}
# 479
union Cyc_Absyn_Cnst Cyc_Absyn_Float_c(struct _fat_ptr s,int i){
# 497
return({union Cyc_Absyn_Cnst _tmp61E;(_tmp61E.Float_c).tag=7U,((_tmp61E.Float_c).val).f1=s,((_tmp61E.Float_c).val).f2=i;_tmp61E;});}
# 479
union Cyc_Absyn_Cnst Cyc_Absyn_String_c(struct _fat_ptr s){
# 498
return({union Cyc_Absyn_Cnst _tmp61F;(_tmp61F.String_c).tag=8U,(_tmp61F.String_c).val=s;_tmp61F;});}
# 479
union Cyc_Absyn_Cnst Cyc_Absyn_Wstring_c(struct _fat_ptr s){
# 499
return({union Cyc_Absyn_Cnst _tmp620;(_tmp620.Wstring_c).tag=9U,(_tmp620.Wstring_c).val=s;_tmp620;});}
# 479
struct Cyc_Absyn_Exp*Cyc_Absyn_new_exp(void*r,unsigned loc){
# 503
return({struct Cyc_Absyn_Exp*_tmpC9=_cycalloc(sizeof(*_tmpC9));_tmpC9->topt=0,_tmpC9->r=r,_tmpC9->loc=loc,_tmpC9->annot=(void*)& Cyc_Absyn_EmptyAnnot_val;_tmpC9;});}
# 479
struct Cyc_Absyn_Exp*Cyc_Absyn_New_exp(struct Cyc_Absyn_Exp*rgn_handle,struct Cyc_Absyn_Exp*e,unsigned loc){
# 506
return({({void*_tmp6BA=(void*)({struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct*_tmpCB=_cycalloc(sizeof(*_tmpCB));_tmpCB->tag=16U,_tmpCB->f1=rgn_handle,_tmpCB->f2=e;_tmpCB;});Cyc_Absyn_new_exp(_tmp6BA,loc);});});}
# 479
struct Cyc_Absyn_Exp*Cyc_Absyn_copy_exp(struct Cyc_Absyn_Exp*e){
# 509
return({struct Cyc_Absyn_Exp*_tmpCD=_cycalloc(sizeof(*_tmpCD));*_tmpCD=*e;_tmpCD;});}
# 479
struct Cyc_Absyn_Exp*Cyc_Absyn_const_exp(union Cyc_Absyn_Cnst c,unsigned loc){
# 512
return({({void*_tmp6BB=(void*)({struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*_tmpCF=_cycalloc(sizeof(*_tmpCF));_tmpCF->tag=0U,_tmpCF->f1=c;_tmpCF;});Cyc_Absyn_new_exp(_tmp6BB,loc);});});}
# 479
struct Cyc_Absyn_Exp*Cyc_Absyn_null_exp(unsigned loc){
# 515
static struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct null_const={0U,{.Null_c={1,0}}};
return({Cyc_Absyn_new_exp((void*)& null_const,loc);});}
# 479
struct Cyc_Absyn_Exp*Cyc_Absyn_int_exp(enum Cyc_Absyn_Sign s,int i,unsigned seg){
# 518
return({struct Cyc_Absyn_Exp*(*_tmpD2)(union Cyc_Absyn_Cnst c,unsigned loc)=Cyc_Absyn_const_exp;union Cyc_Absyn_Cnst _tmpD3=({Cyc_Absyn_Int_c(s,i);});unsigned _tmpD4=seg;_tmpD2(_tmpD3,_tmpD4);});}
# 479
struct Cyc_Absyn_Exp*Cyc_Absyn_signed_int_exp(int i,unsigned loc){
# 520
static struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct szero={0U,{.Int_c={5,{Cyc_Absyn_Signed,0}}}};
static struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct sone={0U,{.Int_c={5,{Cyc_Absyn_Signed,1}}}};
if(i == 0)return({Cyc_Absyn_new_exp((void*)& szero,loc);});else{
if(i == 1)return({Cyc_Absyn_new_exp((void*)& sone,loc);});}
# 522
return({Cyc_Absyn_int_exp(Cyc_Absyn_Signed,i,loc);});}
# 479
struct Cyc_Absyn_Exp*Cyc_Absyn_uint_exp(unsigned i,unsigned loc){
# 527
static struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct uzero={0U,{.Int_c={5,{Cyc_Absyn_Unsigned,0}}}};
static struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct uone={0U,{.Int_c={5,{Cyc_Absyn_Unsigned,1}}}};
if(i == (unsigned)0)return({Cyc_Absyn_new_exp((void*)& uzero,loc);});else{
if(i == (unsigned)1)return({Cyc_Absyn_new_exp((void*)& uone,loc);});else{
return({Cyc_Absyn_int_exp(Cyc_Absyn_Unsigned,(int)i,loc);});}}}
# 479
struct Cyc_Absyn_Exp*Cyc_Absyn_bool_exp(int b,unsigned loc){
# 533
return({Cyc_Absyn_signed_int_exp(b?1: 0,loc);});}
# 479
struct Cyc_Absyn_Exp*Cyc_Absyn_true_exp(unsigned loc){
# 534
return({Cyc_Absyn_bool_exp(1,loc);});}
# 479
struct Cyc_Absyn_Exp*Cyc_Absyn_false_exp(unsigned loc){
# 535
return({Cyc_Absyn_bool_exp(0,loc);});}
# 479
struct Cyc_Absyn_Exp*Cyc_Absyn_char_exp(char c,unsigned loc){
# 536
return({struct Cyc_Absyn_Exp*(*_tmpDB)(union Cyc_Absyn_Cnst c,unsigned loc)=Cyc_Absyn_const_exp;union Cyc_Absyn_Cnst _tmpDC=({Cyc_Absyn_Char_c(Cyc_Absyn_None,c);});unsigned _tmpDD=loc;_tmpDB(_tmpDC,_tmpDD);});}
# 479
struct Cyc_Absyn_Exp*Cyc_Absyn_float_exp(struct _fat_ptr f,int i,unsigned loc){
# 538
return({struct Cyc_Absyn_Exp*(*_tmpDF)(union Cyc_Absyn_Cnst c,unsigned loc)=Cyc_Absyn_const_exp;union Cyc_Absyn_Cnst _tmpE0=({Cyc_Absyn_Float_c(f,i);});unsigned _tmpE1=loc;_tmpDF(_tmpE0,_tmpE1);});}
# 540
static struct Cyc_Absyn_Exp*Cyc_Absyn_str2exp(union Cyc_Absyn_Cnst(*f)(struct _fat_ptr),struct _fat_ptr s,unsigned loc){
return({struct Cyc_Absyn_Exp*(*_tmpE3)(union Cyc_Absyn_Cnst c,unsigned loc)=Cyc_Absyn_const_exp;union Cyc_Absyn_Cnst _tmpE4=({f(s);});unsigned _tmpE5=loc;_tmpE3(_tmpE4,_tmpE5);});}
# 540
struct Cyc_Absyn_Exp*Cyc_Absyn_wchar_exp(struct _fat_ptr s,unsigned loc){
# 543
return({Cyc_Absyn_str2exp(Cyc_Absyn_Wchar_c,s,loc);});}
# 540
struct Cyc_Absyn_Exp*Cyc_Absyn_string_exp(struct _fat_ptr s,unsigned loc){
# 544
return({Cyc_Absyn_str2exp(Cyc_Absyn_String_c,s,loc);});}
# 540
struct Cyc_Absyn_Exp*Cyc_Absyn_wstring_exp(struct _fat_ptr s,unsigned loc){
# 545
return({Cyc_Absyn_str2exp(Cyc_Absyn_Wstring_c,s,loc);});}
# 540
struct Cyc_Absyn_Exp*Cyc_Absyn_var_exp(struct _tuple0*q,unsigned loc){
# 548
return({({void*_tmp6BD=(void*)({struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*_tmpEB=_cycalloc(sizeof(*_tmpEB));_tmpEB->tag=1U,({void*_tmp6BC=(void*)({struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct*_tmpEA=_cycalloc(sizeof(*_tmpEA));_tmpEA->tag=0U,_tmpEA->f1=q;_tmpEA;});_tmpEB->f1=_tmp6BC;});_tmpEB;});Cyc_Absyn_new_exp(_tmp6BD,loc);});});}
# 540
struct Cyc_Absyn_Exp*Cyc_Absyn_varb_exp(void*b,unsigned loc){
# 551
return({({void*_tmp6BE=(void*)({struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*_tmpED=_cycalloc(sizeof(*_tmpED));_tmpED->tag=1U,_tmpED->f1=b;_tmpED;});Cyc_Absyn_new_exp(_tmp6BE,loc);});});}
# 540
struct Cyc_Absyn_Exp*Cyc_Absyn_unknownid_exp(struct _tuple0*q,unsigned loc){
# 555
return({Cyc_Absyn_var_exp(q,loc);});}
# 540
struct Cyc_Absyn_Exp*Cyc_Absyn_pragma_exp(struct _fat_ptr s,unsigned loc){
# 558
return({({void*_tmp6BF=(void*)({struct Cyc_Absyn_Pragma_e_Absyn_Raw_exp_struct*_tmpF0=_cycalloc(sizeof(*_tmpF0));_tmpF0->tag=2U,_tmpF0->f1=s;_tmpF0;});Cyc_Absyn_new_exp(_tmp6BF,loc);});});}
# 540
struct Cyc_Absyn_Exp*Cyc_Absyn_primop_exp(enum Cyc_Absyn_Primop p,struct Cyc_List_List*es,unsigned loc){
# 561
return({({void*_tmp6C0=(void*)({struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*_tmpF2=_cycalloc(sizeof(*_tmpF2));_tmpF2->tag=3U,_tmpF2->f1=p,_tmpF2->f2=es;_tmpF2;});Cyc_Absyn_new_exp(_tmp6C0,loc);});});}
# 540
struct Cyc_Absyn_Exp*Cyc_Absyn_prim1_exp(enum Cyc_Absyn_Primop p,struct Cyc_Absyn_Exp*e,unsigned loc){
# 564
return({({enum Cyc_Absyn_Primop _tmp6C2=p;struct Cyc_List_List*_tmp6C1=({struct Cyc_List_List*_tmpF4=_cycalloc(sizeof(*_tmpF4));_tmpF4->hd=e,_tmpF4->tl=0;_tmpF4;});Cyc_Absyn_primop_exp(_tmp6C2,_tmp6C1,loc);});});}
# 540
struct Cyc_Absyn_Exp*Cyc_Absyn_prim2_exp(enum Cyc_Absyn_Primop p,struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2,unsigned loc){
# 567
return({({enum Cyc_Absyn_Primop _tmp6C5=p;struct Cyc_List_List*_tmp6C4=({struct Cyc_List_List*_tmpF7=_cycalloc(sizeof(*_tmpF7));_tmpF7->hd=e1,({struct Cyc_List_List*_tmp6C3=({struct Cyc_List_List*_tmpF6=_cycalloc(sizeof(*_tmpF6));_tmpF6->hd=e2,_tmpF6->tl=0;_tmpF6;});_tmpF7->tl=_tmp6C3;});_tmpF7;});Cyc_Absyn_primop_exp(_tmp6C5,_tmp6C4,loc);});});}
# 540
struct Cyc_Absyn_Exp*Cyc_Absyn_swap_exp(struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2,unsigned loc){
# 570
return({({void*_tmp6C6=(void*)({struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct*_tmpF9=_cycalloc(sizeof(*_tmpF9));_tmpF9->tag=36U,_tmpF9->f1=e1,_tmpF9->f2=e2;_tmpF9;});Cyc_Absyn_new_exp(_tmp6C6,loc);});});}
# 540
struct Cyc_Absyn_Exp*Cyc_Absyn_add_exp(struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2,unsigned loc){
# 572
return({Cyc_Absyn_prim2_exp(Cyc_Absyn_Plus,e1,e2,loc);});}
# 540
struct Cyc_Absyn_Exp*Cyc_Absyn_times_exp(struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2,unsigned loc){
# 573
return({Cyc_Absyn_prim2_exp(Cyc_Absyn_Times,e1,e2,loc);});}
# 540
struct Cyc_Absyn_Exp*Cyc_Absyn_divide_exp(struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2,unsigned loc){
# 574
return({Cyc_Absyn_prim2_exp(Cyc_Absyn_Div,e1,e2,loc);});}
# 540
struct Cyc_Absyn_Exp*Cyc_Absyn_eq_exp(struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2,unsigned loc){
# 575
return({Cyc_Absyn_prim2_exp(Cyc_Absyn_Eq,e1,e2,loc);});}
# 540
struct Cyc_Absyn_Exp*Cyc_Absyn_neq_exp(struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2,unsigned loc){
# 576
return({Cyc_Absyn_prim2_exp(Cyc_Absyn_Neq,e1,e2,loc);});}
# 540
struct Cyc_Absyn_Exp*Cyc_Absyn_gt_exp(struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2,unsigned loc){
# 577
return({Cyc_Absyn_prim2_exp(Cyc_Absyn_Gt,e1,e2,loc);});}
# 540
struct Cyc_Absyn_Exp*Cyc_Absyn_lt_exp(struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2,unsigned loc){
# 578
return({Cyc_Absyn_prim2_exp(Cyc_Absyn_Lt,e1,e2,loc);});}
# 540
struct Cyc_Absyn_Exp*Cyc_Absyn_gte_exp(struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2,unsigned loc){
# 579
return({Cyc_Absyn_prim2_exp(Cyc_Absyn_Gte,e1,e2,loc);});}
# 540
struct Cyc_Absyn_Exp*Cyc_Absyn_lte_exp(struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2,unsigned loc){
# 580
return({Cyc_Absyn_prim2_exp(Cyc_Absyn_Lte,e1,e2,loc);});}
# 540
struct Cyc_Absyn_Exp*Cyc_Absyn_assignop_exp(struct Cyc_Absyn_Exp*e1,struct Cyc_Core_Opt*popt,struct Cyc_Absyn_Exp*e2,unsigned loc){
# 583
return({({void*_tmp6C7=(void*)({struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*_tmp104=_cycalloc(sizeof(*_tmp104));_tmp104->tag=4U,_tmp104->f1=e1,_tmp104->f2=popt,_tmp104->f3=e2;_tmp104;});Cyc_Absyn_new_exp(_tmp6C7,loc);});});}
# 540
struct Cyc_Absyn_Exp*Cyc_Absyn_assign_exp(struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2,unsigned loc){
# 586
return({Cyc_Absyn_assignop_exp(e1,0,e2,loc);});}
# 540
struct Cyc_Absyn_Exp*Cyc_Absyn_increment_exp(struct Cyc_Absyn_Exp*e,enum Cyc_Absyn_Incrementor i,unsigned loc){
# 589
return({({void*_tmp6C8=(void*)({struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*_tmp107=_cycalloc(sizeof(*_tmp107));_tmp107->tag=5U,_tmp107->f1=e,_tmp107->f2=i;_tmp107;});Cyc_Absyn_new_exp(_tmp6C8,loc);});});}
# 540
struct Cyc_Absyn_Exp*Cyc_Absyn_conditional_exp(struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2,struct Cyc_Absyn_Exp*e3,unsigned loc){
# 592
return({({void*_tmp6C9=(void*)({struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*_tmp109=_cycalloc(sizeof(*_tmp109));_tmp109->tag=6U,_tmp109->f1=e1,_tmp109->f2=e2,_tmp109->f3=e3;_tmp109;});Cyc_Absyn_new_exp(_tmp6C9,loc);});});}
# 540
struct Cyc_Absyn_Exp*Cyc_Absyn_and_exp(struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2,unsigned loc){
# 595
return({({void*_tmp6CA=(void*)({struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*_tmp10B=_cycalloc(sizeof(*_tmp10B));_tmp10B->tag=7U,_tmp10B->f1=e1,_tmp10B->f2=e2;_tmp10B;});Cyc_Absyn_new_exp(_tmp6CA,loc);});});}
# 540
struct Cyc_Absyn_Exp*Cyc_Absyn_or_exp(struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2,unsigned loc){
# 598
return({({void*_tmp6CB=(void*)({struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*_tmp10D=_cycalloc(sizeof(*_tmp10D));_tmp10D->tag=8U,_tmp10D->f1=e1,_tmp10D->f2=e2;_tmp10D;});Cyc_Absyn_new_exp(_tmp6CB,loc);});});}
# 540
struct Cyc_Absyn_Exp*Cyc_Absyn_spawn_exp(struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2,struct Cyc_List_List*f1,struct Cyc_List_List*f2,unsigned loc){
# 602
return({({void*_tmp6CD=(void*)({struct Cyc_Absyn_Spawn_e_Absyn_Raw_exp_struct*_tmp110=_cycalloc(sizeof(*_tmp110));_tmp110->tag=34U,_tmp110->f1=e1,_tmp110->f2=e2,({struct _tuple8*_tmp6CC=({struct _tuple8*_tmp10F=_cycalloc(sizeof(*_tmp10F));_tmp10F->f1=f1,_tmp10F->f2=f2;_tmp10F;});_tmp110->f3=_tmp6CC;});_tmp110;});Cyc_Absyn_new_exp(_tmp6CD,loc);});});}
# 540
struct Cyc_Absyn_Exp*Cyc_Absyn_seq_exp(struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2,unsigned loc){
# 606
return({({void*_tmp6CE=(void*)({struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*_tmp112=_cycalloc(sizeof(*_tmp112));_tmp112->tag=9U,_tmp112->f1=e1,_tmp112->f2=e2;_tmp112;});Cyc_Absyn_new_exp(_tmp6CE,loc);});});}
# 540
struct Cyc_Absyn_Exp*Cyc_Absyn_unknowncall_exp(struct Cyc_Absyn_Exp*e,struct Cyc_List_List*es,unsigned loc){
# 610
return({({void*_tmp6D0=(void*)({struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*_tmp115=_cycalloc(sizeof(*_tmp115));_tmp115->tag=10U,_tmp115->f1=e,_tmp115->f2=es,_tmp115->f3=0,_tmp115->f4=0,({struct _tuple8*_tmp6CF=({struct _tuple8*_tmp114=_cycalloc(sizeof(*_tmp114));_tmp114->f1=0,_tmp114->f2=0;_tmp114;});_tmp115->f5=_tmp6CF;});_tmp115;});Cyc_Absyn_new_exp(_tmp6D0,loc);});});}
# 540
struct Cyc_Absyn_Exp*Cyc_Absyn_fncall_exp(struct Cyc_Absyn_Exp*e,struct Cyc_List_List*es,unsigned loc){
# 613
return({({void*_tmp6D2=(void*)({struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*_tmp118=_cycalloc(sizeof(*_tmp118));_tmp118->tag=10U,_tmp118->f1=e,_tmp118->f2=es,_tmp118->f3=0,_tmp118->f4=1,({struct _tuple8*_tmp6D1=({struct _tuple8*_tmp117=_cycalloc(sizeof(*_tmp117));_tmp117->f1=0,_tmp117->f2=0;_tmp117;});_tmp118->f5=_tmp6D1;});_tmp118;});Cyc_Absyn_new_exp(_tmp6D2,loc);});});}
# 540
struct Cyc_Absyn_Exp*Cyc_Absyn_noinstantiate_exp(struct Cyc_Absyn_Exp*e,unsigned loc){
# 616
return({({void*_tmp6D3=(void*)({struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*_tmp11A=_cycalloc(sizeof(*_tmp11A));_tmp11A->tag=12U,_tmp11A->f1=e;_tmp11A;});Cyc_Absyn_new_exp(_tmp6D3,loc);});});}
# 540
struct Cyc_Absyn_Exp*Cyc_Absyn_instantiate_exp(struct Cyc_Absyn_Exp*e,struct Cyc_List_List*ts,unsigned loc){
# 619
return({({void*_tmp6D4=(void*)({struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*_tmp11C=_cycalloc(sizeof(*_tmp11C));_tmp11C->tag=13U,_tmp11C->f1=e,_tmp11C->f2=ts;_tmp11C;});Cyc_Absyn_new_exp(_tmp6D4,loc);});});}
# 540
struct Cyc_Absyn_Exp*Cyc_Absyn_cast_exp(void*t,struct Cyc_Absyn_Exp*e,int user_cast,enum Cyc_Absyn_Coercion c,unsigned loc){
# 622
return({({void*_tmp6D5=(void*)({struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*_tmp11E=_cycalloc(sizeof(*_tmp11E));_tmp11E->tag=14U,_tmp11E->f1=t,_tmp11E->f2=e,_tmp11E->f3=user_cast,_tmp11E->f4=c;_tmp11E;});Cyc_Absyn_new_exp(_tmp6D5,loc);});});}
# 540
struct Cyc_Absyn_Exp*Cyc_Absyn_throw_exp(struct Cyc_Absyn_Exp*e,unsigned loc){
# 625
return({({void*_tmp6D6=(void*)({struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct*_tmp120=_cycalloc(sizeof(*_tmp120));_tmp120->tag=11U,_tmp120->f1=e,_tmp120->f2=0;_tmp120;});Cyc_Absyn_new_exp(_tmp6D6,loc);});});}
# 540
struct Cyc_Absyn_Exp*Cyc_Absyn_rethrow_exp(struct Cyc_Absyn_Exp*e,unsigned loc){
# 628
return({({void*_tmp6D7=(void*)({struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct*_tmp122=_cycalloc(sizeof(*_tmp122));_tmp122->tag=11U,_tmp122->f1=e,_tmp122->f2=1;_tmp122;});Cyc_Absyn_new_exp(_tmp6D7,loc);});});}
# 540
struct Cyc_Absyn_Exp*Cyc_Absyn_address_exp(struct Cyc_Absyn_Exp*e,unsigned loc){
# 630
return({({void*_tmp6D8=(void*)({struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct*_tmp124=_cycalloc(sizeof(*_tmp124));_tmp124->tag=15U,_tmp124->f1=e;_tmp124;});Cyc_Absyn_new_exp(_tmp6D8,loc);});});}
# 540
struct Cyc_Absyn_Exp*Cyc_Absyn_sizeoftype_exp(void*t,unsigned loc){
# 632
return({({void*_tmp6D9=(void*)({struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct*_tmp126=_cycalloc(sizeof(*_tmp126));_tmp126->tag=17U,_tmp126->f1=t;_tmp126;});Cyc_Absyn_new_exp(_tmp6D9,loc);});});}
# 540
struct Cyc_Absyn_Exp*Cyc_Absyn_sizeofexp_exp(struct Cyc_Absyn_Exp*e,unsigned loc){
# 635
return({({void*_tmp6DA=(void*)({struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct*_tmp128=_cycalloc(sizeof(*_tmp128));_tmp128->tag=18U,_tmp128->f1=e;_tmp128;});Cyc_Absyn_new_exp(_tmp6DA,loc);});});}
# 540
struct Cyc_Absyn_Exp*Cyc_Absyn_offsetof_exp(void*t,struct Cyc_List_List*ofs,unsigned loc){
# 638
return({({void*_tmp6DB=(void*)({struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct*_tmp12A=_cycalloc(sizeof(*_tmp12A));_tmp12A->tag=19U,_tmp12A->f1=t,_tmp12A->f2=ofs;_tmp12A;});Cyc_Absyn_new_exp(_tmp6DB,loc);});});}
# 540
struct Cyc_Absyn_Exp*Cyc_Absyn_deref_exp(struct Cyc_Absyn_Exp*e,unsigned loc){
# 640
return({({void*_tmp6DC=(void*)({struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*_tmp12C=_cycalloc(sizeof(*_tmp12C));_tmp12C->tag=20U,_tmp12C->f1=e;_tmp12C;});Cyc_Absyn_new_exp(_tmp6DC,loc);});});}
# 540
struct Cyc_Absyn_Exp*Cyc_Absyn_aggrmember_exp(struct Cyc_Absyn_Exp*e,struct _fat_ptr*n,unsigned loc){
# 642
return({({void*_tmp6DD=(void*)({struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*_tmp12E=_cycalloc(sizeof(*_tmp12E));_tmp12E->tag=21U,_tmp12E->f1=e,_tmp12E->f2=n,_tmp12E->f3=0,_tmp12E->f4=0;_tmp12E;});Cyc_Absyn_new_exp(_tmp6DD,loc);});});}
# 540
struct Cyc_Absyn_Exp*Cyc_Absyn_aggrarrow_exp(struct Cyc_Absyn_Exp*e,struct _fat_ptr*n,unsigned loc){
# 645
return({({void*_tmp6DE=(void*)({struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*_tmp130=_cycalloc(sizeof(*_tmp130));_tmp130->tag=22U,_tmp130->f1=e,_tmp130->f2=n,_tmp130->f3=0,_tmp130->f4=0;_tmp130;});Cyc_Absyn_new_exp(_tmp6DE,loc);});});}
# 540
struct Cyc_Absyn_Exp*Cyc_Absyn_subscript_exp(struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2,unsigned loc){
# 648
return({({void*_tmp6DF=(void*)({struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*_tmp132=_cycalloc(sizeof(*_tmp132));_tmp132->tag=23U,_tmp132->f1=e1,_tmp132->f2=e2;_tmp132;});Cyc_Absyn_new_exp(_tmp6DF,loc);});});}
# 540
struct Cyc_Absyn_Exp*Cyc_Absyn_tuple_exp(struct Cyc_List_List*es,unsigned loc){
# 651
return({({void*_tmp6E0=(void*)({struct Cyc_Absyn_Tuple_e_Absyn_Raw_exp_struct*_tmp134=_cycalloc(sizeof(*_tmp134));_tmp134->tag=24U,_tmp134->f1=es;_tmp134;});Cyc_Absyn_new_exp(_tmp6E0,loc);});});}
# 540
struct Cyc_Absyn_Exp*Cyc_Absyn_stmt_exp(struct Cyc_Absyn_Stmt*s,unsigned loc){
# 654
return({({void*_tmp6E1=(void*)({struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct*_tmp136=_cycalloc(sizeof(*_tmp136));_tmp136->tag=38U,_tmp136->f1=s;_tmp136;});Cyc_Absyn_new_exp(_tmp6E1,loc);});});}
# 540
struct Cyc_Absyn_Exp*Cyc_Absyn_valueof_exp(void*t,unsigned loc){
# 657
return({({void*_tmp6E2=(void*)({struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct*_tmp138=_cycalloc(sizeof(*_tmp138));_tmp138->tag=40U,_tmp138->f1=t;_tmp138;});Cyc_Absyn_new_exp(_tmp6E2,loc);});});}
# 540
struct Cyc_Absyn_Exp*Cyc_Absyn_asm_exp(int volatile_kw,struct _fat_ptr tmpl,struct Cyc_List_List*outs,struct Cyc_List_List*ins,struct Cyc_List_List*clobs,unsigned loc){
# 663
return({({void*_tmp6E3=(void*)({struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct*_tmp13A=_cycalloc(sizeof(*_tmp13A));_tmp13A->tag=41U,_tmp13A->f1=volatile_kw,_tmp13A->f2=tmpl,_tmp13A->f3=outs,_tmp13A->f4=ins,_tmp13A->f5=clobs;_tmp13A;});Cyc_Absyn_new_exp(_tmp6E3,loc);});});}
# 540
struct Cyc_Absyn_Exp*Cyc_Absyn_extension_exp(struct Cyc_Absyn_Exp*e,unsigned loc){
# 666
return({({void*_tmp6E4=(void*)({struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct*_tmp13C=_cycalloc(sizeof(*_tmp13C));_tmp13C->tag=42U,_tmp13C->f1=e;_tmp13C;});Cyc_Absyn_new_exp(_tmp6E4,loc);});});}struct _tuple14{struct Cyc_List_List*f1;struct Cyc_Absyn_Exp*f2;};
# 540
struct Cyc_Absyn_Exp*Cyc_Absyn_array_exp(struct Cyc_List_List*es,unsigned loc){
# 670
struct Cyc_List_List*dles=0;
for(0;es != 0;es=es->tl){
dles=({struct Cyc_List_List*_tmp13F=_cycalloc(sizeof(*_tmp13F));({struct _tuple14*_tmp6E5=({struct _tuple14*_tmp13E=_cycalloc(sizeof(*_tmp13E));_tmp13E->f1=0,_tmp13E->f2=(struct Cyc_Absyn_Exp*)es->hd;_tmp13E;});_tmp13F->hd=_tmp6E5;}),_tmp13F->tl=dles;_tmp13F;});}
dles=({Cyc_List_imp_rev(dles);});
return({({void*_tmp6E6=(void*)({struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*_tmp140=_cycalloc(sizeof(*_tmp140));_tmp140->tag=26U,_tmp140->f1=dles;_tmp140;});Cyc_Absyn_new_exp(_tmp6E6,loc);});});}
# 540
struct Cyc_Absyn_Exp*Cyc_Absyn_unresolvedmem_exp(struct Cyc_Core_Opt*n,struct Cyc_List_List*dles,unsigned loc){
# 679
return({({void*_tmp6E7=(void*)({struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*_tmp142=_cycalloc(sizeof(*_tmp142));_tmp142->tag=37U,_tmp142->f1=n,_tmp142->f2=dles;_tmp142;});Cyc_Absyn_new_exp(_tmp6E7,loc);});});}
# 540
struct Cyc_Absyn_Stmt*Cyc_Absyn_new_stmt(void*s,unsigned loc){
# 683
return({struct Cyc_Absyn_Stmt*_tmp144=_cycalloc(sizeof(*_tmp144));_tmp144->r=s,_tmp144->loc=loc,_tmp144->annot=(void*)& Cyc_Absyn_EmptyAnnot_val;_tmp144;});}
# 540
struct Cyc_Absyn_Skip_s_Absyn_Raw_stmt_struct Cyc_Absyn_Skip_s_val={0U};
# 686
struct Cyc_Absyn_Break_s_Absyn_Raw_stmt_struct Cyc_Absyn_Break_s_val={6U};
struct Cyc_Absyn_Continue_s_Absyn_Raw_stmt_struct Cyc_Absyn_Continue_s_val={7U};
struct Cyc_Absyn_Stmt*Cyc_Absyn_skip_stmt(unsigned loc){return({Cyc_Absyn_new_stmt((void*)& Cyc_Absyn_Skip_s_val,loc);});}struct Cyc_Absyn_Stmt*Cyc_Absyn_break_stmt(unsigned loc){
return({Cyc_Absyn_new_stmt((void*)& Cyc_Absyn_Break_s_val,loc);});}
# 688
struct Cyc_Absyn_Stmt*Cyc_Absyn_continue_stmt(unsigned loc){
# 690
return({Cyc_Absyn_new_stmt((void*)& Cyc_Absyn_Continue_s_val,loc);});}
# 688
struct Cyc_Absyn_Stmt*Cyc_Absyn_exp_stmt(struct Cyc_Absyn_Exp*e,unsigned loc){
# 691
return({({void*_tmp6E8=(void*)({struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct*_tmp149=_cycalloc(sizeof(*_tmp149));_tmp149->tag=1U,_tmp149->f1=e;_tmp149;});Cyc_Absyn_new_stmt(_tmp6E8,loc);});});}
# 688
struct Cyc_Absyn_Stmt*Cyc_Absyn_return_stmt(struct Cyc_Absyn_Exp*e,unsigned loc){
# 692
return({({void*_tmp6E9=(void*)({struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct*_tmp14B=_cycalloc(sizeof(*_tmp14B));_tmp14B->tag=3U,_tmp14B->f1=e;_tmp14B;});Cyc_Absyn_new_stmt(_tmp6E9,loc);});});}
# 688
struct Cyc_Absyn_Stmt*Cyc_Absyn_seq_stmts(struct Cyc_List_List*ss,unsigned loc){
# 694
if(ss == 0)return({Cyc_Absyn_skip_stmt(loc);});else{
if(ss->tl == 0)return(struct Cyc_Absyn_Stmt*)ss->hd;else{
return({struct Cyc_Absyn_Stmt*(*_tmp14D)(struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_seq_stmt;struct Cyc_Absyn_Stmt*_tmp14E=(struct Cyc_Absyn_Stmt*)ss->hd;struct Cyc_Absyn_Stmt*_tmp14F=({Cyc_Absyn_seq_stmts(ss->tl,loc);});unsigned _tmp150=loc;_tmp14D(_tmp14E,_tmp14F,_tmp150);});}}}struct _tuple15{void*f1;void*f2;};
# 688
struct Cyc_Absyn_Stmt*Cyc_Absyn_seq_stmt(struct Cyc_Absyn_Stmt*s1,struct Cyc_Absyn_Stmt*s2,unsigned loc){
# 699
struct _tuple15 _stmttmp3=({struct _tuple15 _tmp621;_tmp621.f1=s1->r,_tmp621.f2=s2->r;_tmp621;});struct _tuple15 _tmp152=_stmttmp3;if(((struct Cyc_Absyn_Skip_s_Absyn_Raw_stmt_struct*)_tmp152.f1)->tag == 0U){_LL1: _LL2:
 return s2;}else{if(((struct Cyc_Absyn_Skip_s_Absyn_Raw_stmt_struct*)_tmp152.f2)->tag == 0U){_LL3: _LL4:
 return s1;}else{_LL5: _LL6:
 return({({void*_tmp6EA=(void*)({struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*_tmp153=_cycalloc(sizeof(*_tmp153));_tmp153->tag=2U,_tmp153->f1=s1,_tmp153->f2=s2;_tmp153;});Cyc_Absyn_new_stmt(_tmp6EA,loc);});});}}_LL0:;}
# 688
struct Cyc_Absyn_Stmt*Cyc_Absyn_ifthenelse_stmt(struct Cyc_Absyn_Exp*e,struct Cyc_Absyn_Stmt*s1,struct Cyc_Absyn_Stmt*s2,unsigned loc){
# 706
return({({void*_tmp6EB=(void*)({struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*_tmp155=_cycalloc(sizeof(*_tmp155));_tmp155->tag=4U,_tmp155->f1=e,_tmp155->f2=s1,_tmp155->f3=s2;_tmp155;});Cyc_Absyn_new_stmt(_tmp6EB,loc);});});}
# 688
struct Cyc_Absyn_Stmt*Cyc_Absyn_while_stmt(struct Cyc_Absyn_Exp*e,struct Cyc_Absyn_Stmt*s,unsigned loc){
# 709
return({struct Cyc_Absyn_Stmt*(*_tmp157)(void*s,unsigned loc)=Cyc_Absyn_new_stmt;void*_tmp158=(void*)({struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct*_tmp159=_cycalloc(sizeof(*_tmp159));_tmp159->tag=5U,(_tmp159->f1).f1=e,({struct Cyc_Absyn_Stmt*_tmp6EC=({Cyc_Absyn_skip_stmt(e->loc);});(_tmp159->f1).f2=_tmp6EC;}),_tmp159->f2=s;_tmp159;});unsigned _tmp15A=loc;_tmp157(_tmp158,_tmp15A);});}
# 688
struct Cyc_Absyn_Stmt*Cyc_Absyn_for_stmt(struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2,struct Cyc_Absyn_Exp*e3,struct Cyc_Absyn_Stmt*s,unsigned loc){
# 712
return({struct Cyc_Absyn_Stmt*(*_tmp15C)(void*s,unsigned loc)=Cyc_Absyn_new_stmt;void*_tmp15D=(void*)({struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*_tmp15E=_cycalloc(sizeof(*_tmp15E));_tmp15E->tag=9U,_tmp15E->f1=e1,(_tmp15E->f2).f1=e2,({struct Cyc_Absyn_Stmt*_tmp6EE=({Cyc_Absyn_skip_stmt(e3->loc);});(_tmp15E->f2).f2=_tmp6EE;}),
(_tmp15E->f3).f1=e3,({struct Cyc_Absyn_Stmt*_tmp6ED=({Cyc_Absyn_skip_stmt(e3->loc);});(_tmp15E->f3).f2=_tmp6ED;}),_tmp15E->f4=s;_tmp15E;});unsigned _tmp15F=loc;_tmp15C(_tmp15D,_tmp15F);});}
# 688
struct Cyc_Absyn_Stmt*Cyc_Absyn_do_stmt(struct Cyc_Absyn_Stmt*s,struct Cyc_Absyn_Exp*e,unsigned loc){
# 717
return({struct Cyc_Absyn_Stmt*(*_tmp161)(void*s,unsigned loc)=Cyc_Absyn_new_stmt;void*_tmp162=(void*)({struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct*_tmp163=_cycalloc(sizeof(*_tmp163));_tmp163->tag=14U,_tmp163->f1=s,(_tmp163->f2).f1=e,({struct Cyc_Absyn_Stmt*_tmp6EF=({Cyc_Absyn_skip_stmt(e->loc);});(_tmp163->f2).f2=_tmp6EF;});_tmp163;});unsigned _tmp164=loc;_tmp161(_tmp162,_tmp164);});}
# 688
struct Cyc_Absyn_Stmt*Cyc_Absyn_switch_stmt(struct Cyc_Absyn_Exp*e,struct Cyc_List_List*scs,unsigned loc){
# 720
return({({void*_tmp6F0=(void*)({struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct*_tmp166=_cycalloc(sizeof(*_tmp166));_tmp166->tag=10U,_tmp166->f1=e,_tmp166->f2=scs,_tmp166->f3=0;_tmp166;});Cyc_Absyn_new_stmt(_tmp6F0,loc);});});}
# 688
struct Cyc_Absyn_Stmt*Cyc_Absyn_trycatch_stmt(struct Cyc_Absyn_Stmt*s,struct Cyc_List_List*scs,unsigned loc){
# 723
return({({void*_tmp6F1=(void*)({struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct*_tmp168=_cycalloc(sizeof(*_tmp168));_tmp168->tag=15U,_tmp168->f1=s,_tmp168->f2=scs,_tmp168->f3=0;_tmp168;});Cyc_Absyn_new_stmt(_tmp6F1,loc);});});}
# 688
struct Cyc_Absyn_Stmt*Cyc_Absyn_fallthru_stmt(struct Cyc_List_List*el,unsigned loc){
# 726
return({({void*_tmp6F2=(void*)({struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct*_tmp16A=_cycalloc(sizeof(*_tmp16A));_tmp16A->tag=11U,_tmp16A->f1=el,_tmp16A->f2=0;_tmp16A;});Cyc_Absyn_new_stmt(_tmp6F2,loc);});});}
# 688
struct Cyc_Absyn_Stmt*Cyc_Absyn_goto_stmt(struct _fat_ptr*lab,unsigned loc){
# 729
return({({void*_tmp6F3=(void*)({struct Cyc_Absyn_Goto_s_Absyn_Raw_stmt_struct*_tmp16C=_cycalloc(sizeof(*_tmp16C));_tmp16C->tag=8U,_tmp16C->f1=lab;_tmp16C;});Cyc_Absyn_new_stmt(_tmp6F3,loc);});});}
# 688
struct Cyc_Absyn_Stmt*Cyc_Absyn_label_stmt(struct _fat_ptr*v,struct Cyc_Absyn_Stmt*s,unsigned loc){
# 732
return({({void*_tmp6F4=(void*)({struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct*_tmp16E=_cycalloc(sizeof(*_tmp16E));_tmp16E->tag=13U,_tmp16E->f1=v,_tmp16E->f2=s;_tmp16E;});Cyc_Absyn_new_stmt(_tmp6F4,loc);});});}
# 688
struct Cyc_Absyn_Stmt*Cyc_Absyn_decl_stmt(struct Cyc_Absyn_Decl*d,struct Cyc_Absyn_Stmt*s,unsigned loc){
# 735
return({({void*_tmp6F5=(void*)({struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*_tmp170=_cycalloc(sizeof(*_tmp170));_tmp170->tag=12U,_tmp170->f1=d,_tmp170->f2=s;_tmp170;});Cyc_Absyn_new_stmt(_tmp6F5,loc);});});}
# 688
struct Cyc_Absyn_Stmt*Cyc_Absyn_declare_stmt(struct _tuple0*x,void*t,struct Cyc_Absyn_Exp*init,struct Cyc_Absyn_Stmt*s,unsigned loc){
# 738
struct Cyc_Absyn_Decl*d=({struct Cyc_Absyn_Decl*(*_tmp173)(void*,unsigned)=Cyc_Absyn_new_decl;void*_tmp174=(void*)({struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*_tmp175=_cycalloc(sizeof(*_tmp175));_tmp175->tag=0U,({struct Cyc_Absyn_Vardecl*_tmp6F6=({Cyc_Absyn_new_vardecl(0U,x,t,init);});_tmp175->f1=_tmp6F6;});_tmp175;});unsigned _tmp176=loc;_tmp173(_tmp174,_tmp176);});
return({({void*_tmp6F7=(void*)({struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*_tmp172=_cycalloc(sizeof(*_tmp172));_tmp172->tag=12U,_tmp172->f1=d,_tmp172->f2=s;_tmp172;});Cyc_Absyn_new_stmt(_tmp6F7,loc);});});}
# 688
struct Cyc_Absyn_Stmt*Cyc_Absyn_assign_stmt(struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2,unsigned loc){
# 742
return({struct Cyc_Absyn_Stmt*(*_tmp178)(struct Cyc_Absyn_Exp*e,unsigned loc)=Cyc_Absyn_exp_stmt;struct Cyc_Absyn_Exp*_tmp179=({Cyc_Absyn_assign_exp(e1,e2,loc);});unsigned _tmp17A=loc;_tmp178(_tmp179,_tmp17A);});}
# 688
struct Cyc_Absyn_Pat*Cyc_Absyn_new_pat(void*p,unsigned s){
# 745
return({struct Cyc_Absyn_Pat*_tmp17C=_cycalloc(sizeof(*_tmp17C));_tmp17C->r=p,_tmp17C->topt=0,_tmp17C->loc=s;_tmp17C;});}
# 688
struct Cyc_Absyn_Pat*Cyc_Absyn_exp_pat(struct Cyc_Absyn_Exp*e){
# 746
return({({void*_tmp6F8=(void*)({struct Cyc_Absyn_Exp_p_Absyn_Raw_pat_struct*_tmp17E=_cycalloc(sizeof(*_tmp17E));_tmp17E->tag=17U,_tmp17E->f1=e;_tmp17E;});Cyc_Absyn_new_pat(_tmp6F8,e->loc);});});}
# 688
struct Cyc_Absyn_Wild_p_Absyn_Raw_pat_struct Cyc_Absyn_Wild_p_val={0U};
# 748
struct Cyc_Absyn_Null_p_Absyn_Raw_pat_struct Cyc_Absyn_Null_p_val={9U};
# 751
struct Cyc_Absyn_Decl*Cyc_Absyn_new_decl(void*r,unsigned loc){return({struct Cyc_Absyn_Decl*_tmp180=_cycalloc(sizeof(*_tmp180));_tmp180->r=r,_tmp180->loc=loc;_tmp180;});}struct Cyc_Absyn_Decl*Cyc_Absyn_let_decl(struct Cyc_Absyn_Pat*p,struct Cyc_Absyn_Exp*e,unsigned loc){
# 753
return({({void*_tmp6F9=(void*)({struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct*_tmp182=_cycalloc(sizeof(*_tmp182));_tmp182->tag=2U,_tmp182->f1=p,_tmp182->f2=0,_tmp182->f3=e,_tmp182->f4=0;_tmp182;});Cyc_Absyn_new_decl(_tmp6F9,loc);});});}
# 751
struct Cyc_Absyn_Decl*Cyc_Absyn_letv_decl(struct Cyc_List_List*vds,unsigned loc){
# 756
return({({void*_tmp6FA=(void*)({struct Cyc_Absyn_Letv_d_Absyn_Raw_decl_struct*_tmp184=_cycalloc(sizeof(*_tmp184));_tmp184->tag=3U,_tmp184->f1=vds;_tmp184;});Cyc_Absyn_new_decl(_tmp6FA,loc);});});}
# 751
struct Cyc_Absyn_Decl*Cyc_Absyn_region_decl(struct Cyc_Absyn_Tvar*tv,struct Cyc_Absyn_Vardecl*vd,struct Cyc_Absyn_Exp*open_exp,unsigned loc){
# 759
return({({void*_tmp6FB=(void*)({struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*_tmp186=_cycalloc(sizeof(*_tmp186));_tmp186->tag=4U,_tmp186->f1=tv,_tmp186->f2=vd,_tmp186->f3=open_exp;_tmp186;});Cyc_Absyn_new_decl(_tmp6FB,loc);});});}
# 751
struct Cyc_Absyn_Decl*Cyc_Absyn_alias_decl(struct Cyc_Absyn_Tvar*tv,struct Cyc_Absyn_Vardecl*vd,struct Cyc_Absyn_Exp*e,unsigned loc){
# 763
return({struct Cyc_Absyn_Decl*(*_tmp188)(void*r,unsigned loc)=Cyc_Absyn_new_decl;void*_tmp189=(void*)({struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct*_tmp18B=_cycalloc(sizeof(*_tmp18B));_tmp18B->tag=2U,({struct Cyc_Absyn_Pat*_tmp6FD=({({void*_tmp6FC=(void*)({struct Cyc_Absyn_AliasVar_p_Absyn_Raw_pat_struct*_tmp18A=_cycalloc(sizeof(*_tmp18A));_tmp18A->tag=2U,_tmp18A->f1=tv,_tmp18A->f2=vd;_tmp18A;});Cyc_Absyn_new_pat(_tmp6FC,loc);});});_tmp18B->f1=_tmp6FD;}),_tmp18B->f2=0,_tmp18B->f3=e,_tmp18B->f4=0;_tmp18B;});unsigned _tmp18C=loc;_tmp188(_tmp189,_tmp18C);});}
# 751
struct Cyc_Absyn_Vardecl*Cyc_Absyn_new_vardecl(unsigned varloc,struct _tuple0*x,void*t,struct Cyc_Absyn_Exp*init){
# 767
return({struct Cyc_Absyn_Vardecl*_tmp18E=_cycalloc(sizeof(*_tmp18E));_tmp18E->sc=Cyc_Absyn_Public,_tmp18E->name=x,_tmp18E->varloc=varloc,({
struct Cyc_Absyn_Tqual _tmp6FE=({Cyc_Absyn_empty_tqual(0U);});_tmp18E->tq=_tmp6FE;}),_tmp18E->type=t,_tmp18E->initializer=init,_tmp18E->rgn=0,_tmp18E->attributes=0,_tmp18E->escapes=0;_tmp18E;});}
# 751
struct Cyc_Absyn_Vardecl*Cyc_Absyn_static_vardecl(struct _tuple0*x,void*t,struct Cyc_Absyn_Exp*init){
# 773
return({struct Cyc_Absyn_Vardecl*_tmp190=_cycalloc(sizeof(*_tmp190));_tmp190->sc=Cyc_Absyn_Static,_tmp190->name=x,_tmp190->varloc=0U,({struct Cyc_Absyn_Tqual _tmp6FF=({Cyc_Absyn_empty_tqual(0U);});_tmp190->tq=_tmp6FF;}),_tmp190->type=t,_tmp190->initializer=init,_tmp190->rgn=0,_tmp190->attributes=0,_tmp190->escapes=0;_tmp190;});}
# 777
struct Cyc_Absyn_AggrdeclImpl*Cyc_Absyn_aggrdecl_impl(struct Cyc_List_List*exists,struct Cyc_List_List*po,struct Cyc_List_List*fs,int tagged){
# 781
return({struct Cyc_Absyn_AggrdeclImpl*_tmp192=_cycalloc(sizeof(*_tmp192));_tmp192->exist_vars=exists,_tmp192->rgn_po=po,_tmp192->fields=fs,_tmp192->tagged=tagged;_tmp192;});}
# 777
struct Cyc_Absyn_Decl*Cyc_Absyn_aggr_decl(enum Cyc_Absyn_AggrKind k,enum Cyc_Absyn_Scope s,struct _tuple0*n,struct Cyc_List_List*ts,struct Cyc_Absyn_AggrdeclImpl*i,struct Cyc_List_List*atts,unsigned loc){
# 787
return({({void*_tmp701=(void*)({struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct*_tmp195=_cycalloc(sizeof(*_tmp195));_tmp195->tag=5U,({struct Cyc_Absyn_Aggrdecl*_tmp700=({struct Cyc_Absyn_Aggrdecl*_tmp194=_cycalloc(sizeof(*_tmp194));_tmp194->kind=k,_tmp194->sc=s,_tmp194->name=n,_tmp194->tvs=ts,_tmp194->impl=i,_tmp194->attributes=atts,_tmp194->expected_mem_kind=0;_tmp194;});_tmp195->f1=_tmp700;});_tmp195;});Cyc_Absyn_new_decl(_tmp701,loc);});});}
# 777
struct Cyc_Absyn_TypeDecl*Cyc_Absyn_aggr_tdecl(enum Cyc_Absyn_AggrKind k,enum Cyc_Absyn_Scope s,struct _tuple0*n,struct Cyc_List_List*ts,struct Cyc_Absyn_AggrdeclImpl*i,struct Cyc_List_List*atts,unsigned loc){
# 795
return({struct Cyc_Absyn_TypeDecl*_tmp199=_cycalloc(sizeof(*_tmp199));({void*_tmp703=(void*)({struct Cyc_Absyn_Aggr_td_Absyn_Raw_typedecl_struct*_tmp198=_cycalloc(sizeof(*_tmp198));_tmp198->tag=0U,({struct Cyc_Absyn_Aggrdecl*_tmp702=({struct Cyc_Absyn_Aggrdecl*_tmp197=_cycalloc(sizeof(*_tmp197));_tmp197->kind=k,_tmp197->sc=s,_tmp197->name=n,_tmp197->tvs=ts,_tmp197->impl=i,_tmp197->attributes=atts,_tmp197->expected_mem_kind=0;_tmp197;});_tmp198->f1=_tmp702;});_tmp198;});_tmp199->r=_tmp703;}),_tmp199->loc=loc;_tmp199;});}
# 777
struct Cyc_Absyn_Decl*Cyc_Absyn_struct_decl(enum Cyc_Absyn_Scope s,struct _tuple0*n,struct Cyc_List_List*ts,struct Cyc_Absyn_AggrdeclImpl*i,struct Cyc_List_List*atts,unsigned loc){
# 804
return({Cyc_Absyn_aggr_decl(Cyc_Absyn_StructA,s,n,ts,i,atts,loc);});}
# 777
struct Cyc_Absyn_Decl*Cyc_Absyn_union_decl(enum Cyc_Absyn_Scope s,struct _tuple0*n,struct Cyc_List_List*ts,struct Cyc_Absyn_AggrdeclImpl*i,struct Cyc_List_List*atts,unsigned loc){
# 809
return({Cyc_Absyn_aggr_decl(Cyc_Absyn_UnionA,s,n,ts,i,atts,loc);});}
# 777
struct Cyc_Absyn_Decl*Cyc_Absyn_datatype_decl(enum Cyc_Absyn_Scope s,struct _tuple0*n,struct Cyc_List_List*ts,struct Cyc_Core_Opt*fs,int is_extensible,unsigned loc){
# 814
return({({void*_tmp705=(void*)({struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct*_tmp19E=_cycalloc(sizeof(*_tmp19E));_tmp19E->tag=6U,({struct Cyc_Absyn_Datatypedecl*_tmp704=({struct Cyc_Absyn_Datatypedecl*_tmp19D=_cycalloc(sizeof(*_tmp19D));_tmp19D->sc=s,_tmp19D->name=n,_tmp19D->tvs=ts,_tmp19D->fields=fs,_tmp19D->is_extensible=is_extensible;_tmp19D;});_tmp19E->f1=_tmp704;});_tmp19E;});Cyc_Absyn_new_decl(_tmp705,loc);});});}
# 777
struct Cyc_Absyn_TypeDecl*Cyc_Absyn_datatype_tdecl(enum Cyc_Absyn_Scope s,struct _tuple0*n,struct Cyc_List_List*ts,struct Cyc_Core_Opt*fs,int is_extensible,unsigned loc){
# 820
return({struct Cyc_Absyn_TypeDecl*_tmp1A2=_cycalloc(sizeof(*_tmp1A2));({void*_tmp707=(void*)({struct Cyc_Absyn_Datatype_td_Absyn_Raw_typedecl_struct*_tmp1A1=_cycalloc(sizeof(*_tmp1A1));_tmp1A1->tag=2U,({struct Cyc_Absyn_Datatypedecl*_tmp706=({struct Cyc_Absyn_Datatypedecl*_tmp1A0=_cycalloc(sizeof(*_tmp1A0));_tmp1A0->sc=s,_tmp1A0->name=n,_tmp1A0->tvs=ts,_tmp1A0->fields=fs,_tmp1A0->is_extensible=is_extensible;_tmp1A0;});_tmp1A1->f1=_tmp706;});_tmp1A1;});_tmp1A2->r=_tmp707;}),_tmp1A2->loc=loc;_tmp1A2;});}
# 777
void*Cyc_Absyn_function_type(struct Cyc_List_List*tvs,void*eff_type,struct Cyc_Absyn_Tqual ret_tqual,void*ret_type,struct Cyc_List_List*args,int c_varargs,struct Cyc_Absyn_VarargInfo*cyc_varargs,struct Cyc_List_List*rgn_po,struct Cyc_List_List*atts,struct Cyc_Absyn_Exp*req,struct Cyc_Absyn_Exp*ens,struct Cyc_List_List*ieff,struct Cyc_List_List*oeff,struct Cyc_List_List*throws,int reentrant){
# 844 "absyn.cyc"
{struct Cyc_List_List*args2=args;for(0;args2 != 0;args2=args2->tl){
({void*_tmp708=({Cyc_Absyn_pointer_expand((*((struct _tuple9*)args2->hd)).f3,1);});(*((struct _tuple9*)args2->hd)).f3=_tmp708;});}}
return(void*)({struct Cyc_Absyn_FnType_Absyn_Type_struct*_tmp1A4=_cycalloc(sizeof(*_tmp1A4));_tmp1A4->tag=5U,(_tmp1A4->f1).tvars=tvs,(_tmp1A4->f1).ret_tqual=ret_tqual,({
# 848
void*_tmp709=({Cyc_Absyn_pointer_expand(ret_type,0);});(_tmp1A4->f1).ret_type=_tmp709;}),(_tmp1A4->f1).effect=eff_type,(_tmp1A4->f1).args=args,(_tmp1A4->f1).c_varargs=c_varargs,(_tmp1A4->f1).cyc_varargs=cyc_varargs,(_tmp1A4->f1).rgn_po=rgn_po,(_tmp1A4->f1).attributes=atts,(_tmp1A4->f1).requires_clause=req,(_tmp1A4->f1).requires_relns=0,(_tmp1A4->f1).ensures_clause=ens,(_tmp1A4->f1).ensures_relns=0,(_tmp1A4->f1).ieffect=ieff,(_tmp1A4->f1).oeffect=oeff,(_tmp1A4->f1).throws=throws,(_tmp1A4->f1).reentrant=reentrant;_tmp1A4;});}
# 777 "absyn.cyc"
void*Cyc_Absyn_pointer_expand(void*t,int fresh_evar){
# 868 "absyn.cyc"
void*_stmttmp4=({Cyc_Tcutil_compress(t);});void*_tmp1A6=_stmttmp4;if(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1A6)->tag == 5U){_LL1: _LL2:
 return({void*(*_tmp1A7)(void*t,void*r,struct Cyc_Absyn_Tqual tq,void*zeroterm)=Cyc_Absyn_at_type;void*_tmp1A8=t;void*_tmp1A9=
fresh_evar?({Cyc_Absyn_new_evar(({struct Cyc_Core_Opt*_tmp1AA=_cycalloc(sizeof(*_tmp1AA));_tmp1AA->v=& Cyc_Tcutil_rk;_tmp1AA;}),0);}): Cyc_Absyn_heap_rgn_type;struct Cyc_Absyn_Tqual _tmp1AB=({Cyc_Absyn_empty_tqual(0U);});void*_tmp1AC=Cyc_Absyn_false_type;_tmp1A7(_tmp1A8,_tmp1A9,_tmp1AB,_tmp1AC);});}else{_LL3: _LL4:
# 874
 return t;}_LL0:;}
# 777 "absyn.cyc"
int Cyc_Absyn_is_lvalue(struct Cyc_Absyn_Exp*e){
# 890 "absyn.cyc"
void*_stmttmp5=e->r;void*_tmp1AE=_stmttmp5;struct Cyc_Absyn_Exp*_tmp1AF;struct Cyc_Absyn_Exp*_tmp1B0;struct Cyc_Absyn_Exp*_tmp1B1;struct Cyc_Absyn_Vardecl*_tmp1B2;struct Cyc_Absyn_Vardecl*_tmp1B3;switch(*((int*)_tmp1AE)){case 1U: switch(*((int*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp1AE)->f1)){case 2U: _LL1: _LL2:
 return 0;case 1U: _LL3: _tmp1B3=((struct Cyc_Absyn_Global_b_Absyn_Binding_struct*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp1AE)->f1)->f1;_LL4: {struct Cyc_Absyn_Vardecl*vd=_tmp1B3;
_tmp1B2=vd;goto _LL6;}case 4U: _LL5: _tmp1B2=((struct Cyc_Absyn_Local_b_Absyn_Binding_struct*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp1AE)->f1)->f1;_LL6: {struct Cyc_Absyn_Vardecl*vd=_tmp1B2;
# 894
void*_stmttmp6=({Cyc_Tcutil_compress(vd->type);});void*_tmp1B4=_stmttmp6;if(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp1B4)->tag == 4U){_LL18: _LL19:
 return 0;}else{_LL1A: _LL1B:
 return 1;}_LL17:;}default: _LL7: _LL8:
# 898
 goto _LLA;}case 22U: _LL9: _LLA:
 goto _LLC;case 20U: _LLB: _LLC:
 goto _LLE;case 23U: _LLD: _LLE:
 return 1;case 21U: _LLF: _tmp1B1=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp1AE)->f1;_LL10: {struct Cyc_Absyn_Exp*e1=_tmp1B1;
return({Cyc_Absyn_is_lvalue(e1);});}case 13U: _LL11: _tmp1B0=((struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*)_tmp1AE)->f1;_LL12: {struct Cyc_Absyn_Exp*e1=_tmp1B0;
return({Cyc_Absyn_is_lvalue(e1);});}case 12U: _LL13: _tmp1AF=((struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*)_tmp1AE)->f1;_LL14: {struct Cyc_Absyn_Exp*e1=_tmp1AF;
return({Cyc_Absyn_is_lvalue(e1);});}default: _LL15: _LL16:
 return 0;}_LL0:;}
# 909
struct Cyc_Absyn_Aggrfield*Cyc_Absyn_lookup_field(struct Cyc_List_List*fields,struct _fat_ptr*v){
{struct Cyc_List_List*fs=fields;for(0;fs != 0;fs=fs->tl){
if(({Cyc_strptrcmp(((struct Cyc_Absyn_Aggrfield*)fs->hd)->name,v);})== 0)
return(struct Cyc_Absyn_Aggrfield*)fs->hd;}}
# 910
return 0;}
# 915
struct Cyc_Absyn_Aggrfield*Cyc_Absyn_lookup_decl_field(struct Cyc_Absyn_Aggrdecl*ad,struct _fat_ptr*v){
return ad->impl == 0?0:({Cyc_Absyn_lookup_field(((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->fields,v);});}struct _tuple16{struct Cyc_Absyn_Tqual f1;void*f2;};
# 919
struct _tuple16*Cyc_Absyn_lookup_tuple_field(struct Cyc_List_List*ts,int i){
for(0;i != 0;-- i){
if(ts == 0)
return 0;
# 921
ts=ts->tl;}
# 925
if(ts == 0)
return 0;
# 925
return(struct _tuple16*)ts->hd;}
# 930
struct _fat_ptr*Cyc_Absyn_decl_name(struct Cyc_Absyn_Decl*decl){
void*_stmttmp7=decl->r;void*_tmp1B9=_stmttmp7;struct Cyc_Absyn_Fndecl*_tmp1BA;struct Cyc_Absyn_Vardecl*_tmp1BB;struct Cyc_Absyn_Typedefdecl*_tmp1BC;struct Cyc_Absyn_Enumdecl*_tmp1BD;struct Cyc_Absyn_Aggrdecl*_tmp1BE;switch(*((int*)_tmp1B9)){case 5U: _LL1: _tmp1BE=((struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct*)_tmp1B9)->f1;_LL2: {struct Cyc_Absyn_Aggrdecl*x=_tmp1BE;
return(*x->name).f2;}case 7U: _LL3: _tmp1BD=((struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct*)_tmp1B9)->f1;_LL4: {struct Cyc_Absyn_Enumdecl*x=_tmp1BD;
return(*x->name).f2;}case 8U: _LL5: _tmp1BC=((struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct*)_tmp1B9)->f1;_LL6: {struct Cyc_Absyn_Typedefdecl*x=_tmp1BC;
return(*x->name).f2;}case 0U: _LL7: _tmp1BB=((struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_tmp1B9)->f1;_LL8: {struct Cyc_Absyn_Vardecl*x=_tmp1BB;
return(*x->name).f2;}case 1U: _LL9: _tmp1BA=((struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*)_tmp1B9)->f1;_LLA: {struct Cyc_Absyn_Fndecl*x=_tmp1BA;
return(*x->name).f2;}case 13U: _LLB: _LLC:
 goto _LLE;case 14U: _LLD: _LLE:
 goto _LL10;case 15U: _LLF: _LL10:
 goto _LL12;case 16U: _LL11: _LL12:
 goto _LL14;case 2U: _LL13: _LL14:
 goto _LL16;case 6U: _LL15: _LL16:
 goto _LL18;case 3U: _LL17: _LL18:
 goto _LL1A;case 9U: _LL19: _LL1A:
 goto _LL1C;case 10U: _LL1B: _LL1C:
 goto _LL1E;case 11U: _LL1D: _LL1E:
 goto _LL20;case 12U: _LL1F: _LL20:
 goto _LL22;default: _LL21: _LL22:
 return 0;}_LL0:;}
# 953
struct Cyc_Absyn_Decl*Cyc_Absyn_lookup_decl(struct Cyc_List_List*decls,struct _fat_ptr*name){
struct _fat_ptr*dname;
for(0;decls != 0;decls=decls->tl){
dname=({Cyc_Absyn_decl_name((struct Cyc_Absyn_Decl*)decls->hd);});
if((unsigned)dname && !({Cyc_strptrcmp(dname,name);}))
return(struct Cyc_Absyn_Decl*)decls->hd;}
# 960
return 0;}
# 953
struct Cyc_Absyn_Stdcall_att_Absyn_Attribute_struct Cyc_Absyn_Stdcall_att_val={1U};
# 964
struct Cyc_Absyn_Cdecl_att_Absyn_Attribute_struct Cyc_Absyn_Cdecl_att_val={2U};
struct Cyc_Absyn_Fastcall_att_Absyn_Attribute_struct Cyc_Absyn_Fastcall_att_val={3U};
struct Cyc_Absyn_Noreturn_att_Absyn_Attribute_struct Cyc_Absyn_Noreturn_att_val={4U};
struct Cyc_Absyn_Const_att_Absyn_Attribute_struct Cyc_Absyn_Const_att_val={5U};
struct Cyc_Absyn_Packed_att_Absyn_Attribute_struct Cyc_Absyn_Packed_att_val={7U};
struct Cyc_Absyn_Nocommon_att_Absyn_Attribute_struct Cyc_Absyn_Nocommon_att_val={9U};
struct Cyc_Absyn_Shared_att_Absyn_Attribute_struct Cyc_Absyn_Shared_att_val={10U};
struct Cyc_Absyn_Unused_att_Absyn_Attribute_struct Cyc_Absyn_Unused_att_val={11U};
struct Cyc_Absyn_Weak_att_Absyn_Attribute_struct Cyc_Absyn_Weak_att_val={12U};
struct Cyc_Absyn_Dllimport_att_Absyn_Attribute_struct Cyc_Absyn_Dllimport_att_val={13U};
struct Cyc_Absyn_Dllexport_att_Absyn_Attribute_struct Cyc_Absyn_Dllexport_att_val={14U};
struct Cyc_Absyn_No_instrument_function_att_Absyn_Attribute_struct Cyc_Absyn_No_instrument_function_att_val={15U};
struct Cyc_Absyn_Constructor_att_Absyn_Attribute_struct Cyc_Absyn_Constructor_att_val={16U};
struct Cyc_Absyn_Destructor_att_Absyn_Attribute_struct Cyc_Absyn_Destructor_att_val={17U};
struct Cyc_Absyn_No_check_memory_usage_att_Absyn_Attribute_struct Cyc_Absyn_No_check_memory_usage_att_val={18U};
struct Cyc_Absyn_Pure_att_Absyn_Attribute_struct Cyc_Absyn_Pure_att_val={23U};
struct Cyc_Absyn_Always_inline_att_Absyn_Attribute_struct Cyc_Absyn_Always_inline_att_val={26U};
struct Cyc_Absyn_No_throw_att_Absyn_Attribute_struct Cyc_Absyn_No_throw_att_val={27U};
struct Cyc_Absyn_Non_null_att_Absyn_Attribute_struct Cyc_Absyn_Non_null_att_val={28U};
struct Cyc_Absyn_Deprecated_att_Absyn_Attribute_struct Cyc_Absyn_Deprecated_att_val={29U};
# 986
static void*Cyc_Absyn_bad_attribute(unsigned loc,struct _fat_ptr s){
# 988
({void*_tmp1C1=0U;({unsigned _tmp70C=loc;struct _fat_ptr _tmp70B=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp1C4=({struct Cyc_String_pa_PrintArg_struct _tmp622;_tmp622.tag=0U,_tmp622.f1=(struct _fat_ptr)((struct _fat_ptr)s);_tmp622;});void*_tmp1C2[1U];_tmp1C2[0]=& _tmp1C4;({struct _fat_ptr _tmp70A=({const char*_tmp1C3="Unrecognized attribute : %s";_tag_fat(_tmp1C3,sizeof(char),28U);});Cyc_aprintf(_tmp70A,_tag_fat(_tmp1C2,sizeof(void*),1U));});});Cyc_Warn_err(_tmp70C,_tmp70B,_tag_fat(_tmp1C1,sizeof(void*),0U));});});
return(void*)& Cyc_Absyn_Cdecl_att_val;}
# 993
static struct _fat_ptr Cyc_Absyn_exp2string_(unsigned loc,struct Cyc_Absyn_Exp*e){
void*_stmttmp8=e->r;void*_tmp1C6=_stmttmp8;struct _fat_ptr _tmp1C7;if(((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp1C6)->tag == 0U){if(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp1C6)->f1).String_c).tag == 8){_LL1: _tmp1C7=((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp1C6)->f1).String_c).val;_LL2: {struct _fat_ptr s=_tmp1C7;
return s;}}else{goto _LL3;}}else{_LL3: _LL4:
# 997
({void*_tmp1C8=0U;({unsigned _tmp70E=loc;struct _fat_ptr _tmp70D=({const char*_tmp1C9="expecting string constant";_tag_fat(_tmp1C9,sizeof(char),26U);});Cyc_Warn_err(_tmp70E,_tmp70D,_tag_fat(_tmp1C8,sizeof(void*),0U));});});
return _tag_fat(0,0,0);}_LL0:;}struct _tuple17{struct _fat_ptr f1;void*f2;};static char _tmp1CB[8U]="stdcall";static char _tmp1CC[6U]="cdecl";static char _tmp1CD[9U]="fastcall";static char _tmp1CE[9U]="noreturn";static char _tmp1CF[6U]="const";static char _tmp1D0[8U]="aligned";static char _tmp1D1[7U]="packed";static char _tmp1D2[7U]="shared";static char _tmp1D3[7U]="unused";static char _tmp1D4[5U]="weak";static char _tmp1D5[10U]="dllimport";static char _tmp1D6[10U]="dllexport";static char _tmp1D7[23U]="no_instrument_function";static char _tmp1D8[12U]="constructor";static char _tmp1D9[11U]="destructor";static char _tmp1DA[22U]="no_check_memory_usage";static char _tmp1DB[5U]="pure";static char _tmp1DC[14U]="always_inline";static char _tmp1DD[9U]="no_throw";static char _tmp1DE[8U]="nothrow";static char _tmp1DF[8U]="nonnull";static char _tmp1E0[11U]="deprecated";
# 993
void*Cyc_Absyn_parse_nullary_att(unsigned loc,struct _fat_ptr s){
# 1004
static struct Cyc_Absyn_Aligned_att_Absyn_Attribute_struct att_aligned={6U,0};
static struct _tuple17 att_map[22U]={{{_tmp1CB,_tmp1CB,_tmp1CB + 8U},(void*)& Cyc_Absyn_Stdcall_att_val},{{_tmp1CC,_tmp1CC,_tmp1CC + 6U},(void*)& Cyc_Absyn_Cdecl_att_val},{{_tmp1CD,_tmp1CD,_tmp1CD + 9U},(void*)& Cyc_Absyn_Fastcall_att_val},{{_tmp1CE,_tmp1CE,_tmp1CE + 9U},(void*)& Cyc_Absyn_Noreturn_att_val},{{_tmp1CF,_tmp1CF,_tmp1CF + 6U},(void*)& Cyc_Absyn_Const_att_val},{{_tmp1D0,_tmp1D0,_tmp1D0 + 8U},(void*)& att_aligned},{{_tmp1D1,_tmp1D1,_tmp1D1 + 7U},(void*)& Cyc_Absyn_Packed_att_val},{{_tmp1D2,_tmp1D2,_tmp1D2 + 7U},(void*)& Cyc_Absyn_Shared_att_val},{{_tmp1D3,_tmp1D3,_tmp1D3 + 7U},(void*)& Cyc_Absyn_Unused_att_val},{{_tmp1D4,_tmp1D4,_tmp1D4 + 5U},(void*)& Cyc_Absyn_Weak_att_val},{{_tmp1D5,_tmp1D5,_tmp1D5 + 10U},(void*)& Cyc_Absyn_Dllimport_att_val},{{_tmp1D6,_tmp1D6,_tmp1D6 + 10U},(void*)& Cyc_Absyn_Dllexport_att_val},{{_tmp1D7,_tmp1D7,_tmp1D7 + 23U},(void*)& Cyc_Absyn_No_instrument_function_att_val},{{_tmp1D8,_tmp1D8,_tmp1D8 + 12U},(void*)& Cyc_Absyn_Constructor_att_val},{{_tmp1D9,_tmp1D9,_tmp1D9 + 11U},(void*)& Cyc_Absyn_Destructor_att_val},{{_tmp1DA,_tmp1DA,_tmp1DA + 22U},(void*)& Cyc_Absyn_No_check_memory_usage_att_val},{{_tmp1DB,_tmp1DB,_tmp1DB + 5U},(void*)& Cyc_Absyn_Pure_att_val},{{_tmp1DC,_tmp1DC,_tmp1DC + 14U},(void*)& Cyc_Absyn_Always_inline_att_val},{{_tmp1DD,_tmp1DD,_tmp1DD + 9U},(void*)& Cyc_Absyn_No_throw_att_val},{{_tmp1DE,_tmp1DE,_tmp1DE + 8U},(void*)& Cyc_Absyn_No_throw_att_val},{{_tmp1DF,_tmp1DF,_tmp1DF + 8U},(void*)& Cyc_Absyn_Non_null_att_val},{{_tmp1E0,_tmp1E0,_tmp1E0 + 11U},(void*)& Cyc_Absyn_Deprecated_att_val}};
# 1030
if((((_get_fat_size(s,sizeof(char))> (unsigned)4 &&(int)((const char*)s.curr)[0]== (int)'_')&&(int)((const char*)s.curr)[1]== (int)'_')&&(int)*((const char*)_check_fat_subscript(s,sizeof(char),(int)(_get_fat_size(s,sizeof(char))- (unsigned)2)))== (int)'_')&&(int)*((const char*)_check_fat_subscript(s,sizeof(char),(int)(_get_fat_size(s,sizeof(char))- (unsigned)3)))== (int)'_')
# 1032
s=(struct _fat_ptr)({Cyc_substring((struct _fat_ptr)s,2,_get_fat_size(s,sizeof(char))- (unsigned)5);});{
# 1030
int i=0;
# 1034
for(0;(unsigned)i < 22U;++ i){
if(({Cyc_strcmp((struct _fat_ptr)s,(struct _fat_ptr)(*((struct _tuple17*)_check_known_subscript_notnull(att_map,22U,sizeof(struct _tuple17),i))).f1);})== 0)
return(att_map[i]).f2;}
# 1034
return({Cyc_Absyn_bad_attribute(loc,s);});}}
# 1041
static int Cyc_Absyn_exp2int(unsigned loc,struct Cyc_Absyn_Exp*e){
void*_stmttmp9=e->r;void*_tmp1E2=_stmttmp9;int _tmp1E3;if(((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp1E2)->tag == 0U){if(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp1E2)->f1).Int_c).tag == 5){_LL1: _tmp1E3=(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp1E2)->f1).Int_c).val).f2;_LL2: {int i=_tmp1E3;
return i;}}else{goto _LL3;}}else{_LL3: _LL4:
# 1045
({void*_tmp1E4=0U;({unsigned _tmp710=loc;struct _fat_ptr _tmp70F=({const char*_tmp1E5="expecting integer constant";_tag_fat(_tmp1E5,sizeof(char),27U);});Cyc_Warn_err(_tmp710,_tmp70F,_tag_fat(_tmp1E4,sizeof(void*),0U));});});
return 0;}_LL0:;}
# 1041
void*Cyc_Absyn_parse_unary_att(unsigned sloc,struct _fat_ptr s,unsigned eloc,struct Cyc_Absyn_Exp*e){
# 1053
void*a;
if(({({struct _fat_ptr _tmp712=(struct _fat_ptr)s;Cyc_zstrcmp(_tmp712,({const char*_tmp1E7="aligned";_tag_fat(_tmp1E7,sizeof(char),8U);}));});})== 0 ||({({struct _fat_ptr _tmp711=(struct _fat_ptr)s;Cyc_zstrcmp(_tmp711,({const char*_tmp1E8="__aligned__";_tag_fat(_tmp1E8,sizeof(char),12U);}));});})== 0)
return(void*)({struct Cyc_Absyn_Aligned_att_Absyn_Attribute_struct*_tmp1E9=_cycalloc(sizeof(*_tmp1E9));_tmp1E9->tag=6U,_tmp1E9->f1=e;_tmp1E9;});
# 1054
if(
# 1056
({({struct _fat_ptr _tmp714=(struct _fat_ptr)s;Cyc_zstrcmp(_tmp714,({const char*_tmp1EA="section";_tag_fat(_tmp1EA,sizeof(char),8U);}));});})== 0 ||({({struct _fat_ptr _tmp713=(struct _fat_ptr)s;Cyc_zstrcmp(_tmp713,({const char*_tmp1EB="__section__";_tag_fat(_tmp1EB,sizeof(char),12U);}));});})== 0)
return(void*)({struct Cyc_Absyn_Section_att_Absyn_Attribute_struct*_tmp1EC=_cycalloc(sizeof(*_tmp1EC));_tmp1EC->tag=8U,({struct _fat_ptr _tmp715=({Cyc_Absyn_exp2string_(eloc,e);});_tmp1EC->f1=_tmp715;});_tmp1EC;});
# 1054
if(({({struct _fat_ptr _tmp716=(struct _fat_ptr)s;Cyc_zstrcmp(_tmp716,({const char*_tmp1ED="__mode__";_tag_fat(_tmp1ED,sizeof(char),9U);}));});})== 0)
# 1059
return(void*)({struct Cyc_Absyn_Mode_att_Absyn_Attribute_struct*_tmp1EE=_cycalloc(sizeof(*_tmp1EE));_tmp1EE->tag=24U,({struct _fat_ptr _tmp717=({Cyc_Absyn_exp2string_(eloc,e);});_tmp1EE->f1=_tmp717;});_tmp1EE;});
# 1054
if(({({struct _fat_ptr _tmp718=(struct _fat_ptr)s;Cyc_zstrcmp(_tmp718,({const char*_tmp1EF="alias";_tag_fat(_tmp1EF,sizeof(char),6U);}));});})== 0)
# 1061
return(void*)({struct Cyc_Absyn_Alias_att_Absyn_Attribute_struct*_tmp1F0=_cycalloc(sizeof(*_tmp1F0));_tmp1F0->tag=25U,({struct _fat_ptr _tmp719=({Cyc_Absyn_exp2string_(eloc,e);});_tmp1F0->f1=_tmp719;});_tmp1F0;});{
# 1054
int n=({Cyc_Absyn_exp2int(eloc,e);});
# 1063
if(({({struct _fat_ptr _tmp71B=(struct _fat_ptr)s;Cyc_zstrcmp(_tmp71B,({const char*_tmp1F1="regparm";_tag_fat(_tmp1F1,sizeof(char),8U);}));});})== 0 ||({({struct _fat_ptr _tmp71A=(struct _fat_ptr)s;Cyc_zstrcmp(_tmp71A,({const char*_tmp1F2="__regparm__";_tag_fat(_tmp1F2,sizeof(char),12U);}));});})== 0){
if(n < 0 || n > 3)
({void*_tmp1F3=0U;({unsigned _tmp71D=eloc;struct _fat_ptr _tmp71C=({const char*_tmp1F4="regparm requires value between 0 and 3";_tag_fat(_tmp1F4,sizeof(char),39U);});Cyc_Warn_err(_tmp71D,_tmp71C,_tag_fat(_tmp1F3,sizeof(void*),0U));});});
# 1064
return(void*)({struct Cyc_Absyn_Regparm_att_Absyn_Attribute_struct*_tmp1F5=_cycalloc(sizeof(*_tmp1F5));
# 1066
_tmp1F5->tag=0U,_tmp1F5->f1=n;_tmp1F5;});}
# 1063
if(
# 1068
({({struct _fat_ptr _tmp71F=(struct _fat_ptr)s;Cyc_zstrcmp(_tmp71F,({const char*_tmp1F6="initializes";_tag_fat(_tmp1F6,sizeof(char),12U);}));});})== 0 ||({({struct _fat_ptr _tmp71E=(struct _fat_ptr)s;Cyc_zstrcmp(_tmp71E,({const char*_tmp1F7="__initializes__";_tag_fat(_tmp1F7,sizeof(char),16U);}));});})== 0)
return(void*)({struct Cyc_Absyn_Initializes_att_Absyn_Attribute_struct*_tmp1F8=_cycalloc(sizeof(*_tmp1F8));_tmp1F8->tag=20U,_tmp1F8->f1=n;_tmp1F8;});
# 1063
if(
# 1070
({({struct _fat_ptr _tmp721=(struct _fat_ptr)s;Cyc_zstrcmp(_tmp721,({const char*_tmp1F9="noliveunique";_tag_fat(_tmp1F9,sizeof(char),13U);}));});})== 0 ||({({struct _fat_ptr _tmp720=(struct _fat_ptr)s;Cyc_zstrcmp(_tmp720,({const char*_tmp1FA="__noliveunique__";_tag_fat(_tmp1FA,sizeof(char),17U);}));});})== 0)
return(void*)({struct Cyc_Absyn_Noliveunique_att_Absyn_Attribute_struct*_tmp1FB=_cycalloc(sizeof(*_tmp1FB));_tmp1FB->tag=21U,_tmp1FB->f1=n;_tmp1FB;});
# 1063
if(
# 1072
({({struct _fat_ptr _tmp723=(struct _fat_ptr)s;Cyc_zstrcmp(_tmp723,({const char*_tmp1FC="consume";_tag_fat(_tmp1FC,sizeof(char),8U);}));});})== 0 ||({({struct _fat_ptr _tmp722=(struct _fat_ptr)s;Cyc_zstrcmp(_tmp722,({const char*_tmp1FD="__consume__";_tag_fat(_tmp1FD,sizeof(char),12U);}));});})== 0)
return(void*)({struct Cyc_Absyn_Consume_att_Absyn_Attribute_struct*_tmp1FE=_cycalloc(sizeof(*_tmp1FE));_tmp1FE->tag=22U,_tmp1FE->f1=n;_tmp1FE;});
# 1063
if(
# 1074
({({struct _fat_ptr _tmp725=(struct _fat_ptr)s;Cyc_zstrcmp(_tmp725,({const char*_tmp1FF="nonnull";_tag_fat(_tmp1FF,sizeof(char),8U);}));});})== 0 ||({({struct _fat_ptr _tmp724=(struct _fat_ptr)s;Cyc_zstrcmp(_tmp724,({const char*_tmp200="__nonnull__";_tag_fat(_tmp200,sizeof(char),12U);}));});})== 0)
return(void*)({struct Cyc_Absyn_Non_null_att_Absyn_Attribute_struct*_tmp201=_cycalloc(sizeof(*_tmp201));_tmp201->tag=28U;_tmp201;});
# 1063
if(
# 1076
({({struct _fat_ptr _tmp727=(struct _fat_ptr)s;Cyc_zstrcmp(_tmp727,({const char*_tmp202="vector_size";_tag_fat(_tmp202,sizeof(char),12U);}));});})== 0 ||({({struct _fat_ptr _tmp726=(struct _fat_ptr)s;Cyc_zstrcmp(_tmp726,({const char*_tmp203="__vector_size__";_tag_fat(_tmp203,sizeof(char),16U);}));});})== 0)
return(void*)({struct Cyc_Absyn_VectorSize_att_Absyn_Attribute_struct*_tmp204=_cycalloc(sizeof(*_tmp204));_tmp204->tag=30U,_tmp204->f1=n;_tmp204;});
# 1063
return({Cyc_Absyn_bad_attribute(sloc,s);});}}
# 1041
void*Cyc_Absyn_parse_format_att(unsigned loc,unsigned s2loc,struct _fat_ptr s1,struct _fat_ptr s2,unsigned u1,unsigned u2){
# 1084
if(!(({({struct _fat_ptr _tmp729=(struct _fat_ptr)s1;Cyc_zstrcmp(_tmp729,({const char*_tmp206="format";_tag_fat(_tmp206,sizeof(char),7U);}));});})== 0)&& !(({({struct _fat_ptr _tmp728=(struct _fat_ptr)s1;Cyc_zstrcmp(_tmp728,({const char*_tmp207="__format__";_tag_fat(_tmp207,sizeof(char),11U);}));});})== 0))
return({Cyc_Absyn_bad_attribute(loc,s1);});
# 1084
if(
# 1086
({({struct _fat_ptr _tmp72B=(struct _fat_ptr)s2;Cyc_zstrcmp(_tmp72B,({const char*_tmp208="printf";_tag_fat(_tmp208,sizeof(char),7U);}));});})== 0 ||({({struct _fat_ptr _tmp72A=(struct _fat_ptr)s2;Cyc_zstrcmp(_tmp72A,({const char*_tmp209="__printf__";_tag_fat(_tmp209,sizeof(char),11U);}));});})== 0)
return(void*)({struct Cyc_Absyn_Format_att_Absyn_Attribute_struct*_tmp20A=_cycalloc(sizeof(*_tmp20A));_tmp20A->tag=19U,_tmp20A->f1=Cyc_Absyn_Printf_ft,_tmp20A->f2=(int)u1,_tmp20A->f3=(int)u2;_tmp20A;});
# 1084
if(
# 1088
({({struct _fat_ptr _tmp72D=(struct _fat_ptr)s2;Cyc_zstrcmp(_tmp72D,({const char*_tmp20B="scanf";_tag_fat(_tmp20B,sizeof(char),6U);}));});})== 0 ||({({struct _fat_ptr _tmp72C=(struct _fat_ptr)s2;Cyc_zstrcmp(_tmp72C,({const char*_tmp20C="__scanf__";_tag_fat(_tmp20C,sizeof(char),10U);}));});})== 0)
return(void*)({struct Cyc_Absyn_Format_att_Absyn_Attribute_struct*_tmp20D=_cycalloc(sizeof(*_tmp20D));_tmp20D->tag=19U,_tmp20D->f1=Cyc_Absyn_Scanf_ft,_tmp20D->f2=(int)u1,_tmp20D->f3=(int)u2;_tmp20D;});
# 1084
({void*_tmp20E=0U;({unsigned _tmp72F=loc;struct _fat_ptr _tmp72E=({const char*_tmp20F="unrecognized format type";_tag_fat(_tmp20F,sizeof(char),25U);});Cyc_Warn_err(_tmp72F,_tmp72E,_tag_fat(_tmp20E,sizeof(void*),0U));});});
# 1091
return(void*)& Cyc_Absyn_Cdecl_att_val;}
# 1041
struct _fat_ptr Cyc_Absyn_attribute2string(void*a){
# 1097
void*_tmp211=a;int _tmp212;struct _fat_ptr _tmp213;struct _fat_ptr _tmp214;int _tmp215;int _tmp216;int _tmp217;int _tmp219;int _tmp218;int _tmp21B;int _tmp21A;struct _fat_ptr _tmp21C;struct Cyc_Absyn_Exp*_tmp21D;int _tmp21E;switch(*((int*)_tmp211)){case 0U: _LL1: _tmp21E=((struct Cyc_Absyn_Regparm_att_Absyn_Attribute_struct*)_tmp211)->f1;_LL2: {int i=_tmp21E;
return(struct _fat_ptr)({struct Cyc_Int_pa_PrintArg_struct _tmp221=({struct Cyc_Int_pa_PrintArg_struct _tmp623;_tmp623.tag=1U,_tmp623.f1=(unsigned long)i;_tmp623;});void*_tmp21F[1U];_tmp21F[0]=& _tmp221;({struct _fat_ptr _tmp730=({const char*_tmp220="regparm(%d)";_tag_fat(_tmp220,sizeof(char),12U);});Cyc_aprintf(_tmp730,_tag_fat(_tmp21F,sizeof(void*),1U));});});}case 1U: _LL3: _LL4:
 return({const char*_tmp222="stdcall";_tag_fat(_tmp222,sizeof(char),8U);});case 2U: _LL5: _LL6:
 return({const char*_tmp223="cdecl";_tag_fat(_tmp223,sizeof(char),6U);});case 3U: _LL7: _LL8:
 return({const char*_tmp224="fastcall";_tag_fat(_tmp224,sizeof(char),9U);});case 4U: _LL9: _LLA:
 return({const char*_tmp225="noreturn";_tag_fat(_tmp225,sizeof(char),9U);});case 5U: _LLB: _LLC:
 return({const char*_tmp226="const";_tag_fat(_tmp226,sizeof(char),6U);});case 6U: _LLD: _tmp21D=((struct Cyc_Absyn_Aligned_att_Absyn_Attribute_struct*)_tmp211)->f1;_LLE: {struct Cyc_Absyn_Exp*e=_tmp21D;
# 1105
if(e == 0)return({const char*_tmp227="aligned";_tag_fat(_tmp227,sizeof(char),8U);});else{
# 1107
enum Cyc_Cyclone_C_Compilers _tmp228=Cyc_Cyclone_c_compiler;switch(_tmp228){case Cyc_Cyclone_Gcc_c: _LL42: _LL43:
 return(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp22B=({struct Cyc_String_pa_PrintArg_struct _tmp624;_tmp624.tag=0U,({struct _fat_ptr _tmp731=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_exp2string(e);}));_tmp624.f1=_tmp731;});_tmp624;});void*_tmp229[1U];_tmp229[0]=& _tmp22B;({struct _fat_ptr _tmp732=({const char*_tmp22A="aligned(%s)";_tag_fat(_tmp22A,sizeof(char),12U);});Cyc_aprintf(_tmp732,_tag_fat(_tmp229,sizeof(void*),1U));});});case Cyc_Cyclone_Vc_c: _LL44: _LL45:
 goto _LL47;default: _LL46: _LL47:
 return(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp22E=({struct Cyc_String_pa_PrintArg_struct _tmp625;_tmp625.tag=0U,({struct _fat_ptr _tmp733=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_exp2string(e);}));_tmp625.f1=_tmp733;});_tmp625;});void*_tmp22C[1U];_tmp22C[0]=& _tmp22E;({struct _fat_ptr _tmp734=({const char*_tmp22D="align(%s)";_tag_fat(_tmp22D,sizeof(char),10U);});Cyc_aprintf(_tmp734,_tag_fat(_tmp22C,sizeof(void*),1U));});});}_LL41:;}}case 7U: _LLF: _LL10:
# 1112
 return({const char*_tmp22F="packed";_tag_fat(_tmp22F,sizeof(char),7U);});case 8U: _LL11: _tmp21C=((struct Cyc_Absyn_Section_att_Absyn_Attribute_struct*)_tmp211)->f1;_LL12: {struct _fat_ptr s=_tmp21C;
return(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp232=({struct Cyc_String_pa_PrintArg_struct _tmp626;_tmp626.tag=0U,_tmp626.f1=(struct _fat_ptr)((struct _fat_ptr)s);_tmp626;});void*_tmp230[1U];_tmp230[0]=& _tmp232;({struct _fat_ptr _tmp735=({const char*_tmp231="section(\"%s\")";_tag_fat(_tmp231,sizeof(char),14U);});Cyc_aprintf(_tmp735,_tag_fat(_tmp230,sizeof(void*),1U));});});}case 9U: _LL13: _LL14:
 return({const char*_tmp233="nocommon";_tag_fat(_tmp233,sizeof(char),9U);});case 10U: _LL15: _LL16:
 return({const char*_tmp234="shared";_tag_fat(_tmp234,sizeof(char),7U);});case 11U: _LL17: _LL18:
 return({const char*_tmp235="unused";_tag_fat(_tmp235,sizeof(char),7U);});case 12U: _LL19: _LL1A:
 return({const char*_tmp236="weak";_tag_fat(_tmp236,sizeof(char),5U);});case 13U: _LL1B: _LL1C:
 return({const char*_tmp237="dllimport";_tag_fat(_tmp237,sizeof(char),10U);});case 14U: _LL1D: _LL1E:
 return({const char*_tmp238="dllexport";_tag_fat(_tmp238,sizeof(char),10U);});case 15U: _LL1F: _LL20:
 return({const char*_tmp239="no_instrument_function";_tag_fat(_tmp239,sizeof(char),23U);});case 16U: _LL21: _LL22:
 return({const char*_tmp23A="constructor";_tag_fat(_tmp23A,sizeof(char),12U);});case 17U: _LL23: _LL24:
 return({const char*_tmp23B="destructor";_tag_fat(_tmp23B,sizeof(char),11U);});case 18U: _LL25: _LL26:
 return({const char*_tmp23C="no_check_memory_usage";_tag_fat(_tmp23C,sizeof(char),22U);});case 19U: if(((struct Cyc_Absyn_Format_att_Absyn_Attribute_struct*)_tmp211)->f1 == Cyc_Absyn_Printf_ft){_LL27: _tmp21A=((struct Cyc_Absyn_Format_att_Absyn_Attribute_struct*)_tmp211)->f2;_tmp21B=((struct Cyc_Absyn_Format_att_Absyn_Attribute_struct*)_tmp211)->f3;_LL28: {int n=_tmp21A;int m=_tmp21B;
return(struct _fat_ptr)({struct Cyc_Int_pa_PrintArg_struct _tmp23F=({struct Cyc_Int_pa_PrintArg_struct _tmp628;_tmp628.tag=1U,_tmp628.f1=(unsigned)n;_tmp628;});struct Cyc_Int_pa_PrintArg_struct _tmp240=({struct Cyc_Int_pa_PrintArg_struct _tmp627;_tmp627.tag=1U,_tmp627.f1=(unsigned)m;_tmp627;});void*_tmp23D[2U];_tmp23D[0]=& _tmp23F,_tmp23D[1]=& _tmp240;({struct _fat_ptr _tmp736=({const char*_tmp23E="format(printf,%u,%u)";_tag_fat(_tmp23E,sizeof(char),21U);});Cyc_aprintf(_tmp736,_tag_fat(_tmp23D,sizeof(void*),2U));});});}}else{_LL29: _tmp218=((struct Cyc_Absyn_Format_att_Absyn_Attribute_struct*)_tmp211)->f2;_tmp219=((struct Cyc_Absyn_Format_att_Absyn_Attribute_struct*)_tmp211)->f3;_LL2A: {int n=_tmp218;int m=_tmp219;
# 1126
return(struct _fat_ptr)({struct Cyc_Int_pa_PrintArg_struct _tmp243=({struct Cyc_Int_pa_PrintArg_struct _tmp62A;_tmp62A.tag=1U,_tmp62A.f1=(unsigned)n;_tmp62A;});struct Cyc_Int_pa_PrintArg_struct _tmp244=({struct Cyc_Int_pa_PrintArg_struct _tmp629;_tmp629.tag=1U,_tmp629.f1=(unsigned)m;_tmp629;});void*_tmp241[2U];_tmp241[0]=& _tmp243,_tmp241[1]=& _tmp244;({struct _fat_ptr _tmp737=({const char*_tmp242="format(scanf,%u,%u)";_tag_fat(_tmp242,sizeof(char),20U);});Cyc_aprintf(_tmp737,_tag_fat(_tmp241,sizeof(void*),2U));});});}}case 20U: _LL2B: _tmp217=((struct Cyc_Absyn_Initializes_att_Absyn_Attribute_struct*)_tmp211)->f1;_LL2C: {int n=_tmp217;
return(struct _fat_ptr)({struct Cyc_Int_pa_PrintArg_struct _tmp247=({struct Cyc_Int_pa_PrintArg_struct _tmp62B;_tmp62B.tag=1U,_tmp62B.f1=(unsigned long)n;_tmp62B;});void*_tmp245[1U];_tmp245[0]=& _tmp247;({struct _fat_ptr _tmp738=({const char*_tmp246="initializes(%d)";_tag_fat(_tmp246,sizeof(char),16U);});Cyc_aprintf(_tmp738,_tag_fat(_tmp245,sizeof(void*),1U));});});}case 21U: _LL2D: _tmp216=((struct Cyc_Absyn_Noliveunique_att_Absyn_Attribute_struct*)_tmp211)->f1;_LL2E: {int n=_tmp216;
return(struct _fat_ptr)({struct Cyc_Int_pa_PrintArg_struct _tmp24A=({struct Cyc_Int_pa_PrintArg_struct _tmp62C;_tmp62C.tag=1U,_tmp62C.f1=(unsigned long)n;_tmp62C;});void*_tmp248[1U];_tmp248[0]=& _tmp24A;({struct _fat_ptr _tmp739=({const char*_tmp249="noliveunique(%d)";_tag_fat(_tmp249,sizeof(char),17U);});Cyc_aprintf(_tmp739,_tag_fat(_tmp248,sizeof(void*),1U));});});}case 22U: _LL2F: _tmp215=((struct Cyc_Absyn_Consume_att_Absyn_Attribute_struct*)_tmp211)->f1;_LL30: {int n=_tmp215;
return(struct _fat_ptr)({struct Cyc_Int_pa_PrintArg_struct _tmp24D=({struct Cyc_Int_pa_PrintArg_struct _tmp62D;_tmp62D.tag=1U,_tmp62D.f1=(unsigned long)n;_tmp62D;});void*_tmp24B[1U];_tmp24B[0]=& _tmp24D;({struct _fat_ptr _tmp73A=({const char*_tmp24C="consume(%d)";_tag_fat(_tmp24C,sizeof(char),12U);});Cyc_aprintf(_tmp73A,_tag_fat(_tmp24B,sizeof(void*),1U));});});}case 23U: _LL31: _LL32:
 return({const char*_tmp24E="pure";_tag_fat(_tmp24E,sizeof(char),5U);});case 26U: _LL33: _LL34:
 return({const char*_tmp24F="always_inline";_tag_fat(_tmp24F,sizeof(char),14U);});case 24U: _LL35: _tmp214=((struct Cyc_Absyn_Mode_att_Absyn_Attribute_struct*)_tmp211)->f1;_LL36: {struct _fat_ptr s=_tmp214;
return(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp252=({struct Cyc_String_pa_PrintArg_struct _tmp62E;_tmp62E.tag=0U,_tmp62E.f1=(struct _fat_ptr)((struct _fat_ptr)s);_tmp62E;});void*_tmp250[1U];_tmp250[0]=& _tmp252;({struct _fat_ptr _tmp73B=({const char*_tmp251="__mode__(\"%s\")";_tag_fat(_tmp251,sizeof(char),15U);});Cyc_aprintf(_tmp73B,_tag_fat(_tmp250,sizeof(void*),1U));});});}case 25U: _LL37: _tmp213=((struct Cyc_Absyn_Alias_att_Absyn_Attribute_struct*)_tmp211)->f1;_LL38: {struct _fat_ptr s=_tmp213;
return(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp255=({struct Cyc_String_pa_PrintArg_struct _tmp62F;_tmp62F.tag=0U,_tmp62F.f1=(struct _fat_ptr)((struct _fat_ptr)s);_tmp62F;});void*_tmp253[1U];_tmp253[0]=& _tmp255;({struct _fat_ptr _tmp73C=({const char*_tmp254="alias(\"%s\")";_tag_fat(_tmp254,sizeof(char),12U);});Cyc_aprintf(_tmp73C,_tag_fat(_tmp253,sizeof(void*),1U));});});}case 27U: _LL39: _LL3A:
 return({const char*_tmp256="nothrow";_tag_fat(_tmp256,sizeof(char),8U);});case 28U: _LL3B: _LL3C:
 return({const char*_tmp257="nonnull";_tag_fat(_tmp257,sizeof(char),8U);});case 30U: _LL3D: _tmp212=((struct Cyc_Absyn_VectorSize_att_Absyn_Attribute_struct*)_tmp211)->f1;_LL3E: {int n=_tmp212;
return(struct _fat_ptr)({struct Cyc_Int_pa_PrintArg_struct _tmp25A=({struct Cyc_Int_pa_PrintArg_struct _tmp630;_tmp630.tag=1U,_tmp630.f1=(unsigned long)n;_tmp630;});void*_tmp258[1U];_tmp258[0]=& _tmp25A;({struct _fat_ptr _tmp73D=({const char*_tmp259="vector_size(%d)";_tag_fat(_tmp259,sizeof(char),16U);});Cyc_aprintf(_tmp73D,_tag_fat(_tmp258,sizeof(void*),1U));});});}default: _LL3F: _LL40:
 return({const char*_tmp25B="deprecated";_tag_fat(_tmp25B,sizeof(char),11U);});}_LL0:;}
# 1141
static int Cyc_Absyn_attribute_case_number(void*att){
void*_tmp25D=att;switch(*((int*)_tmp25D)){case 0U: _LL1: _LL2:
 return 0;case 1U: _LL3: _LL4:
 return 1;case 2U: _LL5: _LL6:
 return 2;case 3U: _LL7: _LL8:
 return 3;case 4U: _LL9: _LLA:
 return 4;case 5U: _LLB: _LLC:
 return 5;case 6U: _LLD: _LLE:
 return 6;case 7U: _LLF: _LL10:
 return 7;case 8U: _LL11: _LL12:
 return 8;case 9U: _LL13: _LL14:
 return 9;case 10U: _LL15: _LL16:
 return 10;case 11U: _LL17: _LL18:
 return 11;case 12U: _LL19: _LL1A:
 return 12;case 13U: _LL1B: _LL1C:
 return 13;case 14U: _LL1D: _LL1E:
 return 14;case 15U: _LL1F: _LL20:
 return 15;case 16U: _LL21: _LL22:
 return 16;case 17U: _LL23: _LL24:
 return 17;case 18U: _LL25: _LL26:
 return 18;case 19U: _LL27: _LL28:
 return 19;case 20U: _LL29: _LL2A:
 return 20;default: _LL2B: _LL2C:
 return 21;}_LL0:;}
# 1167
int Cyc_Absyn_attribute_cmp(void*att1,void*att2){
struct _tuple15 _stmttmpA=({struct _tuple15 _tmp631;_tmp631.f1=att1,_tmp631.f2=att2;_tmp631;});struct _tuple15 _tmp25F=_stmttmpA;int _tmp265;int _tmp264;enum Cyc_Absyn_Format_Type _tmp263;int _tmp262;int _tmp261;enum Cyc_Absyn_Format_Type _tmp260;struct _fat_ptr _tmp267;struct _fat_ptr _tmp266;struct Cyc_Absyn_Exp*_tmp269;struct Cyc_Absyn_Exp*_tmp268;int _tmp26B;int _tmp26A;int _tmp26D;int _tmp26C;switch(*((int*)_tmp25F.f1)){case 0U: if(((struct Cyc_Absyn_Regparm_att_Absyn_Attribute_struct*)_tmp25F.f2)->tag == 0U){_LL1: _tmp26C=((struct Cyc_Absyn_Regparm_att_Absyn_Attribute_struct*)_tmp25F.f1)->f1;_tmp26D=((struct Cyc_Absyn_Regparm_att_Absyn_Attribute_struct*)_tmp25F.f2)->f1;_LL2: {int i1=_tmp26C;int i2=_tmp26D;
_tmp26A=i1;_tmp26B=i2;goto _LL4;}}else{goto _LLB;}case 20U: if(((struct Cyc_Absyn_Initializes_att_Absyn_Attribute_struct*)_tmp25F.f2)->tag == 20U){_LL3: _tmp26A=((struct Cyc_Absyn_Initializes_att_Absyn_Attribute_struct*)_tmp25F.f1)->f1;_tmp26B=((struct Cyc_Absyn_Initializes_att_Absyn_Attribute_struct*)_tmp25F.f2)->f1;_LL4: {int i1=_tmp26A;int i2=_tmp26B;
# 1171
return({Cyc_Core_intcmp(i1,i2);});}}else{goto _LLB;}case 6U: if(((struct Cyc_Absyn_Aligned_att_Absyn_Attribute_struct*)_tmp25F.f2)->tag == 6U){_LL5: _tmp268=((struct Cyc_Absyn_Aligned_att_Absyn_Attribute_struct*)_tmp25F.f1)->f1;_tmp269=((struct Cyc_Absyn_Aligned_att_Absyn_Attribute_struct*)_tmp25F.f2)->f1;_LL6: {struct Cyc_Absyn_Exp*e1=_tmp268;struct Cyc_Absyn_Exp*e2=_tmp269;
# 1173
if(e1 == e2)return 0;if(e1 == 0)
return - 1;
# 1173
if(e2 == 0)
# 1175
return 1;
# 1173
return({Cyc_Evexp_const_exp_cmp(e1,e2);});}}else{goto _LLB;}case 8U: if(((struct Cyc_Absyn_Section_att_Absyn_Attribute_struct*)_tmp25F.f2)->tag == 8U){_LL7: _tmp266=((struct Cyc_Absyn_Section_att_Absyn_Attribute_struct*)_tmp25F.f1)->f1;_tmp267=((struct Cyc_Absyn_Section_att_Absyn_Attribute_struct*)_tmp25F.f2)->f1;_LL8: {struct _fat_ptr s1=_tmp266;struct _fat_ptr s2=_tmp267;
# 1177
return({Cyc_strcmp((struct _fat_ptr)s1,(struct _fat_ptr)s2);});}}else{goto _LLB;}case 19U: if(((struct Cyc_Absyn_Format_att_Absyn_Attribute_struct*)_tmp25F.f2)->tag == 19U){_LL9: _tmp260=((struct Cyc_Absyn_Format_att_Absyn_Attribute_struct*)_tmp25F.f1)->f1;_tmp261=((struct Cyc_Absyn_Format_att_Absyn_Attribute_struct*)_tmp25F.f1)->f2;_tmp262=((struct Cyc_Absyn_Format_att_Absyn_Attribute_struct*)_tmp25F.f1)->f3;_tmp263=((struct Cyc_Absyn_Format_att_Absyn_Attribute_struct*)_tmp25F.f2)->f1;_tmp264=((struct Cyc_Absyn_Format_att_Absyn_Attribute_struct*)_tmp25F.f2)->f2;_tmp265=((struct Cyc_Absyn_Format_att_Absyn_Attribute_struct*)_tmp25F.f2)->f3;_LLA: {enum Cyc_Absyn_Format_Type ft1=_tmp260;int i1=_tmp261;int j1=_tmp262;enum Cyc_Absyn_Format_Type ft2=_tmp263;int i2=_tmp264;int j2=_tmp265;
# 1179
int ftc=({Cyc_Core_intcmp((int)((unsigned)ft1),(int)((unsigned)ft2));});
if(ftc != 0)return ftc;{int ic=({Cyc_Core_intcmp(i1,i2);});
# 1182
if(ic != 0)return ic;return({Cyc_Core_intcmp(j1,j2);});}}}else{goto _LLB;}default: _LLB: _LLC:
# 1185
 return({int(*_tmp26E)(int,int)=Cyc_Core_intcmp;int _tmp26F=({Cyc_Absyn_attribute_case_number(att1);});int _tmp270=({Cyc_Absyn_attribute_case_number(att2);});_tmp26E(_tmp26F,_tmp270);});}_LL0:;}
# 1167
int Cyc_Absyn_equal_att(void*a1,void*a2){
# 1189
return({Cyc_Absyn_attribute_cmp(a1,a2);})== 0;}
# 1167
int Cyc_Absyn_fntype_att(void*a){
# 1194
void*_tmp273=a;int _tmp274;switch(*((int*)_tmp273)){case 0U: _LL1: _tmp274=((struct Cyc_Absyn_Regparm_att_Absyn_Attribute_struct*)_tmp273)->f1;_LL2: {int i=_tmp274;
goto _LL4;}case 3U: _LL3: _LL4:
 goto _LL6;case 1U: _LL5: _LL6:
 goto _LL8;case 2U: _LL7: _LL8:
 goto _LLA;case 4U: _LL9: _LLA:
 goto _LLC;case 23U: _LLB: _LLC:
 goto _LLE;case 19U: _LLD: _LLE:
 goto _LL10;case 5U: _LLF: _LL10:
 goto _LL12;case 21U: _LL11: _LL12:
 goto _LL14;case 20U: _LL13: _LL14:
 goto _LL16;case 22U: _LL15: _LL16:
 return 1;default: _LL17: _LL18:
 return 0;}_LL0:;}static char _tmp27C[3U]="f0";
# 1167
struct _fat_ptr*Cyc_Absyn_fieldname(int i){
# 1212
static struct _fat_ptr f0={_tmp27C,_tmp27C,_tmp27C + 3U};
static struct _fat_ptr*field_names_v[1U]={& f0};
static struct _fat_ptr field_names={(void*)((struct _fat_ptr**)field_names_v),(void*)((struct _fat_ptr**)field_names_v),(void*)((struct _fat_ptr**)field_names_v + 1U)};
unsigned fsz=_get_fat_size(field_names,sizeof(struct _fat_ptr*));
if((unsigned)i >= fsz)
field_names=({unsigned _tmp27B=(unsigned)(i + 1);struct _fat_ptr**_tmp27A=_cycalloc(_check_times(_tmp27B,sizeof(struct _fat_ptr*)));({{unsigned _tmp632=(unsigned)(i + 1);unsigned j;for(j=0;j < _tmp632;++ j){
# 1219
j < fsz?_tmp27A[j]=*((struct _fat_ptr**)_check_fat_subscript(field_names,sizeof(struct _fat_ptr*),(int)j)):({struct _fat_ptr*_tmp740=({struct _fat_ptr*_tmp279=_cycalloc(sizeof(*_tmp279));({struct _fat_ptr _tmp73F=(struct _fat_ptr)({struct Cyc_Int_pa_PrintArg_struct _tmp278=({struct Cyc_Int_pa_PrintArg_struct _tmp633;_tmp633.tag=1U,_tmp633.f1=(unsigned long)((int)j);_tmp633;});void*_tmp276[1U];_tmp276[0]=& _tmp278;({struct _fat_ptr _tmp73E=({const char*_tmp277="f%d";_tag_fat(_tmp277,sizeof(char),4U);});Cyc_aprintf(_tmp73E,_tag_fat(_tmp276,sizeof(void*),1U));});});*_tmp279=_tmp73F;});_tmp279;});_tmp27A[j]=_tmp740;});}}0;});_tag_fat(_tmp27A,sizeof(struct _fat_ptr*),_tmp27B);});
# 1216
return*((struct _fat_ptr**)_check_fat_subscript(field_names,sizeof(struct _fat_ptr*),i));}struct _tuple18{enum Cyc_Absyn_AggrKind f1;struct _tuple0*f2;};
# 1223
struct _tuple18 Cyc_Absyn_aggr_kinded_name(union Cyc_Absyn_AggrInfo info){
union Cyc_Absyn_AggrInfo _tmp27E=info;struct _tuple0*_tmp280;enum Cyc_Absyn_AggrKind _tmp27F;struct _tuple0*_tmp282;enum Cyc_Absyn_AggrKind _tmp281;if((_tmp27E.UnknownAggr).tag == 1){_LL1: _tmp281=((_tmp27E.UnknownAggr).val).f1;_tmp282=((_tmp27E.UnknownAggr).val).f2;_LL2: {enum Cyc_Absyn_AggrKind ak=_tmp281;struct _tuple0*n=_tmp282;
return({struct _tuple18 _tmp634;_tmp634.f1=ak,_tmp634.f2=n;_tmp634;});}}else{_LL3: _tmp27F=(*(_tmp27E.KnownAggr).val)->kind;_tmp280=(*(_tmp27E.KnownAggr).val)->name;_LL4: {enum Cyc_Absyn_AggrKind k=_tmp27F;struct _tuple0*n=_tmp280;
return({struct _tuple18 _tmp635;_tmp635.f1=k,_tmp635.f2=n;_tmp635;});}}_LL0:;}
# 1223
struct Cyc_Absyn_Aggrdecl*Cyc_Absyn_get_known_aggrdecl(union Cyc_Absyn_AggrInfo info){
# 1230
union Cyc_Absyn_AggrInfo _tmp284=info;struct Cyc_Absyn_Aggrdecl*_tmp285;if((_tmp284.UnknownAggr).tag == 1){_LL1: _LL2:
({void*_tmp286=0U;({int(*_tmp742)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp288)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp288;});struct _fat_ptr _tmp741=({const char*_tmp287="unchecked aggrdecl";_tag_fat(_tmp287,sizeof(char),19U);});_tmp742(_tmp741,_tag_fat(_tmp286,sizeof(void*),0U));});});}else{_LL3: _tmp285=*(_tmp284.KnownAggr).val;_LL4: {struct Cyc_Absyn_Aggrdecl*ad=_tmp285;
return ad;}}_LL0:;}
# 1223
int Cyc_Absyn_is_nontagged_nonrequire_union_type(void*t){
# 1236
void*_stmttmpB=({Cyc_Tcutil_compress(t);});void*_tmp28A=_stmttmpB;union Cyc_Absyn_AggrInfo _tmp28B;struct Cyc_List_List*_tmp28C;switch(*((int*)_tmp28A)){case 7U: if(((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp28A)->f1 == Cyc_Absyn_UnionA){_LL1: _tmp28C=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp28A)->f2;_LL2: {struct Cyc_List_List*fs=_tmp28C;
# 1238
if(fs == 0)return 1;return((struct Cyc_Absyn_Aggrfield*)fs->hd)->requires_clause == 0;}}else{goto _LL5;}case 0U: if(((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp28A)->f1)->tag == 22U){_LL3: _tmp28B=((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp28A)->f1)->f1;_LL4: {union Cyc_Absyn_AggrInfo info=_tmp28B;
# 1241
union Cyc_Absyn_AggrInfo _tmp28D=info;int _tmp28F;enum Cyc_Absyn_AggrKind _tmp28E;enum Cyc_Absyn_AggrKind _tmp290;struct Cyc_Absyn_Aggrdecl*_tmp291;if((_tmp28D.KnownAggr).tag == 2){_LL8: _tmp291=*(_tmp28D.KnownAggr).val;_LL9: {struct Cyc_Absyn_Aggrdecl*ad=_tmp291;
# 1243
if((int)ad->kind != (int)Cyc_Absyn_UnionA)return 0;{struct Cyc_Absyn_AggrdeclImpl*impl=ad->impl;
# 1245
if(((struct Cyc_Absyn_AggrdeclImpl*)_check_null(impl))->tagged)return 0;{struct Cyc_List_List*fields=impl->fields;
# 1247
if(fields == 0)return 1;return((struct Cyc_Absyn_Aggrfield*)fields->hd)->requires_clause == 0;}}}}else{if(((_tmp28D.UnknownAggr).val).f3 == 0){_LLA: _tmp290=((_tmp28D.UnknownAggr).val).f1;_LLB: {enum Cyc_Absyn_AggrKind k=_tmp290;
# 1249
return(int)k == (int)Cyc_Absyn_UnionA;}}else{_LLC: _tmp28E=((_tmp28D.UnknownAggr).val).f1;_tmp28F=(int)(((_tmp28D.UnknownAggr).val).f3)->v;_LLD: {enum Cyc_Absyn_AggrKind k=_tmp28E;int b=_tmp28F;
return(int)k == (int)1U && !b;}}}_LL7:;}}else{goto _LL5;}default: _LL5: _LL6:
# 1252
 return 0;}_LL0:;}
# 1223
int Cyc_Absyn_is_require_union_type(void*t){
# 1256
void*_stmttmpC=({Cyc_Tcutil_compress(t);});void*_tmp293=_stmttmpC;union Cyc_Absyn_AggrInfo _tmp294;struct Cyc_List_List*_tmp295;switch(*((int*)_tmp293)){case 7U: if(((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp293)->f1 == Cyc_Absyn_UnionA){_LL1: _tmp295=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp293)->f2;_LL2: {struct Cyc_List_List*fs=_tmp295;
# 1258
if(fs == 0)return 0;return((struct Cyc_Absyn_Aggrfield*)fs->hd)->requires_clause != 0;}}else{goto _LL5;}case 0U: if(((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp293)->f1)->tag == 22U){_LL3: _tmp294=((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp293)->f1)->f1;_LL4: {union Cyc_Absyn_AggrInfo info=_tmp294;
# 1261
union Cyc_Absyn_AggrInfo _tmp296=info;int _tmp298;enum Cyc_Absyn_AggrKind _tmp297;enum Cyc_Absyn_AggrKind _tmp299;struct Cyc_Absyn_Aggrdecl*_tmp29A;if((_tmp296.KnownAggr).tag == 2){_LL8: _tmp29A=*(_tmp296.KnownAggr).val;_LL9: {struct Cyc_Absyn_Aggrdecl*ad=_tmp29A;
# 1263
if((int)ad->kind != (int)Cyc_Absyn_UnionA)return 0;{struct Cyc_Absyn_AggrdeclImpl*impl=ad->impl;
# 1265
if(((struct Cyc_Absyn_AggrdeclImpl*)_check_null(impl))->tagged)return 0;{struct Cyc_List_List*fields=impl->fields;
# 1267
if(fields == 0)return 0;return((struct Cyc_Absyn_Aggrfield*)fields->hd)->requires_clause != 0;}}}}else{if(((_tmp296.UnknownAggr).val).f3 == 0){_LLA: _tmp299=((_tmp296.UnknownAggr).val).f1;_LLB: {enum Cyc_Absyn_AggrKind k=_tmp299;
# 1269
return 0;}}else{_LLC: _tmp297=((_tmp296.UnknownAggr).val).f1;_tmp298=(int)(((_tmp296.UnknownAggr).val).f3)->v;_LLD: {enum Cyc_Absyn_AggrKind k=_tmp297;int b=_tmp298;
return 0;}}}_LL7:;}}else{goto _LL5;}default: _LL5: _LL6:
# 1272
 return 0;}_LL0:;}
# 1223
struct _tuple0*Cyc_Absyn_binding2qvar(void*b){
# 1277
void*_tmp29C=b;struct Cyc_Absyn_Fndecl*_tmp29D;struct Cyc_Absyn_Vardecl*_tmp29E;struct Cyc_Absyn_Vardecl*_tmp29F;struct Cyc_Absyn_Vardecl*_tmp2A0;struct Cyc_Absyn_Vardecl*_tmp2A1;struct _tuple0*_tmp2A2;switch(*((int*)_tmp29C)){case 0U: _LL1: _tmp2A2=((struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct*)_tmp29C)->f1;_LL2: {struct _tuple0*qv=_tmp2A2;
return qv;}case 1U: _LL3: _tmp2A1=((struct Cyc_Absyn_Global_b_Absyn_Binding_struct*)_tmp29C)->f1;_LL4: {struct Cyc_Absyn_Vardecl*vd=_tmp2A1;
_tmp2A0=vd;goto _LL6;}case 3U: _LL5: _tmp2A0=((struct Cyc_Absyn_Param_b_Absyn_Binding_struct*)_tmp29C)->f1;_LL6: {struct Cyc_Absyn_Vardecl*vd=_tmp2A0;
_tmp29F=vd;goto _LL8;}case 4U: _LL7: _tmp29F=((struct Cyc_Absyn_Local_b_Absyn_Binding_struct*)_tmp29C)->f1;_LL8: {struct Cyc_Absyn_Vardecl*vd=_tmp29F;
_tmp29E=vd;goto _LLA;}case 5U: _LL9: _tmp29E=((struct Cyc_Absyn_Pat_b_Absyn_Binding_struct*)_tmp29C)->f1;_LLA: {struct Cyc_Absyn_Vardecl*vd=_tmp29E;
return vd->name;}default: _LLB: _tmp29D=((struct Cyc_Absyn_Funname_b_Absyn_Binding_struct*)_tmp29C)->f1;_LLC: {struct Cyc_Absyn_Fndecl*fd=_tmp29D;
return fd->name;}}_LL0:;}
# 1223
struct _fat_ptr*Cyc_Absyn_designatorlist_to_fieldname(struct Cyc_List_List*ds){
# 1288
if(ds == 0 || ds->tl != 0)
({void*_tmp2A4=0U;({int(*_tmp744)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp2A6)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp2A6;});struct _fat_ptr _tmp743=({const char*_tmp2A5="designator list not of length 1";_tag_fat(_tmp2A5,sizeof(char),32U);});_tmp744(_tmp743,_tag_fat(_tmp2A4,sizeof(void*),0U));});});{
# 1288
void*_stmttmpD=(void*)ds->hd;void*_tmp2A7=_stmttmpD;struct _fat_ptr*_tmp2A8;if(((struct Cyc_Absyn_FieldName_Absyn_Designator_struct*)_tmp2A7)->tag == 1U){_LL1: _tmp2A8=((struct Cyc_Absyn_FieldName_Absyn_Designator_struct*)_tmp2A7)->f1;_LL2: {struct _fat_ptr*f=_tmp2A8;
# 1291
return f;}}else{_LL3: _LL4:
({void*_tmp2A9=0U;({int(*_tmp746)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp2AB)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp2AB;});struct _fat_ptr _tmp745=({const char*_tmp2AA="array designator in struct";_tag_fat(_tmp2AA,sizeof(char),27U);});_tmp746(_tmp745,_tag_fat(_tmp2A9,sizeof(void*),0U));});});}_LL0:;}}
# 1223
int Cyc_Absyn_type2bool(int def,void*t){
# 1297
void*_stmttmpE=({Cyc_Tcutil_compress(t);});void*_tmp2AD=_stmttmpE;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2AD)->tag == 0U)switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2AD)->f1)){case 13U: _LL1: _LL2:
 return 1;case 14U: _LL3: _LL4:
 return 0;default: goto _LL5;}else{_LL5: _LL6:
 return def;}_LL0:;}
# 1223
void Cyc_Absyn_visit_stmt_pop(int(*)(void*,struct Cyc_Absyn_Exp*),void(*f)(void*,struct Cyc_Absyn_Exp*),int(*)(void*,struct Cyc_Absyn_Stmt*),void(*)(void*,struct Cyc_Absyn_Stmt*),void*,struct Cyc_Absyn_Stmt*);struct _tuple19{struct _fat_ptr f1;struct Cyc_Absyn_Exp*f2;};
# 1312
void Cyc_Absyn_visit_exp_pop(int(*f1)(void*,struct Cyc_Absyn_Exp*),void(*f12)(void*,struct Cyc_Absyn_Exp*),int(*f2)(void*,struct Cyc_Absyn_Stmt*),void(*f22)(void*,struct Cyc_Absyn_Stmt*),void*env,struct Cyc_Absyn_Exp*e){
# 1318
if(!({f1(env,e);})){
# 1320
({f12(env,e);});
return;}
# 1318
{void*_stmttmpF=e->r;void*_tmp2AF=_stmttmpF;struct Cyc_List_List*_tmp2B1;struct Cyc_List_List*_tmp2B0;struct Cyc_Absyn_Stmt*_tmp2B2;struct Cyc_Absyn_Exp*_tmp2B4;struct Cyc_Absyn_Exp*_tmp2B3;struct Cyc_Absyn_Exp*_tmp2B6;struct Cyc_Absyn_Exp*_tmp2B5;struct Cyc_Absyn_Exp*_tmp2B8;struct Cyc_Absyn_Exp*_tmp2B7;struct Cyc_List_List*_tmp2B9;struct Cyc_List_List*_tmp2BA;struct Cyc_List_List*_tmp2BB;struct Cyc_List_List*_tmp2BC;struct Cyc_List_List*_tmp2BD;struct Cyc_List_List*_tmp2BE;struct Cyc_List_List*_tmp2BF;struct Cyc_List_List*_tmp2C0;struct Cyc_List_List*_tmp2C2;struct Cyc_Absyn_Exp*_tmp2C1;struct Cyc_Absyn_Exp*_tmp2C5;struct Cyc_Absyn_Exp*_tmp2C4;struct Cyc_Absyn_Exp*_tmp2C3;struct Cyc_Absyn_Exp*_tmp2C7;struct Cyc_Absyn_Exp*_tmp2C6;struct Cyc_Absyn_Exp*_tmp2C9;struct Cyc_Absyn_Exp*_tmp2C8;struct Cyc_Absyn_Exp*_tmp2CB;struct Cyc_Absyn_Exp*_tmp2CA;struct Cyc_Absyn_Exp*_tmp2CD;struct Cyc_Absyn_Exp*_tmp2CC;struct Cyc_Absyn_Exp*_tmp2CF;struct Cyc_Absyn_Exp*_tmp2CE;struct Cyc_Absyn_Exp*_tmp2D1;struct Cyc_Absyn_Exp*_tmp2D0;struct Cyc_Absyn_Exp*_tmp2D3;struct Cyc_Absyn_Exp*_tmp2D2;struct Cyc_Absyn_Exp*_tmp2D4;struct Cyc_Absyn_Exp*_tmp2D5;struct Cyc_Absyn_Exp*_tmp2D6;struct Cyc_Absyn_Exp*_tmp2D7;struct Cyc_Absyn_Exp*_tmp2D8;struct Cyc_Absyn_Exp*_tmp2D9;struct Cyc_Absyn_Exp*_tmp2DA;struct Cyc_Absyn_Exp*_tmp2DB;struct Cyc_Absyn_Exp*_tmp2DC;struct Cyc_Absyn_Exp*_tmp2DD;struct Cyc_Absyn_Exp*_tmp2DE;struct Cyc_Absyn_Exp*_tmp2DF;struct Cyc_Absyn_Exp*_tmp2E0;switch(*((int*)_tmp2AF)){case 0U: _LL1: _LL2:
# 1324
 goto _LL4;case 1U: _LL3: _LL4:
 goto _LL6;case 2U: _LL5: _LL6:
 goto _LL8;case 32U: _LL7: _LL8:
 goto _LLA;case 33U: _LL9: _LLA:
 goto _LLC;case 40U: _LLB: _LLC:
 goto _LLE;case 19U: _LLD: _LLE:
 goto _LL10;case 17U: _LLF: _LL10:
 goto _LL0;case 42U: _LL11: _tmp2E0=((struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct*)_tmp2AF)->f1;_LL12: {struct Cyc_Absyn_Exp*e1=_tmp2E0;
# 1333
_tmp2DF=e1;goto _LL14;}case 5U: _LL13: _tmp2DF=((struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*)_tmp2AF)->f1;_LL14: {struct Cyc_Absyn_Exp*e1=_tmp2DF;
# 1336
_tmp2DE=e1;goto _LL16;}case 11U: _LL15: _tmp2DE=((struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct*)_tmp2AF)->f1;_LL16: {struct Cyc_Absyn_Exp*e1=_tmp2DE;
_tmp2DD=e1;goto _LL18;}case 12U: _LL17: _tmp2DD=((struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*)_tmp2AF)->f1;_LL18: {struct Cyc_Absyn_Exp*e1=_tmp2DD;
_tmp2DC=e1;goto _LL1A;}case 13U: _LL19: _tmp2DC=((struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*)_tmp2AF)->f1;_LL1A: {struct Cyc_Absyn_Exp*e1=_tmp2DC;
_tmp2DB=e1;goto _LL1C;}case 14U: _LL1B: _tmp2DB=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp2AF)->f2;_LL1C: {struct Cyc_Absyn_Exp*e1=_tmp2DB;
_tmp2DA=e1;goto _LL1E;}case 15U: _LL1D: _tmp2DA=((struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct*)_tmp2AF)->f1;_LL1E: {struct Cyc_Absyn_Exp*e1=_tmp2DA;
_tmp2D9=e1;goto _LL20;}case 20U: _LL1F: _tmp2D9=((struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_tmp2AF)->f1;_LL20: {struct Cyc_Absyn_Exp*e1=_tmp2D9;
_tmp2D8=e1;goto _LL22;}case 21U: _LL21: _tmp2D8=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp2AF)->f1;_LL22: {struct Cyc_Absyn_Exp*e1=_tmp2D8;
_tmp2D7=e1;goto _LL24;}case 28U: _LL23: _tmp2D7=((struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*)_tmp2AF)->f1;_LL24: {struct Cyc_Absyn_Exp*e1=_tmp2D7;
_tmp2D6=e1;goto _LL26;}case 39U: _LL25: _tmp2D6=((struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct*)_tmp2AF)->f1;_LL26: {struct Cyc_Absyn_Exp*e1=_tmp2D6;
_tmp2D5=e1;goto _LL28;}case 18U: _LL27: _tmp2D5=((struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct*)_tmp2AF)->f1;_LL28: {struct Cyc_Absyn_Exp*e1=_tmp2D5;
_tmp2D4=e1;goto _LL2A;}case 22U: _LL29: _tmp2D4=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp2AF)->f1;_LL2A: {struct Cyc_Absyn_Exp*e1=_tmp2D4;
({Cyc_Absyn_visit_exp_pop(f1,f12,f2,f22,env,e1);});goto _LL0;}case 4U: _LL2B: _tmp2D2=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp2AF)->f1;_tmp2D3=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp2AF)->f3;_LL2C: {struct Cyc_Absyn_Exp*e1=_tmp2D2;struct Cyc_Absyn_Exp*e2=_tmp2D3;
# 1349
_tmp2D0=e1;_tmp2D1=e2;goto _LL2E;}case 7U: _LL2D: _tmp2D0=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp2AF)->f1;_tmp2D1=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp2AF)->f2;_LL2E: {struct Cyc_Absyn_Exp*e1=_tmp2D0;struct Cyc_Absyn_Exp*e2=_tmp2D1;
_tmp2CE=e1;_tmp2CF=e2;goto _LL30;}case 8U: _LL2F: _tmp2CE=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp2AF)->f1;_tmp2CF=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp2AF)->f2;_LL30: {struct Cyc_Absyn_Exp*e1=_tmp2CE;struct Cyc_Absyn_Exp*e2=_tmp2CF;
_tmp2CC=e1;_tmp2CD=e2;goto _LL32;}case 9U: _LL31: _tmp2CC=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp2AF)->f1;_tmp2CD=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp2AF)->f2;_LL32: {struct Cyc_Absyn_Exp*e1=_tmp2CC;struct Cyc_Absyn_Exp*e2=_tmp2CD;
_tmp2CA=e1;_tmp2CB=e2;goto _LL34;}case 23U: _LL33: _tmp2CA=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp2AF)->f1;_tmp2CB=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp2AF)->f2;_LL34: {struct Cyc_Absyn_Exp*e1=_tmp2CA;struct Cyc_Absyn_Exp*e2=_tmp2CB;
_tmp2C8=e1;_tmp2C9=e2;goto _LL36;}case 36U: _LL35: _tmp2C8=((struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct*)_tmp2AF)->f1;_tmp2C9=((struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct*)_tmp2AF)->f2;_LL36: {struct Cyc_Absyn_Exp*e1=_tmp2C8;struct Cyc_Absyn_Exp*e2=_tmp2C9;
_tmp2C6=e1;_tmp2C7=e2;goto _LL38;}case 27U: _LL37: _tmp2C6=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmp2AF)->f2;_tmp2C7=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmp2AF)->f3;_LL38: {struct Cyc_Absyn_Exp*e1=_tmp2C6;struct Cyc_Absyn_Exp*e2=_tmp2C7;
# 1356
({Cyc_Absyn_visit_exp_pop(f1,f12,f2,f22,env,e1);});
({Cyc_Absyn_visit_exp_pop(f1,f12,f2,f22,env,e2);});goto _LL0;}case 6U: _LL39: _tmp2C3=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp2AF)->f1;_tmp2C4=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp2AF)->f2;_tmp2C5=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp2AF)->f3;_LL3A: {struct Cyc_Absyn_Exp*e1=_tmp2C3;struct Cyc_Absyn_Exp*e2=_tmp2C4;struct Cyc_Absyn_Exp*e3=_tmp2C5;
# 1359
({Cyc_Absyn_visit_exp_pop(f1,f12,f2,f22,env,e1);});
({Cyc_Absyn_visit_exp_pop(f1,f12,f2,f22,env,e2);});
({Cyc_Absyn_visit_exp_pop(f1,f12,f2,f22,env,e3);});
goto _LL0;}case 10U: _LL3B: _tmp2C1=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp2AF)->f1;_tmp2C2=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp2AF)->f2;_LL3C: {struct Cyc_Absyn_Exp*e1=_tmp2C1;struct Cyc_List_List*lexp=_tmp2C2;
# 1365
for(0;lexp != 0;lexp=lexp->tl){
({Cyc_Absyn_visit_exp_pop(f1,f12,f2,f22,env,(struct Cyc_Absyn_Exp*)lexp->hd);});}
({Cyc_Absyn_visit_exp_pop(f1,f12,f2,f22,env,e1);});
goto _LL0;}case 24U: _LL3D: _tmp2C0=((struct Cyc_Absyn_Tuple_e_Absyn_Raw_exp_struct*)_tmp2AF)->f1;_LL3E: {struct Cyc_List_List*lexp=_tmp2C0;
# 1371
for(0;lexp != 0;lexp=lexp->tl){
({Cyc_Absyn_visit_exp_pop(f1,f12,f2,f22,env,(struct Cyc_Absyn_Exp*)lexp->hd);});}
goto _LL0;}case 26U: _LL3F: _tmp2BF=((struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*)_tmp2AF)->f1;_LL40: {struct Cyc_List_List*ldt=_tmp2BF;
_tmp2BE=ldt;goto _LL42;}case 25U: _LL41: _tmp2BE=((struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct*)_tmp2AF)->f2;_LL42: {struct Cyc_List_List*ldt=_tmp2BE;
_tmp2BD=ldt;goto _LL44;}case 37U: _LL43: _tmp2BD=((struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*)_tmp2AF)->f2;_LL44: {struct Cyc_List_List*ldt=_tmp2BD;
_tmp2BC=ldt;goto _LL46;}case 29U: _LL45: _tmp2BC=((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_tmp2AF)->f3;_LL46: {struct Cyc_List_List*ldt=_tmp2BC;
_tmp2BB=ldt;goto _LL48;}case 30U: _LL47: _tmp2BB=((struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct*)_tmp2AF)->f2;_LL48: {struct Cyc_List_List*ldt=_tmp2BB;
# 1379
for(0;ldt != 0;ldt=ldt->tl){
({Cyc_Absyn_visit_exp_pop(f1,f12,f2,f22,env,(*((struct _tuple14*)ldt->hd)).f2);});}
goto _LL0;}case 3U: _LL49: _tmp2BA=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp2AF)->f2;_LL4A: {struct Cyc_List_List*lexp=_tmp2BA;
# 1383
_tmp2B9=lexp;goto _LL4C;}case 31U: _LL4B: _tmp2B9=((struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*)_tmp2AF)->f1;_LL4C: {struct Cyc_List_List*lexp=_tmp2B9;
# 1385
for(0;lexp != 0;lexp=lexp->tl){
({Cyc_Absyn_visit_exp_pop(f1,f12,f2,f22,env,(struct Cyc_Absyn_Exp*)lexp->hd);});}
goto _LL0;}case 34U: _LL4D: _tmp2B7=((struct Cyc_Absyn_Spawn_e_Absyn_Raw_exp_struct*)_tmp2AF)->f1;_tmp2B8=((struct Cyc_Absyn_Spawn_e_Absyn_Raw_exp_struct*)_tmp2AF)->f2;_LL4E: {struct Cyc_Absyn_Exp*e1=_tmp2B7;struct Cyc_Absyn_Exp*e2=_tmp2B8;
# 1390
({Cyc_Absyn_visit_exp_pop(f1,f12,f2,f22,env,e1);});
({Cyc_Absyn_visit_exp_pop(f1,f12,f2,f22,env,e2);});
goto _LL0;}case 35U: _LL4F: _tmp2B5=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp2AF)->f1).rgn;_tmp2B6=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp2AF)->f1).num_elts;_LL50: {struct Cyc_Absyn_Exp*e1o=_tmp2B5;struct Cyc_Absyn_Exp*e2=_tmp2B6;
_tmp2B3=e1o;_tmp2B4=e2;goto _LL52;}case 16U: _LL51: _tmp2B3=((struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct*)_tmp2AF)->f1;_tmp2B4=((struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct*)_tmp2AF)->f2;_LL52: {struct Cyc_Absyn_Exp*e1=_tmp2B3;struct Cyc_Absyn_Exp*e2=_tmp2B4;
# 1397
if(e1 != 0)({Cyc_Absyn_visit_exp_pop(f1,f12,f2,f22,env,e1);});({Cyc_Absyn_visit_exp_pop(f1,f12,f2,f22,env,e2);});
# 1400
goto _LL0;}case 38U: _LL53: _tmp2B2=((struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct*)_tmp2AF)->f1;_LL54: {struct Cyc_Absyn_Stmt*s=_tmp2B2;
# 1402
({Cyc_Absyn_visit_stmt_pop(f1,f12,f2,f22,env,s);});goto _LL0;}default: _LL55: _tmp2B0=((struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct*)_tmp2AF)->f3;_tmp2B1=((struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct*)_tmp2AF)->f4;_LL56: {struct Cyc_List_List*sl1=_tmp2B0;struct Cyc_List_List*sl2=_tmp2B1;
# 1405
for(0;sl1 != 0;sl1=sl1->tl){
({Cyc_Absyn_visit_exp_pop(f1,f12,f2,f22,env,(*((struct _tuple19*)sl1->hd)).f2);});}
for(0;sl2 != 0;sl2=sl2->tl){
({Cyc_Absyn_visit_exp_pop(f1,f12,f2,f22,env,(*((struct _tuple19*)sl2->hd)).f2);});}
goto _LL0;}}_LL0:;}
# 1411
({f12(env,e);});}
# 1413
static void Cyc_Absyn_visit_scs_pop(int(*f1)(void*,struct Cyc_Absyn_Exp*),void(*f12)(void*,struct Cyc_Absyn_Exp*),int(*f2)(void*,struct Cyc_Absyn_Stmt*),void(*f22)(void*,struct Cyc_Absyn_Stmt*),void*env,struct Cyc_List_List*scs){
# 1419
for(0;scs != 0;scs=scs->tl){
if(((struct Cyc_Absyn_Switch_clause*)scs->hd)->where_clause != 0)
({Cyc_Absyn_visit_exp_pop(f1,f12,f2,f22,env,(struct Cyc_Absyn_Exp*)_check_null(((struct Cyc_Absyn_Switch_clause*)scs->hd)->where_clause));});
# 1420
({Cyc_Absyn_visit_stmt_pop(f1,f12,f2,f22,env,((struct Cyc_Absyn_Switch_clause*)scs->hd)->body);});}}
# 1426
void Cyc_Absyn_visit_stmt_pop(int(*f1)(void*,struct Cyc_Absyn_Exp*),void(*f12)(void*,struct Cyc_Absyn_Exp*),int(*f2)(void*,struct Cyc_Absyn_Stmt*),void(*f22)(void*,struct Cyc_Absyn_Stmt*),void*env,struct Cyc_Absyn_Stmt*s){
# 1432
if(!({f2(env,s);})){
# 1434
({f22(env,s);});
return;}
# 1432
{void*_stmttmp10=s->r;void*_tmp2E3=_stmttmp10;struct Cyc_List_List*_tmp2E5;struct Cyc_Absyn_Stmt*_tmp2E4;struct Cyc_List_List*_tmp2E7;struct Cyc_Absyn_Exp*_tmp2E6;struct Cyc_Absyn_Stmt*_tmp2E8;struct Cyc_Absyn_Stmt*_tmp2EA;struct Cyc_Absyn_Decl*_tmp2E9;struct Cyc_List_List*_tmp2EB;struct Cyc_Absyn_Stmt*_tmp2EF;struct Cyc_Absyn_Exp*_tmp2EE;struct Cyc_Absyn_Exp*_tmp2ED;struct Cyc_Absyn_Exp*_tmp2EC;struct Cyc_Absyn_Exp*_tmp2F1;struct Cyc_Absyn_Stmt*_tmp2F0;struct Cyc_Absyn_Stmt*_tmp2F3;struct Cyc_Absyn_Exp*_tmp2F2;struct Cyc_Absyn_Stmt*_tmp2F5;struct Cyc_Absyn_Stmt*_tmp2F4;struct Cyc_Absyn_Stmt*_tmp2F8;struct Cyc_Absyn_Stmt*_tmp2F7;struct Cyc_Absyn_Exp*_tmp2F6;struct Cyc_Absyn_Exp*_tmp2F9;struct Cyc_Absyn_Exp*_tmp2FA;switch(*((int*)_tmp2E3)){case 0U: _LL1: _LL2:
# 1438
 goto _LL4;case 6U: _LL3: _LL4:
 goto _LL6;case 7U: _LL5: _LL6:
 goto _LL8;case 8U: _LL7: _LL8:
 goto _LLA;case 3U: if(((struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct*)_tmp2E3)->f1 == 0){_LL9: _LLA:
 goto _LL0;}else{_LLB: _tmp2FA=((struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct*)_tmp2E3)->f1;_LLC: {struct Cyc_Absyn_Exp*e=_tmp2FA;
_tmp2F9=(struct Cyc_Absyn_Exp*)_check_null(e);goto _LLE;}}case 1U: _LLD: _tmp2F9=((struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct*)_tmp2E3)->f1;_LLE: {struct Cyc_Absyn_Exp*e=_tmp2F9;
({Cyc_Absyn_visit_exp_pop(f1,f12,f2,f22,env,e);});goto _LL0;}case 4U: _LLF: _tmp2F6=((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmp2E3)->f1;_tmp2F7=((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmp2E3)->f2;_tmp2F8=((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmp2E3)->f3;_LL10: {struct Cyc_Absyn_Exp*e1=_tmp2F6;struct Cyc_Absyn_Stmt*s1=_tmp2F7;struct Cyc_Absyn_Stmt*s2=_tmp2F8;
({Cyc_Absyn_visit_exp_pop(f1,f12,f2,f22,env,e1);});
_tmp2F4=s1;_tmp2F5=s2;goto _LL12;}case 2U: _LL11: _tmp2F4=((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_tmp2E3)->f1;_tmp2F5=((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_tmp2E3)->f2;_LL12: {struct Cyc_Absyn_Stmt*s1=_tmp2F4;struct Cyc_Absyn_Stmt*s2=_tmp2F5;
# 1448
({Cyc_Absyn_visit_stmt_pop(f1,f12,f2,f22,env,s1);});
({Cyc_Absyn_visit_stmt_pop(f1,f12,f2,f22,env,s2);});goto _LL0;}case 5U: _LL13: _tmp2F2=(((struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct*)_tmp2E3)->f1).f1;_tmp2F3=((struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct*)_tmp2E3)->f2;_LL14: {struct Cyc_Absyn_Exp*e=_tmp2F2;struct Cyc_Absyn_Stmt*s1=_tmp2F3;
_tmp2F0=s1;_tmp2F1=e;goto _LL16;}case 14U: _LL15: _tmp2F0=((struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct*)_tmp2E3)->f1;_tmp2F1=(((struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct*)_tmp2E3)->f2).f1;_LL16: {struct Cyc_Absyn_Stmt*s1=_tmp2F0;struct Cyc_Absyn_Exp*e=_tmp2F1;
# 1452
({Cyc_Absyn_visit_exp_pop(f1,f12,f2,f22,env,e);});
({Cyc_Absyn_visit_stmt_pop(f1,f12,f2,f22,env,s1);});goto _LL0;}case 9U: _LL17: _tmp2EC=((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp2E3)->f1;_tmp2ED=(((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp2E3)->f2).f1;_tmp2EE=(((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp2E3)->f3).f1;_tmp2EF=((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp2E3)->f4;_LL18: {struct Cyc_Absyn_Exp*e1=_tmp2EC;struct Cyc_Absyn_Exp*e2=_tmp2ED;struct Cyc_Absyn_Exp*e3=_tmp2EE;struct Cyc_Absyn_Stmt*s1=_tmp2EF;
# 1455
({Cyc_Absyn_visit_exp_pop(f1,f12,f2,f22,env,e1);});
({Cyc_Absyn_visit_exp_pop(f1,f12,f2,f22,env,e2);});
({Cyc_Absyn_visit_exp_pop(f1,f12,f2,f22,env,e3);});
({Cyc_Absyn_visit_stmt_pop(f1,f12,f2,f22,env,s1);});
goto _LL0;}case 11U: _LL19: _tmp2EB=((struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct*)_tmp2E3)->f1;_LL1A: {struct Cyc_List_List*es=_tmp2EB;
# 1461
for(0;es != 0;es=es->tl){
({Cyc_Absyn_visit_exp_pop(f1,f12,f2,f22,env,(struct Cyc_Absyn_Exp*)es->hd);});}
goto _LL0;}case 12U: _LL1B: _tmp2E9=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp2E3)->f1;_tmp2EA=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp2E3)->f2;_LL1C: {struct Cyc_Absyn_Decl*d=_tmp2E9;struct Cyc_Absyn_Stmt*s1=_tmp2EA;
# 1465
{void*_stmttmp11=d->r;void*_tmp2FB=_stmttmp11;struct Cyc_Absyn_Exp*_tmp2FC;struct Cyc_Absyn_Exp*_tmp2FD;struct Cyc_Absyn_Fndecl*_tmp2FE;struct Cyc_Absyn_Vardecl*_tmp2FF;switch(*((int*)_tmp2FB)){case 0U: _LL24: _tmp2FF=((struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_tmp2FB)->f1;_LL25: {struct Cyc_Absyn_Vardecl*vd=_tmp2FF;
# 1467
if(vd->initializer != 0)
({Cyc_Absyn_visit_exp_pop(f1,f12,f2,f22,env,(struct Cyc_Absyn_Exp*)_check_null(vd->initializer));});
# 1467
goto _LL23;}case 1U: _LL26: _tmp2FE=((struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*)_tmp2FB)->f1;_LL27: {struct Cyc_Absyn_Fndecl*fd=_tmp2FE;
# 1470
({Cyc_Absyn_visit_stmt_pop(f1,f12,f2,f22,env,fd->body);});goto _LL23;}case 2U: _LL28: _tmp2FD=((struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct*)_tmp2FB)->f3;_LL29: {struct Cyc_Absyn_Exp*e=_tmp2FD;
({Cyc_Absyn_visit_exp_pop(f1,f12,f2,f22,env,e);});goto _LL23;}case 4U: _LL2A: _tmp2FC=((struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*)_tmp2FB)->f3;_LL2B: {struct Cyc_Absyn_Exp*eo=_tmp2FC;
if((unsigned)eo)({Cyc_Absyn_visit_exp_pop(f1,f12,f2,f22,env,eo);});goto _LL23;}default: _LL2C: _LL2D:
 goto _LL23;}_LL23:;}
# 1475
({Cyc_Absyn_visit_stmt_pop(f1,f12,f2,f22,env,s1);});
goto _LL0;}case 13U: _LL1D: _tmp2E8=((struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct*)_tmp2E3)->f2;_LL1E: {struct Cyc_Absyn_Stmt*s1=_tmp2E8;
({Cyc_Absyn_visit_stmt_pop(f1,f12,f2,f22,env,s1);});goto _LL0;}case 10U: _LL1F: _tmp2E6=((struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct*)_tmp2E3)->f1;_tmp2E7=((struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct*)_tmp2E3)->f2;_LL20: {struct Cyc_Absyn_Exp*e=_tmp2E6;struct Cyc_List_List*scs=_tmp2E7;
# 1479
({Cyc_Absyn_visit_exp_pop(f1,f12,f2,f22,env,e);});
({Cyc_Absyn_visit_scs_pop(f1,f12,f2,f22,env,scs);});
goto _LL0;}default: _LL21: _tmp2E4=((struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct*)_tmp2E3)->f1;_tmp2E5=((struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct*)_tmp2E3)->f2;_LL22: {struct Cyc_Absyn_Stmt*s1=_tmp2E4;struct Cyc_List_List*scs=_tmp2E5;
# 1483
({Cyc_Absyn_visit_stmt_pop(f1,f12,f2,f22,env,s1);});
({Cyc_Absyn_visit_scs_pop(f1,f12,f2,f22,env,scs);});
goto _LL0;}}_LL0:;}
# 1487
({f22(env,s);});}
# 1490
static void Cyc_Absyn_dummy_stmt_pop(void*a,struct Cyc_Absyn_Stmt*s){;}
static void Cyc_Absyn_dummy_exp_pop(void*a,struct Cyc_Absyn_Exp*e){;}
# 1493
void Cyc_Absyn_visit_stmt(int(*f1)(void*,struct Cyc_Absyn_Exp*),int(*f2)(void*,struct Cyc_Absyn_Stmt*),void*env,struct Cyc_Absyn_Stmt*s){
# 1497
return({Cyc_Absyn_visit_stmt_pop(f1,Cyc_Absyn_dummy_exp_pop,f2,Cyc_Absyn_dummy_stmt_pop,env,s);});}
# 1500
void Cyc_Absyn_visit_exp(int(*f1)(void*,struct Cyc_Absyn_Exp*),int(*f2)(void*,struct Cyc_Absyn_Stmt*),void*env,struct Cyc_Absyn_Exp*e){
# 1504
return({Cyc_Absyn_visit_exp_pop(f1,Cyc_Absyn_dummy_exp_pop,f2,Cyc_Absyn_dummy_stmt_pop,env,e);});}
# 1500
static void Cyc_Absyn_i_visit_type(int(*f)(void*,void*),void*env,void*t,struct Cyc_Hashtable_Table*);
# 1523 "absyn.cyc"
static void Cyc_Absyn_i_visit_tycon_types(int(*f)(void*,void*),void*env,void*t,struct Cyc_Hashtable_Table*seen){
void*_tmp305=t;struct Cyc_Absyn_Aggrdecl*_tmp306;struct Cyc_Absyn_Datatypedecl*_tmp307;struct Cyc_Absyn_Datatypefield*_tmp309;struct Cyc_Absyn_Datatypedecl*_tmp308;switch(*((int*)_tmp305)){case 21U: if(((((struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)_tmp305)->f1).KnownDatatypefield).tag == 2){_LL1: _tmp308=(((((struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)_tmp305)->f1).KnownDatatypefield).val).f1;_tmp309=(((((struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)_tmp305)->f1).KnownDatatypefield).val).f2;_LL2: {struct Cyc_Absyn_Datatypedecl*dtd=_tmp308;struct Cyc_Absyn_Datatypefield*dtf=_tmp309;
# 1526
struct Cyc_List_List*ts=dtf->typs;
for(0;(unsigned)ts;ts=ts->tl){
struct _tuple16*_stmttmp12=(struct _tuple16*)ts->hd;void*_tmp30A;_LLA: _tmp30A=_stmttmp12->f2;_LLB: {void*t=_tmp30A;
({Cyc_Absyn_i_visit_type(f,env,t,seen);});}}
# 1531
_tmp307=dtd;goto _LL4;}}else{goto _LL7;}case 20U: if(((((struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*)_tmp305)->f1).KnownDatatype).tag == 2){_LL3: _tmp307=*((((struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*)_tmp305)->f1).KnownDatatype).val;_LL4: {struct Cyc_Absyn_Datatypedecl*dtd=_tmp307;
# 1533
if((unsigned)dtd->fields){
struct Cyc_List_List*fs=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(dtd->fields))->v;
for(0;(unsigned)fs;fs=fs->tl){
struct Cyc_List_List*ts=((struct Cyc_Absyn_Datatypefield*)fs->hd)->typs;
for(0;(unsigned)ts;ts=ts->tl){
struct _tuple16*_stmttmp13=(struct _tuple16*)ts->hd;void*_tmp30B;_LLD: _tmp30B=_stmttmp13->f2;_LLE: {void*t=_tmp30B;
({Cyc_Absyn_i_visit_type(f,env,t,seen);});}}}}
# 1533
goto _LL0;}}else{goto _LL7;}case 22U: if(((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)_tmp305)->f1).KnownAggr).tag == 2){_LL5: _tmp306=*((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)_tmp305)->f1).KnownAggr).val;_LL6: {struct Cyc_Absyn_Aggrdecl*ad=_tmp306;
# 1545
if((unsigned)ad->impl){
# 1552
struct Cyc_List_List*afl=((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->fields;
for(0;(unsigned)afl;afl=afl->tl){
({Cyc_Absyn_i_visit_type(f,env,((struct Cyc_Absyn_Aggrfield*)afl->hd)->type,seen);});}}
# 1545
goto _LL0;}}else{goto _LL7;}default: _LL7: _LL8:
# 1559
 goto _LL0;}_LL0:;}
# 1563
static void Cyc_Absyn_i_visit_type(int(*f)(void*,void*),void*env,void*t,struct Cyc_Hashtable_Table*seen){
if((unsigned)({({int*(*_tmp748)(struct Cyc_Hashtable_Table*t,void*key)=({int*(*_tmp30D)(struct Cyc_Hashtable_Table*t,void*key)=(int*(*)(struct Cyc_Hashtable_Table*t,void*key))Cyc_Hashtable_lookup_opt;_tmp30D;});struct Cyc_Hashtable_Table*_tmp747=seen;_tmp748(_tmp747,t);});}))
return;
# 1564
({({void(*_tmp74A)(struct Cyc_Hashtable_Table*t,void*key,int val)=({void(*_tmp30E)(struct Cyc_Hashtable_Table*t,void*key,int val)=(void(*)(struct Cyc_Hashtable_Table*t,void*key,int val))Cyc_Hashtable_insert;_tmp30E;});struct Cyc_Hashtable_Table*_tmp749=seen;_tmp74A(_tmp749,t,1);});});
# 1567
if(!({f(env,t);}))
return;{
# 1567
void*_tmp30F=t;void**_tmp310;void*_tmp313;struct Cyc_Absyn_Typedefdecl*_tmp312;struct Cyc_List_List*_tmp311;struct Cyc_List_List*_tmp314;struct Cyc_List_List*_tmp319;struct Cyc_Absyn_VarargInfo*_tmp318;struct Cyc_List_List*_tmp317;void*_tmp316;void*_tmp315;struct Cyc_List_List*_tmp31A;void*_tmp31C;void*_tmp31B;void*_tmp321;void*_tmp320;void*_tmp31F;void*_tmp31E;void*_tmp31D;void*_tmp322;struct Cyc_List_List*_tmp324;void*_tmp323;switch(*((int*)_tmp30F)){case 0U: _LL1: _tmp323=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp30F)->f1;_tmp324=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp30F)->f2;_LL2: {void*tc=_tmp323;struct Cyc_List_List*ts=_tmp324;
# 1571
({Cyc_Absyn_i_visit_tycon_types(f,env,tc,seen);});
for(0;(unsigned)ts;ts=ts->tl){
({Cyc_Absyn_i_visit_type(f,env,(void*)ts->hd,seen);});}
goto _LL0;}case 1U: _LL3: _tmp322=(void*)((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp30F)->f2;_LL4: {void*to=_tmp322;
# 1577
if((unsigned)to)
({Cyc_Absyn_i_visit_type(f,env,to,seen);});
# 1577
goto _LL0;}case 2U: _LL5: _LL6:
# 1580
 goto _LL0;case 3U: _LL7: _tmp31D=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp30F)->f1).elt_type;_tmp31E=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp30F)->f1).ptr_atts).rgn;_tmp31F=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp30F)->f1).ptr_atts).nullable;_tmp320=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp30F)->f1).ptr_atts).bounds;_tmp321=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp30F)->f1).ptr_atts).zero_term;_LL8: {void*ta=_tmp31D;void*e=_tmp31E;void*n=_tmp31F;void*b=_tmp320;void*z=_tmp321;
# 1582
({Cyc_Absyn_i_visit_type(f,env,ta,seen);});
({Cyc_Absyn_i_visit_type(f,env,e,seen);});
({Cyc_Absyn_i_visit_type(f,env,n,seen);});
({Cyc_Absyn_i_visit_type(f,env,b,seen);});
({Cyc_Absyn_i_visit_type(f,env,z,seen);});
# 1589
goto _LL0;}case 4U: _LL9: _tmp31B=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp30F)->f1).elt_type;_tmp31C=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp30F)->f1).zero_term;_LLA: {void*ta=_tmp31B;void*z=_tmp31C;
# 1591
({Cyc_Absyn_i_visit_type(f,env,ta,seen);});
({Cyc_Absyn_i_visit_type(f,env,z,seen);});
goto _LL0;}case 6U: _LLB: _tmp31A=((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp30F)->f1;_LLC: {struct Cyc_List_List*args=_tmp31A;
# 1596
for(0;(unsigned)args;args=args->tl){
# 1598
struct _tuple16*_stmttmp14=(struct _tuple16*)args->hd;void*_tmp325;_LL1A: _tmp325=_stmttmp14->f2;_LL1B: {void*at=_tmp325;
({Cyc_Absyn_i_visit_type(f,env,at,seen);});}}
# 1601
goto _LL0;}case 5U: _LLD: _tmp315=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp30F)->f1).effect;_tmp316=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp30F)->f1).ret_type;_tmp317=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp30F)->f1).args;_tmp318=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp30F)->f1).cyc_varargs;_tmp319=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp30F)->f1).rgn_po;_LLE: {void*e=_tmp315;void*r=_tmp316;struct Cyc_List_List*args=_tmp317;struct Cyc_Absyn_VarargInfo*va=_tmp318;struct Cyc_List_List*qb=_tmp319;
# 1603
if(e != 0)({Cyc_Absyn_i_visit_type(f,env,e,seen);});({Cyc_Absyn_i_visit_type(f,env,r,seen);});
# 1605
for(0;(unsigned)args;args=args->tl){
# 1607
struct _tuple9*_stmttmp15=(struct _tuple9*)args->hd;void*_tmp326;_LL1D: _tmp326=_stmttmp15->f3;_LL1E: {void*at=_tmp326;
({Cyc_Absyn_i_visit_type(f,env,at,seen);});}}
# 1610
if((unsigned)va)({Cyc_Absyn_i_visit_type(f,env,va->type,seen);});for(0;(unsigned)qb;qb=qb->tl){
# 1613
struct _tuple15*_stmttmp16=(struct _tuple15*)qb->hd;void*_tmp328;void*_tmp327;_LL20: _tmp327=_stmttmp16->f1;_tmp328=_stmttmp16->f2;_LL21: {void*t1=_tmp327;void*t2=_tmp328;
({Cyc_Absyn_i_visit_type(f,env,t1,seen);});
({Cyc_Absyn_i_visit_type(f,env,t2,seen);});}}
# 1617
goto _LL0;}case 7U: _LLF: _tmp314=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp30F)->f2;_LL10: {struct Cyc_List_List*afl=_tmp314;
# 1619
for(0;(unsigned)afl;afl=afl->tl){
({Cyc_Absyn_i_visit_type(f,env,((struct Cyc_Absyn_Aggrfield*)afl->hd)->type,seen);});}
# 1622
goto _LL0;}case 8U: _LL11: _tmp311=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp30F)->f2;_tmp312=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp30F)->f3;_tmp313=(void*)((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp30F)->f4;_LL12: {struct Cyc_List_List*ts=_tmp311;struct Cyc_Absyn_Typedefdecl*tdef=_tmp312;void*to=_tmp313;
# 1624
for(0;(unsigned)ts;ts=ts->tl){
({Cyc_Absyn_i_visit_type(f,env,(void*)ts->hd,seen);});}
if(tdef != 0){
# 1628
void*x=tdef->defn;
if(x != 0)({Cyc_Absyn_i_visit_type(f,env,x,seen);});}
# 1626
if((unsigned)to)
# 1632
({Cyc_Absyn_i_visit_type(f,env,to,seen);});
# 1626
goto _LL0;}case 10U: _LL13: _tmp310=((struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_tmp30F)->f2;_LL14: {void**tptr=_tmp310;
# 1635
if((unsigned)tptr &&(unsigned)*tptr)
({Cyc_Absyn_i_visit_type(f,env,*tptr,seen);});
# 1635
goto _LL0;}case 9U: _LL15: _LL16:
# 1638
 goto _LL18;default: _LL17: _LL18:
 goto _LL0;}_LL0:;}}
# 1642
int Cyc_Absyn_cmp_ptr(void*a,void*b){
return(int)((unsigned)a - (unsigned)b);}
# 1645
int Cyc_Absyn_hash_ptr(void*a){
return(int)((unsigned)a);}
# 1648
void Cyc_Absyn_visit_type(int(*f)(void*,void*),void*env,void*t){
struct _RegionHandle _tmp32C=_new_region("r");struct _RegionHandle*r=& _tmp32C;_push_region(r);
{struct Cyc_Hashtable_Table*seen=({Cyc_Hashtable_rcreate(r,20,Cyc_Absyn_cmp_ptr,Cyc_Absyn_hash_ptr);});
({Cyc_Absyn_i_visit_type(f,env,t,seen);});}
# 1650
;_pop_region();}
# 1656
static int Cyc_Absyn_no_side_effects_f1(int*env,struct Cyc_Absyn_Exp*e){
void*_stmttmp17=e->r;void*_tmp32E=_stmttmp17;switch(*((int*)_tmp32E)){case 10U: _LL1: _LL2:
 goto _LL4;case 4U: _LL3: _LL4:
 goto _LL6;case 36U: _LL5: _LL6:
 goto _LL8;case 41U: _LL7: _LL8:
 goto _LLA;case 38U: _LL9: _LLA:
 goto _LLC;case 5U: _LLB: _LLC:
*env=0;return 0;case 18U: _LLD: _LLE:
 return 0;case 34U: _LLF: _LL10:
 return 0;default: _LL11: _LL12:
 return 1;}_LL0:;}
# 1669
static int Cyc_Absyn_no_side_effects_f2(int*env,struct Cyc_Absyn_Stmt*s){
({void*_tmp330=0U;({int(*_tmp74C)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp332)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp332;});struct _fat_ptr _tmp74B=({const char*_tmp331="Compiler error: Absyn::no_side_effects looking at a statement";_tag_fat(_tmp331,sizeof(char),62U);});_tmp74C(_tmp74B,_tag_fat(_tmp330,sizeof(void*),0U));});});}
# 1669
int Cyc_Absyn_no_side_effects_exp(struct Cyc_Absyn_Exp*e){
# 1673
int ans=1;
({({void(*_tmp74D)(int(*f1)(int*,struct Cyc_Absyn_Exp*),int(*f2)(int*,struct Cyc_Absyn_Stmt*),int*env,struct Cyc_Absyn_Exp*e)=({void(*_tmp334)(int(*f1)(int*,struct Cyc_Absyn_Exp*),int(*f2)(int*,struct Cyc_Absyn_Stmt*),int*env,struct Cyc_Absyn_Exp*e)=(void(*)(int(*f1)(int*,struct Cyc_Absyn_Exp*),int(*f2)(int*,struct Cyc_Absyn_Stmt*),int*env,struct Cyc_Absyn_Exp*e))Cyc_Absyn_visit_exp;_tmp334;});_tmp74D(Cyc_Absyn_no_side_effects_f1,Cyc_Absyn_no_side_effects_f2,& ans,e);});});
return ans;}struct _tuple20{struct _tuple0*f1;int f2;};
# 1680
static int Cyc_Absyn_var_may_appear_f1(struct _tuple20*env,struct Cyc_Absyn_Exp*e){
void*_stmttmp18=e->r;void*_tmp336=_stmttmp18;struct Cyc_Absyn_Vardecl*_tmp337;void*_tmp338;switch(*((int*)_tmp336)){case 1U: _LL1: _tmp338=(void*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp336)->f1;_LL2: {void*b=_tmp338;
# 1683
if(({int(*_tmp339)(struct _tuple0*q1,struct _tuple0*q2)=Cyc_Absyn_qvar_cmp;struct _tuple0*_tmp33A=({Cyc_Absyn_binding2qvar(b);});struct _tuple0*_tmp33B=(*env).f1;_tmp339(_tmp33A,_tmp33B);})== 0)
(*env).f2=1;
# 1683
return 0;}case 41U: _LL3: _LL4:
# 1686
 goto _LL6;case 38U: _LL5: _LL6:
# 1688
(*env).f2=1;
return 0;case 27U: _LL7: _tmp337=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmp336)->f1;_LL8: {struct Cyc_Absyn_Vardecl*vd=_tmp337;
return({Cyc_Absyn_qvar_cmp(vd->name,(*env).f1);})!= 0;}default: _LL9: _LLA:
 return 1;}_LL0:;}
# 1694
static int Cyc_Absyn_var_may_appear_f2(struct _tuple20*env,struct Cyc_Absyn_Stmt*e){
({void*_tmp33D=0U;({int(*_tmp74F)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp33F)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp33F;});struct _fat_ptr _tmp74E=({const char*_tmp33E="Compiler error Absyn::no_side_effects looking at a statement";_tag_fat(_tmp33E,sizeof(char),61U);});_tmp74F(_tmp74E,_tag_fat(_tmp33D,sizeof(void*),0U));});});}
# 1694
int Cyc_Absyn_var_may_appear_exp(struct _tuple0*v,struct Cyc_Absyn_Exp*e){
# 1698
struct _tuple20 env=({struct _tuple20 _tmp636;_tmp636.f1=v,_tmp636.f2=0;_tmp636;});
({({void(*_tmp750)(int(*f1)(struct _tuple20*,struct Cyc_Absyn_Exp*),int(*f2)(struct _tuple20*,struct Cyc_Absyn_Stmt*),struct _tuple20*env,struct Cyc_Absyn_Exp*e)=({void(*_tmp341)(int(*f1)(struct _tuple20*,struct Cyc_Absyn_Exp*),int(*f2)(struct _tuple20*,struct Cyc_Absyn_Stmt*),struct _tuple20*env,struct Cyc_Absyn_Exp*e)=(void(*)(int(*f1)(struct _tuple20*,struct Cyc_Absyn_Exp*),int(*f2)(struct _tuple20*,struct Cyc_Absyn_Stmt*),struct _tuple20*env,struct Cyc_Absyn_Exp*e))Cyc_Absyn_visit_exp;_tmp341;});_tmp750(Cyc_Absyn_var_may_appear_f1,Cyc_Absyn_var_may_appear_f2,& env,e);});});
return env.f2;}struct Cyc_Absyn_NestedStmtEnv{void(*f)(void*,struct Cyc_Absyn_Stmt*);void*env;int szeof;};
# 1708
static int Cyc_Absyn_do_nested_stmt_f1(struct Cyc_Absyn_NestedStmtEnv*env,struct Cyc_Absyn_Exp*e){
void*_stmttmp19=e->r;void*_tmp343=_stmttmp19;if(((struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct*)_tmp343)->tag == 18U){_LL1: _LL2:
 return env->szeof;}else{_LL3: _LL4:
 return 1;}_LL0:;}
# 1714
static int Cyc_Absyn_do_nested_stmt_f2(struct Cyc_Absyn_NestedStmtEnv*env,struct Cyc_Absyn_Stmt*s){
# 1716
({(env->f)(env->env,s);});
return 0;}
# 1719
void Cyc_Absyn_do_nested_statement(struct Cyc_Absyn_Exp*e,void*env,void(*f)(void*,struct Cyc_Absyn_Stmt*),int szeof){
struct Cyc_Absyn_NestedStmtEnv nested_env=({struct Cyc_Absyn_NestedStmtEnv _tmp637;_tmp637.f=f,_tmp637.env=env,_tmp637.szeof=szeof;_tmp637;});
({({void(*_tmp751)(int(*f1)(struct Cyc_Absyn_NestedStmtEnv*,struct Cyc_Absyn_Exp*),int(*f2)(struct Cyc_Absyn_NestedStmtEnv*,struct Cyc_Absyn_Stmt*),struct Cyc_Absyn_NestedStmtEnv*env,struct Cyc_Absyn_Exp*e)=({void(*_tmp346)(int(*f1)(struct Cyc_Absyn_NestedStmtEnv*,struct Cyc_Absyn_Exp*),int(*f2)(struct Cyc_Absyn_NestedStmtEnv*,struct Cyc_Absyn_Stmt*),struct Cyc_Absyn_NestedStmtEnv*env,struct Cyc_Absyn_Exp*e)=(void(*)(int(*f1)(struct Cyc_Absyn_NestedStmtEnv*,struct Cyc_Absyn_Exp*),int(*f2)(struct Cyc_Absyn_NestedStmtEnv*,struct Cyc_Absyn_Stmt*),struct Cyc_Absyn_NestedStmtEnv*env,struct Cyc_Absyn_Exp*e))Cyc_Absyn_visit_exp;_tmp346;});_tmp751(Cyc_Absyn_do_nested_stmt_f1,Cyc_Absyn_do_nested_stmt_f2,& nested_env,e);});});}
# 1719
static int Cyc_Absyn_debug=0;
# 1725
static struct Cyc_Absyn_CAccessCon_Absyn_TyCon_struct Cyc_Absyn_CAccessCon_val={10U};
static struct Cyc_Absyn_XRgnHandleCon_Absyn_TyCon_struct Cyc_Absyn_XRgnHandleCon_val={4U};
static struct Cyc_Absyn_Star_Absyn_Cap_struct Cyc_Absyn_Star_val={0U};
static struct Cyc_Absyn_Bot_Absyn_Cap_struct Cyc_Absyn_Bot_val={1U};
# 1735
void*Cyc_Absyn_caccess_eff_dummy(void*r){
# 1739
int sz=({Cyc_Absyn_number_of_caps();});int i;
struct Cyc_List_List*lst=0;
for(i=0;i < sz;++ i){
lst=({struct Cyc_List_List*_tmp34A=_cycalloc(sizeof(*_tmp34A));({void*_tmp752=({void*(*_tmp348)(void*c)=Cyc_Absyn_cap2type;void*_tmp349=({Cyc_Absyn_int2cap(0);});_tmp348(_tmp349);});_tmp34A->hd=_tmp752;}),_tmp34A->tl=lst;_tmp34A;});}{
struct Cyc_List_List*lst1=({struct Cyc_List_List*_tmp34D=_cycalloc(sizeof(*_tmp34D));_tmp34D->hd=r,({struct Cyc_List_List*_tmp754=({struct Cyc_List_List*_tmp34C=_cycalloc(sizeof(*_tmp34C));({void*_tmp753=({Cyc_Absyn_join_eff(lst);});_tmp34C->hd=_tmp753;}),_tmp34C->tl=0;_tmp34C;});_tmp34D->tl=_tmp754;});_tmp34D;});
return(void*)({struct Cyc_Absyn_AppType_Absyn_Type_struct*_tmp34B=_cycalloc(sizeof(*_tmp34B));_tmp34B->tag=0U,_tmp34B->f1=(void*)& Cyc_Absyn_CAccessCon_val,_tmp34B->f2=lst1;_tmp34B;});}}
# 1735
struct _fat_ptr Cyc_Absyn_exnstr(){
# 1748
return Cyc_Absyn_exn_str;}
int Cyc_Absyn_typ2int(void*t){
# 1752
void*_tmp350=t;int _tmp351;if(((struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_tmp350)->tag == 9U){if(((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)((struct Cyc_Absyn_Exp*)((struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_tmp350)->f1)->r)->tag == 0U){if(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)((struct Cyc_Absyn_Exp*)((struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_tmp350)->f1)->r)->f1).Int_c).tag == 5){_LL1: _tmp351=(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)(((struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_tmp350)->f1)->r)->f1).Int_c).val).f2;_LL2: {int i=_tmp351;
# 1756
return i;}}else{goto _LL3;}}else{goto _LL3;}}else{_LL3: _LL4:
# 1758
({struct Cyc_String_pa_PrintArg_struct _tmp355=({struct Cyc_String_pa_PrintArg_struct _tmp638;_tmp638.tag=0U,({
struct _fat_ptr _tmp755=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t);}));_tmp638.f1=_tmp755;});_tmp638;});void*_tmp352[1U];_tmp352[0]=& _tmp355;({int(*_tmp757)(struct _fat_ptr fmt,struct _fat_ptr ap)=({
# 1758
int(*_tmp354)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp354;});struct _fat_ptr _tmp756=({const char*_tmp353="typ2int: Expected type-level integer but found %s";_tag_fat(_tmp353,sizeof(char),50U);});_tmp757(_tmp756,_tag_fat(_tmp352,sizeof(void*),1U));});});}_LL0:;}
# 1749
void*Cyc_Absyn_int2typ(int i){
# 1766
return({void*(*_tmp357)(struct Cyc_Absyn_Exp*e)=Cyc_Absyn_valueof_type;struct Cyc_Absyn_Exp*_tmp358=({Cyc_Absyn_signed_int_exp(i,0U);});_tmp357(_tmp358);});}
# 1749
struct Cyc_Absyn_Tvar*Cyc_Absyn_type2tvar(void*t){
# 1771
{void*_tmp35A=t;void*_tmp35B;struct Cyc_Absyn_Tvar*_tmp35C;switch(*((int*)_tmp35A)){case 2U: _LL1: _tmp35C=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp35A)->f1;_LL2: {struct Cyc_Absyn_Tvar*tv=_tmp35C;
# 1773
return tv;}case 1U: _LL3: _tmp35B=(void*)((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp35A)->f2;if(_tmp35B != 0){_LL4: {void*t1=_tmp35B;
# 1775
{void*_tmp35D=t1;struct Cyc_Absyn_Tvar*_tmp35E;if(_tmp35D != 0){if(((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp35D)->tag == 2U){_LL8: _tmp35E=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp35D)->f1;_LL9: {struct Cyc_Absyn_Tvar*tv=_tmp35E;
# 1777
return tv;}}else{goto _LLA;}}else{_LLA: _LLB:
 goto _LL7;}_LL7:;}
# 1780
goto _LL0;}}else{goto _LL5;}default: _LL5: _LL6:
 goto _LL0;}_LL0:;}
# 1783
({void*_tmp35F=0U;({int(*_tmp75B)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp363)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp363;});struct _fat_ptr _tmp75A=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp362=({struct Cyc_String_pa_PrintArg_struct _tmp639;_tmp639.tag=0U,({
# 1785
struct _fat_ptr _tmp758=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t);}));_tmp639.f1=_tmp758;});_tmp639;});void*_tmp360[1U];_tmp360[0]=& _tmp362;({struct _fat_ptr _tmp759=({const char*_tmp361="type2tvar: Expected tvar but found %s";_tag_fat(_tmp361,sizeof(char),38U);});Cyc_aprintf(_tmp759,_tag_fat(_tmp360,sizeof(void*),1U));});});_tmp75B(_tmp75A,_tag_fat(_tmp35F,sizeof(void*),0U));});});}
# 1749
int Cyc_Absyn_is_var_type(void*t){
# 1790
void*_tmp365=t;switch(*((int*)_tmp365)){case 2U: _LL1: _LL2:
# 1792
 goto _LL4;case 1U: _LL3: _LL4:
 return 1;default: _LL5: _LL6:
 return 0;}_LL0:;}
# 1749
int Cyc_Absyn_is_caccess_effect(void*t){
# 1800
void*_tmp367=t;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp367)->tag == 0U){if(((struct Cyc_Absyn_CAccessCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp367)->f1)->tag == 10U){_LL1: _LL2:
# 1802
 return 1;}else{goto _LL3;}}else{_LL3: _LL4:
 return 0;}_LL0:;}
# 1807
static int Cyc_Absyn_is_heap_rgn(void*t){
# 1809
void*_tmp369=t;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp369)->tag == 0U){if(((struct Cyc_Absyn_HeapCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp369)->f1)->tag == 6U){_LL1: _LL2:
# 1811
 return 1;}else{goto _LL3;}}else{_LL3: _LL4:
 return 0;}_LL0:;}
# 1816
static struct Cyc_Absyn_Tvar*Cyc_Absyn_naive_tvar_cmp(struct Cyc_Absyn_Tvar*t1,struct Cyc_Absyn_Tvar*t2){
# 1818
if(({Cyc_strptrcmp(t1->name,t2->name);})== 0)return t2;else{
return 0;}}
# 1822
static int Cyc_Absyn_naive_tvar_cmp2(struct Cyc_Absyn_Tvar*t1,struct Cyc_Absyn_Tvar*t2){
# 1824
return({Cyc_Absyn_naive_tvar_cmp(t1,t2);})!= 0;}
# 1827
static int Cyc_Absyn_my_qvar_cmp(struct _tuple0*q1,struct _tuple0*q2){
# 1830
return({int(*_tmp36D)(struct _fat_ptr s1,struct _fat_ptr s2)=Cyc_strcmp;struct _fat_ptr _tmp36E=(struct _fat_ptr)({Cyc_Absynpp_qvar2string(q1);});struct _fat_ptr _tmp36F=(struct _fat_ptr)({Cyc_Absynpp_qvar2string(q2);});_tmp36D(_tmp36E,_tmp36F);});}
# 1834
static int Cyc_Absyn_q_cmp(struct _tuple0*q1,struct _tuple0*q2){
# 1836
return({Cyc_Absyn_my_qvar_cmp(q1,q2);})== 0;}
# 1839
static struct Cyc_List_List*Cyc_Absyn_qvlist_minus(struct Cyc_List_List*l1,struct Cyc_List_List*l2){
# 1841
if(l1 == 0)return 0;{struct Cyc_List_List*ret=0;
# 1843
for(0;l1 != 0;l1=l1->tl){
if(!({({int(*_tmp75D)(int(*pred)(struct _tuple0*,struct _tuple0*),struct _tuple0*env,struct Cyc_List_List*x)=({int(*_tmp372)(int(*pred)(struct _tuple0*,struct _tuple0*),struct _tuple0*env,struct Cyc_List_List*x)=(int(*)(int(*pred)(struct _tuple0*,struct _tuple0*),struct _tuple0*env,struct Cyc_List_List*x))Cyc_List_exists_c;_tmp372;});struct _tuple0*_tmp75C=(struct _tuple0*)l1->hd;_tmp75D(Cyc_Absyn_q_cmp,_tmp75C,l2);});}))ret=({struct Cyc_List_List*_tmp373=_cycalloc(sizeof(*_tmp373));_tmp373->hd=(struct _tuple0*)l1->hd,_tmp373->tl=ret;_tmp373;});}
# 1843
return ret;}}
# 1848
static void Cyc_Absyn_print_qvlist(struct Cyc_List_List*in,struct _fat_ptr msg){
# 1850
({struct Cyc_String_pa_PrintArg_struct _tmp377=({struct Cyc_String_pa_PrintArg_struct _tmp63A;_tmp63A.tag=0U,_tmp63A.f1=(struct _fat_ptr)((struct _fat_ptr)msg);_tmp63A;});void*_tmp375[1U];_tmp375[0]=& _tmp377;({struct _fat_ptr _tmp75E=({const char*_tmp376="\nPrinting %s:\n";_tag_fat(_tmp376,sizeof(char),15U);});Cyc_printf(_tmp75E,_tag_fat(_tmp375,sizeof(void*),1U));});});
for(0;in != 0;in=in->tl){
({struct Cyc_String_pa_PrintArg_struct _tmp37A=({struct Cyc_String_pa_PrintArg_struct _tmp63B;_tmp63B.tag=0U,({struct _fat_ptr _tmp75F=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string((struct _tuple0*)in->hd);}));_tmp63B.f1=_tmp75F;});_tmp63B;});void*_tmp378[1U];_tmp378[0]=& _tmp37A;({struct _fat_ptr _tmp760=({const char*_tmp379="\n %s";_tag_fat(_tmp379,sizeof(char),5U);});Cyc_printf(_tmp760,_tag_fat(_tmp378,sizeof(void*),1U));});});}
({void*_tmp37B=0U;({struct _fat_ptr _tmp761=({const char*_tmp37C="\n";_tag_fat(_tmp37C,sizeof(char),2U);});Cyc_printf(_tmp761,_tag_fat(_tmp37B,sizeof(void*),0U));});});}
# 1848
struct Cyc_List_List*Cyc_Absyn_add_qvar(struct Cyc_List_List*t,struct _tuple0*q){
# 1858
if(t == 0)
# 1862
return({struct Cyc_List_List*_tmp37E=_cycalloc(sizeof(*_tmp37E));_tmp37E->hd=q,_tmp37E->tl=0;_tmp37E;});else{
# 1866
struct Cyc_List_List*iter=t;
struct Cyc_List_List*prev=0;
for(0;iter != 0;(prev=iter,iter=iter->tl)){
if(({Cyc_Absyn_my_qvar_cmp((struct _tuple0*)iter->hd,q);})== 0)return t;}
# 1868
({struct Cyc_List_List*_tmp762=({struct Cyc_List_List*_tmp37F=_cycalloc(sizeof(*_tmp37F));
# 1870
_tmp37F->hd=q,_tmp37F->tl=0;_tmp37F;});
# 1868
((struct Cyc_List_List*)_check_null(prev))->tl=_tmp762;});
# 1873
return t;}}
# 1878
void Cyc_Absyn_set_debug(int b){Cyc_Absyn_debug=b;}int Cyc_Absyn_get_debug(){
return Cyc_Absyn_debug;}
# 1878
void*Cyc_Absyn_xrgn_var_type(struct Cyc_Absyn_Tvar*tv){
# 1882
({void*_tmp763=({Cyc_Tcutil_kind_to_bound(& Cyc_Tcutil_xrk);});tv->kind=_tmp763;});
return(void*)({struct Cyc_Absyn_VarType_Absyn_Type_struct*_tmp383=_cycalloc(sizeof(*_tmp383));_tmp383->tag=2U,_tmp383->f1=tv;_tmp383;});}
# 1878
int Cyc_Absyn_is_xrgn_tvar(struct Cyc_Absyn_Tvar*tv){
# 1888
void*knd=({Cyc_Absyn_compress_kb(tv->kind);});
void*_tmp385=knd;struct Cyc_Absyn_Kind*_tmp386;if(((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp385)->tag == 0U){_LL1: _tmp386=((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp385)->f1;_LL2: {struct Cyc_Absyn_Kind*k=_tmp386;
# 1892
return({Cyc_Tcutil_kind_eq(k,& Cyc_Tcutil_xrk);});}}else{_LL3: _LL4:
 return 0;}_LL0:;}
# 1878
int Cyc_Absyn_is_xrgn(void*r){
# 1899
void*_tmp388=r;enum Cyc_Absyn_KindQual _tmp389;struct Cyc_Absyn_Tvar*_tmp38A;switch(*((int*)_tmp388)){case 2U: _LL1: _tmp38A=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp388)->f1;_LL2: {struct Cyc_Absyn_Tvar*tv=_tmp38A;
# 1902
return({Cyc_Absyn_is_xrgn_tvar(tv);});}case 1U: if(((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp388)->f1 != 0){_LL3: _tmp389=((struct Cyc_Absyn_Kind*)(((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp388)->f1)->v)->kind;if((int)_tmp389 == (int)3U){_LL4: {enum Cyc_Absyn_KindQual kind=_tmp389;
# 1904
return 1;}}else{goto _LL5;}}else{goto _LL5;}default: _LL5: _LL6:
 return 0;}_LL0:;}
# 1878
void*Cyc_Absyn_xrgn_handle_type(void*r){
# 1913
{void*_tmp38C=r;struct Cyc_Absyn_Tvar*_tmp38D;if(((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp38C)->tag == 2U){_LL1: _tmp38D=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp38C)->f1;_LL2: {struct Cyc_Absyn_Tvar*tv=_tmp38D;
# 1916
({void*_tmp764=({Cyc_Tcutil_kind_to_bound(& Cyc_Tcutil_xrk);});tv->kind=_tmp764;});
goto _LL0;}}else{_LL3: _LL4:
 goto _LL0;}_LL0:;}
# 1920
return({void*_tmp38E[1U];_tmp38E[0]=r;Cyc_Absyn_app_type((void*)& Cyc_Absyn_XRgnHandleCon_val,_tag_fat(_tmp38E,sizeof(void*),1U));});}
# 1878
struct _fat_ptr Cyc_Absyn_list2string(struct Cyc_List_List*l,struct _fat_ptr(*cb)(void*)){
# 1925
if(l == 0)return({const char*_tmp390="";_tag_fat(_tmp390,sizeof(char),1U);});{struct _fat_ptr ret=({cb(l->hd);});
# 1927
for(l=l->tl;l != 0;l=l->tl){ret=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp393=({struct Cyc_String_pa_PrintArg_struct _tmp63D;_tmp63D.tag=0U,_tmp63D.f1=(struct _fat_ptr)((struct _fat_ptr)ret);_tmp63D;});struct Cyc_String_pa_PrintArg_struct _tmp394=({struct Cyc_String_pa_PrintArg_struct _tmp63C;_tmp63C.tag=0U,({struct _fat_ptr _tmp765=(struct _fat_ptr)((struct _fat_ptr)({cb(l->hd);}));_tmp63C.f1=_tmp765;});_tmp63C;});void*_tmp391[2U];_tmp391[0]=& _tmp393,_tmp391[1]=& _tmp394;({struct _fat_ptr _tmp766=({const char*_tmp392="%s,%s";_tag_fat(_tmp392,sizeof(char),6U);});Cyc_aprintf(_tmp766,_tag_fat(_tmp391,sizeof(void*),2U));});});}
return ret;}}
# 1878
int Cyc_Absyn_xorptr(void*p1,void*p2){
# 1933
return p1 == 0 && p2 == 0 ||
 p1 != 0 && p2 != 0;}
# 1937
void*Cyc_Absyn_nonnull(void*p1,void*p2){
# 1939
return p1 != 0?p1: p2;}
# 1937
void*Cyc_Absyn_new_nat_cap(int i,int b){
# 1947
if(i < 0)({void*_tmp398=0U;({int(*_tmp768)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp39A)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp39A;});struct _fat_ptr _tmp767=({const char*_tmp399="new_nat_cap: Expected i >= 0";_tag_fat(_tmp399,sizeof(char),29U);});_tmp768(_tmp767,_tag_fat(_tmp398,sizeof(void*),0U));});});return(void*)({struct Cyc_Absyn_Nat_Absyn_Cap_struct*_tmp39B=_cycalloc(sizeof(*_tmp39B));
_tmp39B->tag=2U,_tmp39B->f1=i,_tmp39B->f2=b;_tmp39B;});}
# 1937
void*Cyc_Absyn_bot_cap(){
# 1951
return(void*)& Cyc_Absyn_Bot_val;}
# 1937
void*Cyc_Absyn_star_cap(){
# 1953
return(void*)& Cyc_Absyn_Star_val;}
# 1937
int Cyc_Absyn_is_bot_cap(void*c){
# 1955
return c == (void*)& Cyc_Absyn_Bot_val;}
# 1937
int Cyc_Absyn_is_star_cap(void*c){
# 1957
return c == (void*)& Cyc_Absyn_Star_val;}
# 1937
int Cyc_Absyn_is_nat_cap(void*c){
# 1961
void*_tmp3A1=c;if(((struct Cyc_Absyn_Nat_Absyn_Cap_struct*)_tmp3A1)->tag == 2U){_LL1: _LL2:
# 1963
 return 1;}else{_LL3: _LL4:
 return 0;}_LL0:;}
# 1937
int Cyc_Absyn_is_nat_bar_cap(void*c){
# 1970
void*_tmp3A3=c;if(((struct Cyc_Absyn_Nat_Absyn_Cap_struct*)_tmp3A3)->tag == 2U){if(((struct Cyc_Absyn_Nat_Absyn_Cap_struct*)_tmp3A3)->f2 == 1){_LL1: _LL2:
# 1972
 return 1;}else{goto _LL3;}}else{_LL3: _LL4:
 return 0;}_LL0:;}
# 1937
int Cyc_Absyn_is_nat_nobar_cap(void*c){
# 1979
void*_tmp3A5=c;if(((struct Cyc_Absyn_Nat_Absyn_Cap_struct*)_tmp3A5)->tag == 2U){if(((struct Cyc_Absyn_Nat_Absyn_Cap_struct*)_tmp3A5)->f2 == 0){_LL1: _LL2:
# 1981
 return 1;}else{goto _LL3;}}else{_LL3: _LL4:
 return 0;}_LL0:;}
# 1986
int Cyc_Absyn_get_nat_cap(void*c){
# 1988
void*_tmp3A7=c;int _tmp3A8;if(((struct Cyc_Absyn_Nat_Absyn_Cap_struct*)_tmp3A7)->tag == 2U){_LL1: _tmp3A8=((struct Cyc_Absyn_Nat_Absyn_Cap_struct*)_tmp3A7)->f1;_LL2: {int a=_tmp3A8;
# 1990
return a;}}else{_LL3: _LL4:
# 1992
({void*_tmp3A9=0U;({int(*_tmp76A)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp3AB)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp3AB;});struct _fat_ptr _tmp769=({const char*_tmp3AA="get_nat_cap: Expected Nat cap";_tag_fat(_tmp3AA,sizeof(char),30U);});_tmp76A(_tmp769,_tag_fat(_tmp3A9,sizeof(void*),0U));});});}_LL0:;}
# 1986
void*Cyc_Absyn_int2cap(int i){
# 1999
if((unsigned)i == 1U << sizeof(int)* 8U - 1U)return(void*)& Cyc_Absyn_Star_val;else{
if((unsigned)i == (1U << sizeof(int)* 8U - 1U)+ 1U)return(void*)& Cyc_Absyn_Bot_val;else{
if(i >= 0)return({Cyc_Absyn_new_nat_cap(i,0);});else{
if(i < 0 &&(unsigned)i > (1U << sizeof(int)* 8U - 1U)+ 2U)return({Cyc_Absyn_new_nat_cap(- i,1);});}}}
# 1999
({struct Cyc_Int_pa_PrintArg_struct _tmp3B0=({struct Cyc_Int_pa_PrintArg_struct _tmp63E;_tmp63E.tag=1U,_tmp63E.f1=(unsigned long)i;_tmp63E;});void*_tmp3AD[1U];_tmp3AD[0]=& _tmp3B0;({int(*_tmp76C)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp3AF)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp3AF;});struct _fat_ptr _tmp76B=({const char*_tmp3AE="int2cap %d";_tag_fat(_tmp3AE,sizeof(char),11U);});_tmp76C(_tmp76B,_tag_fat(_tmp3AD,sizeof(void*),1U));});});}
# 2007
int Cyc_Absyn_cap2int(void*c){
# 2009
void*_tmp3B2=c;int _tmp3B3;int _tmp3B4;switch(*((int*)_tmp3B2)){case 0U: _LL1: _LL2:
# 2011
 return(int)((unsigned)1 << sizeof(int)* (unsigned)8 - (unsigned)1);case 1U: _LL3: _LL4:
 return(int)(((unsigned)1 << sizeof(int)* (unsigned)8 - (unsigned)1)+ (unsigned)1);default: switch(((struct Cyc_Absyn_Nat_Absyn_Cap_struct*)_tmp3B2)->f2){case 0U: _LL5: _tmp3B4=((struct Cyc_Absyn_Nat_Absyn_Cap_struct*)_tmp3B2)->f1;if(_tmp3B4 >= 0){_LL6: {int i=_tmp3B4;
return i;}}else{goto _LL9;}case 1U: _LL7: _tmp3B3=((struct Cyc_Absyn_Nat_Absyn_Cap_struct*)_tmp3B2)->f1;if(
_tmp3B3 > 0 &&(unsigned)(- _tmp3B3)> (1U << sizeof(int)* 8U - 1U)+ 2U){_LL8: {int i=_tmp3B3;return - i;}}else{goto _LL9;}default: _LL9: _LLA:
({struct Cyc_String_pa_PrintArg_struct _tmp3B8=({struct Cyc_String_pa_PrintArg_struct _tmp63F;_tmp63F.tag=0U,({struct _fat_ptr _tmp76D=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_cap2string(c);}));_tmp63F.f1=_tmp76D;});_tmp63F;});void*_tmp3B5[1U];_tmp3B5[0]=& _tmp3B8;({int(*_tmp76F)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp3B7)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp3B7;});struct _fat_ptr _tmp76E=({const char*_tmp3B6="cap2int %s";_tag_fat(_tmp3B6,sizeof(char),11U);});_tmp76F(_tmp76E,_tag_fat(_tmp3B5,sizeof(void*),1U));});});}}_LL0:;}
# 2007
void*Cyc_Absyn_cap2type(void*c){
# 2019
return({void*(*_tmp3BA)(int i)=Cyc_Absyn_int2typ;int _tmp3BB=({Cyc_Absyn_cap2int(c);});_tmp3BA(_tmp3BB);});}
# 2007
void*Cyc_Absyn_type2cap(void*c){
# 2021
return({void*(*_tmp3BD)(int i)=Cyc_Absyn_int2cap;int _tmp3BE=({Cyc_Absyn_typ2int(c);});_tmp3BD(_tmp3BE);});}struct _tuple21{int f1;int f2;};
# 2023
static struct _tuple21 Cyc_Absyn_parse_int(struct _fat_ptr str){
# 2025
if(({char*_tmp770=(char*)str.curr;_tmp770 == (char*)(_tag_fat(0,0,0)).curr;}))return({struct _tuple21 _tmp640;_tmp640.f1=0,_tmp640.f2=0;_tmp640;});{int ret;int cnt;
# 2027
for((ret=0,cnt=1);(int)*((const char*)_check_fat_subscript(str,sizeof(char),0U))!= (int)'\000';(_fat_ptr_inplace_plus_post(& str,sizeof(char),1),cnt *=10)){
# 2029
char x=*((const char*)_check_fat_subscript(str,sizeof(char),0U));
if((int)x > (int)'9' ||(int)x < (int)'0')return({struct _tuple21 _tmp641;_tmp641.f1=0,_tmp641.f2=0;_tmp641;});else{
ret=ret * cnt + ((int)x - (int)'0');}}
# 2033
return({struct _tuple21 _tmp642;_tmp642.f1=ret,_tmp642.f2=1;_tmp642;});}}
# 2037
static void*Cyc_Absyn_parse_cap(unsigned loc,void*z){
# 2039
struct _fat_ptr name=*({Cyc_Absyn_type2tvar(z);})->name;
int len;
if((({char*_tmp771=(char*)name.curr;_tmp771 == (char*)(_tag_fat(0,0,0)).curr;})||(len=(int)({Cyc_strlen((struct _fat_ptr)name);}))< 4)||(int)*((const char*)_check_fat_subscript(name,sizeof(char),0))!= (int)'`')goto FAIL;else{
if(((int)*((const char*)_check_fat_subscript(name,sizeof(char),1))== (int)'n' &&(int)*((const char*)_check_fat_subscript(name,sizeof(char),2))== (int)'a')&&(int)*((const char*)_check_fat_subscript(name,sizeof(char),3))== (int)'t'){
# 2044
struct _tuple21 _stmttmp1A=({Cyc_Absyn_parse_int(_fat_ptr_plus(name,sizeof(char),4));});int _tmp3C2;int _tmp3C1;_LL1: _tmp3C1=_stmttmp1A.f1;_tmp3C2=_stmttmp1A.f2;_LL2: {int i=_tmp3C1;int b=_tmp3C2;
if(!b)goto FAIL;else{
return({Cyc_Absyn_new_nat_cap(i,0);});}}}else{
# 2048
if(((int)((const char*)name.curr)[1]== (int)'b' &&(int)*((const char*)_check_fat_subscript(name,sizeof(char),2))== (int)'a')&&(int)*((const char*)_check_fat_subscript(name,sizeof(char),3))== (int)'r'){
# 2050
struct _tuple21 _stmttmp1B=({Cyc_Absyn_parse_int(_fat_ptr_plus(name,sizeof(char),4));});int _tmp3C4;int _tmp3C3;_LL4: _tmp3C3=_stmttmp1B.f1;_tmp3C4=_stmttmp1B.f2;_LL5: {int i=_tmp3C3;int b=_tmp3C4;
if(!b)goto FAIL;else{
return({Cyc_Absyn_new_nat_cap(i,1);});}}}else{
# 2054
if((((int)((const char*)name.curr)[1]== (int)'s' &&(int)*((const char*)_check_fat_subscript(name,sizeof(char),2))== (int)'t')&&(int)*((const char*)_check_fat_subscript(name,sizeof(char),3))== (int)'a')&&(int)*((const char*)_check_fat_subscript(name,sizeof(char),4))== (int)'r'){
# 2056
if(len != 5)goto FAIL;else{
return({Cyc_Absyn_star_cap();});}}else{
# 2059
if(((int)((const char*)name.curr)[1]== (int)'b' &&(int)*((const char*)_check_fat_subscript(name,sizeof(char),2))== (int)'o')&&(int)*((const char*)_check_fat_subscript(name,sizeof(char),3))== (int)'t'){
# 2061
if(len != 4)goto FAIL;else{
return({Cyc_Absyn_bot_cap();});}}}}}}
# 2041
FAIL:
# 2065
({struct Cyc_String_pa_PrintArg_struct _tmp3C7=({struct Cyc_String_pa_PrintArg_struct _tmp643;_tmp643.tag=0U,_tmp643.f1=(struct _fat_ptr)((struct _fat_ptr)name);_tmp643;});void*_tmp3C5[1U];_tmp3C5[0]=& _tmp3C7;({unsigned _tmp773=loc;struct _fat_ptr _tmp772=({const char*_tmp3C6="Could not parse capability %s";_tag_fat(_tmp3C6,sizeof(char),30U);});Cyc_Warn_err(_tmp773,_tmp772,_tag_fat(_tmp3C5,sizeof(void*),1U));});});
return({Cyc_Absyn_star_cap();});}
# 2037
int Cyc_Absyn_equal_cap(void*c1,void*c2){
# 2071
struct _tuple15 _stmttmp1C=({struct _tuple15 _tmp644;_tmp644.f1=c1,_tmp644.f2=c2;_tmp644;});struct _tuple15 _tmp3C9=_stmttmp1C;int _tmp3CD;int _tmp3CC;int _tmp3CB;int _tmp3CA;switch(*((int*)_tmp3C9.f1)){case 0U: if(((struct Cyc_Absyn_Star_Absyn_Cap_struct*)_tmp3C9.f2)->tag == 0U){_LL1: _LL2:
# 2073
 return 1;}else{goto _LL7;}case 1U: if(((struct Cyc_Absyn_Bot_Absyn_Cap_struct*)_tmp3C9.f2)->tag == 1U){_LL3: _LL4:
 return 1;}else{goto _LL7;}default: if(((struct Cyc_Absyn_Nat_Absyn_Cap_struct*)_tmp3C9.f2)->tag == 2U){_LL5: _tmp3CA=((struct Cyc_Absyn_Nat_Absyn_Cap_struct*)_tmp3C9.f1)->f1;_tmp3CB=((struct Cyc_Absyn_Nat_Absyn_Cap_struct*)_tmp3C9.f1)->f2;_tmp3CC=((struct Cyc_Absyn_Nat_Absyn_Cap_struct*)_tmp3C9.f2)->f1;_tmp3CD=((struct Cyc_Absyn_Nat_Absyn_Cap_struct*)_tmp3C9.f2)->f2;_LL6: {int i1=_tmp3CA;int b1=_tmp3CB;int i2=_tmp3CC;int b2=_tmp3CD;
return i1 == i2 && b1 == b2;}}else{_LL7: _LL8:
 return 0;}}_LL0:;}
# 2037
void*Cyc_Absyn_copy_cap(void*c){
# 2082
void*_tmp3CF=c;int _tmp3D1;int _tmp3D0;switch(*((int*)_tmp3CF)){case 0U: _LL1: _LL2:
# 2084
 return({Cyc_Absyn_star_cap();});case 1U: _LL3: _LL4:
 return({Cyc_Absyn_bot_cap();});default: _LL5: _tmp3D0=((struct Cyc_Absyn_Nat_Absyn_Cap_struct*)_tmp3CF)->f1;_tmp3D1=((struct Cyc_Absyn_Nat_Absyn_Cap_struct*)_tmp3CF)->f2;_LL6: {int i=_tmp3D0;int b=_tmp3D1;
return(void*)({struct Cyc_Absyn_Nat_Absyn_Cap_struct*_tmp3D2=_cycalloc(sizeof(*_tmp3D2));_tmp3D2->tag=2U,_tmp3D2->f1=i,_tmp3D2->f2=b;_tmp3D2;});}}_LL0:;}
# 2037
struct _fat_ptr Cyc_Absyn_cap2string(void*c){
# 2092
void*_tmp3D4=c;int _tmp3D5;int _tmp3D6;switch(*((int*)_tmp3D4)){case 0U: _LL1: _LL2:
# 2094
 return({const char*_tmp3D7="`star";_tag_fat(_tmp3D7,sizeof(char),6U);});case 1U: _LL3: _LL4:
 return({const char*_tmp3D8="`bot";_tag_fat(_tmp3D8,sizeof(char),5U);});default: switch(((struct Cyc_Absyn_Nat_Absyn_Cap_struct*)_tmp3D4)->f2){case 0U: _LL5: _tmp3D6=((struct Cyc_Absyn_Nat_Absyn_Cap_struct*)_tmp3D4)->f1;_LL6: {int i=_tmp3D6;
return(struct _fat_ptr)({struct Cyc_Int_pa_PrintArg_struct _tmp3DB=({struct Cyc_Int_pa_PrintArg_struct _tmp645;_tmp645.tag=1U,_tmp645.f1=(unsigned long)i;_tmp645;});void*_tmp3D9[1U];_tmp3D9[0]=& _tmp3DB;({struct _fat_ptr _tmp774=({const char*_tmp3DA="`nat%d";_tag_fat(_tmp3DA,sizeof(char),7U);});Cyc_aprintf(_tmp774,_tag_fat(_tmp3D9,sizeof(void*),1U));});});}case 1U: _LL7: _tmp3D5=((struct Cyc_Absyn_Nat_Absyn_Cap_struct*)_tmp3D4)->f1;_LL8: {int i=_tmp3D5;
return(struct _fat_ptr)({struct Cyc_Int_pa_PrintArg_struct _tmp3DE=({struct Cyc_Int_pa_PrintArg_struct _tmp646;_tmp646.tag=1U,_tmp646.f1=(unsigned long)i;_tmp646;});void*_tmp3DC[1U];_tmp3DC[0]=& _tmp3DE;({struct _fat_ptr _tmp775=({const char*_tmp3DD="`bar%d";_tag_fat(_tmp3DD,sizeof(char),7U);});Cyc_aprintf(_tmp775,_tag_fat(_tmp3DC,sizeof(void*),1U));});});}default: _LL9: _LLA:
({void*_tmp3DF=0U;({int(*_tmp777)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp3E1)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp3E1;});struct _fat_ptr _tmp776=({const char*_tmp3E0="cap2string";_tag_fat(_tmp3E0,sizeof(char),11U);});_tmp777(_tmp776,_tag_fat(_tmp3DF,sizeof(void*),0U));});});}}_LL0:;}
# 2104
int Cyc_Absyn_number_of_caps(){return 2;}struct Cyc_List_List*Cyc_Absyn_caps(void*c1,void*c2){
# 2108
return({struct Cyc_List_List*_tmp3E5=_cycalloc(sizeof(*_tmp3E5));_tmp3E5->hd=c1,({struct Cyc_List_List*_tmp778=({struct Cyc_List_List*_tmp3E4=_cycalloc(sizeof(*_tmp3E4));_tmp3E4->hd=c2,_tmp3E4->tl=0;_tmp3E4;});_tmp3E5->tl=_tmp778;});_tmp3E5;});}
# 2104
void*Cyc_Absyn_rgn_cap(struct Cyc_List_List*c){
# 2113
if(c == 0)({void*_tmp3E7=0U;({int(*_tmp77A)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp3E9)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp3E9;});struct _fat_ptr _tmp779=({const char*_tmp3E8="rgn_cap";_tag_fat(_tmp3E8,sizeof(char),8U);});_tmp77A(_tmp779,_tag_fat(_tmp3E7,sizeof(void*),0U));});});else{
return(void*)c->hd;}}
# 2104
void*Cyc_Absyn_lock_cap(struct Cyc_List_List*c){
# 2119
if(c == 0 || c->tl == 0)({void*_tmp3EB=0U;({int(*_tmp77C)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp3ED)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp3ED;});struct _fat_ptr _tmp77B=({const char*_tmp3EC="lock_cap";_tag_fat(_tmp3EC,sizeof(char),9U);});_tmp77C(_tmp77B,_tag_fat(_tmp3EB,sizeof(void*),0U));});});else{
return(void*)((struct Cyc_List_List*)_check_null(c->tl))->hd;}}
# 2122
static struct _tuple8 Cyc_Absyn_bounded_map(void*(*f)(unsigned,void*),unsigned env,struct Cyc_List_List*t){
# 2125
int cnt=1;int sz=({Cyc_Absyn_number_of_caps();});
if(sz == 0)return({struct _tuple8 _tmp647;_tmp647.f1=0,_tmp647.f2=t;_tmp647;});{struct Cyc_List_List*ret;struct Cyc_List_List*tl;
# 2128
ret=(tl=({struct Cyc_List_List*_tmp3EF=_cycalloc(sizeof(*_tmp3EF));({void*_tmp77D=({f(env,((struct Cyc_List_List*)_check_null(t))->hd);});_tmp3EF->hd=_tmp77D;}),_tmp3EF->tl=0;_tmp3EF;}));
for(t=t->tl;cnt < sz && t != 0;(cnt ++,t=t->tl)){
tl=({struct Cyc_List_List*_tmp77F=({struct Cyc_List_List*_tmp3F0=_cycalloc(sizeof(*_tmp3F0));({void*_tmp77E=({f(env,t->hd);});_tmp3F0->hd=_tmp77E;}),_tmp3F0->tl=0;_tmp3F0;});((struct Cyc_List_List*)_check_null(tl))->tl=_tmp77F;});}
if(cnt < sz)goto FAIL;if(({
int _tmp780=({Cyc_List_length(ret);});_tmp780 != sz;}))({void*_tmp3F1=0U;({struct _fat_ptr _tmp781=({const char*_tmp3F2="bounded_map";_tag_fat(_tmp3F2,sizeof(char),12U);});Cyc_Warn_impos(_tmp781,_tag_fat(_tmp3F1,sizeof(void*),0U));});});
# 2131
return({struct _tuple8 _tmp648;_tmp648.f1=ret,_tmp648.f2=t;_tmp648;});
# 2134
FAIL:
 if(env == (unsigned)0)
({struct Cyc_Int_pa_PrintArg_struct _tmp3F5=({struct Cyc_Int_pa_PrintArg_struct _tmp64A;_tmp64A.tag=1U,_tmp64A.f1=(unsigned long)cnt;_tmp64A;});struct Cyc_Int_pa_PrintArg_struct _tmp3F6=({struct Cyc_Int_pa_PrintArg_struct _tmp649;_tmp649.tag=1U,_tmp649.f1=(unsigned long)sz;_tmp649;});void*_tmp3F3[2U];_tmp3F3[0]=& _tmp3F5,_tmp3F3[1]=& _tmp3F6;({struct _fat_ptr _tmp782=({const char*_tmp3F4="Bad number of capabilities, found %d but need %d";_tag_fat(_tmp3F4,sizeof(char),49U);});Cyc_Warn_impos(_tmp782,_tag_fat(_tmp3F3,sizeof(void*),2U));});});else{
# 2138
({struct Cyc_Int_pa_PrintArg_struct _tmp3F9=({struct Cyc_Int_pa_PrintArg_struct _tmp64C;_tmp64C.tag=1U,_tmp64C.f1=(unsigned long)cnt;_tmp64C;});struct Cyc_Int_pa_PrintArg_struct _tmp3FA=({struct Cyc_Int_pa_PrintArg_struct _tmp64B;_tmp64B.tag=1U,_tmp64B.f1=(unsigned long)sz;_tmp64B;});void*_tmp3F7[2U];_tmp3F7[0]=& _tmp3F9,_tmp3F7[1]=& _tmp3FA;({unsigned _tmp784=env;struct _fat_ptr _tmp783=({const char*_tmp3F8="Could not parse capabilities: Bad number of capabilities, found %d but need %d";_tag_fat(_tmp3F8,sizeof(char),79U);});Cyc_Warn_err(_tmp784,_tmp783,_tag_fat(_tmp3F7,sizeof(void*),2U));});});}
return({struct _tuple8 _tmp64D;_tmp64D.f1=0,_tmp64D.f2=0;_tmp64D;});}}
# 2142
static void Cyc_Absyn_check_number_of_caps(unsigned loc,struct Cyc_List_List*c,int strict){
# 2144
int cnt=({Cyc_List_length(c);});int sz=({Cyc_Absyn_number_of_caps();});
if(!(!strict && cnt >= sz || strict && cnt == sz)){
# 2147
if(loc != (unsigned)0)
({struct Cyc_Int_pa_PrintArg_struct _tmp3FE=({struct Cyc_Int_pa_PrintArg_struct _tmp64F;_tmp64F.tag=1U,_tmp64F.f1=(unsigned long)cnt;_tmp64F;});struct Cyc_Int_pa_PrintArg_struct _tmp3FF=({struct Cyc_Int_pa_PrintArg_struct _tmp64E;_tmp64E.tag=1U,_tmp64E.f1=(unsigned long)sz;_tmp64E;});void*_tmp3FC[2U];_tmp3FC[0]=& _tmp3FE,_tmp3FC[1]=& _tmp3FF;({unsigned _tmp786=loc;struct _fat_ptr _tmp785=({const char*_tmp3FD="Bad number of capabilities, found %d but need %d.";_tag_fat(_tmp3FD,sizeof(char),50U);});Cyc_Warn_err(_tmp786,_tmp785,_tag_fat(_tmp3FC,sizeof(void*),2U));});});else{
# 2150
({struct Cyc_Int_pa_PrintArg_struct _tmp402=({struct Cyc_Int_pa_PrintArg_struct _tmp651;_tmp651.tag=1U,_tmp651.f1=(unsigned long)cnt;_tmp651;});struct Cyc_Int_pa_PrintArg_struct _tmp403=({struct Cyc_Int_pa_PrintArg_struct _tmp650;_tmp650.tag=1U,_tmp650.f1=(unsigned long)sz;_tmp650;});void*_tmp400[2U];_tmp400[0]=& _tmp402,_tmp400[1]=& _tmp403;({struct _fat_ptr _tmp787=({const char*_tmp401="Bad number of capabilities, found %d but need %d.";_tag_fat(_tmp401,sizeof(char),50U);});Cyc_Warn_impos(_tmp787,_tag_fat(_tmp400,sizeof(void*),2U));});});}}}
# 2154
static void*Cyc_Absyn_type2cap_wrap(unsigned dummy,void*t){return({Cyc_Absyn_type2cap(t);});}
# 2156
static struct _tuple8 Cyc_Absyn_type2caps(struct Cyc_List_List*t1){
# 2158
if(t1 == 0){
# 2160
if(({Cyc_Absyn_number_of_caps();})== 0)return({struct _tuple8 _tmp652;_tmp652.f1=0,_tmp652.f2=0;_tmp652;});else{
({struct Cyc_String_pa_PrintArg_struct _tmp409=({struct Cyc_String_pa_PrintArg_struct _tmp653;_tmp653.tag=0U,({struct _fat_ptr _tmp788=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_list2string(t1,Cyc_Absynpp_typ2string);}));_tmp653.f1=_tmp788;});_tmp653;});void*_tmp406[1U];_tmp406[0]=& _tmp409;({int(*_tmp78A)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp408)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp408;});struct _fat_ptr _tmp789=({const char*_tmp407="type2caps (1): \"%s\"";_tag_fat(_tmp407,sizeof(char),20U);});_tmp78A(_tmp789,_tag_fat(_tmp406,sizeof(void*),1U));});});}}else{
# 2164
void*_stmttmp1D=(void*)t1->hd;void*_tmp40A=_stmttmp1D;struct Cyc_List_List*_tmp40B;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp40A)->tag == 0U){if(((struct Cyc_Absyn_JoinCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp40A)->f1)->tag == 11U){_LL1: _tmp40B=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp40A)->f2;_LL2: {struct Cyc_List_List*t=_tmp40B;
# 2167
({Cyc_Absyn_check_number_of_caps(0U,t,1);});
return({struct _tuple8 _tmp654;({struct Cyc_List_List*_tmp78C=({({struct Cyc_List_List*(*_tmp78B)(void*(*f)(unsigned,void*),unsigned env,struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp40C)(void*(*f)(unsigned,void*),unsigned env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(void*(*f)(unsigned,void*),unsigned env,struct Cyc_List_List*x))Cyc_List_map_c;_tmp40C;});_tmp78B(Cyc_Absyn_type2cap_wrap,0U,t);});});_tmp654.f1=_tmp78C;}),_tmp654.f2=t1->tl;_tmp654;});}}else{goto _LL3;}}else{_LL3: _LL4:
({struct Cyc_String_pa_PrintArg_struct _tmp410=({struct Cyc_String_pa_PrintArg_struct _tmp655;_tmp655.tag=0U,({struct _fat_ptr _tmp78D=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_list2string(t1,Cyc_Absynpp_typ2string);}));_tmp655.f1=_tmp78D;});_tmp655;});void*_tmp40D[1U];_tmp40D[0]=& _tmp410;({int(*_tmp78F)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp40F)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp40F;});struct _fat_ptr _tmp78E=({const char*_tmp40E="type2caps (2): \"%s\"";_tag_fat(_tmp40E,sizeof(char),20U);});_tmp78F(_tmp78E,_tag_fat(_tmp40D,sizeof(void*),1U));});});}_LL0:;}}
# 2156
void*Cyc_Absyn_caps2type(struct Cyc_List_List*t){
# 2175
({Cyc_Absyn_check_number_of_caps(0U,t,1);});
return({void*(*_tmp412)(struct Cyc_List_List*ts)=Cyc_Absyn_join_eff;struct Cyc_List_List*_tmp413=({Cyc_List_map(Cyc_Absyn_cap2type,t);});_tmp412(_tmp413);});}
# 2179
static struct _tuple8 Cyc_Absyn_parse_caps(unsigned loc,struct Cyc_List_List*t){
# 2181
({Cyc_Absyn_check_number_of_caps(loc,t,0);});
return({Cyc_Absyn_bounded_map(Cyc_Absyn_parse_cap,loc,t);});}
# 2179
int Cyc_Absyn_equal_caps(struct Cyc_List_List*c1,struct Cyc_List_List*c2){
# 2187
({Cyc_Absyn_check_number_of_caps(0U,c1,1);});({Cyc_Absyn_check_number_of_caps(0U,c2,1);});
for(0;c1 != 0 && c2 != 0;(c1=c1->tl,c2=c2->tl)){
if(!({Cyc_Absyn_equal_cap((void*)c1->hd,(void*)c2->hd);}))return 0;}
# 2188
return 1;}
# 2179
struct Cyc_List_List*Cyc_Absyn_copy_caps(struct Cyc_List_List*c){
# 2195
({Cyc_Absyn_check_number_of_caps(0U,c,1);});
return({Cyc_List_map(Cyc_Absyn_copy_cap,c);});}
# 2179
struct _fat_ptr Cyc_Absyn_caps2string(struct Cyc_List_List*c){
# 2201
return({Cyc_Absyn_list2string(c,Cyc_Absyn_cap2string);});}
# 2204
struct _tuple12 Cyc_Absyn_caps2tup(struct Cyc_List_List*c){
# 2206
void*rg=({Cyc_Absyn_rgn_cap(c);});
return({struct _tuple12 _tmp656;({int _tmp792=({Cyc_Absyn_get_nat_cap(rg);});_tmp656.f1=_tmp792;}),({int _tmp791=({int(*_tmp419)(void*c)=Cyc_Absyn_get_nat_cap;void*_tmp41A=({Cyc_Absyn_lock_cap(c);});_tmp419(_tmp41A);});_tmp656.f2=_tmp791;}),({int _tmp790=({Cyc_Absyn_is_nat_bar_cap(rg);});_tmp656.f3=_tmp790;});_tmp656;});}
# 2204
struct Cyc_List_List*Cyc_Absyn_tup2caps(int a,int b,int c){
# 2212
return({struct Cyc_List_List*(*_tmp41C)(void*c1,void*c2)=Cyc_Absyn_caps;void*_tmp41D=({Cyc_Absyn_new_nat_cap(a,c);});void*_tmp41E=({Cyc_Absyn_new_nat_cap(b,0);});_tmp41C(_tmp41D,_tmp41E);});}
# 2217
void Cyc_Absyn_updatecaps_rgneffect(struct Cyc_Absyn_RgnEffect*r,struct Cyc_List_List*c){
struct _tuple12 _stmttmp1E=({Cyc_Absyn_caps2tup(c);});_LL1: _LL2:
 r->caps=c;}
# 2217
struct Cyc_Absyn_RgnEffect*Cyc_Absyn_new_rgneffect(void*t1,struct Cyc_List_List*c,void*t2){
# 2224
return({struct Cyc_Absyn_RgnEffect*_tmp421=_cycalloc(sizeof(*_tmp421));_tmp421->name=t1,_tmp421->caps=c,_tmp421->parent=t2;_tmp421;});}
# 2227
static struct Cyc_Absyn_RgnEffect*Cyc_Absyn_parse_rgneffect(unsigned loc,void*t){
# 2229
void*_tmp423=t;struct Cyc_List_List*_tmp424;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp423)->tag == 0U){if(((struct Cyc_Absyn_JoinCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp423)->f1)->tag == 11U){_LL1: _tmp424=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp423)->f2;_LL2: {struct Cyc_List_List*ts1=_tmp424;
# 2232
if(ts1 == 0)
({void*_tmp425=0U;({int(*_tmp794)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp427)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp427;});struct _fat_ptr _tmp793=({const char*_tmp426="Compiler error Absyn:: parse_rgneffect no effects";_tag_fat(_tmp426,sizeof(char),50U);});_tmp794(_tmp793,_tag_fat(_tmp425,sizeof(void*),0U));});});else{
# 2236
struct Cyc_List_List*ts0=({Cyc_List_rev(ts1);});
struct _tuple8 _stmttmp1F=({Cyc_Absyn_parse_caps(loc,((struct Cyc_List_List*)_check_null(ts0))->tl);});struct Cyc_List_List*_tmp429;struct Cyc_List_List*_tmp428;_LL6: _tmp428=_stmttmp1F.f1;_tmp429=_stmttmp1F.f2;_LL7: {struct Cyc_List_List*c=_tmp428;struct Cyc_List_List*rem=_tmp429;
return({Cyc_Absyn_new_rgneffect((void*)ts0->hd,c,rem == 0?0:(void*)rem->hd);});}}}}else{goto _LL3;}}else{_LL3: _LL4:
# 2240
({void*_tmp42A=0U;({int(*_tmp796)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp42C)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp42C;});struct _fat_ptr _tmp795=({const char*_tmp42B="Expected AppType";_tag_fat(_tmp42B,sizeof(char),17U);});_tmp796(_tmp795,_tag_fat(_tmp42A,sizeof(void*),0U));});});}_LL0:;}
# 2227
struct Cyc_List_List*Cyc_Absyn_parse_rgneffects(unsigned loc,void*t){
# 2246
struct Cyc_List_List*ret=0;
if(t == 0)return 0;{void*_tmp42E=t;struct Cyc_List_List*_tmp42F;if(_tmp42E != 0){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp42E)->tag == 0U){if(((struct Cyc_Absyn_JoinCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp42E)->f1)->tag == 11U){_LL1: _tmp42F=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp42E)->f2;_LL2: {struct Cyc_List_List*lexp=_tmp42F;
# 2251
for(0;lexp != 0;lexp=lexp->tl){
ret=({struct Cyc_List_List*_tmp430=_cycalloc(sizeof(*_tmp430));({struct Cyc_Absyn_RgnEffect*_tmp797=({Cyc_Absyn_parse_rgneffect(loc,(void*)lexp->hd);});_tmp430->hd=_tmp797;}),_tmp430->tl=ret;_tmp430;});}
goto _LL0;}}else{goto _LL3;}}else{goto _LL3;}}else{_LL3: _LL4:
# 2255
({void*_tmp431=0U;({unsigned _tmp799=loc;struct _fat_ptr _tmp798=({const char*_tmp432="Expected application type";_tag_fat(_tmp432,sizeof(char),26U);});Cyc_Warn_err(_tmp799,_tmp798,_tag_fat(_tmp431,sizeof(void*),0U));});});
return 0;}_LL0:;}
# 2258
return ret;}
# 2227
struct Cyc_List_List*Cyc_Absyn_rgneffect_rnames(struct Cyc_List_List*l,int dom){
# 2263
struct Cyc_List_List*ret=0;
for(0;l != 0;l=l->tl){
# 2266
void*p=((struct Cyc_Absyn_RgnEffect*)l->hd)->parent;
if(p == 0 || dom == 1)
ret=({struct Cyc_List_List*_tmp434=_cycalloc(sizeof(*_tmp434));_tmp434->hd=((struct Cyc_Absyn_RgnEffect*)l->hd)->name,_tmp434->tl=ret;_tmp434;});else{
if(dom == 0)
ret=({struct Cyc_List_List*_tmp436=_cycalloc(sizeof(*_tmp436));_tmp436->hd=((struct Cyc_Absyn_RgnEffect*)l->hd)->name,({struct Cyc_List_List*_tmp79A=({struct Cyc_List_List*_tmp435=_cycalloc(sizeof(*_tmp435));_tmp435->hd=p,_tmp435->tl=ret;_tmp435;});_tmp436->tl=_tmp79A;});_tmp436;});}}
# 2272
return ret;}
# 2227
struct Cyc_Absyn_RgnEffect*Cyc_Absyn_type2rgneffect(void*t){
# 2277
void*_tmp438=t;struct Cyc_List_List*_tmp439;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp438)->tag == 0U){if(((struct Cyc_Absyn_CAccessCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp438)->f1)->tag == 10U){_LL1: _tmp439=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp438)->f2;if(_tmp439 != 0){_LL2: {struct Cyc_List_List*l=_tmp439;
# 2280
struct _tuple8 _stmttmp20=({Cyc_Absyn_type2caps(l->tl);});struct Cyc_List_List*_tmp43B;struct Cyc_List_List*_tmp43A;_LL6: _tmp43A=_stmttmp20.f1;_tmp43B=_stmttmp20.f2;_LL7: {struct Cyc_List_List*c=_tmp43A;struct Cyc_List_List*r=_tmp43B;
return({Cyc_Absyn_new_rgneffect((void*)l->hd,c,r == 0?0:(void*)r->hd);});}}}else{goto _LL3;}}else{goto _LL3;}}else{_LL3: _LL4:
# 2283
({void*_tmp43C=0U;({int(*_tmp79C)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp43E)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp43E;});struct _fat_ptr _tmp79B=({const char*_tmp43D="type2rgneffect: Expected CAccessCon with some caps";_tag_fat(_tmp43D,sizeof(char),51U);});_tmp79C(_tmp79B,_tag_fat(_tmp43C,sizeof(void*),0U));});});}_LL0:;}
# 2227
void*Cyc_Absyn_rgneffect2type(struct Cyc_Absyn_RgnEffect*re){
# 2290
void*p=re->parent;
return(void*)({struct Cyc_Absyn_AppType_Absyn_Type_struct*_tmp443=_cycalloc(sizeof(*_tmp443));_tmp443->tag=0U,_tmp443->f1=(void*)& Cyc_Absyn_CAccessCon_val,({
struct Cyc_List_List*_tmp7A0=({struct Cyc_List_List*_tmp442=_cycalloc(sizeof(*_tmp442));_tmp442->hd=re->name,({
struct Cyc_List_List*_tmp79F=({struct Cyc_List_List*_tmp441=_cycalloc(sizeof(*_tmp441));({void*_tmp79E=({Cyc_Absyn_caps2type(re->caps);});_tmp441->hd=_tmp79E;}),
p == 0?_tmp441->tl=0:({
struct Cyc_List_List*_tmp79D=({struct Cyc_List_List*_tmp440=_cycalloc(sizeof(*_tmp440));_tmp440->hd=p,_tmp440->tl=0;_tmp440;});_tmp441->tl=_tmp79D;});_tmp441;});
# 2293
_tmp442->tl=_tmp79F;});_tmp442;});
# 2292
_tmp443->f2=_tmp7A0;});_tmp443;});}
# 2227
int Cyc_Absyn_rgneffect_tvar_cmp(struct Cyc_Absyn_Tvar*x,struct Cyc_Absyn_Tvar*y){
# 2299
return({Cyc_Absyn_naive_tvar_cmp2(x,y);});}
# 2227
int Cyc_Absyn_rgneffect_cmp_name_tv(struct Cyc_Absyn_Tvar*x,struct Cyc_Absyn_RgnEffect*r2){
# 2304
void*name2=r2->name;
{void*_tmp446=name2;struct Cyc_Absyn_Tvar*_tmp447;void*_tmp448;switch(*((int*)_tmp446)){case 1U: _LL1: _tmp448=(void*)((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp446)->f2;if(_tmp448 != 0){_LL2: {void*t=_tmp448;
# 2308
struct Cyc_Absyn_Tvar*vt_out=0;
{void*_tmp449=t;struct Cyc_Absyn_Tvar*_tmp44A;if(_tmp449 != 0){if(((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp449)->tag == 2U){_LL8: _tmp44A=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp449)->f1;_LL9: {struct Cyc_Absyn_Tvar*vt=_tmp44A;
# 2311
vt_out=vt;goto _LL7;}}else{goto _LLA;}}else{_LLA: _LLB:
 goto _LL7;}_LL7:;}
# 2314
if(vt_out != 0){_tmp447=vt_out;goto _LL4;}goto _LL0;}}else{goto _LL5;}case 2U: _LL3: _tmp447=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp446)->f1;_LL4: {struct Cyc_Absyn_Tvar*vt=_tmp447;
# 2317
if(({Cyc_Absyn_naive_tvar_cmp2(vt,x);}))return 1;goto _LL0;}default: _LL5: _LL6:
# 2319
({void*_tmp44B=0U;({int(*_tmp7A4)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp44F)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp44F;});struct _fat_ptr _tmp7A3=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp44E=({struct Cyc_String_pa_PrintArg_struct _tmp657;_tmp657.tag=0U,({
struct _fat_ptr _tmp7A1=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_typcon2string(name2);}));_tmp657.f1=_tmp7A1;});_tmp657;});void*_tmp44C[1U];_tmp44C[0]=& _tmp44E;({struct _fat_ptr _tmp7A2=({const char*_tmp44D="rgneffect_compare_name_tv: %s";_tag_fat(_tmp44D,sizeof(char),30U);});Cyc_aprintf(_tmp7A2,_tag_fat(_tmp44C,sizeof(void*),1U));});});_tmp7A4(_tmp7A3,_tag_fat(_tmp44B,sizeof(void*),0U));});});}_LL0:;}
# 2322
return 0;}
# 2227
int Cyc_Absyn_rgneffect_cmp_name(struct Cyc_Absyn_RgnEffect*r1,struct Cyc_Absyn_RgnEffect*r2){
# 2327
return({int(*_tmp451)(struct Cyc_Absyn_Tvar*x,struct Cyc_Absyn_RgnEffect*r2)=Cyc_Absyn_rgneffect_cmp_name_tv;struct Cyc_Absyn_Tvar*_tmp452=({Cyc_Absyn_type2tvar(r1->name);});struct Cyc_Absyn_RgnEffect*_tmp453=r2;_tmp451(_tmp452,_tmp453);});}
# 2330
struct Cyc_List_List*Cyc_Absyn_find_rgneffect_tail(struct Cyc_Absyn_Tvar*x,struct Cyc_List_List*l){
# 2332
for(0;l != 0;l=l->tl){
if(({Cyc_Absyn_rgneffect_cmp_name_tv(x,(struct Cyc_Absyn_RgnEffect*)l->hd);}))return l;}
# 2332
return 0;}
# 2338
struct Cyc_Absyn_RgnEffect*Cyc_Absyn_find_rgneffect(struct Cyc_Absyn_Tvar*x,struct Cyc_List_List*l){
# 2340
struct Cyc_List_List*ret=({Cyc_Absyn_find_rgneffect_tail(x,l);});
if(ret == 0)return 0;else{
return(struct Cyc_Absyn_RgnEffect*)ret->hd;}}
# 2338
int Cyc_Absyn_subset_rgneffect(struct Cyc_List_List*l,struct Cyc_List_List*l1){
# 2347
if(l == 0)return 1;for(0;l != 0;l=l->tl){
# 2350
void*_stmttmp21=((struct Cyc_Absyn_RgnEffect*)l->hd)->name;void*_tmp457=_stmttmp21;struct Cyc_Absyn_Tvar*_tmp458;if(((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp457)->tag == 2U){_LL1: _tmp458=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp457)->f1;_LL2: {struct Cyc_Absyn_Tvar*vt=_tmp458;
# 2353
if(({Cyc_Absyn_find_rgneffect(vt,l1);})== 0)
# 2355
return 0;
# 2353
goto _LL0;}}else{_LL3: _LL4:
# 2359
 return 0;}_LL0:;}
# 2362
return 1;}
# 2338
int Cyc_Absyn_equal_rgneffect(struct Cyc_Absyn_RgnEffect*r1,struct Cyc_Absyn_RgnEffect*r2){
# 2368
void*p1=r1->parent;
void*p2=r2->parent;
if(!({Cyc_Absyn_xorptr(p1,p2);}))return 0;{void*n1=r1->name;
# 2372
void*n2=r2->name;
# 2374
if(p1 == 0)p1=Cyc_Absyn_heap_rgn_type;if(p2 == 0)
p2=Cyc_Absyn_heap_rgn_type;{
# 2374
int bn=({struct Cyc_Absyn_Tvar*(*_tmp45D)(struct Cyc_Absyn_Tvar*t1,struct Cyc_Absyn_Tvar*t2)=Cyc_Absyn_naive_tvar_cmp;struct Cyc_Absyn_Tvar*_tmp45E=({Cyc_Absyn_type2tvar(n1);});struct Cyc_Absyn_Tvar*_tmp45F=({Cyc_Absyn_type2tvar(n2);});_tmp45D(_tmp45E,_tmp45F);})!= 0;
# 2377
int bp=p1 == 0 ||(({Cyc_Absyn_is_heap_rgn(p1);})&&({Cyc_Absyn_is_heap_rgn(p2);})||({struct Cyc_Absyn_Tvar*(*_tmp45A)(struct Cyc_Absyn_Tvar*t1,struct Cyc_Absyn_Tvar*t2)=Cyc_Absyn_naive_tvar_cmp;struct Cyc_Absyn_Tvar*_tmp45B=({Cyc_Absyn_type2tvar(p1);});struct Cyc_Absyn_Tvar*_tmp45C=({Cyc_Absyn_type2tvar(p2);});_tmp45A(_tmp45B,_tmp45C);})!= 0);
# 2379
int bc=({Cyc_Absyn_equal_caps(r1->caps,r2->caps);});
# 2386
return(bn && bp)&& bc;}}}
# 2338
struct Cyc_List_List*Cyc_Absyn_filter_rgneffects(unsigned loc,void*t){
# 2409 "absyn.cyc"
struct Cyc_List_List*ret=0;
{void*_tmp461=t;struct Cyc_List_List*_tmp462;struct Cyc_List_List*_tmp463;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp461)->tag == 0U)switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp461)->f1)){case 11U: _LL1: _tmp463=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp461)->f2;_LL2: {struct Cyc_List_List*lexp=_tmp463;
# 2413
for(0;lexp != 0;lexp=lexp->tl){
# 2415
struct Cyc_List_List*x=({Cyc_Absyn_filter_rgneffects(loc,(void*)lexp->hd);});
ret=({Cyc_List_append(ret,x);});}
# 2418
goto _LL0;}case 10U: _LL3: _tmp462=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp461)->f2;_LL4: {struct Cyc_List_List*r2=_tmp462;
# 2420
return({struct Cyc_List_List*_tmp464=_cycalloc(sizeof(*_tmp464));({struct Cyc_Absyn_RgnEffect*_tmp7A5=({Cyc_Absyn_type2rgneffect(t);});_tmp464->hd=_tmp7A5;}),_tmp464->tl=0;_tmp464;});}default: goto _LL5;}else{_LL5: _LL6:
 goto _LL0;}_LL0:;}
# 2423
return ret;}
# 2426
static int Cyc_Absyn_unify_rgneffect(struct Cyc_Absyn_RgnEffect*r1,struct Cyc_Absyn_RgnEffect*r2,void(*uit)(void*,void*)){
void*p1=r1->parent;
void*p2=r2->parent;
if(!({Cyc_Absyn_xorptr(p1,p2);}))return 0;{void*n1=r1->name;
# 2431
void*n2=r2->name;
({uit(n1,n2);});
if(p1 != 0 &&({Cyc_Absyn_is_heap_rgn(p1);})){
if(!({Cyc_Absyn_is_heap_rgn((void*)_check_null(p2));}))return 0;}else{
# 2436
if(p1 != 0)({uit(p1,(void*)_check_null(p2));});}
# 2433
return({Cyc_Absyn_equal_caps(r1->caps,r2->caps);});}}
# 2426
int Cyc_Absyn_unify_rgneffects(struct Cyc_List_List*l1,struct Cyc_List_List*l2,void(*uit)(void*,void*)){
# 2443
if(({int _tmp7A6=({Cyc_List_length(l1);});_tmp7A6 != ({Cyc_List_length(l2);});}))return 0;{struct Cyc_List_List*iter=l1;
# 2446
struct Cyc_List_List*iter1=l2;
for(0;iter != 0;(iter=iter->tl,iter1=iter1->tl)){
# 2449
if(!({Cyc_Absyn_unify_rgneffect((struct Cyc_Absyn_RgnEffect*)iter->hd,(struct Cyc_Absyn_RgnEffect*)((struct Cyc_List_List*)_check_null(iter1))->hd,uit);}))return 0;}
# 2454
return 1;}}
# 2426
int Cyc_Absyn_equal_rgneffects(struct Cyc_List_List*l1,struct Cyc_List_List*l2){
# 2460
struct Cyc_List_List*iter=l1;
for(0;iter != 0;iter=iter->tl){
# 2463
struct Cyc_Absyn_RgnEffect*z=({struct Cyc_Absyn_RgnEffect*(*_tmp468)(struct Cyc_Absyn_Tvar*x,struct Cyc_List_List*l)=Cyc_Absyn_find_rgneffect;struct Cyc_Absyn_Tvar*_tmp469=({Cyc_Absyn_type2tvar(((struct Cyc_Absyn_RgnEffect*)iter->hd)->name);});struct Cyc_List_List*_tmp46A=l2;_tmp468(_tmp469,_tmp46A);});
if(z == 0 || !({Cyc_Absyn_equal_rgneffect((struct Cyc_Absyn_RgnEffect*)iter->hd,z);}))return 0;}
# 2466
return({int _tmp7A7=({Cyc_List_length(l1);});_tmp7A7 == ({Cyc_List_length(l2);});});}
# 2470
struct _tuple8 Cyc_Absyn_fntyp2ioeffects(void*t){
# 2472
void*_tmp46C=t;struct Cyc_List_List*_tmp46E;struct Cyc_List_List*_tmp46D;if(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp46C)->tag == 5U){_LL1: _tmp46D=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp46C)->f1).ieffect;_tmp46E=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp46C)->f1).oeffect;_LL2: {struct Cyc_List_List*i=_tmp46D;struct Cyc_List_List*o=_tmp46E;
# 2474
return({struct _tuple8 _tmp658;_tmp658.f1=i,_tmp658.f2=o;_tmp658;});}}else{_LL3: _LL4:
({void*_tmp46F=0U;({int(*_tmp7A9)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp471)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp471;});struct _fat_ptr _tmp7A8=({const char*_tmp470="fntyp2ioeffects";_tag_fat(_tmp470,sizeof(char),16U);});_tmp7A9(_tmp7A8,_tag_fat(_tmp46F,sizeof(void*),0U));});});}_LL0:;}
# 2479
static void*Cyc_Absyn_resolve_rgneffect_tvar(unsigned loc,void*p,struct Cyc_List_List*tvars){
# 2482
if(({Cyc_Absyn_is_heap_rgn(p);}))return p;{struct Cyc_Absyn_Tvar*fnd=({struct Cyc_Absyn_Tvar*(*_tmp477)(struct Cyc_Absyn_Tvar*(*pred)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_Absyn_Tvar*env,struct Cyc_List_List*x)=({
struct Cyc_Absyn_Tvar*(*_tmp478)(struct Cyc_Absyn_Tvar*(*pred)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_Absyn_Tvar*env,struct Cyc_List_List*x)=(struct Cyc_Absyn_Tvar*(*)(struct Cyc_Absyn_Tvar*(*pred)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_Absyn_Tvar*env,struct Cyc_List_List*x))Cyc_List_find_c;_tmp478;});struct Cyc_Absyn_Tvar*(*_tmp479)(struct Cyc_Absyn_Tvar*t1,struct Cyc_Absyn_Tvar*t2)=Cyc_Absyn_naive_tvar_cmp;struct Cyc_Absyn_Tvar*_tmp47A=({Cyc_Absyn_type2tvar(p);});struct Cyc_List_List*_tmp47B=tvars;_tmp477(_tmp479,_tmp47A,_tmp47B);});
if(fnd == 0){
# 2486
({void*_tmp473=0U;({unsigned _tmp7AD=loc;struct _fat_ptr _tmp7AC=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp476=({struct Cyc_String_pa_PrintArg_struct _tmp659;_tmp659.tag=0U,({
# 2488
struct _fat_ptr _tmp7AA=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(p);}));_tmp659.f1=_tmp7AA;});_tmp659;});void*_tmp474[1U];_tmp474[0]=& _tmp476;({struct _fat_ptr _tmp7AB=({const char*_tmp475="Unbound type variable %s.";_tag_fat(_tmp475,sizeof(char),26U);});Cyc_aprintf(_tmp7AB,_tag_fat(_tmp474,sizeof(void*),1U));});});Cyc_Warn_err(_tmp7AD,_tmp7AC,_tag_fat(_tmp473,sizeof(void*),0U));});});
return p;}else{
# 2493
void*ret=({Cyc_Absyn_var_type(fnd);});
if(!({Cyc_Absyn_is_xrgn(ret);}))
({void*_tmp7AE=({Cyc_Tcutil_kind_to_bound(& Cyc_Tcutil_xrk);});fnd->kind=_tmp7AE;});
# 2494
return ret;}}}
# 2501
static void Cyc_Absyn_resolve_tvars(unsigned loc,struct Cyc_List_List*in,struct Cyc_List_List*tvars,struct _fat_ptr msg){
# 2506
struct Cyc_List_List*iter=in;
for(0;iter != 0;iter=iter->tl){
# 2509
struct Cyc_Absyn_RgnEffect*rf=(struct Cyc_Absyn_RgnEffect*)iter->hd;
({void*_tmp7AF=({Cyc_Absyn_resolve_rgneffect_tvar(loc,rf->name,tvars);});rf->name=_tmp7AF;});{
void*p=rf->parent;
if(p != 0)
({void*_tmp7B0=({Cyc_Absyn_resolve_rgneffect_tvar(loc,p,tvars);});rf->parent=_tmp7B0;});}}}
# 2517
static struct _fat_ptr Cyc_Absyn_tvar2string(struct Cyc_Absyn_Tvar*t){return*t->name;}
# 2520
void Cyc_Absyn_resolve_effects(unsigned loc,struct Cyc_List_List*in,struct Cyc_List_List*out){
# 2524
struct Cyc_List_List*tvars=({struct Cyc_List_List*(*_tmp481)(struct Cyc_Absyn_Tvar*(*f)(void*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp482)(struct Cyc_Absyn_Tvar*(*f)(void*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_Tvar*(*f)(void*),struct Cyc_List_List*x))Cyc_List_map;_tmp482;});struct Cyc_Absyn_Tvar*(*_tmp483)(void*t)=Cyc_Absyn_type2tvar;struct Cyc_List_List*_tmp484=({Cyc_Absyn_rgneffect_rnames(in,1);});_tmp481(_tmp483,_tmp484);});
# 2529
({({unsigned _tmp7B3=loc;struct Cyc_List_List*_tmp7B2=in;struct Cyc_List_List*_tmp7B1=tvars;Cyc_Absyn_resolve_tvars(_tmp7B3,_tmp7B2,_tmp7B1,({const char*_tmp47F="@ieffect";_tag_fat(_tmp47F,sizeof(char),9U);}));});});
({({unsigned _tmp7B6=loc;struct Cyc_List_List*_tmp7B5=out;struct Cyc_List_List*_tmp7B4=tvars;Cyc_Absyn_resolve_tvars(_tmp7B6,_tmp7B5,_tmp7B4,({const char*_tmp480="@oeffect";_tag_fat(_tmp480,sizeof(char),9U);}));});});}
# 2520
struct _fat_ptr Cyc_Absyn_effect2string(struct Cyc_List_List*f){
# 2535
return({struct _fat_ptr(*_tmp486)(void*)=Cyc_Absynpp_typ2string;void*_tmp487=({void*(*_tmp488)(struct Cyc_List_List*ts)=Cyc_Absyn_join_eff;struct Cyc_List_List*_tmp489=({({struct Cyc_List_List*(*_tmp7B7)(void*(*f)(struct Cyc_Absyn_RgnEffect*),struct Cyc_List_List*x)=({
struct Cyc_List_List*(*_tmp48A)(void*(*f)(struct Cyc_Absyn_RgnEffect*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(void*(*f)(struct Cyc_Absyn_RgnEffect*),struct Cyc_List_List*x))Cyc_List_map;_tmp48A;});_tmp7B7(Cyc_Absyn_rgneffect2type,f);});});_tmp488(_tmp489);});_tmp486(_tmp487);});}
# 2520
struct _fat_ptr Cyc_Absyn_rgneffect2string(struct Cyc_Absyn_RgnEffect*r){
# 2542
void*p=r->parent;
return(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp48E=({struct Cyc_String_pa_PrintArg_struct _tmp65D;_tmp65D.tag=0U,({struct _fat_ptr _tmp7B8=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(r->name);}));_tmp65D.f1=_tmp7B8;});_tmp65D;});struct Cyc_String_pa_PrintArg_struct _tmp48F=({struct Cyc_String_pa_PrintArg_struct _tmp65C;_tmp65C.tag=0U,({
struct _fat_ptr _tmp7B9=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_caps2string(r->caps);}));_tmp65C.f1=_tmp7B9;});_tmp65C;});struct Cyc_String_pa_PrintArg_struct _tmp490=({struct Cyc_String_pa_PrintArg_struct _tmp65A;_tmp65A.tag=0U,({
struct _fat_ptr _tmp7BC=(struct _fat_ptr)(p == 0?({const char*_tmp491="";_tag_fat(_tmp491,sizeof(char),1U);}):(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp494=({struct Cyc_String_pa_PrintArg_struct _tmp65B;_tmp65B.tag=0U,({
struct _fat_ptr _tmp7BA=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(p);}));_tmp65B.f1=_tmp7BA;});_tmp65B;});void*_tmp492[1U];_tmp492[0]=& _tmp494;({struct _fat_ptr _tmp7BB=({const char*_tmp493=",%s";_tag_fat(_tmp493,sizeof(char),4U);});Cyc_aprintf(_tmp7BB,_tag_fat(_tmp492,sizeof(void*),1U));});}));
# 2545
_tmp65A.f1=_tmp7BC;});_tmp65A;});void*_tmp48C[3U];_tmp48C[0]=& _tmp48E,_tmp48C[1]=& _tmp48F,_tmp48C[2]=& _tmp490;({struct _fat_ptr _tmp7BD=({const char*_tmp48D="{%s,%s%s}";_tag_fat(_tmp48D,sizeof(char),10U);});Cyc_aprintf(_tmp7BD,_tag_fat(_tmp48C,sizeof(void*),3U));});});}
# 2520
struct Cyc_Absyn_RgnEffect*Cyc_Absyn_copy_rgneffect(struct Cyc_Absyn_RgnEffect*eff){
# 2551
void*p=eff->parent;
return({struct Cyc_Absyn_RgnEffect*(*_tmp496)(void*t1,struct Cyc_List_List*c,void*t2)=Cyc_Absyn_new_rgneffect;void*_tmp497=({Cyc_Tcutil_copy_type(eff->name);});struct Cyc_List_List*_tmp498=({Cyc_Absyn_copy_caps(eff->caps);});void*_tmp499=
# 2554
p == 0?0:({Cyc_Tcutil_copy_type(p);});_tmp496(_tmp497,_tmp498,_tmp499);});}
# 2520
struct Cyc_List_List*Cyc_Absyn_copy_effect(struct Cyc_List_List*f){
# 2560
return({({struct Cyc_List_List*(*_tmp7BE)(struct Cyc_Absyn_RgnEffect*(*f)(struct Cyc_Absyn_RgnEffect*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp49B)(struct Cyc_Absyn_RgnEffect*(*f)(struct Cyc_Absyn_RgnEffect*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_RgnEffect*(*f)(struct Cyc_Absyn_RgnEffect*),struct Cyc_List_List*x))Cyc_List_map;_tmp49B;});_tmp7BE(Cyc_Absyn_copy_rgneffect,f);});});}
# 2520
void*Cyc_Absyn_rgneffect_name(struct Cyc_Absyn_RgnEffect*r){
# 2565
return r->name;}
# 2520
void*Cyc_Absyn_rgneffect_parent(struct Cyc_Absyn_RgnEffect*r){
# 2570
return r->parent;}
# 2520
struct Cyc_List_List*Cyc_Absyn_rgneffect_caps(struct Cyc_Absyn_RgnEffect*r){
# 2575
return r->caps;}
# 2520
struct Cyc_Absyn_RgnEffect*Cyc_Absyn_rsubstitute_rgneffect(struct _RegionHandle*rgn,struct Cyc_List_List*inst,struct Cyc_Absyn_RgnEffect*eff){
# 2584
void*par=eff->parent;
if(par != 0)par=({Cyc_Tcutil_rsubstitute(rgn,inst,par);});{void*newname=({Cyc_Tcutil_rsubstitute(rgn,inst,eff->name);});
# 2587
if(!({Cyc_Absyn_is_xrgn(newname);})||
(par != 0 && !({Cyc_Absyn_is_heap_rgn(par);}))&& !({Cyc_Absyn_is_xrgn(par);}))
({void*_tmp4A0=0U;({int(*_tmp7C0)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp4A2)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp4A2;});struct _fat_ptr _tmp7BF=({const char*_tmp4A1="rsubstitute_effect: Expected xrgn kinds";_tag_fat(_tmp4A1,sizeof(char),40U);});_tmp7C0(_tmp7BF,_tag_fat(_tmp4A0,sizeof(void*),0U));});});
# 2587
return({struct Cyc_Absyn_RgnEffect*(*_tmp4A3)(void*t1,struct Cyc_List_List*c,void*t2)=Cyc_Absyn_new_rgneffect;void*_tmp4A4=newname;struct Cyc_List_List*_tmp4A5=({Cyc_Absyn_copy_caps(eff->caps);});void*_tmp4A6=par;_tmp4A3(_tmp4A4,_tmp4A5,_tmp4A6);});}}struct Cyc_Absyn_wrapper_struct{struct _RegionHandle*rgn;struct Cyc_List_List*inst;};
# 2599
static struct Cyc_Absyn_RgnEffect*Cyc_Absyn_rsubstitute_rgneffect_wrapper(struct Cyc_Absyn_wrapper_struct*p,struct Cyc_Absyn_RgnEffect*r){
# 2602
return({Cyc_Absyn_rsubstitute_rgneffect(p->rgn,p->inst,r);});}
# 2599
struct Cyc_List_List*Cyc_Absyn_rsubstitute_effect(struct _RegionHandle*rgn,struct Cyc_List_List*inst,struct Cyc_List_List*l){
# 2609
struct Cyc_Absyn_wrapper_struct w=({struct Cyc_Absyn_wrapper_struct _tmp65E;_tmp65E.rgn=rgn,_tmp65E.inst=inst;_tmp65E;});
return({({struct Cyc_List_List*(*_tmp7C1)(struct Cyc_Absyn_RgnEffect*(*f)(struct Cyc_Absyn_wrapper_struct*,struct Cyc_Absyn_RgnEffect*),struct Cyc_Absyn_wrapper_struct*env,struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp4A9)(struct Cyc_Absyn_RgnEffect*(*f)(struct Cyc_Absyn_wrapper_struct*,struct Cyc_Absyn_RgnEffect*),struct Cyc_Absyn_wrapper_struct*env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_RgnEffect*(*f)(struct Cyc_Absyn_wrapper_struct*,struct Cyc_Absyn_RgnEffect*),struct Cyc_Absyn_wrapper_struct*env,struct Cyc_List_List*x))Cyc_List_map_c;_tmp4A9;});_tmp7C1(Cyc_Absyn_rsubstitute_rgneffect_wrapper,& w,l);});});}
# 2614
static struct Cyc_Absyn_Tvar*Cyc_Absyn_mapf(struct Cyc_Absyn_RgnEffect*r){
# 2616
void*_stmttmp22=r->name;void*_tmp4AB=_stmttmp22;struct Cyc_Absyn_Tvar*_tmp4AC;if(((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp4AB)->tag == 2U){_LL1: _tmp4AC=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp4AB)->f1;_LL2: {struct Cyc_Absyn_Tvar*vt=_tmp4AC;
# 2618
return vt;}}else{_LL3: _LL4:
({void*_tmp4AD=0U;({int(*_tmp7C3)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp4AF)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp4AF;});struct _fat_ptr _tmp7C2=({const char*_tmp4AE="mapeff unexpected type";_tag_fat(_tmp4AE,sizeof(char),23U);});_tmp7C3(_tmp7C2,_tag_fat(_tmp4AD,sizeof(void*),0U));});});}_LL0:;}struct Cyc_Absyn_Env{struct Cyc_Absyn_Tvar*r;struct Cyc_List_List*l;struct Cyc_List_List*seen;};
# 2632
static int Cyc_Absyn_check_path_loop(struct Cyc_Absyn_Env*e){
# 2634
struct Cyc_Absyn_Tvar*tv=e->r;
struct Cyc_Absyn_RgnEffect*_stmttmp23=({Cyc_Absyn_find_rgneffect(tv,e->l);});struct Cyc_Absyn_RgnEffect*_tmp4B1=_stmttmp23;void*_tmp4B2;if(_tmp4B1 == 0){_LL1: _LL2:
# 2637
 return 1;}else{_LL3: _tmp4B2=_tmp4B1->parent;_LL4: {void*parent=_tmp4B2;
# 2639
if(({({int(*_tmp7C5)(int(*pred)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_Absyn_Tvar*env,struct Cyc_List_List*x)=({int(*_tmp4B3)(int(*pred)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_Absyn_Tvar*env,struct Cyc_List_List*x)=(int(*)(int(*pred)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_Absyn_Tvar*env,struct Cyc_List_List*x))Cyc_List_exists_c;_tmp4B3;});struct Cyc_Absyn_Tvar*_tmp7C4=tv;_tmp7C5(Cyc_Absyn_naive_tvar_cmp2,_tmp7C4,e->seen);});}))return 0;if(
parent == 0 || parent == Cyc_Absyn_heap_rgn_type)return 1;
# 2639
({struct Cyc_List_List*_tmp7C6=({struct Cyc_List_List*_tmp4B4=_cycalloc(sizeof(*_tmp4B4));
# 2641
_tmp4B4->hd=tv,_tmp4B4->tl=e->seen;_tmp4B4;});
# 2639
e->seen=_tmp7C6;});
# 2642
({struct Cyc_Absyn_Tvar*_tmp7C7=({Cyc_Absyn_type2tvar(parent);});e->r=_tmp7C7;});
return({Cyc_Absyn_check_path_loop(e);});}}_LL0:;}
# 2648
static int Cyc_Absyn_check_tree_loop(struct Cyc_List_List*l){
# 2650
if(l == 0)return 1;{struct Cyc_Absyn_Env*env=({struct Cyc_Absyn_Env*_tmp4B6=_cycalloc(sizeof(*_tmp4B6));
({struct Cyc_Absyn_Tvar*_tmp7C8=({Cyc_Absyn_type2tvar(((struct Cyc_Absyn_RgnEffect*)l->hd)->name);});_tmp4B6->r=_tmp7C8;}),_tmp4B6->l=l,_tmp4B6->seen=0;_tmp4B6;});
for(0;l != 0;l=l->tl){
# 2654
({struct Cyc_Absyn_Tvar*_tmp7C9=({Cyc_Absyn_type2tvar(((struct Cyc_Absyn_RgnEffect*)l->hd)->name);});env->r=_tmp7C9;});
env->seen=0;
if(!({Cyc_Absyn_check_path_loop(env);}))return 0;}
# 2658
return 1;}}
# 2662
static void Cyc_Absyn_check_rgn_effect(unsigned loc,struct Cyc_Absyn_RgnEffect*r){
# 2665
{
void*newname=r->name;
void*par=r->parent;
if(!({Cyc_Absyn_is_xrgn(newname);})||
(par != 0 && !({Cyc_Absyn_is_heap_rgn(par);}))&& !({Cyc_Absyn_is_xrgn(par);}))
({void*_tmp4B8=0U;({int(*_tmp7CB)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp4BA)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp4BA;});struct _fat_ptr _tmp7CA=({const char*_tmp4B9="check_rgn_effect: Expected xrgn kinds";_tag_fat(_tmp4B9,sizeof(char),38U);});_tmp7CB(_tmp7CA,_tag_fat(_tmp4B8,sizeof(void*),0U));});});}
# 2673
if(({Cyc_Absyn_is_heap_rgn(r->name);}))
({struct Cyc_String_pa_PrintArg_struct _tmp4BD=({struct Cyc_String_pa_PrintArg_struct _tmp65F;_tmp65F.tag=0U,({
struct _fat_ptr _tmp7CC=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_rgneffect2string(r);}));_tmp65F.f1=_tmp7CC;});_tmp65F;});void*_tmp4BB[1U];_tmp4BB[0]=& _tmp4BD;({unsigned _tmp7CE=loc;struct _fat_ptr _tmp7CD=({const char*_tmp4BC="Heap region found at the first element of region effect%s";_tag_fat(_tmp4BC,sizeof(char),58U);});Cyc_Warn_err(_tmp7CE,_tmp7CD,_tag_fat(_tmp4BB,sizeof(void*),1U));});});{
# 2673
void*rgn=({Cyc_Absyn_rgn_cap(r->caps);});
# 2677
void*lock=({Cyc_Absyn_lock_cap(r->caps);});
if(({Cyc_Absyn_is_bot_cap(rgn);}))
({struct Cyc_String_pa_PrintArg_struct _tmp4C0=({struct Cyc_String_pa_PrintArg_struct _tmp660;_tmp660.tag=0U,({
struct _fat_ptr _tmp7CF=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_rgneffect2string(r);}));_tmp660.f1=_tmp7CF;});_tmp660;});void*_tmp4BE[1U];_tmp4BE[0]=& _tmp4C0;({unsigned _tmp7D1=loc;struct _fat_ptr _tmp7D0=({const char*_tmp4BF="Region capability of region effect %s cannot be `bot";_tag_fat(_tmp4BF,sizeof(char),53U);});Cyc_Warn_err(_tmp7D1,_tmp7D0,_tag_fat(_tmp4BE,sizeof(void*),1U));});});
# 2678
if(({Cyc_Absyn_is_bot_cap(lock);}))
# 2683
({struct Cyc_String_pa_PrintArg_struct _tmp4C3=({struct Cyc_String_pa_PrintArg_struct _tmp661;_tmp661.tag=0U,({
struct _fat_ptr _tmp7D2=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_rgneffect2string(r);}));_tmp661.f1=_tmp7D2;});_tmp661;});void*_tmp4C1[1U];_tmp4C1[0]=& _tmp4C3;({unsigned _tmp7D4=loc;struct _fat_ptr _tmp7D3=({const char*_tmp4C2="Lock capability of region effect %s cannot be `bot (for now at least!)";_tag_fat(_tmp4C2,sizeof(char),71U);});Cyc_Warn_err(_tmp7D4,_tmp7D3,_tag_fat(_tmp4C1,sizeof(void*),1U));});});
# 2678
if(
# 2686
({Cyc_Absyn_is_star_cap(lock);})||({Cyc_Absyn_is_star_cap(rgn);}))
({struct Cyc_String_pa_PrintArg_struct _tmp4C6=({struct Cyc_String_pa_PrintArg_struct _tmp662;_tmp662.tag=0U,({
struct _fat_ptr _tmp7D5=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_rgneffect2string(r);}));_tmp662.f1=_tmp7D5;});_tmp662;});void*_tmp4C4[1U];_tmp4C4[0]=& _tmp4C6;({unsigned _tmp7D7=loc;struct _fat_ptr _tmp7D6=({const char*_tmp4C5="Star capabilities are disallowed (for now at least!): %s";_tag_fat(_tmp4C5,sizeof(char),57U);});Cyc_Warn_err(_tmp7D7,_tmp7D6,_tag_fat(_tmp4C4,sizeof(void*),1U));});});
# 2678
if(({Cyc_Absyn_is_nat_bar_cap(lock);}))
# 2693
({struct Cyc_String_pa_PrintArg_struct _tmp4C9=({struct Cyc_String_pa_PrintArg_struct _tmp663;_tmp663.tag=0U,({
struct _fat_ptr _tmp7D8=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_rgneffect2string(r);}));_tmp663.f1=_tmp7D8;});_tmp663;});void*_tmp4C7[1U];_tmp4C7[0]=& _tmp4C9;({unsigned _tmp7DA=loc;struct _fat_ptr _tmp7D9=({const char*_tmp4C8="Only region capability of effect %s can be annotated with `bar";_tag_fat(_tmp4C8,sizeof(char),63U);});Cyc_Warn_err(_tmp7DA,_tmp7D9,_tag_fat(_tmp4C7,sizeof(void*),1U));});});}}
# 2662
int Cyc_Absyn_is_aliasable_rgneffect(struct Cyc_Absyn_RgnEffect*r){
# 2702
void*rc=({Cyc_Absyn_rgn_cap(r->caps);});
return({Cyc_Absyn_is_nat_bar_cap(rc);});}
# 2707
static void*Cyc_Absyn_i_check_valid_effect_impl(unsigned loc,void*(*cb)(unsigned,void*,void*,struct Cyc_Absyn_Kind*,void*,int,int),void*te,void*new_cvtenv,struct Cyc_List_List*ieff,struct _fat_ptr msg){
# 2719
({void(*_tmp4CC)(unsigned,struct Cyc_List_List*)=Cyc_Tcutil_check_unique_tvars;unsigned _tmp4CD=loc;struct Cyc_List_List*_tmp4CE=({({struct Cyc_List_List*(*_tmp7DB)(struct Cyc_Absyn_Tvar*(*f)(struct Cyc_Absyn_RgnEffect*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp4CF)(struct Cyc_Absyn_Tvar*(*f)(struct Cyc_Absyn_RgnEffect*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_Tvar*(*f)(struct Cyc_Absyn_RgnEffect*),struct Cyc_List_List*x))Cyc_List_map;_tmp4CF;});_tmp7DB(Cyc_Absyn_mapf,ieff);});});_tmp4CC(_tmp4CD,_tmp4CE);});
if(!({Cyc_Absyn_check_tree_loop(ieff);}))
({struct Cyc_String_pa_PrintArg_struct _tmp4D2=({struct Cyc_String_pa_PrintArg_struct _tmp664;_tmp664.tag=0U,_tmp664.f1=(struct _fat_ptr)((struct _fat_ptr)msg);_tmp664;});void*_tmp4D0[1U];_tmp4D0[0]=& _tmp4D2;({unsigned _tmp7DD=loc;struct _fat_ptr _tmp7DC=({const char*_tmp4D1="Cannot form cycles in %s";_tag_fat(_tmp4D1,sizeof(char),25U);});Cyc_Warn_err(_tmp7DD,_tmp7DC,_tag_fat(_tmp4D0,sizeof(void*),1U));});});
# 2720
({({void(*_tmp7DF)(void(*f)(unsigned,struct Cyc_Absyn_RgnEffect*),unsigned env,struct Cyc_List_List*x)=({void(*_tmp4D3)(void(*f)(unsigned,struct Cyc_Absyn_RgnEffect*),unsigned env,struct Cyc_List_List*x)=(void(*)(void(*f)(unsigned,struct Cyc_Absyn_RgnEffect*),unsigned env,struct Cyc_List_List*x))Cyc_List_iter_c;_tmp4D3;});unsigned _tmp7DE=loc;_tmp7DF(Cyc_Absyn_check_rgn_effect,_tmp7DE,ieff);});});{
# 2723
struct Cyc_List_List*itf=({Cyc_Absyn_rgneffect_rnames(ieff,1);});
for(0;itf != 0;itf=itf->tl){
new_cvtenv=({cb(loc,te,new_cvtenv,& Cyc_Tcutil_xrk,(void*)itf->hd,1,1);});}
# 2727
return new_cvtenv;}}
# 2730
static int Cyc_Absyn_not_same_parents(struct Cyc_List_List*out,struct Cyc_Absyn_RgnEffect*r){
# 2732
struct Cyc_Absyn_RgnEffect*rout=({struct Cyc_Absyn_RgnEffect*(*_tmp4D5)(struct Cyc_Absyn_Tvar*x,struct Cyc_List_List*l)=Cyc_Absyn_find_rgneffect;struct Cyc_Absyn_Tvar*_tmp4D6=({Cyc_Absyn_type2tvar(r->name);});struct Cyc_List_List*_tmp4D7=out;_tmp4D5(_tmp4D6,_tmp4D7);});
if(rout == 0)return 0;{void*p=r->parent;
# 2735
void*pout=rout->parent;
return !({Cyc_Absyn_xorptr(p,pout);})||
(p != 0 && pout != 0)&&({Cyc_Tcutil_typecmp(p,pout);})!= 0;}}
# 2741
void*Cyc_Absyn_i_check_valid_effect(unsigned loc,void*(*cb)(unsigned,void*,void*,struct Cyc_Absyn_Kind*,void*,int,int),void*te,void*new_cvtenv,struct Cyc_List_List*ieff,struct Cyc_List_List*oeff){
# 2752
if(!({Cyc_Absyn_subset_rgneffect(oeff,ieff);}))
({void*_tmp4D9=0U;({unsigned _tmp7E1=loc;struct _fat_ptr _tmp7E0=({const char*_tmp4DA="The output effect type variables outnumber the input effect type variables";_tag_fat(_tmp4DA,sizeof(char),75U);});Cyc_Tcutil_terr(_tmp7E1,_tmp7E0,_tag_fat(_tmp4D9,sizeof(void*),0U));});});{
# 2752
struct Cyc_List_List*lst=({({struct Cyc_List_List*(*_tmp7E3)(int(*f)(struct Cyc_List_List*,struct Cyc_Absyn_RgnEffect*),struct Cyc_List_List*env,struct Cyc_List_List*x)=({
# 2754
struct Cyc_List_List*(*_tmp4E8)(int(*f)(struct Cyc_List_List*,struct Cyc_Absyn_RgnEffect*),struct Cyc_List_List*env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(int(*f)(struct Cyc_List_List*,struct Cyc_Absyn_RgnEffect*),struct Cyc_List_List*env,struct Cyc_List_List*x))Cyc_List_filter_c;_tmp4E8;});struct Cyc_List_List*_tmp7E2=oeff;_tmp7E3(Cyc_Absyn_not_same_parents,_tmp7E2,ieff);});});
if(lst != 0)
# 2757
({struct Cyc_String_pa_PrintArg_struct _tmp4DD=({struct Cyc_String_pa_PrintArg_struct _tmp666;_tmp666.tag=0U,({
struct _fat_ptr _tmp7E4=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_effect2string(lst);}));_tmp666.f1=_tmp7E4;});_tmp666;});struct Cyc_String_pa_PrintArg_struct _tmp4DE=({struct Cyc_String_pa_PrintArg_struct _tmp665;_tmp665.tag=0U,({struct _fat_ptr _tmp7E5=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_effect2string(oeff);}));_tmp665.f1=_tmp7E5;});_tmp665;});void*_tmp4DB[2U];_tmp4DB[0]=& _tmp4DD,_tmp4DB[1]=& _tmp4DE;({unsigned _tmp7E7=loc;struct _fat_ptr _tmp7E6=({const char*_tmp4DC="The following region input effects' parent %s do not match the  parent declared at the output effect %s";_tag_fat(_tmp4DC,sizeof(char),104U);});Cyc_Tcutil_terr(_tmp7E7,_tmp7E6,_tag_fat(_tmp4DB,sizeof(void*),2U));});});
# 2755
return({void*(*_tmp4DF)(unsigned loc,void*(*cb)(unsigned,void*,void*,struct Cyc_Absyn_Kind*,void*,int,int),void*te,void*new_cvtenv,struct Cyc_List_List*ieff,struct _fat_ptr msg)=Cyc_Absyn_i_check_valid_effect_impl;unsigned _tmp4E0=loc;void*(*_tmp4E1)(unsigned,void*,void*,struct Cyc_Absyn_Kind*,void*,int,int)=cb;void*_tmp4E2=te;void*_tmp4E3=({({unsigned _tmp7EC=loc;void*(*_tmp7EB)(unsigned,void*,void*,struct Cyc_Absyn_Kind*,void*,int,int)=cb;void*_tmp7EA=te;void*_tmp7E9=new_cvtenv;struct Cyc_List_List*_tmp7E8=ieff;Cyc_Absyn_i_check_valid_effect_impl(_tmp7EC,_tmp7EB,_tmp7EA,_tmp7E9,_tmp7E8,({const char*_tmp4E4="@ieffect";_tag_fat(_tmp4E4,sizeof(char),9U);}));});});struct Cyc_List_List*_tmp4E5=oeff;struct _fat_ptr _tmp4E6=({const char*_tmp4E7="@oeffect";_tag_fat(_tmp4E7,sizeof(char),9U);});_tmp4DF(_tmp4E0,_tmp4E1,_tmp4E2,_tmp4E3,_tmp4E5,_tmp4E6);});}}static char _tmp4EA[14U]="dummy_dtf_var";
# 2741
static struct _fat_ptr Cyc_Absyn_dummy_dtf_var={_tmp4EA,_tmp4EA,_tmp4EA + 14U};
# 2807 "absyn.cyc"
static struct _tuple0 Cyc_Absyn_dummy_qvar={{.Loc_n={4,0}},& Cyc_Absyn_dummy_dtf_var};
static struct Cyc_Absyn_Datatypefield Cyc_Absyn_dummy_dtf_struct={& Cyc_Absyn_dummy_qvar,0,0U,Cyc_Absyn_Static};
static struct Cyc_List_List Cyc_Absyn_dummy_dtf_list={(void*)& Cyc_Absyn_dummy_dtf_struct,0};static char _tmp4EB[13U]="__!default__";
# 2811
static struct _fat_ptr Cyc_Absyn_default_dtf_var={_tmp4EB,_tmp4EB,_tmp4EB + 13U};static char _tmp4EC[9U]="__!any__";
static struct _fat_ptr Cyc_Absyn_any_dtf_var={_tmp4EC,_tmp4EC,_tmp4EC + 9U};
static struct _tuple0 Cyc_Absyn_default_qvar={{.Loc_n={4,0}},& Cyc_Absyn_default_dtf_var};
static struct _tuple0 Cyc_Absyn_throws_any_qvar={{.Loc_n={4,0}},& Cyc_Absyn_any_dtf_var};
static struct Cyc_Absyn_Datatypefield Cyc_Absyn_any_dtf_struct={& Cyc_Absyn_throws_any_qvar,0,0U,Cyc_Absyn_Static};
static struct Cyc_List_List Cyc_Absyn_any_dtf_list={(void*)& Cyc_Absyn_any_dtf_struct,0};
static struct Cyc_List_List Cyc_Absyn_catch_all={(void*)& Cyc_Absyn_throws_any_qvar,0};
# 2820
struct _tuple0*Cyc_Absyn_any_qvar(){return& Cyc_Absyn_throws_any_qvar;}int Cyc_Absyn_is_any_qvar(struct _tuple0*q){
# 2822
return& Cyc_Absyn_throws_any_qvar == q;}
# 2820
struct _tuple0*Cyc_Absyn_default_case_qvar(){
# 2825
return& Cyc_Absyn_default_qvar;}
# 2820
int Cyc_Absyn_is_default_case_qvar(struct _tuple0*q){
# 2830
return q == & Cyc_Absyn_default_qvar;}
# 2820
struct Cyc_List_List*Cyc_Absyn_throwsany(){
# 2835
return& Cyc_Absyn_any_dtf_list;}
# 2820
int Cyc_Absyn_is_throwsany(struct Cyc_List_List*t){
# 2840
return t == 0 ||& Cyc_Absyn_any_dtf_list == t;}
# 2820
struct Cyc_List_List*Cyc_Absyn_nothrow(){
# 2845
return& Cyc_Absyn_dummy_dtf_list;}
# 2820
int Cyc_Absyn_is_nothrow(struct Cyc_List_List*t){
# 2850
return& Cyc_Absyn_dummy_dtf_list == t;}
# 2853
void Cyc_Absyn_resolve_throws(unsigned loc,struct Cyc_List_List*throws,void(*check)(unsigned loc,void*,struct _tuple0*),void*p){
# 2856
if(({Cyc_Absyn_is_nothrow(throws);}))return;if(({Cyc_Absyn_is_throwsany(throws);}))
return;
# 2856
for(0;throws != 0;throws=throws->tl){
# 2860
if(((struct Cyc_Absyn_Datatypefield*)throws->hd)->typs != 0)
# 2862
({void*_tmp4F5=0U;({unsigned _tmp7EE=((struct Cyc_Absyn_Datatypefield*)throws->hd)->loc;struct _fat_ptr _tmp7ED=({const char*_tmp4F6="Expected exception names but found E(...)";_tag_fat(_tmp4F6,sizeof(char),42U);});Cyc_Warn_err(_tmp7EE,_tmp7ED,_tag_fat(_tmp4F5,sizeof(void*),0U));});});
# 2860
({check(((struct Cyc_Absyn_Datatypefield*)throws->hd)->loc,p,((struct Cyc_Absyn_Datatypefield*)throws->hd)->name);});}}
# 2853
int Cyc_Absyn_exists_throws(struct Cyc_List_List*throws,struct _tuple0*q){
# 2872
if(throws == 0)return 1;if(({Cyc_Absyn_is_nothrow(throws);}))
return 0;
# 2872
if(({Cyc_Absyn_is_throwsany(throws);}))
# 2874
return 1;
# 2872
for(0;throws != 0;throws=throws->tl){
# 2877
if(({Cyc_Absyn_my_qvar_cmp(((struct Cyc_Absyn_Datatypefield*)throws->hd)->name,q);})== 0)return 1;}
# 2879
return 0;}
# 2882
static struct _fat_ptr Cyc_Absyn_d2s(struct Cyc_Absyn_Datatypefield*f){
return({Cyc_Absynpp_qvar2string(f->name);});}
# 2882
struct _fat_ptr Cyc_Absyn_throws_qvar2string(struct _tuple0*q){
# 2888
if(({struct _tuple0*_tmp7EF=q;_tmp7EF == ({Cyc_Absyn_any_qvar();});}))return({const char*_tmp4FA="<any>";_tag_fat(_tmp4FA,sizeof(char),6U);});else{
return({Cyc_Absynpp_qvar2string(q);});}}
# 2882
struct _fat_ptr Cyc_Absyn_throws2string(struct Cyc_List_List*t){
# 2894
if(({Cyc_Absyn_is_throwsany(t);}))return({const char*_tmp4FC="<throws any>";_tag_fat(_tmp4FC,sizeof(char),13U);});else{
if(({Cyc_Absyn_is_nothrow(t);}))return({const char*_tmp4FD="<no throws>";_tag_fat(_tmp4FD,sizeof(char),12U);});else{
return(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp500=({struct Cyc_String_pa_PrintArg_struct _tmp667;_tmp667.tag=0U,({struct _fat_ptr _tmp7F1=(struct _fat_ptr)((struct _fat_ptr)({({struct _fat_ptr(*_tmp7F0)(struct Cyc_List_List*l,struct _fat_ptr(*cb)(struct Cyc_Absyn_Datatypefield*))=({struct _fat_ptr(*_tmp501)(struct Cyc_List_List*l,struct _fat_ptr(*cb)(struct Cyc_Absyn_Datatypefield*))=(struct _fat_ptr(*)(struct Cyc_List_List*l,struct _fat_ptr(*cb)(struct Cyc_Absyn_Datatypefield*)))Cyc_Absyn_list2string;_tmp501;});_tmp7F0(t,Cyc_Absyn_d2s);});}));_tmp667.f1=_tmp7F1;});_tmp667;});void*_tmp4FE[1U];_tmp4FE[0]=& _tmp500;({struct _fat_ptr _tmp7F2=({const char*_tmp4FF="{%s}";_tag_fat(_tmp4FF,sizeof(char),5U);});Cyc_aprintf(_tmp7F2,_tag_fat(_tmp4FE,sizeof(void*),1U));});});}}}
# 2882
struct _tuple0*Cyc_Absyn_throws_hd2qvar(struct Cyc_List_List*t){
# 2901
return((struct Cyc_Absyn_Datatypefield*)((struct Cyc_List_List*)_check_null(t))->hd)->name;}
# 2882
int Cyc_Absyn_equals_throws(struct Cyc_List_List*t1,struct Cyc_List_List*t2){
# 2906
if(t1 == 0)t1=({Cyc_Absyn_throwsany();});if(t2 == 0)
t2=({Cyc_Absyn_throwsany();});
# 2906
if(({Cyc_Absyn_is_nothrow(t1);}))
# 2908
return({Cyc_Absyn_is_nothrow(t2);});
# 2906
if(({Cyc_Absyn_is_throwsany(t1);}))
# 2909
return({Cyc_Absyn_is_throwsany(t2);});
# 2906
if(({
# 2910
int _tmp7F3=({Cyc_List_length(t1);});_tmp7F3 != ({Cyc_List_length(t2);});}))return 0;
# 2906
if(t1 == 0)
# 2911
return 1;
# 2906
for(0;t1 != 0;t1=t1->tl){
# 2913
if(!({Cyc_Absyn_exists_throws(t2,((struct Cyc_Absyn_Datatypefield*)t1->hd)->name);}))return 0;}
# 2906
return 1;}
# 2882
int Cyc_Absyn_subset_throws(struct Cyc_List_List*t1,struct Cyc_List_List*t2){
# 2928 "absyn.cyc"
if(({Cyc_Absyn_is_throwsany(t2);}))return 1;if(({Cyc_Absyn_is_nothrow(t1);}))
return 1;
# 2928
if(({Cyc_Absyn_is_nothrow(t2);}))
# 2930
return 0;
# 2928
if(({Cyc_Absyn_is_throwsany(t1);}))
# 2931
return 0;
# 2928
for(0;t1 != 0;t1=t1->tl){
# 2933
if(!({Cyc_Absyn_exists_throws(t2,((struct Cyc_Absyn_Datatypefield*)t1->hd)->name);}))return 0;}
# 2928
return 1;}static char _tmp506[15U]="Null_Exception";
# 2882 "absyn.cyc"
static struct _fat_ptr Cyc_Absyn_null_exn_var_str={_tmp506,_tmp506,_tmp506 + 15U};static char _tmp507[16U]="Match_Exception";
# 2940 "absyn.cyc"
static struct _fat_ptr Cyc_Absyn_match_exn_var_str={_tmp507,_tmp507,_tmp507 + 16U};static char _tmp508[10U]="Bad_alloc";
static struct _fat_ptr Cyc_Absyn_badalloc_exn_var_str={_tmp508,_tmp508,_tmp508 + 10U};static char _tmp509[13U]="Array_bounds";
static struct _fat_ptr Cyc_Absyn_arraybounds_exn_var_str={_tmp509,_tmp509,_tmp509 + 13U};static char _tmp50A[4U]="exn";
static struct _fat_ptr Cyc_Absyn_exn_var_str={_tmp50A,_tmp50A,_tmp50A + 4U};
# 2945
static struct _tuple0 Cyc_Absyn_null_exn_var={{.Loc_n={4,0}},& Cyc_Absyn_null_exn_var_str};
static struct _tuple0 Cyc_Absyn_match_exn_var={{.Loc_n={4,0}},& Cyc_Absyn_match_exn_var_str};
static struct _tuple0 Cyc_Absyn_badalloc_exn_var={{.Loc_n={4,0}},& Cyc_Absyn_badalloc_exn_var_str};
static struct _tuple0 Cyc_Absyn_arraybounds_exn_var={{.Loc_n={4,0}},& Cyc_Absyn_arraybounds_exn_var_str};
static struct _tuple0 Cyc_Absyn_exn_var={{.Loc_n={4,0}},& Cyc_Absyn_exn_var_str};
# 2952
struct _tuple0*Cyc_Absyn_get_qvar(enum Cyc_Absyn_DefExn de){
# 2954
enum Cyc_Absyn_DefExn _tmp50B=de;switch(_tmp50B){case Cyc_Absyn_De_NullCheck: _LL1: _LL2:
# 2956
 return& Cyc_Absyn_null_exn_var;case Cyc_Absyn_De_BoundsCheck: _LL3: _LL4:
 return& Cyc_Absyn_arraybounds_exn_var;case Cyc_Absyn_De_PatternCheck: _LL5: _LL6:
 return& Cyc_Absyn_match_exn_var;case Cyc_Absyn_De_AllocCheck: _LL7: _LL8:
 return& Cyc_Absyn_badalloc_exn_var;default: _LL9: _LLA:
# 2961
({struct Cyc_String_pa_PrintArg_struct _tmp50E=({struct Cyc_String_pa_PrintArg_struct _tmp668;_tmp668.tag=0U,({struct _fat_ptr _tmp7F4=({const char*_tmp50F="get_qvar";_tag_fat(_tmp50F,sizeof(char),9U);});_tmp668.f1=_tmp7F4;});_tmp668;});void*_tmp50C[1U];_tmp50C[0]=& _tmp50E;({int(*_tmp7F6)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp50D)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp50D;});struct _fat_ptr _tmp7F5=_tag_fat(0,0,0);_tmp7F6(_tmp7F5,_tag_fat(_tmp50C,sizeof(void*),1U));});});}_LL0:;}
# 2952
int Cyc_Absyn_is_exn_stmt(struct Cyc_Absyn_Stmt*s){
# 2968
void*_stmttmp24=s->r;void*_tmp511=_stmttmp24;struct _tuple0*_tmp512;if(((struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct*)_tmp511)->tag == 1U){if(((struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct*)((struct Cyc_Absyn_Exp*)((struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct*)_tmp511)->f1)->r)->tag == 11U){if(((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)((struct Cyc_Absyn_Exp*)((struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct*)((struct Cyc_Absyn_Exp*)((struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct*)_tmp511)->f1)->r)->f1)->r)->tag == 1U){if(((struct Cyc_Absyn_Local_b_Absyn_Binding_struct*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)((struct Cyc_Absyn_Exp*)((struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct*)((struct Cyc_Absyn_Exp*)((struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct*)_tmp511)->f1)->r)->f1)->r)->f1)->tag == 4U){_LL1: _tmp512=(((struct Cyc_Absyn_Local_b_Absyn_Binding_struct*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)(((struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct*)(((struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct*)_tmp511)->f1)->r)->f1)->r)->f1)->f1)->name;_LL2: {struct _tuple0*q=_tmp512;
# 2974
return({Cyc_Absyn_my_qvar_cmp(q,& Cyc_Absyn_exn_var);})== 0;}}else{goto _LL3;}}else{goto _LL3;}}else{goto _LL3;}}else{_LL3: _LL4:
 return 0;}_LL0:;}struct _tuple22{int f1;void*f2;};
# 3008 "absyn.cyc"
static struct _tuple8 Cyc_Absyn_analyze_dec_tree(struct _tuple0*n,void*d,int depth){
# 3014
struct Cyc_List_List*in=0;struct Cyc_List_List*out=0;
# 3016
{void*_tmp514=d;void*_tmp517;struct Cyc_List_List*_tmp516;struct Cyc_List_List*_tmp515;struct Cyc_Tcpat_Rhs*_tmp518;switch(*((int*)_tmp514)){case 1U: _LL1: _tmp518=((struct Cyc_Tcpat_Success_Tcpat_Decision_struct*)_tmp514)->f1;_LL2: {struct Cyc_Tcpat_Rhs*rhs=_tmp518;
# 3019
if(depth > 1 &&({Cyc_Absyn_is_exn_stmt(rhs->rhs);}))
# 3021
out=({struct Cyc_List_List*_tmp519=_cycalloc(sizeof(*_tmp519));_tmp519->hd=n,_tmp519->tl=out;_tmp519;});
# 3019
goto _LL0;}case 0U: _LL3: _LL4:
# 3025
 out=({struct Cyc_List_List*_tmp51A=_cycalloc(sizeof(*_tmp51A));_tmp51A->hd=& Cyc_Absyn_match_exn_var,_tmp51A->tl=out;_tmp51A;});
goto _LL0;default: _LL5: _tmp515=((struct Cyc_Tcpat_SwitchDec_Tcpat_Decision_struct*)_tmp514)->f1;_tmp516=((struct Cyc_Tcpat_SwitchDec_Tcpat_Decision_struct*)_tmp514)->f2;_tmp517=(void*)((struct Cyc_Tcpat_SwitchDec_Tcpat_Decision_struct*)_tmp514)->f3;_LL6: {struct Cyc_List_List*a=_tmp515;struct Cyc_List_List*cases=_tmp516;void*def=_tmp517;
# 3028
for(0;cases != 0;cases=cases->tl){
# 3030
struct _tuple15 _stmttmp25=*((struct _tuple15*)cases->hd);void*_tmp51C;void*_tmp51B;_LL8: _tmp51B=_stmttmp25.f1;_tmp51C=_stmttmp25.f2;_LL9: {void*pt=_tmp51B;void*d=_tmp51C;
struct _tuple22 _stmttmp26=({struct _tuple22 _tmp669;_tmp669.f1=depth,_tmp669.f2=pt;_tmp669;});struct _tuple22 _tmp51D=_stmttmp26;struct Cyc_Absyn_Datatypefield*_tmp51E;if(_tmp51D.f1 == 0){if(((struct Cyc_Tcpat_EqExtensibleDatatype_Tcpat_PatTest_struct*)_tmp51D.f2)->tag == 9U){_LLB: _tmp51E=((struct Cyc_Tcpat_EqExtensibleDatatype_Tcpat_PatTest_struct*)_tmp51D.f2)->f2;_LLC: {struct Cyc_Absyn_Datatypefield*f=_tmp51E;
# 3034
struct _tuple8 _stmttmp27=({Cyc_Absyn_analyze_dec_tree(f->name,d,depth + 1);});struct Cyc_List_List*_tmp520;struct Cyc_List_List*_tmp51F;_LL10: _tmp51F=_stmttmp27.f1;_tmp520=_stmttmp27.f2;_LL11: {struct Cyc_List_List*in1=_tmp51F;struct Cyc_List_List*out1=_tmp520;
in=({struct Cyc_List_List*_tmp521=_cycalloc(sizeof(*_tmp521));_tmp521->hd=f->name,_tmp521->tl=in;_tmp521;});
out=({Cyc_List_append(out,out1);});
goto _LLA;}}}else{goto _LLD;}}else{_LLD: _LLE:
 goto _LLA;}_LLA:;}}
# 3041
if(depth > 0){
# 3043
struct _tuple8 _stmttmp28=({Cyc_Absyn_analyze_dec_tree(n,def,depth + 1);});struct Cyc_List_List*_tmp523;struct Cyc_List_List*_tmp522;_LL13: _tmp522=_stmttmp28.f1;_tmp523=_stmttmp28.f2;_LL14: {struct Cyc_List_List*in1=_tmp522;struct Cyc_List_List*out1=_tmp523;
# 3045
out=({Cyc_List_append(out,out1);});}}else{
# 3049
void*_tmp524=def;switch(*((int*)_tmp524)){case 1U: if(((struct Cyc_Absyn_Break_s_Absyn_Raw_stmt_struct*)((struct Cyc_Absyn_Stmt*)((struct Cyc_Tcpat_Rhs*)((struct Cyc_Tcpat_Success_Tcpat_Decision_struct*)_tmp524)->f1)->rhs)->r)->tag == 6U){_LL16: _LL17:
# 3053
 return({struct _tuple8 _tmp66A;_tmp66A.f1=& Cyc_Absyn_catch_all,_tmp66A.f2=0;_tmp66A;});}else{goto _LL1A;}case 0U: _LL18: _LL19:
# 3055
 return({struct _tuple8 _tmp66B;_tmp66B.f1=0,({struct Cyc_List_List*_tmp7F7=({struct Cyc_List_List*_tmp525=_cycalloc(sizeof(*_tmp525));_tmp525->hd=& Cyc_Absyn_match_exn_var,_tmp525->tl=out;_tmp525;});_tmp66B.f2=_tmp7F7;});_tmp66B;});default: _LL1A: _LL1B:
 goto _LL15;}_LL15:;}
# 3059
goto _LL0;}}_LL0:;}
# 3061
return({struct _tuple8 _tmp66C;_tmp66C.f1=in,_tmp66C.f2=out;_tmp66C;});}
# 3008
struct Cyc_List_List*Cyc_Absyn_uncaught_exn(unsigned loc,void*d,struct Cyc_List_List*stry,struct Cyc_List_List*scatch){
# 3073
struct _tuple8 _stmttmp29=({Cyc_Absyn_analyze_dec_tree(& Cyc_Absyn_dummy_qvar,d,0);});struct Cyc_List_List*_tmp528;struct Cyc_List_List*_tmp527;_LL1: _tmp527=_stmttmp29.f1;_tmp528=_stmttmp29.f2;_LL2: {struct Cyc_List_List*in=_tmp527;struct Cyc_List_List*out=_tmp528;
if(in == & Cyc_Absyn_catch_all)
# 3077
return scatch;
# 3074
in=({Cyc_Absyn_qvlist_minus(in,out);});{
# 3083
struct Cyc_List_List*uncaught=({Cyc_Absyn_qvlist_minus(stry,in);});
# 3085
for(0;out != 0;out=out->tl){
uncaught=({Cyc_Absyn_add_qvar(uncaught,(struct _tuple0*)out->hd);});}
# 3093
for(0;scatch != 0;scatch=scatch->tl){
uncaught=({Cyc_Absyn_add_qvar(uncaught,(struct _tuple0*)scatch->hd);});}
# 3096
return uncaught;}}}
# 3125 "absyn.cyc"
inline int Cyc_Absyn_copy_reentrant(int t){return t;}
inline int Cyc_Absyn_rsubstitute_reentrant(struct _RegionHandle*rgn,struct Cyc_List_List*inst,int re){
# 3129
return re;}
# 3131
inline int Cyc_Absyn_is_reentrant(int t){
return t == Cyc_Absyn_reentrant;}
# 3131
const int Cyc_Absyn_noreentrant=0;
# 3134
const int Cyc_Absyn_reentrant=1;
# 3136
static struct _fat_ptr Cyc_Absyn_con2string(void*t){
# 3138
void*_tmp52D=t;switch(*((int*)_tmp52D)){case 0U: _LL1: _LL2:
# 3140
 return({const char*_tmp52E="VoidCon";_tag_fat(_tmp52E,sizeof(char),8U);});case 1U: _LL3: _LL4:
 return({const char*_tmp52F="IntCon(_,_)";_tag_fat(_tmp52F,sizeof(char),12U);});case 2U: _LL5: _LL6:
 return({const char*_tmp530="FloatCon(_)";_tag_fat(_tmp530,sizeof(char),12U);});case 3U: _LL7: _LL8:
 return({const char*_tmp531="RgnHandleCon";_tag_fat(_tmp531,sizeof(char),13U);});case 4U: _LL9: _LLA:
 return({const char*_tmp532="XRgnHandleCon";_tag_fat(_tmp532,sizeof(char),14U);});case 5U: _LLB: _LLC:
 return({const char*_tmp533="TagCon";_tag_fat(_tmp533,sizeof(char),7U);});case 6U: _LLD: _LLE:
 return({const char*_tmp534="HeapCon";_tag_fat(_tmp534,sizeof(char),8U);});case 7U: _LLF: _LL10:
 return({const char*_tmp535="UniqueCon";_tag_fat(_tmp535,sizeof(char),10U);});case 8U: _LL11: _LL12:
 return({const char*_tmp536="RefCntCon";_tag_fat(_tmp536,sizeof(char),10U);});case 9U: _LL13: _LL14:
 return({const char*_tmp537="AcessCon";_tag_fat(_tmp537,sizeof(char),9U);});case 10U: _LL15: _LL16:
 return({const char*_tmp538="CAccessCon";_tag_fat(_tmp538,sizeof(char),11U);});case 11U: _LL17: _LL18:
 return({const char*_tmp539="JoinCon";_tag_fat(_tmp539,sizeof(char),8U);});case 12U: _LL19: _LL1A:
 return({const char*_tmp53A="RgnsCon";_tag_fat(_tmp53A,sizeof(char),8U);});case 13U: _LL1B: _LL1C:
 return({const char*_tmp53B="TrueCon";_tag_fat(_tmp53B,sizeof(char),8U);});case 14U: _LL1D: _LL1E:
 return({const char*_tmp53C="FalseCon";_tag_fat(_tmp53C,sizeof(char),9U);});case 15U: _LL1F: _LL20:
 return({const char*_tmp53D="ThinCon";_tag_fat(_tmp53D,sizeof(char),8U);});case 16U: _LL21: _LL22:
 return({const char*_tmp53E="FatCon";_tag_fat(_tmp53E,sizeof(char),7U);});case 17U: _LL23: _LL24:
 return({const char*_tmp53F="EnumCon(_,_)";_tag_fat(_tmp53F,sizeof(char),13U);});case 18U: _LL25: _LL26:
 return({const char*_tmp540="AnonEnumCon(_)";_tag_fat(_tmp540,sizeof(char),15U);});case 19U: _LL27: _LL28:
 return({const char*_tmp541="BuiltinCon(_,_)";_tag_fat(_tmp541,sizeof(char),16U);});case 20U: _LL29: _LL2A:
 return({const char*_tmp542="DatatypeCon(_)";_tag_fat(_tmp542,sizeof(char),15U);});case 21U: _LL2B: _LL2C:
 return({const char*_tmp543="DatatypeFieldCon(_)";_tag_fat(_tmp543,sizeof(char),20U);});default: _LL2D: _LL2E:
 return({const char*_tmp544="AggrCoin(_)";_tag_fat(_tmp544,sizeof(char),12U);});}_LL0:;}struct _tuple23{struct _fat_ptr*f1;struct Cyc_Absyn_Tqual f2;};
# 3136
struct _fat_ptr Cyc_Absyn_typcon2string(void*t){
# 3169
void*_tmp546=t;void**_tmp547;void*_tmp54A;struct Cyc_Absyn_Typedefdecl*_tmp549;struct Cyc_List_List*_tmp548;struct Cyc_List_List*_tmp54B;struct Cyc_List_List*_tmp550;struct Cyc_Absyn_VarargInfo*_tmp54F;struct Cyc_List_List*_tmp54E;void*_tmp54D;void*_tmp54C;struct Cyc_List_List*_tmp551;void*_tmp553;void*_tmp552;void*_tmp558;void*_tmp557;void*_tmp556;void*_tmp555;void*_tmp554;struct Cyc_Absyn_Tvar*_tmp559;void*_tmp55A;struct Cyc_List_List*_tmp55C;void*_tmp55B;switch(*((int*)_tmp546)){case 0U: _LL1: _tmp55B=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp546)->f1;_tmp55C=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp546)->f2;_LL2: {void*tc=_tmp55B;struct Cyc_List_List*ts=_tmp55C;
# 3171
return(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp55F=({struct Cyc_String_pa_PrintArg_struct _tmp66E;_tmp66E.tag=0U,({
# 3173
struct _fat_ptr _tmp7F8=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_con2string(tc);}));_tmp66E.f1=_tmp7F8;});_tmp66E;});struct Cyc_String_pa_PrintArg_struct _tmp560=({struct Cyc_String_pa_PrintArg_struct _tmp66D;_tmp66D.tag=0U,({
struct _fat_ptr _tmp7F9=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_list2string(ts,Cyc_Absyn_typcon2string);}));_tmp66D.f1=_tmp7F9;});_tmp66D;});void*_tmp55D[2U];_tmp55D[0]=& _tmp55F,_tmp55D[1]=& _tmp560;({struct _fat_ptr _tmp7FA=({const char*_tmp55E="AppType(%s,{%s})";_tag_fat(_tmp55E,sizeof(char),17U);});Cyc_aprintf(_tmp7FA,_tag_fat(_tmp55D,sizeof(void*),2U));});});}case 1U: _LL3: _tmp55A=(void*)((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp546)->f2;_LL4: {void*to=_tmp55A;
# 3176
return(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp563=({struct Cyc_String_pa_PrintArg_struct _tmp66F;_tmp66F.tag=0U,({
struct _fat_ptr _tmp7FB=(struct _fat_ptr)((struct _fat_ptr)((unsigned)to?({Cyc_Absyn_typcon2string(to);}):(struct _fat_ptr)({const char*_tmp564="NULL";_tag_fat(_tmp564,sizeof(char),5U);})));_tmp66F.f1=_tmp7FB;});_tmp66F;});void*_tmp561[1U];_tmp561[0]=& _tmp563;({struct _fat_ptr _tmp7FC=({const char*_tmp562="Evar(_,%s,_,_)";_tag_fat(_tmp562,sizeof(char),15U);});Cyc_aprintf(_tmp7FC,_tag_fat(_tmp561,sizeof(void*),1U));});});}case 2U: _LL5: _tmp559=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp546)->f1;_LL6: {struct Cyc_Absyn_Tvar*tv=_tmp559;
return(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp567=({struct Cyc_String_pa_PrintArg_struct _tmp672;_tmp672.tag=0U,({
struct _fat_ptr _tmp7FD=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_tvar2string(tv);}));_tmp672.f1=_tmp7FD;});_tmp672;});struct Cyc_String_pa_PrintArg_struct _tmp568=({struct Cyc_String_pa_PrintArg_struct _tmp671;_tmp671.tag=0U,({
struct _fat_ptr _tmp7FE=(struct _fat_ptr)((struct _fat_ptr)({struct _fat_ptr(*_tmp56A)(void*)=Cyc_Absynpp_kindbound2string;void*_tmp56B=({Cyc_Absyn_compress_kb(tv->kind);});_tmp56A(_tmp56B);}));_tmp671.f1=_tmp7FE;});_tmp671;});struct Cyc_Int_pa_PrintArg_struct _tmp569=({struct Cyc_Int_pa_PrintArg_struct _tmp670;_tmp670.tag=1U,_tmp670.f1=(unsigned long)tv->identity;_tmp670;});void*_tmp565[3U];_tmp565[0]=& _tmp567,_tmp565[1]=& _tmp568,_tmp565[2]=& _tmp569;({struct _fat_ptr _tmp7FF=({const char*_tmp566="VarType(tvar=%s,kind=%s,id=%d)";_tag_fat(_tmp566,sizeof(char),31U);});Cyc_aprintf(_tmp7FF,_tag_fat(_tmp565,sizeof(void*),3U));});});}case 3U: _LL7: _tmp554=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp546)->f1).elt_type;_tmp555=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp546)->f1).ptr_atts).rgn;_tmp556=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp546)->f1).ptr_atts).nullable;_tmp557=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp546)->f1).ptr_atts).bounds;_tmp558=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp546)->f1).ptr_atts).zero_term;_LL8: {void*ta=_tmp554;void*e=_tmp555;void*n=_tmp556;void*b=_tmp557;void*z=_tmp558;
# 3182
return(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp56E=({struct Cyc_String_pa_PrintArg_struct _tmp677;_tmp677.tag=0U,({
struct _fat_ptr _tmp800=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_typcon2string(ta);}));_tmp677.f1=_tmp800;});_tmp677;});struct Cyc_String_pa_PrintArg_struct _tmp56F=({struct Cyc_String_pa_PrintArg_struct _tmp676;_tmp676.tag=0U,({
struct _fat_ptr _tmp801=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_typcon2string(e);}));_tmp676.f1=_tmp801;});_tmp676;});struct Cyc_String_pa_PrintArg_struct _tmp570=({struct Cyc_String_pa_PrintArg_struct _tmp675;_tmp675.tag=0U,({
struct _fat_ptr _tmp802=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_typcon2string(n);}));_tmp675.f1=_tmp802;});_tmp675;});struct Cyc_String_pa_PrintArg_struct _tmp571=({struct Cyc_String_pa_PrintArg_struct _tmp674;_tmp674.tag=0U,({
struct _fat_ptr _tmp803=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_typcon2string(b);}));_tmp674.f1=_tmp803;});_tmp674;});struct Cyc_String_pa_PrintArg_struct _tmp572=({struct Cyc_String_pa_PrintArg_struct _tmp673;_tmp673.tag=0U,({
struct _fat_ptr _tmp804=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_typcon2string(z);}));_tmp673.f1=_tmp804;});_tmp673;});void*_tmp56C[5U];_tmp56C[0]=& _tmp56E,_tmp56C[1]=& _tmp56F,_tmp56C[2]=& _tmp570,_tmp56C[3]=& _tmp571,_tmp56C[4]=& _tmp572;({struct _fat_ptr _tmp805=({const char*_tmp56D="PointerType(PtrInfo{%s,_,PtrAtts{%s,%s,%s,%s,_}})";_tag_fat(_tmp56D,sizeof(char),50U);});Cyc_aprintf(_tmp805,_tag_fat(_tmp56C,sizeof(void*),5U));});});}case 4U: _LL9: _tmp552=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp546)->f1).elt_type;_tmp553=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp546)->f1).zero_term;_LLA: {void*ta=_tmp552;void*z=_tmp553;
# 3189
return(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp575=({struct Cyc_String_pa_PrintArg_struct _tmp679;_tmp679.tag=0U,({
struct _fat_ptr _tmp806=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_typcon2string(ta);}));_tmp679.f1=_tmp806;});_tmp679;});struct Cyc_String_pa_PrintArg_struct _tmp576=({struct Cyc_String_pa_PrintArg_struct _tmp678;_tmp678.tag=0U,({struct _fat_ptr _tmp807=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_typcon2string(z);}));_tmp678.f1=_tmp807;});_tmp678;});void*_tmp573[2U];_tmp573[0]=& _tmp575,_tmp573[1]=& _tmp576;({struct _fat_ptr _tmp808=({const char*_tmp574="ArrayType(ArrayInfo{%s,_,_,%s,_})";_tag_fat(_tmp574,sizeof(char),34U);});Cyc_aprintf(_tmp808,_tag_fat(_tmp573,sizeof(void*),2U));});});}case 6U: _LLB: _tmp551=((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp546)->f1;_LLC: {struct Cyc_List_List*args=_tmp551;
# 3192
struct _fat_ptr sarg;
if(args == 0)sarg=({const char*_tmp577="";_tag_fat(_tmp577,sizeof(char),1U);});else{
# 3196
{struct _tuple16*_stmttmp2A=(struct _tuple16*)args->hd;void*_tmp578;_LL1A: _tmp578=_stmttmp2A->f2;_LL1B: {void*at=_tmp578;
sarg=({Cyc_Absyn_typcon2string(at);});
args=args->tl;}}
# 3200
for(0;(unsigned)args;args=args->tl){
# 3202
struct _tuple16*_stmttmp2B=(struct _tuple16*)args->hd;void*_tmp579;_LL1D: _tmp579=_stmttmp2B->f2;_LL1E: {void*at=_tmp579;
sarg=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp57C=({struct Cyc_String_pa_PrintArg_struct _tmp67B;_tmp67B.tag=0U,_tmp67B.f1=(struct _fat_ptr)((struct _fat_ptr)sarg);_tmp67B;});struct Cyc_String_pa_PrintArg_struct _tmp57D=({struct Cyc_String_pa_PrintArg_struct _tmp67A;_tmp67A.tag=0U,({struct _fat_ptr _tmp809=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_typcon2string(at);}));_tmp67A.f1=_tmp809;});_tmp67A;});void*_tmp57A[2U];_tmp57A[0]=& _tmp57C,_tmp57A[1]=& _tmp57D;({struct _fat_ptr _tmp80A=({const char*_tmp57B="%s,%s";_tag_fat(_tmp57B,sizeof(char),6U);});Cyc_aprintf(_tmp80A,_tag_fat(_tmp57A,sizeof(void*),2U));});});}}}
# 3206
return(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp580=({struct Cyc_String_pa_PrintArg_struct _tmp67C;_tmp67C.tag=0U,_tmp67C.f1=(struct _fat_ptr)((struct _fat_ptr)sarg);_tmp67C;});void*_tmp57E[1U];_tmp57E[0]=& _tmp580;({struct _fat_ptr _tmp80B=({const char*_tmp57F="TupleType({%s})";_tag_fat(_tmp57F,sizeof(char),16U);});Cyc_aprintf(_tmp80B,_tag_fat(_tmp57E,sizeof(void*),1U));});});}case 5U: _LLD: _tmp54C=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp546)->f1).effect;_tmp54D=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp546)->f1).ret_type;_tmp54E=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp546)->f1).args;_tmp54F=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp546)->f1).cyc_varargs;_tmp550=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp546)->f1).rgn_po;_LLE: {void*e=_tmp54C;void*r=_tmp54D;struct Cyc_List_List*args=_tmp54E;struct Cyc_Absyn_VarargInfo*va=_tmp54F;struct Cyc_List_List*qb=_tmp550;
# 3208
struct _fat_ptr estr=({const char*_tmp59E="NULL";_tag_fat(_tmp59E,sizeof(char),5U);});
if(e != 0)estr=({Cyc_Absyn_typcon2string(e);});{struct _fat_ptr fstr=({Cyc_Absyn_typcon2string(r);});
# 3212
struct _fat_ptr sarg;
if(args == 0)sarg=({const char*_tmp581="";_tag_fat(_tmp581,sizeof(char),1U);});else{
# 3216
{struct _tuple9*_stmttmp2C=(struct _tuple9*)args->hd;struct _tuple23*_tmp582=(struct _tuple23*)_stmttmp2C;struct Cyc_Absyn_Tqual _tmp583;_LL20: _tmp583=_tmp582->f2;_LL21: {struct Cyc_Absyn_Tqual at=_tmp583;
sarg=({const char*_tmp584="?";_tag_fat(_tmp584,sizeof(char),2U);});
args=args->tl;}}
# 3220
for(0;(unsigned)args;args=args->tl){
# 3222
struct _tuple9*_stmttmp2D=(struct _tuple9*)args->hd;struct _tuple23*_tmp585=(struct _tuple23*)_stmttmp2D;struct Cyc_Absyn_Tqual _tmp586;_LL23: _tmp586=_tmp585->f2;_LL24: {struct Cyc_Absyn_Tqual at=_tmp586;
sarg=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp589=({struct Cyc_String_pa_PrintArg_struct _tmp67E;_tmp67E.tag=0U,_tmp67E.f1=(struct _fat_ptr)((struct _fat_ptr)sarg);_tmp67E;});struct Cyc_String_pa_PrintArg_struct _tmp58A=({struct Cyc_String_pa_PrintArg_struct _tmp67D;_tmp67D.tag=0U,({struct _fat_ptr _tmp80C=(struct _fat_ptr)({const char*_tmp58B="?";_tag_fat(_tmp58B,sizeof(char),2U);});_tmp67D.f1=_tmp80C;});_tmp67D;});void*_tmp587[2U];_tmp587[0]=& _tmp589,_tmp587[1]=& _tmp58A;({struct _fat_ptr _tmp80D=({const char*_tmp588="%s,%s";_tag_fat(_tmp588,sizeof(char),6U);});Cyc_aprintf(_tmp80D,_tag_fat(_tmp587,sizeof(void*),2U));});});}}}{
# 3227
struct _fat_ptr vastr=({const char*_tmp59D="NULL";_tag_fat(_tmp59D,sizeof(char),5U);});
if((unsigned)va)vastr=({const char*_tmp58C="?";_tag_fat(_tmp58C,sizeof(char),2U);});{struct _fat_ptr qbstr;
# 3230
if(qb == 0)qbstr=({const char*_tmp58D="";_tag_fat(_tmp58D,sizeof(char),1U);});else{
# 3233
{struct _tuple15*_stmttmp2E=(struct _tuple15*)qb->hd;void*_tmp58E;_LL26: _tmp58E=_stmttmp2E->f2;_LL27: {void*at=_tmp58E;
qbstr=({Cyc_Absyn_typcon2string(at);});
qb=qb->tl;}}
# 3237
for(0;(unsigned)qb;qb=qb->tl){
# 3239
struct _tuple15*_stmttmp2F=(struct _tuple15*)qb->hd;void*_tmp590;void*_tmp58F;_LL29: _tmp58F=_stmttmp2F->f1;_tmp590=_stmttmp2F->f2;_LL2A: {void*t1=_tmp58F;void*t2=_tmp590;
qbstr=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp593=({struct Cyc_String_pa_PrintArg_struct _tmp681;_tmp681.tag=0U,_tmp681.f1=(struct _fat_ptr)((struct _fat_ptr)qbstr);_tmp681;});struct Cyc_String_pa_PrintArg_struct _tmp594=({struct Cyc_String_pa_PrintArg_struct _tmp680;_tmp680.tag=0U,({struct _fat_ptr _tmp80E=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_typcon2string(t1);}));_tmp680.f1=_tmp80E;});_tmp680;});struct Cyc_String_pa_PrintArg_struct _tmp595=({struct Cyc_String_pa_PrintArg_struct _tmp67F;_tmp67F.tag=0U,({
struct _fat_ptr _tmp80F=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_typcon2string(t2);}));_tmp67F.f1=_tmp80F;});_tmp67F;});void*_tmp591[3U];_tmp591[0]=& _tmp593,_tmp591[1]=& _tmp594,_tmp591[2]=& _tmp595;({struct _fat_ptr _tmp810=({const char*_tmp592="%s,$(%s,%s)";_tag_fat(_tmp592,sizeof(char),12U);});Cyc_aprintf(_tmp810,_tag_fat(_tmp591,sizeof(void*),3U));});});}}}
# 3244
return(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp598=({struct Cyc_String_pa_PrintArg_struct _tmp686;_tmp686.tag=0U,_tmp686.f1=(struct _fat_ptr)((struct _fat_ptr)estr);_tmp686;});struct Cyc_String_pa_PrintArg_struct _tmp599=({struct Cyc_String_pa_PrintArg_struct _tmp685;_tmp685.tag=0U,_tmp685.f1=(struct _fat_ptr)((struct _fat_ptr)fstr);_tmp685;});struct Cyc_String_pa_PrintArg_struct _tmp59A=({struct Cyc_String_pa_PrintArg_struct _tmp684;_tmp684.tag=0U,_tmp684.f1=(struct _fat_ptr)((struct _fat_ptr)sarg);_tmp684;});struct Cyc_String_pa_PrintArg_struct _tmp59B=({struct Cyc_String_pa_PrintArg_struct _tmp683;_tmp683.tag=0U,_tmp683.f1=(struct _fat_ptr)((struct _fat_ptr)vastr);_tmp683;});struct Cyc_String_pa_PrintArg_struct _tmp59C=({struct Cyc_String_pa_PrintArg_struct _tmp682;_tmp682.tag=0U,_tmp682.f1=(struct _fat_ptr)((struct _fat_ptr)qbstr);_tmp682;});void*_tmp596[5U];_tmp596[0]=& _tmp598,_tmp596[1]=& _tmp599,_tmp596[2]=& _tmp59A,_tmp596[3]=& _tmp59B,_tmp596[4]=& _tmp59C;({struct _fat_ptr _tmp811=({const char*_tmp597="FnType(FnInfo{_,%s,_,%s,%s,_,%s,%s,_,_,_,_,_,_,_,_,_})";_tag_fat(_tmp597,sizeof(char),55U);});Cyc_aprintf(_tmp811,_tag_fat(_tmp596,sizeof(void*),5U));});});}}}}case 7U: _LLF: _tmp54B=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp546)->f2;_LL10: {struct Cyc_List_List*afl=_tmp54B;
# 3248
return(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp5A1=({struct Cyc_String_pa_PrintArg_struct _tmp687;_tmp687.tag=0U,({
struct _fat_ptr _tmp812=(struct _fat_ptr)({const char*_tmp5A2="?";_tag_fat(_tmp5A2,sizeof(char),2U);});_tmp687.f1=_tmp812;});_tmp687;});void*_tmp59F[1U];_tmp59F[0]=& _tmp5A1;({struct _fat_ptr _tmp813=({const char*_tmp5A0="AnonAggrType(_,{%s})";_tag_fat(_tmp5A0,sizeof(char),21U);});Cyc_aprintf(_tmp813,_tag_fat(_tmp59F,sizeof(void*),1U));});});}case 8U: _LL11: _tmp548=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp546)->f2;_tmp549=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp546)->f3;_tmp54A=(void*)((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp546)->f4;_LL12: {struct Cyc_List_List*ts=_tmp548;struct Cyc_Absyn_Typedefdecl*tdef=_tmp549;void*to=_tmp54A;
# 3251
struct _fat_ptr tdefstr=({const char*_tmp5A9="NULL";_tag_fat(_tmp5A9,sizeof(char),5U);});
if(tdef != 0){
# 3254
void*x=tdef->defn;
if(x != 0)tdefstr=({Cyc_Absyn_typcon2string(x);});}
# 3252
return(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp5A5=({struct Cyc_String_pa_PrintArg_struct _tmp68A;_tmp68A.tag=0U,({
# 3259
struct _fat_ptr _tmp814=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_list2string(ts,Cyc_Absyn_typcon2string);}));_tmp68A.f1=_tmp814;});_tmp68A;});struct Cyc_String_pa_PrintArg_struct _tmp5A6=({struct Cyc_String_pa_PrintArg_struct _tmp689;_tmp689.tag=0U,_tmp689.f1=(struct _fat_ptr)((struct _fat_ptr)tdefstr);_tmp689;});struct Cyc_String_pa_PrintArg_struct _tmp5A7=({struct Cyc_String_pa_PrintArg_struct _tmp688;_tmp688.tag=0U,({
struct _fat_ptr _tmp815=(struct _fat_ptr)((struct _fat_ptr)(to != 0?({Cyc_Absyn_typcon2string(to);}):(struct _fat_ptr)({const char*_tmp5A8="NULL";_tag_fat(_tmp5A8,sizeof(char),5U);})));_tmp688.f1=_tmp815;});_tmp688;});void*_tmp5A3[3U];_tmp5A3[0]=& _tmp5A5,_tmp5A3[1]=& _tmp5A6,_tmp5A3[2]=& _tmp5A7;({struct _fat_ptr _tmp816=({const char*_tmp5A4="TypedefType(_,{%s},%s,%s)";_tag_fat(_tmp5A4,sizeof(char),26U);});Cyc_aprintf(_tmp816,_tag_fat(_tmp5A3,sizeof(void*),3U));});});}case 10U: _LL13: _tmp547=((struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_tmp546)->f2;_LL14: {void**tptr=_tmp547;
# 3262
struct _fat_ptr tptrstr=({const char*_tmp5AD="NULL";_tag_fat(_tmp5AD,sizeof(char),5U);});
if((unsigned)tptr &&(unsigned)*tptr)tptrstr=({Cyc_Absyn_typcon2string(*tptr);});return(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp5AC=({struct Cyc_String_pa_PrintArg_struct _tmp68B;_tmp68B.tag=0U,_tmp68B.f1=(struct _fat_ptr)((struct _fat_ptr)tptrstr);_tmp68B;});void*_tmp5AA[1U];_tmp5AA[0]=& _tmp5AC;({struct _fat_ptr _tmp817=({const char*_tmp5AB="TypeDeclType(_,%s)";_tag_fat(_tmp5AB,sizeof(char),19U);});Cyc_aprintf(_tmp817,_tag_fat(_tmp5AA,sizeof(void*),1U));});});}case 9U: _LL15: _LL16:
# 3265
 return({const char*_tmp5AE="ValueofType(_)";_tag_fat(_tmp5AE,sizeof(char),15U);});default: _LL17: _LL18:
 return({const char*_tmp5AF="TypeofType(_)";_tag_fat(_tmp5AF,sizeof(char),14U);});}_LL0:;}
# 3136
static int Cyc_Absyn_temp_count=0;static char _tmp5B1[10U]="__zTMPz__";
# 3272
static struct _fat_ptr Cyc_Absyn_tv_signature={_tmp5B1,_tmp5B1,_tmp5B1 + 10U};
static void*Cyc_Absyn_opt_knd=0;
# 3275
static int Cyc_Absyn_is_special_tv(struct Cyc_Absyn_Tvar*tv){
return tv->kind == Cyc_Absyn_opt_knd;}
# 3279
static void*Cyc_Absyn_fn_ret_type(struct Cyc_Absyn_Exp*e1){
void*e1_typ=e1->topt;
if(e1_typ == 0)return 0;{void*_tmp5B3=e1_typ;struct Cyc_Absyn_FnInfo _tmp5B4;struct Cyc_Absyn_FnInfo _tmp5B5;if(_tmp5B3 != 0)switch(*((int*)_tmp5B3)){case 3U: if(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp5B3)->f1).elt_type)->tag == 5U){_LL1: _tmp5B5=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp5B3)->f1).elt_type)->f1;_LL2: {struct Cyc_Absyn_FnInfo i=_tmp5B5;
# 3285
_tmp5B4=i;goto _LL4;}}else{goto _LL5;}case 5U: _LL3: _tmp5B4=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp5B3)->f1;_LL4: {struct Cyc_Absyn_FnInfo info=_tmp5B4;
# 3287
return info.ret_type;}default: goto _LL5;}else{_LL5: _LL6:
 return 0;}_LL0:;}}
# 3292
void Cyc_Absyn_patch_stmt_exp(struct Cyc_Absyn_Stmt*s){
return;{
void*_stmttmp30=s->r;void*_tmp5B7=_stmttmp30;struct Cyc_Absyn_Stmt*_tmp5B9;struct Cyc_Absyn_Decl*_tmp5B8;if(((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp5B7)->tag == 12U){_LL1: _tmp5B8=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp5B7)->f1;_tmp5B9=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp5B7)->f2;_LL2: {struct Cyc_Absyn_Decl*d1=_tmp5B8;struct Cyc_Absyn_Stmt*s1=_tmp5B9;
# 3296
void*_stmttmp31=s1->r;void*_tmp5BA=_stmttmp31;struct Cyc_Absyn_Stmt*_tmp5BC;struct Cyc_Absyn_Stmt*_tmp5BB;if(((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_tmp5BA)->tag == 2U){_LL6: _tmp5BB=((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_tmp5BA)->f1;_tmp5BC=((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_tmp5BA)->f2;_LL7: {struct Cyc_Absyn_Stmt*d1=_tmp5BB;struct Cyc_Absyn_Stmt*s2=_tmp5BC;
# 3298
void*_stmttmp32=d1->r;void*_tmp5BD=_stmttmp32;struct Cyc_Absyn_Stmt*_tmp5BF;struct Cyc_Absyn_Decl*_tmp5BE;if(((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp5BD)->tag == 12U){_LLB: _tmp5BE=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp5BD)->f1;_tmp5BF=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp5BD)->f2;_LLC: {struct Cyc_Absyn_Decl*rd=_tmp5BE;struct Cyc_Absyn_Stmt*inner=_tmp5BF;
# 3300
void*_stmttmp33=rd->r;void*_tmp5C0=_stmttmp33;struct Cyc_Absyn_Vardecl*_tmp5C2;struct Cyc_Absyn_Tvar*_tmp5C1;if(((struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*)_tmp5C0)->tag == 4U){if(((struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*)_tmp5C0)->f3 == 0){_LL10: _tmp5C1=((struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*)_tmp5C0)->f1;_tmp5C2=((struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*)_tmp5C0)->f2;_LL11: {struct Cyc_Absyn_Tvar*tv=_tmp5C1;struct Cyc_Absyn_Vardecl*vd=_tmp5C2;
# 3302
void*_stmttmp34=inner->r;void*_tmp5C3=_stmttmp34;struct Cyc_Absyn_Stmt*_tmp5C4;if(((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_tmp5C3)->tag == 2U){if(((struct Cyc_Absyn_Skip_s_Absyn_Raw_stmt_struct*)((struct Cyc_Absyn_Stmt*)((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_tmp5C3)->f2)->r)->tag == 0U){_LL15: _tmp5C4=((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_tmp5C3)->f1;_LL16: {struct Cyc_Absyn_Stmt*ass=_tmp5C4;
# 3304
void*_stmttmp35=vd->type;void*_tmp5C5=_stmttmp35;struct Cyc_Absyn_Tvar*_tmp5C6;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp5C5)->tag == 0U){if(((struct Cyc_Absyn_RgnHandleCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp5C5)->f1)->tag == 3U){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp5C5)->f2 != 0){if(((struct Cyc_Absyn_VarType_Absyn_Type_struct*)((struct Cyc_List_List*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp5C5)->f2)->hd)->tag == 2U){if(((struct Cyc_List_List*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp5C5)->f2)->tl == 0){_LL1A: _tmp5C6=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp5C5)->f2)->hd)->f1;_LL1B: {struct Cyc_Absyn_Tvar*tv=_tmp5C6;
# 3307
if(({Cyc_Absyn_is_special_tv(tv);})== 0)return;{void*_stmttmp36=ass->r;void*_tmp5C7=_stmttmp36;struct Cyc_Absyn_Exp*_tmp5C8;if(((struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct*)_tmp5C7)->tag == 1U){_LL1F: _tmp5C8=((struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct*)_tmp5C7)->f1;_LL20: {struct Cyc_Absyn_Exp*e0=_tmp5C8;
# 3310
void*_stmttmp37=e0->r;void*_tmp5C9=_stmttmp37;struct Cyc_Absyn_Exp*_tmp5CB;struct Cyc_Absyn_Exp*_tmp5CA;if(((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp5C9)->tag == 4U){if(((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp5C9)->f2 == 0){_LL24: _tmp5CA=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp5C9)->f1;_tmp5CB=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp5C9)->f3;_LL25: {struct Cyc_Absyn_Exp*e1=_tmp5CA;struct Cyc_Absyn_Exp*e2=_tmp5CB;
# 3312
if(e2->topt != Cyc_Absyn_void_type)
return;
# 3312
({void*_tmp5CC=0U;({struct _fat_ptr _tmp818=({const char*_tmp5CD="\nOK!\n";_tag_fat(_tmp5CD,sizeof(char),6U);});Cyc_printf(_tmp818,_tag_fat(_tmp5CC,sizeof(void*),0U));});});
# 3315
({void*_tmp81A=(void*)({struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*_tmp5CE=_cycalloc(sizeof(*_tmp5CE));_tmp5CE->tag=12U,_tmp5CE->f1=rd,({
struct Cyc_Absyn_Stmt*_tmp819=({Cyc_Absyn_exp_stmt(e2,0U);});_tmp5CE->f2=_tmp819;});_tmp5CE;});
# 3315
s->r=_tmp81A;});
# 3317
return;}}else{goto _LL26;}}else{_LL26: _LL27:
 return;}_LL23:;}}else{_LL21: _LL22:
# 3320
 return;}_LL1E:;}
# 3322
return;}}else{goto _LL1C;}}else{goto _LL1C;}}else{goto _LL1C;}}else{goto _LL1C;}}else{_LL1C: _LL1D:
 return;}_LL19:;}}else{goto _LL17;}}else{_LL17: _LL18:
# 3325
 return;}_LL14:;}}else{goto _LL12;}}else{_LL12: _LL13:
# 3327
 return;}_LLF:;}}else{_LLD: _LLE:
# 3329
 return;}_LLA:;}}else{_LL8: _LL9:
# 3331
 return;}_LL5:;}}else{_LL3: _LL4:
# 3333
 return;}_LL0:;}}
# 3292
struct Cyc_Absyn_Exp*Cyc_Absyn_wrap_fncall(struct Cyc_Absyn_Exp*e1,struct Cyc_List_List*es,unsigned loc){
# 3344
if(Cyc_Absyn_opt_knd == 0)Cyc_Absyn_opt_knd=({Cyc_Tcutil_kind_to_bound(& Cyc_Tcutil_rk);});{struct _fat_ptr*v=({struct _fat_ptr*_tmp601=_cycalloc(sizeof(*_tmp601));({
# 3347
struct _fat_ptr _tmp81C=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp5FF=({struct Cyc_String_pa_PrintArg_struct _tmp693;_tmp693.tag=0U,_tmp693.f1=(struct _fat_ptr)((struct _fat_ptr)Cyc_Absyn_tv_signature);_tmp693;});struct Cyc_Int_pa_PrintArg_struct _tmp600=({struct Cyc_Int_pa_PrintArg_struct _tmp692;_tmp692.tag=1U,_tmp692.f1=(unsigned long)Cyc_Absyn_temp_count;_tmp692;});void*_tmp5FD[2U];_tmp5FD[0]=& _tmp5FF,_tmp5FD[1]=& _tmp600;({struct _fat_ptr _tmp81B=({const char*_tmp5FE="%s%d";_tag_fat(_tmp5FE,sizeof(char),5U);});Cyc_aprintf(_tmp81B,_tag_fat(_tmp5FD,sizeof(void*),2U));});});*_tmp601=_tmp81C;});_tmp601;});
struct Cyc_Absyn_Tvar*tv=({struct Cyc_Absyn_Tvar*_tmp5FC=_cycalloc(sizeof(*_tmp5FC));({struct _fat_ptr*_tmp81F=({struct _fat_ptr*_tmp5FB=_cycalloc(sizeof(*_tmp5FB));({struct _fat_ptr _tmp81E=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp5F9=({struct Cyc_String_pa_PrintArg_struct _tmp691;_tmp691.tag=0U,_tmp691.f1=(struct _fat_ptr)((struct _fat_ptr)Cyc_Absyn_tv_signature);_tmp691;});struct Cyc_Int_pa_PrintArg_struct _tmp5FA=({struct Cyc_Int_pa_PrintArg_struct _tmp690;_tmp690.tag=1U,_tmp690.f1=(unsigned long)Cyc_Absyn_temp_count ++;_tmp690;});void*_tmp5F7[2U];_tmp5F7[0]=& _tmp5F9,_tmp5F7[1]=& _tmp5FA;({struct _fat_ptr _tmp81D=({const char*_tmp5F8="`%s%d";_tag_fat(_tmp5F8,sizeof(char),6U);});Cyc_aprintf(_tmp81D,_tag_fat(_tmp5F7,sizeof(void*),2U));});});*_tmp5FB=_tmp81E;});_tmp5FB;});_tmp5FC->name=_tmp81F;}),_tmp5FC->identity=- 1,_tmp5FC->kind=(void*)_check_null(Cyc_Absyn_opt_knd);_tmp5FC;});
# 3351
void*t=({Cyc_Absyn_var_type(tv);});
struct Cyc_Absyn_Vardecl*vd=({struct Cyc_Absyn_Vardecl*(*_tmp5F1)(unsigned varloc,struct _tuple0*x,void*t,struct Cyc_Absyn_Exp*init)=Cyc_Absyn_new_vardecl;unsigned _tmp5F2=loc;struct _tuple0*_tmp5F3=({struct _tuple0*_tmp5F4=_cycalloc(sizeof(*_tmp5F4));
_tmp5F4->f1=Cyc_Absyn_Loc_n,_tmp5F4->f2=v;_tmp5F4;});void*_tmp5F5=({Cyc_Absyn_rgn_handle_type(t);});struct Cyc_Absyn_Exp*_tmp5F6=0;_tmp5F1(_tmp5F2,_tmp5F3,_tmp5F5,_tmp5F6);});
# 3358
struct _fat_ptr*v1=({struct _fat_ptr*_tmp5F0=_cycalloc(sizeof(*_tmp5F0));({struct _fat_ptr _tmp821=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp5EE=({struct Cyc_String_pa_PrintArg_struct _tmp68F;_tmp68F.tag=0U,_tmp68F.f1=(struct _fat_ptr)((struct _fat_ptr)Cyc_Absyn_tv_signature);_tmp68F;});struct Cyc_Int_pa_PrintArg_struct _tmp5EF=({struct Cyc_Int_pa_PrintArg_struct _tmp68E;_tmp68E.tag=1U,_tmp68E.f1=(unsigned long)Cyc_Absyn_temp_count ++;_tmp68E;});void*_tmp5EC[2U];_tmp5EC[0]=& _tmp5EE,_tmp5EC[1]=& _tmp5EF;({struct _fat_ptr _tmp820=({const char*_tmp5ED="%s%d";_tag_fat(_tmp5ED,sizeof(char),5U);});Cyc_aprintf(_tmp820,_tag_fat(_tmp5EC,sizeof(void*),2U));});});*_tmp5F0=_tmp821;});_tmp5F0;});
struct _tuple0*qv=({struct _tuple0*_tmp5EB=_cycalloc(sizeof(*_tmp5EB));({union Cyc_Absyn_Nmspace _tmp822=({Cyc_Absyn_Rel_n(0);});_tmp5EB->f1=_tmp822;}),_tmp5EB->f2=v1;_tmp5EB;});
struct Cyc_Absyn_Exp*v1e=({Cyc_Absyn_var_exp(qv,loc);});
struct Cyc_Absyn_Vardecl*vd1=({struct Cyc_Absyn_Vardecl*(*_tmp5E6)(unsigned varloc,struct _tuple0*x,void*t,struct Cyc_Absyn_Exp*init)=Cyc_Absyn_new_vardecl;unsigned _tmp5E7=0U;struct _tuple0*_tmp5E8=qv;void*_tmp5E9=({Cyc_Absyn_wildtyp(0);});struct Cyc_Absyn_Exp*_tmp5EA=0;_tmp5E6(_tmp5E7,_tmp5E8,_tmp5E9,_tmp5EA);});
# 3363
struct Cyc_Absyn_Exp*fncall=({({void*_tmp824=(void*)({struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*_tmp5E5=_cycalloc(sizeof(*_tmp5E5));_tmp5E5->tag=10U,_tmp5E5->f1=e1,_tmp5E5->f2=es,_tmp5E5->f3=0,_tmp5E5->f4=0,({struct _tuple8*_tmp823=({struct _tuple8*_tmp5E4=_cycalloc(sizeof(*_tmp5E4));_tmp5E4->f1=0,_tmp5E4->f2=0;_tmp5E4;});_tmp5E5->f5=_tmp823;});_tmp5E5;});Cyc_Absyn_new_exp(_tmp824,loc);});});
struct Cyc_Absyn_Stmt*s=({struct Cyc_Absyn_Stmt*(*_tmp5E1)(struct Cyc_Absyn_Exp*e,unsigned loc)=Cyc_Absyn_exp_stmt;struct Cyc_Absyn_Exp*_tmp5E2=({Cyc_Absyn_assign_exp(v1e,fncall,loc);});unsigned _tmp5E3=loc;_tmp5E1(_tmp5E2,_tmp5E3);});
s=({struct Cyc_Absyn_Stmt*(*_tmp5D0)(struct Cyc_Absyn_Decl*d,struct Cyc_Absyn_Stmt*s,unsigned loc)=Cyc_Absyn_decl_stmt;struct Cyc_Absyn_Decl*_tmp5D1=({Cyc_Absyn_region_decl(tv,vd,0,loc);});struct Cyc_Absyn_Stmt*_tmp5D2=s;unsigned _tmp5D3=loc;_tmp5D0(_tmp5D1,_tmp5D2,_tmp5D3);});{
# 3367
struct Cyc_Absyn_Decl*letv=({({struct Cyc_List_List*_tmp825=({struct Cyc_List_List*_tmp5E0=_cycalloc(sizeof(*_tmp5E0));_tmp5E0->hd=vd1,_tmp5E0->tl=0;_tmp5E0;});Cyc_Absyn_letv_decl(_tmp825,loc);});});
s=({struct Cyc_Absyn_Stmt*(*_tmp5D4)(struct Cyc_Absyn_Decl*d,struct Cyc_Absyn_Stmt*s,unsigned loc)=Cyc_Absyn_decl_stmt;struct Cyc_Absyn_Decl*_tmp5D5=letv;struct Cyc_Absyn_Stmt*_tmp5D6=({struct Cyc_Absyn_Stmt*(*_tmp5D7)(struct Cyc_Absyn_Stmt*s1,struct Cyc_Absyn_Stmt*s2,unsigned loc)=Cyc_Absyn_seq_stmt;struct Cyc_Absyn_Stmt*_tmp5D8=s;struct Cyc_Absyn_Stmt*_tmp5D9=({Cyc_Absyn_exp_stmt(v1e,loc);});unsigned _tmp5DA=loc;_tmp5D7(_tmp5D8,_tmp5D9,_tmp5DA);});unsigned _tmp5DB=loc;_tmp5D4(_tmp5D5,_tmp5D6,_tmp5DB);});{
struct Cyc_Absyn_Exp*ret=({Cyc_Absyn_stmt_exp(s,loc);});
({struct Cyc_String_pa_PrintArg_struct _tmp5DE=({struct Cyc_String_pa_PrintArg_struct _tmp68D;_tmp68D.tag=0U,({struct _fat_ptr _tmp826=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_exp2string(ret);}));_tmp68D.f1=_tmp826;});_tmp68D;});struct Cyc_Int_pa_PrintArg_struct _tmp5DF=({struct Cyc_Int_pa_PrintArg_struct _tmp68C;_tmp68C.tag=1U,({
unsigned _tmp827=(unsigned)({Cyc_Absyn_fn_ret_type(e1);});_tmp68C.f1=_tmp827;});_tmp68C;});void*_tmp5DC[2U];_tmp5DC[0]=& _tmp5DE,_tmp5DC[1]=& _tmp5DF;({struct _fat_ptr _tmp828=({const char*_tmp5DD="\n RET %s\t type : %p\n";_tag_fat(_tmp5DD,sizeof(char),21U);});Cyc_printf(_tmp828,_tag_fat(_tmp5DC,sizeof(void*),2U));});});
return ret;}}}}
# 3292
static int Cyc_Absyn_forgive=0;
# 3376
int Cyc_Absyn_forgive_global(){return Cyc_Absyn_forgive;}
void Cyc_Absyn_forgive_global_set(int f){Cyc_Absyn_forgive=f;}static int Cyc_Absyn_b_pthread=0;
# 3381
void Cyc_Absyn_set_pthread(int b){Cyc_Absyn_b_pthread=b;}int Cyc_Absyn_is_pthread(){
return Cyc_Absyn_b_pthread;}
# 3381
struct Cyc_Absyn_Porton_d_Absyn_Raw_decl_struct Cyc_Absyn_Porton_d_val={13U};
# 3387
struct Cyc_Absyn_Portoff_d_Absyn_Raw_decl_struct Cyc_Absyn_Portoff_d_val={14U};
struct Cyc_Absyn_Tempeston_d_Absyn_Raw_decl_struct Cyc_Absyn_Tempeston_d_val={15U};
struct Cyc_Absyn_Tempestoff_d_Absyn_Raw_decl_struct Cyc_Absyn_Tempestoff_d_val={16U};
# 3391
int Cyc_Absyn_porting_c_code=0;
int Cyc_Absyn_no_regions=0;

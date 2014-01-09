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
# 111 "core.h"
void*Cyc_Core_snd(struct _tuple0*);extern char Cyc_Core_Invalid_argument[17U];struct Cyc_Core_Invalid_argument_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Failure[8U];struct Cyc_Core_Failure_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Impossible[11U];struct Cyc_Core_Impossible_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Not_found[10U];struct Cyc_Core_Not_found_exn_struct{char*tag;};extern char Cyc_Core_Unreachable[12U];struct Cyc_Core_Unreachable_exn_struct{char*tag;struct _fat_ptr f1;};
# 165
extern struct _RegionHandle*Cyc_Core_heap_region;
# 168
extern struct _RegionHandle*Cyc_Core_unique_region;struct Cyc_Core_DynamicRegion;struct Cyc_Core_NewDynamicRegion{struct Cyc_Core_DynamicRegion*key;};struct Cyc_Core_ThinRes{void*arr;unsigned nelts;};struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};
# 61 "list.h"
int Cyc_List_length(struct Cyc_List_List*x);
# 76
struct Cyc_List_List*Cyc_List_map(void*(*f)(void*),struct Cyc_List_List*x);
# 83
struct Cyc_List_List*Cyc_List_map_c(void*(*f)(void*,void*),void*env,struct Cyc_List_List*x);extern char Cyc_List_List_mismatch[14U];struct Cyc_List_List_mismatch_exn_struct{char*tag;};
# 178
struct Cyc_List_List*Cyc_List_imp_rev(struct Cyc_List_List*x);
# 184
struct Cyc_List_List*Cyc_List_append(struct Cyc_List_List*x,struct Cyc_List_List*y);extern char Cyc_List_Nth[4U];struct Cyc_List_Nth_exn_struct{char*tag;};
# 242
void*Cyc_List_nth(struct Cyc_List_List*x,int n);
# 246
struct Cyc_List_List*Cyc_List_nth_tail(struct Cyc_List_List*x,int i);
# 265
void*Cyc_List_find_c(void*(*pred)(void*,void*),void*env,struct Cyc_List_List*x);
# 270
struct Cyc_List_List*Cyc_List_zip(struct Cyc_List_List*x,struct Cyc_List_List*y);struct Cyc___cycFILE;struct Cyc_String_pa_PrintArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Int_pa_PrintArg_struct{int tag;unsigned long f1;};struct Cyc_Double_pa_PrintArg_struct{int tag;double f1;};struct Cyc_LongDouble_pa_PrintArg_struct{int tag;long double f1;};struct Cyc_ShortPtr_pa_PrintArg_struct{int tag;short*f1;};struct Cyc_IntPtr_pa_PrintArg_struct{int tag;unsigned long*f1;};
# 73 "cycboot.h"
struct _fat_ptr Cyc_aprintf(struct _fat_ptr,struct _fat_ptr);struct Cyc_ShortPtr_sa_ScanfArg_struct{int tag;short*f1;};struct Cyc_UShortPtr_sa_ScanfArg_struct{int tag;unsigned short*f1;};struct Cyc_IntPtr_sa_ScanfArg_struct{int tag;int*f1;};struct Cyc_UIntPtr_sa_ScanfArg_struct{int tag;unsigned*f1;};struct Cyc_StringPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_DoublePtr_sa_ScanfArg_struct{int tag;double*f1;};struct Cyc_FloatPtr_sa_ScanfArg_struct{int tag;float*f1;};struct Cyc_CharPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};
# 157 "cycboot.h"
int Cyc_printf(struct _fat_ptr,struct _fat_ptr);
# 232 "cycboot.h"
struct _fat_ptr Cyc_vrprintf(struct _RegionHandle*,struct _fat_ptr,struct _fat_ptr);extern char Cyc_FileCloseError[15U];struct Cyc_FileCloseError_exn_struct{char*tag;};extern char Cyc_FileOpenError[14U];struct Cyc_FileOpenError_exn_struct{char*tag;struct _fat_ptr f1;};
# 38 "string.h"
unsigned long Cyc_strlen(struct _fat_ptr s);
# 49 "string.h"
int Cyc_strcmp(struct _fat_ptr s1,struct _fat_ptr s2);struct Cyc_PP_Ppstate;struct Cyc_PP_Out;struct Cyc_PP_Doc;struct Cyc_Position_Error;struct Cyc_Iter_Iter{void*env;int(*next)(void*env,void*dest);};struct Cyc_Dict_T;struct Cyc_Dict_Dict{int(*rel)(void*,void*);struct _RegionHandle*r;const struct Cyc_Dict_T*t;};extern char Cyc_Dict_Present[8U];struct Cyc_Dict_Present_exn_struct{char*tag;};extern char Cyc_Dict_Absent[7U];struct Cyc_Dict_Absent_exn_struct{char*tag;};struct Cyc_Relations_Reln;struct _union_Nmspace_Rel_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Abs_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_C_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Loc_n{int tag;int val;};union Cyc_Absyn_Nmspace{struct _union_Nmspace_Rel_n Rel_n;struct _union_Nmspace_Abs_n Abs_n;struct _union_Nmspace_C_n C_n;struct _union_Nmspace_Loc_n Loc_n;};struct _tuple1{union Cyc_Absyn_Nmspace f1;struct _fat_ptr*f2;};
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
enum Cyc_Absyn_KindQual{Cyc_Absyn_AnyKind =0U,Cyc_Absyn_MemKind =1U,Cyc_Absyn_BoxKind =2U,Cyc_Absyn_XRgnKind =3U,Cyc_Absyn_RgnKind =4U,Cyc_Absyn_EffKind =5U,Cyc_Absyn_IntKind =6U,Cyc_Absyn_BoolKind =7U,Cyc_Absyn_PtrBndKind =8U};struct Cyc_Absyn_Kind{enum Cyc_Absyn_KindQual kind;enum Cyc_Absyn_AliasQual aliasqual;};struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct{int tag;struct Cyc_Absyn_Kind*f1;};struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct{int tag;struct Cyc_Core_Opt*f1;};struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct{int tag;struct Cyc_Core_Opt*f1;struct Cyc_Absyn_Kind*f2;};struct Cyc_Absyn_Tvar{struct _fat_ptr*name;int identity;void*kind;};struct Cyc_Absyn_PtrLoc{unsigned ptr_loc;unsigned rgn_loc;unsigned zt_loc;};struct Cyc_Absyn_PtrAtts{void*rgn;void*nullable;void*bounds;void*zero_term;struct Cyc_Absyn_PtrLoc*ptrloc;};struct Cyc_Absyn_PtrInfo{void*elt_type;struct Cyc_Absyn_Tqual elt_tq;struct Cyc_Absyn_PtrAtts ptr_atts;};struct Cyc_Absyn_VarargInfo{struct _fat_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;int inject;};struct Cyc_Absyn_Star_Absyn_Cap_struct{int tag;};struct Cyc_Absyn_Bot_Absyn_Cap_struct{int tag;};struct Cyc_Absyn_Nat_Absyn_Cap_struct{int tag;int f1;int f2;};struct Cyc_Absyn_RgnEffect{void*name;struct Cyc_List_List*caps;void*parent;};struct Cyc_Absyn_FnInfo{struct Cyc_List_List*tvars;void*effect;struct Cyc_Absyn_Tqual ret_tqual;void*ret_type;struct Cyc_List_List*args;int c_varargs;struct Cyc_Absyn_VarargInfo*cyc_varargs;struct Cyc_List_List*rgn_po;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*requires_clause;struct Cyc_List_List*requires_relns;struct Cyc_Absyn_Exp*ensures_clause;struct Cyc_List_List*ensures_relns;struct Cyc_List_List*ieffect;struct Cyc_List_List*oeffect;struct Cyc_List_List*throws;int reentrant;};struct Cyc_Absyn_UnknownDatatypeInfo{struct _tuple1*name;int is_extensible;};struct _union_DatatypeInfo_UnknownDatatype{int tag;struct Cyc_Absyn_UnknownDatatypeInfo val;};struct _union_DatatypeInfo_KnownDatatype{int tag;struct Cyc_Absyn_Datatypedecl**val;};union Cyc_Absyn_DatatypeInfo{struct _union_DatatypeInfo_UnknownDatatype UnknownDatatype;struct _union_DatatypeInfo_KnownDatatype KnownDatatype;};struct Cyc_Absyn_UnknownDatatypeFieldInfo{struct _tuple1*datatype_name;struct _tuple1*field_name;int is_extensible;};struct _union_DatatypeFieldInfo_UnknownDatatypefield{int tag;struct Cyc_Absyn_UnknownDatatypeFieldInfo val;};struct _tuple2{struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;};struct _union_DatatypeFieldInfo_KnownDatatypefield{int tag;struct _tuple2 val;};union Cyc_Absyn_DatatypeFieldInfo{struct _union_DatatypeFieldInfo_UnknownDatatypefield UnknownDatatypefield;struct _union_DatatypeFieldInfo_KnownDatatypefield KnownDatatypefield;};
# 360
union Cyc_Absyn_DatatypeFieldInfo Cyc_Absyn_KnownDatatypefield(struct Cyc_Absyn_Datatypedecl*,struct Cyc_Absyn_Datatypefield*);struct _tuple3{enum Cyc_Absyn_AggrKind f1;struct _tuple1*f2;struct Cyc_Core_Opt*f3;};struct _union_AggrInfo_UnknownAggr{int tag;struct _tuple3 val;};struct _union_AggrInfo_KnownAggr{int tag;struct Cyc_Absyn_Aggrdecl**val;};union Cyc_Absyn_AggrInfo{struct _union_AggrInfo_UnknownAggr UnknownAggr;struct _union_AggrInfo_KnownAggr KnownAggr;};
# 367
union Cyc_Absyn_AggrInfo Cyc_Absyn_KnownAggr(struct Cyc_Absyn_Aggrdecl**);struct Cyc_Absyn_ArrayInfo{void*elt_type;struct Cyc_Absyn_Tqual tq;struct Cyc_Absyn_Exp*num_elts;void*zero_term;unsigned zt_loc;};struct Cyc_Absyn_Aggr_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Enum_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;};struct Cyc_Absyn_Datatype_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;};struct Cyc_Absyn_TypeDecl{void*r;unsigned loc;};struct Cyc_Absyn_VoidCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_IntCon_Absyn_TyCon_struct{int tag;enum Cyc_Absyn_Sign f1;enum Cyc_Absyn_Size_of f2;};struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct{int tag;int f1;};struct Cyc_Absyn_RgnHandleCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_XRgnHandleCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_TagCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_HeapCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_UniqueCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_RefCntCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_AccessCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_CAccessCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_JoinCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_RgnsCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_TrueCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_FalseCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_ThinCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_FatCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct{int tag;struct _tuple1*f1;struct Cyc_Absyn_Enumdecl*f2;};struct Cyc_Absyn_AnonEnumCon_Absyn_TyCon_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_BuiltinCon_Absyn_TyCon_struct{int tag;struct _fat_ptr f1;struct Cyc_Absyn_Kind*f2;};struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_DatatypeInfo f1;};struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_DatatypeFieldInfo f1;};struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_AggrInfo f1;};struct Cyc_Absyn_AppType_Absyn_Type_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Evar_Absyn_Type_struct{int tag;struct Cyc_Core_Opt*f1;void*f2;int f3;struct Cyc_Core_Opt*f4;};struct Cyc_Absyn_VarType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Tvar*f1;};struct Cyc_Absyn_PointerType_Absyn_Type_struct{int tag;struct Cyc_Absyn_PtrInfo f1;};struct Cyc_Absyn_ArrayType_Absyn_Type_struct{int tag;struct Cyc_Absyn_ArrayInfo f1;};struct Cyc_Absyn_FnType_Absyn_Type_struct{int tag;struct Cyc_Absyn_FnInfo f1;};struct Cyc_Absyn_TupleType_Absyn_Type_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct{int tag;enum Cyc_Absyn_AggrKind f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_TypedefType_Absyn_Type_struct{int tag;struct _tuple1*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_Typedefdecl*f3;void*f4;};struct Cyc_Absyn_ValueofType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct{int tag;struct Cyc_Absyn_TypeDecl*f1;void**f2;};struct Cyc_Absyn_TypeofType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_NoTypes_Absyn_Funcparams_struct{int tag;struct Cyc_List_List*f1;unsigned f2;};struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct{int tag;struct Cyc_List_List*f1;int f2;struct Cyc_Absyn_VarargInfo*f3;void*f4;struct Cyc_List_List*f5;struct Cyc_Absyn_Exp*f6;struct Cyc_Absyn_Exp*f7;struct Cyc_List_List*f8;struct Cyc_List_List*f9;struct Cyc_List_List*f10;int f11;};
# 478 "absyn.h"
enum Cyc_Absyn_Format_Type{Cyc_Absyn_Printf_ft =0U,Cyc_Absyn_Scanf_ft =1U};struct Cyc_Absyn_Regparm_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Stdcall_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Cdecl_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Fastcall_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Noreturn_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Const_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Aligned_att_Absyn_Attribute_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Packed_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Section_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Nocommon_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Shared_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Unused_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Weak_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Dllimport_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Dllexport_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_instrument_function_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Constructor_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Destructor_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_check_memory_usage_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Format_att_Absyn_Attribute_struct{int tag;enum Cyc_Absyn_Format_Type f1;int f2;int f3;};struct Cyc_Absyn_Initializes_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Noliveunique_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Consume_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Pure_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Mode_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Alias_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Always_inline_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_throw_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Non_null_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Deprecated_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_VectorSize_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Carray_mod_Absyn_Type_modifier_struct{int tag;void*f1;unsigned f2;};struct Cyc_Absyn_ConstArray_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;unsigned f3;};struct Cyc_Absyn_Pointer_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_Absyn_PtrAtts f1;struct Cyc_Absyn_Tqual f2;};struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct{int tag;void*f1;};struct Cyc_Absyn_TypeParams_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_List_List*f1;unsigned f2;int f3;};struct Cyc_Absyn_Attributes_mod_Absyn_Type_modifier_struct{int tag;unsigned f1;struct Cyc_List_List*f2;};struct _union_Cnst_Null_c{int tag;int val;};struct _tuple4{enum Cyc_Absyn_Sign f1;char f2;};struct _union_Cnst_Char_c{int tag;struct _tuple4 val;};struct _union_Cnst_Wchar_c{int tag;struct _fat_ptr val;};struct _tuple5{enum Cyc_Absyn_Sign f1;short f2;};struct _union_Cnst_Short_c{int tag;struct _tuple5 val;};struct _tuple6{enum Cyc_Absyn_Sign f1;int f2;};struct _union_Cnst_Int_c{int tag;struct _tuple6 val;};struct _tuple7{enum Cyc_Absyn_Sign f1;long long f2;};struct _union_Cnst_LongLong_c{int tag;struct _tuple7 val;};struct _tuple8{struct _fat_ptr f1;int f2;};struct _union_Cnst_Float_c{int tag;struct _tuple8 val;};struct _union_Cnst_String_c{int tag;struct _fat_ptr val;};struct _union_Cnst_Wstring_c{int tag;struct _fat_ptr val;};union Cyc_Absyn_Cnst{struct _union_Cnst_Null_c Null_c;struct _union_Cnst_Char_c Char_c;struct _union_Cnst_Wchar_c Wchar_c;struct _union_Cnst_Short_c Short_c;struct _union_Cnst_Int_c Int_c;struct _union_Cnst_LongLong_c LongLong_c;struct _union_Cnst_Float_c Float_c;struct _union_Cnst_String_c String_c;struct _union_Cnst_Wstring_c Wstring_c;};
# 564
union Cyc_Absyn_Cnst Cyc_Absyn_Char_c(enum Cyc_Absyn_Sign,char);
# 566
union Cyc_Absyn_Cnst Cyc_Absyn_Short_c(enum Cyc_Absyn_Sign,short);
union Cyc_Absyn_Cnst Cyc_Absyn_Int_c(enum Cyc_Absyn_Sign,int);
# 574
enum Cyc_Absyn_Primop{Cyc_Absyn_Plus =0U,Cyc_Absyn_Times =1U,Cyc_Absyn_Minus =2U,Cyc_Absyn_Div =3U,Cyc_Absyn_Mod =4U,Cyc_Absyn_Eq =5U,Cyc_Absyn_Neq =6U,Cyc_Absyn_Gt =7U,Cyc_Absyn_Lt =8U,Cyc_Absyn_Gte =9U,Cyc_Absyn_Lte =10U,Cyc_Absyn_Not =11U,Cyc_Absyn_Bitnot =12U,Cyc_Absyn_Bitand =13U,Cyc_Absyn_Bitor =14U,Cyc_Absyn_Bitxor =15U,Cyc_Absyn_Bitlshift =16U,Cyc_Absyn_Bitlrshift =17U,Cyc_Absyn_Numelts =18U};
# 581
enum Cyc_Absyn_Incrementor{Cyc_Absyn_PreInc =0U,Cyc_Absyn_PostInc =1U,Cyc_Absyn_PreDec =2U,Cyc_Absyn_PostDec =3U};struct Cyc_Absyn_VarargCallInfo{int num_varargs;struct Cyc_List_List*injectors;struct Cyc_Absyn_VarargInfo*vai;};struct Cyc_Absyn_StructField_Absyn_OffsetofField_struct{int tag;struct _fat_ptr*f1;};struct Cyc_Absyn_TupleIndex_Absyn_OffsetofField_struct{int tag;unsigned f1;};
# 599
enum Cyc_Absyn_Coercion{Cyc_Absyn_Unknown_coercion =0U,Cyc_Absyn_No_coercion =1U,Cyc_Absyn_Null_to_NonNull =2U,Cyc_Absyn_Other_coercion =3U};struct Cyc_Absyn_MallocInfo{int is_calloc;struct Cyc_Absyn_Exp*rgn;void**elt_type;struct Cyc_Absyn_Exp*num_elts;int fat_result;int inline_call;};struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct{int tag;union Cyc_Absyn_Cnst f1;};struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Pragma_e_Absyn_Raw_exp_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct{int tag;enum Cyc_Absyn_Primop f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;enum Cyc_Absyn_Incrementor f2;};struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct _tuple9{struct Cyc_List_List*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_VarargCallInfo*f3;int f4;struct _tuple9*f5;};struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;int f2;};struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Exp*f2;int f3;enum Cyc_Absyn_Coercion f4;};struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;int f3;int f4;};struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;int f3;int f4;};struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Tuple_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;};struct _tuple10{struct _fat_ptr*f1;struct Cyc_Absyn_Tqual f2;void*f3;};struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct{int tag;struct _tuple10*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;int f4;};struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;int f3;};struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct{int tag;struct _tuple1*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct Cyc_Absyn_Aggrdecl*f4;};struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Datatypedecl*f2;struct Cyc_Absyn_Datatypefield*f3;};struct Cyc_Absyn_Enum_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_Spawn_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;struct _tuple9*f3;};struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_MallocInfo f1;};struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Core_Opt*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Stmt*f1;};struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;};struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct{int tag;int f1;struct _fat_ptr f2;struct Cyc_List_List*f3;struct Cyc_List_List*f4;struct Cyc_List_List*f5;};struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Exp{void*topt;void*r;unsigned loc;void*annot;};struct Cyc_Absyn_Skip_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;struct Cyc_Absyn_Stmt*f3;};struct _tuple11{struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct{int tag;struct _tuple11 f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Break_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Continue_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Goto_s_Absyn_Raw_stmt_struct{int tag;struct _fat_ptr*f1;};struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _tuple11 f2;struct _tuple11 f3;struct Cyc_Absyn_Stmt*f4;};struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;void*f3;};struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Switch_clause**f2;};struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Decl*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct{int tag;struct _fat_ptr*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct _tuple11 f2;};struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_List_List*f2;void*f3;};struct Cyc_Absyn_Stmt{void*r;unsigned loc;void*annot;};struct Cyc_Absyn_Wild_p_Absyn_Raw_pat_struct{int tag;};struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_AliasVar_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_TagInt_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Tuple_p_Absyn_Raw_pat_struct{int tag;struct Cyc_List_List*f1;int f2;};struct Cyc_Absyn_Pointer_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Pat*f1;};struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct{int tag;union Cyc_Absyn_AggrInfo*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;int f4;};struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;struct Cyc_List_List*f3;int f4;};struct Cyc_Absyn_Null_p_Absyn_Raw_pat_struct{int tag;};struct Cyc_Absyn_Int_p_Absyn_Raw_pat_struct{int tag;enum Cyc_Absyn_Sign f1;int f2;};struct Cyc_Absyn_Char_p_Absyn_Raw_pat_struct{int tag;char f1;};struct Cyc_Absyn_Float_p_Absyn_Raw_pat_struct{int tag;struct _fat_ptr f1;int f2;};struct Cyc_Absyn_Enum_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_AnonEnum_p_Absyn_Raw_pat_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_UnknownId_p_Absyn_Raw_pat_struct{int tag;struct _tuple1*f1;};struct Cyc_Absyn_UnknownCall_p_Absyn_Raw_pat_struct{int tag;struct _tuple1*f1;struct Cyc_List_List*f2;int f3;};struct Cyc_Absyn_Exp_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Pat{void*r;void*topt;unsigned loc;};struct Cyc_Absyn_Switch_clause{struct Cyc_Absyn_Pat*pattern;struct Cyc_Core_Opt*pat_vars;struct Cyc_Absyn_Exp*where_clause;struct Cyc_Absyn_Stmt*body;unsigned loc;};struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct{int tag;struct _tuple1*f1;};struct Cyc_Absyn_Global_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Funname_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Param_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Local_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Pat_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Vardecl{enum Cyc_Absyn_Scope sc;struct _tuple1*name;unsigned varloc;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*initializer;void*rgn;struct Cyc_List_List*attributes;int escapes;};struct Cyc_Absyn_Fndecl{enum Cyc_Absyn_Scope sc;int is_inline;struct _tuple1*name;struct Cyc_Absyn_Stmt*body;struct Cyc_Absyn_FnInfo i;void*cached_type;struct Cyc_Core_Opt*param_vardecls;struct Cyc_Absyn_Vardecl*fn_vardecl;};struct Cyc_Absyn_Aggrfield{struct _fat_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*width;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*requires_clause;};struct Cyc_Absyn_AggrdeclImpl{struct Cyc_List_List*exist_vars;struct Cyc_List_List*rgn_po;struct Cyc_List_List*fields;int tagged;};struct Cyc_Absyn_Aggrdecl{enum Cyc_Absyn_AggrKind kind;enum Cyc_Absyn_Scope sc;struct _tuple1*name;struct Cyc_List_List*tvs;struct Cyc_Absyn_AggrdeclImpl*impl;struct Cyc_List_List*attributes;int expected_mem_kind;};struct Cyc_Absyn_Datatypefield{struct _tuple1*name;struct Cyc_List_List*typs;unsigned loc;enum Cyc_Absyn_Scope sc;};struct Cyc_Absyn_Datatypedecl{enum Cyc_Absyn_Scope sc;struct _tuple1*name;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*fields;int is_extensible;};struct Cyc_Absyn_Enumfield{struct _tuple1*name;struct Cyc_Absyn_Exp*tag;unsigned loc;};struct Cyc_Absyn_Enumdecl{enum Cyc_Absyn_Scope sc;struct _tuple1*name;struct Cyc_Core_Opt*fields;};struct Cyc_Absyn_Typedefdecl{struct _tuple1*name;struct Cyc_Absyn_Tqual tq;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*kind;void*defn;struct Cyc_List_List*atts;int extern_c;};struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Pat*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;void*f4;};struct Cyc_Absyn_Letv_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;};struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;};struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Typedefdecl*f1;};struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct{int tag;struct _fat_ptr*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct{int tag;struct _tuple1*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_ExternC_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;};struct _tuple12{unsigned f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct _tuple12*f4;};struct Cyc_Absyn_Porton_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Portoff_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Tempeston_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Tempestoff_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Decl{void*r;unsigned loc;};struct Cyc_Absyn_ArrayElement_Absyn_Designator_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_FieldName_Absyn_Designator_struct{int tag;struct _fat_ptr*f1;};extern char Cyc_Absyn_EmptyAnnot[11U];struct Cyc_Absyn_EmptyAnnot_Absyn_AbsynAnnot_struct{char*tag;};
# 962 "absyn.h"
struct Cyc_Absyn_Tqual Cyc_Absyn_const_tqual(unsigned);
struct Cyc_Absyn_Tqual Cyc_Absyn_empty_tqual(unsigned);
# 979
void*Cyc_Absyn_new_evar(struct Cyc_Core_Opt*k,struct Cyc_Core_Opt*tenv);
# 981
void*Cyc_Absyn_wildtyp(struct Cyc_Core_Opt*);
# 984
extern void*Cyc_Absyn_char_type;extern void*Cyc_Absyn_uchar_type;extern void*Cyc_Absyn_ushort_type;extern void*Cyc_Absyn_uint_type;extern void*Cyc_Absyn_ulonglong_type;
# 986
extern void*Cyc_Absyn_schar_type;extern void*Cyc_Absyn_sshort_type;extern void*Cyc_Absyn_sint_type;extern void*Cyc_Absyn_slonglong_type;
# 988
extern void*Cyc_Absyn_double_type;void*Cyc_Absyn_wchar_type();
void*Cyc_Absyn_gen_float_type(unsigned i);
# 991
extern void*Cyc_Absyn_heap_rgn_type;extern void*Cyc_Absyn_unique_rgn_type;
# 995
extern void*Cyc_Absyn_true_type;extern void*Cyc_Absyn_false_type;
# 997
extern void*Cyc_Absyn_void_type;void*Cyc_Absyn_tag_type(void*);void*Cyc_Absyn_rgn_handle_type(void*);void*Cyc_Absyn_enum_type(struct _tuple1*n,struct Cyc_Absyn_Enumdecl*d);
# 1014
void*Cyc_Absyn_exn_type();
# 1022
extern void*Cyc_Absyn_fat_bound_type;
# 1024
void*Cyc_Absyn_thin_bounds_exp(struct Cyc_Absyn_Exp*);
# 1026
void*Cyc_Absyn_bounds_one();
# 1033
void*Cyc_Absyn_atb_type(void*t,void*rgn,struct Cyc_Absyn_Tqual tq,void*b,void*zero_term);
# 1036
void*Cyc_Absyn_star_type(void*t,void*rgn,struct Cyc_Absyn_Tqual,void*zero_term);
# 1038
void*Cyc_Absyn_at_type(void*t,void*rgn,struct Cyc_Absyn_Tqual,void*zero_term);
# 1042
void*Cyc_Absyn_fatptr_type(void*t,void*rgn,struct Cyc_Absyn_Tqual,void*zeroterm);
# 1050
void*Cyc_Absyn_array_type(void*elt_type,struct Cyc_Absyn_Tqual tq,struct Cyc_Absyn_Exp*num_elts,void*zero_term,unsigned ztloc);
# 1054
void*Cyc_Absyn_datatype_field_type(union Cyc_Absyn_DatatypeFieldInfo,struct Cyc_List_List*args);
void*Cyc_Absyn_aggr_type(union Cyc_Absyn_AggrInfo,struct Cyc_List_List*args);
# 1058
struct Cyc_Absyn_Exp*Cyc_Absyn_new_exp(void*,unsigned);
# 1060
struct Cyc_Absyn_Exp*Cyc_Absyn_copy_exp(struct Cyc_Absyn_Exp*);
struct Cyc_Absyn_Exp*Cyc_Absyn_const_exp(union Cyc_Absyn_Cnst,unsigned);
# 1068
struct Cyc_Absyn_Exp*Cyc_Absyn_uint_exp(unsigned,unsigned);
# 1082
struct Cyc_Absyn_Exp*Cyc_Absyn_add_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
# 1104
struct Cyc_Absyn_Exp*Cyc_Absyn_cast_exp(void*,struct Cyc_Absyn_Exp*,int user_cast,enum Cyc_Absyn_Coercion,unsigned);
# 1106
struct Cyc_Absyn_Exp*Cyc_Absyn_sizeoftype_exp(void*t,unsigned);
# 1114
struct Cyc_Absyn_Exp*Cyc_Absyn_stmt_exp(struct Cyc_Absyn_Stmt*,unsigned);
# 1117
struct Cyc_Absyn_Exp*Cyc_Absyn_valueof_exp(void*,unsigned);
# 1125
struct Cyc_Absyn_Exp*Cyc_Absyn_uniquergn_exp();
# 1129
struct Cyc_Absyn_Stmt*Cyc_Absyn_exp_stmt(struct Cyc_Absyn_Exp*,unsigned);
# 1140
struct Cyc_Absyn_Stmt*Cyc_Absyn_decl_stmt(struct Cyc_Absyn_Decl*,struct Cyc_Absyn_Stmt*,unsigned);
# 1198
void*Cyc_Absyn_pointer_expand(void*,int fresh_evar);
# 1200
int Cyc_Absyn_is_lvalue(struct Cyc_Absyn_Exp*);
# 1203
struct Cyc_Absyn_Aggrfield*Cyc_Absyn_lookup_field(struct Cyc_List_List*,struct _fat_ptr*);
# 1205
struct Cyc_Absyn_Aggrfield*Cyc_Absyn_lookup_decl_field(struct Cyc_Absyn_Aggrdecl*,struct _fat_ptr*);
# 1376
int Cyc_Absyn_is_nothrow(struct Cyc_List_List*t);
# 1399
int Cyc_Absyn_is_reentrant(int);
# 1408
int Cyc_Absyn_forgive_global();struct Cyc_Absynpp_Params{int expand_typedefs;int qvar_to_Cids;int add_cyc_prefix;int to_VC;int decls_first;int rewrite_temp_tvars;int print_all_tvars;int print_all_kinds;int print_all_effects;int print_using_stmts;int print_externC_stmts;int print_full_evars;int print_zeroterm;int generate_line_directives;int use_curr_namespace;struct Cyc_List_List*curr_namespace;};
# 62 "absynpp.h"
struct _fat_ptr Cyc_Absynpp_typ2string(void*);
# 67
struct _fat_ptr Cyc_Absynpp_exp2string(struct Cyc_Absyn_Exp*);
struct _fat_ptr Cyc_Absynpp_stmt2string(struct Cyc_Absyn_Stmt*);
struct _fat_ptr Cyc_Absynpp_qvar2string(struct _tuple1*);
# 27 "unify.h"
void Cyc_Unify_explain_failure();
# 29
int Cyc_Unify_unify(void*,void*);struct _union_RelnOp_RConst{int tag;unsigned val;};struct _union_RelnOp_RVar{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RNumelts{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RType{int tag;void*val;};struct _union_RelnOp_RParam{int tag;unsigned val;};struct _union_RelnOp_RParamNumelts{int tag;unsigned val;};struct _union_RelnOp_RReturn{int tag;unsigned val;};union Cyc_Relations_RelnOp{struct _union_RelnOp_RConst RConst;struct _union_RelnOp_RVar RVar;struct _union_RelnOp_RNumelts RNumelts;struct _union_RelnOp_RType RType;struct _union_RelnOp_RParam RParam;struct _union_RelnOp_RParamNumelts RParamNumelts;struct _union_RelnOp_RReturn RReturn;};
# 50 "relations-ap.h"
enum Cyc_Relations_Relation{Cyc_Relations_Req =0U,Cyc_Relations_Rneq =1U,Cyc_Relations_Rlte =2U,Cyc_Relations_Rlt =3U};struct Cyc_Relations_Reln{union Cyc_Relations_RelnOp rop1;enum Cyc_Relations_Relation relation;union Cyc_Relations_RelnOp rop2;};struct Cyc_RgnOrder_RgnPO;extern char Cyc_Tcenv_Env_error[10U];struct Cyc_Tcenv_Env_error_exn_struct{char*tag;};struct Cyc_Tcenv_Genv{struct Cyc_Dict_Dict aggrdecls;struct Cyc_Dict_Dict datatypedecls;struct Cyc_Dict_Dict enumdecls;struct Cyc_Dict_Dict typedefs;struct Cyc_Dict_Dict ordinaries;};struct Cyc_Tcenv_Fenv;struct Cyc_Tcenv_Tenv{struct Cyc_List_List*ns;struct Cyc_Tcenv_Genv*ae;struct Cyc_Tcenv_Fenv*le;int allow_valueof: 1;int in_extern_c_include: 1;int in_tempest: 1;int tempest_generalize: 1;};
# 77 "tcenv.h"
void*Cyc_Tcenv_lookup_ordinary_global(struct Cyc_Tcenv_Tenv*,unsigned,struct _tuple1*,int is_use);
struct Cyc_Absyn_Aggrdecl**Cyc_Tcenv_lookup_aggrdecl(struct Cyc_Tcenv_Tenv*,unsigned,struct _tuple1*);
struct Cyc_Absyn_Datatypedecl**Cyc_Tcenv_lookup_datatypedecl(struct Cyc_Tcenv_Tenv*,unsigned,struct _tuple1*);
# 84
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_allow_valueof(struct Cyc_Tcenv_Tenv*);
# 89
int Cyc_Tcenv_is_reentrant_fun(struct Cyc_Tcenv_Tenv*);
enum Cyc_Tcenv_NewStatus{Cyc_Tcenv_NoneNew =0U,Cyc_Tcenv_InNew =1U,Cyc_Tcenv_InNewAggr =2U};
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_set_new_status(enum Cyc_Tcenv_NewStatus,struct Cyc_Tcenv_Tenv*);
enum Cyc_Tcenv_NewStatus Cyc_Tcenv_new_status(struct Cyc_Tcenv_Tenv*);
# 94
int Cyc_Tcenv_abstract_val_ok(struct Cyc_Tcenv_Tenv*);
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_enter_abstract_val_ok(struct Cyc_Tcenv_Tenv*);
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_clear_abstract_val_ok(struct Cyc_Tcenv_Tenv*);
# 100
struct Cyc_List_List*Cyc_Tcenv_lookup_type_vars(struct Cyc_Tcenv_Tenv*);
struct Cyc_Core_Opt*Cyc_Tcenv_lookup_opt_type_vars(struct Cyc_Tcenv_Tenv*);
# 116
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_enter_notreadctxt(struct Cyc_Tcenv_Tenv*);
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_clear_notreadctxt(struct Cyc_Tcenv_Tenv*);
int Cyc_Tcenv_in_notreadctxt(struct Cyc_Tcenv_Tenv*);
# 121
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_enter_lhs(struct Cyc_Tcenv_Tenv*);
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_clear_lhs(struct Cyc_Tcenv_Tenv*);
# 125
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_enter_stmt_exp(struct Cyc_Tcenv_Tenv*);
# 132
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_new_block(unsigned,struct Cyc_Tcenv_Tenv*);
# 139
void*Cyc_Tcenv_curr_rgn(struct Cyc_Tcenv_Tenv*);
# 143
void Cyc_Tcenv_check_rgn_accessible(struct Cyc_Tcenv_Tenv*,unsigned,void*rgn);
# 145
void Cyc_Tcenv_check_effect_accessible(struct Cyc_Tcenv_Tenv*,unsigned,void*eff);
# 148
void Cyc_Tcenv_check_rgn_partial_order(struct Cyc_Tcenv_Tenv*,unsigned,struct Cyc_List_List*po);
# 30 "tcutil.h"
void*Cyc_Tcutil_impos(struct _fat_ptr,struct _fat_ptr);
void Cyc_Tcutil_terr(unsigned,struct _fat_ptr,struct _fat_ptr);
void Cyc_Tcutil_warn(unsigned,struct _fat_ptr,struct _fat_ptr);
# 44
int Cyc_Tcutil_is_arithmetic_type(void*);
# 47
int Cyc_Tcutil_is_pointer_type(void*);
int Cyc_Tcutil_is_array_type(void*);
int Cyc_Tcutil_is_boxed(void*);
# 54
int Cyc_Tcutil_is_nullable_pointer_type(void*);
int Cyc_Tcutil_is_bound_one(void*);
# 57
int Cyc_Tcutil_is_fat_pointer_type(void*);
# 60
int Cyc_Tcutil_is_bits_only_type(void*);
# 67
void*Cyc_Tcutil_pointer_elt_type(void*);
# 69
void*Cyc_Tcutil_pointer_region(void*);
# 72
int Cyc_Tcutil_rgn_of_pointer(void*t,void**rgn);
# 75
struct Cyc_Absyn_Exp*Cyc_Tcutil_get_bounds_exp(void*def,void*b);
# 80
int Cyc_Tcutil_is_fat_pointer_type_elt(void*t,void**elt_dest);
# 82
int Cyc_Tcutil_is_zero_pointer_type_elt(void*t,void**elt_type_dest);
# 87
struct Cyc_Absyn_Exp*Cyc_Tcutil_get_bounds_exp(void*def,void*b);
# 90
int Cyc_Tcutil_is_integral(struct Cyc_Absyn_Exp*);
int Cyc_Tcutil_is_numeric(struct Cyc_Absyn_Exp*);
int Cyc_Tcutil_is_zero(struct Cyc_Absyn_Exp*);
# 97
void*Cyc_Tcutil_copy_type(void*);
# 100
struct Cyc_Absyn_Exp*Cyc_Tcutil_deep_copy_exp(int preserve_types,struct Cyc_Absyn_Exp*);
# 103
int Cyc_Tcutil_kind_leq(struct Cyc_Absyn_Kind*,struct Cyc_Absyn_Kind*);
# 108
struct Cyc_Absyn_Kind*Cyc_Tcutil_tvar_kind(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Kind*def);
struct Cyc_Absyn_Kind*Cyc_Tcutil_type_kind(void*);
void*Cyc_Tcutil_compress(void*);
void Cyc_Tcutil_unchecked_cast(struct Cyc_Absyn_Exp*,void*,enum Cyc_Absyn_Coercion);
int Cyc_Tcutil_coerce_uint_type(struct Cyc_Absyn_Exp*);
int Cyc_Tcutil_coerce_sint_type(struct Cyc_Absyn_Exp*);
int Cyc_Tcutil_coerce_to_bool(struct Cyc_Absyn_Exp*);
# 116
int Cyc_Tcutil_coerce_arg(struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Exp*,void*,int*alias_coercion);
int Cyc_Tcutil_coerce_assign(struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Exp*,void*);
int Cyc_Tcutil_coerce_list(struct Cyc_Tcenv_Tenv*te,void*,struct Cyc_List_List*);
# 120
int Cyc_Tcutil_silent_castable(struct Cyc_Tcenv_Tenv*te,unsigned,void*,void*);
# 122
enum Cyc_Absyn_Coercion Cyc_Tcutil_castable(struct Cyc_Tcenv_Tenv*te,unsigned,void*,void*);
# 127
int Cyc_Tcutil_zero_to_null(void*,struct Cyc_Absyn_Exp*);struct _tuple13{struct Cyc_Absyn_Decl*f1;struct Cyc_Absyn_Exp*f2;};
# 130
struct _tuple13 Cyc_Tcutil_insert_alias(struct Cyc_Absyn_Exp*e,void*e_typ);
# 140
extern struct Cyc_Absyn_Kind Cyc_Tcutil_ak;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_bk;
# 149
extern struct Cyc_Absyn_Kind Cyc_Tcutil_tak;
# 151
extern struct Cyc_Absyn_Kind Cyc_Tcutil_tmk;
# 158
extern struct Cyc_Core_Opt Cyc_Tcutil_rko;
extern struct Cyc_Core_Opt Cyc_Tcutil_ako;
extern struct Cyc_Core_Opt Cyc_Tcutil_bko;
# 167
extern struct Cyc_Core_Opt Cyc_Tcutil_trko;
extern struct Cyc_Core_Opt Cyc_Tcutil_tako;
# 170
extern struct Cyc_Core_Opt Cyc_Tcutil_tmko;
# 183
void*Cyc_Tcutil_max_arithmetic_type(void*,void*);
# 189
void*Cyc_Tcutil_substitute(struct Cyc_List_List*,void*);
# 192
struct Cyc_List_List*Cyc_Tcutil_rsubst_rgnpo(struct _RegionHandle*,struct Cyc_List_List*,struct Cyc_List_List*);
# 206
void*Cyc_Tcutil_fndecl2type(struct Cyc_Absyn_Fndecl*);struct _tuple14{struct Cyc_List_List*f1;struct _RegionHandle*f2;};struct _tuple15{struct Cyc_Absyn_Tvar*f1;void*f2;};
# 211
struct _tuple15*Cyc_Tcutil_r_make_inst_var(struct _tuple14*,struct Cyc_Absyn_Tvar*);
# 222
void Cyc_Tcutil_check_nonzero_bound(unsigned,void*);
# 224
void Cyc_Tcutil_check_bound(unsigned,unsigned i,void*,int do_warn);
# 226
struct Cyc_List_List*Cyc_Tcutil_resolve_aggregate_designators(struct _RegionHandle*,unsigned,struct Cyc_List_List*,enum Cyc_Absyn_AggrKind,struct Cyc_List_List*);
# 237
int Cyc_Tcutil_is_noalias_region(void*r,int must_be_unique);
# 240
int Cyc_Tcutil_is_noalias_pointer(void*t,int must_be_unique);
# 245
int Cyc_Tcutil_is_noalias_path(struct Cyc_Absyn_Exp*);
# 250
int Cyc_Tcutil_is_noalias_pointer_or_aggr(void*);struct _tuple16{int f1;void*f2;};
# 254
struct _tuple16 Cyc_Tcutil_addressof_props(struct Cyc_Absyn_Exp*);
# 278
int Cyc_Tcutil_is_const_exp(struct Cyc_Absyn_Exp*);
# 292
void Cyc_Tcutil_check_no_qual(unsigned,void*);
# 303
void*Cyc_Tcutil_promote_array(void*t,void*rgn,int convert_tag);
# 306
int Cyc_Tcutil_zeroable_type(void*);
# 310
int Cyc_Tcutil_force_type2bool(int desired,void*);
# 313
void*Cyc_Tcutil_any_bool(struct Cyc_List_List*);
# 315
void*Cyc_Tcutil_any_bounds(struct Cyc_List_List*);
# 331
int Cyc_Tcutil_is_tagged_union_and_needs_check(struct Cyc_Absyn_Exp*e);struct _tuple17{unsigned f1;int f2;};
# 28 "evexp.h"
struct _tuple17 Cyc_Evexp_eval_const_uint_exp(struct Cyc_Absyn_Exp*e);
# 32
int Cyc_Evexp_c_can_eval(struct Cyc_Absyn_Exp*e);
# 41 "evexp.h"
int Cyc_Evexp_same_const_exp(struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2);
# 47
int Cyc_Evexp_okay_szofarg(void*t);
# 26 "tcstmt.h"
void Cyc_Tcstmt_tcStmt(struct Cyc_Tcenv_Tenv*,struct Cyc_Absyn_Stmt*,int new_block);
# 30 "formatstr.h"
struct Cyc_List_List*Cyc_Formatstr_get_format_types(struct Cyc_Tcenv_Tenv*,struct _fat_ptr,int isCproto,unsigned);
# 33
struct Cyc_List_List*Cyc_Formatstr_get_scanf_types(struct Cyc_Tcenv_Tenv*,struct _fat_ptr,int isCproto,unsigned);
# 31 "tc.h"
extern int Cyc_Tc_aggressive_warn;
# 44 "tctyp.h"
void Cyc_Tctyp_check_type(unsigned,struct Cyc_Tcenv_Tenv*,struct Cyc_List_List*bound_tvars,struct Cyc_Absyn_Kind*k,int allow_evars,int allow_abs_aggr,void*);
# 28 "tcexp.h"
void*Cyc_Tcexp_tcExp(struct Cyc_Tcenv_Tenv*,void**,struct Cyc_Absyn_Exp*);
void*Cyc_Tcexp_tcExpInitializer(struct Cyc_Tcenv_Tenv*,void**,struct Cyc_Absyn_Exp*);
void Cyc_Tcexp_tcTest(struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Exp*e,struct _fat_ptr msg_part);
# 49 "tcexp.cyc"
static void*Cyc_Tcexp_expr_err(struct Cyc_Tcenv_Tenv*te,unsigned loc,void**topt,struct _fat_ptr msg,struct _fat_ptr ap){
# 53
({void*_tmp0=0U;({unsigned _tmp6C9=loc;struct _fat_ptr _tmp6C8=(struct _fat_ptr)({Cyc_vrprintf(Cyc_Core_heap_region,msg,ap);});Cyc_Tcutil_terr(_tmp6C9,_tmp6C8,_tag_fat(_tmp0,sizeof(void*),0U));});});
if(topt == 0)
return({void*(*_tmp1)(struct Cyc_Core_Opt*)=Cyc_Absyn_wildtyp;struct Cyc_Core_Opt*_tmp2=({Cyc_Tcenv_lookup_opt_type_vars(te);});_tmp1(_tmp2);});else{
# 57
return*topt;}}
# 67
static void Cyc_Tcexp_resolve_unresolved_mem(struct Cyc_Tcenv_Tenv*te,unsigned loc,void**topt,struct Cyc_Absyn_Exp*e,struct Cyc_List_List*des){
# 70
if(topt == 0){
# 72
({void*_tmp6CA=(void*)({struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*_tmp4=_cycalloc(sizeof(*_tmp4));_tmp4->tag=26U,_tmp4->f1=des;_tmp4;});e->r=_tmp6CA;});
return;}{
# 70
void*t=*topt;
# 76
void*_stmttmp0=({Cyc_Tcutil_compress(t);});void*_tmp5=_stmttmp0;struct Cyc_Absyn_Tqual _tmp7;void*_tmp6;union Cyc_Absyn_AggrInfo _tmp8;switch(*((int*)_tmp5)){case 0U: if(((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp5)->f1)->tag == 22U){_LL1: _tmp8=((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp5)->f1)->f1;_LL2: {union Cyc_Absyn_AggrInfo info=_tmp8;
# 78
{union Cyc_Absyn_AggrInfo _tmp9=info;struct Cyc_Absyn_Aggrdecl*_tmpA;if((_tmp9.UnknownAggr).tag == 1){_LLA: _LLB:
({void*_tmpB=0U;({int(*_tmp6CC)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmpD)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmpD;});struct _fat_ptr _tmp6CB=({const char*_tmpC="struct type not properly set";_tag_fat(_tmpC,sizeof(char),29U);});_tmp6CC(_tmp6CB,_tag_fat(_tmpB,sizeof(void*),0U));});});}else{_LLC: _tmpA=*(_tmp9.KnownAggr).val;_LLD: {struct Cyc_Absyn_Aggrdecl*ad=_tmpA;
({void*_tmp6CD=(void*)({struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*_tmpE=_cycalloc(sizeof(*_tmpE));_tmpE->tag=29U,_tmpE->f1=ad->name,_tmpE->f2=0,_tmpE->f3=des,_tmpE->f4=ad;_tmpE;});e->r=_tmp6CD;});}}_LL9:;}
# 82
goto _LL0;}}else{goto _LL7;}case 4U: _LL3: _tmp6=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp5)->f1).elt_type;_tmp7=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp5)->f1).tq;_LL4: {void*at=_tmp6;struct Cyc_Absyn_Tqual aq=_tmp7;
({void*_tmp6CE=(void*)({struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*_tmpF=_cycalloc(sizeof(*_tmpF));_tmpF->tag=26U,_tmpF->f1=des;_tmpF;});e->r=_tmp6CE;});goto _LL0;}case 7U: _LL5: _LL6:
({void*_tmp6CF=(void*)({struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct*_tmp10=_cycalloc(sizeof(*_tmp10));_tmp10->tag=30U,_tmp10->f1=t,_tmp10->f2=des;_tmp10;});e->r=_tmp6CF;});goto _LL0;default: _LL7: _LL8:
({void*_tmp6D0=(void*)({struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*_tmp11=_cycalloc(sizeof(*_tmp11));_tmp11->tag=26U,_tmp11->f1=des;_tmp11;});e->r=_tmp6D0;});goto _LL0;}_LL0:;}}
# 67
static void Cyc_Tcexp_tcExpNoInst(struct Cyc_Tcenv_Tenv*te,void**topt,struct Cyc_Absyn_Exp*e);
# 93
static void*Cyc_Tcexp_tcExpNoPromote(struct Cyc_Tcenv_Tenv*te,void**topt,struct Cyc_Absyn_Exp*e);
# 96
static void Cyc_Tcexp_tcExpList(struct Cyc_Tcenv_Tenv*te,struct Cyc_List_List*es){
for(0;es != 0;es=es->tl){
({Cyc_Tcexp_tcExp(te,0,(struct Cyc_Absyn_Exp*)es->hd);});}}
# 102
static void Cyc_Tcexp_check_contains_assign(struct Cyc_Absyn_Exp*e){
void*_stmttmp1=e->r;void*_tmp14=_stmttmp1;if(((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp14)->tag == 4U){if(((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp14)->f2 == 0){_LL1: _LL2:
# 105
 if(Cyc_Tc_aggressive_warn)({void*_tmp15=0U;({unsigned _tmp6D2=e->loc;struct _fat_ptr _tmp6D1=({const char*_tmp16="assignment in test";_tag_fat(_tmp16,sizeof(char),19U);});Cyc_Tcutil_warn(_tmp6D2,_tmp6D1,_tag_fat(_tmp15,sizeof(void*),0U));});});goto _LL0;}else{goto _LL3;}}else{_LL3: _LL4:
# 107
 goto _LL0;}_LL0:;}
# 112
void Cyc_Tcexp_tcTest(struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Exp*e,struct _fat_ptr msg_part){
({Cyc_Tcexp_check_contains_assign(e);});
({Cyc_Tcexp_tcExp(te,& Cyc_Absyn_sint_type,e);});
if(!({Cyc_Tcutil_coerce_to_bool(e);}))
({struct Cyc_String_pa_PrintArg_struct _tmp1A=({struct Cyc_String_pa_PrintArg_struct _tmp63B;_tmp63B.tag=0U,_tmp63B.f1=(struct _fat_ptr)((struct _fat_ptr)msg_part);_tmp63B;});struct Cyc_String_pa_PrintArg_struct _tmp1B=({struct Cyc_String_pa_PrintArg_struct _tmp63A;_tmp63A.tag=0U,({
struct _fat_ptr _tmp6D3=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string((void*)_check_null(e->topt));}));_tmp63A.f1=_tmp6D3;});_tmp63A;});void*_tmp18[2U];_tmp18[0]=& _tmp1A,_tmp18[1]=& _tmp1B;({unsigned _tmp6D5=e->loc;struct _fat_ptr _tmp6D4=({const char*_tmp19="test of %s has type %s instead of integral or pointer type";_tag_fat(_tmp19,sizeof(char),59U);});Cyc_Tcutil_terr(_tmp6D5,_tmp6D4,_tag_fat(_tmp18,sizeof(void*),2U));});});}
# 138 "tcexp.cyc"
static int Cyc_Tcexp_wchar_numelts(struct _fat_ptr s){
return 1;}
# 143
static void*Cyc_Tcexp_tcConst(struct Cyc_Tcenv_Tenv*te,unsigned loc,void**topt,union Cyc_Absyn_Cnst*c,struct Cyc_Absyn_Exp*e){
void*t;
void*string_elt_typ=Cyc_Absyn_char_type;
int string_numelts=0;
{union Cyc_Absyn_Cnst _stmttmp2=*((union Cyc_Absyn_Cnst*)_check_null(c));union Cyc_Absyn_Cnst _tmp1E=_stmttmp2;struct _fat_ptr _tmp1F;struct _fat_ptr _tmp20;int _tmp22;enum Cyc_Absyn_Sign _tmp21;int _tmp23;enum Cyc_Absyn_Sign _tmp24;enum Cyc_Absyn_Sign _tmp25;switch((_tmp1E.Wstring_c).tag){case 2U: switch(((_tmp1E.Char_c).val).f1){case Cyc_Absyn_Signed: _LL1: _LL2:
 t=Cyc_Absyn_schar_type;goto _LL0;case Cyc_Absyn_Unsigned: _LL3: _LL4:
 t=Cyc_Absyn_uchar_type;goto _LL0;default: _LL5: _LL6:
 t=Cyc_Absyn_char_type;goto _LL0;}case 3U: _LL7: _LL8:
 t=({Cyc_Absyn_wchar_type();});goto _LL0;case 4U: _LL9: _tmp25=((_tmp1E.Short_c).val).f1;_LLA: {enum Cyc_Absyn_Sign sn=_tmp25;
# 153
t=(int)sn == (int)1U?Cyc_Absyn_ushort_type: Cyc_Absyn_sshort_type;goto _LL0;}case 6U: _LLB: _tmp24=((_tmp1E.LongLong_c).val).f1;_LLC: {enum Cyc_Absyn_Sign sn=_tmp24;
# 155
t=(int)sn == (int)1U?Cyc_Absyn_ulonglong_type: Cyc_Absyn_slonglong_type;goto _LL0;}case 7U: _LLD: _tmp23=((_tmp1E.Float_c).val).f2;_LLE: {int i=_tmp23;
# 157
if(topt == 0)t=({Cyc_Absyn_gen_float_type((unsigned)i);});else{
# 161
void*_stmttmp3=({Cyc_Tcutil_compress(*topt);});void*_tmp26=_stmttmp3;int _tmp27;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp26)->tag == 0U){if(((struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp26)->f1)->tag == 2U){_LL18: _tmp27=((struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp26)->f1)->f1;_LL19: {int i=_tmp27;
t=({Cyc_Absyn_gen_float_type((unsigned)i);});goto _LL17;}}else{goto _LL1A;}}else{_LL1A: _LL1B:
# 165
 t=({Cyc_Absyn_gen_float_type((unsigned)i);});
goto _LL17;}_LL17:;}
# 169
goto _LL0;}case 5U: _LLF: _tmp21=((_tmp1E.Int_c).val).f1;_tmp22=((_tmp1E.Int_c).val).f2;_LL10: {enum Cyc_Absyn_Sign csn=_tmp21;int i=_tmp22;
# 171
if(topt == 0)
t=(int)csn == (int)1U?Cyc_Absyn_uint_type: Cyc_Absyn_sint_type;else{
# 178
void*_stmttmp4=({Cyc_Tcutil_compress(*topt);});void*_tmp28=_stmttmp4;void*_tmp2E;void*_tmp2D;void*_tmp2C;void*_tmp2B;struct Cyc_Absyn_Tqual _tmp2A;void*_tmp29;void*_tmp2F;enum Cyc_Absyn_Sign _tmp30;enum Cyc_Absyn_Sign _tmp31;enum Cyc_Absyn_Sign _tmp32;enum Cyc_Absyn_Sign _tmp33;switch(*((int*)_tmp28)){case 0U: switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp28)->f1)){case 1U: switch(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp28)->f1)->f2){case Cyc_Absyn_Char_sz: _LL1D: _tmp33=((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp28)->f1)->f1;_LL1E: {enum Cyc_Absyn_Sign sn=_tmp33;
# 180
{enum Cyc_Absyn_Sign _tmp34=sn;switch(_tmp34){case Cyc_Absyn_Unsigned: _LL2C: _LL2D:
 t=Cyc_Absyn_uchar_type;goto _LL2B;case Cyc_Absyn_Signed: _LL2E: _LL2F:
 t=Cyc_Absyn_schar_type;goto _LL2B;default: _LL30: _LL31:
 t=Cyc_Absyn_char_type;goto _LL2B;}_LL2B:;}
# 185
({union Cyc_Absyn_Cnst _tmp6D6=({Cyc_Absyn_Char_c(sn,(char)i);});*c=_tmp6D6;});
goto _LL1C;}case Cyc_Absyn_Short_sz: _LL1F: _tmp32=((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp28)->f1)->f1;_LL20: {enum Cyc_Absyn_Sign sn=_tmp32;
# 188
t=(int)sn == (int)1U?Cyc_Absyn_ushort_type: Cyc_Absyn_sshort_type;
({union Cyc_Absyn_Cnst _tmp6D7=({Cyc_Absyn_Short_c(sn,(short)i);});*c=_tmp6D7;});
goto _LL1C;}case Cyc_Absyn_Int_sz: _LL21: _tmp31=((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp28)->f1)->f1;_LL22: {enum Cyc_Absyn_Sign sn=_tmp31;
# 192
t=(int)sn == (int)1U?Cyc_Absyn_uint_type: Cyc_Absyn_sint_type;
# 195
({union Cyc_Absyn_Cnst _tmp6D8=({Cyc_Absyn_Int_c(sn,i);});*c=_tmp6D8;});
goto _LL1C;}case Cyc_Absyn_Long_sz: _LL23: _tmp30=((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp28)->f1)->f1;_LL24: {enum Cyc_Absyn_Sign sn=_tmp30;
# 198
t=(int)sn == (int)1U?Cyc_Absyn_uint_type: Cyc_Absyn_sint_type;
# 201
({union Cyc_Absyn_Cnst _tmp6D9=({Cyc_Absyn_Int_c(sn,i);});*c=_tmp6D9;});
goto _LL1C;}default: goto _LL29;}case 5U: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp28)->f2 != 0){_LL27: _tmp2F=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp28)->f2)->hd;_LL28: {void*t1=_tmp2F;
# 215
struct Cyc_Absyn_ValueofType_Absyn_Type_struct*t2=({struct Cyc_Absyn_ValueofType_Absyn_Type_struct*_tmp39=_cycalloc(sizeof(*_tmp39));_tmp39->tag=9U,({struct Cyc_Absyn_Exp*_tmp6DA=({Cyc_Absyn_uint_exp((unsigned)i,0U);});_tmp39->f1=_tmp6DA;});_tmp39;});
# 222
t=({Cyc_Absyn_tag_type((void*)t2);});
goto _LL1C;}}else{goto _LL29;}default: goto _LL29;}case 3U: _LL25: _tmp29=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp28)->f1).elt_type;_tmp2A=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp28)->f1).elt_tq;_tmp2B=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp28)->f1).ptr_atts).rgn;_tmp2C=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp28)->f1).ptr_atts).nullable;_tmp2D=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp28)->f1).ptr_atts).bounds;_tmp2E=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp28)->f1).ptr_atts).zero_term;if(i == 0){_LL26: {void*et=_tmp29;struct Cyc_Absyn_Tqual etq=_tmp2A;void*rgn=_tmp2B;void*nbl=_tmp2C;void*bnd=_tmp2D;void*zt=_tmp2E;
# 205
static struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct nullc={0U,{.Null_c={1,0}}};
e->r=(void*)& nullc;
if(({Cyc_Tcutil_force_type2bool(1,nbl);}))return*topt;({struct Cyc_String_pa_PrintArg_struct _tmp37=({struct Cyc_String_pa_PrintArg_struct _tmp63C;_tmp63C.tag=0U,({
# 209
struct _fat_ptr _tmp6DB=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(*topt);}));_tmp63C.f1=_tmp6DB;});_tmp63C;});void*_tmp35[1U];_tmp35[0]=& _tmp37;({unsigned _tmp6DD=e->loc;struct _fat_ptr _tmp6DC=({const char*_tmp36="Used NULL when expecting a non-NULL value of type %s";_tag_fat(_tmp36,sizeof(char),53U);});Cyc_Tcutil_terr(_tmp6DD,_tmp6DC,_tag_fat(_tmp35,sizeof(void*),1U));});});{
struct Cyc_List_List*tenv_tvs=({Cyc_Tcenv_lookup_type_vars(te);});
t=(void*)({struct Cyc_Absyn_PointerType_Absyn_Type_struct*_tmp38=_cycalloc(sizeof(*_tmp38));_tmp38->tag=3U,(_tmp38->f1).elt_type=et,(_tmp38->f1).elt_tq=etq,
((_tmp38->f1).ptr_atts).rgn=rgn,((_tmp38->f1).ptr_atts).nullable=Cyc_Absyn_true_type,((_tmp38->f1).ptr_atts).bounds=bnd,((_tmp38->f1).ptr_atts).zero_term=zt,((_tmp38->f1).ptr_atts).ptrloc=0;_tmp38;});
goto _LL1C;}}}else{goto _LL29;}default: _LL29: _LL2A:
# 225
 t=(int)csn == (int)1U?Cyc_Absyn_uint_type: Cyc_Absyn_sint_type;
goto _LL1C;}_LL1C:;}
# 228
goto _LL0;}case 8U: _LL11: _tmp20=(_tmp1E.String_c).val;_LL12: {struct _fat_ptr s=_tmp20;
# 230
string_numelts=(int)_get_fat_size(s,sizeof(char));
_tmp1F=s;goto _LL14;}case 9U: _LL13: _tmp1F=(_tmp1E.Wstring_c).val;_LL14: {struct _fat_ptr s=_tmp1F;
# 233
if(string_numelts == 0){
string_numelts=({Cyc_Tcexp_wchar_numelts(s);});
string_elt_typ=({Cyc_Absyn_wchar_type();});}{
# 233
struct Cyc_Absyn_Exp*elen=({struct Cyc_Absyn_Exp*(*_tmp53)(union Cyc_Absyn_Cnst,unsigned)=Cyc_Absyn_const_exp;union Cyc_Absyn_Cnst _tmp54=({Cyc_Absyn_Int_c(Cyc_Absyn_Unsigned,string_numelts);});unsigned _tmp55=loc;_tmp53(_tmp54,_tmp55);});
# 238
elen->topt=Cyc_Absyn_uint_type;{
# 242
void*elen_bound=({Cyc_Absyn_thin_bounds_exp(elen);});
t=({void*(*_tmp3A)(void*t,void*rgn,struct Cyc_Absyn_Tqual tq,void*b,void*zero_term)=Cyc_Absyn_atb_type;void*_tmp3B=string_elt_typ;void*_tmp3C=Cyc_Absyn_heap_rgn_type;struct Cyc_Absyn_Tqual _tmp3D=({Cyc_Absyn_const_tqual(0U);});void*_tmp3E=elen_bound;void*_tmp3F=Cyc_Absyn_true_type;_tmp3A(_tmp3B,_tmp3C,_tmp3D,_tmp3E,_tmp3F);});
# 245
if(topt != 0){
void*_stmttmp5=({Cyc_Tcutil_compress(*topt);});void*_tmp40=_stmttmp5;struct Cyc_Absyn_Tqual _tmp41;switch(*((int*)_tmp40)){case 4U: _LL33: _tmp41=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp40)->f1).tq;_LL34: {struct Cyc_Absyn_Tqual tq=_tmp41;
# 251
return({void*(*_tmp42)(void*elt_type,struct Cyc_Absyn_Tqual tq,struct Cyc_Absyn_Exp*num_elts,void*zero_term,unsigned ztloc)=Cyc_Absyn_array_type;void*_tmp43=string_elt_typ;struct Cyc_Absyn_Tqual _tmp44=tq;struct Cyc_Absyn_Exp*_tmp45=elen;void*_tmp46=({void*(*_tmp47)(struct Cyc_List_List*)=Cyc_Tcutil_any_bool;struct Cyc_List_List*_tmp48=({Cyc_Tcenv_lookup_type_vars(te);});_tmp47(_tmp48);});unsigned _tmp49=0U;_tmp42(_tmp43,_tmp44,_tmp45,_tmp46,_tmp49);});}case 3U: _LL35: _LL36:
# 254
 if(!({Cyc_Unify_unify(*topt,t);})&&({Cyc_Tcutil_silent_castable(te,loc,t,*topt);})){
# 256
e->topt=t;
({Cyc_Tcutil_unchecked_cast(e,*topt,Cyc_Absyn_Other_coercion);});
t=*topt;}else{
# 261
t=({void*(*_tmp4A)(void*t,void*rgn,struct Cyc_Absyn_Tqual tq,void*b,void*zero_term)=Cyc_Absyn_atb_type;void*_tmp4B=string_elt_typ;void*_tmp4C=({void*(*_tmp4D)(struct Cyc_Core_Opt*k,struct Cyc_Core_Opt*tenv)=Cyc_Absyn_new_evar;struct Cyc_Core_Opt*_tmp4E=& Cyc_Tcutil_rko;struct Cyc_Core_Opt*_tmp4F=({Cyc_Tcenv_lookup_opt_type_vars(te);});_tmp4D(_tmp4E,_tmp4F);});struct Cyc_Absyn_Tqual _tmp50=({Cyc_Absyn_const_tqual(0U);});void*_tmp51=elen_bound;void*_tmp52=Cyc_Absyn_true_type;_tmp4A(_tmp4B,_tmp4C,_tmp50,_tmp51,_tmp52);});
# 263
if(!({Cyc_Unify_unify(*topt,t);})&&({Cyc_Tcutil_silent_castable(te,loc,t,*topt);})){
# 265
e->topt=t;
({Cyc_Tcutil_unchecked_cast(e,*topt,Cyc_Absyn_Other_coercion);});
t=*topt;}}
# 270
goto _LL32;default: _LL37: _LL38:
 goto _LL32;}_LL32:;}
# 245
return t;}}}default: _LL15: _LL16:
# 276
 if(topt != 0){
void*_stmttmp6=({Cyc_Tcutil_compress(*topt);});void*_tmp56=_stmttmp6;void*_tmp5C;void*_tmp5B;void*_tmp5A;void*_tmp59;struct Cyc_Absyn_Tqual _tmp58;void*_tmp57;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp56)->tag == 3U){_LL3A: _tmp57=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp56)->f1).elt_type;_tmp58=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp56)->f1).elt_tq;_tmp59=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp56)->f1).ptr_atts).rgn;_tmp5A=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp56)->f1).ptr_atts).nullable;_tmp5B=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp56)->f1).ptr_atts).bounds;_tmp5C=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp56)->f1).ptr_atts).zero_term;_LL3B: {void*et=_tmp57;struct Cyc_Absyn_Tqual etq=_tmp58;void*rgn=_tmp59;void*nbl=_tmp5A;void*bnd=_tmp5B;void*zt=_tmp5C;
# 280
if(({Cyc_Tcutil_force_type2bool(1,nbl);}))return*topt;({struct Cyc_String_pa_PrintArg_struct _tmp5F=({struct Cyc_String_pa_PrintArg_struct _tmp63D;_tmp63D.tag=0U,({
# 282
struct _fat_ptr _tmp6DE=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(*topt);}));_tmp63D.f1=_tmp6DE;});_tmp63D;});void*_tmp5D[1U];_tmp5D[0]=& _tmp5F;({unsigned _tmp6E0=e->loc;struct _fat_ptr _tmp6DF=({const char*_tmp5E="Used NULL when expecting a non-NULL value of type %s";_tag_fat(_tmp5E,sizeof(char),53U);});Cyc_Tcutil_terr(_tmp6E0,_tmp6DF,_tag_fat(_tmp5D,sizeof(void*),1U));});});
return(void*)({struct Cyc_Absyn_PointerType_Absyn_Type_struct*_tmp60=_cycalloc(sizeof(*_tmp60));_tmp60->tag=3U,(_tmp60->f1).elt_type=et,(_tmp60->f1).elt_tq=etq,((_tmp60->f1).ptr_atts).rgn=rgn,((_tmp60->f1).ptr_atts).nullable=Cyc_Absyn_true_type,((_tmp60->f1).ptr_atts).bounds=bnd,((_tmp60->f1).ptr_atts).zero_term=zt,((_tmp60->f1).ptr_atts).ptrloc=0;_tmp60;});}}else{_LL3C: _LL3D:
# 285
 goto _LL39;}_LL39:;}{
# 276
struct Cyc_List_List*tenv_tvs=({Cyc_Tcenv_lookup_type_vars(te);});
# 289
t=(void*)({struct Cyc_Absyn_PointerType_Absyn_Type_struct*_tmp63=_cycalloc(sizeof(*_tmp63));_tmp63->tag=3U,({void*_tmp6E5=({Cyc_Absyn_new_evar(& Cyc_Tcutil_tako,({struct Cyc_Core_Opt*_tmp61=_cycalloc(sizeof(*_tmp61));_tmp61->v=tenv_tvs;_tmp61;}));});(_tmp63->f1).elt_type=_tmp6E5;}),({
struct Cyc_Absyn_Tqual _tmp6E4=({Cyc_Absyn_empty_tqual(0U);});(_tmp63->f1).elt_tq=_tmp6E4;}),
({void*_tmp6E3=({Cyc_Absyn_new_evar(& Cyc_Tcutil_trko,({struct Cyc_Core_Opt*_tmp62=_cycalloc(sizeof(*_tmp62));_tmp62->v=tenv_tvs;_tmp62;}));});((_tmp63->f1).ptr_atts).rgn=_tmp6E3;}),((_tmp63->f1).ptr_atts).nullable=Cyc_Absyn_true_type,({
void*_tmp6E2=({Cyc_Tcutil_any_bounds(tenv_tvs);});((_tmp63->f1).ptr_atts).bounds=_tmp6E2;}),({
void*_tmp6E1=({Cyc_Tcutil_any_bool(tenv_tvs);});((_tmp63->f1).ptr_atts).zero_term=_tmp6E1;}),((_tmp63->f1).ptr_atts).ptrloc=0;_tmp63;});
# 295
goto _LL0;}}_LL0:;}
# 297
return t;}
# 143
static void*Cyc_Tcexp_tcDatatype(struct Cyc_Tcenv_Tenv*te,unsigned loc,void**topt,struct Cyc_Absyn_Exp*e,struct Cyc_List_List*es,struct Cyc_Absyn_Datatypedecl*tud,struct Cyc_Absyn_Datatypefield*tuf);
# 306
static void*Cyc_Tcexp_tcVar(struct Cyc_Tcenv_Tenv*te,unsigned loc,void**topt,struct Cyc_Absyn_Exp*e,void**b){
void*_stmttmp7=*((void**)_check_null(b));void*_tmp65=_stmttmp7;struct Cyc_Absyn_Vardecl*_tmp66;struct Cyc_Absyn_Vardecl*_tmp67;struct Cyc_Absyn_Vardecl*_tmp68;struct Cyc_Absyn_Fndecl*_tmp69;struct Cyc_Absyn_Vardecl*_tmp6A;struct _tuple1*_tmp6B;switch(*((int*)_tmp65)){case 0U: _LL1: _tmp6B=((struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct*)_tmp65)->f1;_LL2: {struct _tuple1*q=_tmp6B;
({void*_tmp6C=0U;({int(*_tmp6E7)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp6E)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp6E;});struct _fat_ptr _tmp6E6=({const char*_tmp6D="unresolved binding in tcVar";_tag_fat(_tmp6D,sizeof(char),28U);});_tmp6E7(_tmp6E6,_tag_fat(_tmp6C,sizeof(void*),0U));});});}case 1U: _LL3: _tmp6A=((struct Cyc_Absyn_Global_b_Absyn_Binding_struct*)_tmp65)->f1;_LL4: {struct Cyc_Absyn_Vardecl*vd=_tmp6A;
# 312
({Cyc_Tcenv_lookup_ordinary_global(te,loc,vd->name,1);});
# 314
if(({Cyc_Tcenv_is_reentrant_fun(te);})){
# 316
void*t0=({Cyc_Tcutil_compress(vd->type);});
void*_tmp6F=t0;struct Cyc_Absyn_PtrInfo _tmp70;switch(*((int*)_tmp6F)){case 5U: _LLE: _LLF:
# 319
 goto _LLD;case 3U: _LL10: _tmp70=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp6F)->f1;_LL11: {struct Cyc_Absyn_PtrInfo pinfo_a=_tmp70;
# 321
if((pinfo_a.elt_tq).real_const)goto _LLD;goto _LL13;}default: _LL12: _LL13:
# 324
 if(({Cyc_Absyn_forgive_global();})== 0)
({struct Cyc_String_pa_PrintArg_struct _tmp73=({struct Cyc_String_pa_PrintArg_struct _tmp63E;_tmp63E.tag=0U,({
struct _fat_ptr _tmp6E8=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(vd->name);}));_tmp63E.f1=_tmp6E8;});_tmp63E;});void*_tmp71[1U];_tmp71[0]=& _tmp73;({unsigned _tmp6EA=loc;struct _fat_ptr _tmp6E9=({const char*_tmp72="The use of global variables within re-entrant functions is disallowed. Offending variable: %s";_tag_fat(_tmp72,sizeof(char),94U);});Cyc_Tcutil_terr(_tmp6EA,_tmp6E9,_tag_fat(_tmp71,sizeof(void*),1U));});});}_LLD:;}
# 314
return vd->type;}case 2U: _LL5: _tmp69=((struct Cyc_Absyn_Funname_b_Absyn_Binding_struct*)_tmp65)->f1;_LL6: {struct Cyc_Absyn_Fndecl*fd=_tmp69;
# 336
if(fd->fn_vardecl == 0)
({Cyc_Tcenv_lookup_ordinary_global(te,loc,fd->name,1);});
# 336
return({Cyc_Tcutil_fndecl2type(fd);});}case 5U: _LL7: _tmp68=((struct Cyc_Absyn_Pat_b_Absyn_Binding_struct*)_tmp65)->f1;_LL8: {struct Cyc_Absyn_Vardecl*vd=_tmp68;
# 339
_tmp67=vd;goto _LLA;}case 4U: _LL9: _tmp67=((struct Cyc_Absyn_Local_b_Absyn_Binding_struct*)_tmp65)->f1;_LLA: {struct Cyc_Absyn_Vardecl*vd=_tmp67;
_tmp66=vd;goto _LLC;}default: _LLB: _tmp66=((struct Cyc_Absyn_Param_b_Absyn_Binding_struct*)_tmp65)->f1;_LLC: {struct Cyc_Absyn_Vardecl*vd=_tmp66;
# 342
if(te->allow_valueof){
void*_stmttmp8=({Cyc_Tcutil_compress(vd->type);});void*_tmp74=_stmttmp8;void*_tmp75;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp74)->tag == 0U){if(((struct Cyc_Absyn_TagCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp74)->f1)->tag == 5U){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp74)->f2 != 0){_LL15: _tmp75=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp74)->f2)->hd;_LL16: {void*i=_tmp75;
# 345
({void*_tmp6EB=(void*)({struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct*_tmp76=_cycalloc(sizeof(*_tmp76));_tmp76->tag=40U,_tmp76->f1=i;_tmp76;});e->r=_tmp6EB;});
goto _LL14;}}else{goto _LL17;}}else{goto _LL17;}}else{_LL17: _LL18:
 goto _LL14;}_LL14:;}{
# 342
void*ret=vd->type;
# 353
return ret;}}}_LL0:;}
# 358
static void Cyc_Tcexp_check_format_args(struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Exp*fmt,struct Cyc_Core_Opt*opt_args,int arg_cnt,struct Cyc_List_List**alias_arg_exps,int isCproto,struct Cyc_List_List*(*type_getter)(struct Cyc_Tcenv_Tenv*,struct _fat_ptr,int,unsigned)){
# 365
struct Cyc_List_List*desc_types;
{void*_stmttmp9=fmt->r;void*_tmp78=_stmttmp9;struct _fat_ptr _tmp79;struct _fat_ptr _tmp7A;switch(*((int*)_tmp78)){case 0U: if(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp78)->f1).String_c).tag == 8){_LL1: _tmp7A=((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp78)->f1).String_c).val;_LL2: {struct _fat_ptr s=_tmp7A;
_tmp79=s;goto _LL4;}}else{goto _LL5;}case 14U: if(((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)((struct Cyc_Absyn_Exp*)((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp78)->f2)->r)->tag == 0U){if(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)((struct Cyc_Absyn_Exp*)((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp78)->f2)->r)->f1).String_c).tag == 8){_LL3: _tmp79=((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)(((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp78)->f2)->r)->f1).String_c).val;_LL4: {struct _fat_ptr s=_tmp79;
# 369
desc_types=({type_getter(te,(struct _fat_ptr)s,isCproto,fmt->loc);});goto _LL0;}}else{goto _LL5;}}else{goto _LL5;}default: _LL5: _LL6:
# 373
 if(opt_args != 0){
struct Cyc_List_List*args=(struct Cyc_List_List*)opt_args->v;
for(0;args != 0;args=args->tl){
({Cyc_Tcexp_tcExp(te,0,(struct Cyc_Absyn_Exp*)args->hd);});
if(({Cyc_Tcutil_is_noalias_pointer_or_aggr((void*)_check_null(((struct Cyc_Absyn_Exp*)args->hd)->topt));})&& !({Cyc_Tcutil_is_noalias_path((struct Cyc_Absyn_Exp*)args->hd);}))
# 379
({void*_tmp7B=0U;({unsigned _tmp6ED=((struct Cyc_Absyn_Exp*)args->hd)->loc;struct _fat_ptr _tmp6EC=({const char*_tmp7C="Cannot consume non-unique paths; do swap instead";_tag_fat(_tmp7C,sizeof(char),49U);});Cyc_Tcutil_terr(_tmp6ED,_tmp6EC,_tag_fat(_tmp7B,sizeof(void*),0U));});});}}
# 373
return;}_LL0:;}
# 384
if(opt_args != 0){
struct Cyc_List_List*args=(struct Cyc_List_List*)opt_args->v;
# 387
for(0;desc_types != 0 && args != 0;(
desc_types=desc_types->tl,args=args->tl,arg_cnt ++)){
int alias_coercion=0;
void*t=(void*)desc_types->hd;
struct Cyc_Absyn_Exp*e=(struct Cyc_Absyn_Exp*)args->hd;
({Cyc_Tcexp_tcExp(te,& t,e);});
if(!({Cyc_Tcutil_coerce_arg(te,e,t,& alias_coercion);})){
({struct Cyc_String_pa_PrintArg_struct _tmp7F=({struct Cyc_String_pa_PrintArg_struct _tmp640;_tmp640.tag=0U,({
struct _fat_ptr _tmp6EE=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t);}));_tmp640.f1=_tmp6EE;});_tmp640;});struct Cyc_String_pa_PrintArg_struct _tmp80=({struct Cyc_String_pa_PrintArg_struct _tmp63F;_tmp63F.tag=0U,({struct _fat_ptr _tmp6EF=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string((void*)_check_null(e->topt));}));_tmp63F.f1=_tmp6EF;});_tmp63F;});void*_tmp7D[2U];_tmp7D[0]=& _tmp7F,_tmp7D[1]=& _tmp80;({unsigned _tmp6F1=e->loc;struct _fat_ptr _tmp6F0=({const char*_tmp7E="descriptor has type %s but argument has type %s";_tag_fat(_tmp7E,sizeof(char),48U);});Cyc_Tcutil_terr(_tmp6F1,_tmp6F0,_tag_fat(_tmp7D,sizeof(void*),2U));});});
({Cyc_Unify_explain_failure();});}
# 393
if(alias_coercion)
# 399
({struct Cyc_List_List*_tmp6F2=({struct Cyc_List_List*_tmp81=_cycalloc(sizeof(*_tmp81));_tmp81->hd=(void*)arg_cnt,_tmp81->tl=*alias_arg_exps;_tmp81;});*alias_arg_exps=_tmp6F2;});
# 393
if(
# 400
({Cyc_Tcutil_is_noalias_pointer_or_aggr(t);})&& !({Cyc_Tcutil_is_noalias_path(e);}))
# 402
({void*_tmp82=0U;({unsigned _tmp6F4=((struct Cyc_Absyn_Exp*)args->hd)->loc;struct _fat_ptr _tmp6F3=({const char*_tmp83="Cannot consume non-unique paths; do swap instead";_tag_fat(_tmp83,sizeof(char),49U);});Cyc_Tcutil_terr(_tmp6F4,_tmp6F3,_tag_fat(_tmp82,sizeof(void*),0U));});});}
# 405
if(desc_types != 0)
({void*_tmp84=0U;({unsigned _tmp6F6=fmt->loc;struct _fat_ptr _tmp6F5=({const char*_tmp85="too few arguments";_tag_fat(_tmp85,sizeof(char),18U);});Cyc_Tcutil_terr(_tmp6F6,_tmp6F5,_tag_fat(_tmp84,sizeof(void*),0U));});});
# 405
if(args != 0){
# 408
({void*_tmp86=0U;({unsigned _tmp6F8=((struct Cyc_Absyn_Exp*)args->hd)->loc;struct _fat_ptr _tmp6F7=({const char*_tmp87="too many arguments";_tag_fat(_tmp87,sizeof(char),19U);});Cyc_Tcutil_terr(_tmp6F8,_tmp6F7,_tag_fat(_tmp86,sizeof(void*),0U));});});
# 410
for(0;args != 0;args=args->tl){
({Cyc_Tcexp_tcExp(te,0,(struct Cyc_Absyn_Exp*)args->hd);});}}}}
# 415
static void*Cyc_Tcexp_tcUnPrimop(struct Cyc_Tcenv_Tenv*te,unsigned loc,void**topt,enum Cyc_Absyn_Primop p,struct Cyc_Absyn_Exp*e){
# 417
void*t=({Cyc_Tcutil_compress((void*)_check_null(e->topt));});
enum Cyc_Absyn_Primop _tmp89=p;switch(_tmp89){case Cyc_Absyn_Plus: _LL1: _LL2:
 goto _LL4;case Cyc_Absyn_Minus: _LL3: _LL4:
# 421
 if(!({Cyc_Tcutil_is_numeric(e);}))
({struct Cyc_String_pa_PrintArg_struct _tmp8C=({struct Cyc_String_pa_PrintArg_struct _tmp641;_tmp641.tag=0U,({struct _fat_ptr _tmp6F9=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t);}));_tmp641.f1=_tmp6F9;});_tmp641;});void*_tmp8A[1U];_tmp8A[0]=& _tmp8C;({unsigned _tmp6FB=loc;struct _fat_ptr _tmp6FA=({const char*_tmp8B="expecting numeric type but found %s";_tag_fat(_tmp8B,sizeof(char),36U);});Cyc_Tcutil_terr(_tmp6FB,_tmp6FA,_tag_fat(_tmp8A,sizeof(void*),1U));});});
# 421
return(void*)_check_null(e->topt);case Cyc_Absyn_Not: _LL5: _LL6:
# 425
({Cyc_Tcexp_check_contains_assign(e);});
if(!({Cyc_Tcutil_coerce_to_bool(e);}))
({struct Cyc_String_pa_PrintArg_struct _tmp8F=({struct Cyc_String_pa_PrintArg_struct _tmp642;_tmp642.tag=0U,({struct _fat_ptr _tmp6FC=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t);}));_tmp642.f1=_tmp6FC;});_tmp642;});void*_tmp8D[1U];_tmp8D[0]=& _tmp8F;({unsigned _tmp6FE=loc;struct _fat_ptr _tmp6FD=({const char*_tmp8E="expecting integral or * type but found %s";_tag_fat(_tmp8E,sizeof(char),42U);});Cyc_Tcutil_terr(_tmp6FE,_tmp6FD,_tag_fat(_tmp8D,sizeof(void*),1U));});});
# 426
return Cyc_Absyn_sint_type;case Cyc_Absyn_Bitnot: _LL7: _LL8:
# 430
 if(!({Cyc_Tcutil_is_integral(e);}))
({struct Cyc_String_pa_PrintArg_struct _tmp92=({struct Cyc_String_pa_PrintArg_struct _tmp643;_tmp643.tag=0U,({struct _fat_ptr _tmp6FF=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t);}));_tmp643.f1=_tmp6FF;});_tmp643;});void*_tmp90[1U];_tmp90[0]=& _tmp92;({unsigned _tmp701=loc;struct _fat_ptr _tmp700=({const char*_tmp91="expecting integral type but found %s";_tag_fat(_tmp91,sizeof(char),37U);});Cyc_Tcutil_terr(_tmp701,_tmp700,_tag_fat(_tmp90,sizeof(void*),1U));});});
# 430
return(void*)_check_null(e->topt);case Cyc_Absyn_Numelts: _LL9: _LLA:
# 434
{void*_tmp93=t;void*_tmp94;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp93)->tag == 3U){_LLE: _tmp94=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp93)->f1).ptr_atts).bounds;_LLF: {void*b=_tmp94;
# 436
struct Cyc_Absyn_Exp*eopt=({Cyc_Tcutil_get_bounds_exp(Cyc_Absyn_fat_bound_type,b);});
if(eopt != 0){
struct Cyc_Absyn_Exp*e=eopt;
if(!({Cyc_Evexp_c_can_eval(e);})&& !((unsigned)Cyc_Tcenv_allow_valueof))
({void*_tmp95=0U;({unsigned _tmp703=loc;struct _fat_ptr _tmp702=({const char*_tmp96="cannot apply numelts to a pointer with abstract bounds";_tag_fat(_tmp96,sizeof(char),55U);});Cyc_Tcutil_terr(_tmp703,_tmp702,_tag_fat(_tmp95,sizeof(void*),0U));});});}
# 437
goto _LLD;}}else{_LL10: _LL11:
# 444
({struct Cyc_String_pa_PrintArg_struct _tmp99=({struct Cyc_String_pa_PrintArg_struct _tmp644;_tmp644.tag=0U,({struct _fat_ptr _tmp704=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t);}));_tmp644.f1=_tmp704;});_tmp644;});void*_tmp97[1U];_tmp97[0]=& _tmp99;({unsigned _tmp706=loc;struct _fat_ptr _tmp705=({const char*_tmp98="numelts requires pointer type, not %s";_tag_fat(_tmp98,sizeof(char),38U);});Cyc_Tcutil_terr(_tmp706,_tmp705,_tag_fat(_tmp97,sizeof(void*),1U));});});}_LLD:;}
# 446
return Cyc_Absyn_uint_type;default: _LLB: _LLC:
({void*_tmp9A=0U;({int(*_tmp708)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp9C)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp9C;});struct _fat_ptr _tmp707=({const char*_tmp9B="Non-unary primop";_tag_fat(_tmp9B,sizeof(char),17U);});_tmp708(_tmp707,_tag_fat(_tmp9A,sizeof(void*),0U));});});}_LL0:;}
# 453
static void*Cyc_Tcexp_arith_convert(struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2){
void*t1=(void*)_check_null(e1->topt);
void*t2=(void*)_check_null(e2->topt);
void*new_t=({Cyc_Tcutil_max_arithmetic_type(t1,t2);});
if(!({Cyc_Unify_unify(t1,new_t);}))({Cyc_Tcutil_unchecked_cast(e1,new_t,Cyc_Absyn_No_coercion);});if(!({Cyc_Unify_unify(t2,new_t);}))
({Cyc_Tcutil_unchecked_cast(e2,new_t,Cyc_Absyn_No_coercion);});
# 457
return new_t;}
# 463
static void*Cyc_Tcexp_tcArithBinop(struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2,int(*checker)(struct Cyc_Absyn_Exp*)){
# 466
if(!({checker(e1);})){
({struct Cyc_String_pa_PrintArg_struct _tmpA1=({struct Cyc_String_pa_PrintArg_struct _tmp645;_tmp645.tag=0U,({struct _fat_ptr _tmp709=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string((void*)_check_null(e1->topt));}));_tmp645.f1=_tmp709;});_tmp645;});void*_tmp9F[1U];_tmp9F[0]=& _tmpA1;({unsigned _tmp70B=e1->loc;struct _fat_ptr _tmp70A=({const char*_tmpA0="type %s cannot be used here";_tag_fat(_tmpA0,sizeof(char),28U);});Cyc_Tcutil_terr(_tmp70B,_tmp70A,_tag_fat(_tmp9F,sizeof(void*),1U));});});
return({void*(*_tmpA2)(struct Cyc_Core_Opt*)=Cyc_Absyn_wildtyp;struct Cyc_Core_Opt*_tmpA3=({Cyc_Tcenv_lookup_opt_type_vars(te);});_tmpA2(_tmpA3);});}
# 466
if(!({checker(e2);})){
# 471
({struct Cyc_String_pa_PrintArg_struct _tmpA6=({struct Cyc_String_pa_PrintArg_struct _tmp646;_tmp646.tag=0U,({struct _fat_ptr _tmp70C=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string((void*)_check_null(e2->topt));}));_tmp646.f1=_tmp70C;});_tmp646;});void*_tmpA4[1U];_tmpA4[0]=& _tmpA6;({unsigned _tmp70E=e2->loc;struct _fat_ptr _tmp70D=({const char*_tmpA5="type %s cannot be used here";_tag_fat(_tmpA5,sizeof(char),28U);});Cyc_Tcutil_terr(_tmp70E,_tmp70D,_tag_fat(_tmpA4,sizeof(void*),1U));});});
return({void*(*_tmpA7)(struct Cyc_Core_Opt*)=Cyc_Absyn_wildtyp;struct Cyc_Core_Opt*_tmpA8=({Cyc_Tcenv_lookup_opt_type_vars(te);});_tmpA7(_tmpA8);});}
# 466
return({Cyc_Tcexp_arith_convert(te,e1,e2);});}
# 477
static void*Cyc_Tcexp_tcPlus(struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2){
void*t1=({Cyc_Tcutil_compress((void*)_check_null(e1->topt));});
void*t2=({Cyc_Tcutil_compress((void*)_check_null(e2->topt));});
void*_tmpAA=t1;void*_tmpB0;void*_tmpAF;void*_tmpAE;void*_tmpAD;struct Cyc_Absyn_Tqual _tmpAC;void*_tmpAB;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmpAA)->tag == 3U){_LL1: _tmpAB=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmpAA)->f1).elt_type;_tmpAC=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmpAA)->f1).elt_tq;_tmpAD=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmpAA)->f1).ptr_atts).rgn;_tmpAE=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmpAA)->f1).ptr_atts).nullable;_tmpAF=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmpAA)->f1).ptr_atts).bounds;_tmpB0=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmpAA)->f1).ptr_atts).zero_term;_LL2: {void*et=_tmpAB;struct Cyc_Absyn_Tqual tq=_tmpAC;void*r=_tmpAD;void*n=_tmpAE;void*b=_tmpAF;void*zt=_tmpB0;
# 482
if(!({int(*_tmpB1)(struct Cyc_Absyn_Kind*,struct Cyc_Absyn_Kind*)=Cyc_Tcutil_kind_leq;struct Cyc_Absyn_Kind*_tmpB2=({Cyc_Tcutil_type_kind(et);});struct Cyc_Absyn_Kind*_tmpB3=& Cyc_Tcutil_tmk;_tmpB1(_tmpB2,_tmpB3);}))
({void*_tmpB4=0U;({unsigned _tmp710=e1->loc;struct _fat_ptr _tmp70F=({const char*_tmpB5="can't perform arithmetic on abstract pointer type";_tag_fat(_tmpB5,sizeof(char),50U);});Cyc_Tcutil_terr(_tmp710,_tmp70F,_tag_fat(_tmpB4,sizeof(void*),0U));});});
# 482
if(({Cyc_Tcutil_is_noalias_pointer(t1,0);}))
# 485
({void*_tmpB6=0U;({unsigned _tmp712=e1->loc;struct _fat_ptr _tmp711=({const char*_tmpB7="can't perform arithmetic on non-aliasing pointer type";_tag_fat(_tmpB7,sizeof(char),54U);});Cyc_Tcutil_terr(_tmp712,_tmp711,_tag_fat(_tmpB6,sizeof(void*),0U));});});
# 482
if(!({Cyc_Tcutil_coerce_sint_type(e2);}))
# 487
({struct Cyc_String_pa_PrintArg_struct _tmpBA=({struct Cyc_String_pa_PrintArg_struct _tmp647;_tmp647.tag=0U,({struct _fat_ptr _tmp713=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t2);}));_tmp647.f1=_tmp713;});_tmp647;});void*_tmpB8[1U];_tmpB8[0]=& _tmpBA;({unsigned _tmp715=e2->loc;struct _fat_ptr _tmp714=({const char*_tmpB9="expecting int but found %s";_tag_fat(_tmpB9,sizeof(char),27U);});Cyc_Tcutil_terr(_tmp715,_tmp714,_tag_fat(_tmpB8,sizeof(void*),1U));});});{
# 482
struct Cyc_Absyn_Exp*eopt=({Cyc_Tcutil_get_bounds_exp(Cyc_Absyn_fat_bound_type,b);});
# 489
if(eopt == 0)return t1;{struct Cyc_Absyn_Exp*ub=eopt;
# 493
if(({Cyc_Tcutil_force_type2bool(0,zt);})){
struct _tuple17 _stmttmpA=({Cyc_Evexp_eval_const_uint_exp(ub);});int _tmpBC;unsigned _tmpBB;_LL6: _tmpBB=_stmttmpA.f1;_tmpBC=_stmttmpA.f2;_LL7: {unsigned i=_tmpBB;int known=_tmpBC;
if(known && i == (unsigned)1)
({void*_tmpBD=0U;({unsigned _tmp717=e1->loc;struct _fat_ptr _tmp716=({const char*_tmpBE="pointer arithmetic on thin, zero-terminated pointer may be expensive.";_tag_fat(_tmpBE,sizeof(char),70U);});Cyc_Tcutil_warn(_tmp717,_tmp716,_tag_fat(_tmpBD,sizeof(void*),0U));});});}}{
# 493
struct Cyc_Absyn_PointerType_Absyn_Type_struct*new_t1=({struct Cyc_Absyn_PointerType_Absyn_Type_struct*_tmpBF=_cycalloc(sizeof(*_tmpBF));
# 504
_tmpBF->tag=3U,(_tmpBF->f1).elt_type=et,(_tmpBF->f1).elt_tq=tq,
((_tmpBF->f1).ptr_atts).rgn=r,((_tmpBF->f1).ptr_atts).nullable=Cyc_Absyn_true_type,((_tmpBF->f1).ptr_atts).bounds=Cyc_Absyn_fat_bound_type,((_tmpBF->f1).ptr_atts).zero_term=zt,((_tmpBF->f1).ptr_atts).ptrloc=0;_tmpBF;});
# 508
({Cyc_Tcutil_unchecked_cast(e1,(void*)new_t1,Cyc_Absyn_Other_coercion);});
return(void*)new_t1;}}}}}else{_LL3: _LL4:
 return({Cyc_Tcexp_tcArithBinop(te,e1,e2,Cyc_Tcutil_is_numeric);});}_LL0:;}
# 515
static void*Cyc_Tcexp_tcMinus(struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2){
void*t1=(void*)_check_null(e1->topt);
void*t2=(void*)_check_null(e2->topt);
void*t1_elt=Cyc_Absyn_void_type;
void*t2_elt=Cyc_Absyn_void_type;
if(({Cyc_Tcutil_is_fat_pointer_type_elt(t1,& t1_elt);})){
if(({Cyc_Tcutil_is_fat_pointer_type_elt(t2,& t2_elt);})){
if(!({Cyc_Unify_unify(t1_elt,t2_elt);})){
({struct Cyc_String_pa_PrintArg_struct _tmpC3=({struct Cyc_String_pa_PrintArg_struct _tmp649;_tmp649.tag=0U,({
# 525
struct _fat_ptr _tmp718=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t1);}));_tmp649.f1=_tmp718;});_tmp649;});struct Cyc_String_pa_PrintArg_struct _tmpC4=({struct Cyc_String_pa_PrintArg_struct _tmp648;_tmp648.tag=0U,({struct _fat_ptr _tmp719=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t2);}));_tmp648.f1=_tmp719;});_tmp648;});void*_tmpC1[2U];_tmpC1[0]=& _tmpC3,_tmpC1[1]=& _tmpC4;({unsigned _tmp71B=e1->loc;struct _fat_ptr _tmp71A=({const char*_tmpC2="pointer arithmetic on values of different types (%s != %s)";_tag_fat(_tmpC2,sizeof(char),59U);});Cyc_Tcutil_terr(_tmp71B,_tmp71A,_tag_fat(_tmpC1,sizeof(void*),2U));});});
({Cyc_Unify_explain_failure();});}
# 522
return Cyc_Absyn_sint_type;}else{
# 529
if(({Cyc_Tcutil_is_pointer_type(t2);})){
if(!({int(*_tmpC5)(void*,void*)=Cyc_Unify_unify;void*_tmpC6=t1_elt;void*_tmpC7=({Cyc_Tcutil_pointer_elt_type(t2);});_tmpC5(_tmpC6,_tmpC7);})){
({struct Cyc_String_pa_PrintArg_struct _tmpCA=({struct Cyc_String_pa_PrintArg_struct _tmp64B;_tmp64B.tag=0U,({
struct _fat_ptr _tmp71C=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t1);}));_tmp64B.f1=_tmp71C;});_tmp64B;});struct Cyc_String_pa_PrintArg_struct _tmpCB=({struct Cyc_String_pa_PrintArg_struct _tmp64A;_tmp64A.tag=0U,({struct _fat_ptr _tmp71D=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t2);}));_tmp64A.f1=_tmp71D;});_tmp64A;});void*_tmpC8[2U];_tmpC8[0]=& _tmpCA,_tmpC8[1]=& _tmpCB;({unsigned _tmp71F=e1->loc;struct _fat_ptr _tmp71E=({const char*_tmpC9="pointer arithmetic on values of different types (%s != %s)";_tag_fat(_tmpC9,sizeof(char),59U);});Cyc_Tcutil_terr(_tmp71F,_tmp71E,_tag_fat(_tmpC8,sizeof(void*),2U));});});
({Cyc_Unify_explain_failure();});}
# 530
({void*_tmpCC=0U;({unsigned _tmp721=e1->loc;struct _fat_ptr _tmp720=({const char*_tmpCD="coercing fat pointer to thin pointer to support subtraction";_tag_fat(_tmpCD,sizeof(char),60U);});Cyc_Tcutil_warn(_tmp721,_tmp720,_tag_fat(_tmpCC,sizeof(void*),0U));});});
# 537
({void(*_tmpCE)(struct Cyc_Absyn_Exp*,void*,enum Cyc_Absyn_Coercion)=Cyc_Tcutil_unchecked_cast;struct Cyc_Absyn_Exp*_tmpCF=e1;void*_tmpD0=({void*(*_tmpD1)(void*t,void*rgn,struct Cyc_Absyn_Tqual,void*zero_term)=Cyc_Absyn_star_type;void*_tmpD2=t1_elt;void*_tmpD3=({Cyc_Tcutil_pointer_region(t1);});struct Cyc_Absyn_Tqual _tmpD4=({Cyc_Absyn_empty_tqual(0U);});void*_tmpD5=Cyc_Absyn_false_type;_tmpD1(_tmpD2,_tmpD3,_tmpD4,_tmpD5);});enum Cyc_Absyn_Coercion _tmpD6=Cyc_Absyn_Other_coercion;_tmpCE(_tmpCF,_tmpD0,_tmpD6);});
# 540
return Cyc_Absyn_sint_type;}else{
# 542
if(!({int(*_tmpD7)(struct Cyc_Absyn_Kind*,struct Cyc_Absyn_Kind*)=Cyc_Tcutil_kind_leq;struct Cyc_Absyn_Kind*_tmpD8=({Cyc_Tcutil_type_kind(t1_elt);});struct Cyc_Absyn_Kind*_tmpD9=& Cyc_Tcutil_tmk;_tmpD7(_tmpD8,_tmpD9);}))
({void*_tmpDA=0U;({unsigned _tmp723=e1->loc;struct _fat_ptr _tmp722=({const char*_tmpDB="can't perform arithmetic on abstract pointer type";_tag_fat(_tmpDB,sizeof(char),50U);});Cyc_Tcutil_terr(_tmp723,_tmp722,_tag_fat(_tmpDA,sizeof(void*),0U));});});
# 542
if(({Cyc_Tcutil_is_noalias_pointer(t1,0);}))
# 545
({void*_tmpDC=0U;({unsigned _tmp725=e1->loc;struct _fat_ptr _tmp724=({const char*_tmpDD="can't perform arithmetic on non-aliasing pointer type";_tag_fat(_tmpDD,sizeof(char),54U);});Cyc_Tcutil_terr(_tmp725,_tmp724,_tag_fat(_tmpDC,sizeof(void*),0U));});});
# 542
if(!({Cyc_Tcutil_coerce_sint_type(e2);})){
# 547
({struct Cyc_String_pa_PrintArg_struct _tmpE0=({struct Cyc_String_pa_PrintArg_struct _tmp64D;_tmp64D.tag=0U,({
struct _fat_ptr _tmp726=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t1);}));_tmp64D.f1=_tmp726;});_tmp64D;});struct Cyc_String_pa_PrintArg_struct _tmpE1=({struct Cyc_String_pa_PrintArg_struct _tmp64C;_tmp64C.tag=0U,({struct _fat_ptr _tmp727=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t2);}));_tmp64C.f1=_tmp727;});_tmp64C;});void*_tmpDE[2U];_tmpDE[0]=& _tmpE0,_tmpDE[1]=& _tmpE1;({unsigned _tmp729=e2->loc;struct _fat_ptr _tmp728=({const char*_tmpDF="expecting either %s or int but found %s";_tag_fat(_tmpDF,sizeof(char),40U);});Cyc_Tcutil_terr(_tmp729,_tmp728,_tag_fat(_tmpDE,sizeof(void*),2U));});});
({Cyc_Unify_explain_failure();});}
# 542
return t1;}}}
# 520
if(({Cyc_Tcutil_is_pointer_type(t1);})){
# 556
if(({Cyc_Tcutil_is_pointer_type(t2);})&&({int(*_tmpE2)(void*,void*)=Cyc_Unify_unify;void*_tmpE3=({Cyc_Tcutil_pointer_elt_type(t1);});void*_tmpE4=({Cyc_Tcutil_pointer_elt_type(t2);});_tmpE2(_tmpE3,_tmpE4);})){
# 558
if(({Cyc_Tcutil_is_fat_pointer_type(t2);})){
({void*_tmpE5=0U;({unsigned _tmp72B=e1->loc;struct _fat_ptr _tmp72A=({const char*_tmpE6="coercing fat pointer to thin pointer to support subtraction";_tag_fat(_tmpE6,sizeof(char),60U);});Cyc_Tcutil_warn(_tmp72B,_tmp72A,_tag_fat(_tmpE5,sizeof(void*),0U));});});
({void(*_tmpE7)(struct Cyc_Absyn_Exp*,void*,enum Cyc_Absyn_Coercion)=Cyc_Tcutil_unchecked_cast;struct Cyc_Absyn_Exp*_tmpE8=e2;void*_tmpE9=({void*(*_tmpEA)(void*t,void*rgn,struct Cyc_Absyn_Tqual,void*zero_term)=Cyc_Absyn_star_type;void*_tmpEB=({Cyc_Tcutil_pointer_elt_type(t2);});void*_tmpEC=({Cyc_Tcutil_pointer_region(t2);});struct Cyc_Absyn_Tqual _tmpED=({Cyc_Absyn_empty_tqual(0U);});void*_tmpEE=Cyc_Absyn_false_type;_tmpEA(_tmpEB,_tmpEC,_tmpED,_tmpEE);});enum Cyc_Absyn_Coercion _tmpEF=Cyc_Absyn_Other_coercion;_tmpE7(_tmpE8,_tmpE9,_tmpEF);});}
# 558
({void*_tmpF0=0U;({unsigned _tmp72D=e1->loc;struct _fat_ptr _tmp72C=({const char*_tmpF1="thin pointer subtraction!";_tag_fat(_tmpF1,sizeof(char),26U);});Cyc_Tcutil_warn(_tmp72D,_tmp72C,_tag_fat(_tmpF0,sizeof(void*),0U));});});
# 566
return Cyc_Absyn_sint_type;}
# 556
({void*_tmpF2=0U;({unsigned _tmp72F=e1->loc;struct _fat_ptr _tmp72E=({const char*_tmpF3="coercing thin pointer to integer to support subtraction";_tag_fat(_tmpF3,sizeof(char),56U);});Cyc_Tcutil_warn(_tmp72F,_tmp72E,_tag_fat(_tmpF2,sizeof(void*),0U));});});
# 569
({Cyc_Tcutil_unchecked_cast(e1,Cyc_Absyn_sint_type,Cyc_Absyn_Other_coercion);});}
# 520
if(({Cyc_Tcutil_is_pointer_type(t2);})){
# 572
({void*_tmpF4=0U;({unsigned _tmp731=e1->loc;struct _fat_ptr _tmp730=({const char*_tmpF5="coercing pointer to integer to support subtraction";_tag_fat(_tmpF5,sizeof(char),51U);});Cyc_Tcutil_warn(_tmp731,_tmp730,_tag_fat(_tmpF4,sizeof(void*),0U));});});
({Cyc_Tcutil_unchecked_cast(e2,Cyc_Absyn_sint_type,Cyc_Absyn_Other_coercion);});}
# 520
return({Cyc_Tcexp_tcArithBinop(te,e1,e2,Cyc_Tcutil_is_numeric);});}
# 579
static void*Cyc_Tcexp_tcAnyBinop(struct Cyc_Tcenv_Tenv*te,unsigned loc,struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2){
void*t1=({Cyc_Tcutil_compress((void*)_check_null(e1->topt));});
void*t2=({Cyc_Tcutil_compress((void*)_check_null(e2->topt));});
if(({Cyc_Tcutil_is_numeric(e1);})&&({Cyc_Tcutil_is_numeric(e2);})){
({Cyc_Tcexp_arith_convert(te,e1,e2);});
return Cyc_Absyn_sint_type;}
# 582
if(
# 588
(int)({Cyc_Tcutil_type_kind(t1);})->kind == (int)Cyc_Absyn_BoxKind ||({int(*_tmpF7)(void*,void*)=Cyc_Unify_unify;void*_tmpF8=t1;void*_tmpF9=({void*(*_tmpFA)(struct Cyc_Core_Opt*k,struct Cyc_Core_Opt*tenv)=Cyc_Absyn_new_evar;struct Cyc_Core_Opt*_tmpFB=& Cyc_Tcutil_bko;struct Cyc_Core_Opt*_tmpFC=({Cyc_Tcenv_lookup_opt_type_vars(te);});_tmpFA(_tmpFB,_tmpFC);});_tmpF7(_tmpF8,_tmpF9);})){
# 590
if(({Cyc_Unify_unify(t1,t2);}))
return Cyc_Absyn_sint_type;
# 590
if(({Cyc_Tcutil_silent_castable(te,loc,t2,t1);})){
# 594
({Cyc_Tcutil_unchecked_cast(e2,t1,Cyc_Absyn_Other_coercion);});
return Cyc_Absyn_sint_type;}
# 590
if(({Cyc_Tcutil_silent_castable(te,loc,t1,t2);})){
# 598
({Cyc_Tcutil_unchecked_cast(e1,t2,Cyc_Absyn_Other_coercion);});
return Cyc_Absyn_sint_type;}
# 590
if(
# 601
({Cyc_Tcutil_zero_to_null(t2,e1);})||({Cyc_Tcutil_zero_to_null(t1,e2);}))
return Cyc_Absyn_sint_type;}
# 582
{struct _tuple0 _stmttmpB=({struct _tuple0 _tmp64E;({
# 606
void*_tmp733=({Cyc_Tcutil_compress(t1);});_tmp64E.f1=_tmp733;}),({void*_tmp732=({Cyc_Tcutil_compress(t2);});_tmp64E.f2=_tmp732;});_tmp64E;});
# 582
struct _tuple0 _tmpFD=_stmttmpB;void*_tmpFF;void*_tmpFE;switch(*((int*)_tmpFD.f1)){case 3U: if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmpFD.f2)->tag == 3U){_LL1: _tmpFE=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmpFD.f1)->f1).elt_type;_tmpFF=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmpFD.f2)->f1).elt_type;_LL2: {void*t1a=_tmpFE;void*t2a=_tmpFF;
# 608
if(({Cyc_Unify_unify(t1a,t2a);}))
return Cyc_Absyn_sint_type;
# 608
goto _LL0;}}else{goto _LL7;}case 0U: switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpFD.f1)->f1)){case 3U: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpFD.f2)->tag == 0U){if(((struct Cyc_Absyn_RgnHandleCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpFD.f2)->f1)->tag == 3U){_LL3: _LL4:
# 612
 return Cyc_Absyn_sint_type;}else{goto _LL7;}}else{goto _LL7;}case 4U: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpFD.f2)->tag == 0U){if(((struct Cyc_Absyn_XRgnHandleCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpFD.f2)->f1)->tag == 4U){_LL5: _LL6:
# 614
 return Cyc_Absyn_sint_type;}else{goto _LL7;}}else{goto _LL7;}default: goto _LL7;}default: _LL7: _LL8:
 goto _LL0;}_LL0:;}
# 617
({struct Cyc_String_pa_PrintArg_struct _tmp102=({struct Cyc_String_pa_PrintArg_struct _tmp650;_tmp650.tag=0U,({
struct _fat_ptr _tmp734=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t1);}));_tmp650.f1=_tmp734;});_tmp650;});struct Cyc_String_pa_PrintArg_struct _tmp103=({struct Cyc_String_pa_PrintArg_struct _tmp64F;_tmp64F.tag=0U,({struct _fat_ptr _tmp735=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t2);}));_tmp64F.f1=_tmp735;});_tmp64F;});void*_tmp100[2U];_tmp100[0]=& _tmp102,_tmp100[1]=& _tmp103;({unsigned _tmp737=loc;struct _fat_ptr _tmp736=({const char*_tmp101="comparison not allowed between %s and %s";_tag_fat(_tmp101,sizeof(char),41U);});Cyc_Tcutil_terr(_tmp737,_tmp736,_tag_fat(_tmp100,sizeof(void*),2U));});});
({Cyc_Unify_explain_failure();});
return({void*(*_tmp104)(struct Cyc_Core_Opt*)=Cyc_Absyn_wildtyp;struct Cyc_Core_Opt*_tmp105=({Cyc_Tcenv_lookup_opt_type_vars(te);});_tmp104(_tmp105);});}
# 623
static void*Cyc_Tcexp_tcEqPrimop(struct Cyc_Tcenv_Tenv*te,unsigned loc,struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2){
# 625
if(({void*_tmp738=({Cyc_Tcexp_tcAnyBinop(te,loc,e1,e2);});_tmp738 == Cyc_Absyn_sint_type;}))
return Cyc_Absyn_sint_type;
# 625
({struct Cyc_String_pa_PrintArg_struct _tmp109=({struct Cyc_String_pa_PrintArg_struct _tmp652;_tmp652.tag=0U,({
# 628
struct _fat_ptr _tmp739=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string((void*)_check_null(e1->topt));}));_tmp652.f1=_tmp739;});_tmp652;});struct Cyc_String_pa_PrintArg_struct _tmp10A=({struct Cyc_String_pa_PrintArg_struct _tmp651;_tmp651.tag=0U,({struct _fat_ptr _tmp73A=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string((void*)_check_null(e2->topt));}));_tmp651.f1=_tmp73A;});_tmp651;});void*_tmp107[2U];_tmp107[0]=& _tmp109,_tmp107[1]=& _tmp10A;({unsigned _tmp73C=loc;struct _fat_ptr _tmp73B=({const char*_tmp108="comparison not allowed between %s and %s";_tag_fat(_tmp108,sizeof(char),41U);});Cyc_Tcutil_terr(_tmp73C,_tmp73B,_tag_fat(_tmp107,sizeof(void*),2U));});});
({Cyc_Unify_explain_failure();});
return({void*(*_tmp10B)(struct Cyc_Core_Opt*)=Cyc_Absyn_wildtyp;struct Cyc_Core_Opt*_tmp10C=({Cyc_Tcenv_lookup_opt_type_vars(te);});_tmp10B(_tmp10C);});}
# 635
static void*Cyc_Tcexp_tcBinPrimop(struct Cyc_Tcenv_Tenv*te,unsigned loc,void**topt,enum Cyc_Absyn_Primop p,struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2){
# 637
enum Cyc_Absyn_Primop _tmp10E=p;switch(_tmp10E){case Cyc_Absyn_Plus: _LL1: _LL2:
 return({Cyc_Tcexp_tcPlus(te,e1,e2);});case Cyc_Absyn_Minus: _LL3: _LL4:
 return({Cyc_Tcexp_tcMinus(te,e1,e2);});case Cyc_Absyn_Times: _LL5: _LL6:
# 641
 goto _LL8;case Cyc_Absyn_Div: _LL7: _LL8:
 return({Cyc_Tcexp_tcArithBinop(te,e1,e2,Cyc_Tcutil_is_numeric);});case Cyc_Absyn_Mod: _LL9: _LLA:
# 644
 goto _LLC;case Cyc_Absyn_Bitand: _LLB: _LLC:
 goto _LLE;case Cyc_Absyn_Bitor: _LLD: _LLE:
 goto _LL10;case Cyc_Absyn_Bitxor: _LLF: _LL10:
 goto _LL12;case Cyc_Absyn_Bitlshift: _LL11: _LL12:
 goto _LL14;case Cyc_Absyn_Bitlrshift: _LL13: _LL14:
 return({Cyc_Tcexp_tcArithBinop(te,e1,e2,Cyc_Tcutil_is_integral);});case Cyc_Absyn_Eq: _LL15: _LL16:
# 653
 goto _LL18;case Cyc_Absyn_Neq: _LL17: _LL18:
 return({Cyc_Tcexp_tcEqPrimop(te,loc,e1,e2);});case Cyc_Absyn_Gt: _LL19: _LL1A:
# 656
 goto _LL1C;case Cyc_Absyn_Lt: _LL1B: _LL1C:
 goto _LL1E;case Cyc_Absyn_Gte: _LL1D: _LL1E:
 goto _LL20;case Cyc_Absyn_Lte: _LL1F: _LL20:
 return({Cyc_Tcexp_tcAnyBinop(te,loc,e1,e2);});default: _LL21: _LL22:
# 661
({void*_tmp10F=0U;({int(*_tmp73E)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp111)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp111;});struct _fat_ptr _tmp73D=({const char*_tmp110="bad binary primop";_tag_fat(_tmp110,sizeof(char),18U);});_tmp73E(_tmp73D,_tag_fat(_tmp10F,sizeof(void*),0U));});});}_LL0:;}
# 665
static void*Cyc_Tcexp_tcPrimop(struct Cyc_Tcenv_Tenv*te,unsigned loc,void**topt,enum Cyc_Absyn_Primop p,struct Cyc_List_List*es){
# 673
if((int)p == (int)2U &&({Cyc_List_length(es);})== 1){
struct Cyc_Absyn_Exp*e=(struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(es))->hd;
void*t=({Cyc_Tcexp_tcExp(te,topt,e);});
if(!({Cyc_Tcutil_is_numeric(e);}))
({struct Cyc_String_pa_PrintArg_struct _tmp115=({struct Cyc_String_pa_PrintArg_struct _tmp653;_tmp653.tag=0U,({struct _fat_ptr _tmp73F=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t);}));_tmp653.f1=_tmp73F;});_tmp653;});void*_tmp113[1U];_tmp113[0]=& _tmp115;({unsigned _tmp741=e->loc;struct _fat_ptr _tmp740=({const char*_tmp114="expecting numeric type but found %s";_tag_fat(_tmp114,sizeof(char),36U);});Cyc_Tcutil_terr(_tmp741,_tmp740,_tag_fat(_tmp113,sizeof(void*),1U));});});
# 676
return t;}
# 673
({Cyc_Tcexp_tcExpList(te,es);});{
# 681
void*t;
{int _stmttmpC=({Cyc_List_length(es);});int _tmp116=_stmttmpC;switch(_tmp116){case 0U: _LL1: _LL2:
 return({void*_tmp117=0U;({struct Cyc_Tcenv_Tenv*_tmp745=te;unsigned _tmp744=loc;void**_tmp743=topt;struct _fat_ptr _tmp742=({const char*_tmp118="primitive operator has 0 arguments";_tag_fat(_tmp118,sizeof(char),35U);});Cyc_Tcexp_expr_err(_tmp745,_tmp744,_tmp743,_tmp742,_tag_fat(_tmp117,sizeof(void*),0U));});});case 1U: _LL3: _LL4:
 t=({Cyc_Tcexp_tcUnPrimop(te,loc,topt,p,(struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(es))->hd);});goto _LL0;case 2U: _LL5: _LL6:
 t=({({struct Cyc_Tcenv_Tenv*_tmp74A=te;unsigned _tmp749=loc;void**_tmp748=topt;enum Cyc_Absyn_Primop _tmp747=p;struct Cyc_Absyn_Exp*_tmp746=(struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(es))->hd;Cyc_Tcexp_tcBinPrimop(_tmp74A,_tmp749,_tmp748,_tmp747,_tmp746,(struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(es->tl))->hd);});});goto _LL0;default: _LL7: _LL8:
 return({void*_tmp119=0U;({struct Cyc_Tcenv_Tenv*_tmp74E=te;unsigned _tmp74D=loc;void**_tmp74C=topt;struct _fat_ptr _tmp74B=({const char*_tmp11A="primitive operator has > 2 arguments";_tag_fat(_tmp11A,sizeof(char),37U);});Cyc_Tcexp_expr_err(_tmp74E,_tmp74D,_tmp74C,_tmp74B,_tag_fat(_tmp119,sizeof(void*),0U));});});}_LL0:;}
# 688
return t;}}struct _tuple18{struct Cyc_Absyn_Tqual f1;void*f2;};
# 691
static int Cyc_Tcexp_check_writable_aggr(unsigned loc,void*t0){
void*t=({Cyc_Tcutil_compress(t0);});
void*_tmp11C=t;struct Cyc_List_List*_tmp11D;struct Cyc_Absyn_Tqual _tmp11F;void*_tmp11E;struct Cyc_List_List*_tmp120;struct Cyc_Absyn_Datatypefield*_tmp121;struct Cyc_Absyn_Aggrdecl*_tmp122;switch(*((int*)_tmp11C)){case 0U: switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp11C)->f1)){case 22U: if(((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp11C)->f1)->f1).KnownAggr).tag == 2){_LL1: _tmp122=*((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp11C)->f1)->f1).KnownAggr).val;_LL2: {struct Cyc_Absyn_Aggrdecl*ad=_tmp122;
# 695
if(ad->impl == 0){
({void*_tmp123=0U;({unsigned _tmp750=loc;struct _fat_ptr _tmp74F=({const char*_tmp124="attempt to write an abstract aggregate";_tag_fat(_tmp124,sizeof(char),39U);});Cyc_Tcutil_terr(_tmp750,_tmp74F,_tag_fat(_tmp123,sizeof(void*),0U));});});
return 0;}else{
# 699
_tmp120=((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->fields;goto _LL4;}}}else{goto _LLB;}case 21U: if(((((struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp11C)->f1)->f1).KnownDatatypefield).tag == 2){_LL5: _tmp121=(((((struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp11C)->f1)->f1).KnownDatatypefield).val).f2;_LL6: {struct Cyc_Absyn_Datatypefield*df=_tmp121;
# 711
{struct Cyc_List_List*fs=df->typs;for(0;fs != 0;fs=fs->tl){
struct _tuple18*_stmttmpD=(struct _tuple18*)fs->hd;void*_tmp129;struct Cyc_Absyn_Tqual _tmp128;_LLE: _tmp128=_stmttmpD->f1;_tmp129=_stmttmpD->f2;_LLF: {struct Cyc_Absyn_Tqual tq=_tmp128;void*t=_tmp129;
if(tq.real_const){
({struct Cyc_String_pa_PrintArg_struct _tmp12C=({struct Cyc_String_pa_PrintArg_struct _tmp654;_tmp654.tag=0U,({struct _fat_ptr _tmp751=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(df->name);}));_tmp654.f1=_tmp751;});_tmp654;});void*_tmp12A[1U];_tmp12A[0]=& _tmp12C;({unsigned _tmp753=loc;struct _fat_ptr _tmp752=({const char*_tmp12B="attempt to over-write a datatype field (%s) with a const member";_tag_fat(_tmp12B,sizeof(char),64U);});Cyc_Tcutil_terr(_tmp753,_tmp752,_tag_fat(_tmp12A,sizeof(void*),1U));});});
return 0;}
# 713
if(!({Cyc_Tcexp_check_writable_aggr(loc,t);}))
# 717
return 0;}}}
# 719
return 1;}}else{goto _LLB;}default: goto _LLB;}case 7U: _LL3: _tmp120=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp11C)->f2;_LL4: {struct Cyc_List_List*fs=_tmp120;
# 701
for(0;fs != 0;fs=fs->tl){
struct Cyc_Absyn_Aggrfield*f=(struct Cyc_Absyn_Aggrfield*)fs->hd;
if((f->tq).real_const){
({struct Cyc_String_pa_PrintArg_struct _tmp127=({struct Cyc_String_pa_PrintArg_struct _tmp655;_tmp655.tag=0U,_tmp655.f1=(struct _fat_ptr)((struct _fat_ptr)*f->name);_tmp655;});void*_tmp125[1U];_tmp125[0]=& _tmp127;({unsigned _tmp755=loc;struct _fat_ptr _tmp754=({const char*_tmp126="attempt to over-write an aggregate with const member %s";_tag_fat(_tmp126,sizeof(char),56U);});Cyc_Tcutil_terr(_tmp755,_tmp754,_tag_fat(_tmp125,sizeof(void*),1U));});});
return 0;}
# 703
if(!({Cyc_Tcexp_check_writable_aggr(loc,f->type);}))
# 707
return 0;}
# 709
return 1;}case 4U: _LL7: _tmp11E=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp11C)->f1).elt_type;_tmp11F=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp11C)->f1).tq;_LL8: {void*elt_type=_tmp11E;struct Cyc_Absyn_Tqual tq=_tmp11F;
# 721
if(tq.real_const){
({void*_tmp12D=0U;({unsigned _tmp757=loc;struct _fat_ptr _tmp756=({const char*_tmp12E="attempt to over-write a const array";_tag_fat(_tmp12E,sizeof(char),36U);});Cyc_Tcutil_terr(_tmp757,_tmp756,_tag_fat(_tmp12D,sizeof(void*),0U));});});
return 0;}
# 721
return({Cyc_Tcexp_check_writable_aggr(loc,elt_type);});}case 6U: _LL9: _tmp11D=((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp11C)->f1;_LLA: {struct Cyc_List_List*fs=_tmp11D;
# 727
for(0;fs != 0;fs=fs->tl){
struct _tuple18*_stmttmpE=(struct _tuple18*)fs->hd;void*_tmp130;struct Cyc_Absyn_Tqual _tmp12F;_LL11: _tmp12F=_stmttmpE->f1;_tmp130=_stmttmpE->f2;_LL12: {struct Cyc_Absyn_Tqual tq=_tmp12F;void*t=_tmp130;
if(tq.real_const){
({void*_tmp131=0U;({unsigned _tmp759=loc;struct _fat_ptr _tmp758=({const char*_tmp132="attempt to over-write a tuple field with a const member";_tag_fat(_tmp132,sizeof(char),56U);});Cyc_Tcutil_terr(_tmp759,_tmp758,_tag_fat(_tmp131,sizeof(void*),0U));});});
return 0;}
# 729
if(!({Cyc_Tcexp_check_writable_aggr(loc,t);}))
# 733
return 0;}}
# 735
return 1;}default: _LLB: _LLC:
 return 1;}_LL0:;}
# 743
static void Cyc_Tcexp_check_writable(struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Exp*e){
# 746
if(!({Cyc_Tcexp_check_writable_aggr(e->loc,(void*)_check_null(e->topt));}))return;{void*_stmttmpF=e->r;void*_tmp134=_stmttmpF;struct Cyc_Absyn_Exp*_tmp135;struct Cyc_Absyn_Exp*_tmp136;struct Cyc_Absyn_Exp*_tmp137;struct _fat_ptr*_tmp139;struct Cyc_Absyn_Exp*_tmp138;struct _fat_ptr*_tmp13B;struct Cyc_Absyn_Exp*_tmp13A;struct Cyc_Absyn_Exp*_tmp13D;struct Cyc_Absyn_Exp*_tmp13C;struct Cyc_Absyn_Vardecl*_tmp13E;struct Cyc_Absyn_Vardecl*_tmp13F;struct Cyc_Absyn_Vardecl*_tmp140;struct Cyc_Absyn_Vardecl*_tmp141;switch(*((int*)_tmp134)){case 1U: switch(*((int*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp134)->f1)){case 3U: _LL1: _tmp141=((struct Cyc_Absyn_Param_b_Absyn_Binding_struct*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp134)->f1)->f1;_LL2: {struct Cyc_Absyn_Vardecl*vd=_tmp141;
# 748
_tmp140=vd;goto _LL4;}case 4U: _LL3: _tmp140=((struct Cyc_Absyn_Local_b_Absyn_Binding_struct*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp134)->f1)->f1;_LL4: {struct Cyc_Absyn_Vardecl*vd=_tmp140;
_tmp13F=vd;goto _LL6;}case 5U: _LL5: _tmp13F=((struct Cyc_Absyn_Pat_b_Absyn_Binding_struct*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp134)->f1)->f1;_LL6: {struct Cyc_Absyn_Vardecl*vd=_tmp13F;
_tmp13E=vd;goto _LL8;}case 1U: _LL7: _tmp13E=((struct Cyc_Absyn_Global_b_Absyn_Binding_struct*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp134)->f1)->f1;_LL8: {struct Cyc_Absyn_Vardecl*vd=_tmp13E;
if(!(vd->tq).real_const)return;goto _LL0;}default: goto _LL15;}case 23U: _LL9: _tmp13C=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp134)->f1;_tmp13D=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp134)->f2;_LLA: {struct Cyc_Absyn_Exp*e1=_tmp13C;struct Cyc_Absyn_Exp*e2=_tmp13D;
# 753
{void*_stmttmp10=({Cyc_Tcutil_compress((void*)_check_null(e1->topt));});void*_tmp142=_stmttmp10;struct Cyc_List_List*_tmp143;struct Cyc_Absyn_Tqual _tmp144;struct Cyc_Absyn_Tqual _tmp145;switch(*((int*)_tmp142)){case 3U: _LL18: _tmp145=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp142)->f1).elt_tq;_LL19: {struct Cyc_Absyn_Tqual tq=_tmp145;
_tmp144=tq;goto _LL1B;}case 4U: _LL1A: _tmp144=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp142)->f1).tq;_LL1B: {struct Cyc_Absyn_Tqual tq=_tmp144;
if(!tq.real_const)return;goto _LL17;}case 6U: _LL1C: _tmp143=((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp142)->f1;_LL1D: {struct Cyc_List_List*ts=_tmp143;
# 757
struct _tuple17 _stmttmp11=({Cyc_Evexp_eval_const_uint_exp(e2);});int _tmp147;unsigned _tmp146;_LL21: _tmp146=_stmttmp11.f1;_tmp147=_stmttmp11.f2;_LL22: {unsigned i=_tmp146;int known=_tmp147;
if(!known){
({void*_tmp148=0U;({unsigned _tmp75B=e->loc;struct _fat_ptr _tmp75A=({const char*_tmp149="tuple projection cannot use sizeof or offsetof";_tag_fat(_tmp149,sizeof(char),47U);});Cyc_Tcutil_terr(_tmp75B,_tmp75A,_tag_fat(_tmp148,sizeof(void*),0U));});});
return;}
# 758
{struct _handler_cons _tmp14A;_push_handler(& _tmp14A);{int _tmp14C=0;if(setjmp(_tmp14A.handler))_tmp14C=1;if(!_tmp14C){
# 763
{struct _tuple18*_stmttmp12=({({struct _tuple18*(*_tmp75D)(struct Cyc_List_List*x,int n)=({struct _tuple18*(*_tmp14E)(struct Cyc_List_List*x,int n)=(struct _tuple18*(*)(struct Cyc_List_List*x,int n))Cyc_List_nth;_tmp14E;});struct Cyc_List_List*_tmp75C=ts;_tmp75D(_tmp75C,(int)i);});});struct Cyc_Absyn_Tqual _tmp14D;_LL24: _tmp14D=_stmttmp12->f1;_LL25: {struct Cyc_Absyn_Tqual tq=_tmp14D;
if(!tq.real_const){_npop_handler(0U);return;}}}
# 763
;_pop_handler();}else{void*_tmp14B=(void*)Cyc_Core_get_exn_thrown();void*_tmp14F=_tmp14B;void*_tmp150;if(((struct Cyc_List_Nth_exn_struct*)_tmp14F)->tag == Cyc_List_Nth){_LL27: _LL28:
# 765
 return;}else{_LL29: _tmp150=_tmp14F;_LL2A: {void*exn=_tmp150;(int)_rethrow(exn);}}_LL26:;}}}
goto _LL17;}}default: _LL1E: _LL1F:
 goto _LL17;}_LL17:;}
# 769
goto _LL0;}case 21U: _LLB: _tmp13A=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp134)->f1;_tmp13B=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp134)->f2;_LLC: {struct Cyc_Absyn_Exp*e1=_tmp13A;struct _fat_ptr*f=_tmp13B;
# 771
{void*_stmttmp13=({Cyc_Tcutil_compress((void*)_check_null(e1->topt));});void*_tmp151=_stmttmp13;struct Cyc_List_List*_tmp152;struct Cyc_Absyn_Aggrdecl**_tmp153;switch(*((int*)_tmp151)){case 0U: if(((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp151)->f1)->tag == 22U){if(((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp151)->f1)->f1).KnownAggr).tag == 2){_LL2C: _tmp153=((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp151)->f1)->f1).KnownAggr).val;_LL2D: {struct Cyc_Absyn_Aggrdecl**adp=_tmp153;
# 773
struct Cyc_Absyn_Aggrfield*sf=
adp == 0?0:({Cyc_Absyn_lookup_decl_field(*adp,f);});
if(sf == 0 || !(sf->tq).real_const)return;goto _LL2B;}}else{goto _LL30;}}else{goto _LL30;}case 7U: _LL2E: _tmp152=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp151)->f2;_LL2F: {struct Cyc_List_List*fs=_tmp152;
# 778
struct Cyc_Absyn_Aggrfield*sf=({Cyc_Absyn_lookup_field(fs,f);});
if(sf == 0 || !(sf->tq).real_const)return;goto _LL2B;}default: _LL30: _LL31:
# 781
 goto _LL2B;}_LL2B:;}
# 783
goto _LL0;}case 22U: _LLD: _tmp138=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp134)->f1;_tmp139=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp134)->f2;_LLE: {struct Cyc_Absyn_Exp*e1=_tmp138;struct _fat_ptr*f=_tmp139;
# 785
{void*_stmttmp14=({Cyc_Tcutil_compress((void*)_check_null(e1->topt));});void*_tmp154=_stmttmp14;struct Cyc_Absyn_Tqual _tmp156;void*_tmp155;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp154)->tag == 3U){_LL33: _tmp155=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp154)->f1).elt_type;_tmp156=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp154)->f1).elt_tq;_LL34: {void*elt_typ=_tmp155;struct Cyc_Absyn_Tqual tq=_tmp156;
# 787
if(!tq.real_const){
void*_stmttmp15=({Cyc_Tcutil_compress(elt_typ);});void*_tmp157=_stmttmp15;struct Cyc_List_List*_tmp158;struct Cyc_Absyn_Aggrdecl**_tmp159;switch(*((int*)_tmp157)){case 0U: if(((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp157)->f1)->tag == 22U){if(((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp157)->f1)->f1).KnownAggr).tag == 2){_LL38: _tmp159=((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp157)->f1)->f1).KnownAggr).val;_LL39: {struct Cyc_Absyn_Aggrdecl**adp=_tmp159;
# 790
struct Cyc_Absyn_Aggrfield*sf=
adp == 0?0:({Cyc_Absyn_lookup_decl_field(*adp,f);});
if(sf == 0 || !(sf->tq).real_const)return;goto _LL37;}}else{goto _LL3C;}}else{goto _LL3C;}case 7U: _LL3A: _tmp158=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp157)->f2;_LL3B: {struct Cyc_List_List*fs=_tmp158;
# 795
struct Cyc_Absyn_Aggrfield*sf=({Cyc_Absyn_lookup_field(fs,f);});
if(sf == 0 || !(sf->tq).real_const)return;goto _LL37;}default: _LL3C: _LL3D:
# 798
 goto _LL37;}_LL37:;}
# 787
goto _LL32;}}else{_LL35: _LL36:
# 802
 goto _LL32;}_LL32:;}
# 804
goto _LL0;}case 20U: _LLF: _tmp137=((struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_tmp134)->f1;_LL10: {struct Cyc_Absyn_Exp*e1=_tmp137;
# 806
{void*_stmttmp16=({Cyc_Tcutil_compress((void*)_check_null(e1->topt));});void*_tmp15A=_stmttmp16;struct Cyc_Absyn_Tqual _tmp15B;struct Cyc_Absyn_Tqual _tmp15C;switch(*((int*)_tmp15A)){case 3U: _LL3F: _tmp15C=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp15A)->f1).elt_tq;_LL40: {struct Cyc_Absyn_Tqual tq=_tmp15C;
_tmp15B=tq;goto _LL42;}case 4U: _LL41: _tmp15B=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp15A)->f1).tq;_LL42: {struct Cyc_Absyn_Tqual tq=_tmp15B;
if(!tq.real_const)return;goto _LL3E;}default: _LL43: _LL44:
 goto _LL3E;}_LL3E:;}
# 811
goto _LL0;}case 12U: _LL11: _tmp136=((struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*)_tmp134)->f1;_LL12: {struct Cyc_Absyn_Exp*e1=_tmp136;
_tmp135=e1;goto _LL14;}case 13U: _LL13: _tmp135=((struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*)_tmp134)->f1;_LL14: {struct Cyc_Absyn_Exp*e1=_tmp135;
({Cyc_Tcexp_check_writable(te,e1);});return;}default: _LL15: _LL16:
 goto _LL0;}_LL0:;}
# 816
({struct Cyc_String_pa_PrintArg_struct _tmp15F=({struct Cyc_String_pa_PrintArg_struct _tmp656;_tmp656.tag=0U,({struct _fat_ptr _tmp75E=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_exp2string(e);}));_tmp656.f1=_tmp75E;});_tmp656;});void*_tmp15D[1U];_tmp15D[0]=& _tmp15F;({unsigned _tmp760=e->loc;struct _fat_ptr _tmp75F=({const char*_tmp15E="attempt to write a const location: %s";_tag_fat(_tmp15E,sizeof(char),38U);});Cyc_Tcutil_terr(_tmp760,_tmp75F,_tag_fat(_tmp15D,sizeof(void*),1U));});});}
# 819
static void*Cyc_Tcexp_tcIncrement(struct Cyc_Tcenv_Tenv*te,unsigned loc,void**topt,struct Cyc_Absyn_Exp*e,enum Cyc_Absyn_Incrementor i){
# 822
({void*(*_tmp161)(struct Cyc_Tcenv_Tenv*te,void**topt,struct Cyc_Absyn_Exp*e)=Cyc_Tcexp_tcExpNoPromote;struct Cyc_Tcenv_Tenv*_tmp162=({Cyc_Tcenv_enter_lhs(te);});void**_tmp163=0;struct Cyc_Absyn_Exp*_tmp164=e;_tmp161(_tmp162,_tmp163,_tmp164);});
if(!({Cyc_Absyn_is_lvalue(e);}))
return({void*_tmp165=0U;({struct Cyc_Tcenv_Tenv*_tmp764=te;unsigned _tmp763=loc;void**_tmp762=topt;struct _fat_ptr _tmp761=({const char*_tmp166="increment/decrement of non-lvalue";_tag_fat(_tmp166,sizeof(char),34U);});Cyc_Tcexp_expr_err(_tmp764,_tmp763,_tmp762,_tmp761,_tag_fat(_tmp165,sizeof(void*),0U));});});
# 823
({Cyc_Tcexp_check_writable(te,e);});{
# 826
void*t=(void*)_check_null(e->topt);
# 828
if(!({Cyc_Tcutil_is_numeric(e);})){
void*telt=Cyc_Absyn_void_type;
if(({Cyc_Tcutil_is_fat_pointer_type_elt(t,& telt);})||
({Cyc_Tcutil_is_zero_pointer_type_elt(t,& telt);})&&((int)i == (int)0U ||(int)i == (int)1U)){
if(!({int(*_tmp167)(struct Cyc_Absyn_Kind*,struct Cyc_Absyn_Kind*)=Cyc_Tcutil_kind_leq;struct Cyc_Absyn_Kind*_tmp168=({Cyc_Tcutil_type_kind(telt);});struct Cyc_Absyn_Kind*_tmp169=& Cyc_Tcutil_tmk;_tmp167(_tmp168,_tmp169);}))
({void*_tmp16A=0U;({unsigned _tmp766=e->loc;struct _fat_ptr _tmp765=({const char*_tmp16B="can't perform arithmetic on abstract pointer type";_tag_fat(_tmp16B,sizeof(char),50U);});Cyc_Tcutil_terr(_tmp766,_tmp765,_tag_fat(_tmp16A,sizeof(void*),0U));});});
# 832
if(({Cyc_Tcutil_is_noalias_pointer(t,0);}))
# 835
({void*_tmp16C=0U;({unsigned _tmp768=e->loc;struct _fat_ptr _tmp767=({const char*_tmp16D="can't perform arithmetic on non-aliasing pointer type";_tag_fat(_tmp16D,sizeof(char),54U);});Cyc_Tcutil_terr(_tmp768,_tmp767,_tag_fat(_tmp16C,sizeof(void*),0U));});});}else{
# 837
({struct Cyc_String_pa_PrintArg_struct _tmp170=({struct Cyc_String_pa_PrintArg_struct _tmp657;_tmp657.tag=0U,({struct _fat_ptr _tmp769=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t);}));_tmp657.f1=_tmp769;});_tmp657;});void*_tmp16E[1U];_tmp16E[0]=& _tmp170;({unsigned _tmp76B=e->loc;struct _fat_ptr _tmp76A=({const char*_tmp16F="expecting arithmetic or ? type but found %s";_tag_fat(_tmp16F,sizeof(char),44U);});Cyc_Tcutil_terr(_tmp76B,_tmp76A,_tag_fat(_tmp16E,sizeof(void*),1U));});});}}
# 828
return t;}}
# 843
static void*Cyc_Tcexp_tcConditional(struct Cyc_Tcenv_Tenv*te,unsigned loc,void**topt,struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2,struct Cyc_Absyn_Exp*e3){
# 845
({void(*_tmp172)(struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Exp*e,struct _fat_ptr msg_part)=Cyc_Tcexp_tcTest;struct Cyc_Tcenv_Tenv*_tmp173=({Cyc_Tcenv_clear_abstract_val_ok(te);});struct Cyc_Absyn_Exp*_tmp174=e1;struct _fat_ptr _tmp175=({const char*_tmp176="conditional expression";_tag_fat(_tmp176,sizeof(char),23U);});_tmp172(_tmp173,_tmp174,_tmp175);});
({Cyc_Tcexp_tcExp(te,topt,e2);});
({Cyc_Tcexp_tcExp(te,topt,e3);});{
void*t;
if(({Cyc_Tcenv_abstract_val_ok(te);}))
t=({void*(*_tmp177)(struct Cyc_Core_Opt*k,struct Cyc_Core_Opt*tenv)=Cyc_Absyn_new_evar;struct Cyc_Core_Opt*_tmp178=& Cyc_Tcutil_tako;struct Cyc_Core_Opt*_tmp179=({Cyc_Tcenv_lookup_opt_type_vars(te);});_tmp177(_tmp178,_tmp179);});else{
# 852
t=({void*(*_tmp17A)(struct Cyc_Core_Opt*k,struct Cyc_Core_Opt*tenv)=Cyc_Absyn_new_evar;struct Cyc_Core_Opt*_tmp17B=& Cyc_Tcutil_tmko;struct Cyc_Core_Opt*_tmp17C=({Cyc_Tcenv_lookup_opt_type_vars(te);});_tmp17A(_tmp17B,_tmp17C);});}{
struct Cyc_List_List l1=({struct Cyc_List_List _tmp65B;_tmp65B.hd=e3,_tmp65B.tl=0;_tmp65B;});
struct Cyc_List_List l2=({struct Cyc_List_List _tmp65A;_tmp65A.hd=e2,_tmp65A.tl=& l1;_tmp65A;});
if(!({Cyc_Tcutil_coerce_list(te,t,& l2);})){
({struct Cyc_String_pa_PrintArg_struct _tmp17F=({struct Cyc_String_pa_PrintArg_struct _tmp659;_tmp659.tag=0U,({
struct _fat_ptr _tmp76C=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string((void*)_check_null(e2->topt));}));_tmp659.f1=_tmp76C;});_tmp659;});struct Cyc_String_pa_PrintArg_struct _tmp180=({struct Cyc_String_pa_PrintArg_struct _tmp658;_tmp658.tag=0U,({struct _fat_ptr _tmp76D=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string((void*)_check_null(e3->topt));}));_tmp658.f1=_tmp76D;});_tmp658;});void*_tmp17D[2U];_tmp17D[0]=& _tmp17F,_tmp17D[1]=& _tmp180;({unsigned _tmp76F=loc;struct _fat_ptr _tmp76E=({const char*_tmp17E="conditional clause types do not match: %s != %s";_tag_fat(_tmp17E,sizeof(char),48U);});Cyc_Tcutil_terr(_tmp76F,_tmp76E,_tag_fat(_tmp17D,sizeof(void*),2U));});});
({Cyc_Unify_explain_failure();});}
# 855
return t;}}}
# 864
static void*Cyc_Tcexp_tcAnd(struct Cyc_Tcenv_Tenv*te,unsigned loc,struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2){
# 866
({({struct Cyc_Tcenv_Tenv*_tmp771=te;struct Cyc_Absyn_Exp*_tmp770=e1;Cyc_Tcexp_tcTest(_tmp771,_tmp770,({const char*_tmp182="logical-and expression";_tag_fat(_tmp182,sizeof(char),23U);}));});});
({({struct Cyc_Tcenv_Tenv*_tmp773=te;struct Cyc_Absyn_Exp*_tmp772=e2;Cyc_Tcexp_tcTest(_tmp773,_tmp772,({const char*_tmp183="logical-and expression";_tag_fat(_tmp183,sizeof(char),23U);}));});});
return Cyc_Absyn_sint_type;}
# 872
static void*Cyc_Tcexp_tcOr(struct Cyc_Tcenv_Tenv*te,unsigned loc,struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2){
# 874
({({struct Cyc_Tcenv_Tenv*_tmp775=te;struct Cyc_Absyn_Exp*_tmp774=e1;Cyc_Tcexp_tcTest(_tmp775,_tmp774,({const char*_tmp185="logical-or expression";_tag_fat(_tmp185,sizeof(char),22U);}));});});
({({struct Cyc_Tcenv_Tenv*_tmp777=te;struct Cyc_Absyn_Exp*_tmp776=e2;Cyc_Tcexp_tcTest(_tmp777,_tmp776,({const char*_tmp186="logical-or expression";_tag_fat(_tmp186,sizeof(char),22U);}));});});
return Cyc_Absyn_sint_type;}
# 880
static void*Cyc_Tcexp_tcAssignOp(struct Cyc_Tcenv_Tenv*te,unsigned loc,void**topt,struct Cyc_Absyn_Exp*e1,struct Cyc_Core_Opt*po,struct Cyc_Absyn_Exp*e2){
# 887
({void*(*_tmp188)(struct Cyc_Tcenv_Tenv*te,void**topt,struct Cyc_Absyn_Exp*e)=Cyc_Tcexp_tcExpNoPromote;struct Cyc_Tcenv_Tenv*_tmp189=({struct Cyc_Tcenv_Tenv*(*_tmp18A)(struct Cyc_Tcenv_Tenv*)=Cyc_Tcenv_enter_lhs;struct Cyc_Tcenv_Tenv*_tmp18B=({Cyc_Tcenv_enter_notreadctxt(te);});_tmp18A(_tmp18B);});void**_tmp18C=0;struct Cyc_Absyn_Exp*_tmp18D=e1;_tmp188(_tmp189,_tmp18C,_tmp18D);});{
void*t1=(void*)_check_null(e1->topt);
({Cyc_Tcexp_tcExp(te,& t1,e2);});{
void*t2=(void*)_check_null(e2->topt);
# 892
{void*_stmttmp17=({Cyc_Tcutil_compress(t1);});void*_tmp18E=_stmttmp17;if(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp18E)->tag == 4U){_LL1: _LL2:
({void*_tmp18F=0U;({unsigned _tmp779=loc;struct _fat_ptr _tmp778=({const char*_tmp190="cannot assign to an array";_tag_fat(_tmp190,sizeof(char),26U);});Cyc_Tcutil_terr(_tmp779,_tmp778,_tag_fat(_tmp18F,sizeof(void*),0U));});});goto _LL0;}else{_LL3: _LL4:
 goto _LL0;}_LL0:;}
# 897
if(!({int(*_tmp191)(struct Cyc_Absyn_Kind*,struct Cyc_Absyn_Kind*)=Cyc_Tcutil_kind_leq;struct Cyc_Absyn_Kind*_tmp192=({Cyc_Tcutil_type_kind(t1);});struct Cyc_Absyn_Kind*_tmp193=& Cyc_Tcutil_tmk;_tmp191(_tmp192,_tmp193);}))
({void*_tmp194=0U;({unsigned _tmp77B=loc;struct _fat_ptr _tmp77A=({const char*_tmp195="type is abstract (can't determine size).";_tag_fat(_tmp195,sizeof(char),41U);});Cyc_Tcutil_terr(_tmp77B,_tmp77A,_tag_fat(_tmp194,sizeof(void*),0U));});});
# 897
if(!({Cyc_Absyn_is_lvalue(e1);}))
# 902
return({void*_tmp196=0U;({struct Cyc_Tcenv_Tenv*_tmp77F=te;unsigned _tmp77E=loc;void**_tmp77D=topt;struct _fat_ptr _tmp77C=({const char*_tmp197="assignment to non-lvalue";_tag_fat(_tmp197,sizeof(char),25U);});Cyc_Tcexp_expr_err(_tmp77F,_tmp77E,_tmp77D,_tmp77C,_tag_fat(_tmp196,sizeof(void*),0U));});});
# 897
({Cyc_Tcexp_check_writable(te,e1);});
# 904
if(po == 0){
if(({Cyc_Tcutil_is_noalias_pointer_or_aggr(t2);})&& !({Cyc_Tcutil_is_noalias_path(e2);}))
({void*_tmp198=0U;({unsigned _tmp781=e2->loc;struct _fat_ptr _tmp780=({const char*_tmp199="Cannot consume non-unique paths; do swap instead";_tag_fat(_tmp199,sizeof(char),49U);});Cyc_Tcutil_terr(_tmp781,_tmp780,_tag_fat(_tmp198,sizeof(void*),0U));});});
# 905
if(!({Cyc_Tcutil_coerce_assign(te,e2,t1);})){
# 908
void*result=({struct Cyc_String_pa_PrintArg_struct _tmp19C=({struct Cyc_String_pa_PrintArg_struct _tmp65D;_tmp65D.tag=0U,({
struct _fat_ptr _tmp782=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t1);}));_tmp65D.f1=_tmp782;});_tmp65D;});struct Cyc_String_pa_PrintArg_struct _tmp19D=({struct Cyc_String_pa_PrintArg_struct _tmp65C;_tmp65C.tag=0U,({struct _fat_ptr _tmp783=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t2);}));_tmp65C.f1=_tmp783;});_tmp65C;});void*_tmp19A[2U];_tmp19A[0]=& _tmp19C,_tmp19A[1]=& _tmp19D;({struct Cyc_Tcenv_Tenv*_tmp787=te;unsigned _tmp786=loc;void**_tmp785=topt;struct _fat_ptr _tmp784=({const char*_tmp19B="type mismatch: %s != %s";_tag_fat(_tmp19B,sizeof(char),24U);});Cyc_Tcexp_expr_err(_tmp787,_tmp786,_tmp785,_tmp784,_tag_fat(_tmp19A,sizeof(void*),2U));});});
({Cyc_Unify_unify(t1,t2);});
({Cyc_Unify_explain_failure();});
return result;}}else{
# 915
enum Cyc_Absyn_Primop p=(enum Cyc_Absyn_Primop)po->v;
struct Cyc_Absyn_Exp*e1copy=({Cyc_Absyn_copy_exp(e1);});
void*t_result=({Cyc_Tcexp_tcBinPrimop(te,loc,0,p,e1copy,e2);});
if((!({Cyc_Unify_unify(t_result,t1);})&&({Cyc_Tcutil_is_arithmetic_type(t_result);}))&&({Cyc_Tcutil_is_arithmetic_type(t1);}))
t_result=t1;else{
if(!(({Cyc_Unify_unify(t_result,t1);})||({Cyc_Tcutil_is_arithmetic_type(t_result);})&&({Cyc_Tcutil_is_arithmetic_type(t1);}))){
void*result=({struct Cyc_String_pa_PrintArg_struct _tmp1A0=({struct Cyc_String_pa_PrintArg_struct _tmp65F;_tmp65F.tag=0U,({
# 923
struct _fat_ptr _tmp788=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t1);}));_tmp65F.f1=_tmp788;});_tmp65F;});struct Cyc_String_pa_PrintArg_struct _tmp1A1=({struct Cyc_String_pa_PrintArg_struct _tmp65E;_tmp65E.tag=0U,({
struct _fat_ptr _tmp789=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t2);}));_tmp65E.f1=_tmp789;});_tmp65E;});void*_tmp19E[2U];_tmp19E[0]=& _tmp1A0,_tmp19E[1]=& _tmp1A1;({struct Cyc_Tcenv_Tenv*_tmp78D=te;unsigned _tmp78C=loc;void**_tmp78B=topt;struct _fat_ptr _tmp78A=({const char*_tmp19F="Cannot use this operator in an assignment when the arguments have types %s and %s";_tag_fat(_tmp19F,sizeof(char),82U);});Cyc_Tcexp_expr_err(_tmp78D,_tmp78C,_tmp78B,_tmp78A,_tag_fat(_tmp19E,sizeof(void*),2U));});});
({Cyc_Unify_unify(t_result,t1);});
({Cyc_Unify_explain_failure();});
return result;}}
# 918
return t_result;}
# 931
return t1;}}}
# 935
static void*Cyc_Tcexp_tcSpawn(struct Cyc_Tcenv_Tenv*te,unsigned loc,void**topt,struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2){
# 938
{void*_stmttmp18=e2->r;void*_tmp1A3=_stmttmp18;struct Cyc_List_List*_tmp1A5;struct Cyc_Absyn_Exp*_tmp1A4;if(((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp1A3)->tag == 10U){_LL1: _tmp1A4=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp1A3)->f1;_tmp1A5=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp1A3)->f2;_LL2: {struct Cyc_Absyn_Exp*fn=_tmp1A4;struct Cyc_List_List*es=_tmp1A5;
goto _LL0;}}else{_LL3: _LL4:
# 941
({void*_tmp1A6=0U;({unsigned _tmp78F=loc;struct _fat_ptr _tmp78E=({const char*_tmp1A7="Cannot spawn a thread with an arbitrary expression. A function call is required here.";_tag_fat(_tmp1A7,sizeof(char),86U);});Cyc_Tcutil_terr(_tmp78F,_tmp78E,_tag_fat(_tmp1A6,sizeof(void*),0U));});});
return Cyc_Absyn_void_type;}_LL0:;}{
# 945
void*t1=({void*(*_tmp1D6)(struct Cyc_Tcenv_Tenv*,void**,struct Cyc_Absyn_Exp*)=Cyc_Tcexp_tcExp;struct Cyc_Tcenv_Tenv*_tmp1D7=({Cyc_Tcenv_clear_abstract_val_ok(te);});void**_tmp1D8=0;struct Cyc_Absyn_Exp*_tmp1D9=e1;_tmp1D6(_tmp1D7,_tmp1D8,_tmp1D9);});
void*t2=({void*(*_tmp1D2)(struct Cyc_Tcenv_Tenv*,void**,struct Cyc_Absyn_Exp*)=Cyc_Tcexp_tcExp;struct Cyc_Tcenv_Tenv*_tmp1D3=({Cyc_Tcenv_clear_abstract_val_ok(te);});void**_tmp1D4=0;struct Cyc_Absyn_Exp*_tmp1D5=e2;_tmp1D2(_tmp1D3,_tmp1D4,_tmp1D5);});
# 948
if(!({Cyc_Unify_unify(Cyc_Absyn_uint_type,t1);})){
({struct Cyc_String_pa_PrintArg_struct _tmp1AA=({struct Cyc_String_pa_PrintArg_struct _tmp660;_tmp660.tag=0U,({struct _fat_ptr _tmp790=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t1);}));_tmp660.f1=_tmp790;});_tmp660;});void*_tmp1A8[1U];_tmp1A8[0]=& _tmp1AA;({unsigned _tmp792=loc;struct _fat_ptr _tmp791=({const char*_tmp1A9="The parameter to spawn must be an unsigned integer denoting the stack size of the new thread. Found type :%s";_tag_fat(_tmp1A9,sizeof(char),109U);});Cyc_Tcutil_terr(_tmp792,_tmp791,_tag_fat(_tmp1A8,sizeof(void*),1U));});});
return Cyc_Absyn_void_type;}
# 948
if(!({Cyc_Unify_unify(Cyc_Absyn_void_type,t2);})){
# 953
({struct Cyc_String_pa_PrintArg_struct _tmp1AD=({struct Cyc_String_pa_PrintArg_struct _tmp661;_tmp661.tag=0U,({
struct _fat_ptr _tmp793=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t2);}));_tmp661.f1=_tmp793;});_tmp661;});void*_tmp1AB[1U];_tmp1AB[0]=& _tmp1AD;({unsigned _tmp795=loc;struct _fat_ptr _tmp794=({const char*_tmp1AC="Thread function type is %s but expected `void'";_tag_fat(_tmp1AC,sizeof(char),47U);});Cyc_Tcutil_terr(_tmp795,_tmp794,_tag_fat(_tmp1AB,sizeof(void*),1U));});});
return Cyc_Absyn_void_type;}
# 948
{void*_stmttmp19=e2->r;void*_tmp1AE=_stmttmp19;struct Cyc_List_List*_tmp1B0;struct Cyc_Absyn_Exp*_tmp1AF;if(((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp1AE)->tag == 10U){_LL6: _tmp1AF=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp1AE)->f1;_tmp1B0=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp1AE)->f2;_LL7: {struct Cyc_Absyn_Exp*fn=_tmp1AF;struct Cyc_List_List*es=_tmp1B0;
# 959
void*t0=fn->topt;
if(t0 == 0)({void*_tmp1B1=0U;({int(*_tmp797)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp1B3)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp1B3;});struct _fat_ptr _tmp796=({const char*_tmp1B2="tcSpawn t0";_tag_fat(_tmp1B2,sizeof(char),11U);});_tmp797(_tmp796,_tag_fat(_tmp1B1,sizeof(void*),0U));});});{void*_tmp1B4=t0;void*_tmp1B6;void*_tmp1B5;if(_tmp1B4 != 0){if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp1B4)->tag == 3U){_LLB: _tmp1B5=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp1B4)->f1).elt_type;_tmp1B6=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp1B4)->f1).ptr_atts).rgn;_LLC: {void*t=_tmp1B5;void*nullable=_tmp1B6;
# 963
if(({Cyc_Tcutil_is_nullable_pointer_type(t0);}))
({struct Cyc_String_pa_PrintArg_struct _tmp1B9=({struct Cyc_String_pa_PrintArg_struct _tmp662;_tmp662.tag=0U,({
struct _fat_ptr _tmp798=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t0);}));_tmp662.f1=_tmp798;});_tmp662;});void*_tmp1B7[1U];_tmp1B7[0]=& _tmp1B9;({unsigned _tmp79A=loc;struct _fat_ptr _tmp799=({const char*_tmp1B8="Expected a definitely non-null function pointer but found type %s";_tag_fat(_tmp1B8,sizeof(char),66U);});Cyc_Tcutil_terr(_tmp79A,_tmp799,_tag_fat(_tmp1B7,sizeof(void*),1U));});});{
# 963
void*c=({Cyc_Tcutil_compress(t);});
# 967
{void*_tmp1BA=c;int _tmp1BE;struct Cyc_List_List*_tmp1BD;struct Cyc_List_List*_tmp1BC;void*_tmp1BB;if(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1BA)->tag == 5U){_LL10: _tmp1BB=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1BA)->f1).effect;_tmp1BC=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1BA)->f1).args;_tmp1BD=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1BA)->f1).throws;_tmp1BE=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1BA)->f1).reentrant;_LL11: {void*eff=_tmp1BB;struct Cyc_List_List*args_info=_tmp1BC;struct Cyc_List_List*throws=_tmp1BD;int reentrant=_tmp1BE;
# 970
if(!({Cyc_Absyn_is_reentrant(reentrant);}))
({struct Cyc_String_pa_PrintArg_struct _tmp1C1=({struct Cyc_String_pa_PrintArg_struct _tmp663;_tmp663.tag=0U,({
struct _fat_ptr _tmp79B=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(c);}));_tmp663.f1=_tmp79B;});_tmp663;});void*_tmp1BF[1U];_tmp1BF[0]=& _tmp1C1;({unsigned _tmp79D=loc;struct _fat_ptr _tmp79C=({const char*_tmp1C0="Thread function type is not @re_entrant: %s";_tag_fat(_tmp1C0,sizeof(char),44U);});Cyc_Tcutil_terr(_tmp79D,_tmp79C,_tag_fat(_tmp1BF,sizeof(void*),1U));});});
# 970
if(!({Cyc_Absyn_is_nothrow(throws);}))
# 975
({struct Cyc_String_pa_PrintArg_struct _tmp1C4=({struct Cyc_String_pa_PrintArg_struct _tmp664;_tmp664.tag=0U,({
struct _fat_ptr _tmp79E=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(c);}));_tmp664.f1=_tmp79E;});_tmp664;});void*_tmp1C2[1U];_tmp1C2[0]=& _tmp1C4;({unsigned _tmp7A0=loc;struct _fat_ptr _tmp79F=({const char*_tmp1C3="Thread function type is not @nothrow: %s";_tag_fat(_tmp1C3,sizeof(char),41U);});Cyc_Tcutil_terr(_tmp7A0,_tmp79F,_tag_fat(_tmp1C2,sizeof(void*),1U));});});
# 970
goto _LLF;}}else{_LL12: _LL13:
# 983
({struct Cyc_String_pa_PrintArg_struct _tmp1C8=({struct Cyc_String_pa_PrintArg_struct _tmp665;_tmp665.tag=0U,({
struct _fat_ptr _tmp7A1=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(c);}));_tmp665.f1=_tmp7A1;});_tmp665;});void*_tmp1C5[1U];_tmp1C5[0]=& _tmp1C8;({int(*_tmp7A3)(struct _fat_ptr,struct _fat_ptr)=({
# 983
int(*_tmp1C7)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp1C7;});struct _fat_ptr _tmp7A2=({const char*_tmp1C6=" tcexp:tcSpawn : Not a pointer type (internal error!): %s";_tag_fat(_tmp1C6,sizeof(char),58U);});_tmp7A3(_tmp7A2,_tag_fat(_tmp1C5,sizeof(void*),1U));});});}_LLF:;}
# 986
goto _LLA;}}}else{goto _LLD;}}else{_LLD: _LLE:
({struct Cyc_String_pa_PrintArg_struct _tmp1CC=({struct Cyc_String_pa_PrintArg_struct _tmp666;_tmp666.tag=0U,({
struct _fat_ptr _tmp7A4=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t0);}));_tmp666.f1=_tmp7A4;});_tmp666;});void*_tmp1C9[1U];_tmp1C9[0]=& _tmp1CC;({int(*_tmp7A6)(struct _fat_ptr,struct _fat_ptr)=({
# 987
int(*_tmp1CB)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp1CB;});struct _fat_ptr _tmp7A5=({const char*_tmp1CA=" tcexp:tcSpawn : Not a pointer type (internal error) %s";_tag_fat(_tmp1CA,sizeof(char),56U);});_tmp7A6(_tmp7A5,_tag_fat(_tmp1C9,sizeof(void*),1U));});});}_LLA:;}
# 990
goto _LL5;}}else{_LL8: _LL9:
# 993
({void*_tmp1CD=0U;({struct _fat_ptr _tmp7A7=({const char*_tmp1CE="\nTCSPAWN 1\n";_tag_fat(_tmp1CE,sizeof(char),12U);});Cyc_printf(_tmp7A7,_tag_fat(_tmp1CD,sizeof(void*),0U));});});
({void*_tmp1CF=0U;({int(*_tmp7A9)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp1D1)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp1D1;});struct _fat_ptr _tmp7A8=({const char*_tmp1D0="tcSpawn not FnCall";_tag_fat(_tmp1D0,sizeof(char),19U);});_tmp7A9(_tmp7A8,_tag_fat(_tmp1CF,sizeof(void*),0U));});});}_LL5:;}
# 1011 "tcexp.cyc"
return Cyc_Absyn_void_type;}}
# 1015
static void*Cyc_Tcexp_tcSeqExp(struct Cyc_Tcenv_Tenv*te,unsigned loc,void**topt,struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2){
({void*(*_tmp1DB)(struct Cyc_Tcenv_Tenv*,void**,struct Cyc_Absyn_Exp*)=Cyc_Tcexp_tcExp;struct Cyc_Tcenv_Tenv*_tmp1DC=({Cyc_Tcenv_clear_abstract_val_ok(te);});void**_tmp1DD=0;struct Cyc_Absyn_Exp*_tmp1DE=e1;_tmp1DB(_tmp1DC,_tmp1DD,_tmp1DE);});
({void*(*_tmp1DF)(struct Cyc_Tcenv_Tenv*,void**,struct Cyc_Absyn_Exp*)=Cyc_Tcexp_tcExp;struct Cyc_Tcenv_Tenv*_tmp1E0=({Cyc_Tcenv_clear_abstract_val_ok(te);});void**_tmp1E1=topt;struct Cyc_Absyn_Exp*_tmp1E2=e2;_tmp1DF(_tmp1E0,_tmp1E1,_tmp1E2);});
return(void*)_check_null(e2->topt);}
# 1022
static struct Cyc_Absyn_Datatypefield*Cyc_Tcexp_tcInjection(struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Exp*e,void*tu,struct Cyc_List_List*inst,struct Cyc_List_List*fs){
# 1025
struct Cyc_List_List*fields;
void*t1=(void*)_check_null(e->topt);
# 1028
{void*_stmttmp1A=({Cyc_Tcutil_compress(t1);});void*_tmp1E4=_stmttmp1A;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp1E4)->tag == 0U)switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp1E4)->f1)){case 2U: if(((struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp1E4)->f1)->f1 == 0){_LL1: _LL2:
({Cyc_Tcutil_unchecked_cast(e,Cyc_Absyn_double_type,Cyc_Absyn_No_coercion);});t1=Cyc_Absyn_double_type;goto _LL0;}else{goto _LL7;}case 1U: switch(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp1E4)->f1)->f2){case Cyc_Absyn_Char_sz: _LL3: _LL4:
 goto _LL6;case Cyc_Absyn_Short_sz: _LL5: _LL6:
({Cyc_Tcutil_unchecked_cast(e,Cyc_Absyn_sint_type,Cyc_Absyn_No_coercion);});t1=Cyc_Absyn_sint_type;goto _LL0;default: goto _LL7;}default: goto _LL7;}else{_LL7: _LL8:
 goto _LL0;}_LL0:;}
# 1035
for(fields=fs;fields != 0;fields=fields->tl){
struct Cyc_Absyn_Datatypefield _stmttmp1B=*((struct Cyc_Absyn_Datatypefield*)fields->hd);enum Cyc_Absyn_Scope _tmp1E8;unsigned _tmp1E7;struct Cyc_List_List*_tmp1E6;struct _tuple1*_tmp1E5;_LLA: _tmp1E5=_stmttmp1B.name;_tmp1E6=_stmttmp1B.typs;_tmp1E7=_stmttmp1B.loc;_tmp1E8=_stmttmp1B.sc;_LLB: {struct _tuple1*n=_tmp1E5;struct Cyc_List_List*typs=_tmp1E6;unsigned loc=_tmp1E7;enum Cyc_Absyn_Scope sc=_tmp1E8;
# 1038
if(typs == 0 || typs->tl != 0)continue;{void*t2=({Cyc_Tcutil_substitute(inst,(*((struct _tuple18*)typs->hd)).f2);});
# 1041
if(({Cyc_Unify_unify(t1,t2);}))
return(struct Cyc_Absyn_Datatypefield*)fields->hd;}}}
# 1045
for(fields=fs;fields != 0;fields=fields->tl){
struct Cyc_Absyn_Datatypefield _stmttmp1C=*((struct Cyc_Absyn_Datatypefield*)fields->hd);enum Cyc_Absyn_Scope _tmp1EC;unsigned _tmp1EB;struct Cyc_List_List*_tmp1EA;struct _tuple1*_tmp1E9;_LLD: _tmp1E9=_stmttmp1C.name;_tmp1EA=_stmttmp1C.typs;_tmp1EB=_stmttmp1C.loc;_tmp1EC=_stmttmp1C.sc;_LLE: {struct _tuple1*n=_tmp1E9;struct Cyc_List_List*typs=_tmp1EA;unsigned loc=_tmp1EB;enum Cyc_Absyn_Scope sc=_tmp1EC;
# 1048
if(typs == 0 || typs->tl != 0)continue;{void*t2=({Cyc_Tcutil_substitute(inst,(*((struct _tuple18*)typs->hd)).f2);});
# 1052
int bogus=0;
if(({Cyc_Tcutil_coerce_arg(te,e,t2,& bogus);}))
return(struct Cyc_Absyn_Datatypefield*)fields->hd;}}}
# 1057
({struct Cyc_String_pa_PrintArg_struct _tmp1EF=({struct Cyc_String_pa_PrintArg_struct _tmp668;_tmp668.tag=0U,({
struct _fat_ptr _tmp7AA=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(tu);}));_tmp668.f1=_tmp7AA;});_tmp668;});struct Cyc_String_pa_PrintArg_struct _tmp1F0=({struct Cyc_String_pa_PrintArg_struct _tmp667;_tmp667.tag=0U,({struct _fat_ptr _tmp7AB=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t1);}));_tmp667.f1=_tmp7AB;});_tmp667;});void*_tmp1ED[2U];_tmp1ED[0]=& _tmp1EF,_tmp1ED[1]=& _tmp1F0;({unsigned _tmp7AD=e->loc;struct _fat_ptr _tmp7AC=({const char*_tmp1EE="can't find a field in %s to inject a value of type %s";_tag_fat(_tmp1EE,sizeof(char),54U);});Cyc_Tcutil_terr(_tmp7AD,_tmp7AC,_tag_fat(_tmp1ED,sizeof(void*),2U));});});
return 0;}
# 1063
static void*Cyc_Tcexp_tcFnCall(struct Cyc_Tcenv_Tenv*te_orig,unsigned loc,void**topt,struct Cyc_Absyn_Exp*e,struct Cyc_List_List*args,struct Cyc_Absyn_VarargCallInfo**vararg_call_info,struct Cyc_List_List**alias_arg_exps){
# 1069
struct Cyc_List_List*es=args;
int arg_cnt=0;
struct Cyc_Tcenv_Tenv*te=({Cyc_Tcenv_new_block(loc,te_orig);});
struct Cyc_Tcenv_Tenv*_tmp1F2=({Cyc_Tcenv_clear_abstract_val_ok(te);});{struct Cyc_Tcenv_Tenv*te=_tmp1F2;
({Cyc_Tcexp_tcExp(te,0,e);});{
void*t=({Cyc_Tcutil_compress((void*)_check_null(e->topt));});
# 1080
void*_tmp1F3=t;void*_tmp1F9;void*_tmp1F8;void*_tmp1F7;void*_tmp1F6;struct Cyc_Absyn_Tqual _tmp1F5;void*_tmp1F4;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp1F3)->tag == 3U){_LL1: _tmp1F4=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp1F3)->f1).elt_type;_tmp1F5=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp1F3)->f1).elt_tq;_tmp1F6=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp1F3)->f1).ptr_atts).rgn;_tmp1F7=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp1F3)->f1).ptr_atts).nullable;_tmp1F8=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp1F3)->f1).ptr_atts).bounds;_tmp1F9=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp1F3)->f1).ptr_atts).zero_term;_LL2: {void*t1=_tmp1F4;struct Cyc_Absyn_Tqual tq=_tmp1F5;void*rgn=_tmp1F6;void*x=_tmp1F7;void*b=_tmp1F8;void*zt=_tmp1F9;
# 1085
({Cyc_Tcenv_check_rgn_accessible(te,loc,rgn);});
# 1087
({Cyc_Tcutil_check_nonzero_bound(loc,b);});{
void*ct1=({Cyc_Tcutil_compress(t1);});
void*_tmp1FA=ct1;int _tmp20B;struct Cyc_List_List*_tmp20A;struct Cyc_List_List*_tmp209;struct Cyc_List_List*_tmp208;struct Cyc_List_List*_tmp207;struct Cyc_Absyn_Exp*_tmp206;struct Cyc_List_List*_tmp205;struct Cyc_Absyn_Exp*_tmp204;struct Cyc_List_List*_tmp203;struct Cyc_List_List*_tmp202;struct Cyc_Absyn_VarargInfo*_tmp201;int _tmp200;struct Cyc_List_List*_tmp1FF;void*_tmp1FE;struct Cyc_Absyn_Tqual _tmp1FD;void*_tmp1FC;struct Cyc_List_List*_tmp1FB;if(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1FA)->tag == 5U){_LL6: _tmp1FB=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1FA)->f1).tvars;_tmp1FC=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1FA)->f1).effect;_tmp1FD=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1FA)->f1).ret_tqual;_tmp1FE=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1FA)->f1).ret_type;_tmp1FF=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1FA)->f1).args;_tmp200=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1FA)->f1).c_varargs;_tmp201=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1FA)->f1).cyc_varargs;_tmp202=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1FA)->f1).rgn_po;_tmp203=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1FA)->f1).attributes;_tmp204=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1FA)->f1).requires_clause;_tmp205=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1FA)->f1).requires_relns;_tmp206=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1FA)->f1).ensures_clause;_tmp207=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1FA)->f1).ensures_relns;_tmp208=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1FA)->f1).ieffect;_tmp209=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1FA)->f1).oeffect;_tmp20A=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1FA)->f1).throws;_tmp20B=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1FA)->f1).reentrant;_LL7: {struct Cyc_List_List*tvars=_tmp1FB;void*eff=_tmp1FC;struct Cyc_Absyn_Tqual res_tq=_tmp1FD;void*res_typ=_tmp1FE;struct Cyc_List_List*args_info=_tmp1FF;int c_vararg=_tmp200;struct Cyc_Absyn_VarargInfo*cyc_vararg=_tmp201;struct Cyc_List_List*rgn_po=_tmp202;struct Cyc_List_List*atts=_tmp203;struct Cyc_Absyn_Exp*req=_tmp204;struct Cyc_List_List*req_relns=_tmp205;struct Cyc_Absyn_Exp*ens=_tmp206;struct Cyc_List_List*ens_relns=_tmp207;struct Cyc_List_List*ieff=_tmp208;struct Cyc_List_List*oeff=_tmp209;struct Cyc_List_List*throws=_tmp20A;int reentrant=_tmp20B;
# 1094
if(({Cyc_Tcenv_is_reentrant_fun(te);})&& !({Cyc_Absyn_is_reentrant(reentrant);}))
# 1097
({struct Cyc_String_pa_PrintArg_struct _tmp20E=({struct Cyc_String_pa_PrintArg_struct _tmp669;_tmp669.tag=0U,({
struct _fat_ptr _tmp7AE=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t);}));_tmp669.f1=_tmp7AE;});_tmp669;});void*_tmp20C[1U];_tmp20C[0]=& _tmp20E;({unsigned _tmp7B0=e->loc;struct _fat_ptr _tmp7AF=({const char*_tmp20D="Invocation of non @re_entrant functions from a re-entrant function is disallowed. Offending function type: %s";_tag_fat(_tmp20D,sizeof(char),110U);});Cyc_Tcutil_terr(_tmp7B0,_tmp7AF,_tag_fat(_tmp20C,sizeof(void*),1U));});});
# 1094
if(
# 1100
tvars != 0 || rgn_po != 0)
({void*_tmp20F=0U;({unsigned _tmp7B2=e->loc;struct _fat_ptr _tmp7B1=({const char*_tmp210="function should have been instantiated prior to use -- probably a compiler bug";_tag_fat(_tmp210,sizeof(char),79U);});Cyc_Tcutil_terr(_tmp7B2,_tmp7B1,_tag_fat(_tmp20F,sizeof(void*),0U));});});
# 1094
if(topt != 0)
# 1105
({Cyc_Unify_unify(res_typ,*topt);});
# 1094
while(
# 1107
es != 0 && args_info != 0){
# 1109
int alias_coercion=0;
struct Cyc_Absyn_Exp*e1=(struct Cyc_Absyn_Exp*)es->hd;
void*t2=(*((struct _tuple10*)args_info->hd)).f3;
({Cyc_Tcexp_tcExp(te,& t2,e1);});
if(!({Cyc_Tcutil_coerce_arg(te,e1,t2,& alias_coercion);})){
struct _fat_ptr s0=({const char*_tmp220="actual argument has type ";_tag_fat(_tmp220,sizeof(char),26U);});
struct _fat_ptr s1=({Cyc_Absynpp_typ2string((void*)_check_null(e1->topt));});
struct _fat_ptr s2=({const char*_tmp21F=" but formal has type ";_tag_fat(_tmp21F,sizeof(char),22U);});
struct _fat_ptr s3=({Cyc_Absynpp_typ2string(t2);});
if(({unsigned long _tmp7B5=({unsigned long _tmp7B4=({unsigned long _tmp7B3=({Cyc_strlen((struct _fat_ptr)s0);});_tmp7B3 + ({Cyc_strlen((struct _fat_ptr)s1);});});_tmp7B4 + ({Cyc_strlen((struct _fat_ptr)s2);});});_tmp7B5 + ({Cyc_strlen((struct _fat_ptr)s3);});})>= (unsigned long)70)
({void*_tmp211=0U;({unsigned _tmp7B8=e1->loc;struct _fat_ptr _tmp7B7=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp214=({struct Cyc_String_pa_PrintArg_struct _tmp66D;_tmp66D.tag=0U,_tmp66D.f1=(struct _fat_ptr)((struct _fat_ptr)s0);_tmp66D;});struct Cyc_String_pa_PrintArg_struct _tmp215=({struct Cyc_String_pa_PrintArg_struct _tmp66C;_tmp66C.tag=0U,_tmp66C.f1=(struct _fat_ptr)((struct _fat_ptr)s1);_tmp66C;});struct Cyc_String_pa_PrintArg_struct _tmp216=({struct Cyc_String_pa_PrintArg_struct _tmp66B;_tmp66B.tag=0U,_tmp66B.f1=(struct _fat_ptr)((struct _fat_ptr)s2);_tmp66B;});struct Cyc_String_pa_PrintArg_struct _tmp217=({struct Cyc_String_pa_PrintArg_struct _tmp66A;_tmp66A.tag=0U,_tmp66A.f1=(struct _fat_ptr)((struct _fat_ptr)s3);_tmp66A;});void*_tmp212[4U];_tmp212[0]=& _tmp214,_tmp212[1]=& _tmp215,_tmp212[2]=& _tmp216,_tmp212[3]=& _tmp217;({struct _fat_ptr _tmp7B6=({const char*_tmp213="%s\n\t%s\n%s\n\t%s.";_tag_fat(_tmp213,sizeof(char),15U);});Cyc_aprintf(_tmp7B6,_tag_fat(_tmp212,sizeof(void*),4U));});});Cyc_Tcutil_terr(_tmp7B8,_tmp7B7,_tag_fat(_tmp211,sizeof(void*),0U));});});else{
# 1121
({void*_tmp218=0U;({unsigned _tmp7BB=e1->loc;struct _fat_ptr _tmp7BA=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp21B=({struct Cyc_String_pa_PrintArg_struct _tmp671;_tmp671.tag=0U,_tmp671.f1=(struct _fat_ptr)((struct _fat_ptr)s0);_tmp671;});struct Cyc_String_pa_PrintArg_struct _tmp21C=({struct Cyc_String_pa_PrintArg_struct _tmp670;_tmp670.tag=0U,_tmp670.f1=(struct _fat_ptr)((struct _fat_ptr)s1);_tmp670;});struct Cyc_String_pa_PrintArg_struct _tmp21D=({struct Cyc_String_pa_PrintArg_struct _tmp66F;_tmp66F.tag=0U,_tmp66F.f1=(struct _fat_ptr)((struct _fat_ptr)s2);_tmp66F;});struct Cyc_String_pa_PrintArg_struct _tmp21E=({struct Cyc_String_pa_PrintArg_struct _tmp66E;_tmp66E.tag=0U,_tmp66E.f1=(struct _fat_ptr)((struct _fat_ptr)s3);_tmp66E;});void*_tmp219[4U];_tmp219[0]=& _tmp21B,_tmp219[1]=& _tmp21C,_tmp219[2]=& _tmp21D,_tmp219[3]=& _tmp21E;({struct _fat_ptr _tmp7B9=({const char*_tmp21A="%s%s%s%s.";_tag_fat(_tmp21A,sizeof(char),10U);});Cyc_aprintf(_tmp7B9,_tag_fat(_tmp219,sizeof(void*),4U));});});Cyc_Tcutil_terr(_tmp7BB,_tmp7BA,_tag_fat(_tmp218,sizeof(void*),0U));});});}
({Cyc_Unify_unify((void*)_check_null(e1->topt),t2);});
({Cyc_Unify_explain_failure();});}
# 1113
if(alias_coercion)
# 1127
({struct Cyc_List_List*_tmp7BC=({struct Cyc_List_List*_tmp221=_cycalloc(sizeof(*_tmp221));_tmp221->hd=(void*)arg_cnt,_tmp221->tl=*alias_arg_exps;_tmp221;});*alias_arg_exps=_tmp7BC;});
# 1113
if(
# 1128
({Cyc_Tcutil_is_noalias_pointer_or_aggr(t2);})&& !({Cyc_Tcutil_is_noalias_path(e1);}))
({void*_tmp222=0U;({unsigned _tmp7BE=e1->loc;struct _fat_ptr _tmp7BD=({const char*_tmp223="Cannot consume non-unique paths; do swap instead";_tag_fat(_tmp223,sizeof(char),49U);});Cyc_Tcutil_terr(_tmp7BE,_tmp7BD,_tag_fat(_tmp222,sizeof(void*),0U));});});
# 1113
es=es->tl;
# 1131
args_info=args_info->tl;
++ arg_cnt;}{
# 1137
int args_already_checked=0;
{struct Cyc_List_List*a=atts;for(0;a != 0;a=a->tl){
void*_stmttmp1D=(void*)a->hd;void*_tmp224=_stmttmp1D;int _tmp227;int _tmp226;enum Cyc_Absyn_Format_Type _tmp225;if(((struct Cyc_Absyn_Format_att_Absyn_Attribute_struct*)_tmp224)->tag == 19U){_LLB: _tmp225=((struct Cyc_Absyn_Format_att_Absyn_Attribute_struct*)_tmp224)->f1;_tmp226=((struct Cyc_Absyn_Format_att_Absyn_Attribute_struct*)_tmp224)->f2;_tmp227=((struct Cyc_Absyn_Format_att_Absyn_Attribute_struct*)_tmp224)->f3;_LLC: {enum Cyc_Absyn_Format_Type ft=_tmp225;int fmt_arg_pos=_tmp226;int arg_start_pos=_tmp227;
# 1141
{struct _handler_cons _tmp228;_push_handler(& _tmp228);{int _tmp22A=0;if(setjmp(_tmp228.handler))_tmp22A=1;if(!_tmp22A){
# 1143
{struct Cyc_Absyn_Exp*fmt_arg=({({struct Cyc_Absyn_Exp*(*_tmp7C0)(struct Cyc_List_List*x,int n)=({struct Cyc_Absyn_Exp*(*_tmp22D)(struct Cyc_List_List*x,int n)=(struct Cyc_Absyn_Exp*(*)(struct Cyc_List_List*x,int n))Cyc_List_nth;_tmp22D;});struct Cyc_List_List*_tmp7BF=args;_tmp7C0(_tmp7BF,fmt_arg_pos - 1);});});
# 1145
struct Cyc_Core_Opt*fmt_args;
if(arg_start_pos == 0)
fmt_args=0;else{
# 1149
fmt_args=({struct Cyc_Core_Opt*_tmp22B=_cycalloc(sizeof(*_tmp22B));({struct Cyc_List_List*_tmp7C1=({Cyc_List_nth_tail(args,arg_start_pos - 1);});_tmp22B->v=_tmp7C1;});_tmp22B;});}
args_already_checked=1;{
enum Cyc_Absyn_Format_Type _tmp22C=ft;switch(_tmp22C){case Cyc_Absyn_Printf_ft: _LL10: _LL11:
# 1153
({Cyc_Tcexp_check_format_args(te,fmt_arg,fmt_args,arg_start_pos - 1,alias_arg_exps,c_vararg,Cyc_Formatstr_get_format_types);});
# 1156
goto _LLF;case Cyc_Absyn_Scanf_ft: _LL12: _LL13:
 goto _LL15;default: _LL14: _LL15:
# 1159
({Cyc_Tcexp_check_format_args(te,fmt_arg,fmt_args,arg_start_pos - 1,alias_arg_exps,c_vararg,Cyc_Formatstr_get_scanf_types);});
# 1162
goto _LLF;}_LLF:;}}
# 1143
;_pop_handler();}else{void*_tmp229=(void*)Cyc_Core_get_exn_thrown();void*_tmp22E=_tmp229;void*_tmp22F;if(((struct Cyc_List_Nth_exn_struct*)_tmp22E)->tag == Cyc_List_Nth){_LL17: _LL18:
# 1164
({void*_tmp230=0U;({unsigned _tmp7C3=loc;struct _fat_ptr _tmp7C2=({const char*_tmp231="bad format arguments";_tag_fat(_tmp231,sizeof(char),21U);});Cyc_Tcutil_terr(_tmp7C3,_tmp7C2,_tag_fat(_tmp230,sizeof(void*),0U));});});goto _LL16;}else{_LL19: _tmp22F=_tmp22E;_LL1A: {void*exn=_tmp22F;(int)_rethrow(exn);}}_LL16:;}}}
goto _LLA;}}else{_LLD: _LLE:
 goto _LLA;}_LLA:;}}
# 1169
if(args_info != 0)({void*_tmp232=0U;({unsigned _tmp7C5=loc;struct _fat_ptr _tmp7C4=({const char*_tmp233="too few arguments for function";_tag_fat(_tmp233,sizeof(char),31U);});Cyc_Tcutil_terr(_tmp7C5,_tmp7C4,_tag_fat(_tmp232,sizeof(void*),0U));});});else{
# 1171
if((es != 0 || c_vararg)|| cyc_vararg != 0){
if(c_vararg)
for(0;es != 0;es=es->tl){
({Cyc_Tcexp_tcExp(te,0,(struct Cyc_Absyn_Exp*)es->hd);});}else{
if(cyc_vararg == 0)
({void*_tmp234=0U;({unsigned _tmp7C7=loc;struct _fat_ptr _tmp7C6=({const char*_tmp235="too many arguments for function";_tag_fat(_tmp235,sizeof(char),32U);});Cyc_Tcutil_terr(_tmp7C7,_tmp7C6,_tag_fat(_tmp234,sizeof(void*),0U));});});else{
# 1178
struct Cyc_Absyn_VarargInfo _stmttmp1E=*cyc_vararg;int _tmp237;void*_tmp236;_LL1C: _tmp236=_stmttmp1E.type;_tmp237=_stmttmp1E.inject;_LL1D: {void*vt=_tmp236;int inject=_tmp237;
struct Cyc_Absyn_VarargCallInfo*vci=({struct Cyc_Absyn_VarargCallInfo*_tmp256=_cycalloc(sizeof(*_tmp256));_tmp256->num_varargs=0,_tmp256->injectors=0,_tmp256->vai=cyc_vararg;_tmp256;});
# 1182
*vararg_call_info=vci;
# 1184
if(!inject)
# 1186
for(0;es != 0;(es=es->tl,arg_cnt ++)){
int alias_coercion=0;
struct Cyc_Absyn_Exp*e1=(struct Cyc_Absyn_Exp*)es->hd;
++ vci->num_varargs;
({Cyc_Tcexp_tcExp(te,& vt,e1);});
if(!({Cyc_Tcutil_coerce_arg(te,e1,vt,& alias_coercion);})){
({struct Cyc_String_pa_PrintArg_struct _tmp23A=({struct Cyc_String_pa_PrintArg_struct _tmp673;_tmp673.tag=0U,({
struct _fat_ptr _tmp7C8=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(vt);}));_tmp673.f1=_tmp7C8;});_tmp673;});struct Cyc_String_pa_PrintArg_struct _tmp23B=({struct Cyc_String_pa_PrintArg_struct _tmp672;_tmp672.tag=0U,({struct _fat_ptr _tmp7C9=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string((void*)_check_null(e1->topt));}));_tmp672.f1=_tmp7C9;});_tmp672;});void*_tmp238[2U];_tmp238[0]=& _tmp23A,_tmp238[1]=& _tmp23B;({unsigned _tmp7CB=loc;struct _fat_ptr _tmp7CA=({const char*_tmp239="vararg requires type %s but argument has type %s";_tag_fat(_tmp239,sizeof(char),49U);});Cyc_Tcutil_terr(_tmp7CB,_tmp7CA,_tag_fat(_tmp238,sizeof(void*),2U));});});
({Cyc_Unify_explain_failure();});}
# 1191
if(alias_coercion)
# 1197
({struct Cyc_List_List*_tmp7CC=({struct Cyc_List_List*_tmp23C=_cycalloc(sizeof(*_tmp23C));_tmp23C->hd=(void*)arg_cnt,_tmp23C->tl=*alias_arg_exps;_tmp23C;});*alias_arg_exps=_tmp7CC;});
# 1191
if(
# 1198
({Cyc_Tcutil_is_noalias_pointer_or_aggr(vt);})&& !({Cyc_Tcutil_is_noalias_path(e1);}))
# 1200
({void*_tmp23D=0U;({unsigned _tmp7CE=e1->loc;struct _fat_ptr _tmp7CD=({const char*_tmp23E="Cannot consume non-unique paths; do swap instead";_tag_fat(_tmp23E,sizeof(char),49U);});Cyc_Tcutil_terr(_tmp7CE,_tmp7CD,_tag_fat(_tmp23D,sizeof(void*),0U));});});}else{
# 1205
void*_stmttmp1F=({void*(*_tmp254)(void*)=Cyc_Tcutil_compress;void*_tmp255=({Cyc_Tcutil_pointer_elt_type(vt);});_tmp254(_tmp255);});void*_tmp23F=_stmttmp1F;struct Cyc_List_List*_tmp241;struct Cyc_Absyn_Datatypedecl*_tmp240;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp23F)->tag == 0U){if(((struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp23F)->f1)->tag == 20U){if(((((struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp23F)->f1)->f1).KnownDatatype).tag == 2){_LL1F: _tmp240=*((((struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp23F)->f1)->f1).KnownDatatype).val;_tmp241=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp23F)->f2;_LL20: {struct Cyc_Absyn_Datatypedecl*td=_tmp240;struct Cyc_List_List*targs=_tmp241;
# 1209
struct Cyc_Absyn_Datatypedecl*_tmp242=*({Cyc_Tcenv_lookup_datatypedecl(te,loc,td->name);});{struct Cyc_Absyn_Datatypedecl*td=_tmp242;
struct Cyc_List_List*fields=0;
if(td->fields == 0)
({struct Cyc_String_pa_PrintArg_struct _tmp245=({struct Cyc_String_pa_PrintArg_struct _tmp674;_tmp674.tag=0U,({
struct _fat_ptr _tmp7CF=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(vt);}));_tmp674.f1=_tmp7CF;});_tmp674;});void*_tmp243[1U];_tmp243[0]=& _tmp245;({unsigned _tmp7D1=loc;struct _fat_ptr _tmp7D0=({const char*_tmp244="can't inject into abstract datatype %s";_tag_fat(_tmp244,sizeof(char),39U);});Cyc_Tcutil_terr(_tmp7D1,_tmp7D0,_tag_fat(_tmp243,sizeof(void*),1U));});});else{
fields=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(td->fields))->v;}
# 1220
({int(*_tmp246)(void*,void*)=Cyc_Unify_unify;void*_tmp247=({Cyc_Tcutil_pointer_region(vt);});void*_tmp248=({Cyc_Tcenv_curr_rgn(te);});_tmp246(_tmp247,_tmp248);});{
# 1222
struct Cyc_List_List*inst=({Cyc_List_zip(td->tvs,targs);});
for(0;es != 0;es=es->tl){
++ vci->num_varargs;{
struct Cyc_Absyn_Exp*e1=(struct Cyc_Absyn_Exp*)es->hd;
# 1227
if(!args_already_checked){
({Cyc_Tcexp_tcExp(te,0,e1);});
if(({Cyc_Tcutil_is_noalias_pointer_or_aggr((void*)_check_null(e1->topt));})&& !({Cyc_Tcutil_is_noalias_path(e1);}))
# 1231
({void*_tmp249=0U;({unsigned _tmp7D3=e1->loc;struct _fat_ptr _tmp7D2=({const char*_tmp24A="Cannot consume non-unique paths; do swap instead";_tag_fat(_tmp24A,sizeof(char),49U);});Cyc_Tcutil_terr(_tmp7D3,_tmp7D2,_tag_fat(_tmp249,sizeof(void*),0U));});});}{
# 1227
struct Cyc_Absyn_Datatypefield*f=({struct Cyc_Absyn_Datatypefield*(*_tmp24C)(struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Exp*e,void*tu,struct Cyc_List_List*inst,struct Cyc_List_List*fs)=Cyc_Tcexp_tcInjection;struct Cyc_Tcenv_Tenv*_tmp24D=te;struct Cyc_Absyn_Exp*_tmp24E=e1;void*_tmp24F=({Cyc_Tcutil_pointer_elt_type(vt);});struct Cyc_List_List*_tmp250=inst;struct Cyc_List_List*_tmp251=fields;_tmp24C(_tmp24D,_tmp24E,_tmp24F,_tmp250,_tmp251);});
# 1234
if(f != 0)
({struct Cyc_List_List*_tmp7D5=({({struct Cyc_List_List*_tmp7D4=vci->injectors;Cyc_List_append(_tmp7D4,({struct Cyc_List_List*_tmp24B=_cycalloc(sizeof(*_tmp24B));_tmp24B->hd=f,_tmp24B->tl=0;_tmp24B;}));});});vci->injectors=_tmp7D5;});}}}
# 1239
goto _LL1E;}}}}else{goto _LL21;}}else{goto _LL21;}}else{_LL21: _LL22:
({void*_tmp252=0U;({unsigned _tmp7D7=loc;struct _fat_ptr _tmp7D6=({const char*_tmp253="bad inject vararg type";_tag_fat(_tmp253,sizeof(char),23U);});Cyc_Tcutil_terr(_tmp7D7,_tmp7D6,_tag_fat(_tmp252,sizeof(void*),0U));});});goto _LL1E;}_LL1E:;}}}}}}
# 1169
if(*alias_arg_exps == 0)
# 1259 "tcexp.cyc"
({Cyc_Tcenv_check_effect_accessible(te,loc,(void*)_check_null(eff));});
# 1169 "tcexp.cyc"
return res_typ;}}}else{_LL8: _LL9:
# 1262 "tcexp.cyc"
 return({struct Cyc_String_pa_PrintArg_struct _tmp259=({struct Cyc_String_pa_PrintArg_struct _tmp675;_tmp675.tag=0U,({struct _fat_ptr _tmp7D8=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t);}));_tmp675.f1=_tmp7D8;});_tmp675;});void*_tmp257[1U];_tmp257[0]=& _tmp259;({struct Cyc_Tcenv_Tenv*_tmp7DC=te;unsigned _tmp7DB=loc;void**_tmp7DA=topt;struct _fat_ptr _tmp7D9=({const char*_tmp258="expected pointer to function but found %s";_tag_fat(_tmp258,sizeof(char),42U);});Cyc_Tcexp_expr_err(_tmp7DC,_tmp7DB,_tmp7DA,_tmp7D9,_tag_fat(_tmp257,sizeof(void*),1U));});});}_LL5:;}}}else{_LL3: _LL4:
# 1264
 return({struct Cyc_String_pa_PrintArg_struct _tmp25C=({struct Cyc_String_pa_PrintArg_struct _tmp676;_tmp676.tag=0U,({struct _fat_ptr _tmp7DD=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t);}));_tmp676.f1=_tmp7DD;});_tmp676;});void*_tmp25A[1U];_tmp25A[0]=& _tmp25C;({struct Cyc_Tcenv_Tenv*_tmp7E1=te;unsigned _tmp7E0=loc;void**_tmp7DF=topt;struct _fat_ptr _tmp7DE=({const char*_tmp25B="expected pointer to function but found %s";_tag_fat(_tmp25B,sizeof(char),42U);});Cyc_Tcexp_expr_err(_tmp7E1,_tmp7E0,_tmp7DF,_tmp7DE,_tag_fat(_tmp25A,sizeof(void*),1U));});});}_LL0:;}}}
# 1270
static void*Cyc_Tcexp_tcThrow(struct Cyc_Tcenv_Tenv*te,unsigned loc,void**topt,struct Cyc_Absyn_Exp*e){
int bogus=0;
void*exception_type=({Cyc_Absyn_exn_type();});
void*res_typ=({Cyc_Tcexp_tcExp(te,0,e);});
# 1277
if(!({Cyc_Unify_unify(exception_type,res_typ);})&&({Cyc_Tcutil_silent_castable(te,loc,res_typ,exception_type);})){
# 1280
exception_type=res_typ;
({Cyc_Tcutil_unchecked_cast(e,exception_type,Cyc_Absyn_Other_coercion);});}
# 1277
if(!({Cyc_Tcutil_coerce_arg(te,e,exception_type,& bogus);}))
# 1285
({struct Cyc_String_pa_PrintArg_struct _tmp260=({struct Cyc_String_pa_PrintArg_struct _tmp678;_tmp678.tag=0U,({struct _fat_ptr _tmp7E2=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(exception_type);}));_tmp678.f1=_tmp7E2;});_tmp678;});struct Cyc_String_pa_PrintArg_struct _tmp261=({struct Cyc_String_pa_PrintArg_struct _tmp677;_tmp677.tag=0U,({
struct _fat_ptr _tmp7E3=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string((void*)_check_null(e->topt));}));_tmp677.f1=_tmp7E3;});_tmp677;});void*_tmp25E[2U];_tmp25E[0]=& _tmp260,_tmp25E[1]=& _tmp261;({unsigned _tmp7E5=loc;struct _fat_ptr _tmp7E4=({const char*_tmp25F="expected %s but found %s";_tag_fat(_tmp25F,sizeof(char),25U);});Cyc_Tcutil_terr(_tmp7E5,_tmp7E4,_tag_fat(_tmp25E,sizeof(void*),2U));});});
# 1277
if(topt != 0)
# 1288
return*topt;
# 1277
return({void*(*_tmp262)(struct Cyc_Core_Opt*)=Cyc_Absyn_wildtyp;struct Cyc_Core_Opt*_tmp263=({Cyc_Tcenv_lookup_opt_type_vars(te);});_tmp262(_tmp263);});}
# 1293
static void*Cyc_Tcexp_tcInstantiate(struct Cyc_Tcenv_Tenv*te,unsigned loc,void**topt,struct Cyc_Absyn_Exp*e,struct Cyc_List_List*ts){
# 1295
({Cyc_Tcexp_tcExpNoInst(te,0,e);});
# 1297
({void*_tmp7E6=({Cyc_Absyn_pointer_expand((void*)_check_null(e->topt),0);});e->topt=_tmp7E6;});{
void*t1=({Cyc_Tcutil_compress((void*)_check_null(e->topt));});
{void*_tmp265=t1;void*_tmp26B;void*_tmp26A;void*_tmp269;void*_tmp268;struct Cyc_Absyn_Tqual _tmp267;void*_tmp266;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp265)->tag == 3U){_LL1: _tmp266=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp265)->f1).elt_type;_tmp267=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp265)->f1).elt_tq;_tmp268=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp265)->f1).ptr_atts).rgn;_tmp269=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp265)->f1).ptr_atts).nullable;_tmp26A=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp265)->f1).ptr_atts).bounds;_tmp26B=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp265)->f1).ptr_atts).zero_term;_LL2: {void*t0=_tmp266;struct Cyc_Absyn_Tqual tq=_tmp267;void*rt=_tmp268;void*x=_tmp269;void*b=_tmp26A;void*zt=_tmp26B;
# 1301
{void*_stmttmp20=({Cyc_Tcutil_compress(t0);});void*_tmp26C=_stmttmp20;int _tmp27D;struct Cyc_List_List*_tmp27C;struct Cyc_List_List*_tmp27B;struct Cyc_List_List*_tmp27A;struct Cyc_List_List*_tmp279;struct Cyc_Absyn_Exp*_tmp278;struct Cyc_List_List*_tmp277;struct Cyc_Absyn_Exp*_tmp276;struct Cyc_List_List*_tmp275;struct Cyc_List_List*_tmp274;struct Cyc_Absyn_VarargInfo*_tmp273;int _tmp272;struct Cyc_List_List*_tmp271;void*_tmp270;struct Cyc_Absyn_Tqual _tmp26F;void*_tmp26E;struct Cyc_List_List*_tmp26D;if(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp26C)->tag == 5U){_LL6: _tmp26D=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp26C)->f1).tvars;_tmp26E=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp26C)->f1).effect;_tmp26F=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp26C)->f1).ret_tqual;_tmp270=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp26C)->f1).ret_type;_tmp271=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp26C)->f1).args;_tmp272=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp26C)->f1).c_varargs;_tmp273=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp26C)->f1).cyc_varargs;_tmp274=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp26C)->f1).rgn_po;_tmp275=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp26C)->f1).attributes;_tmp276=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp26C)->f1).requires_clause;_tmp277=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp26C)->f1).requires_relns;_tmp278=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp26C)->f1).ensures_clause;_tmp279=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp26C)->f1).ensures_relns;_tmp27A=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp26C)->f1).ieffect;_tmp27B=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp26C)->f1).oeffect;_tmp27C=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp26C)->f1).throws;_tmp27D=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp26C)->f1).reentrant;_LL7: {struct Cyc_List_List*tvars=_tmp26D;void*eff=_tmp26E;struct Cyc_Absyn_Tqual rtq=_tmp26F;void*rtyp=_tmp270;struct Cyc_List_List*args=_tmp271;int c_varargs=_tmp272;struct Cyc_Absyn_VarargInfo*cyc_varargs=_tmp273;struct Cyc_List_List*rpo=_tmp274;struct Cyc_List_List*atts=_tmp275;struct Cyc_Absyn_Exp*req=_tmp276;struct Cyc_List_List*req_relns=_tmp277;struct Cyc_Absyn_Exp*ens=_tmp278;struct Cyc_List_List*ens_relns=_tmp279;struct Cyc_List_List*ieff=_tmp27A;struct Cyc_List_List*oeff=_tmp27B;struct Cyc_List_List*throws=_tmp27C;int reentrant=_tmp27D;
# 1306
struct Cyc_List_List*instantiation=0;
# 1308
for(0;ts != 0 && tvars != 0;(ts=ts->tl,tvars=tvars->tl)){
# 1310
struct Cyc_Absyn_Kind*k=({Cyc_Tcutil_tvar_kind((struct Cyc_Absyn_Tvar*)tvars->hd,& Cyc_Tcutil_bk);});
({void(*_tmp27E)(unsigned,struct Cyc_Tcenv_Tenv*,struct Cyc_List_List*bound_tvars,struct Cyc_Absyn_Kind*k,int allow_evars,int allow_abs_aggr,void*)=Cyc_Tctyp_check_type;unsigned _tmp27F=loc;struct Cyc_Tcenv_Tenv*_tmp280=te;struct Cyc_List_List*_tmp281=({Cyc_Tcenv_lookup_type_vars(te);});struct Cyc_Absyn_Kind*_tmp282=k;int _tmp283=1;int _tmp284=1;void*_tmp285=(void*)ts->hd;_tmp27E(_tmp27F,_tmp280,_tmp281,_tmp282,_tmp283,_tmp284,_tmp285);});
({Cyc_Tcutil_check_no_qual(loc,(void*)ts->hd);});
instantiation=({struct Cyc_List_List*_tmp287=_cycalloc(sizeof(*_tmp287));({struct _tuple15*_tmp7E7=({struct _tuple15*_tmp286=_cycalloc(sizeof(*_tmp286));_tmp286->f1=(struct Cyc_Absyn_Tvar*)tvars->hd,_tmp286->f2=(void*)ts->hd;_tmp286;});_tmp287->hd=_tmp7E7;}),_tmp287->tl=instantiation;_tmp287;});}
# 1315
if(ts != 0)
return({void*_tmp288=0U;({struct Cyc_Tcenv_Tenv*_tmp7EB=te;unsigned _tmp7EA=loc;void**_tmp7E9=topt;struct _fat_ptr _tmp7E8=({const char*_tmp289="too many type variables in instantiation";_tag_fat(_tmp289,sizeof(char),41U);});Cyc_Tcexp_expr_err(_tmp7EB,_tmp7EA,_tmp7E9,_tmp7E8,_tag_fat(_tmp288,sizeof(void*),0U));});});
# 1315
if(tvars == 0){
# 1323
rpo=({Cyc_Tcutil_rsubst_rgnpo(Cyc_Core_heap_region,instantiation,rpo);});
({Cyc_Tcenv_check_rgn_partial_order(te,loc,rpo);});
rpo=0;}{
# 1315
void*new_fn_typ=({({struct Cyc_List_List*_tmp7EC=instantiation;Cyc_Tcutil_substitute(_tmp7EC,(void*)({struct Cyc_Absyn_FnType_Absyn_Type_struct*_tmp28B=_cycalloc(sizeof(*_tmp28B));_tmp28B->tag=5U,(_tmp28B->f1).tvars=tvars,(_tmp28B->f1).effect=eff,(_tmp28B->f1).ret_tqual=rtq,(_tmp28B->f1).ret_type=rtyp,(_tmp28B->f1).args=args,(_tmp28B->f1).c_varargs=c_varargs,(_tmp28B->f1).cyc_varargs=cyc_varargs,(_tmp28B->f1).rgn_po=rpo,(_tmp28B->f1).attributes=atts,(_tmp28B->f1).requires_clause=req,(_tmp28B->f1).requires_relns=req_relns,(_tmp28B->f1).ensures_clause=ens,(_tmp28B->f1).ensures_relns=ens_relns,(_tmp28B->f1).ieffect=ieff,(_tmp28B->f1).oeffect=oeff,(_tmp28B->f1).throws=throws,(_tmp28B->f1).reentrant=reentrant;_tmp28B;}));});});
# 1332
return(void*)({struct Cyc_Absyn_PointerType_Absyn_Type_struct*_tmp28A=_cycalloc(sizeof(*_tmp28A));_tmp28A->tag=3U,(_tmp28A->f1).elt_type=new_fn_typ,(_tmp28A->f1).elt_tq=tq,((_tmp28A->f1).ptr_atts).rgn=rt,((_tmp28A->f1).ptr_atts).nullable=x,((_tmp28A->f1).ptr_atts).bounds=b,((_tmp28A->f1).ptr_atts).zero_term=zt,((_tmp28A->f1).ptr_atts).ptrloc=0;_tmp28A;});}}}else{_LL8: _LL9:
 goto _LL5;}_LL5:;}
# 1335
goto _LL0;}}else{_LL3: _LL4:
 goto _LL0;}_LL0:;}
# 1338
return({struct Cyc_String_pa_PrintArg_struct _tmp28E=({struct Cyc_String_pa_PrintArg_struct _tmp679;_tmp679.tag=0U,({
struct _fat_ptr _tmp7ED=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t1);}));_tmp679.f1=_tmp7ED;});_tmp679;});void*_tmp28C[1U];_tmp28C[0]=& _tmp28E;({struct Cyc_Tcenv_Tenv*_tmp7F1=te;unsigned _tmp7F0=loc;void**_tmp7EF=topt;struct _fat_ptr _tmp7EE=({const char*_tmp28D="expecting polymorphic type but found %s";_tag_fat(_tmp28D,sizeof(char),40U);});Cyc_Tcexp_expr_err(_tmp7F1,_tmp7F0,_tmp7EF,_tmp7EE,_tag_fat(_tmp28C,sizeof(void*),1U));});});}}
# 1343
static void*Cyc_Tcexp_tcCast(struct Cyc_Tcenv_Tenv*te,unsigned loc,void**topt,void*t,struct Cyc_Absyn_Exp*e,enum Cyc_Absyn_Coercion*c){
# 1345
({void(*_tmp290)(unsigned,struct Cyc_Tcenv_Tenv*,struct Cyc_List_List*bound_tvars,struct Cyc_Absyn_Kind*k,int allow_evars,int allow_abs_aggr,void*)=Cyc_Tctyp_check_type;unsigned _tmp291=loc;struct Cyc_Tcenv_Tenv*_tmp292=te;struct Cyc_List_List*_tmp293=({Cyc_Tcenv_lookup_type_vars(te);});struct Cyc_Absyn_Kind*_tmp294=
({Cyc_Tcenv_abstract_val_ok(te);})?& Cyc_Tcutil_tak:& Cyc_Tcutil_tmk;int _tmp295=1;int _tmp296=0;void*_tmp297=t;_tmp290(_tmp291,_tmp292,_tmp293,_tmp294,_tmp295,_tmp296,_tmp297);});
({Cyc_Tcutil_check_no_qual(loc,t);});
# 1349
({Cyc_Tcexp_tcExp(te,& t,e);});{
void*t2=(void*)_check_null(e->topt);
if(({Cyc_Tcutil_silent_castable(te,loc,t2,t);}))
*((enum Cyc_Absyn_Coercion*)_check_null(c))=Cyc_Absyn_No_coercion;else{
# 1354
enum Cyc_Absyn_Coercion crc=({Cyc_Tcutil_castable(te,loc,t2,t);});
if((int)crc != (int)0U)
*((enum Cyc_Absyn_Coercion*)_check_null(c))=crc;else{
if(({Cyc_Tcutil_zero_to_null(t,e);}))
*((enum Cyc_Absyn_Coercion*)_check_null(c))=Cyc_Absyn_No_coercion;else{
# 1361
({Cyc_Unify_unify(t2,t);});{
void*result=({struct Cyc_String_pa_PrintArg_struct _tmp29A=({struct Cyc_String_pa_PrintArg_struct _tmp67B;_tmp67B.tag=0U,({
struct _fat_ptr _tmp7F2=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t2);}));_tmp67B.f1=_tmp7F2;});_tmp67B;});struct Cyc_String_pa_PrintArg_struct _tmp29B=({struct Cyc_String_pa_PrintArg_struct _tmp67A;_tmp67A.tag=0U,({struct _fat_ptr _tmp7F3=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t);}));_tmp67A.f1=_tmp7F3;});_tmp67A;});void*_tmp298[2U];_tmp298[0]=& _tmp29A,_tmp298[1]=& _tmp29B;({struct Cyc_Tcenv_Tenv*_tmp7F6=te;unsigned _tmp7F5=loc;struct _fat_ptr _tmp7F4=({const char*_tmp299="cannot cast %s to %s";_tag_fat(_tmp299,sizeof(char),21U);});Cyc_Tcexp_expr_err(_tmp7F6,_tmp7F5,& t,_tmp7F4,_tag_fat(_tmp298,sizeof(void*),2U));});});
({Cyc_Unify_explain_failure();});
return result;}}}}
# 1371
{struct _tuple0 _stmttmp21=({struct _tuple0 _tmp67C;_tmp67C.f1=e->r,({void*_tmp7F7=({Cyc_Tcutil_compress(t);});_tmp67C.f2=_tmp7F7;});_tmp67C;});struct _tuple0 _tmp29C=_stmttmp21;void*_tmp2A0;void*_tmp29F;void*_tmp29E;int _tmp29D;if(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp29C.f1)->tag == 35U){if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp29C.f2)->tag == 3U){_LL1: _tmp29D=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp29C.f1)->f1).fat_result;_tmp29E=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp29C.f2)->f1).ptr_atts).nullable;_tmp29F=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp29C.f2)->f1).ptr_atts).bounds;_tmp2A0=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp29C.f2)->f1).ptr_atts).zero_term;_LL2: {int fat_result=_tmp29D;void*nbl=_tmp29E;void*bds=_tmp29F;void*zt=_tmp2A0;
# 1375
if((fat_result && !({Cyc_Tcutil_force_type2bool(0,zt);}))&&({Cyc_Tcutil_force_type2bool(0,nbl);})){
struct Cyc_Absyn_Exp*eopt=({struct Cyc_Absyn_Exp*(*_tmp2A3)(void*def,void*b)=Cyc_Tcutil_get_bounds_exp;void*_tmp2A4=({Cyc_Absyn_bounds_one();});void*_tmp2A5=bds;_tmp2A3(_tmp2A4,_tmp2A5);});
if(eopt != 0){
struct Cyc_Absyn_Exp*e2=e;
if(({Cyc_Evexp_eval_const_uint_exp(e2);}).f1 == (unsigned)1)
({void*_tmp2A1=0U;({unsigned _tmp7F9=loc;struct _fat_ptr _tmp7F8=({const char*_tmp2A2="cast from ? pointer to * pointer will lose size information";_tag_fat(_tmp2A2,sizeof(char),60U);});Cyc_Tcutil_warn(_tmp7F9,_tmp7F8,_tag_fat(_tmp2A1,sizeof(void*),0U));});});}}
# 1375
goto _LL0;}}else{goto _LL3;}}else{_LL3: _LL4:
# 1384
 goto _LL0;}_LL0:;}
# 1386
return t;}}
# 1390
static void*Cyc_Tcexp_tcAddress(struct Cyc_Tcenv_Tenv*te,unsigned loc,struct Cyc_Absyn_Exp*e0,void**topt,struct Cyc_Absyn_Exp*e){
void**topt2=0;
struct Cyc_Absyn_Tqual tq2=({Cyc_Absyn_empty_tqual(0U);});
int nullable=0;
if(topt != 0){
void*_stmttmp22=({Cyc_Tcutil_compress(*topt);});void*_tmp2A7=_stmttmp22;void*_tmp2AA;struct Cyc_Absyn_Tqual _tmp2A9;void**_tmp2A8;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp2A7)->tag == 3U){_LL1: _tmp2A8=(void**)&(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp2A7)->f1).elt_type;_tmp2A9=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp2A7)->f1).elt_tq;_tmp2AA=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp2A7)->f1).ptr_atts).nullable;_LL2: {void**elttype=_tmp2A8;struct Cyc_Absyn_Tqual tq=_tmp2A9;void*n=_tmp2AA;
# 1397
topt2=elttype;
tq2=tq;
nullable=({Cyc_Tcutil_force_type2bool(0,n);});
goto _LL0;}}else{_LL3: _LL4:
 goto _LL0;}_LL0:;}
# 1394
({void(*_tmp2AB)(struct Cyc_Tcenv_Tenv*te,void**topt,struct Cyc_Absyn_Exp*e)=Cyc_Tcexp_tcExpNoInst;struct Cyc_Tcenv_Tenv*_tmp2AC=({struct Cyc_Tcenv_Tenv*(*_tmp2AD)(struct Cyc_Tcenv_Tenv*)=Cyc_Tcenv_enter_abstract_val_ok;struct Cyc_Tcenv_Tenv*_tmp2AE=({struct Cyc_Tcenv_Tenv*(*_tmp2AF)(struct Cyc_Tcenv_Tenv*)=Cyc_Tcenv_enter_lhs;struct Cyc_Tcenv_Tenv*_tmp2B0=({Cyc_Tcenv_clear_notreadctxt(te);});_tmp2AF(_tmp2B0);});_tmp2AD(_tmp2AE);});void**_tmp2B1=topt2;struct Cyc_Absyn_Exp*_tmp2B2=e;_tmp2AB(_tmp2AC,_tmp2B1,_tmp2B2);});
# 1416 "tcexp.cyc"
if(({Cyc_Tcutil_is_noalias_pointer_or_aggr((void*)_check_null(e->topt));}))
({void*_tmp2B3=0U;({unsigned _tmp7FB=e->loc;struct _fat_ptr _tmp7FA=({const char*_tmp2B4="Cannot take the address of an alias-free path";_tag_fat(_tmp2B4,sizeof(char),46U);});Cyc_Tcutil_terr(_tmp7FB,_tmp7FA,_tag_fat(_tmp2B3,sizeof(void*),0U));});});
# 1416
{void*_stmttmp23=e->r;void*_tmp2B5=_stmttmp23;struct Cyc_Absyn_Exp*_tmp2B7;struct Cyc_Absyn_Exp*_tmp2B6;if(((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp2B5)->tag == 23U){_LL6: _tmp2B6=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp2B5)->f1;_tmp2B7=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp2B5)->f2;_LL7: {struct Cyc_Absyn_Exp*e1=_tmp2B6;struct Cyc_Absyn_Exp*e2=_tmp2B7;
# 1424
{void*_stmttmp24=({Cyc_Tcutil_compress((void*)_check_null(e1->topt));});void*_tmp2B8=_stmttmp24;if(((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp2B8)->tag == 6U){_LLB: _LLC:
 goto _LLA;}else{_LLD: _LLE:
# 1429
({void*_tmp7FC=({Cyc_Absyn_add_exp(e1,e2,0U);})->r;e0->r=_tmp7FC;});
return({Cyc_Tcexp_tcPlus(te,e1,e2);});}_LLA:;}
# 1432
goto _LL5;}}else{_LL8: _LL9:
 goto _LL5;}_LL5:;}
# 1437
{void*_stmttmp25=e->r;void*_tmp2B9=_stmttmp25;switch(*((int*)_tmp2B9)){case 21U: if(((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp2B9)->f3 == 1){_LL10: _LL11:
 goto _LL13;}else{goto _LL14;}case 22U: if(((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp2B9)->f3 == 1){_LL12: _LL13:
# 1440
({void*_tmp2BA=0U;({unsigned _tmp7FE=e->loc;struct _fat_ptr _tmp7FD=({const char*_tmp2BB="cannot take the address of a @tagged union member";_tag_fat(_tmp2BB,sizeof(char),50U);});Cyc_Tcutil_terr(_tmp7FE,_tmp7FD,_tag_fat(_tmp2BA,sizeof(void*),0U));});});goto _LLF;}else{goto _LL14;}default: _LL14: _LL15:
 goto _LLF;}_LLF:;}{
# 1445
struct _tuple16 _stmttmp26=({Cyc_Tcutil_addressof_props(e);});void*_tmp2BD;int _tmp2BC;_LL17: _tmp2BC=_stmttmp26.f1;_tmp2BD=_stmttmp26.f2;_LL18: {int is_const=_tmp2BC;void*rgn=_tmp2BD;
# 1447
if(({Cyc_Tcutil_is_noalias_region(rgn,0);}))
({void*_tmp2BE=0U;({unsigned _tmp800=e->loc;struct _fat_ptr _tmp7FF=({const char*_tmp2BF="using & would manufacture an alias to an alias-free pointer";_tag_fat(_tmp2BF,sizeof(char),60U);});Cyc_Tcutil_terr(_tmp800,_tmp7FF,_tag_fat(_tmp2BE,sizeof(void*),0U));});});{
# 1447
struct Cyc_Absyn_Tqual tq=({Cyc_Absyn_empty_tqual(0U);});
# 1451
if(is_const){
tq.print_const=1;
tq.real_const=1;}{
# 1451
void*t=
# 1456
nullable?({Cyc_Absyn_star_type((void*)_check_null(e->topt),rgn,tq,Cyc_Absyn_false_type);}):({Cyc_Absyn_at_type((void*)_check_null(e->topt),rgn,tq,Cyc_Absyn_false_type);});
# 1460
return t;}}}}}
# 1464
static void*Cyc_Tcexp_tcSizeof(struct Cyc_Tcenv_Tenv*te,unsigned loc,void**topt,void*t){
if(te->allow_valueof)
# 1468
return Cyc_Absyn_uint_type;
# 1465
({void(*_tmp2C1)(unsigned,struct Cyc_Tcenv_Tenv*,struct Cyc_List_List*bound_tvars,struct Cyc_Absyn_Kind*k,int allow_evars,int allow_abs_aggr,void*)=Cyc_Tctyp_check_type;unsigned _tmp2C2=loc;struct Cyc_Tcenv_Tenv*_tmp2C3=te;struct Cyc_List_List*_tmp2C4=({Cyc_Tcenv_lookup_type_vars(te);});struct Cyc_Absyn_Kind*_tmp2C5=& Cyc_Tcutil_tmk;int _tmp2C6=1;int _tmp2C7=0;void*_tmp2C8=t;_tmp2C1(_tmp2C2,_tmp2C3,_tmp2C4,_tmp2C5,_tmp2C6,_tmp2C7,_tmp2C8);});
# 1471
({Cyc_Tcutil_check_no_qual(loc,t);});
if(!({Cyc_Evexp_okay_szofarg(t);}))
({struct Cyc_String_pa_PrintArg_struct _tmp2CB=({struct Cyc_String_pa_PrintArg_struct _tmp67D;_tmp67D.tag=0U,({
struct _fat_ptr _tmp801=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t);}));_tmp67D.f1=_tmp801;});_tmp67D;});void*_tmp2C9[1U];_tmp2C9[0]=& _tmp2CB;({unsigned _tmp803=loc;struct _fat_ptr _tmp802=({const char*_tmp2CA="sizeof applied to type %s, which has unknown size here";_tag_fat(_tmp2CA,sizeof(char),55U);});Cyc_Tcutil_terr(_tmp803,_tmp802,_tag_fat(_tmp2C9,sizeof(void*),1U));});});
# 1472
if(topt != 0){
# 1476
void*_stmttmp27=({Cyc_Tcutil_compress(*topt);});void*_tmp2CC=_stmttmp27;void*_tmp2CE;void*_tmp2CD;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2CC)->tag == 0U){if(((struct Cyc_Absyn_TagCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2CC)->f1)->tag == 5U){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2CC)->f2 != 0){_LL1: _tmp2CD=_tmp2CC;_tmp2CE=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2CC)->f2)->hd;_LL2: {void*tagtyp=_tmp2CD;void*tt=_tmp2CE;
# 1479
struct Cyc_Absyn_Exp*e=({Cyc_Absyn_sizeoftype_exp(t,0U);});
struct Cyc_Absyn_ValueofType_Absyn_Type_struct*tt2=({struct Cyc_Absyn_ValueofType_Absyn_Type_struct*_tmp2CF=_cycalloc(sizeof(*_tmp2CF));_tmp2CF->tag=9U,_tmp2CF->f1=e;_tmp2CF;});
if(({Cyc_Unify_unify(tt,(void*)tt2);}))return tagtyp;goto _LL0;}}else{goto _LL3;}}else{goto _LL3;}}else{_LL3: _LL4:
# 1483
 goto _LL0;}_LL0:;}
# 1472
return Cyc_Absyn_uint_type;}
# 1464
void*Cyc_Tcexp_structfield_type(struct _fat_ptr*n,struct Cyc_Absyn_Aggrfield*sf){
# 1489
if(({Cyc_strcmp((struct _fat_ptr)*n,(struct _fat_ptr)*sf->name);})== 0)return sf->type;else{
return 0;}}
# 1495
static void*Cyc_Tcexp_tcOffsetof(struct Cyc_Tcenv_Tenv*te,unsigned loc,void**topt,void*t_orig,struct Cyc_List_List*fs){
# 1497
({void(*_tmp2D2)(unsigned,struct Cyc_Tcenv_Tenv*,struct Cyc_List_List*bound_tvars,struct Cyc_Absyn_Kind*k,int allow_evars,int allow_abs_aggr,void*)=Cyc_Tctyp_check_type;unsigned _tmp2D3=loc;struct Cyc_Tcenv_Tenv*_tmp2D4=te;struct Cyc_List_List*_tmp2D5=({Cyc_Tcenv_lookup_type_vars(te);});struct Cyc_Absyn_Kind*_tmp2D6=& Cyc_Tcutil_tmk;int _tmp2D7=1;int _tmp2D8=0;void*_tmp2D9=t_orig;_tmp2D2(_tmp2D3,_tmp2D4,_tmp2D5,_tmp2D6,_tmp2D7,_tmp2D8,_tmp2D9);});
({Cyc_Tcutil_check_no_qual(loc,t_orig);});{
struct Cyc_List_List*l=fs;
void*t=t_orig;
for(0;l != 0;l=l->tl){
void*n=(void*)l->hd;
void*_tmp2DA=n;unsigned _tmp2DB;struct _fat_ptr*_tmp2DC;if(((struct Cyc_Absyn_StructField_Absyn_OffsetofField_struct*)_tmp2DA)->tag == 0U){_LL1: _tmp2DC=((struct Cyc_Absyn_StructField_Absyn_OffsetofField_struct*)_tmp2DA)->f1;_LL2: {struct _fat_ptr*n=_tmp2DC;
# 1505
int bad_type=1;
{void*_stmttmp28=({Cyc_Tcutil_compress(t);});void*_tmp2DD=_stmttmp28;struct Cyc_List_List*_tmp2DE;struct Cyc_Absyn_Aggrdecl**_tmp2DF;switch(*((int*)_tmp2DD)){case 0U: if(((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2DD)->f1)->tag == 22U){if(((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2DD)->f1)->f1).KnownAggr).tag == 2){_LL6: _tmp2DF=((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2DD)->f1)->f1).KnownAggr).val;_LL7: {struct Cyc_Absyn_Aggrdecl**adp=_tmp2DF;
# 1508
if((*adp)->impl == 0)goto _LL5;{void*t2=({({void*(*_tmp805)(void*(*pred)(struct _fat_ptr*,struct Cyc_Absyn_Aggrfield*),struct _fat_ptr*env,struct Cyc_List_List*x)=({
void*(*_tmp2E3)(void*(*pred)(struct _fat_ptr*,struct Cyc_Absyn_Aggrfield*),struct _fat_ptr*env,struct Cyc_List_List*x)=(void*(*)(void*(*pred)(struct _fat_ptr*,struct Cyc_Absyn_Aggrfield*),struct _fat_ptr*env,struct Cyc_List_List*x))Cyc_List_find_c;_tmp2E3;});struct _fat_ptr*_tmp804=n;_tmp805(Cyc_Tcexp_structfield_type,_tmp804,((struct Cyc_Absyn_AggrdeclImpl*)_check_null((*adp)->impl))->fields);});});
if(!((unsigned)t2))
({struct Cyc_String_pa_PrintArg_struct _tmp2E2=({struct Cyc_String_pa_PrintArg_struct _tmp67E;_tmp67E.tag=0U,_tmp67E.f1=(struct _fat_ptr)((struct _fat_ptr)*n);_tmp67E;});void*_tmp2E0[1U];_tmp2E0[0]=& _tmp2E2;({unsigned _tmp807=loc;struct _fat_ptr _tmp806=({const char*_tmp2E1="no field of struct/union has name %s";_tag_fat(_tmp2E1,sizeof(char),37U);});Cyc_Tcutil_terr(_tmp807,_tmp806,_tag_fat(_tmp2E0,sizeof(void*),1U));});});else{
# 1513
t=t2;}
bad_type=0;
goto _LL5;}}}else{goto _LLA;}}else{goto _LLA;}case 7U: _LL8: _tmp2DE=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp2DD)->f2;_LL9: {struct Cyc_List_List*fields=_tmp2DE;
# 1517
void*t2=({({void*(*_tmp809)(void*(*pred)(struct _fat_ptr*,struct Cyc_Absyn_Aggrfield*),struct _fat_ptr*env,struct Cyc_List_List*x)=({void*(*_tmp2E7)(void*(*pred)(struct _fat_ptr*,struct Cyc_Absyn_Aggrfield*),struct _fat_ptr*env,struct Cyc_List_List*x)=(void*(*)(void*(*pred)(struct _fat_ptr*,struct Cyc_Absyn_Aggrfield*),struct _fat_ptr*env,struct Cyc_List_List*x))Cyc_List_find_c;_tmp2E7;});struct _fat_ptr*_tmp808=n;_tmp809(Cyc_Tcexp_structfield_type,_tmp808,fields);});});
if(!((unsigned)t2))
({struct Cyc_String_pa_PrintArg_struct _tmp2E6=({struct Cyc_String_pa_PrintArg_struct _tmp67F;_tmp67F.tag=0U,_tmp67F.f1=(struct _fat_ptr)((struct _fat_ptr)*n);_tmp67F;});void*_tmp2E4[1U];_tmp2E4[0]=& _tmp2E6;({unsigned _tmp80B=loc;struct _fat_ptr _tmp80A=({const char*_tmp2E5="no field of struct/union has name %s";_tag_fat(_tmp2E5,sizeof(char),37U);});Cyc_Tcutil_terr(_tmp80B,_tmp80A,_tag_fat(_tmp2E4,sizeof(void*),1U));});});else{
# 1521
t=t2;}
bad_type=0;
goto _LL5;}default: _LLA: _LLB:
 goto _LL5;}_LL5:;}
# 1526
if(bad_type){
if(l == fs)
({struct Cyc_String_pa_PrintArg_struct _tmp2EA=({struct Cyc_String_pa_PrintArg_struct _tmp680;_tmp680.tag=0U,({struct _fat_ptr _tmp80C=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t);}));_tmp680.f1=_tmp80C;});_tmp680;});void*_tmp2E8[1U];_tmp2E8[0]=& _tmp2EA;({unsigned _tmp80E=loc;struct _fat_ptr _tmp80D=({const char*_tmp2E9="%s is not a known struct/union type";_tag_fat(_tmp2E9,sizeof(char),36U);});Cyc_Tcutil_terr(_tmp80E,_tmp80D,_tag_fat(_tmp2E8,sizeof(void*),1U));});});else{
# 1530
struct _fat_ptr s=({struct Cyc_String_pa_PrintArg_struct _tmp2FC=({struct Cyc_String_pa_PrintArg_struct _tmp687;_tmp687.tag=0U,({struct _fat_ptr _tmp80F=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t_orig);}));_tmp687.f1=_tmp80F;});_tmp687;});void*_tmp2FA[1U];_tmp2FA[0]=& _tmp2FC;({struct _fat_ptr _tmp810=({const char*_tmp2FB="(%s)";_tag_fat(_tmp2FB,sizeof(char),5U);});Cyc_aprintf(_tmp810,_tag_fat(_tmp2FA,sizeof(void*),1U));});});
struct Cyc_List_List*x;
for(x=fs;x != l;x=x->tl){
void*_stmttmp29=(void*)((struct Cyc_List_List*)_check_null(x))->hd;void*_tmp2EB=_stmttmp29;unsigned _tmp2EC;struct _fat_ptr*_tmp2ED;if(((struct Cyc_Absyn_StructField_Absyn_OffsetofField_struct*)_tmp2EB)->tag == 0U){_LLD: _tmp2ED=((struct Cyc_Absyn_StructField_Absyn_OffsetofField_struct*)_tmp2EB)->f1;_LLE: {struct _fat_ptr*n=_tmp2ED;
# 1535
s=({struct Cyc_String_pa_PrintArg_struct _tmp2F0=({struct Cyc_String_pa_PrintArg_struct _tmp682;_tmp682.tag=0U,_tmp682.f1=(struct _fat_ptr)((struct _fat_ptr)s);_tmp682;});struct Cyc_String_pa_PrintArg_struct _tmp2F1=({struct Cyc_String_pa_PrintArg_struct _tmp681;_tmp681.tag=0U,_tmp681.f1=(struct _fat_ptr)((struct _fat_ptr)*n);_tmp681;});void*_tmp2EE[2U];_tmp2EE[0]=& _tmp2F0,_tmp2EE[1]=& _tmp2F1;({struct _fat_ptr _tmp811=({const char*_tmp2EF="%s.%s";_tag_fat(_tmp2EF,sizeof(char),6U);});Cyc_aprintf(_tmp811,_tag_fat(_tmp2EE,sizeof(void*),2U));});});goto _LLC;}}else{_LLF: _tmp2EC=((struct Cyc_Absyn_TupleIndex_Absyn_OffsetofField_struct*)_tmp2EB)->f1;_LL10: {unsigned n=_tmp2EC;
# 1537
s=({struct Cyc_String_pa_PrintArg_struct _tmp2F4=({struct Cyc_String_pa_PrintArg_struct _tmp684;_tmp684.tag=0U,_tmp684.f1=(struct _fat_ptr)((struct _fat_ptr)s);_tmp684;});struct Cyc_Int_pa_PrintArg_struct _tmp2F5=({struct Cyc_Int_pa_PrintArg_struct _tmp683;_tmp683.tag=1U,_tmp683.f1=(unsigned long)((int)n);_tmp683;});void*_tmp2F2[2U];_tmp2F2[0]=& _tmp2F4,_tmp2F2[1]=& _tmp2F5;({struct _fat_ptr _tmp812=({const char*_tmp2F3="%s.%d";_tag_fat(_tmp2F3,sizeof(char),6U);});Cyc_aprintf(_tmp812,_tag_fat(_tmp2F2,sizeof(void*),2U));});});goto _LLC;}}_LLC:;}
# 1539
({struct Cyc_String_pa_PrintArg_struct _tmp2F8=({struct Cyc_String_pa_PrintArg_struct _tmp686;_tmp686.tag=0U,_tmp686.f1=(struct _fat_ptr)((struct _fat_ptr)s);_tmp686;});struct Cyc_String_pa_PrintArg_struct _tmp2F9=({struct Cyc_String_pa_PrintArg_struct _tmp685;_tmp685.tag=0U,({struct _fat_ptr _tmp813=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t);}));_tmp685.f1=_tmp813;});_tmp685;});void*_tmp2F6[2U];_tmp2F6[0]=& _tmp2F8,_tmp2F6[1]=& _tmp2F9;({unsigned _tmp815=loc;struct _fat_ptr _tmp814=({const char*_tmp2F7="%s == %s is not a struct/union type";_tag_fat(_tmp2F7,sizeof(char),36U);});Cyc_Tcutil_terr(_tmp815,_tmp814,_tag_fat(_tmp2F6,sizeof(void*),2U));});});}}
# 1526
goto _LL0;}}else{_LL3: _tmp2DB=((struct Cyc_Absyn_TupleIndex_Absyn_OffsetofField_struct*)_tmp2DA)->f1;_LL4: {unsigned n=_tmp2DB;
# 1544
int bad_type=1;
{void*_stmttmp2A=({Cyc_Tcutil_compress(t);});void*_tmp2FD=_stmttmp2A;struct Cyc_List_List*_tmp2FE;struct Cyc_List_List*_tmp2FF;struct Cyc_Absyn_Datatypefield*_tmp300;struct Cyc_Absyn_Aggrdecl**_tmp301;switch(*((int*)_tmp2FD)){case 0U: switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2FD)->f1)){case 22U: if(((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2FD)->f1)->f1).KnownAggr).tag == 2){_LL12: _tmp301=((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2FD)->f1)->f1).KnownAggr).val;_LL13: {struct Cyc_Absyn_Aggrdecl**adp=_tmp301;
# 1547
if((*adp)->impl == 0)
goto _LL11;
# 1547
_tmp2FF=((struct Cyc_Absyn_AggrdeclImpl*)_check_null((*adp)->impl))->fields;goto _LL15;}}else{goto _LL1A;}case 21U: if(((((struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2FD)->f1)->f1).KnownDatatypefield).tag == 2){_LL18: _tmp300=(((((struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2FD)->f1)->f1).KnownDatatypefield).val).f2;_LL19: {struct Cyc_Absyn_Datatypefield*tuf=_tmp300;
# 1567
if(({unsigned _tmp816=(unsigned)({Cyc_List_length(tuf->typs);});_tmp816 < n;}))
({struct Cyc_Int_pa_PrintArg_struct _tmp30E=({struct Cyc_Int_pa_PrintArg_struct _tmp689;_tmp689.tag=1U,({
unsigned long _tmp817=(unsigned long)({Cyc_List_length(tuf->typs);});_tmp689.f1=_tmp817;});_tmp689;});struct Cyc_Int_pa_PrintArg_struct _tmp30F=({struct Cyc_Int_pa_PrintArg_struct _tmp688;_tmp688.tag=1U,_tmp688.f1=(unsigned long)((int)n);_tmp688;});void*_tmp30C[2U];_tmp30C[0]=& _tmp30E,_tmp30C[1]=& _tmp30F;({unsigned _tmp819=loc;struct _fat_ptr _tmp818=({const char*_tmp30D="datatype field has too few components: %d < %d";_tag_fat(_tmp30D,sizeof(char),47U);});Cyc_Tcutil_terr(_tmp819,_tmp818,_tag_fat(_tmp30C,sizeof(void*),2U));});});else{
# 1571
if(n != (unsigned)0)
t=(*({({struct _tuple18*(*_tmp81B)(struct Cyc_List_List*x,int n)=({struct _tuple18*(*_tmp310)(struct Cyc_List_List*x,int n)=(struct _tuple18*(*)(struct Cyc_List_List*x,int n))Cyc_List_nth;_tmp310;});struct Cyc_List_List*_tmp81A=tuf->typs;_tmp81B(_tmp81A,(int)(n - (unsigned)1));});})).f2;else{
if(l->tl != 0)
({void*_tmp311=0U;({unsigned _tmp81D=loc;struct _fat_ptr _tmp81C=({const char*_tmp312="datatype field index 0 refers to the tag; cannot be indexed through";_tag_fat(_tmp312,sizeof(char),68U);});Cyc_Tcutil_terr(_tmp81D,_tmp81C,_tag_fat(_tmp311,sizeof(void*),0U));});});}}
# 1576
bad_type=0;
goto _LL11;}}else{goto _LL1A;}default: goto _LL1A;}case 7U: _LL14: _tmp2FF=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp2FD)->f2;_LL15: {struct Cyc_List_List*fields=_tmp2FF;
# 1551
if(({unsigned _tmp81E=(unsigned)({Cyc_List_length(fields);});_tmp81E <= n;}))
({struct Cyc_Int_pa_PrintArg_struct _tmp304=({struct Cyc_Int_pa_PrintArg_struct _tmp68B;_tmp68B.tag=1U,({
unsigned long _tmp81F=(unsigned long)({Cyc_List_length(fields);});_tmp68B.f1=_tmp81F;});_tmp68B;});struct Cyc_Int_pa_PrintArg_struct _tmp305=({struct Cyc_Int_pa_PrintArg_struct _tmp68A;_tmp68A.tag=1U,_tmp68A.f1=(unsigned long)((int)n);_tmp68A;});void*_tmp302[2U];_tmp302[0]=& _tmp304,_tmp302[1]=& _tmp305;({unsigned _tmp821=loc;struct _fat_ptr _tmp820=({const char*_tmp303="struct/union has too few components: %d <= %d";_tag_fat(_tmp303,sizeof(char),46U);});Cyc_Tcutil_terr(_tmp821,_tmp820,_tag_fat(_tmp302,sizeof(void*),2U));});});else{
# 1555
t=({({struct Cyc_Absyn_Aggrfield*(*_tmp823)(struct Cyc_List_List*x,int n)=({struct Cyc_Absyn_Aggrfield*(*_tmp306)(struct Cyc_List_List*x,int n)=(struct Cyc_Absyn_Aggrfield*(*)(struct Cyc_List_List*x,int n))Cyc_List_nth;_tmp306;});struct Cyc_List_List*_tmp822=fields;_tmp823(_tmp822,(int)n);});})->type;}
bad_type=0;
goto _LL11;}case 6U: _LL16: _tmp2FE=((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp2FD)->f1;_LL17: {struct Cyc_List_List*l=_tmp2FE;
# 1559
if(({unsigned _tmp824=(unsigned)({Cyc_List_length(l);});_tmp824 <= n;}))
({struct Cyc_Int_pa_PrintArg_struct _tmp309=({struct Cyc_Int_pa_PrintArg_struct _tmp68D;_tmp68D.tag=1U,({
unsigned long _tmp825=(unsigned long)({Cyc_List_length(l);});_tmp68D.f1=_tmp825;});_tmp68D;});struct Cyc_Int_pa_PrintArg_struct _tmp30A=({struct Cyc_Int_pa_PrintArg_struct _tmp68C;_tmp68C.tag=1U,_tmp68C.f1=(unsigned long)((int)n);_tmp68C;});void*_tmp307[2U];_tmp307[0]=& _tmp309,_tmp307[1]=& _tmp30A;({unsigned _tmp827=loc;struct _fat_ptr _tmp826=({const char*_tmp308="tuple has too few components: %d <= %d";_tag_fat(_tmp308,sizeof(char),39U);});Cyc_Tcutil_terr(_tmp827,_tmp826,_tag_fat(_tmp307,sizeof(void*),2U));});});else{
# 1563
t=(*({({struct _tuple18*(*_tmp829)(struct Cyc_List_List*x,int n)=({struct _tuple18*(*_tmp30B)(struct Cyc_List_List*x,int n)=(struct _tuple18*(*)(struct Cyc_List_List*x,int n))Cyc_List_nth;_tmp30B;});struct Cyc_List_List*_tmp828=l;_tmp829(_tmp828,(int)n);});})).f2;}
bad_type=0;
goto _LL11;}default: _LL1A: _LL1B:
# 1578
 goto _LL11;}_LL11:;}
# 1580
if(bad_type)
({struct Cyc_String_pa_PrintArg_struct _tmp315=({struct Cyc_String_pa_PrintArg_struct _tmp68E;_tmp68E.tag=0U,({struct _fat_ptr _tmp82A=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t);}));_tmp68E.f1=_tmp82A;});_tmp68E;});void*_tmp313[1U];_tmp313[0]=& _tmp315;({unsigned _tmp82C=loc;struct _fat_ptr _tmp82B=({const char*_tmp314="%s is not a known type";_tag_fat(_tmp314,sizeof(char),23U);});Cyc_Tcutil_terr(_tmp82C,_tmp82B,_tag_fat(_tmp313,sizeof(void*),1U));});});
# 1580
goto _LL0;}}_LL0:;}
# 1585
return Cyc_Absyn_uint_type;}}
# 1589
static void*Cyc_Tcexp_tcDeref(struct Cyc_Tcenv_Tenv*te_orig,unsigned loc,void**topt,struct Cyc_Absyn_Exp*e){
struct Cyc_Tcenv_Tenv*te=({struct Cyc_Tcenv_Tenv*(*_tmp32A)(struct Cyc_Tcenv_Tenv*)=Cyc_Tcenv_clear_lhs;struct Cyc_Tcenv_Tenv*_tmp32B=({Cyc_Tcenv_clear_notreadctxt(te_orig);});_tmp32A(_tmp32B);});
({Cyc_Tcexp_tcExp(te,0,e);});{
void*t=({Cyc_Tcutil_compress((void*)_check_null(e->topt));});
void*_tmp317=t;void*_tmp31B;void*_tmp31A;void*_tmp319;void*_tmp318;switch(*((int*)_tmp317)){case 1U: _LL1: _LL2: {
# 1595
struct Cyc_List_List*tenv_tvs=({Cyc_Tcenv_lookup_type_vars(te);});
void*t2=({Cyc_Absyn_new_evar(& Cyc_Tcutil_ako,({struct Cyc_Core_Opt*_tmp31E=_cycalloc(sizeof(*_tmp31E));_tmp31E->v=tenv_tvs;_tmp31E;}));});
void*rt=({Cyc_Absyn_new_evar(& Cyc_Tcutil_trko,({struct Cyc_Core_Opt*_tmp31D=_cycalloc(sizeof(*_tmp31D));_tmp31D->v=tenv_tvs;_tmp31D;}));});
void*b=({Cyc_Tcutil_any_bounds(tenv_tvs);});
void*zt=({Cyc_Tcutil_any_bool(tenv_tvs);});
struct Cyc_Absyn_PtrAtts atts=({struct Cyc_Absyn_PtrAtts _tmp68F;_tmp68F.rgn=rt,({void*_tmp82D=({Cyc_Tcutil_any_bool(tenv_tvs);});_tmp68F.nullable=_tmp82D;}),_tmp68F.bounds=b,_tmp68F.zero_term=zt,_tmp68F.ptrloc=0;_tmp68F;});
struct Cyc_Absyn_PointerType_Absyn_Type_struct*new_typ=({struct Cyc_Absyn_PointerType_Absyn_Type_struct*_tmp31C=_cycalloc(sizeof(*_tmp31C));_tmp31C->tag=3U,(_tmp31C->f1).elt_type=t2,({struct Cyc_Absyn_Tqual _tmp82E=({Cyc_Absyn_empty_tqual(0U);});(_tmp31C->f1).elt_tq=_tmp82E;}),(_tmp31C->f1).ptr_atts=atts;_tmp31C;});
({Cyc_Unify_unify(t,(void*)new_typ);});
_tmp318=t2;_tmp319=rt;_tmp31A=b;_tmp31B=zt;goto _LL4;}case 3U: _LL3: _tmp318=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp317)->f1).elt_type;_tmp319=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp317)->f1).ptr_atts).rgn;_tmp31A=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp317)->f1).ptr_atts).bounds;_tmp31B=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp317)->f1).ptr_atts).zero_term;_LL4: {void*t2=_tmp318;void*rt=_tmp319;void*b=_tmp31A;void*zt=_tmp31B;
# 1605
({Cyc_Tcenv_check_rgn_accessible(te,loc,rt);});
({Cyc_Tcutil_check_nonzero_bound(loc,b);});
if(!({int(*_tmp31F)(struct Cyc_Absyn_Kind*,struct Cyc_Absyn_Kind*)=Cyc_Tcutil_kind_leq;struct Cyc_Absyn_Kind*_tmp320=({Cyc_Tcutil_type_kind(t2);});struct Cyc_Absyn_Kind*_tmp321=& Cyc_Tcutil_tmk;_tmp31F(_tmp320,_tmp321);})&& !({Cyc_Tcenv_abstract_val_ok(te);})){
void*_stmttmp2B=({Cyc_Tcutil_compress(t2);});void*_tmp322=_stmttmp2B;if(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp322)->tag == 5U){_LL8: _LL9:
# 1610
 if(Cyc_Tc_aggressive_warn)
({void*_tmp323=0U;({unsigned _tmp830=loc;struct _fat_ptr _tmp82F=({const char*_tmp324="unnecessary dereference for function type";_tag_fat(_tmp324,sizeof(char),42U);});Cyc_Tcutil_warn(_tmp830,_tmp82F,_tag_fat(_tmp323,sizeof(void*),0U));});});
# 1610
return t;}else{_LLA: _LLB:
# 1614
({void*_tmp325=0U;({unsigned _tmp832=loc;struct _fat_ptr _tmp831=({const char*_tmp326="can't dereference abstract pointer type";_tag_fat(_tmp326,sizeof(char),40U);});Cyc_Tcutil_terr(_tmp832,_tmp831,_tag_fat(_tmp325,sizeof(void*),0U));});});}_LL7:;}
# 1607
return t2;}default: _LL5: _LL6:
# 1619
 return({struct Cyc_String_pa_PrintArg_struct _tmp329=({struct Cyc_String_pa_PrintArg_struct _tmp690;_tmp690.tag=0U,({struct _fat_ptr _tmp833=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t);}));_tmp690.f1=_tmp833;});_tmp690;});void*_tmp327[1U];_tmp327[0]=& _tmp329;({struct Cyc_Tcenv_Tenv*_tmp837=te;unsigned _tmp836=loc;void**_tmp835=topt;struct _fat_ptr _tmp834=({const char*_tmp328="expecting *, @, or ? type but found %s";_tag_fat(_tmp328,sizeof(char),39U);});Cyc_Tcexp_expr_err(_tmp837,_tmp836,_tmp835,_tmp834,_tag_fat(_tmp327,sizeof(void*),1U));});});}_LL0:;}}
# 1624
static void*Cyc_Tcexp_tcAggrMember(struct Cyc_Tcenv_Tenv*te,unsigned loc,void**topt,struct Cyc_Absyn_Exp*outer_e,struct Cyc_Absyn_Exp*e,struct _fat_ptr*f,int*is_tagged,int*is_read){
# 1630
({void*(*_tmp32D)(struct Cyc_Tcenv_Tenv*te,void**topt,struct Cyc_Absyn_Exp*e)=Cyc_Tcexp_tcExpNoPromote;struct Cyc_Tcenv_Tenv*_tmp32E=({Cyc_Tcenv_enter_abstract_val_ok(te);});void**_tmp32F=0;struct Cyc_Absyn_Exp*_tmp330=e;_tmp32D(_tmp32E,_tmp32F,_tmp330);});
# 1632
({int _tmp838=!({Cyc_Tcenv_in_notreadctxt(te);});*is_read=_tmp838;});{
void*_stmttmp2C=({Cyc_Tcutil_compress((void*)_check_null(e->topt));});void*_tmp331=_stmttmp2C;struct Cyc_List_List*_tmp333;enum Cyc_Absyn_AggrKind _tmp332;struct Cyc_List_List*_tmp335;struct Cyc_Absyn_Aggrdecl*_tmp334;switch(*((int*)_tmp331)){case 0U: if(((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp331)->f1)->tag == 22U){if(((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp331)->f1)->f1).KnownAggr).tag == 2){_LL1: _tmp334=*((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp331)->f1)->f1).KnownAggr).val;_tmp335=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp331)->f2;_LL2: {struct Cyc_Absyn_Aggrdecl*ad=_tmp334;struct Cyc_List_List*ts=_tmp335;
# 1635
struct Cyc_Absyn_Aggrfield*finfo=({Cyc_Absyn_lookup_decl_field(ad,f);});
if(finfo == 0)
return({struct Cyc_String_pa_PrintArg_struct _tmp338=({struct Cyc_String_pa_PrintArg_struct _tmp692;_tmp692.tag=0U,({
struct _fat_ptr _tmp839=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(ad->name);}));_tmp692.f1=_tmp839;});_tmp692;});struct Cyc_String_pa_PrintArg_struct _tmp339=({struct Cyc_String_pa_PrintArg_struct _tmp691;_tmp691.tag=0U,_tmp691.f1=(struct _fat_ptr)((struct _fat_ptr)*f);_tmp691;});void*_tmp336[2U];_tmp336[0]=& _tmp338,_tmp336[1]=& _tmp339;({struct Cyc_Tcenv_Tenv*_tmp83D=te;unsigned _tmp83C=loc;void**_tmp83B=topt;struct _fat_ptr _tmp83A=({const char*_tmp337="%s has no %s member";_tag_fat(_tmp337,sizeof(char),20U);});Cyc_Tcexp_expr_err(_tmp83D,_tmp83C,_tmp83B,_tmp83A,_tag_fat(_tmp336,sizeof(void*),2U));});});
# 1636
if(((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->tagged)
# 1640
*is_tagged=1;
# 1636
if(({Cyc_Tcutil_is_tagged_union_and_needs_check(e);}))
# 1645
;{
# 1636
void*t2=finfo->type;
# 1649
if(ts != 0){
struct Cyc_List_List*inst=({Cyc_List_zip(ad->tvs,ts);});
t2=({Cyc_Tcutil_substitute(inst,finfo->type);});}
# 1649
if(
# 1655
((((int)ad->kind == (int)Cyc_Absyn_UnionA && !((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->tagged)&& !({Cyc_Tcutil_is_bits_only_type(t2);}))&& !({Cyc_Tcenv_in_notreadctxt(te);}))&& finfo->requires_clause == 0)
return({struct Cyc_String_pa_PrintArg_struct _tmp33C=({struct Cyc_String_pa_PrintArg_struct _tmp693;_tmp693.tag=0U,_tmp693.f1=(struct _fat_ptr)((struct _fat_ptr)*f);_tmp693;});void*_tmp33A[1U];_tmp33A[0]=& _tmp33C;({struct Cyc_Tcenv_Tenv*_tmp841=te;unsigned _tmp840=loc;void**_tmp83F=topt;struct _fat_ptr _tmp83E=({const char*_tmp33B="cannot read union member %s since it is not `bits-only'";_tag_fat(_tmp33B,sizeof(char),56U);});Cyc_Tcexp_expr_err(_tmp841,_tmp840,_tmp83F,_tmp83E,_tag_fat(_tmp33A,sizeof(void*),1U));});});
# 1649
if(((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->exist_vars != 0){
# 1660
if(!({int(*_tmp33D)(void*,void*)=Cyc_Unify_unify;void*_tmp33E=t2;void*_tmp33F=({void*(*_tmp340)(struct Cyc_Core_Opt*)=Cyc_Absyn_wildtyp;struct Cyc_Core_Opt*_tmp341=({Cyc_Tcenv_lookup_opt_type_vars(te);});_tmp340(_tmp341);});_tmp33D(_tmp33E,_tmp33F);}))
return({struct Cyc_String_pa_PrintArg_struct _tmp344=({struct Cyc_String_pa_PrintArg_struct _tmp694;_tmp694.tag=0U,_tmp694.f1=(struct _fat_ptr)((struct _fat_ptr)*f);_tmp694;});void*_tmp342[1U];_tmp342[0]=& _tmp344;({struct Cyc_Tcenv_Tenv*_tmp845=te;unsigned _tmp844=loc;void**_tmp843=topt;struct _fat_ptr _tmp842=({const char*_tmp343="must use pattern-matching to access field %s\n\tdue to existential type variables.";_tag_fat(_tmp343,sizeof(char),81U);});Cyc_Tcexp_expr_err(_tmp845,_tmp844,_tmp843,_tmp842,_tag_fat(_tmp342,sizeof(void*),1U));});});}
# 1649
return t2;}}}else{goto _LL5;}}else{goto _LL5;}case 7U: _LL3: _tmp332=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp331)->f1;_tmp333=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp331)->f2;_LL4: {enum Cyc_Absyn_AggrKind k=_tmp332;struct Cyc_List_List*fs=_tmp333;
# 1665
struct Cyc_Absyn_Aggrfield*finfo=({Cyc_Absyn_lookup_field(fs,f);});
if(finfo == 0)
return({struct Cyc_String_pa_PrintArg_struct _tmp347=({struct Cyc_String_pa_PrintArg_struct _tmp695;_tmp695.tag=0U,_tmp695.f1=(struct _fat_ptr)((struct _fat_ptr)*f);_tmp695;});void*_tmp345[1U];_tmp345[0]=& _tmp347;({struct Cyc_Tcenv_Tenv*_tmp849=te;unsigned _tmp848=loc;void**_tmp847=topt;struct _fat_ptr _tmp846=({const char*_tmp346="type has no %s member";_tag_fat(_tmp346,sizeof(char),22U);});Cyc_Tcexp_expr_err(_tmp849,_tmp848,_tmp847,_tmp846,_tag_fat(_tmp345,sizeof(void*),1U));});});
# 1666
if(
# 1670
(((int)k == (int)1U && !({Cyc_Tcutil_is_bits_only_type(finfo->type);}))&& !({Cyc_Tcenv_in_notreadctxt(te);}))&& finfo->requires_clause == 0)
# 1672
return({struct Cyc_String_pa_PrintArg_struct _tmp34A=({struct Cyc_String_pa_PrintArg_struct _tmp696;_tmp696.tag=0U,_tmp696.f1=(struct _fat_ptr)((struct _fat_ptr)*f);_tmp696;});void*_tmp348[1U];_tmp348[0]=& _tmp34A;({struct Cyc_Tcenv_Tenv*_tmp84D=te;unsigned _tmp84C=loc;void**_tmp84B=topt;struct _fat_ptr _tmp84A=({const char*_tmp349="cannot read union member %s since it is not `bits-only'";_tag_fat(_tmp349,sizeof(char),56U);});Cyc_Tcexp_expr_err(_tmp84D,_tmp84C,_tmp84B,_tmp84A,_tag_fat(_tmp348,sizeof(void*),1U));});});
# 1666
return finfo->type;}default: _LL5: _LL6:
# 1675
 return({struct Cyc_String_pa_PrintArg_struct _tmp34D=({struct Cyc_String_pa_PrintArg_struct _tmp697;_tmp697.tag=0U,({
struct _fat_ptr _tmp84E=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string((void*)_check_null(e->topt));}));_tmp697.f1=_tmp84E;});_tmp697;});void*_tmp34B[1U];_tmp34B[0]=& _tmp34D;({struct Cyc_Tcenv_Tenv*_tmp852=te;unsigned _tmp851=loc;void**_tmp850=topt;struct _fat_ptr _tmp84F=({const char*_tmp34C="expecting struct or union, found %s";_tag_fat(_tmp34C,sizeof(char),36U);});Cyc_Tcexp_expr_err(_tmp852,_tmp851,_tmp850,_tmp84F,_tag_fat(_tmp34B,sizeof(void*),1U));});});}_LL0:;}}
# 1681
static void*Cyc_Tcexp_tcAggrArrow(struct Cyc_Tcenv_Tenv*te,unsigned loc,void**topt,struct Cyc_Absyn_Exp*e,struct _fat_ptr*f,int*is_tagged,int*is_read){
# 1684
({void*(*_tmp34F)(struct Cyc_Tcenv_Tenv*,void**,struct Cyc_Absyn_Exp*)=Cyc_Tcexp_tcExp;struct Cyc_Tcenv_Tenv*_tmp350=({struct Cyc_Tcenv_Tenv*(*_tmp351)(struct Cyc_Tcenv_Tenv*)=Cyc_Tcenv_enter_abstract_val_ok;struct Cyc_Tcenv_Tenv*_tmp352=({struct Cyc_Tcenv_Tenv*(*_tmp353)(struct Cyc_Tcenv_Tenv*)=Cyc_Tcenv_clear_lhs;struct Cyc_Tcenv_Tenv*_tmp354=({Cyc_Tcenv_clear_notreadctxt(te);});_tmp353(_tmp354);});_tmp351(_tmp352);});void**_tmp355=0;struct Cyc_Absyn_Exp*_tmp356=e;_tmp34F(_tmp350,_tmp355,_tmp356);});
# 1686
({int _tmp853=!({Cyc_Tcenv_in_notreadctxt(te);});*is_read=_tmp853;});
{void*_stmttmp2D=({Cyc_Tcutil_compress((void*)_check_null(e->topt));});void*_tmp357=_stmttmp2D;void*_tmp35B;void*_tmp35A;void*_tmp359;void*_tmp358;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp357)->tag == 3U){_LL1: _tmp358=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp357)->f1).elt_type;_tmp359=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp357)->f1).ptr_atts).rgn;_tmp35A=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp357)->f1).ptr_atts).bounds;_tmp35B=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp357)->f1).ptr_atts).zero_term;_LL2: {void*t2=_tmp358;void*rt=_tmp359;void*b=_tmp35A;void*zt=_tmp35B;
# 1689
({Cyc_Tcutil_check_nonzero_bound(loc,b);});
{void*_stmttmp2E=({Cyc_Tcutil_compress(t2);});void*_tmp35C=_stmttmp2E;struct Cyc_List_List*_tmp35E;enum Cyc_Absyn_AggrKind _tmp35D;struct Cyc_List_List*_tmp360;struct Cyc_Absyn_Aggrdecl*_tmp35F;switch(*((int*)_tmp35C)){case 0U: if(((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp35C)->f1)->tag == 22U){if(((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp35C)->f1)->f1).KnownAggr).tag == 2){_LL6: _tmp35F=*((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp35C)->f1)->f1).KnownAggr).val;_tmp360=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp35C)->f2;_LL7: {struct Cyc_Absyn_Aggrdecl*ad=_tmp35F;struct Cyc_List_List*ts=_tmp360;
# 1692
struct Cyc_Absyn_Aggrfield*finfo=({Cyc_Absyn_lookup_decl_field(ad,f);});
if(finfo == 0)
return({struct Cyc_String_pa_PrintArg_struct _tmp363=({struct Cyc_String_pa_PrintArg_struct _tmp699;_tmp699.tag=0U,({
struct _fat_ptr _tmp854=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(ad->name);}));_tmp699.f1=_tmp854;});_tmp699;});struct Cyc_String_pa_PrintArg_struct _tmp364=({struct Cyc_String_pa_PrintArg_struct _tmp698;_tmp698.tag=0U,_tmp698.f1=(struct _fat_ptr)((struct _fat_ptr)*f);_tmp698;});void*_tmp361[2U];_tmp361[0]=& _tmp363,_tmp361[1]=& _tmp364;({struct Cyc_Tcenv_Tenv*_tmp858=te;unsigned _tmp857=loc;void**_tmp856=topt;struct _fat_ptr _tmp855=({const char*_tmp362="type %s has no %s member";_tag_fat(_tmp362,sizeof(char),25U);});Cyc_Tcexp_expr_err(_tmp858,_tmp857,_tmp856,_tmp855,_tag_fat(_tmp361,sizeof(void*),2U));});});
# 1693
if(((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->tagged)
# 1697
*is_tagged=1;{
# 1693
void*t3=finfo->type;
# 1699
if(ts != 0){
struct Cyc_List_List*inst=({Cyc_List_zip(ad->tvs,ts);});
t3=({Cyc_Tcutil_substitute(inst,finfo->type);});}{
# 1699
struct Cyc_Absyn_Kind*t3_kind=({Cyc_Tcutil_type_kind(t3);});
# 1706
if(({Cyc_Tcutil_kind_leq(& Cyc_Tcutil_ak,t3_kind);})&& !({Cyc_Tcenv_abstract_val_ok(te);})){
void*_stmttmp2F=({Cyc_Tcutil_compress(t3);});void*_tmp365=_stmttmp2F;if(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp365)->tag == 4U){_LLD: _LLE:
 goto _LLC;}else{_LLF: _LL10:
# 1710
 return({struct Cyc_String_pa_PrintArg_struct _tmp368=({struct Cyc_String_pa_PrintArg_struct _tmp69A;_tmp69A.tag=0U,_tmp69A.f1=(struct _fat_ptr)((struct _fat_ptr)*f);_tmp69A;});void*_tmp366[1U];_tmp366[0]=& _tmp368;({struct Cyc_Tcenv_Tenv*_tmp85C=te;unsigned _tmp85B=loc;void**_tmp85A=topt;struct _fat_ptr _tmp859=({const char*_tmp367="cannot get member %s since its type is abstract";_tag_fat(_tmp367,sizeof(char),48U);});Cyc_Tcexp_expr_err(_tmp85C,_tmp85B,_tmp85A,_tmp859,_tag_fat(_tmp366,sizeof(void*),1U));});});}_LLC:;}
# 1706
if(
# 1715
((((int)ad->kind == (int)Cyc_Absyn_UnionA && !((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->tagged)&& !({Cyc_Tcutil_is_bits_only_type(t3);}))&& !({Cyc_Tcenv_in_notreadctxt(te);}))&& finfo->requires_clause == 0)
# 1718
return({struct Cyc_String_pa_PrintArg_struct _tmp36B=({struct Cyc_String_pa_PrintArg_struct _tmp69B;_tmp69B.tag=0U,_tmp69B.f1=(struct _fat_ptr)((struct _fat_ptr)*f);_tmp69B;});void*_tmp369[1U];_tmp369[0]=& _tmp36B;({struct Cyc_Tcenv_Tenv*_tmp860=te;unsigned _tmp85F=loc;void**_tmp85E=topt;struct _fat_ptr _tmp85D=({const char*_tmp36A="cannot read union member %s since it is not `bits-only'";_tag_fat(_tmp36A,sizeof(char),56U);});Cyc_Tcexp_expr_err(_tmp860,_tmp85F,_tmp85E,_tmp85D,_tag_fat(_tmp369,sizeof(void*),1U));});});
# 1706
if(((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->exist_vars != 0){
# 1720
if(!({int(*_tmp36C)(void*,void*)=Cyc_Unify_unify;void*_tmp36D=t3;void*_tmp36E=({void*(*_tmp36F)(struct Cyc_Core_Opt*)=Cyc_Absyn_wildtyp;struct Cyc_Core_Opt*_tmp370=({Cyc_Tcenv_lookup_opt_type_vars(te);});_tmp36F(_tmp370);});_tmp36C(_tmp36D,_tmp36E);}))
return({struct Cyc_String_pa_PrintArg_struct _tmp373=({struct Cyc_String_pa_PrintArg_struct _tmp69C;_tmp69C.tag=0U,_tmp69C.f1=(struct _fat_ptr)((struct _fat_ptr)*f);_tmp69C;});void*_tmp371[1U];_tmp371[0]=& _tmp373;({struct Cyc_Tcenv_Tenv*_tmp864=te;unsigned _tmp863=loc;void**_tmp862=topt;struct _fat_ptr _tmp861=({const char*_tmp372="must use pattern-matching to access field %s\n\tdue to extistential types";_tag_fat(_tmp372,sizeof(char),72U);});Cyc_Tcexp_expr_err(_tmp864,_tmp863,_tmp862,_tmp861,_tag_fat(_tmp371,sizeof(void*),1U));});});}
# 1706
return t3;}}}}else{goto _LLA;}}else{goto _LLA;}case 7U: _LL8: _tmp35D=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp35C)->f1;_tmp35E=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp35C)->f2;_LL9: {enum Cyc_Absyn_AggrKind k=_tmp35D;struct Cyc_List_List*fs=_tmp35E;
# 1727
struct Cyc_Absyn_Aggrfield*finfo=({Cyc_Absyn_lookup_field(fs,f);});
if(finfo == 0)
return({struct Cyc_String_pa_PrintArg_struct _tmp376=({struct Cyc_String_pa_PrintArg_struct _tmp69D;_tmp69D.tag=0U,_tmp69D.f1=(struct _fat_ptr)((struct _fat_ptr)*f);_tmp69D;});void*_tmp374[1U];_tmp374[0]=& _tmp376;({struct Cyc_Tcenv_Tenv*_tmp868=te;unsigned _tmp867=loc;void**_tmp866=topt;struct _fat_ptr _tmp865=({const char*_tmp375="type has no %s field";_tag_fat(_tmp375,sizeof(char),21U);});Cyc_Tcexp_expr_err(_tmp868,_tmp867,_tmp866,_tmp865,_tag_fat(_tmp374,sizeof(void*),1U));});});
# 1728
if(
# 1732
((int)k == (int)1U && !({Cyc_Tcutil_is_bits_only_type(finfo->type);}))&& !({Cyc_Tcenv_in_notreadctxt(te);}))
return({struct Cyc_String_pa_PrintArg_struct _tmp379=({struct Cyc_String_pa_PrintArg_struct _tmp69E;_tmp69E.tag=0U,_tmp69E.f1=(struct _fat_ptr)((struct _fat_ptr)*f);_tmp69E;});void*_tmp377[1U];_tmp377[0]=& _tmp379;({struct Cyc_Tcenv_Tenv*_tmp86C=te;unsigned _tmp86B=loc;void**_tmp86A=topt;struct _fat_ptr _tmp869=({const char*_tmp378="cannot read union member %s since it is not `bits-only'";_tag_fat(_tmp378,sizeof(char),56U);});Cyc_Tcexp_expr_err(_tmp86C,_tmp86B,_tmp86A,_tmp869,_tag_fat(_tmp377,sizeof(void*),1U));});});
# 1728
return finfo->type;}default: _LLA: _LLB:
# 1735
 goto _LL5;}_LL5:;}
# 1737
goto _LL0;}}else{_LL3: _LL4:
 goto _LL0;}_LL0:;}
# 1740
return({struct Cyc_String_pa_PrintArg_struct _tmp37C=({struct Cyc_String_pa_PrintArg_struct _tmp69F;_tmp69F.tag=0U,({
struct _fat_ptr _tmp86D=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string((void*)_check_null(e->topt));}));_tmp69F.f1=_tmp86D;});_tmp69F;});void*_tmp37A[1U];_tmp37A[0]=& _tmp37C;({struct Cyc_Tcenv_Tenv*_tmp871=te;unsigned _tmp870=loc;void**_tmp86F=topt;struct _fat_ptr _tmp86E=({const char*_tmp37B="expecting struct or union pointer, found %s";_tag_fat(_tmp37B,sizeof(char),44U);});Cyc_Tcexp_expr_err(_tmp871,_tmp870,_tmp86F,_tmp86E,_tag_fat(_tmp37A,sizeof(void*),1U));});});}
# 1745
static void*Cyc_Tcexp_ithTupleType(struct Cyc_Tcenv_Tenv*te,unsigned loc,struct Cyc_List_List*ts,struct Cyc_Absyn_Exp*index){
# 1747
struct _tuple17 _stmttmp30=({Cyc_Evexp_eval_const_uint_exp(index);});int _tmp37F;unsigned _tmp37E;_LL1: _tmp37E=_stmttmp30.f1;_tmp37F=_stmttmp30.f2;_LL2: {unsigned i=_tmp37E;int known=_tmp37F;
if(!known)
return({void*_tmp380=0U;({struct Cyc_Tcenv_Tenv*_tmp874=te;unsigned _tmp873=loc;struct _fat_ptr _tmp872=({const char*_tmp381="tuple projection cannot use sizeof or offsetof";_tag_fat(_tmp381,sizeof(char),47U);});Cyc_Tcexp_expr_err(_tmp874,_tmp873,0,_tmp872,_tag_fat(_tmp380,sizeof(void*),0U));});});{
# 1748
struct _handler_cons _tmp382;_push_handler(& _tmp382);{int _tmp384=0;if(setjmp(_tmp382.handler))_tmp384=1;if(!_tmp384){
# 1752
{void*_tmp386=(*({({struct _tuple18*(*_tmp876)(struct Cyc_List_List*x,int n)=({struct _tuple18*(*_tmp385)(struct Cyc_List_List*x,int n)=(struct _tuple18*(*)(struct Cyc_List_List*x,int n))Cyc_List_nth;_tmp385;});struct Cyc_List_List*_tmp875=ts;_tmp876(_tmp875,(int)i);});})).f2;_npop_handler(0U);return _tmp386;};_pop_handler();}else{void*_tmp383=(void*)Cyc_Core_get_exn_thrown();void*_tmp387=_tmp383;void*_tmp388;if(((struct Cyc_List_Nth_exn_struct*)_tmp387)->tag == Cyc_List_Nth){_LL4: _LL5:
# 1754
 return({struct Cyc_Int_pa_PrintArg_struct _tmp38B=({struct Cyc_Int_pa_PrintArg_struct _tmp6A1;_tmp6A1.tag=1U,_tmp6A1.f1=(unsigned long)((int)i);_tmp6A1;});struct Cyc_Int_pa_PrintArg_struct _tmp38C=({struct Cyc_Int_pa_PrintArg_struct _tmp6A0;_tmp6A0.tag=1U,({
unsigned long _tmp877=(unsigned long)({Cyc_List_length(ts);});_tmp6A0.f1=_tmp877;});_tmp6A0;});void*_tmp389[2U];_tmp389[0]=& _tmp38B,_tmp389[1]=& _tmp38C;({struct Cyc_Tcenv_Tenv*_tmp87A=te;unsigned _tmp879=loc;struct _fat_ptr _tmp878=({const char*_tmp38A="index is %d but tuple has only %d fields";_tag_fat(_tmp38A,sizeof(char),41U);});Cyc_Tcexp_expr_err(_tmp87A,_tmp879,0,_tmp878,_tag_fat(_tmp389,sizeof(void*),2U));});});}else{_LL6: _tmp388=_tmp387;_LL7: {void*exn=_tmp388;(int)_rethrow(exn);}}_LL3:;}}}}}
# 1759
static void*Cyc_Tcexp_tcSubscript(struct Cyc_Tcenv_Tenv*te_orig,unsigned loc,void**topt,struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2){
# 1761
struct Cyc_Tcenv_Tenv*te=({struct Cyc_Tcenv_Tenv*(*_tmp3BB)(struct Cyc_Tcenv_Tenv*)=Cyc_Tcenv_clear_lhs;struct Cyc_Tcenv_Tenv*_tmp3BC=({Cyc_Tcenv_clear_notreadctxt(te_orig);});_tmp3BB(_tmp3BC);});
({void*(*_tmp38E)(struct Cyc_Tcenv_Tenv*,void**,struct Cyc_Absyn_Exp*)=Cyc_Tcexp_tcExp;struct Cyc_Tcenv_Tenv*_tmp38F=({Cyc_Tcenv_clear_abstract_val_ok(te);});void**_tmp390=0;struct Cyc_Absyn_Exp*_tmp391=e1;_tmp38E(_tmp38F,_tmp390,_tmp391);});
({void*(*_tmp392)(struct Cyc_Tcenv_Tenv*,void**,struct Cyc_Absyn_Exp*)=Cyc_Tcexp_tcExp;struct Cyc_Tcenv_Tenv*_tmp393=({Cyc_Tcenv_clear_abstract_val_ok(te);});void**_tmp394=0;struct Cyc_Absyn_Exp*_tmp395=e2;_tmp392(_tmp393,_tmp394,_tmp395);});{
void*t1=({Cyc_Tcutil_compress((void*)_check_null(e1->topt));});
void*t2=({Cyc_Tcutil_compress((void*)_check_null(e2->topt));});
if(!({Cyc_Tcutil_coerce_sint_type(e2);}))
return({struct Cyc_String_pa_PrintArg_struct _tmp398=({struct Cyc_String_pa_PrintArg_struct _tmp6A2;_tmp6A2.tag=0U,({
struct _fat_ptr _tmp87B=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t2);}));_tmp6A2.f1=_tmp87B;});_tmp6A2;});void*_tmp396[1U];_tmp396[0]=& _tmp398;({struct Cyc_Tcenv_Tenv*_tmp87F=te;unsigned _tmp87E=e2->loc;void**_tmp87D=topt;struct _fat_ptr _tmp87C=({const char*_tmp397="expecting int subscript, found %s";_tag_fat(_tmp397,sizeof(char),34U);});Cyc_Tcexp_expr_err(_tmp87F,_tmp87E,_tmp87D,_tmp87C,_tag_fat(_tmp396,sizeof(void*),1U));});});{
# 1766
void*_tmp399=t1;struct Cyc_List_List*_tmp39A;void*_tmp39F;void*_tmp39E;void*_tmp39D;struct Cyc_Absyn_Tqual _tmp39C;void*_tmp39B;switch(*((int*)_tmp399)){case 3U: _LL1: _tmp39B=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp399)->f1).elt_type;_tmp39C=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp399)->f1).elt_tq;_tmp39D=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp399)->f1).ptr_atts).rgn;_tmp39E=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp399)->f1).ptr_atts).bounds;_tmp39F=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp399)->f1).ptr_atts).zero_term;_LL2: {void*t=_tmp39B;struct Cyc_Absyn_Tqual tq=_tmp39C;void*rt=_tmp39D;void*b=_tmp39E;void*zt=_tmp39F;
# 1775
if(({Cyc_Tcutil_force_type2bool(0,zt);})){
int emit_warning=0;
struct Cyc_Absyn_Exp*eopt=({Cyc_Tcutil_get_bounds_exp(Cyc_Absyn_fat_bound_type,b);});
if(eopt != 0){
struct Cyc_Absyn_Exp*e3=eopt;
struct _tuple17 _stmttmp31=({Cyc_Evexp_eval_const_uint_exp(e3);});int _tmp3A1;unsigned _tmp3A0;_LL8: _tmp3A0=_stmttmp31.f1;_tmp3A1=_stmttmp31.f2;_LL9: {unsigned i=_tmp3A0;int known_i=_tmp3A1;
if(known_i && i == (unsigned)1)emit_warning=1;if(({Cyc_Tcutil_is_const_exp(e2);})){
# 1783
struct _tuple17 _stmttmp32=({Cyc_Evexp_eval_const_uint_exp(e2);});int _tmp3A3;unsigned _tmp3A2;_LLB: _tmp3A2=_stmttmp32.f1;_tmp3A3=_stmttmp32.f2;_LLC: {unsigned j=_tmp3A2;int known_j=_tmp3A3;
if(known_j){
struct _tuple17 _stmttmp33=({Cyc_Evexp_eval_const_uint_exp(e3);});int _tmp3A5;unsigned _tmp3A4;_LLE: _tmp3A4=_stmttmp33.f1;_tmp3A5=_stmttmp33.f2;_LLF: {unsigned j=_tmp3A4;int knownj=_tmp3A5;
if((known_i && j > i)&& i != (unsigned)1)
({void*_tmp3A6=0U;({unsigned _tmp881=loc;struct _fat_ptr _tmp880=({const char*_tmp3A7="subscript will fail at run-time";_tag_fat(_tmp3A7,sizeof(char),32U);});Cyc_Tcutil_terr(_tmp881,_tmp880,_tag_fat(_tmp3A6,sizeof(void*),0U));});});}}}}}}
# 1778
if(emit_warning)
# 1792
({void*_tmp3A8=0U;({unsigned _tmp883=e2->loc;struct _fat_ptr _tmp882=({const char*_tmp3A9="subscript on thin, zero-terminated pointer could be expensive.";_tag_fat(_tmp3A9,sizeof(char),63U);});Cyc_Tcutil_warn(_tmp883,_tmp882,_tag_fat(_tmp3A8,sizeof(void*),0U));});});}else{
# 1795
if(({Cyc_Tcutil_is_const_exp(e2);})){
struct _tuple17 _stmttmp34=({Cyc_Evexp_eval_const_uint_exp(e2);});int _tmp3AB;unsigned _tmp3AA;_LL11: _tmp3AA=_stmttmp34.f1;_tmp3AB=_stmttmp34.f2;_LL12: {unsigned i=_tmp3AA;int known=_tmp3AB;
if(known)
# 1802
({void(*_tmp3AC)(unsigned,unsigned i,void*,int do_warn)=Cyc_Tcutil_check_bound;unsigned _tmp3AD=loc;unsigned _tmp3AE=i;void*_tmp3AF=b;int _tmp3B0=({Cyc_Tcenv_abstract_val_ok(te);});_tmp3AC(_tmp3AD,_tmp3AE,_tmp3AF,_tmp3B0);});}}else{
# 1805
if(({Cyc_Tcutil_is_bound_one(b);})&& !({Cyc_Tcutil_force_type2bool(0,zt);}))
({void*_tmp3B1=0U;({unsigned _tmp885=e1->loc;struct _fat_ptr _tmp884=({const char*_tmp3B2="subscript applied to pointer to one object";_tag_fat(_tmp3B2,sizeof(char),43U);});Cyc_Tcutil_warn(_tmp885,_tmp884,_tag_fat(_tmp3B1,sizeof(void*),0U));});});
# 1805
({Cyc_Tcutil_check_nonzero_bound(loc,b);});}}
# 1810
({Cyc_Tcenv_check_rgn_accessible(te,loc,rt);});
if(!({int(*_tmp3B3)(struct Cyc_Absyn_Kind*,struct Cyc_Absyn_Kind*)=Cyc_Tcutil_kind_leq;struct Cyc_Absyn_Kind*_tmp3B4=({Cyc_Tcutil_type_kind(t);});struct Cyc_Absyn_Kind*_tmp3B5=& Cyc_Tcutil_tmk;_tmp3B3(_tmp3B4,_tmp3B5);})&& !({Cyc_Tcenv_abstract_val_ok(te);}))
({void*_tmp3B6=0U;({unsigned _tmp887=e1->loc;struct _fat_ptr _tmp886=({const char*_tmp3B7="can't subscript an abstract pointer";_tag_fat(_tmp3B7,sizeof(char),36U);});Cyc_Tcutil_terr(_tmp887,_tmp886,_tag_fat(_tmp3B6,sizeof(void*),0U));});});
# 1811
return t;}case 6U: _LL3: _tmp39A=((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp399)->f1;_LL4: {struct Cyc_List_List*ts=_tmp39A;
# 1814
return({Cyc_Tcexp_ithTupleType(te,loc,ts,e2);});}default: _LL5: _LL6:
 return({struct Cyc_String_pa_PrintArg_struct _tmp3BA=({struct Cyc_String_pa_PrintArg_struct _tmp6A3;_tmp6A3.tag=0U,({struct _fat_ptr _tmp888=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t1);}));_tmp6A3.f1=_tmp888;});_tmp6A3;});void*_tmp3B8[1U];_tmp3B8[0]=& _tmp3BA;({struct Cyc_Tcenv_Tenv*_tmp88C=te;unsigned _tmp88B=loc;void**_tmp88A=topt;struct _fat_ptr _tmp889=({const char*_tmp3B9="subscript applied to %s";_tag_fat(_tmp3B9,sizeof(char),24U);});Cyc_Tcexp_expr_err(_tmp88C,_tmp88B,_tmp88A,_tmp889,_tag_fat(_tmp3B8,sizeof(void*),1U));});});}_LL0:;}}}
# 1820
static void*Cyc_Tcexp_tcTuple(struct Cyc_Tcenv_Tenv*te,unsigned loc,void**topt,struct Cyc_List_List*es){
int done=0;
struct Cyc_List_List*fields=0;
if(topt != 0){
void*_stmttmp35=({Cyc_Tcutil_compress(*topt);});void*_tmp3BE=_stmttmp35;struct Cyc_List_List*_tmp3BF;if(((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp3BE)->tag == 6U){_LL1: _tmp3BF=((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp3BE)->f1;_LL2: {struct Cyc_List_List*ts=_tmp3BF;
# 1826
if(({int _tmp88D=({Cyc_List_length(ts);});_tmp88D != ({Cyc_List_length(es);});}))
# 1829
goto _LL0;
# 1826
for(0;es != 0;(
# 1831
es=es->tl,ts=ts->tl)){
int bogus=0;
void*topt2=(*((struct _tuple18*)((struct Cyc_List_List*)_check_null(ts))->hd)).f2;
({void*(*_tmp3C0)(struct Cyc_Tcenv_Tenv*,void**,struct Cyc_Absyn_Exp*)=Cyc_Tcexp_tcExpInitializer;struct Cyc_Tcenv_Tenv*_tmp3C1=({Cyc_Tcenv_clear_abstract_val_ok(te);});void**_tmp3C2=& topt2;struct Cyc_Absyn_Exp*_tmp3C3=(struct Cyc_Absyn_Exp*)es->hd;_tmp3C0(_tmp3C1,_tmp3C2,_tmp3C3);});
# 1836
({Cyc_Tcutil_coerce_arg(te,(struct Cyc_Absyn_Exp*)es->hd,(*((struct _tuple18*)ts->hd)).f2,& bogus);});
fields=({struct Cyc_List_List*_tmp3C5=_cycalloc(sizeof(*_tmp3C5));({struct _tuple18*_tmp88E=({struct _tuple18*_tmp3C4=_cycalloc(sizeof(*_tmp3C4));_tmp3C4->f1=(*((struct _tuple18*)ts->hd)).f1,_tmp3C4->f2=(void*)_check_null(((struct Cyc_Absyn_Exp*)es->hd)->topt);_tmp3C4;});_tmp3C5->hd=_tmp88E;}),_tmp3C5->tl=fields;_tmp3C5;});}
# 1839
done=1;
goto _LL0;}}else{_LL3: _LL4:
 goto _LL0;}_LL0:;}
# 1823
if(!done)
# 1844
for(0;es != 0;es=es->tl){
({void*(*_tmp3C6)(struct Cyc_Tcenv_Tenv*,void**,struct Cyc_Absyn_Exp*)=Cyc_Tcexp_tcExpInitializer;struct Cyc_Tcenv_Tenv*_tmp3C7=({Cyc_Tcenv_clear_abstract_val_ok(te);});void**_tmp3C8=0;struct Cyc_Absyn_Exp*_tmp3C9=(struct Cyc_Absyn_Exp*)es->hd;_tmp3C6(_tmp3C7,_tmp3C8,_tmp3C9);});
fields=({struct Cyc_List_List*_tmp3CB=_cycalloc(sizeof(*_tmp3CB));({struct _tuple18*_tmp890=({struct _tuple18*_tmp3CA=_cycalloc(sizeof(*_tmp3CA));({struct Cyc_Absyn_Tqual _tmp88F=({Cyc_Absyn_empty_tqual(0U);});_tmp3CA->f1=_tmp88F;}),_tmp3CA->f2=(void*)_check_null(((struct Cyc_Absyn_Exp*)es->hd)->topt);_tmp3CA;});_tmp3CB->hd=_tmp890;}),_tmp3CB->tl=fields;_tmp3CB;});}
# 1823
return(void*)({struct Cyc_Absyn_TupleType_Absyn_Type_struct*_tmp3CC=_cycalloc(sizeof(*_tmp3CC));
# 1848
_tmp3CC->tag=6U,({struct Cyc_List_List*_tmp891=({Cyc_List_imp_rev(fields);});_tmp3CC->f1=_tmp891;});_tmp3CC;});}
# 1852
static void*Cyc_Tcexp_tcCompoundLit(struct Cyc_Tcenv_Tenv*te,unsigned loc,struct Cyc_Absyn_Exp*orig_exp,void**topt,struct _tuple10*targ,struct Cyc_List_List*des){
# 1857
void*_tmp3CE;_LL1: _tmp3CE=targ->f3;_LL2: {void*t=_tmp3CE;
({void(*_tmp3CF)(unsigned,struct Cyc_Tcenv_Tenv*,struct Cyc_List_List*bound_tvars,struct Cyc_Absyn_Kind*k,int allow_evars,int allow_abs_aggr,void*)=Cyc_Tctyp_check_type;unsigned _tmp3D0=loc;struct Cyc_Tcenv_Tenv*_tmp3D1=te;struct Cyc_List_List*_tmp3D2=({Cyc_Tcenv_lookup_type_vars(te);});struct Cyc_Absyn_Kind*_tmp3D3=
({Cyc_Tcenv_abstract_val_ok(te);})?& Cyc_Tcutil_tak:& Cyc_Tcutil_tmk;int _tmp3D4=1;int _tmp3D5=0;void*_tmp3D6=t;_tmp3CF(_tmp3D0,_tmp3D1,_tmp3D2,_tmp3D3,_tmp3D4,_tmp3D5,_tmp3D6);});
({void*_tmp892=(void*)({struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*_tmp3D7=_cycalloc(sizeof(*_tmp3D7));_tmp3D7->tag=37U,_tmp3D7->f1=0,_tmp3D7->f2=des;_tmp3D7;});orig_exp->r=_tmp892;});
({Cyc_Tcexp_resolve_unresolved_mem(te,loc,& t,orig_exp,des);});
({Cyc_Tcexp_tcExpNoInst(te,topt,orig_exp);});
return(void*)_check_null(orig_exp->topt);}}struct _tuple19{struct Cyc_List_List*f1;struct Cyc_Absyn_Exp*f2;};
# 1873 "tcexp.cyc"
static void*Cyc_Tcexp_tcArray(struct Cyc_Tcenv_Tenv*te,unsigned loc,void**elt_topt,struct Cyc_Absyn_Tqual*elt_tqopt,int zero_term,struct Cyc_List_List*des){
# 1876
void*res_t2;
int num_es=({Cyc_List_length(des);});
struct Cyc_List_List*es=({({struct Cyc_List_List*(*_tmp894)(struct Cyc_Absyn_Exp*(*f)(struct _tuple19*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp3F5)(struct Cyc_Absyn_Exp*(*f)(struct _tuple19*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_Exp*(*f)(struct _tuple19*),struct Cyc_List_List*x))Cyc_List_map;_tmp3F5;});struct Cyc_Absyn_Exp*(*_tmp893)(struct _tuple19*)=({struct Cyc_Absyn_Exp*(*_tmp3F6)(struct _tuple19*)=(struct Cyc_Absyn_Exp*(*)(struct _tuple19*))Cyc_Core_snd;_tmp3F6;});_tmp894(_tmp893,des);});});
void*res=({void*(*_tmp3F2)(struct Cyc_Core_Opt*k,struct Cyc_Core_Opt*tenv)=Cyc_Absyn_new_evar;struct Cyc_Core_Opt*_tmp3F3=& Cyc_Tcutil_tmko;struct Cyc_Core_Opt*_tmp3F4=({Cyc_Tcenv_lookup_opt_type_vars(te);});_tmp3F2(_tmp3F3,_tmp3F4);});
struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*sz_rexp=({struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*_tmp3F1=_cycalloc(sizeof(*_tmp3F1));_tmp3F1->tag=0U,({union Cyc_Absyn_Cnst _tmp895=({Cyc_Absyn_Int_c(Cyc_Absyn_Unsigned,num_es);});_tmp3F1->f1=_tmp895;});_tmp3F1;});
struct Cyc_Absyn_Exp*sz_exp=({Cyc_Absyn_new_exp((void*)sz_rexp,loc);});
# 1884
if(zero_term){
struct Cyc_Absyn_Exp*e=({({struct Cyc_Absyn_Exp*(*_tmp897)(struct Cyc_List_List*x,int n)=({struct Cyc_Absyn_Exp*(*_tmp3DB)(struct Cyc_List_List*x,int n)=(struct Cyc_Absyn_Exp*(*)(struct Cyc_List_List*x,int n))Cyc_List_nth;_tmp3DB;});struct Cyc_List_List*_tmp896=es;_tmp897(_tmp896,num_es - 1);});});
if(!({Cyc_Tcutil_is_zero(e);}))
({void*_tmp3D9=0U;({unsigned _tmp899=e->loc;struct _fat_ptr _tmp898=({const char*_tmp3DA="zero-terminated array doesn't end with zero.";_tag_fat(_tmp3DA,sizeof(char),45U);});Cyc_Tcutil_terr(_tmp899,_tmp898,_tag_fat(_tmp3D9,sizeof(void*),0U));});});}
# 1884
sz_exp->topt=Cyc_Absyn_uint_type;
# 1890
res_t2=({void*(*_tmp3DC)(void*elt_type,struct Cyc_Absyn_Tqual tq,struct Cyc_Absyn_Exp*num_elts,void*zero_term,unsigned ztloc)=Cyc_Absyn_array_type;void*_tmp3DD=res;struct Cyc_Absyn_Tqual _tmp3DE=
(unsigned)elt_tqopt?*elt_tqopt:({Cyc_Absyn_empty_tqual(0U);});struct Cyc_Absyn_Exp*_tmp3DF=sz_exp;void*_tmp3E0=
zero_term?Cyc_Absyn_true_type: Cyc_Absyn_false_type;unsigned _tmp3E1=0U;_tmp3DC(_tmp3DD,_tmp3DE,_tmp3DF,_tmp3E0,_tmp3E1);});
# 1894
{struct Cyc_List_List*es2=es;for(0;es2 != 0;es2=es2->tl){
({Cyc_Tcexp_tcExpInitializer(te,elt_topt,(struct Cyc_Absyn_Exp*)es2->hd);});}}
# 1897
if(!({Cyc_Tcutil_coerce_list(te,res,es);}))
# 1899
({struct Cyc_String_pa_PrintArg_struct _tmp3E4=({struct Cyc_String_pa_PrintArg_struct _tmp6A4;_tmp6A4.tag=0U,({
struct _fat_ptr _tmp89A=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(res);}));_tmp6A4.f1=_tmp89A;});_tmp6A4;});void*_tmp3E2[1U];_tmp3E2[0]=& _tmp3E4;({unsigned _tmp89C=((struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(es))->hd)->loc;struct _fat_ptr _tmp89B=({const char*_tmp3E3="elements of array do not all have the same type (%s)";_tag_fat(_tmp3E3,sizeof(char),53U);});Cyc_Tcutil_terr(_tmp89C,_tmp89B,_tag_fat(_tmp3E2,sizeof(void*),1U));});});
# 1897
{
# 1902
int offset=0;
# 1897
for(0;des != 0;(
# 1902
offset ++,des=des->tl)){
struct Cyc_List_List*ds=(*((struct _tuple19*)des->hd)).f1;
if(ds != 0){
# 1907
void*_stmttmp36=(void*)ds->hd;void*_tmp3E5=_stmttmp36;struct Cyc_Absyn_Exp*_tmp3E6;if(((struct Cyc_Absyn_FieldName_Absyn_Designator_struct*)_tmp3E5)->tag == 1U){_LL1: _LL2:
# 1909
({void*_tmp3E7=0U;({unsigned _tmp89E=loc;struct _fat_ptr _tmp89D=({const char*_tmp3E8="only array index designators are supported";_tag_fat(_tmp3E8,sizeof(char),43U);});Cyc_Tcutil_terr(_tmp89E,_tmp89D,_tag_fat(_tmp3E7,sizeof(void*),0U));});});
goto _LL0;}else{_LL3: _tmp3E6=((struct Cyc_Absyn_ArrayElement_Absyn_Designator_struct*)_tmp3E5)->f1;_LL4: {struct Cyc_Absyn_Exp*e=_tmp3E6;
# 1912
({Cyc_Tcexp_tcExpInitializer(te,0,e);});{
struct _tuple17 _stmttmp37=({Cyc_Evexp_eval_const_uint_exp(e);});int _tmp3EA;unsigned _tmp3E9;_LL6: _tmp3E9=_stmttmp37.f1;_tmp3EA=_stmttmp37.f2;_LL7: {unsigned i=_tmp3E9;int known=_tmp3EA;
if(!known)
({void*_tmp3EB=0U;({unsigned _tmp8A0=e->loc;struct _fat_ptr _tmp89F=({const char*_tmp3EC="index designator cannot use sizeof or offsetof";_tag_fat(_tmp3EC,sizeof(char),47U);});Cyc_Tcutil_terr(_tmp8A0,_tmp89F,_tag_fat(_tmp3EB,sizeof(void*),0U));});});else{
if(i != (unsigned)offset)
({struct Cyc_Int_pa_PrintArg_struct _tmp3EF=({struct Cyc_Int_pa_PrintArg_struct _tmp6A6;_tmp6A6.tag=1U,_tmp6A6.f1=(unsigned long)offset;_tmp6A6;});struct Cyc_Int_pa_PrintArg_struct _tmp3F0=({struct Cyc_Int_pa_PrintArg_struct _tmp6A5;_tmp6A5.tag=1U,_tmp6A5.f1=(unsigned long)((int)i);_tmp6A5;});void*_tmp3ED[2U];_tmp3ED[0]=& _tmp3EF,_tmp3ED[1]=& _tmp3F0;({unsigned _tmp8A2=e->loc;struct _fat_ptr _tmp8A1=({const char*_tmp3EE="expecting index designator %d but found %d";_tag_fat(_tmp3EE,sizeof(char),43U);});Cyc_Tcutil_terr(_tmp8A2,_tmp8A1,_tag_fat(_tmp3ED,sizeof(void*),2U));});});}
# 1914
goto _LL0;}}}}_LL0:;}}}
# 1923
return res_t2;}
# 1927
static void*Cyc_Tcexp_tcComprehension(struct Cyc_Tcenv_Tenv*te,unsigned loc,void**topt,struct Cyc_Absyn_Vardecl*vd,struct Cyc_Absyn_Exp*bound,struct Cyc_Absyn_Exp*body,int*is_zero_term){
# 1930
({Cyc_Tcexp_tcExp(te,0,bound);});
{void*_stmttmp38=({Cyc_Tcutil_compress((void*)_check_null(bound->topt));});void*_tmp3F8=_stmttmp38;void*_tmp3F9;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp3F8)->tag == 0U){if(((struct Cyc_Absyn_TagCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp3F8)->f1)->tag == 5U){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp3F8)->f2 != 0){_LL1: _tmp3F9=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp3F8)->f2)->hd;_LL2: {void*t=_tmp3F9;
# 1935
if((int)({Cyc_Tcenv_new_status(te);})== (int)Cyc_Tcenv_InNewAggr){
struct Cyc_Absyn_Exp*b=({struct Cyc_Absyn_Exp*(*_tmp3FA)(void*,struct Cyc_Absyn_Exp*,int user_cast,enum Cyc_Absyn_Coercion,unsigned)=Cyc_Absyn_cast_exp;void*_tmp3FB=Cyc_Absyn_uint_type;struct Cyc_Absyn_Exp*_tmp3FC=({Cyc_Absyn_valueof_exp(t,0U);});int _tmp3FD=0;enum Cyc_Absyn_Coercion _tmp3FE=Cyc_Absyn_No_coercion;unsigned _tmp3FF=0U;_tmp3FA(_tmp3FB,_tmp3FC,_tmp3FD,_tmp3FE,_tmp3FF);});
b->topt=bound->topt;
# 1940
bound=b;}
# 1935
goto _LL0;}}else{goto _LL3;}}else{goto _LL3;}}else{_LL3: _LL4:
# 1944
 if(!({Cyc_Tcutil_coerce_uint_type(bound);}))
({struct Cyc_String_pa_PrintArg_struct _tmp402=({struct Cyc_String_pa_PrintArg_struct _tmp6A7;_tmp6A7.tag=0U,({
struct _fat_ptr _tmp8A3=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string((void*)_check_null(bound->topt));}));_tmp6A7.f1=_tmp8A3;});_tmp6A7;});void*_tmp400[1U];_tmp400[0]=& _tmp402;({unsigned _tmp8A5=bound->loc;struct _fat_ptr _tmp8A4=({const char*_tmp401="expecting unsigned int, found %s";_tag_fat(_tmp401,sizeof(char),33U);});Cyc_Tcutil_terr(_tmp8A5,_tmp8A4,_tag_fat(_tmp400,sizeof(void*),1U));});});}_LL0:;}
# 1949
if(!(vd->tq).real_const)
({void*_tmp403=0U;({int(*_tmp8A7)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp405)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp405;});struct _fat_ptr _tmp8A6=({const char*_tmp404="comprehension index variable is not declared const!";_tag_fat(_tmp404,sizeof(char),52U);});_tmp8A7(_tmp8A6,_tag_fat(_tmp403,sizeof(void*),0U));});});
# 1949
if(te->le != 0)
# 1953
te=({Cyc_Tcenv_new_block(loc,te);});{
# 1949
void**topt2=0;
# 1955
struct Cyc_Absyn_Tqual*tqopt=0;
void**ztopt=0;
# 1958
if(topt != 0){
void*_stmttmp39=({Cyc_Tcutil_compress(*topt);});void*_tmp406=_stmttmp39;void*_tmp40A;struct Cyc_Absyn_Exp*_tmp409;struct Cyc_Absyn_Tqual _tmp408;void*_tmp407;struct Cyc_Absyn_PtrInfo _tmp40B;switch(*((int*)_tmp406)){case 3U: _LL6: _tmp40B=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp406)->f1;_LL7: {struct Cyc_Absyn_PtrInfo x=_tmp40B;
# 1961
topt2=({void**_tmp40C=_cycalloc(sizeof(*_tmp40C));*_tmp40C=x.elt_type;_tmp40C;});
tqopt=({struct Cyc_Absyn_Tqual*_tmp40D=_cycalloc(sizeof(*_tmp40D));*_tmp40D=x.elt_tq;_tmp40D;});
ztopt=({void**_tmp40E=_cycalloc(sizeof(*_tmp40E));*_tmp40E=(x.ptr_atts).zero_term;_tmp40E;});
goto _LL5;}case 4U: _LL8: _tmp407=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp406)->f1).elt_type;_tmp408=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp406)->f1).tq;_tmp409=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp406)->f1).num_elts;_tmp40A=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp406)->f1).zero_term;_LL9: {void*t=_tmp407;struct Cyc_Absyn_Tqual tq=_tmp408;struct Cyc_Absyn_Exp*b=_tmp409;void*zt=_tmp40A;
# 1966
topt2=({void**_tmp40F=_cycalloc(sizeof(*_tmp40F));*_tmp40F=t;_tmp40F;});
tqopt=({struct Cyc_Absyn_Tqual*_tmp410=_cycalloc(sizeof(*_tmp410));*_tmp410=tq;_tmp410;});
ztopt=({void**_tmp411=_cycalloc(sizeof(*_tmp411));*_tmp411=zt;_tmp411;});
goto _LL5;}default: _LLA: _LLB:
# 1971
 goto _LL5;}_LL5:;}{
# 1958
void*t=({Cyc_Tcexp_tcExp(te,topt2,body);});
# 1976
if(({Cyc_Tcutil_is_noalias_pointer_or_aggr(t);})&& !({Cyc_Tcutil_is_noalias_path(body);}))
({void*_tmp412=0U;({unsigned _tmp8A9=body->loc;struct _fat_ptr _tmp8A8=({const char*_tmp413="Cannot consume non-unique paths; do swap instead";_tag_fat(_tmp413,sizeof(char),49U);});Cyc_Tcutil_terr(_tmp8A9,_tmp8A8,_tag_fat(_tmp412,sizeof(void*),0U));});});
# 1976
if(te->le == 0){
# 1980
if(!({Cyc_Tcutil_is_const_exp(bound);}))
({void*_tmp414=0U;({unsigned _tmp8AB=bound->loc;struct _fat_ptr _tmp8AA=({const char*_tmp415="bound is not constant";_tag_fat(_tmp415,sizeof(char),22U);});Cyc_Tcutil_terr(_tmp8AB,_tmp8AA,_tag_fat(_tmp414,sizeof(void*),0U));});});
# 1980
if(!({Cyc_Tcutil_is_const_exp(body);}))
# 1983
({void*_tmp416=0U;({unsigned _tmp8AD=bound->loc;struct _fat_ptr _tmp8AC=({const char*_tmp417="body is not constant";_tag_fat(_tmp417,sizeof(char),21U);});Cyc_Tcutil_terr(_tmp8AD,_tmp8AC,_tag_fat(_tmp416,sizeof(void*),0U));});});}
# 1976
if(
# 1985
ztopt != 0 &&({Cyc_Tcutil_force_type2bool(0,*ztopt);})){
# 1988
struct Cyc_Absyn_Exp*e1=({Cyc_Absyn_uint_exp(1U,0U);});e1->topt=Cyc_Absyn_uint_type;
bound=({Cyc_Absyn_add_exp(bound,e1,0U);});bound->topt=Cyc_Absyn_uint_type;
*is_zero_term=1;}
# 1976
if(
# 1992
({Cyc_Tcutil_is_noalias_pointer_or_aggr((void*)_check_null(body->topt));})&& !({Cyc_Tcutil_is_noalias_path(body);}))
# 1994
({void*_tmp418=0U;({unsigned _tmp8AF=body->loc;struct _fat_ptr _tmp8AE=({const char*_tmp419="Cannot consume non-unique paths; do swap instead";_tag_fat(_tmp419,sizeof(char),49U);});Cyc_Tcutil_terr(_tmp8AF,_tmp8AE,_tag_fat(_tmp418,sizeof(void*),0U));});});{
# 1976
void*res=({void*(*_tmp41A)(void*elt_type,struct Cyc_Absyn_Tqual tq,struct Cyc_Absyn_Exp*num_elts,void*zero_term,unsigned ztloc)=Cyc_Absyn_array_type;void*_tmp41B=t;struct Cyc_Absyn_Tqual _tmp41C=
# 1996
tqopt == 0?({Cyc_Absyn_empty_tqual(0U);}):*tqopt;struct Cyc_Absyn_Exp*_tmp41D=bound;void*_tmp41E=
ztopt == 0?Cyc_Absyn_false_type:*ztopt;unsigned _tmp41F=0U;_tmp41A(_tmp41B,_tmp41C,_tmp41D,_tmp41E,_tmp41F);});
return res;}}}}
# 2002
static void*Cyc_Tcexp_tcComprehensionNoinit(struct Cyc_Tcenv_Tenv*te,unsigned loc,void**topt,struct Cyc_Absyn_Exp*bound,void*t,int*is_zero_term){
# 2005
({Cyc_Tcexp_tcExp(te,0,bound);});
{void*_stmttmp3A=({Cyc_Tcutil_compress((void*)_check_null(bound->topt));});void*_tmp421=_stmttmp3A;void*_tmp422;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp421)->tag == 0U){if(((struct Cyc_Absyn_TagCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp421)->f1)->tag == 5U){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp421)->f2 != 0){_LL1: _tmp422=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp421)->f2)->hd;_LL2: {void*t=_tmp422;
# 2010
if((int)({Cyc_Tcenv_new_status(te);})== (int)Cyc_Tcenv_InNewAggr){
struct Cyc_Absyn_Exp*b=({struct Cyc_Absyn_Exp*(*_tmp423)(void*,struct Cyc_Absyn_Exp*,int user_cast,enum Cyc_Absyn_Coercion,unsigned)=Cyc_Absyn_cast_exp;void*_tmp424=Cyc_Absyn_uint_type;struct Cyc_Absyn_Exp*_tmp425=({Cyc_Absyn_valueof_exp(t,0U);});int _tmp426=0;enum Cyc_Absyn_Coercion _tmp427=Cyc_Absyn_No_coercion;unsigned _tmp428=0U;_tmp423(_tmp424,_tmp425,_tmp426,_tmp427,_tmp428);});
b->topt=bound->topt;
# 2015
bound=b;}
# 2010
goto _LL0;}}else{goto _LL3;}}else{goto _LL3;}}else{_LL3: _LL4:
# 2019
 if(!({Cyc_Tcutil_coerce_uint_type(bound);}))
({struct Cyc_String_pa_PrintArg_struct _tmp42B=({struct Cyc_String_pa_PrintArg_struct _tmp6A8;_tmp6A8.tag=0U,({
struct _fat_ptr _tmp8B0=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string((void*)_check_null(bound->topt));}));_tmp6A8.f1=_tmp8B0;});_tmp6A8;});void*_tmp429[1U];_tmp429[0]=& _tmp42B;({unsigned _tmp8B2=bound->loc;struct _fat_ptr _tmp8B1=({const char*_tmp42A="expecting unsigned int, found %s";_tag_fat(_tmp42A,sizeof(char),33U);});Cyc_Tcutil_terr(_tmp8B2,_tmp8B1,_tag_fat(_tmp429,sizeof(void*),1U));});});}_LL0:;}{
# 2024
void**topt2=0;
struct Cyc_Absyn_Tqual*tqopt=0;
void**ztopt=0;
# 2028
if(topt != 0){
void*_stmttmp3B=({Cyc_Tcutil_compress(*topt);});void*_tmp42C=_stmttmp3B;void*_tmp430;struct Cyc_Absyn_Exp*_tmp42F;struct Cyc_Absyn_Tqual _tmp42E;void*_tmp42D;struct Cyc_Absyn_PtrInfo _tmp431;switch(*((int*)_tmp42C)){case 3U: _LL6: _tmp431=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp42C)->f1;_LL7: {struct Cyc_Absyn_PtrInfo x=_tmp431;
# 2031
topt2=({void**_tmp432=_cycalloc(sizeof(*_tmp432));*_tmp432=x.elt_type;_tmp432;});
tqopt=({struct Cyc_Absyn_Tqual*_tmp433=_cycalloc(sizeof(*_tmp433));*_tmp433=x.elt_tq;_tmp433;});
ztopt=({void**_tmp434=_cycalloc(sizeof(*_tmp434));*_tmp434=(x.ptr_atts).zero_term;_tmp434;});
goto _LL5;}case 4U: _LL8: _tmp42D=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp42C)->f1).elt_type;_tmp42E=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp42C)->f1).tq;_tmp42F=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp42C)->f1).num_elts;_tmp430=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp42C)->f1).zero_term;_LL9: {void*t=_tmp42D;struct Cyc_Absyn_Tqual tq=_tmp42E;struct Cyc_Absyn_Exp*b=_tmp42F;void*zt=_tmp430;
# 2036
topt2=({void**_tmp435=_cycalloc(sizeof(*_tmp435));*_tmp435=t;_tmp435;});
tqopt=({struct Cyc_Absyn_Tqual*_tmp436=_cycalloc(sizeof(*_tmp436));*_tmp436=tq;_tmp436;});
ztopt=({void**_tmp437=_cycalloc(sizeof(*_tmp437));*_tmp437=zt;_tmp437;});
goto _LL5;}default: _LLA: _LLB:
# 2041
 goto _LL5;}_LL5:;}
# 2028
({void(*_tmp438)(unsigned,struct Cyc_Tcenv_Tenv*,struct Cyc_List_List*bound_tvars,struct Cyc_Absyn_Kind*k,int allow_evars,int allow_abs_aggr,void*)=Cyc_Tctyp_check_type;unsigned _tmp439=loc;struct Cyc_Tcenv_Tenv*_tmp43A=te;struct Cyc_List_List*_tmp43B=({Cyc_Tcenv_lookup_type_vars(te);});struct Cyc_Absyn_Kind*_tmp43C=& Cyc_Tcutil_tmk;int _tmp43D=1;int _tmp43E=1;void*_tmp43F=t;_tmp438(_tmp439,_tmp43A,_tmp43B,_tmp43C,_tmp43D,_tmp43E,_tmp43F);});
# 2045
if(topt2 != 0)({Cyc_Unify_unify(*topt2,t);});if(te->le == 0){
# 2048
if(!({Cyc_Tcutil_is_const_exp(bound);}))
({void*_tmp440=0U;({unsigned _tmp8B4=bound->loc;struct _fat_ptr _tmp8B3=({const char*_tmp441="bound is not constant";_tag_fat(_tmp441,sizeof(char),22U);});Cyc_Tcutil_terr(_tmp8B4,_tmp8B3,_tag_fat(_tmp440,sizeof(void*),0U));});});}
# 2045
if(
# 2051
ztopt != 0 &&({Cyc_Tcutil_force_type2bool(0,*ztopt);})){
# 2054
struct Cyc_Absyn_Exp*e1=({Cyc_Absyn_uint_exp(1U,0U);});e1->topt=Cyc_Absyn_uint_type;
bound=({Cyc_Absyn_add_exp(bound,e1,0U);});bound->topt=Cyc_Absyn_uint_type;
*is_zero_term=1;
# 2058
({void*_tmp442=0U;({unsigned _tmp8B6=loc;struct _fat_ptr _tmp8B5=({const char*_tmp443="non-initializing comprehensions do not currently support @zeroterm arrays";_tag_fat(_tmp443,sizeof(char),74U);});Cyc_Tcutil_terr(_tmp8B6,_tmp8B5,_tag_fat(_tmp442,sizeof(void*),0U));});});}{
# 2045
void*res=({void*(*_tmp444)(void*elt_type,struct Cyc_Absyn_Tqual tq,struct Cyc_Absyn_Exp*num_elts,void*zero_term,unsigned ztloc)=Cyc_Absyn_array_type;void*_tmp445=t;struct Cyc_Absyn_Tqual _tmp446=
# 2061
tqopt == 0?({Cyc_Absyn_empty_tqual(0U);}):*tqopt;struct Cyc_Absyn_Exp*_tmp447=bound;void*_tmp448=
ztopt == 0?Cyc_Absyn_false_type:*ztopt;unsigned _tmp449=0U;_tmp444(_tmp445,_tmp446,_tmp447,_tmp448,_tmp449);});
return res;}}}struct _tuple20{struct Cyc_Absyn_Aggrfield*f1;struct Cyc_Absyn_Exp*f2;};
# 2076 "tcexp.cyc"
static void*Cyc_Tcexp_tcAggregate(struct Cyc_Tcenv_Tenv*te,unsigned loc,void**topt,struct _tuple1**tn,struct Cyc_List_List**ts,struct Cyc_List_List*args,struct Cyc_Absyn_Aggrdecl**ad_opt){
# 2080
struct Cyc_Absyn_Aggrdecl**adptr;
struct Cyc_Absyn_Aggrdecl*ad;
if(*ad_opt != 0){
ad=(struct Cyc_Absyn_Aggrdecl*)_check_null(*ad_opt);
adptr=({struct Cyc_Absyn_Aggrdecl**_tmp44B=_cycalloc(sizeof(*_tmp44B));*_tmp44B=ad;_tmp44B;});}else{
# 2086
{struct _handler_cons _tmp44C;_push_handler(& _tmp44C);{int _tmp44E=0;if(setjmp(_tmp44C.handler))_tmp44E=1;if(!_tmp44E){adptr=({Cyc_Tcenv_lookup_aggrdecl(te,loc,*tn);});
ad=*adptr;
# 2086
;_pop_handler();}else{void*_tmp44D=(void*)Cyc_Core_get_exn_thrown();void*_tmp44F=_tmp44D;void*_tmp450;if(((struct Cyc_Dict_Absent_exn_struct*)_tmp44F)->tag == Cyc_Dict_Absent){_LL1: _LL2:
# 2089
({struct Cyc_String_pa_PrintArg_struct _tmp453=({struct Cyc_String_pa_PrintArg_struct _tmp6A9;_tmp6A9.tag=0U,({struct _fat_ptr _tmp8B7=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(*tn);}));_tmp6A9.f1=_tmp8B7;});_tmp6A9;});void*_tmp451[1U];_tmp451[0]=& _tmp453;({unsigned _tmp8B9=loc;struct _fat_ptr _tmp8B8=({const char*_tmp452="unbound struct/union name %s";_tag_fat(_tmp452,sizeof(char),29U);});Cyc_Tcutil_terr(_tmp8B9,_tmp8B8,_tag_fat(_tmp451,sizeof(void*),1U));});});
return topt != 0?*topt: Cyc_Absyn_void_type;}else{_LL3: _tmp450=_tmp44F;_LL4: {void*exn=_tmp450;(int)_rethrow(exn);}}_LL0:;}}}
# 2092
*ad_opt=ad;
*tn=ad->name;}
# 2095
if(ad->impl == 0){
({struct Cyc_String_pa_PrintArg_struct _tmp456=({struct Cyc_String_pa_PrintArg_struct _tmp6AA;_tmp6AA.tag=0U,({struct _fat_ptr _tmp8BA=(struct _fat_ptr)((int)ad->kind == (int)Cyc_Absyn_StructA?({const char*_tmp457="struct";_tag_fat(_tmp457,sizeof(char),7U);}):({const char*_tmp458="union";_tag_fat(_tmp458,sizeof(char),6U);}));_tmp6AA.f1=_tmp8BA;});_tmp6AA;});void*_tmp454[1U];_tmp454[0]=& _tmp456;({unsigned _tmp8BC=loc;struct _fat_ptr _tmp8BB=({const char*_tmp455="can't construct abstract %s";_tag_fat(_tmp455,sizeof(char),28U);});Cyc_Tcutil_terr(_tmp8BC,_tmp8BB,_tag_fat(_tmp454,sizeof(void*),1U));});});
return({void*(*_tmp459)(struct Cyc_Core_Opt*)=Cyc_Absyn_wildtyp;struct Cyc_Core_Opt*_tmp45A=({Cyc_Tcenv_lookup_opt_type_vars(te);});_tmp459(_tmp45A);});}{
# 2095
struct Cyc_Tcenv_Tenv*te2;
# 2104
enum Cyc_Tcenv_NewStatus status=({Cyc_Tcenv_new_status(te);});
if((int)status == (int)1U)
te2=({Cyc_Tcenv_set_new_status(Cyc_Tcenv_InNewAggr,te);});else{
# 2112
te2=({Cyc_Tcenv_set_new_status(status,te);});}{
# 2114
struct _tuple14 env=({struct _tuple14 _tmp6B0;({struct Cyc_List_List*_tmp8BD=({Cyc_Tcenv_lookup_type_vars(te2);});_tmp6B0.f1=_tmp8BD;}),_tmp6B0.f2=Cyc_Core_heap_region;_tmp6B0;});
struct Cyc_List_List*all_inst=({({struct Cyc_List_List*(*_tmp8BE)(struct _tuple15*(*f)(struct _tuple14*,struct Cyc_Absyn_Tvar*),struct _tuple14*env,struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp484)(struct _tuple15*(*f)(struct _tuple14*,struct Cyc_Absyn_Tvar*),struct _tuple14*env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct _tuple15*(*f)(struct _tuple14*,struct Cyc_Absyn_Tvar*),struct _tuple14*env,struct Cyc_List_List*x))Cyc_List_map_c;_tmp484;});_tmp8BE(Cyc_Tcutil_r_make_inst_var,& env,ad->tvs);});});
struct Cyc_List_List*exist_inst=({({struct Cyc_List_List*(*_tmp8BF)(struct _tuple15*(*f)(struct _tuple14*,struct Cyc_Absyn_Tvar*),struct _tuple14*env,struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp483)(struct _tuple15*(*f)(struct _tuple14*,struct Cyc_Absyn_Tvar*),struct _tuple14*env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct _tuple15*(*f)(struct _tuple14*,struct Cyc_Absyn_Tvar*),struct _tuple14*env,struct Cyc_List_List*x))Cyc_List_map_c;_tmp483;});_tmp8BF(Cyc_Tcutil_r_make_inst_var,& env,((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->exist_vars);});});
struct Cyc_List_List*all_typs=({({struct Cyc_List_List*(*_tmp8C1)(void*(*f)(struct _tuple15*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp481)(void*(*f)(struct _tuple15*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(void*(*f)(struct _tuple15*),struct Cyc_List_List*x))Cyc_List_map;_tmp481;});void*(*_tmp8C0)(struct _tuple15*)=({void*(*_tmp482)(struct _tuple15*)=(void*(*)(struct _tuple15*))Cyc_Core_snd;_tmp482;});_tmp8C1(_tmp8C0,all_inst);});});
struct Cyc_List_List*exist_typs=({({struct Cyc_List_List*(*_tmp8C3)(void*(*f)(struct _tuple15*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp47F)(void*(*f)(struct _tuple15*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(void*(*f)(struct _tuple15*),struct Cyc_List_List*x))Cyc_List_map;_tmp47F;});void*(*_tmp8C2)(struct _tuple15*)=({void*(*_tmp480)(struct _tuple15*)=(void*(*)(struct _tuple15*))Cyc_Core_snd;_tmp480;});_tmp8C3(_tmp8C2,exist_inst);});});
struct Cyc_List_List*inst=({Cyc_List_append(all_inst,exist_inst);});
void*res_typ;
# 2125
if(topt != 0){
void*_stmttmp3C=({Cyc_Tcutil_compress(*topt);});void*_tmp45B=_stmttmp3C;struct Cyc_List_List*_tmp45D;struct Cyc_Absyn_Aggrdecl**_tmp45C;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp45B)->tag == 0U){if(((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp45B)->f1)->tag == 22U){if(((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp45B)->f1)->f1).KnownAggr).tag == 2){_LL6: _tmp45C=((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp45B)->f1)->f1).KnownAggr).val;_tmp45D=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp45B)->f2;_LL7: {struct Cyc_Absyn_Aggrdecl**adptr2=_tmp45C;struct Cyc_List_List*all_typs2=_tmp45D;
# 2128
if(*adptr2 == *adptr){
{struct Cyc_List_List*ats=all_typs;for(0;ats != 0 && all_typs2 != 0;(
ats=ats->tl,all_typs2=all_typs2->tl)){
({Cyc_Unify_unify((void*)ats->hd,(void*)all_typs2->hd);});}}
# 2133
res_typ=*topt;
goto _LL5;}
# 2128
goto _LL9;}}else{goto _LL8;}}else{goto _LL8;}}else{_LL8: _LL9:
# 2138
 res_typ=({void*(*_tmp45E)(union Cyc_Absyn_AggrInfo,struct Cyc_List_List*args)=Cyc_Absyn_aggr_type;union Cyc_Absyn_AggrInfo _tmp45F=({Cyc_Absyn_KnownAggr(adptr);});struct Cyc_List_List*_tmp460=all_typs;_tmp45E(_tmp45F,_tmp460);});}_LL5:;}else{
# 2141
res_typ=({void*(*_tmp461)(union Cyc_Absyn_AggrInfo,struct Cyc_List_List*args)=Cyc_Absyn_aggr_type;union Cyc_Absyn_AggrInfo _tmp462=({Cyc_Absyn_KnownAggr(adptr);});struct Cyc_List_List*_tmp463=all_typs;_tmp461(_tmp462,_tmp463);});}{
# 2144
struct Cyc_List_List*user_ex_ts=*ts;
struct Cyc_List_List*ex_ts=exist_typs;
while(user_ex_ts != 0 && ex_ts != 0){
# 2148
({void(*_tmp464)(unsigned,struct Cyc_Tcenv_Tenv*,struct Cyc_List_List*bound_tvars,struct Cyc_Absyn_Kind*k,int allow_evars,int allow_abs_aggr,void*)=Cyc_Tctyp_check_type;unsigned _tmp465=loc;struct Cyc_Tcenv_Tenv*_tmp466=te2;struct Cyc_List_List*_tmp467=({Cyc_Tcenv_lookup_type_vars(te2);});struct Cyc_Absyn_Kind*_tmp468=& Cyc_Tcutil_ak;int _tmp469=1;int _tmp46A=0;void*_tmp46B=(void*)user_ex_ts->hd;_tmp464(_tmp465,_tmp466,_tmp467,_tmp468,_tmp469,_tmp46A,_tmp46B);});
({Cyc_Tcutil_check_no_qual(loc,(void*)user_ex_ts->hd);});
({Cyc_Unify_unify((void*)user_ex_ts->hd,(void*)ex_ts->hd);});
user_ex_ts=user_ex_ts->tl;
ex_ts=ex_ts->tl;}
# 2154
if(user_ex_ts != 0)
({void*_tmp46C=0U;({unsigned _tmp8C5=loc;struct _fat_ptr _tmp8C4=({const char*_tmp46D="too many explicit witness types";_tag_fat(_tmp46D,sizeof(char),32U);});Cyc_Tcutil_terr(_tmp8C5,_tmp8C4,_tag_fat(_tmp46C,sizeof(void*),0U));});});
# 2154
*ts=exist_typs;{
# 2160
struct Cyc_List_List*fields=({Cyc_Tcutil_resolve_aggregate_designators(Cyc_Core_heap_region,loc,args,ad->kind,((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->fields);});
# 2162
for(0;fields != 0;fields=fields->tl){
int bogus=0;
struct _tuple20*_stmttmp3D=(struct _tuple20*)fields->hd;struct Cyc_Absyn_Exp*_tmp46F;struct Cyc_Absyn_Aggrfield*_tmp46E;_LLB: _tmp46E=_stmttmp3D->f1;_tmp46F=_stmttmp3D->f2;_LLC: {struct Cyc_Absyn_Aggrfield*field=_tmp46E;struct Cyc_Absyn_Exp*field_exp=_tmp46F;
void*inst_fieldtyp=({Cyc_Tcutil_substitute(inst,field->type);});
({void*(*_tmp470)(struct Cyc_Tcenv_Tenv*,void**,struct Cyc_Absyn_Exp*)=Cyc_Tcexp_tcExpInitializer;struct Cyc_Tcenv_Tenv*_tmp471=({Cyc_Tcenv_clear_abstract_val_ok(te2);});void**_tmp472=& inst_fieldtyp;struct Cyc_Absyn_Exp*_tmp473=field_exp;_tmp470(_tmp471,_tmp472,_tmp473);});
# 2171
if(!({Cyc_Tcutil_coerce_arg(te2,field_exp,inst_fieldtyp,& bogus);})){
({struct Cyc_String_pa_PrintArg_struct _tmp476=({struct Cyc_String_pa_PrintArg_struct _tmp6AF;_tmp6AF.tag=0U,_tmp6AF.f1=(struct _fat_ptr)((struct _fat_ptr)*field->name);_tmp6AF;});struct Cyc_String_pa_PrintArg_struct _tmp477=({struct Cyc_String_pa_PrintArg_struct _tmp6AE;_tmp6AE.tag=0U,({
struct _fat_ptr _tmp8C6=(struct _fat_ptr)((int)ad->kind == (int)Cyc_Absyn_StructA?({const char*_tmp47B="struct";_tag_fat(_tmp47B,sizeof(char),7U);}):({const char*_tmp47C="union";_tag_fat(_tmp47C,sizeof(char),6U);}));_tmp6AE.f1=_tmp8C6;});_tmp6AE;});struct Cyc_String_pa_PrintArg_struct _tmp478=({struct Cyc_String_pa_PrintArg_struct _tmp6AD;_tmp6AD.tag=0U,({
struct _fat_ptr _tmp8C7=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(*tn);}));_tmp6AD.f1=_tmp8C7;});_tmp6AD;});struct Cyc_String_pa_PrintArg_struct _tmp479=({struct Cyc_String_pa_PrintArg_struct _tmp6AC;_tmp6AC.tag=0U,({struct _fat_ptr _tmp8C8=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(inst_fieldtyp);}));_tmp6AC.f1=_tmp8C8;});_tmp6AC;});struct Cyc_String_pa_PrintArg_struct _tmp47A=({struct Cyc_String_pa_PrintArg_struct _tmp6AB;_tmp6AB.tag=0U,({
struct _fat_ptr _tmp8C9=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string((void*)_check_null(field_exp->topt));}));_tmp6AB.f1=_tmp8C9;});_tmp6AB;});void*_tmp474[5U];_tmp474[0]=& _tmp476,_tmp474[1]=& _tmp477,_tmp474[2]=& _tmp478,_tmp474[3]=& _tmp479,_tmp474[4]=& _tmp47A;({unsigned _tmp8CB=field_exp->loc;struct _fat_ptr _tmp8CA=({const char*_tmp475="field %s of %s %s expects type %s != %s";_tag_fat(_tmp475,sizeof(char),40U);});Cyc_Tcutil_terr(_tmp8CB,_tmp8CA,_tag_fat(_tmp474,sizeof(void*),5U));});});
({Cyc_Unify_explain_failure();});}}}{
# 2182
struct Cyc_List_List*rpo_inst=0;
{struct Cyc_List_List*rpo=((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->rgn_po;for(0;rpo != 0;rpo=rpo->tl){
rpo_inst=({struct Cyc_List_List*_tmp47E=_cycalloc(sizeof(*_tmp47E));({struct _tuple0*_tmp8CE=({struct _tuple0*_tmp47D=_cycalloc(sizeof(*_tmp47D));({void*_tmp8CD=({Cyc_Tcutil_substitute(inst,(*((struct _tuple0*)rpo->hd)).f1);});_tmp47D->f1=_tmp8CD;}),({
void*_tmp8CC=({Cyc_Tcutil_substitute(inst,(*((struct _tuple0*)rpo->hd)).f2);});_tmp47D->f2=_tmp8CC;});_tmp47D;});
# 2184
_tmp47E->hd=_tmp8CE;}),_tmp47E->tl=rpo_inst;_tmp47E;});}}
# 2187
rpo_inst=({Cyc_List_imp_rev(rpo_inst);});
({Cyc_Tcenv_check_rgn_partial_order(te2,loc,rpo_inst);});
return res_typ;}}}}}}
# 2193
static void*Cyc_Tcexp_tcAnonStruct(struct Cyc_Tcenv_Tenv*te,unsigned loc,void*ts,struct Cyc_List_List*args){
# 2195
{void*_stmttmp3E=({Cyc_Tcutil_compress(ts);});void*_tmp486=_stmttmp3E;struct Cyc_List_List*_tmp488;enum Cyc_Absyn_AggrKind _tmp487;if(((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp486)->tag == 7U){_LL1: _tmp487=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp486)->f1;_tmp488=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp486)->f2;_LL2: {enum Cyc_Absyn_AggrKind k=_tmp487;struct Cyc_List_List*fs=_tmp488;
# 2197
if((int)k == (int)1U)
({void*_tmp489=0U;({unsigned _tmp8D0=loc;struct _fat_ptr _tmp8CF=({const char*_tmp48A="expecting struct but found union";_tag_fat(_tmp48A,sizeof(char),33U);});Cyc_Tcutil_terr(_tmp8D0,_tmp8CF,_tag_fat(_tmp489,sizeof(void*),0U));});});{
# 2197
struct Cyc_List_List*fields=({Cyc_Tcutil_resolve_aggregate_designators(Cyc_Core_heap_region,loc,args,Cyc_Absyn_StructA,fs);});
# 2201
for(0;fields != 0;fields=fields->tl){
int bogus=0;
struct _tuple20*_stmttmp3F=(struct _tuple20*)fields->hd;struct Cyc_Absyn_Exp*_tmp48C;struct Cyc_Absyn_Aggrfield*_tmp48B;_LL6: _tmp48B=_stmttmp3F->f1;_tmp48C=_stmttmp3F->f2;_LL7: {struct Cyc_Absyn_Aggrfield*field=_tmp48B;struct Cyc_Absyn_Exp*field_exp=_tmp48C;
({void*(*_tmp48D)(struct Cyc_Tcenv_Tenv*,void**,struct Cyc_Absyn_Exp*)=Cyc_Tcexp_tcExpInitializer;struct Cyc_Tcenv_Tenv*_tmp48E=({Cyc_Tcenv_clear_abstract_val_ok(te);});void**_tmp48F=& field->type;struct Cyc_Absyn_Exp*_tmp490=field_exp;_tmp48D(_tmp48E,_tmp48F,_tmp490);});
# 2206
if(!({Cyc_Tcutil_coerce_arg(te,field_exp,field->type,& bogus);})){
({struct Cyc_String_pa_PrintArg_struct _tmp493=({struct Cyc_String_pa_PrintArg_struct _tmp6B3;_tmp6B3.tag=0U,_tmp6B3.f1=(struct _fat_ptr)((struct _fat_ptr)*field->name);_tmp6B3;});struct Cyc_String_pa_PrintArg_struct _tmp494=({struct Cyc_String_pa_PrintArg_struct _tmp6B2;_tmp6B2.tag=0U,({
struct _fat_ptr _tmp8D1=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(field->type);}));_tmp6B2.f1=_tmp8D1;});_tmp6B2;});struct Cyc_String_pa_PrintArg_struct _tmp495=({struct Cyc_String_pa_PrintArg_struct _tmp6B1;_tmp6B1.tag=0U,({
struct _fat_ptr _tmp8D2=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string((void*)_check_null(field_exp->topt));}));_tmp6B1.f1=_tmp8D2;});_tmp6B1;});void*_tmp491[3U];_tmp491[0]=& _tmp493,_tmp491[1]=& _tmp494,_tmp491[2]=& _tmp495;({unsigned _tmp8D4=field_exp->loc;struct _fat_ptr _tmp8D3=({const char*_tmp492="field %s of struct expects type %s != %s";_tag_fat(_tmp492,sizeof(char),41U);});Cyc_Tcutil_terr(_tmp8D4,_tmp8D3,_tag_fat(_tmp491,sizeof(void*),3U));});});
({Cyc_Unify_explain_failure();});}}}
# 2213
goto _LL0;}}}else{_LL3: _LL4:
({void*_tmp496=0U;({int(*_tmp8D6)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp498)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp498;});struct _fat_ptr _tmp8D5=({const char*_tmp497="tcAnonStruct: wrong type";_tag_fat(_tmp497,sizeof(char),25U);});_tmp8D6(_tmp8D5,_tag_fat(_tmp496,sizeof(void*),0U));});});}_LL0:;}
# 2216
return ts;}
# 2220
static void*Cyc_Tcexp_tcDatatype(struct Cyc_Tcenv_Tenv*te,unsigned loc,void**topt,struct Cyc_Absyn_Exp*e,struct Cyc_List_List*es,struct Cyc_Absyn_Datatypedecl*tud,struct Cyc_Absyn_Datatypefield*tuf){
# 2223
struct _tuple14 env=({struct _tuple14 _tmp6B9;({struct Cyc_List_List*_tmp8D7=({Cyc_Tcenv_lookup_type_vars(te);});_tmp6B9.f1=_tmp8D7;}),_tmp6B9.f2=Cyc_Core_heap_region;_tmp6B9;});
struct Cyc_List_List*inst=({({struct Cyc_List_List*(*_tmp8D8)(struct _tuple15*(*f)(struct _tuple14*,struct Cyc_Absyn_Tvar*),struct _tuple14*env,struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp4AC)(struct _tuple15*(*f)(struct _tuple14*,struct Cyc_Absyn_Tvar*),struct _tuple14*env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct _tuple15*(*f)(struct _tuple14*,struct Cyc_Absyn_Tvar*),struct _tuple14*env,struct Cyc_List_List*x))Cyc_List_map_c;_tmp4AC;});_tmp8D8(Cyc_Tcutil_r_make_inst_var,& env,tud->tvs);});});
struct Cyc_List_List*all_typs=({({struct Cyc_List_List*(*_tmp8DA)(void*(*f)(struct _tuple15*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp4AA)(void*(*f)(struct _tuple15*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(void*(*f)(struct _tuple15*),struct Cyc_List_List*x))Cyc_List_map;_tmp4AA;});void*(*_tmp8D9)(struct _tuple15*)=({void*(*_tmp4AB)(struct _tuple15*)=(void*(*)(struct _tuple15*))Cyc_Core_snd;_tmp4AB;});_tmp8DA(_tmp8D9,inst);});});
void*res=({void*(*_tmp4A7)(union Cyc_Absyn_DatatypeFieldInfo,struct Cyc_List_List*args)=Cyc_Absyn_datatype_field_type;union Cyc_Absyn_DatatypeFieldInfo _tmp4A8=({Cyc_Absyn_KnownDatatypefield(tud,tuf);});struct Cyc_List_List*_tmp4A9=all_typs;_tmp4A7(_tmp4A8,_tmp4A9);});
# 2228
if(topt != 0){
void*_stmttmp40=({Cyc_Tcutil_compress(*topt);});void*_tmp49A=_stmttmp40;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp49A)->tag == 0U){if(((struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp49A)->f1)->tag == 21U){_LL1: _LL2:
({Cyc_Unify_unify(*topt,res);});goto _LL0;}else{goto _LL3;}}else{_LL3: _LL4:
 goto _LL0;}_LL0:;}{
# 2228
struct Cyc_List_List*ts=tuf->typs;
# 2235
for(0;es != 0 && ts != 0;(es=es->tl,ts=ts->tl)){
int bogus=0;
struct Cyc_Absyn_Exp*e=(struct Cyc_Absyn_Exp*)es->hd;
void*t=(*((struct _tuple18*)ts->hd)).f2;
if(inst != 0)t=({Cyc_Tcutil_substitute(inst,t);});({Cyc_Tcexp_tcExpInitializer(te,& t,e);});
# 2241
if(!({Cyc_Tcutil_coerce_arg(te,e,t,& bogus);})){
({struct Cyc_String_pa_PrintArg_struct _tmp49D=({struct Cyc_String_pa_PrintArg_struct _tmp6B6;_tmp6B6.tag=0U,({
# 2244
struct _fat_ptr _tmp8DB=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(tuf->name);}));_tmp6B6.f1=_tmp8DB;});_tmp6B6;});struct Cyc_String_pa_PrintArg_struct _tmp49E=({struct Cyc_String_pa_PrintArg_struct _tmp6B5;_tmp6B5.tag=0U,({struct _fat_ptr _tmp8DC=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t);}));_tmp6B5.f1=_tmp8DC;});_tmp6B5;});struct Cyc_String_pa_PrintArg_struct _tmp49F=({struct Cyc_String_pa_PrintArg_struct _tmp6B4;_tmp6B4.tag=0U,({
struct _fat_ptr _tmp8DD=(struct _fat_ptr)((struct _fat_ptr)(e->topt == 0?(struct _fat_ptr)({const char*_tmp4A0="?";_tag_fat(_tmp4A0,sizeof(char),2U);}):({Cyc_Absynpp_typ2string((void*)_check_null(e->topt));})));_tmp6B4.f1=_tmp8DD;});_tmp6B4;});void*_tmp49B[3U];_tmp49B[0]=& _tmp49D,_tmp49B[1]=& _tmp49E,_tmp49B[2]=& _tmp49F;({unsigned _tmp8DF=e->loc;struct _fat_ptr _tmp8DE=({const char*_tmp49C="datatype constructor %s expects argument of type %s but this argument has type %s";_tag_fat(_tmp49C,sizeof(char),82U);});Cyc_Tcutil_terr(_tmp8DF,_tmp8DE,_tag_fat(_tmp49B,sizeof(void*),3U));});});
({Cyc_Unify_explain_failure();});}}
# 2249
if(es != 0)
return({struct Cyc_String_pa_PrintArg_struct _tmp4A3=({struct Cyc_String_pa_PrintArg_struct _tmp6B7;_tmp6B7.tag=0U,({
# 2252
struct _fat_ptr _tmp8E0=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(tuf->name);}));_tmp6B7.f1=_tmp8E0;});_tmp6B7;});void*_tmp4A1[1U];_tmp4A1[0]=& _tmp4A3;({struct Cyc_Tcenv_Tenv*_tmp8E4=te;unsigned _tmp8E3=((struct Cyc_Absyn_Exp*)es->hd)->loc;void**_tmp8E2=topt;struct _fat_ptr _tmp8E1=({const char*_tmp4A2="too many arguments for datatype constructor %s";_tag_fat(_tmp4A2,sizeof(char),47U);});Cyc_Tcexp_expr_err(_tmp8E4,_tmp8E3,_tmp8E2,_tmp8E1,_tag_fat(_tmp4A1,sizeof(void*),1U));});});
# 2249
if(ts != 0)
# 2254
return({struct Cyc_String_pa_PrintArg_struct _tmp4A6=({struct Cyc_String_pa_PrintArg_struct _tmp6B8;_tmp6B8.tag=0U,({
struct _fat_ptr _tmp8E5=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(tuf->name);}));_tmp6B8.f1=_tmp8E5;});_tmp6B8;});void*_tmp4A4[1U];_tmp4A4[0]=& _tmp4A6;({struct Cyc_Tcenv_Tenv*_tmp8E9=te;unsigned _tmp8E8=loc;void**_tmp8E7=topt;struct _fat_ptr _tmp8E6=({const char*_tmp4A5="too few arguments for datatype constructor %s";_tag_fat(_tmp4A5,sizeof(char),46U);});Cyc_Tcexp_expr_err(_tmp8E9,_tmp8E8,_tmp8E7,_tmp8E6,_tag_fat(_tmp4A4,sizeof(void*),1U));});});
# 2249
return res;}}
# 2260
static int Cyc_Tcexp_check_malloc_type(int allow_zero,unsigned loc,void**topt,void*t){
# 2262
if(({Cyc_Tcutil_is_bits_only_type(t);})|| allow_zero &&({Cyc_Tcutil_zeroable_type(t);}))return 1;if(topt != 0){
# 2265
void*_stmttmp41=({Cyc_Tcutil_compress(*topt);});void*_tmp4AE=_stmttmp41;void*_tmp4AF;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp4AE)->tag == 3U){_LL1: _tmp4AF=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp4AE)->f1).elt_type;_LL2: {void*elt_typ=_tmp4AF;
# 2267
({Cyc_Unify_unify(elt_typ,t);});
if(({Cyc_Tcutil_is_bits_only_type(t);})|| allow_zero &&({Cyc_Tcutil_zeroable_type(t);}))return 1;goto _LL0;}}else{_LL3: _LL4:
# 2270
 goto _LL0;}_LL0:;}
# 2262
return 0;}
# 2280
static void*Cyc_Tcexp_mallocRgn(void*rgn){
# 2282
enum Cyc_Absyn_AliasQual _stmttmp42=({struct Cyc_Absyn_Kind*(*_tmp4B2)(void*)=Cyc_Tcutil_type_kind;void*_tmp4B3=({Cyc_Tcutil_compress(rgn);});_tmp4B2(_tmp4B3);})->aliasqual;enum Cyc_Absyn_AliasQual _tmp4B1=_stmttmp42;if(_tmp4B1 == Cyc_Absyn_Unique){_LL1: _LL2:
 return Cyc_Absyn_unique_rgn_type;}else{_LL3: _LL4:
 return Cyc_Absyn_heap_rgn_type;}_LL0:;}
# 2288
static void*Cyc_Tcexp_tcMalloc(struct Cyc_Tcenv_Tenv*te,unsigned loc,void**topt,struct Cyc_Absyn_Exp**ropt,void***t,struct Cyc_Absyn_Exp**e,int*is_calloc,int*is_fat){
# 2292
void*rgn=Cyc_Absyn_heap_rgn_type;
if(*ropt != 0){
# 2296
void*expected_type=({void*(*_tmp4BA)(void*)=Cyc_Absyn_rgn_handle_type;void*_tmp4BB=({void*(*_tmp4BC)(struct Cyc_Core_Opt*k,struct Cyc_Core_Opt*tenv)=Cyc_Absyn_new_evar;struct Cyc_Core_Opt*_tmp4BD=& Cyc_Tcutil_trko;struct Cyc_Core_Opt*_tmp4BE=({Cyc_Tcenv_lookup_opt_type_vars(te);});_tmp4BC(_tmp4BD,_tmp4BE);});_tmp4BA(_tmp4BB);});
# 2298
void*handle_type=({Cyc_Tcexp_tcExp(te,& expected_type,(struct Cyc_Absyn_Exp*)_check_null(*ropt));});
void*_stmttmp43=({Cyc_Tcutil_compress(handle_type);});void*_tmp4B5=_stmttmp43;void*_tmp4B6;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp4B5)->tag == 0U){if(((struct Cyc_Absyn_RgnHandleCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp4B5)->f1)->tag == 3U){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp4B5)->f2 != 0){_LL1: _tmp4B6=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp4B5)->f2)->hd;_LL2: {void*r=_tmp4B6;
# 2301
rgn=r;
({Cyc_Tcenv_check_rgn_accessible(te,loc,rgn);});
goto _LL0;}}else{goto _LL3;}}else{goto _LL3;}}else{_LL3: _LL4:
# 2305
({struct Cyc_String_pa_PrintArg_struct _tmp4B9=({struct Cyc_String_pa_PrintArg_struct _tmp6BA;_tmp6BA.tag=0U,({
struct _fat_ptr _tmp8EA=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(handle_type);}));_tmp6BA.f1=_tmp8EA;});_tmp6BA;});void*_tmp4B7[1U];_tmp4B7[0]=& _tmp4B9;({unsigned _tmp8EC=((struct Cyc_Absyn_Exp*)_check_null(*ropt))->loc;struct _fat_ptr _tmp8EB=({const char*_tmp4B8="expecting region_t type but found %s";_tag_fat(_tmp4B8,sizeof(char),37U);});Cyc_Tcutil_terr(_tmp8EC,_tmp8EB,_tag_fat(_tmp4B7,sizeof(void*),1U));});});
goto _LL0;}_LL0:;}else{
# 2312
if(topt != 0){
void*optrgn=Cyc_Absyn_void_type;
if(({Cyc_Tcutil_rgn_of_pointer(*topt,& optrgn);})){
rgn=({Cyc_Tcexp_mallocRgn(optrgn);});
if(rgn == Cyc_Absyn_unique_rgn_type)({struct Cyc_Absyn_Exp*_tmp8ED=({Cyc_Absyn_uniquergn_exp();});*ropt=_tmp8ED;});}}}
# 2293
({void*(*_tmp4BF)(struct Cyc_Tcenv_Tenv*,void**,struct Cyc_Absyn_Exp*)=Cyc_Tcexp_tcExp;struct Cyc_Tcenv_Tenv*_tmp4C0=({Cyc_Tcenv_clear_abstract_val_ok(te);});void**_tmp4C1=& Cyc_Absyn_uint_type;struct Cyc_Absyn_Exp*_tmp4C2=*e;_tmp4BF(_tmp4C0,_tmp4C1,_tmp4C2);});{
# 2328 "tcexp.cyc"
void*elt_type;
struct Cyc_Absyn_Exp*num_elts;
int one_elt;
if(*is_calloc){
if(*t == 0)({void*_tmp4C3=0U;({int(*_tmp8EF)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp4C5)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp4C5;});struct _fat_ptr _tmp8EE=({const char*_tmp4C4="calloc with empty type";_tag_fat(_tmp4C4,sizeof(char),23U);});_tmp8EF(_tmp8EE,_tag_fat(_tmp4C3,sizeof(void*),0U));});});elt_type=*((void**)_check_null(*t));
# 2334
({void(*_tmp4C6)(unsigned,struct Cyc_Tcenv_Tenv*,struct Cyc_List_List*bound_tvars,struct Cyc_Absyn_Kind*k,int allow_evars,int allow_abs_aggr,void*)=Cyc_Tctyp_check_type;unsigned _tmp4C7=loc;struct Cyc_Tcenv_Tenv*_tmp4C8=te;struct Cyc_List_List*_tmp4C9=({Cyc_Tcenv_lookup_type_vars(te);});struct Cyc_Absyn_Kind*_tmp4CA=& Cyc_Tcutil_tmk;int _tmp4CB=1;int _tmp4CC=0;void*_tmp4CD=elt_type;_tmp4C6(_tmp4C7,_tmp4C8,_tmp4C9,_tmp4CA,_tmp4CB,_tmp4CC,_tmp4CD);});
({Cyc_Tcutil_check_no_qual(loc,elt_type);});
if(!({Cyc_Tcexp_check_malloc_type(1,loc,topt,elt_type);}))
({struct Cyc_String_pa_PrintArg_struct _tmp4D0=({struct Cyc_String_pa_PrintArg_struct _tmp6BB;_tmp6BB.tag=0U,({struct _fat_ptr _tmp8F0=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(elt_type);}));_tmp6BB.f1=_tmp8F0;});_tmp6BB;});void*_tmp4CE[1U];_tmp4CE[0]=& _tmp4D0;({unsigned _tmp8F2=loc;struct _fat_ptr _tmp8F1=({const char*_tmp4CF="calloc cannot be used with type %s\n\t(type needs initialization)";_tag_fat(_tmp4CF,sizeof(char),64U);});Cyc_Tcutil_terr(_tmp8F2,_tmp8F1,_tag_fat(_tmp4CE,sizeof(void*),1U));});});
# 2336
num_elts=*e;
# 2339
one_elt=0;}else{
# 2341
void*er=(*e)->r;
retry_sizeof: {
void*_tmp4D1=er;struct Cyc_Absyn_Exp*_tmp4D3;struct Cyc_Absyn_Exp*_tmp4D2;void*_tmp4D4;switch(*((int*)_tmp4D1)){case 17U: _LL6: _tmp4D4=(void*)((struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct*)_tmp4D1)->f1;_LL7: {void*t2=_tmp4D4;
# 2345
elt_type=t2;
({void**_tmp8F3=({void**_tmp4D5=_cycalloc(sizeof(*_tmp4D5));*_tmp4D5=elt_type;_tmp4D5;});*t=_tmp8F3;});
num_elts=({Cyc_Absyn_uint_exp(1U,0U);});
({Cyc_Tcexp_tcExp(te,& Cyc_Absyn_uint_type,num_elts);});
one_elt=1;
goto _LL5;}case 3U: if(((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp4D1)->f1 == Cyc_Absyn_Times){if(((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp4D1)->f2 != 0){if(((struct Cyc_List_List*)((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp4D1)->f2)->tl != 0){if(((struct Cyc_List_List*)((struct Cyc_List_List*)((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp4D1)->f2)->tl)->tl == 0){_LL8: _tmp4D2=(struct Cyc_Absyn_Exp*)(((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp4D1)->f2)->hd;_tmp4D3=(struct Cyc_Absyn_Exp*)((((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp4D1)->f2)->tl)->hd;_LL9: {struct Cyc_Absyn_Exp*e1=_tmp4D2;struct Cyc_Absyn_Exp*e2=_tmp4D3;
# 2352
{struct _tuple0 _stmttmp44=({struct _tuple0 _tmp6BE;_tmp6BE.f1=e1->r,_tmp6BE.f2=e2->r;_tmp6BE;});struct _tuple0 _tmp4D6=_stmttmp44;void*_tmp4D7;void*_tmp4D8;if(((struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct*)_tmp4D6.f1)->tag == 17U){_LLD: _tmp4D8=(void*)((struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct*)_tmp4D6.f1)->f1;_LLE: {void*t1=_tmp4D8;
# 2355
if(!({Cyc_Tcexp_check_malloc_type(0,loc,topt,t1);})){
# 2358
if(!({Cyc_Tcexp_check_malloc_type(1,loc,topt,t1);}))
({struct Cyc_String_pa_PrintArg_struct _tmp4DB=({struct Cyc_String_pa_PrintArg_struct _tmp6BC;_tmp6BC.tag=0U,({struct _fat_ptr _tmp8F4=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t1);}));_tmp6BC.f1=_tmp8F4;});_tmp6BC;});void*_tmp4D9[1U];_tmp4D9[0]=& _tmp4DB;({unsigned _tmp8F6=loc;struct _fat_ptr _tmp8F5=({const char*_tmp4DA="malloc cannot be used with type %s\n\t(type needs initialization)";_tag_fat(_tmp4DA,sizeof(char),64U);});Cyc_Tcutil_terr(_tmp8F6,_tmp8F5,_tag_fat(_tmp4D9,sizeof(void*),1U));});});else{
# 2361
*is_calloc=1;}}
# 2355
elt_type=t1;
# 2365
({void**_tmp8F7=({void**_tmp4DC=_cycalloc(sizeof(*_tmp4DC));*_tmp4DC=elt_type;_tmp4DC;});*t=_tmp8F7;});
num_elts=e2;
one_elt=0;
goto _LLC;}}else{if(((struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct*)_tmp4D6.f2)->tag == 17U){_LLF: _tmp4D7=(void*)((struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct*)_tmp4D6.f2)->f1;_LL10: {void*t2=_tmp4D7;
# 2371
if(!({Cyc_Tcexp_check_malloc_type(0,loc,topt,t2);})){
# 2374
if(!({Cyc_Tcexp_check_malloc_type(1,loc,topt,t2);}))
({struct Cyc_String_pa_PrintArg_struct _tmp4DF=({struct Cyc_String_pa_PrintArg_struct _tmp6BD;_tmp6BD.tag=0U,({struct _fat_ptr _tmp8F8=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t2);}));_tmp6BD.f1=_tmp8F8;});_tmp6BD;});void*_tmp4DD[1U];_tmp4DD[0]=& _tmp4DF;({unsigned _tmp8FA=loc;struct _fat_ptr _tmp8F9=({const char*_tmp4DE="malloc cannot be used with type %s\n\t(type needs initialization)";_tag_fat(_tmp4DE,sizeof(char),64U);});Cyc_Tcutil_terr(_tmp8FA,_tmp8F9,_tag_fat(_tmp4DD,sizeof(void*),1U));});});else{
# 2377
*is_calloc=1;}}
# 2371
elt_type=t2;
# 2381
({void**_tmp8FB=({void**_tmp4E0=_cycalloc(sizeof(*_tmp4E0));*_tmp4E0=elt_type;_tmp4E0;});*t=_tmp8FB;});
num_elts=e1;
one_elt=0;
goto _LLC;}}else{_LL11: _LL12:
 goto No_sizeof;}}_LLC:;}
# 2387
goto _LL5;}}else{goto _LLA;}}else{goto _LLA;}}else{goto _LLA;}}else{goto _LLA;}default: _LLA: _LLB:
# 2389
 No_sizeof: {
# 2392
struct Cyc_Absyn_Exp*real_e=*e;
{void*_stmttmp45=real_e->r;void*_tmp4E1=_stmttmp45;struct Cyc_Absyn_Exp*_tmp4E2;if(((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp4E1)->tag == 14U){_LL14: _tmp4E2=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp4E1)->f2;_LL15: {struct Cyc_Absyn_Exp*e=_tmp4E2;
real_e=e;goto _LL13;}}else{_LL16: _LL17:
 goto _LL13;}_LL13:;}
# 2397
{void*_stmttmp46=({Cyc_Tcutil_compress((void*)_check_null(real_e->topt));});void*_tmp4E3=_stmttmp46;void*_tmp4E4;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp4E3)->tag == 0U){if(((struct Cyc_Absyn_TagCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp4E3)->f1)->tag == 5U){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp4E3)->f2 != 0){_LL19: _tmp4E4=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp4E3)->f2)->hd;_LL1A: {void*tagt=_tmp4E4;
# 2399
{void*_stmttmp47=({Cyc_Tcutil_compress(tagt);});void*_tmp4E5=_stmttmp47;struct Cyc_Absyn_Exp*_tmp4E6;if(((struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_tmp4E5)->tag == 9U){_LL1E: _tmp4E6=((struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_tmp4E5)->f1;_LL1F: {struct Cyc_Absyn_Exp*vexp=_tmp4E6;
# 2401
er=vexp->r;goto retry_sizeof;}}else{_LL20: _LL21:
 goto _LL1D;}_LL1D:;}
# 2404
goto _LL18;}}else{goto _LL1B;}}else{goto _LL1B;}}else{_LL1B: _LL1C:
 goto _LL18;}_LL18:;}
# 2407
elt_type=Cyc_Absyn_char_type;
({void**_tmp8FC=({void**_tmp4E7=_cycalloc(sizeof(*_tmp4E7));*_tmp4E7=elt_type;_tmp4E7;});*t=_tmp8FC;});
num_elts=*e;
one_elt=0;}
# 2412
goto _LL5;}_LL5:;}}
# 2416
*is_fat=!one_elt;
# 2419
{void*_tmp4E8=elt_type;struct Cyc_Absyn_Aggrdecl*_tmp4E9;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp4E8)->tag == 0U){if(((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp4E8)->f1)->tag == 22U){if(((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp4E8)->f1)->f1).KnownAggr).tag == 2){_LL23: _tmp4E9=*((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp4E8)->f1)->f1).KnownAggr).val;_LL24: {struct Cyc_Absyn_Aggrdecl*ad=_tmp4E9;
# 2421
if(ad->impl != 0 &&((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->exist_vars != 0)
({void*_tmp4EA=0U;({unsigned _tmp8FE=loc;struct _fat_ptr _tmp8FD=({const char*_tmp4EB="malloc with existential types not yet implemented";_tag_fat(_tmp4EB,sizeof(char),50U);});Cyc_Tcutil_terr(_tmp8FE,_tmp8FD,_tag_fat(_tmp4EA,sizeof(void*),0U));});});
# 2421
goto _LL22;}}else{goto _LL25;}}else{goto _LL25;}}else{_LL25: _LL26:
# 2424
 goto _LL22;}_LL22:;}{
# 2428
void*(*ptr_maker)(void*t,void*rgn,struct Cyc_Absyn_Tqual,void*zero_term)=Cyc_Absyn_at_type;
void*zero_term=Cyc_Absyn_false_type;
if(topt != 0){
void*_stmttmp48=({Cyc_Tcutil_compress(*topt);});void*_tmp4EC=_stmttmp48;void*_tmp4EF;void*_tmp4EE;void*_tmp4ED;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp4EC)->tag == 3U){_LL28: _tmp4ED=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp4EC)->f1).ptr_atts).nullable;_tmp4EE=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp4EC)->f1).ptr_atts).bounds;_tmp4EF=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp4EC)->f1).ptr_atts).zero_term;_LL29: {void*n=_tmp4ED;void*b=_tmp4EE;void*zt=_tmp4EF;
# 2433
zero_term=zt;
if(({Cyc_Tcutil_force_type2bool(0,n);}))
ptr_maker=Cyc_Absyn_star_type;
# 2434
if(
# 2438
({Cyc_Tcutil_force_type2bool(0,zt);})&& !(*is_calloc)){
({void*_tmp4F0=0U;({unsigned _tmp900=loc;struct _fat_ptr _tmp8FF=({const char*_tmp4F1="converting malloc to calloc to ensure zero-termination";_tag_fat(_tmp4F1,sizeof(char),55U);});Cyc_Tcutil_warn(_tmp900,_tmp8FF,_tag_fat(_tmp4F0,sizeof(void*),0U));});});
*is_calloc=1;}{
# 2434
struct Cyc_Absyn_Exp*eopt=({struct Cyc_Absyn_Exp*(*_tmp506)(void*def,void*b)=Cyc_Tcutil_get_bounds_exp;void*_tmp507=({Cyc_Absyn_bounds_one();});void*_tmp508=b;_tmp506(_tmp507,_tmp508);});
# 2445
if(eopt != 0 && !one_elt){
struct Cyc_Absyn_Exp*upper_exp=eopt;
int is_constant=({Cyc_Evexp_c_can_eval(num_elts);});
if(is_constant &&({Cyc_Evexp_same_const_exp(upper_exp,num_elts);})){
*is_fat=0;
return({void*(*_tmp4F2)(void*t,void*rgn,struct Cyc_Absyn_Tqual tq,void*b,void*zero_term)=Cyc_Absyn_atb_type;void*_tmp4F3=elt_type;void*_tmp4F4=rgn;struct Cyc_Absyn_Tqual _tmp4F5=({Cyc_Absyn_empty_tqual(0U);});void*_tmp4F6=b;void*_tmp4F7=zero_term;_tmp4F2(_tmp4F3,_tmp4F4,_tmp4F5,_tmp4F6,_tmp4F7);});}{
# 2448
void*_stmttmp49=({Cyc_Tcutil_compress((void*)_check_null(num_elts->topt));});void*_tmp4F8=_stmttmp49;void*_tmp4F9;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp4F8)->tag == 0U){if(((struct Cyc_Absyn_TagCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp4F8)->f1)->tag == 5U){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp4F8)->f2 != 0){_LL2D: _tmp4F9=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp4F8)->f2)->hd;_LL2E: {void*tagtyp=_tmp4F9;
# 2454
struct Cyc_Absyn_Exp*tagtyp_exp=({struct Cyc_Absyn_Exp*(*_tmp500)(void*,struct Cyc_Absyn_Exp*,int user_cast,enum Cyc_Absyn_Coercion,unsigned)=Cyc_Absyn_cast_exp;void*_tmp501=Cyc_Absyn_uint_type;struct Cyc_Absyn_Exp*_tmp502=({Cyc_Absyn_valueof_exp(tagtyp,0U);});int _tmp503=0;enum Cyc_Absyn_Coercion _tmp504=Cyc_Absyn_No_coercion;unsigned _tmp505=0U;_tmp500(_tmp501,_tmp502,_tmp503,_tmp504,_tmp505);});
# 2456
if(({Cyc_Evexp_same_const_exp(tagtyp_exp,upper_exp);})){
*is_fat=0;
return({void*(*_tmp4FA)(void*t,void*rgn,struct Cyc_Absyn_Tqual tq,void*b,void*zero_term)=Cyc_Absyn_atb_type;void*_tmp4FB=elt_type;void*_tmp4FC=rgn;struct Cyc_Absyn_Tqual _tmp4FD=({Cyc_Absyn_empty_tqual(0U);});void*_tmp4FE=b;void*_tmp4FF=zero_term;_tmp4FA(_tmp4FB,_tmp4FC,_tmp4FD,_tmp4FE,_tmp4FF);});}
# 2456
goto _LL2C;}}else{goto _LL2F;}}else{goto _LL2F;}}else{_LL2F: _LL30:
# 2461
 goto _LL2C;}_LL2C:;}}
# 2445
goto _LL27;}}}else{_LL2A: _LL2B:
# 2465
 goto _LL27;}_LL27:;}
# 2430
if(!one_elt)
# 2467
ptr_maker=Cyc_Absyn_fatptr_type;
# 2430
return({void*(*_tmp509)(void*t,void*rgn,struct Cyc_Absyn_Tqual,void*zero_term)=ptr_maker;void*_tmp50A=elt_type;void*_tmp50B=rgn;struct Cyc_Absyn_Tqual _tmp50C=({Cyc_Absyn_empty_tqual(0U);});void*_tmp50D=zero_term;_tmp509(_tmp50A,_tmp50B,_tmp50C,_tmp50D);});}}}
# 2472
static void*Cyc_Tcexp_tcSwap(struct Cyc_Tcenv_Tenv*te,unsigned loc,void**topt,struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2){
# 2474
struct Cyc_Tcenv_Tenv*te2=({Cyc_Tcenv_enter_lhs(te);});
({Cyc_Tcexp_tcExpNoPromote(te2,0,e1);});{
void*t1=(void*)_check_null(e1->topt);
({Cyc_Tcexp_tcExpNoPromote(te2,& t1,e2);});{
void*t1=(void*)_check_null(e1->topt);
void*t2=(void*)_check_null(e2->topt);
# 2481
{void*_stmttmp4A=({Cyc_Tcutil_compress(t1);});void*_tmp50F=_stmttmp4A;if(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp50F)->tag == 4U){_LL1: _LL2:
({void*_tmp510=0U;({unsigned _tmp902=loc;struct _fat_ptr _tmp901=({const char*_tmp511="cannot assign to an array";_tag_fat(_tmp511,sizeof(char),26U);});Cyc_Tcutil_terr(_tmp902,_tmp901,_tag_fat(_tmp510,sizeof(void*),0U));});});goto _LL0;}else{_LL3: _LL4:
 goto _LL0;}_LL0:;}
# 2486
if(!({Cyc_Tcutil_is_boxed(t1);})&& !({Cyc_Tcutil_is_pointer_type(t1);}))
({void*_tmp512=0U;({unsigned _tmp904=loc;struct _fat_ptr _tmp903=({const char*_tmp513="Swap not allowed for non-pointer or non-word-sized types.";_tag_fat(_tmp513,sizeof(char),58U);});Cyc_Tcutil_terr(_tmp904,_tmp903,_tag_fat(_tmp512,sizeof(void*),0U));});});
# 2486
if(!({Cyc_Absyn_is_lvalue(e1);}))
# 2491
return({void*_tmp514=0U;({struct Cyc_Tcenv_Tenv*_tmp908=te;unsigned _tmp907=e1->loc;void**_tmp906=topt;struct _fat_ptr _tmp905=({const char*_tmp515="swap non-lvalue";_tag_fat(_tmp515,sizeof(char),16U);});Cyc_Tcexp_expr_err(_tmp908,_tmp907,_tmp906,_tmp905,_tag_fat(_tmp514,sizeof(void*),0U));});});
# 2486
if(!({Cyc_Absyn_is_lvalue(e2);}))
# 2493
return({void*_tmp516=0U;({struct Cyc_Tcenv_Tenv*_tmp90C=te;unsigned _tmp90B=e2->loc;void**_tmp90A=topt;struct _fat_ptr _tmp909=({const char*_tmp517="swap non-lvalue";_tag_fat(_tmp517,sizeof(char),16U);});Cyc_Tcexp_expr_err(_tmp90C,_tmp90B,_tmp90A,_tmp909,_tag_fat(_tmp516,sizeof(void*),0U));});});
# 2486
({Cyc_Tcexp_check_writable(te,e1);});
# 2496
({Cyc_Tcexp_check_writable(te,e2);});
if(!({Cyc_Unify_unify(t1,t2);})){
void*result=({struct Cyc_String_pa_PrintArg_struct _tmp51A=({struct Cyc_String_pa_PrintArg_struct _tmp6C0;_tmp6C0.tag=0U,({
struct _fat_ptr _tmp90D=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t1);}));_tmp6C0.f1=_tmp90D;});_tmp6C0;});struct Cyc_String_pa_PrintArg_struct _tmp51B=({struct Cyc_String_pa_PrintArg_struct _tmp6BF;_tmp6BF.tag=0U,({struct _fat_ptr _tmp90E=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t2);}));_tmp6BF.f1=_tmp90E;});_tmp6BF;});void*_tmp518[2U];_tmp518[0]=& _tmp51A,_tmp518[1]=& _tmp51B;({struct Cyc_Tcenv_Tenv*_tmp912=te;unsigned _tmp911=loc;void**_tmp910=topt;struct _fat_ptr _tmp90F=({const char*_tmp519="type mismatch: %s != %s";_tag_fat(_tmp519,sizeof(char),24U);});Cyc_Tcexp_expr_err(_tmp912,_tmp911,_tmp910,_tmp90F,_tag_fat(_tmp518,sizeof(void*),2U));});});
return result;}
# 2497
return Cyc_Absyn_void_type;}}}
# 2507
static void*Cyc_Tcexp_tcStmtExp(struct Cyc_Tcenv_Tenv*te,unsigned loc,void**topt,struct Cyc_Absyn_Stmt*s){
({void(*_tmp51D)(struct Cyc_Tcenv_Tenv*,struct Cyc_Absyn_Stmt*,int new_block)=Cyc_Tcstmt_tcStmt;struct Cyc_Tcenv_Tenv*_tmp51E=({struct Cyc_Tcenv_Tenv*(*_tmp51F)(struct Cyc_Tcenv_Tenv*)=Cyc_Tcenv_enter_stmt_exp;struct Cyc_Tcenv_Tenv*_tmp520=({Cyc_Tcenv_clear_abstract_val_ok(te);});_tmp51F(_tmp520);});struct Cyc_Absyn_Stmt*_tmp521=s;int _tmp522=1;_tmp51D(_tmp51E,_tmp521,_tmp522);});
# 2510
while(1){
# 2512
void*_stmttmp4B=s->r;void*_tmp523=_stmttmp4B;struct Cyc_Absyn_Stmt*_tmp525;struct Cyc_Absyn_Decl*_tmp524;struct Cyc_Absyn_Stmt*_tmp527;struct Cyc_Absyn_Stmt*_tmp526;struct Cyc_Absyn_Exp*_tmp528;switch(*((int*)_tmp523)){case 1U: _LL1: _tmp528=((struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct*)_tmp523)->f1;_LL2: {struct Cyc_Absyn_Exp*e=_tmp528;
# 2515
void*t=(void*)_check_null(e->topt);
if(!({int(*_tmp529)(void*,void*)=Cyc_Unify_unify;void*_tmp52A=t;void*_tmp52B=({void*(*_tmp52C)(struct Cyc_Core_Opt*)=Cyc_Absyn_wildtyp;struct Cyc_Core_Opt*_tmp52D=({Cyc_Tcenv_lookup_opt_type_vars(te);});_tmp52C(_tmp52D);});_tmp529(_tmp52A,_tmp52B);})){
({struct Cyc_String_pa_PrintArg_struct _tmp530=({struct Cyc_String_pa_PrintArg_struct _tmp6C1;_tmp6C1.tag=0U,({
struct _fat_ptr _tmp913=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t);}));_tmp6C1.f1=_tmp913;});_tmp6C1;});void*_tmp52E[1U];_tmp52E[0]=& _tmp530;({unsigned _tmp915=loc;struct _fat_ptr _tmp914=({const char*_tmp52F="statement expression returns type %s";_tag_fat(_tmp52F,sizeof(char),37U);});Cyc_Tcutil_terr(_tmp915,_tmp914,_tag_fat(_tmp52E,sizeof(void*),1U));});});
({Cyc_Unify_explain_failure();});}
# 2516
return t;}case 2U: _LL3: _tmp526=((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_tmp523)->f1;_tmp527=((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_tmp523)->f2;_LL4: {struct Cyc_Absyn_Stmt*s1=_tmp526;struct Cyc_Absyn_Stmt*s2=_tmp527;
# 2522
s=s2;continue;}case 12U: _LL5: _tmp524=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp523)->f1;_tmp525=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp523)->f2;_LL6: {struct Cyc_Absyn_Decl*d=_tmp524;struct Cyc_Absyn_Stmt*s1=_tmp525;
s=s1;
continue;}default: _LL7: _LL8:
# 2526
 return({struct Cyc_String_pa_PrintArg_struct _tmp533=({struct Cyc_String_pa_PrintArg_struct _tmp6C2;_tmp6C2.tag=0U,({
struct _fat_ptr _tmp916=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_stmt2string(s);}));_tmp6C2.f1=_tmp916;});_tmp6C2;});void*_tmp531[1U];_tmp531[0]=& _tmp533;({struct Cyc_Tcenv_Tenv*_tmp91A=te;unsigned _tmp919=loc;void**_tmp918=topt;struct _fat_ptr _tmp917=({const char*_tmp532="statement expression must end with expression (found \"%s\").";_tag_fat(_tmp532,sizeof(char),60U);});Cyc_Tcexp_expr_err(_tmp91A,_tmp919,_tmp918,_tmp917,_tag_fat(_tmp531,sizeof(void*),1U));});});}_LL0:;}}
# 2532
static void*Cyc_Tcexp_tcTagcheck(struct Cyc_Tcenv_Tenv*te,unsigned loc,void**topt,struct Cyc_Absyn_Exp*e,struct _fat_ptr*f){
void*t=({void*(*_tmp53A)(void*)=Cyc_Tcutil_compress;void*_tmp53B=({void*(*_tmp53C)(struct Cyc_Tcenv_Tenv*,void**,struct Cyc_Absyn_Exp*)=Cyc_Tcexp_tcExp;struct Cyc_Tcenv_Tenv*_tmp53D=({Cyc_Tcenv_enter_abstract_val_ok(te);});void**_tmp53E=0;struct Cyc_Absyn_Exp*_tmp53F=e;_tmp53C(_tmp53D,_tmp53E,_tmp53F);});_tmp53A(_tmp53B);});
{void*_tmp535=t;struct Cyc_Absyn_Aggrdecl*_tmp536;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp535)->tag == 0U){if(((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp535)->f1)->tag == 22U){if(((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp535)->f1)->f1).KnownAggr).tag == 2){_LL1: _tmp536=*((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp535)->f1)->f1).KnownAggr).val;_LL2: {struct Cyc_Absyn_Aggrdecl*ad=_tmp536;
# 2536
if(((int)ad->kind == (int)Cyc_Absyn_UnionA && ad->impl != 0)&&((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->tagged)goto _LL0;goto _LL4;}}else{goto _LL3;}}else{goto _LL3;}}else{_LL3: _LL4:
# 2539
({struct Cyc_String_pa_PrintArg_struct _tmp539=({struct Cyc_String_pa_PrintArg_struct _tmp6C3;_tmp6C3.tag=0U,({struct _fat_ptr _tmp91B=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t);}));_tmp6C3.f1=_tmp91B;});_tmp6C3;});void*_tmp537[1U];_tmp537[0]=& _tmp539;({unsigned _tmp91D=loc;struct _fat_ptr _tmp91C=({const char*_tmp538="expecting @tagged union but found %s";_tag_fat(_tmp538,sizeof(char),37U);});Cyc_Tcutil_terr(_tmp91D,_tmp91C,_tag_fat(_tmp537,sizeof(void*),1U));});});
goto _LL0;}_LL0:;}
# 2542
return Cyc_Absyn_uint_type;}
# 2546
static void*Cyc_Tcexp_tcNew(struct Cyc_Tcenv_Tenv*te0,unsigned loc,void**topt,struct Cyc_Absyn_Exp**rgn_handle,struct Cyc_Absyn_Exp*e,struct Cyc_Absyn_Exp*e1){
# 2550
void*rgn=Cyc_Absyn_heap_rgn_type;
struct Cyc_Tcenv_Tenv*te=({struct Cyc_Tcenv_Tenv*(*_tmp57E)(struct Cyc_Tcenv_Tenv*)=Cyc_Tcenv_clear_abstract_val_ok;struct Cyc_Tcenv_Tenv*_tmp57F=({Cyc_Tcenv_set_new_status(Cyc_Tcenv_InNew,te0);});_tmp57E(_tmp57F);});
if(*rgn_handle != 0){
# 2555
void*expected_type=({void*(*_tmp546)(void*)=Cyc_Absyn_rgn_handle_type;void*_tmp547=({void*(*_tmp548)(struct Cyc_Core_Opt*k,struct Cyc_Core_Opt*tenv)=Cyc_Absyn_new_evar;struct Cyc_Core_Opt*_tmp549=& Cyc_Tcutil_trko;struct Cyc_Core_Opt*_tmp54A=({Cyc_Tcenv_lookup_opt_type_vars(te);});_tmp548(_tmp549,_tmp54A);});_tmp546(_tmp547);});
# 2557
void*handle_type=({Cyc_Tcexp_tcExp(te,& expected_type,(struct Cyc_Absyn_Exp*)_check_null(*rgn_handle));});
void*_stmttmp4C=({Cyc_Tcutil_compress(handle_type);});void*_tmp541=_stmttmp4C;void*_tmp542;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp541)->tag == 0U){if(((struct Cyc_Absyn_RgnHandleCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp541)->f1)->tag == 3U){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp541)->f2 != 0){_LL1: _tmp542=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp541)->f2)->hd;_LL2: {void*r=_tmp542;
# 2560
rgn=r;
({Cyc_Tcenv_check_rgn_accessible(te,loc,rgn);});
goto _LL0;}}else{goto _LL3;}}else{goto _LL3;}}else{_LL3: _LL4:
# 2564
({struct Cyc_String_pa_PrintArg_struct _tmp545=({struct Cyc_String_pa_PrintArg_struct _tmp6C4;_tmp6C4.tag=0U,({
struct _fat_ptr _tmp91E=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(handle_type);}));_tmp6C4.f1=_tmp91E;});_tmp6C4;});void*_tmp543[1U];_tmp543[0]=& _tmp545;({unsigned _tmp920=((struct Cyc_Absyn_Exp*)_check_null(*rgn_handle))->loc;struct _fat_ptr _tmp91F=({const char*_tmp544="expecting region_t type but found %s";_tag_fat(_tmp544,sizeof(char),37U);});Cyc_Tcutil_terr(_tmp920,_tmp91F,_tag_fat(_tmp543,sizeof(void*),1U));});});
goto _LL0;}_LL0:;}else{
# 2571
if(topt != 0){
void*optrgn=Cyc_Absyn_void_type;
if(({Cyc_Tcutil_rgn_of_pointer(*topt,& optrgn);})){
rgn=({Cyc_Tcexp_mallocRgn(optrgn);});
if(rgn == Cyc_Absyn_unique_rgn_type)({struct Cyc_Absyn_Exp*_tmp921=({Cyc_Absyn_uniquergn_exp();});*rgn_handle=_tmp921;});}}}{
# 2552
void*_stmttmp4D=e1->r;void*_tmp54B=_stmttmp4D;struct Cyc_List_List*_tmp54C;struct Cyc_List_List*_tmp54E;struct Cyc_Core_Opt*_tmp54D;switch(*((int*)_tmp54B)){case 27U: _LL6: _LL7:
# 2580
 goto _LL9;case 28U: _LL8: _LL9: {
# 2584
void*res_typ=({Cyc_Tcexp_tcExpNoPromote(te,topt,e1);});
if(!({Cyc_Tcutil_is_array_type(res_typ);}))
({void*_tmp54F=0U;({int(*_tmp923)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp551)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp551;});struct _fat_ptr _tmp922=({const char*_tmp550="tcNew: comprehension returned non-array type";_tag_fat(_tmp550,sizeof(char),45U);});_tmp923(_tmp922,_tag_fat(_tmp54F,sizeof(void*),0U));});});
# 2585
res_typ=({Cyc_Tcutil_promote_array(res_typ,rgn,1);});
# 2588
if(topt != 0){
if(!({Cyc_Unify_unify(*topt,res_typ);})&&({Cyc_Tcutil_silent_castable(te,loc,res_typ,*topt);})){
# 2591
e->topt=res_typ;
({Cyc_Tcutil_unchecked_cast(e,*topt,Cyc_Absyn_Other_coercion);});
res_typ=*topt;}}
# 2588
return res_typ;}case 37U: _LLA: _tmp54D=((struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*)_tmp54B)->f1;_tmp54E=((struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*)_tmp54B)->f2;_LLB: {struct Cyc_Core_Opt*nopt=_tmp54D;struct Cyc_List_List*des=_tmp54E;
# 2598
({void*_tmp924=(void*)({struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*_tmp552=_cycalloc(sizeof(*_tmp552));_tmp552->tag=26U,_tmp552->f1=des;_tmp552;});e1->r=_tmp924;});
_tmp54C=des;goto _LLD;}case 26U: _LLC: _tmp54C=((struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*)_tmp54B)->f1;_LLD: {struct Cyc_List_List*des=_tmp54C;
# 2601
void**elt_typ_opt=0;
int zero_term=0;
if(topt != 0){
void*_stmttmp4E=({Cyc_Tcutil_compress(*topt);});void*_tmp553=_stmttmp4E;void*_tmp556;struct Cyc_Absyn_Tqual _tmp555;void**_tmp554;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp553)->tag == 3U){_LL15: _tmp554=(void**)&(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp553)->f1).elt_type;_tmp555=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp553)->f1).elt_tq;_tmp556=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp553)->f1).ptr_atts).zero_term;_LL16: {void**elt_typ=_tmp554;struct Cyc_Absyn_Tqual tq=_tmp555;void*zt=_tmp556;
# 2607
elt_typ_opt=elt_typ;
zero_term=({Cyc_Tcutil_force_type2bool(0,zt);});
goto _LL14;}}else{_LL17: _LL18:
 goto _LL14;}_LL14:;}{
# 2603
void*res_typ=({Cyc_Tcexp_tcArray(te,e1->loc,elt_typ_opt,0,zero_term,des);});
# 2614
e1->topt=res_typ;
if(!({Cyc_Tcutil_is_array_type(res_typ);}))
({void*_tmp557=0U;({int(*_tmp926)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp559)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp559;});struct _fat_ptr _tmp925=({const char*_tmp558="tcExpNoPromote on Array_e returned non-array type";_tag_fat(_tmp558,sizeof(char),50U);});_tmp926(_tmp925,_tag_fat(_tmp557,sizeof(void*),0U));});});
# 2615
res_typ=({Cyc_Tcutil_promote_array(res_typ,rgn,0);});
# 2618
if(topt != 0){
# 2622
if(!({Cyc_Unify_unify(*topt,res_typ);})&&({Cyc_Tcutil_silent_castable(te,loc,res_typ,*topt);})){
# 2624
e->topt=res_typ;
({Cyc_Tcutil_unchecked_cast(e,*topt,Cyc_Absyn_Other_coercion);});
res_typ=*topt;}}
# 2618
return res_typ;}}case 0U: switch(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp54B)->f1).Wstring_c).tag){case 8U: _LLE: _LLF: {
# 2634
void*topt2=({void*(*_tmp562)(void*t,void*rgn,struct Cyc_Absyn_Tqual tq,void*b,void*zero_term)=Cyc_Absyn_atb_type;void*_tmp563=Cyc_Absyn_char_type;void*_tmp564=rgn;struct Cyc_Absyn_Tqual _tmp565=({Cyc_Absyn_const_tqual(0U);});void*_tmp566=Cyc_Absyn_fat_bound_type;void*_tmp567=Cyc_Absyn_true_type;_tmp562(_tmp563,_tmp564,_tmp565,_tmp566,_tmp567);});
# 2636
void*t=({Cyc_Tcexp_tcExp(te,& topt2,e1);});
return({void*(*_tmp55A)(void*t,void*rgn,struct Cyc_Absyn_Tqual tq,void*b,void*zero_term)=Cyc_Absyn_atb_type;void*_tmp55B=t;void*_tmp55C=rgn;struct Cyc_Absyn_Tqual _tmp55D=({Cyc_Absyn_empty_tqual(0U);});void*_tmp55E=({void*(*_tmp55F)(struct Cyc_Absyn_Exp*)=Cyc_Absyn_thin_bounds_exp;struct Cyc_Absyn_Exp*_tmp560=({Cyc_Absyn_uint_exp(1U,0U);});_tmp55F(_tmp560);});void*_tmp561=Cyc_Absyn_false_type;_tmp55A(_tmp55B,_tmp55C,_tmp55D,_tmp55E,_tmp561);});}case 9U: _LL10: _LL11: {
# 2641
void*topt2=({void*(*_tmp570)(void*t,void*rgn,struct Cyc_Absyn_Tqual tq,void*b,void*zero_term)=Cyc_Absyn_atb_type;void*_tmp571=({Cyc_Absyn_wchar_type();});void*_tmp572=rgn;struct Cyc_Absyn_Tqual _tmp573=({Cyc_Absyn_const_tqual(0U);});void*_tmp574=Cyc_Absyn_fat_bound_type;void*_tmp575=Cyc_Absyn_true_type;_tmp570(_tmp571,_tmp572,_tmp573,_tmp574,_tmp575);});
# 2643
void*t=({Cyc_Tcexp_tcExp(te,& topt2,e1);});
return({void*(*_tmp568)(void*t,void*rgn,struct Cyc_Absyn_Tqual tq,void*b,void*zero_term)=Cyc_Absyn_atb_type;void*_tmp569=t;void*_tmp56A=rgn;struct Cyc_Absyn_Tqual _tmp56B=({Cyc_Absyn_empty_tqual(0U);});void*_tmp56C=({void*(*_tmp56D)(struct Cyc_Absyn_Exp*)=Cyc_Absyn_thin_bounds_exp;struct Cyc_Absyn_Exp*_tmp56E=({Cyc_Absyn_uint_exp(1U,0U);});_tmp56D(_tmp56E);});void*_tmp56F=Cyc_Absyn_false_type;_tmp568(_tmp569,_tmp56A,_tmp56B,_tmp56C,_tmp56F);});}default: goto _LL12;}default: _LL12: _LL13:
# 2650
 RG: {
void*bogus=Cyc_Absyn_void_type;
void**topt2=0;
if(topt != 0){
void*_stmttmp4F=({Cyc_Tcutil_compress(*topt);});void*_tmp576=_stmttmp4F;struct Cyc_Absyn_Tqual _tmp578;void**_tmp577;switch(*((int*)_tmp576)){case 3U: _LL1A: _tmp577=(void**)&(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp576)->f1).elt_type;_tmp578=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp576)->f1).elt_tq;_LL1B: {void**elttype=_tmp577;struct Cyc_Absyn_Tqual tq=_tmp578;
# 2656
topt2=elttype;goto _LL19;}case 0U: if(((struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp576)->f1)->tag == 20U){_LL1C: _LL1D:
# 2660
 bogus=*topt;
topt2=& bogus;
goto _LL19;}else{goto _LL1E;}default: _LL1E: _LL1F:
 goto _LL19;}_LL19:;}{
# 2653
void*telt=({Cyc_Tcexp_tcExp(te,topt2,e1);});
# 2668
if(({Cyc_Tcutil_is_noalias_pointer_or_aggr(telt);})&& !({Cyc_Tcutil_is_noalias_path(e1);}))
({void*_tmp579=0U;({unsigned _tmp928=e1->loc;struct _fat_ptr _tmp927=({const char*_tmp57A="Cannot consume non-unique paths; do swap instead";_tag_fat(_tmp57A,sizeof(char),49U);});Cyc_Tcutil_terr(_tmp928,_tmp927,_tag_fat(_tmp579,sizeof(void*),0U));});});{
# 2668
void*res_typ=(void*)({struct Cyc_Absyn_PointerType_Absyn_Type_struct*_tmp57D=_cycalloc(sizeof(*_tmp57D));
# 2671
_tmp57D->tag=3U,(_tmp57D->f1).elt_type=telt,({struct Cyc_Absyn_Tqual _tmp92B=({Cyc_Absyn_empty_tqual(0U);});(_tmp57D->f1).elt_tq=_tmp92B;}),
((_tmp57D->f1).ptr_atts).rgn=rgn,({void*_tmp92A=({void*(*_tmp57B)(struct Cyc_List_List*)=Cyc_Tcutil_any_bool;struct Cyc_List_List*_tmp57C=({Cyc_Tcenv_lookup_type_vars(te);});_tmp57B(_tmp57C);});((_tmp57D->f1).ptr_atts).nullable=_tmp92A;}),({
void*_tmp929=({Cyc_Absyn_bounds_one();});((_tmp57D->f1).ptr_atts).bounds=_tmp929;}),((_tmp57D->f1).ptr_atts).zero_term=Cyc_Absyn_false_type,((_tmp57D->f1).ptr_atts).ptrloc=0;_tmp57D;});
# 2675
if(topt != 0){
if(!({Cyc_Unify_unify(*topt,res_typ);})&&({Cyc_Tcutil_silent_castable(te,loc,res_typ,*topt);})){
# 2678
e->topt=res_typ;
({Cyc_Tcutil_unchecked_cast(e,*topt,Cyc_Absyn_Other_coercion);});
res_typ=*topt;}}
# 2675
return res_typ;}}}}_LL5:;}}
# 2546
void*Cyc_Tcexp_tcExp(struct Cyc_Tcenv_Tenv*te,void**topt,struct Cyc_Absyn_Exp*e){
# 2690
void*t=({void*(*_tmp585)(void*)=Cyc_Tcutil_compress;void*_tmp586=({Cyc_Tcexp_tcExpNoPromote(te,topt,e);});_tmp585(_tmp586);});
if(({Cyc_Tcutil_is_array_type(t);}))
({void*_tmp92C=t=({void*(*_tmp581)(void*t,void*rgn,int convert_tag)=Cyc_Tcutil_promote_array;void*_tmp582=t;void*_tmp583=({Cyc_Tcutil_addressof_props(e);}).f2;int _tmp584=0;_tmp581(_tmp582,_tmp583,_tmp584);});e->topt=_tmp92C;});
# 2691
return t;}
# 2546
void*Cyc_Tcexp_tcExpInitializer(struct Cyc_Tcenv_Tenv*te,void**topt,struct Cyc_Absyn_Exp*e){
# 2700
void*t=({Cyc_Tcexp_tcExpNoPromote(te,topt,e);});
# 2703
if(({Cyc_Tcutil_is_noalias_pointer_or_aggr(t);})&& !({Cyc_Tcutil_is_noalias_path(e);}))
# 2708
({void*_tmp588=0U;({unsigned _tmp92E=e->loc;struct _fat_ptr _tmp92D=({const char*_tmp589="Cannot consume non-unique paths; do swap instead";_tag_fat(_tmp589,sizeof(char),49U);});Cyc_Tcutil_terr(_tmp92E,_tmp92D,_tag_fat(_tmp588,sizeof(void*),0U));});});{
# 2703
void*_stmttmp50=e->r;void*_tmp58A=_stmttmp50;switch(*((int*)_tmp58A)){case 26U: _LL1: _LL2:
# 2711
 goto _LL4;case 27U: _LL3: _LL4:
 goto _LL6;case 28U: _LL5: _LL6:
 goto _LL8;case 0U: switch(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp58A)->f1).String_c).tag){case 9U: _LL7: _LL8:
 goto _LLA;case 8U: _LL9: _LLA:
 return t;default: goto _LLB;}default: _LLB: _LLC:
# 2717
 t=({Cyc_Tcutil_compress(t);});
if(({Cyc_Tcutil_is_array_type(t);})){
t=({void*(*_tmp58B)(void*t,void*rgn,int convert_tag)=Cyc_Tcutil_promote_array;void*_tmp58C=t;void*_tmp58D=({Cyc_Tcutil_addressof_props(e);}).f2;int _tmp58E=0;_tmp58B(_tmp58C,_tmp58D,_tmp58E);});
({Cyc_Tcutil_unchecked_cast(e,t,Cyc_Absyn_Other_coercion);});}
# 2718
return t;}_LL0:;}}
# 2733 "tcexp.cyc"
static void*Cyc_Tcexp_tcExpNoPromote(struct Cyc_Tcenv_Tenv*te,void**topt,struct Cyc_Absyn_Exp*e){
{void*_stmttmp51=e->r;void*_tmp590=_stmttmp51;struct Cyc_Absyn_Exp*_tmp591;if(((struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*)_tmp590)->tag == 12U){_LL1: _tmp591=((struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*)_tmp590)->f1;_LL2: {struct Cyc_Absyn_Exp*e2=_tmp591;
# 2737
({Cyc_Tcexp_tcExpNoInst(te,topt,e2);});
({void*_tmp92F=({Cyc_Absyn_pointer_expand((void*)_check_null(e2->topt),0);});e2->topt=_tmp92F;});
e->topt=e2->topt;
goto _LL0;}}else{_LL3: _LL4:
# 2743
({Cyc_Tcexp_tcExpNoInst(te,topt,e);});
({void*_tmp930=({Cyc_Absyn_pointer_expand((void*)_check_null(e->topt),0);});e->topt=_tmp930;});
# 2746
{void*_stmttmp52=({Cyc_Tcutil_compress((void*)_check_null(e->topt));});void*_tmp592=_stmttmp52;void*_tmp598;void*_tmp597;void*_tmp596;void*_tmp595;struct Cyc_Absyn_Tqual _tmp594;void*_tmp593;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp592)->tag == 3U){_LL6: _tmp593=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp592)->f1).elt_type;_tmp594=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp592)->f1).elt_tq;_tmp595=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp592)->f1).ptr_atts).rgn;_tmp596=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp592)->f1).ptr_atts).nullable;_tmp597=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp592)->f1).ptr_atts).bounds;_tmp598=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp592)->f1).ptr_atts).zero_term;_LL7: {void*t=_tmp593;struct Cyc_Absyn_Tqual tq=_tmp594;void*rt=_tmp595;void*x=_tmp596;void*b=_tmp597;void*zt=_tmp598;
# 2748
{void*_stmttmp53=({Cyc_Tcutil_compress(t);});void*_tmp599=_stmttmp53;int _tmp5AA;struct Cyc_List_List*_tmp5A9;struct Cyc_List_List*_tmp5A8;struct Cyc_List_List*_tmp5A7;struct Cyc_List_List*_tmp5A6;struct Cyc_Absyn_Exp*_tmp5A5;struct Cyc_List_List*_tmp5A4;struct Cyc_Absyn_Exp*_tmp5A3;struct Cyc_List_List*_tmp5A2;struct Cyc_List_List*_tmp5A1;struct Cyc_Absyn_VarargInfo*_tmp5A0;int _tmp59F;struct Cyc_List_List*_tmp59E;void*_tmp59D;struct Cyc_Absyn_Tqual _tmp59C;void*_tmp59B;struct Cyc_List_List*_tmp59A;if(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp599)->tag == 5U){_LLB: _tmp59A=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp599)->f1).tvars;_tmp59B=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp599)->f1).effect;_tmp59C=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp599)->f1).ret_tqual;_tmp59D=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp599)->f1).ret_type;_tmp59E=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp599)->f1).args;_tmp59F=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp599)->f1).c_varargs;_tmp5A0=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp599)->f1).cyc_varargs;_tmp5A1=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp599)->f1).rgn_po;_tmp5A2=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp599)->f1).attributes;_tmp5A3=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp599)->f1).requires_clause;_tmp5A4=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp599)->f1).requires_relns;_tmp5A5=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp599)->f1).ensures_clause;_tmp5A6=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp599)->f1).ensures_relns;_tmp5A7=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp599)->f1).ieffect;_tmp5A8=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp599)->f1).oeffect;_tmp5A9=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp599)->f1).throws;_tmp5AA=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp599)->f1).reentrant;_LLC: {struct Cyc_List_List*tvs=_tmp59A;void*eff=_tmp59B;struct Cyc_Absyn_Tqual rtq=_tmp59C;void*rtyp=_tmp59D;struct Cyc_List_List*args=_tmp59E;int c_varargs=_tmp59F;struct Cyc_Absyn_VarargInfo*cyc_varargs=_tmp5A0;struct Cyc_List_List*rpo=_tmp5A1;struct Cyc_List_List*atts=_tmp5A2;struct Cyc_Absyn_Exp*req=_tmp5A3;struct Cyc_List_List*req_relns=_tmp5A4;struct Cyc_Absyn_Exp*ens=_tmp5A5;struct Cyc_List_List*ens_relns=_tmp5A6;struct Cyc_List_List*ieff=_tmp5A7;struct Cyc_List_List*oeff=_tmp5A8;struct Cyc_List_List*throws=_tmp5A9;int reentrant=_tmp5AA;
# 2752
if(tvs != 0){
struct _tuple14 env=({struct _tuple14 _tmp6C5;({struct Cyc_List_List*_tmp931=({Cyc_Tcenv_lookup_type_vars(te);});_tmp6C5.f1=_tmp931;}),_tmp6C5.f2=Cyc_Core_heap_region;_tmp6C5;});
struct Cyc_List_List*inst=({({struct Cyc_List_List*(*_tmp932)(struct _tuple15*(*f)(struct _tuple14*,struct Cyc_Absyn_Tvar*),struct _tuple14*env,struct Cyc_List_List*x)=({
struct Cyc_List_List*(*_tmp5B0)(struct _tuple15*(*f)(struct _tuple14*,struct Cyc_Absyn_Tvar*),struct _tuple14*env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct _tuple15*(*f)(struct _tuple14*,struct Cyc_Absyn_Tvar*),struct _tuple14*env,struct Cyc_List_List*x))Cyc_List_map_c;_tmp5B0;});_tmp932(Cyc_Tcutil_r_make_inst_var,& env,tvs);});});
struct Cyc_List_List*ts=({({struct Cyc_List_List*(*_tmp934)(void*(*f)(struct _tuple15*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp5AE)(void*(*f)(struct _tuple15*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(void*(*f)(struct _tuple15*),struct Cyc_List_List*x))Cyc_List_map;_tmp5AE;});void*(*_tmp933)(struct _tuple15*)=({void*(*_tmp5AF)(struct _tuple15*)=(void*(*)(struct _tuple15*))Cyc_Core_snd;_tmp5AF;});_tmp934(_tmp933,inst);});});
# 2763
void*ftyp=({({struct Cyc_List_List*_tmp935=inst;Cyc_Tcutil_substitute(_tmp935,(void*)({struct Cyc_Absyn_FnType_Absyn_Type_struct*_tmp5AD=_cycalloc(sizeof(*_tmp5AD));_tmp5AD->tag=5U,(_tmp5AD->f1).tvars=0,(_tmp5AD->f1).effect=eff,(_tmp5AD->f1).ret_tqual=rtq,(_tmp5AD->f1).ret_type=rtyp,(_tmp5AD->f1).args=args,(_tmp5AD->f1).c_varargs=c_varargs,(_tmp5AD->f1).cyc_varargs=cyc_varargs,(_tmp5AD->f1).rgn_po=0,(_tmp5AD->f1).attributes=atts,(_tmp5AD->f1).requires_clause=req,(_tmp5AD->f1).requires_relns=req_relns,(_tmp5AD->f1).ensures_clause=ens,(_tmp5AD->f1).ensures_relns=ens_relns,(_tmp5AD->f1).ieffect=ieff,(_tmp5AD->f1).oeffect=oeff,(_tmp5AD->f1).throws=throws,(_tmp5AD->f1).reentrant=reentrant;_tmp5AD;}));});});
# 2768
struct Cyc_Absyn_PointerType_Absyn_Type_struct*new_typ=({struct Cyc_Absyn_PointerType_Absyn_Type_struct*_tmp5AC=_cycalloc(sizeof(*_tmp5AC));_tmp5AC->tag=3U,(_tmp5AC->f1).elt_type=ftyp,(_tmp5AC->f1).elt_tq=tq,((_tmp5AC->f1).ptr_atts).rgn=rt,((_tmp5AC->f1).ptr_atts).nullable=x,((_tmp5AC->f1).ptr_atts).bounds=b,((_tmp5AC->f1).ptr_atts).zero_term=zt,((_tmp5AC->f1).ptr_atts).ptrloc=0;_tmp5AC;});
# 2770
struct Cyc_Absyn_Exp*inner=({Cyc_Absyn_copy_exp(e);});
({void*_tmp936=(void*)({struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*_tmp5AB=_cycalloc(sizeof(*_tmp5AB));_tmp5AB->tag=13U,_tmp5AB->f1=inner,_tmp5AB->f2=ts;_tmp5AB;});e->r=_tmp936;});
e->topt=(void*)new_typ;}
# 2752
goto _LLA;}}else{_LLD: _LLE:
# 2775
 goto _LLA;}_LLA:;}
# 2777
goto _LL5;}}else{_LL8: _LL9:
 goto _LL5;}_LL5:;}
# 2780
goto _LL0;}_LL0:;}
# 2782
return(void*)_check_null(e->topt);}
# 2790
static void Cyc_Tcexp_insert_alias_stmts(struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Exp*e,struct Cyc_Absyn_Exp*fn_exp,struct Cyc_List_List*alias_arg_exps){
# 2793
struct Cyc_List_List*decls=0;
# 2795
{void*_stmttmp54=fn_exp->r;void*_tmp5B2=_stmttmp54;struct Cyc_List_List*_tmp5B3;if(((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp5B2)->tag == 10U){_LL1: _tmp5B3=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp5B2)->f2;_LL2: {struct Cyc_List_List*es=_tmp5B3;
# 2798
{void*_stmttmp55=e->r;void*_tmp5B4=_stmttmp55;struct Cyc_List_List*_tmp5B5;if(((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp5B4)->tag == 10U){_LL6: _tmp5B5=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp5B4)->f2;_LL7: {struct Cyc_List_List*es2=_tmp5B5;
# 2801
struct Cyc_List_List*arg_exps=alias_arg_exps;
int arg_cnt=0;
while(arg_exps != 0){
# 2805
while(arg_cnt != (int)arg_exps->hd){
# 2807
if(es == 0)
({void*_tmp5B6=0U;({int(*_tmp939)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp5BB)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp5BB;});struct _fat_ptr _tmp938=(struct _fat_ptr)({struct Cyc_Int_pa_PrintArg_struct _tmp5B9=({struct Cyc_Int_pa_PrintArg_struct _tmp6C7;_tmp6C7.tag=1U,_tmp6C7.f1=(unsigned long)arg_cnt;_tmp6C7;});struct Cyc_Int_pa_PrintArg_struct _tmp5BA=({struct Cyc_Int_pa_PrintArg_struct _tmp6C6;_tmp6C6.tag=1U,_tmp6C6.f1=(unsigned long)((int)arg_exps->hd);_tmp6C6;});void*_tmp5B7[2U];_tmp5B7[0]=& _tmp5B9,_tmp5B7[1]=& _tmp5BA;({struct _fat_ptr _tmp937=({const char*_tmp5B8="bad count %d/%d for alias coercion!";_tag_fat(_tmp5B8,sizeof(char),36U);});Cyc_aprintf(_tmp937,_tag_fat(_tmp5B7,sizeof(void*),2U));});});_tmp939(_tmp938,_tag_fat(_tmp5B6,sizeof(void*),0U));});});
# 2807
++ arg_cnt;
# 2811
es=es->tl;
es2=((struct Cyc_List_List*)_check_null(es2))->tl;}{
# 2815
struct _tuple13 _stmttmp56=({struct _tuple13(*_tmp5BF)(struct Cyc_Absyn_Exp*e,void*e_typ)=Cyc_Tcutil_insert_alias;struct Cyc_Absyn_Exp*_tmp5C0=(struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(es))->hd;void*_tmp5C1=({Cyc_Tcutil_copy_type((void*)_check_null(((struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(es2))->hd)->topt));});_tmp5BF(_tmp5C0,_tmp5C1);});struct Cyc_Absyn_Exp*_tmp5BD;struct Cyc_Absyn_Decl*_tmp5BC;_LLB: _tmp5BC=_stmttmp56.f1;_tmp5BD=_stmttmp56.f2;_LLC: {struct Cyc_Absyn_Decl*d=_tmp5BC;struct Cyc_Absyn_Exp*ve=_tmp5BD;
es->hd=(void*)ve;
decls=({struct Cyc_List_List*_tmp5BE=_cycalloc(sizeof(*_tmp5BE));_tmp5BE->hd=d,_tmp5BE->tl=decls;_tmp5BE;});
arg_exps=arg_exps->tl;}}}
# 2820
goto _LL5;}}else{_LL8: _LL9:
# 2822
({void*_tmp5C2=0U;({int(*_tmp93B)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp5C4)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp5C4;});struct _fat_ptr _tmp93A=({const char*_tmp5C3="not a function call!";_tag_fat(_tmp5C3,sizeof(char),21U);});_tmp93B(_tmp93A,_tag_fat(_tmp5C2,sizeof(void*),0U));});});}_LL5:;}
# 2824
goto _LL0;}}else{_LL3: _LL4:
# 2826
({void*_tmp5C5=0U;({int(*_tmp93D)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp5C7)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp5C7;});struct _fat_ptr _tmp93C=({const char*_tmp5C6="not a function call!";_tag_fat(_tmp5C6,sizeof(char),21U);});_tmp93D(_tmp93C,_tag_fat(_tmp5C5,sizeof(void*),0U));});});}_LL0:;}
# 2830
while(decls != 0){
# 2832
struct Cyc_Absyn_Decl*d=(struct Cyc_Absyn_Decl*)decls->hd;
fn_exp=({struct Cyc_Absyn_Exp*(*_tmp5C8)(struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_stmt_exp;struct Cyc_Absyn_Stmt*_tmp5C9=({struct Cyc_Absyn_Stmt*(*_tmp5CA)(struct Cyc_Absyn_Decl*,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_decl_stmt;struct Cyc_Absyn_Decl*_tmp5CB=d;struct Cyc_Absyn_Stmt*_tmp5CC=({Cyc_Absyn_exp_stmt(fn_exp,e->loc);});unsigned _tmp5CD=e->loc;_tmp5CA(_tmp5CB,_tmp5CC,_tmp5CD);});unsigned _tmp5CE=e->loc;_tmp5C8(_tmp5C9,_tmp5CE);});
decls=decls->tl;}
# 2838
e->topt=0;
e->r=fn_exp->r;}
# 2844
static void Cyc_Tcexp_tcExpNoInst(struct Cyc_Tcenv_Tenv*te,void**topt,struct Cyc_Absyn_Exp*e){
unsigned loc=e->loc;
void*t;
# 2848
{void*_stmttmp57=e->r;void*_tmp5D0=_stmttmp57;void*_tmp5D1;struct Cyc_Absyn_Exp*_tmp5D2;struct _fat_ptr*_tmp5D4;struct Cyc_Absyn_Exp*_tmp5D3;struct Cyc_Absyn_Exp*_tmp5D6;struct Cyc_Absyn_Exp*_tmp5D5;int*_tmp5DB;struct Cyc_Absyn_Exp**_tmp5DA;void***_tmp5D9;struct Cyc_Absyn_Exp**_tmp5D8;int*_tmp5D7;struct Cyc_Absyn_Enumfield*_tmp5DD;void*_tmp5DC;struct Cyc_Absyn_Enumfield*_tmp5DF;struct Cyc_Absyn_Enumdecl*_tmp5DE;struct Cyc_Absyn_Datatypefield*_tmp5E2;struct Cyc_Absyn_Datatypedecl*_tmp5E1;struct Cyc_List_List*_tmp5E0;struct Cyc_List_List*_tmp5E4;void*_tmp5E3;struct Cyc_Absyn_Aggrdecl**_tmp5E8;struct Cyc_List_List*_tmp5E7;struct Cyc_List_List**_tmp5E6;struct _tuple1**_tmp5E5;int*_tmp5EB;void*_tmp5EA;struct Cyc_Absyn_Exp*_tmp5E9;int*_tmp5EF;struct Cyc_Absyn_Exp*_tmp5EE;struct Cyc_Absyn_Exp*_tmp5ED;struct Cyc_Absyn_Vardecl*_tmp5EC;struct Cyc_Absyn_Stmt*_tmp5F0;struct Cyc_List_List*_tmp5F1;struct Cyc_List_List*_tmp5F3;struct _tuple10*_tmp5F2;struct Cyc_List_List*_tmp5F4;struct Cyc_Absyn_Exp*_tmp5F6;struct Cyc_Absyn_Exp*_tmp5F5;int*_tmp5FA;int*_tmp5F9;struct _fat_ptr*_tmp5F8;struct Cyc_Absyn_Exp*_tmp5F7;int*_tmp5FE;int*_tmp5FD;struct _fat_ptr*_tmp5FC;struct Cyc_Absyn_Exp*_tmp5FB;struct Cyc_Absyn_Exp*_tmp5FF;struct Cyc_List_List*_tmp601;void*_tmp600;void*_tmp602;struct Cyc_Absyn_Exp*_tmp603;struct Cyc_Absyn_Exp*_tmp605;struct Cyc_Absyn_Exp**_tmp604;struct Cyc_Absyn_Exp*_tmp606;enum Cyc_Absyn_Coercion*_tmp609;struct Cyc_Absyn_Exp*_tmp608;void*_tmp607;struct Cyc_List_List*_tmp60B;struct Cyc_Absyn_Exp*_tmp60A;struct Cyc_Absyn_Exp*_tmp60C;struct Cyc_Absyn_Exp*_tmp60E;struct Cyc_Absyn_Exp*_tmp60D;struct Cyc_Absyn_Exp*_tmp610;struct Cyc_Absyn_Exp*_tmp60F;struct Cyc_Absyn_Exp*_tmp612;struct Cyc_Absyn_Exp*_tmp611;struct Cyc_Absyn_Exp*_tmp614;struct Cyc_Absyn_Exp*_tmp613;struct Cyc_Absyn_Exp*_tmp617;struct Cyc_Absyn_Exp*_tmp616;struct Cyc_Absyn_Exp*_tmp615;struct Cyc_Absyn_Exp*_tmp61A;struct Cyc_Core_Opt*_tmp619;struct Cyc_Absyn_Exp*_tmp618;enum Cyc_Absyn_Incrementor _tmp61C;struct Cyc_Absyn_Exp*_tmp61B;struct Cyc_List_List*_tmp61E;enum Cyc_Absyn_Primop _tmp61D;void**_tmp61F;union Cyc_Absyn_Cnst*_tmp620;struct Cyc_List_List*_tmp622;struct Cyc_Core_Opt*_tmp621;struct Cyc_Absyn_VarargCallInfo**_tmp625;struct Cyc_List_List*_tmp624;struct Cyc_Absyn_Exp*_tmp623;int*_tmp629;struct Cyc_Absyn_VarargCallInfo**_tmp628;struct Cyc_List_List*_tmp627;struct Cyc_Absyn_Exp*_tmp626;struct Cyc_Absyn_Exp*_tmp62A;switch(*((int*)_tmp5D0)){case 12U: _LL1: _tmp62A=((struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*)_tmp5D0)->f1;_LL2: {struct Cyc_Absyn_Exp*e2=_tmp62A;
# 2853
({Cyc_Tcexp_tcExpNoInst(te,0,e2);});
return;}case 10U: _LL3: _tmp626=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp5D0)->f1;_tmp627=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp5D0)->f2;_tmp628=(struct Cyc_Absyn_VarargCallInfo**)&((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp5D0)->f3;_tmp629=(int*)&((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp5D0)->f4;if(!(*_tmp629)){_LL4: {struct Cyc_Absyn_Exp*e1=_tmp626;struct Cyc_List_List*es=_tmp627;struct Cyc_Absyn_VarargCallInfo**vci=_tmp628;int*resolved=_tmp629;
# 2857
({void*_tmp62B=0U;({int(*_tmp93F)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp62D)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp62D;});struct _fat_ptr _tmp93E=({const char*_tmp62C="unresolved function in tcExpNoInst";_tag_fat(_tmp62C,sizeof(char),35U);});_tmp93F(_tmp93E,_tag_fat(_tmp62B,sizeof(void*),0U));});});}}else{_LL1D: _tmp623=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp5D0)->f1;_tmp624=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp5D0)->f2;_tmp625=(struct Cyc_Absyn_VarargCallInfo**)&((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp5D0)->f3;_LL1E: {struct Cyc_Absyn_Exp*e1=_tmp623;struct Cyc_List_List*es=_tmp624;struct Cyc_Absyn_VarargCallInfo**vci=_tmp625;
# 2894
struct Cyc_List_List*alias_arg_exps=0;
int ok=1;
struct Cyc_Absyn_Exp*fn_exp;
{struct _handler_cons _tmp62E;_push_handler(& _tmp62E);{int _tmp630=0;if(setjmp(_tmp62E.handler))_tmp630=1;if(!_tmp630){
fn_exp=({Cyc_Tcutil_deep_copy_exp(0,e);});;_pop_handler();}else{void*_tmp62F=(void*)Cyc_Core_get_exn_thrown();void*_tmp631=_tmp62F;void*_tmp632;if(((struct Cyc_Core_Failure_exn_struct*)_tmp631)->tag == Cyc_Core_Failure){_LL5A: _LL5B:
# 2901
 ok=0;
fn_exp=e;
goto _LL59;}else{_LL5C: _tmp632=_tmp631;_LL5D: {void*exn=_tmp632;(int)_rethrow(exn);}}_LL59:;}}}
# 2905
t=({Cyc_Tcexp_tcFnCall(te,loc,topt,e1,es,vci,& alias_arg_exps);});
if(alias_arg_exps != 0 && ok){
alias_arg_exps=({Cyc_List_imp_rev(alias_arg_exps);});
({Cyc_Tcexp_insert_alias_stmts(te,e,fn_exp,alias_arg_exps);});
({Cyc_Tcexp_tcExpNoInst(te,topt,e);});
return;}
# 2906
goto _LL0;}}case 37U: _LL5: _tmp621=((struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*)_tmp5D0)->f1;_tmp622=((struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*)_tmp5D0)->f2;_LL6: {struct Cyc_Core_Opt*nopt=_tmp621;struct Cyc_List_List*des=_tmp622;
# 2861
({Cyc_Tcexp_resolve_unresolved_mem(te,loc,topt,e,des);});
({Cyc_Tcexp_tcExpNoInst(te,topt,e);});
return;}case 0U: _LL7: _tmp620=(union Cyc_Absyn_Cnst*)&((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp5D0)->f1;_LL8: {union Cyc_Absyn_Cnst*c=_tmp620;
# 2866
t=({Cyc_Tcexp_tcConst(te,loc,topt,c,e);});goto _LL0;}case 1U: _LL9: _tmp61F=(void**)&((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp5D0)->f1;_LLA: {void**b=_tmp61F;
# 2868
t=({Cyc_Tcexp_tcVar(te,loc,topt,e,b);});goto _LL0;}case 2U: _LLB: _LLC:
# 2870
 t=Cyc_Absyn_sint_type;goto _LL0;case 3U: _LLD: _tmp61D=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp5D0)->f1;_tmp61E=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp5D0)->f2;_LLE: {enum Cyc_Absyn_Primop p=_tmp61D;struct Cyc_List_List*es=_tmp61E;
# 2872
t=({Cyc_Tcexp_tcPrimop(te,loc,topt,p,es);});goto _LL0;}case 5U: _LLF: _tmp61B=((struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*)_tmp5D0)->f1;_tmp61C=((struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*)_tmp5D0)->f2;_LL10: {struct Cyc_Absyn_Exp*e1=_tmp61B;enum Cyc_Absyn_Incrementor i=_tmp61C;
# 2874
t=({Cyc_Tcexp_tcIncrement(te,loc,topt,e1,i);});goto _LL0;}case 4U: _LL11: _tmp618=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp5D0)->f1;_tmp619=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp5D0)->f2;_tmp61A=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp5D0)->f3;_LL12: {struct Cyc_Absyn_Exp*e1=_tmp618;struct Cyc_Core_Opt*popt=_tmp619;struct Cyc_Absyn_Exp*e2=_tmp61A;
# 2876
t=({Cyc_Tcexp_tcAssignOp(te,loc,topt,e1,popt,e2);});goto _LL0;}case 6U: _LL13: _tmp615=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp5D0)->f1;_tmp616=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp5D0)->f2;_tmp617=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp5D0)->f3;_LL14: {struct Cyc_Absyn_Exp*e1=_tmp615;struct Cyc_Absyn_Exp*e2=_tmp616;struct Cyc_Absyn_Exp*e3=_tmp617;
# 2878
t=({Cyc_Tcexp_tcConditional(te,loc,topt,e1,e2,e3);});goto _LL0;}case 7U: _LL15: _tmp613=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp5D0)->f1;_tmp614=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp5D0)->f2;_LL16: {struct Cyc_Absyn_Exp*e1=_tmp613;struct Cyc_Absyn_Exp*e2=_tmp614;
# 2880
t=({Cyc_Tcexp_tcAnd(te,loc,e1,e2);});goto _LL0;}case 8U: _LL17: _tmp611=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp5D0)->f1;_tmp612=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp5D0)->f2;_LL18: {struct Cyc_Absyn_Exp*e1=_tmp611;struct Cyc_Absyn_Exp*e2=_tmp612;
# 2882
t=({Cyc_Tcexp_tcOr(te,loc,e1,e2);});goto _LL0;}case 34U: _LL19: _tmp60F=((struct Cyc_Absyn_Spawn_e_Absyn_Raw_exp_struct*)_tmp5D0)->f1;_tmp610=((struct Cyc_Absyn_Spawn_e_Absyn_Raw_exp_struct*)_tmp5D0)->f2;_LL1A: {struct Cyc_Absyn_Exp*e1=_tmp60F;struct Cyc_Absyn_Exp*e2=_tmp610;
# 2885
t=({Cyc_Tcexp_tcSpawn(te,loc,topt,e1,e2);});goto _LL0;}case 9U: _LL1B: _tmp60D=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp5D0)->f1;_tmp60E=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp5D0)->f2;_LL1C: {struct Cyc_Absyn_Exp*e1=_tmp60D;struct Cyc_Absyn_Exp*e2=_tmp60E;
# 2888
t=({Cyc_Tcexp_tcSeqExp(te,loc,topt,e1,e2);});goto _LL0;}case 11U: _LL1F: _tmp60C=((struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct*)_tmp5D0)->f1;_LL20: {struct Cyc_Absyn_Exp*e1=_tmp60C;
# 2914
t=({Cyc_Tcexp_tcThrow(te,loc,topt,e1);});goto _LL0;}case 13U: _LL21: _tmp60A=((struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*)_tmp5D0)->f1;_tmp60B=((struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*)_tmp5D0)->f2;_LL22: {struct Cyc_Absyn_Exp*e2=_tmp60A;struct Cyc_List_List*ts=_tmp60B;
# 2916
t=({Cyc_Tcexp_tcInstantiate(te,loc,topt,e2,ts);});goto _LL0;}case 14U: _LL23: _tmp607=(void*)((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp5D0)->f1;_tmp608=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp5D0)->f2;_tmp609=(enum Cyc_Absyn_Coercion*)&((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp5D0)->f4;_LL24: {void*t1=_tmp607;struct Cyc_Absyn_Exp*e1=_tmp608;enum Cyc_Absyn_Coercion*c=_tmp609;
# 2918
t=({Cyc_Tcexp_tcCast(te,loc,topt,t1,e1,c);});goto _LL0;}case 15U: _LL25: _tmp606=((struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct*)_tmp5D0)->f1;_LL26: {struct Cyc_Absyn_Exp*e1=_tmp606;
# 2920
t=({Cyc_Tcexp_tcAddress(te,loc,e,topt,e1);});goto _LL0;}case 16U: _LL27: _tmp604=(struct Cyc_Absyn_Exp**)&((struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct*)_tmp5D0)->f1;_tmp605=((struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct*)_tmp5D0)->f2;_LL28: {struct Cyc_Absyn_Exp**rgn_handle=_tmp604;struct Cyc_Absyn_Exp*e1=_tmp605;
# 2922
t=({Cyc_Tcexp_tcNew(te,loc,topt,rgn_handle,e,e1);});goto _LL0;}case 18U: _LL29: _tmp603=((struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct*)_tmp5D0)->f1;_LL2A: {struct Cyc_Absyn_Exp*e1=_tmp603;
# 2924
void*t1=({Cyc_Tcexp_tcExpNoPromote(te,0,e1);});
t=({Cyc_Tcexp_tcSizeof(te,loc,topt,t1);});goto _LL0;}case 17U: _LL2B: _tmp602=(void*)((struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct*)_tmp5D0)->f1;_LL2C: {void*t1=_tmp602;
# 2927
t=({Cyc_Tcexp_tcSizeof(te,loc,topt,t1);});goto _LL0;}case 19U: _LL2D: _tmp600=(void*)((struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct*)_tmp5D0)->f1;_tmp601=((struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct*)_tmp5D0)->f2;_LL2E: {void*t1=_tmp600;struct Cyc_List_List*l=_tmp601;
# 2929
t=({Cyc_Tcexp_tcOffsetof(te,loc,topt,t1,l);});goto _LL0;}case 20U: _LL2F: _tmp5FF=((struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_tmp5D0)->f1;_LL30: {struct Cyc_Absyn_Exp*e1=_tmp5FF;
# 2931
t=({Cyc_Tcexp_tcDeref(te,loc,topt,e1);});goto _LL0;}case 21U: _LL31: _tmp5FB=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp5D0)->f1;_tmp5FC=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp5D0)->f2;_tmp5FD=(int*)&((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp5D0)->f3;_tmp5FE=(int*)&((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp5D0)->f4;_LL32: {struct Cyc_Absyn_Exp*e1=_tmp5FB;struct _fat_ptr*f=_tmp5FC;int*is_tagged=_tmp5FD;int*is_read=_tmp5FE;
# 2933
t=({Cyc_Tcexp_tcAggrMember(te,loc,topt,e,e1,f,is_tagged,is_read);});goto _LL0;}case 22U: _LL33: _tmp5F7=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp5D0)->f1;_tmp5F8=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp5D0)->f2;_tmp5F9=(int*)&((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp5D0)->f3;_tmp5FA=(int*)&((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp5D0)->f4;_LL34: {struct Cyc_Absyn_Exp*e1=_tmp5F7;struct _fat_ptr*f=_tmp5F8;int*is_tagged=_tmp5F9;int*is_read=_tmp5FA;
# 2935
t=({Cyc_Tcexp_tcAggrArrow(te,loc,topt,e1,f,is_tagged,is_read);});goto _LL0;}case 23U: _LL35: _tmp5F5=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp5D0)->f1;_tmp5F6=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp5D0)->f2;_LL36: {struct Cyc_Absyn_Exp*e1=_tmp5F5;struct Cyc_Absyn_Exp*e2=_tmp5F6;
# 2937
t=({Cyc_Tcexp_tcSubscript(te,loc,topt,e1,e2);});goto _LL0;}case 24U: _LL37: _tmp5F4=((struct Cyc_Absyn_Tuple_e_Absyn_Raw_exp_struct*)_tmp5D0)->f1;_LL38: {struct Cyc_List_List*es=_tmp5F4;
# 2939
t=({Cyc_Tcexp_tcTuple(te,loc,topt,es);});goto _LL0;}case 25U: _LL39: _tmp5F2=((struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct*)_tmp5D0)->f1;_tmp5F3=((struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct*)_tmp5D0)->f2;_LL3A: {struct _tuple10*t1=_tmp5F2;struct Cyc_List_List*des=_tmp5F3;
# 2941
t=({Cyc_Tcexp_tcCompoundLit(te,loc,e,topt,t1,des);});goto _LL0;}case 26U: _LL3B: _tmp5F1=((struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*)_tmp5D0)->f1;_LL3C: {struct Cyc_List_List*des=_tmp5F1;
# 2945
void**elt_topt=0;
struct Cyc_Absyn_Tqual*elt_tqopt=0;
int zero_term=0;
if(topt != 0){
void*_stmttmp58=({Cyc_Tcutil_compress(*topt);});void*_tmp633=_stmttmp58;void*_tmp636;struct Cyc_Absyn_Tqual*_tmp635;void**_tmp634;if(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp633)->tag == 4U){_LL5F: _tmp634=(void**)&(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp633)->f1).elt_type;_tmp635=(struct Cyc_Absyn_Tqual*)&(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp633)->f1).tq;_tmp636=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp633)->f1).zero_term;_LL60: {void**et=_tmp634;struct Cyc_Absyn_Tqual*etq=_tmp635;void*zt=_tmp636;
# 2951
elt_topt=et;
elt_tqopt=etq;
zero_term=({Cyc_Tcutil_force_type2bool(0,zt);});
goto _LL5E;}}else{_LL61: _LL62:
 goto _LL5E;}_LL5E:;}
# 2948
t=({Cyc_Tcexp_tcArray(te,loc,elt_topt,elt_tqopt,zero_term,des);});
# 2958
goto _LL0;}case 38U: _LL3D: _tmp5F0=((struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct*)_tmp5D0)->f1;_LL3E: {struct Cyc_Absyn_Stmt*s=_tmp5F0;
# 2960
t=({Cyc_Tcexp_tcStmtExp(te,loc,topt,s);});goto _LL0;}case 27U: _LL3F: _tmp5EC=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmp5D0)->f1;_tmp5ED=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmp5D0)->f2;_tmp5EE=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmp5D0)->f3;_tmp5EF=(int*)&((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmp5D0)->f4;_LL40: {struct Cyc_Absyn_Vardecl*vd=_tmp5EC;struct Cyc_Absyn_Exp*e1=_tmp5ED;struct Cyc_Absyn_Exp*e2=_tmp5EE;int*iszeroterm=_tmp5EF;
# 2962
t=({Cyc_Tcexp_tcComprehension(te,loc,topt,vd,e1,e2,iszeroterm);});goto _LL0;}case 28U: _LL41: _tmp5E9=((struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*)_tmp5D0)->f1;_tmp5EA=(void*)((struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*)_tmp5D0)->f2;_tmp5EB=(int*)&((struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*)_tmp5D0)->f3;_LL42: {struct Cyc_Absyn_Exp*e1=_tmp5E9;void*t1=_tmp5EA;int*iszeroterm=_tmp5EB;
# 2964
t=({Cyc_Tcexp_tcComprehensionNoinit(te,loc,topt,e1,t1,iszeroterm);});goto _LL0;}case 29U: _LL43: _tmp5E5=(struct _tuple1**)&((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_tmp5D0)->f1;_tmp5E6=(struct Cyc_List_List**)&((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_tmp5D0)->f2;_tmp5E7=((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_tmp5D0)->f3;_tmp5E8=(struct Cyc_Absyn_Aggrdecl**)&((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_tmp5D0)->f4;_LL44: {struct _tuple1**tn=_tmp5E5;struct Cyc_List_List**ts=_tmp5E6;struct Cyc_List_List*args=_tmp5E7;struct Cyc_Absyn_Aggrdecl**sd_opt=_tmp5E8;
# 2966
t=({Cyc_Tcexp_tcAggregate(te,loc,topt,tn,ts,args,sd_opt);});goto _LL0;}case 30U: _LL45: _tmp5E3=(void*)((struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct*)_tmp5D0)->f1;_tmp5E4=((struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct*)_tmp5D0)->f2;_LL46: {void*ts=_tmp5E3;struct Cyc_List_List*args=_tmp5E4;
# 2968
t=({Cyc_Tcexp_tcAnonStruct(te,loc,ts,args);});goto _LL0;}case 31U: _LL47: _tmp5E0=((struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*)_tmp5D0)->f1;_tmp5E1=((struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*)_tmp5D0)->f2;_tmp5E2=((struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*)_tmp5D0)->f3;_LL48: {struct Cyc_List_List*es=_tmp5E0;struct Cyc_Absyn_Datatypedecl*tud=_tmp5E1;struct Cyc_Absyn_Datatypefield*tuf=_tmp5E2;
# 2970
t=({Cyc_Tcexp_tcDatatype(te,loc,topt,e,es,tud,tuf);});goto _LL0;}case 32U: _LL49: _tmp5DE=((struct Cyc_Absyn_Enum_e_Absyn_Raw_exp_struct*)_tmp5D0)->f1;_tmp5DF=((struct Cyc_Absyn_Enum_e_Absyn_Raw_exp_struct*)_tmp5D0)->f2;_LL4A: {struct Cyc_Absyn_Enumdecl*ed=_tmp5DE;struct Cyc_Absyn_Enumfield*ef=_tmp5DF;
# 2972
t=({Cyc_Absyn_enum_type(ed->name,ed);});goto _LL0;}case 33U: _LL4B: _tmp5DC=(void*)((struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct*)_tmp5D0)->f1;_tmp5DD=((struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct*)_tmp5D0)->f2;_LL4C: {void*t2=_tmp5DC;struct Cyc_Absyn_Enumfield*ef=_tmp5DD;
# 2974
t=t2;goto _LL0;}case 35U: _LL4D: _tmp5D7=(int*)&(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp5D0)->f1).is_calloc;_tmp5D8=(struct Cyc_Absyn_Exp**)&(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp5D0)->f1).rgn;_tmp5D9=(void***)&(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp5D0)->f1).elt_type;_tmp5DA=(struct Cyc_Absyn_Exp**)&(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp5D0)->f1).num_elts;_tmp5DB=(int*)&(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp5D0)->f1).fat_result;_LL4E: {int*is_calloc=_tmp5D7;struct Cyc_Absyn_Exp**ropt=_tmp5D8;void***t2=_tmp5D9;struct Cyc_Absyn_Exp**e2=_tmp5DA;int*isfat=_tmp5DB;
# 2976
t=({Cyc_Tcexp_tcMalloc(te,loc,topt,ropt,t2,e2,is_calloc,isfat);});goto _LL0;}case 36U: _LL4F: _tmp5D5=((struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct*)_tmp5D0)->f1;_tmp5D6=((struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct*)_tmp5D0)->f2;_LL50: {struct Cyc_Absyn_Exp*e1=_tmp5D5;struct Cyc_Absyn_Exp*e2=_tmp5D6;
# 2978
t=({Cyc_Tcexp_tcSwap(te,loc,topt,e1,e2);});goto _LL0;}case 39U: _LL51: _tmp5D3=((struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct*)_tmp5D0)->f1;_tmp5D4=((struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct*)_tmp5D0)->f2;_LL52: {struct Cyc_Absyn_Exp*e=_tmp5D3;struct _fat_ptr*f=_tmp5D4;
# 2980
t=({Cyc_Tcexp_tcTagcheck(te,loc,topt,e,f);});goto _LL0;}case 42U: _LL53: _tmp5D2=((struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct*)_tmp5D0)->f1;_LL54: {struct Cyc_Absyn_Exp*e1=_tmp5D2;
# 2982
t=({Cyc_Tcexp_tcExp(te,topt,e1);});goto _LL0;}case 40U: _LL55: _tmp5D1=(void*)((struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct*)_tmp5D0)->f1;_LL56: {void*t2=_tmp5D1;
# 2984
if(!te->allow_valueof)
({void*_tmp637=0U;({unsigned _tmp941=e->loc;struct _fat_ptr _tmp940=({const char*_tmp638="valueof(-) can only occur within types";_tag_fat(_tmp638,sizeof(char),39U);});Cyc_Tcutil_terr(_tmp941,_tmp940,_tag_fat(_tmp637,sizeof(void*),0U));});});
# 2984
t=Cyc_Absyn_sint_type;
# 2993
goto _LL0;}default: _LL57: _LL58:
# 2996
 t=Cyc_Absyn_void_type;
goto _LL0;}_LL0:;}
# 2999
e->topt=t;}

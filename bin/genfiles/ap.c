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
 struct Cyc___cycFILE;struct Cyc_String_pa_PrintArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Int_pa_PrintArg_struct{int tag;unsigned long f1;};struct Cyc_Double_pa_PrintArg_struct{int tag;double f1;};struct Cyc_LongDouble_pa_PrintArg_struct{int tag;long double f1;};struct Cyc_ShortPtr_pa_PrintArg_struct{int tag;short*f1;};struct Cyc_IntPtr_pa_PrintArg_struct{int tag;unsigned long*f1;};struct Cyc_ShortPtr_sa_ScanfArg_struct{int tag;short*f1;};struct Cyc_UShortPtr_sa_ScanfArg_struct{int tag;unsigned short*f1;};struct Cyc_IntPtr_sa_ScanfArg_struct{int tag;int*f1;};struct Cyc_UIntPtr_sa_ScanfArg_struct{int tag;unsigned*f1;};struct Cyc_StringPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_DoublePtr_sa_ScanfArg_struct{int tag;double*f1;};struct Cyc_FloatPtr_sa_ScanfArg_struct{int tag;float*f1;};struct Cyc_CharPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};extern char Cyc_FileCloseError[15U];struct Cyc_FileCloseError_exn_struct{char*tag;};extern char Cyc_FileOpenError[14U];struct Cyc_FileOpenError_exn_struct{char*tag;struct _fat_ptr f1;};
# 300 "cycboot.h"
int isspace(int);
# 331
extern const long Cyc_long_max;extern const long Cyc_long_min;
# 22 "ctype.h"
int isspace(int);struct Cyc_Core_Opt{void*v;};extern char Cyc_Core_Invalid_argument[17U];struct Cyc_Core_Invalid_argument_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Failure[8U];struct Cyc_Core_Failure_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Impossible[11U];struct Cyc_Core_Impossible_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Not_found[10U];struct Cyc_Core_Not_found_exn_struct{char*tag;};extern char Cyc_Core_Unreachable[12U];struct Cyc_Core_Unreachable_exn_struct{char*tag;struct _fat_ptr f1;};
# 168 "core.h"
extern struct _RegionHandle*Cyc_Core_unique_region;struct Cyc_Core_DynamicRegion;struct Cyc_Core_NewDynamicRegion{struct Cyc_Core_DynamicRegion*key;};struct Cyc_Core_ThinRes{void*arr;unsigned nelts;};struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};extern char Cyc_List_List_mismatch[14U];struct Cyc_List_List_mismatch_exn_struct{char*tag;};extern char Cyc_List_Nth[4U];struct Cyc_List_Nth_exn_struct{char*tag;};
# 81 "string.h"
struct _fat_ptr Cyc__memcpy(struct _fat_ptr d,struct _fat_ptr s,unsigned long,unsigned);
# 29 "assert.h"
void*Cyc___assert_fail(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line);
# 7 "ap.h"
extern struct Cyc_AP_T*Cyc_AP_zero;
extern struct Cyc_AP_T*Cyc_AP_one;
struct Cyc_AP_T*Cyc_AP_new(long n);
struct Cyc_AP_T*Cyc_AP_fromint(long x);
# 16
struct Cyc_AP_T*Cyc_AP_add(struct Cyc_AP_T*x,struct Cyc_AP_T*y);
struct Cyc_AP_T*Cyc_AP_sub(struct Cyc_AP_T*x,struct Cyc_AP_T*y);
struct Cyc_AP_T*Cyc_AP_mul(struct Cyc_AP_T*x,struct Cyc_AP_T*y);
struct Cyc_AP_T*Cyc_AP_div(struct Cyc_AP_T*x,struct Cyc_AP_T*y);
struct Cyc_AP_T*Cyc_AP_mod(struct Cyc_AP_T*x,struct Cyc_AP_T*y);
struct Cyc_AP_T*Cyc_AP_pow(struct Cyc_AP_T*x,struct Cyc_AP_T*y,struct Cyc_AP_T*p);
struct Cyc_AP_T*Cyc_AP_addi(struct Cyc_AP_T*x,long y);
# 28
struct Cyc_AP_T*Cyc_AP_rshift(struct Cyc_AP_T*x,int s);
struct Cyc_AP_T*Cyc_AP_and(struct Cyc_AP_T*x,struct Cyc_AP_T*y);
struct Cyc_AP_T*Cyc_AP_or(struct Cyc_AP_T*x,struct Cyc_AP_T*y);
struct Cyc_AP_T*Cyc_AP_xor(struct Cyc_AP_T*x,struct Cyc_AP_T*y);
int Cyc_AP_cmp(struct Cyc_AP_T*x,struct Cyc_AP_T*y);
# 34
struct Cyc_AP_T*Cyc_AP_gcd(struct Cyc_AP_T*x,struct Cyc_AP_T*y);
# 8 "xp.h"
int Cyc_XP_add(int n,struct _fat_ptr z,struct _fat_ptr x,struct _fat_ptr y,int carry);
int Cyc_XP_sub(int n,struct _fat_ptr z,struct _fat_ptr x,struct _fat_ptr y,int borrow);
int Cyc_XP_mul(struct _fat_ptr z,int n,struct _fat_ptr x,int m,struct _fat_ptr y);
int Cyc_XP_div(int n,struct _fat_ptr q,struct _fat_ptr x,int m,struct _fat_ptr y,struct _fat_ptr r,struct _fat_ptr tmp);
int Cyc_XP_sum(int n,struct _fat_ptr z,struct _fat_ptr x,int y);
int Cyc_XP_diff(int n,struct _fat_ptr z,struct _fat_ptr x,int y);
# 17
int Cyc_XP_cmp(int n,struct _fat_ptr x,struct _fat_ptr y);
void Cyc_XP_lshift(int n,struct _fat_ptr z,int m,struct _fat_ptr x,int s,int fill);
# 20
void Cyc_XP_rshift(int n,struct _fat_ptr z,int m,struct _fat_ptr x,int s,int fill);
# 22
void Cyc_XP_and(int n,struct _fat_ptr z,struct _fat_ptr x,struct _fat_ptr y);
void Cyc_XP_or(int n,struct _fat_ptr z,struct _fat_ptr x,struct _fat_ptr y);
void Cyc_XP_xor(int n,struct _fat_ptr z,struct _fat_ptr x,struct _fat_ptr y);
int Cyc_XP_length(int n,struct _fat_ptr x);
unsigned long Cyc_XP_fromint(int n,struct _fat_ptr z,unsigned long u);
# 28
unsigned long Cyc_XP_toint(int n,struct _fat_ptr x);
int Cyc_XP_fromstr(int n,struct _fat_ptr z,const char*str,int base);
# 31
struct _fat_ptr Cyc_XP_tostr(struct _fat_ptr str,int size,int base,int n,struct _fat_ptr x);struct Cyc_AP_T{int sign;int ndigits;int size;struct _fat_ptr digits;};
# 21 "ap.cyc"
struct Cyc_AP_T*Cyc_AP_zero;
struct Cyc_AP_T*Cyc_AP_one;
int Cyc_init=0;
# 25
static struct Cyc_AP_T*Cyc_normalize(struct Cyc_AP_T*z,int n);
static int Cyc_cmp(struct Cyc_AP_T*x,struct Cyc_AP_T*y);
static void Cyc_AP_init(){
Cyc_init=1;
Cyc_AP_zero=({Cyc_AP_fromint(0);});
Cyc_AP_one=({Cyc_AP_fromint(1);});}
# 32
static struct Cyc_AP_T*Cyc_mk(int size){
struct Cyc_AP_T*z;
if(!Cyc_init)({Cyc_AP_init();});z=
_cycalloc(sizeof(struct Cyc_AP_T));
({struct _fat_ptr _tmpCD=({unsigned _tmp1=size;_tag_fat(_cyccalloc_atomic(sizeof(unsigned char),_tmp1),sizeof(unsigned char),_tmp1);});z->digits=_tmpCD;});
size > 0?0:({({int(*_tmpCF)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmp2)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmp2;});struct _fat_ptr _tmpCE=({const char*_tmp3="size > 0";_tag_fat(_tmp3,sizeof(char),9U);});_tmpCF(_tmpCE,({const char*_tmp4="ap.cyc";_tag_fat(_tmp4,sizeof(char),7U);}),37U);});});
z->sign=1;
z->size=size;
z->ndigits=1;
return z;}
# 43
static struct Cyc_AP_T*Cyc_set(struct Cyc_AP_T*z,long n){
if(n == Cyc_long_min)
({Cyc_XP_fromint(((struct Cyc_AP_T*)_check_null(z))->size,z->digits,(unsigned)Cyc_long_max + 1U);});else{
if(n < 0)
({Cyc_XP_fromint(((struct Cyc_AP_T*)_check_null(z))->size,z->digits,(unsigned long)(- n));});else{
# 49
({Cyc_XP_fromint(((struct Cyc_AP_T*)_check_null(z))->size,z->digits,(unsigned long)n);});}}
z->sign=n < 0?- 1: 1;
return({Cyc_normalize(z,z->size);});}
# 53
static struct Cyc_AP_T*Cyc_normalize(struct Cyc_AP_T*z,int n){
({int _tmpD0=({Cyc_XP_length(n,((struct Cyc_AP_T*)_check_null(z))->digits);});z->ndigits=_tmpD0;});
return z;}
# 57
static struct Cyc_AP_T*Cyc_add(struct Cyc_AP_T*z,struct Cyc_AP_T*x,struct Cyc_AP_T*y){
int n=((struct Cyc_AP_T*)_check_null(y))->ndigits;
if(((struct Cyc_AP_T*)_check_null(x))->ndigits < n)
return({Cyc_add(z,y,x);});else{
if(x->ndigits > n){
int carry=({Cyc_XP_add(n,((struct Cyc_AP_T*)_check_null(z))->digits,x->digits,y->digits,0);});
# 64
({unsigned char _tmpD4=(unsigned char)({({int _tmpD3=x->ndigits - n;struct _fat_ptr _tmpD2=
_fat_ptr_plus(z->digits,sizeof(unsigned char),n);struct _fat_ptr _tmpD1=_fat_ptr_plus(x->digits,sizeof(unsigned char),n);Cyc_XP_sum(_tmpD3,_tmpD2,_tmpD1,carry);});});
# 64
*((unsigned char*)_check_fat_subscript(z->digits,sizeof(unsigned char),z->size - 1))=_tmpD4;});}else{
# 67
({unsigned char _tmpD5=(unsigned char)({Cyc_XP_add(n,((struct Cyc_AP_T*)_check_null(z))->digits,x->digits,y->digits,0);});*((unsigned char*)_check_fat_subscript(z->digits,sizeof(unsigned char),n))=_tmpD5;});}}
# 69
return({Cyc_normalize(z,z->size);});}
# 71
static struct Cyc_AP_T*Cyc_sub(struct Cyc_AP_T*z,struct Cyc_AP_T*x,struct Cyc_AP_T*y){
int borrow;int n=((struct Cyc_AP_T*)_check_null(y))->ndigits;
borrow=({({int _tmpD8=n;struct _fat_ptr _tmpD7=((struct Cyc_AP_T*)_check_null(z))->digits;struct _fat_ptr _tmpD6=((struct Cyc_AP_T*)_check_null(x))->digits;Cyc_XP_sub(_tmpD8,_tmpD7,_tmpD6,y->digits,0);});});
# 75
if(x->ndigits > n)
borrow=({({int _tmpDB=x->ndigits - n;struct _fat_ptr _tmpDA=_fat_ptr_plus(z->digits,sizeof(unsigned char),n);struct _fat_ptr _tmpD9=
_fat_ptr_plus(x->digits,sizeof(unsigned char),n);Cyc_XP_diff(_tmpDB,_tmpDA,_tmpD9,borrow);});});
# 75

# 78
borrow == 0?0:({({int(*_tmpDD)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmp9)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmp9;});struct _fat_ptr _tmpDC=({const char*_tmpA="borrow == 0";_tag_fat(_tmpA,sizeof(char),12U);});_tmpDD(_tmpDC,({const char*_tmpB="ap.cyc";_tag_fat(_tmpB,sizeof(char),7U);}),78U);});});
return({Cyc_normalize(z,z->size);});}
# 81
static struct Cyc_AP_T*Cyc_mulmod(struct Cyc_AP_T*x,struct Cyc_AP_T*y,struct Cyc_AP_T*p){
struct Cyc_AP_T*z;struct Cyc_AP_T*xy=({Cyc_AP_mul(x,y);});
z=({Cyc_AP_mod(xy,p);});
return z;}
# 86
static int Cyc_cmp(struct Cyc_AP_T*x,struct Cyc_AP_T*y){
if(({int _tmpDE=((struct Cyc_AP_T*)_check_null(x))->ndigits;_tmpDE != ((struct Cyc_AP_T*)_check_null(y))->ndigits;}))
return x->ndigits - y->ndigits;else{
# 90
return({Cyc_XP_cmp(x->ndigits,x->digits,y->digits);});}}
# 86
struct Cyc_AP_T*Cyc_AP_new(long n){
# 93
return({struct Cyc_AP_T*(*_tmpF)(struct Cyc_AP_T*z,long n)=Cyc_set;struct Cyc_AP_T*_tmp10=({Cyc_mk((int)sizeof(long));});long _tmp11=n;_tmpF(_tmp10,_tmp11);});}
# 86
struct Cyc_AP_T*Cyc_AP_neg(struct Cyc_AP_T*x){
# 96
struct Cyc_AP_T*z;
(unsigned)x?0:({({int(*_tmpE0)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmp13)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmp13;});struct _fat_ptr _tmpDF=({const char*_tmp14="x";_tag_fat(_tmp14,sizeof(char),2U);});_tmpE0(_tmpDF,({const char*_tmp15="ap.cyc";_tag_fat(_tmp15,sizeof(char),7U);}),97U);});});
z=({Cyc_mk(x->ndigits);});
({Cyc__memcpy(((struct Cyc_AP_T*)_check_null(z))->digits,(struct _fat_ptr)x->digits,(unsigned)x->ndigits / sizeof(*((unsigned char*)(x->digits).curr))+ (unsigned)((unsigned)x->ndigits % sizeof(*((unsigned char*)(x->digits).curr))== (unsigned)0?0: 1),sizeof(*((unsigned char*)(x->digits).curr)));});
z->ndigits=x->ndigits;
z->sign=z->ndigits == 1 &&(int)*((unsigned char*)_check_fat_subscript(z->digits,sizeof(unsigned char),0))== 0?1: - x->sign;
return z;}
# 86
struct Cyc_AP_T*Cyc_AP_abs(struct Cyc_AP_T*x){
# 105
struct Cyc_AP_T*z;
(unsigned)x?0:({({int(*_tmpE2)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmp17)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmp17;});struct _fat_ptr _tmpE1=({const char*_tmp18="x";_tag_fat(_tmp18,sizeof(char),2U);});_tmpE2(_tmpE1,({const char*_tmp19="ap.cyc";_tag_fat(_tmp19,sizeof(char),7U);}),106U);});});
z=({Cyc_mk(x->ndigits);});
({Cyc__memcpy(((struct Cyc_AP_T*)_check_null(z))->digits,(struct _fat_ptr)x->digits,(unsigned)x->ndigits / sizeof(*((unsigned char*)(x->digits).curr))+ (unsigned)((unsigned)x->ndigits % sizeof(*((unsigned char*)(x->digits).curr))== (unsigned)0?0: 1),sizeof(*((unsigned char*)(x->digits).curr)));});
z->ndigits=x->ndigits;
z->sign=1;
return z;}
# 86
struct Cyc_AP_T*Cyc_AP_mul(struct Cyc_AP_T*x,struct Cyc_AP_T*y){
# 114
struct Cyc_AP_T*z;
(unsigned)x?0:({({int(*_tmpE4)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmp1B)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmp1B;});struct _fat_ptr _tmpE3=({const char*_tmp1C="x";_tag_fat(_tmp1C,sizeof(char),2U);});_tmpE4(_tmpE3,({const char*_tmp1D="ap.cyc";_tag_fat(_tmp1D,sizeof(char),7U);}),115U);});});
(unsigned)y?0:({({int(*_tmpE6)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmp1E)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmp1E;});struct _fat_ptr _tmpE5=({const char*_tmp1F="y";_tag_fat(_tmp1F,sizeof(char),2U);});_tmpE6(_tmpE5,({const char*_tmp20="ap.cyc";_tag_fat(_tmp20,sizeof(char),7U);}),116U);});});
z=({Cyc_mk(x->ndigits + y->ndigits);});
({Cyc_XP_mul(((struct Cyc_AP_T*)_check_null(z))->digits,x->ndigits,x->digits,y->ndigits,y->digits);});
# 120
({Cyc_normalize(z,z->size);});
z->sign=(z->ndigits == 1 &&(int)*((unsigned char*)_check_fat_subscript(z->digits,sizeof(unsigned char),0))== 0 ||(x->sign ^ y->sign)== 0)?1: - 1;
# 123
return z;}
# 86
struct Cyc_AP_T*Cyc_AP_add(struct Cyc_AP_T*x,struct Cyc_AP_T*y){
# 126
struct Cyc_AP_T*z;
(unsigned)x?0:({({int(*_tmpE8)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmp22)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmp22;});struct _fat_ptr _tmpE7=({const char*_tmp23="x";_tag_fat(_tmp23,sizeof(char),2U);});_tmpE8(_tmpE7,({const char*_tmp24="ap.cyc";_tag_fat(_tmp24,sizeof(char),7U);}),127U);});});
(unsigned)y?0:({({int(*_tmpEA)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmp25)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmp25;});struct _fat_ptr _tmpE9=({const char*_tmp26="y";_tag_fat(_tmp26,sizeof(char),2U);});_tmpEA(_tmpE9,({const char*_tmp27="ap.cyc";_tag_fat(_tmp27,sizeof(char),7U);}),128U);});});
if((x->sign ^ y->sign)== 0){
z=({struct Cyc_AP_T*(*_tmp28)(struct Cyc_AP_T*z,struct Cyc_AP_T*x,struct Cyc_AP_T*y)=Cyc_add;struct Cyc_AP_T*_tmp29=({Cyc_mk((x->ndigits > y->ndigits?x->ndigits: y->ndigits)+ 1);});struct Cyc_AP_T*_tmp2A=x;struct Cyc_AP_T*_tmp2B=y;_tmp28(_tmp29,_tmp2A,_tmp2B);});
({int _tmpEB=((struct Cyc_AP_T*)_check_null(z))->ndigits == 1 &&(int)*((unsigned char*)_check_fat_subscript(z->digits,sizeof(unsigned char),0))== 0?1: x->sign;z->sign=_tmpEB;});}else{
# 133
if(({Cyc_cmp(x,y);})> 0){
z=({struct Cyc_AP_T*(*_tmp2C)(struct Cyc_AP_T*z,struct Cyc_AP_T*x,struct Cyc_AP_T*y)=Cyc_sub;struct Cyc_AP_T*_tmp2D=({Cyc_mk(x->ndigits);});struct Cyc_AP_T*_tmp2E=x;struct Cyc_AP_T*_tmp2F=y;_tmp2C(_tmp2D,_tmp2E,_tmp2F);});
({int _tmpEC=((struct Cyc_AP_T*)_check_null(z))->ndigits == 1 &&(int)*((unsigned char*)_check_fat_subscript(z->digits,sizeof(unsigned char),0))== 0?1: x->sign;z->sign=_tmpEC;});}else{
# 138
z=({struct Cyc_AP_T*(*_tmp30)(struct Cyc_AP_T*z,struct Cyc_AP_T*x,struct Cyc_AP_T*y)=Cyc_sub;struct Cyc_AP_T*_tmp31=({Cyc_mk(y->ndigits);});struct Cyc_AP_T*_tmp32=y;struct Cyc_AP_T*_tmp33=x;_tmp30(_tmp31,_tmp32,_tmp33);});
({int _tmpED=((struct Cyc_AP_T*)_check_null(z))->ndigits == 1 &&(int)*((unsigned char*)_check_fat_subscript(z->digits,sizeof(unsigned char),0))== 0?1: - x->sign;z->sign=_tmpED;});}}
# 141
return z;}
# 86
struct Cyc_AP_T*Cyc_AP_sub(struct Cyc_AP_T*x,struct Cyc_AP_T*y){
# 144
struct Cyc_AP_T*z;
(unsigned)x?0:({({int(*_tmpEF)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmp35)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmp35;});struct _fat_ptr _tmpEE=({const char*_tmp36="x";_tag_fat(_tmp36,sizeof(char),2U);});_tmpEF(_tmpEE,({const char*_tmp37="ap.cyc";_tag_fat(_tmp37,sizeof(char),7U);}),145U);});});
(unsigned)y?0:({({int(*_tmpF1)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmp38)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmp38;});struct _fat_ptr _tmpF0=({const char*_tmp39="y";_tag_fat(_tmp39,sizeof(char),2U);});_tmpF1(_tmpF0,({const char*_tmp3A="ap.cyc";_tag_fat(_tmp3A,sizeof(char),7U);}),146U);});});
if(!((x->sign ^ y->sign)== 0)){
z=({struct Cyc_AP_T*(*_tmp3B)(struct Cyc_AP_T*z,struct Cyc_AP_T*x,struct Cyc_AP_T*y)=Cyc_add;struct Cyc_AP_T*_tmp3C=({Cyc_mk((x->ndigits > y->ndigits?x->ndigits: y->ndigits)+ 1);});struct Cyc_AP_T*_tmp3D=x;struct Cyc_AP_T*_tmp3E=y;_tmp3B(_tmp3C,_tmp3D,_tmp3E);});
({int _tmpF2=((struct Cyc_AP_T*)_check_null(z))->ndigits == 1 &&(int)*((unsigned char*)_check_fat_subscript(z->digits,sizeof(unsigned char),0))== 0?1: x->sign;z->sign=_tmpF2;});}else{
# 151
if(({Cyc_cmp(x,y);})> 0){
z=({struct Cyc_AP_T*(*_tmp3F)(struct Cyc_AP_T*z,struct Cyc_AP_T*x,struct Cyc_AP_T*y)=Cyc_sub;struct Cyc_AP_T*_tmp40=({Cyc_mk(x->ndigits);});struct Cyc_AP_T*_tmp41=x;struct Cyc_AP_T*_tmp42=y;_tmp3F(_tmp40,_tmp41,_tmp42);});
({int _tmpF3=((struct Cyc_AP_T*)_check_null(z))->ndigits == 1 &&(int)*((unsigned char*)_check_fat_subscript(z->digits,sizeof(unsigned char),0))== 0?1: x->sign;z->sign=_tmpF3;});}else{
# 155
z=({struct Cyc_AP_T*(*_tmp43)(struct Cyc_AP_T*z,struct Cyc_AP_T*x,struct Cyc_AP_T*y)=Cyc_sub;struct Cyc_AP_T*_tmp44=({Cyc_mk(y->ndigits);});struct Cyc_AP_T*_tmp45=y;struct Cyc_AP_T*_tmp46=x;_tmp43(_tmp44,_tmp45,_tmp46);});
({int _tmpF4=((struct Cyc_AP_T*)_check_null(z))->ndigits == 1 &&(int)*((unsigned char*)_check_fat_subscript(z->digits,sizeof(unsigned char),0))== 0?1: - x->sign;z->sign=_tmpF4;});}}
# 158
return z;}
# 86
struct Cyc_AP_T*Cyc_AP_div(struct Cyc_AP_T*x,struct Cyc_AP_T*y){
# 161
struct Cyc_AP_T*q;struct Cyc_AP_T*r;
(unsigned)x?0:({({int(*_tmpF6)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmp48)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmp48;});struct _fat_ptr _tmpF5=({const char*_tmp49="x";_tag_fat(_tmp49,sizeof(char),2U);});_tmpF6(_tmpF5,({const char*_tmp4A="ap.cyc";_tag_fat(_tmp4A,sizeof(char),7U);}),162U);});});
(unsigned)y?0:({({int(*_tmpF8)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmp4B)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmp4B;});struct _fat_ptr _tmpF7=({const char*_tmp4C="y";_tag_fat(_tmp4C,sizeof(char),2U);});_tmpF8(_tmpF7,({const char*_tmp4D="ap.cyc";_tag_fat(_tmp4D,sizeof(char),7U);}),163U);});});
!(y->ndigits == 1 &&(int)*((unsigned char*)_check_fat_subscript(y->digits,sizeof(unsigned char),0))== 0)?0:({({int(*_tmpFA)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmp4E)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmp4E;});struct _fat_ptr _tmpF9=({const char*_tmp4F="!((y)->ndigits==1 && (y)->digits[0]==0)";_tag_fat(_tmp4F,sizeof(char),40U);});_tmpFA(_tmpF9,({const char*_tmp50="ap.cyc";_tag_fat(_tmp50,sizeof(char),7U);}),164U);});});
q=({Cyc_mk(x->ndigits);});
r=({Cyc_mk(y->ndigits);});
{
struct _fat_ptr tmp=({unsigned _tmp51=(unsigned)((x->ndigits + y->ndigits)+ 2)* sizeof(unsigned char);_tag_fat(_cycalloc_atomic(_tmp51),1U,_tmp51);});
({({int _tmp100=x->ndigits;struct _fat_ptr _tmpFF=((struct Cyc_AP_T*)_check_null(q))->digits;struct _fat_ptr _tmpFE=x->digits;int _tmpFD=y->ndigits;struct _fat_ptr _tmpFC=y->digits;struct _fat_ptr _tmpFB=((struct Cyc_AP_T*)_check_null(r))->digits;Cyc_XP_div(_tmp100,_tmpFF,_tmpFE,_tmpFD,_tmpFC,_tmpFB,tmp);});});}
# 172
({Cyc_normalize(q,q->size);});
({Cyc_normalize(r,r->size);});
q->sign=(q->ndigits == 1 &&(int)*((unsigned char*)_check_fat_subscript(q->digits,sizeof(unsigned char),0))== 0 ||(x->sign ^ y->sign)== 0)?1: - 1;
# 176
return q;}
# 86
struct Cyc_AP_T*Cyc_AP_mod(struct Cyc_AP_T*x,struct Cyc_AP_T*y){
# 179
struct Cyc_AP_T*q;struct Cyc_AP_T*r;
(unsigned)x?0:({({int(*_tmp102)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmp53)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmp53;});struct _fat_ptr _tmp101=({const char*_tmp54="x";_tag_fat(_tmp54,sizeof(char),2U);});_tmp102(_tmp101,({const char*_tmp55="ap.cyc";_tag_fat(_tmp55,sizeof(char),7U);}),180U);});});
(unsigned)y?0:({({int(*_tmp104)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmp56)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmp56;});struct _fat_ptr _tmp103=({const char*_tmp57="y";_tag_fat(_tmp57,sizeof(char),2U);});_tmp104(_tmp103,({const char*_tmp58="ap.cyc";_tag_fat(_tmp58,sizeof(char),7U);}),181U);});});
!(y->ndigits == 1 &&(int)*((unsigned char*)_check_fat_subscript(y->digits,sizeof(unsigned char),0))== 0)?0:({({int(*_tmp106)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmp59)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmp59;});struct _fat_ptr _tmp105=({const char*_tmp5A="!((y)->ndigits==1 && (y)->digits[0]==0)";_tag_fat(_tmp5A,sizeof(char),40U);});_tmp106(_tmp105,({const char*_tmp5B="ap.cyc";_tag_fat(_tmp5B,sizeof(char),7U);}),182U);});});
q=({Cyc_mk(x->ndigits);});
r=({Cyc_mk(y->ndigits);});
{
struct _fat_ptr tmp=({unsigned _tmp5C=(unsigned)((x->ndigits + y->ndigits)+ 2)* sizeof(unsigned char);_tag_fat(_cycalloc_atomic(_tmp5C),1U,_tmp5C);});
({({int _tmp10C=x->ndigits;struct _fat_ptr _tmp10B=((struct Cyc_AP_T*)_check_null(q))->digits;struct _fat_ptr _tmp10A=x->digits;int _tmp109=y->ndigits;struct _fat_ptr _tmp108=y->digits;struct _fat_ptr _tmp107=((struct Cyc_AP_T*)_check_null(r))->digits;Cyc_XP_div(_tmp10C,_tmp10B,_tmp10A,_tmp109,_tmp108,_tmp107,tmp);});});}
# 190
({Cyc_normalize(q,q->size);});
({Cyc_normalize(r,r->size);});
r->sign=r->ndigits == 1 &&(int)*((unsigned char*)_check_fat_subscript(r->digits,sizeof(unsigned char),0))== 0?1: x->sign;
return r;}
# 86
struct Cyc_AP_T*Cyc_AP_pow(struct Cyc_AP_T*x,struct Cyc_AP_T*y,struct Cyc_AP_T*p){
# 196
struct Cyc_AP_T*z;
(unsigned)x?0:({({int(*_tmp10E)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmp5E)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmp5E;});struct _fat_ptr _tmp10D=({const char*_tmp5F="x";_tag_fat(_tmp5F,sizeof(char),2U);});_tmp10E(_tmp10D,({const char*_tmp60="ap.cyc";_tag_fat(_tmp60,sizeof(char),7U);}),197U);});});
(unsigned)y?0:({({int(*_tmp110)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmp61)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmp61;});struct _fat_ptr _tmp10F=({const char*_tmp62="y";_tag_fat(_tmp62,sizeof(char),2U);});_tmp110(_tmp10F,({const char*_tmp63="ap.cyc";_tag_fat(_tmp63,sizeof(char),7U);}),198U);});});
y->sign == 1?0:({({int(*_tmp112)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmp64)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmp64;});struct _fat_ptr _tmp111=({const char*_tmp65="y->sign == 1";_tag_fat(_tmp65,sizeof(char),13U);});_tmp112(_tmp111,({const char*_tmp66="ap.cyc";_tag_fat(_tmp66,sizeof(char),7U);}),199U);});});
(!((unsigned)p)||(p->sign == 1 && !(p->ndigits == 1 &&(int)*((unsigned char*)_check_fat_subscript(p->digits,sizeof(unsigned char),0))== 0))&& !(p->ndigits == 1 &&(int)*((unsigned char*)_check_fat_subscript(p->digits,sizeof(unsigned char),0))== 1))?0:({({int(*_tmp114)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmp67)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmp67;});struct _fat_ptr _tmp113=({const char*_tmp68="!p || p->sign==1 && !((p)->ndigits==1 && (p)->digits[0]==0) && !((p)->ndigits==1 && (p)->digits[0]==1)";_tag_fat(_tmp68,sizeof(char),103U);});_tmp114(_tmp113,({const char*_tmp69="ap.cyc";_tag_fat(_tmp69,sizeof(char),7U);}),200U);});});
if(x->ndigits == 1 &&(int)*((unsigned char*)_check_fat_subscript(x->digits,sizeof(unsigned char),0))== 0)
return({Cyc_AP_new(0);});
# 201
if(
# 203
y->ndigits == 1 &&(int)*((unsigned char*)_check_fat_subscript(y->digits,sizeof(unsigned char),0))== 0)
return({Cyc_AP_new(1);});
# 201
if(
# 205
x->ndigits == 1 &&(int)*((unsigned char*)_check_fat_subscript(x->digits,sizeof(unsigned char),0))== 1)
return({Cyc_AP_new(((int)*((unsigned char*)_check_fat_subscript(y->digits,sizeof(unsigned char),0))& 1)== 0?1: x->sign);});
# 201
if((unsigned)p){
# 208
if(y->ndigits == 1 &&(int)*((unsigned char*)_check_fat_subscript(y->digits,sizeof(unsigned char),0))== 1)
z=({Cyc_AP_mod(x,p);});else{
# 211
struct Cyc_AP_T*y2=({Cyc_AP_rshift(y,1);});struct Cyc_AP_T*t=({Cyc_AP_pow(x,y2,p);});
z=({Cyc_mulmod(t,t,p);});
if(!(((int)*((unsigned char*)_check_fat_subscript(y->digits,sizeof(unsigned char),0))& 1)== 0))
z=({struct Cyc_AP_T*(*_tmp6A)(struct Cyc_AP_T*x,struct Cyc_AP_T*y,struct Cyc_AP_T*p)=Cyc_mulmod;struct Cyc_AP_T*_tmp6B=y2=({Cyc_AP_mod(x,p);});struct Cyc_AP_T*_tmp6C=t=z;struct Cyc_AP_T*_tmp6D=p;_tmp6A(_tmp6B,_tmp6C,_tmp6D);});}}else{
# 218
if(y->ndigits == 1 &&(int)*((unsigned char*)_check_fat_subscript(y->digits,sizeof(unsigned char),0))== 1)
z=({Cyc_AP_addi(x,0);});else{
# 221
struct Cyc_AP_T*y2=({Cyc_AP_rshift(y,1);});struct Cyc_AP_T*t=({Cyc_AP_pow(x,y2,0);});
z=({Cyc_AP_mul(t,t);});
if(!(((int)*((unsigned char*)_check_fat_subscript(y->digits,sizeof(unsigned char),0))& 1)== 0))
z=({struct Cyc_AP_T*(*_tmp6E)(struct Cyc_AP_T*x,struct Cyc_AP_T*y)=Cyc_AP_mul;struct Cyc_AP_T*_tmp6F=x;struct Cyc_AP_T*_tmp70=t=z;_tmp6E(_tmp6F,_tmp70);});}}
# 227
return z;}
# 229
int Cyc_AP_cmp(struct Cyc_AP_T*x,struct Cyc_AP_T*y){
(unsigned)x?0:({({int(*_tmp116)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmp72)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmp72;});struct _fat_ptr _tmp115=({const char*_tmp73="x";_tag_fat(_tmp73,sizeof(char),2U);});_tmp116(_tmp115,({const char*_tmp74="ap.cyc";_tag_fat(_tmp74,sizeof(char),7U);}),230U);});});
(unsigned)y?0:({({int(*_tmp118)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmp75)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmp75;});struct _fat_ptr _tmp117=({const char*_tmp76="y";_tag_fat(_tmp76,sizeof(char),2U);});_tmp118(_tmp117,({const char*_tmp77="ap.cyc";_tag_fat(_tmp77,sizeof(char),7U);}),231U);});});
if(!((x->sign ^ y->sign)== 0))
return x->sign;else{
if(x->sign == 1)
return({Cyc_cmp(x,y);});else{
# 237
return({Cyc_cmp(y,x);});}}}
# 229
struct Cyc_AP_T*Cyc_AP_addi(struct Cyc_AP_T*x,long y){
# 240
struct Cyc_AP_T*t=({Cyc_mk((int)(sizeof(unsigned long)/ sizeof(unsigned char)));});
return({struct Cyc_AP_T*(*_tmp79)(struct Cyc_AP_T*x,struct Cyc_AP_T*y)=Cyc_AP_add;struct Cyc_AP_T*_tmp7A=x;struct Cyc_AP_T*_tmp7B=({Cyc_set(t,y);});_tmp79(_tmp7A,_tmp7B);});}
# 229
struct Cyc_AP_T*Cyc_AP_subi(struct Cyc_AP_T*x,long y){
# 244
struct Cyc_AP_T*t=({Cyc_mk((int)(sizeof(unsigned long)/ sizeof(unsigned char)));});
return({struct Cyc_AP_T*(*_tmp7D)(struct Cyc_AP_T*x,struct Cyc_AP_T*y)=Cyc_AP_sub;struct Cyc_AP_T*_tmp7E=x;struct Cyc_AP_T*_tmp7F=({Cyc_set(t,y);});_tmp7D(_tmp7E,_tmp7F);});}
# 229
struct Cyc_AP_T*Cyc_AP_muli(struct Cyc_AP_T*x,long y){
# 248
struct Cyc_AP_T*t=({Cyc_mk((int)(sizeof(unsigned long)/ sizeof(unsigned char)));});
return({struct Cyc_AP_T*(*_tmp81)(struct Cyc_AP_T*x,struct Cyc_AP_T*y)=Cyc_AP_mul;struct Cyc_AP_T*_tmp82=x;struct Cyc_AP_T*_tmp83=({Cyc_set(t,y);});_tmp81(_tmp82,_tmp83);});}
# 229
struct Cyc_AP_T*Cyc_AP_divi(struct Cyc_AP_T*x,long y){
# 252
struct Cyc_AP_T*t=({Cyc_mk((int)(sizeof(unsigned long)/ sizeof(unsigned char)));});
return({struct Cyc_AP_T*(*_tmp85)(struct Cyc_AP_T*x,struct Cyc_AP_T*y)=Cyc_AP_div;struct Cyc_AP_T*_tmp86=x;struct Cyc_AP_T*_tmp87=({Cyc_set(t,y);});_tmp85(_tmp86,_tmp87);});}
# 255
int Cyc_AP_cmpi(struct Cyc_AP_T*x,long y){
struct Cyc_AP_T*t=({Cyc_mk((int)(sizeof(unsigned long)/ sizeof(unsigned char)));});
return({int(*_tmp89)(struct Cyc_AP_T*x,struct Cyc_AP_T*y)=Cyc_AP_cmp;struct Cyc_AP_T*_tmp8A=x;struct Cyc_AP_T*_tmp8B=({Cyc_set(t,y);});_tmp89(_tmp8A,_tmp8B);});}
# 259
long Cyc_AP_modi(struct Cyc_AP_T*x,long y){
long rem;
struct Cyc_AP_T*r;
struct Cyc_AP_T*t=({Cyc_mk((int)(sizeof(unsigned long)/ sizeof(unsigned char)));});
r=({struct Cyc_AP_T*(*_tmp8D)(struct Cyc_AP_T*x,struct Cyc_AP_T*y)=Cyc_AP_mod;struct Cyc_AP_T*_tmp8E=x;struct Cyc_AP_T*_tmp8F=({Cyc_set(t,y);});_tmp8D(_tmp8E,_tmp8F);});
rem=(long)({Cyc_XP_toint(((struct Cyc_AP_T*)_check_null(r))->ndigits,r->digits);});
return rem;}
# 259
struct Cyc_AP_T*Cyc_AP_lshift(struct Cyc_AP_T*x,int s){
# 268
struct Cyc_AP_T*z;
(unsigned)x?0:({({int(*_tmp11A)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmp91)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmp91;});struct _fat_ptr _tmp119=({const char*_tmp92="x";_tag_fat(_tmp92,sizeof(char),2U);});_tmp11A(_tmp119,({const char*_tmp93="ap.cyc";_tag_fat(_tmp93,sizeof(char),7U);}),269U);});});
s >= 0?0:({({int(*_tmp11C)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmp94)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmp94;});struct _fat_ptr _tmp11B=({const char*_tmp95="s >= 0";_tag_fat(_tmp95,sizeof(char),7U);});_tmp11C(_tmp11B,({const char*_tmp96="ap.cyc";_tag_fat(_tmp96,sizeof(char),7U);}),270U);});});
z=({Cyc_mk(x->ndigits + (s + 7 & ~ 7)/ 8);});
({Cyc_XP_lshift(((struct Cyc_AP_T*)_check_null(z))->size,z->digits,x->ndigits,x->digits,s,0);});
# 274
z->sign=x->sign;
return({Cyc_normalize(z,z->size);});}
# 259
struct Cyc_AP_T*Cyc_AP_rshift(struct Cyc_AP_T*x,int s){
# 278
(unsigned)x?0:({({int(*_tmp11E)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmp98)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmp98;});struct _fat_ptr _tmp11D=({const char*_tmp99="x";_tag_fat(_tmp99,sizeof(char),2U);});_tmp11E(_tmp11D,({const char*_tmp9A="ap.cyc";_tag_fat(_tmp9A,sizeof(char),7U);}),278U);});});
s >= 0?0:({({int(*_tmp120)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmp9B)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmp9B;});struct _fat_ptr _tmp11F=({const char*_tmp9C="s >= 0";_tag_fat(_tmp9C,sizeof(char),7U);});_tmp120(_tmp11F,({const char*_tmp9D="ap.cyc";_tag_fat(_tmp9D,sizeof(char),7U);}),279U);});});
if(s >= 8 * x->ndigits)
return({Cyc_AP_new(0);});else{
# 283
struct Cyc_AP_T*z=({Cyc_mk(x->ndigits - s / 8);});
({Cyc_XP_rshift(((struct Cyc_AP_T*)_check_null(z))->size,z->digits,x->ndigits,x->digits,s,0);});
# 286
({Cyc_normalize(z,z->size);});
z->sign=z->ndigits == 1 &&(int)*((unsigned char*)_check_fat_subscript(z->digits,sizeof(unsigned char),0))== 0?1: x->sign;
return z;}}
# 259
struct Cyc_AP_T*Cyc_AP_and(struct Cyc_AP_T*x,struct Cyc_AP_T*y){
# 292
int i;
struct Cyc_AP_T*z;
if(({int _tmp121=((struct Cyc_AP_T*)_check_null(x))->ndigits;_tmp121 < ((struct Cyc_AP_T*)_check_null(y))->ndigits;}))
return({Cyc_AP_and(y,x);});
# 294
z=({Cyc_mk(y->ndigits);});
# 297
({Cyc_XP_and(y->ndigits,((struct Cyc_AP_T*)_check_null(z))->digits,x->digits,y->digits);});
({Cyc_normalize(z,z->size);});
z->sign=1;
return z;}
# 259
struct Cyc_AP_T*Cyc_AP_or(struct Cyc_AP_T*x,struct Cyc_AP_T*y){
# 303
int i;
struct Cyc_AP_T*z;
if(({int _tmp122=((struct Cyc_AP_T*)_check_null(x))->ndigits;_tmp122 < ((struct Cyc_AP_T*)_check_null(y))->ndigits;}))
return({Cyc_AP_or(y,x);});
# 305
z=({Cyc_mk(x->ndigits);});
# 308
({Cyc_XP_or(y->ndigits,((struct Cyc_AP_T*)_check_null(z))->digits,x->digits,y->digits);});
for(i=y->ndigits;i < x->ndigits;++ i){
({Cyc_normalize(z,z->size);});}
z->sign=1;
return z;}
# 259
struct Cyc_AP_T*Cyc_AP_xor(struct Cyc_AP_T*x,struct Cyc_AP_T*y){
# 315
int i;
struct Cyc_AP_T*z;
if(({int _tmp123=((struct Cyc_AP_T*)_check_null(x))->ndigits;_tmp123 < ((struct Cyc_AP_T*)_check_null(y))->ndigits;}))
return({Cyc_AP_xor(y,x);});
# 317
z=({Cyc_mk(x->ndigits);});
# 320
({Cyc_XP_xor(y->ndigits,((struct Cyc_AP_T*)_check_null(z))->digits,x->digits,y->digits);});
for(i=y->ndigits;i < x->ndigits;++ i){
({Cyc_normalize(z,z->size);});}
z->sign=1;
return z;}
# 259
struct Cyc_AP_T*Cyc_AP_fromint(long x){
# 327
struct Cyc_AP_T*t=({Cyc_mk((int)(sizeof(unsigned long)/ sizeof(unsigned char)));});
return({Cyc_set(t,x);});}
# 330
long Cyc_AP_toint(struct Cyc_AP_T*x){
unsigned long u;
(unsigned)x?0:({({int(*_tmp125)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmpA3)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmpA3;});struct _fat_ptr _tmp124=({const char*_tmpA4="x";_tag_fat(_tmpA4,sizeof(char),2U);});_tmp125(_tmp124,({const char*_tmpA5="ap.cyc";_tag_fat(_tmpA5,sizeof(char),7U);}),332U);});});
u=({unsigned long _tmp126=({Cyc_XP_toint(x->ndigits,x->digits);});_tmp126 % ((unsigned)Cyc_long_max + 1U);});
if(x->sign == - 1)
return -(long)u;else{
# 337
return(long)u;}}
# 330
struct Cyc_AP_T*Cyc_AP_fromstr(const char*str,int base){
# 340
struct Cyc_AP_T*z;
struct _fat_ptr p=({const char*_tmpB1=str;_tag_fat(_tmpB1,sizeof(char),_get_zero_arr_size_char((void*)_tmpB1,1U));});
char sign='\000';
int carry;
(unsigned)p.curr?0:({({int(*_tmp128)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmpA7)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmpA7;});struct _fat_ptr _tmp127=({const char*_tmpA8="p";_tag_fat(_tmpA8,sizeof(char),2U);});_tmp128(_tmp127,({const char*_tmpA9="ap.cyc";_tag_fat(_tmpA9,sizeof(char),7U);}),344U);});});
base >= 2 && base <= 36?0:({({int(*_tmp12A)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmpAA)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmpAA;});struct _fat_ptr _tmp129=({const char*_tmpAB="base >= 2 && base <= 36";_tag_fat(_tmpAB,sizeof(char),24U);});_tmp12A(_tmp129,({const char*_tmpAC="ap.cyc";_tag_fat(_tmpAC,sizeof(char),7U);}),345U);});});
while((int)*((const char*)_check_fat_subscript(p,sizeof(char),0U))&&({isspace((int)*((const char*)_check_fat_subscript(p,sizeof(char),0U)));})){
_fat_ptr_inplace_plus(& p,sizeof(char),1);}
if((int)*((const char*)_check_fat_subscript(p,sizeof(char),0U))== (int)'-' ||(int)*((const char*)_check_fat_subscript(p,sizeof(char),0U))== (int)'+')
sign=*((const char*)_check_fat_subscript(_fat_ptr_inplace_plus_post(& p,sizeof(char),1),sizeof(char),0U));
# 348
{
# 351
const char*start;
int k;int n=0;
for(0;(int)*((const char*)_check_fat_subscript(p,sizeof(char),0U))== (int)'0' &&(int)*((const char*)_check_fat_subscript(p,sizeof(char),1))== (int)'0';_fat_ptr_inplace_plus(& p,sizeof(char),1)){
;}
start=(const char*)_untag_fat_ptr(p,sizeof(char),1U);
for(0;(((int)'0' <= (int)*((const char*)_check_fat_subscript(p,sizeof(char),0U))&&(int)*((const char*)_check_fat_subscript(p,sizeof(char),0U))<= (int)'9')&&(int)*((const char*)_check_fat_subscript(p,sizeof(char),0U))< (int)'0' + base ||
((int)'a' <= (int)*((const char*)_check_fat_subscript(p,sizeof(char),0U))&&(int)*((const char*)_check_fat_subscript(p,sizeof(char),0U))<= (int)'z')&&(int)*((const char*)_check_fat_subscript(p,sizeof(char),0U))< ((int)'a' + base)- 10)||
((int)'A' <= (int)*((const char*)_check_fat_subscript(p,sizeof(char),0U))&&(int)*((const char*)_check_fat_subscript(p,sizeof(char),0U))<= (int)'Z')&&(int)*((const char*)_check_fat_subscript(p,sizeof(char),0U))< ((int)'A' + base)- 10;_fat_ptr_inplace_plus(& p,sizeof(char),1)){
++ n;}
for(k=1;1 << k < base;++ k){
;}
z=({Cyc_mk((k * n + 7 & ~ 7)/ 8);});
p=({const char*_tmpAD=start;_tag_fat(_tmpAD,sizeof(char),_get_zero_arr_size_char((void*)_tmpAD,1U));});}
# 365
carry=({({int _tmp12D=((struct Cyc_AP_T*)_check_null(z))->size;struct _fat_ptr _tmp12C=z->digits;const char*_tmp12B=(const char*)_untag_fat_ptr(p,sizeof(char),1U);Cyc_XP_fromstr(_tmp12D,_tmp12C,_tmp12B,base);});});
# 367
carry == 0?0:({({int(*_tmp12F)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmpAE)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmpAE;});struct _fat_ptr _tmp12E=({const char*_tmpAF="carry == 0";_tag_fat(_tmpAF,sizeof(char),11U);});_tmp12F(_tmp12E,({const char*_tmpB0="ap.cyc";_tag_fat(_tmpB0,sizeof(char),7U);}),367U);});});
({Cyc_normalize(z,z->size);});
z->sign=(z->ndigits == 1 &&(int)*((unsigned char*)_check_fat_subscript(z->digits,sizeof(unsigned char),0))== 0 ||(int)sign != (int)'-')?1: - 1;
return z;}
# 372
char*Cyc_AP_tostr(struct Cyc_AP_T*x,int base){
struct _fat_ptr q;
struct _fat_ptr str;
int size;int k;
(unsigned)x?0:({({int(*_tmp131)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmpB3)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmpB3;});struct _fat_ptr _tmp130=({const char*_tmpB4="x";_tag_fat(_tmpB4,sizeof(char),2U);});_tmp131(_tmp130,({const char*_tmpB5="ap.cyc";_tag_fat(_tmpB5,sizeof(char),7U);}),376U);});});
base >= 2 && base <= 36?0:({({int(*_tmp133)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmpB6)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmpB6;});struct _fat_ptr _tmp132=({const char*_tmpB7="base >= 2 && base <= 36";_tag_fat(_tmpB7,sizeof(char),24U);});_tmp133(_tmp132,({const char*_tmpB8="ap.cyc";_tag_fat(_tmpB8,sizeof(char),7U);}),377U);});});
for(k=5;1 << k > base;-- k){
;}
size=((8 * x->ndigits)/ k + 1)+ 1;
if(x->sign == - 1)
++ size;
# 381
str=({unsigned _tmpB9=size;_tag_fat(_cyccalloc_atomic(sizeof(char),_tmpB9),sizeof(char),_tmpB9);});
# 384
q=({unsigned _tmpBA=(unsigned)x->ndigits * sizeof(unsigned char);_tag_fat(_cycalloc_atomic(_tmpBA),1U,_tmpBA);});
({Cyc__memcpy(q,(struct _fat_ptr)x->digits,(unsigned)x->ndigits / sizeof(*((unsigned char*)(x->digits).curr))+ (unsigned)((unsigned)x->ndigits % sizeof(*((unsigned char*)(x->digits).curr))== (unsigned)0?0: 1),sizeof(*((unsigned char*)(x->digits).curr)));});
if(x->sign == - 1){
({struct _fat_ptr _tmpBB=_fat_ptr_plus(str,sizeof(char),0);char _tmpBC=*((char*)_check_fat_subscript(_tmpBB,sizeof(char),0U));char _tmpBD='-';if(_get_fat_size(_tmpBB,sizeof(char))== 1U &&(_tmpBC == 0 && _tmpBD != 0))_throw_arraybounds();*((char*)_tmpBB.curr)=_tmpBD;});
({({struct _fat_ptr _tmp137=_fat_ptr_plus(str,sizeof(char),1);int _tmp136=size - 1;int _tmp135=base;int _tmp134=x->ndigits;Cyc_XP_tostr(_tmp137,_tmp136,_tmp135,_tmp134,q);});});}else{
# 390
({Cyc_XP_tostr(str,size,base,x->ndigits,q);});}
return(char*)_untag_fat_ptr(str,sizeof(char),1U);}
# 372
struct Cyc_AP_T*Cyc_AP_gcd(struct Cyc_AP_T*x,struct Cyc_AP_T*y){
# 394
struct Cyc_AP_T*z;struct Cyc_AP_T*tmp;
(unsigned)x?0:({({int(*_tmp139)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmpBF)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmpBF;});struct _fat_ptr _tmp138=({const char*_tmpC0="x";_tag_fat(_tmpC0,sizeof(char),2U);});_tmp139(_tmp138,({const char*_tmpC1="ap.cyc";_tag_fat(_tmpC1,sizeof(char),7U);}),395U);});});
(unsigned)y?0:({({int(*_tmp13B)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmpC2)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmpC2;});struct _fat_ptr _tmp13A=({const char*_tmpC3="y";_tag_fat(_tmpC3,sizeof(char),2U);});_tmp13B(_tmp13A,({const char*_tmpC4="ap.cyc";_tag_fat(_tmpC4,sizeof(char),7U);}),396U);});});
while(!(((struct Cyc_AP_T*)_check_null(y))->ndigits == 1 &&(int)*((unsigned char*)_check_fat_subscript(y->digits,sizeof(unsigned char),0))== 0)){
tmp=({Cyc_AP_mod(x,y);});
x=y;
y=tmp;}
# 402
z=({Cyc_mk(x->ndigits);});
({Cyc__memcpy(((struct Cyc_AP_T*)_check_null(z))->digits,(struct _fat_ptr)x->digits,(unsigned)x->ndigits / sizeof(*((unsigned char*)(x->digits).curr))+ (unsigned)((unsigned)x->ndigits % sizeof(*((unsigned char*)(x->digits).curr))== (unsigned)0?0: 1),sizeof(*((unsigned char*)(x->digits).curr)));});
z->ndigits=x->ndigits;
z->sign=x->sign;
return z;}
# 372
struct Cyc_AP_T*Cyc_AP_lcm(struct Cyc_AP_T*x,struct Cyc_AP_T*y){
# 409
return({struct Cyc_AP_T*(*_tmpC6)(struct Cyc_AP_T*x,struct Cyc_AP_T*y)=Cyc_AP_mul;struct Cyc_AP_T*_tmpC7=x;struct Cyc_AP_T*_tmpC8=({struct Cyc_AP_T*(*_tmpC9)(struct Cyc_AP_T*x,struct Cyc_AP_T*y)=Cyc_AP_div;struct Cyc_AP_T*_tmpCA=y;struct Cyc_AP_T*_tmpCB=({Cyc_AP_gcd(x,y);});_tmpC9(_tmpCA,_tmpCB);});_tmpC6(_tmpC7,_tmpC8);});}

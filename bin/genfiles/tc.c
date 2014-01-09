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
extern struct _RegionHandle*Cyc_Core_unique_region;struct Cyc_Core_DynamicRegion;struct Cyc_Core_NewDynamicRegion{struct Cyc_Core_DynamicRegion*key;};struct Cyc_Core_ThinRes{void*arr;unsigned nelts;};struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};
# 61 "list.h"
int Cyc_List_length(struct Cyc_List_List*x);extern char Cyc_List_List_mismatch[14U];struct Cyc_List_List_mismatch_exn_struct{char*tag;};
# 184
struct Cyc_List_List*Cyc_List_append(struct Cyc_List_List*x,struct Cyc_List_List*y);
# 190
struct Cyc_List_List*Cyc_List_rappend(struct _RegionHandle*,struct Cyc_List_List*x,struct Cyc_List_List*y);extern char Cyc_List_Nth[4U];struct Cyc_List_Nth_exn_struct{char*tag;};
# 242
void*Cyc_List_nth(struct Cyc_List_List*x,int n);
# 319
int Cyc_List_memq(struct Cyc_List_List*l,void*x);
# 322
int Cyc_List_mem(int(*compare)(void*,void*),struct Cyc_List_List*l,void*x);
# 394
struct Cyc_List_List*Cyc_List_filter_c(int(*f)(void*,void*),void*env,struct Cyc_List_List*x);struct Cyc___cycFILE;
# 53 "cycboot.h"
extern struct Cyc___cycFILE*Cyc_stderr;struct Cyc_String_pa_PrintArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Int_pa_PrintArg_struct{int tag;unsigned long f1;};struct Cyc_Double_pa_PrintArg_struct{int tag;double f1;};struct Cyc_LongDouble_pa_PrintArg_struct{int tag;long double f1;};struct Cyc_ShortPtr_pa_PrintArg_struct{int tag;short*f1;};struct Cyc_IntPtr_pa_PrintArg_struct{int tag;unsigned long*f1;};
# 88
int Cyc_fflush(struct Cyc___cycFILE*);
# 100
int Cyc_fprintf(struct Cyc___cycFILE*,struct _fat_ptr,struct _fat_ptr);struct Cyc_ShortPtr_sa_ScanfArg_struct{int tag;short*f1;};struct Cyc_UShortPtr_sa_ScanfArg_struct{int tag;unsigned short*f1;};struct Cyc_IntPtr_sa_ScanfArg_struct{int tag;int*f1;};struct Cyc_UIntPtr_sa_ScanfArg_struct{int tag;unsigned*f1;};struct Cyc_StringPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_DoublePtr_sa_ScanfArg_struct{int tag;double*f1;};struct Cyc_FloatPtr_sa_ScanfArg_struct{int tag;float*f1;};struct Cyc_CharPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};extern char Cyc_FileCloseError[15U];struct Cyc_FileCloseError_exn_struct{char*tag;};extern char Cyc_FileOpenError[14U];struct Cyc_FileOpenError_exn_struct{char*tag;struct _fat_ptr f1;};
# 38 "string.h"
unsigned long Cyc_strlen(struct _fat_ptr s);
# 49 "string.h"
int Cyc_strcmp(struct _fat_ptr s1,struct _fat_ptr s2);
int Cyc_strptrcmp(struct _fat_ptr*s1,struct _fat_ptr*s2);struct Cyc_PP_Ppstate;struct Cyc_PP_Out;struct Cyc_PP_Doc;struct Cyc_Position_Error;struct Cyc_Iter_Iter{void*env;int(*next)(void*env,void*dest);};struct Cyc_Dict_T;struct Cyc_Dict_Dict{int(*rel)(void*,void*);struct _RegionHandle*r;const struct Cyc_Dict_T*t;};extern char Cyc_Dict_Present[8U];struct Cyc_Dict_Present_exn_struct{char*tag;};extern char Cyc_Dict_Absent[7U];struct Cyc_Dict_Absent_exn_struct{char*tag;};
# 83 "dict.h"
int Cyc_Dict_member(struct Cyc_Dict_Dict d,void*k);
# 87
struct Cyc_Dict_Dict Cyc_Dict_insert(struct Cyc_Dict_Dict d,void*k,void*v);
# 110
void*Cyc_Dict_lookup(struct Cyc_Dict_Dict d,void*k);
# 122 "dict.h"
void**Cyc_Dict_lookup_opt(struct Cyc_Dict_Dict d,void*k);struct Cyc_Relations_Reln;struct _union_Nmspace_Rel_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Abs_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_C_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Loc_n{int tag;int val;};union Cyc_Absyn_Nmspace{struct _union_Nmspace_Rel_n Rel_n;struct _union_Nmspace_Abs_n Abs_n;struct _union_Nmspace_C_n C_n;struct _union_Nmspace_Loc_n Loc_n;};
# 113 "absyn.h"
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
# 962
struct Cyc_Absyn_Tqual Cyc_Absyn_const_tqual(unsigned);
struct Cyc_Absyn_Tqual Cyc_Absyn_empty_tqual(unsigned);
# 968
void*Cyc_Absyn_compress_kb(void*);
# 979
void*Cyc_Absyn_new_evar(struct Cyc_Core_Opt*k,struct Cyc_Core_Opt*tenv);
# 1019
void*Cyc_Absyn_string_type(void*rgn);
void*Cyc_Absyn_const_string_type(void*rgn);
# 1042
void*Cyc_Absyn_fatptr_type(void*t,void*rgn,struct Cyc_Absyn_Tqual,void*zeroterm);
# 1050
void*Cyc_Absyn_array_type(void*elt_type,struct Cyc_Absyn_Tqual tq,struct Cyc_Absyn_Exp*num_elts,void*zero_term,unsigned ztloc);
# 1068
struct Cyc_Absyn_Exp*Cyc_Absyn_uint_exp(unsigned,unsigned);
# 1124
struct _tuple0*Cyc_Absyn_uniquergn_qvar();
# 1213
struct _fat_ptr Cyc_Absyn_attribute2string(void*);
# 1284
void Cyc_Absyn_set_debug(int);
int Cyc_Absyn_get_debug();
# 1347
struct _fat_ptr Cyc_Absyn_effect2string(struct Cyc_List_List*f);
# 1399
int Cyc_Absyn_is_reentrant(int);struct Cyc_Absynpp_Params{int expand_typedefs;int qvar_to_Cids;int add_cyc_prefix;int to_VC;int decls_first;int rewrite_temp_tvars;int print_all_tvars;int print_all_kinds;int print_all_effects;int print_using_stmts;int print_externC_stmts;int print_full_evars;int print_zeroterm;int generate_line_directives;int use_curr_namespace;struct Cyc_List_List*curr_namespace;};
# 53 "absynpp.h"
void Cyc_Absynpp_set_params(struct Cyc_Absynpp_Params*fs);
# 55
extern struct Cyc_Absynpp_Params Cyc_Absynpp_tc_params_r;
# 62
struct _fat_ptr Cyc_Absynpp_typ2string(void*);
# 64
struct _fat_ptr Cyc_Absynpp_kind2string(struct Cyc_Absyn_Kind*);
# 69
struct _fat_ptr Cyc_Absynpp_qvar2string(struct _tuple0*);
# 27 "unify.h"
void Cyc_Unify_explain_failure();
# 29
int Cyc_Unify_unify(void*,void*);struct _union_RelnOp_RConst{int tag;unsigned val;};struct _union_RelnOp_RVar{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RNumelts{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RType{int tag;void*val;};struct _union_RelnOp_RParam{int tag;unsigned val;};struct _union_RelnOp_RParamNumelts{int tag;unsigned val;};struct _union_RelnOp_RReturn{int tag;unsigned val;};union Cyc_Relations_RelnOp{struct _union_RelnOp_RConst RConst;struct _union_RelnOp_RVar RVar;struct _union_RelnOp_RNumelts RNumelts;struct _union_RelnOp_RType RType;struct _union_RelnOp_RParam RParam;struct _union_RelnOp_RParamNumelts RParamNumelts;struct _union_RelnOp_RReturn RReturn;};
# 50 "relations-ap.h"
enum Cyc_Relations_Relation{Cyc_Relations_Req =0U,Cyc_Relations_Rneq =1U,Cyc_Relations_Rlte =2U,Cyc_Relations_Rlt =3U};struct Cyc_Relations_Reln{union Cyc_Relations_RelnOp rop1;enum Cyc_Relations_Relation relation;union Cyc_Relations_RelnOp rop2;};
# 84
struct Cyc_List_List*Cyc_Relations_exp2relns(struct _RegionHandle*r,struct Cyc_Absyn_Exp*e);
# 129
int Cyc_Relations_consistent_relations(struct Cyc_List_List*rlns);struct Cyc_RgnOrder_RgnPO;extern char Cyc_Tcenv_Env_error[10U];struct Cyc_Tcenv_Env_error_exn_struct{char*tag;};struct Cyc_Tcenv_Genv{struct Cyc_Dict_Dict aggrdecls;struct Cyc_Dict_Dict datatypedecls;struct Cyc_Dict_Dict enumdecls;struct Cyc_Dict_Dict typedefs;struct Cyc_Dict_Dict ordinaries;};struct Cyc_Tcenv_Fenv;struct Cyc_Tcenv_Tenv{struct Cyc_List_List*ns;struct Cyc_Tcenv_Genv*ae;struct Cyc_Tcenv_Fenv*le;int allow_valueof: 1;int in_extern_c_include: 1;int in_tempest: 1;int tempest_generalize: 1;};
# 69 "tcenv.h"
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_new_fenv_in_env(unsigned loc,struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Fndecl*fd,void*);
# 80
struct Cyc_Absyn_Datatypedecl***Cyc_Tcenv_lookup_xdatatypedecl(struct _RegionHandle*,struct Cyc_Tcenv_Tenv*,unsigned,struct _tuple0*);
# 84
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_allow_valueof(struct Cyc_Tcenv_Tenv*);
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_enter_extern_c_include(struct Cyc_Tcenv_Tenv*);
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_enter_tempest(struct Cyc_Tcenv_Tenv*);
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_clear_tempest(struct Cyc_Tcenv_Tenv*);
int Cyc_Tcenv_in_tempest(struct Cyc_Tcenv_Tenv*);
# 90
enum Cyc_Tcenv_NewStatus{Cyc_Tcenv_NoneNew =0U,Cyc_Tcenv_InNew =1U,Cyc_Tcenv_InNewAggr =2U};
# 30 "tcutil.h"
void*Cyc_Tcutil_impos(struct _fat_ptr,struct _fat_ptr);
void Cyc_Tcutil_terr(unsigned,struct _fat_ptr,struct _fat_ptr);
void Cyc_Tcutil_warn(unsigned,struct _fat_ptr,struct _fat_ptr);
# 46
int Cyc_Tcutil_is_function_type(void*);
# 48
int Cyc_Tcutil_is_array_type(void*);
# 55
int Cyc_Tcutil_is_bound_one(void*);
# 60
int Cyc_Tcutil_is_bits_only_type(void*);
# 90
int Cyc_Tcutil_is_integral(struct Cyc_Absyn_Exp*);
# 110
void*Cyc_Tcutil_compress(void*);
# 117
int Cyc_Tcutil_coerce_assign(struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Exp*,void*);
# 139
extern struct Cyc_Absyn_Kind Cyc_Tcutil_rk;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_ak;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_bk;
# 143
extern struct Cyc_Absyn_Kind Cyc_Tcutil_ek;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_ik;
# 148
extern struct Cyc_Absyn_Kind Cyc_Tcutil_trk;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_tak;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_tbk;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_tmk;
# 178
void*Cyc_Tcutil_kind_to_bound(struct Cyc_Absyn_Kind*k);
# 216
void Cyc_Tcutil_check_bitfield(unsigned,void*field_typ,struct Cyc_Absyn_Exp*width,struct _fat_ptr*fn);
# 219
void Cyc_Tcutil_check_unique_tvars(unsigned,struct Cyc_List_List*);
# 240
int Cyc_Tcutil_is_noalias_pointer(void*t,int must_be_unique);
# 250
int Cyc_Tcutil_is_noalias_pointer_or_aggr(void*);
# 266
void Cyc_Tcutil_add_tvar_identities(struct Cyc_List_List*);
# 278
int Cyc_Tcutil_is_const_exp(struct Cyc_Absyn_Exp*);
# 285
int Cyc_Tcutil_extract_const_from_typedef(unsigned,int declared_const,void*);
# 289
struct Cyc_List_List*Cyc_Tcutil_transfer_fn_type_atts(void*,struct Cyc_List_List*);
# 306
int Cyc_Tcutil_zeroable_type(void*);
# 310
int Cyc_Tcutil_force_type2bool(int desired,void*);
# 313
void*Cyc_Tcutil_any_bool(struct Cyc_List_List*);
# 34 "tctyp.h"
void Cyc_Tctyp_check_valid_toplevel_type(unsigned,struct Cyc_Tcenv_Tenv*,void*);
void Cyc_Tctyp_check_fndecl_valid_type(unsigned,struct Cyc_Tcenv_Tenv*,struct Cyc_Absyn_Fndecl*);
# 44 "tctyp.h"
void Cyc_Tctyp_check_type(unsigned,struct Cyc_Tcenv_Tenv*,struct Cyc_List_List*bound_tvars,struct Cyc_Absyn_Kind*k,int allow_evars,int allow_abs_aggr,void*);
# 28 "tcexp.h"
void*Cyc_Tcexp_tcExp(struct Cyc_Tcenv_Tenv*,void**,struct Cyc_Absyn_Exp*);
void*Cyc_Tcexp_tcExpInitializer(struct Cyc_Tcenv_Tenv*,void**,struct Cyc_Absyn_Exp*);
# 27 "tcstmt.h"
void*Cyc_Tcstmt_tcFndecl_valid_type(struct Cyc_Tcenv_Tenv*te,unsigned loc,struct Cyc_Absyn_Fndecl*fd);
# 30
void Cyc_Tcstmt_tcFndecl_check_body(unsigned loc,struct Cyc_Tcenv_Tenv*te,void*,struct Cyc_Absyn_Fndecl*fd);struct _tuple12{unsigned f1;int f2;};
# 28 "evexp.h"
struct _tuple12 Cyc_Evexp_eval_const_uint_exp(struct Cyc_Absyn_Exp*e);extern char Cyc_Tcdecl_Incompatible[13U];struct Cyc_Tcdecl_Incompatible_exn_struct{char*tag;};struct Cyc_Tcdecl_Xdatatypefielddecl{struct Cyc_Absyn_Datatypedecl*base;struct Cyc_Absyn_Datatypefield*field;};
# 54 "tcdecl.h"
struct Cyc_Absyn_Aggrdecl*Cyc_Tcdecl_merge_aggrdecl(struct Cyc_Absyn_Aggrdecl*,struct Cyc_Absyn_Aggrdecl*,unsigned,struct _fat_ptr*msg);
# 57
struct Cyc_Absyn_Datatypedecl*Cyc_Tcdecl_merge_datatypedecl(struct Cyc_Absyn_Datatypedecl*,struct Cyc_Absyn_Datatypedecl*,unsigned,struct _fat_ptr*msg);
# 59
struct Cyc_Absyn_Enumdecl*Cyc_Tcdecl_merge_enumdecl(struct Cyc_Absyn_Enumdecl*,struct Cyc_Absyn_Enumdecl*,unsigned,struct _fat_ptr*msg);
# 66
void*Cyc_Tcdecl_merge_binding(void*,void*,unsigned,struct _fat_ptr*msg);
# 74
struct Cyc_List_List*Cyc_Tcdecl_sort_xdatatype_fields(struct Cyc_List_List*f,int*res,struct _fat_ptr*v,unsigned,struct _fat_ptr*msg);struct Cyc_Set_Set;extern char Cyc_Set_Absent[7U];struct Cyc_Set_Absent_exn_struct{char*tag;};
# 92 "graph.h"
struct Cyc_Dict_Dict Cyc_Graph_scc(struct Cyc_Dict_Dict g);
# 30 "callgraph.h"
struct Cyc_Dict_Dict Cyc_Callgraph_compute_callgraph(struct Cyc_List_List*);
# 31 "tc.h"
extern int Cyc_Tc_aggressive_warn;
# 40
void Cyc_Tc_tcAggrdecl(struct Cyc_Tcenv_Tenv*,unsigned,struct Cyc_Absyn_Aggrdecl*);
void Cyc_Tc_tcDatatypedecl(struct Cyc_Tcenv_Tenv*,unsigned,struct Cyc_Absyn_Datatypedecl*);
void Cyc_Tc_tcEnumdecl(struct Cyc_Tcenv_Tenv*,unsigned,struct Cyc_Absyn_Enumdecl*);static char _tmp0[1U]="";
# 41 "tc.cyc"
static struct _fat_ptr Cyc_Tc_tc_msg_c={_tmp0,_tmp0,_tmp0 + 1U};
static struct _fat_ptr*Cyc_Tc_tc_msg=& Cyc_Tc_tc_msg_c;
# 45
int Cyc_Tc_aggressive_warn=0;struct _tuple13{unsigned f1;struct _tuple0*f2;int f3;};
# 47
static int Cyc_Tc_export_member(struct _tuple0*x,struct Cyc_List_List*exports){
for(0;exports != 0;exports=exports->tl){
struct _tuple13*p=(struct _tuple13*)exports->hd;
if(({Cyc_Absyn_qvar_cmp(x,(*p).f2);})== 0){
# 53
(*p).f3=1;
return 1;}}
# 57
return 0;}
# 60
static int Cyc_Tc_fnTypeAttsRangeOK(unsigned loc,int i,int nargs,void*att){
if(i < 1 || i > nargs){
({struct Cyc_String_pa_PrintArg_struct _tmp4=({struct Cyc_String_pa_PrintArg_struct _tmp210;_tmp210.tag=0U,({struct _fat_ptr _tmp253=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_attribute2string(att);}));_tmp210.f1=_tmp253;});_tmp210;});void*_tmp2[1U];_tmp2[0]=& _tmp4;({unsigned _tmp255=loc;struct _fat_ptr _tmp254=({const char*_tmp3="%s has an out-of-range index";_tag_fat(_tmp3,sizeof(char),29U);});Cyc_Tcutil_terr(_tmp255,_tmp254,_tag_fat(_tmp2,sizeof(void*),1U));});});
return 0;}
# 61
return 1;}struct _tuple14{struct Cyc_List_List*f1;struct _fat_ptr f2;};
# 67
static void Cyc_Tc_fnTypeAttsOverlap(unsigned loc,int i,struct _tuple14 lst1,struct _tuple14 lst2){
# 70
if(({({int(*_tmp257)(struct Cyc_List_List*l,int x)=({int(*_tmp6)(struct Cyc_List_List*l,int x)=(int(*)(struct Cyc_List_List*l,int x))Cyc_List_memq;_tmp6;});struct Cyc_List_List*_tmp256=lst2.f1;_tmp257(_tmp256,i);});}))
({struct Cyc_String_pa_PrintArg_struct _tmp9=({struct Cyc_String_pa_PrintArg_struct _tmp213;_tmp213.tag=0U,_tmp213.f1=(struct _fat_ptr)((struct _fat_ptr)lst1.f2);_tmp213;});struct Cyc_String_pa_PrintArg_struct _tmpA=({struct Cyc_String_pa_PrintArg_struct _tmp212;_tmp212.tag=0U,_tmp212.f1=(struct _fat_ptr)((struct _fat_ptr)lst2.f2);_tmp212;});struct Cyc_Int_pa_PrintArg_struct _tmpB=({struct Cyc_Int_pa_PrintArg_struct _tmp211;_tmp211.tag=1U,_tmp211.f1=(unsigned long)i;_tmp211;});void*_tmp7[3U];_tmp7[0]=& _tmp9,_tmp7[1]=& _tmpA,_tmp7[2]=& _tmpB;({unsigned _tmp259=loc;struct _fat_ptr _tmp258=({const char*_tmp8="incompatible %s() and %s() attributes on parameter %d";_tag_fat(_tmp8,sizeof(char),54U);});Cyc_Tcutil_terr(_tmp259,_tmp258,_tag_fat(_tmp7,sizeof(void*),3U));});});}
# 74
static void Cyc_Tc_fnTypeAttsOK(struct Cyc_Tcenv_Tenv*te,void*t,unsigned loc){
struct _tuple14 init_params=({struct _tuple14 _tmp21A;_tmp21A.f1=0,({struct _fat_ptr _tmp25A=({const char*_tmp38="initializes";_tag_fat(_tmp38,sizeof(char),12U);});_tmp21A.f2=_tmp25A;});_tmp21A;});
struct _tuple14 nolive_unique_params=({struct _tuple14 _tmp219;_tmp219.f1=0,({struct _fat_ptr _tmp25B=({const char*_tmp37="noliveunique";_tag_fat(_tmp37,sizeof(char),13U);});_tmp219.f2=_tmp25B;});_tmp219;});
struct _tuple14 consume_params=({struct _tuple14 _tmp218;_tmp218.f1=0,({struct _fat_ptr _tmp25C=({const char*_tmp36="consume";_tag_fat(_tmp36,sizeof(char),8U);});_tmp218.f2=_tmp25C;});_tmp218;});
void*_stmttmp0=({Cyc_Tcutil_compress(t);});void*_tmpD=_stmttmp0;struct Cyc_List_List*_tmpF;struct Cyc_List_List*_tmpE;if(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmpD)->tag == 5U){_LL1: _tmpE=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmpD)->f1).attributes;_tmpF=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmpD)->f1).args;_LL2: {struct Cyc_List_List*atts=_tmpE;struct Cyc_List_List*args=_tmpF;
# 80
int nargs=({Cyc_List_length(args);});
for(0;atts != 0;atts=atts->tl){
void*_stmttmp1=(void*)atts->hd;void*_tmp10=_stmttmp1;int _tmp11;int _tmp12;int _tmp13;switch(*((int*)_tmp10)){case 20U: _LL6: _tmp13=((struct Cyc_Absyn_Initializes_att_Absyn_Attribute_struct*)_tmp10)->f1;_LL7: {int i=_tmp13;
# 84
if(!({Cyc_Tc_fnTypeAttsRangeOK(loc,i,nargs,(void*)atts->hd);}))goto _LL5;({Cyc_Tc_fnTypeAttsOverlap(loc,i,init_params,nolive_unique_params);});
# 86
({Cyc_Tc_fnTypeAttsOverlap(loc,i,init_params,consume_params);});{
struct _tuple9*_stmttmp2=({({struct _tuple9*(*_tmp25E)(struct Cyc_List_List*x,int n)=({struct _tuple9*(*_tmp28)(struct Cyc_List_List*x,int n)=(struct _tuple9*(*)(struct Cyc_List_List*x,int n))Cyc_List_nth;_tmp28;});struct Cyc_List_List*_tmp25D=args;_tmp25E(_tmp25D,i - 1);});});void*_tmp14;_LLF: _tmp14=_stmttmp2->f3;_LL10: {void*t=_tmp14;
struct _fat_ptr s=({const char*_tmp27="initializes attribute allowed only on";_tag_fat(_tmp27,sizeof(char),38U);});
{void*_stmttmp3=({Cyc_Tcutil_compress(t);});void*_tmp15=_stmttmp3;void*_tmp19;void*_tmp18;void*_tmp17;void*_tmp16;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp15)->tag == 3U){_LL12: _tmp16=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp15)->f1).elt_type;_tmp17=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp15)->f1).ptr_atts).nullable;_tmp18=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp15)->f1).ptr_atts).bounds;_tmp19=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp15)->f1).ptr_atts).zero_term;_LL13: {void*t=_tmp16;void*nullable=_tmp17;void*bd=_tmp18;void*zt=_tmp19;
# 91
if(({Cyc_Tcutil_force_type2bool(0,nullable);}))
({struct Cyc_String_pa_PrintArg_struct _tmp1C=({struct Cyc_String_pa_PrintArg_struct _tmp214;_tmp214.tag=0U,_tmp214.f1=(struct _fat_ptr)((struct _fat_ptr)s);_tmp214;});void*_tmp1A[1U];_tmp1A[0]=& _tmp1C;({unsigned _tmp260=loc;struct _fat_ptr _tmp25F=({const char*_tmp1B="%s non-null pointers";_tag_fat(_tmp1B,sizeof(char),21U);});Cyc_Tcutil_terr(_tmp260,_tmp25F,_tag_fat(_tmp1A,sizeof(void*),1U));});});
# 91
if(!({Cyc_Tcutil_is_bound_one(bd);}))
# 94
({struct Cyc_String_pa_PrintArg_struct _tmp1F=({struct Cyc_String_pa_PrintArg_struct _tmp215;_tmp215.tag=0U,_tmp215.f1=(struct _fat_ptr)((struct _fat_ptr)s);_tmp215;});void*_tmp1D[1U];_tmp1D[0]=& _tmp1F;({unsigned _tmp262=loc;struct _fat_ptr _tmp261=({const char*_tmp1E="%s pointers of size 1";_tag_fat(_tmp1E,sizeof(char),22U);});Cyc_Tcutil_terr(_tmp262,_tmp261,_tag_fat(_tmp1D,sizeof(void*),1U));});});
# 91
if(({Cyc_Tcutil_force_type2bool(0,zt);}))
# 96
({struct Cyc_String_pa_PrintArg_struct _tmp22=({struct Cyc_String_pa_PrintArg_struct _tmp216;_tmp216.tag=0U,_tmp216.f1=(struct _fat_ptr)((struct _fat_ptr)s);_tmp216;});void*_tmp20[1U];_tmp20[0]=& _tmp22;({unsigned _tmp264=loc;struct _fat_ptr _tmp263=({const char*_tmp21="%s pointers to non-zero-terminated arrays";_tag_fat(_tmp21,sizeof(char),42U);});Cyc_Tcutil_terr(_tmp264,_tmp263,_tag_fat(_tmp20,sizeof(void*),1U));});});
# 91
goto _LL11;}}else{_LL14: _LL15:
# 99
({struct Cyc_String_pa_PrintArg_struct _tmp25=({struct Cyc_String_pa_PrintArg_struct _tmp217;_tmp217.tag=0U,_tmp217.f1=(struct _fat_ptr)((struct _fat_ptr)s);_tmp217;});void*_tmp23[1U];_tmp23[0]=& _tmp25;({unsigned _tmp266=loc;struct _fat_ptr _tmp265=({const char*_tmp24="%s pointers";_tag_fat(_tmp24,sizeof(char),12U);});Cyc_Tcutil_terr(_tmp266,_tmp265,_tag_fat(_tmp23,sizeof(void*),1U));});});}_LL11:;}
# 101
({struct Cyc_List_List*_tmp267=({struct Cyc_List_List*_tmp26=_cycalloc(sizeof(*_tmp26));_tmp26->hd=(void*)i,_tmp26->tl=init_params.f1;_tmp26;});init_params.f1=_tmp267;});
goto _LL5;}}}case 21U: _LL8: _tmp12=((struct Cyc_Absyn_Noliveunique_att_Absyn_Attribute_struct*)_tmp10)->f1;_LL9: {int i=_tmp12;
# 104
if(!({Cyc_Tc_fnTypeAttsRangeOK(loc,i,nargs,(void*)atts->hd);}))goto _LL5;({Cyc_Tc_fnTypeAttsOverlap(loc,i,nolive_unique_params,init_params);});{
# 107
struct _tuple9*_stmttmp4=({({struct _tuple9*(*_tmp269)(struct Cyc_List_List*x,int n)=({struct _tuple9*(*_tmp2D)(struct Cyc_List_List*x,int n)=(struct _tuple9*(*)(struct Cyc_List_List*x,int n))Cyc_List_nth;_tmp2D;});struct Cyc_List_List*_tmp268=args;_tmp269(_tmp268,i - 1);});});void*_tmp29;_LL17: _tmp29=_stmttmp4->f3;_LL18: {void*t=_tmp29;
if(!({Cyc_Tcutil_is_noalias_pointer(t,0);}))
({void*_tmp2A=0U;({unsigned _tmp26B=loc;struct _fat_ptr _tmp26A=({const char*_tmp2B="noliveunique attribute allowed only on unique pointers";_tag_fat(_tmp2B,sizeof(char),55U);});Cyc_Tcutil_terr(_tmp26B,_tmp26A,_tag_fat(_tmp2A,sizeof(void*),0U));});});
# 108
({struct Cyc_List_List*_tmp26C=({struct Cyc_List_List*_tmp2C=_cycalloc(sizeof(*_tmp2C));
# 110
_tmp2C->hd=(void*)i,_tmp2C->tl=nolive_unique_params.f1;_tmp2C;});
# 108
nolive_unique_params.f1=_tmp26C;});
# 111
goto _LL5;}}}case 22U: _LLA: _tmp11=((struct Cyc_Absyn_Consume_att_Absyn_Attribute_struct*)_tmp10)->f1;_LLB: {int i=_tmp11;
# 113
if(!({Cyc_Tc_fnTypeAttsRangeOK(loc,i,nargs,(void*)atts->hd);}))goto _LL5;({Cyc_Tc_fnTypeAttsOverlap(loc,i,consume_params,init_params);});{
# 116
struct _tuple9*_stmttmp5=({({struct _tuple9*(*_tmp26E)(struct Cyc_List_List*x,int n)=({struct _tuple9*(*_tmp32)(struct Cyc_List_List*x,int n)=(struct _tuple9*(*)(struct Cyc_List_List*x,int n))Cyc_List_nth;_tmp32;});struct Cyc_List_List*_tmp26D=args;_tmp26E(_tmp26D,i - 1);});});void*_tmp2E;_LL1A: _tmp2E=_stmttmp5->f3;_LL1B: {void*t=_tmp2E;
if(!({Cyc_Tcutil_is_noalias_pointer(t,0);}))
({void*_tmp2F=0U;({unsigned _tmp270=loc;struct _fat_ptr _tmp26F=({const char*_tmp30="consume attribute allowed only on unique pointers";_tag_fat(_tmp30,sizeof(char),50U);});Cyc_Tcutil_terr(_tmp270,_tmp26F,_tag_fat(_tmp2F,sizeof(void*),0U));});});
# 117
({struct Cyc_List_List*_tmp271=({struct Cyc_List_List*_tmp31=_cycalloc(sizeof(*_tmp31));
# 119
_tmp31->hd=(void*)i,_tmp31->tl=consume_params.f1;_tmp31;});
# 117
consume_params.f1=_tmp271;});
# 120
goto _LL5;}}}default: _LLC: _LLD:
 goto _LL5;}_LL5:;}
# 124
goto _LL0;}}else{_LL3: _LL4:
({void*_tmp33=0U;({int(*_tmp273)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp35)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp35;});struct _fat_ptr _tmp272=({const char*_tmp34="fnTypeAttsOK: not a function type";_tag_fat(_tmp34,sizeof(char),34U);});_tmp273(_tmp272,_tag_fat(_tmp33,sizeof(void*),0U));});});}_LL0:;}struct _tuple15{void*f1;int f2;};
# 129
static void Cyc_Tc_tcVardecl(struct Cyc_Tcenv_Tenv*te,unsigned loc,struct Cyc_Absyn_Vardecl*vd,int check_var_init,int in_cinclude,struct Cyc_List_List**exports){
# 131
struct Cyc_List_List*_tmp3E;struct Cyc_Absyn_Exp*_tmp3D;void*_tmp3C;struct _tuple0*_tmp3B;enum Cyc_Absyn_Scope _tmp3A;_LL1: _tmp3A=vd->sc;_tmp3B=vd->name;_tmp3C=vd->type;_tmp3D=vd->initializer;_tmp3E=vd->attributes;_LL2: {enum Cyc_Absyn_Scope sc=_tmp3A;struct _tuple0*q=_tmp3B;void*t=_tmp3C;struct Cyc_Absyn_Exp*initializer=_tmp3D;struct Cyc_List_List*atts=_tmp3E;
# 138
{void*_stmttmp6=({Cyc_Tcutil_compress(t);});void*_tmp3F=_stmttmp6;unsigned _tmp43;void*_tmp42;struct Cyc_Absyn_Tqual _tmp41;void*_tmp40;if(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp3F)->tag == 4U){if((((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp3F)->f1).num_elts == 0){_LL4: _tmp40=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp3F)->f1).elt_type;_tmp41=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp3F)->f1).tq;_tmp42=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp3F)->f1).zero_term;_tmp43=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp3F)->f1).zt_loc;if(initializer != 0){_LL5: {void*telt=_tmp40;struct Cyc_Absyn_Tqual tq=_tmp41;void*zt=_tmp42;unsigned ztl=_tmp43;
# 140
{void*_stmttmp7=initializer->r;void*_tmp44=_stmttmp7;struct Cyc_List_List*_tmp45;struct Cyc_List_List*_tmp46;struct Cyc_Absyn_Exp*_tmp47;struct Cyc_Absyn_Exp*_tmp48;struct _fat_ptr _tmp49;struct _fat_ptr _tmp4A;switch(*((int*)_tmp44)){case 0U: switch(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp44)->f1).Wstring_c).tag){case 8U: _LL9: _tmp4A=((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp44)->f1).String_c).val;_LLA: {struct _fat_ptr s=_tmp4A;
# 142
t=({void*_tmp274=({void*(*_tmp4B)(void*elt_type,struct Cyc_Absyn_Tqual tq,struct Cyc_Absyn_Exp*num_elts,void*zero_term,unsigned ztloc)=Cyc_Absyn_array_type;void*_tmp4C=telt;struct Cyc_Absyn_Tqual _tmp4D=tq;struct Cyc_Absyn_Exp*_tmp4E=({Cyc_Absyn_uint_exp(_get_fat_size(s,sizeof(char)),0U);});void*_tmp4F=zt;unsigned _tmp50=ztl;_tmp4B(_tmp4C,_tmp4D,_tmp4E,_tmp4F,_tmp50);});vd->type=_tmp274;});
goto _LL8;}case 9U: _LLB: _tmp49=((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp44)->f1).Wstring_c).val;_LLC: {struct _fat_ptr s=_tmp49;
# 145
t=({void*_tmp275=({void*(*_tmp51)(void*elt_type,struct Cyc_Absyn_Tqual tq,struct Cyc_Absyn_Exp*num_elts,void*zero_term,unsigned ztloc)=Cyc_Absyn_array_type;void*_tmp52=telt;struct Cyc_Absyn_Tqual _tmp53=tq;struct Cyc_Absyn_Exp*_tmp54=({Cyc_Absyn_uint_exp(1U,0U);});void*_tmp55=zt;unsigned _tmp56=ztl;_tmp51(_tmp52,_tmp53,_tmp54,_tmp55,_tmp56);});vd->type=_tmp275;});
goto _LL8;}default: goto _LL15;}case 27U: _LLD: _tmp48=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmp44)->f2;_LLE: {struct Cyc_Absyn_Exp*e=_tmp48;
_tmp47=e;goto _LL10;}case 28U: _LLF: _tmp47=((struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*)_tmp44)->f1;_LL10: {struct Cyc_Absyn_Exp*e=_tmp47;
# 150
t=({void*_tmp276=({Cyc_Absyn_array_type(telt,tq,e,zt,ztl);});vd->type=_tmp276;});
goto _LL8;}case 37U: _LL11: _tmp46=((struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*)_tmp44)->f2;_LL12: {struct Cyc_List_List*es=_tmp46;
_tmp45=es;goto _LL14;}case 26U: _LL13: _tmp45=((struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*)_tmp44)->f1;_LL14: {struct Cyc_List_List*es=_tmp45;
# 154
t=({void*_tmp277=({void*(*_tmp57)(void*elt_type,struct Cyc_Absyn_Tqual tq,struct Cyc_Absyn_Exp*num_elts,void*zero_term,unsigned ztloc)=Cyc_Absyn_array_type;void*_tmp58=telt;struct Cyc_Absyn_Tqual _tmp59=tq;struct Cyc_Absyn_Exp*_tmp5A=({struct Cyc_Absyn_Exp*(*_tmp5B)(unsigned,unsigned)=Cyc_Absyn_uint_exp;unsigned _tmp5C=(unsigned)({Cyc_List_length(es);});unsigned _tmp5D=0U;_tmp5B(_tmp5C,_tmp5D);});void*_tmp5E=zt;unsigned _tmp5F=ztl;_tmp57(_tmp58,_tmp59,_tmp5A,_tmp5E,_tmp5F);});vd->type=_tmp277;});
goto _LL8;}default: _LL15: _LL16:
 goto _LL8;}_LL8:;}
# 158
goto _LL3;}}else{goto _LL6;}}else{goto _LL6;}}else{_LL6: _LL7:
 goto _LL3;}_LL3:;}
# 162
({Cyc_Tctyp_check_valid_toplevel_type(loc,te,t);});
# 164
({int _tmp278=({Cyc_Tcutil_extract_const_from_typedef(loc,(vd->tq).print_const,t);});(vd->tq).real_const=_tmp278;});
# 167
({int _tmp279=!({Cyc_Tcutil_is_array_type(t);});vd->escapes=_tmp279;});
# 169
if(({Cyc_Tcutil_is_function_type(t);})){
atts=({Cyc_Tcutil_transfer_fn_type_atts(t,atts);});
({Cyc_Tc_fnTypeAttsOK(te,t,loc);});}
# 169
if(
# 174
(int)sc == (int)3U ||(int)sc == (int)4U){
if(initializer != 0 && !in_cinclude)
({void*_tmp60=0U;({unsigned _tmp27B=loc;struct _fat_ptr _tmp27A=({const char*_tmp61="extern declaration should not have initializer";_tag_fat(_tmp61,sizeof(char),47U);});Cyc_Tcutil_terr(_tmp27B,_tmp27A,_tag_fat(_tmp60,sizeof(void*),0U));});});}else{
if(!({Cyc_Tcutil_is_function_type(t);})){
# 181
for(0;atts != 0;atts=atts->tl){
void*_stmttmp8=(void*)atts->hd;void*_tmp62=_stmttmp8;switch(*((int*)_tmp62)){case 6U: _LL18: _LL19:
 goto _LL1B;case 8U: _LL1A: _LL1B:
# 186
 goto _LL1D;case 9U: _LL1C: _LL1D:
 goto _LL1F;case 10U: _LL1E: _LL1F:
 goto _LL21;case 11U: _LL20: _LL21:
 goto _LL23;case 12U: _LL22: _LL23:
 goto _LL25;case 13U: _LL24: _LL25:
 goto _LL27;case 14U: _LL26: _LL27:
 continue;default: _LL28: _LL29:
# 194
({struct Cyc_String_pa_PrintArg_struct _tmp65=({struct Cyc_String_pa_PrintArg_struct _tmp21C;_tmp21C.tag=0U,({
struct _fat_ptr _tmp27C=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_attribute2string((void*)atts->hd);}));_tmp21C.f1=_tmp27C;});_tmp21C;});struct Cyc_String_pa_PrintArg_struct _tmp66=({struct Cyc_String_pa_PrintArg_struct _tmp21B;_tmp21B.tag=0U,({struct _fat_ptr _tmp27D=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(vd->name);}));_tmp21B.f1=_tmp27D;});_tmp21B;});void*_tmp63[2U];_tmp63[0]=& _tmp65,_tmp63[1]=& _tmp66;({unsigned _tmp27F=loc;struct _fat_ptr _tmp27E=({const char*_tmp64="bad attribute %s for variable %s";_tag_fat(_tmp64,sizeof(char),33U);});Cyc_Tcutil_terr(_tmp27F,_tmp27E,_tag_fat(_tmp63,sizeof(void*),2U));});});
goto _LL17;}_LL17:;}
# 199
if(initializer == 0 || in_cinclude){
if((check_var_init && !in_cinclude)&& !({Cyc_Tcutil_zeroable_type(t);}))
({struct Cyc_String_pa_PrintArg_struct _tmp69=({struct Cyc_String_pa_PrintArg_struct _tmp21E;_tmp21E.tag=0U,({
struct _fat_ptr _tmp280=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(vd->name);}));_tmp21E.f1=_tmp280;});_tmp21E;});struct Cyc_String_pa_PrintArg_struct _tmp6A=({struct Cyc_String_pa_PrintArg_struct _tmp21D;_tmp21D.tag=0U,({struct _fat_ptr _tmp281=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t);}));_tmp21D.f1=_tmp281;});_tmp21D;});void*_tmp67[2U];_tmp67[0]=& _tmp69,_tmp67[1]=& _tmp6A;({unsigned _tmp283=loc;struct _fat_ptr _tmp282=({const char*_tmp68="initializer required for variable %s of type %s";_tag_fat(_tmp68,sizeof(char),48U);});Cyc_Tcutil_terr(_tmp283,_tmp282,_tag_fat(_tmp67,sizeof(void*),2U));});});}else{
# 204
struct Cyc_Absyn_Exp*e=initializer;
# 208
struct _handler_cons _tmp6B;_push_handler(& _tmp6B);{int _tmp6D=0;if(setjmp(_tmp6B.handler))_tmp6D=1;if(!_tmp6D){
{void*t2=({Cyc_Tcexp_tcExpInitializer(te,& t,e);});
if(!({Cyc_Tcutil_coerce_assign(te,e,t);})){
struct _fat_ptr s0=({Cyc_Absynpp_qvar2string(vd->name);});
const char*s1=" declared with type ";
struct _fat_ptr s2=({Cyc_Absynpp_typ2string(t);});
const char*s3=" but initialized with type ";
struct _fat_ptr s4=({Cyc_Absynpp_typ2string(t2);});
if(({unsigned long _tmp287=({unsigned long _tmp286=({unsigned long _tmp285=({unsigned long _tmp284=({Cyc_strlen((struct _fat_ptr)s0);});_tmp284 + ({Cyc_strlen(({const char*_tmp6E=s1;_tag_fat(_tmp6E,sizeof(char),21U);}));});});_tmp285 + ({Cyc_strlen((struct _fat_ptr)s2);});});_tmp286 + ({Cyc_strlen(({const char*_tmp6F=s3;_tag_fat(_tmp6F,sizeof(char),28U);}));});});_tmp287 + ({Cyc_strlen((struct _fat_ptr)s4);});})> (unsigned long)70)
({struct Cyc_String_pa_PrintArg_struct _tmp72=({struct Cyc_String_pa_PrintArg_struct _tmp223;_tmp223.tag=0U,_tmp223.f1=(struct _fat_ptr)((struct _fat_ptr)s0);_tmp223;});struct Cyc_String_pa_PrintArg_struct _tmp73=({struct Cyc_String_pa_PrintArg_struct _tmp222;_tmp222.tag=0U,({struct _fat_ptr _tmp288=(struct _fat_ptr)({const char*_tmp78=s1;_tag_fat(_tmp78,sizeof(char),21U);});_tmp222.f1=_tmp288;});_tmp222;});struct Cyc_String_pa_PrintArg_struct _tmp74=({struct Cyc_String_pa_PrintArg_struct _tmp221;_tmp221.tag=0U,_tmp221.f1=(struct _fat_ptr)((struct _fat_ptr)s2);_tmp221;});struct Cyc_String_pa_PrintArg_struct _tmp75=({struct Cyc_String_pa_PrintArg_struct _tmp220;_tmp220.tag=0U,({struct _fat_ptr _tmp289=(struct _fat_ptr)({const char*_tmp77=s3;_tag_fat(_tmp77,sizeof(char),28U);});_tmp220.f1=_tmp289;});_tmp220;});struct Cyc_String_pa_PrintArg_struct _tmp76=({struct Cyc_String_pa_PrintArg_struct _tmp21F;_tmp21F.tag=0U,_tmp21F.f1=(struct _fat_ptr)((struct _fat_ptr)s4);_tmp21F;});void*_tmp70[5U];_tmp70[0]=& _tmp72,_tmp70[1]=& _tmp73,_tmp70[2]=& _tmp74,_tmp70[3]=& _tmp75,_tmp70[4]=& _tmp76;({unsigned _tmp28B=loc;struct _fat_ptr _tmp28A=({const char*_tmp71="%s%s\n\t%s\n%s\n\t%s";_tag_fat(_tmp71,sizeof(char),16U);});Cyc_Tcutil_terr(_tmp28B,_tmp28A,_tag_fat(_tmp70,sizeof(void*),5U));});});else{
# 219
({struct Cyc_String_pa_PrintArg_struct _tmp7B=({struct Cyc_String_pa_PrintArg_struct _tmp228;_tmp228.tag=0U,_tmp228.f1=(struct _fat_ptr)((struct _fat_ptr)s0);_tmp228;});struct Cyc_String_pa_PrintArg_struct _tmp7C=({struct Cyc_String_pa_PrintArg_struct _tmp227;_tmp227.tag=0U,({struct _fat_ptr _tmp28C=(struct _fat_ptr)({const char*_tmp81=s1;_tag_fat(_tmp81,sizeof(char),21U);});_tmp227.f1=_tmp28C;});_tmp227;});struct Cyc_String_pa_PrintArg_struct _tmp7D=({struct Cyc_String_pa_PrintArg_struct _tmp226;_tmp226.tag=0U,_tmp226.f1=(struct _fat_ptr)((struct _fat_ptr)s2);_tmp226;});struct Cyc_String_pa_PrintArg_struct _tmp7E=({struct Cyc_String_pa_PrintArg_struct _tmp225;_tmp225.tag=0U,({struct _fat_ptr _tmp28D=(struct _fat_ptr)({const char*_tmp80=s3;_tag_fat(_tmp80,sizeof(char),28U);});_tmp225.f1=_tmp28D;});_tmp225;});struct Cyc_String_pa_PrintArg_struct _tmp7F=({struct Cyc_String_pa_PrintArg_struct _tmp224;_tmp224.tag=0U,_tmp224.f1=(struct _fat_ptr)((struct _fat_ptr)s4);_tmp224;});void*_tmp79[5U];_tmp79[0]=& _tmp7B,_tmp79[1]=& _tmp7C,_tmp79[2]=& _tmp7D,_tmp79[3]=& _tmp7E,_tmp79[4]=& _tmp7F;({unsigned _tmp28F=loc;struct _fat_ptr _tmp28E=({const char*_tmp7A="%s%s%s%s%s";_tag_fat(_tmp7A,sizeof(char),11U);});Cyc_Tcutil_terr(_tmp28F,_tmp28E,_tag_fat(_tmp79,sizeof(void*),5U));});});}
({Cyc_Unify_explain_failure();});}
# 210
if(!({Cyc_Tcutil_is_const_exp(e);}))
# 224
({void*_tmp82=0U;({unsigned _tmp291=loc;struct _fat_ptr _tmp290=({const char*_tmp83="initializer is not a constant expression";_tag_fat(_tmp83,sizeof(char),41U);});Cyc_Tcutil_terr(_tmp291,_tmp290,_tag_fat(_tmp82,sizeof(void*),0U));});});}
# 209
;_pop_handler();}else{void*_tmp6C=(void*)Cyc_Core_get_exn_thrown();void*_tmp84=_tmp6C;void*_tmp85;if(((struct Cyc_Tcenv_Env_error_exn_struct*)_tmp84)->tag == Cyc_Tcenv_Env_error){_LL2B: _LL2C:
# 227
({void*_tmp86=0U;({unsigned _tmp293=loc;struct _fat_ptr _tmp292=({const char*_tmp87="initializer is not a constant expression";_tag_fat(_tmp87,sizeof(char),41U);});Cyc_Tcutil_terr(_tmp293,_tmp292,_tag_fat(_tmp86,sizeof(void*),0U));});});
goto _LL2A;}else{_LL2D: _tmp85=_tmp84;_LL2E: {void*exn=_tmp85;(int)_rethrow(exn);}}_LL2A:;}}}}else{
# 233
for(0;atts != 0;atts=atts->tl){
void*_stmttmp9=(void*)atts->hd;void*_tmp88=_stmttmp9;switch(*((int*)_tmp88)){case 0U: _LL30: _LL31:
# 236
 goto _LL33;case 1U: _LL32: _LL33:
 goto _LL35;case 2U: _LL34: _LL35:
 goto _LL37;case 3U: _LL36: _LL37:
 goto _LL39;case 4U: _LL38: _LL39:
 goto _LL3B;case 19U: _LL3A: _LL3B:
 goto _LL3D;case 20U: _LL3C: _LL3D:
 goto _LL3F;case 23U: _LL3E: _LL3F:
 goto _LL41;case 26U: _LL40: _LL41:
 goto _LL43;case 5U: _LL42: _LL43:
({void*_tmp89=0U;({int(*_tmp295)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp8B)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp8B;});struct _fat_ptr _tmp294=({const char*_tmp8A="tcVardecl: fn type atts in function var decl";_tag_fat(_tmp8A,sizeof(char),45U);});_tmp295(_tmp294,_tag_fat(_tmp89,sizeof(void*),0U));});});case 6U: _LL44: _LL45:
# 247
 goto _LL47;case 7U: _LL46: _LL47:
# 249
({struct Cyc_String_pa_PrintArg_struct _tmp8E=({struct Cyc_String_pa_PrintArg_struct _tmp229;_tmp229.tag=0U,({
struct _fat_ptr _tmp296=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_attribute2string((void*)atts->hd);}));_tmp229.f1=_tmp296;});_tmp229;});void*_tmp8C[1U];_tmp8C[0]=& _tmp8E;({unsigned _tmp298=loc;struct _fat_ptr _tmp297=({const char*_tmp8D="bad attribute %s in function declaration";_tag_fat(_tmp8D,sizeof(char),41U);});Cyc_Tcutil_terr(_tmp298,_tmp297,_tag_fat(_tmp8C,sizeof(void*),1U));});});
goto _LL2F;default: _LL48: _LL49:
 continue;}_LL2F:;}}}
# 257
{struct _handler_cons _tmp8F;_push_handler(& _tmp8F);{int _tmp91=0;if(setjmp(_tmp8F.handler))_tmp91=1;if(!_tmp91){
{struct _tuple15*ans=({({struct _tuple15*(*_tmp29A)(struct Cyc_Dict_Dict d,struct _tuple0*k)=({struct _tuple15*(*_tmp95)(struct Cyc_Dict_Dict d,struct _tuple0*k)=(struct _tuple15*(*)(struct Cyc_Dict_Dict d,struct _tuple0*k))Cyc_Dict_lookup;_tmp95;});struct Cyc_Dict_Dict _tmp299=(te->ae)->ordinaries;_tmp29A(_tmp299,q);});});
void*b0=(*ans).f1;
struct Cyc_Absyn_Global_b_Absyn_Binding_struct*b1=({struct Cyc_Absyn_Global_b_Absyn_Binding_struct*_tmp94=_cycalloc(sizeof(*_tmp94));_tmp94->tag=1U,_tmp94->f1=vd;_tmp94;});
void*b=({Cyc_Tcdecl_merge_binding(b0,(void*)b1,loc,Cyc_Tc_tc_msg);});
if(b == 0){_npop_handler(0U);return;}if(
# 264
exports == 0 ||({Cyc_Tc_export_member(vd->name,*exports);})){
if(b != b0 ||(*ans).f2)
# 267
({struct Cyc_Dict_Dict _tmp29E=({({struct Cyc_Dict_Dict(*_tmp29D)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct _tuple15*v)=({struct Cyc_Dict_Dict(*_tmp92)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct _tuple15*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct _tuple15*v))Cyc_Dict_insert;_tmp92;});struct Cyc_Dict_Dict _tmp29C=(te->ae)->ordinaries;struct _tuple0*_tmp29B=q;_tmp29D(_tmp29C,_tmp29B,({struct _tuple15*_tmp93=_cycalloc(sizeof(*_tmp93));_tmp93->f1=b,_tmp93->f2=(*ans).f2;_tmp93;}));});});(te->ae)->ordinaries=_tmp29E;});}
# 262
_npop_handler(0U);return;}
# 258
;_pop_handler();}else{void*_tmp90=(void*)Cyc_Core_get_exn_thrown();void*_tmp96=_tmp90;void*_tmp97;if(((struct Cyc_Dict_Absent_exn_struct*)_tmp96)->tag == Cyc_Dict_Absent){_LL4B: _LL4C:
# 270
 goto _LL4A;}else{_LL4D: _tmp97=_tmp96;_LL4E: {void*exn=_tmp97;(int)_rethrow(exn);}}_LL4A:;}}}
if(exports == 0 ||({Cyc_Tc_export_member(vd->name,*exports);}))
({struct Cyc_Dict_Dict _tmp2A3=({({struct Cyc_Dict_Dict(*_tmp2A2)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct _tuple15*v)=({struct Cyc_Dict_Dict(*_tmp98)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct _tuple15*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct _tuple15*v))Cyc_Dict_insert;_tmp98;});struct Cyc_Dict_Dict _tmp2A1=(te->ae)->ordinaries;struct _tuple0*_tmp2A0=q;_tmp2A2(_tmp2A1,_tmp2A0,({struct _tuple15*_tmp9A=_cycalloc(sizeof(*_tmp9A));({void*_tmp29F=(void*)({struct Cyc_Absyn_Global_b_Absyn_Binding_struct*_tmp99=_cycalloc(sizeof(*_tmp99));_tmp99->tag=1U,_tmp99->f1=vd;_tmp99;});_tmp9A->f1=_tmp29F;}),_tmp9A->f2=0;_tmp9A;}));});});(te->ae)->ordinaries=_tmp2A3;});}}
# 276
static int Cyc_Tc_is_main(struct _tuple0*n){
struct _fat_ptr*_tmp9D;union Cyc_Absyn_Nmspace _tmp9C;_LL1: _tmp9C=n->f1;_tmp9D=n->f2;_LL2: {union Cyc_Absyn_Nmspace nms=_tmp9C;struct _fat_ptr*v=_tmp9D;
union Cyc_Absyn_Nmspace _tmp9E=nms;if((_tmp9E.Abs_n).tag == 2){if((_tmp9E.Abs_n).val == 0){_LL4: _LL5:
# 280
 return({({struct _fat_ptr _tmp2A4=(struct _fat_ptr)*v;Cyc_strcmp(_tmp2A4,({const char*_tmp9F="main";_tag_fat(_tmp9F,sizeof(char),5U);}));});})== 0;}else{goto _LL6;}}else{_LL6: _LL7:
 return 0;}_LL3:;}}
# 285
static void Cyc_Tc_tcFndecl_check_main(unsigned loc,struct Cyc_Absyn_Fndecl*fd){
# 289
{void*_stmttmpA=({Cyc_Tcutil_compress((fd->i).ret_type);});void*_tmpA1=_stmttmpA;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpA1)->tag == 0U)switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpA1)->f1)){case 0U: _LL1: _LL2:
({void*_tmpA2=0U;({unsigned _tmp2A6=loc;struct _fat_ptr _tmp2A5=({const char*_tmpA3="main declared with return type void";_tag_fat(_tmpA3,sizeof(char),36U);});Cyc_Tcutil_warn(_tmp2A6,_tmp2A5,_tag_fat(_tmpA2,sizeof(void*),0U));});});goto _LL0;case 1U: switch(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpA1)->f1)->f2){case Cyc_Absyn_Int_sz: _LL3: _LL4:
 goto _LL0;case Cyc_Absyn_Long_sz: _LL5: _LL6:
 goto _LL0;default: goto _LL7;}default: goto _LL7;}else{_LL7: _LL8:
# 294
({struct Cyc_String_pa_PrintArg_struct _tmpA6=({struct Cyc_String_pa_PrintArg_struct _tmp22A;_tmp22A.tag=0U,({
struct _fat_ptr _tmp2A7=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string((fd->i).ret_type);}));_tmp22A.f1=_tmp2A7;});_tmp22A;});void*_tmpA4[1U];_tmpA4[0]=& _tmpA6;({unsigned _tmp2A9=loc;struct _fat_ptr _tmp2A8=({const char*_tmpA5="main declared with return type %s instead of int or void";_tag_fat(_tmpA5,sizeof(char),57U);});Cyc_Tcutil_terr(_tmp2A9,_tmp2A8,_tag_fat(_tmpA4,sizeof(void*),1U));});});
goto _LL0;}_LL0:;}
# 298
if((fd->i).c_varargs ||(fd->i).cyc_varargs != 0)
({void*_tmpA7=0U;({unsigned _tmp2AB=loc;struct _fat_ptr _tmp2AA=({const char*_tmpA8="main declared with varargs";_tag_fat(_tmpA8,sizeof(char),27U);});Cyc_Tcutil_terr(_tmp2AB,_tmp2AA,_tag_fat(_tmpA7,sizeof(void*),0U));});});{
# 298
struct Cyc_List_List*args=(fd->i).args;
# 301
if(args != 0){
struct _tuple9*_stmttmpB=(struct _tuple9*)args->hd;void*_tmpA9;_LLA: _tmpA9=_stmttmpB->f3;_LLB: {void*t1=_tmpA9;
{void*_stmttmpC=({Cyc_Tcutil_compress(t1);});void*_tmpAA=_stmttmpC;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpAA)->tag == 0U){if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpAA)->f1)->tag == 1U)switch(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpAA)->f1)->f2){case Cyc_Absyn_Int_sz: _LLD: _LLE:
 goto _LLC;case Cyc_Absyn_Long_sz: _LLF: _LL10:
 goto _LLC;default: goto _LL11;}else{goto _LL11;}}else{_LL11: _LL12:
# 307
({struct Cyc_String_pa_PrintArg_struct _tmpAD=({struct Cyc_String_pa_PrintArg_struct _tmp22B;_tmp22B.tag=0U,({
struct _fat_ptr _tmp2AC=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t1);}));_tmp22B.f1=_tmp2AC;});_tmp22B;});void*_tmpAB[1U];_tmpAB[0]=& _tmpAD;({unsigned _tmp2AE=loc;struct _fat_ptr _tmp2AD=({const char*_tmpAC="main declared with first argument of type %s instead of int";_tag_fat(_tmpAC,sizeof(char),60U);});Cyc_Tcutil_terr(_tmp2AE,_tmp2AD,_tag_fat(_tmpAB,sizeof(void*),1U));});});
goto _LLC;}_LLC:;}
# 311
args=args->tl;
if(args != 0){
struct _tuple9*_stmttmpD=(struct _tuple9*)args->hd;void*_tmpAE;_LL14: _tmpAE=_stmttmpD->f3;_LL15: {void*t2=_tmpAE;
args=args->tl;
if(args != 0)
({void*_tmpAF=0U;({unsigned _tmp2B0=loc;struct _fat_ptr _tmp2AF=({const char*_tmpB0="main declared with too many arguments";_tag_fat(_tmpB0,sizeof(char),38U);});Cyc_Tcutil_terr(_tmp2B0,_tmp2AF,_tag_fat(_tmpAF,sizeof(void*),0U));});});{
# 315
struct Cyc_Core_Opt*tvs=({struct Cyc_Core_Opt*_tmpE4=_cycalloc(sizeof(*_tmpE4));_tmpE4->v=(fd->i).tvars;_tmpE4;});
# 318
if(((!({int(*_tmpB1)(void*,void*)=Cyc_Unify_unify;void*_tmpB2=t2;void*_tmpB3=({void*(*_tmpB4)(void*t,void*rgn,struct Cyc_Absyn_Tqual,void*zeroterm)=Cyc_Absyn_fatptr_type;void*_tmpB5=({void*(*_tmpB6)(void*rgn)=Cyc_Absyn_string_type;void*_tmpB7=({({struct Cyc_Core_Opt*_tmp2B7=({struct Cyc_Core_Opt*_tmpB8=_cycalloc(sizeof(*_tmpB8));_tmpB8->v=& Cyc_Tcutil_rk;_tmpB8;});Cyc_Absyn_new_evar(_tmp2B7,tvs);});});_tmpB6(_tmpB7);});void*_tmpB9=({({struct Cyc_Core_Opt*_tmp2B8=({struct Cyc_Core_Opt*_tmpBA=_cycalloc(sizeof(*_tmpBA));_tmpBA->v=& Cyc_Tcutil_rk;_tmpBA;});Cyc_Absyn_new_evar(_tmp2B8,tvs);});});struct Cyc_Absyn_Tqual _tmpBB=({Cyc_Absyn_empty_tqual(0U);});void*_tmpBC=({Cyc_Tcutil_any_bool((struct Cyc_List_List*)tvs->v);});_tmpB4(_tmpB5,_tmpB9,_tmpBB,_tmpBC);});_tmpB1(_tmpB2,_tmpB3);})&& !({int(*_tmpBD)(void*,void*)=Cyc_Unify_unify;void*_tmpBE=t2;void*_tmpBF=({void*(*_tmpC0)(void*t,void*rgn,struct Cyc_Absyn_Tqual,void*zeroterm)=Cyc_Absyn_fatptr_type;void*_tmpC1=({void*(*_tmpC2)(void*rgn)=Cyc_Absyn_const_string_type;void*_tmpC3=({({struct Cyc_Core_Opt*_tmp2B5=({struct Cyc_Core_Opt*_tmpC4=_cycalloc(sizeof(*_tmpC4));_tmpC4->v=& Cyc_Tcutil_rk;_tmpC4;});Cyc_Absyn_new_evar(_tmp2B5,tvs);});});_tmpC2(_tmpC3);});void*_tmpC5=({({struct Cyc_Core_Opt*_tmp2B6=({struct Cyc_Core_Opt*_tmpC6=_cycalloc(sizeof(*_tmpC6));_tmpC6->v=& Cyc_Tcutil_rk;_tmpC6;});Cyc_Absyn_new_evar(_tmp2B6,tvs);});});struct Cyc_Absyn_Tqual _tmpC7=({Cyc_Absyn_empty_tqual(0U);});void*_tmpC8=({Cyc_Tcutil_any_bool((struct Cyc_List_List*)tvs->v);});_tmpC0(_tmpC1,_tmpC5,_tmpC7,_tmpC8);});_tmpBD(_tmpBE,_tmpBF);}))&& !({int(*_tmpC9)(void*,void*)=Cyc_Unify_unify;void*_tmpCA=t2;void*_tmpCB=({void*(*_tmpCC)(void*t,void*rgn,struct Cyc_Absyn_Tqual,void*zeroterm)=Cyc_Absyn_fatptr_type;void*_tmpCD=({void*(*_tmpCE)(void*rgn)=Cyc_Absyn_string_type;void*_tmpCF=({({struct Cyc_Core_Opt*_tmp2B3=({struct Cyc_Core_Opt*_tmpD0=_cycalloc(sizeof(*_tmpD0));_tmpD0->v=& Cyc_Tcutil_rk;_tmpD0;});Cyc_Absyn_new_evar(_tmp2B3,tvs);});});_tmpCE(_tmpCF);});void*_tmpD1=({({struct Cyc_Core_Opt*_tmp2B4=({struct Cyc_Core_Opt*_tmpD2=_cycalloc(sizeof(*_tmpD2));_tmpD2->v=& Cyc_Tcutil_rk;_tmpD2;});Cyc_Absyn_new_evar(_tmp2B4,tvs);});});struct Cyc_Absyn_Tqual _tmpD3=({Cyc_Absyn_const_tqual(0U);});void*_tmpD4=({Cyc_Tcutil_any_bool((struct Cyc_List_List*)tvs->v);});_tmpCC(_tmpCD,_tmpD1,_tmpD3,_tmpD4);});_tmpC9(_tmpCA,_tmpCB);}))&& !({int(*_tmpD5)(void*,void*)=Cyc_Unify_unify;void*_tmpD6=t2;void*_tmpD7=({void*(*_tmpD8)(void*t,void*rgn,struct Cyc_Absyn_Tqual,void*zeroterm)=Cyc_Absyn_fatptr_type;void*_tmpD9=({void*(*_tmpDA)(void*rgn)=Cyc_Absyn_const_string_type;void*_tmpDB=({({struct Cyc_Core_Opt*_tmp2B1=({struct Cyc_Core_Opt*_tmpDC=_cycalloc(sizeof(*_tmpDC));_tmpDC->v=& Cyc_Tcutil_rk;_tmpDC;});Cyc_Absyn_new_evar(_tmp2B1,tvs);});});_tmpDA(_tmpDB);});void*_tmpDD=({({struct Cyc_Core_Opt*_tmp2B2=({struct Cyc_Core_Opt*_tmpDE=_cycalloc(sizeof(*_tmpDE));_tmpDE->v=& Cyc_Tcutil_rk;_tmpDE;});Cyc_Absyn_new_evar(_tmp2B2,tvs);});});struct Cyc_Absyn_Tqual _tmpDF=({Cyc_Absyn_const_tqual(0U);});void*_tmpE0=({Cyc_Tcutil_any_bool((struct Cyc_List_List*)tvs->v);});_tmpD8(_tmpD9,_tmpDD,_tmpDF,_tmpE0);});_tmpD5(_tmpD6,_tmpD7);}))
# 332
({struct Cyc_String_pa_PrintArg_struct _tmpE3=({struct Cyc_String_pa_PrintArg_struct _tmp22C;_tmp22C.tag=0U,({
struct _fat_ptr _tmp2B9=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t2);}));_tmp22C.f1=_tmp2B9;});_tmp22C;});void*_tmpE1[1U];_tmpE1[0]=& _tmpE3;({unsigned _tmp2BB=loc;struct _fat_ptr _tmp2BA=({const char*_tmpE2="second argument of main has type %s instead of char??";_tag_fat(_tmpE2,sizeof(char),54U);});Cyc_Tcutil_terr(_tmp2BB,_tmp2BA,_tag_fat(_tmpE1,sizeof(void*),1U));});});}}}}}}}
# 340
static void Cyc_Tc_tcFndecl(struct Cyc_Tcenv_Tenv*te,unsigned loc,struct Cyc_Absyn_Fndecl*fd,struct Cyc_List_List**exports){
# 343
struct _tuple0*q=fd->name;
if(({Cyc_Absyn_get_debug();})){
# 346
({struct Cyc_String_pa_PrintArg_struct _tmpE8=({struct Cyc_String_pa_PrintArg_struct _tmp22F;_tmp22F.tag=0U,_tmp22F.f1=(struct _fat_ptr)((struct _fat_ptr)*(*q).f2);_tmp22F;});struct Cyc_String_pa_PrintArg_struct _tmpE9=({struct Cyc_String_pa_PrintArg_struct _tmp22E;_tmp22E.tag=0U,({struct _fat_ptr _tmp2BC=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_effect2string((fd->i).ieffect);}));_tmp22E.f1=_tmp2BC;});_tmp22E;});struct Cyc_String_pa_PrintArg_struct _tmpEA=({struct Cyc_String_pa_PrintArg_struct _tmp22D;_tmp22D.tag=0U,({
struct _fat_ptr _tmp2BD=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_effect2string((fd->i).oeffect);}));_tmp22D.f1=_tmp2BD;});_tmp22D;});void*_tmpE6[3U];_tmpE6[0]=& _tmpE8,_tmpE6[1]=& _tmpE9,_tmpE6[2]=& _tmpEA;({struct Cyc___cycFILE*_tmp2BF=Cyc_stderr;struct _fat_ptr _tmp2BE=({const char*_tmpE7="\nChecking function declaration %s @ieffect(%s) @oeffect(%s)";_tag_fat(_tmpE7,sizeof(char),60U);});Cyc_fprintf(_tmp2BF,_tmp2BE,_tag_fat(_tmpE6,sizeof(void*),3U));});});
({Cyc_fflush(Cyc_stderr);});}
# 344
if(
# 350
(int)fd->sc == (int)Cyc_Absyn_ExternC && !te->in_extern_c_include)
({void*_tmpEB=0U;({unsigned _tmp2C1=loc;struct _fat_ptr _tmp2C0=({const char*_tmpEC="extern \"C\" functions cannot be implemented in Cyclone";_tag_fat(_tmpEC,sizeof(char),54U);});Cyc_Tcutil_terr(_tmp2C1,_tmp2C0,_tag_fat(_tmpEB,sizeof(void*),0U));});});{
# 344
void*cyc_eff=(fd->i).effect;
# 353
{void*_tmpED=cyc_eff;if(_tmpED != 0){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpED)->tag == 0U){if(((struct Cyc_Absyn_JoinCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpED)->f1)->tag == 11U){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpED)->f2 == 0){_LL1: _LL2:
# 355
 cyc_eff=0;
goto _LL0;}else{goto _LL3;}}else{goto _LL3;}}else{goto _LL3;}}else{_LL3: _LL4:
 goto _LL0;}_LL0:;}
# 359
if(({Cyc_Tc_is_main(q);})&& cyc_eff != 0)
({struct Cyc_String_pa_PrintArg_struct _tmpF0=({struct Cyc_String_pa_PrintArg_struct _tmp230;_tmp230.tag=0U,({
struct _fat_ptr _tmp2C2=(struct _fat_ptr)((struct _fat_ptr)(cyc_eff != 0?({Cyc_Absynpp_typ2string(cyc_eff);}):(struct _fat_ptr)({const char*_tmpF1="{}";_tag_fat(_tmpF1,sizeof(char),3U);})));_tmp230.f1=_tmp2C2;});_tmp230;});void*_tmpEE[1U];_tmpEE[0]=& _tmpF0;({unsigned _tmp2C4=loc;struct _fat_ptr _tmp2C3=({const char*_tmpEF="function main must be declared with  empty effects. Found effect = %s.";_tag_fat(_tmpEF,sizeof(char),71U);});Cyc_Tcutil_terr(_tmp2C4,_tmp2C3,_tag_fat(_tmpEE,sizeof(void*),1U));});});{
# 359
void*t=({Cyc_Tcstmt_tcFndecl_valid_type(te,loc,fd);});
# 364
({Cyc_Tc_fnTypeAttsOK(te,t,loc);});
# 366
{struct _handler_cons _tmpF2;_push_handler(& _tmpF2);{int _tmpF4=0;if(setjmp(_tmpF2.handler))_tmpF4=1;if(!_tmpF4){
# 368
{struct _tuple15*ans=({({struct _tuple15*(*_tmp2C6)(struct Cyc_Dict_Dict d,struct _tuple0*k)=({struct _tuple15*(*_tmpF8)(struct Cyc_Dict_Dict d,struct _tuple0*k)=(struct _tuple15*(*)(struct Cyc_Dict_Dict d,struct _tuple0*k))Cyc_Dict_lookup;_tmpF8;});struct Cyc_Dict_Dict _tmp2C5=(te->ae)->ordinaries;_tmp2C6(_tmp2C5,q);});});
void*b0=(*ans).f1;
struct Cyc_Absyn_Funname_b_Absyn_Binding_struct*b1=({struct Cyc_Absyn_Funname_b_Absyn_Binding_struct*_tmpF7=_cycalloc(sizeof(*_tmpF7));_tmpF7->tag=2U,_tmpF7->f1=fd;_tmpF7;});
void*b=({Cyc_Tcdecl_merge_binding(b0,(void*)b1,loc,Cyc_Tc_tc_msg);});
if(b != 0){
# 375
if(exports == 0 ||({Cyc_Tc_export_member(q,*exports);})){
if(!(b == b0 &&(*ans).f2))
({struct Cyc_Dict_Dict _tmp2CA=({({struct Cyc_Dict_Dict(*_tmp2C9)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct _tuple15*v)=({struct Cyc_Dict_Dict(*_tmpF5)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct _tuple15*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct _tuple15*v))Cyc_Dict_insert;_tmpF5;});struct Cyc_Dict_Dict _tmp2C8=(te->ae)->ordinaries;struct _tuple0*_tmp2C7=q;_tmp2C9(_tmp2C8,_tmp2C7,({struct _tuple15*_tmpF6=_cycalloc(sizeof(*_tmpF6));_tmpF6->f1=b,_tmpF6->f2=(*ans).f2;_tmpF6;}));});});(te->ae)->ordinaries=_tmp2CA;});}}}
# 368
;_pop_handler();}else{void*_tmpF3=(void*)Cyc_Core_get_exn_thrown();void*_tmpF9=_tmpF3;void*_tmpFA;if(((struct Cyc_Dict_Absent_exn_struct*)_tmpF9)->tag == Cyc_Dict_Absent){_LL6: _LL7:
# 383
 if(exports == 0 ||({Cyc_Tc_export_member(fd->name,*exports);})){
struct Cyc_Absyn_Funname_b_Absyn_Binding_struct*b=({struct Cyc_Absyn_Funname_b_Absyn_Binding_struct*_tmpFE=_cycalloc(sizeof(*_tmpFE));_tmpFE->tag=2U,_tmpFE->f1=fd;_tmpFE;});
({struct Cyc_Dict_Dict _tmp2CF=({({struct Cyc_Dict_Dict(*_tmp2CE)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct _tuple15*v)=({struct Cyc_Dict_Dict(*_tmpFB)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct _tuple15*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct _tuple15*v))Cyc_Dict_insert;_tmpFB;});struct Cyc_Dict_Dict _tmp2CD=(te->ae)->ordinaries;struct _tuple0*_tmp2CC=q;_tmp2CE(_tmp2CD,_tmp2CC,({struct _tuple15*_tmpFD=_cycalloc(sizeof(*_tmpFD));({void*_tmp2CB=(void*)({struct Cyc_Absyn_Funname_b_Absyn_Binding_struct*_tmpFC=_cycalloc(sizeof(*_tmpFC));_tmpFC->tag=2U,_tmpFC->f1=fd;_tmpFC;});_tmpFD->f1=_tmp2CB;}),_tmpFD->f2=0;_tmpFD;}));});});(te->ae)->ordinaries=_tmp2CF;});}
# 383
goto _LL5;}else{_LL8: _tmpFA=_tmpF9;_LL9: {void*exn=_tmpFA;(int)_rethrow(exn);}}_LL5:;}}}
# 391
if(te->in_extern_c_include)return;if(({Cyc_Tc_is_main(q);})){
# 394
struct Cyc_List_List*ieff=(fd->i).ieffect;
struct Cyc_List_List*oeff=(fd->i).oeffect;
if(ieff != 0 || oeff != 0)
({struct Cyc_String_pa_PrintArg_struct _tmp101=({struct Cyc_String_pa_PrintArg_struct _tmp232;_tmp232.tag=0U,({
struct _fat_ptr _tmp2D0=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_effect2string((fd->i).ieffect);}));_tmp232.f1=_tmp2D0;});_tmp232;});struct Cyc_String_pa_PrintArg_struct _tmp102=({struct Cyc_String_pa_PrintArg_struct _tmp231;_tmp231.tag=0U,({
struct _fat_ptr _tmp2D1=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_effect2string((fd->i).oeffect);}));_tmp231.f1=_tmp2D1;});_tmp231;});void*_tmpFF[2U];_tmpFF[0]=& _tmp101,_tmpFF[1]=& _tmp102;({unsigned _tmp2D3=loc;struct _fat_ptr _tmp2D2=({const char*_tmp100="function main must be declared with  empty effects. Found  @ieffect = %s @oeffect = %s";_tag_fat(_tmp100,sizeof(char),87U);});Cyc_Tcutil_terr(_tmp2D3,_tmp2D2,_tag_fat(_tmpFF,sizeof(void*),2U));});});
# 396
if(({Cyc_Absyn_is_reentrant((fd->i).reentrant);}))
# 404
({void*_tmp103=0U;({unsigned _tmp2D5=loc;struct _fat_ptr _tmp2D4=({const char*_tmp104="function main must not be @re_entrant";_tag_fat(_tmp104,sizeof(char),38U);});Cyc_Tcutil_terr(_tmp2D5,_tmp2D4,_tag_fat(_tmp103,sizeof(void*),0U));});});}
# 391
({void(*_tmp105)(unsigned loc,struct Cyc_Tcenv_Tenv*te,void*,struct Cyc_Absyn_Fndecl*fd)=Cyc_Tcstmt_tcFndecl_check_body;unsigned _tmp106=loc;struct Cyc_Tcenv_Tenv*_tmp107=({Cyc_Tcenv_new_fenv_in_env(loc,te,fd,t);});void*_tmp108=t;struct Cyc_Absyn_Fndecl*_tmp109=fd;_tmp105(_tmp106,_tmp107,_tmp108,_tmp109);});
# 412
if(({Cyc_Tcenv_in_tempest(te);})){
# 414
te->tempest_generalize=1;
({Cyc_Tctyp_check_fndecl_valid_type(loc,te,fd);});
te->tempest_generalize=0;}
# 412
if(({Cyc_Tc_is_main(q);}))
# 418
({Cyc_Tc_tcFndecl_check_main(loc,fd);});}}}
# 422
static void Cyc_Tc_tcTypedefdecl(struct Cyc_Tcenv_Tenv*te,unsigned loc,struct Cyc_Absyn_Typedefdecl*td){
struct _tuple0*q=td->name;
# 429
if(({({int(*_tmp2D7)(struct Cyc_Dict_Dict d,struct _tuple0*k)=({int(*_tmp10B)(struct Cyc_Dict_Dict d,struct _tuple0*k)=(int(*)(struct Cyc_Dict_Dict d,struct _tuple0*k))Cyc_Dict_member;_tmp10B;});struct Cyc_Dict_Dict _tmp2D6=(te->ae)->typedefs;_tmp2D7(_tmp2D6,q);});})){
({struct Cyc_String_pa_PrintArg_struct _tmp10E=({struct Cyc_String_pa_PrintArg_struct _tmp233;_tmp233.tag=0U,_tmp233.f1=(struct _fat_ptr)((struct _fat_ptr)*(*q).f2);_tmp233;});void*_tmp10C[1U];_tmp10C[0]=& _tmp10E;({unsigned _tmp2D9=loc;struct _fat_ptr _tmp2D8=({const char*_tmp10D="redeclaration of typedef %s";_tag_fat(_tmp10D,sizeof(char),28U);});Cyc_Tcutil_terr(_tmp2D9,_tmp2D8,_tag_fat(_tmp10C,sizeof(void*),1U));});});
return;}
# 429
({Cyc_Tcutil_check_unique_tvars(loc,td->tvs);});
# 435
({Cyc_Tcutil_add_tvar_identities(td->tvs);});
if(td->defn != 0){
({Cyc_Tctyp_check_type(loc,te,td->tvs,& Cyc_Tcutil_tak,0,1,(void*)_check_null(td->defn));});
({int _tmp2DA=({Cyc_Tcutil_extract_const_from_typedef(loc,(td->tq).print_const,(void*)_check_null(td->defn));});(td->tq).real_const=_tmp2DA;});}
# 436
{
# 443
struct Cyc_List_List*tvs=td->tvs;
# 436
for(0;tvs != 0;tvs=tvs->tl){
# 444
void*_stmttmpE=({Cyc_Absyn_compress_kb(((struct Cyc_Absyn_Tvar*)tvs->hd)->kind);});void*_tmp10F=_stmttmpE;struct Cyc_Absyn_Kind*_tmp111;struct Cyc_Core_Opt**_tmp110;struct Cyc_Core_Opt**_tmp112;switch(*((int*)_tmp10F)){case 1U: _LL1: _tmp112=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*)_tmp10F)->f1;_LL2: {struct Cyc_Core_Opt**f=_tmp112;
# 446
if(td->defn != 0)
({struct Cyc_String_pa_PrintArg_struct _tmp115=({struct Cyc_String_pa_PrintArg_struct _tmp234;_tmp234.tag=0U,_tmp234.f1=(struct _fat_ptr)((struct _fat_ptr)*((struct Cyc_Absyn_Tvar*)tvs->hd)->name);_tmp234;});void*_tmp113[1U];_tmp113[0]=& _tmp115;({unsigned _tmp2DC=loc;struct _fat_ptr _tmp2DB=({const char*_tmp114="type variable %s is not used in typedef definition";_tag_fat(_tmp114,sizeof(char),51U);});Cyc_Tcutil_warn(_tmp2DC,_tmp2DB,_tag_fat(_tmp113,sizeof(void*),1U));});});
# 446
({struct Cyc_Core_Opt*_tmp2DE=({struct Cyc_Core_Opt*_tmp116=_cycalloc(sizeof(*_tmp116));({
# 449
void*_tmp2DD=({Cyc_Tcutil_kind_to_bound(& Cyc_Tcutil_tbk);});_tmp116->v=_tmp2DD;});_tmp116;});
# 446
*f=_tmp2DE;});
# 449
goto _LL0;}case 2U: _LL3: _tmp110=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp10F)->f1;_tmp111=((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp10F)->f2;_LL4: {struct Cyc_Core_Opt**f=_tmp110;struct Cyc_Absyn_Kind*k=_tmp111;
# 456
({struct Cyc_Core_Opt*_tmp2E0=({struct Cyc_Core_Opt*_tmp117=_cycalloc(sizeof(*_tmp117));({void*_tmp2DF=({Cyc_Tcutil_kind_to_bound(k);});_tmp117->v=_tmp2DF;});_tmp117;});*f=_tmp2E0;});
goto _LL0;}default: _LL5: _LL6:
 continue;}_LL0:;}}
# 462
({struct Cyc_Dict_Dict _tmp2E4=({({struct Cyc_Dict_Dict(*_tmp2E3)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Typedefdecl*v)=({struct Cyc_Dict_Dict(*_tmp118)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Typedefdecl*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Typedefdecl*v))Cyc_Dict_insert;_tmp118;});struct Cyc_Dict_Dict _tmp2E2=(te->ae)->typedefs;struct _tuple0*_tmp2E1=q;_tmp2E3(_tmp2E2,_tmp2E1,td);});});(te->ae)->typedefs=_tmp2E4;});}struct _tuple16{void*f1;void*f2;};
# 465
static void Cyc_Tc_tcAggrImpl(struct Cyc_Tcenv_Tenv*te,unsigned loc,enum Cyc_Absyn_AggrKind str_or_union,struct Cyc_List_List*tvs,struct Cyc_List_List*rpo,struct Cyc_List_List*fields){
# 472
struct _RegionHandle _tmp11A=_new_region("uprev_rgn");struct _RegionHandle*uprev_rgn=& _tmp11A;_push_region(uprev_rgn);
# 474
for(0;rpo != 0;rpo=rpo->tl){
struct _tuple16*_stmttmpF=(struct _tuple16*)rpo->hd;void*_tmp11C;void*_tmp11B;_LL1: _tmp11B=_stmttmpF->f1;_tmp11C=_stmttmpF->f2;_LL2: {void*e=_tmp11B;void*r=_tmp11C;
({Cyc_Tctyp_check_type(loc,te,tvs,& Cyc_Tcutil_ek,0,0,e);});
({Cyc_Tctyp_check_type(loc,te,tvs,& Cyc_Tcutil_trk,0,0,r);});}}{
# 480
struct Cyc_List_List*prev_fields=0;
struct Cyc_List_List*prev_relations=0;
# 483
struct Cyc_List_List*fs=fields;for(0;fs != 0;fs=fs->tl){
struct Cyc_Absyn_Aggrfield*_stmttmp10=(struct Cyc_Absyn_Aggrfield*)fs->hd;struct Cyc_Absyn_Exp*_tmp122;struct Cyc_List_List*_tmp121;struct Cyc_Absyn_Exp*_tmp120;void*_tmp11F;struct Cyc_Absyn_Tqual _tmp11E;struct _fat_ptr*_tmp11D;_LL4: _tmp11D=_stmttmp10->name;_tmp11E=_stmttmp10->tq;_tmp11F=_stmttmp10->type;_tmp120=_stmttmp10->width;_tmp121=_stmttmp10->attributes;_tmp122=_stmttmp10->requires_clause;_LL5: {struct _fat_ptr*fn=_tmp11D;struct Cyc_Absyn_Tqual tq=_tmp11E;void*t=_tmp11F;struct Cyc_Absyn_Exp*width=_tmp120;struct Cyc_List_List*atts=_tmp121;struct Cyc_Absyn_Exp*requires_clause=_tmp122;
# 486
if(({({int(*_tmp2E6)(int(*compare)(struct _fat_ptr*,struct _fat_ptr*),struct Cyc_List_List*l,struct _fat_ptr*x)=({int(*_tmp123)(int(*compare)(struct _fat_ptr*,struct _fat_ptr*),struct Cyc_List_List*l,struct _fat_ptr*x)=(int(*)(int(*compare)(struct _fat_ptr*,struct _fat_ptr*),struct Cyc_List_List*l,struct _fat_ptr*x))Cyc_List_mem;_tmp123;});struct Cyc_List_List*_tmp2E5=prev_fields;_tmp2E6(Cyc_strptrcmp,_tmp2E5,fn);});}))
({struct Cyc_String_pa_PrintArg_struct _tmp126=({struct Cyc_String_pa_PrintArg_struct _tmp235;_tmp235.tag=0U,_tmp235.f1=(struct _fat_ptr)((struct _fat_ptr)*fn);_tmp235;});void*_tmp124[1U];_tmp124[0]=& _tmp126;({unsigned _tmp2E8=loc;struct _fat_ptr _tmp2E7=({const char*_tmp125="duplicate member %s";_tag_fat(_tmp125,sizeof(char),20U);});Cyc_Tcutil_terr(_tmp2E8,_tmp2E7,_tag_fat(_tmp124,sizeof(void*),1U));});});
# 486
if(({({struct _fat_ptr _tmp2E9=(struct _fat_ptr)*fn;Cyc_strcmp(_tmp2E9,({const char*_tmp127="";_tag_fat(_tmp127,sizeof(char),1U);}));});})!= 0)
# 491
prev_fields=({struct Cyc_List_List*_tmp128=_region_malloc(uprev_rgn,sizeof(*_tmp128));_tmp128->hd=fn,_tmp128->tl=prev_fields;_tmp128;});{
# 486
struct Cyc_Absyn_Kind*field_kind=& Cyc_Tcutil_tmk;
# 497
if((int)str_or_union == (int)1U ||
 fs->tl == 0 &&(int)str_or_union == (int)0U)
field_kind=& Cyc_Tcutil_tak;
# 497
({Cyc_Tctyp_check_type(loc,te,tvs,field_kind,0,0,t);});
# 502
({int _tmp2EA=({Cyc_Tcutil_extract_const_from_typedef(loc,(((struct Cyc_Absyn_Aggrfield*)fs->hd)->tq).print_const,t);});(((struct Cyc_Absyn_Aggrfield*)fs->hd)->tq).real_const=_tmp2EA;});
# 505
({Cyc_Tcutil_check_bitfield(loc,t,width,fn);});
# 507
if((unsigned)requires_clause){
if((int)str_or_union != (int)1U)
({void*_tmp129=0U;({unsigned _tmp2EC=loc;struct _fat_ptr _tmp2EB=({const char*_tmp12A="@requires clauses are only allowed on union members";_tag_fat(_tmp12A,sizeof(char),52U);});Cyc_Tcutil_terr(_tmp2EC,_tmp2EB,_tag_fat(_tmp129,sizeof(void*),0U));});});{
# 508
struct Cyc_Tcenv_Tenv*te2=({Cyc_Tcenv_allow_valueof(te);});
# 511
({Cyc_Tcexp_tcExp(te2,0,requires_clause);});
if(!({Cyc_Tcutil_is_integral(requires_clause);}))
({struct Cyc_String_pa_PrintArg_struct _tmp12D=({struct Cyc_String_pa_PrintArg_struct _tmp236;_tmp236.tag=0U,({
# 515
struct _fat_ptr _tmp2ED=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string((void*)_check_null(requires_clause->topt));}));_tmp236.f1=_tmp2ED;});_tmp236;});void*_tmp12B[1U];_tmp12B[0]=& _tmp12D;({unsigned _tmp2EF=requires_clause->loc;struct _fat_ptr _tmp2EE=({const char*_tmp12C="@requires clause has type %s instead of integral type";_tag_fat(_tmp12C,sizeof(char),54U);});Cyc_Tcutil_terr(_tmp2EF,_tmp2EE,_tag_fat(_tmp12B,sizeof(void*),1U));});});else{
# 517
({({unsigned _tmp2F2=requires_clause->loc;struct Cyc_Tcenv_Tenv*_tmp2F1=te;struct Cyc_List_List*_tmp2F0=tvs;Cyc_Tctyp_check_type(_tmp2F2,_tmp2F1,_tmp2F0,& Cyc_Tcutil_ik,0,0,(void*)({struct Cyc_Absyn_ValueofType_Absyn_Type_struct*_tmp12E=_cycalloc(sizeof(*_tmp12E));_tmp12E->tag=9U,_tmp12E->f1=requires_clause;_tmp12E;}));});});{
# 519
struct Cyc_List_List*relns=({Cyc_Relations_exp2relns(uprev_rgn,requires_clause);});
# 526
if(!({Cyc_Relations_consistent_relations(relns);}))
({void*_tmp12F=0U;({unsigned _tmp2F4=requires_clause->loc;struct _fat_ptr _tmp2F3=({const char*_tmp130="@requires clause may be unsatisfiable";_tag_fat(_tmp130,sizeof(char),38U);});Cyc_Tcutil_terr(_tmp2F4,_tmp2F3,_tag_fat(_tmp12F,sizeof(void*),0U));});});
# 526
{
# 532
struct Cyc_List_List*p=prev_relations;
# 526
for(0;p != 0;p=p->tl){
# 533
if(({int(*_tmp131)(struct Cyc_List_List*rlns)=Cyc_Relations_consistent_relations;struct Cyc_List_List*_tmp132=({Cyc_List_rappend(uprev_rgn,relns,(struct Cyc_List_List*)p->hd);});_tmp131(_tmp132);}))
({void*_tmp133=0U;({unsigned _tmp2F6=requires_clause->loc;struct _fat_ptr _tmp2F5=({const char*_tmp134="@requires clause may overlap with previous clauses";_tag_fat(_tmp134,sizeof(char),51U);});Cyc_Tcutil_terr(_tmp2F6,_tmp2F5,_tag_fat(_tmp133,sizeof(void*),0U));});});}}
# 537
prev_relations=({struct Cyc_List_List*_tmp135=_region_malloc(uprev_rgn,sizeof(*_tmp135));_tmp135->hd=relns,_tmp135->tl=prev_relations;_tmp135;});}}}}else{
# 540
if(prev_relations != 0)
({void*_tmp136=0U;({unsigned _tmp2F8=loc;struct _fat_ptr _tmp2F7=({const char*_tmp137="if one field has a @requires clause, they all must";_tag_fat(_tmp137,sizeof(char),51U);});Cyc_Tcutil_terr(_tmp2F8,_tmp2F7,_tag_fat(_tmp136,sizeof(void*),0U));});});}}}}}
# 474
;_pop_region();}
# 546
static void Cyc_Tc_rule_out_memkind(unsigned loc,struct _tuple0*n,struct Cyc_List_List*tvs,int constrain_top_kind){
# 548
struct Cyc_List_List*tvs2=tvs;for(0;tvs2 != 0;tvs2=tvs2->tl){
void*_stmttmp11=({Cyc_Absyn_compress_kb(((struct Cyc_Absyn_Tvar*)tvs2->hd)->kind);});void*_tmp139=_stmttmp11;enum Cyc_Absyn_AliasQual _tmp13A;struct Cyc_Absyn_Kind*_tmp13C;struct Cyc_Core_Opt**_tmp13B;struct Cyc_Core_Opt**_tmp13D;enum Cyc_Absyn_AliasQual _tmp13F;struct Cyc_Core_Opt**_tmp13E;struct Cyc_Core_Opt**_tmp140;switch(*((int*)_tmp139)){case 1U: _LL1: _tmp140=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*)_tmp139)->f1;_LL2: {struct Cyc_Core_Opt**f=_tmp140;
# 551
({struct Cyc_Core_Opt*_tmp2FA=({struct Cyc_Core_Opt*_tmp141=_cycalloc(sizeof(*_tmp141));({void*_tmp2F9=({Cyc_Tcutil_kind_to_bound(& Cyc_Tcutil_bk);});_tmp141->v=_tmp2F9;});_tmp141;});*f=_tmp2FA;});continue;}case 2U: switch(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp139)->f2)->kind){case Cyc_Absyn_MemKind: _LL3: _tmp13E=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp139)->f1;_tmp13F=(((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp139)->f2)->aliasqual;_LL4: {struct Cyc_Core_Opt**f=_tmp13E;enum Cyc_Absyn_AliasQual a=_tmp13F;
# 553
if(constrain_top_kind &&(int)a == (int)2U)
({struct Cyc_Core_Opt*_tmp2FC=({struct Cyc_Core_Opt*_tmp143=_cycalloc(sizeof(*_tmp143));({void*_tmp2FB=({Cyc_Tcutil_kind_to_bound(({struct Cyc_Absyn_Kind*_tmp142=_cycalloc(sizeof(*_tmp142));_tmp142->kind=Cyc_Absyn_BoxKind,_tmp142->aliasqual=Cyc_Absyn_Aliasable;_tmp142;}));});_tmp143->v=_tmp2FB;});_tmp143;});*f=_tmp2FC;});else{
# 556
({struct Cyc_Core_Opt*_tmp2FE=({struct Cyc_Core_Opt*_tmp145=_cycalloc(sizeof(*_tmp145));({void*_tmp2FD=({Cyc_Tcutil_kind_to_bound(({struct Cyc_Absyn_Kind*_tmp144=_cycalloc(sizeof(*_tmp144));_tmp144->kind=Cyc_Absyn_BoxKind,_tmp144->aliasqual=a;_tmp144;}));});_tmp145->v=_tmp2FD;});_tmp145;});*f=_tmp2FE;});}
continue;}case Cyc_Absyn_BoxKind: if(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp139)->f2)->aliasqual == Cyc_Absyn_Top){_LL5: _tmp13D=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp139)->f1;if(constrain_top_kind){_LL6: {struct Cyc_Core_Opt**f=_tmp13D;
# 559
({struct Cyc_Core_Opt*_tmp300=({struct Cyc_Core_Opt*_tmp147=_cycalloc(sizeof(*_tmp147));({void*_tmp2FF=({Cyc_Tcutil_kind_to_bound(({struct Cyc_Absyn_Kind*_tmp146=_cycalloc(sizeof(*_tmp146));_tmp146->kind=Cyc_Absyn_BoxKind,_tmp146->aliasqual=Cyc_Absyn_Aliasable;_tmp146;}));});_tmp147->v=_tmp2FF;});_tmp147;});*f=_tmp300;});
continue;}}else{goto _LL7;}}else{goto _LL7;}default: _LL7: _tmp13B=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp139)->f1;_tmp13C=((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp139)->f2;_LL8: {struct Cyc_Core_Opt**f=_tmp13B;struct Cyc_Absyn_Kind*k=_tmp13C;
# 562
({struct Cyc_Core_Opt*_tmp302=({struct Cyc_Core_Opt*_tmp148=_cycalloc(sizeof(*_tmp148));({void*_tmp301=({Cyc_Tcutil_kind_to_bound(k);});_tmp148->v=_tmp301;});_tmp148;});*f=_tmp302;});continue;}}default: if(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp139)->f1)->kind == Cyc_Absyn_MemKind){_LL9: _tmp13A=(((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp139)->f1)->aliasqual;_LLA: {enum Cyc_Absyn_AliasQual a=_tmp13A;
# 564
({struct Cyc_String_pa_PrintArg_struct _tmp14B=({struct Cyc_String_pa_PrintArg_struct _tmp239;_tmp239.tag=0U,_tmp239.f1=(struct _fat_ptr)((struct _fat_ptr)*(*n).f2);_tmp239;});struct Cyc_String_pa_PrintArg_struct _tmp14C=({struct Cyc_String_pa_PrintArg_struct _tmp238;_tmp238.tag=0U,_tmp238.f1=(struct _fat_ptr)((struct _fat_ptr)*((struct Cyc_Absyn_Tvar*)tvs2->hd)->name);_tmp238;});struct Cyc_String_pa_PrintArg_struct _tmp14D=({struct Cyc_String_pa_PrintArg_struct _tmp237;_tmp237.tag=0U,({
struct _fat_ptr _tmp303=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_kind2string(({struct Cyc_Absyn_Kind*_tmp14E=_cycalloc(sizeof(*_tmp14E));_tmp14E->kind=Cyc_Absyn_MemKind,_tmp14E->aliasqual=a;_tmp14E;}));}));_tmp237.f1=_tmp303;});_tmp237;});void*_tmp149[3U];_tmp149[0]=& _tmp14B,_tmp149[1]=& _tmp14C,_tmp149[2]=& _tmp14D;({unsigned _tmp305=loc;struct _fat_ptr _tmp304=({const char*_tmp14A="type %s attempts to abstract type variable %s of kind %s";_tag_fat(_tmp14A,sizeof(char),57U);});Cyc_Tcutil_terr(_tmp305,_tmp304,_tag_fat(_tmp149,sizeof(void*),3U));});});
continue;}}else{_LLB: _LLC:
 continue;}}_LL0:;}}struct _tuple17{struct Cyc_Absyn_AggrdeclImpl*f1;struct Cyc_Absyn_Aggrdecl***f2;};
# 572
void Cyc_Tc_tcAggrdecl(struct Cyc_Tcenv_Tenv*te,unsigned loc,struct Cyc_Absyn_Aggrdecl*ad){
struct _tuple0*q=ad->name;
# 579
{struct Cyc_List_List*atts=ad->attributes;for(0;atts != 0;atts=atts->tl){
void*_stmttmp12=(void*)atts->hd;void*_tmp150=_stmttmp12;switch(*((int*)_tmp150)){case 7U: _LL1: _LL2:
 goto _LL4;case 6U: _LL3: _LL4:
 continue;default: _LL5: _LL6:
# 584
({struct Cyc_String_pa_PrintArg_struct _tmp153=({struct Cyc_String_pa_PrintArg_struct _tmp23B;_tmp23B.tag=0U,({
struct _fat_ptr _tmp306=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_attribute2string((void*)atts->hd);}));_tmp23B.f1=_tmp306;});_tmp23B;});struct Cyc_String_pa_PrintArg_struct _tmp154=({struct Cyc_String_pa_PrintArg_struct _tmp23A;_tmp23A.tag=0U,_tmp23A.f1=(struct _fat_ptr)((struct _fat_ptr)*(*q).f2);_tmp23A;});void*_tmp151[2U];_tmp151[0]=& _tmp153,_tmp151[1]=& _tmp154;({unsigned _tmp308=loc;struct _fat_ptr _tmp307=({const char*_tmp152="bad attribute %s in %s definition";_tag_fat(_tmp152,sizeof(char),34U);});Cyc_Tcutil_terr(_tmp308,_tmp307,_tag_fat(_tmp151,sizeof(void*),2U));});});
goto _LL0;}_LL0:;}}{
# 589
struct Cyc_List_List*tvs=ad->tvs;
# 592
({Cyc_Tcutil_check_unique_tvars(loc,ad->tvs);});
({Cyc_Tcutil_add_tvar_identities(ad->tvs);});{
# 597
struct _tuple17 _stmttmp13=({struct _tuple17 _tmp23F;_tmp23F.f1=ad->impl,({struct Cyc_Absyn_Aggrdecl***_tmp30B=({({struct Cyc_Absyn_Aggrdecl***(*_tmp30A)(struct Cyc_Dict_Dict d,struct _tuple0*k)=({struct Cyc_Absyn_Aggrdecl***(*_tmp17F)(struct Cyc_Dict_Dict d,struct _tuple0*k)=(struct Cyc_Absyn_Aggrdecl***(*)(struct Cyc_Dict_Dict d,struct _tuple0*k))Cyc_Dict_lookup_opt;_tmp17F;});struct Cyc_Dict_Dict _tmp309=(te->ae)->aggrdecls;_tmp30A(_tmp309,q);});});_tmp23F.f2=_tmp30B;});_tmp23F;});struct _tuple17 _tmp155=_stmttmp13;struct Cyc_Absyn_Aggrdecl**_tmp15A;int _tmp159;struct Cyc_List_List*_tmp158;struct Cyc_List_List*_tmp157;struct Cyc_List_List*_tmp156;int _tmp15E;struct Cyc_List_List*_tmp15D;struct Cyc_List_List*_tmp15C;struct Cyc_List_List*_tmp15B;struct Cyc_Absyn_Aggrdecl**_tmp15F;if(_tmp155.f1 == 0){if(_tmp155.f2 == 0){_LL8: _LL9:
# 600
({Cyc_Tc_rule_out_memkind(loc,q,tvs,0);});
# 602
({struct Cyc_Dict_Dict _tmp30F=({({struct Cyc_Dict_Dict(*_tmp30E)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Aggrdecl**v)=({struct Cyc_Dict_Dict(*_tmp160)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Aggrdecl**v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Aggrdecl**v))Cyc_Dict_insert;_tmp160;});struct Cyc_Dict_Dict _tmp30D=(te->ae)->aggrdecls;struct _tuple0*_tmp30C=q;_tmp30E(_tmp30D,_tmp30C,({struct Cyc_Absyn_Aggrdecl**_tmp161=_cycalloc(sizeof(*_tmp161));*_tmp161=ad;_tmp161;}));});});(te->ae)->aggrdecls=_tmp30F;});
goto _LL7;}else{_LLE: _tmp15F=*_tmp155.f2;_LLF: {struct Cyc_Absyn_Aggrdecl**adp=_tmp15F;
# 662
struct Cyc_Absyn_Aggrdecl*ad2=({Cyc_Tcdecl_merge_aggrdecl(*adp,ad,loc,Cyc_Tc_tc_msg);});
if(ad2 == 0)
return;else{
# 666
({Cyc_Tc_rule_out_memkind(loc,q,tvs,0);});
# 669
if(ad->impl != 0)
({Cyc_Tc_rule_out_memkind(loc,q,((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->exist_vars,1);});
# 669
*adp=ad2;
# 673
ad=ad2;
goto _LL7;}}}}else{if(_tmp155.f2 == 0){_LLA: _tmp15B=(_tmp155.f1)->exist_vars;_tmp15C=(_tmp155.f1)->rgn_po;_tmp15D=(_tmp155.f1)->fields;_tmp15E=(_tmp155.f1)->tagged;_LLB: {struct Cyc_List_List*exist_vars=_tmp15B;struct Cyc_List_List*rgn_po=_tmp15C;struct Cyc_List_List*fs=_tmp15D;int tagged=_tmp15E;
# 607
struct Cyc_Absyn_Aggrdecl**adp=({struct Cyc_Absyn_Aggrdecl**_tmp172=_cycalloc(sizeof(*_tmp172));({struct Cyc_Absyn_Aggrdecl*_tmp310=({struct Cyc_Absyn_Aggrdecl*_tmp171=_cycalloc(sizeof(*_tmp171));_tmp171->kind=ad->kind,_tmp171->sc=Cyc_Absyn_Extern,_tmp171->name=ad->name,_tmp171->tvs=tvs,_tmp171->impl=0,_tmp171->attributes=ad->attributes,_tmp171->expected_mem_kind=0;_tmp171;});*_tmp172=_tmp310;});_tmp172;});
# 609
({struct Cyc_Dict_Dict _tmp314=({({struct Cyc_Dict_Dict(*_tmp313)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Aggrdecl**v)=({struct Cyc_Dict_Dict(*_tmp162)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Aggrdecl**v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Aggrdecl**v))Cyc_Dict_insert;_tmp162;});struct Cyc_Dict_Dict _tmp312=(te->ae)->aggrdecls;struct _tuple0*_tmp311=q;_tmp313(_tmp312,_tmp311,adp);});});(te->ae)->aggrdecls=_tmp314;});
# 614
({Cyc_Tcutil_check_unique_tvars(loc,exist_vars);});
({Cyc_Tcutil_add_tvar_identities(exist_vars);});
# 618
if(tagged &&(int)ad->kind == (int)Cyc_Absyn_StructA)
({void*_tmp163=0U;({unsigned _tmp316=loc;struct _fat_ptr _tmp315=({const char*_tmp164="@tagged qualifier is only allowed on union declarations";_tag_fat(_tmp164,sizeof(char),56U);});Cyc_Tcutil_terr(_tmp316,_tmp315,_tag_fat(_tmp163,sizeof(void*),0U));});});
# 618
({void(*_tmp165)(struct Cyc_Tcenv_Tenv*te,unsigned loc,enum Cyc_Absyn_AggrKind str_or_union,struct Cyc_List_List*tvs,struct Cyc_List_List*rpo,struct Cyc_List_List*fields)=Cyc_Tc_tcAggrImpl;struct Cyc_Tcenv_Tenv*_tmp166=te;unsigned _tmp167=loc;enum Cyc_Absyn_AggrKind _tmp168=ad->kind;struct Cyc_List_List*_tmp169=({Cyc_List_append(tvs,exist_vars);});struct Cyc_List_List*_tmp16A=rgn_po;struct Cyc_List_List*_tmp16B=fs;_tmp165(_tmp166,_tmp167,_tmp168,_tmp169,_tmp16A,_tmp16B);});
# 622
({Cyc_Tc_rule_out_memkind(loc,q,tvs,0);});
# 625
({Cyc_Tc_rule_out_memkind(loc,q,exist_vars,1);});
# 627
if((int)ad->kind == (int)Cyc_Absyn_UnionA && !tagged){
# 630
struct Cyc_List_List*f=fs;for(0;f != 0;f=f->tl){
if((Cyc_Tc_aggressive_warn && !({Cyc_Tcutil_is_bits_only_type(((struct Cyc_Absyn_Aggrfield*)f->hd)->type);}))&&((struct Cyc_Absyn_Aggrfield*)f->hd)->requires_clause == 0)
# 633
({struct Cyc_String_pa_PrintArg_struct _tmp16E=({struct Cyc_String_pa_PrintArg_struct _tmp23E;_tmp23E.tag=0U,_tmp23E.f1=(struct _fat_ptr)((struct _fat_ptr)*((struct Cyc_Absyn_Aggrfield*)f->hd)->name);_tmp23E;});struct Cyc_String_pa_PrintArg_struct _tmp16F=({struct Cyc_String_pa_PrintArg_struct _tmp23D;_tmp23D.tag=0U,_tmp23D.f1=(struct _fat_ptr)((struct _fat_ptr)*(*q).f2);_tmp23D;});struct Cyc_String_pa_PrintArg_struct _tmp170=({struct Cyc_String_pa_PrintArg_struct _tmp23C;_tmp23C.tag=0U,({
struct _fat_ptr _tmp317=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(((struct Cyc_Absyn_Aggrfield*)f->hd)->type);}));_tmp23C.f1=_tmp317;});_tmp23C;});void*_tmp16C[3U];_tmp16C[0]=& _tmp16E,_tmp16C[1]=& _tmp16F,_tmp16C[2]=& _tmp170;({unsigned _tmp319=loc;struct _fat_ptr _tmp318=({const char*_tmp16D="member %s of union %s has type %s which is not `bits-only' so it can only be written and not read";_tag_fat(_tmp16D,sizeof(char),98U);});Cyc_Tcutil_warn(_tmp319,_tmp318,_tag_fat(_tmp16C,sizeof(void*),3U));});});}}
# 627
*adp=ad;
# 637
goto _LL7;}}else{_LLC: _tmp156=(_tmp155.f1)->exist_vars;_tmp157=(_tmp155.f1)->rgn_po;_tmp158=(_tmp155.f1)->fields;_tmp159=(_tmp155.f1)->tagged;_tmp15A=*_tmp155.f2;_LLD: {struct Cyc_List_List*exist_vars=_tmp156;struct Cyc_List_List*rgn_po=_tmp157;struct Cyc_List_List*fs=_tmp158;int tagged=_tmp159;struct Cyc_Absyn_Aggrdecl**adp=_tmp15A;
# 640
if((int)ad->kind != (int)(*adp)->kind)
({void*_tmp173=0U;({unsigned _tmp31B=loc;struct _fat_ptr _tmp31A=({const char*_tmp174="cannot reuse struct names for unions and vice-versa";_tag_fat(_tmp174,sizeof(char),52U);});Cyc_Tcutil_terr(_tmp31B,_tmp31A,_tag_fat(_tmp173,sizeof(void*),0U));});});{
# 640
struct Cyc_Absyn_Aggrdecl*ad0=*adp;
# 645
({struct Cyc_Absyn_Aggrdecl*_tmp31C=({struct Cyc_Absyn_Aggrdecl*_tmp175=_cycalloc(sizeof(*_tmp175));_tmp175->kind=ad->kind,_tmp175->sc=Cyc_Absyn_Extern,_tmp175->name=ad->name,_tmp175->tvs=tvs,_tmp175->impl=0,_tmp175->attributes=ad->attributes,_tmp175->expected_mem_kind=0;_tmp175;});*adp=_tmp31C;});
# 651
({Cyc_Tcutil_check_unique_tvars(loc,exist_vars);});
({Cyc_Tcutil_add_tvar_identities(exist_vars);});
# 654
if(tagged &&(int)ad->kind == (int)Cyc_Absyn_StructA)
({void*_tmp176=0U;({unsigned _tmp31E=loc;struct _fat_ptr _tmp31D=({const char*_tmp177="@tagged qualifier is only allowed on union declarations";_tag_fat(_tmp177,sizeof(char),56U);});Cyc_Tcutil_terr(_tmp31E,_tmp31D,_tag_fat(_tmp176,sizeof(void*),0U));});});
# 654
({void(*_tmp178)(struct Cyc_Tcenv_Tenv*te,unsigned loc,enum Cyc_Absyn_AggrKind str_or_union,struct Cyc_List_List*tvs,struct Cyc_List_List*rpo,struct Cyc_List_List*fields)=Cyc_Tc_tcAggrImpl;struct Cyc_Tcenv_Tenv*_tmp179=te;unsigned _tmp17A=loc;enum Cyc_Absyn_AggrKind _tmp17B=ad->kind;struct Cyc_List_List*_tmp17C=({Cyc_List_append(tvs,exist_vars);});struct Cyc_List_List*_tmp17D=rgn_po;struct Cyc_List_List*_tmp17E=fs;_tmp178(_tmp179,_tmp17A,_tmp17B,_tmp17C,_tmp17D,_tmp17E);});
# 658
*adp=ad0;
_tmp15F=adp;goto _LLF;}}}}_LL7:;}}}
# 679
static void Cyc_Tc_rule_out_mem_and_unique(unsigned loc,struct _tuple0*q,struct Cyc_List_List*tvs){
struct Cyc_List_List*tvs2=tvs;for(0;tvs2 != 0;tvs2=tvs2->tl){
void*_stmttmp14=({Cyc_Absyn_compress_kb(((struct Cyc_Absyn_Tvar*)tvs2->hd)->kind);});void*_tmp181=_stmttmp14;enum Cyc_Absyn_AliasQual _tmp182;enum Cyc_Absyn_KindQual _tmp183;enum Cyc_Absyn_KindQual _tmp185;struct Cyc_Core_Opt**_tmp184;struct Cyc_Core_Opt**_tmp186;struct Cyc_Core_Opt**_tmp187;struct Cyc_Core_Opt**_tmp188;struct Cyc_Core_Opt**_tmp189;struct Cyc_Core_Opt**_tmp18A;struct Cyc_Core_Opt**_tmp18B;switch(*((int*)_tmp181)){case 1U: _LL1: _tmp18B=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*)_tmp181)->f1;_LL2: {struct Cyc_Core_Opt**f=_tmp18B;
_tmp18A=f;goto _LL4;}case 2U: switch(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp181)->f2)->kind){case Cyc_Absyn_MemKind: switch(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp181)->f2)->aliasqual){case Cyc_Absyn_Top: _LL3: _tmp18A=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp181)->f1;_LL4: {struct Cyc_Core_Opt**f=_tmp18A;
# 684
_tmp189=f;goto _LL6;}case Cyc_Absyn_Aliasable: _LL5: _tmp189=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp181)->f1;_LL6: {struct Cyc_Core_Opt**f=_tmp189;
# 686
({struct Cyc_Core_Opt*_tmp320=({struct Cyc_Core_Opt*_tmp18C=_cycalloc(sizeof(*_tmp18C));({void*_tmp31F=({Cyc_Tcutil_kind_to_bound(& Cyc_Tcutil_bk);});_tmp18C->v=_tmp31F;});_tmp18C;});*f=_tmp320;});goto _LL0;}case Cyc_Absyn_Unique: goto _LLF;default: goto _LL15;}case Cyc_Absyn_AnyKind: switch(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp181)->f2)->aliasqual){case Cyc_Absyn_Top: _LL7: _tmp188=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp181)->f1;_LL8: {struct Cyc_Core_Opt**f=_tmp188;
_tmp187=f;goto _LLA;}case Cyc_Absyn_Aliasable: _LL9: _tmp187=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp181)->f1;_LLA: {struct Cyc_Core_Opt**f=_tmp187;
# 689
({struct Cyc_Core_Opt*_tmp322=({struct Cyc_Core_Opt*_tmp18D=_cycalloc(sizeof(*_tmp18D));({void*_tmp321=({Cyc_Tcutil_kind_to_bound(& Cyc_Tcutil_ak);});_tmp18D->v=_tmp321;});_tmp18D;});*f=_tmp322;});goto _LL0;}case Cyc_Absyn_Unique: goto _LLF;default: goto _LL15;}case Cyc_Absyn_RgnKind: switch(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp181)->f2)->aliasqual){case Cyc_Absyn_Top: _LLB: _tmp186=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp181)->f1;_LLC: {struct Cyc_Core_Opt**f=_tmp186;
# 691
({struct Cyc_Core_Opt*_tmp324=({struct Cyc_Core_Opt*_tmp18E=_cycalloc(sizeof(*_tmp18E));({void*_tmp323=({Cyc_Tcutil_kind_to_bound(& Cyc_Tcutil_rk);});_tmp18E->v=_tmp323;});_tmp18E;});*f=_tmp324;});goto _LL0;}case Cyc_Absyn_Unique: goto _LLF;default: goto _LL15;}default: if(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp181)->f2)->aliasqual == Cyc_Absyn_Unique){_LLF: _tmp184=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp181)->f1;_tmp185=(((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp181)->f2)->kind;_LL10: {struct Cyc_Core_Opt**f=_tmp184;enum Cyc_Absyn_KindQual k=_tmp185;
# 695
_tmp183=k;goto _LL12;}}else{goto _LL15;}}default: if(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp181)->f1)->kind == Cyc_Absyn_RgnKind)switch(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp181)->f1)->aliasqual){case Cyc_Absyn_Top: _LLD: _LLE:
# 693
({struct Cyc_String_pa_PrintArg_struct _tmp191=({struct Cyc_String_pa_PrintArg_struct _tmp241;_tmp241.tag=0U,_tmp241.f1=(struct _fat_ptr)((struct _fat_ptr)*(*q).f2);_tmp241;});struct Cyc_String_pa_PrintArg_struct _tmp192=({struct Cyc_String_pa_PrintArg_struct _tmp240;_tmp240.tag=0U,_tmp240.f1=(struct _fat_ptr)((struct _fat_ptr)*((struct Cyc_Absyn_Tvar*)tvs2->hd)->name);_tmp240;});void*_tmp18F[2U];_tmp18F[0]=& _tmp191,_tmp18F[1]=& _tmp192;({unsigned _tmp326=loc;struct _fat_ptr _tmp325=({const char*_tmp190="type %s attempts to abstract type variable %s of kind TR";_tag_fat(_tmp190,sizeof(char),57U);});Cyc_Tcutil_terr(_tmp326,_tmp325,_tag_fat(_tmp18F,sizeof(void*),2U));});});
goto _LL0;case Cyc_Absyn_Unique: goto _LL11;default: goto _LL15;}else{if(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp181)->f1)->aliasqual == Cyc_Absyn_Unique){_LL11: _tmp183=(((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp181)->f1)->kind;_LL12: {enum Cyc_Absyn_KindQual k=_tmp183;
# 697
({struct Cyc_String_pa_PrintArg_struct _tmp195=({struct Cyc_String_pa_PrintArg_struct _tmp244;_tmp244.tag=0U,_tmp244.f1=(struct _fat_ptr)((struct _fat_ptr)*(*q).f2);_tmp244;});struct Cyc_String_pa_PrintArg_struct _tmp196=({struct Cyc_String_pa_PrintArg_struct _tmp243;_tmp243.tag=0U,_tmp243.f1=(struct _fat_ptr)((struct _fat_ptr)*((struct Cyc_Absyn_Tvar*)tvs2->hd)->name);_tmp243;});struct Cyc_String_pa_PrintArg_struct _tmp197=({struct Cyc_String_pa_PrintArg_struct _tmp242;_tmp242.tag=0U,({
# 699
struct _fat_ptr _tmp327=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_kind2string(({struct Cyc_Absyn_Kind*_tmp198=_cycalloc(sizeof(*_tmp198));_tmp198->kind=k,_tmp198->aliasqual=Cyc_Absyn_Unique;_tmp198;}));}));_tmp242.f1=_tmp327;});_tmp242;});void*_tmp193[3U];_tmp193[0]=& _tmp195,_tmp193[1]=& _tmp196,_tmp193[2]=& _tmp197;({unsigned _tmp329=loc;struct _fat_ptr _tmp328=({const char*_tmp194="type %s attempts to abstract type variable %s of kind %s";_tag_fat(_tmp194,sizeof(char),57U);});Cyc_Tcutil_terr(_tmp329,_tmp328,_tag_fat(_tmp193,sizeof(void*),3U));});});goto _LL0;}}else{if(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp181)->f1)->kind == Cyc_Absyn_MemKind){_LL13: _tmp182=(((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp181)->f1)->aliasqual;_LL14: {enum Cyc_Absyn_AliasQual a=_tmp182;
# 701
({struct Cyc_String_pa_PrintArg_struct _tmp19B=({struct Cyc_String_pa_PrintArg_struct _tmp247;_tmp247.tag=0U,_tmp247.f1=(struct _fat_ptr)((struct _fat_ptr)*(*q).f2);_tmp247;});struct Cyc_String_pa_PrintArg_struct _tmp19C=({struct Cyc_String_pa_PrintArg_struct _tmp246;_tmp246.tag=0U,_tmp246.f1=(struct _fat_ptr)((struct _fat_ptr)*((struct Cyc_Absyn_Tvar*)tvs2->hd)->name);_tmp246;});struct Cyc_String_pa_PrintArg_struct _tmp19D=({struct Cyc_String_pa_PrintArg_struct _tmp245;_tmp245.tag=0U,({
# 703
struct _fat_ptr _tmp32A=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_kind2string(({struct Cyc_Absyn_Kind*_tmp19E=_cycalloc(sizeof(*_tmp19E));_tmp19E->kind=Cyc_Absyn_MemKind,_tmp19E->aliasqual=a;_tmp19E;}));}));_tmp245.f1=_tmp32A;});_tmp245;});void*_tmp199[3U];_tmp199[0]=& _tmp19B,_tmp199[1]=& _tmp19C,_tmp199[2]=& _tmp19D;({unsigned _tmp32C=loc;struct _fat_ptr _tmp32B=({const char*_tmp19A="type %s attempts to abstract type variable %s of kind %s";_tag_fat(_tmp19A,sizeof(char),57U);});Cyc_Tcutil_terr(_tmp32C,_tmp32B,_tag_fat(_tmp199,sizeof(void*),3U));});});goto _LL0;}}else{_LL15: _LL16:
 goto _LL0;}}}}_LL0:;}}struct _tuple18{struct Cyc_Absyn_Tqual f1;void*f2;};
# 709
static struct Cyc_List_List*Cyc_Tc_tcDatatypeFields(struct Cyc_Tcenv_Tenv*te,unsigned loc,struct _fat_ptr obj,int is_extensible,struct _tuple0*name,struct Cyc_List_List*fields,struct Cyc_List_List*tvs,struct Cyc_Absyn_Datatypedecl*tudres){
# 717
{struct Cyc_List_List*fs=fields;for(0;fs != 0;fs=fs->tl){
struct Cyc_Absyn_Datatypefield*f=(struct Cyc_Absyn_Datatypefield*)fs->hd;
struct Cyc_List_List*typs=f->typs;for(0;typs != 0;typs=typs->tl){
({Cyc_Tctyp_check_type(f->loc,te,tvs,& Cyc_Tcutil_tmk,0,0,(*((struct _tuple18*)typs->hd)).f2);});
# 722
if(({Cyc_Tcutil_is_noalias_pointer_or_aggr((*((struct _tuple18*)typs->hd)).f2);}))
({struct Cyc_String_pa_PrintArg_struct _tmp1A2=({struct Cyc_String_pa_PrintArg_struct _tmp248;_tmp248.tag=0U,({
# 725
struct _fat_ptr _tmp32D=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(f->name);}));_tmp248.f1=_tmp32D;});_tmp248;});void*_tmp1A0[1U];_tmp1A0[0]=& _tmp1A2;({unsigned _tmp32F=f->loc;struct _fat_ptr _tmp32E=({const char*_tmp1A1="noalias pointers in datatypes are not allowed (%s)";_tag_fat(_tmp1A1,sizeof(char),51U);});Cyc_Tcutil_terr(_tmp32F,_tmp32E,_tag_fat(_tmp1A0,sizeof(void*),1U));});});
# 722
({int _tmp330=({Cyc_Tcutil_extract_const_from_typedef(f->loc,((*((struct _tuple18*)typs->hd)).f1).print_const,(*((struct _tuple18*)typs->hd)).f2);});((*((struct _tuple18*)typs->hd)).f1).real_const=_tmp330;});}}}{
# 732
struct Cyc_List_List*fields2;
if(is_extensible){
# 735
int res=1;
struct Cyc_List_List*fs=({Cyc_Tcdecl_sort_xdatatype_fields(fields,& res,(*name).f2,loc,Cyc_Tc_tc_msg);});
if(res)
fields2=fs;else{
# 740
fields2=0;}}else{
# 742
struct _RegionHandle _tmp1A3=_new_region("uprev_rgn");struct _RegionHandle*uprev_rgn=& _tmp1A3;_push_region(uprev_rgn);
# 744
{struct Cyc_List_List*prev_fields=0;
{struct Cyc_List_List*fs=fields;for(0;fs != 0;fs=fs->tl){
struct Cyc_Absyn_Datatypefield*f=(struct Cyc_Absyn_Datatypefield*)fs->hd;
if(({({int(*_tmp332)(int(*compare)(struct _fat_ptr*,struct _fat_ptr*),struct Cyc_List_List*l,struct _fat_ptr*x)=({int(*_tmp1A4)(int(*compare)(struct _fat_ptr*,struct _fat_ptr*),struct Cyc_List_List*l,struct _fat_ptr*x)=(int(*)(int(*compare)(struct _fat_ptr*,struct _fat_ptr*),struct Cyc_List_List*l,struct _fat_ptr*x))Cyc_List_mem;_tmp1A4;});struct Cyc_List_List*_tmp331=prev_fields;_tmp332(Cyc_strptrcmp,_tmp331,(*f->name).f2);});}))
({struct Cyc_String_pa_PrintArg_struct _tmp1A7=({struct Cyc_String_pa_PrintArg_struct _tmp24A;_tmp24A.tag=0U,_tmp24A.f1=(struct _fat_ptr)((struct _fat_ptr)*(*f->name).f2);_tmp24A;});struct Cyc_String_pa_PrintArg_struct _tmp1A8=({struct Cyc_String_pa_PrintArg_struct _tmp249;_tmp249.tag=0U,_tmp249.f1=(struct _fat_ptr)((struct _fat_ptr)obj);_tmp249;});void*_tmp1A5[2U];_tmp1A5[0]=& _tmp1A7,_tmp1A5[1]=& _tmp1A8;({unsigned _tmp334=f->loc;struct _fat_ptr _tmp333=({const char*_tmp1A6="duplicate field name %s in %s";_tag_fat(_tmp1A6,sizeof(char),30U);});Cyc_Tcutil_terr(_tmp334,_tmp333,_tag_fat(_tmp1A5,sizeof(void*),2U));});});else{
# 750
prev_fields=({struct Cyc_List_List*_tmp1A9=_region_malloc(uprev_rgn,sizeof(*_tmp1A9));_tmp1A9->hd=(*f->name).f2,_tmp1A9->tl=prev_fields;_tmp1A9;});}
# 752
if((int)f->sc != (int)Cyc_Absyn_Public){
({struct Cyc_String_pa_PrintArg_struct _tmp1AC=({struct Cyc_String_pa_PrintArg_struct _tmp24B;_tmp24B.tag=0U,_tmp24B.f1=(struct _fat_ptr)((struct _fat_ptr)*(*f->name).f2);_tmp24B;});void*_tmp1AA[1U];_tmp1AA[0]=& _tmp1AC;({unsigned _tmp336=loc;struct _fat_ptr _tmp335=({const char*_tmp1AB="ignoring scope of field %s";_tag_fat(_tmp1AB,sizeof(char),27U);});Cyc_Tcutil_warn(_tmp336,_tmp335,_tag_fat(_tmp1AA,sizeof(void*),1U));});});
f->sc=Cyc_Absyn_Public;}}}
# 757
fields2=fields;}
# 744
;_pop_region();}
# 759
return fields2;}}struct _tuple19{struct Cyc_Core_Opt*f1;struct Cyc_Absyn_Datatypedecl***f2;};
# 762
void Cyc_Tc_tcDatatypedecl(struct Cyc_Tcenv_Tenv*te,unsigned loc,struct Cyc_Absyn_Datatypedecl*tud){
struct _tuple0*q=tud->name;
struct _fat_ptr obj=tud->is_extensible?({const char*_tmp1C1="@extensible datatype";_tag_fat(_tmp1C1,sizeof(char),21U);}):({const char*_tmp1C2="datatype";_tag_fat(_tmp1C2,sizeof(char),9U);});
# 769
struct Cyc_List_List*tvs=tud->tvs;
# 772
({Cyc_Tcutil_check_unique_tvars(loc,tvs);});
({Cyc_Tcutil_add_tvar_identities(tvs);});{
# 778
struct Cyc_Absyn_Datatypedecl***tud_opt;
# 788 "tc.cyc"
{struct _handler_cons _tmp1AE;_push_handler(& _tmp1AE);{int _tmp1B0=0;if(setjmp(_tmp1AE.handler))_tmp1B0=1;if(!_tmp1B0){
tud_opt=({Cyc_Tcenv_lookup_xdatatypedecl(Cyc_Core_heap_region,te,loc,tud->name);});
if(tud_opt == 0 && !tud->is_extensible)(int)_throw(({struct Cyc_Dict_Absent_exn_struct*_tmp1B1=_cycalloc(sizeof(*_tmp1B1));_tmp1B1->tag=Cyc_Dict_Absent;_tmp1B1;}));if(tud_opt != 0)
# 792
tud->name=(*(*tud_opt))->name;else{
# 794
({union Cyc_Absyn_Nmspace _tmp337=({Cyc_Absyn_Abs_n(te->ns,0);});(*tud->name).f1=_tmp337;});}
# 789
;_pop_handler();}else{void*_tmp1AF=(void*)Cyc_Core_get_exn_thrown();void*_tmp1B2=_tmp1AF;void*_tmp1B3;if(((struct Cyc_Dict_Absent_exn_struct*)_tmp1B2)->tag == Cyc_Dict_Absent){_LL1: _LL2: {
# 798
struct Cyc_Absyn_Datatypedecl***tdopt=({({struct Cyc_Absyn_Datatypedecl***(*_tmp339)(struct Cyc_Dict_Dict d,struct _tuple0*k)=({struct Cyc_Absyn_Datatypedecl***(*_tmp1B5)(struct Cyc_Dict_Dict d,struct _tuple0*k)=(struct Cyc_Absyn_Datatypedecl***(*)(struct Cyc_Dict_Dict d,struct _tuple0*k))Cyc_Dict_lookup_opt;_tmp1B5;});struct Cyc_Dict_Dict _tmp338=(te->ae)->datatypedecls;_tmp339(_tmp338,q);});});
tud_opt=(unsigned)tdopt?({struct Cyc_Absyn_Datatypedecl***_tmp1B4=_cycalloc(sizeof(*_tmp1B4));*_tmp1B4=*tdopt;_tmp1B4;}): 0;
goto _LL0;}}else{_LL3: _tmp1B3=_tmp1B2;_LL4: {void*exn=_tmp1B3;(int)_rethrow(exn);}}_LL0:;}}}{
# 805
struct _tuple19 _stmttmp15=({struct _tuple19 _tmp24C;_tmp24C.f1=tud->fields,_tmp24C.f2=tud_opt;_tmp24C;});struct _tuple19 _tmp1B6=_stmttmp15;struct Cyc_Absyn_Datatypedecl**_tmp1B8;struct Cyc_List_List**_tmp1B7;struct Cyc_List_List**_tmp1B9;struct Cyc_Absyn_Datatypedecl**_tmp1BA;if(_tmp1B6.f1 == 0){if(_tmp1B6.f2 == 0){_LL6: _LL7:
# 808
({Cyc_Tc_rule_out_mem_and_unique(loc,q,tvs);});
({struct Cyc_Dict_Dict _tmp33D=({({struct Cyc_Dict_Dict(*_tmp33C)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Datatypedecl**v)=({struct Cyc_Dict_Dict(*_tmp1BB)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Datatypedecl**v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Datatypedecl**v))Cyc_Dict_insert;_tmp1BB;});struct Cyc_Dict_Dict _tmp33B=(te->ae)->datatypedecls;struct _tuple0*_tmp33A=q;_tmp33C(_tmp33B,_tmp33A,({struct Cyc_Absyn_Datatypedecl**_tmp1BC=_cycalloc(sizeof(*_tmp1BC));*_tmp1BC=tud;_tmp1BC;}));});});(te->ae)->datatypedecls=_tmp33D;});
goto _LL5;}else{_LLC: _tmp1BA=*_tmp1B6.f2;_LLD: {struct Cyc_Absyn_Datatypedecl**tudp=_tmp1BA;
# 841
struct Cyc_Absyn_Datatypedecl*tud2=({Cyc_Tcdecl_merge_datatypedecl(*tudp,tud,loc,Cyc_Tc_tc_msg);});
({Cyc_Tc_rule_out_mem_and_unique(loc,q,tvs);});
if(tud2 == 0)
return;else{
# 846
*tudp=tud2;
goto _LL5;}}}}else{if(_tmp1B6.f2 == 0){_LL8: _tmp1B9=(struct Cyc_List_List**)&(_tmp1B6.f1)->v;_LL9: {struct Cyc_List_List**fs=_tmp1B9;
# 813
struct Cyc_Absyn_Datatypedecl**tudp=({struct Cyc_Absyn_Datatypedecl**_tmp1BF=_cycalloc(sizeof(*_tmp1BF));({struct Cyc_Absyn_Datatypedecl*_tmp33E=({struct Cyc_Absyn_Datatypedecl*_tmp1BE=_cycalloc(sizeof(*_tmp1BE));_tmp1BE->sc=Cyc_Absyn_Extern,_tmp1BE->name=tud->name,_tmp1BE->tvs=tvs,_tmp1BE->fields=0,_tmp1BE->is_extensible=tud->is_extensible;_tmp1BE;});*_tmp1BF=_tmp33E;});_tmp1BF;});
# 815
({struct Cyc_Dict_Dict _tmp342=({({struct Cyc_Dict_Dict(*_tmp341)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Datatypedecl**v)=({struct Cyc_Dict_Dict(*_tmp1BD)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Datatypedecl**v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Datatypedecl**v))Cyc_Dict_insert;_tmp1BD;});struct Cyc_Dict_Dict _tmp340=(te->ae)->datatypedecls;struct _tuple0*_tmp33F=q;_tmp341(_tmp340,_tmp33F,tudp);});});(te->ae)->datatypedecls=_tmp342;});
# 818
({struct Cyc_List_List*_tmp343=({Cyc_Tc_tcDatatypeFields(te,loc,obj,tud->is_extensible,tud->name,*fs,tvs,tud);});*fs=_tmp343;});
({Cyc_Tc_rule_out_mem_and_unique(loc,q,tvs);});
*tudp=tud;
goto _LL5;}}else{_LLA: _tmp1B7=(struct Cyc_List_List**)&(_tmp1B6.f1)->v;_tmp1B8=*_tmp1B6.f2;_LLB: {struct Cyc_List_List**fs=_tmp1B7;struct Cyc_Absyn_Datatypedecl**tudp=_tmp1B8;
# 823
struct Cyc_Absyn_Datatypedecl*tud0=*tudp;
# 826
if((!tud->is_extensible &&(unsigned)tud0)&& tud0->is_extensible)
tud->is_extensible=1;
# 826
({struct Cyc_Absyn_Datatypedecl*_tmp344=({struct Cyc_Absyn_Datatypedecl*_tmp1C0=_cycalloc(sizeof(*_tmp1C0));
# 829
_tmp1C0->sc=Cyc_Absyn_Extern,_tmp1C0->name=tud->name,_tmp1C0->tvs=tvs,_tmp1C0->fields=0,_tmp1C0->is_extensible=tud->is_extensible;_tmp1C0;});
# 826
*tudp=_tmp344;});
# 833
({struct Cyc_List_List*_tmp345=({Cyc_Tc_tcDatatypeFields(te,loc,obj,tud->is_extensible,tud->name,*fs,tvs,tud);});*fs=_tmp345;});
# 836
*tudp=tud0;
_tmp1BA=tudp;goto _LLD;}}}_LL5:;}}}
# 852
void Cyc_Tc_tcEnumdecl(struct Cyc_Tcenv_Tenv*te,unsigned loc,struct Cyc_Absyn_Enumdecl*ed){
struct _tuple0*q=ed->name;
# 859
if(ed->fields != 0){struct _RegionHandle _tmp1C4=_new_region("uprev_rgn");struct _RegionHandle*uprev_rgn=& _tmp1C4;_push_region(uprev_rgn);
{struct Cyc_List_List*prev_fields=0;
unsigned tag_count=0U;
struct Cyc_List_List*fs=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(ed->fields))->v;for(0;fs != 0;fs=fs->tl){
struct Cyc_Absyn_Enumfield*f=(struct Cyc_Absyn_Enumfield*)fs->hd;
# 865
if(({({int(*_tmp347)(int(*compare)(struct _fat_ptr*,struct _fat_ptr*),struct Cyc_List_List*l,struct _fat_ptr*x)=({int(*_tmp1C5)(int(*compare)(struct _fat_ptr*,struct _fat_ptr*),struct Cyc_List_List*l,struct _fat_ptr*x)=(int(*)(int(*compare)(struct _fat_ptr*,struct _fat_ptr*),struct Cyc_List_List*l,struct _fat_ptr*x))Cyc_List_mem;_tmp1C5;});struct Cyc_List_List*_tmp346=prev_fields;_tmp347(Cyc_strptrcmp,_tmp346,(*f->name).f2);});}))
({struct Cyc_String_pa_PrintArg_struct _tmp1C8=({struct Cyc_String_pa_PrintArg_struct _tmp24D;_tmp24D.tag=0U,_tmp24D.f1=(struct _fat_ptr)((struct _fat_ptr)*(*f->name).f2);_tmp24D;});void*_tmp1C6[1U];_tmp1C6[0]=& _tmp1C8;({unsigned _tmp349=f->loc;struct _fat_ptr _tmp348=({const char*_tmp1C7="duplicate enum constructor %s";_tag_fat(_tmp1C7,sizeof(char),30U);});Cyc_Tcutil_terr(_tmp349,_tmp348,_tag_fat(_tmp1C6,sizeof(void*),1U));});});else{
# 868
prev_fields=({struct Cyc_List_List*_tmp1C9=_region_malloc(uprev_rgn,sizeof(*_tmp1C9));_tmp1C9->hd=(*f->name).f2,_tmp1C9->tl=prev_fields;_tmp1C9;});}
# 870
if(({({struct _tuple15**(*_tmp34B)(struct Cyc_Dict_Dict d,struct _tuple0*k)=({struct _tuple15**(*_tmp1CA)(struct Cyc_Dict_Dict d,struct _tuple0*k)=(struct _tuple15**(*)(struct Cyc_Dict_Dict d,struct _tuple0*k))Cyc_Dict_lookup_opt;_tmp1CA;});struct Cyc_Dict_Dict _tmp34A=(te->ae)->ordinaries;_tmp34B(_tmp34A,f->name);});})!= 0)
({struct Cyc_String_pa_PrintArg_struct _tmp1CD=({struct Cyc_String_pa_PrintArg_struct _tmp24E;_tmp24E.tag=0U,_tmp24E.f1=(struct _fat_ptr)((struct _fat_ptr)*(*f->name).f2);_tmp24E;});void*_tmp1CB[1U];_tmp1CB[0]=& _tmp1CD;({unsigned _tmp34D=f->loc;struct _fat_ptr _tmp34C=({const char*_tmp1CC="enum field name %s shadows global name";_tag_fat(_tmp1CC,sizeof(char),39U);});Cyc_Tcutil_terr(_tmp34D,_tmp34C,_tag_fat(_tmp1CB,sizeof(void*),1U));});});
# 870
if(f->tag == 0){
# 874
({struct Cyc_Absyn_Exp*_tmp34E=({Cyc_Absyn_uint_exp(tag_count,f->loc);});f->tag=_tmp34E;});
++ tag_count;}else{
# 878
if(({Cyc_Tcutil_is_const_exp((struct Cyc_Absyn_Exp*)_check_null(f->tag));})){
struct _tuple12 _stmttmp16=({Cyc_Evexp_eval_const_uint_exp((struct Cyc_Absyn_Exp*)_check_null(f->tag));});int _tmp1CF;unsigned _tmp1CE;_LL1: _tmp1CE=_stmttmp16.f1;_tmp1CF=_stmttmp16.f2;_LL2: {unsigned t1=_tmp1CE;int known=_tmp1CF;
if(known)tag_count=t1 + (unsigned)1;}}}}}
# 860
;_pop_region();}
# 859
{struct _handler_cons _tmp1D0;_push_handler(& _tmp1D0);{int _tmp1D2=0;if(setjmp(_tmp1D0.handler))_tmp1D2=1;if(!_tmp1D2){
# 887
{struct Cyc_Absyn_Enumdecl**edp=({({struct Cyc_Absyn_Enumdecl**(*_tmp350)(struct Cyc_Dict_Dict d,struct _tuple0*k)=({struct Cyc_Absyn_Enumdecl**(*_tmp1D3)(struct Cyc_Dict_Dict d,struct _tuple0*k)=(struct Cyc_Absyn_Enumdecl**(*)(struct Cyc_Dict_Dict d,struct _tuple0*k))Cyc_Dict_lookup;_tmp1D3;});struct Cyc_Dict_Dict _tmp34F=(te->ae)->enumdecls;_tmp350(_tmp34F,q);});});
struct Cyc_Absyn_Enumdecl*ed2=({Cyc_Tcdecl_merge_enumdecl(*edp,ed,loc,Cyc_Tc_tc_msg);});
if(ed2 == 0){_npop_handler(0U);return;}*edp=ed2;}
# 887
;_pop_handler();}else{void*_tmp1D1=(void*)Cyc_Core_get_exn_thrown();void*_tmp1D4=_tmp1D1;void*_tmp1D5;if(((struct Cyc_Dict_Absent_exn_struct*)_tmp1D4)->tag == Cyc_Dict_Absent){_LL4: _LL5: {
# 892
struct Cyc_Absyn_Enumdecl**edp=({struct Cyc_Absyn_Enumdecl**_tmp1D7=_cycalloc(sizeof(*_tmp1D7));*_tmp1D7=ed;_tmp1D7;});
({struct Cyc_Dict_Dict _tmp354=({({struct Cyc_Dict_Dict(*_tmp353)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Enumdecl**v)=({struct Cyc_Dict_Dict(*_tmp1D6)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Enumdecl**v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Enumdecl**v))Cyc_Dict_insert;_tmp1D6;});struct Cyc_Dict_Dict _tmp352=(te->ae)->enumdecls;struct _tuple0*_tmp351=q;_tmp353(_tmp352,_tmp351,edp);});});(te->ae)->enumdecls=_tmp354;});
goto _LL3;}}else{_LL6: _tmp1D5=_tmp1D4;_LL7: {void*exn=_tmp1D5;(int)_rethrow(exn);}}_LL3:;}}}
# 898
if(ed->fields != 0){
struct Cyc_List_List*fs=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(ed->fields))->v;for(0;fs != 0;fs=fs->tl){
struct Cyc_Absyn_Enumfield*f=(struct Cyc_Absyn_Enumfield*)fs->hd;
({Cyc_Tcexp_tcExp(te,0,(struct Cyc_Absyn_Exp*)_check_null(f->tag));});
if(!({Cyc_Tcutil_is_const_exp((struct Cyc_Absyn_Exp*)_check_null(f->tag));}))
({struct Cyc_String_pa_PrintArg_struct _tmp1DA=({struct Cyc_String_pa_PrintArg_struct _tmp250;_tmp250.tag=0U,_tmp250.f1=(struct _fat_ptr)((struct _fat_ptr)*(*q).f2);_tmp250;});struct Cyc_String_pa_PrintArg_struct _tmp1DB=({struct Cyc_String_pa_PrintArg_struct _tmp24F;_tmp24F.tag=0U,_tmp24F.f1=(struct _fat_ptr)((struct _fat_ptr)*(*f->name).f2);_tmp24F;});void*_tmp1D8[2U];_tmp1D8[0]=& _tmp1DA,_tmp1D8[1]=& _tmp1DB;({unsigned _tmp356=loc;struct _fat_ptr _tmp355=({const char*_tmp1D9="enum %s, field %s: expression is not constant";_tag_fat(_tmp1D9,sizeof(char),46U);});Cyc_Tcutil_terr(_tmp356,_tmp355,_tag_fat(_tmp1D8,sizeof(void*),2U));});});}}}
# 908
static int Cyc_Tc_okay_externC(unsigned loc,enum Cyc_Absyn_Scope sc,int in_include){
enum Cyc_Absyn_Scope _tmp1DD=sc;switch(_tmp1DD){case Cyc_Absyn_Static: _LL1: _LL2:
# 911
 if(!in_include)
({void*_tmp1DE=0U;({unsigned _tmp358=loc;struct _fat_ptr _tmp357=({const char*_tmp1DF="static declaration within extern \"C\"";_tag_fat(_tmp1DF,sizeof(char),37U);});Cyc_Tcutil_warn(_tmp358,_tmp357,_tag_fat(_tmp1DE,sizeof(void*),0U));});});
# 911
return 0;case Cyc_Absyn_Abstract: _LL3: _LL4:
# 915
({void*_tmp1E0=0U;({unsigned _tmp35A=loc;struct _fat_ptr _tmp359=({const char*_tmp1E1="abstract declaration within extern \"C\"";_tag_fat(_tmp1E1,sizeof(char),39U);});Cyc_Tcutil_warn(_tmp35A,_tmp359,_tag_fat(_tmp1E0,sizeof(void*),0U));});});
return 0;case Cyc_Absyn_Public: _LL5: _LL6:
 goto _LL8;case Cyc_Absyn_Register: _LL7: _LL8:
 goto _LLA;case Cyc_Absyn_Extern: _LL9: _LLA:
 return 1;case Cyc_Absyn_ExternC: _LLB: _LLC:
 goto _LLE;default: _LLD: _LLE:
# 922
({void*_tmp1E2=0U;({unsigned _tmp35C=loc;struct _fat_ptr _tmp35B=({const char*_tmp1E3="nested extern \"C\" declaration";_tag_fat(_tmp1E3,sizeof(char),30U);});Cyc_Tcutil_warn(_tmp35C,_tmp35B,_tag_fat(_tmp1E2,sizeof(void*),0U));});});
return 1;}_LL0:;}
# 932
static void Cyc_Tc_tc_decls(struct Cyc_Tcenv_Tenv*te,struct Cyc_List_List*ds0,int in_externC,int check_var_init,struct Cyc_List_List**exports){
# 936
struct Cyc_List_List*ds=ds0;for(0;ds != 0;ds=ds->tl){
struct Cyc_Absyn_Decl*d=(struct Cyc_Absyn_Decl*)ds->hd;
unsigned loc=d->loc;
void*_stmttmp17=d->r;void*_tmp1E5=_stmttmp17;struct _tuple11*_tmp1E9;struct Cyc_List_List*_tmp1E8;struct Cyc_List_List*_tmp1E7;struct Cyc_List_List*_tmp1E6;struct Cyc_List_List*_tmp1EA;struct Cyc_List_List*_tmp1EB;struct Cyc_List_List*_tmp1ED;struct _fat_ptr*_tmp1EC;struct Cyc_Absyn_Enumdecl*_tmp1EE;struct Cyc_Absyn_Datatypedecl*_tmp1EF;struct Cyc_Absyn_Aggrdecl*_tmp1F0;struct Cyc_Absyn_Typedefdecl*_tmp1F1;struct Cyc_Absyn_Fndecl*_tmp1F2;struct Cyc_Absyn_Vardecl*_tmp1F3;switch(*((int*)_tmp1E5)){case 2U: _LL1: _LL2:
 goto _LL4;case 3U: _LL3: _LL4:
# 942
({void*_tmp1F4=0U;({unsigned _tmp35E=loc;struct _fat_ptr _tmp35D=({const char*_tmp1F5="top level let-declarations are not implemented";_tag_fat(_tmp1F5,sizeof(char),47U);});Cyc_Tcutil_terr(_tmp35E,_tmp35D,_tag_fat(_tmp1F4,sizeof(void*),0U));});});goto _LL0;case 4U: _LL5: _LL6:
# 944
({void*_tmp1F6=0U;({unsigned _tmp360=loc;struct _fat_ptr _tmp35F=({const char*_tmp1F7="top level region declarations are not implemented";_tag_fat(_tmp1F7,sizeof(char),50U);});Cyc_Tcutil_terr(_tmp360,_tmp35F,_tag_fat(_tmp1F6,sizeof(void*),0U));});});goto _LL0;case 0U: _LL7: _tmp1F3=((struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_tmp1E5)->f1;_LL8: {struct Cyc_Absyn_Vardecl*vd=_tmp1F3;
# 946
if(in_externC &&({Cyc_Tc_okay_externC(d->loc,vd->sc,te->in_extern_c_include);}))
vd->sc=Cyc_Absyn_ExternC;
# 946
({Cyc_Tc_tcVardecl(te,loc,vd,check_var_init,te->in_extern_c_include,exports);});
# 949
goto _LL0;}case 1U: _LL9: _tmp1F2=((struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*)_tmp1E5)->f1;_LLA: {struct Cyc_Absyn_Fndecl*fd=_tmp1F2;
# 951
if(in_externC &&({Cyc_Tc_okay_externC(d->loc,fd->sc,te->in_extern_c_include);}))
fd->sc=Cyc_Absyn_ExternC;
# 951
({Cyc_Tc_tcFndecl(te,loc,fd,exports);});
# 954
goto _LL0;}case 8U: _LLB: _tmp1F1=((struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct*)_tmp1E5)->f1;_LLC: {struct Cyc_Absyn_Typedefdecl*td=_tmp1F1;
# 956
td->extern_c=te->in_extern_c_include;
# 960
({Cyc_Tc_tcTypedefdecl(te,loc,td);});
goto _LL0;}case 5U: _LLD: _tmp1F0=((struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct*)_tmp1E5)->f1;_LLE: {struct Cyc_Absyn_Aggrdecl*ad=_tmp1F0;
# 963
if(in_externC &&({Cyc_Tc_okay_externC(d->loc,ad->sc,te->in_extern_c_include);}))
ad->sc=Cyc_Absyn_ExternC;
# 963
({Cyc_Tc_tcAggrdecl(te,loc,ad);});
# 966
goto _LL0;}case 6U: _LLF: _tmp1EF=((struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct*)_tmp1E5)->f1;_LL10: {struct Cyc_Absyn_Datatypedecl*tud=_tmp1EF;
# 968
if(in_externC &&({Cyc_Tc_okay_externC(d->loc,tud->sc,te->in_extern_c_include);}))
tud->sc=Cyc_Absyn_ExternC;
# 968
({Cyc_Tc_tcDatatypedecl(te,loc,tud);});
# 971
goto _LL0;}case 7U: _LL11: _tmp1EE=((struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct*)_tmp1E5)->f1;_LL12: {struct Cyc_Absyn_Enumdecl*ed=_tmp1EE;
# 973
if(in_externC &&({Cyc_Tc_okay_externC(d->loc,ed->sc,te->in_extern_c_include);}))
ed->sc=Cyc_Absyn_ExternC;
# 973
({Cyc_Tc_tcEnumdecl(te,loc,ed);});
# 976
goto _LL0;}case 13U: _LL13: _LL14:
({void*_tmp1F8=0U;({unsigned _tmp362=d->loc;struct _fat_ptr _tmp361=({const char*_tmp1F9="spurious __cyclone_port_on__";_tag_fat(_tmp1F9,sizeof(char),29U);});Cyc_Tcutil_warn(_tmp362,_tmp361,_tag_fat(_tmp1F8,sizeof(void*),0U));});});goto _LL0;case 14U: _LL15: _LL16:
 goto _LL0;case 15U: _LL17: _LL18:
 te=({Cyc_Tcenv_enter_tempest(te);});goto _LL0;case 16U: _LL19: _LL1A:
 te=({Cyc_Tcenv_clear_tempest(te);});goto _LL0;case 9U: _LL1B: _tmp1EC=((struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct*)_tmp1E5)->f1;_tmp1ED=((struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct*)_tmp1E5)->f2;_LL1C: {struct _fat_ptr*v=_tmp1EC;struct Cyc_List_List*ds2=_tmp1ED;
# 983
struct Cyc_List_List*ns=te->ns;
({struct Cyc_List_List*_tmp364=({({struct Cyc_List_List*_tmp363=ns;Cyc_List_append(_tmp363,({struct Cyc_List_List*_tmp1FA=_cycalloc(sizeof(*_tmp1FA));_tmp1FA->hd=v,_tmp1FA->tl=0;_tmp1FA;}));});});te->ns=_tmp364;});
({Cyc_Tc_tc_decls(te,ds2,in_externC,check_var_init,exports);});
te->ns=ns;
goto _LL0;}case 10U: _LL1D: _tmp1EB=((struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct*)_tmp1E5)->f2;_LL1E: {struct Cyc_List_List*ds2=_tmp1EB;
# 990
({Cyc_Tc_tc_decls(te,ds2,in_externC,check_var_init,exports);});goto _LL0;}case 11U: _LL1F: _tmp1EA=((struct Cyc_Absyn_ExternC_d_Absyn_Raw_decl_struct*)_tmp1E5)->f1;_LL20: {struct Cyc_List_List*ds2=_tmp1EA;
# 993
({Cyc_Tc_tc_decls(te,ds2,1,check_var_init,exports);});goto _LL0;}default: _LL21: _tmp1E6=((struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct*)_tmp1E5)->f1;_tmp1E7=((struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct*)_tmp1E5)->f2;_tmp1E8=((struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct*)_tmp1E5)->f3;_tmp1E9=((struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct*)_tmp1E5)->f4;_LL22: {struct Cyc_List_List*ds2=_tmp1E6;struct Cyc_List_List*ovrs=_tmp1E7;struct Cyc_List_List*exports2=_tmp1E8;struct _tuple11*wc=_tmp1E9;
# 998
struct Cyc_List_List*newexs=({Cyc_List_append(exports2,(unsigned)exports?*exports: 0);});
# 1000
struct Cyc_Tcenv_Tenv*te2=({Cyc_Tcenv_enter_extern_c_include(te);});
({Cyc_Tc_tc_decls(te2,ds2,1,check_var_init,& newexs);});
# 1003
for(0;exports2 != 0;exports2=exports2->tl){
struct _tuple13*exp=(struct _tuple13*)exports2->hd;
if(!(*exp).f3)
({struct Cyc_String_pa_PrintArg_struct _tmp1FD=({struct Cyc_String_pa_PrintArg_struct _tmp251;_tmp251.tag=0U,({
struct _fat_ptr _tmp365=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string((*exp).f2);}));_tmp251.f1=_tmp365;});_tmp251;});void*_tmp1FB[1U];_tmp1FB[0]=& _tmp1FD;({unsigned _tmp367=(*exp).f1;struct _fat_ptr _tmp366=({const char*_tmp1FC="%s is exported but not defined";_tag_fat(_tmp1FC,sizeof(char),31U);});Cyc_Tcutil_warn(_tmp367,_tmp366,_tag_fat(_tmp1FB,sizeof(void*),1U));});});}
# 1009
goto _LL0;}}_LL0:;}}
# 932
struct Cyc___cycFILE*Cyc_Tc_dotfile=0;
# 1016
void Cyc_Tc_tc(struct Cyc_Tcenv_Tenv*te,int check_var_init,struct Cyc_List_List*ds,int debug){
({Cyc_Absyn_set_debug(debug);});
({Cyc_Absynpp_set_params(& Cyc_Absynpp_tc_params_r);});{
struct Cyc_Dict_Dict cg=({Cyc_Callgraph_compute_callgraph(ds);});
# 1021
struct Cyc_Dict_Dict scc=({Cyc_Graph_scc(cg);});
# 1026
({Cyc_Tc_tc_decls(te,ds,0,check_var_init,0);});}}struct Cyc_Tc_TreeshakeEnv{int in_cinclude;struct Cyc_Dict_Dict ordinaries;};
# 1044 "tc.cyc"
static int Cyc_Tc_vardecl_needed(struct Cyc_Tc_TreeshakeEnv*env,struct Cyc_Absyn_Decl*d);
# 1046
static struct Cyc_List_List*Cyc_Tc_treeshake_f(struct Cyc_Tc_TreeshakeEnv*env,struct Cyc_List_List*ds){
return({({struct Cyc_List_List*(*_tmp369)(int(*f)(struct Cyc_Tc_TreeshakeEnv*,struct Cyc_Absyn_Decl*),struct Cyc_Tc_TreeshakeEnv*env,struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp200)(int(*f)(struct Cyc_Tc_TreeshakeEnv*,struct Cyc_Absyn_Decl*),struct Cyc_Tc_TreeshakeEnv*env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(int(*f)(struct Cyc_Tc_TreeshakeEnv*,struct Cyc_Absyn_Decl*),struct Cyc_Tc_TreeshakeEnv*env,struct Cyc_List_List*x))Cyc_List_filter_c;_tmp200;});struct Cyc_Tc_TreeshakeEnv*_tmp368=env;_tmp369(Cyc_Tc_vardecl_needed,_tmp368,ds);});});}
# 1050
static int Cyc_Tc_is_extern(struct Cyc_Absyn_Vardecl*vd){
if((int)vd->sc == (int)Cyc_Absyn_Extern ||(int)vd->sc == (int)Cyc_Absyn_ExternC)
return 1;{
# 1051
void*_stmttmp18=({Cyc_Tcutil_compress(vd->type);});void*_tmp202=_stmttmp18;if(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp202)->tag == 5U){_LL1: _LL2:
# 1054
 return 1;}else{_LL3: _LL4:
 return 0;}_LL0:;}}
# 1059
static int Cyc_Tc_vardecl_needed(struct Cyc_Tc_TreeshakeEnv*env,struct Cyc_Absyn_Decl*d){
void*_stmttmp19=d->r;void*_tmp204=_stmttmp19;struct Cyc_List_List**_tmp205;struct Cyc_List_List**_tmp206;struct Cyc_List_List**_tmp207;struct Cyc_List_List**_tmp208;struct Cyc_Absyn_Vardecl*_tmp209;switch(*((int*)_tmp204)){case 0U: _LL1: _tmp209=((struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_tmp204)->f1;_LL2: {struct Cyc_Absyn_Vardecl*vd=_tmp209;
# 1064
if((env->in_cinclude || !({Cyc_Tc_is_extern(vd);}))|| !({int(*_tmp20A)(struct _tuple0*,struct _tuple0*)=Cyc_Absyn_qvar_cmp;struct _tuple0*_tmp20B=vd->name;struct _tuple0*_tmp20C=({Cyc_Absyn_uniquergn_qvar();});_tmp20A(_tmp20B,_tmp20C);}))
# 1067
return 1;
# 1064
return(*({({struct _tuple15*(*_tmp36B)(struct Cyc_Dict_Dict d,struct _tuple0*k)=({
# 1069
struct _tuple15*(*_tmp20D)(struct Cyc_Dict_Dict d,struct _tuple0*k)=(struct _tuple15*(*)(struct Cyc_Dict_Dict d,struct _tuple0*k))Cyc_Dict_lookup;_tmp20D;});struct Cyc_Dict_Dict _tmp36A=env->ordinaries;_tmp36B(_tmp36A,vd->name);});})).f2;}case 11U: _LL3: _tmp208=(struct Cyc_List_List**)&((struct Cyc_Absyn_ExternC_d_Absyn_Raw_decl_struct*)_tmp204)->f1;_LL4: {struct Cyc_List_List**ds2p=_tmp208;
_tmp207=ds2p;goto _LL6;}case 10U: _LL5: _tmp207=(struct Cyc_List_List**)&((struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct*)_tmp204)->f2;_LL6: {struct Cyc_List_List**ds2p=_tmp207;
_tmp206=ds2p;goto _LL8;}case 9U: _LL7: _tmp206=(struct Cyc_List_List**)&((struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct*)_tmp204)->f2;_LL8: {struct Cyc_List_List**ds2p=_tmp206;
({struct Cyc_List_List*_tmp36C=({Cyc_Tc_treeshake_f(env,*ds2p);});*ds2p=_tmp36C;});return 1;}case 12U: _LL9: _tmp205=(struct Cyc_List_List**)&((struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct*)_tmp204)->f1;_LLA: {struct Cyc_List_List**ds2p=_tmp205;
# 1074
int in_cinclude=env->in_cinclude;
env->in_cinclude=1;
({struct Cyc_List_List*_tmp36D=({Cyc_Tc_treeshake_f(env,*ds2p);});*ds2p=_tmp36D;});
env->in_cinclude=in_cinclude;
return 1;}default: _LLB: _LLC:
 return 1;}_LL0:;}
# 1083
struct Cyc_List_List*Cyc_Tc_treeshake(struct Cyc_Tcenv_Tenv*te,struct Cyc_List_List*ds){
struct Cyc_Tc_TreeshakeEnv env=({struct Cyc_Tc_TreeshakeEnv _tmp252;_tmp252.in_cinclude=0,_tmp252.ordinaries=(te->ae)->ordinaries;_tmp252;});
return({Cyc_Tc_treeshake_f(& env,ds);});}

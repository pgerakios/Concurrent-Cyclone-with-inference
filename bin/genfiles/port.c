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
extern struct _RegionHandle*Cyc_Core_unique_region;struct Cyc_Core_DynamicRegion;struct Cyc_Core_NewDynamicRegion{struct Cyc_Core_DynamicRegion*key;};struct Cyc_Core_ThinRes{void*arr;unsigned nelts;};struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};
# 54 "list.h"
struct Cyc_List_List*Cyc_List_list(struct _fat_ptr);
# 76
struct Cyc_List_List*Cyc_List_map(void*(*f)(void*),struct Cyc_List_List*x);
# 83
struct Cyc_List_List*Cyc_List_map_c(void*(*f)(void*,void*),void*env,struct Cyc_List_List*x);extern char Cyc_List_List_mismatch[14U];struct Cyc_List_List_mismatch_exn_struct{char*tag;};
# 135
void Cyc_List_iter_c(void(*f)(void*,void*),void*env,struct Cyc_List_List*x);
# 178
struct Cyc_List_List*Cyc_List_imp_rev(struct Cyc_List_List*x);
# 210
struct Cyc_List_List*Cyc_List_merge_sort(int(*cmp)(void*,void*),struct Cyc_List_List*x);extern char Cyc_List_Nth[4U];struct Cyc_List_Nth_exn_struct{char*tag;};struct Cyc___cycFILE;
# 53 "cycboot.h"
extern struct Cyc___cycFILE*Cyc_stderr;struct Cyc_String_pa_PrintArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Int_pa_PrintArg_struct{int tag;unsigned long f1;};struct Cyc_Double_pa_PrintArg_struct{int tag;double f1;};struct Cyc_LongDouble_pa_PrintArg_struct{int tag;long double f1;};struct Cyc_ShortPtr_pa_PrintArg_struct{int tag;short*f1;};struct Cyc_IntPtr_pa_PrintArg_struct{int tag;unsigned long*f1;};
# 73
struct _fat_ptr Cyc_aprintf(struct _fat_ptr,struct _fat_ptr);
# 100
int Cyc_fprintf(struct Cyc___cycFILE*,struct _fat_ptr,struct _fat_ptr);struct Cyc_ShortPtr_sa_ScanfArg_struct{int tag;short*f1;};struct Cyc_UShortPtr_sa_ScanfArg_struct{int tag;unsigned short*f1;};struct Cyc_IntPtr_sa_ScanfArg_struct{int tag;int*f1;};struct Cyc_UIntPtr_sa_ScanfArg_struct{int tag;unsigned*f1;};struct Cyc_StringPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_DoublePtr_sa_ScanfArg_struct{int tag;double*f1;};struct Cyc_FloatPtr_sa_ScanfArg_struct{int tag;float*f1;};struct Cyc_CharPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};
# 157 "cycboot.h"
int Cyc_printf(struct _fat_ptr,struct _fat_ptr);extern char Cyc_FileCloseError[15U];struct Cyc_FileCloseError_exn_struct{char*tag;};extern char Cyc_FileOpenError[14U];struct Cyc_FileOpenError_exn_struct{char*tag;struct _fat_ptr f1;};
# 49 "string.h"
int Cyc_strcmp(struct _fat_ptr s1,struct _fat_ptr s2);
int Cyc_strptrcmp(struct _fat_ptr*s1,struct _fat_ptr*s2);
# 64
struct _fat_ptr Cyc_strconcat_l(struct Cyc_List_List*);struct Cyc_PP_Ppstate;struct Cyc_PP_Out;struct Cyc_PP_Doc;
# 38 "position.h"
struct Cyc_List_List*Cyc_Position_strings_of_segments(struct Cyc_List_List*);struct Cyc_Position_Error;
# 46
extern int Cyc_Position_use_gcc_style_location;struct Cyc_Iter_Iter{void*env;int(*next)(void*env,void*dest);};struct Cyc_Dict_T;struct Cyc_Dict_Dict{int(*rel)(void*,void*);struct _RegionHandle*r;const struct Cyc_Dict_T*t;};extern char Cyc_Dict_Present[8U];struct Cyc_Dict_Present_exn_struct{char*tag;};extern char Cyc_Dict_Absent[7U];struct Cyc_Dict_Absent_exn_struct{char*tag;};
# 62 "dict.h"
struct Cyc_Dict_Dict Cyc_Dict_empty(int(*cmp)(void*,void*));
# 83
int Cyc_Dict_member(struct Cyc_Dict_Dict d,void*k);
# 87
struct Cyc_Dict_Dict Cyc_Dict_insert(struct Cyc_Dict_Dict d,void*k,void*v);
# 110
void*Cyc_Dict_lookup(struct Cyc_Dict_Dict d,void*k);
# 122 "dict.h"
void**Cyc_Dict_lookup_opt(struct Cyc_Dict_Dict d,void*k);struct Cyc_Relations_Reln;struct _union_Nmspace_Rel_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Abs_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_C_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Loc_n{int tag;int val;};union Cyc_Absyn_Nmspace{struct _union_Nmspace_Rel_n Rel_n;struct _union_Nmspace_Abs_n Abs_n;struct _union_Nmspace_C_n C_n;struct _union_Nmspace_Loc_n Loc_n;};
# 110 "absyn.h"
extern union Cyc_Absyn_Nmspace Cyc_Absyn_Loc_n;struct _tuple0{union Cyc_Absyn_Nmspace f1;struct _fat_ptr*f2;};
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
# 981
void*Cyc_Absyn_wildtyp(struct Cyc_Core_Opt*);
# 986
extern void*Cyc_Absyn_sint_type;
# 997
void*Cyc_Absyn_enum_type(struct _tuple0*n,struct Cyc_Absyn_Enumdecl*d);
# 1019
void*Cyc_Absyn_string_type(void*rgn);
# 1022
extern void*Cyc_Absyn_fat_bound_type;
# 1042
void*Cyc_Absyn_fatptr_type(void*t,void*rgn,struct Cyc_Absyn_Tqual,void*zeroterm);
# 1231
struct _tuple0*Cyc_Absyn_binding2qvar(void*);
# 1400
extern const int Cyc_Absyn_noreentrant;struct Cyc_Absynpp_Params{int expand_typedefs;int qvar_to_Cids;int add_cyc_prefix;int to_VC;int decls_first;int rewrite_temp_tvars;int print_all_tvars;int print_all_kinds;int print_all_effects;int print_using_stmts;int print_externC_stmts;int print_full_evars;int print_zeroterm;int generate_line_directives;int use_curr_namespace;struct Cyc_List_List*curr_namespace;};
# 62 "absynpp.h"
struct _fat_ptr Cyc_Absynpp_typ2string(void*);
# 67
struct _fat_ptr Cyc_Absynpp_exp2string(struct Cyc_Absyn_Exp*);
# 69
struct _fat_ptr Cyc_Absynpp_qvar2string(struct _tuple0*);struct _union_RelnOp_RConst{int tag;unsigned val;};struct _union_RelnOp_RVar{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RNumelts{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RType{int tag;void*val;};struct _union_RelnOp_RParam{int tag;unsigned val;};struct _union_RelnOp_RParamNumelts{int tag;unsigned val;};struct _union_RelnOp_RReturn{int tag;unsigned val;};union Cyc_Relations_RelnOp{struct _union_RelnOp_RConst RConst;struct _union_RelnOp_RVar RVar;struct _union_RelnOp_RNumelts RNumelts;struct _union_RelnOp_RType RType;struct _union_RelnOp_RParam RParam;struct _union_RelnOp_RParamNumelts RParamNumelts;struct _union_RelnOp_RReturn RReturn;};
# 50 "relations-ap.h"
enum Cyc_Relations_Relation{Cyc_Relations_Req =0U,Cyc_Relations_Rneq =1U,Cyc_Relations_Rlte =2U,Cyc_Relations_Rlt =3U};struct Cyc_Relations_Reln{union Cyc_Relations_RelnOp rop1;enum Cyc_Relations_Relation relation;union Cyc_Relations_RelnOp rop2;};struct Cyc_RgnOrder_RgnPO;extern char Cyc_Tcenv_Env_error[10U];struct Cyc_Tcenv_Env_error_exn_struct{char*tag;};struct Cyc_Tcenv_Genv{struct Cyc_Dict_Dict aggrdecls;struct Cyc_Dict_Dict datatypedecls;struct Cyc_Dict_Dict enumdecls;struct Cyc_Dict_Dict typedefs;struct Cyc_Dict_Dict ordinaries;};struct Cyc_Tcenv_Fenv;struct Cyc_Tcenv_Tenv{struct Cyc_List_List*ns;struct Cyc_Tcenv_Genv*ae;struct Cyc_Tcenv_Fenv*le;int allow_valueof: 1;int in_extern_c_include: 1;int in_tempest: 1;int tempest_generalize: 1;};
# 90 "tcenv.h"
enum Cyc_Tcenv_NewStatus{Cyc_Tcenv_NoneNew =0U,Cyc_Tcenv_InNew =1U,Cyc_Tcenv_InNewAggr =2U};
# 30 "tcutil.h"
void*Cyc_Tcutil_impos(struct _fat_ptr,struct _fat_ptr);
# 75
struct Cyc_Absyn_Exp*Cyc_Tcutil_get_bounds_exp(void*def,void*b);
# 87
struct Cyc_Absyn_Exp*Cyc_Tcutil_get_bounds_exp(void*def,void*b);
# 110
void*Cyc_Tcutil_compress(void*);
# 310
int Cyc_Tcutil_force_type2bool(int desired,void*);
# 313
void*Cyc_Tcutil_any_bool(struct Cyc_List_List*);extern char Cyc_Tcdecl_Incompatible[13U];struct Cyc_Tcdecl_Incompatible_exn_struct{char*tag;};struct Cyc_Tcdecl_Xdatatypefielddecl{struct Cyc_Absyn_Datatypedecl*base;struct Cyc_Absyn_Datatypefield*field;};struct Cyc_Set_Set;extern char Cyc_Set_Absent[7U];struct Cyc_Set_Absent_exn_struct{char*tag;};struct Cyc_Hashtable_Table;
# 39 "hashtable.h"
struct Cyc_Hashtable_Table*Cyc_Hashtable_create(int sz,int(*cmp)(void*,void*),int(*hash)(void*));
# 50
void Cyc_Hashtable_insert(struct Cyc_Hashtable_Table*t,void*key,void*val);
# 52
void*Cyc_Hashtable_lookup(struct Cyc_Hashtable_Table*t,void*key);
# 59
int Cyc_Hashtable_try_lookup(struct Cyc_Hashtable_Table*t,void*key,void**data);struct Cyc_Port_Edit{unsigned loc;struct _fat_ptr old_string;struct _fat_ptr new_string;};
# 88 "port.cyc"
int Cyc_Port_cmp_edit(struct Cyc_Port_Edit*e1,struct Cyc_Port_Edit*e2){
return(int)e1 - (int)e2;}
# 91
static unsigned Cyc_Port_get_seg(struct Cyc_Port_Edit*e){
return e->loc;}
# 94
int Cyc_Port_cmp_seg_t(unsigned loc1,unsigned loc2){
return(int)(loc1 - loc2);}
# 97
int Cyc_Port_hash_seg_t(unsigned loc){
return(int)loc;}struct Cyc_Port_VarUsage{int address_taken;struct Cyc_Absyn_Exp*init;struct Cyc_List_List*usage_locs;};struct Cyc_Port_Cvar{int id;void**eq;struct Cyc_List_List*uppers;struct Cyc_List_List*lowers;};struct Cyc_Port_Cfield{void*qual;struct _fat_ptr*f;void*type;};struct Cyc_Port_Const_ct_Port_Ctype_struct{int tag;};struct Cyc_Port_Notconst_ct_Port_Ctype_struct{int tag;};struct Cyc_Port_Thin_ct_Port_Ctype_struct{int tag;};struct Cyc_Port_Fat_ct_Port_Ctype_struct{int tag;};struct Cyc_Port_Void_ct_Port_Ctype_struct{int tag;};struct Cyc_Port_Zero_ct_Port_Ctype_struct{int tag;};struct Cyc_Port_Arith_ct_Port_Ctype_struct{int tag;};struct Cyc_Port_Heap_ct_Port_Ctype_struct{int tag;};struct Cyc_Port_Zterm_ct_Port_Ctype_struct{int tag;};struct Cyc_Port_Nozterm_ct_Port_Ctype_struct{int tag;};struct Cyc_Port_RgnVar_ct_Port_Ctype_struct{int tag;struct _fat_ptr*f1;};struct Cyc_Port_Ptr_ct_Port_Ctype_struct{int tag;void*f1;void*f2;void*f3;void*f4;void*f5;};struct Cyc_Port_Array_ct_Port_Ctype_struct{int tag;void*f1;void*f2;void*f3;};struct _tuple12{struct Cyc_Absyn_Aggrdecl*f1;struct Cyc_List_List*f2;};struct Cyc_Port_KnownAggr_ct_Port_Ctype_struct{int tag;struct _tuple12*f1;};struct Cyc_Port_UnknownAggr_ct_Port_Ctype_struct{int tag;struct Cyc_List_List*f1;void**f2;};struct Cyc_Port_Fn_ct_Port_Ctype_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Port_Var_ct_Port_Ctype_struct{int tag;struct Cyc_Port_Cvar*f1;};
# 153
struct Cyc_Port_Const_ct_Port_Ctype_struct Cyc_Port_Const_ct_val={0U};
struct Cyc_Port_Notconst_ct_Port_Ctype_struct Cyc_Port_Notconst_ct_val={1U};
struct Cyc_Port_Thin_ct_Port_Ctype_struct Cyc_Port_Thin_ct_val={2U};
struct Cyc_Port_Fat_ct_Port_Ctype_struct Cyc_Port_Fat_ct_val={3U};
struct Cyc_Port_Void_ct_Port_Ctype_struct Cyc_Port_Void_ct_val={4U};
struct Cyc_Port_Zero_ct_Port_Ctype_struct Cyc_Port_Zero_ct_val={5U};
struct Cyc_Port_Arith_ct_Port_Ctype_struct Cyc_Port_Arith_ct_val={6U};
struct Cyc_Port_Heap_ct_Port_Ctype_struct Cyc_Port_Heap_ct_val={7U};
struct Cyc_Port_Zterm_ct_Port_Ctype_struct Cyc_Port_Zterm_ct_val={8U};
struct Cyc_Port_Nozterm_ct_Port_Ctype_struct Cyc_Port_Nozterm_ct_val={9U};
# 166
static struct _fat_ptr Cyc_Port_ctypes2string(int deep,struct Cyc_List_List*ts);
static struct _fat_ptr Cyc_Port_cfields2string(int deep,struct Cyc_List_List*ts);
static struct _fat_ptr Cyc_Port_ctype2string(int deep,void*t){
void*_tmp4=t;struct Cyc_Port_Cvar*_tmp5;struct Cyc_List_List*_tmp7;void*_tmp6;struct Cyc_List_List*_tmp8;void*_tmpA;struct Cyc_List_List*_tmp9;struct Cyc_List_List*_tmpC;struct Cyc_Absyn_Aggrdecl*_tmpB;void*_tmpF;void*_tmpE;void*_tmpD;void*_tmp14;void*_tmp13;void*_tmp12;void*_tmp11;void*_tmp10;struct _fat_ptr*_tmp15;switch(*((int*)_tmp4)){case 0U: _LL1: _LL2:
 return({const char*_tmp16="const";_tag_fat(_tmp16,sizeof(char),6U);});case 1U: _LL3: _LL4:
 return({const char*_tmp17="notconst";_tag_fat(_tmp17,sizeof(char),9U);});case 2U: _LL5: _LL6:
 return({const char*_tmp18="thin";_tag_fat(_tmp18,sizeof(char),5U);});case 3U: _LL7: _LL8:
 return({const char*_tmp19="fat";_tag_fat(_tmp19,sizeof(char),4U);});case 4U: _LL9: _LLA:
 return({const char*_tmp1A="void";_tag_fat(_tmp1A,sizeof(char),5U);});case 5U: _LLB: _LLC:
 return({const char*_tmp1B="zero";_tag_fat(_tmp1B,sizeof(char),5U);});case 6U: _LLD: _LLE:
 return({const char*_tmp1C="arith";_tag_fat(_tmp1C,sizeof(char),6U);});case 7U: _LLF: _LL10:
 return({const char*_tmp1D="heap";_tag_fat(_tmp1D,sizeof(char),5U);});case 8U: _LL11: _LL12:
 return({const char*_tmp1E="ZT";_tag_fat(_tmp1E,sizeof(char),3U);});case 9U: _LL13: _LL14:
 return({const char*_tmp1F="NZT";_tag_fat(_tmp1F,sizeof(char),4U);});case 10U: _LL15: _tmp15=((struct Cyc_Port_RgnVar_ct_Port_Ctype_struct*)_tmp4)->f1;_LL16: {struct _fat_ptr*n=_tmp15;
return(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp22=({struct Cyc_String_pa_PrintArg_struct _tmp4D4;_tmp4D4.tag=0U,_tmp4D4.f1=(struct _fat_ptr)((struct _fat_ptr)*n);_tmp4D4;});void*_tmp20[1U];_tmp20[0]=& _tmp22;({struct _fat_ptr _tmp504=({const char*_tmp21="%s";_tag_fat(_tmp21,sizeof(char),3U);});Cyc_aprintf(_tmp504,_tag_fat(_tmp20,sizeof(void*),1U));});});}case 11U: _LL17: _tmp10=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmp4)->f1;_tmp11=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmp4)->f2;_tmp12=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmp4)->f3;_tmp13=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmp4)->f4;_tmp14=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmp4)->f5;_LL18: {void*elt=_tmp10;void*qual=_tmp11;void*k=_tmp12;void*rgn=_tmp13;void*zt=_tmp14;
# 182
return(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp25=({struct Cyc_String_pa_PrintArg_struct _tmp4D9;_tmp4D9.tag=0U,({struct _fat_ptr _tmp505=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Port_ctype2string(deep,elt);}));_tmp4D9.f1=_tmp505;});_tmp4D9;});struct Cyc_String_pa_PrintArg_struct _tmp26=({struct Cyc_String_pa_PrintArg_struct _tmp4D8;_tmp4D8.tag=0U,({
struct _fat_ptr _tmp506=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Port_ctype2string(deep,qual);}));_tmp4D8.f1=_tmp506;});_tmp4D8;});struct Cyc_String_pa_PrintArg_struct _tmp27=({struct Cyc_String_pa_PrintArg_struct _tmp4D7;_tmp4D7.tag=0U,({struct _fat_ptr _tmp507=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Port_ctype2string(deep,k);}));_tmp4D7.f1=_tmp507;});_tmp4D7;});struct Cyc_String_pa_PrintArg_struct _tmp28=({struct Cyc_String_pa_PrintArg_struct _tmp4D6;_tmp4D6.tag=0U,({
struct _fat_ptr _tmp508=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Port_ctype2string(deep,rgn);}));_tmp4D6.f1=_tmp508;});_tmp4D6;});struct Cyc_String_pa_PrintArg_struct _tmp29=({struct Cyc_String_pa_PrintArg_struct _tmp4D5;_tmp4D5.tag=0U,({struct _fat_ptr _tmp509=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Port_ctype2string(deep,zt);}));_tmp4D5.f1=_tmp509;});_tmp4D5;});void*_tmp23[5U];_tmp23[0]=& _tmp25,_tmp23[1]=& _tmp26,_tmp23[2]=& _tmp27,_tmp23[3]=& _tmp28,_tmp23[4]=& _tmp29;({struct _fat_ptr _tmp50A=({const char*_tmp24="ptr(%s,%s,%s,%s,%s)";_tag_fat(_tmp24,sizeof(char),20U);});Cyc_aprintf(_tmp50A,_tag_fat(_tmp23,sizeof(void*),5U));});});}case 12U: _LL19: _tmpD=(void*)((struct Cyc_Port_Array_ct_Port_Ctype_struct*)_tmp4)->f1;_tmpE=(void*)((struct Cyc_Port_Array_ct_Port_Ctype_struct*)_tmp4)->f2;_tmpF=(void*)((struct Cyc_Port_Array_ct_Port_Ctype_struct*)_tmp4)->f3;_LL1A: {void*elt=_tmpD;void*qual=_tmpE;void*zt=_tmpF;
# 186
return(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp2C=({struct Cyc_String_pa_PrintArg_struct _tmp4DC;_tmp4DC.tag=0U,({struct _fat_ptr _tmp50B=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Port_ctype2string(deep,elt);}));_tmp4DC.f1=_tmp50B;});_tmp4DC;});struct Cyc_String_pa_PrintArg_struct _tmp2D=({struct Cyc_String_pa_PrintArg_struct _tmp4DB;_tmp4DB.tag=0U,({
struct _fat_ptr _tmp50C=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Port_ctype2string(deep,qual);}));_tmp4DB.f1=_tmp50C;});_tmp4DB;});struct Cyc_String_pa_PrintArg_struct _tmp2E=({struct Cyc_String_pa_PrintArg_struct _tmp4DA;_tmp4DA.tag=0U,({struct _fat_ptr _tmp50D=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Port_ctype2string(deep,zt);}));_tmp4DA.f1=_tmp50D;});_tmp4DA;});void*_tmp2A[3U];_tmp2A[0]=& _tmp2C,_tmp2A[1]=& _tmp2D,_tmp2A[2]=& _tmp2E;({struct _fat_ptr _tmp50E=({const char*_tmp2B="array(%s,%s,%s)";_tag_fat(_tmp2B,sizeof(char),16U);});Cyc_aprintf(_tmp50E,_tag_fat(_tmp2A,sizeof(void*),3U));});});}case 13U: _LL1B: _tmpB=(((struct Cyc_Port_KnownAggr_ct_Port_Ctype_struct*)_tmp4)->f1)->f1;_tmpC=(((struct Cyc_Port_KnownAggr_ct_Port_Ctype_struct*)_tmp4)->f1)->f2;_LL1C: {struct Cyc_Absyn_Aggrdecl*ad=_tmpB;struct Cyc_List_List*cfs=_tmpC;
# 189
struct _fat_ptr s=(int)ad->kind == (int)Cyc_Absyn_StructA?({const char*_tmp38="struct";_tag_fat(_tmp38,sizeof(char),7U);}):({const char*_tmp39="union";_tag_fat(_tmp39,sizeof(char),6U);});
if(!deep)
return(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp31=({struct Cyc_String_pa_PrintArg_struct _tmp4DE;_tmp4DE.tag=0U,_tmp4DE.f1=(struct _fat_ptr)((struct _fat_ptr)s);_tmp4DE;});struct Cyc_String_pa_PrintArg_struct _tmp32=({struct Cyc_String_pa_PrintArg_struct _tmp4DD;_tmp4DD.tag=0U,({struct _fat_ptr _tmp50F=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(ad->name);}));_tmp4DD.f1=_tmp50F;});_tmp4DD;});void*_tmp2F[2U];_tmp2F[0]=& _tmp31,_tmp2F[1]=& _tmp32;({struct _fat_ptr _tmp510=({const char*_tmp30="%s %s";_tag_fat(_tmp30,sizeof(char),6U);});Cyc_aprintf(_tmp510,_tag_fat(_tmp2F,sizeof(void*),2U));});});else{
# 193
return(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp35=({struct Cyc_String_pa_PrintArg_struct _tmp4E1;_tmp4E1.tag=0U,_tmp4E1.f1=(struct _fat_ptr)((struct _fat_ptr)s);_tmp4E1;});struct Cyc_String_pa_PrintArg_struct _tmp36=({struct Cyc_String_pa_PrintArg_struct _tmp4E0;_tmp4E0.tag=0U,({struct _fat_ptr _tmp511=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(ad->name);}));_tmp4E0.f1=_tmp511;});_tmp4E0;});struct Cyc_String_pa_PrintArg_struct _tmp37=({struct Cyc_String_pa_PrintArg_struct _tmp4DF;_tmp4DF.tag=0U,({
struct _fat_ptr _tmp512=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Port_cfields2string(0,cfs);}));_tmp4DF.f1=_tmp512;});_tmp4DF;});void*_tmp33[3U];_tmp33[0]=& _tmp35,_tmp33[1]=& _tmp36,_tmp33[2]=& _tmp37;({struct _fat_ptr _tmp513=({const char*_tmp34="%s %s {%s}";_tag_fat(_tmp34,sizeof(char),11U);});Cyc_aprintf(_tmp513,_tag_fat(_tmp33,sizeof(void*),3U));});});}}case 14U: if(((struct Cyc_Port_UnknownAggr_ct_Port_Ctype_struct*)_tmp4)->f2 != 0){_LL1D: _tmp9=((struct Cyc_Port_UnknownAggr_ct_Port_Ctype_struct*)_tmp4)->f1;_tmpA=*((struct Cyc_Port_UnknownAggr_ct_Port_Ctype_struct*)_tmp4)->f2;_LL1E: {struct Cyc_List_List*cfs=_tmp9;void*eq=_tmpA;
return({Cyc_Port_ctype2string(deep,eq);});}}else{_LL1F: _tmp8=((struct Cyc_Port_UnknownAggr_ct_Port_Ctype_struct*)_tmp4)->f1;_LL20: {struct Cyc_List_List*cfs=_tmp8;
# 197
return(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp3C=({struct Cyc_String_pa_PrintArg_struct _tmp4E2;_tmp4E2.tag=0U,({struct _fat_ptr _tmp514=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Port_cfields2string(deep,cfs);}));_tmp4E2.f1=_tmp514;});_tmp4E2;});void*_tmp3A[1U];_tmp3A[0]=& _tmp3C;({struct _fat_ptr _tmp515=({const char*_tmp3B="aggr {%s}";_tag_fat(_tmp3B,sizeof(char),10U);});Cyc_aprintf(_tmp515,_tag_fat(_tmp3A,sizeof(void*),1U));});});}}case 15U: _LL21: _tmp6=(void*)((struct Cyc_Port_Fn_ct_Port_Ctype_struct*)_tmp4)->f1;_tmp7=((struct Cyc_Port_Fn_ct_Port_Ctype_struct*)_tmp4)->f2;_LL22: {void*t=_tmp6;struct Cyc_List_List*ts=_tmp7;
# 199
return(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp3F=({struct Cyc_String_pa_PrintArg_struct _tmp4E4;_tmp4E4.tag=0U,({struct _fat_ptr _tmp516=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Port_ctypes2string(deep,ts);}));_tmp4E4.f1=_tmp516;});_tmp4E4;});struct Cyc_String_pa_PrintArg_struct _tmp40=({struct Cyc_String_pa_PrintArg_struct _tmp4E3;_tmp4E3.tag=0U,({struct _fat_ptr _tmp517=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Port_ctype2string(deep,t);}));_tmp4E3.f1=_tmp517;});_tmp4E3;});void*_tmp3D[2U];_tmp3D[0]=& _tmp3F,_tmp3D[1]=& _tmp40;({struct _fat_ptr _tmp518=({const char*_tmp3E="fn(%s)->%s";_tag_fat(_tmp3E,sizeof(char),11U);});Cyc_aprintf(_tmp518,_tag_fat(_tmp3D,sizeof(void*),2U));});});}default: _LL23: _tmp5=((struct Cyc_Port_Var_ct_Port_Ctype_struct*)_tmp4)->f1;_LL24: {struct Cyc_Port_Cvar*cv=_tmp5;
# 201
if((unsigned)cv->eq)
return({Cyc_Port_ctype2string(deep,*((void**)_check_null(cv->eq)));});else{
if(!deep || cv->uppers == 0 && cv->lowers == 0)
return(struct _fat_ptr)({struct Cyc_Int_pa_PrintArg_struct _tmp43=({struct Cyc_Int_pa_PrintArg_struct _tmp4E5;_tmp4E5.tag=1U,_tmp4E5.f1=(unsigned long)cv->id;_tmp4E5;});void*_tmp41[1U];_tmp41[0]=& _tmp43;({struct _fat_ptr _tmp519=({const char*_tmp42="var(%d)";_tag_fat(_tmp42,sizeof(char),8U);});Cyc_aprintf(_tmp519,_tag_fat(_tmp41,sizeof(void*),1U));});});else{
return(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp46=({struct Cyc_String_pa_PrintArg_struct _tmp4E8;_tmp4E8.tag=0U,({
struct _fat_ptr _tmp51A=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Port_ctypes2string(0,cv->lowers);}));_tmp4E8.f1=_tmp51A;});_tmp4E8;});struct Cyc_Int_pa_PrintArg_struct _tmp47=({struct Cyc_Int_pa_PrintArg_struct _tmp4E7;_tmp4E7.tag=1U,_tmp4E7.f1=(unsigned long)cv->id;_tmp4E7;});struct Cyc_String_pa_PrintArg_struct _tmp48=({struct Cyc_String_pa_PrintArg_struct _tmp4E6;_tmp4E6.tag=0U,({
# 208
struct _fat_ptr _tmp51B=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Port_ctypes2string(0,cv->uppers);}));_tmp4E6.f1=_tmp51B;});_tmp4E6;});void*_tmp44[3U];_tmp44[0]=& _tmp46,_tmp44[1]=& _tmp47,_tmp44[2]=& _tmp48;({struct _fat_ptr _tmp51C=({const char*_tmp45="var([%s]<=%d<=[%s])";_tag_fat(_tmp45,sizeof(char),20U);});Cyc_aprintf(_tmp51C,_tag_fat(_tmp44,sizeof(void*),3U));});});}}}}_LL0:;}
# 211
static struct _fat_ptr*Cyc_Port_ctype2stringptr(int deep,void*t){return({struct _fat_ptr*_tmp4A=_cycalloc(sizeof(*_tmp4A));({struct _fat_ptr _tmp51D=({Cyc_Port_ctype2string(deep,t);});*_tmp4A=_tmp51D;});_tmp4A;});}
struct Cyc_List_List*Cyc_Port_sep(struct _fat_ptr s,struct Cyc_List_List*xs){
struct _fat_ptr*sptr=({struct _fat_ptr*_tmp4D=_cycalloc(sizeof(*_tmp4D));*_tmp4D=s;_tmp4D;});
if(xs == 0)return xs;{struct Cyc_List_List*prev=xs;
# 216
struct Cyc_List_List*curr=xs->tl;
for(0;curr != 0;(prev=curr,curr=curr->tl)){
({struct Cyc_List_List*_tmp51E=({struct Cyc_List_List*_tmp4C=_cycalloc(sizeof(*_tmp4C));_tmp4C->hd=sptr,_tmp4C->tl=curr;_tmp4C;});prev->tl=_tmp51E;});}
# 220
return xs;}}
# 222
static struct _fat_ptr*Cyc_Port_cfield2stringptr(int deep,struct Cyc_Port_Cfield*f){
struct _fat_ptr s=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp52=({struct Cyc_String_pa_PrintArg_struct _tmp4EB;_tmp4EB.tag=0U,({
struct _fat_ptr _tmp51F=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Port_ctype2string(deep,f->qual);}));_tmp4EB.f1=_tmp51F;});_tmp4EB;});struct Cyc_String_pa_PrintArg_struct _tmp53=({struct Cyc_String_pa_PrintArg_struct _tmp4EA;_tmp4EA.tag=0U,_tmp4EA.f1=(struct _fat_ptr)((struct _fat_ptr)*f->f);_tmp4EA;});struct Cyc_String_pa_PrintArg_struct _tmp54=({struct Cyc_String_pa_PrintArg_struct _tmp4E9;_tmp4E9.tag=0U,({struct _fat_ptr _tmp520=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Port_ctype2string(deep,f->type);}));_tmp4E9.f1=_tmp520;});_tmp4E9;});void*_tmp50[3U];_tmp50[0]=& _tmp52,_tmp50[1]=& _tmp53,_tmp50[2]=& _tmp54;({struct _fat_ptr _tmp521=({const char*_tmp51="%s %s: %s";_tag_fat(_tmp51,sizeof(char),10U);});Cyc_aprintf(_tmp521,_tag_fat(_tmp50,sizeof(void*),3U));});});
return({struct _fat_ptr*_tmp4F=_cycalloc(sizeof(*_tmp4F));*_tmp4F=s;_tmp4F;});}
# 227
static struct _fat_ptr Cyc_Port_ctypes2string(int deep,struct Cyc_List_List*ts){
return(struct _fat_ptr)({struct _fat_ptr(*_tmp56)(struct Cyc_List_List*)=Cyc_strconcat_l;struct Cyc_List_List*_tmp57=({struct Cyc_List_List*(*_tmp58)(struct _fat_ptr s,struct Cyc_List_List*xs)=Cyc_Port_sep;struct _fat_ptr _tmp59=({const char*_tmp5A=",";_tag_fat(_tmp5A,sizeof(char),2U);});struct Cyc_List_List*_tmp5B=({({struct Cyc_List_List*(*_tmp523)(struct _fat_ptr*(*f)(int,void*),int env,struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp5C)(struct _fat_ptr*(*f)(int,void*),int env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct _fat_ptr*(*f)(int,void*),int env,struct Cyc_List_List*x))Cyc_List_map_c;_tmp5C;});int _tmp522=deep;_tmp523(Cyc_Port_ctype2stringptr,_tmp522,ts);});});_tmp58(_tmp59,_tmp5B);});_tmp56(_tmp57);});}
# 230
static struct _fat_ptr Cyc_Port_cfields2string(int deep,struct Cyc_List_List*fs){
return(struct _fat_ptr)({struct _fat_ptr(*_tmp5E)(struct Cyc_List_List*)=Cyc_strconcat_l;struct Cyc_List_List*_tmp5F=({struct Cyc_List_List*(*_tmp60)(struct _fat_ptr s,struct Cyc_List_List*xs)=Cyc_Port_sep;struct _fat_ptr _tmp61=({const char*_tmp62=";";_tag_fat(_tmp62,sizeof(char),2U);});struct Cyc_List_List*_tmp63=({({struct Cyc_List_List*(*_tmp525)(struct _fat_ptr*(*f)(int,struct Cyc_Port_Cfield*),int env,struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp64)(struct _fat_ptr*(*f)(int,struct Cyc_Port_Cfield*),int env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct _fat_ptr*(*f)(int,struct Cyc_Port_Cfield*),int env,struct Cyc_List_List*x))Cyc_List_map_c;_tmp64;});int _tmp524=deep;_tmp525(Cyc_Port_cfield2stringptr,_tmp524,fs);});});_tmp60(_tmp61,_tmp63);});_tmp5E(_tmp5F);});}
# 236
static void*Cyc_Port_notconst_ct(){return(void*)& Cyc_Port_Notconst_ct_val;}
static void*Cyc_Port_const_ct(){return(void*)& Cyc_Port_Const_ct_val;}
static void*Cyc_Port_thin_ct(){return(void*)& Cyc_Port_Thin_ct_val;}
static void*Cyc_Port_fat_ct(){return(void*)& Cyc_Port_Fat_ct_val;}
static void*Cyc_Port_void_ct(){return(void*)& Cyc_Port_Void_ct_val;}
static void*Cyc_Port_zero_ct(){return(void*)& Cyc_Port_Zero_ct_val;}
static void*Cyc_Port_arith_ct(){return(void*)& Cyc_Port_Arith_ct_val;}
static void*Cyc_Port_heap_ct(){return(void*)& Cyc_Port_Heap_ct_val;}
static void*Cyc_Port_zterm_ct(){return(void*)& Cyc_Port_Zterm_ct_val;}
static void*Cyc_Port_nozterm_ct(){return(void*)& Cyc_Port_Nozterm_ct_val;}
static void*Cyc_Port_rgnvar_ct(struct _fat_ptr*n){return(void*)({struct Cyc_Port_RgnVar_ct_Port_Ctype_struct*_tmp70=_cycalloc(sizeof(*_tmp70));_tmp70->tag=10U,_tmp70->f1=n;_tmp70;});}
static void*Cyc_Port_unknown_aggr_ct(struct Cyc_List_List*fs){
return(void*)({struct Cyc_Port_UnknownAggr_ct_Port_Ctype_struct*_tmp72=_cycalloc(sizeof(*_tmp72));_tmp72->tag=14U,_tmp72->f1=fs,_tmp72->f2=0;_tmp72;});}
# 250
static void*Cyc_Port_known_aggr_ct(struct _tuple12*p){
return(void*)({struct Cyc_Port_KnownAggr_ct_Port_Ctype_struct*_tmp74=_cycalloc(sizeof(*_tmp74));_tmp74->tag=13U,_tmp74->f1=p;_tmp74;});}
# 253
static void*Cyc_Port_ptr_ct(void*elt,void*qual,void*ptr_kind,void*r,void*zt){
# 255
return(void*)({struct Cyc_Port_Ptr_ct_Port_Ctype_struct*_tmp76=_cycalloc(sizeof(*_tmp76));_tmp76->tag=11U,_tmp76->f1=elt,_tmp76->f2=qual,_tmp76->f3=ptr_kind,_tmp76->f4=r,_tmp76->f5=zt;_tmp76;});}
# 257
static void*Cyc_Port_array_ct(void*elt,void*qual,void*zt){
return(void*)({struct Cyc_Port_Array_ct_Port_Ctype_struct*_tmp78=_cycalloc(sizeof(*_tmp78));_tmp78->tag=12U,_tmp78->f1=elt,_tmp78->f2=qual,_tmp78->f3=zt;_tmp78;});}
# 260
static void*Cyc_Port_fn_ct(void*return_type,struct Cyc_List_List*args){
return(void*)({struct Cyc_Port_Fn_ct_Port_Ctype_struct*_tmp7A=_cycalloc(sizeof(*_tmp7A));_tmp7A->tag=15U,_tmp7A->f1=return_type,_tmp7A->f2=args;_tmp7A;});}
# 263
static void*Cyc_Port_var(){
static int counter=0;
return(void*)({struct Cyc_Port_Var_ct_Port_Ctype_struct*_tmp7D=_cycalloc(sizeof(*_tmp7D));_tmp7D->tag=16U,({struct Cyc_Port_Cvar*_tmp526=({struct Cyc_Port_Cvar*_tmp7C=_cycalloc(sizeof(*_tmp7C));_tmp7C->id=counter ++,_tmp7C->eq=0,_tmp7C->uppers=0,_tmp7C->lowers=0;_tmp7C;});_tmp7D->f1=_tmp526;});_tmp7D;});}
# 267
static void*Cyc_Port_new_var(void*x){
return({Cyc_Port_var();});}
# 270
static struct _fat_ptr*Cyc_Port_new_region_var(){
static int counter=0;
struct _fat_ptr s=(struct _fat_ptr)({struct Cyc_Int_pa_PrintArg_struct _tmp83=({struct Cyc_Int_pa_PrintArg_struct _tmp4EC;_tmp4EC.tag=1U,_tmp4EC.f1=(unsigned long)counter ++;_tmp4EC;});void*_tmp81[1U];_tmp81[0]=& _tmp83;({struct _fat_ptr _tmp527=({const char*_tmp82="`R%d";_tag_fat(_tmp82,sizeof(char),5U);});Cyc_aprintf(_tmp527,_tag_fat(_tmp81,sizeof(void*),1U));});});
return({struct _fat_ptr*_tmp80=_cycalloc(sizeof(*_tmp80));*_tmp80=s;_tmp80;});}
# 270
static int Cyc_Port_unifies(void*t1,void*t2);
# 280
static void*Cyc_Port_compress_ct(void*t){
void*_tmp85=t;void**_tmp86;struct Cyc_List_List*_tmp89;struct Cyc_List_List*_tmp88;void***_tmp87;switch(*((int*)_tmp85)){case 16U: _LL1: _tmp87=(void***)&(((struct Cyc_Port_Var_ct_Port_Ctype_struct*)_tmp85)->f1)->eq;_tmp88=(((struct Cyc_Port_Var_ct_Port_Ctype_struct*)_tmp85)->f1)->uppers;_tmp89=(((struct Cyc_Port_Var_ct_Port_Ctype_struct*)_tmp85)->f1)->lowers;_LL2: {void***eqp=_tmp87;struct Cyc_List_List*us=_tmp88;struct Cyc_List_List*ls=_tmp89;
# 283
void**eq=*eqp;
if((unsigned)eq){
# 287
void*r=({Cyc_Port_compress_ct(*eq);});
if(*eq != r)({void**_tmp528=({void**_tmp8A=_cycalloc(sizeof(*_tmp8A));*_tmp8A=r;_tmp8A;});*eqp=_tmp528;});return r;}
# 284
for(0;ls != 0;ls=ls->tl){
# 295
void*_stmttmp0=(void*)ls->hd;void*_tmp8B=_stmttmp0;switch(*((int*)_tmp8B)){case 0U: _LL8: _LL9:
 goto _LLB;case 9U: _LLA: _LLB:
 goto _LLD;case 4U: _LLC: _LLD:
 goto _LLF;case 13U: _LLE: _LLF:
 goto _LL11;case 15U: _LL10: _LL11:
# 301
({void**_tmp529=({void**_tmp8C=_cycalloc(sizeof(*_tmp8C));*_tmp8C=(void*)ls->hd;_tmp8C;});*eqp=_tmp529;});
return(void*)ls->hd;default: _LL12: _LL13:
# 304
 goto _LL7;}_LL7:;}
# 307
for(0;us != 0;us=us->tl){
void*_stmttmp1=(void*)us->hd;void*_tmp8D=_stmttmp1;switch(*((int*)_tmp8D)){case 1U: _LL15: _LL16:
 goto _LL18;case 8U: _LL17: _LL18:
 goto _LL1A;case 5U: _LL19: _LL1A:
 goto _LL1C;case 13U: _LL1B: _LL1C:
 goto _LL1E;case 15U: _LL1D: _LL1E:
# 314
({void**_tmp52A=({void**_tmp8E=_cycalloc(sizeof(*_tmp8E));*_tmp8E=(void*)us->hd;_tmp8E;});*eqp=_tmp52A;});
return(void*)us->hd;default: _LL1F: _LL20:
# 317
 goto _LL14;}_LL14:;}
# 320
return t;}case 14U: _LL3: _tmp86=((struct Cyc_Port_UnknownAggr_ct_Port_Ctype_struct*)_tmp85)->f2;_LL4: {void**eq=_tmp86;
# 323
if((unsigned)eq)return({Cyc_Port_compress_ct(*eq);});else{
return t;}}default: _LL5: _LL6:
# 327
 return t;}_LL0:;}struct _tuple13{void*f1;void*f2;};
# 333
static void*Cyc_Port_inst(struct Cyc_Dict_Dict*instenv,void*t){
void*_stmttmp2=({Cyc_Port_compress_ct(t);});void*_tmp90=_stmttmp2;struct Cyc_List_List*_tmp92;void*_tmp91;void*_tmp95;void*_tmp94;void*_tmp93;void*_tmp9A;void*_tmp99;void*_tmp98;void*_tmp97;void*_tmp96;struct _fat_ptr*_tmp9B;switch(*((int*)_tmp90)){case 0U: _LL1: _LL2:
 goto _LL4;case 1U: _LL3: _LL4:
 goto _LL6;case 2U: _LL5: _LL6:
 goto _LL8;case 3U: _LL7: _LL8:
 goto _LLA;case 4U: _LL9: _LLA:
 goto _LLC;case 5U: _LLB: _LLC:
 goto _LLE;case 6U: _LLD: _LLE:
 goto _LL10;case 8U: _LLF: _LL10:
 goto _LL12;case 9U: _LL11: _LL12:
 goto _LL14;case 14U: _LL13: _LL14:
 goto _LL16;case 13U: _LL15: _LL16:
 goto _LL18;case 16U: _LL17: _LL18:
 goto _LL1A;case 7U: _LL19: _LL1A:
 return t;case 10U: _LL1B: _tmp9B=((struct Cyc_Port_RgnVar_ct_Port_Ctype_struct*)_tmp90)->f1;_LL1C: {struct _fat_ptr*n=_tmp9B;
# 349
if(!({({int(*_tmp52C)(struct Cyc_Dict_Dict d,struct _fat_ptr*k)=({int(*_tmp9C)(struct Cyc_Dict_Dict d,struct _fat_ptr*k)=(int(*)(struct Cyc_Dict_Dict d,struct _fat_ptr*k))Cyc_Dict_member;_tmp9C;});struct Cyc_Dict_Dict _tmp52B=*instenv;_tmp52C(_tmp52B,n);});}))
({struct Cyc_Dict_Dict _tmp52D=({struct Cyc_Dict_Dict(*_tmp9D)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,void*v)=({struct Cyc_Dict_Dict(*_tmp9E)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,void*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,void*v))Cyc_Dict_insert;_tmp9E;});struct Cyc_Dict_Dict _tmp9F=*instenv;struct _fat_ptr*_tmpA0=n;void*_tmpA1=({Cyc_Port_var();});_tmp9D(_tmp9F,_tmpA0,_tmpA1);});*instenv=_tmp52D;});
# 349
return({({void*(*_tmp52F)(struct Cyc_Dict_Dict d,struct _fat_ptr*k)=({
# 351
void*(*_tmpA2)(struct Cyc_Dict_Dict d,struct _fat_ptr*k)=(void*(*)(struct Cyc_Dict_Dict d,struct _fat_ptr*k))Cyc_Dict_lookup;_tmpA2;});struct Cyc_Dict_Dict _tmp52E=*instenv;_tmp52F(_tmp52E,n);});});}case 11U: _LL1D: _tmp96=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmp90)->f1;_tmp97=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmp90)->f2;_tmp98=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmp90)->f3;_tmp99=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmp90)->f4;_tmp9A=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmp90)->f5;_LL1E: {void*t1=_tmp96;void*t2=_tmp97;void*t3=_tmp98;void*t4=_tmp99;void*zt=_tmp9A;
# 353
struct _tuple13 _stmttmp3=({struct _tuple13 _tmp4ED;({void*_tmp531=({Cyc_Port_inst(instenv,t1);});_tmp4ED.f1=_tmp531;}),({void*_tmp530=({Cyc_Port_inst(instenv,t4);});_tmp4ED.f2=_tmp530;});_tmp4ED;});void*_tmpA4;void*_tmpA3;_LL24: _tmpA3=_stmttmp3.f1;_tmpA4=_stmttmp3.f2;_LL25: {void*nt1=_tmpA3;void*nt4=_tmpA4;
if(nt1 == t1 && nt4 == t4)return t;return(void*)({struct Cyc_Port_Ptr_ct_Port_Ctype_struct*_tmpA5=_cycalloc(sizeof(*_tmpA5));
_tmpA5->tag=11U,_tmpA5->f1=nt1,_tmpA5->f2=t2,_tmpA5->f3=t3,_tmpA5->f4=nt4,_tmpA5->f5=zt;_tmpA5;});}}case 12U: _LL1F: _tmp93=(void*)((struct Cyc_Port_Array_ct_Port_Ctype_struct*)_tmp90)->f1;_tmp94=(void*)((struct Cyc_Port_Array_ct_Port_Ctype_struct*)_tmp90)->f2;_tmp95=(void*)((struct Cyc_Port_Array_ct_Port_Ctype_struct*)_tmp90)->f3;_LL20: {void*t1=_tmp93;void*t2=_tmp94;void*zt=_tmp95;
# 357
void*nt1=({Cyc_Port_inst(instenv,t1);});
if(nt1 == t1)return t;return(void*)({struct Cyc_Port_Array_ct_Port_Ctype_struct*_tmpA6=_cycalloc(sizeof(*_tmpA6));
_tmpA6->tag=12U,_tmpA6->f1=nt1,_tmpA6->f2=t2,_tmpA6->f3=zt;_tmpA6;});}default: _LL21: _tmp91=(void*)((struct Cyc_Port_Fn_ct_Port_Ctype_struct*)_tmp90)->f1;_tmp92=((struct Cyc_Port_Fn_ct_Port_Ctype_struct*)_tmp90)->f2;_LL22: {void*t1=_tmp91;struct Cyc_List_List*ts=_tmp92;
# 361
return(void*)({struct Cyc_Port_Fn_ct_Port_Ctype_struct*_tmpA8=_cycalloc(sizeof(*_tmpA8));_tmpA8->tag=15U,({void*_tmp535=({Cyc_Port_inst(instenv,t1);});_tmpA8->f1=_tmp535;}),({struct Cyc_List_List*_tmp534=({({struct Cyc_List_List*(*_tmp533)(void*(*f)(struct Cyc_Dict_Dict*,void*),struct Cyc_Dict_Dict*env,struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmpA7)(void*(*f)(struct Cyc_Dict_Dict*,void*),struct Cyc_Dict_Dict*env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(void*(*f)(struct Cyc_Dict_Dict*,void*),struct Cyc_Dict_Dict*env,struct Cyc_List_List*x))Cyc_List_map_c;_tmpA7;});struct Cyc_Dict_Dict*_tmp532=instenv;_tmp533(Cyc_Port_inst,_tmp532,ts);});});_tmpA8->f2=_tmp534;});_tmpA8;});}}_LL0:;}
# 333
void*Cyc_Port_instantiate(void*t){
# 366
return({void*(*_tmpAA)(struct Cyc_Dict_Dict*instenv,void*t)=Cyc_Port_inst;struct Cyc_Dict_Dict*_tmpAB=({struct Cyc_Dict_Dict*_tmpAD=_cycalloc(sizeof(*_tmpAD));({struct Cyc_Dict_Dict _tmp536=({({struct Cyc_Dict_Dict(*_tmpAC)(int(*cmp)(struct _fat_ptr*,struct _fat_ptr*))=(struct Cyc_Dict_Dict(*)(int(*cmp)(struct _fat_ptr*,struct _fat_ptr*)))Cyc_Dict_empty;_tmpAC;})(Cyc_strptrcmp);});*_tmpAD=_tmp536;});_tmpAD;});void*_tmpAE=t;_tmpAA(_tmpAB,_tmpAE);});}
# 372
struct Cyc_List_List*Cyc_Port_filter_self(void*t,struct Cyc_List_List*ts){
int found=0;
{struct Cyc_List_List*xs=ts;for(0;(unsigned)xs;xs=xs->tl){
void*t2=({Cyc_Port_compress_ct((void*)xs->hd);});
if(t == t2)found=1;}}
# 378
if(!found)return ts;{struct Cyc_List_List*res=0;
# 380
for(0;ts != 0;ts=ts->tl){
if(({void*_tmp537=t;_tmp537 == ({Cyc_Port_compress_ct((void*)ts->hd);});}))continue;res=({struct Cyc_List_List*_tmpB0=_cycalloc(sizeof(*_tmpB0));
_tmpB0->hd=(void*)ts->hd,_tmpB0->tl=res;_tmpB0;});}
# 384
return res;}}
# 389
void Cyc_Port_generalize(int is_rgn,void*t){
t=({Cyc_Port_compress_ct(t);});{
void*_tmpB2=t;struct Cyc_List_List*_tmpB4;void*_tmpB3;void*_tmpB7;void*_tmpB6;void*_tmpB5;void*_tmpBC;void*_tmpBB;void*_tmpBA;void*_tmpB9;void*_tmpB8;struct Cyc_Port_Cvar*_tmpBD;switch(*((int*)_tmpB2)){case 16U: _LL1: _tmpBD=((struct Cyc_Port_Var_ct_Port_Ctype_struct*)_tmpB2)->f1;_LL2: {struct Cyc_Port_Cvar*cv=_tmpBD;
# 394
({struct Cyc_List_List*_tmp538=({Cyc_Port_filter_self(t,cv->uppers);});cv->uppers=_tmp538;});
({struct Cyc_List_List*_tmp539=({Cyc_Port_filter_self(t,cv->lowers);});cv->lowers=_tmp539;});
if(is_rgn){
# 399
if(cv->uppers == 0 && cv->lowers == 0){
({int(*_tmpBE)(void*t1,void*t2)=Cyc_Port_unifies;void*_tmpBF=t;void*_tmpC0=({void*(*_tmpC1)(struct _fat_ptr*n)=Cyc_Port_rgnvar_ct;struct _fat_ptr*_tmpC2=({Cyc_Port_new_region_var();});_tmpC1(_tmpC2);});_tmpBE(_tmpBF,_tmpC0);});
return;}
# 399
if((unsigned)cv->uppers){
# 405
({Cyc_Port_unifies(t,(void*)((struct Cyc_List_List*)_check_null(cv->uppers))->hd);});
({Cyc_Port_generalize(1,t);});}else{
# 408
({Cyc_Port_unifies(t,(void*)((struct Cyc_List_List*)_check_null(cv->lowers))->hd);});
({Cyc_Port_generalize(1,t);});}}
# 396
return;}case 0U: _LL3: _LL4:
# 413
 goto _LL6;case 1U: _LL5: _LL6:
 goto _LL8;case 2U: _LL7: _LL8:
 goto _LLA;case 3U: _LL9: _LLA:
 goto _LLC;case 4U: _LLB: _LLC:
 goto _LLE;case 5U: _LLD: _LLE:
 goto _LL10;case 6U: _LLF: _LL10:
 goto _LL12;case 14U: _LL11: _LL12:
 goto _LL14;case 13U: _LL13: _LL14:
 goto _LL16;case 10U: _LL15: _LL16:
 goto _LL18;case 9U: _LL17: _LL18:
 goto _LL1A;case 8U: _LL19: _LL1A:
 goto _LL1C;case 7U: _LL1B: _LL1C:
 return;case 11U: _LL1D: _tmpB8=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmpB2)->f1;_tmpB9=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmpB2)->f2;_tmpBA=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmpB2)->f3;_tmpBB=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmpB2)->f4;_tmpBC=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmpB2)->f5;_LL1E: {void*t1=_tmpB8;void*t2=_tmpB9;void*t3=_tmpBA;void*t4=_tmpBB;void*t5=_tmpBC;
# 429
({Cyc_Port_generalize(0,t1);});({Cyc_Port_generalize(1,t4);});goto _LL0;}case 12U: _LL1F: _tmpB5=(void*)((struct Cyc_Port_Array_ct_Port_Ctype_struct*)_tmpB2)->f1;_tmpB6=(void*)((struct Cyc_Port_Array_ct_Port_Ctype_struct*)_tmpB2)->f2;_tmpB7=(void*)((struct Cyc_Port_Array_ct_Port_Ctype_struct*)_tmpB2)->f3;_LL20: {void*t1=_tmpB5;void*t2=_tmpB6;void*t3=_tmpB7;
# 431
({Cyc_Port_generalize(0,t1);});({Cyc_Port_generalize(0,t2);});goto _LL0;}default: _LL21: _tmpB3=(void*)((struct Cyc_Port_Fn_ct_Port_Ctype_struct*)_tmpB2)->f1;_tmpB4=((struct Cyc_Port_Fn_ct_Port_Ctype_struct*)_tmpB2)->f2;_LL22: {void*t1=_tmpB3;struct Cyc_List_List*ts=_tmpB4;
# 433
({Cyc_Port_generalize(0,t1);});({({void(*_tmp53A)(void(*f)(int,void*),int env,struct Cyc_List_List*x)=({void(*_tmpC3)(void(*f)(int,void*),int env,struct Cyc_List_List*x)=(void(*)(void(*f)(int,void*),int env,struct Cyc_List_List*x))Cyc_List_iter_c;_tmpC3;});_tmp53A(Cyc_Port_generalize,0,ts);});});goto _LL0;}}_LL0:;}}
# 439
static int Cyc_Port_occurs(void*v,void*t){
t=({Cyc_Port_compress_ct(t);});
if(t == v)return 1;{void*_tmpC5=t;struct Cyc_List_List*_tmpC6;struct Cyc_List_List*_tmpC7;struct Cyc_List_List*_tmpC9;void*_tmpC8;void*_tmpCC;void*_tmpCB;void*_tmpCA;void*_tmpD1;void*_tmpD0;void*_tmpCF;void*_tmpCE;void*_tmpCD;switch(*((int*)_tmpC5)){case 11U: _LL1: _tmpCD=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmpC5)->f1;_tmpCE=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmpC5)->f2;_tmpCF=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmpC5)->f3;_tmpD0=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmpC5)->f4;_tmpD1=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmpC5)->f5;_LL2: {void*t1=_tmpCD;void*t2=_tmpCE;void*t3=_tmpCF;void*t4=_tmpD0;void*t5=_tmpD1;
# 444
return(((({Cyc_Port_occurs(v,t1);})||({Cyc_Port_occurs(v,t2);}))||({Cyc_Port_occurs(v,t3);}))||({Cyc_Port_occurs(v,t4);}))||({Cyc_Port_occurs(v,t5);});}case 12U: _LL3: _tmpCA=(void*)((struct Cyc_Port_Array_ct_Port_Ctype_struct*)_tmpC5)->f1;_tmpCB=(void*)((struct Cyc_Port_Array_ct_Port_Ctype_struct*)_tmpC5)->f2;_tmpCC=(void*)((struct Cyc_Port_Array_ct_Port_Ctype_struct*)_tmpC5)->f3;_LL4: {void*t1=_tmpCA;void*t2=_tmpCB;void*t3=_tmpCC;
# 447
return(({Cyc_Port_occurs(v,t1);})||({Cyc_Port_occurs(v,t2);}))||({Cyc_Port_occurs(v,t3);});}case 15U: _LL5: _tmpC8=(void*)((struct Cyc_Port_Fn_ct_Port_Ctype_struct*)_tmpC5)->f1;_tmpC9=((struct Cyc_Port_Fn_ct_Port_Ctype_struct*)_tmpC5)->f2;_LL6: {void*t=_tmpC8;struct Cyc_List_List*ts=_tmpC9;
# 449
if(({Cyc_Port_occurs(v,t);}))return 1;for(0;(unsigned)ts;ts=ts->tl){
# 451
if(({Cyc_Port_occurs(v,(void*)ts->hd);}))return 1;}
# 449
return 0;}case 13U: _LL7: _tmpC7=(((struct Cyc_Port_KnownAggr_ct_Port_Ctype_struct*)_tmpC5)->f1)->f2;_LL8: {struct Cyc_List_List*fs=_tmpC7;
# 453
return 0;}case 14U: _LL9: _tmpC6=((struct Cyc_Port_UnknownAggr_ct_Port_Ctype_struct*)_tmpC5)->f1;_LLA: {struct Cyc_List_List*fs=_tmpC6;
# 455
for(0;(unsigned)fs;fs=fs->tl){
if(({Cyc_Port_occurs(v,((struct Cyc_Port_Cfield*)fs->hd)->qual);})||({Cyc_Port_occurs(v,((struct Cyc_Port_Cfield*)fs->hd)->type);}))return 1;}
# 455
return 0;}default: _LLB: _LLC:
# 458
 return 0;}_LL0:;}}char Cyc_Port_Unify_ct[9U]="Unify_ct";struct Cyc_Port_Unify_ct_exn_struct{char*tag;};
# 467
struct Cyc_Port_Unify_ct_exn_struct Cyc_Port_Unify_ct_val={Cyc_Port_Unify_ct};
# 469
static int Cyc_Port_leq(void*t1,void*t2);
static void Cyc_Port_unify_cts(struct Cyc_List_List*t1,struct Cyc_List_List*t2);
static struct Cyc_List_List*Cyc_Port_merge_fields(struct Cyc_List_List*fs1,struct Cyc_List_List*fs2,int allow_subset);
# 473
static void Cyc_Port_unify_ct(void*t1,void*t2){
t1=({Cyc_Port_compress_ct(t1);});
t2=({Cyc_Port_compress_ct(t2);});
if(t1 == t2)return;{struct _tuple13 _stmttmp4=({struct _tuple13 _tmp4EE;_tmp4EE.f1=t1,_tmp4EE.f2=t2;_tmp4EE;});struct _tuple13 _tmpD3=_stmttmp4;struct Cyc_List_List*_tmpD7;struct Cyc_Absyn_Aggrdecl*_tmpD6;void***_tmpD5;struct Cyc_List_List*_tmpD4;void***_tmpDB;struct Cyc_List_List*_tmpDA;void***_tmpD9;struct Cyc_List_List*_tmpD8;void***_tmpDF;struct Cyc_List_List*_tmpDE;struct Cyc_List_List*_tmpDD;struct Cyc_Absyn_Aggrdecl*_tmpDC;struct _tuple12*_tmpE1;struct _tuple12*_tmpE0;struct Cyc_List_List*_tmpE5;void*_tmpE4;struct Cyc_List_List*_tmpE3;void*_tmpE2;void*_tmpEB;void*_tmpEA;void*_tmpE9;void*_tmpE8;void*_tmpE7;void*_tmpE6;struct _fat_ptr _tmpED;struct _fat_ptr _tmpEC;void*_tmpF7;void*_tmpF6;void*_tmpF5;void*_tmpF4;void*_tmpF3;void*_tmpF2;void*_tmpF1;void*_tmpF0;void*_tmpEF;void*_tmpEE;struct Cyc_Port_Cvar*_tmpF8;struct Cyc_Port_Cvar*_tmpF9;if(((struct Cyc_Port_Var_ct_Port_Ctype_struct*)_tmpD3.f1)->tag == 16U){_LL1: _tmpF9=((struct Cyc_Port_Var_ct_Port_Ctype_struct*)_tmpD3.f1)->f1;_LL2: {struct Cyc_Port_Cvar*v1=_tmpF9;
# 479
if(!({Cyc_Port_occurs(t1,t2);})){
# 482
{struct Cyc_List_List*us=({Cyc_Port_filter_self(t1,v1->uppers);});for(0;(unsigned)us;us=us->tl){
if(!({Cyc_Port_leq(t2,(void*)us->hd);}))(int)_throw(& Cyc_Port_Unify_ct_val);}}
# 482
{
# 484
struct Cyc_List_List*ls=({Cyc_Port_filter_self(t1,v1->lowers);});
# 482
for(0;(unsigned)ls;ls=ls->tl){
# 485
if(!({Cyc_Port_leq((void*)ls->hd,t2);}))(int)_throw(& Cyc_Port_Unify_ct_val);}}
# 482
({void**_tmp53B=({void**_tmpFA=_cycalloc(sizeof(*_tmpFA));*_tmpFA=t2;_tmpFA;});v1->eq=_tmp53B;});
# 487
return;}else{
(int)_throw(& Cyc_Port_Unify_ct_val);}}}else{if(((struct Cyc_Port_Var_ct_Port_Ctype_struct*)_tmpD3.f2)->tag == 16U){_LL3: _tmpF8=((struct Cyc_Port_Var_ct_Port_Ctype_struct*)_tmpD3.f2)->f1;_LL4: {struct Cyc_Port_Cvar*v1=_tmpF8;
({Cyc_Port_unify_ct(t2,t1);});return;}}else{switch(*((int*)_tmpD3.f1)){case 11U: if(((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmpD3.f2)->tag == 11U){_LL5: _tmpEE=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmpD3.f1)->f1;_tmpEF=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmpD3.f1)->f2;_tmpF0=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmpD3.f1)->f3;_tmpF1=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmpD3.f1)->f4;_tmpF2=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmpD3.f1)->f5;_tmpF3=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmpD3.f2)->f1;_tmpF4=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmpD3.f2)->f2;_tmpF5=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmpD3.f2)->f3;_tmpF6=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmpD3.f2)->f4;_tmpF7=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmpD3.f2)->f5;_LL6: {void*e1=_tmpEE;void*q1=_tmpEF;void*k1=_tmpF0;void*r1=_tmpF1;void*zt1=_tmpF2;void*e2=_tmpF3;void*q2=_tmpF4;void*k2=_tmpF5;void*r2=_tmpF6;void*zt2=_tmpF7;
# 491
({Cyc_Port_unify_ct(e1,e2);});({Cyc_Port_unify_ct(q1,q2);});({Cyc_Port_unify_ct(k1,k2);});({Cyc_Port_unify_ct(r1,r2);});
({Cyc_Port_unify_ct(zt1,zt2);});
return;}}else{goto _LL15;}case 10U: if(((struct Cyc_Port_RgnVar_ct_Port_Ctype_struct*)_tmpD3.f2)->tag == 10U){_LL7: _tmpEC=*((struct Cyc_Port_RgnVar_ct_Port_Ctype_struct*)_tmpD3.f1)->f1;_tmpED=*((struct Cyc_Port_RgnVar_ct_Port_Ctype_struct*)_tmpD3.f2)->f1;_LL8: {struct _fat_ptr n1=_tmpEC;struct _fat_ptr n2=_tmpED;
# 495
if(({Cyc_strcmp((struct _fat_ptr)n1,(struct _fat_ptr)n2);})!= 0)(int)_throw(& Cyc_Port_Unify_ct_val);return;}}else{goto _LL15;}case 12U: if(((struct Cyc_Port_Array_ct_Port_Ctype_struct*)_tmpD3.f2)->tag == 12U){_LL9: _tmpE6=(void*)((struct Cyc_Port_Array_ct_Port_Ctype_struct*)_tmpD3.f1)->f1;_tmpE7=(void*)((struct Cyc_Port_Array_ct_Port_Ctype_struct*)_tmpD3.f1)->f2;_tmpE8=(void*)((struct Cyc_Port_Array_ct_Port_Ctype_struct*)_tmpD3.f1)->f3;_tmpE9=(void*)((struct Cyc_Port_Array_ct_Port_Ctype_struct*)_tmpD3.f2)->f1;_tmpEA=(void*)((struct Cyc_Port_Array_ct_Port_Ctype_struct*)_tmpD3.f2)->f2;_tmpEB=(void*)((struct Cyc_Port_Array_ct_Port_Ctype_struct*)_tmpD3.f2)->f3;_LLA: {void*e1=_tmpE6;void*q1=_tmpE7;void*zt1=_tmpE8;void*e2=_tmpE9;void*q2=_tmpEA;void*zt2=_tmpEB;
# 498
({Cyc_Port_unify_ct(e1,e2);});({Cyc_Port_unify_ct(q1,q2);});({Cyc_Port_unify_ct(zt1,zt2);});return;}}else{goto _LL15;}case 15U: if(((struct Cyc_Port_Fn_ct_Port_Ctype_struct*)_tmpD3.f2)->tag == 15U){_LLB: _tmpE2=(void*)((struct Cyc_Port_Fn_ct_Port_Ctype_struct*)_tmpD3.f1)->f1;_tmpE3=((struct Cyc_Port_Fn_ct_Port_Ctype_struct*)_tmpD3.f1)->f2;_tmpE4=(void*)((struct Cyc_Port_Fn_ct_Port_Ctype_struct*)_tmpD3.f2)->f1;_tmpE5=((struct Cyc_Port_Fn_ct_Port_Ctype_struct*)_tmpD3.f2)->f2;_LLC: {void*t1=_tmpE2;struct Cyc_List_List*ts1=_tmpE3;void*t2=_tmpE4;struct Cyc_List_List*ts2=_tmpE5;
# 500
({Cyc_Port_unify_ct(t1,t2);});({Cyc_Port_unify_cts(ts1,ts2);});return;}}else{goto _LL15;}case 13U: switch(*((int*)_tmpD3.f2)){case 13U: _LLD: _tmpE0=((struct Cyc_Port_KnownAggr_ct_Port_Ctype_struct*)_tmpD3.f1)->f1;_tmpE1=((struct Cyc_Port_KnownAggr_ct_Port_Ctype_struct*)_tmpD3.f2)->f1;_LLE: {struct _tuple12*p1=_tmpE0;struct _tuple12*p2=_tmpE1;
# 502
if(p1 == p2)return;else{(int)_throw(& Cyc_Port_Unify_ct_val);}}case 14U: _LL13: _tmpDC=(((struct Cyc_Port_KnownAggr_ct_Port_Ctype_struct*)_tmpD3.f1)->f1)->f1;_tmpDD=(((struct Cyc_Port_KnownAggr_ct_Port_Ctype_struct*)_tmpD3.f1)->f1)->f2;_tmpDE=((struct Cyc_Port_UnknownAggr_ct_Port_Ctype_struct*)_tmpD3.f2)->f1;_tmpDF=(void***)&((struct Cyc_Port_UnknownAggr_ct_Port_Ctype_struct*)_tmpD3.f2)->f2;_LL14: {struct Cyc_Absyn_Aggrdecl*ad=_tmpDC;struct Cyc_List_List*fs2=_tmpDD;struct Cyc_List_List*fs1=_tmpDE;void***eq=_tmpDF;
# 512
({Cyc_Port_merge_fields(fs2,fs1,0);});
({void**_tmp53C=({void**_tmpFF=_cycalloc(sizeof(*_tmpFF));*_tmpFF=t1;_tmpFF;});*eq=_tmp53C;});
return;}default: goto _LL15;}case 14U: switch(*((int*)_tmpD3.f2)){case 14U: _LLF: _tmpD8=((struct Cyc_Port_UnknownAggr_ct_Port_Ctype_struct*)_tmpD3.f1)->f1;_tmpD9=(void***)&((struct Cyc_Port_UnknownAggr_ct_Port_Ctype_struct*)_tmpD3.f1)->f2;_tmpDA=((struct Cyc_Port_UnknownAggr_ct_Port_Ctype_struct*)_tmpD3.f2)->f1;_tmpDB=(void***)&((struct Cyc_Port_UnknownAggr_ct_Port_Ctype_struct*)_tmpD3.f2)->f2;_LL10: {struct Cyc_List_List*fs1=_tmpD8;void***eq1=_tmpD9;struct Cyc_List_List*fs2=_tmpDA;void***eq2=_tmpDB;
# 504
void*t=({void*(*_tmpFC)(struct Cyc_List_List*fs)=Cyc_Port_unknown_aggr_ct;struct Cyc_List_List*_tmpFD=({Cyc_Port_merge_fields(fs1,fs2,1);});_tmpFC(_tmpFD);});
({void**_tmp53E=({void**_tmp53D=({void**_tmpFB=_cycalloc(sizeof(*_tmpFB));*_tmpFB=t;_tmpFB;});*eq2=_tmp53D;});*eq1=_tmp53E;});
return;}case 13U: _LL11: _tmpD4=((struct Cyc_Port_UnknownAggr_ct_Port_Ctype_struct*)_tmpD3.f1)->f1;_tmpD5=(void***)&((struct Cyc_Port_UnknownAggr_ct_Port_Ctype_struct*)_tmpD3.f1)->f2;_tmpD6=(((struct Cyc_Port_KnownAggr_ct_Port_Ctype_struct*)_tmpD3.f2)->f1)->f1;_tmpD7=(((struct Cyc_Port_KnownAggr_ct_Port_Ctype_struct*)_tmpD3.f2)->f1)->f2;_LL12: {struct Cyc_List_List*fs1=_tmpD4;void***eq=_tmpD5;struct Cyc_Absyn_Aggrdecl*ad=_tmpD6;struct Cyc_List_List*fs2=_tmpD7;
# 508
({Cyc_Port_merge_fields(fs2,fs1,0);});
({void**_tmp53F=({void**_tmpFE=_cycalloc(sizeof(*_tmpFE));*_tmpFE=t2;_tmpFE;});*eq=_tmp53F;});
return;}default: goto _LL15;}default: _LL15: _LL16:
# 515
(int)_throw(& Cyc_Port_Unify_ct_val);}}}_LL0:;}}
# 519
static void Cyc_Port_unify_cts(struct Cyc_List_List*t1,struct Cyc_List_List*t2){
for(0;t1 != 0 && t2 != 0;(t1=t1->tl,t2=t2->tl)){
({Cyc_Port_unify_ct((void*)t1->hd,(void*)t2->hd);});}
# 523
if(t1 != 0 || t2 != 0)
(int)_throw(& Cyc_Port_Unify_ct_val);}
# 529
static struct Cyc_List_List*Cyc_Port_merge_fields(struct Cyc_List_List*fs1,struct Cyc_List_List*fs2,int allow_f1_subset_f2){
# 531
struct Cyc_List_List*common=0;
{struct Cyc_List_List*xs2=fs2;for(0;(unsigned)xs2;xs2=xs2->tl){
struct Cyc_Port_Cfield*f2=(struct Cyc_Port_Cfield*)xs2->hd;
int found=0;
{struct Cyc_List_List*xs1=fs1;for(0;(unsigned)xs1;xs1=xs1->tl){
struct Cyc_Port_Cfield*f1=(struct Cyc_Port_Cfield*)xs1->hd;
if(({Cyc_strptrcmp(f1->f,f2->f);})== 0){
common=({struct Cyc_List_List*_tmp102=_cycalloc(sizeof(*_tmp102));_tmp102->hd=f1,_tmp102->tl=common;_tmp102;});
({Cyc_Port_unify_ct(f1->qual,f2->qual);});
({Cyc_Port_unify_ct(f1->type,f2->type);});
found=1;
break;}}}
# 545
if(!found){
if(allow_f1_subset_f2)
common=({struct Cyc_List_List*_tmp103=_cycalloc(sizeof(*_tmp103));_tmp103->hd=f2,_tmp103->tl=common;_tmp103;});else{
(int)_throw(& Cyc_Port_Unify_ct_val);}}}}
# 551
{struct Cyc_List_List*xs1=fs1;for(0;(unsigned)xs1;xs1=xs1->tl){
struct Cyc_Port_Cfield*f1=(struct Cyc_Port_Cfield*)xs1->hd;
int found=0;
{struct Cyc_List_List*xs2=fs2;for(0;(unsigned)xs2;xs2=xs2->tl){
struct Cyc_Port_Cfield*f2=(struct Cyc_Port_Cfield*)xs2->hd;
if(({Cyc_strptrcmp(f1->f,f2->f);}))found=1;}}
# 558
if(!found)
common=({struct Cyc_List_List*_tmp104=_cycalloc(sizeof(*_tmp104));_tmp104->hd=f1,_tmp104->tl=common;_tmp104;});}}
# 561
return common;}
# 564
static int Cyc_Port_unifies(void*t1,void*t2){
{struct _handler_cons _tmp106;_push_handler(& _tmp106);{int _tmp108=0;if(setjmp(_tmp106.handler))_tmp108=1;if(!_tmp108){
# 571
({Cyc_Port_unify_ct(t1,t2);});;_pop_handler();}else{void*_tmp107=(void*)Cyc_Core_get_exn_thrown();void*_tmp109=_tmp107;void*_tmp10A;if(((struct Cyc_Port_Unify_ct_exn_struct*)_tmp109)->tag == Cyc_Port_Unify_ct){_LL1: _LL2:
# 578
 return 0;}else{_LL3: _tmp10A=_tmp109;_LL4: {void*exn=_tmp10A;(int)_rethrow(exn);}}_LL0:;}}}
# 580
return 1;}struct _tuple14{void*f1;void*f2;void*f3;void*f4;void*f5;};
# 586
static struct Cyc_List_List*Cyc_Port_insert_upper(void*v,void*t,struct Cyc_List_List**uppers){
# 588
t=({Cyc_Port_compress_ct(t);});
{void*_tmp10C=t;switch(*((int*)_tmp10C)){case 1U: _LL1: _LL2:
# 592
 goto _LL4;case 8U: _LL3: _LL4:
 goto _LL6;case 5U: _LL5: _LL6:
 goto _LL8;case 2U: _LL7: _LL8:
 goto _LLA;case 3U: _LL9: _LLA:
 goto _LLC;case 12U: _LLB: _LLC:
 goto _LLE;case 13U: _LLD: _LLE:
 goto _LL10;case 15U: _LLF: _LL10:
 goto _LL12;case 7U: _LL11: _LL12:
# 603
*uppers=0;
({Cyc_Port_unifies(v,t);});
return*uppers;case 4U: _LL13: _LL14:
# 608
 goto _LL16;case 0U: _LL15: _LL16:
 goto _LL18;case 9U: _LL17: _LL18:
# 611
 return*uppers;default: _LL19: _LL1A:
 goto _LL0;}_LL0:;}
# 615
{struct Cyc_List_List*us=*uppers;for(0;(unsigned)us;us=us->tl){
void*t2=({Cyc_Port_compress_ct((void*)us->hd);});
# 618
if(t == t2)return*uppers;{struct _tuple13 _stmttmp5=({struct _tuple13 _tmp4F0;_tmp4F0.f1=t,_tmp4F0.f2=t2;_tmp4F0;});struct _tuple13 _tmp10D=_stmttmp5;void*_tmp117;void*_tmp116;void*_tmp115;void*_tmp114;void*_tmp113;void*_tmp112;void*_tmp111;void*_tmp110;void*_tmp10F;void*_tmp10E;switch(*((int*)_tmp10D.f1)){case 6U: switch(*((int*)_tmp10D.f2)){case 11U: _LL1C: _LL1D:
# 623
 goto _LL1F;case 5U: _LL1E: _LL1F:
 goto _LL21;case 12U: _LL20: _LL21:
# 626
 return*uppers;default: goto _LL24;}case 11U: if(((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmp10D.f2)->tag == 11U){_LL22: _tmp10E=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmp10D.f1)->f1;_tmp10F=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmp10D.f1)->f2;_tmp110=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmp10D.f1)->f3;_tmp111=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmp10D.f1)->f4;_tmp112=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmp10D.f1)->f5;_tmp113=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmp10D.f2)->f1;_tmp114=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmp10D.f2)->f2;_tmp115=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmp10D.f2)->f3;_tmp116=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmp10D.f2)->f4;_tmp117=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmp10D.f2)->f5;_LL23: {void*ta=_tmp10E;void*qa=_tmp10F;void*ka=_tmp110;void*ra=_tmp111;void*za=_tmp112;void*tb=_tmp113;void*qb=_tmp114;void*kb=_tmp115;void*rb=_tmp116;void*zb=_tmp117;
# 631
struct _tuple14 _stmttmp6=({struct _tuple14 _tmp4EF;({void*_tmp544=({Cyc_Port_var();});_tmp4EF.f1=_tmp544;}),({void*_tmp543=({Cyc_Port_var();});_tmp4EF.f2=_tmp543;}),({void*_tmp542=({Cyc_Port_var();});_tmp4EF.f3=_tmp542;}),({void*_tmp541=({Cyc_Port_var();});_tmp4EF.f4=_tmp541;}),({void*_tmp540=({Cyc_Port_var();});_tmp4EF.f5=_tmp540;});_tmp4EF;});void*_tmp11C;void*_tmp11B;void*_tmp11A;void*_tmp119;void*_tmp118;_LL27: _tmp118=_stmttmp6.f1;_tmp119=_stmttmp6.f2;_tmp11A=_stmttmp6.f3;_tmp11B=_stmttmp6.f4;_tmp11C=_stmttmp6.f5;_LL28: {void*tc=_tmp118;void*qc=_tmp119;void*kc=_tmp11A;void*rc=_tmp11B;void*zc=_tmp11C;
struct Cyc_Port_Ptr_ct_Port_Ctype_struct*p=({struct Cyc_Port_Ptr_ct_Port_Ctype_struct*_tmp11D=_cycalloc(sizeof(*_tmp11D));_tmp11D->tag=11U,_tmp11D->f1=tc,_tmp11D->f2=qc,_tmp11D->f3=kc,_tmp11D->f4=rc,_tmp11D->f5=zc;_tmp11D;});
({Cyc_Port_leq(tc,ta);});({Cyc_Port_leq(tc,tb);});
({Cyc_Port_leq(qc,qa);});({Cyc_Port_leq(qc,qb);});
({Cyc_Port_leq(kc,ka);});({Cyc_Port_leq(kc,qb);});
({Cyc_Port_leq(rc,ra);});({Cyc_Port_leq(rc,rb);});
({Cyc_Port_leq(zc,za);});({Cyc_Port_leq(zc,zb);});
us->hd=(void*)((void*)p);
return*uppers;}}}else{goto _LL24;}default: _LL24: _LL25:
 goto _LL1B;}_LL1B:;}}}
# 643
return({struct Cyc_List_List*_tmp11E=_cycalloc(sizeof(*_tmp11E));_tmp11E->hd=t,_tmp11E->tl=*uppers;_tmp11E;});}
# 648
static struct Cyc_List_List*Cyc_Port_insert_lower(void*v,void*t,struct Cyc_List_List**lowers){
# 650
t=({Cyc_Port_compress_ct(t);});
{void*_tmp120=t;switch(*((int*)_tmp120)){case 0U: _LL1: _LL2:
 goto _LL4;case 8U: _LL3: _LL4:
 goto _LL6;case 2U: _LL5: _LL6:
 goto _LL8;case 3U: _LL7: _LL8:
 goto _LLA;case 4U: _LL9: _LLA:
 goto _LLC;case 13U: _LLB: _LLC:
 goto _LLE;case 15U: _LLD: _LLE:
 goto _LL10;case 10U: _LLF: _LL10:
# 660
*lowers=0;
({Cyc_Port_unifies(v,t);});
return*lowers;case 7U: _LL11: _LL12:
 goto _LL14;case 1U: _LL13: _LL14:
 goto _LL16;case 9U: _LL15: _LL16:
# 666
 return*lowers;default: _LL17: _LL18:
# 668
 goto _LL0;}_LL0:;}
# 670
{struct Cyc_List_List*ls=*lowers;for(0;(unsigned)ls;ls=ls->tl){
void*t2=({Cyc_Port_compress_ct((void*)ls->hd);});
if(t == t2)return*lowers;{struct _tuple13 _stmttmp7=({struct _tuple13 _tmp4F2;_tmp4F2.f1=t,_tmp4F2.f2=t2;_tmp4F2;});struct _tuple13 _tmp121=_stmttmp7;void*_tmp12B;void*_tmp12A;void*_tmp129;void*_tmp128;void*_tmp127;void*_tmp126;void*_tmp125;void*_tmp124;void*_tmp123;void*_tmp122;if(((struct Cyc_Port_Void_ct_Port_Ctype_struct*)_tmp121.f2)->tag == 4U){_LL1A: _LL1B:
# 674
 goto _LL1D;}else{switch(*((int*)_tmp121.f1)){case 5U: switch(*((int*)_tmp121.f2)){case 6U: _LL1C: _LL1D:
 goto _LL1F;case 11U: _LL1E: _LL1F:
 goto _LL21;default: goto _LL26;}case 11U: switch(*((int*)_tmp121.f2)){case 6U: _LL20: _LL21:
 goto _LL23;case 11U: _LL24: _tmp122=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmp121.f1)->f1;_tmp123=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmp121.f1)->f2;_tmp124=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmp121.f1)->f3;_tmp125=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmp121.f1)->f4;_tmp126=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmp121.f1)->f5;_tmp127=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmp121.f2)->f1;_tmp128=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmp121.f2)->f2;_tmp129=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmp121.f2)->f3;_tmp12A=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmp121.f2)->f4;_tmp12B=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmp121.f2)->f5;_LL25: {void*ta=_tmp122;void*qa=_tmp123;void*ka=_tmp124;void*ra=_tmp125;void*za=_tmp126;void*tb=_tmp127;void*qb=_tmp128;void*kb=_tmp129;void*rb=_tmp12A;void*zb=_tmp12B;
# 684
struct _tuple14 _stmttmp8=({struct _tuple14 _tmp4F1;({void*_tmp549=({Cyc_Port_var();});_tmp4F1.f1=_tmp549;}),({void*_tmp548=({Cyc_Port_var();});_tmp4F1.f2=_tmp548;}),({void*_tmp547=({Cyc_Port_var();});_tmp4F1.f3=_tmp547;}),({void*_tmp546=({Cyc_Port_var();});_tmp4F1.f4=_tmp546;}),({void*_tmp545=({Cyc_Port_var();});_tmp4F1.f5=_tmp545;});_tmp4F1;});void*_tmp130;void*_tmp12F;void*_tmp12E;void*_tmp12D;void*_tmp12C;_LL29: _tmp12C=_stmttmp8.f1;_tmp12D=_stmttmp8.f2;_tmp12E=_stmttmp8.f3;_tmp12F=_stmttmp8.f4;_tmp130=_stmttmp8.f5;_LL2A: {void*tc=_tmp12C;void*qc=_tmp12D;void*kc=_tmp12E;void*rc=_tmp12F;void*zc=_tmp130;
struct Cyc_Port_Ptr_ct_Port_Ctype_struct*p=({struct Cyc_Port_Ptr_ct_Port_Ctype_struct*_tmp131=_cycalloc(sizeof(*_tmp131));_tmp131->tag=11U,_tmp131->f1=tc,_tmp131->f2=qc,_tmp131->f3=kc,_tmp131->f4=rc,_tmp131->f5=zc;_tmp131;});
({Cyc_Port_leq(ta,tc);});({Cyc_Port_leq(tb,tc);});
({Cyc_Port_leq(qa,qc);});({Cyc_Port_leq(qb,qc);});
({Cyc_Port_leq(ka,kc);});({Cyc_Port_leq(qb,kc);});
({Cyc_Port_leq(ra,rc);});({Cyc_Port_leq(rb,rc);});
({Cyc_Port_leq(za,zc);});({Cyc_Port_leq(zb,zc);});
ls->hd=(void*)((void*)p);
return*lowers;}}default: goto _LL26;}case 12U: if(((struct Cyc_Port_Arith_ct_Port_Ctype_struct*)_tmp121.f2)->tag == 6U){_LL22: _LL23:
# 679
 return*lowers;}else{goto _LL26;}default: _LL26: _LL27:
# 693
 goto _LL19;}}_LL19:;}}}
# 696
return({struct Cyc_List_List*_tmp132=_cycalloc(sizeof(*_tmp132));_tmp132->hd=t,_tmp132->tl=*lowers;_tmp132;});}
# 703
static int Cyc_Port_leq(void*t1,void*t2){
# 709
if(t1 == t2)return 1;t1=({Cyc_Port_compress_ct(t1);});
# 711
t2=({Cyc_Port_compress_ct(t2);});{
struct _tuple13 _stmttmp9=({struct _tuple13 _tmp4F3;_tmp4F3.f1=t1,_tmp4F3.f2=t2;_tmp4F3;});struct _tuple13 _tmp134=_stmttmp9;struct Cyc_Port_Cvar*_tmp135;void*_tmp13C;void*_tmp13B;void*_tmp13A;void*_tmp139;void*_tmp138;void*_tmp137;void*_tmp136;void*_tmp142;void*_tmp141;void*_tmp140;void*_tmp13F;void*_tmp13E;void*_tmp13D;void*_tmp14C;void*_tmp14B;void*_tmp14A;void*_tmp149;void*_tmp148;void*_tmp147;void*_tmp146;void*_tmp145;void*_tmp144;void*_tmp143;struct Cyc_Port_Cvar*_tmp14D;struct Cyc_Port_Cvar*_tmp14F;struct Cyc_Port_Cvar*_tmp14E;struct _fat_ptr _tmp150;struct _fat_ptr _tmp152;struct _fat_ptr _tmp151;switch(*((int*)_tmp134.f1)){case 7U: _LL1: _LL2:
 return 1;case 10U: switch(*((int*)_tmp134.f2)){case 10U: _LL3: _tmp151=*((struct Cyc_Port_RgnVar_ct_Port_Ctype_struct*)_tmp134.f1)->f1;_tmp152=*((struct Cyc_Port_RgnVar_ct_Port_Ctype_struct*)_tmp134.f2)->f1;_LL4: {struct _fat_ptr n1=_tmp151;struct _fat_ptr n2=_tmp152;
return({Cyc_strcmp((struct _fat_ptr)n1,(struct _fat_ptr)n2);})== 0;}case 7U: _LL5: _tmp150=*((struct Cyc_Port_RgnVar_ct_Port_Ctype_struct*)_tmp134.f1)->f1;_LL6: {struct _fat_ptr n1=_tmp150;
return 0;}case 16U: goto _LL2D;default: goto _LL2F;}case 1U: switch(*((int*)_tmp134.f2)){case 0U: _LL7: _LL8:
 return 1;case 16U: goto _LL2D;default: goto _LL2F;}case 0U: switch(*((int*)_tmp134.f2)){case 1U: _LL9: _LLA:
 return 0;case 16U: goto _LL2D;default: goto _LL2F;}case 9U: switch(*((int*)_tmp134.f2)){case 8U: _LLB: _LLC:
 return 0;case 16U: goto _LL2D;default: goto _LL2F;}case 8U: switch(*((int*)_tmp134.f2)){case 9U: _LLD: _LLE:
 return 1;case 16U: goto _LL2D;default: goto _LL2F;}case 16U: switch(*((int*)_tmp134.f2)){case 0U: _LLF: _LL10:
 return 1;case 4U: _LL11: _LL12:
 return 1;case 16U: _LL29: _tmp14E=((struct Cyc_Port_Var_ct_Port_Ctype_struct*)_tmp134.f1)->f1;_tmp14F=((struct Cyc_Port_Var_ct_Port_Ctype_struct*)_tmp134.f2)->f1;_LL2A: {struct Cyc_Port_Cvar*v1=_tmp14E;struct Cyc_Port_Cvar*v2=_tmp14F;
# 739
({struct Cyc_List_List*_tmp54A=({Cyc_Port_filter_self(t1,v1->uppers);});v1->uppers=_tmp54A;});
({struct Cyc_List_List*_tmp54B=({Cyc_Port_filter_self(t2,v2->lowers);});v2->lowers=_tmp54B;});
({struct Cyc_List_List*_tmp54C=({Cyc_Port_insert_upper(t1,t2,& v1->uppers);});v1->uppers=_tmp54C;});
({struct Cyc_List_List*_tmp54D=({Cyc_Port_insert_lower(t2,t1,& v2->lowers);});v2->lowers=_tmp54D;});
return 1;}default: _LL2B: _tmp14D=((struct Cyc_Port_Var_ct_Port_Ctype_struct*)_tmp134.f1)->f1;_LL2C: {struct Cyc_Port_Cvar*v1=_tmp14D;
# 745
({struct Cyc_List_List*_tmp54E=({Cyc_Port_filter_self(t1,v1->uppers);});v1->uppers=_tmp54E;});
({struct Cyc_List_List*_tmp54F=({Cyc_Port_insert_upper(t1,t2,& v1->uppers);});v1->uppers=_tmp54F;});
return 1;}}case 4U: _LL13: _LL14:
# 722
 return 0;case 5U: switch(*((int*)_tmp134.f2)){case 6U: _LL15: _LL16:
 return 1;case 11U: _LL17: _LL18:
 return 1;case 4U: _LL19: _LL1A:
 return 1;case 16U: goto _LL2D;default: goto _LL2F;}case 11U: switch(*((int*)_tmp134.f2)){case 6U: _LL1B: _LL1C:
 return 1;case 4U: _LL1D: _LL1E:
 return 1;case 11U: _LL23: _tmp143=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmp134.f1)->f1;_tmp144=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmp134.f1)->f2;_tmp145=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmp134.f1)->f3;_tmp146=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmp134.f1)->f4;_tmp147=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmp134.f1)->f5;_tmp148=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmp134.f2)->f1;_tmp149=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmp134.f2)->f2;_tmp14A=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmp134.f2)->f3;_tmp14B=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmp134.f2)->f4;_tmp14C=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmp134.f2)->f5;_LL24: {void*t1=_tmp143;void*q1=_tmp144;void*k1=_tmp145;void*r1=_tmp146;void*z1=_tmp147;void*t2=_tmp148;void*q2=_tmp149;void*k2=_tmp14A;void*r2=_tmp14B;void*z2=_tmp14C;
# 731
return(((({Cyc_Port_leq(t1,t2);})&&({Cyc_Port_leq(q1,q2);}))&&({Cyc_Port_unifies(k1,k2);}))&&({Cyc_Port_leq(r1,r2);}))&&({Cyc_Port_leq(z1,z2);});}case 16U: goto _LL2D;default: goto _LL2F;}case 12U: switch(*((int*)_tmp134.f2)){case 6U: _LL1F: _LL20:
# 728
 return 1;case 4U: _LL21: _LL22:
 return 1;case 12U: _LL25: _tmp13D=(void*)((struct Cyc_Port_Array_ct_Port_Ctype_struct*)_tmp134.f1)->f1;_tmp13E=(void*)((struct Cyc_Port_Array_ct_Port_Ctype_struct*)_tmp134.f1)->f2;_tmp13F=(void*)((struct Cyc_Port_Array_ct_Port_Ctype_struct*)_tmp134.f1)->f3;_tmp140=(void*)((struct Cyc_Port_Array_ct_Port_Ctype_struct*)_tmp134.f2)->f1;_tmp141=(void*)((struct Cyc_Port_Array_ct_Port_Ctype_struct*)_tmp134.f2)->f2;_tmp142=(void*)((struct Cyc_Port_Array_ct_Port_Ctype_struct*)_tmp134.f2)->f3;_LL26: {void*t1=_tmp13D;void*q1=_tmp13E;void*z1=_tmp13F;void*t2=_tmp140;void*q2=_tmp141;void*z2=_tmp142;
# 734
return(({Cyc_Port_leq(t1,t2);})&&({Cyc_Port_leq(q1,q2);}))&&({Cyc_Port_leq(z1,z2);});}case 11U: _LL27: _tmp136=(void*)((struct Cyc_Port_Array_ct_Port_Ctype_struct*)_tmp134.f1)->f1;_tmp137=(void*)((struct Cyc_Port_Array_ct_Port_Ctype_struct*)_tmp134.f1)->f2;_tmp138=(void*)((struct Cyc_Port_Array_ct_Port_Ctype_struct*)_tmp134.f1)->f3;_tmp139=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmp134.f2)->f1;_tmp13A=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmp134.f2)->f2;_tmp13B=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmp134.f2)->f3;_tmp13C=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmp134.f2)->f5;_LL28: {void*t1=_tmp136;void*q1=_tmp137;void*z1=_tmp138;void*t2=_tmp139;void*q2=_tmp13A;void*k2=_tmp13B;void*z2=_tmp13C;
# 736
return((({Cyc_Port_leq(t1,t2);})&&({Cyc_Port_leq(q1,q2);}))&&({Cyc_Port_unifies((void*)& Cyc_Port_Fat_ct_val,k2);}))&&({Cyc_Port_leq(z1,z2);});}case 16U: goto _LL2D;default: goto _LL2F;}default: if(((struct Cyc_Port_Var_ct_Port_Ctype_struct*)_tmp134.f2)->tag == 16U){_LL2D: _tmp135=((struct Cyc_Port_Var_ct_Port_Ctype_struct*)_tmp134.f2)->f1;_LL2E: {struct Cyc_Port_Cvar*v2=_tmp135;
# 749
({struct Cyc_List_List*_tmp550=({Cyc_Port_filter_self(t2,v2->lowers);});v2->lowers=_tmp550;});
({struct Cyc_List_List*_tmp551=({Cyc_Port_insert_lower(t2,t1,& v2->lowers);});v2->lowers=_tmp551;});
return 1;}}else{_LL2F: _LL30:
 return({Cyc_Port_unifies(t1,t2);});}}_LL0:;}}struct Cyc_Port_GlobalCenv{struct Cyc_Dict_Dict typedef_dict;struct Cyc_Dict_Dict struct_dict;struct Cyc_Dict_Dict union_dict;void*return_type;struct Cyc_List_List*qualifier_edits;struct Cyc_List_List*pointer_edits;struct Cyc_List_List*zeroterm_edits;struct Cyc_List_List*vardecl_locs;struct Cyc_Hashtable_Table*varusage_tab;struct Cyc_List_List*edits;int porting;};
# 783
enum Cyc_Port_CPos{Cyc_Port_FnRes_pos =0U,Cyc_Port_FnArg_pos =1U,Cyc_Port_FnBody_pos =2U,Cyc_Port_Toplevel_pos =3U};struct Cyc_Port_Cenv{struct Cyc_Port_GlobalCenv*gcenv;struct Cyc_Dict_Dict var_dict;enum Cyc_Port_CPos cpos;};
# 796
static struct Cyc_Port_Cenv*Cyc_Port_empty_cenv(){
struct Cyc_Port_GlobalCenv*g=({struct Cyc_Port_GlobalCenv*_tmp15A=_cycalloc(sizeof(*_tmp15A));({struct Cyc_Dict_Dict _tmp556=({({struct Cyc_Dict_Dict(*_tmp156)(int(*cmp)(struct _tuple0*,struct _tuple0*))=(struct Cyc_Dict_Dict(*)(int(*cmp)(struct _tuple0*,struct _tuple0*)))Cyc_Dict_empty;_tmp156;})(Cyc_Absyn_qvar_cmp);});_tmp15A->typedef_dict=_tmp556;}),({
struct Cyc_Dict_Dict _tmp555=({({struct Cyc_Dict_Dict(*_tmp157)(int(*cmp)(struct _tuple0*,struct _tuple0*))=(struct Cyc_Dict_Dict(*)(int(*cmp)(struct _tuple0*,struct _tuple0*)))Cyc_Dict_empty;_tmp157;})(Cyc_Absyn_qvar_cmp);});_tmp15A->struct_dict=_tmp555;}),({
struct Cyc_Dict_Dict _tmp554=({({struct Cyc_Dict_Dict(*_tmp158)(int(*cmp)(struct _tuple0*,struct _tuple0*))=(struct Cyc_Dict_Dict(*)(int(*cmp)(struct _tuple0*,struct _tuple0*)))Cyc_Dict_empty;_tmp158;})(Cyc_Absyn_qvar_cmp);});_tmp15A->union_dict=_tmp554;}),_tmp15A->qualifier_edits=0,_tmp15A->pointer_edits=0,_tmp15A->zeroterm_edits=0,_tmp15A->vardecl_locs=0,({
# 804
struct Cyc_Hashtable_Table*_tmp553=({({struct Cyc_Hashtable_Table*(*_tmp159)(int sz,int(*cmp)(unsigned,unsigned),int(*hash)(unsigned))=(struct Cyc_Hashtable_Table*(*)(int sz,int(*cmp)(unsigned,unsigned),int(*hash)(unsigned)))Cyc_Hashtable_create;_tmp159;})(128,Cyc_Port_cmp_seg_t,Cyc_Port_hash_seg_t);});_tmp15A->varusage_tab=_tmp553;}),_tmp15A->edits=0,_tmp15A->porting=0,({
# 807
void*_tmp552=({Cyc_Port_void_ct();});_tmp15A->return_type=_tmp552;});_tmp15A;});
return({struct Cyc_Port_Cenv*_tmp155=_cycalloc(sizeof(*_tmp155));_tmp155->gcenv=g,_tmp155->cpos=Cyc_Port_Toplevel_pos,({
# 810
struct Cyc_Dict_Dict _tmp557=({({struct Cyc_Dict_Dict(*_tmp154)(int(*cmp)(struct _tuple0*,struct _tuple0*))=(struct Cyc_Dict_Dict(*)(int(*cmp)(struct _tuple0*,struct _tuple0*)))Cyc_Dict_empty;_tmp154;})(Cyc_Absyn_qvar_cmp);});_tmp155->var_dict=_tmp557;});_tmp155;});}
# 816
static int Cyc_Port_in_fn_arg(struct Cyc_Port_Cenv*env){
return(int)env->cpos == (int)Cyc_Port_FnArg_pos;}
# 819
static int Cyc_Port_in_fn_res(struct Cyc_Port_Cenv*env){
return(int)env->cpos == (int)Cyc_Port_FnRes_pos;}
# 822
static int Cyc_Port_in_toplevel(struct Cyc_Port_Cenv*env){
return(int)env->cpos == (int)Cyc_Port_Toplevel_pos;}
# 825
static void*Cyc_Port_lookup_return_type(struct Cyc_Port_Cenv*env){
return(env->gcenv)->return_type;}
# 828
static void*Cyc_Port_lookup_typedef(struct Cyc_Port_Cenv*env,struct _tuple0*n){
struct _handler_cons _tmp160;_push_handler(& _tmp160);{int _tmp162=0;if(setjmp(_tmp160.handler))_tmp162=1;if(!_tmp162){
{struct _tuple13 _stmttmpA=*({({struct _tuple13*(*_tmp559)(struct Cyc_Dict_Dict d,struct _tuple0*k)=({struct _tuple13*(*_tmp165)(struct Cyc_Dict_Dict d,struct _tuple0*k)=(struct _tuple13*(*)(struct Cyc_Dict_Dict d,struct _tuple0*k))Cyc_Dict_lookup;_tmp165;});struct Cyc_Dict_Dict _tmp558=(env->gcenv)->typedef_dict;_tmp559(_tmp558,n);});});void*_tmp163;_LL1: _tmp163=_stmttmpA.f1;_LL2: {void*t=_tmp163;
void*_tmp164=t;_npop_handler(0U);return _tmp164;}}
# 830
;_pop_handler();}else{void*_tmp161=(void*)Cyc_Core_get_exn_thrown();void*_tmp166=_tmp161;void*_tmp167;if(((struct Cyc_Dict_Absent_exn_struct*)_tmp166)->tag == Cyc_Dict_Absent){_LL4: _LL5:
# 837
 return Cyc_Absyn_sint_type;}else{_LL6: _tmp167=_tmp166;_LL7: {void*exn=_tmp167;(int)_rethrow(exn);}}_LL3:;}}}
# 842
static void*Cyc_Port_lookup_typedef_ctype(struct Cyc_Port_Cenv*env,struct _tuple0*n){
struct _handler_cons _tmp169;_push_handler(& _tmp169);{int _tmp16B=0;if(setjmp(_tmp169.handler))_tmp16B=1;if(!_tmp16B){
{struct _tuple13 _stmttmpB=*({({struct _tuple13*(*_tmp55B)(struct Cyc_Dict_Dict d,struct _tuple0*k)=({struct _tuple13*(*_tmp16E)(struct Cyc_Dict_Dict d,struct _tuple0*k)=(struct _tuple13*(*)(struct Cyc_Dict_Dict d,struct _tuple0*k))Cyc_Dict_lookup;_tmp16E;});struct Cyc_Dict_Dict _tmp55A=(env->gcenv)->typedef_dict;_tmp55B(_tmp55A,n);});});void*_tmp16C;_LL1: _tmp16C=_stmttmpB.f2;_LL2: {void*ct=_tmp16C;
void*_tmp16D=ct;_npop_handler(0U);return _tmp16D;}}
# 844
;_pop_handler();}else{void*_tmp16A=(void*)Cyc_Core_get_exn_thrown();void*_tmp16F=_tmp16A;void*_tmp170;if(((struct Cyc_Dict_Absent_exn_struct*)_tmp16F)->tag == Cyc_Dict_Absent){_LL4: _LL5:
# 851
 return({Cyc_Port_var();});}else{_LL6: _tmp170=_tmp16F;_LL7: {void*exn=_tmp170;(int)_rethrow(exn);}}_LL3:;}}}
# 857
static struct _tuple12*Cyc_Port_lookup_struct_decl(struct Cyc_Port_Cenv*env,struct _tuple0*n){
# 859
struct _tuple12**popt=({({struct _tuple12**(*_tmp55D)(struct Cyc_Dict_Dict d,struct _tuple0*k)=({struct _tuple12**(*_tmp175)(struct Cyc_Dict_Dict d,struct _tuple0*k)=(struct _tuple12**(*)(struct Cyc_Dict_Dict d,struct _tuple0*k))Cyc_Dict_lookup_opt;_tmp175;});struct Cyc_Dict_Dict _tmp55C=(env->gcenv)->struct_dict;_tmp55D(_tmp55C,n);});});
if((unsigned)popt)
return*popt;else{
# 863
struct Cyc_Absyn_Aggrdecl*ad=({struct Cyc_Absyn_Aggrdecl*_tmp174=_cycalloc(sizeof(*_tmp174));_tmp174->kind=Cyc_Absyn_StructA,_tmp174->sc=Cyc_Absyn_Public,_tmp174->name=n,_tmp174->tvs=0,_tmp174->impl=0,_tmp174->attributes=0,_tmp174->expected_mem_kind=0;_tmp174;});
# 866
struct _tuple12*p=({struct _tuple12*_tmp173=_cycalloc(sizeof(*_tmp173));_tmp173->f1=ad,_tmp173->f2=0;_tmp173;});
({struct Cyc_Dict_Dict _tmp561=({({struct Cyc_Dict_Dict(*_tmp560)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct _tuple12*v)=({struct Cyc_Dict_Dict(*_tmp172)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct _tuple12*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct _tuple12*v))Cyc_Dict_insert;_tmp172;});struct Cyc_Dict_Dict _tmp55F=(env->gcenv)->struct_dict;struct _tuple0*_tmp55E=n;_tmp560(_tmp55F,_tmp55E,p);});});(env->gcenv)->struct_dict=_tmp561;});
return p;}}
# 874
static struct _tuple12*Cyc_Port_lookup_union_decl(struct Cyc_Port_Cenv*env,struct _tuple0*n){
# 876
struct _tuple12**popt=({({struct _tuple12**(*_tmp563)(struct Cyc_Dict_Dict d,struct _tuple0*k)=({struct _tuple12**(*_tmp17A)(struct Cyc_Dict_Dict d,struct _tuple0*k)=(struct _tuple12**(*)(struct Cyc_Dict_Dict d,struct _tuple0*k))Cyc_Dict_lookup_opt;_tmp17A;});struct Cyc_Dict_Dict _tmp562=(env->gcenv)->union_dict;_tmp563(_tmp562,n);});});
if((unsigned)popt)
return*popt;else{
# 880
struct Cyc_Absyn_Aggrdecl*ad=({struct Cyc_Absyn_Aggrdecl*_tmp179=_cycalloc(sizeof(*_tmp179));_tmp179->kind=Cyc_Absyn_UnionA,_tmp179->sc=Cyc_Absyn_Public,_tmp179->name=n,_tmp179->tvs=0,_tmp179->impl=0,_tmp179->attributes=0,_tmp179->expected_mem_kind=0;_tmp179;});
# 883
struct _tuple12*p=({struct _tuple12*_tmp178=_cycalloc(sizeof(*_tmp178));_tmp178->f1=ad,_tmp178->f2=0;_tmp178;});
({struct Cyc_Dict_Dict _tmp567=({({struct Cyc_Dict_Dict(*_tmp566)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct _tuple12*v)=({struct Cyc_Dict_Dict(*_tmp177)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct _tuple12*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct _tuple12*v))Cyc_Dict_insert;_tmp177;});struct Cyc_Dict_Dict _tmp565=(env->gcenv)->union_dict;struct _tuple0*_tmp564=n;_tmp566(_tmp565,_tmp564,p);});});(env->gcenv)->union_dict=_tmp567;});
return p;}}struct _tuple15{struct _tuple13 f1;unsigned f2;};struct _tuple16{void*f1;struct _tuple13*f2;unsigned f3;};
# 890
static struct _tuple15 Cyc_Port_lookup_var(struct Cyc_Port_Cenv*env,struct _tuple0*x){
struct _handler_cons _tmp17C;_push_handler(& _tmp17C);{int _tmp17E=0;if(setjmp(_tmp17C.handler))_tmp17E=1;if(!_tmp17E){
{struct _tuple16 _stmttmpC=*({({struct _tuple16*(*_tmp569)(struct Cyc_Dict_Dict d,struct _tuple0*k)=({struct _tuple16*(*_tmp182)(struct Cyc_Dict_Dict d,struct _tuple0*k)=(struct _tuple16*(*)(struct Cyc_Dict_Dict d,struct _tuple0*k))Cyc_Dict_lookup;_tmp182;});struct Cyc_Dict_Dict _tmp568=env->var_dict;_tmp569(_tmp568,x);});});unsigned _tmp180;struct _tuple13*_tmp17F;_LL1: _tmp17F=_stmttmpC.f2;_tmp180=_stmttmpC.f3;_LL2: {struct _tuple13*p=_tmp17F;unsigned loc=_tmp180;
struct _tuple15 _tmp181=({struct _tuple15 _tmp4F4;_tmp4F4.f1=*p,_tmp4F4.f2=loc;_tmp4F4;});_npop_handler(0U);return _tmp181;}}
# 892
;_pop_handler();}else{void*_tmp17D=(void*)Cyc_Core_get_exn_thrown();void*_tmp183=_tmp17D;void*_tmp184;if(((struct Cyc_Dict_Absent_exn_struct*)_tmp183)->tag == Cyc_Dict_Absent){_LL4: _LL5:
# 899
 return({struct _tuple15 _tmp4F5;({void*_tmp56B=({Cyc_Port_var();});(_tmp4F5.f1).f1=_tmp56B;}),({void*_tmp56A=({Cyc_Port_var();});(_tmp4F5.f1).f2=_tmp56A;}),_tmp4F5.f2=0U;_tmp4F5;});}else{_LL6: _tmp184=_tmp183;_LL7: {void*exn=_tmp184;(int)_rethrow(exn);}}_LL3:;}}}
# 902
static struct _tuple16*Cyc_Port_lookup_full_var(struct Cyc_Port_Cenv*env,struct _tuple0*x){
return({({struct _tuple16*(*_tmp56D)(struct Cyc_Dict_Dict d,struct _tuple0*k)=({struct _tuple16*(*_tmp186)(struct Cyc_Dict_Dict d,struct _tuple0*k)=(struct _tuple16*(*)(struct Cyc_Dict_Dict d,struct _tuple0*k))Cyc_Dict_lookup;_tmp186;});struct Cyc_Dict_Dict _tmp56C=env->var_dict;_tmp56D(_tmp56C,x);});});}
# 906
static int Cyc_Port_declared_var(struct Cyc_Port_Cenv*env,struct _tuple0*x){
return({({int(*_tmp56F)(struct Cyc_Dict_Dict d,struct _tuple0*k)=({int(*_tmp188)(struct Cyc_Dict_Dict d,struct _tuple0*k)=(int(*)(struct Cyc_Dict_Dict d,struct _tuple0*k))Cyc_Dict_member;_tmp188;});struct Cyc_Dict_Dict _tmp56E=env->var_dict;_tmp56F(_tmp56E,x);});});}
# 910
static int Cyc_Port_isfn(struct Cyc_Port_Cenv*env,struct _tuple0*x){
struct _handler_cons _tmp18A;_push_handler(& _tmp18A);{int _tmp18C=0;if(setjmp(_tmp18A.handler))_tmp18C=1;if(!_tmp18C){
{struct _tuple16 _stmttmpD=*({({struct _tuple16*(*_tmp571)(struct Cyc_Dict_Dict d,struct _tuple0*k)=({struct _tuple16*(*_tmp192)(struct Cyc_Dict_Dict d,struct _tuple0*k)=(struct _tuple16*(*)(struct Cyc_Dict_Dict d,struct _tuple0*k))Cyc_Dict_lookup;_tmp192;});struct Cyc_Dict_Dict _tmp570=env->var_dict;_tmp571(_tmp570,x);});});void*_tmp18D;_LL1: _tmp18D=_stmttmpD.f1;_LL2: {void*t=_tmp18D;
LOOP: {void*_tmp18E=t;struct _tuple0*_tmp18F;switch(*((int*)_tmp18E)){case 8U: _LL4: _tmp18F=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp18E)->f1;_LL5: {struct _tuple0*n=_tmp18F;
t=({Cyc_Port_lookup_typedef(env,n);});goto LOOP;}case 5U: _LL6: _LL7: {
int _tmp190=1;_npop_handler(0U);return _tmp190;}default: _LL8: _LL9: {
int _tmp191=0;_npop_handler(0U);return _tmp191;}}_LL3:;}}}
# 912
;_pop_handler();}else{void*_tmp18B=(void*)Cyc_Core_get_exn_thrown();void*_tmp193=_tmp18B;void*_tmp194;if(((struct Cyc_Dict_Absent_exn_struct*)_tmp193)->tag == Cyc_Dict_Absent){_LLB: _LLC:
# 923
 return 0;}else{_LLD: _tmp194=_tmp193;_LLE: {void*exn=_tmp194;(int)_rethrow(exn);}}_LLA:;}}}
# 927
static int Cyc_Port_isarray(struct Cyc_Port_Cenv*env,struct _tuple0*x){
struct _handler_cons _tmp196;_push_handler(& _tmp196);{int _tmp198=0;if(setjmp(_tmp196.handler))_tmp198=1;if(!_tmp198){
{struct _tuple16 _stmttmpE=*({({struct _tuple16*(*_tmp573)(struct Cyc_Dict_Dict d,struct _tuple0*k)=({struct _tuple16*(*_tmp19E)(struct Cyc_Dict_Dict d,struct _tuple0*k)=(struct _tuple16*(*)(struct Cyc_Dict_Dict d,struct _tuple0*k))Cyc_Dict_lookup;_tmp19E;});struct Cyc_Dict_Dict _tmp572=env->var_dict;_tmp573(_tmp572,x);});});void*_tmp199;_LL1: _tmp199=_stmttmpE.f1;_LL2: {void*t=_tmp199;
LOOP: {void*_tmp19A=t;struct _tuple0*_tmp19B;switch(*((int*)_tmp19A)){case 8U: _LL4: _tmp19B=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp19A)->f1;_LL5: {struct _tuple0*n=_tmp19B;
t=({Cyc_Port_lookup_typedef(env,n);});goto LOOP;}case 4U: _LL6: _LL7: {
int _tmp19C=1;_npop_handler(0U);return _tmp19C;}default: _LL8: _LL9: {
int _tmp19D=0;_npop_handler(0U);return _tmp19D;}}_LL3:;}}}
# 929
;_pop_handler();}else{void*_tmp197=(void*)Cyc_Core_get_exn_thrown();void*_tmp19F=_tmp197;void*_tmp1A0;if(((struct Cyc_Dict_Absent_exn_struct*)_tmp19F)->tag == Cyc_Dict_Absent){_LLB: _LLC:
# 940
 return 0;}else{_LLD: _tmp1A0=_tmp19F;_LLE: {void*exn=_tmp1A0;(int)_rethrow(exn);}}_LLA:;}}}
# 946
static struct Cyc_Port_Cenv*Cyc_Port_set_cpos(struct Cyc_Port_Cenv*env,enum Cyc_Port_CPos cpos){
return({struct Cyc_Port_Cenv*_tmp1A2=_cycalloc(sizeof(*_tmp1A2));_tmp1A2->gcenv=env->gcenv,_tmp1A2->var_dict=env->var_dict,_tmp1A2->cpos=cpos;_tmp1A2;});}
# 951
static void Cyc_Port_add_return_type(struct Cyc_Port_Cenv*env,void*t){
(env->gcenv)->return_type=t;}
# 956
static struct Cyc_Port_Cenv*Cyc_Port_add_var(struct Cyc_Port_Cenv*env,struct _tuple0*x,void*t,void*qual,void*ctype,unsigned loc){
# 958
return({struct Cyc_Port_Cenv*_tmp1A8=_cycalloc(sizeof(*_tmp1A8));_tmp1A8->gcenv=env->gcenv,({
struct Cyc_Dict_Dict _tmp578=({({struct Cyc_Dict_Dict(*_tmp577)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct _tuple16*v)=({struct Cyc_Dict_Dict(*_tmp1A5)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct _tuple16*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct _tuple16*v))Cyc_Dict_insert;_tmp1A5;});struct Cyc_Dict_Dict _tmp576=env->var_dict;struct _tuple0*_tmp575=x;_tmp577(_tmp576,_tmp575,({struct _tuple16*_tmp1A7=_cycalloc(sizeof(*_tmp1A7));_tmp1A7->f1=t,({struct _tuple13*_tmp574=({struct _tuple13*_tmp1A6=_cycalloc(sizeof(*_tmp1A6));_tmp1A6->f1=qual,_tmp1A6->f2=ctype;_tmp1A6;});_tmp1A7->f2=_tmp574;}),_tmp1A7->f3=loc;_tmp1A7;}));});});_tmp1A8->var_dict=_tmp578;}),_tmp1A8->cpos=env->cpos;_tmp1A8;});}
# 964
static void Cyc_Port_add_typedef(struct Cyc_Port_Cenv*env,struct _tuple0*n,void*t,void*ct){
({struct Cyc_Dict_Dict _tmp57C=({({struct Cyc_Dict_Dict(*_tmp57B)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct _tuple13*v)=({struct Cyc_Dict_Dict(*_tmp1AA)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct _tuple13*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct _tuple13*v))Cyc_Dict_insert;_tmp1AA;});struct Cyc_Dict_Dict _tmp57A=(env->gcenv)->typedef_dict;struct _tuple0*_tmp579=n;_tmp57B(_tmp57A,_tmp579,({struct _tuple13*_tmp1AB=_cycalloc(sizeof(*_tmp1AB));_tmp1AB->f1=t,_tmp1AB->f2=ct;_tmp1AB;}));});});(env->gcenv)->typedef_dict=_tmp57C;});}struct _tuple17{struct _tuple0*f1;unsigned f2;void*f3;};
# 968
static void Cyc_Port_register_localvar_decl(struct Cyc_Port_Cenv*env,struct _tuple0*x,unsigned loc,void*type,struct Cyc_Absyn_Exp*init){
({struct Cyc_List_List*_tmp57E=({struct Cyc_List_List*_tmp1AE=_cycalloc(sizeof(*_tmp1AE));({struct _tuple17*_tmp57D=({struct _tuple17*_tmp1AD=_cycalloc(sizeof(*_tmp1AD));_tmp1AD->f1=x,_tmp1AD->f2=loc,_tmp1AD->f3=type;_tmp1AD;});_tmp1AE->hd=_tmp57D;}),_tmp1AE->tl=(env->gcenv)->vardecl_locs;_tmp1AE;});(env->gcenv)->vardecl_locs=_tmp57E;});
({({void(*_tmp581)(struct Cyc_Hashtable_Table*t,unsigned key,struct Cyc_Port_VarUsage*val)=({void(*_tmp1AF)(struct Cyc_Hashtable_Table*t,unsigned key,struct Cyc_Port_VarUsage*val)=(void(*)(struct Cyc_Hashtable_Table*t,unsigned key,struct Cyc_Port_VarUsage*val))Cyc_Hashtable_insert;_tmp1AF;});struct Cyc_Hashtable_Table*_tmp580=(env->gcenv)->varusage_tab;unsigned _tmp57F=loc;_tmp581(_tmp580,_tmp57F,({struct Cyc_Port_VarUsage*_tmp1B0=_cycalloc(sizeof(*_tmp1B0));_tmp1B0->address_taken=0,_tmp1B0->init=init,_tmp1B0->usage_locs=0;_tmp1B0;}));});});}
# 972
static void Cyc_Port_register_localvar_usage(struct Cyc_Port_Cenv*env,unsigned declloc,unsigned useloc,int takeaddress){
struct Cyc_Port_VarUsage*varusage=0;
if(({({int(*_tmp583)(struct Cyc_Hashtable_Table*t,unsigned key,struct Cyc_Port_VarUsage**data)=({int(*_tmp1B2)(struct Cyc_Hashtable_Table*t,unsigned key,struct Cyc_Port_VarUsage**data)=(int(*)(struct Cyc_Hashtable_Table*t,unsigned key,struct Cyc_Port_VarUsage**data))Cyc_Hashtable_try_lookup;_tmp1B2;});struct Cyc_Hashtable_Table*_tmp582=(env->gcenv)->varusage_tab;_tmp583(_tmp582,declloc,& varusage);});})){
if(takeaddress)((struct Cyc_Port_VarUsage*)_check_null(varusage))->address_taken=1;{struct Cyc_List_List*l=((struct Cyc_Port_VarUsage*)_check_null(varusage))->usage_locs;
# 977
({struct Cyc_List_List*_tmp584=({struct Cyc_List_List*_tmp1B3=_cycalloc(sizeof(*_tmp1B3));_tmp1B3->hd=(void*)useloc,_tmp1B3->tl=l;_tmp1B3;});((struct Cyc_Port_VarUsage*)_check_null(varusage))->usage_locs=_tmp584;});}}}struct _tuple18{void*f1;void*f2;unsigned f3;};
# 983
static void Cyc_Port_register_const_cvar(struct Cyc_Port_Cenv*env,void*new_qual,void*orig_qual,unsigned loc){
# 985
({struct Cyc_List_List*_tmp586=({struct Cyc_List_List*_tmp1B6=_cycalloc(sizeof(*_tmp1B6));({struct _tuple18*_tmp585=({struct _tuple18*_tmp1B5=_cycalloc(sizeof(*_tmp1B5));_tmp1B5->f1=new_qual,_tmp1B5->f2=orig_qual,_tmp1B5->f3=loc;_tmp1B5;});_tmp1B6->hd=_tmp585;}),_tmp1B6->tl=(env->gcenv)->qualifier_edits;_tmp1B6;});(env->gcenv)->qualifier_edits=_tmp586;});}
# 991
static void Cyc_Port_register_ptr_cvars(struct Cyc_Port_Cenv*env,void*new_ptr,void*orig_ptr,unsigned loc){
# 993
({struct Cyc_List_List*_tmp588=({struct Cyc_List_List*_tmp1B9=_cycalloc(sizeof(*_tmp1B9));({struct _tuple18*_tmp587=({struct _tuple18*_tmp1B8=_cycalloc(sizeof(*_tmp1B8));_tmp1B8->f1=new_ptr,_tmp1B8->f2=orig_ptr,_tmp1B8->f3=loc;_tmp1B8;});_tmp1B9->hd=_tmp587;}),_tmp1B9->tl=(env->gcenv)->pointer_edits;_tmp1B9;});(env->gcenv)->pointer_edits=_tmp588;});}
# 997
static void Cyc_Port_register_zeroterm_cvars(struct Cyc_Port_Cenv*env,void*new_zt,void*orig_zt,unsigned loc){
# 999
({struct Cyc_List_List*_tmp58A=({struct Cyc_List_List*_tmp1BC=_cycalloc(sizeof(*_tmp1BC));({struct _tuple18*_tmp589=({struct _tuple18*_tmp1BB=_cycalloc(sizeof(*_tmp1BB));_tmp1BB->f1=new_zt,_tmp1BB->f2=orig_zt,_tmp1BB->f3=loc;_tmp1BB;});_tmp1BC->hd=_tmp589;}),_tmp1BC->tl=(env->gcenv)->zeroterm_edits;_tmp1BC;});(env->gcenv)->zeroterm_edits=_tmp58A;});}
# 1005
static void*Cyc_Port_main_type(){
struct _tuple9*arg1=({struct _tuple9*_tmp1C8=_cycalloc(sizeof(*_tmp1C8));
_tmp1C8->f1=0,({struct Cyc_Absyn_Tqual _tmp58B=({Cyc_Absyn_empty_tqual(0U);});_tmp1C8->f2=_tmp58B;}),_tmp1C8->f3=Cyc_Absyn_sint_type;_tmp1C8;});
struct _tuple9*arg2=({struct _tuple9*_tmp1C7=_cycalloc(sizeof(*_tmp1C7));
_tmp1C7->f1=0,({struct Cyc_Absyn_Tqual _tmp58D=({Cyc_Absyn_empty_tqual(0U);});_tmp1C7->f2=_tmp58D;}),({
void*_tmp58C=({void*(*_tmp1C0)(void*t,void*rgn,struct Cyc_Absyn_Tqual,void*zeroterm)=Cyc_Absyn_fatptr_type;void*_tmp1C1=({void*(*_tmp1C2)(void*rgn)=Cyc_Absyn_string_type;void*_tmp1C3=({Cyc_Absyn_wildtyp(0);});_tmp1C2(_tmp1C3);});void*_tmp1C4=({Cyc_Absyn_wildtyp(0);});struct Cyc_Absyn_Tqual _tmp1C5=({Cyc_Absyn_empty_tqual(0U);});void*_tmp1C6=({Cyc_Tcutil_any_bool(0);});_tmp1C0(_tmp1C1,_tmp1C4,_tmp1C5,_tmp1C6);});_tmp1C7->f3=_tmp58C;});_tmp1C7;});
# 1012
return(void*)({struct Cyc_Absyn_FnType_Absyn_Type_struct*_tmp1BF=_cycalloc(sizeof(*_tmp1BF));_tmp1BF->tag=5U,(_tmp1BF->f1).tvars=0,(_tmp1BF->f1).effect=0,({
struct Cyc_Absyn_Tqual _tmp58F=({Cyc_Absyn_empty_tqual(0U);});(_tmp1BF->f1).ret_tqual=_tmp58F;}),(_tmp1BF->f1).ret_type=Cyc_Absyn_sint_type,({
# 1015
struct Cyc_List_List*_tmp58E=({struct _tuple9*_tmp1BE[2U];_tmp1BE[0]=arg1,_tmp1BE[1]=arg2;Cyc_List_list(_tag_fat(_tmp1BE,sizeof(struct _tuple9*),2U));});(_tmp1BF->f1).args=_tmp58E;}),(_tmp1BF->f1).c_varargs=0,(_tmp1BF->f1).cyc_varargs=0,(_tmp1BF->f1).rgn_po=0,(_tmp1BF->f1).attributes=0,(_tmp1BF->f1).requires_clause=0,(_tmp1BF->f1).requires_relns=0,(_tmp1BF->f1).ensures_clause=0,(_tmp1BF->f1).ensures_relns=0,(_tmp1BF->f1).ieffect=0,(_tmp1BF->f1).oeffect=0,(_tmp1BF->f1).throws=0,(_tmp1BF->f1).reentrant=Cyc_Absyn_noreentrant;_tmp1BF;});}
# 1005
static void*Cyc_Port_type_to_ctype(struct Cyc_Port_Cenv*env,void*t);
# 1031
static struct Cyc_Port_Cenv*Cyc_Port_initial_cenv(){
struct Cyc_Port_Cenv*e=({Cyc_Port_empty_cenv();});
# 1034
e=({struct Cyc_Port_Cenv*(*_tmp1CA)(struct Cyc_Port_Cenv*env,struct _tuple0*x,void*t,void*qual,void*ctype,unsigned loc)=Cyc_Port_add_var;struct Cyc_Port_Cenv*_tmp1CB=e;struct _tuple0*_tmp1CC=({struct _tuple0*_tmp1CF=_cycalloc(sizeof(*_tmp1CF));_tmp1CF->f1=Cyc_Absyn_Loc_n,({struct _fat_ptr*_tmp591=({struct _fat_ptr*_tmp1CE=_cycalloc(sizeof(*_tmp1CE));({struct _fat_ptr _tmp590=({const char*_tmp1CD="main";_tag_fat(_tmp1CD,sizeof(char),5U);});*_tmp1CE=_tmp590;});_tmp1CE;});_tmp1CF->f2=_tmp591;});_tmp1CF;});void*_tmp1D0=({Cyc_Port_main_type();});void*_tmp1D1=({Cyc_Port_const_ct();});void*_tmp1D2=({void*(*_tmp1D3)(struct Cyc_Port_Cenv*env,void*t)=Cyc_Port_type_to_ctype;struct Cyc_Port_Cenv*_tmp1D4=e;void*_tmp1D5=({Cyc_Port_main_type();});_tmp1D3(_tmp1D4,_tmp1D5);});unsigned _tmp1D6=0U;_tmp1CA(_tmp1CB,_tmp1CC,_tmp1D0,_tmp1D1,_tmp1D2,_tmp1D6);});
# 1036
return e;}
# 1042
static void Cyc_Port_region_counts(struct Cyc_Dict_Dict*cts,void*t){
void*_stmttmpF=({Cyc_Port_compress_ct(t);});void*_tmp1D8=_stmttmpF;struct Cyc_List_List*_tmp1DA;void*_tmp1D9;void*_tmp1DD;void*_tmp1DC;void*_tmp1DB;void*_tmp1E2;void*_tmp1E1;void*_tmp1E0;void*_tmp1DF;void*_tmp1DE;struct _fat_ptr*_tmp1E3;switch(*((int*)_tmp1D8)){case 0U: _LL1: _LL2:
 goto _LL4;case 1U: _LL3: _LL4:
 goto _LL6;case 2U: _LL5: _LL6:
 goto _LL8;case 3U: _LL7: _LL8:
 goto _LLA;case 4U: _LL9: _LLA:
 goto _LLC;case 5U: _LLB: _LLC:
 goto _LLE;case 6U: _LLD: _LLE:
 goto _LL10;case 14U: _LLF: _LL10:
 goto _LL12;case 13U: _LL11: _LL12:
 goto _LL14;case 16U: _LL13: _LL14:
 goto _LL16;case 8U: _LL15: _LL16:
 goto _LL18;case 9U: _LL17: _LL18:
 goto _LL1A;case 7U: _LL19: _LL1A:
 return;case 10U: _LL1B: _tmp1E3=((struct Cyc_Port_RgnVar_ct_Port_Ctype_struct*)_tmp1D8)->f1;_LL1C: {struct _fat_ptr*n=_tmp1E3;
# 1058
if(!({({int(*_tmp593)(struct Cyc_Dict_Dict d,struct _fat_ptr*k)=({int(*_tmp1E4)(struct Cyc_Dict_Dict d,struct _fat_ptr*k)=(int(*)(struct Cyc_Dict_Dict d,struct _fat_ptr*k))Cyc_Dict_member;_tmp1E4;});struct Cyc_Dict_Dict _tmp592=*cts;_tmp593(_tmp592,n);});}))
({struct Cyc_Dict_Dict _tmp597=({({struct Cyc_Dict_Dict(*_tmp596)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,int*v)=({struct Cyc_Dict_Dict(*_tmp1E5)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,int*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,int*v))Cyc_Dict_insert;_tmp1E5;});struct Cyc_Dict_Dict _tmp595=*cts;struct _fat_ptr*_tmp594=n;_tmp596(_tmp595,_tmp594,({int*_tmp1E6=_cycalloc_atomic(sizeof(*_tmp1E6));*_tmp1E6=0;_tmp1E6;}));});});*cts=_tmp597;});{
# 1058
int*p=({({int*(*_tmp599)(struct Cyc_Dict_Dict d,struct _fat_ptr*k)=({
# 1060
int*(*_tmp1E7)(struct Cyc_Dict_Dict d,struct _fat_ptr*k)=(int*(*)(struct Cyc_Dict_Dict d,struct _fat_ptr*k))Cyc_Dict_lookup;_tmp1E7;});struct Cyc_Dict_Dict _tmp598=*cts;_tmp599(_tmp598,n);});});
*p=*p + 1;
return;}}case 11U: _LL1D: _tmp1DE=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmp1D8)->f1;_tmp1DF=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmp1D8)->f2;_tmp1E0=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmp1D8)->f3;_tmp1E1=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmp1D8)->f4;_tmp1E2=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmp1D8)->f5;_LL1E: {void*t1=_tmp1DE;void*t2=_tmp1DF;void*t3=_tmp1E0;void*t4=_tmp1E1;void*zt=_tmp1E2;
# 1064
({Cyc_Port_region_counts(cts,t1);});
({Cyc_Port_region_counts(cts,t4);});
return;}case 12U: _LL1F: _tmp1DB=(void*)((struct Cyc_Port_Array_ct_Port_Ctype_struct*)_tmp1D8)->f1;_tmp1DC=(void*)((struct Cyc_Port_Array_ct_Port_Ctype_struct*)_tmp1D8)->f2;_tmp1DD=(void*)((struct Cyc_Port_Array_ct_Port_Ctype_struct*)_tmp1D8)->f3;_LL20: {void*t1=_tmp1DB;void*t2=_tmp1DC;void*zt=_tmp1DD;
# 1068
({Cyc_Port_region_counts(cts,t1);});
return;}default: _LL21: _tmp1D9=(void*)((struct Cyc_Port_Fn_ct_Port_Ctype_struct*)_tmp1D8)->f1;_tmp1DA=((struct Cyc_Port_Fn_ct_Port_Ctype_struct*)_tmp1D8)->f2;_LL22: {void*t1=_tmp1D9;struct Cyc_List_List*ts=_tmp1DA;
# 1071
({Cyc_Port_region_counts(cts,t1);});
for(0;(unsigned)ts;ts=ts->tl){({Cyc_Port_region_counts(cts,(void*)ts->hd);});}
return;}}_LL0:;}
# 1082
static void Cyc_Port_register_rgns(struct Cyc_Port_Cenv*env,struct Cyc_Dict_Dict counts,int fn_res,void*t,void*c){
# 1084
c=({Cyc_Port_compress_ct(c);});{
struct _tuple13 _stmttmp10=({struct _tuple13 _tmp4F6;_tmp4F6.f1=t,_tmp4F6.f2=c;_tmp4F6;});struct _tuple13 _tmp1E9=_stmttmp10;struct Cyc_List_List*_tmp1ED;void*_tmp1EC;struct Cyc_List_List*_tmp1EB;void*_tmp1EA;void*_tmp1EF;void*_tmp1EE;void*_tmp1F4;void*_tmp1F3;struct Cyc_Absyn_PtrLoc*_tmp1F2;void*_tmp1F1;void*_tmp1F0;switch(*((int*)_tmp1E9.f1)){case 3U: if(((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmp1E9.f2)->tag == 11U){_LL1: _tmp1F0=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp1E9.f1)->f1).elt_type;_tmp1F1=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp1E9.f1)->f1).ptr_atts).rgn;_tmp1F2=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp1E9.f1)->f1).ptr_atts).ptrloc;_tmp1F3=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmp1E9.f2)->f1;_tmp1F4=(void*)((struct Cyc_Port_Ptr_ct_Port_Ctype_struct*)_tmp1E9.f2)->f4;_LL2: {void*et=_tmp1F0;void*rt=_tmp1F1;struct Cyc_Absyn_PtrLoc*ploc=_tmp1F2;void*ec=_tmp1F3;void*rc=_tmp1F4;
# 1087
({Cyc_Port_register_rgns(env,counts,fn_res,et,ec);});{
unsigned loc=(unsigned)ploc?ploc->rgn_loc:(unsigned)0;
rc=({Cyc_Port_compress_ct(rc);});
# 1104 "port.cyc"
({struct Cyc_List_List*_tmp59D=({struct Cyc_List_List*_tmp1F8=_cycalloc(sizeof(*_tmp1F8));
({struct Cyc_Port_Edit*_tmp59C=({struct Cyc_Port_Edit*_tmp1F7=_cycalloc(sizeof(*_tmp1F7));_tmp1F7->loc=loc,({struct _fat_ptr _tmp59B=({const char*_tmp1F5="`H ";_tag_fat(_tmp1F5,sizeof(char),4U);});_tmp1F7->old_string=_tmp59B;}),({struct _fat_ptr _tmp59A=({const char*_tmp1F6="";_tag_fat(_tmp1F6,sizeof(char),1U);});_tmp1F7->new_string=_tmp59A;});_tmp1F7;});_tmp1F8->hd=_tmp59C;}),_tmp1F8->tl=(env->gcenv)->edits;_tmp1F8;});
# 1104
(env->gcenv)->edits=_tmp59D;});
# 1107
goto _LL0;}}}else{goto _LL7;}case 4U: if(((struct Cyc_Port_Array_ct_Port_Ctype_struct*)_tmp1E9.f2)->tag == 12U){_LL3: _tmp1EE=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp1E9.f1)->f1).elt_type;_tmp1EF=(void*)((struct Cyc_Port_Array_ct_Port_Ctype_struct*)_tmp1E9.f2)->f1;_LL4: {void*et=_tmp1EE;void*ec=_tmp1EF;
# 1109
({Cyc_Port_register_rgns(env,counts,fn_res,et,ec);});
goto _LL0;}}else{goto _LL7;}case 5U: if(((struct Cyc_Port_Fn_ct_Port_Ctype_struct*)_tmp1E9.f2)->tag == 15U){_LL5: _tmp1EA=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1E9.f1)->f1).ret_type;_tmp1EB=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1E9.f1)->f1).args;_tmp1EC=(void*)((struct Cyc_Port_Fn_ct_Port_Ctype_struct*)_tmp1E9.f2)->f1;_tmp1ED=((struct Cyc_Port_Fn_ct_Port_Ctype_struct*)_tmp1E9.f2)->f2;_LL6: {void*rt=_tmp1EA;struct Cyc_List_List*argst=_tmp1EB;void*rc=_tmp1EC;struct Cyc_List_List*argsc=_tmp1ED;
# 1112
({Cyc_Port_register_rgns(env,counts,1,rt,rc);});
for(0;argst != 0 && argsc != 0;(argst=argst->tl,argsc=argsc->tl)){
struct _tuple9 _stmttmp11=*((struct _tuple9*)argst->hd);void*_tmp1F9;_LLA: _tmp1F9=_stmttmp11.f3;_LLB: {void*at=_tmp1F9;
void*ac=(void*)argsc->hd;
({Cyc_Port_register_rgns(env,counts,0,at,ac);});}}
# 1118
goto _LL0;}}else{goto _LL7;}default: _LL7: _LL8:
 goto _LL0;}_LL0:;}}
# 1082 "port.cyc"
static void*Cyc_Port_gen_exp(int takeaddress,struct Cyc_Port_Cenv*env,struct Cyc_Absyn_Exp*e);
# 1126 "port.cyc"
static void Cyc_Port_gen_stmt(struct Cyc_Port_Cenv*env,struct Cyc_Absyn_Stmt*s);
static struct Cyc_Port_Cenv*Cyc_Port_gen_localdecl(struct Cyc_Port_Cenv*env,struct Cyc_Absyn_Decl*d,int takeaddress);
# 1130
static int Cyc_Port_is_char(struct Cyc_Port_Cenv*env,void*t){
void*_tmp1FB=t;struct _tuple0*_tmp1FC;switch(*((int*)_tmp1FB)){case 8U: _LL1: _tmp1FC=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp1FB)->f1;_LL2: {struct _tuple0*n=_tmp1FC;
# 1133
(*n).f1=Cyc_Absyn_Loc_n;
return({int(*_tmp1FD)(struct Cyc_Port_Cenv*env,void*t)=Cyc_Port_is_char;struct Cyc_Port_Cenv*_tmp1FE=env;void*_tmp1FF=({Cyc_Port_lookup_typedef(env,n);});_tmp1FD(_tmp1FE,_tmp1FF);});}case 0U: if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp1FB)->f1)->tag == 1U){if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp1FB)->f1)->f2 == Cyc_Absyn_Char_sz){_LL3: _LL4:
 return 1;}else{goto _LL5;}}else{goto _LL5;}default: _LL5: _LL6:
 return 0;}_LL0:;}
# 1141
static void*Cyc_Port_type_to_ctype(struct Cyc_Port_Cenv*env,void*t){
void*_tmp201=t;struct Cyc_List_List*_tmp203;void*_tmp202;unsigned _tmp207;void*_tmp206;struct Cyc_Absyn_Tqual _tmp205;void*_tmp204;struct Cyc_Absyn_PtrLoc*_tmp20E;void*_tmp20D;void*_tmp20C;void*_tmp20B;void*_tmp20A;struct Cyc_Absyn_Tqual _tmp209;void*_tmp208;struct Cyc_List_List*_tmp20F;union Cyc_Absyn_AggrInfo _tmp210;struct _tuple0*_tmp211;switch(*((int*)_tmp201)){case 8U: _LL1: _tmp211=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp201)->f1;_LL2: {struct _tuple0*n=_tmp211;
# 1144
(*n).f1=Cyc_Absyn_Loc_n;
return({Cyc_Port_lookup_typedef_ctype(env,n);});}case 0U: switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp201)->f1)){case 0U: _LL3: _LL4:
 return({Cyc_Port_void_ct();});case 1U: _LL7: _LL8:
# 1223
 goto _LLA;case 2U: _LL9: _LLA:
 return({Cyc_Port_arith_ct();});case 22U: _LLF: _tmp210=((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp201)->f1)->f1;_LL10: {union Cyc_Absyn_AggrInfo ai=_tmp210;
# 1266
union Cyc_Absyn_AggrInfo _tmp252=ai;struct Cyc_Absyn_Aggrdecl**_tmp253;struct _tuple0*_tmp254;struct _tuple0*_tmp255;if((_tmp252.UnknownAggr).tag == 1){if(((_tmp252.UnknownAggr).val).f1 == Cyc_Absyn_StructA){_LL38: _tmp255=((_tmp252.UnknownAggr).val).f2;_LL39: {struct _tuple0*n=_tmp255;
# 1268
(*n).f1=Cyc_Absyn_Loc_n;{
struct _tuple12*p=({Cyc_Port_lookup_struct_decl(env,n);});
return({Cyc_Port_known_aggr_ct(p);});}}}else{_LL3A: _tmp254=((_tmp252.UnknownAggr).val).f2;_LL3B: {struct _tuple0*n=_tmp254;
# 1272
(*n).f1=Cyc_Absyn_Loc_n;{
struct _tuple12*p=({Cyc_Port_lookup_union_decl(env,n);});
return({Cyc_Port_known_aggr_ct(p);});}}}}else{_LL3C: _tmp253=(_tmp252.KnownAggr).val;_LL3D: {struct Cyc_Absyn_Aggrdecl**adp=_tmp253;
# 1276
struct Cyc_Absyn_Aggrdecl*ad=*adp;
enum Cyc_Absyn_AggrKind _stmttmp17=ad->kind;enum Cyc_Absyn_AggrKind _tmp256=_stmttmp17;if(_tmp256 == Cyc_Absyn_StructA){_LL3F: _LL40: {
# 1279
struct _tuple12*p=({Cyc_Port_lookup_struct_decl(env,ad->name);});
return({Cyc_Port_known_aggr_ct(p);});}}else{_LL41: _LL42: {
# 1282
struct _tuple12*p=({Cyc_Port_lookup_union_decl(env,ad->name);});
return({Cyc_Port_known_aggr_ct(p);});}}_LL3E:;}}_LL37:;}case 17U: _LL11: _LL12:
# 1286
 return({Cyc_Port_arith_ct();});case 18U: _LL13: _tmp20F=((struct Cyc_Absyn_AnonEnumCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp201)->f1)->f1;_LL14: {struct Cyc_List_List*fs=_tmp20F;
# 1289
for(0;(unsigned)fs;fs=fs->tl){
(*((struct Cyc_Absyn_Enumfield*)fs->hd)->name).f1=Cyc_Absyn_Loc_n;
env=({struct Cyc_Port_Cenv*(*_tmp257)(struct Cyc_Port_Cenv*env,struct _tuple0*x,void*t,void*qual,void*ctype,unsigned loc)=Cyc_Port_add_var;struct Cyc_Port_Cenv*_tmp258=env;struct _tuple0*_tmp259=((struct Cyc_Absyn_Enumfield*)fs->hd)->name;void*_tmp25A=Cyc_Absyn_sint_type;void*_tmp25B=({Cyc_Port_const_ct();});void*_tmp25C=({Cyc_Port_arith_ct();});unsigned _tmp25D=0U;_tmp257(_tmp258,_tmp259,_tmp25A,_tmp25B,_tmp25C,_tmp25D);});}
# 1293
return({Cyc_Port_arith_ct();});}default: goto _LL15;}case 3U: _LL5: _tmp208=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp201)->f1).elt_type;_tmp209=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp201)->f1).elt_tq;_tmp20A=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp201)->f1).ptr_atts).rgn;_tmp20B=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp201)->f1).ptr_atts).nullable;_tmp20C=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp201)->f1).ptr_atts).bounds;_tmp20D=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp201)->f1).ptr_atts).zero_term;_tmp20E=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp201)->f1).ptr_atts).ptrloc;_LL6: {void*et=_tmp208;struct Cyc_Absyn_Tqual tq=_tmp209;void*r=_tmp20A;void*n=_tmp20B;void*b=_tmp20C;void*zt=_tmp20D;struct Cyc_Absyn_PtrLoc*loc=_tmp20E;
# 1148
unsigned ptr_loc=(unsigned)loc?loc->ptr_loc:(unsigned)0;
unsigned rgn_loc=(unsigned)loc?loc->rgn_loc:(unsigned)0;
unsigned zt_loc=(unsigned)loc?loc->zt_loc:(unsigned)0;
# 1154
void*cet=({Cyc_Port_type_to_ctype(env,et);});
void*new_rgn;
# 1157
{void*_tmp212=r;struct _fat_ptr*_tmp213;switch(*((int*)_tmp212)){case 0U: if(((struct Cyc_Absyn_HeapCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp212)->f1)->tag == 6U){_LL18: _LL19:
# 1159
 new_rgn=({Cyc_Port_heap_ct();});goto _LL17;}else{goto _LL1E;}case 2U: _LL1A: _tmp213=(((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp212)->f1)->name;_LL1B: {struct _fat_ptr*name=_tmp213;
# 1161
new_rgn=({Cyc_Port_rgnvar_ct(name);});goto _LL17;}case 1U: _LL1C: _LL1D:
# 1165
 if(({Cyc_Port_in_fn_arg(env);}))
new_rgn=({void*(*_tmp214)(struct _fat_ptr*n)=Cyc_Port_rgnvar_ct;struct _fat_ptr*_tmp215=({Cyc_Port_new_region_var();});_tmp214(_tmp215);});else{
# 1168
if(({Cyc_Port_in_fn_res(env);})||({Cyc_Port_in_toplevel(env);}))
new_rgn=({Cyc_Port_heap_ct();});else{
new_rgn=({Cyc_Port_var();});}}
goto _LL17;default: _LL1E: _LL1F:
# 1173
 new_rgn=({Cyc_Port_heap_ct();});goto _LL17;}_LL17:;}{
# 1175
int orig_fat=({Cyc_Tcutil_get_bounds_exp(Cyc_Absyn_fat_bound_type,b);})== 0;
int orig_zt;
{void*_stmttmp12=({Cyc_Tcutil_compress(zt);});void*_tmp216=_stmttmp12;if(((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp216)->tag == 1U){_LL21: _LL22:
 orig_zt=({Cyc_Port_is_char(env,et);});goto _LL20;}else{_LL23: _LL24:
 orig_zt=({Cyc_Tcutil_force_type2bool(0,zt);});goto _LL20;}_LL20:;}
# 1181
if((env->gcenv)->porting){
void*cqv=({Cyc_Port_var();});
# 1186
({void(*_tmp217)(struct Cyc_Port_Cenv*env,void*new_qual,void*orig_qual,unsigned loc)=Cyc_Port_register_const_cvar;struct Cyc_Port_Cenv*_tmp218=env;void*_tmp219=cqv;void*_tmp21A=tq.print_const?({Cyc_Port_const_ct();}):({Cyc_Port_notconst_ct();});unsigned _tmp21B=tq.loc;_tmp217(_tmp218,_tmp219,_tmp21A,_tmp21B);});
# 1189
if(tq.print_const)({void(*_tmp21C)(void*t1,void*t2)=Cyc_Port_unify_ct;void*_tmp21D=cqv;void*_tmp21E=({Cyc_Port_const_ct();});_tmp21C(_tmp21D,_tmp21E);});{void*cbv=({Cyc_Port_var();});
# 1198
({void(*_tmp21F)(struct Cyc_Port_Cenv*env,void*new_ptr,void*orig_ptr,unsigned loc)=Cyc_Port_register_ptr_cvars;struct Cyc_Port_Cenv*_tmp220=env;void*_tmp221=cbv;void*_tmp222=orig_fat?({Cyc_Port_fat_ct();}):({Cyc_Port_thin_ct();});unsigned _tmp223=ptr_loc;_tmp21F(_tmp220,_tmp221,_tmp222,_tmp223);});
# 1200
if(orig_fat)({void(*_tmp224)(void*t1,void*t2)=Cyc_Port_unify_ct;void*_tmp225=cbv;void*_tmp226=({Cyc_Port_fat_ct();});_tmp224(_tmp225,_tmp226);});{void*czv=({Cyc_Port_var();});
# 1204
({void(*_tmp227)(struct Cyc_Port_Cenv*env,void*new_zt,void*orig_zt,unsigned loc)=Cyc_Port_register_zeroterm_cvars;struct Cyc_Port_Cenv*_tmp228=env;void*_tmp229=czv;void*_tmp22A=orig_zt?({Cyc_Port_zterm_ct();}):({Cyc_Port_nozterm_ct();});unsigned _tmp22B=zt_loc;_tmp227(_tmp228,_tmp229,_tmp22A,_tmp22B);});
# 1206
{void*_stmttmp13=({Cyc_Tcutil_compress(zt);});void*_tmp22C=_stmttmp13;if(((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp22C)->tag == 1U){_LL26: _LL27:
 goto _LL25;}else{_LL28: _LL29:
# 1209
({void(*_tmp22D)(void*t1,void*t2)=Cyc_Port_unify_ct;void*_tmp22E=czv;void*_tmp22F=({Cyc_Tcutil_force_type2bool(0,zt);})?({Cyc_Port_zterm_ct();}):({Cyc_Port_nozterm_ct();});_tmp22D(_tmp22E,_tmp22F);});
goto _LL25;}_LL25:;}
# 1215
return({Cyc_Port_ptr_ct(cet,cqv,cbv,new_rgn,czv);});}}}else{
# 1219
return({void*(*_tmp230)(void*elt,void*qual,void*ptr_kind,void*r,void*zt)=Cyc_Port_ptr_ct;void*_tmp231=cet;void*_tmp232=tq.print_const?({Cyc_Port_const_ct();}):({Cyc_Port_notconst_ct();});void*_tmp233=
orig_fat?({Cyc_Port_fat_ct();}):({Cyc_Port_thin_ct();});void*_tmp234=new_rgn;void*_tmp235=
orig_zt?({Cyc_Port_zterm_ct();}):({Cyc_Port_nozterm_ct();});_tmp230(_tmp231,_tmp232,_tmp233,_tmp234,_tmp235);});}}}case 4U: _LLB: _tmp204=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp201)->f1).elt_type;_tmp205=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp201)->f1).tq;_tmp206=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp201)->f1).zero_term;_tmp207=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp201)->f1).zt_loc;_LLC: {void*et=_tmp204;struct Cyc_Absyn_Tqual tq=_tmp205;void*zt=_tmp206;unsigned ztloc=_tmp207;
# 1227
void*cet=({Cyc_Port_type_to_ctype(env,et);});
int orig_zt;
{void*_stmttmp14=({Cyc_Tcutil_compress(zt);});void*_tmp236=_stmttmp14;if(((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp236)->tag == 1U){_LL2B: _LL2C:
 orig_zt=0;goto _LL2A;}else{_LL2D: _LL2E:
 orig_zt=({Cyc_Tcutil_force_type2bool(0,zt);});goto _LL2A;}_LL2A:;}
# 1233
if((env->gcenv)->porting){
void*cqv=({Cyc_Port_var();});
({void(*_tmp237)(struct Cyc_Port_Cenv*env,void*new_qual,void*orig_qual,unsigned loc)=Cyc_Port_register_const_cvar;struct Cyc_Port_Cenv*_tmp238=env;void*_tmp239=cqv;void*_tmp23A=tq.print_const?({Cyc_Port_const_ct();}):({Cyc_Port_notconst_ct();});unsigned _tmp23B=tq.loc;_tmp237(_tmp238,_tmp239,_tmp23A,_tmp23B);});{
# 1242
void*czv=({Cyc_Port_var();});
({void(*_tmp23C)(struct Cyc_Port_Cenv*env,void*new_zt,void*orig_zt,unsigned loc)=Cyc_Port_register_zeroterm_cvars;struct Cyc_Port_Cenv*_tmp23D=env;void*_tmp23E=czv;void*_tmp23F=orig_zt?({Cyc_Port_zterm_ct();}):({Cyc_Port_nozterm_ct();});unsigned _tmp240=ztloc;_tmp23C(_tmp23D,_tmp23E,_tmp23F,_tmp240);});
# 1245
{void*_stmttmp15=({Cyc_Tcutil_compress(zt);});void*_tmp241=_stmttmp15;if(((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp241)->tag == 1U){_LL30: _LL31:
 goto _LL2F;}else{_LL32: _LL33:
# 1248
({void(*_tmp242)(void*t1,void*t2)=Cyc_Port_unify_ct;void*_tmp243=czv;void*_tmp244=({Cyc_Tcutil_force_type2bool(0,zt);})?({Cyc_Port_zterm_ct();}):({Cyc_Port_nozterm_ct();});_tmp242(_tmp243,_tmp244);});
goto _LL2F;}_LL2F:;}
# 1251
return({Cyc_Port_array_ct(cet,cqv,czv);});}}else{
# 1253
return({void*(*_tmp245)(void*elt,void*qual,void*zt)=Cyc_Port_array_ct;void*_tmp246=cet;void*_tmp247=tq.print_const?({Cyc_Port_const_ct();}):({Cyc_Port_notconst_ct();});void*_tmp248=
orig_zt?({Cyc_Port_zterm_ct();}):({Cyc_Port_nozterm_ct();});_tmp245(_tmp246,_tmp247,_tmp248);});}}case 5U: _LLD: _tmp202=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp201)->f1).ret_type;_tmp203=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp201)->f1).args;_LLE: {void*rt=_tmp202;struct Cyc_List_List*args=_tmp203;
# 1257
void*crt=({void*(*_tmp24F)(struct Cyc_Port_Cenv*env,void*t)=Cyc_Port_type_to_ctype;struct Cyc_Port_Cenv*_tmp250=({Cyc_Port_set_cpos(env,Cyc_Port_FnRes_pos);});void*_tmp251=rt;_tmp24F(_tmp250,_tmp251);});
struct Cyc_Port_Cenv*env_arg=({Cyc_Port_set_cpos(env,Cyc_Port_FnArg_pos);});
struct Cyc_List_List*cargs=0;
for(0;args != 0;args=args->tl){
struct _tuple9 _stmttmp16=*((struct _tuple9*)args->hd);void*_tmp24A;struct Cyc_Absyn_Tqual _tmp249;_LL35: _tmp249=_stmttmp16.f2;_tmp24A=_stmttmp16.f3;_LL36: {struct Cyc_Absyn_Tqual tq=_tmp249;void*t=_tmp24A;
cargs=({struct Cyc_List_List*_tmp24B=_cycalloc(sizeof(*_tmp24B));({void*_tmp59E=({Cyc_Port_type_to_ctype(env_arg,t);});_tmp24B->hd=_tmp59E;}),_tmp24B->tl=cargs;_tmp24B;});}}
# 1264
return({void*(*_tmp24C)(void*return_type,struct Cyc_List_List*args)=Cyc_Port_fn_ct;void*_tmp24D=crt;struct Cyc_List_List*_tmp24E=({Cyc_List_imp_rev(cargs);});_tmp24C(_tmp24D,_tmp24E);});}default: _LL15: _LL16:
# 1294
 return({Cyc_Port_arith_ct();});}_LL0:;}
# 1299
static void*Cyc_Port_gen_primop(struct Cyc_Port_Cenv*env,enum Cyc_Absyn_Primop p,struct Cyc_List_List*args){
void*arg1=({Cyc_Port_compress_ct((void*)((struct Cyc_List_List*)_check_null(args))->hd);});
struct Cyc_List_List*arg2s=args->tl;
enum Cyc_Absyn_Primop _tmp25F=p;switch(_tmp25F){case Cyc_Absyn_Plus: _LL1: _LL2: {
# 1307
void*arg2=({Cyc_Port_compress_ct((void*)((struct Cyc_List_List*)_check_null(arg2s))->hd);});
if(({int(*_tmp260)(void*t1,void*t2)=Cyc_Port_leq;void*_tmp261=arg1;void*_tmp262=({void*(*_tmp263)(void*elt,void*qual,void*ptr_kind,void*r,void*zt)=Cyc_Port_ptr_ct;void*_tmp264=({Cyc_Port_var();});void*_tmp265=({Cyc_Port_var();});void*_tmp266=({Cyc_Port_fat_ct();});void*_tmp267=({Cyc_Port_var();});void*_tmp268=({Cyc_Port_var();});_tmp263(_tmp264,_tmp265,_tmp266,_tmp267,_tmp268);});_tmp260(_tmp261,_tmp262);})){
({int(*_tmp269)(void*t1,void*t2)=Cyc_Port_leq;void*_tmp26A=arg2;void*_tmp26B=({Cyc_Port_arith_ct();});_tmp269(_tmp26A,_tmp26B);});
return arg1;}else{
if(({int(*_tmp26C)(void*t1,void*t2)=Cyc_Port_leq;void*_tmp26D=arg2;void*_tmp26E=({void*(*_tmp26F)(void*elt,void*qual,void*ptr_kind,void*r,void*zt)=Cyc_Port_ptr_ct;void*_tmp270=({Cyc_Port_var();});void*_tmp271=({Cyc_Port_var();});void*_tmp272=({Cyc_Port_fat_ct();});void*_tmp273=({Cyc_Port_var();});void*_tmp274=({Cyc_Port_var();});_tmp26F(_tmp270,_tmp271,_tmp272,_tmp273,_tmp274);});_tmp26C(_tmp26D,_tmp26E);})){
({int(*_tmp275)(void*t1,void*t2)=Cyc_Port_leq;void*_tmp276=arg1;void*_tmp277=({Cyc_Port_arith_ct();});_tmp275(_tmp276,_tmp277);});
return arg2;}else{
# 1315
({int(*_tmp278)(void*t1,void*t2)=Cyc_Port_leq;void*_tmp279=arg1;void*_tmp27A=({Cyc_Port_arith_ct();});_tmp278(_tmp279,_tmp27A);});
({int(*_tmp27B)(void*t1,void*t2)=Cyc_Port_leq;void*_tmp27C=arg2;void*_tmp27D=({Cyc_Port_arith_ct();});_tmp27B(_tmp27C,_tmp27D);});
return({Cyc_Port_arith_ct();});}}}case Cyc_Absyn_Minus: _LL3: _LL4:
# 1324
 if(arg2s == 0){
# 1326
({int(*_tmp27E)(void*t1,void*t2)=Cyc_Port_leq;void*_tmp27F=arg1;void*_tmp280=({Cyc_Port_arith_ct();});_tmp27E(_tmp27F,_tmp280);});
return({Cyc_Port_arith_ct();});}else{
# 1329
void*arg2=({Cyc_Port_compress_ct((void*)arg2s->hd);});
if(({int(*_tmp281)(void*t1,void*t2)=Cyc_Port_leq;void*_tmp282=arg1;void*_tmp283=({void*(*_tmp284)(void*elt,void*qual,void*ptr_kind,void*r,void*zt)=Cyc_Port_ptr_ct;void*_tmp285=({Cyc_Port_var();});void*_tmp286=({Cyc_Port_var();});void*_tmp287=({Cyc_Port_fat_ct();});void*_tmp288=({Cyc_Port_var();});void*_tmp289=({Cyc_Port_var();});_tmp284(_tmp285,_tmp286,_tmp287,_tmp288,_tmp289);});_tmp281(_tmp282,_tmp283);})){
if(({int(*_tmp28A)(void*t1,void*t2)=Cyc_Port_leq;void*_tmp28B=arg2;void*_tmp28C=({void*(*_tmp28D)(void*elt,void*qual,void*ptr_kind,void*r,void*zt)=Cyc_Port_ptr_ct;void*_tmp28E=({Cyc_Port_var();});void*_tmp28F=({Cyc_Port_var();});void*_tmp290=({Cyc_Port_fat_ct();});void*_tmp291=({Cyc_Port_var();});void*_tmp292=({Cyc_Port_var();});_tmp28D(_tmp28E,_tmp28F,_tmp290,_tmp291,_tmp292);});_tmp28A(_tmp28B,_tmp28C);}))
return({Cyc_Port_arith_ct();});else{
# 1334
({int(*_tmp293)(void*t1,void*t2)=Cyc_Port_leq;void*_tmp294=arg2;void*_tmp295=({Cyc_Port_arith_ct();});_tmp293(_tmp294,_tmp295);});
return arg1;}}else{
# 1338
({int(*_tmp296)(void*t1,void*t2)=Cyc_Port_leq;void*_tmp297=arg1;void*_tmp298=({Cyc_Port_arith_ct();});_tmp296(_tmp297,_tmp298);});
({int(*_tmp299)(void*t1,void*t2)=Cyc_Port_leq;void*_tmp29A=arg1;void*_tmp29B=({Cyc_Port_arith_ct();});_tmp299(_tmp29A,_tmp29B);});
return({Cyc_Port_arith_ct();});}}case Cyc_Absyn_Times: _LL5: _LL6:
# 1343
 goto _LL8;case Cyc_Absyn_Div: _LL7: _LL8: goto _LLA;case Cyc_Absyn_Mod: _LL9: _LLA: goto _LLC;case Cyc_Absyn_Eq: _LLB: _LLC: goto _LLE;case Cyc_Absyn_Neq: _LLD: _LLE: goto _LL10;case Cyc_Absyn_Lt: _LLF: _LL10: goto _LL12;case Cyc_Absyn_Gt: _LL11: _LL12: goto _LL14;case Cyc_Absyn_Lte: _LL13: _LL14:
 goto _LL16;case Cyc_Absyn_Gte: _LL15: _LL16: goto _LL18;case Cyc_Absyn_Bitand: _LL17: _LL18: goto _LL1A;case Cyc_Absyn_Bitor: _LL19: _LL1A: goto _LL1C;case Cyc_Absyn_Bitxor: _LL1B: _LL1C: goto _LL1E;case Cyc_Absyn_Bitlshift: _LL1D: _LL1E: goto _LL20;case Cyc_Absyn_Bitlrshift: _LL1F: _LL20:
# 1346
({int(*_tmp29C)(void*t1,void*t2)=Cyc_Port_leq;void*_tmp29D=arg1;void*_tmp29E=({Cyc_Port_arith_ct();});_tmp29C(_tmp29D,_tmp29E);});
({int(*_tmp29F)(void*t1,void*t2)=Cyc_Port_leq;void*_tmp2A0=(void*)((struct Cyc_List_List*)_check_null(arg2s))->hd;void*_tmp2A1=({Cyc_Port_arith_ct();});_tmp29F(_tmp2A0,_tmp2A1);});
return({Cyc_Port_arith_ct();});case Cyc_Absyn_Not: _LL21: _LL22:
 goto _LL24;case Cyc_Absyn_Bitnot: _LL23: _LL24:
# 1351
({int(*_tmp2A2)(void*t1,void*t2)=Cyc_Port_leq;void*_tmp2A3=arg1;void*_tmp2A4=({Cyc_Port_arith_ct();});_tmp2A2(_tmp2A3,_tmp2A4);});
return({Cyc_Port_arith_ct();});default: _LL25: _LL26:
({void*_tmp2A5=0U;({int(*_tmp5A0)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp2A7)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp2A7;});struct _fat_ptr _tmp59F=({const char*_tmp2A6="Port: unknown primop";_tag_fat(_tmp2A6,sizeof(char),21U);});_tmp5A0(_tmp59F,_tag_fat(_tmp2A5,sizeof(void*),0U));});});}_LL0:;}
# 1358
static struct _tuple13 Cyc_Port_gen_lhs(struct Cyc_Port_Cenv*env,struct Cyc_Absyn_Exp*e,int takeaddress){
void*_stmttmp18=e->r;void*_tmp2A9=_stmttmp18;struct _fat_ptr*_tmp2AB;struct Cyc_Absyn_Exp*_tmp2AA;struct _fat_ptr*_tmp2AD;struct Cyc_Absyn_Exp*_tmp2AC;struct Cyc_Absyn_Exp*_tmp2AE;struct Cyc_Absyn_Exp*_tmp2B0;struct Cyc_Absyn_Exp*_tmp2AF;void*_tmp2B1;switch(*((int*)_tmp2A9)){case 1U: _LL1: _tmp2B1=(void*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp2A9)->f1;_LL2: {void*b=_tmp2B1;
# 1361
struct _tuple0*x=({Cyc_Absyn_binding2qvar(b);});
(*x).f1=Cyc_Absyn_Loc_n;{
struct _tuple15 _stmttmp19=({Cyc_Port_lookup_var(env,x);});unsigned _tmp2B3;struct _tuple13 _tmp2B2;_LLE: _tmp2B2=_stmttmp19.f1;_tmp2B3=_stmttmp19.f2;_LLF: {struct _tuple13 p=_tmp2B2;unsigned loc=_tmp2B3;
({Cyc_Port_register_localvar_usage(env,loc,e->loc,takeaddress);});
return p;}}}case 23U: _LL3: _tmp2AF=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp2A9)->f1;_tmp2B0=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp2A9)->f2;_LL4: {struct Cyc_Absyn_Exp*e1=_tmp2AF;struct Cyc_Absyn_Exp*e2=_tmp2B0;
# 1367
void*v=({Cyc_Port_var();});
void*q=({Cyc_Port_var();});
void*t1=({Cyc_Port_gen_exp(0,env,e1);});
({int(*_tmp2B4)(void*t1,void*t2)=Cyc_Port_leq;void*_tmp2B5=({Cyc_Port_gen_exp(0,env,e2);});void*_tmp2B6=({Cyc_Port_arith_ct();});_tmp2B4(_tmp2B5,_tmp2B6);});{
void*p=({void*(*_tmp2B7)(void*elt,void*qual,void*ptr_kind,void*r,void*zt)=Cyc_Port_ptr_ct;void*_tmp2B8=v;void*_tmp2B9=q;void*_tmp2BA=({Cyc_Port_fat_ct();});void*_tmp2BB=({Cyc_Port_var();});void*_tmp2BC=({Cyc_Port_var();});_tmp2B7(_tmp2B8,_tmp2B9,_tmp2BA,_tmp2BB,_tmp2BC);});
({Cyc_Port_leq(t1,p);});
return({struct _tuple13 _tmp4F7;_tmp4F7.f1=q,_tmp4F7.f2=v;_tmp4F7;});}}case 20U: _LL5: _tmp2AE=((struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_tmp2A9)->f1;_LL6: {struct Cyc_Absyn_Exp*e1=_tmp2AE;
# 1375
void*v=({Cyc_Port_var();});
void*q=({Cyc_Port_var();});
void*p=({void*(*_tmp2C0)(void*elt,void*qual,void*ptr_kind,void*r,void*zt)=Cyc_Port_ptr_ct;void*_tmp2C1=v;void*_tmp2C2=q;void*_tmp2C3=({Cyc_Port_var();});void*_tmp2C4=({Cyc_Port_var();});void*_tmp2C5=({Cyc_Port_var();});_tmp2C0(_tmp2C1,_tmp2C2,_tmp2C3,_tmp2C4,_tmp2C5);});
({int(*_tmp2BD)(void*t1,void*t2)=Cyc_Port_leq;void*_tmp2BE=({Cyc_Port_gen_exp(0,env,e1);});void*_tmp2BF=p;_tmp2BD(_tmp2BE,_tmp2BF);});
return({struct _tuple13 _tmp4F8;_tmp4F8.f1=q,_tmp4F8.f2=v;_tmp4F8;});}case 21U: _LL7: _tmp2AC=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp2A9)->f1;_tmp2AD=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp2A9)->f2;_LL8: {struct Cyc_Absyn_Exp*e1=_tmp2AC;struct _fat_ptr*f=_tmp2AD;
# 1381
void*v=({Cyc_Port_var();});
void*q=({Cyc_Port_var();});
void*t1=({Cyc_Port_gen_exp(takeaddress,env,e1);});
({int(*_tmp2C6)(void*t1,void*t2)=Cyc_Port_leq;void*_tmp2C7=t1;void*_tmp2C8=({void*(*_tmp2C9)(struct Cyc_List_List*fs)=Cyc_Port_unknown_aggr_ct;struct Cyc_List_List*_tmp2CA=({struct Cyc_Port_Cfield*_tmp2CB[1U];({struct Cyc_Port_Cfield*_tmp5A1=({struct Cyc_Port_Cfield*_tmp2CC=_cycalloc(sizeof(*_tmp2CC));_tmp2CC->qual=q,_tmp2CC->f=f,_tmp2CC->type=v;_tmp2CC;});_tmp2CB[0]=_tmp5A1;});Cyc_List_list(_tag_fat(_tmp2CB,sizeof(struct Cyc_Port_Cfield*),1U));});_tmp2C9(_tmp2CA);});_tmp2C6(_tmp2C7,_tmp2C8);});
return({struct _tuple13 _tmp4F9;_tmp4F9.f1=q,_tmp4F9.f2=v;_tmp4F9;});}case 22U: _LL9: _tmp2AA=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp2A9)->f1;_tmp2AB=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp2A9)->f2;_LLA: {struct Cyc_Absyn_Exp*e1=_tmp2AA;struct _fat_ptr*f=_tmp2AB;
# 1387
void*v=({Cyc_Port_var();});
void*q=({Cyc_Port_var();});
void*t1=({Cyc_Port_gen_exp(0,env,e1);});
({int(*_tmp2CD)(void*t1,void*t2)=Cyc_Port_leq;void*_tmp2CE=t1;void*_tmp2CF=({void*(*_tmp2D0)(void*elt,void*qual,void*ptr_kind,void*r,void*zt)=Cyc_Port_ptr_ct;void*_tmp2D1=({void*(*_tmp2D2)(struct Cyc_List_List*fs)=Cyc_Port_unknown_aggr_ct;struct Cyc_List_List*_tmp2D3=({struct Cyc_Port_Cfield*_tmp2D4[1U];({struct Cyc_Port_Cfield*_tmp5A2=({struct Cyc_Port_Cfield*_tmp2D5=_cycalloc(sizeof(*_tmp2D5));_tmp2D5->qual=q,_tmp2D5->f=f,_tmp2D5->type=v;_tmp2D5;});_tmp2D4[0]=_tmp5A2;});Cyc_List_list(_tag_fat(_tmp2D4,sizeof(struct Cyc_Port_Cfield*),1U));});_tmp2D2(_tmp2D3);});void*_tmp2D6=({Cyc_Port_var();});void*_tmp2D7=({Cyc_Port_var();});void*_tmp2D8=({Cyc_Port_var();});void*_tmp2D9=({Cyc_Port_var();});_tmp2D0(_tmp2D1,_tmp2D6,_tmp2D7,_tmp2D8,_tmp2D9);});_tmp2CD(_tmp2CE,_tmp2CF);});
# 1392
return({struct _tuple13 _tmp4FA;_tmp4FA.f1=q,_tmp4FA.f2=v;_tmp4FA;});}default: _LLB: _LLC:
({void*_tmp2DA=0U;({int(*_tmp5A6)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp2DE)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp2DE;});struct _fat_ptr _tmp5A5=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp2DD=({struct Cyc_String_pa_PrintArg_struct _tmp4FB;_tmp4FB.tag=0U,({struct _fat_ptr _tmp5A3=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_exp2string(e);}));_tmp4FB.f1=_tmp5A3;});_tmp4FB;});void*_tmp2DB[1U];_tmp2DB[0]=& _tmp2DD;({struct _fat_ptr _tmp5A4=({const char*_tmp2DC="gen_lhs: %s";_tag_fat(_tmp2DC,sizeof(char),12U);});Cyc_aprintf(_tmp5A4,_tag_fat(_tmp2DB,sizeof(void*),1U));});});_tmp5A6(_tmp5A5,_tag_fat(_tmp2DA,sizeof(void*),0U));});});}_LL0:;}
# 1398
static void*Cyc_Port_gen_exp_false(struct Cyc_Port_Cenv*env,struct Cyc_Absyn_Exp*e){
return({Cyc_Port_gen_exp(0,env,e);});}
# 1401
static void*Cyc_Port_gen_exp(int takeaddress,struct Cyc_Port_Cenv*env,struct Cyc_Absyn_Exp*e){
void*_stmttmp1A=e->r;void*_tmp2E1=_stmttmp1A;struct Cyc_Absyn_Stmt*_tmp2E2;struct Cyc_Absyn_Exp*_tmp2E4;struct Cyc_Absyn_Exp*_tmp2E3;struct Cyc_Absyn_Exp*_tmp2E6;void**_tmp2E5;struct _fat_ptr*_tmp2E8;struct Cyc_Absyn_Exp*_tmp2E7;struct _fat_ptr*_tmp2EA;struct Cyc_Absyn_Exp*_tmp2E9;struct Cyc_Absyn_Exp*_tmp2EB;struct Cyc_Absyn_Exp*_tmp2ED;struct Cyc_Absyn_Exp*_tmp2EC;struct Cyc_Absyn_Exp*_tmp2EE;struct Cyc_Absyn_Exp*_tmp2F0;void*_tmp2EF;struct Cyc_Absyn_Exp*_tmp2F1;struct Cyc_Absyn_Exp*_tmp2F2;struct Cyc_List_List*_tmp2F4;struct Cyc_Absyn_Exp*_tmp2F3;struct Cyc_Absyn_Exp*_tmp2F6;struct Cyc_Absyn_Exp*_tmp2F5;struct Cyc_Absyn_Exp*_tmp2F8;struct Cyc_Absyn_Exp*_tmp2F7;struct Cyc_Absyn_Exp*_tmp2FA;struct Cyc_Absyn_Exp*_tmp2F9;struct Cyc_Absyn_Exp*_tmp2FD;struct Cyc_Absyn_Exp*_tmp2FC;struct Cyc_Absyn_Exp*_tmp2FB;enum Cyc_Absyn_Incrementor _tmp2FF;struct Cyc_Absyn_Exp*_tmp2FE;struct Cyc_Absyn_Exp*_tmp302;struct Cyc_Core_Opt*_tmp301;struct Cyc_Absyn_Exp*_tmp300;struct Cyc_List_List*_tmp304;enum Cyc_Absyn_Primop _tmp303;void*_tmp305;switch(*((int*)_tmp2E1)){case 0U: switch(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp2E1)->f1).Wstring_c).tag){case 2U: _LL1: _LL2:
 goto _LL4;case 3U: _LL3: _LL4:
 goto _LL6;case 4U: _LL5: _LL6:
 goto _LL8;case 6U: _LL7: _LL8:
 goto _LLA;case 7U: _LL13: _LL14:
# 1412
 return({Cyc_Port_arith_ct();});case 5U: if((((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp2E1)->f1).Int_c).val).f2 == 0){_LL15: _LL16:
 return({Cyc_Port_zero_ct();});}else{_LL17: _LL18:
 return({Cyc_Port_arith_ct();});}case 8U: _LL19: _LL1A:
# 1416
 return({void*(*_tmp306)(void*elt,void*qual,void*ptr_kind,void*r,void*zt)=Cyc_Port_ptr_ct;void*_tmp307=({Cyc_Port_arith_ct();});void*_tmp308=({Cyc_Port_const_ct();});void*_tmp309=({Cyc_Port_fat_ct();});void*_tmp30A=({Cyc_Port_heap_ct();});void*_tmp30B=({Cyc_Port_zterm_ct();});_tmp306(_tmp307,_tmp308,_tmp309,_tmp30A,_tmp30B);});case 9U: _LL1B: _LL1C:
# 1418
 return({void*(*_tmp30C)(void*elt,void*qual,void*ptr_kind,void*r,void*zt)=Cyc_Port_ptr_ct;void*_tmp30D=({Cyc_Port_arith_ct();});void*_tmp30E=({Cyc_Port_const_ct();});void*_tmp30F=({Cyc_Port_fat_ct();});void*_tmp310=({Cyc_Port_heap_ct();});void*_tmp311=({Cyc_Port_zterm_ct();});_tmp30C(_tmp30D,_tmp30E,_tmp30F,_tmp310,_tmp311);});default: _LL1D: _LL1E:
 return({Cyc_Port_zero_ct();});}case 17U: _LL9: _LLA:
# 1407
 goto _LLC;case 18U: _LLB: _LLC:
 goto _LLE;case 19U: _LLD: _LLE:
 goto _LL10;case 32U: _LLF: _LL10:
 goto _LL12;case 33U: _LL11: _LL12:
 goto _LL14;case 1U: _LL1F: _tmp305=(void*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp2E1)->f1;_LL20: {void*b=_tmp305;
# 1421
struct _tuple0*x=({Cyc_Absyn_binding2qvar(b);});
(*x).f1=Cyc_Absyn_Loc_n;{
struct _tuple15 _stmttmp1B=({Cyc_Port_lookup_var(env,x);});unsigned _tmp313;struct _tuple13 _tmp312;_LL6A: _tmp312=_stmttmp1B.f1;_tmp313=_stmttmp1B.f2;_LL6B: {struct _tuple13 p=_tmp312;unsigned loc=_tmp313;
void*_tmp314;_LL6D: _tmp314=p.f2;_LL6E: {void*t=_tmp314;
if(({Cyc_Port_isfn(env,x);})){
t=({Cyc_Port_instantiate(t);});
return({void*(*_tmp315)(void*elt,void*qual,void*ptr_kind,void*r,void*zt)=Cyc_Port_ptr_ct;void*_tmp316=t;void*_tmp317=({Cyc_Port_var();});void*_tmp318=({Cyc_Port_var();});void*_tmp319=({Cyc_Port_heap_ct();});void*_tmp31A=({Cyc_Port_nozterm_ct();});_tmp315(_tmp316,_tmp317,_tmp318,_tmp319,_tmp31A);});}else{
# 1429
if(({Cyc_Port_isarray(env,x);})){
void*elt_type=({Cyc_Port_var();});
void*qual=({Cyc_Port_var();});
void*zt=({Cyc_Port_var();});
void*array_type=({Cyc_Port_array_ct(elt_type,qual,zt);});
({Cyc_Port_unifies(t,array_type);});{
void*ptr_type=({void*(*_tmp31B)(void*elt,void*qual,void*ptr_kind,void*r,void*zt)=Cyc_Port_ptr_ct;void*_tmp31C=elt_type;void*_tmp31D=qual;void*_tmp31E=({Cyc_Port_fat_ct();});void*_tmp31F=({Cyc_Port_var();});void*_tmp320=zt;_tmp31B(_tmp31C,_tmp31D,_tmp31E,_tmp31F,_tmp320);});
return ptr_type;}}else{
# 1438
({Cyc_Port_register_localvar_usage(env,loc,e->loc,takeaddress);});
return t;}}}}}}case 3U: _LL21: _tmp303=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp2E1)->f1;_tmp304=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp2E1)->f2;_LL22: {enum Cyc_Absyn_Primop p=_tmp303;struct Cyc_List_List*es=_tmp304;
# 1441
return({void*(*_tmp321)(struct Cyc_Port_Cenv*env,enum Cyc_Absyn_Primop p,struct Cyc_List_List*args)=Cyc_Port_gen_primop;struct Cyc_Port_Cenv*_tmp322=env;enum Cyc_Absyn_Primop _tmp323=p;struct Cyc_List_List*_tmp324=({({struct Cyc_List_List*(*_tmp5A8)(void*(*f)(struct Cyc_Port_Cenv*,struct Cyc_Absyn_Exp*),struct Cyc_Port_Cenv*env,struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp325)(void*(*f)(struct Cyc_Port_Cenv*,struct Cyc_Absyn_Exp*),struct Cyc_Port_Cenv*env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(void*(*f)(struct Cyc_Port_Cenv*,struct Cyc_Absyn_Exp*),struct Cyc_Port_Cenv*env,struct Cyc_List_List*x))Cyc_List_map_c;_tmp325;});struct Cyc_Port_Cenv*_tmp5A7=env;_tmp5A8(Cyc_Port_gen_exp_false,_tmp5A7,es);});});_tmp321(_tmp322,_tmp323,_tmp324);});}case 4U: _LL23: _tmp300=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp2E1)->f1;_tmp301=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp2E1)->f2;_tmp302=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp2E1)->f3;_LL24: {struct Cyc_Absyn_Exp*e1=_tmp300;struct Cyc_Core_Opt*popt=_tmp301;struct Cyc_Absyn_Exp*e2=_tmp302;
# 1443
struct _tuple13 _stmttmp1C=({Cyc_Port_gen_lhs(env,e1,0);});void*_tmp327;void*_tmp326;_LL70: _tmp326=_stmttmp1C.f1;_tmp327=_stmttmp1C.f2;_LL71: {void*q=_tmp326;void*t1=_tmp327;
({int(*_tmp328)(void*t1,void*t2)=Cyc_Port_unifies;void*_tmp329=q;void*_tmp32A=({Cyc_Port_notconst_ct();});_tmp328(_tmp329,_tmp32A);});{
void*t2=({Cyc_Port_gen_exp(0,env,e2);});
if((unsigned)popt){
struct Cyc_List_List x2=({struct Cyc_List_List _tmp4FD;_tmp4FD.hd=t2,_tmp4FD.tl=0;_tmp4FD;});
struct Cyc_List_List x1=({struct Cyc_List_List _tmp4FC;_tmp4FC.hd=t1,_tmp4FC.tl=& x2;_tmp4FC;});
void*t3=({Cyc_Port_gen_primop(env,(enum Cyc_Absyn_Primop)popt->v,& x1);});
({Cyc_Port_leq(t3,t1);});
return t1;}else{
# 1453
({Cyc_Port_leq(t2,t1);});
return t1;}}}}case 5U: _LL25: _tmp2FE=((struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*)_tmp2E1)->f1;_tmp2FF=((struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*)_tmp2E1)->f2;_LL26: {struct Cyc_Absyn_Exp*e1=_tmp2FE;enum Cyc_Absyn_Incrementor inc=_tmp2FF;
# 1457
struct _tuple13 _stmttmp1D=({Cyc_Port_gen_lhs(env,e1,0);});void*_tmp32C;void*_tmp32B;_LL73: _tmp32B=_stmttmp1D.f1;_tmp32C=_stmttmp1D.f2;_LL74: {void*q=_tmp32B;void*t=_tmp32C;
({int(*_tmp32D)(void*t1,void*t2)=Cyc_Port_unifies;void*_tmp32E=q;void*_tmp32F=({Cyc_Port_notconst_ct();});_tmp32D(_tmp32E,_tmp32F);});
# 1460
({int(*_tmp330)(void*t1,void*t2)=Cyc_Port_leq;void*_tmp331=t;void*_tmp332=({void*(*_tmp333)(void*elt,void*qual,void*ptr_kind,void*r,void*zt)=Cyc_Port_ptr_ct;void*_tmp334=({Cyc_Port_var();});void*_tmp335=({Cyc_Port_var();});void*_tmp336=({Cyc_Port_fat_ct();});void*_tmp337=({Cyc_Port_var();});void*_tmp338=({Cyc_Port_var();});_tmp333(_tmp334,_tmp335,_tmp336,_tmp337,_tmp338);});_tmp330(_tmp331,_tmp332);});
({int(*_tmp339)(void*t1,void*t2)=Cyc_Port_leq;void*_tmp33A=t;void*_tmp33B=({Cyc_Port_arith_ct();});_tmp339(_tmp33A,_tmp33B);});
return t;}}case 6U: _LL27: _tmp2FB=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp2E1)->f1;_tmp2FC=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp2E1)->f2;_tmp2FD=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp2E1)->f3;_LL28: {struct Cyc_Absyn_Exp*e1=_tmp2FB;struct Cyc_Absyn_Exp*e2=_tmp2FC;struct Cyc_Absyn_Exp*e3=_tmp2FD;
# 1464
void*v=({Cyc_Port_var();});
({int(*_tmp33C)(void*t1,void*t2)=Cyc_Port_leq;void*_tmp33D=({Cyc_Port_gen_exp(0,env,e1);});void*_tmp33E=({Cyc_Port_arith_ct();});_tmp33C(_tmp33D,_tmp33E);});
({int(*_tmp33F)(void*t1,void*t2)=Cyc_Port_leq;void*_tmp340=({Cyc_Port_gen_exp(0,env,e2);});void*_tmp341=v;_tmp33F(_tmp340,_tmp341);});
({int(*_tmp342)(void*t1,void*t2)=Cyc_Port_leq;void*_tmp343=({Cyc_Port_gen_exp(0,env,e3);});void*_tmp344=v;_tmp342(_tmp343,_tmp344);});
return v;}case 7U: _LL29: _tmp2F9=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp2E1)->f1;_tmp2FA=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp2E1)->f2;_LL2A: {struct Cyc_Absyn_Exp*e1=_tmp2F9;struct Cyc_Absyn_Exp*e2=_tmp2FA;
_tmp2F7=e1;_tmp2F8=e2;goto _LL2C;}case 8U: _LL2B: _tmp2F7=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp2E1)->f1;_tmp2F8=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp2E1)->f2;_LL2C: {struct Cyc_Absyn_Exp*e1=_tmp2F7;struct Cyc_Absyn_Exp*e2=_tmp2F8;
# 1471
({int(*_tmp345)(void*t1,void*t2)=Cyc_Port_leq;void*_tmp346=({Cyc_Port_gen_exp(0,env,e1);});void*_tmp347=({Cyc_Port_arith_ct();});_tmp345(_tmp346,_tmp347);});
({int(*_tmp348)(void*t1,void*t2)=Cyc_Port_leq;void*_tmp349=({Cyc_Port_gen_exp(0,env,e2);});void*_tmp34A=({Cyc_Port_arith_ct();});_tmp348(_tmp349,_tmp34A);});
return({Cyc_Port_arith_ct();});}case 9U: _LL2D: _tmp2F5=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp2E1)->f1;_tmp2F6=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp2E1)->f2;_LL2E: {struct Cyc_Absyn_Exp*e1=_tmp2F5;struct Cyc_Absyn_Exp*e2=_tmp2F6;
# 1475
({Cyc_Port_gen_exp(0,env,e1);});
return({Cyc_Port_gen_exp(0,env,e2);});}case 10U: _LL2F: _tmp2F3=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp2E1)->f1;_tmp2F4=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp2E1)->f2;_LL30: {struct Cyc_Absyn_Exp*e1=_tmp2F3;struct Cyc_List_List*es=_tmp2F4;
# 1478
void*v=({Cyc_Port_var();});
void*t1=({Cyc_Port_gen_exp(0,env,e1);});
struct Cyc_List_List*ts=({({struct Cyc_List_List*(*_tmp5AA)(void*(*f)(struct Cyc_Port_Cenv*,struct Cyc_Absyn_Exp*),struct Cyc_Port_Cenv*env,struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp354)(void*(*f)(struct Cyc_Port_Cenv*,struct Cyc_Absyn_Exp*),struct Cyc_Port_Cenv*env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(void*(*f)(struct Cyc_Port_Cenv*,struct Cyc_Absyn_Exp*),struct Cyc_Port_Cenv*env,struct Cyc_List_List*x))Cyc_List_map_c;_tmp354;});struct Cyc_Port_Cenv*_tmp5A9=env;_tmp5AA(Cyc_Port_gen_exp_false,_tmp5A9,es);});});
struct Cyc_List_List*vs=({Cyc_List_map(Cyc_Port_new_var,ts);});
({int(*_tmp34B)(void*t1,void*t2)=Cyc_Port_unifies;void*_tmp34C=t1;void*_tmp34D=({void*(*_tmp34E)(void*elt,void*qual,void*ptr_kind,void*r,void*zt)=Cyc_Port_ptr_ct;void*_tmp34F=({Cyc_Port_fn_ct(v,vs);});void*_tmp350=({Cyc_Port_var();});void*_tmp351=({Cyc_Port_var();});void*_tmp352=({Cyc_Port_var();});void*_tmp353=({Cyc_Port_nozterm_ct();});_tmp34E(_tmp34F,_tmp350,_tmp351,_tmp352,_tmp353);});_tmp34B(_tmp34C,_tmp34D);});
for(0;ts != 0;(ts=ts->tl,vs=vs->tl)){
({Cyc_Port_leq((void*)ts->hd,(void*)((struct Cyc_List_List*)_check_null(vs))->hd);});}
# 1486
return v;}case 34U: _LL31: _LL32:
({void*_tmp355=0U;({int(*_tmp5AC)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp357)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp357;});struct _fat_ptr _tmp5AB=({const char*_tmp356="throw";_tag_fat(_tmp356,sizeof(char),6U);});_tmp5AC(_tmp5AB,_tag_fat(_tmp355,sizeof(void*),0U));});});case 11U: _LL33: _LL34:
({void*_tmp358=0U;({int(*_tmp5AE)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp35A)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp35A;});struct _fat_ptr _tmp5AD=({const char*_tmp359="throw";_tag_fat(_tmp359,sizeof(char),6U);});_tmp5AE(_tmp5AD,_tag_fat(_tmp358,sizeof(void*),0U));});});case 42U: _LL35: _tmp2F2=((struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct*)_tmp2E1)->f1;_LL36: {struct Cyc_Absyn_Exp*e1=_tmp2F2;
return({Cyc_Port_gen_exp(0,env,e1);});}case 12U: _LL37: _tmp2F1=((struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*)_tmp2E1)->f1;_LL38: {struct Cyc_Absyn_Exp*e1=_tmp2F1;
return({Cyc_Port_gen_exp(0,env,e1);});}case 13U: _LL39: _LL3A:
({void*_tmp35B=0U;({int(*_tmp5B0)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp35D)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp35D;});struct _fat_ptr _tmp5AF=({const char*_tmp35C="instantiate";_tag_fat(_tmp35C,sizeof(char),12U);});_tmp5B0(_tmp5AF,_tag_fat(_tmp35B,sizeof(void*),0U));});});case 14U: _LL3B: _tmp2EF=(void*)((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp2E1)->f1;_tmp2F0=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp2E1)->f2;_LL3C: {void*t=_tmp2EF;struct Cyc_Absyn_Exp*e1=_tmp2F0;
# 1493
void*t1=({Cyc_Port_type_to_ctype(env,t);});
void*t2=({Cyc_Port_gen_exp(0,env,e1);});
({Cyc_Port_leq(t1,t2);});
return t2;}case 15U: _LL3D: _tmp2EE=((struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct*)_tmp2E1)->f1;_LL3E: {struct Cyc_Absyn_Exp*e1=_tmp2EE;
# 1498
struct _tuple13 _stmttmp1E=({Cyc_Port_gen_lhs(env,e1,1);});void*_tmp35F;void*_tmp35E;_LL76: _tmp35E=_stmttmp1E.f1;_tmp35F=_stmttmp1E.f2;_LL77: {void*q=_tmp35E;void*t=_tmp35F;
return({void*(*_tmp360)(void*elt,void*qual,void*ptr_kind,void*r,void*zt)=Cyc_Port_ptr_ct;void*_tmp361=t;void*_tmp362=q;void*_tmp363=({Cyc_Port_var();});void*_tmp364=({Cyc_Port_var();});void*_tmp365=({Cyc_Port_var();});_tmp360(_tmp361,_tmp362,_tmp363,_tmp364,_tmp365);});}}case 23U: _LL3F: _tmp2EC=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp2E1)->f1;_tmp2ED=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp2E1)->f2;_LL40: {struct Cyc_Absyn_Exp*e1=_tmp2EC;struct Cyc_Absyn_Exp*e2=_tmp2ED;
# 1501
void*v=({Cyc_Port_var();});
({int(*_tmp366)(void*t1,void*t2)=Cyc_Port_leq;void*_tmp367=({Cyc_Port_gen_exp(0,env,e2);});void*_tmp368=({Cyc_Port_arith_ct();});_tmp366(_tmp367,_tmp368);});{
void*t=({Cyc_Port_gen_exp(0,env,e1);});
({int(*_tmp369)(void*t1,void*t2)=Cyc_Port_leq;void*_tmp36A=t;void*_tmp36B=({void*(*_tmp36C)(void*elt,void*qual,void*ptr_kind,void*r,void*zt)=Cyc_Port_ptr_ct;void*_tmp36D=v;void*_tmp36E=({Cyc_Port_var();});void*_tmp36F=({Cyc_Port_fat_ct();});void*_tmp370=({Cyc_Port_var();});void*_tmp371=({Cyc_Port_var();});_tmp36C(_tmp36D,_tmp36E,_tmp36F,_tmp370,_tmp371);});_tmp369(_tmp36A,_tmp36B);});
return v;}}case 20U: _LL41: _tmp2EB=((struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_tmp2E1)->f1;_LL42: {struct Cyc_Absyn_Exp*e1=_tmp2EB;
# 1507
void*v=({Cyc_Port_var();});
({int(*_tmp372)(void*t1,void*t2)=Cyc_Port_leq;void*_tmp373=({Cyc_Port_gen_exp(0,env,e1);});void*_tmp374=({void*(*_tmp375)(void*elt,void*qual,void*ptr_kind,void*r,void*zt)=Cyc_Port_ptr_ct;void*_tmp376=v;void*_tmp377=({Cyc_Port_var();});void*_tmp378=({Cyc_Port_var();});void*_tmp379=({Cyc_Port_var();});void*_tmp37A=({Cyc_Port_var();});_tmp375(_tmp376,_tmp377,_tmp378,_tmp379,_tmp37A);});_tmp372(_tmp373,_tmp374);});
return v;}case 21U: _LL43: _tmp2E9=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp2E1)->f1;_tmp2EA=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp2E1)->f2;_LL44: {struct Cyc_Absyn_Exp*e1=_tmp2E9;struct _fat_ptr*f=_tmp2EA;
# 1511
void*v=({Cyc_Port_var();});
void*t=({Cyc_Port_gen_exp(takeaddress,env,e1);});
({int(*_tmp37B)(void*t1,void*t2)=Cyc_Port_leq;void*_tmp37C=t;void*_tmp37D=({void*(*_tmp37E)(struct Cyc_List_List*fs)=Cyc_Port_unknown_aggr_ct;struct Cyc_List_List*_tmp37F=({struct Cyc_Port_Cfield*_tmp380[1U];({struct Cyc_Port_Cfield*_tmp5B2=({struct Cyc_Port_Cfield*_tmp381=_cycalloc(sizeof(*_tmp381));({void*_tmp5B1=({Cyc_Port_var();});_tmp381->qual=_tmp5B1;}),_tmp381->f=f,_tmp381->type=v;_tmp381;});_tmp380[0]=_tmp5B2;});Cyc_List_list(_tag_fat(_tmp380,sizeof(struct Cyc_Port_Cfield*),1U));});_tmp37E(_tmp37F);});_tmp37B(_tmp37C,_tmp37D);});
return v;}case 22U: _LL45: _tmp2E7=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp2E1)->f1;_tmp2E8=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp2E1)->f2;_LL46: {struct Cyc_Absyn_Exp*e1=_tmp2E7;struct _fat_ptr*f=_tmp2E8;
# 1516
void*v=({Cyc_Port_var();});
void*t=({Cyc_Port_gen_exp(0,env,e1);});
({int(*_tmp382)(void*t1,void*t2)=Cyc_Port_leq;void*_tmp383=({Cyc_Port_gen_exp(0,env,e1);});void*_tmp384=({void*(*_tmp385)(void*elt,void*qual,void*ptr_kind,void*r,void*zt)=Cyc_Port_ptr_ct;void*_tmp386=({void*(*_tmp387)(struct Cyc_List_List*fs)=Cyc_Port_unknown_aggr_ct;struct Cyc_List_List*_tmp388=({struct Cyc_Port_Cfield*_tmp389[1U];({struct Cyc_Port_Cfield*_tmp5B4=({struct Cyc_Port_Cfield*_tmp38A=_cycalloc(sizeof(*_tmp38A));({void*_tmp5B3=({Cyc_Port_var();});_tmp38A->qual=_tmp5B3;}),_tmp38A->f=f,_tmp38A->type=v;_tmp38A;});_tmp389[0]=_tmp5B4;});Cyc_List_list(_tag_fat(_tmp389,sizeof(struct Cyc_Port_Cfield*),1U));});_tmp387(_tmp388);});void*_tmp38B=({Cyc_Port_var();});void*_tmp38C=({Cyc_Port_var();});void*_tmp38D=({Cyc_Port_var();});void*_tmp38E=({Cyc_Port_var();});_tmp385(_tmp386,_tmp38B,_tmp38C,_tmp38D,_tmp38E);});_tmp382(_tmp383,_tmp384);});
# 1520
return v;}case 35U: _LL47: _tmp2E5=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp2E1)->f1).elt_type;_tmp2E6=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp2E1)->f1).num_elts;_LL48: {void**topt=_tmp2E5;struct Cyc_Absyn_Exp*e1=_tmp2E6;
# 1522
({int(*_tmp38F)(void*t1,void*t2)=Cyc_Port_leq;void*_tmp390=({Cyc_Port_gen_exp(0,env,e1);});void*_tmp391=({Cyc_Port_arith_ct();});_tmp38F(_tmp390,_tmp391);});{
void*t=(unsigned)topt?({Cyc_Port_type_to_ctype(env,*topt);}):({Cyc_Port_var();});
return({void*(*_tmp392)(void*elt,void*qual,void*ptr_kind,void*r,void*zt)=Cyc_Port_ptr_ct;void*_tmp393=t;void*_tmp394=({Cyc_Port_var();});void*_tmp395=({Cyc_Port_fat_ct();});void*_tmp396=({Cyc_Port_heap_ct();});void*_tmp397=({Cyc_Port_var();});_tmp392(_tmp393,_tmp394,_tmp395,_tmp396,_tmp397);});}}case 2U: _LL49: _LL4A:
({void*_tmp398=0U;({int(*_tmp5B6)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp39A)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp39A;});struct _fat_ptr _tmp5B5=({const char*_tmp399="Pragma_e";_tag_fat(_tmp399,sizeof(char),9U);});_tmp5B6(_tmp5B5,_tag_fat(_tmp398,sizeof(void*),0U));});});case 36U: _LL4B: _tmp2E3=((struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct*)_tmp2E1)->f1;_tmp2E4=((struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct*)_tmp2E1)->f2;_LL4C: {struct Cyc_Absyn_Exp*e1=_tmp2E3;struct Cyc_Absyn_Exp*e2=_tmp2E4;
({void*_tmp39B=0U;({int(*_tmp5B8)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp39D)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp39D;});struct _fat_ptr _tmp5B7=({const char*_tmp39C="Swap_e";_tag_fat(_tmp39C,sizeof(char),7U);});_tmp5B8(_tmp5B7,_tag_fat(_tmp39B,sizeof(void*),0U));});});}case 16U: _LL4D: _LL4E:
({void*_tmp39E=0U;({int(*_tmp5BA)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp3A0)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp3A0;});struct _fat_ptr _tmp5B9=({const char*_tmp39F="New_e";_tag_fat(_tmp39F,sizeof(char),6U);});_tmp5BA(_tmp5B9,_tag_fat(_tmp39E,sizeof(void*),0U));});});case 24U: _LL4F: _LL50:
({void*_tmp3A1=0U;({int(*_tmp5BC)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp3A3)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp3A3;});struct _fat_ptr _tmp5BB=({const char*_tmp3A2="Tuple_e";_tag_fat(_tmp3A2,sizeof(char),8U);});_tmp5BC(_tmp5BB,_tag_fat(_tmp3A1,sizeof(void*),0U));});});case 25U: _LL51: _LL52:
({void*_tmp3A4=0U;({int(*_tmp5BE)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp3A6)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp3A6;});struct _fat_ptr _tmp5BD=({const char*_tmp3A5="CompoundLit_e";_tag_fat(_tmp3A5,sizeof(char),14U);});_tmp5BE(_tmp5BD,_tag_fat(_tmp3A4,sizeof(void*),0U));});});case 26U: _LL53: _LL54:
({void*_tmp3A7=0U;({int(*_tmp5C0)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp3A9)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp3A9;});struct _fat_ptr _tmp5BF=({const char*_tmp3A8="Array_e";_tag_fat(_tmp3A8,sizeof(char),8U);});_tmp5C0(_tmp5BF,_tag_fat(_tmp3A7,sizeof(void*),0U));});});case 27U: _LL55: _LL56:
({void*_tmp3AA=0U;({int(*_tmp5C2)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp3AC)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp3AC;});struct _fat_ptr _tmp5C1=({const char*_tmp3AB="Comprehension_e";_tag_fat(_tmp3AB,sizeof(char),16U);});_tmp5C2(_tmp5C1,_tag_fat(_tmp3AA,sizeof(void*),0U));});});case 28U: _LL57: _LL58:
({void*_tmp3AD=0U;({int(*_tmp5C4)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp3AF)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp3AF;});struct _fat_ptr _tmp5C3=({const char*_tmp3AE="ComprehensionNoinit_e";_tag_fat(_tmp3AE,sizeof(char),22U);});_tmp5C4(_tmp5C3,_tag_fat(_tmp3AD,sizeof(void*),0U));});});case 29U: _LL59: _LL5A:
({void*_tmp3B0=0U;({int(*_tmp5C6)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp3B2)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp3B2;});struct _fat_ptr _tmp5C5=({const char*_tmp3B1="Aggregate_e";_tag_fat(_tmp3B1,sizeof(char),12U);});_tmp5C6(_tmp5C5,_tag_fat(_tmp3B0,sizeof(void*),0U));});});case 30U: _LL5B: _LL5C:
({void*_tmp3B3=0U;({int(*_tmp5C8)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp3B5)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp3B5;});struct _fat_ptr _tmp5C7=({const char*_tmp3B4="AnonStruct_e";_tag_fat(_tmp3B4,sizeof(char),13U);});_tmp5C8(_tmp5C7,_tag_fat(_tmp3B3,sizeof(void*),0U));});});case 31U: _LL5D: _LL5E:
({void*_tmp3B6=0U;({int(*_tmp5CA)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp3B8)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp3B8;});struct _fat_ptr _tmp5C9=({const char*_tmp3B7="Datatype_e";_tag_fat(_tmp3B7,sizeof(char),11U);});_tmp5CA(_tmp5C9,_tag_fat(_tmp3B6,sizeof(void*),0U));});});case 37U: _LL5F: _LL60:
({void*_tmp3B9=0U;({int(*_tmp5CC)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp3BB)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp3BB;});struct _fat_ptr _tmp5CB=({const char*_tmp3BA="UnresolvedMem_e";_tag_fat(_tmp3BA,sizeof(char),16U);});_tmp5CC(_tmp5CB,_tag_fat(_tmp3B9,sizeof(void*),0U));});});case 38U: _LL61: _tmp2E2=((struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct*)_tmp2E1)->f1;_LL62: {struct Cyc_Absyn_Stmt*s=_tmp2E2;
({void*_tmp3BC=0U;({int(*_tmp5CE)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp3BE)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp3BE;});struct _fat_ptr _tmp5CD=({const char*_tmp3BD="StmtExp_e";_tag_fat(_tmp3BD,sizeof(char),10U);});_tmp5CE(_tmp5CD,_tag_fat(_tmp3BC,sizeof(void*),0U));});});}case 40U: _LL63: _LL64:
({void*_tmp3BF=0U;({int(*_tmp5D0)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp3C1)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp3C1;});struct _fat_ptr _tmp5CF=({const char*_tmp3C0="Valueof_e";_tag_fat(_tmp3C0,sizeof(char),10U);});_tmp5D0(_tmp5CF,_tag_fat(_tmp3BF,sizeof(void*),0U));});});case 41U: _LL65: _LL66:
({void*_tmp3C2=0U;({int(*_tmp5D2)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp3C4)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp3C4;});struct _fat_ptr _tmp5D1=({const char*_tmp3C3="Asm_e";_tag_fat(_tmp3C3,sizeof(char),6U);});_tmp5D2(_tmp5D1,_tag_fat(_tmp3C2,sizeof(void*),0U));});});default: _LL67: _LL68:
({void*_tmp3C5=0U;({int(*_tmp5D4)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp3C7)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp3C7;});struct _fat_ptr _tmp5D3=({const char*_tmp3C6="Tagcheck_e";_tag_fat(_tmp3C6,sizeof(char),11U);});_tmp5D4(_tmp5D3,_tag_fat(_tmp3C5,sizeof(void*),0U));});});}_LL0:;}
# 1545
static void Cyc_Port_gen_stmt(struct Cyc_Port_Cenv*env,struct Cyc_Absyn_Stmt*s){
void*_stmttmp1F=s->r;void*_tmp3C9=_stmttmp1F;struct Cyc_Absyn_Exp*_tmp3CB;struct Cyc_Absyn_Stmt*_tmp3CA;struct Cyc_Absyn_Stmt*_tmp3CC;struct Cyc_Absyn_Stmt*_tmp3CE;struct Cyc_Absyn_Decl*_tmp3CD;struct Cyc_List_List*_tmp3D0;struct Cyc_Absyn_Exp*_tmp3CF;struct Cyc_Absyn_Stmt*_tmp3D4;struct Cyc_Absyn_Exp*_tmp3D3;struct Cyc_Absyn_Exp*_tmp3D2;struct Cyc_Absyn_Exp*_tmp3D1;struct Cyc_Absyn_Stmt*_tmp3D6;struct Cyc_Absyn_Exp*_tmp3D5;struct Cyc_Absyn_Stmt*_tmp3D9;struct Cyc_Absyn_Stmt*_tmp3D8;struct Cyc_Absyn_Exp*_tmp3D7;struct Cyc_Absyn_Exp*_tmp3DA;struct Cyc_Absyn_Stmt*_tmp3DC;struct Cyc_Absyn_Stmt*_tmp3DB;struct Cyc_Absyn_Exp*_tmp3DD;switch(*((int*)_tmp3C9)){case 0U: _LL1: _LL2:
 return;case 1U: _LL3: _tmp3DD=((struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct*)_tmp3C9)->f1;_LL4: {struct Cyc_Absyn_Exp*e=_tmp3DD;
({Cyc_Port_gen_exp(0,env,e);});return;}case 2U: _LL5: _tmp3DB=((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_tmp3C9)->f1;_tmp3DC=((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_tmp3C9)->f2;_LL6: {struct Cyc_Absyn_Stmt*s1=_tmp3DB;struct Cyc_Absyn_Stmt*s2=_tmp3DC;
({Cyc_Port_gen_stmt(env,s1);});({Cyc_Port_gen_stmt(env,s2);});return;}case 3U: _LL7: _tmp3DA=((struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct*)_tmp3C9)->f1;_LL8: {struct Cyc_Absyn_Exp*eopt=_tmp3DA;
# 1551
void*v=({Cyc_Port_lookup_return_type(env);});
void*t=(unsigned)eopt?({Cyc_Port_gen_exp(0,env,eopt);}):({Cyc_Port_void_ct();});
({Cyc_Port_leq(t,v);});
return;}case 4U: _LL9: _tmp3D7=((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmp3C9)->f1;_tmp3D8=((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmp3C9)->f2;_tmp3D9=((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmp3C9)->f3;_LLA: {struct Cyc_Absyn_Exp*e=_tmp3D7;struct Cyc_Absyn_Stmt*s1=_tmp3D8;struct Cyc_Absyn_Stmt*s2=_tmp3D9;
# 1556
({int(*_tmp3DE)(void*t1,void*t2)=Cyc_Port_leq;void*_tmp3DF=({Cyc_Port_gen_exp(0,env,e);});void*_tmp3E0=({Cyc_Port_arith_ct();});_tmp3DE(_tmp3DF,_tmp3E0);});
({Cyc_Port_gen_stmt(env,s1);});
({Cyc_Port_gen_stmt(env,s2);});
return;}case 5U: _LLB: _tmp3D5=(((struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct*)_tmp3C9)->f1).f1;_tmp3D6=((struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct*)_tmp3C9)->f2;_LLC: {struct Cyc_Absyn_Exp*e=_tmp3D5;struct Cyc_Absyn_Stmt*s=_tmp3D6;
# 1561
({int(*_tmp3E1)(void*t1,void*t2)=Cyc_Port_leq;void*_tmp3E2=({Cyc_Port_gen_exp(0,env,e);});void*_tmp3E3=({Cyc_Port_arith_ct();});_tmp3E1(_tmp3E2,_tmp3E3);});
({Cyc_Port_gen_stmt(env,s);});
return;}case 6U: _LLD: _LLE:
 goto _LL10;case 7U: _LLF: _LL10:
 goto _LL12;case 8U: _LL11: _LL12:
 return;case 9U: _LL13: _tmp3D1=((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp3C9)->f1;_tmp3D2=(((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp3C9)->f2).f1;_tmp3D3=(((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp3C9)->f3).f1;_tmp3D4=((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp3C9)->f4;_LL14: {struct Cyc_Absyn_Exp*e1=_tmp3D1;struct Cyc_Absyn_Exp*e2=_tmp3D2;struct Cyc_Absyn_Exp*e3=_tmp3D3;struct Cyc_Absyn_Stmt*s=_tmp3D4;
# 1568
({Cyc_Port_gen_exp(0,env,e1);});
({int(*_tmp3E4)(void*t1,void*t2)=Cyc_Port_leq;void*_tmp3E5=({Cyc_Port_gen_exp(0,env,e2);});void*_tmp3E6=({Cyc_Port_arith_ct();});_tmp3E4(_tmp3E5,_tmp3E6);});
({Cyc_Port_gen_exp(0,env,e3);});
({Cyc_Port_gen_stmt(env,s);});
return;}case 10U: _LL15: _tmp3CF=((struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct*)_tmp3C9)->f1;_tmp3D0=((struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct*)_tmp3C9)->f2;_LL16: {struct Cyc_Absyn_Exp*e=_tmp3CF;struct Cyc_List_List*scs=_tmp3D0;
# 1574
({int(*_tmp3E7)(void*t1,void*t2)=Cyc_Port_leq;void*_tmp3E8=({Cyc_Port_gen_exp(0,env,e);});void*_tmp3E9=({Cyc_Port_arith_ct();});_tmp3E7(_tmp3E8,_tmp3E9);});
for(0;(unsigned)scs;scs=scs->tl){
# 1577
({Cyc_Port_gen_stmt(env,((struct Cyc_Absyn_Switch_clause*)scs->hd)->body);});}
# 1579
return;}case 11U: _LL17: _LL18:
({void*_tmp3EA=0U;({int(*_tmp5D6)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp3EC)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp3EC;});struct _fat_ptr _tmp5D5=({const char*_tmp3EB="fallthru";_tag_fat(_tmp3EB,sizeof(char),9U);});_tmp5D6(_tmp5D5,_tag_fat(_tmp3EA,sizeof(void*),0U));});});case 12U: _LL19: _tmp3CD=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp3C9)->f1;_tmp3CE=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp3C9)->f2;_LL1A: {struct Cyc_Absyn_Decl*d=_tmp3CD;struct Cyc_Absyn_Stmt*s=_tmp3CE;
# 1582
env=({Cyc_Port_gen_localdecl(env,d,0);});({Cyc_Port_gen_stmt(env,s);});return;}case 13U: _LL1B: _tmp3CC=((struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct*)_tmp3C9)->f2;_LL1C: {struct Cyc_Absyn_Stmt*s=_tmp3CC;
({Cyc_Port_gen_stmt(env,s);});return;}case 14U: _LL1D: _tmp3CA=((struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct*)_tmp3C9)->f1;_tmp3CB=(((struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct*)_tmp3C9)->f2).f1;_LL1E: {struct Cyc_Absyn_Stmt*s=_tmp3CA;struct Cyc_Absyn_Exp*e=_tmp3CB;
# 1585
({Cyc_Port_gen_stmt(env,s);});
({int(*_tmp3ED)(void*t1,void*t2)=Cyc_Port_leq;void*_tmp3EE=({Cyc_Port_gen_exp(0,env,e);});void*_tmp3EF=({Cyc_Port_arith_ct();});_tmp3ED(_tmp3EE,_tmp3EF);});
return;}default: _LL1F: _LL20:
({void*_tmp3F0=0U;({int(*_tmp5D8)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp3F2)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp3F2;});struct _fat_ptr _tmp5D7=({const char*_tmp3F1="try/catch";_tag_fat(_tmp3F1,sizeof(char),10U);});_tmp5D8(_tmp5D7,_tag_fat(_tmp3F0,sizeof(void*),0U));});});}_LL0:;}struct _tuple19{struct Cyc_List_List*f1;struct Cyc_Absyn_Exp*f2;};
# 1593
static void*Cyc_Port_gen_initializer(struct Cyc_Port_Cenv*env,void*t,struct Cyc_Absyn_Exp*e){
void*_stmttmp20=e->r;void*_tmp3F4=_stmttmp20;struct Cyc_List_List*_tmp3F5;struct Cyc_List_List*_tmp3F6;struct Cyc_List_List*_tmp3F7;switch(*((int*)_tmp3F4)){case 37U: _LL1: _tmp3F7=((struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*)_tmp3F4)->f2;_LL2: {struct Cyc_List_List*dles=_tmp3F7;
_tmp3F6=dles;goto _LL4;}case 26U: _LL3: _tmp3F6=((struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*)_tmp3F4)->f1;_LL4: {struct Cyc_List_List*dles=_tmp3F6;
_tmp3F5=dles;goto _LL6;}case 25U: _LL5: _tmp3F5=((struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct*)_tmp3F4)->f2;_LL6: {struct Cyc_List_List*dles=_tmp3F5;
# 1598
LOOP: {
void*_tmp3F8=t;struct Cyc_Absyn_Aggrdecl*_tmp3F9;struct _tuple0*_tmp3FA;unsigned _tmp3FD;void*_tmp3FC;void*_tmp3FB;struct _tuple0*_tmp3FE;switch(*((int*)_tmp3F8)){case 8U: _LLE: _tmp3FE=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp3F8)->f1;_LLF: {struct _tuple0*n=_tmp3FE;
# 1601
(*n).f1=Cyc_Absyn_Loc_n;
t=({Cyc_Port_lookup_typedef(env,n);});goto LOOP;}case 4U: _LL10: _tmp3FB=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp3F8)->f1).elt_type;_tmp3FC=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp3F8)->f1).zero_term;_tmp3FD=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp3F8)->f1).zt_loc;_LL11: {void*et=_tmp3FB;void*zt=_tmp3FC;unsigned ztl=_tmp3FD;
# 1604
void*v=({Cyc_Port_var();});
void*q=({Cyc_Port_var();});
void*z=({Cyc_Port_var();});
void*t=({Cyc_Port_type_to_ctype(env,et);});
for(0;dles != 0;dles=dles->tl){
struct _tuple19 _stmttmp21=*((struct _tuple19*)dles->hd);struct Cyc_Absyn_Exp*_tmp400;struct Cyc_List_List*_tmp3FF;_LL19: _tmp3FF=_stmttmp21.f1;_tmp400=_stmttmp21.f2;_LL1A: {struct Cyc_List_List*d=_tmp3FF;struct Cyc_Absyn_Exp*e=_tmp400;
if((unsigned)d)({void*_tmp401=0U;({int(*_tmp5DA)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp403)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp403;});struct _fat_ptr _tmp5D9=({const char*_tmp402="designators in initializers";_tag_fat(_tmp402,sizeof(char),28U);});_tmp5DA(_tmp5D9,_tag_fat(_tmp401,sizeof(void*),0U));});});{void*te=({Cyc_Port_gen_initializer(env,et,e);});
# 1612
({Cyc_Port_leq(te,v);});}}}
# 1614
return({Cyc_Port_array_ct(v,q,z);});}case 0U: if(((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp3F8)->f1)->tag == 22U){if(((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp3F8)->f1)->f1).UnknownAggr).tag == 1){if((((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp3F8)->f1)->f1).UnknownAggr).val).f1 == Cyc_Absyn_StructA){_LL12: _tmp3FA=(((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp3F8)->f1)->f1).UnknownAggr).val).f2;_LL13: {struct _tuple0*n=_tmp3FA;
# 1616
(*n).f1=Cyc_Absyn_Loc_n;{
struct _tuple12 _stmttmp22=*({Cyc_Port_lookup_struct_decl(env,n);});struct Cyc_Absyn_Aggrdecl*_tmp404;_LL1C: _tmp404=_stmttmp22.f1;_LL1D: {struct Cyc_Absyn_Aggrdecl*ad=_tmp404;
if(ad == 0)({void*_tmp405=0U;({int(*_tmp5DC)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp407)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp407;});struct _fat_ptr _tmp5DB=({const char*_tmp406="struct is not yet defined";_tag_fat(_tmp406,sizeof(char),26U);});_tmp5DC(_tmp5DB,_tag_fat(_tmp405,sizeof(void*),0U));});});_tmp3F9=ad;goto _LL15;}}}}else{goto _LL16;}}else{_LL14: _tmp3F9=*((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp3F8)->f1)->f1).KnownAggr).val;_LL15: {struct Cyc_Absyn_Aggrdecl*ad=_tmp3F9;
# 1621
struct Cyc_List_List*fields=((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->fields;
for(0;dles != 0;dles=dles->tl){
struct _tuple19 _stmttmp23=*((struct _tuple19*)dles->hd);struct Cyc_Absyn_Exp*_tmp409;struct Cyc_List_List*_tmp408;_LL1F: _tmp408=_stmttmp23.f1;_tmp409=_stmttmp23.f2;_LL20: {struct Cyc_List_List*d=_tmp408;struct Cyc_Absyn_Exp*e=_tmp409;
if((unsigned)d)({void*_tmp40A=0U;({int(*_tmp5DE)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp40C)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp40C;});struct _fat_ptr _tmp5DD=({const char*_tmp40B="designators in initializers";_tag_fat(_tmp40B,sizeof(char),28U);});_tmp5DE(_tmp5DD,_tag_fat(_tmp40A,sizeof(void*),0U));});});{struct Cyc_Absyn_Aggrfield*f=(struct Cyc_Absyn_Aggrfield*)((struct Cyc_List_List*)_check_null(fields))->hd;
# 1626
fields=fields->tl;{
void*te=({Cyc_Port_gen_initializer(env,f->type,e);});;}}}}
# 1629
return({Cyc_Port_type_to_ctype(env,t);});}}}else{goto _LL16;}default: _LL16: _LL17:
({void*_tmp40D=0U;({int(*_tmp5E0)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp40F)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp40F;});struct _fat_ptr _tmp5DF=({const char*_tmp40E="bad type for aggregate initializer";_tag_fat(_tmp40E,sizeof(char),35U);});_tmp5E0(_tmp5DF,_tag_fat(_tmp40D,sizeof(void*),0U));});});}_LLD:;}}case 0U: switch(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp3F4)->f1).Wstring_c).tag){case 8U: _LL7: _LL8:
# 1632
 goto _LLA;case 9U: _LL9: _LLA:
# 1634
 LOOP2: {
void*_tmp410=t;struct _tuple0*_tmp411;switch(*((int*)_tmp410)){case 8U: _LL22: _tmp411=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp410)->f1;_LL23: {struct _tuple0*n=_tmp411;
# 1637
(*n).f1=Cyc_Absyn_Loc_n;
t=({Cyc_Port_lookup_typedef(env,n);});goto LOOP2;}case 4U: _LL24: _LL25:
 return({void*(*_tmp412)(void*elt,void*qual,void*zt)=Cyc_Port_array_ct;void*_tmp413=({Cyc_Port_arith_ct();});void*_tmp414=({Cyc_Port_const_ct();});void*_tmp415=({Cyc_Port_zterm_ct();});_tmp412(_tmp413,_tmp414,_tmp415);});default: _LL26: _LL27:
 return({Cyc_Port_gen_exp(0,env,e);});}_LL21:;}default: goto _LLB;}default: _LLB: _LLC:
# 1642
 return({Cyc_Port_gen_exp(0,env,e);});}_LL0:;}
# 1647
static struct Cyc_Port_Cenv*Cyc_Port_gen_localdecl(struct Cyc_Port_Cenv*env,struct Cyc_Absyn_Decl*d,int fromglobal){
void*_stmttmp24=d->r;void*_tmp417=_stmttmp24;struct Cyc_Absyn_Vardecl*_tmp418;if(((struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_tmp417)->tag == 0U){_LL1: _tmp418=((struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_tmp417)->f1;_LL2: {struct Cyc_Absyn_Vardecl*vd=_tmp418;
# 1650
if(!fromglobal)({Cyc_Port_register_localvar_decl(env,vd->name,vd->varloc,vd->type,vd->initializer);});{void*t=({Cyc_Port_var();});
# 1652
void*q;
if((env->gcenv)->porting){
q=({Cyc_Port_var();});
({void(*_tmp419)(struct Cyc_Port_Cenv*env,void*new_qual,void*orig_qual,unsigned loc)=Cyc_Port_register_const_cvar;struct Cyc_Port_Cenv*_tmp41A=env;void*_tmp41B=q;void*_tmp41C=
(vd->tq).print_const?({Cyc_Port_const_ct();}):({Cyc_Port_notconst_ct();});unsigned _tmp41D=(vd->tq).loc;_tmp419(_tmp41A,_tmp41B,_tmp41C,_tmp41D);});}else{
# 1664
q=(vd->tq).print_const?({Cyc_Port_const_ct();}):({Cyc_Port_notconst_ct();});}
# 1666
(*vd->name).f1=Cyc_Absyn_Loc_n;
env=({Cyc_Port_add_var(env,vd->name,vd->type,q,t,vd->varloc);});
({int(*_tmp41E)(void*t1,void*t2)=Cyc_Port_unifies;void*_tmp41F=t;void*_tmp420=({Cyc_Port_type_to_ctype(env,vd->type);});_tmp41E(_tmp41F,_tmp420);});
if((unsigned)vd->initializer){
struct Cyc_Absyn_Exp*e=(struct Cyc_Absyn_Exp*)_check_null(vd->initializer);
void*t2=({Cyc_Port_gen_initializer(env,vd->type,e);});
({Cyc_Port_leq(t2,t);});}
# 1669
return env;}}}else{_LL3: _LL4:
# 1675
({void*_tmp421=0U;({int(*_tmp5E2)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp423)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp423;});struct _fat_ptr _tmp5E1=({const char*_tmp422="non-local decl that isn't a variable";_tag_fat(_tmp422,sizeof(char),37U);});_tmp5E2(_tmp5E1,_tag_fat(_tmp421,sizeof(void*),0U));});});}_LL0:;}
# 1679
static struct _tuple9*Cyc_Port_make_targ(struct _tuple9*arg){
struct _tuple9 _stmttmp25=*arg;void*_tmp427;struct Cyc_Absyn_Tqual _tmp426;struct _fat_ptr*_tmp425;_LL1: _tmp425=_stmttmp25.f1;_tmp426=_stmttmp25.f2;_tmp427=_stmttmp25.f3;_LL2: {struct _fat_ptr*x=_tmp425;struct Cyc_Absyn_Tqual q=_tmp426;void*t=_tmp427;
return({struct _tuple9*_tmp428=_cycalloc(sizeof(*_tmp428));_tmp428->f1=0,_tmp428->f2=q,_tmp428->f3=t;_tmp428;});}}
# 1679
static struct Cyc_Port_Cenv*Cyc_Port_gen_topdecls(struct Cyc_Port_Cenv*env,struct Cyc_List_List*ds);struct _tuple20{struct _fat_ptr*f1;void*f2;void*f3;void*f4;};
# 1686
static struct Cyc_Port_Cenv*Cyc_Port_gen_topdecl(struct Cyc_Port_Cenv*env,struct Cyc_Absyn_Decl*d){
void*_stmttmp26=d->r;void*_tmp42A=_stmttmp26;struct Cyc_Absyn_Enumdecl*_tmp42B;struct Cyc_Absyn_Aggrdecl*_tmp42C;struct Cyc_Absyn_Typedefdecl*_tmp42D;struct Cyc_Absyn_Fndecl*_tmp42E;struct Cyc_Absyn_Vardecl*_tmp42F;switch(*((int*)_tmp42A)){case 13U: _LL1: _LL2:
# 1689
(env->gcenv)->porting=1;
return env;case 14U: _LL3: _LL4:
# 1692
(env->gcenv)->porting=0;
return env;case 0U: _LL5: _tmp42F=((struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_tmp42A)->f1;_LL6: {struct Cyc_Absyn_Vardecl*vd=_tmp42F;
# 1695
(*vd->name).f1=Cyc_Absyn_Loc_n;
# 1699
if(({Cyc_Port_declared_var(env,vd->name);})){
struct _tuple15 _stmttmp27=({Cyc_Port_lookup_var(env,vd->name);});unsigned _tmp431;struct _tuple13 _tmp430;_LL12: _tmp430=_stmttmp27.f1;_tmp431=_stmttmp27.f2;_LL13: {struct _tuple13 p=_tmp430;unsigned loc=_tmp431;
void*_tmp433;void*_tmp432;_LL15: _tmp432=p.f1;_tmp433=p.f2;_LL16: {void*q=_tmp432;void*t=_tmp433;
void*q2;
if((env->gcenv)->porting && !({Cyc_Port_isfn(env,vd->name);})){
q2=({Cyc_Port_var();});
({void(*_tmp434)(struct Cyc_Port_Cenv*env,void*new_qual,void*orig_qual,unsigned loc)=Cyc_Port_register_const_cvar;struct Cyc_Port_Cenv*_tmp435=env;void*_tmp436=q2;void*_tmp437=
(vd->tq).print_const?({Cyc_Port_const_ct();}):({Cyc_Port_notconst_ct();});unsigned _tmp438=(vd->tq).loc;_tmp434(_tmp435,_tmp436,_tmp437,_tmp438);});}else{
# 1714
q2=(vd->tq).print_const?({Cyc_Port_const_ct();}):({Cyc_Port_notconst_ct();});}
# 1716
({Cyc_Port_unifies(q,q2);});
({int(*_tmp439)(void*t1,void*t2)=Cyc_Port_unifies;void*_tmp43A=t;void*_tmp43B=({Cyc_Port_type_to_ctype(env,vd->type);});_tmp439(_tmp43A,_tmp43B);});
if((unsigned)vd->initializer){
struct Cyc_Absyn_Exp*e=(struct Cyc_Absyn_Exp*)_check_null(vd->initializer);
({int(*_tmp43C)(void*t1,void*t2)=Cyc_Port_leq;void*_tmp43D=({Cyc_Port_gen_initializer(env,vd->type,e);});void*_tmp43E=t;_tmp43C(_tmp43D,_tmp43E);});}
# 1718
return env;}}}else{
# 1724
return({Cyc_Port_gen_localdecl(env,d,1);});}}case 1U: _LL7: _tmp42E=((struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*)_tmp42A)->f1;_LL8: {struct Cyc_Absyn_Fndecl*fd=_tmp42E;
# 1730
(*fd->name).f1=Cyc_Absyn_Loc_n;{
struct _tuple16*predeclared=0;
# 1733
if(({Cyc_Port_declared_var(env,fd->name);}))
predeclared=({Cyc_Port_lookup_full_var(env,fd->name);});{
# 1733
void*rettype=(fd->i).ret_type;
# 1737
struct Cyc_List_List*args=(fd->i).args;
struct Cyc_List_List*targs=({({struct Cyc_List_List*(*_tmp5E3)(struct _tuple9*(*f)(struct _tuple9*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp468)(struct _tuple9*(*f)(struct _tuple9*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct _tuple9*(*f)(struct _tuple9*),struct Cyc_List_List*x))Cyc_List_map;_tmp468;});_tmp5E3(Cyc_Port_make_targ,args);});});
struct Cyc_Absyn_FnType_Absyn_Type_struct*fntype=({struct Cyc_Absyn_FnType_Absyn_Type_struct*_tmp467=_cycalloc(sizeof(*_tmp467));
_tmp467->tag=5U,(_tmp467->f1).tvars=0,(_tmp467->f1).effect=0,({struct Cyc_Absyn_Tqual _tmp5E4=({Cyc_Absyn_empty_tqual(0U);});(_tmp467->f1).ret_tqual=_tmp5E4;}),(_tmp467->f1).ret_type=rettype,(_tmp467->f1).args=targs,(_tmp467->f1).c_varargs=0,(_tmp467->f1).cyc_varargs=0,(_tmp467->f1).rgn_po=0,(_tmp467->f1).attributes=0,(_tmp467->f1).requires_clause=0,(_tmp467->f1).requires_relns=0,(_tmp467->f1).ensures_clause=0,(_tmp467->f1).ensures_relns=0,(_tmp467->f1).ieffect=0,(_tmp467->f1).oeffect=0,(_tmp467->f1).throws=0,(_tmp467->f1).reentrant=Cyc_Absyn_noreentrant;_tmp467;});
# 1743
struct Cyc_Port_Cenv*fn_env=({Cyc_Port_set_cpos(env,Cyc_Port_FnBody_pos);});
void*c_rettype=({Cyc_Port_type_to_ctype(fn_env,rettype);});
struct Cyc_List_List*c_args=0;
struct Cyc_List_List*c_arg_types=0;
{struct Cyc_List_List*xs=args;for(0;(unsigned)xs;xs=xs->tl){
struct _tuple9 _stmttmp28=*((struct _tuple9*)xs->hd);void*_tmp441;struct Cyc_Absyn_Tqual _tmp440;struct _fat_ptr*_tmp43F;_LL18: _tmp43F=_stmttmp28.f1;_tmp440=_stmttmp28.f2;_tmp441=_stmttmp28.f3;_LL19: {struct _fat_ptr*x=_tmp43F;struct Cyc_Absyn_Tqual tq=_tmp440;void*t=_tmp441;
# 1751
void*ct=({Cyc_Port_type_to_ctype(fn_env,t);});
void*tqv;
if((env->gcenv)->porting){
tqv=({Cyc_Port_var();});
({void(*_tmp442)(struct Cyc_Port_Cenv*env,void*new_qual,void*orig_qual,unsigned loc)=Cyc_Port_register_const_cvar;struct Cyc_Port_Cenv*_tmp443=env;void*_tmp444=tqv;void*_tmp445=tq.print_const?({Cyc_Port_const_ct();}):({Cyc_Port_notconst_ct();});unsigned _tmp446=tq.loc;_tmp442(_tmp443,_tmp444,_tmp445,_tmp446);});}else{
# 1763
tqv=tq.print_const?({Cyc_Port_const_ct();}):({Cyc_Port_notconst_ct();});}
# 1765
c_args=({struct Cyc_List_List*_tmp448=_cycalloc(sizeof(*_tmp448));({struct _tuple20*_tmp5E5=({struct _tuple20*_tmp447=_cycalloc(sizeof(*_tmp447));_tmp447->f1=x,_tmp447->f2=t,_tmp447->f3=tqv,_tmp447->f4=ct;_tmp447;});_tmp448->hd=_tmp5E5;}),_tmp448->tl=c_args;_tmp448;});
c_arg_types=({struct Cyc_List_List*_tmp449=_cycalloc(sizeof(*_tmp449));_tmp449->hd=ct,_tmp449->tl=c_arg_types;_tmp449;});}}}
# 1768
c_args=({Cyc_List_imp_rev(c_args);});
c_arg_types=({Cyc_List_imp_rev(c_arg_types);});{
void*ctype=({Cyc_Port_fn_ct(c_rettype,c_arg_types);});
# 1774
(*fd->name).f1=Cyc_Absyn_Loc_n;
fn_env=({struct Cyc_Port_Cenv*(*_tmp44A)(struct Cyc_Port_Cenv*env,struct _tuple0*x,void*t,void*qual,void*ctype,unsigned loc)=Cyc_Port_add_var;struct Cyc_Port_Cenv*_tmp44B=fn_env;struct _tuple0*_tmp44C=fd->name;void*_tmp44D=(void*)fntype;void*_tmp44E=({Cyc_Port_const_ct();});void*_tmp44F=ctype;unsigned _tmp450=0U;_tmp44A(_tmp44B,_tmp44C,_tmp44D,_tmp44E,_tmp44F,_tmp450);});
({Cyc_Port_add_return_type(fn_env,c_rettype);});
{struct Cyc_List_List*xs=c_args;for(0;(unsigned)xs;xs=xs->tl){
struct _tuple20 _stmttmp29=*((struct _tuple20*)xs->hd);void*_tmp454;void*_tmp453;void*_tmp452;struct _fat_ptr*_tmp451;_LL1B: _tmp451=_stmttmp29.f1;_tmp452=_stmttmp29.f2;_tmp453=_stmttmp29.f3;_tmp454=_stmttmp29.f4;_LL1C: {struct _fat_ptr*x=_tmp451;void*t=_tmp452;void*q=_tmp453;void*ct=_tmp454;
fn_env=({({struct Cyc_Port_Cenv*_tmp5E9=fn_env;struct _tuple0*_tmp5E8=({struct _tuple0*_tmp455=_cycalloc(sizeof(*_tmp455));_tmp455->f1=Cyc_Absyn_Loc_n,_tmp455->f2=(struct _fat_ptr*)_check_null(x);_tmp455;});void*_tmp5E7=t;void*_tmp5E6=q;Cyc_Port_add_var(_tmp5E9,_tmp5E8,_tmp5E7,_tmp5E6,ct,0U);});});}}}
# 1781
({Cyc_Port_gen_stmt(fn_env,fd->body);});
# 1786
({Cyc_Port_generalize(0,ctype);});{
# 1793
struct Cyc_Dict_Dict counts=({({struct Cyc_Dict_Dict(*_tmp466)(int(*cmp)(struct _fat_ptr*,struct _fat_ptr*))=(struct Cyc_Dict_Dict(*)(int(*cmp)(struct _fat_ptr*,struct _fat_ptr*)))Cyc_Dict_empty;_tmp466;})(Cyc_strptrcmp);});
({Cyc_Port_region_counts(& counts,ctype);});
# 1797
({Cyc_Port_register_rgns(env,counts,1,(void*)fntype,ctype);});
env=({struct Cyc_Port_Cenv*(*_tmp456)(struct Cyc_Port_Cenv*env,struct _tuple0*x,void*t,void*qual,void*ctype,unsigned loc)=Cyc_Port_add_var;struct Cyc_Port_Cenv*_tmp457=fn_env;struct _tuple0*_tmp458=fd->name;void*_tmp459=(void*)fntype;void*_tmp45A=({Cyc_Port_const_ct();});void*_tmp45B=ctype;unsigned _tmp45C=0U;_tmp456(_tmp457,_tmp458,_tmp459,_tmp45A,_tmp45B,_tmp45C);});
if((unsigned)predeclared){
# 1806
struct _tuple16 _stmttmp2A=*predeclared;void*_tmp45F;void*_tmp45E;void*_tmp45D;_LL1E: _tmp45D=_stmttmp2A.f1;_tmp45E=(_stmttmp2A.f2)->f1;_tmp45F=(_stmttmp2A.f2)->f2;_LL1F: {void*orig_type=_tmp45D;void*q2=_tmp45E;void*t2=_tmp45F;
({int(*_tmp460)(void*t1,void*t2)=Cyc_Port_unifies;void*_tmp461=q2;void*_tmp462=({Cyc_Port_const_ct();});_tmp460(_tmp461,_tmp462);});
({int(*_tmp463)(void*t1,void*t2)=Cyc_Port_unifies;void*_tmp464=t2;void*_tmp465=({Cyc_Port_instantiate(ctype);});_tmp463(_tmp464,_tmp465);});
# 1810
({Cyc_Port_register_rgns(env,counts,1,orig_type,ctype);});}}
# 1799
return env;}}}}}case 8U: _LL9: _tmp42D=((struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct*)_tmp42A)->f1;_LLA: {struct Cyc_Absyn_Typedefdecl*td=_tmp42D;
# 1818
void*t=(void*)_check_null(td->defn);
void*ct=({Cyc_Port_type_to_ctype(env,t);});
(*td->name).f1=Cyc_Absyn_Loc_n;
({Cyc_Port_add_typedef(env,td->name,t,ct);});
return env;}case 5U: _LLB: _tmp42C=((struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct*)_tmp42A)->f1;_LLC: {struct Cyc_Absyn_Aggrdecl*ad=_tmp42C;
# 1828
struct _tuple0*name=ad->name;
(*ad->name).f1=Cyc_Absyn_Loc_n;{
struct _tuple12*previous;
struct Cyc_List_List*curr=0;
{enum Cyc_Absyn_AggrKind _stmttmp2B=ad->kind;enum Cyc_Absyn_AggrKind _tmp469=_stmttmp2B;if(_tmp469 == Cyc_Absyn_StructA){_LL21: _LL22:
# 1834
 previous=({Cyc_Port_lookup_struct_decl(env,name);});
goto _LL20;}else{_LL23: _LL24:
# 1837
 previous=({Cyc_Port_lookup_union_decl(env,name);});
goto _LL20;}_LL20:;}
# 1840
if((unsigned)ad->impl){
struct Cyc_List_List*cf=(*previous).f2;
{struct Cyc_List_List*fields=((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->fields;for(0;(unsigned)fields;fields=fields->tl){
struct Cyc_Absyn_Aggrfield*f=(struct Cyc_Absyn_Aggrfield*)fields->hd;
void*qv;
if((env->gcenv)->porting){
qv=({Cyc_Port_var();});
({void(*_tmp46A)(struct Cyc_Port_Cenv*env,void*new_qual,void*orig_qual,unsigned loc)=Cyc_Port_register_const_cvar;struct Cyc_Port_Cenv*_tmp46B=env;void*_tmp46C=qv;void*_tmp46D=
(f->tq).print_const?({Cyc_Port_const_ct();}):({Cyc_Port_notconst_ct();});unsigned _tmp46E=(f->tq).loc;_tmp46A(_tmp46B,_tmp46C,_tmp46D,_tmp46E);});}else{
# 1856
qv=(f->tq).print_const?({Cyc_Port_const_ct();}):({Cyc_Port_notconst_ct();});}{
# 1858
void*ct=({Cyc_Port_type_to_ctype(env,f->type);});
if(cf != 0){
struct Cyc_Port_Cfield _stmttmp2C=*((struct Cyc_Port_Cfield*)cf->hd);void*_tmp470;void*_tmp46F;_LL26: _tmp46F=_stmttmp2C.qual;_tmp470=_stmttmp2C.type;_LL27: {void*qv2=_tmp46F;void*ct2=_tmp470;
cf=cf->tl;
({Cyc_Port_unifies(qv,qv2);});
({Cyc_Port_unifies(ct,ct2);});}}
# 1859
curr=({struct Cyc_List_List*_tmp472=_cycalloc(sizeof(*_tmp472));
# 1865
({struct Cyc_Port_Cfield*_tmp5EA=({struct Cyc_Port_Cfield*_tmp471=_cycalloc(sizeof(*_tmp471));_tmp471->qual=qv,_tmp471->f=f->name,_tmp471->type=ct;_tmp471;});_tmp472->hd=_tmp5EA;}),_tmp472->tl=curr;_tmp472;});}}}
# 1867
curr=({Cyc_List_imp_rev(curr);});
if((*previous).f2 == 0)
(*previous).f2=curr;}
# 1840
return env;}}case 7U: _LLD: _tmp42B=((struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct*)_tmp42A)->f1;_LLE: {struct Cyc_Absyn_Enumdecl*ed=_tmp42B;
# 1877
(*ed->name).f1=Cyc_Absyn_Loc_n;
# 1879
if((unsigned)ed->fields){
struct Cyc_List_List*fs=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(ed->fields))->v;for(0;(unsigned)fs;fs=fs->tl){
(*((struct Cyc_Absyn_Enumfield*)fs->hd)->name).f1=Cyc_Absyn_Loc_n;
env=({struct Cyc_Port_Cenv*(*_tmp473)(struct Cyc_Port_Cenv*env,struct _tuple0*x,void*t,void*qual,void*ctype,unsigned loc)=Cyc_Port_add_var;struct Cyc_Port_Cenv*_tmp474=env;struct _tuple0*_tmp475=((struct Cyc_Absyn_Enumfield*)fs->hd)->name;void*_tmp476=({Cyc_Absyn_enum_type(ed->name,ed);});void*_tmp477=({Cyc_Port_const_ct();});void*_tmp478=({Cyc_Port_arith_ct();});unsigned _tmp479=0U;_tmp473(_tmp474,_tmp475,_tmp476,_tmp477,_tmp478,_tmp479);});}}
# 1879
return env;}default: _LLF: _LL10:
# 1887
 if((env->gcenv)->porting)
({void*_tmp47A=0U;({struct Cyc___cycFILE*_tmp5EC=Cyc_stderr;struct _fat_ptr _tmp5EB=({const char*_tmp47B="Warning: Cyclone declaration found in code to be ported -- skipping.";_tag_fat(_tmp47B,sizeof(char),69U);});Cyc_fprintf(_tmp5EC,_tmp5EB,_tag_fat(_tmp47A,sizeof(void*),0U));});});
# 1887
return env;}_LL0:;}
# 1894
static struct Cyc_Port_Cenv*Cyc_Port_gen_topdecls(struct Cyc_Port_Cenv*env,struct Cyc_List_List*ds){
for(0;(unsigned)ds;ds=ds->tl){
env=({Cyc_Port_gen_topdecl(env,(struct Cyc_Absyn_Decl*)ds->hd);});}
return env;}
# 1901
static struct Cyc_List_List*Cyc_Port_gen_edits(struct Cyc_List_List*ds){
# 1903
struct Cyc_Port_Cenv*env=({struct Cyc_Port_Cenv*(*_tmp4C4)(struct Cyc_Port_Cenv*env,struct Cyc_List_List*ds)=Cyc_Port_gen_topdecls;struct Cyc_Port_Cenv*_tmp4C5=({Cyc_Port_initial_cenv();});struct Cyc_List_List*_tmp4C6=ds;_tmp4C4(_tmp4C5,_tmp4C6);});
# 1905
struct Cyc_List_List*ptrs=(env->gcenv)->pointer_edits;
struct Cyc_List_List*consts=(env->gcenv)->qualifier_edits;
struct Cyc_List_List*zts=(env->gcenv)->zeroterm_edits;
struct Cyc_List_List*edits=(env->gcenv)->edits;
struct Cyc_List_List*localvars=(env->gcenv)->vardecl_locs;
# 1911
for(0;(unsigned)localvars;localvars=localvars->tl){
struct _tuple17 _stmttmp2D=*((struct _tuple17*)localvars->hd);void*_tmp480;unsigned _tmp47F;struct _tuple0*_tmp47E;_LL1: _tmp47E=_stmttmp2D.f1;_tmp47F=_stmttmp2D.f2;_tmp480=_stmttmp2D.f3;_LL2: {struct _tuple0*var=_tmp47E;unsigned loc=_tmp47F;void*vartype=_tmp480;
struct _tuple0 _stmttmp2E=*var;struct _fat_ptr*_tmp481;_LL4: _tmp481=_stmttmp2E.f2;_LL5: {struct _fat_ptr*x=_tmp481;
struct Cyc_Port_VarUsage*varusage=({({struct Cyc_Port_VarUsage*(*_tmp5EE)(struct Cyc_Hashtable_Table*t,unsigned key)=({struct Cyc_Port_VarUsage*(*_tmp495)(struct Cyc_Hashtable_Table*t,unsigned key)=(struct Cyc_Port_VarUsage*(*)(struct Cyc_Hashtable_Table*t,unsigned key))Cyc_Hashtable_lookup;_tmp495;});struct Cyc_Hashtable_Table*_tmp5ED=(env->gcenv)->varusage_tab;_tmp5EE(_tmp5ED,loc);});});
if(((struct Cyc_Port_VarUsage*)_check_null(varusage))->address_taken){
if((unsigned)varusage->init){
# 1918
edits=({struct Cyc_List_List*_tmp485=_cycalloc(sizeof(*_tmp485));({struct Cyc_Port_Edit*_tmp5F1=({struct Cyc_Port_Edit*_tmp484=_cycalloc(sizeof(*_tmp484));_tmp484->loc=loc,({struct _fat_ptr _tmp5F0=({const char*_tmp482="@";_tag_fat(_tmp482,sizeof(char),2U);});_tmp484->old_string=_tmp5F0;}),({struct _fat_ptr _tmp5EF=({const char*_tmp483="";_tag_fat(_tmp483,sizeof(char),1U);});_tmp484->new_string=_tmp5EF;});_tmp484;});_tmp485->hd=_tmp5F1;}),_tmp485->tl=edits;_tmp485;});
edits=({struct Cyc_List_List*_tmp489=_cycalloc(sizeof(*_tmp489));({struct Cyc_Port_Edit*_tmp5F4=({struct Cyc_Port_Edit*_tmp488=_cycalloc(sizeof(*_tmp488));_tmp488->loc=((struct Cyc_Absyn_Exp*)_check_null(varusage->init))->loc,({struct _fat_ptr _tmp5F3=({const char*_tmp486="new ";_tag_fat(_tmp486,sizeof(char),5U);});_tmp488->old_string=_tmp5F3;}),({struct _fat_ptr _tmp5F2=({const char*_tmp487="";_tag_fat(_tmp487,sizeof(char),1U);});_tmp488->new_string=_tmp5F2;});_tmp488;});_tmp489->hd=_tmp5F4;}),_tmp489->tl=edits;_tmp489;});}else{
# 1923
edits=({struct Cyc_List_List*_tmp48F=_cycalloc(sizeof(*_tmp48F));({struct Cyc_Port_Edit*_tmp5F8=({struct Cyc_Port_Edit*_tmp48E=_cycalloc(sizeof(*_tmp48E));_tmp48E->loc=loc,({struct _fat_ptr _tmp5F7=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp48C=({struct Cyc_String_pa_PrintArg_struct _tmp4FF;_tmp4FF.tag=0U,_tmp4FF.f1=(struct _fat_ptr)((struct _fat_ptr)*x);_tmp4FF;});struct Cyc_String_pa_PrintArg_struct _tmp48D=({struct Cyc_String_pa_PrintArg_struct _tmp4FE;_tmp4FE.tag=0U,({struct _fat_ptr _tmp5F5=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(vartype);}));_tmp4FE.f1=_tmp5F5;});_tmp4FE;});void*_tmp48A[2U];_tmp48A[0]=& _tmp48C,_tmp48A[1]=& _tmp48D;({struct _fat_ptr _tmp5F6=({const char*_tmp48B="?%s = malloc(sizeof(%s))";_tag_fat(_tmp48B,sizeof(char),25U);});Cyc_aprintf(_tmp5F6,_tag_fat(_tmp48A,sizeof(void*),2U));});});_tmp48E->old_string=_tmp5F7;}),_tmp48E->new_string=*x;_tmp48E;});_tmp48F->hd=_tmp5F8;}),_tmp48F->tl=edits;_tmp48F;});}{
# 1925
struct Cyc_List_List*loclist=varusage->usage_locs;
for(0;(unsigned)loclist;loclist=loclist->tl){
edits=({struct Cyc_List_List*_tmp494=_cycalloc(sizeof(*_tmp494));({struct Cyc_Port_Edit*_tmp5FB=({struct Cyc_Port_Edit*_tmp493=_cycalloc(sizeof(*_tmp493));_tmp493->loc=(unsigned)loclist->hd,({struct _fat_ptr _tmp5FA=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp492=({struct Cyc_String_pa_PrintArg_struct _tmp500;_tmp500.tag=0U,_tmp500.f1=(struct _fat_ptr)((struct _fat_ptr)*x);_tmp500;});void*_tmp490[1U];_tmp490[0]=& _tmp492;({struct _fat_ptr _tmp5F9=({const char*_tmp491="(*%s)";_tag_fat(_tmp491,sizeof(char),6U);});Cyc_aprintf(_tmp5F9,_tag_fat(_tmp490,sizeof(void*),1U));});});_tmp493->old_string=_tmp5FA;}),_tmp493->new_string=*x;_tmp493;});_tmp494->hd=_tmp5FB;}),_tmp494->tl=edits;_tmp494;});}}}}}}
# 1933
{struct Cyc_List_List*ps=ptrs;for(0;(unsigned)ps;ps=ps->tl){
struct _tuple18 _stmttmp2F=*((struct _tuple18*)ps->hd);unsigned _tmp498;void*_tmp497;void*_tmp496;_LL7: _tmp496=_stmttmp2F.f1;_tmp497=_stmttmp2F.f2;_tmp498=_stmttmp2F.f3;_LL8: {void*new_ctype=_tmp496;void*orig_ctype=_tmp497;unsigned loc=_tmp498;
({Cyc_Port_unifies(new_ctype,orig_ctype);});}}}
# 1937
{struct Cyc_List_List*cs=consts;for(0;(unsigned)cs;cs=cs->tl){
struct _tuple18 _stmttmp30=*((struct _tuple18*)cs->hd);unsigned _tmp49B;void*_tmp49A;void*_tmp499;_LLA: _tmp499=_stmttmp30.f1;_tmp49A=_stmttmp30.f2;_tmp49B=_stmttmp30.f3;_LLB: {void*new_qual=_tmp499;void*old_qual=_tmp49A;unsigned loc=_tmp49B;
({Cyc_Port_unifies(new_qual,old_qual);});}}}
# 1941
{struct Cyc_List_List*zs=zts;for(0;(unsigned)zs;zs=zs->tl){
struct _tuple18 _stmttmp31=*((struct _tuple18*)zs->hd);unsigned _tmp49E;void*_tmp49D;void*_tmp49C;_LLD: _tmp49C=_stmttmp31.f1;_tmp49D=_stmttmp31.f2;_tmp49E=_stmttmp31.f3;_LLE: {void*new_zt=_tmp49C;void*old_zt=_tmp49D;unsigned loc=_tmp49E;
({Cyc_Port_unifies(new_zt,old_zt);});}}}
# 1947
for(0;(unsigned)ptrs;ptrs=ptrs->tl){
struct _tuple18 _stmttmp32=*((struct _tuple18*)ptrs->hd);unsigned _tmp4A1;void*_tmp4A0;void*_tmp49F;_LL10: _tmp49F=_stmttmp32.f1;_tmp4A0=_stmttmp32.f2;_tmp4A1=_stmttmp32.f3;_LL11: {void*new_ctype=_tmp49F;void*orig_ctype=_tmp4A0;unsigned loc=_tmp4A1;
if(!({Cyc_Port_unifies(new_ctype,orig_ctype);})&&(int)loc){
void*_stmttmp33=({Cyc_Port_compress_ct(orig_ctype);});void*_tmp4A2=_stmttmp33;switch(*((int*)_tmp4A2)){case 2U: _LL13: _LL14:
# 1952
 edits=({struct Cyc_List_List*_tmp4A6=_cycalloc(sizeof(*_tmp4A6));({struct Cyc_Port_Edit*_tmp5FE=({struct Cyc_Port_Edit*_tmp4A5=_cycalloc(sizeof(*_tmp4A5));_tmp4A5->loc=loc,({struct _fat_ptr _tmp5FD=({const char*_tmp4A3="?";_tag_fat(_tmp4A3,sizeof(char),2U);});_tmp4A5->old_string=_tmp5FD;}),({struct _fat_ptr _tmp5FC=({const char*_tmp4A4="*";_tag_fat(_tmp4A4,sizeof(char),2U);});_tmp4A5->new_string=_tmp5FC;});_tmp4A5;});_tmp4A6->hd=_tmp5FE;}),_tmp4A6->tl=edits;_tmp4A6;});
goto _LL12;case 3U: _LL15: _LL16:
# 1955
 edits=({struct Cyc_List_List*_tmp4AA=_cycalloc(sizeof(*_tmp4AA));({struct Cyc_Port_Edit*_tmp601=({struct Cyc_Port_Edit*_tmp4A9=_cycalloc(sizeof(*_tmp4A9));_tmp4A9->loc=loc,({struct _fat_ptr _tmp600=({const char*_tmp4A7="*";_tag_fat(_tmp4A7,sizeof(char),2U);});_tmp4A9->old_string=_tmp600;}),({struct _fat_ptr _tmp5FF=({const char*_tmp4A8="?";_tag_fat(_tmp4A8,sizeof(char),2U);});_tmp4A9->new_string=_tmp5FF;});_tmp4A9;});_tmp4AA->hd=_tmp601;}),_tmp4AA->tl=edits;_tmp4AA;});
goto _LL12;default: _LL17: _LL18:
 goto _LL12;}_LL12:;}}}
# 1962
for(0;(unsigned)consts;consts=consts->tl){
struct _tuple18 _stmttmp34=*((struct _tuple18*)consts->hd);unsigned _tmp4AD;void*_tmp4AC;void*_tmp4AB;_LL1A: _tmp4AB=_stmttmp34.f1;_tmp4AC=_stmttmp34.f2;_tmp4AD=_stmttmp34.f3;_LL1B: {void*new_qual=_tmp4AB;void*old_qual=_tmp4AC;unsigned loc=_tmp4AD;
if(!({Cyc_Port_unifies(new_qual,old_qual);})&&(int)loc){
void*_stmttmp35=({Cyc_Port_compress_ct(old_qual);});void*_tmp4AE=_stmttmp35;switch(*((int*)_tmp4AE)){case 1U: _LL1D: _LL1E:
# 1967
 edits=({struct Cyc_List_List*_tmp4B2=_cycalloc(sizeof(*_tmp4B2));({struct Cyc_Port_Edit*_tmp604=({struct Cyc_Port_Edit*_tmp4B1=_cycalloc(sizeof(*_tmp4B1));_tmp4B1->loc=loc,({struct _fat_ptr _tmp603=({const char*_tmp4AF="const ";_tag_fat(_tmp4AF,sizeof(char),7U);});_tmp4B1->old_string=_tmp603;}),({struct _fat_ptr _tmp602=({const char*_tmp4B0="";_tag_fat(_tmp4B0,sizeof(char),1U);});_tmp4B1->new_string=_tmp602;});_tmp4B1;});_tmp4B2->hd=_tmp604;}),_tmp4B2->tl=edits;_tmp4B2;});goto _LL1C;case 0U: _LL1F: _LL20:
# 1969
 edits=({struct Cyc_List_List*_tmp4B6=_cycalloc(sizeof(*_tmp4B6));({struct Cyc_Port_Edit*_tmp607=({struct Cyc_Port_Edit*_tmp4B5=_cycalloc(sizeof(*_tmp4B5));_tmp4B5->loc=loc,({struct _fat_ptr _tmp606=({const char*_tmp4B3="";_tag_fat(_tmp4B3,sizeof(char),1U);});_tmp4B5->old_string=_tmp606;}),({struct _fat_ptr _tmp605=({const char*_tmp4B4="const ";_tag_fat(_tmp4B4,sizeof(char),7U);});_tmp4B5->new_string=_tmp605;});_tmp4B5;});_tmp4B6->hd=_tmp607;}),_tmp4B6->tl=edits;_tmp4B6;});goto _LL1C;default: _LL21: _LL22:
 goto _LL1C;}_LL1C:;}}}
# 1975
for(0;(unsigned)zts;zts=zts->tl){
struct _tuple18 _stmttmp36=*((struct _tuple18*)zts->hd);unsigned _tmp4B9;void*_tmp4B8;void*_tmp4B7;_LL24: _tmp4B7=_stmttmp36.f1;_tmp4B8=_stmttmp36.f2;_tmp4B9=_stmttmp36.f3;_LL25: {void*new_zt=_tmp4B7;void*old_zt=_tmp4B8;unsigned loc=_tmp4B9;
if(!({Cyc_Port_unifies(new_zt,old_zt);})&&(int)loc){
void*_stmttmp37=({Cyc_Port_compress_ct(old_zt);});void*_tmp4BA=_stmttmp37;switch(*((int*)_tmp4BA)){case 8U: _LL27: _LL28:
# 1980
 edits=({struct Cyc_List_List*_tmp4BE=_cycalloc(sizeof(*_tmp4BE));({struct Cyc_Port_Edit*_tmp60A=({struct Cyc_Port_Edit*_tmp4BD=_cycalloc(sizeof(*_tmp4BD));_tmp4BD->loc=loc,({struct _fat_ptr _tmp609=({const char*_tmp4BB="@nozeroterm ";_tag_fat(_tmp4BB,sizeof(char),13U);});_tmp4BD->old_string=_tmp609;}),({struct _fat_ptr _tmp608=({const char*_tmp4BC="";_tag_fat(_tmp4BC,sizeof(char),1U);});_tmp4BD->new_string=_tmp608;});_tmp4BD;});_tmp4BE->hd=_tmp60A;}),_tmp4BE->tl=edits;_tmp4BE;});goto _LL26;case 9U: _LL29: _LL2A:
# 1982
 edits=({struct Cyc_List_List*_tmp4C2=_cycalloc(sizeof(*_tmp4C2));({struct Cyc_Port_Edit*_tmp60D=({struct Cyc_Port_Edit*_tmp4C1=_cycalloc(sizeof(*_tmp4C1));_tmp4C1->loc=loc,({struct _fat_ptr _tmp60C=({const char*_tmp4BF="@zeroterm ";_tag_fat(_tmp4BF,sizeof(char),11U);});_tmp4C1->old_string=_tmp60C;}),({struct _fat_ptr _tmp60B=({const char*_tmp4C0="";_tag_fat(_tmp4C0,sizeof(char),1U);});_tmp4C1->new_string=_tmp60B;});_tmp4C1;});_tmp4C2->hd=_tmp60D;}),_tmp4C2->tl=edits;_tmp4C2;});goto _LL26;default: _LL2B: _LL2C:
 goto _LL26;}_LL26:;}}}
# 1989
edits=({({struct Cyc_List_List*(*_tmp60E)(int(*cmp)(struct Cyc_Port_Edit*,struct Cyc_Port_Edit*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp4C3)(int(*cmp)(struct Cyc_Port_Edit*,struct Cyc_Port_Edit*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(int(*cmp)(struct Cyc_Port_Edit*,struct Cyc_Port_Edit*),struct Cyc_List_List*x))Cyc_List_merge_sort;_tmp4C3;});_tmp60E(Cyc_Port_cmp_edit,edits);});});
# 1991
while((unsigned)edits &&((struct Cyc_Port_Edit*)edits->hd)->loc == (unsigned)0){
# 1995
edits=edits->tl;}
# 1997
return edits;}
# 2002
void Cyc_Port_port(struct Cyc_List_List*ds){
struct Cyc_List_List*edits=({Cyc_Port_gen_edits(ds);});
struct Cyc_List_List*locs=({({struct Cyc_List_List*(*_tmp60F)(unsigned(*f)(struct Cyc_Port_Edit*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp4D2)(unsigned(*f)(struct Cyc_Port_Edit*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(unsigned(*f)(struct Cyc_Port_Edit*),struct Cyc_List_List*x))Cyc_List_map;_tmp4D2;});_tmp60F(Cyc_Port_get_seg,edits);});});
Cyc_Position_use_gcc_style_location=0;{
struct Cyc_List_List*slocs=({struct Cyc_List_List*(*_tmp4D0)(struct Cyc_List_List*x)=Cyc_List_imp_rev;struct Cyc_List_List*_tmp4D1=({Cyc_Position_strings_of_segments(locs);});_tmp4D0(_tmp4D1);});
for(0;(unsigned)edits;(edits=edits->tl,slocs=slocs->tl)){
struct Cyc_Port_Edit _stmttmp38=*((struct Cyc_Port_Edit*)edits->hd);struct _fat_ptr _tmp4CA;struct _fat_ptr _tmp4C9;unsigned _tmp4C8;_LL1: _tmp4C8=_stmttmp38.loc;_tmp4C9=_stmttmp38.old_string;_tmp4CA=_stmttmp38.new_string;_LL2: {unsigned loc=_tmp4C8;struct _fat_ptr s1=_tmp4C9;struct _fat_ptr s2=_tmp4CA;
struct _fat_ptr sloc=(struct _fat_ptr)*((struct _fat_ptr*)((struct Cyc_List_List*)_check_null(slocs))->hd);
({struct Cyc_String_pa_PrintArg_struct _tmp4CD=({struct Cyc_String_pa_PrintArg_struct _tmp503;_tmp503.tag=0U,_tmp503.f1=(struct _fat_ptr)((struct _fat_ptr)sloc);_tmp503;});struct Cyc_String_pa_PrintArg_struct _tmp4CE=({struct Cyc_String_pa_PrintArg_struct _tmp502;_tmp502.tag=0U,_tmp502.f1=(struct _fat_ptr)((struct _fat_ptr)s1);_tmp502;});struct Cyc_String_pa_PrintArg_struct _tmp4CF=({struct Cyc_String_pa_PrintArg_struct _tmp501;_tmp501.tag=0U,_tmp501.f1=(struct _fat_ptr)((struct _fat_ptr)s2);_tmp501;});void*_tmp4CB[3U];_tmp4CB[0]=& _tmp4CD,_tmp4CB[1]=& _tmp4CE,_tmp4CB[2]=& _tmp4CF;({struct _fat_ptr _tmp610=({const char*_tmp4CC="%s: insert `%s' for `%s'\n";_tag_fat(_tmp4CC,sizeof(char),26U);});Cyc_printf(_tmp610,_tag_fat(_tmp4CB,sizeof(void*),3U));});});}}}}

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
# 95 "core.h"
struct _fat_ptr Cyc_Core_new_string(unsigned);
# 117
void*Cyc_Core_identity(void*);
# 119
int Cyc_Core_intcmp(int,int);extern char Cyc_Core_Invalid_argument[17U];struct Cyc_Core_Invalid_argument_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Failure[8U];struct Cyc_Core_Failure_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Impossible[11U];struct Cyc_Core_Impossible_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Not_found[10U];struct Cyc_Core_Not_found_exn_struct{char*tag;};extern char Cyc_Core_Unreachable[12U];struct Cyc_Core_Unreachable_exn_struct{char*tag;struct _fat_ptr f1;};
# 168
extern struct _RegionHandle*Cyc_Core_unique_region;struct Cyc_Core_DynamicRegion;struct Cyc_Core_NewDynamicRegion{struct Cyc_Core_DynamicRegion*key;};struct Cyc_Core_ThinRes{void*arr;unsigned nelts;};struct Cyc___cycFILE;struct Cyc_String_pa_PrintArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Int_pa_PrintArg_struct{int tag;unsigned long f1;};struct Cyc_Double_pa_PrintArg_struct{int tag;double f1;};struct Cyc_LongDouble_pa_PrintArg_struct{int tag;long double f1;};struct Cyc_ShortPtr_pa_PrintArg_struct{int tag;short*f1;};struct Cyc_IntPtr_pa_PrintArg_struct{int tag;unsigned long*f1;};
# 100 "cycboot.h"
int Cyc_fprintf(struct Cyc___cycFILE*,struct _fat_ptr,struct _fat_ptr);struct Cyc_ShortPtr_sa_ScanfArg_struct{int tag;short*f1;};struct Cyc_UShortPtr_sa_ScanfArg_struct{int tag;unsigned short*f1;};struct Cyc_IntPtr_sa_ScanfArg_struct{int tag;int*f1;};struct Cyc_UIntPtr_sa_ScanfArg_struct{int tag;unsigned*f1;};struct Cyc_StringPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_DoublePtr_sa_ScanfArg_struct{int tag;double*f1;};struct Cyc_FloatPtr_sa_ScanfArg_struct{int tag;float*f1;};struct Cyc_CharPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};extern char Cyc_FileCloseError[15U];struct Cyc_FileCloseError_exn_struct{char*tag;};extern char Cyc_FileOpenError[14U];struct Cyc_FileOpenError_exn_struct{char*tag;struct _fat_ptr f1;};struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};
# 61 "list.h"
int Cyc_List_length(struct Cyc_List_List*x);extern char Cyc_List_List_mismatch[14U];struct Cyc_List_List_mismatch_exn_struct{char*tag;};extern char Cyc_List_Nth[4U];struct Cyc_List_Nth_exn_struct{char*tag;};
# 38 "string.h"
unsigned long Cyc_strlen(struct _fat_ptr s);
# 54 "string.h"
int Cyc_zstrptrcmp(struct _fat_ptr*,struct _fat_ptr*);
# 66
struct _fat_ptr Cyc_str_sepstr(struct Cyc_List_List*,struct _fat_ptr);struct Cyc_Hashtable_Table;
# 39 "hashtable.h"
struct Cyc_Hashtable_Table*Cyc_Hashtable_create(int sz,int(*cmp)(void*,void*),int(*hash)(void*));
# 50
void Cyc_Hashtable_insert(struct Cyc_Hashtable_Table*t,void*key,void*val);
# 52
void*Cyc_Hashtable_lookup(struct Cyc_Hashtable_Table*t,void*key);
# 78
int Cyc_Hashtable_hash_stringptr(struct _fat_ptr*p);struct Cyc_Fn_Function{void*(*f)(void*,void*);void*env;};
# 48 "fn.h"
struct Cyc_Fn_Function*Cyc_Fn_make_fn(void*(*f)(void*,void*),void*x);
# 51
struct Cyc_Fn_Function*Cyc_Fn_fp2fn(void*(*f)(void*));
# 54
void*Cyc_Fn_apply(struct Cyc_Fn_Function*f,void*x);
# 39 "pp.h"
extern int Cyc_PP_tex_output;struct Cyc_PP_Ppstate;struct Cyc_PP_Out;struct Cyc_PP_Doc;
# 67 "pp.h"
struct Cyc_PP_Doc*Cyc_PP_nil_doc();
# 72
struct Cyc_PP_Doc*Cyc_PP_line_doc();
# 74
struct Cyc_PP_Doc*Cyc_PP_oline_doc();
# 78
struct Cyc_PP_Doc*Cyc_PP_text(struct _fat_ptr s);
# 94
struct Cyc_PP_Doc*Cyc_PP_cat(struct _fat_ptr);
# 97
struct Cyc_PP_Doc*Cyc_PP_cats(struct Cyc_List_List*doclist);
# 103
struct Cyc_PP_Doc*Cyc_PP_doc_union(struct Cyc_PP_Doc*d1,struct Cyc_PP_Doc*d2);
# 105
struct Cyc_PP_Doc*Cyc_PP_tab(struct Cyc_PP_Doc*d);
# 108
struct Cyc_PP_Doc*Cyc_PP_seq(struct _fat_ptr sep,struct Cyc_List_List*l);
# 117
struct Cyc_PP_Doc*Cyc_PP_seql(struct _fat_ptr sep,struct Cyc_List_List*l0);struct Cyc_Xarray_Xarray{struct _fat_ptr elmts;int num_elmts;};
# 40 "xarray.h"
int Cyc_Xarray_length(struct Cyc_Xarray_Xarray*);
# 42
void*Cyc_Xarray_get(struct Cyc_Xarray_Xarray*,int);
# 45
void Cyc_Xarray_set(struct Cyc_Xarray_Xarray*,int,void*);
# 48
struct Cyc_Xarray_Xarray*Cyc_Xarray_create(int,void*);
# 66
void Cyc_Xarray_add(struct Cyc_Xarray_Xarray*,void*);
# 121
void Cyc_Xarray_reuse(struct Cyc_Xarray_Xarray*xarr);struct Cyc_PP_Empty_PP_Alist_struct{int tag;int f1;};struct Cyc_PP_Single_PP_Alist_struct{int tag;void*f1;};struct Cyc_PP_Append_PP_Alist_struct{int tag;void*f1;void*f2;};
# 51 "pp.cyc"
struct Cyc_PP_Empty_PP_Alist_struct Cyc_PP_Empty_stringptr={0U,0};
struct Cyc_PP_Empty_PP_Alist_struct Cyc_PP_Empty_link={0U,0};struct _tuple0{void*f1;void*f2;};
# 54
void*Cyc_PP_append(void*a1,void*a2){
struct _tuple0 _stmttmp0=({struct _tuple0 _tmpF4;_tmpF4.f1=a1,_tmpF4.f2=a2;_tmpF4;});struct _tuple0 _tmp0=_stmttmp0;if(((struct Cyc_PP_Empty_PP_Alist_struct*)_tmp0.f1)->tag == 0U){_LL1: _LL2:
 return a2;}else{if(((struct Cyc_PP_Empty_PP_Alist_struct*)_tmp0.f2)->tag == 0U){_LL3: _LL4:
 return a1;}else{_LL5: _LL6:
 return(void*)({struct Cyc_PP_Append_PP_Alist_struct*_tmp1=_cycalloc(sizeof(*_tmp1));_tmp1->tag=2U,_tmp1->f1=a1,_tmp1->f2=a2;_tmp1;});}}_LL0:;}
# 62
struct Cyc_List_List*Cyc_PP_list_of_alist_f(void*y,struct Cyc_List_List*l){
void*_tmp3=y;void*_tmp5;void*_tmp4;void*_tmp6;switch(*((int*)_tmp3)){case 0U: _LL1: _LL2:
 return l;case 1U: _LL3: _tmp6=(void*)((struct Cyc_PP_Single_PP_Alist_struct*)_tmp3)->f1;_LL4: {void*z=_tmp6;
return({struct Cyc_List_List*_tmp7=_cycalloc(sizeof(*_tmp7));_tmp7->hd=z,_tmp7->tl=l;_tmp7;});}default: _LL5: _tmp4=(void*)((struct Cyc_PP_Append_PP_Alist_struct*)_tmp3)->f1;_tmp5=(void*)((struct Cyc_PP_Append_PP_Alist_struct*)_tmp3)->f2;_LL6: {void*a1=_tmp4;void*a2=_tmp5;
return({struct Cyc_List_List*(*_tmp8)(void*y,struct Cyc_List_List*l)=Cyc_PP_list_of_alist_f;void*_tmp9=a1;struct Cyc_List_List*_tmpA=({Cyc_PP_list_of_alist_f(a2,l);});_tmp8(_tmp9,_tmpA);});}}_LL0:;}
# 69
struct Cyc_List_List*Cyc_PP_list_of_alist(void*x){
return({Cyc_PP_list_of_alist_f(x,0);});}struct Cyc_PP_Ppstate{int ci;int cc;int cl;int pw;int epw;};struct Cyc_PP_Out{int newcc;int newcl;void*ppout;void*links;};struct Cyc_PP_Doc{int mwo;int mw;struct Cyc_Fn_Function*f;};
# 98
static void Cyc_PP_dump_out(struct Cyc___cycFILE*f,void*al){
struct Cyc_Xarray_Xarray*xarr=({Cyc_Xarray_create(16,al);});
({Cyc_Xarray_add(xarr,al);});{
int last=0;
while(last >= 0){
void*_stmttmp1=({Cyc_Xarray_get(xarr,last);});void*_tmpD=_stmttmp1;void*_tmpF;void*_tmpE;struct _fat_ptr*_tmp10;switch(*((int*)_tmpD)){case 0U: _LL1: _LL2:
 -- last;goto _LL0;case 1U: _LL3: _tmp10=(struct _fat_ptr*)((struct Cyc_PP_Single_PP_Alist_struct*)_tmpD)->f1;_LL4: {struct _fat_ptr*s=_tmp10;
-- last;({struct Cyc_String_pa_PrintArg_struct _tmp13=({struct Cyc_String_pa_PrintArg_struct _tmpF5;_tmpF5.tag=0U,_tmpF5.f1=(struct _fat_ptr)((struct _fat_ptr)*s);_tmpF5;});void*_tmp11[1U];_tmp11[0]=& _tmp13;({struct Cyc___cycFILE*_tmpF7=f;struct _fat_ptr _tmpF6=({const char*_tmp12="%s";_tag_fat(_tmp12,sizeof(char),3U);});Cyc_fprintf(_tmpF7,_tmpF6,_tag_fat(_tmp11,sizeof(void*),1U));});});goto _LL0;}default: _LL5: _tmpE=(void*)((struct Cyc_PP_Append_PP_Alist_struct*)_tmpD)->f1;_tmpF=(void*)((struct Cyc_PP_Append_PP_Alist_struct*)_tmpD)->f2;_LL6: {void*a1=_tmpE;void*a2=_tmpF;
# 107
({Cyc_Xarray_set(xarr,last,a2);});
if(({int _tmpF8=last;_tmpF8 == ({Cyc_Xarray_length(xarr);})- 1;}))
({Cyc_Xarray_add(xarr,a1);});else{
# 111
({Cyc_Xarray_set(xarr,last + 1,a1);});}
++ last;
goto _LL0;}}_LL0:;}
# 116
({Cyc_Xarray_reuse(xarr);});}}
# 120
void Cyc_PP_file_of_doc(struct Cyc_PP_Doc*d,int w,struct Cyc___cycFILE*f){
struct Cyc_PP_Out*o=({({struct Cyc_PP_Out*(*_tmpFA)(struct Cyc_Fn_Function*f,struct Cyc_PP_Ppstate*x)=({struct Cyc_PP_Out*(*_tmp15)(struct Cyc_Fn_Function*f,struct Cyc_PP_Ppstate*x)=(struct Cyc_PP_Out*(*)(struct Cyc_Fn_Function*f,struct Cyc_PP_Ppstate*x))Cyc_Fn_apply;_tmp15;});struct Cyc_Fn_Function*_tmpF9=d->f;_tmpFA(_tmpF9,({struct Cyc_PP_Ppstate*_tmp16=_cycalloc(sizeof(*_tmp16));_tmp16->ci=0,_tmp16->cc=0,_tmp16->cl=1,_tmp16->pw=w,_tmp16->epw=w;_tmp16;}));});});
({Cyc_PP_dump_out(f,o->ppout);});}
# 120
struct _fat_ptr Cyc_PP_string_of_doc(struct Cyc_PP_Doc*d,int w){
# 127
struct Cyc_PP_Out*o=({({struct Cyc_PP_Out*(*_tmpFC)(struct Cyc_Fn_Function*f,struct Cyc_PP_Ppstate*x)=({struct Cyc_PP_Out*(*_tmp1C)(struct Cyc_Fn_Function*f,struct Cyc_PP_Ppstate*x)=(struct Cyc_PP_Out*(*)(struct Cyc_Fn_Function*f,struct Cyc_PP_Ppstate*x))Cyc_Fn_apply;_tmp1C;});struct Cyc_Fn_Function*_tmpFB=d->f;_tmpFC(_tmpFB,({struct Cyc_PP_Ppstate*_tmp1D=_cycalloc(sizeof(*_tmp1D));_tmp1D->ci=0,_tmp1D->cc=0,_tmp1D->cl=1,_tmp1D->pw=w,_tmp1D->epw=w;_tmp1D;}));});});
return(struct _fat_ptr)({struct _fat_ptr(*_tmp18)(struct Cyc_List_List*,struct _fat_ptr)=Cyc_str_sepstr;struct Cyc_List_List*_tmp19=({Cyc_PP_list_of_alist(o->ppout);});struct _fat_ptr _tmp1A=({const char*_tmp1B="";_tag_fat(_tmp1B,sizeof(char),1U);});_tmp18(_tmp19,_tmp1A);});}struct _tuple1{struct _fat_ptr f1;struct Cyc_List_List*f2;};
# 133
struct _tuple1*Cyc_PP_string_and_links(struct Cyc_PP_Doc*d,int w){
struct Cyc_PP_Out*o=({({struct Cyc_PP_Out*(*_tmpFE)(struct Cyc_Fn_Function*f,struct Cyc_PP_Ppstate*x)=({struct Cyc_PP_Out*(*_tmp24)(struct Cyc_Fn_Function*f,struct Cyc_PP_Ppstate*x)=(struct Cyc_PP_Out*(*)(struct Cyc_Fn_Function*f,struct Cyc_PP_Ppstate*x))Cyc_Fn_apply;_tmp24;});struct Cyc_Fn_Function*_tmpFD=d->f;_tmpFE(_tmpFD,({struct Cyc_PP_Ppstate*_tmp25=_cycalloc(sizeof(*_tmp25));_tmp25->ci=0,_tmp25->cc=0,_tmp25->cl=1,_tmp25->pw=w,_tmp25->epw=w;_tmp25;}));});});
return({struct _tuple1*_tmp23=_cycalloc(sizeof(*_tmp23));({struct _fat_ptr _tmp100=(struct _fat_ptr)({struct _fat_ptr(*_tmp1F)(struct Cyc_List_List*,struct _fat_ptr)=Cyc_str_sepstr;struct Cyc_List_List*_tmp20=({Cyc_PP_list_of_alist(o->ppout);});struct _fat_ptr _tmp21=({const char*_tmp22="";_tag_fat(_tmp22,sizeof(char),1U);});_tmp1F(_tmp20,_tmp21);});_tmp23->f1=_tmp100;}),({
# 137
struct Cyc_List_List*_tmpFF=({Cyc_PP_list_of_alist(o->links);});_tmp23->f2=_tmpFF;});_tmp23;});}
# 133
static struct Cyc_Core_Opt*Cyc_PP_bhashtbl=0;
# 142
int Cyc_PP_tex_output=0;
# 144
struct _fat_ptr Cyc_PP_nlblanks(int i){
if(Cyc_PP_bhashtbl == 0)
Cyc_PP_bhashtbl=({struct Cyc_Core_Opt*_tmp29=_cycalloc(sizeof(*_tmp29));({struct Cyc_Hashtable_Table*_tmp102=({({struct Cyc_Hashtable_Table*(*_tmp101)(int sz,int(*cmp)(int,int),int(*hash)(int))=({struct Cyc_Hashtable_Table*(*_tmp27)(int sz,int(*cmp)(int,int),int(*hash)(int))=(struct Cyc_Hashtable_Table*(*)(int sz,int(*cmp)(int,int),int(*hash)(int)))Cyc_Hashtable_create;_tmp27;});_tmp101(61,Cyc_Core_intcmp,({int(*_tmp28)(int)=(int(*)(int))Cyc_Core_identity;_tmp28;}));});});_tmp29->v=_tmp102;});_tmp29;});
# 145
if(i < 0)
# 147
(int)_throw(({struct Cyc_Core_Failure_exn_struct*_tmp2B=_cycalloc(sizeof(*_tmp2B));_tmp2B->tag=Cyc_Core_Failure,({struct _fat_ptr _tmp103=({const char*_tmp2A="nlblanks";_tag_fat(_tmp2A,sizeof(char),9U);});_tmp2B->f1=_tmp103;});_tmp2B;}));{
# 145
struct _handler_cons _tmp2C;_push_handler(& _tmp2C);{int _tmp2E=0;if(setjmp(_tmp2C.handler))_tmp2E=1;if(!_tmp2E){
# 149
{struct _fat_ptr _tmp30=*({({struct _fat_ptr*(*_tmp105)(struct Cyc_Hashtable_Table*t,int key)=({struct _fat_ptr*(*_tmp2F)(struct Cyc_Hashtable_Table*t,int key)=(struct _fat_ptr*(*)(struct Cyc_Hashtable_Table*t,int key))Cyc_Hashtable_lookup;_tmp2F;});struct Cyc_Hashtable_Table*_tmp104=(struct Cyc_Hashtable_Table*)((struct Cyc_Core_Opt*)_check_null(Cyc_PP_bhashtbl))->v;_tmp105(_tmp104,i);});});_npop_handler(0U);return _tmp30;};_pop_handler();}else{void*_tmp2D=(void*)Cyc_Core_get_exn_thrown();void*_tmp31=_tmp2D;void*_tmp32;if(((struct Cyc_Core_Not_found_exn_struct*)_tmp31)->tag == Cyc_Core_Not_found){_LL1: _LL2:
# 152
 if(!Cyc_PP_tex_output){
int num_tabs=i / 8;
int num_spaces=i % 8;
int total=(2 + num_tabs)+ num_spaces;
struct _fat_ptr nlb=({Cyc_Core_new_string((unsigned)total);});
({struct _fat_ptr _tmp33=_fat_ptr_plus(nlb,sizeof(char),0);char _tmp34=*((char*)_check_fat_subscript(_tmp33,sizeof(char),0U));char _tmp35='\n';if(_get_fat_size(_tmp33,sizeof(char))== 1U &&(_tmp34 == 0 && _tmp35 != 0))_throw_arraybounds();*((char*)_tmp33.curr)=_tmp35;});
{int j=0;for(0;j < num_tabs;++ j){
({struct _fat_ptr _tmp36=_fat_ptr_plus(nlb,sizeof(char),j + 1);char _tmp37=*((char*)_check_fat_subscript(_tmp36,sizeof(char),0U));char _tmp38='\t';if(_get_fat_size(_tmp36,sizeof(char))== 1U &&(_tmp37 == 0 && _tmp38 != 0))_throw_arraybounds();*((char*)_tmp36.curr)=_tmp38;});}}
{int j=0;for(0;j < num_spaces;++ j){
({struct _fat_ptr _tmp39=_fat_ptr_plus(nlb,sizeof(char),(j + 1)+ num_tabs);char _tmp3A=*((char*)_check_fat_subscript(_tmp39,sizeof(char),0U));char _tmp3B=' ';if(_get_fat_size(_tmp39,sizeof(char))== 1U &&(_tmp3A == 0 && _tmp3B != 0))_throw_arraybounds();*((char*)_tmp39.curr)=_tmp3B;});}}
({({void(*_tmp108)(struct Cyc_Hashtable_Table*t,int key,struct _fat_ptr*val)=({void(*_tmp3C)(struct Cyc_Hashtable_Table*t,int key,struct _fat_ptr*val)=(void(*)(struct Cyc_Hashtable_Table*t,int key,struct _fat_ptr*val))Cyc_Hashtable_insert;_tmp3C;});struct Cyc_Hashtable_Table*_tmp107=(struct Cyc_Hashtable_Table*)((struct Cyc_Core_Opt*)_check_null(Cyc_PP_bhashtbl))->v;int _tmp106=i;_tmp108(_tmp107,_tmp106,({unsigned _tmp3E=1;struct _fat_ptr*_tmp3D=_cycalloc(_check_times(_tmp3E,sizeof(struct _fat_ptr)));_tmp3D[0]=(struct _fat_ptr)nlb;_tmp3D;}));});});
return(struct _fat_ptr)nlb;}else{
# 168
int total=3 + i;
struct _fat_ptr nlb=({Cyc_Core_new_string((unsigned)(total + 1));});
({struct _fat_ptr _tmp3F=_fat_ptr_plus(nlb,sizeof(char),0);char _tmp40=*((char*)_check_fat_subscript(_tmp3F,sizeof(char),0U));char _tmp41='\\';if(_get_fat_size(_tmp3F,sizeof(char))== 1U &&(_tmp40 == 0 && _tmp41 != 0))_throw_arraybounds();*((char*)_tmp3F.curr)=_tmp41;});
({struct _fat_ptr _tmp42=_fat_ptr_plus(nlb,sizeof(char),1);char _tmp43=*((char*)_check_fat_subscript(_tmp42,sizeof(char),0U));char _tmp44='\\';if(_get_fat_size(_tmp42,sizeof(char))== 1U &&(_tmp43 == 0 && _tmp44 != 0))_throw_arraybounds();*((char*)_tmp42.curr)=_tmp44;});
({struct _fat_ptr _tmp45=_fat_ptr_plus(nlb,sizeof(char),2);char _tmp46=*((char*)_check_fat_subscript(_tmp45,sizeof(char),0U));char _tmp47='\n';if(_get_fat_size(_tmp45,sizeof(char))== 1U &&(_tmp46 == 0 && _tmp47 != 0))_throw_arraybounds();*((char*)_tmp45.curr)=_tmp47;});
{int j=3;for(0;j < total;++ j){
({struct _fat_ptr _tmp48=_fat_ptr_plus(nlb,sizeof(char),j);char _tmp49=*((char*)_check_fat_subscript(_tmp48,sizeof(char),0U));char _tmp4A='~';if(_get_fat_size(_tmp48,sizeof(char))== 1U &&(_tmp49 == 0 && _tmp4A != 0))_throw_arraybounds();*((char*)_tmp48.curr)=_tmp4A;});}}
({({void(*_tmp10B)(struct Cyc_Hashtable_Table*t,int key,struct _fat_ptr*val)=({void(*_tmp4B)(struct Cyc_Hashtable_Table*t,int key,struct _fat_ptr*val)=(void(*)(struct Cyc_Hashtable_Table*t,int key,struct _fat_ptr*val))Cyc_Hashtable_insert;_tmp4B;});struct Cyc_Hashtable_Table*_tmp10A=(struct Cyc_Hashtable_Table*)((struct Cyc_Core_Opt*)_check_null(Cyc_PP_bhashtbl))->v;int _tmp109=i;_tmp10B(_tmp10A,_tmp109,({unsigned _tmp4D=1;struct _fat_ptr*_tmp4C=_cycalloc(_check_times(_tmp4D,sizeof(struct _fat_ptr)));_tmp4C[0]=(struct _fat_ptr)nlb;_tmp4C;}));});});
return(struct _fat_ptr)nlb;}}else{_LL3: _tmp32=_tmp31;_LL4: {void*exn=_tmp32;(int)_rethrow(exn);}}_LL0:;}}}}
# 144
static struct Cyc_Core_Opt*Cyc_PP_str_hashtbl=0;
# 184
int Cyc_PP_infinity=9999999;struct _tuple2{int f1;struct _fat_ptr f2;};
# 186
static struct Cyc_PP_Out*Cyc_PP_text_doc_f(struct _tuple2*clo,struct Cyc_PP_Ppstate*st){
struct _fat_ptr _tmp50;int _tmp4F;_LL1: _tmp4F=clo->f1;_tmp50=clo->f2;_LL2: {int slen=_tmp4F;struct _fat_ptr s=_tmp50;
return({struct Cyc_PP_Out*_tmp54=_cycalloc(sizeof(*_tmp54));_tmp54->newcc=st->cc + slen,_tmp54->newcl=st->cl,({
# 190
void*_tmp10D=(void*)({struct Cyc_PP_Single_PP_Alist_struct*_tmp53=_cycalloc(sizeof(*_tmp53));_tmp53->tag=1U,({struct _fat_ptr*_tmp10C=({unsigned _tmp52=1;struct _fat_ptr*_tmp51=_cycalloc(_check_times(_tmp52,sizeof(struct _fat_ptr)));_tmp51[0]=s;_tmp51;});_tmp53->f1=_tmp10C;});_tmp53;});_tmp54->ppout=_tmp10D;}),_tmp54->links=(void*)& Cyc_PP_Empty_link;_tmp54;});}}
# 193
static struct Cyc_PP_Doc*Cyc_PP_text_doc(struct _fat_ptr s){
int slen=(int)({Cyc_strlen((struct _fat_ptr)s);});
return({struct Cyc_PP_Doc*_tmp58=_cycalloc(sizeof(*_tmp58));
_tmp58->mwo=slen,_tmp58->mw=Cyc_PP_infinity,({
# 198
struct Cyc_Fn_Function*_tmp10F=({({struct Cyc_Fn_Function*(*_tmp10E)(struct Cyc_PP_Out*(*f)(struct _tuple2*,struct Cyc_PP_Ppstate*),struct _tuple2*x)=({struct Cyc_Fn_Function*(*_tmp56)(struct Cyc_PP_Out*(*f)(struct _tuple2*,struct Cyc_PP_Ppstate*),struct _tuple2*x)=(struct Cyc_Fn_Function*(*)(struct Cyc_PP_Out*(*f)(struct _tuple2*,struct Cyc_PP_Ppstate*),struct _tuple2*x))Cyc_Fn_make_fn;_tmp56;});_tmp10E(Cyc_PP_text_doc_f,({struct _tuple2*_tmp57=_cycalloc(sizeof(*_tmp57));_tmp57->f1=slen,_tmp57->f2=s;_tmp57;}));});});_tmp58->f=_tmp10F;});_tmp58;});}
# 193
struct Cyc_PP_Doc*Cyc_PP_text(struct _fat_ptr s){
# 203
struct Cyc_Hashtable_Table*t;
if(Cyc_PP_str_hashtbl == 0){
t=({({struct Cyc_Hashtable_Table*(*_tmp5A)(int sz,int(*cmp)(struct _fat_ptr*,struct _fat_ptr*),int(*hash)(struct _fat_ptr*))=(struct Cyc_Hashtable_Table*(*)(int sz,int(*cmp)(struct _fat_ptr*,struct _fat_ptr*),int(*hash)(struct _fat_ptr*)))Cyc_Hashtable_create;_tmp5A;})(101,Cyc_zstrptrcmp,Cyc_Hashtable_hash_stringptr);});
Cyc_PP_str_hashtbl=({struct Cyc_Core_Opt*_tmp5B=_cycalloc(sizeof(*_tmp5B));_tmp5B->v=t;_tmp5B;});}else{
# 208
t=(struct Cyc_Hashtable_Table*)((struct Cyc_Core_Opt*)_check_null(Cyc_PP_str_hashtbl))->v;}{
# 210
struct _handler_cons _tmp5C;_push_handler(& _tmp5C);{int _tmp5E=0;if(setjmp(_tmp5C.handler))_tmp5E=1;if(!_tmp5E){
{struct Cyc_PP_Doc*_tmp61=({({struct Cyc_PP_Doc*(*_tmp111)(struct Cyc_Hashtable_Table*t,struct _fat_ptr*key)=({struct Cyc_PP_Doc*(*_tmp5F)(struct Cyc_Hashtable_Table*t,struct _fat_ptr*key)=(struct Cyc_PP_Doc*(*)(struct Cyc_Hashtable_Table*t,struct _fat_ptr*key))Cyc_Hashtable_lookup;_tmp5F;});struct Cyc_Hashtable_Table*_tmp110=t;_tmp111(_tmp110,({struct _fat_ptr*_tmp60=_cycalloc(sizeof(*_tmp60));*_tmp60=s;_tmp60;}));});});_npop_handler(0U);return _tmp61;};_pop_handler();}else{void*_tmp5D=(void*)Cyc_Core_get_exn_thrown();void*_tmp62=_tmp5D;void*_tmp63;if(((struct Cyc_Core_Not_found_exn_struct*)_tmp62)->tag == Cyc_Core_Not_found){_LL1: _LL2: {
# 214
struct Cyc_PP_Doc*d=({Cyc_PP_text_doc(s);});
({({void(*_tmp114)(struct Cyc_Hashtable_Table*t,struct _fat_ptr*key,struct Cyc_PP_Doc*val)=({void(*_tmp64)(struct Cyc_Hashtable_Table*t,struct _fat_ptr*key,struct Cyc_PP_Doc*val)=(void(*)(struct Cyc_Hashtable_Table*t,struct _fat_ptr*key,struct Cyc_PP_Doc*val))Cyc_Hashtable_insert;_tmp64;});struct Cyc_Hashtable_Table*_tmp113=t;struct _fat_ptr*_tmp112=({struct _fat_ptr*_tmp65=_cycalloc(sizeof(*_tmp65));*_tmp65=s;_tmp65;});_tmp114(_tmp113,_tmp112,d);});});
return d;}}else{_LL3: _tmp63=_tmp62;_LL4: {void*exn=_tmp63;(int)_rethrow(exn);}}_LL0:;}}}}
# 193
struct Cyc_PP_Doc*Cyc_PP_textptr(struct _fat_ptr*s){
# 219
return({Cyc_PP_text(*s);});}
# 224
static struct Cyc_PP_Doc*Cyc_PP_text_width_doc(struct _fat_ptr s,int slen){
return({struct Cyc_PP_Doc*_tmp6A=_cycalloc(sizeof(*_tmp6A));
_tmp6A->mwo=slen,_tmp6A->mw=Cyc_PP_infinity,({
# 228
struct Cyc_Fn_Function*_tmp116=({({struct Cyc_Fn_Function*(*_tmp115)(struct Cyc_PP_Out*(*f)(struct _tuple2*,struct Cyc_PP_Ppstate*),struct _tuple2*x)=({struct Cyc_Fn_Function*(*_tmp68)(struct Cyc_PP_Out*(*f)(struct _tuple2*,struct Cyc_PP_Ppstate*),struct _tuple2*x)=(struct Cyc_Fn_Function*(*)(struct Cyc_PP_Out*(*f)(struct _tuple2*,struct Cyc_PP_Ppstate*),struct _tuple2*x))Cyc_Fn_make_fn;_tmp68;});_tmp115(Cyc_PP_text_doc_f,({struct _tuple2*_tmp69=_cycalloc(sizeof(*_tmp69));_tmp69->f1=slen,_tmp69->f2=s;_tmp69;}));});});_tmp6A->f=_tmp116;});_tmp6A;});}
# 224
static struct Cyc_Core_Opt*Cyc_PP_str_hashtbl2=0;
# 233
struct Cyc_PP_Doc*Cyc_PP_text_width(struct _fat_ptr s,int slen){
struct Cyc_Hashtable_Table*t;
if(Cyc_PP_str_hashtbl2 == 0){
t=({({struct Cyc_Hashtable_Table*(*_tmp6C)(int sz,int(*cmp)(struct _fat_ptr*,struct _fat_ptr*),int(*hash)(struct _fat_ptr*))=(struct Cyc_Hashtable_Table*(*)(int sz,int(*cmp)(struct _fat_ptr*,struct _fat_ptr*),int(*hash)(struct _fat_ptr*)))Cyc_Hashtable_create;_tmp6C;})(101,Cyc_zstrptrcmp,Cyc_Hashtable_hash_stringptr);});
Cyc_PP_str_hashtbl2=({struct Cyc_Core_Opt*_tmp6D=_cycalloc(sizeof(*_tmp6D));_tmp6D->v=t;_tmp6D;});}else{
# 239
t=(struct Cyc_Hashtable_Table*)((struct Cyc_Core_Opt*)_check_null(Cyc_PP_str_hashtbl2))->v;}{
# 241
struct _handler_cons _tmp6E;_push_handler(& _tmp6E);{int _tmp70=0;if(setjmp(_tmp6E.handler))_tmp70=1;if(!_tmp70){
{struct Cyc_PP_Doc*_tmp73=({({struct Cyc_PP_Doc*(*_tmp118)(struct Cyc_Hashtable_Table*t,struct _fat_ptr*key)=({struct Cyc_PP_Doc*(*_tmp71)(struct Cyc_Hashtable_Table*t,struct _fat_ptr*key)=(struct Cyc_PP_Doc*(*)(struct Cyc_Hashtable_Table*t,struct _fat_ptr*key))Cyc_Hashtable_lookup;_tmp71;});struct Cyc_Hashtable_Table*_tmp117=t;_tmp118(_tmp117,({struct _fat_ptr*_tmp72=_cycalloc(sizeof(*_tmp72));*_tmp72=s;_tmp72;}));});});_npop_handler(0U);return _tmp73;};_pop_handler();}else{void*_tmp6F=(void*)Cyc_Core_get_exn_thrown();void*_tmp74=_tmp6F;void*_tmp75;if(((struct Cyc_Core_Not_found_exn_struct*)_tmp74)->tag == Cyc_Core_Not_found){_LL1: _LL2: {
# 245
struct Cyc_PP_Doc*d=({Cyc_PP_text_width_doc(s,slen);});
({({void(*_tmp11B)(struct Cyc_Hashtable_Table*t,struct _fat_ptr*key,struct Cyc_PP_Doc*val)=({void(*_tmp76)(struct Cyc_Hashtable_Table*t,struct _fat_ptr*key,struct Cyc_PP_Doc*val)=(void(*)(struct Cyc_Hashtable_Table*t,struct _fat_ptr*key,struct Cyc_PP_Doc*val))Cyc_Hashtable_insert;_tmp76;});struct Cyc_Hashtable_Table*_tmp11A=t;struct _fat_ptr*_tmp119=({struct _fat_ptr*_tmp77=_cycalloc(sizeof(*_tmp77));*_tmp77=s;_tmp77;});_tmp11B(_tmp11A,_tmp119,d);});});
return d;}}else{_LL3: _tmp75=_tmp74;_LL4: {void*exn=_tmp75;(int)_rethrow(exn);}}_LL0:;}}}}
# 233
struct Cyc_Core_Opt*Cyc_PP_nil_doc_opt=0;
# 252
struct Cyc_Core_Opt*Cyc_PP_blank_doc_opt=0;
struct Cyc_Core_Opt*Cyc_PP_line_doc_opt=0;
# 257
struct Cyc_PP_Doc*Cyc_PP_nil_doc(){
if(Cyc_PP_nil_doc_opt == 0)
Cyc_PP_nil_doc_opt=({struct Cyc_Core_Opt*_tmp7A=_cycalloc(sizeof(*_tmp7A));({struct Cyc_PP_Doc*_tmp11C=({Cyc_PP_text(({const char*_tmp79="";_tag_fat(_tmp79,sizeof(char),1U);}));});_tmp7A->v=_tmp11C;});_tmp7A;});
# 258
return(struct Cyc_PP_Doc*)((struct Cyc_Core_Opt*)_check_null(Cyc_PP_nil_doc_opt))->v;}
# 257
struct Cyc_PP_Doc*Cyc_PP_blank_doc(){
# 265
if(Cyc_PP_blank_doc_opt == 0)
Cyc_PP_blank_doc_opt=({struct Cyc_Core_Opt*_tmp7D=_cycalloc(sizeof(*_tmp7D));({struct Cyc_PP_Doc*_tmp11D=({Cyc_PP_text(({const char*_tmp7C="";_tag_fat(_tmp7C,sizeof(char),1U);}));});_tmp7D->v=_tmp11D;});_tmp7D;});
# 265
return(struct Cyc_PP_Doc*)((struct Cyc_Core_Opt*)_check_null(Cyc_PP_blank_doc_opt))->v;}struct _tuple3{int f1;struct _fat_ptr f2;struct _fat_ptr f3;};struct _tuple4{int f1;int f2;int f3;struct _fat_ptr f4;};
# 270
static struct Cyc_PP_Out*Cyc_PP_hyperlink_f(struct _tuple3*clo,struct Cyc_PP_Ppstate*st){
struct _fat_ptr _tmp81;struct _fat_ptr _tmp80;int _tmp7F;_LL1: _tmp7F=clo->f1;_tmp80=clo->f2;_tmp81=clo->f3;_LL2: {int slen=_tmp7F;struct _fat_ptr shrt=_tmp80;struct _fat_ptr full=_tmp81;
return({struct Cyc_PP_Out*_tmp87=_cycalloc(sizeof(*_tmp87));_tmp87->newcc=st->cc + slen,_tmp87->newcl=st->cl,({
# 274
void*_tmp121=(void*)({struct Cyc_PP_Single_PP_Alist_struct*_tmp84=_cycalloc(sizeof(*_tmp84));_tmp84->tag=1U,({struct _fat_ptr*_tmp120=({unsigned _tmp83=1;struct _fat_ptr*_tmp82=_cycalloc(_check_times(_tmp83,sizeof(struct _fat_ptr)));_tmp82[0]=shrt;_tmp82;});_tmp84->f1=_tmp120;});_tmp84;});_tmp87->ppout=_tmp121;}),({
void*_tmp11F=(void*)({struct Cyc_PP_Single_PP_Alist_struct*_tmp86=_cycalloc(sizeof(*_tmp86));_tmp86->tag=1U,({struct _tuple4*_tmp11E=({struct _tuple4*_tmp85=_cycalloc(sizeof(*_tmp85));_tmp85->f1=st->cl,_tmp85->f2=st->cc,_tmp85->f3=slen,_tmp85->f4=full;_tmp85;});_tmp86->f1=_tmp11E;});_tmp86;});_tmp87->links=_tmp11F;});_tmp87;});}}
# 270
struct Cyc_PP_Doc*Cyc_PP_hyperlink(struct _fat_ptr shrt,struct _fat_ptr full){
# 278
int slen=(int)({Cyc_strlen((struct _fat_ptr)shrt);});
return({struct Cyc_PP_Doc*_tmp8B=_cycalloc(sizeof(*_tmp8B));_tmp8B->mwo=slen,_tmp8B->mw=Cyc_PP_infinity,({
# 281
struct Cyc_Fn_Function*_tmp123=({({struct Cyc_Fn_Function*(*_tmp122)(struct Cyc_PP_Out*(*f)(struct _tuple3*,struct Cyc_PP_Ppstate*),struct _tuple3*x)=({struct Cyc_Fn_Function*(*_tmp89)(struct Cyc_PP_Out*(*f)(struct _tuple3*,struct Cyc_PP_Ppstate*),struct _tuple3*x)=(struct Cyc_Fn_Function*(*)(struct Cyc_PP_Out*(*f)(struct _tuple3*,struct Cyc_PP_Ppstate*),struct _tuple3*x))Cyc_Fn_make_fn;_tmp89;});_tmp122(Cyc_PP_hyperlink_f,({struct _tuple3*_tmp8A=_cycalloc(sizeof(*_tmp8A));_tmp8A->f1=slen,_tmp8A->f2=shrt,_tmp8A->f3=full;_tmp8A;}));});});_tmp8B->f=_tmp123;});_tmp8B;});}
# 285
static struct Cyc_PP_Out*Cyc_PP_line_f(struct Cyc_PP_Ppstate*st){
return({struct Cyc_PP_Out*_tmp90=_cycalloc(sizeof(*_tmp90));_tmp90->newcc=st->ci,_tmp90->newcl=st->cl + 1,({
# 288
void*_tmp126=(void*)({struct Cyc_PP_Single_PP_Alist_struct*_tmp8F=_cycalloc(sizeof(*_tmp8F));_tmp8F->tag=1U,({struct _fat_ptr*_tmp125=({unsigned _tmp8E=1;struct _fat_ptr*_tmp8D=_cycalloc(_check_times(_tmp8E,sizeof(struct _fat_ptr)));({struct _fat_ptr _tmp124=({Cyc_PP_nlblanks(st->ci);});_tmp8D[0]=_tmp124;});_tmp8D;});_tmp8F->f1=_tmp125;});_tmp8F;});_tmp90->ppout=_tmp126;}),_tmp90->links=(void*)& Cyc_PP_Empty_link;_tmp90;});}
# 285
struct Cyc_PP_Doc*Cyc_PP_line_doc(){
# 292
if(Cyc_PP_line_doc_opt == 0)
Cyc_PP_line_doc_opt=({struct Cyc_Core_Opt*_tmp94=_cycalloc(sizeof(*_tmp94));({struct Cyc_PP_Doc*_tmp128=({struct Cyc_PP_Doc*_tmp93=_cycalloc(sizeof(*_tmp93));_tmp93->mwo=0,_tmp93->mw=0,({struct Cyc_Fn_Function*_tmp127=({({struct Cyc_Fn_Function*(*_tmp92)(struct Cyc_PP_Out*(*f)(struct Cyc_PP_Ppstate*))=(struct Cyc_Fn_Function*(*)(struct Cyc_PP_Out*(*f)(struct Cyc_PP_Ppstate*)))Cyc_Fn_fp2fn;_tmp92;})(Cyc_PP_line_f);});_tmp93->f=_tmp127;});_tmp93;});_tmp94->v=_tmp128;});_tmp94;});
# 292
return(struct Cyc_PP_Doc*)((struct Cyc_Core_Opt*)_check_null(Cyc_PP_line_doc_opt))->v;}struct _tuple5{int f1;struct Cyc_PP_Doc*f2;};
# 297
static struct Cyc_PP_Out*Cyc_PP_nest_f(struct _tuple5*clo,struct Cyc_PP_Ppstate*st){
struct Cyc_PP_Doc*_tmp97;int _tmp96;_LL1: _tmp96=clo->f1;_tmp97=clo->f2;_LL2: {int k=_tmp96;struct Cyc_PP_Doc*d=_tmp97;
struct Cyc_PP_Ppstate*st2=({struct Cyc_PP_Ppstate*_tmp99=_cycalloc(sizeof(*_tmp99));
_tmp99->ci=st->ci + k,_tmp99->cc=st->cc,_tmp99->cl=st->cl,_tmp99->pw=st->pw,_tmp99->epw=st->epw;_tmp99;});
# 305
return({({struct Cyc_PP_Out*(*_tmp12A)(struct Cyc_Fn_Function*f,struct Cyc_PP_Ppstate*x)=({struct Cyc_PP_Out*(*_tmp98)(struct Cyc_Fn_Function*f,struct Cyc_PP_Ppstate*x)=(struct Cyc_PP_Out*(*)(struct Cyc_Fn_Function*f,struct Cyc_PP_Ppstate*x))Cyc_Fn_apply;_tmp98;});struct Cyc_Fn_Function*_tmp129=d->f;_tmp12A(_tmp129,st2);});});}}
# 297
struct Cyc_PP_Doc*Cyc_PP_nest(int k,struct Cyc_PP_Doc*d){
# 308
return({struct Cyc_PP_Doc*_tmp9D=_cycalloc(sizeof(*_tmp9D));_tmp9D->mwo=d->mwo,_tmp9D->mw=d->mw,({
# 310
struct Cyc_Fn_Function*_tmp12C=({({struct Cyc_Fn_Function*(*_tmp12B)(struct Cyc_PP_Out*(*f)(struct _tuple5*,struct Cyc_PP_Ppstate*),struct _tuple5*x)=({struct Cyc_Fn_Function*(*_tmp9B)(struct Cyc_PP_Out*(*f)(struct _tuple5*,struct Cyc_PP_Ppstate*),struct _tuple5*x)=(struct Cyc_Fn_Function*(*)(struct Cyc_PP_Out*(*f)(struct _tuple5*,struct Cyc_PP_Ppstate*),struct _tuple5*x))Cyc_Fn_make_fn;_tmp9B;});_tmp12B(Cyc_PP_nest_f,({struct _tuple5*_tmp9C=_cycalloc(sizeof(*_tmp9C));_tmp9C->f1=k,_tmp9C->f2=d;_tmp9C;}));});});_tmp9D->f=_tmp12C;});_tmp9D;});}
# 315
int Cyc_PP_min(int x,int y){
if(x <= y)return x;else{
return y;}}
# 320
int Cyc_PP_max(int x,int y){
if(x >= y)return x;else{
return y;}}struct _tuple6{struct Cyc_PP_Doc*f1;struct Cyc_PP_Doc*f2;};
# 325
static struct Cyc_PP_Out*Cyc_PP_concat_f(struct _tuple6*clo,struct Cyc_PP_Ppstate*st){
struct Cyc_PP_Doc*_tmpA2;struct Cyc_PP_Doc*_tmpA1;_LL1: _tmpA1=clo->f1;_tmpA2=clo->f2;_LL2: {struct Cyc_PP_Doc*d1=_tmpA1;struct Cyc_PP_Doc*d2=_tmpA2;
int epw2=({Cyc_PP_max(st->pw - d2->mw,st->epw - d1->mwo);});
struct Cyc_PP_Ppstate*st2=({struct Cyc_PP_Ppstate*_tmpA7=_cycalloc(sizeof(*_tmpA7));_tmpA7->ci=st->ci,_tmpA7->cc=st->cc,_tmpA7->cl=st->cl,_tmpA7->pw=st->pw,_tmpA7->epw=epw2;_tmpA7;});
struct Cyc_PP_Out*o1=({({struct Cyc_PP_Out*(*_tmp12E)(struct Cyc_Fn_Function*f,struct Cyc_PP_Ppstate*x)=({struct Cyc_PP_Out*(*_tmpA6)(struct Cyc_Fn_Function*f,struct Cyc_PP_Ppstate*x)=(struct Cyc_PP_Out*(*)(struct Cyc_Fn_Function*f,struct Cyc_PP_Ppstate*x))Cyc_Fn_apply;_tmpA6;});struct Cyc_Fn_Function*_tmp12D=d1->f;_tmp12E(_tmp12D,st2);});});
struct Cyc_PP_Ppstate*st3=({struct Cyc_PP_Ppstate*_tmpA5=_cycalloc(sizeof(*_tmpA5));_tmpA5->ci=st->ci,_tmpA5->cc=o1->newcc,_tmpA5->cl=o1->newcl,_tmpA5->pw=st->pw,_tmpA5->epw=epw2;_tmpA5;});
struct Cyc_PP_Out*o2=({({struct Cyc_PP_Out*(*_tmp130)(struct Cyc_Fn_Function*f,struct Cyc_PP_Ppstate*x)=({struct Cyc_PP_Out*(*_tmpA4)(struct Cyc_Fn_Function*f,struct Cyc_PP_Ppstate*x)=(struct Cyc_PP_Out*(*)(struct Cyc_Fn_Function*f,struct Cyc_PP_Ppstate*x))Cyc_Fn_apply;_tmpA4;});struct Cyc_Fn_Function*_tmp12F=d2->f;_tmp130(_tmp12F,st3);});});
struct Cyc_PP_Out*o3=({struct Cyc_PP_Out*_tmpA3=_cycalloc(sizeof(*_tmpA3));_tmpA3->newcc=o2->newcc,_tmpA3->newcl=o2->newcl,({
# 334
void*_tmp132=({Cyc_PP_append(o1->ppout,o2->ppout);});_tmpA3->ppout=_tmp132;}),({
void*_tmp131=({Cyc_PP_append(o1->links,o2->links);});_tmpA3->links=_tmp131;});_tmpA3;});
return o3;}}
# 338
static struct Cyc_PP_Doc*Cyc_PP_concat(struct Cyc_PP_Doc*d1,struct Cyc_PP_Doc*d2){
return({struct Cyc_PP_Doc*_tmpAB=_cycalloc(sizeof(*_tmpAB));({int _tmp136=({Cyc_PP_min(d1->mw,d1->mwo + d2->mwo);});_tmpAB->mwo=_tmp136;}),({
int _tmp135=({Cyc_PP_min(d1->mw,d1->mwo + d2->mw);});_tmpAB->mw=_tmp135;}),({
struct Cyc_Fn_Function*_tmp134=({({struct Cyc_Fn_Function*(*_tmp133)(struct Cyc_PP_Out*(*f)(struct _tuple6*,struct Cyc_PP_Ppstate*),struct _tuple6*x)=({struct Cyc_Fn_Function*(*_tmpA9)(struct Cyc_PP_Out*(*f)(struct _tuple6*,struct Cyc_PP_Ppstate*),struct _tuple6*x)=(struct Cyc_Fn_Function*(*)(struct Cyc_PP_Out*(*f)(struct _tuple6*,struct Cyc_PP_Ppstate*),struct _tuple6*x))Cyc_Fn_make_fn;_tmpA9;});_tmp133(Cyc_PP_concat_f,({struct _tuple6*_tmpAA=_cycalloc(sizeof(*_tmpAA));_tmpAA->f1=d1,_tmpAA->f2=d2;_tmpAA;}));});});_tmpAB->f=_tmp134;});_tmpAB;});}
# 338
struct Cyc_PP_Doc*Cyc_PP_cat(struct _fat_ptr l){
# 344
struct Cyc_PP_Doc*d=({Cyc_PP_nil_doc();});
{int i=(int)(_get_fat_size(l,sizeof(struct Cyc_PP_Doc*))- (unsigned)1);for(0;i >= 0;-- i){
d=({Cyc_PP_concat(*((struct Cyc_PP_Doc**)_check_fat_subscript(l,sizeof(struct Cyc_PP_Doc*),i)),d);});}}
# 348
return d;}
# 353
static struct Cyc_PP_Out*Cyc_PP_long_cats_f(struct Cyc_List_List*ds0,struct Cyc_PP_Ppstate*st){
struct Cyc_List_List*os=0;
{struct Cyc_List_List*ds=ds0;for(0;ds != 0;ds=ds->tl){
struct Cyc_PP_Doc*d=(struct Cyc_PP_Doc*)ds->hd;
struct Cyc_PP_Out*o=({({struct Cyc_PP_Out*(*_tmp138)(struct Cyc_Fn_Function*f,struct Cyc_PP_Ppstate*x)=({struct Cyc_PP_Out*(*_tmpB0)(struct Cyc_Fn_Function*f,struct Cyc_PP_Ppstate*x)=(struct Cyc_PP_Out*(*)(struct Cyc_Fn_Function*f,struct Cyc_PP_Ppstate*x))Cyc_Fn_apply;_tmpB0;});struct Cyc_Fn_Function*_tmp137=d->f;_tmp138(_tmp137,st);});});
st=({struct Cyc_PP_Ppstate*_tmpAE=_cycalloc(sizeof(*_tmpAE));_tmpAE->ci=st->ci,_tmpAE->cc=o->newcc,_tmpAE->cl=o->newcl,_tmpAE->pw=st->pw,_tmpAE->epw=st->epw - d->mwo;_tmpAE;});
os=({struct Cyc_List_List*_tmpAF=_cycalloc(sizeof(*_tmpAF));_tmpAF->hd=o,_tmpAF->tl=os;_tmpAF;});}}{
# 361
int newcc=((struct Cyc_PP_Out*)((struct Cyc_List_List*)_check_null(os))->hd)->newcc;
int newcl=((struct Cyc_PP_Out*)os->hd)->newcl;
void*s=(void*)& Cyc_PP_Empty_stringptr;
void*links=(void*)& Cyc_PP_Empty_link;
for(0;os != 0;os=os->tl){
s=({Cyc_PP_append(((struct Cyc_PP_Out*)os->hd)->ppout,s);});
links=({Cyc_PP_append(((struct Cyc_PP_Out*)os->hd)->links,links);});}
# 369
return({struct Cyc_PP_Out*_tmpB1=_cycalloc(sizeof(*_tmpB1));_tmpB1->newcc=newcc,_tmpB1->newcl=newcl,_tmpB1->ppout=s,_tmpB1->links=links;_tmpB1;});}}
# 371
static struct Cyc_PP_Doc*Cyc_PP_long_cats(struct Cyc_List_List*doclist){
# 375
struct Cyc_List_List*orig=doclist;
struct Cyc_PP_Doc*d=(struct Cyc_PP_Doc*)((struct Cyc_List_List*)_check_null(doclist))->hd;
doclist=doclist->tl;{
int mw=d->mw;
int mwo=d->mw;
# 381
{struct Cyc_List_List*ds=doclist;for(0;ds != 0;ds=ds->tl){
int mw2=({Cyc_PP_min(mw,mwo + ((struct Cyc_PP_Doc*)ds->hd)->mwo);});
int mwo2=({Cyc_PP_min(mw,mwo + ((struct Cyc_PP_Doc*)ds->hd)->mw);});
mw=mw2;
mwo=mwo2;}}
# 387
return({struct Cyc_PP_Doc*_tmpB4=_cycalloc(sizeof(*_tmpB4));_tmpB4->mwo=mw,_tmpB4->mw=mwo,({struct Cyc_Fn_Function*_tmp13A=({({struct Cyc_Fn_Function*(*_tmp139)(struct Cyc_PP_Out*(*f)(struct Cyc_List_List*,struct Cyc_PP_Ppstate*),struct Cyc_List_List*x)=({struct Cyc_Fn_Function*(*_tmpB3)(struct Cyc_PP_Out*(*f)(struct Cyc_List_List*,struct Cyc_PP_Ppstate*),struct Cyc_List_List*x)=(struct Cyc_Fn_Function*(*)(struct Cyc_PP_Out*(*f)(struct Cyc_List_List*,struct Cyc_PP_Ppstate*),struct Cyc_List_List*x))Cyc_Fn_make_fn;_tmpB3;});_tmp139(Cyc_PP_long_cats_f,orig);});});_tmpB4->f=_tmp13A;});_tmpB4;});}}
# 371
struct Cyc_PP_Doc*Cyc_PP_cats(struct Cyc_List_List*doclist){
# 391
if(doclist == 0)return({Cyc_PP_nil_doc();});else{
if(doclist->tl == 0)return(struct Cyc_PP_Doc*)doclist->hd;else{
# 394
if(({Cyc_List_length(doclist);})> 30)return({Cyc_PP_long_cats(doclist);});else{
return({struct Cyc_PP_Doc*(*_tmpB6)(struct Cyc_PP_Doc*d1,struct Cyc_PP_Doc*d2)=Cyc_PP_concat;struct Cyc_PP_Doc*_tmpB7=(struct Cyc_PP_Doc*)doclist->hd;struct Cyc_PP_Doc*_tmpB8=({Cyc_PP_cats(doclist->tl);});_tmpB6(_tmpB7,_tmpB8);});}}}}
# 398
static struct Cyc_PP_Out*Cyc_PP_cats_arr_f(struct _fat_ptr*docs_ptr,struct Cyc_PP_Ppstate*st){
struct Cyc_List_List*os=0;
struct _fat_ptr docs=*docs_ptr;
int sz=(int)_get_fat_size(docs,sizeof(struct Cyc_PP_Doc*));
{int i=0;for(0;i < sz;++ i){
struct Cyc_PP_Doc*d=*((struct Cyc_PP_Doc**)_check_fat_subscript(docs,sizeof(struct Cyc_PP_Doc*),i));
struct Cyc_PP_Out*o=({({struct Cyc_PP_Out*(*_tmp13C)(struct Cyc_Fn_Function*f,struct Cyc_PP_Ppstate*x)=({struct Cyc_PP_Out*(*_tmpBC)(struct Cyc_Fn_Function*f,struct Cyc_PP_Ppstate*x)=(struct Cyc_PP_Out*(*)(struct Cyc_Fn_Function*f,struct Cyc_PP_Ppstate*x))Cyc_Fn_apply;_tmpBC;});struct Cyc_Fn_Function*_tmp13B=d->f;_tmp13C(_tmp13B,st);});});
st=({struct Cyc_PP_Ppstate*_tmpBA=_cycalloc(sizeof(*_tmpBA));_tmpBA->ci=st->ci,_tmpBA->cc=o->newcc,_tmpBA->cl=o->newcl,_tmpBA->pw=st->pw,_tmpBA->epw=st->epw - d->mwo;_tmpBA;});
os=({struct Cyc_List_List*_tmpBB=_cycalloc(sizeof(*_tmpBB));_tmpBB->hd=o,_tmpBB->tl=os;_tmpBB;});}}{
# 408
int newcc=((struct Cyc_PP_Out*)((struct Cyc_List_List*)_check_null(os))->hd)->newcc;
int newcl=((struct Cyc_PP_Out*)os->hd)->newcl;
void*s=(void*)& Cyc_PP_Empty_stringptr;
void*links=(void*)& Cyc_PP_Empty_link;
for(0;os != 0;os=os->tl){
s=({Cyc_PP_append(((struct Cyc_PP_Out*)os->hd)->ppout,s);});
links=({Cyc_PP_append(((struct Cyc_PP_Out*)os->hd)->links,links);});}
# 416
return({struct Cyc_PP_Out*_tmpBD=_cycalloc(sizeof(*_tmpBD));_tmpBD->newcc=newcc,_tmpBD->newcl=newcl,_tmpBD->ppout=s,_tmpBD->links=links;_tmpBD;});}}
# 398
struct Cyc_PP_Doc*Cyc_PP_cats_arr(struct _fat_ptr docs){
# 420
int sz=(int)_get_fat_size(docs,sizeof(struct Cyc_PP_Doc*));
if(sz == 0)
(int)_throw(({struct Cyc_Core_Failure_exn_struct*_tmpC0=_cycalloc(sizeof(*_tmpC0));_tmpC0->tag=Cyc_Core_Failure,({struct _fat_ptr _tmp13D=({const char*_tmpBF="cats_arr -- size zero array";_tag_fat(_tmpBF,sizeof(char),28U);});_tmpC0->f1=_tmp13D;});_tmpC0;}));{
# 421
struct Cyc_PP_Doc*d=*((struct Cyc_PP_Doc**)_check_fat_subscript(docs,sizeof(struct Cyc_PP_Doc*),0));
# 424
int mw=d->mw;
int mwo=d->mw;
{int i=1;for(0;i < sz;++ i){
int mw2=({Cyc_PP_min(mw,mwo + (*((struct Cyc_PP_Doc**)_check_fat_subscript(docs,sizeof(struct Cyc_PP_Doc*),i)))->mwo);});
int mwo2=({Cyc_PP_min(mw,mwo + (((struct Cyc_PP_Doc**)docs.curr)[i])->mw);});
mw=mw2;
mwo=mwo2;}}
# 432
return({struct Cyc_PP_Doc*_tmpC4=_cycalloc(sizeof(*_tmpC4));_tmpC4->mwo=mw,_tmpC4->mw=mwo,({struct Cyc_Fn_Function*_tmp13F=({({struct Cyc_Fn_Function*(*_tmp13E)(struct Cyc_PP_Out*(*f)(struct _fat_ptr*,struct Cyc_PP_Ppstate*),struct _fat_ptr*x)=({struct Cyc_Fn_Function*(*_tmpC1)(struct Cyc_PP_Out*(*f)(struct _fat_ptr*,struct Cyc_PP_Ppstate*),struct _fat_ptr*x)=(struct Cyc_Fn_Function*(*)(struct Cyc_PP_Out*(*f)(struct _fat_ptr*,struct Cyc_PP_Ppstate*),struct _fat_ptr*x))Cyc_Fn_make_fn;_tmpC1;});_tmp13E(Cyc_PP_cats_arr_f,({unsigned _tmpC3=1;struct _fat_ptr*_tmpC2=_cycalloc(_check_times(_tmpC3,sizeof(struct _fat_ptr)));_tmpC2[0]=docs;_tmpC2;}));});});_tmpC4->f=_tmp13F;});_tmpC4;});}}
# 435
static struct Cyc_PP_Out*Cyc_PP_doc_union_f(struct _tuple6*clo,struct Cyc_PP_Ppstate*st){
struct Cyc_PP_Doc*_tmpC7;struct Cyc_PP_Doc*_tmpC6;_LL1: _tmpC6=clo->f1;_tmpC7=clo->f2;_LL2: {struct Cyc_PP_Doc*d=_tmpC6;struct Cyc_PP_Doc*d2=_tmpC7;
int dfits=st->cc + d->mwo <= st->epw || st->cc + d->mw <= st->pw;
# 439
if(dfits)return({({struct Cyc_PP_Out*(*_tmp141)(struct Cyc_Fn_Function*f,struct Cyc_PP_Ppstate*x)=({struct Cyc_PP_Out*(*_tmpC8)(struct Cyc_Fn_Function*f,struct Cyc_PP_Ppstate*x)=(struct Cyc_PP_Out*(*)(struct Cyc_Fn_Function*f,struct Cyc_PP_Ppstate*x))Cyc_Fn_apply;_tmpC8;});struct Cyc_Fn_Function*_tmp140=d->f;_tmp141(_tmp140,st);});});else{
return({({struct Cyc_PP_Out*(*_tmp143)(struct Cyc_Fn_Function*f,struct Cyc_PP_Ppstate*x)=({struct Cyc_PP_Out*(*_tmpC9)(struct Cyc_Fn_Function*f,struct Cyc_PP_Ppstate*x)=(struct Cyc_PP_Out*(*)(struct Cyc_Fn_Function*f,struct Cyc_PP_Ppstate*x))Cyc_Fn_apply;_tmpC9;});struct Cyc_Fn_Function*_tmp142=d2->f;_tmp143(_tmp142,st);});});}}}
# 435
struct Cyc_PP_Doc*Cyc_PP_doc_union(struct Cyc_PP_Doc*d,struct Cyc_PP_Doc*d2){
# 443
return({struct Cyc_PP_Doc*_tmpCD=_cycalloc(sizeof(*_tmpCD));({int _tmp147=({Cyc_PP_min(d->mwo,d2->mwo);});_tmpCD->mwo=_tmp147;}),({
int _tmp146=({Cyc_PP_min(d->mw,d2->mw);});_tmpCD->mw=_tmp146;}),({
struct Cyc_Fn_Function*_tmp145=({({struct Cyc_Fn_Function*(*_tmp144)(struct Cyc_PP_Out*(*f)(struct _tuple6*,struct Cyc_PP_Ppstate*),struct _tuple6*x)=({struct Cyc_Fn_Function*(*_tmpCB)(struct Cyc_PP_Out*(*f)(struct _tuple6*,struct Cyc_PP_Ppstate*),struct _tuple6*x)=(struct Cyc_Fn_Function*(*)(struct Cyc_PP_Out*(*f)(struct _tuple6*,struct Cyc_PP_Ppstate*),struct _tuple6*x))Cyc_Fn_make_fn;_tmpCB;});_tmp144(Cyc_PP_doc_union_f,({struct _tuple6*_tmpCC=_cycalloc(sizeof(*_tmpCC));_tmpCC->f1=d,_tmpCC->f2=d2;_tmpCC;}));});});_tmpCD->f=_tmp145;});_tmpCD;});}
# 435
struct Cyc_PP_Doc*Cyc_PP_oline_doc(){
# 450
return({struct Cyc_PP_Doc*(*_tmpCF)(struct Cyc_PP_Doc*d,struct Cyc_PP_Doc*d2)=Cyc_PP_doc_union;struct Cyc_PP_Doc*_tmpD0=({Cyc_PP_nil_doc();});struct Cyc_PP_Doc*_tmpD1=({Cyc_PP_line_doc();});_tmpCF(_tmpD0,_tmpD1);});}
# 453
static struct Cyc_PP_Out*Cyc_PP_tab_f(struct Cyc_PP_Doc*d,struct Cyc_PP_Ppstate*st){
struct Cyc_PP_Ppstate*st2=({struct Cyc_PP_Ppstate*_tmpD4=_cycalloc(sizeof(*_tmpD4));_tmpD4->ci=st->cc,_tmpD4->cc=st->cc,_tmpD4->cl=st->cl,_tmpD4->pw=st->pw,_tmpD4->epw=st->epw;_tmpD4;});
return({({struct Cyc_PP_Out*(*_tmp149)(struct Cyc_Fn_Function*f,struct Cyc_PP_Ppstate*x)=({struct Cyc_PP_Out*(*_tmpD3)(struct Cyc_Fn_Function*f,struct Cyc_PP_Ppstate*x)=(struct Cyc_PP_Out*(*)(struct Cyc_Fn_Function*f,struct Cyc_PP_Ppstate*x))Cyc_Fn_apply;_tmpD3;});struct Cyc_Fn_Function*_tmp148=d->f;_tmp149(_tmp148,st2);});});}
# 453
struct Cyc_PP_Doc*Cyc_PP_tab(struct Cyc_PP_Doc*d){
# 458
struct Cyc_PP_Doc*d2=({struct Cyc_PP_Doc*_tmpD7=_cycalloc(sizeof(*_tmpD7));
_tmpD7->mwo=d->mwo,_tmpD7->mw=d->mw,({
# 461
struct Cyc_Fn_Function*_tmp14B=({({struct Cyc_Fn_Function*(*_tmp14A)(struct Cyc_PP_Out*(*f)(struct Cyc_PP_Doc*,struct Cyc_PP_Ppstate*),struct Cyc_PP_Doc*x)=({struct Cyc_Fn_Function*(*_tmpD6)(struct Cyc_PP_Out*(*f)(struct Cyc_PP_Doc*,struct Cyc_PP_Ppstate*),struct Cyc_PP_Doc*x)=(struct Cyc_Fn_Function*(*)(struct Cyc_PP_Out*(*f)(struct Cyc_PP_Doc*,struct Cyc_PP_Ppstate*),struct Cyc_PP_Doc*x))Cyc_Fn_make_fn;_tmpD6;});_tmp14A(Cyc_PP_tab_f,d);});});_tmpD7->f=_tmp14B;});_tmpD7;});
return d2;}
# 467
static struct Cyc_PP_Doc*Cyc_PP_ppseq_f(struct Cyc_PP_Doc*(*pp)(void*),struct _fat_ptr sep,struct Cyc_List_List*l){
if(l == 0)return({Cyc_PP_nil_doc();});else{
if(l->tl == 0)return({pp(l->hd);});else{
return({struct Cyc_PP_Doc*_tmpD9[4U];({struct Cyc_PP_Doc*_tmp14F=({pp(l->hd);});_tmpD9[0]=_tmp14F;}),({struct Cyc_PP_Doc*_tmp14E=({Cyc_PP_text(sep);});_tmpD9[1]=_tmp14E;}),({struct Cyc_PP_Doc*_tmp14D=({Cyc_PP_oline_doc();});_tmpD9[2]=_tmp14D;}),({struct Cyc_PP_Doc*_tmp14C=({Cyc_PP_ppseq_f(pp,sep,l->tl);});_tmpD9[3]=_tmp14C;});Cyc_PP_cat(_tag_fat(_tmpD9,sizeof(struct Cyc_PP_Doc*),4U));});}}}
# 467
struct Cyc_PP_Doc*Cyc_PP_ppseq(struct Cyc_PP_Doc*(*pp)(void*),struct _fat_ptr sep,struct Cyc_List_List*l){
# 474
return({struct Cyc_PP_Doc*(*_tmpDB)(struct Cyc_PP_Doc*d)=Cyc_PP_tab;struct Cyc_PP_Doc*_tmpDC=({Cyc_PP_ppseq_f(pp,sep,l);});_tmpDB(_tmpDC);});}
# 467
struct Cyc_PP_Doc*Cyc_PP_seq_f(struct _fat_ptr sep,struct Cyc_List_List*l){
# 478
if(l == 0)return({Cyc_PP_nil_doc();});else{
if(l->tl == 0)return(struct Cyc_PP_Doc*)l->hd;else{
# 481
struct Cyc_PP_Doc*sep2=({Cyc_PP_text(sep);});
struct Cyc_PP_Doc*oline=({Cyc_PP_oline_doc();});
struct Cyc_List_List*x=l;
while(((struct Cyc_List_List*)_check_null(x))->tl != 0){
struct Cyc_List_List*temp=x->tl;
({struct Cyc_List_List*_tmp151=({struct Cyc_List_List*_tmpDF=_cycalloc(sizeof(*_tmpDF));_tmpDF->hd=sep2,({struct Cyc_List_List*_tmp150=({struct Cyc_List_List*_tmpDE=_cycalloc(sizeof(*_tmpDE));_tmpDE->hd=oline,_tmpDE->tl=temp;_tmpDE;});_tmpDF->tl=_tmp150;});_tmpDF;});x->tl=_tmp151;});
x=temp;}
# 489
return({Cyc_PP_cats(l);});}}}
# 467
struct Cyc_PP_Doc*Cyc_PP_seq(struct _fat_ptr sep,struct Cyc_List_List*l){
# 498
return({struct Cyc_PP_Doc*(*_tmpE1)(struct Cyc_PP_Doc*d)=Cyc_PP_tab;struct Cyc_PP_Doc*_tmpE2=({Cyc_PP_seq_f(sep,l);});_tmpE1(_tmpE2);});}
# 467
struct Cyc_PP_Doc*Cyc_PP_ppseql_f(struct Cyc_PP_Doc*(*pp)(void*),struct _fat_ptr sep,struct Cyc_List_List*l){
# 503
if(l == 0)return({Cyc_PP_nil_doc();});else{
if(l->tl == 0)return({pp(l->hd);});else{
return({struct Cyc_PP_Doc*_tmpE4[4U];({struct Cyc_PP_Doc*_tmp155=({pp(l->hd);});_tmpE4[0]=_tmp155;}),({struct Cyc_PP_Doc*_tmp154=({Cyc_PP_text(sep);});_tmpE4[1]=_tmp154;}),({struct Cyc_PP_Doc*_tmp153=({Cyc_PP_line_doc();});_tmpE4[2]=_tmp153;}),({struct Cyc_PP_Doc*_tmp152=({Cyc_PP_ppseql_f(pp,sep,l->tl);});_tmpE4[3]=_tmp152;});Cyc_PP_cat(_tag_fat(_tmpE4,sizeof(struct Cyc_PP_Doc*),4U));});}}}
# 467
struct Cyc_PP_Doc*Cyc_PP_ppseql(struct Cyc_PP_Doc*(*pp)(void*),struct _fat_ptr sep,struct Cyc_List_List*l){
# 508
return({struct Cyc_PP_Doc*(*_tmpE6)(struct Cyc_PP_Doc*d)=Cyc_PP_tab;struct Cyc_PP_Doc*_tmpE7=({Cyc_PP_ppseql_f(pp,sep,l);});_tmpE6(_tmpE7);});}
# 511
static struct Cyc_PP_Doc*Cyc_PP_seql_f(struct _fat_ptr sep,struct Cyc_List_List*l){
if(l == 0)return({Cyc_PP_nil_doc();});else{
if(l->tl == 0)return(struct Cyc_PP_Doc*)l->hd;else{
return({struct Cyc_PP_Doc*_tmpE9[4U];_tmpE9[0]=(struct Cyc_PP_Doc*)l->hd,({struct Cyc_PP_Doc*_tmp158=({Cyc_PP_text(sep);});_tmpE9[1]=_tmp158;}),({struct Cyc_PP_Doc*_tmp157=({Cyc_PP_line_doc();});_tmpE9[2]=_tmp157;}),({struct Cyc_PP_Doc*_tmp156=({Cyc_PP_seql_f(sep,l->tl);});_tmpE9[3]=_tmp156;});Cyc_PP_cat(_tag_fat(_tmpE9,sizeof(struct Cyc_PP_Doc*),4U));});}}}
# 511
struct Cyc_PP_Doc*Cyc_PP_seql(struct _fat_ptr sep,struct Cyc_List_List*l0){
# 517
return({struct Cyc_PP_Doc*(*_tmpEB)(struct Cyc_PP_Doc*d)=Cyc_PP_tab;struct Cyc_PP_Doc*_tmpEC=({Cyc_PP_seql_f(sep,l0);});_tmpEB(_tmpEC);});}
# 511
struct Cyc_PP_Doc*Cyc_PP_group(struct _fat_ptr start,struct _fat_ptr stop,struct _fat_ptr sep,struct Cyc_List_List*ss){
# 522
return({struct Cyc_PP_Doc*_tmpEE[3U];({struct Cyc_PP_Doc*_tmp15B=({Cyc_PP_text(start);});_tmpEE[0]=_tmp15B;}),({
struct Cyc_PP_Doc*_tmp15A=({Cyc_PP_seq(sep,ss);});_tmpEE[1]=_tmp15A;}),({
struct Cyc_PP_Doc*_tmp159=({Cyc_PP_text(stop);});_tmpEE[2]=_tmp159;});Cyc_PP_cat(_tag_fat(_tmpEE,sizeof(struct Cyc_PP_Doc*),3U));});}
# 511
struct Cyc_PP_Doc*Cyc_PP_egroup(struct _fat_ptr start,struct _fat_ptr stop,struct _fat_ptr sep,struct Cyc_List_List*ss){
# 529
if(ss == 0)return({Cyc_PP_nil_doc();});else{
return({struct Cyc_PP_Doc*_tmpF0[3U];({struct Cyc_PP_Doc*_tmp15E=({Cyc_PP_text(start);});_tmpF0[0]=_tmp15E;}),({
struct Cyc_PP_Doc*_tmp15D=({Cyc_PP_seq(sep,ss);});_tmpF0[1]=_tmp15D;}),({
struct Cyc_PP_Doc*_tmp15C=({Cyc_PP_text(stop);});_tmpF0[2]=_tmp15C;});Cyc_PP_cat(_tag_fat(_tmpF0,sizeof(struct Cyc_PP_Doc*),3U));});}}
# 511
struct Cyc_PP_Doc*Cyc_PP_groupl(struct _fat_ptr start,struct _fat_ptr stop,struct _fat_ptr sep,struct Cyc_List_List*ss){
# 536
return({struct Cyc_PP_Doc*_tmpF2[3U];({struct Cyc_PP_Doc*_tmp161=({Cyc_PP_text(start);});_tmpF2[0]=_tmp161;}),({
struct Cyc_PP_Doc*_tmp160=({Cyc_PP_seql(sep,ss);});_tmpF2[1]=_tmp160;}),({
struct Cyc_PP_Doc*_tmp15F=({Cyc_PP_text(stop);});_tmpF2[2]=_tmp15F;});Cyc_PP_cat(_tag_fat(_tmpF2,sizeof(struct Cyc_PP_Doc*),3U));});}

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
# 100
int Cyc_fprintf(struct Cyc___cycFILE*,struct _fat_ptr,struct _fat_ptr);struct Cyc_ShortPtr_sa_ScanfArg_struct{int tag;short*f1;};struct Cyc_UShortPtr_sa_ScanfArg_struct{int tag;unsigned short*f1;};struct Cyc_IntPtr_sa_ScanfArg_struct{int tag;int*f1;};struct Cyc_UIntPtr_sa_ScanfArg_struct{int tag;unsigned*f1;};struct Cyc_StringPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_DoublePtr_sa_ScanfArg_struct{int tag;double*f1;};struct Cyc_FloatPtr_sa_ScanfArg_struct{int tag;float*f1;};struct Cyc_CharPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};
# 197 "cycboot.h"
int Cyc_sscanf(struct _fat_ptr,struct _fat_ptr,struct _fat_ptr);extern char Cyc_FileCloseError[15U];struct Cyc_FileCloseError_exn_struct{char*tag;};extern char Cyc_FileOpenError[14U];struct Cyc_FileOpenError_exn_struct{char*tag;struct _fat_ptr f1;};
# 300 "cycboot.h"
int isspace(int);struct Cyc_Core_Opt{void*v;};extern char Cyc_Core_Invalid_argument[17U];struct Cyc_Core_Invalid_argument_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Failure[8U];struct Cyc_Core_Failure_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Impossible[11U];struct Cyc_Core_Impossible_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Not_found[10U];struct Cyc_Core_Not_found_exn_struct{char*tag;};extern char Cyc_Core_Unreachable[12U];struct Cyc_Core_Unreachable_exn_struct{char*tag;struct _fat_ptr f1;};
# 168 "core.h"
extern struct _RegionHandle*Cyc_Core_unique_region;struct Cyc_Core_DynamicRegion;struct Cyc_Core_NewDynamicRegion{struct Cyc_Core_DynamicRegion*key;};struct Cyc_Core_ThinRes{void*arr;unsigned nelts;};struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};extern char Cyc_List_List_mismatch[14U];struct Cyc_List_List_mismatch_exn_struct{char*tag;};extern char Cyc_List_Nth[4U];struct Cyc_List_Nth_exn_struct{char*tag;};
# 38 "string.h"
unsigned long Cyc_strlen(struct _fat_ptr s);
# 49 "string.h"
int Cyc_strcmp(struct _fat_ptr s1,struct _fat_ptr s2);
# 51
int Cyc_strncmp(struct _fat_ptr s1,struct _fat_ptr s2,unsigned long len);
# 62
struct _fat_ptr Cyc_strconcat(struct _fat_ptr,struct _fat_ptr);extern char Cyc_Arg_Bad[4U];struct Cyc_Arg_Bad_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Arg_Error[6U];struct Cyc_Arg_Error_exn_struct{char*tag;};struct Cyc_Arg_Unit_spec_Arg_Spec_struct{int tag;void(*f1)();};struct Cyc_Arg_Flag_spec_Arg_Spec_struct{int tag;void(*f1)(struct _fat_ptr);};struct Cyc_Arg_FlagString_spec_Arg_Spec_struct{int tag;void(*f1)(struct _fat_ptr,struct _fat_ptr);};struct Cyc_Arg_Set_spec_Arg_Spec_struct{int tag;int*f1;};struct Cyc_Arg_Clear_spec_Arg_Spec_struct{int tag;int*f1;};struct Cyc_Arg_String_spec_Arg_Spec_struct{int tag;void(*f1)(struct _fat_ptr);};struct Cyc_Arg_Int_spec_Arg_Spec_struct{int tag;void(*f1)(int);};struct Cyc_Arg_Rest_spec_Arg_Spec_struct{int tag;void(*f1)(struct _fat_ptr);};
# 66 "arg.h"
void Cyc_Arg_usage(struct Cyc_List_List*,struct _fat_ptr);
# 69
extern int Cyc_Arg_current;struct Cyc_Buffer_t;
# 50 "buffer.h"
struct Cyc_Buffer_t*Cyc_Buffer_create(unsigned n);
# 58
struct _fat_ptr Cyc_Buffer_contents(struct Cyc_Buffer_t*);
# 83
void Cyc_Buffer_add_substring(struct Cyc_Buffer_t*,struct _fat_ptr,int offset,int len);
# 92 "buffer.h"
void Cyc_Buffer_add_string(struct Cyc_Buffer_t*,struct _fat_ptr);
# 29 "assert.h"
void*Cyc___assert_fail(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line);char Cyc_Arg_Bad[4U]="Bad";char Cyc_Arg_Error[6U]="Error";struct Cyc_Arg_Prefix_Arg_Flag_struct{int tag;struct _fat_ptr f1;};struct Cyc_Arg_Exact_Arg_Flag_struct{int tag;struct _fat_ptr f1;};struct Cyc_Arg_Unknown_Arg_error_struct{int tag;struct _fat_ptr f1;};struct Cyc_Arg_Missing_Arg_error_struct{int tag;struct _fat_ptr f1;};struct Cyc_Arg_Message_Arg_error_struct{int tag;struct _fat_ptr f1;};struct Cyc_Arg_Wrong_Arg_error_struct{int tag;struct _fat_ptr f1;struct _fat_ptr f2;struct _fat_ptr f3;};struct _tuple0{struct _fat_ptr f1;int f2;struct _fat_ptr f3;void*f4;struct _fat_ptr f5;};
# 68 "arg.cyc"
static void*Cyc_Arg_lookup(struct Cyc_List_List*l,struct _fat_ptr x){
while(l != 0){
struct _fat_ptr flag=(*((struct _tuple0*)l->hd)).f1;
unsigned long len=({Cyc_strlen((struct _fat_ptr)flag);});
if(len > (unsigned long)0 &&(*((struct _tuple0*)l->hd)).f2){
if(({Cyc_strncmp((struct _fat_ptr)x,(struct _fat_ptr)(*((struct _tuple0*)l->hd)).f1,len);})== 0)
return(*((struct _tuple0*)l->hd)).f4;}else{
# 76
if(({Cyc_strcmp((struct _fat_ptr)x,(struct _fat_ptr)(*((struct _tuple0*)l->hd)).f1);})== 0)
return(*((struct _tuple0*)l->hd)).f4;}
# 72
l=l->tl;}
# 80
(int)_throw(({struct Cyc_Core_Not_found_exn_struct*_tmp0=_cycalloc(sizeof(*_tmp0));_tmp0->tag=Cyc_Core_Not_found;_tmp0;}));}
# 89
static struct _fat_ptr Cyc_Arg_Justify_break_line(struct Cyc_Buffer_t*b,int howmuch,struct _fat_ptr s){
if(({char*_tmp6F=(char*)s.curr;_tmp6F == (char*)(_tag_fat(0,0,0)).curr;}))return _tag_fat(0,0,0);if(howmuch < 0)
howmuch=0;{
# 90
unsigned long len=({Cyc_strlen((struct _fat_ptr)s);});
# 93
len <= _get_fat_size(s,sizeof(char))?0:({({int(*_tmp71)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmp2)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmp2;});struct _fat_ptr _tmp70=({const char*_tmp3="len <= numelts(s)";_tag_fat(_tmp3,sizeof(char),18U);});_tmp71(_tmp70,({const char*_tmp4="arg.cyc";_tag_fat(_tmp4,sizeof(char),8U);}),93U);});});
if((unsigned long)howmuch > len){
({Cyc_Buffer_add_string(b,s);});
return _tag_fat(0,0,0);}{
# 94
int i;
# 101
for(i=howmuch - 1;i >= 0 && !({isspace((int)*((const char*)_check_fat_subscript(s,sizeof(char),i)));});-- i){
;}
# 105
if(i < 0)
for(i=howmuch?howmuch - 1: 0;((unsigned long)i < len &&(int)((const char*)s.curr)[i])&& !({isspace((int)((const char*)s.curr)[i]);});++ i){
;}
# 105
({Cyc_Buffer_add_substring(b,s,0,i);});{
# 114
struct _fat_ptr whatsleft=_tag_fat(0,0,0);
# 116
for(0;((unsigned long)i < len &&(int)((const char*)s.curr)[i])&&({isspace((int)((const char*)s.curr)[i]);});++ i){
;}
if((unsigned long)i < len &&(int)((const char*)s.curr)[i])whatsleft=_fat_ptr_plus(s,sizeof(char),i);return whatsleft;}}}}
# 128
void Cyc_Arg_Justify_justify_b(struct Cyc_Buffer_t*b,int indent,int margin,struct _fat_ptr item,struct _fat_ptr desc){
if(({char*_tmp72=(char*)item.curr;_tmp72 != (char*)(_tag_fat(0,0,0)).curr;}))({Cyc_Buffer_add_string(b,item);});if(({
char*_tmp73=(char*)desc.curr;_tmp73 == (char*)(_tag_fat(0,0,0)).curr;}))return;
# 129
if(indent < 0)
# 131
indent=0;
# 129
if(margin < 0)
# 132
margin=0;{
# 129
struct _fat_ptr indentstr=({unsigned _tmp10=(unsigned)(indent + 2)+ 1U;char*_tmpF=_cycalloc_atomic(_check_times(_tmp10,sizeof(char)));({{unsigned _tmp61=(unsigned)(indent + 2);unsigned i;for(i=0;i < _tmp61;++ i){_tmpF[i]='\000';}_tmpF[_tmp61]=0;}0;});_tag_fat(_tmpF,sizeof(char),_tmp10);});
# 136
{unsigned i=0U;for(0;i < (unsigned)(indent + 1);++ i){
({struct _fat_ptr _tmp6=_fat_ptr_plus(indentstr,sizeof(char),(int)i);char _tmp7=*((char*)_check_fat_subscript(_tmp6,sizeof(char),0U));char _tmp8=i == (unsigned)0?'\n':' ';if(_get_fat_size(_tmp6,sizeof(char))== 1U &&(_tmp7 == 0 && _tmp8 != 0))_throw_arraybounds();*((char*)_tmp6.curr)=_tmp8;});}}{
unsigned long itemlen=({Cyc_strlen((struct _fat_ptr)item);});
struct _fat_ptr itemsep;
if(({Cyc_strlen((struct _fat_ptr)desc);})> (unsigned long)0){
if(itemlen + (unsigned long)1 > (unsigned long)indent)
itemsep=indentstr;else{
# 144
struct _fat_ptr temp=({unsigned _tmpD=(((unsigned long)indent - itemlen)+ (unsigned long)1)+ 1U;char*_tmpC=_cycalloc_atomic(_check_times(_tmpD,sizeof(char)));({{unsigned _tmp60=((unsigned long)indent - itemlen)+ (unsigned long)1;unsigned i;for(i=0;i < _tmp60;++ i){_tmpC[i]='\000';}_tmpC[_tmp60]=0;}0;});_tag_fat(_tmpC,sizeof(char),_tmpD);});
{unsigned i=0U;for(0;i < (unsigned long)indent - itemlen;++ i){({struct _fat_ptr _tmp9=_fat_ptr_plus(temp,sizeof(char),(int)i);char _tmpA=*((char*)_check_fat_subscript(_tmp9,sizeof(char),0U));char _tmpB=' ';if(_get_fat_size(_tmp9,sizeof(char))== 1U &&(_tmpA == 0 && _tmpB != 0))_throw_arraybounds();*((char*)_tmp9.curr)=_tmpB;});}}
itemsep=temp;}}else{
# 149
return;}
# 151
({Cyc_Buffer_add_string(b,(struct _fat_ptr)itemsep);});
# 153
while(({char*_tmp74=(char*)desc.curr;_tmp74 != (char*)(_tag_fat(0,0,0)).curr;})){
desc=({Cyc_Arg_Justify_break_line(b,margin - indent,desc);});
if(({char*_tmp75=(char*)desc.curr;_tmp75 != (char*)(_tag_fat(0,0,0)).curr;}))
({Cyc_Buffer_add_string(b,(struct _fat_ptr)indentstr);});else{
# 158
({({struct Cyc_Buffer_t*_tmp76=b;Cyc_Buffer_add_string(_tmp76,({const char*_tmpE="\n";_tag_fat(_tmpE,sizeof(char),2U);}));});});}}
# 160
return;}}}
# 164
void Cyc_Arg_usage(struct Cyc_List_List*speclist,struct _fat_ptr errmsg){
# 166
({struct Cyc_String_pa_PrintArg_struct _tmp14=({struct Cyc_String_pa_PrintArg_struct _tmp62;_tmp62.tag=0U,_tmp62.f1=(struct _fat_ptr)((struct _fat_ptr)errmsg);_tmp62;});void*_tmp12[1U];_tmp12[0]=& _tmp14;({struct Cyc___cycFILE*_tmp78=Cyc_stderr;struct _fat_ptr _tmp77=({const char*_tmp13="%s\n";_tag_fat(_tmp13,sizeof(char),4U);});Cyc_fprintf(_tmp78,_tmp77,_tag_fat(_tmp12,sizeof(void*),1U));});});{
struct Cyc_Buffer_t*b=({Cyc_Buffer_create(1024U);});
while(speclist != 0){
({void(*_tmp15)(struct Cyc_Buffer_t*b,int indent,int margin,struct _fat_ptr item,struct _fat_ptr desc)=Cyc_Arg_Justify_justify_b;struct Cyc_Buffer_t*_tmp16=b;int _tmp17=12;int _tmp18=72;struct _fat_ptr _tmp19=(struct _fat_ptr)({Cyc_strconcat((struct _fat_ptr)(*((struct _tuple0*)speclist->hd)).f1,(struct _fat_ptr)(*((struct _tuple0*)speclist->hd)).f3);});struct _fat_ptr _tmp1A=(*((struct _tuple0*)speclist->hd)).f5;_tmp15(_tmp16,_tmp17,_tmp18,_tmp19,_tmp1A);});
# 172
speclist=speclist->tl;}
# 174
({struct Cyc_String_pa_PrintArg_struct _tmp1D=({struct Cyc_String_pa_PrintArg_struct _tmp63;_tmp63.tag=0U,({struct _fat_ptr _tmp79=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Buffer_contents(b);}));_tmp63.f1=_tmp79;});_tmp63;});void*_tmp1B[1U];_tmp1B[0]=& _tmp1D;({struct Cyc___cycFILE*_tmp7B=Cyc_stderr;struct _fat_ptr _tmp7A=({const char*_tmp1C="%s";_tag_fat(_tmp1C,sizeof(char),3U);});Cyc_fprintf(_tmp7B,_tmp7A,_tag_fat(_tmp1B,sizeof(void*),1U));});});}}
# 164
int Cyc_Arg_current=0;
# 178
static struct _fat_ptr Cyc_Arg_args={(void*)0,(void*)0,(void*)(0 + 0)};
# 180
static void Cyc_Arg_stop(int prog_pos,void*e,struct Cyc_List_List*speclist,struct _fat_ptr errmsg){
# 183
struct _fat_ptr progname=(unsigned)prog_pos < _get_fat_size(Cyc_Arg_args,sizeof(struct _fat_ptr))?*((struct _fat_ptr*)_check_fat_subscript(Cyc_Arg_args,sizeof(struct _fat_ptr),prog_pos)):({const char*_tmp39="(?)";_tag_fat(_tmp39,sizeof(char),4U);});
{void*_tmp1F=e;struct _fat_ptr _tmp20;struct _fat_ptr _tmp23;struct _fat_ptr _tmp22;struct _fat_ptr _tmp21;struct _fat_ptr _tmp24;struct _fat_ptr _tmp25;switch(*((int*)_tmp1F)){case 0U: _LL1: _tmp25=((struct Cyc_Arg_Unknown_Arg_error_struct*)_tmp1F)->f1;_LL2: {struct _fat_ptr s=_tmp25;
# 186
if(({({struct _fat_ptr _tmp7C=(struct _fat_ptr)s;Cyc_strcmp(_tmp7C,({const char*_tmp26="-help";_tag_fat(_tmp26,sizeof(char),6U);}));});})!= 0)
({struct Cyc_String_pa_PrintArg_struct _tmp29=({struct Cyc_String_pa_PrintArg_struct _tmp65;_tmp65.tag=0U,_tmp65.f1=(struct _fat_ptr)((struct _fat_ptr)progname);_tmp65;});struct Cyc_String_pa_PrintArg_struct _tmp2A=({struct Cyc_String_pa_PrintArg_struct _tmp64;_tmp64.tag=0U,_tmp64.f1=(struct _fat_ptr)((struct _fat_ptr)s);_tmp64;});void*_tmp27[2U];_tmp27[0]=& _tmp29,_tmp27[1]=& _tmp2A;({struct Cyc___cycFILE*_tmp7E=Cyc_stderr;struct _fat_ptr _tmp7D=({const char*_tmp28="%s: unknown option `%s'.\n";_tag_fat(_tmp28,sizeof(char),26U);});Cyc_fprintf(_tmp7E,_tmp7D,_tag_fat(_tmp27,sizeof(void*),2U));});});
# 186
goto _LL0;}case 1U: _LL3: _tmp24=((struct Cyc_Arg_Missing_Arg_error_struct*)_tmp1F)->f1;_LL4: {struct _fat_ptr s=_tmp24;
# 190
({struct Cyc_String_pa_PrintArg_struct _tmp2D=({struct Cyc_String_pa_PrintArg_struct _tmp67;_tmp67.tag=0U,_tmp67.f1=(struct _fat_ptr)((struct _fat_ptr)progname);_tmp67;});struct Cyc_String_pa_PrintArg_struct _tmp2E=({struct Cyc_String_pa_PrintArg_struct _tmp66;_tmp66.tag=0U,_tmp66.f1=(struct _fat_ptr)((struct _fat_ptr)s);_tmp66;});void*_tmp2B[2U];_tmp2B[0]=& _tmp2D,_tmp2B[1]=& _tmp2E;({struct Cyc___cycFILE*_tmp80=Cyc_stderr;struct _fat_ptr _tmp7F=({const char*_tmp2C="%s: option `%s' needs an argument.\n";_tag_fat(_tmp2C,sizeof(char),36U);});Cyc_fprintf(_tmp80,_tmp7F,_tag_fat(_tmp2B,sizeof(void*),2U));});});
goto _LL0;}case 3U: _LL5: _tmp21=((struct Cyc_Arg_Wrong_Arg_error_struct*)_tmp1F)->f1;_tmp22=((struct Cyc_Arg_Wrong_Arg_error_struct*)_tmp1F)->f2;_tmp23=((struct Cyc_Arg_Wrong_Arg_error_struct*)_tmp1F)->f3;_LL6: {struct _fat_ptr flag=_tmp21;struct _fat_ptr cmd=_tmp22;struct _fat_ptr t=_tmp23;
# 193
({struct Cyc_String_pa_PrintArg_struct _tmp31=({struct Cyc_String_pa_PrintArg_struct _tmp6B;_tmp6B.tag=0U,_tmp6B.f1=(struct _fat_ptr)((struct _fat_ptr)progname);_tmp6B;});struct Cyc_String_pa_PrintArg_struct _tmp32=({struct Cyc_String_pa_PrintArg_struct _tmp6A;_tmp6A.tag=0U,_tmp6A.f1=(struct _fat_ptr)((struct _fat_ptr)cmd);_tmp6A;});struct Cyc_String_pa_PrintArg_struct _tmp33=({struct Cyc_String_pa_PrintArg_struct _tmp69;_tmp69.tag=0U,_tmp69.f1=(struct _fat_ptr)((struct _fat_ptr)flag);_tmp69;});struct Cyc_String_pa_PrintArg_struct _tmp34=({struct Cyc_String_pa_PrintArg_struct _tmp68;_tmp68.tag=0U,_tmp68.f1=(struct _fat_ptr)((struct _fat_ptr)t);_tmp68;});void*_tmp2F[4U];_tmp2F[0]=& _tmp31,_tmp2F[1]=& _tmp32,_tmp2F[2]=& _tmp33,_tmp2F[3]=& _tmp34;({struct Cyc___cycFILE*_tmp82=Cyc_stderr;struct _fat_ptr _tmp81=({const char*_tmp30="%s: wrong argument `%s'; option `%s' expects %s.\n";_tag_fat(_tmp30,sizeof(char),50U);});Cyc_fprintf(_tmp82,_tmp81,_tag_fat(_tmp2F,sizeof(void*),4U));});});
# 195
goto _LL0;}default: _LL7: _tmp20=((struct Cyc_Arg_Message_Arg_error_struct*)_tmp1F)->f1;_LL8: {struct _fat_ptr s=_tmp20;
# 197
({struct Cyc_String_pa_PrintArg_struct _tmp37=({struct Cyc_String_pa_PrintArg_struct _tmp6D;_tmp6D.tag=0U,_tmp6D.f1=(struct _fat_ptr)((struct _fat_ptr)progname);_tmp6D;});struct Cyc_String_pa_PrintArg_struct _tmp38=({struct Cyc_String_pa_PrintArg_struct _tmp6C;_tmp6C.tag=0U,_tmp6C.f1=(struct _fat_ptr)((struct _fat_ptr)s);_tmp6C;});void*_tmp35[2U];_tmp35[0]=& _tmp37,_tmp35[1]=& _tmp38;({struct Cyc___cycFILE*_tmp84=Cyc_stderr;struct _fat_ptr _tmp83=({const char*_tmp36="%s: %s.\n";_tag_fat(_tmp36,sizeof(char),9U);});Cyc_fprintf(_tmp84,_tmp83,_tag_fat(_tmp35,sizeof(void*),2U));});});
goto _LL0;}}_LL0:;}
# 200
({Cyc_Arg_usage(speclist,errmsg);});
Cyc_Arg_current=(int)_get_fat_size(Cyc_Arg_args,sizeof(struct _fat_ptr));}
# 204
void Cyc_Arg_parse(struct Cyc_List_List*speclist,void(*anonfun)(struct _fat_ptr),int(*anonflagfun)(struct _fat_ptr),struct _fat_ptr errmsg,struct _fat_ptr orig_args){
# 209
Cyc_Arg_args=orig_args;{
# 211
int initpos=Cyc_Arg_current;
unsigned l=_get_fat_size(Cyc_Arg_args,sizeof(struct _fat_ptr));
# 214
if(({char*_tmp85=(char*)((struct _fat_ptr*)_check_fat_subscript(Cyc_Arg_args,sizeof(struct _fat_ptr),(int)(l - (unsigned)1)))->curr;_tmp85 == (char*)(_tag_fat(0,0,0)).curr;}))l=l - (unsigned)1;++ Cyc_Arg_current;
# 216
while((unsigned)Cyc_Arg_current < l){
struct _fat_ptr s=*((struct _fat_ptr*)_check_fat_subscript(Cyc_Arg_args,sizeof(struct _fat_ptr),Cyc_Arg_current));
if((({char*_tmp86=(char*)s.curr;_tmp86 != (char*)(_tag_fat(0,0,0)).curr;})&& _get_fat_size(s,sizeof(char))>= (unsigned)1)&&(int)((const char*)s.curr)[0]== (int)'-'){
void*action;
{struct _handler_cons _tmp3B;_push_handler(& _tmp3B);{int _tmp3D=0;if(setjmp(_tmp3B.handler))_tmp3D=1;if(!_tmp3D){action=({Cyc_Arg_lookup(speclist,s);});;_pop_handler();}else{void*_tmp3C=(void*)Cyc_Core_get_exn_thrown();void*_tmp3E=_tmp3C;void*_tmp3F;if(((struct Cyc_Core_Not_found_exn_struct*)_tmp3E)->tag == Cyc_Core_Not_found){_LL1: _LL2:
# 224
 if(!({anonflagfun(s);})){
({({int _tmp89=initpos;void*_tmp88=(void*)({struct Cyc_Arg_Unknown_Arg_error_struct*_tmp40=_cycalloc(sizeof(*_tmp40));_tmp40->tag=0U,_tmp40->f1=s;_tmp40;});struct Cyc_List_List*_tmp87=speclist;Cyc_Arg_stop(_tmp89,_tmp88,_tmp87,errmsg);});});
return;}else{
# 229
++ Cyc_Arg_current;
continue;}}else{_LL3: _tmp3F=_tmp3E;_LL4: {void*exn=_tmp3F;(int)_rethrow(exn);}}_LL0:;}}}
# 233
{struct _handler_cons _tmp41;_push_handler(& _tmp41);{int _tmp43=0;if(setjmp(_tmp41.handler))_tmp43=1;if(!_tmp43){
{void*_tmp44=action;void(*_tmp45)(struct _fat_ptr);void(*_tmp46)(int);void(*_tmp47)(struct _fat_ptr);void(*_tmp48)(struct _fat_ptr,struct _fat_ptr);int*_tmp49;int*_tmp4A;void(*_tmp4B)(struct _fat_ptr);void(*_tmp4C)();switch(*((int*)_tmp44)){case 0U: _LL6: _tmp4C=((struct Cyc_Arg_Unit_spec_Arg_Spec_struct*)_tmp44)->f1;_LL7: {void(*f)()=_tmp4C;
({f();});goto _LL5;}case 1U: _LL8: _tmp4B=((struct Cyc_Arg_Flag_spec_Arg_Spec_struct*)_tmp44)->f1;_LL9: {void(*f)(struct _fat_ptr)=_tmp4B;
({f(s);});goto _LL5;}case 3U: _LLA: _tmp4A=((struct Cyc_Arg_Set_spec_Arg_Spec_struct*)_tmp44)->f1;_LLB: {int*r=_tmp4A;
*r=1;goto _LL5;}case 4U: _LLC: _tmp49=((struct Cyc_Arg_Clear_spec_Arg_Spec_struct*)_tmp44)->f1;_LLD: {int*r=_tmp49;
*r=0;goto _LL5;}case 2U: _LLE: _tmp48=((struct Cyc_Arg_FlagString_spec_Arg_Spec_struct*)_tmp44)->f1;_LLF: {void(*f)(struct _fat_ptr,struct _fat_ptr)=_tmp48;
# 240
if((unsigned)(Cyc_Arg_current + 1)< l){
({f(s,*((struct _fat_ptr*)_check_fat_subscript(Cyc_Arg_args,sizeof(struct _fat_ptr),Cyc_Arg_current + 1)));});
++ Cyc_Arg_current;}else{
# 244
({({int _tmp8C=initpos;void*_tmp8B=(void*)({struct Cyc_Arg_Missing_Arg_error_struct*_tmp4D=_cycalloc(sizeof(*_tmp4D));_tmp4D->tag=1U,_tmp4D->f1=s;_tmp4D;});struct Cyc_List_List*_tmp8A=speclist;Cyc_Arg_stop(_tmp8C,_tmp8B,_tmp8A,errmsg);});});}
goto _LL5;}case 5U: _LL10: _tmp47=((struct Cyc_Arg_String_spec_Arg_Spec_struct*)_tmp44)->f1;_LL11: {void(*f)(struct _fat_ptr)=_tmp47;
# 247
if((unsigned)(Cyc_Arg_current + 1)< l){
({f(*((struct _fat_ptr*)_check_fat_subscript(Cyc_Arg_args,sizeof(struct _fat_ptr),Cyc_Arg_current + 1)));});
++ Cyc_Arg_current;}else{
# 251
({({int _tmp8F=initpos;void*_tmp8E=(void*)({struct Cyc_Arg_Missing_Arg_error_struct*_tmp4E=_cycalloc(sizeof(*_tmp4E));_tmp4E->tag=1U,_tmp4E->f1=s;_tmp4E;});struct Cyc_List_List*_tmp8D=speclist;Cyc_Arg_stop(_tmp8F,_tmp8E,_tmp8D,errmsg);});});}
goto _LL5;}case 6U: _LL12: _tmp46=((struct Cyc_Arg_Int_spec_Arg_Spec_struct*)_tmp44)->f1;_LL13: {void(*f)(int)=_tmp46;
# 254
struct _fat_ptr arg=*((struct _fat_ptr*)_check_fat_subscript(Cyc_Arg_args,sizeof(struct _fat_ptr),Cyc_Arg_current + 1));
int n=0;
if(({struct Cyc_IntPtr_sa_ScanfArg_struct _tmp51=({struct Cyc_IntPtr_sa_ScanfArg_struct _tmp6E;_tmp6E.tag=2U,_tmp6E.f1=& n;_tmp6E;});void*_tmp4F[1U];_tmp4F[0]=& _tmp51;({struct _fat_ptr _tmp91=arg;struct _fat_ptr _tmp90=({const char*_tmp50="%d";_tag_fat(_tmp50,sizeof(char),3U);});Cyc_sscanf(_tmp91,_tmp90,_tag_fat(_tmp4F,sizeof(void*),1U));});})!= 1){
({({int _tmp95=initpos;void*_tmp94=(void*)({struct Cyc_Arg_Wrong_Arg_error_struct*_tmp53=_cycalloc(sizeof(*_tmp53));_tmp53->tag=3U,_tmp53->f1=s,_tmp53->f2=arg,({struct _fat_ptr _tmp92=({const char*_tmp52="an integer";_tag_fat(_tmp52,sizeof(char),11U);});_tmp53->f3=_tmp92;});_tmp53;});struct Cyc_List_List*_tmp93=speclist;Cyc_Arg_stop(_tmp95,_tmp94,_tmp93,errmsg);});});
_npop_handler(0U);return;}
# 256
({f(n);});
# 261
++ Cyc_Arg_current;
goto _LL5;}default: _LL14: _tmp45=((struct Cyc_Arg_Rest_spec_Arg_Spec_struct*)_tmp44)->f1;_LL15: {void(*f)(struct _fat_ptr)=_tmp45;
# 264
while((unsigned)Cyc_Arg_current < l - (unsigned)1){
({f(*((struct _fat_ptr*)_check_fat_subscript(Cyc_Arg_args,sizeof(struct _fat_ptr),Cyc_Arg_current + 1)));});
++ Cyc_Arg_current;}
# 268
goto _LL5;}}_LL5:;}
# 234
;_pop_handler();}else{void*_tmp42=(void*)Cyc_Core_get_exn_thrown();void*_tmp54=_tmp42;void*_tmp55;struct _fat_ptr _tmp56;if(((struct Cyc_Arg_Bad_exn_struct*)_tmp54)->tag == Cyc_Arg_Bad){_LL17: _tmp56=((struct Cyc_Arg_Bad_exn_struct*)_tmp54)->f1;_LL18: {struct _fat_ptr s2=_tmp56;
# 271
({({int _tmp98=initpos;void*_tmp97=(void*)({struct Cyc_Arg_Message_Arg_error_struct*_tmp57=_cycalloc(sizeof(*_tmp57));_tmp57->tag=2U,_tmp57->f1=s2;_tmp57;});struct Cyc_List_List*_tmp96=speclist;Cyc_Arg_stop(_tmp98,_tmp97,_tmp96,errmsg);});});goto _LL16;}}else{_LL19: _tmp55=_tmp54;_LL1A: {void*exn=_tmp55;(int)_rethrow(exn);}}_LL16:;}}}
# 273
++ Cyc_Arg_current;}else{
# 276
{struct _handler_cons _tmp58;_push_handler(& _tmp58);{int _tmp5A=0;if(setjmp(_tmp58.handler))_tmp5A=1;if(!_tmp5A){({anonfun(s);});;_pop_handler();}else{void*_tmp59=(void*)Cyc_Core_get_exn_thrown();void*_tmp5B=_tmp59;void*_tmp5C;struct _fat_ptr _tmp5D;if(((struct Cyc_Arg_Bad_exn_struct*)_tmp5B)->tag == Cyc_Arg_Bad){_LL1C: _tmp5D=((struct Cyc_Arg_Bad_exn_struct*)_tmp5B)->f1;_LL1D: {struct _fat_ptr s2=_tmp5D;
# 278
({({int _tmp9B=initpos;void*_tmp9A=(void*)({struct Cyc_Arg_Message_Arg_error_struct*_tmp5E=_cycalloc(sizeof(*_tmp5E));_tmp5E->tag=2U,_tmp5E->f1=s2;_tmp5E;});struct Cyc_List_List*_tmp99=speclist;Cyc_Arg_stop(_tmp9B,_tmp9A,_tmp99,errmsg);});});goto _LL1B;}}else{_LL1E: _tmp5C=_tmp5B;_LL1F: {void*exn=_tmp5C;(int)_rethrow(exn);}}_LL1B:;}}}
# 280
++ Cyc_Arg_current;}}}}

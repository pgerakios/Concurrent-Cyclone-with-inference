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
extern struct _RegionHandle*Cyc_Core_unique_region;struct Cyc_Core_DynamicRegion;struct Cyc_Core_NewDynamicRegion{struct Cyc_Core_DynamicRegion*key;};
# 275 "core.h"
void Cyc_Core_rethrow(void*);
# 279
const char*Cyc_Core_get_exn_name(void*);
# 281
const char*Cyc_Core_get_exn_filename();
# 288
int Cyc_Core_get_exn_lineno();struct Cyc_Core_ThinRes{void*arr;unsigned nelts;};struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};
# 54 "list.h"
struct Cyc_List_List*Cyc_List_list(struct _fat_ptr);
# 76
struct Cyc_List_List*Cyc_List_map(void*(*f)(void*),struct Cyc_List_List*x);extern char Cyc_List_List_mismatch[14U];struct Cyc_List_List_mismatch_exn_struct{char*tag;};
# 172
struct Cyc_List_List*Cyc_List_rev(struct Cyc_List_List*x);
# 178
struct Cyc_List_List*Cyc_List_imp_rev(struct Cyc_List_List*x);
# 184
struct Cyc_List_List*Cyc_List_append(struct Cyc_List_List*x,struct Cyc_List_List*y);extern char Cyc_List_Nth[4U];struct Cyc_List_Nth_exn_struct{char*tag;};
# 391
struct Cyc_List_List*Cyc_List_filter(int(*f)(void*),struct Cyc_List_List*x);extern char Cyc_Arg_Bad[4U];struct Cyc_Arg_Bad_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Arg_Error[6U];struct Cyc_Arg_Error_exn_struct{char*tag;};struct Cyc_Arg_Unit_spec_Arg_Spec_struct{int tag;void(*f1)();};struct Cyc_Arg_Flag_spec_Arg_Spec_struct{int tag;void(*f1)(struct _fat_ptr);};struct Cyc_Arg_FlagString_spec_Arg_Spec_struct{int tag;void(*f1)(struct _fat_ptr,struct _fat_ptr);};struct Cyc_Arg_Set_spec_Arg_Spec_struct{int tag;int*f1;};struct Cyc_Arg_Clear_spec_Arg_Spec_struct{int tag;int*f1;};struct Cyc_Arg_String_spec_Arg_Spec_struct{int tag;void(*f1)(struct _fat_ptr);};struct Cyc_Arg_Int_spec_Arg_Spec_struct{int tag;void(*f1)(int);};struct Cyc_Arg_Rest_spec_Arg_Spec_struct{int tag;void(*f1)(struct _fat_ptr);};
# 66 "arg.h"
void Cyc_Arg_usage(struct Cyc_List_List*,struct _fat_ptr);
# 69
extern int Cyc_Arg_current;
# 71
void Cyc_Arg_parse(struct Cyc_List_List*specs,void(*anonfun)(struct _fat_ptr),int(*anonflagfun)(struct _fat_ptr),struct _fat_ptr errmsg,struct _fat_ptr args);struct Cyc___cycFILE;
# 51 "cycboot.h"
extern struct Cyc___cycFILE*Cyc_stdout;
# 53
extern struct Cyc___cycFILE*Cyc_stderr;struct Cyc_String_pa_PrintArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Int_pa_PrintArg_struct{int tag;unsigned long f1;};struct Cyc_Double_pa_PrintArg_struct{int tag;double f1;};struct Cyc_LongDouble_pa_PrintArg_struct{int tag;long double f1;};struct Cyc_ShortPtr_pa_PrintArg_struct{int tag;short*f1;};struct Cyc_IntPtr_pa_PrintArg_struct{int tag;unsigned long*f1;};
# 73
struct _fat_ptr Cyc_aprintf(struct _fat_ptr,struct _fat_ptr);
# 79
int Cyc_fclose(struct Cyc___cycFILE*);
# 84
int Cyc_feof(struct Cyc___cycFILE*);
# 88
int Cyc_fflush(struct Cyc___cycFILE*);
# 98
struct Cyc___cycFILE*Cyc_fopen(const char*,const char*);
# 100
int Cyc_fprintf(struct Cyc___cycFILE*,struct _fat_ptr,struct _fat_ptr);
# 108
unsigned long Cyc_fread(struct _fat_ptr,unsigned long,unsigned long,struct Cyc___cycFILE*);struct Cyc_ShortPtr_sa_ScanfArg_struct{int tag;short*f1;};struct Cyc_UShortPtr_sa_ScanfArg_struct{int tag;unsigned short*f1;};struct Cyc_IntPtr_sa_ScanfArg_struct{int tag;int*f1;};struct Cyc_UIntPtr_sa_ScanfArg_struct{int tag;unsigned*f1;};struct Cyc_StringPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_DoublePtr_sa_ScanfArg_struct{int tag;double*f1;};struct Cyc_FloatPtr_sa_ScanfArg_struct{int tag;float*f1;};struct Cyc_CharPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};
# 140 "cycboot.h"
unsigned long Cyc_fwrite(struct _fat_ptr,unsigned long,unsigned long,struct Cyc___cycFILE*);
# 157 "cycboot.h"
int Cyc_printf(struct _fat_ptr,struct _fat_ptr);
# 167
int remove(const char*);extern char Cyc_FileCloseError[15U];struct Cyc_FileCloseError_exn_struct{char*tag;};extern char Cyc_FileOpenError[14U];struct Cyc_FileOpenError_exn_struct{char*tag;struct _fat_ptr f1;};
# 271 "cycboot.h"
struct Cyc___cycFILE*Cyc_file_open(struct _fat_ptr,struct _fat_ptr);
void Cyc_file_close(struct Cyc___cycFILE*);
# 313
char*getenv(const char*);
# 318
int system(const char*);
void exit(int);
# 38 "string.h"
unsigned long Cyc_strlen(struct _fat_ptr s);
# 49 "string.h"
int Cyc_strcmp(struct _fat_ptr s1,struct _fat_ptr s2);
# 51
int Cyc_strncmp(struct _fat_ptr s1,struct _fat_ptr s2,unsigned long len);
# 62
struct _fat_ptr Cyc_strconcat(struct _fat_ptr,struct _fat_ptr);
# 64
struct _fat_ptr Cyc_strconcat_l(struct Cyc_List_List*);
# 66
struct _fat_ptr Cyc_str_sepstr(struct Cyc_List_List*,struct _fat_ptr);
# 104 "string.h"
struct _fat_ptr Cyc_strdup(struct _fat_ptr src);
# 109
struct _fat_ptr Cyc_substring(struct _fat_ptr,int ofs,unsigned long n);
# 120
struct _fat_ptr Cyc_strchr(struct _fat_ptr s,char c);
# 30 "filename.h"
struct _fat_ptr Cyc_Filename_concat(struct _fat_ptr,struct _fat_ptr);
# 34
struct _fat_ptr Cyc_Filename_chop_extension(struct _fat_ptr);
# 40
struct _fat_ptr Cyc_Filename_dirname(struct _fat_ptr);
# 43
struct _fat_ptr Cyc_Filename_basename(struct _fat_ptr);
# 46
int Cyc_Filename_check_suffix(struct _fat_ptr,struct _fat_ptr);
# 28 "position.h"
void Cyc_Position_reset_position(struct _fat_ptr);struct Cyc_Position_Error;
# 46
extern int Cyc_Position_use_gcc_style_location;
# 48
extern int Cyc_Position_max_errors;
# 50
int Cyc_Position_error_p();struct Cyc_Iter_Iter{void*env;int(*next)(void*env,void*dest);};struct Cyc_Dict_T;struct Cyc_Dict_Dict{int(*rel)(void*,void*);struct _RegionHandle*r;const struct Cyc_Dict_T*t;};extern char Cyc_Dict_Present[8U];struct Cyc_Dict_Present_exn_struct{char*tag;};extern char Cyc_Dict_Absent[7U];struct Cyc_Dict_Absent_exn_struct{char*tag;};struct Cyc_Relations_Reln;struct _union_Nmspace_Rel_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Abs_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_C_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Loc_n{int tag;int val;};union Cyc_Absyn_Nmspace{struct _union_Nmspace_Rel_n Rel_n;struct _union_Nmspace_Abs_n Abs_n;struct _union_Nmspace_C_n C_n;struct _union_Nmspace_Loc_n Loc_n;};struct _tuple0{union Cyc_Absyn_Nmspace f1;struct _fat_ptr*f2;};
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
# 1237 "absyn.h"
extern int Cyc_Absyn_porting_c_code;
# 1239
extern int Cyc_Absyn_no_regions;
# 1411
void Cyc_Absyn_set_pthread(int b);extern char Cyc_Lexing_Error[6U];struct Cyc_Lexing_Error_exn_struct{char*tag;struct _fat_ptr f1;};struct Cyc_Lexing_lexbuf{void(*refill_buff)(struct Cyc_Lexing_lexbuf*);void*refill_state;struct _fat_ptr lex_buffer;int lex_buffer_len;int lex_abs_pos;int lex_start_pos;int lex_curr_pos;int lex_last_pos;int lex_last_action;int lex_eof_reached;};struct Cyc_Lexing_function_lexbuf_state{int(*read_fun)(struct _fat_ptr,int,void*);void*read_fun_state;};struct Cyc_Lexing_lex_tables{struct _fat_ptr lex_base;struct _fat_ptr lex_backtrk;struct _fat_ptr lex_default;struct _fat_ptr lex_trans;struct _fat_ptr lex_check;};
# 28 "parse.h"
struct Cyc_List_List*Cyc_Parse_parse_file(struct Cyc___cycFILE*f,struct _fat_ptr name);
extern int Cyc_Parse_no_register;extern char Cyc_Parse_Exit[5U];struct Cyc_Parse_Exit_exn_struct{char*tag;};struct Cyc_FlatList{struct Cyc_FlatList*tl;void*hd[0U] __attribute__((aligned )) ;};struct Cyc_Type_specifier{int Signed_spec: 1;int Unsigned_spec: 1;int Short_spec: 1;int Long_spec: 1;int Long_Long_spec: 1;int Valid_type_spec: 1;void*Type_spec;unsigned loc;};struct Cyc_Declarator{struct _tuple0*id;struct Cyc_List_List*tms;};struct _tuple13{struct Cyc_Declarator f1;struct Cyc_Absyn_Exp*f2;};struct _tuple12{struct _tuple12*tl;struct _tuple13 hd  __attribute__((aligned )) ;};
# 52
enum Cyc_Storage_class{Cyc_Typedef_sc =0U,Cyc_Extern_sc =1U,Cyc_ExternC_sc =2U,Cyc_Static_sc =3U,Cyc_Auto_sc =4U,Cyc_Register_sc =5U,Cyc_Abstract_sc =6U};struct Cyc_Declaration_spec{enum Cyc_Storage_class*sc;struct Cyc_Absyn_Tqual tq;struct Cyc_Type_specifier type_specs;int is_inline;struct Cyc_List_List*attributes;};struct Cyc_Abstractdeclarator{struct Cyc_List_List*tms;};struct Cyc_Numelts_ptrqual_Pointer_qual_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Region_ptrqual_Pointer_qual_struct{int tag;void*f1;};struct Cyc_Thin_ptrqual_Pointer_qual_struct{int tag;};struct Cyc_Fat_ptrqual_Pointer_qual_struct{int tag;};struct Cyc_Zeroterm_ptrqual_Pointer_qual_struct{int tag;};struct Cyc_Nozeroterm_ptrqual_Pointer_qual_struct{int tag;};struct Cyc_Notnull_ptrqual_Pointer_qual_struct{int tag;};struct Cyc_Nullable_ptrqual_Pointer_qual_struct{int tag;};struct _union_YYSTYPE_Int_tok{int tag;union Cyc_Absyn_Cnst val;};struct _union_YYSTYPE_Char_tok{int tag;char val;};struct _union_YYSTYPE_String_tok{int tag;struct _fat_ptr val;};struct _union_YYSTYPE_Stringopt_tok{int tag;struct Cyc_Core_Opt*val;};struct _union_YYSTYPE_QualId_tok{int tag;struct _tuple0*val;};struct _tuple14{int f1;struct _fat_ptr f2;};struct _union_YYSTYPE_Asm_tok{int tag;struct _tuple14 val;};struct _union_YYSTYPE_Exp_tok{int tag;struct Cyc_Absyn_Exp*val;};struct _union_YYSTYPE_Stmt_tok{int tag;struct Cyc_Absyn_Stmt*val;};struct _tuple15{unsigned f1;void*f2;void*f3;};struct _union_YYSTYPE_YY1{int tag;struct _tuple15*val;};struct _union_YYSTYPE_YY2{int tag;void*val;};struct _union_YYSTYPE_YY3{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY4{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY5{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY6{int tag;enum Cyc_Absyn_Primop val;};struct _union_YYSTYPE_YY7{int tag;struct Cyc_Core_Opt*val;};struct _union_YYSTYPE_YY8{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY9{int tag;struct Cyc_Absyn_Pat*val;};struct _tuple16{struct Cyc_List_List*f1;int f2;};struct _union_YYSTYPE_YY10{int tag;struct _tuple16*val;};struct _union_YYSTYPE_YY11{int tag;struct Cyc_List_List*val;};struct _tuple17{struct Cyc_List_List*f1;struct Cyc_Absyn_Pat*f2;};struct _union_YYSTYPE_YY12{int tag;struct _tuple17*val;};struct _union_YYSTYPE_YY13{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY14{int tag;struct _tuple16*val;};struct _union_YYSTYPE_YY15{int tag;struct Cyc_Absyn_Fndecl*val;};struct _union_YYSTYPE_YY16{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY17{int tag;struct Cyc_Declaration_spec val;};struct _union_YYSTYPE_YY18{int tag;struct _tuple13 val;};struct _union_YYSTYPE_YY19{int tag;struct _tuple12*val;};struct _union_YYSTYPE_YY20{int tag;enum Cyc_Storage_class*val;};struct _union_YYSTYPE_YY21{int tag;struct Cyc_Type_specifier val;};struct _union_YYSTYPE_YY22{int tag;enum Cyc_Absyn_AggrKind val;};struct _union_YYSTYPE_YY23{int tag;struct Cyc_Absyn_Tqual val;};struct _union_YYSTYPE_YY24{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY25{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY26{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY27{int tag;struct Cyc_Declarator val;};struct _tuple18{struct Cyc_Declarator f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;};struct _union_YYSTYPE_YY28{int tag;struct _tuple18*val;};struct _union_YYSTYPE_YY29{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY30{int tag;struct Cyc_Abstractdeclarator val;};struct _union_YYSTYPE_YY31{int tag;int val;};struct _union_YYSTYPE_YY32{int tag;enum Cyc_Absyn_Scope val;};struct _union_YYSTYPE_YY33{int tag;struct Cyc_Absyn_Datatypefield*val;};struct _union_YYSTYPE_YY34{int tag;struct Cyc_List_List*val;};struct _tuple19{struct Cyc_Absyn_Tqual f1;struct Cyc_Type_specifier f2;struct Cyc_List_List*f3;};struct _union_YYSTYPE_YY35{int tag;struct _tuple19 val;};struct _union_YYSTYPE_YY36{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY37{int tag;struct _tuple9*val;};struct _union_YYSTYPE_YY38{int tag;struct Cyc_List_List*val;};struct _tuple20{struct Cyc_List_List*f1;int f2;struct Cyc_Absyn_VarargInfo*f3;void*f4;struct Cyc_List_List*f5;};struct _union_YYSTYPE_YY39{int tag;struct _tuple20*val;};struct _union_YYSTYPE_YY40{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY41{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY42{int tag;void*val;};struct _union_YYSTYPE_YY43{int tag;struct Cyc_Absyn_Kind*val;};struct _union_YYSTYPE_YY44{int tag;void*val;};struct _union_YYSTYPE_YY45{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY46{int tag;void*val;};struct _union_YYSTYPE_YY47{int tag;struct Cyc_Absyn_Enumfield*val;};struct _union_YYSTYPE_YY48{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY49{int tag;void*val;};struct _union_YYSTYPE_YY50{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY51{int tag;void*val;};struct _union_YYSTYPE_YY52{int tag;struct Cyc_List_List*val;};struct _tuple21{struct Cyc_List_List*f1;unsigned f2;};struct _union_YYSTYPE_YY53{int tag;struct _tuple21*val;};struct _union_YYSTYPE_YY54{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY55{int tag;void*val;};struct _union_YYSTYPE_YY56{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY57{int tag;struct Cyc_Absyn_Exp*val;};struct _union_YYSTYPE_YY58{int tag;void*val;};struct _tuple22{struct Cyc_List_List*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;};struct _union_YYSTYPE_YY59{int tag;struct _tuple22*val;};struct _union_YYSTYPE_YY60{int tag;struct _tuple8*val;};struct _union_YYSTYPE_YY61{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY62{int tag;struct Cyc_List_List*val;};struct _tuple23{struct _fat_ptr f1;struct Cyc_Absyn_Exp*f2;};struct _union_YYSTYPE_YY63{int tag;struct _tuple23*val;};struct _union_YYSTYPE_YY64{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY65{int tag;int val;};struct _union_YYSTYPE_YY66{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YYINITIALSVAL{int tag;int val;};union Cyc_YYSTYPE{struct _union_YYSTYPE_Int_tok Int_tok;struct _union_YYSTYPE_Char_tok Char_tok;struct _union_YYSTYPE_String_tok String_tok;struct _union_YYSTYPE_Stringopt_tok Stringopt_tok;struct _union_YYSTYPE_QualId_tok QualId_tok;struct _union_YYSTYPE_Asm_tok Asm_tok;struct _union_YYSTYPE_Exp_tok Exp_tok;struct _union_YYSTYPE_Stmt_tok Stmt_tok;struct _union_YYSTYPE_YY1 YY1;struct _union_YYSTYPE_YY2 YY2;struct _union_YYSTYPE_YY3 YY3;struct _union_YYSTYPE_YY4 YY4;struct _union_YYSTYPE_YY5 YY5;struct _union_YYSTYPE_YY6 YY6;struct _union_YYSTYPE_YY7 YY7;struct _union_YYSTYPE_YY8 YY8;struct _union_YYSTYPE_YY9 YY9;struct _union_YYSTYPE_YY10 YY10;struct _union_YYSTYPE_YY11 YY11;struct _union_YYSTYPE_YY12 YY12;struct _union_YYSTYPE_YY13 YY13;struct _union_YYSTYPE_YY14 YY14;struct _union_YYSTYPE_YY15 YY15;struct _union_YYSTYPE_YY16 YY16;struct _union_YYSTYPE_YY17 YY17;struct _union_YYSTYPE_YY18 YY18;struct _union_YYSTYPE_YY19 YY19;struct _union_YYSTYPE_YY20 YY20;struct _union_YYSTYPE_YY21 YY21;struct _union_YYSTYPE_YY22 YY22;struct _union_YYSTYPE_YY23 YY23;struct _union_YYSTYPE_YY24 YY24;struct _union_YYSTYPE_YY25 YY25;struct _union_YYSTYPE_YY26 YY26;struct _union_YYSTYPE_YY27 YY27;struct _union_YYSTYPE_YY28 YY28;struct _union_YYSTYPE_YY29 YY29;struct _union_YYSTYPE_YY30 YY30;struct _union_YYSTYPE_YY31 YY31;struct _union_YYSTYPE_YY32 YY32;struct _union_YYSTYPE_YY33 YY33;struct _union_YYSTYPE_YY34 YY34;struct _union_YYSTYPE_YY35 YY35;struct _union_YYSTYPE_YY36 YY36;struct _union_YYSTYPE_YY37 YY37;struct _union_YYSTYPE_YY38 YY38;struct _union_YYSTYPE_YY39 YY39;struct _union_YYSTYPE_YY40 YY40;struct _union_YYSTYPE_YY41 YY41;struct _union_YYSTYPE_YY42 YY42;struct _union_YYSTYPE_YY43 YY43;struct _union_YYSTYPE_YY44 YY44;struct _union_YYSTYPE_YY45 YY45;struct _union_YYSTYPE_YY46 YY46;struct _union_YYSTYPE_YY47 YY47;struct _union_YYSTYPE_YY48 YY48;struct _union_YYSTYPE_YY49 YY49;struct _union_YYSTYPE_YY50 YY50;struct _union_YYSTYPE_YY51 YY51;struct _union_YYSTYPE_YY52 YY52;struct _union_YYSTYPE_YY53 YY53;struct _union_YYSTYPE_YY54 YY54;struct _union_YYSTYPE_YY55 YY55;struct _union_YYSTYPE_YY56 YY56;struct _union_YYSTYPE_YY57 YY57;struct _union_YYSTYPE_YY58 YY58;struct _union_YYSTYPE_YY59 YY59;struct _union_YYSTYPE_YY60 YY60;struct _union_YYSTYPE_YY61 YY61;struct _union_YYSTYPE_YY62 YY62;struct _union_YYSTYPE_YY63 YY63;struct _union_YYSTYPE_YY64 YY64;struct _union_YYSTYPE_YY65 YY65;struct _union_YYSTYPE_YY66 YY66;struct _union_YYSTYPE_YYINITIALSVAL YYINITIALSVAL;};struct Cyc_Yyltype{int timestamp;int first_line;int first_column;int last_line;int last_column;};struct Cyc_PP_Ppstate;struct Cyc_PP_Out;struct Cyc_PP_Doc;struct Cyc_Absynpp_Params{int expand_typedefs;int qvar_to_Cids;int add_cyc_prefix;int to_VC;int decls_first;int rewrite_temp_tvars;int print_all_tvars;int print_all_kinds;int print_all_effects;int print_using_stmts;int print_externC_stmts;int print_full_evars;int print_zeroterm;int generate_line_directives;int use_curr_namespace;struct Cyc_List_List*curr_namespace;};
# 53 "absynpp.h"
void Cyc_Absynpp_set_params(struct Cyc_Absynpp_Params*fs);
# 55
extern struct Cyc_Absynpp_Params Cyc_Absynpp_cyc_params_r;extern struct Cyc_Absynpp_Params Cyc_Absynpp_cyci_params_r;extern struct Cyc_Absynpp_Params Cyc_Absynpp_c_params_r;extern struct Cyc_Absynpp_Params Cyc_Absynpp_tc_params_r;
# 57
void Cyc_Absynpp_decllist2file(struct Cyc_List_List*tdl,struct Cyc___cycFILE*f);
# 26 "absyndump.h"
void Cyc_Absyndump_set_params(struct Cyc_Absynpp_Params*fs);
void Cyc_Absyndump_dumpdecllist2file(struct Cyc_List_List*tdl,struct Cyc___cycFILE*f);
void Cyc_Absyndump_dump_interface(struct Cyc_List_List*ds,struct Cyc___cycFILE*f);
# 29 "binding.h"
extern int Cyc_Binding_warn_override;
void Cyc_Binding_resolve_all(struct Cyc_List_List*tds);struct Cyc_Binding_Namespace_Binding_NSDirective_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Binding_Using_Binding_NSDirective_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Binding_NSCtxt{struct Cyc_List_List*curr_ns;struct Cyc_List_List*availables;struct Cyc_Dict_Dict ns_data;};struct _union_RelnOp_RConst{int tag;unsigned val;};struct _union_RelnOp_RVar{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RNumelts{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RType{int tag;void*val;};struct _union_RelnOp_RParam{int tag;unsigned val;};struct _union_RelnOp_RParamNumelts{int tag;unsigned val;};struct _union_RelnOp_RReturn{int tag;unsigned val;};union Cyc_Relations_RelnOp{struct _union_RelnOp_RConst RConst;struct _union_RelnOp_RVar RVar;struct _union_RelnOp_RNumelts RNumelts;struct _union_RelnOp_RType RType;struct _union_RelnOp_RParam RParam;struct _union_RelnOp_RParamNumelts RParamNumelts;struct _union_RelnOp_RReturn RReturn;};
# 50 "relations-ap.h"
enum Cyc_Relations_Relation{Cyc_Relations_Req =0U,Cyc_Relations_Rneq =1U,Cyc_Relations_Rlte =2U,Cyc_Relations_Rlt =3U};struct Cyc_Relations_Reln{union Cyc_Relations_RelnOp rop1;enum Cyc_Relations_Relation relation;union Cyc_Relations_RelnOp rop2;};struct Cyc_RgnOrder_RgnPO;extern char Cyc_Tcenv_Env_error[10U];struct Cyc_Tcenv_Env_error_exn_struct{char*tag;};struct Cyc_Tcenv_Genv{struct Cyc_Dict_Dict aggrdecls;struct Cyc_Dict_Dict datatypedecls;struct Cyc_Dict_Dict enumdecls;struct Cyc_Dict_Dict typedefs;struct Cyc_Dict_Dict ordinaries;};struct Cyc_Tcenv_Fenv;struct Cyc_Tcenv_Tenv{struct Cyc_List_List*ns;struct Cyc_Tcenv_Genv*ae;struct Cyc_Tcenv_Fenv*le;int allow_valueof: 1;int in_extern_c_include: 1;int in_tempest: 1;int tempest_generalize: 1;};
# 68 "tcenv.h"
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_tc_init();
# 90
enum Cyc_Tcenv_NewStatus{Cyc_Tcenv_NoneNew =0U,Cyc_Tcenv_InNew =1U,Cyc_Tcenv_InNewAggr =2U};
# 132 "tcutil.h"
extern int Cyc_Tcutil_warn_alias_coerce;
# 135
extern int Cyc_Tcutil_warn_region_coerce;
# 31 "tc.h"
extern int Cyc_Tc_aggressive_warn;
# 33
void Cyc_Tc_tc(struct Cyc_Tcenv_Tenv*te,int var_default_init,struct Cyc_List_List*ds,int);
# 37
struct Cyc_List_List*Cyc_Tc_treeshake(struct Cyc_Tcenv_Tenv*te,struct Cyc_List_List*);struct Cyc_Hashtable_Table;
# 34 "toc.h"
struct Cyc_List_List*Cyc_Toc_toc(struct Cyc_Hashtable_Table*pop_tables,struct Cyc_List_List*ds);extern char Cyc_Toc_Dest[5U];struct Cyc_Toc_Dest_Absyn_AbsynAnnot_struct{char*tag;struct Cyc_Absyn_Exp*f1;};
# 53
void Cyc_Toc_finish();
# 26 "remove_aggregates.h"
struct Cyc_List_List*Cyc_RemoveAggrs_remove_aggrs(struct Cyc_List_List*decls);
# 26 "toseqc.h"
struct Cyc_List_List*Cyc_Toseqc_toseqc(struct Cyc_List_List*decls);
# 27 "tovc.h"
extern int Cyc_Tovc_elim_array_initializers;
struct Cyc_List_List*Cyc_Tovc_tovc(struct Cyc_List_List*decls);struct Cyc_Set_Set;extern char Cyc_Set_Absent[7U];struct Cyc_Set_Absent_exn_struct{char*tag;};struct Cyc_JumpAnalysis_Jump_Anal_Result{struct Cyc_Hashtable_Table*pop_tables;struct Cyc_Hashtable_Table*succ_tables;struct Cyc_Hashtable_Table*pat_pop_tables;};
# 46 "jump_analysis.h"
struct Cyc_JumpAnalysis_Jump_Anal_Result*Cyc_JumpAnalysis_jump_analysis(struct Cyc_List_List*tds);
# 41 "cf_flowinfo.h"
int Cyc_CfFlowInfo_anal_error;struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_CfFlowInfo_MallocPt_CfFlowInfo_Root_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;};struct Cyc_CfFlowInfo_InitParam_CfFlowInfo_Root_struct{int tag;int f1;void*f2;};struct Cyc_CfFlowInfo_Dot_CfFlowInfo_PathCon_struct{int tag;int f1;};struct Cyc_CfFlowInfo_Star_CfFlowInfo_PathCon_struct{int tag;};struct Cyc_CfFlowInfo_Place{void*root;struct Cyc_List_List*path;};
# 74
enum Cyc_CfFlowInfo_InitLevel{Cyc_CfFlowInfo_NoneIL =0U,Cyc_CfFlowInfo_AllIL =1U};extern char Cyc_CfFlowInfo_IsZero[7U];struct Cyc_CfFlowInfo_IsZero_Absyn_AbsynAnnot_struct{char*tag;};extern char Cyc_CfFlowInfo_NotZero[8U];struct Cyc_CfFlowInfo_NotZero_Absyn_AbsynAnnot_struct{char*tag;struct Cyc_List_List*f1;};extern char Cyc_CfFlowInfo_UnknownZ[9U];struct Cyc_CfFlowInfo_UnknownZ_Absyn_AbsynAnnot_struct{char*tag;struct Cyc_List_List*f1;};struct _union_AbsLVal_PlaceL{int tag;struct Cyc_CfFlowInfo_Place*val;};struct _union_AbsLVal_UnknownL{int tag;int val;};union Cyc_CfFlowInfo_AbsLVal{struct _union_AbsLVal_PlaceL PlaceL;struct _union_AbsLVal_UnknownL UnknownL;};struct Cyc_CfFlowInfo_UnionRInfo{int is_union;int fieldnum;};struct Cyc_CfFlowInfo_Zero_CfFlowInfo_AbsRVal_struct{int tag;};struct Cyc_CfFlowInfo_NotZeroAll_CfFlowInfo_AbsRVal_struct{int tag;};struct Cyc_CfFlowInfo_UnknownR_CfFlowInfo_AbsRVal_struct{int tag;enum Cyc_CfFlowInfo_InitLevel f1;};struct Cyc_CfFlowInfo_Esc_CfFlowInfo_AbsRVal_struct{int tag;enum Cyc_CfFlowInfo_InitLevel f1;};struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct{int tag;struct Cyc_CfFlowInfo_Place*f1;};struct Cyc_CfFlowInfo_UniquePtr_CfFlowInfo_AbsRVal_struct{int tag;void*f1;};struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct{int tag;struct Cyc_CfFlowInfo_UnionRInfo f1;struct _fat_ptr f2;};struct Cyc_CfFlowInfo_Consumed_CfFlowInfo_AbsRVal_struct{int tag;struct Cyc_Absyn_Exp*f1;int f2;void*f3;};struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct{int tag;struct Cyc_Absyn_Vardecl*f1;void*f2;};struct _union_FlowInfo_BottomFL{int tag;int val;};struct _tuple24{struct Cyc_Dict_Dict f1;struct Cyc_List_List*f2;};struct _union_FlowInfo_ReachableFL{int tag;struct _tuple24 val;};union Cyc_CfFlowInfo_FlowInfo{struct _union_FlowInfo_BottomFL BottomFL;struct _union_FlowInfo_ReachableFL ReachableFL;};struct Cyc_CfFlowInfo_FlowEnv{void*zero;void*notzeroall;void*unknown_none;void*unknown_all;void*esc_none;void*esc_all;struct Cyc_Dict_Dict mt_flowdict;struct Cyc_CfFlowInfo_Place*dummy_place;};
# 32 "new_control_flow.h"
void Cyc_NewControlFlow_cf_check(struct Cyc_JumpAnalysis_Jump_Anal_Result*tables,struct Cyc_List_List*ds);
# 37
extern int Cyc_NewControlFlow_warn_lose_unique;extern char Cyc_InsertChecks_FatBound[9U];struct Cyc_InsertChecks_FatBound_Absyn_AbsynAnnot_struct{char*tag;};extern char Cyc_InsertChecks_NoCheck[8U];struct Cyc_InsertChecks_NoCheck_Absyn_AbsynAnnot_struct{char*tag;};extern char Cyc_InsertChecks_NullAndFatBound[16U];struct Cyc_InsertChecks_NullAndFatBound_Absyn_AbsynAnnot_struct{char*tag;};extern char Cyc_InsertChecks_NullAndThinBound[17U];struct Cyc_InsertChecks_NullAndThinBound_Absyn_AbsynAnnot_struct{char*tag;struct Cyc_Absyn_Exp*f1;};extern char Cyc_InsertChecks_NullOnly[9U];struct Cyc_InsertChecks_NullOnly_Absyn_AbsynAnnot_struct{char*tag;};extern char Cyc_InsertChecks_ThinBound[10U];struct Cyc_InsertChecks_ThinBound_Absyn_AbsynAnnot_struct{char*tag;struct Cyc_Absyn_Exp*f1;};
# 40 "insert_checks.h"
extern int Cyc_InsertChecks_warn_bounds_checks;
extern int Cyc_InsertChecks_warn_all_null_deref;
# 43
void Cyc_InsertChecks_insert_checks(struct Cyc_List_List*,struct Cyc_JumpAnalysis_Jump_Anal_Result*tables);struct Cyc_Interface_I;
# 36 "interface.h"
struct Cyc_Interface_I*Cyc_Interface_empty();
# 45 "interface.h"
struct Cyc_Interface_I*Cyc_Interface_final();
# 49
struct Cyc_Interface_I*Cyc_Interface_extract(struct Cyc_Tcenv_Genv*,struct Cyc_List_List*);struct _tuple25{struct _fat_ptr f1;struct _fat_ptr f2;};
# 57
int Cyc_Interface_is_subinterface(struct Cyc_Interface_I*i1,struct Cyc_Interface_I*i2,struct _tuple25*info);
# 72 "interface.h"
struct Cyc_Interface_I*Cyc_Interface_get_and_merge_list(struct Cyc_Interface_I*(*get)(void*),struct Cyc_List_List*la,struct Cyc_List_List*linfo);
# 78
struct Cyc_Interface_I*Cyc_Interface_parse(struct Cyc___cycFILE*);
# 81
void Cyc_Interface_save(struct Cyc_Interface_I*,struct Cyc___cycFILE*);
# 84
struct Cyc_Interface_I*Cyc_Interface_load(struct Cyc___cycFILE*);
# 27 "ioeffect.h"
void Cyc_IOEffect_analysis(struct Cyc_JumpAnalysis_Jump_Anal_Result*tables,struct Cyc_List_List*tds);
# 31 "warn.h"
void Cyc_Warn_flush_warnings();struct Cyc_Warn_String_Warn_Warg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Warn_Exp_Warn_Warg_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Warn_Typ_Warn_Warg_struct{int tag;void*f1;};struct Cyc_Warn_Qvar_Warn_Warg_struct{int tag;struct _tuple0*f1;};
# 29 "specsfile.h"
struct _fat_ptr Cyc_Specsfile_target_arch;
void Cyc_Specsfile_set_target_arch(struct _fat_ptr s);
struct Cyc_List_List*Cyc_Specsfile_cyclone_exec_path;
void Cyc_Specsfile_add_cyclone_exec_path(struct _fat_ptr s);
# 34
struct Cyc_List_List*Cyc_Specsfile_read_specs(struct _fat_ptr file);
struct _fat_ptr Cyc_Specsfile_split_specs(struct _fat_ptr cmdline);
struct _fat_ptr Cyc_Specsfile_get_spec(struct Cyc_List_List*specs,struct _fat_ptr spec_name);
struct Cyc_List_List*Cyc_Specsfile_cyclone_arch_path;
struct _fat_ptr Cyc_Specsfile_def_lib_path;
struct _fat_ptr Cyc_Specsfile_parse_b(struct Cyc_List_List*specs,void(*anonfun)(struct _fat_ptr),int(*anonflagfun)(struct _fat_ptr),struct _fat_ptr errmsg,struct _fat_ptr argv);
# 44
struct _fat_ptr Cyc_Specsfile_find_in_arch_path(struct _fat_ptr s);
struct _fat_ptr Cyc_Specsfile_find_in_exec_path(struct _fat_ptr s);
# 24 "cyclone.h"
extern int Cyc_Cyclone_tovc_r;
# 26
enum Cyc_Cyclone_C_Compilers{Cyc_Cyclone_Gcc_c =0U,Cyc_Cyclone_Vc_c =1U};
# 32
extern enum Cyc_Cyclone_C_Compilers Cyc_Cyclone_c_compiler;
# 41 "cyclone.cyc"
void Cyc_Port_port(struct Cyc_List_List*);
# 44
extern int Cyc_ParseErrors_print_state_and_token;
# 47
extern int Cyc_Lex_compile_for_boot_r;
void Cyc_Lex_pos_init();
void Cyc_Lex_lex_init(int use_cyclone_keywords);
# 58
static int Cyc_pp_r=0;
int Cyc_noexpand_r=0;
static int Cyc_noshake_r=0;
static int Cyc_stop_after_cpp_r=0;
static int Cyc_parseonly_r=0;
static int Cyc_tc_r=0;
static int Cyc_cf_r=0;
static int Cyc_noprint_r=0;
static int Cyc_ic_r=0;
static int Cyc_toc_r=0;
static int Cyc_stop_after_objectfile_r=0;
static int Cyc_stop_after_asmfile_r=0;
static int Cyc_inline_functions_r=0;
static int Cyc_elim_se_r=0;
static int Cyc_v_r=0;
static int Cyc_save_temps_r=0;
static int Cyc_save_c_r=0;
static int Cyc_nogc_r=0;
static int Cyc_pa_r=0;
static int Cyc_pg_r=0;
static int Cyc_nocheck_r=0;
static int Cyc_add_cyc_namespace_r=1;
static int Cyc_generate_line_directives_r=1;
static int Cyc_print_full_evars_r=0;
static int Cyc_print_all_tvars_r=0;
static int Cyc_print_all_kinds_r=0;
static int Cyc_print_all_effects_r=0;
static int Cyc_nocyc_setjmp_r=0;
static int Cyc_generate_interface_r=0;
static int Cyc_toseqc_r=1;
static int Cyc_pthread_r=0;
static int Cyc_debug_r=0;
# 91
struct _fat_ptr Cyc_last_filename={(void*)0,(void*)0,(void*)(0 + 0)};
# 93
static struct Cyc_List_List*Cyc_ccargs=0;
static void Cyc_add_ccarg(struct _fat_ptr s){
Cyc_ccargs=({struct Cyc_List_List*_tmp1=_cycalloc(sizeof(*_tmp1));({struct _fat_ptr*_tmp31F=({struct _fat_ptr*_tmp0=_cycalloc(sizeof(*_tmp0));*_tmp0=s;_tmp0;});_tmp1->hd=_tmp31F;}),_tmp1->tl=Cyc_ccargs;_tmp1;});}
# 101
void Cyc_set_c_compiler(struct _fat_ptr s){
if(({({struct _fat_ptr _tmp320=(struct _fat_ptr)s;Cyc_strcmp(_tmp320,({const char*_tmp3="vc";_tag_fat(_tmp3,sizeof(char),3U);}));});})== 0){
Cyc_Cyclone_c_compiler=Cyc_Cyclone_Vc_c;
({Cyc_add_ccarg(({const char*_tmp4="-DVC_C";_tag_fat(_tmp4,sizeof(char),7U);}));});}else{
# 106
if(({({struct _fat_ptr _tmp321=(struct _fat_ptr)s;Cyc_strcmp(_tmp321,({const char*_tmp5="gcc";_tag_fat(_tmp5,sizeof(char),4U);}));});})== 0){
Cyc_Cyclone_c_compiler=Cyc_Cyclone_Gcc_c;
({Cyc_add_ccarg(({const char*_tmp6="-DGCC_C";_tag_fat(_tmp6,sizeof(char),8U);}));});}}}
# 113
void Cyc_set_port_c_code(){
Cyc_Absyn_porting_c_code=1;
Cyc_Position_max_errors=65535;
Cyc_save_c_r=1;
Cyc_parseonly_r=1;}
# 113
static struct _fat_ptr*Cyc_output_file=0;
# 121
static void Cyc_set_output_file(struct _fat_ptr s){
Cyc_output_file=({struct _fat_ptr*_tmp9=_cycalloc(sizeof(*_tmp9));*_tmp9=s;_tmp9;});}
# 121
static struct _fat_ptr Cyc_specified_interface={(void*)0,(void*)0,(void*)(0 + 0)};
# 126
static void Cyc_set_specified_interface(struct _fat_ptr s){
Cyc_specified_interface=s;}
# 135
extern char*Cdef_lib_path;
extern char*Carch;
extern char*Cversion;static char _tmpC[1U]="";
# 140
static struct _fat_ptr Cyc_cpp={_tmpC,_tmpC,_tmpC + 1U};
static void Cyc_set_cpp(struct _fat_ptr s){
Cyc_cpp=s;}
# 141
static struct Cyc_List_List*Cyc_cppargs=0;
# 146
static void Cyc_add_cpparg(struct _fat_ptr s){
Cyc_cppargs=({struct Cyc_List_List*_tmpF=_cycalloc(sizeof(*_tmpF));({struct _fat_ptr*_tmp322=({struct _fat_ptr*_tmpE=_cycalloc(sizeof(*_tmpE));*_tmpE=s;_tmpE;});_tmpF->hd=_tmp322;}),_tmpF->tl=Cyc_cppargs;_tmpF;});}
# 150
static void Cyc_add_cpp_and_ccarg(struct _fat_ptr s){
({Cyc_add_cpparg(s);});
({Cyc_add_ccarg(s);});}
# 155
static void Cyc_add_iprefix(struct _fat_ptr s){
({void(*_tmp12)(struct _fat_ptr s)=Cyc_add_cpparg;struct _fat_ptr _tmp13=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp16=({struct Cyc_String_pa_PrintArg_struct _tmp2EB;_tmp2EB.tag=0U,_tmp2EB.f1=(struct _fat_ptr)((struct _fat_ptr)s);_tmp2EB;});void*_tmp14[1U];_tmp14[0]=& _tmp16;({struct _fat_ptr _tmp323=({const char*_tmp15="-iprefix %s";_tag_fat(_tmp15,sizeof(char),12U);});Cyc_aprintf(_tmp323,_tag_fat(_tmp14,sizeof(void*),1U));});});_tmp12(_tmp13);});}
# 158
static void Cyc_add_iwithprefix(struct _fat_ptr s){
({void(*_tmp18)(struct _fat_ptr s)=Cyc_add_cpparg;struct _fat_ptr _tmp19=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp1C=({struct Cyc_String_pa_PrintArg_struct _tmp2EC;_tmp2EC.tag=0U,_tmp2EC.f1=(struct _fat_ptr)((struct _fat_ptr)s);_tmp2EC;});void*_tmp1A[1U];_tmp1A[0]=& _tmp1C;({struct _fat_ptr _tmp324=({const char*_tmp1B="-iwithprefix %s";_tag_fat(_tmp1B,sizeof(char),16U);});Cyc_aprintf(_tmp324,_tag_fat(_tmp1A,sizeof(void*),1U));});});_tmp18(_tmp19);});}
# 161
static void Cyc_add_iwithprefixbefore(struct _fat_ptr s){
({void(*_tmp1E)(struct _fat_ptr s)=Cyc_add_cpparg;struct _fat_ptr _tmp1F=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp22=({struct Cyc_String_pa_PrintArg_struct _tmp2ED;_tmp2ED.tag=0U,_tmp2ED.f1=(struct _fat_ptr)((struct _fat_ptr)s);_tmp2ED;});void*_tmp20[1U];_tmp20[0]=& _tmp22;({struct _fat_ptr _tmp325=({const char*_tmp21="-iwithprefixbefore %s";_tag_fat(_tmp21,sizeof(char),22U);});Cyc_aprintf(_tmp325,_tag_fat(_tmp20,sizeof(void*),1U));});});_tmp1E(_tmp1F);});}
# 164
static void Cyc_add_isystem(struct _fat_ptr s){
({void(*_tmp24)(struct _fat_ptr s)=Cyc_add_cpparg;struct _fat_ptr _tmp25=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp28=({struct Cyc_String_pa_PrintArg_struct _tmp2EE;_tmp2EE.tag=0U,_tmp2EE.f1=(struct _fat_ptr)((struct _fat_ptr)s);_tmp2EE;});void*_tmp26[1U];_tmp26[0]=& _tmp28;({struct _fat_ptr _tmp326=({const char*_tmp27="-isystem %s";_tag_fat(_tmp27,sizeof(char),12U);});Cyc_aprintf(_tmp326,_tag_fat(_tmp26,sizeof(void*),1U));});});_tmp24(_tmp25);});}
# 167
static void Cyc_add_idirafter(struct _fat_ptr s){
({void(*_tmp2A)(struct _fat_ptr s)=Cyc_add_cpparg;struct _fat_ptr _tmp2B=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp2E=({struct Cyc_String_pa_PrintArg_struct _tmp2EF;_tmp2EF.tag=0U,_tmp2EF.f1=(struct _fat_ptr)((struct _fat_ptr)s);_tmp2EF;});void*_tmp2C[1U];_tmp2C[0]=& _tmp2E;({struct _fat_ptr _tmp327=({const char*_tmp2D="-idirafter %s";_tag_fat(_tmp2D,sizeof(char),14U);});Cyc_aprintf(_tmp327,_tag_fat(_tmp2C,sizeof(void*),1U));});});_tmp2A(_tmp2B);});}
# 171
static void Cyc_set_many_errors(){
Cyc_Position_max_errors=65535;}static char _tmp31[20U]="_see_cycspecs_file_";
# 171
static struct _fat_ptr Cyc_def_inc_path={_tmp31,_tmp31,_tmp31 + 20U};
# 178
static void Cyc_print_version(){
({struct Cyc_String_pa_PrintArg_struct _tmp34=({struct Cyc_String_pa_PrintArg_struct _tmp2F0;_tmp2F0.tag=0U,({struct _fat_ptr _tmp328=(struct _fat_ptr)({char*_tmp35=Cversion;_tag_fat(_tmp35,sizeof(char),_get_zero_arr_size_char((void*)_tmp35,1U));});_tmp2F0.f1=_tmp328;});_tmp2F0;});void*_tmp32[1U];_tmp32[0]=& _tmp34;({struct _fat_ptr _tmp329=({const char*_tmp33="The Cyclone compiler, version %s\n";_tag_fat(_tmp33,sizeof(char),34U);});Cyc_printf(_tmp329,_tag_fat(_tmp32,sizeof(void*),1U));});});
({struct Cyc_String_pa_PrintArg_struct _tmp38=({struct Cyc_String_pa_PrintArg_struct _tmp2F1;_tmp2F1.tag=0U,({struct _fat_ptr _tmp32A=(struct _fat_ptr)({char*_tmp39=Carch;_tag_fat(_tmp39,sizeof(char),_get_zero_arr_size_char((void*)_tmp39,1U));});_tmp2F1.f1=_tmp32A;});_tmp2F1;});void*_tmp36[1U];_tmp36[0]=& _tmp38;({struct _fat_ptr _tmp32B=({const char*_tmp37="Compiled for architecture: %s\n";_tag_fat(_tmp37,sizeof(char),31U);});Cyc_printf(_tmp32B,_tag_fat(_tmp36,sizeof(void*),1U));});});
({struct Cyc_String_pa_PrintArg_struct _tmp3C=({struct Cyc_String_pa_PrintArg_struct _tmp2F2;_tmp2F2.tag=0U,({struct _fat_ptr _tmp32C=(struct _fat_ptr)({char*_tmp3D=Cdef_lib_path;_tag_fat(_tmp3D,sizeof(char),_get_zero_arr_size_char((void*)_tmp3D,1U));});_tmp2F2.f1=_tmp32C;});_tmp2F2;});void*_tmp3A[1U];_tmp3A[0]=& _tmp3C;({struct _fat_ptr _tmp32D=({const char*_tmp3B="Standard library directory: %s\n";_tag_fat(_tmp3B,sizeof(char),32U);});Cyc_printf(_tmp32D,_tag_fat(_tmp3A,sizeof(void*),1U));});});
# 183
({struct Cyc_String_pa_PrintArg_struct _tmp40=({struct Cyc_String_pa_PrintArg_struct _tmp2F3;_tmp2F3.tag=0U,_tmp2F3.f1=(struct _fat_ptr)((struct _fat_ptr)Cyc_def_inc_path);_tmp2F3;});void*_tmp3E[1U];_tmp3E[0]=& _tmp40;({struct _fat_ptr _tmp32E=({const char*_tmp3F="Standard include directory: %s\n";_tag_fat(_tmp3F,sizeof(char),32U);});Cyc_printf(_tmp32E,_tag_fat(_tmp3E,sizeof(void*),1U));});});
({exit(0);});}
# 187
static int Cyc_is_cyclone_sourcefile(struct _fat_ptr s){
unsigned long len=({Cyc_strlen((struct _fat_ptr)s);});
if(len <= (unsigned long)4)return 0;else{
return({({struct _fat_ptr _tmp32F=(struct _fat_ptr)_fat_ptr_plus(s,sizeof(char),(int)(len - (unsigned long)4));Cyc_strcmp(_tmp32F,({const char*_tmp42=".cyc";_tag_fat(_tmp42,sizeof(char),5U);}));});})== 0;}}
# 187
static struct Cyc_List_List*Cyc_libargs=0;
# 204 "cyclone.cyc"
static void Cyc_add_libarg(struct _fat_ptr s){
if(({({struct _fat_ptr _tmp330=(struct _fat_ptr)s;Cyc_strcmp(_tmp330,({const char*_tmp44="-lxml";_tag_fat(_tmp44,sizeof(char),6U);}));});})== 0){
if(!Cyc_pa_r)
({Cyc_add_ccarg(s);});else{
# 209
({Cyc_add_ccarg(({const char*_tmp45="-lxml_a";_tag_fat(_tmp45,sizeof(char),8U);}));});}}else{
if(({({struct _fat_ptr _tmp331=(struct _fat_ptr)s;Cyc_strncmp(_tmp331,({const char*_tmp46="-lcyc";_tag_fat(_tmp46,sizeof(char),6U);}),5U);});})== 0)
({Cyc_add_ccarg(s);});else{
# 213
if(({({struct _fat_ptr _tmp332=(struct _fat_ptr)s;Cyc_strncmp(_tmp332,({const char*_tmp47="-lpthread";_tag_fat(_tmp47,sizeof(char),10U);}),9U);});})== 0){
Cyc_pthread_r=1;
({Cyc_Absyn_set_pthread(1);});}
# 213
Cyc_libargs=({struct Cyc_List_List*_tmp49=_cycalloc(sizeof(*_tmp49));
# 217
({struct _fat_ptr*_tmp333=({struct _fat_ptr*_tmp48=_cycalloc(sizeof(*_tmp48));*_tmp48=s;_tmp48;});_tmp49->hd=_tmp333;}),_tmp49->tl=Cyc_libargs;_tmp49;});}}}
# 221
static void Cyc_add_marg(struct _fat_ptr s){
({Cyc_add_ccarg(s);});}
# 225
static void Cyc_set_save_temps(){
Cyc_save_temps_r=1;
({Cyc_add_ccarg(({const char*_tmp4C="-save-temps";_tag_fat(_tmp4C,sizeof(char),12U);}));});}
# 225
static int Cyc_produce_dependencies=0;
# 231
static void Cyc_set_produce_dependencies(){
Cyc_stop_after_cpp_r=1;
Cyc_produce_dependencies=1;
({Cyc_add_cpparg(({const char*_tmp4E="-M";_tag_fat(_tmp4E,sizeof(char),3U);}));});}
# 231
static struct _fat_ptr*Cyc_dependencies_target=0;
# 238
static void Cyc_set_dependencies_target(struct _fat_ptr s){
Cyc_dependencies_target=({struct _fat_ptr*_tmp50=_cycalloc(sizeof(*_tmp50));*_tmp50=s;_tmp50;});}
# 242
static void Cyc_set_stop_after_objectfile(){
Cyc_stop_after_objectfile_r=1;
({Cyc_add_ccarg(({const char*_tmp52="-c";_tag_fat(_tmp52,sizeof(char),3U);}));});}
# 247
static void Cyc_set_nocppprecomp(){
({Cyc_add_cpp_and_ccarg(({const char*_tmp54="-no-cpp-precomp";_tag_fat(_tmp54,sizeof(char),16U);}));});}
# 251
static void Cyc_clear_lineno(){
Cyc_generate_line_directives_r=0;
({Cyc_set_save_temps();});}
# 255
static void Cyc_set_inline_functions(){
Cyc_inline_functions_r=1;}
# 258
static void Cyc_set_elim_se(){
Cyc_elim_se_r=1;
({Cyc_set_inline_functions();});}
# 262
static void Cyc_set_tovc(){
Cyc_Cyclone_tovc_r=1;
({Cyc_add_ccarg(({const char*_tmp59="-DCYC_ANSI_OUTPUT";_tag_fat(_tmp59,sizeof(char),18U);}));});
({Cyc_set_elim_se();});}
# 267
static void Cyc_set_notoseqc(){
Cyc_toseqc_r=0;}
# 270
static void Cyc_set_noboundschecks(){
({Cyc_add_ccarg(({const char*_tmp5C="-DNO_CYC_BOUNDS_CHECKS";_tag_fat(_tmp5C,sizeof(char),23U);}));});}
# 273
static void Cyc_set_nonullchecks(){
({Cyc_add_ccarg(({const char*_tmp5E="-DNO_CYC_NULL_CHECKS";_tag_fat(_tmp5E,sizeof(char),21U);}));});}
# 276
static void Cyc_set_nochecks(){
({Cyc_set_noboundschecks();});
({Cyc_set_nonullchecks();});
Cyc_nocheck_r=1;}
# 282
static void Cyc_set_nocyc(){
Cyc_add_cyc_namespace_r=0;
({Cyc_add_ccarg(({const char*_tmp61="-DNO_CYC_PREFIX";_tag_fat(_tmp61,sizeof(char),16U);}));});}
# 287
static void Cyc_set_pa(){
Cyc_pa_r=1;
({Cyc_add_ccarg(({const char*_tmp63="-DCYC_REGION_PROFILE";_tag_fat(_tmp63,sizeof(char),21U);}));});
({Cyc_add_cpparg(({const char*_tmp64="-DCYC_REGION_PROFILE";_tag_fat(_tmp64,sizeof(char),21U);}));});}
# 293
static void Cyc_set_pg(){
Cyc_pg_r=1;
({Cyc_add_ccarg(({const char*_tmp66="-pg";_tag_fat(_tmp66,sizeof(char),4U);}));});}
# 298
static void Cyc_set_stop_after_asmfile(){
Cyc_stop_after_asmfile_r=1;
({Cyc_add_ccarg(({const char*_tmp68="-S";_tag_fat(_tmp68,sizeof(char),3U);}));});}
# 303
static void Cyc_set_all_warnings(){
Cyc_InsertChecks_warn_bounds_checks=1;
Cyc_InsertChecks_warn_all_null_deref=1;
Cyc_NewControlFlow_warn_lose_unique=1;
Cyc_Tcutil_warn_alias_coerce=1;
Cyc_Tcutil_warn_region_coerce=1;
Cyc_Tc_aggressive_warn=1;
Cyc_Binding_warn_override=1;}
# 303
enum Cyc_inputtype{Cyc_DEFAULTINPUT =0U,Cyc_CYCLONEFILE =1U};
# 319
static enum Cyc_inputtype Cyc_intype=Cyc_DEFAULTINPUT;
static void Cyc_set_inputtype(struct _fat_ptr s){
if(({({struct _fat_ptr _tmp334=(struct _fat_ptr)s;Cyc_strcmp(_tmp334,({const char*_tmp6B="cyc";_tag_fat(_tmp6B,sizeof(char),4U);}));});})== 0)
Cyc_intype=Cyc_CYCLONEFILE;else{
if(({({struct _fat_ptr _tmp335=(struct _fat_ptr)s;Cyc_strcmp(_tmp335,({const char*_tmp6C="none";_tag_fat(_tmp6C,sizeof(char),5U);}));});})== 0)
Cyc_intype=Cyc_DEFAULTINPUT;else{
# 326
({struct Cyc_String_pa_PrintArg_struct _tmp6F=({struct Cyc_String_pa_PrintArg_struct _tmp2F4;_tmp2F4.tag=0U,_tmp2F4.f1=(struct _fat_ptr)((struct _fat_ptr)s);_tmp2F4;});void*_tmp6D[1U];_tmp6D[0]=& _tmp6F;({struct Cyc___cycFILE*_tmp337=Cyc_stderr;struct _fat_ptr _tmp336=({const char*_tmp6E="Input type '%s' not supported\n";_tag_fat(_tmp6E,sizeof(char),31U);});Cyc_fprintf(_tmp337,_tmp336,_tag_fat(_tmp6D,sizeof(void*),1U));});});}}}
# 320
struct _fat_ptr Cyc_make_base_filename(struct _fat_ptr s,struct _fat_ptr*output_file){
# 330
struct _fat_ptr outdir=({Cyc_Filename_dirname(output_file == 0?s:*output_file);});
struct _fat_ptr base=({struct _fat_ptr(*_tmp71)(struct _fat_ptr)=Cyc_Filename_chop_extension;struct _fat_ptr _tmp72=(struct _fat_ptr)({Cyc_Filename_basename(s);});_tmp71(_tmp72);});
# 335
struct _fat_ptr basename=({Cyc_strlen((struct _fat_ptr)outdir);})> (unsigned long)0?({Cyc_Filename_concat((struct _fat_ptr)outdir,(struct _fat_ptr)base);}): base;
return(struct _fat_ptr)basename;}
# 320
static struct Cyc_List_List*Cyc_cyclone_files=0;
# 343
static int Cyc_gcc_optarg=0;
static void Cyc_add_other(struct _fat_ptr s){
# 346
if(Cyc_gcc_optarg > 0){
({Cyc_add_ccarg(s);});
-- Cyc_gcc_optarg;}else{
# 350
if((int)Cyc_intype == (int)Cyc_CYCLONEFILE ||({Cyc_is_cyclone_sourcefile(s);})){
# 352
Cyc_cyclone_files=({struct Cyc_List_List*_tmp75=_cycalloc(sizeof(*_tmp75));({struct _fat_ptr*_tmp338=({struct _fat_ptr*_tmp74=_cycalloc(sizeof(*_tmp74));*_tmp74=s;_tmp74;});_tmp75->hd=_tmp338;}),_tmp75->tl=Cyc_cyclone_files;_tmp75;});{
# 358
struct _fat_ptr basename=({Cyc_make_base_filename(s,0);});
struct _fat_ptr cfile=({({struct _fat_ptr _tmp339=(struct _fat_ptr)basename;Cyc_strconcat(_tmp339,({const char*_tmp76=".c";_tag_fat(_tmp76,sizeof(char),3U);}));});});
({Cyc_add_ccarg((struct _fat_ptr)cfile);});}}else{
# 362
({Cyc_add_ccarg(s);});}}}
# 344
static int Cyc_assume_gcc_flag=1;struct _tuple26{struct _fat_ptr flag;int optargs;};static char _tmp78[3U]="-s";static char _tmp79[3U]="-O";static char _tmp7A[4U]="-O0";static char _tmp7B[4U]="-O2";static char _tmp7C[4U]="-O3";static char _tmp7D[21U]="-fomit-frame-pointer";static char _tmp7E[13U]="-fno-builtin";static char _tmp7F[3U]="-g";static char _tmp80[3U]="-p";static char _tmp81[8U]="-static";
# 368
static int Cyc_add_other_flag(struct _fat_ptr s){
static struct _tuple26 known_gcc_flags[10U]={{.flag={_tmp78,_tmp78,_tmp78 + 3U},.optargs=0},{.flag={_tmp79,_tmp79,_tmp79 + 3U},.optargs=0},{.flag={_tmp7A,_tmp7A,_tmp7A + 4U},.optargs=0},{.flag={_tmp7B,_tmp7B,_tmp7B + 4U},.optargs=0},{.flag={_tmp7C,_tmp7C,_tmp7C + 4U},.optargs=0},{.flag={_tmp7D,_tmp7D,_tmp7D + 21U},.optargs=0},{.flag={_tmp7E,_tmp7E,_tmp7E + 13U},.optargs=0},{.flag={_tmp7F,_tmp7F,_tmp7F + 3U},.optargs=0},{.flag={_tmp80,_tmp80,_tmp80 + 3U},.optargs=0},{.flag={_tmp81,_tmp81,_tmp81 + 8U},.optargs=0}};
# 381
if(Cyc_assume_gcc_flag)
({Cyc_add_ccarg(s);});else{
# 384
{int i=0;for(0;(unsigned)i < 10U;++ i){
if(!({Cyc_strcmp((struct _fat_ptr)(*((struct _tuple26*)_check_known_subscript_notnull(known_gcc_flags,10U,sizeof(struct _tuple26),i))).flag,(struct _fat_ptr)s);})){
({Cyc_add_ccarg(s);});
Cyc_gcc_optarg=(known_gcc_flags[i]).optargs;
break;}}}
# 391
return 0;}
# 393
return 1;}
# 396
static void Cyc_noassume_gcc_flag(){
Cyc_assume_gcc_flag=0;}
# 400
static void Cyc_remove_file(struct _fat_ptr s){
if(Cyc_save_temps_r)return;else{
({remove((const char*)_check_null(_untag_fat_ptr(s,sizeof(char),1U)));});}}
# 400
int Cyc_compile_failure=0;
# 409
struct Cyc___cycFILE*Cyc_try_file_open(struct _fat_ptr filename,struct _fat_ptr mode,struct _fat_ptr msg_part){
struct _handler_cons _tmp85;_push_handler(& _tmp85);{int _tmp87=0;if(setjmp(_tmp85.handler))_tmp87=1;if(!_tmp87){{struct Cyc___cycFILE*_tmp88=({Cyc_file_open(filename,mode);});_npop_handler(0U);return _tmp88;};_pop_handler();}else{void*_tmp86=(void*)Cyc_Core_get_exn_thrown();void*_tmp89=_tmp86;_LL1: _LL2:
# 413
 Cyc_compile_failure=1;
({struct Cyc_String_pa_PrintArg_struct _tmp8C=({struct Cyc_String_pa_PrintArg_struct _tmp2F6;_tmp2F6.tag=0U,_tmp2F6.f1=(struct _fat_ptr)((struct _fat_ptr)msg_part);_tmp2F6;});struct Cyc_String_pa_PrintArg_struct _tmp8D=({struct Cyc_String_pa_PrintArg_struct _tmp2F5;_tmp2F5.tag=0U,_tmp2F5.f1=(struct _fat_ptr)((struct _fat_ptr)filename);_tmp2F5;});void*_tmp8A[2U];_tmp8A[0]=& _tmp8C,_tmp8A[1]=& _tmp8D;({struct Cyc___cycFILE*_tmp33B=Cyc_stderr;struct _fat_ptr _tmp33A=({const char*_tmp8B="\nError: couldn't open %s %s\n";_tag_fat(_tmp8B,sizeof(char),29U);});Cyc_fprintf(_tmp33B,_tmp33A,_tag_fat(_tmp8A,sizeof(void*),2U));});});
({Cyc_fflush(Cyc_stderr);});
return 0;_LL0:;}}}
# 409
void CYCALLOCPROFILE_mark(const char*s);
# 422
void*Cyc_do_stage(struct _fat_ptr stage_name,struct Cyc_List_List*tds,void*(*f)(void*,struct Cyc_List_List*),void*env,void(*on_fail)(void*),void*failenv){
# 425
({CYCALLOCPROFILE_mark((const char*)_untag_fat_ptr(stage_name,sizeof(char),1U));});{
# 427
void*ans;
{struct _handler_cons _tmp8F;_push_handler(& _tmp8F);{int _tmp91=0;if(setjmp(_tmp8F.handler))_tmp91=1;if(!_tmp91){ans=({f(env,tds);});;_pop_handler();}else{void*_tmp90=(void*)Cyc_Core_get_exn_thrown();void*_tmp92=_tmp90;void*_tmp93;_LL1: _tmp93=_tmp92;_LL2: {void*x=_tmp93;
# 432
if(({Cyc_Position_error_p();})){
Cyc_compile_failure=1;
({on_fail(failenv);});
({void*_tmp94=0U;({struct Cyc___cycFILE*_tmp33D=Cyc_stderr;struct _fat_ptr _tmp33C=({const char*_tmp95="\nCOMPILATION FAILED!\n";_tag_fat(_tmp95,sizeof(char),22U);});Cyc_fprintf(_tmp33D,_tmp33C,_tag_fat(_tmp94,sizeof(void*),0U));});});
({Cyc_fflush(Cyc_stderr);});}else{
# 439
({struct Cyc_String_pa_PrintArg_struct _tmp98=({struct Cyc_String_pa_PrintArg_struct _tmp2F7;_tmp2F7.tag=0U,_tmp2F7.f1=(struct _fat_ptr)((struct _fat_ptr)stage_name);_tmp2F7;});void*_tmp96[1U];_tmp96[0]=& _tmp98;({struct Cyc___cycFILE*_tmp33F=Cyc_stderr;struct _fat_ptr _tmp33E=({const char*_tmp97="COMPILER STAGE %s\n";_tag_fat(_tmp97,sizeof(char),19U);});Cyc_fprintf(_tmp33F,_tmp33E,_tag_fat(_tmp96,sizeof(void*),1U));});});
({on_fail(failenv);});}
# 442
({Cyc_Core_rethrow(x);});
goto _LL0;}_LL0:;}}}
# 445
if(({Cyc_Position_error_p();}))
Cyc_compile_failure=1;
# 445
if(Cyc_compile_failure){
# 448
({on_fail(failenv);});
({void*_tmp99=0U;({struct Cyc___cycFILE*_tmp341=Cyc_stderr;struct _fat_ptr _tmp340=({const char*_tmp9A="\nCOMPILATION FAILED!\n";_tag_fat(_tmp9A,sizeof(char),22U);});Cyc_fprintf(_tmp341,_tmp340,_tag_fat(_tmp99,sizeof(void*),0U));});});
({Cyc_fflush(Cyc_stderr);});
return ans;}else{
# 453
if(Cyc_v_r){
({struct Cyc_String_pa_PrintArg_struct _tmp9D=({struct Cyc_String_pa_PrintArg_struct _tmp2F8;_tmp2F8.tag=0U,_tmp2F8.f1=(struct _fat_ptr)((struct _fat_ptr)stage_name);_tmp2F8;});void*_tmp9B[1U];_tmp9B[0]=& _tmp9D;({struct Cyc___cycFILE*_tmp343=Cyc_stderr;struct _fat_ptr _tmp342=({const char*_tmp9C="%s completed.\n";_tag_fat(_tmp9C,sizeof(char),15U);});Cyc_fprintf(_tmp343,_tmp342,_tag_fat(_tmp9B,sizeof(void*),1U));});});
({Cyc_fflush(Cyc_stderr);});
return ans;}}
# 445
return ans;}}
# 461
static void Cyc_ignore(void*x){;}
static void Cyc_remove_fileptr(struct _fat_ptr*s){({Cyc_remove_file((struct _fat_ptr)*s);});}
# 464
struct Cyc_List_List*Cyc_do_parse(struct Cyc___cycFILE*f,struct Cyc_List_List*ignore){
({Cyc_Lex_lex_init(1);});
({Cyc_Lex_pos_init();});{
struct Cyc_List_List*ans=0;
{struct _handler_cons _tmpA1;_push_handler(& _tmpA1);{int _tmpA3=0;if(setjmp(_tmpA1.handler))_tmpA3=1;if(!_tmpA3){
ans=({Cyc_Parse_parse_file(f,Cyc_last_filename);});
({Cyc_file_close(f);});
# 469
;_pop_handler();}else{void*_tmpA2=(void*)Cyc_Core_get_exn_thrown();void*_tmpA4=_tmpA2;void*_tmpA5;if(((struct Cyc_Parse_Exit_exn_struct*)_tmpA4)->tag == Cyc_Parse_Exit){_LL1: _LL2:
# 472
({Cyc_file_close(f);});Cyc_compile_failure=1;goto _LL0;}else{_LL3: _tmpA5=_tmpA4;_LL4: {void*e=_tmpA5;
({Cyc_file_close(f);});({Cyc_Core_rethrow(e);});}}_LL0:;}}}
# 475
({Cyc_Lex_lex_init(1);});
return ans;}}
# 464
int Cyc_do_binding(int ignore,struct Cyc_List_List*tds){
# 480
({Cyc_Binding_resolve_all(tds);});
return 1;}
# 484
struct Cyc_List_List*Cyc_do_typecheck(struct Cyc_Tcenv_Tenv*te,struct Cyc_List_List*tds){
# 486
({Cyc_Tc_tc(te,1,tds,Cyc_debug_r);});
if(!Cyc_noshake_r)
tds=({Cyc_Tc_treeshake(te,tds);});
# 487
return tds;}
# 484
struct Cyc_JumpAnalysis_Jump_Anal_Result*Cyc_do_jumpanalysis(int ignore,struct Cyc_List_List*tds){
# 494
return({Cyc_JumpAnalysis_jump_analysis(tds);});}
# 497
struct Cyc_List_List*Cyc_do_cfcheck(struct Cyc_JumpAnalysis_Jump_Anal_Result*tables,struct Cyc_List_List*tds){
# 499
({Cyc_NewControlFlow_cf_check(tables,tds);});
return tds;}
# 497
int Cyc_do_insert_checks(struct Cyc_JumpAnalysis_Jump_Anal_Result*tables,struct Cyc_List_List*tds){
# 505
({Cyc_InsertChecks_insert_checks(tds,tables);});
return 1;}
# 497
int Cyc_do_ioeffect_analysis(struct Cyc_JumpAnalysis_Jump_Anal_Result*tables,struct Cyc_List_List*tds){
# 510
({Cyc_IOEffect_analysis(tables,tds);});
return 1;}struct _tuple27{struct Cyc_Tcenv_Tenv*f1;struct Cyc___cycFILE*f2;struct Cyc___cycFILE*f3;};
# 517
struct Cyc_List_List*Cyc_do_interface(struct _tuple27*params,struct Cyc_List_List*tds){
# 519
struct Cyc___cycFILE*_tmpAF;struct Cyc___cycFILE*_tmpAE;struct Cyc_Tcenv_Tenv*_tmpAD;_LL1: _tmpAD=params->f1;_tmpAE=params->f2;_tmpAF=params->f3;_LL2: {struct Cyc_Tcenv_Tenv*te=_tmpAD;struct Cyc___cycFILE*fi=_tmpAE;struct Cyc___cycFILE*fo=_tmpAF;
struct Cyc_Interface_I*i1=({Cyc_Interface_extract(te->ae,tds);});
if(fi == 0)
({Cyc_Interface_save(i1,fo);});else{
# 524
struct Cyc_Interface_I*i0=({Cyc_Interface_parse(fi);});
if(!({({struct Cyc_Interface_I*_tmp347=i0;struct Cyc_Interface_I*_tmp346=i1;Cyc_Interface_is_subinterface(_tmp347,_tmp346,({struct _tuple25*_tmpB2=_cycalloc(sizeof(*_tmpB2));({struct _fat_ptr _tmp345=({const char*_tmpB0="written interface";_tag_fat(_tmpB0,sizeof(char),18U);});_tmpB2->f1=_tmp345;}),({struct _fat_ptr _tmp344=({const char*_tmpB1="maximal interface";_tag_fat(_tmpB1,sizeof(char),18U);});_tmpB2->f2=_tmp344;});_tmpB2;}));});}))
Cyc_compile_failure=1;else{
# 528
({Cyc_Interface_save(i0,fo);});}}
# 530
return tds;}}
# 532
void Cyc_interface_fail(struct _tuple27*params){
struct Cyc___cycFILE*_tmpB5;struct Cyc___cycFILE*_tmpB4;_LL1: _tmpB4=params->f2;_tmpB5=params->f3;_LL2: {struct Cyc___cycFILE*fi=_tmpB4;struct Cyc___cycFILE*fo=_tmpB5;
if(fi != 0)
({Cyc_file_close(fi);});
# 534
({Cyc_file_close(fo);});}}
# 540
struct Cyc_List_List*Cyc_do_translate(struct Cyc_Hashtable_Table*pops,struct Cyc_List_List*tds){
# 542
return({Cyc_Toc_toc(pops,tds);});}
# 544
struct Cyc_List_List*Cyc_do_removeaggrs(int ignore,struct Cyc_List_List*tds){
return({Cyc_RemoveAggrs_remove_aggrs(tds);});}
# 548
struct Cyc_List_List*Cyc_do_toseqc(int ignore,struct Cyc_List_List*tds){
return({Cyc_Toseqc_toseqc(tds);});}
# 551
struct Cyc_List_List*Cyc_do_tovc(int ignore,struct Cyc_List_List*tds){
Cyc_Tovc_elim_array_initializers=Cyc_Cyclone_tovc_r;
return({Cyc_Tovc_tovc(tds);});}
# 551
static struct _fat_ptr Cyc_cyc_setjmp={(void*)0,(void*)0,(void*)(0 + 0)};
# 558
static struct _fat_ptr Cyc_cyc_include={(void*)0,(void*)0,(void*)(0 + 0)};
# 560
static void Cyc_set_cyc_include(struct _fat_ptr f){
Cyc_cyc_include=f;}
# 565
int Cyc_copy_internal_file(struct _fat_ptr file,struct Cyc___cycFILE*out_file){
# 568
if(({char*_tmp348=(char*)file.curr;_tmp348 == (char*)(_tag_fat(0,0,0)).curr;})){
({void*_tmpBC=0U;({struct Cyc___cycFILE*_tmp34A=Cyc_stderr;struct _fat_ptr _tmp349=({const char*_tmpBD="Internal error: copy_internal_file called with NULL\n";_tag_fat(_tmpBD,sizeof(char),53U);});Cyc_fprintf(_tmp34A,_tmp349,_tag_fat(_tmpBC,sizeof(void*),0U));});});
return 1;}{
# 568
struct Cyc___cycFILE*file_f=({({struct _fat_ptr _tmp34C=file;struct _fat_ptr _tmp34B=({const char*_tmpC4="r";_tag_fat(_tmpC4,sizeof(char),2U);});Cyc_try_file_open(_tmp34C,_tmp34B,({const char*_tmpC5="internal compiler file";_tag_fat(_tmpC5,sizeof(char),23U);}));});});
# 573
if(file_f == 0)return 1;{unsigned long n_read=1024U;
# 575
unsigned long n_written;
char buf[1024U];({{unsigned _tmp2FB=1024U;unsigned i;for(i=0;i < _tmp2FB;++ i){buf[i]='\000';}}0;});
while(n_read == (unsigned long)1024){
n_read=({({struct _fat_ptr _tmp34D=_tag_fat(buf,sizeof(char),1024U);Cyc_fread(_tmp34D,1U,1024U,file_f);});});
if(n_read != (unsigned long)1024 && !({Cyc_feof(file_f);})){
Cyc_compile_failure=1;
({struct Cyc_String_pa_PrintArg_struct _tmpC0=({struct Cyc_String_pa_PrintArg_struct _tmp2F9;_tmp2F9.tag=0U,_tmp2F9.f1=(struct _fat_ptr)((struct _fat_ptr)file);_tmp2F9;});void*_tmpBE[1U];_tmpBE[0]=& _tmpC0;({struct Cyc___cycFILE*_tmp34F=Cyc_stderr;struct _fat_ptr _tmp34E=({const char*_tmpBF="\nError: problem copying %s\n";_tag_fat(_tmpBF,sizeof(char),28U);});Cyc_fprintf(_tmp34F,_tmp34E,_tag_fat(_tmpBE,sizeof(void*),1U));});});
({Cyc_fflush(Cyc_stderr);});
return 1;}
# 579
n_written=({({struct _fat_ptr _tmp351=
# 585
_tag_fat(buf,sizeof(char),1024U);unsigned long _tmp350=n_read;Cyc_fwrite(_tmp351,1U,_tmp350,out_file);});});
if(n_read != n_written){
Cyc_compile_failure=1;
({struct Cyc_String_pa_PrintArg_struct _tmpC3=({struct Cyc_String_pa_PrintArg_struct _tmp2FA;_tmp2FA.tag=0U,_tmp2FA.f1=(struct _fat_ptr)((struct _fat_ptr)file);_tmp2FA;});void*_tmpC1[1U];_tmpC1[0]=& _tmpC3;({struct Cyc___cycFILE*_tmp353=Cyc_stderr;struct _fat_ptr _tmp352=({const char*_tmpC2="\nError: problem copying %s\n";_tag_fat(_tmpC2,sizeof(char),28U);});Cyc_fprintf(_tmp353,_tmp352,_tag_fat(_tmpC1,sizeof(void*),1U));});});
({Cyc_fflush(Cyc_stderr);});
return 1;}}
# 593
({Cyc_fclose(file_f);});
return 0;}}}
# 565
static struct Cyc_List_List*Cyc_cfiles=0;
# 600
static void Cyc_remove_cfiles(){
if(!Cyc_save_c_r)
for(0;Cyc_cfiles != 0;Cyc_cfiles=((struct Cyc_List_List*)_check_null(Cyc_cfiles))->tl){
({Cyc_remove_file((struct _fat_ptr)*((struct _fat_ptr*)((struct Cyc_List_List*)_check_null(Cyc_cfiles))->hd));});}}
# 605
static void Cyc_find_fail(struct _fat_ptr file){
Cyc_compile_failure=1;
({Cyc_remove_cfiles();});
(int)_throw(({struct Cyc_Core_Failure_exn_struct*_tmpCB=_cycalloc(sizeof(*_tmpCB));_tmpCB->tag=Cyc_Core_Failure,({struct _fat_ptr _tmp355=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmpCA=({struct Cyc_String_pa_PrintArg_struct _tmp2FC;_tmp2FC.tag=0U,_tmp2FC.f1=(struct _fat_ptr)((struct _fat_ptr)file);_tmp2FC;});void*_tmpC8[1U];_tmpC8[0]=& _tmpCA;({struct _fat_ptr _tmp354=({const char*_tmpC9="Error: can't find internal compiler file %s\n";_tag_fat(_tmpC9,sizeof(char),45U);});Cyc_aprintf(_tmp354,_tag_fat(_tmpC8,sizeof(void*),1U));});});_tmpCB->f1=_tmp355;});_tmpCB;}));}
# 610
static struct _fat_ptr Cyc_find_in_arch_path(struct _fat_ptr s){
struct _fat_ptr r=({Cyc_Specsfile_find_in_arch_path(s);});
if(!((unsigned)r.curr))({Cyc_find_fail(s);});return r;}
# 615
static struct _fat_ptr Cyc_find_in_exec_path(struct _fat_ptr s){
struct _fat_ptr r=({Cyc_Specsfile_find_in_exec_path(s);});
if(!((unsigned)r.curr))({Cyc_find_fail(s);});return r;}struct _tuple28{struct Cyc___cycFILE*f1;struct _fat_ptr f2;};
# 621
struct Cyc_List_List*Cyc_do_print(struct _tuple28*env,struct Cyc_List_List*tds){
# 623
struct _fat_ptr _tmpD0;struct Cyc___cycFILE*_tmpCF;_LL1: _tmpCF=env->f1;_tmpD0=env->f2;_LL2: {struct Cyc___cycFILE*out_file=_tmpCF;struct _fat_ptr cfile=_tmpD0;
struct Cyc_Absynpp_Params params_r=Cyc_tc_r?Cyc_Absynpp_cyc_params_r: Cyc_Absynpp_c_params_r;
params_r.expand_typedefs=!Cyc_noexpand_r;
params_r.to_VC=Cyc_Cyclone_tovc_r;
params_r.add_cyc_prefix=Cyc_add_cyc_namespace_r;
params_r.generate_line_directives=Cyc_generate_line_directives_r;
params_r.print_full_evars=Cyc_print_full_evars_r;
params_r.print_all_tvars=Cyc_print_all_tvars_r;
params_r.print_all_kinds=Cyc_print_all_kinds_r;
params_r.print_all_effects=Cyc_print_all_effects_r;
# 634
if(Cyc_inline_functions_r)
({void*_tmpD1=0U;({struct Cyc___cycFILE*_tmp357=out_file;struct _fat_ptr _tmp356=({const char*_tmpD2="#define _INLINE_FUNCTIONS\n";_tag_fat(_tmpD2,sizeof(char),27U);});Cyc_fprintf(_tmp357,_tmp356,_tag_fat(_tmpD1,sizeof(void*),0U));});});
# 634
if(
# 638
(!Cyc_parseonly_r && !Cyc_tc_r)&& !Cyc_cf_r){
if(!Cyc_nocyc_setjmp_r){
if(Cyc_Lex_compile_for_boot_r)
({void*_tmpD3=0U;({struct Cyc___cycFILE*_tmp359=out_file;struct _fat_ptr _tmp358=({const char*_tmpD4="#include <setjmp.h>\n";_tag_fat(_tmpD4,sizeof(char),21U);});Cyc_fprintf(_tmp359,_tmp358,_tag_fat(_tmpD3,sizeof(void*),0U));});});else{
if(({Cyc_copy_internal_file(Cyc_cyc_setjmp,out_file);}))return tds;}}
# 639
if(({Cyc_copy_internal_file(Cyc_cyc_include,out_file);}))
# 644
return tds;}
# 634
if(Cyc_pp_r){
# 647
({Cyc_Absynpp_set_params(& params_r);});
({Cyc_Absynpp_decllist2file(tds,out_file);});}else{
# 650
({Cyc_Absyndump_set_params(& params_r);});
({Cyc_Absyndump_dumpdecllist2file(tds,out_file);});}
# 653
({Cyc_fflush(out_file);});
({Cyc_file_close(out_file);});
Cyc_cfiles=({struct Cyc_List_List*_tmpD6=_cycalloc(sizeof(*_tmpD6));({struct _fat_ptr*_tmp35A=({struct _fat_ptr*_tmpD5=_cycalloc(sizeof(*_tmpD5));*_tmpD5=cfile;_tmpD5;});_tmpD6->hd=_tmp35A;}),_tmpD6->tl=Cyc_cfiles;_tmpD6;});
return tds;}}
# 658
void Cyc_print_fail(struct _tuple28*env){
struct _fat_ptr _tmpD9;struct Cyc___cycFILE*_tmpD8;_LL1: _tmpD8=env->f1;_tmpD9=env->f2;_LL2: {struct Cyc___cycFILE*out_file=_tmpD8;struct _fat_ptr cfile=_tmpD9;
({Cyc_file_close(out_file);});
Cyc_cfiles=({struct Cyc_List_List*_tmpDB=_cycalloc(sizeof(*_tmpDB));({struct _fat_ptr*_tmp35C=({struct _fat_ptr*_tmpDA=_cycalloc(sizeof(*_tmpDA));({struct _fat_ptr _tmp35B=({Cyc_strdup((struct _fat_ptr)cfile);});*_tmpDA=_tmp35B;});_tmpDA;});_tmpDB->hd=_tmp35C;}),_tmpDB->tl=Cyc_cfiles;_tmpDB;});}}
# 665
static struct Cyc_List_List*Cyc_split_by_char(struct _fat_ptr s,char c){
if(({char*_tmp35D=(char*)s.curr;_tmp35D == (char*)(_tag_fat(0,0,0)).curr;}))return 0;{struct Cyc_List_List*result=0;
# 668
unsigned long len=({Cyc_strlen((struct _fat_ptr)s);});
# 670
while(len > (unsigned long)0){
struct _fat_ptr end=({Cyc_strchr(s,c);});
if(({char*_tmp35E=(char*)end.curr;_tmp35E == (char*)(_tag_fat(0,0,0)).curr;})){
result=({struct Cyc_List_List*_tmpDE=_cycalloc(sizeof(*_tmpDE));({struct _fat_ptr*_tmp35F=({struct _fat_ptr*_tmpDD=_cycalloc(sizeof(*_tmpDD));*_tmpDD=s;_tmpDD;});_tmpDE->hd=_tmp35F;}),_tmpDE->tl=result;_tmpDE;});
break;}else{
# 677
result=({struct Cyc_List_List*_tmpE0=_cycalloc(sizeof(*_tmpE0));({struct _fat_ptr*_tmp361=({struct _fat_ptr*_tmpDF=_cycalloc(sizeof(*_tmpDF));({struct _fat_ptr _tmp360=(struct _fat_ptr)({Cyc_substring((struct _fat_ptr)s,0,(unsigned long)((((struct _fat_ptr)end).curr - s.curr)/ sizeof(char)));});*_tmpDF=_tmp360;});_tmpDF;});_tmpE0->hd=_tmp361;}),_tmpE0->tl=result;_tmpE0;});
len -=(unsigned long)((((struct _fat_ptr)end).curr - s.curr)/ sizeof(char));
s=_fat_ptr_plus(end,sizeof(char),1);}}
# 682
return({Cyc_List_imp_rev(result);});}}
# 688
static struct Cyc_List_List*Cyc_add_subdir(struct Cyc_List_List*dirs,struct _fat_ptr subdir){
struct Cyc_List_List*l=0;
for(0;dirs != 0;dirs=dirs->tl){
l=({struct Cyc_List_List*_tmpE3=_cycalloc(sizeof(*_tmpE3));({struct _fat_ptr*_tmp363=({struct _fat_ptr*_tmpE2=_cycalloc(sizeof(*_tmpE2));({struct _fat_ptr _tmp362=(struct _fat_ptr)({Cyc_Filename_concat(*((struct _fat_ptr*)dirs->hd),subdir);});*_tmpE2=_tmp362;});_tmpE2;});_tmpE3->hd=_tmp363;}),_tmpE3->tl=l;_tmpE3;});}
# 693
l=({Cyc_List_imp_rev(l);});
return l;}
# 698
static int Cyc_is_other_special(char c){
char _tmpE5=c;switch(_tmpE5){case 92U: _LL1: _LL2:
 goto _LL4;case 34U: _LL3: _LL4:
 goto _LL6;case 59U: _LL5: _LL6:
 goto _LL8;case 38U: _LL7: _LL8:
 goto _LLA;case 40U: _LL9: _LLA:
 goto _LLC;case 41U: _LLB: _LLC:
 goto _LLE;case 124U: _LLD: _LLE:
 goto _LL10;case 94U: _LLF: _LL10:
 goto _LL12;case 60U: _LL11: _LL12:
 goto _LL14;case 62U: _LL13: _LL14:
 goto _LL16;case 10U: _LL15: _LL16:
# 713
 goto _LL18;case 9U: _LL17: _LL18:
 return 1;default: _LL19: _LL1A:
 return 0;}_LL0:;}
# 720
static struct _fat_ptr Cyc_sh_escape_string(struct _fat_ptr s){
unsigned long len=({Cyc_strlen((struct _fat_ptr)s);});
# 724
int single_quotes=0;
int other_special=0;
{int i=0;for(0;(unsigned long)i < len;++ i){
char c=*((const char*)_check_fat_subscript(s,sizeof(char),i));
if((int)c == (int)'\'')++ single_quotes;else{
if(({Cyc_is_other_special(c);}))++ other_special;}}}
# 733
if(single_quotes == 0 && other_special == 0)
return s;
# 733
if(single_quotes == 0)
# 738
return(struct _fat_ptr)({struct _fat_ptr(*_tmpE7)(struct Cyc_List_List*)=Cyc_strconcat_l;struct Cyc_List_List*_tmpE8=({struct _fat_ptr*_tmpE9[3U];({struct _fat_ptr*_tmp368=({struct _fat_ptr*_tmpEB=_cycalloc(sizeof(*_tmpEB));({struct _fat_ptr _tmp367=({const char*_tmpEA="'";_tag_fat(_tmpEA,sizeof(char),2U);});*_tmpEB=_tmp367;});_tmpEB;});_tmpE9[0]=_tmp368;}),({struct _fat_ptr*_tmp366=({struct _fat_ptr*_tmpEC=_cycalloc(sizeof(*_tmpEC));*_tmpEC=(struct _fat_ptr)s;_tmpEC;});_tmpE9[1]=_tmp366;}),({struct _fat_ptr*_tmp365=({struct _fat_ptr*_tmpEE=_cycalloc(sizeof(*_tmpEE));({struct _fat_ptr _tmp364=({const char*_tmpED="'";_tag_fat(_tmpED,sizeof(char),2U);});*_tmpEE=_tmp364;});_tmpEE;});_tmpE9[2]=_tmp365;});Cyc_List_list(_tag_fat(_tmpE9,sizeof(struct _fat_ptr*),3U));});_tmpE7(_tmpE8);});{
# 733
unsigned long len2=(len + (unsigned long)single_quotes)+ (unsigned long)other_special;
# 742
struct _fat_ptr s2=({unsigned _tmpF6=(len2 + (unsigned long)1)+ 1U;char*_tmpF5=_cycalloc_atomic(_check_times(_tmpF6,sizeof(char)));({{unsigned _tmp2FD=len2 + (unsigned long)1;unsigned i;for(i=0;i < _tmp2FD;++ i){_tmpF5[i]='\000';}_tmpF5[_tmp2FD]=0;}0;});_tag_fat(_tmpF5,sizeof(char),_tmpF6);});
int i=0;
int j=0;
for(0;(unsigned long)i < len;++ i){
char c=*((const char*)_check_fat_subscript(s,sizeof(char),i));
if((int)c == (int)'\'' ||({Cyc_is_other_special(c);}))
({struct _fat_ptr _tmpEF=_fat_ptr_plus(s2,sizeof(char),j ++);char _tmpF0=*((char*)_check_fat_subscript(_tmpEF,sizeof(char),0U));char _tmpF1='\\';if(_get_fat_size(_tmpEF,sizeof(char))== 1U &&(_tmpF0 == 0 && _tmpF1 != 0))_throw_arraybounds();*((char*)_tmpEF.curr)=_tmpF1;});
# 747
({struct _fat_ptr _tmpF2=_fat_ptr_plus(s2,sizeof(char),j ++);char _tmpF3=*((char*)_check_fat_subscript(_tmpF2,sizeof(char),0U));char _tmpF4=c;if(_get_fat_size(_tmpF2,sizeof(char))== 1U &&(_tmpF3 == 0 && _tmpF4 != 0))_throw_arraybounds();*((char*)_tmpF2.curr)=_tmpF4;});}
# 751
return(struct _fat_ptr)s2;}}
# 753
static struct _fat_ptr*Cyc_sh_escape_stringptr(struct _fat_ptr*sp){
return({struct _fat_ptr*_tmpF8=_cycalloc(sizeof(*_tmpF8));({struct _fat_ptr _tmp369=({Cyc_sh_escape_string(*sp);});*_tmpF8=_tmp369;});_tmpF8;});}
# 758
static void Cyc_process_file(struct _fat_ptr filename){
# 760
struct _fat_ptr basename=({Cyc_make_base_filename(filename,Cyc_output_file);});
struct _fat_ptr preprocfile=({({struct _fat_ptr _tmp36A=(struct _fat_ptr)basename;Cyc_strconcat(_tmp36A,({const char*_tmp164=".cyp";_tag_fat(_tmp164,sizeof(char),5U);}));});});
struct _fat_ptr interfacefile=({char*_tmp36C=(char*)Cyc_specified_interface.curr;_tmp36C != (char*)(_tag_fat(0,0,0)).curr;})?Cyc_specified_interface:(struct _fat_ptr)({({struct _fat_ptr _tmp36B=(struct _fat_ptr)basename;Cyc_strconcat(_tmp36B,({const char*_tmp163=".cyci";_tag_fat(_tmp163,sizeof(char),6U);}));});});
# 764
struct _fat_ptr interfaceobjfile=({({struct _fat_ptr _tmp36D=(struct _fat_ptr)basename;Cyc_strconcat(_tmp36D,({const char*_tmp162=".cycio";_tag_fat(_tmp162,sizeof(char),7U);}));});});
# 768
struct _fat_ptr cfile=({struct _fat_ptr(*_tmp15E)(struct _fat_ptr,struct _fat_ptr)=Cyc_strconcat;struct _fat_ptr _tmp15F=(struct _fat_ptr)({Cyc_make_base_filename(filename,0);});struct _fat_ptr _tmp160=({const char*_tmp161=".c";_tag_fat(_tmp161,sizeof(char),3U);});_tmp15E(_tmp15F,_tmp160);});
struct _fat_ptr*preprocfileptr=({struct _fat_ptr*_tmp15D=_cycalloc(sizeof(*_tmp15D));*_tmp15D=preprocfile;_tmp15D;});
# 771
if(Cyc_v_r)({struct Cyc_String_pa_PrintArg_struct _tmpFC=({struct Cyc_String_pa_PrintArg_struct _tmp2FE;_tmp2FE.tag=0U,_tmp2FE.f1=(struct _fat_ptr)((struct _fat_ptr)filename);_tmp2FE;});void*_tmpFA[1U];_tmpFA[0]=& _tmpFC;({struct Cyc___cycFILE*_tmp36F=Cyc_stderr;struct _fat_ptr _tmp36E=({const char*_tmpFB="Compiling %s\n";_tag_fat(_tmpFB,sizeof(char),14U);});Cyc_fprintf(_tmp36F,_tmp36E,_tag_fat(_tmpFA,sizeof(void*),1U));});});{struct Cyc___cycFILE*f0=({({struct _fat_ptr _tmp371=filename;struct _fat_ptr _tmp370=({const char*_tmp15B="r";_tag_fat(_tmp15B,sizeof(char),2U);});Cyc_try_file_open(_tmp371,_tmp370,({const char*_tmp15C="input file";_tag_fat(_tmp15C,sizeof(char),11U);}));});});
# 775
if(Cyc_compile_failure || !((unsigned)f0))
return;
# 775
({Cyc_fclose(f0);});{
# 781
struct _fat_ptr cppargs_string=({struct _fat_ptr(*_tmp150)(struct Cyc_List_List*,struct _fat_ptr)=Cyc_str_sepstr;struct Cyc_List_List*_tmp151=({struct Cyc_List_List*_tmp158=_cycalloc(sizeof(*_tmp158));
({struct _fat_ptr*_tmp374=({struct _fat_ptr*_tmp153=_cycalloc(sizeof(*_tmp153));({struct _fat_ptr _tmp373=(struct _fat_ptr)({const char*_tmp152="";_tag_fat(_tmp152,sizeof(char),1U);});*_tmp153=_tmp373;});_tmp153;});_tmp158->hd=_tmp374;}),({
struct Cyc_List_List*_tmp372=({struct Cyc_List_List*(*_tmp154)(struct _fat_ptr*(*f)(struct _fat_ptr*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp155)(struct _fat_ptr*(*f)(struct _fat_ptr*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct _fat_ptr*(*f)(struct _fat_ptr*),struct Cyc_List_List*x))Cyc_List_map;_tmp155;});struct _fat_ptr*(*_tmp156)(struct _fat_ptr*sp)=Cyc_sh_escape_stringptr;struct Cyc_List_List*_tmp157=({Cyc_List_rev(Cyc_cppargs);});_tmp154(_tmp156,_tmp157);});_tmp158->tl=_tmp372;});_tmp158;});struct _fat_ptr _tmp159=({const char*_tmp15A=" ";_tag_fat(_tmp15A,sizeof(char),2U);});_tmp150(_tmp151,_tmp159);});
# 791
struct Cyc_List_List*stdinc_dirs=({Cyc_add_subdir(Cyc_Specsfile_cyclone_exec_path,Cyc_Specsfile_target_arch);});
stdinc_dirs=({({struct Cyc_List_List*_tmp375=stdinc_dirs;Cyc_add_subdir(_tmp375,({const char*_tmpFD="include";_tag_fat(_tmpFD,sizeof(char),8U);}));});});
if(({Cyc_strlen((struct _fat_ptr)Cyc_def_inc_path);})> (unsigned long)0)
stdinc_dirs=({struct Cyc_List_List*_tmpFF=_cycalloc(sizeof(*_tmpFF));({struct _fat_ptr*_tmp376=({struct _fat_ptr*_tmpFE=_cycalloc(sizeof(*_tmpFE));*_tmpFE=Cyc_def_inc_path;_tmpFE;});_tmpFF->hd=_tmp376;}),_tmpFF->tl=stdinc_dirs;_tmpFF;});{
# 793
char*cyclone_include_path=({getenv("CYCLONE_INCLUDE_PATH");});
# 797
if(cyclone_include_path != 0)
stdinc_dirs=({struct Cyc_List_List*(*_tmp100)(struct Cyc_List_List*x,struct Cyc_List_List*y)=Cyc_List_append;struct Cyc_List_List*_tmp101=({Cyc_split_by_char(({char*_tmp102=cyclone_include_path;_tag_fat(_tmp102,sizeof(char),_get_zero_arr_size_char((void*)_tmp102,1U));}),':');});struct Cyc_List_List*_tmp103=stdinc_dirs;_tmp100(_tmp101,_tmp103);});{
# 797
struct _fat_ptr stdinc_string=(struct _fat_ptr)({struct _fat_ptr(*_tmp148)(struct Cyc_List_List*,struct _fat_ptr)=Cyc_str_sepstr;struct Cyc_List_List*_tmp149=({struct Cyc_List_List*_tmp14D=_cycalloc(sizeof(*_tmp14D));
# 802
({struct _fat_ptr*_tmp37A=({struct _fat_ptr*_tmp14B=_cycalloc(sizeof(*_tmp14B));({struct _fat_ptr _tmp379=(struct _fat_ptr)({const char*_tmp14A="";_tag_fat(_tmp14A,sizeof(char),1U);});*_tmp14B=_tmp379;});_tmp14B;});_tmp14D->hd=_tmp37A;}),({
struct Cyc_List_List*_tmp378=({({struct Cyc_List_List*(*_tmp377)(struct _fat_ptr*(*f)(struct _fat_ptr*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp14C)(struct _fat_ptr*(*f)(struct _fat_ptr*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct _fat_ptr*(*f)(struct _fat_ptr*),struct Cyc_List_List*x))Cyc_List_map;_tmp14C;});_tmp377(Cyc_sh_escape_stringptr,stdinc_dirs);});});_tmp14D->tl=_tmp378;});_tmp14D;});struct _fat_ptr _tmp14E=({const char*_tmp14F=" -I";_tag_fat(_tmp14F,sizeof(char),4U);});_tmp148(_tmp149,_tmp14E);});
# 805
struct _fat_ptr ofile_string;
if(Cyc_stop_after_cpp_r){
if(Cyc_output_file != 0)
ofile_string=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp106=({struct Cyc_String_pa_PrintArg_struct _tmp2FF;_tmp2FF.tag=0U,_tmp2FF.f1=(struct _fat_ptr)((struct _fat_ptr)*((struct _fat_ptr*)_check_null(Cyc_output_file)));_tmp2FF;});void*_tmp104[1U];_tmp104[0]=& _tmp106;({struct _fat_ptr _tmp37B=({const char*_tmp105=" > %s";_tag_fat(_tmp105,sizeof(char),6U);});Cyc_aprintf(_tmp37B,_tag_fat(_tmp104,sizeof(void*),1U));});});else{
# 810
ofile_string=({const char*_tmp107="";_tag_fat(_tmp107,sizeof(char),1U);});}}else{
# 812
ofile_string=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp10A=({struct Cyc_String_pa_PrintArg_struct _tmp300;_tmp300.tag=0U,({struct _fat_ptr _tmp37C=(struct _fat_ptr)((struct _fat_ptr)({Cyc_sh_escape_string((struct _fat_ptr)preprocfile);}));_tmp300.f1=_tmp37C;});_tmp300;});void*_tmp108[1U];_tmp108[0]=& _tmp10A;({struct _fat_ptr _tmp37D=({const char*_tmp109=" > %s";_tag_fat(_tmp109,sizeof(char),6U);});Cyc_aprintf(_tmp37D,_tag_fat(_tmp108,sizeof(void*),1U));});});}{
# 814
struct _fat_ptr fixup_string;
if(Cyc_produce_dependencies){
# 818
if(Cyc_dependencies_target == 0)
# 822
fixup_string=({const char*_tmp10B=" | sed 's/^\\(.*\\)\\.cyc\\.o:/\\1.o:/'";_tag_fat(_tmp10B,sizeof(char),35U);});else{
# 827
fixup_string=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp10E=({struct Cyc_String_pa_PrintArg_struct _tmp301;_tmp301.tag=0U,_tmp301.f1=(struct _fat_ptr)((struct _fat_ptr)*((struct _fat_ptr*)_check_null(Cyc_dependencies_target)));_tmp301;});void*_tmp10C[1U];_tmp10C[0]=& _tmp10E;({struct _fat_ptr _tmp37E=({const char*_tmp10D=" | sed 's/^.*\\.cyc\\.o:/%s:/'";_tag_fat(_tmp10D,sizeof(char),29U);});Cyc_aprintf(_tmp37E,_tag_fat(_tmp10C,sizeof(void*),1U));});});}}else{
# 831
fixup_string=({const char*_tmp10F="";_tag_fat(_tmp10F,sizeof(char),1U);});}{
# 833
struct _fat_ptr cmd=({struct Cyc_String_pa_PrintArg_struct _tmp142=({struct Cyc_String_pa_PrintArg_struct _tmp309;_tmp309.tag=0U,_tmp309.f1=(struct _fat_ptr)((struct _fat_ptr)Cyc_cpp);_tmp309;});struct Cyc_String_pa_PrintArg_struct _tmp143=({struct Cyc_String_pa_PrintArg_struct _tmp308;_tmp308.tag=0U,_tmp308.f1=(struct _fat_ptr)((struct _fat_ptr)cppargs_string);_tmp308;});struct Cyc_String_pa_PrintArg_struct _tmp144=({struct Cyc_String_pa_PrintArg_struct _tmp307;_tmp307.tag=0U,_tmp307.f1=(struct _fat_ptr)((struct _fat_ptr)stdinc_string);_tmp307;});struct Cyc_String_pa_PrintArg_struct _tmp145=({struct Cyc_String_pa_PrintArg_struct _tmp306;_tmp306.tag=0U,({
# 836
struct _fat_ptr _tmp37F=(struct _fat_ptr)((struct _fat_ptr)({Cyc_sh_escape_string(filename);}));_tmp306.f1=_tmp37F;});_tmp306;});struct Cyc_String_pa_PrintArg_struct _tmp146=({struct Cyc_String_pa_PrintArg_struct _tmp305;_tmp305.tag=0U,_tmp305.f1=(struct _fat_ptr)((struct _fat_ptr)fixup_string);_tmp305;});struct Cyc_String_pa_PrintArg_struct _tmp147=({struct Cyc_String_pa_PrintArg_struct _tmp304;_tmp304.tag=0U,_tmp304.f1=(struct _fat_ptr)((struct _fat_ptr)ofile_string);_tmp304;});void*_tmp140[6U];_tmp140[0]=& _tmp142,_tmp140[1]=& _tmp143,_tmp140[2]=& _tmp144,_tmp140[3]=& _tmp145,_tmp140[4]=& _tmp146,_tmp140[5]=& _tmp147;({struct _fat_ptr _tmp380=({const char*_tmp141="%s %s%s %s%s%s";_tag_fat(_tmp141,sizeof(char),15U);});Cyc_aprintf(_tmp380,_tag_fat(_tmp140,sizeof(void*),6U));});});
# 839
if(Cyc_v_r)({struct Cyc_String_pa_PrintArg_struct _tmp112=({struct Cyc_String_pa_PrintArg_struct _tmp302;_tmp302.tag=0U,_tmp302.f1=(struct _fat_ptr)((struct _fat_ptr)cmd);_tmp302;});void*_tmp110[1U];_tmp110[0]=& _tmp112;({struct Cyc___cycFILE*_tmp382=Cyc_stderr;struct _fat_ptr _tmp381=({const char*_tmp111="%s\n";_tag_fat(_tmp111,sizeof(char),4U);});Cyc_fprintf(_tmp382,_tmp381,_tag_fat(_tmp110,sizeof(void*),1U));});});if(({system((const char*)_check_null(_untag_fat_ptr(cmd,sizeof(char),1U)));})!= 0){
# 841
Cyc_compile_failure=1;
({void*_tmp113=0U;({struct Cyc___cycFILE*_tmp384=Cyc_stderr;struct _fat_ptr _tmp383=({const char*_tmp114="\nError: preprocessing\n";_tag_fat(_tmp114,sizeof(char),23U);});Cyc_fprintf(_tmp384,_tmp383,_tag_fat(_tmp113,sizeof(void*),0U));});});
if(!Cyc_stop_after_cpp_r)({Cyc_remove_file((struct _fat_ptr)preprocfile);});return;}
# 839
if(Cyc_stop_after_cpp_r)
# 846
return;
# 839
({Cyc_Position_reset_position((struct _fat_ptr)preprocfile);});{
# 850
struct Cyc___cycFILE*in_file=({({struct _fat_ptr _tmp386=(struct _fat_ptr)preprocfile;struct _fat_ptr _tmp385=({const char*_tmp13E="r";_tag_fat(_tmp13E,sizeof(char),2U);});Cyc_try_file_open(_tmp386,_tmp385,({const char*_tmp13F="file";_tag_fat(_tmp13F,sizeof(char),5U);}));});});
if(Cyc_compile_failure)return;{struct Cyc_List_List*tds=0;
# 856
tds=({({struct Cyc_List_List*(*_tmp38A)(struct _fat_ptr stage_name,struct Cyc_List_List*tds,struct Cyc_List_List*(*f)(struct Cyc___cycFILE*,struct Cyc_List_List*),struct Cyc___cycFILE*env,void(*on_fail)(struct _fat_ptr*),struct _fat_ptr*failenv)=({struct Cyc_List_List*(*_tmp115)(struct _fat_ptr stage_name,struct Cyc_List_List*tds,struct Cyc_List_List*(*f)(struct Cyc___cycFILE*,struct Cyc_List_List*),struct Cyc___cycFILE*env,void(*on_fail)(struct _fat_ptr*),struct _fat_ptr*failenv)=(struct Cyc_List_List*(*)(struct _fat_ptr stage_name,struct Cyc_List_List*tds,struct Cyc_List_List*(*f)(struct Cyc___cycFILE*,struct Cyc_List_List*),struct Cyc___cycFILE*env,void(*on_fail)(struct _fat_ptr*),struct _fat_ptr*failenv))Cyc_do_stage;_tmp115;});struct _fat_ptr _tmp389=({const char*_tmp116="parsing";_tag_fat(_tmp116,sizeof(char),8U);});struct Cyc_List_List*_tmp388=tds;struct Cyc___cycFILE*_tmp387=(struct Cyc___cycFILE*)_check_null(in_file);_tmp38A(_tmp389,_tmp388,Cyc_do_parse,_tmp387,Cyc_remove_fileptr,preprocfileptr);});});
# 858
if(Cyc_compile_failure)return;if(Cyc_Absyn_porting_c_code){
# 861
({Cyc_Port_port(tds);});
return;}
# 858
({({int(*_tmp38D)(struct _fat_ptr stage_name,struct Cyc_List_List*tds,int(*f)(int,struct Cyc_List_List*),int env,void(*on_fail)(struct _fat_ptr*),struct _fat_ptr*failenv)=({int(*_tmp117)(struct _fat_ptr stage_name,struct Cyc_List_List*tds,int(*f)(int,struct Cyc_List_List*),int env,void(*on_fail)(struct _fat_ptr*),struct _fat_ptr*failenv)=(int(*)(struct _fat_ptr stage_name,struct Cyc_List_List*tds,int(*f)(int,struct Cyc_List_List*),int env,void(*on_fail)(struct _fat_ptr*),struct _fat_ptr*failenv))Cyc_do_stage;_tmp117;});struct _fat_ptr _tmp38C=({const char*_tmp118="binding";_tag_fat(_tmp118,sizeof(char),8U);});struct Cyc_List_List*_tmp38B=tds;_tmp38D(_tmp38C,_tmp38B,Cyc_do_binding,1,Cyc_remove_fileptr,preprocfileptr);});});
# 866
if(Cyc_compile_failure)return;{struct Cyc_JumpAnalysis_Jump_Anal_Result*jump_tables;
# 869
{struct Cyc_Tcenv_Tenv*te=({Cyc_Tcenv_tc_init();});
# 871
if(Cyc_parseonly_r)goto PRINTC;tds=({({struct Cyc_List_List*(*_tmp391)(struct _fat_ptr stage_name,struct Cyc_List_List*tds,struct Cyc_List_List*(*f)(struct Cyc_Tcenv_Tenv*,struct Cyc_List_List*),struct Cyc_Tcenv_Tenv*env,void(*on_fail)(struct _fat_ptr*),struct _fat_ptr*failenv)=({
# 874
struct Cyc_List_List*(*_tmp119)(struct _fat_ptr stage_name,struct Cyc_List_List*tds,struct Cyc_List_List*(*f)(struct Cyc_Tcenv_Tenv*,struct Cyc_List_List*),struct Cyc_Tcenv_Tenv*env,void(*on_fail)(struct _fat_ptr*),struct _fat_ptr*failenv)=(struct Cyc_List_List*(*)(struct _fat_ptr stage_name,struct Cyc_List_List*tds,struct Cyc_List_List*(*f)(struct Cyc_Tcenv_Tenv*,struct Cyc_List_List*),struct Cyc_Tcenv_Tenv*env,void(*on_fail)(struct _fat_ptr*),struct _fat_ptr*failenv))Cyc_do_stage;_tmp119;});struct _fat_ptr _tmp390=({const char*_tmp11A="type checking";_tag_fat(_tmp11A,sizeof(char),14U);});struct Cyc_List_List*_tmp38F=tds;struct Cyc_Tcenv_Tenv*_tmp38E=te;_tmp391(_tmp390,_tmp38F,Cyc_do_typecheck,_tmp38E,Cyc_remove_fileptr,preprocfileptr);});});
# 876
if(Cyc_compile_failure)return;jump_tables=({({struct Cyc_JumpAnalysis_Jump_Anal_Result*(*_tmp394)(struct _fat_ptr stage_name,struct Cyc_List_List*tds,struct Cyc_JumpAnalysis_Jump_Anal_Result*(*f)(int,struct Cyc_List_List*),int env,void(*on_fail)(struct _fat_ptr*),struct _fat_ptr*failenv)=({
# 879
struct Cyc_JumpAnalysis_Jump_Anal_Result*(*_tmp11B)(struct _fat_ptr stage_name,struct Cyc_List_List*tds,struct Cyc_JumpAnalysis_Jump_Anal_Result*(*f)(int,struct Cyc_List_List*),int env,void(*on_fail)(struct _fat_ptr*),struct _fat_ptr*failenv)=(struct Cyc_JumpAnalysis_Jump_Anal_Result*(*)(struct _fat_ptr stage_name,struct Cyc_List_List*tds,struct Cyc_JumpAnalysis_Jump_Anal_Result*(*f)(int,struct Cyc_List_List*),int env,void(*on_fail)(struct _fat_ptr*),struct _fat_ptr*failenv))Cyc_do_stage;_tmp11B;});struct _fat_ptr _tmp393=({const char*_tmp11C="jump checking";_tag_fat(_tmp11C,sizeof(char),14U);});struct Cyc_List_List*_tmp392=tds;_tmp394(_tmp393,_tmp392,Cyc_do_jumpanalysis,1,Cyc_remove_fileptr,preprocfileptr);});});
# 881
if(Cyc_compile_failure)return;if(Cyc_tc_r)
# 885
goto PRINTC;
# 881
({({struct Cyc_List_List*(*_tmp398)(struct _fat_ptr stage_name,struct Cyc_List_List*tds,struct Cyc_List_List*(*f)(struct Cyc_JumpAnalysis_Jump_Anal_Result*,struct Cyc_List_List*),struct Cyc_JumpAnalysis_Jump_Anal_Result*env,void(*on_fail)(struct _fat_ptr*),struct _fat_ptr*failenv)=({struct Cyc_List_List*(*_tmp11D)(struct _fat_ptr stage_name,struct Cyc_List_List*tds,struct Cyc_List_List*(*f)(struct Cyc_JumpAnalysis_Jump_Anal_Result*,struct Cyc_List_List*),struct Cyc_JumpAnalysis_Jump_Anal_Result*env,void(*on_fail)(struct _fat_ptr*),struct _fat_ptr*failenv)=(struct Cyc_List_List*(*)(struct _fat_ptr stage_name,struct Cyc_List_List*tds,struct Cyc_List_List*(*f)(struct Cyc_JumpAnalysis_Jump_Anal_Result*,struct Cyc_List_List*),struct Cyc_JumpAnalysis_Jump_Anal_Result*env,void(*on_fail)(struct _fat_ptr*),struct _fat_ptr*failenv))Cyc_do_stage;_tmp11D;});struct _fat_ptr _tmp397=({const char*_tmp11E="control-flow checking";_tag_fat(_tmp11E,sizeof(char),22U);});struct Cyc_List_List*_tmp396=tds;struct Cyc_JumpAnalysis_Jump_Anal_Result*_tmp395=jump_tables;_tmp398(_tmp397,_tmp396,Cyc_do_cfcheck,_tmp395,Cyc_remove_fileptr,preprocfileptr);});});
# 888
if(Cyc_compile_failure)return;({({int(*_tmp39C)(struct _fat_ptr stage_name,struct Cyc_List_List*tds,int(*f)(struct Cyc_JumpAnalysis_Jump_Anal_Result*,struct Cyc_List_List*),struct Cyc_JumpAnalysis_Jump_Anal_Result*env,void(*on_fail)(struct _fat_ptr*),struct _fat_ptr*failenv)=({int(*_tmp11F)(struct _fat_ptr stage_name,struct Cyc_List_List*tds,int(*f)(struct Cyc_JumpAnalysis_Jump_Anal_Result*,struct Cyc_List_List*),struct Cyc_JumpAnalysis_Jump_Anal_Result*env,void(*on_fail)(struct _fat_ptr*),struct _fat_ptr*failenv)=(int(*)(struct _fat_ptr stage_name,struct Cyc_List_List*tds,int(*f)(struct Cyc_JumpAnalysis_Jump_Anal_Result*,struct Cyc_List_List*),struct Cyc_JumpAnalysis_Jump_Anal_Result*env,void(*on_fail)(struct _fat_ptr*),struct _fat_ptr*failenv))Cyc_do_stage;_tmp11F;});struct _fat_ptr _tmp39B=({const char*_tmp120="check insertion";_tag_fat(_tmp120,sizeof(char),16U);});struct Cyc_List_List*_tmp39A=tds;struct Cyc_JumpAnalysis_Jump_Anal_Result*_tmp399=jump_tables;_tmp39C(_tmp39B,_tmp39A,Cyc_do_insert_checks,_tmp399,Cyc_remove_fileptr,preprocfileptr);});});
# 891
if(Cyc_compile_failure)return;({({int(*_tmp3A0)(struct _fat_ptr stage_name,struct Cyc_List_List*tds,int(*f)(struct Cyc_JumpAnalysis_Jump_Anal_Result*,struct Cyc_List_List*),struct Cyc_JumpAnalysis_Jump_Anal_Result*env,void(*on_fail)(struct _fat_ptr*),struct _fat_ptr*failenv)=({int(*_tmp121)(struct _fat_ptr stage_name,struct Cyc_List_List*tds,int(*f)(struct Cyc_JumpAnalysis_Jump_Anal_Result*,struct Cyc_List_List*),struct Cyc_JumpAnalysis_Jump_Anal_Result*env,void(*on_fail)(struct _fat_ptr*),struct _fat_ptr*failenv)=(int(*)(struct _fat_ptr stage_name,struct Cyc_List_List*tds,int(*f)(struct Cyc_JumpAnalysis_Jump_Anal_Result*,struct Cyc_List_List*),struct Cyc_JumpAnalysis_Jump_Anal_Result*env,void(*on_fail)(struct _fat_ptr*),struct _fat_ptr*failenv))Cyc_do_stage;_tmp121;});struct _fat_ptr _tmp39F=({const char*_tmp122="I/O effect checking";_tag_fat(_tmp122,sizeof(char),20U);});struct Cyc_List_List*_tmp39E=tds;struct Cyc_JumpAnalysis_Jump_Anal_Result*_tmp39D=jump_tables;_tmp3A0(_tmp39F,_tmp39E,Cyc_do_ioeffect_analysis,_tmp39D,Cyc_remove_fileptr,preprocfileptr);});});
# 895
if(Cyc_compile_failure)return;if(Cyc_generate_interface_r){
# 899
struct Cyc___cycFILE*inter_file=({({struct _fat_ptr _tmp3A2=({const char*_tmp123=(const char*)_check_null(_untag_fat_ptr(interfacefile,sizeof(char),1U));_tag_fat(_tmp123,sizeof(char),_get_zero_arr_size_char((void*)_tmp123,1U));});struct _fat_ptr _tmp3A1=({const char*_tmp124="w";_tag_fat(_tmp124,sizeof(char),2U);});Cyc_try_file_open(_tmp3A2,_tmp3A1,({const char*_tmp125="interface file";_tag_fat(_tmp125,sizeof(char),15U);}));});});
if(inter_file == 0){
Cyc_compile_failure=1;
return;}
# 900
({Cyc_Absyndump_set_params(& Cyc_Absynpp_cyci_params_r);});
# 905
({Cyc_Absyndump_dump_interface(tds,inter_file);});
({Cyc_fclose(inter_file);});
({Cyc_Absynpp_set_params(& Cyc_Absynpp_tc_params_r);});}
# 895
if(Cyc_ic_r){
# 912
struct Cyc___cycFILE*inter_file=({Cyc_fopen((const char*)_check_null(_untag_fat_ptr(interfacefile,sizeof(char),1U)),"r");});
struct Cyc___cycFILE*inter_objfile=({({struct _fat_ptr _tmp3A4=(struct _fat_ptr)interfaceobjfile;struct _fat_ptr _tmp3A3=({const char*_tmp128="w";_tag_fat(_tmp128,sizeof(char),2U);});Cyc_try_file_open(_tmp3A4,_tmp3A3,({const char*_tmp129="interface object file";_tag_fat(_tmp129,sizeof(char),22U);}));});});
if(inter_objfile == 0){
Cyc_compile_failure=1;
return;}
# 914
({Cyc_Position_reset_position(interfacefile);});{
# 919
struct _tuple27 int_env=({struct _tuple27 _tmp303;_tmp303.f1=te,_tmp303.f2=inter_file,_tmp303.f3=inter_objfile;_tmp303;});
tds=({({struct Cyc_List_List*(*_tmp3A6)(struct _fat_ptr stage_name,struct Cyc_List_List*tds,struct Cyc_List_List*(*f)(struct _tuple27*,struct Cyc_List_List*),struct _tuple27*env,void(*on_fail)(struct _tuple27*),struct _tuple27*failenv)=({struct Cyc_List_List*(*_tmp126)(struct _fat_ptr stage_name,struct Cyc_List_List*tds,struct Cyc_List_List*(*f)(struct _tuple27*,struct Cyc_List_List*),struct _tuple27*env,void(*on_fail)(struct _tuple27*),struct _tuple27*failenv)=(struct Cyc_List_List*(*)(struct _fat_ptr stage_name,struct Cyc_List_List*tds,struct Cyc_List_List*(*f)(struct _tuple27*,struct Cyc_List_List*),struct _tuple27*env,void(*on_fail)(struct _tuple27*),struct _tuple27*failenv))Cyc_do_stage;_tmp126;});struct _fat_ptr _tmp3A5=({const char*_tmp127="interface checking";_tag_fat(_tmp127,sizeof(char),19U);});_tmp3A6(_tmp3A5,tds,Cyc_do_interface,& int_env,Cyc_interface_fail,& int_env);});});}}}
# 924
if(Cyc_cf_r)goto PRINTC;tds=({({struct Cyc_List_List*(*_tmp3AA)(struct _fat_ptr stage_name,struct Cyc_List_List*tds,struct Cyc_List_List*(*f)(struct Cyc_Hashtable_Table*,struct Cyc_List_List*),struct Cyc_Hashtable_Table*env,void(*on_fail)(struct _fat_ptr*),struct _fat_ptr*failenv)=({
# 927
struct Cyc_List_List*(*_tmp12A)(struct _fat_ptr stage_name,struct Cyc_List_List*tds,struct Cyc_List_List*(*f)(struct Cyc_Hashtable_Table*,struct Cyc_List_List*),struct Cyc_Hashtable_Table*env,void(*on_fail)(struct _fat_ptr*),struct _fat_ptr*failenv)=(struct Cyc_List_List*(*)(struct _fat_ptr stage_name,struct Cyc_List_List*tds,struct Cyc_List_List*(*f)(struct Cyc_Hashtable_Table*,struct Cyc_List_List*),struct Cyc_Hashtable_Table*env,void(*on_fail)(struct _fat_ptr*),struct _fat_ptr*failenv))Cyc_do_stage;_tmp12A;});struct _fat_ptr _tmp3A9=({const char*_tmp12B="translation to C";_tag_fat(_tmp12B,sizeof(char),17U);});struct Cyc_List_List*_tmp3A8=tds;struct Cyc_Hashtable_Table*_tmp3A7=jump_tables->pop_tables;_tmp3AA(_tmp3A9,_tmp3A8,Cyc_do_translate,_tmp3A7,Cyc_remove_fileptr,preprocfileptr);});});
# 930
tds=({({struct Cyc_List_List*(*_tmp3AD)(struct _fat_ptr stage_name,struct Cyc_List_List*tds,struct Cyc_List_List*(*f)(int,struct Cyc_List_List*),int env,void(*on_fail)(int),int failenv)=({struct Cyc_List_List*(*_tmp12C)(struct _fat_ptr stage_name,struct Cyc_List_List*tds,struct Cyc_List_List*(*f)(int,struct Cyc_List_List*),int env,void(*on_fail)(int),int failenv)=(struct Cyc_List_List*(*)(struct _fat_ptr stage_name,struct Cyc_List_List*tds,struct Cyc_List_List*(*f)(int,struct Cyc_List_List*),int env,void(*on_fail)(int),int failenv))Cyc_do_stage;_tmp12C;});struct _fat_ptr _tmp3AC=({const char*_tmp12D="aggregate removal";_tag_fat(_tmp12D,sizeof(char),18U);});struct Cyc_List_List*_tmp3AB=tds;_tmp3AD(_tmp3AC,_tmp3AB,Cyc_do_removeaggrs,1,({void(*_tmp12E)(int x)=(void(*)(int x))Cyc_ignore;_tmp12E;}),1);});});
if(Cyc_compile_failure)return;if(Cyc_toseqc_r)
# 935
tds=({({struct Cyc_List_List*(*_tmp3B0)(struct _fat_ptr stage_name,struct Cyc_List_List*tds,struct Cyc_List_List*(*f)(int,struct Cyc_List_List*),int env,void(*on_fail)(int),int failenv)=({struct Cyc_List_List*(*_tmp12F)(struct _fat_ptr stage_name,struct Cyc_List_List*tds,struct Cyc_List_List*(*f)(int,struct Cyc_List_List*),int env,void(*on_fail)(int),int failenv)=(struct Cyc_List_List*(*)(struct _fat_ptr stage_name,struct Cyc_List_List*tds,struct Cyc_List_List*(*f)(int,struct Cyc_List_List*),int env,void(*on_fail)(int),int failenv))Cyc_do_stage;_tmp12F;});struct _fat_ptr _tmp3AF=({const char*_tmp130="post-pass to L-to-R evaluation order";_tag_fat(_tmp130,sizeof(char),37U);});struct Cyc_List_List*_tmp3AE=tds;_tmp3B0(_tmp3AF,_tmp3AE,Cyc_do_toseqc,1,({void(*_tmp131)(int x)=(void(*)(int x))Cyc_ignore;_tmp131;}),1);});});
# 931
({Cyc_Toc_finish();});
# 940
if(Cyc_compile_failure)return;if(
# 943
Cyc_Cyclone_tovc_r || Cyc_elim_se_r)
tds=({({struct Cyc_List_List*(*_tmp3B3)(struct _fat_ptr stage_name,struct Cyc_List_List*tds,struct Cyc_List_List*(*f)(int,struct Cyc_List_List*),int env,void(*on_fail)(int),int failenv)=({struct Cyc_List_List*(*_tmp132)(struct _fat_ptr stage_name,struct Cyc_List_List*tds,struct Cyc_List_List*(*f)(int,struct Cyc_List_List*),int env,void(*on_fail)(int),int failenv)=(struct Cyc_List_List*(*)(struct _fat_ptr stage_name,struct Cyc_List_List*tds,struct Cyc_List_List*(*f)(int,struct Cyc_List_List*),int env,void(*on_fail)(int),int failenv))Cyc_do_stage;_tmp132;});struct _fat_ptr _tmp3B2=({const char*_tmp133="post-pass to VC";_tag_fat(_tmp133,sizeof(char),16U);});struct Cyc_List_List*_tmp3B1=tds;_tmp3B3(_tmp3B2,_tmp3B1,Cyc_do_tovc,1,({void(*_tmp134)(int x)=(void(*)(int x))Cyc_ignore;_tmp134;}),1);});});
# 940
if(Cyc_compile_failure)
# 945
return;
# 940
({Cyc_Warn_flush_warnings();});
# 948
({Cyc_remove_file((struct _fat_ptr)preprocfile);});
# 951
PRINTC: {
struct Cyc___cycFILE*out_file;
if((Cyc_parseonly_r || Cyc_tc_r)|| Cyc_cf_r){
if(Cyc_output_file != 0)
out_file=({({struct _fat_ptr _tmp3B5=*((struct _fat_ptr*)_check_null(Cyc_output_file));struct _fat_ptr _tmp3B4=({const char*_tmp135="w";_tag_fat(_tmp135,sizeof(char),2U);});Cyc_try_file_open(_tmp3B5,_tmp3B4,({const char*_tmp136="output file";_tag_fat(_tmp136,sizeof(char),12U);}));});});else{
# 957
out_file=Cyc_stdout;}}else{
if(Cyc_toc_r && Cyc_output_file != 0)
out_file=({({struct _fat_ptr _tmp3B7=*((struct _fat_ptr*)_check_null(Cyc_output_file));struct _fat_ptr _tmp3B6=({const char*_tmp137="w";_tag_fat(_tmp137,sizeof(char),2U);});Cyc_try_file_open(_tmp3B7,_tmp3B6,({const char*_tmp138="output file";_tag_fat(_tmp138,sizeof(char),12U);}));});});else{
# 961
out_file=({({struct _fat_ptr _tmp3B9=(struct _fat_ptr)cfile;struct _fat_ptr _tmp3B8=({const char*_tmp139="w";_tag_fat(_tmp139,sizeof(char),2U);});Cyc_try_file_open(_tmp3B9,_tmp3B8,({const char*_tmp13A="output file";_tag_fat(_tmp13A,sizeof(char),12U);}));});});}}
# 963
if(Cyc_compile_failure || !((unsigned)out_file))return;if(!Cyc_noprint_r){
# 966
struct _tuple28*env=({struct _tuple28*_tmp13D=_cycalloc(sizeof(*_tmp13D));_tmp13D->f1=out_file,_tmp13D->f2=cfile;_tmp13D;});
tds=({({struct Cyc_List_List*(*_tmp3BD)(struct _fat_ptr stage_name,struct Cyc_List_List*tds,struct Cyc_List_List*(*f)(struct _tuple28*,struct Cyc_List_List*),struct _tuple28*env,void(*on_fail)(struct _tuple28*),struct _tuple28*failenv)=({struct Cyc_List_List*(*_tmp13B)(struct _fat_ptr stage_name,struct Cyc_List_List*tds,struct Cyc_List_List*(*f)(struct _tuple28*,struct Cyc_List_List*),struct _tuple28*env,void(*on_fail)(struct _tuple28*),struct _tuple28*failenv)=(struct Cyc_List_List*(*)(struct _fat_ptr stage_name,struct Cyc_List_List*tds,struct Cyc_List_List*(*f)(struct _tuple28*,struct Cyc_List_List*),struct _tuple28*env,void(*on_fail)(struct _tuple28*),struct _tuple28*failenv))Cyc_do_stage;_tmp13B;});struct _fat_ptr _tmp3BC=({const char*_tmp13C="printing";_tag_fat(_tmp13C,sizeof(char),9U);});struct Cyc_List_List*_tmp3BB=tds;struct _tuple28*_tmp3BA=env;_tmp3BD(_tmp3BC,_tmp3BB,Cyc_do_print,_tmp3BA,Cyc_print_fail,env);});});}}}}}}}}}}}}static char _tmp166[8U]="<final>";
# 758
static struct _fat_ptr Cyc_final_str={_tmp166,_tmp166,_tmp166 + 8U};
# 973
static struct _fat_ptr*Cyc_final_strptr=& Cyc_final_str;
# 975
static struct Cyc_Interface_I*Cyc_read_cycio(struct _fat_ptr*n){
if(n == Cyc_final_strptr)
return({Cyc_Interface_final();});{
# 976
struct _fat_ptr basename;
# 980
{struct _handler_cons _tmp167;_push_handler(& _tmp167);{int _tmp169=0;if(setjmp(_tmp167.handler))_tmp169=1;if(!_tmp169){basename=(struct _fat_ptr)({Cyc_Filename_chop_extension(*n);});;_pop_handler();}else{void*_tmp168=(void*)Cyc_Core_get_exn_thrown();void*_tmp16A=_tmp168;void*_tmp16B;if(((struct Cyc_Core_Invalid_argument_exn_struct*)_tmp16A)->tag == Cyc_Core_Invalid_argument){_LL1: _LL2:
 basename=*n;goto _LL0;}else{_LL3: _tmp16B=_tmp16A;_LL4: {void*exn=_tmp16B;(int)_rethrow(exn);}}_LL0:;}}}{
# 983
struct _fat_ptr nf=({({struct _fat_ptr _tmp3BE=(struct _fat_ptr)basename;Cyc_strconcat(_tmp3BE,({const char*_tmp16E=".cycio";_tag_fat(_tmp16E,sizeof(char),7U);}));});});
struct Cyc___cycFILE*f=({({struct _fat_ptr _tmp3C0=(struct _fat_ptr)nf;struct _fat_ptr _tmp3BF=({const char*_tmp16C="rb";_tag_fat(_tmp16C,sizeof(char),3U);});Cyc_try_file_open(_tmp3C0,_tmp3BF,({const char*_tmp16D="interface object file";_tag_fat(_tmp16D,sizeof(char),22U);}));});});
if(f == 0){
Cyc_compile_failure=1;
({Cyc_remove_cfiles();});
({exit(1);});}
# 985
({Cyc_Position_reset_position((struct _fat_ptr)nf);});{
# 992
struct Cyc_Interface_I*i=({Cyc_Interface_load(f);});
({Cyc_file_close(f);});
return i;}}}}
# 997
static int Cyc_is_cfile(struct _fat_ptr*n){
return(int)*((const char*)_check_fat_subscript(*n,sizeof(char),0))!= (int)'-';}
# 997
void GC_blacklist_warn_clear();struct _tuple29{struct _fat_ptr f1;int f2;struct _fat_ptr f3;void*f4;struct _fat_ptr f5;};
# 1011
void Cyc_print_options();
# 1019
struct Cyc_Arg_Set_spec_Arg_Spec_struct Cyc_opt1_spec={3U,& Cyc_v_r};static char _tmp171[3U]="-v";static char _tmp172[1U]="";static char _tmp173[35U]="print compilation stages verbosely";struct _tuple29 Cyc_opt1_tuple={{_tmp171,_tmp171,_tmp171 + 3U},0,{_tmp172,_tmp172,_tmp172 + 1U},(void*)& Cyc_opt1_spec,{_tmp173,_tmp173,_tmp173 + 35U}};struct Cyc_List_List Cyc_opt1={(void*)& Cyc_opt1_tuple,0};
# 1021
struct Cyc_Arg_Unit_spec_Arg_Spec_struct Cyc_opt2_spec={0U,Cyc_print_version};static char _tmp174[10U]="--version";static char _tmp175[1U]="";static char _tmp176[35U]="Print version information and exit";struct _tuple29 Cyc_opt2_tuple={{_tmp174,_tmp174,_tmp174 + 10U},0,{_tmp175,_tmp175,_tmp175 + 1U},(void*)& Cyc_opt2_spec,{_tmp176,_tmp176,_tmp176 + 35U}};struct Cyc_List_List Cyc_opt2={(void*)& Cyc_opt2_tuple,0};
# 1024
struct Cyc_Arg_String_spec_Arg_Spec_struct Cyc_opt3_spec={5U,Cyc_set_output_file};static char _tmp177[3U]="-o";static char _tmp178[8U]=" <file>";static char _tmp179[35U]="Set the output file name to <file>";struct _tuple29 Cyc_opt3_tuple={{_tmp177,_tmp177,_tmp177 + 3U},0,{_tmp178,_tmp178,_tmp178 + 8U},(void*)& Cyc_opt3_spec,{_tmp179,_tmp179,_tmp179 + 35U}};struct Cyc_List_List Cyc_opt3={(void*)& Cyc_opt3_tuple,0};
# 1027
struct Cyc_Arg_Flag_spec_Arg_Spec_struct Cyc_opt4_spec={1U,Cyc_add_cpparg};static char _tmp17A[3U]="-D";static char _tmp17B[17U]="<name>[=<value>]";static char _tmp17C[32U]="Pass definition to preprocessor";struct _tuple29 Cyc_opt4_tuple={{_tmp17A,_tmp17A,_tmp17A + 3U},1,{_tmp17B,_tmp17B,_tmp17B + 17U},(void*)& Cyc_opt4_spec,{_tmp17C,_tmp17C,_tmp17C + 32U}};struct Cyc_List_List Cyc_opt4={(void*)& Cyc_opt4_tuple,0};
# 1030
struct Cyc_Arg_Flag_spec_Arg_Spec_struct Cyc_opt5_spec={1U,Cyc_Specsfile_add_cyclone_exec_path};static char _tmp17D[3U]="-B";static char _tmp17E[7U]="<file>";static char _tmp17F[60U]="Add to the list of directories to search for compiler files";struct _tuple29 Cyc_opt5_tuple={{_tmp17D,_tmp17D,_tmp17D + 3U},1,{_tmp17E,_tmp17E,_tmp17E + 7U},(void*)& Cyc_opt5_spec,{_tmp17F,_tmp17F,_tmp17F + 60U}};struct Cyc_List_List Cyc_opt5={(void*)& Cyc_opt5_tuple,0};
# 1033
struct Cyc_Arg_Flag_spec_Arg_Spec_struct Cyc_opt6_spec={1U,Cyc_add_cpparg};static char _tmp180[3U]="-I";static char _tmp181[6U]="<dir>";static char _tmp182[59U]="Add to the list of directories to search for include files";struct _tuple29 Cyc_opt6_tuple={{_tmp180,_tmp180,_tmp180 + 3U},1,{_tmp181,_tmp181,_tmp181 + 6U},(void*)& Cyc_opt6_spec,{_tmp182,_tmp182,_tmp182 + 59U}};struct Cyc_List_List Cyc_opt6={(void*)& Cyc_opt6_tuple,0};
# 1036
struct Cyc_Arg_Flag_spec_Arg_Spec_struct Cyc_opt7_spec={1U,Cyc_add_ccarg};static char _tmp183[3U]="-L";static char _tmp184[6U]="<dir>";static char _tmp185[38U]="Add to the list of directories for -l";struct _tuple29 Cyc_opt7_tuple={{_tmp183,_tmp183,_tmp183 + 3U},1,{_tmp184,_tmp184,_tmp184 + 6U},(void*)& Cyc_opt7_spec,{_tmp185,_tmp185,_tmp185 + 38U}};struct Cyc_List_List Cyc_opt7={(void*)& Cyc_opt7_tuple,0};
# 1039
struct Cyc_Arg_Flag_spec_Arg_Spec_struct Cyc_opt8_spec={1U,Cyc_add_libarg};static char _tmp186[3U]="-l";static char _tmp187[10U]="<libname>";static char _tmp188[13U]="Library file";struct _tuple29 Cyc_opt8_tuple={{_tmp186,_tmp186,_tmp186 + 3U},1,{_tmp187,_tmp187,_tmp187 + 10U},(void*)& Cyc_opt8_spec,{_tmp188,_tmp188,_tmp188 + 13U}};struct Cyc_List_List Cyc_opt8={(void*)& Cyc_opt8_tuple,0};
# 1042
struct Cyc_Arg_Flag_spec_Arg_Spec_struct Cyc_opt9_spec={1U,Cyc_add_marg};static char _tmp189[3U]="-m";static char _tmp18A[9U]="<option>";static char _tmp18B[52U]="GCC specific: pass machine-dependent flag on to gcc";struct _tuple29 Cyc_opt9_tuple={{_tmp189,_tmp189,_tmp189 + 3U},1,{_tmp18A,_tmp18A,_tmp18A + 9U},(void*)& Cyc_opt9_spec,{_tmp18B,_tmp18B,_tmp18B + 52U}};struct Cyc_List_List Cyc_opt9={(void*)& Cyc_opt9_tuple,0};
# 1045
struct Cyc_Arg_Unit_spec_Arg_Spec_struct Cyc_opt10_spec={0U,Cyc_set_stop_after_objectfile};static char _tmp18C[3U]="-c";static char _tmp18D[1U]="";static char _tmp18E[61U]="Produce an object file instead of an executable; do not link";struct _tuple29 Cyc_opt10_tuple={{_tmp18C,_tmp18C,_tmp18C + 3U},0,{_tmp18D,_tmp18D,_tmp18D + 1U},(void*)& Cyc_opt10_spec,{_tmp18E,_tmp18E,_tmp18E + 61U}};struct Cyc_List_List Cyc_opt10={(void*)& Cyc_opt10_tuple,0};
# 1048
struct Cyc_Arg_String_spec_Arg_Spec_struct Cyc_opt11_spec={5U,Cyc_set_inputtype};static char _tmp18F[3U]="-x";static char _tmp190[12U]=" <language>";static char _tmp191[49U]="Specify <language> for the following input files";struct _tuple29 Cyc_opt11_tuple={{_tmp18F,_tmp18F,_tmp18F + 3U},0,{_tmp190,_tmp190,_tmp190 + 12U},(void*)& Cyc_opt11_spec,{_tmp191,_tmp191,_tmp191 + 49U}};struct Cyc_List_List Cyc_opt11={(void*)& Cyc_opt11_tuple,0};
# 1051
struct Cyc_Arg_Unit_spec_Arg_Spec_struct Cyc_opt12_spec={0U,Cyc_set_pa};static char _tmp192[4U]="-pa";static char _tmp193[1U]="";static char _tmp194[33U]="Compile for profiling with aprof";struct _tuple29 Cyc_opt12_tuple={{_tmp192,_tmp192,_tmp192 + 4U},0,{_tmp193,_tmp193,_tmp193 + 1U},(void*)& Cyc_opt12_spec,{_tmp194,_tmp194,_tmp194 + 33U}};struct Cyc_List_List Cyc_opt12={(void*)& Cyc_opt12_tuple,0};
# 1054
struct Cyc_Arg_Unit_spec_Arg_Spec_struct Cyc_opt13_spec={0U,Cyc_set_stop_after_asmfile};static char _tmp195[3U]="-S";static char _tmp196[1U]="";static char _tmp197[35U]="Stop after producing assembly code";struct _tuple29 Cyc_opt13_tuple={{_tmp195,_tmp195,_tmp195 + 3U},0,{_tmp196,_tmp196,_tmp196 + 1U},(void*)& Cyc_opt13_spec,{_tmp197,_tmp197,_tmp197 + 35U}};struct Cyc_List_List Cyc_opt13={(void*)& Cyc_opt13_tuple,0};
# 1057
struct Cyc_Arg_Unit_spec_Arg_Spec_struct Cyc_opt14_spec={0U,Cyc_set_produce_dependencies};static char _tmp198[3U]="-M";static char _tmp199[1U]="";static char _tmp19A[21U]="Produce dependencies";struct _tuple29 Cyc_opt14_tuple={{_tmp198,_tmp198,_tmp198 + 3U},0,{_tmp199,_tmp199,_tmp199 + 1U},(void*)& Cyc_opt14_spec,{_tmp19A,_tmp19A,_tmp19A + 21U}};struct Cyc_List_List Cyc_opt14={(void*)& Cyc_opt14_tuple,0};
# 1060
struct Cyc_Arg_Flag_spec_Arg_Spec_struct Cyc_opt15_spec={1U,Cyc_add_cpparg};static char _tmp19B[4U]="-MG";static char _tmp19C[1U]="";static char _tmp19D[68U]="When producing dependencies assume that missing files are generated";struct _tuple29 Cyc_opt15_tuple={{_tmp19B,_tmp19B,_tmp19B + 4U},0,{_tmp19C,_tmp19C,_tmp19C + 1U},(void*)& Cyc_opt15_spec,{_tmp19D,_tmp19D,_tmp19D + 68U}};struct Cyc_List_List Cyc_opt15={(void*)& Cyc_opt15_tuple,0};
# 1064
struct Cyc_Arg_String_spec_Arg_Spec_struct Cyc_opt16_spec={5U,Cyc_set_dependencies_target};static char _tmp19E[4U]="-MT";static char _tmp19F[10U]=" <target>";static char _tmp1A0[29U]="Give target for dependencies";struct _tuple29 Cyc_opt16_tuple={{_tmp19E,_tmp19E,_tmp19E + 4U},0,{_tmp19F,_tmp19F,_tmp19F + 10U},(void*)& Cyc_opt16_spec,{_tmp1A0,_tmp1A0,_tmp1A0 + 29U}};struct Cyc_List_List Cyc_opt16={(void*)& Cyc_opt16_tuple,0};
# 1067
struct Cyc_Arg_String_spec_Arg_Spec_struct Cyc_opt17_spec={5U,Cyc_Specsfile_set_target_arch};static char _tmp1A1[3U]="-b";static char _tmp1A2[10U]="<machine>";static char _tmp1A3[19U]="Set target machine";struct _tuple29 Cyc_opt17_tuple={{_tmp1A1,_tmp1A1,_tmp1A1 + 3U},0,{_tmp1A2,_tmp1A2,_tmp1A2 + 10U},(void*)& Cyc_opt17_spec,{_tmp1A3,_tmp1A3,_tmp1A3 + 19U}};struct Cyc_List_List Cyc_opt17={(void*)& Cyc_opt17_tuple,0};
# 1070
struct Cyc_Arg_Set_spec_Arg_Spec_struct Cyc_opt18_spec={3U,& Cyc_NewControlFlow_warn_lose_unique};static char _tmp1A4[14U]="-Wlose-unique";static char _tmp1A5[1U]="";static char _tmp1A6[49U]="Try to warn when a unique pointer might get lost";struct _tuple29 Cyc_opt18_tuple={{_tmp1A4,_tmp1A4,_tmp1A4 + 14U},0,{_tmp1A5,_tmp1A5,_tmp1A5 + 1U},(void*)& Cyc_opt18_spec,{_tmp1A6,_tmp1A6,_tmp1A6 + 49U}};struct Cyc_List_List Cyc_opt18={(void*)& Cyc_opt18_tuple,0};
# 1073
struct Cyc_Arg_Set_spec_Arg_Spec_struct Cyc_opt19_spec={3U,& Cyc_Binding_warn_override};static char _tmp1A7[11U]="-Woverride";static char _tmp1A8[1U]="";static char _tmp1A9[58U]="Warn when a local variable overrides an existing variable";struct _tuple29 Cyc_opt19_tuple={{_tmp1A7,_tmp1A7,_tmp1A7 + 11U},0,{_tmp1A8,_tmp1A8,_tmp1A8 + 1U},(void*)& Cyc_opt19_spec,{_tmp1A9,_tmp1A9,_tmp1A9 + 58U}};struct Cyc_List_List Cyc_opt19={(void*)& Cyc_opt19_tuple,0};
# 1076
struct Cyc_Arg_Unit_spec_Arg_Spec_struct Cyc_opt20_spec={0U,Cyc_set_all_warnings};static char _tmp1AA[6U]="-Wall";static char _tmp1AB[1U]="";static char _tmp1AC[21U]="Turn on all warnings";struct _tuple29 Cyc_opt20_tuple={{_tmp1AA,_tmp1AA,_tmp1AA + 6U},0,{_tmp1AB,_tmp1AB,_tmp1AB + 1U},(void*)& Cyc_opt20_spec,{_tmp1AC,_tmp1AC,_tmp1AC + 21U}};struct Cyc_List_List Cyc_opt20={(void*)& Cyc_opt20_tuple,0};
# 1079
struct Cyc_Arg_Set_spec_Arg_Spec_struct Cyc_opt21_spec={3U,& Cyc_stop_after_cpp_r};static char _tmp1AD[3U]="-E";static char _tmp1AE[1U]="";static char _tmp1AF[25U]="Stop after preprocessing";struct _tuple29 Cyc_opt21_tuple={{_tmp1AD,_tmp1AD,_tmp1AD + 3U},0,{_tmp1AE,_tmp1AE,_tmp1AE + 1U},(void*)& Cyc_opt21_spec,{_tmp1AF,_tmp1AF,_tmp1AF + 25U}};struct Cyc_List_List Cyc_opt21={(void*)& Cyc_opt21_tuple,0};
# 1082
struct Cyc_Arg_Set_spec_Arg_Spec_struct Cyc_opt22_spec={3U,& Cyc_parseonly_r};static char _tmp1B0[17U]="-stopafter-parse";static char _tmp1B1[1U]="";static char _tmp1B2[19U]="Stop after parsing";struct _tuple29 Cyc_opt22_tuple={{_tmp1B0,_tmp1B0,_tmp1B0 + 17U},0,{_tmp1B1,_tmp1B1,_tmp1B1 + 1U},(void*)& Cyc_opt22_spec,{_tmp1B2,_tmp1B2,_tmp1B2 + 19U}};struct Cyc_List_List Cyc_opt22={(void*)& Cyc_opt22_tuple,0};
# 1085
struct Cyc_Arg_Set_spec_Arg_Spec_struct Cyc_opt23_spec={3U,& Cyc_tc_r};static char _tmp1B3[14U]="-stopafter-tc";static char _tmp1B4[1U]="";static char _tmp1B5[25U]="Stop after type checking";struct _tuple29 Cyc_opt23_tuple={{_tmp1B3,_tmp1B3,_tmp1B3 + 14U},0,{_tmp1B4,_tmp1B4,_tmp1B4 + 1U},(void*)& Cyc_opt23_spec,{_tmp1B5,_tmp1B5,_tmp1B5 + 25U}};struct Cyc_List_List Cyc_opt23={(void*)& Cyc_opt23_tuple,0};
# 1088
struct Cyc_Arg_Set_spec_Arg_Spec_struct Cyc_opt24_spec={3U,& Cyc_noprint_r};static char _tmp1B6[9U]="-noprint";static char _tmp1B7[1U]="";static char _tmp1B8[42U]="Do not print output (when stopping early)";struct _tuple29 Cyc_opt24_tuple={{_tmp1B6,_tmp1B6,_tmp1B6 + 9U},0,{_tmp1B7,_tmp1B7,_tmp1B7 + 1U},(void*)& Cyc_opt24_spec,{_tmp1B8,_tmp1B8,_tmp1B8 + 42U}};struct Cyc_List_List Cyc_opt24={(void*)& Cyc_opt24_tuple,0};
# 1091
struct Cyc_Arg_Set_spec_Arg_Spec_struct Cyc_opt25_spec={3U,& Cyc_cf_r};static char _tmp1B9[14U]="-stopafter-cf";static char _tmp1BA[1U]="";static char _tmp1BB[33U]="Stop after control-flow checking";struct _tuple29 Cyc_opt25_tuple={{_tmp1B9,_tmp1B9,_tmp1B9 + 14U},0,{_tmp1BA,_tmp1BA,_tmp1BA + 1U},(void*)& Cyc_opt25_spec,{_tmp1BB,_tmp1BB,_tmp1BB + 33U}};struct Cyc_List_List Cyc_opt25={(void*)& Cyc_opt25_tuple,0};
# 1094
struct Cyc_Arg_Set_spec_Arg_Spec_struct Cyc_opt26_spec={3U,& Cyc_toc_r};static char _tmp1BC[15U]="-stopafter-toc";static char _tmp1BD[1U]="";static char _tmp1BE[28U]="Stop after translation to C";struct _tuple29 Cyc_opt26_tuple={{_tmp1BC,_tmp1BC,_tmp1BC + 15U},0,{_tmp1BD,_tmp1BD,_tmp1BD + 1U},(void*)& Cyc_opt26_spec,{_tmp1BE,_tmp1BE,_tmp1BE + 28U}};struct Cyc_List_List Cyc_opt26={(void*)& Cyc_opt26_tuple,0};
# 1097
struct Cyc_Arg_Set_spec_Arg_Spec_struct Cyc_opt27_spec={3U,& Cyc_ic_r};static char _tmp1BF[4U]="-ic";static char _tmp1C0[1U]="";static char _tmp1C1[26U]="Activate the link-checker";struct _tuple29 Cyc_opt27_tuple={{_tmp1BF,_tmp1BF,_tmp1BF + 4U},0,{_tmp1C0,_tmp1C0,_tmp1C0 + 1U},(void*)& Cyc_opt27_spec,{_tmp1C1,_tmp1C1,_tmp1C1 + 26U}};struct Cyc_List_List Cyc_opt27={(void*)& Cyc_opt27_tuple,0};
# 1100
struct Cyc_Arg_Set_spec_Arg_Spec_struct Cyc_opt28_spec={3U,& Cyc_pp_r};static char _tmp1C2[4U]="-pp";static char _tmp1C3[1U]="";static char _tmp1C4[47U]="Pretty print the C code that Cyclone generates";struct _tuple29 Cyc_opt28_tuple={{_tmp1C2,_tmp1C2,_tmp1C2 + 4U},0,{_tmp1C3,_tmp1C3,_tmp1C3 + 1U},(void*)& Cyc_opt28_spec,{_tmp1C4,_tmp1C4,_tmp1C4 + 47U}};struct Cyc_List_List Cyc_opt28={(void*)& Cyc_opt28_tuple,0};
# 1103
struct Cyc_Arg_Clear_spec_Arg_Spec_struct Cyc_opt29_spec={4U,& Cyc_pp_r};static char _tmp1C5[4U]="-up";static char _tmp1C6[1U]="";static char _tmp1C7[55U]="Ugly print the C code that Cyclone generates (default)";struct _tuple29 Cyc_opt29_tuple={{_tmp1C5,_tmp1C5,_tmp1C5 + 4U},0,{_tmp1C6,_tmp1C6,_tmp1C6 + 1U},(void*)& Cyc_opt29_spec,{_tmp1C7,_tmp1C7,_tmp1C7 + 55U}};struct Cyc_List_List Cyc_opt29={(void*)& Cyc_opt29_tuple,0};
# 1106
struct Cyc_Arg_Set_spec_Arg_Spec_struct Cyc_opt30_spec={3U,& Cyc_elim_se_r};static char _tmp1C8[28U]="-elim-statement-expressions";static char _tmp1C9[1U]="";static char _tmp1CA[66U]="Avoid statement expressions in C output (implies --inline-checks)";struct _tuple29 Cyc_opt30_tuple={{_tmp1C8,_tmp1C8,_tmp1C8 + 28U},0,{_tmp1C9,_tmp1C9,_tmp1C9 + 1U},(void*)& Cyc_opt30_spec,{_tmp1CA,_tmp1CA,_tmp1CA + 66U}};struct Cyc_List_List Cyc_opt30={(void*)& Cyc_opt30_tuple,0};
# 1110
struct Cyc_Arg_Clear_spec_Arg_Spec_struct Cyc_opt31_spec={4U,& Cyc_elim_se_r};static char _tmp1CB[31U]="-no-elim-statement-expressions";static char _tmp1CC[1U]="";static char _tmp1CD[47U]="Do not avoid statement expressions in C output";struct _tuple29 Cyc_opt31_tuple={{_tmp1CB,_tmp1CB,_tmp1CB + 31U},0,{_tmp1CC,_tmp1CC,_tmp1CC + 1U},(void*)& Cyc_opt31_spec,{_tmp1CD,_tmp1CD,_tmp1CD + 47U}};struct Cyc_List_List Cyc_opt31={(void*)& Cyc_opt31_tuple,0};
# 1113
struct Cyc_Arg_Unit_spec_Arg_Spec_struct Cyc_opt32_spec={0U,Cyc_set_tovc};static char _tmp1CE[8U]="-un-gcc";static char _tmp1CF[1U]="";static char _tmp1D0[57U]="Avoid gcc extensions in C output (except for attributes)";struct _tuple29 Cyc_opt32_tuple={{_tmp1CE,_tmp1CE,_tmp1CE + 8U},0,{_tmp1CF,_tmp1CF,_tmp1CF + 1U},(void*)& Cyc_opt32_spec,{_tmp1D0,_tmp1D0,_tmp1D0 + 57U}};struct Cyc_List_List Cyc_opt32={(void*)& Cyc_opt32_tuple,0};
# 1116
struct Cyc_Arg_String_spec_Arg_Spec_struct Cyc_opt33_spec={5U,Cyc_set_c_compiler};static char _tmp1D1[8U]="-c-comp";static char _tmp1D2[12U]=" <compiler>";static char _tmp1D3[40U]="Produce C output for the given compiler";struct _tuple29 Cyc_opt33_tuple={{_tmp1D1,_tmp1D1,_tmp1D1 + 8U},0,{_tmp1D2,_tmp1D2,_tmp1D2 + 12U},(void*)& Cyc_opt33_spec,{_tmp1D3,_tmp1D3,_tmp1D3 + 40U}};struct Cyc_List_List Cyc_opt33={(void*)& Cyc_opt33_tuple,0};
# 1119
struct Cyc_Arg_Unit_spec_Arg_Spec_struct Cyc_opt34_spec={0U,Cyc_set_save_temps};static char _tmp1D4[12U]="-save-temps";static char _tmp1D5[1U]="";static char _tmp1D6[29U]="Don't delete temporary files";struct _tuple29 Cyc_opt34_tuple={{_tmp1D4,_tmp1D4,_tmp1D4 + 12U},0,{_tmp1D5,_tmp1D5,_tmp1D5 + 1U},(void*)& Cyc_opt34_spec,{_tmp1D6,_tmp1D6,_tmp1D6 + 29U}};struct Cyc_List_List Cyc_opt34={(void*)& Cyc_opt34_tuple,0};
# 1122
struct Cyc_Arg_Set_spec_Arg_Spec_struct Cyc_opt35_spec={3U,& Cyc_save_c_r};static char _tmp1D7[8U]="-save-c";static char _tmp1D8[1U]="";static char _tmp1D9[31U]="Don't delete temporary C files";struct _tuple29 Cyc_opt35_tuple={{_tmp1D7,_tmp1D7,_tmp1D7 + 8U},0,{_tmp1D8,_tmp1D8,_tmp1D8 + 1U},(void*)& Cyc_opt35_spec,{_tmp1D9,_tmp1D9,_tmp1D9 + 31U}};struct Cyc_List_List Cyc_opt35={(void*)& Cyc_opt35_tuple,0};
# 1125
struct Cyc_Arg_Unit_spec_Arg_Spec_struct Cyc_opt36_spec={0U,Cyc_clear_lineno};static char _tmp1DA[11U]="--nolineno";static char _tmp1DB[1U]="";static char _tmp1DC[63U]="Don't generate line numbers for debugging (automatic with -pp)";struct _tuple29 Cyc_opt36_tuple={{_tmp1DA,_tmp1DA,_tmp1DA + 11U},0,{_tmp1DB,_tmp1DB,_tmp1DB + 1U},(void*)& Cyc_opt36_spec,{_tmp1DC,_tmp1DC,_tmp1DC + 63U}};struct Cyc_List_List Cyc_opt36={(void*)& Cyc_opt36_tuple,0};
# 1128
struct Cyc_Arg_Unit_spec_Arg_Spec_struct Cyc_opt37_spec={0U,Cyc_set_nochecks};static char _tmp1DD[11U]="--nochecks";static char _tmp1DE[1U]="";static char _tmp1DF[27U]="Disable bounds/null checks";struct _tuple29 Cyc_opt37_tuple={{_tmp1DD,_tmp1DD,_tmp1DD + 11U},0,{_tmp1DE,_tmp1DE,_tmp1DE + 1U},(void*)& Cyc_opt37_spec,{_tmp1DF,_tmp1DF,_tmp1DF + 27U}};struct Cyc_List_List Cyc_opt37={(void*)& Cyc_opt37_tuple,0};
# 1131
struct Cyc_Arg_Unit_spec_Arg_Spec_struct Cyc_opt38_spec={0U,Cyc_set_nonullchecks};static char _tmp1E0[15U]="--nonullchecks";static char _tmp1E1[1U]="";static char _tmp1E2[20U]="Disable null checks";struct _tuple29 Cyc_opt38_tuple={{_tmp1E0,_tmp1E0,_tmp1E0 + 15U},0,{_tmp1E1,_tmp1E1,_tmp1E1 + 1U},(void*)& Cyc_opt38_spec,{_tmp1E2,_tmp1E2,_tmp1E2 + 20U}};struct Cyc_List_List Cyc_opt38={(void*)& Cyc_opt38_tuple,0};
# 1134
struct Cyc_Arg_Unit_spec_Arg_Spec_struct Cyc_opt39_spec={0U,Cyc_set_noboundschecks};static char _tmp1E3[17U]="--noboundschecks";static char _tmp1E4[1U]="";static char _tmp1E5[22U]="Disable bounds checks";struct _tuple29 Cyc_opt39_tuple={{_tmp1E3,_tmp1E3,_tmp1E3 + 17U},0,{_tmp1E4,_tmp1E4,_tmp1E4 + 1U},(void*)& Cyc_opt39_spec,{_tmp1E5,_tmp1E5,_tmp1E5 + 22U}};struct Cyc_List_List Cyc_opt39={(void*)& Cyc_opt39_tuple,0};
# 1137
struct Cyc_Arg_Unit_spec_Arg_Spec_struct Cyc_opt40_spec={0U,Cyc_set_inline_functions};static char _tmp1E6[16U]="--inline-checks";static char _tmp1E7[1U]="";static char _tmp1E8[40U]="Inline bounds checks instead of #define";struct _tuple29 Cyc_opt40_tuple={{_tmp1E6,_tmp1E6,_tmp1E6 + 16U},0,{_tmp1E7,_tmp1E7,_tmp1E7 + 1U},(void*)& Cyc_opt40_spec,{_tmp1E8,_tmp1E8,_tmp1E8 + 40U}};struct Cyc_List_List Cyc_opt40={(void*)& Cyc_opt40_tuple,0};
# 1140
struct Cyc_Arg_String_spec_Arg_Spec_struct Cyc_opt41_spec={5U,Cyc_set_cpp};static char _tmp1E9[9U]="-use-cpp";static char _tmp1EA[7U]="<path>";static char _tmp1EB[35U]="Indicate which preprocessor to use";struct _tuple29 Cyc_opt41_tuple={{_tmp1E9,_tmp1E9,_tmp1E9 + 9U},0,{_tmp1EA,_tmp1EA,_tmp1EA + 7U},(void*)& Cyc_opt41_spec,{_tmp1EB,_tmp1EB,_tmp1EB + 35U}};struct Cyc_List_List Cyc_opt41={(void*)& Cyc_opt41_tuple,0};
# 1143
struct Cyc_Arg_Unit_spec_Arg_Spec_struct Cyc_opt42_spec={0U,Cyc_set_nocppprecomp};static char _tmp1EC[16U]="-no-cpp-precomp";static char _tmp1ED[1U]="";static char _tmp1EE[40U]="Don't do smart preprocessing (mac only)";struct _tuple29 Cyc_opt42_tuple={{_tmp1EC,_tmp1EC,_tmp1EC + 16U},0,{_tmp1ED,_tmp1ED,_tmp1ED + 1U},(void*)& Cyc_opt42_spec,{_tmp1EE,_tmp1EE,_tmp1EE + 40U}};struct Cyc_List_List Cyc_opt42={(void*)& Cyc_opt42_tuple,0};
# 1146
struct Cyc_Arg_Unit_spec_Arg_Spec_struct Cyc_opt43_spec={0U,Cyc_set_nocyc};static char _tmp1EF[7U]="-nocyc";static char _tmp1F0[1U]="";static char _tmp1F1[33U]="Don't add implicit namespace Cyc";struct _tuple29 Cyc_opt43_tuple={{_tmp1EF,_tmp1EF,_tmp1EF + 7U},0,{_tmp1F0,_tmp1F0,_tmp1F0 + 1U},(void*)& Cyc_opt43_spec,{_tmp1F1,_tmp1F1,_tmp1F1 + 33U}};struct Cyc_List_List Cyc_opt43={(void*)& Cyc_opt43_tuple,0};
# 1149
struct Cyc_Arg_Set_spec_Arg_Spec_struct Cyc_opt44_spec={3U,& Cyc_nogc_r};static char _tmp1F2[6U]="-nogc";static char _tmp1F3[1U]="";static char _tmp1F4[36U]="Don't link in the garbage collector";struct _tuple29 Cyc_opt44_tuple={{_tmp1F2,_tmp1F2,_tmp1F2 + 6U},0,{_tmp1F3,_tmp1F3,_tmp1F3 + 1U},(void*)& Cyc_opt44_spec,{_tmp1F4,_tmp1F4,_tmp1F4 + 36U}};struct Cyc_List_List Cyc_opt44={(void*)& Cyc_opt44_tuple,0};
# 1152
struct Cyc_Arg_Set_spec_Arg_Spec_struct Cyc_opt45_spec={3U,& Cyc_noshake_r};static char _tmp1F5[16U]="-noremoveunused";static char _tmp1F6[1U]="";static char _tmp1F7[49U]="Don't remove externed variables that aren't used";struct _tuple29 Cyc_opt45_tuple={{_tmp1F5,_tmp1F5,_tmp1F5 + 16U},0,{_tmp1F6,_tmp1F6,_tmp1F6 + 1U},(void*)& Cyc_opt45_spec,{_tmp1F7,_tmp1F7,_tmp1F7 + 49U}};struct Cyc_List_List Cyc_opt45={(void*)& Cyc_opt45_tuple,0};
# 1155
struct Cyc_Arg_Set_spec_Arg_Spec_struct Cyc_opt46_spec={3U,& Cyc_noexpand_r};static char _tmp1F8[18U]="-noexpandtypedefs";static char _tmp1F9[1U]="";static char _tmp1FA[41U]="Don't expand typedefs in pretty printing";struct _tuple29 Cyc_opt46_tuple={{_tmp1F8,_tmp1F8,_tmp1F8 + 18U},0,{_tmp1F9,_tmp1F9,_tmp1F9 + 1U},(void*)& Cyc_opt46_spec,{_tmp1FA,_tmp1FA,_tmp1FA + 41U}};struct Cyc_List_List Cyc_opt46={(void*)& Cyc_opt46_tuple,0};
# 1158
struct Cyc_Arg_Set_spec_Arg_Spec_struct Cyc_opt47_spec={3U,& Cyc_print_all_tvars_r};static char _tmp1FB[15U]="-printalltvars";static char _tmp1FC[1U]="";static char _tmp1FD[57U]="Print all type variables (even implicit default effects)";struct _tuple29 Cyc_opt47_tuple={{_tmp1FB,_tmp1FB,_tmp1FB + 15U},0,{_tmp1FC,_tmp1FC,_tmp1FC + 1U},(void*)& Cyc_opt47_spec,{_tmp1FD,_tmp1FD,_tmp1FD + 57U}};struct Cyc_List_List Cyc_opt47={(void*)& Cyc_opt47_tuple,0};
# 1161
struct Cyc_Arg_Set_spec_Arg_Spec_struct Cyc_opt48_spec={3U,& Cyc_print_all_kinds_r};static char _tmp1FE[15U]="-printallkinds";static char _tmp1FF[1U]="";static char _tmp200[37U]="Always print kinds of type variables";struct _tuple29 Cyc_opt48_tuple={{_tmp1FE,_tmp1FE,_tmp1FE + 15U},0,{_tmp1FF,_tmp1FF,_tmp1FF + 1U},(void*)& Cyc_opt48_spec,{_tmp200,_tmp200,_tmp200 + 37U}};struct Cyc_List_List Cyc_opt48={(void*)& Cyc_opt48_tuple,0};
# 1164
struct Cyc_Arg_Set_spec_Arg_Spec_struct Cyc_opt49_spec={3U,& Cyc_print_full_evars_r};static char _tmp201[16U]="-printfullevars";static char _tmp202[1U]="";static char _tmp203[50U]="Print full information for evars (type debugging)";struct _tuple29 Cyc_opt49_tuple={{_tmp201,_tmp201,_tmp201 + 16U},0,{_tmp202,_tmp202,_tmp202 + 1U},(void*)& Cyc_opt49_spec,{_tmp203,_tmp203,_tmp203 + 50U}};struct Cyc_List_List Cyc_opt49={(void*)& Cyc_opt49_tuple,0};
# 1167
struct Cyc_Arg_Set_spec_Arg_Spec_struct Cyc_opt50_spec={3U,& Cyc_print_all_effects_r};static char _tmp204[17U]="-printalleffects";static char _tmp205[1U]="";static char _tmp206[45U]="Print effects for functions (type debugging)";struct _tuple29 Cyc_opt50_tuple={{_tmp204,_tmp204,_tmp204 + 17U},0,{_tmp205,_tmp205,_tmp205 + 1U},(void*)& Cyc_opt50_spec,{_tmp206,_tmp206,_tmp206 + 45U}};struct Cyc_List_List Cyc_opt50={(void*)& Cyc_opt50_tuple,0};
# 1170
struct Cyc_Arg_Set_spec_Arg_Spec_struct Cyc_opt51_spec={3U,& Cyc_nocyc_setjmp_r};static char _tmp207[14U]="-nocyc_setjmp";static char _tmp208[1U]="";static char _tmp209[46U]="Do not use compiler special file cyc_setjmp.h";struct _tuple29 Cyc_opt51_tuple={{_tmp207,_tmp207,_tmp207 + 14U},0,{_tmp208,_tmp208,_tmp208 + 1U},(void*)& Cyc_opt51_spec,{_tmp209,_tmp209,_tmp209 + 46U}};struct Cyc_List_List Cyc_opt51={(void*)& Cyc_opt51_tuple,0};
# 1173
struct Cyc_Arg_Set_spec_Arg_Spec_struct Cyc_opt52_spec={3U,& Cyc_Lex_compile_for_boot_r};static char _tmp20A[18U]="-compile-for-boot";static char _tmp20B[1U]="";static char _tmp20C[71U]="Compile using the special boot library instead of the standard library";struct _tuple29 Cyc_opt52_tuple={{_tmp20A,_tmp20A,_tmp20A + 18U},0,{_tmp20B,_tmp20B,_tmp20B + 1U},(void*)& Cyc_opt52_spec,{_tmp20C,_tmp20C,_tmp20C + 71U}};struct Cyc_List_List Cyc_opt52={(void*)& Cyc_opt52_tuple,0};
# 1176
struct Cyc_Arg_String_spec_Arg_Spec_struct Cyc_opt53_spec={5U,Cyc_set_cyc_include};static char _tmp20D[4U]="-CI";static char _tmp20E[8U]=" <file>";static char _tmp20F[31U]="Set cyc_include.h to be <file>";struct _tuple29 Cyc_opt53_tuple={{_tmp20D,_tmp20D,_tmp20D + 4U},0,{_tmp20E,_tmp20E,_tmp20E + 8U},(void*)& Cyc_opt53_spec,{_tmp20F,_tmp20F,_tmp20F + 31U}};struct Cyc_List_List Cyc_opt53={(void*)& Cyc_opt53_tuple,0};
# 1179
struct Cyc_Arg_Set_spec_Arg_Spec_struct Cyc_opt54_spec={3U,& Cyc_InsertChecks_warn_bounds_checks};static char _tmp210[18U]="-warnboundschecks";static char _tmp211[1U]="";static char _tmp212[44U]="Warn when bounds checks can't be eliminated";struct _tuple29 Cyc_opt54_tuple={{_tmp210,_tmp210,_tmp210 + 18U},0,{_tmp211,_tmp211,_tmp211 + 1U},(void*)& Cyc_opt54_spec,{_tmp212,_tmp212,_tmp212 + 44U}};struct Cyc_List_List Cyc_opt54={(void*)& Cyc_opt54_tuple,0};
# 1182
struct Cyc_Arg_Set_spec_Arg_Spec_struct Cyc_opt55_spec={3U,& Cyc_InsertChecks_warn_all_null_deref};static char _tmp213[16U]="-warnnullchecks";static char _tmp214[1U]="";static char _tmp215[45U]="Warn when any null check can't be eliminated";struct _tuple29 Cyc_opt55_tuple={{_tmp213,_tmp213,_tmp213 + 16U},0,{_tmp214,_tmp214,_tmp214 + 1U},(void*)& Cyc_opt55_spec,{_tmp215,_tmp215,_tmp215 + 45U}};struct Cyc_List_List Cyc_opt55={(void*)& Cyc_opt55_tuple,0};
# 1185
struct Cyc_Arg_Set_spec_Arg_Spec_struct Cyc_opt56_spec={3U,& Cyc_Tcutil_warn_alias_coerce};static char _tmp216[19U]="-warnaliascoercion";static char _tmp217[1U]="";static char _tmp218[41U]="Warn when any alias coercion is inserted";struct _tuple29 Cyc_opt56_tuple={{_tmp216,_tmp216,_tmp216 + 19U},0,{_tmp217,_tmp217,_tmp217 + 1U},(void*)& Cyc_opt56_spec,{_tmp218,_tmp218,_tmp218 + 41U}};struct Cyc_List_List Cyc_opt56={(void*)& Cyc_opt56_tuple,0};
# 1188
struct Cyc_Arg_Set_spec_Arg_Spec_struct Cyc_opt57_spec={3U,& Cyc_Tcutil_warn_region_coerce};static char _tmp219[18U]="-warnregioncoerce";static char _tmp21A[1U]="";static char _tmp21B[42U]="Warn when any region coercion is inserted";struct _tuple29 Cyc_opt57_tuple={{_tmp219,_tmp219,_tmp219 + 18U},0,{_tmp21A,_tmp21A,_tmp21A + 1U},(void*)& Cyc_opt57_spec,{_tmp21B,_tmp21B,_tmp21B + 42U}};struct Cyc_List_List Cyc_opt57={(void*)& Cyc_opt57_tuple,0};
# 1191
struct Cyc_Arg_Set_spec_Arg_Spec_struct Cyc_opt58_spec={3U,& Cyc_Parse_no_register};static char _tmp21C[12U]="-noregister";static char _tmp21D[1U]="";static char _tmp21E[39U]="Treat register storage class as public";struct _tuple29 Cyc_opt58_tuple={{_tmp21C,_tmp21C,_tmp21C + 12U},0,{_tmp21D,_tmp21D,_tmp21D + 1U},(void*)& Cyc_opt58_spec,{_tmp21E,_tmp21E,_tmp21E + 39U}};struct Cyc_List_List Cyc_opt58={(void*)& Cyc_opt58_tuple,0};
# 1194
struct Cyc_Arg_Clear_spec_Arg_Spec_struct Cyc_opt59_spec={4U,& Cyc_Position_use_gcc_style_location};static char _tmp21F[18U]="-detailedlocation";static char _tmp220[1U]="";static char _tmp221[58U]="Try to give more detailed location information for errors";struct _tuple29 Cyc_opt59_tuple={{_tmp21F,_tmp21F,_tmp21F + 18U},0,{_tmp220,_tmp220,_tmp220 + 1U},(void*)& Cyc_opt59_spec,{_tmp221,_tmp221,_tmp221 + 58U}};struct Cyc_List_List Cyc_opt59={(void*)& Cyc_opt59_tuple,0};
# 1197
struct Cyc_Arg_Unit_spec_Arg_Spec_struct Cyc_opt60_spec={0U,Cyc_set_port_c_code};static char _tmp222[6U]="-port";static char _tmp223[1U]="";static char _tmp224[38U]="Suggest how to port C code to Cyclone";struct _tuple29 Cyc_opt60_tuple={{_tmp222,_tmp222,_tmp222 + 6U},0,{_tmp223,_tmp223,_tmp223 + 1U},(void*)& Cyc_opt60_spec,{_tmp224,_tmp224,_tmp224 + 38U}};struct Cyc_List_List Cyc_opt60={(void*)& Cyc_opt60_tuple,0};
# 1200
struct Cyc_Arg_Set_spec_Arg_Spec_struct Cyc_opt61_spec={3U,& Cyc_Absyn_no_regions};static char _tmp225[11U]="-noregions";static char _tmp226[1U]="";static char _tmp227[39U]="Generate code that doesn't use regions";struct _tuple29 Cyc_opt61_tuple={{_tmp225,_tmp225,_tmp225 + 11U},0,{_tmp226,_tmp226,_tmp226 + 1U},(void*)& Cyc_opt61_spec,{_tmp227,_tmp227,_tmp227 + 39U}};struct Cyc_List_List Cyc_opt61={(void*)& Cyc_opt61_tuple,0};
# 1203
struct Cyc_Arg_String_spec_Arg_Spec_struct Cyc_opt62_spec={5U,Cyc_add_iprefix};static char _tmp228[9U]="-iprefix";static char _tmp229[9U]="<prefix>";static char _tmp22A[67U]="Specify <prefix> as the prefix for subsequent -iwithprefix options";struct _tuple29 Cyc_opt62_tuple={{_tmp228,_tmp228,_tmp228 + 9U},0,{_tmp229,_tmp229,_tmp229 + 9U},(void*)& Cyc_opt62_spec,{_tmp22A,_tmp22A,_tmp22A + 67U}};struct Cyc_List_List Cyc_opt62={(void*)& Cyc_opt62_tuple,0};
# 1206
struct Cyc_Arg_String_spec_Arg_Spec_struct Cyc_opt63_spec={5U,Cyc_add_iwithprefix};static char _tmp22B[13U]="-iwithprefix";static char _tmp22C[6U]="<dir>";static char _tmp22D[47U]="Add <prefix>/<dir> to the second include path.";struct _tuple29 Cyc_opt63_tuple={{_tmp22B,_tmp22B,_tmp22B + 13U},0,{_tmp22C,_tmp22C,_tmp22C + 6U},(void*)& Cyc_opt63_spec,{_tmp22D,_tmp22D,_tmp22D + 47U}};struct Cyc_List_List Cyc_opt63={(void*)& Cyc_opt63_tuple,0};
# 1209
struct Cyc_Arg_String_spec_Arg_Spec_struct Cyc_opt64_spec={5U,Cyc_add_iwithprefixbefore};static char _tmp22E[19U]="-iwithprefixbefore";static char _tmp22F[6U]="<dir>";static char _tmp230[45U]="Add <prefix>/<dir> to the main include path.";struct _tuple29 Cyc_opt64_tuple={{_tmp22E,_tmp22E,_tmp22E + 19U},0,{_tmp22F,_tmp22F,_tmp22F + 6U},(void*)& Cyc_opt64_spec,{_tmp230,_tmp230,_tmp230 + 45U}};struct Cyc_List_List Cyc_opt64={(void*)& Cyc_opt64_tuple,0};
# 1212
struct Cyc_Arg_String_spec_Arg_Spec_struct Cyc_opt65_spec={5U,Cyc_add_isystem};static char _tmp231[9U]="-isystem";static char _tmp232[6U]="<dir>";static char _tmp233[90U]="Add <dir> to the beginning of the second include path and treat it as a\nsystem directory.";struct _tuple29 Cyc_opt65_tuple={{_tmp231,_tmp231,_tmp231 + 9U},0,{_tmp232,_tmp232,_tmp232 + 6U},(void*)& Cyc_opt65_spec,{_tmp233,_tmp233,_tmp233 + 90U}};struct Cyc_List_List Cyc_opt65={(void*)& Cyc_opt65_tuple,0};
# 1215
struct Cyc_Arg_String_spec_Arg_Spec_struct Cyc_opt66_spec={5U,Cyc_add_idirafter};static char _tmp234[11U]="-idirafter";static char _tmp235[6U]="<dir>";static char _tmp236[46U]="Add the directory to the second include path.";struct _tuple29 Cyc_opt66_tuple={{_tmp234,_tmp234,_tmp234 + 11U},0,{_tmp235,_tmp235,_tmp235 + 6U},(void*)& Cyc_opt66_spec,{_tmp236,_tmp236,_tmp236 + 46U}};struct Cyc_List_List Cyc_opt66={(void*)& Cyc_opt66_tuple,0};
# 1218
struct Cyc_Arg_Set_spec_Arg_Spec_struct Cyc_opt67_spec={3U,& Cyc_generate_interface_r};static char _tmp237[15U]="--geninterface";static char _tmp238[1U]="";static char _tmp239[25U]="Generate interface files";struct _tuple29 Cyc_opt67_tuple={{_tmp237,_tmp237,_tmp237 + 15U},0,{_tmp238,_tmp238,_tmp238 + 1U},(void*)& Cyc_opt67_spec,{_tmp239,_tmp239,_tmp239 + 25U}};struct Cyc_List_List Cyc_opt67={(void*)& Cyc_opt67_tuple,0};
# 1221
struct Cyc_Arg_String_spec_Arg_Spec_struct Cyc_opt68_spec={5U,Cyc_set_specified_interface};static char _tmp23A[12U]="--interface";static char _tmp23B[8U]=" <file>";static char _tmp23C[37U]="Set the interface file to be <file>.";struct _tuple29 Cyc_opt68_tuple={{_tmp23A,_tmp23A,_tmp23A + 12U},0,{_tmp23B,_tmp23B,_tmp23B + 8U},(void*)& Cyc_opt68_spec,{_tmp23C,_tmp23C,_tmp23C + 37U}};struct Cyc_List_List Cyc_opt68={(void*)& Cyc_opt68_tuple,0};
# 1224
struct Cyc_Arg_Unit_spec_Arg_Spec_struct Cyc_opt69_spec={0U,Cyc_set_many_errors};static char _tmp23D[13U]="--manyerrors";static char _tmp23E[1U]="";static char _tmp23F[36U]="don't stop after only a few errors.";struct _tuple29 Cyc_opt69_tuple={{_tmp23D,_tmp23D,_tmp23D + 13U},0,{_tmp23E,_tmp23E,_tmp23E + 1U},(void*)& Cyc_opt69_spec,{_tmp23F,_tmp23F,_tmp23F + 36U}};struct Cyc_List_List Cyc_opt69={(void*)& Cyc_opt69_tuple,0};
# 1227
struct Cyc_Arg_Set_spec_Arg_Spec_struct Cyc_opt70_spec={3U,& Cyc_ParseErrors_print_state_and_token};static char _tmp240[13U]="--parsestate";static char _tmp241[1U]="";static char _tmp242[50U]="print the parse state and token on a syntax error";struct _tuple29 Cyc_opt70_tuple={{_tmp240,_tmp240,_tmp240 + 13U},0,{_tmp241,_tmp241,_tmp241 + 1U},(void*)& Cyc_opt70_spec,{_tmp242,_tmp242,_tmp242 + 50U}};struct Cyc_List_List Cyc_opt70={(void*)& Cyc_opt70_tuple,0};
# 1230
struct Cyc_Arg_Unit_spec_Arg_Spec_struct Cyc_opt71_spec={0U,Cyc_noassume_gcc_flag};static char _tmp243[22U]="-known-gcc-flags-only";static char _tmp244[1U]="";static char _tmp245[57U]="do not assume that unknown flags should be passed to gcc";struct _tuple29 Cyc_opt71_tuple={{_tmp243,_tmp243,_tmp243 + 22U},0,{_tmp244,_tmp244,_tmp244 + 1U},(void*)& Cyc_opt71_spec,{_tmp245,_tmp245,_tmp245 + 57U}};struct Cyc_List_List Cyc_opt71={(void*)& Cyc_opt71_tuple,0};
# 1233
struct Cyc_Arg_Unit_spec_Arg_Spec_struct Cyc_opt72_spec={0U,Cyc_print_options};static char _tmp246[6U]="-help";static char _tmp247[1U]="";static char _tmp248[32U]="print out the available options";struct _tuple29 Cyc_opt72_tuple={{_tmp246,_tmp246,_tmp246 + 6U},0,{_tmp247,_tmp247,_tmp247 + 1U},(void*)& Cyc_opt72_spec,{_tmp248,_tmp248,_tmp248 + 32U}};struct Cyc_List_List Cyc_opt72={(void*)& Cyc_opt72_tuple,0};
# 1236
struct Cyc_Arg_Unit_spec_Arg_Spec_struct Cyc_opt73_spec={0U,Cyc_print_options};static char _tmp249[7U]="-usage";static char _tmp24A[1U]="";static char _tmp24B[32U]="print out the available options";struct _tuple29 Cyc_opt73_tuple={{_tmp249,_tmp249,_tmp249 + 7U},0,{_tmp24A,_tmp24A,_tmp24A + 1U},(void*)& Cyc_opt73_spec,{_tmp24B,_tmp24B,_tmp24B + 32U}};struct Cyc_List_List Cyc_opt73={(void*)& Cyc_opt73_tuple,0};
# 1239
struct Cyc_Arg_Unit_spec_Arg_Spec_struct Cyc_opt74_spec={0U,Cyc_set_notoseqc};static char _tmp24C[10U]="-no-seq-c";static char _tmp24D[1U]="";static char _tmp24E[71U]="Do not force left-to-right evaluation order of generated code (unsafe)";struct _tuple29 Cyc_opt74_tuple={{_tmp24C,_tmp24C,_tmp24C + 10U},0,{_tmp24D,_tmp24D,_tmp24D + 1U},(void*)& Cyc_opt74_spec,{_tmp24E,_tmp24E,_tmp24E + 71U}};struct Cyc_List_List Cyc_opt74={(void*)& Cyc_opt74_tuple,0};
# 1242
struct Cyc_Arg_Unit_spec_Arg_Spec_struct Cyc_opt75_spec={0U,Cyc_set_pg};static char _tmp24F[4U]="-pg";static char _tmp250[1U]="";static char _tmp251[33U]="Compile for profiling with gprof";struct _tuple29 Cyc_opt75_tuple={{_tmp24F,_tmp24F,_tmp24F + 4U},0,{_tmp250,_tmp250,_tmp250 + 1U},(void*)& Cyc_opt75_spec,{_tmp251,_tmp251,_tmp251 + 33U}};struct Cyc_List_List Cyc_opt75={(void*)& Cyc_opt75_tuple,0};
# 1246
struct Cyc_Arg_Set_spec_Arg_Spec_struct Cyc_opt76_spec={3U,& Cyc_debug_r};static char _tmp252[7U]="-debug";static char _tmp253[1U]="";static char _tmp254[37U]="print compiler debugging information";struct _tuple29 Cyc_opt76_tuple={{_tmp252,_tmp252,_tmp252 + 7U},0,{_tmp253,_tmp253,_tmp253 + 1U},(void*)& Cyc_opt76_spec,{_tmp254,_tmp254,_tmp254 + 37U}};struct Cyc_List_List Cyc_opt76={(void*)& Cyc_opt76_tuple,0};
# 1249
struct Cyc_List_List*Cyc_global_options[76U]={& Cyc_opt1,& Cyc_opt2,& Cyc_opt3,& Cyc_opt4,& Cyc_opt5,& Cyc_opt6,& Cyc_opt7,& Cyc_opt8,& Cyc_opt9,& Cyc_opt10,& Cyc_opt11,& Cyc_opt12,& Cyc_opt13,& Cyc_opt14,& Cyc_opt15,& Cyc_opt16,& Cyc_opt17,& Cyc_opt18,& Cyc_opt19,& Cyc_opt20,& Cyc_opt21,& Cyc_opt22,& Cyc_opt23,& Cyc_opt24,& Cyc_opt25,& Cyc_opt26,& Cyc_opt27,& Cyc_opt28,& Cyc_opt29,& Cyc_opt30,& Cyc_opt31,& Cyc_opt32,& Cyc_opt74,& Cyc_opt75,& Cyc_opt33,& Cyc_opt34,& Cyc_opt35,& Cyc_opt36,& Cyc_opt37,& Cyc_opt38,& Cyc_opt39,& Cyc_opt40,& Cyc_opt41,& Cyc_opt42,& Cyc_opt43,& Cyc_opt44,& Cyc_opt45,& Cyc_opt46,& Cyc_opt47,& Cyc_opt48,& Cyc_opt49,& Cyc_opt50,& Cyc_opt51,& Cyc_opt52,& Cyc_opt53,& Cyc_opt54,& Cyc_opt55,& Cyc_opt56,& Cyc_opt57,& Cyc_opt58,& Cyc_opt59,& Cyc_opt60,& Cyc_opt61,& Cyc_opt62,& Cyc_opt63,& Cyc_opt64,& Cyc_opt65,& Cyc_opt66,& Cyc_opt67,& Cyc_opt68,& Cyc_opt69,& Cyc_opt70,& Cyc_opt71,& Cyc_opt72,& Cyc_opt73,& Cyc_opt76};
# 1260
void Cyc_print_options(){
({({struct Cyc_List_List*_tmp3C1=Cyc_global_options[0];Cyc_Arg_usage(_tmp3C1,({const char*_tmp255="<program.cyc>";_tag_fat(_tmp255,sizeof(char),14U);}));});});}
# 1265
int Cyc_main(int argc,struct _fat_ptr argv){
# 1269
({GC_blacklist_warn_clear();});{
# 1272
struct _fat_ptr optstring=({const char*_tmp2E9="Options:";_tag_fat(_tmp2E9,sizeof(char),9U);});
# 1274
{int i=1;for(0;(unsigned)i < 76U;++ i){
(*((struct Cyc_List_List**)_check_known_subscript_notnull(Cyc_global_options,76U,sizeof(struct Cyc_List_List*),i - 1)))->tl=Cyc_global_options[i];}}{
# 1277
struct Cyc_List_List*options=Cyc_global_options[0];
# 1279
struct _fat_ptr otherargs=({Cyc_Specsfile_parse_b(options,Cyc_add_other,Cyc_add_other_flag,optstring,argv);});
# 1285
struct _fat_ptr specs_file=({Cyc_find_in_arch_path(({const char*_tmp2E8="cycspecs";_tag_fat(_tmp2E8,sizeof(char),9U);}));});
if(Cyc_v_r)({struct Cyc_String_pa_PrintArg_struct _tmp259=({struct Cyc_String_pa_PrintArg_struct _tmp30A;_tmp30A.tag=0U,_tmp30A.f1=(struct _fat_ptr)((struct _fat_ptr)specs_file);_tmp30A;});void*_tmp257[1U];_tmp257[0]=& _tmp259;({struct Cyc___cycFILE*_tmp3C3=Cyc_stderr;struct _fat_ptr _tmp3C2=({const char*_tmp258="Reading from specs file %s\n";_tag_fat(_tmp258,sizeof(char),28U);});Cyc_fprintf(_tmp3C3,_tmp3C2,_tag_fat(_tmp257,sizeof(void*),1U));});});{struct Cyc_List_List*specs=({Cyc_Specsfile_read_specs(specs_file);});
# 1289
struct _fat_ptr args_from_specs_file=({struct _fat_ptr(*_tmp2E5)(struct _fat_ptr cmdline)=Cyc_Specsfile_split_specs;struct _fat_ptr _tmp2E6=({({struct Cyc_List_List*_tmp3C4=specs;Cyc_Specsfile_get_spec(_tmp3C4,({const char*_tmp2E7="cyclone";_tag_fat(_tmp2E7,sizeof(char),8U);}));});});_tmp2E5(_tmp2E6);});
if(({struct _fat_ptr*_tmp3C5=(struct _fat_ptr*)args_from_specs_file.curr;_tmp3C5 != (struct _fat_ptr*)(_tag_fat(0,0,0)).curr;})){
Cyc_Arg_current=0;
({Cyc_Arg_parse(options,Cyc_add_other,Cyc_add_other_flag,optstring,args_from_specs_file);});}{
# 1290
struct _fat_ptr target_cflags=({({struct Cyc_List_List*_tmp3C6=specs;Cyc_Specsfile_get_spec(_tmp3C6,({const char*_tmp2E4="cyclone_target_cflags";_tag_fat(_tmp2E4,sizeof(char),22U);}));});});
# 1295
struct _fat_ptr cyclone_cc=({({struct Cyc_List_List*_tmp3C7=specs;Cyc_Specsfile_get_spec(_tmp3C7,({const char*_tmp2E3="cyclone_cc";_tag_fat(_tmp2E3,sizeof(char),11U);}));});});
if(!((unsigned)cyclone_cc.curr))cyclone_cc=({const char*_tmp25A="gcc";_tag_fat(_tmp25A,sizeof(char),4U);});Cyc_def_inc_path=({({struct Cyc_List_List*_tmp3C8=specs;Cyc_Specsfile_get_spec(_tmp3C8,({const char*_tmp25B="cyclone_inc_path";_tag_fat(_tmp25B,sizeof(char),17U);}));});});
# 1300
Cyc_Arg_current=0;
({Cyc_Arg_parse(options,Cyc_add_other,Cyc_add_other_flag,optstring,otherargs);});
if(({({struct _fat_ptr _tmp3C9=(struct _fat_ptr)Cyc_cpp;Cyc_strcmp(_tmp3C9,({const char*_tmp25C="";_tag_fat(_tmp25C,sizeof(char),1U);}));});})== 0){
# 1318 "cyclone.cyc"
const char*dash_E=Cyc_produce_dependencies?"":" -E";
({void(*_tmp25D)(struct _fat_ptr s)=Cyc_set_cpp;struct _fat_ptr _tmp25E=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp261=({struct Cyc_String_pa_PrintArg_struct _tmp30E;_tmp30E.tag=0U,_tmp30E.f1=(struct _fat_ptr)((struct _fat_ptr)cyclone_cc);_tmp30E;});struct Cyc_String_pa_PrintArg_struct _tmp262=({struct Cyc_String_pa_PrintArg_struct _tmp30D;_tmp30D.tag=0U,_tmp30D.f1=(struct _fat_ptr)((struct _fat_ptr)target_cflags);_tmp30D;});struct Cyc_String_pa_PrintArg_struct _tmp263=({struct Cyc_String_pa_PrintArg_struct _tmp30C;_tmp30C.tag=0U,({
# 1321
struct _fat_ptr _tmp3CA=(struct _fat_ptr)({const char*_tmp265=dash_E;_tag_fat(_tmp265,sizeof(char),_get_zero_arr_size_char((void*)_tmp265,1U));});_tmp30C.f1=_tmp3CA;});_tmp30C;});struct Cyc_String_pa_PrintArg_struct _tmp264=({struct Cyc_String_pa_PrintArg_struct _tmp30B;_tmp30B.tag=0U,_tmp30B.f1=(struct _fat_ptr)((struct _fat_ptr)specs_file);_tmp30B;});void*_tmp25F[4U];_tmp25F[0]=& _tmp261,_tmp25F[1]=& _tmp262,_tmp25F[2]=& _tmp263,_tmp25F[3]=& _tmp264;({struct _fat_ptr _tmp3CB=({const char*_tmp260="%s %s -w -x c%s -specs %s";_tag_fat(_tmp260,sizeof(char),26U);});Cyc_aprintf(_tmp3CB,_tag_fat(_tmp25F,sizeof(void*),4U));});});_tmp25D(_tmp25E);});}
# 1302 "cyclone.cyc"
if(
# 1323 "cyclone.cyc"
Cyc_cyclone_files == 0 && Cyc_ccargs == 0){
({void*_tmp266=0U;({struct Cyc___cycFILE*_tmp3CD=Cyc_stderr;struct _fat_ptr _tmp3CC=({const char*_tmp267="missing file\n";_tag_fat(_tmp267,sizeof(char),14U);});Cyc_fprintf(_tmp3CD,_tmp3CC,_tag_fat(_tmp266,sizeof(void*),0U));});});
({exit(1);});}
# 1302 "cyclone.cyc"
if(
# 1332 "cyclone.cyc"
!Cyc_stop_after_cpp_r && !Cyc_nocyc_setjmp_r)
Cyc_cyc_setjmp=({Cyc_find_in_arch_path(({const char*_tmp268="cyc_setjmp.h";_tag_fat(_tmp268,sizeof(char),13U);}));});
# 1302 "cyclone.cyc"
if(
# 1334 "cyclone.cyc"
!Cyc_stop_after_cpp_r &&({Cyc_strlen((struct _fat_ptr)Cyc_cyc_include);})== (unsigned long)0)
Cyc_cyc_include=({Cyc_find_in_exec_path(({const char*_tmp269="cyc_include.h";_tag_fat(_tmp269,sizeof(char),14U);}));});
# 1302 "cyclone.cyc"
{struct _handler_cons _tmp26A;_push_handler(& _tmp26A);{int _tmp26C=0;if(setjmp(_tmp26A.handler))_tmp26C=1;if(!_tmp26C){
# 1339 "cyclone.cyc"
{struct Cyc_List_List*l=({Cyc_List_rev(Cyc_cyclone_files);});for(0;l != 0;l=l->tl){
Cyc_last_filename=*((struct _fat_ptr*)l->hd);
({Cyc_process_file(*((struct _fat_ptr*)l->hd));});
if(Cyc_compile_failure){int _tmp26D=1;_npop_handler(0U);return _tmp26D;}}}
# 1339
;_pop_handler();}else{void*_tmp26B=(void*)Cyc_Core_get_exn_thrown();void*_tmp26E=_tmp26B;void*_tmp26F;_LL1: _tmp26F=_tmp26E;_LL2: {void*x=_tmp26F;
# 1346
if(Cyc_compile_failure)return 1;else{
({Cyc_Core_rethrow(x);});}}_LL0:;}}}
# 1350
if(((Cyc_stop_after_cpp_r || Cyc_parseonly_r)|| Cyc_tc_r)|| Cyc_toc_r)return 0;if(Cyc_ccargs == 0)
# 1356
return 0;
# 1350
if(
# 1359
(unsigned)Cyc_Specsfile_target_arch.curr &&(unsigned)Cyc_Specsfile_cyclone_exec_path)
({void(*_tmp270)(struct _fat_ptr s)=Cyc_add_ccarg;struct _fat_ptr _tmp271=(struct _fat_ptr)({struct _fat_ptr(*_tmp272)(struct Cyc_List_List*,struct _fat_ptr)=Cyc_str_sepstr;struct Cyc_List_List*_tmp273=({struct Cyc_List_List*_tmp276=_cycalloc(sizeof(*_tmp276));({struct _fat_ptr*_tmp3D0=({struct _fat_ptr*_tmp275=_cycalloc(sizeof(*_tmp275));({struct _fat_ptr _tmp3CF=({const char*_tmp274="";_tag_fat(_tmp274,sizeof(char),1U);});*_tmp275=_tmp3CF;});_tmp275;});_tmp276->hd=_tmp3D0;}),({
struct Cyc_List_List*_tmp3CE=({Cyc_add_subdir(Cyc_Specsfile_cyclone_exec_path,Cyc_Specsfile_target_arch);});_tmp276->tl=_tmp3CE;});_tmp276;});struct _fat_ptr _tmp277=({const char*_tmp278=" -L";_tag_fat(_tmp278,sizeof(char),4U);});_tmp272(_tmp273,_tmp277);});_tmp270(_tmp271);});
# 1350
({void(*_tmp279)(struct _fat_ptr s)=Cyc_add_ccarg;struct _fat_ptr _tmp27A=(struct _fat_ptr)({({struct _fat_ptr _tmp3D1=({const char*_tmp27B="-L";_tag_fat(_tmp27B,sizeof(char),3U);});Cyc_strconcat(_tmp3D1,(struct _fat_ptr)Cyc_Specsfile_def_lib_path);});});_tmp279(_tmp27A);});
# 1365
Cyc_ccargs=({Cyc_List_rev(Cyc_ccargs);});{
struct _fat_ptr ccargs_string=({struct _fat_ptr(*_tmp2DE)(struct Cyc_List_List*,struct _fat_ptr)=Cyc_str_sepstr;struct Cyc_List_List*_tmp2DF=({({struct Cyc_List_List*(*_tmp3D2)(struct _fat_ptr*(*f)(struct _fat_ptr*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp2E0)(struct _fat_ptr*(*f)(struct _fat_ptr*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct _fat_ptr*(*f)(struct _fat_ptr*),struct Cyc_List_List*x))Cyc_List_map;_tmp2E0;});_tmp3D2(Cyc_sh_escape_stringptr,Cyc_ccargs);});});struct _fat_ptr _tmp2E1=({const char*_tmp2E2=" ";_tag_fat(_tmp2E2,sizeof(char),2U);});_tmp2DE(_tmp2DF,_tmp2E1);});
Cyc_libargs=({Cyc_List_rev(Cyc_libargs);});{
struct _fat_ptr libargs_string=({struct _fat_ptr(*_tmp2D6)(struct Cyc_List_List*,struct _fat_ptr)=Cyc_str_sepstr;struct Cyc_List_List*_tmp2D7=({struct Cyc_List_List*_tmp2DB=_cycalloc(sizeof(*_tmp2DB));({struct _fat_ptr*_tmp3D6=({struct _fat_ptr*_tmp2D9=_cycalloc(sizeof(*_tmp2D9));({struct _fat_ptr _tmp3D5=({const char*_tmp2D8="";_tag_fat(_tmp2D8,sizeof(char),1U);});*_tmp2D9=_tmp3D5;});_tmp2D9;});_tmp2DB->hd=_tmp3D6;}),({
struct Cyc_List_List*_tmp3D4=({({struct Cyc_List_List*(*_tmp3D3)(struct _fat_ptr*(*f)(struct _fat_ptr*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp2DA)(struct _fat_ptr*(*f)(struct _fat_ptr*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct _fat_ptr*(*f)(struct _fat_ptr*),struct Cyc_List_List*x))Cyc_List_map;_tmp2DA;});_tmp3D3(Cyc_sh_escape_stringptr,Cyc_libargs);});});_tmp2DB->tl=_tmp3D4;});_tmp2DB;});struct _fat_ptr _tmp2DC=({const char*_tmp2DD=" ";_tag_fat(_tmp2DD,sizeof(char),2U);});_tmp2D6(_tmp2D7,_tmp2DC);});
# 1371
struct Cyc_List_List*stdlib;
struct _fat_ptr stdlib_string;
int is_not_executable=
((Cyc_stop_after_asmfile_r || Cyc_stop_after_objectfile_r)||
# 1376
 Cyc_output_file != 0 &&({({struct _fat_ptr _tmp3D8=*((struct _fat_ptr*)_check_null(Cyc_output_file));Cyc_Filename_check_suffix(_tmp3D8,({const char*_tmp2D4=".a";_tag_fat(_tmp2D4,sizeof(char),3U);}));});}))||
 Cyc_output_file != 0 &&({({struct _fat_ptr _tmp3D7=*((struct _fat_ptr*)_check_null(Cyc_output_file));Cyc_Filename_check_suffix(_tmp3D7,({const char*_tmp2D5=".lib";_tag_fat(_tmp2D5,sizeof(char),5U);}));});});
if(is_not_executable){
stdlib=0;
stdlib_string=(struct _fat_ptr)({const char*_tmp27C="";_tag_fat(_tmp27C,sizeof(char),1U);});}else{
# 1383
struct _fat_ptr libcyc_flag;
struct _fat_ptr nogc_filename;
struct _fat_ptr runtime_filename;
struct _fat_ptr gc_filename;
struct _fat_ptr sigsegv_filename;
if(Cyc_pa_r){
libcyc_flag=({const char*_tmp27D="-lcyc_a";_tag_fat(_tmp27D,sizeof(char),8U);});
nogc_filename=({const char*_tmp27E="nogc_a.a";_tag_fat(_tmp27E,sizeof(char),9U);});
runtime_filename=({const char*_tmp27F="runtime_cyc_a.a";_tag_fat(_tmp27F,sizeof(char),16U);});
sigsegv_filename=({const char*_tmp280="libsigsegv.a";_tag_fat(_tmp280,sizeof(char),13U);});}else{
if(Cyc_nocheck_r){
libcyc_flag=({const char*_tmp281="-lcyc_nocheck";_tag_fat(_tmp281,sizeof(char),14U);});
nogc_filename=({const char*_tmp282="nogc.a";_tag_fat(_tmp282,sizeof(char),7U);});
runtime_filename=({const char*_tmp283="runtime_cyc.a";_tag_fat(_tmp283,sizeof(char),14U);});
sigsegv_filename=({const char*_tmp284="libsigsegv.a";_tag_fat(_tmp284,sizeof(char),13U);});}else{
if(Cyc_pg_r){
libcyc_flag=({const char*_tmp285="-lcyc_pg";_tag_fat(_tmp285,sizeof(char),9U);});
runtime_filename=({const char*_tmp286="runtime_cyc_pg.a";_tag_fat(_tmp286,sizeof(char),17U);});
nogc_filename=({const char*_tmp287="nogc_pg.a";_tag_fat(_tmp287,sizeof(char),10U);});
sigsegv_filename=({const char*_tmp288="libsigsegv.a";_tag_fat(_tmp288,sizeof(char),13U);});}else{
if(Cyc_Lex_compile_for_boot_r){
# 1405
libcyc_flag=({const char*_tmp289="-lcycboot";_tag_fat(_tmp289,sizeof(char),10U);});
nogc_filename=({const char*_tmp28A="nogc.a";_tag_fat(_tmp28A,sizeof(char),7U);});
runtime_filename=({const char*_tmp28B="runtime_cyc.a";_tag_fat(_tmp28B,sizeof(char),14U);});
sigsegv_filename=({const char*_tmp28C="libsigsegv.a";_tag_fat(_tmp28C,sizeof(char),13U);});}else{
if(Cyc_pthread_r){
libcyc_flag=({const char*_tmp28D="-lcyc";_tag_fat(_tmp28D,sizeof(char),6U);});
nogc_filename=({const char*_tmp28E="nogc_pthread.a";_tag_fat(_tmp28E,sizeof(char),15U);});
runtime_filename=({const char*_tmp28F="runtime_cyc_pthread.a";_tag_fat(_tmp28F,sizeof(char),22U);});
sigsegv_filename=({const char*_tmp290="libsigsegv.a";_tag_fat(_tmp290,sizeof(char),13U);});}else{
# 1415
libcyc_flag=({const char*_tmp291="-lcyc";_tag_fat(_tmp291,sizeof(char),6U);});
nogc_filename=({const char*_tmp292="nogc.a";_tag_fat(_tmp292,sizeof(char),7U);});
runtime_filename=({const char*_tmp293="runtime_cyc.a";_tag_fat(_tmp293,sizeof(char),14U);});
sigsegv_filename=({const char*_tmp294="libsigsegv.a";_tag_fat(_tmp294,sizeof(char),13U);});}}}}}
# 1420
if(!Cyc_pthread_r)
gc_filename=({const char*_tmp295="gc.a";_tag_fat(_tmp295,sizeof(char),5U);});else{
# 1423
gc_filename=({const char*_tmp296="gc_pthread.a";_tag_fat(_tmp296,sizeof(char),13U);});}{
# 1425
struct _fat_ptr gc=Cyc_nogc_r?({Cyc_find_in_arch_path(nogc_filename);}):({Cyc_find_in_arch_path(gc_filename);});
struct _fat_ptr runtime=({Cyc_find_in_arch_path(runtime_filename);});
struct _fat_ptr sigsegv=({Cyc_find_in_arch_path(sigsegv_filename);});
# 1429
stdlib=0;
stdlib_string=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp299=({struct Cyc_String_pa_PrintArg_struct _tmp312;_tmp312.tag=0U,_tmp312.f1=(struct _fat_ptr)((struct _fat_ptr)libcyc_flag);_tmp312;});struct Cyc_String_pa_PrintArg_struct _tmp29A=({struct Cyc_String_pa_PrintArg_struct _tmp311;_tmp311.tag=0U,_tmp311.f1=(struct _fat_ptr)((struct _fat_ptr)runtime);_tmp311;});struct Cyc_String_pa_PrintArg_struct _tmp29B=({struct Cyc_String_pa_PrintArg_struct _tmp310;_tmp310.tag=0U,_tmp310.f1=(struct _fat_ptr)((struct _fat_ptr)sigsegv);_tmp310;});struct Cyc_String_pa_PrintArg_struct _tmp29C=({struct Cyc_String_pa_PrintArg_struct _tmp30F;_tmp30F.tag=0U,_tmp30F.f1=(struct _fat_ptr)((struct _fat_ptr)gc);_tmp30F;});void*_tmp297[4U];_tmp297[0]=& _tmp299,_tmp297[1]=& _tmp29A,_tmp297[2]=& _tmp29B,_tmp297[3]=& _tmp29C;({struct _fat_ptr _tmp3D9=({const char*_tmp298=" %s %s %s %s";_tag_fat(_tmp298,sizeof(char),13U);});Cyc_aprintf(_tmp3D9,_tag_fat(_tmp297,sizeof(void*),4U));});});}}
# 1434
if(Cyc_ic_r){struct _handler_cons _tmp29D;_push_handler(& _tmp29D);{int _tmp29F=0;if(setjmp(_tmp29D.handler))_tmp29F=1;if(!_tmp29F){
Cyc_ccargs=({({struct Cyc_List_List*(*_tmp3DA)(int(*f)(struct _fat_ptr*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp2A0)(int(*f)(struct _fat_ptr*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(int(*f)(struct _fat_ptr*),struct Cyc_List_List*x))Cyc_List_filter;_tmp2A0;});_tmp3DA(Cyc_is_cfile,Cyc_ccargs);});});
Cyc_libargs=({({struct Cyc_List_List*(*_tmp3DB)(int(*f)(struct _fat_ptr*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp2A1)(int(*f)(struct _fat_ptr*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(int(*f)(struct _fat_ptr*),struct Cyc_List_List*x))Cyc_List_filter;_tmp2A1;});_tmp3DB(Cyc_is_cfile,Cyc_libargs);});});{
struct Cyc_List_List*lf=({struct Cyc_List_List*(*_tmp2B7)(struct Cyc_List_List*x,struct Cyc_List_List*y)=Cyc_List_append;struct Cyc_List_List*_tmp2B8=stdlib;struct Cyc_List_List*_tmp2B9=({Cyc_List_append(Cyc_ccargs,Cyc_libargs);});_tmp2B7(_tmp2B8,_tmp2B9);});
if(!is_not_executable)
lf=({struct Cyc_List_List*_tmp2A2=_cycalloc(sizeof(*_tmp2A2));_tmp2A2->hd=Cyc_final_strptr,_tmp2A2->tl=lf;_tmp2A2;});{
# 1438
struct Cyc_Interface_I*i=({({struct Cyc_Interface_I*(*_tmp3DD)(struct Cyc_Interface_I*(*get)(struct _fat_ptr*),struct Cyc_List_List*la,struct Cyc_List_List*linfo)=({
# 1442
struct Cyc_Interface_I*(*_tmp2B6)(struct Cyc_Interface_I*(*get)(struct _fat_ptr*),struct Cyc_List_List*la,struct Cyc_List_List*linfo)=(struct Cyc_Interface_I*(*)(struct Cyc_Interface_I*(*get)(struct _fat_ptr*),struct Cyc_List_List*la,struct Cyc_List_List*linfo))Cyc_Interface_get_and_merge_list;_tmp2B6;});struct Cyc_List_List*_tmp3DC=lf;_tmp3DD(Cyc_read_cycio,_tmp3DC,lf);});});
if(i == 0){
({void*_tmp2A3=0U;({struct Cyc___cycFILE*_tmp3DF=Cyc_stderr;struct _fat_ptr _tmp3DE=({const char*_tmp2A4="Error: interfaces incompatible\n";_tag_fat(_tmp2A4,sizeof(char),32U);});Cyc_fprintf(_tmp3DF,_tmp3DE,_tag_fat(_tmp2A3,sizeof(void*),0U));});});
Cyc_compile_failure=1;
({Cyc_remove_cfiles();});{
int _tmp2A5=1;_npop_handler(0U);return _tmp2A5;}}
# 1443
if(is_not_executable){
# 1450
if(Cyc_output_file != 0){
struct _fat_ptr output_file_io=({struct Cyc_String_pa_PrintArg_struct _tmp2AB=({struct Cyc_String_pa_PrintArg_struct _tmp313;_tmp313.tag=0U,({struct _fat_ptr _tmp3E0=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Filename_chop_extension(*((struct _fat_ptr*)_check_null(Cyc_output_file)));}));_tmp313.f1=_tmp3E0;});_tmp313;});void*_tmp2A9[1U];_tmp2A9[0]=& _tmp2AB;({struct _fat_ptr _tmp3E1=({const char*_tmp2AA="%s.cycio";_tag_fat(_tmp2AA,sizeof(char),9U);});Cyc_aprintf(_tmp3E1,_tag_fat(_tmp2A9,sizeof(void*),1U));});});
struct Cyc___cycFILE*f=({({struct _fat_ptr _tmp3E3=(struct _fat_ptr)output_file_io;struct _fat_ptr _tmp3E2=({const char*_tmp2A7="wb";_tag_fat(_tmp2A7,sizeof(char),3U);});Cyc_try_file_open(_tmp3E3,_tmp3E2,({const char*_tmp2A8="interface object file";_tag_fat(_tmp2A8,sizeof(char),22U);}));});});
if(f == 0){
Cyc_compile_failure=1;
({Cyc_remove_cfiles();});{
int _tmp2A6=1;_npop_handler(0U);return _tmp2A6;}}
# 1453
({Cyc_Interface_save(i,f);});
# 1459
({Cyc_file_close(f);});}}else{
# 1462
if(!({int(*_tmp2AC)(struct Cyc_Interface_I*i1,struct Cyc_Interface_I*i2,struct _tuple25*info)=Cyc_Interface_is_subinterface;struct Cyc_Interface_I*_tmp2AD=({Cyc_Interface_empty();});struct Cyc_Interface_I*_tmp2AE=i;struct _tuple25*_tmp2AF=({struct _tuple25*_tmp2B2=_cycalloc(sizeof(*_tmp2B2));
({struct _fat_ptr _tmp3E5=({const char*_tmp2B0="empty interface";_tag_fat(_tmp2B0,sizeof(char),16U);});_tmp2B2->f1=_tmp3E5;}),({struct _fat_ptr _tmp3E4=({const char*_tmp2B1="global interface";_tag_fat(_tmp2B1,sizeof(char),17U);});_tmp2B2->f2=_tmp3E4;});_tmp2B2;});_tmp2AC(_tmp2AD,_tmp2AE,_tmp2AF);})){
({void*_tmp2B3=0U;({struct Cyc___cycFILE*_tmp3E7=Cyc_stderr;struct _fat_ptr _tmp3E6=({const char*_tmp2B4="Error: some objects are still undefined\n";_tag_fat(_tmp2B4,sizeof(char),41U);});Cyc_fprintf(_tmp3E7,_tmp3E6,_tag_fat(_tmp2B3,sizeof(void*),0U));});});
Cyc_compile_failure=1;
({Cyc_remove_cfiles();});{
int _tmp2B5=1;_npop_handler(0U);return _tmp2B5;}}}}}
# 1435
;_pop_handler();}else{void*_tmp29E=(void*)Cyc_Core_get_exn_thrown();void*_tmp2BA=_tmp29E;void*_tmp2BB;_LL6: _tmp2BB=_tmp2BA;_LL7: {void*e=_tmp2BB;
# 1472
({struct Cyc_String_pa_PrintArg_struct _tmp2BE=({struct Cyc_String_pa_PrintArg_struct _tmp316;_tmp316.tag=0U,({struct _fat_ptr _tmp3E8=(struct _fat_ptr)({const char*_tmp2C2=({Cyc_Core_get_exn_name(e);});_tag_fat(_tmp2C2,sizeof(char),_get_zero_arr_size_char((void*)_tmp2C2,1U));});_tmp316.f1=_tmp3E8;});_tmp316;});struct Cyc_String_pa_PrintArg_struct _tmp2BF=({struct Cyc_String_pa_PrintArg_struct _tmp315;_tmp315.tag=0U,({struct _fat_ptr _tmp3E9=(struct _fat_ptr)({const char*_tmp2C1=({Cyc_Core_get_exn_filename();});_tag_fat(_tmp2C1,sizeof(char),_get_zero_arr_size_char((void*)_tmp2C1,1U));});_tmp315.f1=_tmp3E9;});_tmp315;});struct Cyc_Int_pa_PrintArg_struct _tmp2C0=({struct Cyc_Int_pa_PrintArg_struct _tmp314;_tmp314.tag=1U,({unsigned long _tmp3EA=(unsigned long)({Cyc_Core_get_exn_lineno();});_tmp314.f1=_tmp3EA;});_tmp314;});void*_tmp2BC[3U];_tmp2BC[0]=& _tmp2BE,_tmp2BC[1]=& _tmp2BF,_tmp2BC[2]=& _tmp2C0;({struct Cyc___cycFILE*_tmp3EC=Cyc_stderr;struct _fat_ptr _tmp3EB=({const char*_tmp2BD="INTERNAL COMPILER FAILURE:  exception %s from around %s:%d thrown.\n  Please send bug report to cyclone-bugs-l@lists.cs.cornell.edu";_tag_fat(_tmp2BD,sizeof(char),131U);});Cyc_fprintf(_tmp3EC,_tmp3EB,_tag_fat(_tmp2BC,sizeof(void*),3U));});});
Cyc_compile_failure=1;
({Cyc_remove_cfiles();});
return 1;}_LL5:;}}}{
# 1434
struct _fat_ptr outfile_str=({const char*_tmp2D3="";_tag_fat(_tmp2D3,sizeof(char),1U);});
# 1479
if(Cyc_output_file != 0)
outfile_str=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp2C5=({struct Cyc_String_pa_PrintArg_struct _tmp317;_tmp317.tag=0U,({struct _fat_ptr _tmp3ED=(struct _fat_ptr)((struct _fat_ptr)({Cyc_sh_escape_string(*((struct _fat_ptr*)_check_null(Cyc_output_file)));}));_tmp317.f1=_tmp3ED;});_tmp317;});void*_tmp2C3[1U];_tmp2C3[0]=& _tmp2C5;({struct _fat_ptr _tmp3EE=({const char*_tmp2C4=" -o %s";_tag_fat(_tmp2C4,sizeof(char),7U);});Cyc_aprintf(_tmp3EE,_tag_fat(_tmp2C3,sizeof(void*),1U));});});{
# 1479
struct _fat_ptr cccmd=({struct Cyc_String_pa_PrintArg_struct _tmp2CD=({struct Cyc_String_pa_PrintArg_struct _tmp31E;_tmp31E.tag=0U,_tmp31E.f1=(struct _fat_ptr)((struct _fat_ptr)cyclone_cc);_tmp31E;});struct Cyc_String_pa_PrintArg_struct _tmp2CE=({struct Cyc_String_pa_PrintArg_struct _tmp31D;_tmp31D.tag=0U,_tmp31D.f1=(struct _fat_ptr)((struct _fat_ptr)target_cflags);_tmp31D;});struct Cyc_String_pa_PrintArg_struct _tmp2CF=({struct Cyc_String_pa_PrintArg_struct _tmp31C;_tmp31C.tag=0U,_tmp31C.f1=(struct _fat_ptr)((struct _fat_ptr)ccargs_string);_tmp31C;});struct Cyc_String_pa_PrintArg_struct _tmp2D0=({struct Cyc_String_pa_PrintArg_struct _tmp31B;_tmp31B.tag=0U,_tmp31B.f1=(struct _fat_ptr)((struct _fat_ptr)outfile_str);_tmp31B;});struct Cyc_String_pa_PrintArg_struct _tmp2D1=({struct Cyc_String_pa_PrintArg_struct _tmp31A;_tmp31A.tag=0U,_tmp31A.f1=(struct _fat_ptr)((struct _fat_ptr)stdlib_string);_tmp31A;});struct Cyc_String_pa_PrintArg_struct _tmp2D2=({struct Cyc_String_pa_PrintArg_struct _tmp319;_tmp319.tag=0U,_tmp319.f1=(struct _fat_ptr)((struct _fat_ptr)libargs_string);_tmp319;});void*_tmp2CB[6U];_tmp2CB[0]=& _tmp2CD,_tmp2CB[1]=& _tmp2CE,_tmp2CB[2]=& _tmp2CF,_tmp2CB[3]=& _tmp2D0,_tmp2CB[4]=& _tmp2D1,_tmp2CB[5]=& _tmp2D2;({struct _fat_ptr _tmp3EF=({const char*_tmp2CC="%s %s %s%s %s%s";_tag_fat(_tmp2CC,sizeof(char),16U);});Cyc_aprintf(_tmp3EF,_tag_fat(_tmp2CB,sizeof(void*),6U));});});
# 1487
if(Cyc_v_r){({struct Cyc_String_pa_PrintArg_struct _tmp2C8=({struct Cyc_String_pa_PrintArg_struct _tmp318;_tmp318.tag=0U,_tmp318.f1=(struct _fat_ptr)((struct _fat_ptr)cccmd);_tmp318;});void*_tmp2C6[1U];_tmp2C6[0]=& _tmp2C8;({struct Cyc___cycFILE*_tmp3F1=Cyc_stderr;struct _fat_ptr _tmp3F0=({const char*_tmp2C7="%s\n";_tag_fat(_tmp2C7,sizeof(char),4U);});Cyc_fprintf(_tmp3F1,_tmp3F0,_tag_fat(_tmp2C6,sizeof(void*),1U));});});({Cyc_fflush(Cyc_stderr);});}if(({system((const char*)_check_null(_untag_fat_ptr(cccmd,sizeof(char),1U)));})!= 0){
# 1489
({void*_tmp2C9=0U;({struct Cyc___cycFILE*_tmp3F3=Cyc_stderr;struct _fat_ptr _tmp3F2=({const char*_tmp2CA="Error: C compiler failed\n";_tag_fat(_tmp2CA,sizeof(char),26U);});Cyc_fprintf(_tmp3F3,_tmp3F2,_tag_fat(_tmp2C9,sizeof(void*),0U));});});
Cyc_compile_failure=1;
({Cyc_remove_cfiles();});
return 1;}
# 1487
({Cyc_remove_cfiles();});
# 1496
return Cyc_compile_failure?1: 0;}}}}}}}}}

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
# 108 "core.h"
void*Cyc_Core_fst(struct _tuple0*);
# 111
void*Cyc_Core_snd(struct _tuple0*);extern char Cyc_Core_Invalid_argument[17U];struct Cyc_Core_Invalid_argument_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Failure[8U];struct Cyc_Core_Failure_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Impossible[11U];struct Cyc_Core_Impossible_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Not_found[10U];struct Cyc_Core_Not_found_exn_struct{char*tag;};extern char Cyc_Core_Unreachable[12U];struct Cyc_Core_Unreachable_exn_struct{char*tag;struct _fat_ptr f1;};
# 168
extern struct _RegionHandle*Cyc_Core_unique_region;struct Cyc_Core_DynamicRegion;struct Cyc_Core_NewDynamicRegion{struct Cyc_Core_DynamicRegion*key;};struct Cyc_Core_ThinRes{void*arr;unsigned nelts;};struct Cyc___cycFILE;
# 53 "cycboot.h"
extern struct Cyc___cycFILE*Cyc_stderr;struct Cyc_String_pa_PrintArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Int_pa_PrintArg_struct{int tag;unsigned long f1;};struct Cyc_Double_pa_PrintArg_struct{int tag;double f1;};struct Cyc_LongDouble_pa_PrintArg_struct{int tag;long double f1;};struct Cyc_ShortPtr_pa_PrintArg_struct{int tag;short*f1;};struct Cyc_IntPtr_pa_PrintArg_struct{int tag;unsigned long*f1;};
# 73
struct _fat_ptr Cyc_aprintf(struct _fat_ptr,struct _fat_ptr);
# 88
int Cyc_fflush(struct Cyc___cycFILE*);
# 100
int Cyc_fprintf(struct Cyc___cycFILE*,struct _fat_ptr,struct _fat_ptr);struct Cyc_ShortPtr_sa_ScanfArg_struct{int tag;short*f1;};struct Cyc_UShortPtr_sa_ScanfArg_struct{int tag;unsigned short*f1;};struct Cyc_IntPtr_sa_ScanfArg_struct{int tag;int*f1;};struct Cyc_UIntPtr_sa_ScanfArg_struct{int tag;unsigned*f1;};struct Cyc_StringPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_DoublePtr_sa_ScanfArg_struct{int tag;double*f1;};struct Cyc_FloatPtr_sa_ScanfArg_struct{int tag;float*f1;};struct Cyc_CharPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};extern char Cyc_FileCloseError[15U];struct Cyc_FileCloseError_exn_struct{char*tag;};extern char Cyc_FileOpenError[14U];struct Cyc_FileOpenError_exn_struct{char*tag;struct _fat_ptr f1;};struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};
# 54 "list.h"
struct Cyc_List_List*Cyc_List_list(struct _fat_ptr);
# 61
int Cyc_List_length(struct Cyc_List_List*x);
# 76
struct Cyc_List_List*Cyc_List_map(void*(*f)(void*),struct Cyc_List_List*x);
# 83
struct Cyc_List_List*Cyc_List_map_c(void*(*f)(void*,void*),void*env,struct Cyc_List_List*x);extern char Cyc_List_List_mismatch[14U];struct Cyc_List_List_mismatch_exn_struct{char*tag;};
# 178
struct Cyc_List_List*Cyc_List_imp_rev(struct Cyc_List_List*x);
# 184
struct Cyc_List_List*Cyc_List_append(struct Cyc_List_List*x,struct Cyc_List_List*y);extern char Cyc_List_Nth[4U];struct Cyc_List_Nth_exn_struct{char*tag;};
# 251
int Cyc_List_forall(int(*pred)(void*),struct Cyc_List_List*x);struct _tuple1{struct Cyc_List_List*f1;struct Cyc_List_List*f2;};
# 294
struct _tuple1 Cyc_List_split(struct Cyc_List_List*x);
# 391
struct Cyc_List_List*Cyc_List_filter(int(*f)(void*),struct Cyc_List_List*x);struct Cyc_PP_Ppstate;struct Cyc_PP_Out;struct Cyc_PP_Doc;
# 36 "position.h"
struct _fat_ptr Cyc_Position_string_of_loc(unsigned);struct Cyc_Position_Error;struct Cyc_Iter_Iter{void*env;int(*next)(void*env,void*dest);};struct Cyc_Dict_T;struct Cyc_Dict_Dict{int(*rel)(void*,void*);struct _RegionHandle*r;const struct Cyc_Dict_T*t;};extern char Cyc_Dict_Present[8U];struct Cyc_Dict_Present_exn_struct{char*tag;};extern char Cyc_Dict_Absent[7U];struct Cyc_Dict_Absent_exn_struct{char*tag;};struct Cyc_Relations_Reln;struct _union_Nmspace_Rel_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Abs_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_C_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Loc_n{int tag;int val;};union Cyc_Absyn_Nmspace{struct _union_Nmspace_Rel_n Rel_n;struct _union_Nmspace_Abs_n Abs_n;struct _union_Nmspace_C_n C_n;struct _union_Nmspace_Loc_n Loc_n;};
# 110 "absyn.h"
extern union Cyc_Absyn_Nmspace Cyc_Absyn_Loc_n;struct _tuple2{union Cyc_Absyn_Nmspace f1;struct _fat_ptr*f2;};
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
enum Cyc_Absyn_KindQual{Cyc_Absyn_AnyKind =0U,Cyc_Absyn_MemKind =1U,Cyc_Absyn_BoxKind =2U,Cyc_Absyn_XRgnKind =3U,Cyc_Absyn_RgnKind =4U,Cyc_Absyn_EffKind =5U,Cyc_Absyn_IntKind =6U,Cyc_Absyn_BoolKind =7U,Cyc_Absyn_PtrBndKind =8U};struct Cyc_Absyn_Kind{enum Cyc_Absyn_KindQual kind;enum Cyc_Absyn_AliasQual aliasqual;};struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct{int tag;struct Cyc_Absyn_Kind*f1;};struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct{int tag;struct Cyc_Core_Opt*f1;};struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct{int tag;struct Cyc_Core_Opt*f1;struct Cyc_Absyn_Kind*f2;};struct Cyc_Absyn_Tvar{struct _fat_ptr*name;int identity;void*kind;};struct Cyc_Absyn_PtrLoc{unsigned ptr_loc;unsigned rgn_loc;unsigned zt_loc;};struct Cyc_Absyn_PtrAtts{void*rgn;void*nullable;void*bounds;void*zero_term;struct Cyc_Absyn_PtrLoc*ptrloc;};struct Cyc_Absyn_PtrInfo{void*elt_type;struct Cyc_Absyn_Tqual elt_tq;struct Cyc_Absyn_PtrAtts ptr_atts;};struct Cyc_Absyn_VarargInfo{struct _fat_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;int inject;};struct Cyc_Absyn_Star_Absyn_Cap_struct{int tag;};struct Cyc_Absyn_Bot_Absyn_Cap_struct{int tag;};struct Cyc_Absyn_Nat_Absyn_Cap_struct{int tag;int f1;int f2;};struct Cyc_Absyn_RgnEffect{void*name;struct Cyc_List_List*caps;void*parent;};struct Cyc_Absyn_FnInfo{struct Cyc_List_List*tvars;void*effect;struct Cyc_Absyn_Tqual ret_tqual;void*ret_type;struct Cyc_List_List*args;int c_varargs;struct Cyc_Absyn_VarargInfo*cyc_varargs;struct Cyc_List_List*rgn_po;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*requires_clause;struct Cyc_List_List*requires_relns;struct Cyc_Absyn_Exp*ensures_clause;struct Cyc_List_List*ensures_relns;struct Cyc_List_List*ieffect;struct Cyc_List_List*oeffect;struct Cyc_List_List*throws;int reentrant;};struct Cyc_Absyn_UnknownDatatypeInfo{struct _tuple2*name;int is_extensible;};struct _union_DatatypeInfo_UnknownDatatype{int tag;struct Cyc_Absyn_UnknownDatatypeInfo val;};struct _union_DatatypeInfo_KnownDatatype{int tag;struct Cyc_Absyn_Datatypedecl**val;};union Cyc_Absyn_DatatypeInfo{struct _union_DatatypeInfo_UnknownDatatype UnknownDatatype;struct _union_DatatypeInfo_KnownDatatype KnownDatatype;};struct Cyc_Absyn_UnknownDatatypeFieldInfo{struct _tuple2*datatype_name;struct _tuple2*field_name;int is_extensible;};struct _union_DatatypeFieldInfo_UnknownDatatypefield{int tag;struct Cyc_Absyn_UnknownDatatypeFieldInfo val;};struct _tuple3{struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;};struct _union_DatatypeFieldInfo_KnownDatatypefield{int tag;struct _tuple3 val;};union Cyc_Absyn_DatatypeFieldInfo{struct _union_DatatypeFieldInfo_UnknownDatatypefield UnknownDatatypefield;struct _union_DatatypeFieldInfo_KnownDatatypefield KnownDatatypefield;};struct _tuple4{enum Cyc_Absyn_AggrKind f1;struct _tuple2*f2;struct Cyc_Core_Opt*f3;};struct _union_AggrInfo_UnknownAggr{int tag;struct _tuple4 val;};struct _union_AggrInfo_KnownAggr{int tag;struct Cyc_Absyn_Aggrdecl**val;};union Cyc_Absyn_AggrInfo{struct _union_AggrInfo_UnknownAggr UnknownAggr;struct _union_AggrInfo_KnownAggr KnownAggr;};
# 366
union Cyc_Absyn_AggrInfo Cyc_Absyn_UnknownAggr(enum Cyc_Absyn_AggrKind,struct _tuple2*,struct Cyc_Core_Opt*);struct Cyc_Absyn_ArrayInfo{void*elt_type;struct Cyc_Absyn_Tqual tq;struct Cyc_Absyn_Exp*num_elts;void*zero_term;unsigned zt_loc;};struct Cyc_Absyn_Aggr_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Enum_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;};struct Cyc_Absyn_Datatype_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;};struct Cyc_Absyn_TypeDecl{void*r;unsigned loc;};struct Cyc_Absyn_VoidCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_IntCon_Absyn_TyCon_struct{int tag;enum Cyc_Absyn_Sign f1;enum Cyc_Absyn_Size_of f2;};struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct{int tag;int f1;};struct Cyc_Absyn_RgnHandleCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_XRgnHandleCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_TagCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_HeapCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_UniqueCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_RefCntCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_AccessCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_CAccessCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_JoinCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_RgnsCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_TrueCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_FalseCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_ThinCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_FatCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct{int tag;struct _tuple2*f1;struct Cyc_Absyn_Enumdecl*f2;};struct Cyc_Absyn_AnonEnumCon_Absyn_TyCon_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_BuiltinCon_Absyn_TyCon_struct{int tag;struct _fat_ptr f1;struct Cyc_Absyn_Kind*f2;};struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_DatatypeInfo f1;};struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_DatatypeFieldInfo f1;};struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_AggrInfo f1;};struct Cyc_Absyn_AppType_Absyn_Type_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Evar_Absyn_Type_struct{int tag;struct Cyc_Core_Opt*f1;void*f2;int f3;struct Cyc_Core_Opt*f4;};struct Cyc_Absyn_VarType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Tvar*f1;};struct Cyc_Absyn_PointerType_Absyn_Type_struct{int tag;struct Cyc_Absyn_PtrInfo f1;};struct Cyc_Absyn_ArrayType_Absyn_Type_struct{int tag;struct Cyc_Absyn_ArrayInfo f1;};struct Cyc_Absyn_FnType_Absyn_Type_struct{int tag;struct Cyc_Absyn_FnInfo f1;};struct Cyc_Absyn_TupleType_Absyn_Type_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct{int tag;enum Cyc_Absyn_AggrKind f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_TypedefType_Absyn_Type_struct{int tag;struct _tuple2*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_Typedefdecl*f3;void*f4;};struct Cyc_Absyn_ValueofType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct{int tag;struct Cyc_Absyn_TypeDecl*f1;void**f2;};struct Cyc_Absyn_TypeofType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_NoTypes_Absyn_Funcparams_struct{int tag;struct Cyc_List_List*f1;unsigned f2;};struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct{int tag;struct Cyc_List_List*f1;int f2;struct Cyc_Absyn_VarargInfo*f3;void*f4;struct Cyc_List_List*f5;struct Cyc_Absyn_Exp*f6;struct Cyc_Absyn_Exp*f7;struct Cyc_List_List*f8;struct Cyc_List_List*f9;struct Cyc_List_List*f10;int f11;};
# 478 "absyn.h"
enum Cyc_Absyn_Format_Type{Cyc_Absyn_Printf_ft =0U,Cyc_Absyn_Scanf_ft =1U};struct Cyc_Absyn_Regparm_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Stdcall_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Cdecl_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Fastcall_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Noreturn_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Const_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Aligned_att_Absyn_Attribute_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Packed_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Section_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Nocommon_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Shared_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Unused_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Weak_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Dllimport_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Dllexport_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_instrument_function_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Constructor_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Destructor_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_check_memory_usage_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Format_att_Absyn_Attribute_struct{int tag;enum Cyc_Absyn_Format_Type f1;int f2;int f3;};struct Cyc_Absyn_Initializes_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Noliveunique_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Consume_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Pure_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Mode_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Alias_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Always_inline_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_throw_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Non_null_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Deprecated_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_VectorSize_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Carray_mod_Absyn_Type_modifier_struct{int tag;void*f1;unsigned f2;};struct Cyc_Absyn_ConstArray_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;unsigned f3;};struct Cyc_Absyn_Pointer_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_Absyn_PtrAtts f1;struct Cyc_Absyn_Tqual f2;};struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct{int tag;void*f1;};struct Cyc_Absyn_TypeParams_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_List_List*f1;unsigned f2;int f3;};struct Cyc_Absyn_Attributes_mod_Absyn_Type_modifier_struct{int tag;unsigned f1;struct Cyc_List_List*f2;};struct _union_Cnst_Null_c{int tag;int val;};struct _tuple5{enum Cyc_Absyn_Sign f1;char f2;};struct _union_Cnst_Char_c{int tag;struct _tuple5 val;};struct _union_Cnst_Wchar_c{int tag;struct _fat_ptr val;};struct _tuple6{enum Cyc_Absyn_Sign f1;short f2;};struct _union_Cnst_Short_c{int tag;struct _tuple6 val;};struct _tuple7{enum Cyc_Absyn_Sign f1;int f2;};struct _union_Cnst_Int_c{int tag;struct _tuple7 val;};struct _tuple8{enum Cyc_Absyn_Sign f1;long long f2;};struct _union_Cnst_LongLong_c{int tag;struct _tuple8 val;};struct _tuple9{struct _fat_ptr f1;int f2;};struct _union_Cnst_Float_c{int tag;struct _tuple9 val;};struct _union_Cnst_String_c{int tag;struct _fat_ptr val;};struct _union_Cnst_Wstring_c{int tag;struct _fat_ptr val;};union Cyc_Absyn_Cnst{struct _union_Cnst_Null_c Null_c;struct _union_Cnst_Char_c Char_c;struct _union_Cnst_Wchar_c Wchar_c;struct _union_Cnst_Short_c Short_c;struct _union_Cnst_Int_c Int_c;struct _union_Cnst_LongLong_c LongLong_c;struct _union_Cnst_Float_c Float_c;struct _union_Cnst_String_c String_c;struct _union_Cnst_Wstring_c Wstring_c;};
# 574
enum Cyc_Absyn_Primop{Cyc_Absyn_Plus =0U,Cyc_Absyn_Times =1U,Cyc_Absyn_Minus =2U,Cyc_Absyn_Div =3U,Cyc_Absyn_Mod =4U,Cyc_Absyn_Eq =5U,Cyc_Absyn_Neq =6U,Cyc_Absyn_Gt =7U,Cyc_Absyn_Lt =8U,Cyc_Absyn_Gte =9U,Cyc_Absyn_Lte =10U,Cyc_Absyn_Not =11U,Cyc_Absyn_Bitnot =12U,Cyc_Absyn_Bitand =13U,Cyc_Absyn_Bitor =14U,Cyc_Absyn_Bitxor =15U,Cyc_Absyn_Bitlshift =16U,Cyc_Absyn_Bitlrshift =17U,Cyc_Absyn_Numelts =18U};
# 581
enum Cyc_Absyn_Incrementor{Cyc_Absyn_PreInc =0U,Cyc_Absyn_PostInc =1U,Cyc_Absyn_PreDec =2U,Cyc_Absyn_PostDec =3U};struct Cyc_Absyn_VarargCallInfo{int num_varargs;struct Cyc_List_List*injectors;struct Cyc_Absyn_VarargInfo*vai;};struct Cyc_Absyn_StructField_Absyn_OffsetofField_struct{int tag;struct _fat_ptr*f1;};struct Cyc_Absyn_TupleIndex_Absyn_OffsetofField_struct{int tag;unsigned f1;};
# 599
enum Cyc_Absyn_Coercion{Cyc_Absyn_Unknown_coercion =0U,Cyc_Absyn_No_coercion =1U,Cyc_Absyn_Null_to_NonNull =2U,Cyc_Absyn_Other_coercion =3U};struct Cyc_Absyn_MallocInfo{int is_calloc;struct Cyc_Absyn_Exp*rgn;void**elt_type;struct Cyc_Absyn_Exp*num_elts;int fat_result;int inline_call;};struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct{int tag;union Cyc_Absyn_Cnst f1;};struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Pragma_e_Absyn_Raw_exp_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct{int tag;enum Cyc_Absyn_Primop f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;enum Cyc_Absyn_Incrementor f2;};struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_VarargCallInfo*f3;int f4;struct _tuple1*f5;};struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;int f2;};struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Exp*f2;int f3;enum Cyc_Absyn_Coercion f4;};struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;int f3;int f4;};struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;int f3;int f4;};struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Tuple_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;};struct _tuple10{struct _fat_ptr*f1;struct Cyc_Absyn_Tqual f2;void*f3;};struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct{int tag;struct _tuple10*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;int f4;};struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;int f3;};struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct{int tag;struct _tuple2*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct Cyc_Absyn_Aggrdecl*f4;};struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Datatypedecl*f2;struct Cyc_Absyn_Datatypefield*f3;};struct Cyc_Absyn_Enum_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_Spawn_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;struct _tuple1*f3;};struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_MallocInfo f1;};struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Core_Opt*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Stmt*f1;};struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;};struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct{int tag;int f1;struct _fat_ptr f2;struct Cyc_List_List*f3;struct Cyc_List_List*f4;struct Cyc_List_List*f5;};struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Exp{void*topt;void*r;unsigned loc;void*annot;};struct Cyc_Absyn_Skip_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;struct Cyc_Absyn_Stmt*f3;};struct _tuple11{struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct{int tag;struct _tuple11 f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Break_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Continue_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Goto_s_Absyn_Raw_stmt_struct{int tag;struct _fat_ptr*f1;};struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _tuple11 f2;struct _tuple11 f3;struct Cyc_Absyn_Stmt*f4;};struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;void*f3;};struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Switch_clause**f2;};struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Decl*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct{int tag;struct _fat_ptr*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct _tuple11 f2;};struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_List_List*f2;void*f3;};struct Cyc_Absyn_Stmt{void*r;unsigned loc;void*annot;};struct Cyc_Absyn_Wild_p_Absyn_Raw_pat_struct{int tag;};struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_AliasVar_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_TagInt_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Tuple_p_Absyn_Raw_pat_struct{int tag;struct Cyc_List_List*f1;int f2;};struct Cyc_Absyn_Pointer_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Pat*f1;};struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct{int tag;union Cyc_Absyn_AggrInfo*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;int f4;};struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;struct Cyc_List_List*f3;int f4;};struct Cyc_Absyn_Null_p_Absyn_Raw_pat_struct{int tag;};struct Cyc_Absyn_Int_p_Absyn_Raw_pat_struct{int tag;enum Cyc_Absyn_Sign f1;int f2;};struct Cyc_Absyn_Char_p_Absyn_Raw_pat_struct{int tag;char f1;};struct Cyc_Absyn_Float_p_Absyn_Raw_pat_struct{int tag;struct _fat_ptr f1;int f2;};struct Cyc_Absyn_Enum_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_AnonEnum_p_Absyn_Raw_pat_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_UnknownId_p_Absyn_Raw_pat_struct{int tag;struct _tuple2*f1;};struct Cyc_Absyn_UnknownCall_p_Absyn_Raw_pat_struct{int tag;struct _tuple2*f1;struct Cyc_List_List*f2;int f3;};struct Cyc_Absyn_Exp_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Exp*f1;};
# 771 "absyn.h"
extern struct Cyc_Absyn_Wild_p_Absyn_Raw_pat_struct Cyc_Absyn_Wild_p_val;struct Cyc_Absyn_Pat{void*r;void*topt;unsigned loc;};struct Cyc_Absyn_Switch_clause{struct Cyc_Absyn_Pat*pattern;struct Cyc_Core_Opt*pat_vars;struct Cyc_Absyn_Exp*where_clause;struct Cyc_Absyn_Stmt*body;unsigned loc;};struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct{int tag;struct _tuple2*f1;};struct Cyc_Absyn_Global_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Funname_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Param_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Local_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Pat_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Vardecl{enum Cyc_Absyn_Scope sc;struct _tuple2*name;unsigned varloc;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*initializer;void*rgn;struct Cyc_List_List*attributes;int escapes;};struct Cyc_Absyn_Fndecl{enum Cyc_Absyn_Scope sc;int is_inline;struct _tuple2*name;struct Cyc_Absyn_Stmt*body;struct Cyc_Absyn_FnInfo i;void*cached_type;struct Cyc_Core_Opt*param_vardecls;struct Cyc_Absyn_Vardecl*fn_vardecl;};struct Cyc_Absyn_Aggrfield{struct _fat_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*width;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*requires_clause;};struct Cyc_Absyn_AggrdeclImpl{struct Cyc_List_List*exist_vars;struct Cyc_List_List*rgn_po;struct Cyc_List_List*fields;int tagged;};struct Cyc_Absyn_Aggrdecl{enum Cyc_Absyn_AggrKind kind;enum Cyc_Absyn_Scope sc;struct _tuple2*name;struct Cyc_List_List*tvs;struct Cyc_Absyn_AggrdeclImpl*impl;struct Cyc_List_List*attributes;int expected_mem_kind;};struct Cyc_Absyn_Datatypefield{struct _tuple2*name;struct Cyc_List_List*typs;unsigned loc;enum Cyc_Absyn_Scope sc;};struct Cyc_Absyn_Datatypedecl{enum Cyc_Absyn_Scope sc;struct _tuple2*name;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*fields;int is_extensible;};struct Cyc_Absyn_Enumfield{struct _tuple2*name;struct Cyc_Absyn_Exp*tag;unsigned loc;};struct Cyc_Absyn_Enumdecl{enum Cyc_Absyn_Scope sc;struct _tuple2*name;struct Cyc_Core_Opt*fields;};struct Cyc_Absyn_Typedefdecl{struct _tuple2*name;struct Cyc_Absyn_Tqual tq;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*kind;void*defn;struct Cyc_List_List*atts;int extern_c;};struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Pat*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;void*f4;};struct Cyc_Absyn_Letv_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;};struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;};struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Typedefdecl*f1;};struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct{int tag;struct _fat_ptr*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct{int tag;struct _tuple2*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_ExternC_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;};struct _tuple12{unsigned f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct _tuple12*f4;};struct Cyc_Absyn_Porton_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Portoff_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Tempeston_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Tempestoff_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Decl{void*r;unsigned loc;};struct Cyc_Absyn_ArrayElement_Absyn_Designator_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_FieldName_Absyn_Designator_struct{int tag;struct _fat_ptr*f1;};extern char Cyc_Absyn_EmptyAnnot[11U];struct Cyc_Absyn_EmptyAnnot_Absyn_AbsynAnnot_struct{char*tag;};
# 962
struct Cyc_Absyn_Tqual Cyc_Absyn_const_tqual(unsigned);
struct Cyc_Absyn_Tqual Cyc_Absyn_empty_tqual(unsigned);
# 979
void*Cyc_Absyn_new_evar(struct Cyc_Core_Opt*k,struct Cyc_Core_Opt*tenv);
# 991
extern void*Cyc_Absyn_heap_rgn_type;extern void*Cyc_Absyn_unique_rgn_type;extern void*Cyc_Absyn_refcnt_rgn_type;
# 995
extern void*Cyc_Absyn_false_type;
# 997
extern void*Cyc_Absyn_void_type;void*Cyc_Absyn_rgn_handle_type(void*);
# 1014
void*Cyc_Absyn_exn_type();
# 1038
void*Cyc_Absyn_at_type(void*t,void*rgn,struct Cyc_Absyn_Tqual,void*zero_term);
# 1050
void*Cyc_Absyn_array_type(void*elt_type,struct Cyc_Absyn_Tqual tq,struct Cyc_Absyn_Exp*num_elts,void*zero_term,unsigned ztloc);
# 1055
void*Cyc_Absyn_aggr_type(union Cyc_Absyn_AggrInfo,struct Cyc_List_List*args);
# 1058
struct Cyc_Absyn_Exp*Cyc_Absyn_new_exp(void*,unsigned);
# 1068
struct Cyc_Absyn_Exp*Cyc_Absyn_uint_exp(unsigned,unsigned);
# 1075
struct Cyc_Absyn_Exp*Cyc_Absyn_varb_exp(void*,unsigned);
# 1127
struct Cyc_Absyn_Stmt*Cyc_Absyn_new_stmt(void*,unsigned);
struct Cyc_Absyn_Stmt*Cyc_Absyn_skip_stmt(unsigned);
struct Cyc_Absyn_Stmt*Cyc_Absyn_exp_stmt(struct Cyc_Absyn_Exp*,unsigned);
# 1140
struct Cyc_Absyn_Stmt*Cyc_Absyn_decl_stmt(struct Cyc_Absyn_Decl*,struct Cyc_Absyn_Stmt*,unsigned);
# 1149
struct Cyc_Absyn_Pat*Cyc_Absyn_new_pat(void*,unsigned);
# 1154
struct Cyc_Absyn_Decl*Cyc_Absyn_let_decl(struct Cyc_Absyn_Pat*,struct Cyc_Absyn_Exp*,unsigned);
# 1158
struct Cyc_Absyn_Vardecl*Cyc_Absyn_new_vardecl(unsigned varloc,struct _tuple2*,void*,struct Cyc_Absyn_Exp*init);
# 1213
struct _fat_ptr Cyc_Absyn_attribute2string(void*);
# 1277
struct _fat_ptr Cyc_Absyn_exnstr();
# 1285
int Cyc_Absyn_get_debug();
# 1287
int Cyc_Absyn_is_xrgn_tvar(struct Cyc_Absyn_Tvar*tv);
int Cyc_Absyn_is_xrgn(void*r);
# 1409
void Cyc_Absyn_forgive_global_set(int);
int Cyc_Absyn_no_side_effects_exp(struct Cyc_Absyn_Exp*);struct Cyc_Absynpp_Params{int expand_typedefs;int qvar_to_Cids;int add_cyc_prefix;int to_VC;int decls_first;int rewrite_temp_tvars;int print_all_tvars;int print_all_kinds;int print_all_effects;int print_using_stmts;int print_externC_stmts;int print_full_evars;int print_zeroterm;int generate_line_directives;int use_curr_namespace;struct Cyc_List_List*curr_namespace;};
# 62 "absynpp.h"
struct _fat_ptr Cyc_Absynpp_typ2string(void*);
# 27 "unify.h"
void Cyc_Unify_explain_failure();
# 29
int Cyc_Unify_unify(void*,void*);struct _union_RelnOp_RConst{int tag;unsigned val;};struct _union_RelnOp_RVar{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RNumelts{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RType{int tag;void*val;};struct _union_RelnOp_RParam{int tag;unsigned val;};struct _union_RelnOp_RParamNumelts{int tag;unsigned val;};struct _union_RelnOp_RReturn{int tag;unsigned val;};union Cyc_Relations_RelnOp{struct _union_RelnOp_RConst RConst;struct _union_RelnOp_RVar RVar;struct _union_RelnOp_RNumelts RNumelts;struct _union_RelnOp_RType RType;struct _union_RelnOp_RParam RParam;struct _union_RelnOp_RParamNumelts RParamNumelts;struct _union_RelnOp_RReturn RReturn;};
# 50 "relations-ap.h"
enum Cyc_Relations_Relation{Cyc_Relations_Req =0U,Cyc_Relations_Rneq =1U,Cyc_Relations_Rlte =2U,Cyc_Relations_Rlt =3U};struct Cyc_Relations_Reln{union Cyc_Relations_RelnOp rop1;enum Cyc_Relations_Relation relation;union Cyc_Relations_RelnOp rop2;};struct Cyc_RgnOrder_RgnPO;extern char Cyc_Tcenv_Env_error[10U];struct Cyc_Tcenv_Env_error_exn_struct{char*tag;};struct Cyc_Tcenv_Genv{struct Cyc_Dict_Dict aggrdecls;struct Cyc_Dict_Dict datatypedecls;struct Cyc_Dict_Dict enumdecls;struct Cyc_Dict_Dict typedefs;struct Cyc_Dict_Dict ordinaries;};struct Cyc_Tcenv_Fenv;struct Cyc_Tcenv_Tenv{struct Cyc_List_List*ns;struct Cyc_Tcenv_Genv*ae;struct Cyc_Tcenv_Fenv*le;int allow_valueof: 1;int in_extern_c_include: 1;int in_tempest: 1;int tempest_generalize: 1;};
# 70 "tcenv.h"
struct Cyc_Tcenv_Fenv*Cyc_Tcenv_nested_fenv(unsigned,struct Cyc_Tcenv_Fenv*old_fenv,struct Cyc_Absyn_Fndecl*new_fn,void*);
# 90
enum Cyc_Tcenv_NewStatus{Cyc_Tcenv_NoneNew =0U,Cyc_Tcenv_InNew =1U,Cyc_Tcenv_InNewAggr =2U};
# 98
void*Cyc_Tcenv_return_typ(struct Cyc_Tcenv_Tenv*);
# 100
struct Cyc_List_List*Cyc_Tcenv_lookup_type_vars(struct Cyc_Tcenv_Tenv*);
# 102
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_add_type_vars(unsigned,struct Cyc_Tcenv_Tenv*,struct Cyc_List_List*);
# 104
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_set_fallthru(struct Cyc_Tcenv_Tenv*,struct Cyc_List_List*new_tvs,struct Cyc_List_List*vds,struct Cyc_Absyn_Switch_clause*clause);
# 108
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_clear_fallthru(struct Cyc_Tcenv_Tenv*);
# 126
int Cyc_Tcenv_in_stmt_exp(struct Cyc_Tcenv_Tenv*);struct _tuple13{struct Cyc_Absyn_Switch_clause*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;};
# 129
const struct _tuple13*const Cyc_Tcenv_process_fallthru(struct Cyc_Tcenv_Tenv*,struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Switch_clause***);
# 132
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_new_block(unsigned,struct Cyc_Tcenv_Tenv*);
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_new_named_block(unsigned,struct Cyc_Tcenv_Tenv*,struct Cyc_Absyn_Tvar*);
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_new_outlives_constraints(struct Cyc_Tcenv_Tenv*,struct Cyc_List_List*,unsigned);
# 139
void*Cyc_Tcenv_curr_rgn(struct Cyc_Tcenv_Tenv*);
# 141
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_add_region(struct Cyc_Tcenv_Tenv*,void*,int opened);
# 150
void Cyc_Tcenv_check_delayed_effects(struct Cyc_Tcenv_Tenv*);
void Cyc_Tcenv_check_delayed_constraints(struct Cyc_Tcenv_Tenv*);
# 156
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_new_outlives_constraint_special(struct Cyc_Tcenv_Tenv*te,void*xparent,void*child);
# 30 "tcutil.h"
void*Cyc_Tcutil_impos(struct _fat_ptr,struct _fat_ptr);
void Cyc_Tcutil_terr(unsigned,struct _fat_ptr,struct _fat_ptr);
void Cyc_Tcutil_warn(unsigned,struct _fat_ptr,struct _fat_ptr);
# 39
int Cyc_Tcutil_is_void_type(void*);
# 41
int Cyc_Tcutil_is_any_int_type(void*);
int Cyc_Tcutil_is_any_float_type(void*);
# 110
void*Cyc_Tcutil_compress(void*);
# 116
int Cyc_Tcutil_coerce_arg(struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Exp*,void*,int*alias_coercion);
int Cyc_Tcutil_coerce_assign(struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Exp*,void*);
# 138
extern struct Cyc_Absyn_Kind Cyc_Tcutil_xrk;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_rk;
# 141
extern struct Cyc_Absyn_Kind Cyc_Tcutil_bk;
# 148
extern struct Cyc_Absyn_Kind Cyc_Tcutil_trk;
# 151
extern struct Cyc_Absyn_Kind Cyc_Tcutil_tmk;
# 178
void*Cyc_Tcutil_kind_to_bound(struct Cyc_Absyn_Kind*k);
# 189
void*Cyc_Tcutil_substitute(struct Cyc_List_List*,void*);
# 206
void*Cyc_Tcutil_fndecl2type(struct Cyc_Absyn_Fndecl*);struct _tuple14{struct Cyc_Absyn_Tvar*f1;void*f2;};
# 210
struct _tuple14*Cyc_Tcutil_make_inst_var(struct Cyc_List_List*,struct Cyc_Absyn_Tvar*);
# 245
int Cyc_Tcutil_is_noalias_path(struct Cyc_Absyn_Exp*);
# 250
int Cyc_Tcutil_is_noalias_pointer_or_aggr(void*);
# 263
int Cyc_Tcutil_new_tvar_id();
# 278
int Cyc_Tcutil_is_const_exp(struct Cyc_Absyn_Exp*);
# 285
int Cyc_Tcutil_extract_const_from_typedef(unsigned,int declared_const,void*);
# 289
struct Cyc_List_List*Cyc_Tcutil_transfer_fn_type_atts(void*,struct Cyc_List_List*);
# 299
struct Cyc_List_List*Cyc_Tcutil_filter_nulls(struct Cyc_List_List*);
# 35 "tctyp.h"
void Cyc_Tctyp_check_fndecl_valid_type(unsigned,struct Cyc_Tcenv_Tenv*,struct Cyc_Absyn_Fndecl*);
# 44 "tctyp.h"
void Cyc_Tctyp_check_type(unsigned,struct Cyc_Tcenv_Tenv*,struct Cyc_List_List*bound_tvars,struct Cyc_Absyn_Kind*k,int allow_evars,int allow_abs_aggr,void*);
# 28 "tcexp.h"
void*Cyc_Tcexp_tcExp(struct Cyc_Tcenv_Tenv*,void**,struct Cyc_Absyn_Exp*);
void*Cyc_Tcexp_tcExpInitializer(struct Cyc_Tcenv_Tenv*,void**,struct Cyc_Absyn_Exp*);
void Cyc_Tcexp_tcTest(struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Exp*e,struct _fat_ptr msg_part);struct Cyc_Tcpat_TcPatResult{struct _tuple1*tvars_and_bounds_opt;struct Cyc_List_List*patvars;};
# 53 "tcpat.h"
struct Cyc_Tcpat_TcPatResult Cyc_Tcpat_tcPat(struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Pat*p,void**topt,struct Cyc_Absyn_Exp*pat_var_exp);
# 55
void Cyc_Tcpat_check_pat_regions(struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Pat*p,struct Cyc_List_List*patvars);struct Cyc_Tcpat_WhereTest_Tcpat_PatTest_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Tcpat_EqNull_Tcpat_PatTest_struct{int tag;};struct Cyc_Tcpat_NeqNull_Tcpat_PatTest_struct{int tag;};struct Cyc_Tcpat_EqEnum_Tcpat_PatTest_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Tcpat_EqAnonEnum_Tcpat_PatTest_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Tcpat_EqFloat_Tcpat_PatTest_struct{int tag;struct _fat_ptr f1;int f2;};struct Cyc_Tcpat_EqConst_Tcpat_PatTest_struct{int tag;unsigned f1;};struct Cyc_Tcpat_EqDatatypeTag_Tcpat_PatTest_struct{int tag;int f1;struct Cyc_Absyn_Datatypedecl*f2;struct Cyc_Absyn_Datatypefield*f3;};struct Cyc_Tcpat_EqTaggedUnion_Tcpat_PatTest_struct{int tag;struct _fat_ptr*f1;int f2;};struct Cyc_Tcpat_EqExtensibleDatatype_Tcpat_PatTest_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;};struct Cyc_Tcpat_Dummy_Tcpat_Access_struct{int tag;};struct Cyc_Tcpat_Deref_Tcpat_Access_struct{int tag;};struct Cyc_Tcpat_TupleField_Tcpat_Access_struct{int tag;unsigned f1;};struct Cyc_Tcpat_DatatypeField_Tcpat_Access_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;unsigned f3;};struct Cyc_Tcpat_AggrField_Tcpat_Access_struct{int tag;int f1;struct _fat_ptr*f2;};struct _union_PatOrWhere_pattern{int tag;struct Cyc_Absyn_Pat*val;};struct _union_PatOrWhere_where_clause{int tag;struct Cyc_Absyn_Exp*val;};union Cyc_Tcpat_PatOrWhere{struct _union_PatOrWhere_pattern pattern;struct _union_PatOrWhere_where_clause where_clause;};struct Cyc_Tcpat_PathNode{union Cyc_Tcpat_PatOrWhere orig_pat;void*access;};struct Cyc_Tcpat_Rhs{int used;unsigned pat_loc;struct Cyc_Absyn_Stmt*rhs;};struct Cyc_Tcpat_Failure_Tcpat_Decision_struct{int tag;void*f1;};struct Cyc_Tcpat_Success_Tcpat_Decision_struct{int tag;struct Cyc_Tcpat_Rhs*f1;};struct Cyc_Tcpat_SwitchDec_Tcpat_Decision_struct{int tag;struct Cyc_List_List*f1;struct Cyc_List_List*f2;void*f3;};
# 109
void Cyc_Tcpat_check_switch_exhaustive(unsigned,struct Cyc_List_List*,void**);
# 111
int Cyc_Tcpat_check_let_pat_exhaustive(unsigned,struct Cyc_Tcenv_Tenv*,struct Cyc_Absyn_Pat*p,void**);
# 113
void Cyc_Tcpat_check_catch_overlap(unsigned,struct Cyc_Tcenv_Tenv*,struct Cyc_List_List*,void**);struct Cyc_Set_Set;extern char Cyc_Set_Absent[7U];struct Cyc_Set_Absent_exn_struct{char*tag;};struct Cyc_Hashtable_Table;struct Cyc_JumpAnalysis_Jump_Anal_Result{struct Cyc_Hashtable_Table*pop_tables;struct Cyc_Hashtable_Table*succ_tables;struct Cyc_Hashtable_Table*pat_pop_tables;};
# 41 "cf_flowinfo.h"
int Cyc_CfFlowInfo_anal_error;struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_CfFlowInfo_MallocPt_CfFlowInfo_Root_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;};struct Cyc_CfFlowInfo_InitParam_CfFlowInfo_Root_struct{int tag;int f1;void*f2;};struct Cyc_CfFlowInfo_Dot_CfFlowInfo_PathCon_struct{int tag;int f1;};struct Cyc_CfFlowInfo_Star_CfFlowInfo_PathCon_struct{int tag;};struct Cyc_CfFlowInfo_Place{void*root;struct Cyc_List_List*path;};
# 74
enum Cyc_CfFlowInfo_InitLevel{Cyc_CfFlowInfo_NoneIL =0U,Cyc_CfFlowInfo_AllIL =1U};extern char Cyc_CfFlowInfo_IsZero[7U];struct Cyc_CfFlowInfo_IsZero_Absyn_AbsynAnnot_struct{char*tag;};extern char Cyc_CfFlowInfo_NotZero[8U];struct Cyc_CfFlowInfo_NotZero_Absyn_AbsynAnnot_struct{char*tag;struct Cyc_List_List*f1;};extern char Cyc_CfFlowInfo_UnknownZ[9U];struct Cyc_CfFlowInfo_UnknownZ_Absyn_AbsynAnnot_struct{char*tag;struct Cyc_List_List*f1;};struct _union_AbsLVal_PlaceL{int tag;struct Cyc_CfFlowInfo_Place*val;};struct _union_AbsLVal_UnknownL{int tag;int val;};union Cyc_CfFlowInfo_AbsLVal{struct _union_AbsLVal_PlaceL PlaceL;struct _union_AbsLVal_UnknownL UnknownL;};struct Cyc_CfFlowInfo_UnionRInfo{int is_union;int fieldnum;};struct Cyc_CfFlowInfo_Zero_CfFlowInfo_AbsRVal_struct{int tag;};struct Cyc_CfFlowInfo_NotZeroAll_CfFlowInfo_AbsRVal_struct{int tag;};struct Cyc_CfFlowInfo_UnknownR_CfFlowInfo_AbsRVal_struct{int tag;enum Cyc_CfFlowInfo_InitLevel f1;};struct Cyc_CfFlowInfo_Esc_CfFlowInfo_AbsRVal_struct{int tag;enum Cyc_CfFlowInfo_InitLevel f1;};struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct{int tag;struct Cyc_CfFlowInfo_Place*f1;};struct Cyc_CfFlowInfo_UniquePtr_CfFlowInfo_AbsRVal_struct{int tag;void*f1;};struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct{int tag;struct Cyc_CfFlowInfo_UnionRInfo f1;struct _fat_ptr f2;};struct Cyc_CfFlowInfo_Consumed_CfFlowInfo_AbsRVal_struct{int tag;struct Cyc_Absyn_Exp*f1;int f2;void*f3;};struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct{int tag;struct Cyc_Absyn_Vardecl*f1;void*f2;};struct _union_FlowInfo_BottomFL{int tag;int val;};struct _tuple15{struct Cyc_Dict_Dict f1;struct Cyc_List_List*f2;};struct _union_FlowInfo_ReachableFL{int tag;struct _tuple15 val;};union Cyc_CfFlowInfo_FlowInfo{struct _union_FlowInfo_BottomFL BottomFL;struct _union_FlowInfo_ReachableFL ReachableFL;};struct Cyc_CfFlowInfo_FlowEnv{void*zero;void*notzeroall;void*unknown_none;void*unknown_all;void*esc_none;void*esc_all;struct Cyc_Dict_Dict mt_flowdict;struct Cyc_CfFlowInfo_Place*dummy_place;};
# 26 "tcstmt.h"
void Cyc_Tcstmt_tcStmt(struct Cyc_Tcenv_Tenv*,struct Cyc_Absyn_Stmt*,int new_block);
void*Cyc_Tcstmt_tcFndecl_valid_type(struct Cyc_Tcenv_Tenv*te,unsigned loc,struct Cyc_Absyn_Fndecl*fd);
# 30
void Cyc_Tcstmt_tcFndecl_check_body(unsigned loc,struct Cyc_Tcenv_Tenv*te,void*,struct Cyc_Absyn_Fndecl*fd);
# 51 "tcstmt.cyc"
static void Cyc_Tcstmt_simplify_unused_result_exp(struct Cyc_Absyn_Exp*e){
void*_stmttmp0=e->r;void*_tmp0=_stmttmp0;struct Cyc_Absyn_Exp*_tmp1;struct Cyc_Absyn_Exp*_tmp2;if(((struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*)_tmp0)->tag == 5U)switch(((struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*)_tmp0)->f2){case Cyc_Absyn_PostInc: _LL1: _tmp2=((struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*)_tmp0)->f1;_LL2: {struct Cyc_Absyn_Exp*e1=_tmp2;
({void*_tmp1FD=(void*)({struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*_tmp3=_cycalloc(sizeof(*_tmp3));_tmp3->tag=5U,_tmp3->f1=e1,_tmp3->f2=Cyc_Absyn_PreInc;_tmp3;});e->r=_tmp1FD;});goto _LL0;}case Cyc_Absyn_PostDec: _LL3: _tmp1=((struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*)_tmp0)->f1;_LL4: {struct Cyc_Absyn_Exp*e1=_tmp1;
({void*_tmp1FE=(void*)({struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*_tmp4=_cycalloc(sizeof(*_tmp4));_tmp4->tag=5U,_tmp4->f1=e1,_tmp4->f2=Cyc_Absyn_PreDec;_tmp4;});e->r=_tmp1FE;});goto _LL0;}default: goto _LL5;}else{_LL5: _LL6:
 goto _LL0;}_LL0:;}
# 60
static int Cyc_Tcstmt_is_var_exp(struct Cyc_Absyn_Exp*e){
while(1){
void*_stmttmp1=e->r;void*_tmp6=_stmttmp1;struct Cyc_Absyn_Exp*_tmp7;struct Cyc_Absyn_Exp*_tmp8;switch(*((int*)_tmp6)){case 1U: _LL1: _LL2:
 return 1;case 12U: _LL3: _tmp8=((struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*)_tmp6)->f1;_LL4: {struct Cyc_Absyn_Exp*e2=_tmp8;
_tmp7=e2;goto _LL6;}case 13U: _LL5: _tmp7=((struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*)_tmp6)->f1;_LL6: {struct Cyc_Absyn_Exp*e2=_tmp7;
e=e2;continue;}default: _LL7: _LL8:
# 67
 return 0;}_LL0:;}}struct _tuple16{struct Cyc_List_List*f1;struct Cyc_Absyn_Exp*f2;};
# 77
static int Cyc_Tcstmt_noassign_exp(struct Cyc_Absyn_Exp*e){
void*_stmttmp2=e->r;void*_tmpA=_stmttmp2;struct Cyc_List_List*_tmpB;struct Cyc_List_List*_tmpC;struct Cyc_List_List*_tmpD;struct Cyc_List_List*_tmpE;struct Cyc_List_List*_tmpF;struct Cyc_List_List*_tmp10;struct Cyc_List_List*_tmp11;struct Cyc_List_List*_tmp12;struct Cyc_Absyn_Exp*_tmp14;struct Cyc_Absyn_Exp*_tmp13;struct Cyc_Absyn_Exp*_tmp16;struct Cyc_Absyn_Exp*_tmp15;struct Cyc_Absyn_Exp*_tmp17;struct Cyc_Absyn_Exp*_tmp18;struct Cyc_Absyn_Exp*_tmp19;struct Cyc_Absyn_Exp*_tmp1A;struct Cyc_Absyn_Exp*_tmp1B;struct Cyc_Absyn_Exp*_tmp1C;struct Cyc_Absyn_Exp*_tmp1D;struct Cyc_Absyn_Exp*_tmp1E;struct Cyc_Absyn_Exp*_tmp1F;struct Cyc_Absyn_Exp*_tmp20;struct Cyc_Absyn_Exp*_tmp21;struct Cyc_Absyn_Exp*_tmp22;struct Cyc_Absyn_Exp*_tmp24;struct Cyc_Absyn_Exp*_tmp23;struct Cyc_Absyn_Exp*_tmp26;struct Cyc_Absyn_Exp*_tmp25;struct Cyc_Absyn_Exp*_tmp28;struct Cyc_Absyn_Exp*_tmp27;struct Cyc_Absyn_Exp*_tmp2A;struct Cyc_Absyn_Exp*_tmp29;struct Cyc_Absyn_Exp*_tmp2C;struct Cyc_Absyn_Exp*_tmp2B;struct Cyc_Absyn_Exp*_tmp2E;struct Cyc_Absyn_Exp*_tmp2D;struct Cyc_Absyn_Exp*_tmp31;struct Cyc_Absyn_Exp*_tmp30;struct Cyc_Absyn_Exp*_tmp2F;switch(*((int*)_tmpA)){case 10U: _LL1: _LL2:
 goto _LL4;case 4U: _LL3: _LL4:
 goto _LL6;case 36U: _LL5: _LL6:
 goto _LL8;case 41U: _LL7: _LL8:
 goto _LLA;case 38U: _LL9: _LLA:
 goto _LLC;case 5U: _LLB: _LLC:
 return 0;case 40U: _LLD: _LLE:
# 86
 goto _LL10;case 0U: _LLF: _LL10:
 goto _LL12;case 1U: _LL11: _LL12:
 goto _LL14;case 17U: _LL13: _LL14:
 goto _LL16;case 19U: _LL15: _LL16:
 goto _LL18;case 32U: _LL17: _LL18:
 goto _LL1A;case 33U: _LL19: _LL1A:
 goto _LL1C;case 2U: _LL1B: _LL1C:
 return 1;case 6U: _LL1D: _tmp2F=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmpA)->f1;_tmp30=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmpA)->f2;_tmp31=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmpA)->f3;_LL1E: {struct Cyc_Absyn_Exp*e1=_tmp2F;struct Cyc_Absyn_Exp*e2=_tmp30;struct Cyc_Absyn_Exp*e3=_tmp31;
# 96
if(!({Cyc_Tcstmt_noassign_exp(e1);}))return 0;_tmp2D=e2;_tmp2E=e3;goto _LL20;}case 27U: _LL1F: _tmp2D=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmpA)->f2;_tmp2E=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmpA)->f3;_LL20: {struct Cyc_Absyn_Exp*e1=_tmp2D;struct Cyc_Absyn_Exp*e2=_tmp2E;
# 98
_tmp2B=e1;_tmp2C=e2;goto _LL22;}case 7U: _LL21: _tmp2B=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmpA)->f1;_tmp2C=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmpA)->f2;_LL22: {struct Cyc_Absyn_Exp*e1=_tmp2B;struct Cyc_Absyn_Exp*e2=_tmp2C;
_tmp29=e1;_tmp2A=e2;goto _LL24;}case 8U: _LL23: _tmp29=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmpA)->f1;_tmp2A=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmpA)->f2;_LL24: {struct Cyc_Absyn_Exp*e1=_tmp29;struct Cyc_Absyn_Exp*e2=_tmp2A;
_tmp27=e1;_tmp28=e2;goto _LL26;}case 23U: _LL25: _tmp27=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmpA)->f1;_tmp28=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmpA)->f2;_LL26: {struct Cyc_Absyn_Exp*e1=_tmp27;struct Cyc_Absyn_Exp*e2=_tmp28;
_tmp25=e1;_tmp26=e2;goto _LL28;}case 34U: _LL27: _tmp25=((struct Cyc_Absyn_Spawn_e_Absyn_Raw_exp_struct*)_tmpA)->f1;_tmp26=((struct Cyc_Absyn_Spawn_e_Absyn_Raw_exp_struct*)_tmpA)->f2;_LL28: {struct Cyc_Absyn_Exp*e1=_tmp25;struct Cyc_Absyn_Exp*e2=_tmp26;
_tmp23=e1;_tmp24=e2;goto _LL2A;}case 9U: _LL29: _tmp23=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmpA)->f1;_tmp24=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmpA)->f2;_LL2A: {struct Cyc_Absyn_Exp*e1=_tmp23;struct Cyc_Absyn_Exp*e2=_tmp24;
return({Cyc_Tcstmt_noassign_exp(e1);})&&({Cyc_Tcstmt_noassign_exp(e2);});}case 42U: _LL2B: _tmp22=((struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct*)_tmpA)->f1;_LL2C: {struct Cyc_Absyn_Exp*e=_tmp22;
# 105
_tmp21=e;goto _LL2E;}case 39U: _LL2D: _tmp21=((struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct*)_tmpA)->f1;_LL2E: {struct Cyc_Absyn_Exp*e=_tmp21;
_tmp20=e;goto _LL30;}case 11U: _LL2F: _tmp20=((struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct*)_tmpA)->f1;_LL30: {struct Cyc_Absyn_Exp*e=_tmp20;
_tmp1F=e;goto _LL32;}case 12U: _LL31: _tmp1F=((struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*)_tmpA)->f1;_LL32: {struct Cyc_Absyn_Exp*e=_tmp1F;
_tmp1E=e;goto _LL34;}case 13U: _LL33: _tmp1E=((struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*)_tmpA)->f1;_LL34: {struct Cyc_Absyn_Exp*e=_tmp1E;
_tmp1D=e;goto _LL36;}case 14U: _LL35: _tmp1D=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmpA)->f2;_LL36: {struct Cyc_Absyn_Exp*e=_tmp1D;
_tmp1C=e;goto _LL38;}case 18U: _LL37: _tmp1C=((struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct*)_tmpA)->f1;_LL38: {struct Cyc_Absyn_Exp*e=_tmp1C;
_tmp1B=e;goto _LL3A;}case 20U: _LL39: _tmp1B=((struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_tmpA)->f1;_LL3A: {struct Cyc_Absyn_Exp*e=_tmp1B;
_tmp1A=e;goto _LL3C;}case 21U: _LL3B: _tmp1A=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmpA)->f1;_LL3C: {struct Cyc_Absyn_Exp*e=_tmp1A;
_tmp19=e;goto _LL3E;}case 22U: _LL3D: _tmp19=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmpA)->f1;_LL3E: {struct Cyc_Absyn_Exp*e=_tmp19;
_tmp18=e;goto _LL40;}case 28U: _LL3F: _tmp18=((struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*)_tmpA)->f1;_LL40: {struct Cyc_Absyn_Exp*e=_tmp18;
_tmp17=e;goto _LL42;}case 15U: _LL41: _tmp17=((struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct*)_tmpA)->f1;_LL42: {struct Cyc_Absyn_Exp*e=_tmp17;
return({Cyc_Tcstmt_noassign_exp(e);});}case 35U: _LL43: _tmp15=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmpA)->f1).rgn;_tmp16=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmpA)->f1).num_elts;_LL44: {struct Cyc_Absyn_Exp*eopt=_tmp15;struct Cyc_Absyn_Exp*e=_tmp16;
# 119
_tmp13=eopt;_tmp14=e;goto _LL46;}case 16U: _LL45: _tmp13=((struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct*)_tmpA)->f1;_tmp14=((struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct*)_tmpA)->f2;_LL46: {struct Cyc_Absyn_Exp*eopt=_tmp13;struct Cyc_Absyn_Exp*e=_tmp14;
# 121
if(!({Cyc_Tcstmt_noassign_exp(e);}))return 0;if(eopt != 0)
return({Cyc_Tcstmt_noassign_exp(eopt);});else{
return 1;}}case 3U: _LL47: _tmp12=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmpA)->f2;_LL48: {struct Cyc_List_List*es=_tmp12;
# 125
_tmp11=es;goto _LL4A;}case 31U: _LL49: _tmp11=((struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*)_tmpA)->f1;_LL4A: {struct Cyc_List_List*es=_tmp11;
_tmp10=es;goto _LL4C;}case 24U: _LL4B: _tmp10=((struct Cyc_Absyn_Tuple_e_Absyn_Raw_exp_struct*)_tmpA)->f1;_LL4C: {struct Cyc_List_List*es=_tmp10;
return({({int(*_tmp1FF)(int(*pred)(struct Cyc_Absyn_Exp*),struct Cyc_List_List*x)=({int(*_tmp32)(int(*pred)(struct Cyc_Absyn_Exp*),struct Cyc_List_List*x)=(int(*)(int(*pred)(struct Cyc_Absyn_Exp*),struct Cyc_List_List*x))Cyc_List_forall;_tmp32;});_tmp1FF(Cyc_Tcstmt_noassign_exp,es);});});}case 37U: _LL4D: _tmpF=((struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*)_tmpA)->f2;_LL4E: {struct Cyc_List_List*dles=_tmpF;
# 129
_tmpE=dles;goto _LL50;}case 29U: _LL4F: _tmpE=((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_tmpA)->f3;_LL50: {struct Cyc_List_List*dles=_tmpE;
_tmpD=dles;goto _LL52;}case 30U: _LL51: _tmpD=((struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct*)_tmpA)->f2;_LL52: {struct Cyc_List_List*dles=_tmpD;
_tmpC=dles;goto _LL54;}case 26U: _LL53: _tmpC=((struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*)_tmpA)->f1;_LL54: {struct Cyc_List_List*dles=_tmpC;
_tmpB=dles;goto _LL56;}default: _LL55: _tmpB=((struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct*)_tmpA)->f2;_LL56: {struct Cyc_List_List*dles=_tmpB;
# 134
for(0;dles != 0;dles=dles->tl){
struct _tuple16*_stmttmp3=(struct _tuple16*)dles->hd;struct Cyc_Absyn_Exp*_tmp33;_LL58: _tmp33=_stmttmp3->f2;_LL59: {struct Cyc_Absyn_Exp*e=_tmp33;
if(!({Cyc_Tcstmt_noassign_exp(e);}))return 0;}}
# 138
return 1;}}_LL0:;}struct _tuple17{struct Cyc_Absyn_Tvar*f1;int f2;};struct _tuple18{struct Cyc_Absyn_Vardecl**f1;struct Cyc_Absyn_Exp*f2;};
# 143
static void Cyc_Tcstmt_pattern_synth(unsigned loc,struct Cyc_Tcenv_Tenv*te,struct Cyc_Tcpat_TcPatResult pat_res,struct Cyc_Absyn_Stmt*s,struct Cyc_Absyn_Exp*where_opt,int new_block){
# 145
struct Cyc_List_List*_tmp36;struct _tuple1*_tmp35;_LL1: _tmp35=pat_res.tvars_and_bounds_opt;_tmp36=pat_res.patvars;_LL2: {struct _tuple1*p=_tmp35;struct Cyc_List_List*vse=_tmp36;
struct Cyc_List_List*vs=({({struct Cyc_List_List*(*_tmp201)(struct Cyc_Absyn_Vardecl**(*f)(struct _tuple18*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp44)(struct Cyc_Absyn_Vardecl**(*f)(struct _tuple18*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_Vardecl**(*f)(struct _tuple18*),struct Cyc_List_List*x))Cyc_List_map;_tmp44;});struct Cyc_Absyn_Vardecl**(*_tmp200)(struct _tuple18*)=({struct Cyc_Absyn_Vardecl**(*_tmp45)(struct _tuple18*)=(struct Cyc_Absyn_Vardecl**(*)(struct _tuple18*))Cyc_Core_fst;_tmp45;});_tmp201(_tmp200,vse);});});
struct Cyc_List_List*tvs=p == 0?0:({({struct Cyc_List_List*(*_tmp203)(struct Cyc_Absyn_Tvar*(*f)(struct _tuple17*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp42)(struct Cyc_Absyn_Tvar*(*f)(struct _tuple17*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_Tvar*(*f)(struct _tuple17*),struct Cyc_List_List*x))Cyc_List_map;_tmp42;});struct Cyc_Absyn_Tvar*(*_tmp202)(struct _tuple17*)=({struct Cyc_Absyn_Tvar*(*_tmp43)(struct _tuple17*)=(struct Cyc_Absyn_Tvar*(*)(struct _tuple17*))Cyc_Core_fst;_tmp43;});_tmp203(_tmp202,(*p).f1);});});
struct Cyc_List_List*bds=p == 0?0:(*p).f2;
struct Cyc_List_List*rgns=p == 0?0:({struct Cyc_List_List*(*_tmp3B)(struct Cyc_Absyn_Tvar*(*f)(struct _tuple17*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp3C)(struct Cyc_Absyn_Tvar*(*f)(struct _tuple17*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_Tvar*(*f)(struct _tuple17*),struct Cyc_List_List*x))Cyc_List_map;_tmp3C;});struct Cyc_Absyn_Tvar*(*_tmp3D)(struct _tuple17*)=({struct Cyc_Absyn_Tvar*(*_tmp3E)(struct _tuple17*)=(struct Cyc_Absyn_Tvar*(*)(struct _tuple17*))Cyc_Core_fst;_tmp3E;});struct Cyc_List_List*_tmp3F=({({struct Cyc_List_List*(*_tmp205)(int(*f)(struct _tuple17*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp40)(int(*f)(struct _tuple17*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(int(*f)(struct _tuple17*),struct Cyc_List_List*x))Cyc_List_filter;_tmp40;});int(*_tmp204)(struct _tuple17*)=({int(*_tmp41)(struct _tuple17*)=(int(*)(struct _tuple17*))Cyc_Core_snd;_tmp41;});_tmp205(_tmp204,(*p).f1);});});_tmp3B(_tmp3D,_tmp3F);});
struct Cyc_Tcenv_Tenv*te2=({Cyc_Tcenv_add_type_vars(loc,te,tvs);});
for(0;rgns != 0;rgns=rgns->tl){
te2=({({struct Cyc_Tcenv_Tenv*_tmp206=te2;Cyc_Tcenv_add_region(_tmp206,(void*)({struct Cyc_Absyn_VarType_Absyn_Type_struct*_tmp37=_cycalloc(sizeof(*_tmp37));_tmp37->tag=2U,_tmp37->f1=(struct Cyc_Absyn_Tvar*)rgns->hd;_tmp37;}),0);});});}
te2=({Cyc_Tcenv_new_outlives_constraints(te2,bds,loc);});
if(new_block)
te2=({Cyc_Tcenv_new_block(loc,te2);});{
# 154
void*ropt=({Cyc_Tcenv_curr_rgn(te2);});
# 157
{struct Cyc_List_List*vs2=vs;for(0;vs2 != 0;vs2=vs2->tl){
if((struct Cyc_Absyn_Vardecl**)vs2->hd != 0)
(*((struct Cyc_Absyn_Vardecl**)_check_null((struct Cyc_Absyn_Vardecl**)vs2->hd)))->rgn=ropt;}}
# 157
if(where_opt != 0){
# 162
({({struct Cyc_Tcenv_Tenv*_tmp208=te2;struct Cyc_Absyn_Exp*_tmp207=where_opt;Cyc_Tcexp_tcTest(_tmp208,_tmp207,({const char*_tmp38="switch clause guard";_tag_fat(_tmp38,sizeof(char),20U);}));});});
if(!({Cyc_Tcstmt_noassign_exp(where_opt);}))
({void*_tmp39=0U;({unsigned _tmp20A=where_opt->loc;struct _fat_ptr _tmp209=({const char*_tmp3A="cannot show &&-clause in pattern has no effects";_tag_fat(_tmp3A,sizeof(char),48U);});Cyc_Tcutil_terr(_tmp20A,_tmp209,_tag_fat(_tmp39,sizeof(void*),0U));});});}
# 157
({Cyc_Tcstmt_tcStmt(te2,s,0);});}}}
# 143
static int Cyc_Tcstmt_stmt_temp_var_counter=0;
# 170
static struct _tuple2*Cyc_Tcstmt_stmt_temp_var(){
int i=Cyc_Tcstmt_stmt_temp_var_counter ++;
struct _tuple2*res=({struct _tuple2*_tmp4B=_cycalloc(sizeof(*_tmp4B));_tmp4B->f1=Cyc_Absyn_Loc_n,({struct _fat_ptr*_tmp20D=({struct _fat_ptr*_tmp4A=_cycalloc(sizeof(*_tmp4A));({struct _fat_ptr _tmp20C=(struct _fat_ptr)({struct Cyc_Int_pa_PrintArg_struct _tmp49=({struct Cyc_Int_pa_PrintArg_struct _tmp1DD;_tmp1DD.tag=1U,_tmp1DD.f1=(unsigned)i;_tmp1DD;});void*_tmp47[1U];_tmp47[0]=& _tmp49;({struct _fat_ptr _tmp20B=({const char*_tmp48="_stmttmp%X";_tag_fat(_tmp48,sizeof(char),11U);});Cyc_aprintf(_tmp20B,_tag_fat(_tmp47,sizeof(void*),1U));});});*_tmp4A=_tmp20C;});_tmp4A;});_tmp4B->f2=_tmp20D;});_tmp4B;});
return res;}
# 170
static void Cyc_Tcstmt_tcFndecl_inner(struct Cyc_Tcenv_Tenv*te,unsigned loc,struct Cyc_Absyn_Fndecl*fd);
# 186 "tcstmt.cyc"
void Cyc_Tcstmt_tcStmt(struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Stmt*s0,int new_block){
# 188
void*_stmttmp4=s0->r;void*_tmp4D=_stmttmp4;struct Cyc_Absyn_Stmt*_tmp4F;struct Cyc_Absyn_Decl*_tmp4E;void**_tmp52;struct Cyc_List_List**_tmp51;struct Cyc_Absyn_Stmt*_tmp50;void**_tmp55;struct Cyc_List_List*_tmp54;struct Cyc_Absyn_Exp**_tmp53;struct Cyc_Absyn_Stmt*_tmp57;struct _fat_ptr*_tmp56;struct Cyc_Absyn_Switch_clause***_tmp59;struct Cyc_List_List*_tmp58;struct Cyc_Absyn_Stmt*_tmp5C;struct Cyc_Absyn_Exp*_tmp5B;struct Cyc_Absyn_Stmt*_tmp5A;struct Cyc_Absyn_Stmt*_tmp62;struct Cyc_Absyn_Stmt*_tmp61;struct Cyc_Absyn_Exp*_tmp60;struct Cyc_Absyn_Stmt*_tmp5F;struct Cyc_Absyn_Exp*_tmp5E;struct Cyc_Absyn_Exp*_tmp5D;struct Cyc_Absyn_Stmt*_tmp65;struct Cyc_Absyn_Stmt*_tmp64;struct Cyc_Absyn_Exp*_tmp63;struct Cyc_Absyn_Stmt*_tmp68;struct Cyc_Absyn_Stmt*_tmp67;struct Cyc_Absyn_Exp*_tmp66;struct Cyc_Absyn_Exp*_tmp69;struct Cyc_Absyn_Stmt*_tmp6B;struct Cyc_Absyn_Stmt*_tmp6A;struct Cyc_Absyn_Exp*_tmp6C;switch(*((int*)_tmp4D)){case 0U: _LL1: _LL2:
# 190
 return;case 1U: _LL3: _tmp6C=((struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct*)_tmp4D)->f1;_LL4: {struct Cyc_Absyn_Exp*e=_tmp6C;
# 193
({Cyc_Tcexp_tcExp(te,0,e);});
if(!({Cyc_Tcenv_in_stmt_exp(te);}))
({Cyc_Tcstmt_simplify_unused_result_exp(e);});
# 194
return;}case 2U: _LL5: _tmp6A=((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_tmp4D)->f1;_tmp6B=((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_tmp4D)->f2;_LL6: {struct Cyc_Absyn_Stmt*s1=_tmp6A;struct Cyc_Absyn_Stmt*s2=_tmp6B;
# 199
({Cyc_Tcstmt_tcStmt(te,s1,1);});
({Cyc_Tcstmt_tcStmt(te,s2,1);});
return;}case 3U: _LL7: _tmp69=((struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct*)_tmp4D)->f1;_LL8: {struct Cyc_Absyn_Exp*eopt=_tmp69;
# 204
void*t=({Cyc_Tcenv_return_typ(te);});
if(eopt == 0){
if(!({Cyc_Tcutil_is_void_type(t);})){
if(({Cyc_Tcutil_is_any_float_type(t);})||({Cyc_Tcutil_is_any_int_type(t);}))
({struct Cyc_String_pa_PrintArg_struct _tmp6F=({struct Cyc_String_pa_PrintArg_struct _tmp1DE;_tmp1DE.tag=0U,({struct _fat_ptr _tmp20E=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t);}));_tmp1DE.f1=_tmp20E;});_tmp1DE;});void*_tmp6D[1U];_tmp6D[0]=& _tmp6F;({unsigned _tmp210=s0->loc;struct _fat_ptr _tmp20F=({const char*_tmp6E="should return a value of type %s";_tag_fat(_tmp6E,sizeof(char),33U);});Cyc_Tcutil_warn(_tmp210,_tmp20F,_tag_fat(_tmp6D,sizeof(void*),1U));});});else{
# 210
({struct Cyc_String_pa_PrintArg_struct _tmp72=({struct Cyc_String_pa_PrintArg_struct _tmp1DF;_tmp1DF.tag=0U,({struct _fat_ptr _tmp211=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t);}));_tmp1DF.f1=_tmp211;});_tmp1DF;});void*_tmp70[1U];_tmp70[0]=& _tmp72;({unsigned _tmp213=s0->loc;struct _fat_ptr _tmp212=({const char*_tmp71="must return a value of type %s";_tag_fat(_tmp71,sizeof(char),31U);});Cyc_Tcutil_terr(_tmp213,_tmp212,_tag_fat(_tmp70,sizeof(void*),1U));});});}}}else{
# 214
int bogus=0;
struct Cyc_Absyn_Exp*e=eopt;
({Cyc_Tcexp_tcExp(te,& t,e);});
if(!({Cyc_Tcutil_coerce_arg(te,e,t,& bogus);})){
({struct Cyc_String_pa_PrintArg_struct _tmp75=({struct Cyc_String_pa_PrintArg_struct _tmp1E1;_tmp1E1.tag=0U,({
struct _fat_ptr _tmp214=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string((void*)_check_null(e->topt));}));_tmp1E1.f1=_tmp214;});_tmp1E1;});struct Cyc_String_pa_PrintArg_struct _tmp76=({struct Cyc_String_pa_PrintArg_struct _tmp1E0;_tmp1E0.tag=0U,({struct _fat_ptr _tmp215=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t);}));_tmp1E0.f1=_tmp215;});_tmp1E0;});void*_tmp73[2U];_tmp73[0]=& _tmp75,_tmp73[1]=& _tmp76;({unsigned _tmp217=s0->loc;struct _fat_ptr _tmp216=({const char*_tmp74="returns value of type %s but requires %s";_tag_fat(_tmp74,sizeof(char),41U);});Cyc_Tcutil_terr(_tmp217,_tmp216,_tag_fat(_tmp73,sizeof(void*),2U));});});
({Cyc_Unify_explain_failure();});}
# 217
if(
# 222
({Cyc_Tcutil_is_noalias_pointer_or_aggr(t);})&& !({Cyc_Tcutil_is_noalias_path(e);}))
({void*_tmp77=0U;({unsigned _tmp219=e->loc;struct _fat_ptr _tmp218=({const char*_tmp78="Cannot consume non-unique paths; do swap instead";_tag_fat(_tmp78,sizeof(char),49U);});Cyc_Tcutil_terr(_tmp219,_tmp218,_tag_fat(_tmp77,sizeof(void*),0U));});});}
# 225
return;}case 4U: _LL9: _tmp66=((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmp4D)->f1;_tmp67=((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmp4D)->f2;_tmp68=((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmp4D)->f3;_LLA: {struct Cyc_Absyn_Exp*e=_tmp66;struct Cyc_Absyn_Stmt*s1=_tmp67;struct Cyc_Absyn_Stmt*s2=_tmp68;
# 228
({({struct Cyc_Tcenv_Tenv*_tmp21B=te;struct Cyc_Absyn_Exp*_tmp21A=e;Cyc_Tcexp_tcTest(_tmp21B,_tmp21A,({const char*_tmp79="if statement";_tag_fat(_tmp79,sizeof(char),13U);}));});});
({Cyc_Tcstmt_tcStmt(te,s1,1);});
({Cyc_Tcstmt_tcStmt(te,s2,1);});
return;}case 5U: _LLB: _tmp63=(((struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct*)_tmp4D)->f1).f1;_tmp64=(((struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct*)_tmp4D)->f1).f2;_tmp65=((struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct*)_tmp4D)->f2;_LLC: {struct Cyc_Absyn_Exp*e=_tmp63;struct Cyc_Absyn_Stmt*cont_s=_tmp64;struct Cyc_Absyn_Stmt*s=_tmp65;
# 234
({({struct Cyc_Tcenv_Tenv*_tmp21D=te;struct Cyc_Absyn_Exp*_tmp21C=e;Cyc_Tcexp_tcTest(_tmp21D,_tmp21C,({const char*_tmp7A="while loop";_tag_fat(_tmp7A,sizeof(char),11U);}));});});
({Cyc_Tcstmt_tcStmt(te,s,1);});
return;}case 9U: _LLD: _tmp5D=((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp4D)->f1;_tmp5E=(((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp4D)->f2).f1;_tmp5F=(((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp4D)->f2).f2;_tmp60=(((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp4D)->f3).f1;_tmp61=(((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp4D)->f3).f2;_tmp62=((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp4D)->f4;_LLE: {struct Cyc_Absyn_Exp*e1=_tmp5D;struct Cyc_Absyn_Exp*e2=_tmp5E;struct Cyc_Absyn_Stmt*guard_s=_tmp5F;struct Cyc_Absyn_Exp*e3=_tmp60;struct Cyc_Absyn_Stmt*cont_s=_tmp61;struct Cyc_Absyn_Stmt*s=_tmp62;
# 239
({Cyc_Tcexp_tcExp(te,0,e1);});
({({struct Cyc_Tcenv_Tenv*_tmp21F=te;struct Cyc_Absyn_Exp*_tmp21E=e2;Cyc_Tcexp_tcTest(_tmp21F,_tmp21E,({const char*_tmp7B="for loop";_tag_fat(_tmp7B,sizeof(char),9U);}));});});
({Cyc_Tcstmt_tcStmt(te,s,1);});
({Cyc_Tcexp_tcExp(te,0,e3);});
({Cyc_Tcstmt_simplify_unused_result_exp(e3);});
return;}case 14U: _LLF: _tmp5A=((struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct*)_tmp4D)->f1;_tmp5B=(((struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct*)_tmp4D)->f2).f1;_tmp5C=(((struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct*)_tmp4D)->f2).f2;_LL10: {struct Cyc_Absyn_Stmt*s=_tmp5A;struct Cyc_Absyn_Exp*e=_tmp5B;struct Cyc_Absyn_Stmt*cont_s=_tmp5C;
# 247
({Cyc_Tcstmt_tcStmt(te,s,1);});
({({struct Cyc_Tcenv_Tenv*_tmp221=te;struct Cyc_Absyn_Exp*_tmp220=e;Cyc_Tcexp_tcTest(_tmp221,_tmp220,({const char*_tmp7C="do loop";_tag_fat(_tmp7C,sizeof(char),8U);}));});});
return;}case 6U: _LL11: _LL12:
# 251
 goto _LL14;case 7U: _LL13: _LL14:
 goto _LL16;case 8U: _LL15: _LL16:
 return;case 11U: _LL17: _tmp58=((struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct*)_tmp4D)->f1;_tmp59=(struct Cyc_Absyn_Switch_clause***)&((struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct*)_tmp4D)->f2;_LL18: {struct Cyc_List_List*es=_tmp58;struct Cyc_Absyn_Switch_clause***clauseopt=_tmp59;
# 256
const struct _tuple13*trip_opt=({Cyc_Tcenv_process_fallthru(te,s0,clauseopt);});
if(trip_opt == (const struct _tuple13*)0){
({void*_tmp7D=0U;({unsigned _tmp223=s0->loc;struct _fat_ptr _tmp222=({const char*_tmp7E="fallthru not in a non-last case";_tag_fat(_tmp7E,sizeof(char),32U);});Cyc_Tcutil_terr(_tmp223,_tmp222,_tag_fat(_tmp7D,sizeof(void*),0U));});});
return;}{
# 257
struct Cyc_List_List*tvs=(*trip_opt).f2;
# 262
struct Cyc_List_List*ts_orig=(*trip_opt).f3;
struct Cyc_List_List*instantiation=({struct Cyc_List_List*(*_tmp8A)(struct _tuple14*(*f)(struct Cyc_List_List*,struct Cyc_Absyn_Tvar*),struct Cyc_List_List*env,struct Cyc_List_List*x)=({
struct Cyc_List_List*(*_tmp8B)(struct _tuple14*(*f)(struct Cyc_List_List*,struct Cyc_Absyn_Tvar*),struct Cyc_List_List*env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct _tuple14*(*f)(struct Cyc_List_List*,struct Cyc_Absyn_Tvar*),struct Cyc_List_List*env,struct Cyc_List_List*x))Cyc_List_map_c;_tmp8B;});struct _tuple14*(*_tmp8C)(struct Cyc_List_List*,struct Cyc_Absyn_Tvar*)=Cyc_Tcutil_make_inst_var;struct Cyc_List_List*_tmp8D=({Cyc_Tcenv_lookup_type_vars(te);});struct Cyc_List_List*_tmp8E=tvs;_tmp8A(_tmp8C,_tmp8D,_tmp8E);});
struct Cyc_List_List*ts=({({struct Cyc_List_List*(*_tmp225)(void*(*f)(struct Cyc_List_List*,void*),struct Cyc_List_List*env,struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp89)(void*(*f)(struct Cyc_List_List*,void*),struct Cyc_List_List*env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(void*(*f)(struct Cyc_List_List*,void*),struct Cyc_List_List*env,struct Cyc_List_List*x))Cyc_List_map_c;_tmp89;});struct Cyc_List_List*_tmp224=instantiation;_tmp225(Cyc_Tcutil_substitute,_tmp224,ts_orig);});});
for(0;ts != 0 && es != 0;(ts=ts->tl,es=es->tl)){
# 268
int bogus=0;
({Cyc_Tcexp_tcExp(te,0,(struct Cyc_Absyn_Exp*)es->hd);});
if(!({Cyc_Tcutil_coerce_arg(te,(struct Cyc_Absyn_Exp*)es->hd,(void*)ts->hd,& bogus);})){
({struct Cyc_String_pa_PrintArg_struct _tmp81=({struct Cyc_String_pa_PrintArg_struct _tmp1E3;_tmp1E3.tag=0U,({
# 273
struct _fat_ptr _tmp226=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string((void*)_check_null(((struct Cyc_Absyn_Exp*)es->hd)->topt));}));_tmp1E3.f1=_tmp226;});_tmp1E3;});struct Cyc_String_pa_PrintArg_struct _tmp82=({struct Cyc_String_pa_PrintArg_struct _tmp1E2;_tmp1E2.tag=0U,({struct _fat_ptr _tmp227=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string((void*)ts->hd);}));_tmp1E2.f1=_tmp227;});_tmp1E2;});void*_tmp7F[2U];_tmp7F[0]=& _tmp81,_tmp7F[1]=& _tmp82;({unsigned _tmp229=s0->loc;struct _fat_ptr _tmp228=({const char*_tmp80="fallthru argument has type %s but pattern variable has type %s";_tag_fat(_tmp80,sizeof(char),63U);});Cyc_Tcutil_terr(_tmp229,_tmp228,_tag_fat(_tmp7F,sizeof(void*),2U));});});
({Cyc_Unify_explain_failure();});}
# 270
if(
# 277
({Cyc_Tcutil_is_noalias_pointer_or_aggr((void*)ts->hd);})&& !({Cyc_Tcutil_is_noalias_path((struct Cyc_Absyn_Exp*)es->hd);}))
({void*_tmp83=0U;({unsigned _tmp22B=((struct Cyc_Absyn_Exp*)es->hd)->loc;struct _fat_ptr _tmp22A=({const char*_tmp84="Cannot consume non-unique paths; do swap instead";_tag_fat(_tmp84,sizeof(char),49U);});Cyc_Tcutil_terr(_tmp22B,_tmp22A,_tag_fat(_tmp83,sizeof(void*),0U));});});}
# 280
if(es != 0)
({void*_tmp85=0U;({unsigned _tmp22D=s0->loc;struct _fat_ptr _tmp22C=({const char*_tmp86="too many arguments in explicit fallthru";_tag_fat(_tmp86,sizeof(char),40U);});Cyc_Tcutil_terr(_tmp22D,_tmp22C,_tag_fat(_tmp85,sizeof(void*),0U));});});
# 280
if(ts != 0)
# 283
({void*_tmp87=0U;({unsigned _tmp22F=s0->loc;struct _fat_ptr _tmp22E=({const char*_tmp88="too few arguments in explicit fallthru";_tag_fat(_tmp88,sizeof(char),39U);});Cyc_Tcutil_terr(_tmp22F,_tmp22E,_tag_fat(_tmp87,sizeof(void*),0U));});});
# 280
return;}}case 13U: _LL19: _tmp56=((struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct*)_tmp4D)->f1;_tmp57=((struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct*)_tmp4D)->f2;_LL1A: {struct _fat_ptr*l=_tmp56;struct Cyc_Absyn_Stmt*s=_tmp57;
# 290
({void(*_tmp8F)(struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Stmt*s0,int new_block)=Cyc_Tcstmt_tcStmt;struct Cyc_Tcenv_Tenv*_tmp90=({struct Cyc_Tcenv_Tenv*(*_tmp91)(unsigned,struct Cyc_Tcenv_Tenv*,struct Cyc_Absyn_Tvar*)=Cyc_Tcenv_new_named_block;unsigned _tmp92=s0->loc;struct Cyc_Tcenv_Tenv*_tmp93=te;struct Cyc_Absyn_Tvar*_tmp94=({struct Cyc_Absyn_Tvar*_tmp99=_cycalloc(sizeof(*_tmp99));
({struct _fat_ptr*_tmp234=({struct _fat_ptr*_tmp98=_cycalloc(sizeof(*_tmp98));({struct _fat_ptr _tmp233=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp97=({struct Cyc_String_pa_PrintArg_struct _tmp1E4;_tmp1E4.tag=0U,_tmp1E4.f1=(struct _fat_ptr)((struct _fat_ptr)*l);_tmp1E4;});void*_tmp95[1U];_tmp95[0]=& _tmp97;({struct _fat_ptr _tmp232=({const char*_tmp96="`%s";_tag_fat(_tmp96,sizeof(char),4U);});Cyc_aprintf(_tmp232,_tag_fat(_tmp95,sizeof(void*),1U));});});*_tmp98=_tmp233;});_tmp98;});_tmp99->name=_tmp234;}),({
int _tmp231=({Cyc_Tcutil_new_tvar_id();});_tmp99->identity=_tmp231;}),({
void*_tmp230=({Cyc_Tcutil_kind_to_bound(& Cyc_Tcutil_rk);});_tmp99->kind=_tmp230;});_tmp99;});_tmp91(_tmp92,_tmp93,_tmp94);});struct Cyc_Absyn_Stmt*_tmp9A=s;int _tmp9B=0;_tmp8F(_tmp90,_tmp9A,_tmp9B);});
return;}case 10U: _LL1B: _tmp53=(struct Cyc_Absyn_Exp**)&((struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct*)_tmp4D)->f1;_tmp54=((struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct*)_tmp4D)->f2;_tmp55=(void**)&((struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct*)_tmp4D)->f3;_LL1C: {struct Cyc_Absyn_Exp**exp=_tmp53;struct Cyc_List_List*scs0=_tmp54;void**dtp=_tmp55;
# 299
struct Cyc_Absyn_Exp*e=*exp;
if(!({Cyc_Tcstmt_is_var_exp(e);})){
struct Cyc_Absyn_Vardecl*vd=({struct Cyc_Absyn_Vardecl*(*_tmpA5)(unsigned varloc,struct _tuple2*,void*,struct Cyc_Absyn_Exp*init)=Cyc_Absyn_new_vardecl;unsigned _tmpA6=0U;struct _tuple2*_tmpA7=({Cyc_Tcstmt_stmt_temp_var();});void*_tmpA8=({Cyc_Absyn_new_evar(0,0);});struct Cyc_Absyn_Exp*_tmpA9=0;_tmpA5(_tmpA6,_tmpA7,_tmpA8,_tmpA9);});
struct Cyc_Absyn_Stmt*s1=({Cyc_Absyn_new_stmt(s0->r,s0->loc);});
struct Cyc_Absyn_Decl*d=({struct Cyc_Absyn_Decl*(*_tmp9D)(struct Cyc_Absyn_Pat*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_let_decl;struct Cyc_Absyn_Pat*_tmp9E=({struct Cyc_Absyn_Pat*(*_tmp9F)(void*,unsigned)=Cyc_Absyn_new_pat;void*_tmpA0=(void*)({struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct*_tmpA1=_cycalloc(sizeof(*_tmpA1));_tmpA1->tag=1U,_tmpA1->f1=vd,({struct Cyc_Absyn_Pat*_tmp235=({Cyc_Absyn_new_pat((void*)& Cyc_Absyn_Wild_p_val,e->loc);});_tmpA1->f2=_tmp235;});_tmpA1;});unsigned _tmpA2=e->loc;_tmp9F(_tmpA0,_tmpA2);});struct Cyc_Absyn_Exp*_tmpA3=e;unsigned _tmpA4=s0->loc;_tmp9D(_tmp9E,_tmpA3,_tmpA4);});
# 306
struct Cyc_Absyn_Stmt*s2=({Cyc_Absyn_decl_stmt(d,s1,s0->loc);});
s0->r=s2->r;
({struct Cyc_Absyn_Exp*_tmp237=({({void*_tmp236=(void*)({struct Cyc_Absyn_Local_b_Absyn_Binding_struct*_tmp9C=_cycalloc(sizeof(*_tmp9C));_tmp9C->tag=4U,_tmp9C->f1=vd;_tmp9C;});Cyc_Absyn_varb_exp(_tmp236,e->loc);});});*exp=_tmp237;});
({Cyc_Tcstmt_tcStmt(te,s0,new_block);});
return;}{
# 300
void*t=({Cyc_Tcexp_tcExp(te,0,e);});
# 315
if(({Cyc_Tcutil_is_noalias_pointer_or_aggr(t);})&& !({Cyc_Tcutil_is_noalias_path(e);}))
({void*_tmpAA=0U;({unsigned _tmp239=e->loc;struct _fat_ptr _tmp238=({const char*_tmpAB="Cannot consume non-unique paths; do swap instead";_tag_fat(_tmpAB,sizeof(char),49U);});Cyc_Tcutil_terr(_tmp239,_tmp238,_tag_fat(_tmpAA,sizeof(void*),0U));});});{
# 315
struct Cyc_Tcpat_TcPatResult this_pattern;
# 321
struct Cyc_Tcpat_TcPatResult*next_pattern=0;
int first_case=1;
{struct Cyc_List_List*scs=scs0;for(0;scs != 0;scs=scs->tl){
if(first_case){
first_case=0;
this_pattern=({Cyc_Tcpat_tcPat(te,((struct Cyc_Absyn_Switch_clause*)scs->hd)->pattern,& t,e);});}else{
# 328
this_pattern=*((struct Cyc_Tcpat_TcPatResult*)_check_null(next_pattern));}
# 330
if(scs->tl != 0){
next_pattern=({struct Cyc_Tcpat_TcPatResult*_tmpAC=_cycalloc(sizeof(*_tmpAC));({struct Cyc_Tcpat_TcPatResult _tmp23A=({Cyc_Tcpat_tcPat(te,((struct Cyc_Absyn_Switch_clause*)((struct Cyc_List_List*)_check_null(scs->tl))->hd)->pattern,& t,e);});*_tmpAC=_tmp23A;});_tmpAC;});
if(next_pattern->tvars_and_bounds_opt != 0 &&(*next_pattern->tvars_and_bounds_opt).f2 != 0)
# 334
te=({Cyc_Tcenv_clear_fallthru(te);});else{
# 336
struct Cyc_List_List*vs=({struct Cyc_List_List*(*_tmpAF)(struct Cyc_List_List*x)=Cyc_List_imp_rev;struct Cyc_List_List*_tmpB0=({struct Cyc_List_List*(*_tmpB1)(struct Cyc_List_List*)=Cyc_Tcutil_filter_nulls;struct Cyc_List_List*_tmpB2=({Cyc_List_split(next_pattern->patvars);}).f1;_tmpB1(_tmpB2);});_tmpAF(_tmpB0);});
struct Cyc_List_List*new_tvs=next_pattern->tvars_and_bounds_opt == 0?0:({({struct Cyc_List_List*(*_tmp23C)(struct Cyc_Absyn_Tvar*(*f)(struct _tuple17*),struct Cyc_List_List*x)=({
struct Cyc_List_List*(*_tmpAD)(struct Cyc_Absyn_Tvar*(*f)(struct _tuple17*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_Tvar*(*f)(struct _tuple17*),struct Cyc_List_List*x))Cyc_List_map;_tmpAD;});struct Cyc_Absyn_Tvar*(*_tmp23B)(struct _tuple17*)=({struct Cyc_Absyn_Tvar*(*_tmpAE)(struct _tuple17*)=(struct Cyc_Absyn_Tvar*(*)(struct _tuple17*))Cyc_Core_fst;_tmpAE;});_tmp23C(_tmp23B,(*next_pattern->tvars_and_bounds_opt).f1);});});
te=({Cyc_Tcenv_set_fallthru(te,new_tvs,vs,(struct Cyc_Absyn_Switch_clause*)((struct Cyc_List_List*)_check_null(scs->tl))->hd);});}}else{
# 342
te=({Cyc_Tcenv_clear_fallthru(te);});}{
# 344
struct Cyc_Absyn_Pat*p=((struct Cyc_Absyn_Switch_clause*)scs->hd)->pattern;
if(!({Cyc_Unify_unify((void*)_check_null(p->topt),t);})){
({struct Cyc_String_pa_PrintArg_struct _tmpB5=({struct Cyc_String_pa_PrintArg_struct _tmp1E6;_tmp1E6.tag=0U,({
struct _fat_ptr _tmp23D=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t);}));_tmp1E6.f1=_tmp23D;});_tmp1E6;});struct Cyc_String_pa_PrintArg_struct _tmpB6=({struct Cyc_String_pa_PrintArg_struct _tmp1E5;_tmp1E5.tag=0U,({struct _fat_ptr _tmp23E=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string((void*)_check_null(p->topt));}));_tmp1E5.f1=_tmp23E;});_tmp1E5;});void*_tmpB3[2U];_tmpB3[0]=& _tmpB5,_tmpB3[1]=& _tmpB6;({unsigned _tmp240=((struct Cyc_Absyn_Switch_clause*)scs->hd)->loc;struct _fat_ptr _tmp23F=({const char*_tmpB4="switch on type %s, but case expects type %s";_tag_fat(_tmpB4,sizeof(char),44U);});Cyc_Tcutil_terr(_tmp240,_tmp23F,_tag_fat(_tmpB3,sizeof(void*),2U));});});
({Cyc_Unify_explain_failure();});}else{
# 351
({Cyc_Tcpat_check_pat_regions(te,p,this_pattern.patvars);});}
# 353
({struct Cyc_Core_Opt*_tmp241=({struct Cyc_Core_Opt*_tmpB7=_cycalloc(sizeof(*_tmpB7));_tmpB7->v=this_pattern.patvars;_tmpB7;});((struct Cyc_Absyn_Switch_clause*)scs->hd)->pat_vars=_tmp241;});
({Cyc_Tcstmt_pattern_synth(((struct Cyc_Absyn_Switch_clause*)scs->hd)->loc,te,this_pattern,((struct Cyc_Absyn_Switch_clause*)scs->hd)->body,((struct Cyc_Absyn_Switch_clause*)scs->hd)->where_clause,1);});}}}
# 357
({Cyc_Tcpat_check_switch_exhaustive(s0->loc,scs0,dtp);});
return;}}}case 15U: _LL1D: _tmp50=((struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct*)_tmp4D)->f1;_tmp51=(struct Cyc_List_List**)&((struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct*)_tmp4D)->f2;_tmp52=(void**)&((struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct*)_tmp4D)->f3;_LL1E: {struct Cyc_Absyn_Stmt*s=_tmp50;struct Cyc_List_List**scs0=_tmp51;void**dtp=_tmp52;
# 365
({void*_tmp244=({struct Cyc_Absyn_Stmt*(*_tmpB8)(void*,unsigned)=Cyc_Absyn_new_stmt;void*_tmpB9=(void*)({struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*_tmpBA=_cycalloc(sizeof(*_tmpBA));_tmpBA->tag=2U,({struct Cyc_Absyn_Stmt*_tmp243=({Cyc_Absyn_new_stmt(s->r,s->loc);});_tmpBA->f1=_tmp243;}),({struct Cyc_Absyn_Stmt*_tmp242=({Cyc_Absyn_skip_stmt(s->loc);});_tmpBA->f2=_tmp242;});_tmpBA;});unsigned _tmpBB=s->loc;_tmpB8(_tmpB9,_tmpBB);})->r;s->r=_tmp244;});
# 368
({Cyc_Tcstmt_tcStmt(te,s,1);});{
# 374
struct _tuple2*def_v=({struct _tuple2*_tmpDA=_cycalloc(sizeof(*_tmpDA));_tmpDA->f1=Cyc_Absyn_Loc_n,({struct _fat_ptr*_tmp246=({struct _fat_ptr*_tmpD9=_cycalloc(sizeof(*_tmpD9));({struct _fat_ptr _tmp245=({Cyc_Absyn_exnstr();});*_tmpD9=_tmp245;});_tmpD9;});_tmpDA->f2=_tmp246;});_tmpDA;});
struct Cyc_Absyn_Vardecl*vd=({Cyc_Absyn_new_vardecl(0U,def_v,Cyc_Absyn_void_type,0);});
struct Cyc_Absyn_Pat*def_pat=({struct Cyc_Absyn_Pat*(*_tmpD5)(void*,unsigned)=Cyc_Absyn_new_pat;void*_tmpD6=(void*)({struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct*_tmpD7=_cycalloc(sizeof(*_tmpD7));_tmpD7->tag=1U,_tmpD7->f1=vd,({struct Cyc_Absyn_Pat*_tmp247=({Cyc_Absyn_new_pat((void*)& Cyc_Absyn_Wild_p_val,0U);});_tmpD7->f2=_tmp247;});_tmpD7;});unsigned _tmpD8=0U;_tmpD5(_tmpD6,_tmpD8);});
struct Cyc_Absyn_Stmt*def_stmt=({struct Cyc_Absyn_Stmt*(*_tmpCD)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_exp_stmt;struct Cyc_Absyn_Exp*_tmpCE=({struct Cyc_Absyn_Exp*(*_tmpCF)(void*,unsigned)=Cyc_Absyn_new_exp;void*_tmpD0=(void*)({struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct*_tmpD2=_cycalloc(sizeof(*_tmpD2));
_tmpD2->tag=11U,({struct Cyc_Absyn_Exp*_tmp248=({Cyc_Absyn_varb_exp((void*)({struct Cyc_Absyn_Local_b_Absyn_Binding_struct*_tmpD1=_cycalloc(sizeof(*_tmpD1));_tmpD1->tag=4U,_tmpD1->f1=vd;_tmpD1;}),0U);});_tmpD2->f1=_tmp248;}),_tmpD2->f2=1;_tmpD2;});unsigned _tmpD3=0U;_tmpCF(_tmpD0,_tmpD3);});unsigned _tmpD4=0U;_tmpCD(_tmpCE,_tmpD4);});
# 380
struct Cyc_Absyn_Switch_clause*def_clause=({struct Cyc_Absyn_Switch_clause*_tmpCC=_cycalloc(sizeof(*_tmpCC));_tmpCC->pattern=def_pat,_tmpCC->pat_vars=0,_tmpCC->where_clause=0,_tmpCC->body=def_stmt,_tmpCC->loc=0U;_tmpCC;});
({struct Cyc_List_List*_tmp249=({struct Cyc_List_List*(*_tmpBC)(struct Cyc_List_List*x,struct Cyc_List_List*y)=Cyc_List_append;struct Cyc_List_List*_tmpBD=*scs0;struct Cyc_List_List*_tmpBE=({struct Cyc_Absyn_Switch_clause*_tmpBF[1U];_tmpBF[0]=def_clause;Cyc_List_list(_tag_fat(_tmpBF,sizeof(struct Cyc_Absyn_Switch_clause*),1U));});_tmpBC(_tmpBD,_tmpBE);});*scs0=_tmp249;});{
# 387
void*exception_type=({Cyc_Absyn_exn_type();});
struct Cyc_Tcpat_TcPatResult this_pattern;
struct Cyc_Tcpat_TcPatResult*next_pattern=0;
int first_case=1;
# 392
{struct Cyc_List_List*scs=*scs0;for(0;scs != 0;scs=scs->tl){
# 394
if(first_case){
# 396
first_case=0;
this_pattern=({Cyc_Tcpat_tcPat(te,((struct Cyc_Absyn_Switch_clause*)scs->hd)->pattern,& exception_type,0);});}else{
# 400
this_pattern=*((struct Cyc_Tcpat_TcPatResult*)_check_null(next_pattern));}
# 402
if(scs->tl != 0){
# 404
next_pattern=({struct Cyc_Tcpat_TcPatResult*_tmpC0=_cycalloc(sizeof(*_tmpC0));({struct Cyc_Tcpat_TcPatResult _tmp24A=({Cyc_Tcpat_tcPat(te,((struct Cyc_Absyn_Switch_clause*)((struct Cyc_List_List*)_check_null(scs->tl))->hd)->pattern,& exception_type,0);});*_tmpC0=_tmp24A;});_tmpC0;});
# 406
if(next_pattern->tvars_and_bounds_opt != 0 &&(*next_pattern->tvars_and_bounds_opt).f2 != 0)
# 408
te=({Cyc_Tcenv_clear_fallthru(te);});else{
# 411
struct Cyc_List_List*vs=({struct Cyc_List_List*(*_tmpC3)(struct Cyc_List_List*x)=Cyc_List_imp_rev;struct Cyc_List_List*_tmpC4=({struct Cyc_List_List*(*_tmpC5)(struct Cyc_List_List*)=Cyc_Tcutil_filter_nulls;struct Cyc_List_List*_tmpC6=({Cyc_List_split(next_pattern->patvars);}).f1;_tmpC5(_tmpC6);});_tmpC3(_tmpC4);});
struct Cyc_List_List*new_tvs=next_pattern->tvars_and_bounds_opt == 0?0:({({struct Cyc_List_List*(*_tmp24C)(struct Cyc_Absyn_Tvar*(*f)(struct _tuple17*),struct Cyc_List_List*x)=({
struct Cyc_List_List*(*_tmpC1)(struct Cyc_Absyn_Tvar*(*f)(struct _tuple17*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_Tvar*(*f)(struct _tuple17*),struct Cyc_List_List*x))Cyc_List_map;_tmpC1;});struct Cyc_Absyn_Tvar*(*_tmp24B)(struct _tuple17*)=({struct Cyc_Absyn_Tvar*(*_tmpC2)(struct _tuple17*)=(struct Cyc_Absyn_Tvar*(*)(struct _tuple17*))Cyc_Core_fst;_tmpC2;});_tmp24C(_tmp24B,(*next_pattern->tvars_and_bounds_opt).f1);});});
te=({Cyc_Tcenv_set_fallthru(te,new_tvs,vs,(struct Cyc_Absyn_Switch_clause*)((struct Cyc_List_List*)_check_null(scs->tl))->hd);});}}else{
# 418
te=({Cyc_Tcenv_clear_fallthru(te);});}{
# 420
struct Cyc_Absyn_Pat*p=((struct Cyc_Absyn_Switch_clause*)scs->hd)->pattern;
# 424
if(!({Cyc_Unify_unify((void*)_check_null(p->topt),exception_type);})){
({struct Cyc_String_pa_PrintArg_struct _tmpC9=({struct Cyc_String_pa_PrintArg_struct _tmp1E8;_tmp1E8.tag=0U,({
struct _fat_ptr _tmp24D=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(exception_type);}));_tmp1E8.f1=_tmp24D;});_tmp1E8;});struct Cyc_String_pa_PrintArg_struct _tmpCA=({struct Cyc_String_pa_PrintArg_struct _tmp1E7;_tmp1E7.tag=0U,({struct _fat_ptr _tmp24E=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string((void*)_check_null(p->topt));}));_tmp1E7.f1=_tmp24E;});_tmp1E7;});void*_tmpC7[2U];_tmpC7[0]=& _tmpC9,_tmpC7[1]=& _tmpCA;({unsigned _tmp250=((struct Cyc_Absyn_Switch_clause*)scs->hd)->loc;struct _fat_ptr _tmp24F=({const char*_tmpC8="switch on type %s, but case expects type %s";_tag_fat(_tmpC8,sizeof(char),44U);});Cyc_Tcutil_terr(_tmp250,_tmp24F,_tag_fat(_tmpC7,sizeof(void*),2U));});});
({Cyc_Unify_explain_failure();});}else{
# 430
({Cyc_Tcpat_check_pat_regions(te,p,this_pattern.patvars);});}
# 432
({struct Cyc_Core_Opt*_tmp251=({struct Cyc_Core_Opt*_tmpCB=_cycalloc(sizeof(*_tmpCB));_tmpCB->v=this_pattern.patvars;_tmpCB;});((struct Cyc_Absyn_Switch_clause*)scs->hd)->pat_vars=_tmp251;});
# 435
({Cyc_Tcstmt_pattern_synth(((struct Cyc_Absyn_Switch_clause*)scs->hd)->loc,te,this_pattern,((struct Cyc_Absyn_Switch_clause*)scs->hd)->body,((struct Cyc_Absyn_Switch_clause*)scs->hd)->where_clause,1);});}}}
# 441
({Cyc_Tcpat_check_catch_overlap(s0->loc,te,*scs0,dtp);});
# 445
return;}}}default: _LL1F: _tmp4E=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp4D)->f1;_tmp4F=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp4D)->f2;_LL20: {struct Cyc_Absyn_Decl*d=_tmp4E;struct Cyc_Absyn_Stmt*s=_tmp4F;
# 448
struct _fat_ptr unimp_msg_part;
struct Cyc_Tcenv_Tenv*_tmpDB=new_block?({Cyc_Tcenv_new_block(s0->loc,te);}): te;struct Cyc_Tcenv_Tenv*te=_tmpDB;
{void*_stmttmp5=d->r;void*_tmpDC=_stmttmp5;struct Cyc_List_List*_tmpDE;struct _tuple2*_tmpDD;struct Cyc_List_List*_tmpE0;struct _fat_ptr*_tmpDF;struct Cyc_Absyn_Fndecl*_tmpE1;struct Cyc_Absyn_Exp*_tmpE4;struct Cyc_Absyn_Vardecl*_tmpE3;struct Cyc_Absyn_Tvar*_tmpE2;struct Cyc_List_List*_tmpE5;void**_tmpE9;struct Cyc_Absyn_Exp**_tmpE8;struct Cyc_Core_Opt**_tmpE7;struct Cyc_Absyn_Pat*_tmpE6;struct Cyc_Absyn_Vardecl*_tmpEA;switch(*((int*)_tmpDC)){case 0U: _LL22: _tmpEA=((struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_tmpDC)->f1;_LL23: {struct Cyc_Absyn_Vardecl*vd=_tmpEA;
# 452
struct Cyc_List_List*_tmpF2;void**_tmpF1;struct Cyc_Absyn_Exp*_tmpF0;void*_tmpEF;struct Cyc_Absyn_Tqual _tmpEE;struct _fat_ptr*_tmpED;union Cyc_Absyn_Nmspace _tmpEC;enum Cyc_Absyn_Scope _tmpEB;_LL45: _tmpEB=vd->sc;_tmpEC=(vd->name)->f1;_tmpED=(vd->name)->f2;_tmpEE=vd->tq;_tmpEF=vd->type;_tmpF0=vd->initializer;_tmpF1=(void**)& vd->rgn;_tmpF2=vd->attributes;_LL46: {enum Cyc_Absyn_Scope sc=_tmpEB;union Cyc_Absyn_Nmspace nsl=_tmpEC;struct _fat_ptr*x=_tmpED;struct Cyc_Absyn_Tqual tq=_tmpEE;void*t=_tmpEF;struct Cyc_Absyn_Exp*initializer=_tmpF0;void**rgn_ptr=_tmpF1;struct Cyc_List_List*atts=_tmpF2;
void*curr_bl=({Cyc_Tcenv_curr_rgn(te);});
int is_local;
{enum Cyc_Absyn_Scope _tmpF3=sc;switch(_tmpF3){case Cyc_Absyn_Static: _LL48: _LL49:
 goto _LL4B;case Cyc_Absyn_Extern: _LL4A: _LL4B:
 goto _LL4D;case Cyc_Absyn_ExternC: _LL4C: _LL4D:
# 459
 vd->escapes=1;
is_local=0;goto _LL47;case Cyc_Absyn_Abstract: _LL4E: _LL4F:
# 462
({void*_tmpF4=0U;({unsigned _tmp253=d->loc;struct _fat_ptr _tmp252=({const char*_tmpF5="bad abstract scope for local variable";_tag_fat(_tmpF5,sizeof(char),38U);});Cyc_Tcutil_terr(_tmp253,_tmp252,_tag_fat(_tmpF4,sizeof(void*),0U));});});
goto _LL51;case Cyc_Absyn_Register: _LL50: _LL51:
 goto _LL53;case Cyc_Absyn_Public: _LL52: _LL53:
 goto _LL55;default: _LL54: _LL55:
 is_local=1;goto _LL47;}_LL47:;}
# 472
*rgn_ptr=is_local?curr_bl: Cyc_Absyn_heap_rgn_type;
# 474
{void*_stmttmp6=({Cyc_Tcutil_compress(t);});void*_tmpF6=_stmttmp6;unsigned _tmpFA;void*_tmpF9;struct Cyc_Absyn_Tqual _tmpF8;void*_tmpF7;switch(*((int*)_tmpF6)){case 5U: _LL57: if(is_local){_LL58:
# 477
 vd->escapes=1;
sc=3U;
is_local=0;
goto _LL56;}else{goto _LL5B;}case 4U: if((((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmpF6)->f1).num_elts == 0){_LL59: _tmpF7=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmpF6)->f1).elt_type;_tmpF8=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmpF6)->f1).tq;_tmpF9=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmpF6)->f1).zero_term;_tmpFA=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmpF6)->f1).zt_loc;if(vd->initializer != 0){_LL5A: {void*telt=_tmpF7;struct Cyc_Absyn_Tqual tq=_tmpF8;void*zt=_tmpF9;unsigned ztl=_tmpFA;
# 482
{void*_stmttmp7=((struct Cyc_Absyn_Exp*)_check_null(vd->initializer))->r;void*_tmpFB=_stmttmp7;struct Cyc_List_List*_tmpFC;struct Cyc_List_List*_tmpFD;struct Cyc_Absyn_Exp*_tmpFE;struct Cyc_Absyn_Exp*_tmpFF;struct _fat_ptr _tmp100;struct _fat_ptr _tmp101;switch(*((int*)_tmpFB)){case 0U: switch(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmpFB)->f1).Wstring_c).tag){case 8U: _LL5E: _tmp101=((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmpFB)->f1).String_c).val;_LL5F: {struct _fat_ptr s=_tmp101;
# 484
t=({void*_tmp254=({void*(*_tmp102)(void*elt_type,struct Cyc_Absyn_Tqual tq,struct Cyc_Absyn_Exp*num_elts,void*zero_term,unsigned ztloc)=Cyc_Absyn_array_type;void*_tmp103=telt;struct Cyc_Absyn_Tqual _tmp104=tq;struct Cyc_Absyn_Exp*_tmp105=({Cyc_Absyn_uint_exp(_get_fat_size(s,sizeof(char)),0U);});void*_tmp106=zt;unsigned _tmp107=ztl;_tmp102(_tmp103,_tmp104,_tmp105,_tmp106,_tmp107);});vd->type=_tmp254;});
goto _LL5D;}case 9U: _LL60: _tmp100=((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmpFB)->f1).Wstring_c).val;_LL61: {struct _fat_ptr s=_tmp100;
# 487
t=({void*_tmp255=({void*(*_tmp108)(void*elt_type,struct Cyc_Absyn_Tqual tq,struct Cyc_Absyn_Exp*num_elts,void*zero_term,unsigned ztloc)=Cyc_Absyn_array_type;void*_tmp109=telt;struct Cyc_Absyn_Tqual _tmp10A=tq;struct Cyc_Absyn_Exp*_tmp10B=({Cyc_Absyn_uint_exp(1U,0U);});void*_tmp10C=zt;unsigned _tmp10D=ztl;_tmp108(_tmp109,_tmp10A,_tmp10B,_tmp10C,_tmp10D);});vd->type=_tmp255;});
goto _LL5D;}default: goto _LL6A;}case 27U: _LL62: _tmpFF=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmpFB)->f2;_LL63: {struct Cyc_Absyn_Exp*e=_tmpFF;
_tmpFE=e;goto _LL65;}case 28U: _LL64: _tmpFE=((struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*)_tmpFB)->f1;_LL65: {struct Cyc_Absyn_Exp*e=_tmpFE;
# 492
t=({void*_tmp256=({Cyc_Absyn_array_type(telt,tq,e,zt,ztl);});vd->type=_tmp256;});
goto _LL5D;}case 37U: _LL66: _tmpFD=((struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*)_tmpFB)->f2;_LL67: {struct Cyc_List_List*es=_tmpFD;
_tmpFC=es;goto _LL69;}case 26U: _LL68: _tmpFC=((struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*)_tmpFB)->f1;_LL69: {struct Cyc_List_List*es=_tmpFC;
# 496
t=({void*_tmp257=({void*(*_tmp10E)(void*elt_type,struct Cyc_Absyn_Tqual tq,struct Cyc_Absyn_Exp*num_elts,void*zero_term,unsigned ztloc)=Cyc_Absyn_array_type;void*_tmp10F=telt;struct Cyc_Absyn_Tqual _tmp110=tq;struct Cyc_Absyn_Exp*_tmp111=({struct Cyc_Absyn_Exp*(*_tmp112)(unsigned,unsigned)=Cyc_Absyn_uint_exp;unsigned _tmp113=(unsigned)({Cyc_List_length(es);});unsigned _tmp114=0U;_tmp112(_tmp113,_tmp114);});void*_tmp115=zt;unsigned _tmp116=ztl;_tmp10E(_tmp10F,_tmp110,_tmp111,_tmp115,_tmp116);});vd->type=_tmp257;});
goto _LL5D;}default: _LL6A: _LL6B:
 goto _LL5D;}_LL5D:;}
# 500
goto _LL56;}}else{goto _LL5B;}}else{goto _LL5B;}default: _LL5B: _LL5C:
 goto _LL56;}_LL56:;}{
# 507
struct Cyc_List_List*bound_vars=!is_local?0:({Cyc_Tcenv_lookup_type_vars(te);});
int allow_evars=!is_local?0: 1;
({Cyc_Tctyp_check_type(s0->loc,te,bound_vars,& Cyc_Tcutil_tmk,allow_evars,1,t);});
({int _tmp258=({Cyc_Tcutil_extract_const_from_typedef(s0->loc,(vd->tq).print_const,t);});(vd->tq).real_const=_tmp258;});{
# 512
struct Cyc_Tcenv_Tenv*te2=te;
if((int)sc == (int)3U ||(int)sc == (int)4U)
({void*_tmp117=0U;({unsigned _tmp25A=d->loc;struct _fat_ptr _tmp259=({const char*_tmp118="extern declarations are not yet supported within functions";_tag_fat(_tmp118,sizeof(char),59U);});Cyc_Tcutil_terr(_tmp25A,_tmp259,_tag_fat(_tmp117,sizeof(void*),0U));});});
# 513
if(initializer != 0){
# 516
({Cyc_Tcexp_tcExpInitializer(te2,& t,initializer);});
# 518
if(!is_local && !({Cyc_Tcutil_is_const_exp(initializer);}))
({void*_tmp119=0U;({unsigned _tmp25C=d->loc;struct _fat_ptr _tmp25B=({const char*_tmp11A="initializer for static variable needs to be a constant expression";_tag_fat(_tmp11A,sizeof(char),66U);});Cyc_Tcutil_terr(_tmp25C,_tmp25B,_tag_fat(_tmp119,sizeof(void*),0U));});});
# 518
if(!({Cyc_Tcutil_coerce_assign(te2,initializer,t);})){
# 521
struct _fat_ptr varstr=*x;
struct _fat_ptr tstr=({Cyc_Absynpp_typ2string(t);});
struct _fat_ptr estr=({Cyc_Absynpp_typ2string((void*)_check_null(initializer->topt));});
if(((_get_fat_size(varstr,sizeof(char))+ _get_fat_size(tstr,sizeof(char)))+ _get_fat_size(estr,sizeof(char)))+ (unsigned)50 < (unsigned)80)
({struct Cyc_String_pa_PrintArg_struct _tmp11D=({struct Cyc_String_pa_PrintArg_struct _tmp1EB;_tmp1EB.tag=0U,_tmp1EB.f1=(struct _fat_ptr)((struct _fat_ptr)varstr);_tmp1EB;});struct Cyc_String_pa_PrintArg_struct _tmp11E=({struct Cyc_String_pa_PrintArg_struct _tmp1EA;_tmp1EA.tag=0U,_tmp1EA.f1=(struct _fat_ptr)((struct _fat_ptr)tstr);_tmp1EA;});struct Cyc_String_pa_PrintArg_struct _tmp11F=({struct Cyc_String_pa_PrintArg_struct _tmp1E9;_tmp1E9.tag=0U,_tmp1E9.f1=(struct _fat_ptr)((struct _fat_ptr)estr);_tmp1E9;});void*_tmp11B[3U];_tmp11B[0]=& _tmp11D,_tmp11B[1]=& _tmp11E,_tmp11B[2]=& _tmp11F;({unsigned _tmp25E=d->loc;struct _fat_ptr _tmp25D=({const char*_tmp11C="%s was declared with type %s but initialized with type %s.";_tag_fat(_tmp11C,sizeof(char),59U);});Cyc_Tcutil_terr(_tmp25E,_tmp25D,_tag_fat(_tmp11B,sizeof(void*),3U));});});else{
# 527
if((_get_fat_size(varstr,sizeof(char))+ _get_fat_size(tstr,sizeof(char)))+ (unsigned)25 < (unsigned)80 &&
 _get_fat_size(estr,sizeof(char))+ (unsigned)25 < (unsigned)80)
({struct Cyc_String_pa_PrintArg_struct _tmp122=({struct Cyc_String_pa_PrintArg_struct _tmp1EE;_tmp1EE.tag=0U,_tmp1EE.f1=(struct _fat_ptr)((struct _fat_ptr)varstr);_tmp1EE;});struct Cyc_String_pa_PrintArg_struct _tmp123=({struct Cyc_String_pa_PrintArg_struct _tmp1ED;_tmp1ED.tag=0U,_tmp1ED.f1=(struct _fat_ptr)((struct _fat_ptr)tstr);_tmp1ED;});struct Cyc_String_pa_PrintArg_struct _tmp124=({struct Cyc_String_pa_PrintArg_struct _tmp1EC;_tmp1EC.tag=0U,_tmp1EC.f1=(struct _fat_ptr)((struct _fat_ptr)estr);_tmp1EC;});void*_tmp120[3U];_tmp120[0]=& _tmp122,_tmp120[1]=& _tmp123,_tmp120[2]=& _tmp124;({unsigned _tmp260=d->loc;struct _fat_ptr _tmp25F=({const char*_tmp121="%s was declared with type %s\n but initialized with type %s.";_tag_fat(_tmp121,sizeof(char),60U);});Cyc_Tcutil_terr(_tmp260,_tmp25F,_tag_fat(_tmp120,sizeof(void*),3U));});});else{
# 532
({struct Cyc_String_pa_PrintArg_struct _tmp127=({struct Cyc_String_pa_PrintArg_struct _tmp1F1;_tmp1F1.tag=0U,_tmp1F1.f1=(struct _fat_ptr)((struct _fat_ptr)varstr);_tmp1F1;});struct Cyc_String_pa_PrintArg_struct _tmp128=({struct Cyc_String_pa_PrintArg_struct _tmp1F0;_tmp1F0.tag=0U,_tmp1F0.f1=(struct _fat_ptr)((struct _fat_ptr)tstr);_tmp1F0;});struct Cyc_String_pa_PrintArg_struct _tmp129=({struct Cyc_String_pa_PrintArg_struct _tmp1EF;_tmp1EF.tag=0U,_tmp1EF.f1=(struct _fat_ptr)((struct _fat_ptr)estr);_tmp1EF;});void*_tmp125[3U];_tmp125[0]=& _tmp127,_tmp125[1]=& _tmp128,_tmp125[2]=& _tmp129;({unsigned _tmp262=d->loc;struct _fat_ptr _tmp261=({const char*_tmp126="%s declared with type \n%s\n but initialized with type \n%s.";_tag_fat(_tmp126,sizeof(char),58U);});Cyc_Tcutil_terr(_tmp262,_tmp261,_tag_fat(_tmp125,sizeof(void*),3U));});});}}
# 534
({Cyc_Unify_unify(t,(void*)_check_null(initializer->topt));});
({Cyc_Unify_explain_failure();});}}
# 513
({Cyc_Tcstmt_tcStmt(te2,s,0);});
# 539
return;}}}}case 2U: _LL24: _tmpE6=((struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct*)_tmpDC)->f1;_tmpE7=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct*)_tmpDC)->f2;_tmpE8=(struct Cyc_Absyn_Exp**)&((struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct*)_tmpDC)->f3;_tmpE9=(void**)&((struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct*)_tmpDC)->f4;_LL25: {struct Cyc_Absyn_Pat*p=_tmpE6;struct Cyc_Core_Opt**vds=_tmpE7;struct Cyc_Absyn_Exp**exp=_tmpE8;void**dtp=_tmpE9;
# 546
struct Cyc_Absyn_Exp*e=*exp;
{void*_stmttmp8=p->r;void*_tmp12A=_stmttmp8;switch(*((int*)_tmp12A)){case 1U: _LL6D: _LL6E:
 goto _LL70;case 2U: _LL6F: _LL70:
 goto _LL72;case 15U: _LL71: _LL72:
 goto _LL6C;default: _LL73: _LL74:
# 552
 if(!({Cyc_Tcstmt_is_var_exp(e);})){
struct Cyc_Absyn_Vardecl*vd=({struct Cyc_Absyn_Vardecl*(*_tmp134)(unsigned varloc,struct _tuple2*,void*,struct Cyc_Absyn_Exp*init)=Cyc_Absyn_new_vardecl;unsigned _tmp135=0U;struct _tuple2*_tmp136=({Cyc_Tcstmt_stmt_temp_var();});void*_tmp137=({Cyc_Absyn_new_evar(0,0);});struct Cyc_Absyn_Exp*_tmp138=0;_tmp134(_tmp135,_tmp136,_tmp137,_tmp138);});
struct Cyc_Absyn_Stmt*s1=({Cyc_Absyn_new_stmt(s0->r,s0->loc);});
struct Cyc_Absyn_Decl*d=({struct Cyc_Absyn_Decl*(*_tmp12C)(struct Cyc_Absyn_Pat*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_let_decl;struct Cyc_Absyn_Pat*_tmp12D=({struct Cyc_Absyn_Pat*(*_tmp12E)(void*,unsigned)=Cyc_Absyn_new_pat;void*_tmp12F=(void*)({struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct*_tmp130=_cycalloc(sizeof(*_tmp130));_tmp130->tag=1U,_tmp130->f1=vd,({struct Cyc_Absyn_Pat*_tmp263=({Cyc_Absyn_new_pat((void*)& Cyc_Absyn_Wild_p_val,e->loc);});_tmp130->f2=_tmp263;});_tmp130;});unsigned _tmp131=e->loc;_tmp12E(_tmp12F,_tmp131);});struct Cyc_Absyn_Exp*_tmp132=e;unsigned _tmp133=s0->loc;_tmp12C(_tmp12D,_tmp132,_tmp133);});
# 558
struct Cyc_Absyn_Stmt*s2=({Cyc_Absyn_decl_stmt(d,s1,s0->loc);});
# 560
s0->r=s2->r;
({struct Cyc_Absyn_Exp*_tmp265=({({void*_tmp264=(void*)({struct Cyc_Absyn_Local_b_Absyn_Binding_struct*_tmp12B=_cycalloc(sizeof(*_tmp12B));_tmp12B->tag=4U,_tmp12B->f1=vd;_tmp12B;});Cyc_Absyn_varb_exp(_tmp264,e->loc);});});*exp=_tmp265;});
({Cyc_Tcstmt_tcStmt(te,s0,new_block);});
return;}}_LL6C:;}
# 566
({Cyc_Tcexp_tcExpInitializer(te,0,e);});{
# 568
void*pat_type=(void*)_check_null(e->topt);
# 570
struct Cyc_Tcpat_TcPatResult pat_res=({Cyc_Tcpat_tcPat(te,p,& pat_type,e);});
# 572
({struct Cyc_Core_Opt*_tmp266=({struct Cyc_Core_Opt*_tmp139=_cycalloc(sizeof(*_tmp139));_tmp139->v=pat_res.patvars;_tmp139;});*vds=_tmp266;});
if(!({({void*_tmp267=(void*)_check_null(p->topt);Cyc_Unify_unify(_tmp267,(void*)_check_null(e->topt));});})&& !({Cyc_Tcutil_coerce_assign(te,e,(void*)_check_null(p->topt));})){
# 575
({struct Cyc_String_pa_PrintArg_struct _tmp13C=({struct Cyc_String_pa_PrintArg_struct _tmp1F3;_tmp1F3.tag=0U,({
struct _fat_ptr _tmp268=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string((void*)_check_null(p->topt));}));_tmp1F3.f1=_tmp268;});_tmp1F3;});struct Cyc_String_pa_PrintArg_struct _tmp13D=({struct Cyc_String_pa_PrintArg_struct _tmp1F2;_tmp1F2.tag=0U,({struct _fat_ptr _tmp269=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string((void*)_check_null(e->topt));}));_tmp1F2.f1=_tmp269;});_tmp1F2;});void*_tmp13A[2U];_tmp13A[0]=& _tmp13C,_tmp13A[1]=& _tmp13D;({unsigned _tmp26B=d->loc;struct _fat_ptr _tmp26A=({const char*_tmp13B="pattern type %s does not match definition type %s";_tag_fat(_tmp13B,sizeof(char),50U);});Cyc_Tcutil_terr(_tmp26B,_tmp26A,_tag_fat(_tmp13A,sizeof(void*),2U));});});
({({void*_tmp26C=(void*)_check_null(p->topt);Cyc_Unify_unify(_tmp26C,(void*)_check_null(e->topt));});});
({Cyc_Unify_explain_failure();});}else{
# 581
({Cyc_Tcpat_check_pat_regions(te,p,pat_res.patvars);});}
({Cyc_Tcpat_check_let_pat_exhaustive(p->loc,te,p,dtp);});
({Cyc_Tcstmt_pattern_synth(s0->loc,te,pat_res,s,0,0);});
return;}}case 3U: _LL26: _tmpE5=((struct Cyc_Absyn_Letv_d_Absyn_Raw_decl_struct*)_tmpDC)->f1;_LL27: {struct Cyc_List_List*vds=_tmpE5;
# 587
void*curr_bl=({Cyc_Tcenv_curr_rgn(te);});
struct Cyc_Tcenv_Tenv*te2=te;
for(0;vds != 0;vds=vds->tl){
struct Cyc_Absyn_Vardecl*vd=(struct Cyc_Absyn_Vardecl*)vds->hd;
void**_tmp140;void*_tmp13F;union Cyc_Absyn_Nmspace _tmp13E;_LL76: _tmp13E=(vd->name)->f1;_tmp13F=vd->type;_tmp140=(void**)& vd->rgn;_LL77: {union Cyc_Absyn_Nmspace nsl=_tmp13E;void*t=_tmp13F;void**rgn_ptr=_tmp140;
*rgn_ptr=curr_bl;
({void(*_tmp141)(unsigned,struct Cyc_Tcenv_Tenv*,struct Cyc_List_List*bound_tvars,struct Cyc_Absyn_Kind*k,int allow_evars,int allow_abs_aggr,void*)=Cyc_Tctyp_check_type;unsigned _tmp142=s0->loc;struct Cyc_Tcenv_Tenv*_tmp143=te2;struct Cyc_List_List*_tmp144=({Cyc_Tcenv_lookup_type_vars(te2);});struct Cyc_Absyn_Kind*_tmp145=& Cyc_Tcutil_tmk;int _tmp146=1;int _tmp147=1;void*_tmp148=t;_tmp141(_tmp142,_tmp143,_tmp144,_tmp145,_tmp146,_tmp147,_tmp148);});}}
# 595
({Cyc_Tcstmt_tcStmt(te2,s,0);});
return;}case 4U: _LL28: _tmpE2=((struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*)_tmpDC)->f1;_tmpE3=((struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*)_tmpDC)->f2;_tmpE4=((struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*)_tmpDC)->f3;_LL29: {struct Cyc_Absyn_Tvar*tv=_tmpE2;struct Cyc_Absyn_Vardecl*vd=_tmpE3;struct Cyc_Absyn_Exp*open_exp_opt=_tmpE4;
# 601
({void*_tmp26F=({struct Cyc_Absyn_Stmt*(*_tmp149)(void*,unsigned)=Cyc_Absyn_new_stmt;void*_tmp14A=(void*)({struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*_tmp14B=_cycalloc(sizeof(*_tmp14B));_tmp14B->tag=2U,({struct Cyc_Absyn_Stmt*_tmp26E=({Cyc_Absyn_new_stmt(s->r,s->loc);});_tmp14B->f1=_tmp26E;}),({struct Cyc_Absyn_Stmt*_tmp26D=({Cyc_Absyn_skip_stmt(s->loc);});_tmp14B->f2=_tmp26D;});_tmp14B;});unsigned _tmp14C=s->loc;_tmp149(_tmp14A,_tmp14C);})->r;s->r=_tmp26F;});{
struct Cyc_Tcenv_Tenv*te2=te;
void**_tmp14E;void**_tmp14D;_LL79: _tmp14D=(void**)& vd->type;_tmp14E=(void**)& vd->rgn;_LL7A: {void**t=_tmp14D;void**rgn_ptr=_tmp14E;
void*curr_bl=({Cyc_Tcenv_curr_rgn(te);});
*rgn_ptr=curr_bl;{
void*rgn_typ;
if((unsigned)open_exp_opt){
# 609
int is__xrgn=({Cyc_Absyn_is_xrgn_tvar(tv);});
int is_xopen=tv->identity == - 2;
# 614
if(!is__xrgn && !is_xopen){
# 616
struct _tuple2*drname=({struct _tuple2*_tmp17B=_cycalloc(sizeof(*_tmp17B));((_tmp17B->f1).Abs_n).tag=2U,({struct Cyc_List_List*_tmp274=({struct _fat_ptr*_tmp176[1U];({struct _fat_ptr*_tmp273=({struct _fat_ptr*_tmp178=_cycalloc(sizeof(*_tmp178));({struct _fat_ptr _tmp272=({const char*_tmp177="Core";_tag_fat(_tmp177,sizeof(char),5U);});*_tmp178=_tmp272;});_tmp178;});_tmp176[0]=_tmp273;});Cyc_List_list(_tag_fat(_tmp176,sizeof(struct _fat_ptr*),1U));});((_tmp17B->f1).Abs_n).val=_tmp274;}),({struct _fat_ptr*_tmp271=({struct _fat_ptr*_tmp17A=_cycalloc(sizeof(*_tmp17A));({struct _fat_ptr _tmp270=({const char*_tmp179="DynamicRegion";_tag_fat(_tmp179,sizeof(char),14U);});*_tmp17A=_tmp270;});_tmp17A;});_tmp17B->f2=_tmp271;});_tmp17B;});
void*outer_rgn=({Cyc_Absyn_new_evar(({struct Cyc_Core_Opt*_tmp175=_cycalloc(sizeof(*_tmp175));_tmp175->v=& Cyc_Tcutil_trk;_tmp175;}),0);});
rgn_typ=({Cyc_Absyn_new_evar(({struct Cyc_Core_Opt*_tmp14F=_cycalloc(sizeof(*_tmp14F));_tmp14F->v=& Cyc_Tcutil_rk;_tmp14F;}),0);});{
void*dr_type=({void*(*_tmp171)(union Cyc_Absyn_AggrInfo,struct Cyc_List_List*args)=Cyc_Absyn_aggr_type;union Cyc_Absyn_AggrInfo _tmp172=({Cyc_Absyn_UnknownAggr(Cyc_Absyn_StructA,drname,0);});struct Cyc_List_List*_tmp173=({void*_tmp174[1U];_tmp174[0]=rgn_typ;Cyc_List_list(_tag_fat(_tmp174,sizeof(void*),1U));});_tmp171(_tmp172,_tmp173);});
void*exp_type=({void*(*_tmp16C)(void*t,void*rgn,struct Cyc_Absyn_Tqual,void*zero_term)=Cyc_Absyn_at_type;void*_tmp16D=dr_type;void*_tmp16E=outer_rgn;struct Cyc_Absyn_Tqual _tmp16F=({Cyc_Absyn_empty_tqual(0U);});void*_tmp170=Cyc_Absyn_false_type;_tmp16C(_tmp16D,_tmp16E,_tmp16F,_tmp170);});
({void(*_tmp150)(unsigned,struct Cyc_Tcenv_Tenv*,struct Cyc_List_List*bound_tvars,struct Cyc_Absyn_Kind*k,int allow_evars,int allow_abs_aggr,void*)=Cyc_Tctyp_check_type;unsigned _tmp151=s0->loc;struct Cyc_Tcenv_Tenv*_tmp152=te;struct Cyc_List_List*_tmp153=({Cyc_Tcenv_lookup_type_vars(te);});struct Cyc_Absyn_Kind*_tmp154=& Cyc_Tcutil_tmk;int _tmp155=1;int _tmp156=0;void*_tmp157=exp_type;_tmp150(_tmp151,_tmp152,_tmp153,_tmp154,_tmp155,_tmp156,_tmp157);});{
void*key_typ=({Cyc_Tcexp_tcExp(te,& exp_type,open_exp_opt);});
if(!({Cyc_Unify_unify(exp_type,key_typ);})&& !({Cyc_Tcutil_coerce_assign(te,open_exp_opt,exp_type);}))
({struct Cyc_String_pa_PrintArg_struct _tmp15A=({struct Cyc_String_pa_PrintArg_struct _tmp1F5;_tmp1F5.tag=0U,({struct _fat_ptr _tmp275=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(exp_type);}));_tmp1F5.f1=_tmp275;});_tmp1F5;});struct Cyc_String_pa_PrintArg_struct _tmp15B=({struct Cyc_String_pa_PrintArg_struct _tmp1F4;_tmp1F4.tag=0U,({struct _fat_ptr _tmp276=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(key_typ);}));_tmp1F4.f1=_tmp276;});_tmp1F4;});void*_tmp158[2U];_tmp158[0]=& _tmp15A,_tmp158[1]=& _tmp15B;({unsigned _tmp278=s0->loc;struct _fat_ptr _tmp277=({const char*_tmp159="expected %s but found %s";_tag_fat(_tmp159,sizeof(char),25U);});Cyc_Tcutil_terr(_tmp278,_tmp277,_tag_fat(_tmp158,sizeof(void*),2U));});});
# 623
if(
# 625
!({Cyc_Unify_unify(outer_rgn,Cyc_Absyn_unique_rgn_type);})&& !({Cyc_Unify_unify(outer_rgn,Cyc_Absyn_refcnt_rgn_type);}))
({void*_tmp15C=0U;({unsigned _tmp27A=s0->loc;struct _fat_ptr _tmp279=({const char*_tmp15D="open is only allowed on unique or reference-counted keys";_tag_fat(_tmp15D,sizeof(char),57U);});Cyc_Tcutil_terr(_tmp27A,_tmp279,_tag_fat(_tmp15C,sizeof(void*),0U));});});
# 623
({void*_tmp27B=({Cyc_Absyn_rgn_handle_type(rgn_typ);});*t=_tmp27B;});
# 628
te2=({Cyc_Tcenv_add_region(te2,rgn_typ,1);});
({void(*_tmp15E)(unsigned,struct Cyc_Tcenv_Tenv*,struct Cyc_List_List*bound_tvars,struct Cyc_Absyn_Kind*k,int allow_evars,int allow_abs_aggr,void*)=Cyc_Tctyp_check_type;unsigned _tmp15F=s0->loc;struct Cyc_Tcenv_Tenv*_tmp160=te;struct Cyc_List_List*_tmp161=({Cyc_Tcenv_lookup_type_vars(te2);});struct Cyc_Absyn_Kind*_tmp162=& Cyc_Tcutil_bk;int _tmp163=1;int _tmp164=0;void*_tmp165=*t;_tmp15E(_tmp15F,_tmp160,_tmp161,_tmp162,_tmp163,_tmp164,_tmp165);});
if(!({int(*_tmp166)(void*,void*)=Cyc_Unify_unify;void*_tmp167=*t;void*_tmp168=({Cyc_Absyn_rgn_handle_type(rgn_typ);});_tmp166(_tmp167,_tmp168);}))
({void*_tmp169=0U;({int(*_tmp27D)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp16B)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp16B;});struct _fat_ptr _tmp27C=({const char*_tmp16A="region stmt: type of region handle is wrong!";_tag_fat(_tmp16A,sizeof(char),45U);});_tmp27D(_tmp27C,_tag_fat(_tmp169,sizeof(void*),0U));});});}}}else{
# 633
if(!is__xrgn && is_xopen){
# 636
if(({Cyc_Absyn_no_side_effects_exp(open_exp_opt);}))({Cyc_Absyn_forgive_global_set(1);});{void*typ=({Cyc_Tcexp_tcExp(te,0,open_exp_opt);});
# 638
({Cyc_Absyn_forgive_global_set(0);});{
# 640
void*r0=Cyc_Absyn_void_type;
{void*_tmp17C=typ;void*_tmp17D;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp17C)->tag == 0U){if(((struct Cyc_Absyn_RgnHandleCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp17C)->f1)->tag == 3U){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp17C)->f2 != 0){_LL7C: _tmp17D=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp17C)->f2)->hd;_LL7D: {void*r=_tmp17D;
# 644
if(!({Cyc_Absyn_is_xrgn(r);}))goto _LL7F;r0=r;
# 646
goto _LL7B;}}else{goto _LL7E;}}else{goto _LL7E;}}else{_LL7E: _LL7F:
# 648
({void*_tmp17E=0U;({unsigned _tmp27F=s0->loc;struct _fat_ptr _tmp27E=({const char*_tmp17F="`xopen' requires the an extended region handle";_tag_fat(_tmp17F,sizeof(char),47U);});Cyc_Tcutil_terr(_tmp27F,_tmp27E,_tag_fat(_tmp17E,sizeof(void*),0U));});});}_LL7B:;}
# 651
rgn_typ=(void*)({struct Cyc_Absyn_VarType_Absyn_Type_struct*_tmp180=_cycalloc(sizeof(*_tmp180));_tmp180->tag=2U,_tmp180->f1=tv;_tmp180;});
({void*_tmp280=({Cyc_Tcutil_kind_to_bound(& Cyc_Tcutil_rk);});tv->kind=_tmp280;});
te2=({({unsigned _tmp282=s0->loc;struct Cyc_Tcenv_Tenv*_tmp281=te2;Cyc_Tcenv_add_type_vars(_tmp282,_tmp281,({struct Cyc_List_List*_tmp181=_cycalloc(sizeof(*_tmp181));_tmp181->hd=tv,_tmp181->tl=0;_tmp181;}));});});
te2=({Cyc_Tcenv_add_region(te2,rgn_typ,1);});
({void(*_tmp182)(unsigned,struct Cyc_Tcenv_Tenv*,struct Cyc_List_List*bound_tvars,struct Cyc_Absyn_Kind*k,int allow_evars,int allow_abs_aggr,void*)=Cyc_Tctyp_check_type;unsigned _tmp183=s0->loc;struct Cyc_Tcenv_Tenv*_tmp184=te;struct Cyc_List_List*_tmp185=({Cyc_Tcenv_lookup_type_vars(te2);});struct Cyc_Absyn_Kind*_tmp186=& Cyc_Tcutil_bk;int _tmp187=1;int _tmp188=0;void*_tmp189=*t;_tmp182(_tmp183,_tmp184,_tmp185,_tmp186,_tmp187,_tmp188,_tmp189);});
# 657
if(!({int(*_tmp18A)(void*,void*)=Cyc_Unify_unify;void*_tmp18B=*t;void*_tmp18C=({Cyc_Absyn_rgn_handle_type(rgn_typ);});_tmp18A(_tmp18B,_tmp18C);}))
({void*_tmp18D=0U;({int(*_tmp284)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp18F)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp18F;});struct _fat_ptr _tmp283=({const char*_tmp18E="region stmt: type of region handle is wrong!";_tag_fat(_tmp18E,sizeof(char),45U);});_tmp284(_tmp283,_tag_fat(_tmp18D,sizeof(void*),0U));});});
# 657
te2=({Cyc_Tcenv_new_outlives_constraint_special(te2,rgn_typ,r0);});}}}else{
# 662
if(is__xrgn){
# 672 "tcstmt.cyc"
if(({Cyc_Absyn_no_side_effects_exp(open_exp_opt);}))({Cyc_Absyn_forgive_global_set(1);});{void*typ=({Cyc_Tcexp_tcExp(te,0,open_exp_opt);});
# 674
({Cyc_Absyn_forgive_global_set(0);});
# 680
{void*_tmp190=typ;void*_tmp191;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp190)->tag == 0U){if(((struct Cyc_Absyn_RgnHandleCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp190)->f1)->tag == 3U){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp190)->f2 != 0){_LL81: _tmp191=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp190)->f2)->hd;_LL82: {void*r=_tmp191;
# 684
if(!({Cyc_Absyn_is_xrgn(r);})&& r != Cyc_Absyn_heap_rgn_type)
# 686
({void*_tmp192=0U;({unsigned _tmp286=s0->loc;struct _fat_ptr _tmp285=({const char*_tmp193="The parent of an extended region can only be the heap or another extended region";_tag_fat(_tmp193,sizeof(char),81U);});Cyc_Tcutil_terr(_tmp286,_tmp285,_tag_fat(_tmp192,sizeof(void*),0U));});});
# 684
goto _LL80;}}else{goto _LL83;}}else{goto _LL83;}}else{_LL83: _LL84:
# 689
 goto _LL80;}_LL80:;}
# 691
rgn_typ=(void*)({struct Cyc_Absyn_VarType_Absyn_Type_struct*_tmp194=_cycalloc(sizeof(*_tmp194));_tmp194->tag=2U,_tmp194->f1=tv;_tmp194;});
({void*_tmp287=({Cyc_Tcutil_kind_to_bound(& Cyc_Tcutil_xrk);});tv->kind=_tmp287;});
te2=({({unsigned _tmp289=s0->loc;struct Cyc_Tcenv_Tenv*_tmp288=te2;Cyc_Tcenv_add_type_vars(_tmp289,_tmp288,({struct Cyc_List_List*_tmp195=_cycalloc(sizeof(*_tmp195));_tmp195->hd=tv,_tmp195->tl=0;_tmp195;}));});});
te2=({Cyc_Tcenv_add_region(te2,rgn_typ,1);});
({void(*_tmp196)(unsigned,struct Cyc_Tcenv_Tenv*,struct Cyc_List_List*bound_tvars,struct Cyc_Absyn_Kind*k,int allow_evars,int allow_abs_aggr,void*)=Cyc_Tctyp_check_type;unsigned _tmp197=s0->loc;struct Cyc_Tcenv_Tenv*_tmp198=te;struct Cyc_List_List*_tmp199=({Cyc_Tcenv_lookup_type_vars(te2);});struct Cyc_Absyn_Kind*_tmp19A=& Cyc_Tcutil_bk;int _tmp19B=1;int _tmp19C=0;void*_tmp19D=*t;_tmp196(_tmp197,_tmp198,_tmp199,_tmp19A,_tmp19B,_tmp19C,_tmp19D);});
if(!({int(*_tmp19E)(void*,void*)=Cyc_Unify_unify;void*_tmp19F=*t;void*_tmp1A0=({Cyc_Absyn_rgn_handle_type(rgn_typ);});_tmp19E(_tmp19F,_tmp1A0);}))
({void*_tmp1A1=0U;({int(*_tmp28B)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp1A3)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp1A3;});struct _fat_ptr _tmp28A=({const char*_tmp1A2="region stmt: type of xregion handle is wrong!";_tag_fat(_tmp1A2,sizeof(char),46U);});_tmp28B(_tmp28A,_tag_fat(_tmp1A1,sizeof(void*),0U));});});}}else{
# 699
({struct Cyc_Int_pa_PrintArg_struct _tmp1A7=({struct Cyc_Int_pa_PrintArg_struct _tmp1F7;_tmp1F7.tag=1U,_tmp1F7.f1=(unsigned long)is__xrgn;_tmp1F7;});struct Cyc_Int_pa_PrintArg_struct _tmp1A8=({struct Cyc_Int_pa_PrintArg_struct _tmp1F6;_tmp1F6.tag=1U,_tmp1F6.f1=(unsigned long)is_xopen;_tmp1F6;});void*_tmp1A4[2U];_tmp1A4[0]=& _tmp1A7,_tmp1A4[1]=& _tmp1A8;({int(*_tmp28D)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp1A6)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp1A6;});struct _fat_ptr _tmp28C=({const char*_tmp1A5="region stmt: could not match any case : is_xrgn=%d is_xopen=%d!";_tag_fat(_tmp1A5,sizeof(char),64U);});_tmp28D(_tmp28C,_tag_fat(_tmp1A4,sizeof(void*),2U));});});}}}}else{
# 705
rgn_typ=(void*)({struct Cyc_Absyn_VarType_Absyn_Type_struct*_tmp1A9=_cycalloc(sizeof(*_tmp1A9));_tmp1A9->tag=2U,_tmp1A9->f1=tv;_tmp1A9;});
te2=({({unsigned _tmp28F=s0->loc;struct Cyc_Tcenv_Tenv*_tmp28E=te2;Cyc_Tcenv_add_type_vars(_tmp28F,_tmp28E,({struct Cyc_List_List*_tmp1AA=_cycalloc(sizeof(*_tmp1AA));_tmp1AA->hd=tv,_tmp1AA->tl=0;_tmp1AA;}));});});
te2=({Cyc_Tcenv_add_region(te2,rgn_typ,0);});
({void(*_tmp1AB)(unsigned,struct Cyc_Tcenv_Tenv*,struct Cyc_List_List*bound_tvars,struct Cyc_Absyn_Kind*k,int allow_evars,int allow_abs_aggr,void*)=Cyc_Tctyp_check_type;unsigned _tmp1AC=s0->loc;struct Cyc_Tcenv_Tenv*_tmp1AD=te;struct Cyc_List_List*_tmp1AE=({Cyc_Tcenv_lookup_type_vars(te2);});struct Cyc_Absyn_Kind*_tmp1AF=& Cyc_Tcutil_bk;int _tmp1B0=1;int _tmp1B1=0;void*_tmp1B2=*t;_tmp1AB(_tmp1AC,_tmp1AD,_tmp1AE,_tmp1AF,_tmp1B0,_tmp1B1,_tmp1B2);});
if(!({int(*_tmp1B3)(void*,void*)=Cyc_Unify_unify;void*_tmp1B4=*t;void*_tmp1B5=({Cyc_Absyn_rgn_handle_type(rgn_typ);});_tmp1B3(_tmp1B4,_tmp1B5);}))
({void*_tmp1B6=0U;({int(*_tmp291)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp1B8)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp1B8;});struct _fat_ptr _tmp290=({const char*_tmp1B7="region stmt: type of region handle is wrong!";_tag_fat(_tmp1B7,sizeof(char),45U);});_tmp291(_tmp290,_tag_fat(_tmp1B6,sizeof(void*),0U));});});}
# 713
({Cyc_Tcstmt_tcStmt(te2,s,0);});
return;}}}}case 1U: _LL2A: _tmpE1=((struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*)_tmpDC)->f1;_LL2B: {struct Cyc_Absyn_Fndecl*fd=_tmpE1;
# 718
({Cyc_Tcstmt_tcFndecl_inner(te,d->loc,fd);});
# 752 "tcstmt.cyc"
({Cyc_Tcstmt_tcStmt(te,s,0);});
return;}case 9U: _LL2C: _tmpDF=((struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct*)_tmpDC)->f1;_tmpE0=((struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct*)_tmpDC)->f2;_LL2D: {struct _fat_ptr*n=_tmpDF;struct Cyc_List_List*tds=_tmpE0;
unimp_msg_part=({const char*_tmp1B9="namespace";_tag_fat(_tmp1B9,sizeof(char),10U);});goto _LL21;}case 10U: _LL2E: _tmpDD=((struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct*)_tmpDC)->f1;_tmpDE=((struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct*)_tmpDC)->f2;_LL2F: {struct _tuple2*q=_tmpDD;struct Cyc_List_List*tds=_tmpDE;
unimp_msg_part=({const char*_tmp1BA="using";_tag_fat(_tmp1BA,sizeof(char),6U);});goto _LL21;}case 5U: _LL30: _LL31:
 unimp_msg_part=({const char*_tmp1BB="type";_tag_fat(_tmp1BB,sizeof(char),5U);});goto _LL21;case 6U: _LL32: _LL33:
 unimp_msg_part=({const char*_tmp1BC="datatype";_tag_fat(_tmp1BC,sizeof(char),9U);});goto _LL21;case 7U: _LL34: _LL35:
 unimp_msg_part=({const char*_tmp1BD="enum";_tag_fat(_tmp1BD,sizeof(char),5U);});goto _LL21;case 8U: _LL36: _LL37:
 unimp_msg_part=({const char*_tmp1BE="typedef";_tag_fat(_tmp1BE,sizeof(char),8U);});goto _LL21;case 11U: _LL38: _LL39:
 unimp_msg_part=({const char*_tmp1BF="extern \"C\"";_tag_fat(_tmp1BF,sizeof(char),11U);});goto _LL21;case 12U: _LL3A: _LL3B:
# 762
 unimp_msg_part=({const char*_tmp1C0="extern \"C include\"";_tag_fat(_tmp1C0,sizeof(char),19U);});goto _LL21;case 13U: _LL3C: _LL3D:
 unimp_msg_part=({const char*_tmp1C1="__cyclone_port_on__";_tag_fat(_tmp1C1,sizeof(char),20U);});goto _LL21;case 14U: _LL3E: _LL3F:
 unimp_msg_part=({const char*_tmp1C2="__cyclone_port_off__";_tag_fat(_tmp1C2,sizeof(char),21U);});goto _LL21;case 15U: _LL40: _LL41:
 unimp_msg_part=({const char*_tmp1C3="__tempest_on__";_tag_fat(_tmp1C3,sizeof(char),15U);});goto _LL21;default: _LL42: _LL43:
 unimp_msg_part=({const char*_tmp1C4="__tempest_off__";_tag_fat(_tmp1C4,sizeof(char),16U);});goto _LL21;}_LL21:;}
# 768
(int)_throw(({struct Cyc_String_pa_PrintArg_struct _tmp1C7=({struct Cyc_String_pa_PrintArg_struct _tmp1F8;_tmp1F8.tag=0U,_tmp1F8.f1=(struct _fat_ptr)((struct _fat_ptr)unimp_msg_part);_tmp1F8;});void*_tmp1C5[1U];_tmp1C5[0]=& _tmp1C7;({struct _fat_ptr _tmp292=({const char*_tmp1C6="tcStmt: nested %s declarations unimplemented";_tag_fat(_tmp1C6,sizeof(char),45U);});Cyc_Tcutil_impos(_tmp292,_tag_fat(_tmp1C5,sizeof(void*),1U));});}));}}_LL0:;}
# 186 "tcstmt.cyc"
void*Cyc_Tcstmt_tcFndecl_valid_type(struct Cyc_Tcenv_Tenv*te,unsigned loc,struct Cyc_Absyn_Fndecl*fd){
# 776 "tcstmt.cyc"
{struct Cyc_List_List*atts=(fd->i).attributes;for(0;atts != 0;atts=atts->tl){
void*_stmttmp9=(void*)atts->hd;void*_tmp1C9=_stmttmp9;switch(*((int*)_tmp1C9)){case 7U: _LL1: _LL2:
 goto _LL4;case 6U: _LL3: _LL4:
# 780
({struct Cyc_String_pa_PrintArg_struct _tmp1CC=({struct Cyc_String_pa_PrintArg_struct _tmp1F9;_tmp1F9.tag=0U,({struct _fat_ptr _tmp293=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_attribute2string((void*)atts->hd);}));_tmp1F9.f1=_tmp293;});_tmp1F9;});void*_tmp1CA[1U];_tmp1CA[0]=& _tmp1CC;({unsigned _tmp295=loc;struct _fat_ptr _tmp294=({const char*_tmp1CB="bad attribute %s for function";_tag_fat(_tmp1CB,sizeof(char),30U);});Cyc_Tcutil_terr(_tmp295,_tmp294,_tag_fat(_tmp1CA,sizeof(void*),1U));});});
goto _LL0;default: _LL5: _LL6:
 goto _LL0;}_LL0:;}}
# 785
({Cyc_Tctyp_check_fndecl_valid_type(loc,te,fd);});{
void*t=({Cyc_Tcutil_fndecl2type(fd);});
# 788
({struct Cyc_List_List*_tmp296=({Cyc_Tcutil_transfer_fn_type_atts(t,(fd->i).attributes);});(fd->i).attributes=_tmp296;});
return t;}}
# 792
static void Cyc_Tcstmt_tcFndecl_inner(struct Cyc_Tcenv_Tenv*te,unsigned loc,struct Cyc_Absyn_Fndecl*fd){
# 794
if((int)fd->sc != (int)Cyc_Absyn_Public)({void*_tmp1CE=0U;({unsigned _tmp298=loc;struct _fat_ptr _tmp297=({const char*_tmp1CF="bad storage class for inner function";_tag_fat(_tmp1CF,sizeof(char),37U);});Cyc_Tcutil_terr(_tmp298,_tmp297,_tag_fat(_tmp1CE,sizeof(void*),0U));});});{void*t=({Cyc_Tcstmt_tcFndecl_valid_type(te,loc,fd);});
# 796
void*curr_bl=({Cyc_Tcenv_curr_rgn(te);});
struct Cyc_Absyn_Vardecl*vd=({struct Cyc_Absyn_Vardecl*_tmp1D5=_cycalloc(sizeof(*_tmp1D5));_tmp1D5->sc=fd->sc,_tmp1D5->name=fd->name,_tmp1D5->varloc=0U,({struct Cyc_Absyn_Tqual _tmp29A=({Cyc_Absyn_const_tqual(0U);});_tmp1D5->tq=_tmp29A;}),({
void*_tmp299=({void*(*_tmp1D0)(void*t,void*rgn,struct Cyc_Absyn_Tqual,void*zero_term)=Cyc_Absyn_at_type;void*_tmp1D1=t;void*_tmp1D2=curr_bl;struct Cyc_Absyn_Tqual _tmp1D3=({Cyc_Absyn_empty_tqual(0U);});void*_tmp1D4=Cyc_Absyn_false_type;_tmp1D0(_tmp1D1,_tmp1D2,_tmp1D3,_tmp1D4);});_tmp1D5->type=_tmp299;}),_tmp1D5->initializer=0,_tmp1D5->rgn=curr_bl,_tmp1D5->attributes=0,_tmp1D5->escapes=0;_tmp1D5;});
# 801
fd->fn_vardecl=vd;{
struct Cyc_Tcenv_Fenv*old_fenv=(struct Cyc_Tcenv_Fenv*)_check_null(te->le);
({struct Cyc_Tcenv_Fenv*_tmp29B=({Cyc_Tcenv_nested_fenv(loc,old_fenv,fd,t);});te->le=_tmp29B;});
({Cyc_Tcstmt_tcFndecl_check_body(loc,te,t,fd);});
te->le=old_fenv;}}}
# 808
void Cyc_Tcstmt_tcFndecl_check_body(unsigned loc,struct Cyc_Tcenv_Tenv*te,void*t,struct Cyc_Absyn_Fndecl*fd){
# 810
void*f=(fd->i).effect;
if(f != 0 &&({Cyc_Absyn_get_debug();})){
# 813
({struct Cyc_String_pa_PrintArg_struct _tmp1D9=({struct Cyc_String_pa_PrintArg_struct _tmp1FC;_tmp1FC.tag=0U,({
struct _fat_ptr _tmp29C=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Position_string_of_loc(loc);}));_tmp1FC.f1=_tmp29C;});_tmp1FC;});struct Cyc_String_pa_PrintArg_struct _tmp1DA=({struct Cyc_String_pa_PrintArg_struct _tmp1FB;_tmp1FB.tag=0U,_tmp1FB.f1=(struct _fat_ptr)((struct _fat_ptr)*(*fd->name).f2);_tmp1FB;});struct Cyc_String_pa_PrintArg_struct _tmp1DB=({struct Cyc_String_pa_PrintArg_struct _tmp1FA;_tmp1FA.tag=0U,({
# 816
struct _fat_ptr _tmp29D=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(f);}));_tmp1FA.f1=_tmp29D;});_tmp1FA;});void*_tmp1D7[3U];_tmp1D7[0]=& _tmp1D9,_tmp1D7[1]=& _tmp1DA,_tmp1D7[2]=& _tmp1DB;({struct Cyc___cycFILE*_tmp29F=Cyc_stderr;struct _fat_ptr _tmp29E=({const char*_tmp1D8="\nFunction (%s) %s ==>  effect : %s";_tag_fat(_tmp1D8,sizeof(char),35U);});Cyc_fprintf(_tmp29F,_tmp29E,_tag_fat(_tmp1D7,sizeof(void*),3U));});});
({Cyc_fflush(Cyc_stderr);});}
# 811
({Cyc_Tcstmt_tcStmt(te,fd->body,0);});
# 824
({Cyc_Tcenv_check_delayed_effects(te);});
({Cyc_Tcenv_check_delayed_constraints(te);});}

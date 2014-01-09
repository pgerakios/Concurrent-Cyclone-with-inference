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
struct _fat_ptr Cyc_Core_new_string(unsigned);extern char Cyc_Core_Invalid_argument[17U];struct Cyc_Core_Invalid_argument_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Failure[8U];struct Cyc_Core_Failure_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Impossible[11U];struct Cyc_Core_Impossible_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Not_found[10U];struct Cyc_Core_Not_found_exn_struct{char*tag;};extern char Cyc_Core_Unreachable[12U];struct Cyc_Core_Unreachable_exn_struct{char*tag;struct _fat_ptr f1;};
# 168
extern struct _RegionHandle*Cyc_Core_unique_region;struct Cyc_Core_DynamicRegion;struct Cyc_Core_NewDynamicRegion{struct Cyc_Core_DynamicRegion*key;};struct Cyc_Core_ThinRes{void*arr;unsigned nelts;};struct Cyc___cycFILE;
# 53 "cycboot.h"
extern struct Cyc___cycFILE*Cyc_stderr;struct Cyc_String_pa_PrintArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Int_pa_PrintArg_struct{int tag;unsigned long f1;};struct Cyc_Double_pa_PrintArg_struct{int tag;double f1;};struct Cyc_LongDouble_pa_PrintArg_struct{int tag;long double f1;};struct Cyc_ShortPtr_pa_PrintArg_struct{int tag;short*f1;};struct Cyc_IntPtr_pa_PrintArg_struct{int tag;unsigned long*f1;};
# 73
struct _fat_ptr Cyc_aprintf(struct _fat_ptr,struct _fat_ptr);
# 100
int Cyc_fprintf(struct Cyc___cycFILE*,struct _fat_ptr,struct _fat_ptr);struct Cyc_ShortPtr_sa_ScanfArg_struct{int tag;short*f1;};struct Cyc_UShortPtr_sa_ScanfArg_struct{int tag;unsigned short*f1;};struct Cyc_IntPtr_sa_ScanfArg_struct{int tag;int*f1;};struct Cyc_UIntPtr_sa_ScanfArg_struct{int tag;unsigned*f1;};struct Cyc_StringPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_DoublePtr_sa_ScanfArg_struct{int tag;double*f1;};struct Cyc_FloatPtr_sa_ScanfArg_struct{int tag;float*f1;};struct Cyc_CharPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};extern char Cyc_FileCloseError[15U];struct Cyc_FileCloseError_exn_struct{char*tag;};extern char Cyc_FileOpenError[14U];struct Cyc_FileOpenError_exn_struct{char*tag;struct _fat_ptr f1;};struct Cyc_Buffer_t;struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};
# 76 "list.h"
struct Cyc_List_List*Cyc_List_map(void*(*f)(void*),struct Cyc_List_List*x);
# 83
struct Cyc_List_List*Cyc_List_map_c(void*(*f)(void*,void*),void*env,struct Cyc_List_List*x);extern char Cyc_List_List_mismatch[14U];struct Cyc_List_List_mismatch_exn_struct{char*tag;};
# 133
void Cyc_List_iter(void(*f)(void*),struct Cyc_List_List*x);
# 178
struct Cyc_List_List*Cyc_List_imp_rev(struct Cyc_List_List*x);
# 184
struct Cyc_List_List*Cyc_List_append(struct Cyc_List_List*x,struct Cyc_List_List*y);
# 195
struct Cyc_List_List*Cyc_List_imp_append(struct Cyc_List_List*x,struct Cyc_List_List*y);extern char Cyc_List_Nth[4U];struct Cyc_List_Nth_exn_struct{char*tag;};
# 258
int Cyc_List_exists(int(*pred)(void*),struct Cyc_List_List*x);
# 322
int Cyc_List_mem(int(*compare)(void*,void*),struct Cyc_List_List*l,void*x);
# 383
int Cyc_List_list_cmp(int(*cmp)(void*,void*),struct Cyc_List_List*l1,struct Cyc_List_List*l2);
# 387
int Cyc_List_list_prefix(int(*cmp)(void*,void*),struct Cyc_List_List*l1,struct Cyc_List_List*l2);
# 38 "string.h"
unsigned long Cyc_strlen(struct _fat_ptr s);
# 50 "string.h"
int Cyc_strptrcmp(struct _fat_ptr*s1,struct _fat_ptr*s2);
# 62
struct _fat_ptr Cyc_strconcat(struct _fat_ptr,struct _fat_ptr);
# 66
struct _fat_ptr Cyc_str_sepstr(struct Cyc_List_List*,struct _fat_ptr);struct Cyc_Position_Error;struct Cyc_Iter_Iter{void*env;int(*next)(void*env,void*dest);};struct Cyc_Dict_T;struct Cyc_Dict_Dict{int(*rel)(void*,void*);struct _RegionHandle*r;const struct Cyc_Dict_T*t;};extern char Cyc_Dict_Present[8U];struct Cyc_Dict_Present_exn_struct{char*tag;};extern char Cyc_Dict_Absent[7U];struct Cyc_Dict_Absent_exn_struct{char*tag;};struct Cyc_Relations_Reln;struct _union_Nmspace_Rel_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Abs_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_C_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Loc_n{int tag;int val;};union Cyc_Absyn_Nmspace{struct _union_Nmspace_Rel_n Rel_n;struct _union_Nmspace_Abs_n Abs_n;struct _union_Nmspace_C_n C_n;struct _union_Nmspace_Loc_n Loc_n;};struct _tuple0{union Cyc_Absyn_Nmspace f1;struct _fat_ptr*f2;};
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
enum Cyc_Absyn_Coercion{Cyc_Absyn_Unknown_coercion =0U,Cyc_Absyn_No_coercion =1U,Cyc_Absyn_Null_to_NonNull =2U,Cyc_Absyn_Other_coercion =3U};struct Cyc_Absyn_MallocInfo{int is_calloc;struct Cyc_Absyn_Exp*rgn;void**elt_type;struct Cyc_Absyn_Exp*num_elts;int fat_result;int inline_call;};struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct{int tag;union Cyc_Absyn_Cnst f1;};struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Pragma_e_Absyn_Raw_exp_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct{int tag;enum Cyc_Absyn_Primop f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;enum Cyc_Absyn_Incrementor f2;};struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct _tuple8{struct Cyc_List_List*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_VarargCallInfo*f3;int f4;struct _tuple8*f5;};struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;int f2;};struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Exp*f2;int f3;enum Cyc_Absyn_Coercion f4;};struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;int f3;int f4;};struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;int f3;int f4;};struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Tuple_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;};struct _tuple9{struct _fat_ptr*f1;struct Cyc_Absyn_Tqual f2;void*f3;};struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct{int tag;struct _tuple9*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;int f4;};struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;int f3;};struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct Cyc_Absyn_Aggrdecl*f4;};struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Datatypedecl*f2;struct Cyc_Absyn_Datatypefield*f3;};struct Cyc_Absyn_Enum_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_Spawn_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;struct _tuple8*f3;};struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_MallocInfo f1;};struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Core_Opt*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Stmt*f1;};struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;};struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct{int tag;int f1;struct _fat_ptr f2;struct Cyc_List_List*f3;struct Cyc_List_List*f4;struct Cyc_List_List*f5;};struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Exp{void*topt;void*r;unsigned loc;void*annot;};struct Cyc_Absyn_Skip_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;struct Cyc_Absyn_Stmt*f3;};struct _tuple10{struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct{int tag;struct _tuple10 f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Break_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Continue_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Goto_s_Absyn_Raw_stmt_struct{int tag;struct _fat_ptr*f1;};struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _tuple10 f2;struct _tuple10 f3;struct Cyc_Absyn_Stmt*f4;};struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;void*f3;};struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Switch_clause**f2;};struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Decl*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct{int tag;struct _fat_ptr*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct _tuple10 f2;};struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_List_List*f2;void*f3;};struct Cyc_Absyn_Stmt{void*r;unsigned loc;void*annot;};struct Cyc_Absyn_Wild_p_Absyn_Raw_pat_struct{int tag;};struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_AliasVar_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_TagInt_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Tuple_p_Absyn_Raw_pat_struct{int tag;struct Cyc_List_List*f1;int f2;};struct Cyc_Absyn_Pointer_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Pat*f1;};struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct{int tag;union Cyc_Absyn_AggrInfo*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;int f4;};struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;struct Cyc_List_List*f3;int f4;};struct Cyc_Absyn_Null_p_Absyn_Raw_pat_struct{int tag;};struct Cyc_Absyn_Int_p_Absyn_Raw_pat_struct{int tag;enum Cyc_Absyn_Sign f1;int f2;};struct Cyc_Absyn_Char_p_Absyn_Raw_pat_struct{int tag;char f1;};struct Cyc_Absyn_Float_p_Absyn_Raw_pat_struct{int tag;struct _fat_ptr f1;int f2;};struct Cyc_Absyn_Enum_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_AnonEnum_p_Absyn_Raw_pat_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_UnknownId_p_Absyn_Raw_pat_struct{int tag;struct _tuple0*f1;};struct Cyc_Absyn_UnknownCall_p_Absyn_Raw_pat_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;int f3;};struct Cyc_Absyn_Exp_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Exp*f1;};
# 771 "absyn.h"
extern struct Cyc_Absyn_Wild_p_Absyn_Raw_pat_struct Cyc_Absyn_Wild_p_val;struct Cyc_Absyn_Pat{void*r;void*topt;unsigned loc;};struct Cyc_Absyn_Switch_clause{struct Cyc_Absyn_Pat*pattern;struct Cyc_Core_Opt*pat_vars;struct Cyc_Absyn_Exp*where_clause;struct Cyc_Absyn_Stmt*body;unsigned loc;};struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct{int tag;struct _tuple0*f1;};struct Cyc_Absyn_Global_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Funname_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Param_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Local_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Pat_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Vardecl{enum Cyc_Absyn_Scope sc;struct _tuple0*name;unsigned varloc;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*initializer;void*rgn;struct Cyc_List_List*attributes;int escapes;};struct Cyc_Absyn_Fndecl{enum Cyc_Absyn_Scope sc;int is_inline;struct _tuple0*name;struct Cyc_Absyn_Stmt*body;struct Cyc_Absyn_FnInfo i;void*cached_type;struct Cyc_Core_Opt*param_vardecls;struct Cyc_Absyn_Vardecl*fn_vardecl;};struct Cyc_Absyn_Aggrfield{struct _fat_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*width;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*requires_clause;};struct Cyc_Absyn_AggrdeclImpl{struct Cyc_List_List*exist_vars;struct Cyc_List_List*rgn_po;struct Cyc_List_List*fields;int tagged;};struct Cyc_Absyn_Aggrdecl{enum Cyc_Absyn_AggrKind kind;enum Cyc_Absyn_Scope sc;struct _tuple0*name;struct Cyc_List_List*tvs;struct Cyc_Absyn_AggrdeclImpl*impl;struct Cyc_List_List*attributes;int expected_mem_kind;};struct Cyc_Absyn_Datatypefield{struct _tuple0*name;struct Cyc_List_List*typs;unsigned loc;enum Cyc_Absyn_Scope sc;};struct Cyc_Absyn_Datatypedecl{enum Cyc_Absyn_Scope sc;struct _tuple0*name;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*fields;int is_extensible;};struct Cyc_Absyn_Enumfield{struct _tuple0*name;struct Cyc_Absyn_Exp*tag;unsigned loc;};struct Cyc_Absyn_Enumdecl{enum Cyc_Absyn_Scope sc;struct _tuple0*name;struct Cyc_Core_Opt*fields;};struct Cyc_Absyn_Typedefdecl{struct _tuple0*name;struct Cyc_Absyn_Tqual tq;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*kind;void*defn;struct Cyc_List_List*atts;int extern_c;};struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Pat*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;void*f4;};struct Cyc_Absyn_Letv_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;};struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;};struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Typedefdecl*f1;};struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct{int tag;struct _fat_ptr*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_ExternC_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;};struct _tuple11{unsigned f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct _tuple11*f4;};struct Cyc_Absyn_Porton_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Portoff_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Tempeston_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Tempestoff_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Decl{void*r;unsigned loc;};struct Cyc_Absyn_ArrayElement_Absyn_Designator_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_FieldName_Absyn_Designator_struct{int tag;struct _fat_ptr*f1;};extern char Cyc_Absyn_EmptyAnnot[11U];struct Cyc_Absyn_EmptyAnnot_Absyn_AbsynAnnot_struct{char*tag;};
# 954
int Cyc_Absyn_qvar_cmp(struct _tuple0*,struct _tuple0*);
# 963
struct Cyc_Absyn_Tqual Cyc_Absyn_empty_tqual(unsigned);
# 968
void*Cyc_Absyn_compress_kb(void*);
# 974
int Cyc_Absyn_type2bool(int def,void*);
# 979
void*Cyc_Absyn_new_evar(struct Cyc_Core_Opt*k,struct Cyc_Core_Opt*tenv);
# 1083
struct Cyc_Absyn_Exp*Cyc_Absyn_times_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
# 1106
struct Cyc_Absyn_Exp*Cyc_Absyn_sizeoftype_exp(void*t,unsigned);
# 1213
struct _fat_ptr Cyc_Absyn_attribute2string(void*);struct _tuple12{enum Cyc_Absyn_AggrKind f1;struct _tuple0*f2;};
# 1223
struct _tuple12 Cyc_Absyn_aggr_kinded_name(union Cyc_Absyn_AggrInfo);
# 1231
struct _tuple0*Cyc_Absyn_binding2qvar(void*);
# 1330
struct Cyc_Absyn_RgnEffect*Cyc_Absyn_type2rgneffect(void*t);
# 1347
struct _fat_ptr Cyc_Absyn_effect2string(struct Cyc_List_List*f);
struct _fat_ptr Cyc_Absyn_rgneffect2string(struct Cyc_Absyn_RgnEffect*r);
# 1374
int Cyc_Absyn_is_throwsany(struct Cyc_List_List*t);
# 1376
int Cyc_Absyn_is_nothrow(struct Cyc_List_List*t);
# 1399
int Cyc_Absyn_is_reentrant(int);struct _tuple13{unsigned f1;int f2;};
# 28 "evexp.h"
struct _tuple13 Cyc_Evexp_eval_const_uint_exp(struct Cyc_Absyn_Exp*e);struct _union_RelnOp_RConst{int tag;unsigned val;};struct _union_RelnOp_RVar{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RNumelts{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RType{int tag;void*val;};struct _union_RelnOp_RParam{int tag;unsigned val;};struct _union_RelnOp_RParamNumelts{int tag;unsigned val;};struct _union_RelnOp_RReturn{int tag;unsigned val;};union Cyc_Relations_RelnOp{struct _union_RelnOp_RConst RConst;struct _union_RelnOp_RVar RVar;struct _union_RelnOp_RNumelts RNumelts;struct _union_RelnOp_RType RType;struct _union_RelnOp_RParam RParam;struct _union_RelnOp_RParamNumelts RParamNumelts;struct _union_RelnOp_RReturn RReturn;};
# 50 "relations-ap.h"
enum Cyc_Relations_Relation{Cyc_Relations_Req =0U,Cyc_Relations_Rneq =1U,Cyc_Relations_Rlte =2U,Cyc_Relations_Rlt =3U};struct Cyc_Relations_Reln{union Cyc_Relations_RelnOp rop1;enum Cyc_Relations_Relation relation;union Cyc_Relations_RelnOp rop2;};struct Cyc_RgnOrder_RgnPO;extern char Cyc_Tcenv_Env_error[10U];struct Cyc_Tcenv_Env_error_exn_struct{char*tag;};struct Cyc_Tcenv_Genv{struct Cyc_Dict_Dict aggrdecls;struct Cyc_Dict_Dict datatypedecls;struct Cyc_Dict_Dict enumdecls;struct Cyc_Dict_Dict typedefs;struct Cyc_Dict_Dict ordinaries;};struct Cyc_Tcenv_Fenv;struct Cyc_Tcenv_Tenv{struct Cyc_List_List*ns;struct Cyc_Tcenv_Genv*ae;struct Cyc_Tcenv_Fenv*le;int allow_valueof: 1;int in_extern_c_include: 1;int in_tempest: 1;int tempest_generalize: 1;};
# 90 "tcenv.h"
enum Cyc_Tcenv_NewStatus{Cyc_Tcenv_NoneNew =0U,Cyc_Tcenv_InNew =1U,Cyc_Tcenv_InNewAggr =2U};
# 110 "tcutil.h"
void*Cyc_Tcutil_compress(void*);
# 268
int Cyc_Tcutil_is_temp_tvar(struct Cyc_Absyn_Tvar*);
# 40 "warn.h"
void*Cyc_Warn_impos(struct _fat_ptr fmt,struct _fat_ptr ap);struct Cyc_Warn_String_Warn_Warg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Warn_Exp_Warn_Warg_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Warn_Typ_Warn_Warg_struct{int tag;void*f1;};struct Cyc_Warn_Qvar_Warn_Warg_struct{int tag;struct _tuple0*f1;};
# 39 "pp.h"
extern int Cyc_PP_tex_output;struct Cyc_PP_Ppstate;struct Cyc_PP_Out;struct Cyc_PP_Doc;
# 50
void Cyc_PP_file_of_doc(struct Cyc_PP_Doc*d,int w,struct Cyc___cycFILE*f);
# 53
struct _fat_ptr Cyc_PP_string_of_doc(struct Cyc_PP_Doc*d,int w);
# 67 "pp.h"
struct Cyc_PP_Doc*Cyc_PP_nil_doc();
# 69
struct Cyc_PP_Doc*Cyc_PP_blank_doc();
# 72
struct Cyc_PP_Doc*Cyc_PP_line_doc();
# 78
struct Cyc_PP_Doc*Cyc_PP_text(struct _fat_ptr s);
# 80
struct Cyc_PP_Doc*Cyc_PP_textptr(struct _fat_ptr*p);
# 83
struct Cyc_PP_Doc*Cyc_PP_text_width(struct _fat_ptr s,int w);
# 91
struct Cyc_PP_Doc*Cyc_PP_nest(int k,struct Cyc_PP_Doc*d);
# 94
struct Cyc_PP_Doc*Cyc_PP_cat(struct _fat_ptr);
# 108
struct Cyc_PP_Doc*Cyc_PP_seq(struct _fat_ptr sep,struct Cyc_List_List*l);
# 112
struct Cyc_PP_Doc*Cyc_PP_ppseq(struct Cyc_PP_Doc*(*pp)(void*),struct _fat_ptr sep,struct Cyc_List_List*l);
# 117
struct Cyc_PP_Doc*Cyc_PP_seql(struct _fat_ptr sep,struct Cyc_List_List*l0);
# 120
struct Cyc_PP_Doc*Cyc_PP_ppseql(struct Cyc_PP_Doc*(*pp)(void*),struct _fat_ptr sep,struct Cyc_List_List*l);
# 123
struct Cyc_PP_Doc*Cyc_PP_group(struct _fat_ptr start,struct _fat_ptr stop,struct _fat_ptr sep,struct Cyc_List_List*l);
# 129
struct Cyc_PP_Doc*Cyc_PP_egroup(struct _fat_ptr start,struct _fat_ptr stop,struct _fat_ptr sep,struct Cyc_List_List*l);struct Cyc_Absynpp_Params{int expand_typedefs;int qvar_to_Cids;int add_cyc_prefix;int to_VC;int decls_first;int rewrite_temp_tvars;int print_all_tvars;int print_all_kinds;int print_all_effects;int print_using_stmts;int print_externC_stmts;int print_full_evars;int print_zeroterm;int generate_line_directives;int use_curr_namespace;struct Cyc_List_List*curr_namespace;};
# 51 "absynpp.h"
extern int Cyc_Absynpp_print_for_cycdoc;
# 59
struct Cyc_PP_Doc*Cyc_Absynpp_decl2doc(struct Cyc_Absyn_Decl*d);
# 61
struct _fat_ptr Cyc_Absynpp_longlong2string(unsigned long long);
struct _fat_ptr Cyc_Absynpp_typ2string(void*);
# 64
struct _fat_ptr Cyc_Absynpp_kind2string(struct Cyc_Absyn_Kind*);
# 69
struct _fat_ptr Cyc_Absynpp_qvar2string(struct _tuple0*);
# 77
int Cyc_Absynpp_is_anon_aggrtype(void*t);
extern struct _fat_ptr Cyc_Absynpp_cyc_string;
extern struct _fat_ptr*Cyc_Absynpp_cyc_stringptr;
int Cyc_Absynpp_exp_prec(struct Cyc_Absyn_Exp*);
struct _fat_ptr Cyc_Absynpp_char_escape(char);
struct _fat_ptr Cyc_Absynpp_string_escape(struct _fat_ptr);
struct _fat_ptr Cyc_Absynpp_prim2str(enum Cyc_Absyn_Primop p);
int Cyc_Absynpp_is_declaration(struct Cyc_Absyn_Stmt*s);struct _tuple14{struct Cyc_Absyn_Tqual f1;void*f2;struct Cyc_List_List*f3;};
struct _tuple14 Cyc_Absynpp_to_tms(struct _RegionHandle*,struct Cyc_Absyn_Tqual tq,void*t);struct _tuple15{int f1;struct Cyc_List_List*f2;};
# 94 "absynpp.h"
struct _tuple15 Cyc_Absynpp_shadows(struct Cyc_Absyn_Decl*d,struct Cyc_List_List*varsinblock);
# 26 "cyclone.h"
enum Cyc_Cyclone_C_Compilers{Cyc_Cyclone_Gcc_c =0U,Cyc_Cyclone_Vc_c =1U};
# 32
extern enum Cyc_Cyclone_C_Compilers Cyc_Cyclone_c_compiler;struct _tuple16{struct Cyc_List_List*f1;struct Cyc_Absyn_Pat*f2;};
# 39 "absynpp.cyc"
struct Cyc_PP_Doc*Cyc_Absynpp_dp2doc(struct _tuple16*dp);
struct Cyc_PP_Doc*Cyc_Absynpp_switchclauses2doc(struct Cyc_List_List*cs);
struct Cyc_PP_Doc*Cyc_Absynpp_typ2doc(void*);
struct Cyc_PP_Doc*Cyc_Absynpp_aggrfields2doc(struct Cyc_List_List*fields);
struct Cyc_PP_Doc*Cyc_Absynpp_scope2doc(enum Cyc_Absyn_Scope);
struct Cyc_PP_Doc*Cyc_Absynpp_stmt2doc(struct Cyc_Absyn_Stmt*,int expstmt,struct Cyc_List_List*varsinblock);
struct Cyc_PP_Doc*Cyc_Absynpp_exp2doc(struct Cyc_Absyn_Exp*);
struct Cyc_PP_Doc*Cyc_Absynpp_exp2doc_prec(int inprec,struct Cyc_Absyn_Exp*e);
struct Cyc_PP_Doc*Cyc_Absynpp_exps2doc_prec(int inprec,struct Cyc_List_List*es);
struct Cyc_PP_Doc*Cyc_Absynpp_qvar2doc(struct _tuple0*);
struct Cyc_PP_Doc*Cyc_Absynpp_typedef_name2doc(struct _tuple0*);
struct Cyc_PP_Doc*Cyc_Absynpp_cnst2doc(union Cyc_Absyn_Cnst);
struct Cyc_PP_Doc*Cyc_Absynpp_prim2doc(enum Cyc_Absyn_Primop);
struct Cyc_PP_Doc*Cyc_Absynpp_primapp2doc(int inprec,enum Cyc_Absyn_Primop p,struct Cyc_List_List*es);struct _tuple17{struct Cyc_List_List*f1;struct Cyc_Absyn_Exp*f2;};
struct Cyc_PP_Doc*Cyc_Absynpp_de2doc(struct _tuple17*de);
struct Cyc_PP_Doc*Cyc_Absynpp_tqtd2doc(struct Cyc_Absyn_Tqual tq,void*t,struct Cyc_Core_Opt*dopt);
struct Cyc_PP_Doc*Cyc_Absynpp_funargs2doc(struct Cyc_List_List*args,int c_varargs,struct Cyc_Absyn_VarargInfo*cyc_varargs,void*effopt,struct Cyc_List_List*rgn_po,struct Cyc_Absyn_Exp*requires,struct Cyc_Absyn_Exp*ensures,struct Cyc_List_List*ieff,struct Cyc_List_List*oeff,struct Cyc_List_List*throws,int reentrant);
# 65
struct Cyc_PP_Doc*Cyc_Absynpp_datatypefields2doc(struct Cyc_List_List*fields);
struct Cyc_PP_Doc*Cyc_Absynpp_enumfields2doc(struct Cyc_List_List*fs);
struct Cyc_PP_Doc*Cyc_Absynpp_vardecl2doc(struct Cyc_Absyn_Vardecl*vd);
struct Cyc_PP_Doc*Cyc_Absynpp_aggrdecl2doc(struct Cyc_Absyn_Aggrdecl*ad);
struct Cyc_PP_Doc*Cyc_Absynpp_enumdecl2doc(struct Cyc_Absyn_Enumdecl*ad);
struct Cyc_PP_Doc*Cyc_Absynpp_datatypedecl2doc(struct Cyc_Absyn_Datatypedecl*ad);
# 72
static int Cyc_Absynpp_expand_typedefs;
# 76
static int Cyc_Absynpp_qvar_to_Cids;static char _tmp0[4U]="Cyc";
# 78
struct _fat_ptr Cyc_Absynpp_cyc_string={_tmp0,_tmp0,_tmp0 + 4U};
struct _fat_ptr*Cyc_Absynpp_cyc_stringptr=& Cyc_Absynpp_cyc_string;
# 81
static int Cyc_Absynpp_add_cyc_prefix;
# 85
static int Cyc_Absynpp_to_VC;
# 88
static int Cyc_Absynpp_decls_first;
# 92
static int Cyc_Absynpp_rewrite_temp_tvars;
# 95
static int Cyc_Absynpp_print_all_tvars;
# 98
static int Cyc_Absynpp_print_all_kinds;
# 101
static int Cyc_Absynpp_print_all_effects;
# 104
static int Cyc_Absynpp_print_using_stmts;
# 109
static int Cyc_Absynpp_print_externC_stmts;
# 113
static int Cyc_Absynpp_print_full_evars;
# 116
static int Cyc_Absynpp_generate_line_directives;
# 119
static int Cyc_Absynpp_use_curr_namespace;
# 122
static int Cyc_Absynpp_print_zeroterm;
# 125
static struct Cyc_List_List*Cyc_Absynpp_curr_namespace=0;
# 128
int Cyc_Absynpp_print_for_cycdoc=0;
# 149
void Cyc_Absynpp_set_params(struct Cyc_Absynpp_Params*fs){
Cyc_Absynpp_expand_typedefs=fs->expand_typedefs;
Cyc_Absynpp_qvar_to_Cids=fs->qvar_to_Cids;
Cyc_Absynpp_add_cyc_prefix=fs->add_cyc_prefix;
Cyc_Absynpp_to_VC=fs->to_VC;
Cyc_Absynpp_decls_first=fs->decls_first;
Cyc_Absynpp_rewrite_temp_tvars=fs->rewrite_temp_tvars;
Cyc_Absynpp_print_all_tvars=fs->print_all_tvars;
Cyc_Absynpp_print_all_kinds=fs->print_all_kinds;
Cyc_Absynpp_print_all_effects=fs->print_all_effects;
Cyc_Absynpp_print_using_stmts=fs->print_using_stmts;
Cyc_Absynpp_print_externC_stmts=fs->print_externC_stmts;
Cyc_Absynpp_print_full_evars=fs->print_full_evars;
Cyc_Absynpp_print_zeroterm=fs->print_zeroterm;
Cyc_Absynpp_generate_line_directives=fs->generate_line_directives;
Cyc_Absynpp_use_curr_namespace=fs->use_curr_namespace;
Cyc_Absynpp_curr_namespace=fs->curr_namespace;}
# 149
struct Cyc_Absynpp_Params Cyc_Absynpp_cyc_params_r={0,0,0,0,0,1,0,0,0,1,1,0,1,0,1,0};
# 188
struct Cyc_Absynpp_Params Cyc_Absynpp_cyci_params_r={1,0,1,0,0,1,0,0,0,1,1,0,1,0,1,0};
# 208
struct Cyc_Absynpp_Params Cyc_Absynpp_c_params_r={1,1,1,0,1,0,0,0,0,0,0,0,0,1,0,0};
# 228
struct Cyc_Absynpp_Params Cyc_Absynpp_tc_params_r={0,0,0,0,0,1,0,0,0,1,1,0,1,0,0,0};
# 249
static void Cyc_Absynpp_curr_namespace_add(struct _fat_ptr*v){
Cyc_Absynpp_curr_namespace=({({struct Cyc_List_List*_tmp752=Cyc_Absynpp_curr_namespace;Cyc_List_imp_append(_tmp752,({struct Cyc_List_List*_tmp2=_cycalloc(sizeof(*_tmp2));_tmp2->hd=v,_tmp2->tl=0;_tmp2;}));});});}
# 253
static void Cyc_Absynpp_suppr_last(struct Cyc_List_List**l){
if(((struct Cyc_List_List*)_check_null(*l))->tl == 0)
*l=0;else{
# 257
({Cyc_Absynpp_suppr_last(&((struct Cyc_List_List*)_check_null(*l))->tl);});}}
# 261
static void Cyc_Absynpp_curr_namespace_drop(){
({Cyc_Absynpp_suppr_last(& Cyc_Absynpp_curr_namespace);});}
# 261
struct _fat_ptr Cyc_Absynpp_char_escape(char c){
# 266
char _tmp6=c;switch(_tmp6){case 7U: _LL1: _LL2:
 return({const char*_tmp7="\\a";_tag_fat(_tmp7,sizeof(char),3U);});case 8U: _LL3: _LL4:
 return({const char*_tmp8="\\b";_tag_fat(_tmp8,sizeof(char),3U);});case 12U: _LL5: _LL6:
 return({const char*_tmp9="\\f";_tag_fat(_tmp9,sizeof(char),3U);});case 10U: _LL7: _LL8:
 return({const char*_tmpA="\\n";_tag_fat(_tmpA,sizeof(char),3U);});case 13U: _LL9: _LLA:
 return({const char*_tmpB="\\r";_tag_fat(_tmpB,sizeof(char),3U);});case 9U: _LLB: _LLC:
 return({const char*_tmpC="\\t";_tag_fat(_tmpC,sizeof(char),3U);});case 11U: _LLD: _LLE:
 return({const char*_tmpD="\\v";_tag_fat(_tmpD,sizeof(char),3U);});case 92U: _LLF: _LL10:
 return({const char*_tmpE="\\\\";_tag_fat(_tmpE,sizeof(char),3U);});case 34U: _LL11: _LL12:
 return({const char*_tmpF="\"";_tag_fat(_tmpF,sizeof(char),2U);});case 39U: _LL13: _LL14:
 return({const char*_tmp10="\\'";_tag_fat(_tmp10,sizeof(char),3U);});default: _LL15: _LL16:
# 278
 if((int)c >= (int)' ' &&(int)c <= (int)'~'){
struct _fat_ptr t=({Cyc_Core_new_string(2U);});
({struct _fat_ptr _tmp11=_fat_ptr_plus(t,sizeof(char),0);char _tmp12=*((char*)_check_fat_subscript(_tmp11,sizeof(char),0U));char _tmp13=c;if(_get_fat_size(_tmp11,sizeof(char))== 1U &&(_tmp12 == 0 && _tmp13 != 0))_throw_arraybounds();*((char*)_tmp11.curr)=_tmp13;});
return(struct _fat_ptr)t;}else{
# 283
struct _fat_ptr t=({Cyc_Core_new_string(5U);});
int j=0;
({struct _fat_ptr _tmp14=_fat_ptr_plus(t,sizeof(char),j ++);char _tmp15=*((char*)_check_fat_subscript(_tmp14,sizeof(char),0U));char _tmp16='\\';if(_get_fat_size(_tmp14,sizeof(char))== 1U &&(_tmp15 == 0 && _tmp16 != 0))_throw_arraybounds();*((char*)_tmp14.curr)=_tmp16;});
({struct _fat_ptr _tmp17=_fat_ptr_plus(t,sizeof(char),j ++);char _tmp18=*((char*)_check_fat_subscript(_tmp17,sizeof(char),0U));char _tmp19=(char)((int)'0' + ((int)((unsigned char)c)>> 6 & 3));if(_get_fat_size(_tmp17,sizeof(char))== 1U &&(_tmp18 == 0 && _tmp19 != 0))_throw_arraybounds();*((char*)_tmp17.curr)=_tmp19;});
({struct _fat_ptr _tmp1A=_fat_ptr_plus(t,sizeof(char),j ++);char _tmp1B=*((char*)_check_fat_subscript(_tmp1A,sizeof(char),0U));char _tmp1C=(char)((int)'0' + ((int)c >> 3 & 7));if(_get_fat_size(_tmp1A,sizeof(char))== 1U &&(_tmp1B == 0 && _tmp1C != 0))_throw_arraybounds();*((char*)_tmp1A.curr)=_tmp1C;});
({struct _fat_ptr _tmp1D=_fat_ptr_plus(t,sizeof(char),j ++);char _tmp1E=*((char*)_check_fat_subscript(_tmp1D,sizeof(char),0U));char _tmp1F=(char)((int)'0' + ((int)c & 7));if(_get_fat_size(_tmp1D,sizeof(char))== 1U &&(_tmp1E == 0 && _tmp1F != 0))_throw_arraybounds();*((char*)_tmp1D.curr)=_tmp1F;});
return(struct _fat_ptr)t;}}_LL0:;}
# 294
static int Cyc_Absynpp_special(struct _fat_ptr s){
int sz=(int)(_get_fat_size(s,sizeof(char))- (unsigned)1);
{int i=0;for(0;i < sz;++ i){
char c=*((const char*)_check_fat_subscript(s,sizeof(char),i));
if((((int)c <= (int)' ' ||(int)c >= (int)'~')||(int)c == (int)'"')||(int)c == (int)'\\')
return 1;}}
# 301
return 0;}
# 294
struct _fat_ptr Cyc_Absynpp_string_escape(struct _fat_ptr s){
# 305
if(!({Cyc_Absynpp_special(s);}))return s;{int n=(int)(
# 307
_get_fat_size(s,sizeof(char))- (unsigned)1);
# 309
if(n > 0 &&(int)*((const char*)_check_fat_subscript(s,sizeof(char),n))== (int)'\000')-- n;{int len=0;
# 312
{int i=0;for(0;i <= n;++ i){
char _stmttmp0=*((const char*)_check_fat_subscript(s,sizeof(char),i));char _tmp22=_stmttmp0;char _tmp23;switch(_tmp22){case 7U: _LL1: _LL2:
 goto _LL4;case 8U: _LL3: _LL4:
 goto _LL6;case 12U: _LL5: _LL6:
 goto _LL8;case 10U: _LL7: _LL8:
 goto _LLA;case 13U: _LL9: _LLA:
 goto _LLC;case 9U: _LLB: _LLC:
 goto _LLE;case 11U: _LLD: _LLE:
 goto _LL10;case 92U: _LLF: _LL10:
 goto _LL12;case 34U: _LL11: _LL12:
 len +=2;goto _LL0;default: _LL13: _tmp23=_tmp22;_LL14: {char c=_tmp23;
# 324
if((int)c >= (int)' ' &&(int)c <= (int)'~')++ len;else{
len +=4;}
goto _LL0;}}_LL0:;}}{
# 329
struct _fat_ptr t=({Cyc_Core_new_string((unsigned)(len + 1));});
int j=0;
{int i=0;for(0;i <= n;++ i){
char _stmttmp1=*((const char*)_check_fat_subscript(s,sizeof(char),i));char _tmp24=_stmttmp1;char _tmp25;switch(_tmp24){case 7U: _LL16: _LL17:
({struct _fat_ptr _tmp26=_fat_ptr_plus(t,sizeof(char),j ++);char _tmp27=*((char*)_check_fat_subscript(_tmp26,sizeof(char),0U));char _tmp28='\\';if(_get_fat_size(_tmp26,sizeof(char))== 1U &&(_tmp27 == 0 && _tmp28 != 0))_throw_arraybounds();*((char*)_tmp26.curr)=_tmp28;});({struct _fat_ptr _tmp29=_fat_ptr_plus(t,sizeof(char),j ++);char _tmp2A=*((char*)_check_fat_subscript(_tmp29,sizeof(char),0U));char _tmp2B='a';if(_get_fat_size(_tmp29,sizeof(char))== 1U &&(_tmp2A == 0 && _tmp2B != 0))_throw_arraybounds();*((char*)_tmp29.curr)=_tmp2B;});goto _LL15;case 8U: _LL18: _LL19:
({struct _fat_ptr _tmp2C=_fat_ptr_plus(t,sizeof(char),j ++);char _tmp2D=*((char*)_check_fat_subscript(_tmp2C,sizeof(char),0U));char _tmp2E='\\';if(_get_fat_size(_tmp2C,sizeof(char))== 1U &&(_tmp2D == 0 && _tmp2E != 0))_throw_arraybounds();*((char*)_tmp2C.curr)=_tmp2E;});({struct _fat_ptr _tmp2F=_fat_ptr_plus(t,sizeof(char),j ++);char _tmp30=*((char*)_check_fat_subscript(_tmp2F,sizeof(char),0U));char _tmp31='b';if(_get_fat_size(_tmp2F,sizeof(char))== 1U &&(_tmp30 == 0 && _tmp31 != 0))_throw_arraybounds();*((char*)_tmp2F.curr)=_tmp31;});goto _LL15;case 12U: _LL1A: _LL1B:
({struct _fat_ptr _tmp32=_fat_ptr_plus(t,sizeof(char),j ++);char _tmp33=*((char*)_check_fat_subscript(_tmp32,sizeof(char),0U));char _tmp34='\\';if(_get_fat_size(_tmp32,sizeof(char))== 1U &&(_tmp33 == 0 && _tmp34 != 0))_throw_arraybounds();*((char*)_tmp32.curr)=_tmp34;});({struct _fat_ptr _tmp35=_fat_ptr_plus(t,sizeof(char),j ++);char _tmp36=*((char*)_check_fat_subscript(_tmp35,sizeof(char),0U));char _tmp37='f';if(_get_fat_size(_tmp35,sizeof(char))== 1U &&(_tmp36 == 0 && _tmp37 != 0))_throw_arraybounds();*((char*)_tmp35.curr)=_tmp37;});goto _LL15;case 10U: _LL1C: _LL1D:
({struct _fat_ptr _tmp38=_fat_ptr_plus(t,sizeof(char),j ++);char _tmp39=*((char*)_check_fat_subscript(_tmp38,sizeof(char),0U));char _tmp3A='\\';if(_get_fat_size(_tmp38,sizeof(char))== 1U &&(_tmp39 == 0 && _tmp3A != 0))_throw_arraybounds();*((char*)_tmp38.curr)=_tmp3A;});({struct _fat_ptr _tmp3B=_fat_ptr_plus(t,sizeof(char),j ++);char _tmp3C=*((char*)_check_fat_subscript(_tmp3B,sizeof(char),0U));char _tmp3D='n';if(_get_fat_size(_tmp3B,sizeof(char))== 1U &&(_tmp3C == 0 && _tmp3D != 0))_throw_arraybounds();*((char*)_tmp3B.curr)=_tmp3D;});goto _LL15;case 13U: _LL1E: _LL1F:
({struct _fat_ptr _tmp3E=_fat_ptr_plus(t,sizeof(char),j ++);char _tmp3F=*((char*)_check_fat_subscript(_tmp3E,sizeof(char),0U));char _tmp40='\\';if(_get_fat_size(_tmp3E,sizeof(char))== 1U &&(_tmp3F == 0 && _tmp40 != 0))_throw_arraybounds();*((char*)_tmp3E.curr)=_tmp40;});({struct _fat_ptr _tmp41=_fat_ptr_plus(t,sizeof(char),j ++);char _tmp42=*((char*)_check_fat_subscript(_tmp41,sizeof(char),0U));char _tmp43='r';if(_get_fat_size(_tmp41,sizeof(char))== 1U &&(_tmp42 == 0 && _tmp43 != 0))_throw_arraybounds();*((char*)_tmp41.curr)=_tmp43;});goto _LL15;case 9U: _LL20: _LL21:
({struct _fat_ptr _tmp44=_fat_ptr_plus(t,sizeof(char),j ++);char _tmp45=*((char*)_check_fat_subscript(_tmp44,sizeof(char),0U));char _tmp46='\\';if(_get_fat_size(_tmp44,sizeof(char))== 1U &&(_tmp45 == 0 && _tmp46 != 0))_throw_arraybounds();*((char*)_tmp44.curr)=_tmp46;});({struct _fat_ptr _tmp47=_fat_ptr_plus(t,sizeof(char),j ++);char _tmp48=*((char*)_check_fat_subscript(_tmp47,sizeof(char),0U));char _tmp49='t';if(_get_fat_size(_tmp47,sizeof(char))== 1U &&(_tmp48 == 0 && _tmp49 != 0))_throw_arraybounds();*((char*)_tmp47.curr)=_tmp49;});goto _LL15;case 11U: _LL22: _LL23:
({struct _fat_ptr _tmp4A=_fat_ptr_plus(t,sizeof(char),j ++);char _tmp4B=*((char*)_check_fat_subscript(_tmp4A,sizeof(char),0U));char _tmp4C='\\';if(_get_fat_size(_tmp4A,sizeof(char))== 1U &&(_tmp4B == 0 && _tmp4C != 0))_throw_arraybounds();*((char*)_tmp4A.curr)=_tmp4C;});({struct _fat_ptr _tmp4D=_fat_ptr_plus(t,sizeof(char),j ++);char _tmp4E=*((char*)_check_fat_subscript(_tmp4D,sizeof(char),0U));char _tmp4F='v';if(_get_fat_size(_tmp4D,sizeof(char))== 1U &&(_tmp4E == 0 && _tmp4F != 0))_throw_arraybounds();*((char*)_tmp4D.curr)=_tmp4F;});goto _LL15;case 92U: _LL24: _LL25:
({struct _fat_ptr _tmp50=_fat_ptr_plus(t,sizeof(char),j ++);char _tmp51=*((char*)_check_fat_subscript(_tmp50,sizeof(char),0U));char _tmp52='\\';if(_get_fat_size(_tmp50,sizeof(char))== 1U &&(_tmp51 == 0 && _tmp52 != 0))_throw_arraybounds();*((char*)_tmp50.curr)=_tmp52;});({struct _fat_ptr _tmp53=_fat_ptr_plus(t,sizeof(char),j ++);char _tmp54=*((char*)_check_fat_subscript(_tmp53,sizeof(char),0U));char _tmp55='\\';if(_get_fat_size(_tmp53,sizeof(char))== 1U &&(_tmp54 == 0 && _tmp55 != 0))_throw_arraybounds();*((char*)_tmp53.curr)=_tmp55;});goto _LL15;case 34U: _LL26: _LL27:
({struct _fat_ptr _tmp56=_fat_ptr_plus(t,sizeof(char),j ++);char _tmp57=*((char*)_check_fat_subscript(_tmp56,sizeof(char),0U));char _tmp58='\\';if(_get_fat_size(_tmp56,sizeof(char))== 1U &&(_tmp57 == 0 && _tmp58 != 0))_throw_arraybounds();*((char*)_tmp56.curr)=_tmp58;});({struct _fat_ptr _tmp59=_fat_ptr_plus(t,sizeof(char),j ++);char _tmp5A=*((char*)_check_fat_subscript(_tmp59,sizeof(char),0U));char _tmp5B='"';if(_get_fat_size(_tmp59,sizeof(char))== 1U &&(_tmp5A == 0 && _tmp5B != 0))_throw_arraybounds();*((char*)_tmp59.curr)=_tmp5B;});goto _LL15;default: _LL28: _tmp25=_tmp24;_LL29: {char c=_tmp25;
# 343
if((int)c >= (int)' ' &&(int)c <= (int)'~')({struct _fat_ptr _tmp5C=_fat_ptr_plus(t,sizeof(char),j ++);char _tmp5D=*((char*)_check_fat_subscript(_tmp5C,sizeof(char),0U));char _tmp5E=c;if(_get_fat_size(_tmp5C,sizeof(char))== 1U &&(_tmp5D == 0 && _tmp5E != 0))_throw_arraybounds();*((char*)_tmp5C.curr)=_tmp5E;});else{
# 345
({struct _fat_ptr _tmp5F=_fat_ptr_plus(t,sizeof(char),j ++);char _tmp60=*((char*)_check_fat_subscript(_tmp5F,sizeof(char),0U));char _tmp61='\\';if(_get_fat_size(_tmp5F,sizeof(char))== 1U &&(_tmp60 == 0 && _tmp61 != 0))_throw_arraybounds();*((char*)_tmp5F.curr)=_tmp61;});
({struct _fat_ptr _tmp62=_fat_ptr_plus(t,sizeof(char),j ++);char _tmp63=*((char*)_check_fat_subscript(_tmp62,sizeof(char),0U));char _tmp64=(char)((int)'0' + ((int)c >> 6 & 7));if(_get_fat_size(_tmp62,sizeof(char))== 1U &&(_tmp63 == 0 && _tmp64 != 0))_throw_arraybounds();*((char*)_tmp62.curr)=_tmp64;});
({struct _fat_ptr _tmp65=_fat_ptr_plus(t,sizeof(char),j ++);char _tmp66=*((char*)_check_fat_subscript(_tmp65,sizeof(char),0U));char _tmp67=(char)((int)'0' + ((int)c >> 3 & 7));if(_get_fat_size(_tmp65,sizeof(char))== 1U &&(_tmp66 == 0 && _tmp67 != 0))_throw_arraybounds();*((char*)_tmp65.curr)=_tmp67;});
({struct _fat_ptr _tmp68=_fat_ptr_plus(t,sizeof(char),j ++);char _tmp69=*((char*)_check_fat_subscript(_tmp68,sizeof(char),0U));char _tmp6A=(char)((int)'0' + ((int)c & 7));if(_get_fat_size(_tmp68,sizeof(char))== 1U &&(_tmp69 == 0 && _tmp6A != 0))_throw_arraybounds();*((char*)_tmp68.curr)=_tmp6A;});}
# 350
goto _LL15;}}_LL15:;}}
# 352
return(struct _fat_ptr)t;}}}}static char _tmp6C[9U]="restrict";
# 294
static struct _fat_ptr Cyc_Absynpp_restrict_string={_tmp6C,_tmp6C,_tmp6C + 9U};static char _tmp6D[9U]="volatile";
# 356
static struct _fat_ptr Cyc_Absynpp_volatile_string={_tmp6D,_tmp6D,_tmp6D + 9U};static char _tmp6E[6U]="const";
static struct _fat_ptr Cyc_Absynpp_const_str={_tmp6E,_tmp6E,_tmp6E + 6U};
static struct _fat_ptr*Cyc_Absynpp_restrict_sp=& Cyc_Absynpp_restrict_string;
static struct _fat_ptr*Cyc_Absynpp_volatile_sp=& Cyc_Absynpp_volatile_string;
static struct _fat_ptr*Cyc_Absynpp_const_sp=& Cyc_Absynpp_const_str;
# 362
struct Cyc_PP_Doc*Cyc_Absynpp_tqual2doc(struct Cyc_Absyn_Tqual tq){
struct Cyc_List_List*l=0;
# 365
if(tq.q_restrict)l=({struct Cyc_List_List*_tmp6F=_cycalloc(sizeof(*_tmp6F));_tmp6F->hd=Cyc_Absynpp_restrict_sp,_tmp6F->tl=l;_tmp6F;});if(tq.q_volatile)
l=({struct Cyc_List_List*_tmp70=_cycalloc(sizeof(*_tmp70));_tmp70->hd=Cyc_Absynpp_volatile_sp,_tmp70->tl=l;_tmp70;});
# 365
if(tq.print_const)
# 367
l=({struct Cyc_List_List*_tmp71=_cycalloc(sizeof(*_tmp71));_tmp71->hd=Cyc_Absynpp_const_sp,_tmp71->tl=l;_tmp71;});
# 365
return({struct Cyc_PP_Doc*(*_tmp72)(struct _fat_ptr start,struct _fat_ptr stop,struct _fat_ptr sep,struct Cyc_List_List*l)=Cyc_PP_egroup;struct _fat_ptr _tmp73=({const char*_tmp74="";_tag_fat(_tmp74,sizeof(char),1U);});struct _fat_ptr _tmp75=({const char*_tmp76=" ";_tag_fat(_tmp76,sizeof(char),2U);});struct _fat_ptr _tmp77=({const char*_tmp78=" ";_tag_fat(_tmp78,sizeof(char),2U);});struct Cyc_List_List*_tmp79=({({struct Cyc_List_List*(*_tmp753)(struct Cyc_PP_Doc*(*f)(struct _fat_ptr*),struct Cyc_List_List*x)=({
# 368
struct Cyc_List_List*(*_tmp7A)(struct Cyc_PP_Doc*(*f)(struct _fat_ptr*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_PP_Doc*(*f)(struct _fat_ptr*),struct Cyc_List_List*x))Cyc_List_map;_tmp7A;});_tmp753(Cyc_PP_textptr,l);});});_tmp72(_tmp73,_tmp75,_tmp77,_tmp79);});}
# 362
struct _fat_ptr Cyc_Absynpp_kind2string(struct Cyc_Absyn_Kind*ka){
# 372
enum Cyc_Absyn_AliasQual _tmp7D;enum Cyc_Absyn_KindQual _tmp7C;_LL1: _tmp7C=ka->kind;_tmp7D=ka->aliasqual;_LL2: {enum Cyc_Absyn_KindQual k=_tmp7C;enum Cyc_Absyn_AliasQual a=_tmp7D;
enum Cyc_Absyn_KindQual _tmp7E=k;switch(_tmp7E){case Cyc_Absyn_AnyKind: _LL4: _LL5: {
# 375
enum Cyc_Absyn_AliasQual _tmp7F=a;switch(_tmp7F){case Cyc_Absyn_Aliasable: _LL19: _LL1A:
 return({const char*_tmp80="A";_tag_fat(_tmp80,sizeof(char),2U);});case Cyc_Absyn_Unique: _LL1B: _LL1C:
 return({const char*_tmp81="UA";_tag_fat(_tmp81,sizeof(char),3U);});case Cyc_Absyn_Top: _LL1D: _LL1E:
 goto _LL20;default: _LL1F: _LL20: return({const char*_tmp82="TA";_tag_fat(_tmp82,sizeof(char),3U);});}_LL18:;}case Cyc_Absyn_MemKind: _LL6: _LL7: {
# 381
enum Cyc_Absyn_AliasQual _tmp83=a;switch(_tmp83){case Cyc_Absyn_Aliasable: _LL22: _LL23:
 return({const char*_tmp84="M";_tag_fat(_tmp84,sizeof(char),2U);});case Cyc_Absyn_Unique: _LL24: _LL25:
 return({const char*_tmp85="UM";_tag_fat(_tmp85,sizeof(char),3U);});case Cyc_Absyn_Top: _LL26: _LL27:
 goto _LL29;default: _LL28: _LL29: return({const char*_tmp86="TM";_tag_fat(_tmp86,sizeof(char),3U);});}_LL21:;}case Cyc_Absyn_BoxKind: _LL8: _LL9: {
# 387
enum Cyc_Absyn_AliasQual _tmp87=a;switch(_tmp87){case Cyc_Absyn_Aliasable: _LL2B: _LL2C:
 return({const char*_tmp88="B";_tag_fat(_tmp88,sizeof(char),2U);});case Cyc_Absyn_Unique: _LL2D: _LL2E:
 return({const char*_tmp89="UB";_tag_fat(_tmp89,sizeof(char),3U);});case Cyc_Absyn_Top: _LL2F: _LL30:
 goto _LL32;default: _LL31: _LL32: return({const char*_tmp8A="TB";_tag_fat(_tmp8A,sizeof(char),3U);});}_LL2A:;}case Cyc_Absyn_XRgnKind: _LLA: _LLB: {
# 393
enum Cyc_Absyn_AliasQual _tmp8B=a;switch(_tmp8B){case Cyc_Absyn_Aliasable: _LL34: _LL35:
 return({const char*_tmp8C="AX";_tag_fat(_tmp8C,sizeof(char),3U);});case Cyc_Absyn_Unique: _LL36: _LL37:
 return({const char*_tmp8D="UX";_tag_fat(_tmp8D,sizeof(char),3U);});case Cyc_Absyn_Top: _LL38: _LL39:
 goto _LL3B;default: _LL3A: _LL3B: return({const char*_tmp8E="TX";_tag_fat(_tmp8E,sizeof(char),3U);});}_LL33:;}case Cyc_Absyn_RgnKind: _LLC: _LLD: {
# 399
enum Cyc_Absyn_AliasQual _tmp8F=a;switch(_tmp8F){case Cyc_Absyn_Aliasable: _LL3D: _LL3E:
 return({const char*_tmp90="R";_tag_fat(_tmp90,sizeof(char),2U);});case Cyc_Absyn_Unique: _LL3F: _LL40:
 return({const char*_tmp91="UR";_tag_fat(_tmp91,sizeof(char),3U);});case Cyc_Absyn_Top: _LL41: _LL42:
 goto _LL44;default: _LL43: _LL44: return({const char*_tmp92="TR";_tag_fat(_tmp92,sizeof(char),3U);});}_LL3C:;}case Cyc_Absyn_EffKind: _LLE: _LLF:
# 404
 return({const char*_tmp93="E";_tag_fat(_tmp93,sizeof(char),2U);});case Cyc_Absyn_IntKind: _LL10: _LL11:
 return({const char*_tmp94="I";_tag_fat(_tmp94,sizeof(char),2U);});case Cyc_Absyn_BoolKind: _LL12: _LL13:
 return({const char*_tmp95="BOOL";_tag_fat(_tmp95,sizeof(char),5U);});case Cyc_Absyn_PtrBndKind: _LL14: _LL15:
 goto _LL17;default: _LL16: _LL17: return({const char*_tmp96="PTRBND";_tag_fat(_tmp96,sizeof(char),7U);});}_LL3:;}}
# 362
struct Cyc_PP_Doc*Cyc_Absynpp_kind2doc(struct Cyc_Absyn_Kind*k){
# 410
return({struct Cyc_PP_Doc*(*_tmp98)(struct _fat_ptr s)=Cyc_PP_text;struct _fat_ptr _tmp99=({Cyc_Absynpp_kind2string(k);});_tmp98(_tmp99);});}
# 362
struct _fat_ptr Cyc_Absynpp_kindbound2string(void*c){
# 413
void*_stmttmp2=({Cyc_Absyn_compress_kb(c);});void*_tmp9B=_stmttmp2;struct Cyc_Absyn_Kind*_tmp9C;struct Cyc_Absyn_Kind*_tmp9D;switch(*((int*)_tmp9B)){case 0U: _LL1: _tmp9D=((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp9B)->f1;_LL2: {struct Cyc_Absyn_Kind*k=_tmp9D;
return({Cyc_Absynpp_kind2string(k);});}case 1U: _LL3: _LL4:
# 416
 if(Cyc_PP_tex_output)
return({const char*_tmp9E="{?}";_tag_fat(_tmp9E,sizeof(char),4U);});else{
return({const char*_tmp9F="?";_tag_fat(_tmp9F,sizeof(char),2U);});}default: _LL5: _tmp9C=((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp9B)->f2;_LL6: {struct Cyc_Absyn_Kind*k=_tmp9C;
return(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmpA2=({struct Cyc_String_pa_PrintArg_struct _tmp733;_tmp733.tag=0U,({struct _fat_ptr _tmp754=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_kind2string(k);}));_tmp733.f1=_tmp754;});_tmp733;});void*_tmpA0[1U];_tmpA0[0]=& _tmpA2;({struct _fat_ptr _tmp755=({const char*_tmpA1="<=%s";_tag_fat(_tmpA1,sizeof(char),5U);});Cyc_aprintf(_tmp755,_tag_fat(_tmpA0,sizeof(void*),1U));});});}}_LL0:;}
# 362
struct Cyc_PP_Doc*Cyc_Absynpp_kindbound2doc(void*c){
# 424
void*_stmttmp3=({Cyc_Absyn_compress_kb(c);});void*_tmpA4=_stmttmp3;struct Cyc_Absyn_Kind*_tmpA5;struct Cyc_Absyn_Kind*_tmpA6;switch(*((int*)_tmpA4)){case 0U: _LL1: _tmpA6=((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmpA4)->f1;_LL2: {struct Cyc_Absyn_Kind*k=_tmpA6;
return({struct Cyc_PP_Doc*(*_tmpA7)(struct _fat_ptr s)=Cyc_PP_text;struct _fat_ptr _tmpA8=({Cyc_Absynpp_kind2string(k);});_tmpA7(_tmpA8);});}case 1U: _LL3: _LL4:
# 427
 if(Cyc_PP_tex_output)
return({Cyc_PP_text_width(({const char*_tmpA9="{?}";_tag_fat(_tmpA9,sizeof(char),4U);}),1);});else{
return({Cyc_PP_text(({const char*_tmpAA="?";_tag_fat(_tmpAA,sizeof(char),2U);}));});}default: _LL5: _tmpA5=((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmpA4)->f2;_LL6: {struct Cyc_Absyn_Kind*k=_tmpA5;
return({struct Cyc_PP_Doc*(*_tmpAB)(struct _fat_ptr s)=Cyc_PP_text;struct _fat_ptr _tmpAC=({Cyc_Absynpp_kind2string(k);});_tmpAB(_tmpAC);});}}_LL0:;}
# 362
struct Cyc_PP_Doc*Cyc_Absynpp_tps2doc(struct Cyc_List_List*ts){
# 435
return({struct Cyc_PP_Doc*(*_tmpAE)(struct _fat_ptr start,struct _fat_ptr stop,struct _fat_ptr sep,struct Cyc_List_List*l)=Cyc_PP_egroup;struct _fat_ptr _tmpAF=({const char*_tmpB0="<";_tag_fat(_tmpB0,sizeof(char),2U);});struct _fat_ptr _tmpB1=({const char*_tmpB2=">";_tag_fat(_tmpB2,sizeof(char),2U);});struct _fat_ptr _tmpB3=({const char*_tmpB4=",";_tag_fat(_tmpB4,sizeof(char),2U);});struct Cyc_List_List*_tmpB5=({({struct Cyc_List_List*(*_tmp756)(struct Cyc_PP_Doc*(*f)(void*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmpB6)(struct Cyc_PP_Doc*(*f)(void*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_PP_Doc*(*f)(void*),struct Cyc_List_List*x))Cyc_List_map;_tmpB6;});_tmp756(Cyc_Absynpp_typ2doc,ts);});});_tmpAE(_tmpAF,_tmpB1,_tmpB3,_tmpB5);});}
# 362
struct Cyc_PP_Doc*Cyc_Absynpp_tvar2doc(struct Cyc_Absyn_Tvar*tv){
# 439
struct _fat_ptr n=*tv->name;
# 442
if(Cyc_Absynpp_rewrite_temp_tvars &&({Cyc_Tcutil_is_temp_tvar(tv);})){
# 444
struct _fat_ptr kstring=({const char*_tmpC1="K";_tag_fat(_tmpC1,sizeof(char),2U);});
{void*_stmttmp4=({Cyc_Absyn_compress_kb(tv->kind);});void*_tmpB8=_stmttmp4;struct Cyc_Absyn_Kind*_tmpB9;struct Cyc_Absyn_Kind*_tmpBA;switch(*((int*)_tmpB8)){case 2U: _LL1: _tmpBA=((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmpB8)->f2;_LL2: {struct Cyc_Absyn_Kind*k=_tmpBA;
_tmpB9=k;goto _LL4;}case 0U: _LL3: _tmpB9=((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmpB8)->f1;_LL4: {struct Cyc_Absyn_Kind*k=_tmpB9;
kstring=({Cyc_Absynpp_kind2string(k);});goto _LL0;}default: _LL5: _LL6:
 goto _LL0;}_LL0:;}
# 450
return({struct Cyc_PP_Doc*(*_tmpBB)(struct _fat_ptr s)=Cyc_PP_text;struct _fat_ptr _tmpBC=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmpBF=({struct Cyc_String_pa_PrintArg_struct _tmp735;_tmp735.tag=0U,_tmp735.f1=(struct _fat_ptr)((struct _fat_ptr)kstring);_tmp735;});struct Cyc_String_pa_PrintArg_struct _tmpC0=({struct Cyc_String_pa_PrintArg_struct _tmp734;_tmp734.tag=0U,({struct _fat_ptr _tmp757=(struct _fat_ptr)((struct _fat_ptr)_fat_ptr_plus(n,sizeof(char),1));_tmp734.f1=_tmp757;});_tmp734;});void*_tmpBD[2U];_tmpBD[0]=& _tmpBF,_tmpBD[1]=& _tmpC0;({struct _fat_ptr _tmp758=({const char*_tmpBE="`G%s%s";_tag_fat(_tmpBE,sizeof(char),7U);});Cyc_aprintf(_tmp758,_tag_fat(_tmpBD,sizeof(void*),2U));});});_tmpBB(_tmpBC);});}
# 442
return({Cyc_PP_text(n);});}
# 362
struct Cyc_PP_Doc*Cyc_Absynpp_ktvar2doc(struct Cyc_Absyn_Tvar*tv){
# 456
void*_stmttmp5=({Cyc_Absyn_compress_kb(tv->kind);});void*_tmpC3=_stmttmp5;struct Cyc_Absyn_Kind*_tmpC4;struct Cyc_Absyn_Kind*_tmpC5;switch(*((int*)_tmpC3)){case 1U: _LL1: _LL2:
 goto _LL4;case 0U: if(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmpC3)->f1)->kind == Cyc_Absyn_BoxKind){if(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmpC3)->f1)->aliasqual == Cyc_Absyn_Aliasable){_LL3: _LL4:
 return({Cyc_Absynpp_tvar2doc(tv);});}else{goto _LL7;}}else{_LL7: _tmpC5=((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmpC3)->f1;_LL8: {struct Cyc_Absyn_Kind*k=_tmpC5;
# 460
return({struct Cyc_PP_Doc*_tmpC6[3U];({struct Cyc_PP_Doc*_tmp75B=({Cyc_Absynpp_tvar2doc(tv);});_tmpC6[0]=_tmp75B;}),({struct Cyc_PP_Doc*_tmp75A=({Cyc_PP_text(({const char*_tmpC7="::";_tag_fat(_tmpC7,sizeof(char),3U);}));});_tmpC6[1]=_tmp75A;}),({struct Cyc_PP_Doc*_tmp759=({Cyc_Absynpp_kind2doc(k);});_tmpC6[2]=_tmp759;});Cyc_PP_cat(_tag_fat(_tmpC6,sizeof(struct Cyc_PP_Doc*),3U));});}}default: _LL5: _tmpC4=((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmpC3)->f2;_LL6: {struct Cyc_Absyn_Kind*k=_tmpC4;
# 459
_tmpC5=k;goto _LL8;}}_LL0:;}
# 362
struct Cyc_PP_Doc*Cyc_Absynpp_ktvars2doc(struct Cyc_List_List*tvs){
# 465
return({struct Cyc_PP_Doc*(*_tmpC9)(struct _fat_ptr start,struct _fat_ptr stop,struct _fat_ptr sep,struct Cyc_List_List*l)=Cyc_PP_egroup;struct _fat_ptr _tmpCA=({const char*_tmpCB="<";_tag_fat(_tmpCB,sizeof(char),2U);});struct _fat_ptr _tmpCC=({const char*_tmpCD=">";_tag_fat(_tmpCD,sizeof(char),2U);});struct _fat_ptr _tmpCE=({const char*_tmpCF=",";_tag_fat(_tmpCF,sizeof(char),2U);});struct Cyc_List_List*_tmpD0=({({struct Cyc_List_List*(*_tmp75C)(struct Cyc_PP_Doc*(*f)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmpD1)(struct Cyc_PP_Doc*(*f)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_PP_Doc*(*f)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*x))Cyc_List_map;_tmpD1;});_tmp75C(Cyc_Absynpp_ktvar2doc,tvs);});});_tmpC9(_tmpCA,_tmpCC,_tmpCE,_tmpD0);});}
# 362
struct Cyc_PP_Doc*Cyc_Absynpp_tvars2doc(struct Cyc_List_List*tvs){
# 469
if(Cyc_Absynpp_print_all_kinds)
return({Cyc_Absynpp_ktvars2doc(tvs);});
# 469
return({struct Cyc_PP_Doc*(*_tmpD3)(struct _fat_ptr start,struct _fat_ptr stop,struct _fat_ptr sep,struct Cyc_List_List*l)=Cyc_PP_egroup;struct _fat_ptr _tmpD4=({const char*_tmpD5="<";_tag_fat(_tmpD5,sizeof(char),2U);});struct _fat_ptr _tmpD6=({const char*_tmpD7=">";_tag_fat(_tmpD7,sizeof(char),2U);});struct _fat_ptr _tmpD8=({const char*_tmpD9=",";_tag_fat(_tmpD9,sizeof(char),2U);});struct Cyc_List_List*_tmpDA=({({struct Cyc_List_List*(*_tmp75D)(struct Cyc_PP_Doc*(*f)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*x)=({
# 471
struct Cyc_List_List*(*_tmpDB)(struct Cyc_PP_Doc*(*f)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_PP_Doc*(*f)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*x))Cyc_List_map;_tmpDB;});_tmp75D(Cyc_Absynpp_tvar2doc,tvs);});});_tmpD3(_tmpD4,_tmpD6,_tmpD8,_tmpDA);});}struct _tuple18{struct Cyc_Absyn_Tqual f1;void*f2;};
# 362
struct Cyc_PP_Doc*Cyc_Absynpp_arg2doc(struct _tuple18*t){
# 475
return({Cyc_Absynpp_tqtd2doc((*t).f1,(*t).f2,0);});}
# 362
struct Cyc_PP_Doc*Cyc_Absynpp_args2doc(struct Cyc_List_List*ts){
# 479
return({struct Cyc_PP_Doc*(*_tmpDE)(struct _fat_ptr start,struct _fat_ptr stop,struct _fat_ptr sep,struct Cyc_List_List*l)=Cyc_PP_group;struct _fat_ptr _tmpDF=({const char*_tmpE0="(";_tag_fat(_tmpE0,sizeof(char),2U);});struct _fat_ptr _tmpE1=({const char*_tmpE2=")";_tag_fat(_tmpE2,sizeof(char),2U);});struct _fat_ptr _tmpE3=({const char*_tmpE4=",";_tag_fat(_tmpE4,sizeof(char),2U);});struct Cyc_List_List*_tmpE5=({({struct Cyc_List_List*(*_tmp75E)(struct Cyc_PP_Doc*(*f)(struct _tuple18*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmpE6)(struct Cyc_PP_Doc*(*f)(struct _tuple18*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_PP_Doc*(*f)(struct _tuple18*),struct Cyc_List_List*x))Cyc_List_map;_tmpE6;});_tmp75E(Cyc_Absynpp_arg2doc,ts);});});_tmpDE(_tmpDF,_tmpE1,_tmpE3,_tmpE5);});}
# 362
struct Cyc_PP_Doc*Cyc_Absynpp_noncallatt2doc(void*att){
# 483
void*_tmpE8=att;switch(*((int*)_tmpE8)){case 1U: _LL1: _LL2:
 goto _LL4;case 2U: _LL3: _LL4:
 goto _LL6;case 3U: _LL5: _LL6:
 return({Cyc_PP_nil_doc();});default: _LL7: _LL8:
 return({struct Cyc_PP_Doc*(*_tmpE9)(struct _fat_ptr s)=Cyc_PP_text;struct _fat_ptr _tmpEA=({Cyc_Absyn_attribute2string(att);});_tmpE9(_tmpEA);});}_LL0:;}
# 362
struct Cyc_PP_Doc*Cyc_Absynpp_callconv2doc(struct Cyc_List_List*atts){
# 492
for(0;atts != 0;atts=atts->tl){
void*_stmttmp6=(void*)atts->hd;void*_tmpEC=_stmttmp6;switch(*((int*)_tmpEC)){case 1U: _LL1: _LL2:
 return({Cyc_PP_text(({const char*_tmpED=" _stdcall ";_tag_fat(_tmpED,sizeof(char),11U);}));});case 2U: _LL3: _LL4:
 return({Cyc_PP_text(({const char*_tmpEE=" _cdecl ";_tag_fat(_tmpEE,sizeof(char),9U);}));});case 3U: _LL5: _LL6:
 return({Cyc_PP_text(({const char*_tmpEF=" _fastcall ";_tag_fat(_tmpEF,sizeof(char),12U);}));});default: _LL7: _LL8:
 goto _LL0;}_LL0:;}
# 499
return({Cyc_PP_nil_doc();});}
# 362
struct Cyc_PP_Doc*Cyc_Absynpp_noncallconv2doc(struct Cyc_List_List*atts){
# 503
int hasatt=0;
{struct Cyc_List_List*atts2=atts;for(0;atts2 != 0;atts2=atts2->tl){
void*_stmttmp7=(void*)atts2->hd;void*_tmpF1=_stmttmp7;switch(*((int*)_tmpF1)){case 1U: _LL1: _LL2:
 goto _LL4;case 2U: _LL3: _LL4:
 goto _LL6;case 3U: _LL5: _LL6:
 goto _LL0;default: _LL7: _LL8:
 hasatt=1;goto _LL0;}_LL0:;}}
# 511
if(!hasatt)
return({Cyc_PP_nil_doc();});
# 511
return({struct Cyc_PP_Doc*_tmpF2[3U];({
# 513
struct Cyc_PP_Doc*_tmp762=({Cyc_PP_text(({const char*_tmpF3=" __declspec(";_tag_fat(_tmpF3,sizeof(char),13U);}));});_tmpF2[0]=_tmp762;}),({
struct Cyc_PP_Doc*_tmp761=({struct Cyc_PP_Doc*(*_tmpF4)(struct _fat_ptr start,struct _fat_ptr stop,struct _fat_ptr sep,struct Cyc_List_List*l)=Cyc_PP_group;struct _fat_ptr _tmpF5=({const char*_tmpF6="";_tag_fat(_tmpF6,sizeof(char),1U);});struct _fat_ptr _tmpF7=({const char*_tmpF8="";_tag_fat(_tmpF8,sizeof(char),1U);});struct _fat_ptr _tmpF9=({const char*_tmpFA=" ";_tag_fat(_tmpFA,sizeof(char),2U);});struct Cyc_List_List*_tmpFB=({({struct Cyc_List_List*(*_tmp760)(struct Cyc_PP_Doc*(*f)(void*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmpFC)(struct Cyc_PP_Doc*(*f)(void*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_PP_Doc*(*f)(void*),struct Cyc_List_List*x))Cyc_List_map;_tmpFC;});_tmp760(Cyc_Absynpp_noncallatt2doc,atts);});});_tmpF4(_tmpF5,_tmpF7,_tmpF9,_tmpFB);});_tmpF2[1]=_tmp761;}),({
struct Cyc_PP_Doc*_tmp75F=({Cyc_PP_text(({const char*_tmpFD=")";_tag_fat(_tmpFD,sizeof(char),2U);}));});_tmpF2[2]=_tmp75F;});Cyc_PP_cat(_tag_fat(_tmpF2,sizeof(struct Cyc_PP_Doc*),3U));});}
# 362
struct Cyc_PP_Doc*Cyc_Absynpp_att2doc(void*a){
# 519
return({struct Cyc_PP_Doc*(*_tmpFF)(struct _fat_ptr s)=Cyc_PP_text;struct _fat_ptr _tmp100=({Cyc_Absyn_attribute2string(a);});_tmpFF(_tmp100);});}
# 362
struct Cyc_PP_Doc*Cyc_Absynpp_atts2doc(struct Cyc_List_List*atts){
# 523
if(atts == 0)return({Cyc_PP_nil_doc();});{enum Cyc_Cyclone_C_Compilers _tmp102=Cyc_Cyclone_c_compiler;if(_tmp102 == Cyc_Cyclone_Vc_c){_LL1: _LL2:
# 525
 return({Cyc_Absynpp_noncallconv2doc(atts);});}else{_LL3: _LL4:
# 527
 return({struct Cyc_PP_Doc*_tmp103[2U];({struct Cyc_PP_Doc*_tmp765=({Cyc_PP_text(({const char*_tmp104=" __attribute__";_tag_fat(_tmp104,sizeof(char),15U);}));});_tmp103[0]=_tmp765;}),({
struct Cyc_PP_Doc*_tmp764=({struct Cyc_PP_Doc*(*_tmp105)(struct _fat_ptr start,struct _fat_ptr stop,struct _fat_ptr sep,struct Cyc_List_List*l)=Cyc_PP_group;struct _fat_ptr _tmp106=({const char*_tmp107="((";_tag_fat(_tmp107,sizeof(char),3U);});struct _fat_ptr _tmp108=({const char*_tmp109="))";_tag_fat(_tmp109,sizeof(char),3U);});struct _fat_ptr _tmp10A=({const char*_tmp10B=",";_tag_fat(_tmp10B,sizeof(char),2U);});struct Cyc_List_List*_tmp10C=({({struct Cyc_List_List*(*_tmp763)(struct Cyc_PP_Doc*(*f)(void*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp10D)(struct Cyc_PP_Doc*(*f)(void*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_PP_Doc*(*f)(void*),struct Cyc_List_List*x))Cyc_List_map;_tmp10D;});_tmp763(Cyc_Absynpp_att2doc,atts);});});_tmp105(_tmp106,_tmp108,_tmp10A,_tmp10C);});_tmp103[1]=_tmp764;});Cyc_PP_cat(_tag_fat(_tmp103,sizeof(struct Cyc_PP_Doc*),2U));});}_LL0:;}}
# 362
int Cyc_Absynpp_next_is_pointer(struct Cyc_List_List*tms){
# 533
if(tms == 0)return 0;{void*_stmttmp8=(void*)tms->hd;void*_tmp10F=_stmttmp8;switch(*((int*)_tmp10F)){case 2U: _LL1: _LL2:
# 535
 return 1;case 5U: _LL3: _LL4: {
# 537
enum Cyc_Cyclone_C_Compilers _tmp110=Cyc_Cyclone_c_compiler;if(_tmp110 == Cyc_Cyclone_Gcc_c){_LL8: _LL9:
 return 0;}else{_LLA: _LLB:
 return({Cyc_Absynpp_next_is_pointer(tms->tl);});}_LL7:;}default: _LL5: _LL6:
# 541
 return 0;}_LL0:;}}
# 362
struct Cyc_PP_Doc*Cyc_Absynpp_ntyp2doc(void*t);
# 548
static struct Cyc_PP_Doc*Cyc_Absynpp_cache_question=0;
static struct Cyc_PP_Doc*Cyc_Absynpp_question(){
if(!((unsigned)Cyc_Absynpp_cache_question)){
if(Cyc_PP_tex_output)
Cyc_Absynpp_cache_question=({Cyc_PP_text_width(({const char*_tmp112="{?}";_tag_fat(_tmp112,sizeof(char),4U);}),1);});else{
Cyc_Absynpp_cache_question=({Cyc_PP_text(({const char*_tmp113="?";_tag_fat(_tmp113,sizeof(char),2U);}));});}}
# 550
return(struct Cyc_PP_Doc*)_check_null(Cyc_Absynpp_cache_question);}
# 549
static struct Cyc_PP_Doc*Cyc_Absynpp_cache_lb=0;
# 558
static struct Cyc_PP_Doc*Cyc_Absynpp_lb(){
if(!((unsigned)Cyc_Absynpp_cache_lb)){
if(Cyc_PP_tex_output)
Cyc_Absynpp_cache_lb=({Cyc_PP_text_width(({const char*_tmp115="{\\lb}";_tag_fat(_tmp115,sizeof(char),6U);}),1);});else{
Cyc_Absynpp_cache_lb=({Cyc_PP_text(({const char*_tmp116="{";_tag_fat(_tmp116,sizeof(char),2U);}));});}}
# 559
return(struct Cyc_PP_Doc*)_check_null(Cyc_Absynpp_cache_lb);}
# 558
static struct Cyc_PP_Doc*Cyc_Absynpp_cache_rb=0;
# 567
static struct Cyc_PP_Doc*Cyc_Absynpp_rb(){
if(!((unsigned)Cyc_Absynpp_cache_rb)){
if(Cyc_PP_tex_output)
Cyc_Absynpp_cache_rb=({Cyc_PP_text_width(({const char*_tmp118="{\\rb}";_tag_fat(_tmp118,sizeof(char),6U);}),1);});else{
Cyc_Absynpp_cache_rb=({Cyc_PP_text(({const char*_tmp119="}";_tag_fat(_tmp119,sizeof(char),2U);}));});}}
# 568
return(struct Cyc_PP_Doc*)_check_null(Cyc_Absynpp_cache_rb);}
# 567
static struct Cyc_PP_Doc*Cyc_Absynpp_cache_dollar=0;
# 576
static struct Cyc_PP_Doc*Cyc_Absynpp_dollar(){
if(!((unsigned)Cyc_Absynpp_cache_dollar)){
if(Cyc_PP_tex_output)
Cyc_Absynpp_cache_dollar=({Cyc_PP_text_width(({const char*_tmp11B="\\$";_tag_fat(_tmp11B,sizeof(char),3U);}),1);});else{
Cyc_Absynpp_cache_dollar=({Cyc_PP_text(({const char*_tmp11C="$";_tag_fat(_tmp11C,sizeof(char),2U);}));});}}
# 577
return(struct Cyc_PP_Doc*)_check_null(Cyc_Absynpp_cache_dollar);}
# 576
struct Cyc_PP_Doc*Cyc_Absynpp_group_braces(struct _fat_ptr sep,struct Cyc_List_List*ss){
# 585
return({struct Cyc_PP_Doc*_tmp11E[3U];({struct Cyc_PP_Doc*_tmp768=({Cyc_Absynpp_lb();});_tmp11E[0]=_tmp768;}),({struct Cyc_PP_Doc*_tmp767=({Cyc_PP_seq(sep,ss);});_tmp11E[1]=_tmp767;}),({struct Cyc_PP_Doc*_tmp766=({Cyc_Absynpp_rb();});_tmp11E[2]=_tmp766;});Cyc_PP_cat(_tag_fat(_tmp11E,sizeof(struct Cyc_PP_Doc*),3U));});}
# 589
static void Cyc_Absynpp_print_tms(struct Cyc_List_List*tms){
for(0;tms != 0;tms=tms->tl){
void*_stmttmp9=(void*)tms->hd;void*_tmp120=_stmttmp9;struct Cyc_List_List*_tmp121;switch(*((int*)_tmp120)){case 0U: _LL1: _LL2:
({void*_tmp122=0U;({struct Cyc___cycFILE*_tmp76A=Cyc_stderr;struct _fat_ptr _tmp769=({const char*_tmp123="Carray_mod ";_tag_fat(_tmp123,sizeof(char),12U);});Cyc_fprintf(_tmp76A,_tmp769,_tag_fat(_tmp122,sizeof(void*),0U));});});goto _LL0;case 1U: _LL3: _LL4:
({void*_tmp124=0U;({struct Cyc___cycFILE*_tmp76C=Cyc_stderr;struct _fat_ptr _tmp76B=({const char*_tmp125="ConstArray_mod ";_tag_fat(_tmp125,sizeof(char),16U);});Cyc_fprintf(_tmp76C,_tmp76B,_tag_fat(_tmp124,sizeof(void*),0U));});});goto _LL0;case 3U: if(((struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*)((struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct*)_tmp120)->f1)->tag == 1U){_LL5: _tmp121=((struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*)((struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct*)_tmp120)->f1)->f1;_LL6: {struct Cyc_List_List*ys=_tmp121;
# 595
({void*_tmp126=0U;({struct Cyc___cycFILE*_tmp76E=Cyc_stderr;struct _fat_ptr _tmp76D=({const char*_tmp127="Function_mod(";_tag_fat(_tmp127,sizeof(char),14U);});Cyc_fprintf(_tmp76E,_tmp76D,_tag_fat(_tmp126,sizeof(void*),0U));});});
for(0;ys != 0;ys=ys->tl){
struct _fat_ptr*v=(*((struct _tuple9*)ys->hd)).f1;
if(v == 0)({void*_tmp128=0U;({struct Cyc___cycFILE*_tmp770=Cyc_stderr;struct _fat_ptr _tmp76F=({const char*_tmp129="?";_tag_fat(_tmp129,sizeof(char),2U);});Cyc_fprintf(_tmp770,_tmp76F,_tag_fat(_tmp128,sizeof(void*),0U));});});else{
({void*_tmp12A=0U;({struct Cyc___cycFILE*_tmp772=Cyc_stderr;struct _fat_ptr _tmp771=*v;Cyc_fprintf(_tmp772,_tmp771,_tag_fat(_tmp12A,sizeof(void*),0U));});});}
if(ys->tl != 0)({void*_tmp12B=0U;({struct Cyc___cycFILE*_tmp774=Cyc_stderr;struct _fat_ptr _tmp773=({const char*_tmp12C=",";_tag_fat(_tmp12C,sizeof(char),2U);});Cyc_fprintf(_tmp774,_tmp773,_tag_fat(_tmp12B,sizeof(void*),0U));});});}
# 602
({void*_tmp12D=0U;({struct Cyc___cycFILE*_tmp776=Cyc_stderr;struct _fat_ptr _tmp775=({const char*_tmp12E=") ";_tag_fat(_tmp12E,sizeof(char),3U);});Cyc_fprintf(_tmp776,_tmp775,_tag_fat(_tmp12D,sizeof(void*),0U));});});
goto _LL0;}}else{_LL7: _LL8:
# 605
({void*_tmp12F=0U;({struct Cyc___cycFILE*_tmp778=Cyc_stderr;struct _fat_ptr _tmp777=({const char*_tmp130="Function_mod()";_tag_fat(_tmp130,sizeof(char),15U);});Cyc_fprintf(_tmp778,_tmp777,_tag_fat(_tmp12F,sizeof(void*),0U));});});goto _LL0;}case 5U: _LL9: _LLA:
({void*_tmp131=0U;({struct Cyc___cycFILE*_tmp77A=Cyc_stderr;struct _fat_ptr _tmp779=({const char*_tmp132="Attributes_mod ";_tag_fat(_tmp132,sizeof(char),16U);});Cyc_fprintf(_tmp77A,_tmp779,_tag_fat(_tmp131,sizeof(void*),0U));});});goto _LL0;case 4U: _LLB: _LLC:
({void*_tmp133=0U;({struct Cyc___cycFILE*_tmp77C=Cyc_stderr;struct _fat_ptr _tmp77B=({const char*_tmp134="TypeParams_mod ";_tag_fat(_tmp134,sizeof(char),16U);});Cyc_fprintf(_tmp77C,_tmp77B,_tag_fat(_tmp133,sizeof(void*),0U));});});goto _LL0;default: _LLD: _LLE:
({void*_tmp135=0U;({struct Cyc___cycFILE*_tmp77E=Cyc_stderr;struct _fat_ptr _tmp77D=({const char*_tmp136="Pointer_mod ";_tag_fat(_tmp136,sizeof(char),13U);});Cyc_fprintf(_tmp77E,_tmp77D,_tag_fat(_tmp135,sizeof(void*),0U));});});goto _LL0;}_LL0:;}
# 610
({void*_tmp137=0U;({struct Cyc___cycFILE*_tmp780=Cyc_stderr;struct _fat_ptr _tmp77F=({const char*_tmp138="\n";_tag_fat(_tmp138,sizeof(char),2U);});Cyc_fprintf(_tmp780,_tmp77F,_tag_fat(_tmp137,sizeof(void*),0U));});});}
# 589
struct Cyc_PP_Doc*Cyc_Absynpp_rgn2doc(void*t);
# 615
struct Cyc_PP_Doc*Cyc_Absynpp_dtms2doc(int is_char_ptr,struct Cyc_PP_Doc*d,struct Cyc_List_List*tms){
if(tms == 0)return d;{struct Cyc_PP_Doc*rest=({Cyc_Absynpp_dtms2doc(0,d,tms->tl);});
# 618
struct Cyc_PP_Doc*p_rest=({struct Cyc_PP_Doc*_tmp188[3U];({struct Cyc_PP_Doc*_tmp782=({Cyc_PP_text(({const char*_tmp189="(";_tag_fat(_tmp189,sizeof(char),2U);}));});_tmp188[0]=_tmp782;}),_tmp188[1]=rest,({struct Cyc_PP_Doc*_tmp781=({Cyc_PP_text(({const char*_tmp18A=")";_tag_fat(_tmp18A,sizeof(char),2U);}));});_tmp188[2]=_tmp781;});Cyc_PP_cat(_tag_fat(_tmp188,sizeof(struct Cyc_PP_Doc*),3U));});
void*_stmttmpA=(void*)tms->hd;void*_tmp13A=_stmttmpA;struct Cyc_Absyn_Tqual _tmp13F;void*_tmp13E;void*_tmp13D;void*_tmp13C;void*_tmp13B;int _tmp142;unsigned _tmp141;struct Cyc_List_List*_tmp140;struct Cyc_List_List*_tmp143;void*_tmp144;void*_tmp146;struct Cyc_Absyn_Exp*_tmp145;void*_tmp147;switch(*((int*)_tmp13A)){case 0U: _LL1: _tmp147=(void*)((struct Cyc_Absyn_Carray_mod_Absyn_Type_modifier_struct*)_tmp13A)->f1;_LL2: {void*zeroterm=_tmp147;
# 621
if(({Cyc_Absynpp_next_is_pointer(tms->tl);}))rest=p_rest;return({struct Cyc_PP_Doc*_tmp148[2U];_tmp148[0]=rest,
({Cyc_Absyn_type2bool(0,zeroterm);})?({
struct Cyc_PP_Doc*_tmp784=({Cyc_PP_text(({const char*_tmp149="[]@zeroterm";_tag_fat(_tmp149,sizeof(char),12U);}));});_tmp148[1]=_tmp784;}):({struct Cyc_PP_Doc*_tmp783=({Cyc_PP_text(({const char*_tmp14A="[]";_tag_fat(_tmp14A,sizeof(char),3U);}));});_tmp148[1]=_tmp783;});Cyc_PP_cat(_tag_fat(_tmp148,sizeof(struct Cyc_PP_Doc*),2U));});}case 1U: _LL3: _tmp145=((struct Cyc_Absyn_ConstArray_mod_Absyn_Type_modifier_struct*)_tmp13A)->f1;_tmp146=(void*)((struct Cyc_Absyn_ConstArray_mod_Absyn_Type_modifier_struct*)_tmp13A)->f2;_LL4: {struct Cyc_Absyn_Exp*e=_tmp145;void*zeroterm=_tmp146;
# 625
if(({Cyc_Absynpp_next_is_pointer(tms->tl);}))rest=p_rest;return({struct Cyc_PP_Doc*_tmp14B[4U];_tmp14B[0]=rest,({
struct Cyc_PP_Doc*_tmp788=({Cyc_PP_text(({const char*_tmp14C="[";_tag_fat(_tmp14C,sizeof(char),2U);}));});_tmp14B[1]=_tmp788;}),({struct Cyc_PP_Doc*_tmp787=({Cyc_Absynpp_exp2doc(e);});_tmp14B[2]=_tmp787;}),
({Cyc_Absyn_type2bool(0,zeroterm);})?({struct Cyc_PP_Doc*_tmp786=({Cyc_PP_text(({const char*_tmp14D="]@zeroterm";_tag_fat(_tmp14D,sizeof(char),11U);}));});_tmp14B[3]=_tmp786;}):({struct Cyc_PP_Doc*_tmp785=({Cyc_PP_text(({const char*_tmp14E="]";_tag_fat(_tmp14E,sizeof(char),2U);}));});_tmp14B[3]=_tmp785;});Cyc_PP_cat(_tag_fat(_tmp14B,sizeof(struct Cyc_PP_Doc*),4U));});}case 3U: _LL5: _tmp144=(void*)((struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct*)_tmp13A)->f1;_LL6: {void*args=_tmp144;
# 629
if(({Cyc_Absynpp_next_is_pointer(tms->tl);}))rest=p_rest;{void*_tmp14F=args;unsigned _tmp151;struct Cyc_List_List*_tmp150;int _tmp15C;struct Cyc_List_List*_tmp15B;struct Cyc_List_List*_tmp15A;struct Cyc_List_List*_tmp159;struct Cyc_Absyn_Exp*_tmp158;struct Cyc_Absyn_Exp*_tmp157;struct Cyc_List_List*_tmp156;void*_tmp155;struct Cyc_Absyn_VarargInfo*_tmp154;int _tmp153;struct Cyc_List_List*_tmp152;if(((struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*)_tmp14F)->tag == 1U){_LLE: _tmp152=((struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*)_tmp14F)->f1;_tmp153=((struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*)_tmp14F)->f2;_tmp154=((struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*)_tmp14F)->f3;_tmp155=(void*)((struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*)_tmp14F)->f4;_tmp156=((struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*)_tmp14F)->f5;_tmp157=((struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*)_tmp14F)->f6;_tmp158=((struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*)_tmp14F)->f7;_tmp159=((struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*)_tmp14F)->f8;_tmp15A=((struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*)_tmp14F)->f9;_tmp15B=((struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*)_tmp14F)->f10;_tmp15C=((struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*)_tmp14F)->f11;_LLF: {struct Cyc_List_List*args2=_tmp152;int c_varargs=_tmp153;struct Cyc_Absyn_VarargInfo*cyc_varargs=_tmp154;void*effopt=_tmp155;struct Cyc_List_List*rgn_po=_tmp156;struct Cyc_Absyn_Exp*req=_tmp157;struct Cyc_Absyn_Exp*ens=_tmp158;struct Cyc_List_List*ieff=_tmp159;struct Cyc_List_List*oeff=_tmp15A;struct Cyc_List_List*throws=_tmp15B;int reentrant=_tmp15C;
# 632
return({struct Cyc_PP_Doc*_tmp15D[2U];_tmp15D[0]=rest,({struct Cyc_PP_Doc*_tmp789=({Cyc_Absynpp_funargs2doc(args2,c_varargs,cyc_varargs,effopt,rgn_po,req,ens,ieff,oeff,throws,reentrant);});_tmp15D[1]=_tmp789;});Cyc_PP_cat(_tag_fat(_tmp15D,sizeof(struct Cyc_PP_Doc*),2U));});}}else{_LL10: _tmp150=((struct Cyc_Absyn_NoTypes_Absyn_Funcparams_struct*)_tmp14F)->f1;_tmp151=((struct Cyc_Absyn_NoTypes_Absyn_Funcparams_struct*)_tmp14F)->f2;_LL11: {struct Cyc_List_List*sl=_tmp150;unsigned loc=_tmp151;
# 634
return({struct Cyc_PP_Doc*_tmp15E[2U];_tmp15E[0]=rest,({struct Cyc_PP_Doc*_tmp78B=({struct Cyc_PP_Doc*(*_tmp15F)(struct _fat_ptr start,struct _fat_ptr stop,struct _fat_ptr sep,struct Cyc_List_List*l)=Cyc_PP_group;struct _fat_ptr _tmp160=({const char*_tmp161="(";_tag_fat(_tmp161,sizeof(char),2U);});struct _fat_ptr _tmp162=({const char*_tmp163=")";_tag_fat(_tmp163,sizeof(char),2U);});struct _fat_ptr _tmp164=({const char*_tmp165=",";_tag_fat(_tmp165,sizeof(char),2U);});struct Cyc_List_List*_tmp166=({({struct Cyc_List_List*(*_tmp78A)(struct Cyc_PP_Doc*(*f)(struct _fat_ptr*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp167)(struct Cyc_PP_Doc*(*f)(struct _fat_ptr*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_PP_Doc*(*f)(struct _fat_ptr*),struct Cyc_List_List*x))Cyc_List_map;_tmp167;});_tmp78A(Cyc_PP_textptr,sl);});});_tmp15F(_tmp160,_tmp162,_tmp164,_tmp166);});_tmp15E[1]=_tmp78B;});Cyc_PP_cat(_tag_fat(_tmp15E,sizeof(struct Cyc_PP_Doc*),2U));});}}_LLD:;}}case 5U: _LL7: _tmp143=((struct Cyc_Absyn_Attributes_mod_Absyn_Type_modifier_struct*)_tmp13A)->f2;_LL8: {struct Cyc_List_List*atts=_tmp143;
# 637
enum Cyc_Cyclone_C_Compilers _tmp168=Cyc_Cyclone_c_compiler;if(_tmp168 == Cyc_Cyclone_Gcc_c){_LL13: _LL14:
# 639
 if(({Cyc_Absynpp_next_is_pointer(tms->tl);}))
rest=p_rest;
# 639
return({struct Cyc_PP_Doc*_tmp169[2U];_tmp169[0]=rest,({
# 641
struct Cyc_PP_Doc*_tmp78C=({Cyc_Absynpp_atts2doc(atts);});_tmp169[1]=_tmp78C;});Cyc_PP_cat(_tag_fat(_tmp169,sizeof(struct Cyc_PP_Doc*),2U));});}else{_LL15: _LL16:
# 643
 if(({Cyc_Absynpp_next_is_pointer(tms->tl);}))
return({struct Cyc_PP_Doc*_tmp16A[2U];({struct Cyc_PP_Doc*_tmp78D=({Cyc_Absynpp_callconv2doc(atts);});_tmp16A[0]=_tmp78D;}),_tmp16A[1]=rest;Cyc_PP_cat(_tag_fat(_tmp16A,sizeof(struct Cyc_PP_Doc*),2U));});
# 643
return rest;}_LL12:;}case 4U: _LL9: _tmp140=((struct Cyc_Absyn_TypeParams_mod_Absyn_Type_modifier_struct*)_tmp13A)->f1;_tmp141=((struct Cyc_Absyn_TypeParams_mod_Absyn_Type_modifier_struct*)_tmp13A)->f2;_tmp142=((struct Cyc_Absyn_TypeParams_mod_Absyn_Type_modifier_struct*)_tmp13A)->f3;_LLA: {struct Cyc_List_List*ts=_tmp140;unsigned loc=_tmp141;int print_kinds=_tmp142;
# 648
if(({Cyc_Absynpp_next_is_pointer(tms->tl);}))rest=p_rest;if(print_kinds)
# 650
return({struct Cyc_PP_Doc*_tmp16B[2U];_tmp16B[0]=rest,({struct Cyc_PP_Doc*_tmp78E=({Cyc_Absynpp_ktvars2doc(ts);});_tmp16B[1]=_tmp78E;});Cyc_PP_cat(_tag_fat(_tmp16B,sizeof(struct Cyc_PP_Doc*),2U));});else{
# 652
return({struct Cyc_PP_Doc*_tmp16C[2U];_tmp16C[0]=rest,({struct Cyc_PP_Doc*_tmp78F=({Cyc_Absynpp_tvars2doc(ts);});_tmp16C[1]=_tmp78F;});Cyc_PP_cat(_tag_fat(_tmp16C,sizeof(struct Cyc_PP_Doc*),2U));});}}default: _LLB: _tmp13B=(((struct Cyc_Absyn_Pointer_mod_Absyn_Type_modifier_struct*)_tmp13A)->f1).rgn;_tmp13C=(((struct Cyc_Absyn_Pointer_mod_Absyn_Type_modifier_struct*)_tmp13A)->f1).nullable;_tmp13D=(((struct Cyc_Absyn_Pointer_mod_Absyn_Type_modifier_struct*)_tmp13A)->f1).bounds;_tmp13E=(((struct Cyc_Absyn_Pointer_mod_Absyn_Type_modifier_struct*)_tmp13A)->f1).zero_term;_tmp13F=((struct Cyc_Absyn_Pointer_mod_Absyn_Type_modifier_struct*)_tmp13A)->f2;_LLC: {void*rgn=_tmp13B;void*nullable=_tmp13C;void*bd=_tmp13D;void*zt=_tmp13E;struct Cyc_Absyn_Tqual tq2=_tmp13F;
# 656
struct Cyc_PP_Doc*ptr;
struct Cyc_PP_Doc*mt=({Cyc_PP_nil_doc();});
struct Cyc_PP_Doc*ztd=mt;
struct Cyc_PP_Doc*rgd=mt;
struct Cyc_PP_Doc*tqd=({Cyc_Absynpp_tqual2doc(tq2);});
{void*_stmttmpB=({Cyc_Tcutil_compress(bd);});void*_tmp16D=_stmttmpB;void*_tmp16E;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp16D)->tag == 0U)switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp16D)->f1)){case 16U: _LL18: _LL19:
 ptr=({Cyc_Absynpp_question();});goto _LL17;case 15U: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp16D)->f2 != 0){_LL1A: _tmp16E=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp16D)->f2)->hd;_LL1B: {void*targ=_tmp16E;
# 664
{void*_stmttmpC=({Cyc_Tcutil_compress(targ);});void*_tmp16F=_stmttmpC;struct Cyc_Absyn_Exp*_tmp170;if(((struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_tmp16F)->tag == 9U){_LL1F: _tmp170=((struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_tmp16F)->f1;_LL20: {struct Cyc_Absyn_Exp*e=_tmp170;
# 666
ptr=({struct Cyc_PP_Doc*(*_tmp171)(struct _fat_ptr s)=Cyc_PP_text;struct _fat_ptr _tmp172=({Cyc_Absyn_type2bool(1,nullable);})?({const char*_tmp173="*";_tag_fat(_tmp173,sizeof(char),2U);}):({const char*_tmp174="@";_tag_fat(_tmp174,sizeof(char),2U);});_tmp171(_tmp172);});{
struct _tuple13 _stmttmpD=({Cyc_Evexp_eval_const_uint_exp(e);});int _tmp176;unsigned _tmp175;_LL24: _tmp175=_stmttmpD.f1;_tmp176=_stmttmpD.f2;_LL25: {unsigned val=_tmp175;int known=_tmp176;
if(!known || val != (unsigned)1)
ptr=({struct Cyc_PP_Doc*_tmp177[4U];_tmp177[0]=ptr,({struct Cyc_PP_Doc*_tmp792=({Cyc_Absynpp_lb();});_tmp177[1]=_tmp792;}),({struct Cyc_PP_Doc*_tmp791=({Cyc_Absynpp_exp2doc(e);});_tmp177[2]=_tmp791;}),({struct Cyc_PP_Doc*_tmp790=({Cyc_Absynpp_rb();});_tmp177[3]=_tmp790;});Cyc_PP_cat(_tag_fat(_tmp177,sizeof(struct Cyc_PP_Doc*),4U));});
# 668
goto _LL1E;}}}}else{_LL21: _LL22:
# 672
 ptr=({struct Cyc_PP_Doc*(*_tmp178)(struct _fat_ptr s)=Cyc_PP_text;struct _fat_ptr _tmp179=({Cyc_Absyn_type2bool(1,nullable);})?({const char*_tmp17A="*";_tag_fat(_tmp17A,sizeof(char),2U);}):({const char*_tmp17B="@";_tag_fat(_tmp17B,sizeof(char),2U);});_tmp178(_tmp179);});
ptr=({struct Cyc_PP_Doc*_tmp17C[4U];_tmp17C[0]=ptr,({struct Cyc_PP_Doc*_tmp795=({Cyc_Absynpp_lb();});_tmp17C[1]=_tmp795;}),({struct Cyc_PP_Doc*_tmp794=({Cyc_Absynpp_typ2doc(targ);});_tmp17C[2]=_tmp794;}),({struct Cyc_PP_Doc*_tmp793=({Cyc_Absynpp_rb();});_tmp17C[3]=_tmp793;});Cyc_PP_cat(_tag_fat(_tmp17C,sizeof(struct Cyc_PP_Doc*),4U));});
goto _LL1E;}_LL1E:;}
# 676
goto _LL17;}}else{goto _LL1C;}default: goto _LL1C;}else{_LL1C: _LL1D:
# 678
 ptr=({struct Cyc_PP_Doc*(*_tmp17D)(struct _fat_ptr s)=Cyc_PP_text;struct _fat_ptr _tmp17E=({Cyc_Absyn_type2bool(1,nullable);})?({const char*_tmp17F="*";_tag_fat(_tmp17F,sizeof(char),2U);}):({const char*_tmp180="@";_tag_fat(_tmp180,sizeof(char),2U);});_tmp17D(_tmp17E);});
ptr=({struct Cyc_PP_Doc*_tmp181[4U];_tmp181[0]=ptr,({struct Cyc_PP_Doc*_tmp798=({Cyc_Absynpp_lb();});_tmp181[1]=_tmp798;}),({struct Cyc_PP_Doc*_tmp797=({Cyc_Absynpp_typ2doc(bd);});_tmp181[2]=_tmp797;}),({struct Cyc_PP_Doc*_tmp796=({Cyc_Absynpp_rb();});_tmp181[3]=_tmp796;});Cyc_PP_cat(_tag_fat(_tmp181,sizeof(struct Cyc_PP_Doc*),4U));});
goto _LL17;}_LL17:;}
# 683
if(Cyc_Absynpp_print_zeroterm){
if(!is_char_ptr &&({Cyc_Absyn_type2bool(0,zt);}))
ztd=({Cyc_PP_text(({const char*_tmp182="@zeroterm";_tag_fat(_tmp182,sizeof(char),10U);}));});else{
if(is_char_ptr && !({Cyc_Absyn_type2bool(0,zt);}))
ztd=({Cyc_PP_text(({const char*_tmp183="@nozeroterm";_tag_fat(_tmp183,sizeof(char),12U);}));});}}
# 683
{void*_stmttmpE=({Cyc_Tcutil_compress(rgn);});void*_tmp184=_stmttmpE;switch(*((int*)_tmp184)){case 0U: if(((struct Cyc_Absyn_HeapCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp184)->f1)->tag == 6U){_LL27: _LL28:
# 690
 goto _LL26;}else{goto _LL2B;}case 1U: _LL29: if(Cyc_Absynpp_print_for_cycdoc){_LL2A:
 goto _LL26;}else{goto _LL2B;}default: _LL2B: _LL2C:
 rgd=({Cyc_Absynpp_rgn2doc(rgn);});}_LL26:;}{
# 694
struct Cyc_PP_Doc*spacer1=tqd != mt &&(ztd != mt || rgd != mt)?({Cyc_PP_text(({const char*_tmp187=" ";_tag_fat(_tmp187,sizeof(char),2U);}));}): mt;
struct Cyc_PP_Doc*spacer2=rest != mt?({Cyc_PP_text(({const char*_tmp186=" ";_tag_fat(_tmp186,sizeof(char),2U);}));}): mt;
return({struct Cyc_PP_Doc*_tmp185[7U];_tmp185[0]=ptr,_tmp185[1]=ztd,_tmp185[2]=rgd,_tmp185[3]=spacer1,_tmp185[4]=tqd,_tmp185[5]=spacer2,_tmp185[6]=rest;Cyc_PP_cat(_tag_fat(_tmp185,sizeof(struct Cyc_PP_Doc*),7U));});}}}_LL0:;}}
# 615
struct Cyc_PP_Doc*Cyc_Absynpp_rgn2doc(void*t){
# 701
void*_stmttmpF=({Cyc_Tcutil_compress(t);});void*_tmp18C=_stmttmpF;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp18C)->tag == 0U)switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp18C)->f1)){case 6U: _LL1: _LL2:
 return({Cyc_PP_text(({const char*_tmp18D="`H";_tag_fat(_tmp18D,sizeof(char),3U);}));});case 7U: _LL3: _LL4:
 return({Cyc_PP_text(({const char*_tmp18E="`U";_tag_fat(_tmp18E,sizeof(char),3U);}));});case 8U: _LL5: _LL6:
 return({Cyc_PP_text(({const char*_tmp18F="`RC";_tag_fat(_tmp18F,sizeof(char),4U);}));});default: goto _LL7;}else{_LL7: _LL8:
 return({Cyc_Absynpp_ntyp2doc(t);});}_LL0:;}
# 710
static void Cyc_Absynpp_effects2docs(struct Cyc_List_List**rgions,struct Cyc_List_List**effects,void*t){
# 714
void*_stmttmp10=({Cyc_Tcutil_compress(t);});void*_tmp191=_stmttmp10;struct Cyc_List_List*_tmp192;void*_tmp193;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp191)->tag == 0U)switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp191)->f1)){case 9U: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp191)->f2 != 0){_LL1: _tmp193=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp191)->f2)->hd;_LL2: {void*r=_tmp193;
# 716
({struct Cyc_List_List*_tmp79A=({struct Cyc_List_List*_tmp194=_cycalloc(sizeof(*_tmp194));({struct Cyc_PP_Doc*_tmp799=({Cyc_Absynpp_rgn2doc(r);});_tmp194->hd=_tmp799;}),_tmp194->tl=*rgions;_tmp194;});*rgions=_tmp79A;});goto _LL0;}}else{goto _LL7;}case 10U: _LL3: _LL4: {
# 719
struct Cyc_PP_Doc*s=({struct Cyc_PP_Doc*_tmp196[1U];({struct Cyc_PP_Doc*_tmp79B=({struct Cyc_PP_Doc*(*_tmp197)(struct _fat_ptr s)=Cyc_PP_text;struct _fat_ptr _tmp198=({struct _fat_ptr(*_tmp199)(struct Cyc_Absyn_RgnEffect*r)=Cyc_Absyn_rgneffect2string;struct Cyc_Absyn_RgnEffect*_tmp19A=({Cyc_Absyn_type2rgneffect(t);});_tmp199(_tmp19A);});_tmp197(_tmp198);});_tmp196[0]=_tmp79B;});Cyc_PP_cat(_tag_fat(_tmp196,sizeof(struct Cyc_PP_Doc*),1U));});
({struct Cyc_List_List*_tmp79C=({struct Cyc_List_List*_tmp195=_cycalloc(sizeof(*_tmp195));_tmp195->hd=s,_tmp195->tl=*rgions;_tmp195;});*rgions=_tmp79C;});
goto _LL0;}case 11U: _LL5: _tmp192=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp191)->f2;_LL6: {struct Cyc_List_List*ts=_tmp192;
# 723
for(0;ts != 0;ts=ts->tl){
({Cyc_Absynpp_effects2docs(rgions,effects,(void*)ts->hd);});}
# 726
goto _LL0;}default: goto _LL7;}else{_LL7: _LL8:
({struct Cyc_List_List*_tmp79E=({struct Cyc_List_List*_tmp19B=_cycalloc(sizeof(*_tmp19B));({struct Cyc_PP_Doc*_tmp79D=({Cyc_Absynpp_typ2doc(t);});_tmp19B->hd=_tmp79D;}),_tmp19B->tl=*effects;_tmp19B;});*effects=_tmp79E;});goto _LL0;}_LL0:;}
# 710
struct Cyc_PP_Doc*Cyc_Absynpp_eff2doc(void*t){
# 732
struct Cyc_List_List*rgions=0;struct Cyc_List_List*effects=0;
({Cyc_Absynpp_effects2docs(& rgions,& effects,t);});
rgions=({Cyc_List_imp_rev(rgions);});
effects=({Cyc_List_imp_rev(effects);});
if(rgions == 0 && effects != 0)
return({({struct _fat_ptr _tmp7A1=({const char*_tmp19D="";_tag_fat(_tmp19D,sizeof(char),1U);});struct _fat_ptr _tmp7A0=({const char*_tmp19E="";_tag_fat(_tmp19E,sizeof(char),1U);});struct _fat_ptr _tmp79F=({const char*_tmp19F="+";_tag_fat(_tmp19F,sizeof(char),2U);});Cyc_PP_group(_tmp7A1,_tmp7A0,_tmp79F,effects);});});else{
# 739
struct Cyc_PP_Doc*doc1=({({struct _fat_ptr _tmp7A2=({const char*_tmp1A9=",";_tag_fat(_tmp1A9,sizeof(char),2U);});Cyc_Absynpp_group_braces(_tmp7A2,rgions);});});
return({struct Cyc_PP_Doc*(*_tmp1A0)(struct _fat_ptr start,struct _fat_ptr stop,struct _fat_ptr sep,struct Cyc_List_List*l)=Cyc_PP_group;struct _fat_ptr _tmp1A1=({const char*_tmp1A2="";_tag_fat(_tmp1A2,sizeof(char),1U);});struct _fat_ptr _tmp1A3=({const char*_tmp1A4="";_tag_fat(_tmp1A4,sizeof(char),1U);});struct _fat_ptr _tmp1A5=({const char*_tmp1A6="+";_tag_fat(_tmp1A6,sizeof(char),2U);});struct Cyc_List_List*_tmp1A7=({({struct Cyc_List_List*_tmp7A3=effects;Cyc_List_imp_append(_tmp7A3,({struct Cyc_List_List*_tmp1A8=_cycalloc(sizeof(*_tmp1A8));_tmp1A8->hd=doc1,_tmp1A8->tl=0;_tmp1A8;}));});});_tmp1A0(_tmp1A1,_tmp1A3,_tmp1A5,_tmp1A7);});}}
# 710
struct Cyc_PP_Doc*Cyc_Absynpp_aggr_kind2doc(enum Cyc_Absyn_AggrKind k){
# 745
enum Cyc_Absyn_AggrKind _tmp1AB=k;if(_tmp1AB == Cyc_Absyn_StructA){_LL1: _LL2:
 return({Cyc_PP_text(({const char*_tmp1AC="struct ";_tag_fat(_tmp1AC,sizeof(char),8U);}));});}else{_LL3: _LL4:
 return({Cyc_PP_text(({const char*_tmp1AD="union ";_tag_fat(_tmp1AD,sizeof(char),7U);}));});}_LL0:;}
# 710
struct Cyc_PP_Doc*Cyc_Absynpp_ntyp2doc(void*t){
# 753
struct Cyc_PP_Doc*s;
{void*_tmp1AF=t;struct Cyc_Absyn_Datatypedecl*_tmp1B0;struct Cyc_Absyn_Enumdecl*_tmp1B1;struct Cyc_Absyn_Aggrdecl*_tmp1B2;struct Cyc_Absyn_Typedefdecl*_tmp1B5;struct Cyc_List_List*_tmp1B4;struct _tuple0*_tmp1B3;struct Cyc_Absyn_Exp*_tmp1B6;struct Cyc_Absyn_Exp*_tmp1B7;struct Cyc_List_List*_tmp1B9;enum Cyc_Absyn_AggrKind _tmp1B8;struct Cyc_List_List*_tmp1BA;struct Cyc_Absyn_Tvar*_tmp1BB;struct Cyc_Core_Opt*_tmp1BF;int _tmp1BE;void*_tmp1BD;struct Cyc_Core_Opt*_tmp1BC;void*_tmp1C0;struct Cyc_List_List*_tmp1C1;struct Cyc_List_List*_tmp1C2;struct Cyc_List_List*_tmp1C3;struct Cyc_List_List*_tmp1C4;struct _fat_ptr _tmp1C5;struct _tuple0*_tmp1C6;struct Cyc_List_List*_tmp1C7;struct Cyc_List_List*_tmp1C9;union Cyc_Absyn_AggrInfo _tmp1C8;int _tmp1CA;enum Cyc_Absyn_Size_of _tmp1CC;enum Cyc_Absyn_Sign _tmp1CB;struct Cyc_List_List*_tmp1CE;union Cyc_Absyn_DatatypeFieldInfo _tmp1CD;struct Cyc_List_List*_tmp1D0;union Cyc_Absyn_DatatypeInfo _tmp1CF;switch(*((int*)_tmp1AF)){case 4U: _LL1: _LL2:
# 756
 return({Cyc_PP_text(({const char*_tmp1D1="[[[array]]]";_tag_fat(_tmp1D1,sizeof(char),12U);}));});case 5U: _LL3: _LL4:
 return({Cyc_PP_nil_doc();});case 3U: _LL5: _LL6:
 return({Cyc_PP_nil_doc();});case 0U: switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp1AF)->f1)){case 0U: _LL7: _LL8:
# 760
 s=({Cyc_PP_text(({const char*_tmp1D2="void";_tag_fat(_tmp1D2,sizeof(char),5U);}));});goto _LL0;case 20U: _LLD: _tmp1CF=((struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp1AF)->f1)->f1;_tmp1D0=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp1AF)->f2;_LLE: {union Cyc_Absyn_DatatypeInfo tu_info=_tmp1CF;struct Cyc_List_List*ts=_tmp1D0;
# 776
{union Cyc_Absyn_DatatypeInfo _tmp1DC=tu_info;int _tmp1DE;struct _tuple0*_tmp1DD;int _tmp1E0;struct _tuple0*_tmp1DF;if((_tmp1DC.UnknownDatatype).tag == 1){_LL4C: _tmp1DF=((_tmp1DC.UnknownDatatype).val).name;_tmp1E0=((_tmp1DC.UnknownDatatype).val).is_extensible;_LL4D: {struct _tuple0*n=_tmp1DF;int is_x=_tmp1E0;
_tmp1DD=n;_tmp1DE=is_x;goto _LL4F;}}else{_LL4E: _tmp1DD=(*(_tmp1DC.KnownDatatype).val)->name;_tmp1DE=(*(_tmp1DC.KnownDatatype).val)->is_extensible;_LL4F: {struct _tuple0*n=_tmp1DD;int is_x=_tmp1DE;
# 779
struct Cyc_PP_Doc*kw=({Cyc_PP_text(({const char*_tmp1E3="datatype ";_tag_fat(_tmp1E3,sizeof(char),10U);}));});
struct Cyc_PP_Doc*ext=is_x?({Cyc_PP_text(({const char*_tmp1E2="@extensible ";_tag_fat(_tmp1E2,sizeof(char),13U);}));}):({Cyc_PP_nil_doc();});
s=({struct Cyc_PP_Doc*_tmp1E1[4U];_tmp1E1[0]=ext,_tmp1E1[1]=kw,({struct Cyc_PP_Doc*_tmp7A5=({Cyc_Absynpp_qvar2doc(n);});_tmp1E1[2]=_tmp7A5;}),({struct Cyc_PP_Doc*_tmp7A4=({Cyc_Absynpp_tps2doc(ts);});_tmp1E1[3]=_tmp7A4;});Cyc_PP_cat(_tag_fat(_tmp1E1,sizeof(struct Cyc_PP_Doc*),4U));});
goto _LL4B;}}_LL4B:;}
# 784
goto _LL0;}case 21U: _LLF: _tmp1CD=((struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp1AF)->f1)->f1;_tmp1CE=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp1AF)->f2;_LL10: {union Cyc_Absyn_DatatypeFieldInfo tuf_info=_tmp1CD;struct Cyc_List_List*ts=_tmp1CE;
# 786
{union Cyc_Absyn_DatatypeFieldInfo _tmp1E4=tuf_info;struct _tuple0*_tmp1E7;int _tmp1E6;struct _tuple0*_tmp1E5;int _tmp1EA;struct _tuple0*_tmp1E9;struct _tuple0*_tmp1E8;if((_tmp1E4.UnknownDatatypefield).tag == 1){_LL51: _tmp1E8=((_tmp1E4.UnknownDatatypefield).val).datatype_name;_tmp1E9=((_tmp1E4.UnknownDatatypefield).val).field_name;_tmp1EA=((_tmp1E4.UnknownDatatypefield).val).is_extensible;_LL52: {struct _tuple0*tname=_tmp1E8;struct _tuple0*fname=_tmp1E9;int is_x=_tmp1EA;
# 788
_tmp1E5=tname;_tmp1E6=is_x;_tmp1E7=fname;goto _LL54;}}else{_LL53: _tmp1E5=(((_tmp1E4.KnownDatatypefield).val).f1)->name;_tmp1E6=(((_tmp1E4.KnownDatatypefield).val).f1)->is_extensible;_tmp1E7=(((_tmp1E4.KnownDatatypefield).val).f2)->name;_LL54: {struct _tuple0*tname=_tmp1E5;int is_x=_tmp1E6;struct _tuple0*fname=_tmp1E7;
# 791
struct Cyc_PP_Doc*kw=({Cyc_PP_text(is_x?({const char*_tmp1ED="@extensible datatype ";_tag_fat(_tmp1ED,sizeof(char),22U);}):({const char*_tmp1EE="datatype ";_tag_fat(_tmp1EE,sizeof(char),10U);}));});
s=({struct Cyc_PP_Doc*_tmp1EB[4U];_tmp1EB[0]=kw,({struct Cyc_PP_Doc*_tmp7A8=({Cyc_Absynpp_qvar2doc(tname);});_tmp1EB[1]=_tmp7A8;}),({struct Cyc_PP_Doc*_tmp7A7=({Cyc_PP_text(({const char*_tmp1EC=".";_tag_fat(_tmp1EC,sizeof(char),2U);}));});_tmp1EB[2]=_tmp7A7;}),({struct Cyc_PP_Doc*_tmp7A6=({Cyc_Absynpp_qvar2doc(fname);});_tmp1EB[3]=_tmp7A6;});Cyc_PP_cat(_tag_fat(_tmp1EB,sizeof(struct Cyc_PP_Doc*),4U));});
goto _LL50;}}_LL50:;}
# 795
goto _LL0;}case 1U: _LL11: _tmp1CB=((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp1AF)->f1)->f1;_tmp1CC=((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp1AF)->f1)->f2;_LL12: {enum Cyc_Absyn_Sign sn=_tmp1CB;enum Cyc_Absyn_Size_of sz=_tmp1CC;
# 797
struct _fat_ptr sns;
struct _fat_ptr ts;
{enum Cyc_Absyn_Sign _tmp1EF=sn;switch(_tmp1EF){case Cyc_Absyn_None: _LL56: _LL57:
 goto _LL59;case Cyc_Absyn_Signed: _LL58: _LL59:
 sns=({const char*_tmp1F0="";_tag_fat(_tmp1F0,sizeof(char),1U);});goto _LL55;default: _LL5A: _LL5B:
 sns=({const char*_tmp1F1="unsigned ";_tag_fat(_tmp1F1,sizeof(char),10U);});goto _LL55;}_LL55:;}
# 804
{enum Cyc_Absyn_Size_of _tmp1F2=sz;switch(_tmp1F2){case Cyc_Absyn_Char_sz: _LL5D: _LL5E:
# 806
{enum Cyc_Absyn_Sign _tmp1F3=sn;switch(_tmp1F3){case Cyc_Absyn_None: _LL6A: _LL6B:
 sns=({const char*_tmp1F4="";_tag_fat(_tmp1F4,sizeof(char),1U);});goto _LL69;case Cyc_Absyn_Signed: _LL6C: _LL6D:
 sns=({const char*_tmp1F5="signed ";_tag_fat(_tmp1F5,sizeof(char),8U);});goto _LL69;default: _LL6E: _LL6F:
 sns=({const char*_tmp1F6="unsigned ";_tag_fat(_tmp1F6,sizeof(char),10U);});goto _LL69;}_LL69:;}
# 811
ts=({const char*_tmp1F7="char";_tag_fat(_tmp1F7,sizeof(char),5U);});
goto _LL5C;case Cyc_Absyn_Short_sz: _LL5F: _LL60:
 ts=({const char*_tmp1F8="short";_tag_fat(_tmp1F8,sizeof(char),6U);});goto _LL5C;case Cyc_Absyn_Int_sz: _LL61: _LL62:
 ts=({const char*_tmp1F9="int";_tag_fat(_tmp1F9,sizeof(char),4U);});goto _LL5C;case Cyc_Absyn_Long_sz: _LL63: _LL64:
 ts=({const char*_tmp1FA="long";_tag_fat(_tmp1FA,sizeof(char),5U);});goto _LL5C;case Cyc_Absyn_LongLong_sz: _LL65: _LL66:
 goto _LL68;default: _LL67: _LL68:
# 818
{enum Cyc_Cyclone_C_Compilers _tmp1FB=Cyc_Cyclone_c_compiler;if(_tmp1FB == Cyc_Cyclone_Gcc_c){_LL71: _LL72:
 ts=({const char*_tmp1FC="long long";_tag_fat(_tmp1FC,sizeof(char),10U);});goto _LL70;}else{_LL73: _LL74:
 ts=({const char*_tmp1FD="__int64";_tag_fat(_tmp1FD,sizeof(char),8U);});goto _LL70;}_LL70:;}
# 822
goto _LL5C;}_LL5C:;}
# 824
s=({struct Cyc_PP_Doc*(*_tmp1FE)(struct _fat_ptr s)=Cyc_PP_text;struct _fat_ptr _tmp1FF=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp202=({struct Cyc_String_pa_PrintArg_struct _tmp737;_tmp737.tag=0U,_tmp737.f1=(struct _fat_ptr)((struct _fat_ptr)sns);_tmp737;});struct Cyc_String_pa_PrintArg_struct _tmp203=({struct Cyc_String_pa_PrintArg_struct _tmp736;_tmp736.tag=0U,_tmp736.f1=(struct _fat_ptr)((struct _fat_ptr)ts);_tmp736;});void*_tmp200[2U];_tmp200[0]=& _tmp202,_tmp200[1]=& _tmp203;({struct _fat_ptr _tmp7A9=({const char*_tmp201="%s%s";_tag_fat(_tmp201,sizeof(char),5U);});Cyc_aprintf(_tmp7A9,_tag_fat(_tmp200,sizeof(void*),2U));});});_tmp1FE(_tmp1FF);});
goto _LL0;}case 2U: _LL13: _tmp1CA=((struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp1AF)->f1)->f1;_LL14: {int i=_tmp1CA;
# 827
{int _tmp204=i;switch(_tmp204){case 0U: _LL76: _LL77:
 s=({Cyc_PP_text(({const char*_tmp205="float";_tag_fat(_tmp205,sizeof(char),6U);}));});goto _LL75;case 1U: _LL78: _LL79:
 s=({Cyc_PP_text(({const char*_tmp206="double";_tag_fat(_tmp206,sizeof(char),7U);}));});goto _LL75;default: _LL7A: _LL7B:
 s=({Cyc_PP_text(({const char*_tmp207="long double";_tag_fat(_tmp207,sizeof(char),12U);}));});goto _LL75;}_LL75:;}
# 832
goto _LL0;}case 22U: _LL17: _tmp1C8=((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp1AF)->f1)->f1;_tmp1C9=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp1AF)->f2;_LL18: {union Cyc_Absyn_AggrInfo info=_tmp1C8;struct Cyc_List_List*ts=_tmp1C9;
# 835
struct _tuple12 _stmttmp11=({Cyc_Absyn_aggr_kinded_name(info);});struct _tuple0*_tmp20A;enum Cyc_Absyn_AggrKind _tmp209;_LL7D: _tmp209=_stmttmp11.f1;_tmp20A=_stmttmp11.f2;_LL7E: {enum Cyc_Absyn_AggrKind k=_tmp209;struct _tuple0*n=_tmp20A;
s=({struct Cyc_PP_Doc*_tmp20B[3U];({struct Cyc_PP_Doc*_tmp7AC=({Cyc_Absynpp_aggr_kind2doc(k);});_tmp20B[0]=_tmp7AC;}),({struct Cyc_PP_Doc*_tmp7AB=({Cyc_Absynpp_qvar2doc(n);});_tmp20B[1]=_tmp7AB;}),({struct Cyc_PP_Doc*_tmp7AA=({Cyc_Absynpp_tps2doc(ts);});_tmp20B[2]=_tmp7AA;});Cyc_PP_cat(_tag_fat(_tmp20B,sizeof(struct Cyc_PP_Doc*),3U));});
goto _LL0;}}case 18U: _LL1B: _tmp1C7=((struct Cyc_Absyn_AnonEnumCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp1AF)->f1)->f1;_LL1C: {struct Cyc_List_List*fs=_tmp1C7;
# 844
s=({struct Cyc_PP_Doc*_tmp210[4U];({struct Cyc_PP_Doc*_tmp7B0=({Cyc_PP_text(({const char*_tmp211="enum ";_tag_fat(_tmp211,sizeof(char),6U);}));});_tmp210[0]=_tmp7B0;}),({struct Cyc_PP_Doc*_tmp7AF=({Cyc_Absynpp_lb();});_tmp210[1]=_tmp7AF;}),({struct Cyc_PP_Doc*_tmp7AE=({struct Cyc_PP_Doc*(*_tmp212)(int k,struct Cyc_PP_Doc*d)=Cyc_PP_nest;int _tmp213=2;struct Cyc_PP_Doc*_tmp214=({Cyc_Absynpp_enumfields2doc(fs);});_tmp212(_tmp213,_tmp214);});_tmp210[2]=_tmp7AE;}),({struct Cyc_PP_Doc*_tmp7AD=({Cyc_Absynpp_rb();});_tmp210[3]=_tmp7AD;});Cyc_PP_cat(_tag_fat(_tmp210,sizeof(struct Cyc_PP_Doc*),4U));});goto _LL0;}case 17U: _LL1D: _tmp1C6=((struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp1AF)->f1)->f1;_LL1E: {struct _tuple0*n=_tmp1C6;
s=({struct Cyc_PP_Doc*_tmp215[2U];({struct Cyc_PP_Doc*_tmp7B2=({Cyc_PP_text(({const char*_tmp216="enum ";_tag_fat(_tmp216,sizeof(char),6U);}));});_tmp215[0]=_tmp7B2;}),({struct Cyc_PP_Doc*_tmp7B1=({Cyc_Absynpp_qvar2doc(n);});_tmp215[1]=_tmp7B1;});Cyc_PP_cat(_tag_fat(_tmp215,sizeof(struct Cyc_PP_Doc*),2U));});goto _LL0;}case 19U: _LL23: _tmp1C5=((struct Cyc_Absyn_BuiltinCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp1AF)->f1)->f1;_LL24: {struct _fat_ptr t=_tmp1C5;
# 848
s=({Cyc_PP_text(t);});goto _LL0;}case 4U: _LL27: _tmp1C4=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp1AF)->f2;_LL28: {struct Cyc_List_List*ts=_tmp1C4;
# 858
s=({struct Cyc_PP_Doc*_tmp21E[3U];({struct Cyc_PP_Doc*_tmp7B5=({Cyc_PP_text(({const char*_tmp21F="xregion_t<";_tag_fat(_tmp21F,sizeof(char),11U);}));});_tmp21E[0]=_tmp7B5;}),({struct Cyc_PP_Doc*_tmp7B4=({Cyc_Absynpp_rgn2doc((void*)((struct Cyc_List_List*)_check_null(ts))->hd);});_tmp21E[1]=_tmp7B4;}),({struct Cyc_PP_Doc*_tmp7B3=({Cyc_PP_text(({const char*_tmp220=">";_tag_fat(_tmp220,sizeof(char),2U);}));});_tmp21E[2]=_tmp7B3;});Cyc_PP_cat(_tag_fat(_tmp21E,sizeof(struct Cyc_PP_Doc*),3U));});goto _LL0;}case 3U: _LL29: _tmp1C3=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp1AF)->f2;_LL2A: {struct Cyc_List_List*ts=_tmp1C3;
# 861
s=({struct Cyc_PP_Doc*_tmp221[3U];({struct Cyc_PP_Doc*_tmp7B8=({Cyc_PP_text(({const char*_tmp222="region_t<";_tag_fat(_tmp222,sizeof(char),10U);}));});_tmp221[0]=_tmp7B8;}),({struct Cyc_PP_Doc*_tmp7B7=({Cyc_Absynpp_rgn2doc((void*)((struct Cyc_List_List*)_check_null(ts))->hd);});_tmp221[1]=_tmp7B7;}),({struct Cyc_PP_Doc*_tmp7B6=({Cyc_PP_text(({const char*_tmp223=">";_tag_fat(_tmp223,sizeof(char),2U);}));});_tmp221[2]=_tmp7B6;});Cyc_PP_cat(_tag_fat(_tmp221,sizeof(struct Cyc_PP_Doc*),3U));});goto _LL0;}case 5U: _LL2B: _tmp1C2=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp1AF)->f2;_LL2C: {struct Cyc_List_List*ts=_tmp1C2;
# 863
s=({struct Cyc_PP_Doc*_tmp224[3U];({struct Cyc_PP_Doc*_tmp7BB=({Cyc_PP_text(({const char*_tmp225="tag_t<";_tag_fat(_tmp225,sizeof(char),7U);}));});_tmp224[0]=_tmp7BB;}),({struct Cyc_PP_Doc*_tmp7BA=({Cyc_Absynpp_typ2doc((void*)((struct Cyc_List_List*)_check_null(ts))->hd);});_tmp224[1]=_tmp7BA;}),({struct Cyc_PP_Doc*_tmp7B9=({Cyc_PP_text(({const char*_tmp226=">";_tag_fat(_tmp226,sizeof(char),2U);}));});_tmp224[2]=_tmp7B9;});Cyc_PP_cat(_tag_fat(_tmp224,sizeof(struct Cyc_PP_Doc*),3U));});goto _LL0;}case 7U: _LL2D: _LL2E:
 goto _LL30;case 6U: _LL2F: _LL30:
 goto _LL32;case 8U: _LL31: _LL32:
 s=({Cyc_Absynpp_rgn2doc(t);});goto _LL0;case 12U: _LL33: _tmp1C1=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp1AF)->f2;_LL34: {struct Cyc_List_List*ts=_tmp1C1;
# 868
s=({struct Cyc_PP_Doc*_tmp227[3U];({struct Cyc_PP_Doc*_tmp7BE=({Cyc_PP_text(({const char*_tmp228="regions(";_tag_fat(_tmp228,sizeof(char),9U);}));});_tmp227[0]=_tmp7BE;}),({struct Cyc_PP_Doc*_tmp7BD=({Cyc_Absynpp_typ2doc((void*)((struct Cyc_List_List*)_check_null(ts))->hd);});_tmp227[1]=_tmp7BD;}),({struct Cyc_PP_Doc*_tmp7BC=({Cyc_PP_text(({const char*_tmp229=")";_tag_fat(_tmp229,sizeof(char),2U);}));});_tmp227[2]=_tmp7BC;});Cyc_PP_cat(_tag_fat(_tmp227,sizeof(struct Cyc_PP_Doc*),3U));});goto _LL0;}case 9U: _LL35: _LL36:
 goto _LL38;case 10U: _LL37: _LL38:
 goto _LL3A;case 11U: _LL39: _LL3A:
 s=({Cyc_Absynpp_eff2doc(t);});goto _LL0;case 13U: _LL41: _LL42:
# 875
 s=({Cyc_PP_text(({const char*_tmp22A="@true";_tag_fat(_tmp22A,sizeof(char),6U);}));});goto _LL0;case 14U: _LL43: _LL44:
 s=({Cyc_PP_text(({const char*_tmp22B="@false";_tag_fat(_tmp22B,sizeof(char),7U);}));});goto _LL0;case 15U: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp1AF)->f2 != 0){_LL45: _tmp1C0=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp1AF)->f2)->hd;_LL46: {void*t=_tmp1C0;
# 878
s=({struct Cyc_PP_Doc*_tmp22C[4U];({struct Cyc_PP_Doc*_tmp7C2=({Cyc_PP_text(({const char*_tmp22D="@thin @numelts";_tag_fat(_tmp22D,sizeof(char),15U);}));});_tmp22C[0]=_tmp7C2;}),({struct Cyc_PP_Doc*_tmp7C1=({Cyc_Absynpp_lb();});_tmp22C[1]=_tmp7C1;}),({struct Cyc_PP_Doc*_tmp7C0=({Cyc_Absynpp_typ2doc(t);});_tmp22C[2]=_tmp7C0;}),({struct Cyc_PP_Doc*_tmp7BF=({Cyc_Absynpp_rb();});_tmp22C[3]=_tmp7BF;});Cyc_PP_cat(_tag_fat(_tmp22C,sizeof(struct Cyc_PP_Doc*),4U));});goto _LL0;}}else{_LL47: _LL48:
# 880
 s=({Cyc_PP_text(({const char*_tmp22E="@thin";_tag_fat(_tmp22E,sizeof(char),6U);}));});goto _LL0;}default: _LL49: _LL4A:
 s=({Cyc_PP_text(({const char*_tmp22F="@fat";_tag_fat(_tmp22F,sizeof(char),5U);}));});goto _LL0;}case 1U: _LL9: _tmp1BC=((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp1AF)->f1;_tmp1BD=(void*)((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp1AF)->f2;_tmp1BE=((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp1AF)->f3;_tmp1BF=((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp1AF)->f4;_LLA: {struct Cyc_Core_Opt*k=_tmp1BC;void*topt=_tmp1BD;int i=_tmp1BE;struct Cyc_Core_Opt*tvs=_tmp1BF;
# 762
if(topt != 0)
# 764
return({Cyc_Absynpp_ntyp2doc(topt);});else{
# 766
struct _fat_ptr kindstring=k == 0?({const char*_tmp1D9="K";_tag_fat(_tmp1D9,sizeof(char),2U);}):({Cyc_Absynpp_kind2string((struct Cyc_Absyn_Kind*)k->v);});
s=({struct Cyc_PP_Doc*(*_tmp1D3)(struct _fat_ptr s)=Cyc_PP_text;struct _fat_ptr _tmp1D4=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp1D7=({struct Cyc_String_pa_PrintArg_struct _tmp739;_tmp739.tag=0U,_tmp739.f1=(struct _fat_ptr)((struct _fat_ptr)kindstring);_tmp739;});struct Cyc_Int_pa_PrintArg_struct _tmp1D8=({struct Cyc_Int_pa_PrintArg_struct _tmp738;_tmp738.tag=1U,_tmp738.f1=(unsigned long)i;_tmp738;});void*_tmp1D5[2U];_tmp1D5[0]=& _tmp1D7,_tmp1D5[1]=& _tmp1D8;({struct _fat_ptr _tmp7C3=({const char*_tmp1D6="`E%s%d";_tag_fat(_tmp1D6,sizeof(char),7U);});Cyc_aprintf(_tmp7C3,_tag_fat(_tmp1D5,sizeof(void*),2U));});});_tmp1D3(_tmp1D4);});}
# 769
goto _LL0;}case 2U: _LLB: _tmp1BB=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp1AF)->f1;_LLC: {struct Cyc_Absyn_Tvar*tv=_tmp1BB;
# 771
s=({Cyc_Absynpp_tvar2doc(tv);});
if(Cyc_Absynpp_print_all_kinds)
s=({struct Cyc_PP_Doc*_tmp1DA[3U];_tmp1DA[0]=s,({struct Cyc_PP_Doc*_tmp7C5=({Cyc_PP_text(({const char*_tmp1DB="::";_tag_fat(_tmp1DB,sizeof(char),3U);}));});_tmp1DA[1]=_tmp7C5;}),({struct Cyc_PP_Doc*_tmp7C4=({Cyc_Absynpp_kindbound2doc(tv->kind);});_tmp1DA[2]=_tmp7C4;});Cyc_PP_cat(_tag_fat(_tmp1DA,sizeof(struct Cyc_PP_Doc*),3U));});
# 772
goto _LL0;}case 6U: _LL15: _tmp1BA=((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp1AF)->f1;_LL16: {struct Cyc_List_List*ts=_tmp1BA;
# 833
s=({struct Cyc_PP_Doc*_tmp208[2U];({struct Cyc_PP_Doc*_tmp7C7=({Cyc_Absynpp_dollar();});_tmp208[0]=_tmp7C7;}),({struct Cyc_PP_Doc*_tmp7C6=({Cyc_Absynpp_args2doc(ts);});_tmp208[1]=_tmp7C6;});Cyc_PP_cat(_tag_fat(_tmp208,sizeof(struct Cyc_PP_Doc*),2U));});goto _LL0;}case 7U: _LL19: _tmp1B8=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp1AF)->f1;_tmp1B9=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp1AF)->f2;_LL1A: {enum Cyc_Absyn_AggrKind k=_tmp1B8;struct Cyc_List_List*fs=_tmp1B9;
# 839
s=({struct Cyc_PP_Doc*_tmp20C[4U];({struct Cyc_PP_Doc*_tmp7CB=({Cyc_Absynpp_aggr_kind2doc(k);});_tmp20C[0]=_tmp7CB;}),({struct Cyc_PP_Doc*_tmp7CA=({Cyc_Absynpp_lb();});_tmp20C[1]=_tmp7CA;}),({
struct Cyc_PP_Doc*_tmp7C9=({struct Cyc_PP_Doc*(*_tmp20D)(int k,struct Cyc_PP_Doc*d)=Cyc_PP_nest;int _tmp20E=2;struct Cyc_PP_Doc*_tmp20F=({Cyc_Absynpp_aggrfields2doc(fs);});_tmp20D(_tmp20E,_tmp20F);});_tmp20C[2]=_tmp7C9;}),({
struct Cyc_PP_Doc*_tmp7C8=({Cyc_Absynpp_rb();});_tmp20C[3]=_tmp7C8;});Cyc_PP_cat(_tag_fat(_tmp20C,sizeof(struct Cyc_PP_Doc*),4U));});
goto _LL0;}case 9U: _LL1F: _tmp1B7=((struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_tmp1AF)->f1;_LL20: {struct Cyc_Absyn_Exp*e=_tmp1B7;
# 846
s=({struct Cyc_PP_Doc*_tmp217[3U];({struct Cyc_PP_Doc*_tmp7CE=({Cyc_PP_text(({const char*_tmp218="valueof_t(";_tag_fat(_tmp218,sizeof(char),11U);}));});_tmp217[0]=_tmp7CE;}),({struct Cyc_PP_Doc*_tmp7CD=({Cyc_Absynpp_exp2doc(e);});_tmp217[1]=_tmp7CD;}),({struct Cyc_PP_Doc*_tmp7CC=({Cyc_PP_text(({const char*_tmp219=")";_tag_fat(_tmp219,sizeof(char),2U);}));});_tmp217[2]=_tmp7CC;});Cyc_PP_cat(_tag_fat(_tmp217,sizeof(struct Cyc_PP_Doc*),3U));});goto _LL0;}case 11U: _LL21: _tmp1B6=((struct Cyc_Absyn_TypeofType_Absyn_Type_struct*)_tmp1AF)->f1;_LL22: {struct Cyc_Absyn_Exp*e=_tmp1B6;
s=({struct Cyc_PP_Doc*_tmp21A[3U];({struct Cyc_PP_Doc*_tmp7D1=({Cyc_PP_text(({const char*_tmp21B="typeof(";_tag_fat(_tmp21B,sizeof(char),8U);}));});_tmp21A[0]=_tmp7D1;}),({struct Cyc_PP_Doc*_tmp7D0=({Cyc_Absynpp_exp2doc(e);});_tmp21A[1]=_tmp7D0;}),({struct Cyc_PP_Doc*_tmp7CF=({Cyc_PP_text(({const char*_tmp21C=")";_tag_fat(_tmp21C,sizeof(char),2U);}));});_tmp21A[2]=_tmp7CF;});Cyc_PP_cat(_tag_fat(_tmp21A,sizeof(struct Cyc_PP_Doc*),3U));});goto _LL0;}case 8U: _LL25: _tmp1B3=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp1AF)->f1;_tmp1B4=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp1AF)->f2;_tmp1B5=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp1AF)->f3;_LL26: {struct _tuple0*n=_tmp1B3;struct Cyc_List_List*ts=_tmp1B4;struct Cyc_Absyn_Typedefdecl*kopt=_tmp1B5;
# 855
s=({struct Cyc_PP_Doc*_tmp21D[2U];({struct Cyc_PP_Doc*_tmp7D3=({Cyc_Absynpp_qvar2doc(n);});_tmp21D[0]=_tmp7D3;}),({struct Cyc_PP_Doc*_tmp7D2=({Cyc_Absynpp_tps2doc(ts);});_tmp21D[1]=_tmp7D2;});Cyc_PP_cat(_tag_fat(_tmp21D,sizeof(struct Cyc_PP_Doc*),2U));});
goto _LL0;}default: switch(*((int*)((struct Cyc_Absyn_TypeDecl*)((struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_tmp1AF)->f1)->r)){case 0U: _LL3B: _tmp1B2=((struct Cyc_Absyn_Aggr_td_Absyn_Raw_typedecl_struct*)(((struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_tmp1AF)->f1)->r)->f1;_LL3C: {struct Cyc_Absyn_Aggrdecl*d=_tmp1B2;
# 872
s=({Cyc_Absynpp_aggrdecl2doc(d);});goto _LL0;}case 1U: _LL3D: _tmp1B1=((struct Cyc_Absyn_Enum_td_Absyn_Raw_typedecl_struct*)(((struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_tmp1AF)->f1)->r)->f1;_LL3E: {struct Cyc_Absyn_Enumdecl*d=_tmp1B1;
s=({Cyc_Absynpp_enumdecl2doc(d);});goto _LL0;}default: _LL3F: _tmp1B0=((struct Cyc_Absyn_Datatype_td_Absyn_Raw_typedecl_struct*)(((struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_tmp1AF)->f1)->r)->f1;_LL40: {struct Cyc_Absyn_Datatypedecl*d=_tmp1B0;
s=({Cyc_Absynpp_datatypedecl2doc(d);});goto _LL0;}}}_LL0:;}
# 883
return s;}
# 710
struct Cyc_PP_Doc*Cyc_Absynpp_vo2doc(struct _fat_ptr*vo){
# 887
return vo == 0?({Cyc_PP_nil_doc();}):({Cyc_PP_text(*vo);});}struct _tuple19{void*f1;void*f2;};
# 710
struct Cyc_PP_Doc*Cyc_Absynpp_rgn_cmp2doc(struct _tuple19*cmp){
# 891
void*_tmp233;void*_tmp232;_LL1: _tmp232=cmp->f1;_tmp233=cmp->f2;_LL2: {void*r1=_tmp232;void*r2=_tmp233;
return({struct Cyc_PP_Doc*_tmp234[3U];({struct Cyc_PP_Doc*_tmp7D6=({Cyc_Absynpp_rgn2doc(r1);});_tmp234[0]=_tmp7D6;}),({struct Cyc_PP_Doc*_tmp7D5=({Cyc_PP_text(({const char*_tmp235=" > ";_tag_fat(_tmp235,sizeof(char),4U);}));});_tmp234[1]=_tmp7D5;}),({struct Cyc_PP_Doc*_tmp7D4=({Cyc_Absynpp_rgn2doc(r2);});_tmp234[2]=_tmp7D4;});Cyc_PP_cat(_tag_fat(_tmp234,sizeof(struct Cyc_PP_Doc*),3U));});}}
# 710
struct Cyc_PP_Doc*Cyc_Absynpp_rgnpo2doc(struct Cyc_List_List*po){
# 896
return({struct Cyc_PP_Doc*(*_tmp237)(struct _fat_ptr start,struct _fat_ptr stop,struct _fat_ptr sep,struct Cyc_List_List*l)=Cyc_PP_group;struct _fat_ptr _tmp238=({const char*_tmp239="";_tag_fat(_tmp239,sizeof(char),1U);});struct _fat_ptr _tmp23A=({const char*_tmp23B="";_tag_fat(_tmp23B,sizeof(char),1U);});struct _fat_ptr _tmp23C=({const char*_tmp23D=",";_tag_fat(_tmp23D,sizeof(char),2U);});struct Cyc_List_List*_tmp23E=({({struct Cyc_List_List*(*_tmp7D7)(struct Cyc_PP_Doc*(*f)(struct _tuple19*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp23F)(struct Cyc_PP_Doc*(*f)(struct _tuple19*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_PP_Doc*(*f)(struct _tuple19*),struct Cyc_List_List*x))Cyc_List_map;_tmp23F;});_tmp7D7(Cyc_Absynpp_rgn_cmp2doc,po);});});_tmp237(_tmp238,_tmp23A,_tmp23C,_tmp23E);});}
# 710
struct Cyc_PP_Doc*Cyc_Absynpp_funarg2doc(struct _tuple9*t){
# 900
struct _fat_ptr*vo=(*t).f1;
struct Cyc_Core_Opt*dopt=vo == 0?0:({struct Cyc_Core_Opt*_tmp241=_cycalloc(sizeof(*_tmp241));({struct Cyc_PP_Doc*_tmp7D8=({Cyc_PP_text(*vo);});_tmp241->v=_tmp7D8;});_tmp241;});
return({Cyc_Absynpp_tqtd2doc((*t).f2,(*t).f3,dopt);});}
# 905
static struct Cyc_PP_Doc*Cyc_Absynpp_efft2doc(struct Cyc_List_List*ieff,int b){
# 907
return({struct Cyc_PP_Doc*_tmp243[3U];b?({struct Cyc_PP_Doc*_tmp7DC=({Cyc_PP_text(({const char*_tmp244=" @ieffect(";_tag_fat(_tmp244,sizeof(char),11U);}));});_tmp243[0]=_tmp7DC;}):({struct Cyc_PP_Doc*_tmp7DB=({Cyc_PP_text(({const char*_tmp245="@oeffect(";_tag_fat(_tmp245,sizeof(char),10U);}));});_tmp243[0]=_tmp7DB;}),({
struct Cyc_PP_Doc*_tmp7DA=({struct Cyc_PP_Doc*(*_tmp246)(struct _fat_ptr s)=Cyc_PP_text;struct _fat_ptr _tmp247=({Cyc_Absyn_effect2string(ieff);});_tmp246(_tmp247);});_tmp243[1]=_tmp7DA;}),({struct Cyc_PP_Doc*_tmp7D9=({Cyc_PP_text(({const char*_tmp248=")";_tag_fat(_tmp248,sizeof(char),2U);}));});_tmp243[2]=_tmp7D9;});Cyc_PP_cat(_tag_fat(_tmp243,sizeof(struct Cyc_PP_Doc*),3U));});}
# 905
struct Cyc_PP_Doc*Cyc_Absynpp_datatypefield2doc(struct Cyc_Absyn_Datatypefield*f);
# 911
static struct Cyc_PP_Doc*Cyc_Absynpp_throws2doc(struct Cyc_List_List*t){
# 913
if(({Cyc_Absyn_is_throwsany(t);}))return({Cyc_PP_text(({const char*_tmp24A=" @throwsany";_tag_fat(_tmp24A,sizeof(char),12U);}));});else{
if(({Cyc_Absyn_is_nothrow(t);}))return({Cyc_PP_text(({const char*_tmp24B=" @nothrow";_tag_fat(_tmp24B,sizeof(char),10U);}));});else{
# 917
struct Cyc_PP_Doc*p=({struct Cyc_PP_Doc*_tmp250[2U];({
struct Cyc_PP_Doc*_tmp7DE=({Cyc_PP_text(({const char*_tmp251=" @throws(";_tag_fat(_tmp251,sizeof(char),10U);}));});_tmp250[0]=_tmp7DE;}),({struct Cyc_PP_Doc*_tmp7DD=({Cyc_Absynpp_datatypefield2doc((struct Cyc_Absyn_Datatypefield*)((struct Cyc_List_List*)_check_null(t))->hd);});_tmp250[1]=_tmp7DD;});Cyc_PP_cat(_tag_fat(_tmp250,sizeof(struct Cyc_PP_Doc*),2U));});
for(t=t->tl;t != 0;t=t->tl){
p=({struct Cyc_PP_Doc*_tmp24C[2U];({struct Cyc_PP_Doc*_tmp7E0=({Cyc_PP_text(({const char*_tmp24D=",";_tag_fat(_tmp24D,sizeof(char),2U);}));});_tmp24C[0]=_tmp7E0;}),({struct Cyc_PP_Doc*_tmp7DF=({Cyc_Absynpp_datatypefield2doc((struct Cyc_Absyn_Datatypefield*)t->hd);});_tmp24C[1]=_tmp7DF;});Cyc_PP_cat(_tag_fat(_tmp24C,sizeof(struct Cyc_PP_Doc*),2U));});}
return({struct Cyc_PP_Doc*_tmp24E[2U];_tmp24E[0]=p,({struct Cyc_PP_Doc*_tmp7E1=({Cyc_PP_text(({const char*_tmp24F=")";_tag_fat(_tmp24F,sizeof(char),2U);}));});_tmp24E[1]=_tmp7E1;});Cyc_PP_cat(_tag_fat(_tmp24E,sizeof(struct Cyc_PP_Doc*),2U));});}}}
# 911
struct Cyc_PP_Doc*Cyc_Absynpp_funargs2doc(struct Cyc_List_List*args,int c_varargs,struct Cyc_Absyn_VarargInfo*cyc_varargs,void*effopt,struct Cyc_List_List*rgn_po,struct Cyc_Absyn_Exp*req,struct Cyc_Absyn_Exp*ens,struct Cyc_List_List*ieff,struct Cyc_List_List*oeff,struct Cyc_List_List*throws,int reentrant){
# 935
struct Cyc_List_List*arg_docs=({({struct Cyc_List_List*(*_tmp7E2)(struct Cyc_PP_Doc*(*f)(struct _tuple9*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp273)(struct Cyc_PP_Doc*(*f)(struct _tuple9*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_PP_Doc*(*f)(struct _tuple9*),struct Cyc_List_List*x))Cyc_List_map;_tmp273;});_tmp7E2(Cyc_Absynpp_funarg2doc,args);});});
struct Cyc_PP_Doc*eff_doc;
if(c_varargs)
arg_docs=({struct Cyc_List_List*(*_tmp253)(struct Cyc_List_List*x,struct Cyc_List_List*y)=Cyc_List_append;struct Cyc_List_List*_tmp254=arg_docs;struct Cyc_List_List*_tmp255=({struct Cyc_List_List*_tmp257=_cycalloc(sizeof(*_tmp257));({struct Cyc_PP_Doc*_tmp7E3=({Cyc_PP_text(({const char*_tmp256="...";_tag_fat(_tmp256,sizeof(char),4U);}));});_tmp257->hd=_tmp7E3;}),_tmp257->tl=0;_tmp257;});_tmp253(_tmp254,_tmp255);});else{
if(cyc_varargs != 0){
struct Cyc_PP_Doc*varargs_doc=({struct Cyc_PP_Doc*_tmp259[3U];({struct Cyc_PP_Doc*_tmp7E7=({Cyc_PP_text(({const char*_tmp25A="...";_tag_fat(_tmp25A,sizeof(char),4U);}));});_tmp259[0]=_tmp7E7;}),
cyc_varargs->inject?({struct Cyc_PP_Doc*_tmp7E6=({Cyc_PP_text(({const char*_tmp25B=" inject ";_tag_fat(_tmp25B,sizeof(char),9U);}));});_tmp259[1]=_tmp7E6;}):({struct Cyc_PP_Doc*_tmp7E5=({Cyc_PP_text(({const char*_tmp25C=" ";_tag_fat(_tmp25C,sizeof(char),2U);}));});_tmp259[1]=_tmp7E5;}),({
struct Cyc_PP_Doc*_tmp7E4=({Cyc_Absynpp_funarg2doc(({struct _tuple9*_tmp25D=_cycalloc(sizeof(*_tmp25D));_tmp25D->f1=cyc_varargs->name,_tmp25D->f2=cyc_varargs->tq,_tmp25D->f3=cyc_varargs->type;_tmp25D;}));});_tmp259[2]=_tmp7E4;});Cyc_PP_cat(_tag_fat(_tmp259,sizeof(struct Cyc_PP_Doc*),3U));});
# 944
arg_docs=({({struct Cyc_List_List*_tmp7E8=arg_docs;Cyc_List_append(_tmp7E8,({struct Cyc_List_List*_tmp258=_cycalloc(sizeof(*_tmp258));_tmp258->hd=varargs_doc,_tmp258->tl=0;_tmp258;}));});});}}{
# 937
struct Cyc_PP_Doc*arg_doc=({({struct _fat_ptr _tmp7EB=({const char*_tmp270="";_tag_fat(_tmp270,sizeof(char),1U);});struct _fat_ptr _tmp7EA=({const char*_tmp271="";_tag_fat(_tmp271,sizeof(char),1U);});struct _fat_ptr _tmp7E9=({const char*_tmp272=",";_tag_fat(_tmp272,sizeof(char),2U);});Cyc_PP_group(_tmp7EB,_tmp7EA,_tmp7E9,arg_docs);});});
# 947
if((effopt != 0 && Cyc_Absynpp_print_all_effects)&& !Cyc_Absynpp_qvar_to_Cids)
arg_doc=({struct Cyc_PP_Doc*_tmp25E[3U];_tmp25E[0]=arg_doc,({struct Cyc_PP_Doc*_tmp7ED=({Cyc_PP_text(({const char*_tmp25F=";";_tag_fat(_tmp25F,sizeof(char),2U);}));});_tmp25E[1]=_tmp7ED;}),({struct Cyc_PP_Doc*_tmp7EC=({Cyc_Absynpp_eff2doc(effopt);});_tmp25E[2]=_tmp7EC;});Cyc_PP_cat(_tag_fat(_tmp25E,sizeof(struct Cyc_PP_Doc*),3U));});
# 947
if(
# 949
rgn_po != 0 && !Cyc_Absynpp_qvar_to_Cids)
arg_doc=({struct Cyc_PP_Doc*_tmp260[3U];_tmp260[0]=arg_doc,({struct Cyc_PP_Doc*_tmp7EF=({Cyc_PP_text(({const char*_tmp261=":";_tag_fat(_tmp261,sizeof(char),2U);}));});_tmp260[1]=_tmp7EF;}),({struct Cyc_PP_Doc*_tmp7EE=({Cyc_Absynpp_rgnpo2doc(rgn_po);});_tmp260[2]=_tmp7EE;});Cyc_PP_cat(_tag_fat(_tmp260,sizeof(struct Cyc_PP_Doc*),3U));});{
# 947
struct Cyc_PP_Doc*res=({struct Cyc_PP_Doc*_tmp26D[3U];({
# 951
struct Cyc_PP_Doc*_tmp7F1=({Cyc_PP_text(({const char*_tmp26E="(";_tag_fat(_tmp26E,sizeof(char),2U);}));});_tmp26D[0]=_tmp7F1;}),_tmp26D[1]=arg_doc,({struct Cyc_PP_Doc*_tmp7F0=({Cyc_PP_text(({const char*_tmp26F=")";_tag_fat(_tmp26F,sizeof(char),2U);}));});_tmp26D[2]=_tmp7F0;});Cyc_PP_cat(_tag_fat(_tmp26D,sizeof(struct Cyc_PP_Doc*),3U));});
if(req != 0 && !Cyc_Absynpp_qvar_to_Cids)
res=({struct Cyc_PP_Doc*_tmp262[4U];_tmp262[0]=res,({struct Cyc_PP_Doc*_tmp7F4=({Cyc_PP_text(({const char*_tmp263=" @requires(";_tag_fat(_tmp263,sizeof(char),12U);}));});_tmp262[1]=_tmp7F4;}),({struct Cyc_PP_Doc*_tmp7F3=({Cyc_Absynpp_exp2doc(req);});_tmp262[2]=_tmp7F3;}),({struct Cyc_PP_Doc*_tmp7F2=({Cyc_PP_text(({const char*_tmp264=")";_tag_fat(_tmp264,sizeof(char),2U);}));});_tmp262[3]=_tmp7F2;});Cyc_PP_cat(_tag_fat(_tmp262,sizeof(struct Cyc_PP_Doc*),4U));});
# 952
if(
# 954
ens != 0 && !Cyc_Absynpp_qvar_to_Cids)
res=({struct Cyc_PP_Doc*_tmp265[4U];_tmp265[0]=res,({struct Cyc_PP_Doc*_tmp7F7=({Cyc_PP_text(({const char*_tmp266=" @ensures(";_tag_fat(_tmp266,sizeof(char),11U);}));});_tmp265[1]=_tmp7F7;}),({struct Cyc_PP_Doc*_tmp7F6=({Cyc_Absynpp_exp2doc(ens);});_tmp265[2]=_tmp7F6;}),({struct Cyc_PP_Doc*_tmp7F5=({Cyc_PP_text(({const char*_tmp267=")";_tag_fat(_tmp267,sizeof(char),2U);}));});_tmp265[3]=_tmp7F5;});Cyc_PP_cat(_tag_fat(_tmp265,sizeof(struct Cyc_PP_Doc*),4U));});
# 952
if(
# 957
ieff != 0 && !Cyc_Absynpp_qvar_to_Cids)
# 959
res=({struct Cyc_PP_Doc*_tmp268[2U];_tmp268[0]=res,({struct Cyc_PP_Doc*_tmp7F8=({Cyc_Absynpp_efft2doc(ieff,1);});_tmp268[1]=_tmp7F8;});Cyc_PP_cat(_tag_fat(_tmp268,sizeof(struct Cyc_PP_Doc*),2U));});
# 952
if(
# 962
oeff != 0 && !Cyc_Absynpp_qvar_to_Cids)
# 964
res=({struct Cyc_PP_Doc*_tmp269[2U];_tmp269[0]=res,({struct Cyc_PP_Doc*_tmp7F9=({Cyc_Absynpp_efft2doc(oeff,0);});_tmp269[1]=_tmp7F9;});Cyc_PP_cat(_tag_fat(_tmp269,sizeof(struct Cyc_PP_Doc*),2U));});
# 952
if(
# 967
throws != 0 && !Cyc_Absynpp_qvar_to_Cids)
# 969
res=({struct Cyc_PP_Doc*_tmp26A[2U];_tmp26A[0]=res,({struct Cyc_PP_Doc*_tmp7FA=({Cyc_Absynpp_throws2doc(throws);});_tmp26A[1]=_tmp7FA;});Cyc_PP_cat(_tag_fat(_tmp26A,sizeof(struct Cyc_PP_Doc*),2U));});
# 952
if(
# 972
({Cyc_Absyn_is_reentrant(reentrant);})&& !Cyc_Absynpp_qvar_to_Cids)
res=({struct Cyc_PP_Doc*_tmp26B[2U];_tmp26B[0]=res,({struct Cyc_PP_Doc*_tmp7FB=({Cyc_PP_text(({const char*_tmp26C=" @re_entrant";_tag_fat(_tmp26C,sizeof(char),13U);}));});_tmp26B[1]=_tmp7FB;});Cyc_PP_cat(_tag_fat(_tmp26B,sizeof(struct Cyc_PP_Doc*),2U));});
# 952
return res;}}}
# 911
struct Cyc_PP_Doc*Cyc_Absynpp_var2doc(struct _fat_ptr*v){
# 977
return({Cyc_PP_text(*v);});}
# 911
struct _fat_ptr Cyc_Absynpp_qvar2string(struct _tuple0*q){
# 980
struct Cyc_List_List*prefix=0;
int match;
{union Cyc_Absyn_Nmspace _stmttmp12=(*q).f1;union Cyc_Absyn_Nmspace _tmp276=_stmttmp12;struct Cyc_List_List*_tmp277;struct Cyc_List_List*_tmp278;struct Cyc_List_List*_tmp279;switch((_tmp276.C_n).tag){case 4U: _LL1: _LL2:
 _tmp279=0;goto _LL4;case 1U: _LL3: _tmp279=(_tmp276.Rel_n).val;_LL4: {struct Cyc_List_List*x=_tmp279;
# 985
match=0;
prefix=x;
goto _LL0;}case 3U: _LL5: _tmp278=(_tmp276.C_n).val;_LL6: {struct Cyc_List_List*x=_tmp278;
# 989
match=Cyc_Absynpp_use_curr_namespace &&({({int(*_tmp7FD)(int(*cmp)(struct _fat_ptr*,struct _fat_ptr*),struct Cyc_List_List*l1,struct Cyc_List_List*l2)=({int(*_tmp27A)(int(*cmp)(struct _fat_ptr*,struct _fat_ptr*),struct Cyc_List_List*l1,struct Cyc_List_List*l2)=(int(*)(int(*cmp)(struct _fat_ptr*,struct _fat_ptr*),struct Cyc_List_List*l1,struct Cyc_List_List*l2))Cyc_List_list_prefix;_tmp27A;});struct Cyc_List_List*_tmp7FC=x;_tmp7FD(Cyc_strptrcmp,_tmp7FC,Cyc_Absynpp_curr_namespace);});});
# 991
goto _LL0;}default: _LL7: _tmp277=(_tmp276.Abs_n).val;_LL8: {struct Cyc_List_List*x=_tmp277;
# 993
match=Cyc_Absynpp_use_curr_namespace &&({({int(*_tmp7FF)(int(*cmp)(struct _fat_ptr*,struct _fat_ptr*),struct Cyc_List_List*l1,struct Cyc_List_List*l2)=({int(*_tmp27B)(int(*cmp)(struct _fat_ptr*,struct _fat_ptr*),struct Cyc_List_List*l1,struct Cyc_List_List*l2)=(int(*)(int(*cmp)(struct _fat_ptr*,struct _fat_ptr*),struct Cyc_List_List*l1,struct Cyc_List_List*l2))Cyc_List_list_prefix;_tmp27B;});struct Cyc_List_List*_tmp7FE=x;_tmp7FF(Cyc_strptrcmp,_tmp7FE,Cyc_Absynpp_curr_namespace);});});
prefix=Cyc_Absynpp_qvar_to_Cids && Cyc_Absynpp_add_cyc_prefix?({struct Cyc_List_List*_tmp27C=_cycalloc(sizeof(*_tmp27C));_tmp27C->hd=Cyc_Absynpp_cyc_stringptr,_tmp27C->tl=x;_tmp27C;}): x;
goto _LL0;}}_LL0:;}
# 997
if(Cyc_Absynpp_qvar_to_Cids)
return(struct _fat_ptr)({struct _fat_ptr(*_tmp27D)(struct Cyc_List_List*,struct _fat_ptr)=Cyc_str_sepstr;struct Cyc_List_List*_tmp27E=({({struct Cyc_List_List*_tmp800=prefix;Cyc_List_append(_tmp800,({struct Cyc_List_List*_tmp27F=_cycalloc(sizeof(*_tmp27F));_tmp27F->hd=(*q).f2,_tmp27F->tl=0;_tmp27F;}));});});struct _fat_ptr _tmp280=({const char*_tmp281="_";_tag_fat(_tmp281,sizeof(char),2U);});_tmp27D(_tmp27E,_tmp280);});else{
# 1002
if(match)
return*(*q).f2;else{
# 1005
return(struct _fat_ptr)({struct _fat_ptr(*_tmp282)(struct Cyc_List_List*,struct _fat_ptr)=Cyc_str_sepstr;struct Cyc_List_List*_tmp283=({({struct Cyc_List_List*_tmp801=prefix;Cyc_List_append(_tmp801,({struct Cyc_List_List*_tmp284=_cycalloc(sizeof(*_tmp284));_tmp284->hd=(*q).f2,_tmp284->tl=0;_tmp284;}));});});struct _fat_ptr _tmp285=({const char*_tmp286="::";_tag_fat(_tmp286,sizeof(char),3U);});_tmp282(_tmp283,_tmp285);});}}}
# 911
struct Cyc_PP_Doc*Cyc_Absynpp_qvar2doc(struct _tuple0*q){
# 1010
return({struct Cyc_PP_Doc*(*_tmp288)(struct _fat_ptr s)=Cyc_PP_text;struct _fat_ptr _tmp289=({Cyc_Absynpp_qvar2string(q);});_tmp288(_tmp289);});}
# 911
struct Cyc_PP_Doc*Cyc_Absynpp_qvar2bolddoc(struct _tuple0*q){
# 1014
struct _fat_ptr qs=({Cyc_Absynpp_qvar2string(q);});
if(Cyc_PP_tex_output)
# 1017
return({struct Cyc_PP_Doc*(*_tmp28B)(struct _fat_ptr s,int w)=Cyc_PP_text_width;struct _fat_ptr _tmp28C=(struct _fat_ptr)({struct _fat_ptr(*_tmp28D)(struct _fat_ptr,struct _fat_ptr)=Cyc_strconcat;struct _fat_ptr _tmp28E=(struct _fat_ptr)({({struct _fat_ptr _tmp802=({const char*_tmp28F="\\textbf{";_tag_fat(_tmp28F,sizeof(char),9U);});Cyc_strconcat(_tmp802,(struct _fat_ptr)qs);});});struct _fat_ptr _tmp290=({const char*_tmp291="}";_tag_fat(_tmp291,sizeof(char),2U);});_tmp28D(_tmp28E,_tmp290);});int _tmp292=(int)({Cyc_strlen((struct _fat_ptr)qs);});_tmp28B(_tmp28C,_tmp292);});else{
# 1019
return({Cyc_PP_text(qs);});}}
# 911
struct _fat_ptr Cyc_Absynpp_typedef_name2string(struct _tuple0*v){
# 1024
if(Cyc_Absynpp_qvar_to_Cids)return({Cyc_Absynpp_qvar2string(v);});if(Cyc_Absynpp_use_curr_namespace){
# 1028
union Cyc_Absyn_Nmspace _stmttmp13=(*v).f1;union Cyc_Absyn_Nmspace _tmp294=_stmttmp13;struct Cyc_List_List*_tmp295;struct Cyc_List_List*_tmp296;switch((_tmp294.C_n).tag){case 4U: _LL1: _LL2:
 goto _LL4;case 1U: if((_tmp294.Rel_n).val == 0){_LL3: _LL4:
 return*(*v).f2;}else{_LL9: _LLA:
# 1038
 return(struct _fat_ptr)({struct _fat_ptr(*_tmp298)(struct _fat_ptr,struct _fat_ptr)=Cyc_strconcat;struct _fat_ptr _tmp299=({const char*_tmp29A="/* bad namespace : */ ";_tag_fat(_tmp29A,sizeof(char),23U);});struct _fat_ptr _tmp29B=(struct _fat_ptr)({Cyc_Absynpp_qvar2string(v);});_tmp298(_tmp299,_tmp29B);});}case 3U: _LL5: _tmp296=(_tmp294.C_n).val;_LL6: {struct Cyc_List_List*l=_tmp296;
# 1031
_tmp295=l;goto _LL8;}default: _LL7: _tmp295=(_tmp294.Abs_n).val;_LL8: {struct Cyc_List_List*l=_tmp295;
# 1033
if(({({int(*_tmp804)(int(*cmp)(struct _fat_ptr*,struct _fat_ptr*),struct Cyc_List_List*l1,struct Cyc_List_List*l2)=({int(*_tmp297)(int(*cmp)(struct _fat_ptr*,struct _fat_ptr*),struct Cyc_List_List*l1,struct Cyc_List_List*l2)=(int(*)(int(*cmp)(struct _fat_ptr*,struct _fat_ptr*),struct Cyc_List_List*l1,struct Cyc_List_List*l2))Cyc_List_list_cmp;_tmp297;});struct Cyc_List_List*_tmp803=l;_tmp804(Cyc_strptrcmp,_tmp803,Cyc_Absynpp_curr_namespace);});})== 0)
return*(*v).f2;else{
# 1036
goto _LLA;}}}_LL0:;}else{
# 1041
return*(*v).f2;}}
# 911
struct Cyc_PP_Doc*Cyc_Absynpp_typedef_name2doc(struct _tuple0*v){
# 1045
return({struct Cyc_PP_Doc*(*_tmp29D)(struct _fat_ptr s)=Cyc_PP_text;struct _fat_ptr _tmp29E=({Cyc_Absynpp_typedef_name2string(v);});_tmp29D(_tmp29E);});}
# 911
struct Cyc_PP_Doc*Cyc_Absynpp_typedef_name2bolddoc(struct _tuple0*v){
# 1048
struct _fat_ptr vs=({Cyc_Absynpp_typedef_name2string(v);});
if(Cyc_PP_tex_output)
# 1051
return({struct Cyc_PP_Doc*(*_tmp2A0)(struct _fat_ptr s,int w)=Cyc_PP_text_width;struct _fat_ptr _tmp2A1=(struct _fat_ptr)({struct _fat_ptr(*_tmp2A2)(struct _fat_ptr,struct _fat_ptr)=Cyc_strconcat;struct _fat_ptr _tmp2A3=(struct _fat_ptr)({({struct _fat_ptr _tmp805=({const char*_tmp2A4="\\textbf{";_tag_fat(_tmp2A4,sizeof(char),9U);});Cyc_strconcat(_tmp805,(struct _fat_ptr)vs);});});struct _fat_ptr _tmp2A5=({const char*_tmp2A6="}";_tag_fat(_tmp2A6,sizeof(char),2U);});_tmp2A2(_tmp2A3,_tmp2A5);});int _tmp2A7=(int)({Cyc_strlen((struct _fat_ptr)vs);});_tmp2A0(_tmp2A1,_tmp2A7);});else{
# 1053
return({Cyc_PP_text(vs);});}}
# 911
struct Cyc_PP_Doc*Cyc_Absynpp_typ2doc(void*t){
# 1057
return({struct Cyc_PP_Doc*(*_tmp2A9)(struct Cyc_Absyn_Tqual tq,void*t,struct Cyc_Core_Opt*dopt)=Cyc_Absynpp_tqtd2doc;struct Cyc_Absyn_Tqual _tmp2AA=({Cyc_Absyn_empty_tqual(0U);});void*_tmp2AB=t;struct Cyc_Core_Opt*_tmp2AC=0;_tmp2A9(_tmp2AA,_tmp2AB,_tmp2AC);});}
# 1060
static struct Cyc_PP_Doc*Cyc_Absynpp_offsetof_field_to_doc(void*f){
void*_tmp2AE=f;unsigned _tmp2AF;struct _fat_ptr*_tmp2B0;if(((struct Cyc_Absyn_StructField_Absyn_OffsetofField_struct*)_tmp2AE)->tag == 0U){_LL1: _tmp2B0=((struct Cyc_Absyn_StructField_Absyn_OffsetofField_struct*)_tmp2AE)->f1;_LL2: {struct _fat_ptr*n=_tmp2B0;
return({Cyc_PP_textptr(n);});}}else{_LL3: _tmp2AF=((struct Cyc_Absyn_TupleIndex_Absyn_OffsetofField_struct*)_tmp2AE)->f1;_LL4: {unsigned n=_tmp2AF;
return({struct Cyc_PP_Doc*(*_tmp2B1)(struct _fat_ptr s)=Cyc_PP_text;struct _fat_ptr _tmp2B2=(struct _fat_ptr)({struct Cyc_Int_pa_PrintArg_struct _tmp2B5=({struct Cyc_Int_pa_PrintArg_struct _tmp73A;_tmp73A.tag=1U,_tmp73A.f1=(unsigned long)((int)n);_tmp73A;});void*_tmp2B3[1U];_tmp2B3[0]=& _tmp2B5;({struct _fat_ptr _tmp806=({const char*_tmp2B4="%d";_tag_fat(_tmp2B4,sizeof(char),3U);});Cyc_aprintf(_tmp806,_tag_fat(_tmp2B3,sizeof(void*),1U));});});_tmp2B1(_tmp2B2);});}}_LL0:;}
# 1074 "absynpp.cyc"
int Cyc_Absynpp_exp_prec(struct Cyc_Absyn_Exp*e){
void*_stmttmp14=e->r;void*_tmp2B7=_stmttmp14;struct Cyc_Absyn_Exp*_tmp2B8;struct Cyc_Absyn_Exp*_tmp2B9;enum Cyc_Absyn_Primop _tmp2BA;switch(*((int*)_tmp2B7)){case 0U: _LL1: _LL2:
 goto _LL4;case 1U: _LL3: _LL4:
 return 10000;case 3U: _LL5: _tmp2BA=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp2B7)->f1;_LL6: {enum Cyc_Absyn_Primop p=_tmp2BA;
# 1079
enum Cyc_Absyn_Primop _tmp2BB=p;switch(_tmp2BB){case Cyc_Absyn_Plus: _LL58: _LL59:
 return 100;case Cyc_Absyn_Times: _LL5A: _LL5B:
 return 110;case Cyc_Absyn_Minus: _LL5C: _LL5D:
 return 100;case Cyc_Absyn_Div: _LL5E: _LL5F:
 goto _LL61;case Cyc_Absyn_Mod: _LL60: _LL61:
 return 110;case Cyc_Absyn_Eq: _LL62: _LL63:
 goto _LL65;case Cyc_Absyn_Neq: _LL64: _LL65:
 return 70;case Cyc_Absyn_Gt: _LL66: _LL67:
 goto _LL69;case Cyc_Absyn_Lt: _LL68: _LL69:
 goto _LL6B;case Cyc_Absyn_Gte: _LL6A: _LL6B:
 goto _LL6D;case Cyc_Absyn_Lte: _LL6C: _LL6D:
 return 80;case Cyc_Absyn_Not: _LL6E: _LL6F:
 goto _LL71;case Cyc_Absyn_Bitnot: _LL70: _LL71:
 return 130;case Cyc_Absyn_Bitand: _LL72: _LL73:
 return 60;case Cyc_Absyn_Bitor: _LL74: _LL75:
 return 40;case Cyc_Absyn_Bitxor: _LL76: _LL77:
 return 50;case Cyc_Absyn_Bitlshift: _LL78: _LL79:
 return 90;case Cyc_Absyn_Bitlrshift: _LL7A: _LL7B:
 return 80;case Cyc_Absyn_Numelts: _LL7C: _LL7D:
 return 140;default: _LL7E: _LL7F:
 return 140;}_LL57:;}case 4U: _LL7: _LL8:
# 1101
 return 20;case 5U: _LL9: _LLA:
 return 130;case 6U: _LLB: _LLC:
 return 30;case 7U: _LLD: _LLE:
 return 35;case 8U: _LLF: _LL10:
 return 30;case 9U: _LL11: _LL12:
 return - 10;case 10U: _LL13: _LL14:
 return 140;case 2U: _LL15: _LL16:
 return 140;case 11U: _LL17: _LL18:
 return 130;case 12U: _LL19: _tmp2B9=((struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*)_tmp2B7)->f1;_LL1A: {struct Cyc_Absyn_Exp*e2=_tmp2B9;
return({Cyc_Absynpp_exp_prec(e2);});}case 13U: _LL1B: _tmp2B8=((struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*)_tmp2B7)->f1;_LL1C: {struct Cyc_Absyn_Exp*e2=_tmp2B8;
return({Cyc_Absynpp_exp_prec(e2);});}case 14U: _LL1D: _LL1E:
 return 120;case 16U: _LL1F: _LL20:
 return 15;case 15U: _LL21: _LL22:
 goto _LL24;case 17U: _LL23: _LL24:
 goto _LL26;case 18U: _LL25: _LL26:
 goto _LL28;case 40U: _LL27: _LL28:
 goto _LL2A;case 41U: _LL29: _LL2A:
 goto _LL2C;case 39U: _LL2B: _LL2C:
 goto _LL2E;case 19U: _LL2D: _LL2E:
 goto _LL30;case 20U: _LL2F: _LL30:
 goto _LL32;case 42U: _LL31: _LL32:
 return 130;case 21U: _LL33: _LL34:
 goto _LL36;case 22U: _LL35: _LL36:
 goto _LL38;case 23U: _LL37: _LL38:
 return 140;case 24U: _LL39: _LL3A:
 return 150;case 25U: _LL3B: _LL3C:
 goto _LL3E;case 26U: _LL3D: _LL3E:
 goto _LL40;case 27U: _LL3F: _LL40:
 goto _LL42;case 28U: _LL41: _LL42:
 goto _LL44;case 29U: _LL43: _LL44:
 goto _LL46;case 30U: _LL45: _LL46:
 goto _LL48;case 31U: _LL47: _LL48:
 goto _LL4A;case 32U: _LL49: _LL4A:
 goto _LL4C;case 33U: _LL4B: _LL4C:
 goto _LL4E;case 34U: _LL4D: _LL4E:
 goto _LL50;case 35U: _LL4F: _LL50:
 goto _LL52;case 36U: _LL51: _LL52:
 goto _LL54;case 37U: _LL53: _LL54:
 return 140;default: _LL55: _LL56:
 return 10000;}_LL0:;}
# 1074
struct Cyc_PP_Doc*Cyc_Absynpp_exp2doc(struct Cyc_Absyn_Exp*e){
# 1145
return({Cyc_Absynpp_exp2doc_prec(0,e);});}struct _tuple20{struct _fat_ptr f1;struct Cyc_Absyn_Exp*f2;};
# 1148
static struct Cyc_PP_Doc*Cyc_Absynpp_asm_iolist_doc_in(struct Cyc_List_List*o){
struct Cyc_PP_Doc*s=({Cyc_PP_nil_doc();});
while((unsigned)o){
struct _tuple20*_stmttmp15=(struct _tuple20*)o->hd;struct Cyc_Absyn_Exp*_tmp2BF;struct _fat_ptr _tmp2BE;_LL1: _tmp2BE=_stmttmp15->f1;_tmp2BF=_stmttmp15->f2;_LL2: {struct _fat_ptr c=_tmp2BE;struct Cyc_Absyn_Exp*e=_tmp2BF;
s=({struct Cyc_PP_Doc*_tmp2C0[5U];({struct Cyc_PP_Doc*_tmp80B=({Cyc_PP_text(({const char*_tmp2C1="\"";_tag_fat(_tmp2C1,sizeof(char),2U);}));});_tmp2C0[0]=_tmp80B;}),({struct Cyc_PP_Doc*_tmp80A=({Cyc_PP_text(c);});_tmp2C0[1]=_tmp80A;}),({struct Cyc_PP_Doc*_tmp809=({Cyc_PP_text(({const char*_tmp2C2="\" (";_tag_fat(_tmp2C2,sizeof(char),4U);}));});_tmp2C0[2]=_tmp809;}),({struct Cyc_PP_Doc*_tmp808=({Cyc_Absynpp_exp2doc(e);});_tmp2C0[3]=_tmp808;}),({struct Cyc_PP_Doc*_tmp807=({Cyc_PP_text(({const char*_tmp2C3=")";_tag_fat(_tmp2C3,sizeof(char),2U);}));});_tmp2C0[4]=_tmp807;});Cyc_PP_cat(_tag_fat(_tmp2C0,sizeof(struct Cyc_PP_Doc*),5U));});
o=o->tl;
if((unsigned)o)
s=({struct Cyc_PP_Doc*_tmp2C4[2U];_tmp2C4[0]=s,({struct Cyc_PP_Doc*_tmp80C=({Cyc_PP_text(({const char*_tmp2C5=",";_tag_fat(_tmp2C5,sizeof(char),2U);}));});_tmp2C4[1]=_tmp80C;});Cyc_PP_cat(_tag_fat(_tmp2C4,sizeof(struct Cyc_PP_Doc*),2U));});}}
# 1158
return s;}
# 1161
static struct Cyc_PP_Doc*Cyc_Absynpp_asm_iolist_doc(struct Cyc_List_List*o,struct Cyc_List_List*i,struct Cyc_List_List*cl){
struct Cyc_PP_Doc*s=({Cyc_PP_nil_doc();});
if((unsigned)o)
s=({struct Cyc_PP_Doc*_tmp2C7[2U];({struct Cyc_PP_Doc*_tmp80E=({Cyc_PP_text(({const char*_tmp2C8=": ";_tag_fat(_tmp2C8,sizeof(char),3U);}));});_tmp2C7[0]=_tmp80E;}),({struct Cyc_PP_Doc*_tmp80D=({Cyc_Absynpp_asm_iolist_doc_in(o);});_tmp2C7[1]=_tmp80D;});Cyc_PP_cat(_tag_fat(_tmp2C7,sizeof(struct Cyc_PP_Doc*),2U));});
# 1163
if((unsigned)i){
# 1167
if(!((unsigned)o))
s=({struct Cyc_PP_Doc*_tmp2C9[3U];_tmp2C9[0]=s,({struct Cyc_PP_Doc*_tmp810=({Cyc_PP_text(({const char*_tmp2CA=": : ";_tag_fat(_tmp2CA,sizeof(char),5U);}));});_tmp2C9[1]=_tmp810;}),({struct Cyc_PP_Doc*_tmp80F=({Cyc_Absynpp_asm_iolist_doc_in(i);});_tmp2C9[2]=_tmp80F;});Cyc_PP_cat(_tag_fat(_tmp2C9,sizeof(struct Cyc_PP_Doc*),3U));});else{
# 1170
s=({struct Cyc_PP_Doc*_tmp2CB[3U];_tmp2CB[0]=s,({struct Cyc_PP_Doc*_tmp812=({Cyc_PP_text(({const char*_tmp2CC=" : ";_tag_fat(_tmp2CC,sizeof(char),4U);}));});_tmp2CB[1]=_tmp812;}),({struct Cyc_PP_Doc*_tmp811=({Cyc_Absynpp_asm_iolist_doc_in(i);});_tmp2CB[2]=_tmp811;});Cyc_PP_cat(_tag_fat(_tmp2CB,sizeof(struct Cyc_PP_Doc*),3U));});}}
# 1163
if((unsigned)cl){
# 1173
int ncol=(unsigned)i?2:((unsigned)o?1: 0);
s=({struct Cyc_PP_Doc*_tmp2CD[2U];_tmp2CD[0]=s,ncol == 0?({struct Cyc_PP_Doc*_tmp815=({Cyc_PP_text(({const char*_tmp2CE=": : :";_tag_fat(_tmp2CE,sizeof(char),6U);}));});_tmp2CD[1]=_tmp815;}):(ncol == 1?({struct Cyc_PP_Doc*_tmp814=({Cyc_PP_text(({const char*_tmp2CF=" : : ";_tag_fat(_tmp2CF,sizeof(char),6U);}));});_tmp2CD[1]=_tmp814;}):({struct Cyc_PP_Doc*_tmp813=({Cyc_PP_text(({const char*_tmp2D0=" : ";_tag_fat(_tmp2D0,sizeof(char),4U);}));});_tmp2CD[1]=_tmp813;}));Cyc_PP_cat(_tag_fat(_tmp2CD,sizeof(struct Cyc_PP_Doc*),2U));});
while(cl != 0){
s=({struct Cyc_PP_Doc*_tmp2D1[4U];_tmp2D1[0]=s,({struct Cyc_PP_Doc*_tmp818=({Cyc_PP_text(({const char*_tmp2D2="\"";_tag_fat(_tmp2D2,sizeof(char),2U);}));});_tmp2D1[1]=_tmp818;}),({struct Cyc_PP_Doc*_tmp817=({Cyc_PP_text(*((struct _fat_ptr*)cl->hd));});_tmp2D1[2]=_tmp817;}),({struct Cyc_PP_Doc*_tmp816=({Cyc_PP_text(({const char*_tmp2D3="\"";_tag_fat(_tmp2D3,sizeof(char),2U);}));});_tmp2D1[3]=_tmp816;});Cyc_PP_cat(_tag_fat(_tmp2D1,sizeof(struct Cyc_PP_Doc*),4U));});
cl=cl->tl;
if((unsigned)cl)
s=({struct Cyc_PP_Doc*_tmp2D4[2U];_tmp2D4[0]=s,({struct Cyc_PP_Doc*_tmp819=({Cyc_PP_text(({const char*_tmp2D5=", ";_tag_fat(_tmp2D5,sizeof(char),3U);}));});_tmp2D4[1]=_tmp819;});Cyc_PP_cat(_tag_fat(_tmp2D4,sizeof(struct Cyc_PP_Doc*),2U));});}}
# 1163
return s;}
# 1161
struct Cyc_PP_Doc*Cyc_Absynpp_exp2doc_prec(int inprec,struct Cyc_Absyn_Exp*e){
# 1186
int myprec=({Cyc_Absynpp_exp_prec(e);});
struct Cyc_PP_Doc*s;
{void*_stmttmp16=e->r;void*_tmp2D7=_stmttmp16;struct Cyc_Absyn_Stmt*_tmp2D8;struct Cyc_List_List*_tmp2DA;struct Cyc_Core_Opt*_tmp2D9;struct Cyc_Absyn_Exp*_tmp2DC;struct Cyc_Absyn_Exp*_tmp2DB;int _tmp2E1;struct Cyc_Absyn_Exp*_tmp2E0;void**_tmp2DF;struct Cyc_Absyn_Exp*_tmp2DE;int _tmp2DD;struct Cyc_Absyn_Exp*_tmp2E3;struct Cyc_Absyn_Exp*_tmp2E2;struct Cyc_Absyn_Enumfield*_tmp2E4;struct Cyc_Absyn_Enumfield*_tmp2E5;struct Cyc_Absyn_Datatypefield*_tmp2E7;struct Cyc_List_List*_tmp2E6;struct Cyc_List_List*_tmp2E8;struct Cyc_List_List*_tmp2EB;struct Cyc_List_List*_tmp2EA;struct _tuple0*_tmp2E9;void*_tmp2ED;struct Cyc_Absyn_Exp*_tmp2EC;struct Cyc_Absyn_Exp*_tmp2F0;struct Cyc_Absyn_Exp*_tmp2EF;struct Cyc_Absyn_Vardecl*_tmp2EE;struct Cyc_List_List*_tmp2F1;struct Cyc_List_List*_tmp2F3;struct _tuple9*_tmp2F2;struct Cyc_List_List*_tmp2F4;struct Cyc_Absyn_Exp*_tmp2F6;struct Cyc_Absyn_Exp*_tmp2F5;struct _fat_ptr*_tmp2F8;struct Cyc_Absyn_Exp*_tmp2F7;struct _fat_ptr*_tmp2FA;struct Cyc_Absyn_Exp*_tmp2F9;struct Cyc_Absyn_Exp*_tmp2FB;struct Cyc_List_List*_tmp2FD;void*_tmp2FC;struct _fat_ptr*_tmp2FF;struct Cyc_Absyn_Exp*_tmp2FE;struct Cyc_List_List*_tmp304;struct Cyc_List_List*_tmp303;struct Cyc_List_List*_tmp302;struct _fat_ptr _tmp301;int _tmp300;void*_tmp305;struct Cyc_Absyn_Exp*_tmp306;struct Cyc_Absyn_Exp*_tmp307;void*_tmp308;struct Cyc_Absyn_Exp*_tmp30A;struct Cyc_Absyn_Exp*_tmp309;struct Cyc_Absyn_Exp*_tmp30B;struct Cyc_Absyn_Exp*_tmp30D;void*_tmp30C;struct Cyc_Absyn_Exp*_tmp30E;struct Cyc_Absyn_Exp*_tmp30F;struct Cyc_Absyn_Exp*_tmp310;struct Cyc_List_List*_tmp312;struct Cyc_Absyn_Exp*_tmp311;struct Cyc_Absyn_Exp*_tmp314;struct Cyc_Absyn_Exp*_tmp313;struct Cyc_Absyn_Exp*_tmp316;struct Cyc_Absyn_Exp*_tmp315;struct Cyc_Absyn_Exp*_tmp318;struct Cyc_Absyn_Exp*_tmp317;struct Cyc_Absyn_Exp*_tmp31B;struct Cyc_Absyn_Exp*_tmp31A;struct Cyc_Absyn_Exp*_tmp319;enum Cyc_Absyn_Incrementor _tmp31D;struct Cyc_Absyn_Exp*_tmp31C;struct Cyc_Absyn_Exp*_tmp320;struct Cyc_Core_Opt*_tmp31F;struct Cyc_Absyn_Exp*_tmp31E;struct Cyc_List_List*_tmp322;enum Cyc_Absyn_Primop _tmp321;struct _fat_ptr _tmp323;void*_tmp324;union Cyc_Absyn_Cnst _tmp325;switch(*((int*)_tmp2D7)){case 0U: _LL1: _tmp325=((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp2D7)->f1;_LL2: {union Cyc_Absyn_Cnst c=_tmp325;
s=({Cyc_Absynpp_cnst2doc(c);});goto _LL0;}case 1U: _LL3: _tmp324=(void*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp2D7)->f1;_LL4: {void*b=_tmp324;
s=({struct Cyc_PP_Doc*(*_tmp326)(struct _tuple0*q)=Cyc_Absynpp_qvar2doc;struct _tuple0*_tmp327=({Cyc_Absyn_binding2qvar(b);});_tmp326(_tmp327);});goto _LL0;}case 2U: _LL5: _tmp323=((struct Cyc_Absyn_Pragma_e_Absyn_Raw_exp_struct*)_tmp2D7)->f1;_LL6: {struct _fat_ptr p=_tmp323;
# 1192
s=({struct Cyc_PP_Doc*_tmp328[4U];({struct Cyc_PP_Doc*_tmp81D=({Cyc_PP_text(({const char*_tmp329="__cyclone_pragma__";_tag_fat(_tmp329,sizeof(char),19U);}));});_tmp328[0]=_tmp81D;}),({struct Cyc_PP_Doc*_tmp81C=({Cyc_PP_text(({const char*_tmp32A="(";_tag_fat(_tmp32A,sizeof(char),2U);}));});_tmp328[1]=_tmp81C;}),({struct Cyc_PP_Doc*_tmp81B=({Cyc_PP_text(p);});_tmp328[2]=_tmp81B;}),({struct Cyc_PP_Doc*_tmp81A=({Cyc_PP_text(({const char*_tmp32B=")";_tag_fat(_tmp32B,sizeof(char),2U);}));});_tmp328[3]=_tmp81A;});Cyc_PP_cat(_tag_fat(_tmp328,sizeof(struct Cyc_PP_Doc*),4U));});goto _LL0;}case 3U: _LL7: _tmp321=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp2D7)->f1;_tmp322=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp2D7)->f2;_LL8: {enum Cyc_Absyn_Primop p=_tmp321;struct Cyc_List_List*es=_tmp322;
s=({Cyc_Absynpp_primapp2doc(myprec,p,es);});goto _LL0;}case 4U: _LL9: _tmp31E=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp2D7)->f1;_tmp31F=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp2D7)->f2;_tmp320=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp2D7)->f3;_LLA: {struct Cyc_Absyn_Exp*e1=_tmp31E;struct Cyc_Core_Opt*popt=_tmp31F;struct Cyc_Absyn_Exp*e2=_tmp320;
# 1195
s=({struct Cyc_PP_Doc*_tmp32C[5U];({struct Cyc_PP_Doc*_tmp823=({Cyc_Absynpp_exp2doc_prec(myprec,e1);});_tmp32C[0]=_tmp823;}),({
struct Cyc_PP_Doc*_tmp822=({Cyc_PP_text(({const char*_tmp32D=" ";_tag_fat(_tmp32D,sizeof(char),2U);}));});_tmp32C[1]=_tmp822;}),
popt == 0?({struct Cyc_PP_Doc*_tmp821=({Cyc_PP_nil_doc();});_tmp32C[2]=_tmp821;}):({struct Cyc_PP_Doc*_tmp820=({Cyc_Absynpp_prim2doc((enum Cyc_Absyn_Primop)popt->v);});_tmp32C[2]=_tmp820;}),({
struct Cyc_PP_Doc*_tmp81F=({Cyc_PP_text(({const char*_tmp32E="= ";_tag_fat(_tmp32E,sizeof(char),3U);}));});_tmp32C[3]=_tmp81F;}),({
struct Cyc_PP_Doc*_tmp81E=({Cyc_Absynpp_exp2doc_prec(myprec,e2);});_tmp32C[4]=_tmp81E;});Cyc_PP_cat(_tag_fat(_tmp32C,sizeof(struct Cyc_PP_Doc*),5U));});
goto _LL0;}case 5U: _LLB: _tmp31C=((struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*)_tmp2D7)->f1;_tmp31D=((struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*)_tmp2D7)->f2;_LLC: {struct Cyc_Absyn_Exp*e2=_tmp31C;enum Cyc_Absyn_Incrementor i=_tmp31D;
# 1202
struct Cyc_PP_Doc*es=({Cyc_Absynpp_exp2doc_prec(myprec,e2);});
{enum Cyc_Absyn_Incrementor _tmp32F=i;switch(_tmp32F){case Cyc_Absyn_PreInc: _LL58: _LL59:
 s=({struct Cyc_PP_Doc*_tmp330[2U];({struct Cyc_PP_Doc*_tmp824=({Cyc_PP_text(({const char*_tmp331="++";_tag_fat(_tmp331,sizeof(char),3U);}));});_tmp330[0]=_tmp824;}),_tmp330[1]=es;Cyc_PP_cat(_tag_fat(_tmp330,sizeof(struct Cyc_PP_Doc*),2U));});goto _LL57;case Cyc_Absyn_PreDec: _LL5A: _LL5B:
 s=({struct Cyc_PP_Doc*_tmp332[2U];({struct Cyc_PP_Doc*_tmp825=({Cyc_PP_text(({const char*_tmp333="--";_tag_fat(_tmp333,sizeof(char),3U);}));});_tmp332[0]=_tmp825;}),_tmp332[1]=es;Cyc_PP_cat(_tag_fat(_tmp332,sizeof(struct Cyc_PP_Doc*),2U));});goto _LL57;case Cyc_Absyn_PostInc: _LL5C: _LL5D:
 s=({struct Cyc_PP_Doc*_tmp334[2U];_tmp334[0]=es,({struct Cyc_PP_Doc*_tmp826=({Cyc_PP_text(({const char*_tmp335="++";_tag_fat(_tmp335,sizeof(char),3U);}));});_tmp334[1]=_tmp826;});Cyc_PP_cat(_tag_fat(_tmp334,sizeof(struct Cyc_PP_Doc*),2U));});goto _LL57;case Cyc_Absyn_PostDec: _LL5E: _LL5F:
 goto _LL61;default: _LL60: _LL61:
 s=({struct Cyc_PP_Doc*_tmp336[2U];_tmp336[0]=es,({struct Cyc_PP_Doc*_tmp827=({Cyc_PP_text(({const char*_tmp337="--";_tag_fat(_tmp337,sizeof(char),3U);}));});_tmp336[1]=_tmp827;});Cyc_PP_cat(_tag_fat(_tmp336,sizeof(struct Cyc_PP_Doc*),2U));});goto _LL57;}_LL57:;}
# 1210
goto _LL0;}case 6U: _LLD: _tmp319=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp2D7)->f1;_tmp31A=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp2D7)->f2;_tmp31B=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp2D7)->f3;_LLE: {struct Cyc_Absyn_Exp*e1=_tmp319;struct Cyc_Absyn_Exp*e2=_tmp31A;struct Cyc_Absyn_Exp*e3=_tmp31B;
# 1212
s=({struct Cyc_PP_Doc*_tmp338[5U];({struct Cyc_PP_Doc*_tmp82C=({Cyc_Absynpp_exp2doc_prec(myprec,e1);});_tmp338[0]=_tmp82C;}),({struct Cyc_PP_Doc*_tmp82B=({Cyc_PP_text(({const char*_tmp339=" ? ";_tag_fat(_tmp339,sizeof(char),4U);}));});_tmp338[1]=_tmp82B;}),({struct Cyc_PP_Doc*_tmp82A=({Cyc_Absynpp_exp2doc_prec(0,e2);});_tmp338[2]=_tmp82A;}),({
struct Cyc_PP_Doc*_tmp829=({Cyc_PP_text(({const char*_tmp33A=" : ";_tag_fat(_tmp33A,sizeof(char),4U);}));});_tmp338[3]=_tmp829;}),({struct Cyc_PP_Doc*_tmp828=({Cyc_Absynpp_exp2doc_prec(myprec,e3);});_tmp338[4]=_tmp828;});Cyc_PP_cat(_tag_fat(_tmp338,sizeof(struct Cyc_PP_Doc*),5U));});
goto _LL0;}case 7U: _LLF: _tmp317=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp2D7)->f1;_tmp318=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp2D7)->f2;_LL10: {struct Cyc_Absyn_Exp*e1=_tmp317;struct Cyc_Absyn_Exp*e2=_tmp318;
# 1216
s=({struct Cyc_PP_Doc*_tmp33B[3U];({struct Cyc_PP_Doc*_tmp82F=({Cyc_Absynpp_exp2doc_prec(myprec,e1);});_tmp33B[0]=_tmp82F;}),({struct Cyc_PP_Doc*_tmp82E=({Cyc_PP_text(({const char*_tmp33C=" && ";_tag_fat(_tmp33C,sizeof(char),5U);}));});_tmp33B[1]=_tmp82E;}),({struct Cyc_PP_Doc*_tmp82D=({Cyc_Absynpp_exp2doc_prec(myprec,e2);});_tmp33B[2]=_tmp82D;});Cyc_PP_cat(_tag_fat(_tmp33B,sizeof(struct Cyc_PP_Doc*),3U));});
goto _LL0;}case 8U: _LL11: _tmp315=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp2D7)->f1;_tmp316=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp2D7)->f2;_LL12: {struct Cyc_Absyn_Exp*e1=_tmp315;struct Cyc_Absyn_Exp*e2=_tmp316;
# 1219
s=({struct Cyc_PP_Doc*_tmp33D[3U];({struct Cyc_PP_Doc*_tmp832=({Cyc_Absynpp_exp2doc_prec(myprec,e1);});_tmp33D[0]=_tmp832;}),({struct Cyc_PP_Doc*_tmp831=({Cyc_PP_text(({const char*_tmp33E=" || ";_tag_fat(_tmp33E,sizeof(char),5U);}));});_tmp33D[1]=_tmp831;}),({struct Cyc_PP_Doc*_tmp830=({Cyc_Absynpp_exp2doc_prec(myprec,e2);});_tmp33D[2]=_tmp830;});Cyc_PP_cat(_tag_fat(_tmp33D,sizeof(struct Cyc_PP_Doc*),3U));});
goto _LL0;}case 9U: _LL13: _tmp313=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp2D7)->f1;_tmp314=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp2D7)->f2;_LL14: {struct Cyc_Absyn_Exp*e1=_tmp313;struct Cyc_Absyn_Exp*e2=_tmp314;
# 1224
s=({struct Cyc_PP_Doc*_tmp33F[3U];({struct Cyc_PP_Doc*_tmp835=({Cyc_Absynpp_exp2doc_prec(myprec - 1,e1);});_tmp33F[0]=_tmp835;}),({struct Cyc_PP_Doc*_tmp834=({Cyc_PP_text(({const char*_tmp340=", ";_tag_fat(_tmp340,sizeof(char),3U);}));});_tmp33F[1]=_tmp834;}),({struct Cyc_PP_Doc*_tmp833=({Cyc_Absynpp_exp2doc_prec(myprec - 1,e2);});_tmp33F[2]=_tmp833;});Cyc_PP_cat(_tag_fat(_tmp33F,sizeof(struct Cyc_PP_Doc*),3U));});
goto _LL0;}case 10U: _LL15: _tmp311=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp2D7)->f1;_tmp312=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp2D7)->f2;_LL16: {struct Cyc_Absyn_Exp*e1=_tmp311;struct Cyc_List_List*es=_tmp312;
# 1227
s=({struct Cyc_PP_Doc*_tmp341[4U];({struct Cyc_PP_Doc*_tmp839=({Cyc_Absynpp_exp2doc_prec(myprec,e1);});_tmp341[0]=_tmp839;}),({
struct Cyc_PP_Doc*_tmp838=({Cyc_PP_text(({const char*_tmp342="(";_tag_fat(_tmp342,sizeof(char),2U);}));});_tmp341[1]=_tmp838;}),({
struct Cyc_PP_Doc*_tmp837=({Cyc_Absynpp_exps2doc_prec(20,es);});_tmp341[2]=_tmp837;}),({
struct Cyc_PP_Doc*_tmp836=({Cyc_PP_text(({const char*_tmp343=")";_tag_fat(_tmp343,sizeof(char),2U);}));});_tmp341[3]=_tmp836;});Cyc_PP_cat(_tag_fat(_tmp341,sizeof(struct Cyc_PP_Doc*),4U));});
goto _LL0;}case 11U: _LL17: _tmp310=((struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct*)_tmp2D7)->f1;_LL18: {struct Cyc_Absyn_Exp*e1=_tmp310;
s=({struct Cyc_PP_Doc*_tmp344[2U];({struct Cyc_PP_Doc*_tmp83B=({Cyc_PP_text(({const char*_tmp345="throw ";_tag_fat(_tmp345,sizeof(char),7U);}));});_tmp344[0]=_tmp83B;}),({struct Cyc_PP_Doc*_tmp83A=({Cyc_Absynpp_exp2doc_prec(myprec,e1);});_tmp344[1]=_tmp83A;});Cyc_PP_cat(_tag_fat(_tmp344,sizeof(struct Cyc_PP_Doc*),2U));});goto _LL0;}case 12U: _LL19: _tmp30F=((struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*)_tmp2D7)->f1;_LL1A: {struct Cyc_Absyn_Exp*e1=_tmp30F;
s=({Cyc_Absynpp_exp2doc_prec(inprec,e1);});goto _LL0;}case 13U: _LL1B: _tmp30E=((struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*)_tmp2D7)->f1;_LL1C: {struct Cyc_Absyn_Exp*e1=_tmp30E;
s=({Cyc_Absynpp_exp2doc_prec(inprec,e1);});goto _LL0;}case 14U: _LL1D: _tmp30C=(void*)((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp2D7)->f1;_tmp30D=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp2D7)->f2;_LL1E: {void*t=_tmp30C;struct Cyc_Absyn_Exp*e1=_tmp30D;
# 1236
s=({struct Cyc_PP_Doc*_tmp346[4U];({struct Cyc_PP_Doc*_tmp83F=({Cyc_PP_text(({const char*_tmp347="(";_tag_fat(_tmp347,sizeof(char),2U);}));});_tmp346[0]=_tmp83F;}),({struct Cyc_PP_Doc*_tmp83E=({Cyc_Absynpp_typ2doc(t);});_tmp346[1]=_tmp83E;}),({struct Cyc_PP_Doc*_tmp83D=({Cyc_PP_text(({const char*_tmp348=")";_tag_fat(_tmp348,sizeof(char),2U);}));});_tmp346[2]=_tmp83D;}),({struct Cyc_PP_Doc*_tmp83C=({Cyc_Absynpp_exp2doc_prec(myprec,e1);});_tmp346[3]=_tmp83C;});Cyc_PP_cat(_tag_fat(_tmp346,sizeof(struct Cyc_PP_Doc*),4U));});goto _LL0;}case 15U: _LL1F: _tmp30B=((struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct*)_tmp2D7)->f1;_LL20: {struct Cyc_Absyn_Exp*e1=_tmp30B;
s=({struct Cyc_PP_Doc*_tmp349[2U];({struct Cyc_PP_Doc*_tmp841=({Cyc_PP_text(({const char*_tmp34A="&";_tag_fat(_tmp34A,sizeof(char),2U);}));});_tmp349[0]=_tmp841;}),({struct Cyc_PP_Doc*_tmp840=({Cyc_Absynpp_exp2doc_prec(myprec,e1);});_tmp349[1]=_tmp840;});Cyc_PP_cat(_tag_fat(_tmp349,sizeof(struct Cyc_PP_Doc*),2U));});goto _LL0;}case 16U: _LL21: _tmp309=((struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct*)_tmp2D7)->f1;_tmp30A=((struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct*)_tmp2D7)->f2;_LL22: {struct Cyc_Absyn_Exp*ropt=_tmp309;struct Cyc_Absyn_Exp*e1=_tmp30A;
# 1239
if(ropt == 0)
s=({struct Cyc_PP_Doc*_tmp34B[2U];({struct Cyc_PP_Doc*_tmp843=({Cyc_PP_text(({const char*_tmp34C="new ";_tag_fat(_tmp34C,sizeof(char),5U);}));});_tmp34B[0]=_tmp843;}),({struct Cyc_PP_Doc*_tmp842=({Cyc_Absynpp_exp2doc_prec(myprec,e1);});_tmp34B[1]=_tmp842;});Cyc_PP_cat(_tag_fat(_tmp34B,sizeof(struct Cyc_PP_Doc*),2U));});else{
# 1242
s=({struct Cyc_PP_Doc*_tmp34D[4U];({struct Cyc_PP_Doc*_tmp847=({Cyc_PP_text(({const char*_tmp34E="rnew(";_tag_fat(_tmp34E,sizeof(char),6U);}));});_tmp34D[0]=_tmp847;}),({struct Cyc_PP_Doc*_tmp846=({Cyc_Absynpp_exp2doc(ropt);});_tmp34D[1]=_tmp846;}),({struct Cyc_PP_Doc*_tmp845=({Cyc_PP_text(({const char*_tmp34F=") ";_tag_fat(_tmp34F,sizeof(char),3U);}));});_tmp34D[2]=_tmp845;}),({struct Cyc_PP_Doc*_tmp844=({Cyc_Absynpp_exp2doc_prec(myprec,e1);});_tmp34D[3]=_tmp844;});Cyc_PP_cat(_tag_fat(_tmp34D,sizeof(struct Cyc_PP_Doc*),4U));});}
goto _LL0;}case 17U: _LL23: _tmp308=(void*)((struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct*)_tmp2D7)->f1;_LL24: {void*t=_tmp308;
s=({struct Cyc_PP_Doc*_tmp350[3U];({struct Cyc_PP_Doc*_tmp84A=({Cyc_PP_text(({const char*_tmp351="sizeof(";_tag_fat(_tmp351,sizeof(char),8U);}));});_tmp350[0]=_tmp84A;}),({struct Cyc_PP_Doc*_tmp849=({Cyc_Absynpp_typ2doc(t);});_tmp350[1]=_tmp849;}),({struct Cyc_PP_Doc*_tmp848=({Cyc_PP_text(({const char*_tmp352=")";_tag_fat(_tmp352,sizeof(char),2U);}));});_tmp350[2]=_tmp848;});Cyc_PP_cat(_tag_fat(_tmp350,sizeof(struct Cyc_PP_Doc*),3U));});goto _LL0;}case 18U: _LL25: _tmp307=((struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct*)_tmp2D7)->f1;_LL26: {struct Cyc_Absyn_Exp*e1=_tmp307;
s=({struct Cyc_PP_Doc*_tmp353[3U];({struct Cyc_PP_Doc*_tmp84D=({Cyc_PP_text(({const char*_tmp354="sizeof(";_tag_fat(_tmp354,sizeof(char),8U);}));});_tmp353[0]=_tmp84D;}),({struct Cyc_PP_Doc*_tmp84C=({Cyc_Absynpp_exp2doc(e1);});_tmp353[1]=_tmp84C;}),({struct Cyc_PP_Doc*_tmp84B=({Cyc_PP_text(({const char*_tmp355=")";_tag_fat(_tmp355,sizeof(char),2U);}));});_tmp353[2]=_tmp84B;});Cyc_PP_cat(_tag_fat(_tmp353,sizeof(struct Cyc_PP_Doc*),3U));});goto _LL0;}case 42U: _LL27: _tmp306=((struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct*)_tmp2D7)->f1;_LL28: {struct Cyc_Absyn_Exp*e=_tmp306;
# 1247
s=({struct Cyc_PP_Doc*_tmp356[3U];({struct Cyc_PP_Doc*_tmp850=({Cyc_PP_text(({const char*_tmp357="__extension__(";_tag_fat(_tmp357,sizeof(char),15U);}));});_tmp356[0]=_tmp850;}),({struct Cyc_PP_Doc*_tmp84F=({Cyc_Absynpp_exp2doc(e);});_tmp356[1]=_tmp84F;}),({struct Cyc_PP_Doc*_tmp84E=({Cyc_PP_text(({const char*_tmp358=")";_tag_fat(_tmp358,sizeof(char),2U);}));});_tmp356[2]=_tmp84E;});Cyc_PP_cat(_tag_fat(_tmp356,sizeof(struct Cyc_PP_Doc*),3U));});goto _LL0;}case 40U: _LL29: _tmp305=(void*)((struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct*)_tmp2D7)->f1;_LL2A: {void*t=_tmp305;
s=({struct Cyc_PP_Doc*_tmp359[3U];({struct Cyc_PP_Doc*_tmp853=({Cyc_PP_text(({const char*_tmp35A="valueof(";_tag_fat(_tmp35A,sizeof(char),9U);}));});_tmp359[0]=_tmp853;}),({struct Cyc_PP_Doc*_tmp852=({Cyc_Absynpp_typ2doc(t);});_tmp359[1]=_tmp852;}),({struct Cyc_PP_Doc*_tmp851=({Cyc_PP_text(({const char*_tmp35B=")";_tag_fat(_tmp35B,sizeof(char),2U);}));});_tmp359[2]=_tmp851;});Cyc_PP_cat(_tag_fat(_tmp359,sizeof(struct Cyc_PP_Doc*),3U));});goto _LL0;}case 41U: _LL2B: _tmp300=((struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct*)_tmp2D7)->f1;_tmp301=((struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct*)_tmp2D7)->f2;_tmp302=((struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct*)_tmp2D7)->f3;_tmp303=((struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct*)_tmp2D7)->f4;_tmp304=((struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct*)_tmp2D7)->f5;_LL2C: {int vol=_tmp300;struct _fat_ptr t=_tmp301;struct Cyc_List_List*o=_tmp302;struct Cyc_List_List*i=_tmp303;struct Cyc_List_List*cl=_tmp304;
# 1250
if(vol)
s=({struct Cyc_PP_Doc*_tmp35C[7U];({struct Cyc_PP_Doc*_tmp85A=({Cyc_PP_text(({const char*_tmp35D="__asm__";_tag_fat(_tmp35D,sizeof(char),8U);}));});_tmp35C[0]=_tmp85A;}),({struct Cyc_PP_Doc*_tmp859=({Cyc_PP_text(({const char*_tmp35E=" volatile (";_tag_fat(_tmp35E,sizeof(char),12U);}));});_tmp35C[1]=_tmp859;}),({struct Cyc_PP_Doc*_tmp858=({Cyc_PP_text(({const char*_tmp35F="\"";_tag_fat(_tmp35F,sizeof(char),2U);}));});_tmp35C[2]=_tmp858;}),({struct Cyc_PP_Doc*_tmp857=({Cyc_PP_text(t);});_tmp35C[3]=_tmp857;}),({struct Cyc_PP_Doc*_tmp856=({Cyc_PP_text(({const char*_tmp360="\"";_tag_fat(_tmp360,sizeof(char),2U);}));});_tmp35C[4]=_tmp856;}),({struct Cyc_PP_Doc*_tmp855=({Cyc_Absynpp_asm_iolist_doc(o,i,cl);});_tmp35C[5]=_tmp855;}),({struct Cyc_PP_Doc*_tmp854=({Cyc_PP_text(({const char*_tmp361=")";_tag_fat(_tmp361,sizeof(char),2U);}));});_tmp35C[6]=_tmp854;});Cyc_PP_cat(_tag_fat(_tmp35C,sizeof(struct Cyc_PP_Doc*),7U));});else{
# 1253
s=({struct Cyc_PP_Doc*_tmp362[6U];({struct Cyc_PP_Doc*_tmp860=({Cyc_PP_text(({const char*_tmp363="__asm__(";_tag_fat(_tmp363,sizeof(char),9U);}));});_tmp362[0]=_tmp860;}),({struct Cyc_PP_Doc*_tmp85F=({Cyc_PP_text(({const char*_tmp364="\"";_tag_fat(_tmp364,sizeof(char),2U);}));});_tmp362[1]=_tmp85F;}),({struct Cyc_PP_Doc*_tmp85E=({Cyc_PP_text(t);});_tmp362[2]=_tmp85E;}),({struct Cyc_PP_Doc*_tmp85D=({Cyc_PP_text(({const char*_tmp365="\"";_tag_fat(_tmp365,sizeof(char),2U);}));});_tmp362[3]=_tmp85D;}),({struct Cyc_PP_Doc*_tmp85C=({Cyc_Absynpp_asm_iolist_doc(o,i,cl);});_tmp362[4]=_tmp85C;}),({struct Cyc_PP_Doc*_tmp85B=({Cyc_PP_text(({const char*_tmp366=")";_tag_fat(_tmp366,sizeof(char),2U);}));});_tmp362[5]=_tmp85B;});Cyc_PP_cat(_tag_fat(_tmp362,sizeof(struct Cyc_PP_Doc*),6U));});}
goto _LL0;}case 39U: _LL2D: _tmp2FE=((struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct*)_tmp2D7)->f1;_tmp2FF=((struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct*)_tmp2D7)->f2;_LL2E: {struct Cyc_Absyn_Exp*e=_tmp2FE;struct _fat_ptr*f=_tmp2FF;
# 1256
s=({struct Cyc_PP_Doc*_tmp367[5U];({struct Cyc_PP_Doc*_tmp865=({Cyc_PP_text(({const char*_tmp368="tagcheck(";_tag_fat(_tmp368,sizeof(char),10U);}));});_tmp367[0]=_tmp865;}),({struct Cyc_PP_Doc*_tmp864=({Cyc_Absynpp_exp2doc(e);});_tmp367[1]=_tmp864;}),({struct Cyc_PP_Doc*_tmp863=({Cyc_PP_text(({const char*_tmp369=".";_tag_fat(_tmp369,sizeof(char),2U);}));});_tmp367[2]=_tmp863;}),({struct Cyc_PP_Doc*_tmp862=({Cyc_PP_textptr(f);});_tmp367[3]=_tmp862;}),({struct Cyc_PP_Doc*_tmp861=({Cyc_PP_text(({const char*_tmp36A=")";_tag_fat(_tmp36A,sizeof(char),2U);}));});_tmp367[4]=_tmp861;});Cyc_PP_cat(_tag_fat(_tmp367,sizeof(struct Cyc_PP_Doc*),5U));});
goto _LL0;}case 19U: _LL2F: _tmp2FC=(void*)((struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct*)_tmp2D7)->f1;_tmp2FD=((struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct*)_tmp2D7)->f2;_LL30: {void*t=_tmp2FC;struct Cyc_List_List*l=_tmp2FD;
# 1259
s=({struct Cyc_PP_Doc*_tmp36B[5U];({struct Cyc_PP_Doc*_tmp86B=({Cyc_PP_text(({const char*_tmp36C="offsetof(";_tag_fat(_tmp36C,sizeof(char),10U);}));});_tmp36B[0]=_tmp86B;}),({struct Cyc_PP_Doc*_tmp86A=({Cyc_Absynpp_typ2doc(t);});_tmp36B[1]=_tmp86A;}),({struct Cyc_PP_Doc*_tmp869=({Cyc_PP_text(({const char*_tmp36D=",";_tag_fat(_tmp36D,sizeof(char),2U);}));});_tmp36B[2]=_tmp869;}),({
struct Cyc_PP_Doc*_tmp868=({struct Cyc_PP_Doc*(*_tmp36E)(struct _fat_ptr sep,struct Cyc_List_List*l)=Cyc_PP_seq;struct _fat_ptr _tmp36F=({const char*_tmp370=".";_tag_fat(_tmp370,sizeof(char),2U);});struct Cyc_List_List*_tmp371=({({struct Cyc_List_List*(*_tmp867)(struct Cyc_PP_Doc*(*f)(void*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp372)(struct Cyc_PP_Doc*(*f)(void*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_PP_Doc*(*f)(void*),struct Cyc_List_List*x))Cyc_List_map;_tmp372;});_tmp867(Cyc_Absynpp_offsetof_field_to_doc,l);});});_tmp36E(_tmp36F,_tmp371);});_tmp36B[3]=_tmp868;}),({struct Cyc_PP_Doc*_tmp866=({Cyc_PP_text(({const char*_tmp373=")";_tag_fat(_tmp373,sizeof(char),2U);}));});_tmp36B[4]=_tmp866;});Cyc_PP_cat(_tag_fat(_tmp36B,sizeof(struct Cyc_PP_Doc*),5U));});
goto _LL0;}case 20U: _LL31: _tmp2FB=((struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_tmp2D7)->f1;_LL32: {struct Cyc_Absyn_Exp*e1=_tmp2FB;
s=({struct Cyc_PP_Doc*_tmp374[2U];({struct Cyc_PP_Doc*_tmp86D=({Cyc_PP_text(({const char*_tmp375="*";_tag_fat(_tmp375,sizeof(char),2U);}));});_tmp374[0]=_tmp86D;}),({struct Cyc_PP_Doc*_tmp86C=({Cyc_Absynpp_exp2doc_prec(myprec,e1);});_tmp374[1]=_tmp86C;});Cyc_PP_cat(_tag_fat(_tmp374,sizeof(struct Cyc_PP_Doc*),2U));});goto _LL0;}case 21U: _LL33: _tmp2F9=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp2D7)->f1;_tmp2FA=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp2D7)->f2;_LL34: {struct Cyc_Absyn_Exp*e1=_tmp2F9;struct _fat_ptr*n=_tmp2FA;
# 1264
s=({struct Cyc_PP_Doc*_tmp376[3U];({struct Cyc_PP_Doc*_tmp870=({Cyc_Absynpp_exp2doc_prec(myprec,e1);});_tmp376[0]=_tmp870;}),({struct Cyc_PP_Doc*_tmp86F=({Cyc_PP_text(({const char*_tmp377=".";_tag_fat(_tmp377,sizeof(char),2U);}));});_tmp376[1]=_tmp86F;}),({struct Cyc_PP_Doc*_tmp86E=({Cyc_PP_textptr(n);});_tmp376[2]=_tmp86E;});Cyc_PP_cat(_tag_fat(_tmp376,sizeof(struct Cyc_PP_Doc*),3U));});goto _LL0;}case 22U: _LL35: _tmp2F7=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp2D7)->f1;_tmp2F8=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp2D7)->f2;_LL36: {struct Cyc_Absyn_Exp*e1=_tmp2F7;struct _fat_ptr*n=_tmp2F8;
# 1266
s=({struct Cyc_PP_Doc*_tmp378[3U];({struct Cyc_PP_Doc*_tmp873=({Cyc_Absynpp_exp2doc_prec(myprec,e1);});_tmp378[0]=_tmp873;}),({struct Cyc_PP_Doc*_tmp872=({Cyc_PP_text(({const char*_tmp379="->";_tag_fat(_tmp379,sizeof(char),3U);}));});_tmp378[1]=_tmp872;}),({struct Cyc_PP_Doc*_tmp871=({Cyc_PP_textptr(n);});_tmp378[2]=_tmp871;});Cyc_PP_cat(_tag_fat(_tmp378,sizeof(struct Cyc_PP_Doc*),3U));});goto _LL0;}case 23U: _LL37: _tmp2F5=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp2D7)->f1;_tmp2F6=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp2D7)->f2;_LL38: {struct Cyc_Absyn_Exp*e1=_tmp2F5;struct Cyc_Absyn_Exp*e2=_tmp2F6;
# 1268
s=({struct Cyc_PP_Doc*_tmp37A[4U];({struct Cyc_PP_Doc*_tmp877=({Cyc_Absynpp_exp2doc_prec(myprec,e1);});_tmp37A[0]=_tmp877;}),({struct Cyc_PP_Doc*_tmp876=({Cyc_PP_text(({const char*_tmp37B="[";_tag_fat(_tmp37B,sizeof(char),2U);}));});_tmp37A[1]=_tmp876;}),({struct Cyc_PP_Doc*_tmp875=({Cyc_Absynpp_exp2doc(e2);});_tmp37A[2]=_tmp875;}),({struct Cyc_PP_Doc*_tmp874=({Cyc_PP_text(({const char*_tmp37C="]";_tag_fat(_tmp37C,sizeof(char),2U);}));});_tmp37A[3]=_tmp874;});Cyc_PP_cat(_tag_fat(_tmp37A,sizeof(struct Cyc_PP_Doc*),4U));});goto _LL0;}case 24U: _LL39: _tmp2F4=((struct Cyc_Absyn_Tuple_e_Absyn_Raw_exp_struct*)_tmp2D7)->f1;_LL3A: {struct Cyc_List_List*es=_tmp2F4;
# 1270
s=({struct Cyc_PP_Doc*_tmp37D[4U];({struct Cyc_PP_Doc*_tmp87B=({Cyc_Absynpp_dollar();});_tmp37D[0]=_tmp87B;}),({struct Cyc_PP_Doc*_tmp87A=({Cyc_PP_text(({const char*_tmp37E="(";_tag_fat(_tmp37E,sizeof(char),2U);}));});_tmp37D[1]=_tmp87A;}),({struct Cyc_PP_Doc*_tmp879=({Cyc_Absynpp_exps2doc_prec(20,es);});_tmp37D[2]=_tmp879;}),({struct Cyc_PP_Doc*_tmp878=({Cyc_PP_text(({const char*_tmp37F=")";_tag_fat(_tmp37F,sizeof(char),2U);}));});_tmp37D[3]=_tmp878;});Cyc_PP_cat(_tag_fat(_tmp37D,sizeof(struct Cyc_PP_Doc*),4U));});goto _LL0;}case 25U: _LL3B: _tmp2F2=((struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct*)_tmp2D7)->f1;_tmp2F3=((struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct*)_tmp2D7)->f2;_LL3C: {struct _tuple9*vat=_tmp2F2;struct Cyc_List_List*des=_tmp2F3;
# 1272
s=({struct Cyc_PP_Doc*_tmp380[4U];({struct Cyc_PP_Doc*_tmp880=({Cyc_PP_text(({const char*_tmp381="(";_tag_fat(_tmp381,sizeof(char),2U);}));});_tmp380[0]=_tmp880;}),({struct Cyc_PP_Doc*_tmp87F=({Cyc_Absynpp_typ2doc((*vat).f3);});_tmp380[1]=_tmp87F;}),({struct Cyc_PP_Doc*_tmp87E=({Cyc_PP_text(({const char*_tmp382=")";_tag_fat(_tmp382,sizeof(char),2U);}));});_tmp380[2]=_tmp87E;}),({
struct Cyc_PP_Doc*_tmp87D=({struct Cyc_PP_Doc*(*_tmp383)(struct _fat_ptr sep,struct Cyc_List_List*ss)=Cyc_Absynpp_group_braces;struct _fat_ptr _tmp384=({const char*_tmp385=",";_tag_fat(_tmp385,sizeof(char),2U);});struct Cyc_List_List*_tmp386=({({struct Cyc_List_List*(*_tmp87C)(struct Cyc_PP_Doc*(*f)(struct _tuple17*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp387)(struct Cyc_PP_Doc*(*f)(struct _tuple17*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_PP_Doc*(*f)(struct _tuple17*),struct Cyc_List_List*x))Cyc_List_map;_tmp387;});_tmp87C(Cyc_Absynpp_de2doc,des);});});_tmp383(_tmp384,_tmp386);});_tmp380[3]=_tmp87D;});Cyc_PP_cat(_tag_fat(_tmp380,sizeof(struct Cyc_PP_Doc*),4U));});
goto _LL0;}case 26U: _LL3D: _tmp2F1=((struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*)_tmp2D7)->f1;_LL3E: {struct Cyc_List_List*des=_tmp2F1;
s=({struct Cyc_PP_Doc*(*_tmp388)(struct _fat_ptr sep,struct Cyc_List_List*ss)=Cyc_Absynpp_group_braces;struct _fat_ptr _tmp389=({const char*_tmp38A=",";_tag_fat(_tmp38A,sizeof(char),2U);});struct Cyc_List_List*_tmp38B=({({struct Cyc_List_List*(*_tmp881)(struct Cyc_PP_Doc*(*f)(struct _tuple17*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp38C)(struct Cyc_PP_Doc*(*f)(struct _tuple17*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_PP_Doc*(*f)(struct _tuple17*),struct Cyc_List_List*x))Cyc_List_map;_tmp38C;});_tmp881(Cyc_Absynpp_de2doc,des);});});_tmp388(_tmp389,_tmp38B);});goto _LL0;}case 27U: _LL3F: _tmp2EE=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmp2D7)->f1;_tmp2EF=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmp2D7)->f2;_tmp2F0=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmp2D7)->f3;_LL40: {struct Cyc_Absyn_Vardecl*vd=_tmp2EE;struct Cyc_Absyn_Exp*e1=_tmp2EF;struct Cyc_Absyn_Exp*e2=_tmp2F0;
# 1277
s=({struct Cyc_PP_Doc*_tmp38D[8U];({struct Cyc_PP_Doc*_tmp889=({Cyc_Absynpp_lb();});_tmp38D[0]=_tmp889;}),({struct Cyc_PP_Doc*_tmp888=({Cyc_PP_text(({const char*_tmp38E="for ";_tag_fat(_tmp38E,sizeof(char),5U);}));});_tmp38D[1]=_tmp888;}),({
struct Cyc_PP_Doc*_tmp887=({Cyc_PP_text(*(*vd->name).f2);});_tmp38D[2]=_tmp887;}),({struct Cyc_PP_Doc*_tmp886=({Cyc_PP_text(({const char*_tmp38F=" < ";_tag_fat(_tmp38F,sizeof(char),4U);}));});_tmp38D[3]=_tmp886;}),({struct Cyc_PP_Doc*_tmp885=({Cyc_Absynpp_exp2doc(e1);});_tmp38D[4]=_tmp885;}),({struct Cyc_PP_Doc*_tmp884=({Cyc_PP_text(({const char*_tmp390=" : ";_tag_fat(_tmp390,sizeof(char),4U);}));});_tmp38D[5]=_tmp884;}),({
struct Cyc_PP_Doc*_tmp883=({Cyc_Absynpp_exp2doc(e2);});_tmp38D[6]=_tmp883;}),({struct Cyc_PP_Doc*_tmp882=({Cyc_Absynpp_rb();});_tmp38D[7]=_tmp882;});Cyc_PP_cat(_tag_fat(_tmp38D,sizeof(struct Cyc_PP_Doc*),8U));});
goto _LL0;}case 28U: _LL41: _tmp2EC=((struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*)_tmp2D7)->f1;_tmp2ED=(void*)((struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*)_tmp2D7)->f2;_LL42: {struct Cyc_Absyn_Exp*e=_tmp2EC;void*t=_tmp2ED;
# 1282
s=({struct Cyc_PP_Doc*_tmp391[7U];({struct Cyc_PP_Doc*_tmp890=({Cyc_Absynpp_lb();});_tmp391[0]=_tmp890;}),({struct Cyc_PP_Doc*_tmp88F=({Cyc_PP_text(({const char*_tmp392="for x ";_tag_fat(_tmp392,sizeof(char),7U);}));});_tmp391[1]=_tmp88F;}),({
struct Cyc_PP_Doc*_tmp88E=({Cyc_PP_text(({const char*_tmp393=" < ";_tag_fat(_tmp393,sizeof(char),4U);}));});_tmp391[2]=_tmp88E;}),({
struct Cyc_PP_Doc*_tmp88D=({Cyc_Absynpp_exp2doc(e);});_tmp391[3]=_tmp88D;}),({
struct Cyc_PP_Doc*_tmp88C=({Cyc_PP_text(({const char*_tmp394=" : ";_tag_fat(_tmp394,sizeof(char),4U);}));});_tmp391[4]=_tmp88C;}),({
struct Cyc_PP_Doc*_tmp88B=({Cyc_Absynpp_typ2doc(t);});_tmp391[5]=_tmp88B;}),({
struct Cyc_PP_Doc*_tmp88A=({Cyc_Absynpp_rb();});_tmp391[6]=_tmp88A;});Cyc_PP_cat(_tag_fat(_tmp391,sizeof(struct Cyc_PP_Doc*),7U));});
goto _LL0;}case 29U: _LL43: _tmp2E9=((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_tmp2D7)->f1;_tmp2EA=((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_tmp2D7)->f2;_tmp2EB=((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_tmp2D7)->f3;_LL44: {struct _tuple0*n=_tmp2E9;struct Cyc_List_List*ts=_tmp2EA;struct Cyc_List_List*des=_tmp2EB;
# 1290
struct Cyc_List_List*des_doc=({({struct Cyc_List_List*(*_tmp891)(struct Cyc_PP_Doc*(*f)(struct _tuple17*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp39B)(struct Cyc_PP_Doc*(*f)(struct _tuple17*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_PP_Doc*(*f)(struct _tuple17*),struct Cyc_List_List*x))Cyc_List_map;_tmp39B;});_tmp891(Cyc_Absynpp_de2doc,des);});});
s=({struct Cyc_PP_Doc*_tmp395[2U];({struct Cyc_PP_Doc*_tmp894=({Cyc_Absynpp_qvar2doc(n);});_tmp395[0]=_tmp894;}),({
struct Cyc_PP_Doc*_tmp893=({struct Cyc_PP_Doc*(*_tmp396)(struct _fat_ptr sep,struct Cyc_List_List*ss)=Cyc_Absynpp_group_braces;struct _fat_ptr _tmp397=({const char*_tmp398=",";_tag_fat(_tmp398,sizeof(char),2U);});struct Cyc_List_List*_tmp399=
ts != 0?({struct Cyc_List_List*_tmp39A=_cycalloc(sizeof(*_tmp39A));({struct Cyc_PP_Doc*_tmp892=({Cyc_Absynpp_tps2doc(ts);});_tmp39A->hd=_tmp892;}),_tmp39A->tl=des_doc;_tmp39A;}): des_doc;_tmp396(_tmp397,_tmp399);});
# 1292
_tmp395[1]=_tmp893;});Cyc_PP_cat(_tag_fat(_tmp395,sizeof(struct Cyc_PP_Doc*),2U));});
# 1294
goto _LL0;}case 30U: _LL45: _tmp2E8=((struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct*)_tmp2D7)->f2;_LL46: {struct Cyc_List_List*des=_tmp2E8;
s=({struct Cyc_PP_Doc*(*_tmp39C)(struct _fat_ptr sep,struct Cyc_List_List*ss)=Cyc_Absynpp_group_braces;struct _fat_ptr _tmp39D=({const char*_tmp39E=",";_tag_fat(_tmp39E,sizeof(char),2U);});struct Cyc_List_List*_tmp39F=({({struct Cyc_List_List*(*_tmp895)(struct Cyc_PP_Doc*(*f)(struct _tuple17*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp3A0)(struct Cyc_PP_Doc*(*f)(struct _tuple17*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_PP_Doc*(*f)(struct _tuple17*),struct Cyc_List_List*x))Cyc_List_map;_tmp3A0;});_tmp895(Cyc_Absynpp_de2doc,des);});});_tmp39C(_tmp39D,_tmp39F);});goto _LL0;}case 31U: _LL47: _tmp2E6=((struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*)_tmp2D7)->f1;_tmp2E7=((struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*)_tmp2D7)->f3;_LL48: {struct Cyc_List_List*es=_tmp2E6;struct Cyc_Absyn_Datatypefield*tuf=_tmp2E7;
# 1297
if(es == 0)
# 1299
s=({Cyc_Absynpp_qvar2doc(tuf->name);});else{
# 1301
s=({struct Cyc_PP_Doc*_tmp3A1[2U];({struct Cyc_PP_Doc*_tmp898=({Cyc_Absynpp_qvar2doc(tuf->name);});_tmp3A1[0]=_tmp898;}),({
struct Cyc_PP_Doc*_tmp897=({struct Cyc_PP_Doc*(*_tmp3A2)(struct _fat_ptr start,struct _fat_ptr stop,struct _fat_ptr sep,struct Cyc_List_List*l)=Cyc_PP_egroup;struct _fat_ptr _tmp3A3=({const char*_tmp3A4="(";_tag_fat(_tmp3A4,sizeof(char),2U);});struct _fat_ptr _tmp3A5=({const char*_tmp3A6=")";_tag_fat(_tmp3A6,sizeof(char),2U);});struct _fat_ptr _tmp3A7=({const char*_tmp3A8=",";_tag_fat(_tmp3A8,sizeof(char),2U);});struct Cyc_List_List*_tmp3A9=({({struct Cyc_List_List*(*_tmp896)(struct Cyc_PP_Doc*(*f)(struct Cyc_Absyn_Exp*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp3AA)(struct Cyc_PP_Doc*(*f)(struct Cyc_Absyn_Exp*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_PP_Doc*(*f)(struct Cyc_Absyn_Exp*),struct Cyc_List_List*x))Cyc_List_map;_tmp3AA;});_tmp896(Cyc_Absynpp_exp2doc,es);});});_tmp3A2(_tmp3A3,_tmp3A5,_tmp3A7,_tmp3A9);});_tmp3A1[1]=_tmp897;});Cyc_PP_cat(_tag_fat(_tmp3A1,sizeof(struct Cyc_PP_Doc*),2U));});}
goto _LL0;}case 32U: _LL49: _tmp2E5=((struct Cyc_Absyn_Enum_e_Absyn_Raw_exp_struct*)_tmp2D7)->f2;_LL4A: {struct Cyc_Absyn_Enumfield*ef=_tmp2E5;
_tmp2E4=ef;goto _LL4C;}case 33U: _LL4B: _tmp2E4=((struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct*)_tmp2D7)->f2;_LL4C: {struct Cyc_Absyn_Enumfield*ef=_tmp2E4;
s=({Cyc_Absynpp_qvar2doc(ef->name);});goto _LL0;}case 34U: _LL4D: _tmp2E2=((struct Cyc_Absyn_Spawn_e_Absyn_Raw_exp_struct*)_tmp2D7)->f1;_tmp2E3=((struct Cyc_Absyn_Spawn_e_Absyn_Raw_exp_struct*)_tmp2D7)->f2;_LL4E: {struct Cyc_Absyn_Exp*e1=_tmp2E2;struct Cyc_Absyn_Exp*e2=_tmp2E3;
# 1308
s=({struct Cyc_PP_Doc*_tmp3AB[4U];({struct Cyc_PP_Doc*_tmp89C=({Cyc_PP_text(({const char*_tmp3AC="spawn(";_tag_fat(_tmp3AC,sizeof(char),7U);}));});_tmp3AB[0]=_tmp89C;}),({struct Cyc_PP_Doc*_tmp89B=({Cyc_Absynpp_exp2doc(e1);});_tmp3AB[1]=_tmp89B;}),({struct Cyc_PP_Doc*_tmp89A=({Cyc_PP_text(({const char*_tmp3AD=") ";_tag_fat(_tmp3AD,sizeof(char),3U);}));});_tmp3AB[2]=_tmp89A;}),({struct Cyc_PP_Doc*_tmp899=({Cyc_Absynpp_exp2doc(e2);});_tmp3AB[3]=_tmp899;});Cyc_PP_cat(_tag_fat(_tmp3AB,sizeof(struct Cyc_PP_Doc*),4U));});
goto _LL0;}case 35U: _LL4F: _tmp2DD=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp2D7)->f1).is_calloc;_tmp2DE=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp2D7)->f1).rgn;_tmp2DF=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp2D7)->f1).elt_type;_tmp2E0=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp2D7)->f1).num_elts;_tmp2E1=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp2D7)->f1).inline_call;_LL50: {int is_calloc=_tmp2DD;struct Cyc_Absyn_Exp*rgnopt=_tmp2DE;void**topt=_tmp2DF;struct Cyc_Absyn_Exp*e=_tmp2E0;int inline_call=_tmp2E1;
# 1314
if(is_calloc){
# 1316
struct Cyc_Absyn_Exp*st=({Cyc_Absyn_sizeoftype_exp(*((void**)_check_null(topt)),0U);});
if(rgnopt == 0)
s=({struct Cyc_PP_Doc*_tmp3AE[5U];({struct Cyc_PP_Doc*_tmp8A1=({Cyc_PP_text(({const char*_tmp3AF="calloc(";_tag_fat(_tmp3AF,sizeof(char),8U);}));});_tmp3AE[0]=_tmp8A1;}),({struct Cyc_PP_Doc*_tmp8A0=({Cyc_Absynpp_exp2doc(e);});_tmp3AE[1]=_tmp8A0;}),({struct Cyc_PP_Doc*_tmp89F=({Cyc_PP_text(({const char*_tmp3B0=",";_tag_fat(_tmp3B0,sizeof(char),2U);}));});_tmp3AE[2]=_tmp89F;}),({struct Cyc_PP_Doc*_tmp89E=({Cyc_Absynpp_exp2doc(st);});_tmp3AE[3]=_tmp89E;}),({struct Cyc_PP_Doc*_tmp89D=({Cyc_PP_text(({const char*_tmp3B1=")";_tag_fat(_tmp3B1,sizeof(char),2U);}));});_tmp3AE[4]=_tmp89D;});Cyc_PP_cat(_tag_fat(_tmp3AE,sizeof(struct Cyc_PP_Doc*),5U));});else{
# 1320
s=({struct Cyc_PP_Doc*_tmp3B2[7U];({struct Cyc_PP_Doc*_tmp8A8=({Cyc_PP_text(({const char*_tmp3B3="rcalloc(";_tag_fat(_tmp3B3,sizeof(char),9U);}));});_tmp3B2[0]=_tmp8A8;}),({struct Cyc_PP_Doc*_tmp8A7=({Cyc_Absynpp_exp2doc(rgnopt);});_tmp3B2[1]=_tmp8A7;}),({struct Cyc_PP_Doc*_tmp8A6=({Cyc_PP_text(({const char*_tmp3B4=",";_tag_fat(_tmp3B4,sizeof(char),2U);}));});_tmp3B2[2]=_tmp8A6;}),({
struct Cyc_PP_Doc*_tmp8A5=({Cyc_Absynpp_exp2doc(e);});_tmp3B2[3]=_tmp8A5;}),({struct Cyc_PP_Doc*_tmp8A4=({Cyc_PP_text(({const char*_tmp3B5=",";_tag_fat(_tmp3B5,sizeof(char),2U);}));});_tmp3B2[4]=_tmp8A4;}),({struct Cyc_PP_Doc*_tmp8A3=({Cyc_Absynpp_exp2doc(st);});_tmp3B2[5]=_tmp8A3;}),({struct Cyc_PP_Doc*_tmp8A2=({Cyc_PP_text(({const char*_tmp3B6=")";_tag_fat(_tmp3B6,sizeof(char),2U);}));});_tmp3B2[6]=_tmp8A2;});Cyc_PP_cat(_tag_fat(_tmp3B2,sizeof(struct Cyc_PP_Doc*),7U));});}}else{
# 1323
struct Cyc_Absyn_Exp*new_e;
# 1325
if(topt == 0)
new_e=e;else{
# 1328
new_e=({struct Cyc_Absyn_Exp*(*_tmp3B7)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_times_exp;struct Cyc_Absyn_Exp*_tmp3B8=({Cyc_Absyn_sizeoftype_exp(*topt,0U);});struct Cyc_Absyn_Exp*_tmp3B9=e;unsigned _tmp3BA=0U;_tmp3B7(_tmp3B8,_tmp3B9,_tmp3BA);});}
# 1330
if(rgnopt == 0)
s=({struct Cyc_PP_Doc*_tmp3BB[3U];({struct Cyc_PP_Doc*_tmp8AB=({Cyc_PP_text(({const char*_tmp3BC="malloc(";_tag_fat(_tmp3BC,sizeof(char),8U);}));});_tmp3BB[0]=_tmp8AB;}),({struct Cyc_PP_Doc*_tmp8AA=({Cyc_Absynpp_exp2doc(new_e);});_tmp3BB[1]=_tmp8AA;}),({struct Cyc_PP_Doc*_tmp8A9=({Cyc_PP_text(({const char*_tmp3BD=")";_tag_fat(_tmp3BD,sizeof(char),2U);}));});_tmp3BB[2]=_tmp8A9;});Cyc_PP_cat(_tag_fat(_tmp3BB,sizeof(struct Cyc_PP_Doc*),3U));});else{
# 1333
if(inline_call)
s=({struct Cyc_PP_Doc*_tmp3BE[5U];({struct Cyc_PP_Doc*_tmp8B0=({Cyc_PP_text(({const char*_tmp3BF="rmalloc_inline(";_tag_fat(_tmp3BF,sizeof(char),16U);}));});_tmp3BE[0]=_tmp8B0;}),({struct Cyc_PP_Doc*_tmp8AF=({Cyc_Absynpp_exp2doc(rgnopt);});_tmp3BE[1]=_tmp8AF;}),({struct Cyc_PP_Doc*_tmp8AE=({Cyc_PP_text(({const char*_tmp3C0=",";_tag_fat(_tmp3C0,sizeof(char),2U);}));});_tmp3BE[2]=_tmp8AE;}),({
struct Cyc_PP_Doc*_tmp8AD=({Cyc_Absynpp_exp2doc(new_e);});_tmp3BE[3]=_tmp8AD;}),({struct Cyc_PP_Doc*_tmp8AC=({Cyc_PP_text(({const char*_tmp3C1=")";_tag_fat(_tmp3C1,sizeof(char),2U);}));});_tmp3BE[4]=_tmp8AC;});Cyc_PP_cat(_tag_fat(_tmp3BE,sizeof(struct Cyc_PP_Doc*),5U));});else{
# 1337
s=({struct Cyc_PP_Doc*_tmp3C2[5U];({struct Cyc_PP_Doc*_tmp8B5=({Cyc_PP_text(({const char*_tmp3C3="rmalloc(";_tag_fat(_tmp3C3,sizeof(char),9U);}));});_tmp3C2[0]=_tmp8B5;}),({struct Cyc_PP_Doc*_tmp8B4=({Cyc_Absynpp_exp2doc(rgnopt);});_tmp3C2[1]=_tmp8B4;}),({struct Cyc_PP_Doc*_tmp8B3=({Cyc_PP_text(({const char*_tmp3C4=",";_tag_fat(_tmp3C4,sizeof(char),2U);}));});_tmp3C2[2]=_tmp8B3;}),({
struct Cyc_PP_Doc*_tmp8B2=({Cyc_Absynpp_exp2doc(new_e);});_tmp3C2[3]=_tmp8B2;}),({struct Cyc_PP_Doc*_tmp8B1=({Cyc_PP_text(({const char*_tmp3C5=")";_tag_fat(_tmp3C5,sizeof(char),2U);}));});_tmp3C2[4]=_tmp8B1;});Cyc_PP_cat(_tag_fat(_tmp3C2,sizeof(struct Cyc_PP_Doc*),5U));});}}}
# 1341
goto _LL0;}case 36U: _LL51: _tmp2DB=((struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct*)_tmp2D7)->f1;_tmp2DC=((struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct*)_tmp2D7)->f2;_LL52: {struct Cyc_Absyn_Exp*e1=_tmp2DB;struct Cyc_Absyn_Exp*e2=_tmp2DC;
# 1343
s=({struct Cyc_PP_Doc*_tmp3C6[3U];({struct Cyc_PP_Doc*_tmp8B8=({Cyc_Absynpp_exp2doc_prec(myprec,e1);});_tmp3C6[0]=_tmp8B8;}),({struct Cyc_PP_Doc*_tmp8B7=({Cyc_PP_text(({const char*_tmp3C7=" :=: ";_tag_fat(_tmp3C7,sizeof(char),6U);}));});_tmp3C6[1]=_tmp8B7;}),({struct Cyc_PP_Doc*_tmp8B6=({Cyc_Absynpp_exp2doc_prec(myprec,e2);});_tmp3C6[2]=_tmp8B6;});Cyc_PP_cat(_tag_fat(_tmp3C6,sizeof(struct Cyc_PP_Doc*),3U));});
goto _LL0;}case 37U: _LL53: _tmp2D9=((struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*)_tmp2D7)->f1;_tmp2DA=((struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*)_tmp2D7)->f2;_LL54: {struct Cyc_Core_Opt*n=_tmp2D9;struct Cyc_List_List*des=_tmp2DA;
# 1346
s=({struct Cyc_PP_Doc*(*_tmp3C8)(struct _fat_ptr sep,struct Cyc_List_List*ss)=Cyc_Absynpp_group_braces;struct _fat_ptr _tmp3C9=({const char*_tmp3CA=",";_tag_fat(_tmp3CA,sizeof(char),2U);});struct Cyc_List_List*_tmp3CB=({({struct Cyc_List_List*(*_tmp8B9)(struct Cyc_PP_Doc*(*f)(struct _tuple17*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp3CC)(struct Cyc_PP_Doc*(*f)(struct _tuple17*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_PP_Doc*(*f)(struct _tuple17*),struct Cyc_List_List*x))Cyc_List_map;_tmp3CC;});_tmp8B9(Cyc_Absynpp_de2doc,des);});});_tmp3C8(_tmp3C9,_tmp3CB);});
goto _LL0;}default: _LL55: _tmp2D8=((struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct*)_tmp2D7)->f1;_LL56: {struct Cyc_Absyn_Stmt*s2=_tmp2D8;
# 1349
s=({struct Cyc_PP_Doc*_tmp3CD[7U];({struct Cyc_PP_Doc*_tmp8C0=({Cyc_PP_text(({const char*_tmp3CE="(";_tag_fat(_tmp3CE,sizeof(char),2U);}));});_tmp3CD[0]=_tmp8C0;}),({struct Cyc_PP_Doc*_tmp8BF=({Cyc_Absynpp_lb();});_tmp3CD[1]=_tmp8BF;}),({struct Cyc_PP_Doc*_tmp8BE=({Cyc_PP_blank_doc();});_tmp3CD[2]=_tmp8BE;}),({
struct Cyc_PP_Doc*_tmp8BD=({struct Cyc_PP_Doc*(*_tmp3CF)(int k,struct Cyc_PP_Doc*d)=Cyc_PP_nest;int _tmp3D0=2;struct Cyc_PP_Doc*_tmp3D1=({Cyc_Absynpp_stmt2doc(s2,1,0);});_tmp3CF(_tmp3D0,_tmp3D1);});_tmp3CD[3]=_tmp8BD;}),({
struct Cyc_PP_Doc*_tmp8BC=({Cyc_PP_blank_doc();});_tmp3CD[4]=_tmp8BC;}),({struct Cyc_PP_Doc*_tmp8BB=({Cyc_Absynpp_rb();});_tmp3CD[5]=_tmp8BB;}),({struct Cyc_PP_Doc*_tmp8BA=({Cyc_PP_text(({const char*_tmp3D2=")";_tag_fat(_tmp3D2,sizeof(char),2U);}));});_tmp3CD[6]=_tmp8BA;});Cyc_PP_cat(_tag_fat(_tmp3CD,sizeof(struct Cyc_PP_Doc*),7U));});
goto _LL0;}}_LL0:;}
# 1354
if(inprec >= myprec)
s=({struct Cyc_PP_Doc*_tmp3D3[3U];({struct Cyc_PP_Doc*_tmp8C2=({Cyc_PP_text(({const char*_tmp3D4="(";_tag_fat(_tmp3D4,sizeof(char),2U);}));});_tmp3D3[0]=_tmp8C2;}),_tmp3D3[1]=s,({struct Cyc_PP_Doc*_tmp8C1=({Cyc_PP_text(({const char*_tmp3D5=")";_tag_fat(_tmp3D5,sizeof(char),2U);}));});_tmp3D3[2]=_tmp8C1;});Cyc_PP_cat(_tag_fat(_tmp3D3,sizeof(struct Cyc_PP_Doc*),3U));});
# 1354
return s;}
# 1161
struct Cyc_PP_Doc*Cyc_Absynpp_designator2doc(void*d){
# 1360
void*_tmp3D7=d;struct _fat_ptr*_tmp3D8;struct Cyc_Absyn_Exp*_tmp3D9;if(((struct Cyc_Absyn_ArrayElement_Absyn_Designator_struct*)_tmp3D7)->tag == 0U){_LL1: _tmp3D9=((struct Cyc_Absyn_ArrayElement_Absyn_Designator_struct*)_tmp3D7)->f1;_LL2: {struct Cyc_Absyn_Exp*e=_tmp3D9;
return({struct Cyc_PP_Doc*_tmp3DA[3U];({struct Cyc_PP_Doc*_tmp8C5=({Cyc_PP_text(({const char*_tmp3DB=".[";_tag_fat(_tmp3DB,sizeof(char),3U);}));});_tmp3DA[0]=_tmp8C5;}),({struct Cyc_PP_Doc*_tmp8C4=({Cyc_Absynpp_exp2doc(e);});_tmp3DA[1]=_tmp8C4;}),({struct Cyc_PP_Doc*_tmp8C3=({Cyc_PP_text(({const char*_tmp3DC="]";_tag_fat(_tmp3DC,sizeof(char),2U);}));});_tmp3DA[2]=_tmp8C3;});Cyc_PP_cat(_tag_fat(_tmp3DA,sizeof(struct Cyc_PP_Doc*),3U));});}}else{_LL3: _tmp3D8=((struct Cyc_Absyn_FieldName_Absyn_Designator_struct*)_tmp3D7)->f1;_LL4: {struct _fat_ptr*v=_tmp3D8;
return({struct Cyc_PP_Doc*_tmp3DD[2U];({struct Cyc_PP_Doc*_tmp8C7=({Cyc_PP_text(({const char*_tmp3DE=".";_tag_fat(_tmp3DE,sizeof(char),2U);}));});_tmp3DD[0]=_tmp8C7;}),({struct Cyc_PP_Doc*_tmp8C6=({Cyc_PP_textptr(v);});_tmp3DD[1]=_tmp8C6;});Cyc_PP_cat(_tag_fat(_tmp3DD,sizeof(struct Cyc_PP_Doc*),2U));});}}_LL0:;}
# 1161
struct Cyc_PP_Doc*Cyc_Absynpp_de2doc(struct _tuple17*de){
# 1367
if((*de).f1 == 0)return({Cyc_Absynpp_exp2doc((*de).f2);});else{
return({struct Cyc_PP_Doc*_tmp3E0[2U];({struct Cyc_PP_Doc*_tmp8CA=({struct Cyc_PP_Doc*(*_tmp3E1)(struct _fat_ptr start,struct _fat_ptr stop,struct _fat_ptr sep,struct Cyc_List_List*l)=Cyc_PP_egroup;struct _fat_ptr _tmp3E2=({const char*_tmp3E3="";_tag_fat(_tmp3E3,sizeof(char),1U);});struct _fat_ptr _tmp3E4=({const char*_tmp3E5="=";_tag_fat(_tmp3E5,sizeof(char),2U);});struct _fat_ptr _tmp3E6=({const char*_tmp3E7="=";_tag_fat(_tmp3E7,sizeof(char),2U);});struct Cyc_List_List*_tmp3E8=({({struct Cyc_List_List*(*_tmp8C9)(struct Cyc_PP_Doc*(*f)(void*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp3E9)(struct Cyc_PP_Doc*(*f)(void*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_PP_Doc*(*f)(void*),struct Cyc_List_List*x))Cyc_List_map;_tmp3E9;});_tmp8C9(Cyc_Absynpp_designator2doc,(*de).f1);});});_tmp3E1(_tmp3E2,_tmp3E4,_tmp3E6,_tmp3E8);});_tmp3E0[0]=_tmp8CA;}),({
struct Cyc_PP_Doc*_tmp8C8=({Cyc_Absynpp_exp2doc((*de).f2);});_tmp3E0[1]=_tmp8C8;});Cyc_PP_cat(_tag_fat(_tmp3E0,sizeof(struct Cyc_PP_Doc*),2U));});}}
# 1161
struct Cyc_PP_Doc*Cyc_Absynpp_exps2doc_prec(int inprec,struct Cyc_List_List*es){
# 1373
return({struct Cyc_PP_Doc*(*_tmp3EB)(struct _fat_ptr start,struct _fat_ptr stop,struct _fat_ptr sep,struct Cyc_List_List*l)=Cyc_PP_group;struct _fat_ptr _tmp3EC=({const char*_tmp3ED="";_tag_fat(_tmp3ED,sizeof(char),1U);});struct _fat_ptr _tmp3EE=({const char*_tmp3EF="";_tag_fat(_tmp3EF,sizeof(char),1U);});struct _fat_ptr _tmp3F0=({const char*_tmp3F1=",";_tag_fat(_tmp3F1,sizeof(char),2U);});struct Cyc_List_List*_tmp3F2=({({struct Cyc_List_List*(*_tmp8CC)(struct Cyc_PP_Doc*(*f)(int,struct Cyc_Absyn_Exp*),int env,struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp3F3)(struct Cyc_PP_Doc*(*f)(int,struct Cyc_Absyn_Exp*),int env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_PP_Doc*(*f)(int,struct Cyc_Absyn_Exp*),int env,struct Cyc_List_List*x))Cyc_List_map_c;_tmp3F3;});int _tmp8CB=inprec;_tmp8CC(Cyc_Absynpp_exp2doc_prec,_tmp8CB,es);});});_tmp3EB(_tmp3EC,_tmp3EE,_tmp3F0,_tmp3F2);});}
# 1161
struct _fat_ptr Cyc_Absynpp_longlong2string(unsigned long long i){
# 1377
struct _fat_ptr x=({char*_tmp409=({unsigned _tmp408=28U + 1U;char*_tmp407=_cycalloc_atomic(_check_times(_tmp408,sizeof(char)));({{unsigned _tmp73B=28U;unsigned i;for(i=0;i < _tmp73B;++ i){_tmp407[i]='z';}_tmp407[_tmp73B]=0;}0;});_tmp407;});_tag_fat(_tmp409,sizeof(char),29U);});
({struct _fat_ptr _tmp3F5=_fat_ptr_plus(x,sizeof(char),27);char _tmp3F6=*((char*)_check_fat_subscript(_tmp3F5,sizeof(char),0U));char _tmp3F7='\000';if(_get_fat_size(_tmp3F5,sizeof(char))== 1U &&(_tmp3F6 == 0 && _tmp3F7 != 0))_throw_arraybounds();*((char*)_tmp3F5.curr)=_tmp3F7;});
({struct _fat_ptr _tmp3F8=_fat_ptr_plus(x,sizeof(char),26);char _tmp3F9=*((char*)_check_fat_subscript(_tmp3F8,sizeof(char),0U));char _tmp3FA='L';if(_get_fat_size(_tmp3F8,sizeof(char))== 1U &&(_tmp3F9 == 0 && _tmp3FA != 0))_throw_arraybounds();*((char*)_tmp3F8.curr)=_tmp3FA;});
({struct _fat_ptr _tmp3FB=_fat_ptr_plus(x,sizeof(char),25);char _tmp3FC=*((char*)_check_fat_subscript(_tmp3FB,sizeof(char),0U));char _tmp3FD='L';if(_get_fat_size(_tmp3FB,sizeof(char))== 1U &&(_tmp3FC == 0 && _tmp3FD != 0))_throw_arraybounds();*((char*)_tmp3FB.curr)=_tmp3FD;});
({struct _fat_ptr _tmp3FE=_fat_ptr_plus(x,sizeof(char),24);char _tmp3FF=*((char*)_check_fat_subscript(_tmp3FE,sizeof(char),0U));char _tmp400='U';if(_get_fat_size(_tmp3FE,sizeof(char))== 1U &&(_tmp3FF == 0 && _tmp400 != 0))_throw_arraybounds();*((char*)_tmp3FE.curr)=_tmp400;});
({struct _fat_ptr _tmp401=_fat_ptr_plus(x,sizeof(char),23);char _tmp402=*((char*)_check_fat_subscript(_tmp401,sizeof(char),0U));char _tmp403='0';if(_get_fat_size(_tmp401,sizeof(char))== 1U &&(_tmp402 == 0 && _tmp403 != 0))_throw_arraybounds();*((char*)_tmp401.curr)=_tmp403;});{
int index=23;
while(i != (unsigned long long)0){
char c=(char)((unsigned long long)'0' + i % (unsigned long long)10);
({struct _fat_ptr _tmp404=_fat_ptr_plus(x,sizeof(char),index);char _tmp405=*((char*)_check_fat_subscript(_tmp404,sizeof(char),0U));char _tmp406=c;if(_get_fat_size(_tmp404,sizeof(char))== 1U &&(_tmp405 == 0 && _tmp406 != 0))_throw_arraybounds();*((char*)_tmp404.curr)=_tmp406;});
i=i / (unsigned long long)10;
-- index;}
# 1390
return(struct _fat_ptr)_fat_ptr_plus(_fat_ptr_plus(x,sizeof(char),index),sizeof(char),1);}}
# 1161
struct Cyc_PP_Doc*Cyc_Absynpp_cnst2doc(union Cyc_Absyn_Cnst c){
# 1395
union Cyc_Absyn_Cnst _tmp40B=c;struct _fat_ptr _tmp40C;struct _fat_ptr _tmp40D;struct _fat_ptr _tmp40E;long long _tmp410;enum Cyc_Absyn_Sign _tmp40F;int _tmp412;enum Cyc_Absyn_Sign _tmp411;short _tmp414;enum Cyc_Absyn_Sign _tmp413;struct _fat_ptr _tmp415;char _tmp417;enum Cyc_Absyn_Sign _tmp416;switch((_tmp40B.String_c).tag){case 2U: _LL1: _tmp416=((_tmp40B.Char_c).val).f1;_tmp417=((_tmp40B.Char_c).val).f2;_LL2: {enum Cyc_Absyn_Sign sg=_tmp416;char ch=_tmp417;
return({struct Cyc_PP_Doc*(*_tmp418)(struct _fat_ptr s)=Cyc_PP_text;struct _fat_ptr _tmp419=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp41C=({struct Cyc_String_pa_PrintArg_struct _tmp73C;_tmp73C.tag=0U,({struct _fat_ptr _tmp8CD=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_char_escape(ch);}));_tmp73C.f1=_tmp8CD;});_tmp73C;});void*_tmp41A[1U];_tmp41A[0]=& _tmp41C;({struct _fat_ptr _tmp8CE=({const char*_tmp41B="'%s'";_tag_fat(_tmp41B,sizeof(char),5U);});Cyc_aprintf(_tmp8CE,_tag_fat(_tmp41A,sizeof(void*),1U));});});_tmp418(_tmp419);});}case 3U: _LL3: _tmp415=(_tmp40B.Wchar_c).val;_LL4: {struct _fat_ptr s=_tmp415;
return({struct Cyc_PP_Doc*(*_tmp41D)(struct _fat_ptr s)=Cyc_PP_text;struct _fat_ptr _tmp41E=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp421=({struct Cyc_String_pa_PrintArg_struct _tmp73D;_tmp73D.tag=0U,_tmp73D.f1=(struct _fat_ptr)((struct _fat_ptr)s);_tmp73D;});void*_tmp41F[1U];_tmp41F[0]=& _tmp421;({struct _fat_ptr _tmp8CF=({const char*_tmp420="L'%s'";_tag_fat(_tmp420,sizeof(char),6U);});Cyc_aprintf(_tmp8CF,_tag_fat(_tmp41F,sizeof(void*),1U));});});_tmp41D(_tmp41E);});}case 4U: _LL5: _tmp413=((_tmp40B.Short_c).val).f1;_tmp414=((_tmp40B.Short_c).val).f2;_LL6: {enum Cyc_Absyn_Sign sg=_tmp413;short s=_tmp414;
return({struct Cyc_PP_Doc*(*_tmp422)(struct _fat_ptr s)=Cyc_PP_text;struct _fat_ptr _tmp423=(struct _fat_ptr)({struct Cyc_Int_pa_PrintArg_struct _tmp426=({struct Cyc_Int_pa_PrintArg_struct _tmp73E;_tmp73E.tag=1U,_tmp73E.f1=(unsigned long)((int)s);_tmp73E;});void*_tmp424[1U];_tmp424[0]=& _tmp426;({struct _fat_ptr _tmp8D0=({const char*_tmp425="%d";_tag_fat(_tmp425,sizeof(char),3U);});Cyc_aprintf(_tmp8D0,_tag_fat(_tmp424,sizeof(void*),1U));});});_tmp422(_tmp423);});}case 5U: _LL7: _tmp411=((_tmp40B.Int_c).val).f1;_tmp412=((_tmp40B.Int_c).val).f2;_LL8: {enum Cyc_Absyn_Sign sn=_tmp411;int i=_tmp412;
# 1400
if((int)sn == (int)1U)return({struct Cyc_PP_Doc*(*_tmp427)(struct _fat_ptr s)=Cyc_PP_text;struct _fat_ptr _tmp428=(struct _fat_ptr)({struct Cyc_Int_pa_PrintArg_struct _tmp42B=({struct Cyc_Int_pa_PrintArg_struct _tmp73F;_tmp73F.tag=1U,_tmp73F.f1=(unsigned)i;_tmp73F;});void*_tmp429[1U];_tmp429[0]=& _tmp42B;({struct _fat_ptr _tmp8D1=({const char*_tmp42A="%uU";_tag_fat(_tmp42A,sizeof(char),4U);});Cyc_aprintf(_tmp8D1,_tag_fat(_tmp429,sizeof(void*),1U));});});_tmp427(_tmp428);});else{
return({struct Cyc_PP_Doc*(*_tmp42C)(struct _fat_ptr s)=Cyc_PP_text;struct _fat_ptr _tmp42D=(struct _fat_ptr)({struct Cyc_Int_pa_PrintArg_struct _tmp430=({struct Cyc_Int_pa_PrintArg_struct _tmp740;_tmp740.tag=1U,_tmp740.f1=(unsigned long)i;_tmp740;});void*_tmp42E[1U];_tmp42E[0]=& _tmp430;({struct _fat_ptr _tmp8D2=({const char*_tmp42F="%d";_tag_fat(_tmp42F,sizeof(char),3U);});Cyc_aprintf(_tmp8D2,_tag_fat(_tmp42E,sizeof(void*),1U));});});_tmp42C(_tmp42D);});}}case 6U: _LL9: _tmp40F=((_tmp40B.LongLong_c).val).f1;_tmp410=((_tmp40B.LongLong_c).val).f2;_LLA: {enum Cyc_Absyn_Sign sg=_tmp40F;long long i=_tmp410;
# 1404
return({struct Cyc_PP_Doc*(*_tmp431)(struct _fat_ptr s)=Cyc_PP_text;struct _fat_ptr _tmp432=({Cyc_Absynpp_longlong2string((unsigned long long)i);});_tmp431(_tmp432);});}case 7U: _LLB: _tmp40E=((_tmp40B.Float_c).val).f1;_LLC: {struct _fat_ptr x=_tmp40E;
return({Cyc_PP_text(x);});}case 1U: _LLD: _LLE:
 return({Cyc_PP_text(({const char*_tmp433="NULL";_tag_fat(_tmp433,sizeof(char),5U);}));});case 8U: _LLF: _tmp40D=(_tmp40B.String_c).val;_LL10: {struct _fat_ptr s=_tmp40D;
return({struct Cyc_PP_Doc*_tmp434[3U];({struct Cyc_PP_Doc*_tmp8D5=({Cyc_PP_text(({const char*_tmp435="\"";_tag_fat(_tmp435,sizeof(char),2U);}));});_tmp434[0]=_tmp8D5;}),({struct Cyc_PP_Doc*_tmp8D4=({struct Cyc_PP_Doc*(*_tmp436)(struct _fat_ptr s)=Cyc_PP_text;struct _fat_ptr _tmp437=({Cyc_Absynpp_string_escape(s);});_tmp436(_tmp437);});_tmp434[1]=_tmp8D4;}),({struct Cyc_PP_Doc*_tmp8D3=({Cyc_PP_text(({const char*_tmp438="\"";_tag_fat(_tmp438,sizeof(char),2U);}));});_tmp434[2]=_tmp8D3;});Cyc_PP_cat(_tag_fat(_tmp434,sizeof(struct Cyc_PP_Doc*),3U));});}default: _LL11: _tmp40C=(_tmp40B.Wstring_c).val;_LL12: {struct _fat_ptr s=_tmp40C;
return({struct Cyc_PP_Doc*_tmp439[3U];({struct Cyc_PP_Doc*_tmp8D8=({Cyc_PP_text(({const char*_tmp43A="L\"";_tag_fat(_tmp43A,sizeof(char),3U);}));});_tmp439[0]=_tmp8D8;}),({struct Cyc_PP_Doc*_tmp8D7=({Cyc_PP_text(s);});_tmp439[1]=_tmp8D7;}),({struct Cyc_PP_Doc*_tmp8D6=({Cyc_PP_text(({const char*_tmp43B="\"";_tag_fat(_tmp43B,sizeof(char),2U);}));});_tmp439[2]=_tmp8D6;});Cyc_PP_cat(_tag_fat(_tmp439,sizeof(struct Cyc_PP_Doc*),3U));});}}_LL0:;}
# 1161
struct Cyc_PP_Doc*Cyc_Absynpp_primapp2doc(int inprec,enum Cyc_Absyn_Primop p,struct Cyc_List_List*es){
# 1413
struct Cyc_PP_Doc*ps=({Cyc_Absynpp_prim2doc(p);});
if((int)p == (int)18U){
if(es == 0 || es->tl != 0)
(int)_throw(({struct Cyc_Core_Failure_exn_struct*_tmp440=_cycalloc(sizeof(*_tmp440));_tmp440->tag=Cyc_Core_Failure,({struct _fat_ptr _tmp8DB=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp43F=({struct Cyc_String_pa_PrintArg_struct _tmp741;_tmp741.tag=0U,({struct _fat_ptr _tmp8D9=(struct _fat_ptr)((struct _fat_ptr)({Cyc_PP_string_of_doc(ps,72);}));_tmp741.f1=_tmp8D9;});_tmp741;});void*_tmp43D[1U];_tmp43D[0]=& _tmp43F;({struct _fat_ptr _tmp8DA=({const char*_tmp43E="Absynpp::primapp2doc Numelts: %s with bad args";_tag_fat(_tmp43E,sizeof(char),47U);});Cyc_aprintf(_tmp8DA,_tag_fat(_tmp43D,sizeof(void*),1U));});});_tmp440->f1=_tmp8DB;});_tmp440;}));
# 1415
return({struct Cyc_PP_Doc*_tmp441[3U];({
# 1418
struct Cyc_PP_Doc*_tmp8DE=({Cyc_PP_text(({const char*_tmp442="numelts(";_tag_fat(_tmp442,sizeof(char),9U);}));});_tmp441[0]=_tmp8DE;}),({struct Cyc_PP_Doc*_tmp8DD=({Cyc_Absynpp_exp2doc((struct Cyc_Absyn_Exp*)es->hd);});_tmp441[1]=_tmp8DD;}),({struct Cyc_PP_Doc*_tmp8DC=({Cyc_PP_text(({const char*_tmp443=")";_tag_fat(_tmp443,sizeof(char),2U);}));});_tmp441[2]=_tmp8DC;});Cyc_PP_cat(_tag_fat(_tmp441,sizeof(struct Cyc_PP_Doc*),3U));});}else{
# 1420
struct Cyc_List_List*ds=({({struct Cyc_List_List*(*_tmp8E0)(struct Cyc_PP_Doc*(*f)(int,struct Cyc_Absyn_Exp*),int env,struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp451)(struct Cyc_PP_Doc*(*f)(int,struct Cyc_Absyn_Exp*),int env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_PP_Doc*(*f)(int,struct Cyc_Absyn_Exp*),int env,struct Cyc_List_List*x))Cyc_List_map_c;_tmp451;});int _tmp8DF=inprec;_tmp8E0(Cyc_Absynpp_exp2doc_prec,_tmp8DF,es);});});
if(ds == 0)
(int)_throw(({struct Cyc_Core_Failure_exn_struct*_tmp447=_cycalloc(sizeof(*_tmp447));_tmp447->tag=Cyc_Core_Failure,({struct _fat_ptr _tmp8E3=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp446=({struct Cyc_String_pa_PrintArg_struct _tmp742;_tmp742.tag=0U,({struct _fat_ptr _tmp8E1=(struct _fat_ptr)((struct _fat_ptr)({Cyc_PP_string_of_doc(ps,72);}));_tmp742.f1=_tmp8E1;});_tmp742;});void*_tmp444[1U];_tmp444[0]=& _tmp446;({struct _fat_ptr _tmp8E2=({const char*_tmp445="Absynpp::primapp2doc: %s with no args";_tag_fat(_tmp445,sizeof(char),38U);});Cyc_aprintf(_tmp8E2,_tag_fat(_tmp444,sizeof(void*),1U));});});_tmp447->f1=_tmp8E3;});_tmp447;}));else{
# 1424
if(ds->tl == 0)
return({struct Cyc_PP_Doc*_tmp448[3U];_tmp448[0]=ps,({struct Cyc_PP_Doc*_tmp8E4=({Cyc_PP_text(({const char*_tmp449=" ";_tag_fat(_tmp449,sizeof(char),2U);}));});_tmp448[1]=_tmp8E4;}),_tmp448[2]=(struct Cyc_PP_Doc*)ds->hd;Cyc_PP_cat(_tag_fat(_tmp448,sizeof(struct Cyc_PP_Doc*),3U));});else{
if(((struct Cyc_List_List*)_check_null(ds->tl))->tl != 0)
(int)_throw(({struct Cyc_Core_Failure_exn_struct*_tmp44D=_cycalloc(sizeof(*_tmp44D));_tmp44D->tag=Cyc_Core_Failure,({struct _fat_ptr _tmp8E7=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp44C=({struct Cyc_String_pa_PrintArg_struct _tmp743;_tmp743.tag=0U,({struct _fat_ptr _tmp8E5=(struct _fat_ptr)((struct _fat_ptr)({Cyc_PP_string_of_doc(ps,72);}));_tmp743.f1=_tmp8E5;});_tmp743;});void*_tmp44A[1U];_tmp44A[0]=& _tmp44C;({struct _fat_ptr _tmp8E6=({const char*_tmp44B="Absynpp::primapp2doc: %s with more than 2 args";_tag_fat(_tmp44B,sizeof(char),47U);});Cyc_aprintf(_tmp8E6,_tag_fat(_tmp44A,sizeof(void*),1U));});});_tmp44D->f1=_tmp8E7;});_tmp44D;}));else{
# 1430
return({struct Cyc_PP_Doc*_tmp44E[5U];_tmp44E[0]=(struct Cyc_PP_Doc*)ds->hd,({struct Cyc_PP_Doc*_tmp8E9=({Cyc_PP_text(({const char*_tmp44F=" ";_tag_fat(_tmp44F,sizeof(char),2U);}));});_tmp44E[1]=_tmp8E9;}),_tmp44E[2]=ps,({struct Cyc_PP_Doc*_tmp8E8=({Cyc_PP_text(({const char*_tmp450=" ";_tag_fat(_tmp450,sizeof(char),2U);}));});_tmp44E[3]=_tmp8E8;}),_tmp44E[4]=(struct Cyc_PP_Doc*)((struct Cyc_List_List*)_check_null(ds->tl))->hd;Cyc_PP_cat(_tag_fat(_tmp44E,sizeof(struct Cyc_PP_Doc*),5U));});}}}}}
# 1161
struct _fat_ptr Cyc_Absynpp_prim2str(enum Cyc_Absyn_Primop p){
# 1435
enum Cyc_Absyn_Primop _tmp453=p;switch(_tmp453){case Cyc_Absyn_Plus: _LL1: _LL2:
 return({const char*_tmp454="+";_tag_fat(_tmp454,sizeof(char),2U);});case Cyc_Absyn_Times: _LL3: _LL4:
 return({const char*_tmp455="*";_tag_fat(_tmp455,sizeof(char),2U);});case Cyc_Absyn_Minus: _LL5: _LL6:
 return({const char*_tmp456="-";_tag_fat(_tmp456,sizeof(char),2U);});case Cyc_Absyn_Div: _LL7: _LL8:
 return({const char*_tmp457="/";_tag_fat(_tmp457,sizeof(char),2U);});case Cyc_Absyn_Mod: _LL9: _LLA:
 return Cyc_Absynpp_print_for_cycdoc?({const char*_tmp458="\\%";_tag_fat(_tmp458,sizeof(char),3U);}):({const char*_tmp459="%";_tag_fat(_tmp459,sizeof(char),2U);});case Cyc_Absyn_Eq: _LLB: _LLC:
 return({const char*_tmp45A="==";_tag_fat(_tmp45A,sizeof(char),3U);});case Cyc_Absyn_Neq: _LLD: _LLE:
 return({const char*_tmp45B="!=";_tag_fat(_tmp45B,sizeof(char),3U);});case Cyc_Absyn_Gt: _LLF: _LL10:
 return({const char*_tmp45C=">";_tag_fat(_tmp45C,sizeof(char),2U);});case Cyc_Absyn_Lt: _LL11: _LL12:
 return({const char*_tmp45D="<";_tag_fat(_tmp45D,sizeof(char),2U);});case Cyc_Absyn_Gte: _LL13: _LL14:
 return({const char*_tmp45E=">=";_tag_fat(_tmp45E,sizeof(char),3U);});case Cyc_Absyn_Lte: _LL15: _LL16:
 return({const char*_tmp45F="<=";_tag_fat(_tmp45F,sizeof(char),3U);});case Cyc_Absyn_Not: _LL17: _LL18:
 return({const char*_tmp460="!";_tag_fat(_tmp460,sizeof(char),2U);});case Cyc_Absyn_Bitnot: _LL19: _LL1A:
 return({const char*_tmp461="~";_tag_fat(_tmp461,sizeof(char),2U);});case Cyc_Absyn_Bitand: _LL1B: _LL1C:
 return({const char*_tmp462="&";_tag_fat(_tmp462,sizeof(char),2U);});case Cyc_Absyn_Bitor: _LL1D: _LL1E:
 return({const char*_tmp463="|";_tag_fat(_tmp463,sizeof(char),2U);});case Cyc_Absyn_Bitxor: _LL1F: _LL20:
 return({const char*_tmp464="^";_tag_fat(_tmp464,sizeof(char),2U);});case Cyc_Absyn_Bitlshift: _LL21: _LL22:
 return({const char*_tmp465="<<";_tag_fat(_tmp465,sizeof(char),3U);});case Cyc_Absyn_Bitlrshift: _LL23: _LL24:
 return({const char*_tmp466=">>";_tag_fat(_tmp466,sizeof(char),3U);});case Cyc_Absyn_Numelts: _LL25: _LL26:
 return({const char*_tmp467="numelts";_tag_fat(_tmp467,sizeof(char),8U);});default: _LL27: _LL28:
 return({const char*_tmp468="?";_tag_fat(_tmp468,sizeof(char),2U);});}_LL0:;}
# 1161
struct Cyc_PP_Doc*Cyc_Absynpp_prim2doc(enum Cyc_Absyn_Primop p){
# 1460
return({struct Cyc_PP_Doc*(*_tmp46A)(struct _fat_ptr s)=Cyc_PP_text;struct _fat_ptr _tmp46B=({Cyc_Absynpp_prim2str(p);});_tmp46A(_tmp46B);});}
# 1161
int Cyc_Absynpp_is_declaration(struct Cyc_Absyn_Stmt*s){
# 1464
void*_stmttmp17=s->r;void*_tmp46D=_stmttmp17;if(((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp46D)->tag == 12U){_LL1: _LL2:
 return 1;}else{_LL3: _LL4:
 return 0;}_LL0:;}
# 1478 "absynpp.cyc"
struct _tuple15 Cyc_Absynpp_shadows(struct Cyc_Absyn_Decl*d,struct Cyc_List_List*varsinblock){
void*_stmttmp18=d->r;void*_tmp46F=_stmttmp18;struct Cyc_Absyn_Vardecl*_tmp470;if(((struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_tmp46F)->tag == 0U){_LL1: _tmp470=((struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_tmp46F)->f1;_LL2: {struct Cyc_Absyn_Vardecl*vd=_tmp470;
# 1481
if(({({int(*_tmp8EB)(int(*compare)(struct _tuple0*,struct _tuple0*),struct Cyc_List_List*l,struct _tuple0*x)=({int(*_tmp471)(int(*compare)(struct _tuple0*,struct _tuple0*),struct Cyc_List_List*l,struct _tuple0*x)=(int(*)(int(*compare)(struct _tuple0*,struct _tuple0*),struct Cyc_List_List*l,struct _tuple0*x))Cyc_List_mem;_tmp471;});struct Cyc_List_List*_tmp8EA=varsinblock;_tmp8EB(Cyc_Absyn_qvar_cmp,_tmp8EA,vd->name);});}))
return({struct _tuple15 _tmp744;_tmp744.f1=1,({struct Cyc_List_List*_tmp8EC=({struct Cyc_List_List*_tmp472=_cycalloc(sizeof(*_tmp472));_tmp472->hd=vd->name,_tmp472->tl=0;_tmp472;});_tmp744.f2=_tmp8EC;});_tmp744;});else{
# 1484
return({struct _tuple15 _tmp745;_tmp745.f1=0,({struct Cyc_List_List*_tmp8ED=({struct Cyc_List_List*_tmp473=_cycalloc(sizeof(*_tmp473));_tmp473->hd=vd->name,_tmp473->tl=varsinblock;_tmp473;});_tmp745.f2=_tmp8ED;});_tmp745;});}}}else{_LL3: _LL4:
# 1486
 return({struct _tuple15 _tmp746;_tmp746.f1=0,_tmp746.f2=varsinblock;_tmp746;});}_LL0:;}
# 1478
struct Cyc_PP_Doc*Cyc_Absynpp_block(int stmtexp,struct Cyc_PP_Doc*d){
# 1491
if(stmtexp)
return({struct Cyc_PP_Doc*_tmp475[8U];({struct Cyc_PP_Doc*_tmp8F5=({Cyc_PP_text(({const char*_tmp476="(";_tag_fat(_tmp476,sizeof(char),2U);}));});_tmp475[0]=_tmp8F5;}),({struct Cyc_PP_Doc*_tmp8F4=({Cyc_Absynpp_lb();});_tmp475[1]=_tmp8F4;}),({struct Cyc_PP_Doc*_tmp8F3=({Cyc_PP_blank_doc();});_tmp475[2]=_tmp8F3;}),({struct Cyc_PP_Doc*_tmp8F2=({Cyc_PP_nest(2,d);});_tmp475[3]=_tmp8F2;}),({struct Cyc_PP_Doc*_tmp8F1=({Cyc_PP_line_doc();});_tmp475[4]=_tmp8F1;}),({struct Cyc_PP_Doc*_tmp8F0=({Cyc_Absynpp_rb();});_tmp475[5]=_tmp8F0;}),({
struct Cyc_PP_Doc*_tmp8EF=({Cyc_PP_text(({const char*_tmp477=");";_tag_fat(_tmp477,sizeof(char),3U);}));});_tmp475[6]=_tmp8EF;}),({struct Cyc_PP_Doc*_tmp8EE=({Cyc_PP_line_doc();});_tmp475[7]=_tmp8EE;});Cyc_PP_cat(_tag_fat(_tmp475,sizeof(struct Cyc_PP_Doc*),8U));});else{
# 1495
return({struct Cyc_PP_Doc*_tmp478[6U];({struct Cyc_PP_Doc*_tmp8FB=({Cyc_Absynpp_lb();});_tmp478[0]=_tmp8FB;}),({struct Cyc_PP_Doc*_tmp8FA=({Cyc_PP_blank_doc();});_tmp478[1]=_tmp8FA;}),({struct Cyc_PP_Doc*_tmp8F9=({Cyc_PP_nest(2,d);});_tmp478[2]=_tmp8F9;}),({struct Cyc_PP_Doc*_tmp8F8=({Cyc_PP_line_doc();});_tmp478[3]=_tmp8F8;}),({struct Cyc_PP_Doc*_tmp8F7=({Cyc_Absynpp_rb();});_tmp478[4]=_tmp8F7;}),({struct Cyc_PP_Doc*_tmp8F6=({Cyc_PP_line_doc();});_tmp478[5]=_tmp8F6;});Cyc_PP_cat(_tag_fat(_tmp478,sizeof(struct Cyc_PP_Doc*),6U));});}}
# 1478
struct Cyc_PP_Doc*Cyc_Absynpp_stmt2doc(struct Cyc_Absyn_Stmt*st,int stmtexp,struct Cyc_List_List*varsinblock){
# 1499
struct Cyc_PP_Doc*s;
{void*_stmttmp19=st->r;void*_tmp47A=_stmttmp19;struct Cyc_List_List*_tmp47C;struct Cyc_Absyn_Stmt*_tmp47B;struct Cyc_Absyn_Exp*_tmp47E;struct Cyc_Absyn_Stmt*_tmp47D;struct Cyc_Absyn_Stmt*_tmp480;struct _fat_ptr*_tmp47F;struct Cyc_Absyn_Stmt*_tmp482;struct Cyc_Absyn_Decl*_tmp481;struct Cyc_List_List*_tmp483;struct Cyc_List_List*_tmp485;struct Cyc_Absyn_Exp*_tmp484;struct Cyc_Absyn_Stmt*_tmp489;struct Cyc_Absyn_Exp*_tmp488;struct Cyc_Absyn_Exp*_tmp487;struct Cyc_Absyn_Exp*_tmp486;struct _fat_ptr*_tmp48A;struct Cyc_Absyn_Stmt*_tmp48C;struct Cyc_Absyn_Exp*_tmp48B;struct Cyc_Absyn_Stmt*_tmp48F;struct Cyc_Absyn_Stmt*_tmp48E;struct Cyc_Absyn_Exp*_tmp48D;struct Cyc_Absyn_Exp*_tmp490;struct Cyc_Absyn_Stmt*_tmp492;struct Cyc_Absyn_Stmt*_tmp491;struct Cyc_Absyn_Exp*_tmp493;switch(*((int*)_tmp47A)){case 0U: _LL1: _LL2:
 s=({Cyc_PP_text(({const char*_tmp494=";";_tag_fat(_tmp494,sizeof(char),2U);}));});goto _LL0;case 1U: _LL3: _tmp493=((struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct*)_tmp47A)->f1;_LL4: {struct Cyc_Absyn_Exp*e=_tmp493;
s=({struct Cyc_PP_Doc*_tmp495[2U];({struct Cyc_PP_Doc*_tmp8FD=({Cyc_Absynpp_exp2doc(e);});_tmp495[0]=_tmp8FD;}),({struct Cyc_PP_Doc*_tmp8FC=({Cyc_PP_text(({const char*_tmp496=";";_tag_fat(_tmp496,sizeof(char),2U);}));});_tmp495[1]=_tmp8FC;});Cyc_PP_cat(_tag_fat(_tmp495,sizeof(struct Cyc_PP_Doc*),2U));});goto _LL0;}case 2U: _LL5: _tmp491=((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_tmp47A)->f1;_tmp492=((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_tmp47A)->f2;_LL6: {struct Cyc_Absyn_Stmt*s1=_tmp491;struct Cyc_Absyn_Stmt*s2=_tmp492;
# 1504
if(Cyc_Absynpp_decls_first){
if(({Cyc_Absynpp_is_declaration(s1);}))
s=({struct Cyc_PP_Doc*_tmp497[2U];({struct Cyc_PP_Doc*_tmp900=({struct Cyc_PP_Doc*(*_tmp498)(int stmtexp,struct Cyc_PP_Doc*d)=Cyc_Absynpp_block;int _tmp499=0;struct Cyc_PP_Doc*_tmp49A=({Cyc_Absynpp_stmt2doc(s1,0,0);});_tmp498(_tmp499,_tmp49A);});_tmp497[0]=_tmp900;}),
({Cyc_Absynpp_is_declaration(s2);})?({
struct Cyc_PP_Doc*_tmp8FF=({struct Cyc_PP_Doc*(*_tmp49B)(int stmtexp,struct Cyc_PP_Doc*d)=Cyc_Absynpp_block;int _tmp49C=stmtexp;struct Cyc_PP_Doc*_tmp49D=({Cyc_Absynpp_stmt2doc(s2,stmtexp,0);});_tmp49B(_tmp49C,_tmp49D);});_tmp497[1]=_tmp8FF;}):({
struct Cyc_PP_Doc*_tmp8FE=({Cyc_Absynpp_stmt2doc(s2,stmtexp,varsinblock);});_tmp497[1]=_tmp8FE;});Cyc_PP_cat(_tag_fat(_tmp497,sizeof(struct Cyc_PP_Doc*),2U));});else{
if(({Cyc_Absynpp_is_declaration(s2);}))
s=({struct Cyc_PP_Doc*_tmp49E[3U];({struct Cyc_PP_Doc*_tmp903=({Cyc_Absynpp_stmt2doc(s1,0,varsinblock);});_tmp49E[0]=_tmp903;}),({
struct Cyc_PP_Doc*_tmp902=({Cyc_PP_line_doc();});_tmp49E[1]=_tmp902;}),({
struct Cyc_PP_Doc*_tmp901=({struct Cyc_PP_Doc*(*_tmp49F)(int stmtexp,struct Cyc_PP_Doc*d)=Cyc_Absynpp_block;int _tmp4A0=stmtexp;struct Cyc_PP_Doc*_tmp4A1=({Cyc_Absynpp_stmt2doc(s2,stmtexp,0);});_tmp49F(_tmp4A0,_tmp4A1);});_tmp49E[2]=_tmp901;});Cyc_PP_cat(_tag_fat(_tmp49E,sizeof(struct Cyc_PP_Doc*),3U));});else{
# 1515
s=({struct Cyc_PP_Doc*_tmp4A2[3U];({struct Cyc_PP_Doc*_tmp906=({Cyc_Absynpp_stmt2doc(s1,0,varsinblock);});_tmp4A2[0]=_tmp906;}),({struct Cyc_PP_Doc*_tmp905=({Cyc_PP_line_doc();});_tmp4A2[1]=_tmp905;}),({
struct Cyc_PP_Doc*_tmp904=({Cyc_Absynpp_stmt2doc(s2,stmtexp,varsinblock);});_tmp4A2[2]=_tmp904;});Cyc_PP_cat(_tag_fat(_tmp4A2,sizeof(struct Cyc_PP_Doc*),3U));});}}}else{
# 1519
s=({struct Cyc_PP_Doc*_tmp4A3[3U];({struct Cyc_PP_Doc*_tmp909=({Cyc_Absynpp_stmt2doc(s1,0,0);});_tmp4A3[0]=_tmp909;}),({struct Cyc_PP_Doc*_tmp908=({Cyc_PP_line_doc();});_tmp4A3[1]=_tmp908;}),({struct Cyc_PP_Doc*_tmp907=({Cyc_Absynpp_stmt2doc(s2,stmtexp,0);});_tmp4A3[2]=_tmp907;});Cyc_PP_cat(_tag_fat(_tmp4A3,sizeof(struct Cyc_PP_Doc*),3U));});}
goto _LL0;}case 3U: _LL7: _tmp490=((struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct*)_tmp47A)->f1;_LL8: {struct Cyc_Absyn_Exp*eopt=_tmp490;
# 1522
if(eopt == 0)
s=({Cyc_PP_text(({const char*_tmp4A4="return;";_tag_fat(_tmp4A4,sizeof(char),8U);}));});else{
# 1525
s=({struct Cyc_PP_Doc*_tmp4A5[3U];({struct Cyc_PP_Doc*_tmp90D=({Cyc_PP_text(({const char*_tmp4A6="return ";_tag_fat(_tmp4A6,sizeof(char),8U);}));});_tmp4A5[0]=_tmp90D;}),
eopt == 0?({struct Cyc_PP_Doc*_tmp90C=({Cyc_PP_nil_doc();});_tmp4A5[1]=_tmp90C;}):({struct Cyc_PP_Doc*_tmp90B=({Cyc_Absynpp_exp2doc(eopt);});_tmp4A5[1]=_tmp90B;}),({
struct Cyc_PP_Doc*_tmp90A=({Cyc_PP_text(({const char*_tmp4A7=";";_tag_fat(_tmp4A7,sizeof(char),2U);}));});_tmp4A5[2]=_tmp90A;});Cyc_PP_cat(_tag_fat(_tmp4A5,sizeof(struct Cyc_PP_Doc*),3U));});}
goto _LL0;}case 4U: _LL9: _tmp48D=((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmp47A)->f1;_tmp48E=((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmp47A)->f2;_tmp48F=((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmp47A)->f3;_LLA: {struct Cyc_Absyn_Exp*e=_tmp48D;struct Cyc_Absyn_Stmt*s1=_tmp48E;struct Cyc_Absyn_Stmt*s2=_tmp48F;
# 1530
int print_else;
{void*_stmttmp1A=s2->r;void*_tmp4A8=_stmttmp1A;if(((struct Cyc_Absyn_Skip_s_Absyn_Raw_stmt_struct*)_tmp4A8)->tag == 0U){_LL24: _LL25:
 print_else=0;goto _LL23;}else{_LL26: _LL27:
 print_else=1;goto _LL23;}_LL23:;}
# 1535
s=({struct Cyc_PP_Doc*_tmp4A9[5U];({struct Cyc_PP_Doc*_tmp916=({Cyc_PP_text(({const char*_tmp4AA="if (";_tag_fat(_tmp4AA,sizeof(char),5U);}));});_tmp4A9[0]=_tmp916;}),({
struct Cyc_PP_Doc*_tmp915=({Cyc_Absynpp_exp2doc(e);});_tmp4A9[1]=_tmp915;}),({
struct Cyc_PP_Doc*_tmp914=({Cyc_PP_text(({const char*_tmp4AB=") ";_tag_fat(_tmp4AB,sizeof(char),3U);}));});_tmp4A9[2]=_tmp914;}),({
struct Cyc_PP_Doc*_tmp913=({struct Cyc_PP_Doc*(*_tmp4AC)(int stmtexp,struct Cyc_PP_Doc*d)=Cyc_Absynpp_block;int _tmp4AD=0;struct Cyc_PP_Doc*_tmp4AE=({Cyc_Absynpp_stmt2doc(s1,0,0);});_tmp4AC(_tmp4AD,_tmp4AE);});_tmp4A9[3]=_tmp913;}),
print_else?({
struct Cyc_PP_Doc*_tmp912=({struct Cyc_PP_Doc*_tmp4AF[3U];({struct Cyc_PP_Doc*_tmp911=({Cyc_PP_line_doc();});_tmp4AF[0]=_tmp911;}),({
struct Cyc_PP_Doc*_tmp910=({Cyc_PP_text(({const char*_tmp4B0="else ";_tag_fat(_tmp4B0,sizeof(char),6U);}));});_tmp4AF[1]=_tmp910;}),({
struct Cyc_PP_Doc*_tmp90F=({struct Cyc_PP_Doc*(*_tmp4B1)(int stmtexp,struct Cyc_PP_Doc*d)=Cyc_Absynpp_block;int _tmp4B2=0;struct Cyc_PP_Doc*_tmp4B3=({Cyc_Absynpp_stmt2doc(s2,0,0);});_tmp4B1(_tmp4B2,_tmp4B3);});_tmp4AF[2]=_tmp90F;});Cyc_PP_cat(_tag_fat(_tmp4AF,sizeof(struct Cyc_PP_Doc*),3U));});
# 1540
_tmp4A9[4]=_tmp912;}):({
# 1543
struct Cyc_PP_Doc*_tmp90E=({Cyc_PP_nil_doc();});_tmp4A9[4]=_tmp90E;});Cyc_PP_cat(_tag_fat(_tmp4A9,sizeof(struct Cyc_PP_Doc*),5U));});
goto _LL0;}case 5U: _LLB: _tmp48B=(((struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct*)_tmp47A)->f1).f1;_tmp48C=((struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct*)_tmp47A)->f2;_LLC: {struct Cyc_Absyn_Exp*e=_tmp48B;struct Cyc_Absyn_Stmt*s1=_tmp48C;
# 1546
s=({struct Cyc_PP_Doc*_tmp4B4[4U];({struct Cyc_PP_Doc*_tmp91A=({Cyc_PP_text(({const char*_tmp4B5="while (";_tag_fat(_tmp4B5,sizeof(char),8U);}));});_tmp4B4[0]=_tmp91A;}),({
struct Cyc_PP_Doc*_tmp919=({Cyc_Absynpp_exp2doc(e);});_tmp4B4[1]=_tmp919;}),({
struct Cyc_PP_Doc*_tmp918=({Cyc_PP_text(({const char*_tmp4B6=") ";_tag_fat(_tmp4B6,sizeof(char),3U);}));});_tmp4B4[2]=_tmp918;}),({
struct Cyc_PP_Doc*_tmp917=({struct Cyc_PP_Doc*(*_tmp4B7)(int stmtexp,struct Cyc_PP_Doc*d)=Cyc_Absynpp_block;int _tmp4B8=0;struct Cyc_PP_Doc*_tmp4B9=({Cyc_Absynpp_stmt2doc(s1,0,0);});_tmp4B7(_tmp4B8,_tmp4B9);});_tmp4B4[3]=_tmp917;});Cyc_PP_cat(_tag_fat(_tmp4B4,sizeof(struct Cyc_PP_Doc*),4U));});
goto _LL0;}case 6U: _LLD: _LLE:
 s=({Cyc_PP_text(({const char*_tmp4BA="break;";_tag_fat(_tmp4BA,sizeof(char),7U);}));});goto _LL0;case 7U: _LLF: _LL10:
 s=({Cyc_PP_text(({const char*_tmp4BB="continue;";_tag_fat(_tmp4BB,sizeof(char),10U);}));});goto _LL0;case 8U: _LL11: _tmp48A=((struct Cyc_Absyn_Goto_s_Absyn_Raw_stmt_struct*)_tmp47A)->f1;_LL12: {struct _fat_ptr*x=_tmp48A;
s=({struct Cyc_PP_Doc*(*_tmp4BC)(struct _fat_ptr s)=Cyc_PP_text;struct _fat_ptr _tmp4BD=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp4C0=({struct Cyc_String_pa_PrintArg_struct _tmp747;_tmp747.tag=0U,_tmp747.f1=(struct _fat_ptr)((struct _fat_ptr)*x);_tmp747;});void*_tmp4BE[1U];_tmp4BE[0]=& _tmp4C0;({struct _fat_ptr _tmp91B=({const char*_tmp4BF="goto %s;";_tag_fat(_tmp4BF,sizeof(char),9U);});Cyc_aprintf(_tmp91B,_tag_fat(_tmp4BE,sizeof(void*),1U));});});_tmp4BC(_tmp4BD);});goto _LL0;}case 9U: _LL13: _tmp486=((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp47A)->f1;_tmp487=(((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp47A)->f2).f1;_tmp488=(((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp47A)->f3).f1;_tmp489=((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp47A)->f4;_LL14: {struct Cyc_Absyn_Exp*e1=_tmp486;struct Cyc_Absyn_Exp*e2=_tmp487;struct Cyc_Absyn_Exp*e3=_tmp488;struct Cyc_Absyn_Stmt*s1=_tmp489;
# 1555
s=({struct Cyc_PP_Doc*_tmp4C1[8U];({struct Cyc_PP_Doc*_tmp923=({Cyc_PP_text(({const char*_tmp4C2="for(";_tag_fat(_tmp4C2,sizeof(char),5U);}));});_tmp4C1[0]=_tmp923;}),({
struct Cyc_PP_Doc*_tmp922=({Cyc_Absynpp_exp2doc(e1);});_tmp4C1[1]=_tmp922;}),({
struct Cyc_PP_Doc*_tmp921=({Cyc_PP_text(({const char*_tmp4C3="; ";_tag_fat(_tmp4C3,sizeof(char),3U);}));});_tmp4C1[2]=_tmp921;}),({
struct Cyc_PP_Doc*_tmp920=({Cyc_Absynpp_exp2doc(e2);});_tmp4C1[3]=_tmp920;}),({
struct Cyc_PP_Doc*_tmp91F=({Cyc_PP_text(({const char*_tmp4C4="; ";_tag_fat(_tmp4C4,sizeof(char),3U);}));});_tmp4C1[4]=_tmp91F;}),({
struct Cyc_PP_Doc*_tmp91E=({Cyc_Absynpp_exp2doc(e3);});_tmp4C1[5]=_tmp91E;}),({
struct Cyc_PP_Doc*_tmp91D=({Cyc_PP_text(({const char*_tmp4C5=") ";_tag_fat(_tmp4C5,sizeof(char),3U);}));});_tmp4C1[6]=_tmp91D;}),({
struct Cyc_PP_Doc*_tmp91C=({struct Cyc_PP_Doc*(*_tmp4C6)(int stmtexp,struct Cyc_PP_Doc*d)=Cyc_Absynpp_block;int _tmp4C7=0;struct Cyc_PP_Doc*_tmp4C8=({Cyc_Absynpp_stmt2doc(s1,0,0);});_tmp4C6(_tmp4C7,_tmp4C8);});_tmp4C1[7]=_tmp91C;});Cyc_PP_cat(_tag_fat(_tmp4C1,sizeof(struct Cyc_PP_Doc*),8U));});
goto _LL0;}case 10U: _LL15: _tmp484=((struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct*)_tmp47A)->f1;_tmp485=((struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct*)_tmp47A)->f2;_LL16: {struct Cyc_Absyn_Exp*e=_tmp484;struct Cyc_List_List*ss=_tmp485;
# 1565
s=({struct Cyc_PP_Doc*_tmp4C9[8U];({struct Cyc_PP_Doc*_tmp92B=({Cyc_PP_text(({const char*_tmp4CA="switch (";_tag_fat(_tmp4CA,sizeof(char),9U);}));});_tmp4C9[0]=_tmp92B;}),({
struct Cyc_PP_Doc*_tmp92A=({Cyc_Absynpp_exp2doc(e);});_tmp4C9[1]=_tmp92A;}),({
struct Cyc_PP_Doc*_tmp929=({Cyc_PP_text(({const char*_tmp4CB=") ";_tag_fat(_tmp4CB,sizeof(char),3U);}));});_tmp4C9[2]=_tmp929;}),({
struct Cyc_PP_Doc*_tmp928=({Cyc_Absynpp_lb();});_tmp4C9[3]=_tmp928;}),({
struct Cyc_PP_Doc*_tmp927=({Cyc_PP_line_doc();});_tmp4C9[4]=_tmp927;}),({
struct Cyc_PP_Doc*_tmp926=({Cyc_Absynpp_switchclauses2doc(ss);});_tmp4C9[5]=_tmp926;}),({
struct Cyc_PP_Doc*_tmp925=({Cyc_PP_line_doc();});_tmp4C9[6]=_tmp925;}),({
struct Cyc_PP_Doc*_tmp924=({Cyc_Absynpp_rb();});_tmp4C9[7]=_tmp924;});Cyc_PP_cat(_tag_fat(_tmp4C9,sizeof(struct Cyc_PP_Doc*),8U));});
goto _LL0;}case 11U: if(((struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct*)_tmp47A)->f1 == 0){_LL17: _LL18:
 s=({Cyc_PP_text(({const char*_tmp4CC="fallthru;";_tag_fat(_tmp4CC,sizeof(char),10U);}));});goto _LL0;}else{_LL19: _tmp483=((struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct*)_tmp47A)->f1;_LL1A: {struct Cyc_List_List*es=_tmp483;
# 1576
s=({struct Cyc_PP_Doc*_tmp4CD[3U];({struct Cyc_PP_Doc*_tmp92E=({Cyc_PP_text(({const char*_tmp4CE="fallthru(";_tag_fat(_tmp4CE,sizeof(char),10U);}));});_tmp4CD[0]=_tmp92E;}),({struct Cyc_PP_Doc*_tmp92D=({Cyc_Absynpp_exps2doc_prec(20,es);});_tmp4CD[1]=_tmp92D;}),({struct Cyc_PP_Doc*_tmp92C=({Cyc_PP_text(({const char*_tmp4CF=");";_tag_fat(_tmp4CF,sizeof(char),3U);}));});_tmp4CD[2]=_tmp92C;});Cyc_PP_cat(_tag_fat(_tmp4CD,sizeof(struct Cyc_PP_Doc*),3U));});goto _LL0;}}case 12U: _LL1B: _tmp481=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp47A)->f1;_tmp482=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp47A)->f2;_LL1C: {struct Cyc_Absyn_Decl*d=_tmp481;struct Cyc_Absyn_Stmt*s1=_tmp482;
# 1578
struct _tuple15 _stmttmp1B=({Cyc_Absynpp_shadows(d,varsinblock);});struct Cyc_List_List*_tmp4D1;int _tmp4D0;_LL29: _tmp4D0=_stmttmp1B.f1;_tmp4D1=_stmttmp1B.f2;_LL2A: {int newblock=_tmp4D0;struct Cyc_List_List*newvarsinblock=_tmp4D1;
s=({struct Cyc_PP_Doc*_tmp4D2[3U];({struct Cyc_PP_Doc*_tmp931=({Cyc_Absynpp_decl2doc(d);});_tmp4D2[0]=_tmp931;}),({struct Cyc_PP_Doc*_tmp930=({Cyc_PP_line_doc();});_tmp4D2[1]=_tmp930;}),({struct Cyc_PP_Doc*_tmp92F=({Cyc_Absynpp_stmt2doc(s1,stmtexp,newvarsinblock);});_tmp4D2[2]=_tmp92F;});Cyc_PP_cat(_tag_fat(_tmp4D2,sizeof(struct Cyc_PP_Doc*),3U));});
if(newblock)s=({Cyc_Absynpp_block(stmtexp,s);});goto _LL0;}}case 13U: _LL1D: _tmp47F=((struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct*)_tmp47A)->f1;_tmp480=((struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct*)_tmp47A)->f2;_LL1E: {struct _fat_ptr*x=_tmp47F;struct Cyc_Absyn_Stmt*s1=_tmp480;
# 1583
if(Cyc_Absynpp_decls_first &&({Cyc_Absynpp_is_declaration(s1);}))
s=({struct Cyc_PP_Doc*_tmp4D3[3U];({struct Cyc_PP_Doc*_tmp934=({Cyc_PP_textptr(x);});_tmp4D3[0]=_tmp934;}),({struct Cyc_PP_Doc*_tmp933=({Cyc_PP_text(({const char*_tmp4D4=": ";_tag_fat(_tmp4D4,sizeof(char),3U);}));});_tmp4D3[1]=_tmp933;}),({struct Cyc_PP_Doc*_tmp932=({struct Cyc_PP_Doc*(*_tmp4D5)(int stmtexp,struct Cyc_PP_Doc*d)=Cyc_Absynpp_block;int _tmp4D6=stmtexp;struct Cyc_PP_Doc*_tmp4D7=({Cyc_Absynpp_stmt2doc(s1,stmtexp,0);});_tmp4D5(_tmp4D6,_tmp4D7);});_tmp4D3[2]=_tmp932;});Cyc_PP_cat(_tag_fat(_tmp4D3,sizeof(struct Cyc_PP_Doc*),3U));});else{
# 1586
s=({struct Cyc_PP_Doc*_tmp4D8[3U];({struct Cyc_PP_Doc*_tmp937=({Cyc_PP_textptr(x);});_tmp4D8[0]=_tmp937;}),({struct Cyc_PP_Doc*_tmp936=({Cyc_PP_text(({const char*_tmp4D9=": ";_tag_fat(_tmp4D9,sizeof(char),3U);}));});_tmp4D8[1]=_tmp936;}),({struct Cyc_PP_Doc*_tmp935=({Cyc_Absynpp_stmt2doc(s1,stmtexp,varsinblock);});_tmp4D8[2]=_tmp935;});Cyc_PP_cat(_tag_fat(_tmp4D8,sizeof(struct Cyc_PP_Doc*),3U));});}
goto _LL0;}case 14U: _LL1F: _tmp47D=((struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct*)_tmp47A)->f1;_tmp47E=(((struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct*)_tmp47A)->f2).f1;_LL20: {struct Cyc_Absyn_Stmt*s1=_tmp47D;struct Cyc_Absyn_Exp*e=_tmp47E;
# 1589
s=({struct Cyc_PP_Doc*_tmp4DA[5U];({struct Cyc_PP_Doc*_tmp93C=({Cyc_PP_text(({const char*_tmp4DB="do ";_tag_fat(_tmp4DB,sizeof(char),4U);}));});_tmp4DA[0]=_tmp93C;}),({
struct Cyc_PP_Doc*_tmp93B=({struct Cyc_PP_Doc*(*_tmp4DC)(int stmtexp,struct Cyc_PP_Doc*d)=Cyc_Absynpp_block;int _tmp4DD=0;struct Cyc_PP_Doc*_tmp4DE=({Cyc_Absynpp_stmt2doc(s1,0,0);});_tmp4DC(_tmp4DD,_tmp4DE);});_tmp4DA[1]=_tmp93B;}),({
struct Cyc_PP_Doc*_tmp93A=({Cyc_PP_text(({const char*_tmp4DF=" while (";_tag_fat(_tmp4DF,sizeof(char),9U);}));});_tmp4DA[2]=_tmp93A;}),({
struct Cyc_PP_Doc*_tmp939=({Cyc_Absynpp_exp2doc(e);});_tmp4DA[3]=_tmp939;}),({
struct Cyc_PP_Doc*_tmp938=({Cyc_PP_text(({const char*_tmp4E0=");";_tag_fat(_tmp4E0,sizeof(char),3U);}));});_tmp4DA[4]=_tmp938;});Cyc_PP_cat(_tag_fat(_tmp4DA,sizeof(struct Cyc_PP_Doc*),5U));});
goto _LL0;}default: _LL21: _tmp47B=((struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct*)_tmp47A)->f1;_tmp47C=((struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct*)_tmp47A)->f2;_LL22: {struct Cyc_Absyn_Stmt*s1=_tmp47B;struct Cyc_List_List*ss=_tmp47C;
# 1596
s=({struct Cyc_PP_Doc*_tmp4E1[4U];({struct Cyc_PP_Doc*_tmp940=({Cyc_PP_text(({const char*_tmp4E2="try ";_tag_fat(_tmp4E2,sizeof(char),5U);}));});_tmp4E1[0]=_tmp940;}),({
struct Cyc_PP_Doc*_tmp93F=({struct Cyc_PP_Doc*(*_tmp4E3)(int stmtexp,struct Cyc_PP_Doc*d)=Cyc_Absynpp_block;int _tmp4E4=0;struct Cyc_PP_Doc*_tmp4E5=({Cyc_Absynpp_stmt2doc(s1,0,0);});_tmp4E3(_tmp4E4,_tmp4E5);});_tmp4E1[1]=_tmp93F;}),({
struct Cyc_PP_Doc*_tmp93E=({Cyc_PP_text(({const char*_tmp4E6=" catch ";_tag_fat(_tmp4E6,sizeof(char),8U);}));});_tmp4E1[2]=_tmp93E;}),({
struct Cyc_PP_Doc*_tmp93D=({struct Cyc_PP_Doc*(*_tmp4E7)(int stmtexp,struct Cyc_PP_Doc*d)=Cyc_Absynpp_block;int _tmp4E8=0;struct Cyc_PP_Doc*_tmp4E9=({Cyc_Absynpp_switchclauses2doc(ss);});_tmp4E7(_tmp4E8,_tmp4E9);});_tmp4E1[3]=_tmp93D;});Cyc_PP_cat(_tag_fat(_tmp4E1,sizeof(struct Cyc_PP_Doc*),4U));});
goto _LL0;}}_LL0:;}
# 1602
return s;}
# 1478
struct Cyc_PP_Doc*Cyc_Absynpp_pat2doc(struct Cyc_Absyn_Pat*p){
# 1606
struct Cyc_PP_Doc*s;
{void*_stmttmp1C=p->r;void*_tmp4EB=_stmttmp1C;struct Cyc_Absyn_Exp*_tmp4EC;int _tmp4EF;struct Cyc_List_List*_tmp4EE;struct Cyc_Absyn_Datatypefield*_tmp4ED;struct Cyc_Absyn_Datatypefield*_tmp4F0;struct Cyc_Absyn_Enumfield*_tmp4F1;struct Cyc_Absyn_Enumfield*_tmp4F2;int _tmp4F5;struct Cyc_List_List*_tmp4F4;struct Cyc_List_List*_tmp4F3;int _tmp4F9;struct Cyc_List_List*_tmp4F8;struct Cyc_List_List*_tmp4F7;union Cyc_Absyn_AggrInfo _tmp4F6;int _tmp4FC;struct Cyc_List_List*_tmp4FB;struct _tuple0*_tmp4FA;struct _tuple0*_tmp4FD;struct Cyc_Absyn_Pat*_tmp4FF;struct Cyc_Absyn_Vardecl*_tmp4FE;struct Cyc_Absyn_Vardecl*_tmp500;struct Cyc_Absyn_Pat*_tmp501;int _tmp503;struct Cyc_List_List*_tmp502;struct Cyc_Absyn_Vardecl*_tmp505;struct Cyc_Absyn_Tvar*_tmp504;struct Cyc_Absyn_Vardecl*_tmp507;struct Cyc_Absyn_Tvar*_tmp506;struct Cyc_Absyn_Pat*_tmp509;struct Cyc_Absyn_Vardecl*_tmp508;struct Cyc_Absyn_Vardecl*_tmp50A;struct _fat_ptr _tmp50B;char _tmp50C;int _tmp50E;enum Cyc_Absyn_Sign _tmp50D;switch(*((int*)_tmp4EB)){case 0U: _LL1: _LL2:
 s=({Cyc_PP_text(({const char*_tmp50F="_";_tag_fat(_tmp50F,sizeof(char),2U);}));});goto _LL0;case 9U: _LL3: _LL4:
 s=({Cyc_PP_text(({const char*_tmp510="NULL";_tag_fat(_tmp510,sizeof(char),5U);}));});goto _LL0;case 10U: _LL5: _tmp50D=((struct Cyc_Absyn_Int_p_Absyn_Raw_pat_struct*)_tmp4EB)->f1;_tmp50E=((struct Cyc_Absyn_Int_p_Absyn_Raw_pat_struct*)_tmp4EB)->f2;_LL6: {enum Cyc_Absyn_Sign sg=_tmp50D;int i=_tmp50E;
# 1611
if((int)sg != (int)1U)
s=({struct Cyc_PP_Doc*(*_tmp511)(struct _fat_ptr s)=Cyc_PP_text;struct _fat_ptr _tmp512=(struct _fat_ptr)({struct Cyc_Int_pa_PrintArg_struct _tmp515=({struct Cyc_Int_pa_PrintArg_struct _tmp748;_tmp748.tag=1U,_tmp748.f1=(unsigned long)i;_tmp748;});void*_tmp513[1U];_tmp513[0]=& _tmp515;({struct _fat_ptr _tmp941=({const char*_tmp514="%d";_tag_fat(_tmp514,sizeof(char),3U);});Cyc_aprintf(_tmp941,_tag_fat(_tmp513,sizeof(void*),1U));});});_tmp511(_tmp512);});else{
s=({struct Cyc_PP_Doc*(*_tmp516)(struct _fat_ptr s)=Cyc_PP_text;struct _fat_ptr _tmp517=(struct _fat_ptr)({struct Cyc_Int_pa_PrintArg_struct _tmp51A=({struct Cyc_Int_pa_PrintArg_struct _tmp749;_tmp749.tag=1U,_tmp749.f1=(unsigned)i;_tmp749;});void*_tmp518[1U];_tmp518[0]=& _tmp51A;({struct _fat_ptr _tmp942=({const char*_tmp519="%u";_tag_fat(_tmp519,sizeof(char),3U);});Cyc_aprintf(_tmp942,_tag_fat(_tmp518,sizeof(void*),1U));});});_tmp516(_tmp517);});}
goto _LL0;}case 11U: _LL7: _tmp50C=((struct Cyc_Absyn_Char_p_Absyn_Raw_pat_struct*)_tmp4EB)->f1;_LL8: {char ch=_tmp50C;
s=({struct Cyc_PP_Doc*(*_tmp51B)(struct _fat_ptr s)=Cyc_PP_text;struct _fat_ptr _tmp51C=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp51F=({struct Cyc_String_pa_PrintArg_struct _tmp74A;_tmp74A.tag=0U,({struct _fat_ptr _tmp943=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_char_escape(ch);}));_tmp74A.f1=_tmp943;});_tmp74A;});void*_tmp51D[1U];_tmp51D[0]=& _tmp51F;({struct _fat_ptr _tmp944=({const char*_tmp51E="'%s'";_tag_fat(_tmp51E,sizeof(char),5U);});Cyc_aprintf(_tmp944,_tag_fat(_tmp51D,sizeof(void*),1U));});});_tmp51B(_tmp51C);});goto _LL0;}case 12U: _LL9: _tmp50B=((struct Cyc_Absyn_Float_p_Absyn_Raw_pat_struct*)_tmp4EB)->f1;_LLA: {struct _fat_ptr x=_tmp50B;
s=({Cyc_PP_text(x);});goto _LL0;}case 1U: if(((struct Cyc_Absyn_Wild_p_Absyn_Raw_pat_struct*)((struct Cyc_Absyn_Pat*)((struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct*)_tmp4EB)->f2)->r)->tag == 0U){_LLB: _tmp50A=((struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct*)_tmp4EB)->f1;_LLC: {struct Cyc_Absyn_Vardecl*vd=_tmp50A;
# 1618
s=({Cyc_Absynpp_qvar2doc(vd->name);});goto _LL0;}}else{_LLD: _tmp508=((struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct*)_tmp4EB)->f1;_tmp509=((struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct*)_tmp4EB)->f2;_LLE: {struct Cyc_Absyn_Vardecl*vd=_tmp508;struct Cyc_Absyn_Pat*p2=_tmp509;
# 1620
s=({struct Cyc_PP_Doc*_tmp520[3U];({struct Cyc_PP_Doc*_tmp947=({Cyc_Absynpp_qvar2doc(vd->name);});_tmp520[0]=_tmp947;}),({struct Cyc_PP_Doc*_tmp946=({Cyc_PP_text(({const char*_tmp521=" as ";_tag_fat(_tmp521,sizeof(char),5U);}));});_tmp520[1]=_tmp946;}),({struct Cyc_PP_Doc*_tmp945=({Cyc_Absynpp_pat2doc(p2);});_tmp520[2]=_tmp945;});Cyc_PP_cat(_tag_fat(_tmp520,sizeof(struct Cyc_PP_Doc*),3U));});goto _LL0;}}case 2U: _LLF: _tmp506=((struct Cyc_Absyn_AliasVar_p_Absyn_Raw_pat_struct*)_tmp4EB)->f1;_tmp507=((struct Cyc_Absyn_AliasVar_p_Absyn_Raw_pat_struct*)_tmp4EB)->f2;_LL10: {struct Cyc_Absyn_Tvar*tv=_tmp506;struct Cyc_Absyn_Vardecl*vd=_tmp507;
# 1622
s=({struct Cyc_PP_Doc*_tmp522[5U];({struct Cyc_PP_Doc*_tmp94C=({Cyc_PP_text(({const char*_tmp523="alias";_tag_fat(_tmp523,sizeof(char),6U);}));});_tmp522[0]=_tmp94C;}),({struct Cyc_PP_Doc*_tmp94B=({Cyc_PP_text(({const char*_tmp524=" <";_tag_fat(_tmp524,sizeof(char),3U);}));});_tmp522[1]=_tmp94B;}),({struct Cyc_PP_Doc*_tmp94A=({Cyc_Absynpp_tvar2doc(tv);});_tmp522[2]=_tmp94A;}),({struct Cyc_PP_Doc*_tmp949=({Cyc_PP_text(({const char*_tmp525="> ";_tag_fat(_tmp525,sizeof(char),3U);}));});_tmp522[3]=_tmp949;}),({
struct Cyc_PP_Doc*_tmp948=({Cyc_Absynpp_vardecl2doc(vd);});_tmp522[4]=_tmp948;});Cyc_PP_cat(_tag_fat(_tmp522,sizeof(struct Cyc_PP_Doc*),5U));});
goto _LL0;}case 4U: _LL11: _tmp504=((struct Cyc_Absyn_TagInt_p_Absyn_Raw_pat_struct*)_tmp4EB)->f1;_tmp505=((struct Cyc_Absyn_TagInt_p_Absyn_Raw_pat_struct*)_tmp4EB)->f2;_LL12: {struct Cyc_Absyn_Tvar*tv=_tmp504;struct Cyc_Absyn_Vardecl*vd=_tmp505;
# 1626
s=({struct Cyc_PP_Doc*_tmp526[4U];({struct Cyc_PP_Doc*_tmp950=({Cyc_Absynpp_qvar2doc(vd->name);});_tmp526[0]=_tmp950;}),({struct Cyc_PP_Doc*_tmp94F=({Cyc_PP_text(({const char*_tmp527="<";_tag_fat(_tmp527,sizeof(char),2U);}));});_tmp526[1]=_tmp94F;}),({struct Cyc_PP_Doc*_tmp94E=({Cyc_Absynpp_tvar2doc(tv);});_tmp526[2]=_tmp94E;}),({struct Cyc_PP_Doc*_tmp94D=({Cyc_PP_text(({const char*_tmp528=">";_tag_fat(_tmp528,sizeof(char),2U);}));});_tmp526[3]=_tmp94D;});Cyc_PP_cat(_tag_fat(_tmp526,sizeof(struct Cyc_PP_Doc*),4U));});
goto _LL0;}case 5U: _LL13: _tmp502=((struct Cyc_Absyn_Tuple_p_Absyn_Raw_pat_struct*)_tmp4EB)->f1;_tmp503=((struct Cyc_Absyn_Tuple_p_Absyn_Raw_pat_struct*)_tmp4EB)->f2;_LL14: {struct Cyc_List_List*ts=_tmp502;int dots=_tmp503;
# 1629
s=({struct Cyc_PP_Doc*_tmp529[4U];({struct Cyc_PP_Doc*_tmp957=({Cyc_Absynpp_dollar();});_tmp529[0]=_tmp957;}),({struct Cyc_PP_Doc*_tmp956=({Cyc_PP_text(({const char*_tmp52A="(";_tag_fat(_tmp52A,sizeof(char),2U);}));});_tmp529[1]=_tmp956;}),({struct Cyc_PP_Doc*_tmp955=({({struct Cyc_PP_Doc*(*_tmp954)(struct Cyc_PP_Doc*(*pp)(struct Cyc_Absyn_Pat*),struct _fat_ptr sep,struct Cyc_List_List*l)=({struct Cyc_PP_Doc*(*_tmp52B)(struct Cyc_PP_Doc*(*pp)(struct Cyc_Absyn_Pat*),struct _fat_ptr sep,struct Cyc_List_List*l)=(struct Cyc_PP_Doc*(*)(struct Cyc_PP_Doc*(*pp)(struct Cyc_Absyn_Pat*),struct _fat_ptr sep,struct Cyc_List_List*l))Cyc_PP_ppseq;_tmp52B;});struct _fat_ptr _tmp953=({const char*_tmp52C=",";_tag_fat(_tmp52C,sizeof(char),2U);});_tmp954(Cyc_Absynpp_pat2doc,_tmp953,ts);});});_tmp529[2]=_tmp955;}),
dots?({struct Cyc_PP_Doc*_tmp952=({Cyc_PP_text(({const char*_tmp52D=", ...)";_tag_fat(_tmp52D,sizeof(char),7U);}));});_tmp529[3]=_tmp952;}):({struct Cyc_PP_Doc*_tmp951=({Cyc_PP_text(({const char*_tmp52E=")";_tag_fat(_tmp52E,sizeof(char),2U);}));});_tmp529[3]=_tmp951;});Cyc_PP_cat(_tag_fat(_tmp529,sizeof(struct Cyc_PP_Doc*),4U));});
goto _LL0;}case 6U: _LL15: _tmp501=((struct Cyc_Absyn_Pointer_p_Absyn_Raw_pat_struct*)_tmp4EB)->f1;_LL16: {struct Cyc_Absyn_Pat*p2=_tmp501;
# 1633
s=({struct Cyc_PP_Doc*_tmp52F[2U];({struct Cyc_PP_Doc*_tmp959=({Cyc_PP_text(({const char*_tmp530="&";_tag_fat(_tmp530,sizeof(char),2U);}));});_tmp52F[0]=_tmp959;}),({struct Cyc_PP_Doc*_tmp958=({Cyc_Absynpp_pat2doc(p2);});_tmp52F[1]=_tmp958;});Cyc_PP_cat(_tag_fat(_tmp52F,sizeof(struct Cyc_PP_Doc*),2U));});
goto _LL0;}case 3U: if(((struct Cyc_Absyn_Wild_p_Absyn_Raw_pat_struct*)((struct Cyc_Absyn_Pat*)((struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct*)_tmp4EB)->f2)->r)->tag == 0U){_LL17: _tmp500=((struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct*)_tmp4EB)->f1;_LL18: {struct Cyc_Absyn_Vardecl*vd=_tmp500;
# 1636
s=({struct Cyc_PP_Doc*_tmp531[2U];({struct Cyc_PP_Doc*_tmp95B=({Cyc_PP_text(({const char*_tmp532="*";_tag_fat(_tmp532,sizeof(char),2U);}));});_tmp531[0]=_tmp95B;}),({struct Cyc_PP_Doc*_tmp95A=({Cyc_Absynpp_qvar2doc(vd->name);});_tmp531[1]=_tmp95A;});Cyc_PP_cat(_tag_fat(_tmp531,sizeof(struct Cyc_PP_Doc*),2U));});
goto _LL0;}}else{_LL19: _tmp4FE=((struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct*)_tmp4EB)->f1;_tmp4FF=((struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct*)_tmp4EB)->f2;_LL1A: {struct Cyc_Absyn_Vardecl*vd=_tmp4FE;struct Cyc_Absyn_Pat*p2=_tmp4FF;
# 1639
s=({struct Cyc_PP_Doc*_tmp533[4U];({struct Cyc_PP_Doc*_tmp95F=({Cyc_PP_text(({const char*_tmp534="*";_tag_fat(_tmp534,sizeof(char),2U);}));});_tmp533[0]=_tmp95F;}),({struct Cyc_PP_Doc*_tmp95E=({Cyc_Absynpp_qvar2doc(vd->name);});_tmp533[1]=_tmp95E;}),({struct Cyc_PP_Doc*_tmp95D=({Cyc_PP_text(({const char*_tmp535=" as ";_tag_fat(_tmp535,sizeof(char),5U);}));});_tmp533[2]=_tmp95D;}),({struct Cyc_PP_Doc*_tmp95C=({Cyc_Absynpp_pat2doc(p2);});_tmp533[3]=_tmp95C;});Cyc_PP_cat(_tag_fat(_tmp533,sizeof(struct Cyc_PP_Doc*),4U));});
goto _LL0;}}case 15U: _LL1B: _tmp4FD=((struct Cyc_Absyn_UnknownId_p_Absyn_Raw_pat_struct*)_tmp4EB)->f1;_LL1C: {struct _tuple0*q=_tmp4FD;
# 1642
s=({Cyc_Absynpp_qvar2doc(q);});
goto _LL0;}case 16U: _LL1D: _tmp4FA=((struct Cyc_Absyn_UnknownCall_p_Absyn_Raw_pat_struct*)_tmp4EB)->f1;_tmp4FB=((struct Cyc_Absyn_UnknownCall_p_Absyn_Raw_pat_struct*)_tmp4EB)->f2;_tmp4FC=((struct Cyc_Absyn_UnknownCall_p_Absyn_Raw_pat_struct*)_tmp4EB)->f3;_LL1E: {struct _tuple0*q=_tmp4FA;struct Cyc_List_List*ps=_tmp4FB;int dots=_tmp4FC;
# 1645
struct _fat_ptr term=dots?({const char*_tmp53F=", ...)";_tag_fat(_tmp53F,sizeof(char),7U);}):({const char*_tmp540=")";_tag_fat(_tmp540,sizeof(char),2U);});
s=({struct Cyc_PP_Doc*_tmp536[2U];({struct Cyc_PP_Doc*_tmp962=({Cyc_Absynpp_qvar2doc(q);});_tmp536[0]=_tmp962;}),({struct Cyc_PP_Doc*_tmp961=({struct Cyc_PP_Doc*(*_tmp537)(struct _fat_ptr start,struct _fat_ptr stop,struct _fat_ptr sep,struct Cyc_List_List*l)=Cyc_PP_group;struct _fat_ptr _tmp538=({const char*_tmp539="(";_tag_fat(_tmp539,sizeof(char),2U);});struct _fat_ptr _tmp53A=term;struct _fat_ptr _tmp53B=({const char*_tmp53C=",";_tag_fat(_tmp53C,sizeof(char),2U);});struct Cyc_List_List*_tmp53D=({({struct Cyc_List_List*(*_tmp960)(struct Cyc_PP_Doc*(*f)(struct Cyc_Absyn_Pat*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp53E)(struct Cyc_PP_Doc*(*f)(struct Cyc_Absyn_Pat*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_PP_Doc*(*f)(struct Cyc_Absyn_Pat*),struct Cyc_List_List*x))Cyc_List_map;_tmp53E;});_tmp960(Cyc_Absynpp_pat2doc,ps);});});_tmp537(_tmp538,_tmp53A,_tmp53B,_tmp53D);});_tmp536[1]=_tmp961;});Cyc_PP_cat(_tag_fat(_tmp536,sizeof(struct Cyc_PP_Doc*),2U));});
goto _LL0;}case 7U: if(((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp4EB)->f1 != 0){_LL1F: _tmp4F6=*((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp4EB)->f1;_tmp4F7=((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp4EB)->f2;_tmp4F8=((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp4EB)->f3;_tmp4F9=((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp4EB)->f4;_LL20: {union Cyc_Absyn_AggrInfo info=_tmp4F6;struct Cyc_List_List*exists=_tmp4F7;struct Cyc_List_List*dps=_tmp4F8;int dots=_tmp4F9;
# 1649
struct _fat_ptr term=dots?({const char*_tmp554=", ...}";_tag_fat(_tmp554,sizeof(char),7U);}):({const char*_tmp555="}";_tag_fat(_tmp555,sizeof(char),2U);});
struct _tuple12 _stmttmp1D=({Cyc_Absyn_aggr_kinded_name(info);});struct _tuple0*_tmp541;_LL2E: _tmp541=_stmttmp1D.f2;_LL2F: {struct _tuple0*n=_tmp541;
s=({struct Cyc_PP_Doc*_tmp542[4U];({struct Cyc_PP_Doc*_tmp968=({Cyc_Absynpp_qvar2doc(n);});_tmp542[0]=_tmp968;}),({struct Cyc_PP_Doc*_tmp967=({Cyc_Absynpp_lb();});_tmp542[1]=_tmp967;}),({
struct Cyc_PP_Doc*_tmp966=({struct Cyc_PP_Doc*(*_tmp543)(struct _fat_ptr start,struct _fat_ptr stop,struct _fat_ptr sep,struct Cyc_List_List*l)=Cyc_PP_egroup;struct _fat_ptr _tmp544=({const char*_tmp545="[";_tag_fat(_tmp545,sizeof(char),2U);});struct _fat_ptr _tmp546=({const char*_tmp547="]";_tag_fat(_tmp547,sizeof(char),2U);});struct _fat_ptr _tmp548=({const char*_tmp549=",";_tag_fat(_tmp549,sizeof(char),2U);});struct Cyc_List_List*_tmp54A=({({struct Cyc_List_List*(*_tmp965)(struct Cyc_PP_Doc*(*f)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp54B)(struct Cyc_PP_Doc*(*f)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_PP_Doc*(*f)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*x))Cyc_List_map;_tmp54B;});_tmp965(Cyc_Absynpp_tvar2doc,exists);});});_tmp543(_tmp544,_tmp546,_tmp548,_tmp54A);});_tmp542[2]=_tmp966;}),({
struct Cyc_PP_Doc*_tmp964=({struct Cyc_PP_Doc*(*_tmp54C)(struct _fat_ptr start,struct _fat_ptr stop,struct _fat_ptr sep,struct Cyc_List_List*l)=Cyc_PP_group;struct _fat_ptr _tmp54D=({const char*_tmp54E="";_tag_fat(_tmp54E,sizeof(char),1U);});struct _fat_ptr _tmp54F=term;struct _fat_ptr _tmp550=({const char*_tmp551=",";_tag_fat(_tmp551,sizeof(char),2U);});struct Cyc_List_List*_tmp552=({({struct Cyc_List_List*(*_tmp963)(struct Cyc_PP_Doc*(*f)(struct _tuple16*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp553)(struct Cyc_PP_Doc*(*f)(struct _tuple16*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_PP_Doc*(*f)(struct _tuple16*),struct Cyc_List_List*x))Cyc_List_map;_tmp553;});_tmp963(Cyc_Absynpp_dp2doc,dps);});});_tmp54C(_tmp54D,_tmp54F,_tmp550,_tmp552);});_tmp542[3]=_tmp964;});Cyc_PP_cat(_tag_fat(_tmp542,sizeof(struct Cyc_PP_Doc*),4U));});
goto _LL0;}}}else{_LL21: _tmp4F3=((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp4EB)->f2;_tmp4F4=((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp4EB)->f3;_tmp4F5=((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp4EB)->f4;_LL22: {struct Cyc_List_List*exists=_tmp4F3;struct Cyc_List_List*dps=_tmp4F4;int dots=_tmp4F5;
# 1656
struct _fat_ptr term=dots?({const char*_tmp568=", ...}";_tag_fat(_tmp568,sizeof(char),7U);}):({const char*_tmp569="}";_tag_fat(_tmp569,sizeof(char),2U);});
s=({struct Cyc_PP_Doc*_tmp556[3U];({struct Cyc_PP_Doc*_tmp96D=({Cyc_Absynpp_lb();});_tmp556[0]=_tmp96D;}),({
struct Cyc_PP_Doc*_tmp96C=({struct Cyc_PP_Doc*(*_tmp557)(struct _fat_ptr start,struct _fat_ptr stop,struct _fat_ptr sep,struct Cyc_List_List*l)=Cyc_PP_egroup;struct _fat_ptr _tmp558=({const char*_tmp559="[";_tag_fat(_tmp559,sizeof(char),2U);});struct _fat_ptr _tmp55A=({const char*_tmp55B="]";_tag_fat(_tmp55B,sizeof(char),2U);});struct _fat_ptr _tmp55C=({const char*_tmp55D=",";_tag_fat(_tmp55D,sizeof(char),2U);});struct Cyc_List_List*_tmp55E=({({struct Cyc_List_List*(*_tmp96B)(struct Cyc_PP_Doc*(*f)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp55F)(struct Cyc_PP_Doc*(*f)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_PP_Doc*(*f)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*x))Cyc_List_map;_tmp55F;});_tmp96B(Cyc_Absynpp_tvar2doc,exists);});});_tmp557(_tmp558,_tmp55A,_tmp55C,_tmp55E);});_tmp556[1]=_tmp96C;}),({
struct Cyc_PP_Doc*_tmp96A=({struct Cyc_PP_Doc*(*_tmp560)(struct _fat_ptr start,struct _fat_ptr stop,struct _fat_ptr sep,struct Cyc_List_List*l)=Cyc_PP_group;struct _fat_ptr _tmp561=({const char*_tmp562="";_tag_fat(_tmp562,sizeof(char),1U);});struct _fat_ptr _tmp563=term;struct _fat_ptr _tmp564=({const char*_tmp565=",";_tag_fat(_tmp565,sizeof(char),2U);});struct Cyc_List_List*_tmp566=({({struct Cyc_List_List*(*_tmp969)(struct Cyc_PP_Doc*(*f)(struct _tuple16*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp567)(struct Cyc_PP_Doc*(*f)(struct _tuple16*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_PP_Doc*(*f)(struct _tuple16*),struct Cyc_List_List*x))Cyc_List_map;_tmp567;});_tmp969(Cyc_Absynpp_dp2doc,dps);});});_tmp560(_tmp561,_tmp563,_tmp564,_tmp566);});_tmp556[2]=_tmp96A;});Cyc_PP_cat(_tag_fat(_tmp556,sizeof(struct Cyc_PP_Doc*),3U));});
goto _LL0;}}case 13U: _LL23: _tmp4F2=((struct Cyc_Absyn_Enum_p_Absyn_Raw_pat_struct*)_tmp4EB)->f2;_LL24: {struct Cyc_Absyn_Enumfield*ef=_tmp4F2;
_tmp4F1=ef;goto _LL26;}case 14U: _LL25: _tmp4F1=((struct Cyc_Absyn_AnonEnum_p_Absyn_Raw_pat_struct*)_tmp4EB)->f2;_LL26: {struct Cyc_Absyn_Enumfield*ef=_tmp4F1;
s=({Cyc_Absynpp_qvar2doc(ef->name);});goto _LL0;}case 8U: if(((struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct*)_tmp4EB)->f3 == 0){_LL27: _tmp4F0=((struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct*)_tmp4EB)->f2;_LL28: {struct Cyc_Absyn_Datatypefield*tuf=_tmp4F0;
s=({Cyc_Absynpp_qvar2doc(tuf->name);});goto _LL0;}}else{_LL29: _tmp4ED=((struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct*)_tmp4EB)->f2;_tmp4EE=((struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct*)_tmp4EB)->f3;_tmp4EF=((struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct*)_tmp4EB)->f4;_LL2A: {struct Cyc_Absyn_Datatypefield*tuf=_tmp4ED;struct Cyc_List_List*ps=_tmp4EE;int dots=_tmp4EF;
# 1665
struct _fat_ptr term=dots?({const char*_tmp573=", ...)";_tag_fat(_tmp573,sizeof(char),7U);}):({const char*_tmp574=")";_tag_fat(_tmp574,sizeof(char),2U);});
s=({struct Cyc_PP_Doc*_tmp56A[2U];({struct Cyc_PP_Doc*_tmp970=({Cyc_Absynpp_qvar2doc(tuf->name);});_tmp56A[0]=_tmp970;}),({struct Cyc_PP_Doc*_tmp96F=({struct Cyc_PP_Doc*(*_tmp56B)(struct _fat_ptr start,struct _fat_ptr stop,struct _fat_ptr sep,struct Cyc_List_List*l)=Cyc_PP_egroup;struct _fat_ptr _tmp56C=({const char*_tmp56D="(";_tag_fat(_tmp56D,sizeof(char),2U);});struct _fat_ptr _tmp56E=term;struct _fat_ptr _tmp56F=({const char*_tmp570=",";_tag_fat(_tmp570,sizeof(char),2U);});struct Cyc_List_List*_tmp571=({({struct Cyc_List_List*(*_tmp96E)(struct Cyc_PP_Doc*(*f)(struct Cyc_Absyn_Pat*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp572)(struct Cyc_PP_Doc*(*f)(struct Cyc_Absyn_Pat*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_PP_Doc*(*f)(struct Cyc_Absyn_Pat*),struct Cyc_List_List*x))Cyc_List_map;_tmp572;});_tmp96E(Cyc_Absynpp_pat2doc,ps);});});_tmp56B(_tmp56C,_tmp56E,_tmp56F,_tmp571);});_tmp56A[1]=_tmp96F;});Cyc_PP_cat(_tag_fat(_tmp56A,sizeof(struct Cyc_PP_Doc*),2U));});
goto _LL0;}}default: _LL2B: _tmp4EC=((struct Cyc_Absyn_Exp_p_Absyn_Raw_pat_struct*)_tmp4EB)->f1;_LL2C: {struct Cyc_Absyn_Exp*e=_tmp4EC;
# 1669
s=({Cyc_Absynpp_exp2doc(e);});goto _LL0;}}_LL0:;}
# 1671
return s;}
# 1478
struct Cyc_PP_Doc*Cyc_Absynpp_dp2doc(struct _tuple16*dp){
# 1675
return({struct Cyc_PP_Doc*_tmp576[2U];({struct Cyc_PP_Doc*_tmp973=({struct Cyc_PP_Doc*(*_tmp577)(struct _fat_ptr start,struct _fat_ptr stop,struct _fat_ptr sep,struct Cyc_List_List*l)=Cyc_PP_egroup;struct _fat_ptr _tmp578=({const char*_tmp579="";_tag_fat(_tmp579,sizeof(char),1U);});struct _fat_ptr _tmp57A=({const char*_tmp57B="=";_tag_fat(_tmp57B,sizeof(char),2U);});struct _fat_ptr _tmp57C=({const char*_tmp57D="=";_tag_fat(_tmp57D,sizeof(char),2U);});struct Cyc_List_List*_tmp57E=({({struct Cyc_List_List*(*_tmp972)(struct Cyc_PP_Doc*(*f)(void*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp57F)(struct Cyc_PP_Doc*(*f)(void*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_PP_Doc*(*f)(void*),struct Cyc_List_List*x))Cyc_List_map;_tmp57F;});_tmp972(Cyc_Absynpp_designator2doc,(*dp).f1);});});_tmp577(_tmp578,_tmp57A,_tmp57C,_tmp57E);});_tmp576[0]=_tmp973;}),({
struct Cyc_PP_Doc*_tmp971=({Cyc_Absynpp_pat2doc((*dp).f2);});_tmp576[1]=_tmp971;});Cyc_PP_cat(_tag_fat(_tmp576,sizeof(struct Cyc_PP_Doc*),2U));});}
# 1478
struct Cyc_PP_Doc*Cyc_Absynpp_switchclause2doc(struct Cyc_Absyn_Switch_clause*c){
# 1681
struct Cyc_PP_Doc*body=({Cyc_Absynpp_stmt2doc(c->body,0,0);});
if(Cyc_Absynpp_decls_first &&({Cyc_Absynpp_is_declaration(c->body);}))
body=({Cyc_Absynpp_block(0,body);});
# 1682
if(
# 1684
c->where_clause == 0 &&(c->pattern)->r == (void*)& Cyc_Absyn_Wild_p_val)
return({struct Cyc_PP_Doc*_tmp581[2U];({struct Cyc_PP_Doc*_tmp976=({Cyc_PP_text(({const char*_tmp582="default: ";_tag_fat(_tmp582,sizeof(char),10U);}));});_tmp581[0]=_tmp976;}),({
struct Cyc_PP_Doc*_tmp975=({struct Cyc_PP_Doc*(*_tmp583)(int k,struct Cyc_PP_Doc*d)=Cyc_PP_nest;int _tmp584=2;struct Cyc_PP_Doc*_tmp585=({struct Cyc_PP_Doc*_tmp586[2U];({struct Cyc_PP_Doc*_tmp974=({Cyc_PP_line_doc();});_tmp586[0]=_tmp974;}),_tmp586[1]=body;Cyc_PP_cat(_tag_fat(_tmp586,sizeof(struct Cyc_PP_Doc*),2U));});_tmp583(_tmp584,_tmp585);});_tmp581[1]=_tmp975;});Cyc_PP_cat(_tag_fat(_tmp581,sizeof(struct Cyc_PP_Doc*),2U));});else{
if(c->where_clause == 0)
return({struct Cyc_PP_Doc*_tmp587[4U];({struct Cyc_PP_Doc*_tmp97B=({Cyc_PP_text(({const char*_tmp588="case ";_tag_fat(_tmp588,sizeof(char),6U);}));});_tmp587[0]=_tmp97B;}),({
struct Cyc_PP_Doc*_tmp97A=({Cyc_Absynpp_pat2doc(c->pattern);});_tmp587[1]=_tmp97A;}),({
struct Cyc_PP_Doc*_tmp979=({Cyc_PP_text(({const char*_tmp589=": ";_tag_fat(_tmp589,sizeof(char),3U);}));});_tmp587[2]=_tmp979;}),({
struct Cyc_PP_Doc*_tmp978=({struct Cyc_PP_Doc*(*_tmp58A)(int k,struct Cyc_PP_Doc*d)=Cyc_PP_nest;int _tmp58B=2;struct Cyc_PP_Doc*_tmp58C=({struct Cyc_PP_Doc*_tmp58D[2U];({struct Cyc_PP_Doc*_tmp977=({Cyc_PP_line_doc();});_tmp58D[0]=_tmp977;}),_tmp58D[1]=body;Cyc_PP_cat(_tag_fat(_tmp58D,sizeof(struct Cyc_PP_Doc*),2U));});_tmp58A(_tmp58B,_tmp58C);});_tmp587[3]=_tmp978;});Cyc_PP_cat(_tag_fat(_tmp587,sizeof(struct Cyc_PP_Doc*),4U));});else{
# 1693
return({struct Cyc_PP_Doc*_tmp58E[6U];({struct Cyc_PP_Doc*_tmp982=({Cyc_PP_text(({const char*_tmp58F="case ";_tag_fat(_tmp58F,sizeof(char),6U);}));});_tmp58E[0]=_tmp982;}),({
struct Cyc_PP_Doc*_tmp981=({Cyc_Absynpp_pat2doc(c->pattern);});_tmp58E[1]=_tmp981;}),({
struct Cyc_PP_Doc*_tmp980=({Cyc_PP_text(({const char*_tmp590=" && ";_tag_fat(_tmp590,sizeof(char),5U);}));});_tmp58E[2]=_tmp980;}),({
struct Cyc_PP_Doc*_tmp97F=({Cyc_Absynpp_exp2doc((struct Cyc_Absyn_Exp*)_check_null(c->where_clause));});_tmp58E[3]=_tmp97F;}),({
struct Cyc_PP_Doc*_tmp97E=({Cyc_PP_text(({const char*_tmp591=": ";_tag_fat(_tmp591,sizeof(char),3U);}));});_tmp58E[4]=_tmp97E;}),({
struct Cyc_PP_Doc*_tmp97D=({struct Cyc_PP_Doc*(*_tmp592)(int k,struct Cyc_PP_Doc*d)=Cyc_PP_nest;int _tmp593=2;struct Cyc_PP_Doc*_tmp594=({struct Cyc_PP_Doc*_tmp595[2U];({struct Cyc_PP_Doc*_tmp97C=({Cyc_PP_line_doc();});_tmp595[0]=_tmp97C;}),_tmp595[1]=body;Cyc_PP_cat(_tag_fat(_tmp595,sizeof(struct Cyc_PP_Doc*),2U));});_tmp592(_tmp593,_tmp594);});_tmp58E[5]=_tmp97D;});Cyc_PP_cat(_tag_fat(_tmp58E,sizeof(struct Cyc_PP_Doc*),6U));});}}}
# 1478
struct Cyc_PP_Doc*Cyc_Absynpp_switchclauses2doc(struct Cyc_List_List*cs){
# 1702
return({({struct Cyc_PP_Doc*(*_tmp984)(struct Cyc_PP_Doc*(*pp)(struct Cyc_Absyn_Switch_clause*),struct _fat_ptr sep,struct Cyc_List_List*l)=({struct Cyc_PP_Doc*(*_tmp597)(struct Cyc_PP_Doc*(*pp)(struct Cyc_Absyn_Switch_clause*),struct _fat_ptr sep,struct Cyc_List_List*l)=(struct Cyc_PP_Doc*(*)(struct Cyc_PP_Doc*(*pp)(struct Cyc_Absyn_Switch_clause*),struct _fat_ptr sep,struct Cyc_List_List*l))Cyc_PP_ppseql;_tmp597;});struct _fat_ptr _tmp983=({const char*_tmp598="";_tag_fat(_tmp598,sizeof(char),1U);});_tmp984(Cyc_Absynpp_switchclause2doc,_tmp983,cs);});});}
# 1478
struct Cyc_PP_Doc*Cyc_Absynpp_enumfield2doc(struct Cyc_Absyn_Enumfield*f){
# 1706
if(f->tag == 0)
return({Cyc_Absynpp_qvar2doc(f->name);});else{
# 1709
return({struct Cyc_PP_Doc*_tmp59A[3U];({struct Cyc_PP_Doc*_tmp987=({Cyc_Absynpp_qvar2doc(f->name);});_tmp59A[0]=_tmp987;}),({struct Cyc_PP_Doc*_tmp986=({Cyc_PP_text(({const char*_tmp59B=" = ";_tag_fat(_tmp59B,sizeof(char),4U);}));});_tmp59A[1]=_tmp986;}),({struct Cyc_PP_Doc*_tmp985=({Cyc_Absynpp_exp2doc((struct Cyc_Absyn_Exp*)_check_null(f->tag));});_tmp59A[2]=_tmp985;});Cyc_PP_cat(_tag_fat(_tmp59A,sizeof(struct Cyc_PP_Doc*),3U));});}}
# 1478
struct Cyc_PP_Doc*Cyc_Absynpp_enumfields2doc(struct Cyc_List_List*fs){
# 1713
return({({struct Cyc_PP_Doc*(*_tmp989)(struct Cyc_PP_Doc*(*pp)(struct Cyc_Absyn_Enumfield*),struct _fat_ptr sep,struct Cyc_List_List*l)=({struct Cyc_PP_Doc*(*_tmp59D)(struct Cyc_PP_Doc*(*pp)(struct Cyc_Absyn_Enumfield*),struct _fat_ptr sep,struct Cyc_List_List*l)=(struct Cyc_PP_Doc*(*)(struct Cyc_PP_Doc*(*pp)(struct Cyc_Absyn_Enumfield*),struct _fat_ptr sep,struct Cyc_List_List*l))Cyc_PP_ppseql;_tmp59D;});struct _fat_ptr _tmp988=({const char*_tmp59E=",";_tag_fat(_tmp59E,sizeof(char),2U);});_tmp989(Cyc_Absynpp_enumfield2doc,_tmp988,fs);});});}
# 1716
static struct Cyc_PP_Doc*Cyc_Absynpp_id2doc(struct Cyc_Absyn_Vardecl*v){
return({Cyc_Absynpp_qvar2doc(v->name);});}
# 1720
static struct Cyc_PP_Doc*Cyc_Absynpp_ids2doc(struct Cyc_List_List*vds){
return({({struct Cyc_PP_Doc*(*_tmp98B)(struct Cyc_PP_Doc*(*pp)(struct Cyc_Absyn_Vardecl*),struct _fat_ptr sep,struct Cyc_List_List*l)=({struct Cyc_PP_Doc*(*_tmp5A1)(struct Cyc_PP_Doc*(*pp)(struct Cyc_Absyn_Vardecl*),struct _fat_ptr sep,struct Cyc_List_List*l)=(struct Cyc_PP_Doc*(*)(struct Cyc_PP_Doc*(*pp)(struct Cyc_Absyn_Vardecl*),struct _fat_ptr sep,struct Cyc_List_List*l))Cyc_PP_ppseq;_tmp5A1;});struct _fat_ptr _tmp98A=({const char*_tmp5A2=",";_tag_fat(_tmp5A2,sizeof(char),2U);});_tmp98B(Cyc_Absynpp_id2doc,_tmp98A,vds);});});}
# 1720
struct Cyc_PP_Doc*Cyc_Absynpp_vardecl2doc(struct Cyc_Absyn_Vardecl*vd){
# 1725
struct Cyc_List_List*_tmp5AA;struct Cyc_Absyn_Exp*_tmp5A9;void*_tmp5A8;struct Cyc_Absyn_Tqual _tmp5A7;unsigned _tmp5A6;struct _tuple0*_tmp5A5;enum Cyc_Absyn_Scope _tmp5A4;_LL1: _tmp5A4=vd->sc;_tmp5A5=vd->name;_tmp5A6=vd->varloc;_tmp5A7=vd->tq;_tmp5A8=vd->type;_tmp5A9=vd->initializer;_tmp5AA=vd->attributes;_LL2: {enum Cyc_Absyn_Scope sc=_tmp5A4;struct _tuple0*name=_tmp5A5;unsigned varloc=_tmp5A6;struct Cyc_Absyn_Tqual tq=_tmp5A7;void*type=_tmp5A8;struct Cyc_Absyn_Exp*initializer=_tmp5A9;struct Cyc_List_List*atts=_tmp5AA;
struct Cyc_PP_Doc*s;
struct Cyc_PP_Doc*sn=({Cyc_Absynpp_typedef_name2bolddoc(name);});
struct Cyc_PP_Doc*attsdoc=({Cyc_Absynpp_atts2doc(atts);});
struct Cyc_PP_Doc*beforenamedoc;
{enum Cyc_Cyclone_C_Compilers _tmp5AB=Cyc_Cyclone_c_compiler;if(_tmp5AB == Cyc_Cyclone_Gcc_c){_LL4: _LL5:
 beforenamedoc=attsdoc;goto _LL3;}else{_LL6: _LL7:
# 1733
{void*_stmttmp1E=({Cyc_Tcutil_compress(type);});void*_tmp5AC=_stmttmp1E;struct Cyc_List_List*_tmp5AD;if(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp5AC)->tag == 5U){_LL9: _tmp5AD=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp5AC)->f1).attributes;_LLA: {struct Cyc_List_List*atts2=_tmp5AD;
# 1735
beforenamedoc=({Cyc_Absynpp_callconv2doc(atts2);});
goto _LL8;}}else{_LLB: _LLC:
 beforenamedoc=({Cyc_PP_nil_doc();});goto _LL8;}_LL8:;}
# 1739
goto _LL3;}_LL3:;}{
# 1742
struct Cyc_PP_Doc*tmp_doc;
{enum Cyc_Cyclone_C_Compilers _tmp5AE=Cyc_Cyclone_c_compiler;if(_tmp5AE == Cyc_Cyclone_Gcc_c){_LLE: _LLF:
 tmp_doc=({Cyc_PP_nil_doc();});goto _LLD;}else{_LL10: _LL11:
 tmp_doc=attsdoc;goto _LLD;}_LLD:;}
# 1747
s=({struct Cyc_PP_Doc*_tmp5AF[5U];_tmp5AF[0]=tmp_doc,({
# 1749
struct Cyc_PP_Doc*_tmp993=({Cyc_Absynpp_scope2doc(sc);});_tmp5AF[1]=_tmp993;}),({
struct Cyc_PP_Doc*_tmp992=({struct Cyc_PP_Doc*(*_tmp5B0)(struct Cyc_Absyn_Tqual tq,void*t,struct Cyc_Core_Opt*dopt)=Cyc_Absynpp_tqtd2doc;struct Cyc_Absyn_Tqual _tmp5B1=tq;void*_tmp5B2=type;struct Cyc_Core_Opt*_tmp5B3=({struct Cyc_Core_Opt*_tmp5B5=_cycalloc(sizeof(*_tmp5B5));({struct Cyc_PP_Doc*_tmp991=({struct Cyc_PP_Doc*_tmp5B4[2U];_tmp5B4[0]=beforenamedoc,_tmp5B4[1]=sn;Cyc_PP_cat(_tag_fat(_tmp5B4,sizeof(struct Cyc_PP_Doc*),2U));});_tmp5B5->v=_tmp991;});_tmp5B5;});_tmp5B0(_tmp5B1,_tmp5B2,_tmp5B3);});_tmp5AF[2]=_tmp992;}),
initializer == 0?({
struct Cyc_PP_Doc*_tmp990=({Cyc_PP_nil_doc();});_tmp5AF[3]=_tmp990;}):({
struct Cyc_PP_Doc*_tmp98F=({struct Cyc_PP_Doc*_tmp5B6[2U];({struct Cyc_PP_Doc*_tmp98E=({Cyc_PP_text(({const char*_tmp5B7=" = ";_tag_fat(_tmp5B7,sizeof(char),4U);}));});_tmp5B6[0]=_tmp98E;}),({struct Cyc_PP_Doc*_tmp98D=({Cyc_Absynpp_exp2doc(initializer);});_tmp5B6[1]=_tmp98D;});Cyc_PP_cat(_tag_fat(_tmp5B6,sizeof(struct Cyc_PP_Doc*),2U));});_tmp5AF[3]=_tmp98F;}),({
struct Cyc_PP_Doc*_tmp98C=({Cyc_PP_text(({const char*_tmp5B8=";";_tag_fat(_tmp5B8,sizeof(char),2U);}));});_tmp5AF[4]=_tmp98C;});Cyc_PP_cat(_tag_fat(_tmp5AF,sizeof(struct Cyc_PP_Doc*),5U));});
return s;}}}struct _tuple21{unsigned f1;struct _tuple0*f2;int f3;};
# 1720
struct Cyc_PP_Doc*Cyc_Absynpp_export2doc(struct _tuple21*x){
# 1759
struct _tuple21 _stmttmp1F=*x;struct _tuple0*_tmp5BA;_LL1: _tmp5BA=_stmttmp1F.f2;_LL2: {struct _tuple0*v=_tmp5BA;
return({Cyc_Absynpp_qvar2doc(v);});}}
# 1720
struct Cyc_PP_Doc*Cyc_Absynpp_aggrdecl2doc(struct Cyc_Absyn_Aggrdecl*ad){
# 1764
if(ad->impl == 0)
return({struct Cyc_PP_Doc*_tmp5BC[4U];({struct Cyc_PP_Doc*_tmp997=({Cyc_Absynpp_scope2doc(ad->sc);});_tmp5BC[0]=_tmp997;}),({
struct Cyc_PP_Doc*_tmp996=({Cyc_Absynpp_aggr_kind2doc(ad->kind);});_tmp5BC[1]=_tmp996;}),({
struct Cyc_PP_Doc*_tmp995=({Cyc_Absynpp_qvar2bolddoc(ad->name);});_tmp5BC[2]=_tmp995;}),({
struct Cyc_PP_Doc*_tmp994=({Cyc_Absynpp_ktvars2doc(ad->tvs);});_tmp5BC[3]=_tmp994;});Cyc_PP_cat(_tag_fat(_tmp5BC,sizeof(struct Cyc_PP_Doc*),4U));});else{
# 1770
return({struct Cyc_PP_Doc*_tmp5BD[12U];({struct Cyc_PP_Doc*_tmp9A8=({Cyc_Absynpp_scope2doc(ad->sc);});_tmp5BD[0]=_tmp9A8;}),({
struct Cyc_PP_Doc*_tmp9A7=({Cyc_Absynpp_aggr_kind2doc(ad->kind);});_tmp5BD[1]=_tmp9A7;}),({
struct Cyc_PP_Doc*_tmp9A6=({Cyc_Absynpp_qvar2bolddoc(ad->name);});_tmp5BD[2]=_tmp9A6;}),({
struct Cyc_PP_Doc*_tmp9A5=({Cyc_Absynpp_ktvars2doc(ad->tvs);});_tmp5BD[3]=_tmp9A5;}),({
struct Cyc_PP_Doc*_tmp9A4=({Cyc_PP_blank_doc();});_tmp5BD[4]=_tmp9A4;}),({struct Cyc_PP_Doc*_tmp9A3=({Cyc_Absynpp_lb();});_tmp5BD[5]=_tmp9A3;}),({
struct Cyc_PP_Doc*_tmp9A2=({Cyc_Absynpp_ktvars2doc(((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->exist_vars);});_tmp5BD[6]=_tmp9A2;}),
((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->rgn_po == 0?({struct Cyc_PP_Doc*_tmp9A1=({Cyc_PP_nil_doc();});_tmp5BD[7]=_tmp9A1;}):({
struct Cyc_PP_Doc*_tmp9A0=({struct Cyc_PP_Doc*_tmp5BE[2U];({struct Cyc_PP_Doc*_tmp99F=({Cyc_PP_text(({const char*_tmp5BF=":";_tag_fat(_tmp5BF,sizeof(char),2U);}));});_tmp5BE[0]=_tmp99F;}),({struct Cyc_PP_Doc*_tmp99E=({Cyc_Absynpp_rgnpo2doc(((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->rgn_po);});_tmp5BE[1]=_tmp99E;});Cyc_PP_cat(_tag_fat(_tmp5BE,sizeof(struct Cyc_PP_Doc*),2U));});_tmp5BD[7]=_tmp9A0;}),({
struct Cyc_PP_Doc*_tmp99D=({struct Cyc_PP_Doc*(*_tmp5C0)(int k,struct Cyc_PP_Doc*d)=Cyc_PP_nest;int _tmp5C1=2;struct Cyc_PP_Doc*_tmp5C2=({struct Cyc_PP_Doc*_tmp5C3[2U];({struct Cyc_PP_Doc*_tmp99C=({Cyc_PP_line_doc();});_tmp5C3[0]=_tmp99C;}),({struct Cyc_PP_Doc*_tmp99B=({Cyc_Absynpp_aggrfields2doc(((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->fields);});_tmp5C3[1]=_tmp99B;});Cyc_PP_cat(_tag_fat(_tmp5C3,sizeof(struct Cyc_PP_Doc*),2U));});_tmp5C0(_tmp5C1,_tmp5C2);});_tmp5BD[8]=_tmp99D;}),({
struct Cyc_PP_Doc*_tmp99A=({Cyc_PP_line_doc();});_tmp5BD[9]=_tmp99A;}),({
struct Cyc_PP_Doc*_tmp999=({Cyc_Absynpp_rb();});_tmp5BD[10]=_tmp999;}),({
struct Cyc_PP_Doc*_tmp998=({Cyc_Absynpp_atts2doc(ad->attributes);});_tmp5BD[11]=_tmp998;});Cyc_PP_cat(_tag_fat(_tmp5BD,sizeof(struct Cyc_PP_Doc*),12U));});}}
# 1720
struct Cyc_PP_Doc*Cyc_Absynpp_datatypedecl2doc(struct Cyc_Absyn_Datatypedecl*dd){
# 1785
int _tmp5C9;struct Cyc_Core_Opt*_tmp5C8;struct Cyc_List_List*_tmp5C7;struct _tuple0*_tmp5C6;enum Cyc_Absyn_Scope _tmp5C5;_LL1: _tmp5C5=dd->sc;_tmp5C6=dd->name;_tmp5C7=dd->tvs;_tmp5C8=dd->fields;_tmp5C9=dd->is_extensible;_LL2: {enum Cyc_Absyn_Scope sc=_tmp5C5;struct _tuple0*name=_tmp5C6;struct Cyc_List_List*tvs=_tmp5C7;struct Cyc_Core_Opt*fields=_tmp5C8;int is_x=_tmp5C9;
if(fields == 0)
return({struct Cyc_PP_Doc*_tmp5CA[5U];({struct Cyc_PP_Doc*_tmp9AF=({Cyc_Absynpp_scope2doc(sc);});_tmp5CA[0]=_tmp9AF;}),
is_x?({struct Cyc_PP_Doc*_tmp9AE=({Cyc_PP_text(({const char*_tmp5CB="@extensible ";_tag_fat(_tmp5CB,sizeof(char),13U);}));});_tmp5CA[1]=_tmp9AE;}):({struct Cyc_PP_Doc*_tmp9AD=({Cyc_PP_blank_doc();});_tmp5CA[1]=_tmp9AD;}),({
struct Cyc_PP_Doc*_tmp9AC=({Cyc_PP_text(({const char*_tmp5CC="datatype ";_tag_fat(_tmp5CC,sizeof(char),10U);}));});_tmp5CA[2]=_tmp9AC;}),
is_x?({struct Cyc_PP_Doc*_tmp9AB=({Cyc_Absynpp_qvar2bolddoc(name);});_tmp5CA[3]=_tmp9AB;}):({struct Cyc_PP_Doc*_tmp9AA=({Cyc_Absynpp_typedef_name2bolddoc(name);});_tmp5CA[3]=_tmp9AA;}),({
struct Cyc_PP_Doc*_tmp9A9=({Cyc_Absynpp_ktvars2doc(tvs);});_tmp5CA[4]=_tmp9A9;});Cyc_PP_cat(_tag_fat(_tmp5CA,sizeof(struct Cyc_PP_Doc*),5U));});else{
# 1793
return({struct Cyc_PP_Doc*_tmp5CD[10U];({struct Cyc_PP_Doc*_tmp9BD=({Cyc_Absynpp_scope2doc(sc);});_tmp5CD[0]=_tmp9BD;}),
is_x?({struct Cyc_PP_Doc*_tmp9BC=({Cyc_PP_text(({const char*_tmp5CE="@extensible ";_tag_fat(_tmp5CE,sizeof(char),13U);}));});_tmp5CD[1]=_tmp9BC;}):({struct Cyc_PP_Doc*_tmp9BB=({Cyc_PP_blank_doc();});_tmp5CD[1]=_tmp9BB;}),({
struct Cyc_PP_Doc*_tmp9BA=({Cyc_PP_text(({const char*_tmp5CF="datatype ";_tag_fat(_tmp5CF,sizeof(char),10U);}));});_tmp5CD[2]=_tmp9BA;}),
is_x?({struct Cyc_PP_Doc*_tmp9B9=({Cyc_Absynpp_qvar2bolddoc(name);});_tmp5CD[3]=_tmp9B9;}):({struct Cyc_PP_Doc*_tmp9B8=({Cyc_Absynpp_typedef_name2bolddoc(name);});_tmp5CD[3]=_tmp9B8;}),({
struct Cyc_PP_Doc*_tmp9B7=({Cyc_Absynpp_ktvars2doc(tvs);});_tmp5CD[4]=_tmp9B7;}),({
struct Cyc_PP_Doc*_tmp9B6=({Cyc_PP_blank_doc();});_tmp5CD[5]=_tmp9B6;}),({struct Cyc_PP_Doc*_tmp9B5=({Cyc_Absynpp_lb();});_tmp5CD[6]=_tmp9B5;}),({
struct Cyc_PP_Doc*_tmp9B4=({struct Cyc_PP_Doc*(*_tmp5D0)(int k,struct Cyc_PP_Doc*d)=Cyc_PP_nest;int _tmp5D1=2;struct Cyc_PP_Doc*_tmp5D2=({struct Cyc_PP_Doc*_tmp5D3[2U];({struct Cyc_PP_Doc*_tmp9B3=({Cyc_PP_line_doc();});_tmp5D3[0]=_tmp9B3;}),({struct Cyc_PP_Doc*_tmp9B2=({Cyc_Absynpp_datatypefields2doc((struct Cyc_List_List*)fields->v);});_tmp5D3[1]=_tmp9B2;});Cyc_PP_cat(_tag_fat(_tmp5D3,sizeof(struct Cyc_PP_Doc*),2U));});_tmp5D0(_tmp5D1,_tmp5D2);});_tmp5CD[7]=_tmp9B4;}),({
struct Cyc_PP_Doc*_tmp9B1=({Cyc_PP_line_doc();});_tmp5CD[8]=_tmp9B1;}),({
struct Cyc_PP_Doc*_tmp9B0=({Cyc_Absynpp_rb();});_tmp5CD[9]=_tmp9B0;});Cyc_PP_cat(_tag_fat(_tmp5CD,sizeof(struct Cyc_PP_Doc*),10U));});}}}
# 1720
struct Cyc_PP_Doc*Cyc_Absynpp_enumdecl2doc(struct Cyc_Absyn_Enumdecl*ed){
# 1805
struct Cyc_Core_Opt*_tmp5D7;struct _tuple0*_tmp5D6;enum Cyc_Absyn_Scope _tmp5D5;_LL1: _tmp5D5=ed->sc;_tmp5D6=ed->name;_tmp5D7=ed->fields;_LL2: {enum Cyc_Absyn_Scope sc=_tmp5D5;struct _tuple0*n=_tmp5D6;struct Cyc_Core_Opt*fields=_tmp5D7;
if(fields == 0)
return({struct Cyc_PP_Doc*_tmp5D8[3U];({struct Cyc_PP_Doc*_tmp9C0=({Cyc_Absynpp_scope2doc(sc);});_tmp5D8[0]=_tmp9C0;}),({
struct Cyc_PP_Doc*_tmp9BF=({Cyc_PP_text(({const char*_tmp5D9="enum ";_tag_fat(_tmp5D9,sizeof(char),6U);}));});_tmp5D8[1]=_tmp9BF;}),({
struct Cyc_PP_Doc*_tmp9BE=({Cyc_Absynpp_typedef_name2bolddoc(n);});_tmp5D8[2]=_tmp9BE;});Cyc_PP_cat(_tag_fat(_tmp5D8,sizeof(struct Cyc_PP_Doc*),3U));});else{
# 1812
return({struct Cyc_PP_Doc*_tmp5DA[8U];({struct Cyc_PP_Doc*_tmp9CA=({Cyc_Absynpp_scope2doc(sc);});_tmp5DA[0]=_tmp9CA;}),({
struct Cyc_PP_Doc*_tmp9C9=({Cyc_PP_text(({const char*_tmp5DB="enum ";_tag_fat(_tmp5DB,sizeof(char),6U);}));});_tmp5DA[1]=_tmp9C9;}),({
struct Cyc_PP_Doc*_tmp9C8=({Cyc_Absynpp_qvar2bolddoc(n);});_tmp5DA[2]=_tmp9C8;}),({
struct Cyc_PP_Doc*_tmp9C7=({Cyc_PP_blank_doc();});_tmp5DA[3]=_tmp9C7;}),({struct Cyc_PP_Doc*_tmp9C6=({Cyc_Absynpp_lb();});_tmp5DA[4]=_tmp9C6;}),({
struct Cyc_PP_Doc*_tmp9C5=({struct Cyc_PP_Doc*(*_tmp5DC)(int k,struct Cyc_PP_Doc*d)=Cyc_PP_nest;int _tmp5DD=2;struct Cyc_PP_Doc*_tmp5DE=({struct Cyc_PP_Doc*_tmp5DF[2U];({struct Cyc_PP_Doc*_tmp9C4=({Cyc_PP_line_doc();});_tmp5DF[0]=_tmp9C4;}),({struct Cyc_PP_Doc*_tmp9C3=({Cyc_Absynpp_enumfields2doc((struct Cyc_List_List*)fields->v);});_tmp5DF[1]=_tmp9C3;});Cyc_PP_cat(_tag_fat(_tmp5DF,sizeof(struct Cyc_PP_Doc*),2U));});_tmp5DC(_tmp5DD,_tmp5DE);});_tmp5DA[5]=_tmp9C5;}),({
struct Cyc_PP_Doc*_tmp9C2=({Cyc_PP_line_doc();});_tmp5DA[6]=_tmp9C2;}),({
struct Cyc_PP_Doc*_tmp9C1=({Cyc_Absynpp_rb();});_tmp5DA[7]=_tmp9C1;});Cyc_PP_cat(_tag_fat(_tmp5DA,sizeof(struct Cyc_PP_Doc*),8U));});}}}
# 1720
struct Cyc_PP_Doc*Cyc_Absynpp_decl2doc(struct Cyc_Absyn_Decl*d){
# 1822
struct Cyc_PP_Doc*s;
{void*_stmttmp20=d->r;void*_tmp5E1=_stmttmp20;struct _tuple11*_tmp5E5;struct Cyc_List_List*_tmp5E4;struct Cyc_List_List*_tmp5E3;struct Cyc_List_List*_tmp5E2;struct Cyc_List_List*_tmp5E6;struct Cyc_List_List*_tmp5E8;struct _tuple0*_tmp5E7;struct Cyc_List_List*_tmp5EA;struct _fat_ptr*_tmp5E9;struct Cyc_Absyn_Typedefdecl*_tmp5EB;struct Cyc_Absyn_Enumdecl*_tmp5EC;struct Cyc_List_List*_tmp5ED;struct Cyc_Absyn_Exp*_tmp5EF;struct Cyc_Absyn_Pat*_tmp5EE;struct Cyc_Absyn_Datatypedecl*_tmp5F0;struct Cyc_Absyn_Exp*_tmp5F3;struct Cyc_Absyn_Vardecl*_tmp5F2;struct Cyc_Absyn_Tvar*_tmp5F1;struct Cyc_Absyn_Vardecl*_tmp5F4;struct Cyc_Absyn_Aggrdecl*_tmp5F5;struct Cyc_Absyn_Fndecl*_tmp5F6;switch(*((int*)_tmp5E1)){case 1U: _LL1: _tmp5F6=((struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*)_tmp5E1)->f1;_LL2: {struct Cyc_Absyn_Fndecl*fd=_tmp5F6;
# 1825
struct Cyc_Absyn_FnInfo type_info=fd->i;
type_info.attributes=0;{
void*t=(void*)({struct Cyc_Absyn_FnType_Absyn_Type_struct*_tmp60E=_cycalloc(sizeof(*_tmp60E));_tmp60E->tag=5U,_tmp60E->f1=type_info;_tmp60E;});
if(fd->cached_type != 0){
void*_stmttmp21=({Cyc_Tcutil_compress((void*)_check_null(fd->cached_type));});void*_tmp5F7=_stmttmp21;struct Cyc_Absyn_FnInfo _tmp5F8;if(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp5F7)->tag == 5U){_LL24: _tmp5F8=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp5F7)->f1;_LL25: {struct Cyc_Absyn_FnInfo i=_tmp5F8;
# 1831
({struct Cyc_List_List*_tmp9CB=({Cyc_List_append((fd->i).attributes,i.attributes);});(fd->i).attributes=_tmp9CB;});goto _LL23;}}else{_LL26: _LL27:
({void*_tmp5F9=0U;({int(*_tmp9CD)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp5FB)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp5FB;});struct _fat_ptr _tmp9CC=({const char*_tmp5FA="function has non-function type";_tag_fat(_tmp5FA,sizeof(char),31U);});_tmp9CD(_tmp9CC,_tag_fat(_tmp5F9,sizeof(void*),0U));});});}_LL23:;}{
# 1828
struct Cyc_PP_Doc*attsdoc=({Cyc_Absynpp_atts2doc((fd->i).attributes);});
# 1835
struct Cyc_PP_Doc*inlinedoc;
if(fd->is_inline){
enum Cyc_Cyclone_C_Compilers _tmp5FC=Cyc_Cyclone_c_compiler;if(_tmp5FC == Cyc_Cyclone_Gcc_c){_LL29: _LL2A:
 inlinedoc=({Cyc_PP_text(({const char*_tmp5FD="inline ";_tag_fat(_tmp5FD,sizeof(char),8U);}));});goto _LL28;}else{_LL2B: _LL2C:
 inlinedoc=({Cyc_PP_text(({const char*_tmp5FE="__inline ";_tag_fat(_tmp5FE,sizeof(char),10U);}));});goto _LL28;}_LL28:;}else{
# 1842
inlinedoc=({Cyc_PP_nil_doc();});}{
struct Cyc_PP_Doc*scopedoc=({Cyc_Absynpp_scope2doc(fd->sc);});
struct Cyc_PP_Doc*beforenamedoc;
{enum Cyc_Cyclone_C_Compilers _tmp5FF=Cyc_Cyclone_c_compiler;if(_tmp5FF == Cyc_Cyclone_Gcc_c){_LL2E: _LL2F:
 beforenamedoc=attsdoc;goto _LL2D;}else{_LL30: _LL31:
 beforenamedoc=({Cyc_Absynpp_callconv2doc((fd->i).attributes);});goto _LL2D;}_LL2D:;}{
# 1849
struct Cyc_PP_Doc*namedoc=({Cyc_Absynpp_typedef_name2doc(fd->name);});
struct Cyc_PP_Doc*tqtddoc=({struct Cyc_PP_Doc*(*_tmp608)(struct Cyc_Absyn_Tqual tq,void*t,struct Cyc_Core_Opt*dopt)=Cyc_Absynpp_tqtd2doc;struct Cyc_Absyn_Tqual _tmp609=({Cyc_Absyn_empty_tqual(0U);});void*_tmp60A=t;struct Cyc_Core_Opt*_tmp60B=({struct Cyc_Core_Opt*_tmp60D=_cycalloc(sizeof(*_tmp60D));({
struct Cyc_PP_Doc*_tmp9CE=({struct Cyc_PP_Doc*_tmp60C[2U];_tmp60C[0]=beforenamedoc,_tmp60C[1]=namedoc;Cyc_PP_cat(_tag_fat(_tmp60C,sizeof(struct Cyc_PP_Doc*),2U));});_tmp60D->v=_tmp9CE;});_tmp60D;});_tmp608(_tmp609,_tmp60A,_tmp60B);});
# 1857
struct Cyc_PP_Doc*bodydoc=({struct Cyc_PP_Doc*_tmp603[5U];({struct Cyc_PP_Doc*_tmp9D5=({Cyc_PP_blank_doc();});_tmp603[0]=_tmp9D5;}),({struct Cyc_PP_Doc*_tmp9D4=({Cyc_Absynpp_lb();});_tmp603[1]=_tmp9D4;}),({
struct Cyc_PP_Doc*_tmp9D3=({struct Cyc_PP_Doc*(*_tmp604)(int k,struct Cyc_PP_Doc*d)=Cyc_PP_nest;int _tmp605=2;struct Cyc_PP_Doc*_tmp606=({struct Cyc_PP_Doc*_tmp607[2U];({struct Cyc_PP_Doc*_tmp9D2=({Cyc_PP_line_doc();});_tmp607[0]=_tmp9D2;}),({struct Cyc_PP_Doc*_tmp9D1=({Cyc_Absynpp_stmt2doc(fd->body,0,0);});_tmp607[1]=_tmp9D1;});Cyc_PP_cat(_tag_fat(_tmp607,sizeof(struct Cyc_PP_Doc*),2U));});_tmp604(_tmp605,_tmp606);});_tmp603[2]=_tmp9D3;}),({
struct Cyc_PP_Doc*_tmp9D0=({Cyc_PP_line_doc();});_tmp603[3]=_tmp9D0;}),({
struct Cyc_PP_Doc*_tmp9CF=({Cyc_Absynpp_rb();});_tmp603[4]=_tmp9CF;});Cyc_PP_cat(_tag_fat(_tmp603,sizeof(struct Cyc_PP_Doc*),5U));});
s=({struct Cyc_PP_Doc*_tmp600[4U];_tmp600[0]=inlinedoc,_tmp600[1]=scopedoc,_tmp600[2]=tqtddoc,_tmp600[3]=bodydoc;Cyc_PP_cat(_tag_fat(_tmp600,sizeof(struct Cyc_PP_Doc*),4U));});
# 1863
{enum Cyc_Cyclone_C_Compilers _tmp601=Cyc_Cyclone_c_compiler;if(_tmp601 == Cyc_Cyclone_Vc_c){_LL33: _LL34:
 s=({struct Cyc_PP_Doc*_tmp602[2U];_tmp602[0]=attsdoc,_tmp602[1]=s;Cyc_PP_cat(_tag_fat(_tmp602,sizeof(struct Cyc_PP_Doc*),2U));});goto _LL32;}else{_LL35: _LL36:
 goto _LL32;}_LL32:;}
# 1868
goto _LL0;}}}}}case 5U: _LL3: _tmp5F5=((struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct*)_tmp5E1)->f1;_LL4: {struct Cyc_Absyn_Aggrdecl*ad=_tmp5F5;
# 1871
s=({struct Cyc_PP_Doc*_tmp60F[2U];({struct Cyc_PP_Doc*_tmp9D7=({Cyc_Absynpp_aggrdecl2doc(ad);});_tmp60F[0]=_tmp9D7;}),({struct Cyc_PP_Doc*_tmp9D6=({Cyc_PP_text(({const char*_tmp610=";";_tag_fat(_tmp610,sizeof(char),2U);}));});_tmp60F[1]=_tmp9D6;});Cyc_PP_cat(_tag_fat(_tmp60F,sizeof(struct Cyc_PP_Doc*),2U));});
goto _LL0;}case 0U: _LL5: _tmp5F4=((struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_tmp5E1)->f1;_LL6: {struct Cyc_Absyn_Vardecl*vd=_tmp5F4;
# 1874
s=({Cyc_Absynpp_vardecl2doc(vd);});
goto _LL0;}case 4U: _LL7: _tmp5F1=((struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*)_tmp5E1)->f1;_tmp5F2=((struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*)_tmp5E1)->f2;_tmp5F3=((struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*)_tmp5E1)->f3;_LL8: {struct Cyc_Absyn_Tvar*tv=_tmp5F1;struct Cyc_Absyn_Vardecl*vd=_tmp5F2;struct Cyc_Absyn_Exp*open_exp_opt=_tmp5F3;
# 1878
s=({struct Cyc_PP_Doc*_tmp611[5U];({struct Cyc_PP_Doc*_tmp9DC=({Cyc_PP_text(({const char*_tmp612="region";_tag_fat(_tmp612,sizeof(char),7U);}));});_tmp611[0]=_tmp9DC;}),({
struct Cyc_PP_Doc*_tmp9DB=({Cyc_PP_text(({const char*_tmp613="<";_tag_fat(_tmp613,sizeof(char),2U);}));});_tmp611[1]=_tmp9DB;}),({
struct Cyc_PP_Doc*_tmp9DA=({Cyc_Absynpp_tvar2doc(tv);});_tmp611[2]=_tmp9DA;}),({
struct Cyc_PP_Doc*_tmp9D9=({Cyc_PP_text(({const char*_tmp614=">";_tag_fat(_tmp614,sizeof(char),2U);}));});_tmp611[3]=_tmp9D9;}),({
struct Cyc_PP_Doc*_tmp9D8=({Cyc_Absynpp_qvar2doc(vd->name);});_tmp611[4]=_tmp9D8;});Cyc_PP_cat(_tag_fat(_tmp611,sizeof(struct Cyc_PP_Doc*),5U));});
# 1884
if(open_exp_opt != 0){
# 1886
if((int)open_exp_opt->loc > - 1)
s=({struct Cyc_PP_Doc*_tmp615[4U];_tmp615[0]=s,({struct Cyc_PP_Doc*_tmp9DF=({Cyc_PP_text(({const char*_tmp616=" = open(";_tag_fat(_tmp616,sizeof(char),9U);}));});_tmp615[1]=_tmp9DF;}),({struct Cyc_PP_Doc*_tmp9DE=({Cyc_Absynpp_exp2doc(open_exp_opt);});_tmp615[2]=_tmp9DE;}),({
struct Cyc_PP_Doc*_tmp9DD=({Cyc_PP_text(({const char*_tmp617=")";_tag_fat(_tmp617,sizeof(char),2U);}));});_tmp615[3]=_tmp9DD;});Cyc_PP_cat(_tag_fat(_tmp615,sizeof(struct Cyc_PP_Doc*),4U));});else{
# 1890
s=({struct Cyc_PP_Doc*_tmp618[3U];_tmp618[0]=s,({struct Cyc_PP_Doc*_tmp9E1=({Cyc_PP_text(({const char*_tmp619=" @ ";_tag_fat(_tmp619,sizeof(char),4U);}));});_tmp618[1]=_tmp9E1;}),({struct Cyc_PP_Doc*_tmp9E0=({Cyc_Absynpp_exp2doc(open_exp_opt);});_tmp618[2]=_tmp9E0;});Cyc_PP_cat(_tag_fat(_tmp618,sizeof(struct Cyc_PP_Doc*),3U));});}}
# 1884
s=({struct Cyc_PP_Doc*_tmp61A[2U];_tmp61A[0]=s,({
# 1892
struct Cyc_PP_Doc*_tmp9E2=({Cyc_PP_text(({const char*_tmp61B=";";_tag_fat(_tmp61B,sizeof(char),2U);}));});_tmp61A[1]=_tmp9E2;});Cyc_PP_cat(_tag_fat(_tmp61A,sizeof(struct Cyc_PP_Doc*),2U));});
goto _LL0;}case 6U: _LL9: _tmp5F0=((struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct*)_tmp5E1)->f1;_LLA: {struct Cyc_Absyn_Datatypedecl*dd=_tmp5F0;
# 1895
s=({struct Cyc_PP_Doc*_tmp61C[2U];({struct Cyc_PP_Doc*_tmp9E4=({Cyc_Absynpp_datatypedecl2doc(dd);});_tmp61C[0]=_tmp9E4;}),({struct Cyc_PP_Doc*_tmp9E3=({Cyc_PP_text(({const char*_tmp61D=";";_tag_fat(_tmp61D,sizeof(char),2U);}));});_tmp61C[1]=_tmp9E3;});Cyc_PP_cat(_tag_fat(_tmp61C,sizeof(struct Cyc_PP_Doc*),2U));});
goto _LL0;}case 2U: _LLB: _tmp5EE=((struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct*)_tmp5E1)->f1;_tmp5EF=((struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct*)_tmp5E1)->f3;_LLC: {struct Cyc_Absyn_Pat*p=_tmp5EE;struct Cyc_Absyn_Exp*e=_tmp5EF;
# 1898
s=({struct Cyc_PP_Doc*_tmp61E[5U];({struct Cyc_PP_Doc*_tmp9E9=({Cyc_PP_text(({const char*_tmp61F="let ";_tag_fat(_tmp61F,sizeof(char),5U);}));});_tmp61E[0]=_tmp9E9;}),({
struct Cyc_PP_Doc*_tmp9E8=({Cyc_Absynpp_pat2doc(p);});_tmp61E[1]=_tmp9E8;}),({
struct Cyc_PP_Doc*_tmp9E7=({Cyc_PP_text(({const char*_tmp620=" = ";_tag_fat(_tmp620,sizeof(char),4U);}));});_tmp61E[2]=_tmp9E7;}),({
struct Cyc_PP_Doc*_tmp9E6=({Cyc_Absynpp_exp2doc(e);});_tmp61E[3]=_tmp9E6;}),({
struct Cyc_PP_Doc*_tmp9E5=({Cyc_PP_text(({const char*_tmp621=";";_tag_fat(_tmp621,sizeof(char),2U);}));});_tmp61E[4]=_tmp9E5;});Cyc_PP_cat(_tag_fat(_tmp61E,sizeof(struct Cyc_PP_Doc*),5U));});
goto _LL0;}case 3U: _LLD: _tmp5ED=((struct Cyc_Absyn_Letv_d_Absyn_Raw_decl_struct*)_tmp5E1)->f1;_LLE: {struct Cyc_List_List*vds=_tmp5ED;
# 1905
s=({struct Cyc_PP_Doc*_tmp622[3U];({struct Cyc_PP_Doc*_tmp9EC=({Cyc_PP_text(({const char*_tmp623="let ";_tag_fat(_tmp623,sizeof(char),5U);}));});_tmp622[0]=_tmp9EC;}),({struct Cyc_PP_Doc*_tmp9EB=({Cyc_Absynpp_ids2doc(vds);});_tmp622[1]=_tmp9EB;}),({struct Cyc_PP_Doc*_tmp9EA=({Cyc_PP_text(({const char*_tmp624=";";_tag_fat(_tmp624,sizeof(char),2U);}));});_tmp622[2]=_tmp9EA;});Cyc_PP_cat(_tag_fat(_tmp622,sizeof(struct Cyc_PP_Doc*),3U));});
goto _LL0;}case 7U: _LLF: _tmp5EC=((struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct*)_tmp5E1)->f1;_LL10: {struct Cyc_Absyn_Enumdecl*ed=_tmp5EC;
# 1908
s=({struct Cyc_PP_Doc*_tmp625[2U];({struct Cyc_PP_Doc*_tmp9EE=({Cyc_Absynpp_enumdecl2doc(ed);});_tmp625[0]=_tmp9EE;}),({struct Cyc_PP_Doc*_tmp9ED=({Cyc_PP_text(({const char*_tmp626=";";_tag_fat(_tmp626,sizeof(char),2U);}));});_tmp625[1]=_tmp9ED;});Cyc_PP_cat(_tag_fat(_tmp625,sizeof(struct Cyc_PP_Doc*),2U));});
goto _LL0;}case 8U: _LL11: _tmp5EB=((struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct*)_tmp5E1)->f1;_LL12: {struct Cyc_Absyn_Typedefdecl*td=_tmp5EB;
# 1911
void*t;
if(td->defn != 0)
t=(void*)_check_null(td->defn);else{
# 1915
t=({Cyc_Absyn_new_evar(td->kind,0);});}
s=({struct Cyc_PP_Doc*_tmp627[4U];({struct Cyc_PP_Doc*_tmp9F5=({Cyc_PP_text(({const char*_tmp628="typedef ";_tag_fat(_tmp628,sizeof(char),9U);}));});_tmp627[0]=_tmp9F5;}),({
struct Cyc_PP_Doc*_tmp9F4=({struct Cyc_PP_Doc*(*_tmp629)(struct Cyc_Absyn_Tqual tq,void*t,struct Cyc_Core_Opt*dopt)=Cyc_Absynpp_tqtd2doc;struct Cyc_Absyn_Tqual _tmp62A=td->tq;void*_tmp62B=t;struct Cyc_Core_Opt*_tmp62C=({struct Cyc_Core_Opt*_tmp62E=_cycalloc(sizeof(*_tmp62E));({
# 1919
struct Cyc_PP_Doc*_tmp9F3=({struct Cyc_PP_Doc*_tmp62D[2U];({struct Cyc_PP_Doc*_tmp9F2=({Cyc_Absynpp_typedef_name2bolddoc(td->name);});_tmp62D[0]=_tmp9F2;}),({
struct Cyc_PP_Doc*_tmp9F1=({Cyc_Absynpp_tvars2doc(td->tvs);});_tmp62D[1]=_tmp9F1;});Cyc_PP_cat(_tag_fat(_tmp62D,sizeof(struct Cyc_PP_Doc*),2U));});
# 1919
_tmp62E->v=_tmp9F3;});_tmp62E;});_tmp629(_tmp62A,_tmp62B,_tmp62C);});
# 1917
_tmp627[1]=_tmp9F4;}),({
# 1922
struct Cyc_PP_Doc*_tmp9F0=({Cyc_Absynpp_atts2doc(td->atts);});_tmp627[2]=_tmp9F0;}),({
struct Cyc_PP_Doc*_tmp9EF=({Cyc_PP_text(({const char*_tmp62F=";";_tag_fat(_tmp62F,sizeof(char),2U);}));});_tmp627[3]=_tmp9EF;});Cyc_PP_cat(_tag_fat(_tmp627,sizeof(struct Cyc_PP_Doc*),4U));});
# 1925
goto _LL0;}case 9U: _LL13: _tmp5E9=((struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct*)_tmp5E1)->f1;_tmp5EA=((struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct*)_tmp5E1)->f2;_LL14: {struct _fat_ptr*v=_tmp5E9;struct Cyc_List_List*tdl=_tmp5EA;
# 1927
if(Cyc_Absynpp_use_curr_namespace)({Cyc_Absynpp_curr_namespace_add(v);});s=({struct Cyc_PP_Doc*_tmp630[8U];({
struct Cyc_PP_Doc*_tmp9FF=({Cyc_PP_text(({const char*_tmp631="namespace ";_tag_fat(_tmp631,sizeof(char),11U);}));});_tmp630[0]=_tmp9FF;}),({
struct Cyc_PP_Doc*_tmp9FE=({Cyc_PP_textptr(v);});_tmp630[1]=_tmp9FE;}),({
struct Cyc_PP_Doc*_tmp9FD=({Cyc_PP_blank_doc();});_tmp630[2]=_tmp9FD;}),({struct Cyc_PP_Doc*_tmp9FC=({Cyc_Absynpp_lb();});_tmp630[3]=_tmp9FC;}),({
struct Cyc_PP_Doc*_tmp9FB=({Cyc_PP_line_doc();});_tmp630[4]=_tmp9FB;}),({
struct Cyc_PP_Doc*_tmp9FA=({({struct Cyc_PP_Doc*(*_tmp9F9)(struct Cyc_PP_Doc*(*pp)(struct Cyc_Absyn_Decl*),struct _fat_ptr sep,struct Cyc_List_List*l)=({struct Cyc_PP_Doc*(*_tmp632)(struct Cyc_PP_Doc*(*pp)(struct Cyc_Absyn_Decl*),struct _fat_ptr sep,struct Cyc_List_List*l)=(struct Cyc_PP_Doc*(*)(struct Cyc_PP_Doc*(*pp)(struct Cyc_Absyn_Decl*),struct _fat_ptr sep,struct Cyc_List_List*l))Cyc_PP_ppseql;_tmp632;});struct _fat_ptr _tmp9F8=({const char*_tmp633="";_tag_fat(_tmp633,sizeof(char),1U);});_tmp9F9(Cyc_Absynpp_decl2doc,_tmp9F8,tdl);});});_tmp630[5]=_tmp9FA;}),({
struct Cyc_PP_Doc*_tmp9F7=({Cyc_PP_line_doc();});_tmp630[6]=_tmp9F7;}),({
struct Cyc_PP_Doc*_tmp9F6=({Cyc_Absynpp_rb();});_tmp630[7]=_tmp9F6;});Cyc_PP_cat(_tag_fat(_tmp630,sizeof(struct Cyc_PP_Doc*),8U));});
if(Cyc_Absynpp_use_curr_namespace)({Cyc_Absynpp_curr_namespace_drop();});goto _LL0;}case 10U: _LL15: _tmp5E7=((struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct*)_tmp5E1)->f1;_tmp5E8=((struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct*)_tmp5E1)->f2;_LL16: {struct _tuple0*q=_tmp5E7;struct Cyc_List_List*tdl=_tmp5E8;
# 1938
if(Cyc_Absynpp_print_using_stmts)
s=({struct Cyc_PP_Doc*_tmp634[8U];({struct Cyc_PP_Doc*_tmpA09=({Cyc_PP_text(({const char*_tmp635="using ";_tag_fat(_tmp635,sizeof(char),7U);}));});_tmp634[0]=_tmpA09;}),({
struct Cyc_PP_Doc*_tmpA08=({Cyc_Absynpp_qvar2doc(q);});_tmp634[1]=_tmpA08;}),({
struct Cyc_PP_Doc*_tmpA07=({Cyc_PP_blank_doc();});_tmp634[2]=_tmpA07;}),({struct Cyc_PP_Doc*_tmpA06=({Cyc_Absynpp_lb();});_tmp634[3]=_tmpA06;}),({
struct Cyc_PP_Doc*_tmpA05=({Cyc_PP_line_doc();});_tmp634[4]=_tmpA05;}),({
struct Cyc_PP_Doc*_tmpA04=({({struct Cyc_PP_Doc*(*_tmpA03)(struct Cyc_PP_Doc*(*pp)(struct Cyc_Absyn_Decl*),struct _fat_ptr sep,struct Cyc_List_List*l)=({struct Cyc_PP_Doc*(*_tmp636)(struct Cyc_PP_Doc*(*pp)(struct Cyc_Absyn_Decl*),struct _fat_ptr sep,struct Cyc_List_List*l)=(struct Cyc_PP_Doc*(*)(struct Cyc_PP_Doc*(*pp)(struct Cyc_Absyn_Decl*),struct _fat_ptr sep,struct Cyc_List_List*l))Cyc_PP_ppseql;_tmp636;});struct _fat_ptr _tmpA02=({const char*_tmp637="";_tag_fat(_tmp637,sizeof(char),1U);});_tmpA03(Cyc_Absynpp_decl2doc,_tmpA02,tdl);});});_tmp634[5]=_tmpA04;}),({
struct Cyc_PP_Doc*_tmpA01=({Cyc_PP_line_doc();});_tmp634[6]=_tmpA01;}),({
struct Cyc_PP_Doc*_tmpA00=({Cyc_Absynpp_rb();});_tmp634[7]=_tmpA00;});Cyc_PP_cat(_tag_fat(_tmp634,sizeof(struct Cyc_PP_Doc*),8U));});else{
# 1947
s=({struct Cyc_PP_Doc*_tmp638[11U];({struct Cyc_PP_Doc*_tmpA16=({Cyc_PP_text(({const char*_tmp639="/* using ";_tag_fat(_tmp639,sizeof(char),10U);}));});_tmp638[0]=_tmpA16;}),({
struct Cyc_PP_Doc*_tmpA15=({Cyc_Absynpp_qvar2doc(q);});_tmp638[1]=_tmpA15;}),({
struct Cyc_PP_Doc*_tmpA14=({Cyc_PP_blank_doc();});_tmp638[2]=_tmpA14;}),({
struct Cyc_PP_Doc*_tmpA13=({Cyc_Absynpp_lb();});_tmp638[3]=_tmpA13;}),({
struct Cyc_PP_Doc*_tmpA12=({Cyc_PP_text(({const char*_tmp63A=" */";_tag_fat(_tmp63A,sizeof(char),4U);}));});_tmp638[4]=_tmpA12;}),({
struct Cyc_PP_Doc*_tmpA11=({Cyc_PP_line_doc();});_tmp638[5]=_tmpA11;}),({
struct Cyc_PP_Doc*_tmpA10=({({struct Cyc_PP_Doc*(*_tmpA0F)(struct Cyc_PP_Doc*(*pp)(struct Cyc_Absyn_Decl*),struct _fat_ptr sep,struct Cyc_List_List*l)=({struct Cyc_PP_Doc*(*_tmp63B)(struct Cyc_PP_Doc*(*pp)(struct Cyc_Absyn_Decl*),struct _fat_ptr sep,struct Cyc_List_List*l)=(struct Cyc_PP_Doc*(*)(struct Cyc_PP_Doc*(*pp)(struct Cyc_Absyn_Decl*),struct _fat_ptr sep,struct Cyc_List_List*l))Cyc_PP_ppseql;_tmp63B;});struct _fat_ptr _tmpA0E=({const char*_tmp63C="";_tag_fat(_tmp63C,sizeof(char),1U);});_tmpA0F(Cyc_Absynpp_decl2doc,_tmpA0E,tdl);});});_tmp638[6]=_tmpA10;}),({
struct Cyc_PP_Doc*_tmpA0D=({Cyc_PP_line_doc();});_tmp638[7]=_tmpA0D;}),({
struct Cyc_PP_Doc*_tmpA0C=({Cyc_PP_text(({const char*_tmp63D="/* ";_tag_fat(_tmp63D,sizeof(char),4U);}));});_tmp638[8]=_tmpA0C;}),({
struct Cyc_PP_Doc*_tmpA0B=({Cyc_Absynpp_rb();});_tmp638[9]=_tmpA0B;}),({
struct Cyc_PP_Doc*_tmpA0A=({Cyc_PP_text(({const char*_tmp63E=" */";_tag_fat(_tmp63E,sizeof(char),4U);}));});_tmp638[10]=_tmpA0A;});Cyc_PP_cat(_tag_fat(_tmp638,sizeof(struct Cyc_PP_Doc*),11U));});}
goto _LL0;}case 11U: _LL17: _tmp5E6=((struct Cyc_Absyn_ExternC_d_Absyn_Raw_decl_struct*)_tmp5E1)->f1;_LL18: {struct Cyc_List_List*tdl=_tmp5E6;
# 1960
if(Cyc_Absynpp_print_externC_stmts)
s=({struct Cyc_PP_Doc*_tmp63F[6U];({struct Cyc_PP_Doc*_tmpA1E=({Cyc_PP_text(({const char*_tmp640="extern \"C\" ";_tag_fat(_tmp640,sizeof(char),12U);}));});_tmp63F[0]=_tmpA1E;}),({
struct Cyc_PP_Doc*_tmpA1D=({Cyc_Absynpp_lb();});_tmp63F[1]=_tmpA1D;}),({
struct Cyc_PP_Doc*_tmpA1C=({Cyc_PP_line_doc();});_tmp63F[2]=_tmpA1C;}),({
struct Cyc_PP_Doc*_tmpA1B=({({struct Cyc_PP_Doc*(*_tmpA1A)(struct Cyc_PP_Doc*(*pp)(struct Cyc_Absyn_Decl*),struct _fat_ptr sep,struct Cyc_List_List*l)=({struct Cyc_PP_Doc*(*_tmp641)(struct Cyc_PP_Doc*(*pp)(struct Cyc_Absyn_Decl*),struct _fat_ptr sep,struct Cyc_List_List*l)=(struct Cyc_PP_Doc*(*)(struct Cyc_PP_Doc*(*pp)(struct Cyc_Absyn_Decl*),struct _fat_ptr sep,struct Cyc_List_List*l))Cyc_PP_ppseql;_tmp641;});struct _fat_ptr _tmpA19=({const char*_tmp642="";_tag_fat(_tmp642,sizeof(char),1U);});_tmpA1A(Cyc_Absynpp_decl2doc,_tmpA19,tdl);});});_tmp63F[3]=_tmpA1B;}),({
struct Cyc_PP_Doc*_tmpA18=({Cyc_PP_line_doc();});_tmp63F[4]=_tmpA18;}),({
struct Cyc_PP_Doc*_tmpA17=({Cyc_Absynpp_rb();});_tmp63F[5]=_tmpA17;});Cyc_PP_cat(_tag_fat(_tmp63F,sizeof(struct Cyc_PP_Doc*),6U));});else{
# 1968
s=({struct Cyc_PP_Doc*_tmp643[9U];({struct Cyc_PP_Doc*_tmpA29=({Cyc_PP_text(({const char*_tmp644="/* extern \"C\" ";_tag_fat(_tmp644,sizeof(char),15U);}));});_tmp643[0]=_tmpA29;}),({
struct Cyc_PP_Doc*_tmpA28=({Cyc_Absynpp_lb();});_tmp643[1]=_tmpA28;}),({
struct Cyc_PP_Doc*_tmpA27=({Cyc_PP_text(({const char*_tmp645=" */";_tag_fat(_tmp645,sizeof(char),4U);}));});_tmp643[2]=_tmpA27;}),({
struct Cyc_PP_Doc*_tmpA26=({Cyc_PP_line_doc();});_tmp643[3]=_tmpA26;}),({
struct Cyc_PP_Doc*_tmpA25=({({struct Cyc_PP_Doc*(*_tmpA24)(struct Cyc_PP_Doc*(*pp)(struct Cyc_Absyn_Decl*),struct _fat_ptr sep,struct Cyc_List_List*l)=({struct Cyc_PP_Doc*(*_tmp646)(struct Cyc_PP_Doc*(*pp)(struct Cyc_Absyn_Decl*),struct _fat_ptr sep,struct Cyc_List_List*l)=(struct Cyc_PP_Doc*(*)(struct Cyc_PP_Doc*(*pp)(struct Cyc_Absyn_Decl*),struct _fat_ptr sep,struct Cyc_List_List*l))Cyc_PP_ppseql;_tmp646;});struct _fat_ptr _tmpA23=({const char*_tmp647="";_tag_fat(_tmp647,sizeof(char),1U);});_tmpA24(Cyc_Absynpp_decl2doc,_tmpA23,tdl);});});_tmp643[4]=_tmpA25;}),({
struct Cyc_PP_Doc*_tmpA22=({Cyc_PP_line_doc();});_tmp643[5]=_tmpA22;}),({
struct Cyc_PP_Doc*_tmpA21=({Cyc_PP_text(({const char*_tmp648="/* ";_tag_fat(_tmp648,sizeof(char),4U);}));});_tmp643[6]=_tmpA21;}),({
struct Cyc_PP_Doc*_tmpA20=({Cyc_Absynpp_rb();});_tmp643[7]=_tmpA20;}),({
struct Cyc_PP_Doc*_tmpA1F=({Cyc_PP_text(({const char*_tmp649=" */";_tag_fat(_tmp649,sizeof(char),4U);}));});_tmp643[8]=_tmpA1F;});Cyc_PP_cat(_tag_fat(_tmp643,sizeof(struct Cyc_PP_Doc*),9U));});}
goto _LL0;}case 12U: _LL19: _tmp5E2=((struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct*)_tmp5E1)->f1;_tmp5E3=((struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct*)_tmp5E1)->f2;_tmp5E4=((struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct*)_tmp5E1)->f3;_tmp5E5=((struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct*)_tmp5E1)->f4;_LL1A: {struct Cyc_List_List*tdl=_tmp5E2;struct Cyc_List_List*ovrs=_tmp5E3;struct Cyc_List_List*exs=_tmp5E4;struct _tuple11*wc=_tmp5E5;
# 1979
if(Cyc_Absynpp_print_externC_stmts){
struct Cyc_PP_Doc*exs_doc;
struct Cyc_PP_Doc*ovrs_doc;
if(exs != 0)
exs_doc=({struct Cyc_PP_Doc*_tmp64A[7U];({struct Cyc_PP_Doc*_tmpA32=({Cyc_Absynpp_rb();});_tmp64A[0]=_tmpA32;}),({struct Cyc_PP_Doc*_tmpA31=({Cyc_PP_text(({const char*_tmp64B=" export ";_tag_fat(_tmp64B,sizeof(char),9U);}));});_tmp64A[1]=_tmpA31;}),({struct Cyc_PP_Doc*_tmpA30=({Cyc_Absynpp_lb();});_tmp64A[2]=_tmpA30;}),({
struct Cyc_PP_Doc*_tmpA2F=({Cyc_PP_line_doc();});_tmp64A[3]=_tmpA2F;}),({struct Cyc_PP_Doc*_tmpA2E=({({struct Cyc_PP_Doc*(*_tmpA2D)(struct Cyc_PP_Doc*(*pp)(struct _tuple21*),struct _fat_ptr sep,struct Cyc_List_List*l)=({struct Cyc_PP_Doc*(*_tmp64C)(struct Cyc_PP_Doc*(*pp)(struct _tuple21*),struct _fat_ptr sep,struct Cyc_List_List*l)=(struct Cyc_PP_Doc*(*)(struct Cyc_PP_Doc*(*pp)(struct _tuple21*),struct _fat_ptr sep,struct Cyc_List_List*l))Cyc_PP_ppseql;_tmp64C;});struct _fat_ptr _tmpA2C=({const char*_tmp64D=",";_tag_fat(_tmp64D,sizeof(char),2U);});_tmpA2D(Cyc_Absynpp_export2doc,_tmpA2C,exs);});});_tmp64A[4]=_tmpA2E;}),({
struct Cyc_PP_Doc*_tmpA2B=({Cyc_PP_line_doc();});_tmp64A[5]=_tmpA2B;}),({struct Cyc_PP_Doc*_tmpA2A=({Cyc_Absynpp_rb();});_tmp64A[6]=_tmpA2A;});Cyc_PP_cat(_tag_fat(_tmp64A,sizeof(struct Cyc_PP_Doc*),7U));});else{
# 1987
exs_doc=({Cyc_Absynpp_rb();});}
if(ovrs != 0)
ovrs_doc=({struct Cyc_PP_Doc*_tmp64E[7U];({struct Cyc_PP_Doc*_tmpA3B=({Cyc_Absynpp_rb();});_tmp64E[0]=_tmpA3B;}),({struct Cyc_PP_Doc*_tmpA3A=({Cyc_PP_text(({const char*_tmp64F=" cycdef ";_tag_fat(_tmp64F,sizeof(char),9U);}));});_tmp64E[1]=_tmpA3A;}),({struct Cyc_PP_Doc*_tmpA39=({Cyc_Absynpp_lb();});_tmp64E[2]=_tmpA39;}),({
struct Cyc_PP_Doc*_tmpA38=({Cyc_PP_line_doc();});_tmp64E[3]=_tmpA38;}),({struct Cyc_PP_Doc*_tmpA37=({({struct Cyc_PP_Doc*(*_tmpA36)(struct Cyc_PP_Doc*(*pp)(struct Cyc_Absyn_Decl*),struct _fat_ptr sep,struct Cyc_List_List*l)=({struct Cyc_PP_Doc*(*_tmp650)(struct Cyc_PP_Doc*(*pp)(struct Cyc_Absyn_Decl*),struct _fat_ptr sep,struct Cyc_List_List*l)=(struct Cyc_PP_Doc*(*)(struct Cyc_PP_Doc*(*pp)(struct Cyc_Absyn_Decl*),struct _fat_ptr sep,struct Cyc_List_List*l))Cyc_PP_ppseql;_tmp650;});struct _fat_ptr _tmpA35=({const char*_tmp651="";_tag_fat(_tmp651,sizeof(char),1U);});_tmpA36(Cyc_Absynpp_decl2doc,_tmpA35,ovrs);});});_tmp64E[4]=_tmpA37;}),({
struct Cyc_PP_Doc*_tmpA34=({Cyc_PP_line_doc();});_tmp64E[5]=_tmpA34;}),({struct Cyc_PP_Doc*_tmpA33=({Cyc_Absynpp_rb();});_tmp64E[6]=_tmpA33;});Cyc_PP_cat(_tag_fat(_tmp64E,sizeof(struct Cyc_PP_Doc*),7U));});else{
# 1993
ovrs_doc=({Cyc_Absynpp_rb();});}
s=({struct Cyc_PP_Doc*_tmp652[6U];({struct Cyc_PP_Doc*_tmpA42=({Cyc_PP_text(({const char*_tmp653="extern \"C include\" ";_tag_fat(_tmp653,sizeof(char),20U);}));});_tmp652[0]=_tmpA42;}),({
struct Cyc_PP_Doc*_tmpA41=({Cyc_Absynpp_lb();});_tmp652[1]=_tmpA41;}),({
struct Cyc_PP_Doc*_tmpA40=({Cyc_PP_line_doc();});_tmp652[2]=_tmpA40;}),({
struct Cyc_PP_Doc*_tmpA3F=({({struct Cyc_PP_Doc*(*_tmpA3E)(struct Cyc_PP_Doc*(*pp)(struct Cyc_Absyn_Decl*),struct _fat_ptr sep,struct Cyc_List_List*l)=({struct Cyc_PP_Doc*(*_tmp654)(struct Cyc_PP_Doc*(*pp)(struct Cyc_Absyn_Decl*),struct _fat_ptr sep,struct Cyc_List_List*l)=(struct Cyc_PP_Doc*(*)(struct Cyc_PP_Doc*(*pp)(struct Cyc_Absyn_Decl*),struct _fat_ptr sep,struct Cyc_List_List*l))Cyc_PP_ppseql;_tmp654;});struct _fat_ptr _tmpA3D=({const char*_tmp655="";_tag_fat(_tmp655,sizeof(char),1U);});_tmpA3E(Cyc_Absynpp_decl2doc,_tmpA3D,tdl);});});_tmp652[3]=_tmpA3F;}),({
struct Cyc_PP_Doc*_tmpA3C=({Cyc_PP_line_doc();});_tmp652[4]=_tmpA3C;}),_tmp652[5]=exs_doc;Cyc_PP_cat(_tag_fat(_tmp652,sizeof(struct Cyc_PP_Doc*),6U));});}else{
# 2001
s=({struct Cyc_PP_Doc*_tmp656[9U];({struct Cyc_PP_Doc*_tmpA4D=({Cyc_PP_text(({const char*_tmp657="/* extern \"C include\" ";_tag_fat(_tmp657,sizeof(char),23U);}));});_tmp656[0]=_tmpA4D;}),({
struct Cyc_PP_Doc*_tmpA4C=({Cyc_Absynpp_lb();});_tmp656[1]=_tmpA4C;}),({
struct Cyc_PP_Doc*_tmpA4B=({Cyc_PP_text(({const char*_tmp658=" */";_tag_fat(_tmp658,sizeof(char),4U);}));});_tmp656[2]=_tmpA4B;}),({
struct Cyc_PP_Doc*_tmpA4A=({Cyc_PP_line_doc();});_tmp656[3]=_tmpA4A;}),({
struct Cyc_PP_Doc*_tmpA49=({({struct Cyc_PP_Doc*(*_tmpA48)(struct Cyc_PP_Doc*(*pp)(struct Cyc_Absyn_Decl*),struct _fat_ptr sep,struct Cyc_List_List*l)=({struct Cyc_PP_Doc*(*_tmp659)(struct Cyc_PP_Doc*(*pp)(struct Cyc_Absyn_Decl*),struct _fat_ptr sep,struct Cyc_List_List*l)=(struct Cyc_PP_Doc*(*)(struct Cyc_PP_Doc*(*pp)(struct Cyc_Absyn_Decl*),struct _fat_ptr sep,struct Cyc_List_List*l))Cyc_PP_ppseql;_tmp659;});struct _fat_ptr _tmpA47=({const char*_tmp65A="";_tag_fat(_tmp65A,sizeof(char),1U);});_tmpA48(Cyc_Absynpp_decl2doc,_tmpA47,tdl);});});_tmp656[4]=_tmpA49;}),({
struct Cyc_PP_Doc*_tmpA46=({Cyc_PP_line_doc();});_tmp656[5]=_tmpA46;}),({
struct Cyc_PP_Doc*_tmpA45=({Cyc_PP_text(({const char*_tmp65B="/* ";_tag_fat(_tmp65B,sizeof(char),4U);}));});_tmp656[6]=_tmpA45;}),({
struct Cyc_PP_Doc*_tmpA44=({Cyc_Absynpp_rb();});_tmp656[7]=_tmpA44;}),({
struct Cyc_PP_Doc*_tmpA43=({Cyc_PP_text(({const char*_tmp65C=" */";_tag_fat(_tmp65C,sizeof(char),4U);}));});_tmp656[8]=_tmpA43;});Cyc_PP_cat(_tag_fat(_tmp656,sizeof(struct Cyc_PP_Doc*),9U));});}
goto _LL0;}case 13U: _LL1B: _LL1C:
# 2012
 s=({struct Cyc_PP_Doc*_tmp65D[2U];({struct Cyc_PP_Doc*_tmpA4F=({Cyc_PP_text(({const char*_tmp65E="__cyclone_port_on__;";_tag_fat(_tmp65E,sizeof(char),21U);}));});_tmp65D[0]=_tmpA4F;}),({struct Cyc_PP_Doc*_tmpA4E=({Cyc_Absynpp_lb();});_tmp65D[1]=_tmpA4E;});Cyc_PP_cat(_tag_fat(_tmp65D,sizeof(struct Cyc_PP_Doc*),2U));});
goto _LL0;case 14U: _LL1D: _LL1E:
# 2015
 s=({struct Cyc_PP_Doc*_tmp65F[2U];({struct Cyc_PP_Doc*_tmpA51=({Cyc_PP_text(({const char*_tmp660="__cyclone_port_off__;";_tag_fat(_tmp660,sizeof(char),22U);}));});_tmp65F[0]=_tmpA51;}),({struct Cyc_PP_Doc*_tmpA50=({Cyc_Absynpp_lb();});_tmp65F[1]=_tmpA50;});Cyc_PP_cat(_tag_fat(_tmp65F,sizeof(struct Cyc_PP_Doc*),2U));});
goto _LL0;case 15U: _LL1F: _LL20:
# 2018
 s=({struct Cyc_PP_Doc*_tmp661[2U];({struct Cyc_PP_Doc*_tmpA53=({Cyc_PP_text(({const char*_tmp662="__tempest_on__;";_tag_fat(_tmp662,sizeof(char),16U);}));});_tmp661[0]=_tmpA53;}),({struct Cyc_PP_Doc*_tmpA52=({Cyc_Absynpp_lb();});_tmp661[1]=_tmpA52;});Cyc_PP_cat(_tag_fat(_tmp661,sizeof(struct Cyc_PP_Doc*),2U));});
goto _LL0;default: _LL21: _LL22:
# 2021
 s=({struct Cyc_PP_Doc*_tmp663[2U];({struct Cyc_PP_Doc*_tmpA55=({Cyc_PP_text(({const char*_tmp664="__tempest_off__;";_tag_fat(_tmp664,sizeof(char),17U);}));});_tmp663[0]=_tmpA55;}),({struct Cyc_PP_Doc*_tmpA54=({Cyc_Absynpp_lb();});_tmp663[1]=_tmpA54;});Cyc_PP_cat(_tag_fat(_tmp663,sizeof(struct Cyc_PP_Doc*),2U));});
goto _LL0;}_LL0:;}
# 2025
return s;}
# 1720
struct Cyc_PP_Doc*Cyc_Absynpp_scope2doc(enum Cyc_Absyn_Scope sc){
# 2029
if(Cyc_Absynpp_print_for_cycdoc)return({Cyc_PP_nil_doc();});{enum Cyc_Absyn_Scope _tmp666=sc;switch(_tmp666){case Cyc_Absyn_Static: _LL1: _LL2:
# 2031
 return({Cyc_PP_text(({const char*_tmp667="static ";_tag_fat(_tmp667,sizeof(char),8U);}));});case Cyc_Absyn_Public: _LL3: _LL4:
 return({Cyc_PP_nil_doc();});case Cyc_Absyn_Extern: _LL5: _LL6:
 return({Cyc_PP_text(({const char*_tmp668="extern ";_tag_fat(_tmp668,sizeof(char),8U);}));});case Cyc_Absyn_ExternC: _LL7: _LL8:
 return({Cyc_PP_text(({const char*_tmp669="extern \"C\" ";_tag_fat(_tmp669,sizeof(char),12U);}));});case Cyc_Absyn_Abstract: _LL9: _LLA:
 return({Cyc_PP_text(({const char*_tmp66A="abstract ";_tag_fat(_tmp66A,sizeof(char),10U);}));});case Cyc_Absyn_Register: _LLB: _LLC:
 return({Cyc_PP_text(({const char*_tmp66B="register ";_tag_fat(_tmp66B,sizeof(char),10U);}));});default: _LLD: _LLE:
 return({Cyc_PP_nil_doc();});}_LL0:;}}
# 1720
int Cyc_Absynpp_exists_temp_tvar_in_effect(void*t){
# 2043
void*_stmttmp22=({Cyc_Tcutil_compress(t);});void*_tmp66D=_stmttmp22;struct Cyc_List_List*_tmp66E;struct Cyc_Absyn_Tvar*_tmp66F;switch(*((int*)_tmp66D)){case 2U: _LL1: _tmp66F=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp66D)->f1;_LL2: {struct Cyc_Absyn_Tvar*tv=_tmp66F;
return({Cyc_Tcutil_is_temp_tvar(tv);});}case 0U: if(((struct Cyc_Absyn_JoinCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp66D)->f1)->tag == 11U){_LL3: _tmp66E=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp66D)->f2;_LL4: {struct Cyc_List_List*l=_tmp66E;
return({Cyc_List_exists(Cyc_Absynpp_exists_temp_tvar_in_effect,l);});}}else{goto _LL5;}default: _LL5: _LL6:
 return 0;}_LL0:;}
# 1720
int Cyc_Absynpp_is_anon_aggrtype(void*t){
# 2055
void*_tmp671=t;void*_tmp673;struct Cyc_Absyn_Typedefdecl*_tmp672;switch(*((int*)_tmp671)){case 7U: _LL1: _LL2:
 return 1;case 0U: if(((struct Cyc_Absyn_AnonEnumCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp671)->f1)->tag == 18U){_LL3: _LL4:
 return 1;}else{goto _LL7;}case 8U: _LL5: _tmp672=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp671)->f3;_tmp673=(void*)((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp671)->f4;if(_tmp673 != 0){_LL6: {struct Cyc_Absyn_Typedefdecl*td=_tmp672;void*x=_tmp673;
# 2061
return({Cyc_Absynpp_is_anon_aggrtype(x);});}}else{goto _LL7;}default: _LL7: _LL8:
 return 0;}_LL0:;}
# 2069
static struct Cyc_List_List*Cyc_Absynpp_bubble_attributes(struct _RegionHandle*r,void*atts,struct Cyc_List_List*tms){
# 2071
if(tms != 0 && tms->tl != 0){
struct _tuple19 _stmttmp23=({struct _tuple19 _tmp74B;_tmp74B.f1=(void*)tms->hd,_tmp74B.f2=(void*)((struct Cyc_List_List*)_check_null(tms->tl))->hd;_tmp74B;});struct _tuple19 _tmp675=_stmttmp23;if(((struct Cyc_Absyn_Pointer_mod_Absyn_Type_modifier_struct*)_tmp675.f1)->tag == 2U){if(((struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct*)_tmp675.f2)->tag == 3U){_LL1: _LL2:
# 2074
 return({struct Cyc_List_List*_tmp677=_region_malloc(r,sizeof(*_tmp677));_tmp677->hd=(void*)tms->hd,({struct Cyc_List_List*_tmpA57=({struct Cyc_List_List*_tmp676=_region_malloc(r,sizeof(*_tmp676));_tmp676->hd=(void*)((struct Cyc_List_List*)_check_null(tms->tl))->hd,({struct Cyc_List_List*_tmpA56=({Cyc_Absynpp_bubble_attributes(r,atts,((struct Cyc_List_List*)_check_null(tms->tl))->tl);});_tmp676->tl=_tmpA56;});_tmp676;});_tmp677->tl=_tmpA57;});_tmp677;});}else{goto _LL3;}}else{_LL3: _LL4:
 return({struct Cyc_List_List*_tmp678=_region_malloc(r,sizeof(*_tmp678));_tmp678->hd=atts,_tmp678->tl=tms;_tmp678;});}_LL0:;}else{
# 2077
return({struct Cyc_List_List*_tmp679=_region_malloc(r,sizeof(*_tmp679));_tmp679->hd=atts,_tmp679->tl=tms;_tmp679;});}}
# 2080
static void Cyc_Absynpp_rewrite_temp_tvar(struct Cyc_Absyn_Tvar*t){
if(!({Cyc_Tcutil_is_temp_tvar(t);}))return;{struct _fat_ptr s=({({struct _fat_ptr _tmpA58=({const char*_tmp680="`";_tag_fat(_tmp680,sizeof(char),2U);});Cyc_strconcat(_tmpA58,(struct _fat_ptr)*t->name);});});
# 2083
({struct _fat_ptr _tmp67B=_fat_ptr_plus(s,sizeof(char),1);char _tmp67C=*((char*)_check_fat_subscript(_tmp67B,sizeof(char),0U));char _tmp67D='t';if(_get_fat_size(_tmp67B,sizeof(char))== 1U &&(_tmp67C == 0 && _tmp67D != 0))_throw_arraybounds();*((char*)_tmp67B.curr)=_tmp67D;});
({struct _fat_ptr*_tmpA59=({unsigned _tmp67F=1;struct _fat_ptr*_tmp67E=_cycalloc(_check_times(_tmp67F,sizeof(struct _fat_ptr)));_tmp67E[0]=(struct _fat_ptr)s;_tmp67E;});t->name=_tmpA59;});}}
# 2089
struct _tuple14 Cyc_Absynpp_to_tms(struct _RegionHandle*r,struct Cyc_Absyn_Tqual tq,void*t){
# 2091
void*_tmp682=t;void*_tmp686;struct Cyc_Absyn_Typedefdecl*_tmp685;struct Cyc_List_List*_tmp684;struct _tuple0*_tmp683;int _tmp689;void*_tmp688;struct Cyc_Core_Opt*_tmp687;int _tmp698;struct Cyc_List_List*_tmp697;struct Cyc_List_List*_tmp696;struct Cyc_List_List*_tmp695;struct Cyc_Absyn_Exp*_tmp694;struct Cyc_Absyn_Exp*_tmp693;struct Cyc_List_List*_tmp692;struct Cyc_List_List*_tmp691;struct Cyc_Absyn_VarargInfo*_tmp690;int _tmp68F;struct Cyc_List_List*_tmp68E;void*_tmp68D;struct Cyc_Absyn_Tqual _tmp68C;void*_tmp68B;struct Cyc_List_List*_tmp68A;struct Cyc_Absyn_PtrAtts _tmp69B;struct Cyc_Absyn_Tqual _tmp69A;void*_tmp699;unsigned _tmp6A0;void*_tmp69F;struct Cyc_Absyn_Exp*_tmp69E;struct Cyc_Absyn_Tqual _tmp69D;void*_tmp69C;switch(*((int*)_tmp682)){case 4U: _LL1: _tmp69C=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp682)->f1).elt_type;_tmp69D=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp682)->f1).tq;_tmp69E=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp682)->f1).num_elts;_tmp69F=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp682)->f1).zero_term;_tmp6A0=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp682)->f1).zt_loc;_LL2: {void*t2=_tmp69C;struct Cyc_Absyn_Tqual tq2=_tmp69D;struct Cyc_Absyn_Exp*e=_tmp69E;void*zeroterm=_tmp69F;unsigned ztl=_tmp6A0;
# 2094
struct _tuple14 _stmttmp24=({Cyc_Absynpp_to_tms(r,tq2,t2);});struct Cyc_List_List*_tmp6A3;void*_tmp6A2;struct Cyc_Absyn_Tqual _tmp6A1;_LLE: _tmp6A1=_stmttmp24.f1;_tmp6A2=_stmttmp24.f2;_tmp6A3=_stmttmp24.f3;_LLF: {struct Cyc_Absyn_Tqual tq3=_tmp6A1;void*t3=_tmp6A2;struct Cyc_List_List*tml3=_tmp6A3;
void*tm;
if(e == 0)
tm=(void*)({struct Cyc_Absyn_Carray_mod_Absyn_Type_modifier_struct*_tmp6A4=_region_malloc(r,sizeof(*_tmp6A4));_tmp6A4->tag=0U,_tmp6A4->f1=zeroterm,_tmp6A4->f2=ztl;_tmp6A4;});else{
# 2099
tm=(void*)({struct Cyc_Absyn_ConstArray_mod_Absyn_Type_modifier_struct*_tmp6A5=_region_malloc(r,sizeof(*_tmp6A5));_tmp6A5->tag=1U,_tmp6A5->f1=e,_tmp6A5->f2=zeroterm,_tmp6A5->f3=ztl;_tmp6A5;});}
return({struct _tuple14 _tmp74C;_tmp74C.f1=tq3,_tmp74C.f2=t3,({struct Cyc_List_List*_tmpA5A=({struct Cyc_List_List*_tmp6A6=_region_malloc(r,sizeof(*_tmp6A6));_tmp6A6->hd=tm,_tmp6A6->tl=tml3;_tmp6A6;});_tmp74C.f3=_tmpA5A;});_tmp74C;});}}case 3U: _LL3: _tmp699=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp682)->f1).elt_type;_tmp69A=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp682)->f1).elt_tq;_tmp69B=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp682)->f1).ptr_atts;_LL4: {void*t2=_tmp699;struct Cyc_Absyn_Tqual tq2=_tmp69A;struct Cyc_Absyn_PtrAtts ptratts=_tmp69B;
# 2103
struct _tuple14 _stmttmp25=({Cyc_Absynpp_to_tms(r,tq2,t2);});struct Cyc_List_List*_tmp6A9;void*_tmp6A8;struct Cyc_Absyn_Tqual _tmp6A7;_LL11: _tmp6A7=_stmttmp25.f1;_tmp6A8=_stmttmp25.f2;_tmp6A9=_stmttmp25.f3;_LL12: {struct Cyc_Absyn_Tqual tq3=_tmp6A7;void*t3=_tmp6A8;struct Cyc_List_List*tml3=_tmp6A9;
tml3=({struct Cyc_List_List*_tmp6AB=_region_malloc(r,sizeof(*_tmp6AB));({void*_tmpA5B=(void*)({struct Cyc_Absyn_Pointer_mod_Absyn_Type_modifier_struct*_tmp6AA=_region_malloc(r,sizeof(*_tmp6AA));_tmp6AA->tag=2U,_tmp6AA->f1=ptratts,_tmp6AA->f2=tq;_tmp6AA;});_tmp6AB->hd=_tmpA5B;}),_tmp6AB->tl=tml3;_tmp6AB;});
return({struct _tuple14 _tmp74D;_tmp74D.f1=tq3,_tmp74D.f2=t3,_tmp74D.f3=tml3;_tmp74D;});}}case 5U: _LL5: _tmp68A=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp682)->f1).tvars;_tmp68B=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp682)->f1).effect;_tmp68C=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp682)->f1).ret_tqual;_tmp68D=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp682)->f1).ret_type;_tmp68E=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp682)->f1).args;_tmp68F=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp682)->f1).c_varargs;_tmp690=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp682)->f1).cyc_varargs;_tmp691=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp682)->f1).rgn_po;_tmp692=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp682)->f1).attributes;_tmp693=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp682)->f1).requires_clause;_tmp694=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp682)->f1).ensures_clause;_tmp695=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp682)->f1).ieffect;_tmp696=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp682)->f1).oeffect;_tmp697=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp682)->f1).throws;_tmp698=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp682)->f1).reentrant;_LL6: {struct Cyc_List_List*typvars=_tmp68A;void*effopt=_tmp68B;struct Cyc_Absyn_Tqual t2qual=_tmp68C;void*t2=_tmp68D;struct Cyc_List_List*args=_tmp68E;int c_varargs=_tmp68F;struct Cyc_Absyn_VarargInfo*cyc_varargs=_tmp690;struct Cyc_List_List*rgn_po=_tmp691;struct Cyc_List_List*fn_atts=_tmp692;struct Cyc_Absyn_Exp*req=_tmp693;struct Cyc_Absyn_Exp*ens=_tmp694;struct Cyc_List_List*iff=_tmp695;struct Cyc_List_List*off=_tmp696;struct Cyc_List_List*th=_tmp697;int reentrant=_tmp698;
# 2109
if(!Cyc_Absynpp_print_all_tvars){
# 2113
if(effopt == 0 ||({Cyc_Absynpp_exists_temp_tvar_in_effect(effopt);})){
effopt=0;
typvars=0;}}else{
# 2118
if(Cyc_Absynpp_rewrite_temp_tvars)
# 2121
({({void(*_tmpA5C)(void(*f)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*x)=({void(*_tmp6AC)(void(*f)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*x)=(void(*)(void(*f)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*x))Cyc_List_iter;_tmp6AC;});_tmpA5C(Cyc_Absynpp_rewrite_temp_tvar,typvars);});});}{
# 2126
struct _tuple14 _stmttmp26=({Cyc_Absynpp_to_tms(r,t2qual,t2);});struct Cyc_List_List*_tmp6AF;void*_tmp6AE;struct Cyc_Absyn_Tqual _tmp6AD;_LL14: _tmp6AD=_stmttmp26.f1;_tmp6AE=_stmttmp26.f2;_tmp6AF=_stmttmp26.f3;_LL15: {struct Cyc_Absyn_Tqual tq3=_tmp6AD;void*t3=_tmp6AE;struct Cyc_List_List*tml3=_tmp6AF;
struct Cyc_List_List*tms=tml3;
# 2137 "absynpp.cyc"
{enum Cyc_Cyclone_C_Compilers _tmp6B0=Cyc_Cyclone_c_compiler;if(_tmp6B0 == Cyc_Cyclone_Gcc_c){_LL17: _LL18:
# 2139
 if(fn_atts != 0)
# 2142
tms=({({struct _RegionHandle*_tmpA5E=r;void*_tmpA5D=(void*)({struct Cyc_Absyn_Attributes_mod_Absyn_Type_modifier_struct*_tmp6B1=_region_malloc(r,sizeof(*_tmp6B1));_tmp6B1->tag=5U,_tmp6B1->f1=0U,_tmp6B1->f2=fn_atts;_tmp6B1;});Cyc_Absynpp_bubble_attributes(_tmpA5E,_tmpA5D,tms);});});
# 2139
tms=({struct Cyc_List_List*_tmp6B4=_region_malloc(r,sizeof(*_tmp6B4));
# 2144
({void*_tmpA60=(void*)({struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct*_tmp6B3=_region_malloc(r,sizeof(*_tmp6B3));
_tmp6B3->tag=3U,({void*_tmpA5F=(void*)({struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*_tmp6B2=_region_malloc(r,sizeof(*_tmp6B2));_tmp6B2->tag=1U,_tmp6B2->f1=args,_tmp6B2->f2=c_varargs,_tmp6B2->f3=cyc_varargs,_tmp6B2->f4=effopt,_tmp6B2->f5=rgn_po,_tmp6B2->f6=req,_tmp6B2->f7=ens,_tmp6B2->f8=iff,_tmp6B2->f9=off,_tmp6B2->f10=th,_tmp6B2->f11=reentrant;_tmp6B2;});_tmp6B3->f1=_tmpA5F;});_tmp6B3;});
# 2144
_tmp6B4->hd=_tmpA60;}),_tmp6B4->tl=tms;_tmp6B4;});
# 2148
goto _LL16;}else{_LL19: _LL1A:
# 2150
 tms=({struct Cyc_List_List*_tmp6B7=_region_malloc(r,sizeof(*_tmp6B7));({void*_tmpA62=(void*)({struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct*_tmp6B6=_region_malloc(r,sizeof(*_tmp6B6));
_tmp6B6->tag=3U,({void*_tmpA61=(void*)({struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*_tmp6B5=_region_malloc(r,sizeof(*_tmp6B5));_tmp6B5->tag=1U,_tmp6B5->f1=args,_tmp6B5->f2=c_varargs,_tmp6B5->f3=cyc_varargs,_tmp6B5->f4=effopt,_tmp6B5->f5=rgn_po,_tmp6B5->f6=req,_tmp6B5->f7=ens,_tmp6B5->f8=iff,_tmp6B5->f9=off,_tmp6B5->f10=th,_tmp6B5->f11=reentrant;_tmp6B5;});_tmp6B6->f1=_tmpA61;});_tmp6B6;});
# 2150
_tmp6B7->hd=_tmpA62;}),_tmp6B7->tl=tms;_tmp6B7;});
# 2154
for(0;fn_atts != 0;fn_atts=fn_atts->tl){
void*_stmttmp27=(void*)fn_atts->hd;void*_tmp6B8=_stmttmp27;switch(*((int*)_tmp6B8)){case 1U: _LL1C: _LL1D:
 goto _LL1F;case 2U: _LL1E: _LL1F:
 goto _LL21;case 3U: _LL20: _LL21:
# 2159
 tms=({struct Cyc_List_List*_tmp6BB=_region_malloc(r,sizeof(*_tmp6BB));({void*_tmpA64=(void*)({struct Cyc_Absyn_Attributes_mod_Absyn_Type_modifier_struct*_tmp6BA=_region_malloc(r,sizeof(*_tmp6BA));_tmp6BA->tag=5U,_tmp6BA->f1=0U,({struct Cyc_List_List*_tmpA63=({struct Cyc_List_List*_tmp6B9=_cycalloc(sizeof(*_tmp6B9));_tmp6B9->hd=(void*)fn_atts->hd,_tmp6B9->tl=0;_tmp6B9;});_tmp6BA->f2=_tmpA63;});_tmp6BA;});_tmp6BB->hd=_tmpA64;}),_tmp6BB->tl=tms;_tmp6BB;});
goto AfterAtts;default: _LL22: _LL23:
 goto _LL1B;}_LL1B:;}
# 2163
goto _LL16;}_LL16:;}
# 2166
AfterAtts:
 if(typvars != 0)
tms=({struct Cyc_List_List*_tmp6BD=_region_malloc(r,sizeof(*_tmp6BD));({void*_tmpA65=(void*)({struct Cyc_Absyn_TypeParams_mod_Absyn_Type_modifier_struct*_tmp6BC=_region_malloc(r,sizeof(*_tmp6BC));_tmp6BC->tag=4U,_tmp6BC->f1=typvars,_tmp6BC->f2=0U,_tmp6BC->f3=1;_tmp6BC;});_tmp6BD->hd=_tmpA65;}),_tmp6BD->tl=tms;_tmp6BD;});
# 2166
return({struct _tuple14 _tmp74E;_tmp74E.f1=tq3,_tmp74E.f2=t3,_tmp74E.f3=tms;_tmp74E;});}}}case 1U: _LL7: _tmp687=((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp682)->f1;_tmp688=(void*)((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp682)->f2;_tmp689=((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp682)->f3;_LL8: {struct Cyc_Core_Opt*k=_tmp687;void*topt=_tmp688;int i=_tmp689;
# 2172
if(topt == 0)
return({struct _tuple14 _tmp74F;_tmp74F.f1=tq,_tmp74F.f2=t,_tmp74F.f3=0;_tmp74F;});else{
# 2175
return({Cyc_Absynpp_to_tms(r,tq,topt);});}}case 8U: _LL9: _tmp683=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp682)->f1;_tmp684=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp682)->f2;_tmp685=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp682)->f3;_tmp686=(void*)((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp682)->f4;_LLA: {struct _tuple0*n=_tmp683;struct Cyc_List_List*ts=_tmp684;struct Cyc_Absyn_Typedefdecl*td=_tmp685;void*topt=_tmp686;
# 2181
if(topt == 0 || !Cyc_Absynpp_expand_typedefs)
return({struct _tuple14 _tmp750;_tmp750.f1=tq,_tmp750.f2=t,_tmp750.f3=0;_tmp750;});else{
# 2184
if(tq.real_const)
tq.print_const=tq.real_const;
# 2184
return({Cyc_Absynpp_to_tms(r,tq,topt);});}}default: _LLB: _LLC:
# 2189
 return({struct _tuple14 _tmp751;_tmp751.f1=tq,_tmp751.f2=t,_tmp751.f3=0;_tmp751;});}_LL0:;}
# 2193
static int Cyc_Absynpp_is_char_ptr(void*t){
# 2195
void*_tmp6BF=t;void*_tmp6C0;void*_tmp6C1;switch(*((int*)_tmp6BF)){case 1U: _LL1: _tmp6C1=(void*)((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp6BF)->f2;if(_tmp6C1 != 0){_LL2: {void*def=_tmp6C1;
return({Cyc_Absynpp_is_char_ptr(def);});}}else{goto _LL5;}case 3U: _LL3: _tmp6C0=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp6BF)->f1).elt_type;_LL4: {void*elt_typ=_tmp6C0;
# 2198
L: {
void*_tmp6C2=elt_typ;void*_tmp6C3;void*_tmp6C4;switch(*((int*)_tmp6C2)){case 1U: _LL8: _tmp6C4=(void*)((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp6C2)->f2;if(_tmp6C4 != 0){_LL9: {void*t=_tmp6C4;
elt_typ=t;goto L;}}else{goto _LLE;}case 8U: _LLA: _tmp6C3=(void*)((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp6C2)->f4;if(_tmp6C3 != 0){_LLB: {void*t=_tmp6C3;
elt_typ=t;goto L;}}else{goto _LLE;}case 0U: if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp6C2)->f1)->tag == 1U){if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp6C2)->f1)->f2 == Cyc_Absyn_Char_sz){_LLC: _LLD:
 return 1;}else{goto _LLE;}}else{goto _LLE;}default: _LLE: _LLF:
 return 0;}_LL7:;}}default: _LL5: _LL6:
# 2205
 return 0;}_LL0:;}
# 2193
struct Cyc_PP_Doc*Cyc_Absynpp_tqtd2doc(struct Cyc_Absyn_Tqual tq,void*typ,struct Cyc_Core_Opt*dopt){
# 2210
struct _RegionHandle _tmp6C6=_new_region("temp");struct _RegionHandle*temp=& _tmp6C6;_push_region(temp);
{struct _tuple14 _stmttmp28=({Cyc_Absynpp_to_tms(temp,tq,typ);});struct Cyc_List_List*_tmp6C9;void*_tmp6C8;struct Cyc_Absyn_Tqual _tmp6C7;_LL1: _tmp6C7=_stmttmp28.f1;_tmp6C8=_stmttmp28.f2;_tmp6C9=_stmttmp28.f3;_LL2: {struct Cyc_Absyn_Tqual tq=_tmp6C7;void*t=_tmp6C8;struct Cyc_List_List*tms=_tmp6C9;
tms=({Cyc_List_imp_rev(tms);});
if(tms == 0 && dopt == 0){
struct Cyc_PP_Doc*_tmp6CB=({struct Cyc_PP_Doc*_tmp6CA[2U];({struct Cyc_PP_Doc*_tmpA67=({Cyc_Absynpp_tqual2doc(tq);});_tmp6CA[0]=_tmpA67;}),({struct Cyc_PP_Doc*_tmpA66=({Cyc_Absynpp_ntyp2doc(t);});_tmp6CA[1]=_tmpA66;});Cyc_PP_cat(_tag_fat(_tmp6CA,sizeof(struct Cyc_PP_Doc*),2U));});_npop_handler(0U);return _tmp6CB;}else{
# 2216
struct Cyc_PP_Doc*_tmp6D2=({struct Cyc_PP_Doc*_tmp6CC[4U];({
struct Cyc_PP_Doc*_tmpA6B=({Cyc_Absynpp_tqual2doc(tq);});_tmp6CC[0]=_tmpA6B;}),({
struct Cyc_PP_Doc*_tmpA6A=({Cyc_Absynpp_ntyp2doc(t);});_tmp6CC[1]=_tmpA6A;}),({
struct Cyc_PP_Doc*_tmpA69=({Cyc_PP_text(({const char*_tmp6CD=" ";_tag_fat(_tmp6CD,sizeof(char),2U);}));});_tmp6CC[2]=_tmpA69;}),({
struct Cyc_PP_Doc*_tmpA68=({struct Cyc_PP_Doc*(*_tmp6CE)(int is_char_ptr,struct Cyc_PP_Doc*d,struct Cyc_List_List*tms)=Cyc_Absynpp_dtms2doc;int _tmp6CF=({Cyc_Absynpp_is_char_ptr(typ);});struct Cyc_PP_Doc*_tmp6D0=dopt == 0?({Cyc_PP_nil_doc();}):(struct Cyc_PP_Doc*)dopt->v;struct Cyc_List_List*_tmp6D1=tms;_tmp6CE(_tmp6CF,_tmp6D0,_tmp6D1);});_tmp6CC[3]=_tmpA68;});Cyc_PP_cat(_tag_fat(_tmp6CC,sizeof(struct Cyc_PP_Doc*),4U));});_npop_handler(0U);return _tmp6D2;}}}
# 2211
;_pop_region();}
# 2193
struct Cyc_PP_Doc*Cyc_Absynpp_aggrfield2doc(struct Cyc_Absyn_Aggrfield*f){
# 2225
struct Cyc_PP_Doc*requires_doc;
struct Cyc_Absyn_Exp*req=f->requires_clause;
if((unsigned)req)
requires_doc=({struct Cyc_PP_Doc*_tmp6D4[2U];({struct Cyc_PP_Doc*_tmpA6D=({Cyc_PP_text(({const char*_tmp6D5="@requires ";_tag_fat(_tmp6D5,sizeof(char),11U);}));});_tmp6D4[0]=_tmpA6D;}),({struct Cyc_PP_Doc*_tmpA6C=({Cyc_Absynpp_exp2doc(req);});_tmp6D4[1]=_tmpA6C;});Cyc_PP_cat(_tag_fat(_tmp6D4,sizeof(struct Cyc_PP_Doc*),2U));});else{
# 2230
requires_doc=({Cyc_PP_nil_doc();});}{
# 2232
enum Cyc_Cyclone_C_Compilers _tmp6D6=Cyc_Cyclone_c_compiler;if(_tmp6D6 == Cyc_Cyclone_Gcc_c){_LL1: _LL2:
# 2235
 if(f->width != 0)
return({struct Cyc_PP_Doc*_tmp6D7[5U];({struct Cyc_PP_Doc*_tmpA73=({struct Cyc_PP_Doc*(*_tmp6D8)(struct Cyc_Absyn_Tqual tq,void*typ,struct Cyc_Core_Opt*dopt)=Cyc_Absynpp_tqtd2doc;struct Cyc_Absyn_Tqual _tmp6D9=f->tq;void*_tmp6DA=f->type;struct Cyc_Core_Opt*_tmp6DB=({struct Cyc_Core_Opt*_tmp6DC=_cycalloc(sizeof(*_tmp6DC));({struct Cyc_PP_Doc*_tmpA72=({Cyc_PP_textptr(f->name);});_tmp6DC->v=_tmpA72;});_tmp6DC;});_tmp6D8(_tmp6D9,_tmp6DA,_tmp6DB);});_tmp6D7[0]=_tmpA73;}),({
struct Cyc_PP_Doc*_tmpA71=({Cyc_PP_text(({const char*_tmp6DD=":";_tag_fat(_tmp6DD,sizeof(char),2U);}));});_tmp6D7[1]=_tmpA71;}),({struct Cyc_PP_Doc*_tmpA70=({Cyc_Absynpp_exp2doc((struct Cyc_Absyn_Exp*)_check_null(f->width));});_tmp6D7[2]=_tmpA70;}),({
struct Cyc_PP_Doc*_tmpA6F=({Cyc_Absynpp_atts2doc(f->attributes);});_tmp6D7[3]=_tmpA6F;}),({struct Cyc_PP_Doc*_tmpA6E=({Cyc_PP_text(({const char*_tmp6DE=";";_tag_fat(_tmp6DE,sizeof(char),2U);}));});_tmp6D7[4]=_tmpA6E;});Cyc_PP_cat(_tag_fat(_tmp6D7,sizeof(struct Cyc_PP_Doc*),5U));});else{
# 2240
return({struct Cyc_PP_Doc*_tmp6DF[4U];({struct Cyc_PP_Doc*_tmpA77=({struct Cyc_PP_Doc*(*_tmp6E0)(struct Cyc_Absyn_Tqual tq,void*typ,struct Cyc_Core_Opt*dopt)=Cyc_Absynpp_tqtd2doc;struct Cyc_Absyn_Tqual _tmp6E1=f->tq;void*_tmp6E2=f->type;struct Cyc_Core_Opt*_tmp6E3=({struct Cyc_Core_Opt*_tmp6E4=_cycalloc(sizeof(*_tmp6E4));({struct Cyc_PP_Doc*_tmpA76=({Cyc_PP_textptr(f->name);});_tmp6E4->v=_tmpA76;});_tmp6E4;});_tmp6E0(_tmp6E1,_tmp6E2,_tmp6E3);});_tmp6DF[0]=_tmpA77;}),({
struct Cyc_PP_Doc*_tmpA75=({Cyc_Absynpp_atts2doc(f->attributes);});_tmp6DF[1]=_tmpA75;}),_tmp6DF[2]=requires_doc,({struct Cyc_PP_Doc*_tmpA74=({Cyc_PP_text(({const char*_tmp6E5=";";_tag_fat(_tmp6E5,sizeof(char),2U);}));});_tmp6DF[3]=_tmpA74;});Cyc_PP_cat(_tag_fat(_tmp6DF,sizeof(struct Cyc_PP_Doc*),4U));});}}else{_LL3: _LL4:
# 2243
 if(f->width != 0)
return({struct Cyc_PP_Doc*_tmp6E6[5U];({struct Cyc_PP_Doc*_tmpA7D=({Cyc_Absynpp_atts2doc(f->attributes);});_tmp6E6[0]=_tmpA7D;}),({
struct Cyc_PP_Doc*_tmpA7C=({struct Cyc_PP_Doc*(*_tmp6E7)(struct Cyc_Absyn_Tqual tq,void*typ,struct Cyc_Core_Opt*dopt)=Cyc_Absynpp_tqtd2doc;struct Cyc_Absyn_Tqual _tmp6E8=f->tq;void*_tmp6E9=f->type;struct Cyc_Core_Opt*_tmp6EA=({struct Cyc_Core_Opt*_tmp6EB=_cycalloc(sizeof(*_tmp6EB));({struct Cyc_PP_Doc*_tmpA7B=({Cyc_PP_textptr(f->name);});_tmp6EB->v=_tmpA7B;});_tmp6EB;});_tmp6E7(_tmp6E8,_tmp6E9,_tmp6EA);});_tmp6E6[1]=_tmpA7C;}),({
struct Cyc_PP_Doc*_tmpA7A=({Cyc_PP_text(({const char*_tmp6EC=":";_tag_fat(_tmp6EC,sizeof(char),2U);}));});_tmp6E6[2]=_tmpA7A;}),({struct Cyc_PP_Doc*_tmpA79=({Cyc_Absynpp_exp2doc((struct Cyc_Absyn_Exp*)_check_null(f->width));});_tmp6E6[3]=_tmpA79;}),({struct Cyc_PP_Doc*_tmpA78=({Cyc_PP_text(({const char*_tmp6ED=";";_tag_fat(_tmp6ED,sizeof(char),2U);}));});_tmp6E6[4]=_tmpA78;});Cyc_PP_cat(_tag_fat(_tmp6E6,sizeof(struct Cyc_PP_Doc*),5U));});else{
# 2248
return({struct Cyc_PP_Doc*_tmp6EE[4U];({struct Cyc_PP_Doc*_tmpA81=({Cyc_Absynpp_atts2doc(f->attributes);});_tmp6EE[0]=_tmpA81;}),({
struct Cyc_PP_Doc*_tmpA80=({struct Cyc_PP_Doc*(*_tmp6EF)(struct Cyc_Absyn_Tqual tq,void*typ,struct Cyc_Core_Opt*dopt)=Cyc_Absynpp_tqtd2doc;struct Cyc_Absyn_Tqual _tmp6F0=f->tq;void*_tmp6F1=f->type;struct Cyc_Core_Opt*_tmp6F2=({struct Cyc_Core_Opt*_tmp6F3=_cycalloc(sizeof(*_tmp6F3));({struct Cyc_PP_Doc*_tmpA7F=({Cyc_PP_textptr(f->name);});_tmp6F3->v=_tmpA7F;});_tmp6F3;});_tmp6EF(_tmp6F0,_tmp6F1,_tmp6F2);});_tmp6EE[1]=_tmpA80;}),_tmp6EE[2]=requires_doc,({
struct Cyc_PP_Doc*_tmpA7E=({Cyc_PP_text(({const char*_tmp6F4=";";_tag_fat(_tmp6F4,sizeof(char),2U);}));});_tmp6EE[3]=_tmpA7E;});Cyc_PP_cat(_tag_fat(_tmp6EE,sizeof(struct Cyc_PP_Doc*),4U));});}}_LL0:;}}
# 2193
struct Cyc_PP_Doc*Cyc_Absynpp_aggrfields2doc(struct Cyc_List_List*fields){
# 2255
return({({struct Cyc_PP_Doc*(*_tmpA83)(struct Cyc_PP_Doc*(*pp)(struct Cyc_Absyn_Aggrfield*),struct _fat_ptr sep,struct Cyc_List_List*l)=({struct Cyc_PP_Doc*(*_tmp6F6)(struct Cyc_PP_Doc*(*pp)(struct Cyc_Absyn_Aggrfield*),struct _fat_ptr sep,struct Cyc_List_List*l)=(struct Cyc_PP_Doc*(*)(struct Cyc_PP_Doc*(*pp)(struct Cyc_Absyn_Aggrfield*),struct _fat_ptr sep,struct Cyc_List_List*l))Cyc_PP_ppseql;_tmp6F6;});struct _fat_ptr _tmpA82=({const char*_tmp6F7="";_tag_fat(_tmp6F7,sizeof(char),1U);});_tmpA83(Cyc_Absynpp_aggrfield2doc,_tmpA82,fields);});});}
# 2193
struct Cyc_PP_Doc*Cyc_Absynpp_datatypefield2doc(struct Cyc_Absyn_Datatypefield*f){
# 2259
return({struct Cyc_PP_Doc*_tmp6F9[3U];({struct Cyc_PP_Doc*_tmpA87=({Cyc_Absynpp_scope2doc(f->sc);});_tmp6F9[0]=_tmpA87;}),({struct Cyc_PP_Doc*_tmpA86=({Cyc_Absynpp_typedef_name2doc(f->name);});_tmp6F9[1]=_tmpA86;}),
f->typs == 0?({struct Cyc_PP_Doc*_tmpA85=({Cyc_PP_nil_doc();});_tmp6F9[2]=_tmpA85;}):({struct Cyc_PP_Doc*_tmpA84=({Cyc_Absynpp_args2doc(f->typs);});_tmp6F9[2]=_tmpA84;});Cyc_PP_cat(_tag_fat(_tmp6F9,sizeof(struct Cyc_PP_Doc*),3U));});}
# 2193
struct Cyc_PP_Doc*Cyc_Absynpp_datatypefields2doc(struct Cyc_List_List*fields){
# 2264
return({({struct Cyc_PP_Doc*(*_tmpA89)(struct Cyc_PP_Doc*(*pp)(struct Cyc_Absyn_Datatypefield*),struct _fat_ptr sep,struct Cyc_List_List*l)=({struct Cyc_PP_Doc*(*_tmp6FB)(struct Cyc_PP_Doc*(*pp)(struct Cyc_Absyn_Datatypefield*),struct _fat_ptr sep,struct Cyc_List_List*l)=(struct Cyc_PP_Doc*(*)(struct Cyc_PP_Doc*(*pp)(struct Cyc_Absyn_Datatypefield*),struct _fat_ptr sep,struct Cyc_List_List*l))Cyc_PP_ppseql;_tmp6FB;});struct _fat_ptr _tmpA88=({const char*_tmp6FC=",";_tag_fat(_tmp6FC,sizeof(char),2U);});_tmpA89(Cyc_Absynpp_datatypefield2doc,_tmpA88,fields);});});}
# 2273
void Cyc_Absynpp_decllist2file(struct Cyc_List_List*tdl,struct Cyc___cycFILE*f){
for(0;tdl != 0;tdl=tdl->tl){
({void(*_tmp6FE)(struct Cyc_PP_Doc*d,int w,struct Cyc___cycFILE*f)=Cyc_PP_file_of_doc;struct Cyc_PP_Doc*_tmp6FF=({Cyc_Absynpp_decl2doc((struct Cyc_Absyn_Decl*)tdl->hd);});int _tmp700=72;struct Cyc___cycFILE*_tmp701=f;_tmp6FE(_tmp6FF,_tmp700,_tmp701);});
({void*_tmp702=0U;({struct Cyc___cycFILE*_tmpA8B=f;struct _fat_ptr _tmpA8A=({const char*_tmp703="\n";_tag_fat(_tmp703,sizeof(char),2U);});Cyc_fprintf(_tmpA8B,_tmpA8A,_tag_fat(_tmp702,sizeof(void*),0U));});});}}
# 2273
struct _fat_ptr Cyc_Absynpp_decllist2string(struct Cyc_List_List*tdl){
# 2281
return({struct _fat_ptr(*_tmp705)(struct Cyc_PP_Doc*d,int w)=Cyc_PP_string_of_doc;struct Cyc_PP_Doc*_tmp706=({struct Cyc_PP_Doc*(*_tmp707)(struct _fat_ptr sep,struct Cyc_List_List*l0)=Cyc_PP_seql;struct _fat_ptr _tmp708=({const char*_tmp709="";_tag_fat(_tmp709,sizeof(char),1U);});struct Cyc_List_List*_tmp70A=({({struct Cyc_List_List*(*_tmpA8C)(struct Cyc_PP_Doc*(*f)(struct Cyc_Absyn_Decl*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp70B)(struct Cyc_PP_Doc*(*f)(struct Cyc_Absyn_Decl*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_PP_Doc*(*f)(struct Cyc_Absyn_Decl*),struct Cyc_List_List*x))Cyc_List_map;_tmp70B;});_tmpA8C(Cyc_Absynpp_decl2doc,tdl);});});_tmp707(_tmp708,_tmp70A);});int _tmp70C=72;_tmp705(_tmp706,_tmp70C);});}
# 2273
struct _fat_ptr Cyc_Absynpp_eff2string(void*t){
# 2283
return({struct _fat_ptr(*_tmp70E)(struct Cyc_PP_Doc*d,int w)=Cyc_PP_string_of_doc;struct Cyc_PP_Doc*_tmp70F=({Cyc_Absynpp_eff2doc(t);});int _tmp710=72;_tmp70E(_tmp70F,_tmp710);});}
# 2273
struct _fat_ptr Cyc_Absynpp_exp2string(struct Cyc_Absyn_Exp*e){
# 2284
return({struct _fat_ptr(*_tmp712)(struct Cyc_PP_Doc*d,int w)=Cyc_PP_string_of_doc;struct Cyc_PP_Doc*_tmp713=({Cyc_Absynpp_exp2doc(e);});int _tmp714=72;_tmp712(_tmp713,_tmp714);});}
# 2273
struct _fat_ptr Cyc_Absynpp_stmt2string(struct Cyc_Absyn_Stmt*s){
# 2285
return({struct _fat_ptr(*_tmp716)(struct Cyc_PP_Doc*d,int w)=Cyc_PP_string_of_doc;struct Cyc_PP_Doc*_tmp717=({Cyc_Absynpp_stmt2doc(s,0,0);});int _tmp718=72;_tmp716(_tmp717,_tmp718);});}
# 2273
struct _fat_ptr Cyc_Absynpp_typ2string(void*t){
# 2286
return({struct _fat_ptr(*_tmp71A)(struct Cyc_PP_Doc*d,int w)=Cyc_PP_string_of_doc;struct Cyc_PP_Doc*_tmp71B=({Cyc_Absynpp_typ2doc(t);});int _tmp71C=72;_tmp71A(_tmp71B,_tmp71C);});}
# 2273
struct _fat_ptr Cyc_Absynpp_tvar2string(struct Cyc_Absyn_Tvar*t){
# 2287
return({struct _fat_ptr(*_tmp71E)(struct Cyc_PP_Doc*d,int w)=Cyc_PP_string_of_doc;struct Cyc_PP_Doc*_tmp71F=({Cyc_Absynpp_tvar2doc(t);});int _tmp720=72;_tmp71E(_tmp71F,_tmp720);});}
# 2273
struct _fat_ptr Cyc_Absynpp_typ2cstring(void*t){
# 2289
int old_qvar_to_Cids=Cyc_Absynpp_qvar_to_Cids;
int old_add_cyc_prefix=Cyc_Absynpp_add_cyc_prefix;
Cyc_Absynpp_qvar_to_Cids=1;
Cyc_Absynpp_add_cyc_prefix=0;{
struct _fat_ptr s=({Cyc_Absynpp_typ2string(t);});
Cyc_Absynpp_qvar_to_Cids=old_qvar_to_Cids;
Cyc_Absynpp_add_cyc_prefix=old_add_cyc_prefix;
return s;}}
# 2273
struct _fat_ptr Cyc_Absynpp_prim2string(enum Cyc_Absyn_Primop p){
# 2298
return({struct _fat_ptr(*_tmp723)(struct Cyc_PP_Doc*d,int w)=Cyc_PP_string_of_doc;struct Cyc_PP_Doc*_tmp724=({Cyc_Absynpp_prim2doc(p);});int _tmp725=72;_tmp723(_tmp724,_tmp725);});}
# 2273
struct _fat_ptr Cyc_Absynpp_pat2string(struct Cyc_Absyn_Pat*p){
# 2299
return({struct _fat_ptr(*_tmp727)(struct Cyc_PP_Doc*d,int w)=Cyc_PP_string_of_doc;struct Cyc_PP_Doc*_tmp728=({Cyc_Absynpp_pat2doc(p);});int _tmp729=72;_tmp727(_tmp728,_tmp729);});}
# 2273
struct _fat_ptr Cyc_Absynpp_scope2string(enum Cyc_Absyn_Scope sc){
# 2300
return({struct _fat_ptr(*_tmp72B)(struct Cyc_PP_Doc*d,int w)=Cyc_PP_string_of_doc;struct Cyc_PP_Doc*_tmp72C=({Cyc_Absynpp_scope2doc(sc);});int _tmp72D=72;_tmp72B(_tmp72C,_tmp72D);});}
# 2273
struct _fat_ptr Cyc_Absynpp_cnst2string(union Cyc_Absyn_Cnst c){
# 2301
return({struct _fat_ptr(*_tmp72F)(struct Cyc_PP_Doc*d,int w)=Cyc_PP_string_of_doc;struct Cyc_PP_Doc*_tmp730=({Cyc_Absynpp_cnst2doc(c);});int _tmp731=72;_tmp72F(_tmp730,_tmp731);});}

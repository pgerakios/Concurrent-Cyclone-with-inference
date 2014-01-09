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
int Cyc_List_length(struct Cyc_List_List*x);
# 86
struct Cyc_List_List*Cyc_List_rmap_c(struct _RegionHandle*,void*(*f)(void*,void*),void*env,struct Cyc_List_List*x);extern char Cyc_List_List_mismatch[14U];struct Cyc_List_List_mismatch_exn_struct{char*tag;};
# 178
struct Cyc_List_List*Cyc_List_imp_rev(struct Cyc_List_List*x);extern char Cyc_List_Nth[4U];struct Cyc_List_Nth_exn_struct{char*tag;};
# 322
int Cyc_List_mem(int(*compare)(void*,void*),struct Cyc_List_List*l,void*x);
# 383
int Cyc_List_list_cmp(int(*cmp)(void*,void*),struct Cyc_List_List*l1,struct Cyc_List_List*l2);
# 394
struct Cyc_List_List*Cyc_List_filter_c(int(*f)(void*,void*),void*env,struct Cyc_List_List*x);struct Cyc_Iter_Iter{void*env;int(*next)(void*env,void*dest);};
# 37 "iter.h"
int Cyc_Iter_next(struct Cyc_Iter_Iter,void*);struct Cyc_Set_Set;extern char Cyc_Set_Absent[7U];struct Cyc_Set_Absent_exn_struct{char*tag;};struct Cyc___cycFILE;
# 53 "cycboot.h"
extern struct Cyc___cycFILE*Cyc_stderr;struct Cyc_String_pa_PrintArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Int_pa_PrintArg_struct{int tag;unsigned long f1;};struct Cyc_Double_pa_PrintArg_struct{int tag;double f1;};struct Cyc_LongDouble_pa_PrintArg_struct{int tag;long double f1;};struct Cyc_ShortPtr_pa_PrintArg_struct{int tag;short*f1;};struct Cyc_IntPtr_pa_PrintArg_struct{int tag;unsigned long*f1;};
# 73
struct _fat_ptr Cyc_aprintf(struct _fat_ptr,struct _fat_ptr);
# 100
int Cyc_fprintf(struct Cyc___cycFILE*,struct _fat_ptr,struct _fat_ptr);struct Cyc_ShortPtr_sa_ScanfArg_struct{int tag;short*f1;};struct Cyc_UShortPtr_sa_ScanfArg_struct{int tag;unsigned short*f1;};struct Cyc_IntPtr_sa_ScanfArg_struct{int tag;int*f1;};struct Cyc_UIntPtr_sa_ScanfArg_struct{int tag;unsigned*f1;};struct Cyc_StringPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_DoublePtr_sa_ScanfArg_struct{int tag;double*f1;};struct Cyc_FloatPtr_sa_ScanfArg_struct{int tag;float*f1;};struct Cyc_CharPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};extern char Cyc_FileCloseError[15U];struct Cyc_FileCloseError_exn_struct{char*tag;};extern char Cyc_FileOpenError[14U];struct Cyc_FileOpenError_exn_struct{char*tag;struct _fat_ptr f1;};struct Cyc_Dict_T;struct Cyc_Dict_Dict{int(*rel)(void*,void*);struct _RegionHandle*r;const struct Cyc_Dict_T*t;};extern char Cyc_Dict_Present[8U];struct Cyc_Dict_Present_exn_struct{char*tag;};extern char Cyc_Dict_Absent[7U];struct Cyc_Dict_Absent_exn_struct{char*tag;};
# 62 "dict.h"
struct Cyc_Dict_Dict Cyc_Dict_empty(int(*cmp)(void*,void*));
# 87
struct Cyc_Dict_Dict Cyc_Dict_insert(struct Cyc_Dict_Dict d,void*k,void*v);
# 110
void*Cyc_Dict_lookup(struct Cyc_Dict_Dict d,void*k);
# 147 "dict.h"
void Cyc_Dict_iter(void(*f)(void*,void*),struct Cyc_Dict_Dict d);
# 212
struct Cyc_Dict_Dict Cyc_Dict_intersect_c(void*(*f)(void*,void*,void*,void*),void*env,struct Cyc_Dict_Dict d1,struct Cyc_Dict_Dict d2);
# 222
int Cyc_Dict_forall_intersect(int(*f)(void*,void*,void*),struct Cyc_Dict_Dict d1,struct Cyc_Dict_Dict d2);
# 283
struct Cyc_Iter_Iter Cyc_Dict_make_iter(struct _RegionHandle*rgn,struct Cyc_Dict_Dict d);
# 50 "string.h"
int Cyc_strptrcmp(struct _fat_ptr*s1,struct _fat_ptr*s2);struct Cyc_Position_Error;struct Cyc_Relations_Reln;struct _union_Nmspace_Rel_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Abs_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_C_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Loc_n{int tag;int val;};union Cyc_Absyn_Nmspace{struct _union_Nmspace_Rel_n Rel_n;struct _union_Nmspace_Abs_n Abs_n;struct _union_Nmspace_C_n C_n;struct _union_Nmspace_Loc_n Loc_n;};struct _tuple0{union Cyc_Absyn_Nmspace f1;struct _fat_ptr*f2;};
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
# 947 "absyn.h"
extern struct Cyc_Absyn_EmptyAnnot_Absyn_AbsynAnnot_struct Cyc_Absyn_EmptyAnnot_val;
# 997
extern void*Cyc_Absyn_void_type;
# 1225
struct Cyc_Absyn_Aggrdecl*Cyc_Absyn_get_known_aggrdecl(union Cyc_Absyn_AggrInfo);
# 1227
int Cyc_Absyn_is_nontagged_nonrequire_union_type(void*);struct _union_RelnOp_RConst{int tag;unsigned val;};struct _union_RelnOp_RVar{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RNumelts{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RType{int tag;void*val;};struct _union_RelnOp_RParam{int tag;unsigned val;};struct _union_RelnOp_RParamNumelts{int tag;unsigned val;};struct _union_RelnOp_RReturn{int tag;unsigned val;};union Cyc_Relations_RelnOp{struct _union_RelnOp_RConst RConst;struct _union_RelnOp_RVar RVar;struct _union_RelnOp_RNumelts RNumelts;struct _union_RelnOp_RType RType;struct _union_RelnOp_RParam RParam;struct _union_RelnOp_RParamNumelts RParamNumelts;struct _union_RelnOp_RReturn RReturn;};
# 50 "relations-ap.h"
enum Cyc_Relations_Relation{Cyc_Relations_Req =0U,Cyc_Relations_Rneq =1U,Cyc_Relations_Rlte =2U,Cyc_Relations_Rlt =3U};struct Cyc_Relations_Reln{union Cyc_Relations_RelnOp rop1;enum Cyc_Relations_Relation relation;union Cyc_Relations_RelnOp rop2;};
# 114
int Cyc_Relations_relns_approx(struct Cyc_List_List*r2s,struct Cyc_List_List*r1s);
# 116
struct Cyc_List_List*Cyc_Relations_join_relns(struct _RegionHandle*r,struct Cyc_List_List*,struct Cyc_List_List*);
# 118
void Cyc_Relations_print_relns(struct Cyc___cycFILE*,struct Cyc_List_List*);struct Cyc_RgnOrder_RgnPO;extern char Cyc_Tcenv_Env_error[10U];struct Cyc_Tcenv_Env_error_exn_struct{char*tag;};struct Cyc_Tcenv_Genv{struct Cyc_Dict_Dict aggrdecls;struct Cyc_Dict_Dict datatypedecls;struct Cyc_Dict_Dict enumdecls;struct Cyc_Dict_Dict typedefs;struct Cyc_Dict_Dict ordinaries;};struct Cyc_Tcenv_Fenv;struct Cyc_Tcenv_Tenv{struct Cyc_List_List*ns;struct Cyc_Tcenv_Genv*ae;struct Cyc_Tcenv_Fenv*le;int allow_valueof: 1;int in_extern_c_include: 1;int in_tempest: 1;int tempest_generalize: 1;};
# 90 "tcenv.h"
enum Cyc_Tcenv_NewStatus{Cyc_Tcenv_NoneNew =0U,Cyc_Tcenv_InNew =1U,Cyc_Tcenv_InNewAggr =2U};
# 30 "tcutil.h"
void*Cyc_Tcutil_impos(struct _fat_ptr,struct _fat_ptr);
# 60
int Cyc_Tcutil_is_bits_only_type(void*);
# 108
struct Cyc_Absyn_Kind*Cyc_Tcutil_tvar_kind(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Kind*def);
# 110
void*Cyc_Tcutil_compress(void*);
# 141
extern struct Cyc_Absyn_Kind Cyc_Tcutil_bk;
# 191
void*Cyc_Tcutil_rsubstitute(struct _RegionHandle*,struct Cyc_List_List*,void*);
# 197
struct Cyc_Absyn_Exp*Cyc_Tcutil_rsubsexp(struct _RegionHandle*,struct Cyc_List_List*,struct Cyc_Absyn_Exp*);
# 240
int Cyc_Tcutil_is_noalias_pointer(void*t,int must_be_unique);
# 310
int Cyc_Tcutil_force_type2bool(int desired,void*);struct Cyc_PP_Ppstate;struct Cyc_PP_Out;struct Cyc_PP_Doc;struct Cyc_Absynpp_Params{int expand_typedefs;int qvar_to_Cids;int add_cyc_prefix;int to_VC;int decls_first;int rewrite_temp_tvars;int print_all_tvars;int print_all_kinds;int print_all_effects;int print_using_stmts;int print_externC_stmts;int print_full_evars;int print_zeroterm;int generate_line_directives;int use_curr_namespace;struct Cyc_List_List*curr_namespace;};
# 62 "absynpp.h"
struct _fat_ptr Cyc_Absynpp_typ2string(void*);
# 67
struct _fat_ptr Cyc_Absynpp_exp2string(struct Cyc_Absyn_Exp*);
# 69
struct _fat_ptr Cyc_Absynpp_qvar2string(struct _tuple0*);
# 33 "warn.h"
void Cyc_Warn_verr(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap);
# 40
void*Cyc_Warn_impos(struct _fat_ptr fmt,struct _fat_ptr ap);struct Cyc_Warn_String_Warn_Warg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Warn_Exp_Warn_Warg_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Warn_Typ_Warn_Warg_struct{int tag;void*f1;};struct Cyc_Warn_Qvar_Warn_Warg_struct{int tag;struct _tuple0*f1;};
# 41 "cf_flowinfo.h"
int Cyc_CfFlowInfo_anal_error;
void Cyc_CfFlowInfo_aerr(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap);struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_CfFlowInfo_MallocPt_CfFlowInfo_Root_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;};struct Cyc_CfFlowInfo_InitParam_CfFlowInfo_Root_struct{int tag;int f1;void*f2;};struct Cyc_CfFlowInfo_Dot_CfFlowInfo_PathCon_struct{int tag;int f1;};struct Cyc_CfFlowInfo_Star_CfFlowInfo_PathCon_struct{int tag;};struct Cyc_CfFlowInfo_Place{void*root;struct Cyc_List_List*path;};
# 74
enum Cyc_CfFlowInfo_InitLevel{Cyc_CfFlowInfo_NoneIL =0U,Cyc_CfFlowInfo_AllIL =1U};char Cyc_CfFlowInfo_IsZero[7U]="IsZero";struct Cyc_CfFlowInfo_IsZero_Absyn_AbsynAnnot_struct{char*tag;};char Cyc_CfFlowInfo_NotZero[8U]="NotZero";struct Cyc_CfFlowInfo_NotZero_Absyn_AbsynAnnot_struct{char*tag;struct Cyc_List_List*f1;};char Cyc_CfFlowInfo_UnknownZ[9U]="UnknownZ";struct Cyc_CfFlowInfo_UnknownZ_Absyn_AbsynAnnot_struct{char*tag;struct Cyc_List_List*f1;};struct _union_AbsLVal_PlaceL{int tag;struct Cyc_CfFlowInfo_Place*val;};struct _union_AbsLVal_UnknownL{int tag;int val;};union Cyc_CfFlowInfo_AbsLVal{struct _union_AbsLVal_PlaceL PlaceL;struct _union_AbsLVal_UnknownL UnknownL;};struct Cyc_CfFlowInfo_UnionRInfo{int is_union;int fieldnum;};struct Cyc_CfFlowInfo_Zero_CfFlowInfo_AbsRVal_struct{int tag;};struct Cyc_CfFlowInfo_NotZeroAll_CfFlowInfo_AbsRVal_struct{int tag;};struct Cyc_CfFlowInfo_UnknownR_CfFlowInfo_AbsRVal_struct{int tag;enum Cyc_CfFlowInfo_InitLevel f1;};struct Cyc_CfFlowInfo_Esc_CfFlowInfo_AbsRVal_struct{int tag;enum Cyc_CfFlowInfo_InitLevel f1;};struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct{int tag;struct Cyc_CfFlowInfo_Place*f1;};struct Cyc_CfFlowInfo_UniquePtr_CfFlowInfo_AbsRVal_struct{int tag;void*f1;};struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct{int tag;struct Cyc_CfFlowInfo_UnionRInfo f1;struct _fat_ptr f2;};struct Cyc_CfFlowInfo_Consumed_CfFlowInfo_AbsRVal_struct{int tag;struct Cyc_Absyn_Exp*f1;int f2;void*f3;};struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct{int tag;struct Cyc_Absyn_Vardecl*f1;void*f2;};struct _union_FlowInfo_BottomFL{int tag;int val;};struct _tuple12{struct Cyc_Dict_Dict f1;struct Cyc_List_List*f2;};struct _union_FlowInfo_ReachableFL{int tag;struct _tuple12 val;};union Cyc_CfFlowInfo_FlowInfo{struct _union_FlowInfo_BottomFL BottomFL;struct _union_FlowInfo_ReachableFL ReachableFL;};
# 139 "cf_flowinfo.h"
union Cyc_CfFlowInfo_FlowInfo Cyc_CfFlowInfo_ReachableFL(struct Cyc_Dict_Dict,struct Cyc_List_List*);struct Cyc_CfFlowInfo_FlowEnv{void*zero;void*notzeroall;void*unknown_none;void*unknown_all;void*esc_none;void*esc_all;struct Cyc_Dict_Dict mt_flowdict;struct Cyc_CfFlowInfo_Place*dummy_place;};
# 156
int Cyc_CfFlowInfo_get_field_index_fs(struct Cyc_List_List*fs,struct _fat_ptr*f);
# 158
int Cyc_CfFlowInfo_root_cmp(void*,void*);
int Cyc_CfFlowInfo_place_cmp(struct Cyc_CfFlowInfo_Place*,struct Cyc_CfFlowInfo_Place*);
# 161
struct _fat_ptr Cyc_CfFlowInfo_aggrfields_to_aggrdict(struct Cyc_CfFlowInfo_FlowEnv*,struct Cyc_List_List*,int no_init_bits_only,void*);
# 163
void*Cyc_CfFlowInfo_make_unique_consumed(struct Cyc_CfFlowInfo_FlowEnv*fenv,void*t,struct Cyc_Absyn_Exp*consumer,int iteration,void*);
int Cyc_CfFlowInfo_is_unique_consumed(struct Cyc_Absyn_Exp*e,int env_iteration,void*r,int*needs_unconsume);
void*Cyc_CfFlowInfo_make_unique_unconsumed(struct Cyc_CfFlowInfo_FlowEnv*fenv,void*r);
# 168
enum Cyc_CfFlowInfo_InitLevel Cyc_CfFlowInfo_initlevel(struct Cyc_CfFlowInfo_FlowEnv*,struct Cyc_Dict_Dict d,void*r);
void*Cyc_CfFlowInfo_lookup_place(struct Cyc_Dict_Dict d,struct Cyc_CfFlowInfo_Place*place);
# 171
int Cyc_CfFlowInfo_is_init_pointer(void*r);
int Cyc_CfFlowInfo_flow_lessthan_approx(union Cyc_CfFlowInfo_FlowInfo f1,union Cyc_CfFlowInfo_FlowInfo f2);
# 174
void Cyc_CfFlowInfo_print_absrval(void*rval);
void Cyc_CfFlowInfo_print_initlevel(enum Cyc_CfFlowInfo_InitLevel il);
void Cyc_CfFlowInfo_print_root(void*root);
void Cyc_CfFlowInfo_print_path(struct Cyc_List_List*p);
void Cyc_CfFlowInfo_print_place(struct Cyc_CfFlowInfo_Place*p);
# 180
void Cyc_CfFlowInfo_print_flowdict(struct Cyc_Dict_Dict d);
# 201 "cf_flowinfo.h"
union Cyc_CfFlowInfo_FlowInfo Cyc_CfFlowInfo_join_flow(struct Cyc_CfFlowInfo_FlowEnv*,union Cyc_CfFlowInfo_FlowInfo,union Cyc_CfFlowInfo_FlowInfo);
# 38 "cf_flowinfo.cyc"
int Cyc_CfFlowInfo_anal_error=0;
void Cyc_CfFlowInfo_aerr(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap){
# 41
Cyc_CfFlowInfo_anal_error=1;
({Cyc_Warn_verr(loc,fmt,ap);});}
# 39
struct Cyc_CfFlowInfo_IsZero_Absyn_AbsynAnnot_struct Cyc_CfFlowInfo_IsZero_val={Cyc_CfFlowInfo_IsZero};
# 47
union Cyc_CfFlowInfo_AbsLVal Cyc_CfFlowInfo_PlaceL(struct Cyc_CfFlowInfo_Place*x){
return({union Cyc_CfFlowInfo_AbsLVal _tmp230;(_tmp230.PlaceL).tag=1U,(_tmp230.PlaceL).val=x;_tmp230;});}
# 47
union Cyc_CfFlowInfo_AbsLVal Cyc_CfFlowInfo_UnknownL(){
# 51
return({union Cyc_CfFlowInfo_AbsLVal _tmp231;(_tmp231.UnknownL).tag=2U,(_tmp231.UnknownL).val=0;_tmp231;});}
# 47
union Cyc_CfFlowInfo_FlowInfo Cyc_CfFlowInfo_BottomFL(){
# 53
return({union Cyc_CfFlowInfo_FlowInfo _tmp232;(_tmp232.BottomFL).tag=1U,(_tmp232.BottomFL).val=0;_tmp232;});}
# 47
union Cyc_CfFlowInfo_FlowInfo Cyc_CfFlowInfo_ReachableFL(struct Cyc_Dict_Dict fd,struct Cyc_List_List*r){
# 55
return({union Cyc_CfFlowInfo_FlowInfo _tmp233;(_tmp233.ReachableFL).tag=2U,((_tmp233.ReachableFL).val).f1=fd,((_tmp233.ReachableFL).val).f2=r;_tmp233;});}
# 47
struct Cyc_CfFlowInfo_FlowEnv*Cyc_CfFlowInfo_new_flow_env(){
# 59
static struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct dummy_rawexp={0U,{.Null_c={1,0}}};
static struct Cyc_Absyn_Exp dummy_exp={0,(void*)& dummy_rawexp,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};
return({struct Cyc_CfFlowInfo_FlowEnv*_tmpD=_cycalloc(sizeof(*_tmpD));
({void*_tmp26D=(void*)({struct Cyc_CfFlowInfo_Zero_CfFlowInfo_AbsRVal_struct*_tmp5=_cycalloc(sizeof(*_tmp5));_tmp5->tag=0U;_tmp5;});_tmpD->zero=_tmp26D;}),({
void*_tmp26C=(void*)({struct Cyc_CfFlowInfo_NotZeroAll_CfFlowInfo_AbsRVal_struct*_tmp6=_cycalloc(sizeof(*_tmp6));_tmp6->tag=1U;_tmp6;});_tmpD->notzeroall=_tmp26C;}),({
void*_tmp26B=(void*)({struct Cyc_CfFlowInfo_UnknownR_CfFlowInfo_AbsRVal_struct*_tmp7=_cycalloc(sizeof(*_tmp7));_tmp7->tag=2U,_tmp7->f1=Cyc_CfFlowInfo_NoneIL;_tmp7;});_tmpD->unknown_none=_tmp26B;}),({
void*_tmp26A=(void*)({struct Cyc_CfFlowInfo_UnknownR_CfFlowInfo_AbsRVal_struct*_tmp8=_cycalloc(sizeof(*_tmp8));_tmp8->tag=2U,_tmp8->f1=Cyc_CfFlowInfo_AllIL;_tmp8;});_tmpD->unknown_all=_tmp26A;}),({
void*_tmp269=(void*)({struct Cyc_CfFlowInfo_Esc_CfFlowInfo_AbsRVal_struct*_tmp9=_cycalloc(sizeof(*_tmp9));_tmp9->tag=3U,_tmp9->f1=Cyc_CfFlowInfo_NoneIL;_tmp9;});_tmpD->esc_none=_tmp269;}),({
void*_tmp268=(void*)({struct Cyc_CfFlowInfo_Esc_CfFlowInfo_AbsRVal_struct*_tmpA=_cycalloc(sizeof(*_tmpA));_tmpA->tag=3U,_tmpA->f1=Cyc_CfFlowInfo_AllIL;_tmpA;});_tmpD->esc_all=_tmp268;}),({
struct Cyc_Dict_Dict _tmp267=({Cyc_Dict_empty(Cyc_CfFlowInfo_root_cmp);});_tmpD->mt_flowdict=_tmp267;}),({
struct Cyc_CfFlowInfo_Place*_tmp266=({struct Cyc_CfFlowInfo_Place*_tmpC=_cycalloc(sizeof(*_tmpC));({void*_tmp265=(void*)({struct Cyc_CfFlowInfo_MallocPt_CfFlowInfo_Root_struct*_tmpB=_cycalloc(sizeof(*_tmpB));_tmpB->tag=1U,_tmpB->f1=& dummy_exp,_tmpB->f2=Cyc_Absyn_void_type;_tmpB;});_tmpC->root=_tmp265;}),_tmpC->path=0;_tmpC;});_tmpD->dummy_place=_tmp266;});_tmpD;});}
# 47
struct _fat_ptr Cyc_CfFlowInfo_place_err_string(struct Cyc_CfFlowInfo_Place*place){
# 75
struct Cyc_CfFlowInfo_Place _stmttmp0=*place;struct Cyc_List_List*_tmp10;void*_tmpF;_LL1: _tmpF=_stmttmp0.root;_tmp10=_stmttmp0.path;_LL2: {void*root=_tmpF;struct Cyc_List_List*fields=_tmp10;
void*_tmp11=root;struct Cyc_Absyn_Vardecl*_tmp12;if(((struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct*)_tmp11)->tag == 0U){_LL4: _tmp12=((struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct*)_tmp11)->f1;_LL5: {struct Cyc_Absyn_Vardecl*vd=_tmp12;
# 78
if(fields == 0)
return({Cyc_Absynpp_qvar2string(vd->name);});else{
# 81
return(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp15=({struct Cyc_String_pa_PrintArg_struct _tmp234;_tmp234.tag=0U,({struct _fat_ptr _tmp26E=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(vd->name);}));_tmp234.f1=_tmp26E;});_tmp234;});void*_tmp13[1U];_tmp13[0]=& _tmp15;({struct _fat_ptr _tmp26F=({const char*_tmp14="reachable from %s";_tag_fat(_tmp14,sizeof(char),18U);});Cyc_aprintf(_tmp26F,_tag_fat(_tmp13,sizeof(void*),1U));});});}}}else{_LL6: _LL7:
({void*_tmp16=0U;({int(*_tmp271)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp18)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp18;});struct _fat_ptr _tmp270=({const char*_tmp17="error locations not for VarRoots";_tag_fat(_tmp17,sizeof(char),33U);});_tmp271(_tmp270,_tag_fat(_tmp16,sizeof(void*),0U));});});}_LL3:;}}
# 87
int Cyc_CfFlowInfo_get_field_index_fs(struct Cyc_List_List*fs,struct _fat_ptr*f){
int n=0;
for(0;(unsigned)fs;fs=fs->tl){
struct _fat_ptr*f2=((struct Cyc_Absyn_Aggrfield*)fs->hd)->name;
if(({Cyc_strptrcmp(f2,f);})== 0)return n;n=n + 1;}
# 94
({struct Cyc_String_pa_PrintArg_struct _tmp1D=({struct Cyc_String_pa_PrintArg_struct _tmp235;_tmp235.tag=0U,_tmp235.f1=(struct _fat_ptr)((struct _fat_ptr)*f);_tmp235;});void*_tmp1A[1U];_tmp1A[0]=& _tmp1D;({int(*_tmp273)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp1C)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp1C;});struct _fat_ptr _tmp272=({const char*_tmp1B="get_field_index_fs failed: %s";_tag_fat(_tmp1B,sizeof(char),30U);});_tmp273(_tmp272,_tag_fat(_tmp1A,sizeof(void*),1U));});});}
# 97
int Cyc_CfFlowInfo_get_field_index(void*t,struct _fat_ptr*f){
void*_stmttmp1=({Cyc_Tcutil_compress(t);});void*_tmp1F=_stmttmp1;struct Cyc_List_List*_tmp20;union Cyc_Absyn_AggrInfo _tmp21;switch(*((int*)_tmp1F)){case 0U: if(((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp1F)->f1)->tag == 22U){_LL1: _tmp21=((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp1F)->f1)->f1;_LL2: {union Cyc_Absyn_AggrInfo info=_tmp21;
# 100
struct Cyc_List_List*fs=((struct Cyc_Absyn_AggrdeclImpl*)_check_null(({Cyc_Absyn_get_known_aggrdecl(info);})->impl))->fields;
_tmp20=fs;goto _LL4;}}else{goto _LL5;}case 7U: _LL3: _tmp20=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp1F)->f2;_LL4: {struct Cyc_List_List*fs=_tmp20;
# 103
return({Cyc_CfFlowInfo_get_field_index_fs(fs,f);});}default: _LL5: _LL6:
# 105
({struct Cyc_String_pa_PrintArg_struct _tmp25=({struct Cyc_String_pa_PrintArg_struct _tmp236;_tmp236.tag=0U,({struct _fat_ptr _tmp274=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t);}));_tmp236.f1=_tmp274;});_tmp236;});void*_tmp22[1U];_tmp22[0]=& _tmp25;({int(*_tmp276)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp24)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp24;});struct _fat_ptr _tmp275=({const char*_tmp23="get_field_index failed: %s";_tag_fat(_tmp23,sizeof(char),27U);});_tmp276(_tmp275,_tag_fat(_tmp22,sizeof(void*),1U));});});}_LL0:;}struct _tuple13{void*f1;void*f2;};
# 109
int Cyc_CfFlowInfo_root_cmp(void*r1,void*r2){
if((int)r1 == (int)r2)
return 0;{
# 110
struct _tuple13 _stmttmp2=({struct _tuple13 _tmp237;_tmp237.f1=r1,_tmp237.f2=r2;_tmp237;});struct _tuple13 _tmp27=_stmttmp2;int _tmp29;int _tmp28;struct Cyc_Absyn_Exp*_tmp2B;struct Cyc_Absyn_Exp*_tmp2A;struct Cyc_Absyn_Vardecl*_tmp2D;struct Cyc_Absyn_Vardecl*_tmp2C;if(((struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct*)_tmp27.f1)->tag == 0U){if(((struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct*)_tmp27.f2)->tag == 0U){_LL1: _tmp2C=((struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct*)_tmp27.f1)->f1;_tmp2D=((struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct*)_tmp27.f2)->f1;_LL2: {struct Cyc_Absyn_Vardecl*vd1=_tmp2C;struct Cyc_Absyn_Vardecl*vd2=_tmp2D;
# 113
return(int)vd1 - (int)vd2;}}else{_LL3: _LL4:
 return - 1;}}else{if(((struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct*)_tmp27.f2)->tag == 0U){_LL5: _LL6:
 return 1;}else{if(((struct Cyc_CfFlowInfo_MallocPt_CfFlowInfo_Root_struct*)_tmp27.f1)->tag == 1U){if(((struct Cyc_CfFlowInfo_MallocPt_CfFlowInfo_Root_struct*)_tmp27.f2)->tag == 1U){_LL7: _tmp2A=((struct Cyc_CfFlowInfo_MallocPt_CfFlowInfo_Root_struct*)_tmp27.f1)->f1;_tmp2B=((struct Cyc_CfFlowInfo_MallocPt_CfFlowInfo_Root_struct*)_tmp27.f2)->f1;_LL8: {struct Cyc_Absyn_Exp*e1=_tmp2A;struct Cyc_Absyn_Exp*e2=_tmp2B;
return(int)e1 - (int)e2;}}else{_LL9: _LLA:
 return - 1;}}else{if(((struct Cyc_CfFlowInfo_MallocPt_CfFlowInfo_Root_struct*)_tmp27.f2)->tag == 1U){_LLB: _LLC:
 return 1;}else{_LLD: _tmp28=((struct Cyc_CfFlowInfo_InitParam_CfFlowInfo_Root_struct*)_tmp27.f1)->f1;_tmp29=((struct Cyc_CfFlowInfo_InitParam_CfFlowInfo_Root_struct*)_tmp27.f2)->f1;_LLE: {int i1=_tmp28;int i2=_tmp29;
return i1 - i2;}}}}}_LL0:;}}
# 123
static int Cyc_CfFlowInfo_pathcon_cmp(void*p1,void*p2){
struct _tuple13 _stmttmp3=({struct _tuple13 _tmp238;_tmp238.f1=p1,_tmp238.f2=p2;_tmp238;});struct _tuple13 _tmp2F=_stmttmp3;int _tmp31;int _tmp30;if(((struct Cyc_CfFlowInfo_Dot_CfFlowInfo_PathCon_struct*)_tmp2F.f1)->tag == 0U){if(((struct Cyc_CfFlowInfo_Dot_CfFlowInfo_PathCon_struct*)_tmp2F.f2)->tag == 0U){_LL1: _tmp30=((struct Cyc_CfFlowInfo_Dot_CfFlowInfo_PathCon_struct*)_tmp2F.f1)->f1;_tmp31=((struct Cyc_CfFlowInfo_Dot_CfFlowInfo_PathCon_struct*)_tmp2F.f2)->f1;_LL2: {int i1=_tmp30;int i2=_tmp31;
# 126
if(i1 == i2)return 0;if(i1 < i2)
return - 1;else{
return 1;}}}else{_LL7: _LL8:
# 131
 return 1;}}else{if(((struct Cyc_CfFlowInfo_Star_CfFlowInfo_PathCon_struct*)_tmp2F.f2)->tag == 1U){_LL3: _LL4:
# 129
 return 0;}else{_LL5: _LL6:
 return - 1;}}_LL0:;}
# 135
static int Cyc_CfFlowInfo_path_cmp(struct Cyc_List_List*path1,struct Cyc_List_List*path2){
return({Cyc_List_list_cmp(Cyc_CfFlowInfo_pathcon_cmp,path1,path2);});}
# 139
int Cyc_CfFlowInfo_place_cmp(struct Cyc_CfFlowInfo_Place*p1,struct Cyc_CfFlowInfo_Place*p2){
if((int)p1 == (int)p2)
return 0;{
# 140
struct Cyc_CfFlowInfo_Place _stmttmp4=*p1;struct Cyc_List_List*_tmp35;void*_tmp34;_LL1: _tmp34=_stmttmp4.root;_tmp35=_stmttmp4.path;_LL2: {void*root1=_tmp34;struct Cyc_List_List*path1=_tmp35;
# 143
struct Cyc_CfFlowInfo_Place _stmttmp5=*p2;struct Cyc_List_List*_tmp37;void*_tmp36;_LL4: _tmp36=_stmttmp5.root;_tmp37=_stmttmp5.path;_LL5: {void*root2=_tmp36;struct Cyc_List_List*path2=_tmp37;
int i=({Cyc_CfFlowInfo_root_cmp(root1,root2);});
if(i != 0)
return i;
# 145
return({Cyc_CfFlowInfo_path_cmp(path1,path2);});}}}}
# 139
static void*Cyc_CfFlowInfo_i_typ_to_absrval(struct Cyc_CfFlowInfo_FlowEnv*fenv,int allow_zeroterm,int no_init_bits_only,void*t,void*leafval);
# 165 "cf_flowinfo.cyc"
struct _fat_ptr Cyc_CfFlowInfo_aggrfields_to_aggrdict(struct Cyc_CfFlowInfo_FlowEnv*fenv,struct Cyc_List_List*fs,int no_init_bits_only,void*leafval){
# 171
unsigned sz=(unsigned)({Cyc_List_length(fs);});
struct _fat_ptr d=({unsigned _tmp3C=sz;void**_tmp3B=_cycalloc(_check_times(_tmp3C,sizeof(void*)));({{unsigned _tmp239=sz;unsigned i;for(i=0;i < _tmp239;++ i){_tmp3B[i]=fenv->unknown_all;}}0;});_tag_fat(_tmp3B,sizeof(void*),_tmp3C);});
{int i=0;for(0;(unsigned)i < sz;(i ++,fs=fs->tl)){
struct Cyc_Absyn_Aggrfield*_stmttmp6=(struct Cyc_Absyn_Aggrfield*)((struct Cyc_List_List*)_check_null(fs))->hd;void*_tmp3A;struct _fat_ptr*_tmp39;_LL1: _tmp39=_stmttmp6->name;_tmp3A=_stmttmp6->type;_LL2: {struct _fat_ptr*n=_tmp39;void*t2=_tmp3A;
if(_get_fat_size(*n,sizeof(char))!= (unsigned)1)
({void*_tmp277=({Cyc_CfFlowInfo_i_typ_to_absrval(fenv,0,no_init_bits_only,t2,leafval);});*((void**)_check_fat_subscript(d,sizeof(void*),i))=_tmp277;});}}}
# 179
return d;}struct _tuple14{struct _RegionHandle*f1;struct Cyc_List_List*f2;};
# 184
static struct Cyc_Absyn_Aggrfield*Cyc_CfFlowInfo_substitute_field(struct _tuple14*env,struct Cyc_Absyn_Aggrfield*f){
# 187
struct Cyc_List_List*_tmp3F;struct _RegionHandle*_tmp3E;_LL1: _tmp3E=env->f1;_tmp3F=env->f2;_LL2: {struct _RegionHandle*t=_tmp3E;struct Cyc_List_List*inst=_tmp3F;
void*new_typ=({Cyc_Tcutil_rsubstitute(t,inst,f->type);});
struct Cyc_Absyn_Exp*r=f->requires_clause;
struct Cyc_Absyn_Exp*new_r=r == 0?0:({Cyc_Tcutil_rsubsexp(t,inst,r);});
return({struct Cyc_Absyn_Aggrfield*_tmp40=_region_malloc(t,sizeof(*_tmp40));_tmp40->name=f->name,_tmp40->tq=f->tq,_tmp40->type=new_typ,_tmp40->width=f->width,_tmp40->attributes=f->attributes,_tmp40->requires_clause=new_r;_tmp40;});}}struct _tuple15{struct Cyc_Absyn_Tvar*f1;void*f2;};
# 198
static struct _fat_ptr Cyc_CfFlowInfo_substitute_aggrfields_to_aggrdict(struct Cyc_CfFlowInfo_FlowEnv*fenv,struct Cyc_List_List*tvs,struct Cyc_List_List*targs,struct Cyc_List_List*fs,int no_init_bits_only,void*leafval){
# 204
struct _RegionHandle _tmp42=_new_region("temp");struct _RegionHandle*temp=& _tmp42;_push_region(temp);
# 208
{struct Cyc_List_List*inst=0;
for(0;tvs != 0;(tvs=tvs->tl,targs=targs->tl)){
struct Cyc_Absyn_Tvar*tv=(struct Cyc_Absyn_Tvar*)tvs->hd;
void*t=(void*)((struct Cyc_List_List*)_check_null(targs))->hd;
{struct Cyc_Absyn_Kind*_stmttmp7=({Cyc_Tcutil_tvar_kind(tv,& Cyc_Tcutil_bk);});struct Cyc_Absyn_Kind*_tmp43=_stmttmp7;switch(((struct Cyc_Absyn_Kind*)_tmp43)->kind){case Cyc_Absyn_RgnKind: _LL1: _LL2:
 goto _LL4;case Cyc_Absyn_EffKind: _LL3: _LL4:
 continue;default: _LL5: _LL6:
 goto _LL0;}_LL0:;}
# 217
inst=({struct Cyc_List_List*_tmp45=_region_malloc(temp,sizeof(*_tmp45));({struct _tuple15*_tmp278=({struct _tuple15*_tmp44=_region_malloc(temp,sizeof(*_tmp44));_tmp44->f1=tv,_tmp44->f2=t;_tmp44;});_tmp45->hd=_tmp278;}),_tmp45->tl=inst;_tmp45;});}
# 219
if(inst != 0){
struct _tuple14 env=({struct _tuple14 _tmp23A;_tmp23A.f1=temp,({struct Cyc_List_List*_tmp279=({Cyc_List_imp_rev(inst);});_tmp23A.f2=_tmp279;});_tmp23A;});
struct Cyc_List_List*subs_fs=({({struct Cyc_List_List*(*_tmp27B)(struct _RegionHandle*,struct Cyc_Absyn_Aggrfield*(*f)(struct _tuple14*,struct Cyc_Absyn_Aggrfield*),struct _tuple14*env,struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp47)(struct _RegionHandle*,struct Cyc_Absyn_Aggrfield*(*f)(struct _tuple14*,struct Cyc_Absyn_Aggrfield*),struct _tuple14*env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct _RegionHandle*,struct Cyc_Absyn_Aggrfield*(*f)(struct _tuple14*,struct Cyc_Absyn_Aggrfield*),struct _tuple14*env,struct Cyc_List_List*x))Cyc_List_rmap_c;_tmp47;});struct _RegionHandle*_tmp27A=temp;_tmp27B(_tmp27A,Cyc_CfFlowInfo_substitute_field,& env,fs);});});
struct _fat_ptr _tmp46=({Cyc_CfFlowInfo_aggrfields_to_aggrdict(fenv,subs_fs,no_init_bits_only,leafval);});_npop_handler(0U);return _tmp46;}else{
# 224
struct _fat_ptr _tmp48=({Cyc_CfFlowInfo_aggrfields_to_aggrdict(fenv,fs,no_init_bits_only,leafval);});_npop_handler(0U);return _tmp48;}}
# 208
;_pop_region();}struct _tuple16{struct Cyc_Absyn_Tqual f1;void*f2;};
# 228
static void*Cyc_CfFlowInfo_i_typ_to_absrval(struct Cyc_CfFlowInfo_FlowEnv*fenv,int allow_zeroterm,int no_init_bits_only,void*t,void*leafval){
# 234
if(({Cyc_Absyn_is_nontagged_nonrequire_union_type(t);}))return fenv->unknown_all;{void*_stmttmp8=({Cyc_Tcutil_compress(t);});void*_tmp4A=_stmttmp8;void*_tmp4B;void*_tmp4D;void*_tmp4C;struct Cyc_List_List*_tmp4F;enum Cyc_Absyn_AggrKind _tmp4E;struct Cyc_List_List*_tmp50;void*_tmp51;struct Cyc_List_List*_tmp53;union Cyc_Absyn_AggrInfo _tmp52;struct Cyc_Absyn_Datatypefield*_tmp54;switch(*((int*)_tmp4A)){case 0U: switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp4A)->f1)){case 21U: if(((((struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp4A)->f1)->f1).KnownDatatypefield).tag == 2){_LL1: _tmp54=(((((struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp4A)->f1)->f1).KnownDatatypefield).val).f2;_LL2: {struct Cyc_Absyn_Datatypefield*tuf=_tmp54;
# 238
if(tuf->typs == 0)
return leafval;
# 238
_tmp50=tuf->typs;goto _LL4;}}else{goto _LLF;}case 22U: _LL5: _tmp52=((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp4A)->f1)->f1;_tmp53=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp4A)->f2;_LL6: {union Cyc_Absyn_AggrInfo info=_tmp52;struct Cyc_List_List*targs=_tmp53;
# 250
struct Cyc_Absyn_Aggrdecl*ad=({Cyc_Absyn_get_known_aggrdecl(info);});
if(ad->impl == 0)
goto _LL0;{
# 251
struct Cyc_List_List*fields=((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->fields;
# 254
if(targs == 0){_tmp4E=ad->kind;_tmp4F=fields;goto _LL8;}return(void*)({struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*_tmp58=_cycalloc(sizeof(*_tmp58));
_tmp58->tag=6U,(_tmp58->f1).is_union=(int)ad->kind == (int)Cyc_Absyn_UnionA,(_tmp58->f1).fieldnum=- 1,({
struct _fat_ptr _tmp27C=({Cyc_CfFlowInfo_substitute_aggrfields_to_aggrdict(fenv,ad->tvs,targs,fields,(int)ad->kind == (int)Cyc_Absyn_UnionA,leafval);});_tmp58->f2=_tmp27C;});_tmp58;});}}case 5U: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp4A)->f2 != 0){_LLB: _tmp51=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp4A)->f2)->hd;_LLC: {void*t=_tmp51;
# 273
return leafval;}}else{goto _LLF;}default: goto _LLF;}case 6U: _LL3: _tmp50=((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp4A)->f1;_LL4: {struct Cyc_List_List*tqts=_tmp50;
# 242
unsigned sz=(unsigned)({Cyc_List_length(tqts);});
struct _fat_ptr d=({unsigned _tmp57=sz;void**_tmp56=_cycalloc(_check_times(_tmp57,sizeof(void*)));({{unsigned _tmp23B=sz;unsigned i;for(i=0;i < _tmp23B;++ i){_tmp56[i]=fenv->unknown_all;}}0;});_tag_fat(_tmp56,sizeof(void*),_tmp57);});
{int i=0;for(0;(unsigned)i < sz;++ i){
({void*_tmp27D=({Cyc_CfFlowInfo_i_typ_to_absrval(fenv,0,no_init_bits_only,(*((struct _tuple16*)((struct Cyc_List_List*)_check_null(tqts))->hd)).f2,leafval);});*((void**)_check_fat_subscript(d,sizeof(void*),i))=_tmp27D;});
tqts=tqts->tl;}}
# 248
return(void*)({struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*_tmp55=_cycalloc(sizeof(*_tmp55));_tmp55->tag=6U,(_tmp55->f1).is_union=0,(_tmp55->f1).fieldnum=- 1,_tmp55->f2=d;_tmp55;});}case 7U: _LL7: _tmp4E=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp4A)->f1;_tmp4F=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp4A)->f2;_LL8: {enum Cyc_Absyn_AggrKind k=_tmp4E;struct Cyc_List_List*fs=_tmp4F;
# 265
return(void*)({struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*_tmp59=_cycalloc(sizeof(*_tmp59));_tmp59->tag=6U,(_tmp59->f1).is_union=(int)k == (int)Cyc_Absyn_UnionA,(_tmp59->f1).fieldnum=- 1,({
struct _fat_ptr _tmp27E=({Cyc_CfFlowInfo_aggrfields_to_aggrdict(fenv,fs,(int)k == (int)Cyc_Absyn_UnionA,leafval);});_tmp59->f2=_tmp27E;});_tmp59;});}case 4U: _LL9: _tmp4C=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp4A)->f1).elt_type;_tmp4D=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp4A)->f1).zero_term;_LLA: {void*et=_tmp4C;void*zeroterm=_tmp4D;
# 268
if(({Cyc_Tcutil_force_type2bool(0,zeroterm);}))
# 271
return(allow_zeroterm && !no_init_bits_only)&&({Cyc_Tcutil_is_bits_only_type(et);})?fenv->unknown_all: leafval;
# 268
goto _LL0;}case 3U: _LLD: _tmp4B=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp4A)->f1).ptr_atts).nullable;_LLE: {void*nbl=_tmp4B;
# 275
if(!({Cyc_Tcutil_force_type2bool(0,nbl);})){
void*_tmp5A=leafval;if(((struct Cyc_CfFlowInfo_UnknownR_CfFlowInfo_AbsRVal_struct*)_tmp5A)->tag == 2U){if(((struct Cyc_CfFlowInfo_UnknownR_CfFlowInfo_AbsRVal_struct*)_tmp5A)->f1 == Cyc_CfFlowInfo_AllIL){_LL12: _LL13:
 return fenv->notzeroall;}else{goto _LL14;}}else{_LL14: _LL15:
 goto _LL11;}_LL11:;}
# 275
goto _LL0;}default: _LLF: _LL10:
# 281
 goto _LL0;}_LL0:;}
# 284
return !no_init_bits_only &&({Cyc_Tcutil_is_bits_only_type(t);})?fenv->unknown_all: leafval;}
# 228
void*Cyc_CfFlowInfo_typ_to_absrval(struct Cyc_CfFlowInfo_FlowEnv*fenv,void*t,int no_init_bits_only,void*leafval){
# 288
return({Cyc_CfFlowInfo_i_typ_to_absrval(fenv,1,no_init_bits_only,t,leafval);});}
# 228
int Cyc_CfFlowInfo_is_unique_consumed(struct Cyc_Absyn_Exp*e,int env_iteration,void*r,int*needs_unconsume){
# 294
void*_tmp5D=r;void*_tmp5E;struct _fat_ptr _tmp61;int _tmp60;int _tmp5F;void*_tmp64;int _tmp63;struct Cyc_Absyn_Exp*_tmp62;switch(*((int*)_tmp5D)){case 7U: _LL1: _tmp62=((struct Cyc_CfFlowInfo_Consumed_CfFlowInfo_AbsRVal_struct*)_tmp5D)->f1;_tmp63=((struct Cyc_CfFlowInfo_Consumed_CfFlowInfo_AbsRVal_struct*)_tmp5D)->f2;_tmp64=(void*)((struct Cyc_CfFlowInfo_Consumed_CfFlowInfo_AbsRVal_struct*)_tmp5D)->f3;_LL2: {struct Cyc_Absyn_Exp*consumer=_tmp62;int iteration=_tmp63;void*r=_tmp64;
# 296
if(consumer == e && iteration == env_iteration){
*needs_unconsume=1;
return 0;}
# 296
return 1;}case 6U: _LL3: _tmp5F=(((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp5D)->f1).is_union;_tmp60=(((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp5D)->f1).fieldnum;_tmp61=((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp5D)->f2;_LL4: {int is_union=_tmp5F;int field_no=_tmp60;struct _fat_ptr d=_tmp61;
# 302
if(!is_union || field_no == -1){
unsigned sz=_get_fat_size(d,sizeof(void*));
{int i=0;for(0;(unsigned)i < sz;++ i){
if(({Cyc_CfFlowInfo_is_unique_consumed(e,env_iteration,((void**)d.curr)[i],needs_unconsume);}))
return 1;}}
# 308
return 0;}else{
# 311
return({Cyc_CfFlowInfo_is_unique_consumed(e,env_iteration,*((void**)_check_fat_subscript(d,sizeof(void*),field_no)),needs_unconsume);});}}case 8U: _LL5: _tmp5E=(void*)((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp5D)->f2;_LL6: {void*r=_tmp5E;
# 313
return({Cyc_CfFlowInfo_is_unique_consumed(e,env_iteration,r,needs_unconsume);});}default: _LL7: _LL8:
 return 0;}_LL0:;}
# 228
void*Cyc_CfFlowInfo_make_unique_unconsumed(struct Cyc_CfFlowInfo_FlowEnv*fenv,void*r){
# 321
void*_tmp66=r;void*_tmp68;struct Cyc_Absyn_Vardecl*_tmp67;struct _fat_ptr _tmp6A;struct Cyc_CfFlowInfo_UnionRInfo _tmp69;void*_tmp6D;int _tmp6C;struct Cyc_Absyn_Exp*_tmp6B;switch(*((int*)_tmp66)){case 7U: _LL1: _tmp6B=((struct Cyc_CfFlowInfo_Consumed_CfFlowInfo_AbsRVal_struct*)_tmp66)->f1;_tmp6C=((struct Cyc_CfFlowInfo_Consumed_CfFlowInfo_AbsRVal_struct*)_tmp66)->f2;_tmp6D=(void*)((struct Cyc_CfFlowInfo_Consumed_CfFlowInfo_AbsRVal_struct*)_tmp66)->f3;_LL2: {struct Cyc_Absyn_Exp*consumer=_tmp6B;int iteration=_tmp6C;void*r=_tmp6D;
# 323
return r;}case 6U: _LL3: _tmp69=((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp66)->f1;_tmp6A=((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp66)->f2;_LL4: {struct Cyc_CfFlowInfo_UnionRInfo uinfo=_tmp69;struct _fat_ptr d=_tmp6A;
# 325
unsigned sz=_get_fat_size(d,sizeof(void*));
int change=0;
struct _fat_ptr d2=({unsigned _tmp70=sz;void**_tmp6F=_cycalloc(_check_times(_tmp70,sizeof(void*)));({{unsigned _tmp23C=sz;unsigned i;for(i=0;i < _tmp23C;++ i){_tmp6F[i]=((void**)d.curr)[(int)i];}}0;});_tag_fat(_tmp6F,sizeof(void*),_tmp70);});
{int i=0;for(0;(unsigned)i < sz;++ i){
({void*_tmp27F=({Cyc_CfFlowInfo_make_unique_unconsumed(fenv,((void**)d.curr)[i]);});*((void**)_check_fat_subscript(d2,sizeof(void*),i))=_tmp27F;});
if(((void**)d2.curr)[i]!= ((void**)d.curr)[i])
change=1;}}
# 333
if(change)
return(void*)({struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*_tmp6E=_cycalloc(sizeof(*_tmp6E));_tmp6E->tag=6U,_tmp6E->f1=uinfo,_tmp6E->f2=d2;_tmp6E;});else{
return r;}}case 8U: _LL5: _tmp67=((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp66)->f1;_tmp68=(void*)((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp66)->f2;_LL6: {struct Cyc_Absyn_Vardecl*n=_tmp67;void*r2=_tmp68;
# 337
void*r3=({Cyc_CfFlowInfo_make_unique_unconsumed(fenv,r2);});
if(r3 != r2)
return(void*)({struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*_tmp71=_cycalloc(sizeof(*_tmp71));_tmp71->tag=8U,_tmp71->f1=n,_tmp71->f2=r3;_tmp71;});else{
return r;}}default: _LL7: _LL8:
 return r;}_LL0:;}
# 228
void*Cyc_CfFlowInfo_make_unique_consumed(struct Cyc_CfFlowInfo_FlowEnv*fenv,void*t,struct Cyc_Absyn_Exp*consumer,int iteration,void*r){
# 348
struct _tuple13 _stmttmp9=({struct _tuple13 _tmp23F;({void*_tmp280=({Cyc_Tcutil_compress(t);});_tmp23F.f1=_tmp280;}),_tmp23F.f2=r;_tmp23F;});struct _tuple13 _tmp73=_stmttmp9;struct _fat_ptr _tmp77;struct Cyc_CfFlowInfo_UnionRInfo _tmp76;struct Cyc_List_List*_tmp75;enum Cyc_Absyn_AggrKind _tmp74;struct _fat_ptr _tmp7A;struct Cyc_CfFlowInfo_UnionRInfo _tmp79;union Cyc_Absyn_AggrInfo _tmp78;struct _fat_ptr _tmp7D;struct Cyc_CfFlowInfo_UnionRInfo _tmp7C;struct Cyc_List_List*_tmp7B;void*_tmp7F;struct Cyc_Absyn_Vardecl*_tmp7E;if(((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp73.f2)->tag == 8U){_LL1: _tmp7E=((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp73.f2)->f1;_tmp7F=(void*)((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp73.f2)->f2;_LL2: {struct Cyc_Absyn_Vardecl*s=_tmp7E;void*r2=_tmp7F;
# 350
void*r3=({Cyc_CfFlowInfo_make_unique_consumed(fenv,t,consumer,iteration,r2);});
if(r3 != r2)return(void*)({struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*_tmp80=_cycalloc(sizeof(*_tmp80));_tmp80->tag=8U,_tmp80->f1=s,_tmp80->f2=r3;_tmp80;});else{
return r;}}}else{switch(*((int*)_tmp73.f1)){case 6U: if(((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp73.f2)->tag == 6U){_LL3: _tmp7B=((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp73.f1)->f1;_tmp7C=((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp73.f2)->f1;_tmp7D=((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp73.f2)->f2;_LL4: {struct Cyc_List_List*tqts=_tmp7B;struct Cyc_CfFlowInfo_UnionRInfo b=_tmp7C;struct _fat_ptr d=_tmp7D;
# 354
unsigned sz=_get_fat_size(d,sizeof(void*));
struct _fat_ptr d2=({unsigned _tmp83=sz;void**_tmp82=_cycalloc(_check_times(_tmp83,sizeof(void*)));({{unsigned _tmp23D=sz;unsigned i;for(i=0;i < _tmp23D;++ i){_tmp82[i]=fenv->unknown_all;}}0;});_tag_fat(_tmp82,sizeof(void*),_tmp83);});
{int i=0;for(0;(unsigned)i < sz;++ i){
({void*_tmp281=({Cyc_CfFlowInfo_make_unique_consumed(fenv,(*((struct _tuple16*)((struct Cyc_List_List*)_check_null(tqts))->hd)).f2,consumer,iteration,((void**)d.curr)[i]);});*((void**)_check_fat_subscript(d2,sizeof(void*),i))=_tmp281;});
tqts=tqts->tl;}}
# 360
return(void*)({struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*_tmp81=_cycalloc(sizeof(*_tmp81));_tmp81->tag=6U,_tmp81->f1=b,_tmp81->f2=d2;_tmp81;});}}else{goto _LL9;}case 0U: if(((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp73.f1)->f1)->tag == 22U){if(((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp73.f2)->tag == 6U){_LL5: _tmp78=((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp73.f1)->f1)->f1;_tmp79=((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp73.f2)->f1;_tmp7A=((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp73.f2)->f2;_LL6: {union Cyc_Absyn_AggrInfo info=_tmp78;struct Cyc_CfFlowInfo_UnionRInfo b=_tmp79;struct _fat_ptr d=_tmp7A;
# 362
struct Cyc_Absyn_Aggrdecl*ad=({Cyc_Absyn_get_known_aggrdecl(info);});
if(ad->impl == 0)return r;_tmp74=ad->kind;_tmp75=((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->fields;_tmp76=b;_tmp77=d;goto _LL8;}}else{goto _LL9;}}else{goto _LL9;}case 7U: if(((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp73.f2)->tag == 6U){_LL7: _tmp74=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp73.f1)->f1;_tmp75=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp73.f1)->f2;_tmp76=((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp73.f2)->f1;_tmp77=((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp73.f2)->f2;_LL8: {enum Cyc_Absyn_AggrKind k=_tmp74;struct Cyc_List_List*fs=_tmp75;struct Cyc_CfFlowInfo_UnionRInfo b=_tmp76;struct _fat_ptr d=_tmp77;
# 366
struct _fat_ptr d2=({unsigned _tmp88=_get_fat_size(d,sizeof(void*));void**_tmp87=_cycalloc(_check_times(_tmp88,sizeof(void*)));({{unsigned _tmp23E=_get_fat_size(d,sizeof(void*));unsigned i;for(i=0;i < _tmp23E;++ i){_tmp87[i]=((void**)d.curr)[(int)i];}}0;});_tag_fat(_tmp87,sizeof(void*),_tmp88);});
unsigned sz=(unsigned)({Cyc_List_length(fs);});
{int i=0;for(0;(unsigned)i < sz;(i ++,fs=fs->tl)){
struct Cyc_Absyn_Aggrfield*_stmttmpA=(struct Cyc_Absyn_Aggrfield*)((struct Cyc_List_List*)_check_null(fs))->hd;void*_tmp85;struct _fat_ptr*_tmp84;_LLC: _tmp84=_stmttmpA->name;_tmp85=_stmttmpA->type;_LLD: {struct _fat_ptr*n=_tmp84;void*t2=_tmp85;
if(_get_fat_size(*n,sizeof(char))!= (unsigned)1)
({void*_tmp282=({Cyc_CfFlowInfo_make_unique_consumed(fenv,t2,consumer,iteration,*((void**)_check_fat_subscript(d,sizeof(void*),i)));});*((void**)_check_fat_subscript(d2,sizeof(void*),i))=_tmp282;});}}}
# 373
return(void*)({struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*_tmp86=_cycalloc(sizeof(*_tmp86));_tmp86->tag=6U,_tmp86->f1=b,_tmp86->f2=d2;_tmp86;});}}else{goto _LL9;}default: _LL9: _LLA:
# 375
 if(({Cyc_Tcutil_is_noalias_pointer(t,0);}))
return(void*)({struct Cyc_CfFlowInfo_Consumed_CfFlowInfo_AbsRVal_struct*_tmp89=_cycalloc(sizeof(*_tmp89));_tmp89->tag=7U,_tmp89->f1=consumer,_tmp89->f2=iteration,_tmp89->f3=r;_tmp89;});else{
return r;}}}_LL0:;}struct _tuple17{struct Cyc_CfFlowInfo_Place*f1;unsigned f2;};
# 381
static int Cyc_CfFlowInfo_prefix_of_member(struct Cyc_CfFlowInfo_Place*place,struct Cyc_Dict_Dict set){
# 383
struct _RegionHandle _tmp8B=_new_region("r");struct _RegionHandle*r=& _tmp8B;_push_region(r);
{struct _tuple17 elem=({struct _tuple17 _tmp240;_tmp240.f1=place,_tmp240.f2=0U;_tmp240;});
struct Cyc_Iter_Iter iter=({Cyc_Dict_make_iter(r,set);});
while(({({int(*_tmp283)(struct Cyc_Iter_Iter,struct _tuple17*)=({int(*_tmp8C)(struct Cyc_Iter_Iter,struct _tuple17*)=(int(*)(struct Cyc_Iter_Iter,struct _tuple17*))Cyc_Iter_next;_tmp8C;});_tmp283(iter,& elem);});})){
struct Cyc_CfFlowInfo_Place*place2=elem.f1;
struct Cyc_CfFlowInfo_Place _stmttmpB=*place;struct Cyc_List_List*_tmp8E;void*_tmp8D;_LL1: _tmp8D=_stmttmpB.root;_tmp8E=_stmttmpB.path;_LL2: {void*root1=_tmp8D;struct Cyc_List_List*fs1=_tmp8E;
struct Cyc_CfFlowInfo_Place _stmttmpC=*place2;struct Cyc_List_List*_tmp90;void*_tmp8F;_LL4: _tmp8F=_stmttmpC.root;_tmp90=_stmttmpC.path;_LL5: {void*root2=_tmp8F;struct Cyc_List_List*fs2=_tmp90;
if(({Cyc_CfFlowInfo_root_cmp(root1,root2);})!= 0)
continue;
# 390
for(0;
# 392
fs1 != 0 && fs2 != 0;(fs1=fs1->tl,fs2=fs2->tl)){
if((void*)fs1->hd != (void*)fs2->hd)break;}
# 390
if(fs1 == 0){
# 395
int _tmp91=1;_npop_handler(0U);return _tmp91;}}}}{
# 397
int _tmp92=0;_npop_handler(0U);return _tmp92;}}
# 384
;_pop_region();}struct Cyc_CfFlowInfo_EscPile{struct Cyc_List_List*places;};
# 407
static void Cyc_CfFlowInfo_add_place(struct Cyc_CfFlowInfo_EscPile*pile,struct Cyc_CfFlowInfo_Place*place){
# 410
if(!({({int(*_tmp285)(int(*compare)(struct Cyc_CfFlowInfo_Place*,struct Cyc_CfFlowInfo_Place*),struct Cyc_List_List*l,struct Cyc_CfFlowInfo_Place*x)=({int(*_tmp94)(int(*compare)(struct Cyc_CfFlowInfo_Place*,struct Cyc_CfFlowInfo_Place*),struct Cyc_List_List*l,struct Cyc_CfFlowInfo_Place*x)=(int(*)(int(*compare)(struct Cyc_CfFlowInfo_Place*,struct Cyc_CfFlowInfo_Place*),struct Cyc_List_List*l,struct Cyc_CfFlowInfo_Place*x))Cyc_List_mem;_tmp94;});struct Cyc_List_List*_tmp284=pile->places;_tmp285(Cyc_CfFlowInfo_place_cmp,_tmp284,place);});}))
({struct Cyc_List_List*_tmp286=({struct Cyc_List_List*_tmp95=_cycalloc(sizeof(*_tmp95));_tmp95->hd=place,_tmp95->tl=pile->places;_tmp95;});pile->places=_tmp286;});}
# 413
static void Cyc_CfFlowInfo_add_places(struct Cyc_CfFlowInfo_EscPile*pile,void*r){
void*_tmp97=r;struct _fat_ptr _tmp99;struct Cyc_CfFlowInfo_UnionRInfo _tmp98;struct Cyc_CfFlowInfo_Place*_tmp9A;void*_tmp9B;void*_tmp9C;switch(*((int*)_tmp97)){case 7U: _LL1: _tmp9C=(void*)((struct Cyc_CfFlowInfo_Consumed_CfFlowInfo_AbsRVal_struct*)_tmp97)->f3;_LL2: {void*r=_tmp9C;
({Cyc_CfFlowInfo_add_places(pile,r);});return;}case 8U: _LL3: _tmp9B=(void*)((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp97)->f2;_LL4: {void*r=_tmp9B;
({Cyc_CfFlowInfo_add_places(pile,r);});return;}case 4U: _LL5: _tmp9A=((struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct*)_tmp97)->f1;_LL6: {struct Cyc_CfFlowInfo_Place*p=_tmp9A;
({Cyc_CfFlowInfo_add_place(pile,p);});return;}case 6U: _LL7: _tmp98=((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp97)->f1;_tmp99=((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp97)->f2;_LL8: {struct Cyc_CfFlowInfo_UnionRInfo b=_tmp98;struct _fat_ptr d=_tmp99;
# 419
{int i=0;for(0;(unsigned)i < _get_fat_size(d,sizeof(void*));++ i){
({Cyc_CfFlowInfo_add_places(pile,((void**)d.curr)[i]);});}}
return;}default: _LL9: _LLA:
 return;}_LL0:;}
# 429
static void*Cyc_CfFlowInfo_insert_place_inner(void*new_val,void*old_val){
void*_tmp9E=old_val;void*_tmpA0;struct Cyc_Absyn_Vardecl*_tmp9F;void*_tmpA3;int _tmpA2;struct Cyc_Absyn_Exp*_tmpA1;struct _fat_ptr _tmpA5;int _tmpA4;switch(*((int*)_tmp9E)){case 6U: _LL1: _tmpA4=(((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp9E)->f1).is_union;_tmpA5=((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp9E)->f2;_LL2: {int is_union=_tmpA4;struct _fat_ptr d=_tmpA5;
# 432
struct _fat_ptr d2=({unsigned _tmpA8=_get_fat_size(d,sizeof(void*));void**_tmpA7=_cycalloc(_check_times(_tmpA8,sizeof(void*)));({{unsigned _tmp241=_get_fat_size(d,sizeof(void*));unsigned i;for(i=0;i < _tmp241;++ i){({
void*_tmp287=({Cyc_CfFlowInfo_insert_place_inner(new_val,((void**)d.curr)[(int)i]);});_tmpA7[i]=_tmp287;});}}0;});_tag_fat(_tmpA7,sizeof(void*),_tmpA8);});
# 436
return(void*)({struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*_tmpA6=_cycalloc(sizeof(*_tmpA6));_tmpA6->tag=6U,(_tmpA6->f1).is_union=is_union,(_tmpA6->f1).fieldnum=- 1,_tmpA6->f2=d2;_tmpA6;});}case 7U: _LL3: _tmpA1=((struct Cyc_CfFlowInfo_Consumed_CfFlowInfo_AbsRVal_struct*)_tmp9E)->f1;_tmpA2=((struct Cyc_CfFlowInfo_Consumed_CfFlowInfo_AbsRVal_struct*)_tmp9E)->f2;_tmpA3=(void*)((struct Cyc_CfFlowInfo_Consumed_CfFlowInfo_AbsRVal_struct*)_tmp9E)->f3;_LL4: {struct Cyc_Absyn_Exp*e=_tmpA1;int i=_tmpA2;void*rval=_tmpA3;
# 438
return(void*)({struct Cyc_CfFlowInfo_Consumed_CfFlowInfo_AbsRVal_struct*_tmpA9=_cycalloc(sizeof(*_tmpA9));_tmpA9->tag=7U,_tmpA9->f1=e,_tmpA9->f2=i,({void*_tmp288=({Cyc_CfFlowInfo_insert_place_inner(new_val,rval);});_tmpA9->f3=_tmp288;});_tmpA9;});}case 8U: _LL5: _tmp9F=((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp9E)->f1;_tmpA0=(void*)((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp9E)->f2;_LL6: {struct Cyc_Absyn_Vardecl*n=_tmp9F;void*rval=_tmpA0;
# 440
return(void*)({struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*_tmpAA=_cycalloc(sizeof(*_tmpAA));_tmpAA->tag=8U,_tmpAA->f1=n,({void*_tmp289=({Cyc_CfFlowInfo_insert_place_inner(new_val,rval);});_tmpAA->f2=_tmp289;});_tmpAA;});}default: _LL7: _LL8:
 return new_val;}_LL0:;}
# 447
static struct _fat_ptr Cyc_CfFlowInfo_aggr_dict_insert(struct _fat_ptr d,int n,void*rval){
void*old_rval=*((void**)_check_fat_subscript(d,sizeof(void*),n));
if(old_rval == rval)return d;{struct _fat_ptr res=({unsigned _tmpAD=
_get_fat_size(d,sizeof(void*));void**_tmpAC=_cycalloc(_check_times(_tmpAD,sizeof(void*)));({{unsigned _tmp242=_get_fat_size(d,sizeof(void*));unsigned i;for(i=0;i < _tmp242;++ i){_tmpAC[i]=((void**)d.curr)[(int)i];}}0;});_tag_fat(_tmpAC,sizeof(void*),_tmpAD);});
*((void**)_check_fat_subscript(res,sizeof(void*),n))=rval;
return res;}}struct _tuple18{struct Cyc_List_List*f1;void*f2;};
# 460
static void*Cyc_CfFlowInfo_insert_place_outer(struct Cyc_List_List*path,void*old_val,void*new_val){
# 462
if(path == 0)
return({Cyc_CfFlowInfo_insert_place_inner(new_val,old_val);});{
# 462
struct _tuple18 _stmttmpD=({struct _tuple18 _tmp243;_tmp243.f1=path,_tmp243.f2=old_val;_tmp243;});struct _tuple18 _tmpAF=_stmttmpD;void*_tmpB1;struct Cyc_Absyn_Vardecl*_tmpB0;void*_tmpB3;struct Cyc_List_List*_tmpB2;struct _fat_ptr _tmpB7;int _tmpB6;struct Cyc_List_List*_tmpB5;int _tmpB4;if(_tmpAF.f1 != 0){if(((struct Cyc_CfFlowInfo_Dot_CfFlowInfo_PathCon_struct*)((struct Cyc_List_List*)_tmpAF.f1)->hd)->tag == 0U)switch(*((int*)_tmpAF.f2)){case 6U: _LL1: _tmpB4=((struct Cyc_CfFlowInfo_Dot_CfFlowInfo_PathCon_struct*)(_tmpAF.f1)->hd)->f1;_tmpB5=(_tmpAF.f1)->tl;_tmpB6=(((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmpAF.f2)->f1).is_union;_tmpB7=((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmpAF.f2)->f2;_LL2: {int i=_tmpB4;struct Cyc_List_List*tl=_tmpB5;int is_union=_tmpB6;struct _fat_ptr d=_tmpB7;
# 466
void*new_child=({Cyc_CfFlowInfo_insert_place_outer(tl,*((void**)_check_fat_subscript(d,sizeof(void*),i)),new_val);});
struct _fat_ptr new_d=({Cyc_CfFlowInfo_aggr_dict_insert(d,i,new_child);});
if((void**)new_d.curr == (void**)d.curr)return old_val;else{
return(void*)({struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*_tmpB8=_cycalloc(sizeof(*_tmpB8));_tmpB8->tag=6U,(_tmpB8->f1).is_union=is_union,(_tmpB8->f1).fieldnum=- 1,_tmpB8->f2=new_d;_tmpB8;});}}case 8U: goto _LL5;default: goto _LL7;}else{switch(*((int*)_tmpAF.f2)){case 5U: _LL3: _tmpB2=(_tmpAF.f1)->tl;_tmpB3=(void*)((struct Cyc_CfFlowInfo_UniquePtr_CfFlowInfo_AbsRVal_struct*)_tmpAF.f2)->f1;_LL4: {struct Cyc_List_List*tl=_tmpB2;void*rval=_tmpB3;
# 471
void*new_rval=({Cyc_CfFlowInfo_insert_place_outer(tl,rval,new_val);});
if(new_rval == rval)return old_val;else{
return(void*)({struct Cyc_CfFlowInfo_UniquePtr_CfFlowInfo_AbsRVal_struct*_tmpB9=_cycalloc(sizeof(*_tmpB9));_tmpB9->tag=5U,_tmpB9->f1=new_rval;_tmpB9;});}}case 8U: goto _LL5;default: goto _LL7;}}}else{if(((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmpAF.f2)->tag == 8U){_LL5: _tmpB0=((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmpAF.f2)->f1;_tmpB1=(void*)((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmpAF.f2)->f2;_LL6: {struct Cyc_Absyn_Vardecl*n=_tmpB0;void*rval=_tmpB1;
# 475
void*new_rval=({Cyc_CfFlowInfo_insert_place_outer(path,rval,new_val);});
if(new_rval == rval)return old_val;else{
return(void*)({struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*_tmpBA=_cycalloc(sizeof(*_tmpBA));_tmpBA->tag=8U,_tmpBA->f1=n,_tmpBA->f2=new_rval;_tmpBA;});}}}else{_LL7: _LL8:
({void*_tmpBB=0U;({int(*_tmp28B)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmpBD)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmpBD;});struct _fat_ptr _tmp28A=({const char*_tmpBC="bad insert place";_tag_fat(_tmpBC,sizeof(char),17U);});_tmp28B(_tmp28A,_tag_fat(_tmpBB,sizeof(void*),0U));});});}}_LL0:;}}
# 485
static struct Cyc_Dict_Dict Cyc_CfFlowInfo_escape_these(struct Cyc_CfFlowInfo_FlowEnv*fenv,struct Cyc_CfFlowInfo_EscPile*pile,struct Cyc_Dict_Dict d){
while(pile->places != 0){
struct Cyc_CfFlowInfo_Place*place=(struct Cyc_CfFlowInfo_Place*)((struct Cyc_List_List*)_check_null(pile->places))->hd;
pile->places=((struct Cyc_List_List*)_check_null(pile->places))->tl;{
void*oldval;void*newval;
{struct _handler_cons _tmpBF;_push_handler(& _tmpBF);{int _tmpC1=0;if(setjmp(_tmpBF.handler))_tmpC1=1;if(!_tmpC1){oldval=({Cyc_CfFlowInfo_lookup_place(d,place);});;_pop_handler();}else{void*_tmpC0=(void*)Cyc_Core_get_exn_thrown();void*_tmpC2=_tmpC0;void*_tmpC3;if(((struct Cyc_Dict_Absent_exn_struct*)_tmpC2)->tag == Cyc_Dict_Absent){_LL1: _LL2:
 continue;}else{_LL3: _tmpC3=_tmpC2;_LL4: {void*exn=_tmpC3;(int)_rethrow(exn);}}_LL0:;}}}
{enum Cyc_CfFlowInfo_InitLevel _stmttmpE=({Cyc_CfFlowInfo_initlevel(fenv,d,oldval);});enum Cyc_CfFlowInfo_InitLevel _tmpC4=_stmttmpE;if(_tmpC4 == Cyc_CfFlowInfo_AllIL){_LL6: _LL7:
 newval=fenv->esc_all;goto _LL5;}else{_LL8: _LL9:
 newval=fenv->esc_none;goto _LL5;}_LL5:;}
# 496
({Cyc_CfFlowInfo_add_places(pile,oldval);});{
struct Cyc_CfFlowInfo_Place _stmttmpF=*place;struct Cyc_List_List*_tmpC6;void*_tmpC5;_LLB: _tmpC5=_stmttmpF.root;_tmpC6=_stmttmpF.path;_LLC: {void*root=_tmpC5;struct Cyc_List_List*path=_tmpC6;
d=({struct Cyc_Dict_Dict(*_tmpC7)(struct Cyc_Dict_Dict d,void*k,void*v)=Cyc_Dict_insert;struct Cyc_Dict_Dict _tmpC8=d;void*_tmpC9=root;void*_tmpCA=({void*(*_tmpCB)(struct Cyc_List_List*path,void*old_val,void*new_val)=Cyc_CfFlowInfo_insert_place_outer;struct Cyc_List_List*_tmpCC=path;void*_tmpCD=({Cyc_Dict_lookup(d,root);});void*_tmpCE=newval;_tmpCB(_tmpCC,_tmpCD,_tmpCE);});_tmpC7(_tmpC8,_tmpC9,_tmpCA);});}}}}
# 503
return d;}struct Cyc_CfFlowInfo_InitlevelEnv{struct Cyc_Dict_Dict d;struct Cyc_List_List*seen;};
# 513
static enum Cyc_CfFlowInfo_InitLevel Cyc_CfFlowInfo_initlevel_approx(void*r){
void*_tmpD0=r;void*_tmpD1;enum Cyc_CfFlowInfo_InitLevel _tmpD2;enum Cyc_CfFlowInfo_InitLevel _tmpD3;switch(*((int*)_tmpD0)){case 2U: _LL1: _tmpD3=((struct Cyc_CfFlowInfo_UnknownR_CfFlowInfo_AbsRVal_struct*)_tmpD0)->f1;_LL2: {enum Cyc_CfFlowInfo_InitLevel il=_tmpD3;
return il;}case 3U: _LL3: _tmpD2=((struct Cyc_CfFlowInfo_Esc_CfFlowInfo_AbsRVal_struct*)_tmpD0)->f1;_LL4: {enum Cyc_CfFlowInfo_InitLevel il=_tmpD2;
return il;}case 0U: _LL5: _LL6:
 goto _LL8;case 1U: _LL7: _LL8:
 return Cyc_CfFlowInfo_AllIL;case 7U: _LL9: _tmpD1=(void*)((struct Cyc_CfFlowInfo_Consumed_CfFlowInfo_AbsRVal_struct*)_tmpD0)->f3;_LLA: {void*r=_tmpD1;
return Cyc_CfFlowInfo_NoneIL;}default: _LLB: _LLC:
({void*_tmpD4=0U;({int(*_tmp28D)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmpD6)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmpD6;});struct _fat_ptr _tmp28C=({const char*_tmpD5="initlevel_approx";_tag_fat(_tmpD5,sizeof(char),17U);});_tmp28D(_tmp28C,_tag_fat(_tmpD4,sizeof(void*),0U));});});}_LL0:;}
# 523
static enum Cyc_CfFlowInfo_InitLevel Cyc_CfFlowInfo_initlevel_rec(struct Cyc_CfFlowInfo_InitlevelEnv*env,void*r,enum Cyc_CfFlowInfo_InitLevel acc){
# 525
enum Cyc_CfFlowInfo_InitLevel this_ans;
if((int)acc == (int)0U)return Cyc_CfFlowInfo_NoneIL;{void*_tmpD8=r;void*_tmpD9;struct Cyc_CfFlowInfo_Place*_tmpDA;struct _fat_ptr _tmpDB;struct _fat_ptr _tmpDE;int _tmpDD;int _tmpDC;void*_tmpDF;switch(*((int*)_tmpD8)){case 8U: _LL1: _tmpDF=(void*)((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmpD8)->f2;_LL2: {void*r=_tmpDF;
# 528
return({Cyc_CfFlowInfo_initlevel_rec(env,r,acc);});}case 6U: _LL3: _tmpDC=(((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmpD8)->f1).is_union;_tmpDD=(((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmpD8)->f1).fieldnum;_tmpDE=((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmpD8)->f2;if(_tmpDC){_LL4: {int iu=_tmpDC;int f=_tmpDD;struct _fat_ptr d=_tmpDE;
# 533
unsigned sz=_get_fat_size(d,sizeof(void*));
this_ans=0U;
if(f == -1){
int i=0;for(0;(unsigned)i < sz;++ i){
if((int)({Cyc_CfFlowInfo_initlevel_rec(env,((void**)d.curr)[i],Cyc_CfFlowInfo_AllIL);})== (int)Cyc_CfFlowInfo_AllIL){
this_ans=1U;
break;}}}else{
# 543
if((int)({Cyc_CfFlowInfo_initlevel_rec(env,*((void**)_check_fat_subscript(d,sizeof(void*),f)),Cyc_CfFlowInfo_AllIL);})== (int)Cyc_CfFlowInfo_AllIL)
this_ans=1U;}
# 535
goto _LL0;}}else{_LL5: _tmpDB=((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmpD8)->f2;_LL6: {struct _fat_ptr d=_tmpDB;
# 547
unsigned sz=_get_fat_size(d,sizeof(void*));
this_ans=1U;
{int i=0;for(0;(unsigned)i < sz;++ i){
this_ans=({Cyc_CfFlowInfo_initlevel_rec(env,((void**)d.curr)[i],this_ans);});}}
goto _LL0;}}case 4U: _LL7: _tmpDA=((struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct*)_tmpD8)->f1;_LL8: {struct Cyc_CfFlowInfo_Place*p=_tmpDA;
# 553
if(({({int(*_tmp28F)(int(*compare)(struct Cyc_CfFlowInfo_Place*,struct Cyc_CfFlowInfo_Place*),struct Cyc_List_List*l,struct Cyc_CfFlowInfo_Place*x)=({int(*_tmpE0)(int(*compare)(struct Cyc_CfFlowInfo_Place*,struct Cyc_CfFlowInfo_Place*),struct Cyc_List_List*l,struct Cyc_CfFlowInfo_Place*x)=(int(*)(int(*compare)(struct Cyc_CfFlowInfo_Place*,struct Cyc_CfFlowInfo_Place*),struct Cyc_List_List*l,struct Cyc_CfFlowInfo_Place*x))Cyc_List_mem;_tmpE0;});struct Cyc_List_List*_tmp28E=env->seen;_tmp28F(Cyc_CfFlowInfo_place_cmp,_tmp28E,p);});}))
this_ans=1U;else{
# 556
({struct Cyc_List_List*_tmp290=({struct Cyc_List_List*_tmpE1=_cycalloc(sizeof(*_tmpE1));_tmpE1->hd=p,_tmpE1->tl=env->seen;_tmpE1;});env->seen=_tmp290;});
this_ans=({enum Cyc_CfFlowInfo_InitLevel(*_tmpE2)(struct Cyc_CfFlowInfo_InitlevelEnv*env,void*r,enum Cyc_CfFlowInfo_InitLevel acc)=Cyc_CfFlowInfo_initlevel_rec;struct Cyc_CfFlowInfo_InitlevelEnv*_tmpE3=env;void*_tmpE4=({Cyc_CfFlowInfo_lookup_place(env->d,p);});enum Cyc_CfFlowInfo_InitLevel _tmpE5=Cyc_CfFlowInfo_AllIL;_tmpE2(_tmpE3,_tmpE4,_tmpE5);});
env->seen=((struct Cyc_List_List*)_check_null(env->seen))->tl;}
# 560
goto _LL0;}case 5U: _LL9: _tmpD9=(void*)((struct Cyc_CfFlowInfo_UniquePtr_CfFlowInfo_AbsRVal_struct*)_tmpD8)->f1;_LLA: {void*r=_tmpD9;
this_ans=({Cyc_CfFlowInfo_initlevel_rec(env,r,Cyc_CfFlowInfo_AllIL);});goto _LL0;}default: _LLB: _LLC:
 this_ans=({Cyc_CfFlowInfo_initlevel_approx(r);});goto _LL0;}_LL0:;}
# 564
return this_ans;}
# 523
enum Cyc_CfFlowInfo_InitLevel Cyc_CfFlowInfo_initlevel(struct Cyc_CfFlowInfo_FlowEnv*env0,struct Cyc_Dict_Dict d,void*r){
# 567
struct Cyc_CfFlowInfo_InitlevelEnv env=({struct Cyc_CfFlowInfo_InitlevelEnv _tmp244;_tmp244.d=d,_tmp244.seen=0;_tmp244;});
return({Cyc_CfFlowInfo_initlevel_rec(& env,r,Cyc_CfFlowInfo_AllIL);});}
# 523
void*Cyc_CfFlowInfo_lookup_place(struct Cyc_Dict_Dict d,struct Cyc_CfFlowInfo_Place*place){
# 572
struct Cyc_CfFlowInfo_Place _stmttmp10=*place;struct Cyc_List_List*_tmpE9;void*_tmpE8;_LL1: _tmpE8=_stmttmp10.root;_tmpE9=_stmttmp10.path;_LL2: {void*root=_tmpE8;struct Cyc_List_List*path=_tmpE9;
void*ans=({Cyc_Dict_lookup(d,root);});
for(0;path != 0;path=path->tl){
retry: {struct _tuple13 _stmttmp11=({struct _tuple13 _tmp245;_tmp245.f1=ans,_tmp245.f2=(void*)path->hd;_tmp245;});struct _tuple13 _tmpEA=_stmttmp11;void*_tmpEB;int _tmpEE;struct _fat_ptr _tmpED;struct Cyc_CfFlowInfo_UnionRInfo _tmpEC;void*_tmpEF;void*_tmpF0;switch(*((int*)_tmpEA.f1)){case 8U: _LL4: _tmpF0=(void*)((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmpEA.f1)->f2;_LL5: {void*rval=_tmpF0;
# 577
ans=rval;goto retry;}case 7U: _LL6: _tmpEF=(void*)((struct Cyc_CfFlowInfo_Consumed_CfFlowInfo_AbsRVal_struct*)_tmpEA.f1)->f3;_LL7: {void*rval=_tmpEF;
# 580
ans=rval;goto retry;}case 6U: if(((struct Cyc_CfFlowInfo_Dot_CfFlowInfo_PathCon_struct*)_tmpEA.f2)->tag == 0U){_LL8: _tmpEC=((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmpEA.f1)->f1;_tmpED=((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmpEA.f1)->f2;_tmpEE=((struct Cyc_CfFlowInfo_Dot_CfFlowInfo_PathCon_struct*)_tmpEA.f2)->f1;_LL9: {struct Cyc_CfFlowInfo_UnionRInfo is_union=_tmpEC;struct _fat_ptr d2=_tmpED;int fname=_tmpEE;
# 582
ans=*((void**)_check_fat_subscript(d2,sizeof(void*),fname));
goto _LL3;}}else{goto _LLC;}case 5U: if(((struct Cyc_CfFlowInfo_Star_CfFlowInfo_PathCon_struct*)_tmpEA.f2)->tag == 1U){_LLA: _tmpEB=(void*)((struct Cyc_CfFlowInfo_UniquePtr_CfFlowInfo_AbsRVal_struct*)_tmpEA.f1)->f1;_LLB: {void*rval=_tmpEB;
# 585
ans=rval;
goto _LL3;}}else{goto _LLC;}default: _LLC: _LLD:
# 593
({void*_tmpF1=0U;({int(*_tmp292)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmpF3)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmpF3;});struct _fat_ptr _tmp291=({const char*_tmpF2="bad lookup_place";_tag_fat(_tmpF2,sizeof(char),17U);});_tmp292(_tmp291,_tag_fat(_tmpF1,sizeof(void*),0U));});});}_LL3:;}}
# 595
return ans;}}
# 598
static int Cyc_CfFlowInfo_is_rval_unescaped(void*rval){
void*_tmpF5=rval;struct _fat_ptr _tmpF8;int _tmpF7;int _tmpF6;void*_tmpF9;void*_tmpFA;switch(*((int*)_tmpF5)){case 3U: _LL1: _LL2:
 return 0;case 7U: _LL3: _tmpFA=(void*)((struct Cyc_CfFlowInfo_Consumed_CfFlowInfo_AbsRVal_struct*)_tmpF5)->f3;_LL4: {void*r=_tmpFA;
return({Cyc_CfFlowInfo_is_rval_unescaped(r);});}case 8U: _LL5: _tmpF9=(void*)((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmpF5)->f2;_LL6: {void*r=_tmpF9;
return({Cyc_CfFlowInfo_is_rval_unescaped(r);});}case 6U: _LL7: _tmpF6=(((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmpF5)->f1).is_union;_tmpF7=(((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmpF5)->f1).fieldnum;_tmpF8=((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmpF5)->f2;_LL8: {int is_union=_tmpF6;int field_no=_tmpF7;struct _fat_ptr d=_tmpF8;
# 604
if(is_union && field_no != -1)
return !({Cyc_CfFlowInfo_is_rval_unescaped(*((void**)_check_fat_subscript(d,sizeof(void*),field_no)));});else{
# 607
unsigned sz=_get_fat_size(d,sizeof(void*));
{int i=0;for(0;(unsigned)i < sz;++ i){
if(!({Cyc_CfFlowInfo_is_rval_unescaped(((void**)d.curr)[i]);}))return 0;}}
# 608
return 1;}}default: _LL9: _LLA:
# 612
 return 1;}_LL0:;}
# 598
int Cyc_CfFlowInfo_is_unescaped(struct Cyc_Dict_Dict d,struct Cyc_CfFlowInfo_Place*place){
# 616
return({int(*_tmpFC)(void*rval)=Cyc_CfFlowInfo_is_rval_unescaped;void*_tmpFD=({Cyc_CfFlowInfo_lookup_place(d,place);});_tmpFC(_tmpFD);});}
# 598
int Cyc_CfFlowInfo_is_init_pointer(void*rval){
# 619
void*_tmpFF=rval;void*_tmp100;switch(*((int*)_tmpFF)){case 8U: _LL1: _tmp100=(void*)((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmpFF)->f2;_LL2: {void*r=_tmp100;
return({Cyc_CfFlowInfo_is_init_pointer(r);});}case 4U: _LL3: _LL4:
 goto _LL6;case 5U: _LL5: _LL6:
 return 1;default: _LL7: _LL8:
 return 0;}_LL0:;}
# 598
struct Cyc_Dict_Dict Cyc_CfFlowInfo_escape_deref(struct Cyc_CfFlowInfo_FlowEnv*fenv,struct Cyc_Dict_Dict d,void*r){
# 629
struct _RegionHandle _tmp102=_new_region("rgn");struct _RegionHandle*rgn=& _tmp102;_push_region(rgn);
{struct Cyc_CfFlowInfo_EscPile*pile=({struct Cyc_CfFlowInfo_EscPile*_tmp104=_cycalloc(sizeof(*_tmp104));_tmp104->places=0;_tmp104;});
({Cyc_CfFlowInfo_add_places(pile,r);});{
struct Cyc_Dict_Dict _tmp103=({Cyc_CfFlowInfo_escape_these(fenv,pile,d);});_npop_handler(0U);return _tmp103;}}
# 630
;_pop_region();}struct Cyc_CfFlowInfo_AssignEnv{struct Cyc_CfFlowInfo_FlowEnv*fenv;struct Cyc_CfFlowInfo_EscPile*pile;struct Cyc_Dict_Dict d;unsigned loc;struct Cyc_CfFlowInfo_Place*root_place;};
# 642
static void*Cyc_CfFlowInfo_assign_place_inner(struct Cyc_CfFlowInfo_AssignEnv*env,void*oldval,void*newval){
# 647
struct _tuple13 _stmttmp12=({struct _tuple13 _tmp247;_tmp247.f1=oldval,_tmp247.f2=newval;_tmp247;});struct _tuple13 _tmp106=_stmttmp12;enum Cyc_CfFlowInfo_InitLevel _tmp107;struct _fat_ptr _tmp10B;struct Cyc_CfFlowInfo_UnionRInfo _tmp10A;struct _fat_ptr _tmp109;struct Cyc_CfFlowInfo_UnionRInfo _tmp108;struct Cyc_CfFlowInfo_Place*_tmp10C;void*_tmp10E;struct Cyc_Absyn_Vardecl*_tmp10D;void*_tmp10F;if(((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp106.f1)->tag == 8U){_LL1: _tmp10F=(void*)((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp106.f1)->f2;_LL2: {void*r1=_tmp10F;
# 649
return({Cyc_CfFlowInfo_assign_place_inner(env,r1,newval);});}}else{if(((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp106.f2)->tag == 8U){_LL3: _tmp10D=((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp106.f2)->f1;_tmp10E=(void*)((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp106.f2)->f2;_LL4: {struct Cyc_Absyn_Vardecl*n=_tmp10D;void*r=_tmp10E;
# 651
void*new_rval=({Cyc_CfFlowInfo_assign_place_inner(env,oldval,r);});
if(new_rval == r)return newval;else{
return(void*)({struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*_tmp110=_cycalloc(sizeof(*_tmp110));_tmp110->tag=8U,_tmp110->f1=n,_tmp110->f2=new_rval;_tmp110;});}}}else{switch(*((int*)_tmp106.f1)){case 3U: if(((struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct*)_tmp106.f2)->tag == 4U){_LL5: _tmp10C=((struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct*)_tmp106.f2)->f1;_LL6: {struct Cyc_CfFlowInfo_Place*p=_tmp10C;
({Cyc_CfFlowInfo_add_place(env->pile,p);});goto _LL8;}}else{_LL7: _LL8:
# 656
 if((int)({Cyc_CfFlowInfo_initlevel(env->fenv,env->d,newval);})!= (int)Cyc_CfFlowInfo_AllIL)
({void*_tmp111=0U;({unsigned _tmp294=env->loc;struct _fat_ptr _tmp293=({const char*_tmp112="assignment puts possibly-uninitialized data in an escaped location";_tag_fat(_tmp112,sizeof(char),67U);});Cyc_CfFlowInfo_aerr(_tmp294,_tmp293,_tag_fat(_tmp111,sizeof(void*),0U));});});
# 656
return(env->fenv)->esc_all;}case 6U: switch(*((int*)_tmp106.f2)){case 6U: _LL9: _tmp108=((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp106.f1)->f1;_tmp109=((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp106.f1)->f2;_tmp10A=((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp106.f2)->f1;_tmp10B=((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp106.f2)->f2;_LLA: {struct Cyc_CfFlowInfo_UnionRInfo is_union1=_tmp108;struct _fat_ptr d1=_tmp109;struct Cyc_CfFlowInfo_UnionRInfo is_union2=_tmp10A;struct _fat_ptr d2=_tmp10B;
# 661
struct _fat_ptr new_d=({unsigned _tmp115=
_get_fat_size(d1,sizeof(void*));void**_tmp114=_cycalloc(_check_times(_tmp115,sizeof(void*)));({{unsigned _tmp246=_get_fat_size(d1,sizeof(void*));unsigned i;for(i=0;i < _tmp246;++ i){({void*_tmp295=({Cyc_CfFlowInfo_assign_place_inner(env,((void**)d1.curr)[(int)i],*((void**)_check_fat_subscript(d2,sizeof(void*),(int)i)));});_tmp114[i]=_tmp295;});}}0;});_tag_fat(_tmp114,sizeof(void*),_tmp115);});
# 665
int change=0;
{int i=0;for(0;(unsigned)i < _get_fat_size(d1,sizeof(void*));++ i){
if(*((void**)_check_fat_subscript(new_d,sizeof(void*),i))!= ((void**)d1.curr)[i]){
change=1;break;}}}
# 666
if(!change){
# 671
if(!is_union1.is_union)return oldval;new_d=d1;}else{
# 675
change=0;
{int i=0;for(0;(unsigned)i < _get_fat_size(d1,sizeof(void*));++ i){
if(({void*_tmp296=*((void**)_check_fat_subscript(new_d,sizeof(void*),i));_tmp296 != *((void**)_check_fat_subscript(d2,sizeof(void*),i));})){
change=1;break;}}}
# 676
if(!change){
# 681
if(!is_union1.is_union)return newval;new_d=d2;}}
# 685
return(void*)({struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*_tmp113=_cycalloc(sizeof(*_tmp113));_tmp113->tag=6U,_tmp113->f1=is_union2,_tmp113->f2=new_d;_tmp113;});}case 3U: goto _LLB;default: goto _LLD;}default: if(((struct Cyc_CfFlowInfo_Esc_CfFlowInfo_AbsRVal_struct*)_tmp106.f2)->tag == 3U){_LLB: _tmp107=((struct Cyc_CfFlowInfo_Esc_CfFlowInfo_AbsRVal_struct*)_tmp106.f2)->f1;_LLC: {enum Cyc_CfFlowInfo_InitLevel il=_tmp107;
# 687
enum Cyc_CfFlowInfo_InitLevel _tmp116=il;if(_tmp116 == Cyc_CfFlowInfo_NoneIL){_LL10: _LL11:
 return(env->fenv)->unknown_none;}else{_LL12: _LL13:
 return(env->fenv)->unknown_all;}_LLF:;}}else{_LLD: _LLE:
# 691
 return newval;}}}}_LL0:;}
# 697
static int Cyc_CfFlowInfo_nprefix(int*n,void*unused){
if(*n > 0){*n=*n - 1;return 1;}else{
return 0;}}
# 701
static void*Cyc_CfFlowInfo_assign_place_outer(struct Cyc_CfFlowInfo_AssignEnv*env,struct Cyc_List_List*path,int path_prefix,void*oldval,void*newval){
# 712
if(path == 0)return({Cyc_CfFlowInfo_assign_place_inner(env,oldval,newval);});{struct _tuple18 _stmttmp13=({struct _tuple18 _tmp248;_tmp248.f1=path,_tmp248.f2=oldval;_tmp248;});struct _tuple18 _tmp119=_stmttmp13;struct _fat_ptr _tmp11E;int _tmp11D;int _tmp11C;struct Cyc_List_List*_tmp11B;int _tmp11A;void*_tmp120;struct Cyc_List_List*_tmp11F;void*_tmp123;int _tmp122;struct Cyc_Absyn_Exp*_tmp121;void*_tmp125;struct Cyc_Absyn_Vardecl*_tmp124;switch(*((int*)_tmp119.f2)){case 8U: _LL1: _tmp124=((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp119.f2)->f1;_tmp125=(void*)((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp119.f2)->f2;_LL2: {struct Cyc_Absyn_Vardecl*n=_tmp124;void*r=_tmp125;
# 715
void*new_r=({Cyc_CfFlowInfo_assign_place_outer(env,path,path_prefix,r,newval);});
if(new_r == r)return oldval;else{
return(void*)({struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*_tmp126=_cycalloc(sizeof(*_tmp126));_tmp126->tag=8U,_tmp126->f1=n,_tmp126->f2=new_r;_tmp126;});}}case 7U: _LL3: _tmp121=((struct Cyc_CfFlowInfo_Consumed_CfFlowInfo_AbsRVal_struct*)_tmp119.f2)->f1;_tmp122=((struct Cyc_CfFlowInfo_Consumed_CfFlowInfo_AbsRVal_struct*)_tmp119.f2)->f2;_tmp123=(void*)((struct Cyc_CfFlowInfo_Consumed_CfFlowInfo_AbsRVal_struct*)_tmp119.f2)->f3;_LL4: {struct Cyc_Absyn_Exp*x=_tmp121;int y=_tmp122;void*r=_tmp123;
# 719
void*new_r=({Cyc_CfFlowInfo_assign_place_outer(env,path,path_prefix,r,newval);});
if(new_r == r)return oldval;else{
return(void*)({struct Cyc_CfFlowInfo_Consumed_CfFlowInfo_AbsRVal_struct*_tmp127=_cycalloc(sizeof(*_tmp127));_tmp127->tag=7U,_tmp127->f1=x,_tmp127->f2=y,_tmp127->f3=new_r;_tmp127;});}}default: if(_tmp119.f1 != 0){if(((struct Cyc_CfFlowInfo_Star_CfFlowInfo_PathCon_struct*)((struct Cyc_List_List*)_tmp119.f1)->hd)->tag == 1U){if(((struct Cyc_CfFlowInfo_UniquePtr_CfFlowInfo_AbsRVal_struct*)_tmp119.f2)->tag == 5U){_LL5: _tmp11F=(_tmp119.f1)->tl;_tmp120=(void*)((struct Cyc_CfFlowInfo_UniquePtr_CfFlowInfo_AbsRVal_struct*)_tmp119.f2)->f1;_LL6: {struct Cyc_List_List*tl=_tmp11F;void*r=_tmp120;
# 723
void*new_r=({Cyc_CfFlowInfo_assign_place_outer(env,tl,path_prefix + 1,r,newval);});
if(new_r == r)return oldval;else{
return(void*)({struct Cyc_CfFlowInfo_UniquePtr_CfFlowInfo_AbsRVal_struct*_tmp128=_cycalloc(sizeof(*_tmp128));_tmp128->tag=5U,_tmp128->f1=new_r;_tmp128;});}}}else{goto _LL9;}}else{if(((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp119.f2)->tag == 6U){_LL7: _tmp11A=((struct Cyc_CfFlowInfo_Dot_CfFlowInfo_PathCon_struct*)(_tmp119.f1)->hd)->f1;_tmp11B=(_tmp119.f1)->tl;_tmp11C=(((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp119.f2)->f1).is_union;_tmp11D=(((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp119.f2)->f1).fieldnum;_tmp11E=((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp119.f2)->f2;_LL8: {int fnum=_tmp11A;struct Cyc_List_List*tl=_tmp11B;int is_union=_tmp11C;int fldnum=_tmp11D;struct _fat_ptr d=_tmp11E;
# 727
void*new_child=({Cyc_CfFlowInfo_assign_place_outer(env,tl,path_prefix + 1,*((void**)_check_fat_subscript(d,sizeof(void*),fnum)),newval);});
# 729
struct _fat_ptr new_child_agg=({Cyc_CfFlowInfo_aggr_dict_insert(d,fnum,new_child);});
if((void**)new_child_agg.curr == (void**)d.curr &&(!is_union || fldnum == fnum))return oldval;d=new_child_agg;
# 734
if(is_union){
int changed=0;
int sz=(int)_get_fat_size(d,sizeof(void*));
{int i=0;for(0;i < sz;++ i){
if(i != fnum){
struct _fat_ptr new_d=({struct _fat_ptr(*_tmp129)(struct _fat_ptr d,int n,void*rval)=Cyc_CfFlowInfo_aggr_dict_insert;struct _fat_ptr _tmp12A=d;int _tmp12B=i;void*_tmp12C=({Cyc_CfFlowInfo_insert_place_inner((env->fenv)->unknown_all,*((void**)_check_fat_subscript(d,sizeof(void*),i)));});_tmp129(_tmp12A,_tmp12B,_tmp12C);});
# 742
if((void**)new_d.curr != (void**)d.curr){
d=new_d;
changed=1;}}}}
# 750
if(changed){
struct Cyc_CfFlowInfo_Place*_stmttmp14=env->root_place;struct Cyc_List_List*_tmp12E;void*_tmp12D;_LLC: _tmp12D=_stmttmp14->root;_tmp12E=_stmttmp14->path;_LLD: {void*root=_tmp12D;struct Cyc_List_List*path=_tmp12E;
struct Cyc_List_List*new_path=({({struct Cyc_List_List*(*_tmp298)(int(*f)(int*,void*),int*env,struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp130)(int(*f)(int*,void*),int*env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(int(*f)(int*,void*),int*env,struct Cyc_List_List*x))Cyc_List_filter_c;_tmp130;});int*_tmp297=({int*_tmp131=_cycalloc_atomic(sizeof(*_tmp131));*_tmp131=path_prefix;_tmp131;});_tmp298(Cyc_CfFlowInfo_nprefix,_tmp297,path);});});
struct Cyc_CfFlowInfo_Place*curr_place=({struct Cyc_CfFlowInfo_Place*_tmp12F=_cycalloc(sizeof(*_tmp12F));_tmp12F->root=root,_tmp12F->path=new_path;_tmp12F;});;}}}
# 734
return(void*)({struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*_tmp132=_cycalloc(sizeof(*_tmp132));
# 756
_tmp132->tag=6U,(_tmp132->f1).is_union=is_union,(_tmp132->f1).fieldnum=fnum,_tmp132->f2=d;_tmp132;});}}else{goto _LL9;}}}else{_LL9: _LLA:
({void*_tmp133=0U;({int(*_tmp29A)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp135)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp135;});struct _fat_ptr _tmp299=({const char*_tmp134="bad assign place";_tag_fat(_tmp134,sizeof(char),17U);});_tmp29A(_tmp299,_tag_fat(_tmp133,sizeof(void*),0U));});});}}_LL0:;}}
# 701
struct Cyc_Dict_Dict Cyc_CfFlowInfo_assign_place(struct Cyc_CfFlowInfo_FlowEnv*fenv,unsigned loc,struct Cyc_Dict_Dict d,struct Cyc_CfFlowInfo_Place*place,void*r){
# 768
struct Cyc_List_List*_tmp138;void*_tmp137;_LL1: _tmp137=place->root;_tmp138=place->path;_LL2: {void*root=_tmp137;struct Cyc_List_List*path=_tmp138;
struct Cyc_CfFlowInfo_AssignEnv env=({struct Cyc_CfFlowInfo_AssignEnv _tmp249;_tmp249.fenv=fenv,({struct Cyc_CfFlowInfo_EscPile*_tmp29B=({struct Cyc_CfFlowInfo_EscPile*_tmp143=_cycalloc(sizeof(*_tmp143));_tmp143->places=0;_tmp143;});_tmp249.pile=_tmp29B;}),_tmp249.d=d,_tmp249.loc=loc,_tmp249.root_place=place;_tmp249;});
void*newval=({void*(*_tmp13D)(struct Cyc_CfFlowInfo_AssignEnv*env,struct Cyc_List_List*path,int path_prefix,void*oldval,void*newval)=Cyc_CfFlowInfo_assign_place_outer;struct Cyc_CfFlowInfo_AssignEnv*_tmp13E=& env;struct Cyc_List_List*_tmp13F=path;int _tmp140=0;void*_tmp141=({Cyc_Dict_lookup(d,root);});void*_tmp142=r;_tmp13D(_tmp13E,_tmp13F,_tmp140,_tmp141,_tmp142);});
return({struct Cyc_Dict_Dict(*_tmp139)(struct Cyc_CfFlowInfo_FlowEnv*fenv,struct Cyc_CfFlowInfo_EscPile*pile,struct Cyc_Dict_Dict d)=Cyc_CfFlowInfo_escape_these;struct Cyc_CfFlowInfo_FlowEnv*_tmp13A=fenv;struct Cyc_CfFlowInfo_EscPile*_tmp13B=env.pile;struct Cyc_Dict_Dict _tmp13C=({Cyc_Dict_insert(d,root,newval);});_tmp139(_tmp13A,_tmp13B,_tmp13C);});}}struct Cyc_CfFlowInfo_JoinEnv{struct Cyc_CfFlowInfo_FlowEnv*fenv;struct Cyc_CfFlowInfo_EscPile*pile;struct Cyc_Dict_Dict d1;struct Cyc_Dict_Dict d2;};
# 783
static int Cyc_CfFlowInfo_absRval_lessthan_approx(void*ignore,void*r1,void*r2);struct _tuple19{enum Cyc_CfFlowInfo_InitLevel f1;enum Cyc_CfFlowInfo_InitLevel f2;};
# 791
static void*Cyc_CfFlowInfo_join_absRval(struct Cyc_CfFlowInfo_JoinEnv*env,void*a,void*r1,void*r2){
if(r1 == r2)return r1;{struct _tuple13 _stmttmp15=({struct _tuple13 _tmp24B;_tmp24B.f1=r1,_tmp24B.f2=r2;_tmp24B;});struct _tuple13 _tmp145=_stmttmp15;struct _fat_ptr _tmp14B;int _tmp14A;int _tmp149;struct _fat_ptr _tmp148;int _tmp147;int _tmp146;struct Cyc_CfFlowInfo_Place*_tmp14C;void*_tmp14D;void*_tmp14F;void*_tmp14E;void*_tmp150;struct Cyc_CfFlowInfo_Place*_tmp151;struct Cyc_CfFlowInfo_Place*_tmp152;struct Cyc_CfFlowInfo_Place*_tmp153;struct Cyc_CfFlowInfo_Place*_tmp155;struct Cyc_CfFlowInfo_Place*_tmp154;void*_tmp158;int _tmp157;struct Cyc_Absyn_Exp*_tmp156;void*_tmp15B;int _tmp15A;struct Cyc_Absyn_Exp*_tmp159;void*_tmp161;int _tmp160;struct Cyc_Absyn_Exp*_tmp15F;void*_tmp15E;int _tmp15D;struct Cyc_Absyn_Exp*_tmp15C;void*_tmp163;struct Cyc_Absyn_Vardecl*_tmp162;void*_tmp165;struct Cyc_Absyn_Vardecl*_tmp164;void*_tmp169;struct Cyc_Absyn_Vardecl*_tmp168;void*_tmp167;struct Cyc_Absyn_Vardecl*_tmp166;if(((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp145.f1)->tag == 8U){if(((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp145.f2)->tag == 8U){_LL1: _tmp166=((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp145.f1)->f1;_tmp167=(void*)((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp145.f1)->f2;_tmp168=((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp145.f2)->f1;_tmp169=(void*)((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp145.f2)->f2;_LL2: {struct Cyc_Absyn_Vardecl*n1=_tmp166;void*r1=_tmp167;struct Cyc_Absyn_Vardecl*n2=_tmp168;void*r2=_tmp169;
# 797
if(n1 == n2)
return(void*)({struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*_tmp16A=_cycalloc(sizeof(*_tmp16A));_tmp16A->tag=8U,_tmp16A->f1=n1,({void*_tmp29C=({Cyc_CfFlowInfo_join_absRval(env,a,r1,r2);});_tmp16A->f2=_tmp29C;});_tmp16A;});else{
# 800
return({Cyc_CfFlowInfo_join_absRval(env,a,r1,r2);});}}}else{_LL3: _tmp164=((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp145.f1)->f1;_tmp165=(void*)((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp145.f1)->f2;_LL4: {struct Cyc_Absyn_Vardecl*n1=_tmp164;void*r1=_tmp165;
return({Cyc_CfFlowInfo_join_absRval(env,a,r1,r2);});}}}else{if(((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp145.f2)->tag == 8U){_LL5: _tmp162=((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp145.f2)->f1;_tmp163=(void*)((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp145.f2)->f2;_LL6: {struct Cyc_Absyn_Vardecl*n2=_tmp162;void*r2=_tmp163;
return({Cyc_CfFlowInfo_join_absRval(env,a,r1,r2);});}}else{if(((struct Cyc_CfFlowInfo_Consumed_CfFlowInfo_AbsRVal_struct*)_tmp145.f1)->tag == 7U){if(((struct Cyc_CfFlowInfo_Consumed_CfFlowInfo_AbsRVal_struct*)_tmp145.f2)->tag == 7U){_LL7: _tmp15C=((struct Cyc_CfFlowInfo_Consumed_CfFlowInfo_AbsRVal_struct*)_tmp145.f1)->f1;_tmp15D=((struct Cyc_CfFlowInfo_Consumed_CfFlowInfo_AbsRVal_struct*)_tmp145.f1)->f2;_tmp15E=(void*)((struct Cyc_CfFlowInfo_Consumed_CfFlowInfo_AbsRVal_struct*)_tmp145.f1)->f3;_tmp15F=((struct Cyc_CfFlowInfo_Consumed_CfFlowInfo_AbsRVal_struct*)_tmp145.f2)->f1;_tmp160=((struct Cyc_CfFlowInfo_Consumed_CfFlowInfo_AbsRVal_struct*)_tmp145.f2)->f2;_tmp161=(void*)((struct Cyc_CfFlowInfo_Consumed_CfFlowInfo_AbsRVal_struct*)_tmp145.f2)->f3;_LL8: {struct Cyc_Absyn_Exp*e1=_tmp15C;int i1=_tmp15D;void*r1=_tmp15E;struct Cyc_Absyn_Exp*e2=_tmp15F;int i2=_tmp160;void*r2=_tmp161;
# 805
if(e1 == e2)
return(void*)({struct Cyc_CfFlowInfo_Consumed_CfFlowInfo_AbsRVal_struct*_tmp16B=_cycalloc(sizeof(*_tmp16B));_tmp16B->tag=7U,_tmp16B->f1=e1,i1 > i2?_tmp16B->f2=i1:(_tmp16B->f2=i2),({void*_tmp29D=({Cyc_CfFlowInfo_join_absRval(env,a,r1,r2);});_tmp16B->f3=_tmp29D;});_tmp16B;});
# 805
{void*_tmp16C=r1;struct Cyc_CfFlowInfo_Place*_tmp16D;if(((struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct*)_tmp16C)->tag == 4U){_LL22: _tmp16D=((struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct*)_tmp16C)->f1;_LL23: {struct Cyc_CfFlowInfo_Place*p=_tmp16D;
# 808
({Cyc_CfFlowInfo_add_place(env->pile,p);});goto _LL21;}}else{_LL24: _LL25:
 goto _LL21;}_LL21:;}
# 811
{void*_tmp16E=r2;struct Cyc_CfFlowInfo_Place*_tmp16F;if(((struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct*)_tmp16E)->tag == 4U){_LL27: _tmp16F=((struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct*)_tmp16E)->f1;_LL28: {struct Cyc_CfFlowInfo_Place*p=_tmp16F;
({Cyc_CfFlowInfo_add_place(env->pile,p);});goto _LL26;}}else{_LL29: _LL2A:
 goto _LL26;}_LL26:;}
# 815
goto _LL0;}}else{_LL9: _tmp159=((struct Cyc_CfFlowInfo_Consumed_CfFlowInfo_AbsRVal_struct*)_tmp145.f1)->f1;_tmp15A=((struct Cyc_CfFlowInfo_Consumed_CfFlowInfo_AbsRVal_struct*)_tmp145.f1)->f2;_tmp15B=(void*)((struct Cyc_CfFlowInfo_Consumed_CfFlowInfo_AbsRVal_struct*)_tmp145.f1)->f3;_LLA: {struct Cyc_Absyn_Exp*e1=_tmp159;int i1=_tmp15A;void*r1=_tmp15B;
# 817
return(void*)({struct Cyc_CfFlowInfo_Consumed_CfFlowInfo_AbsRVal_struct*_tmp170=_cycalloc(sizeof(*_tmp170));_tmp170->tag=7U,_tmp170->f1=e1,_tmp170->f2=i1,({void*_tmp29E=({Cyc_CfFlowInfo_join_absRval(env,a,r1,r2);});_tmp170->f3=_tmp29E;});_tmp170;});}}}else{if(((struct Cyc_CfFlowInfo_Consumed_CfFlowInfo_AbsRVal_struct*)_tmp145.f2)->tag == 7U){_LLB: _tmp156=((struct Cyc_CfFlowInfo_Consumed_CfFlowInfo_AbsRVal_struct*)_tmp145.f2)->f1;_tmp157=((struct Cyc_CfFlowInfo_Consumed_CfFlowInfo_AbsRVal_struct*)_tmp145.f2)->f2;_tmp158=(void*)((struct Cyc_CfFlowInfo_Consumed_CfFlowInfo_AbsRVal_struct*)_tmp145.f2)->f3;_LLC: {struct Cyc_Absyn_Exp*e2=_tmp156;int i2=_tmp157;void*r2=_tmp158;
# 819
return(void*)({struct Cyc_CfFlowInfo_Consumed_CfFlowInfo_AbsRVal_struct*_tmp171=_cycalloc(sizeof(*_tmp171));_tmp171->tag=7U,_tmp171->f1=e2,_tmp171->f2=i2,({void*_tmp29F=({Cyc_CfFlowInfo_join_absRval(env,a,r1,r2);});_tmp171->f3=_tmp29F;});_tmp171;});}}else{switch(*((int*)_tmp145.f1)){case 4U: switch(*((int*)_tmp145.f2)){case 4U: _LLD: _tmp154=((struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct*)_tmp145.f1)->f1;_tmp155=((struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct*)_tmp145.f2)->f1;_LLE: {struct Cyc_CfFlowInfo_Place*p1=_tmp154;struct Cyc_CfFlowInfo_Place*p2=_tmp155;
# 822
if(({Cyc_CfFlowInfo_place_cmp(p1,p2);})== 0)return r1;({Cyc_CfFlowInfo_add_place(env->pile,p1);});
# 824
({Cyc_CfFlowInfo_add_place(env->pile,p2);});
goto _LL0;}case 1U: _LLF: _tmp153=((struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct*)_tmp145.f1)->f1;_LL10: {struct Cyc_CfFlowInfo_Place*p=_tmp153;
# 828
({Cyc_CfFlowInfo_add_place(env->pile,p);});{
enum Cyc_CfFlowInfo_InitLevel _stmttmp16=({Cyc_CfFlowInfo_initlevel(env->fenv,env->d1,r1);});enum Cyc_CfFlowInfo_InitLevel _tmp172=_stmttmp16;if(_tmp172 == Cyc_CfFlowInfo_AllIL){_LL2C: _LL2D:
 return(env->fenv)->notzeroall;}else{_LL2E: _LL2F:
 return(env->fenv)->unknown_none;}_LL2B:;}}default: _LL19: _tmp152=((struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct*)_tmp145.f1)->f1;_LL1A: {struct Cyc_CfFlowInfo_Place*p=_tmp152;
# 854
({Cyc_CfFlowInfo_add_place(env->pile,p);});goto _LL0;}}case 1U: switch(*((int*)_tmp145.f2)){case 4U: _LL11: _tmp151=((struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct*)_tmp145.f2)->f1;_LL12: {struct Cyc_CfFlowInfo_Place*p=_tmp151;
# 834
({Cyc_CfFlowInfo_add_place(env->pile,p);});{
enum Cyc_CfFlowInfo_InitLevel _stmttmp17=({Cyc_CfFlowInfo_initlevel(env->fenv,env->d2,r2);});enum Cyc_CfFlowInfo_InitLevel _tmp173=_stmttmp17;if(_tmp173 == Cyc_CfFlowInfo_AllIL){_LL31: _LL32:
 return(env->fenv)->notzeroall;}else{_LL33: _LL34:
 return(env->fenv)->unknown_none;}_LL30:;}}case 5U: _LL17: _tmp150=(void*)((struct Cyc_CfFlowInfo_UniquePtr_CfFlowInfo_AbsRVal_struct*)_tmp145.f2)->f1;_LL18: {void*r2=_tmp150;
# 849
enum Cyc_CfFlowInfo_InitLevel _stmttmp19=({Cyc_CfFlowInfo_initlevel(env->fenv,env->d2,r2);});enum Cyc_CfFlowInfo_InitLevel _tmp176=_stmttmp19;if(_tmp176 == Cyc_CfFlowInfo_AllIL){_LL3B: _LL3C:
 return(env->fenv)->notzeroall;}else{_LL3D: _LL3E:
 return(env->fenv)->unknown_none;}_LL3A:;}default: goto _LL1F;}case 5U: switch(*((int*)_tmp145.f2)){case 5U: _LL13: _tmp14E=(void*)((struct Cyc_CfFlowInfo_UniquePtr_CfFlowInfo_AbsRVal_struct*)_tmp145.f1)->f1;_tmp14F=(void*)((struct Cyc_CfFlowInfo_UniquePtr_CfFlowInfo_AbsRVal_struct*)_tmp145.f2)->f1;_LL14: {void*r1=_tmp14E;void*r2=_tmp14F;
# 841
return(void*)({struct Cyc_CfFlowInfo_UniquePtr_CfFlowInfo_AbsRVal_struct*_tmp174=_cycalloc(sizeof(*_tmp174));_tmp174->tag=5U,({void*_tmp2A0=({Cyc_CfFlowInfo_join_absRval(env,a,r1,r2);});_tmp174->f1=_tmp2A0;});_tmp174;});}case 1U: _LL15: _tmp14D=(void*)((struct Cyc_CfFlowInfo_UniquePtr_CfFlowInfo_AbsRVal_struct*)_tmp145.f1)->f1;_LL16: {void*r1=_tmp14D;
# 844
enum Cyc_CfFlowInfo_InitLevel _stmttmp18=({Cyc_CfFlowInfo_initlevel(env->fenv,env->d1,r1);});enum Cyc_CfFlowInfo_InitLevel _tmp175=_stmttmp18;if(_tmp175 == Cyc_CfFlowInfo_AllIL){_LL36: _LL37:
 return(env->fenv)->notzeroall;}else{_LL38: _LL39:
 return(env->fenv)->unknown_none;}_LL35:;}case 4U: goto _LL1B;default: goto _LL1F;}default: if(((struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct*)_tmp145.f2)->tag == 4U){_LL1B: _tmp14C=((struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct*)_tmp145.f2)->f1;_LL1C: {struct Cyc_CfFlowInfo_Place*p=_tmp14C;
# 855
({Cyc_CfFlowInfo_add_place(env->pile,p);});goto _LL0;}}else{if(((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp145.f1)->tag == 6U){if(((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp145.f2)->tag == 6U){_LL1D: _tmp146=(((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp145.f1)->f1).is_union;_tmp147=(((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp145.f1)->f1).fieldnum;_tmp148=((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp145.f1)->f2;_tmp149=(((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp145.f2)->f1).is_union;_tmp14A=(((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp145.f2)->f1).fieldnum;_tmp14B=((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp145.f2)->f2;_LL1E: {int is_union1=_tmp146;int field_no1=_tmp147;struct _fat_ptr d1=_tmp148;int is_union2=_tmp149;int field_no2=_tmp14A;struct _fat_ptr d2=_tmp14B;
# 858
struct _fat_ptr new_d=({unsigned _tmp17A=
_get_fat_size(d1,sizeof(void*));void**_tmp179=_cycalloc(_check_times(_tmp17A,sizeof(void*)));({{unsigned _tmp24A=_get_fat_size(d1,sizeof(void*));unsigned i;for(i=0;i < _tmp24A;++ i){({void*_tmp2A4=({({void*(*_tmp2A3)(struct Cyc_CfFlowInfo_JoinEnv*env,int a,void*r1,void*r2)=({void*(*_tmp178)(struct Cyc_CfFlowInfo_JoinEnv*env,int a,void*r1,void*r2)=(void*(*)(struct Cyc_CfFlowInfo_JoinEnv*env,int a,void*r1,void*r2))Cyc_CfFlowInfo_join_absRval;_tmp178;});struct Cyc_CfFlowInfo_JoinEnv*_tmp2A2=env;void*_tmp2A1=((void**)d1.curr)[(int)i];_tmp2A3(_tmp2A2,0,_tmp2A1,*((void**)_check_fat_subscript(d2,sizeof(void*),(int)i)));});});_tmp179[i]=_tmp2A4;});}}0;});_tag_fat(_tmp179,sizeof(void*),_tmp17A);});
# 863
int change=0;
{int i=0;for(0;(unsigned)i < _get_fat_size(d1,sizeof(void*));++ i){
if(*((void**)_check_fat_subscript(new_d,sizeof(void*),i))!= ((void**)d1.curr)[i]){
change=1;break;}}}
# 864
if(!change){
# 869
if(!is_union1)return r1;new_d=d1;}else{
# 873
change=0;
{int i=0;for(0;(unsigned)i < _get_fat_size(d1,sizeof(void*));++ i){
if(({void*_tmp2A5=*((void**)_check_fat_subscript(new_d,sizeof(void*),i));_tmp2A5 != *((void**)_check_fat_subscript(d2,sizeof(void*),i));})){
change=1;break;}}}
# 874
if(!change){
# 879
if(!is_union1)return r2;new_d=d2;}}
# 883
return(void*)({struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*_tmp177=_cycalloc(sizeof(*_tmp177));_tmp177->tag=6U,(_tmp177->f1).is_union=is_union1,field_no1 == field_no2?(_tmp177->f1).fieldnum=field_no1:((_tmp177->f1).fieldnum=- 1),_tmp177->f2=new_d;_tmp177;});}}else{goto _LL1F;}}else{_LL1F: _LL20:
# 885
 goto _LL0;}}}}}}}_LL0:;}{
# 887
enum Cyc_CfFlowInfo_InitLevel il1=({Cyc_CfFlowInfo_initlevel(env->fenv,env->d1,r1);});
enum Cyc_CfFlowInfo_InitLevel il2=({Cyc_CfFlowInfo_initlevel(env->fenv,env->d2,r2);});
struct _tuple13 _stmttmp1A=({struct _tuple13 _tmp24E;_tmp24E.f1=r1,_tmp24E.f2=r2;_tmp24E;});struct _tuple13 _tmp17B=_stmttmp1A;if(((struct Cyc_CfFlowInfo_Esc_CfFlowInfo_AbsRVal_struct*)_tmp17B.f1)->tag == 3U){_LL40: _LL41:
 goto _LL43;}else{if(((struct Cyc_CfFlowInfo_Esc_CfFlowInfo_AbsRVal_struct*)_tmp17B.f2)->tag == 3U){_LL42: _LL43: {
# 892
struct _tuple19 _stmttmp1B=({struct _tuple19 _tmp24C;_tmp24C.f1=il1,_tmp24C.f2=il2;_tmp24C;});struct _tuple19 _tmp17C=_stmttmp1B;if(_tmp17C.f2 == Cyc_CfFlowInfo_NoneIL){_LL47: _LL48:
 goto _LL4A;}else{if(_tmp17C.f1 == Cyc_CfFlowInfo_NoneIL){_LL49: _LL4A: return(env->fenv)->esc_none;}else{_LL4B: _LL4C:
 return(env->fenv)->esc_all;}}_LL46:;}}else{_LL44: _LL45: {
# 897
struct _tuple19 _stmttmp1C=({struct _tuple19 _tmp24D;_tmp24D.f1=il1,_tmp24D.f2=il2;_tmp24D;});struct _tuple19 _tmp17D=_stmttmp1C;if(_tmp17D.f2 == Cyc_CfFlowInfo_NoneIL){_LL4E: _LL4F:
 goto _LL51;}else{if(_tmp17D.f1 == Cyc_CfFlowInfo_NoneIL){_LL50: _LL51: return(env->fenv)->unknown_none;}else{_LL52: _LL53:
 return(env->fenv)->unknown_all;}}_LL4D:;}}}_LL3F:;}}struct _tuple20{union Cyc_CfFlowInfo_FlowInfo f1;union Cyc_CfFlowInfo_FlowInfo f2;};
# 791
union Cyc_CfFlowInfo_FlowInfo Cyc_CfFlowInfo_join_flow(struct Cyc_CfFlowInfo_FlowEnv*fenv,union Cyc_CfFlowInfo_FlowInfo f1,union Cyc_CfFlowInfo_FlowInfo f2){
# 906
struct _tuple20 _stmttmp1D=({struct _tuple20 _tmp250;_tmp250.f1=f1,_tmp250.f2=f2;_tmp250;});struct _tuple20 _tmp17F=_stmttmp1D;struct Cyc_List_List*_tmp183;struct Cyc_Dict_Dict _tmp182;struct Cyc_List_List*_tmp181;struct Cyc_Dict_Dict _tmp180;if(((_tmp17F.f1).BottomFL).tag == 1){_LL1: _LL2:
 return f2;}else{if(((_tmp17F.f2).BottomFL).tag == 1){_LL3: _LL4:
 return f1;}else{_LL5: _tmp180=(((_tmp17F.f1).ReachableFL).val).f1;_tmp181=(((_tmp17F.f1).ReachableFL).val).f2;_tmp182=(((_tmp17F.f2).ReachableFL).val).f1;_tmp183=(((_tmp17F.f2).ReachableFL).val).f2;_LL6: {struct Cyc_Dict_Dict d1=_tmp180;struct Cyc_List_List*r1=_tmp181;struct Cyc_Dict_Dict d2=_tmp182;struct Cyc_List_List*r2=_tmp183;
# 912
if(d1.t == d2.t && r1 == r2)return f1;if(({Cyc_CfFlowInfo_flow_lessthan_approx(f1,f2);}))
return f2;
# 912
if(({Cyc_CfFlowInfo_flow_lessthan_approx(f2,f1);}))
# 914
return f1;{
# 912
struct Cyc_CfFlowInfo_JoinEnv env=({struct Cyc_CfFlowInfo_JoinEnv _tmp24F;_tmp24F.fenv=fenv,({
# 915
struct Cyc_CfFlowInfo_EscPile*_tmp2A6=({struct Cyc_CfFlowInfo_EscPile*_tmp188=_cycalloc(sizeof(*_tmp188));_tmp188->places=0;_tmp188;});_tmp24F.pile=_tmp2A6;}),_tmp24F.d1=d1,_tmp24F.d2=d2;_tmp24F;});
struct Cyc_Dict_Dict outdict=({({struct Cyc_Dict_Dict(*_tmp2A8)(void*(*f)(struct Cyc_CfFlowInfo_JoinEnv*,void*,void*,void*),struct Cyc_CfFlowInfo_JoinEnv*env,struct Cyc_Dict_Dict d1,struct Cyc_Dict_Dict d2)=({struct Cyc_Dict_Dict(*_tmp187)(void*(*f)(struct Cyc_CfFlowInfo_JoinEnv*,void*,void*,void*),struct Cyc_CfFlowInfo_JoinEnv*env,struct Cyc_Dict_Dict d1,struct Cyc_Dict_Dict d2)=(struct Cyc_Dict_Dict(*)(void*(*f)(struct Cyc_CfFlowInfo_JoinEnv*,void*,void*,void*),struct Cyc_CfFlowInfo_JoinEnv*env,struct Cyc_Dict_Dict d1,struct Cyc_Dict_Dict d2))Cyc_Dict_intersect_c;_tmp187;});struct Cyc_Dict_Dict _tmp2A7=d1;_tmp2A8(Cyc_CfFlowInfo_join_absRval,& env,_tmp2A7,d2);});});
struct Cyc_List_List*r=({Cyc_Relations_join_relns(Cyc_Core_heap_region,r1,r2);});
return({union Cyc_CfFlowInfo_FlowInfo(*_tmp184)(struct Cyc_Dict_Dict fd,struct Cyc_List_List*r)=Cyc_CfFlowInfo_ReachableFL;struct Cyc_Dict_Dict _tmp185=({Cyc_CfFlowInfo_escape_these(fenv,env.pile,outdict);});struct Cyc_List_List*_tmp186=r;_tmp184(_tmp185,_tmp186);});}}}}_LL0:;}struct _tuple21{union Cyc_CfFlowInfo_FlowInfo f1;void*f2;};struct _tuple22{union Cyc_CfFlowInfo_FlowInfo f1;union Cyc_CfFlowInfo_FlowInfo f2;union Cyc_CfFlowInfo_FlowInfo f3;};
# 922
struct _tuple21 Cyc_CfFlowInfo_join_flow_and_rval(struct Cyc_CfFlowInfo_FlowEnv*fenv,struct _tuple21 pr1,struct _tuple21 pr2){
# 927
void*_tmp18B;union Cyc_CfFlowInfo_FlowInfo _tmp18A;_LL1: _tmp18A=pr1.f1;_tmp18B=pr1.f2;_LL2: {union Cyc_CfFlowInfo_FlowInfo f1=_tmp18A;void*r1=_tmp18B;
void*_tmp18D;union Cyc_CfFlowInfo_FlowInfo _tmp18C;_LL4: _tmp18C=pr2.f1;_tmp18D=pr2.f2;_LL5: {union Cyc_CfFlowInfo_FlowInfo f2=_tmp18C;void*r2=_tmp18D;
union Cyc_CfFlowInfo_FlowInfo outflow=({Cyc_CfFlowInfo_join_flow(fenv,f1,f2);});
struct _tuple22 _stmttmp1E=({struct _tuple22 _tmp257;_tmp257.f1=f1,_tmp257.f2=f2,_tmp257.f3=outflow;_tmp257;});struct _tuple22 _tmp18E=_stmttmp1E;struct Cyc_List_List*_tmp192;struct Cyc_Dict_Dict _tmp191;struct Cyc_Dict_Dict _tmp190;struct Cyc_Dict_Dict _tmp18F;if(((_tmp18E.f1).BottomFL).tag == 1){_LL7: _LL8:
 return({struct _tuple21 _tmp251;_tmp251.f1=outflow,_tmp251.f2=r2;_tmp251;});}else{if(((_tmp18E.f2).BottomFL).tag == 1){_LL9: _LLA:
 return({struct _tuple21 _tmp252;_tmp252.f1=outflow,_tmp252.f2=r1;_tmp252;});}else{if(((_tmp18E.f3).ReachableFL).tag == 2){_LLB: _tmp18F=(((_tmp18E.f1).ReachableFL).val).f1;_tmp190=(((_tmp18E.f2).ReachableFL).val).f1;_tmp191=(((_tmp18E.f3).ReachableFL).val).f1;_tmp192=(((_tmp18E.f3).ReachableFL).val).f2;_LLC: {struct Cyc_Dict_Dict d1=_tmp18F;struct Cyc_Dict_Dict d2=_tmp190;struct Cyc_Dict_Dict outd=_tmp191;struct Cyc_List_List*relns=_tmp192;
# 935
if(({({int(*_tmp2AA)(int ignore,void*r1,void*r2)=({int(*_tmp193)(int ignore,void*r1,void*r2)=(int(*)(int ignore,void*r1,void*r2))Cyc_CfFlowInfo_absRval_lessthan_approx;_tmp193;});void*_tmp2A9=r1;_tmp2AA(0,_tmp2A9,r2);});}))return({struct _tuple21 _tmp253;_tmp253.f1=outflow,_tmp253.f2=r2;_tmp253;});if(({({int(*_tmp2AC)(int ignore,void*r1,void*r2)=({
int(*_tmp194)(int ignore,void*r1,void*r2)=(int(*)(int ignore,void*r1,void*r2))Cyc_CfFlowInfo_absRval_lessthan_approx;_tmp194;});void*_tmp2AB=r2;_tmp2AC(0,_tmp2AB,r1);});}))return({struct _tuple21 _tmp254;_tmp254.f1=outflow,_tmp254.f2=r1;_tmp254;});{
# 935
struct Cyc_CfFlowInfo_JoinEnv env=({struct Cyc_CfFlowInfo_JoinEnv _tmp256;_tmp256.fenv=fenv,({
# 937
struct Cyc_CfFlowInfo_EscPile*_tmp2AD=({struct Cyc_CfFlowInfo_EscPile*_tmp199=_cycalloc(sizeof(*_tmp199));_tmp199->places=0;_tmp199;});_tmp256.pile=_tmp2AD;}),_tmp256.d1=d1,_tmp256.d2=d2;_tmp256;});
void*outr=({({void*(*_tmp2AF)(struct Cyc_CfFlowInfo_JoinEnv*env,int a,void*r1,void*r2)=({void*(*_tmp198)(struct Cyc_CfFlowInfo_JoinEnv*env,int a,void*r1,void*r2)=(void*(*)(struct Cyc_CfFlowInfo_JoinEnv*env,int a,void*r1,void*r2))Cyc_CfFlowInfo_join_absRval;_tmp198;});void*_tmp2AE=r1;_tmp2AF(& env,0,_tmp2AE,r2);});});
return({struct _tuple21 _tmp255;({union Cyc_CfFlowInfo_FlowInfo _tmp2B0=({union Cyc_CfFlowInfo_FlowInfo(*_tmp195)(struct Cyc_Dict_Dict fd,struct Cyc_List_List*r)=Cyc_CfFlowInfo_ReachableFL;struct Cyc_Dict_Dict _tmp196=({Cyc_CfFlowInfo_escape_these(fenv,env.pile,outd);});struct Cyc_List_List*_tmp197=relns;_tmp195(_tmp196,_tmp197);});_tmp255.f1=_tmp2B0;}),_tmp255.f2=outr;_tmp255;});}}}else{_LLD: _LLE:
# 941
({void*_tmp19A=0U;({int(*_tmp2B2)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp19C)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp19C;});struct _fat_ptr _tmp2B1=({const char*_tmp19B="join_flow_and_rval: BottomFL outflow";_tag_fat(_tmp19B,sizeof(char),37U);});_tmp2B2(_tmp2B1,_tag_fat(_tmp19A,sizeof(void*),0U));});});}}}_LL6:;}}}
# 946
static int Cyc_CfFlowInfo_absRval_lessthan_approx(void*ignore,void*r1,void*r2){
if(r1 == r2)return 1;{struct _tuple13 _stmttmp1F=({struct _tuple13 _tmp258;_tmp258.f1=r1,_tmp258.f2=r2;_tmp258;});struct _tuple13 _tmp19E=_stmttmp1F;struct _fat_ptr _tmp1A4;int _tmp1A3;int _tmp1A2;struct _fat_ptr _tmp1A1;int _tmp1A0;int _tmp19F;void*_tmp1A6;void*_tmp1A5;struct Cyc_CfFlowInfo_Place*_tmp1A8;struct Cyc_CfFlowInfo_Place*_tmp1A7;void*_tmp1AC;struct Cyc_Absyn_Vardecl*_tmp1AB;void*_tmp1AA;struct Cyc_Absyn_Vardecl*_tmp1A9;if(((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp19E.f1)->tag == 8U){if(((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp19E.f2)->tag == 8U){_LL1: _tmp1A9=((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp19E.f1)->f1;_tmp1AA=(void*)((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp19E.f1)->f2;_tmp1AB=((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp19E.f2)->f1;_tmp1AC=(void*)((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp19E.f2)->f2;_LL2: {struct Cyc_Absyn_Vardecl*n1=_tmp1A9;void*r1=_tmp1AA;struct Cyc_Absyn_Vardecl*n2=_tmp1AB;void*r2=_tmp1AC;
# 951
return n1 == n2 &&({Cyc_CfFlowInfo_absRval_lessthan_approx(ignore,r1,r2);});}}else{_LL3: _LL4:
 goto _LL6;}}else{if(((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp19E.f2)->tag == 8U){_LL5: _LL6:
 return 0;}else{if(((struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct*)_tmp19E.f1)->tag == 4U){if(((struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct*)_tmp19E.f2)->tag == 4U){_LL7: _tmp1A7=((struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct*)_tmp19E.f1)->f1;_tmp1A8=((struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct*)_tmp19E.f2)->f1;_LL8: {struct Cyc_CfFlowInfo_Place*p1=_tmp1A7;struct Cyc_CfFlowInfo_Place*p2=_tmp1A8;
return({Cyc_CfFlowInfo_place_cmp(p1,p2);})== 0;}}else{_LL9: _LLA:
 goto _LLC;}}else{if(((struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct*)_tmp19E.f2)->tag == 4U){_LLB: _LLC:
 return 0;}else{if(((struct Cyc_CfFlowInfo_UniquePtr_CfFlowInfo_AbsRVal_struct*)_tmp19E.f1)->tag == 5U){if(((struct Cyc_CfFlowInfo_UniquePtr_CfFlowInfo_AbsRVal_struct*)_tmp19E.f2)->tag == 5U){_LLD: _tmp1A5=(void*)((struct Cyc_CfFlowInfo_UniquePtr_CfFlowInfo_AbsRVal_struct*)_tmp19E.f1)->f1;_tmp1A6=(void*)((struct Cyc_CfFlowInfo_UniquePtr_CfFlowInfo_AbsRVal_struct*)_tmp19E.f2)->f1;_LLE: {void*r1=_tmp1A5;void*r2=_tmp1A6;
return({Cyc_CfFlowInfo_absRval_lessthan_approx(ignore,r1,r2);});}}else{_LLF: _LL10:
 goto _LL12;}}else{if(((struct Cyc_CfFlowInfo_UniquePtr_CfFlowInfo_AbsRVal_struct*)_tmp19E.f2)->tag == 5U){_LL11: _LL12:
 return 0;}else{if(((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp19E.f1)->tag == 6U)switch(*((int*)_tmp19E.f2)){case 6U: _LL13: _tmp19F=(((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp19E.f1)->f1).is_union;_tmp1A0=(((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp19E.f1)->f1).fieldnum;_tmp1A1=((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp19E.f1)->f2;_tmp1A2=(((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp19E.f2)->f1).is_union;_tmp1A3=(((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp19E.f2)->f1).fieldnum;_tmp1A4=((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp19E.f2)->f2;_LL14: {int is_union1=_tmp19F;int fld1=_tmp1A0;struct _fat_ptr d1=_tmp1A1;int is_union2=_tmp1A2;int fld2=_tmp1A3;struct _fat_ptr d2=_tmp1A4;
# 962
if(is_union1 && fld1 != fld2)return 0;if((void**)d1.curr == (void**)d2.curr)
return 1;
# 962
{
# 964
int i=0;
# 962
for(0;(unsigned)i < 
# 964
_get_fat_size(d1,sizeof(void*));++ i){
if(!({({int(*_tmp2B4)(int ignore,void*r1,void*r2)=({int(*_tmp1AD)(int ignore,void*r1,void*r2)=(int(*)(int ignore,void*r1,void*r2))Cyc_CfFlowInfo_absRval_lessthan_approx;_tmp1AD;});void*_tmp2B3=((void**)d1.curr)[i];_tmp2B4(0,_tmp2B3,*((void**)_check_fat_subscript(d2,sizeof(void*),i)));});}))return 0;}}
# 962
return 1;}case 0U: goto _LL15;case 1U: goto _LL17;default: goto _LL1D;}else{switch(*((int*)_tmp19E.f2)){case 0U: _LL15: _LL16:
# 967
 goto _LL18;case 1U: _LL17: _LL18:
 return 0;default: if(((struct Cyc_CfFlowInfo_Esc_CfFlowInfo_AbsRVal_struct*)_tmp19E.f1)->tag == 3U){if(((struct Cyc_CfFlowInfo_Esc_CfFlowInfo_AbsRVal_struct*)_tmp19E.f2)->tag == 3U){_LL19: _LL1A:
 goto _LL0;}else{_LL1B: _LL1C:
 return 0;}}else{_LL1D: _LL1E:
# 977
 goto _LL0;}}}}}}}}}_LL0:;}{
# 979
struct _tuple19 _stmttmp20=({struct _tuple19 _tmp259;({enum Cyc_CfFlowInfo_InitLevel _tmp2B6=({Cyc_CfFlowInfo_initlevel_approx(r1);});_tmp259.f1=_tmp2B6;}),({enum Cyc_CfFlowInfo_InitLevel _tmp2B5=({Cyc_CfFlowInfo_initlevel_approx(r2);});_tmp259.f2=_tmp2B5;});_tmp259;});struct _tuple19 _tmp1AE=_stmttmp20;if(_tmp1AE.f1 == Cyc_CfFlowInfo_AllIL){_LL20: _LL21:
 goto _LL23;}else{if(_tmp1AE.f2 == Cyc_CfFlowInfo_NoneIL){_LL22: _LL23:
 return 1;}else{_LL24: _LL25:
 return 0;}}_LL1F:;}}
# 946
int Cyc_CfFlowInfo_flow_lessthan_approx(union Cyc_CfFlowInfo_FlowInfo f1,union Cyc_CfFlowInfo_FlowInfo f2){
# 994
struct _tuple20 _stmttmp21=({struct _tuple20 _tmp25A;_tmp25A.f1=f1,_tmp25A.f2=f2;_tmp25A;});struct _tuple20 _tmp1B0=_stmttmp21;struct Cyc_List_List*_tmp1B4;struct Cyc_Dict_Dict _tmp1B3;struct Cyc_List_List*_tmp1B2;struct Cyc_Dict_Dict _tmp1B1;if(((_tmp1B0.f1).BottomFL).tag == 1){_LL1: _LL2:
 return 1;}else{if(((_tmp1B0.f2).BottomFL).tag == 1){_LL3: _LL4:
 return 0;}else{_LL5: _tmp1B1=(((_tmp1B0.f1).ReachableFL).val).f1;_tmp1B2=(((_tmp1B0.f1).ReachableFL).val).f2;_tmp1B3=(((_tmp1B0.f2).ReachableFL).val).f1;_tmp1B4=(((_tmp1B0.f2).ReachableFL).val).f2;_LL6: {struct Cyc_Dict_Dict d1=_tmp1B1;struct Cyc_List_List*r1=_tmp1B2;struct Cyc_Dict_Dict d2=_tmp1B3;struct Cyc_List_List*r2=_tmp1B4;
# 998
if(d1.t == d2.t && r1 == r2)return 1;return
({Cyc_Dict_forall_intersect(Cyc_CfFlowInfo_absRval_lessthan_approx,d1,d2);})&&({Cyc_Relations_relns_approx(r1,r2);});}}}_LL0:;}struct _tuple23{void*f1;struct Cyc_List_List*f2;};
# 1004
struct _tuple23 Cyc_CfFlowInfo_unname_rval(void*rv){
struct Cyc_List_List*names=0;
int done=0;
while(!done){
void*_tmp1B6=rv;void*_tmp1B8;struct Cyc_Absyn_Vardecl*_tmp1B7;if(((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp1B6)->tag == 8U){_LL1: _tmp1B7=((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp1B6)->f1;_tmp1B8=(void*)((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp1B6)->f2;_LL2: {struct Cyc_Absyn_Vardecl*vd=_tmp1B7;void*rv2=_tmp1B8;
# 1010
names=({struct Cyc_List_List*_tmp1B9=_cycalloc(sizeof(*_tmp1B9));_tmp1B9->hd=vd,_tmp1B9->tl=names;_tmp1B9;});rv=rv2;goto _LL0;}}else{_LL3: _LL4:
# 1012
 done=1;goto _LL0;}_LL0:;}
# 1015
return({struct _tuple23 _tmp25B;_tmp25B.f1=rv,_tmp25B.f2=names;_tmp25B;});}
# 1018
void Cyc_CfFlowInfo_print_initlevel(enum Cyc_CfFlowInfo_InitLevel il){
enum Cyc_CfFlowInfo_InitLevel _tmp1BB=il;if(_tmp1BB == Cyc_CfFlowInfo_NoneIL){_LL1: _LL2:
({void*_tmp1BC=0U;({struct Cyc___cycFILE*_tmp2B8=Cyc_stderr;struct _fat_ptr _tmp2B7=({const char*_tmp1BD="uninitialized";_tag_fat(_tmp1BD,sizeof(char),14U);});Cyc_fprintf(_tmp2B8,_tmp2B7,_tag_fat(_tmp1BC,sizeof(void*),0U));});});goto _LL0;}else{_LL3: _LL4:
({void*_tmp1BE=0U;({struct Cyc___cycFILE*_tmp2BA=Cyc_stderr;struct _fat_ptr _tmp2B9=({const char*_tmp1BF="all-initialized";_tag_fat(_tmp1BF,sizeof(char),16U);});Cyc_fprintf(_tmp2BA,_tmp2B9,_tag_fat(_tmp1BE,sizeof(void*),0U));});});goto _LL0;}_LL0:;}
# 1025
void Cyc_CfFlowInfo_print_root(void*root){
void*_tmp1C1=root;void*_tmp1C3;struct Cyc_Absyn_Exp*_tmp1C2;struct Cyc_Absyn_Vardecl*_tmp1C4;switch(*((int*)_tmp1C1)){case 0U: _LL1: _tmp1C4=((struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct*)_tmp1C1)->f1;_LL2: {struct Cyc_Absyn_Vardecl*vd=_tmp1C4;
# 1028
({struct Cyc_String_pa_PrintArg_struct _tmp1C7=({struct Cyc_String_pa_PrintArg_struct _tmp25C;_tmp25C.tag=0U,({struct _fat_ptr _tmp2BB=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(vd->name);}));_tmp25C.f1=_tmp2BB;});_tmp25C;});void*_tmp1C5[1U];_tmp1C5[0]=& _tmp1C7;({struct Cyc___cycFILE*_tmp2BD=Cyc_stderr;struct _fat_ptr _tmp2BC=({const char*_tmp1C6="Root(%s)";_tag_fat(_tmp1C6,sizeof(char),9U);});Cyc_fprintf(_tmp2BD,_tmp2BC,_tag_fat(_tmp1C5,sizeof(void*),1U));});});goto _LL0;}case 1U: _LL3: _tmp1C2=((struct Cyc_CfFlowInfo_MallocPt_CfFlowInfo_Root_struct*)_tmp1C1)->f1;_tmp1C3=(void*)((struct Cyc_CfFlowInfo_MallocPt_CfFlowInfo_Root_struct*)_tmp1C1)->f2;_LL4: {struct Cyc_Absyn_Exp*e=_tmp1C2;void*t=_tmp1C3;
# 1030
({struct Cyc_String_pa_PrintArg_struct _tmp1CA=({struct Cyc_String_pa_PrintArg_struct _tmp25E;_tmp25E.tag=0U,({
struct _fat_ptr _tmp2BE=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_exp2string(e);}));_tmp25E.f1=_tmp2BE;});_tmp25E;});struct Cyc_String_pa_PrintArg_struct _tmp1CB=({struct Cyc_String_pa_PrintArg_struct _tmp25D;_tmp25D.tag=0U,({struct _fat_ptr _tmp2BF=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t);}));_tmp25D.f1=_tmp2BF;});_tmp25D;});void*_tmp1C8[2U];_tmp1C8[0]=& _tmp1CA,_tmp1C8[1]=& _tmp1CB;({struct Cyc___cycFILE*_tmp2C1=Cyc_stderr;struct _fat_ptr _tmp2C0=({const char*_tmp1C9="MallocPt(%s,%s)";_tag_fat(_tmp1C9,sizeof(char),16U);});Cyc_fprintf(_tmp2C1,_tmp2C0,_tag_fat(_tmp1C8,sizeof(void*),2U));});});goto _LL0;}default: _LL5: _LL6:
# 1033
({void*_tmp1CC=0U;({struct Cyc___cycFILE*_tmp2C3=Cyc_stderr;struct _fat_ptr _tmp2C2=({const char*_tmp1CD="InitParam(_,_)";_tag_fat(_tmp1CD,sizeof(char),15U);});Cyc_fprintf(_tmp2C3,_tmp2C2,_tag_fat(_tmp1CC,sizeof(void*),0U));});});goto _LL0;}_LL0:;}
# 1037
void Cyc_CfFlowInfo_print_path(struct Cyc_List_List*p){
{struct Cyc_List_List*x=p;for(0;x != 0;x=((struct Cyc_List_List*)_check_null(x))->tl){
void*_stmttmp22=(void*)x->hd;void*_tmp1CF=_stmttmp22;int _tmp1D0;if(((struct Cyc_CfFlowInfo_Dot_CfFlowInfo_PathCon_struct*)_tmp1CF)->tag == 0U){_LL1: _tmp1D0=((struct Cyc_CfFlowInfo_Dot_CfFlowInfo_PathCon_struct*)_tmp1CF)->f1;_LL2: {int i=_tmp1D0;
# 1041
({struct Cyc_Int_pa_PrintArg_struct _tmp1D3=({struct Cyc_Int_pa_PrintArg_struct _tmp25F;_tmp25F.tag=1U,_tmp25F.f1=(unsigned long)i;_tmp25F;});void*_tmp1D1[1U];_tmp1D1[0]=& _tmp1D3;({struct Cyc___cycFILE*_tmp2C5=Cyc_stderr;struct _fat_ptr _tmp2C4=({const char*_tmp1D2=".%d";_tag_fat(_tmp1D2,sizeof(char),4U);});Cyc_fprintf(_tmp2C5,_tmp2C4,_tag_fat(_tmp1D1,sizeof(void*),1U));});});
goto _LL0;}}else{_LL3: _LL4:
# 1044
 if(x->tl != 0){
void*_stmttmp23=(void*)((struct Cyc_List_List*)_check_null(x->tl))->hd;void*_tmp1D4=_stmttmp23;int _tmp1D5;if(((struct Cyc_CfFlowInfo_Dot_CfFlowInfo_PathCon_struct*)_tmp1D4)->tag == 0U){_LL6: _tmp1D5=((struct Cyc_CfFlowInfo_Dot_CfFlowInfo_PathCon_struct*)_tmp1D4)->f1;_LL7: {int i=_tmp1D5;
# 1047
x=x->tl;
({struct Cyc_Int_pa_PrintArg_struct _tmp1D8=({struct Cyc_Int_pa_PrintArg_struct _tmp260;_tmp260.tag=1U,_tmp260.f1=(unsigned long)i;_tmp260;});void*_tmp1D6[1U];_tmp1D6[0]=& _tmp1D8;({struct Cyc___cycFILE*_tmp2C7=Cyc_stderr;struct _fat_ptr _tmp2C6=({const char*_tmp1D7="->%d";_tag_fat(_tmp1D7,sizeof(char),5U);});Cyc_fprintf(_tmp2C7,_tmp2C6,_tag_fat(_tmp1D6,sizeof(void*),1U));});});
continue;}}else{_LL8: _LL9:
 goto _LL5;}_LL5:;}
# 1044
({void*_tmp1D9=0U;({struct Cyc___cycFILE*_tmp2C9=Cyc_stderr;struct _fat_ptr _tmp2C8=({const char*_tmp1DA="*";_tag_fat(_tmp1DA,sizeof(char),2U);});Cyc_fprintf(_tmp2C9,_tmp2C8,_tag_fat(_tmp1D9,sizeof(void*),0U));});});}_LL0:;}}
# 1056
({void*_tmp1DB=0U;({struct Cyc___cycFILE*_tmp2CB=Cyc_stderr;struct _fat_ptr _tmp2CA=({const char*_tmp1DC=" ";_tag_fat(_tmp1DC,sizeof(char),2U);});Cyc_fprintf(_tmp2CB,_tmp2CA,_tag_fat(_tmp1DB,sizeof(void*),0U));});});}
# 1059
void Cyc_CfFlowInfo_print_place(struct Cyc_CfFlowInfo_Place*p){
({Cyc_CfFlowInfo_print_root(p->root);});
({Cyc_CfFlowInfo_print_path(p->path);});}
# 1064
void Cyc_CfFlowInfo_print_list(struct Cyc_List_List*x,void(*pr)(void*)){
int first=1;
while(x != 0){
if(!first){({void*_tmp1DF=0U;({struct Cyc___cycFILE*_tmp2CD=Cyc_stderr;struct _fat_ptr _tmp2CC=({const char*_tmp1E0=", ";_tag_fat(_tmp1E0,sizeof(char),3U);});Cyc_fprintf(_tmp2CD,_tmp2CC,_tag_fat(_tmp1DF,sizeof(void*),0U));});});first=0;}({pr(x->hd);});
# 1069
x=x->tl;}
# 1071
({void*_tmp1E1=0U;({struct Cyc___cycFILE*_tmp2CF=Cyc_stderr;struct _fat_ptr _tmp2CE=({const char*_tmp1E2="\n";_tag_fat(_tmp1E2,sizeof(char),2U);});Cyc_fprintf(_tmp2CF,_tmp2CE,_tag_fat(_tmp1E1,sizeof(void*),0U));});});}
# 1074
void Cyc_CfFlowInfo_print_absrval(void*rval){
void*_tmp1E4=rval;void*_tmp1E6;struct Cyc_Absyn_Vardecl*_tmp1E5;void*_tmp1E9;int _tmp1E8;struct Cyc_Absyn_Exp*_tmp1E7;struct _fat_ptr _tmp1EC;int _tmp1EB;int _tmp1EA;void*_tmp1ED;struct Cyc_CfFlowInfo_Place*_tmp1EE;enum Cyc_CfFlowInfo_InitLevel _tmp1EF;enum Cyc_CfFlowInfo_InitLevel _tmp1F0;switch(*((int*)_tmp1E4)){case 0U: _LL1: _LL2:
({void*_tmp1F1=0U;({struct Cyc___cycFILE*_tmp2D1=Cyc_stderr;struct _fat_ptr _tmp2D0=({const char*_tmp1F2="Zero";_tag_fat(_tmp1F2,sizeof(char),5U);});Cyc_fprintf(_tmp2D1,_tmp2D0,_tag_fat(_tmp1F1,sizeof(void*),0U));});});goto _LL0;case 1U: _LL3: _LL4:
({void*_tmp1F3=0U;({struct Cyc___cycFILE*_tmp2D3=Cyc_stderr;struct _fat_ptr _tmp2D2=({const char*_tmp1F4="NotZeroAll";_tag_fat(_tmp1F4,sizeof(char),11U);});Cyc_fprintf(_tmp2D3,_tmp2D2,_tag_fat(_tmp1F3,sizeof(void*),0U));});});goto _LL0;case 2U: _LL5: _tmp1F0=((struct Cyc_CfFlowInfo_UnknownR_CfFlowInfo_AbsRVal_struct*)_tmp1E4)->f1;_LL6: {enum Cyc_CfFlowInfo_InitLevel il=_tmp1F0;
({void*_tmp1F5=0U;({struct Cyc___cycFILE*_tmp2D5=Cyc_stderr;struct _fat_ptr _tmp2D4=({const char*_tmp1F6="Unknown(";_tag_fat(_tmp1F6,sizeof(char),9U);});Cyc_fprintf(_tmp2D5,_tmp2D4,_tag_fat(_tmp1F5,sizeof(void*),0U));});});({Cyc_CfFlowInfo_print_initlevel(il);});
({void*_tmp1F7=0U;({struct Cyc___cycFILE*_tmp2D7=Cyc_stderr;struct _fat_ptr _tmp2D6=({const char*_tmp1F8=")";_tag_fat(_tmp1F8,sizeof(char),2U);});Cyc_fprintf(_tmp2D7,_tmp2D6,_tag_fat(_tmp1F7,sizeof(void*),0U));});});goto _LL0;}case 3U: _LL7: _tmp1EF=((struct Cyc_CfFlowInfo_Esc_CfFlowInfo_AbsRVal_struct*)_tmp1E4)->f1;_LL8: {enum Cyc_CfFlowInfo_InitLevel il=_tmp1EF;
({void*_tmp1F9=0U;({struct Cyc___cycFILE*_tmp2D9=Cyc_stderr;struct _fat_ptr _tmp2D8=({const char*_tmp1FA="Esc(";_tag_fat(_tmp1FA,sizeof(char),5U);});Cyc_fprintf(_tmp2D9,_tmp2D8,_tag_fat(_tmp1F9,sizeof(void*),0U));});});({Cyc_CfFlowInfo_print_initlevel(il);});
({void*_tmp1FB=0U;({struct Cyc___cycFILE*_tmp2DB=Cyc_stderr;struct _fat_ptr _tmp2DA=({const char*_tmp1FC=")";_tag_fat(_tmp1FC,sizeof(char),2U);});Cyc_fprintf(_tmp2DB,_tmp2DA,_tag_fat(_tmp1FB,sizeof(void*),0U));});});goto _LL0;}case 4U: _LL9: _tmp1EE=((struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct*)_tmp1E4)->f1;_LLA: {struct Cyc_CfFlowInfo_Place*p=_tmp1EE;
({void*_tmp1FD=0U;({struct Cyc___cycFILE*_tmp2DD=Cyc_stderr;struct _fat_ptr _tmp2DC=({const char*_tmp1FE="AddrOf(";_tag_fat(_tmp1FE,sizeof(char),8U);});Cyc_fprintf(_tmp2DD,_tmp2DC,_tag_fat(_tmp1FD,sizeof(void*),0U));});});({Cyc_CfFlowInfo_print_place(p);});
({void*_tmp1FF=0U;({struct Cyc___cycFILE*_tmp2DF=Cyc_stderr;struct _fat_ptr _tmp2DE=({const char*_tmp200=")";_tag_fat(_tmp200,sizeof(char),2U);});Cyc_fprintf(_tmp2DF,_tmp2DE,_tag_fat(_tmp1FF,sizeof(void*),0U));});});goto _LL0;}case 5U: _LLB: _tmp1ED=(void*)((struct Cyc_CfFlowInfo_UniquePtr_CfFlowInfo_AbsRVal_struct*)_tmp1E4)->f1;_LLC: {void*r=_tmp1ED;
({void*_tmp201=0U;({struct Cyc___cycFILE*_tmp2E1=Cyc_stderr;struct _fat_ptr _tmp2E0=({const char*_tmp202="UniquePtr(";_tag_fat(_tmp202,sizeof(char),11U);});Cyc_fprintf(_tmp2E1,_tmp2E0,_tag_fat(_tmp201,sizeof(void*),0U));});});({Cyc_CfFlowInfo_print_absrval(r);});
({void*_tmp203=0U;({struct Cyc___cycFILE*_tmp2E3=Cyc_stderr;struct _fat_ptr _tmp2E2=({const char*_tmp204=")";_tag_fat(_tmp204,sizeof(char),2U);});Cyc_fprintf(_tmp2E3,_tmp2E2,_tag_fat(_tmp203,sizeof(void*),0U));});});goto _LL0;}case 6U: _LLD: _tmp1EA=(((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp1E4)->f1).is_union;_tmp1EB=(((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp1E4)->f1).fieldnum;_tmp1EC=((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp1E4)->f2;_LLE: {int is_union=_tmp1EA;int f=_tmp1EB;struct _fat_ptr d=_tmp1EC;
# 1087
if(is_union){
({void*_tmp205=0U;({struct Cyc___cycFILE*_tmp2E5=Cyc_stderr;struct _fat_ptr _tmp2E4=({const char*_tmp206="AggrUnion{";_tag_fat(_tmp206,sizeof(char),11U);});Cyc_fprintf(_tmp2E5,_tmp2E4,_tag_fat(_tmp205,sizeof(void*),0U));});});
if(f != -1)
({struct Cyc_Int_pa_PrintArg_struct _tmp209=({struct Cyc_Int_pa_PrintArg_struct _tmp261;_tmp261.tag=1U,_tmp261.f1=(unsigned long)f;_tmp261;});void*_tmp207[1U];_tmp207[0]=& _tmp209;({struct Cyc___cycFILE*_tmp2E7=Cyc_stderr;struct _fat_ptr _tmp2E6=({const char*_tmp208="tag == %d;";_tag_fat(_tmp208,sizeof(char),11U);});Cyc_fprintf(_tmp2E7,_tmp2E6,_tag_fat(_tmp207,sizeof(void*),1U));});});}else{
# 1092
({void*_tmp20A=0U;({struct Cyc___cycFILE*_tmp2E9=Cyc_stderr;struct _fat_ptr _tmp2E8=({const char*_tmp20B="AggrStruct{";_tag_fat(_tmp20B,sizeof(char),12U);});Cyc_fprintf(_tmp2E9,_tmp2E8,_tag_fat(_tmp20A,sizeof(void*),0U));});});}
{int i=0;for(0;(unsigned)i < _get_fat_size(d,sizeof(void*));++ i){
({Cyc_CfFlowInfo_print_absrval(((void**)d.curr)[i]);});
if((unsigned)(i + 1)< _get_fat_size(d,sizeof(void*)))({void*_tmp20C=0U;({struct Cyc___cycFILE*_tmp2EB=Cyc_stderr;struct _fat_ptr _tmp2EA=({const char*_tmp20D=",";_tag_fat(_tmp20D,sizeof(char),2U);});Cyc_fprintf(_tmp2EB,_tmp2EA,_tag_fat(_tmp20C,sizeof(void*),0U));});});}}
# 1097
({void*_tmp20E=0U;({struct Cyc___cycFILE*_tmp2ED=Cyc_stderr;struct _fat_ptr _tmp2EC=({const char*_tmp20F="}";_tag_fat(_tmp20F,sizeof(char),2U);});Cyc_fprintf(_tmp2ED,_tmp2EC,_tag_fat(_tmp20E,sizeof(void*),0U));});});
goto _LL0;}case 7U: _LLF: _tmp1E7=((struct Cyc_CfFlowInfo_Consumed_CfFlowInfo_AbsRVal_struct*)_tmp1E4)->f1;_tmp1E8=((struct Cyc_CfFlowInfo_Consumed_CfFlowInfo_AbsRVal_struct*)_tmp1E4)->f2;_tmp1E9=(void*)((struct Cyc_CfFlowInfo_Consumed_CfFlowInfo_AbsRVal_struct*)_tmp1E4)->f3;_LL10: {struct Cyc_Absyn_Exp*e=_tmp1E7;int i=_tmp1E8;void*r=_tmp1E9;
# 1100
({struct Cyc_String_pa_PrintArg_struct _tmp212=({struct Cyc_String_pa_PrintArg_struct _tmp263;_tmp263.tag=0U,({struct _fat_ptr _tmp2EE=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_exp2string(e);}));_tmp263.f1=_tmp2EE;});_tmp263;});struct Cyc_Int_pa_PrintArg_struct _tmp213=({struct Cyc_Int_pa_PrintArg_struct _tmp262;_tmp262.tag=1U,_tmp262.f1=(unsigned long)i;_tmp262;});void*_tmp210[2U];_tmp210[0]=& _tmp212,_tmp210[1]=& _tmp213;({struct Cyc___cycFILE*_tmp2F0=Cyc_stderr;struct _fat_ptr _tmp2EF=({const char*_tmp211="[Consumed(%s,%d,";_tag_fat(_tmp211,sizeof(char),17U);});Cyc_fprintf(_tmp2F0,_tmp2EF,_tag_fat(_tmp210,sizeof(void*),2U));});});
({Cyc_CfFlowInfo_print_absrval(r);});({void*_tmp214=0U;({struct Cyc___cycFILE*_tmp2F2=Cyc_stderr;struct _fat_ptr _tmp2F1=({const char*_tmp215=")]";_tag_fat(_tmp215,sizeof(char),3U);});Cyc_fprintf(_tmp2F2,_tmp2F1,_tag_fat(_tmp214,sizeof(void*),0U));});});
goto _LL0;}default: _LL11: _tmp1E5=((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp1E4)->f1;_tmp1E6=(void*)((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp1E4)->f2;_LL12: {struct Cyc_Absyn_Vardecl*n=_tmp1E5;void*r=_tmp1E6;
# 1104
({struct Cyc_String_pa_PrintArg_struct _tmp218=({struct Cyc_String_pa_PrintArg_struct _tmp264;_tmp264.tag=0U,({struct _fat_ptr _tmp2F3=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(n->name);}));_tmp264.f1=_tmp2F3;});_tmp264;});void*_tmp216[1U];_tmp216[0]=& _tmp218;({struct Cyc___cycFILE*_tmp2F5=Cyc_stderr;struct _fat_ptr _tmp2F4=({const char*_tmp217="[NamedLocation(%s,";_tag_fat(_tmp217,sizeof(char),19U);});Cyc_fprintf(_tmp2F5,_tmp2F4,_tag_fat(_tmp216,sizeof(void*),1U));});});
({Cyc_CfFlowInfo_print_absrval(r);});({void*_tmp219=0U;({struct Cyc___cycFILE*_tmp2F7=Cyc_stderr;struct _fat_ptr _tmp2F6=({const char*_tmp21A=")]";_tag_fat(_tmp21A,sizeof(char),3U);});Cyc_fprintf(_tmp2F7,_tmp2F6,_tag_fat(_tmp219,sizeof(void*),0U));});});
goto _LL0;}}_LL0:;}
# 1110
static void Cyc_CfFlowInfo_print_flow_mapping(void*root,void*rval){
({void*_tmp21C=0U;({struct Cyc___cycFILE*_tmp2F9=Cyc_stderr;struct _fat_ptr _tmp2F8=({const char*_tmp21D="    ";_tag_fat(_tmp21D,sizeof(char),5U);});Cyc_fprintf(_tmp2F9,_tmp2F8,_tag_fat(_tmp21C,sizeof(void*),0U));});});
({Cyc_CfFlowInfo_print_root(root);});
({void*_tmp21E=0U;({struct Cyc___cycFILE*_tmp2FB=Cyc_stderr;struct _fat_ptr _tmp2FA=({const char*_tmp21F=" --> ";_tag_fat(_tmp21F,sizeof(char),6U);});Cyc_fprintf(_tmp2FB,_tmp2FA,_tag_fat(_tmp21E,sizeof(void*),0U));});});
({Cyc_CfFlowInfo_print_absrval(rval);});
({void*_tmp220=0U;({struct Cyc___cycFILE*_tmp2FD=Cyc_stderr;struct _fat_ptr _tmp2FC=({const char*_tmp221="\n";_tag_fat(_tmp221,sizeof(char),2U);});Cyc_fprintf(_tmp2FD,_tmp2FC,_tag_fat(_tmp220,sizeof(void*),0U));});});}
# 1118
void Cyc_CfFlowInfo_print_flowdict(struct Cyc_Dict_Dict d){
({Cyc_Dict_iter(Cyc_CfFlowInfo_print_flow_mapping,d);});}
# 1122
void Cyc_CfFlowInfo_print_flow(union Cyc_CfFlowInfo_FlowInfo f){
union Cyc_CfFlowInfo_FlowInfo _tmp224=f;struct Cyc_List_List*_tmp226;struct Cyc_Dict_Dict _tmp225;if((_tmp224.BottomFL).tag == 1){_LL1: _LL2:
({void*_tmp227=0U;({struct Cyc___cycFILE*_tmp2FF=Cyc_stderr;struct _fat_ptr _tmp2FE=({const char*_tmp228="  BottomFL\n";_tag_fat(_tmp228,sizeof(char),12U);});Cyc_fprintf(_tmp2FF,_tmp2FE,_tag_fat(_tmp227,sizeof(void*),0U));});});goto _LL0;}else{_LL3: _tmp225=((_tmp224.ReachableFL).val).f1;_tmp226=((_tmp224.ReachableFL).val).f2;_LL4: {struct Cyc_Dict_Dict fd=_tmp225;struct Cyc_List_List*rlns=_tmp226;
# 1126
({void*_tmp229=0U;({struct Cyc___cycFILE*_tmp301=Cyc_stderr;struct _fat_ptr _tmp300=({const char*_tmp22A="  ReachableFL:\n";_tag_fat(_tmp22A,sizeof(char),16U);});Cyc_fprintf(_tmp301,_tmp300,_tag_fat(_tmp229,sizeof(void*),0U));});});
({Cyc_CfFlowInfo_print_flowdict(fd);});
({void*_tmp22B=0U;({struct Cyc___cycFILE*_tmp303=Cyc_stderr;struct _fat_ptr _tmp302=({const char*_tmp22C="\n  Relations: ";_tag_fat(_tmp22C,sizeof(char),15U);});Cyc_fprintf(_tmp303,_tmp302,_tag_fat(_tmp22B,sizeof(void*),0U));});});
({Cyc_Relations_print_relns(Cyc_stderr,rlns);});
({void*_tmp22D=0U;({struct Cyc___cycFILE*_tmp305=Cyc_stderr;struct _fat_ptr _tmp304=({const char*_tmp22E="\n";_tag_fat(_tmp22E,sizeof(char),2U);});Cyc_fprintf(_tmp305,_tmp304,_tag_fat(_tmp22D,sizeof(void*),0U));});});
goto _LL0;}}_LL0:;}

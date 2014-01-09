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
# 72 "list.h"
struct Cyc_List_List*Cyc_List_rcopy(struct _RegionHandle*,struct Cyc_List_List*x);extern char Cyc_List_List_mismatch[14U];struct Cyc_List_List_mismatch_exn_struct{char*tag;};
# 178
struct Cyc_List_List*Cyc_List_imp_rev(struct Cyc_List_List*x);
# 184
struct Cyc_List_List*Cyc_List_append(struct Cyc_List_List*x,struct Cyc_List_List*y);extern char Cyc_List_Nth[4U];struct Cyc_List_Nth_exn_struct{char*tag;};struct Cyc_Iter_Iter{void*env;int(*next)(void*env,void*dest);};
# 37 "iter.h"
int Cyc_Iter_next(struct Cyc_Iter_Iter,void*);struct Cyc_Set_Set;
# 54 "set.h"
struct Cyc_Set_Set*Cyc_Set_rempty(struct _RegionHandle*r,int(*cmp)(void*,void*));
# 69
struct Cyc_Set_Set*Cyc_Set_rinsert(struct _RegionHandle*r,struct Cyc_Set_Set*s,void*elt);extern char Cyc_Set_Absent[7U];struct Cyc_Set_Absent_exn_struct{char*tag;};
# 141
struct Cyc_Iter_Iter Cyc_Set_make_iter(struct _RegionHandle*rgn,struct Cyc_Set_Set*s);struct Cyc___cycFILE;
# 53 "cycboot.h"
extern struct Cyc___cycFILE*Cyc_stderr;struct Cyc_String_pa_PrintArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Int_pa_PrintArg_struct{int tag;unsigned long f1;};struct Cyc_Double_pa_PrintArg_struct{int tag;double f1;};struct Cyc_LongDouble_pa_PrintArg_struct{int tag;long double f1;};struct Cyc_ShortPtr_pa_PrintArg_struct{int tag;short*f1;};struct Cyc_IntPtr_pa_PrintArg_struct{int tag;unsigned long*f1;};
# 73
struct _fat_ptr Cyc_aprintf(struct _fat_ptr,struct _fat_ptr);
# 88
int Cyc_fflush(struct Cyc___cycFILE*);
# 100
int Cyc_fprintf(struct Cyc___cycFILE*,struct _fat_ptr,struct _fat_ptr);struct Cyc_ShortPtr_sa_ScanfArg_struct{int tag;short*f1;};struct Cyc_UShortPtr_sa_ScanfArg_struct{int tag;unsigned short*f1;};struct Cyc_IntPtr_sa_ScanfArg_struct{int tag;int*f1;};struct Cyc_UIntPtr_sa_ScanfArg_struct{int tag;unsigned*f1;};struct Cyc_StringPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_DoublePtr_sa_ScanfArg_struct{int tag;double*f1;};struct Cyc_FloatPtr_sa_ScanfArg_struct{int tag;float*f1;};struct Cyc_CharPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};extern char Cyc_FileCloseError[15U];struct Cyc_FileCloseError_exn_struct{char*tag;};extern char Cyc_FileOpenError[14U];struct Cyc_FileOpenError_exn_struct{char*tag;struct _fat_ptr f1;};struct Cyc_Dict_T;struct Cyc_Dict_Dict{int(*rel)(void*,void*);struct _RegionHandle*r;const struct Cyc_Dict_T*t;};extern char Cyc_Dict_Present[8U];struct Cyc_Dict_Present_exn_struct{char*tag;};extern char Cyc_Dict_Absent[7U];struct Cyc_Dict_Absent_exn_struct{char*tag;};
# 68 "dict.h"
struct Cyc_Dict_Dict Cyc_Dict_rempty(struct _RegionHandle*,int(*cmp)(void*,void*));
# 83
int Cyc_Dict_member(struct Cyc_Dict_Dict d,void*k);
# 87
struct Cyc_Dict_Dict Cyc_Dict_insert(struct Cyc_Dict_Dict d,void*k,void*v);
# 110
void*Cyc_Dict_lookup(struct Cyc_Dict_Dict d,void*k);
# 277 "dict.h"
struct Cyc_Dict_Dict Cyc_Dict_rdelete_same(struct Cyc_Dict_Dict,void*);
# 50 "string.h"
int Cyc_strptrcmp(struct _fat_ptr*s1,struct _fat_ptr*s2);struct Cyc_PP_Ppstate;struct Cyc_PP_Out;struct Cyc_PP_Doc;struct Cyc_Position_Error;struct Cyc_Relations_Reln;struct _union_Nmspace_Rel_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Abs_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_C_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Loc_n{int tag;int val;};union Cyc_Absyn_Nmspace{struct _union_Nmspace_Rel_n Rel_n;struct _union_Nmspace_Abs_n Abs_n;struct _union_Nmspace_C_n C_n;struct _union_Nmspace_Loc_n Loc_n;};struct _tuple0{union Cyc_Absyn_Nmspace f1;struct _fat_ptr*f2;};
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
# 954 "absyn.h"
int Cyc_Absyn_qvar_cmp(struct _tuple0*,struct _tuple0*);
# 962
struct Cyc_Absyn_Tqual Cyc_Absyn_const_tqual(unsigned);
struct Cyc_Absyn_Tqual Cyc_Absyn_empty_tqual(unsigned);
# 997
extern void*Cyc_Absyn_void_type;
# 1012
extern struct _tuple0*Cyc_Absyn_exn_name;
# 1040
void*Cyc_Absyn_cstar_type(void*,struct Cyc_Absyn_Tqual);
# 1058
struct Cyc_Absyn_Exp*Cyc_Absyn_new_exp(void*,unsigned);
# 1064
struct Cyc_Absyn_Exp*Cyc_Absyn_true_exp(unsigned);
# 1068
struct Cyc_Absyn_Exp*Cyc_Absyn_uint_exp(unsigned,unsigned);
# 1074
struct Cyc_Absyn_Exp*Cyc_Absyn_var_exp(struct _tuple0*,unsigned);
# 1092
struct Cyc_Absyn_Exp*Cyc_Absyn_assign_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
# 1094
struct Cyc_Absyn_Exp*Cyc_Absyn_conditional_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
# 1097
struct Cyc_Absyn_Exp*Cyc_Absyn_seq_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
# 1099
struct Cyc_Absyn_Exp*Cyc_Absyn_fncall_exp(struct Cyc_Absyn_Exp*,struct Cyc_List_List*,unsigned);
# 1105
struct Cyc_Absyn_Exp*Cyc_Absyn_address_exp(struct Cyc_Absyn_Exp*,unsigned);
# 1109
struct Cyc_Absyn_Exp*Cyc_Absyn_deref_exp(struct Cyc_Absyn_Exp*,unsigned);
# 1112
struct Cyc_Absyn_Exp*Cyc_Absyn_subscript_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
# 1127
struct Cyc_Absyn_Stmt*Cyc_Absyn_new_stmt(void*,unsigned);
# 1129
struct Cyc_Absyn_Stmt*Cyc_Absyn_exp_stmt(struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Stmt*Cyc_Absyn_seq_stmt(struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt*,unsigned);
# 1140
struct Cyc_Absyn_Stmt*Cyc_Absyn_decl_stmt(struct Cyc_Absyn_Decl*,struct Cyc_Absyn_Stmt*,unsigned);
# 1153
struct Cyc_Absyn_Decl*Cyc_Absyn_new_decl(void*,unsigned);
# 1231
struct _tuple0*Cyc_Absyn_binding2qvar(void*);
# 1400
extern const int Cyc_Absyn_noreentrant;struct Cyc_Absynpp_Params{int expand_typedefs;int qvar_to_Cids;int add_cyc_prefix;int to_VC;int decls_first;int rewrite_temp_tvars;int print_all_tvars;int print_all_kinds;int print_all_effects;int print_using_stmts;int print_externC_stmts;int print_full_evars;int print_zeroterm;int generate_line_directives;int use_curr_namespace;struct Cyc_List_List*curr_namespace;};
# 67 "absynpp.h"
struct _fat_ptr Cyc_Absynpp_exp2string(struct Cyc_Absyn_Exp*);
struct _fat_ptr Cyc_Absynpp_stmt2string(struct Cyc_Absyn_Stmt*);struct _union_RelnOp_RConst{int tag;unsigned val;};struct _union_RelnOp_RVar{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RNumelts{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RType{int tag;void*val;};struct _union_RelnOp_RParam{int tag;unsigned val;};struct _union_RelnOp_RParamNumelts{int tag;unsigned val;};struct _union_RelnOp_RReturn{int tag;unsigned val;};union Cyc_Relations_RelnOp{struct _union_RelnOp_RConst RConst;struct _union_RelnOp_RVar RVar;struct _union_RelnOp_RNumelts RNumelts;struct _union_RelnOp_RType RType;struct _union_RelnOp_RParam RParam;struct _union_RelnOp_RParamNumelts RParamNumelts;struct _union_RelnOp_RReturn RReturn;};
# 50 "relations-ap.h"
enum Cyc_Relations_Relation{Cyc_Relations_Req =0U,Cyc_Relations_Rneq =1U,Cyc_Relations_Rlte =2U,Cyc_Relations_Rlt =3U};struct Cyc_Relations_Reln{union Cyc_Relations_RelnOp rop1;enum Cyc_Relations_Relation relation;union Cyc_Relations_RelnOp rop2;};struct Cyc_RgnOrder_RgnPO;extern char Cyc_Tcenv_Env_error[10U];struct Cyc_Tcenv_Env_error_exn_struct{char*tag;};struct Cyc_Tcenv_Genv{struct Cyc_Dict_Dict aggrdecls;struct Cyc_Dict_Dict datatypedecls;struct Cyc_Dict_Dict enumdecls;struct Cyc_Dict_Dict typedefs;struct Cyc_Dict_Dict ordinaries;};struct Cyc_Tcenv_Fenv;struct Cyc_Tcenv_Tenv{struct Cyc_List_List*ns;struct Cyc_Tcenv_Genv*ae;struct Cyc_Tcenv_Fenv*le;int allow_valueof: 1;int in_extern_c_include: 1;int in_tempest: 1;int tempest_generalize: 1;};
# 90 "tcenv.h"
enum Cyc_Tcenv_NewStatus{Cyc_Tcenv_NoneNew =0U,Cyc_Tcenv_InNew =1U,Cyc_Tcenv_InNewAggr =2U};
# 32 "tcutil.h"
void Cyc_Tcutil_warn(unsigned,struct _fat_ptr,struct _fat_ptr);
# 110
void*Cyc_Tcutil_compress(void*);
# 206
void*Cyc_Tcutil_fndecl2type(struct Cyc_Absyn_Fndecl*);struct Cyc_Hashtable_Table;
# 40 "toc.h"
struct _tuple0*Cyc_Toc_temp_var();
extern struct _fat_ptr Cyc_Toc_globals;extern char Cyc_Toc_Dest[5U];struct Cyc_Toc_Dest_Absyn_AbsynAnnot_struct{char*tag;struct Cyc_Absyn_Exp*f1;};
# 27 "tovc.h"
extern int Cyc_Tovc_elim_array_initializers;
# 72 "tovc.cyc"
int Cyc_Tovc_elim_array_initializers=1;struct Cyc_Tovc_BoxingEnv{struct _RegionHandle*rgn;struct Cyc_List_List*all_locals;struct Cyc_Dict_Dict varmap;struct Cyc_List_List*boundvars;struct Cyc_Set_Set**freevars;};struct Cyc_Tovc_ToExpEnv{struct _RegionHandle*rgn;struct Cyc_List_List*all_locals;struct Cyc_Dict_Dict varmap;struct Cyc_Absyn_Stmt*encloser;struct Cyc_List_List**gen_ds;};struct _tuple12{struct Cyc_List_List*f1;struct Cyc_Absyn_Exp*f2;};
# 100
static struct Cyc_Absyn_Exp*Cyc_Tovc_box_free_vars_exp(struct Cyc_Tovc_BoxingEnv env,struct Cyc_Absyn_Exp*e){
while(1){
void*_stmttmp0=e->r;void*_tmp0=_stmttmp0;struct Cyc_List_List*_tmp1;struct Cyc_Absyn_Exp**_tmp2;struct Cyc_Absyn_Exp**_tmp3;struct Cyc_Absyn_Exp**_tmp4;struct Cyc_Absyn_Exp**_tmp5;struct Cyc_Absyn_Exp**_tmp6;struct Cyc_Absyn_Exp**_tmp8;struct Cyc_Absyn_Exp**_tmp7;struct Cyc_Absyn_Exp**_tmpA;struct Cyc_Absyn_Exp**_tmp9;struct Cyc_Absyn_Exp**_tmpC;struct Cyc_Absyn_Exp**_tmpB;struct Cyc_Absyn_Exp**_tmpE;struct Cyc_Absyn_Exp**_tmpD;struct Cyc_Absyn_Exp**_tmp11;struct Cyc_Absyn_Exp**_tmp10;struct Cyc_Absyn_Exp**_tmpF;struct Cyc_List_List*_tmp12;struct Cyc_List_List*_tmp14;struct Cyc_Absyn_Exp**_tmp13;int _tmp18;int _tmp17;struct _fat_ptr*_tmp16;struct Cyc_Absyn_Exp*_tmp15;int _tmp1C;int _tmp1B;struct _fat_ptr*_tmp1A;struct Cyc_Absyn_Exp*_tmp19;struct Cyc_Absyn_Exp*_tmp1D;struct Cyc_Absyn_Exp*_tmp1E;struct Cyc_Absyn_Exp*_tmp20;struct Cyc_Absyn_Exp*_tmp1F;void*_tmp21;switch(*((int*)_tmp0)){case 1U: _LL1: _tmp21=(void*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp0)->f1;_LL2: {void*b=_tmp21;
# 104
struct _tuple0*_stmttmp1=({Cyc_Absyn_binding2qvar(b);});struct _tuple0*_tmp22=_stmttmp1;struct _fat_ptr*_tmp23;if(((((struct _tuple0*)_tmp22)->f1).Loc_n).tag == 4){_LL32: _tmp23=_tmp22->f2;_LL33: {struct _fat_ptr*v=_tmp23;
# 108
if(({({int(*_tmp149)(struct Cyc_Dict_Dict d,struct _fat_ptr*k)=({int(*_tmp24)(struct Cyc_Dict_Dict d,struct _fat_ptr*k)=(int(*)(struct Cyc_Dict_Dict d,struct _fat_ptr*k))Cyc_Dict_member;_tmp24;});struct Cyc_Dict_Dict _tmp148=env.varmap;_tmp149(_tmp148,v);});})){
({void*_tmp14C=({({struct Cyc_Absyn_Exp*(*_tmp14B)(struct Cyc_Dict_Dict d,struct _fat_ptr*k)=({struct Cyc_Absyn_Exp*(*_tmp25)(struct Cyc_Dict_Dict d,struct _fat_ptr*k)=(struct Cyc_Absyn_Exp*(*)(struct Cyc_Dict_Dict d,struct _fat_ptr*k))Cyc_Dict_lookup;_tmp25;});struct Cyc_Dict_Dict _tmp14A=env.varmap;_tmp14B(_tmp14A,v);});})->r;e->r=_tmp14C;});
continue;}
# 108
{
# 113
int i=0;
# 108
for(0;(unsigned)i < 
# 113
_get_fat_size(Cyc_Toc_globals,sizeof(struct _fat_ptr*));++ i){
if(({Cyc_strptrcmp(v,*((struct _fat_ptr**)_check_fat_subscript(Cyc_Toc_globals,sizeof(struct _fat_ptr*),i)));})== 0)
return 0;}}
# 108
for(0;env.boundvars != 0;env.boundvars=(env.boundvars)->tl){
# 118
if(({Cyc_strptrcmp(v,(*((struct Cyc_Absyn_Vardecl*)(env.boundvars)->hd)->name).f2);})== 0)
return 0;}
# 108
for(0;env.all_locals != 0;env.all_locals=(env.all_locals)->tl){
# 122
struct Cyc_Absyn_Vardecl*vd=(struct Cyc_Absyn_Vardecl*)(env.all_locals)->hd;
if(({Cyc_strptrcmp(v,(*vd->name).f2);})== 0){
({struct Cyc_Set_Set*_tmp150=({({struct Cyc_Set_Set*(*_tmp14F)(struct _RegionHandle*r,struct Cyc_Set_Set*s,struct Cyc_Absyn_Vardecl*elt)=({struct Cyc_Set_Set*(*_tmp26)(struct _RegionHandle*r,struct Cyc_Set_Set*s,struct Cyc_Absyn_Vardecl*elt)=(struct Cyc_Set_Set*(*)(struct _RegionHandle*r,struct Cyc_Set_Set*s,struct Cyc_Absyn_Vardecl*elt))Cyc_Set_rinsert;_tmp26;});struct _RegionHandle*_tmp14E=env.rgn;struct Cyc_Set_Set*_tmp14D=*env.freevars;_tmp14F(_tmp14E,_tmp14D,vd);});});*env.freevars=_tmp150;});{
# 126
void*_stmttmp2=vd->type;void*_tmp27=_stmttmp2;if(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp27)->tag == 4U){_LL37: _LL38:
 return 0;}else{_LL39: _LL3A:
 return({struct Cyc_Absyn_Exp*(*_tmp28)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_deref_exp;struct Cyc_Absyn_Exp*_tmp29=({Cyc_Absyn_var_exp(vd->name,0U);});unsigned _tmp2A=0U;_tmp28(_tmp29,_tmp2A);});}_LL36:;}}}
# 135
for(0;env.all_locals != 0;env.all_locals=(env.all_locals)->tl){
if(({Cyc_strptrcmp(v,(*((struct Cyc_Absyn_Vardecl*)(env.all_locals)->hd)->name).f2);})== 0)
(int)_throw(({struct Cyc_Core_Impossible_exn_struct*_tmp2E=_cycalloc(sizeof(*_tmp2E));_tmp2E->tag=Cyc_Core_Impossible,({struct _fat_ptr _tmp152=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp2D=({struct Cyc_String_pa_PrintArg_struct _tmp141;_tmp141.tag=0U,_tmp141.f1=(struct _fat_ptr)((struct _fat_ptr)*v);_tmp141;});void*_tmp2B[1U];_tmp2B[0]=& _tmp2D;({struct _fat_ptr _tmp151=({const char*_tmp2C="unbound variable %s found in box_free_vars_exp";_tag_fat(_tmp2C,sizeof(char),47U);});Cyc_aprintf(_tmp151,_tag_fat(_tmp2B,sizeof(void*),1U));});});_tmp2E->f1=_tmp152;});_tmp2E;}));}
# 135
return 0;}}else{_LL34: _LL35:
# 143
 return 0;}_LL31:;}case 23U: _LL3: _tmp1F=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp0)->f1;_tmp20=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp0)->f2;_LL4: {struct Cyc_Absyn_Exp*e1=_tmp1F;struct Cyc_Absyn_Exp*e2=_tmp20;
# 149
struct Cyc_Absyn_Exp*e1b=({Cyc_Tovc_box_free_vars_exp(env,e1);});
struct Cyc_Absyn_Exp*e2b=({Cyc_Tovc_box_free_vars_exp(env,e2);});
if(e1b != 0){
if(e2b != 0){
struct Cyc_Absyn_Exp*eb=({Cyc_Absyn_subscript_exp(e1b,e2b,e->loc);});
eb->topt=e->topt;
eb->annot=e->annot;
return eb;}else{
# 159
struct Cyc_Absyn_Exp*eb=({Cyc_Absyn_subscript_exp(e1b,e2,e->loc);});
eb->topt=e->topt;
eb->annot=e->annot;
return eb;}}else{
# 165
if(e2b != 0){
struct Cyc_Absyn_Exp*eb=({Cyc_Absyn_subscript_exp(e1,e2b,e->loc);});
eb->topt=e->topt;
eb->annot=e->annot;
return eb;}}
# 151
return 0;}case 15U: _LL5: _tmp1E=((struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct*)_tmp0)->f1;_LL6: {struct Cyc_Absyn_Exp*e1=_tmp1E;
# 174
struct Cyc_Absyn_Exp*e1b=({Cyc_Tovc_box_free_vars_exp(env,e1);});
if(e1b != 0){
struct Cyc_Absyn_Exp*eb=({Cyc_Absyn_address_exp(e1b,e->loc);});
eb->topt=e->topt;
eb->annot=e->annot;
return eb;}
# 175
return 0;}case 20U: _LL7: _tmp1D=((struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_tmp0)->f1;_LL8: {struct Cyc_Absyn_Exp*e1=_tmp1D;
# 184
struct Cyc_Absyn_Exp*e1b=({Cyc_Tovc_box_free_vars_exp(env,e1);});
if(e1b != 0){
struct Cyc_Absyn_Exp*eb=({Cyc_Absyn_deref_exp(e1b,e->loc);});
eb->topt=e->topt;
eb->annot=e->annot;
return eb;}
# 185
return 0;}case 21U: _LL9: _tmp19=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp0)->f1;_tmp1A=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp0)->f2;_tmp1B=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp0)->f3;_tmp1C=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp0)->f4;_LLA: {struct Cyc_Absyn_Exp*e1=_tmp19;struct _fat_ptr*fn=_tmp1A;int ist=_tmp1B;int isr=_tmp1C;
# 194
struct Cyc_Absyn_Exp*e1b=({Cyc_Tovc_box_free_vars_exp(env,e1);});
if(e1b != 0){
struct Cyc_Absyn_Exp*eb=({({void*_tmp153=(void*)({struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*_tmp2F=_cycalloc(sizeof(*_tmp2F));_tmp2F->tag=21U,_tmp2F->f1=e1b,_tmp2F->f2=fn,_tmp2F->f3=ist,_tmp2F->f4=isr;_tmp2F;});Cyc_Absyn_new_exp(_tmp153,e->loc);});});
eb->topt=e->topt;
eb->annot=e->annot;
return eb;}
# 195
return 0;}case 22U: _LLB: _tmp15=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp0)->f1;_tmp16=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp0)->f2;_tmp17=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp0)->f3;_tmp18=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp0)->f4;_LLC: {struct Cyc_Absyn_Exp*e1=_tmp15;struct _fat_ptr*fn=_tmp16;int ist=_tmp17;int isr=_tmp18;
# 204
struct Cyc_Absyn_Exp*e1b=({Cyc_Tovc_box_free_vars_exp(env,e1);});
if(e1b != 0){
struct Cyc_Absyn_Exp*eb=({({void*_tmp154=(void*)({struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*_tmp30=_cycalloc(sizeof(*_tmp30));_tmp30->tag=22U,_tmp30->f1=e1b,_tmp30->f2=fn,_tmp30->f3=ist,_tmp30->f4=isr;_tmp30;});Cyc_Absyn_new_exp(_tmp154,e->loc);});});
eb->topt=e->topt;
eb->annot=e->annot;
return eb;}
# 205
return 0;}case 0U: _LLD: _LLE:
# 214
 return 0;case 10U: _LLF: _tmp13=(struct Cyc_Absyn_Exp**)&((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp0)->f1;_tmp14=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp0)->f2;_LL10: {struct Cyc_Absyn_Exp**e1=_tmp13;struct Cyc_List_List*es=_tmp14;
# 216
{struct Cyc_Absyn_Exp*__ei=({Cyc_Tovc_box_free_vars_exp(env,*e1);});if(__ei != 0)*e1=__ei;}
_tmp12=es;goto _LL12;}case 3U: _LL11: _tmp12=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp0)->f2;_LL12: {struct Cyc_List_List*es=_tmp12;
# 219
for(0;es != 0;es=es->tl){
struct Cyc_Absyn_Exp*__ei=({Cyc_Tovc_box_free_vars_exp(env,(struct Cyc_Absyn_Exp*)es->hd);});if(__ei != 0)es->hd=(void*)__ei;}
# 222
return 0;}case 6U: _LL13: _tmpF=(struct Cyc_Absyn_Exp**)&((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp0)->f1;_tmp10=(struct Cyc_Absyn_Exp**)&((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp0)->f2;_tmp11=(struct Cyc_Absyn_Exp**)&((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp0)->f3;_LL14: {struct Cyc_Absyn_Exp**e1=_tmpF;struct Cyc_Absyn_Exp**e2=_tmp10;struct Cyc_Absyn_Exp**e3=_tmp11;
# 225
{struct Cyc_Absyn_Exp*__ei=({Cyc_Tovc_box_free_vars_exp(env,*e3);});if(__ei != 0)*e3=__ei;}
_tmpD=e1;_tmpE=e2;goto _LL16;}case 7U: _LL15: _tmpD=(struct Cyc_Absyn_Exp**)&((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp0)->f1;_tmpE=(struct Cyc_Absyn_Exp**)&((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp0)->f2;_LL16: {struct Cyc_Absyn_Exp**e1=_tmpD;struct Cyc_Absyn_Exp**e2=_tmpE;
_tmpB=e1;_tmpC=e2;goto _LL18;}case 8U: _LL17: _tmpB=(struct Cyc_Absyn_Exp**)&((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp0)->f1;_tmpC=(struct Cyc_Absyn_Exp**)&((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp0)->f2;_LL18: {struct Cyc_Absyn_Exp**e1=_tmpB;struct Cyc_Absyn_Exp**e2=_tmpC;
_tmp9=e1;_tmpA=e2;goto _LL1A;}case 9U: _LL19: _tmp9=(struct Cyc_Absyn_Exp**)&((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp0)->f1;_tmpA=(struct Cyc_Absyn_Exp**)&((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp0)->f2;_LL1A: {struct Cyc_Absyn_Exp**e1=_tmp9;struct Cyc_Absyn_Exp**e2=_tmpA;
_tmp7=e1;_tmp8=e2;goto _LL1C;}case 4U: _LL1B: _tmp7=(struct Cyc_Absyn_Exp**)&((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp0)->f1;_tmp8=(struct Cyc_Absyn_Exp**)&((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp0)->f3;_LL1C: {struct Cyc_Absyn_Exp**e1=_tmp7;struct Cyc_Absyn_Exp**e2=_tmp8;
# 231
{struct Cyc_Absyn_Exp*__ei=({Cyc_Tovc_box_free_vars_exp(env,*e2);});if(__ei != 0)*e2=__ei;}
_tmp6=e1;goto _LL1E;}case 12U: _LL1D: _tmp6=(struct Cyc_Absyn_Exp**)&((struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*)_tmp0)->f1;_LL1E: {struct Cyc_Absyn_Exp**e1=_tmp6;
# 234
_tmp5=e1;goto _LL20;}case 13U: _LL1F: _tmp5=(struct Cyc_Absyn_Exp**)&((struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*)_tmp0)->f1;_LL20: {struct Cyc_Absyn_Exp**e1=_tmp5;
_tmp4=e1;goto _LL22;}case 14U: _LL21: _tmp4=(struct Cyc_Absyn_Exp**)&((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp0)->f2;_LL22: {struct Cyc_Absyn_Exp**e1=_tmp4;
_tmp3=e1;goto _LL24;}case 18U: _LL23: _tmp3=(struct Cyc_Absyn_Exp**)&((struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct*)_tmp0)->f1;_LL24: {struct Cyc_Absyn_Exp**e1=_tmp3;
_tmp2=e1;goto _LL26;}case 5U: _LL25: _tmp2=(struct Cyc_Absyn_Exp**)&((struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*)_tmp0)->f1;_LL26: {struct Cyc_Absyn_Exp**e1=_tmp2;
# 239
{struct Cyc_Absyn_Exp*__ei=({Cyc_Tovc_box_free_vars_exp(env,*e1);});if(__ei != 0)*e1=__ei;}
return 0;}case 17U: _LL27: _LL28:
# 242
 goto _LL2A;case 19U: _LL29: _LL2A:
 goto _LL2C;case 32U: _LL2B: _LL2C:
 return 0;case 37U: _LL2D: _tmp1=((struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*)_tmp0)->f2;_LL2E: {struct Cyc_List_List*dles=_tmp1;
# 247
for(0;dles != 0;dles=dles->tl){
struct Cyc_Absyn_Exp*__ei=({Cyc_Tovc_box_free_vars_exp(env,(*((struct _tuple12*)dles->hd)).f2);});if(__ei != 0)(*((struct _tuple12*)dles->hd)).f2=__ei;}
# 250
return 0;}default: _LL2F: _LL30:
# 252
(int)_throw(({struct Cyc_Core_Impossible_exn_struct*_tmp32=_cycalloc(sizeof(*_tmp32));_tmp32->tag=Cyc_Core_Impossible,({struct _fat_ptr _tmp155=({const char*_tmp31="bad exp form in Tocv::box_free_vars_exp";_tag_fat(_tmp31,sizeof(char),40U);});_tmp32->f1=_tmp155;});_tmp32;}));}_LL0:;}}
# 256
static void Cyc_Tovc_box_free_vars_stmt(struct Cyc_Tovc_BoxingEnv env,struct Cyc_Absyn_Stmt*s){
# 260
while(1){
void*_stmttmp3=s->r;void*_tmp34=_stmttmp3;struct Cyc_Absyn_Stmt*_tmp36;struct Cyc_Absyn_Decl*_tmp35;struct Cyc_List_List*_tmp38;struct Cyc_Absyn_Exp**_tmp37;struct Cyc_Absyn_Exp**_tmp3A;struct Cyc_Absyn_Stmt*_tmp39;struct Cyc_Absyn_Stmt*_tmp3E;struct Cyc_Absyn_Exp**_tmp3D;struct Cyc_Absyn_Exp**_tmp3C;struct Cyc_Absyn_Exp**_tmp3B;struct Cyc_Absyn_Stmt*_tmp40;struct Cyc_Absyn_Exp**_tmp3F;struct Cyc_Absyn_Stmt*_tmp43;struct Cyc_Absyn_Stmt*_tmp42;struct Cyc_Absyn_Exp**_tmp41;struct Cyc_Absyn_Stmt*_tmp45;struct Cyc_Absyn_Stmt*_tmp44;struct Cyc_Absyn_Exp**_tmp46;struct Cyc_Absyn_Exp**_tmp47;struct Cyc_Absyn_Stmt*_tmp48;switch(*((int*)_tmp34)){case 0U: _LL1: _LL2:
 return;case 6U: _LL3: _LL4:
# 264
({void*_tmp49=0U;({unsigned _tmp157=s->loc;struct _fat_ptr _tmp156=({const char*_tmp4A="break in eliminated statement expression";_tag_fat(_tmp4A,sizeof(char),41U);});Cyc_Tcutil_warn(_tmp157,_tmp156,_tag_fat(_tmp49,sizeof(void*),0U));});});
return;case 7U: _LL5: _LL6:
# 267
({void*_tmp4B=0U;({unsigned _tmp159=s->loc;struct _fat_ptr _tmp158=({const char*_tmp4C="continue in eliminated statement expression";_tag_fat(_tmp4C,sizeof(char),44U);});Cyc_Tcutil_warn(_tmp159,_tmp158,_tag_fat(_tmp4B,sizeof(void*),0U));});});
# 270
return;case 8U: _LL7: _LL8:
# 272
({void*_tmp4D=0U;({unsigned _tmp15B=s->loc;struct _fat_ptr _tmp15A=({const char*_tmp4E="goto in eliminated statement expression";_tag_fat(_tmp4E,sizeof(char),40U);});Cyc_Tcutil_warn(_tmp15B,_tmp15A,_tag_fat(_tmp4D,sizeof(void*),0U));});});
return;case 13U: _LL9: _tmp48=((struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct*)_tmp34)->f2;_LLA: {struct Cyc_Absyn_Stmt*s2=_tmp48;
# 275
s=s2;
continue;}case 3U: _LLB: _tmp47=(struct Cyc_Absyn_Exp**)&((struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct*)_tmp34)->f1;_LLC: {struct Cyc_Absyn_Exp**eopt=_tmp47;
# 278
({void*_tmp4F=0U;({unsigned _tmp15D=s->loc;struct _fat_ptr _tmp15C=({const char*_tmp50="return in eliminated statement expression";_tag_fat(_tmp50,sizeof(char),42U);});Cyc_Tcutil_warn(_tmp15D,_tmp15C,_tag_fat(_tmp4F,sizeof(void*),0U));});});
# 280
if(*eopt != 0){
struct Cyc_Absyn_Exp*ei=({Cyc_Tovc_box_free_vars_exp(env,(struct Cyc_Absyn_Exp*)_check_null(*eopt));});
if(ei != 0)*eopt=ei;}
# 280
return;}case 1U: _LLD: _tmp46=(struct Cyc_Absyn_Exp**)&((struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct*)_tmp34)->f1;_LLE: {struct Cyc_Absyn_Exp**e=_tmp46;
# 286
{struct Cyc_Absyn_Exp*__ei=({Cyc_Tovc_box_free_vars_exp(env,*e);});if(__ei != 0)*e=__ei;}
return;}case 2U: _LLF: _tmp44=((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_tmp34)->f1;_tmp45=((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_tmp34)->f2;_LL10: {struct Cyc_Absyn_Stmt*s1=_tmp44;struct Cyc_Absyn_Stmt*s2=_tmp45;
# 289
({Cyc_Tovc_box_free_vars_stmt(env,s1);});
s=s2;
continue;}case 4U: _LL11: _tmp41=(struct Cyc_Absyn_Exp**)&((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmp34)->f1;_tmp42=((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmp34)->f2;_tmp43=((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmp34)->f3;_LL12: {struct Cyc_Absyn_Exp**e=_tmp41;struct Cyc_Absyn_Stmt*s1=_tmp42;struct Cyc_Absyn_Stmt*s2=_tmp43;
# 293
{struct Cyc_Absyn_Exp*__ei=({Cyc_Tovc_box_free_vars_exp(env,*e);});if(__ei != 0)*e=__ei;}
({Cyc_Tovc_box_free_vars_stmt(env,s1);});
s=s2;
continue;}case 5U: _LL13: _tmp3F=(struct Cyc_Absyn_Exp**)&(((struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct*)_tmp34)->f1).f1;_tmp40=((struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct*)_tmp34)->f2;_LL14: {struct Cyc_Absyn_Exp**e=_tmp3F;struct Cyc_Absyn_Stmt*s2=_tmp40;
# 298
{struct Cyc_Absyn_Exp*__ei=({Cyc_Tovc_box_free_vars_exp(env,*e);});if(__ei != 0)*e=__ei;}
s=s2;
continue;}case 9U: _LL15: _tmp3B=(struct Cyc_Absyn_Exp**)&((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp34)->f1;_tmp3C=(struct Cyc_Absyn_Exp**)&(((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp34)->f2).f1;_tmp3D=(struct Cyc_Absyn_Exp**)&(((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp34)->f3).f1;_tmp3E=((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp34)->f4;_LL16: {struct Cyc_Absyn_Exp**e1=_tmp3B;struct Cyc_Absyn_Exp**e2=_tmp3C;struct Cyc_Absyn_Exp**e3=_tmp3D;struct Cyc_Absyn_Stmt*s2=_tmp3E;
# 302
{struct Cyc_Absyn_Exp*__ei=({Cyc_Tovc_box_free_vars_exp(env,*e1);});if(__ei != 0)*e1=__ei;}
{struct Cyc_Absyn_Exp*__ei=({Cyc_Tovc_box_free_vars_exp(env,*e2);});if(__ei != 0)*e2=__ei;}
{struct Cyc_Absyn_Exp*__ei=({Cyc_Tovc_box_free_vars_exp(env,*e3);});if(__ei != 0)*e3=__ei;}
s=s2;
continue;}case 14U: _LL17: _tmp39=((struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct*)_tmp34)->f1;_tmp3A=(struct Cyc_Absyn_Exp**)&(((struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct*)_tmp34)->f2).f1;_LL18: {struct Cyc_Absyn_Stmt*s2=_tmp39;struct Cyc_Absyn_Exp**e=_tmp3A;
# 308
{struct Cyc_Absyn_Exp*__ei=({Cyc_Tovc_box_free_vars_exp(env,*e);});if(__ei != 0)*e=__ei;}
s=s2;
continue;}case 10U: _LL19: _tmp37=(struct Cyc_Absyn_Exp**)&((struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct*)_tmp34)->f1;_tmp38=((struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct*)_tmp34)->f2;_LL1A: {struct Cyc_Absyn_Exp**e=_tmp37;struct Cyc_List_List*scs=_tmp38;
# 313
{struct Cyc_Absyn_Exp*__ei=({Cyc_Tovc_box_free_vars_exp(env,*e);});if(__ei != 0)*e=__ei;}
for(0;scs != 0;scs=scs->tl){
({Cyc_Tovc_box_free_vars_stmt(env,((struct Cyc_Absyn_Switch_clause*)scs->hd)->body);});}
return;}case 12U: _LL1B: _tmp35=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp34)->f1;_tmp36=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp34)->f2;_LL1C: {struct Cyc_Absyn_Decl*d=_tmp35;struct Cyc_Absyn_Stmt*s2=_tmp36;
# 318
{void*_stmttmp4=d->r;void*_tmp51=_stmttmp4;struct Cyc_Absyn_Vardecl*_tmp52;if(((struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_tmp51)->tag == 0U){_LL20: _tmp52=((struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_tmp51)->f1;_LL21: {struct Cyc_Absyn_Vardecl*vd=_tmp52;
# 320
({struct Cyc_List_List*_tmp15E=({struct Cyc_List_List*_tmp53=_region_malloc(env.rgn,sizeof(*_tmp53));_tmp53->hd=vd,_tmp53->tl=env.boundvars;_tmp53;});env.boundvars=_tmp15E;});
({struct Cyc_Dict_Dict _tmp161=({({struct Cyc_Dict_Dict(*_tmp160)(struct Cyc_Dict_Dict,struct _fat_ptr*)=({struct Cyc_Dict_Dict(*_tmp54)(struct Cyc_Dict_Dict,struct _fat_ptr*)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict,struct _fat_ptr*))Cyc_Dict_rdelete_same;_tmp54;});struct Cyc_Dict_Dict _tmp15F=env.varmap;_tmp160(_tmp15F,(*vd->name).f2);});});env.varmap=_tmp161;});
if(vd->initializer != 0){
struct Cyc_Absyn_Exp*ei=({Cyc_Tovc_box_free_vars_exp(env,(struct Cyc_Absyn_Exp*)_check_null(vd->initializer));});
if(ei != 0)vd->initializer=ei;}
# 322
s=s2;
# 327
continue;}}else{_LL22: _LL23:
 goto _LL1F;}_LL1F:;}
# 330
goto _LL1E;}default: _LL1D: _LL1E:
(int)_throw(({struct Cyc_Core_Impossible_exn_struct*_tmp56=_cycalloc(sizeof(*_tmp56));_tmp56->tag=Cyc_Core_Impossible,({struct _fat_ptr _tmp162=({const char*_tmp55="bad stmt after xlation to C";_tag_fat(_tmp55,sizeof(char),28U);});_tmp56->f1=_tmp162;});_tmp56;}));}_LL0:;}}
# 341
static int Cyc_Tovc_stmt_to_fun_cmp(struct Cyc_Absyn_Vardecl*x,struct Cyc_Absyn_Vardecl*y){
return({Cyc_Absyn_qvar_cmp(x->name,y->name);});}
# 345
static struct Cyc_Absyn_Exp*Cyc_Tovc_stmt_to_fun(struct Cyc_Tovc_ToExpEnv env,struct Cyc_Absyn_Stmt*s,void*rettype){
# 348
struct Cyc_Set_Set*freevars=({({struct Cyc_Set_Set*(*_tmp163)(struct _RegionHandle*r,int(*cmp)(struct Cyc_Absyn_Vardecl*,struct Cyc_Absyn_Vardecl*))=({struct Cyc_Set_Set*(*_tmp76)(struct _RegionHandle*r,int(*cmp)(struct Cyc_Absyn_Vardecl*,struct Cyc_Absyn_Vardecl*))=(struct Cyc_Set_Set*(*)(struct _RegionHandle*r,int(*cmp)(struct Cyc_Absyn_Vardecl*,struct Cyc_Absyn_Vardecl*)))Cyc_Set_rempty;_tmp76;});_tmp163(env.rgn,Cyc_Tovc_stmt_to_fun_cmp);});});
struct Cyc_Tovc_BoxingEnv boxenv=({struct Cyc_Tovc_BoxingEnv _tmp142;_tmp142.rgn=env.rgn,_tmp142.all_locals=env.all_locals,_tmp142.varmap=env.varmap,_tmp142.boundvars=0,_tmp142.freevars=& freevars;_tmp142;});
({Cyc_Tovc_box_free_vars_stmt(boxenv,s);});{
struct Cyc_List_List*params=0;
struct Cyc_List_List*args=0;
# 354
struct Cyc_Iter_Iter iter=({Cyc_Set_make_iter(Cyc_Core_heap_region,freevars);});
# 356
struct Cyc_Absyn_Vardecl*vd=({struct Cyc_Absyn_Vardecl*_tmp75=_cycalloc(sizeof(*_tmp75));_tmp75->sc=Cyc_Absyn_Public,_tmp75->name=Cyc_Absyn_exn_name,_tmp75->varloc=0U,({struct Cyc_Absyn_Tqual _tmp164=({Cyc_Absyn_empty_tqual(0U);});_tmp75->tq=_tmp164;}),_tmp75->type=Cyc_Absyn_void_type,_tmp75->initializer=0,_tmp75->rgn=0,_tmp75->attributes=0,_tmp75->escapes=0;_tmp75;});
# 359
while(({({int(*_tmp165)(struct Cyc_Iter_Iter,struct Cyc_Absyn_Vardecl**)=({int(*_tmp59)(struct Cyc_Iter_Iter,struct Cyc_Absyn_Vardecl**)=(int(*)(struct Cyc_Iter_Iter,struct Cyc_Absyn_Vardecl**))Cyc_Iter_next;_tmp59;});_tmp165(iter,& vd);});})){
void*_stmttmp5=vd->type;void*_tmp5A=_stmttmp5;void*_tmp5B;if(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp5A)->tag == 4U){_LL1: _tmp5B=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp5A)->f1).elt_type;_LL2: {void*elt_type=_tmp5B;
# 362
params=({struct Cyc_List_List*_tmp60=_cycalloc(sizeof(*_tmp60));({struct _tuple9*_tmp167=({struct _tuple9*_tmp5F=_cycalloc(sizeof(*_tmp5F));_tmp5F->f1=(*vd->name).f2,_tmp5F->f2=vd->tq,({
void*_tmp166=({void*(*_tmp5C)(void*,struct Cyc_Absyn_Tqual)=Cyc_Absyn_cstar_type;void*_tmp5D=elt_type;struct Cyc_Absyn_Tqual _tmp5E=({Cyc_Absyn_empty_tqual(0U);});_tmp5C(_tmp5D,_tmp5E);});_tmp5F->f3=_tmp166;});_tmp5F;});
# 362
_tmp60->hd=_tmp167;}),_tmp60->tl=params;_tmp60;});
# 365
args=({struct Cyc_List_List*_tmp61=_cycalloc(sizeof(*_tmp61));({struct Cyc_Absyn_Exp*_tmp168=({Cyc_Absyn_var_exp(vd->name,0U);});_tmp61->hd=_tmp168;}),_tmp61->tl=args;_tmp61;});
goto _LL0;}}else{_LL3: _LL4:
# 368
 params=({struct Cyc_List_List*_tmp66=_cycalloc(sizeof(*_tmp66));({struct _tuple9*_tmp16A=({struct _tuple9*_tmp65=_cycalloc(sizeof(*_tmp65));_tmp65->f1=(*vd->name).f2,_tmp65->f2=vd->tq,({
void*_tmp169=({void*(*_tmp62)(void*,struct Cyc_Absyn_Tqual)=Cyc_Absyn_cstar_type;void*_tmp63=vd->type;struct Cyc_Absyn_Tqual _tmp64=({Cyc_Absyn_empty_tqual(0U);});_tmp62(_tmp63,_tmp64);});_tmp65->f3=_tmp169;});_tmp65;});
# 368
_tmp66->hd=_tmp16A;}),_tmp66->tl=params;_tmp66;});
# 371
args=({struct Cyc_List_List*_tmp6A=_cycalloc(sizeof(*_tmp6A));({struct Cyc_Absyn_Exp*_tmp16B=({struct Cyc_Absyn_Exp*(*_tmp67)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_address_exp;struct Cyc_Absyn_Exp*_tmp68=({Cyc_Absyn_var_exp(vd->name,0U);});unsigned _tmp69=0U;_tmp67(_tmp68,_tmp69);});_tmp6A->hd=_tmp16B;}),_tmp6A->tl=args;_tmp6A;});
goto _LL0;}_LL0:;}{
# 374
struct _tuple0*funname=({Cyc_Toc_temp_var();});
({struct Cyc_List_List*_tmp16F=({struct Cyc_List_List*_tmp70=_cycalloc(sizeof(*_tmp70));
({struct Cyc_Absyn_Decl*_tmp16E=({struct Cyc_Absyn_Decl*(*_tmp6B)(void*,unsigned)=Cyc_Absyn_new_decl;void*_tmp6C=(void*)({struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*_tmp6E=_cycalloc(sizeof(*_tmp6E));_tmp6E->tag=1U,({struct Cyc_Absyn_Fndecl*_tmp16D=({struct Cyc_Absyn_Fndecl*_tmp6D=_cycalloc(sizeof(*_tmp6D));_tmp6D->sc=Cyc_Absyn_Static,_tmp6D->is_inline=0,_tmp6D->name=funname,_tmp6D->body=s,
(_tmp6D->i).tvars=0,(_tmp6D->i).effect=0,({struct Cyc_Absyn_Tqual _tmp16C=({Cyc_Absyn_empty_tqual(0U);});(_tmp6D->i).ret_tqual=_tmp16C;}),(_tmp6D->i).ret_type=rettype,(_tmp6D->i).args=params,(_tmp6D->i).c_varargs=0,(_tmp6D->i).cyc_varargs=0,(_tmp6D->i).rgn_po=0,(_tmp6D->i).attributes=0,(_tmp6D->i).requires_clause=0,(_tmp6D->i).requires_relns=0,(_tmp6D->i).ensures_clause=0,(_tmp6D->i).ensures_relns=0,(_tmp6D->i).ieffect=0,(_tmp6D->i).oeffect=0,(_tmp6D->i).throws=0,(_tmp6D->i).reentrant=Cyc_Absyn_noreentrant,_tmp6D->cached_type=0,_tmp6D->param_vardecls=0,_tmp6D->fn_vardecl=0;_tmp6D;});
# 376
_tmp6E->f1=_tmp16D;});_tmp6E;});unsigned _tmp6F=0U;_tmp6B(_tmp6C,_tmp6F);});_tmp70->hd=_tmp16E;}),_tmp70->tl=*env.gen_ds;_tmp70;});
# 375
*env.gen_ds=_tmp16F;});
# 383
return({struct Cyc_Absyn_Exp*(*_tmp71)(struct Cyc_Absyn_Exp*,struct Cyc_List_List*,unsigned)=Cyc_Absyn_fncall_exp;struct Cyc_Absyn_Exp*_tmp72=({Cyc_Absyn_var_exp(funname,0U);});struct Cyc_List_List*_tmp73=args;unsigned _tmp74=0U;_tmp71(_tmp72,_tmp73,_tmp74);});}}}
# 386
static void Cyc_Tovc_apply_varmap(struct Cyc_Dict_Dict varmap,struct Cyc_Absyn_Exp*e){
void*_stmttmp6=e->r;void*_tmp78=_stmttmp6;struct Cyc_List_List*_tmp79;struct Cyc_Absyn_Exp*_tmp7A;struct Cyc_Absyn_Exp*_tmp7B;struct Cyc_Absyn_Exp*_tmp7C;struct Cyc_Absyn_Exp*_tmp7D;struct Cyc_Absyn_Exp*_tmp7E;struct Cyc_Absyn_Exp*_tmp7F;struct Cyc_Absyn_Exp*_tmp80;struct Cyc_Absyn_Exp*_tmp81;struct Cyc_Absyn_Exp*_tmp82;struct Cyc_Absyn_Exp*_tmp84;struct Cyc_Absyn_Exp*_tmp83;struct Cyc_Absyn_Exp*_tmp86;struct Cyc_Absyn_Exp*_tmp85;struct Cyc_Absyn_Exp*_tmp88;struct Cyc_Absyn_Exp*_tmp87;struct Cyc_Absyn_Exp*_tmp8A;struct Cyc_Absyn_Exp*_tmp89;struct Cyc_Absyn_Exp*_tmp8C;struct Cyc_Absyn_Exp*_tmp8B;struct Cyc_Absyn_Exp*_tmp8F;struct Cyc_Absyn_Exp*_tmp8E;struct Cyc_Absyn_Exp*_tmp8D;struct Cyc_List_List*_tmp90;struct Cyc_List_List*_tmp92;struct Cyc_Absyn_Exp*_tmp91;void*_tmp93;switch(*((int*)_tmp78)){case 1U: _LL1: _tmp93=(void*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp78)->f1;_LL2: {void*b=_tmp93;
# 389
struct _tuple0*_stmttmp7=({Cyc_Absyn_binding2qvar(b);});struct _tuple0*_tmp94=_stmttmp7;struct _fat_ptr*_tmp95;if(((((struct _tuple0*)_tmp94)->f1).Loc_n).tag == 4){_LL32: _tmp95=_tmp94->f2;_LL33: {struct _fat_ptr*v=_tmp95;
# 391
if(({({int(*_tmp171)(struct Cyc_Dict_Dict d,struct _fat_ptr*k)=({int(*_tmp96)(struct Cyc_Dict_Dict d,struct _fat_ptr*k)=(int(*)(struct Cyc_Dict_Dict d,struct _fat_ptr*k))Cyc_Dict_member;_tmp96;});struct Cyc_Dict_Dict _tmp170=varmap;_tmp171(_tmp170,v);});}))
({void*_tmp174=({({struct Cyc_Absyn_Exp*(*_tmp173)(struct Cyc_Dict_Dict d,struct _fat_ptr*k)=({struct Cyc_Absyn_Exp*(*_tmp97)(struct Cyc_Dict_Dict d,struct _fat_ptr*k)=(struct Cyc_Absyn_Exp*(*)(struct Cyc_Dict_Dict d,struct _fat_ptr*k))Cyc_Dict_lookup;_tmp97;});struct Cyc_Dict_Dict _tmp172=varmap;_tmp173(_tmp172,v);});})->r;e->r=_tmp174;});
# 391
return;}}else{_LL34: _LL35:
# 394
 return;}_LL31:;}case 0U: _LL3: _LL4:
# 398
 return;case 10U: _LL5: _tmp91=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp78)->f1;_tmp92=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp78)->f2;_LL6: {struct Cyc_Absyn_Exp*e1=_tmp91;struct Cyc_List_List*es=_tmp92;
# 400
({Cyc_Tovc_apply_varmap(varmap,e1);});_tmp90=es;goto _LL8;}case 3U: _LL7: _tmp90=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp78)->f2;_LL8: {struct Cyc_List_List*es=_tmp90;
# 402
for(0;es != 0;es=es->tl){
({Cyc_Tovc_apply_varmap(varmap,(struct Cyc_Absyn_Exp*)es->hd);});}
return;}case 6U: _LL9: _tmp8D=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp78)->f1;_tmp8E=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp78)->f2;_tmp8F=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp78)->f3;_LLA: {struct Cyc_Absyn_Exp*e1=_tmp8D;struct Cyc_Absyn_Exp*e2=_tmp8E;struct Cyc_Absyn_Exp*e3=_tmp8F;
# 407
({Cyc_Tovc_apply_varmap(varmap,e3);});_tmp8B=e1;_tmp8C=e2;goto _LLC;}case 7U: _LLB: _tmp8B=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp78)->f1;_tmp8C=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp78)->f2;_LLC: {struct Cyc_Absyn_Exp*e1=_tmp8B;struct Cyc_Absyn_Exp*e2=_tmp8C;
_tmp89=e1;_tmp8A=e2;goto _LLE;}case 8U: _LLD: _tmp89=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp78)->f1;_tmp8A=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp78)->f2;_LLE: {struct Cyc_Absyn_Exp*e1=_tmp89;struct Cyc_Absyn_Exp*e2=_tmp8A;
_tmp87=e1;_tmp88=e2;goto _LL10;}case 23U: _LLF: _tmp87=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp78)->f1;_tmp88=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp78)->f2;_LL10: {struct Cyc_Absyn_Exp*e1=_tmp87;struct Cyc_Absyn_Exp*e2=_tmp88;
_tmp85=e1;_tmp86=e2;goto _LL12;}case 9U: _LL11: _tmp85=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp78)->f1;_tmp86=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp78)->f2;_LL12: {struct Cyc_Absyn_Exp*e1=_tmp85;struct Cyc_Absyn_Exp*e2=_tmp86;
_tmp83=e1;_tmp84=e2;goto _LL14;}case 4U: _LL13: _tmp83=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp78)->f1;_tmp84=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp78)->f3;_LL14: {struct Cyc_Absyn_Exp*e1=_tmp83;struct Cyc_Absyn_Exp*e2=_tmp84;
# 413
({Cyc_Tovc_apply_varmap(varmap,e2);});_tmp82=e1;goto _LL16;}case 12U: _LL15: _tmp82=((struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*)_tmp78)->f1;_LL16: {struct Cyc_Absyn_Exp*e1=_tmp82;
# 415
_tmp81=e1;goto _LL18;}case 13U: _LL17: _tmp81=((struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*)_tmp78)->f1;_LL18: {struct Cyc_Absyn_Exp*e1=_tmp81;
_tmp80=e1;goto _LL1A;}case 14U: _LL19: _tmp80=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp78)->f2;_LL1A: {struct Cyc_Absyn_Exp*e1=_tmp80;
_tmp7F=e1;goto _LL1C;}case 15U: _LL1B: _tmp7F=((struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct*)_tmp78)->f1;_LL1C: {struct Cyc_Absyn_Exp*e1=_tmp7F;
_tmp7E=e1;goto _LL1E;}case 18U: _LL1D: _tmp7E=((struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct*)_tmp78)->f1;_LL1E: {struct Cyc_Absyn_Exp*e1=_tmp7E;
_tmp7D=e1;goto _LL20;}case 20U: _LL1F: _tmp7D=((struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_tmp78)->f1;_LL20: {struct Cyc_Absyn_Exp*e1=_tmp7D;
_tmp7C=e1;goto _LL22;}case 21U: _LL21: _tmp7C=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp78)->f1;_LL22: {struct Cyc_Absyn_Exp*e1=_tmp7C;
_tmp7B=e1;goto _LL24;}case 22U: _LL23: _tmp7B=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp78)->f1;_LL24: {struct Cyc_Absyn_Exp*e1=_tmp7B;
_tmp7A=e1;goto _LL26;}case 5U: _LL25: _tmp7A=((struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*)_tmp78)->f1;_LL26: {struct Cyc_Absyn_Exp*e1=_tmp7A;
# 424
({Cyc_Tovc_apply_varmap(varmap,e1);});
return;}case 17U: _LL27: _LL28:
# 427
 goto _LL2A;case 19U: _LL29: _LL2A:
 goto _LL2C;case 32U: _LL2B: _LL2C:
 return;case 37U: _LL2D: _tmp79=((struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*)_tmp78)->f2;_LL2E: {struct Cyc_List_List*dles=_tmp79;
# 432
for(0;dles != 0;dles=dles->tl){
({Cyc_Tovc_apply_varmap(varmap,(*((struct _tuple12*)dles->hd)).f2);});}
return;}default: _LL2F: _LL30:
# 436
(int)_throw(({struct Cyc_Core_Impossible_exn_struct*_tmp99=_cycalloc(sizeof(*_tmp99));_tmp99->tag=Cyc_Core_Impossible,({struct _fat_ptr _tmp175=({const char*_tmp98="bad exp form in Tocv::apply_varmap";_tag_fat(_tmp98,sizeof(char),35U);});_tmp99->f1=_tmp175;});_tmp99;}));}_LL0:;}
# 440
static struct Cyc_Absyn_Exp*Cyc_Tovc_stmt_to_exp(struct Cyc_Tovc_ToExpEnv env,struct Cyc_Absyn_Stmt*s){
void*_stmttmp8=s->r;void*_tmp9B=_stmttmp8;struct Cyc_Absyn_Stmt*_tmp9D;struct Cyc_Absyn_Decl*_tmp9C;struct Cyc_Absyn_Stmt*_tmpA0;struct Cyc_Absyn_Stmt*_tmp9F;struct Cyc_Absyn_Exp*_tmp9E;struct Cyc_Absyn_Stmt*_tmpA2;struct Cyc_Absyn_Stmt*_tmpA1;struct Cyc_Absyn_Exp*_tmpA3;switch(*((int*)_tmp9B)){case 0U: _LL1: _LL2:
 return({Cyc_Absyn_true_exp(0U);});case 1U: _LL3: _tmpA3=((struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct*)_tmp9B)->f1;_LL4: {struct Cyc_Absyn_Exp*e=_tmpA3;
# 444
({Cyc_Tovc_apply_varmap(env.varmap,e);});
return e;}case 2U: _LL5: _tmpA1=((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_tmp9B)->f1;_tmpA2=((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_tmp9B)->f2;_LL6: {struct Cyc_Absyn_Stmt*s1=_tmpA1;struct Cyc_Absyn_Stmt*s2=_tmpA2;
# 448
struct Cyc_Absyn_Exp*s1_new=({Cyc_Tovc_stmt_to_exp(env,s1);});
struct Cyc_Absyn_Exp*s2_new=({Cyc_Tovc_stmt_to_exp(env,s2);});
return({Cyc_Absyn_seq_exp(s1_new,s2_new,0U);});}case 4U: _LL7: _tmp9E=((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmp9B)->f1;_tmp9F=((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmp9B)->f2;_tmpA0=((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmp9B)->f3;_LL8: {struct Cyc_Absyn_Exp*e=_tmp9E;struct Cyc_Absyn_Stmt*s1=_tmp9F;struct Cyc_Absyn_Stmt*s2=_tmpA0;
# 452
({Cyc_Tovc_apply_varmap(env.varmap,e);});{
# 454
struct Cyc_Absyn_Exp*s1_new=({Cyc_Tovc_stmt_to_exp(env,s1);});
struct Cyc_Absyn_Exp*s2_new=({Cyc_Tovc_stmt_to_exp(env,s2);});
return({Cyc_Absyn_conditional_exp(e,s1_new,s2_new,0U);});}}case 12U: _LL9: _tmp9C=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp9B)->f1;_tmp9D=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp9B)->f2;_LLA: {struct Cyc_Absyn_Decl*d=_tmp9C;struct Cyc_Absyn_Stmt*s2=_tmp9D;
# 458
void*_stmttmp9=d->r;void*_tmpA4=_stmttmp9;struct Cyc_Absyn_Vardecl*_tmpA5;if(((struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_tmpA4)->tag == 0U){_LLE: _tmpA5=((struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_tmpA4)->f1;_LLF: {struct Cyc_Absyn_Vardecl*vd=_tmpA5;
# 461
struct _fat_ptr*oldname=(*vd->name).f2;
struct _tuple0*newqvar=({Cyc_Toc_temp_var();});
vd->name=newqvar;
({struct Cyc_Dict_Dict _tmp176=({struct Cyc_Dict_Dict(*_tmpA6)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,struct Cyc_Absyn_Exp*v)=({struct Cyc_Dict_Dict(*_tmpA7)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,struct Cyc_Absyn_Exp*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _fat_ptr*k,struct Cyc_Absyn_Exp*v))Cyc_Dict_insert;_tmpA7;});struct Cyc_Dict_Dict _tmpA8=env.varmap;struct _fat_ptr*_tmpA9=oldname;struct Cyc_Absyn_Exp*_tmpAA=({Cyc_Absyn_var_exp(newqvar,0U);});_tmpA6(_tmpA8,_tmpA9,_tmpAA);});env.varmap=_tmp176;});{
struct Cyc_List_List*unresolvedmem_inits=0;
if(vd->initializer != 0){
({Cyc_Tovc_apply_varmap(env.varmap,(struct Cyc_Absyn_Exp*)_check_null(vd->initializer));});{
void*_stmttmpA=((struct Cyc_Absyn_Exp*)_check_null(vd->initializer))->r;void*_tmpAB=_stmttmpA;struct Cyc_List_List*_tmpAC;if(((struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*)_tmpAB)->tag == 37U){_LL13: _tmpAC=((struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*)_tmpAB)->f2;_LL14: {struct Cyc_List_List*dles=_tmpAC;
# 480 "tovc.cyc"
struct Cyc_Absyn_Exp*var=({Cyc_Absyn_var_exp(vd->name,0U);});
{unsigned i=0U;for(0;dles != 0;(dles=dles->tl,++ i)){
unresolvedmem_inits=({struct Cyc_List_List*_tmpB5=_cycalloc(sizeof(*_tmpB5));({struct Cyc_Absyn_Exp*_tmp177=({struct Cyc_Absyn_Exp*(*_tmpAD)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_assign_exp;struct Cyc_Absyn_Exp*_tmpAE=({struct Cyc_Absyn_Exp*(*_tmpAF)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_subscript_exp;struct Cyc_Absyn_Exp*_tmpB0=var;struct Cyc_Absyn_Exp*_tmpB1=({Cyc_Absyn_uint_exp(i,0U);});unsigned _tmpB2=0U;_tmpAF(_tmpB0,_tmpB1,_tmpB2);});struct Cyc_Absyn_Exp*_tmpB3=(*((struct _tuple12*)dles->hd)).f2;unsigned _tmpB4=0U;_tmpAD(_tmpAE,_tmpB3,_tmpB4);});_tmpB5->hd=_tmp177;}),_tmpB5->tl=unresolvedmem_inits;_tmpB5;});}}
# 486
vd->initializer=0;
goto _LL12;}}else{_LL15: _LL16:
 goto _LL12;}_LL12:;}}
# 466 "tovc.cyc"
({struct Cyc_List_List*_tmp178=({struct Cyc_List_List*_tmpB6=_region_malloc(env.rgn,sizeof(*_tmpB6));
# 491 "tovc.cyc"
_tmpB6->hd=vd,_tmpB6->tl=env.all_locals;_tmpB6;});
# 466 "tovc.cyc"
env.all_locals=_tmp178;});
# 493 "tovc.cyc"
({void*_tmp179=({struct Cyc_Absyn_Stmt*(*_tmpB7)(struct Cyc_Absyn_Decl*,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_decl_stmt;struct Cyc_Absyn_Decl*_tmpB8=d;struct Cyc_Absyn_Stmt*_tmpB9=({Cyc_Absyn_new_stmt((env.encloser)->r,0U);});unsigned _tmpBA=0U;_tmpB7(_tmpB8,_tmpB9,_tmpBA);})->r;(env.encloser)->r=_tmp179;});{
struct Cyc_Absyn_Exp*e=({Cyc_Tovc_stmt_to_exp(env,s2);});
if(vd->initializer != 0){
e=({struct Cyc_Absyn_Exp*(*_tmpBB)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_seq_exp;struct Cyc_Absyn_Exp*_tmpBC=({struct Cyc_Absyn_Exp*(*_tmpBD)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_assign_exp;struct Cyc_Absyn_Exp*_tmpBE=({Cyc_Absyn_var_exp(vd->name,0U);});struct Cyc_Absyn_Exp*_tmpBF=(struct Cyc_Absyn_Exp*)_check_null(vd->initializer);unsigned _tmpC0=0U;_tmpBD(_tmpBE,_tmpBF,_tmpC0);});struct Cyc_Absyn_Exp*_tmpC1=e;unsigned _tmpC2=0U;_tmpBB(_tmpBC,_tmpC1,_tmpC2);});
# 498
vd->initializer=0;}
# 495
for(0;unresolvedmem_inits != 0;unresolvedmem_inits=unresolvedmem_inits->tl){
# 501
e=({Cyc_Absyn_seq_exp((struct Cyc_Absyn_Exp*)unresolvedmem_inits->hd,e,0U);});}
return e;}}}}else{_LL10: _LL11:
(int)_throw(({struct Cyc_Core_Impossible_exn_struct*_tmpC4=_cycalloc(sizeof(*_tmpC4));_tmpC4->tag=Cyc_Core_Impossible,({struct _fat_ptr _tmp17A=({const char*_tmpC3="bad local decl in Tovc::stmt_to_exp";_tag_fat(_tmpC3,sizeof(char),36U);});_tmpC4->f1=_tmp17A;});_tmpC4;}));}_LLD:;}default: _LLB: _LLC:
# 506
 return({Cyc_Tovc_stmt_to_fun(env,s,Cyc_Absyn_void_type);});}_LL0:;}
# 440 "tovc.cyc"
static void Cyc_Tovc_stmt_to_vc(struct Cyc_Tovc_ToExpEnv env,struct Cyc_Absyn_Stmt*s);
# 512 "tovc.cyc"
static void Cyc_Tovc_exp_to_vc(struct Cyc_Tovc_ToExpEnv env,struct Cyc_Absyn_Exp*e){
struct _fat_ptr bad_form=({const char*_tmpF8="";_tag_fat(_tmpF8,sizeof(char),1U);});
{void*_stmttmpB=e->r;void*_tmpC6=_stmttmpB;struct Cyc_Absyn_Stmt*_tmpC7;struct Cyc_List_List*_tmpC8;struct Cyc_Absyn_Exp*_tmpC9;struct Cyc_Absyn_Exp*_tmpCA;struct Cyc_Absyn_Exp*_tmpCB;struct Cyc_Absyn_Exp*_tmpCC;struct Cyc_Absyn_Exp*_tmpCD;struct Cyc_Absyn_Exp*_tmpCE;struct Cyc_Absyn_Exp*_tmpCF;struct Cyc_Absyn_Exp*_tmpD0;struct Cyc_Absyn_Exp*_tmpD1;struct Cyc_Absyn_Exp*_tmpD2;struct Cyc_Absyn_Exp*_tmpD4;struct Cyc_Absyn_Exp*_tmpD3;struct Cyc_Absyn_Exp*_tmpD6;struct Cyc_Absyn_Exp*_tmpD5;struct Cyc_Absyn_Exp*_tmpD8;struct Cyc_Absyn_Exp*_tmpD7;struct Cyc_Absyn_Exp*_tmpDA;struct Cyc_Absyn_Exp*_tmpD9;struct Cyc_Absyn_Exp*_tmpDC;struct Cyc_Absyn_Exp*_tmpDB;struct Cyc_Absyn_Exp*_tmpDF;struct Cyc_Absyn_Exp*_tmpDE;struct Cyc_Absyn_Exp*_tmpDD;struct Cyc_List_List*_tmpE0;struct Cyc_List_List*_tmpE2;struct Cyc_Absyn_Exp*_tmpE1;switch(*((int*)_tmpC6)){case 0U: _LL1: _LL2:
 return;case 1U: _LL3: _LL4:
 return;case 10U: _LL5: _tmpE1=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmpC6)->f1;_tmpE2=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmpC6)->f2;_LL6: {struct Cyc_Absyn_Exp*e1=_tmpE1;struct Cyc_List_List*es=_tmpE2;
# 519
({Cyc_Tovc_exp_to_vc(env,e1);});_tmpE0=es;goto _LL8;}case 3U: _LL7: _tmpE0=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmpC6)->f2;_LL8: {struct Cyc_List_List*es=_tmpE0;
# 521
for(0;es != 0;es=es->tl){
({Cyc_Tovc_exp_to_vc(env,(struct Cyc_Absyn_Exp*)es->hd);});}
return;}case 6U: _LL9: _tmpDD=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmpC6)->f1;_tmpDE=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmpC6)->f2;_tmpDF=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmpC6)->f3;_LLA: {struct Cyc_Absyn_Exp*e1=_tmpDD;struct Cyc_Absyn_Exp*e2=_tmpDE;struct Cyc_Absyn_Exp*e3=_tmpDF;
# 526
({Cyc_Tovc_exp_to_vc(env,e3);});_tmpDB=e1;_tmpDC=e2;goto _LLC;}case 7U: _LLB: _tmpDB=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmpC6)->f1;_tmpDC=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmpC6)->f2;_LLC: {struct Cyc_Absyn_Exp*e1=_tmpDB;struct Cyc_Absyn_Exp*e2=_tmpDC;
_tmpD9=e1;_tmpDA=e2;goto _LLE;}case 8U: _LLD: _tmpD9=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmpC6)->f1;_tmpDA=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmpC6)->f2;_LLE: {struct Cyc_Absyn_Exp*e1=_tmpD9;struct Cyc_Absyn_Exp*e2=_tmpDA;
_tmpD7=e1;_tmpD8=e2;goto _LL10;}case 23U: _LLF: _tmpD7=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmpC6)->f1;_tmpD8=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmpC6)->f2;_LL10: {struct Cyc_Absyn_Exp*e1=_tmpD7;struct Cyc_Absyn_Exp*e2=_tmpD8;
_tmpD5=e1;_tmpD6=e2;goto _LL12;}case 9U: _LL11: _tmpD5=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmpC6)->f1;_tmpD6=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmpC6)->f2;_LL12: {struct Cyc_Absyn_Exp*e1=_tmpD5;struct Cyc_Absyn_Exp*e2=_tmpD6;
_tmpD3=e1;_tmpD4=e2;goto _LL14;}case 4U: _LL13: _tmpD3=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmpC6)->f1;_tmpD4=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmpC6)->f3;_LL14: {struct Cyc_Absyn_Exp*e1=_tmpD3;struct Cyc_Absyn_Exp*e2=_tmpD4;
# 532
({Cyc_Tovc_exp_to_vc(env,e2);});_tmpD2=e1;goto _LL16;}case 42U: _LL15: _tmpD2=((struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct*)_tmpC6)->f1;_LL16: {struct Cyc_Absyn_Exp*e1=_tmpD2;
# 534
_tmpD1=e1;goto _LL18;}case 12U: _LL17: _tmpD1=((struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*)_tmpC6)->f1;_LL18: {struct Cyc_Absyn_Exp*e1=_tmpD1;
_tmpD0=e1;goto _LL1A;}case 13U: _LL19: _tmpD0=((struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*)_tmpC6)->f1;_LL1A: {struct Cyc_Absyn_Exp*e1=_tmpD0;
_tmpCF=e1;goto _LL1C;}case 14U: _LL1B: _tmpCF=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmpC6)->f2;_LL1C: {struct Cyc_Absyn_Exp*e1=_tmpCF;
_tmpCE=e1;goto _LL1E;}case 15U: _LL1D: _tmpCE=((struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct*)_tmpC6)->f1;_LL1E: {struct Cyc_Absyn_Exp*e1=_tmpCE;
_tmpCD=e1;goto _LL20;}case 18U: _LL1F: _tmpCD=((struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct*)_tmpC6)->f1;_LL20: {struct Cyc_Absyn_Exp*e1=_tmpCD;
_tmpCC=e1;goto _LL22;}case 20U: _LL21: _tmpCC=((struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_tmpC6)->f1;_LL22: {struct Cyc_Absyn_Exp*e1=_tmpCC;
_tmpCB=e1;goto _LL24;}case 21U: _LL23: _tmpCB=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmpC6)->f1;_LL24: {struct Cyc_Absyn_Exp*e1=_tmpCB;
_tmpCA=e1;goto _LL26;}case 22U: _LL25: _tmpCA=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmpC6)->f1;_LL26: {struct Cyc_Absyn_Exp*e1=_tmpCA;
_tmpC9=e1;goto _LL28;}case 5U: _LL27: _tmpC9=((struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*)_tmpC6)->f1;_LL28: {struct Cyc_Absyn_Exp*e1=_tmpC9;
# 544
({Cyc_Tovc_exp_to_vc(env,e1);});
return;}case 17U: _LL29: _LL2A:
# 547
 goto _LL2C;case 19U: _LL2B: _LL2C:
 goto _LL2E;case 33U: _LL2D: _LL2E:
 goto _LL30;case 32U: _LL2F: _LL30:
 return;case 37U: _LL31: _tmpC8=((struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*)_tmpC6)->f2;_LL32: {struct Cyc_List_List*dles=_tmpC8;
# 556
for(0;dles != 0;dles=dles->tl){
({Cyc_Tovc_exp_to_vc(env,(*((struct _tuple12*)dles->hd)).f2);});}
return;}case 38U: _LL33: _tmpC7=((struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct*)_tmpC6)->f1;_LL34: {struct Cyc_Absyn_Stmt*s=_tmpC7;
# 564
({Cyc_Tovc_stmt_to_vc(env,s);});
# 566
({void*_tmp17B=({Cyc_Tovc_stmt_to_exp(env,s);})->r;e->r=_tmp17B;});
return;}case 41U: _LL35: _LL36:
# 569
 return;case 34U: _LL37: _LL38:
# 571
 bad_form=({const char*_tmpE3="Spawn_e";_tag_fat(_tmpE3,sizeof(char),8U);});goto _LL0;case 2U: _LL39: _LL3A:
 bad_form=({const char*_tmpE4="Pragma_e";_tag_fat(_tmpE4,sizeof(char),9U);});goto _LL0;case 11U: _LL3B: _LL3C:
 bad_form=({const char*_tmpE5="Throw_e";_tag_fat(_tmpE5,sizeof(char),8U);});goto _LL0;case 16U: _LL3D: _LL3E:
 bad_form=({const char*_tmpE6="New_e";_tag_fat(_tmpE6,sizeof(char),6U);});goto _LL0;case 24U: _LL3F: _LL40:
 bad_form=({const char*_tmpE7="Tuple_e";_tag_fat(_tmpE7,sizeof(char),8U);});goto _LL0;case 25U: _LL41: _LL42:
 bad_form=({const char*_tmpE8="CompoundLit_e";_tag_fat(_tmpE8,sizeof(char),14U);});goto _LL0;case 26U: _LL43: _LL44:
 bad_form=({const char*_tmpE9="Array_e";_tag_fat(_tmpE9,sizeof(char),8U);});goto _LL0;case 27U: _LL45: _LL46:
 bad_form=({const char*_tmpEA="Comprehension_e";_tag_fat(_tmpEA,sizeof(char),16U);});goto _LL0;case 28U: _LL47: _LL48:
 bad_form=({const char*_tmpEB="ComprehensionNoinit_e";_tag_fat(_tmpEB,sizeof(char),22U);});goto _LL0;case 29U: _LL49: _LL4A:
 bad_form=({const char*_tmpEC="Aggregate_e";_tag_fat(_tmpEC,sizeof(char),12U);});goto _LL0;case 30U: _LL4B: _LL4C:
 bad_form=({const char*_tmpED="AnonStruct_e";_tag_fat(_tmpED,sizeof(char),13U);});goto _LL0;case 31U: _LL4D: _LL4E:
# 583
 bad_form=({const char*_tmpEE="Datatype_e";_tag_fat(_tmpEE,sizeof(char),11U);});goto _LL0;case 35U: _LL4F: _LL50:
 bad_form=({const char*_tmpEF="Malloc_e";_tag_fat(_tmpEF,sizeof(char),9U);});goto _LL0;case 36U: _LL51: _LL52:
 bad_form=({const char*_tmpF0="Swap_e";_tag_fat(_tmpF0,sizeof(char),7U);});goto _LL0;case 39U: _LL53: _LL54:
 bad_form=({const char*_tmpF1="Tagcheck_e";_tag_fat(_tmpF1,sizeof(char),11U);});goto _LL0;default: _LL55: _LL56:
 bad_form=({const char*_tmpF2="Valueof_e";_tag_fat(_tmpF2,sizeof(char),10U);});goto _LL0;}_LL0:;}
# 589
(int)_throw(({struct Cyc_Core_Impossible_exn_struct*_tmpF7=_cycalloc(sizeof(*_tmpF7));_tmpF7->tag=Cyc_Core_Impossible,({struct _fat_ptr _tmp17E=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmpF5=({struct Cyc_String_pa_PrintArg_struct _tmp144;_tmp144.tag=0U,_tmp144.f1=(struct _fat_ptr)((struct _fat_ptr)bad_form);_tmp144;});struct Cyc_String_pa_PrintArg_struct _tmpF6=({struct Cyc_String_pa_PrintArg_struct _tmp143;_tmp143.tag=0U,({struct _fat_ptr _tmp17C=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_exp2string(e);}));_tmp143.f1=_tmp17C;});_tmp143;});void*_tmpF3[2U];_tmpF3[0]=& _tmpF5,_tmpF3[1]=& _tmpF6;({struct _fat_ptr _tmp17D=({const char*_tmpF4="bad exp form %s (exp=|%s|) after xlation to C";_tag_fat(_tmpF4,sizeof(char),46U);});Cyc_aprintf(_tmp17D,_tag_fat(_tmpF3,sizeof(void*),2U));});});_tmpF7->f1=_tmp17E;});_tmpF7;}));}
# 593
static void Cyc_Tovc_stmt_to_vc(struct Cyc_Tovc_ToExpEnv env,struct Cyc_Absyn_Stmt*s){
# 595
while(1){
env.encloser=s;{
void*_stmttmpC=s->r;void*_tmpFA=_stmttmpC;struct Cyc_Absyn_Stmt*_tmpFC;struct Cyc_Absyn_Decl*_tmpFB;struct Cyc_List_List*_tmpFE;struct Cyc_Absyn_Exp*_tmpFD;struct Cyc_Absyn_Exp*_tmp100;struct Cyc_Absyn_Stmt*_tmpFF;struct Cyc_Absyn_Stmt*_tmp104;struct Cyc_Absyn_Exp*_tmp103;struct Cyc_Absyn_Exp*_tmp102;struct Cyc_Absyn_Exp*_tmp101;struct Cyc_Absyn_Stmt*_tmp106;struct Cyc_Absyn_Exp*_tmp105;struct Cyc_Absyn_Stmt*_tmp109;struct Cyc_Absyn_Stmt*_tmp108;struct Cyc_Absyn_Exp*_tmp107;struct Cyc_Absyn_Stmt*_tmp10B;struct Cyc_Absyn_Stmt*_tmp10A;struct Cyc_Absyn_Exp*_tmp10C;struct Cyc_Absyn_Exp*_tmp10D;struct Cyc_Absyn_Stmt*_tmp10E;switch(*((int*)_tmpFA)){case 0U: _LL1: _LL2:
 goto _LL4;case 6U: _LL3: _LL4:
 goto _LL6;case 7U: _LL5: _LL6:
 goto _LL8;case 8U: _LL7: _LL8:
 return;case 13U: _LL9: _tmp10E=((struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct*)_tmpFA)->f2;_LLA: {struct Cyc_Absyn_Stmt*s2=_tmp10E;
# 603
s=s2;
continue;}case 3U: _LLB: _tmp10D=((struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct*)_tmpFA)->f1;_LLC: {struct Cyc_Absyn_Exp*eopt=_tmp10D;
# 606
if(eopt == 0)
return;
# 606
_tmp10C=eopt;goto _LLE;}case 1U: _LLD: _tmp10C=((struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct*)_tmpFA)->f1;_LLE: {struct Cyc_Absyn_Exp*e=_tmp10C;
# 610
({Cyc_Tovc_exp_to_vc(env,e);});
return;}case 2U: _LLF: _tmp10A=((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_tmpFA)->f1;_tmp10B=((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_tmpFA)->f2;_LL10: {struct Cyc_Absyn_Stmt*s1=_tmp10A;struct Cyc_Absyn_Stmt*s2=_tmp10B;
# 613
({Cyc_Tovc_stmt_to_vc(env,s1);});
s=s2;
continue;}case 4U: _LL11: _tmp107=((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmpFA)->f1;_tmp108=((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmpFA)->f2;_tmp109=((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmpFA)->f3;_LL12: {struct Cyc_Absyn_Exp*e=_tmp107;struct Cyc_Absyn_Stmt*s1=_tmp108;struct Cyc_Absyn_Stmt*s2=_tmp109;
# 617
({Cyc_Tovc_exp_to_vc(env,e);});
({Cyc_Tovc_stmt_to_vc(env,s1);});
s=s2;
continue;}case 5U: _LL13: _tmp105=(((struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct*)_tmpFA)->f1).f1;_tmp106=((struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct*)_tmpFA)->f2;_LL14: {struct Cyc_Absyn_Exp*e=_tmp105;struct Cyc_Absyn_Stmt*s2=_tmp106;
# 622
({Cyc_Tovc_exp_to_vc(env,e);});
s=s2;
continue;}case 9U: _LL15: _tmp101=((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmpFA)->f1;_tmp102=(((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmpFA)->f2).f1;_tmp103=(((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmpFA)->f3).f1;_tmp104=((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmpFA)->f4;_LL16: {struct Cyc_Absyn_Exp*e1=_tmp101;struct Cyc_Absyn_Exp*e2=_tmp102;struct Cyc_Absyn_Exp*e3=_tmp103;struct Cyc_Absyn_Stmt*s2=_tmp104;
# 626
({Cyc_Tovc_exp_to_vc(env,e1);});
({Cyc_Tovc_exp_to_vc(env,e2);});
({Cyc_Tovc_exp_to_vc(env,e3);});
s=s2;
continue;}case 14U: _LL17: _tmpFF=((struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct*)_tmpFA)->f1;_tmp100=(((struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct*)_tmpFA)->f2).f1;_LL18: {struct Cyc_Absyn_Stmt*s2=_tmpFF;struct Cyc_Absyn_Exp*e=_tmp100;
# 632
({Cyc_Tovc_exp_to_vc(env,e);});
s=s2;
continue;}case 10U: _LL19: _tmpFD=((struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct*)_tmpFA)->f1;_tmpFE=((struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct*)_tmpFA)->f2;_LL1A: {struct Cyc_Absyn_Exp*e=_tmpFD;struct Cyc_List_List*scs=_tmpFE;
# 638
({Cyc_Tovc_exp_to_vc(env,e);});
for(0;scs != 0;scs=scs->tl){
({Cyc_Tovc_stmt_to_vc(env,((struct Cyc_Absyn_Switch_clause*)scs->hd)->body);});}
return;}case 12U: _LL1B: _tmpFB=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmpFA)->f1;_tmpFC=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmpFA)->f2;_LL1C: {struct Cyc_Absyn_Decl*d=_tmpFB;struct Cyc_Absyn_Stmt*s2=_tmpFC;
# 643
{void*_stmttmpD=d->r;void*_tmp10F=_stmttmpD;struct Cyc_Absyn_Vardecl*_tmp110;if(((struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_tmp10F)->tag == 0U){_LL20: _tmp110=((struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_tmp10F)->f1;_LL21: {struct Cyc_Absyn_Vardecl*vd=_tmp110;
# 645
if(Cyc_Tovc_elim_array_initializers){
# 650
void*_stmttmpE=({Cyc_Tcutil_compress(vd->type);});void*_tmp111=_stmttmpE;if(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp111)->tag == 4U){_LL25: _LL26:
# 652
 if(vd->initializer != 0){
void*_stmttmpF=((struct Cyc_Absyn_Exp*)_check_null(vd->initializer))->r;void*_tmp112=_stmttmpF;struct Cyc_List_List*_tmp113;if(((struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*)_tmp112)->tag == 37U){_LL2A: _tmp113=((struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*)_tmp112)->f2;_LL2B: {struct Cyc_List_List*dles=_tmp113;
# 655
vd->initializer=0;{
# 657
struct Cyc_Absyn_Exp*arr=({Cyc_Absyn_var_exp(vd->name,0U);});
struct Cyc_Absyn_Stmt*s3=({Cyc_Absyn_new_stmt(s2->r,0U);});
{unsigned i=0U;for(0;dles != 0;(dles=dles->tl,++ i)){
s3=({struct Cyc_Absyn_Stmt*(*_tmp114)(struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_seq_stmt;struct Cyc_Absyn_Stmt*_tmp115=({struct Cyc_Absyn_Stmt*(*_tmp116)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_exp_stmt;struct Cyc_Absyn_Exp*_tmp117=({struct Cyc_Absyn_Exp*(*_tmp118)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_assign_exp;struct Cyc_Absyn_Exp*_tmp119=({struct Cyc_Absyn_Exp*(*_tmp11A)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_subscript_exp;struct Cyc_Absyn_Exp*_tmp11B=arr;struct Cyc_Absyn_Exp*_tmp11C=({Cyc_Absyn_uint_exp(i,0U);});unsigned _tmp11D=0U;_tmp11A(_tmp11B,_tmp11C,_tmp11D);});struct Cyc_Absyn_Exp*_tmp11E=(*((struct _tuple12*)dles->hd)).f2;unsigned _tmp11F=0U;_tmp118(_tmp119,_tmp11E,_tmp11F);});unsigned _tmp120=0U;_tmp116(_tmp117,_tmp120);});struct Cyc_Absyn_Stmt*_tmp121=s3;unsigned _tmp122=0U;_tmp114(_tmp115,_tmp121,_tmp122);});}}
# 665
s2->r=s3->r;
goto _LL29;}}}else{_LL2C: _LL2D: {
# 668
struct _fat_ptr msg=({struct Cyc_String_pa_PrintArg_struct _tmp126=({struct Cyc_String_pa_PrintArg_struct _tmp145;_tmp145.tag=0U,({
struct _fat_ptr _tmp17F=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_stmt2string(s);}));_tmp145.f1=_tmp17F;});_tmp145;});void*_tmp124[1U];_tmp124[0]=& _tmp126;({struct _fat_ptr _tmp180=({const char*_tmp125="|%s|: array type bad initializer";_tag_fat(_tmp125,sizeof(char),33U);});Cyc_aprintf(_tmp180,_tag_fat(_tmp124,sizeof(void*),1U));});});
(int)_throw(({struct Cyc_Core_Impossible_exn_struct*_tmp123=_cycalloc(sizeof(*_tmp123));_tmp123->tag=Cyc_Core_Impossible,_tmp123->f1=(struct _fat_ptr)msg;_tmp123;}));}}_LL29:;}
# 652
goto _LL24;}else{_LL27: _LL28:
# 673
 goto _LL24;}_LL24:;}
# 645
if(vd->initializer != 0)
# 677
({Cyc_Tovc_exp_to_vc(env,(struct Cyc_Absyn_Exp*)_check_null(vd->initializer));});
# 645
({struct Cyc_List_List*_tmp181=({struct Cyc_List_List*_tmp127=_region_malloc(env.rgn,sizeof(*_tmp127));
# 678
_tmp127->hd=vd,_tmp127->tl=env.all_locals;_tmp127;});
# 645
env.all_locals=_tmp181;});
# 679
s=s2;
continue;}}else{_LL22: _LL23:
 goto _LL1F;}_LL1F:;}
# 683
goto _LL1E;}default: _LL1D: _LL1E:
(int)_throw(({struct Cyc_Core_Impossible_exn_struct*_tmp129=_cycalloc(sizeof(*_tmp129));_tmp129->tag=Cyc_Core_Impossible,({struct _fat_ptr _tmp182=({const char*_tmp128="bad stmt after xlation to C";_tag_fat(_tmp128,sizeof(char),28U);});_tmp129->f1=_tmp182;});_tmp129;}));}_LL0:;}}}
# 689
struct Cyc_List_List*Cyc_Tovc_tovc(struct Cyc_List_List*old_ds){
struct Cyc_List_List*new_ds=0;
for(0;old_ds != 0;old_ds=old_ds->tl){
struct Cyc_Absyn_Decl*next_d=(struct Cyc_Absyn_Decl*)old_ds->hd;
void*_stmttmp10=next_d->r;void*_tmp12B=_stmttmp10;struct Cyc_List_List*_tmp12C;struct Cyc_List_List*_tmp12D;struct Cyc_List_List*_tmp12E;struct Cyc_List_List*_tmp12F;struct Cyc_Absyn_Fndecl*_tmp130;switch(*((int*)_tmp12B)){case 1U: _LL1: _tmp130=((struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*)_tmp12B)->f1;_LL2: {struct Cyc_Absyn_Fndecl*fd=_tmp130;
# 698
fd->cached_type=0;
# 705
if(fd->param_vardecls != 0){
struct _RegionHandle _tmp131=_new_region("rgn");struct _RegionHandle*rgn=& _tmp131;_push_region(rgn);
{struct Cyc_List_List**gen_ds=({struct Cyc_List_List**_tmp13C=_region_malloc(rgn,sizeof(*_tmp13C));*_tmp13C=0;_tmp13C;});
if(fd->param_vardecls == 0){
({struct Cyc_String_pa_PrintArg_struct _tmp134=({struct Cyc_String_pa_PrintArg_struct _tmp146;_tmp146.tag=0U,_tmp146.f1=(struct _fat_ptr)((struct _fat_ptr)*(*fd->name).f2);_tmp146;});void*_tmp132[1U];_tmp132[0]=& _tmp134;({struct Cyc___cycFILE*_tmp184=Cyc_stderr;struct _fat_ptr _tmp183=({const char*_tmp133="No param vardecls for function %s\n";_tag_fat(_tmp133,sizeof(char),35U);});Cyc_fprintf(_tmp184,_tmp183,_tag_fat(_tmp132,sizeof(void*),1U));});});
({Cyc_fflush(Cyc_stderr);});}
# 708
({void(*_tmp135)(struct Cyc_Tovc_ToExpEnv env,struct Cyc_Absyn_Stmt*s)=Cyc_Tovc_stmt_to_vc;struct Cyc_Tovc_ToExpEnv _tmp136=({struct Cyc_Tovc_ToExpEnv _tmp147;_tmp147.rgn=rgn,({
# 713
struct Cyc_List_List*_tmp187=({Cyc_List_rcopy(rgn,(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(fd->param_vardecls))->v);});_tmp147.all_locals=_tmp187;}),({
struct Cyc_Dict_Dict _tmp186=({({struct Cyc_Dict_Dict(*_tmp185)(struct _RegionHandle*,int(*cmp)(struct _fat_ptr*,struct _fat_ptr*))=({struct Cyc_Dict_Dict(*_tmp137)(struct _RegionHandle*,int(*cmp)(struct _fat_ptr*,struct _fat_ptr*))=(struct Cyc_Dict_Dict(*)(struct _RegionHandle*,int(*cmp)(struct _fat_ptr*,struct _fat_ptr*)))Cyc_Dict_rempty;_tmp137;});_tmp185(rgn,Cyc_strptrcmp);});});_tmp147.varmap=_tmp186;}),_tmp147.encloser=fd->body,_tmp147.gen_ds=gen_ds;_tmp147;});struct Cyc_Absyn_Stmt*_tmp138=fd->body;_tmp135(_tmp136,_tmp138);});
# 718
if(*gen_ds != 0){
struct Cyc_Absyn_Vardecl*vd=({struct Cyc_Absyn_Vardecl*_tmp13B=_cycalloc(sizeof(*_tmp13B));_tmp13B->sc=fd->sc,_tmp13B->name=fd->name,_tmp13B->varloc=0U,({struct Cyc_Absyn_Tqual _tmp189=({Cyc_Absyn_const_tqual(0U);});_tmp13B->tq=_tmp189;}),({
void*_tmp188=({Cyc_Tcutil_fndecl2type(fd);});_tmp13B->type=_tmp188;}),_tmp13B->initializer=0,_tmp13B->rgn=0,_tmp13B->attributes=(fd->i).attributes,_tmp13B->escapes=0;_tmp13B;});
# 722
new_ds=({struct Cyc_List_List*_tmp13A=_cycalloc(sizeof(*_tmp13A));({struct Cyc_Absyn_Decl*_tmp18A=({Cyc_Absyn_new_decl((void*)({struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*_tmp139=_cycalloc(sizeof(*_tmp139));_tmp139->tag=0U,_tmp139->f1=vd;_tmp139;}),0U);});_tmp13A->hd=_tmp18A;}),_tmp13A->tl=new_ds;_tmp13A;});
# 724
new_ds=({Cyc_List_append(*gen_ds,new_ds);});}}
# 707
;_pop_region();}
# 705
goto _LL4;}case 0U: _LL3: _LL4:
# 729
 goto _LL6;case 2U: _LL5: _LL6:
 goto _LL8;case 3U: _LL7: _LL8:
 goto _LLA;case 4U: _LL9: _LLA:
 goto _LLC;case 5U: _LLB: _LLC:
 goto _LLE;case 6U: _LLD: _LLE:
 goto _LL10;case 7U: _LLF: _LL10:
 goto _LL12;case 8U: _LL11: _LL12:
# 737
 new_ds=({struct Cyc_List_List*_tmp13D=_cycalloc(sizeof(*_tmp13D));_tmp13D->hd=next_d,_tmp13D->tl=new_ds;_tmp13D;});
goto _LL0;case 9U: _LL13: _tmp12F=((struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct*)_tmp12B)->f2;_LL14: {struct Cyc_List_List*ds2=_tmp12F;
_tmp12E=ds2;goto _LL16;}case 10U: _LL15: _tmp12E=((struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct*)_tmp12B)->f2;_LL16: {struct Cyc_List_List*ds2=_tmp12E;
_tmp12D=ds2;goto _LL18;}case 11U: _LL17: _tmp12D=((struct Cyc_Absyn_ExternC_d_Absyn_Raw_decl_struct*)_tmp12B)->f1;_LL18: {struct Cyc_List_List*ds2=_tmp12D;
_tmp12C=ds2;goto _LL1A;}case 12U: _LL19: _tmp12C=((struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct*)_tmp12B)->f1;_LL1A: {struct Cyc_List_List*ds2=_tmp12C;
goto _LL1C;}case 13U: _LL1B: _LL1C:
 goto _LL1E;case 14U: _LL1D: _LL1E:
 goto _LL20;case 15U: _LL1F: _LL20:
 goto _LL22;default: _LL21: _LL22:
# 747
(int)_throw(({struct Cyc_Core_Impossible_exn_struct*_tmp13F=_cycalloc(sizeof(*_tmp13F));_tmp13F->tag=Cyc_Core_Impossible,({struct _fat_ptr _tmp18B=({const char*_tmp13E="nested translation unit after translation to C";_tag_fat(_tmp13E,sizeof(char),47U);});_tmp13F->f1=_tmp18B;});_tmp13F;}));}_LL0:;}
# 750
return({Cyc_List_imp_rev(new_ds);});}

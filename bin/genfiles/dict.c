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
int Cyc_List_length(struct Cyc_List_List*x);extern char Cyc_List_List_mismatch[14U];struct Cyc_List_List_mismatch_exn_struct{char*tag;};extern char Cyc_List_Nth[4U];struct Cyc_List_Nth_exn_struct{char*tag;};struct Cyc___cycFILE;struct Cyc_String_pa_PrintArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Int_pa_PrintArg_struct{int tag;unsigned long f1;};struct Cyc_Double_pa_PrintArg_struct{int tag;double f1;};struct Cyc_LongDouble_pa_PrintArg_struct{int tag;long double f1;};struct Cyc_ShortPtr_pa_PrintArg_struct{int tag;short*f1;};struct Cyc_IntPtr_pa_PrintArg_struct{int tag;unsigned long*f1;};struct Cyc_ShortPtr_sa_ScanfArg_struct{int tag;short*f1;};struct Cyc_UShortPtr_sa_ScanfArg_struct{int tag;unsigned short*f1;};struct Cyc_IntPtr_sa_ScanfArg_struct{int tag;int*f1;};struct Cyc_UIntPtr_sa_ScanfArg_struct{int tag;unsigned*f1;};struct Cyc_StringPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_DoublePtr_sa_ScanfArg_struct{int tag;double*f1;};struct Cyc_FloatPtr_sa_ScanfArg_struct{int tag;float*f1;};struct Cyc_CharPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};
# 252 "cycboot.h"
int Cyc_getw(struct Cyc___cycFILE*);extern char Cyc_FileCloseError[15U];struct Cyc_FileCloseError_exn_struct{char*tag;};extern char Cyc_FileOpenError[14U];struct Cyc_FileOpenError_exn_struct{char*tag;struct _fat_ptr f1;};struct Cyc_Iter_Iter{void*env;int(*next)(void*env,void*dest);};struct Cyc_Dict_T;struct Cyc_Dict_Dict{int(*rel)(void*,void*);struct _RegionHandle*r;const struct Cyc_Dict_T*t;};extern char Cyc_Dict_Present[8U];struct Cyc_Dict_Present_exn_struct{char*tag;};extern char Cyc_Dict_Absent[7U];struct Cyc_Dict_Absent_exn_struct{char*tag;};
# 62 "dict.h"
struct Cyc_Dict_Dict Cyc_Dict_empty(int(*cmp)(void*,void*));
# 68
struct Cyc_Dict_Dict Cyc_Dict_rempty(struct _RegionHandle*,int(*cmp)(void*,void*));
# 83
int Cyc_Dict_member(struct Cyc_Dict_Dict d,void*k);
# 87
struct Cyc_Dict_Dict Cyc_Dict_insert(struct Cyc_Dict_Dict d,void*k,void*v);
# 104
struct Cyc_Dict_Dict Cyc_Dict_rsingleton(struct _RegionHandle*,int(*cmp)(void*,void*),void*k,void*v);
# 110
void*Cyc_Dict_lookup(struct Cyc_Dict_Dict d,void*k);
# 135 "dict.h"
void*Cyc_Dict_fold_c(void*(*f)(void*,void*,void*,void*),void*env,struct Cyc_Dict_Dict d,void*accum);
# 149
void Cyc_Dict_iter_c(void(*f)(void*,void*,void*),void*env,struct Cyc_Dict_Dict d);
# 170
struct Cyc_Dict_Dict Cyc_Dict_rcopy(struct _RegionHandle*,struct Cyc_Dict_Dict);
# 181
struct Cyc_Dict_Dict Cyc_Dict_rmap(struct _RegionHandle*,void*(*f)(void*),struct Cyc_Dict_Dict d);
# 187
struct Cyc_Dict_Dict Cyc_Dict_rmap_c(struct _RegionHandle*,void*(*f)(void*,void*),void*env,struct Cyc_Dict_Dict d);
# 212
struct Cyc_Dict_Dict Cyc_Dict_intersect_c(void*(*f)(void*,void*,void*,void*),void*env,struct Cyc_Dict_Dict d1,struct Cyc_Dict_Dict d2);
# 218
int Cyc_Dict_forall_c(int(*f)(void*,void*,void*),void*env,struct Cyc_Dict_Dict d);
# 236
struct Cyc_List_List*Cyc_Dict_rto_list(struct _RegionHandle*,struct Cyc_Dict_Dict d);
# 244
struct Cyc_Dict_Dict Cyc_Dict_rfilter(struct _RegionHandle*,int(*f)(void*,void*),struct Cyc_Dict_Dict d);
# 250
struct Cyc_Dict_Dict Cyc_Dict_rfilter_c(struct _RegionHandle*,int(*f)(void*,void*,void*),void*env,struct Cyc_Dict_Dict d);
# 263
struct Cyc_Dict_Dict Cyc_Dict_rdifference(struct _RegionHandle*,struct Cyc_Dict_Dict d1,struct Cyc_Dict_Dict d2);
# 274
struct Cyc_Dict_Dict Cyc_Dict_rdelete(struct _RegionHandle*,struct Cyc_Dict_Dict,void*);char Cyc_Dict_Absent[7U]="Absent";char Cyc_Dict_Present[8U]="Present";
# 27 "dict.cyc"
struct Cyc_Dict_Absent_exn_struct Cyc_Dict_Absent_val={Cyc_Dict_Absent};
struct Cyc_Dict_Present_exn_struct Cyc_Dict_Present_val={Cyc_Dict_Present};
# 30
enum Cyc_Dict_Color{Cyc_Dict_R =0U,Cyc_Dict_B =1U};struct _tuple0{void*f1;void*f2;};struct Cyc_Dict_T{enum Cyc_Dict_Color color;const struct Cyc_Dict_T*left;const struct Cyc_Dict_T*right;struct _tuple0 key_val;};
# 40
struct Cyc_Dict_Dict Cyc_Dict_rempty(struct _RegionHandle*r,int(*comp)(void*,void*)){
return({struct Cyc_Dict_Dict _tmp105;_tmp105.rel=comp,_tmp105.r=r,_tmp105.t=0;_tmp105;});}
# 43
struct Cyc_Dict_Dict Cyc_Dict_empty(int(*comp)(void*,void*)){
return({Cyc_Dict_rempty(Cyc_Core_heap_region,comp);});}
# 46
struct Cyc_Dict_Dict Cyc_Dict_rsingleton(struct _RegionHandle*r,int(*comp)(void*,void*),void*key,void*data){
# 48
return({struct Cyc_Dict_Dict _tmp106;_tmp106.rel=comp,_tmp106.r=r,({const struct Cyc_Dict_T*_tmp120=(const struct Cyc_Dict_T*)({struct Cyc_Dict_T*_tmp2=_region_malloc(r,sizeof(*_tmp2));_tmp2->color=Cyc_Dict_B,_tmp2->left=0,_tmp2->right=0,(_tmp2->key_val).f1=key,(_tmp2->key_val).f2=data;_tmp2;});_tmp106.t=_tmp120;});_tmp106;});}
# 50
struct Cyc_Dict_Dict Cyc_Dict_singleton(int(*comp)(void*,void*),void*key,void*data){
return({Cyc_Dict_rsingleton(Cyc_Core_heap_region,comp,key,data);});}
# 54
struct Cyc_Dict_Dict Cyc_Dict_rshare(struct _RegionHandle*r,struct Cyc_Dict_Dict d){
const struct Cyc_Dict_T*t=d.t;
const struct Cyc_Dict_T*t2=t;
return({struct Cyc_Dict_Dict _tmp107;_tmp107.rel=d.rel,_tmp107.r=r,_tmp107.t=t2;_tmp107;});}
# 54
int Cyc_Dict_is_empty(struct Cyc_Dict_Dict d){
# 61
return d.t == (const struct Cyc_Dict_T*)0;}
# 54
int Cyc_Dict_member(struct Cyc_Dict_Dict d,void*key){
# 65
int(*rel)(void*,void*)=d.rel;
const struct Cyc_Dict_T*t=d.t;
while(t != (const struct Cyc_Dict_T*)0){
int i=({rel(key,(t->key_val).f1);});
if(i < 0)t=t->left;else{
if(i > 0)t=t->right;else{
return 1;}}}
# 73
return 0;}
# 76
void*Cyc_Dict_lookup(struct Cyc_Dict_Dict d,void*key){
int(*rel)(void*,void*)=d.rel;
const struct Cyc_Dict_T*t=d.t;
while(t != (const struct Cyc_Dict_T*)0){
int i=({rel(key,(t->key_val).f1);});
if(i < 0)t=t->left;else{
if(i > 0)t=t->right;else{
return(t->key_val).f2;}}}
# 85
(int)_throw(& Cyc_Dict_Absent_val);}
# 88
void*Cyc_Dict_lookup_other(struct Cyc_Dict_Dict d,int(*cmp)(void*,void*),void*key){
const struct Cyc_Dict_T*t=d.t;
while(t != (const struct Cyc_Dict_T*)0){
int i=({cmp(key,(t->key_val).f1);});
if(i < 0)t=t->left;else{
if(i > 0)t=t->right;else{
return(t->key_val).f2;}}}
# 96
(int)_throw(& Cyc_Dict_Absent_val);}
# 99
void**Cyc_Dict_lookup_opt(struct Cyc_Dict_Dict d,void*key){
int(*rel)(void*,void*)=d.rel;
const struct Cyc_Dict_T*t=d.t;
while(t != (const struct Cyc_Dict_T*)0){
int i=({rel(key,(t->key_val).f1);});
if(i < 0)t=t->left;else{
if(i > 0)t=t->right;else{
return&(t->key_val).f2;}}}
# 108
return 0;}
# 99
int Cyc_Dict_lookup_bool(struct Cyc_Dict_Dict d,void*key,void**ans_place){
# 112
int(*rel)(void*,void*)=d.rel;
const struct Cyc_Dict_T*t=d.t;
while(t != (const struct Cyc_Dict_T*)0){
int i=({rel(key,(t->key_val).f1);});
if(i < 0)t=t->left;else{
if(i > 0)t=t->right;else{
# 119
*ans_place=(t->key_val).f2;
return 1;}}}
# 123
return 0;}struct _tuple1{enum Cyc_Dict_Color f1;const struct Cyc_Dict_T*f2;const struct Cyc_Dict_T*f3;struct _tuple0 f4;};
# 127
static struct Cyc_Dict_T*Cyc_Dict_balance(struct _RegionHandle*r,struct _tuple1 quad){
# 129
struct _tuple1 _tmpC=quad;struct _tuple0 _tmp10;const struct Cyc_Dict_T*_tmpF;const struct Cyc_Dict_T*_tmpE;enum Cyc_Dict_Color _tmpD;struct _tuple0 _tmp17;struct _tuple0 _tmp16;struct _tuple0 _tmp15;const struct Cyc_Dict_T*_tmp14;const struct Cyc_Dict_T*_tmp13;const struct Cyc_Dict_T*_tmp12;const struct Cyc_Dict_T*_tmp11;struct _tuple0 _tmp1E;struct _tuple0 _tmp1D;const struct Cyc_Dict_T*_tmp1C;struct _tuple0 _tmp1B;const struct Cyc_Dict_T*_tmp1A;const struct Cyc_Dict_T*_tmp19;const struct Cyc_Dict_T*_tmp18;struct _tuple0 _tmp25;const struct Cyc_Dict_T*_tmp24;struct _tuple0 _tmp23;struct _tuple0 _tmp22;const struct Cyc_Dict_T*_tmp21;const struct Cyc_Dict_T*_tmp20;const struct Cyc_Dict_T*_tmp1F;struct _tuple0 _tmp2C;const struct Cyc_Dict_T*_tmp2B;struct _tuple0 _tmp2A;const struct Cyc_Dict_T*_tmp29;struct _tuple0 _tmp28;const struct Cyc_Dict_T*_tmp27;const struct Cyc_Dict_T*_tmp26;if(_tmpC.f1 == Cyc_Dict_B){if(_tmpC.f2 != 0){if(((const struct Cyc_Dict_T*)_tmpC.f2)->color == Cyc_Dict_R){if(((const struct Cyc_Dict_T*)_tmpC.f2)->left != 0){if(((const struct Cyc_Dict_T*)((const struct Cyc_Dict_T*)_tmpC.f2)->left)->color == Cyc_Dict_R){_LL1: _tmp26=((_tmpC.f2)->left)->left;_tmp27=((_tmpC.f2)->left)->right;_tmp28=((_tmpC.f2)->left)->key_val;_tmp29=(_tmpC.f2)->right;_tmp2A=(_tmpC.f2)->key_val;_tmp2B=_tmpC.f3;_tmp2C=_tmpC.f4;_LL2: {const struct Cyc_Dict_T*a=_tmp26;const struct Cyc_Dict_T*b=_tmp27;struct _tuple0 x=_tmp28;const struct Cyc_Dict_T*c=_tmp29;struct _tuple0 y=_tmp2A;const struct Cyc_Dict_T*d=_tmp2B;struct _tuple0 z=_tmp2C;
# 131
return({struct Cyc_Dict_T*_tmp2F=_region_malloc(r,sizeof(*_tmp2F));_tmp2F->color=Cyc_Dict_R,({const struct Cyc_Dict_T*_tmp122=(const struct Cyc_Dict_T*)({struct Cyc_Dict_T*_tmp2D=_region_malloc(r,sizeof(*_tmp2D));_tmp2D->color=Cyc_Dict_B,_tmp2D->left=a,_tmp2D->right=b,_tmp2D->key_val=x;_tmp2D;});_tmp2F->left=_tmp122;}),({const struct Cyc_Dict_T*_tmp121=(const struct Cyc_Dict_T*)({struct Cyc_Dict_T*_tmp2E=_region_malloc(r,sizeof(*_tmp2E));_tmp2E->color=Cyc_Dict_B,_tmp2E->left=c,_tmp2E->right=d,_tmp2E->key_val=z;_tmp2E;});_tmp2F->right=_tmp121;}),_tmp2F->key_val=y;_tmp2F;});}}else{if(((const struct Cyc_Dict_T*)_tmpC.f2)->right != 0){if(((const struct Cyc_Dict_T*)((const struct Cyc_Dict_T*)_tmpC.f2)->right)->color == Cyc_Dict_R)goto _LL3;else{if(_tmpC.f3 != 0){if(((const struct Cyc_Dict_T*)_tmpC.f3)->color == Cyc_Dict_R){if(((const struct Cyc_Dict_T*)_tmpC.f3)->left != 0){if(((const struct Cyc_Dict_T*)((const struct Cyc_Dict_T*)_tmpC.f3)->left)->color == Cyc_Dict_R)goto _LL5;else{if(((const struct Cyc_Dict_T*)_tmpC.f3)->right != 0){if(((const struct Cyc_Dict_T*)((const struct Cyc_Dict_T*)_tmpC.f3)->right)->color == Cyc_Dict_R)goto _LL7;else{goto _LL9;}}else{goto _LL9;}}}else{if(((const struct Cyc_Dict_T*)_tmpC.f3)->right != 0){if(((const struct Cyc_Dict_T*)((const struct Cyc_Dict_T*)_tmpC.f3)->right)->color == Cyc_Dict_R)goto _LL7;else{goto _LL9;}}else{goto _LL9;}}}else{goto _LL9;}}else{goto _LL9;}}}else{if(_tmpC.f3 != 0){if(((const struct Cyc_Dict_T*)_tmpC.f3)->color == Cyc_Dict_R){if(((const struct Cyc_Dict_T*)_tmpC.f3)->left != 0){if(((const struct Cyc_Dict_T*)((const struct Cyc_Dict_T*)_tmpC.f3)->left)->color == Cyc_Dict_R)goto _LL5;else{if(((const struct Cyc_Dict_T*)_tmpC.f3)->right != 0){if(((const struct Cyc_Dict_T*)((const struct Cyc_Dict_T*)_tmpC.f3)->right)->color == Cyc_Dict_R)goto _LL7;else{goto _LL9;}}else{goto _LL9;}}}else{if(((const struct Cyc_Dict_T*)_tmpC.f3)->right != 0){if(((const struct Cyc_Dict_T*)((const struct Cyc_Dict_T*)_tmpC.f3)->right)->color == Cyc_Dict_R)goto _LL7;else{goto _LL9;}}else{goto _LL9;}}}else{goto _LL9;}}else{goto _LL9;}}}}else{if(((const struct Cyc_Dict_T*)_tmpC.f2)->right != 0){if(((const struct Cyc_Dict_T*)((const struct Cyc_Dict_T*)_tmpC.f2)->right)->color == Cyc_Dict_R){_LL3: _tmp1F=(_tmpC.f2)->left;_tmp20=((_tmpC.f2)->right)->left;_tmp21=((_tmpC.f2)->right)->right;_tmp22=((_tmpC.f2)->right)->key_val;_tmp23=(_tmpC.f2)->key_val;_tmp24=_tmpC.f3;_tmp25=_tmpC.f4;_LL4: {const struct Cyc_Dict_T*a=_tmp1F;const struct Cyc_Dict_T*b=_tmp20;const struct Cyc_Dict_T*c=_tmp21;struct _tuple0 y=_tmp22;struct _tuple0 x=_tmp23;const struct Cyc_Dict_T*d=_tmp24;struct _tuple0 z=_tmp25;
# 133
return({struct Cyc_Dict_T*_tmp32=_region_malloc(r,sizeof(*_tmp32));_tmp32->color=Cyc_Dict_R,({const struct Cyc_Dict_T*_tmp124=(const struct Cyc_Dict_T*)({struct Cyc_Dict_T*_tmp30=_region_malloc(r,sizeof(*_tmp30));_tmp30->color=Cyc_Dict_B,_tmp30->left=a,_tmp30->right=b,_tmp30->key_val=x;_tmp30;});_tmp32->left=_tmp124;}),({const struct Cyc_Dict_T*_tmp123=(const struct Cyc_Dict_T*)({struct Cyc_Dict_T*_tmp31=_region_malloc(r,sizeof(*_tmp31));_tmp31->color=Cyc_Dict_B,_tmp31->left=c,_tmp31->right=d,_tmp31->key_val=z;_tmp31;});_tmp32->right=_tmp123;}),_tmp32->key_val=y;_tmp32;});}}else{if(_tmpC.f3 != 0){if(((const struct Cyc_Dict_T*)_tmpC.f3)->color == Cyc_Dict_R){if(((const struct Cyc_Dict_T*)_tmpC.f3)->left != 0){if(((const struct Cyc_Dict_T*)((const struct Cyc_Dict_T*)_tmpC.f3)->left)->color == Cyc_Dict_R)goto _LL5;else{if(((const struct Cyc_Dict_T*)_tmpC.f3)->right != 0){if(((const struct Cyc_Dict_T*)((const struct Cyc_Dict_T*)_tmpC.f3)->right)->color == Cyc_Dict_R)goto _LL7;else{goto _LL9;}}else{goto _LL9;}}}else{if(((const struct Cyc_Dict_T*)_tmpC.f3)->right != 0){if(((const struct Cyc_Dict_T*)((const struct Cyc_Dict_T*)_tmpC.f3)->right)->color == Cyc_Dict_R)goto _LL7;else{goto _LL9;}}else{goto _LL9;}}}else{goto _LL9;}}else{goto _LL9;}}}else{if(_tmpC.f3 != 0){if(((const struct Cyc_Dict_T*)_tmpC.f3)->color == Cyc_Dict_R){if(((const struct Cyc_Dict_T*)_tmpC.f3)->left != 0){if(((const struct Cyc_Dict_T*)((const struct Cyc_Dict_T*)_tmpC.f3)->left)->color == Cyc_Dict_R)goto _LL5;else{if(((const struct Cyc_Dict_T*)_tmpC.f3)->right != 0){if(((const struct Cyc_Dict_T*)((const struct Cyc_Dict_T*)_tmpC.f3)->right)->color == Cyc_Dict_R)goto _LL7;else{goto _LL9;}}else{goto _LL9;}}}else{if(((const struct Cyc_Dict_T*)_tmpC.f3)->right != 0){if(((const struct Cyc_Dict_T*)((const struct Cyc_Dict_T*)_tmpC.f3)->right)->color == Cyc_Dict_R)goto _LL7;else{goto _LL9;}}else{goto _LL9;}}}else{goto _LL9;}}else{goto _LL9;}}}}else{if(_tmpC.f3 != 0){if(((const struct Cyc_Dict_T*)_tmpC.f3)->color == Cyc_Dict_R){if(((const struct Cyc_Dict_T*)_tmpC.f3)->left != 0){if(((const struct Cyc_Dict_T*)((const struct Cyc_Dict_T*)_tmpC.f3)->left)->color == Cyc_Dict_R)goto _LL5;else{if(((const struct Cyc_Dict_T*)_tmpC.f3)->right != 0){if(((const struct Cyc_Dict_T*)((const struct Cyc_Dict_T*)_tmpC.f3)->right)->color == Cyc_Dict_R)goto _LL7;else{goto _LL9;}}else{goto _LL9;}}}else{if(((const struct Cyc_Dict_T*)_tmpC.f3)->right != 0){if(((const struct Cyc_Dict_T*)((const struct Cyc_Dict_T*)_tmpC.f3)->right)->color == Cyc_Dict_R)goto _LL7;else{goto _LL9;}}else{goto _LL9;}}}else{goto _LL9;}}else{goto _LL9;}}}else{if(_tmpC.f3 != 0){if(((const struct Cyc_Dict_T*)_tmpC.f3)->color == Cyc_Dict_R){if(((const struct Cyc_Dict_T*)_tmpC.f3)->left != 0){if(((const struct Cyc_Dict_T*)((const struct Cyc_Dict_T*)_tmpC.f3)->left)->color == Cyc_Dict_R){_LL5: _tmp18=_tmpC.f2;_tmp19=((_tmpC.f3)->left)->left;_tmp1A=((_tmpC.f3)->left)->right;_tmp1B=((_tmpC.f3)->left)->key_val;_tmp1C=(_tmpC.f3)->right;_tmp1D=(_tmpC.f3)->key_val;_tmp1E=_tmpC.f4;_LL6: {const struct Cyc_Dict_T*a=_tmp18;const struct Cyc_Dict_T*b=_tmp19;const struct Cyc_Dict_T*c=_tmp1A;struct _tuple0 y=_tmp1B;const struct Cyc_Dict_T*d=_tmp1C;struct _tuple0 z=_tmp1D;struct _tuple0 x=_tmp1E;
# 135
return({struct Cyc_Dict_T*_tmp35=_region_malloc(r,sizeof(*_tmp35));_tmp35->color=Cyc_Dict_R,({const struct Cyc_Dict_T*_tmp126=(const struct Cyc_Dict_T*)({struct Cyc_Dict_T*_tmp33=_region_malloc(r,sizeof(*_tmp33));_tmp33->color=Cyc_Dict_B,_tmp33->left=a,_tmp33->right=b,_tmp33->key_val=x;_tmp33;});_tmp35->left=_tmp126;}),({const struct Cyc_Dict_T*_tmp125=(const struct Cyc_Dict_T*)({struct Cyc_Dict_T*_tmp34=_region_malloc(r,sizeof(*_tmp34));_tmp34->color=Cyc_Dict_B,_tmp34->left=c,_tmp34->right=d,_tmp34->key_val=z;_tmp34;});_tmp35->right=_tmp125;}),_tmp35->key_val=y;_tmp35;});}}else{if(((const struct Cyc_Dict_T*)_tmpC.f3)->right != 0){if(((const struct Cyc_Dict_T*)((const struct Cyc_Dict_T*)_tmpC.f3)->right)->color == Cyc_Dict_R)goto _LL7;else{goto _LL9;}}else{goto _LL9;}}}else{if(((const struct Cyc_Dict_T*)_tmpC.f3)->right != 0){if(((const struct Cyc_Dict_T*)((const struct Cyc_Dict_T*)_tmpC.f3)->right)->color == Cyc_Dict_R){_LL7: _tmp11=_tmpC.f2;_tmp12=(_tmpC.f3)->left;_tmp13=((_tmpC.f3)->right)->left;_tmp14=((_tmpC.f3)->right)->right;_tmp15=((_tmpC.f3)->right)->key_val;_tmp16=(_tmpC.f3)->key_val;_tmp17=_tmpC.f4;_LL8: {const struct Cyc_Dict_T*a=_tmp11;const struct Cyc_Dict_T*b=_tmp12;const struct Cyc_Dict_T*c=_tmp13;const struct Cyc_Dict_T*d=_tmp14;struct _tuple0 z=_tmp15;struct _tuple0 y=_tmp16;struct _tuple0 x=_tmp17;
# 137
return({struct Cyc_Dict_T*_tmp38=_region_malloc(r,sizeof(*_tmp38));_tmp38->color=Cyc_Dict_R,({const struct Cyc_Dict_T*_tmp128=(const struct Cyc_Dict_T*)({struct Cyc_Dict_T*_tmp36=_region_malloc(r,sizeof(*_tmp36));_tmp36->color=Cyc_Dict_B,_tmp36->left=a,_tmp36->right=b,_tmp36->key_val=x;_tmp36;});_tmp38->left=_tmp128;}),({const struct Cyc_Dict_T*_tmp127=(const struct Cyc_Dict_T*)({struct Cyc_Dict_T*_tmp37=_region_malloc(r,sizeof(*_tmp37));_tmp37->color=Cyc_Dict_B,_tmp37->left=c,_tmp37->right=d,_tmp37->key_val=z;_tmp37;});_tmp38->right=_tmp127;}),_tmp38->key_val=y;_tmp38;});}}else{goto _LL9;}}else{goto _LL9;}}}else{goto _LL9;}}else{goto _LL9;}}}else{_LL9: _tmpD=_tmpC.f1;_tmpE=_tmpC.f2;_tmpF=_tmpC.f3;_tmp10=_tmpC.f4;_LLA: {enum Cyc_Dict_Color a=_tmpD;const struct Cyc_Dict_T*b=_tmpE;const struct Cyc_Dict_T*c=_tmpF;struct _tuple0 d=_tmp10;
# 139
return({struct Cyc_Dict_T*_tmp39=_region_malloc(r,sizeof(*_tmp39));_tmp39->color=a,_tmp39->left=b,_tmp39->right=c,_tmp39->key_val=d;_tmp39;});}}_LL0:;}
# 144
static struct Cyc_Dict_T*Cyc_Dict_ins(struct _RegionHandle*r,int(*rel)(void*,void*),struct _tuple0 key_val,const struct Cyc_Dict_T*t){
const struct Cyc_Dict_T*_tmp3B=t;struct _tuple0 _tmp3F;const struct Cyc_Dict_T*_tmp3E;const struct Cyc_Dict_T*_tmp3D;enum Cyc_Dict_Color _tmp3C;if(_tmp3B == 0){_LL1: _LL2:
 return({struct Cyc_Dict_T*_tmp40=_region_malloc(r,sizeof(*_tmp40));_tmp40->color=Cyc_Dict_R,_tmp40->left=0,_tmp40->right=0,_tmp40->key_val=key_val;_tmp40;});}else{_LL3: _tmp3C=_tmp3B->color;_tmp3D=_tmp3B->left;_tmp3E=_tmp3B->right;_tmp3F=_tmp3B->key_val;_LL4: {enum Cyc_Dict_Color color=_tmp3C;const struct Cyc_Dict_T*a=_tmp3D;const struct Cyc_Dict_T*b=_tmp3E;struct _tuple0 y=_tmp3F;
# 148
int i=({rel(key_val.f1,y.f1);});
if(i < 0)return({struct Cyc_Dict_T*(*_tmp41)(struct _RegionHandle*r,struct _tuple1 quad)=Cyc_Dict_balance;struct _RegionHandle*_tmp42=r;struct _tuple1 _tmp43=({struct _tuple1 _tmp108;_tmp108.f1=color,({const struct Cyc_Dict_T*_tmp129=(const struct Cyc_Dict_T*)({Cyc_Dict_ins(r,rel,key_val,a);});_tmp108.f2=_tmp129;}),_tmp108.f3=b,_tmp108.f4=y;_tmp108;});_tmp41(_tmp42,_tmp43);});else{
if(i > 0)return({struct Cyc_Dict_T*(*_tmp44)(struct _RegionHandle*r,struct _tuple1 quad)=Cyc_Dict_balance;struct _RegionHandle*_tmp45=r;struct _tuple1 _tmp46=({struct _tuple1 _tmp109;_tmp109.f1=color,_tmp109.f2=a,({const struct Cyc_Dict_T*_tmp12A=(const struct Cyc_Dict_T*)({Cyc_Dict_ins(r,rel,key_val,b);});_tmp109.f3=_tmp12A;}),_tmp109.f4=y;_tmp109;});_tmp44(_tmp45,_tmp46);});else{
return({struct Cyc_Dict_T*_tmp47=_region_malloc(r,sizeof(*_tmp47));_tmp47->color=color,_tmp47->left=a,_tmp47->right=b,_tmp47->key_val=key_val;_tmp47;});}}}}_LL0:;}
# 155
struct Cyc_Dict_Dict Cyc_Dict_insert(struct Cyc_Dict_Dict d,void*key,void*data){
struct Cyc_Dict_T*ans=({({struct _RegionHandle*_tmp12D=d.r;int(*_tmp12C)(void*,void*)=d.rel;struct _tuple0 _tmp12B=({struct _tuple0 _tmp10B;_tmp10B.f1=key,_tmp10B.f2=data;_tmp10B;});Cyc_Dict_ins(_tmp12D,_tmp12C,_tmp12B,d.t);});});
((struct Cyc_Dict_T*)_check_null(ans))->color=Cyc_Dict_B;
return({struct Cyc_Dict_Dict _tmp10A;_tmp10A.rel=d.rel,_tmp10A.r=d.r,_tmp10A.t=(const struct Cyc_Dict_T*)ans;_tmp10A;});}
# 162
struct Cyc_Dict_Dict Cyc_Dict_insert_new(struct Cyc_Dict_Dict d,void*key,void*data){
if(({Cyc_Dict_member(d,key);}))
(int)_throw(& Cyc_Dict_Absent_val);
# 163
return({Cyc_Dict_insert(d,key,data);});}
# 168
struct Cyc_Dict_Dict Cyc_Dict_inserts(struct Cyc_Dict_Dict d,struct Cyc_List_List*kds){
# 170
for(0;kds != 0;kds=kds->tl){
d=({Cyc_Dict_insert(d,(((struct _tuple0*)kds->hd)[0]).f1,(((struct _tuple0*)kds->hd)[0]).f2);});}
return d;}
# 175
static void*Cyc_Dict_fold_tree(void*(*f)(void*,void*,void*),const struct Cyc_Dict_T*t,void*accum){
void*_tmp4F;void*_tmp4E;const struct Cyc_Dict_T*_tmp4D;const struct Cyc_Dict_T*_tmp4C;_LL1: _tmp4C=t->left;_tmp4D=t->right;_tmp4E=(t->key_val).f1;_tmp4F=(t->key_val).f2;_LL2: {const struct Cyc_Dict_T*left=_tmp4C;const struct Cyc_Dict_T*right=_tmp4D;void*key=_tmp4E;void*val=_tmp4F;
if(left != (const struct Cyc_Dict_T*)0)accum=({Cyc_Dict_fold_tree(f,left,accum);});accum=({f(key,val,accum);});
# 179
if(right != (const struct Cyc_Dict_T*)0)accum=({Cyc_Dict_fold_tree(f,right,accum);});return accum;}}
# 183
void*Cyc_Dict_fold(void*(*f)(void*,void*,void*),struct Cyc_Dict_Dict d,void*accum){
const struct Cyc_Dict_T*t=d.t;
if(t == (const struct Cyc_Dict_T*)0)
return accum;
# 185
return({Cyc_Dict_fold_tree(f,t,accum);});}
# 190
static void*Cyc_Dict_fold_tree_c(void*(*f)(void*,void*,void*,void*),void*env,const struct Cyc_Dict_T*t,void*accum){
# 192
void*_tmp55;void*_tmp54;const struct Cyc_Dict_T*_tmp53;const struct Cyc_Dict_T*_tmp52;_LL1: _tmp52=t->left;_tmp53=t->right;_tmp54=(t->key_val).f1;_tmp55=(t->key_val).f2;_LL2: {const struct Cyc_Dict_T*left=_tmp52;const struct Cyc_Dict_T*right=_tmp53;void*key=_tmp54;void*val=_tmp55;
if(left != (const struct Cyc_Dict_T*)0)
accum=({Cyc_Dict_fold_tree_c(f,env,left,accum);});
# 193
accum=({f(env,key,val,accum);});
# 196
if(right != (const struct Cyc_Dict_T*)0)
accum=({Cyc_Dict_fold_tree_c(f,env,right,accum);});
# 196
return accum;}}
# 201
void*Cyc_Dict_fold_c(void*(*f)(void*,void*,void*,void*),void*env,struct Cyc_Dict_Dict d,void*accum){
const struct Cyc_Dict_T*t=d.t;
if(t == (const struct Cyc_Dict_T*)0)
return accum;
# 203
return({Cyc_Dict_fold_tree_c(f,env,t,accum);});}
# 208
static void Cyc_Dict_app_tree(void*(*f)(void*,void*),const struct Cyc_Dict_T*t){
void*_tmp5B;void*_tmp5A;const struct Cyc_Dict_T*_tmp59;const struct Cyc_Dict_T*_tmp58;_LL1: _tmp58=t->left;_tmp59=t->right;_tmp5A=(t->key_val).f1;_tmp5B=(t->key_val).f2;_LL2: {const struct Cyc_Dict_T*left=_tmp58;const struct Cyc_Dict_T*right=_tmp59;void*key=_tmp5A;void*val=_tmp5B;
if(left != (const struct Cyc_Dict_T*)0)({Cyc_Dict_app_tree(f,left);});({f(key,val);});
# 212
if(right != (const struct Cyc_Dict_T*)0)({Cyc_Dict_app_tree(f,right);});}}
# 215
void Cyc_Dict_app(void*(*f)(void*,void*),struct Cyc_Dict_Dict d){
const struct Cyc_Dict_T*t=d.t;
if(t != (const struct Cyc_Dict_T*)0)
({Cyc_Dict_app_tree(f,t);});}
# 221
static void Cyc_Dict_app_tree_c(void*(*f)(void*,void*,void*),void*env,const struct Cyc_Dict_T*t){
void*_tmp61;void*_tmp60;const struct Cyc_Dict_T*_tmp5F;const struct Cyc_Dict_T*_tmp5E;_LL1: _tmp5E=t->left;_tmp5F=t->right;_tmp60=(t->key_val).f1;_tmp61=(t->key_val).f2;_LL2: {const struct Cyc_Dict_T*left=_tmp5E;const struct Cyc_Dict_T*right=_tmp5F;void*key=_tmp60;void*val=_tmp61;
if(left != (const struct Cyc_Dict_T*)0)({Cyc_Dict_app_tree_c(f,env,left);});({f(env,key,val);});
# 225
if(right != (const struct Cyc_Dict_T*)0)({Cyc_Dict_app_tree_c(f,env,right);});}}
# 228
void Cyc_Dict_app_c(void*(*f)(void*,void*,void*),void*env,struct Cyc_Dict_Dict d){
const struct Cyc_Dict_T*t=d.t;
if(t != (const struct Cyc_Dict_T*)0)
({Cyc_Dict_app_tree_c(f,env,t);});}
# 234
static void Cyc_Dict_iter_tree(void(*f)(void*,void*),const struct Cyc_Dict_T*t){
void*_tmp67;void*_tmp66;const struct Cyc_Dict_T*_tmp65;const struct Cyc_Dict_T*_tmp64;_LL1: _tmp64=t->left;_tmp65=t->right;_tmp66=(t->key_val).f1;_tmp67=(t->key_val).f2;_LL2: {const struct Cyc_Dict_T*left=_tmp64;const struct Cyc_Dict_T*right=_tmp65;void*key=_tmp66;void*val=_tmp67;
if(left != (const struct Cyc_Dict_T*)0)({Cyc_Dict_iter_tree(f,left);});({f(key,val);});
# 238
if(right != (const struct Cyc_Dict_T*)0)({Cyc_Dict_iter_tree(f,right);});}}
# 241
void Cyc_Dict_iter(void(*f)(void*,void*),struct Cyc_Dict_Dict d){
const struct Cyc_Dict_T*t=d.t;
if(t != (const struct Cyc_Dict_T*)0)
({Cyc_Dict_iter_tree(f,t);});}
# 247
static void Cyc_Dict_iter_tree_c(void(*f)(void*,void*,void*),void*env,const struct Cyc_Dict_T*t){
void*_tmp6D;void*_tmp6C;const struct Cyc_Dict_T*_tmp6B;const struct Cyc_Dict_T*_tmp6A;_LL1: _tmp6A=t->left;_tmp6B=t->right;_tmp6C=(t->key_val).f1;_tmp6D=(t->key_val).f2;_LL2: {const struct Cyc_Dict_T*left=_tmp6A;const struct Cyc_Dict_T*right=_tmp6B;void*key=_tmp6C;void*val=_tmp6D;
if(left != (const struct Cyc_Dict_T*)0)({Cyc_Dict_iter_tree_c(f,env,left);});({f(env,key,val);});
# 251
if(right != (const struct Cyc_Dict_T*)0)({Cyc_Dict_iter_tree_c(f,env,right);});}}
# 254
void Cyc_Dict_iter_c(void(*f)(void*,void*,void*),void*env,struct Cyc_Dict_Dict d){
const struct Cyc_Dict_T*t=d.t;
if(t != (const struct Cyc_Dict_T*)0)
({Cyc_Dict_iter_tree_c(f,env,t);});}
# 260
static void Cyc_Dict_count_elem(int*cnt,void*a,void*b){
*cnt=*cnt + 1;}
# 263
int Cyc_Dict_cardinality(struct Cyc_Dict_Dict d){
int num=0;
({({void(*_tmp12E)(void(*f)(int*,void*,void*),int*env,struct Cyc_Dict_Dict d)=({void(*_tmp71)(void(*f)(int*,void*,void*),int*env,struct Cyc_Dict_Dict d)=(void(*)(void(*f)(int*,void*,void*),int*env,struct Cyc_Dict_Dict d))Cyc_Dict_iter_c;_tmp71;});_tmp12E(Cyc_Dict_count_elem,& num,d);});});
return num;}struct _tuple2{void(*f1)(void*,void*);struct Cyc_Dict_Dict f2;};
# 269
static void Cyc_Dict_iter2_f(struct _tuple2*env,void*a,void*b1){
# 271
struct Cyc_Dict_Dict _tmp74;void(*_tmp73)(void*,void*);_LL1: _tmp73=env->f1;_tmp74=env->f2;_LL2: {void(*f)(void*,void*)=_tmp73;struct Cyc_Dict_Dict d2=_tmp74;
({void(*_tmp75)(void*,void*)=f;void*_tmp76=b1;void*_tmp77=({Cyc_Dict_lookup(d2,a);});_tmp75(_tmp76,_tmp77);});}}
# 275
void Cyc_Dict_iter2(void(*f)(void*,void*),struct Cyc_Dict_Dict d1,struct Cyc_Dict_Dict d2){
# 278
struct _tuple2 env=({struct _tuple2 _tmp10C;_tmp10C.f1=f,_tmp10C.f2=d2;_tmp10C;});
({({void(*_tmp12F)(void(*f)(struct _tuple2*,void*,void*),struct _tuple2*env,struct Cyc_Dict_Dict d)=({void(*_tmp79)(void(*f)(struct _tuple2*,void*,void*),struct _tuple2*env,struct Cyc_Dict_Dict d)=(void(*)(void(*f)(struct _tuple2*,void*,void*),struct _tuple2*env,struct Cyc_Dict_Dict d))Cyc_Dict_iter_c;_tmp79;});_tmp12F(Cyc_Dict_iter2_f,& env,d1);});});}struct _tuple3{void(*f1)(void*,void*,void*);struct Cyc_Dict_Dict f2;void*f3;};
# 282
static void Cyc_Dict_iter2_c_f(struct _tuple3*env,void*a,void*b1){
# 284
void*_tmp7D;struct Cyc_Dict_Dict _tmp7C;void(*_tmp7B)(void*,void*,void*);_LL1: _tmp7B=env->f1;_tmp7C=env->f2;_tmp7D=env->f3;_LL2: {void(*f)(void*,void*,void*)=_tmp7B;struct Cyc_Dict_Dict d2=_tmp7C;void*inner_env=_tmp7D;
({void(*_tmp7E)(void*,void*,void*)=f;void*_tmp7F=inner_env;void*_tmp80=b1;void*_tmp81=({Cyc_Dict_lookup(d2,a);});_tmp7E(_tmp7F,_tmp80,_tmp81);});}}
# 288
void Cyc_Dict_iter2_c(void(*f)(void*,void*,void*),void*inner_env,struct Cyc_Dict_Dict d1,struct Cyc_Dict_Dict d2){
# 291
struct _tuple3 env=({struct _tuple3 _tmp10D;_tmp10D.f1=f,_tmp10D.f2=d2,_tmp10D.f3=inner_env;_tmp10D;});
({({void(*_tmp130)(void(*f)(struct _tuple3*,void*,void*),struct _tuple3*env,struct Cyc_Dict_Dict d)=({void(*_tmp83)(void(*f)(struct _tuple3*,void*,void*),struct _tuple3*env,struct Cyc_Dict_Dict d)=(void(*)(void(*f)(struct _tuple3*,void*,void*),struct _tuple3*env,struct Cyc_Dict_Dict d))Cyc_Dict_iter_c;_tmp83;});_tmp130(Cyc_Dict_iter2_c_f,& env,d1);});});}struct _tuple4{void*(*f1)(void*,void*,void*,void*,void*);struct Cyc_Dict_Dict f2;void*f3;};
# 295
static void*Cyc_Dict_fold2_c_f(struct _tuple4*env,void*a,void*b1,void*accum){
# 299
void*_tmp87;struct Cyc_Dict_Dict _tmp86;void*(*_tmp85)(void*,void*,void*,void*,void*);_LL1: _tmp85=env->f1;_tmp86=env->f2;_tmp87=env->f3;_LL2: {void*(*f)(void*,void*,void*,void*,void*)=_tmp85;struct Cyc_Dict_Dict d2=_tmp86;void*inner_env=_tmp87;
return({void*(*_tmp88)(void*,void*,void*,void*,void*)=f;void*_tmp89=inner_env;void*_tmp8A=a;void*_tmp8B=b1;void*_tmp8C=({Cyc_Dict_lookup(d2,a);});void*_tmp8D=accum;_tmp88(_tmp89,_tmp8A,_tmp8B,_tmp8C,_tmp8D);});}}
# 303
void*Cyc_Dict_fold2_c(void*(*f)(void*,void*,void*,void*,void*),void*inner_env,struct Cyc_Dict_Dict d1,struct Cyc_Dict_Dict d2,void*accum){
# 307
struct _tuple4 env=({struct _tuple4 _tmp10E;_tmp10E.f1=f,_tmp10E.f2=d2,_tmp10E.f3=inner_env;_tmp10E;});
return({({void*(*_tmp132)(void*(*f)(struct _tuple4*,void*,void*,void*),struct _tuple4*env,struct Cyc_Dict_Dict d,void*accum)=({void*(*_tmp8F)(void*(*f)(struct _tuple4*,void*,void*,void*),struct _tuple4*env,struct Cyc_Dict_Dict d,void*accum)=(void*(*)(void*(*f)(struct _tuple4*,void*,void*,void*),struct _tuple4*env,struct Cyc_Dict_Dict d,void*accum))Cyc_Dict_fold_c;_tmp8F;});struct Cyc_Dict_Dict _tmp131=d1;_tmp132(Cyc_Dict_fold2_c_f,& env,_tmp131,accum);});});}
# 311
static const struct Cyc_Dict_T*Cyc_Dict_copy_tree(struct _RegionHandle*r2,const struct Cyc_Dict_T*t){
if(t == (const struct Cyc_Dict_T*)0)return 0;else{
# 314
struct Cyc_Dict_T _stmttmp0=*t;struct _tuple0 _tmp94;const struct Cyc_Dict_T*_tmp93;const struct Cyc_Dict_T*_tmp92;enum Cyc_Dict_Color _tmp91;_LL1: _tmp91=_stmttmp0.color;_tmp92=_stmttmp0.left;_tmp93=_stmttmp0.right;_tmp94=_stmttmp0.key_val;_LL2: {enum Cyc_Dict_Color c=_tmp91;const struct Cyc_Dict_T*left=_tmp92;const struct Cyc_Dict_T*right=_tmp93;struct _tuple0 pr=_tmp94;
const struct Cyc_Dict_T*new_left=({Cyc_Dict_copy_tree(r2,left);});
const struct Cyc_Dict_T*new_right=({Cyc_Dict_copy_tree(r2,right);});
return(const struct Cyc_Dict_T*)({struct Cyc_Dict_T*_tmp95=_region_malloc(r2,sizeof(*_tmp95));_tmp95->color=c,_tmp95->left=new_left,_tmp95->right=new_right,_tmp95->key_val=pr;_tmp95;});}}}
# 321
struct Cyc_Dict_Dict Cyc_Dict_rcopy(struct _RegionHandle*r2,struct Cyc_Dict_Dict d){
return({struct Cyc_Dict_Dict _tmp10F;_tmp10F.rel=d.rel,_tmp10F.r=r2,({const struct Cyc_Dict_T*_tmp133=({Cyc_Dict_copy_tree(r2,d.t);});_tmp10F.t=_tmp133;});_tmp10F;});}
# 325
struct Cyc_Dict_Dict Cyc_Dict_copy(struct Cyc_Dict_Dict d){
return({Cyc_Dict_rcopy(Cyc_Core_heap_region,d);});}
# 329
static const struct Cyc_Dict_T*Cyc_Dict_map_tree(struct _RegionHandle*r,void*(*f)(void*),const struct Cyc_Dict_T*t){
# 331
void*_tmp9D;void*_tmp9C;const struct Cyc_Dict_T*_tmp9B;const struct Cyc_Dict_T*_tmp9A;enum Cyc_Dict_Color _tmp99;_LL1: _tmp99=t->color;_tmp9A=t->left;_tmp9B=t->right;_tmp9C=(t->key_val).f1;_tmp9D=(t->key_val).f2;_LL2: {enum Cyc_Dict_Color c=_tmp99;const struct Cyc_Dict_T*left=_tmp9A;const struct Cyc_Dict_T*right=_tmp9B;void*key=_tmp9C;void*val=_tmp9D;
const struct Cyc_Dict_T*new_left=left == (const struct Cyc_Dict_T*)0?0:({Cyc_Dict_map_tree(r,f,left);});
void*new_val=({f(val);});
const struct Cyc_Dict_T*new_right=right == (const struct Cyc_Dict_T*)0?0:({Cyc_Dict_map_tree(r,f,right);});
return(const struct Cyc_Dict_T*)({struct Cyc_Dict_T*_tmp9E=_region_malloc(r,sizeof(*_tmp9E));_tmp9E->color=c,_tmp9E->left=new_left,_tmp9E->right=new_right,(_tmp9E->key_val).f1=key,(_tmp9E->key_val).f2=new_val;_tmp9E;});}}
# 338
struct Cyc_Dict_Dict Cyc_Dict_rmap(struct _RegionHandle*r,void*(*f)(void*),struct Cyc_Dict_Dict d){
const struct Cyc_Dict_T*t=d.t;
if(t == (const struct Cyc_Dict_T*)0)
return({struct Cyc_Dict_Dict _tmp110;_tmp110.rel=d.rel,_tmp110.r=r,_tmp110.t=0;_tmp110;});
# 340
return({struct Cyc_Dict_Dict _tmp111;_tmp111.rel=d.rel,_tmp111.r=r,({
# 342
const struct Cyc_Dict_T*_tmp134=({Cyc_Dict_map_tree(r,f,t);});_tmp111.t=_tmp134;});_tmp111;});}
# 345
struct Cyc_Dict_Dict Cyc_Dict_map(void*(*f)(void*),struct Cyc_Dict_Dict d){
return({Cyc_Dict_rmap(Cyc_Core_heap_region,f,d);});}
# 349
static const struct Cyc_Dict_T*Cyc_Dict_map_tree_c(struct _RegionHandle*r,void*(*f)(void*,void*),void*env,const struct Cyc_Dict_T*t){
# 351
void*_tmpA6;void*_tmpA5;const struct Cyc_Dict_T*_tmpA4;const struct Cyc_Dict_T*_tmpA3;enum Cyc_Dict_Color _tmpA2;_LL1: _tmpA2=t->color;_tmpA3=t->left;_tmpA4=t->right;_tmpA5=(t->key_val).f1;_tmpA6=(t->key_val).f2;_LL2: {enum Cyc_Dict_Color c=_tmpA2;const struct Cyc_Dict_T*left=_tmpA3;const struct Cyc_Dict_T*right=_tmpA4;void*key=_tmpA5;void*val=_tmpA6;
const struct Cyc_Dict_T*new_left=
left == (const struct Cyc_Dict_T*)0?0:({Cyc_Dict_map_tree_c(r,f,env,left);});
void*new_val=({f(env,val);});
const struct Cyc_Dict_T*new_right=
right == (const struct Cyc_Dict_T*)0?0:({Cyc_Dict_map_tree_c(r,f,env,right);});
return(const struct Cyc_Dict_T*)({struct Cyc_Dict_T*_tmpA7=_region_malloc(r,sizeof(*_tmpA7));_tmpA7->color=c,_tmpA7->left=new_left,_tmpA7->right=new_right,(_tmpA7->key_val).f1=key,(_tmpA7->key_val).f2=new_val;_tmpA7;});}}
# 360
struct Cyc_Dict_Dict Cyc_Dict_rmap_c(struct _RegionHandle*r,void*(*f)(void*,void*),void*env,struct Cyc_Dict_Dict d){
# 362
const struct Cyc_Dict_T*t=d.t;
if(t == (const struct Cyc_Dict_T*)0)
return({struct Cyc_Dict_Dict _tmp112;_tmp112.rel=d.rel,_tmp112.r=r,_tmp112.t=0;_tmp112;});
# 363
return({struct Cyc_Dict_Dict _tmp113;_tmp113.rel=d.rel,_tmp113.r=r,({
# 365
const struct Cyc_Dict_T*_tmp135=({Cyc_Dict_map_tree_c(r,f,env,t);});_tmp113.t=_tmp135;});_tmp113;});}
# 368
struct Cyc_Dict_Dict Cyc_Dict_map_c(void*(*f)(void*,void*),void*env,struct Cyc_Dict_Dict d){
return({Cyc_Dict_rmap_c(Cyc_Core_heap_region,f,env,d);});}
# 372
struct _tuple0*Cyc_Dict_rchoose(struct _RegionHandle*r,struct Cyc_Dict_Dict d){
if(d.t == (const struct Cyc_Dict_T*)0)
(int)_throw(& Cyc_Dict_Absent_val);
# 373
return({struct _tuple0*_tmpAB=_region_malloc(r,sizeof(*_tmpAB));
# 375
_tmpAB->f1=((d.t)->key_val).f1,_tmpAB->f2=((d.t)->key_val).f2;_tmpAB;});}
# 378
static int Cyc_Dict_forall_tree_c(int(*f)(void*,void*,void*),void*env,const struct Cyc_Dict_T*t){
void*_tmpB0;void*_tmpAF;const struct Cyc_Dict_T*_tmpAE;const struct Cyc_Dict_T*_tmpAD;_LL1: _tmpAD=t->left;_tmpAE=t->right;_tmpAF=(t->key_val).f1;_tmpB0=(t->key_val).f2;_LL2: {const struct Cyc_Dict_T*left=_tmpAD;const struct Cyc_Dict_T*right=_tmpAE;void*key=_tmpAF;void*val=_tmpB0;
return
((left == (const struct Cyc_Dict_T*)0 ||({Cyc_Dict_forall_tree_c(f,env,left);}))&&({f(env,key,val);}))&&(
# 383
right == (const struct Cyc_Dict_T*)0 ||({Cyc_Dict_forall_tree_c(f,env,right);}));}}
# 378
int Cyc_Dict_forall_c(int(*f)(void*,void*,void*),void*env,struct Cyc_Dict_Dict d){
# 387
const struct Cyc_Dict_T*t=d.t;
if(t == (const struct Cyc_Dict_T*)0)
return 1;
# 388
return({Cyc_Dict_forall_tree_c(f,env,t);});}struct _tuple5{int(*f1)(void*,void*,void*);struct Cyc_Dict_Dict f2;};
# 393
static int Cyc_Dict_forall_intersect_f(struct _tuple5*env,void*a,void*b){
# 395
struct Cyc_Dict_Dict _tmpB4;int(*_tmpB3)(void*,void*,void*);_LL1: _tmpB3=env->f1;_tmpB4=env->f2;_LL2: {int(*f)(void*,void*,void*)=_tmpB3;struct Cyc_Dict_Dict d2=_tmpB4;
if(({Cyc_Dict_member(d2,a);}))
return({int(*_tmpB5)(void*,void*,void*)=f;void*_tmpB6=a;void*_tmpB7=b;void*_tmpB8=({Cyc_Dict_lookup(d2,a);});_tmpB5(_tmpB6,_tmpB7,_tmpB8);});
# 396
return 1;}}
# 393
int Cyc_Dict_forall_intersect(int(*f)(void*,void*,void*),struct Cyc_Dict_Dict d1,struct Cyc_Dict_Dict d2){
# 403
struct _tuple5 env=({struct _tuple5 _tmp114;_tmp114.f1=f,_tmp114.f2=d2;_tmp114;});
return({({int(*_tmp136)(int(*f)(struct _tuple5*,void*,void*),struct _tuple5*env,struct Cyc_Dict_Dict d)=({int(*_tmpBA)(int(*f)(struct _tuple5*,void*,void*),struct _tuple5*env,struct Cyc_Dict_Dict d)=(int(*)(int(*f)(struct _tuple5*,void*,void*),struct _tuple5*env,struct Cyc_Dict_Dict d))Cyc_Dict_forall_c;_tmpBA;});_tmp136(Cyc_Dict_forall_intersect_f,& env,d1);});});}struct _tuple6{void*(*f1)(void*,void*,void*,void*);void*f2;};
# 409
static struct Cyc_Dict_Dict*Cyc_Dict_union_f(struct _tuple6*env,void*a,void*b,struct Cyc_Dict_Dict*d1){
# 412
if(({Cyc_Dict_member(*d1,a);})){
# 414
void*old_val=({Cyc_Dict_lookup(*d1,a);});
void*new_val=({((*env).f1)((*env).f2,a,old_val,b);});
if(new_val != old_val)
({struct Cyc_Dict_Dict _tmp137=({Cyc_Dict_insert(*d1,a,new_val);});*d1=_tmp137;});
# 416
return d1;}
# 412
({struct Cyc_Dict_Dict _tmp138=({Cyc_Dict_insert(*d1,a,b);});*d1=_tmp138;});
# 421
return d1;}
# 424
struct Cyc_Dict_Dict Cyc_Dict_union_two_c(void*(*f)(void*,void*,void*,void*),void*env,struct Cyc_Dict_Dict d1,struct Cyc_Dict_Dict d2){
# 427
if((int)d1.t == (int)d2.t)return d1;if(d1.t == (const struct Cyc_Dict_T*)0)
return d2;
# 427
if(d2.t == (const struct Cyc_Dict_T*)0)
# 429
return d1;{
# 427
struct _tuple6 fenvpair=({struct _tuple6 _tmp115;_tmp115.f1=f,_tmp115.f2=env;_tmp115;});
# 431
({({struct Cyc_Dict_Dict*(*_tmp139)(struct Cyc_Dict_Dict*(*f)(struct _tuple6*,void*,void*,struct Cyc_Dict_Dict*),struct _tuple6*env,struct Cyc_Dict_Dict d,struct Cyc_Dict_Dict*accum)=({struct Cyc_Dict_Dict*(*_tmpBD)(struct Cyc_Dict_Dict*(*f)(struct _tuple6*,void*,void*,struct Cyc_Dict_Dict*),struct _tuple6*env,struct Cyc_Dict_Dict d,struct Cyc_Dict_Dict*accum)=(struct Cyc_Dict_Dict*(*)(struct Cyc_Dict_Dict*(*f)(struct _tuple6*,void*,void*,struct Cyc_Dict_Dict*),struct _tuple6*env,struct Cyc_Dict_Dict d,struct Cyc_Dict_Dict*accum))Cyc_Dict_fold_c;_tmpBD;});_tmp139(Cyc_Dict_union_f,& fenvpair,d2,& d1);});});
return d1;}}
# 435
struct Cyc_Dict_Dict Cyc_Dict_intersect_c(void*(*f)(void*,void*,void*,void*),void*env,struct Cyc_Dict_Dict d1,struct Cyc_Dict_Dict d2){
# 438
const struct Cyc_Dict_T*t2=d2.t;
if(t2 == (const struct Cyc_Dict_T*)0)return d2;if((int)d1.t == (int)t2)
return d2;{
# 439
const struct Cyc_Dict_T*ans_tree=0;
# 442
struct _RegionHandle _tmpBF=_new_region("temp");struct _RegionHandle*temp=& _tmpBF;_push_region(temp);{
# 444
struct _fat_ptr queue=_tag_fat(({unsigned _tmpD1=16U;const struct Cyc_Dict_T**_tmpD0=({struct _RegionHandle*_tmp13A=temp;_region_malloc(_tmp13A,_check_times(_tmpD1,sizeof(const struct Cyc_Dict_T*)));});({{unsigned _tmp118=16U;unsigned i;for(i=0;i < _tmp118;++ i){_tmpD0[i]=t2;}}0;});_tmpD0;}),sizeof(const struct Cyc_Dict_T*),16U);
int ind=0;
while(ind != -1){
const struct Cyc_Dict_T*_stmttmp1=*((const struct Cyc_Dict_T**)_check_fat_subscript(queue,sizeof(const struct Cyc_Dict_T*),ind --));void*_tmpC3;void*_tmpC2;const struct Cyc_Dict_T*_tmpC1;const struct Cyc_Dict_T*_tmpC0;_LL1: _tmpC0=_stmttmp1->left;_tmpC1=_stmttmp1->right;_tmpC2=(_stmttmp1->key_val).f1;_tmpC3=(_stmttmp1->key_val).f2;_LL2: {const struct Cyc_Dict_T*l=_tmpC0;const struct Cyc_Dict_T*r=_tmpC1;void*k=_tmpC2;void*v=_tmpC3;
if((unsigned)(ind + 2)>= _get_fat_size(queue,sizeof(const struct Cyc_Dict_T*)))
queue=({unsigned _tmpC5=_get_fat_size(queue,sizeof(const struct Cyc_Dict_T*))* (unsigned)2;const struct Cyc_Dict_T**_tmpC4=({struct _RegionHandle*_tmp13B=temp;_region_malloc(_tmp13B,_check_times(_tmpC5,sizeof(const struct Cyc_Dict_T*)));});({{unsigned _tmp116=_get_fat_size(queue,sizeof(const struct Cyc_Dict_T*))* (unsigned)2;unsigned i;for(i=0;i < _tmp116;++ i){
i < _get_fat_size(queue,sizeof(const struct Cyc_Dict_T*))?_tmpC4[i]=((const struct Cyc_Dict_T**)queue.curr)[(int)i]:(_tmpC4[i]=t2);}}0;});_tag_fat(_tmpC4,sizeof(const struct Cyc_Dict_T*),_tmpC5);});
# 448
if(l != (const struct Cyc_Dict_T*)0)
# 451
*((const struct Cyc_Dict_T**)_check_fat_subscript(queue,sizeof(const struct Cyc_Dict_T*),++ ind))=l;
# 448
if(r != (const struct Cyc_Dict_T*)0)
# 452
*((const struct Cyc_Dict_T**)_check_fat_subscript(queue,sizeof(const struct Cyc_Dict_T*),++ ind))=r;
# 448
if(({Cyc_Dict_member(d1,k);}))
# 454
ans_tree=(const struct Cyc_Dict_T*)({struct Cyc_Dict_T*(*_tmpC6)(struct _RegionHandle*r,int(*rel)(void*,void*),struct _tuple0 key_val,const struct Cyc_Dict_T*t)=Cyc_Dict_ins;struct _RegionHandle*_tmpC7=d2.r;int(*_tmpC8)(void*,void*)=d2.rel;struct _tuple0 _tmpC9=({struct _tuple0 _tmp117;_tmp117.f1=k,({void*_tmp13C=({void*(*_tmpCA)(void*,void*,void*,void*)=f;void*_tmpCB=env;void*_tmpCC=k;void*_tmpCD=({Cyc_Dict_lookup(d1,k);});void*_tmpCE=v;_tmpCA(_tmpCB,_tmpCC,_tmpCD,_tmpCE);});_tmp117.f2=_tmp13C;});_tmp117;});const struct Cyc_Dict_T*_tmpCF=ans_tree;_tmpC6(_tmpC7,_tmpC8,_tmpC9,_tmpCF);});}}}{
# 457
struct Cyc_Dict_Dict _tmpD2=({struct Cyc_Dict_Dict _tmp119;_tmp119.rel=d2.rel,_tmp119.r=d2.r,_tmp119.t=ans_tree;_tmp119;});_npop_handler(0U);return _tmpD2;}
# 442
;_pop_region();}}
# 460
static void*Cyc_Dict_intersect_f(void*(*f)(void*,void*,void*),void*a,void*b1,void*b2){
return({f(a,b1,b2);});}
# 464
struct Cyc_Dict_Dict Cyc_Dict_intersect(void*(*f)(void*,void*,void*),struct Cyc_Dict_Dict d1,struct Cyc_Dict_Dict d2){
# 467
return({({struct Cyc_Dict_Dict(*_tmp13F)(void*(*f)(void*(*)(void*,void*,void*),void*,void*,void*),void*(*env)(void*,void*,void*),struct Cyc_Dict_Dict d1,struct Cyc_Dict_Dict d2)=({struct Cyc_Dict_Dict(*_tmpD5)(void*(*f)(void*(*)(void*,void*,void*),void*,void*,void*),void*(*env)(void*,void*,void*),struct Cyc_Dict_Dict d1,struct Cyc_Dict_Dict d2)=(struct Cyc_Dict_Dict(*)(void*(*f)(void*(*)(void*,void*,void*),void*,void*,void*),void*(*env)(void*,void*,void*),struct Cyc_Dict_Dict d1,struct Cyc_Dict_Dict d2))Cyc_Dict_intersect_c;_tmpD5;});void*(*_tmp13E)(void*,void*,void*)=f;struct Cyc_Dict_Dict _tmp13D=d1;_tmp13F(Cyc_Dict_intersect_f,_tmp13E,_tmp13D,d2);});});}
# 471
static struct Cyc_List_List*Cyc_Dict_to_list_f(struct _RegionHandle*r,void*k,void*v,struct Cyc_List_List*accum){
return({struct Cyc_List_List*_tmpD8=_region_malloc(r,sizeof(*_tmpD8));({struct _tuple0*_tmp140=({struct _tuple0*_tmpD7=_region_malloc(r,sizeof(*_tmpD7));_tmpD7->f1=k,_tmpD7->f2=v;_tmpD7;});_tmpD8->hd=_tmp140;}),_tmpD8->tl=accum;_tmpD8;});}
# 475
struct Cyc_List_List*Cyc_Dict_rto_list(struct _RegionHandle*r,struct Cyc_Dict_Dict d){
return({({struct Cyc_List_List*(*_tmp142)(struct Cyc_List_List*(*f)(struct _RegionHandle*,void*,void*,struct Cyc_List_List*),struct _RegionHandle*env,struct Cyc_Dict_Dict d,struct Cyc_List_List*accum)=({struct Cyc_List_List*(*_tmpDA)(struct Cyc_List_List*(*f)(struct _RegionHandle*,void*,void*,struct Cyc_List_List*),struct _RegionHandle*env,struct Cyc_Dict_Dict d,struct Cyc_List_List*accum)=(struct Cyc_List_List*(*)(struct Cyc_List_List*(*f)(struct _RegionHandle*,void*,void*,struct Cyc_List_List*),struct _RegionHandle*env,struct Cyc_Dict_Dict d,struct Cyc_List_List*accum))Cyc_Dict_fold_c;_tmpDA;});struct _RegionHandle*_tmp141=r;_tmp142(Cyc_Dict_to_list_f,_tmp141,d,0);});});}
# 478
struct Cyc_List_List*Cyc_Dict_to_list(struct Cyc_Dict_Dict d){
return({Cyc_Dict_rto_list(Cyc_Core_heap_region,d);});}struct _tuple7{int(*f1)(void*,void*);struct _RegionHandle*f2;};
# 482
static struct Cyc_Dict_Dict*Cyc_Dict_filter_f(struct _tuple7*env,void*x,void*y,struct Cyc_Dict_Dict*acc){
# 484
struct _RegionHandle*_tmpDE;int(*_tmpDD)(void*,void*);_LL1: _tmpDD=env->f1;_tmpDE=env->f2;_LL2: {int(*f)(void*,void*)=_tmpDD;struct _RegionHandle*r=_tmpDE;
if(({f(x,y);}))
({struct Cyc_Dict_Dict _tmp143=({Cyc_Dict_insert(*acc,x,y);});*acc=_tmp143;});
# 485
return acc;}}
# 491
struct Cyc_Dict_Dict Cyc_Dict_rfilter(struct _RegionHandle*r2,int(*f)(void*,void*),struct Cyc_Dict_Dict d){
struct _tuple7 env=({struct _tuple7 _tmp11A;_tmp11A.f1=f,_tmp11A.f2=r2;_tmp11A;});
struct Cyc_Dict_Dict res=({Cyc_Dict_rempty(r2,d.rel);});
return*({({struct Cyc_Dict_Dict*(*_tmp144)(struct Cyc_Dict_Dict*(*f)(struct _tuple7*,void*,void*,struct Cyc_Dict_Dict*),struct _tuple7*env,struct Cyc_Dict_Dict d,struct Cyc_Dict_Dict*accum)=({struct Cyc_Dict_Dict*(*_tmpE0)(struct Cyc_Dict_Dict*(*f)(struct _tuple7*,void*,void*,struct Cyc_Dict_Dict*),struct _tuple7*env,struct Cyc_Dict_Dict d,struct Cyc_Dict_Dict*accum)=(struct Cyc_Dict_Dict*(*)(struct Cyc_Dict_Dict*(*f)(struct _tuple7*,void*,void*,struct Cyc_Dict_Dict*),struct _tuple7*env,struct Cyc_Dict_Dict d,struct Cyc_Dict_Dict*accum))Cyc_Dict_fold_c;_tmpE0;});_tmp144(Cyc_Dict_filter_f,& env,d,& res);});});}
# 502
struct Cyc_Dict_Dict Cyc_Dict_filter(int(*f)(void*,void*),struct Cyc_Dict_Dict d){
return({Cyc_Dict_rfilter(Cyc_Core_heap_region,f,d);});}struct _tuple8{int(*f1)(void*,void*,void*);void*f2;struct _RegionHandle*f3;};
# 507
static struct Cyc_Dict_Dict*Cyc_Dict_filter_c_f(struct _tuple8*env,void*x,void*y,struct Cyc_Dict_Dict*acc){
# 510
struct _RegionHandle*_tmpE5;void*_tmpE4;int(*_tmpE3)(void*,void*,void*);_LL1: _tmpE3=env->f1;_tmpE4=env->f2;_tmpE5=env->f3;_LL2: {int(*f)(void*,void*,void*)=_tmpE3;void*f_env=_tmpE4;struct _RegionHandle*r2=_tmpE5;
if(({f(f_env,x,y);}))
({struct Cyc_Dict_Dict _tmp145=({Cyc_Dict_insert(*acc,x,y);});*acc=_tmp145;});
# 511
return acc;}}
# 516
struct Cyc_Dict_Dict Cyc_Dict_rfilter_c(struct _RegionHandle*r2,int(*f)(void*,void*,void*),void*f_env,struct Cyc_Dict_Dict d){
# 518
struct _tuple8 env=({struct _tuple8 _tmp11B;_tmp11B.f1=f,_tmp11B.f2=f_env,_tmp11B.f3=r2;_tmp11B;});
struct Cyc_Dict_Dict res=({Cyc_Dict_rempty(r2,d.rel);});
return*({({struct Cyc_Dict_Dict*(*_tmp146)(struct Cyc_Dict_Dict*(*f)(struct _tuple8*,void*,void*,struct Cyc_Dict_Dict*),struct _tuple8*env,struct Cyc_Dict_Dict d,struct Cyc_Dict_Dict*accum)=({struct Cyc_Dict_Dict*(*_tmpE7)(struct Cyc_Dict_Dict*(*f)(struct _tuple8*,void*,void*,struct Cyc_Dict_Dict*),struct _tuple8*env,struct Cyc_Dict_Dict d,struct Cyc_Dict_Dict*accum)=(struct Cyc_Dict_Dict*(*)(struct Cyc_Dict_Dict*(*f)(struct _tuple8*,void*,void*,struct Cyc_Dict_Dict*),struct _tuple8*env,struct Cyc_Dict_Dict d,struct Cyc_Dict_Dict*accum))Cyc_Dict_fold_c;_tmpE7;});_tmp146(Cyc_Dict_filter_c_f,& env,d,& res);});});}
# 522
struct Cyc_Dict_Dict Cyc_Dict_filter_c(int(*f)(void*,void*,void*),void*f_env,struct Cyc_Dict_Dict d){
# 524
return({Cyc_Dict_rfilter_c(Cyc_Core_heap_region,f,f_env,d);});}
# 528
static int Cyc_Dict_difference_f(struct Cyc_Dict_Dict*d,void*x,void*y){
return !({Cyc_Dict_member(*d,x);});}
# 532
struct Cyc_Dict_Dict Cyc_Dict_rdifference(struct _RegionHandle*r2,struct Cyc_Dict_Dict d1,struct Cyc_Dict_Dict d2){
# 534
return({({struct Cyc_Dict_Dict(*_tmp148)(struct _RegionHandle*r2,int(*f)(struct Cyc_Dict_Dict*,void*,void*),struct Cyc_Dict_Dict*f_env,struct Cyc_Dict_Dict d)=({struct Cyc_Dict_Dict(*_tmpEB)(struct _RegionHandle*r2,int(*f)(struct Cyc_Dict_Dict*,void*,void*),struct Cyc_Dict_Dict*f_env,struct Cyc_Dict_Dict d)=(struct Cyc_Dict_Dict(*)(struct _RegionHandle*r2,int(*f)(struct Cyc_Dict_Dict*,void*,void*),struct Cyc_Dict_Dict*f_env,struct Cyc_Dict_Dict d))Cyc_Dict_rfilter_c;_tmpEB;});struct _RegionHandle*_tmp147=r2;_tmp148(_tmp147,Cyc_Dict_difference_f,& d2,d1);});});}
# 536
struct Cyc_Dict_Dict Cyc_Dict_difference(struct Cyc_Dict_Dict d1,struct Cyc_Dict_Dict d2){
return({Cyc_Dict_rdifference(Cyc_Core_heap_region,d1,d2);});}struct _tuple9{int(*f1)(void*,void*);void*f2;};
# 540
static int Cyc_Dict_delete_f(struct _tuple9*env,void*x,void*y){
void*_tmpEF;int(*_tmpEE)(void*,void*);_LL1: _tmpEE=env->f1;_tmpEF=env->f2;_LL2: {int(*rel)(void*,void*)=_tmpEE;void*x0=_tmpEF;
return({rel(x0,x);})!= 0;}}
# 545
struct Cyc_Dict_Dict Cyc_Dict_rdelete(struct _RegionHandle*r2,struct Cyc_Dict_Dict d,void*x){
if(!({Cyc_Dict_member(d,x);}))return({Cyc_Dict_rcopy(r2,d);});{struct _tuple9 env=({struct _tuple9 _tmp11C;_tmp11C.f1=d.rel,_tmp11C.f2=x;_tmp11C;});
# 548
return({({struct Cyc_Dict_Dict(*_tmp14A)(struct _RegionHandle*r2,int(*f)(struct _tuple9*,void*,void*),struct _tuple9*f_env,struct Cyc_Dict_Dict d)=({struct Cyc_Dict_Dict(*_tmpF1)(struct _RegionHandle*r2,int(*f)(struct _tuple9*,void*,void*),struct _tuple9*f_env,struct Cyc_Dict_Dict d)=(struct Cyc_Dict_Dict(*)(struct _RegionHandle*r2,int(*f)(struct _tuple9*,void*,void*),struct _tuple9*f_env,struct Cyc_Dict_Dict d))Cyc_Dict_rfilter_c;_tmpF1;});struct _RegionHandle*_tmp149=r2;_tmp14A(_tmp149,Cyc_Dict_delete_f,& env,d);});});}}
# 551
struct Cyc_Dict_Dict Cyc_Dict_rdelete_same(struct Cyc_Dict_Dict d,void*x){
if(!({Cyc_Dict_member(d,x);}))return d;{struct _tuple9 env=({struct _tuple9 _tmp11D;_tmp11D.f1=d.rel,_tmp11D.f2=x;_tmp11D;});
# 554
return({({struct Cyc_Dict_Dict(*_tmp14C)(struct _RegionHandle*r2,int(*f)(struct _tuple9*,void*,void*),struct _tuple9*f_env,struct Cyc_Dict_Dict d)=({struct Cyc_Dict_Dict(*_tmpF3)(struct _RegionHandle*r2,int(*f)(struct _tuple9*,void*,void*),struct _tuple9*f_env,struct Cyc_Dict_Dict d)=(struct Cyc_Dict_Dict(*)(struct _RegionHandle*r2,int(*f)(struct _tuple9*,void*,void*),struct _tuple9*f_env,struct Cyc_Dict_Dict d))Cyc_Dict_rfilter_c;_tmpF3;});struct _RegionHandle*_tmp14B=d.r;_tmp14C(_tmp14B,Cyc_Dict_delete_f,& env,d);});});}}
# 557
struct Cyc_Dict_Dict Cyc_Dict_delete(struct Cyc_Dict_Dict d,void*x){
return({Cyc_Dict_rdelete(Cyc_Core_heap_region,d,x);});}struct _tuple10{struct _fat_ptr f1;int f2;};
# 557
int Cyc_Dict_iter_f(struct _tuple10*stk,struct _tuple0*dest){
# 564
int*_tmpF7;struct _fat_ptr _tmpF6;_LL1: _tmpF6=stk->f1;_tmpF7=(int*)& stk->f2;_LL2: {struct _fat_ptr stack=_tmpF6;int*indp=_tmpF7;
int ind=*indp;
if(ind == -1)
return 0;{
# 566
const struct Cyc_Dict_T*t=*((const struct Cyc_Dict_T**)_check_fat_subscript(stack,sizeof(const struct Cyc_Dict_T*),ind));
# 569
*dest=((const struct Cyc_Dict_T*)_check_null(t))->key_val;
-- ind;
if((unsigned)t->left)
*((const struct Cyc_Dict_T**)_check_fat_subscript(stack,sizeof(const struct Cyc_Dict_T*),++ ind))=t->left;
# 571
if((unsigned)t->right)
# 574
*((const struct Cyc_Dict_T**)_check_fat_subscript(stack,sizeof(const struct Cyc_Dict_T*),++ ind))=t->right;
# 571
*indp=ind;
# 576
return 1;}}}
# 579
struct Cyc_Iter_Iter Cyc_Dict_make_iter(struct _RegionHandle*rgn,struct Cyc_Dict_Dict d){
# 584
int half_max_size=1;
const struct Cyc_Dict_T*t=d.t;
while(t != (const struct Cyc_Dict_T*)0){
t=t->left;
++ half_max_size;}
# 590
t=d.t;{
struct _fat_ptr stack=({unsigned _tmpFB=(unsigned)(2 * half_max_size);const struct Cyc_Dict_T**_tmpFA=({struct _RegionHandle*_tmp14D=rgn;_region_malloc(_tmp14D,_check_times(_tmpFB,sizeof(const struct Cyc_Dict_T*)));});({{unsigned _tmp11F=(unsigned)(2 * half_max_size);unsigned i;for(i=0;i < _tmp11F;++ i){_tmpFA[i]=t;}}0;});_tag_fat(_tmpFA,sizeof(const struct Cyc_Dict_T*),_tmpFB);});
return({struct Cyc_Iter_Iter _tmp11E;({struct _tuple10*_tmp14E=({struct _tuple10*_tmpF9=_region_malloc(rgn,sizeof(*_tmpF9));_tmpF9->f1=stack,(unsigned)t?_tmpF9->f2=0:(_tmpF9->f2=- 1);_tmpF9;});_tmp11E.env=_tmp14E;}),_tmp11E.next=Cyc_Dict_iter_f;_tmp11E;});}}
# 600
void*Cyc_Dict_marshal(struct _RegionHandle*rgn,void*env,void*(*write_key)(void*,struct Cyc___cycFILE*,void*),void*(*write_val)(void*,struct Cyc___cycFILE*,void*),struct Cyc___cycFILE*fp,struct Cyc_Dict_Dict dict){
# 607
struct Cyc_List_List*dict_list=({Cyc_Dict_rto_list(rgn,dict);});
int len=({Cyc_List_length(dict_list);});
# 611
(int)_throw(({struct Cyc_Core_Failure_exn_struct*_tmpFE=_cycalloc(sizeof(*_tmpFE));_tmpFE->tag=Cyc_Core_Failure,({struct _fat_ptr _tmp14F=({const char*_tmpFD="Dict::marshal: Write failure";_tag_fat(_tmpFD,sizeof(char),29U);});_tmpFE->f1=_tmp14F;});_tmpFE;}));
# 613
while(dict_list != 0){
env=({({void*(*_tmp152)(void*,struct Cyc___cycFILE*,struct _tuple0*)=({void*(*_tmpFF)(void*,struct Cyc___cycFILE*,struct _tuple0*)=(void*(*)(void*,struct Cyc___cycFILE*,struct _tuple0*))write_key;_tmpFF;});void*_tmp151=env;struct Cyc___cycFILE*_tmp150=fp;_tmp152(_tmp151,_tmp150,(struct _tuple0*)dict_list->hd);});});
env=({({void*(*_tmp155)(void*,struct Cyc___cycFILE*,struct _tuple0*)=({void*(*_tmp100)(void*,struct Cyc___cycFILE*,struct _tuple0*)=(void*(*)(void*,struct Cyc___cycFILE*,struct _tuple0*))write_val;_tmp100;});void*_tmp154=env;struct Cyc___cycFILE*_tmp153=fp;_tmp155(_tmp154,_tmp153,(struct _tuple0*)dict_list->hd);});});
dict_list=dict_list->tl;}
# 618
return env;}
# 621
struct Cyc_Dict_Dict Cyc_Dict_unmarshal(struct _RegionHandle*rgn,void*env,int(*cmp)(void*,void*),void*(*read_key)(void*,struct Cyc___cycFILE*),void*(*read_val)(void*,struct Cyc___cycFILE*),struct Cyc___cycFILE*fp){
# 628
struct Cyc_Dict_Dict dict=({Cyc_Dict_empty(cmp);});
int len=({Cyc_getw(fp);});
if(len == -1)
(int)_throw(({struct Cyc_Core_Failure_exn_struct*_tmp103=_cycalloc(sizeof(*_tmp103));_tmp103->tag=Cyc_Core_Failure,({struct _fat_ptr _tmp156=({const char*_tmp102="Dict::unmarshal: list length is -1";_tag_fat(_tmp102,sizeof(char),35U);});_tmp103->f1=_tmp156;});_tmp103;}));
# 630
{
# 632
int i=0;
# 630
for(0;i < len;++ i){
# 633
void*key=({read_key(env,fp);});
void*val=({read_val(env,fp);});
dict=({Cyc_Dict_insert(dict,key,val);});}}
# 637
return dict;}

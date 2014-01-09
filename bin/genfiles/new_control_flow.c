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
# 111 "core.h"
void*Cyc_Core_snd(struct _tuple0*);
# 126
int Cyc_Core_ptrcmp(void*,void*);extern char Cyc_Core_Invalid_argument[17U];struct Cyc_Core_Invalid_argument_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Failure[8U];struct Cyc_Core_Failure_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Impossible[11U];struct Cyc_Core_Impossible_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Not_found[10U];struct Cyc_Core_Not_found_exn_struct{char*tag;};extern char Cyc_Core_Unreachable[12U];struct Cyc_Core_Unreachable_exn_struct{char*tag;struct _fat_ptr f1;};
# 165
extern struct _RegionHandle*Cyc_Core_heap_region;
# 168
extern struct _RegionHandle*Cyc_Core_unique_region;struct Cyc_Core_DynamicRegion;struct Cyc_Core_NewDynamicRegion{struct Cyc_Core_DynamicRegion*key;};
# 275 "core.h"
void Cyc_Core_rethrow(void*);struct Cyc_Core_ThinRes{void*arr;unsigned nelts;};struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};
# 54 "list.h"
struct Cyc_List_List*Cyc_List_list(struct _fat_ptr);
# 61
int Cyc_List_length(struct Cyc_List_List*x);
# 70
struct Cyc_List_List*Cyc_List_copy(struct Cyc_List_List*x);
# 76
struct Cyc_List_List*Cyc_List_map(void*(*f)(void*),struct Cyc_List_List*x);extern char Cyc_List_List_mismatch[14U];struct Cyc_List_List_mismatch_exn_struct{char*tag;};
# 135
void Cyc_List_iter_c(void(*f)(void*,void*),void*env,struct Cyc_List_List*x);
# 172
struct Cyc_List_List*Cyc_List_rev(struct Cyc_List_List*x);
# 178
struct Cyc_List_List*Cyc_List_imp_rev(struct Cyc_List_List*x);
# 184
struct Cyc_List_List*Cyc_List_append(struct Cyc_List_List*x,struct Cyc_List_List*y);extern char Cyc_List_Nth[4U];struct Cyc_List_Nth_exn_struct{char*tag;};
# 242
void*Cyc_List_nth(struct Cyc_List_List*x,int n);
# 276
struct Cyc_List_List*Cyc_List_rzip(struct _RegionHandle*r1,struct _RegionHandle*r2,struct Cyc_List_List*x,struct Cyc_List_List*y);struct _tuple1{struct Cyc_List_List*f1;struct Cyc_List_List*f2;};
# 294
struct _tuple1 Cyc_List_split(struct Cyc_List_List*x);
# 319
int Cyc_List_memq(struct Cyc_List_List*l,void*x);
# 37 "position.h"
struct _fat_ptr Cyc_Position_string_of_segment(unsigned);struct Cyc_Position_Error;
# 47
extern int Cyc_Position_num_errors;struct Cyc___cycFILE;
# 53 "cycboot.h"
extern struct Cyc___cycFILE*Cyc_stderr;struct Cyc_String_pa_PrintArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Int_pa_PrintArg_struct{int tag;unsigned long f1;};struct Cyc_Double_pa_PrintArg_struct{int tag;double f1;};struct Cyc_LongDouble_pa_PrintArg_struct{int tag;long double f1;};struct Cyc_ShortPtr_pa_PrintArg_struct{int tag;short*f1;};struct Cyc_IntPtr_pa_PrintArg_struct{int tag;unsigned long*f1;};
# 100
int Cyc_fprintf(struct Cyc___cycFILE*,struct _fat_ptr,struct _fat_ptr);struct Cyc_ShortPtr_sa_ScanfArg_struct{int tag;short*f1;};struct Cyc_UShortPtr_sa_ScanfArg_struct{int tag;unsigned short*f1;};struct Cyc_IntPtr_sa_ScanfArg_struct{int tag;int*f1;};struct Cyc_UIntPtr_sa_ScanfArg_struct{int tag;unsigned*f1;};struct Cyc_StringPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_DoublePtr_sa_ScanfArg_struct{int tag;double*f1;};struct Cyc_FloatPtr_sa_ScanfArg_struct{int tag;float*f1;};struct Cyc_CharPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};extern char Cyc_FileCloseError[15U];struct Cyc_FileCloseError_exn_struct{char*tag;};extern char Cyc_FileOpenError[14U];struct Cyc_FileOpenError_exn_struct{char*tag;struct _fat_ptr f1;};struct Cyc_Iter_Iter{void*env;int(*next)(void*env,void*dest);};struct Cyc_Dict_T;struct Cyc_Dict_Dict{int(*rel)(void*,void*);struct _RegionHandle*r;const struct Cyc_Dict_T*t;};extern char Cyc_Dict_Present[8U];struct Cyc_Dict_Present_exn_struct{char*tag;};extern char Cyc_Dict_Absent[7U];struct Cyc_Dict_Absent_exn_struct{char*tag;};
# 87 "dict.h"
struct Cyc_Dict_Dict Cyc_Dict_insert(struct Cyc_Dict_Dict d,void*k,void*v);
# 113
void*Cyc_Dict_lookup_other(struct Cyc_Dict_Dict d,int(*cmp)(void*,void*),void*k);
# 126 "dict.h"
int Cyc_Dict_lookup_bool(struct Cyc_Dict_Dict d,void*k,void**ans);
# 149
void Cyc_Dict_iter_c(void(*f)(void*,void*,void*),void*env,struct Cyc_Dict_Dict d);struct Cyc_Relations_Reln;struct _union_Nmspace_Rel_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Abs_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_C_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Loc_n{int tag;int val;};union Cyc_Absyn_Nmspace{struct _union_Nmspace_Rel_n Rel_n;struct _union_Nmspace_Abs_n Abs_n;struct _union_Nmspace_C_n C_n;struct _union_Nmspace_Loc_n Loc_n;};struct _tuple2{union Cyc_Absyn_Nmspace f1;struct _fat_ptr*f2;};
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
enum Cyc_Absyn_KindQual{Cyc_Absyn_AnyKind =0U,Cyc_Absyn_MemKind =1U,Cyc_Absyn_BoxKind =2U,Cyc_Absyn_XRgnKind =3U,Cyc_Absyn_RgnKind =4U,Cyc_Absyn_EffKind =5U,Cyc_Absyn_IntKind =6U,Cyc_Absyn_BoolKind =7U,Cyc_Absyn_PtrBndKind =8U};struct Cyc_Absyn_Kind{enum Cyc_Absyn_KindQual kind;enum Cyc_Absyn_AliasQual aliasqual;};struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct{int tag;struct Cyc_Absyn_Kind*f1;};struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct{int tag;struct Cyc_Core_Opt*f1;};struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct{int tag;struct Cyc_Core_Opt*f1;struct Cyc_Absyn_Kind*f2;};struct Cyc_Absyn_Tvar{struct _fat_ptr*name;int identity;void*kind;};struct Cyc_Absyn_PtrLoc{unsigned ptr_loc;unsigned rgn_loc;unsigned zt_loc;};struct Cyc_Absyn_PtrAtts{void*rgn;void*nullable;void*bounds;void*zero_term;struct Cyc_Absyn_PtrLoc*ptrloc;};struct Cyc_Absyn_PtrInfo{void*elt_type;struct Cyc_Absyn_Tqual elt_tq;struct Cyc_Absyn_PtrAtts ptr_atts;};struct Cyc_Absyn_VarargInfo{struct _fat_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;int inject;};struct Cyc_Absyn_Star_Absyn_Cap_struct{int tag;};struct Cyc_Absyn_Bot_Absyn_Cap_struct{int tag;};struct Cyc_Absyn_Nat_Absyn_Cap_struct{int tag;int f1;int f2;};struct Cyc_Absyn_RgnEffect{void*name;struct Cyc_List_List*caps;void*parent;};struct Cyc_Absyn_FnInfo{struct Cyc_List_List*tvars;void*effect;struct Cyc_Absyn_Tqual ret_tqual;void*ret_type;struct Cyc_List_List*args;int c_varargs;struct Cyc_Absyn_VarargInfo*cyc_varargs;struct Cyc_List_List*rgn_po;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*requires_clause;struct Cyc_List_List*requires_relns;struct Cyc_Absyn_Exp*ensures_clause;struct Cyc_List_List*ensures_relns;struct Cyc_List_List*ieffect;struct Cyc_List_List*oeffect;struct Cyc_List_List*throws;int reentrant;};struct Cyc_Absyn_UnknownDatatypeInfo{struct _tuple2*name;int is_extensible;};struct _union_DatatypeInfo_UnknownDatatype{int tag;struct Cyc_Absyn_UnknownDatatypeInfo val;};struct _union_DatatypeInfo_KnownDatatype{int tag;struct Cyc_Absyn_Datatypedecl**val;};union Cyc_Absyn_DatatypeInfo{struct _union_DatatypeInfo_UnknownDatatype UnknownDatatype;struct _union_DatatypeInfo_KnownDatatype KnownDatatype;};struct Cyc_Absyn_UnknownDatatypeFieldInfo{struct _tuple2*datatype_name;struct _tuple2*field_name;int is_extensible;};struct _union_DatatypeFieldInfo_UnknownDatatypefield{int tag;struct Cyc_Absyn_UnknownDatatypeFieldInfo val;};struct _tuple3{struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;};struct _union_DatatypeFieldInfo_KnownDatatypefield{int tag;struct _tuple3 val;};union Cyc_Absyn_DatatypeFieldInfo{struct _union_DatatypeFieldInfo_UnknownDatatypefield UnknownDatatypefield;struct _union_DatatypeFieldInfo_KnownDatatypefield KnownDatatypefield;};struct _tuple4{enum Cyc_Absyn_AggrKind f1;struct _tuple2*f2;struct Cyc_Core_Opt*f3;};struct _union_AggrInfo_UnknownAggr{int tag;struct _tuple4 val;};struct _union_AggrInfo_KnownAggr{int tag;struct Cyc_Absyn_Aggrdecl**val;};union Cyc_Absyn_AggrInfo{struct _union_AggrInfo_UnknownAggr UnknownAggr;struct _union_AggrInfo_KnownAggr KnownAggr;};struct Cyc_Absyn_ArrayInfo{void*elt_type;struct Cyc_Absyn_Tqual tq;struct Cyc_Absyn_Exp*num_elts;void*zero_term;unsigned zt_loc;};struct Cyc_Absyn_Aggr_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Enum_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;};struct Cyc_Absyn_Datatype_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;};struct Cyc_Absyn_TypeDecl{void*r;unsigned loc;};struct Cyc_Absyn_VoidCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_IntCon_Absyn_TyCon_struct{int tag;enum Cyc_Absyn_Sign f1;enum Cyc_Absyn_Size_of f2;};struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct{int tag;int f1;};struct Cyc_Absyn_RgnHandleCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_XRgnHandleCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_TagCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_HeapCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_UniqueCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_RefCntCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_AccessCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_CAccessCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_JoinCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_RgnsCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_TrueCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_FalseCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_ThinCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_FatCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct{int tag;struct _tuple2*f1;struct Cyc_Absyn_Enumdecl*f2;};struct Cyc_Absyn_AnonEnumCon_Absyn_TyCon_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_BuiltinCon_Absyn_TyCon_struct{int tag;struct _fat_ptr f1;struct Cyc_Absyn_Kind*f2;};struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_DatatypeInfo f1;};struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_DatatypeFieldInfo f1;};struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_AggrInfo f1;};struct Cyc_Absyn_AppType_Absyn_Type_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Evar_Absyn_Type_struct{int tag;struct Cyc_Core_Opt*f1;void*f2;int f3;struct Cyc_Core_Opt*f4;};struct Cyc_Absyn_VarType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Tvar*f1;};struct Cyc_Absyn_PointerType_Absyn_Type_struct{int tag;struct Cyc_Absyn_PtrInfo f1;};struct Cyc_Absyn_ArrayType_Absyn_Type_struct{int tag;struct Cyc_Absyn_ArrayInfo f1;};struct Cyc_Absyn_FnType_Absyn_Type_struct{int tag;struct Cyc_Absyn_FnInfo f1;};struct Cyc_Absyn_TupleType_Absyn_Type_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct{int tag;enum Cyc_Absyn_AggrKind f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_TypedefType_Absyn_Type_struct{int tag;struct _tuple2*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_Typedefdecl*f3;void*f4;};struct Cyc_Absyn_ValueofType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct{int tag;struct Cyc_Absyn_TypeDecl*f1;void**f2;};struct Cyc_Absyn_TypeofType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_NoTypes_Absyn_Funcparams_struct{int tag;struct Cyc_List_List*f1;unsigned f2;};struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct{int tag;struct Cyc_List_List*f1;int f2;struct Cyc_Absyn_VarargInfo*f3;void*f4;struct Cyc_List_List*f5;struct Cyc_Absyn_Exp*f6;struct Cyc_Absyn_Exp*f7;struct Cyc_List_List*f8;struct Cyc_List_List*f9;struct Cyc_List_List*f10;int f11;};
# 478 "absyn.h"
enum Cyc_Absyn_Format_Type{Cyc_Absyn_Printf_ft =0U,Cyc_Absyn_Scanf_ft =1U};struct Cyc_Absyn_Regparm_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Stdcall_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Cdecl_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Fastcall_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Noreturn_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Const_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Aligned_att_Absyn_Attribute_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Packed_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Section_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Nocommon_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Shared_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Unused_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Weak_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Dllimport_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Dllexport_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_instrument_function_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Constructor_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Destructor_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_check_memory_usage_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Format_att_Absyn_Attribute_struct{int tag;enum Cyc_Absyn_Format_Type f1;int f2;int f3;};struct Cyc_Absyn_Initializes_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Noliveunique_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Consume_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Pure_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Mode_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Alias_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Always_inline_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_throw_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Non_null_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Deprecated_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_VectorSize_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Carray_mod_Absyn_Type_modifier_struct{int tag;void*f1;unsigned f2;};struct Cyc_Absyn_ConstArray_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;unsigned f3;};struct Cyc_Absyn_Pointer_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_Absyn_PtrAtts f1;struct Cyc_Absyn_Tqual f2;};struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct{int tag;void*f1;};struct Cyc_Absyn_TypeParams_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_List_List*f1;unsigned f2;int f3;};struct Cyc_Absyn_Attributes_mod_Absyn_Type_modifier_struct{int tag;unsigned f1;struct Cyc_List_List*f2;};struct _union_Cnst_Null_c{int tag;int val;};struct _tuple5{enum Cyc_Absyn_Sign f1;char f2;};struct _union_Cnst_Char_c{int tag;struct _tuple5 val;};struct _union_Cnst_Wchar_c{int tag;struct _fat_ptr val;};struct _tuple6{enum Cyc_Absyn_Sign f1;short f2;};struct _union_Cnst_Short_c{int tag;struct _tuple6 val;};struct _tuple7{enum Cyc_Absyn_Sign f1;int f2;};struct _union_Cnst_Int_c{int tag;struct _tuple7 val;};struct _tuple8{enum Cyc_Absyn_Sign f1;long long f2;};struct _union_Cnst_LongLong_c{int tag;struct _tuple8 val;};struct _tuple9{struct _fat_ptr f1;int f2;};struct _union_Cnst_Float_c{int tag;struct _tuple9 val;};struct _union_Cnst_String_c{int tag;struct _fat_ptr val;};struct _union_Cnst_Wstring_c{int tag;struct _fat_ptr val;};union Cyc_Absyn_Cnst{struct _union_Cnst_Null_c Null_c;struct _union_Cnst_Char_c Char_c;struct _union_Cnst_Wchar_c Wchar_c;struct _union_Cnst_Short_c Short_c;struct _union_Cnst_Int_c Int_c;struct _union_Cnst_LongLong_c LongLong_c;struct _union_Cnst_Float_c Float_c;struct _union_Cnst_String_c String_c;struct _union_Cnst_Wstring_c Wstring_c;};
# 574
enum Cyc_Absyn_Primop{Cyc_Absyn_Plus =0U,Cyc_Absyn_Times =1U,Cyc_Absyn_Minus =2U,Cyc_Absyn_Div =3U,Cyc_Absyn_Mod =4U,Cyc_Absyn_Eq =5U,Cyc_Absyn_Neq =6U,Cyc_Absyn_Gt =7U,Cyc_Absyn_Lt =8U,Cyc_Absyn_Gte =9U,Cyc_Absyn_Lte =10U,Cyc_Absyn_Not =11U,Cyc_Absyn_Bitnot =12U,Cyc_Absyn_Bitand =13U,Cyc_Absyn_Bitor =14U,Cyc_Absyn_Bitxor =15U,Cyc_Absyn_Bitlshift =16U,Cyc_Absyn_Bitlrshift =17U,Cyc_Absyn_Numelts =18U};
# 581
enum Cyc_Absyn_Incrementor{Cyc_Absyn_PreInc =0U,Cyc_Absyn_PostInc =1U,Cyc_Absyn_PreDec =2U,Cyc_Absyn_PostDec =3U};struct Cyc_Absyn_VarargCallInfo{int num_varargs;struct Cyc_List_List*injectors;struct Cyc_Absyn_VarargInfo*vai;};struct Cyc_Absyn_StructField_Absyn_OffsetofField_struct{int tag;struct _fat_ptr*f1;};struct Cyc_Absyn_TupleIndex_Absyn_OffsetofField_struct{int tag;unsigned f1;};
# 599
enum Cyc_Absyn_Coercion{Cyc_Absyn_Unknown_coercion =0U,Cyc_Absyn_No_coercion =1U,Cyc_Absyn_Null_to_NonNull =2U,Cyc_Absyn_Other_coercion =3U};struct Cyc_Absyn_MallocInfo{int is_calloc;struct Cyc_Absyn_Exp*rgn;void**elt_type;struct Cyc_Absyn_Exp*num_elts;int fat_result;int inline_call;};struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct{int tag;union Cyc_Absyn_Cnst f1;};struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Pragma_e_Absyn_Raw_exp_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct{int tag;enum Cyc_Absyn_Primop f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;enum Cyc_Absyn_Incrementor f2;};struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_VarargCallInfo*f3;int f4;struct _tuple1*f5;};struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;int f2;};struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Exp*f2;int f3;enum Cyc_Absyn_Coercion f4;};struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;int f3;int f4;};struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;int f3;int f4;};struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Tuple_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;};struct _tuple10{struct _fat_ptr*f1;struct Cyc_Absyn_Tqual f2;void*f3;};struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct{int tag;struct _tuple10*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;int f4;};struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;int f3;};struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct{int tag;struct _tuple2*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct Cyc_Absyn_Aggrdecl*f4;};struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Datatypedecl*f2;struct Cyc_Absyn_Datatypefield*f3;};struct Cyc_Absyn_Enum_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_Spawn_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;struct _tuple1*f3;};struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_MallocInfo f1;};struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Core_Opt*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Stmt*f1;};struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;};struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct{int tag;int f1;struct _fat_ptr f2;struct Cyc_List_List*f3;struct Cyc_List_List*f4;struct Cyc_List_List*f5;};struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Exp{void*topt;void*r;unsigned loc;void*annot;};struct Cyc_Absyn_Skip_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;struct Cyc_Absyn_Stmt*f3;};struct _tuple11{struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct{int tag;struct _tuple11 f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Break_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Continue_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Goto_s_Absyn_Raw_stmt_struct{int tag;struct _fat_ptr*f1;};struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _tuple11 f2;struct _tuple11 f3;struct Cyc_Absyn_Stmt*f4;};struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;void*f3;};struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Switch_clause**f2;};struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Decl*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct{int tag;struct _fat_ptr*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct _tuple11 f2;};struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_List_List*f2;void*f3;};struct Cyc_Absyn_Stmt{void*r;unsigned loc;void*annot;};struct Cyc_Absyn_Wild_p_Absyn_Raw_pat_struct{int tag;};struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_AliasVar_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_TagInt_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Tuple_p_Absyn_Raw_pat_struct{int tag;struct Cyc_List_List*f1;int f2;};struct Cyc_Absyn_Pointer_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Pat*f1;};struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct{int tag;union Cyc_Absyn_AggrInfo*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;int f4;};struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;struct Cyc_List_List*f3;int f4;};struct Cyc_Absyn_Null_p_Absyn_Raw_pat_struct{int tag;};struct Cyc_Absyn_Int_p_Absyn_Raw_pat_struct{int tag;enum Cyc_Absyn_Sign f1;int f2;};struct Cyc_Absyn_Char_p_Absyn_Raw_pat_struct{int tag;char f1;};struct Cyc_Absyn_Float_p_Absyn_Raw_pat_struct{int tag;struct _fat_ptr f1;int f2;};struct Cyc_Absyn_Enum_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_AnonEnum_p_Absyn_Raw_pat_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_UnknownId_p_Absyn_Raw_pat_struct{int tag;struct _tuple2*f1;};struct Cyc_Absyn_UnknownCall_p_Absyn_Raw_pat_struct{int tag;struct _tuple2*f1;struct Cyc_List_List*f2;int f3;};struct Cyc_Absyn_Exp_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Pat{void*r;void*topt;unsigned loc;};struct Cyc_Absyn_Switch_clause{struct Cyc_Absyn_Pat*pattern;struct Cyc_Core_Opt*pat_vars;struct Cyc_Absyn_Exp*where_clause;struct Cyc_Absyn_Stmt*body;unsigned loc;};struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct{int tag;struct _tuple2*f1;};struct Cyc_Absyn_Global_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Funname_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Param_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Local_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Pat_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Vardecl{enum Cyc_Absyn_Scope sc;struct _tuple2*name;unsigned varloc;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*initializer;void*rgn;struct Cyc_List_List*attributes;int escapes;};struct Cyc_Absyn_Fndecl{enum Cyc_Absyn_Scope sc;int is_inline;struct _tuple2*name;struct Cyc_Absyn_Stmt*body;struct Cyc_Absyn_FnInfo i;void*cached_type;struct Cyc_Core_Opt*param_vardecls;struct Cyc_Absyn_Vardecl*fn_vardecl;};struct Cyc_Absyn_Aggrfield{struct _fat_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*width;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*requires_clause;};struct Cyc_Absyn_AggrdeclImpl{struct Cyc_List_List*exist_vars;struct Cyc_List_List*rgn_po;struct Cyc_List_List*fields;int tagged;};struct Cyc_Absyn_Aggrdecl{enum Cyc_Absyn_AggrKind kind;enum Cyc_Absyn_Scope sc;struct _tuple2*name;struct Cyc_List_List*tvs;struct Cyc_Absyn_AggrdeclImpl*impl;struct Cyc_List_List*attributes;int expected_mem_kind;};struct Cyc_Absyn_Datatypefield{struct _tuple2*name;struct Cyc_List_List*typs;unsigned loc;enum Cyc_Absyn_Scope sc;};struct Cyc_Absyn_Datatypedecl{enum Cyc_Absyn_Scope sc;struct _tuple2*name;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*fields;int is_extensible;};struct Cyc_Absyn_Enumfield{struct _tuple2*name;struct Cyc_Absyn_Exp*tag;unsigned loc;};struct Cyc_Absyn_Enumdecl{enum Cyc_Absyn_Scope sc;struct _tuple2*name;struct Cyc_Core_Opt*fields;};struct Cyc_Absyn_Typedefdecl{struct _tuple2*name;struct Cyc_Absyn_Tqual tq;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*kind;void*defn;struct Cyc_List_List*atts;int extern_c;};struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Pat*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;void*f4;};struct Cyc_Absyn_Letv_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;};struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;};struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Typedefdecl*f1;};struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct{int tag;struct _fat_ptr*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct{int tag;struct _tuple2*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_ExternC_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;};struct _tuple12{unsigned f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct _tuple12*f4;};struct Cyc_Absyn_Porton_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Portoff_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Tempeston_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Tempestoff_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Decl{void*r;unsigned loc;};struct Cyc_Absyn_ArrayElement_Absyn_Designator_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_FieldName_Absyn_Designator_struct{int tag;struct _fat_ptr*f1;};extern char Cyc_Absyn_EmptyAnnot[11U];struct Cyc_Absyn_EmptyAnnot_Absyn_AbsynAnnot_struct{char*tag;};
# 997 "absyn.h"
extern void*Cyc_Absyn_void_type;
# 1067
struct Cyc_Absyn_Exp*Cyc_Absyn_signed_int_exp(int,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_uint_exp(unsigned,unsigned);
# 1079
struct Cyc_Absyn_Exp*Cyc_Absyn_prim1_exp(enum Cyc_Absyn_Primop,struct Cyc_Absyn_Exp*,unsigned);
# 1203
struct Cyc_Absyn_Aggrfield*Cyc_Absyn_lookup_field(struct Cyc_List_List*,struct _fat_ptr*);
# 1205
struct Cyc_Absyn_Aggrfield*Cyc_Absyn_lookup_decl_field(struct Cyc_Absyn_Aggrdecl*,struct _fat_ptr*);
# 1225
struct Cyc_Absyn_Aggrdecl*Cyc_Absyn_get_known_aggrdecl(union Cyc_Absyn_AggrInfo);
# 1227
int Cyc_Absyn_is_nontagged_nonrequire_union_type(void*);
# 1229
int Cyc_Absyn_is_require_union_type(void*);struct _union_RelnOp_RConst{int tag;unsigned val;};struct _union_RelnOp_RVar{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RNumelts{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RType{int tag;void*val;};struct _union_RelnOp_RParam{int tag;unsigned val;};struct _union_RelnOp_RParamNumelts{int tag;unsigned val;};struct _union_RelnOp_RReturn{int tag;unsigned val;};union Cyc_Relations_RelnOp{struct _union_RelnOp_RConst RConst;struct _union_RelnOp_RVar RVar;struct _union_RelnOp_RNumelts RNumelts;struct _union_RelnOp_RType RType;struct _union_RelnOp_RParam RParam;struct _union_RelnOp_RParamNumelts RParamNumelts;struct _union_RelnOp_RReturn RReturn;};
# 41 "relations-ap.h"
union Cyc_Relations_RelnOp Cyc_Relations_RConst(unsigned);union Cyc_Relations_RelnOp Cyc_Relations_RVar(struct Cyc_Absyn_Vardecl*);union Cyc_Relations_RelnOp Cyc_Relations_RNumelts(struct Cyc_Absyn_Vardecl*);
# 50
enum Cyc_Relations_Relation{Cyc_Relations_Req =0U,Cyc_Relations_Rneq =1U,Cyc_Relations_Rlte =2U,Cyc_Relations_Rlt =3U};struct Cyc_Relations_Reln{union Cyc_Relations_RelnOp rop1;enum Cyc_Relations_Relation relation;union Cyc_Relations_RelnOp rop2;};struct _tuple13{struct Cyc_Absyn_Exp*f1;enum Cyc_Relations_Relation f2;struct Cyc_Absyn_Exp*f3;};
# 64
struct _tuple13 Cyc_Relations_primop2relation(struct Cyc_Absyn_Exp*e1,enum Cyc_Absyn_Primop p,struct Cyc_Absyn_Exp*e2);
# 68
enum Cyc_Relations_Relation Cyc_Relations_flip_relation(enum Cyc_Relations_Relation r);
# 70
struct Cyc_Relations_Reln*Cyc_Relations_negate(struct _RegionHandle*,struct Cyc_Relations_Reln*);
# 76
int Cyc_Relations_exp2relnop(struct Cyc_Absyn_Exp*e,union Cyc_Relations_RelnOp*p);
# 84
struct Cyc_List_List*Cyc_Relations_exp2relns(struct _RegionHandle*r,struct Cyc_Absyn_Exp*e);
# 87
struct Cyc_List_List*Cyc_Relations_add_relation(struct _RegionHandle*rgn,union Cyc_Relations_RelnOp rop1,enum Cyc_Relations_Relation r,union Cyc_Relations_RelnOp rop2,struct Cyc_List_List*relns);
# 100
struct Cyc_List_List*Cyc_Relations_reln_assign_var(struct _RegionHandle*,struct Cyc_List_List*,struct Cyc_Absyn_Vardecl*,struct Cyc_Absyn_Exp*);
# 103
struct Cyc_List_List*Cyc_Relations_reln_assign_exp(struct _RegionHandle*,struct Cyc_List_List*,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*);
# 108
struct Cyc_List_List*Cyc_Relations_reln_kill_exp(struct _RegionHandle*,struct Cyc_List_List*,struct Cyc_Absyn_Exp*);
# 112
int Cyc_Relations_same_relns(struct Cyc_List_List*,struct Cyc_List_List*);
# 118
void Cyc_Relations_print_relns(struct Cyc___cycFILE*,struct Cyc_List_List*);
# 121
struct _fat_ptr Cyc_Relations_rop2string(union Cyc_Relations_RelnOp r);
struct _fat_ptr Cyc_Relations_relation2string(enum Cyc_Relations_Relation r);
struct _fat_ptr Cyc_Relations_relns2string(struct Cyc_List_List*r);
# 129
int Cyc_Relations_consistent_relations(struct Cyc_List_List*rlns);struct Cyc_RgnOrder_RgnPO;extern char Cyc_Tcenv_Env_error[10U];struct Cyc_Tcenv_Env_error_exn_struct{char*tag;};struct Cyc_Tcenv_Genv{struct Cyc_Dict_Dict aggrdecls;struct Cyc_Dict_Dict datatypedecls;struct Cyc_Dict_Dict enumdecls;struct Cyc_Dict_Dict typedefs;struct Cyc_Dict_Dict ordinaries;};struct Cyc_Tcenv_Fenv;struct Cyc_Tcenv_Tenv{struct Cyc_List_List*ns;struct Cyc_Tcenv_Genv*ae;struct Cyc_Tcenv_Fenv*le;int allow_valueof: 1;int in_extern_c_include: 1;int in_tempest: 1;int tempest_generalize: 1;};
# 90 "tcenv.h"
enum Cyc_Tcenv_NewStatus{Cyc_Tcenv_NoneNew =0U,Cyc_Tcenv_InNew =1U,Cyc_Tcenv_InNewAggr =2U};
# 30 "tcutil.h"
void*Cyc_Tcutil_impos(struct _fat_ptr,struct _fat_ptr);
# 39
int Cyc_Tcutil_is_void_type(void*);
# 41
int Cyc_Tcutil_is_any_int_type(void*);
int Cyc_Tcutil_is_any_float_type(void*);
# 53
int Cyc_Tcutil_is_zeroterm_pointer_type(void*);
# 55
int Cyc_Tcutil_is_bound_one(void*);
# 62
int Cyc_Tcutil_is_noreturn_fn_type(void*);
# 67
void*Cyc_Tcutil_pointer_elt_type(void*);
# 78
struct Cyc_Absyn_Exp*Cyc_Tcutil_get_type_bound(void*);
# 110
void*Cyc_Tcutil_compress(void*);
# 191
void*Cyc_Tcutil_rsubstitute(struct _RegionHandle*,struct Cyc_List_List*,void*);
# 197
struct Cyc_Absyn_Exp*Cyc_Tcutil_rsubsexp(struct _RegionHandle*,struct Cyc_List_List*,struct Cyc_Absyn_Exp*);
# 206
void*Cyc_Tcutil_fndecl2type(struct Cyc_Absyn_Fndecl*);
# 233
int Cyc_Tcutil_is_zero_ptr_deref(struct Cyc_Absyn_Exp*,void**ptr_type,int*is_fat,void**elt_type);
# 240
int Cyc_Tcutil_is_noalias_pointer(void*t,int must_be_unique);
# 250
int Cyc_Tcutil_is_noalias_pointer_or_aggr(void*);
# 296
struct Cyc_Absyn_Vardecl*Cyc_Tcutil_nonesc_vardecl(void*);
# 299
struct Cyc_List_List*Cyc_Tcutil_filter_nulls(struct Cyc_List_List*);struct _tuple14{unsigned f1;int f2;};
# 28 "evexp.h"
struct _tuple14 Cyc_Evexp_eval_const_uint_exp(struct Cyc_Absyn_Exp*e);struct Cyc_Set_Set;extern char Cyc_Set_Absent[7U];struct Cyc_Set_Absent_exn_struct{char*tag;};struct Cyc_Hashtable_Table;
# 39 "hashtable.h"
struct Cyc_Hashtable_Table*Cyc_Hashtable_create(int sz,int(*cmp)(void*,void*),int(*hash)(void*));
# 50
void Cyc_Hashtable_insert(struct Cyc_Hashtable_Table*t,void*key,void*val);
# 52
void*Cyc_Hashtable_lookup(struct Cyc_Hashtable_Table*t,void*key);
# 56
void**Cyc_Hashtable_lookup_opt(struct Cyc_Hashtable_Table*t,void*key);struct Cyc_JumpAnalysis_Jump_Anal_Result{struct Cyc_Hashtable_Table*pop_tables;struct Cyc_Hashtable_Table*succ_tables;struct Cyc_Hashtable_Table*pat_pop_tables;};
# 41 "cf_flowinfo.h"
int Cyc_CfFlowInfo_anal_error;
void Cyc_CfFlowInfo_aerr(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap);struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_CfFlowInfo_MallocPt_CfFlowInfo_Root_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;};struct Cyc_CfFlowInfo_InitParam_CfFlowInfo_Root_struct{int tag;int f1;void*f2;};struct Cyc_CfFlowInfo_Dot_CfFlowInfo_PathCon_struct{int tag;int f1;};struct Cyc_CfFlowInfo_Star_CfFlowInfo_PathCon_struct{int tag;};struct Cyc_CfFlowInfo_Place{void*root;struct Cyc_List_List*path;};
# 74
enum Cyc_CfFlowInfo_InitLevel{Cyc_CfFlowInfo_NoneIL =0U,Cyc_CfFlowInfo_AllIL =1U};extern char Cyc_CfFlowInfo_IsZero[7U];struct Cyc_CfFlowInfo_IsZero_Absyn_AbsynAnnot_struct{char*tag;};extern char Cyc_CfFlowInfo_NotZero[8U];struct Cyc_CfFlowInfo_NotZero_Absyn_AbsynAnnot_struct{char*tag;struct Cyc_List_List*f1;};extern char Cyc_CfFlowInfo_UnknownZ[9U];struct Cyc_CfFlowInfo_UnknownZ_Absyn_AbsynAnnot_struct{char*tag;struct Cyc_List_List*f1;};
# 87
extern struct Cyc_CfFlowInfo_IsZero_Absyn_AbsynAnnot_struct Cyc_CfFlowInfo_IsZero_val;struct _union_AbsLVal_PlaceL{int tag;struct Cyc_CfFlowInfo_Place*val;};struct _union_AbsLVal_UnknownL{int tag;int val;};union Cyc_CfFlowInfo_AbsLVal{struct _union_AbsLVal_PlaceL PlaceL;struct _union_AbsLVal_UnknownL UnknownL;};
# 94
union Cyc_CfFlowInfo_AbsLVal Cyc_CfFlowInfo_PlaceL(struct Cyc_CfFlowInfo_Place*);
union Cyc_CfFlowInfo_AbsLVal Cyc_CfFlowInfo_UnknownL();struct Cyc_CfFlowInfo_UnionRInfo{int is_union;int fieldnum;};struct Cyc_CfFlowInfo_Zero_CfFlowInfo_AbsRVal_struct{int tag;};struct Cyc_CfFlowInfo_NotZeroAll_CfFlowInfo_AbsRVal_struct{int tag;};struct Cyc_CfFlowInfo_UnknownR_CfFlowInfo_AbsRVal_struct{int tag;enum Cyc_CfFlowInfo_InitLevel f1;};struct Cyc_CfFlowInfo_Esc_CfFlowInfo_AbsRVal_struct{int tag;enum Cyc_CfFlowInfo_InitLevel f1;};struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct{int tag;struct Cyc_CfFlowInfo_Place*f1;};struct Cyc_CfFlowInfo_UniquePtr_CfFlowInfo_AbsRVal_struct{int tag;void*f1;};struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct{int tag;struct Cyc_CfFlowInfo_UnionRInfo f1;struct _fat_ptr f2;};struct Cyc_CfFlowInfo_Consumed_CfFlowInfo_AbsRVal_struct{int tag;struct Cyc_Absyn_Exp*f1;int f2;void*f3;};struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct{int tag;struct Cyc_Absyn_Vardecl*f1;void*f2;};struct _union_FlowInfo_BottomFL{int tag;int val;};struct _tuple15{struct Cyc_Dict_Dict f1;struct Cyc_List_List*f2;};struct _union_FlowInfo_ReachableFL{int tag;struct _tuple15 val;};union Cyc_CfFlowInfo_FlowInfo{struct _union_FlowInfo_BottomFL BottomFL;struct _union_FlowInfo_ReachableFL ReachableFL;};
# 138 "cf_flowinfo.h"
union Cyc_CfFlowInfo_FlowInfo Cyc_CfFlowInfo_BottomFL();
union Cyc_CfFlowInfo_FlowInfo Cyc_CfFlowInfo_ReachableFL(struct Cyc_Dict_Dict,struct Cyc_List_List*);struct Cyc_CfFlowInfo_FlowEnv{void*zero;void*notzeroall;void*unknown_none;void*unknown_all;void*esc_none;void*esc_all;struct Cyc_Dict_Dict mt_flowdict;struct Cyc_CfFlowInfo_Place*dummy_place;};
# 153
struct Cyc_CfFlowInfo_FlowEnv*Cyc_CfFlowInfo_new_flow_env();
# 155
int Cyc_CfFlowInfo_get_field_index(void*t,struct _fat_ptr*f);
int Cyc_CfFlowInfo_get_field_index_fs(struct Cyc_List_List*fs,struct _fat_ptr*f);
# 158
int Cyc_CfFlowInfo_root_cmp(void*,void*);
# 161
struct _fat_ptr Cyc_CfFlowInfo_aggrfields_to_aggrdict(struct Cyc_CfFlowInfo_FlowEnv*,struct Cyc_List_List*,int no_init_bits_only,void*);
void*Cyc_CfFlowInfo_typ_to_absrval(struct Cyc_CfFlowInfo_FlowEnv*,void*t,int no_init_bits_only,void*leafval);
void*Cyc_CfFlowInfo_make_unique_consumed(struct Cyc_CfFlowInfo_FlowEnv*fenv,void*t,struct Cyc_Absyn_Exp*consumer,int iteration,void*);
int Cyc_CfFlowInfo_is_unique_consumed(struct Cyc_Absyn_Exp*e,int env_iteration,void*r,int*needs_unconsume);
void*Cyc_CfFlowInfo_make_unique_unconsumed(struct Cyc_CfFlowInfo_FlowEnv*fenv,void*r);struct _tuple16{void*f1;struct Cyc_List_List*f2;};
struct _tuple16 Cyc_CfFlowInfo_unname_rval(void*rv);
# 168
enum Cyc_CfFlowInfo_InitLevel Cyc_CfFlowInfo_initlevel(struct Cyc_CfFlowInfo_FlowEnv*,struct Cyc_Dict_Dict d,void*r);
void*Cyc_CfFlowInfo_lookup_place(struct Cyc_Dict_Dict d,struct Cyc_CfFlowInfo_Place*place);
# 171
int Cyc_CfFlowInfo_is_init_pointer(void*r);
int Cyc_CfFlowInfo_flow_lessthan_approx(union Cyc_CfFlowInfo_FlowInfo f1,union Cyc_CfFlowInfo_FlowInfo f2);
# 174
void Cyc_CfFlowInfo_print_absrval(void*rval);
# 181
void Cyc_CfFlowInfo_print_flow(union Cyc_CfFlowInfo_FlowInfo f);
# 196 "cf_flowinfo.h"
struct Cyc_Dict_Dict Cyc_CfFlowInfo_escape_deref(struct Cyc_CfFlowInfo_FlowEnv*fenv,struct Cyc_Dict_Dict d,void*r);
struct Cyc_Dict_Dict Cyc_CfFlowInfo_assign_place(struct Cyc_CfFlowInfo_FlowEnv*fenv,unsigned loc,struct Cyc_Dict_Dict d,struct Cyc_CfFlowInfo_Place*place,void*r);
# 201
union Cyc_CfFlowInfo_FlowInfo Cyc_CfFlowInfo_join_flow(struct Cyc_CfFlowInfo_FlowEnv*,union Cyc_CfFlowInfo_FlowInfo,union Cyc_CfFlowInfo_FlowInfo);struct _tuple17{union Cyc_CfFlowInfo_FlowInfo f1;void*f2;};
struct _tuple17 Cyc_CfFlowInfo_join_flow_and_rval(struct Cyc_CfFlowInfo_FlowEnv*,struct _tuple17 pr1,struct _tuple17 pr2);
# 32 "new_control_flow.h"
void Cyc_NewControlFlow_cf_check(struct Cyc_JumpAnalysis_Jump_Anal_Result*tables,struct Cyc_List_List*ds);
# 37
extern int Cyc_NewControlFlow_warn_lose_unique;struct Cyc_Tcpat_TcPatResult{struct _tuple1*tvars_and_bounds_opt;struct Cyc_List_List*patvars;};struct Cyc_Tcpat_WhereTest_Tcpat_PatTest_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Tcpat_EqNull_Tcpat_PatTest_struct{int tag;};struct Cyc_Tcpat_NeqNull_Tcpat_PatTest_struct{int tag;};struct Cyc_Tcpat_EqEnum_Tcpat_PatTest_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Tcpat_EqAnonEnum_Tcpat_PatTest_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Tcpat_EqFloat_Tcpat_PatTest_struct{int tag;struct _fat_ptr f1;int f2;};struct Cyc_Tcpat_EqConst_Tcpat_PatTest_struct{int tag;unsigned f1;};struct Cyc_Tcpat_EqDatatypeTag_Tcpat_PatTest_struct{int tag;int f1;struct Cyc_Absyn_Datatypedecl*f2;struct Cyc_Absyn_Datatypefield*f3;};struct Cyc_Tcpat_EqTaggedUnion_Tcpat_PatTest_struct{int tag;struct _fat_ptr*f1;int f2;};struct Cyc_Tcpat_EqExtensibleDatatype_Tcpat_PatTest_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;};struct Cyc_Tcpat_Dummy_Tcpat_Access_struct{int tag;};struct Cyc_Tcpat_Deref_Tcpat_Access_struct{int tag;};struct Cyc_Tcpat_TupleField_Tcpat_Access_struct{int tag;unsigned f1;};struct Cyc_Tcpat_DatatypeField_Tcpat_Access_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;unsigned f3;};struct Cyc_Tcpat_AggrField_Tcpat_Access_struct{int tag;int f1;struct _fat_ptr*f2;};struct _union_PatOrWhere_pattern{int tag;struct Cyc_Absyn_Pat*val;};struct _union_PatOrWhere_where_clause{int tag;struct Cyc_Absyn_Exp*val;};union Cyc_Tcpat_PatOrWhere{struct _union_PatOrWhere_pattern pattern;struct _union_PatOrWhere_where_clause where_clause;};struct Cyc_Tcpat_PathNode{union Cyc_Tcpat_PatOrWhere orig_pat;void*access;};struct Cyc_Tcpat_Rhs{int used;unsigned pat_loc;struct Cyc_Absyn_Stmt*rhs;};struct Cyc_Tcpat_Failure_Tcpat_Decision_struct{int tag;void*f1;};struct Cyc_Tcpat_Success_Tcpat_Decision_struct{int tag;struct Cyc_Tcpat_Rhs*f1;};struct Cyc_Tcpat_SwitchDec_Tcpat_Decision_struct{int tag;struct Cyc_List_List*f1;struct Cyc_List_List*f2;void*f3;};
# 117 "tcpat.h"
int Cyc_Tcpat_has_vars(struct Cyc_Core_Opt*pat_vars);
# 29 "warn.h"
void Cyc_Warn_warn(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap);
# 40
void*Cyc_Warn_impos(struct _fat_ptr fmt,struct _fat_ptr ap);struct Cyc_Warn_String_Warn_Warg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Warn_Exp_Warn_Warg_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Warn_Typ_Warn_Warg_struct{int tag;void*f1;};struct Cyc_Warn_Qvar_Warn_Warg_struct{int tag;struct _tuple2*f1;};struct Cyc_PP_Ppstate;struct Cyc_PP_Out;struct Cyc_PP_Doc;struct Cyc_Absynpp_Params{int expand_typedefs;int qvar_to_Cids;int add_cyc_prefix;int to_VC;int decls_first;int rewrite_temp_tvars;int print_all_tvars;int print_all_kinds;int print_all_effects;int print_using_stmts;int print_externC_stmts;int print_full_evars;int print_zeroterm;int generate_line_directives;int use_curr_namespace;struct Cyc_List_List*curr_namespace;};
# 67 "absynpp.h"
struct _fat_ptr Cyc_Absynpp_exp2string(struct Cyc_Absyn_Exp*);
# 69
struct _fat_ptr Cyc_Absynpp_qvar2string(struct _tuple2*);
# 51 "new_control_flow.cyc"
static int Cyc_NewControlFlow_strcmp(struct _fat_ptr s1,struct _fat_ptr s2){
if((char*)s1.curr == (char*)s2.curr)
return 0;{
# 52
int i=0;
# 55
unsigned sz1=_get_fat_size(s1,sizeof(char));
unsigned sz2=_get_fat_size(s2,sizeof(char));
unsigned minsz=sz1 < sz2?sz1: sz2;
# 59
while((unsigned)i < minsz){
char c1=*((const char*)_check_fat_subscript(s1,sizeof(char),i));
char c2=*((const char*)_check_fat_subscript(s2,sizeof(char),i));
if((int)c1 == (int)'\000'){
if((int)c2 == (int)'\000')return 0;else{
return - 1;}}else{
if((int)c2 == (int)'\000')return 1;else{
# 67
int diff=(int)c1 - (int)c2;
if(diff != 0)return diff;}}
# 70
++ i;}
# 72
if(sz1 == sz2)return 0;if(minsz < sz2){
# 74
if((int)*((const char*)_check_fat_subscript(s2,sizeof(char),i))== (int)'\000')return 0;else{
return - 1;}}else{
# 77
if((int)*((const char*)_check_fat_subscript(s1,sizeof(char),i))== (int)'\000')return 0;else{
return 1;}}}}
# 51
int Cyc_NewControlFlow_warn_lose_unique=0;struct Cyc_NewControlFlow_CFStmtAnnot{int visited;};static char Cyc_NewControlFlow_CFAnnot[8U]="CFAnnot";struct Cyc_NewControlFlow_CFAnnot_Absyn_AbsynAnnot_struct{char*tag;struct Cyc_NewControlFlow_CFStmtAnnot f1;};struct Cyc_NewControlFlow_AnalEnv{struct Cyc_JumpAnalysis_Jump_Anal_Result*all_tables;struct Cyc_Hashtable_Table*succ_table;struct Cyc_Hashtable_Table*pat_pop_table;struct Cyc_CfFlowInfo_FlowEnv*fenv;int iterate_again;int iteration_num;int in_try;union Cyc_CfFlowInfo_FlowInfo tryflow;int noreturn;void*return_type;struct Cyc_List_List*unique_pat_vars;struct Cyc_List_List*param_roots;struct Cyc_List_List*noconsume_params;struct Cyc_Hashtable_Table*flow_table;struct Cyc_List_List*return_relns;};struct _tuple18{void*f1;int f2;};
# 125 "new_control_flow.cyc"
static union Cyc_CfFlowInfo_FlowInfo Cyc_NewControlFlow_anal_stmt(struct Cyc_NewControlFlow_AnalEnv*,union Cyc_CfFlowInfo_FlowInfo,struct Cyc_Absyn_Stmt*,struct _tuple18*);
static union Cyc_CfFlowInfo_FlowInfo Cyc_NewControlFlow_anal_decl(struct Cyc_NewControlFlow_AnalEnv*,union Cyc_CfFlowInfo_FlowInfo,struct Cyc_Absyn_Decl*);struct _tuple19{union Cyc_CfFlowInfo_FlowInfo f1;union Cyc_CfFlowInfo_AbsLVal f2;};
static struct _tuple19 Cyc_NewControlFlow_anal_Lexp(struct Cyc_NewControlFlow_AnalEnv*,union Cyc_CfFlowInfo_FlowInfo,int expand_unique,int passthrough_consumes,struct Cyc_Absyn_Exp*);
static struct _tuple17 Cyc_NewControlFlow_anal_Rexp(struct Cyc_NewControlFlow_AnalEnv*,int copy_ctxt,union Cyc_CfFlowInfo_FlowInfo,struct Cyc_Absyn_Exp*);struct _tuple20{union Cyc_CfFlowInfo_FlowInfo f1;union Cyc_CfFlowInfo_FlowInfo f2;};
static struct _tuple20 Cyc_NewControlFlow_anal_test(struct Cyc_NewControlFlow_AnalEnv*,union Cyc_CfFlowInfo_FlowInfo,struct Cyc_Absyn_Exp*);
# 131
static union Cyc_CfFlowInfo_FlowInfo Cyc_NewControlFlow_expand_unique_places(struct Cyc_NewControlFlow_AnalEnv*,union Cyc_CfFlowInfo_FlowInfo inflow,struct Cyc_List_List*es);
# 135
static struct Cyc_NewControlFlow_CFStmtAnnot*Cyc_NewControlFlow_get_stmt_annot(struct Cyc_Absyn_Stmt*s){
void*_stmttmp0=s->annot;void*_tmp1=_stmttmp0;struct Cyc_NewControlFlow_CFStmtAnnot*_tmp2;if(((struct Cyc_NewControlFlow_CFAnnot_Absyn_AbsynAnnot_struct*)_tmp1)->tag == Cyc_NewControlFlow_CFAnnot){_LL1: _tmp2=(struct Cyc_NewControlFlow_CFStmtAnnot*)&((struct Cyc_NewControlFlow_CFAnnot_Absyn_AbsynAnnot_struct*)_tmp1)->f1;_LL2: {struct Cyc_NewControlFlow_CFStmtAnnot*x=_tmp2;
return x;}}else{_LL3: _LL4:
({void*_tmp5B5=(void*)({struct Cyc_NewControlFlow_CFAnnot_Absyn_AbsynAnnot_struct*_tmp3=_cycalloc(sizeof(*_tmp3));_tmp3->tag=Cyc_NewControlFlow_CFAnnot,(_tmp3->f1).visited=0;_tmp3;});s->annot=_tmp5B5;});return({Cyc_NewControlFlow_get_stmt_annot(s);});}_LL0:;}
# 142
static union Cyc_CfFlowInfo_FlowInfo*Cyc_NewControlFlow_get_stmt_flow(struct Cyc_NewControlFlow_AnalEnv*env,struct Cyc_Absyn_Stmt*s){
union Cyc_CfFlowInfo_FlowInfo**sflow=({({union Cyc_CfFlowInfo_FlowInfo**(*_tmp5B7)(struct Cyc_Hashtable_Table*t,struct Cyc_Absyn_Stmt*key)=({union Cyc_CfFlowInfo_FlowInfo**(*_tmp7)(struct Cyc_Hashtable_Table*t,struct Cyc_Absyn_Stmt*key)=(union Cyc_CfFlowInfo_FlowInfo**(*)(struct Cyc_Hashtable_Table*t,struct Cyc_Absyn_Stmt*key))Cyc_Hashtable_lookup_opt;_tmp7;});struct Cyc_Hashtable_Table*_tmp5B6=env->flow_table;_tmp5B7(_tmp5B6,s);});});
if(sflow == 0){
union Cyc_CfFlowInfo_FlowInfo*res=({union Cyc_CfFlowInfo_FlowInfo*_tmp6=_cycalloc(sizeof(*_tmp6));({union Cyc_CfFlowInfo_FlowInfo _tmp5B8=({Cyc_CfFlowInfo_BottomFL();});*_tmp6=_tmp5B8;});_tmp6;});
({({void(*_tmp5BB)(struct Cyc_Hashtable_Table*t,struct Cyc_Absyn_Stmt*key,union Cyc_CfFlowInfo_FlowInfo*val)=({void(*_tmp5)(struct Cyc_Hashtable_Table*t,struct Cyc_Absyn_Stmt*key,union Cyc_CfFlowInfo_FlowInfo*val)=(void(*)(struct Cyc_Hashtable_Table*t,struct Cyc_Absyn_Stmt*key,union Cyc_CfFlowInfo_FlowInfo*val))Cyc_Hashtable_insert;_tmp5;});struct Cyc_Hashtable_Table*_tmp5BA=env->flow_table;struct Cyc_Absyn_Stmt*_tmp5B9=s;_tmp5BB(_tmp5BA,_tmp5B9,res);});});
return res;}
# 144
return*sflow;}
# 152
static struct Cyc_List_List*Cyc_NewControlFlow_flowrelns(union Cyc_CfFlowInfo_FlowInfo f){
union Cyc_CfFlowInfo_FlowInfo _tmp9=f;struct Cyc_List_List*_tmpA;if((_tmp9.BottomFL).tag == 1){_LL1: _LL2:
 return 0;}else{_LL3: _tmpA=((_tmp9.ReachableFL).val).f2;_LL4: {struct Cyc_List_List*r=_tmpA;
return r;}}_LL0:;}struct _tuple21{struct Cyc_NewControlFlow_CFStmtAnnot*f1;union Cyc_CfFlowInfo_FlowInfo*f2;};
# 159
static struct _tuple21 Cyc_NewControlFlow_pre_stmt_check(struct Cyc_NewControlFlow_AnalEnv*env,union Cyc_CfFlowInfo_FlowInfo inflow,struct Cyc_Absyn_Stmt*s){
struct Cyc_NewControlFlow_CFStmtAnnot*annot=({Cyc_NewControlFlow_get_stmt_annot(s);});
union Cyc_CfFlowInfo_FlowInfo*sflow=({Cyc_NewControlFlow_get_stmt_flow(env,s);});
# 163
({union Cyc_CfFlowInfo_FlowInfo _tmp5BC=({Cyc_CfFlowInfo_join_flow(env->fenv,inflow,*sflow);});*sflow=_tmp5BC;});
# 169
annot->visited=env->iteration_num;
return({struct _tuple21 _tmp513;_tmp513.f1=annot,_tmp513.f2=sflow;_tmp513;});}
# 179
static void Cyc_NewControlFlow_update_tryflow(struct Cyc_NewControlFlow_AnalEnv*env,union Cyc_CfFlowInfo_FlowInfo new_flow){
if(env->in_try)
# 187
({union Cyc_CfFlowInfo_FlowInfo _tmp5BD=({Cyc_CfFlowInfo_join_flow(env->fenv,new_flow,env->tryflow);});env->tryflow=_tmp5BD;});}struct _tuple22{struct Cyc_NewControlFlow_AnalEnv*f1;unsigned f2;struct Cyc_Dict_Dict f3;};
# 194
static void Cyc_NewControlFlow_check_unique_root(struct _tuple22*ckenv,void*root,void*rval){
# 196
struct Cyc_Dict_Dict _tmp10;unsigned _tmpF;struct Cyc_NewControlFlow_AnalEnv*_tmpE;_LL1: _tmpE=ckenv->f1;_tmpF=ckenv->f2;_tmp10=ckenv->f3;_LL2: {struct Cyc_NewControlFlow_AnalEnv*env=_tmpE;unsigned loc=_tmpF;struct Cyc_Dict_Dict new_fd=_tmp10;
void*_tmp11=root;struct Cyc_Absyn_Vardecl*_tmp12;if(((struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct*)_tmp11)->tag == 0U){_LL4: _tmp12=((struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct*)_tmp11)->f1;_LL5: {struct Cyc_Absyn_Vardecl*vd=_tmp12;
# 199
if(!({Cyc_Dict_lookup_bool(new_fd,root,& rval);})&&({Cyc_Tcutil_is_noalias_pointer_or_aggr(vd->type);})){
# 201
retry: {void*_tmp13=rval;void*_tmp14;switch(*((int*)_tmp13)){case 8U: _LL9: _tmp14=(void*)((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp13)->f2;_LLA: {void*r=_tmp14;
rval=r;goto retry;}case 7U: _LLB: _LLC:
 goto _LLE;case 2U: if(((struct Cyc_CfFlowInfo_UnknownR_CfFlowInfo_AbsRVal_struct*)_tmp13)->f1 == Cyc_CfFlowInfo_NoneIL){_LLD: _LLE:
 goto _LL10;}else{goto _LL11;}case 0U: _LLF: _LL10:
 goto _LL8;default: _LL11: _LL12:
# 208
({struct Cyc_String_pa_PrintArg_struct _tmp17=({struct Cyc_String_pa_PrintArg_struct _tmp514;_tmp514.tag=0U,({struct _fat_ptr _tmp5BE=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(vd->name);}));_tmp514.f1=_tmp5BE;});_tmp514;});void*_tmp15[1U];_tmp15[0]=& _tmp17;({unsigned _tmp5C0=loc;struct _fat_ptr _tmp5BF=({const char*_tmp16="alias-free pointer(s) reachable from %s may become unreachable.";_tag_fat(_tmp16,sizeof(char),64U);});Cyc_Warn_warn(_tmp5C0,_tmp5BF,_tag_fat(_tmp15,sizeof(void*),1U));});});
goto _LL8;}_LL8:;}}
# 199
goto _LL3;}}else{_LL6: _LL7:
# 213
 goto _LL3;}_LL3:;}}
# 220
static void Cyc_NewControlFlow_update_flow(struct Cyc_NewControlFlow_AnalEnv*env,struct Cyc_Absyn_Stmt*s,union Cyc_CfFlowInfo_FlowInfo flow){
struct Cyc_NewControlFlow_CFStmtAnnot*annot=({Cyc_NewControlFlow_get_stmt_annot(s);});
union Cyc_CfFlowInfo_FlowInfo*sflow=({Cyc_NewControlFlow_get_stmt_flow(env,s);});
union Cyc_CfFlowInfo_FlowInfo new_flow=({Cyc_CfFlowInfo_join_flow(env->fenv,flow,*sflow);});
# 226
if(Cyc_NewControlFlow_warn_lose_unique){
struct _tuple20 _stmttmp1=({struct _tuple20 _tmp516;_tmp516.f1=flow,_tmp516.f2=new_flow;_tmp516;});struct _tuple20 _tmp19=_stmttmp1;struct Cyc_Dict_Dict _tmp1B;struct Cyc_Dict_Dict _tmp1A;if(((_tmp19.f1).ReachableFL).tag == 2){if(((_tmp19.f2).ReachableFL).tag == 2){_LL1: _tmp1A=(((_tmp19.f1).ReachableFL).val).f1;_tmp1B=(((_tmp19.f2).ReachableFL).val).f1;_LL2: {struct Cyc_Dict_Dict fd=_tmp1A;struct Cyc_Dict_Dict new_fd=_tmp1B;
# 229
struct _tuple22 ckenv=({struct _tuple22 _tmp515;_tmp515.f1=env,_tmp515.f2=s->loc,_tmp515.f3=new_fd;_tmp515;});
({({void(*_tmp5C1)(void(*f)(struct _tuple22*,void*,void*),struct _tuple22*env,struct Cyc_Dict_Dict d)=({void(*_tmp1C)(void(*f)(struct _tuple22*,void*,void*),struct _tuple22*env,struct Cyc_Dict_Dict d)=(void(*)(void(*f)(struct _tuple22*,void*,void*),struct _tuple22*env,struct Cyc_Dict_Dict d))Cyc_Dict_iter_c;_tmp1C;});_tmp5C1(Cyc_NewControlFlow_check_unique_root,& ckenv,fd);});});
goto _LL0;}}else{goto _LL3;}}else{_LL3: _LL4:
 goto _LL0;}_LL0:;}
# 226
if(!({Cyc_CfFlowInfo_flow_lessthan_approx(new_flow,*sflow);})){
# 242
*sflow=new_flow;
# 246
if(annot->visited == env->iteration_num)
# 248
env->iterate_again=1;}}
# 253
static union Cyc_CfFlowInfo_FlowInfo Cyc_NewControlFlow_add_vars(struct Cyc_CfFlowInfo_FlowEnv*fenv,union Cyc_CfFlowInfo_FlowInfo inflow,struct Cyc_List_List*vds,void*leafval,unsigned loc,int nameit){
# 257
union Cyc_CfFlowInfo_FlowInfo _tmp1E=inflow;struct Cyc_List_List*_tmp20;struct Cyc_Dict_Dict _tmp1F;if((_tmp1E.BottomFL).tag == 1){_LL1: _LL2:
 return({Cyc_CfFlowInfo_BottomFL();});}else{_LL3: _tmp1F=((_tmp1E.ReachableFL).val).f1;_tmp20=((_tmp1E.ReachableFL).val).f2;_LL4: {struct Cyc_Dict_Dict d=_tmp1F;struct Cyc_List_List*relns=_tmp20;
# 260
for(0;vds != 0;vds=vds->tl){
struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct*root=({struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct*_tmp22=_cycalloc(sizeof(*_tmp22));_tmp22->tag=0U,_tmp22->f1=(struct Cyc_Absyn_Vardecl*)vds->hd;_tmp22;});
void*rval=({Cyc_CfFlowInfo_typ_to_absrval(fenv,((struct Cyc_Absyn_Vardecl*)vds->hd)->type,0,leafval);});
if(nameit)
rval=(void*)({struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*_tmp21=_cycalloc(sizeof(*_tmp21));_tmp21->tag=8U,_tmp21->f1=(struct Cyc_Absyn_Vardecl*)vds->hd,_tmp21->f2=rval;_tmp21;});
# 267
d=({Cyc_Dict_insert(d,(void*)root,rval);});}
# 269
return({Cyc_CfFlowInfo_ReachableFL(d,relns);});}}_LL0:;}
# 273
static int Cyc_NewControlFlow_relns_ok(struct Cyc_List_List*assume,struct Cyc_List_List*req){
# 280
for(0;(unsigned)req;req=req->tl){
struct Cyc_Relations_Reln*neg_rln=({Cyc_Relations_negate(Cyc_Core_heap_region,(struct Cyc_Relations_Reln*)req->hd);});
if(({Cyc_Relations_consistent_relations(({struct Cyc_List_List*_tmp24=_cycalloc(sizeof(*_tmp24));_tmp24->hd=neg_rln,_tmp24->tl=assume;_tmp24;}));}))
return 0;}
# 285
return 1;}
# 288
static struct Cyc_Absyn_Exp*Cyc_NewControlFlow_strip_cast(struct Cyc_Absyn_Exp*e){
void*_stmttmp2=e->r;void*_tmp26=_stmttmp2;struct Cyc_Absyn_Exp*_tmp27;if(((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp26)->tag == 14U){_LL1: _tmp27=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp26)->f2;_LL2: {struct Cyc_Absyn_Exp*e1=_tmp27;
return e1;}}else{_LL3: _LL4:
 return e;}_LL0:;}
# 295
static void Cyc_NewControlFlow_check_union_requires(unsigned loc,void*t,struct _fat_ptr*f,union Cyc_CfFlowInfo_FlowInfo inflow){
# 297
union Cyc_CfFlowInfo_FlowInfo _tmp29=inflow;struct Cyc_List_List*_tmp2A;if((_tmp29.BottomFL).tag == 1){_LL1: _LL2:
 return;}else{_LL3: _tmp2A=((_tmp29.ReachableFL).val).f2;_LL4: {struct Cyc_List_List*relns=_tmp2A;
# 300
{void*_stmttmp3=({Cyc_Tcutil_compress(t);});void*_tmp2B=_stmttmp3;struct Cyc_List_List*_tmp2C;struct Cyc_List_List*_tmp2E;union Cyc_Absyn_AggrInfo _tmp2D;switch(*((int*)_tmp2B)){case 0U: if(((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2B)->f1)->tag == 22U){_LL6: _tmp2D=((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2B)->f1)->f1;_tmp2E=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2B)->f2;_LL7: {union Cyc_Absyn_AggrInfo ainfo=_tmp2D;struct Cyc_List_List*ts=_tmp2E;
# 302
struct Cyc_Absyn_Aggrdecl*ad=({Cyc_Absyn_get_known_aggrdecl(ainfo);});
struct Cyc_Absyn_Aggrfield*field=({Cyc_Absyn_lookup_decl_field(ad,f);});
struct Cyc_Absyn_Exp*req=((struct Cyc_Absyn_Aggrfield*)_check_null(field))->requires_clause;
if(req != 0){
struct _RegionHandle _tmp2F=_new_region("temp");struct _RegionHandle*temp=& _tmp2F;_push_region(temp);
{struct Cyc_Absyn_Exp*ireq=({struct Cyc_Absyn_Exp*(*_tmp3D)(struct _RegionHandle*,struct Cyc_List_List*,struct Cyc_Absyn_Exp*)=Cyc_Tcutil_rsubsexp;struct _RegionHandle*_tmp3E=temp;struct Cyc_List_List*_tmp3F=({Cyc_List_rzip(temp,temp,ad->tvs,ts);});struct Cyc_Absyn_Exp*_tmp40=req;_tmp3D(_tmp3E,_tmp3F,_tmp40);});
# 309
if(!({int(*_tmp30)(struct Cyc_List_List*assume,struct Cyc_List_List*req)=Cyc_NewControlFlow_relns_ok;struct Cyc_List_List*_tmp31=relns;struct Cyc_List_List*_tmp32=({Cyc_Relations_exp2relns(temp,ireq);});_tmp30(_tmp31,_tmp32);})){
({struct Cyc_String_pa_PrintArg_struct _tmp35=({struct Cyc_String_pa_PrintArg_struct _tmp518;_tmp518.tag=0U,({struct _fat_ptr _tmp5C2=(struct _fat_ptr)((struct _fat_ptr)({struct _fat_ptr(*_tmp37)(struct Cyc_Absyn_Exp*)=Cyc_Absynpp_exp2string;struct Cyc_Absyn_Exp*_tmp38=({Cyc_NewControlFlow_strip_cast(ireq);});_tmp37(_tmp38);}));_tmp518.f1=_tmp5C2;});_tmp518;});struct Cyc_String_pa_PrintArg_struct _tmp36=({struct Cyc_String_pa_PrintArg_struct _tmp517;_tmp517.tag=0U,_tmp517.f1=(struct _fat_ptr)((struct _fat_ptr)*f);_tmp517;});void*_tmp33[2U];_tmp33[0]=& _tmp35,_tmp33[1]=& _tmp36;({unsigned _tmp5C4=loc;struct _fat_ptr _tmp5C3=({const char*_tmp34="unable to prove %s, required to access %s";_tag_fat(_tmp34,sizeof(char),42U);});Cyc_CfFlowInfo_aerr(_tmp5C4,_tmp5C3,_tag_fat(_tmp33,sizeof(void*),2U));});});
({void*_tmp39=0U;({struct Cyc___cycFILE*_tmp5C6=Cyc_stderr;struct _fat_ptr _tmp5C5=({const char*_tmp3A="  [recorded facts on path: ";_tag_fat(_tmp3A,sizeof(char),28U);});Cyc_fprintf(_tmp5C6,_tmp5C5,_tag_fat(_tmp39,sizeof(void*),0U));});});
({Cyc_Relations_print_relns(Cyc_stderr,relns);});
({void*_tmp3B=0U;({struct Cyc___cycFILE*_tmp5C8=Cyc_stderr;struct _fat_ptr _tmp5C7=({const char*_tmp3C="]\n";_tag_fat(_tmp3C,sizeof(char),3U);});Cyc_fprintf(_tmp5C8,_tmp5C7,_tag_fat(_tmp3B,sizeof(void*),0U));});});}}
# 307
;_pop_region();}
# 305
goto _LL5;}}else{goto _LLA;}case 7U: _LL8: _tmp2C=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp2B)->f2;_LL9: {struct Cyc_List_List*fs=_tmp2C;
# 318
struct Cyc_Absyn_Aggrfield*field=({Cyc_Absyn_lookup_field(fs,f);});
struct Cyc_Absyn_Exp*req=((struct Cyc_Absyn_Aggrfield*)_check_null(field))->requires_clause;
if(req != 0){
struct _RegionHandle _tmp41=_new_region("temp");struct _RegionHandle*temp=& _tmp41;_push_region(temp);
if(!({int(*_tmp42)(struct Cyc_List_List*assume,struct Cyc_List_List*req)=Cyc_NewControlFlow_relns_ok;struct Cyc_List_List*_tmp43=relns;struct Cyc_List_List*_tmp44=({Cyc_Relations_exp2relns(temp,req);});_tmp42(_tmp43,_tmp44);})){
({struct Cyc_String_pa_PrintArg_struct _tmp47=({struct Cyc_String_pa_PrintArg_struct _tmp51A;_tmp51A.tag=0U,({struct _fat_ptr _tmp5C9=(struct _fat_ptr)((struct _fat_ptr)({struct _fat_ptr(*_tmp49)(struct Cyc_Absyn_Exp*)=Cyc_Absynpp_exp2string;struct Cyc_Absyn_Exp*_tmp4A=({Cyc_NewControlFlow_strip_cast(req);});_tmp49(_tmp4A);}));_tmp51A.f1=_tmp5C9;});_tmp51A;});struct Cyc_String_pa_PrintArg_struct _tmp48=({struct Cyc_String_pa_PrintArg_struct _tmp519;_tmp519.tag=0U,_tmp519.f1=(struct _fat_ptr)((struct _fat_ptr)*f);_tmp519;});void*_tmp45[2U];_tmp45[0]=& _tmp47,_tmp45[1]=& _tmp48;({unsigned _tmp5CB=loc;struct _fat_ptr _tmp5CA=({const char*_tmp46="unable to prove %s, required to access %s";_tag_fat(_tmp46,sizeof(char),42U);});Cyc_CfFlowInfo_aerr(_tmp5CB,_tmp5CA,_tag_fat(_tmp45,sizeof(void*),2U));});});
({void*_tmp4B=0U;({struct Cyc___cycFILE*_tmp5CD=Cyc_stderr;struct _fat_ptr _tmp5CC=({const char*_tmp4C="  [recorded facts on path: ";_tag_fat(_tmp4C,sizeof(char),28U);});Cyc_fprintf(_tmp5CD,_tmp5CC,_tag_fat(_tmp4B,sizeof(void*),0U));});});
({Cyc_Relations_print_relns(Cyc_stderr,relns);});
({void*_tmp4D=0U;({struct Cyc___cycFILE*_tmp5CF=Cyc_stderr;struct _fat_ptr _tmp5CE=({const char*_tmp4E="]\n";_tag_fat(_tmp4E,sizeof(char),3U);});Cyc_fprintf(_tmp5CF,_tmp5CE,_tag_fat(_tmp4D,sizeof(void*),0U));});});}
# 322
;_pop_region();}
# 320
goto _LL5;}default: _LLA: _LLB:
# 330
 goto _LL5;}_LL5:;}
# 332
goto _LL0;}}_LL0:;}
# 336
static union Cyc_CfFlowInfo_FlowInfo Cyc_NewControlFlow_use_Rval(struct Cyc_NewControlFlow_AnalEnv*env,unsigned loc,union Cyc_CfFlowInfo_FlowInfo inflow,void*r){
union Cyc_CfFlowInfo_FlowInfo _tmp50=inflow;struct Cyc_List_List*_tmp52;struct Cyc_Dict_Dict _tmp51;if((_tmp50.BottomFL).tag == 1){_LL1: _LL2:
 return({Cyc_CfFlowInfo_BottomFL();});}else{_LL3: _tmp51=((_tmp50.ReachableFL).val).f1;_tmp52=((_tmp50.ReachableFL).val).f2;_LL4: {struct Cyc_Dict_Dict d=_tmp51;struct Cyc_List_List*relns=_tmp52;
# 340
if((int)({Cyc_CfFlowInfo_initlevel(env->fenv,d,r);})!= (int)Cyc_CfFlowInfo_AllIL)
({void*_tmp53=0U;({unsigned _tmp5D1=loc;struct _fat_ptr _tmp5D0=({const char*_tmp54="expression may not be fully initialized";_tag_fat(_tmp54,sizeof(char),40U);});Cyc_CfFlowInfo_aerr(_tmp5D1,_tmp5D0,_tag_fat(_tmp53,sizeof(void*),0U));});});{
# 340
struct Cyc_Dict_Dict ans_d=({Cyc_CfFlowInfo_escape_deref(env->fenv,d,r);});
# 343
if(d.t == ans_d.t)return inflow;{union Cyc_CfFlowInfo_FlowInfo ans=({Cyc_CfFlowInfo_ReachableFL(ans_d,relns);});
# 345
({Cyc_NewControlFlow_update_tryflow(env,ans);});
return ans;}}}}_LL0:;}struct _tuple23{struct Cyc_Absyn_Tqual f1;void*f2;};
# 350
static void Cyc_NewControlFlow_check_nounique(struct Cyc_NewControlFlow_AnalEnv*env,unsigned loc,void*t,void*r){
struct _tuple0 _stmttmp4=({struct _tuple0 _tmp51B;({void*_tmp5D2=({Cyc_Tcutil_compress(t);});_tmp51B.f1=_tmp5D2;}),_tmp51B.f2=r;_tmp51B;});struct _tuple0 _tmp56=_stmttmp4;struct _fat_ptr _tmp59;struct Cyc_List_List*_tmp58;enum Cyc_Absyn_AggrKind _tmp57;struct _fat_ptr _tmp5B;struct Cyc_List_List*_tmp5A;struct _fat_ptr _tmp5E;struct Cyc_List_List*_tmp5D;union Cyc_Absyn_AggrInfo _tmp5C;struct _fat_ptr _tmp60;struct Cyc_Absyn_Datatypefield*_tmp5F;void*_tmp61;switch(*((int*)_tmp56.f2)){case 2U: if(((struct Cyc_CfFlowInfo_UnknownR_CfFlowInfo_AbsRVal_struct*)_tmp56.f2)->f1 == Cyc_CfFlowInfo_NoneIL){_LL1: _LL2:
 return;}else{switch(*((int*)_tmp56.f1)){case 0U: switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp56.f1)->f1)){case 21U: if(((((struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp56.f1)->f1)->f1).KnownDatatypefield).tag == 2)goto _LL13;else{goto _LL13;}case 22U: goto _LL13;default: goto _LL13;}case 6U: goto _LL13;case 7U: goto _LL13;case 3U: goto _LL11;default: goto _LL13;}}case 0U: _LL3: _LL4:
 return;case 7U: _LL5: _LL6:
 return;case 8U: _LL7: _tmp61=(void*)((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp56.f2)->f2;_LL8: {void*r=_tmp61;
({Cyc_NewControlFlow_check_nounique(env,loc,t,r);});return;}default: switch(*((int*)_tmp56.f1)){case 0U: switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp56.f1)->f1)){case 21U: if(((((struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp56.f1)->f1)->f1).KnownDatatypefield).tag == 2){if(((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp56.f2)->tag == 6U){_LL9: _tmp5F=(((((struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp56.f1)->f1)->f1).KnownDatatypefield).val).f2;_tmp60=((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp56.f2)->f2;_LLA: {struct Cyc_Absyn_Datatypefield*tuf=_tmp5F;struct _fat_ptr ad=_tmp60;
# 357
if(tuf->typs == 0)
return;
# 357
_tmp5A=tuf->typs;_tmp5B=ad;goto _LLC;}}else{goto _LL13;}}else{goto _LL13;}case 22U: if(((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp56.f2)->tag == 6U){_LLD: _tmp5C=((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp56.f1)->f1)->f1;_tmp5D=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp56.f1)->f2;_tmp5E=((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp56.f2)->f2;_LLE: {union Cyc_Absyn_AggrInfo info=_tmp5C;struct Cyc_List_List*targs=_tmp5D;struct _fat_ptr d=_tmp5E;
# 368
struct Cyc_Absyn_Aggrdecl*ad=({Cyc_Absyn_get_known_aggrdecl(info);});
if(ad->impl == 0)return;{struct Cyc_List_List*fs=((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->fields;
# 371
struct _RegionHandle _tmp62=_new_region("temp");struct _RegionHandle*temp=& _tmp62;_push_region(temp);
{struct Cyc_List_List*inst=({Cyc_List_rzip(temp,temp,ad->tvs,targs);});
{int i=0;for(0;(unsigned)i < _get_fat_size(d,sizeof(void*));(i ++,fs=fs->tl)){
void*t=((struct Cyc_Absyn_Aggrfield*)((struct Cyc_List_List*)_check_null(fs))->hd)->type;
if(inst != 0)t=({Cyc_Tcutil_rsubstitute(temp,inst,t);});({Cyc_NewControlFlow_check_nounique(env,loc,t,((void**)d.curr)[i]);});}}
# 378
_npop_handler(0U);return;}
# 372
;_pop_region();}}}else{goto _LL13;}default: goto _LL13;}case 6U: if(((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp56.f2)->tag == 6U){_LLB: _tmp5A=((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp56.f1)->f1;_tmp5B=((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp56.f2)->f2;_LLC: {struct Cyc_List_List*tqts=_tmp5A;struct _fat_ptr ad=_tmp5B;
# 361
unsigned sz=(unsigned)({Cyc_List_length(tqts);});
{int i=0;for(0;(unsigned)i < sz;(i ++,tqts=tqts->tl)){
({({struct Cyc_NewControlFlow_AnalEnv*_tmp5D5=env;unsigned _tmp5D4=loc;void*_tmp5D3=(*((struct _tuple23*)((struct Cyc_List_List*)_check_null(tqts))->hd)).f2;Cyc_NewControlFlow_check_nounique(_tmp5D5,_tmp5D4,_tmp5D3,*((void**)_check_fat_subscript(ad,sizeof(void*),i)));});});}}
# 365
return;}}else{goto _LL13;}case 7U: if(((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp56.f2)->tag == 6U){_LLF: _tmp57=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp56.f1)->f1;_tmp58=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp56.f1)->f2;_tmp59=((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp56.f2)->f2;_LL10: {enum Cyc_Absyn_AggrKind k=_tmp57;struct Cyc_List_List*fs=_tmp58;struct _fat_ptr d=_tmp59;
# 381
{int i=0;for(0;(unsigned)i < _get_fat_size(d,sizeof(void*));(i ++,fs=fs->tl)){
({Cyc_NewControlFlow_check_nounique(env,loc,((struct Cyc_Absyn_Aggrfield*)((struct Cyc_List_List*)_check_null(fs))->hd)->type,((void**)d.curr)[i]);});}}
return;}}else{goto _LL13;}case 3U: _LL11: _LL12:
# 385
 if(({Cyc_Tcutil_is_noalias_pointer(t,0);}))
({void*_tmp63=0U;({unsigned _tmp5D7=loc;struct _fat_ptr _tmp5D6=({const char*_tmp64="argument may still contain alias-free pointers";_tag_fat(_tmp64,sizeof(char),47U);});Cyc_Warn_warn(_tmp5D7,_tmp5D6,_tag_fat(_tmp63,sizeof(void*),0U));});});
# 385
return;default: _LL13: _LL14:
# 388
 return;}}_LL0:;}
# 392
static union Cyc_CfFlowInfo_FlowInfo Cyc_NewControlFlow_use_nounique_Rval(struct Cyc_NewControlFlow_AnalEnv*env,unsigned loc,void*t,union Cyc_CfFlowInfo_FlowInfo inflow,void*r){
union Cyc_CfFlowInfo_FlowInfo _tmp66=inflow;struct Cyc_List_List*_tmp68;struct Cyc_Dict_Dict _tmp67;if((_tmp66.BottomFL).tag == 1){_LL1: _LL2:
 return({Cyc_CfFlowInfo_BottomFL();});}else{_LL3: _tmp67=((_tmp66.ReachableFL).val).f1;_tmp68=((_tmp66.ReachableFL).val).f2;_LL4: {struct Cyc_Dict_Dict d=_tmp67;struct Cyc_List_List*relns=_tmp68;
# 396
if(!({Cyc_Tcutil_is_noalias_pointer(t,0);}))
({void*_tmp69=0U;({int(*_tmp5D9)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp6B)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp6B;});struct _fat_ptr _tmp5D8=({const char*_tmp6A="noliveunique attribute requires alias-free pointer";_tag_fat(_tmp6A,sizeof(char),51U);});_tmp5D9(_tmp5D8,_tag_fat(_tmp69,sizeof(void*),0U));});});{
# 396
void*elt_type=({Cyc_Tcutil_pointer_elt_type(t);});
# 399
retry: {void*_tmp6C=r;void*_tmp6D;struct Cyc_CfFlowInfo_Place*_tmp6E;void*_tmp6F;switch(*((int*)_tmp6C)){case 8U: _LL6: _tmp6F=(void*)((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp6C)->f2;_LL7: {void*r2=_tmp6F;
r=r2;goto retry;}case 4U: _LL8: _tmp6E=((struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct*)_tmp6C)->f1;_LL9: {struct Cyc_CfFlowInfo_Place*p=_tmp6E;
_tmp6D=({Cyc_CfFlowInfo_lookup_place(d,p);});goto _LLB;}case 5U: _LLA: _tmp6D=(void*)((struct Cyc_CfFlowInfo_UniquePtr_CfFlowInfo_AbsRVal_struct*)_tmp6C)->f1;_LLB: {void*r=_tmp6D;
({Cyc_NewControlFlow_check_nounique(env,loc,elt_type,r);});goto _LL5;}default: _LLC: _LLD:
# 404
 if(({Cyc_Tcutil_is_noalias_pointer_or_aggr(elt_type);}))
({void*_tmp70=0U;({unsigned _tmp5DB=loc;struct _fat_ptr _tmp5DA=({const char*_tmp71="argument may contain live alias-free pointers";_tag_fat(_tmp71,sizeof(char),46U);});Cyc_Warn_warn(_tmp5DB,_tmp5DA,_tag_fat(_tmp70,sizeof(void*),0U));});});
# 404
return({Cyc_NewControlFlow_use_Rval(env,loc,inflow,r);});}_LL5:;}{
# 408
struct Cyc_Dict_Dict ans_d=({Cyc_CfFlowInfo_escape_deref(env->fenv,d,r);});
if(d.t == ans_d.t)return inflow;{union Cyc_CfFlowInfo_FlowInfo ans=({Cyc_CfFlowInfo_ReachableFL(ans_d,relns);});
# 411
({Cyc_NewControlFlow_update_tryflow(env,ans);});
return ans;}}}}}_LL0:;}struct _tuple24{union Cyc_CfFlowInfo_FlowInfo f1;struct Cyc_List_List*f2;};
# 418
static struct _tuple24 Cyc_NewControlFlow_anal_Rexps(struct Cyc_NewControlFlow_AnalEnv*env,union Cyc_CfFlowInfo_FlowInfo inflow,struct Cyc_List_List*es,int first_is_copy,int others_are_copy){
# 423
struct Cyc_List_List*rvals=0;
if(es == 0)
return({struct _tuple24 _tmp51C;_tmp51C.f1=inflow,_tmp51C.f2=0;_tmp51C;});
# 424
do{
# 427
struct _tuple17 _stmttmp5=({Cyc_NewControlFlow_anal_Rexp(env,first_is_copy,inflow,(struct Cyc_Absyn_Exp*)es->hd);});void*_tmp74;union Cyc_CfFlowInfo_FlowInfo _tmp73;_LL1: _tmp73=_stmttmp5.f1;_tmp74=_stmttmp5.f2;_LL2: {union Cyc_CfFlowInfo_FlowInfo f=_tmp73;void*r=_tmp74;
inflow=f;
rvals=({struct Cyc_List_List*_tmp75=_cycalloc(sizeof(*_tmp75));_tmp75->hd=r,_tmp75->tl=rvals;_tmp75;});
es=es->tl;
first_is_copy=others_are_copy;}}while(es != 0);
# 434
({Cyc_NewControlFlow_update_tryflow(env,inflow);});
return({struct _tuple24 _tmp51D;_tmp51D.f1=inflow,({struct Cyc_List_List*_tmp5DC=({Cyc_List_imp_rev(rvals);});_tmp51D.f2=_tmp5DC;});_tmp51D;});}
# 440
static struct _tuple17 Cyc_NewControlFlow_anal_use_ints(struct Cyc_NewControlFlow_AnalEnv*env,union Cyc_CfFlowInfo_FlowInfo inflow,struct Cyc_List_List*es){
# 442
struct _tuple24 _stmttmp6=({Cyc_NewControlFlow_anal_Rexps(env,inflow,es,0,0);});struct Cyc_List_List*_tmp78;union Cyc_CfFlowInfo_FlowInfo _tmp77;_LL1: _tmp77=_stmttmp6.f1;_tmp78=_stmttmp6.f2;_LL2: {union Cyc_CfFlowInfo_FlowInfo afterflow=_tmp77;struct Cyc_List_List*rvals=_tmp78;
# 444
{union Cyc_CfFlowInfo_FlowInfo _tmp79=afterflow;struct Cyc_Dict_Dict _tmp7A;if((_tmp79.ReachableFL).tag == 2){_LL4: _tmp7A=((_tmp79.ReachableFL).val).f1;_LL5: {struct Cyc_Dict_Dict d=_tmp7A;
# 446
for(0;rvals != 0;(rvals=rvals->tl,es=((struct Cyc_List_List*)_check_null(es))->tl)){
if((int)({Cyc_CfFlowInfo_initlevel(env->fenv,d,(void*)rvals->hd);})== (int)Cyc_CfFlowInfo_NoneIL)
({void*_tmp7B=0U;({unsigned _tmp5DE=((struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(es))->hd)->loc;struct _fat_ptr _tmp5DD=({const char*_tmp7C="expression may not be initialized";_tag_fat(_tmp7C,sizeof(char),34U);});Cyc_CfFlowInfo_aerr(_tmp5DE,_tmp5DD,_tag_fat(_tmp7B,sizeof(void*),0U));});});}
# 446
goto _LL3;}}else{_LL6: _LL7:
# 450
 goto _LL3;}_LL3:;}
# 452
return({struct _tuple17 _tmp51E;_tmp51E.f1=afterflow,_tmp51E.f2=(env->fenv)->unknown_all;_tmp51E;});}}
# 459
static void*Cyc_NewControlFlow_consume_zero_rval(struct Cyc_NewControlFlow_AnalEnv*env,struct Cyc_Dict_Dict new_dict,struct Cyc_CfFlowInfo_Place*p,struct Cyc_Absyn_Exp*e,void*new_rval){
# 466
int needs_unconsume=0;
void*old_rval=({Cyc_CfFlowInfo_lookup_place(new_dict,p);});
if(({Cyc_CfFlowInfo_is_unique_consumed(e,env->iteration_num,old_rval,& needs_unconsume);}))
({void*_tmp7E=0U;({int(*_tmp5E0)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp80)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp80;});struct _fat_ptr _tmp5DF=({const char*_tmp7F="consume_zero_rval";_tag_fat(_tmp7F,sizeof(char),18U);});_tmp5E0(_tmp5DF,_tag_fat(_tmp7E,sizeof(void*),0U));});});else{
# 471
if(needs_unconsume)
return({Cyc_CfFlowInfo_make_unique_consumed(env->fenv,(void*)_check_null(e->topt),e,env->iteration_num,new_rval);});else{
# 475
return new_rval;}}}
# 488 "new_control_flow.cyc"
static union Cyc_CfFlowInfo_FlowInfo Cyc_NewControlFlow_notzero(struct Cyc_NewControlFlow_AnalEnv*env,union Cyc_CfFlowInfo_FlowInfo inflow,union Cyc_CfFlowInfo_FlowInfo outflow,struct Cyc_Absyn_Exp*e,enum Cyc_CfFlowInfo_InitLevel il,struct Cyc_List_List*names){
# 493
union Cyc_CfFlowInfo_FlowInfo _tmp82=outflow;struct Cyc_List_List*_tmp84;struct Cyc_Dict_Dict _tmp83;if((_tmp82.BottomFL).tag == 1){_LL1: _LL2:
 return outflow;}else{_LL3: _tmp83=((_tmp82.ReachableFL).val).f1;_tmp84=((_tmp82.ReachableFL).val).f2;_LL4: {struct Cyc_Dict_Dict d=_tmp83;struct Cyc_List_List*relns=_tmp84;
# 496
union Cyc_CfFlowInfo_AbsLVal _stmttmp7=({Cyc_NewControlFlow_anal_Lexp(env,inflow,0,0,e);}).f2;union Cyc_CfFlowInfo_AbsLVal _tmp85=_stmttmp7;struct Cyc_CfFlowInfo_Place*_tmp86;if((_tmp85.UnknownL).tag == 2){_LL6: _LL7:
# 500
 return outflow;}else{_LL8: _tmp86=(_tmp85.PlaceL).val;_LL9: {struct Cyc_CfFlowInfo_Place*p=_tmp86;
# 504
void*nzval=(int)il == (int)1U?(env->fenv)->notzeroall:(env->fenv)->unknown_none;
for(0;names != 0;names=names->tl){
nzval=(void*)({struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*_tmp87=_cycalloc(sizeof(*_tmp87));_tmp87->tag=8U,_tmp87->f1=(struct Cyc_Absyn_Vardecl*)names->hd,_tmp87->f2=nzval;_tmp87;});}
# 508
nzval=({Cyc_NewControlFlow_consume_zero_rval(env,d,p,e,nzval);});{
union Cyc_CfFlowInfo_FlowInfo outflow=({union Cyc_CfFlowInfo_FlowInfo(*_tmp88)(struct Cyc_Dict_Dict,struct Cyc_List_List*)=Cyc_CfFlowInfo_ReachableFL;struct Cyc_Dict_Dict _tmp89=({Cyc_CfFlowInfo_assign_place(env->fenv,e->loc,d,p,nzval);});struct Cyc_List_List*_tmp8A=relns;_tmp88(_tmp89,_tmp8A);});
# 513
return outflow;}}}_LL5:;}}_LL0:;}
# 522
static struct _tuple20 Cyc_NewControlFlow_splitzero(struct Cyc_NewControlFlow_AnalEnv*env,union Cyc_CfFlowInfo_FlowInfo inflow,union Cyc_CfFlowInfo_FlowInfo outflow,struct Cyc_Absyn_Exp*e,enum Cyc_CfFlowInfo_InitLevel il,struct Cyc_List_List*names){
# 524
union Cyc_CfFlowInfo_FlowInfo _tmp8C=outflow;struct Cyc_List_List*_tmp8E;struct Cyc_Dict_Dict _tmp8D;if((_tmp8C.BottomFL).tag == 1){_LL1: _LL2:
 return({struct _tuple20 _tmp51F;_tmp51F.f1=outflow,_tmp51F.f2=outflow;_tmp51F;});}else{_LL3: _tmp8D=((_tmp8C.ReachableFL).val).f1;_tmp8E=((_tmp8C.ReachableFL).val).f2;_LL4: {struct Cyc_Dict_Dict d=_tmp8D;struct Cyc_List_List*relns=_tmp8E;
# 527
union Cyc_CfFlowInfo_AbsLVal _stmttmp8=({Cyc_NewControlFlow_anal_Lexp(env,inflow,0,0,e);}).f2;union Cyc_CfFlowInfo_AbsLVal _tmp8F=_stmttmp8;struct Cyc_CfFlowInfo_Place*_tmp90;if((_tmp8F.UnknownL).tag == 2){_LL6: _LL7:
 return({struct _tuple20 _tmp520;_tmp520.f1=outflow,_tmp520.f2=outflow;_tmp520;});}else{_LL8: _tmp90=(_tmp8F.PlaceL).val;_LL9: {struct Cyc_CfFlowInfo_Place*p=_tmp90;
# 530
void*nzval=(int)il == (int)1U?(env->fenv)->notzeroall:(env->fenv)->unknown_none;
void*zval=(env->fenv)->zero;
for(0;names != 0;names=names->tl){
nzval=(void*)({struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*_tmp91=_cycalloc(sizeof(*_tmp91));_tmp91->tag=8U,_tmp91->f1=(struct Cyc_Absyn_Vardecl*)names->hd,_tmp91->f2=nzval;_tmp91;});
zval=(void*)({struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*_tmp92=_cycalloc(sizeof(*_tmp92));_tmp92->tag=8U,_tmp92->f1=(struct Cyc_Absyn_Vardecl*)names->hd,_tmp92->f2=zval;_tmp92;});}
# 536
nzval=({Cyc_NewControlFlow_consume_zero_rval(env,d,p,e,nzval);});
zval=({Cyc_NewControlFlow_consume_zero_rval(env,d,p,e,zval);});
return({struct _tuple20 _tmp521;({
union Cyc_CfFlowInfo_FlowInfo _tmp5E2=({union Cyc_CfFlowInfo_FlowInfo(*_tmp93)(struct Cyc_Dict_Dict,struct Cyc_List_List*)=Cyc_CfFlowInfo_ReachableFL;struct Cyc_Dict_Dict _tmp94=({Cyc_CfFlowInfo_assign_place(env->fenv,e->loc,d,p,nzval);});struct Cyc_List_List*_tmp95=relns;_tmp93(_tmp94,_tmp95);});_tmp521.f1=_tmp5E2;}),({
union Cyc_CfFlowInfo_FlowInfo _tmp5E1=({union Cyc_CfFlowInfo_FlowInfo(*_tmp96)(struct Cyc_Dict_Dict,struct Cyc_List_List*)=Cyc_CfFlowInfo_ReachableFL;struct Cyc_Dict_Dict _tmp97=({Cyc_CfFlowInfo_assign_place(env->fenv,e->loc,d,p,zval);});struct Cyc_List_List*_tmp98=relns;_tmp96(_tmp97,_tmp98);});_tmp521.f2=_tmp5E1;});_tmp521;});}}_LL5:;}}_LL0:;}
# 522
static struct Cyc_CfFlowInfo_NotZero_Absyn_AbsynAnnot_struct Cyc_NewControlFlow_mt_notzero_v={Cyc_CfFlowInfo_NotZero,0};
# 546
static struct Cyc_CfFlowInfo_UnknownZ_Absyn_AbsynAnnot_struct Cyc_NewControlFlow_mt_unknownz_v={Cyc_CfFlowInfo_UnknownZ,0};
# 548
static void Cyc_NewControlFlow_update_relns(struct Cyc_Absyn_Exp*e,int possibly_null,struct Cyc_List_List*relns){
# 551
{void*_stmttmp9=e->annot;void*_tmp9A=_stmttmp9;struct Cyc_List_List*_tmp9B;struct Cyc_List_List*_tmp9C;if(((struct Cyc_CfFlowInfo_UnknownZ_Absyn_AbsynAnnot_struct*)_tmp9A)->tag == Cyc_CfFlowInfo_UnknownZ){_LL1: _tmp9C=((struct Cyc_CfFlowInfo_UnknownZ_Absyn_AbsynAnnot_struct*)_tmp9A)->f1;_LL2: {struct Cyc_List_List*relns2=_tmp9C;
# 553
if(possibly_null &&({Cyc_Relations_same_relns(relns,relns2);}))return;goto _LL0;}}else{if(((struct Cyc_CfFlowInfo_NotZero_Absyn_AbsynAnnot_struct*)_tmp9A)->tag == Cyc_CfFlowInfo_NotZero){_LL3: _tmp9B=((struct Cyc_CfFlowInfo_NotZero_Absyn_AbsynAnnot_struct*)_tmp9A)->f1;_LL4: {struct Cyc_List_List*relns2=_tmp9B;
# 556
if(!possibly_null &&({Cyc_Relations_same_relns(relns,relns2);}))return;goto _LL0;}}else{_LL5: _LL6:
# 558
 goto _LL0;}}_LL0:;}
# 560
if(possibly_null)
({void*_tmp5E3=(void*)({struct Cyc_CfFlowInfo_UnknownZ_Absyn_AbsynAnnot_struct*_tmp9D=_cycalloc(sizeof(*_tmp9D));_tmp9D->tag=Cyc_CfFlowInfo_UnknownZ,_tmp9D->f1=relns;_tmp9D;});e->annot=_tmp5E3;});else{
# 563
({void*_tmp5E4=(void*)({struct Cyc_CfFlowInfo_NotZero_Absyn_AbsynAnnot_struct*_tmp9E=_cycalloc(sizeof(*_tmp9E));_tmp9E->tag=Cyc_CfFlowInfo_NotZero,_tmp9E->f1=relns;_tmp9E;});e->annot=_tmp5E4;});}}
# 571
static struct _tuple17 Cyc_NewControlFlow_anal_derefR(struct Cyc_NewControlFlow_AnalEnv*env,union Cyc_CfFlowInfo_FlowInfo inflow,union Cyc_CfFlowInfo_FlowInfo f,struct Cyc_Absyn_Exp*e,void*r,struct Cyc_Absyn_Exp*index){
# 575
void*_stmttmpA=({Cyc_Tcutil_compress((void*)_check_null(e->topt));});void*_tmpA0=_stmttmpA;void*_tmpA3;void*_tmpA2;void*_tmpA1;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmpA0)->tag == 3U){_LL1: _tmpA1=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmpA0)->f1).elt_type;_tmpA2=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmpA0)->f1).ptr_atts).rgn;_tmpA3=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmpA0)->f1).ptr_atts).bounds;_LL2: {void*elttype=_tmpA1;void*rgn=_tmpA2;void*bd=_tmpA3;
# 577
union Cyc_CfFlowInfo_FlowInfo _tmpA4=f;struct Cyc_List_List*_tmpA6;struct Cyc_Dict_Dict _tmpA5;if((_tmpA4.BottomFL).tag == 1){_LL6: _LL7:
# 579
 return({struct _tuple17 _tmp522;_tmp522.f1=f,({void*_tmp5E5=({Cyc_CfFlowInfo_typ_to_absrval(env->fenv,elttype,0,(env->fenv)->unknown_all);});_tmp522.f2=_tmp5E5;});_tmp522;});}else{_LL8: _tmpA5=((_tmpA4.ReachableFL).val).f1;_tmpA6=((_tmpA4.ReachableFL).val).f2;_LL9: {struct Cyc_Dict_Dict outdict=_tmpA5;struct Cyc_List_List*relns=_tmpA6;
# 581
struct _tuple16 _stmttmpB=({Cyc_CfFlowInfo_unname_rval(r);});struct Cyc_List_List*_tmpA8;void*_tmpA7;_LLB: _tmpA7=_stmttmpB.f1;_tmpA8=_stmttmpB.f2;_LLC: {void*r=_tmpA7;struct Cyc_List_List*names=_tmpA8;
{void*_tmpA9=r;enum Cyc_CfFlowInfo_InitLevel _tmpAA;void*_tmpAB;struct Cyc_CfFlowInfo_Place*_tmpAC;void*_tmpAE;struct Cyc_Absyn_Vardecl*_tmpAD;switch(*((int*)_tmpA9)){case 8U: _LLE: _tmpAD=((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmpA9)->f1;_tmpAE=(void*)((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmpA9)->f2;_LLF: {struct Cyc_Absyn_Vardecl*n=_tmpAD;void*r2=_tmpAE;
# 584
({void*_tmpAF=0U;({int(*_tmp5E7)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmpB1)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmpB1;});struct _fat_ptr _tmp5E6=({const char*_tmpB0="named location in anal_derefR";_tag_fat(_tmpB0,sizeof(char),30U);});_tmp5E7(_tmp5E6,_tag_fat(_tmpAF,sizeof(void*),0U));});});}case 1U: _LL10: _LL11:
# 591
({Cyc_NewControlFlow_update_relns(e,0,relns);});
goto _LLD;case 4U: _LL12: _tmpAC=((struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct*)_tmpA9)->f1;_LL13: {struct Cyc_CfFlowInfo_Place*p=_tmpAC;
# 594
({Cyc_NewControlFlow_update_relns(e,0,relns);});
if(index == 0 &&({Cyc_Tcutil_is_bound_one(bd);}))
return({struct _tuple17 _tmp523;_tmp523.f1=f,({void*_tmp5E8=({Cyc_CfFlowInfo_lookup_place(outdict,p);});_tmp523.f2=_tmp5E8;});_tmp523;});
# 595
goto _LLD;}case 5U: _LL14: _tmpAB=(void*)((struct Cyc_CfFlowInfo_UniquePtr_CfFlowInfo_AbsRVal_struct*)_tmpA9)->f1;_LL15: {void*r=_tmpAB;
# 600
({Cyc_NewControlFlow_update_relns(e,1,relns);});
if(index == 0 &&({Cyc_Tcutil_is_bound_one(bd);}))
return({struct _tuple17 _tmp524;_tmp524.f1=f,_tmp524.f2=r;_tmp524;});
# 601
goto _LLD;}case 0U: _LL16: _LL17:
# 605
 e->annot=(void*)& Cyc_CfFlowInfo_IsZero_val;
return({struct _tuple17 _tmp525;({union Cyc_CfFlowInfo_FlowInfo _tmp5EA=({Cyc_CfFlowInfo_BottomFL();});_tmp525.f1=_tmp5EA;}),({void*_tmp5E9=({Cyc_CfFlowInfo_typ_to_absrval(env->fenv,elttype,0,(env->fenv)->unknown_all);});_tmp525.f2=_tmp5E9;});_tmp525;});case 2U: _LL18: _tmpAA=((struct Cyc_CfFlowInfo_UnknownR_CfFlowInfo_AbsRVal_struct*)_tmpA9)->f1;_LL19: {enum Cyc_CfFlowInfo_InitLevel il=_tmpAA;
# 609
f=({Cyc_NewControlFlow_notzero(env,inflow,f,e,il,names);});
goto _LL1B;}default: _LL1A: _LL1B:
# 612
({Cyc_NewControlFlow_update_relns(e,1,relns);});
goto _LLD;}_LLD:;}{
# 615
enum Cyc_CfFlowInfo_InitLevel _stmttmpC=({Cyc_CfFlowInfo_initlevel(env->fenv,outdict,r);});enum Cyc_CfFlowInfo_InitLevel _tmpB2=_stmttmpC;if(_tmpB2 == Cyc_CfFlowInfo_NoneIL){_LL1D: _LL1E: {
# 617
struct _tuple16 _stmttmpD=({Cyc_CfFlowInfo_unname_rval(r);});void*_tmpB3;_LL22: _tmpB3=_stmttmpD.f1;_LL23: {void*r=_tmpB3;
{void*_tmpB4=r;if(((struct Cyc_CfFlowInfo_Consumed_CfFlowInfo_AbsRVal_struct*)_tmpB4)->tag == 7U){_LL25: _LL26:
# 620
({void*_tmpB5=0U;({unsigned _tmp5EC=e->loc;struct _fat_ptr _tmp5EB=({const char*_tmpB6="attempt to dereference a consumed alias-free pointer";_tag_fat(_tmpB6,sizeof(char),53U);});Cyc_CfFlowInfo_aerr(_tmp5EC,_tmp5EB,_tag_fat(_tmpB5,sizeof(void*),0U));});});
goto _LL24;}else{_LL27: _LL28:
# 623
({void*_tmpB7=0U;({unsigned _tmp5EE=e->loc;struct _fat_ptr _tmp5ED=({const char*_tmpB8="dereference of possibly uninitialized pointer";_tag_fat(_tmpB8,sizeof(char),46U);});Cyc_CfFlowInfo_aerr(_tmp5EE,_tmp5ED,_tag_fat(_tmpB7,sizeof(void*),0U));});});}_LL24:;}
# 625
goto _LL20;}}}else{_LL1F: _LL20:
# 627
 return({struct _tuple17 _tmp526;_tmp526.f1=f,({void*_tmp5EF=({Cyc_CfFlowInfo_typ_to_absrval(env->fenv,elttype,0,(env->fenv)->unknown_all);});_tmp526.f2=_tmp5EF;});_tmp526;});}_LL1C:;}}}}_LL5:;}}else{_LL3: _LL4:
# 630
({void*_tmpB9=0U;({int(*_tmp5F1)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmpBB)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmpBB;});struct _fat_ptr _tmp5F0=({const char*_tmpBA="right deref of non-pointer-type";_tag_fat(_tmpBA,sizeof(char),32U);});_tmp5F1(_tmp5F0,_tag_fat(_tmpB9,sizeof(void*),0U));});});}_LL0:;}
# 637
static struct Cyc_List_List*Cyc_NewControlFlow_add_subscript_reln(struct Cyc_List_List*relns,struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2){
# 639
union Cyc_Relations_RelnOp n2=({Cyc_Relations_RConst(0U);});
int e2_valid_op=({Cyc_Relations_exp2relnop(e2,& n2);});
# 642
{void*_stmttmpE=e1->r;void*_tmpBD=_stmttmpE;void*_tmpBE;if(((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmpBD)->tag == 1U){_LL1: _tmpBE=(void*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmpBD)->f1;_LL2: {void*b1=_tmpBE;
# 644
struct Cyc_Absyn_Vardecl*x=({Cyc_Tcutil_nonesc_vardecl(b1);});
if(x != 0){
union Cyc_Relations_RelnOp n1=({Cyc_Relations_RNumelts(x);});
if(e2_valid_op)
relns=({Cyc_Relations_add_relation(Cyc_Core_heap_region,n2,Cyc_Relations_Rlt,n1,relns);});}
# 645
goto _LL0;}}else{_LL3: _LL4:
# 651
 goto _LL0;}_LL0:;}{
# 654
struct Cyc_Absyn_Exp*bound=({Cyc_Tcutil_get_type_bound((void*)_check_null(e1->topt));});
if(bound != 0){
union Cyc_Relations_RelnOp rbound=({Cyc_Relations_RConst(0U);});
if(({Cyc_Relations_exp2relnop(bound,& rbound);}))
relns=({Cyc_Relations_add_relation(Cyc_Core_heap_region,n2,Cyc_Relations_Rlt,rbound,relns);});}
# 655
return relns;}}
# 668
static union Cyc_CfFlowInfo_FlowInfo Cyc_NewControlFlow_restore_noconsume_arg(struct Cyc_NewControlFlow_AnalEnv*env,union Cyc_CfFlowInfo_FlowInfo inflow,struct Cyc_Absyn_Exp*exp,unsigned loc,void*old_rval){
# 673
struct _tuple19 _stmttmpF=({Cyc_NewControlFlow_anal_Lexp(env,inflow,1,1,exp);});union Cyc_CfFlowInfo_AbsLVal _tmpC0;_LL1: _tmpC0=_stmttmpF.f2;_LL2: {union Cyc_CfFlowInfo_AbsLVal lval=_tmpC0;
{struct _tuple19 _stmttmp10=({struct _tuple19 _tmp528;_tmp528.f1=inflow,_tmp528.f2=lval;_tmp528;});struct _tuple19 _tmpC1=_stmttmp10;struct Cyc_CfFlowInfo_Place*_tmpC4;struct Cyc_List_List*_tmpC3;struct Cyc_Dict_Dict _tmpC2;if(((_tmpC1.f1).ReachableFL).tag == 2){if(((_tmpC1.f2).PlaceL).tag == 1){_LL4: _tmpC2=(((_tmpC1.f1).ReachableFL).val).f1;_tmpC3=(((_tmpC1.f1).ReachableFL).val).f2;_tmpC4=((_tmpC1.f2).PlaceL).val;_LL5: {struct Cyc_Dict_Dict fd=_tmpC2;struct Cyc_List_List*relns=_tmpC3;struct Cyc_CfFlowInfo_Place*p=_tmpC4;
# 676
void*new_rval=({Cyc_CfFlowInfo_typ_to_absrval(env->fenv,(void*)_check_null(exp->topt),0,(env->fenv)->unknown_all);});
# 678
struct _tuple16 _stmttmp11=({Cyc_CfFlowInfo_unname_rval(old_rval);});struct Cyc_List_List*_tmpC6;void*_tmpC5;_LLB: _tmpC5=_stmttmp11.f1;_tmpC6=_stmttmp11.f2;_LLC: {void*old_rval=_tmpC5;struct Cyc_List_List*names=_tmpC6;
for(0;names != 0;names=names->tl){
new_rval=(void*)({struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*_tmpC7=_cycalloc(sizeof(*_tmpC7));_tmpC7->tag=8U,_tmpC7->f1=(struct Cyc_Absyn_Vardecl*)names->hd,_tmpC7->f2=new_rval;_tmpC7;});}
# 683
fd=({Cyc_CfFlowInfo_assign_place(env->fenv,loc,fd,p,new_rval);});
inflow=({Cyc_CfFlowInfo_ReachableFL(fd,relns);});
({Cyc_NewControlFlow_update_tryflow(env,inflow);});
# 689
goto _LL3;}}}else{_LL8: _LL9:
# 692
({struct Cyc_String_pa_PrintArg_struct _tmpCB=({struct Cyc_String_pa_PrintArg_struct _tmp527;_tmp527.tag=0U,({struct _fat_ptr _tmp5F2=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_exp2string(exp);}));_tmp527.f1=_tmp5F2;});_tmp527;});void*_tmpC8[1U];_tmpC8[0]=& _tmpCB;({int(*_tmp5F4)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmpCA)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmpCA;});struct _fat_ptr _tmp5F3=({const char*_tmpC9="noconsume parameter %s must be l-values";_tag_fat(_tmpC9,sizeof(char),40U);});_tmp5F4(_tmp5F3,_tag_fat(_tmpC8,sizeof(void*),1U));});});
goto _LL3;}}else{_LL6: _LL7:
# 690
 goto _LL3;}_LL3:;}
# 695
return inflow;}}
# 700
static struct _tuple17 Cyc_NewControlFlow_do_assign(struct Cyc_CfFlowInfo_FlowEnv*fenv,struct Cyc_NewControlFlow_AnalEnv*env,union Cyc_CfFlowInfo_FlowInfo outflow,struct Cyc_Absyn_Exp*lexp,union Cyc_CfFlowInfo_AbsLVal lval,struct Cyc_Absyn_Exp*rexp,void*rval,unsigned loc){
# 707
union Cyc_CfFlowInfo_FlowInfo _tmpCD=outflow;struct Cyc_List_List*_tmpCF;struct Cyc_Dict_Dict _tmpCE;if((_tmpCD.BottomFL).tag == 1){_LL1: _LL2:
# 709
 return({struct _tuple17 _tmp529;({union Cyc_CfFlowInfo_FlowInfo _tmp5F5=({Cyc_CfFlowInfo_BottomFL();});_tmp529.f1=_tmp5F5;}),_tmp529.f2=rval;_tmp529;});}else{_LL3: _tmpCE=((_tmpCD.ReachableFL).val).f1;_tmpCF=((_tmpCD.ReachableFL).val).f2;_LL4: {struct Cyc_Dict_Dict indict=_tmpCE;struct Cyc_List_List*relns=_tmpCF;
# 711
union Cyc_CfFlowInfo_AbsLVal _tmpD0=lval;struct Cyc_CfFlowInfo_Place*_tmpD1;if((_tmpD0.PlaceL).tag == 1){_LL6: _tmpD1=(_tmpD0.PlaceL).val;_LL7: {struct Cyc_CfFlowInfo_Place*p=_tmpD1;
# 719
struct Cyc_Dict_Dict outdict=({Cyc_CfFlowInfo_assign_place(fenv,loc,indict,p,rval);});
relns=({Cyc_Relations_reln_assign_exp(Cyc_Core_heap_region,relns,lexp,rexp);});
outflow=({Cyc_CfFlowInfo_ReachableFL(outdict,relns);});
if(Cyc_NewControlFlow_warn_lose_unique &&({Cyc_Tcutil_is_noalias_pointer_or_aggr((void*)_check_null(lexp->topt));})){
# 725
struct _tuple16 _stmttmp12=({struct _tuple16(*_tmpD6)(void*rv)=Cyc_CfFlowInfo_unname_rval;void*_tmpD7=({Cyc_CfFlowInfo_lookup_place(indict,p);});_tmpD6(_tmpD7);});void*_tmpD2;_LLB: _tmpD2=_stmttmp12.f1;_LLC: {void*rv=_tmpD2;
void*_tmpD3=rv;switch(*((int*)_tmpD3)){case 2U: if(((struct Cyc_CfFlowInfo_UnknownR_CfFlowInfo_AbsRVal_struct*)_tmpD3)->f1 == Cyc_CfFlowInfo_NoneIL){_LLE: _LLF:
 goto _LL11;}else{goto _LL14;}case 7U: _LL10: _LL11:
 goto _LL13;case 0U: _LL12: _LL13:
 goto _LLD;default: _LL14: _LL15:
# 731
({void*_tmpD4=0U;({unsigned _tmp5F7=lexp->loc;struct _fat_ptr _tmp5F6=({const char*_tmpD5="assignment may overwrite alias-free pointer(s)";_tag_fat(_tmpD5,sizeof(char),47U);});Cyc_Warn_warn(_tmp5F7,_tmp5F6,_tag_fat(_tmpD4,sizeof(void*),0U));});});
goto _LLD;}_LLD:;}}
# 736
({Cyc_NewControlFlow_update_tryflow(env,outflow);});
return({struct _tuple17 _tmp52A;_tmp52A.f1=outflow,_tmp52A.f2=rval;_tmp52A;});}}else{_LL8: _LL9:
# 742
 return({struct _tuple17 _tmp52B;({union Cyc_CfFlowInfo_FlowInfo _tmp5F8=({Cyc_NewControlFlow_use_Rval(env,rexp->loc,outflow,rval);});_tmp52B.f1=_tmp5F8;}),_tmp52B.f2=rval;_tmp52B;});}_LL5:;}}_LL0:;}
# 749
static union Cyc_CfFlowInfo_FlowInfo Cyc_NewControlFlow_do_initialize_var(struct Cyc_CfFlowInfo_FlowEnv*fenv,struct Cyc_NewControlFlow_AnalEnv*env,union Cyc_CfFlowInfo_FlowInfo f,struct Cyc_Absyn_Vardecl*vd,struct Cyc_Absyn_Exp*rexp,void*rval,unsigned loc){
# 755
union Cyc_CfFlowInfo_FlowInfo _tmpD9=f;struct Cyc_List_List*_tmpDB;struct Cyc_Dict_Dict _tmpDA;if((_tmpD9.BottomFL).tag == 1){_LL1: _LL2:
 return({Cyc_CfFlowInfo_BottomFL();});}else{_LL3: _tmpDA=((_tmpD9.ReachableFL).val).f1;_tmpDB=((_tmpD9.ReachableFL).val).f2;_LL4: {struct Cyc_Dict_Dict outdict=_tmpDA;struct Cyc_List_List*relns=_tmpDB;
# 760
outdict=({({struct Cyc_CfFlowInfo_FlowEnv*_tmp5FD=fenv;unsigned _tmp5FC=loc;struct Cyc_Dict_Dict _tmp5FB=outdict;struct Cyc_CfFlowInfo_Place*_tmp5FA=({struct Cyc_CfFlowInfo_Place*_tmpDD=_cycalloc(sizeof(*_tmpDD));
({void*_tmp5F9=(void*)({struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct*_tmpDC=_cycalloc(sizeof(*_tmpDC));_tmpDC->tag=0U,_tmpDC->f1=vd;_tmpDC;});_tmpDD->root=_tmp5F9;}),_tmpDD->path=0;_tmpDD;});Cyc_CfFlowInfo_assign_place(_tmp5FD,_tmp5FC,_tmp5FB,_tmp5FA,rval);});});
# 763
relns=({Cyc_Relations_reln_assign_var(Cyc_Core_heap_region,relns,vd,rexp);});{
union Cyc_CfFlowInfo_FlowInfo outflow=({Cyc_CfFlowInfo_ReachableFL(outdict,relns);});
({Cyc_NewControlFlow_update_tryflow(env,outflow);});
# 768
return outflow;}}}_LL0:;}struct _tuple25{struct Cyc_Absyn_Vardecl**f1;struct Cyc_Absyn_Exp*f2;};
# 772
static union Cyc_CfFlowInfo_FlowInfo Cyc_NewControlFlow_initialize_pat_vars(struct Cyc_CfFlowInfo_FlowEnv*fenv,struct Cyc_NewControlFlow_AnalEnv*env,union Cyc_CfFlowInfo_FlowInfo inflow,struct Cyc_List_List*vds,int name_locs,unsigned pat_loc,unsigned exp_loc){
# 779
if(vds == 0)return inflow;{struct Cyc_List_List*vs=({struct Cyc_List_List*(*_tmpF6)(struct Cyc_List_List*)=Cyc_Tcutil_filter_nulls;struct Cyc_List_List*_tmpF7=({Cyc_List_split(vds);}).f1;_tmpF6(_tmpF7);});
# 783
struct Cyc_List_List*es=0;
{struct Cyc_List_List*x=vds;for(0;x != 0;x=x->tl){
if((*((struct _tuple25*)x->hd)).f1 == 0)es=({struct Cyc_List_List*_tmpDF=_cycalloc(sizeof(*_tmpDF));_tmpDF->hd=(struct Cyc_Absyn_Exp*)_check_null((*((struct _tuple25*)x->hd)).f2),_tmpDF->tl=es;_tmpDF;});}}
# 788
inflow=({Cyc_NewControlFlow_add_vars(fenv,inflow,vs,fenv->unknown_all,pat_loc,name_locs);});
# 790
inflow=({Cyc_NewControlFlow_expand_unique_places(env,inflow,es);});
{struct Cyc_List_List*x=es;for(0;x != 0;x=x->tl){
# 794
struct _tuple17 _stmttmp13=({Cyc_NewControlFlow_anal_Rexp(env,1,inflow,(struct Cyc_Absyn_Exp*)x->hd);});void*_tmpE1;union Cyc_CfFlowInfo_FlowInfo _tmpE0;_LL1: _tmpE0=_stmttmp13.f1;_tmpE1=_stmttmp13.f2;_LL2: {union Cyc_CfFlowInfo_FlowInfo f=_tmpE0;void*r=_tmpE1;
inflow=({Cyc_NewControlFlow_use_Rval(env,exp_loc,f,r);});}}}{
# 802
struct Cyc_List_List*_tmpE2=({Cyc_List_rev(vds);});struct Cyc_List_List*vds=_tmpE2;
for(0;vds != 0;vds=vds->tl){
struct _tuple25*_stmttmp14=(struct _tuple25*)vds->hd;struct Cyc_Absyn_Exp*_tmpE4;struct Cyc_Absyn_Vardecl**_tmpE3;_LL4: _tmpE3=_stmttmp14->f1;_tmpE4=_stmttmp14->f2;_LL5: {struct Cyc_Absyn_Vardecl**vd=_tmpE3;struct Cyc_Absyn_Exp*ve=_tmpE4;
if(vd != 0 && ve != 0){
if(ve->topt == 0)
({struct Cyc_String_pa_PrintArg_struct _tmpE8=({struct Cyc_String_pa_PrintArg_struct _tmp52C;_tmp52C.tag=0U,({
struct _fat_ptr _tmp5FE=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_exp2string(ve);}));_tmp52C.f1=_tmp5FE;});_tmp52C;});void*_tmpE5[1U];_tmpE5[0]=& _tmpE8;({int(*_tmp600)(struct _fat_ptr fmt,struct _fat_ptr ap)=({
# 807
int(*_tmpE7)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmpE7;});struct _fat_ptr _tmp5FF=({const char*_tmpE6="oops! pattern init expr %s has no type!\n";_tag_fat(_tmpE6,sizeof(char),41U);});_tmp600(_tmp5FF,_tag_fat(_tmpE5,sizeof(void*),1U));});});{
# 816
struct Cyc_List_List l=({struct Cyc_List_List _tmp52E;_tmp52E.hd=ve,_tmp52E.tl=0;_tmp52E;});
union Cyc_CfFlowInfo_FlowInfo f=({Cyc_NewControlFlow_expand_unique_places(env,inflow,& l);});
struct _tuple19 _stmttmp15=({Cyc_NewControlFlow_anal_Lexp(env,f,0,0,ve);});union Cyc_CfFlowInfo_AbsLVal _tmpE9;_LL7: _tmpE9=_stmttmp15.f2;_LL8: {union Cyc_CfFlowInfo_AbsLVal lval=_tmpE9;
struct _tuple17 _stmttmp16=({Cyc_NewControlFlow_anal_Rexp(env,1,f,ve);});void*_tmpEB;union Cyc_CfFlowInfo_FlowInfo _tmpEA;_LLA: _tmpEA=_stmttmp16.f1;_tmpEB=_stmttmp16.f2;_LLB: {union Cyc_CfFlowInfo_FlowInfo f=_tmpEA;void*rval=_tmpEB;
union Cyc_CfFlowInfo_FlowInfo _tmpEC=f;struct Cyc_List_List*_tmpEE;struct Cyc_Dict_Dict _tmpED;if((_tmpEC.ReachableFL).tag == 2){_LLD: _tmpED=((_tmpEC.ReachableFL).val).f1;_tmpEE=((_tmpEC.ReachableFL).val).f2;_LLE: {struct Cyc_Dict_Dict fd=_tmpED;struct Cyc_List_List*relns=_tmpEE;
# 822
if(name_locs){
union Cyc_CfFlowInfo_AbsLVal _tmpEF=lval;struct Cyc_CfFlowInfo_Place*_tmpF0;if((_tmpEF.PlaceL).tag == 1){_LL12: _tmpF0=(_tmpEF.PlaceL).val;_LL13: {struct Cyc_CfFlowInfo_Place*p=_tmpF0;
# 825
rval=(void*)({struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*_tmpF1=_cycalloc(sizeof(*_tmpF1));_tmpF1->tag=8U,_tmpF1->f1=*vd,_tmpF1->f2=rval;_tmpF1;});{
# 828
void*new_rval=({Cyc_CfFlowInfo_lookup_place(fd,p);});
new_rval=(void*)({struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*_tmpF2=_cycalloc(sizeof(*_tmpF2));_tmpF2->tag=8U,_tmpF2->f1=*vd,_tmpF2->f2=new_rval;_tmpF2;});
fd=({Cyc_CfFlowInfo_assign_place(fenv,exp_loc,fd,p,new_rval);});
f=({Cyc_CfFlowInfo_ReachableFL(fd,relns);});
goto _LL11;}}}else{_LL14: _LL15:
# 835
 if(({Cyc_Tcutil_is_noalias_pointer_or_aggr((void*)_check_null(ve->topt));})&& !({Cyc_Tcutil_is_noalias_pointer_or_aggr((*vd)->type);}))
# 837
({struct Cyc_String_pa_PrintArg_struct _tmpF5=({struct Cyc_String_pa_PrintArg_struct _tmp52D;_tmp52D.tag=0U,({
struct _fat_ptr _tmp601=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_exp2string(ve);}));_tmp52D.f1=_tmp601;});_tmp52D;});void*_tmpF3[1U];_tmpF3[0]=& _tmpF5;({unsigned _tmp603=exp_loc;struct _fat_ptr _tmp602=({const char*_tmpF4="aliased pattern expression not an l-value: %s";_tag_fat(_tmpF4,sizeof(char),46U);});Cyc_CfFlowInfo_aerr(_tmp603,_tmp602,_tag_fat(_tmpF3,sizeof(void*),1U));});});}_LL11:;}
# 822
inflow=({Cyc_NewControlFlow_do_initialize_var(fenv,env,f,*vd,ve,rval,pat_loc);});
# 846
goto _LLC;}}else{_LLF: _LL10:
# 848
 goto _LLC;}_LLC:;}}}}}}
# 853
return inflow;}}}
# 856
static int Cyc_NewControlFlow_is_local_var_rooted_path(struct Cyc_Absyn_Exp*e,int cast_ok){
# 858
void*_stmttmp17=e->r;void*_tmpF9=_stmttmp17;struct Cyc_Absyn_Exp*_tmpFA;struct Cyc_Absyn_Exp*_tmpFB;struct Cyc_Absyn_Exp*_tmpFC;struct Cyc_Absyn_Exp*_tmpFD;struct Cyc_Absyn_Exp*_tmpFE;switch(*((int*)_tmpF9)){case 1U: switch(*((int*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmpF9)->f1)){case 4U: _LL1: _LL2:
 goto _LL4;case 3U: _LL3: _LL4:
 goto _LL6;case 5U: _LL5: _LL6:
 return 1;default: goto _LL11;}case 20U: _LL7: _tmpFE=((struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_tmpF9)->f1;_LL8: {struct Cyc_Absyn_Exp*e=_tmpFE;
_tmpFD=e;goto _LLA;}case 21U: _LL9: _tmpFD=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmpF9)->f1;_LLA: {struct Cyc_Absyn_Exp*e=_tmpFD;
_tmpFC=e;goto _LLC;}case 22U: _LLB: _tmpFC=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmpF9)->f1;_LLC: {struct Cyc_Absyn_Exp*e=_tmpFC;
# 865
return({Cyc_NewControlFlow_is_local_var_rooted_path(e,cast_ok);});}case 23U: _LLD: _tmpFB=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmpF9)->f1;_LLE: {struct Cyc_Absyn_Exp*e=_tmpFB;
# 867
void*_stmttmp18=({Cyc_Tcutil_compress((void*)_check_null(e->topt));});void*_tmpFF=_stmttmp18;if(((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmpFF)->tag == 6U){_LL14: _LL15:
 return({Cyc_NewControlFlow_is_local_var_rooted_path(e,cast_ok);});}else{_LL16: _LL17:
 return 0;}_LL13:;}case 14U: _LLF: _tmpFA=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmpF9)->f2;_LL10: {struct Cyc_Absyn_Exp*e=_tmpFA;
# 872
if(cast_ok)return({Cyc_NewControlFlow_is_local_var_rooted_path(e,cast_ok);});else{
return 0;}}default: _LL11: _LL12:
 return 0;}_LL0:;}
# 878
static int Cyc_NewControlFlow_subst_param(struct Cyc_List_List*exps,union Cyc_Relations_RelnOp*rop){
union Cyc_Relations_RelnOp _stmttmp19=*rop;union Cyc_Relations_RelnOp _tmp101=_stmttmp19;unsigned _tmp102;unsigned _tmp103;switch((_tmp101.RParamNumelts).tag){case 5U: _LL1: _tmp103=(_tmp101.RParam).val;_LL2: {unsigned i=_tmp103;
# 881
struct Cyc_Absyn_Exp*e=({({struct Cyc_Absyn_Exp*(*_tmp605)(struct Cyc_List_List*x,int n)=({struct Cyc_Absyn_Exp*(*_tmp104)(struct Cyc_List_List*x,int n)=(struct Cyc_Absyn_Exp*(*)(struct Cyc_List_List*x,int n))Cyc_List_nth;_tmp104;});struct Cyc_List_List*_tmp604=exps;_tmp605(_tmp604,(int)i);});});
return({Cyc_Relations_exp2relnop(e,rop);});}case 6U: _LL3: _tmp102=(_tmp101.RParamNumelts).val;_LL4: {unsigned i=_tmp102;
# 884
struct Cyc_Absyn_Exp*e=({({struct Cyc_Absyn_Exp*(*_tmp607)(struct Cyc_List_List*x,int n)=({struct Cyc_Absyn_Exp*(*_tmp108)(struct Cyc_List_List*x,int n)=(struct Cyc_Absyn_Exp*(*)(struct Cyc_List_List*x,int n))Cyc_List_nth;_tmp108;});struct Cyc_List_List*_tmp606=exps;_tmp607(_tmp606,(int)i);});});
return({int(*_tmp105)(struct Cyc_Absyn_Exp*e,union Cyc_Relations_RelnOp*p)=Cyc_Relations_exp2relnop;struct Cyc_Absyn_Exp*_tmp106=({Cyc_Absyn_prim1_exp(Cyc_Absyn_Numelts,e,0U);});union Cyc_Relations_RelnOp*_tmp107=rop;_tmp105(_tmp106,_tmp107);});}default: _LL5: _LL6:
 return 1;}_LL0:;}
# 890
static struct _fat_ptr Cyc_NewControlFlow_subst_param_string(struct Cyc_List_List*exps,union Cyc_Relations_RelnOp rop){
union Cyc_Relations_RelnOp _tmp10A=rop;unsigned _tmp10B;unsigned _tmp10C;switch((_tmp10A.RParamNumelts).tag){case 5U: _LL1: _tmp10C=(_tmp10A.RParam).val;_LL2: {unsigned i=_tmp10C;
# 893
return({struct _fat_ptr(*_tmp10D)(struct Cyc_Absyn_Exp*)=Cyc_Absynpp_exp2string;struct Cyc_Absyn_Exp*_tmp10E=({({struct Cyc_Absyn_Exp*(*_tmp609)(struct Cyc_List_List*x,int n)=({struct Cyc_Absyn_Exp*(*_tmp10F)(struct Cyc_List_List*x,int n)=(struct Cyc_Absyn_Exp*(*)(struct Cyc_List_List*x,int n))Cyc_List_nth;_tmp10F;});struct Cyc_List_List*_tmp608=exps;_tmp609(_tmp608,(int)i);});});_tmp10D(_tmp10E);});}case 6U: _LL3: _tmp10B=(_tmp10A.RParamNumelts).val;_LL4: {unsigned i=_tmp10B;
# 895
return({struct _fat_ptr(*_tmp110)(struct Cyc_Absyn_Exp*)=Cyc_Absynpp_exp2string;struct Cyc_Absyn_Exp*_tmp111=({({struct Cyc_Absyn_Exp*(*_tmp60B)(struct Cyc_List_List*x,int n)=({struct Cyc_Absyn_Exp*(*_tmp112)(struct Cyc_List_List*x,int n)=(struct Cyc_Absyn_Exp*(*)(struct Cyc_List_List*x,int n))Cyc_List_nth;_tmp112;});struct Cyc_List_List*_tmp60A=exps;_tmp60B(_tmp60A,(int)i);});});_tmp110(_tmp111);});}default: _LL5: _LL6:
 return({Cyc_Relations_rop2string(rop);});}_LL0:;}
# 900
static void Cyc_NewControlFlow_check_fn_requires(struct Cyc_NewControlFlow_AnalEnv*env,union Cyc_CfFlowInfo_FlowInfo inflow,struct Cyc_List_List*exps,struct Cyc_List_List*req,unsigned loc){
# 903
union Cyc_CfFlowInfo_FlowInfo _tmp114=inflow;struct Cyc_List_List*_tmp116;struct Cyc_Dict_Dict _tmp115;if((_tmp114.BottomFL).tag == 1){_LL1: _LL2:
 return;}else{_LL3: _tmp115=((_tmp114.ReachableFL).val).f1;_tmp116=((_tmp114.ReachableFL).val).f2;_LL4: {struct Cyc_Dict_Dict fd=_tmp115;struct Cyc_List_List*relns=_tmp116;
# 906
for(0;req != 0;req=req->tl){
struct Cyc_Relations_Reln*reln=(struct Cyc_Relations_Reln*)req->hd;
union Cyc_Relations_RelnOp rop1=reln->rop1;
union Cyc_Relations_RelnOp rop2=reln->rop2;
enum Cyc_Relations_Relation r=({Cyc_Relations_flip_relation(reln->relation);});
if((!({Cyc_NewControlFlow_subst_param(exps,& rop1);})|| !({Cyc_NewControlFlow_subst_param(exps,& rop2);}))||({int(*_tmp117)(struct Cyc_List_List*rlns)=Cyc_Relations_consistent_relations;struct Cyc_List_List*_tmp118=({Cyc_Relations_add_relation(Cyc_Core_heap_region,rop2,r,rop1,relns);});_tmp117(_tmp118);})){
# 913
struct _fat_ptr s1=({Cyc_NewControlFlow_subst_param_string(exps,rop1);});
struct _fat_ptr s2=({Cyc_Relations_relation2string(reln->relation);});
struct _fat_ptr s3=({Cyc_NewControlFlow_subst_param_string(exps,rop2);});
({struct Cyc_String_pa_PrintArg_struct _tmp11B=({struct Cyc_String_pa_PrintArg_struct _tmp532;_tmp532.tag=0U,_tmp532.f1=(struct _fat_ptr)((struct _fat_ptr)s1);_tmp532;});struct Cyc_String_pa_PrintArg_struct _tmp11C=({struct Cyc_String_pa_PrintArg_struct _tmp531;_tmp531.tag=0U,_tmp531.f1=(struct _fat_ptr)((struct _fat_ptr)s2);_tmp531;});struct Cyc_String_pa_PrintArg_struct _tmp11D=({struct Cyc_String_pa_PrintArg_struct _tmp530;_tmp530.tag=0U,_tmp530.f1=(struct _fat_ptr)((struct _fat_ptr)s3);_tmp530;});struct Cyc_String_pa_PrintArg_struct _tmp11E=({struct Cyc_String_pa_PrintArg_struct _tmp52F;_tmp52F.tag=0U,({
struct _fat_ptr _tmp60C=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Relations_relns2string(relns);}));_tmp52F.f1=_tmp60C;});_tmp52F;});void*_tmp119[4U];_tmp119[0]=& _tmp11B,_tmp119[1]=& _tmp11C,_tmp119[2]=& _tmp11D,_tmp119[3]=& _tmp11E;({unsigned _tmp60E=loc;struct _fat_ptr _tmp60D=({const char*_tmp11A="cannot prove that @requires clause %s %s %s is satisfied\n (all I know is %s)";_tag_fat(_tmp11A,sizeof(char),77U);});Cyc_CfFlowInfo_aerr(_tmp60E,_tmp60D,_tag_fat(_tmp119,sizeof(void*),4U));});});
break;}}
# 921
goto _LL0;}}_LL0:;}struct _tuple26{union Cyc_CfFlowInfo_AbsLVal f1;union Cyc_CfFlowInfo_FlowInfo f2;};struct _tuple27{struct Cyc_List_List*f1;struct Cyc_Absyn_Exp*f2;};
# 926
static struct _tuple17 Cyc_NewControlFlow_anal_Rexp(struct Cyc_NewControlFlow_AnalEnv*env,int copy_ctxt,union Cyc_CfFlowInfo_FlowInfo inflow,struct Cyc_Absyn_Exp*e){
# 930
struct Cyc_CfFlowInfo_FlowEnv*fenv=env->fenv;
struct Cyc_Dict_Dict d;
struct Cyc_List_List*relns;
# 943
{union Cyc_CfFlowInfo_FlowInfo _tmp120=inflow;struct Cyc_List_List*_tmp122;struct Cyc_Dict_Dict _tmp121;if((_tmp120.BottomFL).tag == 1){_LL1: _LL2:
 return({struct _tuple17 _tmp533;({union Cyc_CfFlowInfo_FlowInfo _tmp60F=({Cyc_CfFlowInfo_BottomFL();});_tmp533.f1=_tmp60F;}),_tmp533.f2=fenv->unknown_all;_tmp533;});}else{_LL3: _tmp121=((_tmp120.ReachableFL).val).f1;_tmp122=((_tmp120.ReachableFL).val).f2;_LL4: {struct Cyc_Dict_Dict d2=_tmp121;struct Cyc_List_List*relns2=_tmp122;
d=d2;relns=relns2;}}_LL0:;}
# 958 "new_control_flow.cyc"
if((copy_ctxt &&({Cyc_NewControlFlow_is_local_var_rooted_path(e,0);}))&&({Cyc_Tcutil_is_noalias_pointer_or_aggr((void*)_check_null(e->topt));})){
# 979 "new_control_flow.cyc"
struct _tuple19 _stmttmp1A=({Cyc_NewControlFlow_anal_Lexp(env,inflow,1,0,e);});union Cyc_CfFlowInfo_AbsLVal _tmp124;union Cyc_CfFlowInfo_FlowInfo _tmp123;_LL6: _tmp123=_stmttmp1A.f1;_tmp124=_stmttmp1A.f2;_LL7: {union Cyc_CfFlowInfo_FlowInfo flval=_tmp123;union Cyc_CfFlowInfo_AbsLVal lval=_tmp124;
struct _tuple19 _stmttmp1B=({struct _tuple19 _tmp537;_tmp537.f1=flval,_tmp537.f2=lval;_tmp537;});struct _tuple19 _tmp125=_stmttmp1B;struct Cyc_CfFlowInfo_Place*_tmp128;struct Cyc_List_List*_tmp127;struct Cyc_Dict_Dict _tmp126;if(((_tmp125.f1).ReachableFL).tag == 2){if(((_tmp125.f2).PlaceL).tag == 1){_LL9: _tmp126=(((_tmp125.f1).ReachableFL).val).f1;_tmp127=(((_tmp125.f1).ReachableFL).val).f2;_tmp128=((_tmp125.f2).PlaceL).val;_LLA: {struct Cyc_Dict_Dict fd=_tmp126;struct Cyc_List_List*r=_tmp127;struct Cyc_CfFlowInfo_Place*p=_tmp128;
# 982
void*old_rval=({Cyc_CfFlowInfo_lookup_place(fd,p);});
int needs_unconsume=0;
if(({Cyc_CfFlowInfo_is_unique_consumed(e,env->iteration_num,old_rval,& needs_unconsume);})){
({void*_tmp129=0U;({unsigned _tmp611=e->loc;struct _fat_ptr _tmp610=({const char*_tmp12A="expression attempts to copy a consumed alias-free value";_tag_fat(_tmp12A,sizeof(char),56U);});Cyc_CfFlowInfo_aerr(_tmp611,_tmp610,_tag_fat(_tmp129,sizeof(void*),0U));});});
return({struct _tuple17 _tmp534;({union Cyc_CfFlowInfo_FlowInfo _tmp612=({Cyc_CfFlowInfo_BottomFL();});_tmp534.f1=_tmp612;}),_tmp534.f2=fenv->unknown_all;_tmp534;});}else{
# 988
if(needs_unconsume)
# 990
return({struct _tuple17 _tmp535;_tmp535.f1=flval,({void*_tmp613=({Cyc_CfFlowInfo_make_unique_unconsumed(fenv,old_rval);});_tmp535.f2=_tmp613;});_tmp535;});else{
# 993
void*new_rval=({Cyc_CfFlowInfo_make_unique_consumed(fenv,(void*)_check_null(e->topt),e,env->iteration_num,old_rval);});
struct Cyc_Dict_Dict outdict=({Cyc_CfFlowInfo_assign_place(fenv,e->loc,fd,p,new_rval);});
# 1005
return({struct _tuple17 _tmp536;({union Cyc_CfFlowInfo_FlowInfo _tmp614=({Cyc_CfFlowInfo_ReachableFL(outdict,r);});_tmp536.f1=_tmp614;}),_tmp536.f2=old_rval;_tmp536;});}}}}else{goto _LLB;}}else{_LLB: _LLC:
# 1009
 goto _LL8;}_LL8:;}}{
# 958 "new_control_flow.cyc"
void*_stmttmp1C=e->r;void*_tmp12B=_stmttmp1C;struct Cyc_Absyn_Stmt*_tmp12C;int _tmp12F;void*_tmp12E;struct Cyc_Absyn_Exp*_tmp12D;int _tmp133;struct Cyc_Absyn_Exp*_tmp132;struct Cyc_Absyn_Exp*_tmp131;struct Cyc_Absyn_Vardecl*_tmp130;struct Cyc_List_List*_tmp134;struct Cyc_List_List*_tmp137;enum Cyc_Absyn_AggrKind _tmp136;struct Cyc_List_List*_tmp135;struct Cyc_List_List*_tmp138;struct Cyc_List_List*_tmp139;struct Cyc_Absyn_Exp*_tmp13B;struct Cyc_Absyn_Exp*_tmp13A;struct Cyc_Absyn_Exp*_tmp13D;struct Cyc_Absyn_Exp*_tmp13C;struct Cyc_Absyn_Exp*_tmp13F;struct Cyc_Absyn_Exp*_tmp13E;struct Cyc_Absyn_Exp*_tmp142;struct Cyc_Absyn_Exp*_tmp141;struct Cyc_Absyn_Exp*_tmp140;struct _fat_ptr*_tmp144;struct Cyc_Absyn_Exp*_tmp143;struct _fat_ptr*_tmp146;struct Cyc_Absyn_Exp*_tmp145;struct _fat_ptr*_tmp148;struct Cyc_Absyn_Exp*_tmp147;struct Cyc_Absyn_Exp*_tmp149;struct Cyc_Absyn_Exp*_tmp14A;struct Cyc_Absyn_Exp*_tmp14C;struct Cyc_Absyn_Exp*_tmp14B;struct Cyc_Absyn_Exp*_tmp14E;struct Cyc_Absyn_Exp*_tmp14D;int _tmp153;struct Cyc_Absyn_Exp*_tmp152;void**_tmp151;struct Cyc_Absyn_Exp*_tmp150;int _tmp14F;struct Cyc_List_List*_tmp155;struct Cyc_Absyn_Exp*_tmp154;struct Cyc_Absyn_Exp*_tmp157;struct Cyc_Absyn_Exp*_tmp156;struct Cyc_Absyn_Exp*_tmp158;struct Cyc_Absyn_Exp*_tmp15A;struct Cyc_Absyn_Exp*_tmp159;struct Cyc_Absyn_Exp*_tmp15C;struct Cyc_Absyn_Exp*_tmp15B;struct Cyc_Absyn_Exp*_tmp15E;struct Cyc_Absyn_Exp*_tmp15D;struct Cyc_Absyn_Exp*_tmp15F;struct Cyc_List_List*_tmp161;enum Cyc_Absyn_Primop _tmp160;struct Cyc_Absyn_Datatypedecl*_tmp163;struct Cyc_List_List*_tmp162;struct Cyc_Absyn_Vardecl*_tmp164;struct Cyc_Absyn_Vardecl*_tmp165;struct Cyc_Absyn_Vardecl*_tmp166;struct _fat_ptr _tmp167;struct Cyc_Absyn_Exp*_tmp168;struct Cyc_Absyn_Exp*_tmp169;struct Cyc_Absyn_Exp*_tmp16A;struct Cyc_Absyn_Exp*_tmp16B;struct Cyc_Absyn_Exp*_tmp16C;switch(*((int*)_tmp12B)){case 14U: if(((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp12B)->f4 == Cyc_Absyn_Null_to_NonNull){_LLE: _tmp16C=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp12B)->f2;_LLF: {struct Cyc_Absyn_Exp*e1=_tmp16C;
# 1016 "new_control_flow.cyc"
struct _tuple17 _stmttmp1D=({Cyc_NewControlFlow_anal_Rexp(env,copy_ctxt,inflow,e1);});void*_tmp16E;union Cyc_CfFlowInfo_FlowInfo _tmp16D;_LL81: _tmp16D=_stmttmp1D.f1;_tmp16E=_stmttmp1D.f2;_LL82: {union Cyc_CfFlowInfo_FlowInfo f1=_tmp16D;void*r1=_tmp16E;
struct _tuple17 _stmttmp1E=({Cyc_NewControlFlow_anal_derefR(env,inflow,f1,e1,r1,0);});void*_tmp170;union Cyc_CfFlowInfo_FlowInfo _tmp16F;_LL84: _tmp16F=_stmttmp1E.f1;_tmp170=_stmttmp1E.f2;_LL85: {union Cyc_CfFlowInfo_FlowInfo f2=_tmp16F;void*r2=_tmp170;
return({struct _tuple17 _tmp538;_tmp538.f1=f2,_tmp538.f2=r1;_tmp538;});}}}}else{_LL10: _tmp16B=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp12B)->f2;_LL11: {struct Cyc_Absyn_Exp*e1=_tmp16B;
# 1022
_tmp16A=e1;goto _LL13;}}case 12U: _LL12: _tmp16A=((struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*)_tmp12B)->f1;_LL13: {struct Cyc_Absyn_Exp*e1=_tmp16A;
_tmp169=e1;goto _LL15;}case 42U: _LL14: _tmp169=((struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct*)_tmp12B)->f1;_LL15: {struct Cyc_Absyn_Exp*e1=_tmp169;
_tmp168=e1;goto _LL17;}case 13U: _LL16: _tmp168=((struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*)_tmp12B)->f1;_LL17: {struct Cyc_Absyn_Exp*e1=_tmp168;
return({Cyc_NewControlFlow_anal_Rexp(env,copy_ctxt,inflow,e1);});}case 2U: _LL18: _tmp167=((struct Cyc_Absyn_Pragma_e_Absyn_Raw_exp_struct*)_tmp12B)->f1;_LL19: {struct _fat_ptr p=_tmp167;
# 1028
if(!({({struct _fat_ptr _tmp615=(struct _fat_ptr)p;Cyc_NewControlFlow_strcmp(_tmp615,({const char*_tmp171="print_flow";_tag_fat(_tmp171,sizeof(char),11U);}));});})){
struct _fat_ptr seg_str=({Cyc_Position_string_of_segment(e->loc);});
({struct Cyc_String_pa_PrintArg_struct _tmp174=({struct Cyc_String_pa_PrintArg_struct _tmp539;_tmp539.tag=0U,_tmp539.f1=(struct _fat_ptr)((struct _fat_ptr)seg_str);_tmp539;});void*_tmp172[1U];_tmp172[0]=& _tmp174;({struct Cyc___cycFILE*_tmp617=Cyc_stderr;struct _fat_ptr _tmp616=({const char*_tmp173="%s: current flow is\n";_tag_fat(_tmp173,sizeof(char),21U);});Cyc_fprintf(_tmp617,_tmp616,_tag_fat(_tmp172,sizeof(void*),1U));});});
({Cyc_CfFlowInfo_print_flow(inflow);});
({void*_tmp175=0U;({struct Cyc___cycFILE*_tmp619=Cyc_stderr;struct _fat_ptr _tmp618=({const char*_tmp176="\n";_tag_fat(_tmp176,sizeof(char),2U);});Cyc_fprintf(_tmp619,_tmp618,_tag_fat(_tmp175,sizeof(void*),0U));});});}else{
# 1034
if(!({({struct _fat_ptr _tmp61A=(struct _fat_ptr)p;Cyc_NewControlFlow_strcmp(_tmp61A,({const char*_tmp177="debug_on";_tag_fat(_tmp177,sizeof(char),9U);}));});}));else{
# 1039
if(!({({struct _fat_ptr _tmp61B=(struct _fat_ptr)p;Cyc_NewControlFlow_strcmp(_tmp61B,({const char*_tmp178="debug_off";_tag_fat(_tmp178,sizeof(char),10U);}));});}));else{
# 1045
({struct Cyc_String_pa_PrintArg_struct _tmp17B=({struct Cyc_String_pa_PrintArg_struct _tmp53A;_tmp53A.tag=0U,_tmp53A.f1=(struct _fat_ptr)((struct _fat_ptr)p);_tmp53A;});void*_tmp179[1U];_tmp179[0]=& _tmp17B;({unsigned _tmp61D=e->loc;struct _fat_ptr _tmp61C=({const char*_tmp17A="unknown pragma %s";_tag_fat(_tmp17A,sizeof(char),18U);});Cyc_Warn_warn(_tmp61D,_tmp61C,_tag_fat(_tmp179,sizeof(void*),1U));});});}}}
return({struct _tuple17 _tmp53B;_tmp53B.f1=inflow,_tmp53B.f2=fenv->zero;_tmp53B;});}case 41U: _LL1A: _LL1B:
# 1048
 goto _LL1D;case 0U: switch(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp12B)->f1).Wstring_c).tag){case 1U: _LL1C: _LL1D:
 goto _LL1F;case 5U: if((((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp12B)->f1).Int_c).val).f2 == 0){_LL1E: _LL1F:
 return({struct _tuple17 _tmp53C;_tmp53C.f1=inflow,_tmp53C.f2=fenv->zero;_tmp53C;});}else{_LL20: _LL21:
 goto _LL23;}case 8U: _LL22: _LL23:
 goto _LL25;case 9U: _LL24: _LL25:
 goto _LL27;default: _LL2A: _LL2B:
# 1057
 goto _LL2D;}case 1U: switch(*((int*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp12B)->f1)){case 2U: _LL26: _LL27:
# 1054
 return({struct _tuple17 _tmp53D;_tmp53D.f1=inflow,_tmp53D.f2=fenv->notzeroall;_tmp53D;});case 1U: _LL36: _LL37:
# 1065
 return({struct _tuple17 _tmp53E;_tmp53E.f1=inflow,({void*_tmp61E=({Cyc_CfFlowInfo_typ_to_absrval(fenv,(void*)_check_null(e->topt),0,fenv->unknown_all);});_tmp53E.f2=_tmp61E;});_tmp53E;});case 4U: _LL38: _tmp166=((struct Cyc_Absyn_Local_b_Absyn_Binding_struct*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp12B)->f1)->f1;_LL39: {struct Cyc_Absyn_Vardecl*vd=_tmp166;
# 1070
if((int)vd->sc == (int)Cyc_Absyn_Static)
return({struct _tuple17 _tmp53F;_tmp53F.f1=inflow,({void*_tmp61F=({Cyc_CfFlowInfo_typ_to_absrval(fenv,(void*)_check_null(e->topt),0,fenv->unknown_all);});_tmp53F.f2=_tmp61F;});_tmp53F;});
# 1070
_tmp165=vd;goto _LL3B;}case 3U: _LL3A: _tmp165=((struct Cyc_Absyn_Param_b_Absyn_Binding_struct*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp12B)->f1)->f1;_LL3B: {struct Cyc_Absyn_Vardecl*vd=_tmp165;
# 1074
_tmp164=vd;goto _LL3D;}case 5U: _LL3C: _tmp164=((struct Cyc_Absyn_Pat_b_Absyn_Binding_struct*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp12B)->f1)->f1;_LL3D: {struct Cyc_Absyn_Vardecl*vd=_tmp164;
# 1077
struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct vdroot=({struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct _tmp541;_tmp541.tag=0U,_tmp541.f1=vd;_tmp541;});
return({struct _tuple17 _tmp540;_tmp540.f1=inflow,({void*_tmp621=({({struct Cyc_Dict_Dict _tmp620=d;Cyc_Dict_lookup_other(_tmp620,Cyc_CfFlowInfo_root_cmp,(void*)({struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct*_tmp17C=_cycalloc(sizeof(*_tmp17C));*_tmp17C=vdroot;_tmp17C;}));});});_tmp540.f2=_tmp621;});_tmp540;});}default: _LL78: _LL79:
# 1685
 goto _LL7B;}case 31U: if(((struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*)_tmp12B)->f1 == 0){_LL28: _LL29:
# 1056
 goto _LL2B;}else{_LL66: _tmp162=((struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*)_tmp12B)->f1;_tmp163=((struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*)_tmp12B)->f2;_LL67: {struct Cyc_List_List*es=_tmp162;struct Cyc_Absyn_Datatypedecl*tud=_tmp163;
# 1547
_tmp139=es;goto _LL69;}}case 18U: _LL2C: _LL2D:
# 1058
 goto _LL2F;case 17U: _LL2E: _LL2F:
 goto _LL31;case 19U: _LL30: _LL31:
 goto _LL33;case 33U: _LL32: _LL33:
 goto _LL35;case 32U: _LL34: _LL35:
 return({struct _tuple17 _tmp542;_tmp542.f1=inflow,_tmp542.f2=fenv->unknown_all;_tmp542;});case 3U: _LL3E: _tmp160=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp12B)->f1;_tmp161=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp12B)->f2;_LL3F: {enum Cyc_Absyn_Primop p=_tmp160;struct Cyc_List_List*es=_tmp161;
# 1084
struct _tuple17 _stmttmp1F=({Cyc_NewControlFlow_anal_use_ints(env,inflow,es);});void*_tmp17E;union Cyc_CfFlowInfo_FlowInfo _tmp17D;_LL87: _tmp17D=_stmttmp1F.f1;_tmp17E=_stmttmp1F.f2;_LL88: {union Cyc_CfFlowInfo_FlowInfo f=_tmp17D;void*r=_tmp17E;
return({struct _tuple17 _tmp543;_tmp543.f1=f,_tmp543.f2=r;_tmp543;});}}case 5U: _LL40: _tmp15F=((struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*)_tmp12B)->f1;_LL41: {struct Cyc_Absyn_Exp*e1=_tmp15F;
# 1088
struct Cyc_List_List arg=({struct Cyc_List_List _tmp547;_tmp547.hd=e1,_tmp547.tl=0;_tmp547;});
struct _tuple17 _stmttmp20=({Cyc_NewControlFlow_anal_use_ints(env,inflow,& arg);});union Cyc_CfFlowInfo_FlowInfo _tmp17F;_LL8A: _tmp17F=_stmttmp20.f1;_LL8B: {union Cyc_CfFlowInfo_FlowInfo f=_tmp17F;
struct _tuple19 _stmttmp21=({Cyc_NewControlFlow_anal_Lexp(env,f,0,0,e1);});union Cyc_CfFlowInfo_AbsLVal _tmp180;_LL8D: _tmp180=_stmttmp21.f2;_LL8E: {union Cyc_CfFlowInfo_AbsLVal lval=_tmp180;
int iszt=({Cyc_Tcutil_is_zeroterm_pointer_type((void*)_check_null(e1->topt));});
if(iszt){
# 1095
struct _tuple17 _stmttmp22=({Cyc_NewControlFlow_anal_Rexp(env,0,inflow,e1);});void*_tmp182;union Cyc_CfFlowInfo_FlowInfo _tmp181;_LL90: _tmp181=_stmttmp22.f1;_tmp182=_stmttmp22.f2;_LL91: {union Cyc_CfFlowInfo_FlowInfo g=_tmp181;void*r=_tmp182;
({Cyc_NewControlFlow_anal_derefR(env,inflow,g,e1,r,0);});}}
# 1092
{struct _tuple26 _stmttmp23=({struct _tuple26 _tmp544;_tmp544.f1=lval,_tmp544.f2=f;_tmp544;});struct _tuple26 _tmp183=_stmttmp23;struct Cyc_List_List*_tmp186;struct Cyc_Dict_Dict _tmp185;struct Cyc_CfFlowInfo_Place*_tmp184;if(((_tmp183.f1).PlaceL).tag == 1){if(((_tmp183.f2).ReachableFL).tag == 2){_LL93: _tmp184=((_tmp183.f1).PlaceL).val;_tmp185=(((_tmp183.f2).ReachableFL).val).f1;_tmp186=(((_tmp183.f2).ReachableFL).val).f2;_LL94: {struct Cyc_CfFlowInfo_Place*p=_tmp184;struct Cyc_Dict_Dict outdict=_tmp185;struct Cyc_List_List*relns=_tmp186;
# 1100
relns=({Cyc_Relations_reln_kill_exp(Cyc_Core_heap_region,relns,e1);});
f=({union Cyc_CfFlowInfo_FlowInfo(*_tmp187)(struct Cyc_Dict_Dict,struct Cyc_List_List*)=Cyc_CfFlowInfo_ReachableFL;struct Cyc_Dict_Dict _tmp188=({Cyc_CfFlowInfo_assign_place(fenv,e1->loc,outdict,p,fenv->unknown_all);});struct Cyc_List_List*_tmp189=relns;_tmp187(_tmp188,_tmp189);});
# 1105
goto _LL92;}}else{goto _LL95;}}else{_LL95: _LL96:
 goto _LL92;}_LL92:;}
# 1109
if(iszt)
return({struct _tuple17 _tmp545;_tmp545.f1=f,_tmp545.f2=fenv->notzeroall;_tmp545;});else{
return({struct _tuple17 _tmp546;_tmp546.f1=f,_tmp546.f2=fenv->unknown_all;_tmp546;});}}}}case 4U: if(((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp12B)->f2 != 0){_LL42: _tmp15D=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp12B)->f1;_tmp15E=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp12B)->f3;_LL43: {struct Cyc_Absyn_Exp*l=_tmp15D;struct Cyc_Absyn_Exp*r=_tmp15E;
# 1114
if(copy_ctxt &&({Cyc_Tcutil_is_noalias_pointer_or_aggr((void*)_check_null(e->topt));})){
({void*_tmp18A=0U;({unsigned _tmp623=e->loc;struct _fat_ptr _tmp622=({const char*_tmp18B="cannot track alias-free pointers through multiple assignments";_tag_fat(_tmp18B,sizeof(char),62U);});Cyc_CfFlowInfo_aerr(_tmp623,_tmp622,_tag_fat(_tmp18A,sizeof(void*),0U));});});
return({struct _tuple17 _tmp548;({union Cyc_CfFlowInfo_FlowInfo _tmp624=({Cyc_CfFlowInfo_BottomFL();});_tmp548.f1=_tmp624;}),_tmp548.f2=fenv->unknown_all;_tmp548;});}{
# 1114
struct _tuple17 _stmttmp24=({Cyc_NewControlFlow_anal_Rexp(env,1,inflow,r);});void*_tmp18D;union Cyc_CfFlowInfo_FlowInfo _tmp18C;_LL98: _tmp18C=_stmttmp24.f1;_tmp18D=_stmttmp24.f2;_LL99: {union Cyc_CfFlowInfo_FlowInfo right_out=_tmp18C;void*rval=_tmp18D;
# 1119
struct _tuple19 _stmttmp25=({Cyc_NewControlFlow_anal_Lexp(env,right_out,0,0,l);});union Cyc_CfFlowInfo_AbsLVal _tmp18F;union Cyc_CfFlowInfo_FlowInfo _tmp18E;_LL9B: _tmp18E=_stmttmp25.f1;_tmp18F=_stmttmp25.f2;_LL9C: {union Cyc_CfFlowInfo_FlowInfo f=_tmp18E;union Cyc_CfFlowInfo_AbsLVal lval=_tmp18F;
{union Cyc_CfFlowInfo_FlowInfo _tmp190=f;struct Cyc_List_List*_tmp192;struct Cyc_Dict_Dict _tmp191;if((_tmp190.ReachableFL).tag == 2){_LL9E: _tmp191=((_tmp190.ReachableFL).val).f1;_tmp192=((_tmp190.ReachableFL).val).f2;_LL9F: {struct Cyc_Dict_Dict outdict=_tmp191;struct Cyc_List_List*relns=_tmp192;
# 1122
{union Cyc_CfFlowInfo_AbsLVal _tmp193=lval;struct Cyc_CfFlowInfo_Place*_tmp194;if((_tmp193.PlaceL).tag == 1){_LLA3: _tmp194=(_tmp193.PlaceL).val;_LLA4: {struct Cyc_CfFlowInfo_Place*p=_tmp194;
# 1126
relns=({Cyc_Relations_reln_kill_exp(Cyc_Core_heap_region,relns,l);});
outdict=({Cyc_CfFlowInfo_assign_place(fenv,l->loc,outdict,p,fenv->unknown_all);});
# 1129
f=({Cyc_CfFlowInfo_ReachableFL(outdict,relns);});
# 1133
goto _LLA2;}}else{_LLA5: _LLA6:
# 1136
 goto _LLA2;}_LLA2:;}
# 1138
goto _LL9D;}}else{_LLA0: _LLA1:
 goto _LL9D;}_LL9D:;}
# 1141
return({struct _tuple17 _tmp549;_tmp549.f1=f,_tmp549.f2=fenv->unknown_all;_tmp549;});}}}}}else{_LL44: _tmp15B=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp12B)->f1;_tmp15C=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp12B)->f3;_LL45: {struct Cyc_Absyn_Exp*e1=_tmp15B;struct Cyc_Absyn_Exp*e2=_tmp15C;
# 1144
if(copy_ctxt &&({Cyc_Tcutil_is_noalias_pointer_or_aggr((void*)_check_null(e->topt));})){
({void*_tmp195=0U;({unsigned _tmp626=e->loc;struct _fat_ptr _tmp625=({const char*_tmp196="cannot track alias-free pointers through multiple assignments";_tag_fat(_tmp196,sizeof(char),62U);});Cyc_CfFlowInfo_aerr(_tmp626,_tmp625,_tag_fat(_tmp195,sizeof(void*),0U));});});
return({struct _tuple17 _tmp54A;({union Cyc_CfFlowInfo_FlowInfo _tmp627=({Cyc_CfFlowInfo_BottomFL();});_tmp54A.f1=_tmp627;}),_tmp54A.f2=fenv->unknown_all;_tmp54A;});}
# 1150
inflow=({union Cyc_CfFlowInfo_FlowInfo(*_tmp197)(struct Cyc_NewControlFlow_AnalEnv*,union Cyc_CfFlowInfo_FlowInfo inflow,struct Cyc_List_List*es)=Cyc_NewControlFlow_expand_unique_places;struct Cyc_NewControlFlow_AnalEnv*_tmp198=env;union Cyc_CfFlowInfo_FlowInfo _tmp199=inflow;struct Cyc_List_List*_tmp19A=({struct Cyc_Absyn_Exp*_tmp19B[2U];_tmp19B[0]=e1,_tmp19B[1]=e2;Cyc_List_list(_tag_fat(_tmp19B,sizeof(struct Cyc_Absyn_Exp*),2U));});_tmp197(_tmp198,_tmp199,_tmp19A);});{
# 1152
struct _tuple17 _stmttmp26=({Cyc_NewControlFlow_anal_Rexp(env,1,inflow,e2);});void*_tmp19D;union Cyc_CfFlowInfo_FlowInfo _tmp19C;_LLA8: _tmp19C=_stmttmp26.f1;_tmp19D=_stmttmp26.f2;_LLA9: {union Cyc_CfFlowInfo_FlowInfo right_out=_tmp19C;void*rval=_tmp19D;
struct _tuple19 _stmttmp27=({Cyc_NewControlFlow_anal_Lexp(env,right_out,0,0,e1);});union Cyc_CfFlowInfo_AbsLVal _tmp19F;union Cyc_CfFlowInfo_FlowInfo _tmp19E;_LLAB: _tmp19E=_stmttmp27.f1;_tmp19F=_stmttmp27.f2;_LLAC: {union Cyc_CfFlowInfo_FlowInfo outflow=_tmp19E;union Cyc_CfFlowInfo_AbsLVal lval=_tmp19F;
struct _tuple17 _stmttmp28=({Cyc_NewControlFlow_do_assign(fenv,env,outflow,e1,lval,e2,rval,e->loc);});void*_tmp1A1;union Cyc_CfFlowInfo_FlowInfo _tmp1A0;_LLAE: _tmp1A0=_stmttmp28.f1;_tmp1A1=_stmttmp28.f2;_LLAF: {union Cyc_CfFlowInfo_FlowInfo outflow=_tmp1A0;void*rval=_tmp1A1;
# 1158
return({struct _tuple17 _tmp54B;_tmp54B.f1=outflow,_tmp54B.f2=rval;_tmp54B;});}}}}}}case 9U: _LL46: _tmp159=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp12B)->f1;_tmp15A=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp12B)->f2;_LL47: {struct Cyc_Absyn_Exp*e1=_tmp159;struct Cyc_Absyn_Exp*e2=_tmp15A;
# 1161
struct _tuple17 _stmttmp29=({Cyc_NewControlFlow_anal_Rexp(env,0,inflow,e1);});void*_tmp1A3;union Cyc_CfFlowInfo_FlowInfo _tmp1A2;_LLB1: _tmp1A2=_stmttmp29.f1;_tmp1A3=_stmttmp29.f2;_LLB2: {union Cyc_CfFlowInfo_FlowInfo f=_tmp1A2;void*r=_tmp1A3;
return({Cyc_NewControlFlow_anal_Rexp(env,copy_ctxt,f,e2);});}}case 11U: _LL48: _tmp158=((struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct*)_tmp12B)->f1;_LL49: {struct Cyc_Absyn_Exp*e1=_tmp158;
# 1165
struct _tuple17 _stmttmp2A=({Cyc_NewControlFlow_anal_Rexp(env,1,inflow,e1);});void*_tmp1A5;union Cyc_CfFlowInfo_FlowInfo _tmp1A4;_LLB4: _tmp1A4=_stmttmp2A.f1;_tmp1A5=_stmttmp2A.f2;_LLB5: {union Cyc_CfFlowInfo_FlowInfo f=_tmp1A4;void*r=_tmp1A5;
({Cyc_NewControlFlow_use_Rval(env,e1->loc,f,r);});
return({struct _tuple17 _tmp54C;({union Cyc_CfFlowInfo_FlowInfo _tmp629=({Cyc_CfFlowInfo_BottomFL();});_tmp54C.f1=_tmp629;}),({void*_tmp628=({Cyc_CfFlowInfo_typ_to_absrval(fenv,(void*)_check_null(e->topt),0,fenv->unknown_all);});_tmp54C.f2=_tmp628;});_tmp54C;});}}case 34U: _LL4A: _tmp156=((struct Cyc_Absyn_Spawn_e_Absyn_Raw_exp_struct*)_tmp12B)->f1;_tmp157=((struct Cyc_Absyn_Spawn_e_Absyn_Raw_exp_struct*)_tmp12B)->f2;_LL4B: {struct Cyc_Absyn_Exp*e1=_tmp156;struct Cyc_Absyn_Exp*e2=_tmp157;
# 1170
struct _tuple17 _stmttmp2B=({Cyc_NewControlFlow_anal_Rexp(env,0,inflow,e1);});void*_tmp1A7;union Cyc_CfFlowInfo_FlowInfo _tmp1A6;_LLB7: _tmp1A6=_stmttmp2B.f1;_tmp1A7=_stmttmp2B.f2;_LLB8: {union Cyc_CfFlowInfo_FlowInfo f=_tmp1A6;void*r=_tmp1A7;
return({Cyc_NewControlFlow_anal_Rexp(env,copy_ctxt,f,e2);});}}case 10U: _LL4C: _tmp154=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp12B)->f1;_tmp155=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp12B)->f2;_LL4D: {struct Cyc_Absyn_Exp*e1=_tmp154;struct Cyc_List_List*es=_tmp155;
# 1173
struct Cyc_List_List*orig_es=es;
struct _tuple17 _stmttmp2C=({Cyc_NewControlFlow_anal_Rexp(env,0,inflow,e1);});void*_tmp1A9;union Cyc_CfFlowInfo_FlowInfo _tmp1A8;_LLBA: _tmp1A8=_stmttmp2C.f1;_tmp1A9=_stmttmp2C.f2;_LLBB: {union Cyc_CfFlowInfo_FlowInfo f1=_tmp1A8;void*r1=_tmp1A9;
# 1176
({Cyc_NewControlFlow_anal_derefR(env,inflow,f1,e1,r1,0);});{
# 1179
struct _tuple24 _stmttmp2D=({struct _tuple24(*_tmp1DB)(struct Cyc_NewControlFlow_AnalEnv*env,union Cyc_CfFlowInfo_FlowInfo inflow,struct Cyc_List_List*es,int first_is_copy,int others_are_copy)=Cyc_NewControlFlow_anal_Rexps;struct Cyc_NewControlFlow_AnalEnv*_tmp1DC=env;union Cyc_CfFlowInfo_FlowInfo _tmp1DD=f1;struct Cyc_List_List*_tmp1DE=({Cyc_List_copy(es);});int _tmp1DF=1;int _tmp1E0=1;_tmp1DB(_tmp1DC,_tmp1DD,_tmp1DE,_tmp1DF,_tmp1E0);});struct Cyc_List_List*_tmp1AB;union Cyc_CfFlowInfo_FlowInfo _tmp1AA;_LLBD: _tmp1AA=_stmttmp2D.f1;_tmp1AB=_stmttmp2D.f2;_LLBE: {union Cyc_CfFlowInfo_FlowInfo fst_outflow=_tmp1AA;struct Cyc_List_List*rvals=_tmp1AB;
# 1181
union Cyc_CfFlowInfo_FlowInfo outflow=({Cyc_NewControlFlow_use_Rval(env,e1->loc,fst_outflow,r1);});
# 1183
struct Cyc_List_List*init_params=0;
struct Cyc_List_List*nolive_unique_params=0;
struct Cyc_List_List*consume_params=0;
struct Cyc_List_List*requires;
struct Cyc_List_List*ensures;
{void*_stmttmp2E=({Cyc_Tcutil_compress((void*)_check_null(e1->topt));});void*_tmp1AC=_stmttmp2E;void*_tmp1AD;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp1AC)->tag == 3U){_LLC0: _tmp1AD=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp1AC)->f1).elt_type;_LLC1: {void*t=_tmp1AD;
# 1190
{void*_stmttmp2F=({Cyc_Tcutil_compress(t);});void*_tmp1AE=_stmttmp2F;struct Cyc_List_List*_tmp1B1;struct Cyc_List_List*_tmp1B0;struct Cyc_List_List*_tmp1AF;if(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1AE)->tag == 5U){_LLC5: _tmp1AF=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1AE)->f1).attributes;_tmp1B0=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1AE)->f1).requires_relns;_tmp1B1=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp1AE)->f1).ensures_relns;_LLC6: {struct Cyc_List_List*atts=_tmp1AF;struct Cyc_List_List*req=_tmp1B0;struct Cyc_List_List*ens=_tmp1B1;
# 1192
requires=req;
ensures=ens;
for(0;atts != 0;atts=atts->tl){
# 1196
void*_stmttmp30=(void*)atts->hd;void*_tmp1B2=_stmttmp30;int _tmp1B3;int _tmp1B4;int _tmp1B5;switch(*((int*)_tmp1B2)){case 20U: _LLCA: _tmp1B5=((struct Cyc_Absyn_Initializes_att_Absyn_Attribute_struct*)_tmp1B2)->f1;_LLCB: {int i=_tmp1B5;
# 1198
init_params=({struct Cyc_List_List*_tmp1B6=_cycalloc(sizeof(*_tmp1B6));_tmp1B6->hd=(void*)i,_tmp1B6->tl=init_params;_tmp1B6;});goto _LLC9;}case 21U: _LLCC: _tmp1B4=((struct Cyc_Absyn_Noliveunique_att_Absyn_Attribute_struct*)_tmp1B2)->f1;_LLCD: {int i=_tmp1B4;
# 1200
nolive_unique_params=({struct Cyc_List_List*_tmp1B7=_cycalloc(sizeof(*_tmp1B7));_tmp1B7->hd=(void*)i,_tmp1B7->tl=nolive_unique_params;_tmp1B7;});
consume_params=({struct Cyc_List_List*_tmp1B8=_cycalloc(sizeof(*_tmp1B8));_tmp1B8->hd=(void*)i,_tmp1B8->tl=consume_params;_tmp1B8;});
goto _LLC9;}case 22U: _LLCE: _tmp1B3=((struct Cyc_Absyn_Consume_att_Absyn_Attribute_struct*)_tmp1B2)->f1;_LLCF: {int i=_tmp1B3;
# 1205
consume_params=({struct Cyc_List_List*_tmp1B9=_cycalloc(sizeof(*_tmp1B9));_tmp1B9->hd=(void*)i,_tmp1B9->tl=consume_params;_tmp1B9;});
goto _LLC9;}default: _LLD0: _LLD1:
 goto _LLC9;}_LLC9:;}
# 1209
goto _LLC4;}}else{_LLC7: _LLC8:
({void*_tmp1BA=0U;({int(*_tmp62B)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp1BC)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp1BC;});struct _fat_ptr _tmp62A=({const char*_tmp1BB="anal_Rexp: bad function type";_tag_fat(_tmp1BB,sizeof(char),29U);});_tmp62B(_tmp62A,_tag_fat(_tmp1BA,sizeof(void*),0U));});});}_LLC4:;}
# 1212
goto _LLBF;}}else{_LLC2: _LLC3:
({void*_tmp1BD=0U;({int(*_tmp62D)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp1BF)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp1BF;});struct _fat_ptr _tmp62C=({const char*_tmp1BE="anal_Rexp: bad function type";_tag_fat(_tmp1BE,sizeof(char),29U);});_tmp62D(_tmp62C,_tag_fat(_tmp1BD,sizeof(void*),0U));});});}_LLBF:;}
# 1215
{int i=1;for(0;rvals != 0;(rvals=rvals->tl,es=((struct Cyc_List_List*)_check_null(es))->tl,++ i)){
if(({({int(*_tmp62F)(struct Cyc_List_List*l,int x)=({int(*_tmp1C0)(struct Cyc_List_List*l,int x)=(int(*)(struct Cyc_List_List*l,int x))Cyc_List_memq;_tmp1C0;});struct Cyc_List_List*_tmp62E=init_params;_tmp62F(_tmp62E,i);});})){
union Cyc_CfFlowInfo_FlowInfo _tmp1C1=fst_outflow;struct Cyc_Dict_Dict _tmp1C2;if((_tmp1C1.BottomFL).tag == 1){_LLD3: _LLD4:
 goto _LLD2;}else{_LLD5: _tmp1C2=((_tmp1C1.ReachableFL).val).f1;_LLD6: {struct Cyc_Dict_Dict fst_d=_tmp1C2;
# 1220
struct _tuple16 _stmttmp31=({Cyc_CfFlowInfo_unname_rval((void*)rvals->hd);});void*_tmp1C3;_LLD8: _tmp1C3=_stmttmp31.f1;_LLD9: {void*r=_tmp1C3;
if((int)({Cyc_CfFlowInfo_initlevel(env->fenv,fst_d,(void*)rvals->hd);})== (int)Cyc_CfFlowInfo_NoneIL && !({Cyc_CfFlowInfo_is_init_pointer((void*)rvals->hd);}))
({void*_tmp1C4=0U;({unsigned _tmp631=((struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(es))->hd)->loc;struct _fat_ptr _tmp630=({const char*_tmp1C5="expression may not be initialized";_tag_fat(_tmp1C5,sizeof(char),34U);});Cyc_CfFlowInfo_aerr(_tmp631,_tmp630,_tag_fat(_tmp1C4,sizeof(void*),0U));});});
# 1221
{union Cyc_CfFlowInfo_FlowInfo _tmp1C6=outflow;struct Cyc_List_List*_tmp1C8;struct Cyc_Dict_Dict _tmp1C7;if((_tmp1C6.BottomFL).tag == 1){_LLDB: _LLDC:
# 1224
 goto _LLDA;}else{_LLDD: _tmp1C7=((_tmp1C6.ReachableFL).val).f1;_tmp1C8=((_tmp1C6.ReachableFL).val).f2;_LLDE: {struct Cyc_Dict_Dict d=_tmp1C7;struct Cyc_List_List*relns=_tmp1C8;
# 1228
struct Cyc_Dict_Dict ans_d=({Cyc_CfFlowInfo_escape_deref(fenv,d,(void*)rvals->hd);});
{void*_stmttmp32=(void*)rvals->hd;void*_tmp1C9=_stmttmp32;struct Cyc_CfFlowInfo_Place*_tmp1CA;switch(*((int*)_tmp1C9)){case 4U: _LLE0: _tmp1CA=((struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct*)_tmp1C9)->f1;_LLE1: {struct Cyc_CfFlowInfo_Place*p=_tmp1CA;
# 1231
{void*_stmttmp33=({Cyc_Tcutil_compress((void*)_check_null(((struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(es))->hd)->topt));});void*_tmp1CB=_stmttmp33;void*_tmp1CC;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp1CB)->tag == 3U){_LLE7: _tmp1CC=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp1CB)->f1).elt_type;_LLE8: {void*t=_tmp1CC;
# 1233
ans_d=({struct Cyc_Dict_Dict(*_tmp1CD)(struct Cyc_CfFlowInfo_FlowEnv*fenv,unsigned loc,struct Cyc_Dict_Dict d,struct Cyc_CfFlowInfo_Place*place,void*r)=Cyc_CfFlowInfo_assign_place;struct Cyc_CfFlowInfo_FlowEnv*_tmp1CE=fenv;unsigned _tmp1CF=((struct Cyc_Absyn_Exp*)es->hd)->loc;struct Cyc_Dict_Dict _tmp1D0=ans_d;struct Cyc_CfFlowInfo_Place*_tmp1D1=p;void*_tmp1D2=({Cyc_CfFlowInfo_typ_to_absrval(fenv,t,0,fenv->esc_all);});_tmp1CD(_tmp1CE,_tmp1CF,_tmp1D0,_tmp1D1,_tmp1D2);});
# 1237
goto _LLE6;}}else{_LLE9: _LLEA:
({void*_tmp1D3=0U;({int(*_tmp633)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp1D5)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp1D5;});struct _fat_ptr _tmp632=({const char*_tmp1D4="anal_Rexp:bad type for initialized arg";_tag_fat(_tmp1D4,sizeof(char),39U);});_tmp633(_tmp632,_tag_fat(_tmp1D3,sizeof(void*),0U));});});}_LLE6:;}
# 1240
goto _LLDF;}case 5U: _LLE2: _LLE3:
# 1242
({void*_tmp1D6=0U;({int(*_tmp635)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp1D8)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp1D8;});struct _fat_ptr _tmp634=({const char*_tmp1D7="anal_Rexp:initialize attribute on unique pointers not yet supported";_tag_fat(_tmp1D7,sizeof(char),68U);});_tmp635(_tmp634,_tag_fat(_tmp1D6,sizeof(void*),0U));});});default: _LLE4: _LLE5:
# 1244
 goto _LLDF;}_LLDF:;}
# 1246
outflow=({Cyc_CfFlowInfo_ReachableFL(ans_d,relns);});
goto _LLDA;}}_LLDA:;}
# 1249
goto _LLD2;}}}_LLD2:;}else{
# 1252
if(({({int(*_tmp637)(struct Cyc_List_List*l,int x)=({int(*_tmp1D9)(struct Cyc_List_List*l,int x)=(int(*)(struct Cyc_List_List*l,int x))Cyc_List_memq;_tmp1D9;});struct Cyc_List_List*_tmp636=nolive_unique_params;_tmp637(_tmp636,i);});}))
# 1257
outflow=({({struct Cyc_NewControlFlow_AnalEnv*_tmp63B=env;unsigned _tmp63A=((struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(es))->hd)->loc;void*_tmp639=(void*)_check_null(((struct Cyc_Absyn_Exp*)es->hd)->topt);union Cyc_CfFlowInfo_FlowInfo _tmp638=outflow;Cyc_NewControlFlow_use_nounique_Rval(_tmp63B,_tmp63A,_tmp639,_tmp638,(void*)rvals->hd);});});else{
# 1267
outflow=({Cyc_NewControlFlow_use_Rval(env,((struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(es))->hd)->loc,outflow,(void*)rvals->hd);});
if(!({({int(*_tmp63D)(struct Cyc_List_List*l,int x)=({int(*_tmp1DA)(struct Cyc_List_List*l,int x)=(int(*)(struct Cyc_List_List*l,int x))Cyc_List_memq;_tmp1DA;});struct Cyc_List_List*_tmp63C=consume_params;_tmp63D(_tmp63C,i);});})&&({Cyc_Tcutil_is_noalias_pointer((void*)_check_null(((struct Cyc_Absyn_Exp*)es->hd)->topt),0);}))
# 1270
outflow=({Cyc_NewControlFlow_restore_noconsume_arg(env,outflow,(struct Cyc_Absyn_Exp*)es->hd,((struct Cyc_Absyn_Exp*)es->hd)->loc,(void*)rvals->hd);});}}}}
# 1274
({Cyc_NewControlFlow_check_fn_requires(env,outflow,orig_es,requires,e->loc);});
# 1277
if(({Cyc_Tcutil_is_noreturn_fn_type((void*)_check_null(e1->topt));}))
return({struct _tuple17 _tmp54D;({union Cyc_CfFlowInfo_FlowInfo _tmp63F=({Cyc_CfFlowInfo_BottomFL();});_tmp54D.f1=_tmp63F;}),({void*_tmp63E=({Cyc_CfFlowInfo_typ_to_absrval(fenv,(void*)_check_null(e->topt),0,fenv->unknown_all);});_tmp54D.f2=_tmp63E;});_tmp54D;});else{
# 1280
return({struct _tuple17 _tmp54E;_tmp54E.f1=outflow,({void*_tmp640=({Cyc_CfFlowInfo_typ_to_absrval(fenv,(void*)_check_null(e->topt),0,fenv->unknown_all);});_tmp54E.f2=_tmp640;});_tmp54E;});}}}}}case 35U: _LL4E: _tmp14F=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp12B)->f1).is_calloc;_tmp150=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp12B)->f1).rgn;_tmp151=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp12B)->f1).elt_type;_tmp152=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp12B)->f1).num_elts;_tmp153=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp12B)->f1).fat_result;_LL4F: {int iscalloc=_tmp14F;struct Cyc_Absyn_Exp*eopt=_tmp150;void**topt=_tmp151;struct Cyc_Absyn_Exp*exp=_tmp152;int isfat=_tmp153;
# 1283
void*place_val;
if(isfat)place_val=fenv->notzeroall;else{
if(iscalloc)place_val=({Cyc_CfFlowInfo_typ_to_absrval(fenv,*((void**)_check_null(topt)),0,fenv->zero);});else{
place_val=({Cyc_CfFlowInfo_typ_to_absrval(fenv,*((void**)_check_null(topt)),0,fenv->unknown_none);});}}{
union Cyc_CfFlowInfo_FlowInfo outflow;
if(eopt != 0){
struct _tuple24 _stmttmp34=({struct _tuple24(*_tmp1E3)(struct Cyc_NewControlFlow_AnalEnv*env,union Cyc_CfFlowInfo_FlowInfo inflow,struct Cyc_List_List*es,int first_is_copy,int others_are_copy)=Cyc_NewControlFlow_anal_Rexps;struct Cyc_NewControlFlow_AnalEnv*_tmp1E4=env;union Cyc_CfFlowInfo_FlowInfo _tmp1E5=inflow;struct Cyc_List_List*_tmp1E6=({struct Cyc_Absyn_Exp*_tmp1E7[2U];_tmp1E7[0]=eopt,_tmp1E7[1]=exp;Cyc_List_list(_tag_fat(_tmp1E7,sizeof(struct Cyc_Absyn_Exp*),2U));});int _tmp1E8=1;int _tmp1E9=1;_tmp1E3(_tmp1E4,_tmp1E5,_tmp1E6,_tmp1E8,_tmp1E9);});struct Cyc_List_List*_tmp1E2;union Cyc_CfFlowInfo_FlowInfo _tmp1E1;_LLEC: _tmp1E1=_stmttmp34.f1;_tmp1E2=_stmttmp34.f2;_LLED: {union Cyc_CfFlowInfo_FlowInfo f=_tmp1E1;struct Cyc_List_List*rvals=_tmp1E2;
outflow=f;}}else{
# 1292
struct _tuple17 _stmttmp35=({Cyc_NewControlFlow_anal_Rexp(env,1,inflow,exp);});union Cyc_CfFlowInfo_FlowInfo _tmp1EA;_LLEF: _tmp1EA=_stmttmp35.f1;_LLF0: {union Cyc_CfFlowInfo_FlowInfo f=_tmp1EA;
outflow=f;}}
# 1297
if(({Cyc_Tcutil_is_noalias_pointer((void*)_check_null(e->topt),1);}))
return({struct _tuple17 _tmp54F;_tmp54F.f1=outflow,({void*_tmp641=(void*)({struct Cyc_CfFlowInfo_UniquePtr_CfFlowInfo_AbsRVal_struct*_tmp1EB=_cycalloc(sizeof(*_tmp1EB));_tmp1EB->tag=5U,_tmp1EB->f1=place_val;_tmp1EB;});_tmp54F.f2=_tmp641;});_tmp54F;});else{
# 1301
void*root=(void*)({struct Cyc_CfFlowInfo_MallocPt_CfFlowInfo_Root_struct*_tmp1F4=_cycalloc(sizeof(*_tmp1F4));_tmp1F4->tag=1U,_tmp1F4->f1=exp,_tmp1F4->f2=(void*)_check_null(e->topt);_tmp1F4;});
struct Cyc_CfFlowInfo_Place*place=({struct Cyc_CfFlowInfo_Place*_tmp1F3=_cycalloc(sizeof(*_tmp1F3));_tmp1F3->root=root,_tmp1F3->path=0;_tmp1F3;});
void*rval=(void*)({struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct*_tmp1F2=_cycalloc(sizeof(*_tmp1F2));_tmp1F2->tag=4U,_tmp1F2->f1=place;_tmp1F2;});
union Cyc_CfFlowInfo_FlowInfo _tmp1EC=outflow;struct Cyc_List_List*_tmp1EE;struct Cyc_Dict_Dict _tmp1ED;if((_tmp1EC.BottomFL).tag == 1){_LLF2: _LLF3:
 return({struct _tuple17 _tmp550;_tmp550.f1=outflow,_tmp550.f2=rval;_tmp550;});}else{_LLF4: _tmp1ED=((_tmp1EC.ReachableFL).val).f1;_tmp1EE=((_tmp1EC.ReachableFL).val).f2;_LLF5: {struct Cyc_Dict_Dict d2=_tmp1ED;struct Cyc_List_List*relns=_tmp1EE;
# 1307
return({struct _tuple17 _tmp551;({union Cyc_CfFlowInfo_FlowInfo _tmp642=({union Cyc_CfFlowInfo_FlowInfo(*_tmp1EF)(struct Cyc_Dict_Dict,struct Cyc_List_List*)=Cyc_CfFlowInfo_ReachableFL;struct Cyc_Dict_Dict _tmp1F0=({Cyc_Dict_insert(d2,root,place_val);});struct Cyc_List_List*_tmp1F1=relns;_tmp1EF(_tmp1F0,_tmp1F1);});_tmp551.f1=_tmp642;}),_tmp551.f2=rval;_tmp551;});}}_LLF1:;}}}case 36U: _LL50: _tmp14D=((struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct*)_tmp12B)->f1;_tmp14E=((struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct*)_tmp12B)->f2;_LL51: {struct Cyc_Absyn_Exp*e1=_tmp14D;struct Cyc_Absyn_Exp*e2=_tmp14E;
# 1312
void*left_rval;
void*right_rval;
union Cyc_CfFlowInfo_FlowInfo outflow;
# 1318
struct _tuple24 _stmttmp36=({struct _tuple24(*_tmp20A)(struct Cyc_NewControlFlow_AnalEnv*env,union Cyc_CfFlowInfo_FlowInfo inflow,struct Cyc_List_List*es,int first_is_copy,int others_are_copy)=Cyc_NewControlFlow_anal_Rexps;struct Cyc_NewControlFlow_AnalEnv*_tmp20B=env;union Cyc_CfFlowInfo_FlowInfo _tmp20C=inflow;struct Cyc_List_List*_tmp20D=({struct Cyc_Absyn_Exp*_tmp20E[2U];_tmp20E[0]=e1,_tmp20E[1]=e2;Cyc_List_list(_tag_fat(_tmp20E,sizeof(struct Cyc_Absyn_Exp*),2U));});int _tmp20F=0;int _tmp210=0;_tmp20A(_tmp20B,_tmp20C,_tmp20D,_tmp20F,_tmp210);});struct Cyc_List_List*_tmp1F6;union Cyc_CfFlowInfo_FlowInfo _tmp1F5;_LLF7: _tmp1F5=_stmttmp36.f1;_tmp1F6=_stmttmp36.f2;_LLF8: {union Cyc_CfFlowInfo_FlowInfo f=_tmp1F5;struct Cyc_List_List*rvals=_tmp1F6;
left_rval=(void*)((struct Cyc_List_List*)_check_null(rvals))->hd;
right_rval=(void*)((struct Cyc_List_List*)_check_null(rvals->tl))->hd;
outflow=f;{
# 1324
void*t_ign1=Cyc_Absyn_void_type;void*t_ign2=Cyc_Absyn_void_type;
int b_ign1=0;
if(({Cyc_Tcutil_is_zero_ptr_deref(e1,& t_ign1,& b_ign1,& t_ign2);})){
void*_tmp1F7=right_rval;if(((struct Cyc_CfFlowInfo_Zero_CfFlowInfo_AbsRVal_struct*)_tmp1F7)->tag == 0U){_LLFA: _LLFB:
 goto _LLF9;}else{_LLFC: _LLFD:
({void*_tmp1F8=0U;({unsigned _tmp644=e1->loc;struct _fat_ptr _tmp643=({const char*_tmp1F9="cannot swap value into zeroterm array not known to be 0";_tag_fat(_tmp1F9,sizeof(char),56U);});Cyc_CfFlowInfo_aerr(_tmp644,_tmp643,_tag_fat(_tmp1F8,sizeof(void*),0U));});});}_LLF9:;}else{
# 1335
if(({Cyc_Tcutil_is_zero_ptr_deref(e2,& t_ign1,& b_ign1,& t_ign2);})){
void*_tmp1FA=left_rval;if(((struct Cyc_CfFlowInfo_Zero_CfFlowInfo_AbsRVal_struct*)_tmp1FA)->tag == 0U){_LLFF: _LL100:
 goto _LLFE;}else{_LL101: _LL102:
({void*_tmp1FB=0U;({unsigned _tmp646=e2->loc;struct _fat_ptr _tmp645=({const char*_tmp1FC="cannot swap value into zeroterm array not known to be 0";_tag_fat(_tmp1FC,sizeof(char),56U);});Cyc_CfFlowInfo_aerr(_tmp646,_tmp645,_tag_fat(_tmp1FB,sizeof(void*),0U));});});}_LLFE:;}}{
# 1326
struct _tuple19 _stmttmp37=({Cyc_NewControlFlow_anal_Lexp(env,outflow,0,0,e1);});union Cyc_CfFlowInfo_AbsLVal _tmp1FD;_LL104: _tmp1FD=_stmttmp37.f2;_LL105: {union Cyc_CfFlowInfo_AbsLVal left_lval=_tmp1FD;
# 1344
struct _tuple19 _stmttmp38=({Cyc_NewControlFlow_anal_Lexp(env,outflow,0,0,e2);});union Cyc_CfFlowInfo_AbsLVal _tmp1FE;_LL107: _tmp1FE=_stmttmp38.f2;_LL108: {union Cyc_CfFlowInfo_AbsLVal right_lval=_tmp1FE;
{union Cyc_CfFlowInfo_FlowInfo _tmp1FF=outflow;struct Cyc_List_List*_tmp201;struct Cyc_Dict_Dict _tmp200;if((_tmp1FF.ReachableFL).tag == 2){_LL10A: _tmp200=((_tmp1FF.ReachableFL).val).f1;_tmp201=((_tmp1FF.ReachableFL).val).f2;_LL10B: {struct Cyc_Dict_Dict outdict=_tmp200;struct Cyc_List_List*relns=_tmp201;
# 1347
{union Cyc_CfFlowInfo_AbsLVal _tmp202=left_lval;struct Cyc_CfFlowInfo_Place*_tmp203;if((_tmp202.PlaceL).tag == 1){_LL10F: _tmp203=(_tmp202.PlaceL).val;_LL110: {struct Cyc_CfFlowInfo_Place*lp=_tmp203;
# 1349
outdict=({Cyc_CfFlowInfo_assign_place(fenv,e1->loc,outdict,lp,right_rval);});
goto _LL10E;}}else{_LL111: _LL112:
# 1355
 if((int)({Cyc_CfFlowInfo_initlevel(fenv,outdict,right_rval);})!= (int)Cyc_CfFlowInfo_AllIL)
({void*_tmp204=0U;({unsigned _tmp648=e2->loc;struct _fat_ptr _tmp647=({const char*_tmp205="expression may not be fully initialized";_tag_fat(_tmp205,sizeof(char),40U);});Cyc_CfFlowInfo_aerr(_tmp648,_tmp647,_tag_fat(_tmp204,sizeof(void*),0U));});});
# 1355
goto _LL10E;}_LL10E:;}
# 1359
{union Cyc_CfFlowInfo_AbsLVal _tmp206=right_lval;struct Cyc_CfFlowInfo_Place*_tmp207;if((_tmp206.PlaceL).tag == 1){_LL114: _tmp207=(_tmp206.PlaceL).val;_LL115: {struct Cyc_CfFlowInfo_Place*rp=_tmp207;
# 1361
outdict=({Cyc_CfFlowInfo_assign_place(fenv,e2->loc,outdict,rp,left_rval);});
goto _LL113;}}else{_LL116: _LL117:
# 1364
 if((int)({Cyc_CfFlowInfo_initlevel(fenv,outdict,left_rval);})!= (int)Cyc_CfFlowInfo_AllIL)
({void*_tmp208=0U;({unsigned _tmp64A=e1->loc;struct _fat_ptr _tmp649=({const char*_tmp209="expression may not be fully initialized";_tag_fat(_tmp209,sizeof(char),40U);});Cyc_CfFlowInfo_aerr(_tmp64A,_tmp649,_tag_fat(_tmp208,sizeof(void*),0U));});});
# 1364
goto _LL113;}_LL113:;}
# 1369
relns=({Cyc_Relations_reln_kill_exp(Cyc_Core_heap_region,relns,e1);});
relns=({Cyc_Relations_reln_kill_exp(Cyc_Core_heap_region,relns,e2);});
# 1372
outflow=({Cyc_CfFlowInfo_ReachableFL(outdict,relns);});
({Cyc_NewControlFlow_update_tryflow(env,outflow);});
goto _LL109;}}else{_LL10C: _LL10D:
 goto _LL109;}_LL109:;}
# 1379
return({struct _tuple17 _tmp552;_tmp552.f1=outflow,_tmp552.f2=fenv->unknown_all;_tmp552;});}}}}}}case 16U: _LL52: _tmp14B=((struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct*)_tmp12B)->f1;_tmp14C=((struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct*)_tmp12B)->f2;_LL53: {struct Cyc_Absyn_Exp*eopt=_tmp14B;struct Cyc_Absyn_Exp*e2=_tmp14C;
# 1382
union Cyc_CfFlowInfo_FlowInfo outflow;
void*place_val;
if(eopt != 0){
struct _tuple24 _stmttmp39=({struct _tuple24(*_tmp213)(struct Cyc_NewControlFlow_AnalEnv*env,union Cyc_CfFlowInfo_FlowInfo inflow,struct Cyc_List_List*es,int first_is_copy,int others_are_copy)=Cyc_NewControlFlow_anal_Rexps;struct Cyc_NewControlFlow_AnalEnv*_tmp214=env;union Cyc_CfFlowInfo_FlowInfo _tmp215=inflow;struct Cyc_List_List*_tmp216=({struct Cyc_Absyn_Exp*_tmp217[2U];_tmp217[0]=eopt,_tmp217[1]=e2;Cyc_List_list(_tag_fat(_tmp217,sizeof(struct Cyc_Absyn_Exp*),2U));});int _tmp218=1;int _tmp219=1;_tmp213(_tmp214,_tmp215,_tmp216,_tmp218,_tmp219);});struct Cyc_List_List*_tmp212;union Cyc_CfFlowInfo_FlowInfo _tmp211;_LL119: _tmp211=_stmttmp39.f1;_tmp212=_stmttmp39.f2;_LL11A: {union Cyc_CfFlowInfo_FlowInfo f=_tmp211;struct Cyc_List_List*rvals=_tmp212;
outflow=f;
place_val=(void*)((struct Cyc_List_List*)_check_null(((struct Cyc_List_List*)_check_null(rvals))->tl))->hd;}}else{
# 1389
struct _tuple17 _stmttmp3A=({Cyc_NewControlFlow_anal_Rexp(env,1,inflow,e2);});void*_tmp21B;union Cyc_CfFlowInfo_FlowInfo _tmp21A;_LL11C: _tmp21A=_stmttmp3A.f1;_tmp21B=_stmttmp3A.f2;_LL11D: {union Cyc_CfFlowInfo_FlowInfo f=_tmp21A;void*r=_tmp21B;
outflow=f;
place_val=r;}}
# 1394
if(({Cyc_Tcutil_is_noalias_pointer((void*)_check_null(e->topt),1);}))
return({struct _tuple17 _tmp553;_tmp553.f1=outflow,({void*_tmp64B=(void*)({struct Cyc_CfFlowInfo_UniquePtr_CfFlowInfo_AbsRVal_struct*_tmp21C=_cycalloc(sizeof(*_tmp21C));_tmp21C->tag=5U,_tmp21C->f1=place_val;_tmp21C;});_tmp553.f2=_tmp64B;});_tmp553;});else{
# 1398
void*root=(void*)({struct Cyc_CfFlowInfo_MallocPt_CfFlowInfo_Root_struct*_tmp225=_cycalloc(sizeof(*_tmp225));_tmp225->tag=1U,_tmp225->f1=e2,_tmp225->f2=(void*)_check_null(e->topt);_tmp225;});
struct Cyc_CfFlowInfo_Place*place=({struct Cyc_CfFlowInfo_Place*_tmp224=_cycalloc(sizeof(*_tmp224));_tmp224->root=root,_tmp224->path=0;_tmp224;});
void*rval=(void*)({struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct*_tmp223=_cycalloc(sizeof(*_tmp223));_tmp223->tag=4U,_tmp223->f1=place;_tmp223;});
union Cyc_CfFlowInfo_FlowInfo _tmp21D=outflow;struct Cyc_List_List*_tmp21F;struct Cyc_Dict_Dict _tmp21E;if((_tmp21D.BottomFL).tag == 1){_LL11F: _LL120:
 return({struct _tuple17 _tmp554;_tmp554.f1=outflow,_tmp554.f2=rval;_tmp554;});}else{_LL121: _tmp21E=((_tmp21D.ReachableFL).val).f1;_tmp21F=((_tmp21D.ReachableFL).val).f2;_LL122: {struct Cyc_Dict_Dict d2=_tmp21E;struct Cyc_List_List*relns=_tmp21F;
# 1404
return({struct _tuple17 _tmp555;({union Cyc_CfFlowInfo_FlowInfo _tmp64C=({union Cyc_CfFlowInfo_FlowInfo(*_tmp220)(struct Cyc_Dict_Dict,struct Cyc_List_List*)=Cyc_CfFlowInfo_ReachableFL;struct Cyc_Dict_Dict _tmp221=({Cyc_Dict_insert(d2,root,place_val);});struct Cyc_List_List*_tmp222=relns;_tmp220(_tmp221,_tmp222);});_tmp555.f1=_tmp64C;}),_tmp555.f2=rval;_tmp555;});}}_LL11E:;}}case 15U: _LL54: _tmp14A=((struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct*)_tmp12B)->f1;_LL55: {struct Cyc_Absyn_Exp*e1=_tmp14A;
# 1409
struct _tuple19 _stmttmp3B=({Cyc_NewControlFlow_anal_Lexp(env,inflow,0,0,e1);});union Cyc_CfFlowInfo_AbsLVal _tmp227;union Cyc_CfFlowInfo_FlowInfo _tmp226;_LL124: _tmp226=_stmttmp3B.f1;_tmp227=_stmttmp3B.f2;_LL125: {union Cyc_CfFlowInfo_FlowInfo f=_tmp226;union Cyc_CfFlowInfo_AbsLVal l=_tmp227;
union Cyc_CfFlowInfo_AbsLVal _tmp228=l;struct Cyc_CfFlowInfo_Place*_tmp229;if((_tmp228.UnknownL).tag == 2){_LL127: _LL128:
 return({struct _tuple17 _tmp556;_tmp556.f1=f,_tmp556.f2=fenv->notzeroall;_tmp556;});}else{_LL129: _tmp229=(_tmp228.PlaceL).val;_LL12A: {struct Cyc_CfFlowInfo_Place*p=_tmp229;
return({struct _tuple17 _tmp557;_tmp557.f1=f,({void*_tmp64D=(void*)({struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct*_tmp22A=_cycalloc(sizeof(*_tmp22A));_tmp22A->tag=4U,_tmp22A->f1=p;_tmp22A;});_tmp557.f2=_tmp64D;});_tmp557;});}}_LL126:;}}case 20U: _LL56: _tmp149=((struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_tmp12B)->f1;_LL57: {struct Cyc_Absyn_Exp*e1=_tmp149;
# 1416
struct _tuple17 _stmttmp3C=({Cyc_NewControlFlow_anal_Rexp(env,0,inflow,e1);});void*_tmp22C;union Cyc_CfFlowInfo_FlowInfo _tmp22B;_LL12C: _tmp22B=_stmttmp3C.f1;_tmp22C=_stmttmp3C.f2;_LL12D: {union Cyc_CfFlowInfo_FlowInfo f=_tmp22B;void*r=_tmp22C;
return({Cyc_NewControlFlow_anal_derefR(env,inflow,f,e1,r,0);});}}case 21U: _LL58: _tmp147=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp12B)->f1;_tmp148=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp12B)->f2;_LL59: {struct Cyc_Absyn_Exp*e1=_tmp147;struct _fat_ptr*field=_tmp148;
# 1420
struct _tuple17 _stmttmp3D=({Cyc_NewControlFlow_anal_Rexp(env,0,inflow,e1);});void*_tmp22E;union Cyc_CfFlowInfo_FlowInfo _tmp22D;_LL12F: _tmp22D=_stmttmp3D.f1;_tmp22E=_stmttmp3D.f2;_LL130: {union Cyc_CfFlowInfo_FlowInfo f=_tmp22D;void*r=_tmp22E;
void*e1_type=(void*)_check_null(e1->topt);
# 1423
if(({Cyc_Absyn_is_nontagged_nonrequire_union_type(e1_type);}))
return({struct _tuple17 _tmp558;_tmp558.f1=f,({void*_tmp64E=({Cyc_CfFlowInfo_typ_to_absrval(fenv,(void*)_check_null(e->topt),0,fenv->unknown_all);});_tmp558.f2=_tmp64E;});_tmp558;});
# 1423
if(({Cyc_Absyn_is_require_union_type(e1_type);}))
# 1426
({Cyc_NewControlFlow_check_union_requires(e1->loc,e1_type,field,f);});{
# 1423
struct _tuple16 _stmttmp3E=({Cyc_CfFlowInfo_unname_rval(r);});void*_tmp22F;_LL132: _tmp22F=_stmttmp3E.f1;_LL133: {void*r=_tmp22F;
# 1428
void*_tmp230=r;struct _fat_ptr _tmp233;int _tmp232;int _tmp231;if(((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp230)->tag == 6U){_LL135: _tmp231=(((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp230)->f1).is_union;_tmp232=(((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp230)->f1).fieldnum;_tmp233=((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp230)->f2;_LL136: {int is_union=_tmp231;int fnum=_tmp232;struct _fat_ptr rdict=_tmp233;
# 1430
int field_no=({Cyc_CfFlowInfo_get_field_index((void*)_check_null(e1->topt),field);});
if((is_union && fnum != -1)&& fnum != field_no)
return({struct _tuple17 _tmp559;_tmp559.f1=f,({void*_tmp64F=({Cyc_CfFlowInfo_typ_to_absrval(fenv,(void*)_check_null(e->topt),1,fenv->unknown_none);});_tmp559.f2=_tmp64F;});_tmp559;});
# 1431
return({struct _tuple17 _tmp55A;_tmp55A.f1=f,_tmp55A.f2=*((void**)_check_fat_subscript(rdict,sizeof(void*),field_no));_tmp55A;});}}else{_LL137: _LL138:
# 1435
({void*_tmp234=0U;({struct Cyc___cycFILE*_tmp651=Cyc_stderr;struct _fat_ptr _tmp650=({const char*_tmp235="the bad rexp is :";_tag_fat(_tmp235,sizeof(char),18U);});Cyc_fprintf(_tmp651,_tmp650,_tag_fat(_tmp234,sizeof(void*),0U));});});
({Cyc_CfFlowInfo_print_absrval(r);});
({struct Cyc_String_pa_PrintArg_struct _tmp239=({struct Cyc_String_pa_PrintArg_struct _tmp55B;_tmp55B.tag=0U,({struct _fat_ptr _tmp652=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_exp2string(e);}));_tmp55B.f1=_tmp652;});_tmp55B;});void*_tmp236[1U];_tmp236[0]=& _tmp239;({int(*_tmp654)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp238)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp238;});struct _fat_ptr _tmp653=({const char*_tmp237="\nanal_Rexp: AggrMember: %s";_tag_fat(_tmp237,sizeof(char),27U);});_tmp654(_tmp653,_tag_fat(_tmp236,sizeof(void*),1U));});});}_LL134:;}}}}case 39U: _LL5A: _tmp145=((struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct*)_tmp12B)->f1;_tmp146=((struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct*)_tmp12B)->f2;_LL5B: {struct Cyc_Absyn_Exp*e1=_tmp145;struct _fat_ptr*field=_tmp146;
# 1443
struct _tuple17 _stmttmp3F=({Cyc_NewControlFlow_anal_Rexp(env,0,inflow,e1);});void*_tmp23B;union Cyc_CfFlowInfo_FlowInfo _tmp23A;_LL13A: _tmp23A=_stmttmp3F.f1;_tmp23B=_stmttmp3F.f2;_LL13B: {union Cyc_CfFlowInfo_FlowInfo f=_tmp23A;void*r=_tmp23B;
# 1445
if(({Cyc_Absyn_is_nontagged_nonrequire_union_type((void*)_check_null(e1->topt));}))
return({struct _tuple17 _tmp55C;_tmp55C.f1=f,_tmp55C.f2=fenv->unknown_all;_tmp55C;});{
# 1445
struct _tuple16 _stmttmp40=({Cyc_CfFlowInfo_unname_rval(r);});void*_tmp23C;_LL13D: _tmp23C=_stmttmp40.f1;_LL13E: {void*r=_tmp23C;
# 1448
void*_tmp23D=r;struct _fat_ptr _tmp240;int _tmp23F;int _tmp23E;if(((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp23D)->tag == 6U){_LL140: _tmp23E=(((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp23D)->f1).is_union;_tmp23F=(((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp23D)->f1).fieldnum;_tmp240=((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp23D)->f2;_LL141: {int is_union=_tmp23E;int fnum=_tmp23F;struct _fat_ptr rdict=_tmp240;
# 1450
int field_no=({Cyc_CfFlowInfo_get_field_index((void*)_check_null(e1->topt),field);});
if(is_union && fnum != -1){
if(fnum != field_no)
return({struct _tuple17 _tmp55D;_tmp55D.f1=f,_tmp55D.f2=fenv->zero;_tmp55D;});else{
# 1455
return({struct _tuple17 _tmp55E;_tmp55E.f1=f,_tmp55E.f2=fenv->notzeroall;_tmp55E;});}}else{
# 1457
return({struct _tuple17 _tmp55F;_tmp55F.f1=f,_tmp55F.f2=fenv->unknown_all;_tmp55F;});}}}else{_LL142: _LL143:
# 1459
({struct Cyc_String_pa_PrintArg_struct _tmp244=({struct Cyc_String_pa_PrintArg_struct _tmp560;_tmp560.tag=0U,({struct _fat_ptr _tmp655=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_exp2string(e);}));_tmp560.f1=_tmp655;});_tmp560;});void*_tmp241[1U];_tmp241[0]=& _tmp244;({int(*_tmp657)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp243)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp243;});struct _fat_ptr _tmp656=({const char*_tmp242="anal_Rexp: TagCheck_e: %s";_tag_fat(_tmp242,sizeof(char),26U);});_tmp657(_tmp656,_tag_fat(_tmp241,sizeof(void*),1U));});});}_LL13F:;}}}}case 22U: _LL5C: _tmp143=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp12B)->f1;_tmp144=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp12B)->f2;_LL5D: {struct Cyc_Absyn_Exp*e1=_tmp143;struct _fat_ptr*field=_tmp144;
# 1463
struct _tuple17 _stmttmp41=({Cyc_NewControlFlow_anal_Rexp(env,0,inflow,e1);});void*_tmp246;union Cyc_CfFlowInfo_FlowInfo _tmp245;_LL145: _tmp245=_stmttmp41.f1;_tmp246=_stmttmp41.f2;_LL146: {union Cyc_CfFlowInfo_FlowInfo f1=_tmp245;void*r1=_tmp246;
# 1466
struct _tuple17 _stmttmp42=({Cyc_NewControlFlow_anal_derefR(env,inflow,f1,e1,r1,0);});void*_tmp248;union Cyc_CfFlowInfo_FlowInfo _tmp247;_LL148: _tmp247=_stmttmp42.f1;_tmp248=_stmttmp42.f2;_LL149: {union Cyc_CfFlowInfo_FlowInfo f2=_tmp247;void*r2=_tmp248;
# 1469
void*_stmttmp43=({Cyc_Tcutil_compress((void*)_check_null(e1->topt));});void*_tmp249=_stmttmp43;void*_tmp24A;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp249)->tag == 3U){_LL14B: _tmp24A=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp249)->f1).elt_type;_LL14C: {void*t2=_tmp24A;
# 1471
if(({Cyc_Absyn_is_nontagged_nonrequire_union_type(t2);}))
# 1473
return({struct _tuple17 _tmp561;_tmp561.f1=f2,({void*_tmp658=({Cyc_CfFlowInfo_typ_to_absrval(fenv,(void*)_check_null(e->topt),0,fenv->unknown_all);});_tmp561.f2=_tmp658;});_tmp561;});
# 1471
if(({Cyc_Absyn_is_require_union_type(t2);}))
# 1476
({Cyc_NewControlFlow_check_union_requires(e1->loc,t2,field,f1);});{
# 1471
struct _tuple16 _stmttmp44=({Cyc_CfFlowInfo_unname_rval(r2);});void*_tmp24B;_LL150: _tmp24B=_stmttmp44.f1;_LL151: {void*r2=_tmp24B;
# 1479
void*_tmp24C=r2;struct _fat_ptr _tmp24F;int _tmp24E;int _tmp24D;if(((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp24C)->tag == 6U){_LL153: _tmp24D=(((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp24C)->f1).is_union;_tmp24E=(((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp24C)->f1).fieldnum;_tmp24F=((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp24C)->f2;_LL154: {int is_union=_tmp24D;int fnum=_tmp24E;struct _fat_ptr rdict=_tmp24F;
# 1481
int field_no=({Cyc_CfFlowInfo_get_field_index(t2,field);});
if((is_union && fnum != -1)&& fnum != field_no)
return({struct _tuple17 _tmp562;_tmp562.f1=f2,({void*_tmp659=({Cyc_CfFlowInfo_typ_to_absrval(fenv,(void*)_check_null(e->topt),1,fenv->unknown_none);});_tmp562.f2=_tmp659;});_tmp562;});
# 1482
return({struct _tuple17 _tmp563;_tmp563.f1=f2,_tmp563.f2=*((void**)_check_fat_subscript(rdict,sizeof(void*),field_no));_tmp563;});}}else{_LL155: _LL156:
# 1485
({void*_tmp250=0U;({int(*_tmp65B)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp252)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp252;});struct _fat_ptr _tmp65A=({const char*_tmp251="anal_Rexp: AggrArrow";_tag_fat(_tmp251,sizeof(char),21U);});_tmp65B(_tmp65A,_tag_fat(_tmp250,sizeof(void*),0U));});});}_LL152:;}}}}else{_LL14D: _LL14E:
# 1487
({void*_tmp253=0U;({int(*_tmp65D)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp255)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp255;});struct _fat_ptr _tmp65C=({const char*_tmp254="anal_Rexp: AggrArrow ptr";_tag_fat(_tmp254,sizeof(char),25U);});_tmp65D(_tmp65C,_tag_fat(_tmp253,sizeof(void*),0U));});});}_LL14A:;}}}case 6U: _LL5E: _tmp140=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp12B)->f1;_tmp141=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp12B)->f2;_tmp142=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp12B)->f3;_LL5F: {struct Cyc_Absyn_Exp*e1=_tmp140;struct Cyc_Absyn_Exp*e2=_tmp141;struct Cyc_Absyn_Exp*e3=_tmp142;
# 1491
struct _tuple20 _stmttmp45=({Cyc_NewControlFlow_anal_test(env,inflow,e1);});union Cyc_CfFlowInfo_FlowInfo _tmp257;union Cyc_CfFlowInfo_FlowInfo _tmp256;_LL158: _tmp256=_stmttmp45.f1;_tmp257=_stmttmp45.f2;_LL159: {union Cyc_CfFlowInfo_FlowInfo f1t=_tmp256;union Cyc_CfFlowInfo_FlowInfo f1f=_tmp257;
struct _tuple17 pr2=({Cyc_NewControlFlow_anal_Rexp(env,copy_ctxt,f1t,e2);});
struct _tuple17 pr3=({Cyc_NewControlFlow_anal_Rexp(env,copy_ctxt,f1f,e3);});
return({Cyc_CfFlowInfo_join_flow_and_rval(fenv,pr2,pr3);});}}case 7U: _LL60: _tmp13E=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp12B)->f1;_tmp13F=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp12B)->f2;_LL61: {struct Cyc_Absyn_Exp*e1=_tmp13E;struct Cyc_Absyn_Exp*e2=_tmp13F;
# 1497
struct _tuple20 _stmttmp46=({Cyc_NewControlFlow_anal_test(env,inflow,e1);});union Cyc_CfFlowInfo_FlowInfo _tmp259;union Cyc_CfFlowInfo_FlowInfo _tmp258;_LL15B: _tmp258=_stmttmp46.f1;_tmp259=_stmttmp46.f2;_LL15C: {union Cyc_CfFlowInfo_FlowInfo f1t=_tmp258;union Cyc_CfFlowInfo_FlowInfo f1f=_tmp259;
struct _tuple17 _stmttmp47=({Cyc_NewControlFlow_anal_Rexp(env,copy_ctxt,f1t,e2);});void*_tmp25B;union Cyc_CfFlowInfo_FlowInfo _tmp25A;_LL15E: _tmp25A=_stmttmp47.f1;_tmp25B=_stmttmp47.f2;_LL15F: {union Cyc_CfFlowInfo_FlowInfo f2t=_tmp25A;void*f2r=_tmp25B;
struct _tuple17 pr2=({struct _tuple17 _tmp565;_tmp565.f1=f2t,_tmp565.f2=f2r;_tmp565;});
struct _tuple17 pr3=({struct _tuple17 _tmp564;_tmp564.f1=f1f,_tmp564.f2=fenv->zero;_tmp564;});
return({Cyc_CfFlowInfo_join_flow_and_rval(fenv,pr2,pr3);});}}}case 8U: _LL62: _tmp13C=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp12B)->f1;_tmp13D=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp12B)->f2;_LL63: {struct Cyc_Absyn_Exp*e1=_tmp13C;struct Cyc_Absyn_Exp*e2=_tmp13D;
# 1504
struct _tuple20 _stmttmp48=({Cyc_NewControlFlow_anal_test(env,inflow,e1);});union Cyc_CfFlowInfo_FlowInfo _tmp25D;union Cyc_CfFlowInfo_FlowInfo _tmp25C;_LL161: _tmp25C=_stmttmp48.f1;_tmp25D=_stmttmp48.f2;_LL162: {union Cyc_CfFlowInfo_FlowInfo f1t=_tmp25C;union Cyc_CfFlowInfo_FlowInfo f1f=_tmp25D;
struct _tuple17 _stmttmp49=({Cyc_NewControlFlow_anal_Rexp(env,copy_ctxt,f1f,e2);});void*_tmp25F;union Cyc_CfFlowInfo_FlowInfo _tmp25E;_LL164: _tmp25E=_stmttmp49.f1;_tmp25F=_stmttmp49.f2;_LL165: {union Cyc_CfFlowInfo_FlowInfo f2t=_tmp25E;void*f2r=_tmp25F;
struct _tuple17 pr2=({struct _tuple17 _tmp567;_tmp567.f1=f2t,_tmp567.f2=f2r;_tmp567;});
struct _tuple17 pr3=({struct _tuple17 _tmp566;_tmp566.f1=f1t,_tmp566.f2=fenv->notzeroall;_tmp566;});
return({Cyc_CfFlowInfo_join_flow_and_rval(fenv,pr2,pr3);});}}}case 23U: _LL64: _tmp13A=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp12B)->f1;_tmp13B=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp12B)->f2;_LL65: {struct Cyc_Absyn_Exp*e1=_tmp13A;struct Cyc_Absyn_Exp*e2=_tmp13B;
# 1511
struct _tuple24 _stmttmp4A=({struct _tuple24(*_tmp275)(struct Cyc_NewControlFlow_AnalEnv*env,union Cyc_CfFlowInfo_FlowInfo inflow,struct Cyc_List_List*es,int first_is_copy,int others_are_copy)=Cyc_NewControlFlow_anal_Rexps;struct Cyc_NewControlFlow_AnalEnv*_tmp276=env;union Cyc_CfFlowInfo_FlowInfo _tmp277=inflow;struct Cyc_List_List*_tmp278=({struct Cyc_Absyn_Exp*_tmp279[2U];_tmp279[0]=e1,_tmp279[1]=e2;Cyc_List_list(_tag_fat(_tmp279,sizeof(struct Cyc_Absyn_Exp*),2U));});int _tmp27A=0;int _tmp27B=1;_tmp275(_tmp276,_tmp277,_tmp278,_tmp27A,_tmp27B);});struct Cyc_List_List*_tmp261;union Cyc_CfFlowInfo_FlowInfo _tmp260;_LL167: _tmp260=_stmttmp4A.f1;_tmp261=_stmttmp4A.f2;_LL168: {union Cyc_CfFlowInfo_FlowInfo f=_tmp260;struct Cyc_List_List*rvals=_tmp261;
# 1515
union Cyc_CfFlowInfo_FlowInfo f2=f;
{union Cyc_CfFlowInfo_FlowInfo _tmp262=f;struct Cyc_List_List*_tmp264;struct Cyc_Dict_Dict _tmp263;if((_tmp262.ReachableFL).tag == 2){_LL16A: _tmp263=((_tmp262.ReachableFL).val).f1;_tmp264=((_tmp262.ReachableFL).val).f2;_LL16B: {struct Cyc_Dict_Dict d2=_tmp263;struct Cyc_List_List*relns=_tmp264;
# 1520
if((int)({Cyc_CfFlowInfo_initlevel(env->fenv,d2,(void*)((struct Cyc_List_List*)_check_null(((struct Cyc_List_List*)_check_null(rvals))->tl))->hd);})== (int)Cyc_CfFlowInfo_NoneIL)
({void*_tmp265=0U;({unsigned _tmp65F=e2->loc;struct _fat_ptr _tmp65E=({const char*_tmp266="expression may not be initialized";_tag_fat(_tmp266,sizeof(char),34U);});Cyc_CfFlowInfo_aerr(_tmp65F,_tmp65E,_tag_fat(_tmp265,sizeof(void*),0U));});});{
# 1520
struct Cyc_List_List*new_relns=({Cyc_NewControlFlow_add_subscript_reln(relns,e1,e2);});
# 1523
if(relns != new_relns)
f2=({Cyc_CfFlowInfo_ReachableFL(d2,new_relns);});
# 1523
goto _LL169;}}}else{_LL16C: _LL16D:
# 1526
 goto _LL169;}_LL169:;}{
# 1528
void*_stmttmp4B=({Cyc_Tcutil_compress((void*)_check_null(e1->topt));});void*_tmp267=_stmttmp4B;struct Cyc_List_List*_tmp268;switch(*((int*)_tmp267)){case 6U: _LL16F: _tmp268=((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp267)->f1;_LL170: {struct Cyc_List_List*tqts=_tmp268;
# 1530
struct _tuple16 _stmttmp4C=({Cyc_CfFlowInfo_unname_rval((void*)((struct Cyc_List_List*)_check_null(rvals))->hd);});void*_tmp269;_LL176: _tmp269=_stmttmp4C.f1;_LL177: {void*r=_tmp269;
void*_tmp26A=r;struct _fat_ptr _tmp26B;if(((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp26A)->tag == 6U){_LL179: _tmp26B=((struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*)_tmp26A)->f2;_LL17A: {struct _fat_ptr rdict=_tmp26B;
# 1533
unsigned i=({Cyc_Evexp_eval_const_uint_exp(e2);}).f1;
return({struct _tuple17 _tmp568;_tmp568.f1=f2,_tmp568.f2=*((void**)_check_fat_subscript(rdict,sizeof(void*),(int)i));_tmp568;});}}else{_LL17B: _LL17C:
({void*_tmp26C=0U;({int(*_tmp661)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp26E)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp26E;});struct _fat_ptr _tmp660=({const char*_tmp26D="anal_Rexp: Subscript";_tag_fat(_tmp26D,sizeof(char),21U);});_tmp661(_tmp660,_tag_fat(_tmp26C,sizeof(void*),0U));});});}_LL178:;}}case 3U: _LL171: _LL172: {
# 1538
struct _tuple17 _stmttmp4D=({Cyc_NewControlFlow_anal_derefR(env,inflow,f,e1,(void*)((struct Cyc_List_List*)_check_null(rvals))->hd,e2);});void*_tmp270;union Cyc_CfFlowInfo_FlowInfo _tmp26F;_LL17E: _tmp26F=_stmttmp4D.f1;_tmp270=_stmttmp4D.f2;_LL17F: {union Cyc_CfFlowInfo_FlowInfo f3=_tmp26F;void*r=_tmp270;
union Cyc_CfFlowInfo_FlowInfo _tmp271=f3;if((_tmp271.BottomFL).tag == 1){_LL181: _LL182:
 return({struct _tuple17 _tmp569;_tmp569.f1=f3,_tmp569.f2=r;_tmp569;});}else{_LL183: _LL184:
 return({struct _tuple17 _tmp56A;_tmp56A.f1=f2,_tmp56A.f2=r;_tmp56A;});}_LL180:;}}default: _LL173: _LL174:
# 1543
({void*_tmp272=0U;({int(*_tmp663)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp274)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp274;});struct _fat_ptr _tmp662=({const char*_tmp273="anal_Rexp: Subscript -- bad type";_tag_fat(_tmp273,sizeof(char),33U);});_tmp663(_tmp662,_tag_fat(_tmp272,sizeof(void*),0U));});});}_LL16E:;}}}case 24U: _LL68: _tmp139=((struct Cyc_Absyn_Tuple_e_Absyn_Raw_exp_struct*)_tmp12B)->f1;_LL69: {struct Cyc_List_List*es=_tmp139;
# 1549
struct _tuple24 _stmttmp4E=({Cyc_NewControlFlow_anal_Rexps(env,inflow,es,1,1);});struct Cyc_List_List*_tmp27D;union Cyc_CfFlowInfo_FlowInfo _tmp27C;_LL186: _tmp27C=_stmttmp4E.f1;_tmp27D=_stmttmp4E.f2;_LL187: {union Cyc_CfFlowInfo_FlowInfo f=_tmp27C;struct Cyc_List_List*rvals=_tmp27D;
struct _fat_ptr aggrdict=({unsigned _tmp280=(unsigned)({Cyc_List_length(es);});void**_tmp27F=_cycalloc(_check_times(_tmp280,sizeof(void*)));({{unsigned _tmp56C=(unsigned)({Cyc_List_length(es);});unsigned i;for(i=0;i < _tmp56C;++ i){({
# 1552
void*_tmp664=({void*temp=(void*)((struct Cyc_List_List*)_check_null(rvals))->hd;
rvals=rvals->tl;
temp;});
# 1552
_tmp27F[i]=_tmp664;});}}0;});_tag_fat(_tmp27F,sizeof(void*),_tmp280);});
# 1556
return({struct _tuple17 _tmp56B;_tmp56B.f1=f,({void*_tmp665=(void*)({struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*_tmp27E=_cycalloc(sizeof(*_tmp27E));_tmp27E->tag=6U,(_tmp27E->f1).is_union=0,(_tmp27E->f1).fieldnum=- 1,_tmp27E->f2=aggrdict;_tmp27E;});_tmp56B.f2=_tmp665;});_tmp56B;});}}case 30U: _LL6A: _tmp138=((struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct*)_tmp12B)->f2;_LL6B: {struct Cyc_List_List*des=_tmp138;
# 1559
struct Cyc_List_List*fs;
{void*_stmttmp4F=({Cyc_Tcutil_compress((void*)_check_null(e->topt));});void*_tmp281=_stmttmp4F;struct Cyc_List_List*_tmp282;if(((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp281)->tag == 7U){_LL189: _tmp282=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp281)->f2;_LL18A: {struct Cyc_List_List*f=_tmp282;
fs=f;goto _LL188;}}else{_LL18B: _LL18C:
({void*_tmp283=0U;({int(*_tmp667)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp285)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp285;});struct _fat_ptr _tmp666=({const char*_tmp284="anal_Rexp:anon struct has bad type";_tag_fat(_tmp284,sizeof(char),35U);});_tmp667(_tmp666,_tag_fat(_tmp283,sizeof(void*),0U));});});}_LL188:;}
# 1564
_tmp135=des;_tmp136=0U;_tmp137=fs;goto _LL6D;}case 29U: if(((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_tmp12B)->f4 != 0){if(((struct Cyc_Absyn_Aggrdecl*)((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_tmp12B)->f4)->impl != 0){_LL6C: _tmp135=((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_tmp12B)->f3;_tmp136=(((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_tmp12B)->f4)->kind;_tmp137=((((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_tmp12B)->f4)->impl)->fields;_LL6D: {struct Cyc_List_List*des=_tmp135;enum Cyc_Absyn_AggrKind kind=_tmp136;struct Cyc_List_List*fs=_tmp137;
# 1566
void*exp_type=(void*)_check_null(e->topt);
struct _tuple24 _stmttmp50=({struct _tuple24(*_tmp28E)(struct Cyc_NewControlFlow_AnalEnv*env,union Cyc_CfFlowInfo_FlowInfo inflow,struct Cyc_List_List*es,int first_is_copy,int others_are_copy)=Cyc_NewControlFlow_anal_Rexps;struct Cyc_NewControlFlow_AnalEnv*_tmp28F=env;union Cyc_CfFlowInfo_FlowInfo _tmp290=inflow;struct Cyc_List_List*_tmp291=({({struct Cyc_List_List*(*_tmp669)(struct Cyc_Absyn_Exp*(*f)(struct _tuple27*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp292)(struct Cyc_Absyn_Exp*(*f)(struct _tuple27*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_Exp*(*f)(struct _tuple27*),struct Cyc_List_List*x))Cyc_List_map;_tmp292;});struct Cyc_Absyn_Exp*(*_tmp668)(struct _tuple27*)=({struct Cyc_Absyn_Exp*(*_tmp293)(struct _tuple27*)=(struct Cyc_Absyn_Exp*(*)(struct _tuple27*))Cyc_Core_snd;_tmp293;});_tmp669(_tmp668,des);});});int _tmp294=1;int _tmp295=1;_tmp28E(_tmp28F,_tmp290,_tmp291,_tmp294,_tmp295);});struct Cyc_List_List*_tmp287;union Cyc_CfFlowInfo_FlowInfo _tmp286;_LL18E: _tmp286=_stmttmp50.f1;_tmp287=_stmttmp50.f2;_LL18F: {union Cyc_CfFlowInfo_FlowInfo f=_tmp286;struct Cyc_List_List*rvals=_tmp287;
struct _fat_ptr aggrdict=({Cyc_CfFlowInfo_aggrfields_to_aggrdict(fenv,fs,(int)kind == (int)Cyc_Absyn_UnionA,fenv->unknown_all);});
# 1571
int field_no=-1;
{int i=0;for(0;rvals != 0;(rvals=rvals->tl,des=des->tl,++ i)){
struct Cyc_List_List*ds=(*((struct _tuple27*)((struct Cyc_List_List*)_check_null(des))->hd)).f1;for(0;ds != 0;ds=ds->tl){
void*_stmttmp51=(void*)ds->hd;void*_tmp288=_stmttmp51;struct _fat_ptr*_tmp289;if(((struct Cyc_Absyn_ArrayElement_Absyn_Designator_struct*)_tmp288)->tag == 0U){_LL191: _LL192:
({void*_tmp28A=0U;({int(*_tmp66B)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp28C)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp28C;});struct _fat_ptr _tmp66A=({const char*_tmp28B="anal_Rexp:Aggregate_e";_tag_fat(_tmp28B,sizeof(char),22U);});_tmp66B(_tmp66A,_tag_fat(_tmp28A,sizeof(void*),0U));});});}else{_LL193: _tmp289=((struct Cyc_Absyn_FieldName_Absyn_Designator_struct*)_tmp288)->f1;_LL194: {struct _fat_ptr*fld=_tmp289;
# 1578
field_no=({Cyc_CfFlowInfo_get_field_index_fs(fs,fld);});
*((void**)_check_fat_subscript(aggrdict,sizeof(void*),field_no))=(void*)rvals->hd;
# 1581
if((int)kind == (int)1U){
struct Cyc_Absyn_Exp*e=(*((struct _tuple27*)des->hd)).f2;
f=({Cyc_NewControlFlow_use_Rval(env,e->loc,f,(void*)rvals->hd);});
# 1585
({Cyc_NewControlFlow_check_union_requires(e->loc,exp_type,fld,f);});}}}_LL190:;}}}{
# 1588
struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*res=({struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct*_tmp28D=_cycalloc(sizeof(*_tmp28D));_tmp28D->tag=6U,(_tmp28D->f1).is_union=(int)kind == (int)Cyc_Absyn_UnionA,(_tmp28D->f1).fieldnum=field_no,_tmp28D->f2=aggrdict;_tmp28D;});
return({struct _tuple17 _tmp56D;_tmp56D.f1=f,_tmp56D.f2=(void*)res;_tmp56D;});}}}}else{goto _LL6E;}}else{_LL6E: _LL6F:
# 1592
({struct Cyc_String_pa_PrintArg_struct _tmp299=({struct Cyc_String_pa_PrintArg_struct _tmp56E;_tmp56E.tag=0U,({struct _fat_ptr _tmp66C=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_exp2string(e);}));_tmp56E.f1=_tmp66C;});_tmp56E;});void*_tmp296[1U];_tmp296[0]=& _tmp299;({int(*_tmp66E)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp298)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp298;});struct _fat_ptr _tmp66D=({const char*_tmp297="anal_Rexp:missing aggrdeclimpl in %s";_tag_fat(_tmp297,sizeof(char),37U);});_tmp66E(_tmp66D,_tag_fat(_tmp296,sizeof(void*),1U));});});}case 26U: _LL70: _tmp134=((struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*)_tmp12B)->f1;_LL71: {struct Cyc_List_List*dles=_tmp134;
# 1594
struct Cyc_List_List*es=({({struct Cyc_List_List*(*_tmp670)(struct Cyc_Absyn_Exp*(*f)(struct _tuple27*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp29C)(struct Cyc_Absyn_Exp*(*f)(struct _tuple27*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_Exp*(*f)(struct _tuple27*),struct Cyc_List_List*x))Cyc_List_map;_tmp29C;});struct Cyc_Absyn_Exp*(*_tmp66F)(struct _tuple27*)=({struct Cyc_Absyn_Exp*(*_tmp29D)(struct _tuple27*)=(struct Cyc_Absyn_Exp*(*)(struct _tuple27*))Cyc_Core_snd;_tmp29D;});_tmp670(_tmp66F,dles);});});
struct _tuple24 _stmttmp52=({Cyc_NewControlFlow_anal_Rexps(env,inflow,es,1,1);});struct Cyc_List_List*_tmp29B;union Cyc_CfFlowInfo_FlowInfo _tmp29A;_LL196: _tmp29A=_stmttmp52.f1;_tmp29B=_stmttmp52.f2;_LL197: {union Cyc_CfFlowInfo_FlowInfo outflow=_tmp29A;struct Cyc_List_List*rvals=_tmp29B;
for(0;rvals != 0;(rvals=rvals->tl,es=es->tl)){
outflow=({Cyc_NewControlFlow_use_Rval(env,((struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(es))->hd)->loc,outflow,(void*)rvals->hd);});}
# 1599
return({struct _tuple17 _tmp56F;_tmp56F.f1=outflow,({void*_tmp671=({Cyc_CfFlowInfo_typ_to_absrval(fenv,(void*)_check_null(e->topt),0,fenv->notzeroall);});_tmp56F.f2=_tmp671;});_tmp56F;});}}case 27U: _LL72: _tmp130=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmp12B)->f1;_tmp131=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmp12B)->f2;_tmp132=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmp12B)->f3;_tmp133=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmp12B)->f4;_LL73: {struct Cyc_Absyn_Vardecl*vd=_tmp130;struct Cyc_Absyn_Exp*e1=_tmp131;struct Cyc_Absyn_Exp*e2=_tmp132;int zt=_tmp133;
# 1603
struct _tuple17 _stmttmp53=({Cyc_NewControlFlow_anal_Rexp(env,1,inflow,e1);});void*_tmp29F;union Cyc_CfFlowInfo_FlowInfo _tmp29E;_LL199: _tmp29E=_stmttmp53.f1;_tmp29F=_stmttmp53.f2;_LL19A: {union Cyc_CfFlowInfo_FlowInfo f1=_tmp29E;void*r1=_tmp29F;
union Cyc_CfFlowInfo_FlowInfo _tmp2A0=f1;struct Cyc_List_List*_tmp2A2;struct Cyc_Dict_Dict _tmp2A1;if((_tmp2A0.BottomFL).tag == 1){_LL19C: _LL19D:
 return({struct _tuple17 _tmp570;_tmp570.f1=f1,_tmp570.f2=fenv->unknown_all;_tmp570;});}else{_LL19E: _tmp2A1=((_tmp2A0.ReachableFL).val).f1;_tmp2A2=((_tmp2A0.ReachableFL).val).f2;_LL19F: {struct Cyc_Dict_Dict d1=_tmp2A1;struct Cyc_List_List*relns=_tmp2A2;
# 1607
if((int)({Cyc_CfFlowInfo_initlevel(env->fenv,d1,r1);})== (int)Cyc_CfFlowInfo_NoneIL)
({void*_tmp2A3=0U;({unsigned _tmp673=e1->loc;struct _fat_ptr _tmp672=({const char*_tmp2A4="expression may not be initialized";_tag_fat(_tmp2A4,sizeof(char),34U);});Cyc_CfFlowInfo_aerr(_tmp673,_tmp672,_tag_fat(_tmp2A3,sizeof(void*),0U));});});{
# 1607
struct Cyc_List_List*new_relns=relns;
# 1612
union Cyc_Relations_RelnOp n1=({Cyc_Relations_RVar(vd);});
union Cyc_Relations_RelnOp n2=({Cyc_Relations_RConst(0U);});
if(({Cyc_Relations_exp2relnop(e1,& n2);}))
new_relns=({Cyc_Relations_add_relation(Cyc_Core_heap_region,n1,Cyc_Relations_Rlt,n2,relns);});
# 1614
if(relns != new_relns)
# 1617
f1=({Cyc_CfFlowInfo_ReachableFL(d1,new_relns);});{
# 1614
void*_tmp2A5=r1;switch(*((int*)_tmp2A5)){case 0U: _LL1A1: _LL1A2:
# 1621
 return({struct _tuple17 _tmp571;_tmp571.f1=f1,_tmp571.f2=fenv->unknown_all;_tmp571;});case 1U: _LL1A3: _LL1A4:
 goto _LL1A6;case 4U: _LL1A5: _LL1A6: {
# 1624
struct Cyc_List_List l=({struct Cyc_List_List _tmp574;_tmp574.hd=vd,_tmp574.tl=0;_tmp574;});
f1=({Cyc_NewControlFlow_add_vars(fenv,f1,& l,fenv->unknown_all,e->loc,0);});{
# 1627
struct _tuple17 _stmttmp54=({Cyc_NewControlFlow_anal_Rexp(env,1,f1,e2);});void*_tmp2A7;union Cyc_CfFlowInfo_FlowInfo _tmp2A6;_LL1AA: _tmp2A6=_stmttmp54.f1;_tmp2A7=_stmttmp54.f2;_LL1AB: {union Cyc_CfFlowInfo_FlowInfo f2=_tmp2A6;void*r2=_tmp2A7;
{union Cyc_CfFlowInfo_FlowInfo _tmp2A8=f2;struct Cyc_Dict_Dict _tmp2A9;if((_tmp2A8.BottomFL).tag == 1){_LL1AD: _LL1AE:
 return({struct _tuple17 _tmp572;_tmp572.f1=f2,_tmp572.f2=fenv->unknown_all;_tmp572;});}else{_LL1AF: _tmp2A9=((_tmp2A8.ReachableFL).val).f1;_LL1B0: {struct Cyc_Dict_Dict d2=_tmp2A9;
# 1631
if((int)({Cyc_CfFlowInfo_initlevel(fenv,d2,r2);})!= (int)Cyc_CfFlowInfo_AllIL){
({void*_tmp2AA=0U;({unsigned _tmp675=e1->loc;struct _fat_ptr _tmp674=({const char*_tmp2AB="expression may not be initialized";_tag_fat(_tmp2AB,sizeof(char),34U);});Cyc_CfFlowInfo_aerr(_tmp675,_tmp674,_tag_fat(_tmp2AA,sizeof(void*),0U));});});
return({struct _tuple17 _tmp573;({union Cyc_CfFlowInfo_FlowInfo _tmp676=({Cyc_CfFlowInfo_BottomFL();});_tmp573.f1=_tmp676;}),_tmp573.f2=fenv->unknown_all;_tmp573;});}}}_LL1AC:;}
# 1636
f1=f2;
goto _LL1A8;}}}default: _LL1A7: _LL1A8:
# 1639
 while(1){
struct Cyc_List_List l=({struct Cyc_List_List _tmp576;_tmp576.hd=vd,_tmp576.tl=0;_tmp576;});
f1=({Cyc_NewControlFlow_add_vars(fenv,f1,& l,fenv->unknown_all,e->loc,0);});{
struct _tuple17 _stmttmp55=({Cyc_NewControlFlow_anal_Rexp(env,1,f1,e2);});void*_tmp2AD;union Cyc_CfFlowInfo_FlowInfo _tmp2AC;_LL1B2: _tmp2AC=_stmttmp55.f1;_tmp2AD=_stmttmp55.f2;_LL1B3: {union Cyc_CfFlowInfo_FlowInfo f2=_tmp2AC;void*r2=_tmp2AD;
{union Cyc_CfFlowInfo_FlowInfo _tmp2AE=f2;struct Cyc_Dict_Dict _tmp2AF;if((_tmp2AE.BottomFL).tag == 1){_LL1B5: _LL1B6:
 goto _LL1B4;}else{_LL1B7: _tmp2AF=((_tmp2AE.ReachableFL).val).f1;_LL1B8: {struct Cyc_Dict_Dict d2=_tmp2AF;
# 1646
if((int)({Cyc_CfFlowInfo_initlevel(fenv,d2,r2);})!= (int)Cyc_CfFlowInfo_AllIL){
({void*_tmp2B0=0U;({unsigned _tmp678=e1->loc;struct _fat_ptr _tmp677=({const char*_tmp2B1="expression may not be initialized";_tag_fat(_tmp2B1,sizeof(char),34U);});Cyc_CfFlowInfo_aerr(_tmp678,_tmp677,_tag_fat(_tmp2B0,sizeof(void*),0U));});});
return({struct _tuple17 _tmp575;({union Cyc_CfFlowInfo_FlowInfo _tmp679=({Cyc_CfFlowInfo_BottomFL();});_tmp575.f1=_tmp679;}),_tmp575.f2=fenv->unknown_all;_tmp575;});}}}_LL1B4:;}{
# 1651
union Cyc_CfFlowInfo_FlowInfo newflow=({Cyc_CfFlowInfo_join_flow(fenv,f1,f2);});
if(({Cyc_CfFlowInfo_flow_lessthan_approx(newflow,f1);}))
break;
# 1652
f1=newflow;}}}}
# 1656
return({struct _tuple17 _tmp577;_tmp577.f1=f1,_tmp577.f2=fenv->notzeroall;_tmp577;});}_LL1A0:;}}}}_LL19B:;}}case 28U: _LL74: _tmp12D=((struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*)_tmp12B)->f1;_tmp12E=(void*)((struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*)_tmp12B)->f2;_tmp12F=((struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*)_tmp12B)->f3;_LL75: {struct Cyc_Absyn_Exp*exp=_tmp12D;void*typ=_tmp12E;int iszeroterm=_tmp12F;
# 1662
void*root=(void*)({struct Cyc_CfFlowInfo_MallocPt_CfFlowInfo_Root_struct*_tmp2BB=_cycalloc(sizeof(*_tmp2BB));_tmp2BB->tag=1U,_tmp2BB->f1=exp,_tmp2BB->f2=(void*)_check_null(e->topt);_tmp2BB;});
struct Cyc_CfFlowInfo_Place*place=({struct Cyc_CfFlowInfo_Place*_tmp2BA=_cycalloc(sizeof(*_tmp2BA));_tmp2BA->root=root,_tmp2BA->path=0;_tmp2BA;});
void*rval=(void*)({struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct*_tmp2B9=_cycalloc(sizeof(*_tmp2B9));_tmp2B9->tag=4U,_tmp2B9->f1=place;_tmp2B9;});
void*place_val;
# 1670
place_val=({Cyc_CfFlowInfo_typ_to_absrval(fenv,typ,0,fenv->unknown_none);});{
union Cyc_CfFlowInfo_FlowInfo outflow;
struct _tuple17 _stmttmp56=({Cyc_NewControlFlow_anal_Rexp(env,1,inflow,exp);});union Cyc_CfFlowInfo_FlowInfo _tmp2B2;_LL1BA: _tmp2B2=_stmttmp56.f1;_LL1BB: {union Cyc_CfFlowInfo_FlowInfo f=_tmp2B2;
outflow=f;{
union Cyc_CfFlowInfo_FlowInfo _tmp2B3=outflow;struct Cyc_List_List*_tmp2B5;struct Cyc_Dict_Dict _tmp2B4;if((_tmp2B3.BottomFL).tag == 1){_LL1BD: _LL1BE:
 return({struct _tuple17 _tmp578;_tmp578.f1=outflow,_tmp578.f2=rval;_tmp578;});}else{_LL1BF: _tmp2B4=((_tmp2B3.ReachableFL).val).f1;_tmp2B5=((_tmp2B3.ReachableFL).val).f2;_LL1C0: {struct Cyc_Dict_Dict d2=_tmp2B4;struct Cyc_List_List*relns=_tmp2B5;
# 1677
return({struct _tuple17 _tmp579;({union Cyc_CfFlowInfo_FlowInfo _tmp67A=({union Cyc_CfFlowInfo_FlowInfo(*_tmp2B6)(struct Cyc_Dict_Dict,struct Cyc_List_List*)=Cyc_CfFlowInfo_ReachableFL;struct Cyc_Dict_Dict _tmp2B7=({Cyc_Dict_insert(d2,root,place_val);});struct Cyc_List_List*_tmp2B8=relns;_tmp2B6(_tmp2B7,_tmp2B8);});_tmp579.f1=_tmp67A;}),_tmp579.f2=rval;_tmp579;});}}_LL1BC:;}}}}case 38U: _LL76: _tmp12C=((struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct*)_tmp12B)->f1;_LL77: {struct Cyc_Absyn_Stmt*s=_tmp12C;
# 1681
struct _tuple18 rval_opt=({struct _tuple18 _tmp57B;_tmp57B.f1=(env->fenv)->unknown_all,_tmp57B.f2=copy_ctxt;_tmp57B;});
union Cyc_CfFlowInfo_FlowInfo f=({Cyc_NewControlFlow_anal_stmt(env,inflow,s,& rval_opt);});
return({struct _tuple17 _tmp57A;_tmp57A.f1=f,_tmp57A.f2=rval_opt.f1;_tmp57A;});}case 37U: _LL7A: _LL7B:
# 1686
 goto _LL7D;case 25U: _LL7C: _LL7D:
 goto _LL7F;default: _LL7E: _LL7F:
# 1689
({void*_tmp2BC=0U;({int(*_tmp67C)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp2BE)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp2BE;});struct _fat_ptr _tmp67B=({const char*_tmp2BD="anal_Rexp, unexpected exp form";_tag_fat(_tmp2BD,sizeof(char),31U);});_tmp67C(_tmp67B,_tag_fat(_tmp2BC,sizeof(void*),0U));});});}_LLD:;}}
# 1701 "new_control_flow.cyc"
static struct _tuple19 Cyc_NewControlFlow_anal_derefL(struct Cyc_NewControlFlow_AnalEnv*env,union Cyc_CfFlowInfo_FlowInfo inflow,struct Cyc_Absyn_Exp*e,union Cyc_CfFlowInfo_FlowInfo f,void*r,int passthrough_consumes,int expanded_place_possibly_null,struct Cyc_List_List*path){
# 1710
struct Cyc_CfFlowInfo_FlowEnv*fenv=env->fenv;
void*_stmttmp57=({Cyc_Tcutil_compress((void*)_check_null(e->topt));});void*_tmp2C0=_stmttmp57;void*_tmp2C2;void*_tmp2C1;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp2C0)->tag == 3U){_LL1: _tmp2C1=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp2C0)->f1).ptr_atts).rgn;_tmp2C2=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp2C0)->f1).ptr_atts).bounds;_LL2: {void*rgn=_tmp2C1;void*bd=_tmp2C2;
# 1713
union Cyc_CfFlowInfo_FlowInfo _tmp2C3=f;struct Cyc_List_List*_tmp2C5;struct Cyc_Dict_Dict _tmp2C4;if((_tmp2C3.BottomFL).tag == 1){_LL6: _LL7:
 return({struct _tuple19 _tmp57C;_tmp57C.f1=f,({union Cyc_CfFlowInfo_AbsLVal _tmp67D=({Cyc_CfFlowInfo_UnknownL();});_tmp57C.f2=_tmp67D;});_tmp57C;});}else{_LL8: _tmp2C4=((_tmp2C3.ReachableFL).val).f1;_tmp2C5=((_tmp2C3.ReachableFL).val).f2;_LL9: {struct Cyc_Dict_Dict outdict=_tmp2C4;struct Cyc_List_List*relns=_tmp2C5;
# 1717
struct _tuple16 _stmttmp58=({Cyc_CfFlowInfo_unname_rval(r);});struct Cyc_List_List*_tmp2C7;void*_tmp2C6;_LLB: _tmp2C6=_stmttmp58.f1;_tmp2C7=_stmttmp58.f2;_LLC: {void*r=_tmp2C6;struct Cyc_List_List*names=_tmp2C7;
retry: {void*_tmp2C8=r;void*_tmp2C9;enum Cyc_CfFlowInfo_InitLevel _tmp2CA;struct Cyc_List_List*_tmp2CC;void*_tmp2CB;void*_tmp2CD;switch(*((int*)_tmp2C8)){case 8U: _LLE: _LLF:
# 1720
({void*_tmp2CE=0U;({int(*_tmp67F)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp2D0)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp2D0;});struct _fat_ptr _tmp67E=({const char*_tmp2CF="named location in anal_derefL";_tag_fat(_tmp2CF,sizeof(char),30U);});_tmp67F(_tmp67E,_tag_fat(_tmp2CE,sizeof(void*),0U));});});case 1U: _LL10: _LL11:
# 1722
({void*_tmp680=(void*)({struct Cyc_CfFlowInfo_NotZero_Absyn_AbsynAnnot_struct*_tmp2D1=_cycalloc(sizeof(*_tmp2D1));_tmp2D1->tag=Cyc_CfFlowInfo_NotZero,_tmp2D1->f1=relns;_tmp2D1;});e->annot=_tmp680;});goto _LLD;case 5U: _LL12: _tmp2CD=(void*)((struct Cyc_CfFlowInfo_UniquePtr_CfFlowInfo_AbsRVal_struct*)_tmp2C8)->f1;_LL13: {void*rv=_tmp2CD;
# 1725
if(expanded_place_possibly_null)
# 1728
({void*_tmp681=(void*)({struct Cyc_CfFlowInfo_UnknownZ_Absyn_AbsynAnnot_struct*_tmp2D2=_cycalloc(sizeof(*_tmp2D2));_tmp2D2->tag=Cyc_CfFlowInfo_UnknownZ,_tmp2D2->f1=relns;_tmp2D2;});e->annot=_tmp681;});else{
# 1731
void*_stmttmp59=e->annot;void*_tmp2D3=_stmttmp59;if(((struct Cyc_CfFlowInfo_UnknownZ_Absyn_AbsynAnnot_struct*)_tmp2D3)->tag == Cyc_CfFlowInfo_UnknownZ){_LL1F: _LL20:
# 1735
({void*_tmp682=(void*)({struct Cyc_CfFlowInfo_UnknownZ_Absyn_AbsynAnnot_struct*_tmp2D4=_cycalloc(sizeof(*_tmp2D4));_tmp2D4->tag=Cyc_CfFlowInfo_UnknownZ,_tmp2D4->f1=relns;_tmp2D4;});e->annot=_tmp682;});
goto _LL1E;}else{_LL21: _LL22:
# 1738
({void*_tmp683=(void*)({struct Cyc_CfFlowInfo_NotZero_Absyn_AbsynAnnot_struct*_tmp2D5=_cycalloc(sizeof(*_tmp2D5));_tmp2D5->tag=Cyc_CfFlowInfo_NotZero,_tmp2D5->f1=relns;_tmp2D5;});e->annot=_tmp683;});
# 1740
goto _LL1E;}_LL1E:;}
# 1744
if(({Cyc_Tcutil_is_bound_one(bd);}))return({struct _tuple19 _tmp57D;_tmp57D.f1=f,({union Cyc_CfFlowInfo_AbsLVal _tmp684=({Cyc_CfFlowInfo_UnknownL();});_tmp57D.f2=_tmp684;});_tmp57D;});goto _LLD;}case 4U: _LL14: _tmp2CB=(((struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct*)_tmp2C8)->f1)->root;_tmp2CC=(((struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct*)_tmp2C8)->f1)->path;_LL15: {void*root=_tmp2CB;struct Cyc_List_List*path2=_tmp2CC;
# 1748
({void*_tmp685=(void*)({struct Cyc_CfFlowInfo_NotZero_Absyn_AbsynAnnot_struct*_tmp2D6=_cycalloc(sizeof(*_tmp2D6));_tmp2D6->tag=Cyc_CfFlowInfo_NotZero,_tmp2D6->f1=relns;_tmp2D6;});e->annot=_tmp685;});
if(({Cyc_Tcutil_is_bound_one(bd);}))
return({struct _tuple19 _tmp57E;_tmp57E.f1=f,({union Cyc_CfFlowInfo_AbsLVal _tmp687=({union Cyc_CfFlowInfo_AbsLVal(*_tmp2D7)(struct Cyc_CfFlowInfo_Place*)=Cyc_CfFlowInfo_PlaceL;struct Cyc_CfFlowInfo_Place*_tmp2D8=({struct Cyc_CfFlowInfo_Place*_tmp2D9=_cycalloc(sizeof(*_tmp2D9));_tmp2D9->root=root,({struct Cyc_List_List*_tmp686=({Cyc_List_append(path2,path);});_tmp2D9->path=_tmp686;});_tmp2D9;});_tmp2D7(_tmp2D8);});_tmp57E.f2=_tmp687;});_tmp57E;});
# 1749
goto _LLD;}case 0U: _LL16: _LL17:
# 1753
 e->annot=(void*)& Cyc_CfFlowInfo_IsZero_val;
return({struct _tuple19 _tmp57F;({union Cyc_CfFlowInfo_FlowInfo _tmp689=({Cyc_CfFlowInfo_BottomFL();});_tmp57F.f1=_tmp689;}),({union Cyc_CfFlowInfo_AbsLVal _tmp688=({Cyc_CfFlowInfo_UnknownL();});_tmp57F.f2=_tmp688;});_tmp57F;});case 2U: _LL18: _tmp2CA=((struct Cyc_CfFlowInfo_UnknownR_CfFlowInfo_AbsRVal_struct*)_tmp2C8)->f1;_LL19: {enum Cyc_CfFlowInfo_InitLevel il=_tmp2CA;
# 1758
if(({Cyc_Tcutil_is_bound_one(bd);}))
f=({Cyc_NewControlFlow_notzero(env,inflow,f,e,il,names);});
# 1758
({void*_tmp68A=(void*)({struct Cyc_CfFlowInfo_UnknownZ_Absyn_AbsynAnnot_struct*_tmp2DA=_cycalloc(sizeof(*_tmp2DA));
# 1760
_tmp2DA->tag=Cyc_CfFlowInfo_UnknownZ,_tmp2DA->f1=relns;_tmp2DA;});
# 1758
e->annot=_tmp68A;});
# 1761
goto _LLD;}case 7U: _LL1A: _tmp2C9=(void*)((struct Cyc_CfFlowInfo_Consumed_CfFlowInfo_AbsRVal_struct*)_tmp2C8)->f3;_LL1B: {void*r2=_tmp2C9;
# 1763
if(passthrough_consumes){
r=r2;goto retry;}
# 1763
goto _LL1D;}default: _LL1C: _LL1D:
# 1768
({void*_tmp68B=(void*)({struct Cyc_CfFlowInfo_UnknownZ_Absyn_AbsynAnnot_struct*_tmp2DB=_cycalloc(sizeof(*_tmp2DB));_tmp2DB->tag=Cyc_CfFlowInfo_UnknownZ,_tmp2DB->f1=relns;_tmp2DB;});e->annot=_tmp68B;});
goto _LLD;}_LLD:;}
# 1771
if((int)({Cyc_CfFlowInfo_initlevel(fenv,outdict,r);})== (int)Cyc_CfFlowInfo_NoneIL){
struct _tuple16 _stmttmp5A=({Cyc_CfFlowInfo_unname_rval(r);});void*_tmp2DC;_LL24: _tmp2DC=_stmttmp5A.f1;_LL25: {void*r=_tmp2DC;
void*_tmp2DD=r;if(((struct Cyc_CfFlowInfo_Consumed_CfFlowInfo_AbsRVal_struct*)_tmp2DD)->tag == 7U){_LL27: _LL28:
# 1775
({void*_tmp2DE=0U;({unsigned _tmp68D=e->loc;struct _fat_ptr _tmp68C=({const char*_tmp2DF="attempt to dereference an alias-free that has already been copied";_tag_fat(_tmp2DF,sizeof(char),66U);});Cyc_CfFlowInfo_aerr(_tmp68D,_tmp68C,_tag_fat(_tmp2DE,sizeof(void*),0U));});});
goto _LL26;}else{_LL29: _LL2A:
# 1778
({void*_tmp2E0=0U;({unsigned _tmp68F=e->loc;struct _fat_ptr _tmp68E=({const char*_tmp2E1="dereference of possibly uninitialized pointer";_tag_fat(_tmp2E1,sizeof(char),46U);});Cyc_CfFlowInfo_aerr(_tmp68F,_tmp68E,_tag_fat(_tmp2E0,sizeof(void*),0U));});});
goto _LL26;}_LL26:;}}
# 1771
return({struct _tuple19 _tmp580;_tmp580.f1=f,({
# 1782
union Cyc_CfFlowInfo_AbsLVal _tmp690=({Cyc_CfFlowInfo_UnknownL();});_tmp580.f2=_tmp690;});_tmp580;});}}}_LL5:;}}else{_LL3: _LL4:
# 1784
({void*_tmp2E2=0U;({int(*_tmp692)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp2E4)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp2E4;});struct _fat_ptr _tmp691=({const char*_tmp2E3="left deref of non-pointer-type";_tag_fat(_tmp2E3,sizeof(char),31U);});_tmp692(_tmp691,_tag_fat(_tmp2E2,sizeof(void*),0U));});});}_LL0:;}
# 1790
static struct Cyc_CfFlowInfo_Place*Cyc_NewControlFlow_make_expanded_place(struct Cyc_CfFlowInfo_Place*p,struct Cyc_List_List*path){
struct Cyc_List_List*_tmp2E7;void*_tmp2E6;_LL1: _tmp2E6=p->root;_tmp2E7=p->path;_LL2: {void*proot=_tmp2E6;struct Cyc_List_List*ppath=_tmp2E7;
path=({struct Cyc_List_List*_tmp2E9=_cycalloc(sizeof(*_tmp2E9));({void*_tmp693=(void*)({struct Cyc_CfFlowInfo_Star_CfFlowInfo_PathCon_struct*_tmp2E8=_cycalloc(sizeof(*_tmp2E8));_tmp2E8->tag=1U;_tmp2E8;});_tmp2E9->hd=_tmp693;}),_tmp2E9->tl=path;_tmp2E9;});
return({struct Cyc_CfFlowInfo_Place*_tmp2EA=_cycalloc(sizeof(*_tmp2EA));_tmp2EA->root=proot,({struct Cyc_List_List*_tmp694=({Cyc_List_append(ppath,path);});_tmp2EA->path=_tmp694;});_tmp2EA;});}}
# 1800
static struct _tuple19 Cyc_NewControlFlow_anal_Lexp_rec(struct Cyc_NewControlFlow_AnalEnv*env,union Cyc_CfFlowInfo_FlowInfo inflow,int expand_unique,int passthrough_consumes,struct Cyc_Absyn_Exp*e,struct Cyc_List_List*path){
# 1804
struct Cyc_Dict_Dict d;
struct Cyc_CfFlowInfo_FlowEnv*fenv=env->fenv;
{union Cyc_CfFlowInfo_FlowInfo _tmp2EC=inflow;struct Cyc_List_List*_tmp2EE;struct Cyc_Dict_Dict _tmp2ED;if((_tmp2EC.BottomFL).tag == 1){_LL1: _LL2:
 return({struct _tuple19 _tmp581;({union Cyc_CfFlowInfo_FlowInfo _tmp696=({Cyc_CfFlowInfo_BottomFL();});_tmp581.f1=_tmp696;}),({union Cyc_CfFlowInfo_AbsLVal _tmp695=({Cyc_CfFlowInfo_UnknownL();});_tmp581.f2=_tmp695;});_tmp581;});}else{_LL3: _tmp2ED=((_tmp2EC.ReachableFL).val).f1;_tmp2EE=((_tmp2EC.ReachableFL).val).f2;_LL4: {struct Cyc_Dict_Dict d2=_tmp2ED;struct Cyc_List_List*relns=_tmp2EE;
# 1809
d=d2;}}_LL0:;}{
# 1813
void*_stmttmp5B=e->r;void*_tmp2EF=_stmttmp5B;struct _fat_ptr*_tmp2F1;struct Cyc_Absyn_Exp*_tmp2F0;struct Cyc_Absyn_Exp*_tmp2F3;struct Cyc_Absyn_Exp*_tmp2F2;struct Cyc_Absyn_Exp*_tmp2F4;struct _fat_ptr*_tmp2F6;struct Cyc_Absyn_Exp*_tmp2F5;struct Cyc_Absyn_Vardecl*_tmp2F7;struct Cyc_Absyn_Vardecl*_tmp2F8;struct Cyc_Absyn_Vardecl*_tmp2F9;struct Cyc_Absyn_Vardecl*_tmp2FA;struct Cyc_Absyn_Exp*_tmp2FB;struct Cyc_Absyn_Exp*_tmp2FC;struct Cyc_Absyn_Exp*_tmp2FD;switch(*((int*)_tmp2EF)){case 14U: _LL6: _tmp2FD=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp2EF)->f2;_LL7: {struct Cyc_Absyn_Exp*e1=_tmp2FD;
_tmp2FC=e1;goto _LL9;}case 12U: _LL8: _tmp2FC=((struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*)_tmp2EF)->f1;_LL9: {struct Cyc_Absyn_Exp*e1=_tmp2FC;
_tmp2FB=e1;goto _LLB;}case 13U: _LLA: _tmp2FB=((struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*)_tmp2EF)->f1;_LLB: {struct Cyc_Absyn_Exp*e1=_tmp2FB;
return({Cyc_NewControlFlow_anal_Lexp_rec(env,inflow,expand_unique,passthrough_consumes,e1,path);});}case 1U: switch(*((int*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp2EF)->f1)){case 4U: _LLC: _tmp2FA=((struct Cyc_Absyn_Local_b_Absyn_Binding_struct*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp2EF)->f1)->f1;_LLD: {struct Cyc_Absyn_Vardecl*vd=_tmp2FA;
# 1819
if((int)vd->sc == (int)Cyc_Absyn_Static)
return({struct _tuple19 _tmp582;_tmp582.f1=inflow,({union Cyc_CfFlowInfo_AbsLVal _tmp697=({Cyc_CfFlowInfo_UnknownL();});_tmp582.f2=_tmp697;});_tmp582;});
# 1819
_tmp2F9=vd;goto _LLF;}case 3U: _LLE: _tmp2F9=((struct Cyc_Absyn_Param_b_Absyn_Binding_struct*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp2EF)->f1)->f1;_LLF: {struct Cyc_Absyn_Vardecl*vd=_tmp2F9;
# 1822
_tmp2F8=vd;goto _LL11;}case 5U: _LL10: _tmp2F8=((struct Cyc_Absyn_Pat_b_Absyn_Binding_struct*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp2EF)->f1)->f1;_LL11: {struct Cyc_Absyn_Vardecl*vd=_tmp2F8;
# 1824
return({struct _tuple19 _tmp583;_tmp583.f1=inflow,({union Cyc_CfFlowInfo_AbsLVal _tmp699=({Cyc_CfFlowInfo_PlaceL(({struct Cyc_CfFlowInfo_Place*_tmp2FF=_cycalloc(sizeof(*_tmp2FF));({void*_tmp698=(void*)({struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct*_tmp2FE=_cycalloc(sizeof(*_tmp2FE));_tmp2FE->tag=0U,_tmp2FE->f1=vd;_tmp2FE;});_tmp2FF->root=_tmp698;}),_tmp2FF->path=path;_tmp2FF;}));});_tmp583.f2=_tmp699;});_tmp583;});}case 1U: _LL12: _tmp2F7=((struct Cyc_Absyn_Global_b_Absyn_Binding_struct*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp2EF)->f1)->f1;_LL13: {struct Cyc_Absyn_Vardecl*vd=_tmp2F7;
# 1826
return({struct _tuple19 _tmp584;_tmp584.f1=inflow,({union Cyc_CfFlowInfo_AbsLVal _tmp69A=({Cyc_CfFlowInfo_UnknownL();});_tmp584.f2=_tmp69A;});_tmp584;});}default: goto _LL1C;}case 22U: _LL14: _tmp2F5=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp2EF)->f1;_tmp2F6=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp2EF)->f2;_LL15: {struct Cyc_Absyn_Exp*e1=_tmp2F5;struct _fat_ptr*f=_tmp2F6;
# 1829
{void*_stmttmp5C=({Cyc_Tcutil_compress((void*)_check_null(e1->topt));});void*_tmp300=_stmttmp5C;void*_tmp301;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp300)->tag == 3U){_LL1F: _tmp301=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp300)->f1).elt_type;_LL20: {void*t2=_tmp301;
# 1831
if(!({Cyc_Absyn_is_nontagged_nonrequire_union_type(t2);})){
({Cyc_NewControlFlow_check_union_requires(e1->loc,t2,f,inflow);});
path=({struct Cyc_List_List*_tmp303=_cycalloc(sizeof(*_tmp303));({void*_tmp69C=(void*)({struct Cyc_CfFlowInfo_Dot_CfFlowInfo_PathCon_struct*_tmp302=_cycalloc(sizeof(*_tmp302));_tmp302->tag=0U,({int _tmp69B=({Cyc_CfFlowInfo_get_field_index(t2,f);});_tmp302->f1=_tmp69B;});_tmp302;});_tmp303->hd=_tmp69C;}),_tmp303->tl=path;_tmp303;});}
# 1831
goto _LL1E;}}else{_LL21: _LL22:
# 1836
({void*_tmp304=0U;({int(*_tmp69E)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp306)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp306;});struct _fat_ptr _tmp69D=({const char*_tmp305="anal_Lexp: AggrArrow ptr";_tag_fat(_tmp305,sizeof(char),25U);});_tmp69E(_tmp69D,_tag_fat(_tmp304,sizeof(void*),0U));});});}_LL1E:;}
# 1838
_tmp2F4=e1;goto _LL17;}case 20U: _LL16: _tmp2F4=((struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_tmp2EF)->f1;_LL17: {struct Cyc_Absyn_Exp*e1=_tmp2F4;
# 1842
if(({Cyc_Tcutil_is_noalias_pointer((void*)_check_null(e1->topt),1);})){
# 1844
struct _tuple19 _stmttmp5D=({Cyc_NewControlFlow_anal_Lexp(env,inflow,expand_unique,passthrough_consumes,e1);});union Cyc_CfFlowInfo_AbsLVal _tmp308;union Cyc_CfFlowInfo_FlowInfo _tmp307;_LL24: _tmp307=_stmttmp5D.f1;_tmp308=_stmttmp5D.f2;_LL25: {union Cyc_CfFlowInfo_FlowInfo f=_tmp307;union Cyc_CfFlowInfo_AbsLVal lval=_tmp308;
struct _tuple19 _stmttmp5E=({struct _tuple19 _tmp588;_tmp588.f1=f,_tmp588.f2=lval;_tmp588;});struct _tuple19 _tmp309=_stmttmp5E;struct Cyc_CfFlowInfo_Place*_tmp30C;struct Cyc_List_List*_tmp30B;struct Cyc_Dict_Dict _tmp30A;if(((_tmp309.f1).ReachableFL).tag == 2){if(((_tmp309.f2).PlaceL).tag == 1){_LL27: _tmp30A=(((_tmp309.f1).ReachableFL).val).f1;_tmp30B=(((_tmp309.f1).ReachableFL).val).f2;_tmp30C=((_tmp309.f2).PlaceL).val;_LL28: {struct Cyc_Dict_Dict fd=_tmp30A;struct Cyc_List_List*r=_tmp30B;struct Cyc_CfFlowInfo_Place*p=_tmp30C;
# 1847
void*old_rval=({Cyc_CfFlowInfo_lookup_place(fd,p);});
struct _tuple16 _stmttmp5F=({Cyc_CfFlowInfo_unname_rval(old_rval);});struct Cyc_List_List*_tmp30E;void*_tmp30D;_LL2C: _tmp30D=_stmttmp5F.f1;_tmp30E=_stmttmp5F.f2;_LL2D: {void*rval=_tmp30D;struct Cyc_List_List*names=_tmp30E;
if(expand_unique){
int possibly_null=1;
void*_tmp30F=rval;void*_tmp311;struct Cyc_Absyn_Vardecl*_tmp310;switch(*((int*)_tmp30F)){case 8U: _LL2F: _tmp310=((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp30F)->f1;_tmp311=(void*)((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp30F)->f2;_LL30: {struct Cyc_Absyn_Vardecl*n=_tmp310;void*r=_tmp311;
# 1853
({void*_tmp312=0U;({int(*_tmp6A0)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp314)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp314;});struct _fat_ptr _tmp69F=({const char*_tmp313="bad named location in anal_Lexp:deref";_tag_fat(_tmp313,sizeof(char),38U);});_tmp6A0(_tmp69F,_tag_fat(_tmp312,sizeof(void*),0U));});});}case 7U: switch(*((int*)((struct Cyc_CfFlowInfo_Consumed_CfFlowInfo_AbsRVal_struct*)_tmp30F)->f3)){case 4U: _LL31: _LL32:
 goto _LL34;case 5U: _LL35: _LL36:
# 1857
 goto _LL38;default: goto _LL3B;}case 4U: _LL33: _LL34:
# 1856
 return({Cyc_NewControlFlow_anal_derefL(env,f,e1,f,old_rval,passthrough_consumes,0,path);});case 5U: _LL37: _LL38: {
# 1861
struct _tuple19 _stmttmp60=({Cyc_NewControlFlow_anal_derefL(env,f,e1,f,old_rval,passthrough_consumes,0,path);});union Cyc_CfFlowInfo_FlowInfo _tmp315;_LL3E: _tmp315=_stmttmp60.f1;_LL3F: {union Cyc_CfFlowInfo_FlowInfo f=_tmp315;
return({struct _tuple19 _tmp585;_tmp585.f1=f,({union Cyc_CfFlowInfo_AbsLVal _tmp6A1=({union Cyc_CfFlowInfo_AbsLVal(*_tmp316)(struct Cyc_CfFlowInfo_Place*)=Cyc_CfFlowInfo_PlaceL;struct Cyc_CfFlowInfo_Place*_tmp317=({Cyc_NewControlFlow_make_expanded_place(p,path);});_tmp316(_tmp317);});_tmp585.f2=_tmp6A1;});_tmp585;});}}case 1U: _LL39: _LL3A:
# 1864
 possibly_null=0;goto _LL3C;default: _LL3B: _LL3C: {
# 1871
enum Cyc_CfFlowInfo_InitLevel il=({Cyc_CfFlowInfo_initlevel(fenv,fd,rval);});
void*leaf=(int)il == (int)1U?fenv->unknown_all: fenv->unknown_none;
void*res=(void*)({struct Cyc_CfFlowInfo_UniquePtr_CfFlowInfo_AbsRVal_struct*_tmp321=_cycalloc(sizeof(*_tmp321));_tmp321->tag=5U,({void*_tmp6A2=({void*(*_tmp31C)(struct Cyc_CfFlowInfo_FlowEnv*,void*t,int no_init_bits_only,void*leafval)=Cyc_CfFlowInfo_typ_to_absrval;struct Cyc_CfFlowInfo_FlowEnv*_tmp31D=fenv;void*_tmp31E=({Cyc_Tcutil_pointer_elt_type((void*)_check_null(e1->topt));});int _tmp31F=0;void*_tmp320=leaf;_tmp31C(_tmp31D,_tmp31E,_tmp31F,_tmp320);});_tmp321->f1=_tmp6A2;});_tmp321;});
for(0;names != 0;names=names->tl){
res=(void*)({struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*_tmp318=_cycalloc(sizeof(*_tmp318));_tmp318->tag=8U,_tmp318->f1=(struct Cyc_Absyn_Vardecl*)names->hd,_tmp318->f2=res;_tmp318;});}
fd=({Cyc_CfFlowInfo_assign_place(fenv,e->loc,fd,p,res);});{
union Cyc_CfFlowInfo_FlowInfo outflow=({Cyc_CfFlowInfo_ReachableFL(fd,r);});
# 1884
struct _tuple19 _stmttmp61=({Cyc_NewControlFlow_anal_derefL(env,outflow,e1,outflow,res,passthrough_consumes,possibly_null,path);});union Cyc_CfFlowInfo_FlowInfo _tmp319;_LL41: _tmp319=_stmttmp61.f1;_LL42: {union Cyc_CfFlowInfo_FlowInfo f=_tmp319;
# 1886
return({struct _tuple19 _tmp586;_tmp586.f1=f,({union Cyc_CfFlowInfo_AbsLVal _tmp6A3=({union Cyc_CfFlowInfo_AbsLVal(*_tmp31A)(struct Cyc_CfFlowInfo_Place*)=Cyc_CfFlowInfo_PlaceL;struct Cyc_CfFlowInfo_Place*_tmp31B=({Cyc_NewControlFlow_make_expanded_place(p,path);});_tmp31A(_tmp31B);});_tmp586.f2=_tmp6A3;});_tmp586;});}}}}_LL2E:;}else{
# 1890
void*_tmp322=rval;switch(*((int*)_tmp322)){case 7U: if(((struct Cyc_CfFlowInfo_UniquePtr_CfFlowInfo_AbsRVal_struct*)((struct Cyc_CfFlowInfo_Consumed_CfFlowInfo_AbsRVal_struct*)_tmp322)->f3)->tag == 5U){_LL44: _LL45:
 goto _LL47;}else{goto _LL48;}case 5U: _LL46: _LL47: {
# 1895
struct _tuple19 _stmttmp62=({Cyc_NewControlFlow_anal_derefL(env,f,e1,f,old_rval,passthrough_consumes,0,path);});union Cyc_CfFlowInfo_FlowInfo _tmp323;_LL4B: _tmp323=_stmttmp62.f1;_LL4C: {union Cyc_CfFlowInfo_FlowInfo f=_tmp323;
return({struct _tuple19 _tmp587;_tmp587.f1=f,({union Cyc_CfFlowInfo_AbsLVal _tmp6A4=({union Cyc_CfFlowInfo_AbsLVal(*_tmp324)(struct Cyc_CfFlowInfo_Place*)=Cyc_CfFlowInfo_PlaceL;struct Cyc_CfFlowInfo_Place*_tmp325=({Cyc_NewControlFlow_make_expanded_place(p,path);});_tmp324(_tmp325);});_tmp587.f2=_tmp6A4;});_tmp587;});}}default: _LL48: _LL49:
# 1899
 goto _LL43;}_LL43:;}
# 1902
goto _LL26;}}}else{goto _LL29;}}else{_LL29: _LL2A:
 goto _LL26;}_LL26:;}}{
# 1907
struct _tuple17 _stmttmp63=({Cyc_NewControlFlow_anal_Rexp(env,0,inflow,e1);});void*_tmp327;union Cyc_CfFlowInfo_FlowInfo _tmp326;_LL4E: _tmp326=_stmttmp63.f1;_tmp327=_stmttmp63.f2;_LL4F: {union Cyc_CfFlowInfo_FlowInfo f=_tmp326;void*r=_tmp327;
# 1909
return({Cyc_NewControlFlow_anal_derefL(env,inflow,e1,f,r,passthrough_consumes,0,path);});}}}case 23U: _LL18: _tmp2F2=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp2EF)->f1;_tmp2F3=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp2EF)->f2;_LL19: {struct Cyc_Absyn_Exp*e1=_tmp2F2;struct Cyc_Absyn_Exp*e2=_tmp2F3;
# 1912
void*_stmttmp64=({Cyc_Tcutil_compress((void*)_check_null(e1->topt));});void*_tmp328=_stmttmp64;switch(*((int*)_tmp328)){case 6U: _LL51: _LL52: {
# 1914
unsigned fld=({Cyc_Evexp_eval_const_uint_exp(e2);}).f1;
return({({struct Cyc_NewControlFlow_AnalEnv*_tmp6AA=env;union Cyc_CfFlowInfo_FlowInfo _tmp6A9=inflow;int _tmp6A8=expand_unique;int _tmp6A7=passthrough_consumes;struct Cyc_Absyn_Exp*_tmp6A6=e1;Cyc_NewControlFlow_anal_Lexp_rec(_tmp6AA,_tmp6A9,_tmp6A8,_tmp6A7,_tmp6A6,({struct Cyc_List_List*_tmp32A=_cycalloc(sizeof(*_tmp32A));({void*_tmp6A5=(void*)({struct Cyc_CfFlowInfo_Dot_CfFlowInfo_PathCon_struct*_tmp329=_cycalloc(sizeof(*_tmp329));_tmp329->tag=0U,_tmp329->f1=(int)fld;_tmp329;});_tmp32A->hd=_tmp6A5;}),_tmp32A->tl=path;_tmp32A;}));});});}case 3U: _LL53: _LL54: {
# 1918
struct _tuple24 _stmttmp65=({struct _tuple24(*_tmp335)(struct Cyc_NewControlFlow_AnalEnv*env,union Cyc_CfFlowInfo_FlowInfo inflow,struct Cyc_List_List*es,int first_is_copy,int others_are_copy)=Cyc_NewControlFlow_anal_Rexps;struct Cyc_NewControlFlow_AnalEnv*_tmp336=env;union Cyc_CfFlowInfo_FlowInfo _tmp337=inflow;struct Cyc_List_List*_tmp338=({struct Cyc_Absyn_Exp*_tmp339[2U];_tmp339[0]=e1,_tmp339[1]=e2;Cyc_List_list(_tag_fat(_tmp339,sizeof(struct Cyc_Absyn_Exp*),2U));});int _tmp33A=0;int _tmp33B=1;_tmp335(_tmp336,_tmp337,_tmp338,_tmp33A,_tmp33B);});struct Cyc_List_List*_tmp32C;union Cyc_CfFlowInfo_FlowInfo _tmp32B;_LL58: _tmp32B=_stmttmp65.f1;_tmp32C=_stmttmp65.f2;_LL59: {union Cyc_CfFlowInfo_FlowInfo f=_tmp32B;struct Cyc_List_List*rvals=_tmp32C;
union Cyc_CfFlowInfo_FlowInfo f2=f;
{union Cyc_CfFlowInfo_FlowInfo _tmp32D=f;struct Cyc_List_List*_tmp32F;struct Cyc_Dict_Dict _tmp32E;if((_tmp32D.ReachableFL).tag == 2){_LL5B: _tmp32E=((_tmp32D.ReachableFL).val).f1;_tmp32F=((_tmp32D.ReachableFL).val).f2;_LL5C: {struct Cyc_Dict_Dict d2=_tmp32E;struct Cyc_List_List*relns=_tmp32F;
# 1922
if((int)({Cyc_CfFlowInfo_initlevel(fenv,d2,(void*)((struct Cyc_List_List*)_check_null(((struct Cyc_List_List*)_check_null(rvals))->tl))->hd);})== (int)Cyc_CfFlowInfo_NoneIL)
({void*_tmp330=0U;({unsigned _tmp6AC=e2->loc;struct _fat_ptr _tmp6AB=({const char*_tmp331="expression may not be initialized";_tag_fat(_tmp331,sizeof(char),34U);});Cyc_CfFlowInfo_aerr(_tmp6AC,_tmp6AB,_tag_fat(_tmp330,sizeof(void*),0U));});});{
# 1922
struct Cyc_List_List*new_relns=({Cyc_NewControlFlow_add_subscript_reln(relns,e1,e2);});
# 1925
if(relns != new_relns)
f2=({Cyc_CfFlowInfo_ReachableFL(d2,new_relns);});
# 1925
goto _LL5A;}}}else{_LL5D: _LL5E:
# 1928
 goto _LL5A;}_LL5A:;}{
# 1931
struct _tuple19 _stmttmp66=({Cyc_NewControlFlow_anal_derefL(env,inflow,e1,f,(void*)((struct Cyc_List_List*)_check_null(rvals))->hd,passthrough_consumes,0,path);});union Cyc_CfFlowInfo_AbsLVal _tmp333;union Cyc_CfFlowInfo_FlowInfo _tmp332;_LL60: _tmp332=_stmttmp66.f1;_tmp333=_stmttmp66.f2;_LL61: {union Cyc_CfFlowInfo_FlowInfo f3=_tmp332;union Cyc_CfFlowInfo_AbsLVal r=_tmp333;
union Cyc_CfFlowInfo_FlowInfo _tmp334=f3;if((_tmp334.BottomFL).tag == 1){_LL63: _LL64:
 return({struct _tuple19 _tmp589;_tmp589.f1=f3,_tmp589.f2=r;_tmp589;});}else{_LL65: _LL66:
 return({struct _tuple19 _tmp58A;_tmp58A.f1=f2,_tmp58A.f2=r;_tmp58A;});}_LL62:;}}}}default: _LL55: _LL56:
# 1936
({void*_tmp33C=0U;({int(*_tmp6AE)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp33E)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp33E;});struct _fat_ptr _tmp6AD=({const char*_tmp33D="anal_Lexp: Subscript -- bad type";_tag_fat(_tmp33D,sizeof(char),33U);});_tmp6AE(_tmp6AD,_tag_fat(_tmp33C,sizeof(void*),0U));});});}_LL50:;}case 21U: _LL1A: _tmp2F0=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp2EF)->f1;_tmp2F1=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp2EF)->f2;_LL1B: {struct Cyc_Absyn_Exp*e1=_tmp2F0;struct _fat_ptr*fld=_tmp2F1;
# 1940
void*e1_type=(void*)_check_null(e1->topt);
if(({Cyc_Absyn_is_require_union_type(e1_type);}))
({Cyc_NewControlFlow_check_union_requires(e1->loc,e1_type,fld,inflow);});
# 1941
if(({Cyc_Absyn_is_nontagged_nonrequire_union_type(e1_type);}))
# 1945
return({struct _tuple19 _tmp58B;_tmp58B.f1=inflow,({union Cyc_CfFlowInfo_AbsLVal _tmp6AF=({Cyc_CfFlowInfo_UnknownL();});_tmp58B.f2=_tmp6AF;});_tmp58B;});
# 1941
return({struct _tuple19(*_tmp33F)(struct Cyc_NewControlFlow_AnalEnv*env,union Cyc_CfFlowInfo_FlowInfo inflow,int expand_unique,int passthrough_consumes,struct Cyc_Absyn_Exp*e,struct Cyc_List_List*path)=Cyc_NewControlFlow_anal_Lexp_rec;struct Cyc_NewControlFlow_AnalEnv*_tmp340=env;union Cyc_CfFlowInfo_FlowInfo _tmp341=inflow;int _tmp342=expand_unique;int _tmp343=passthrough_consumes;struct Cyc_Absyn_Exp*_tmp344=e1;struct Cyc_List_List*_tmp345=({struct Cyc_List_List*_tmp347=_cycalloc(sizeof(*_tmp347));
# 1948
({void*_tmp6B1=(void*)({struct Cyc_CfFlowInfo_Dot_CfFlowInfo_PathCon_struct*_tmp346=_cycalloc(sizeof(*_tmp346));_tmp346->tag=0U,({int _tmp6B0=({Cyc_CfFlowInfo_get_field_index(e1_type,fld);});_tmp346->f1=_tmp6B0;});_tmp346;});_tmp347->hd=_tmp6B1;}),_tmp347->tl=path;_tmp347;});_tmp33F(_tmp340,_tmp341,_tmp342,_tmp343,_tmp344,_tmp345);});}default: _LL1C: _LL1D:
# 1951
 return({struct _tuple19 _tmp58C;({union Cyc_CfFlowInfo_FlowInfo _tmp6B3=({Cyc_CfFlowInfo_BottomFL();});_tmp58C.f1=_tmp6B3;}),({union Cyc_CfFlowInfo_AbsLVal _tmp6B2=({Cyc_CfFlowInfo_UnknownL();});_tmp58C.f2=_tmp6B2;});_tmp58C;});}_LL5:;}}
# 1955
static struct _tuple19 Cyc_NewControlFlow_anal_Lexp(struct Cyc_NewControlFlow_AnalEnv*env,union Cyc_CfFlowInfo_FlowInfo inflow,int expand_unique,int passthrough_consumes,struct Cyc_Absyn_Exp*e){
# 1959
struct _tuple19 _stmttmp67=({Cyc_NewControlFlow_anal_Lexp_rec(env,inflow,expand_unique,passthrough_consumes,e,0);});union Cyc_CfFlowInfo_AbsLVal _tmp34A;union Cyc_CfFlowInfo_FlowInfo _tmp349;_LL1: _tmp349=_stmttmp67.f1;_tmp34A=_stmttmp67.f2;_LL2: {union Cyc_CfFlowInfo_FlowInfo f=_tmp349;union Cyc_CfFlowInfo_AbsLVal r=_tmp34A;
return({struct _tuple19 _tmp58D;_tmp58D.f1=f,_tmp58D.f2=r;_tmp58D;});}}
# 1966
static union Cyc_CfFlowInfo_FlowInfo Cyc_NewControlFlow_expand_unique_places(struct Cyc_NewControlFlow_AnalEnv*env,union Cyc_CfFlowInfo_FlowInfo inflow,struct Cyc_List_List*es){
# 1969
{struct Cyc_List_List*x=es;for(0;x != 0;x=x->tl){
# 1973
if(({Cyc_NewControlFlow_is_local_var_rooted_path((struct Cyc_Absyn_Exp*)x->hd,1);})&&({Cyc_Tcutil_is_noalias_pointer_or_aggr((void*)_check_null(((struct Cyc_Absyn_Exp*)x->hd)->topt));})){
# 1976
struct _tuple19 _stmttmp68=({Cyc_NewControlFlow_anal_Lexp(env,inflow,1,0,(struct Cyc_Absyn_Exp*)x->hd);});union Cyc_CfFlowInfo_FlowInfo _tmp34C;_LL1: _tmp34C=_stmttmp68.f1;_LL2: {union Cyc_CfFlowInfo_FlowInfo f=_tmp34C;
inflow=f;}}}}
# 1981
return inflow;}struct _tuple28{enum Cyc_Absyn_Primop f1;void*f2;void*f3;};
# 1987
static struct _tuple20 Cyc_NewControlFlow_anal_primop_test(struct Cyc_NewControlFlow_AnalEnv*env,union Cyc_CfFlowInfo_FlowInfo inflow,enum Cyc_Absyn_Primop p,struct Cyc_List_List*es){
# 1989
struct Cyc_CfFlowInfo_FlowEnv*fenv=env->fenv;
# 1992
struct _tuple17 _stmttmp69=({Cyc_NewControlFlow_anal_Rexp(env,0,inflow,(struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(es))->hd);});void*_tmp34F;union Cyc_CfFlowInfo_FlowInfo _tmp34E;_LL1: _tmp34E=_stmttmp69.f1;_tmp34F=_stmttmp69.f2;_LL2: {union Cyc_CfFlowInfo_FlowInfo f1=_tmp34E;void*r1=_tmp34F;
struct _tuple17 _stmttmp6A=({Cyc_NewControlFlow_anal_Rexp(env,0,f1,(struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(es->tl))->hd);});void*_tmp351;union Cyc_CfFlowInfo_FlowInfo _tmp350;_LL4: _tmp350=_stmttmp6A.f1;_tmp351=_stmttmp6A.f2;_LL5: {union Cyc_CfFlowInfo_FlowInfo f2=_tmp350;void*r2=_tmp351;
union Cyc_CfFlowInfo_FlowInfo f=f2;
# 1997
union Cyc_CfFlowInfo_FlowInfo _tmp352=f;struct Cyc_List_List*_tmp354;struct Cyc_Dict_Dict _tmp353;if((_tmp352.BottomFL).tag == 1){_LL7: _LL8:
 return({struct _tuple20 _tmp58E;_tmp58E.f1=f,_tmp58E.f2=f;_tmp58E;});}else{_LL9: _tmp353=((_tmp352.ReachableFL).val).f1;_tmp354=((_tmp352.ReachableFL).val).f2;_LLA: {struct Cyc_Dict_Dict d=_tmp353;struct Cyc_List_List*relns=_tmp354;
# 2000
struct Cyc_Absyn_Exp*e1=(struct Cyc_Absyn_Exp*)es->hd;
struct Cyc_Absyn_Exp*e2=(struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(es->tl))->hd;
if((int)({Cyc_CfFlowInfo_initlevel(env->fenv,d,r1);})== (int)Cyc_CfFlowInfo_NoneIL && !({Cyc_CfFlowInfo_is_init_pointer(r1);}))
({void*_tmp355=0U;({unsigned _tmp6B5=((struct Cyc_Absyn_Exp*)es->hd)->loc;struct _fat_ptr _tmp6B4=({const char*_tmp356="expression may not be initialized";_tag_fat(_tmp356,sizeof(char),34U);});Cyc_CfFlowInfo_aerr(_tmp6B5,_tmp6B4,_tag_fat(_tmp355,sizeof(void*),0U));});});
# 2002
if(
# 2004
(int)({Cyc_CfFlowInfo_initlevel(env->fenv,d,r2);})== (int)Cyc_CfFlowInfo_NoneIL && !({Cyc_CfFlowInfo_is_init_pointer(r1);}))
({void*_tmp357=0U;({unsigned _tmp6B7=((struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(es->tl))->hd)->loc;struct _fat_ptr _tmp6B6=({const char*_tmp358="expression may not be initialized";_tag_fat(_tmp358,sizeof(char),34U);});Cyc_CfFlowInfo_aerr(_tmp6B7,_tmp6B6,_tag_fat(_tmp357,sizeof(void*),0U));});});{
# 2002
union Cyc_CfFlowInfo_FlowInfo ft=f;
# 2008
union Cyc_CfFlowInfo_FlowInfo ff=f;
# 2012
if((int)p == (int)5U ||(int)p == (int)6U){
struct _tuple16 _stmttmp6B=({Cyc_CfFlowInfo_unname_rval(r1);});struct Cyc_List_List*_tmp35A;void*_tmp359;_LLC: _tmp359=_stmttmp6B.f1;_tmp35A=_stmttmp6B.f2;_LLD: {void*r1=_tmp359;struct Cyc_List_List*r1n=_tmp35A;
struct _tuple16 _stmttmp6C=({Cyc_CfFlowInfo_unname_rval(r2);});struct Cyc_List_List*_tmp35C;void*_tmp35B;_LLF: _tmp35B=_stmttmp6C.f1;_tmp35C=_stmttmp6C.f2;_LL10: {void*r2=_tmp35B;struct Cyc_List_List*r2n=_tmp35C;
struct _tuple0 _stmttmp6D=({struct _tuple0 _tmp58F;_tmp58F.f1=r1,_tmp58F.f2=r2;_tmp58F;});struct _tuple0 _tmp35D=_stmttmp6D;enum Cyc_CfFlowInfo_InitLevel _tmp35E;enum Cyc_CfFlowInfo_InitLevel _tmp35F;switch(*((int*)_tmp35D.f1)){case 2U: if(((struct Cyc_CfFlowInfo_Zero_CfFlowInfo_AbsRVal_struct*)_tmp35D.f2)->tag == 0U){_LL12: _tmp35F=((struct Cyc_CfFlowInfo_UnknownR_CfFlowInfo_AbsRVal_struct*)_tmp35D.f1)->f1;_LL13: {enum Cyc_CfFlowInfo_InitLevel il=_tmp35F;
# 2017
struct _tuple20 _stmttmp6E=({Cyc_NewControlFlow_splitzero(env,inflow,f,e1,il,r1n);});union Cyc_CfFlowInfo_FlowInfo _tmp361;union Cyc_CfFlowInfo_FlowInfo _tmp360;_LL23: _tmp360=_stmttmp6E.f1;_tmp361=_stmttmp6E.f2;_LL24: {union Cyc_CfFlowInfo_FlowInfo f1=_tmp360;union Cyc_CfFlowInfo_FlowInfo f2=_tmp361;
{enum Cyc_Absyn_Primop _tmp362=p;switch(_tmp362){case Cyc_Absyn_Eq: _LL26: _LL27:
 ft=f2;ff=f1;goto _LL25;case Cyc_Absyn_Neq: _LL28: _LL29:
 ft=f1;ff=f2;goto _LL25;default: _LL2A: _LL2B:
({void*_tmp363=0U;({int(*_tmp6B9)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp365)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp365;});struct _fat_ptr _tmp6B8=({const char*_tmp364="anal_test, zero-split";_tag_fat(_tmp364,sizeof(char),22U);});_tmp6B9(_tmp6B8,_tag_fat(_tmp363,sizeof(void*),0U));});});}_LL25:;}
# 2023
goto _LL11;}}}else{goto _LL20;}case 0U: switch(*((int*)_tmp35D.f2)){case 2U: _LL14: _tmp35E=((struct Cyc_CfFlowInfo_UnknownR_CfFlowInfo_AbsRVal_struct*)_tmp35D.f2)->f1;_LL15: {enum Cyc_CfFlowInfo_InitLevel il=_tmp35E;
# 2025
struct _tuple20 _stmttmp6F=({Cyc_NewControlFlow_splitzero(env,f2,f,e2,il,r2n);});union Cyc_CfFlowInfo_FlowInfo _tmp367;union Cyc_CfFlowInfo_FlowInfo _tmp366;_LL2D: _tmp366=_stmttmp6F.f1;_tmp367=_stmttmp6F.f2;_LL2E: {union Cyc_CfFlowInfo_FlowInfo f1=_tmp366;union Cyc_CfFlowInfo_FlowInfo f2=_tmp367;
{enum Cyc_Absyn_Primop _tmp368=p;switch(_tmp368){case Cyc_Absyn_Eq: _LL30: _LL31:
 ft=f2;ff=f1;goto _LL2F;case Cyc_Absyn_Neq: _LL32: _LL33:
 ft=f1;ff=f2;goto _LL2F;default: _LL34: _LL35:
({void*_tmp369=0U;({int(*_tmp6BB)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp36B)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp36B;});struct _fat_ptr _tmp6BA=({const char*_tmp36A="anal_test, zero-split";_tag_fat(_tmp36A,sizeof(char),22U);});_tmp6BB(_tmp6BA,_tag_fat(_tmp369,sizeof(void*),0U));});});}_LL2F:;}
# 2031
goto _LL11;}}case 0U: _LL16: _LL17:
# 2033
 if((int)p == (int)5U)ff=({Cyc_CfFlowInfo_BottomFL();});else{
ft=({Cyc_CfFlowInfo_BottomFL();});}
goto _LL11;case 1U: _LL18: _LL19:
 goto _LL1B;case 4U: _LL1A: _LL1B:
 goto _LL1D;default: goto _LL20;}case 1U: if(((struct Cyc_CfFlowInfo_Zero_CfFlowInfo_AbsRVal_struct*)_tmp35D.f2)->tag == 0U){_LL1C: _LL1D:
 goto _LL1F;}else{goto _LL20;}case 4U: if(((struct Cyc_CfFlowInfo_Zero_CfFlowInfo_AbsRVal_struct*)_tmp35D.f2)->tag == 0U){_LL1E: _LL1F:
# 2040
 if((int)p == (int)6U)ff=({Cyc_CfFlowInfo_BottomFL();});else{
ft=({Cyc_CfFlowInfo_BottomFL();});}
goto _LL11;}else{goto _LL20;}default: _LL20: _LL21:
 goto _LL11;}_LL11:;}}}
# 2012
{struct _tuple28 _stmttmp70=({struct _tuple28 _tmp591;_tmp591.f1=p,({
# 2053
void*_tmp6BD=({Cyc_Tcutil_compress((void*)_check_null(e1->topt));});_tmp591.f2=_tmp6BD;}),({
void*_tmp6BC=({Cyc_Tcutil_compress((void*)_check_null(e2->topt));});_tmp591.f3=_tmp6BC;});_tmp591;});
# 2012
struct _tuple28 _tmp36C=_stmttmp70;switch(_tmp36C.f1){case Cyc_Absyn_Eq: _LL37: _LL38:
# 2055
 goto _LL3A;case Cyc_Absyn_Neq: _LL39: _LL3A:
 goto _LL3C;default: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp36C.f2)->tag == 0U){if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp36C.f2)->f1)->tag == 1U){if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp36C.f2)->f1)->f1 == Cyc_Absyn_Unsigned){_LL3B: _LL3C:
 goto _LL3E;}else{if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp36C.f3)->tag == 0U)switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp36C.f3)->f1)){case 1U: if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp36C.f3)->f1)->f1 == Cyc_Absyn_Unsigned)goto _LL3D;else{goto _LL43;}case 5U: goto _LL41;default: goto _LL43;}else{goto _LL43;}}}else{if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp36C.f3)->tag == 0U){if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp36C.f3)->f1)->tag == 1U){if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp36C.f3)->f1)->f1 == Cyc_Absyn_Unsigned)goto _LL3D;else{if(((struct Cyc_Absyn_TagCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp36C.f2)->f1)->tag == 5U)goto _LL3F;else{goto _LL43;}}}else{if(((struct Cyc_Absyn_TagCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp36C.f2)->f1)->tag == 5U)goto _LL3F;else{if(((struct Cyc_Absyn_TagCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp36C.f3)->f1)->tag == 5U)goto _LL41;else{goto _LL43;}}}}else{if(((struct Cyc_Absyn_TagCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp36C.f2)->f1)->tag == 5U){_LL3F: _LL40:
# 2059
 goto _LL42;}else{goto _LL43;}}}}else{if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp36C.f3)->tag == 0U)switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp36C.f3)->f1)){case 1U: if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp36C.f3)->f1)->f1 == Cyc_Absyn_Unsigned){_LL3D: _LL3E:
# 2058
 goto _LL40;}else{goto _LL43;}case 5U: _LL41: _LL42:
# 2060
 goto _LL36;default: goto _LL43;}else{_LL43: _LL44:
 return({struct _tuple20 _tmp590;_tmp590.f1=ft,_tmp590.f2=ff;_tmp590;});}}}_LL36:;}
# 2065
{enum Cyc_Absyn_Primop _tmp36D=p;switch(_tmp36D){case Cyc_Absyn_Eq: _LL46: _LL47:
 goto _LL49;case Cyc_Absyn_Neq: _LL48: _LL49: goto _LL4B;case Cyc_Absyn_Gt: _LL4A: _LL4B: goto _LL4D;case Cyc_Absyn_Gte: _LL4C: _LL4D: goto _LL4F;case Cyc_Absyn_Lt: _LL4E: _LL4F: goto _LL51;case Cyc_Absyn_Lte: _LL50: _LL51: goto _LL45;default: _LL52: _LL53:
 return({struct _tuple20 _tmp592;_tmp592.f1=ft,_tmp592.f2=ff;_tmp592;});}_LL45:;}{
# 2070
struct _RegionHandle*r=Cyc_Core_heap_region;
struct _tuple13 _stmttmp71=({Cyc_Relations_primop2relation(e1,p,e2);});struct Cyc_Absyn_Exp*_tmp370;enum Cyc_Relations_Relation _tmp36F;struct Cyc_Absyn_Exp*_tmp36E;_LL55: _tmp36E=_stmttmp71.f1;_tmp36F=_stmttmp71.f2;_tmp370=_stmttmp71.f3;_LL56: {struct Cyc_Absyn_Exp*e1=_tmp36E;enum Cyc_Relations_Relation relation=_tmp36F;struct Cyc_Absyn_Exp*e2=_tmp370;
union Cyc_Relations_RelnOp n1=({Cyc_Relations_RConst(0U);});
union Cyc_Relations_RelnOp n2=({Cyc_Relations_RConst(0U);});
# 2075
if(({Cyc_Relations_exp2relnop(e1,& n1);})&&({Cyc_Relations_exp2relnop(e2,& n2);})){
# 2077
struct Cyc_List_List*relns_true=({Cyc_Relations_add_relation(r,n1,relation,n2,relns);});
# 2081
struct Cyc_List_List*relns_false=({struct Cyc_List_List*(*_tmp376)(struct _RegionHandle*rgn,union Cyc_Relations_RelnOp rop1,enum Cyc_Relations_Relation r,union Cyc_Relations_RelnOp rop2,struct Cyc_List_List*relns)=Cyc_Relations_add_relation;struct _RegionHandle*_tmp377=r;union Cyc_Relations_RelnOp _tmp378=n2;enum Cyc_Relations_Relation _tmp379=({Cyc_Relations_flip_relation(relation);});union Cyc_Relations_RelnOp _tmp37A=n1;struct Cyc_List_List*_tmp37B=relns;_tmp376(_tmp377,_tmp378,_tmp379,_tmp37A,_tmp37B);});
struct _tuple20 _stmttmp72=({struct _tuple20 _tmp597;_tmp597.f1=ft,_tmp597.f2=ff;_tmp597;});struct _tuple20 _tmp371=_stmttmp72;struct Cyc_Dict_Dict _tmp372;struct Cyc_Dict_Dict _tmp373;struct Cyc_Dict_Dict _tmp375;struct Cyc_Dict_Dict _tmp374;if(((_tmp371.f1).ReachableFL).tag == 2){if(((_tmp371.f2).ReachableFL).tag == 2){_LL58: _tmp374=(((_tmp371.f1).ReachableFL).val).f1;_tmp375=(((_tmp371.f2).ReachableFL).val).f1;_LL59: {struct Cyc_Dict_Dict dt=_tmp374;struct Cyc_Dict_Dict df=_tmp375;
# 2084
return({struct _tuple20 _tmp593;({union Cyc_CfFlowInfo_FlowInfo _tmp6BF=({Cyc_CfFlowInfo_ReachableFL(dt,relns_true);});_tmp593.f1=_tmp6BF;}),({union Cyc_CfFlowInfo_FlowInfo _tmp6BE=({Cyc_CfFlowInfo_ReachableFL(df,relns_false);});_tmp593.f2=_tmp6BE;});_tmp593;});}}else{_LL5C: _tmp373=(((_tmp371.f1).ReachableFL).val).f1;_LL5D: {struct Cyc_Dict_Dict dt=_tmp373;
# 2088
return({struct _tuple20 _tmp594;({union Cyc_CfFlowInfo_FlowInfo _tmp6C0=({Cyc_CfFlowInfo_ReachableFL(dt,relns_true);});_tmp594.f1=_tmp6C0;}),_tmp594.f2=ff;_tmp594;});}}}else{if(((_tmp371.f2).ReachableFL).tag == 2){_LL5A: _tmp372=(((_tmp371.f2).ReachableFL).val).f1;_LL5B: {struct Cyc_Dict_Dict df=_tmp372;
# 2086
return({struct _tuple20 _tmp595;_tmp595.f1=ft,({union Cyc_CfFlowInfo_FlowInfo _tmp6C1=({Cyc_CfFlowInfo_ReachableFL(df,relns_false);});_tmp595.f2=_tmp6C1;});_tmp595;});}}else{_LL5E: _LL5F:
# 2090
 return({struct _tuple20 _tmp596;_tmp596.f1=ft,_tmp596.f2=ff;_tmp596;});}}_LL57:;}else{
# 2093
return({struct _tuple20 _tmp598;_tmp598.f1=ft,_tmp598.f2=ff;_tmp598;});}}}}}}_LL6:;}}}
# 2099
static struct _tuple20 Cyc_NewControlFlow_anal_test(struct Cyc_NewControlFlow_AnalEnv*env,union Cyc_CfFlowInfo_FlowInfo inflow,struct Cyc_Absyn_Exp*e){
struct Cyc_CfFlowInfo_FlowEnv*fenv=env->fenv;
void*_stmttmp73=e->r;void*_tmp37D=_stmttmp73;struct Cyc_List_List*_tmp37F;enum Cyc_Absyn_Primop _tmp37E;struct Cyc_Absyn_Exp*_tmp380;struct Cyc_Absyn_Exp*_tmp382;struct Cyc_Absyn_Exp*_tmp381;struct Cyc_Absyn_Exp*_tmp384;struct Cyc_Absyn_Exp*_tmp383;struct Cyc_Absyn_Exp*_tmp386;struct Cyc_Absyn_Exp*_tmp385;struct Cyc_Absyn_Exp*_tmp388;struct Cyc_Absyn_Exp*_tmp387;struct Cyc_Absyn_Exp*_tmp38B;struct Cyc_Absyn_Exp*_tmp38A;struct Cyc_Absyn_Exp*_tmp389;switch(*((int*)_tmp37D)){case 6U: _LL1: _tmp389=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp37D)->f1;_tmp38A=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp37D)->f2;_tmp38B=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp37D)->f3;_LL2: {struct Cyc_Absyn_Exp*e1=_tmp389;struct Cyc_Absyn_Exp*e2=_tmp38A;struct Cyc_Absyn_Exp*e3=_tmp38B;
# 2103
struct _tuple20 _stmttmp74=({Cyc_NewControlFlow_anal_test(env,inflow,e1);});union Cyc_CfFlowInfo_FlowInfo _tmp38D;union Cyc_CfFlowInfo_FlowInfo _tmp38C;_LL12: _tmp38C=_stmttmp74.f1;_tmp38D=_stmttmp74.f2;_LL13: {union Cyc_CfFlowInfo_FlowInfo f1t=_tmp38C;union Cyc_CfFlowInfo_FlowInfo f1f=_tmp38D;
struct _tuple20 _stmttmp75=({Cyc_NewControlFlow_anal_test(env,f1t,e2);});union Cyc_CfFlowInfo_FlowInfo _tmp38F;union Cyc_CfFlowInfo_FlowInfo _tmp38E;_LL15: _tmp38E=_stmttmp75.f1;_tmp38F=_stmttmp75.f2;_LL16: {union Cyc_CfFlowInfo_FlowInfo f2t=_tmp38E;union Cyc_CfFlowInfo_FlowInfo f2f=_tmp38F;
struct _tuple20 _stmttmp76=({Cyc_NewControlFlow_anal_test(env,f1f,e3);});union Cyc_CfFlowInfo_FlowInfo _tmp391;union Cyc_CfFlowInfo_FlowInfo _tmp390;_LL18: _tmp390=_stmttmp76.f1;_tmp391=_stmttmp76.f2;_LL19: {union Cyc_CfFlowInfo_FlowInfo f3t=_tmp390;union Cyc_CfFlowInfo_FlowInfo f3f=_tmp391;
return({struct _tuple20 _tmp599;({union Cyc_CfFlowInfo_FlowInfo _tmp6C3=({Cyc_CfFlowInfo_join_flow(fenv,f2t,f3t);});_tmp599.f1=_tmp6C3;}),({
union Cyc_CfFlowInfo_FlowInfo _tmp6C2=({Cyc_CfFlowInfo_join_flow(fenv,f2f,f3f);});_tmp599.f2=_tmp6C2;});_tmp599;});}}}}case 7U: _LL3: _tmp387=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp37D)->f1;_tmp388=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp37D)->f2;_LL4: {struct Cyc_Absyn_Exp*e1=_tmp387;struct Cyc_Absyn_Exp*e2=_tmp388;
# 2109
struct _tuple20 _stmttmp77=({Cyc_NewControlFlow_anal_test(env,inflow,e1);});union Cyc_CfFlowInfo_FlowInfo _tmp393;union Cyc_CfFlowInfo_FlowInfo _tmp392;_LL1B: _tmp392=_stmttmp77.f1;_tmp393=_stmttmp77.f2;_LL1C: {union Cyc_CfFlowInfo_FlowInfo f1t=_tmp392;union Cyc_CfFlowInfo_FlowInfo f1f=_tmp393;
struct _tuple20 _stmttmp78=({Cyc_NewControlFlow_anal_test(env,f1t,e2);});union Cyc_CfFlowInfo_FlowInfo _tmp395;union Cyc_CfFlowInfo_FlowInfo _tmp394;_LL1E: _tmp394=_stmttmp78.f1;_tmp395=_stmttmp78.f2;_LL1F: {union Cyc_CfFlowInfo_FlowInfo f2t=_tmp394;union Cyc_CfFlowInfo_FlowInfo f2f=_tmp395;
return({struct _tuple20 _tmp59A;_tmp59A.f1=f2t,({union Cyc_CfFlowInfo_FlowInfo _tmp6C4=({Cyc_CfFlowInfo_join_flow(fenv,f1f,f2f);});_tmp59A.f2=_tmp6C4;});_tmp59A;});}}}case 8U: _LL5: _tmp385=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp37D)->f1;_tmp386=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp37D)->f2;_LL6: {struct Cyc_Absyn_Exp*e1=_tmp385;struct Cyc_Absyn_Exp*e2=_tmp386;
# 2113
struct _tuple20 _stmttmp79=({Cyc_NewControlFlow_anal_test(env,inflow,e1);});union Cyc_CfFlowInfo_FlowInfo _tmp397;union Cyc_CfFlowInfo_FlowInfo _tmp396;_LL21: _tmp396=_stmttmp79.f1;_tmp397=_stmttmp79.f2;_LL22: {union Cyc_CfFlowInfo_FlowInfo f1t=_tmp396;union Cyc_CfFlowInfo_FlowInfo f1f=_tmp397;
struct _tuple20 _stmttmp7A=({Cyc_NewControlFlow_anal_test(env,f1f,e2);});union Cyc_CfFlowInfo_FlowInfo _tmp399;union Cyc_CfFlowInfo_FlowInfo _tmp398;_LL24: _tmp398=_stmttmp7A.f1;_tmp399=_stmttmp7A.f2;_LL25: {union Cyc_CfFlowInfo_FlowInfo f2t=_tmp398;union Cyc_CfFlowInfo_FlowInfo f2f=_tmp399;
return({struct _tuple20 _tmp59B;({union Cyc_CfFlowInfo_FlowInfo _tmp6C5=({Cyc_CfFlowInfo_join_flow(fenv,f1t,f2t);});_tmp59B.f1=_tmp6C5;}),_tmp59B.f2=f2f;_tmp59B;});}}}case 34U: _LL7: _tmp383=((struct Cyc_Absyn_Spawn_e_Absyn_Raw_exp_struct*)_tmp37D)->f1;_tmp384=((struct Cyc_Absyn_Spawn_e_Absyn_Raw_exp_struct*)_tmp37D)->f2;_LL8: {struct Cyc_Absyn_Exp*e1=_tmp383;struct Cyc_Absyn_Exp*e2=_tmp384;
# 2118
struct _tuple17 _stmttmp7B=({Cyc_NewControlFlow_anal_Rexp(env,0,inflow,e1);});void*_tmp39B;union Cyc_CfFlowInfo_FlowInfo _tmp39A;_LL27: _tmp39A=_stmttmp7B.f1;_tmp39B=_stmttmp7B.f2;_LL28: {union Cyc_CfFlowInfo_FlowInfo f=_tmp39A;void*r=_tmp39B;
return({Cyc_NewControlFlow_anal_test(env,f,e2);});}}case 9U: _LL9: _tmp381=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp37D)->f1;_tmp382=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp37D)->f2;_LLA: {struct Cyc_Absyn_Exp*e1=_tmp381;struct Cyc_Absyn_Exp*e2=_tmp382;
# 2121
struct _tuple17 _stmttmp7C=({Cyc_NewControlFlow_anal_Rexp(env,0,inflow,e1);});void*_tmp39D;union Cyc_CfFlowInfo_FlowInfo _tmp39C;_LL2A: _tmp39C=_stmttmp7C.f1;_tmp39D=_stmttmp7C.f2;_LL2B: {union Cyc_CfFlowInfo_FlowInfo f=_tmp39C;void*r=_tmp39D;
return({Cyc_NewControlFlow_anal_test(env,f,e2);});}}case 3U: if(((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp37D)->f1 == Cyc_Absyn_Not){if(((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp37D)->f2 != 0){if(((struct Cyc_List_List*)((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp37D)->f2)->tl == 0){_LLB: _tmp380=(struct Cyc_Absyn_Exp*)(((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp37D)->f2)->hd;_LLC: {struct Cyc_Absyn_Exp*e1=_tmp380;
# 2124
struct _tuple20 _stmttmp7D=({Cyc_NewControlFlow_anal_test(env,inflow,e1);});union Cyc_CfFlowInfo_FlowInfo _tmp39F;union Cyc_CfFlowInfo_FlowInfo _tmp39E;_LL2D: _tmp39E=_stmttmp7D.f1;_tmp39F=_stmttmp7D.f2;_LL2E: {union Cyc_CfFlowInfo_FlowInfo f1=_tmp39E;union Cyc_CfFlowInfo_FlowInfo f2=_tmp39F;
return({struct _tuple20 _tmp59C;_tmp59C.f1=f2,_tmp59C.f2=f1;_tmp59C;});}}}else{goto _LLD;}}else{goto _LLD;}}else{_LLD: _tmp37E=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp37D)->f1;_tmp37F=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp37D)->f2;_LLE: {enum Cyc_Absyn_Primop p=_tmp37E;struct Cyc_List_List*es=_tmp37F;
# 2127
return({Cyc_NewControlFlow_anal_primop_test(env,inflow,p,es);});}}default: _LLF: _LL10: {
# 2131
struct _tuple17 _stmttmp7E=({Cyc_NewControlFlow_anal_Rexp(env,0,inflow,e);});void*_tmp3A1;union Cyc_CfFlowInfo_FlowInfo _tmp3A0;_LL30: _tmp3A0=_stmttmp7E.f1;_tmp3A1=_stmttmp7E.f2;_LL31: {union Cyc_CfFlowInfo_FlowInfo f=_tmp3A0;void*r=_tmp3A1;
union Cyc_CfFlowInfo_FlowInfo _tmp3A2=f;struct Cyc_Dict_Dict _tmp3A3;if((_tmp3A2.BottomFL).tag == 1){_LL33: _LL34:
 return({struct _tuple20 _tmp59D;_tmp59D.f1=f,_tmp59D.f2=f;_tmp59D;});}else{_LL35: _tmp3A3=((_tmp3A2.ReachableFL).val).f1;_LL36: {struct Cyc_Dict_Dict d=_tmp3A3;
# 2135
struct _tuple16 _stmttmp7F=({Cyc_CfFlowInfo_unname_rval(r);});struct Cyc_List_List*_tmp3A5;void*_tmp3A4;_LL38: _tmp3A4=_stmttmp7F.f1;_tmp3A5=_stmttmp7F.f2;_LL39: {void*r=_tmp3A4;struct Cyc_List_List*names=_tmp3A5;
void*_tmp3A6=r;enum Cyc_CfFlowInfo_InitLevel _tmp3A7;void*_tmp3A9;struct Cyc_Absyn_Vardecl*_tmp3A8;switch(*((int*)_tmp3A6)){case 8U: _LL3B: _tmp3A8=((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp3A6)->f1;_tmp3A9=(void*)((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp3A6)->f2;_LL3C: {struct Cyc_Absyn_Vardecl*n=_tmp3A8;void*r2=_tmp3A9;
# 2138
({void*_tmp3AA=0U;({int(*_tmp6C7)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp3AC)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp3AC;});struct _fat_ptr _tmp6C6=({const char*_tmp3AB="anal_test: bad namedlocation";_tag_fat(_tmp3AB,sizeof(char),29U);});_tmp6C7(_tmp6C6,_tag_fat(_tmp3AA,sizeof(void*),0U));});});}case 0U: _LL3D: _LL3E:
 return({struct _tuple20 _tmp59E;({union Cyc_CfFlowInfo_FlowInfo _tmp6C8=({Cyc_CfFlowInfo_BottomFL();});_tmp59E.f1=_tmp6C8;}),_tmp59E.f2=f;_tmp59E;});case 1U: _LL3F: _LL40:
 goto _LL42;case 5U: _LL41: _LL42:
 goto _LL44;case 4U: _LL43: _LL44:
 return({struct _tuple20 _tmp59F;_tmp59F.f1=f,({union Cyc_CfFlowInfo_FlowInfo _tmp6C9=({Cyc_CfFlowInfo_BottomFL();});_tmp59F.f2=_tmp6C9;});_tmp59F;});case 2U: if(((struct Cyc_CfFlowInfo_UnknownR_CfFlowInfo_AbsRVal_struct*)_tmp3A6)->f1 == Cyc_CfFlowInfo_NoneIL){_LL45: _LL46:
 goto _LL48;}else{_LL4B: _tmp3A7=((struct Cyc_CfFlowInfo_UnknownR_CfFlowInfo_AbsRVal_struct*)_tmp3A6)->f1;_LL4C: {enum Cyc_CfFlowInfo_InitLevel il=_tmp3A7;
# 2148
return({Cyc_NewControlFlow_splitzero(env,inflow,f,e,il,names);});}}case 3U: if(((struct Cyc_CfFlowInfo_Esc_CfFlowInfo_AbsRVal_struct*)_tmp3A6)->f1 == Cyc_CfFlowInfo_NoneIL){_LL47: _LL48:
# 2144
 goto _LL4A;}else{_LL4D: _LL4E:
# 2149
 return({struct _tuple20 _tmp5A0;_tmp5A0.f1=f,_tmp5A0.f2=f;_tmp5A0;});}case 7U: _LL49: _LL4A:
# 2146
({void*_tmp3AD=0U;({unsigned _tmp6CB=e->loc;struct _fat_ptr _tmp6CA=({const char*_tmp3AE="expression may not be initialized";_tag_fat(_tmp3AE,sizeof(char),34U);});Cyc_CfFlowInfo_aerr(_tmp6CB,_tmp6CA,_tag_fat(_tmp3AD,sizeof(void*),0U));});});
return({struct _tuple20 _tmp5A1;({union Cyc_CfFlowInfo_FlowInfo _tmp6CD=({Cyc_CfFlowInfo_BottomFL();});_tmp5A1.f1=_tmp6CD;}),({union Cyc_CfFlowInfo_FlowInfo _tmp6CC=({Cyc_CfFlowInfo_BottomFL();});_tmp5A1.f2=_tmp6CC;});_tmp5A1;});default: _LL4F: _LL50:
# 2150
({void*_tmp3AF=0U;({int(*_tmp6CF)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp3B1)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp3B1;});struct _fat_ptr _tmp6CE=({const char*_tmp3B0="anal_test";_tag_fat(_tmp3B0,sizeof(char),10U);});_tmp6CF(_tmp6CE,_tag_fat(_tmp3AF,sizeof(void*),0U));});});}_LL3A:;}}}_LL32:;}}}_LL0:;}struct _tuple29{unsigned f1;struct Cyc_NewControlFlow_AnalEnv*f2;struct Cyc_Dict_Dict f3;};
# 2156
static void Cyc_NewControlFlow_check_for_unused_unique(struct _tuple29*ckenv,void*root,void*rval){
# 2158
struct Cyc_Dict_Dict _tmp3B5;struct Cyc_NewControlFlow_AnalEnv*_tmp3B4;unsigned _tmp3B3;_LL1: _tmp3B3=ckenv->f1;_tmp3B4=ckenv->f2;_tmp3B5=ckenv->f3;_LL2: {unsigned loc=_tmp3B3;struct Cyc_NewControlFlow_AnalEnv*env=_tmp3B4;struct Cyc_Dict_Dict fd=_tmp3B5;
void*_tmp3B6=root;struct Cyc_Absyn_Vardecl*_tmp3B7;if(((struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct*)_tmp3B6)->tag == 0U){_LL4: _tmp3B7=((struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct*)_tmp3B6)->f1;_LL5: {struct Cyc_Absyn_Vardecl*vd=_tmp3B7;
# 2161
if(({Cyc_Tcutil_is_noalias_pointer_or_aggr(vd->type);})){
struct _tuple16 _stmttmp80=({Cyc_CfFlowInfo_unname_rval(rval);});void*_tmp3B8;_LL9: _tmp3B8=_stmttmp80.f1;_LLA: {void*rval=_tmp3B8;
void*_tmp3B9=rval;switch(*((int*)_tmp3B9)){case 7U: _LLC: _LLD:
 goto _LLF;case 0U: _LLE: _LLF:
 goto _LL11;case 2U: if(((struct Cyc_CfFlowInfo_UnknownR_CfFlowInfo_AbsRVal_struct*)_tmp3B9)->f1 == Cyc_CfFlowInfo_NoneIL){_LL10: _LL11:
 goto _LLB;}else{goto _LL12;}default: _LL12: _LL13:
# 2168
({struct Cyc_String_pa_PrintArg_struct _tmp3BC=({struct Cyc_String_pa_PrintArg_struct _tmp5A2;_tmp5A2.tag=0U,({struct _fat_ptr _tmp6D0=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(vd->name);}));_tmp5A2.f1=_tmp6D0;});_tmp5A2;});void*_tmp3BA[1U];_tmp3BA[0]=& _tmp3BC;({unsigned _tmp6D2=loc;struct _fat_ptr _tmp6D1=({const char*_tmp3BB="unique pointers reachable from %s may become unreachable";_tag_fat(_tmp3BB,sizeof(char),57U);});Cyc_Warn_warn(_tmp6D2,_tmp6D1,_tag_fat(_tmp3BA,sizeof(void*),1U));});});
goto _LLB;}_LLB:;}}
# 2161
goto _LL3;}}else{_LL6: _LL7:
# 2173
 goto _LL3;}_LL3:;}}
# 2177
static void Cyc_NewControlFlow_check_init_params(unsigned loc,struct Cyc_NewControlFlow_AnalEnv*env,union Cyc_CfFlowInfo_FlowInfo flow){
union Cyc_CfFlowInfo_FlowInfo _tmp3BE=flow;struct Cyc_Dict_Dict _tmp3BF;if((_tmp3BE.BottomFL).tag == 1){_LL1: _LL2:
 return;}else{_LL3: _tmp3BF=((_tmp3BE.ReachableFL).val).f1;_LL4: {struct Cyc_Dict_Dict d=_tmp3BF;
# 2181
{struct Cyc_List_List*inits=env->param_roots;for(0;inits != 0;inits=inits->tl){
if((int)({enum Cyc_CfFlowInfo_InitLevel(*_tmp3C0)(struct Cyc_CfFlowInfo_FlowEnv*,struct Cyc_Dict_Dict d,void*r)=Cyc_CfFlowInfo_initlevel;struct Cyc_CfFlowInfo_FlowEnv*_tmp3C1=env->fenv;struct Cyc_Dict_Dict _tmp3C2=d;void*_tmp3C3=({Cyc_CfFlowInfo_lookup_place(d,(struct Cyc_CfFlowInfo_Place*)inits->hd);});_tmp3C0(_tmp3C1,_tmp3C2,_tmp3C3);})!= (int)Cyc_CfFlowInfo_AllIL)
({void*_tmp3C4=0U;({unsigned _tmp6D4=loc;struct _fat_ptr _tmp6D3=({const char*_tmp3C5="function may not initialize all the parameters with attribute 'initializes'";_tag_fat(_tmp3C5,sizeof(char),76U);});Cyc_CfFlowInfo_aerr(_tmp6D4,_tmp6D3,_tag_fat(_tmp3C4,sizeof(void*),0U));});});}}
# 2181
if(Cyc_NewControlFlow_warn_lose_unique){
# 2186
struct _tuple29 check_env=({struct _tuple29 _tmp5A3;_tmp5A3.f1=loc,_tmp5A3.f2=env,_tmp5A3.f3=d;_tmp5A3;});
({({void(*_tmp6D5)(void(*f)(struct _tuple29*,void*,void*),struct _tuple29*env,struct Cyc_Dict_Dict d)=({void(*_tmp3C6)(void(*f)(struct _tuple29*,void*,void*),struct _tuple29*env,struct Cyc_Dict_Dict d)=(void(*)(void(*f)(struct _tuple29*,void*,void*),struct _tuple29*env,struct Cyc_Dict_Dict d))Cyc_Dict_iter_c;_tmp3C6;});_tmp6D5(Cyc_NewControlFlow_check_for_unused_unique,& check_env,d);});});}
# 2181
return;}}_LL0:;}
# 2199
static struct _tuple1 Cyc_NewControlFlow_get_unconsume_pat_vars(struct Cyc_List_List*vds){
struct Cyc_List_List*roots=0;
struct Cyc_List_List*es=0;
{struct Cyc_List_List*x=vds;for(0;x != 0;x=x->tl){
struct _tuple25*_stmttmp81=(struct _tuple25*)x->hd;struct Cyc_Absyn_Exp*_tmp3C9;struct Cyc_Absyn_Vardecl**_tmp3C8;_LL1: _tmp3C8=_stmttmp81->f1;_tmp3C9=_stmttmp81->f2;_LL2: {struct Cyc_Absyn_Vardecl**vopt=_tmp3C8;struct Cyc_Absyn_Exp*eopt=_tmp3C9;
if((vopt != 0 && eopt != 0)&&({Cyc_Tcutil_is_noalias_pointer((void*)_check_null(eopt->topt),0);})){
# 2206
struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct*root=({struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct*_tmp3CD=_cycalloc(sizeof(*_tmp3CD));_tmp3CD->tag=0U,_tmp3CD->f1=*vopt;_tmp3CD;});
struct Cyc_CfFlowInfo_Place*rp=({struct Cyc_CfFlowInfo_Place*_tmp3CC=_cycalloc(sizeof(*_tmp3CC));_tmp3CC->root=(void*)root,_tmp3CC->path=0;_tmp3CC;});
roots=({struct Cyc_List_List*_tmp3CA=_cycalloc(sizeof(*_tmp3CA));_tmp3CA->hd=rp,_tmp3CA->tl=roots;_tmp3CA;});
es=({struct Cyc_List_List*_tmp3CB=_cycalloc(sizeof(*_tmp3CB));_tmp3CB->hd=eopt,_tmp3CB->tl=es;_tmp3CB;});}}}}
# 2212
return({struct _tuple1 _tmp5A4;_tmp5A4.f1=roots,_tmp5A4.f2=es;_tmp5A4;});}struct _tuple30{int f1;void*f2;};
# 2218
static struct _tuple30 Cyc_NewControlFlow_noconsume_place_ok(struct Cyc_NewControlFlow_AnalEnv*env,struct Cyc_CfFlowInfo_Place*place,int do_unconsume,struct Cyc_Absyn_Vardecl*vd,union Cyc_CfFlowInfo_FlowInfo flow,unsigned loc){
# 2225
union Cyc_CfFlowInfo_FlowInfo _tmp3CF=flow;struct Cyc_Dict_Dict _tmp3D0;if((_tmp3CF.BottomFL).tag == 1){_LL1: _LL2:
({void*_tmp3D1=0U;({int(*_tmp6D7)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp3D3)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp3D3;});struct _fat_ptr _tmp6D6=({const char*_tmp3D2="noconsume_place_ok: flow became Bottom!";_tag_fat(_tmp3D2,sizeof(char),40U);});_tmp6D7(_tmp6D6,_tag_fat(_tmp3D1,sizeof(void*),0U));});});}else{_LL3: _tmp3D0=((_tmp3CF.ReachableFL).val).f1;_LL4: {struct Cyc_Dict_Dict d=_tmp3D0;
# 2233
struct Cyc_Absyn_Exp*bogus_exp=({Cyc_Absyn_uint_exp(1U,0U);});
int bogus_bool=0;
int bogus_int=1;
void*curr_rval=({Cyc_CfFlowInfo_lookup_place(d,place);});
void*rval=curr_rval;
# 2245
int varok=0;
{void*_tmp3D4=curr_rval;void*_tmp3D6;struct Cyc_Absyn_Vardecl*_tmp3D5;if(((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp3D4)->tag == 8U){_LL6: _tmp3D5=((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp3D4)->f1;_tmp3D6=(void*)((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp3D4)->f2;_LL7: {struct Cyc_Absyn_Vardecl*n=_tmp3D5;void*r=_tmp3D6;
# 2248
if(vd == n){
rval=r;
# 2251
if(({Cyc_Tcutil_is_noalias_pointer_or_aggr(vd->type);})){
# 2253
if(({Cyc_CfFlowInfo_is_unique_consumed(bogus_exp,bogus_int,rval,& bogus_bool);})){
if(!do_unconsume)
({struct Cyc_String_pa_PrintArg_struct _tmp3D9=({struct Cyc_String_pa_PrintArg_struct _tmp5A5;_tmp5A5.tag=0U,({
# 2257
struct _fat_ptr _tmp6D8=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(vd->name);}));_tmp5A5.f1=_tmp6D8;});_tmp5A5;});void*_tmp3D7[1U];_tmp3D7[0]=& _tmp3D9;({unsigned _tmp6DA=loc;struct _fat_ptr _tmp6D9=({const char*_tmp3D8="function consumes parameter %s which does not have the 'consume' attribute";_tag_fat(_tmp3D8,sizeof(char),75U);});Cyc_CfFlowInfo_aerr(_tmp6DA,_tmp6D9,_tag_fat(_tmp3D7,sizeof(void*),1U));});});}else{
# 2260
if((int)({Cyc_CfFlowInfo_initlevel(env->fenv,d,rval);})!= (int)Cyc_CfFlowInfo_AllIL && !do_unconsume)
({struct Cyc_String_pa_PrintArg_struct _tmp3DC=({struct Cyc_String_pa_PrintArg_struct _tmp5A6;_tmp5A6.tag=0U,({
# 2263
struct _fat_ptr _tmp6DB=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(vd->name);}));_tmp5A6.f1=_tmp6DB;});_tmp5A6;});void*_tmp3DA[1U];_tmp3DA[0]=& _tmp3DC;({unsigned _tmp6DD=loc;struct _fat_ptr _tmp6DC=({const char*_tmp3DB="function consumes value pointed to by parameter %s, which does not have the 'consume' attribute";_tag_fat(_tmp3DB,sizeof(char),96U);});Cyc_CfFlowInfo_aerr(_tmp6DD,_tmp6DC,_tag_fat(_tmp3DA,sizeof(void*),1U));});});else{
# 2265
varok=1;}}}else{
# 2268
varok=1;}}else{
# 2271
goto _LL9;}
goto _LL5;}}else{_LL8: _LL9:
# 2275
 if(!({Cyc_Tcutil_is_noalias_pointer_or_aggr(vd->type);}))
varok=1;else{
if(!do_unconsume)
({struct Cyc_String_pa_PrintArg_struct _tmp3DF=({struct Cyc_String_pa_PrintArg_struct _tmp5A7;_tmp5A7.tag=0U,({
# 2280
struct _fat_ptr _tmp6DE=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(vd->name);}));_tmp5A7.f1=_tmp6DE;});_tmp5A7;});void*_tmp3DD[1U];_tmp3DD[0]=& _tmp3DF;({unsigned _tmp6E0=loc;struct _fat_ptr _tmp6DF=({const char*_tmp3DE="function parameter %s without 'consume' attribute no longer set to its original value";_tag_fat(_tmp3DE,sizeof(char),86U);});Cyc_CfFlowInfo_aerr(_tmp6E0,_tmp6DF,_tag_fat(_tmp3DD,sizeof(void*),1U));});});}
# 2275
goto _LL5;}_LL5:;}
# 2287
return({struct _tuple30 _tmp5A8;_tmp5A8.f1=varok,_tmp5A8.f2=rval;_tmp5A8;});}}_LL0:;}
# 2293
static struct Cyc_Absyn_Vardecl*Cyc_NewControlFlow_get_vd_from_place(struct Cyc_CfFlowInfo_Place*p){
struct Cyc_List_List*_tmp3E2;void*_tmp3E1;_LL1: _tmp3E1=p->root;_tmp3E2=p->path;_LL2: {void*root=_tmp3E1;struct Cyc_List_List*fs=_tmp3E2;
if(fs != 0)
({void*_tmp3E3=0U;({int(*_tmp6E2)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp3E5)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp3E5;});struct _fat_ptr _tmp6E1=({const char*_tmp3E4="unconsume_params: param to unconsume is non-variable";_tag_fat(_tmp3E4,sizeof(char),53U);});_tmp6E2(_tmp6E1,_tag_fat(_tmp3E3,sizeof(void*),0U));});});{
# 2295
struct Cyc_Absyn_Vardecl*vd;
# 2298
void*_tmp3E6=root;struct Cyc_Absyn_Vardecl*_tmp3E7;if(((struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct*)_tmp3E6)->tag == 0U){_LL4: _tmp3E7=((struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct*)_tmp3E6)->f1;_LL5: {struct Cyc_Absyn_Vardecl*vd=_tmp3E7;
return vd;}}else{_LL6: _LL7:
({void*_tmp3E8=0U;({int(*_tmp6E4)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp3EA)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp3EA;});struct _fat_ptr _tmp6E3=({const char*_tmp3E9="unconsume_params: root is not a varroot";_tag_fat(_tmp3E9,sizeof(char),40U);});_tmp6E4(_tmp6E3,_tag_fat(_tmp3E8,sizeof(void*),0U));});});}_LL3:;}}}
# 2312 "new_control_flow.cyc"
static union Cyc_CfFlowInfo_FlowInfo Cyc_NewControlFlow_unconsume_exp(struct Cyc_NewControlFlow_AnalEnv*env,struct Cyc_Absyn_Exp*unconsume_exp,struct Cyc_Absyn_Vardecl*vd,int varok,void*ropt,union Cyc_CfFlowInfo_FlowInfo flow,unsigned loc){
# 2320
{union Cyc_CfFlowInfo_FlowInfo _tmp3EC=flow;struct Cyc_Dict_Dict _tmp3ED;if((_tmp3EC.BottomFL).tag == 1){_LL1: _LL2:
 return flow;}else{_LL3: _tmp3ED=((_tmp3EC.ReachableFL).val).f1;_LL4: {struct Cyc_Dict_Dict d=_tmp3ED;
# 2327
struct _tuple19 _stmttmp82=({Cyc_NewControlFlow_anal_Lexp(env,flow,0,1,unconsume_exp);});union Cyc_CfFlowInfo_AbsLVal _tmp3EF;union Cyc_CfFlowInfo_FlowInfo _tmp3EE;_LL6: _tmp3EE=_stmttmp82.f1;_tmp3EF=_stmttmp82.f2;_LL7: {union Cyc_CfFlowInfo_FlowInfo f=_tmp3EE;union Cyc_CfFlowInfo_AbsLVal lval=_tmp3EF;
# 2330
{union Cyc_CfFlowInfo_AbsLVal _tmp3F0=lval;struct Cyc_CfFlowInfo_Place*_tmp3F1;if((_tmp3F0.PlaceL).tag == 1){_LL9: _tmp3F1=(_tmp3F0.PlaceL).val;_LLA: {struct Cyc_CfFlowInfo_Place*p=_tmp3F1;
# 2334
void*old_rval=({Cyc_CfFlowInfo_lookup_place(d,p);});
{void*_tmp3F2=old_rval;void*_tmp3F4;struct Cyc_Absyn_Vardecl*_tmp3F3;if(((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp3F2)->tag == 8U){_LLE: _tmp3F3=((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp3F2)->f1;_tmp3F4=(void*)((struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*)_tmp3F2)->f2;_LLF: {struct Cyc_Absyn_Vardecl*old_vd=_tmp3F3;void*r=_tmp3F4;
# 2340
void*new_rval;
if(old_vd == vd){
# 2344
if(varok){
# 2346
old_rval=({Cyc_CfFlowInfo_make_unique_unconsumed(env->fenv,r);});
# 2351
if(ropt != 0){
# 2357
struct _tuple17 _stmttmp83=({({struct Cyc_CfFlowInfo_FlowEnv*_tmp6E6=env->fenv;struct _tuple17 _tmp6E5=({struct _tuple17 _tmp5A9;_tmp5A9.f1=f,_tmp5A9.f2=old_rval;_tmp5A9;});Cyc_CfFlowInfo_join_flow_and_rval(_tmp6E6,_tmp6E5,({struct _tuple17 _tmp5AA;_tmp5AA.f1=f,_tmp5AA.f2=ropt;_tmp5AA;}));});});void*_tmp3F6;union Cyc_CfFlowInfo_FlowInfo _tmp3F5;_LL13: _tmp3F5=_stmttmp83.f1;_tmp3F6=_stmttmp83.f2;_LL14: {union Cyc_CfFlowInfo_FlowInfo f2=_tmp3F5;void*new_rval2=_tmp3F6;
# 2361
f=f2;new_rval=new_rval2;}}else{
# 2366
new_rval=old_rval;}}else{
# 2369
new_rval=r;}
# 2371
{union Cyc_CfFlowInfo_FlowInfo _tmp3F7=f;struct Cyc_List_List*_tmp3F9;struct Cyc_Dict_Dict _tmp3F8;if((_tmp3F7.ReachableFL).tag == 2){_LL16: _tmp3F8=((_tmp3F7.ReachableFL).val).f1;_tmp3F9=((_tmp3F7.ReachableFL).val).f2;_LL17: {struct Cyc_Dict_Dict d2=_tmp3F8;struct Cyc_List_List*relns=_tmp3F9;
# 2373
flow=({union Cyc_CfFlowInfo_FlowInfo(*_tmp3FA)(struct Cyc_Dict_Dict,struct Cyc_List_List*)=Cyc_CfFlowInfo_ReachableFL;struct Cyc_Dict_Dict _tmp3FB=({Cyc_CfFlowInfo_assign_place(env->fenv,loc,d2,p,new_rval);});struct Cyc_List_List*_tmp3FC=relns;_tmp3FA(_tmp3FB,_tmp3FC);});
# 2377
goto _LL15;}}else{_LL18: _LL19:
# 2379
({void*_tmp3FD=0U;({int(*_tmp6E8)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp3FF)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp3FF;});struct _fat_ptr _tmp6E7=({const char*_tmp3FE="unconsume_params: joined flow became bot!";_tag_fat(_tmp3FE,sizeof(char),42U);});_tmp6E8(_tmp6E7,_tag_fat(_tmp3FD,sizeof(void*),0U));});});}_LL15:;}
# 2381
goto _LLD;}else{
# 2383
goto _LL11;}
goto _LLD;}}else{_LL10: _LL11:
# 2390
 if(ropt != 0 && !({Cyc_Tcutil_is_noalias_pointer_or_aggr(vd->type);}))
# 2392
({struct Cyc_String_pa_PrintArg_struct _tmp402=({struct Cyc_String_pa_PrintArg_struct _tmp5AB;_tmp5AB.tag=0U,({
struct _fat_ptr _tmp6E9=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_exp2string(unconsume_exp);}));_tmp5AB.f1=_tmp6E9;});_tmp5AB;});void*_tmp400[1U];_tmp400[0]=& _tmp402;({unsigned _tmp6EB=loc;struct _fat_ptr _tmp6EA=({const char*_tmp401="aliased expression %s was overwritten";_tag_fat(_tmp401,sizeof(char),38U);});Cyc_CfFlowInfo_aerr(_tmp6EB,_tmp6EA,_tag_fat(_tmp400,sizeof(void*),1U));});});
# 2390
goto _LLD;}_LLD:;}
# 2409 "new_control_flow.cyc"
goto _LL8;}}else{_LLB: _LLC:
# 2415
 goto _LL8;}_LL8:;}
# 2417
goto _LL0;}}}_LL0:;}
# 2419
return flow;}
# 2432 "new_control_flow.cyc"
static union Cyc_CfFlowInfo_FlowInfo Cyc_NewControlFlow_unconsume_params(struct Cyc_NewControlFlow_AnalEnv*env,struct Cyc_List_List*consumed_vals,struct Cyc_List_List*unconsume_exps,int is_region_open,union Cyc_CfFlowInfo_FlowInfo flow,unsigned loc){
# 2438
{union Cyc_CfFlowInfo_FlowInfo _tmp404=flow;if((_tmp404.BottomFL).tag == 1){_LL1: _LL2:
 return flow;}else{_LL3: _LL4:
 goto _LL0;}_LL0:;}{
# 2442
int do_unconsume=unconsume_exps != 0;
struct Cyc_List_List*_tmp405=({Cyc_List_rev(consumed_vals);});struct Cyc_List_List*consumed_vals=_tmp405;
struct Cyc_List_List*_tmp406=({Cyc_List_rev(unconsume_exps);});struct Cyc_List_List*unconsume_exps=_tmp406;
{struct Cyc_List_List*params=consumed_vals;for(0;params != 0;(
params=params->tl,
unconsume_exps != 0?unconsume_exps=unconsume_exps->tl: 0)){
# 2451
struct Cyc_Absyn_Vardecl*vd=({Cyc_NewControlFlow_get_vd_from_place((struct Cyc_CfFlowInfo_Place*)params->hd);});
struct _tuple30 _stmttmp84=
is_region_open?({struct _tuple30 _tmp5AC;_tmp5AC.f1=1,_tmp5AC.f2=0;_tmp5AC;}):({Cyc_NewControlFlow_noconsume_place_ok(env,(struct Cyc_CfFlowInfo_Place*)params->hd,do_unconsume,vd,flow,loc);});
# 2452
void*_tmp408;int _tmp407;_LL6: _tmp407=_stmttmp84.f1;_tmp408=_stmttmp84.f2;_LL7: {int varok=_tmp407;void*rval=_tmp408;
# 2458
if(do_unconsume)
flow=({Cyc_NewControlFlow_unconsume_exp(env,(struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(unconsume_exps))->hd,vd,varok,rval,flow,loc);});}}}
# 2461
({Cyc_NewControlFlow_update_tryflow(env,flow);});
return flow;}}struct _tuple31{int f1;struct Cyc_Absyn_Stmt*f2;struct Cyc_List_List*f3;struct Cyc_List_List*f4;};
# 2465
static union Cyc_CfFlowInfo_FlowInfo Cyc_NewControlFlow_anal_scs(struct Cyc_NewControlFlow_AnalEnv*env,union Cyc_CfFlowInfo_FlowInfo inflow,struct Cyc_List_List*scs,unsigned exp_loc){
# 2467
struct Cyc_CfFlowInfo_FlowEnv*fenv=env->fenv;
for(0;scs != 0;scs=scs->tl){
struct Cyc_Absyn_Switch_clause*_stmttmp85=(struct Cyc_Absyn_Switch_clause*)scs->hd;unsigned _tmp40D;struct Cyc_Absyn_Stmt*_tmp40C;struct Cyc_Absyn_Exp*_tmp40B;struct Cyc_Core_Opt*_tmp40A;_LL1: _tmp40A=_stmttmp85->pat_vars;_tmp40B=_stmttmp85->where_clause;_tmp40C=_stmttmp85->body;_tmp40D=_stmttmp85->loc;_LL2: {struct Cyc_Core_Opt*vds_opt=_tmp40A;struct Cyc_Absyn_Exp*where_opt=_tmp40B;struct Cyc_Absyn_Stmt*body=_tmp40C;unsigned loc=_tmp40D;
struct _tuple1 _stmttmp86=({Cyc_NewControlFlow_get_unconsume_pat_vars((struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(vds_opt))->v);});struct Cyc_List_List*_tmp40F;struct Cyc_List_List*_tmp40E;_LL4: _tmp40E=_stmttmp86.f1;_tmp40F=_stmttmp86.f2;_LL5: {struct Cyc_List_List*roots=_tmp40E;struct Cyc_List_List*es=_tmp40F;
union Cyc_CfFlowInfo_FlowInfo clause_inflow=({Cyc_NewControlFlow_initialize_pat_vars(env->fenv,env,inflow,(struct Cyc_List_List*)vds_opt->v,roots != 0,loc,exp_loc);});
# 2475
union Cyc_CfFlowInfo_FlowInfo clause_outflow;
struct Cyc_List_List*old_unique_pat_vars=env->unique_pat_vars;
# 2478
if(({Cyc_Tcpat_has_vars(vds_opt);}))
({struct Cyc_List_List*_tmp6ED=({struct Cyc_List_List*_tmp411=_cycalloc(sizeof(*_tmp411));
({struct _tuple31*_tmp6EC=({struct _tuple31*_tmp410=_cycalloc(sizeof(*_tmp410));_tmp410->f1=0,_tmp410->f2=body,_tmp410->f3=roots,_tmp410->f4=es;_tmp410;});_tmp411->hd=_tmp6EC;}),_tmp411->tl=old_unique_pat_vars;_tmp411;});
# 2479
env->unique_pat_vars=_tmp6ED;});
# 2478
if(where_opt != 0){
# 2483
struct Cyc_Absyn_Exp*wexp=where_opt;
struct _tuple20 _stmttmp87=({Cyc_NewControlFlow_anal_test(env,clause_inflow,wexp);});union Cyc_CfFlowInfo_FlowInfo _tmp413;union Cyc_CfFlowInfo_FlowInfo _tmp412;_LL7: _tmp412=_stmttmp87.f1;_tmp413=_stmttmp87.f2;_LL8: {union Cyc_CfFlowInfo_FlowInfo ft=_tmp412;union Cyc_CfFlowInfo_FlowInfo ff=_tmp413;
inflow=ff;
clause_outflow=({Cyc_NewControlFlow_anal_stmt(env,ft,body,0);});}}else{
# 2488
clause_outflow=({Cyc_NewControlFlow_anal_stmt(env,clause_inflow,body,0);});}
# 2490
env->unique_pat_vars=old_unique_pat_vars;{
union Cyc_CfFlowInfo_FlowInfo _tmp414=clause_outflow;if((_tmp414.BottomFL).tag == 1){_LLA: _LLB:
 goto _LL9;}else{_LLC: _LLD:
# 2495
 clause_outflow=({Cyc_NewControlFlow_unconsume_params(env,roots,es,0,clause_outflow,loc);});
# 2497
if(scs->tl == 0)
return clause_outflow;else{
# 2502
if((struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(((struct Cyc_Absyn_Switch_clause*)((struct Cyc_List_List*)_check_null(scs->tl))->hd)->pat_vars))->v != 0)
({void*_tmp415=0U;({unsigned _tmp6EF=body->loc;struct _fat_ptr _tmp6EE=({const char*_tmp416="switch clause may implicitly fallthru";_tag_fat(_tmp416,sizeof(char),38U);});Cyc_CfFlowInfo_aerr(_tmp6EF,_tmp6EE,_tag_fat(_tmp415,sizeof(void*),0U));});});else{
# 2506
({void*_tmp417=0U;({unsigned _tmp6F1=body->loc;struct _fat_ptr _tmp6F0=({const char*_tmp418="switch clause may implicitly fallthru";_tag_fat(_tmp418,sizeof(char),38U);});Cyc_CfFlowInfo_aerr(_tmp6F1,_tmp6F0,_tag_fat(_tmp417,sizeof(void*),0U));});});}
# 2510
({Cyc_NewControlFlow_update_flow(env,((struct Cyc_Absyn_Switch_clause*)((struct Cyc_List_List*)_check_null(scs->tl))->hd)->body,clause_outflow);});}
# 2512
goto _LL9;}_LL9:;}}}}
# 2515
return({Cyc_CfFlowInfo_BottomFL();});}struct _tuple32{struct Cyc_NewControlFlow_AnalEnv*f1;struct Cyc_Dict_Dict f2;unsigned f3;};
# 2520
static void Cyc_NewControlFlow_check_dropped_unique_vd(struct _tuple32*vdenv,struct Cyc_Absyn_Vardecl*vd){
# 2522
unsigned _tmp41C;struct Cyc_Dict_Dict _tmp41B;struct Cyc_NewControlFlow_AnalEnv*_tmp41A;_LL1: _tmp41A=vdenv->f1;_tmp41B=vdenv->f2;_tmp41C=vdenv->f3;_LL2: {struct Cyc_NewControlFlow_AnalEnv*env=_tmp41A;struct Cyc_Dict_Dict fd=_tmp41B;unsigned loc=_tmp41C;
if(({Cyc_Tcutil_is_noalias_pointer_or_aggr(vd->type);})){
# 2525
struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct vdroot=({struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct _tmp5AE;_tmp5AE.tag=0U,_tmp5AE.f1=vd;_tmp5AE;});
struct _tuple16 _stmttmp88=({struct _tuple16(*_tmp422)(void*rv)=Cyc_CfFlowInfo_unname_rval;void*_tmp423=({({struct Cyc_Dict_Dict _tmp6F2=fd;Cyc_Dict_lookup_other(_tmp6F2,Cyc_CfFlowInfo_root_cmp,(void*)({struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct*_tmp424=_cycalloc(sizeof(*_tmp424));*_tmp424=vdroot;_tmp424;}));});});_tmp422(_tmp423);});void*_tmp41D;_LL4: _tmp41D=_stmttmp88.f1;_LL5: {void*rval=_tmp41D;
void*_tmp41E=rval;switch(*((int*)_tmp41E)){case 7U: _LL7: _LL8:
 goto _LLA;case 0U: _LL9: _LLA:
 goto _LLC;case 2U: if(((struct Cyc_CfFlowInfo_UnknownR_CfFlowInfo_AbsRVal_struct*)_tmp41E)->f1 == Cyc_CfFlowInfo_NoneIL){_LLB: _LLC:
 goto _LL6;}else{goto _LLD;}default: _LLD: _LLE:
# 2532
({struct Cyc_String_pa_PrintArg_struct _tmp421=({struct Cyc_String_pa_PrintArg_struct _tmp5AD;_tmp5AD.tag=0U,({struct _fat_ptr _tmp6F3=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(vd->name);}));_tmp5AD.f1=_tmp6F3;});_tmp5AD;});void*_tmp41F[1U];_tmp41F[0]=& _tmp421;({unsigned _tmp6F5=loc;struct _fat_ptr _tmp6F4=({const char*_tmp420="unique pointers may still exist after variable %s goes out of scope";_tag_fat(_tmp420,sizeof(char),68U);});Cyc_Warn_warn(_tmp6F5,_tmp6F4,_tag_fat(_tmp41F,sizeof(void*),1U));});});
# 2534
goto _LL6;}_LL6:;}}}}
# 2539
static void Cyc_NewControlFlow_check_dropped_unique(struct Cyc_NewControlFlow_AnalEnv*env,union Cyc_CfFlowInfo_FlowInfo inflow,struct Cyc_Absyn_Decl*decl){
{union Cyc_CfFlowInfo_FlowInfo _tmp426=inflow;struct Cyc_Dict_Dict _tmp427;if((_tmp426.ReachableFL).tag == 2){_LL1: _tmp427=((_tmp426.ReachableFL).val).f1;_LL2: {struct Cyc_Dict_Dict fd=_tmp427;
# 2542
struct _tuple32 vdenv=({struct _tuple32 _tmp5AF;_tmp5AF.f1=env,_tmp5AF.f2=fd,_tmp5AF.f3=decl->loc;_tmp5AF;});
struct Cyc_CfFlowInfo_FlowEnv*fenv=env->fenv;
{void*_stmttmp89=decl->r;void*_tmp428=_stmttmp89;struct Cyc_List_List*_tmp429;struct Cyc_List_List*_tmp42A;struct Cyc_Absyn_Vardecl*_tmp42B;switch(*((int*)_tmp428)){case 0U: _LL6: _tmp42B=((struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_tmp428)->f1;_LL7: {struct Cyc_Absyn_Vardecl*vd=_tmp42B;
# 2546
({Cyc_NewControlFlow_check_dropped_unique_vd(& vdenv,vd);});
goto _LL5;}case 2U: if(((struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct*)_tmp428)->f2 != 0){_LL8: _tmp42A=(struct Cyc_List_List*)(((struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct*)_tmp428)->f2)->v;_LL9: {struct Cyc_List_List*vds=_tmp42A;
# 2549
struct _tuple1 _stmttmp8A=({Cyc_List_split(vds);});struct Cyc_List_List*_tmp42C;_LLF: _tmp42C=_stmttmp8A.f1;_LL10: {struct Cyc_List_List*vs=_tmp42C;
struct Cyc_List_List*_tmp42D=({Cyc_Tcutil_filter_nulls(vs);});{struct Cyc_List_List*vs=_tmp42D;
_tmp429=vs;goto _LLB;}}}}else{goto _LLC;}case 3U: _LLA: _tmp429=((struct Cyc_Absyn_Letv_d_Absyn_Raw_decl_struct*)_tmp428)->f1;_LLB: {struct Cyc_List_List*vds=_tmp429;
# 2553
({({void(*_tmp6F6)(void(*f)(struct _tuple32*,struct Cyc_Absyn_Vardecl*),struct _tuple32*env,struct Cyc_List_List*x)=({void(*_tmp42E)(void(*f)(struct _tuple32*,struct Cyc_Absyn_Vardecl*),struct _tuple32*env,struct Cyc_List_List*x)=(void(*)(void(*f)(struct _tuple32*,struct Cyc_Absyn_Vardecl*),struct _tuple32*env,struct Cyc_List_List*x))Cyc_List_iter_c;_tmp42E;});_tmp6F6(Cyc_NewControlFlow_check_dropped_unique_vd,& vdenv,vds);});});
goto _LL5;}default: _LLC: _LLD:
 goto _LL5;}_LL5:;}
# 2557
goto _LL0;}}else{_LL3: _LL4:
 goto _LL0;}_LL0:;}
# 2560
return;}
# 2566
static union Cyc_CfFlowInfo_FlowInfo Cyc_NewControlFlow_unconsume_pat_vars(struct Cyc_NewControlFlow_AnalEnv*env,struct Cyc_Absyn_Stmt*src,union Cyc_CfFlowInfo_FlowInfo inflow,struct Cyc_Absyn_Stmt*dest){
# 2570
int num_pop=({({int(*_tmp6F8)(struct Cyc_Hashtable_Table*t,struct Cyc_Absyn_Stmt*key)=({int(*_tmp434)(struct Cyc_Hashtable_Table*t,struct Cyc_Absyn_Stmt*key)=(int(*)(struct Cyc_Hashtable_Table*t,struct Cyc_Absyn_Stmt*key))Cyc_Hashtable_lookup;_tmp434;});struct Cyc_Hashtable_Table*_tmp6F7=env->pat_pop_table;_tmp6F8(_tmp6F7,src);});});
{struct Cyc_List_List*x=env->unique_pat_vars;for(0;num_pop > 0;(x=x->tl,-- num_pop)){
struct _tuple31*_stmttmp8B=(struct _tuple31*)((struct Cyc_List_List*)_check_null(x))->hd;struct Cyc_List_List*_tmp433;struct Cyc_List_List*_tmp432;struct Cyc_Absyn_Stmt*_tmp431;int _tmp430;_LL1: _tmp430=_stmttmp8B->f1;_tmp431=_stmttmp8B->f2;_tmp432=_stmttmp8B->f3;_tmp433=_stmttmp8B->f4;_LL2: {int is_open=_tmp430;struct Cyc_Absyn_Stmt*pat_stmt=_tmp431;struct Cyc_List_List*roots=_tmp432;struct Cyc_List_List*es=_tmp433;
inflow=({Cyc_NewControlFlow_unconsume_params(env,roots,es,is_open,inflow,dest->loc);});}}}
# 2575
return inflow;}
# 2581
static union Cyc_CfFlowInfo_FlowInfo Cyc_NewControlFlow_anal_stmt(struct Cyc_NewControlFlow_AnalEnv*env,union Cyc_CfFlowInfo_FlowInfo inflow,struct Cyc_Absyn_Stmt*s,struct _tuple18*rval_opt){
# 2583
union Cyc_CfFlowInfo_FlowInfo outflow;
struct _tuple21 _stmttmp8C=({Cyc_NewControlFlow_pre_stmt_check(env,inflow,s);});union Cyc_CfFlowInfo_FlowInfo*_tmp437;struct Cyc_NewControlFlow_CFStmtAnnot*_tmp436;_LL1: _tmp436=_stmttmp8C.f1;_tmp437=_stmttmp8C.f2;_LL2: {struct Cyc_NewControlFlow_CFStmtAnnot*annot=_tmp436;union Cyc_CfFlowInfo_FlowInfo*sflow=_tmp437;
inflow=*sflow;{
struct Cyc_CfFlowInfo_FlowEnv*fenv=env->fenv;
# 2590
void*_stmttmp8D=s->r;void*_tmp438=_stmttmp8D;struct Cyc_Absyn_Stmt*_tmp439;struct Cyc_Absyn_Stmt*_tmp43B;struct Cyc_Absyn_Decl*_tmp43A;struct Cyc_Absyn_Stmt*_tmp43F;unsigned _tmp43E;struct Cyc_Absyn_Exp*_tmp43D;struct Cyc_Absyn_Vardecl*_tmp43C;struct Cyc_Absyn_Stmt*_tmp443;unsigned _tmp442;struct Cyc_Absyn_Exp*_tmp441;struct Cyc_List_List*_tmp440;void*_tmp446;struct Cyc_List_List*_tmp445;struct Cyc_Absyn_Stmt*_tmp444;void*_tmp449;struct Cyc_List_List*_tmp448;struct Cyc_Absyn_Exp*_tmp447;struct Cyc_Absyn_Switch_clause*_tmp44B;struct Cyc_List_List*_tmp44A;struct Cyc_Absyn_Stmt*_tmp451;struct Cyc_Absyn_Stmt*_tmp450;struct Cyc_Absyn_Exp*_tmp44F;struct Cyc_Absyn_Stmt*_tmp44E;struct Cyc_Absyn_Exp*_tmp44D;struct Cyc_Absyn_Exp*_tmp44C;struct Cyc_Absyn_Stmt*_tmp454;struct Cyc_Absyn_Exp*_tmp453;struct Cyc_Absyn_Stmt*_tmp452;struct Cyc_Absyn_Stmt*_tmp457;struct Cyc_Absyn_Stmt*_tmp456;struct Cyc_Absyn_Exp*_tmp455;struct Cyc_Absyn_Stmt*_tmp45A;struct Cyc_Absyn_Stmt*_tmp459;struct Cyc_Absyn_Exp*_tmp458;struct Cyc_Absyn_Stmt*_tmp45C;struct Cyc_Absyn_Stmt*_tmp45B;struct Cyc_Absyn_Exp*_tmp45D;struct Cyc_Absyn_Exp*_tmp45E;switch(*((int*)_tmp438)){case 0U: _LL4: _LL5:
 return inflow;case 3U: if(((struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct*)_tmp438)->f1 == 0){_LL6: _LL7:
# 2594
 if(env->noreturn)
({void*_tmp45F=0U;({unsigned _tmp6FA=s->loc;struct _fat_ptr _tmp6F9=({const char*_tmp460="`noreturn' function might return";_tag_fat(_tmp460,sizeof(char),33U);});Cyc_CfFlowInfo_aerr(_tmp6FA,_tmp6F9,_tag_fat(_tmp45F,sizeof(void*),0U));});});
# 2594
({Cyc_NewControlFlow_check_init_params(s->loc,env,inflow);});
# 2597
({Cyc_NewControlFlow_unconsume_params(env,env->noconsume_params,0,0,inflow,s->loc);});
return({Cyc_CfFlowInfo_BottomFL();});}else{_LL8: _tmp45E=((struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct*)_tmp438)->f1;_LL9: {struct Cyc_Absyn_Exp*e=_tmp45E;
# 2600
if(env->noreturn)
({void*_tmp461=0U;({unsigned _tmp6FC=s->loc;struct _fat_ptr _tmp6FB=({const char*_tmp462="`noreturn' function might return";_tag_fat(_tmp462,sizeof(char),33U);});Cyc_CfFlowInfo_aerr(_tmp6FC,_tmp6FB,_tag_fat(_tmp461,sizeof(void*),0U));});});{
# 2600
struct _tuple17 _stmttmp8E=({Cyc_NewControlFlow_anal_Rexp(env,1,inflow,(struct Cyc_Absyn_Exp*)_check_null(e));});void*_tmp464;union Cyc_CfFlowInfo_FlowInfo _tmp463;_LL2D: _tmp463=_stmttmp8E.f1;_tmp464=_stmttmp8E.f2;_LL2E: {union Cyc_CfFlowInfo_FlowInfo f=_tmp463;void*r=_tmp464;
# 2603
f=({Cyc_NewControlFlow_use_Rval(env,e->loc,f,r);});
({Cyc_NewControlFlow_check_init_params(s->loc,env,f);});
({Cyc_NewControlFlow_unconsume_params(env,env->noconsume_params,0,0,f,s->loc);});
return({Cyc_CfFlowInfo_BottomFL();});}}}}case 1U: _LLA: _tmp45D=((struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct*)_tmp438)->f1;_LLB: {struct Cyc_Absyn_Exp*e=_tmp45D;
# 2609
struct _tuple18*_tmp465=rval_opt;int _tmp467;void**_tmp466;if(_tmp465 != 0){_LL30: _tmp466=(void**)& _tmp465->f1;_tmp467=_tmp465->f2;_LL31: {void**rval=_tmp466;int copy_ctxt=_tmp467;
# 2611
struct _tuple17 _stmttmp8F=({Cyc_NewControlFlow_anal_Rexp(env,copy_ctxt,inflow,e);});void*_tmp469;union Cyc_CfFlowInfo_FlowInfo _tmp468;_LL35: _tmp468=_stmttmp8F.f1;_tmp469=_stmttmp8F.f2;_LL36: {union Cyc_CfFlowInfo_FlowInfo f=_tmp468;void*r=_tmp469;
*rval=r;
return f;}}}else{_LL32: _LL33:
# 2615
 return({Cyc_NewControlFlow_anal_Rexp(env,0,inflow,e);}).f1;}_LL2F:;}case 2U: _LLC: _tmp45B=((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_tmp438)->f1;_tmp45C=((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_tmp438)->f2;_LLD: {struct Cyc_Absyn_Stmt*s1=_tmp45B;struct Cyc_Absyn_Stmt*s2=_tmp45C;
# 2619
return({union Cyc_CfFlowInfo_FlowInfo(*_tmp46A)(struct Cyc_NewControlFlow_AnalEnv*env,union Cyc_CfFlowInfo_FlowInfo inflow,struct Cyc_Absyn_Stmt*s,struct _tuple18*rval_opt)=Cyc_NewControlFlow_anal_stmt;struct Cyc_NewControlFlow_AnalEnv*_tmp46B=env;union Cyc_CfFlowInfo_FlowInfo _tmp46C=({Cyc_NewControlFlow_anal_stmt(env,inflow,s1,0);});struct Cyc_Absyn_Stmt*_tmp46D=s2;struct _tuple18*_tmp46E=rval_opt;_tmp46A(_tmp46B,_tmp46C,_tmp46D,_tmp46E);});}case 4U: _LLE: _tmp458=((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmp438)->f1;_tmp459=((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmp438)->f2;_tmp45A=((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmp438)->f3;_LLF: {struct Cyc_Absyn_Exp*e=_tmp458;struct Cyc_Absyn_Stmt*s1=_tmp459;struct Cyc_Absyn_Stmt*s2=_tmp45A;
# 2622
struct _tuple20 _stmttmp90=({Cyc_NewControlFlow_anal_test(env,inflow,e);});union Cyc_CfFlowInfo_FlowInfo _tmp470;union Cyc_CfFlowInfo_FlowInfo _tmp46F;_LL38: _tmp46F=_stmttmp90.f1;_tmp470=_stmttmp90.f2;_LL39: {union Cyc_CfFlowInfo_FlowInfo f1t=_tmp46F;union Cyc_CfFlowInfo_FlowInfo f1f=_tmp470;
# 2629
union Cyc_CfFlowInfo_FlowInfo ff=({Cyc_NewControlFlow_anal_stmt(env,f1f,s2,0);});
union Cyc_CfFlowInfo_FlowInfo ft=({Cyc_NewControlFlow_anal_stmt(env,f1t,s1,0);});
return({Cyc_CfFlowInfo_join_flow(fenv,ft,ff);});}}case 5U: _LL10: _tmp455=(((struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct*)_tmp438)->f1).f1;_tmp456=(((struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct*)_tmp438)->f1).f2;_tmp457=((struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct*)_tmp438)->f2;_LL11: {struct Cyc_Absyn_Exp*e=_tmp455;struct Cyc_Absyn_Stmt*cont=_tmp456;struct Cyc_Absyn_Stmt*body=_tmp457;
# 2637
struct _tuple21 _stmttmp91=({Cyc_NewControlFlow_pre_stmt_check(env,inflow,cont);});union Cyc_CfFlowInfo_FlowInfo*_tmp471;_LL3B: _tmp471=_stmttmp91.f2;_LL3C: {union Cyc_CfFlowInfo_FlowInfo*eflow_ptr=_tmp471;
union Cyc_CfFlowInfo_FlowInfo e_inflow=*eflow_ptr;
struct _tuple20 _stmttmp92=({Cyc_NewControlFlow_anal_test(env,e_inflow,e);});union Cyc_CfFlowInfo_FlowInfo _tmp473;union Cyc_CfFlowInfo_FlowInfo _tmp472;_LL3E: _tmp472=_stmttmp92.f1;_tmp473=_stmttmp92.f2;_LL3F: {union Cyc_CfFlowInfo_FlowInfo f1t=_tmp472;union Cyc_CfFlowInfo_FlowInfo f1f=_tmp473;
union Cyc_CfFlowInfo_FlowInfo body_outflow=({Cyc_NewControlFlow_anal_stmt(env,f1t,body,0);});
({Cyc_NewControlFlow_update_flow(env,cont,body_outflow);});
return f1f;}}}case 14U: _LL12: _tmp452=((struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct*)_tmp438)->f1;_tmp453=(((struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct*)_tmp438)->f2).f1;_tmp454=(((struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct*)_tmp438)->f2).f2;_LL13: {struct Cyc_Absyn_Stmt*body=_tmp452;struct Cyc_Absyn_Exp*e=_tmp453;struct Cyc_Absyn_Stmt*cont=_tmp454;
# 2647
union Cyc_CfFlowInfo_FlowInfo body_outflow=({Cyc_NewControlFlow_anal_stmt(env,inflow,body,0);});
struct _tuple21 _stmttmp93=({Cyc_NewControlFlow_pre_stmt_check(env,body_outflow,cont);});union Cyc_CfFlowInfo_FlowInfo*_tmp474;_LL41: _tmp474=_stmttmp93.f2;_LL42: {union Cyc_CfFlowInfo_FlowInfo*eflow_ptr=_tmp474;
union Cyc_CfFlowInfo_FlowInfo e_inflow=*eflow_ptr;
struct _tuple20 _stmttmp94=({Cyc_NewControlFlow_anal_test(env,e_inflow,e);});union Cyc_CfFlowInfo_FlowInfo _tmp476;union Cyc_CfFlowInfo_FlowInfo _tmp475;_LL44: _tmp475=_stmttmp94.f1;_tmp476=_stmttmp94.f2;_LL45: {union Cyc_CfFlowInfo_FlowInfo f1t=_tmp475;union Cyc_CfFlowInfo_FlowInfo f1f=_tmp476;
({Cyc_NewControlFlow_update_flow(env,body,f1t);});
return f1f;}}}case 9U: _LL14: _tmp44C=((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp438)->f1;_tmp44D=(((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp438)->f2).f1;_tmp44E=(((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp438)->f2).f2;_tmp44F=(((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp438)->f3).f1;_tmp450=(((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp438)->f3).f2;_tmp451=((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp438)->f4;_LL15: {struct Cyc_Absyn_Exp*e1=_tmp44C;struct Cyc_Absyn_Exp*e2=_tmp44D;struct Cyc_Absyn_Stmt*guard=_tmp44E;struct Cyc_Absyn_Exp*e3=_tmp44F;struct Cyc_Absyn_Stmt*cont=_tmp450;struct Cyc_Absyn_Stmt*body=_tmp451;
# 2656
union Cyc_CfFlowInfo_FlowInfo e1_outflow=({Cyc_NewControlFlow_anal_Rexp(env,0,inflow,e1);}).f1;
struct _tuple21 _stmttmp95=({Cyc_NewControlFlow_pre_stmt_check(env,e1_outflow,guard);});union Cyc_CfFlowInfo_FlowInfo*_tmp477;_LL47: _tmp477=_stmttmp95.f2;_LL48: {union Cyc_CfFlowInfo_FlowInfo*e2flow_ptr=_tmp477;
union Cyc_CfFlowInfo_FlowInfo e2_inflow=*e2flow_ptr;
struct _tuple20 _stmttmp96=({Cyc_NewControlFlow_anal_test(env,e2_inflow,e2);});union Cyc_CfFlowInfo_FlowInfo _tmp479;union Cyc_CfFlowInfo_FlowInfo _tmp478;_LL4A: _tmp478=_stmttmp96.f1;_tmp479=_stmttmp96.f2;_LL4B: {union Cyc_CfFlowInfo_FlowInfo f2t=_tmp478;union Cyc_CfFlowInfo_FlowInfo f2f=_tmp479;
union Cyc_CfFlowInfo_FlowInfo body_outflow=({Cyc_NewControlFlow_anal_stmt(env,f2t,body,0);});
struct _tuple21 _stmttmp97=({Cyc_NewControlFlow_pre_stmt_check(env,body_outflow,cont);});union Cyc_CfFlowInfo_FlowInfo*_tmp47A;_LL4D: _tmp47A=_stmttmp97.f2;_LL4E: {union Cyc_CfFlowInfo_FlowInfo*e3flow_ptr=_tmp47A;
union Cyc_CfFlowInfo_FlowInfo e3_inflow=*e3flow_ptr;
union Cyc_CfFlowInfo_FlowInfo e3_outflow=({Cyc_NewControlFlow_anal_Rexp(env,0,e3_inflow,e3);}).f1;
({Cyc_NewControlFlow_update_flow(env,guard,e3_outflow);});
return f2f;}}}}case 11U: if(((struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct*)_tmp438)->f2 != 0){_LL16: _tmp44A=((struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct*)_tmp438)->f1;_tmp44B=*((struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct*)_tmp438)->f2;_LL17: {struct Cyc_List_List*es=_tmp44A;struct Cyc_Absyn_Switch_clause*destclause=_tmp44B;
# 2668
struct _tuple24 _stmttmp98=({Cyc_NewControlFlow_anal_Rexps(env,inflow,es,1,1);});struct Cyc_List_List*_tmp47C;union Cyc_CfFlowInfo_FlowInfo _tmp47B;_LL50: _tmp47B=_stmttmp98.f1;_tmp47C=_stmttmp98.f2;_LL51: {union Cyc_CfFlowInfo_FlowInfo f=_tmp47B;struct Cyc_List_List*rvals=_tmp47C;
# 2670
inflow=({Cyc_NewControlFlow_unconsume_pat_vars(env,s,inflow,destclause->body);});{
# 2672
struct Cyc_List_List*vds=({struct Cyc_List_List*(*_tmp47F)(struct Cyc_List_List*)=Cyc_Tcutil_filter_nulls;struct Cyc_List_List*_tmp480=({Cyc_List_split((struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(destclause->pat_vars))->v);}).f1;_tmp47F(_tmp480);});
f=({Cyc_NewControlFlow_add_vars(fenv,f,vds,fenv->unknown_all,s->loc,0);});
# 2675
{struct Cyc_List_List*x=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(destclause->pat_vars))->v;for(0;x != 0;x=x->tl){
struct _tuple25*_stmttmp99=(struct _tuple25*)x->hd;struct Cyc_Absyn_Exp*_tmp47E;struct Cyc_Absyn_Vardecl**_tmp47D;_LL53: _tmp47D=_stmttmp99->f1;_tmp47E=_stmttmp99->f2;_LL54: {struct Cyc_Absyn_Vardecl**vd=_tmp47D;struct Cyc_Absyn_Exp*ve=_tmp47E;
if(vd != 0){
f=({({struct Cyc_CfFlowInfo_FlowEnv*_tmp702=fenv;struct Cyc_NewControlFlow_AnalEnv*_tmp701=env;union Cyc_CfFlowInfo_FlowInfo _tmp700=f;struct Cyc_Absyn_Vardecl*_tmp6FF=*vd;struct Cyc_Absyn_Exp*_tmp6FE=(struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(es))->hd;void*_tmp6FD=(void*)((struct Cyc_List_List*)_check_null(rvals))->hd;Cyc_NewControlFlow_do_initialize_var(_tmp702,_tmp701,_tmp700,_tmp6FF,_tmp6FE,_tmp6FD,s->loc);});});
rvals=rvals->tl;
es=es->tl;}}}}
# 2683
({Cyc_NewControlFlow_update_flow(env,destclause->body,f);});
return({Cyc_CfFlowInfo_BottomFL();});}}}}else{_LL2A: _LL2B:
# 2847
({void*_tmp4A6=0U;({int(*_tmp704)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp4A8)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp4A8;});struct _fat_ptr _tmp703=({const char*_tmp4A7="anal_stmt: bad stmt syntax or unimplemented stmt form";_tag_fat(_tmp4A7,sizeof(char),54U);});_tmp704(_tmp703,_tag_fat(_tmp4A6,sizeof(void*),0U));});});}case 6U: _LL18: _LL19:
# 2689
 if(({({struct Cyc_Absyn_Stmt*(*_tmp706)(struct Cyc_Hashtable_Table*t,struct Cyc_Absyn_Stmt*key)=({struct Cyc_Absyn_Stmt*(*_tmp481)(struct Cyc_Hashtable_Table*t,struct Cyc_Absyn_Stmt*key)=(struct Cyc_Absyn_Stmt*(*)(struct Cyc_Hashtable_Table*t,struct Cyc_Absyn_Stmt*key))Cyc_Hashtable_lookup;_tmp481;});struct Cyc_Hashtable_Table*_tmp705=env->succ_table;_tmp706(_tmp705,s);});})== 0){
{union Cyc_CfFlowInfo_FlowInfo _tmp482=inflow;if((_tmp482.ReachableFL).tag == 2){_LL56: _LL57:
# 2692
 if(!({Cyc_Tcutil_is_void_type(env->return_type);})){
if(({Cyc_Tcutil_is_any_float_type(env->return_type);})||({Cyc_Tcutil_is_any_int_type(env->return_type);}))
# 2695
({void*_tmp483=0U;({unsigned _tmp708=s->loc;struct _fat_ptr _tmp707=({const char*_tmp484="break may cause function not to return a value";_tag_fat(_tmp484,sizeof(char),47U);});Cyc_Warn_warn(_tmp708,_tmp707,_tag_fat(_tmp483,sizeof(void*),0U));});});else{
# 2697
({void*_tmp485=0U;({unsigned _tmp70A=s->loc;struct _fat_ptr _tmp709=({const char*_tmp486="break may cause function not to return a value";_tag_fat(_tmp486,sizeof(char),47U);});Cyc_CfFlowInfo_aerr(_tmp70A,_tmp709,_tag_fat(_tmp485,sizeof(void*),0U));});});}}
# 2692
goto _LL55;}else{_LL58: _LL59:
# 2700
 goto _LL55;}_LL55:;}
# 2702
if(env->noreturn)
({void*_tmp487=0U;({unsigned _tmp70C=s->loc;struct _fat_ptr _tmp70B=({const char*_tmp488="`noreturn' function might return";_tag_fat(_tmp488,sizeof(char),33U);});Cyc_CfFlowInfo_aerr(_tmp70C,_tmp70B,_tag_fat(_tmp487,sizeof(void*),0U));});});
# 2702
({Cyc_NewControlFlow_check_init_params(s->loc,env,inflow);});
# 2705
({Cyc_NewControlFlow_unconsume_params(env,env->noconsume_params,0,0,inflow,s->loc);});
return({Cyc_CfFlowInfo_BottomFL();});}
# 2689
goto _LL1B;case 7U: _LL1A: _LL1B:
# 2709
 goto _LL1D;case 8U: _LL1C: _LL1D: {
# 2712
struct Cyc_Absyn_Stmt*dest=({({struct Cyc_Absyn_Stmt*(*_tmp70E)(struct Cyc_Hashtable_Table*t,struct Cyc_Absyn_Stmt*key)=({struct Cyc_Absyn_Stmt*(*_tmp48B)(struct Cyc_Hashtable_Table*t,struct Cyc_Absyn_Stmt*key)=(struct Cyc_Absyn_Stmt*(*)(struct Cyc_Hashtable_Table*t,struct Cyc_Absyn_Stmt*key))Cyc_Hashtable_lookup;_tmp48B;});struct Cyc_Hashtable_Table*_tmp70D=env->succ_table;_tmp70E(_tmp70D,s);});});
if(dest == 0)
({void*_tmp489=0U;({unsigned _tmp710=s->loc;struct _fat_ptr _tmp70F=({const char*_tmp48A="jump has no target (should be impossible)";_tag_fat(_tmp48A,sizeof(char),42U);});Cyc_CfFlowInfo_aerr(_tmp710,_tmp70F,_tag_fat(_tmp489,sizeof(void*),0U));});});
# 2713
inflow=({Cyc_NewControlFlow_unconsume_pat_vars(env,s,inflow,(struct Cyc_Absyn_Stmt*)_check_null(dest));});
# 2717
({Cyc_NewControlFlow_update_flow(env,dest,inflow);});
return({Cyc_CfFlowInfo_BottomFL();});}case 10U: _LL1E: _tmp447=((struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct*)_tmp438)->f1;_tmp448=((struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct*)_tmp438)->f2;_tmp449=(void*)((struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct*)_tmp438)->f3;_LL1F: {struct Cyc_Absyn_Exp*e=_tmp447;struct Cyc_List_List*scs=_tmp448;void*dec_tree=_tmp449;
# 2723
return({Cyc_NewControlFlow_anal_scs(env,inflow,scs,e->loc);});}case 15U: _LL20: _tmp444=((struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct*)_tmp438)->f1;_tmp445=((struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct*)_tmp438)->f2;_tmp446=(void*)((struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct*)_tmp438)->f3;_LL21: {struct Cyc_Absyn_Stmt*s1=_tmp444;struct Cyc_List_List*scs=_tmp445;void*dec_tree=_tmp446;
# 2728
int old_in_try=env->in_try;
union Cyc_CfFlowInfo_FlowInfo old_tryflow=env->tryflow;
env->in_try=1;
env->tryflow=inflow;{
union Cyc_CfFlowInfo_FlowInfo s1_outflow=({Cyc_NewControlFlow_anal_stmt(env,inflow,s1,0);});
union Cyc_CfFlowInfo_FlowInfo scs_inflow=env->tryflow;
# 2736
env->in_try=old_in_try;
env->tryflow=old_tryflow;
# 2739
({Cyc_NewControlFlow_update_tryflow(env,scs_inflow);});{
union Cyc_CfFlowInfo_FlowInfo scs_outflow=({Cyc_NewControlFlow_anal_scs(env,scs_inflow,scs,0U);});
{union Cyc_CfFlowInfo_FlowInfo _tmp48C=scs_outflow;if((_tmp48C.BottomFL).tag == 1){_LL5B: _LL5C:
 goto _LL5A;}else{_LL5D: _LL5E:
({void*_tmp48D=0U;({unsigned _tmp712=s->loc;struct _fat_ptr _tmp711=({const char*_tmp48E="last catch clause may implicitly fallthru";_tag_fat(_tmp48E,sizeof(char),42U);});Cyc_CfFlowInfo_aerr(_tmp712,_tmp711,_tag_fat(_tmp48D,sizeof(void*),0U));});});}_LL5A:;}
# 2745
outflow=s1_outflow;
# 2747
return outflow;}}}case 12U: switch(*((int*)((struct Cyc_Absyn_Decl*)((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp438)->f1)->r)){case 2U: if(((struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct*)((struct Cyc_Absyn_Decl*)((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp438)->f1)->r)->f2 != 0){_LL22: _tmp440=(struct Cyc_List_List*)(((struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct*)(((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp438)->f1)->r)->f2)->v;_tmp441=((struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct*)(((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp438)->f1)->r)->f3;_tmp442=(((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp438)->f1)->loc;_tmp443=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp438)->f2;_LL23: {struct Cyc_List_List*vds=_tmp440;struct Cyc_Absyn_Exp*e=_tmp441;unsigned loc=_tmp442;struct Cyc_Absyn_Stmt*s1=_tmp443;
# 2757
struct _tuple1 _stmttmp9A=({Cyc_NewControlFlow_get_unconsume_pat_vars(vds);});struct Cyc_List_List*_tmp490;struct Cyc_List_List*_tmp48F;_LL60: _tmp48F=_stmttmp9A.f1;_tmp490=_stmttmp9A.f2;_LL61: {struct Cyc_List_List*roots=_tmp48F;struct Cyc_List_List*es=_tmp490;
inflow=({Cyc_NewControlFlow_initialize_pat_vars(fenv,env,inflow,vds,roots != 0,loc,e->loc);});{
struct Cyc_List_List*old_unique_pat_vars=env->unique_pat_vars;
# 2761
({struct Cyc_List_List*_tmp714=({struct Cyc_List_List*_tmp492=_cycalloc(sizeof(*_tmp492));
({struct _tuple31*_tmp713=({struct _tuple31*_tmp491=_cycalloc(sizeof(*_tmp491));_tmp491->f1=0,_tmp491->f2=s,_tmp491->f3=roots,_tmp491->f4=es;_tmp491;});_tmp492->hd=_tmp713;}),_tmp492->tl=old_unique_pat_vars;_tmp492;});
# 2761
env->unique_pat_vars=_tmp714;});
# 2766
inflow=({Cyc_NewControlFlow_anal_stmt(env,inflow,s1,rval_opt);});
env->unique_pat_vars=old_unique_pat_vars;
# 2770
inflow=({Cyc_NewControlFlow_unconsume_params(env,roots,es,0,inflow,loc);});
# 2774
return inflow;}}}}else{goto _LL26;}case 4U: _LL24: _tmp43C=((struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*)(((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp438)->f1)->r)->f2;_tmp43D=((struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*)(((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp438)->f1)->r)->f3;_tmp43E=(((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp438)->f1)->loc;_tmp43F=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp438)->f2;if(_tmp43D != 0){_LL25: {struct Cyc_Absyn_Vardecl*vd=_tmp43C;struct Cyc_Absyn_Exp*open_exp_opt=_tmp43D;unsigned loc=_tmp43E;struct Cyc_Absyn_Stmt*s1=_tmp43F;
# 2786
struct Cyc_List_List l=({struct Cyc_List_List _tmp5B1;_tmp5B1.hd=open_exp_opt,_tmp5B1.tl=0;_tmp5B1;});
union Cyc_CfFlowInfo_FlowInfo f=({Cyc_NewControlFlow_expand_unique_places(env,inflow,& l);});
struct _tuple19 _stmttmp9B=({Cyc_NewControlFlow_anal_Lexp(env,f,0,0,open_exp_opt);});union Cyc_CfFlowInfo_AbsLVal _tmp493;_LL63: _tmp493=_stmttmp9B.f2;_LL64: {union Cyc_CfFlowInfo_AbsLVal lval=_tmp493;
struct _tuple17 _stmttmp9C=({Cyc_NewControlFlow_anal_Rexp(env,1,f,open_exp_opt);});union Cyc_CfFlowInfo_FlowInfo _tmp494;_LL66: _tmp494=_stmttmp9C.f1;_LL67: {union Cyc_CfFlowInfo_FlowInfo f=_tmp494;
struct Cyc_List_List*roots=0;
struct Cyc_List_List*es=0;
{union Cyc_CfFlowInfo_FlowInfo _tmp495=f;struct Cyc_List_List*_tmp497;struct Cyc_Dict_Dict _tmp496;if((_tmp495.ReachableFL).tag == 2){_LL69: _tmp496=((_tmp495.ReachableFL).val).f1;_tmp497=((_tmp495.ReachableFL).val).f2;_LL6A: {struct Cyc_Dict_Dict fd=_tmp496;struct Cyc_List_List*relns=_tmp497;
# 2794
{union Cyc_CfFlowInfo_AbsLVal _tmp498=lval;struct Cyc_CfFlowInfo_Place*_tmp499;if((_tmp498.PlaceL).tag == 1){_LL6E: _tmp499=(_tmp498.PlaceL).val;_LL6F: {struct Cyc_CfFlowInfo_Place*p=_tmp499;
# 2798
void*new_rval=({Cyc_CfFlowInfo_lookup_place(fd,p);});
new_rval=(void*)({struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct*_tmp49A=_cycalloc(sizeof(*_tmp49A));_tmp49A->tag=8U,_tmp49A->f1=vd,_tmp49A->f2=new_rval;_tmp49A;});
fd=({Cyc_CfFlowInfo_assign_place(fenv,open_exp_opt->loc,fd,p,new_rval);});
f=({Cyc_CfFlowInfo_ReachableFL(fd,relns);});{
struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct*root=({struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct*_tmp49E=_cycalloc(sizeof(*_tmp49E));_tmp49E->tag=0U,_tmp49E->f1=vd;_tmp49E;});
struct Cyc_CfFlowInfo_Place*rp=({struct Cyc_CfFlowInfo_Place*_tmp49D=_cycalloc(sizeof(*_tmp49D));_tmp49D->root=(void*)root,_tmp49D->path=0;_tmp49D;});
roots=({struct Cyc_List_List*_tmp49B=_cycalloc(sizeof(*_tmp49B));_tmp49B->hd=rp,_tmp49B->tl=roots;_tmp49B;});
es=({struct Cyc_List_List*_tmp49C=_cycalloc(sizeof(*_tmp49C));_tmp49C->hd=open_exp_opt,_tmp49C->tl=es;_tmp49C;});
goto _LL6D;}}}else{_LL70: _LL71:
# 2813
 goto _LL6D;}_LL6D:;}
# 2815
goto _LL68;}}else{_LL6B: _LL6C:
# 2817
 goto _LL68;}_LL68:;}{
# 2820
struct Cyc_List_List vds=({struct Cyc_List_List _tmp5B0;_tmp5B0.hd=vd,_tmp5B0.tl=0;_tmp5B0;});
f=({Cyc_NewControlFlow_add_vars(fenv,f,& vds,fenv->unknown_all,loc,0);});{
# 2824
struct Cyc_List_List*old_unique_pat_vars=env->unique_pat_vars;
({struct Cyc_List_List*_tmp716=({struct Cyc_List_List*_tmp4A0=_cycalloc(sizeof(*_tmp4A0));({struct _tuple31*_tmp715=({struct _tuple31*_tmp49F=_cycalloc(sizeof(*_tmp49F));_tmp49F->f1=1,_tmp49F->f2=s,_tmp49F->f3=roots,_tmp49F->f4=es;_tmp49F;});_tmp4A0->hd=_tmp715;}),_tmp4A0->tl=old_unique_pat_vars;_tmp4A0;});env->unique_pat_vars=_tmp716;});
# 2829
f=({Cyc_NewControlFlow_anal_stmt(env,f,s1,rval_opt);});
env->unique_pat_vars=old_unique_pat_vars;
# 2833
f=({Cyc_NewControlFlow_unconsume_params(env,roots,es,1,f,loc);});
# 2837
return f;}}}}}}else{goto _LL26;}default: _LL26: _tmp43A=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp438)->f1;_tmp43B=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp438)->f2;_LL27: {struct Cyc_Absyn_Decl*d=_tmp43A;struct Cyc_Absyn_Stmt*s=_tmp43B;
# 2840
outflow=({union Cyc_CfFlowInfo_FlowInfo(*_tmp4A1)(struct Cyc_NewControlFlow_AnalEnv*env,union Cyc_CfFlowInfo_FlowInfo inflow,struct Cyc_Absyn_Stmt*s,struct _tuple18*rval_opt)=Cyc_NewControlFlow_anal_stmt;struct Cyc_NewControlFlow_AnalEnv*_tmp4A2=env;union Cyc_CfFlowInfo_FlowInfo _tmp4A3=({Cyc_NewControlFlow_anal_decl(env,inflow,d);});struct Cyc_Absyn_Stmt*_tmp4A4=s;struct _tuple18*_tmp4A5=rval_opt;_tmp4A1(_tmp4A2,_tmp4A3,_tmp4A4,_tmp4A5);});
if(Cyc_NewControlFlow_warn_lose_unique)
({Cyc_NewControlFlow_check_dropped_unique(env,outflow,d);});
# 2841
return outflow;}}default: _LL28: _tmp439=((struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct*)_tmp438)->f2;_LL29: {struct Cyc_Absyn_Stmt*s=_tmp439;
# 2845
return({Cyc_NewControlFlow_anal_stmt(env,inflow,s,rval_opt);});}}_LL3:;}}}
# 2581
static void Cyc_NewControlFlow_check_nested_fun(struct Cyc_JumpAnalysis_Jump_Anal_Result*tables,struct Cyc_CfFlowInfo_FlowEnv*,union Cyc_CfFlowInfo_FlowInfo inflow,struct Cyc_Absyn_Fndecl*fd);
# 2855
static union Cyc_CfFlowInfo_FlowInfo Cyc_NewControlFlow_anal_decl(struct Cyc_NewControlFlow_AnalEnv*env,union Cyc_CfFlowInfo_FlowInfo inflow,struct Cyc_Absyn_Decl*decl){
struct Cyc_CfFlowInfo_FlowEnv*fenv=env->fenv;
void*_stmttmp9D=decl->r;void*_tmp4AA=_stmttmp9D;struct Cyc_Absyn_Exp*_tmp4AD;struct Cyc_Absyn_Vardecl*_tmp4AC;struct Cyc_Absyn_Tvar*_tmp4AB;struct Cyc_Absyn_Fndecl*_tmp4AE;struct Cyc_List_List*_tmp4AF;struct Cyc_Absyn_Vardecl*_tmp4B0;switch(*((int*)_tmp4AA)){case 0U: _LL1: _tmp4B0=((struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_tmp4AA)->f1;_LL2: {struct Cyc_Absyn_Vardecl*vd=_tmp4B0;
# 2863
if((int)vd->sc == (int)Cyc_Absyn_Static)
return inflow;{
# 2868
struct Cyc_List_List vds=({struct Cyc_List_List _tmp5B2;_tmp5B2.hd=vd,_tmp5B2.tl=0;_tmp5B2;});
inflow=({Cyc_NewControlFlow_add_vars(fenv,inflow,& vds,fenv->unknown_none,decl->loc,0);});{
struct Cyc_Absyn_Exp*e=vd->initializer;
if(e == 0)
return inflow;{
# 2871
struct _tuple17 _stmttmp9E=({Cyc_NewControlFlow_anal_Rexp(env,1,inflow,e);});void*_tmp4B2;union Cyc_CfFlowInfo_FlowInfo _tmp4B1;_LLC: _tmp4B1=_stmttmp9E.f1;_tmp4B2=_stmttmp9E.f2;_LLD: {union Cyc_CfFlowInfo_FlowInfo f=_tmp4B1;void*r=_tmp4B2;
# 2874
return({Cyc_NewControlFlow_do_initialize_var(fenv,env,f,vd,e,r,decl->loc);});}}}}}case 3U: _LL3: _tmp4AF=((struct Cyc_Absyn_Letv_d_Absyn_Raw_decl_struct*)_tmp4AA)->f1;_LL4: {struct Cyc_List_List*vds=_tmp4AF;
# 2877
return({Cyc_NewControlFlow_add_vars(fenv,inflow,vds,fenv->unknown_none,decl->loc,0);});}case 1U: _LL5: _tmp4AE=((struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*)_tmp4AA)->f1;_LL6: {struct Cyc_Absyn_Fndecl*fd=_tmp4AE;
# 2880
({Cyc_NewControlFlow_check_nested_fun(env->all_tables,fenv,inflow,fd);});{
struct Cyc_Absyn_Vardecl*vd=(struct Cyc_Absyn_Vardecl*)_check_null(fd->fn_vardecl);
# 2885
return({({struct Cyc_CfFlowInfo_FlowEnv*_tmp71A=fenv;union Cyc_CfFlowInfo_FlowInfo _tmp719=inflow;struct Cyc_List_List*_tmp718=({struct Cyc_List_List*_tmp4B3=_cycalloc(sizeof(*_tmp4B3));_tmp4B3->hd=vd,_tmp4B3->tl=0;_tmp4B3;});void*_tmp717=fenv->unknown_all;Cyc_NewControlFlow_add_vars(_tmp71A,_tmp719,_tmp718,_tmp717,decl->loc,0);});});}}case 4U: _LL7: _tmp4AB=((struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*)_tmp4AA)->f1;_tmp4AC=((struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*)_tmp4AA)->f2;_tmp4AD=((struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*)_tmp4AA)->f3;_LL8: {struct Cyc_Absyn_Tvar*tv=_tmp4AB;struct Cyc_Absyn_Vardecl*vd=_tmp4AC;struct Cyc_Absyn_Exp*open_exp_opt=_tmp4AD;
# 2888
if(open_exp_opt != 0)
({void*_tmp4B4=0U;({int(*_tmp71C)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp4B6)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp4B6;});struct _fat_ptr _tmp71B=({const char*_tmp4B5="found open expression in declaration!";_tag_fat(_tmp4B5,sizeof(char),38U);});_tmp71C(_tmp71B,_tag_fat(_tmp4B4,sizeof(void*),0U));});});{
# 2888
struct Cyc_List_List vds=({struct Cyc_List_List _tmp5B3;_tmp5B3.hd=vd,_tmp5B3.tl=0;_tmp5B3;});
# 2891
return({Cyc_NewControlFlow_add_vars(fenv,inflow,& vds,fenv->unknown_all,decl->loc,0);});}}default: _LL9: _LLA:
# 2894
({void*_tmp4B7=0U;({int(*_tmp71E)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp4B9)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp4B9;});struct _fat_ptr _tmp71D=({const char*_tmp4B8="anal_decl: unexpected decl variant";_tag_fat(_tmp4B8,sizeof(char),35U);});_tmp71E(_tmp71D,_tag_fat(_tmp4B7,sizeof(void*),0U));});});}_LL0:;}
# 2902
static void Cyc_NewControlFlow_check_fun(struct Cyc_JumpAnalysis_Jump_Anal_Result*tables,struct Cyc_Absyn_Fndecl*fd){
struct _handler_cons _tmp4BB;_push_handler(& _tmp4BB);{int _tmp4BD=0;if(setjmp(_tmp4BB.handler))_tmp4BD=1;if(!_tmp4BD){
{struct Cyc_CfFlowInfo_FlowEnv*fenv=({Cyc_CfFlowInfo_new_flow_env();});
({void(*_tmp4BE)(struct Cyc_JumpAnalysis_Jump_Anal_Result*tables,struct Cyc_CfFlowInfo_FlowEnv*,union Cyc_CfFlowInfo_FlowInfo inflow,struct Cyc_Absyn_Fndecl*fd)=Cyc_NewControlFlow_check_nested_fun;struct Cyc_JumpAnalysis_Jump_Anal_Result*_tmp4BF=tables;struct Cyc_CfFlowInfo_FlowEnv*_tmp4C0=fenv;union Cyc_CfFlowInfo_FlowInfo _tmp4C1=({Cyc_CfFlowInfo_ReachableFL(fenv->mt_flowdict,0);});struct Cyc_Absyn_Fndecl*_tmp4C2=fd;_tmp4BE(_tmp4BF,_tmp4C0,_tmp4C1,_tmp4C2);});}
# 2904
;_pop_handler();}else{void*_tmp4BC=(void*)Cyc_Core_get_exn_thrown();void*_tmp4C3=_tmp4BC;void*_tmp4C4;if(((struct Cyc_Dict_Absent_exn_struct*)_tmp4C3)->tag == Cyc_Dict_Absent){_LL1: _LL2:
# 2910
 if(Cyc_Position_num_errors > 0)
goto _LL0;else{
({Cyc_Core_rethrow((void*)({struct Cyc_Dict_Absent_exn_struct*_tmp4C5=_cycalloc(sizeof(*_tmp4C5));_tmp4C5->tag=Cyc_Dict_Absent;_tmp4C5;}));});}}else{_LL3: _tmp4C4=_tmp4C3;_LL4: {void*exn=_tmp4C4;(int)_rethrow(exn);}}_LL0:;}}}
# 2916
static int Cyc_NewControlFlow_hash_ptr(void*s){
return(int)s;}
# 2921
static union Cyc_Relations_RelnOp Cyc_NewControlFlow_translate_rop(struct Cyc_List_List*vds,union Cyc_Relations_RelnOp r){
union Cyc_Relations_RelnOp _tmp4C8=r;unsigned _tmp4C9;if((_tmp4C8.RParam).tag == 5){_LL1: _tmp4C9=(_tmp4C8.RParam).val;_LL2: {unsigned i=_tmp4C9;
# 2924
struct Cyc_Absyn_Vardecl*vd=({({struct Cyc_Absyn_Vardecl*(*_tmp720)(struct Cyc_List_List*x,int n)=({struct Cyc_Absyn_Vardecl*(*_tmp4CA)(struct Cyc_List_List*x,int n)=(struct Cyc_Absyn_Vardecl*(*)(struct Cyc_List_List*x,int n))Cyc_List_nth;_tmp4CA;});struct Cyc_List_List*_tmp71F=vds;_tmp720(_tmp71F,(int)i);});});
if(!vd->escapes)
return({Cyc_Relations_RVar(vd);});
# 2925
return r;}}else{_LL3: _LL4:
# 2928
 return r;}_LL0:;}
# 2935
static struct Cyc_List_List*Cyc_NewControlFlow_get_noconsume_params(struct Cyc_List_List*param_vardecls,struct Cyc_List_List*atts){
# 2937
struct _RegionHandle _tmp4CC=_new_region("r");struct _RegionHandle*r=& _tmp4CC;_push_region(r);
{int len=({Cyc_List_length(param_vardecls);});
struct _fat_ptr cons=({unsigned _tmp4D5=(unsigned)len;int*_tmp4D4=({struct _RegionHandle*_tmp721=r;_region_malloc(_tmp721,_check_times(_tmp4D5,sizeof(int)));});({{unsigned _tmp5B4=(unsigned)len;unsigned i;for(i=0;i < _tmp5B4;++ i){_tmp4D4[i]=0;}}0;});_tag_fat(_tmp4D4,sizeof(int),_tmp4D5);});
for(0;atts != 0;atts=atts->tl){
void*_stmttmp9F=(void*)atts->hd;void*_tmp4CD=_stmttmp9F;int _tmp4CE;int _tmp4CF;switch(*((int*)_tmp4CD)){case 22U: _LL1: _tmp4CF=((struct Cyc_Absyn_Consume_att_Absyn_Attribute_struct*)_tmp4CD)->f1;_LL2: {int i=_tmp4CF;
_tmp4CE=i;goto _LL4;}case 21U: _LL3: _tmp4CE=((struct Cyc_Absyn_Noliveunique_att_Absyn_Attribute_struct*)_tmp4CD)->f1;_LL4: {int i=_tmp4CE;
*((int*)_check_fat_subscript(cons,sizeof(int),i - 1))=1;goto _LL0;}default: _LL5: _LL6:
 goto _LL0;}_LL0:;}{
# 2947
struct Cyc_List_List*noconsume_roots=0;
{int i=0;for(0;param_vardecls != 0;(param_vardecls=param_vardecls->tl,i ++)){
struct Cyc_Absyn_Vardecl*vd=(struct Cyc_Absyn_Vardecl*)param_vardecls->hd;
if(({Cyc_Tcutil_is_noalias_pointer(vd->type,0);})&& !(*((int*)_check_fat_subscript(cons,sizeof(int),i)))){
struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct*root=({struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct*_tmp4D2=_cycalloc(sizeof(*_tmp4D2));_tmp4D2->tag=0U,_tmp4D2->f1=vd;_tmp4D2;});
struct Cyc_CfFlowInfo_Place*rp=({struct Cyc_CfFlowInfo_Place*_tmp4D1=_cycalloc(sizeof(*_tmp4D1));_tmp4D1->root=(void*)root,_tmp4D1->path=0;_tmp4D1;});
noconsume_roots=({struct Cyc_List_List*_tmp4D0=_cycalloc(sizeof(*_tmp4D0));_tmp4D0->hd=rp,_tmp4D0->tl=noconsume_roots;_tmp4D0;});}}}{
# 2956
struct Cyc_List_List*_tmp4D3=noconsume_roots;_npop_handler(0U);return _tmp4D3;}}}
# 2938
;_pop_region();}
# 2959
static void Cyc_NewControlFlow_check_nested_fun(struct Cyc_JumpAnalysis_Jump_Anal_Result*tables,struct Cyc_CfFlowInfo_FlowEnv*fenv,union Cyc_CfFlowInfo_FlowInfo inflow,struct Cyc_Absyn_Fndecl*fd){
# 2963
unsigned loc=(fd->body)->loc;
inflow=({Cyc_NewControlFlow_add_vars(fenv,inflow,(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(fd->param_vardecls))->v,fenv->unknown_all,loc,1);});{
# 2968
struct Cyc_List_List*param_roots=0;
struct _tuple15 _stmttmpA0=({union Cyc_CfFlowInfo_FlowInfo _tmp50B=inflow;if((_tmp50B.ReachableFL).tag != 2)_throw_match();(_tmp50B.ReachableFL).val;});struct Cyc_List_List*_tmp4D8;struct Cyc_Dict_Dict _tmp4D7;_LL1: _tmp4D7=_stmttmpA0.f1;_tmp4D8=_stmttmpA0.f2;_LL2: {struct Cyc_Dict_Dict d=_tmp4D7;struct Cyc_List_List*relns=_tmp4D8;
# 2972
struct Cyc_List_List*vardecls=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(fd->param_vardecls))->v;
{struct Cyc_List_List*reqs=(fd->i).requires_relns;for(0;reqs != 0;reqs=reqs->tl){
struct Cyc_Relations_Reln*req=(struct Cyc_Relations_Reln*)reqs->hd;
relns=({struct Cyc_List_List*(*_tmp4D9)(struct _RegionHandle*rgn,union Cyc_Relations_RelnOp rop1,enum Cyc_Relations_Relation r,union Cyc_Relations_RelnOp rop2,struct Cyc_List_List*relns)=Cyc_Relations_add_relation;struct _RegionHandle*_tmp4DA=Cyc_Core_heap_region;union Cyc_Relations_RelnOp _tmp4DB=({Cyc_NewControlFlow_translate_rop(vardecls,req->rop1);});enum Cyc_Relations_Relation _tmp4DC=req->relation;union Cyc_Relations_RelnOp _tmp4DD=({Cyc_NewControlFlow_translate_rop(vardecls,req->rop2);});struct Cyc_List_List*_tmp4DE=relns;_tmp4D9(_tmp4DA,_tmp4DB,_tmp4DC,_tmp4DD,_tmp4DE);});}}{
# 2981
struct Cyc_List_List*atts;
{void*_stmttmpA1=({Cyc_Tcutil_compress((void*)_check_null(fd->cached_type));});void*_tmp4DF=_stmttmpA1;struct Cyc_List_List*_tmp4E0;if(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp4DF)->tag == 5U){_LL4: _tmp4E0=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp4DF)->f1).attributes;_LL5: {struct Cyc_List_List*as=_tmp4E0;
atts=as;goto _LL3;}}else{_LL6: _LL7:
({void*_tmp4E1=0U;({int(*_tmp723)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp4E3)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp4E3;});struct _fat_ptr _tmp722=({const char*_tmp4E2="check_fun: non-function type cached with fndecl_t";_tag_fat(_tmp4E2,sizeof(char),50U);});_tmp723(_tmp722,_tag_fat(_tmp4E1,sizeof(void*),0U));});});}_LL3:;}{
# 2986
struct Cyc_List_List*noconsume_roots=({Cyc_NewControlFlow_get_noconsume_params((struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(fd->param_vardecls))->v,atts);});
# 2989
for(0;atts != 0;atts=atts->tl){
void*_stmttmpA2=(void*)atts->hd;void*_tmp4E4=_stmttmpA2;int _tmp4E5;int _tmp4E6;switch(*((int*)_tmp4E4)){case 21U: _LL9: _tmp4E6=((struct Cyc_Absyn_Noliveunique_att_Absyn_Attribute_struct*)_tmp4E4)->f1;_LLA: {int i=_tmp4E6;
# 2992
struct Cyc_Absyn_Exp*bogus_exp=({Cyc_Absyn_signed_int_exp(- 1,0U);});
struct Cyc_Absyn_Vardecl*vd=({({struct Cyc_Absyn_Vardecl*(*_tmp725)(struct Cyc_List_List*x,int n)=({struct Cyc_Absyn_Vardecl*(*_tmp4F1)(struct Cyc_List_List*x,int n)=(struct Cyc_Absyn_Vardecl*(*)(struct Cyc_List_List*x,int n))Cyc_List_nth;_tmp4F1;});struct Cyc_List_List*_tmp724=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(fd->param_vardecls))->v;_tmp725(_tmp724,i - 1);});});
void*t=({Cyc_Tcutil_compress(vd->type);});
void*elttype=({Cyc_Tcutil_pointer_elt_type(t);});
void*rval=({void*(*_tmp4EB)(struct Cyc_CfFlowInfo_FlowEnv*fenv,void*t,struct Cyc_Absyn_Exp*consumer,int iteration,void*)=Cyc_CfFlowInfo_make_unique_consumed;struct Cyc_CfFlowInfo_FlowEnv*_tmp4EC=fenv;void*_tmp4ED=elttype;struct Cyc_Absyn_Exp*_tmp4EE=bogus_exp;int _tmp4EF=- 1;void*_tmp4F0=({Cyc_CfFlowInfo_typ_to_absrval(fenv,elttype,0,fenv->unknown_all);});_tmp4EB(_tmp4EC,_tmp4ED,_tmp4EE,_tmp4EF,_tmp4F0);});
# 2999
struct Cyc_CfFlowInfo_InitParam_CfFlowInfo_Root_struct*r=({struct Cyc_CfFlowInfo_InitParam_CfFlowInfo_Root_struct*_tmp4EA=_cycalloc(sizeof(*_tmp4EA));_tmp4EA->tag=2U,_tmp4EA->f1=i,_tmp4EA->f2=t;_tmp4EA;});
struct Cyc_CfFlowInfo_Place*rp=({struct Cyc_CfFlowInfo_Place*_tmp4E9=_cycalloc(sizeof(*_tmp4E9));_tmp4E9->root=(void*)r,_tmp4E9->path=0;_tmp4E9;});
d=({Cyc_Dict_insert(d,(void*)r,rval);});
d=({({struct Cyc_Dict_Dict _tmp727=d;void*_tmp726=(void*)({struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct*_tmp4E7=_cycalloc(sizeof(*_tmp4E7));_tmp4E7->tag=0U,_tmp4E7->f1=vd;_tmp4E7;});Cyc_Dict_insert(_tmp727,_tmp726,(void*)({struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct*_tmp4E8=_cycalloc(sizeof(*_tmp4E8));_tmp4E8->tag=4U,_tmp4E8->f1=rp;_tmp4E8;}));});});
goto _LL8;}case 20U: _LLB: _tmp4E5=((struct Cyc_Absyn_Initializes_att_Absyn_Attribute_struct*)_tmp4E4)->f1;_LLC: {int i=_tmp4E5;
# 3005
struct Cyc_Absyn_Vardecl*vd=({({struct Cyc_Absyn_Vardecl*(*_tmp729)(struct Cyc_List_List*x,int n)=({struct Cyc_Absyn_Vardecl*(*_tmp4FB)(struct Cyc_List_List*x,int n)=(struct Cyc_Absyn_Vardecl*(*)(struct Cyc_List_List*x,int n))Cyc_List_nth;_tmp4FB;});struct Cyc_List_List*_tmp728=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(fd->param_vardecls))->v;_tmp729(_tmp728,i - 1);});});
void*elttype=({Cyc_Tcutil_pointer_elt_type(vd->type);});
struct Cyc_CfFlowInfo_InitParam_CfFlowInfo_Root_struct*r=({struct Cyc_CfFlowInfo_InitParam_CfFlowInfo_Root_struct*_tmp4FA=_cycalloc(sizeof(*_tmp4FA));_tmp4FA->tag=2U,_tmp4FA->f1=i,_tmp4FA->f2=elttype;_tmp4FA;});
struct Cyc_CfFlowInfo_Place*rp=({struct Cyc_CfFlowInfo_Place*_tmp4F9=_cycalloc(sizeof(*_tmp4F9));_tmp4F9->root=(void*)r,_tmp4F9->path=0;_tmp4F9;});
d=({struct Cyc_Dict_Dict(*_tmp4F2)(struct Cyc_Dict_Dict d,void*k,void*v)=Cyc_Dict_insert;struct Cyc_Dict_Dict _tmp4F3=d;void*_tmp4F4=(void*)r;void*_tmp4F5=({Cyc_CfFlowInfo_typ_to_absrval(fenv,elttype,0,fenv->esc_none);});_tmp4F2(_tmp4F3,_tmp4F4,_tmp4F5);});
d=({({struct Cyc_Dict_Dict _tmp72B=d;void*_tmp72A=(void*)({struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct*_tmp4F6=_cycalloc(sizeof(*_tmp4F6));_tmp4F6->tag=0U,_tmp4F6->f1=vd;_tmp4F6;});Cyc_Dict_insert(_tmp72B,_tmp72A,(void*)({struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct*_tmp4F7=_cycalloc(sizeof(*_tmp4F7));_tmp4F7->tag=4U,_tmp4F7->f1=rp;_tmp4F7;}));});});
param_roots=({struct Cyc_List_List*_tmp4F8=_cycalloc(sizeof(*_tmp4F8));_tmp4F8->hd=rp,_tmp4F8->tl=param_roots;_tmp4F8;});
goto _LL8;}default: _LLD: _LLE:
 goto _LL8;}_LL8:;}
# 3016
inflow=({Cyc_CfFlowInfo_ReachableFL(d,relns);});{
# 3018
int noreturn=({int(*_tmp509)(void*)=Cyc_Tcutil_is_noreturn_fn_type;void*_tmp50A=({Cyc_Tcutil_fndecl2type(fd);});_tmp509(_tmp50A);});
struct Cyc_Hashtable_Table*flow_table=({({struct Cyc_Hashtable_Table*(*_tmp72D)(int sz,int(*cmp)(struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt*),int(*hash)(struct Cyc_Absyn_Stmt*))=({
struct Cyc_Hashtable_Table*(*_tmp506)(int sz,int(*cmp)(struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt*),int(*hash)(struct Cyc_Absyn_Stmt*))=(struct Cyc_Hashtable_Table*(*)(int sz,int(*cmp)(struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt*),int(*hash)(struct Cyc_Absyn_Stmt*)))Cyc_Hashtable_create;_tmp506;});int(*_tmp72C)(struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt*)=({int(*_tmp507)(struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt*)=(int(*)(struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt*))Cyc_Core_ptrcmp;_tmp507;});_tmp72D(33,_tmp72C,({int(*_tmp508)(struct Cyc_Absyn_Stmt*s)=(int(*)(struct Cyc_Absyn_Stmt*s))Cyc_NewControlFlow_hash_ptr;_tmp508;}));});});
struct Cyc_NewControlFlow_AnalEnv*env=({struct Cyc_NewControlFlow_AnalEnv*_tmp505=_cycalloc(sizeof(*_tmp505));
_tmp505->all_tables=tables,({
struct Cyc_Hashtable_Table*_tmp733=({({struct Cyc_Hashtable_Table*(*_tmp732)(struct Cyc_Hashtable_Table*t,struct Cyc_Absyn_Fndecl*key)=({struct Cyc_Hashtable_Table*(*_tmp503)(struct Cyc_Hashtable_Table*t,struct Cyc_Absyn_Fndecl*key)=(struct Cyc_Hashtable_Table*(*)(struct Cyc_Hashtable_Table*t,struct Cyc_Absyn_Fndecl*key))Cyc_Hashtable_lookup;_tmp503;});struct Cyc_Hashtable_Table*_tmp731=tables->succ_tables;_tmp732(_tmp731,fd);});});_tmp505->succ_table=_tmp733;}),({
struct Cyc_Hashtable_Table*_tmp730=({({struct Cyc_Hashtable_Table*(*_tmp72F)(struct Cyc_Hashtable_Table*t,struct Cyc_Absyn_Fndecl*key)=({struct Cyc_Hashtable_Table*(*_tmp504)(struct Cyc_Hashtable_Table*t,struct Cyc_Absyn_Fndecl*key)=(struct Cyc_Hashtable_Table*(*)(struct Cyc_Hashtable_Table*t,struct Cyc_Absyn_Fndecl*key))Cyc_Hashtable_lookup;_tmp504;});struct Cyc_Hashtable_Table*_tmp72E=tables->pat_pop_tables;_tmp72F(_tmp72E,fd);});});_tmp505->pat_pop_table=_tmp730;}),_tmp505->fenv=fenv,_tmp505->iterate_again=1,_tmp505->iteration_num=0,_tmp505->in_try=0,_tmp505->tryflow=inflow,_tmp505->noreturn=noreturn,_tmp505->return_type=(fd->i).ret_type,_tmp505->unique_pat_vars=0,_tmp505->param_roots=param_roots,_tmp505->noconsume_params=noconsume_roots,_tmp505->flow_table=flow_table,_tmp505->return_relns=(fd->i).ensures_relns;_tmp505;});
# 3028
union Cyc_CfFlowInfo_FlowInfo outflow=inflow;
while(env->iterate_again && !Cyc_CfFlowInfo_anal_error){
++ env->iteration_num;
# 3034
env->iterate_again=0;
outflow=({Cyc_NewControlFlow_anal_stmt(env,inflow,fd->body,0);});}{
# 3037
union Cyc_CfFlowInfo_FlowInfo _tmp4FC=outflow;if((_tmp4FC.BottomFL).tag == 1){_LL10: _LL11:
 goto _LLF;}else{_LL12: _LL13:
# 3040
({Cyc_NewControlFlow_check_init_params(loc,env,outflow);});
({Cyc_NewControlFlow_unconsume_params(env,env->noconsume_params,0,0,outflow,loc);});
# 3044
if(noreturn)
({void*_tmp4FD=0U;({unsigned _tmp735=loc;struct _fat_ptr _tmp734=({const char*_tmp4FE="`noreturn' function might (implicitly) return";_tag_fat(_tmp4FE,sizeof(char),46U);});Cyc_CfFlowInfo_aerr(_tmp735,_tmp734,_tag_fat(_tmp4FD,sizeof(void*),0U));});});else{
if(!({Cyc_Tcutil_is_void_type((fd->i).ret_type);})){
if(({Cyc_Tcutil_is_any_float_type((fd->i).ret_type);})||({Cyc_Tcutil_is_any_int_type((fd->i).ret_type);}))
# 3049
({void*_tmp4FF=0U;({unsigned _tmp737=loc;struct _fat_ptr _tmp736=({const char*_tmp500="function may not return a value";_tag_fat(_tmp500,sizeof(char),32U);});Cyc_Warn_warn(_tmp737,_tmp736,_tag_fat(_tmp4FF,sizeof(void*),0U));});});else{
# 3051
({void*_tmp501=0U;({unsigned _tmp739=loc;struct _fat_ptr _tmp738=({const char*_tmp502="function may not return a value";_tag_fat(_tmp502,sizeof(char),32U);});Cyc_CfFlowInfo_aerr(_tmp739,_tmp738,_tag_fat(_tmp501,sizeof(void*),0U));});});}goto _LLF;}}
# 3044
goto _LLF;}_LLF:;}}}}}}}
# 3057
void Cyc_NewControlFlow_cf_check(struct Cyc_JumpAnalysis_Jump_Anal_Result*tables,struct Cyc_List_List*ds){
for(0;ds != 0;ds=ds->tl){
Cyc_CfFlowInfo_anal_error=0;{
void*_stmttmpA3=((struct Cyc_Absyn_Decl*)ds->hd)->r;void*_tmp50D=_stmttmpA3;struct Cyc_List_List*_tmp50E;struct Cyc_List_List*_tmp50F;struct Cyc_List_List*_tmp510;struct Cyc_Absyn_Fndecl*_tmp511;switch(*((int*)_tmp50D)){case 1U: _LL1: _tmp511=((struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*)_tmp50D)->f1;_LL2: {struct Cyc_Absyn_Fndecl*fd=_tmp511;
({Cyc_NewControlFlow_check_fun(tables,fd);});goto _LL0;}case 11U: _LL3: _tmp510=((struct Cyc_Absyn_ExternC_d_Absyn_Raw_decl_struct*)_tmp50D)->f1;_LL4: {struct Cyc_List_List*ds2=_tmp510;
# 3063
_tmp50F=ds2;goto _LL6;}case 10U: _LL5: _tmp50F=((struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct*)_tmp50D)->f2;_LL6: {struct Cyc_List_List*ds2=_tmp50F;
_tmp50E=ds2;goto _LL8;}case 9U: _LL7: _tmp50E=((struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct*)_tmp50D)->f2;_LL8: {struct Cyc_List_List*ds2=_tmp50E;
({Cyc_NewControlFlow_cf_check(tables,ds2);});goto _LL0;}case 12U: _LL9: _LLA:
 goto _LL0;default: _LLB: _LLC:
 goto _LL0;}_LL0:;}}}

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
# 73
struct _fat_ptr Cyc_aprintf(struct _fat_ptr,struct _fat_ptr);
# 100
int Cyc_fprintf(struct Cyc___cycFILE*,struct _fat_ptr,struct _fat_ptr);struct Cyc_ShortPtr_sa_ScanfArg_struct{int tag;short*f1;};struct Cyc_UShortPtr_sa_ScanfArg_struct{int tag;unsigned short*f1;};struct Cyc_IntPtr_sa_ScanfArg_struct{int tag;int*f1;};struct Cyc_UIntPtr_sa_ScanfArg_struct{int tag;unsigned*f1;};struct Cyc_StringPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_DoublePtr_sa_ScanfArg_struct{int tag;double*f1;};struct Cyc_FloatPtr_sa_ScanfArg_struct{int tag;float*f1;};struct Cyc_CharPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};extern char Cyc_FileCloseError[15U];struct Cyc_FileCloseError_exn_struct{char*tag;};extern char Cyc_FileOpenError[14U];struct Cyc_FileOpenError_exn_struct{char*tag;struct _fat_ptr f1;};struct Cyc_Core_Opt{void*v;};struct _tuple0{void*f1;void*f2;};
# 111 "core.h"
void*Cyc_Core_snd(struct _tuple0*);extern char Cyc_Core_Invalid_argument[17U];struct Cyc_Core_Invalid_argument_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Failure[8U];struct Cyc_Core_Failure_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Impossible[11U];struct Cyc_Core_Impossible_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Not_found[10U];struct Cyc_Core_Not_found_exn_struct{char*tag;};extern char Cyc_Core_Unreachable[12U];struct Cyc_Core_Unreachable_exn_struct{char*tag;struct _fat_ptr f1;};
# 165
extern struct _RegionHandle*Cyc_Core_heap_region;
# 168
extern struct _RegionHandle*Cyc_Core_unique_region;struct Cyc_Core_DynamicRegion;struct Cyc_Core_NewDynamicRegion{struct Cyc_Core_DynamicRegion*key;};struct Cyc_Core_ThinRes{void*arr;unsigned nelts;};struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};
# 54 "list.h"
struct Cyc_List_List*Cyc_List_list(struct _fat_ptr);
# 61
int Cyc_List_length(struct Cyc_List_List*x);
# 76
struct Cyc_List_List*Cyc_List_map(void*(*f)(void*),struct Cyc_List_List*x);
# 83
struct Cyc_List_List*Cyc_List_map_c(void*(*f)(void*,void*),void*env,struct Cyc_List_List*x);
# 86
struct Cyc_List_List*Cyc_List_rmap_c(struct _RegionHandle*,void*(*f)(void*,void*),void*env,struct Cyc_List_List*x);extern char Cyc_List_List_mismatch[14U];struct Cyc_List_List_mismatch_exn_struct{char*tag;};
# 172
struct Cyc_List_List*Cyc_List_rev(struct Cyc_List_List*x);
# 178
struct Cyc_List_List*Cyc_List_imp_rev(struct Cyc_List_List*x);
# 184
struct Cyc_List_List*Cyc_List_append(struct Cyc_List_List*x,struct Cyc_List_List*y);
# 190
struct Cyc_List_List*Cyc_List_rappend(struct _RegionHandle*,struct Cyc_List_List*x,struct Cyc_List_List*y);
# 195
struct Cyc_List_List*Cyc_List_imp_append(struct Cyc_List_List*x,struct Cyc_List_List*y);extern char Cyc_List_Nth[4U];struct Cyc_List_Nth_exn_struct{char*tag;};
# 276
struct Cyc_List_List*Cyc_List_rzip(struct _RegionHandle*r1,struct _RegionHandle*r2,struct Cyc_List_List*x,struct Cyc_List_List*y);struct _tuple1{struct Cyc_List_List*f1;struct Cyc_List_List*f2;};
# 294
struct _tuple1 Cyc_List_split(struct Cyc_List_List*x);
# 379
struct Cyc_List_List*Cyc_List_tabulate_c(int n,void*(*f)(void*,int),void*env);struct Cyc_Iter_Iter{void*env;int(*next)(void*env,void*dest);};struct Cyc_Dict_T;struct Cyc_Dict_Dict{int(*rel)(void*,void*);struct _RegionHandle*r;const struct Cyc_Dict_T*t;};extern char Cyc_Dict_Present[8U];struct Cyc_Dict_Present_exn_struct{char*tag;};extern char Cyc_Dict_Absent[7U];struct Cyc_Dict_Absent_exn_struct{char*tag;};struct Cyc_Set_Set;
# 54 "set.h"
struct Cyc_Set_Set*Cyc_Set_rempty(struct _RegionHandle*r,int(*cmp)(void*,void*));
# 63
struct Cyc_Set_Set*Cyc_Set_insert(struct Cyc_Set_Set*s,void*elt);
# 94
int Cyc_Set_cardinality(struct Cyc_Set_Set*s);
# 97
int Cyc_Set_is_empty(struct Cyc_Set_Set*s);
# 100
int Cyc_Set_member(struct Cyc_Set_Set*s,void*elt);extern char Cyc_Set_Absent[7U];struct Cyc_Set_Absent_exn_struct{char*tag;};
# 137
void*Cyc_Set_choose(struct Cyc_Set_Set*s);
# 49 "string.h"
int Cyc_strcmp(struct _fat_ptr s1,struct _fat_ptr s2);
int Cyc_strptrcmp(struct _fat_ptr*s1,struct _fat_ptr*s2);
# 64
struct _fat_ptr Cyc_strconcat_l(struct Cyc_List_List*);struct Cyc_PP_Ppstate;struct Cyc_PP_Out;struct Cyc_PP_Doc;struct Cyc_Position_Error;struct Cyc_Relations_Reln;struct _union_Nmspace_Rel_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Abs_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_C_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Loc_n{int tag;int val;};union Cyc_Absyn_Nmspace{struct _union_Nmspace_Rel_n Rel_n;struct _union_Nmspace_Abs_n Abs_n;struct _union_Nmspace_C_n C_n;struct _union_Nmspace_Loc_n Loc_n;};struct _tuple2{union Cyc_Absyn_Nmspace f1;struct _fat_ptr*f2;};
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
enum Cyc_Absyn_KindQual{Cyc_Absyn_AnyKind =0U,Cyc_Absyn_MemKind =1U,Cyc_Absyn_BoxKind =2U,Cyc_Absyn_XRgnKind =3U,Cyc_Absyn_RgnKind =4U,Cyc_Absyn_EffKind =5U,Cyc_Absyn_IntKind =6U,Cyc_Absyn_BoolKind =7U,Cyc_Absyn_PtrBndKind =8U};struct Cyc_Absyn_Kind{enum Cyc_Absyn_KindQual kind;enum Cyc_Absyn_AliasQual aliasqual;};struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct{int tag;struct Cyc_Absyn_Kind*f1;};struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct{int tag;struct Cyc_Core_Opt*f1;};struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct{int tag;struct Cyc_Core_Opt*f1;struct Cyc_Absyn_Kind*f2;};struct Cyc_Absyn_Tvar{struct _fat_ptr*name;int identity;void*kind;};struct Cyc_Absyn_PtrLoc{unsigned ptr_loc;unsigned rgn_loc;unsigned zt_loc;};struct Cyc_Absyn_PtrAtts{void*rgn;void*nullable;void*bounds;void*zero_term;struct Cyc_Absyn_PtrLoc*ptrloc;};struct Cyc_Absyn_PtrInfo{void*elt_type;struct Cyc_Absyn_Tqual elt_tq;struct Cyc_Absyn_PtrAtts ptr_atts;};struct Cyc_Absyn_VarargInfo{struct _fat_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;int inject;};struct Cyc_Absyn_Star_Absyn_Cap_struct{int tag;};struct Cyc_Absyn_Bot_Absyn_Cap_struct{int tag;};struct Cyc_Absyn_Nat_Absyn_Cap_struct{int tag;int f1;int f2;};struct Cyc_Absyn_RgnEffect{void*name;struct Cyc_List_List*caps;void*parent;};struct Cyc_Absyn_FnInfo{struct Cyc_List_List*tvars;void*effect;struct Cyc_Absyn_Tqual ret_tqual;void*ret_type;struct Cyc_List_List*args;int c_varargs;struct Cyc_Absyn_VarargInfo*cyc_varargs;struct Cyc_List_List*rgn_po;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*requires_clause;struct Cyc_List_List*requires_relns;struct Cyc_Absyn_Exp*ensures_clause;struct Cyc_List_List*ensures_relns;struct Cyc_List_List*ieffect;struct Cyc_List_List*oeffect;struct Cyc_List_List*throws;int reentrant;};struct Cyc_Absyn_UnknownDatatypeInfo{struct _tuple2*name;int is_extensible;};struct _union_DatatypeInfo_UnknownDatatype{int tag;struct Cyc_Absyn_UnknownDatatypeInfo val;};struct _union_DatatypeInfo_KnownDatatype{int tag;struct Cyc_Absyn_Datatypedecl**val;};union Cyc_Absyn_DatatypeInfo{struct _union_DatatypeInfo_UnknownDatatype UnknownDatatype;struct _union_DatatypeInfo_KnownDatatype KnownDatatype;};
# 347
union Cyc_Absyn_DatatypeInfo Cyc_Absyn_KnownDatatype(struct Cyc_Absyn_Datatypedecl**);struct Cyc_Absyn_UnknownDatatypeFieldInfo{struct _tuple2*datatype_name;struct _tuple2*field_name;int is_extensible;};struct _union_DatatypeFieldInfo_UnknownDatatypefield{int tag;struct Cyc_Absyn_UnknownDatatypeFieldInfo val;};struct _tuple3{struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;};struct _union_DatatypeFieldInfo_KnownDatatypefield{int tag;struct _tuple3 val;};union Cyc_Absyn_DatatypeFieldInfo{struct _union_DatatypeFieldInfo_UnknownDatatypefield UnknownDatatypefield;struct _union_DatatypeFieldInfo_KnownDatatypefield KnownDatatypefield;};
# 360
union Cyc_Absyn_DatatypeFieldInfo Cyc_Absyn_KnownDatatypefield(struct Cyc_Absyn_Datatypedecl*,struct Cyc_Absyn_Datatypefield*);struct _tuple4{enum Cyc_Absyn_AggrKind f1;struct _tuple2*f2;struct Cyc_Core_Opt*f3;};struct _union_AggrInfo_UnknownAggr{int tag;struct _tuple4 val;};struct _union_AggrInfo_KnownAggr{int tag;struct Cyc_Absyn_Aggrdecl**val;};union Cyc_Absyn_AggrInfo{struct _union_AggrInfo_UnknownAggr UnknownAggr;struct _union_AggrInfo_KnownAggr KnownAggr;};
# 367
union Cyc_Absyn_AggrInfo Cyc_Absyn_KnownAggr(struct Cyc_Absyn_Aggrdecl**);struct Cyc_Absyn_ArrayInfo{void*elt_type;struct Cyc_Absyn_Tqual tq;struct Cyc_Absyn_Exp*num_elts;void*zero_term;unsigned zt_loc;};struct Cyc_Absyn_Aggr_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Enum_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;};struct Cyc_Absyn_Datatype_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;};struct Cyc_Absyn_TypeDecl{void*r;unsigned loc;};struct Cyc_Absyn_VoidCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_IntCon_Absyn_TyCon_struct{int tag;enum Cyc_Absyn_Sign f1;enum Cyc_Absyn_Size_of f2;};struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct{int tag;int f1;};struct Cyc_Absyn_RgnHandleCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_XRgnHandleCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_TagCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_HeapCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_UniqueCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_RefCntCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_AccessCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_CAccessCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_JoinCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_RgnsCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_TrueCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_FalseCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_ThinCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_FatCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct{int tag;struct _tuple2*f1;struct Cyc_Absyn_Enumdecl*f2;};struct Cyc_Absyn_AnonEnumCon_Absyn_TyCon_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_BuiltinCon_Absyn_TyCon_struct{int tag;struct _fat_ptr f1;struct Cyc_Absyn_Kind*f2;};struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_DatatypeInfo f1;};struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_DatatypeFieldInfo f1;};struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_AggrInfo f1;};struct Cyc_Absyn_AppType_Absyn_Type_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Evar_Absyn_Type_struct{int tag;struct Cyc_Core_Opt*f1;void*f2;int f3;struct Cyc_Core_Opt*f4;};struct Cyc_Absyn_VarType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Tvar*f1;};struct Cyc_Absyn_PointerType_Absyn_Type_struct{int tag;struct Cyc_Absyn_PtrInfo f1;};struct Cyc_Absyn_ArrayType_Absyn_Type_struct{int tag;struct Cyc_Absyn_ArrayInfo f1;};struct Cyc_Absyn_FnType_Absyn_Type_struct{int tag;struct Cyc_Absyn_FnInfo f1;};struct Cyc_Absyn_TupleType_Absyn_Type_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct{int tag;enum Cyc_Absyn_AggrKind f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_TypedefType_Absyn_Type_struct{int tag;struct _tuple2*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_Typedefdecl*f3;void*f4;};struct Cyc_Absyn_ValueofType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct{int tag;struct Cyc_Absyn_TypeDecl*f1;void**f2;};struct Cyc_Absyn_TypeofType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_NoTypes_Absyn_Funcparams_struct{int tag;struct Cyc_List_List*f1;unsigned f2;};struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct{int tag;struct Cyc_List_List*f1;int f2;struct Cyc_Absyn_VarargInfo*f3;void*f4;struct Cyc_List_List*f5;struct Cyc_Absyn_Exp*f6;struct Cyc_Absyn_Exp*f7;struct Cyc_List_List*f8;struct Cyc_List_List*f9;struct Cyc_List_List*f10;int f11;};
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
# 954
int Cyc_Absyn_qvar_cmp(struct _tuple2*,struct _tuple2*);
# 962
struct Cyc_Absyn_Tqual Cyc_Absyn_const_tqual(unsigned);
struct Cyc_Absyn_Tqual Cyc_Absyn_empty_tqual(unsigned);
# 968
void*Cyc_Absyn_compress_kb(void*);
# 979
void*Cyc_Absyn_new_evar(struct Cyc_Core_Opt*k,struct Cyc_Core_Opt*tenv);
# 981
void*Cyc_Absyn_wildtyp(struct Cyc_Core_Opt*);
# 984
extern void*Cyc_Absyn_char_type;extern void*Cyc_Absyn_uint_type;
# 986
extern void*Cyc_Absyn_sint_type;
# 989
void*Cyc_Absyn_gen_float_type(unsigned i);
# 991
extern void*Cyc_Absyn_unique_rgn_type;
# 993
extern void*Cyc_Absyn_empty_effect;
# 995
extern void*Cyc_Absyn_true_type;extern void*Cyc_Absyn_false_type;
# 997
extern void*Cyc_Absyn_void_type;void*Cyc_Absyn_enum_type(struct _tuple2*n,struct Cyc_Absyn_Enumdecl*d);
# 1026
void*Cyc_Absyn_bounds_one();
# 1053
void*Cyc_Absyn_datatype_type(union Cyc_Absyn_DatatypeInfo,struct Cyc_List_List*args);
void*Cyc_Absyn_datatype_field_type(union Cyc_Absyn_DatatypeFieldInfo,struct Cyc_List_List*args);
void*Cyc_Absyn_aggr_type(union Cyc_Absyn_AggrInfo,struct Cyc_List_List*args);
# 1061
struct Cyc_Absyn_Exp*Cyc_Absyn_const_exp(union Cyc_Absyn_Cnst,unsigned);
# 1105
struct Cyc_Absyn_Exp*Cyc_Absyn_address_exp(struct Cyc_Absyn_Exp*,unsigned);
# 1109
struct Cyc_Absyn_Exp*Cyc_Absyn_deref_exp(struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_aggrmember_exp(struct Cyc_Absyn_Exp*,struct _fat_ptr*,unsigned);
# 1112
struct Cyc_Absyn_Exp*Cyc_Absyn_subscript_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
# 1128
struct Cyc_Absyn_Stmt*Cyc_Absyn_skip_stmt(unsigned);
# 1225
struct Cyc_Absyn_Aggrdecl*Cyc_Absyn_get_known_aggrdecl(union Cyc_Absyn_AggrInfo);struct Cyc_Absynpp_Params{int expand_typedefs;int qvar_to_Cids;int add_cyc_prefix;int to_VC;int decls_first;int rewrite_temp_tvars;int print_all_tvars;int print_all_kinds;int print_all_effects;int print_using_stmts;int print_externC_stmts;int print_full_evars;int print_zeroterm;int generate_line_directives;int use_curr_namespace;struct Cyc_List_List*curr_namespace;};
# 62 "absynpp.h"
struct _fat_ptr Cyc_Absynpp_typ2string(void*);
# 64
struct _fat_ptr Cyc_Absynpp_kind2string(struct Cyc_Absyn_Kind*);
struct _fat_ptr Cyc_Absynpp_kindbound2string(void*);
# 67
struct _fat_ptr Cyc_Absynpp_exp2string(struct Cyc_Absyn_Exp*);
struct _fat_ptr Cyc_Absynpp_stmt2string(struct Cyc_Absyn_Stmt*);
struct _fat_ptr Cyc_Absynpp_qvar2string(struct _tuple2*);
# 72
struct _fat_ptr Cyc_Absynpp_pat2string(struct Cyc_Absyn_Pat*p);
# 29 "unify.h"
int Cyc_Unify_unify(void*,void*);struct _union_RelnOp_RConst{int tag;unsigned val;};struct _union_RelnOp_RVar{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RNumelts{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RType{int tag;void*val;};struct _union_RelnOp_RParam{int tag;unsigned val;};struct _union_RelnOp_RParamNumelts{int tag;unsigned val;};struct _union_RelnOp_RReturn{int tag;unsigned val;};union Cyc_Relations_RelnOp{struct _union_RelnOp_RConst RConst;struct _union_RelnOp_RVar RVar;struct _union_RelnOp_RNumelts RNumelts;struct _union_RelnOp_RType RType;struct _union_RelnOp_RParam RParam;struct _union_RelnOp_RParamNumelts RParamNumelts;struct _union_RelnOp_RReturn RReturn;};
# 50 "relations-ap.h"
enum Cyc_Relations_Relation{Cyc_Relations_Req =0U,Cyc_Relations_Rneq =1U,Cyc_Relations_Rlte =2U,Cyc_Relations_Rlt =3U};struct Cyc_Relations_Reln{union Cyc_Relations_RelnOp rop1;enum Cyc_Relations_Relation relation;union Cyc_Relations_RelnOp rop2;};struct Cyc_RgnOrder_RgnPO;extern char Cyc_Tcenv_Env_error[10U];struct Cyc_Tcenv_Env_error_exn_struct{char*tag;};struct Cyc_Tcenv_Genv{struct Cyc_Dict_Dict aggrdecls;struct Cyc_Dict_Dict datatypedecls;struct Cyc_Dict_Dict enumdecls;struct Cyc_Dict_Dict typedefs;struct Cyc_Dict_Dict ordinaries;};struct Cyc_Tcenv_Fenv;struct Cyc_Tcenv_Tenv{struct Cyc_List_List*ns;struct Cyc_Tcenv_Genv*ae;struct Cyc_Tcenv_Fenv*le;int allow_valueof: 1;int in_extern_c_include: 1;int in_tempest: 1;int tempest_generalize: 1;};
# 90 "tcenv.h"
enum Cyc_Tcenv_NewStatus{Cyc_Tcenv_NoneNew =0U,Cyc_Tcenv_InNew =1U,Cyc_Tcenv_InNewAggr =2U};
# 100
struct Cyc_List_List*Cyc_Tcenv_lookup_type_vars(struct Cyc_Tcenv_Tenv*);
struct Cyc_Core_Opt*Cyc_Tcenv_lookup_opt_type_vars(struct Cyc_Tcenv_Tenv*);
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_add_type_vars(unsigned,struct Cyc_Tcenv_Tenv*,struct Cyc_List_List*);
# 141
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_add_region(struct Cyc_Tcenv_Tenv*,void*,int opened);
# 143
void Cyc_Tcenv_check_rgn_accessible(struct Cyc_Tcenv_Tenv*,unsigned,void*rgn);
# 30 "tcutil.h"
void*Cyc_Tcutil_impos(struct _fat_ptr,struct _fat_ptr);
void Cyc_Tcutil_terr(unsigned,struct _fat_ptr,struct _fat_ptr);
void Cyc_Tcutil_warn(unsigned,struct _fat_ptr,struct _fat_ptr);
# 44
int Cyc_Tcutil_is_arithmetic_type(void*);
# 48
int Cyc_Tcutil_is_array_type(void*);
# 100
struct Cyc_Absyn_Exp*Cyc_Tcutil_deep_copy_exp(int preserve_types,struct Cyc_Absyn_Exp*);
# 103
int Cyc_Tcutil_kind_leq(struct Cyc_Absyn_Kind*,struct Cyc_Absyn_Kind*);
# 109
struct Cyc_Absyn_Kind*Cyc_Tcutil_type_kind(void*);
void*Cyc_Tcutil_compress(void*);
# 117
int Cyc_Tcutil_coerce_assign(struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Exp*,void*);
# 124
int Cyc_Tcutil_subtype(struct Cyc_Tcenv_Tenv*te,struct Cyc_List_List*assume,void*t1,void*t2);
# 140
extern struct Cyc_Absyn_Kind Cyc_Tcutil_ak;
# 151
extern struct Cyc_Absyn_Kind Cyc_Tcutil_tmk;
# 158
extern struct Cyc_Core_Opt Cyc_Tcutil_rko;
extern struct Cyc_Core_Opt Cyc_Tcutil_ako;
# 161
extern struct Cyc_Core_Opt Cyc_Tcutil_mko;
# 167
extern struct Cyc_Core_Opt Cyc_Tcutil_trko;
# 189
void*Cyc_Tcutil_substitute(struct Cyc_List_List*,void*);
# 191
void*Cyc_Tcutil_rsubstitute(struct _RegionHandle*,struct Cyc_List_List*,void*);struct _tuple13{struct Cyc_List_List*f1;struct _RegionHandle*f2;};struct _tuple14{struct Cyc_Absyn_Tvar*f1;void*f2;};
# 211
struct _tuple14*Cyc_Tcutil_r_make_inst_var(struct _tuple13*,struct Cyc_Absyn_Tvar*);
# 218
void Cyc_Tcutil_check_unique_vars(struct Cyc_List_List*,unsigned,struct _fat_ptr err_msg);
# 226
struct Cyc_List_List*Cyc_Tcutil_resolve_aggregate_designators(struct _RegionHandle*,unsigned,struct Cyc_List_List*,enum Cyc_Absyn_AggrKind,struct Cyc_List_List*);
# 237
int Cyc_Tcutil_is_noalias_region(void*r,int must_be_unique);
# 245
int Cyc_Tcutil_is_noalias_path(struct Cyc_Absyn_Exp*);
# 250
int Cyc_Tcutil_is_noalias_pointer_or_aggr(void*);
# 261
struct Cyc_Absyn_Tvar*Cyc_Tcutil_new_tvar(void*);
# 278
int Cyc_Tcutil_is_const_exp(struct Cyc_Absyn_Exp*);
# 303
void*Cyc_Tcutil_promote_array(void*t,void*rgn,int convert_tag);
# 310
int Cyc_Tcutil_force_type2bool(int desired,void*);
# 313
void*Cyc_Tcutil_any_bool(struct Cyc_List_List*);
# 315
void*Cyc_Tcutil_any_bounds(struct Cyc_List_List*);
# 44 "tctyp.h"
void Cyc_Tctyp_check_type(unsigned,struct Cyc_Tcenv_Tenv*,struct Cyc_List_List*bound_tvars,struct Cyc_Absyn_Kind*k,int allow_evars,int allow_abs_aggr,void*);
# 28 "tcexp.h"
void*Cyc_Tcexp_tcExp(struct Cyc_Tcenv_Tenv*,void**,struct Cyc_Absyn_Exp*);struct Cyc_Tcpat_TcPatResult{struct _tuple1*tvars_and_bounds_opt;struct Cyc_List_List*patvars;};struct Cyc_Tcpat_WhereTest_Tcpat_PatTest_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Tcpat_EqNull_Tcpat_PatTest_struct{int tag;};struct Cyc_Tcpat_NeqNull_Tcpat_PatTest_struct{int tag;};struct Cyc_Tcpat_EqEnum_Tcpat_PatTest_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Tcpat_EqAnonEnum_Tcpat_PatTest_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Tcpat_EqFloat_Tcpat_PatTest_struct{int tag;struct _fat_ptr f1;int f2;};struct Cyc_Tcpat_EqConst_Tcpat_PatTest_struct{int tag;unsigned f1;};struct Cyc_Tcpat_EqDatatypeTag_Tcpat_PatTest_struct{int tag;int f1;struct Cyc_Absyn_Datatypedecl*f2;struct Cyc_Absyn_Datatypefield*f3;};struct Cyc_Tcpat_EqTaggedUnion_Tcpat_PatTest_struct{int tag;struct _fat_ptr*f1;int f2;};struct Cyc_Tcpat_EqExtensibleDatatype_Tcpat_PatTest_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;};struct Cyc_Tcpat_Dummy_Tcpat_Access_struct{int tag;};struct Cyc_Tcpat_Deref_Tcpat_Access_struct{int tag;};struct Cyc_Tcpat_TupleField_Tcpat_Access_struct{int tag;unsigned f1;};struct Cyc_Tcpat_DatatypeField_Tcpat_Access_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;unsigned f3;};struct Cyc_Tcpat_AggrField_Tcpat_Access_struct{int tag;int f1;struct _fat_ptr*f2;};struct _union_PatOrWhere_pattern{int tag;struct Cyc_Absyn_Pat*val;};struct _union_PatOrWhere_where_clause{int tag;struct Cyc_Absyn_Exp*val;};union Cyc_Tcpat_PatOrWhere{struct _union_PatOrWhere_pattern pattern;struct _union_PatOrWhere_where_clause where_clause;};struct Cyc_Tcpat_PathNode{union Cyc_Tcpat_PatOrWhere orig_pat;void*access;};struct Cyc_Tcpat_Rhs{int used;unsigned pat_loc;struct Cyc_Absyn_Stmt*rhs;};struct Cyc_Tcpat_Failure_Tcpat_Decision_struct{int tag;void*f1;};struct Cyc_Tcpat_Success_Tcpat_Decision_struct{int tag;struct Cyc_Tcpat_Rhs*f1;};struct Cyc_Tcpat_SwitchDec_Tcpat_Decision_struct{int tag;struct Cyc_List_List*f1;struct Cyc_List_List*f2;void*f3;};struct _tuple15{unsigned f1;int f2;};
# 28 "evexp.h"
struct _tuple15 Cyc_Evexp_eval_const_uint_exp(struct Cyc_Absyn_Exp*e);
# 45 "tcpat.cyc"
static void Cyc_Tcpat_resolve_pat(struct Cyc_Tcenv_Tenv*te,void**topt,struct Cyc_Absyn_Pat*p){
void*_stmttmp0=p->r;void*_tmp0=_stmttmp0;struct Cyc_Absyn_Exp*_tmp1;int _tmp5;struct Cyc_List_List*_tmp4;struct Cyc_List_List**_tmp3;struct Cyc_Absyn_Aggrdecl**_tmp2;int _tmp8;struct Cyc_List_List*_tmp7;struct Cyc_List_List*_tmp6;switch(*((int*)_tmp0)){case 7U: if(((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp0)->f1 == 0){_LL1: _tmp6=((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp0)->f2;_tmp7=((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp0)->f3;_tmp8=((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp0)->f4;_LL2: {struct Cyc_List_List*exist_ts=_tmp6;struct Cyc_List_List*dps=_tmp7;int dots=_tmp8;
# 51
if(topt == 0)
({void*_tmp9=0U;({unsigned _tmp45D=p->loc;struct _fat_ptr _tmp45C=({const char*_tmpA="cannot determine pattern type";_tag_fat(_tmpA,sizeof(char),30U);});Cyc_Tcutil_terr(_tmp45D,_tmp45C,_tag_fat(_tmp9,sizeof(void*),0U));});});{
# 51
void*t=({Cyc_Tcutil_compress(*((void**)_check_null(topt)));});
# 54
{void*_tmpB=t;union Cyc_Absyn_AggrInfo _tmpC;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpB)->tag == 0U){if(((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpB)->f1)->tag == 22U){_LL10: _tmpC=((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpB)->f1)->f1;_LL11: {union Cyc_Absyn_AggrInfo ainfo=_tmpC;
# 56
({void*_tmp45F=(void*)({struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*_tmpE=_cycalloc(sizeof(*_tmpE));_tmpE->tag=7U,({union Cyc_Absyn_AggrInfo*_tmp45E=({union Cyc_Absyn_AggrInfo*_tmpD=_cycalloc(sizeof(*_tmpD));*_tmpD=ainfo;_tmpD;});_tmpE->f1=_tmp45E;}),_tmpE->f2=exist_ts,_tmpE->f3=dps,_tmpE->f4=dots;_tmpE;});p->r=_tmp45F;});
({Cyc_Tcpat_resolve_pat(te,topt,p);});
goto _LLF;}}else{goto _LL12;}}else{_LL12: _LL13:
# 60
({struct Cyc_String_pa_PrintArg_struct _tmp11=({struct Cyc_String_pa_PrintArg_struct _tmp40B;_tmp40B.tag=0U,({struct _fat_ptr _tmp460=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t);}));_tmp40B.f1=_tmp460;});_tmp40B;});void*_tmpF[1U];_tmpF[0]=& _tmp11;({unsigned _tmp462=p->loc;struct _fat_ptr _tmp461=({const char*_tmp10="pattern expects aggregate type instead of %s";_tag_fat(_tmp10,sizeof(char),45U);});Cyc_Tcutil_terr(_tmp462,_tmp461,_tag_fat(_tmpF,sizeof(void*),1U));});});
goto _LLF;}_LLF:;}
# 63
return;}}}else{if((((union Cyc_Absyn_AggrInfo*)((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp0)->f1)->KnownAggr).tag == 2){_LL3: _tmp2=((((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp0)->f1)->KnownAggr).val;_tmp3=(struct Cyc_List_List**)&((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp0)->f2;_tmp4=((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp0)->f3;_tmp5=((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp0)->f4;_LL4: {struct Cyc_Absyn_Aggrdecl**adp=_tmp2;struct Cyc_List_List**exist_ts=_tmp3;struct Cyc_List_List*dps=_tmp4;int dots=_tmp5;
# 66
struct Cyc_Absyn_Aggrdecl*ad=*adp;
if(ad->impl == 0){
({struct Cyc_String_pa_PrintArg_struct _tmp14=({struct Cyc_String_pa_PrintArg_struct _tmp40C;_tmp40C.tag=0U,({
struct _fat_ptr _tmp463=(struct _fat_ptr)((int)ad->kind == (int)Cyc_Absyn_StructA?({const char*_tmp15="struct";_tag_fat(_tmp15,sizeof(char),7U);}):({const char*_tmp16="union";_tag_fat(_tmp16,sizeof(char),6U);}));_tmp40C.f1=_tmp463;});_tmp40C;});void*_tmp12[1U];_tmp12[0]=& _tmp14;({unsigned _tmp465=p->loc;struct _fat_ptr _tmp464=({const char*_tmp13="can't destructure an abstract %s";_tag_fat(_tmp13,sizeof(char),33U);});Cyc_Tcutil_terr(_tmp465,_tmp464,_tag_fat(_tmp12,sizeof(void*),1U));});});
p->r=(void*)& Cyc_Absyn_Wild_p_val;
return;}{
# 67
int more_exists=({
# 73
int _tmp466=({Cyc_List_length(((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->exist_vars);});_tmp466 - ({Cyc_List_length(*exist_ts);});});
if(more_exists < 0){
({void*_tmp17=0U;({unsigned _tmp468=p->loc;struct _fat_ptr _tmp467=({const char*_tmp18="too many existentially bound type variables in pattern";_tag_fat(_tmp18,sizeof(char),55U);});Cyc_Tcutil_terr(_tmp468,_tmp467,_tag_fat(_tmp17,sizeof(void*),0U));});});{
struct Cyc_List_List**ts=exist_ts;
{int n=({Cyc_List_length(((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->exist_vars);});for(0;n != 0;-- n){
ts=&((struct Cyc_List_List*)_check_null(*ts))->tl;}}
*ts=0;}}else{
if(more_exists > 0){
# 82
struct Cyc_List_List*new_ts=0;
for(0;more_exists != 0;-- more_exists){
new_ts=({struct Cyc_List_List*_tmp1A=_cycalloc(sizeof(*_tmp1A));({struct Cyc_Absyn_Tvar*_tmp469=({Cyc_Tcutil_new_tvar((void*)({struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*_tmp19=_cycalloc(sizeof(*_tmp19));_tmp19->tag=1U,_tmp19->f1=0;_tmp19;}));});_tmp1A->hd=_tmp469;}),_tmp1A->tl=new_ts;_tmp1A;});}
({struct Cyc_List_List*_tmp46A=({Cyc_List_imp_append(*exist_ts,new_ts);});*exist_ts=_tmp46A;});}}
# 74
return;}}}else{_LLB: _LLC:
# 100
({void*_tmp26=0U;({int(*_tmp46C)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp28)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp28;});struct _fat_ptr _tmp46B=({const char*_tmp27="resolve_pat unknownAggr";_tag_fat(_tmp27,sizeof(char),24U);});_tmp46C(_tmp46B,_tag_fat(_tmp26,sizeof(void*),0U));});});}}case 17U: _LL5: _tmp1=((struct Cyc_Absyn_Exp_p_Absyn_Raw_pat_struct*)_tmp0)->f1;_LL6: {struct Cyc_Absyn_Exp*e=_tmp1;
# 89
({Cyc_Tcexp_tcExp(te,0,e);});
if(!({Cyc_Tcutil_is_const_exp(e);})){
({void*_tmp1B=0U;({unsigned _tmp46E=p->loc;struct _fat_ptr _tmp46D=({const char*_tmp1C="non-constant expression in case pattern";_tag_fat(_tmp1C,sizeof(char),40U);});Cyc_Tcutil_terr(_tmp46E,_tmp46D,_tag_fat(_tmp1B,sizeof(void*),0U));});});
p->r=(void*)& Cyc_Absyn_Wild_p_val;}{
# 90
struct _tuple15 _stmttmp1=({Cyc_Evexp_eval_const_uint_exp(e);});int _tmp1E;unsigned _tmp1D;_LL15: _tmp1D=_stmttmp1.f1;_tmp1E=_stmttmp1.f2;_LL16: {unsigned i=_tmp1D;int known=_tmp1E;
# 95
({void*_tmp46F=(void*)({struct Cyc_Absyn_Int_p_Absyn_Raw_pat_struct*_tmp1F=_cycalloc(sizeof(*_tmp1F));_tmp1F->tag=10U,_tmp1F->f1=Cyc_Absyn_None,_tmp1F->f2=(int)i;_tmp1F;});p->r=_tmp46F;});
return;}}}case 15U: _LL7: _LL8:
# 98
({void*_tmp20=0U;({int(*_tmp471)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp22)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp22;});struct _fat_ptr _tmp470=({const char*_tmp21="resolve_pat UnknownId_p";_tag_fat(_tmp21,sizeof(char),24U);});_tmp471(_tmp470,_tag_fat(_tmp20,sizeof(void*),0U));});});case 16U: _LL9: _LLA:
({void*_tmp23=0U;({int(*_tmp473)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp25)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp25;});struct _fat_ptr _tmp472=({const char*_tmp24="resolve_pat UnknownCall_p";_tag_fat(_tmp24,sizeof(char),26U);});_tmp473(_tmp472,_tag_fat(_tmp23,sizeof(void*),0U));});});default: _LLD: _LLE:
# 102
 return;}_LL0:;}
# 106
static struct _fat_ptr*Cyc_Tcpat_get_name(struct Cyc_Absyn_Vardecl*vd){
return(*vd->name).f2;}
# 109
static void*Cyc_Tcpat_any_type(struct Cyc_List_List*s,void**topt){
if(topt != 0)
return*topt;
# 110
return({Cyc_Absyn_new_evar(& Cyc_Tcutil_mko,({struct Cyc_Core_Opt*_tmp2B=_cycalloc(sizeof(*_tmp2B));_tmp2B->v=s;_tmp2B;}));});}
# 114
static void*Cyc_Tcpat_num_type(void**topt,void*numt){
# 118
if(topt != 0 &&({Cyc_Tcutil_is_arithmetic_type(*topt);}))
return*topt;
# 118
{void*_stmttmp2=({Cyc_Tcutil_compress(numt);});void*_tmp2D=_stmttmp2;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2D)->tag == 0U)switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2D)->f1)){case 17U: _LL1: _LL2:
# 122
 goto _LL4;case 18U: _LL3: _LL4:
 if(topt != 0)return*topt;goto _LL0;default: goto _LL5;}else{_LL5: _LL6:
 goto _LL0;}_LL0:;}
# 126
return numt;}struct _tuple16{struct Cyc_Absyn_Vardecl**f1;struct Cyc_Absyn_Exp*f2;};
# 129
static void Cyc_Tcpat_set_vd(struct Cyc_Absyn_Vardecl**vd,struct Cyc_Absyn_Exp*e,struct Cyc_List_List**v_result_ptr,void*t){
# 131
if(vd != 0){
(*vd)->type=t;
({struct Cyc_Absyn_Tqual _tmp474=({Cyc_Absyn_empty_tqual(0U);});(*vd)->tq=_tmp474;});}
# 131
({struct Cyc_List_List*_tmp476=({struct Cyc_List_List*_tmp30=_cycalloc(sizeof(*_tmp30));
# 135
({struct _tuple16*_tmp475=({struct _tuple16*_tmp2F=_cycalloc(sizeof(*_tmp2F));_tmp2F->f1=vd,_tmp2F->f2=e;_tmp2F;});_tmp30->hd=_tmp475;}),_tmp30->tl=*v_result_ptr;_tmp30;});
# 131
*v_result_ptr=_tmp476;});}
# 137
static struct Cyc_Tcpat_TcPatResult Cyc_Tcpat_combine_results(struct Cyc_Tcpat_TcPatResult res1,struct Cyc_Tcpat_TcPatResult res2){
# 139
struct Cyc_List_List*_tmp33;struct _tuple1*_tmp32;_LL1: _tmp32=res1.tvars_and_bounds_opt;_tmp33=res1.patvars;_LL2: {struct _tuple1*p1=_tmp32;struct Cyc_List_List*vs1=_tmp33;
struct Cyc_List_List*_tmp35;struct _tuple1*_tmp34;_LL4: _tmp34=res2.tvars_and_bounds_opt;_tmp35=res2.patvars;_LL5: {struct _tuple1*p2=_tmp34;struct Cyc_List_List*vs2=_tmp35;
if(p1 != 0 || p2 != 0){
if(p1 == 0)
p1=({struct _tuple1*_tmp36=_cycalloc(sizeof(*_tmp36));_tmp36->f1=0,_tmp36->f2=0;_tmp36;});
# 142
if(p2 == 0)
# 145
p2=({struct _tuple1*_tmp37=_cycalloc(sizeof(*_tmp37));_tmp37->f1=0,_tmp37->f2=0;_tmp37;});
# 142
return({struct Cyc_Tcpat_TcPatResult _tmp40D;({
# 146
struct _tuple1*_tmp47A=({struct _tuple1*_tmp38=_cycalloc(sizeof(*_tmp38));({struct Cyc_List_List*_tmp479=({Cyc_List_append((*p1).f1,(*p2).f1);});_tmp38->f1=_tmp479;}),({
struct Cyc_List_List*_tmp478=({Cyc_List_append((*p1).f2,(*p2).f2);});_tmp38->f2=_tmp478;});_tmp38;});
# 146
_tmp40D.tvars_and_bounds_opt=_tmp47A;}),({
# 148
struct Cyc_List_List*_tmp477=({Cyc_List_append(vs1,vs2);});_tmp40D.patvars=_tmp477;});_tmp40D;});}
# 141
return({struct Cyc_Tcpat_TcPatResult _tmp40E;_tmp40E.tvars_and_bounds_opt=0,({
# 150
struct Cyc_List_List*_tmp47B=({Cyc_List_append(vs1,vs2);});_tmp40E.patvars=_tmp47B;});_tmp40E;});}}}
# 153
static struct Cyc_Absyn_Pat*Cyc_Tcpat_wild_pat(unsigned loc){
return({struct Cyc_Absyn_Pat*_tmp3A=_cycalloc(sizeof(*_tmp3A));_tmp3A->loc=loc,_tmp3A->topt=0,_tmp3A->r=(void*)& Cyc_Absyn_Wild_p_val;_tmp3A;});}
# 158
static void*Cyc_Tcpat_pat_promote_array(struct Cyc_Tcenv_Tenv*te,void*t,void*rgn_opt){
return({Cyc_Tcutil_is_array_type(t);})?({void*(*_tmp3C)(void*t,void*rgn,int convert_tag)=Cyc_Tcutil_promote_array;void*_tmp3D=t;void*_tmp3E=
rgn_opt == 0?({void*(*_tmp3F)(struct Cyc_Core_Opt*k,struct Cyc_Core_Opt*tenv)=Cyc_Absyn_new_evar;struct Cyc_Core_Opt*_tmp40=& Cyc_Tcutil_rko;struct Cyc_Core_Opt*_tmp41=({struct Cyc_Core_Opt*_tmp42=_cycalloc(sizeof(*_tmp42));({struct Cyc_List_List*_tmp47C=({Cyc_Tcenv_lookup_type_vars(te);});_tmp42->v=_tmp47C;});_tmp42;});_tmp3F(_tmp40,_tmp41);}): rgn_opt;int _tmp43=0;_tmp3C(_tmp3D,_tmp3E,_tmp43);}): t;}struct _tuple17{struct Cyc_Absyn_Tvar*f1;int f2;};
# 165
static struct _tuple17*Cyc_Tcpat_add_false(struct Cyc_Absyn_Tvar*tv){
return({struct _tuple17*_tmp45=_cycalloc(sizeof(*_tmp45));_tmp45->f1=tv,_tmp45->f2=0;_tmp45;});}struct _tuple18{struct Cyc_Absyn_Tqual f1;void*f2;};struct _tuple19{struct Cyc_List_List*f1;struct Cyc_Absyn_Pat*f2;};struct _tuple20{struct Cyc_Absyn_Aggrfield*f1;struct Cyc_Absyn_Pat*f2;};
# 169
static struct Cyc_Tcpat_TcPatResult Cyc_Tcpat_tcPatRec(struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Pat*p,void**topt,void*rgn_pat,int allow_ref_pat,struct Cyc_Absyn_Exp*access_exp){
# 172
({Cyc_Tcpat_resolve_pat(te,topt,p);});{
void*t;
struct Cyc_Tcpat_TcPatResult res=({struct Cyc_Tcpat_TcPatResult _tmp421;_tmp421.tvars_and_bounds_opt=0,_tmp421.patvars=0;_tmp421;});
# 177
{void*_stmttmp3=p->r;void*_tmp47=_stmttmp3;int _tmp4B;struct Cyc_List_List**_tmp4A;struct Cyc_Absyn_Datatypefield*_tmp49;struct Cyc_Absyn_Datatypedecl*_tmp48;int _tmp4F;struct Cyc_List_List**_tmp4E;struct Cyc_List_List*_tmp4D;struct Cyc_Absyn_Aggrdecl*_tmp4C;int _tmp51;struct Cyc_List_List**_tmp50;struct Cyc_Absyn_Pat*_tmp52;void*_tmp53;struct Cyc_Absyn_Enumdecl*_tmp54;int _tmp55;struct Cyc_Absyn_Vardecl*_tmp57;struct Cyc_Absyn_Tvar*_tmp56;struct Cyc_Absyn_Pat*_tmp59;struct Cyc_Absyn_Vardecl*_tmp58;struct Cyc_Absyn_Vardecl*_tmp5B;struct Cyc_Absyn_Tvar*_tmp5A;struct Cyc_Absyn_Pat*_tmp5D;struct Cyc_Absyn_Vardecl*_tmp5C;switch(*((int*)_tmp47)){case 0U: _LL1: _LL2:
# 180
 if(topt != 0)
t=*topt;else{
# 183
t=({void*(*_tmp5E)(struct Cyc_List_List*s,void**topt)=Cyc_Tcpat_any_type;struct Cyc_List_List*_tmp5F=({Cyc_Tcenv_lookup_type_vars(te);});void**_tmp60=topt;_tmp5E(_tmp5F,_tmp60);});}
goto _LL0;case 1U: _LL3: _tmp5C=((struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct*)_tmp47)->f1;_tmp5D=((struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct*)_tmp47)->f2;_LL4: {struct Cyc_Absyn_Vardecl*vd=_tmp5C;struct Cyc_Absyn_Pat*p2=_tmp5D;
# 187
struct _tuple2*_stmttmp4=vd->name;struct _fat_ptr _tmp61;_LL2E: _tmp61=*_stmttmp4->f2;_LL2F: {struct _fat_ptr v=_tmp61;
if(({({struct _fat_ptr _tmp47E=(struct _fat_ptr)v;Cyc_strcmp(_tmp47E,({const char*_tmp62="true";_tag_fat(_tmp62,sizeof(char),5U);}));});})== 0 ||({({struct _fat_ptr _tmp47D=(struct _fat_ptr)v;Cyc_strcmp(_tmp47D,({const char*_tmp63="false";_tag_fat(_tmp63,sizeof(char),6U);}));});})== 0)
({struct Cyc_String_pa_PrintArg_struct _tmp66=({struct Cyc_String_pa_PrintArg_struct _tmp40F;_tmp40F.tag=0U,_tmp40F.f1=(struct _fat_ptr)((struct _fat_ptr)v);_tmp40F;});void*_tmp64[1U];_tmp64[0]=& _tmp66;({unsigned _tmp480=p->loc;struct _fat_ptr _tmp47F=({const char*_tmp65="you probably do not want to use %s as a local variable";_tag_fat(_tmp65,sizeof(char),55U);});Cyc_Tcutil_warn(_tmp480,_tmp47F,_tag_fat(_tmp64,sizeof(void*),1U));});});
# 188
res=({Cyc_Tcpat_tcPatRec(te,p2,topt,rgn_pat,allow_ref_pat,access_exp);});
# 191
t=(void*)_check_null(p2->topt);
# 194
{void*_stmttmp5=({Cyc_Tcutil_compress(t);});void*_tmp67=_stmttmp5;if(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp67)->tag == 4U){_LL31: _LL32:
# 196
 if(rgn_pat == 0 || !allow_ref_pat)
({void*_tmp68=0U;({unsigned _tmp482=p->loc;struct _fat_ptr _tmp481=({const char*_tmp69="array reference would point into unknown/unallowed region";_tag_fat(_tmp69,sizeof(char),58U);});Cyc_Tcutil_terr(_tmp482,_tmp481,_tag_fat(_tmp68,sizeof(void*),0U));});});
# 196
goto _LL30;}else{_LL33: _LL34:
# 200
 if(!({int(*_tmp6A)(struct Cyc_Absyn_Kind*,struct Cyc_Absyn_Kind*)=Cyc_Tcutil_kind_leq;struct Cyc_Absyn_Kind*_tmp6B=({Cyc_Tcutil_type_kind(t);});struct Cyc_Absyn_Kind*_tmp6C=& Cyc_Tcutil_tmk;_tmp6A(_tmp6B,_tmp6C);}))
({void*_tmp6D=0U;({unsigned _tmp484=p->loc;struct _fat_ptr _tmp483=({const char*_tmp6E="pattern would point to an abstract member";_tag_fat(_tmp6E,sizeof(char),42U);});Cyc_Tcutil_terr(_tmp484,_tmp483,_tag_fat(_tmp6D,sizeof(void*),0U));});});
# 200
goto _LL30;}_LL30:;}
# 204
({void(*_tmp6F)(struct Cyc_Absyn_Vardecl**vd,struct Cyc_Absyn_Exp*e,struct Cyc_List_List**v_result_ptr,void*t)=Cyc_Tcpat_set_vd;struct Cyc_Absyn_Vardecl**_tmp70=({struct Cyc_Absyn_Vardecl**_tmp71=_cycalloc(sizeof(*_tmp71));*_tmp71=vd;_tmp71;});struct Cyc_Absyn_Exp*_tmp72=access_exp;struct Cyc_List_List**_tmp73=& res.patvars;void*_tmp74=({Cyc_Tcpat_pat_promote_array(te,t,rgn_pat);});_tmp6F(_tmp70,_tmp72,_tmp73,_tmp74);});
goto _LL0;}}case 2U: _LL5: _tmp5A=((struct Cyc_Absyn_AliasVar_p_Absyn_Raw_pat_struct*)_tmp47)->f1;_tmp5B=((struct Cyc_Absyn_AliasVar_p_Absyn_Raw_pat_struct*)_tmp47)->f2;_LL6: {struct Cyc_Absyn_Tvar*tv=_tmp5A;struct Cyc_Absyn_Vardecl*vd=_tmp5B;
# 207
struct Cyc_Tcenv_Tenv*te2=({struct Cyc_Tcenv_Tenv*(*_tmp8C)(unsigned,struct Cyc_Tcenv_Tenv*,struct Cyc_List_List*)=Cyc_Tcenv_add_type_vars;unsigned _tmp8D=p->loc;struct Cyc_Tcenv_Tenv*_tmp8E=te;struct Cyc_List_List*_tmp8F=({struct Cyc_Absyn_Tvar*_tmp90[1U];_tmp90[0]=tv;Cyc_List_list(_tag_fat(_tmp90,sizeof(struct Cyc_Absyn_Tvar*),1U));});_tmp8C(_tmp8D,_tmp8E,_tmp8F);});
if(res.tvars_and_bounds_opt == 0)
({struct _tuple1*_tmp485=({struct _tuple1*_tmp75=_cycalloc(sizeof(*_tmp75));_tmp75->f1=0,_tmp75->f2=0;_tmp75;});res.tvars_and_bounds_opt=_tmp485;});
# 208
({struct Cyc_List_List*_tmp488=({({struct Cyc_List_List*_tmp487=(*res.tvars_and_bounds_opt).f1;Cyc_List_append(_tmp487,({struct Cyc_List_List*_tmp77=_cycalloc(sizeof(*_tmp77));({struct _tuple17*_tmp486=({struct _tuple17*_tmp76=_cycalloc(sizeof(*_tmp76));_tmp76->f1=tv,_tmp76->f2=1;_tmp76;});_tmp77->hd=_tmp486;}),_tmp77->tl=0;_tmp77;}));});});(*res.tvars_and_bounds_opt).f1=_tmp488;});
# 212
({void(*_tmp78)(unsigned,struct Cyc_Tcenv_Tenv*,struct Cyc_List_List*bound_tvars,struct Cyc_Absyn_Kind*k,int allow_evars,int allow_abs_aggr,void*)=Cyc_Tctyp_check_type;unsigned _tmp79=p->loc;struct Cyc_Tcenv_Tenv*_tmp7A=te2;struct Cyc_List_List*_tmp7B=({Cyc_Tcenv_lookup_type_vars(te2);});struct Cyc_Absyn_Kind*_tmp7C=& Cyc_Tcutil_tmk;int _tmp7D=1;int _tmp7E=0;void*_tmp7F=vd->type;_tmp78(_tmp79,_tmp7A,_tmp7B,_tmp7C,_tmp7D,_tmp7E,_tmp7F);});
# 215
if(topt != 0)t=*topt;else{
t=({void*(*_tmp80)(struct Cyc_List_List*s,void**topt)=Cyc_Tcpat_any_type;struct Cyc_List_List*_tmp81=({Cyc_Tcenv_lookup_type_vars(te);});void**_tmp82=topt;_tmp80(_tmp81,_tmp82);});}
{void*_stmttmp6=({Cyc_Tcutil_compress(t);});void*_tmp83=_stmttmp6;if(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp83)->tag == 4U){_LL36: _LL37:
# 219
 if(rgn_pat == 0 || !allow_ref_pat)
({void*_tmp84=0U;({unsigned _tmp48A=p->loc;struct _fat_ptr _tmp489=({const char*_tmp85="array reference would point into unknown/unallowed region";_tag_fat(_tmp85,sizeof(char),58U);});Cyc_Tcutil_terr(_tmp48A,_tmp489,_tag_fat(_tmp84,sizeof(void*),0U));});});
# 219
goto _LL35;}else{_LL38: _LL39:
# 223
 if(!({int(*_tmp86)(struct Cyc_Absyn_Kind*,struct Cyc_Absyn_Kind*)=Cyc_Tcutil_kind_leq;struct Cyc_Absyn_Kind*_tmp87=({Cyc_Tcutil_type_kind(t);});struct Cyc_Absyn_Kind*_tmp88=& Cyc_Tcutil_tmk;_tmp86(_tmp87,_tmp88);}))
({void*_tmp89=0U;({unsigned _tmp48C=p->loc;struct _fat_ptr _tmp48B=({const char*_tmp8A="pattern would point to an abstract member";_tag_fat(_tmp8A,sizeof(char),42U);});Cyc_Tcutil_terr(_tmp48C,_tmp48B,_tag_fat(_tmp89,sizeof(void*),0U));});});
# 223
goto _LL35;}_LL35:;}
# 227
({({struct Cyc_Absyn_Vardecl**_tmp48F=({struct Cyc_Absyn_Vardecl**_tmp8B=_cycalloc(sizeof(*_tmp8B));*_tmp8B=vd;_tmp8B;});struct Cyc_Absyn_Exp*_tmp48E=access_exp;struct Cyc_List_List**_tmp48D=& res.patvars;Cyc_Tcpat_set_vd(_tmp48F,_tmp48E,_tmp48D,vd->type);});});
goto _LL0;}case 3U: _LL7: _tmp58=((struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct*)_tmp47)->f1;_tmp59=((struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct*)_tmp47)->f2;_LL8: {struct Cyc_Absyn_Vardecl*vd=_tmp58;struct Cyc_Absyn_Pat*p2=_tmp59;
# 231
res=({Cyc_Tcpat_tcPatRec(te,p2,topt,rgn_pat,allow_ref_pat,access_exp);});
t=(void*)_check_null(p2->topt);
if(!allow_ref_pat || rgn_pat == 0){
({void*_tmp91=0U;({unsigned _tmp491=p->loc;struct _fat_ptr _tmp490=({const char*_tmp92="* pattern would point into an unknown/unallowed region";_tag_fat(_tmp92,sizeof(char),55U);});Cyc_Tcutil_terr(_tmp491,_tmp490,_tag_fat(_tmp91,sizeof(void*),0U));});});
goto _LL0;}else{
# 238
if(({Cyc_Tcutil_is_noalias_pointer_or_aggr(t);}))
({void*_tmp93=0U;({unsigned _tmp493=p->loc;struct _fat_ptr _tmp492=({const char*_tmp94="* pattern cannot take the address of an alias-free path";_tag_fat(_tmp94,sizeof(char),56U);});Cyc_Tcutil_terr(_tmp493,_tmp492,_tag_fat(_tmp93,sizeof(void*),0U));});});}{
# 241
struct Cyc_Absyn_Exp*new_access_exp=0;
struct Cyc_List_List*tvs=({Cyc_Tcenv_lookup_type_vars(te);});
void*t2=(void*)({struct Cyc_Absyn_PointerType_Absyn_Type_struct*_tmp96=_cycalloc(sizeof(*_tmp96));_tmp96->tag=3U,(_tmp96->f1).elt_type=t,({struct Cyc_Absyn_Tqual _tmp496=({Cyc_Absyn_empty_tqual(0U);});(_tmp96->f1).elt_tq=_tmp496;}),
((_tmp96->f1).ptr_atts).rgn=rgn_pat,((_tmp96->f1).ptr_atts).nullable=Cyc_Absyn_false_type,({
void*_tmp495=({Cyc_Tcutil_any_bounds(tvs);});((_tmp96->f1).ptr_atts).bounds=_tmp495;}),({void*_tmp494=({Cyc_Tcutil_any_bool(tvs);});((_tmp96->f1).ptr_atts).zero_term=_tmp494;}),((_tmp96->f1).ptr_atts).ptrloc=0;_tmp96;});
# 247
if((unsigned)access_exp){
new_access_exp=({Cyc_Absyn_address_exp(access_exp,0U);});
new_access_exp->topt=t2;}
# 247
({({struct Cyc_Absyn_Vardecl**_tmp499=({struct Cyc_Absyn_Vardecl**_tmp95=_cycalloc(sizeof(*_tmp95));*_tmp95=vd;_tmp95;});struct Cyc_Absyn_Exp*_tmp498=new_access_exp;struct Cyc_List_List**_tmp497=& res.patvars;Cyc_Tcpat_set_vd(_tmp499,_tmp498,_tmp497,t2);});});
# 252
goto _LL0;}}case 4U: _LL9: _tmp56=((struct Cyc_Absyn_TagInt_p_Absyn_Raw_pat_struct*)_tmp47)->f1;_tmp57=((struct Cyc_Absyn_TagInt_p_Absyn_Raw_pat_struct*)_tmp47)->f2;_LLA: {struct Cyc_Absyn_Tvar*tv=_tmp56;struct Cyc_Absyn_Vardecl*vd=_tmp57;
# 257
({({struct Cyc_Absyn_Vardecl**_tmp49C=({struct Cyc_Absyn_Vardecl**_tmp97=_cycalloc(sizeof(*_tmp97));*_tmp97=vd;_tmp97;});struct Cyc_Absyn_Exp*_tmp49B=access_exp;struct Cyc_List_List**_tmp49A=& res.patvars;Cyc_Tcpat_set_vd(_tmp49C,_tmp49B,_tmp49A,vd->type);});});
# 261
({struct Cyc_Tcenv_Tenv*(*_tmp98)(unsigned,struct Cyc_Tcenv_Tenv*,struct Cyc_List_List*)=Cyc_Tcenv_add_type_vars;unsigned _tmp99=p->loc;struct Cyc_Tcenv_Tenv*_tmp9A=te;struct Cyc_List_List*_tmp9B=({struct Cyc_Absyn_Tvar*_tmp9C[1U];_tmp9C[0]=tv;Cyc_List_list(_tag_fat(_tmp9C,sizeof(struct Cyc_Absyn_Tvar*),1U));});_tmp98(_tmp99,_tmp9A,_tmp9B);});
if(res.tvars_and_bounds_opt == 0)
({struct _tuple1*_tmp49D=({struct _tuple1*_tmp9D=_cycalloc(sizeof(*_tmp9D));_tmp9D->f1=0,_tmp9D->f2=0;_tmp9D;});res.tvars_and_bounds_opt=_tmp49D;});
# 262
({struct Cyc_List_List*_tmp49F=({struct Cyc_List_List*_tmp9F=_cycalloc(sizeof(*_tmp9F));
# 265
({struct _tuple17*_tmp49E=({struct _tuple17*_tmp9E=_cycalloc(sizeof(*_tmp9E));_tmp9E->f1=tv,_tmp9E->f2=0;_tmp9E;});_tmp9F->hd=_tmp49E;}),_tmp9F->tl=(*res.tvars_and_bounds_opt).f1;_tmp9F;});
# 262
(*res.tvars_and_bounds_opt).f1=_tmp49F;});
# 266
t=Cyc_Absyn_uint_type;
goto _LL0;}case 10U: switch(((struct Cyc_Absyn_Int_p_Absyn_Raw_pat_struct*)_tmp47)->f1){case Cyc_Absyn_Unsigned: _LLB: _LLC:
# 269
 t=({Cyc_Tcpat_num_type(topt,Cyc_Absyn_uint_type);});goto _LL0;case Cyc_Absyn_None: _LLD: _LLE:
 goto _LL10;default: _LLF: _LL10:
 t=({Cyc_Tcpat_num_type(topt,Cyc_Absyn_sint_type);});goto _LL0;}case 11U: _LL11: _LL12:
 t=({Cyc_Tcpat_num_type(topt,Cyc_Absyn_char_type);});goto _LL0;case 12U: _LL13: _tmp55=((struct Cyc_Absyn_Float_p_Absyn_Raw_pat_struct*)_tmp47)->f2;_LL14: {int i=_tmp55;
t=({void*(*_tmpA0)(void**topt,void*numt)=Cyc_Tcpat_num_type;void**_tmpA1=topt;void*_tmpA2=({Cyc_Absyn_gen_float_type((unsigned)i);});_tmpA0(_tmpA1,_tmpA2);});goto _LL0;}case 13U: _LL15: _tmp54=((struct Cyc_Absyn_Enum_p_Absyn_Raw_pat_struct*)_tmp47)->f1;_LL16: {struct Cyc_Absyn_Enumdecl*ed=_tmp54;
# 275
t=({void*(*_tmpA3)(void**topt,void*numt)=Cyc_Tcpat_num_type;void**_tmpA4=topt;void*_tmpA5=({Cyc_Absyn_enum_type(ed->name,ed);});_tmpA3(_tmpA4,_tmpA5);});
goto _LL0;}case 14U: _LL17: _tmp53=(void*)((struct Cyc_Absyn_AnonEnum_p_Absyn_Raw_pat_struct*)_tmp47)->f1;_LL18: {void*tenum=_tmp53;
t=({Cyc_Tcpat_num_type(topt,tenum);});goto _LL0;}case 9U: _LL19: _LL1A:
# 279
 if(topt != 0){
void*_stmttmp7=({Cyc_Tcutil_compress(*topt);});void*_tmpA6=_stmttmp7;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmpA6)->tag == 3U){_LL3B: _LL3C:
 t=*topt;goto tcpat_end;}else{_LL3D: _LL3E:
 goto _LL3A;}_LL3A:;}{
# 279
struct Cyc_List_List*tvs=({Cyc_Tcenv_lookup_type_vars(te);});
# 285
t=(void*)({struct Cyc_Absyn_PointerType_Absyn_Type_struct*_tmpA9=_cycalloc(sizeof(*_tmpA9));_tmpA9->tag=3U,({void*_tmp4A4=({Cyc_Absyn_new_evar(& Cyc_Tcutil_ako,({struct Cyc_Core_Opt*_tmpA7=_cycalloc(sizeof(*_tmpA7));_tmpA7->v=tvs;_tmpA7;}));});(_tmpA9->f1).elt_type=_tmp4A4;}),({
struct Cyc_Absyn_Tqual _tmp4A3=({Cyc_Absyn_empty_tqual(0U);});(_tmpA9->f1).elt_tq=_tmp4A3;}),
({void*_tmp4A2=({Cyc_Absyn_new_evar(& Cyc_Tcutil_trko,({struct Cyc_Core_Opt*_tmpA8=_cycalloc(sizeof(*_tmpA8));_tmpA8->v=tvs;_tmpA8;}));});((_tmpA9->f1).ptr_atts).rgn=_tmp4A2;}),((_tmpA9->f1).ptr_atts).nullable=Cyc_Absyn_true_type,({
# 289
void*_tmp4A1=({Cyc_Tcutil_any_bounds(tvs);});((_tmpA9->f1).ptr_atts).bounds=_tmp4A1;}),({void*_tmp4A0=({Cyc_Tcutil_any_bool(tvs);});((_tmpA9->f1).ptr_atts).zero_term=_tmp4A0;}),((_tmpA9->f1).ptr_atts).ptrloc=0;_tmpA9;});
goto _LL0;}case 6U: _LL1B: _tmp52=((struct Cyc_Absyn_Pointer_p_Absyn_Raw_pat_struct*)_tmp47)->f1;_LL1C: {struct Cyc_Absyn_Pat*p2=_tmp52;
# 295
void*inner_typ=Cyc_Absyn_void_type;
void**inner_topt=0;
int elt_const=0;
if(topt != 0){
void*_stmttmp8=({Cyc_Tcutil_compress(*topt);});void*_tmpAA=_stmttmp8;struct Cyc_Absyn_Tqual _tmpAC;void*_tmpAB;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmpAA)->tag == 3U){_LL40: _tmpAB=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmpAA)->f1).elt_type;_tmpAC=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmpAA)->f1).elt_tq;_LL41: {void*elt_typ=_tmpAB;struct Cyc_Absyn_Tqual elt_tq=_tmpAC;
# 301
inner_typ=elt_typ;
inner_topt=& inner_typ;
elt_const=elt_tq.real_const;
goto _LL3F;}}else{_LL42: _LL43:
 goto _LL3F;}_LL3F:;}{
# 298
void*ptr_rgn=({void*(*_tmpBD)(struct Cyc_Core_Opt*k,struct Cyc_Core_Opt*tenv)=Cyc_Absyn_new_evar;struct Cyc_Core_Opt*_tmpBE=& Cyc_Tcutil_trko;struct Cyc_Core_Opt*_tmpBF=({Cyc_Tcenv_lookup_opt_type_vars(te);});_tmpBD(_tmpBE,_tmpBF);});
# 311
struct Cyc_Absyn_Exp*new_access_exp=0;
if((unsigned)access_exp)new_access_exp=({Cyc_Absyn_deref_exp(access_exp,0U);});res=({struct Cyc_Tcpat_TcPatResult(*_tmpAD)(struct Cyc_Tcpat_TcPatResult res1,struct Cyc_Tcpat_TcPatResult res2)=Cyc_Tcpat_combine_results;struct Cyc_Tcpat_TcPatResult _tmpAE=res;struct Cyc_Tcpat_TcPatResult _tmpAF=({Cyc_Tcpat_tcPatRec(te,p2,inner_topt,ptr_rgn,1,new_access_exp);});_tmpAD(_tmpAE,_tmpAF);});
# 319
{void*_stmttmp9=({Cyc_Tcutil_compress((void*)_check_null(p2->topt));});void*_tmpB0=_stmttmp9;struct Cyc_List_List*_tmpB3;struct Cyc_Absyn_Datatypefield*_tmpB2;struct Cyc_Absyn_Datatypedecl*_tmpB1;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpB0)->tag == 0U){if(((struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpB0)->f1)->tag == 21U){if(((((struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpB0)->f1)->f1).KnownDatatypefield).tag == 2){_LL45: _tmpB1=(((((struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpB0)->f1)->f1).KnownDatatypefield).val).f1;_tmpB2=(((((struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpB0)->f1)->f1).KnownDatatypefield).val).f2;_tmpB3=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpB0)->f2;_LL46: {struct Cyc_Absyn_Datatypedecl*tud=_tmpB1;struct Cyc_Absyn_Datatypefield*tuf=_tmpB2;struct Cyc_List_List*targs=_tmpB3;
# 323
{void*_stmttmpA=({Cyc_Tcutil_compress(inner_typ);});void*_tmpB4=_stmttmpA;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpB4)->tag == 0U){if(((struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpB4)->f1)->tag == 21U){_LL4A: _LL4B:
# 325
 goto DONT_PROMOTE;}else{goto _LL4C;}}else{_LL4C: _LL4D:
 goto _LL49;}_LL49:;}{
# 329
void*new_type=({void*(*_tmpB8)(union Cyc_Absyn_DatatypeInfo,struct Cyc_List_List*args)=Cyc_Absyn_datatype_type;union Cyc_Absyn_DatatypeInfo _tmpB9=({Cyc_Absyn_KnownDatatype(({struct Cyc_Absyn_Datatypedecl**_tmpBA=_cycalloc(sizeof(*_tmpBA));*_tmpBA=tud;_tmpBA;}));});struct Cyc_List_List*_tmpBB=targs;_tmpB8(_tmpB9,_tmpBB);});
# 331
p2->topt=new_type;
t=(void*)({struct Cyc_Absyn_PointerType_Absyn_Type_struct*_tmpB7=_cycalloc(sizeof(*_tmpB7));_tmpB7->tag=3U,(_tmpB7->f1).elt_type=new_type,
elt_const?({struct Cyc_Absyn_Tqual _tmp4A8=({Cyc_Absyn_const_tqual(0U);});(_tmpB7->f1).elt_tq=_tmp4A8;}):({
struct Cyc_Absyn_Tqual _tmp4A7=({Cyc_Absyn_empty_tqual(0U);});(_tmpB7->f1).elt_tq=_tmp4A7;}),
((_tmpB7->f1).ptr_atts).rgn=ptr_rgn,({void*_tmp4A6=({void*(*_tmpB5)(struct Cyc_List_List*)=Cyc_Tcutil_any_bool;struct Cyc_List_List*_tmpB6=({Cyc_Tcenv_lookup_type_vars(te);});_tmpB5(_tmpB6);});((_tmpB7->f1).ptr_atts).nullable=_tmp4A6;}),({
void*_tmp4A5=({Cyc_Absyn_bounds_one();});((_tmpB7->f1).ptr_atts).bounds=_tmp4A5;}),((_tmpB7->f1).ptr_atts).zero_term=Cyc_Absyn_false_type,((_tmpB7->f1).ptr_atts).ptrloc=0;_tmpB7;});
# 338
goto _LL44;}}}else{goto _LL47;}}else{goto _LL47;}}else{_LL47: _LL48:
# 340
 DONT_PROMOTE: {
struct Cyc_List_List*tvs=({Cyc_Tcenv_lookup_type_vars(te);});
t=(void*)({struct Cyc_Absyn_PointerType_Absyn_Type_struct*_tmpBC=_cycalloc(sizeof(*_tmpBC));_tmpBC->tag=3U,(_tmpBC->f1).elt_type=(void*)_check_null(p2->topt),
elt_const?({struct Cyc_Absyn_Tqual _tmp4AD=({Cyc_Absyn_const_tqual(0U);});(_tmpBC->f1).elt_tq=_tmp4AD;}):({
struct Cyc_Absyn_Tqual _tmp4AC=({Cyc_Absyn_empty_tqual(0U);});(_tmpBC->f1).elt_tq=_tmp4AC;}),
((_tmpBC->f1).ptr_atts).rgn=ptr_rgn,({void*_tmp4AB=({Cyc_Tcutil_any_bool(tvs);});((_tmpBC->f1).ptr_atts).nullable=_tmp4AB;}),({
void*_tmp4AA=({Cyc_Tcutil_any_bounds(tvs);});((_tmpBC->f1).ptr_atts).bounds=_tmp4AA;}),({void*_tmp4A9=({Cyc_Tcutil_any_bool(tvs);});((_tmpBC->f1).ptr_atts).zero_term=_tmp4A9;}),((_tmpBC->f1).ptr_atts).ptrloc=0;_tmpBC;});}}_LL44:;}
# 348
if((unsigned)new_access_exp)new_access_exp->topt=p2->topt;goto _LL0;}}case 5U: _LL1D: _tmp50=(struct Cyc_List_List**)&((struct Cyc_Absyn_Tuple_p_Absyn_Raw_pat_struct*)_tmp47)->f1;_tmp51=((struct Cyc_Absyn_Tuple_p_Absyn_Raw_pat_struct*)_tmp47)->f2;_LL1E: {struct Cyc_List_List**ps_p=_tmp50;int dots=_tmp51;
# 352
struct Cyc_List_List*ps=*ps_p;
struct Cyc_List_List*pat_ts=0;
struct Cyc_List_List*topt_ts=0;
if(topt != 0){
void*_stmttmpB=({Cyc_Tcutil_compress(*topt);});void*_tmpC0=_stmttmpB;struct Cyc_List_List*_tmpC1;if(((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmpC0)->tag == 6U){_LL4F: _tmpC1=((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmpC0)->f1;_LL50: {struct Cyc_List_List*tqts=_tmpC1;
# 358
topt_ts=tqts;
if(dots){
# 361
int lps=({Cyc_List_length(ps);});
int lts=({Cyc_List_length(tqts);});
if(lps < lts){
struct Cyc_List_List*wild_ps=0;
{int i=0;for(0;i < lts - lps;++ i){
wild_ps=({struct Cyc_List_List*_tmpC2=_cycalloc(sizeof(*_tmpC2));({struct Cyc_Absyn_Pat*_tmp4AE=({Cyc_Tcpat_wild_pat(p->loc);});_tmpC2->hd=_tmp4AE;}),_tmpC2->tl=wild_ps;_tmpC2;});}}
({struct Cyc_List_List*_tmp4AF=({Cyc_List_imp_append(ps,wild_ps);});*ps_p=_tmp4AF;});
ps=*ps_p;}else{
if(({int _tmp4B0=({Cyc_List_length(ps);});_tmp4B0 == ({Cyc_List_length(tqts);});}))
({void*_tmpC3=0U;({unsigned _tmp4B2=p->loc;struct _fat_ptr _tmp4B1=({const char*_tmpC4="unnecessary ... in tuple pattern";_tag_fat(_tmpC4,sizeof(char),33U);});Cyc_Tcutil_warn(_tmp4B2,_tmp4B1,_tag_fat(_tmpC3,sizeof(void*),0U));});});}}
# 359
goto _LL4E;}}else{_LL51: _LL52:
# 375
 goto _LL4E;}_LL4E:;}else{
# 377
if(dots)
({void*_tmpC5=0U;({unsigned _tmp4B4=p->loc;struct _fat_ptr _tmp4B3=({const char*_tmpC6="cannot determine missing fields for ... in tuple pattern";_tag_fat(_tmpC6,sizeof(char),57U);});Cyc_Tcutil_terr(_tmp4B4,_tmp4B3,_tag_fat(_tmpC5,sizeof(void*),0U));});});}
# 355
{
# 379
int i=0;
# 355
for(0;ps != 0;(
# 379
ps=ps->tl,i ++)){
void**inner_topt=0;
if(topt_ts != 0){
inner_topt=&(*((struct _tuple18*)topt_ts->hd)).f2;
topt_ts=topt_ts->tl;}{
# 381
struct Cyc_Absyn_Exp*new_access_exp=0;
# 386
if((unsigned)access_exp)
new_access_exp=({struct Cyc_Absyn_Exp*(*_tmpC7)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_subscript_exp;struct Cyc_Absyn_Exp*_tmpC8=access_exp;struct Cyc_Absyn_Exp*_tmpC9=({Cyc_Absyn_const_exp(({union Cyc_Absyn_Cnst _tmp410;(_tmp410.Int_c).tag=5U,((_tmp410.Int_c).val).f1=Cyc_Absyn_Unsigned,((_tmp410.Int_c).val).f2=i;_tmp410;}),0U);});unsigned _tmpCA=0U;_tmpC7(_tmpC8,_tmpC9,_tmpCA);});
# 386
res=({struct Cyc_Tcpat_TcPatResult(*_tmpCB)(struct Cyc_Tcpat_TcPatResult res1,struct Cyc_Tcpat_TcPatResult res2)=Cyc_Tcpat_combine_results;struct Cyc_Tcpat_TcPatResult _tmpCC=res;struct Cyc_Tcpat_TcPatResult _tmpCD=({Cyc_Tcpat_tcPatRec(te,(struct Cyc_Absyn_Pat*)ps->hd,inner_topt,rgn_pat,allow_ref_pat,new_access_exp);});_tmpCB(_tmpCC,_tmpCD);});
# 393
if((unsigned)new_access_exp)new_access_exp->topt=((struct Cyc_Absyn_Pat*)ps->hd)->topt;pat_ts=({struct Cyc_List_List*_tmpCF=_cycalloc(sizeof(*_tmpCF));
({struct _tuple18*_tmp4B6=({struct _tuple18*_tmpCE=_cycalloc(sizeof(*_tmpCE));({struct Cyc_Absyn_Tqual _tmp4B5=({Cyc_Absyn_empty_tqual(0U);});_tmpCE->f1=_tmp4B5;}),_tmpCE->f2=(void*)_check_null(((struct Cyc_Absyn_Pat*)ps->hd)->topt);_tmpCE;});_tmpCF->hd=_tmp4B6;}),_tmpCF->tl=pat_ts;_tmpCF;});}}}
# 396
t=(void*)({struct Cyc_Absyn_TupleType_Absyn_Type_struct*_tmpD0=_cycalloc(sizeof(*_tmpD0));_tmpD0->tag=6U,({struct Cyc_List_List*_tmp4B7=({Cyc_List_imp_rev(pat_ts);});_tmpD0->f1=_tmp4B7;});_tmpD0;});
goto _LL0;}case 7U: if(((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp47)->f1 != 0){if((((union Cyc_Absyn_AggrInfo*)((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp47)->f1)->KnownAggr).tag == 2){_LL1F: _tmp4C=*((((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp47)->f1)->KnownAggr).val;_tmp4D=((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp47)->f2;_tmp4E=(struct Cyc_List_List**)&((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp47)->f3;_tmp4F=((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp47)->f4;_LL20: {struct Cyc_Absyn_Aggrdecl*ad=_tmp4C;struct Cyc_List_List*exist_ts=_tmp4D;struct Cyc_List_List**dps_p=_tmp4E;int dots=_tmp4F;
# 400
struct Cyc_List_List*dps=*dps_p;
struct _fat_ptr aggr_str=(int)ad->kind == (int)Cyc_Absyn_StructA?({const char*_tmp110="struct";_tag_fat(_tmp110,sizeof(char),7U);}):({const char*_tmp111="union";_tag_fat(_tmp111,sizeof(char),6U);});
if(ad->impl == 0){
({struct Cyc_String_pa_PrintArg_struct _tmpD3=({struct Cyc_String_pa_PrintArg_struct _tmp411;_tmp411.tag=0U,_tmp411.f1=(struct _fat_ptr)((struct _fat_ptr)aggr_str);_tmp411;});void*_tmpD1[1U];_tmpD1[0]=& _tmpD3;({unsigned _tmp4B9=p->loc;struct _fat_ptr _tmp4B8=({const char*_tmpD2="can't destructure an abstract %s";_tag_fat(_tmpD2,sizeof(char),33U);});Cyc_Tcutil_terr(_tmp4B9,_tmp4B8,_tag_fat(_tmpD1,sizeof(void*),1U));});});
t=({void*(*_tmpD4)(struct Cyc_Core_Opt*)=Cyc_Absyn_wildtyp;struct Cyc_Core_Opt*_tmpD5=({Cyc_Tcenv_lookup_opt_type_vars(te);});_tmpD4(_tmpD5);});
goto _LL0;}
# 402
if(
# 409
(int)ad->kind == (int)Cyc_Absyn_UnionA &&((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->tagged)
allow_ref_pat=0;
# 402
if(exist_ts != 0){
# 415
if(topt == 0 ||({Cyc_Tcutil_type_kind(*topt);})!= & Cyc_Tcutil_ak)
allow_ref_pat=0;}
# 402
{
# 418
struct _RegionHandle _tmpD6=_new_region("rgn");struct _RegionHandle*rgn=& _tmpD6;_push_region(rgn);
# 420
{struct Cyc_List_List*var_tvs=0;
struct Cyc_List_List*outlives_constraints=0;
struct Cyc_List_List*u=((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->exist_vars;
{struct Cyc_List_List*t=exist_ts;for(0;t != 0;t=t->tl){
struct Cyc_Absyn_Tvar*tv=(struct Cyc_Absyn_Tvar*)t->hd;
struct Cyc_Absyn_Tvar*uv=(struct Cyc_Absyn_Tvar*)((struct Cyc_List_List*)_check_null(u))->hd;
u=u->tl;{
void*kb1=({Cyc_Absyn_compress_kb(tv->kind);});
void*kb2=({Cyc_Absyn_compress_kb(uv->kind);});
int error=0;
struct Cyc_Absyn_Kind*k2;
{void*_tmpD7=kb2;struct Cyc_Absyn_Kind*_tmpD8;struct Cyc_Absyn_Kind*_tmpD9;switch(*((int*)_tmpD7)){case 2U: _LL54: _tmpD9=((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmpD7)->f2;_LL55: {struct Cyc_Absyn_Kind*k=_tmpD9;
_tmpD8=k;goto _LL57;}case 0U: _LL56: _tmpD8=((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmpD7)->f1;_LL57: {struct Cyc_Absyn_Kind*k=_tmpD8;
k2=k;goto _LL53;}default: _LL58: _LL59:
({void*_tmpDA=0U;({int(*_tmp4BB)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmpDC)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmpDC;});struct _fat_ptr _tmp4BA=({const char*_tmpDB="unconstrained existential type variable in aggregate";_tag_fat(_tmpDB,sizeof(char),53U);});_tmp4BB(_tmp4BA,_tag_fat(_tmpDA,sizeof(void*),0U));});});}_LL53:;}
# 436
{void*_tmpDD=kb1;struct Cyc_Core_Opt**_tmpDE;struct Cyc_Absyn_Kind*_tmpE0;struct Cyc_Core_Opt**_tmpDF;struct Cyc_Absyn_Kind*_tmpE1;switch(*((int*)_tmpDD)){case 0U: _LL5B: _tmpE1=((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmpDD)->f1;_LL5C: {struct Cyc_Absyn_Kind*k1=_tmpE1;
# 439
if(!({Cyc_Tcutil_kind_leq(k2,k1);}))
error=1;
# 439
goto _LL5A;}case 2U: _LL5D: _tmpDF=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmpDD)->f1;_tmpE0=((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmpDD)->f2;_LL5E: {struct Cyc_Core_Opt**f=_tmpDF;struct Cyc_Absyn_Kind*k=_tmpE0;
# 442
_tmpDE=f;goto _LL60;}default: _LL5F: _tmpDE=(struct Cyc_Core_Opt**)&((struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*)_tmpDD)->f1;_LL60: {struct Cyc_Core_Opt**f=_tmpDE;
({struct Cyc_Core_Opt*_tmp4BC=({struct Cyc_Core_Opt*_tmpE2=_cycalloc(sizeof(*_tmpE2));_tmpE2->v=kb2;_tmpE2;});*f=_tmp4BC;});goto _LL5A;}}_LL5A:;}
# 445
if(error)
({struct Cyc_String_pa_PrintArg_struct _tmpE5=({struct Cyc_String_pa_PrintArg_struct _tmp414;_tmp414.tag=0U,_tmp414.f1=(struct _fat_ptr)((struct _fat_ptr)*tv->name);_tmp414;});struct Cyc_String_pa_PrintArg_struct _tmpE6=({struct Cyc_String_pa_PrintArg_struct _tmp413;_tmp413.tag=0U,({
# 449
struct _fat_ptr _tmp4BD=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_kindbound2string(kb1);}));_tmp413.f1=_tmp4BD;});_tmp413;});struct Cyc_String_pa_PrintArg_struct _tmpE7=({struct Cyc_String_pa_PrintArg_struct _tmp412;_tmp412.tag=0U,({struct _fat_ptr _tmp4BE=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_kind2string(k2);}));_tmp412.f1=_tmp4BE;});_tmp412;});void*_tmpE3[3U];_tmpE3[0]=& _tmpE5,_tmpE3[1]=& _tmpE6,_tmpE3[2]=& _tmpE7;({unsigned _tmp4C0=p->loc;struct _fat_ptr _tmp4BF=({const char*_tmpE4="type variable %s has kind %s but must have at least kind %s";_tag_fat(_tmpE4,sizeof(char),60U);});Cyc_Tcutil_terr(_tmp4C0,_tmp4BF,_tag_fat(_tmpE3,sizeof(void*),3U));});});{
# 445
void*vartype=(void*)({struct Cyc_Absyn_VarType_Absyn_Type_struct*_tmpEE=_cycalloc(sizeof(*_tmpEE));
# 451
_tmpEE->tag=2U,_tmpEE->f1=tv;_tmpEE;});
var_tvs=({struct Cyc_List_List*_tmpE8=_region_malloc(rgn,sizeof(*_tmpE8));_tmpE8->hd=vartype,_tmpE8->tl=var_tvs;_tmpE8;});
# 455
if((int)k2->kind == (int)Cyc_Absyn_RgnKind){
if((int)k2->aliasqual == (int)Cyc_Absyn_Aliasable)
outlives_constraints=({struct Cyc_List_List*_tmpEA=_cycalloc(sizeof(*_tmpEA));
({struct _tuple0*_tmp4C1=({struct _tuple0*_tmpE9=_cycalloc(sizeof(*_tmpE9));_tmpE9->f1=Cyc_Absyn_empty_effect,_tmpE9->f2=vartype;_tmpE9;});_tmpEA->hd=_tmp4C1;}),_tmpEA->tl=outlives_constraints;_tmpEA;});else{
# 460
({void*_tmpEB=0U;({int(*_tmp4C3)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmpED)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmpED;});struct _fat_ptr _tmp4C2=({const char*_tmpEC="opened existential had unique or top region kind";_tag_fat(_tmpEC,sizeof(char),49U);});_tmp4C3(_tmp4C2,_tag_fat(_tmpEB,sizeof(void*),0U));});});}}}}}}
# 464
var_tvs=({Cyc_List_imp_rev(var_tvs);});{
# 466
struct Cyc_Tcenv_Tenv*te2=({Cyc_Tcenv_add_type_vars(p->loc,te,exist_ts);});
# 468
struct Cyc_List_List*tenv_tvs=({Cyc_Tcenv_lookup_type_vars(te2);});
struct _tuple13 env=({struct _tuple13 _tmp419;_tmp419.f1=tenv_tvs,_tmp419.f2=rgn;_tmp419;});
struct Cyc_List_List*all_inst=({({struct Cyc_List_List*(*_tmp4C5)(struct _RegionHandle*,struct _tuple14*(*f)(struct _tuple13*,struct Cyc_Absyn_Tvar*),struct _tuple13*env,struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp10F)(struct _RegionHandle*,struct _tuple14*(*f)(struct _tuple13*,struct Cyc_Absyn_Tvar*),struct _tuple13*env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct _RegionHandle*,struct _tuple14*(*f)(struct _tuple13*,struct Cyc_Absyn_Tvar*),struct _tuple13*env,struct Cyc_List_List*x))Cyc_List_rmap_c;_tmp10F;});struct _RegionHandle*_tmp4C4=rgn;_tmp4C5(_tmp4C4,Cyc_Tcutil_r_make_inst_var,& env,ad->tvs);});});
struct Cyc_List_List*exist_inst=({Cyc_List_rzip(rgn,rgn,((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->exist_vars,var_tvs);});
struct Cyc_List_List*all_typs=({({struct Cyc_List_List*(*_tmp4C7)(void*(*f)(struct _tuple14*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp10D)(void*(*f)(struct _tuple14*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(void*(*f)(struct _tuple14*),struct Cyc_List_List*x))Cyc_List_map;_tmp10D;});void*(*_tmp4C6)(struct _tuple14*)=({void*(*_tmp10E)(struct _tuple14*)=(void*(*)(struct _tuple14*))Cyc_Core_snd;_tmp10E;});_tmp4C7(_tmp4C6,all_inst);});});
struct Cyc_List_List*exist_typs=({({struct Cyc_List_List*(*_tmp4C9)(void*(*f)(struct _tuple14*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp10B)(void*(*f)(struct _tuple14*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(void*(*f)(struct _tuple14*),struct Cyc_List_List*x))Cyc_List_map;_tmp10B;});void*(*_tmp4C8)(struct _tuple14*)=({void*(*_tmp10C)(struct _tuple14*)=(void*(*)(struct _tuple14*))Cyc_Core_snd;_tmp10C;});_tmp4C9(_tmp4C8,exist_inst);});});
struct Cyc_List_List*inst=({Cyc_List_rappend(rgn,all_inst,exist_inst);});
# 476
if(exist_ts != 0 ||((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->rgn_po != 0){
if(res.tvars_and_bounds_opt == 0)
({struct _tuple1*_tmp4CA=({struct _tuple1*_tmpEF=_cycalloc(sizeof(*_tmpEF));_tmpEF->f1=0,_tmpEF->f2=0;_tmpEF;});res.tvars_and_bounds_opt=_tmp4CA;});
# 477
({struct Cyc_List_List*_tmp4CC=({struct Cyc_List_List*(*_tmpF0)(struct Cyc_List_List*x,struct Cyc_List_List*y)=Cyc_List_append;struct Cyc_List_List*_tmpF1=(*res.tvars_and_bounds_opt).f1;struct Cyc_List_List*_tmpF2=({({struct Cyc_List_List*(*_tmp4CB)(struct _tuple17*(*f)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*x)=({
# 480
struct Cyc_List_List*(*_tmpF3)(struct _tuple17*(*f)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct _tuple17*(*f)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*x))Cyc_List_map;_tmpF3;});_tmp4CB(Cyc_Tcpat_add_false,exist_ts);});});_tmpF0(_tmpF1,_tmpF2);});
# 477
(*res.tvars_and_bounds_opt).f1=_tmp4CC;});
# 481
({struct Cyc_List_List*_tmp4CD=({Cyc_List_append((*res.tvars_and_bounds_opt).f2,outlives_constraints);});(*res.tvars_and_bounds_opt).f2=_tmp4CD;});{
# 483
struct Cyc_List_List*rpo_inst=0;
{struct Cyc_List_List*rpo=((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->rgn_po;for(0;rpo != 0;rpo=rpo->tl){
rpo_inst=({struct Cyc_List_List*_tmpF5=_cycalloc(sizeof(*_tmpF5));({struct _tuple0*_tmp4D0=({struct _tuple0*_tmpF4=_cycalloc(sizeof(*_tmpF4));({void*_tmp4CF=({Cyc_Tcutil_rsubstitute(rgn,inst,(*((struct _tuple0*)rpo->hd)).f1);});_tmpF4->f1=_tmp4CF;}),({
void*_tmp4CE=({Cyc_Tcutil_rsubstitute(rgn,inst,(*((struct _tuple0*)rpo->hd)).f2);});_tmpF4->f2=_tmp4CE;});_tmpF4;});
# 485
_tmpF5->hd=_tmp4D0;}),_tmpF5->tl=rpo_inst;_tmpF5;});}}
# 488
rpo_inst=({Cyc_List_imp_rev(rpo_inst);});
({struct Cyc_List_List*_tmp4D1=({Cyc_List_append((*res.tvars_and_bounds_opt).f2,rpo_inst);});(*res.tvars_and_bounds_opt).f2=_tmp4D1;});}}
# 476
t=({void*(*_tmpF6)(union Cyc_Absyn_AggrInfo,struct Cyc_List_List*args)=Cyc_Absyn_aggr_type;union Cyc_Absyn_AggrInfo _tmpF7=({Cyc_Absyn_KnownAggr(({struct Cyc_Absyn_Aggrdecl**_tmpF8=_cycalloc(sizeof(*_tmpF8));*_tmpF8=ad;_tmpF8;}));});struct Cyc_List_List*_tmpF9=all_typs;_tmpF6(_tmpF7,_tmpF9);});
# 495
if(dots &&(int)ad->kind == (int)Cyc_Absyn_UnionA)
({void*_tmpFA=0U;({unsigned _tmp4D3=p->loc;struct _fat_ptr _tmp4D2=({const char*_tmpFB="`...' pattern not allowed in union pattern";_tag_fat(_tmpFB,sizeof(char),43U);});Cyc_Tcutil_warn(_tmp4D3,_tmp4D2,_tag_fat(_tmpFA,sizeof(void*),0U));});});else{
if(dots){
# 499
int ldps=({Cyc_List_length(dps);});
int lfields=({Cyc_List_length(((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->fields);});
if(ldps < lfields){
struct Cyc_List_List*wild_dps=0;
{int i=0;for(0;i < lfields - ldps;++ i){
wild_dps=({struct Cyc_List_List*_tmpFD=_cycalloc(sizeof(*_tmpFD));({struct _tuple19*_tmp4D5=({struct _tuple19*_tmpFC=_cycalloc(sizeof(*_tmpFC));_tmpFC->f1=0,({struct Cyc_Absyn_Pat*_tmp4D4=({Cyc_Tcpat_wild_pat(p->loc);});_tmpFC->f2=_tmp4D4;});_tmpFC;});_tmpFD->hd=_tmp4D5;}),_tmpFD->tl=wild_dps;_tmpFD;});}}
({struct Cyc_List_List*_tmp4D6=({Cyc_List_imp_append(dps,wild_dps);});*dps_p=_tmp4D6;});
dps=*dps_p;}else{
if(ldps == lfields)
({void*_tmpFE=0U;({unsigned _tmp4D8=p->loc;struct _fat_ptr _tmp4D7=({const char*_tmpFF="unnecessary ... in struct pattern";_tag_fat(_tmpFF,sizeof(char),34U);});Cyc_Tcutil_warn(_tmp4D8,_tmp4D7,_tag_fat(_tmpFE,sizeof(void*),0U));});});}}}{
# 495
struct Cyc_List_List*fields=({Cyc_Tcutil_resolve_aggregate_designators(rgn,p->loc,dps,ad->kind,((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->fields);});
# 512
for(0;fields != 0;fields=fields->tl){
struct _tuple20*_stmttmpC=(struct _tuple20*)fields->hd;struct Cyc_Absyn_Pat*_tmp101;struct Cyc_Absyn_Aggrfield*_tmp100;_LL62: _tmp100=_stmttmpC->f1;_tmp101=_stmttmpC->f2;_LL63: {struct Cyc_Absyn_Aggrfield*field=_tmp100;struct Cyc_Absyn_Pat*pat=_tmp101;
void*inst_fieldtyp=({Cyc_Tcutil_rsubstitute(rgn,inst,field->type);});
# 516
struct Cyc_Absyn_Exp*new_access_exp=0;
if((unsigned)access_exp)
new_access_exp=({Cyc_Absyn_aggrmember_exp(access_exp,field->name,0U);});
# 517
res=({struct Cyc_Tcpat_TcPatResult(*_tmp102)(struct Cyc_Tcpat_TcPatResult res1,struct Cyc_Tcpat_TcPatResult res2)=Cyc_Tcpat_combine_results;struct Cyc_Tcpat_TcPatResult _tmp103=res;struct Cyc_Tcpat_TcPatResult _tmp104=({Cyc_Tcpat_tcPatRec(te2,pat,& inst_fieldtyp,rgn_pat,allow_ref_pat,new_access_exp);});_tmp102(_tmp103,_tmp104);});
# 524
if(!({Cyc_Unify_unify((void*)_check_null(pat->topt),inst_fieldtyp);}))
({struct Cyc_String_pa_PrintArg_struct _tmp107=({struct Cyc_String_pa_PrintArg_struct _tmp418;_tmp418.tag=0U,_tmp418.f1=(struct _fat_ptr)((struct _fat_ptr)*field->name);_tmp418;});struct Cyc_String_pa_PrintArg_struct _tmp108=({struct Cyc_String_pa_PrintArg_struct _tmp417;_tmp417.tag=0U,_tmp417.f1=(struct _fat_ptr)((struct _fat_ptr)aggr_str);_tmp417;});struct Cyc_String_pa_PrintArg_struct _tmp109=({struct Cyc_String_pa_PrintArg_struct _tmp416;_tmp416.tag=0U,({
struct _fat_ptr _tmp4D9=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(inst_fieldtyp);}));_tmp416.f1=_tmp4D9;});_tmp416;});struct Cyc_String_pa_PrintArg_struct _tmp10A=({struct Cyc_String_pa_PrintArg_struct _tmp415;_tmp415.tag=0U,({
struct _fat_ptr _tmp4DA=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string((void*)_check_null(pat->topt));}));_tmp415.f1=_tmp4DA;});_tmp415;});void*_tmp105[4U];_tmp105[0]=& _tmp107,_tmp105[1]=& _tmp108,_tmp105[2]=& _tmp109,_tmp105[3]=& _tmp10A;({unsigned _tmp4DC=p->loc;struct _fat_ptr _tmp4DB=({const char*_tmp106="field %s of %s pattern expects type %s != %s";_tag_fat(_tmp106,sizeof(char),45U);});Cyc_Tcutil_terr(_tmp4DC,_tmp4DB,_tag_fat(_tmp105,sizeof(void*),4U));});});
# 524
if((unsigned)new_access_exp)
# 528
new_access_exp->topt=pat->topt;}}}}}
# 420
;_pop_region();}
# 531
goto _LL0;}}else{_LL25: _LL26:
# 584
 goto _LL28;}}else{_LL23: _LL24:
# 583
 goto _LL26;}case 8U: _LL21: _tmp48=((struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct*)_tmp47)->f1;_tmp49=((struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct*)_tmp47)->f2;_tmp4A=(struct Cyc_List_List**)&((struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct*)_tmp47)->f3;_tmp4B=((struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct*)_tmp47)->f4;_LL22: {struct Cyc_Absyn_Datatypedecl*tud=_tmp48;struct Cyc_Absyn_Datatypefield*tuf=_tmp49;struct Cyc_List_List**ps_p=_tmp4A;int dots=_tmp4B;
# 534
struct Cyc_List_List*ps=*ps_p;
struct Cyc_List_List*tqts=tuf->typs;
# 537
struct Cyc_List_List*tenv_tvs=({Cyc_Tcenv_lookup_type_vars(te);});
struct _tuple13 env=({struct _tuple13 _tmp420;_tmp420.f1=tenv_tvs,_tmp420.f2=Cyc_Core_heap_region;_tmp420;});
struct Cyc_List_List*inst=({({struct Cyc_List_List*(*_tmp4DD)(struct _tuple14*(*f)(struct _tuple13*,struct Cyc_Absyn_Tvar*),struct _tuple13*env,struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp129)(struct _tuple14*(*f)(struct _tuple13*,struct Cyc_Absyn_Tvar*),struct _tuple13*env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct _tuple14*(*f)(struct _tuple13*,struct Cyc_Absyn_Tvar*),struct _tuple13*env,struct Cyc_List_List*x))Cyc_List_map_c;_tmp129;});_tmp4DD(Cyc_Tcutil_r_make_inst_var,& env,tud->tvs);});});
struct Cyc_List_List*all_typs=({({struct Cyc_List_List*(*_tmp4DF)(void*(*f)(struct _tuple14*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp127)(void*(*f)(struct _tuple14*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(void*(*f)(struct _tuple14*),struct Cyc_List_List*x))Cyc_List_map;_tmp127;});void*(*_tmp4DE)(struct _tuple14*)=({void*(*_tmp128)(struct _tuple14*)=(void*(*)(struct _tuple14*))Cyc_Core_snd;_tmp128;});_tmp4DF(_tmp4DE,inst);});});
t=({void*(*_tmp112)(union Cyc_Absyn_DatatypeFieldInfo,struct Cyc_List_List*args)=Cyc_Absyn_datatype_field_type;union Cyc_Absyn_DatatypeFieldInfo _tmp113=({Cyc_Absyn_KnownDatatypefield(tud,tuf);});struct Cyc_List_List*_tmp114=all_typs;_tmp112(_tmp113,_tmp114);});
if(dots){
# 544
int lps=({Cyc_List_length(ps);});
int ltqts=({Cyc_List_length(tqts);});
if(lps < ltqts){
struct Cyc_List_List*wild_ps=0;
{int i=0;for(0;i < ltqts - lps;++ i){
wild_ps=({struct Cyc_List_List*_tmp115=_cycalloc(sizeof(*_tmp115));({struct Cyc_Absyn_Pat*_tmp4E0=({Cyc_Tcpat_wild_pat(p->loc);});_tmp115->hd=_tmp4E0;}),_tmp115->tl=wild_ps;_tmp115;});}}
({struct Cyc_List_List*_tmp4E1=({Cyc_List_imp_append(ps,wild_ps);});*ps_p=_tmp4E1;});
ps=*ps_p;}else{
if(lps == ltqts)
({struct Cyc_String_pa_PrintArg_struct _tmp118=({struct Cyc_String_pa_PrintArg_struct _tmp41A;_tmp41A.tag=0U,({
struct _fat_ptr _tmp4E2=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(tud->name);}));_tmp41A.f1=_tmp4E2;});_tmp41A;});void*_tmp116[1U];_tmp116[0]=& _tmp118;({unsigned _tmp4E4=p->loc;struct _fat_ptr _tmp4E3=({const char*_tmp117="unnecessary ... in datatype field %s";_tag_fat(_tmp117,sizeof(char),37U);});Cyc_Tcutil_warn(_tmp4E4,_tmp4E3,_tag_fat(_tmp116,sizeof(void*),1U));});});}}
# 542
for(0;
# 556
ps != 0 && tqts != 0;(ps=ps->tl,tqts=tqts->tl)){
struct Cyc_Absyn_Pat*p2=(struct Cyc_Absyn_Pat*)ps->hd;
# 560
void*field_typ=({Cyc_Tcutil_substitute(inst,(*((struct _tuple18*)tqts->hd)).f2);});
# 563
if((unsigned)access_exp)
({Cyc_Tcpat_set_vd(0,access_exp,& res.patvars,Cyc_Absyn_char_type);});
# 563
res=({struct Cyc_Tcpat_TcPatResult(*_tmp119)(struct Cyc_Tcpat_TcPatResult res1,struct Cyc_Tcpat_TcPatResult res2)=Cyc_Tcpat_combine_results;struct Cyc_Tcpat_TcPatResult _tmp11A=res;struct Cyc_Tcpat_TcPatResult _tmp11B=({Cyc_Tcpat_tcPatRec(te,p2,& field_typ,rgn_pat,allow_ref_pat,0);});_tmp119(_tmp11A,_tmp11B);});
# 570
if(!({Cyc_Unify_unify((void*)_check_null(p2->topt),field_typ);}))
({struct Cyc_String_pa_PrintArg_struct _tmp11E=({struct Cyc_String_pa_PrintArg_struct _tmp41D;_tmp41D.tag=0U,({
struct _fat_ptr _tmp4E5=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(tuf->name);}));_tmp41D.f1=_tmp4E5;});_tmp41D;});struct Cyc_String_pa_PrintArg_struct _tmp11F=({struct Cyc_String_pa_PrintArg_struct _tmp41C;_tmp41C.tag=0U,({struct _fat_ptr _tmp4E6=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(field_typ);}));_tmp41C.f1=_tmp4E6;});_tmp41C;});struct Cyc_String_pa_PrintArg_struct _tmp120=({struct Cyc_String_pa_PrintArg_struct _tmp41B;_tmp41B.tag=0U,({
struct _fat_ptr _tmp4E7=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string((void*)_check_null(p2->topt));}));_tmp41B.f1=_tmp4E7;});_tmp41B;});void*_tmp11C[3U];_tmp11C[0]=& _tmp11E,_tmp11C[1]=& _tmp11F,_tmp11C[2]=& _tmp120;({unsigned _tmp4E9=p2->loc;struct _fat_ptr _tmp4E8=({const char*_tmp11D="%s expects argument type %s, not %s";_tag_fat(_tmp11D,sizeof(char),36U);});Cyc_Tcutil_terr(_tmp4E9,_tmp4E8,_tag_fat(_tmp11C,sizeof(void*),3U));});});}
# 575
if(ps != 0)
({struct Cyc_String_pa_PrintArg_struct _tmp123=({struct Cyc_String_pa_PrintArg_struct _tmp41E;_tmp41E.tag=0U,({
struct _fat_ptr _tmp4EA=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(tuf->name);}));_tmp41E.f1=_tmp4EA;});_tmp41E;});void*_tmp121[1U];_tmp121[0]=& _tmp123;({unsigned _tmp4EC=p->loc;struct _fat_ptr _tmp4EB=({const char*_tmp122="too many arguments for datatype constructor %s";_tag_fat(_tmp122,sizeof(char),47U);});Cyc_Tcutil_terr(_tmp4EC,_tmp4EB,_tag_fat(_tmp121,sizeof(void*),1U));});});
# 575
if(tqts != 0)
# 579
({struct Cyc_String_pa_PrintArg_struct _tmp126=({struct Cyc_String_pa_PrintArg_struct _tmp41F;_tmp41F.tag=0U,({
struct _fat_ptr _tmp4ED=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(tuf->name);}));_tmp41F.f1=_tmp4ED;});_tmp41F;});void*_tmp124[1U];_tmp124[0]=& _tmp126;({unsigned _tmp4EF=p->loc;struct _fat_ptr _tmp4EE=({const char*_tmp125="too few arguments for datatype constructor %s";_tag_fat(_tmp125,sizeof(char),46U);});Cyc_Tcutil_terr(_tmp4EF,_tmp4EE,_tag_fat(_tmp124,sizeof(void*),1U));});});
# 575
goto _LL0;}case 15U: _LL27: _LL28:
# 585
 goto _LL2A;case 17U: _LL29: _LL2A:
 goto _LL2C;default: _LL2B: _LL2C:
# 588
 t=({void*(*_tmp12A)(struct Cyc_Core_Opt*)=Cyc_Absyn_wildtyp;struct Cyc_Core_Opt*_tmp12B=({Cyc_Tcenv_lookup_opt_type_vars(te);});_tmp12A(_tmp12B);});goto _LL0;}_LL0:;}
# 590
tcpat_end:
 p->topt=t;
return res;}}
# 169
struct Cyc_Tcpat_TcPatResult Cyc_Tcpat_tcPat(struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Pat*p,void**topt,struct Cyc_Absyn_Exp*pat_var_exp){
# 597
struct Cyc_Tcpat_TcPatResult ans=({Cyc_Tcpat_tcPatRec(te,p,topt,0,0,pat_var_exp);});
# 599
struct _tuple1 _stmttmpD=({Cyc_List_split(ans.patvars);});struct Cyc_List_List*_tmp12D;_LL1: _tmp12D=_stmttmpD.f1;_LL2: {struct Cyc_List_List*vs1=_tmp12D;
struct Cyc_List_List*vs=0;
{struct Cyc_List_List*x=vs1;for(0;x != 0;x=x->tl){
if((struct Cyc_Absyn_Vardecl**)x->hd != 0)vs=({struct Cyc_List_List*_tmp12E=_cycalloc(sizeof(*_tmp12E));_tmp12E->hd=*((struct Cyc_Absyn_Vardecl**)_check_null((struct Cyc_Absyn_Vardecl**)x->hd)),_tmp12E->tl=vs;_tmp12E;});}}
# 601
({void(*_tmp12F)(struct Cyc_List_List*,unsigned,struct _fat_ptr err_msg)=Cyc_Tcutil_check_unique_vars;struct Cyc_List_List*_tmp130=({({struct Cyc_List_List*(*_tmp4F0)(struct _fat_ptr*(*f)(struct Cyc_Absyn_Vardecl*),struct Cyc_List_List*x)=({
# 603
struct Cyc_List_List*(*_tmp131)(struct _fat_ptr*(*f)(struct Cyc_Absyn_Vardecl*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct _fat_ptr*(*f)(struct Cyc_Absyn_Vardecl*),struct Cyc_List_List*x))Cyc_List_map;_tmp131;});_tmp4F0(Cyc_Tcpat_get_name,vs);});});unsigned _tmp132=p->loc;struct _fat_ptr _tmp133=({const char*_tmp134="pattern contains a repeated variable";_tag_fat(_tmp134,sizeof(char),37U);});_tmp12F(_tmp130,_tmp132,_tmp133);});
# 608
{struct Cyc_List_List*x=ans.patvars;for(0;x != 0;x=x->tl){
struct _tuple16*_stmttmpE=(struct _tuple16*)x->hd;struct Cyc_Absyn_Exp**_tmp136;struct Cyc_Absyn_Vardecl**_tmp135;_LL4: _tmp135=_stmttmpE->f1;_tmp136=(struct Cyc_Absyn_Exp**)& _stmttmpE->f2;_LL5: {struct Cyc_Absyn_Vardecl**vdopt=_tmp135;struct Cyc_Absyn_Exp**expopt=_tmp136;
if(*expopt != 0 &&*expopt != pat_var_exp)
({struct Cyc_Absyn_Exp*_tmp4F1=({Cyc_Tcutil_deep_copy_exp(1,(struct Cyc_Absyn_Exp*)_check_null(*expopt));});*expopt=_tmp4F1;});}}}
# 613
return ans;}}
# 619
static int Cyc_Tcpat_try_alias_coerce(struct Cyc_Tcenv_Tenv*tenv,void*old_type,void*new_type,struct Cyc_Absyn_Exp*initializer,struct Cyc_List_List*assump){
# 622
struct _tuple0 _stmttmpF=({struct _tuple0 _tmp422;({void*_tmp4F3=({Cyc_Tcutil_compress(old_type);});_tmp422.f1=_tmp4F3;}),({void*_tmp4F2=({Cyc_Tcutil_compress(new_type);});_tmp422.f2=_tmp4F2;});_tmp422;});struct _tuple0 _tmp138=_stmttmpF;struct Cyc_Absyn_PtrInfo _tmp13A;struct Cyc_Absyn_PtrInfo _tmp139;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp138.f1)->tag == 3U){if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp138.f2)->tag == 3U){_LL1: _tmp139=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp138.f1)->f1;_tmp13A=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp138.f2)->f1;_LL2: {struct Cyc_Absyn_PtrInfo pold=_tmp139;struct Cyc_Absyn_PtrInfo pnew=_tmp13A;
# 624
struct Cyc_Absyn_PointerType_Absyn_Type_struct*ptry=({struct Cyc_Absyn_PointerType_Absyn_Type_struct*_tmp13B=_cycalloc(sizeof(*_tmp13B));_tmp13B->tag=3U,(_tmp13B->f1).elt_type=pold.elt_type,(_tmp13B->f1).elt_tq=pnew.elt_tq,
((_tmp13B->f1).ptr_atts).rgn=(pold.ptr_atts).rgn,((_tmp13B->f1).ptr_atts).nullable=(pnew.ptr_atts).nullable,((_tmp13B->f1).ptr_atts).bounds=(pnew.ptr_atts).bounds,((_tmp13B->f1).ptr_atts).zero_term=(pnew.ptr_atts).zero_term,((_tmp13B->f1).ptr_atts).ptrloc=(pold.ptr_atts).ptrloc;_tmp13B;});
# 630
return({Cyc_Tcutil_subtype(tenv,assump,(void*)ptry,new_type);})&&({Cyc_Tcutil_coerce_assign(tenv,initializer,(void*)ptry);});}}else{goto _LL3;}}else{_LL3: _LL4:
# 632
 return 0;}_LL0:;}
# 639
static void Cyc_Tcpat_check_alias_coercion(struct Cyc_Tcenv_Tenv*tenv,unsigned loc,void*old_type,struct Cyc_Absyn_Tvar*tv,void*new_type,struct Cyc_Absyn_Exp*initializer){
# 642
struct Cyc_List_List*assump=({struct Cyc_List_List*_tmp148=_cycalloc(sizeof(*_tmp148));({struct _tuple0*_tmp4F5=({struct _tuple0*_tmp147=_cycalloc(sizeof(*_tmp147));_tmp147->f1=Cyc_Absyn_unique_rgn_type,({void*_tmp4F4=(void*)({struct Cyc_Absyn_VarType_Absyn_Type_struct*_tmp146=_cycalloc(sizeof(*_tmp146));_tmp146->tag=2U,_tmp146->f1=tv;_tmp146;});_tmp147->f2=_tmp4F4;});_tmp147;});_tmp148->hd=_tmp4F5;}),_tmp148->tl=0;_tmp148;});
if(({Cyc_Tcutil_subtype(tenv,assump,old_type,new_type);})){
# 660 "tcpat.cyc"
struct _tuple0 _stmttmp10=({struct _tuple0 _tmp423;({void*_tmp4F7=({Cyc_Tcutil_compress(old_type);});_tmp423.f1=_tmp4F7;}),({void*_tmp4F6=({Cyc_Tcutil_compress(new_type);});_tmp423.f2=_tmp4F6;});_tmp423;});struct _tuple0 _tmp13D=_stmttmp10;struct Cyc_Absyn_PtrInfo _tmp13F;struct Cyc_Absyn_PtrInfo _tmp13E;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp13D.f1)->tag == 3U){if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp13D.f2)->tag == 3U){_LL1: _tmp13E=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp13D.f1)->f1;_tmp13F=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp13D.f2)->f1;_LL2: {struct Cyc_Absyn_PtrInfo pold=_tmp13E;struct Cyc_Absyn_PtrInfo pnew=_tmp13F;
goto _LL0;}}else{goto _LL3;}}else{_LL3: _LL4:
({void*_tmp140=0U;({unsigned _tmp4F9=loc;struct _fat_ptr _tmp4F8=({const char*_tmp141="alias requires pointer type";_tag_fat(_tmp141,sizeof(char),28U);});Cyc_Tcutil_terr(_tmp4F9,_tmp4F8,_tag_fat(_tmp140,sizeof(void*),0U));});});goto _LL0;}_LL0:;}else{
# 665
({struct Cyc_String_pa_PrintArg_struct _tmp144=({struct Cyc_String_pa_PrintArg_struct _tmp425;_tmp425.tag=0U,({
struct _fat_ptr _tmp4FA=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(old_type);}));_tmp425.f1=_tmp4FA;});_tmp425;});struct Cyc_String_pa_PrintArg_struct _tmp145=({struct Cyc_String_pa_PrintArg_struct _tmp424;_tmp424.tag=0U,({struct _fat_ptr _tmp4FB=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(new_type);}));_tmp424.f1=_tmp4FB;});_tmp424;});void*_tmp142[2U];_tmp142[0]=& _tmp144,_tmp142[1]=& _tmp145;({unsigned _tmp4FD=loc;struct _fat_ptr _tmp4FC=({const char*_tmp143="cannot alias value of type %s to type %s";_tag_fat(_tmp143,sizeof(char),41U);});Cyc_Tcutil_terr(_tmp4FD,_tmp4FC,_tag_fat(_tmp142,sizeof(void*),2U));});});}}
# 672
void Cyc_Tcpat_check_pat_regions_rec(struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Pat*p,int did_noalias_deref,struct Cyc_List_List*patvars){
# 676
void*_stmttmp11=p->r;void*_tmp14A=_stmttmp11;struct Cyc_Absyn_Vardecl*_tmp14C;struct Cyc_Absyn_Tvar*_tmp14B;struct Cyc_Absyn_Pat*_tmp14E;struct Cyc_Absyn_Vardecl*_tmp14D;struct Cyc_Absyn_Pat*_tmp150;struct Cyc_Absyn_Vardecl*_tmp14F;struct Cyc_List_List*_tmp151;struct Cyc_List_List*_tmp152;struct Cyc_List_List*_tmp155;struct Cyc_List_List*_tmp154;union Cyc_Absyn_AggrInfo*_tmp153;struct Cyc_Absyn_Pat*_tmp156;switch(*((int*)_tmp14A)){case 6U: _LL1: _tmp156=((struct Cyc_Absyn_Pointer_p_Absyn_Raw_pat_struct*)_tmp14A)->f1;_LL2: {struct Cyc_Absyn_Pat*p2=_tmp156;
# 678
void*_stmttmp12=(void*)_check_null(p->topt);void*_tmp157=_stmttmp12;void*_tmp158;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp157)->tag == 3U){_LL12: _tmp158=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp157)->f1).ptr_atts).rgn;_LL13: {void*rt=_tmp158;
# 680
({Cyc_Tcenv_check_rgn_accessible(te,p->loc,rt);});
({void(*_tmp159)(struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Pat*p,int did_noalias_deref,struct Cyc_List_List*patvars)=Cyc_Tcpat_check_pat_regions_rec;struct Cyc_Tcenv_Tenv*_tmp15A=te;struct Cyc_Absyn_Pat*_tmp15B=p2;int _tmp15C=({Cyc_Tcutil_is_noalias_region(rt,0);});struct Cyc_List_List*_tmp15D=patvars;_tmp159(_tmp15A,_tmp15B,_tmp15C,_tmp15D);});
return;}}else{_LL14: _LL15:
({void*_tmp15E=0U;({int(*_tmp4FF)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp160)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp160;});struct _fat_ptr _tmp4FE=({const char*_tmp15F="check_pat_regions: bad pointer type";_tag_fat(_tmp15F,sizeof(char),36U);});_tmp4FF(_tmp4FE,_tag_fat(_tmp15E,sizeof(void*),0U));});});}_LL11:;}case 7U: _LL3: _tmp153=((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp14A)->f1;_tmp154=((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp14A)->f2;_tmp155=((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp14A)->f3;_LL4: {union Cyc_Absyn_AggrInfo*ai=_tmp153;struct Cyc_List_List*exist_ts=_tmp154;struct Cyc_List_List*dps=_tmp155;
# 686
for(0;dps != 0;dps=dps->tl){
({Cyc_Tcpat_check_pat_regions_rec(te,(*((struct _tuple19*)dps->hd)).f2,did_noalias_deref,patvars);});}
return;}case 8U: _LL5: _tmp152=((struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct*)_tmp14A)->f3;_LL6: {struct Cyc_List_List*ps=_tmp152;
# 690
did_noalias_deref=0;_tmp151=ps;goto _LL8;}case 5U: _LL7: _tmp151=((struct Cyc_Absyn_Tuple_p_Absyn_Raw_pat_struct*)_tmp14A)->f1;_LL8: {struct Cyc_List_List*ps=_tmp151;
# 692
for(0;ps != 0;ps=ps->tl){
({Cyc_Tcpat_check_pat_regions_rec(te,(struct Cyc_Absyn_Pat*)ps->hd,did_noalias_deref,patvars);});}
return;}case 3U: _LL9: _tmp14F=((struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct*)_tmp14A)->f1;_tmp150=((struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct*)_tmp14A)->f2;_LLA: {struct Cyc_Absyn_Vardecl*vd=_tmp14F;struct Cyc_Absyn_Pat*p2=_tmp150;
# 696
{struct Cyc_List_List*x=patvars;for(0;x != 0;x=x->tl){
struct _tuple16*_stmttmp13=(struct _tuple16*)x->hd;struct Cyc_Absyn_Exp*_tmp162;struct Cyc_Absyn_Vardecl**_tmp161;_LL17: _tmp161=_stmttmp13->f1;_tmp162=_stmttmp13->f2;_LL18: {struct Cyc_Absyn_Vardecl**vdopt=_tmp161;struct Cyc_Absyn_Exp*eopt=_tmp162;
# 702
if((vdopt != 0 &&*vdopt == vd)&& eopt != 0){
{void*_stmttmp14=eopt->r;void*_tmp163=_stmttmp14;struct Cyc_Absyn_Exp*_tmp164;if(((struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct*)_tmp163)->tag == 15U){_LL1A: _tmp164=((struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct*)_tmp163)->f1;_LL1B: {struct Cyc_Absyn_Exp*e=_tmp164;
# 705
if(({Cyc_Tcutil_is_noalias_pointer_or_aggr((void*)_check_null(e->topt));}))
({void*_tmp165=0U;({unsigned _tmp501=p->loc;struct _fat_ptr _tmp500=({const char*_tmp166="reference pattern not allowed on alias-free pointers";_tag_fat(_tmp166,sizeof(char),53U);});Cyc_Tcutil_terr(_tmp501,_tmp500,_tag_fat(_tmp165,sizeof(void*),0U));});});
# 705
goto _LL19;}}else{_LL1C: _LL1D:
# 709
({void*_tmp167=0U;({int(*_tmp503)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp169)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp169;});struct _fat_ptr _tmp502=({const char*_tmp168="check_pat_regions: bad reference access expression";_tag_fat(_tmp168,sizeof(char),51U);});_tmp503(_tmp502,_tag_fat(_tmp167,sizeof(void*),0U));});});}_LL19:;}
# 711
break;}}}}
# 714
({Cyc_Tcpat_check_pat_regions_rec(te,p2,did_noalias_deref,patvars);});
return;}case 1U: _LLB: _tmp14D=((struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct*)_tmp14A)->f1;_tmp14E=((struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct*)_tmp14A)->f2;_LLC: {struct Cyc_Absyn_Vardecl*vd=_tmp14D;struct Cyc_Absyn_Pat*p2=_tmp14E;
# 717
{void*_stmttmp15=p->topt;void*_tmp16A=_stmttmp15;if(_tmp16A != 0){if(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp16A)->tag == 4U){_LL1F: _LL20:
# 719
 if(did_noalias_deref){
({void*_tmp16B=0U;({unsigned _tmp505=p->loc;struct _fat_ptr _tmp504=({const char*_tmp16C="pattern to array would create alias of no-alias pointer";_tag_fat(_tmp16C,sizeof(char),56U);});Cyc_Tcutil_terr(_tmp505,_tmp504,_tag_fat(_tmp16B,sizeof(void*),0U));});});
return;}
# 719
goto _LL1E;}else{goto _LL21;}}else{_LL21: _LL22:
# 724
 goto _LL1E;}_LL1E:;}
# 726
({Cyc_Tcpat_check_pat_regions_rec(te,p2,did_noalias_deref,patvars);});
return;}case 2U: _LLD: _tmp14B=((struct Cyc_Absyn_AliasVar_p_Absyn_Raw_pat_struct*)_tmp14A)->f1;_tmp14C=((struct Cyc_Absyn_AliasVar_p_Absyn_Raw_pat_struct*)_tmp14A)->f2;_LLE: {struct Cyc_Absyn_Tvar*tv=_tmp14B;struct Cyc_Absyn_Vardecl*vd=_tmp14C;
# 729
{struct Cyc_List_List*x=patvars;for(0;x != 0;x=x->tl){
struct _tuple16*_stmttmp16=(struct _tuple16*)x->hd;struct Cyc_Absyn_Exp*_tmp16E;struct Cyc_Absyn_Vardecl**_tmp16D;_LL24: _tmp16D=_stmttmp16->f1;_tmp16E=_stmttmp16->f2;_LL25: {struct Cyc_Absyn_Vardecl**vdopt=_tmp16D;struct Cyc_Absyn_Exp*eopt=_tmp16E;
# 733
if(vdopt != 0 &&*vdopt == vd){
if(eopt == 0)
({void*_tmp16F=0U;({unsigned _tmp507=p->loc;struct _fat_ptr _tmp506=({const char*_tmp170="cannot alias pattern expression in datatype";_tag_fat(_tmp170,sizeof(char),44U);});Cyc_Tcutil_terr(_tmp507,_tmp506,_tag_fat(_tmp16F,sizeof(void*),0U));});});else{
# 737
struct Cyc_Tcenv_Tenv*te2=({struct Cyc_Tcenv_Tenv*(*_tmp172)(unsigned,struct Cyc_Tcenv_Tenv*,struct Cyc_List_List*)=Cyc_Tcenv_add_type_vars;unsigned _tmp173=p->loc;struct Cyc_Tcenv_Tenv*_tmp174=te;struct Cyc_List_List*_tmp175=({struct Cyc_Absyn_Tvar*_tmp176[1U];_tmp176[0]=tv;Cyc_List_list(_tag_fat(_tmp176,sizeof(struct Cyc_Absyn_Tvar*),1U));});_tmp172(_tmp173,_tmp174,_tmp175);});
te2=({({struct Cyc_Tcenv_Tenv*_tmp508=te2;Cyc_Tcenv_add_region(_tmp508,(void*)({struct Cyc_Absyn_VarType_Absyn_Type_struct*_tmp171=_cycalloc(sizeof(*_tmp171));_tmp171->tag=2U,_tmp171->f1=tv;_tmp171;}),0);});});
# 740
({Cyc_Tcpat_check_alias_coercion(te2,p->loc,(void*)_check_null(eopt->topt),tv,vd->type,eopt);});}
# 743
break;}}}}
# 746
goto _LL0;}default: _LLF: _LL10:
 return;}_LL0:;}
# 762 "tcpat.cyc"
void Cyc_Tcpat_check_pat_regions(struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Pat*p,struct Cyc_List_List*patvars){
# 764
({Cyc_Tcpat_check_pat_regions_rec(te,p,0,patvars);});{
struct Cyc_List_List*x=patvars;for(0;x != 0;x=x->tl){
struct _tuple16*_stmttmp17=(struct _tuple16*)x->hd;struct Cyc_Absyn_Exp*_tmp179;struct Cyc_Absyn_Vardecl**_tmp178;_LL1: _tmp178=_stmttmp17->f1;_tmp179=_stmttmp17->f2;_LL2: {struct Cyc_Absyn_Vardecl**vdopt=_tmp178;struct Cyc_Absyn_Exp*eopt=_tmp179;
if(eopt != 0){
struct Cyc_Absyn_Exp*e=eopt;
# 771
if(({Cyc_Tcutil_is_noalias_pointer_or_aggr((void*)_check_null(e->topt));})&& !({Cyc_Tcutil_is_noalias_path(e);}))
# 773
({struct Cyc_String_pa_PrintArg_struct _tmp17C=({struct Cyc_String_pa_PrintArg_struct _tmp426;_tmp426.tag=0U,({
# 775
struct _fat_ptr _tmp50B=(struct _fat_ptr)(vdopt != 0?(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp17F=({struct Cyc_String_pa_PrintArg_struct _tmp427;_tmp427.tag=0U,({
# 777
struct _fat_ptr _tmp509=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string((*vdopt)->name);}));_tmp427.f1=_tmp509;});_tmp427;});void*_tmp17D[1U];_tmp17D[0]=& _tmp17F;({struct _fat_ptr _tmp50A=({const char*_tmp17E="for variable %s";_tag_fat(_tmp17E,sizeof(char),16U);});Cyc_aprintf(_tmp50A,_tag_fat(_tmp17D,sizeof(void*),1U));});}):({const char*_tmp180="";_tag_fat(_tmp180,sizeof(char),1U);}));
# 775
_tmp426.f1=_tmp50B;});_tmp426;});void*_tmp17A[1U];_tmp17A[0]=& _tmp17C;({unsigned _tmp50D=p->loc;struct _fat_ptr _tmp50C=({const char*_tmp17B="pattern %s dereferences a alias-free pointer from a non-unique path";_tag_fat(_tmp17B,sizeof(char),68U);});Cyc_Tcutil_terr(_tmp50D,_tmp50C,_tag_fat(_tmp17A,sizeof(void*),1U));});});}}}}}
# 831 "tcpat.cyc"
struct Cyc_Tcpat_EqNull_Tcpat_PatTest_struct Cyc_Tcpat_EqNull_val={1U};
struct Cyc_Tcpat_NeqNull_Tcpat_PatTest_struct Cyc_Tcpat_NeqNull_val={2U};
# 841
struct Cyc_Tcpat_Dummy_Tcpat_Access_struct Cyc_Tcpat_Dummy_val={0U};
struct Cyc_Tcpat_Deref_Tcpat_Access_struct Cyc_Tcpat_Deref_val={1U};struct _union_Name_value_Name_v{int tag;struct _fat_ptr val;};struct _union_Name_value_Int_v{int tag;int val;};union Cyc_Tcpat_Name_value{struct _union_Name_value_Name_v Name_v;struct _union_Name_value_Int_v Int_v;};
# 855
union Cyc_Tcpat_Name_value Cyc_Tcpat_Name_v(struct _fat_ptr s){return({union Cyc_Tcpat_Name_value _tmp428;(_tmp428.Name_v).tag=1U,(_tmp428.Name_v).val=s;_tmp428;});}union Cyc_Tcpat_Name_value Cyc_Tcpat_Int_v(int i){
return({union Cyc_Tcpat_Name_value _tmp429;(_tmp429.Int_v).tag=2U,(_tmp429.Int_v).val=i;_tmp429;});}struct Cyc_Tcpat_Con_s{union Cyc_Tcpat_Name_value name;int arity;int*span;union Cyc_Tcpat_PatOrWhere orig_pat;};struct Cyc_Tcpat_Any_Tcpat_Simple_pat_struct{int tag;};struct Cyc_Tcpat_Con_Tcpat_Simple_pat_struct{int tag;struct Cyc_Tcpat_Con_s*f1;struct Cyc_List_List*f2;};
# 872
static int Cyc_Tcpat_compare_con(struct Cyc_Tcpat_Con_s*c1,struct Cyc_Tcpat_Con_s*c2){
union Cyc_Tcpat_Name_value _stmttmp18=c1->name;union Cyc_Tcpat_Name_value _tmp184=_stmttmp18;int _tmp185;struct _fat_ptr _tmp186;if((_tmp184.Name_v).tag == 1){_LL1: _tmp186=(_tmp184.Name_v).val;_LL2: {struct _fat_ptr n1=_tmp186;
# 875
union Cyc_Tcpat_Name_value _stmttmp19=c2->name;union Cyc_Tcpat_Name_value _tmp187=_stmttmp19;struct _fat_ptr _tmp188;if((_tmp187.Name_v).tag == 1){_LL6: _tmp188=(_tmp187.Name_v).val;_LL7: {struct _fat_ptr n2=_tmp188;
return({Cyc_strcmp((struct _fat_ptr)n1,(struct _fat_ptr)n2);});}}else{_LL8: _LL9:
 return - 1;}_LL5:;}}else{_LL3: _tmp185=(_tmp184.Int_v).val;_LL4: {int i1=_tmp185;
# 880
union Cyc_Tcpat_Name_value _stmttmp1A=c2->name;union Cyc_Tcpat_Name_value _tmp189=_stmttmp1A;int _tmp18A;if((_tmp189.Name_v).tag == 1){_LLB: _LLC:
 return 1;}else{_LLD: _tmp18A=(_tmp189.Int_v).val;_LLE: {int i2=_tmp18A;
return i1 - i2;}}_LLA:;}}_LL0:;}
# 888
static struct Cyc_Set_Set*Cyc_Tcpat_empty_con_set(){
return({({struct Cyc_Set_Set*(*_tmp50E)(struct _RegionHandle*r,int(*cmp)(struct Cyc_Tcpat_Con_s*,struct Cyc_Tcpat_Con_s*))=({struct Cyc_Set_Set*(*_tmp18C)(struct _RegionHandle*r,int(*cmp)(struct Cyc_Tcpat_Con_s*,struct Cyc_Tcpat_Con_s*))=(struct Cyc_Set_Set*(*)(struct _RegionHandle*r,int(*cmp)(struct Cyc_Tcpat_Con_s*,struct Cyc_Tcpat_Con_s*)))Cyc_Set_rempty;_tmp18C;});_tmp50E(Cyc_Core_heap_region,Cyc_Tcpat_compare_con);});});}
# 888
static int Cyc_Tcpat_one_opt=1;
# 893
static int Cyc_Tcpat_two_opt=2;
static int Cyc_Tcpat_twofiftysix_opt=256;
# 896
static unsigned Cyc_Tcpat_datatype_tag_number(struct Cyc_Absyn_Datatypedecl*td,struct _tuple2*name){
unsigned ans=0U;
struct Cyc_List_List*fs=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(td->fields))->v;
while(({Cyc_Absyn_qvar_cmp(name,((struct Cyc_Absyn_Datatypefield*)((struct Cyc_List_List*)_check_null(fs))->hd)->name);})!= 0){
++ ans;
fs=fs->tl;}
# 903
return ans;}
# 906
static int Cyc_Tcpat_get_member_offset(struct Cyc_Absyn_Aggrdecl*ad,struct _fat_ptr*f){
int i=1;
{struct Cyc_List_List*fields=((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->fields;for(0;fields != 0;fields=fields->tl){
struct Cyc_Absyn_Aggrfield*field=(struct Cyc_Absyn_Aggrfield*)fields->hd;
if(({Cyc_strcmp((struct _fat_ptr)*field->name,(struct _fat_ptr)*f);})== 0)return i;++ i;}}
# 913
({void*_tmp18F=0U;({int(*_tmp511)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp193)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp193;});struct _fat_ptr _tmp510=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp192=({struct Cyc_String_pa_PrintArg_struct _tmp42A;_tmp42A.tag=0U,_tmp42A.f1=(struct _fat_ptr)((struct _fat_ptr)*f);_tmp42A;});void*_tmp190[1U];_tmp190[0]=& _tmp192;({struct _fat_ptr _tmp50F=({const char*_tmp191="get_member_offset %s failed";_tag_fat(_tmp191,sizeof(char),28U);});Cyc_aprintf(_tmp50F,_tag_fat(_tmp190,sizeof(void*),1U));});});_tmp511(_tmp510,_tag_fat(_tmp18F,sizeof(void*),0U));});});}
# 916
static void*Cyc_Tcpat_get_pat_test(union Cyc_Tcpat_PatOrWhere pw){
union Cyc_Tcpat_PatOrWhere _tmp195=pw;struct Cyc_Absyn_Pat*_tmp196;struct Cyc_Absyn_Exp*_tmp197;if((_tmp195.where_clause).tag == 2){_LL1: _tmp197=(_tmp195.where_clause).val;_LL2: {struct Cyc_Absyn_Exp*e=_tmp197;
return(void*)({struct Cyc_Tcpat_WhereTest_Tcpat_PatTest_struct*_tmp198=_cycalloc(sizeof(*_tmp198));_tmp198->tag=0U,_tmp198->f1=e;_tmp198;});}}else{_LL3: _tmp196=(_tmp195.pattern).val;_LL4: {struct Cyc_Absyn_Pat*p=_tmp196;
# 920
void*_stmttmp1B=p->r;void*_tmp199=_stmttmp1B;struct Cyc_List_List*_tmp19B;union Cyc_Absyn_AggrInfo _tmp19A;struct Cyc_Absyn_Datatypefield*_tmp19D;struct Cyc_Absyn_Datatypedecl*_tmp19C;struct Cyc_Absyn_Enumfield*_tmp19F;void*_tmp19E;struct Cyc_Absyn_Enumfield*_tmp1A1;struct Cyc_Absyn_Enumdecl*_tmp1A0;int _tmp1A3;struct _fat_ptr _tmp1A2;char _tmp1A4;int _tmp1A6;enum Cyc_Absyn_Sign _tmp1A5;struct Cyc_Absyn_Pat*_tmp1A7;struct Cyc_Absyn_Pat*_tmp1A8;switch(*((int*)_tmp199)){case 1U: _LL6: _tmp1A8=((struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct*)_tmp199)->f2;_LL7: {struct Cyc_Absyn_Pat*p1=_tmp1A8;
_tmp1A7=p1;goto _LL9;}case 3U: _LL8: _tmp1A7=((struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct*)_tmp199)->f2;_LL9: {struct Cyc_Absyn_Pat*p1=_tmp1A7;
return({Cyc_Tcpat_get_pat_test(({union Cyc_Tcpat_PatOrWhere _tmp42B;(_tmp42B.pattern).tag=1U,(_tmp42B.pattern).val=p1;_tmp42B;}));});}case 9U: _LLA: _LLB:
 return(void*)& Cyc_Tcpat_EqNull_val;case 10U: _LLC: _tmp1A5=((struct Cyc_Absyn_Int_p_Absyn_Raw_pat_struct*)_tmp199)->f1;_tmp1A6=((struct Cyc_Absyn_Int_p_Absyn_Raw_pat_struct*)_tmp199)->f2;_LLD: {enum Cyc_Absyn_Sign s=_tmp1A5;int i=_tmp1A6;
return(void*)({struct Cyc_Tcpat_EqConst_Tcpat_PatTest_struct*_tmp1A9=_cycalloc(sizeof(*_tmp1A9));_tmp1A9->tag=6U,_tmp1A9->f1=(unsigned)i;_tmp1A9;});}case 11U: _LLE: _tmp1A4=((struct Cyc_Absyn_Char_p_Absyn_Raw_pat_struct*)_tmp199)->f1;_LLF: {char c=_tmp1A4;
return(void*)({struct Cyc_Tcpat_EqConst_Tcpat_PatTest_struct*_tmp1AA=_cycalloc(sizeof(*_tmp1AA));_tmp1AA->tag=6U,_tmp1AA->f1=(unsigned)c;_tmp1AA;});}case 12U: _LL10: _tmp1A2=((struct Cyc_Absyn_Float_p_Absyn_Raw_pat_struct*)_tmp199)->f1;_tmp1A3=((struct Cyc_Absyn_Float_p_Absyn_Raw_pat_struct*)_tmp199)->f2;_LL11: {struct _fat_ptr f=_tmp1A2;int i=_tmp1A3;
return(void*)({struct Cyc_Tcpat_EqFloat_Tcpat_PatTest_struct*_tmp1AB=_cycalloc(sizeof(*_tmp1AB));_tmp1AB->tag=5U,_tmp1AB->f1=f,_tmp1AB->f2=i;_tmp1AB;});}case 13U: _LL12: _tmp1A0=((struct Cyc_Absyn_Enum_p_Absyn_Raw_pat_struct*)_tmp199)->f1;_tmp1A1=((struct Cyc_Absyn_Enum_p_Absyn_Raw_pat_struct*)_tmp199)->f2;_LL13: {struct Cyc_Absyn_Enumdecl*ed=_tmp1A0;struct Cyc_Absyn_Enumfield*ef=_tmp1A1;
return(void*)({struct Cyc_Tcpat_EqEnum_Tcpat_PatTest_struct*_tmp1AC=_cycalloc(sizeof(*_tmp1AC));_tmp1AC->tag=3U,_tmp1AC->f1=ed,_tmp1AC->f2=ef;_tmp1AC;});}case 14U: _LL14: _tmp19E=(void*)((struct Cyc_Absyn_AnonEnum_p_Absyn_Raw_pat_struct*)_tmp199)->f1;_tmp19F=((struct Cyc_Absyn_AnonEnum_p_Absyn_Raw_pat_struct*)_tmp199)->f2;_LL15: {void*t=_tmp19E;struct Cyc_Absyn_Enumfield*ef=_tmp19F;
return(void*)({struct Cyc_Tcpat_EqAnonEnum_Tcpat_PatTest_struct*_tmp1AD=_cycalloc(sizeof(*_tmp1AD));_tmp1AD->tag=4U,_tmp1AD->f1=t,_tmp1AD->f2=ef;_tmp1AD;});}case 6U: _LL16: _LL17:
# 930
{void*_stmttmp1C=({Cyc_Tcutil_compress((void*)_check_null(p->topt));});void*_tmp1AE=_stmttmp1C;void*_tmp1AF;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp1AE)->tag == 3U){_LL1F: _tmp1AF=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp1AE)->f1).ptr_atts).nullable;_LL20: {void*n=_tmp1AF;
# 932
if(({Cyc_Tcutil_force_type2bool(0,n);}))
return(void*)& Cyc_Tcpat_NeqNull_val;
# 932
goto _LL1E;}}else{_LL21: _LL22:
# 935
 goto _LL1E;}_LL1E:;}
# 937
({void*_tmp1B0=0U;({int(*_tmp513)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp1B2)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp1B2;});struct _fat_ptr _tmp512=({const char*_tmp1B1="non-null pointer type or non-pointer type in pointer pattern";_tag_fat(_tmp1B1,sizeof(char),61U);});_tmp513(_tmp512,_tag_fat(_tmp1B0,sizeof(void*),0U));});});case 8U: _LL18: _tmp19C=((struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct*)_tmp199)->f1;_tmp19D=((struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct*)_tmp199)->f2;_LL19: {struct Cyc_Absyn_Datatypedecl*ddecl=_tmp19C;struct Cyc_Absyn_Datatypefield*df=_tmp19D;
# 939
if(ddecl->is_extensible)
return(void*)({struct Cyc_Tcpat_EqExtensibleDatatype_Tcpat_PatTest_struct*_tmp1B3=_cycalloc(sizeof(*_tmp1B3));_tmp1B3->tag=9U,_tmp1B3->f1=ddecl,_tmp1B3->f2=df;_tmp1B3;});else{
# 942
return(void*)({struct Cyc_Tcpat_EqDatatypeTag_Tcpat_PatTest_struct*_tmp1B4=_cycalloc(sizeof(*_tmp1B4));_tmp1B4->tag=7U,({int _tmp514=(int)({Cyc_Tcpat_datatype_tag_number(ddecl,df->name);});_tmp1B4->f1=_tmp514;}),_tmp1B4->f2=ddecl,_tmp1B4->f3=df;_tmp1B4;});}}case 7U: if(((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp199)->f1 != 0){_LL1A: _tmp19A=*((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp199)->f1;_tmp19B=((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp199)->f3;_LL1B: {union Cyc_Absyn_AggrInfo info=_tmp19A;struct Cyc_List_List*dlps=_tmp19B;
# 944
struct Cyc_Absyn_Aggrdecl*ad=({Cyc_Absyn_get_known_aggrdecl(info);});
if(((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->tagged){
struct _tuple19*_stmttmp1D=(struct _tuple19*)((struct Cyc_List_List*)_check_null(dlps))->hd;struct Cyc_Absyn_Pat*_tmp1B6;struct Cyc_List_List*_tmp1B5;_LL24: _tmp1B5=_stmttmp1D->f1;_tmp1B6=_stmttmp1D->f2;_LL25: {struct Cyc_List_List*designators=_tmp1B5;struct Cyc_Absyn_Pat*pat=_tmp1B6;
struct _fat_ptr*f;
{void*_stmttmp1E=(void*)((struct Cyc_List_List*)_check_null(designators))->hd;void*_tmp1B7=_stmttmp1E;struct _fat_ptr*_tmp1B8;if(((struct Cyc_Absyn_FieldName_Absyn_Designator_struct*)_tmp1B7)->tag == 1U){_LL27: _tmp1B8=((struct Cyc_Absyn_FieldName_Absyn_Designator_struct*)_tmp1B7)->f1;_LL28: {struct _fat_ptr*fn=_tmp1B8;
f=fn;goto _LL26;}}else{_LL29: _LL2A:
({void*_tmp1B9=0U;({int(*_tmp516)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp1BB)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp1BB;});struct _fat_ptr _tmp515=({const char*_tmp1BA="no field name in tagged union pattern";_tag_fat(_tmp1BA,sizeof(char),38U);});_tmp516(_tmp515,_tag_fat(_tmp1B9,sizeof(void*),0U));});});}_LL26:;}
# 952
return(void*)({struct Cyc_Tcpat_EqTaggedUnion_Tcpat_PatTest_struct*_tmp1BC=_cycalloc(sizeof(*_tmp1BC));_tmp1BC->tag=8U,_tmp1BC->f1=f,({int _tmp517=({Cyc_Tcpat_get_member_offset(ad,f);});_tmp1BC->f2=_tmp517;});_tmp1BC;});}}else{
# 954
({void*_tmp1BD=0U;({int(*_tmp519)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp1BF)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp1BF;});struct _fat_ptr _tmp518=({const char*_tmp1BE="non-tagged aggregate in pattern test";_tag_fat(_tmp1BE,sizeof(char),37U);});_tmp519(_tmp518,_tag_fat(_tmp1BD,sizeof(void*),0U));});});}}}else{goto _LL1C;}default: _LL1C: _LL1D:
({void*_tmp1C0=0U;({int(*_tmp51B)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp1C2)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp1C2;});struct _fat_ptr _tmp51A=({const char*_tmp1C1="non-test pattern in pattern test";_tag_fat(_tmp1C1,sizeof(char),33U);});_tmp51B(_tmp51A,_tag_fat(_tmp1C0,sizeof(void*),0U));});});}_LL5:;}}_LL0:;}
# 960
static union Cyc_Tcpat_PatOrWhere Cyc_Tcpat_pw(struct Cyc_Absyn_Pat*p){
return({union Cyc_Tcpat_PatOrWhere _tmp42C;(_tmp42C.pattern).tag=1U,(_tmp42C.pattern).val=p;_tmp42C;});}
# 964
static struct Cyc_Tcpat_Con_s*Cyc_Tcpat_null_con(struct Cyc_Absyn_Pat*p){
return({struct Cyc_Tcpat_Con_s*_tmp1C6=_cycalloc(sizeof(*_tmp1C6));({union Cyc_Tcpat_Name_value _tmp51D=({Cyc_Tcpat_Name_v(({const char*_tmp1C5="NULL";_tag_fat(_tmp1C5,sizeof(char),5U);}));});_tmp1C6->name=_tmp51D;}),_tmp1C6->arity=0,_tmp1C6->span=& Cyc_Tcpat_two_opt,({union Cyc_Tcpat_PatOrWhere _tmp51C=({Cyc_Tcpat_pw(p);});_tmp1C6->orig_pat=_tmp51C;});_tmp1C6;});}
# 967
static struct Cyc_Tcpat_Con_s*Cyc_Tcpat_null_ptr_con(struct Cyc_Absyn_Pat*p){
return({struct Cyc_Tcpat_Con_s*_tmp1C9=_cycalloc(sizeof(*_tmp1C9));({union Cyc_Tcpat_Name_value _tmp51F=({Cyc_Tcpat_Name_v(({const char*_tmp1C8="&";_tag_fat(_tmp1C8,sizeof(char),2U);}));});_tmp1C9->name=_tmp51F;}),_tmp1C9->arity=1,_tmp1C9->span=& Cyc_Tcpat_two_opt,({union Cyc_Tcpat_PatOrWhere _tmp51E=({Cyc_Tcpat_pw(p);});_tmp1C9->orig_pat=_tmp51E;});_tmp1C9;});}
# 970
static struct Cyc_Tcpat_Con_s*Cyc_Tcpat_ptr_con(struct Cyc_Absyn_Pat*p){
return({struct Cyc_Tcpat_Con_s*_tmp1CC=_cycalloc(sizeof(*_tmp1CC));({union Cyc_Tcpat_Name_value _tmp521=({Cyc_Tcpat_Name_v(({const char*_tmp1CB="&";_tag_fat(_tmp1CB,sizeof(char),2U);}));});_tmp1CC->name=_tmp521;}),_tmp1CC->arity=1,_tmp1CC->span=& Cyc_Tcpat_one_opt,({union Cyc_Tcpat_PatOrWhere _tmp520=({Cyc_Tcpat_pw(p);});_tmp1CC->orig_pat=_tmp520;});_tmp1CC;});}
# 973
static struct Cyc_Tcpat_Con_s*Cyc_Tcpat_int_con(int i,union Cyc_Tcpat_PatOrWhere p){
return({struct Cyc_Tcpat_Con_s*_tmp1CE=_cycalloc(sizeof(*_tmp1CE));({union Cyc_Tcpat_Name_value _tmp522=({Cyc_Tcpat_Int_v(i);});_tmp1CE->name=_tmp522;}),_tmp1CE->arity=0,_tmp1CE->span=0,_tmp1CE->orig_pat=p;_tmp1CE;});}
# 976
static struct Cyc_Tcpat_Con_s*Cyc_Tcpat_float_con(struct _fat_ptr f,struct Cyc_Absyn_Pat*p){
return({struct Cyc_Tcpat_Con_s*_tmp1D0=_cycalloc(sizeof(*_tmp1D0));({union Cyc_Tcpat_Name_value _tmp524=({Cyc_Tcpat_Name_v(f);});_tmp1D0->name=_tmp524;}),_tmp1D0->arity=0,_tmp1D0->span=0,({union Cyc_Tcpat_PatOrWhere _tmp523=({Cyc_Tcpat_pw(p);});_tmp1D0->orig_pat=_tmp523;});_tmp1D0;});}
# 979
static struct Cyc_Tcpat_Con_s*Cyc_Tcpat_char_con(char c,struct Cyc_Absyn_Pat*p){
return({struct Cyc_Tcpat_Con_s*_tmp1D2=_cycalloc(sizeof(*_tmp1D2));({union Cyc_Tcpat_Name_value _tmp526=({Cyc_Tcpat_Int_v((int)c);});_tmp1D2->name=_tmp526;}),_tmp1D2->arity=0,_tmp1D2->span=& Cyc_Tcpat_twofiftysix_opt,({union Cyc_Tcpat_PatOrWhere _tmp525=({Cyc_Tcpat_pw(p);});_tmp1D2->orig_pat=_tmp525;});_tmp1D2;});}
# 982
static struct Cyc_Tcpat_Con_s*Cyc_Tcpat_tuple_con(int i,union Cyc_Tcpat_PatOrWhere p){
return({struct Cyc_Tcpat_Con_s*_tmp1D5=_cycalloc(sizeof(*_tmp1D5));({union Cyc_Tcpat_Name_value _tmp527=({Cyc_Tcpat_Name_v(({const char*_tmp1D4="$";_tag_fat(_tmp1D4,sizeof(char),2U);}));});_tmp1D5->name=_tmp527;}),_tmp1D5->arity=i,_tmp1D5->span=& Cyc_Tcpat_one_opt,_tmp1D5->orig_pat=p;_tmp1D5;});}
# 987
static void*Cyc_Tcpat_null_pat(struct Cyc_Absyn_Pat*p){
return(void*)({struct Cyc_Tcpat_Con_Tcpat_Simple_pat_struct*_tmp1D7=_cycalloc(sizeof(*_tmp1D7));_tmp1D7->tag=1U,({struct Cyc_Tcpat_Con_s*_tmp528=({Cyc_Tcpat_null_con(p);});_tmp1D7->f1=_tmp528;}),_tmp1D7->f2=0;_tmp1D7;});}
# 990
static void*Cyc_Tcpat_int_pat(int i,union Cyc_Tcpat_PatOrWhere p){
return(void*)({struct Cyc_Tcpat_Con_Tcpat_Simple_pat_struct*_tmp1D9=_cycalloc(sizeof(*_tmp1D9));_tmp1D9->tag=1U,({struct Cyc_Tcpat_Con_s*_tmp529=({Cyc_Tcpat_int_con(i,p);});_tmp1D9->f1=_tmp529;}),_tmp1D9->f2=0;_tmp1D9;});}
# 993
static void*Cyc_Tcpat_char_pat(char c,struct Cyc_Absyn_Pat*p){
return(void*)({struct Cyc_Tcpat_Con_Tcpat_Simple_pat_struct*_tmp1DB=_cycalloc(sizeof(*_tmp1DB));_tmp1DB->tag=1U,({struct Cyc_Tcpat_Con_s*_tmp52A=({Cyc_Tcpat_char_con(c,p);});_tmp1DB->f1=_tmp52A;}),_tmp1DB->f2=0;_tmp1DB;});}
# 996
static void*Cyc_Tcpat_float_pat(struct _fat_ptr f,struct Cyc_Absyn_Pat*p){
return(void*)({struct Cyc_Tcpat_Con_Tcpat_Simple_pat_struct*_tmp1DD=_cycalloc(sizeof(*_tmp1DD));_tmp1DD->tag=1U,({struct Cyc_Tcpat_Con_s*_tmp52B=({Cyc_Tcpat_float_con(f,p);});_tmp1DD->f1=_tmp52B;}),_tmp1DD->f2=0;_tmp1DD;});}
# 999
static void*Cyc_Tcpat_null_ptr_pat(void*p,struct Cyc_Absyn_Pat*p0){
return(void*)({struct Cyc_Tcpat_Con_Tcpat_Simple_pat_struct*_tmp1E0=_cycalloc(sizeof(*_tmp1E0));_tmp1E0->tag=1U,({struct Cyc_Tcpat_Con_s*_tmp52D=({Cyc_Tcpat_null_ptr_con(p0);});_tmp1E0->f1=_tmp52D;}),({struct Cyc_List_List*_tmp52C=({struct Cyc_List_List*_tmp1DF=_cycalloc(sizeof(*_tmp1DF));_tmp1DF->hd=p,_tmp1DF->tl=0;_tmp1DF;});_tmp1E0->f2=_tmp52C;});_tmp1E0;});}
# 1002
static void*Cyc_Tcpat_ptr_pat(void*p,struct Cyc_Absyn_Pat*p0){
return(void*)({struct Cyc_Tcpat_Con_Tcpat_Simple_pat_struct*_tmp1E3=_cycalloc(sizeof(*_tmp1E3));_tmp1E3->tag=1U,({struct Cyc_Tcpat_Con_s*_tmp52F=({Cyc_Tcpat_ptr_con(p0);});_tmp1E3->f1=_tmp52F;}),({struct Cyc_List_List*_tmp52E=({struct Cyc_List_List*_tmp1E2=_cycalloc(sizeof(*_tmp1E2));_tmp1E2->hd=p,_tmp1E2->tl=0;_tmp1E2;});_tmp1E3->f2=_tmp52E;});_tmp1E3;});}
# 1005
static void*Cyc_Tcpat_tuple_pat(struct Cyc_List_List*ss,union Cyc_Tcpat_PatOrWhere p){
return(void*)({struct Cyc_Tcpat_Con_Tcpat_Simple_pat_struct*_tmp1E8=_cycalloc(sizeof(*_tmp1E8));_tmp1E8->tag=1U,({struct Cyc_Tcpat_Con_s*_tmp530=({struct Cyc_Tcpat_Con_s*(*_tmp1E5)(int i,union Cyc_Tcpat_PatOrWhere p)=Cyc_Tcpat_tuple_con;int _tmp1E6=({Cyc_List_length(ss);});union Cyc_Tcpat_PatOrWhere _tmp1E7=p;_tmp1E5(_tmp1E6,_tmp1E7);});_tmp1E8->f1=_tmp530;}),_tmp1E8->f2=ss;_tmp1E8;});}
# 1008
static void*Cyc_Tcpat_con_pat(struct _fat_ptr con_name,int*span,struct Cyc_List_List*ps,struct Cyc_Absyn_Pat*p){
# 1010
struct Cyc_Tcpat_Con_s*c=({struct Cyc_Tcpat_Con_s*_tmp1EB=_cycalloc(sizeof(*_tmp1EB));({union Cyc_Tcpat_Name_value _tmp533=({Cyc_Tcpat_Name_v(con_name);});_tmp1EB->name=_tmp533;}),({int _tmp532=({Cyc_List_length(ps);});_tmp1EB->arity=_tmp532;}),_tmp1EB->span=span,({union Cyc_Tcpat_PatOrWhere _tmp531=({Cyc_Tcpat_pw(p);});_tmp1EB->orig_pat=_tmp531;});_tmp1EB;});
return(void*)({struct Cyc_Tcpat_Con_Tcpat_Simple_pat_struct*_tmp1EA=_cycalloc(sizeof(*_tmp1EA));_tmp1EA->tag=1U,_tmp1EA->f1=c,_tmp1EA->f2=ps;_tmp1EA;});}
# 1015
static void*Cyc_Tcpat_compile_pat(struct Cyc_Absyn_Pat*p){
void*s;
{void*_stmttmp1F=p->r;void*_tmp1ED=_stmttmp1F;struct Cyc_Absyn_Enumfield*_tmp1EF;void*_tmp1EE;struct Cyc_Absyn_Enumfield*_tmp1F1;struct Cyc_Absyn_Enumdecl*_tmp1F0;struct Cyc_List_List*_tmp1F3;struct Cyc_Absyn_Aggrdecl*_tmp1F2;struct Cyc_List_List*_tmp1F4;struct Cyc_List_List*_tmp1F7;struct Cyc_Absyn_Datatypefield*_tmp1F6;struct Cyc_Absyn_Datatypedecl*_tmp1F5;struct Cyc_Absyn_Pat*_tmp1F8;struct Cyc_Absyn_Pat*_tmp1F9;struct Cyc_Absyn_Pat*_tmp1FA;struct _fat_ptr _tmp1FB;char _tmp1FC;int _tmp1FE;enum Cyc_Absyn_Sign _tmp1FD;switch(*((int*)_tmp1ED)){case 0U: _LL1: _LL2:
 goto _LL4;case 2U: _LL3: _LL4:
 goto _LL6;case 4U: _LL5: _LL6:
 s=(void*)({struct Cyc_Tcpat_Any_Tcpat_Simple_pat_struct*_tmp1FF=_cycalloc(sizeof(*_tmp1FF));_tmp1FF->tag=0U;_tmp1FF;});goto _LL0;case 9U: _LL7: _LL8:
 s=({Cyc_Tcpat_null_pat(p);});goto _LL0;case 10U: _LL9: _tmp1FD=((struct Cyc_Absyn_Int_p_Absyn_Raw_pat_struct*)_tmp1ED)->f1;_tmp1FE=((struct Cyc_Absyn_Int_p_Absyn_Raw_pat_struct*)_tmp1ED)->f2;_LLA: {enum Cyc_Absyn_Sign sn=_tmp1FD;int i=_tmp1FE;
s=({void*(*_tmp200)(int i,union Cyc_Tcpat_PatOrWhere p)=Cyc_Tcpat_int_pat;int _tmp201=i;union Cyc_Tcpat_PatOrWhere _tmp202=({Cyc_Tcpat_pw(p);});_tmp200(_tmp201,_tmp202);});goto _LL0;}case 11U: _LLB: _tmp1FC=((struct Cyc_Absyn_Char_p_Absyn_Raw_pat_struct*)_tmp1ED)->f1;_LLC: {char c=_tmp1FC;
s=({Cyc_Tcpat_char_pat(c,p);});goto _LL0;}case 12U: _LLD: _tmp1FB=((struct Cyc_Absyn_Float_p_Absyn_Raw_pat_struct*)_tmp1ED)->f1;_LLE: {struct _fat_ptr f=_tmp1FB;
s=({Cyc_Tcpat_float_pat(f,p);});goto _LL0;}case 1U: _LLF: _tmp1FA=((struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct*)_tmp1ED)->f2;_LL10: {struct Cyc_Absyn_Pat*p2=_tmp1FA;
s=({Cyc_Tcpat_compile_pat(p2);});goto _LL0;}case 3U: _LL11: _tmp1F9=((struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct*)_tmp1ED)->f2;_LL12: {struct Cyc_Absyn_Pat*p2=_tmp1F9;
s=({Cyc_Tcpat_compile_pat(p2);});goto _LL0;}case 6U: _LL13: _tmp1F8=((struct Cyc_Absyn_Pointer_p_Absyn_Raw_pat_struct*)_tmp1ED)->f1;_LL14: {struct Cyc_Absyn_Pat*pp=_tmp1F8;
# 1028
{void*_stmttmp20=({Cyc_Tcutil_compress((void*)_check_null(p->topt));});void*_tmp203=_stmttmp20;void*_tmp204;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp203)->tag == 3U){_LL28: _tmp204=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp203)->f1).ptr_atts).nullable;_LL29: {void*n=_tmp204;
# 1030
int is_nullable=({Cyc_Tcutil_force_type2bool(0,n);});
void*ss=({Cyc_Tcpat_compile_pat(pp);});
if(is_nullable)
s=({Cyc_Tcpat_null_ptr_pat(ss,p);});else{
# 1035
s=({Cyc_Tcpat_ptr_pat(ss,p);});}
goto _LL27;}}else{_LL2A: _LL2B:
({void*_tmp205=0U;({int(*_tmp535)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp207)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp207;});struct _fat_ptr _tmp534=({const char*_tmp206="expecting pointertype for pattern!";_tag_fat(_tmp206,sizeof(char),35U);});_tmp535(_tmp534,_tag_fat(_tmp205,sizeof(void*),0U));});});}_LL27:;}
# 1039
goto _LL0;}case 8U: _LL15: _tmp1F5=((struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct*)_tmp1ED)->f1;_tmp1F6=((struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct*)_tmp1ED)->f2;_tmp1F7=((struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct*)_tmp1ED)->f3;_LL16: {struct Cyc_Absyn_Datatypedecl*tud=_tmp1F5;struct Cyc_Absyn_Datatypefield*tuf=_tmp1F6;struct Cyc_List_List*ps=_tmp1F7;
# 1041
int*span;
{void*_stmttmp21=({Cyc_Tcutil_compress((void*)_check_null(p->topt));});void*_tmp208=_stmttmp21;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp208)->tag == 0U)switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp208)->f1)){case 20U: _LL2D: _LL2E:
# 1044
 if(tud->is_extensible)
span=0;else{
# 1047
span=({int*_tmp209=_cycalloc_atomic(sizeof(*_tmp209));({int _tmp536=({Cyc_List_length((struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(tud->fields))->v);});*_tmp209=_tmp536;});_tmp209;});}
goto _LL2C;case 21U: _LL2F: _LL30:
 span=& Cyc_Tcpat_one_opt;goto _LL2C;default: goto _LL31;}else{_LL31: _LL32:
 span=({void*_tmp20A=0U;({int*(*_tmp538)(struct _fat_ptr,struct _fat_ptr)=({int*(*_tmp20C)(struct _fat_ptr,struct _fat_ptr)=(int*(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp20C;});struct _fat_ptr _tmp537=({const char*_tmp20B="void datatype pattern has bad type";_tag_fat(_tmp20B,sizeof(char),35U);});_tmp538(_tmp537,_tag_fat(_tmp20A,sizeof(void*),0U));});});goto _LL2C;}_LL2C:;}
# 1052
s=({void*(*_tmp20D)(struct _fat_ptr con_name,int*span,struct Cyc_List_List*ps,struct Cyc_Absyn_Pat*p)=Cyc_Tcpat_con_pat;struct _fat_ptr _tmp20E=*(*tuf->name).f2;int*_tmp20F=span;struct Cyc_List_List*_tmp210=({({struct Cyc_List_List*(*_tmp539)(void*(*f)(struct Cyc_Absyn_Pat*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp211)(void*(*f)(struct Cyc_Absyn_Pat*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(void*(*f)(struct Cyc_Absyn_Pat*),struct Cyc_List_List*x))Cyc_List_map;_tmp211;});_tmp539(Cyc_Tcpat_compile_pat,ps);});});struct Cyc_Absyn_Pat*_tmp212=p;_tmp20D(_tmp20E,_tmp20F,_tmp210,_tmp212);});
goto _LL0;}case 5U: _LL17: _tmp1F4=((struct Cyc_Absyn_Tuple_p_Absyn_Raw_pat_struct*)_tmp1ED)->f1;_LL18: {struct Cyc_List_List*ps=_tmp1F4;
# 1056
s=({void*(*_tmp213)(struct Cyc_List_List*ss,union Cyc_Tcpat_PatOrWhere p)=Cyc_Tcpat_tuple_pat;struct Cyc_List_List*_tmp214=({({struct Cyc_List_List*(*_tmp53A)(void*(*f)(struct Cyc_Absyn_Pat*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp215)(void*(*f)(struct Cyc_Absyn_Pat*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(void*(*f)(struct Cyc_Absyn_Pat*),struct Cyc_List_List*x))Cyc_List_map;_tmp215;});_tmp53A(Cyc_Tcpat_compile_pat,ps);});});union Cyc_Tcpat_PatOrWhere _tmp216=({Cyc_Tcpat_pw(p);});_tmp213(_tmp214,_tmp216);});goto _LL0;}case 7U: if(((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp1ED)->f1 != 0){if((((union Cyc_Absyn_AggrInfo*)((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp1ED)->f1)->KnownAggr).tag == 2){_LL19: _tmp1F2=*((((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp1ED)->f1)->KnownAggr).val;_tmp1F3=((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp1ED)->f3;_LL1A: {struct Cyc_Absyn_Aggrdecl*ad=_tmp1F2;struct Cyc_List_List*dlps=_tmp1F3;
# 1061
if((int)ad->kind == (int)Cyc_Absyn_StructA){
struct Cyc_List_List*ps=0;
{struct Cyc_List_List*fields=((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->fields;for(0;fields != 0;fields=fields->tl){
# 1065
int found=({({struct _fat_ptr _tmp53B=(struct _fat_ptr)*((struct Cyc_Absyn_Aggrfield*)fields->hd)->name;Cyc_strcmp(_tmp53B,({const char*_tmp222="";_tag_fat(_tmp222,sizeof(char),1U);}));});})== 0;
{struct Cyc_List_List*dlps0=dlps;for(0;!found && dlps0 != 0;dlps0=dlps0->tl){
struct _tuple19*_stmttmp22=(struct _tuple19*)dlps0->hd;struct Cyc_Absyn_Pat*_tmp218;struct Cyc_List_List*_tmp217;_LL34: _tmp217=_stmttmp22->f1;_tmp218=_stmttmp22->f2;_LL35: {struct Cyc_List_List*dl=_tmp217;struct Cyc_Absyn_Pat*p=_tmp218;
struct Cyc_List_List*_tmp219=dl;struct _fat_ptr*_tmp21A;if(_tmp219 != 0){if(((struct Cyc_Absyn_FieldName_Absyn_Designator_struct*)((struct Cyc_List_List*)_tmp219)->hd)->tag == 1U){if(((struct Cyc_List_List*)_tmp219)->tl == 0){_LL37: _tmp21A=((struct Cyc_Absyn_FieldName_Absyn_Designator_struct*)_tmp219->hd)->f1;_LL38: {struct _fat_ptr*f=_tmp21A;
# 1070
if(({Cyc_strptrcmp(f,((struct Cyc_Absyn_Aggrfield*)fields->hd)->name);})== 0){
ps=({struct Cyc_List_List*_tmp21B=_cycalloc(sizeof(*_tmp21B));({void*_tmp53C=({Cyc_Tcpat_compile_pat(p);});_tmp21B->hd=_tmp53C;}),_tmp21B->tl=ps;_tmp21B;});
found=1;}
# 1070
goto _LL36;}}else{goto _LL39;}}else{goto _LL39;}}else{_LL39: _LL3A:
# 1075
({void*_tmp21C=0U;({int(*_tmp53E)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp21E)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp21E;});struct _fat_ptr _tmp53D=({const char*_tmp21D="bad designator(s)";_tag_fat(_tmp21D,sizeof(char),18U);});_tmp53E(_tmp53D,_tag_fat(_tmp21C,sizeof(void*),0U));});});}_LL36:;}}}
# 1078
if(!found)
({void*_tmp21F=0U;({int(*_tmp540)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp221)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp221;});struct _fat_ptr _tmp53F=({const char*_tmp220="bad designator";_tag_fat(_tmp220,sizeof(char),15U);});_tmp540(_tmp53F,_tag_fat(_tmp21F,sizeof(void*),0U));});});}}
# 1081
s=({void*(*_tmp223)(struct Cyc_List_List*ss,union Cyc_Tcpat_PatOrWhere p)=Cyc_Tcpat_tuple_pat;struct Cyc_List_List*_tmp224=({Cyc_List_imp_rev(ps);});union Cyc_Tcpat_PatOrWhere _tmp225=({Cyc_Tcpat_pw(p);});_tmp223(_tmp224,_tmp225);});}else{
# 1084
if(!((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->tagged)
({void*_tmp226=0U;({unsigned _tmp542=p->loc;struct _fat_ptr _tmp541=({const char*_tmp227="patterns on untagged unions not yet supported.";_tag_fat(_tmp227,sizeof(char),47U);});Cyc_Tcutil_terr(_tmp542,_tmp541,_tag_fat(_tmp226,sizeof(void*),0U));});});{
# 1084
int*span=({int*_tmp234=_cycalloc_atomic(sizeof(*_tmp234));({
# 1086
int _tmp543=({Cyc_List_length(((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->fields);});*_tmp234=_tmp543;});_tmp234;});
struct Cyc_List_List*_tmp228=dlps;struct Cyc_Absyn_Pat*_tmp22A;struct _fat_ptr*_tmp229;if(_tmp228 != 0){if(((struct _tuple19*)((struct Cyc_List_List*)_tmp228)->hd)->f1 != 0){if(((struct Cyc_Absyn_FieldName_Absyn_Designator_struct*)((struct Cyc_List_List*)((struct _tuple19*)((struct Cyc_List_List*)_tmp228)->hd)->f1)->hd)->tag == 1U){if(((struct Cyc_List_List*)((struct _tuple19*)((struct Cyc_List_List*)_tmp228)->hd)->f1)->tl == 0){if(((struct Cyc_List_List*)_tmp228)->tl == 0){_LL3C: _tmp229=((struct Cyc_Absyn_FieldName_Absyn_Designator_struct*)(((struct _tuple19*)_tmp228->hd)->f1)->hd)->f1;_tmp22A=((struct _tuple19*)_tmp228->hd)->f2;_LL3D: {struct _fat_ptr*f=_tmp229;struct Cyc_Absyn_Pat*p2=_tmp22A;
# 1089
s=({void*(*_tmp22B)(struct _fat_ptr con_name,int*span,struct Cyc_List_List*ps,struct Cyc_Absyn_Pat*p)=Cyc_Tcpat_con_pat;struct _fat_ptr _tmp22C=*f;int*_tmp22D=span;struct Cyc_List_List*_tmp22E=({struct Cyc_List_List*_tmp22F=_cycalloc(sizeof(*_tmp22F));({void*_tmp544=({Cyc_Tcpat_compile_pat(p2);});_tmp22F->hd=_tmp544;}),_tmp22F->tl=0;_tmp22F;});struct Cyc_Absyn_Pat*_tmp230=p;_tmp22B(_tmp22C,_tmp22D,_tmp22E,_tmp230);});
goto _LL3B;}}else{goto _LL3E;}}else{goto _LL3E;}}else{goto _LL3E;}}else{goto _LL3E;}}else{_LL3E: _LL3F:
({void*_tmp231=0U;({int(*_tmp546)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp233)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp233;});struct _fat_ptr _tmp545=({const char*_tmp232="bad union pattern";_tag_fat(_tmp232,sizeof(char),18U);});_tmp546(_tmp545,_tag_fat(_tmp231,sizeof(void*),0U));});});}_LL3B:;}}
# 1094
goto _LL0;}}else{goto _LL23;}}else{_LL23: _LL24:
# 1122
 goto _LL26;}case 13U: _LL1B: _tmp1F0=((struct Cyc_Absyn_Enum_p_Absyn_Raw_pat_struct*)_tmp1ED)->f1;_tmp1F1=((struct Cyc_Absyn_Enum_p_Absyn_Raw_pat_struct*)_tmp1ED)->f2;_LL1C: {struct Cyc_Absyn_Enumdecl*ed=_tmp1F0;struct Cyc_Absyn_Enumfield*ef=_tmp1F1;
# 1102
s=({Cyc_Tcpat_con_pat(*(*ef->name).f2,0,0,p);});
goto _LL0;}case 14U: _LL1D: _tmp1EE=(void*)((struct Cyc_Absyn_AnonEnum_p_Absyn_Raw_pat_struct*)_tmp1ED)->f1;_tmp1EF=((struct Cyc_Absyn_AnonEnum_p_Absyn_Raw_pat_struct*)_tmp1ED)->f2;_LL1E: {void*tenum=_tmp1EE;struct Cyc_Absyn_Enumfield*ef=_tmp1EF;
# 1108
struct Cyc_List_List*fields;
{void*_stmttmp23=({Cyc_Tcutil_compress(tenum);});void*_tmp235=_stmttmp23;struct Cyc_List_List*_tmp236;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp235)->tag == 0U){if(((struct Cyc_Absyn_AnonEnumCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp235)->f1)->tag == 18U){_LL41: _tmp236=((struct Cyc_Absyn_AnonEnumCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp235)->f1)->f1;_LL42: {struct Cyc_List_List*fs=_tmp236;
fields=fs;goto _LL40;}}else{goto _LL43;}}else{_LL43: _LL44:
({void*_tmp237=0U;({int(*_tmp548)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp239)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp239;});struct _fat_ptr _tmp547=({const char*_tmp238="bad type in AnonEnum_p";_tag_fat(_tmp238,sizeof(char),23U);});_tmp548(_tmp547,_tag_fat(_tmp237,sizeof(void*),0U));});});}_LL40:;}
# 1118
s=({Cyc_Tcpat_con_pat(*(*ef->name).f2,0,0,p);});
goto _LL0;}case 15U: _LL1F: _LL20:
 goto _LL22;case 16U: _LL21: _LL22:
 goto _LL24;default: _LL25: _LL26:
# 1123
 s=(void*)({struct Cyc_Tcpat_Any_Tcpat_Simple_pat_struct*_tmp23A=_cycalloc(sizeof(*_tmp23A));_tmp23A->tag=0U;_tmp23A;});}_LL0:;}
# 1125
return s;}struct Cyc_Tcpat_Pos_Tcpat_Term_desc_struct{int tag;struct Cyc_Tcpat_Con_s*f1;struct Cyc_List_List*f2;};struct Cyc_Tcpat_Neg_Tcpat_Term_desc_struct{int tag;struct Cyc_Set_Set*f1;};
# 1154
static int Cyc_Tcpat_same_access(void*a1,void*a2){
struct _tuple0 _stmttmp24=({struct _tuple0 _tmp42D;_tmp42D.f1=a1,_tmp42D.f2=a2;_tmp42D;});struct _tuple0 _tmp23C=_stmttmp24;struct _fat_ptr*_tmp240;int _tmp23F;struct _fat_ptr*_tmp23E;int _tmp23D;unsigned _tmp244;struct Cyc_Absyn_Datatypefield*_tmp243;unsigned _tmp242;struct Cyc_Absyn_Datatypefield*_tmp241;unsigned _tmp246;unsigned _tmp245;switch(*((int*)_tmp23C.f1)){case 0U: if(((struct Cyc_Tcpat_Dummy_Tcpat_Access_struct*)_tmp23C.f2)->tag == 0U){_LL1: _LL2:
 return 1;}else{goto _LLB;}case 1U: if(((struct Cyc_Tcpat_Deref_Tcpat_Access_struct*)_tmp23C.f2)->tag == 1U){_LL3: _LL4:
 return 1;}else{goto _LLB;}case 2U: if(((struct Cyc_Tcpat_TupleField_Tcpat_Access_struct*)_tmp23C.f2)->tag == 2U){_LL5: _tmp245=((struct Cyc_Tcpat_TupleField_Tcpat_Access_struct*)_tmp23C.f1)->f1;_tmp246=((struct Cyc_Tcpat_TupleField_Tcpat_Access_struct*)_tmp23C.f2)->f1;_LL6: {unsigned i1=_tmp245;unsigned i2=_tmp246;
return i1 == i2;}}else{goto _LLB;}case 3U: if(((struct Cyc_Tcpat_DatatypeField_Tcpat_Access_struct*)_tmp23C.f2)->tag == 3U){_LL7: _tmp241=((struct Cyc_Tcpat_DatatypeField_Tcpat_Access_struct*)_tmp23C.f1)->f2;_tmp242=((struct Cyc_Tcpat_DatatypeField_Tcpat_Access_struct*)_tmp23C.f1)->f3;_tmp243=((struct Cyc_Tcpat_DatatypeField_Tcpat_Access_struct*)_tmp23C.f2)->f2;_tmp244=((struct Cyc_Tcpat_DatatypeField_Tcpat_Access_struct*)_tmp23C.f2)->f3;_LL8: {struct Cyc_Absyn_Datatypefield*df1=_tmp241;unsigned i1=_tmp242;struct Cyc_Absyn_Datatypefield*df2=_tmp243;unsigned i2=_tmp244;
# 1160
return df1 == df2 && i1 == i2;}}else{goto _LLB;}default: if(((struct Cyc_Tcpat_AggrField_Tcpat_Access_struct*)_tmp23C.f2)->tag == 4U){_LL9: _tmp23D=((struct Cyc_Tcpat_AggrField_Tcpat_Access_struct*)_tmp23C.f1)->f1;_tmp23E=((struct Cyc_Tcpat_AggrField_Tcpat_Access_struct*)_tmp23C.f1)->f2;_tmp23F=((struct Cyc_Tcpat_AggrField_Tcpat_Access_struct*)_tmp23C.f2)->f1;_tmp240=((struct Cyc_Tcpat_AggrField_Tcpat_Access_struct*)_tmp23C.f2)->f2;_LLA: {int b1=_tmp23D;struct _fat_ptr*f1=_tmp23E;int b2=_tmp23F;struct _fat_ptr*f2=_tmp240;
# 1162
return b1 == b2 &&({Cyc_strptrcmp(f1,f2);})== 0;}}else{_LLB: _LLC:
 return 0;}}_LL0:;}
# 1167
static int Cyc_Tcpat_same_path(struct Cyc_List_List*p1,struct Cyc_List_List*p2){
while(p1 != 0 && p2 != 0){
if(!({Cyc_Tcpat_same_access(((struct Cyc_Tcpat_PathNode*)p1->hd)->access,((struct Cyc_Tcpat_PathNode*)p2->hd)->access);}))return 0;p1=p1->tl;
# 1171
p2=p2->tl;}
# 1173
if(p1 != p2)return 0;return 1;}
# 1177
static void*Cyc_Tcpat_ifeq(struct Cyc_List_List*access,struct Cyc_Tcpat_Con_s*con,void*d1,void*d2){
void*test=({Cyc_Tcpat_get_pat_test(con->orig_pat);});
{void*_tmp249=d2;void*_tmp24C;struct Cyc_List_List*_tmp24B;struct Cyc_List_List*_tmp24A;if(((struct Cyc_Tcpat_SwitchDec_Tcpat_Decision_struct*)_tmp249)->tag == 2U){_LL1: _tmp24A=((struct Cyc_Tcpat_SwitchDec_Tcpat_Decision_struct*)_tmp249)->f1;_tmp24B=((struct Cyc_Tcpat_SwitchDec_Tcpat_Decision_struct*)_tmp249)->f2;_tmp24C=(void*)((struct Cyc_Tcpat_SwitchDec_Tcpat_Decision_struct*)_tmp249)->f3;_LL2: {struct Cyc_List_List*access2=_tmp24A;struct Cyc_List_List*switch_clauses=_tmp24B;void*default_decision=_tmp24C;
# 1181
if(({Cyc_Tcpat_same_path(access,access2);})&&(int)(((con->orig_pat).pattern).tag == 1))
return(void*)({struct Cyc_Tcpat_SwitchDec_Tcpat_Decision_struct*_tmp24F=_cycalloc(sizeof(*_tmp24F));_tmp24F->tag=2U,_tmp24F->f1=access2,({
struct Cyc_List_List*_tmp54A=({struct Cyc_List_List*_tmp24E=_cycalloc(sizeof(*_tmp24E));({struct _tuple0*_tmp549=({struct _tuple0*_tmp24D=_cycalloc(sizeof(*_tmp24D));_tmp24D->f1=test,_tmp24D->f2=d1;_tmp24D;});_tmp24E->hd=_tmp549;}),_tmp24E->tl=switch_clauses;_tmp24E;});_tmp24F->f2=_tmp54A;}),_tmp24F->f3=default_decision;_tmp24F;});else{
# 1185
goto _LL0;}}}else{_LL3: _LL4:
 goto _LL0;}_LL0:;}
# 1188
return(void*)({struct Cyc_Tcpat_SwitchDec_Tcpat_Decision_struct*_tmp252=_cycalloc(sizeof(*_tmp252));_tmp252->tag=2U,_tmp252->f1=access,({struct Cyc_List_List*_tmp54C=({struct Cyc_List_List*_tmp251=_cycalloc(sizeof(*_tmp251));({struct _tuple0*_tmp54B=({struct _tuple0*_tmp250=_cycalloc(sizeof(*_tmp250));_tmp250->f1=test,_tmp250->f2=d1;_tmp250;});_tmp251->hd=_tmp54B;}),_tmp251->tl=0;_tmp251;});_tmp252->f2=_tmp54C;}),_tmp252->f3=d2;_tmp252;});}struct _tuple21{struct Cyc_List_List*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;};
# 1199
enum Cyc_Tcpat_Answer{Cyc_Tcpat_Yes =0U,Cyc_Tcpat_No =1U,Cyc_Tcpat_Maybe =2U};
# 1202
static void Cyc_Tcpat_print_tab(int i){
for(0;i != 0;-- i){
({void*_tmp254=0U;({struct Cyc___cycFILE*_tmp54E=Cyc_stderr;struct _fat_ptr _tmp54D=({const char*_tmp255=" ";_tag_fat(_tmp255,sizeof(char),2U);});Cyc_fprintf(_tmp54E,_tmp54D,_tag_fat(_tmp254,sizeof(void*),0U));});});}}
# 1208
static void Cyc_Tcpat_print_con(struct Cyc_Tcpat_Con_s*c){
union Cyc_Tcpat_Name_value n=c->name;
union Cyc_Tcpat_Name_value _tmp257=n;int _tmp258;struct _fat_ptr _tmp259;if((_tmp257.Name_v).tag == 1){_LL1: _tmp259=(_tmp257.Name_v).val;_LL2: {struct _fat_ptr s=_tmp259;
({struct Cyc_String_pa_PrintArg_struct _tmp25C=({struct Cyc_String_pa_PrintArg_struct _tmp42E;_tmp42E.tag=0U,_tmp42E.f1=(struct _fat_ptr)((struct _fat_ptr)s);_tmp42E;});void*_tmp25A[1U];_tmp25A[0]=& _tmp25C;({struct Cyc___cycFILE*_tmp550=Cyc_stderr;struct _fat_ptr _tmp54F=({const char*_tmp25B="%s";_tag_fat(_tmp25B,sizeof(char),3U);});Cyc_fprintf(_tmp550,_tmp54F,_tag_fat(_tmp25A,sizeof(void*),1U));});});goto _LL0;}}else{_LL3: _tmp258=(_tmp257.Int_v).val;_LL4: {int i=_tmp258;
({struct Cyc_Int_pa_PrintArg_struct _tmp25F=({struct Cyc_Int_pa_PrintArg_struct _tmp42F;_tmp42F.tag=1U,_tmp42F.f1=(unsigned long)i;_tmp42F;});void*_tmp25D[1U];_tmp25D[0]=& _tmp25F;({struct Cyc___cycFILE*_tmp552=Cyc_stderr;struct _fat_ptr _tmp551=({const char*_tmp25E="%d";_tag_fat(_tmp25E,sizeof(char),3U);});Cyc_fprintf(_tmp552,_tmp551,_tag_fat(_tmp25D,sizeof(void*),1U));});});goto _LL0;}}_LL0:;}
# 1216
static void Cyc_Tcpat_print_access(void*a){
void*_tmp261=a;struct _fat_ptr*_tmp263;int _tmp262;unsigned _tmp265;struct Cyc_Absyn_Datatypefield*_tmp264;unsigned _tmp266;switch(*((int*)_tmp261)){case 0U: _LL1: _LL2:
({void*_tmp267=0U;({struct Cyc___cycFILE*_tmp554=Cyc_stderr;struct _fat_ptr _tmp553=({const char*_tmp268="DUMMY";_tag_fat(_tmp268,sizeof(char),6U);});Cyc_fprintf(_tmp554,_tmp553,_tag_fat(_tmp267,sizeof(void*),0U));});});goto _LL0;case 1U: _LL3: _LL4:
({void*_tmp269=0U;({struct Cyc___cycFILE*_tmp556=Cyc_stderr;struct _fat_ptr _tmp555=({const char*_tmp26A="*";_tag_fat(_tmp26A,sizeof(char),2U);});Cyc_fprintf(_tmp556,_tmp555,_tag_fat(_tmp269,sizeof(void*),0U));});});goto _LL0;case 2U: _LL5: _tmp266=((struct Cyc_Tcpat_TupleField_Tcpat_Access_struct*)_tmp261)->f1;_LL6: {unsigned i=_tmp266;
({struct Cyc_Int_pa_PrintArg_struct _tmp26D=({struct Cyc_Int_pa_PrintArg_struct _tmp430;_tmp430.tag=1U,_tmp430.f1=(unsigned long)((int)i);_tmp430;});void*_tmp26B[1U];_tmp26B[0]=& _tmp26D;({struct Cyc___cycFILE*_tmp558=Cyc_stderr;struct _fat_ptr _tmp557=({const char*_tmp26C="[%d]";_tag_fat(_tmp26C,sizeof(char),5U);});Cyc_fprintf(_tmp558,_tmp557,_tag_fat(_tmp26B,sizeof(void*),1U));});});goto _LL0;}case 3U: _LL7: _tmp264=((struct Cyc_Tcpat_DatatypeField_Tcpat_Access_struct*)_tmp261)->f2;_tmp265=((struct Cyc_Tcpat_DatatypeField_Tcpat_Access_struct*)_tmp261)->f3;_LL8: {struct Cyc_Absyn_Datatypefield*f=_tmp264;unsigned i=_tmp265;
# 1222
({struct Cyc_String_pa_PrintArg_struct _tmp270=({struct Cyc_String_pa_PrintArg_struct _tmp432;_tmp432.tag=0U,({struct _fat_ptr _tmp559=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(f->name);}));_tmp432.f1=_tmp559;});_tmp432;});struct Cyc_Int_pa_PrintArg_struct _tmp271=({struct Cyc_Int_pa_PrintArg_struct _tmp431;_tmp431.tag=1U,_tmp431.f1=(unsigned long)((int)i);_tmp431;});void*_tmp26E[2U];_tmp26E[0]=& _tmp270,_tmp26E[1]=& _tmp271;({struct Cyc___cycFILE*_tmp55B=Cyc_stderr;struct _fat_ptr _tmp55A=({const char*_tmp26F="%s[%d]";_tag_fat(_tmp26F,sizeof(char),7U);});Cyc_fprintf(_tmp55B,_tmp55A,_tag_fat(_tmp26E,sizeof(void*),2U));});});goto _LL0;}default: _LL9: _tmp262=((struct Cyc_Tcpat_AggrField_Tcpat_Access_struct*)_tmp261)->f1;_tmp263=((struct Cyc_Tcpat_AggrField_Tcpat_Access_struct*)_tmp261)->f2;_LLA: {int tagged=_tmp262;struct _fat_ptr*f=_tmp263;
# 1224
if(tagged)
({struct Cyc_String_pa_PrintArg_struct _tmp274=({struct Cyc_String_pa_PrintArg_struct _tmp433;_tmp433.tag=0U,_tmp433.f1=(struct _fat_ptr)((struct _fat_ptr)*f);_tmp433;});void*_tmp272[1U];_tmp272[0]=& _tmp274;({struct Cyc___cycFILE*_tmp55D=Cyc_stderr;struct _fat_ptr _tmp55C=({const char*_tmp273=".tagunion.%s";_tag_fat(_tmp273,sizeof(char),13U);});Cyc_fprintf(_tmp55D,_tmp55C,_tag_fat(_tmp272,sizeof(void*),1U));});});else{
# 1227
({struct Cyc_String_pa_PrintArg_struct _tmp277=({struct Cyc_String_pa_PrintArg_struct _tmp434;_tmp434.tag=0U,_tmp434.f1=(struct _fat_ptr)((struct _fat_ptr)*f);_tmp434;});void*_tmp275[1U];_tmp275[0]=& _tmp277;({struct Cyc___cycFILE*_tmp55F=Cyc_stderr;struct _fat_ptr _tmp55E=({const char*_tmp276=".%s";_tag_fat(_tmp276,sizeof(char),4U);});Cyc_fprintf(_tmp55F,_tmp55E,_tag_fat(_tmp275,sizeof(void*),1U));});});}
goto _LL0;}}_LL0:;}
# 1232
static void Cyc_Tcpat_print_pat_test(void*p){
void*_tmp279=p;struct Cyc_Absyn_Datatypefield*_tmp27A;int _tmp27C;struct _fat_ptr*_tmp27B;int _tmp27D;unsigned _tmp27E;struct _fat_ptr _tmp27F;struct Cyc_Absyn_Enumfield*_tmp280;struct Cyc_Absyn_Enumfield*_tmp281;struct Cyc_Absyn_Exp*_tmp282;switch(*((int*)_tmp279)){case 0U: if(((struct Cyc_Tcpat_WhereTest_Tcpat_PatTest_struct*)_tmp279)->f1 == 0){_LL1: _LL2:
({void*_tmp283=0U;({struct Cyc___cycFILE*_tmp561=Cyc_stderr;struct _fat_ptr _tmp560=({const char*_tmp284="where(NULL)";_tag_fat(_tmp284,sizeof(char),12U);});Cyc_fprintf(_tmp561,_tmp560,_tag_fat(_tmp283,sizeof(void*),0U));});});goto _LL0;}else{_LL3: _tmp282=((struct Cyc_Tcpat_WhereTest_Tcpat_PatTest_struct*)_tmp279)->f1;_LL4: {struct Cyc_Absyn_Exp*e=_tmp282;
({struct Cyc_String_pa_PrintArg_struct _tmp287=({struct Cyc_String_pa_PrintArg_struct _tmp435;_tmp435.tag=0U,({struct _fat_ptr _tmp562=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_exp2string((struct Cyc_Absyn_Exp*)_check_null(e));}));_tmp435.f1=_tmp562;});_tmp435;});void*_tmp285[1U];_tmp285[0]=& _tmp287;({struct Cyc___cycFILE*_tmp564=Cyc_stderr;struct _fat_ptr _tmp563=({const char*_tmp286="where(%s)";_tag_fat(_tmp286,sizeof(char),10U);});Cyc_fprintf(_tmp564,_tmp563,_tag_fat(_tmp285,sizeof(void*),1U));});});goto _LL0;}}case 1U: _LL5: _LL6:
({void*_tmp288=0U;({struct Cyc___cycFILE*_tmp566=Cyc_stderr;struct _fat_ptr _tmp565=({const char*_tmp289="NULL";_tag_fat(_tmp289,sizeof(char),5U);});Cyc_fprintf(_tmp566,_tmp565,_tag_fat(_tmp288,sizeof(void*),0U));});});goto _LL0;case 2U: _LL7: _LL8:
({void*_tmp28A=0U;({struct Cyc___cycFILE*_tmp568=Cyc_stderr;struct _fat_ptr _tmp567=({const char*_tmp28B="NOT-NULL:";_tag_fat(_tmp28B,sizeof(char),10U);});Cyc_fprintf(_tmp568,_tmp567,_tag_fat(_tmp28A,sizeof(void*),0U));});});goto _LL0;case 4U: _LL9: _tmp281=((struct Cyc_Tcpat_EqAnonEnum_Tcpat_PatTest_struct*)_tmp279)->f2;_LLA: {struct Cyc_Absyn_Enumfield*ef=_tmp281;
_tmp280=ef;goto _LLC;}case 3U: _LLB: _tmp280=((struct Cyc_Tcpat_EqEnum_Tcpat_PatTest_struct*)_tmp279)->f2;_LLC: {struct Cyc_Absyn_Enumfield*ef=_tmp280;
({struct Cyc_String_pa_PrintArg_struct _tmp28E=({struct Cyc_String_pa_PrintArg_struct _tmp436;_tmp436.tag=0U,({struct _fat_ptr _tmp569=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(ef->name);}));_tmp436.f1=_tmp569;});_tmp436;});void*_tmp28C[1U];_tmp28C[0]=& _tmp28E;({struct Cyc___cycFILE*_tmp56B=Cyc_stderr;struct _fat_ptr _tmp56A=({const char*_tmp28D="%s";_tag_fat(_tmp28D,sizeof(char),3U);});Cyc_fprintf(_tmp56B,_tmp56A,_tag_fat(_tmp28C,sizeof(void*),1U));});});goto _LL0;}case 5U: _LLD: _tmp27F=((struct Cyc_Tcpat_EqFloat_Tcpat_PatTest_struct*)_tmp279)->f1;_LLE: {struct _fat_ptr s=_tmp27F;
({struct Cyc_String_pa_PrintArg_struct _tmp291=({struct Cyc_String_pa_PrintArg_struct _tmp437;_tmp437.tag=0U,_tmp437.f1=(struct _fat_ptr)((struct _fat_ptr)s);_tmp437;});void*_tmp28F[1U];_tmp28F[0]=& _tmp291;({struct Cyc___cycFILE*_tmp56D=Cyc_stderr;struct _fat_ptr _tmp56C=({const char*_tmp290="%s";_tag_fat(_tmp290,sizeof(char),3U);});Cyc_fprintf(_tmp56D,_tmp56C,_tag_fat(_tmp28F,sizeof(void*),1U));});});goto _LL0;}case 6U: _LLF: _tmp27E=((struct Cyc_Tcpat_EqConst_Tcpat_PatTest_struct*)_tmp279)->f1;_LL10: {unsigned i=_tmp27E;
({struct Cyc_Int_pa_PrintArg_struct _tmp294=({struct Cyc_Int_pa_PrintArg_struct _tmp438;_tmp438.tag=1U,_tmp438.f1=(unsigned long)((int)i);_tmp438;});void*_tmp292[1U];_tmp292[0]=& _tmp294;({struct Cyc___cycFILE*_tmp56F=Cyc_stderr;struct _fat_ptr _tmp56E=({const char*_tmp293="%d";_tag_fat(_tmp293,sizeof(char),3U);});Cyc_fprintf(_tmp56F,_tmp56E,_tag_fat(_tmp292,sizeof(void*),1U));});});goto _LL0;}case 7U: _LL11: _tmp27D=((struct Cyc_Tcpat_EqDatatypeTag_Tcpat_PatTest_struct*)_tmp279)->f1;_LL12: {int i=_tmp27D;
({struct Cyc_Int_pa_PrintArg_struct _tmp297=({struct Cyc_Int_pa_PrintArg_struct _tmp439;_tmp439.tag=1U,_tmp439.f1=(unsigned long)i;_tmp439;});void*_tmp295[1U];_tmp295[0]=& _tmp297;({struct Cyc___cycFILE*_tmp571=Cyc_stderr;struct _fat_ptr _tmp570=({const char*_tmp296="datatypetag(%d)";_tag_fat(_tmp296,sizeof(char),16U);});Cyc_fprintf(_tmp571,_tmp570,_tag_fat(_tmp295,sizeof(void*),1U));});});goto _LL0;}case 8U: _LL13: _tmp27B=((struct Cyc_Tcpat_EqTaggedUnion_Tcpat_PatTest_struct*)_tmp279)->f1;_tmp27C=((struct Cyc_Tcpat_EqTaggedUnion_Tcpat_PatTest_struct*)_tmp279)->f2;_LL14: {struct _fat_ptr*f=_tmp27B;int i=_tmp27C;
({struct Cyc_String_pa_PrintArg_struct _tmp29A=({struct Cyc_String_pa_PrintArg_struct _tmp43B;_tmp43B.tag=0U,_tmp43B.f1=(struct _fat_ptr)((struct _fat_ptr)*f);_tmp43B;});struct Cyc_Int_pa_PrintArg_struct _tmp29B=({struct Cyc_Int_pa_PrintArg_struct _tmp43A;_tmp43A.tag=1U,_tmp43A.f1=(unsigned long)i;_tmp43A;});void*_tmp298[2U];_tmp298[0]=& _tmp29A,_tmp298[1]=& _tmp29B;({struct Cyc___cycFILE*_tmp573=Cyc_stderr;struct _fat_ptr _tmp572=({const char*_tmp299="uniontag[%s](%d)";_tag_fat(_tmp299,sizeof(char),17U);});Cyc_fprintf(_tmp573,_tmp572,_tag_fat(_tmp298,sizeof(void*),2U));});});goto _LL0;}default: _LL15: _tmp27A=((struct Cyc_Tcpat_EqExtensibleDatatype_Tcpat_PatTest_struct*)_tmp279)->f2;_LL16: {struct Cyc_Absyn_Datatypefield*f=_tmp27A;
# 1245
({struct Cyc_String_pa_PrintArg_struct _tmp29E=({struct Cyc_String_pa_PrintArg_struct _tmp43C;_tmp43C.tag=0U,({struct _fat_ptr _tmp574=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(f->name);}));_tmp43C.f1=_tmp574;});_tmp43C;});void*_tmp29C[1U];_tmp29C[0]=& _tmp29E;({struct Cyc___cycFILE*_tmp576=Cyc_stderr;struct _fat_ptr _tmp575=({const char*_tmp29D="datatypefield(%s)";_tag_fat(_tmp29D,sizeof(char),18U);});Cyc_fprintf(_tmp576,_tmp575,_tag_fat(_tmp29C,sizeof(void*),1U));});});}}_LL0:;}
# 1249
static void Cyc_Tcpat_print_rhs(struct Cyc_Tcpat_Rhs*r){
({struct Cyc_String_pa_PrintArg_struct _tmp2A2=({struct Cyc_String_pa_PrintArg_struct _tmp43D;_tmp43D.tag=0U,({struct _fat_ptr _tmp577=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_stmt2string(r->rhs);}));_tmp43D.f1=_tmp577;});_tmp43D;});void*_tmp2A0[1U];_tmp2A0[0]=& _tmp2A2;({struct Cyc___cycFILE*_tmp579=Cyc_stderr;struct _fat_ptr _tmp578=({const char*_tmp2A1="%s";_tag_fat(_tmp2A1,sizeof(char),3U);});Cyc_fprintf(_tmp579,_tmp578,_tag_fat(_tmp2A0,sizeof(void*),1U));});});}
# 1253
static void Cyc_Tcpat_print_dec_tree(void*d,int tab){
void*_tmp2A4=d;void*_tmp2A7;struct Cyc_List_List*_tmp2A6;struct Cyc_List_List*_tmp2A5;struct Cyc_Tcpat_Rhs*_tmp2A8;switch(*((int*)_tmp2A4)){case 1U: _LL1: _tmp2A8=((struct Cyc_Tcpat_Success_Tcpat_Decision_struct*)_tmp2A4)->f1;_LL2: {struct Cyc_Tcpat_Rhs*rhs=_tmp2A8;
# 1256
({Cyc_Tcpat_print_tab(tab);});
({void*_tmp2A9=0U;({struct Cyc___cycFILE*_tmp57B=Cyc_stderr;struct _fat_ptr _tmp57A=({const char*_tmp2AA="Success(";_tag_fat(_tmp2AA,sizeof(char),9U);});Cyc_fprintf(_tmp57B,_tmp57A,_tag_fat(_tmp2A9,sizeof(void*),0U));});});({Cyc_Tcpat_print_rhs(rhs);});({void*_tmp2AB=0U;({struct Cyc___cycFILE*_tmp57D=Cyc_stderr;struct _fat_ptr _tmp57C=({const char*_tmp2AC=")\n";_tag_fat(_tmp2AC,sizeof(char),3U);});Cyc_fprintf(_tmp57D,_tmp57C,_tag_fat(_tmp2AB,sizeof(void*),0U));});});
goto _LL0;}case 0U: _LL3: _LL4:
({void*_tmp2AD=0U;({struct Cyc___cycFILE*_tmp57F=Cyc_stderr;struct _fat_ptr _tmp57E=({const char*_tmp2AE="Failure\n";_tag_fat(_tmp2AE,sizeof(char),9U);});Cyc_fprintf(_tmp57F,_tmp57E,_tag_fat(_tmp2AD,sizeof(void*),0U));});});goto _LL0;default: _LL5: _tmp2A5=((struct Cyc_Tcpat_SwitchDec_Tcpat_Decision_struct*)_tmp2A4)->f1;_tmp2A6=((struct Cyc_Tcpat_SwitchDec_Tcpat_Decision_struct*)_tmp2A4)->f2;_tmp2A7=(void*)((struct Cyc_Tcpat_SwitchDec_Tcpat_Decision_struct*)_tmp2A4)->f3;_LL6: {struct Cyc_List_List*a=_tmp2A5;struct Cyc_List_List*cases=_tmp2A6;void*def=_tmp2A7;
# 1261
({Cyc_Tcpat_print_tab(tab);});
({void*_tmp2AF=0U;({struct Cyc___cycFILE*_tmp581=Cyc_stderr;struct _fat_ptr _tmp580=({const char*_tmp2B0="Switch[";_tag_fat(_tmp2B0,sizeof(char),8U);});Cyc_fprintf(_tmp581,_tmp580,_tag_fat(_tmp2AF,sizeof(void*),0U));});});
for(0;a != 0;a=a->tl){
({Cyc_Tcpat_print_access(((struct Cyc_Tcpat_PathNode*)a->hd)->access);});
if(a->tl != 0)({void*_tmp2B1=0U;({struct Cyc___cycFILE*_tmp583=Cyc_stderr;struct _fat_ptr _tmp582=({const char*_tmp2B2=",";_tag_fat(_tmp2B2,sizeof(char),2U);});Cyc_fprintf(_tmp583,_tmp582,_tag_fat(_tmp2B1,sizeof(void*),0U));});});}
# 1267
({void*_tmp2B3=0U;({struct Cyc___cycFILE*_tmp585=Cyc_stderr;struct _fat_ptr _tmp584=({const char*_tmp2B4="] {\n";_tag_fat(_tmp2B4,sizeof(char),5U);});Cyc_fprintf(_tmp585,_tmp584,_tag_fat(_tmp2B3,sizeof(void*),0U));});});
for(0;cases != 0;cases=cases->tl){
struct _tuple0 _stmttmp25=*((struct _tuple0*)cases->hd);void*_tmp2B6;void*_tmp2B5;_LL8: _tmp2B5=_stmttmp25.f1;_tmp2B6=_stmttmp25.f2;_LL9: {void*pt=_tmp2B5;void*d=_tmp2B6;
({Cyc_Tcpat_print_tab(tab);});
({void*_tmp2B7=0U;({struct Cyc___cycFILE*_tmp587=Cyc_stderr;struct _fat_ptr _tmp586=({const char*_tmp2B8="case ";_tag_fat(_tmp2B8,sizeof(char),6U);});Cyc_fprintf(_tmp587,_tmp586,_tag_fat(_tmp2B7,sizeof(void*),0U));});});
({Cyc_Tcpat_print_pat_test(pt);});
({void*_tmp2B9=0U;({struct Cyc___cycFILE*_tmp589=Cyc_stderr;struct _fat_ptr _tmp588=({const char*_tmp2BA=":\n";_tag_fat(_tmp2BA,sizeof(char),3U);});Cyc_fprintf(_tmp589,_tmp588,_tag_fat(_tmp2B9,sizeof(void*),0U));});});
({Cyc_Tcpat_print_dec_tree(d,tab + 7);});}}
# 1276
({Cyc_Tcpat_print_tab(tab);});
({void*_tmp2BB=0U;({struct Cyc___cycFILE*_tmp58B=Cyc_stderr;struct _fat_ptr _tmp58A=({const char*_tmp2BC="default:\n";_tag_fat(_tmp2BC,sizeof(char),10U);});Cyc_fprintf(_tmp58B,_tmp58A,_tag_fat(_tmp2BB,sizeof(void*),0U));});});
({Cyc_Tcpat_print_dec_tree(def,tab + 7);});
({Cyc_Tcpat_print_tab(tab);});
({void*_tmp2BD=0U;({struct Cyc___cycFILE*_tmp58D=Cyc_stderr;struct _fat_ptr _tmp58C=({const char*_tmp2BE="}\n";_tag_fat(_tmp2BE,sizeof(char),3U);});Cyc_fprintf(_tmp58D,_tmp58C,_tag_fat(_tmp2BD,sizeof(void*),0U));});});}}_LL0:;}
# 1284
void Cyc_Tcpat_print_decision_tree(void*d){
({Cyc_Tcpat_print_dec_tree(d,0);});}
# 1292
static void*Cyc_Tcpat_add_neg(void*td,struct Cyc_Tcpat_Con_s*c){
void*_tmp2C1=td;struct Cyc_Set_Set*_tmp2C2;if(((struct Cyc_Tcpat_Neg_Tcpat_Term_desc_struct*)_tmp2C1)->tag == 1U){_LL1: _tmp2C2=((struct Cyc_Tcpat_Neg_Tcpat_Term_desc_struct*)_tmp2C1)->f1;_LL2: {struct Cyc_Set_Set*cs=_tmp2C2;
# 1302
return(void*)({struct Cyc_Tcpat_Neg_Tcpat_Term_desc_struct*_tmp2C4=_cycalloc(sizeof(*_tmp2C4));_tmp2C4->tag=1U,({struct Cyc_Set_Set*_tmp590=({({struct Cyc_Set_Set*(*_tmp58F)(struct Cyc_Set_Set*s,struct Cyc_Tcpat_Con_s*elt)=({struct Cyc_Set_Set*(*_tmp2C3)(struct Cyc_Set_Set*s,struct Cyc_Tcpat_Con_s*elt)=(struct Cyc_Set_Set*(*)(struct Cyc_Set_Set*s,struct Cyc_Tcpat_Con_s*elt))Cyc_Set_insert;_tmp2C3;});struct Cyc_Set_Set*_tmp58E=cs;_tmp58F(_tmp58E,c);});});_tmp2C4->f1=_tmp590;});_tmp2C4;});}}else{_LL3: _LL4:
({void*_tmp2C5=0U;({int(*_tmp592)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp2C7)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp2C7;});struct _fat_ptr _tmp591=({const char*_tmp2C6="add_neg called when td is Positive";_tag_fat(_tmp2C6,sizeof(char),35U);});_tmp592(_tmp591,_tag_fat(_tmp2C5,sizeof(void*),0U));});});}_LL0:;}
# 1309
static enum Cyc_Tcpat_Answer Cyc_Tcpat_static_match(struct Cyc_Tcpat_Con_s*c,void*td){
void*_tmp2C9=td;struct Cyc_Set_Set*_tmp2CA;struct Cyc_Tcpat_Con_s*_tmp2CB;if(((struct Cyc_Tcpat_Pos_Tcpat_Term_desc_struct*)_tmp2C9)->tag == 0U){_LL1: _tmp2CB=((struct Cyc_Tcpat_Pos_Tcpat_Term_desc_struct*)_tmp2C9)->f1;_LL2: {struct Cyc_Tcpat_Con_s*c2=_tmp2CB;
# 1313
if(({Cyc_Tcpat_compare_con(c,c2);})== 0)return Cyc_Tcpat_Yes;else{
return Cyc_Tcpat_No;}}}else{_LL3: _tmp2CA=((struct Cyc_Tcpat_Neg_Tcpat_Term_desc_struct*)_tmp2C9)->f1;_LL4: {struct Cyc_Set_Set*cs=_tmp2CA;
# 1317
if(({({int(*_tmp594)(struct Cyc_Set_Set*s,struct Cyc_Tcpat_Con_s*elt)=({int(*_tmp2CC)(struct Cyc_Set_Set*s,struct Cyc_Tcpat_Con_s*elt)=(int(*)(struct Cyc_Set_Set*s,struct Cyc_Tcpat_Con_s*elt))Cyc_Set_member;_tmp2CC;});struct Cyc_Set_Set*_tmp593=cs;_tmp594(_tmp593,c);});}))return Cyc_Tcpat_No;else{
# 1320
if(c->span != 0 &&({int _tmp595=*((int*)_check_null(c->span));_tmp595 == ({Cyc_Set_cardinality(cs);})+ 1;}))
return Cyc_Tcpat_Yes;else{
# 1323
return Cyc_Tcpat_Maybe;}}}}_LL0:;}struct _tuple22{struct Cyc_Tcpat_Con_s*f1;struct Cyc_List_List*f2;};
# 1331
static struct Cyc_List_List*Cyc_Tcpat_augment(struct Cyc_List_List*ctxt,void*dsc){
struct Cyc_List_List*_tmp2CE=ctxt;struct Cyc_List_List*_tmp2D1;struct Cyc_List_List*_tmp2D0;struct Cyc_Tcpat_Con_s*_tmp2CF;if(_tmp2CE == 0){_LL1: _LL2:
 return 0;}else{_LL3: _tmp2CF=((struct _tuple22*)_tmp2CE->hd)->f1;_tmp2D0=((struct _tuple22*)_tmp2CE->hd)->f2;_tmp2D1=_tmp2CE->tl;_LL4: {struct Cyc_Tcpat_Con_s*c=_tmp2CF;struct Cyc_List_List*args=_tmp2D0;struct Cyc_List_List*rest=_tmp2D1;
# 1335
return({struct Cyc_List_List*_tmp2D4=_cycalloc(sizeof(*_tmp2D4));
({struct _tuple22*_tmp597=({struct _tuple22*_tmp2D3=_cycalloc(sizeof(*_tmp2D3));_tmp2D3->f1=c,({struct Cyc_List_List*_tmp596=({struct Cyc_List_List*_tmp2D2=_cycalloc(sizeof(*_tmp2D2));_tmp2D2->hd=dsc,_tmp2D2->tl=args;_tmp2D2;});_tmp2D3->f2=_tmp596;});_tmp2D3;});_tmp2D4->hd=_tmp597;}),_tmp2D4->tl=rest;_tmp2D4;});}}_LL0:;}
# 1343
static struct Cyc_List_List*Cyc_Tcpat_norm_context(struct Cyc_List_List*ctxt){
struct Cyc_List_List*_tmp2D6=ctxt;struct Cyc_List_List*_tmp2D9;struct Cyc_List_List*_tmp2D8;struct Cyc_Tcpat_Con_s*_tmp2D7;if(_tmp2D6 == 0){_LL1: _LL2:
({void*_tmp2DA=0U;({int(*_tmp599)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp2DC)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp2DC;});struct _fat_ptr _tmp598=({const char*_tmp2DB="norm_context: empty context";_tag_fat(_tmp2DB,sizeof(char),28U);});_tmp599(_tmp598,_tag_fat(_tmp2DA,sizeof(void*),0U));});});}else{_LL3: _tmp2D7=((struct _tuple22*)_tmp2D6->hd)->f1;_tmp2D8=((struct _tuple22*)_tmp2D6->hd)->f2;_tmp2D9=_tmp2D6->tl;_LL4: {struct Cyc_Tcpat_Con_s*c=_tmp2D7;struct Cyc_List_List*args=_tmp2D8;struct Cyc_List_List*rest=_tmp2D9;
# 1347
return({struct Cyc_List_List*(*_tmp2DD)(struct Cyc_List_List*ctxt,void*dsc)=Cyc_Tcpat_augment;struct Cyc_List_List*_tmp2DE=rest;void*_tmp2DF=(void*)({struct Cyc_Tcpat_Pos_Tcpat_Term_desc_struct*_tmp2E0=_cycalloc(sizeof(*_tmp2E0));_tmp2E0->tag=0U,_tmp2E0->f1=c,({struct Cyc_List_List*_tmp59A=({Cyc_List_rev(args);});_tmp2E0->f2=_tmp59A;});_tmp2E0;});_tmp2DD(_tmp2DE,_tmp2DF);});}}_LL0:;}
# 1356
static void*Cyc_Tcpat_build_desc(struct Cyc_List_List*ctxt,void*dsc,struct Cyc_List_List*work){
# 1358
struct _tuple1 _stmttmp26=({struct _tuple1 _tmp43E;_tmp43E.f1=ctxt,_tmp43E.f2=work;_tmp43E;});struct _tuple1 _tmp2E2=_stmttmp26;struct Cyc_List_List*_tmp2E7;struct Cyc_List_List*_tmp2E6;struct Cyc_List_List*_tmp2E5;struct Cyc_List_List*_tmp2E4;struct Cyc_Tcpat_Con_s*_tmp2E3;if(_tmp2E2.f1 == 0){if(_tmp2E2.f2 == 0){_LL1: _LL2:
 return dsc;}else{_LL3: _LL4:
 goto _LL6;}}else{if(_tmp2E2.f2 == 0){_LL5: _LL6:
({void*_tmp2E8=0U;({int(*_tmp59C)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp2EA)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp2EA;});struct _fat_ptr _tmp59B=({const char*_tmp2E9="build_desc: ctxt and work don't match";_tag_fat(_tmp2E9,sizeof(char),38U);});_tmp59C(_tmp59B,_tag_fat(_tmp2E8,sizeof(void*),0U));});});}else{_LL7: _tmp2E3=((struct _tuple22*)(_tmp2E2.f1)->hd)->f1;_tmp2E4=((struct _tuple22*)(_tmp2E2.f1)->hd)->f2;_tmp2E5=(_tmp2E2.f1)->tl;_tmp2E6=((struct _tuple21*)(_tmp2E2.f2)->hd)->f3;_tmp2E7=(_tmp2E2.f2)->tl;_LL8: {struct Cyc_Tcpat_Con_s*c=_tmp2E3;struct Cyc_List_List*args=_tmp2E4;struct Cyc_List_List*rest=_tmp2E5;struct Cyc_List_List*dargs=_tmp2E6;struct Cyc_List_List*work2=_tmp2E7;
# 1363
struct Cyc_Tcpat_Pos_Tcpat_Term_desc_struct*td=({struct Cyc_Tcpat_Pos_Tcpat_Term_desc_struct*_tmp2EF=_cycalloc(sizeof(*_tmp2EF));_tmp2EF->tag=0U,_tmp2EF->f1=c,({struct Cyc_List_List*_tmp59D=({struct Cyc_List_List*(*_tmp2EB)(struct Cyc_List_List*x,struct Cyc_List_List*y)=Cyc_List_append;struct Cyc_List_List*_tmp2EC=({Cyc_List_rev(args);});struct Cyc_List_List*_tmp2ED=({struct Cyc_List_List*_tmp2EE=_cycalloc(sizeof(*_tmp2EE));_tmp2EE->hd=dsc,_tmp2EE->tl=dargs;_tmp2EE;});_tmp2EB(_tmp2EC,_tmp2ED);});_tmp2EF->f2=_tmp59D;});_tmp2EF;});
return({Cyc_Tcpat_build_desc(rest,(void*)td,work2);});}}}_LL0:;}
# 1356
static void*Cyc_Tcpat_match(void*p,struct Cyc_List_List*obj,void*dsc,struct Cyc_List_List*ctx,struct Cyc_List_List*work,struct Cyc_Tcpat_Rhs*right_hand_side,struct Cyc_List_List*rules);struct _tuple23{void*f1;struct Cyc_Tcpat_Rhs*f2;};
# 1375
static void*Cyc_Tcpat_or_match(void*dsc,struct Cyc_List_List*allmrules){
struct Cyc_List_List*_tmp2F1=allmrules;struct Cyc_List_List*_tmp2F4;struct Cyc_Tcpat_Rhs*_tmp2F3;void*_tmp2F2;if(_tmp2F1 == 0){_LL1: _LL2:
 return(void*)({struct Cyc_Tcpat_Failure_Tcpat_Decision_struct*_tmp2F5=_cycalloc(sizeof(*_tmp2F5));_tmp2F5->tag=0U,_tmp2F5->f1=dsc;_tmp2F5;});}else{_LL3: _tmp2F2=((struct _tuple23*)_tmp2F1->hd)->f1;_tmp2F3=((struct _tuple23*)_tmp2F1->hd)->f2;_tmp2F4=_tmp2F1->tl;_LL4: {void*pat1=_tmp2F2;struct Cyc_Tcpat_Rhs*rhs1=_tmp2F3;struct Cyc_List_List*rulerest=_tmp2F4;
# 1379
return({Cyc_Tcpat_match(pat1,0,dsc,0,0,rhs1,rulerest);});}}_LL0:;}
# 1384
static void*Cyc_Tcpat_match_compile(struct Cyc_List_List*allmrules){
return({void*(*_tmp2F7)(void*dsc,struct Cyc_List_List*allmrules)=Cyc_Tcpat_or_match;void*_tmp2F8=(void*)({struct Cyc_Tcpat_Neg_Tcpat_Term_desc_struct*_tmp2F9=_cycalloc(sizeof(*_tmp2F9));_tmp2F9->tag=1U,({struct Cyc_Set_Set*_tmp59E=({Cyc_Tcpat_empty_con_set();});_tmp2F9->f1=_tmp59E;});_tmp2F9;});struct Cyc_List_List*_tmp2FA=allmrules;_tmp2F7(_tmp2F8,_tmp2FA);});}
# 1391
static void*Cyc_Tcpat_and_match(struct Cyc_List_List*ctx,struct Cyc_List_List*work,struct Cyc_Tcpat_Rhs*right_hand_side,struct Cyc_List_List*rules){
# 1394
struct Cyc_List_List*_tmp2FC=work;struct Cyc_List_List*_tmp300;struct Cyc_List_List*_tmp2FF;struct Cyc_List_List*_tmp2FE;struct Cyc_List_List*_tmp2FD;struct Cyc_List_List*_tmp301;if(_tmp2FC == 0){_LL1: _LL2:
 return(void*)({struct Cyc_Tcpat_Success_Tcpat_Decision_struct*_tmp302=_cycalloc(sizeof(*_tmp302));_tmp302->tag=1U,_tmp302->f1=right_hand_side;_tmp302;});}else{if(((struct _tuple21*)((struct Cyc_List_List*)_tmp2FC)->hd)->f1 == 0){if(((struct _tuple21*)((struct Cyc_List_List*)_tmp2FC)->hd)->f2 == 0){if(((struct _tuple21*)((struct Cyc_List_List*)_tmp2FC)->hd)->f3 == 0){_LL3: _tmp301=_tmp2FC->tl;_LL4: {struct Cyc_List_List*workr=_tmp301;
# 1397
return({void*(*_tmp303)(struct Cyc_List_List*ctx,struct Cyc_List_List*work,struct Cyc_Tcpat_Rhs*right_hand_side,struct Cyc_List_List*rules)=Cyc_Tcpat_and_match;struct Cyc_List_List*_tmp304=({Cyc_Tcpat_norm_context(ctx);});struct Cyc_List_List*_tmp305=workr;struct Cyc_Tcpat_Rhs*_tmp306=right_hand_side;struct Cyc_List_List*_tmp307=rules;_tmp303(_tmp304,_tmp305,_tmp306,_tmp307);});}}else{goto _LL5;}}else{goto _LL5;}}else{_LL5: _tmp2FD=((struct _tuple21*)_tmp2FC->hd)->f1;_tmp2FE=((struct _tuple21*)_tmp2FC->hd)->f2;_tmp2FF=((struct _tuple21*)_tmp2FC->hd)->f3;_tmp300=_tmp2FC->tl;_LL6: {struct Cyc_List_List*pats=_tmp2FD;struct Cyc_List_List*objs=_tmp2FE;struct Cyc_List_List*dscs=_tmp2FF;struct Cyc_List_List*workr=_tmp300;
# 1399
if((pats == 0 || objs == 0)|| dscs == 0)
({void*_tmp308=0U;({int(*_tmp5A0)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp30A)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp30A;});struct _fat_ptr _tmp59F=({const char*_tmp309="tcpat:and_match: malformed work frame";_tag_fat(_tmp309,sizeof(char),38U);});_tmp5A0(_tmp59F,_tag_fat(_tmp308,sizeof(void*),0U));});});{
# 1399
struct Cyc_List_List*_stmttmp27=pats;struct Cyc_List_List*_tmp30C;void*_tmp30B;_LL8: _tmp30B=(void*)_stmttmp27->hd;_tmp30C=_stmttmp27->tl;_LL9: {void*pat1=_tmp30B;struct Cyc_List_List*patr=_tmp30C;
# 1402
struct Cyc_List_List*_stmttmp28=objs;struct Cyc_List_List*_tmp30E;struct Cyc_List_List*_tmp30D;_LLB: _tmp30D=(struct Cyc_List_List*)_stmttmp28->hd;_tmp30E=_stmttmp28->tl;_LLC: {struct Cyc_List_List*obj1=_tmp30D;struct Cyc_List_List*objr=_tmp30E;
struct Cyc_List_List*_stmttmp29=dscs;struct Cyc_List_List*_tmp310;void*_tmp30F;_LLE: _tmp30F=(void*)_stmttmp29->hd;_tmp310=_stmttmp29->tl;_LLF: {void*dsc1=_tmp30F;struct Cyc_List_List*dscr=_tmp310;
struct _tuple21*wf=({struct _tuple21*_tmp312=_cycalloc(sizeof(*_tmp312));_tmp312->f1=patr,_tmp312->f2=objr,_tmp312->f3=dscr;_tmp312;});
return({({void*_tmp5A6=pat1;struct Cyc_List_List*_tmp5A5=obj1;void*_tmp5A4=dsc1;struct Cyc_List_List*_tmp5A3=ctx;struct Cyc_List_List*_tmp5A2=({struct Cyc_List_List*_tmp311=_cycalloc(sizeof(*_tmp311));_tmp311->hd=wf,_tmp311->tl=workr;_tmp311;});struct Cyc_Tcpat_Rhs*_tmp5A1=right_hand_side;Cyc_Tcpat_match(_tmp5A6,_tmp5A5,_tmp5A4,_tmp5A3,_tmp5A2,_tmp5A1,rules);});});}}}}}}}_LL0:;}
# 1410
static struct Cyc_List_List*Cyc_Tcpat_getdargs(struct Cyc_Tcpat_Con_s*pcon,void*dsc){
void*_tmp314=dsc;struct Cyc_List_List*_tmp315;struct Cyc_Set_Set*_tmp316;if(((struct Cyc_Tcpat_Neg_Tcpat_Term_desc_struct*)_tmp314)->tag == 1U){_LL1: _tmp316=((struct Cyc_Tcpat_Neg_Tcpat_Term_desc_struct*)_tmp314)->f1;_LL2: {struct Cyc_Set_Set*ncs=_tmp316;
# 1416
void*any=(void*)({struct Cyc_Tcpat_Neg_Tcpat_Term_desc_struct*_tmp318=_cycalloc(sizeof(*_tmp318));_tmp318->tag=1U,({struct Cyc_Set_Set*_tmp5A7=({Cyc_Tcpat_empty_con_set();});_tmp318->f1=_tmp5A7;});_tmp318;});
struct Cyc_List_List*res=0;
{int i=0;for(0;i < pcon->arity;++ i){
res=({struct Cyc_List_List*_tmp317=_cycalloc(sizeof(*_tmp317));_tmp317->hd=any,_tmp317->tl=res;_tmp317;});}}
return res;}}else{_LL3: _tmp315=((struct Cyc_Tcpat_Pos_Tcpat_Term_desc_struct*)_tmp314)->f2;_LL4: {struct Cyc_List_List*dargs=_tmp315;
return dargs;}}_LL0:;}
# 1425
static void*Cyc_Tcpat_get_access(union Cyc_Tcpat_PatOrWhere pw,int i){
union Cyc_Tcpat_PatOrWhere _tmp31A=pw;struct Cyc_Absyn_Pat*_tmp31B;if((_tmp31A.where_clause).tag == 2){_LL1: _LL2:
 return(void*)& Cyc_Tcpat_Dummy_val;}else{_LL3: _tmp31B=(_tmp31A.pattern).val;_LL4: {struct Cyc_Absyn_Pat*p=_tmp31B;
# 1429
void*_stmttmp2A=p->r;void*_tmp31C=_stmttmp2A;struct Cyc_List_List*_tmp31E;union Cyc_Absyn_AggrInfo _tmp31D;struct Cyc_Absyn_Datatypefield*_tmp320;struct Cyc_Absyn_Datatypedecl*_tmp31F;switch(*((int*)_tmp31C)){case 6U: _LL6: _LL7:
# 1431
 if(i != 0)
({void*_tmp321=0U;({int(*_tmp5AA)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp325)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp325;});struct _fat_ptr _tmp5A9=(struct _fat_ptr)({struct Cyc_Int_pa_PrintArg_struct _tmp324=({struct Cyc_Int_pa_PrintArg_struct _tmp43F;_tmp43F.tag=1U,_tmp43F.f1=(unsigned long)i;_tmp43F;});void*_tmp322[1U];_tmp322[0]=& _tmp324;({struct _fat_ptr _tmp5A8=({const char*_tmp323="get_access on pointer pattern with offset %d\n";_tag_fat(_tmp323,sizeof(char),46U);});Cyc_aprintf(_tmp5A8,_tag_fat(_tmp322,sizeof(void*),1U));});});_tmp5AA(_tmp5A9,_tag_fat(_tmp321,sizeof(void*),0U));});});
# 1431
return(void*)& Cyc_Tcpat_Deref_val;case 5U: _LL8: _LL9:
# 1434
 return(void*)({struct Cyc_Tcpat_TupleField_Tcpat_Access_struct*_tmp326=_cycalloc(sizeof(*_tmp326));_tmp326->tag=2U,_tmp326->f1=(unsigned)i;_tmp326;});case 8U: _LLA: _tmp31F=((struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct*)_tmp31C)->f1;_tmp320=((struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct*)_tmp31C)->f2;_LLB: {struct Cyc_Absyn_Datatypedecl*tud=_tmp31F;struct Cyc_Absyn_Datatypefield*tuf=_tmp320;
return(void*)({struct Cyc_Tcpat_DatatypeField_Tcpat_Access_struct*_tmp327=_cycalloc(sizeof(*_tmp327));_tmp327->tag=3U,_tmp327->f1=tud,_tmp327->f2=tuf,_tmp327->f3=(unsigned)i;_tmp327;});}case 7U: if(((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp31C)->f1 != 0){_LLC: _tmp31D=*((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp31C)->f1;_tmp31E=((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp31C)->f3;_LLD: {union Cyc_Absyn_AggrInfo info=_tmp31D;struct Cyc_List_List*dlps=_tmp31E;
# 1437
struct Cyc_Absyn_Aggrdecl*ad=({Cyc_Absyn_get_known_aggrdecl(info);});
if(((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->tagged){
struct Cyc_List_List*_stmttmp2B=(*((struct _tuple19*)((struct Cyc_List_List*)_check_null(dlps))->hd)).f1;struct Cyc_List_List*_tmp328=_stmttmp2B;struct _fat_ptr*_tmp329;if(_tmp328 != 0){if(((struct Cyc_Absyn_FieldName_Absyn_Designator_struct*)((struct Cyc_List_List*)_tmp328)->hd)->tag == 1U){if(((struct Cyc_List_List*)_tmp328)->tl == 0){_LL11: _tmp329=((struct Cyc_Absyn_FieldName_Absyn_Designator_struct*)_tmp328->hd)->f1;_LL12: {struct _fat_ptr*f=_tmp329;
# 1441
return(void*)({struct Cyc_Tcpat_AggrField_Tcpat_Access_struct*_tmp32A=_cycalloc(sizeof(*_tmp32A));_tmp32A->tag=4U,_tmp32A->f1=((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->tagged,_tmp32A->f2=f;_tmp32A;});}}else{goto _LL13;}}else{goto _LL13;}}else{_LL13: _LL14:
({void*_tmp32B=0U;({int(*_tmp5AE)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp32F)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp32F;});struct _fat_ptr _tmp5AD=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp32E=({struct Cyc_String_pa_PrintArg_struct _tmp440;_tmp440.tag=0U,({struct _fat_ptr _tmp5AB=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_pat2string(p);}));_tmp440.f1=_tmp5AB;});_tmp440;});void*_tmp32C[1U];_tmp32C[0]=& _tmp32E;({struct _fat_ptr _tmp5AC=({const char*_tmp32D="get_access on bad aggr pattern: %s";_tag_fat(_tmp32D,sizeof(char),35U);});Cyc_aprintf(_tmp5AC,_tag_fat(_tmp32C,sizeof(void*),1U));});});_tmp5AE(_tmp5AD,_tag_fat(_tmp32B,sizeof(void*),0U));});});}_LL10:;}{
# 1438
struct Cyc_List_List*fields=((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->fields;
# 1446
int orig_i=i;
for(0;i != 0;-- i){fields=((struct Cyc_List_List*)_check_null(fields))->tl;}
return(void*)({struct Cyc_Tcpat_AggrField_Tcpat_Access_struct*_tmp330=_cycalloc(sizeof(*_tmp330));_tmp330->tag=4U,_tmp330->f1=((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->tagged,_tmp330->f2=((struct Cyc_Absyn_Aggrfield*)((struct Cyc_List_List*)_check_null(fields))->hd)->name;_tmp330;});}}}else{goto _LLE;}default: _LLE: _LLF:
({void*_tmp331=0U;({int(*_tmp5B2)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp335)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp335;});struct _fat_ptr _tmp5B1=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp334=({struct Cyc_String_pa_PrintArg_struct _tmp441;_tmp441.tag=0U,({struct _fat_ptr _tmp5AF=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_pat2string(p);}));_tmp441.f1=_tmp5AF;});_tmp441;});void*_tmp332[1U];_tmp332[0]=& _tmp334;({struct _fat_ptr _tmp5B0=({const char*_tmp333="get_access on bad pattern: %s";_tag_fat(_tmp333,sizeof(char),30U);});Cyc_aprintf(_tmp5B0,_tag_fat(_tmp332,sizeof(void*),1U));});});_tmp5B2(_tmp5B1,_tag_fat(_tmp331,sizeof(void*),0U));});});}_LL5:;}}_LL0:;}struct _tuple24{struct Cyc_List_List*f1;struct Cyc_Tcpat_Con_s*f2;};
# 1455
static struct Cyc_List_List*Cyc_Tcpat_getoarg(struct _tuple24*env,int i){
struct Cyc_Tcpat_Con_s*_tmp338;struct Cyc_List_List*_tmp337;_LL1: _tmp337=env->f1;_tmp338=env->f2;_LL2: {struct Cyc_List_List*obj=_tmp337;struct Cyc_Tcpat_Con_s*pcon=_tmp338;
void*acc=({Cyc_Tcpat_get_access(pcon->orig_pat,i);});
return({struct Cyc_List_List*_tmp33A=_cycalloc(sizeof(*_tmp33A));({struct Cyc_Tcpat_PathNode*_tmp5B3=({struct Cyc_Tcpat_PathNode*_tmp339=_cycalloc(sizeof(*_tmp339));_tmp339->orig_pat=pcon->orig_pat,_tmp339->access=acc;_tmp339;});_tmp33A->hd=_tmp5B3;}),_tmp33A->tl=obj;_tmp33A;});}}
# 1460
static struct Cyc_List_List*Cyc_Tcpat_getoargs(struct Cyc_Tcpat_Con_s*pcon,struct Cyc_List_List*obj){
struct _tuple24 env=({struct _tuple24 _tmp442;_tmp442.f1=obj,_tmp442.f2=pcon;_tmp442;});
return({({struct Cyc_List_List*(*_tmp5B4)(int n,struct Cyc_List_List*(*f)(struct _tuple24*,int),struct _tuple24*env)=({struct Cyc_List_List*(*_tmp33C)(int n,struct Cyc_List_List*(*f)(struct _tuple24*,int),struct _tuple24*env)=(struct Cyc_List_List*(*)(int n,struct Cyc_List_List*(*f)(struct _tuple24*,int),struct _tuple24*env))Cyc_List_tabulate_c;_tmp33C;});_tmp5B4(pcon->arity,Cyc_Tcpat_getoarg,& env);});});}
# 1468
static void*Cyc_Tcpat_match(void*p,struct Cyc_List_List*obj,void*dsc,struct Cyc_List_List*ctx,struct Cyc_List_List*work,struct Cyc_Tcpat_Rhs*right_hand_side,struct Cyc_List_List*rules){
# 1472
void*_tmp33E=p;struct Cyc_List_List*_tmp340;struct Cyc_Tcpat_Con_s*_tmp33F;if(((struct Cyc_Tcpat_Any_Tcpat_Simple_pat_struct*)_tmp33E)->tag == 0U){_LL1: _LL2:
# 1474
 return({void*(*_tmp341)(struct Cyc_List_List*ctx,struct Cyc_List_List*work,struct Cyc_Tcpat_Rhs*right_hand_side,struct Cyc_List_List*rules)=Cyc_Tcpat_and_match;struct Cyc_List_List*_tmp342=({Cyc_Tcpat_augment(ctx,dsc);});struct Cyc_List_List*_tmp343=work;struct Cyc_Tcpat_Rhs*_tmp344=right_hand_side;struct Cyc_List_List*_tmp345=rules;_tmp341(_tmp342,_tmp343,_tmp344,_tmp345);});}else{_LL3: _tmp33F=((struct Cyc_Tcpat_Con_Tcpat_Simple_pat_struct*)_tmp33E)->f1;_tmp340=((struct Cyc_Tcpat_Con_Tcpat_Simple_pat_struct*)_tmp33E)->f2;_LL4: {struct Cyc_Tcpat_Con_s*pcon=_tmp33F;struct Cyc_List_List*pargs=_tmp340;
# 1476
enum Cyc_Tcpat_Answer _stmttmp2C=({Cyc_Tcpat_static_match(pcon,dsc);});enum Cyc_Tcpat_Answer _tmp346=_stmttmp2C;switch(_tmp346){case Cyc_Tcpat_Yes: _LL6: _LL7: {
# 1478
struct Cyc_List_List*ctx2=({struct Cyc_List_List*_tmp34A=_cycalloc(sizeof(*_tmp34A));({struct _tuple22*_tmp5B5=({struct _tuple22*_tmp349=_cycalloc(sizeof(*_tmp349));_tmp349->f1=pcon,_tmp349->f2=0;_tmp349;});_tmp34A->hd=_tmp5B5;}),_tmp34A->tl=ctx;_tmp34A;});
struct _tuple21*work_frame=({struct _tuple21*_tmp348=_cycalloc(sizeof(*_tmp348));_tmp348->f1=pargs,({struct Cyc_List_List*_tmp5B7=({Cyc_Tcpat_getoargs(pcon,obj);});_tmp348->f2=_tmp5B7;}),({
struct Cyc_List_List*_tmp5B6=({Cyc_Tcpat_getdargs(pcon,dsc);});_tmp348->f3=_tmp5B6;});_tmp348;});
struct Cyc_List_List*work2=({struct Cyc_List_List*_tmp347=_cycalloc(sizeof(*_tmp347));_tmp347->hd=work_frame,_tmp347->tl=work;_tmp347;});
return({Cyc_Tcpat_and_match(ctx2,work2,right_hand_side,rules);});}case Cyc_Tcpat_No: _LL8: _LL9:
# 1484
 return({void*(*_tmp34B)(void*dsc,struct Cyc_List_List*allmrules)=Cyc_Tcpat_or_match;void*_tmp34C=({Cyc_Tcpat_build_desc(ctx,dsc,work);});struct Cyc_List_List*_tmp34D=rules;_tmp34B(_tmp34C,_tmp34D);});default: _LLA: _LLB: {
# 1486
struct Cyc_List_List*ctx2=({struct Cyc_List_List*_tmp358=_cycalloc(sizeof(*_tmp358));({struct _tuple22*_tmp5B8=({struct _tuple22*_tmp357=_cycalloc(sizeof(*_tmp357));_tmp357->f1=pcon,_tmp357->f2=0;_tmp357;});_tmp358->hd=_tmp5B8;}),_tmp358->tl=ctx;_tmp358;});
struct _tuple21*work_frame=({struct _tuple21*_tmp356=_cycalloc(sizeof(*_tmp356));_tmp356->f1=pargs,({struct Cyc_List_List*_tmp5BA=({Cyc_Tcpat_getoargs(pcon,obj);});_tmp356->f2=_tmp5BA;}),({
struct Cyc_List_List*_tmp5B9=({Cyc_Tcpat_getdargs(pcon,dsc);});_tmp356->f3=_tmp5B9;});_tmp356;});
struct Cyc_List_List*work2=({struct Cyc_List_List*_tmp355=_cycalloc(sizeof(*_tmp355));_tmp355->hd=work_frame,_tmp355->tl=work;_tmp355;});
void*s=({Cyc_Tcpat_and_match(ctx2,work2,right_hand_side,rules);});
void*f=({void*(*_tmp34E)(void*dsc,struct Cyc_List_List*allmrules)=Cyc_Tcpat_or_match;void*_tmp34F=({void*(*_tmp350)(struct Cyc_List_List*ctxt,void*dsc,struct Cyc_List_List*work)=Cyc_Tcpat_build_desc;struct Cyc_List_List*_tmp351=ctx;void*_tmp352=({Cyc_Tcpat_add_neg(dsc,pcon);});struct Cyc_List_List*_tmp353=work;_tmp350(_tmp351,_tmp352,_tmp353);});struct Cyc_List_List*_tmp354=rules;_tmp34E(_tmp34F,_tmp354);});
# 1493
return({Cyc_Tcpat_ifeq(obj,pcon,s,f);});}}_LL5:;}}_LL0:;}
# 1503
static void Cyc_Tcpat_check_exhaust_overlap(void*d,void(*not_exhaust)(void*,void*),void*env1,void(*rhs_appears)(void*,struct Cyc_Tcpat_Rhs*),void*env2,int*exhaust_done){
# 1508
void*_tmp35A=d;void*_tmp35C;struct Cyc_List_List*_tmp35B;struct Cyc_Tcpat_Rhs*_tmp35D;void*_tmp35E;switch(*((int*)_tmp35A)){case 0U: _LL1: _tmp35E=(void*)((struct Cyc_Tcpat_Failure_Tcpat_Decision_struct*)_tmp35A)->f1;_LL2: {void*td=_tmp35E;
# 1510
if(!(*exhaust_done)){
({not_exhaust(env1,td);});*exhaust_done=1;}
# 1510
goto _LL0;}case 1U: _LL3: _tmp35D=((struct Cyc_Tcpat_Success_Tcpat_Decision_struct*)_tmp35A)->f1;_LL4: {struct Cyc_Tcpat_Rhs*r=_tmp35D;
# 1513
({rhs_appears(env2,r);});goto _LL0;}default: _LL5: _tmp35B=((struct Cyc_Tcpat_SwitchDec_Tcpat_Decision_struct*)_tmp35A)->f2;_tmp35C=(void*)((struct Cyc_Tcpat_SwitchDec_Tcpat_Decision_struct*)_tmp35A)->f3;_LL6: {struct Cyc_List_List*cases=_tmp35B;void*def=_tmp35C;
# 1515
for(0;cases != 0;cases=cases->tl){
struct _tuple0 _stmttmp2D=*((struct _tuple0*)cases->hd);void*_tmp35F;_LL8: _tmp35F=_stmttmp2D.f2;_LL9: {void*d=_tmp35F;
({Cyc_Tcpat_check_exhaust_overlap(d,not_exhaust,env1,rhs_appears,env2,exhaust_done);});}}
# 1521
({Cyc_Tcpat_check_exhaust_overlap(def,not_exhaust,env1,rhs_appears,env2,exhaust_done);});
# 1523
goto _LL0;}}_LL0:;}
# 1533
static struct _tuple23*Cyc_Tcpat_get_match(int*ctr,struct Cyc_Absyn_Switch_clause*swc){
# 1535
void*sp0=({Cyc_Tcpat_compile_pat(swc->pattern);});
struct Cyc_Tcpat_Rhs*rhs=({struct Cyc_Tcpat_Rhs*_tmp36B=_cycalloc(sizeof(*_tmp36B));_tmp36B->used=0,_tmp36B->pat_loc=(swc->pattern)->loc,_tmp36B->rhs=swc->body;_tmp36B;});
void*sp;
if(swc->where_clause != 0){
union Cyc_Tcpat_PatOrWhere w=({union Cyc_Tcpat_PatOrWhere _tmp443;(_tmp443.where_clause).tag=2U,(_tmp443.where_clause).val=swc->where_clause;_tmp443;});
sp=({void*(*_tmp361)(struct Cyc_List_List*ss,union Cyc_Tcpat_PatOrWhere p)=Cyc_Tcpat_tuple_pat;struct Cyc_List_List*_tmp362=({void*_tmp363[2U];_tmp363[0]=sp0,({void*_tmp5BB=({Cyc_Tcpat_int_pat(*ctr,w);});_tmp363[1]=_tmp5BB;});Cyc_List_list(_tag_fat(_tmp363,sizeof(void*),2U));});union Cyc_Tcpat_PatOrWhere _tmp364=w;_tmp361(_tmp362,_tmp364);});
*ctr=*ctr + 1;}else{
# 1543
sp=({void*(*_tmp365)(struct Cyc_List_List*ss,union Cyc_Tcpat_PatOrWhere p)=Cyc_Tcpat_tuple_pat;struct Cyc_List_List*_tmp366=({void*_tmp367[2U];_tmp367[0]=sp0,({void*_tmp5BC=(void*)({struct Cyc_Tcpat_Any_Tcpat_Simple_pat_struct*_tmp368=_cycalloc(sizeof(*_tmp368));_tmp368->tag=0U;_tmp368;});_tmp367[1]=_tmp5BC;});Cyc_List_list(_tag_fat(_tmp367,sizeof(void*),2U));});union Cyc_Tcpat_PatOrWhere _tmp369=({union Cyc_Tcpat_PatOrWhere _tmp444;(_tmp444.where_clause).tag=2U,(_tmp444.where_clause).val=0;_tmp444;});_tmp365(_tmp366,_tmp369);});}
return({struct _tuple23*_tmp36A=_cycalloc(sizeof(*_tmp36A));_tmp36A->f1=sp,_tmp36A->f2=rhs;_tmp36A;});}char Cyc_Tcpat_Desc2string[12U]="Desc2string";struct Cyc_Tcpat_Desc2string_exn_struct{char*tag;};
# 1549
struct Cyc_Tcpat_Desc2string_exn_struct Cyc_Tcpat_Desc2string_val={Cyc_Tcpat_Desc2string};
# 1551
static struct _fat_ptr Cyc_Tcpat_descs2string(struct Cyc_List_List*);
static struct _fat_ptr Cyc_Tcpat_desc2string(void*d){
void*_tmp36D=d;struct Cyc_Set_Set*_tmp36E;struct Cyc_List_List*_tmp370;struct Cyc_Tcpat_Con_s*_tmp36F;if(((struct Cyc_Tcpat_Pos_Tcpat_Term_desc_struct*)_tmp36D)->tag == 0U){_LL1: _tmp36F=((struct Cyc_Tcpat_Pos_Tcpat_Term_desc_struct*)_tmp36D)->f1;_tmp370=((struct Cyc_Tcpat_Pos_Tcpat_Term_desc_struct*)_tmp36D)->f2;_LL2: {struct Cyc_Tcpat_Con_s*c=_tmp36F;struct Cyc_List_List*ds=_tmp370;
# 1555
union Cyc_Tcpat_Name_value n=c->name;
struct Cyc_Absyn_Pat*p;
{union Cyc_Tcpat_PatOrWhere _stmttmp2E=c->orig_pat;union Cyc_Tcpat_PatOrWhere _tmp371=_stmttmp2E;struct Cyc_Absyn_Pat*_tmp372;if((_tmp371.where_clause).tag == 2){_LL6: _LL7:
 return({Cyc_Tcpat_desc2string((void*)((struct Cyc_List_List*)_check_null(ds))->hd);});}else{_LL8: _tmp372=(_tmp371.pattern).val;_LL9: {struct Cyc_Absyn_Pat*p2=_tmp372;
p=p2;goto _LL5;}}_LL5:;}{
# 1561
void*_stmttmp2F=p->r;void*_tmp373=_stmttmp2F;struct Cyc_Absyn_Exp*_tmp374;struct Cyc_Absyn_Enumfield*_tmp375;struct Cyc_Absyn_Enumfield*_tmp376;int _tmp378;struct _fat_ptr _tmp377;char _tmp379;int _tmp37A;struct Cyc_Absyn_Datatypefield*_tmp37B;struct Cyc_List_List*_tmp37D;struct Cyc_Absyn_Aggrdecl*_tmp37C;struct Cyc_Absyn_Vardecl*_tmp37F;struct Cyc_Absyn_Tvar*_tmp37E;struct Cyc_Absyn_Vardecl*_tmp380;struct Cyc_Absyn_Vardecl*_tmp381;switch(*((int*)_tmp373)){case 0U: _LLB: _LLC:
 return({const char*_tmp382="_";_tag_fat(_tmp382,sizeof(char),2U);});case 1U: _LLD: _tmp381=((struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct*)_tmp373)->f1;_LLE: {struct Cyc_Absyn_Vardecl*vd=_tmp381;
return({Cyc_Absynpp_qvar2string(vd->name);});}case 3U: _LLF: _tmp380=((struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct*)_tmp373)->f1;_LL10: {struct Cyc_Absyn_Vardecl*vd=_tmp380;
return(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp385=({struct Cyc_String_pa_PrintArg_struct _tmp445;_tmp445.tag=0U,({struct _fat_ptr _tmp5BD=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(vd->name);}));_tmp445.f1=_tmp5BD;});_tmp445;});void*_tmp383[1U];_tmp383[0]=& _tmp385;({struct _fat_ptr _tmp5BE=({const char*_tmp384="*%s";_tag_fat(_tmp384,sizeof(char),4U);});Cyc_aprintf(_tmp5BE,_tag_fat(_tmp383,sizeof(void*),1U));});});}case 4U: _LL11: _tmp37E=((struct Cyc_Absyn_TagInt_p_Absyn_Raw_pat_struct*)_tmp373)->f1;_tmp37F=((struct Cyc_Absyn_TagInt_p_Absyn_Raw_pat_struct*)_tmp373)->f2;_LL12: {struct Cyc_Absyn_Tvar*tv=_tmp37E;struct Cyc_Absyn_Vardecl*vd=_tmp37F;
return(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp388=({struct Cyc_String_pa_PrintArg_struct _tmp447;_tmp447.tag=0U,({struct _fat_ptr _tmp5BF=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(vd->name);}));_tmp447.f1=_tmp5BF;});_tmp447;});struct Cyc_String_pa_PrintArg_struct _tmp389=({struct Cyc_String_pa_PrintArg_struct _tmp446;_tmp446.tag=0U,_tmp446.f1=(struct _fat_ptr)((struct _fat_ptr)*tv->name);_tmp446;});void*_tmp386[2U];_tmp386[0]=& _tmp388,_tmp386[1]=& _tmp389;({struct _fat_ptr _tmp5C0=({const char*_tmp387="%s<`%s>";_tag_fat(_tmp387,sizeof(char),8U);});Cyc_aprintf(_tmp5C0,_tag_fat(_tmp386,sizeof(void*),2U));});});}case 5U: _LL13: _LL14:
# 1567
 return(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp38C=({struct Cyc_String_pa_PrintArg_struct _tmp448;_tmp448.tag=0U,({struct _fat_ptr _tmp5C1=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Tcpat_descs2string(ds);}));_tmp448.f1=_tmp5C1;});_tmp448;});void*_tmp38A[1U];_tmp38A[0]=& _tmp38C;({struct _fat_ptr _tmp5C2=({const char*_tmp38B="$(%s)";_tag_fat(_tmp38B,sizeof(char),6U);});Cyc_aprintf(_tmp5C2,_tag_fat(_tmp38A,sizeof(void*),1U));});});case 6U: _LL15: _LL16:
# 1569
 return(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp38F=({struct Cyc_String_pa_PrintArg_struct _tmp449;_tmp449.tag=0U,({struct _fat_ptr _tmp5C3=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Tcpat_descs2string(ds);}));_tmp449.f1=_tmp5C3;});_tmp449;});void*_tmp38D[1U];_tmp38D[0]=& _tmp38F;({struct _fat_ptr _tmp5C4=({const char*_tmp38E="&%s";_tag_fat(_tmp38E,sizeof(char),4U);});Cyc_aprintf(_tmp5C4,_tag_fat(_tmp38D,sizeof(void*),1U));});});case 7U: if(((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp373)->f1 != 0){if((((union Cyc_Absyn_AggrInfo*)((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp373)->f1)->KnownAggr).tag == 2){_LL17: _tmp37C=*((((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp373)->f1)->KnownAggr).val;_tmp37D=((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp373)->f3;_LL18: {struct Cyc_Absyn_Aggrdecl*ad=_tmp37C;struct Cyc_List_List*dlps=_tmp37D;
# 1571
if((int)ad->kind == (int)Cyc_Absyn_UnionA){
struct Cyc_List_List*_tmp390=dlps;struct _fat_ptr*_tmp391;if(_tmp390 != 0){if(((struct _tuple19*)((struct Cyc_List_List*)_tmp390)->hd)->f1 != 0){if(((struct Cyc_Absyn_FieldName_Absyn_Designator_struct*)((struct Cyc_List_List*)((struct _tuple19*)((struct Cyc_List_List*)_tmp390)->hd)->f1)->hd)->tag == 1U){_LL2C: _tmp391=((struct Cyc_Absyn_FieldName_Absyn_Designator_struct*)(((struct _tuple19*)_tmp390->hd)->f1)->hd)->f1;_LL2D: {struct _fat_ptr*f=_tmp391;
# 1574
return(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp394=({struct Cyc_String_pa_PrintArg_struct _tmp44C;_tmp44C.tag=0U,({struct _fat_ptr _tmp5C5=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(ad->name);}));_tmp44C.f1=_tmp5C5;});_tmp44C;});struct Cyc_String_pa_PrintArg_struct _tmp395=({struct Cyc_String_pa_PrintArg_struct _tmp44B;_tmp44B.tag=0U,_tmp44B.f1=(struct _fat_ptr)((struct _fat_ptr)*f);_tmp44B;});struct Cyc_String_pa_PrintArg_struct _tmp396=({struct Cyc_String_pa_PrintArg_struct _tmp44A;_tmp44A.tag=0U,({
struct _fat_ptr _tmp5C6=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Tcpat_descs2string(ds);}));_tmp44A.f1=_tmp5C6;});_tmp44A;});void*_tmp392[3U];_tmp392[0]=& _tmp394,_tmp392[1]=& _tmp395,_tmp392[2]=& _tmp396;({struct _fat_ptr _tmp5C7=({const char*_tmp393="%s{.%s = %s}";_tag_fat(_tmp393,sizeof(char),13U);});Cyc_aprintf(_tmp5C7,_tag_fat(_tmp392,sizeof(void*),3U));});});}}else{goto _LL2E;}}else{goto _LL2E;}}else{_LL2E: _LL2F:
 goto _LL2B;}_LL2B:;}
# 1571
return(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp399=({struct Cyc_String_pa_PrintArg_struct _tmp44E;_tmp44E.tag=0U,({
# 1579
struct _fat_ptr _tmp5C8=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(ad->name);}));_tmp44E.f1=_tmp5C8;});_tmp44E;});struct Cyc_String_pa_PrintArg_struct _tmp39A=({struct Cyc_String_pa_PrintArg_struct _tmp44D;_tmp44D.tag=0U,({struct _fat_ptr _tmp5C9=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Tcpat_descs2string(ds);}));_tmp44D.f1=_tmp5C9;});_tmp44D;});void*_tmp397[2U];_tmp397[0]=& _tmp399,_tmp397[1]=& _tmp39A;({struct _fat_ptr _tmp5CA=({const char*_tmp398="%s{%s}";_tag_fat(_tmp398,sizeof(char),7U);});Cyc_aprintf(_tmp5CA,_tag_fat(_tmp397,sizeof(void*),2U));});});}}else{goto _LL29;}}else{goto _LL29;}case 8U: _LL19: _tmp37B=((struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct*)_tmp373)->f2;_LL1A: {struct Cyc_Absyn_Datatypefield*tuf=_tmp37B;
# 1581
if(ds == 0)
return(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp39D=({struct Cyc_String_pa_PrintArg_struct _tmp44F;_tmp44F.tag=0U,({struct _fat_ptr _tmp5CB=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(tuf->name);}));_tmp44F.f1=_tmp5CB;});_tmp44F;});void*_tmp39B[1U];_tmp39B[0]=& _tmp39D;({struct _fat_ptr _tmp5CC=({const char*_tmp39C="%s";_tag_fat(_tmp39C,sizeof(char),3U);});Cyc_aprintf(_tmp5CC,_tag_fat(_tmp39B,sizeof(void*),1U));});});else{
# 1584
return(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp3A0=({struct Cyc_String_pa_PrintArg_struct _tmp451;_tmp451.tag=0U,({struct _fat_ptr _tmp5CD=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(tuf->name);}));_tmp451.f1=_tmp5CD;});_tmp451;});struct Cyc_String_pa_PrintArg_struct _tmp3A1=({struct Cyc_String_pa_PrintArg_struct _tmp450;_tmp450.tag=0U,({struct _fat_ptr _tmp5CE=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Tcpat_descs2string(ds);}));_tmp450.f1=_tmp5CE;});_tmp450;});void*_tmp39E[2U];_tmp39E[0]=& _tmp3A0,_tmp39E[1]=& _tmp3A1;({struct _fat_ptr _tmp5CF=({const char*_tmp39F="%s(%s)";_tag_fat(_tmp39F,sizeof(char),7U);});Cyc_aprintf(_tmp5CF,_tag_fat(_tmp39E,sizeof(void*),2U));});});}}case 9U: _LL1B: _LL1C:
 return({const char*_tmp3A2="NULL";_tag_fat(_tmp3A2,sizeof(char),5U);});case 10U: _LL1D: _tmp37A=((struct Cyc_Absyn_Int_p_Absyn_Raw_pat_struct*)_tmp373)->f2;_LL1E: {int i=_tmp37A;
return(struct _fat_ptr)({struct Cyc_Int_pa_PrintArg_struct _tmp3A5=({struct Cyc_Int_pa_PrintArg_struct _tmp452;_tmp452.tag=1U,_tmp452.f1=(unsigned long)i;_tmp452;});void*_tmp3A3[1U];_tmp3A3[0]=& _tmp3A5;({struct _fat_ptr _tmp5D0=({const char*_tmp3A4="%d";_tag_fat(_tmp3A4,sizeof(char),3U);});Cyc_aprintf(_tmp5D0,_tag_fat(_tmp3A3,sizeof(void*),1U));});});}case 11U: _LL1F: _tmp379=((struct Cyc_Absyn_Char_p_Absyn_Raw_pat_struct*)_tmp373)->f1;_LL20: {char c=_tmp379;
return(struct _fat_ptr)({struct Cyc_Int_pa_PrintArg_struct _tmp3A8=({struct Cyc_Int_pa_PrintArg_struct _tmp453;_tmp453.tag=1U,_tmp453.f1=(unsigned long)((int)c);_tmp453;});void*_tmp3A6[1U];_tmp3A6[0]=& _tmp3A8;({struct _fat_ptr _tmp5D1=({const char*_tmp3A7="%d";_tag_fat(_tmp3A7,sizeof(char),3U);});Cyc_aprintf(_tmp5D1,_tag_fat(_tmp3A6,sizeof(void*),1U));});});}case 12U: _LL21: _tmp377=((struct Cyc_Absyn_Float_p_Absyn_Raw_pat_struct*)_tmp373)->f1;_tmp378=((struct Cyc_Absyn_Float_p_Absyn_Raw_pat_struct*)_tmp373)->f2;_LL22: {struct _fat_ptr f=_tmp377;int i=_tmp378;
return f;}case 13U: _LL23: _tmp376=((struct Cyc_Absyn_Enum_p_Absyn_Raw_pat_struct*)_tmp373)->f2;_LL24: {struct Cyc_Absyn_Enumfield*ef=_tmp376;
_tmp375=ef;goto _LL26;}case 14U: _LL25: _tmp375=((struct Cyc_Absyn_AnonEnum_p_Absyn_Raw_pat_struct*)_tmp373)->f2;_LL26: {struct Cyc_Absyn_Enumfield*ef=_tmp375;
return({Cyc_Absynpp_qvar2string(ef->name);});}case 17U: _LL27: _tmp374=((struct Cyc_Absyn_Exp_p_Absyn_Raw_pat_struct*)_tmp373)->f1;_LL28: {struct Cyc_Absyn_Exp*e=_tmp374;
return({Cyc_Absynpp_exp2string(e);});}default: _LL29: _LL2A:
(int)_throw(& Cyc_Tcpat_Desc2string_val);}_LLA:;}}}else{_LL3: _tmp36E=((struct Cyc_Tcpat_Neg_Tcpat_Term_desc_struct*)_tmp36D)->f1;_LL4: {struct Cyc_Set_Set*s=_tmp36E;
# 1595
if(({Cyc_Set_is_empty(s);}))return({const char*_tmp3A9="_";_tag_fat(_tmp3A9,sizeof(char),2U);});{struct Cyc_Tcpat_Con_s*c=({({struct Cyc_Tcpat_Con_s*(*_tmp5D2)(struct Cyc_Set_Set*s)=({
struct Cyc_Tcpat_Con_s*(*_tmp3D0)(struct Cyc_Set_Set*s)=(struct Cyc_Tcpat_Con_s*(*)(struct Cyc_Set_Set*s))Cyc_Set_choose;_tmp3D0;});_tmp5D2(s);});});
# 1599
if((int)(((c->orig_pat).where_clause).tag == 2))(int)_throw(& Cyc_Tcpat_Desc2string_val);{struct Cyc_Absyn_Pat*orig_pat=({union Cyc_Tcpat_PatOrWhere _tmp3CF=c->orig_pat;if((_tmp3CF.pattern).tag != 1)_throw_match();(_tmp3CF.pattern).val;});
# 1601
void*_stmttmp30=({Cyc_Tcutil_compress((void*)_check_null(orig_pat->topt));});void*_tmp3AA=_stmttmp30;struct Cyc_Absyn_PtrAtts _tmp3AB;struct Cyc_Absyn_Aggrdecl*_tmp3AC;struct Cyc_Absyn_Datatypedecl*_tmp3AD;switch(*((int*)_tmp3AA)){case 0U: switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp3AA)->f1)){case 1U: if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp3AA)->f1)->f2 == Cyc_Absyn_Char_sz){_LL31: _LL32:
# 1604
{int i=0;for(0;i < 256;++ i){
struct Cyc_Tcpat_Con_s*c=({Cyc_Tcpat_char_con((char)i,orig_pat);});
if(!({({int(*_tmp5D4)(struct Cyc_Set_Set*s,struct Cyc_Tcpat_Con_s*elt)=({int(*_tmp3AE)(struct Cyc_Set_Set*s,struct Cyc_Tcpat_Con_s*elt)=(int(*)(struct Cyc_Set_Set*s,struct Cyc_Tcpat_Con_s*elt))Cyc_Set_member;_tmp3AE;});struct Cyc_Set_Set*_tmp5D3=s;_tmp5D4(_tmp5D3,c);});}))
return(struct _fat_ptr)({struct Cyc_Int_pa_PrintArg_struct _tmp3B1=({struct Cyc_Int_pa_PrintArg_struct _tmp454;_tmp454.tag=1U,_tmp454.f1=(unsigned long)i;_tmp454;});void*_tmp3AF[1U];_tmp3AF[0]=& _tmp3B1;({struct _fat_ptr _tmp5D5=({const char*_tmp3B0="%d";_tag_fat(_tmp3B0,sizeof(char),3U);});Cyc_aprintf(_tmp5D5,_tag_fat(_tmp3AF,sizeof(void*),1U));});});}}
# 1609
(int)_throw(& Cyc_Tcpat_Desc2string_val);}else{_LL33: _LL34:
# 1612
{unsigned i=0U;for(0;i < (unsigned)-1;++ i){
struct Cyc_Tcpat_Con_s*_tmp3B2=({Cyc_Tcpat_int_con((int)i,c->orig_pat);});struct Cyc_Tcpat_Con_s*c=_tmp3B2;
if(!({({int(*_tmp5D7)(struct Cyc_Set_Set*s,struct Cyc_Tcpat_Con_s*elt)=({int(*_tmp3B3)(struct Cyc_Set_Set*s,struct Cyc_Tcpat_Con_s*elt)=(int(*)(struct Cyc_Set_Set*s,struct Cyc_Tcpat_Con_s*elt))Cyc_Set_member;_tmp3B3;});struct Cyc_Set_Set*_tmp5D6=s;_tmp5D7(_tmp5D6,c);});}))
return(struct _fat_ptr)({struct Cyc_Int_pa_PrintArg_struct _tmp3B6=({struct Cyc_Int_pa_PrintArg_struct _tmp455;_tmp455.tag=1U,_tmp455.f1=(unsigned long)((int)i);_tmp455;});void*_tmp3B4[1U];_tmp3B4[0]=& _tmp3B6;({struct _fat_ptr _tmp5D8=({const char*_tmp3B5="%d";_tag_fat(_tmp3B5,sizeof(char),3U);});Cyc_aprintf(_tmp5D8,_tag_fat(_tmp3B4,sizeof(void*),1U));});});}}
# 1617
(int)_throw(& Cyc_Tcpat_Desc2string_val);}case 20U: if(((((struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp3AA)->f1)->f1).KnownDatatype).tag == 2){_LL37: _tmp3AD=*((((struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp3AA)->f1)->f1).KnownDatatype).val;_LL38: {struct Cyc_Absyn_Datatypedecl*tud=_tmp3AD;
# 1627
if(tud->is_extensible)(int)_throw(& Cyc_Tcpat_Desc2string_val);{struct Cyc_List_List*fields=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(tud->fields))->v;
# 1629
int span=({Cyc_List_length(fields);});
for(0;(unsigned)fields;fields=fields->tl){
struct _fat_ptr n=*(*((struct Cyc_Absyn_Datatypefield*)fields->hd)->name).f2;
struct Cyc_List_List*args=((struct Cyc_Absyn_Datatypefield*)fields->hd)->typs;
if(!({int(*_tmp3BD)(struct Cyc_Set_Set*s,struct Cyc_Tcpat_Con_s*elt)=({int(*_tmp3BE)(struct Cyc_Set_Set*s,struct Cyc_Tcpat_Con_s*elt)=(int(*)(struct Cyc_Set_Set*s,struct Cyc_Tcpat_Con_s*elt))Cyc_Set_member;_tmp3BE;});struct Cyc_Set_Set*_tmp3BF=s;struct Cyc_Tcpat_Con_s*_tmp3C0=({struct Cyc_Tcpat_Con_s*_tmp3C1=_cycalloc(sizeof(*_tmp3C1));({union Cyc_Tcpat_Name_value _tmp5D9=({Cyc_Tcpat_Name_v(n);});_tmp3C1->name=_tmp5D9;}),_tmp3C1->arity=0,_tmp3C1->span=0,_tmp3C1->orig_pat=c->orig_pat;_tmp3C1;});_tmp3BD(_tmp3BF,_tmp3C0);})){
if(({Cyc_List_length(args);})== 0)
return n;else{
# 1637
return(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp3C4=({struct Cyc_String_pa_PrintArg_struct _tmp456;_tmp456.tag=0U,_tmp456.f1=(struct _fat_ptr)((struct _fat_ptr)n);_tmp456;});void*_tmp3C2[1U];_tmp3C2[0]=& _tmp3C4;({struct _fat_ptr _tmp5DA=({const char*_tmp3C3="%s(...)";_tag_fat(_tmp3C3,sizeof(char),8U);});Cyc_aprintf(_tmp5DA,_tag_fat(_tmp3C2,sizeof(void*),1U));});});}}}
# 1640
(int)_throw(& Cyc_Tcpat_Desc2string_val);}}}else{goto _LL3B;}case 22U: if(((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp3AA)->f1)->f1).KnownAggr).tag == 2){_LL39: _tmp3AC=*((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp3AA)->f1)->f1).KnownAggr).val;if((int)_tmp3AC->kind == (int)Cyc_Absyn_UnionA){_LL3A: {struct Cyc_Absyn_Aggrdecl*ad=_tmp3AC;
# 1642
struct Cyc_List_List*fields=((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->fields;
int span=({Cyc_List_length(fields);});
struct _tuple2*_stmttmp31=ad->name;struct _fat_ptr _tmp3C5;_LL3E: _tmp3C5=*_stmttmp31->f2;_LL3F: {struct _fat_ptr union_name=_tmp3C5;
for(0;(unsigned)fields;fields=fields->tl){
struct _fat_ptr n=*((struct Cyc_Absyn_Aggrfield*)fields->hd)->name;
if(!({int(*_tmp3C6)(struct Cyc_Set_Set*s,struct Cyc_Tcpat_Con_s*elt)=({int(*_tmp3C7)(struct Cyc_Set_Set*s,struct Cyc_Tcpat_Con_s*elt)=(int(*)(struct Cyc_Set_Set*s,struct Cyc_Tcpat_Con_s*elt))Cyc_Set_member;_tmp3C7;});struct Cyc_Set_Set*_tmp3C8=s;struct Cyc_Tcpat_Con_s*_tmp3C9=({struct Cyc_Tcpat_Con_s*_tmp3CA=_cycalloc(sizeof(*_tmp3CA));({union Cyc_Tcpat_Name_value _tmp5DB=({Cyc_Tcpat_Name_v(n);});_tmp3CA->name=_tmp5DB;}),_tmp3CA->arity=0,_tmp3CA->span=0,_tmp3CA->orig_pat=c->orig_pat;_tmp3CA;});_tmp3C6(_tmp3C8,_tmp3C9);}))
return(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp3CD=({struct Cyc_String_pa_PrintArg_struct _tmp458;_tmp458.tag=0U,_tmp458.f1=(struct _fat_ptr)((struct _fat_ptr)union_name);_tmp458;});struct Cyc_String_pa_PrintArg_struct _tmp3CE=({struct Cyc_String_pa_PrintArg_struct _tmp457;_tmp457.tag=0U,_tmp457.f1=(struct _fat_ptr)((struct _fat_ptr)n);_tmp457;});void*_tmp3CB[2U];_tmp3CB[0]=& _tmp3CD,_tmp3CB[1]=& _tmp3CE;({struct _fat_ptr _tmp5DC=({const char*_tmp3CC="%s{.%s = _}";_tag_fat(_tmp3CC,sizeof(char),12U);});Cyc_aprintf(_tmp5DC,_tag_fat(_tmp3CB,sizeof(void*),2U));});});}
# 1650
(int)_throw(& Cyc_Tcpat_Desc2string_val);}}}else{goto _LL3B;}}else{goto _LL3B;}default: goto _LL3B;}case 3U: _LL35: _tmp3AB=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp3AA)->f1).ptr_atts;_LL36: {struct Cyc_Absyn_PtrAtts patts=_tmp3AB;
# 1619
void*n=patts.nullable;
int is_nullable=({Cyc_Tcutil_force_type2bool(0,n);});
if(is_nullable){
if(!({int(*_tmp3B7)(struct Cyc_Set_Set*s,struct Cyc_Tcpat_Con_s*elt)=({int(*_tmp3B8)(struct Cyc_Set_Set*s,struct Cyc_Tcpat_Con_s*elt)=(int(*)(struct Cyc_Set_Set*s,struct Cyc_Tcpat_Con_s*elt))Cyc_Set_member;_tmp3B8;});struct Cyc_Set_Set*_tmp3B9=s;struct Cyc_Tcpat_Con_s*_tmp3BA=({Cyc_Tcpat_null_con(orig_pat);});_tmp3B7(_tmp3B9,_tmp3BA);}))
return({const char*_tmp3BB="NULL";_tag_fat(_tmp3BB,sizeof(char),5U);});}
# 1621
return({const char*_tmp3BC="&_";_tag_fat(_tmp3BC,sizeof(char),3U);});}default: _LL3B: _LL3C:
# 1651
(int)_throw(& Cyc_Tcpat_Desc2string_val);}_LL30:;}}}}_LL0:;}
# 1655
static struct _fat_ptr*Cyc_Tcpat_desc2stringptr(void*d){
return({struct _fat_ptr*_tmp3D2=_cycalloc(sizeof(*_tmp3D2));({struct _fat_ptr _tmp5DD=({Cyc_Tcpat_desc2string(d);});*_tmp3D2=_tmp5DD;});_tmp3D2;});}
# 1658
static struct _fat_ptr Cyc_Tcpat_descs2string(struct Cyc_List_List*ds){
struct Cyc_List_List*ss=({({struct Cyc_List_List*(*_tmp5DE)(struct _fat_ptr*(*f)(void*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp3D7)(struct _fat_ptr*(*f)(void*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct _fat_ptr*(*f)(void*),struct Cyc_List_List*x))Cyc_List_map;_tmp3D7;});_tmp5DE(Cyc_Tcpat_desc2stringptr,ds);});});
struct _fat_ptr*comma=({struct _fat_ptr*_tmp3D6=_cycalloc(sizeof(*_tmp3D6));({struct _fat_ptr _tmp5DF=({const char*_tmp3D5=",";_tag_fat(_tmp3D5,sizeof(char),2U);});*_tmp3D6=_tmp5DF;});_tmp3D6;});
{struct Cyc_List_List*x=ss;for(0;x != 0;x=((struct Cyc_List_List*)_check_null(x))->tl){
if(x->tl != 0){
({struct Cyc_List_List*_tmp5E0=({struct Cyc_List_List*_tmp3D4=_cycalloc(sizeof(*_tmp3D4));_tmp3D4->hd=comma,_tmp3D4->tl=x->tl;_tmp3D4;});x->tl=_tmp5E0;});
x=x->tl;}}}
# 1667
return(struct _fat_ptr)({Cyc_strconcat_l(ss);});}
# 1670
static void Cyc_Tcpat_not_exhaust_err(unsigned loc,void*desc){
struct _handler_cons _tmp3D9;_push_handler(& _tmp3D9);{int _tmp3DB=0;if(setjmp(_tmp3D9.handler))_tmp3DB=1;if(!_tmp3DB){
{struct _fat_ptr s=({Cyc_Tcpat_desc2string(desc);});
({struct Cyc_String_pa_PrintArg_struct _tmp3DE=({struct Cyc_String_pa_PrintArg_struct _tmp459;_tmp459.tag=0U,_tmp459.f1=(struct _fat_ptr)((struct _fat_ptr)s);_tmp459;});void*_tmp3DC[1U];_tmp3DC[0]=& _tmp3DE;({unsigned _tmp5E2=loc;struct _fat_ptr _tmp5E1=({const char*_tmp3DD="patterns may not be exhaustive.\n\tmissing case for %s";_tag_fat(_tmp3DD,sizeof(char),53U);});Cyc_Tcutil_terr(_tmp5E2,_tmp5E1,_tag_fat(_tmp3DC,sizeof(void*),1U));});});}
# 1672
;_pop_handler();}else{void*_tmp3DA=(void*)Cyc_Core_get_exn_thrown();void*_tmp3DF=_tmp3DA;void*_tmp3E0;if(((struct Cyc_Tcpat_Desc2string_exn_struct*)_tmp3DF)->tag == Cyc_Tcpat_Desc2string){_LL1: _LL2:
# 1676
({void*_tmp3E1=0U;({unsigned _tmp5E4=loc;struct _fat_ptr _tmp5E3=({const char*_tmp3E2="patterns may not be exhaustive.";_tag_fat(_tmp3E2,sizeof(char),32U);});Cyc_Tcutil_terr(_tmp5E4,_tmp5E3,_tag_fat(_tmp3E1,sizeof(void*),0U));});});
goto _LL0;}else{_LL3: _tmp3E0=_tmp3DF;_LL4: {void*exn=_tmp3E0;(int)_rethrow(exn);}}_LL0:;}}}
# 1680
static void Cyc_Tcpat_rule_occurs(int dummy,struct Cyc_Tcpat_Rhs*rhs){
rhs->used=1;}
# 1684
void Cyc_Tcpat_check_switch_exhaustive(unsigned loc,struct Cyc_List_List*swcs,void**dopt){
# 1690
int where_ctr=0;
int*env=& where_ctr;
struct Cyc_List_List*match_rules=({({struct Cyc_List_List*(*_tmp5E6)(struct _tuple23*(*f)(int*,struct Cyc_Absyn_Switch_clause*),int*env,struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp3EA)(struct _tuple23*(*f)(int*,struct Cyc_Absyn_Switch_clause*),int*env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct _tuple23*(*f)(int*,struct Cyc_Absyn_Switch_clause*),int*env,struct Cyc_List_List*x))Cyc_List_map_c;_tmp3EA;});int*_tmp5E5=env;_tmp5E6(Cyc_Tcpat_get_match,_tmp5E5,swcs);});});
void*dec_tree=({Cyc_Tcpat_match_compile(match_rules);});
*dopt=dec_tree;{
# 1696
int ex_done=0;
({({void(*_tmp5E8)(void*d,void(*not_exhaust)(unsigned,void*),unsigned env1,void(*rhs_appears)(int,struct Cyc_Tcpat_Rhs*),int env2,int*exhaust_done)=({void(*_tmp3E5)(void*d,void(*not_exhaust)(unsigned,void*),unsigned env1,void(*rhs_appears)(int,struct Cyc_Tcpat_Rhs*),int env2,int*exhaust_done)=(void(*)(void*d,void(*not_exhaust)(unsigned,void*),unsigned env1,void(*rhs_appears)(int,struct Cyc_Tcpat_Rhs*),int env2,int*exhaust_done))Cyc_Tcpat_check_exhaust_overlap;_tmp3E5;});void*_tmp5E7=dec_tree;_tmp5E8(_tmp5E7,Cyc_Tcpat_not_exhaust_err,loc,Cyc_Tcpat_rule_occurs,0,& ex_done);});});
# 1699
for(0;match_rules != 0;match_rules=match_rules->tl){
struct _tuple23*_stmttmp32=(struct _tuple23*)match_rules->hd;unsigned _tmp3E7;int _tmp3E6;_LL1: _tmp3E6=(_stmttmp32->f2)->used;_tmp3E7=(_stmttmp32->f2)->pat_loc;_LL2: {int b=_tmp3E6;unsigned loc2=_tmp3E7;
if(!b){
({void*_tmp3E8=0U;({unsigned _tmp5EA=loc2;struct _fat_ptr _tmp5E9=({const char*_tmp3E9="redundant pattern (check for misspelled constructors in earlier patterns)";_tag_fat(_tmp3E9,sizeof(char),74U);});Cyc_Tcutil_terr(_tmp5EA,_tmp5E9,_tag_fat(_tmp3E8,sizeof(void*),0U));});});
# 1704
break;}}}}}
# 1713
static void Cyc_Tcpat_not_exhaust_warn(struct _tuple15*pr,void*desc){
(*pr).f2=0;{
struct _handler_cons _tmp3EC;_push_handler(& _tmp3EC);{int _tmp3EE=0;if(setjmp(_tmp3EC.handler))_tmp3EE=1;if(!_tmp3EE){
{struct _fat_ptr s=({Cyc_Tcpat_desc2string(desc);});
({struct Cyc_String_pa_PrintArg_struct _tmp3F1=({struct Cyc_String_pa_PrintArg_struct _tmp45A;_tmp45A.tag=0U,_tmp45A.f1=(struct _fat_ptr)((struct _fat_ptr)s);_tmp45A;});void*_tmp3EF[1U];_tmp3EF[0]=& _tmp3F1;({unsigned _tmp5EC=(*pr).f1;struct _fat_ptr _tmp5EB=({const char*_tmp3F0="pattern not exhaustive.\n\tmissing case for %s";_tag_fat(_tmp3F0,sizeof(char),45U);});Cyc_Tcutil_warn(_tmp5EC,_tmp5EB,_tag_fat(_tmp3EF,sizeof(void*),1U));});});}
# 1716
;_pop_handler();}else{void*_tmp3ED=(void*)Cyc_Core_get_exn_thrown();void*_tmp3F2=_tmp3ED;void*_tmp3F3;if(((struct Cyc_Tcpat_Desc2string_exn_struct*)_tmp3F2)->tag == Cyc_Tcpat_Desc2string){_LL1: _LL2:
# 1720
({void*_tmp3F4=0U;({unsigned _tmp5EE=(*pr).f1;struct _fat_ptr _tmp5ED=({const char*_tmp3F5="pattern not exhaustive.";_tag_fat(_tmp3F5,sizeof(char),24U);});Cyc_Tcutil_warn(_tmp5EE,_tmp5ED,_tag_fat(_tmp3F4,sizeof(void*),0U));});});
goto _LL0;}else{_LL3: _tmp3F3=_tmp3F2;_LL4: {void*exn=_tmp3F3;(int)_rethrow(exn);}}_LL0:;}}}}
# 1724
static void Cyc_Tcpat_dummy_fn(int i,struct Cyc_Tcpat_Rhs*rhs){
return;}
# 1724
int Cyc_Tcpat_check_let_pat_exhaustive(unsigned loc,struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Pat*p,void**dopt){
# 1729
struct Cyc_Tcpat_Rhs*rhs=({struct Cyc_Tcpat_Rhs*_tmp3FB=_cycalloc(sizeof(*_tmp3FB));_tmp3FB->used=0,_tmp3FB->pat_loc=p->loc,({struct Cyc_Absyn_Stmt*_tmp5EF=({Cyc_Absyn_skip_stmt(0U);});_tmp3FB->rhs=_tmp5EF;});_tmp3FB;});
struct Cyc_List_List*match_rules=({struct Cyc_List_List*_tmp3FA=_cycalloc(sizeof(*_tmp3FA));({struct _tuple23*_tmp5F1=({struct _tuple23*_tmp3F9=_cycalloc(sizeof(*_tmp3F9));({void*_tmp5F0=({Cyc_Tcpat_compile_pat(p);});_tmp3F9->f1=_tmp5F0;}),_tmp3F9->f2=rhs;_tmp3F9;});_tmp3FA->hd=_tmp5F1;}),_tmp3FA->tl=0;_tmp3FA;});
void*dec_tree=({Cyc_Tcpat_match_compile(match_rules);});
struct _tuple15 exhaust_env=({struct _tuple15 _tmp45B;_tmp45B.f1=loc,_tmp45B.f2=1;_tmp45B;});
int ex_done=0;
({({void(*_tmp5F2)(void*d,void(*not_exhaust)(struct _tuple15*,void*),struct _tuple15*env1,void(*rhs_appears)(int,struct Cyc_Tcpat_Rhs*),int env2,int*exhaust_done)=({void(*_tmp3F8)(void*d,void(*not_exhaust)(struct _tuple15*,void*),struct _tuple15*env1,void(*rhs_appears)(int,struct Cyc_Tcpat_Rhs*),int env2,int*exhaust_done)=(void(*)(void*d,void(*not_exhaust)(struct _tuple15*,void*),struct _tuple15*env1,void(*rhs_appears)(int,struct Cyc_Tcpat_Rhs*),int env2,int*exhaust_done))Cyc_Tcpat_check_exhaust_overlap;_tmp3F8;});_tmp5F2(dec_tree,Cyc_Tcpat_not_exhaust_warn,& exhaust_env,Cyc_Tcpat_dummy_fn,0,& ex_done);});});
# 1736
*dopt=dec_tree;
return exhaust_env.f2;}
# 1744
static struct _tuple23*Cyc_Tcpat_get_match2(struct Cyc_Absyn_Switch_clause*swc){
void*sp0=({Cyc_Tcpat_compile_pat(swc->pattern);});
# 1748
if(swc->where_clause != 0)
({void*_tmp3FD=0U;({unsigned _tmp5F4=((struct Cyc_Absyn_Exp*)_check_null(swc->where_clause))->loc;struct _fat_ptr _tmp5F3=({const char*_tmp3FE="&&-clauses not supported in exception handlers.";_tag_fat(_tmp3FE,sizeof(char),48U);});Cyc_Tcutil_terr(_tmp5F4,_tmp5F3,_tag_fat(_tmp3FD,sizeof(void*),0U));});});{
# 1748
struct Cyc_Tcpat_Rhs*rhs=({struct Cyc_Tcpat_Rhs*_tmp400=_cycalloc(sizeof(*_tmp400));
# 1751
_tmp400->used=0,_tmp400->pat_loc=(swc->pattern)->loc,_tmp400->rhs=swc->body;_tmp400;});
return({struct _tuple23*_tmp3FF=_cycalloc(sizeof(*_tmp3FF));_tmp3FF->f1=sp0,_tmp3FF->f2=rhs;_tmp3FF;});}}
# 1754
static void Cyc_Tcpat_not_exhaust_err2(unsigned loc,void*d){;}
# 1757
void Cyc_Tcpat_check_catch_overlap(unsigned loc,struct Cyc_Tcenv_Tenv*te,struct Cyc_List_List*swcs,void**dopt){
# 1761
struct Cyc_List_List*match_rules=({({struct Cyc_List_List*(*_tmp5F5)(struct _tuple23*(*f)(struct Cyc_Absyn_Switch_clause*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp408)(struct _tuple23*(*f)(struct Cyc_Absyn_Switch_clause*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct _tuple23*(*f)(struct Cyc_Absyn_Switch_clause*),struct Cyc_List_List*x))Cyc_List_map;_tmp408;});_tmp5F5(Cyc_Tcpat_get_match2,swcs);});});
void*dec_tree=({Cyc_Tcpat_match_compile(match_rules);});
*dopt=dec_tree;{
int ex_done=0;
({({void(*_tmp5F7)(void*d,void(*not_exhaust)(unsigned,void*),unsigned env1,void(*rhs_appears)(int,struct Cyc_Tcpat_Rhs*),int env2,int*exhaust_done)=({void(*_tmp403)(void*d,void(*not_exhaust)(unsigned,void*),unsigned env1,void(*rhs_appears)(int,struct Cyc_Tcpat_Rhs*),int env2,int*exhaust_done)=(void(*)(void*d,void(*not_exhaust)(unsigned,void*),unsigned env1,void(*rhs_appears)(int,struct Cyc_Tcpat_Rhs*),int env2,int*exhaust_done))Cyc_Tcpat_check_exhaust_overlap;_tmp403;});void*_tmp5F6=dec_tree;_tmp5F7(_tmp5F6,Cyc_Tcpat_not_exhaust_err2,loc,Cyc_Tcpat_rule_occurs,0,& ex_done);});});
# 1767
for(0;match_rules != 0;match_rules=match_rules->tl){
# 1769
if(match_rules->tl == 0)break;{struct _tuple23*_stmttmp33=(struct _tuple23*)match_rules->hd;unsigned _tmp405;int _tmp404;_LL1: _tmp404=(_stmttmp33->f2)->used;_tmp405=(_stmttmp33->f2)->pat_loc;_LL2: {int b=_tmp404;unsigned loc2=_tmp405;
# 1771
if(!b){
({void*_tmp406=0U;({unsigned _tmp5F9=loc2;struct _fat_ptr _tmp5F8=({const char*_tmp407="redundant pattern (check for misspelled constructors in earlier patterns)!";_tag_fat(_tmp407,sizeof(char),75U);});Cyc_Tcutil_terr(_tmp5F9,_tmp5F8,_tag_fat(_tmp406,sizeof(void*),0U));});});
break;}}}}}}
# 1757
int Cyc_Tcpat_has_vars(struct Cyc_Core_Opt*pat_vars){
# 1779
{struct Cyc_List_List*l=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(pat_vars))->v;for(0;l != 0;l=l->tl){
if((*((struct _tuple16*)l->hd)).f1 != 0)
return 1;}}
# 1779
return 0;}

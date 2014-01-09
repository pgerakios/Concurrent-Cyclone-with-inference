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
# 99
struct _fat_ptr Cyc_Core_rnew_string(struct _RegionHandle*,unsigned);extern char Cyc_Core_Invalid_argument[17U];struct Cyc_Core_Invalid_argument_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Failure[8U];struct Cyc_Core_Failure_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Impossible[11U];struct Cyc_Core_Impossible_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Not_found[10U];struct Cyc_Core_Not_found_exn_struct{char*tag;};extern char Cyc_Core_Unreachable[12U];struct Cyc_Core_Unreachable_exn_struct{char*tag;struct _fat_ptr f1;};
# 165
extern struct _RegionHandle*Cyc_Core_heap_region;
# 168
extern struct _RegionHandle*Cyc_Core_unique_region;struct Cyc_Core_DynamicRegion;struct Cyc_Core_NewDynamicRegion{struct Cyc_Core_DynamicRegion*key;};
# 305 "core.h"
struct _fat_ptr Cyc_Core_mkfat(void*arr,unsigned s,unsigned n);struct Cyc_Core_ThinRes{void*arr;unsigned nelts;};struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};
# 61 "list.h"
int Cyc_List_length(struct Cyc_List_List*x);extern char Cyc_List_List_mismatch[14U];struct Cyc_List_List_mismatch_exn_struct{char*tag;};extern char Cyc_List_Nth[4U];struct Cyc_List_Nth_exn_struct{char*tag;};struct Cyc___cycFILE;struct Cyc_String_pa_PrintArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Int_pa_PrintArg_struct{int tag;unsigned long f1;};struct Cyc_Double_pa_PrintArg_struct{int tag;double f1;};struct Cyc_LongDouble_pa_PrintArg_struct{int tag;long double f1;};struct Cyc_ShortPtr_pa_PrintArg_struct{int tag;short*f1;};struct Cyc_IntPtr_pa_PrintArg_struct{int tag;unsigned long*f1;};struct Cyc_ShortPtr_sa_ScanfArg_struct{int tag;short*f1;};struct Cyc_UShortPtr_sa_ScanfArg_struct{int tag;unsigned short*f1;};struct Cyc_IntPtr_sa_ScanfArg_struct{int tag;int*f1;};struct Cyc_UIntPtr_sa_ScanfArg_struct{int tag;unsigned*f1;};struct Cyc_StringPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_DoublePtr_sa_ScanfArg_struct{int tag;double*f1;};struct Cyc_FloatPtr_sa_ScanfArg_struct{int tag;float*f1;};struct Cyc_CharPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};extern char Cyc_FileCloseError[15U];struct Cyc_FileCloseError_exn_struct{char*tag;};extern char Cyc_FileOpenError[14U];struct Cyc_FileOpenError_exn_struct{char*tag;struct _fat_ptr f1;};
# 310 "cycboot.h"
int toupper(int);
# 38 "string.h"
unsigned long Cyc_strlen(struct _fat_ptr s);
# 49 "string.h"
int Cyc_strcmp(struct _fat_ptr s1,struct _fat_ptr s2);
# 51
int Cyc_strncmp(struct _fat_ptr s1,struct _fat_ptr s2,unsigned long len);
int Cyc_zstrcmp(struct _fat_ptr,struct _fat_ptr);
# 63
struct _fat_ptr Cyc_rstrconcat(struct _RegionHandle*,struct _fat_ptr,struct _fat_ptr);
# 65
struct _fat_ptr Cyc_rstrconcat_l(struct _RegionHandle*,struct Cyc_List_List*);
# 67
struct _fat_ptr Cyc_rstr_sepstr(struct _RegionHandle*,struct Cyc_List_List*,struct _fat_ptr);
# 72
struct _fat_ptr Cyc_strncpy(struct _fat_ptr,struct _fat_ptr,unsigned long);
# 100 "string.h"
struct _fat_ptr Cyc_rexpand(struct _RegionHandle*,struct _fat_ptr s,unsigned long sz);
# 102
struct _fat_ptr Cyc_cond_rrealloc_str(struct _RegionHandle*r,struct _fat_ptr str,unsigned long sz);
# 105
struct _fat_ptr Cyc_rstrdup(struct _RegionHandle*,struct _fat_ptr src);
# 110
struct _fat_ptr Cyc_rsubstring(struct _RegionHandle*,struct _fat_ptr,int ofs,unsigned long n);
# 115
struct _fat_ptr Cyc_rreplace_suffix(struct _RegionHandle*r,struct _fat_ptr src,struct _fat_ptr curr_suffix,struct _fat_ptr new_suffix);
# 120
struct _fat_ptr Cyc_strchr(struct _fat_ptr s,char c);
struct _fat_ptr Cyc_mstrchr(struct _fat_ptr s,char c);
# 127
struct _fat_ptr Cyc_mstrpbrk(struct _fat_ptr s,struct _fat_ptr accept);
unsigned long Cyc_strspn(struct _fat_ptr s,struct _fat_ptr accept);
# 135
struct Cyc_List_List*Cyc_rexplode(struct _RegionHandle*,struct _fat_ptr s);
# 29 "assert.h"
void*Cyc___assert_fail(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line);
# 39 "string.cyc"
unsigned long Cyc_strlen(struct _fat_ptr s){
unsigned long i;
unsigned sz=_get_fat_size(s,sizeof(char));
for(i=0U;i < sz;++ i){
if((int)((const char*)s.curr)[(int)i]== (int)'\000')
return i;}
# 46
return i;}
# 52
int Cyc_strcmp(struct _fat_ptr s1,struct _fat_ptr s2){
if((char*)s1.curr == (char*)s2.curr)
return 0;{
# 53
int i=0;
# 56
unsigned sz1=_get_fat_size(s1,sizeof(char));
unsigned sz2=_get_fat_size(s2,sizeof(char));
unsigned minsz=sz1 < sz2?sz1: sz2;
minsz <= _get_fat_size(s1,sizeof(char))&& minsz <= _get_fat_size(s2,sizeof(char))?0:({({int(*_tmpB4)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmp1)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmp1;});struct _fat_ptr _tmpB3=({const char*_tmp2="minsz <= numelts(s1) && minsz <= numelts(s2)";_tag_fat(_tmp2,sizeof(char),45U);});_tmpB4(_tmpB3,({const char*_tmp3="string.cyc";_tag_fat(_tmp3,sizeof(char),11U);}),59U);});});
while((unsigned)i < minsz){
char c1=((const char*)s1.curr)[i];
char c2=((const char*)s2.curr)[i];
if((int)c1 == (int)'\000'){
if((int)c2 == (int)'\000')return 0;else{
return - 1;}}else{
if((int)c2 == (int)'\000')return 1;else{
# 68
int diff=(int)c1 - (int)c2;
if(diff != 0)return diff;}}
# 71
++ i;}
# 73
if(sz1 == sz2)return 0;if(minsz < sz2){
# 75
if((int)*((const char*)_check_fat_subscript(s2,sizeof(char),i))== (int)'\000')return 0;else{
return - 1;}}else{
# 78
if((int)*((const char*)_check_fat_subscript(s1,sizeof(char),i))== (int)'\000')return 0;else{
return 1;}}}}
# 83
int Cyc_strptrcmp(struct _fat_ptr*s1,struct _fat_ptr*s2){
return({Cyc_strcmp((struct _fat_ptr)*s1,(struct _fat_ptr)*s2);});}
# 87
inline static int Cyc_ncmp(struct _fat_ptr s1,unsigned long len1,struct _fat_ptr s2,unsigned long len2,unsigned long n){
# 90
if(n <= (unsigned long)0)return 0;{unsigned long min_len=
# 92
len1 > len2?len2: len1;
unsigned long bound=min_len > n?n: min_len;
# 95
bound <= _get_fat_size(s1,sizeof(char))&& bound <= _get_fat_size(s2,sizeof(char))?0:({({int(*_tmpB6)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmp6)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmp6;});struct _fat_ptr _tmpB5=({const char*_tmp7="bound <= numelts(s1) && bound <= numelts(s2)";_tag_fat(_tmp7,sizeof(char),45U);});_tmpB6(_tmpB5,({const char*_tmp8="string.cyc";_tag_fat(_tmp8,sizeof(char),11U);}),95U);});});
# 97
{int i=0;for(0;(unsigned long)i < bound;++ i){
int retc;
if((retc=(int)((const char*)s1.curr)[i]- (int)((const char*)s2.curr)[i])!= 0)
return retc;}}
# 102
if(len1 < n || len2 < n)
return(int)len1 - (int)len2;
# 102
return 0;}}
# 109
int Cyc_strncmp(struct _fat_ptr s1,struct _fat_ptr s2,unsigned long n){
unsigned long len1=({Cyc_strlen(s1);});
unsigned long len2=({Cyc_strlen(s2);});
return({Cyc_ncmp(s1,len1,s2,len2,n);});}
# 119
int Cyc_zstrcmp(struct _fat_ptr a,struct _fat_ptr b){
if((char*)a.curr == (char*)b.curr)
return 0;{
# 120
unsigned long as=
# 122
_get_fat_size(a,sizeof(char));
unsigned long bs=_get_fat_size(b,sizeof(char));
unsigned long min_length=as < bs?as: bs;
int i=-1;
# 127
min_length <= _get_fat_size(a,sizeof(char))&& min_length <= _get_fat_size(b,sizeof(char))?0:({({int(*_tmpB8)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmpB)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmpB;});struct _fat_ptr _tmpB7=({const char*_tmpC="min_length <= numelts(a) && min_length <= numelts(b)";_tag_fat(_tmpC,sizeof(char),53U);});_tmpB8(_tmpB7,({const char*_tmpD="string.cyc";_tag_fat(_tmpD,sizeof(char),11U);}),127U);});});
# 129
while((++ i,(unsigned long)i < min_length)){
int diff=(int)((const char*)a.curr)[i]- (int)((const char*)b.curr)[i];
if(diff != 0)
return diff;}
# 134
return(int)as - (int)bs;}}
# 137
int Cyc_zstrncmp(struct _fat_ptr s1,struct _fat_ptr s2,unsigned long n){
if(n <= (unsigned long)0)return 0;{unsigned long s1size=
# 140
_get_fat_size(s1,sizeof(char));
unsigned long s2size=_get_fat_size(s2,sizeof(char));
unsigned long min_size=s1size > s2size?s2size: s1size;
unsigned long bound=min_size > n?n: min_size;
# 145
bound <= _get_fat_size(s1,sizeof(char))&& bound <= _get_fat_size(s2,sizeof(char))?0:({({int(*_tmpBA)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmpF)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmpF;});struct _fat_ptr _tmpB9=({const char*_tmp10="bound <= numelts(s1) && bound <= numelts(s2)";_tag_fat(_tmp10,sizeof(char),45U);});_tmpBA(_tmpB9,({const char*_tmp11="string.cyc";_tag_fat(_tmp11,sizeof(char),11U);}),145U);});});
# 147
{int i=0;for(0;(unsigned long)i < bound;++ i){
if((int)((const char*)s1.curr)[i]< (int)((const char*)s2.curr)[i])
return - 1;else{
if((int)((const char*)s2.curr)[i]< (int)((const char*)s1.curr)[i])
return 1;}}}
# 153
if(min_size <= bound)
return 0;
# 153
if(s1size < s2size)
# 156
return - 1;else{
# 158
return 1;}}}
# 162
int Cyc_zstrptrcmp(struct _fat_ptr*a,struct _fat_ptr*b){
if((int)a == (int)b)return 0;return({Cyc_zstrcmp((struct _fat_ptr)*a,(struct _fat_ptr)*b);});}
# 172
inline static struct _fat_ptr Cyc_int_strcato(struct _fat_ptr dest,struct _fat_ptr src){
int i;
unsigned long dsize;unsigned long slen;unsigned long dlen;
# 176
dsize=_get_fat_size(dest,sizeof(char));
dlen=({Cyc_strlen((struct _fat_ptr)dest);});
slen=({Cyc_strlen(src);});
# 180
if(slen + dlen <= dsize){
slen <= _get_fat_size(src,sizeof(char))?0:({({int(*_tmpBC)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmp14)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmp14;});struct _fat_ptr _tmpBB=({const char*_tmp15="slen <= numelts(src)";_tag_fat(_tmp15,sizeof(char),21U);});_tmpBC(_tmpBB,({const char*_tmp16="string.cyc";_tag_fat(_tmp16,sizeof(char),11U);}),181U);});});
for(i=0;(unsigned long)i < slen;++ i){
({struct _fat_ptr _tmp17=_fat_ptr_plus(dest,sizeof(char),(int)((unsigned long)i + dlen));char _tmp18=*((char*)_check_fat_subscript(_tmp17,sizeof(char),0U));char _tmp19=((const char*)src.curr)[i];if(_get_fat_size(_tmp17,sizeof(char))== 1U &&(_tmp18 == 0 && _tmp19 != 0))_throw_arraybounds();*((char*)_tmp17.curr)=_tmp19;});}
# 185
if((unsigned long)i + dlen < dsize)
({struct _fat_ptr _tmp1A=_fat_ptr_plus(dest,sizeof(char),(int)((unsigned long)i + dlen));char _tmp1B=*((char*)_check_fat_subscript(_tmp1A,sizeof(char),0U));char _tmp1C='\000';if(_get_fat_size(_tmp1A,sizeof(char))== 1U &&(_tmp1B == 0 && _tmp1C != 0))_throw_arraybounds();*((char*)_tmp1A.curr)=_tmp1C;});}else{
# 189
(int)_throw(({struct Cyc_Core_Invalid_argument_exn_struct*_tmp1E=_cycalloc(sizeof(*_tmp1E));_tmp1E->tag=Cyc_Core_Invalid_argument,({struct _fat_ptr _tmpBD=({const char*_tmp1D="strcat";_tag_fat(_tmp1D,sizeof(char),7U);});_tmp1E->f1=_tmpBD;});_tmp1E;}));}
return dest;}
# 195
struct _fat_ptr Cyc_strcat(struct _fat_ptr dest,struct _fat_ptr src){
return({Cyc_int_strcato(dest,src);});}
# 200
struct _fat_ptr Cyc_rstrconcat(struct _RegionHandle*r,struct _fat_ptr a,struct _fat_ptr b){
unsigned long alen=({Cyc_strlen(a);});
unsigned long blen=({Cyc_strlen(b);});
struct _fat_ptr ans=({Cyc_Core_rnew_string(r,(alen + blen)+ (unsigned long)1);});
int i;int j;
alen <= _get_fat_size(ans,sizeof(char))&& alen <= _get_fat_size(a,sizeof(char))?0:({({int(*_tmpBF)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmp21)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmp21;});struct _fat_ptr _tmpBE=({const char*_tmp22="alen <= numelts(ans) && alen <= numelts(a)";_tag_fat(_tmp22,sizeof(char),43U);});_tmpBF(_tmpBE,({const char*_tmp23="string.cyc";_tag_fat(_tmp23,sizeof(char),11U);}),205U);});});
for(i=0;(unsigned long)i < alen;++ i){({struct _fat_ptr _tmp24=_fat_ptr_plus(ans,sizeof(char),i);char _tmp25=*((char*)_check_fat_subscript(_tmp24,sizeof(char),0U));char _tmp26=((const char*)a.curr)[i];if(_get_fat_size(_tmp24,sizeof(char))== 1U &&(_tmp25 == 0 && _tmp26 != 0))_throw_arraybounds();*((char*)_tmp24.curr)=_tmp26;});}
blen <= _get_fat_size(b,sizeof(char))?0:({({int(*_tmpC1)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmp27)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmp27;});struct _fat_ptr _tmpC0=({const char*_tmp28="blen <= numelts(b)";_tag_fat(_tmp28,sizeof(char),19U);});_tmpC1(_tmpC0,({const char*_tmp29="string.cyc";_tag_fat(_tmp29,sizeof(char),11U);}),207U);});});
for(j=0;(unsigned long)j < blen;++ j){({struct _fat_ptr _tmp2A=_fat_ptr_plus(ans,sizeof(char),i + j);char _tmp2B=*((char*)_check_fat_subscript(_tmp2A,sizeof(char),0U));char _tmp2C=((const char*)b.curr)[j];if(_get_fat_size(_tmp2A,sizeof(char))== 1U &&(_tmp2B == 0 && _tmp2C != 0))_throw_arraybounds();*((char*)_tmp2A.curr)=_tmp2C;});}
return ans;}
# 200
struct _fat_ptr Cyc_strconcat(struct _fat_ptr a,struct _fat_ptr b){
# 213
return({Cyc_rstrconcat(Cyc_Core_heap_region,a,b);});}
# 217
struct _fat_ptr Cyc_rstrconcat_l(struct _RegionHandle*r,struct Cyc_List_List*strs){
# 219
unsigned long len;
unsigned long total_len=0U;
struct _fat_ptr ans;
struct _RegionHandle _tmp2F=_new_region("temp");struct _RegionHandle*temp=& _tmp2F;_push_region(temp);{
struct Cyc_List_List*lens=({struct Cyc_List_List*_tmp31=_region_malloc(temp,sizeof(*_tmp31));
_tmp31->hd=(void*)0U,_tmp31->tl=0;_tmp31;});
# 226
struct Cyc_List_List*end=lens;
{struct Cyc_List_List*p=strs;for(0;p != 0;p=p->tl){
len=({Cyc_strlen((struct _fat_ptr)*((struct _fat_ptr*)p->hd));});
total_len +=len;
({struct Cyc_List_List*_tmpC2=({struct Cyc_List_List*_tmp30=_region_malloc(temp,sizeof(*_tmp30));_tmp30->hd=(void*)len,_tmp30->tl=0;_tmp30;});((struct Cyc_List_List*)_check_null(end))->tl=_tmpC2;});
end=end->tl;}}
# 233
lens=lens->tl;
ans=({Cyc_Core_rnew_string(r,total_len + (unsigned long)1);});{
unsigned long i=0U;
while(strs != 0){
struct _fat_ptr next=*((struct _fat_ptr*)strs->hd);
len=(unsigned long)((struct Cyc_List_List*)_check_null(lens))->hd;
({({struct _fat_ptr _tmpC4=_fat_ptr_decrease_size(_fat_ptr_plus(ans,sizeof(char),(int)i),sizeof(char),1U);struct _fat_ptr _tmpC3=(struct _fat_ptr)next;Cyc_strncpy(_tmpC4,_tmpC3,len);});});
i +=len;
strs=strs->tl;
lens=lens->tl;}}}{
# 245
struct _fat_ptr _tmp32=ans;_npop_handler(0U);return _tmp32;}
# 222
;_pop_region();}
# 217
struct _fat_ptr Cyc_strconcat_l(struct Cyc_List_List*strs){
# 249
return({Cyc_rstrconcat_l(Cyc_Core_heap_region,strs);});}
# 254
struct _fat_ptr Cyc_rstr_sepstr(struct _RegionHandle*r,struct Cyc_List_List*strs,struct _fat_ptr separator){
if(strs == 0)return({Cyc_Core_rnew_string(r,0U);});if(strs->tl == 0)
return({Cyc_rstrdup(r,(struct _fat_ptr)*((struct _fat_ptr*)strs->hd));});{
# 255
struct Cyc_List_List*p=strs;
# 258
struct _RegionHandle _tmp35=_new_region("temp");struct _RegionHandle*temp=& _tmp35;_push_region(temp);
{struct Cyc_List_List*lens=({struct Cyc_List_List*_tmp38=_region_malloc(temp,sizeof(*_tmp38));_tmp38->hd=(void*)0U,_tmp38->tl=0;_tmp38;});
struct Cyc_List_List*end=lens;
unsigned long len;
unsigned long total_len=0U;
unsigned long list_len=0U;
for(0;p != 0;p=p->tl){
len=({Cyc_strlen((struct _fat_ptr)*((struct _fat_ptr*)p->hd));});
total_len +=len;
({struct Cyc_List_List*_tmpC5=({struct Cyc_List_List*_tmp36=_region_malloc(temp,sizeof(*_tmp36));_tmp36->hd=(void*)len,_tmp36->tl=0;_tmp36;});((struct Cyc_List_List*)_check_null(end))->tl=_tmpC5;});
end=end->tl;
++ list_len;}
# 271
lens=lens->tl;{
unsigned long seplen=({Cyc_strlen(separator);});
total_len +=(list_len - (unsigned long)1)* seplen;{
struct _fat_ptr ans=({Cyc_Core_rnew_string(r,total_len + (unsigned long)1);});
unsigned long i=0U;
while(((struct Cyc_List_List*)_check_null(strs))->tl != 0){
struct _fat_ptr next=*((struct _fat_ptr*)strs->hd);
len=(unsigned long)((struct Cyc_List_List*)_check_null(lens))->hd;
({({struct _fat_ptr _tmpC7=_fat_ptr_decrease_size(_fat_ptr_plus(ans,sizeof(char),(int)i),sizeof(char),1U);struct _fat_ptr _tmpC6=(struct _fat_ptr)next;Cyc_strncpy(_tmpC7,_tmpC6,len);});});
i +=len;
({({struct _fat_ptr _tmpC9=_fat_ptr_decrease_size(_fat_ptr_plus(ans,sizeof(char),(int)i),sizeof(char),1U);struct _fat_ptr _tmpC8=separator;Cyc_strncpy(_tmpC9,_tmpC8,seplen);});});
i +=seplen;
strs=strs->tl;
lens=lens->tl;}
# 286
({({struct _fat_ptr _tmpCB=_fat_ptr_decrease_size(_fat_ptr_plus(ans,sizeof(char),(int)i),sizeof(char),1U);struct _fat_ptr _tmpCA=(struct _fat_ptr)*((struct _fat_ptr*)strs->hd);Cyc_strncpy(_tmpCB,_tmpCA,(unsigned long)((struct Cyc_List_List*)_check_null(lens))->hd);});});{
struct _fat_ptr _tmp37=ans;_npop_handler(0U);return _tmp37;}}}}
# 259
;_pop_region();}}
# 254
struct _fat_ptr Cyc_str_sepstr(struct Cyc_List_List*strs,struct _fat_ptr separator){
# 292
return({Cyc_rstr_sepstr(Cyc_Core_heap_region,strs,separator);});}
# 296
struct _fat_ptr Cyc_strncpy(struct _fat_ptr dest,struct _fat_ptr src,unsigned long n){
int i;
n <= _get_fat_size(src,sizeof(char))&& n <= _get_fat_size(dest,sizeof(char))?0:({({int(*_tmpCD)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmp3B)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmp3B;});struct _fat_ptr _tmpCC=({const char*_tmp3C="n <= numelts(src) && n <= numelts(dest)";_tag_fat(_tmp3C,sizeof(char),40U);});_tmpCD(_tmpCC,({const char*_tmp3D="string.cyc";_tag_fat(_tmp3D,sizeof(char),11U);}),298U);});});
for(i=0;(unsigned long)i < n;++ i){
char srcChar=((const char*)src.curr)[i];
if((int)srcChar == (int)'\000')break;((char*)dest.curr)[i]=srcChar;}
# 304
for(0;(unsigned long)i < n;++ i){
((char*)dest.curr)[i]='\000';}
# 307
return dest;}
# 311
struct _fat_ptr Cyc_zstrncpy(struct _fat_ptr dest,struct _fat_ptr src,unsigned long n){
n <= _get_fat_size(dest,sizeof(char))&& n <= _get_fat_size(src,sizeof(char))?0:({({int(*_tmpCF)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmp3F)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmp3F;});struct _fat_ptr _tmpCE=({const char*_tmp40="n <= numelts(dest) && n <= numelts(src)";_tag_fat(_tmp40,sizeof(char),40U);});_tmpCF(_tmpCE,({const char*_tmp41="string.cyc";_tag_fat(_tmp41,sizeof(char),11U);}),312U);});});{
int i;
for(i=0;(unsigned long)i < n;++ i){
((char*)dest.curr)[i]=((const char*)src.curr)[i];}
return dest;}}
# 319
struct _fat_ptr Cyc_strcpy(struct _fat_ptr dest,struct _fat_ptr src){
unsigned ssz=_get_fat_size(src,sizeof(char));
unsigned dsz=_get_fat_size(dest,sizeof(char));
if(ssz <= dsz){
unsigned i;
for(i=0U;i < ssz;++ i){
char srcChar=((const char*)src.curr)[(int)i];
({struct _fat_ptr _tmp43=_fat_ptr_plus(dest,sizeof(char),(int)i);char _tmp44=*((char*)_check_fat_subscript(_tmp43,sizeof(char),0U));char _tmp45=srcChar;if(_get_fat_size(_tmp43,sizeof(char))== 1U &&(_tmp44 == 0 && _tmp45 != 0))_throw_arraybounds();*((char*)_tmp43.curr)=_tmp45;});
if((int)srcChar == (int)'\000')break;}}else{
# 331
unsigned long len=({Cyc_strlen(src);});
({({struct _fat_ptr _tmpD1=_fat_ptr_decrease_size(dest,sizeof(char),1U);struct _fat_ptr _tmpD0=src;Cyc_strncpy(_tmpD1,_tmpD0,len);});});
if(len < _get_fat_size(dest,sizeof(char)))
({struct _fat_ptr _tmp46=_fat_ptr_plus(dest,sizeof(char),(int)len);char _tmp47=*((char*)_check_fat_subscript(_tmp46,sizeof(char),0U));char _tmp48='\000';if(_get_fat_size(_tmp46,sizeof(char))== 1U &&(_tmp47 == 0 && _tmp48 != 0))_throw_arraybounds();*((char*)_tmp46.curr)=_tmp48;});}
# 336
return dest;}
# 342
struct _fat_ptr Cyc_rstrdup(struct _RegionHandle*r,struct _fat_ptr src){
unsigned long len;
struct _fat_ptr temp;
# 346
len=({Cyc_strlen(src);});
temp=({Cyc_Core_rnew_string(r,len + (unsigned long)1);});
{struct _fat_ptr temp2=_fat_ptr_decrease_size(temp,sizeof(char),1U);
struct _fat_ptr _tmp4A;_LL1: _tmp4A=temp2;_LL2: {struct _fat_ptr dst=_tmp4A;
({Cyc_strncpy(dst,src,len);});}}
# 352
return temp;}
# 342
struct _fat_ptr Cyc_strdup(struct _fat_ptr src){
# 356
return({Cyc_rstrdup(Cyc_Core_heap_region,src);});}
# 359
struct _fat_ptr Cyc_rrealloc(struct _RegionHandle*r,struct _fat_ptr s,unsigned long sz){
struct _fat_ptr temp;
unsigned long slen;
# 363
slen=_get_fat_size(s,sizeof(char));
sz=sz > slen?sz: slen;
temp=({unsigned _tmp4D=sz;_tag_fat(_region_calloc(r,sizeof(char),_tmp4D),sizeof(char),_tmp4D);});
# 367
{struct _fat_ptr _tmp4E;_LL1: _tmp4E=temp;_LL2: {struct _fat_ptr dst=_tmp4E;
({Cyc_strncpy((struct _fat_ptr)dst,(struct _fat_ptr)s,slen);});}}
# 371
return temp;}
# 374
struct _fat_ptr Cyc_rexpand(struct _RegionHandle*r,struct _fat_ptr s,unsigned long sz){
struct _fat_ptr temp;
unsigned long slen;
# 378
slen=({Cyc_strlen(s);});
sz=sz > slen?sz: slen;
temp=({Cyc_Core_rnew_string(r,sz);});
# 382
{struct _fat_ptr _tmp50;_LL1: _tmp50=temp;_LL2: {struct _fat_ptr dst=_tmp50;
({Cyc_strncpy((struct _fat_ptr)dst,s,slen);});}}
# 386
if(slen != _get_fat_size(s,sizeof(char)))
({struct _fat_ptr _tmp51=_fat_ptr_plus(temp,sizeof(char),(int)slen);char _tmp52=*((char*)_check_fat_subscript(_tmp51,sizeof(char),0U));char _tmp53='\000';if(_get_fat_size(_tmp51,sizeof(char))== 1U &&(_tmp52 == 0 && _tmp53 != 0))_throw_arraybounds();*((char*)_tmp51.curr)=_tmp53;});
# 386
return temp;}
# 374
struct _fat_ptr Cyc_expand(struct _fat_ptr s,unsigned long sz){
# 393
return({Cyc_rexpand(Cyc_Core_heap_region,s,sz);});}
# 396
struct _fat_ptr Cyc_cond_rrealloc_str(struct _RegionHandle*r,struct _fat_ptr str,unsigned long sz){
unsigned long maxsizeP=_get_fat_size(str,sizeof(char));
struct _fat_ptr res=_tag_fat(0,0,0);
# 400
if(maxsizeP == (unsigned long)0){
maxsizeP=(unsigned long)30 > sz?30U: sz;
res=({Cyc_Core_rnew_string(r,maxsizeP);});
({struct _fat_ptr _tmp56=_fat_ptr_plus(res,sizeof(char),0);char _tmp57=*((char*)_check_fat_subscript(_tmp56,sizeof(char),0U));char _tmp58='\000';if(_get_fat_size(_tmp56,sizeof(char))== 1U &&(_tmp57 == 0 && _tmp58 != 0))_throw_arraybounds();*((char*)_tmp56.curr)=_tmp58;});}else{
# 405
if(sz > maxsizeP){
maxsizeP=maxsizeP * (unsigned long)2 > (sz * (unsigned long)5)/ (unsigned long)4?maxsizeP * (unsigned long)2:(sz * (unsigned long)5)/ (unsigned long)4;{
struct _fat_ptr _tmp59;_LL1: _tmp59=str;_LL2: {struct _fat_ptr dst=_tmp59;
res=({Cyc_rexpand(r,(struct _fat_ptr)dst,maxsizeP);});}}}}
# 400
return res;}
# 396
struct _fat_ptr Cyc_realloc_str(struct _fat_ptr str,unsigned long sz){
# 416
struct _fat_ptr res=({Cyc_cond_rrealloc_str(Cyc_Core_heap_region,str,sz);});
if(({char*_tmpD2=(char*)res.curr;_tmpD2 == (char*)(_tag_fat(0,0,0)).curr;}))return str;else{
return res;}}
# 426
struct _fat_ptr Cyc_rsubstring(struct _RegionHandle*r,struct _fat_ptr s,int start,unsigned long amt){
# 430
struct _fat_ptr ans=({Cyc_Core_rnew_string(r,amt + (unsigned long)1);});
s=_fat_ptr_plus(s,sizeof(char),start);
amt < _get_fat_size(ans,sizeof(char))&& amt <= _get_fat_size(s,sizeof(char))?0:({({int(*_tmpD4)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmp5C)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmp5C;});struct _fat_ptr _tmpD3=({const char*_tmp5D="amt < numelts(ans) && amt <= numelts(s)";_tag_fat(_tmp5D,sizeof(char),40U);});_tmpD4(_tmpD3,({const char*_tmp5E="string.cyc";_tag_fat(_tmp5E,sizeof(char),11U);}),432U);});});
{unsigned long i=0U;for(0;i < amt;++ i){
({struct _fat_ptr _tmp5F=_fat_ptr_plus(ans,sizeof(char),(int)i);char _tmp60=*((char*)_check_fat_subscript(_tmp5F,sizeof(char),0U));char _tmp61=((const char*)s.curr)[(int)i];if(_get_fat_size(_tmp5F,sizeof(char))== 1U &&(_tmp60 == 0 && _tmp61 != 0))_throw_arraybounds();*((char*)_tmp5F.curr)=_tmp61;});}}
({struct _fat_ptr _tmp62=_fat_ptr_plus(ans,sizeof(char),(int)amt);char _tmp63=*((char*)_check_fat_subscript(_tmp62,sizeof(char),0U));char _tmp64='\000';if(_get_fat_size(_tmp62,sizeof(char))== 1U &&(_tmp63 == 0 && _tmp64 != 0))_throw_arraybounds();*((char*)_tmp62.curr)=_tmp64;});
return ans;}
# 426
struct _fat_ptr Cyc_substring(struct _fat_ptr s,int start,unsigned long amt){
# 440
return({Cyc_rsubstring(Cyc_Core_heap_region,s,start,amt);});}
# 445
struct _fat_ptr Cyc_rreplace_suffix(struct _RegionHandle*r,struct _fat_ptr src,struct _fat_ptr curr_suffix,struct _fat_ptr new_suffix){
unsigned long m=_get_fat_size(src,sizeof(char));
unsigned long n=_get_fat_size(curr_suffix,sizeof(char));
struct _fat_ptr err=({const char*_tmp69="replace_suffix";_tag_fat(_tmp69,sizeof(char),15U);});
if(m < n)
(int)_throw(({struct Cyc_Core_Invalid_argument_exn_struct*_tmp67=_cycalloc(sizeof(*_tmp67));_tmp67->tag=Cyc_Core_Invalid_argument,_tmp67->f1=err;_tmp67;}));
# 449
{
# 451
unsigned long i=1U;
# 449
for(0;i <= n;++ i){
# 452
if(({int _tmpD5=(int)*((const char*)_check_fat_subscript(src,sizeof(char),(int)(m - i)));_tmpD5 != (int)*((const char*)_check_fat_subscript(curr_suffix,sizeof(char),(int)(n - i)));}))
(int)_throw(({struct Cyc_Core_Invalid_argument_exn_struct*_tmp68=_cycalloc(sizeof(*_tmp68));_tmp68->tag=Cyc_Core_Invalid_argument,_tmp68->f1=err;_tmp68;}));}}{
# 449
struct _fat_ptr ans=({Cyc_Core_rnew_string(r,((m - n)+ _get_fat_size(new_suffix,sizeof(char)))+ (unsigned long)1);});
# 455
({({struct _fat_ptr _tmpD7=_fat_ptr_decrease_size(ans,sizeof(char),1U);struct _fat_ptr _tmpD6=src;Cyc_strncpy(_tmpD7,_tmpD6,m - n);});});
({({struct _fat_ptr _tmpD9=_fat_ptr_decrease_size(_fat_ptr_plus(ans,sizeof(char),(int)(m - n)),sizeof(char),1U);struct _fat_ptr _tmpD8=new_suffix;Cyc_strncpy(_tmpD9,_tmpD8,_get_fat_size(new_suffix,sizeof(char)));});});
return ans;}}
# 445
struct _fat_ptr Cyc_replace_suffix(struct _fat_ptr src,struct _fat_ptr curr_suffix,struct _fat_ptr new_suffix){
# 461
return({Cyc_rreplace_suffix(Cyc_Core_heap_region,src,curr_suffix,new_suffix);});}
# 467
struct _fat_ptr Cyc_strpbrk(struct _fat_ptr s,struct _fat_ptr accept){
int len=(int)_get_fat_size(s,sizeof(char));
unsigned asize=_get_fat_size(accept,sizeof(char));
char c;
unsigned i;
for(i=0U;i < (unsigned)len &&(int)(c=((const char*)s.curr)[(int)i])!= 0;++ i){
unsigned j=0U;for(0;j < asize;++ j){
if((int)c == (int)((const char*)accept.curr)[(int)j])
return _fat_ptr_plus(s,sizeof(char),(int)i);}}
# 477
return _tag_fat(0,0,0);}
# 480
struct _fat_ptr Cyc_mstrpbrk(struct _fat_ptr s,struct _fat_ptr accept){
int len=(int)_get_fat_size(s,sizeof(char));
unsigned asize=_get_fat_size(accept,sizeof(char));
char c;
unsigned i;
for(i=0U;i < (unsigned)len &&(int)(c=((char*)s.curr)[(int)i])!= 0;++ i){
unsigned j=0U;for(0;j < asize;++ j){
if((int)c == (int)((const char*)accept.curr)[(int)j])
return _fat_ptr_plus(s,sizeof(char),(int)i);}}
# 490
return _tag_fat(0,0,0);}
# 495
struct _fat_ptr Cyc_mstrchr(struct _fat_ptr s,char c){
int len=(int)_get_fat_size(s,sizeof(char));
char c2;
unsigned i;
# 500
for(i=0U;i < (unsigned)len &&(int)(c2=((char*)s.curr)[(int)i])!= 0;++ i){
if((int)c2 == (int)c)return _fat_ptr_plus(s,sizeof(char),(int)i);}
# 503
return _tag_fat(0,0,0);}
# 506
struct _fat_ptr Cyc_strchr(struct _fat_ptr s,char c){
int len=(int)_get_fat_size(s,sizeof(char));
char c2;
unsigned i;
# 511
for(i=0U;i < (unsigned)len &&(int)(c2=((const char*)s.curr)[(int)i])!= 0;++ i){
if((int)c2 == (int)c)return _fat_ptr_plus(s,sizeof(char),(int)i);}
# 514
return _tag_fat(0,0,0);}
# 519
struct _fat_ptr Cyc_strrchr(struct _fat_ptr s0,char c){
int len=(int)({Cyc_strlen((struct _fat_ptr)s0);});
int i=len - 1;
struct _fat_ptr s=s0;
_fat_ptr_inplace_plus(& s,sizeof(char),i);
# 525
for(0;i >= 0;(i --,_fat_ptr_inplace_plus_post(& s,sizeof(char),-1))){
if((int)*((const char*)_check_fat_subscript(s,sizeof(char),0U))== (int)c)
return(struct _fat_ptr)s;}
# 529
return _tag_fat(0,0,0);}
# 532
struct _fat_ptr Cyc_mstrrchr(struct _fat_ptr s0,char c){
int len=(int)({Cyc_strlen((struct _fat_ptr)s0);});
int i=len - 1;
struct _fat_ptr s=s0;
_fat_ptr_inplace_plus(& s,sizeof(char),i);
# 538
for(0;i >= 0;(i --,_fat_ptr_inplace_plus_post(& s,sizeof(char),-1))){
if((int)*((char*)_check_fat_subscript(s,sizeof(char),0U))== (int)c)
return(struct _fat_ptr)s;}
# 542
return _tag_fat(0,0,0);}
# 547
struct _fat_ptr Cyc_strstr(struct _fat_ptr haystack,struct _fat_ptr needle){
if(!((unsigned)haystack.curr)|| !((unsigned)needle.curr))(int)_throw(({struct Cyc_Core_Invalid_argument_exn_struct*_tmp73=_cycalloc(sizeof(*_tmp73));_tmp73->tag=Cyc_Core_Invalid_argument,({struct _fat_ptr _tmpDA=({const char*_tmp72="strstr";_tag_fat(_tmp72,sizeof(char),7U);});_tmp73->f1=_tmpDA;});_tmp73;}));if((int)*((const char*)_check_fat_subscript(needle,sizeof(char),0U))== (int)'\000')
return haystack;{
# 548
int len=(int)({Cyc_strlen((struct _fat_ptr)needle);});
# 552
{struct _fat_ptr start=haystack;for(0;({
char*_tmpDB=(char*)(start=({Cyc_strchr(start,*((const char*)_check_fat_subscript(needle,sizeof(char),0U)));})).curr;_tmpDB != (char*)(_tag_fat(0,0,0)).curr;});start=({({struct _fat_ptr _tmpDC=
_fat_ptr_plus(start,sizeof(char),1);Cyc_strchr(_tmpDC,*((const char*)_check_fat_subscript(needle,sizeof(char),0U)));});})){
if(({Cyc_strncmp((struct _fat_ptr)start,(struct _fat_ptr)needle,(unsigned long)len);})== 0)
return start;}}
# 552
return
# 558
 _tag_fat(0,0,0);}}
# 561
struct _fat_ptr Cyc_mstrstr(struct _fat_ptr haystack,struct _fat_ptr needle){
if(!((unsigned)haystack.curr)|| !((unsigned)needle.curr))(int)_throw(({struct Cyc_Core_Invalid_argument_exn_struct*_tmp76=_cycalloc(sizeof(*_tmp76));_tmp76->tag=Cyc_Core_Invalid_argument,({struct _fat_ptr _tmpDD=({const char*_tmp75="mstrstr";_tag_fat(_tmp75,sizeof(char),8U);});_tmp76->f1=_tmpDD;});_tmp76;}));if((int)*((const char*)_check_fat_subscript(needle,sizeof(char),0U))== (int)'\000')
return haystack;{
# 562
int len=(int)({Cyc_strlen((struct _fat_ptr)needle);});
# 566
{struct _fat_ptr start=haystack;for(0;({
char*_tmpDE=(char*)(start=({Cyc_mstrchr(start,*((const char*)_check_fat_subscript(needle,sizeof(char),0U)));})).curr;_tmpDE != (char*)(_tag_fat(0,0,0)).curr;});start=({({struct _fat_ptr _tmpDF=
_fat_ptr_plus(start,sizeof(char),1);Cyc_mstrchr(_tmpDF,*((const char*)_check_fat_subscript(needle,sizeof(char),0U)));});})){
if(({Cyc_strncmp((struct _fat_ptr)start,(struct _fat_ptr)needle,(unsigned long)len);})== 0)
return start;}}
# 566
return
# 572
 _tag_fat(0,0,0);}}
# 561
unsigned long Cyc_strspn(struct _fat_ptr s,struct _fat_ptr accept){
# 579
unsigned long len=({Cyc_strlen((struct _fat_ptr)s);});
unsigned asize=_get_fat_size(accept,sizeof(char));
# 582
len <= _get_fat_size(s,sizeof(char))?0:({({int(*_tmpE1)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmp78)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmp78;});struct _fat_ptr _tmpE0=({const char*_tmp79="len <= numelts(s)";_tag_fat(_tmp79,sizeof(char),18U);});_tmpE1(_tmpE0,({const char*_tmp7A="string.cyc";_tag_fat(_tmp7A,sizeof(char),11U);}),582U);});});
{unsigned long i=0U;for(0;i < len;++ i){
int j;
for(j=0;(unsigned)j < asize;++ j){
if((int)((const char*)s.curr)[(int)i]== (int)((const char*)accept.curr)[j])
break;}
# 585
if((unsigned)j == asize)
# 589
return i;}}
# 592
return len;}
# 561
unsigned long Cyc_strcspn(struct _fat_ptr s,struct _fat_ptr accept){
# 599
unsigned long len=({Cyc_strlen((struct _fat_ptr)s);});
unsigned asize=_get_fat_size(accept,sizeof(char));
# 602
len <= _get_fat_size(s,sizeof(char))?0:({({int(*_tmpE3)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmp7C)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmp7C;});struct _fat_ptr _tmpE2=({const char*_tmp7D="len <= numelts(s)";_tag_fat(_tmp7D,sizeof(char),18U);});_tmpE3(_tmpE2,({const char*_tmp7E="string.cyc";_tag_fat(_tmp7E,sizeof(char),11U);}),602U);});});
{unsigned long i=0U;for(0;i < len;++ i){
int j;
for(j=0;(unsigned)j < asize;++ j){
if((int)((const char*)s.curr)[(int)i]== (int)((const char*)accept.curr)[j])return i;}}}
# 608
return len;}
# 615
struct _fat_ptr Cyc_strtok(struct _fat_ptr s,struct _fat_ptr delim){
# 617
static struct _fat_ptr olds={(void*)0,(void*)0,(void*)(0 + 0)};
struct _fat_ptr token;
# 620
if(({char*_tmpE4=(char*)s.curr;_tmpE4 == (char*)(_tag_fat(0,0,0)).curr;})){
if(({char*_tmpE5=(char*)olds.curr;_tmpE5 == (char*)(_tag_fat(0,0,0)).curr;}))
return _tag_fat(0,0,0);
# 621
s=olds;}{
# 620
unsigned long inc=({Cyc_strspn((struct _fat_ptr)s,delim);});
# 628
if(inc >= _get_fat_size(s,sizeof(char))||(int)*((char*)_check_fat_subscript(_fat_ptr_plus(s,sizeof(char),(int)inc),sizeof(char),0U))== (int)'\000'){
# 630
olds=_tag_fat(0,0,0);
return _tag_fat(0,0,0);}else{
# 634
_fat_ptr_inplace_plus(& s,sizeof(char),(int)inc);}
# 637
token=s;
s=({Cyc_mstrpbrk(token,(struct _fat_ptr)delim);});
if(({char*_tmpE6=(char*)s.curr;_tmpE6 == (char*)(_tag_fat(0,0,0)).curr;}))
# 641
olds=_tag_fat(0,0,0);else{
# 645
({struct _fat_ptr _tmp80=s;char _tmp81=*((char*)_check_fat_subscript(_tmp80,sizeof(char),0U));char _tmp82='\000';if(_get_fat_size(_tmp80,sizeof(char))== 1U &&(_tmp81 == 0 && _tmp82 != 0))_throw_arraybounds();*((char*)_tmp80.curr)=_tmp82;});
olds=_fat_ptr_plus(s,sizeof(char),1);}
# 648
return token;}}
# 652
struct Cyc_List_List*Cyc_rexplode(struct _RegionHandle*r,struct _fat_ptr s){
struct Cyc_List_List*result=0;
{int i=(int)(({Cyc_strlen(s);})- (unsigned long)1);for(0;i >= 0;-- i){
result=({struct Cyc_List_List*_tmp84=_region_malloc(r,sizeof(*_tmp84));_tmp84->hd=(void*)((int)*((const char*)_check_fat_subscript(s,sizeof(char),i))),_tmp84->tl=result;_tmp84;});}}
return result;}
# 659
struct Cyc_List_List*Cyc_explode(struct _fat_ptr s){
return({Cyc_rexplode(Cyc_Core_heap_region,s);});}
# 659
struct _fat_ptr Cyc_implode(struct Cyc_List_List*chars){
# 664
struct _fat_ptr s=({struct _fat_ptr(*_tmp8A)(unsigned)=Cyc_Core_new_string;unsigned _tmp8B=(unsigned)(({Cyc_List_length(chars);})+ 1);_tmp8A(_tmp8B);});
unsigned long i=0U;
while(chars != 0){
({struct _fat_ptr _tmp87=_fat_ptr_plus(s,sizeof(char),(int)i ++);char _tmp88=*((char*)_check_fat_subscript(_tmp87,sizeof(char),0U));char _tmp89=(char)((int)chars->hd);if(_get_fat_size(_tmp87,sizeof(char))== 1U &&(_tmp88 == 0 && _tmp89 != 0))_throw_arraybounds();*((char*)_tmp87.curr)=_tmp89;});
chars=chars->tl;}
# 670
return s;}
# 674
inline static int Cyc_casecmp(struct _fat_ptr s1,unsigned long len1,struct _fat_ptr s2,unsigned long len2){
# 677
unsigned long min_length=len1 < len2?len1: len2;
# 679
min_length <= _get_fat_size(s1,sizeof(char))&& min_length <= _get_fat_size(s2,sizeof(char))?0:({({int(*_tmpE8)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmp8D)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmp8D;});struct _fat_ptr _tmpE7=({const char*_tmp8E="min_length <= numelts(s1) && min_length <= numelts(s2)";_tag_fat(_tmp8E,sizeof(char),55U);});_tmpE8(_tmpE7,({const char*_tmp8F="string.cyc";_tag_fat(_tmp8F,sizeof(char),11U);}),679U);});});{
# 681
int i=-1;
while((++ i,(unsigned long)i < min_length)){
int diff=({int _tmpE9=({toupper((int)((const char*)s1.curr)[i]);});_tmpE9 - ({toupper((int)((const char*)s2.curr)[i]);});});
if(diff != 0)
return diff;}
# 687
return(int)len1 - (int)len2;}}
# 690
int Cyc_strcasecmp(struct _fat_ptr s1,struct _fat_ptr s2){
if((char*)s1.curr == (char*)s2.curr)
return 0;{
# 691
unsigned long len1=({Cyc_strlen(s1);});
# 694
unsigned long len2=({Cyc_strlen(s2);});
return({Cyc_casecmp(s1,len1,s2,len2);});}}
# 698
inline static int Cyc_caseless_ncmp(struct _fat_ptr s1,unsigned long len1,struct _fat_ptr s2,unsigned long len2,unsigned long n){
# 701
if(n <= (unsigned long)0)return 0;{unsigned long min_len=
# 703
len1 > len2?len2: len1;
unsigned long bound=min_len > n?n: min_len;
# 706
bound <= _get_fat_size(s1,sizeof(char))&& bound <= _get_fat_size(s2,sizeof(char))?0:({({int(*_tmpEB)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmp92)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmp92;});struct _fat_ptr _tmpEA=({const char*_tmp93="bound <= numelts(s1) && bound <= numelts(s2)";_tag_fat(_tmp93,sizeof(char),45U);});_tmpEB(_tmpEA,({const char*_tmp94="string.cyc";_tag_fat(_tmp94,sizeof(char),11U);}),706U);});});
# 708
{int i=0;for(0;(unsigned long)i < bound;++ i){
int retc;
if((retc=({int _tmpEC=({toupper((int)((const char*)s1.curr)[i]);});_tmpEC - ({toupper((int)((const char*)s2.curr)[i]);});}))!= 0)
return retc;}}
# 713
if(len1 < n || len2 < n)
return(int)len1 - (int)len2;
# 713
return 0;}}
# 719
int Cyc_strncasecmp(struct _fat_ptr s1,struct _fat_ptr s2,unsigned long n){
unsigned long len1=({Cyc_strlen(s1);});
unsigned long len2=({Cyc_strlen(s2);});
return({Cyc_caseless_ncmp(s1,len1,s2,len2,n);});}
# 728
void*memcpy(void*,const void*,unsigned long n);
void*memmove(void*,const void*,unsigned long n);
int memcmp(const void*,const void*,unsigned long n);
char*memchr(const char*,char c,unsigned long n);
void*memset(void*,int c,unsigned long n);
void bcopy(const void*src,void*dest,unsigned long n);
void bzero(void*s,unsigned long n);
char*GC_realloc(char*,unsigned n);
# 740
struct _fat_ptr Cyc_realloc(struct _fat_ptr s,unsigned long n){
unsigned _tmp97;_LL1: _tmp97=n;_LL2: {unsigned l=_tmp97;
char*res=({GC_realloc((char*)_untag_fat_ptr(s,sizeof(char),1U),l);});
return({({struct _fat_ptr(*_tmpEE)(char*arr,unsigned s,unsigned n)=({struct _fat_ptr(*_tmp98)(char*arr,unsigned s,unsigned n)=(struct _fat_ptr(*)(char*arr,unsigned s,unsigned n))Cyc_Core_mkfat;_tmp98;});char*_tmpED=(char*)_check_null(res);_tmpEE(_tmpED,sizeof(char),l);});});}}
# 746
struct _fat_ptr Cyc__memcpy(struct _fat_ptr d,struct _fat_ptr s,unsigned long n,unsigned sz){
if(((({void*_tmpF0=(void*)d.curr;_tmpF0 == (void*)(_tag_fat(0,0,0)).curr;})|| _get_fat_size(d,sizeof(void))< n)||({void*_tmpEF=(void*)s.curr;_tmpEF == (void*)(_tag_fat(0,0,0)).curr;}))|| _get_fat_size(s,sizeof(void))< n)
(int)_throw(({struct Cyc_Core_Invalid_argument_exn_struct*_tmp9B=_cycalloc(sizeof(*_tmp9B));_tmp9B->tag=Cyc_Core_Invalid_argument,({struct _fat_ptr _tmpF1=({const char*_tmp9A="memcpy";_tag_fat(_tmp9A,sizeof(char),7U);});_tmp9B->f1=_tmpF1;});_tmp9B;}));
# 747
({({void*_tmpF3=(void*)_untag_fat_ptr(d,sizeof(void),1U);const void*_tmpF2=(const void*)_untag_fat_ptr(s,sizeof(void),1U);memcpy(_tmpF3,_tmpF2,n * sz);});});
# 750
return d;}
# 753
struct _fat_ptr Cyc__memmove(struct _fat_ptr d,struct _fat_ptr s,unsigned long n,unsigned sz){
if(((({void*_tmpF5=(void*)d.curr;_tmpF5 == (void*)(_tag_fat(0,0,0)).curr;})|| _get_fat_size(d,sizeof(void))< n)||({void*_tmpF4=(void*)s.curr;_tmpF4 == (void*)(_tag_fat(0,0,0)).curr;}))|| _get_fat_size(s,sizeof(void))< n)
(int)_throw(({struct Cyc_Core_Invalid_argument_exn_struct*_tmp9E=_cycalloc(sizeof(*_tmp9E));_tmp9E->tag=Cyc_Core_Invalid_argument,({struct _fat_ptr _tmpF6=({const char*_tmp9D="memove";_tag_fat(_tmp9D,sizeof(char),7U);});_tmp9E->f1=_tmpF6;});_tmp9E;}));
# 754
({({void*_tmpF8=(void*)_untag_fat_ptr(d,sizeof(void),1U);const void*_tmpF7=(const void*)_untag_fat_ptr(s,sizeof(void),1U);memmove(_tmpF8,_tmpF7,n * sz);});});
# 757
return d;}
# 760
int Cyc_memcmp(struct _fat_ptr s1,struct _fat_ptr s2,unsigned long n){
if(((({char*_tmpFA=(char*)s1.curr;_tmpFA == (char*)(_tag_fat(0,0,0)).curr;})||({char*_tmpF9=(char*)s2.curr;_tmpF9 == (char*)(_tag_fat(0,0,0)).curr;}))|| _get_fat_size(s1,sizeof(char))< n)|| _get_fat_size(s2,sizeof(char))< n)
(int)_throw(({struct Cyc_Core_Invalid_argument_exn_struct*_tmpA1=_cycalloc(sizeof(*_tmpA1));_tmpA1->tag=Cyc_Core_Invalid_argument,({struct _fat_ptr _tmpFB=({const char*_tmpA0="memcmp";_tag_fat(_tmpA0,sizeof(char),7U);});_tmpA1->f1=_tmpFB;});_tmpA1;}));
# 761
return({({const void*_tmpFD=(const void*)_untag_fat_ptr(s1,sizeof(char),1U);const void*_tmpFC=(const void*)_untag_fat_ptr(s2,sizeof(char),1U);memcmp(_tmpFD,_tmpFC,n);});});}
# 766
struct _fat_ptr Cyc_memchr(struct _fat_ptr s,char c,unsigned long n){
unsigned sz=_get_fat_size(s,sizeof(char));
if(({char*_tmpFE=(char*)s.curr;_tmpFE == (char*)(_tag_fat(0,0,0)).curr;})|| n > sz)
(int)_throw(({struct Cyc_Core_Invalid_argument_exn_struct*_tmpA4=_cycalloc(sizeof(*_tmpA4));_tmpA4->tag=Cyc_Core_Invalid_argument,({struct _fat_ptr _tmpFF=({const char*_tmpA3="memchr";_tag_fat(_tmpA3,sizeof(char),7U);});_tmpA4->f1=_tmpFF;});_tmpA4;}));{
# 768
char*p=({memchr((const char*)_untag_fat_ptr(s,sizeof(char),1U),c,n);});
# 771
if(p == 0)return _tag_fat(0,0,0);{unsigned sval=(unsigned)((const char*)_untag_fat_ptr(s,sizeof(char),1U));
# 773
unsigned pval=(unsigned)((const char*)p);
unsigned delta=pval - sval;
return _fat_ptr_plus(s,sizeof(char),(int)delta);}}}
# 778
struct _fat_ptr Cyc_mmemchr(struct _fat_ptr s,char c,unsigned long n){
unsigned sz=_get_fat_size(s,sizeof(char));
if(({char*_tmp100=(char*)s.curr;_tmp100 == (char*)(_tag_fat(0,0,0)).curr;})|| n > sz)
(int)_throw(({struct Cyc_Core_Invalid_argument_exn_struct*_tmpA7=_cycalloc(sizeof(*_tmpA7));_tmpA7->tag=Cyc_Core_Invalid_argument,({struct _fat_ptr _tmp101=({const char*_tmpA6="mmemchr";_tag_fat(_tmpA6,sizeof(char),8U);});_tmpA7->f1=_tmp101;});_tmpA7;}));{
# 780
char*p=({memchr((const char*)_untag_fat_ptr(s,sizeof(char),1U),c,n);});
# 783
if(p == 0)return _tag_fat(0,0,0);{unsigned sval=(unsigned)((const char*)_untag_fat_ptr(s,sizeof(char),1U));
# 785
unsigned pval=(unsigned)p;
unsigned delta=pval - sval;
return _fat_ptr_plus(s,sizeof(char),(int)delta);}}}
# 790
struct _fat_ptr Cyc_memset(struct _fat_ptr s,char c,unsigned long n){
if(({char*_tmp102=(char*)s.curr;_tmp102 == (char*)(_tag_fat(0,0,0)).curr;})|| n > _get_fat_size(s,sizeof(char)))
(int)_throw(({struct Cyc_Core_Invalid_argument_exn_struct*_tmpAA=_cycalloc(sizeof(*_tmpAA));_tmpAA->tag=Cyc_Core_Invalid_argument,({struct _fat_ptr _tmp103=({const char*_tmpA9="memset";_tag_fat(_tmpA9,sizeof(char),7U);});_tmpAA->f1=_tmp103;});_tmpAA;}));
# 791
({memset((void*)((char*)_untag_fat_ptr(s,sizeof(char),1U)),(int)c,n);});
# 794
return s;}
# 797
void Cyc_bzero(struct _fat_ptr s,unsigned long n){
if(({char*_tmp104=(char*)s.curr;_tmp104 == (char*)(_tag_fat(0,0,0)).curr;})|| _get_fat_size(s,sizeof(char))< n)
(int)_throw(({struct Cyc_Core_Invalid_argument_exn_struct*_tmpAD=_cycalloc(sizeof(*_tmpAD));_tmpAD->tag=Cyc_Core_Invalid_argument,({struct _fat_ptr _tmp105=({const char*_tmpAC="bzero";_tag_fat(_tmpAC,sizeof(char),6U);});_tmpAD->f1=_tmp105;});_tmpAD;}));
# 798
({({void(*_tmp107)(char*s,unsigned long n)=({void(*_tmpAE)(char*s,unsigned long n)=(void(*)(char*s,unsigned long n))bzero;_tmpAE;});char*_tmp106=(char*)_untag_fat_ptr(s,sizeof(char),1U);_tmp107(_tmp106,n);});});}
# 803
void Cyc__bcopy(struct _fat_ptr src,struct _fat_ptr dst,unsigned long n,unsigned sz){
if(((({void*_tmp109=(void*)src.curr;_tmp109 == (void*)(_tag_fat(0,0,0)).curr;})|| _get_fat_size(src,sizeof(void))< n)||({void*_tmp108=(void*)dst.curr;_tmp108 == (void*)(_tag_fat(0,0,0)).curr;}))|| _get_fat_size(dst,sizeof(void))< n)
(int)_throw(({struct Cyc_Core_Invalid_argument_exn_struct*_tmpB1=_cycalloc(sizeof(*_tmpB1));_tmpB1->tag=Cyc_Core_Invalid_argument,({struct _fat_ptr _tmp10A=({const char*_tmpB0="bcopy";_tag_fat(_tmpB0,sizeof(char),6U);});_tmpB1->f1=_tmp10A;});_tmpB1;}));
# 804
({({const void*_tmp10C=(const void*)_untag_fat_ptr(src,sizeof(void),1U);void*_tmp10B=(void*)_untag_fat_ptr(dst,sizeof(void),1U);bcopy(_tmp10C,_tmp10B,n * sz);});});}

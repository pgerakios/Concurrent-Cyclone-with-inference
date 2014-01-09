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
# 126 "core.h"
int Cyc_Core_ptrcmp(void*,void*);extern char Cyc_Core_Invalid_argument[17U];struct Cyc_Core_Invalid_argument_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Failure[8U];struct Cyc_Core_Failure_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Impossible[11U];struct Cyc_Core_Impossible_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Not_found[10U];struct Cyc_Core_Not_found_exn_struct{char*tag;};extern char Cyc_Core_Unreachable[12U];struct Cyc_Core_Unreachable_exn_struct{char*tag;struct _fat_ptr f1;};
# 165
extern struct _RegionHandle*Cyc_Core_heap_region;
# 168
extern struct _RegionHandle*Cyc_Core_unique_region;struct Cyc_Core_DynamicRegion;struct Cyc_Core_NewDynamicRegion{struct Cyc_Core_DynamicRegion*key;};struct Cyc_Core_ThinRes{void*arr;unsigned nelts;};struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};
# 61 "list.h"
int Cyc_List_length(struct Cyc_List_List*x);
# 70
struct Cyc_List_List*Cyc_List_copy(struct Cyc_List_List*x);
# 76
struct Cyc_List_List*Cyc_List_map(void*(*f)(void*),struct Cyc_List_List*x);extern char Cyc_List_List_mismatch[14U];struct Cyc_List_List_mismatch_exn_struct{char*tag;};
# 135
void Cyc_List_iter_c(void(*f)(void*,void*),void*env,struct Cyc_List_List*x);
# 175
struct Cyc_List_List*Cyc_List_rrev(struct _RegionHandle*,struct Cyc_List_List*x);
# 184
struct Cyc_List_List*Cyc_List_append(struct Cyc_List_List*x,struct Cyc_List_List*y);extern char Cyc_List_Nth[4U];struct Cyc_List_Nth_exn_struct{char*tag;};
# 258
int Cyc_List_exists(int(*pred)(void*),struct Cyc_List_List*x);
# 261
int Cyc_List_exists_c(int(*pred)(void*,void*),void*env,struct Cyc_List_List*x);struct Cyc___cycFILE;
# 51 "cycboot.h"
extern struct Cyc___cycFILE*Cyc_stdout;
# 53
extern struct Cyc___cycFILE*Cyc_stderr;struct Cyc_String_pa_PrintArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Int_pa_PrintArg_struct{int tag;unsigned long f1;};struct Cyc_Double_pa_PrintArg_struct{int tag;double f1;};struct Cyc_LongDouble_pa_PrintArg_struct{int tag;long double f1;};struct Cyc_ShortPtr_pa_PrintArg_struct{int tag;short*f1;};struct Cyc_IntPtr_pa_PrintArg_struct{int tag;unsigned long*f1;};
# 73
struct _fat_ptr Cyc_aprintf(struct _fat_ptr,struct _fat_ptr);
# 88
int Cyc_fflush(struct Cyc___cycFILE*);
# 100
int Cyc_fprintf(struct Cyc___cycFILE*,struct _fat_ptr,struct _fat_ptr);struct Cyc_ShortPtr_sa_ScanfArg_struct{int tag;short*f1;};struct Cyc_UShortPtr_sa_ScanfArg_struct{int tag;unsigned short*f1;};struct Cyc_IntPtr_sa_ScanfArg_struct{int tag;int*f1;};struct Cyc_UIntPtr_sa_ScanfArg_struct{int tag;unsigned*f1;};struct Cyc_StringPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_DoublePtr_sa_ScanfArg_struct{int tag;double*f1;};struct Cyc_FloatPtr_sa_ScanfArg_struct{int tag;float*f1;};struct Cyc_CharPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};
# 157 "cycboot.h"
int Cyc_printf(struct _fat_ptr,struct _fat_ptr);extern char Cyc_FileCloseError[15U];struct Cyc_FileCloseError_exn_struct{char*tag;};extern char Cyc_FileOpenError[14U];struct Cyc_FileOpenError_exn_struct{char*tag;struct _fat_ptr f1;};
# 49 "string.h"
int Cyc_strcmp(struct _fat_ptr s1,struct _fat_ptr s2);
int Cyc_strptrcmp(struct _fat_ptr*s1,struct _fat_ptr*s2);
# 36 "position.h"
struct _fat_ptr Cyc_Position_string_of_loc(unsigned);
struct _fat_ptr Cyc_Position_string_of_segment(unsigned);struct Cyc_Position_Error;struct Cyc_Hashtable_Table;
# 39 "hashtable.h"
struct Cyc_Hashtable_Table*Cyc_Hashtable_create(int sz,int(*cmp)(void*,void*),int(*hash)(void*));
# 50
void Cyc_Hashtable_insert(struct Cyc_Hashtable_Table*t,void*key,void*val);
# 56
void**Cyc_Hashtable_lookup_opt(struct Cyc_Hashtable_Table*t,void*key);struct Cyc_Iter_Iter{void*env;int(*next)(void*env,void*dest);};struct Cyc_Dict_T;struct Cyc_Dict_Dict{int(*rel)(void*,void*);struct _RegionHandle*r;const struct Cyc_Dict_T*t;};extern char Cyc_Dict_Present[8U];struct Cyc_Dict_Present_exn_struct{char*tag;};extern char Cyc_Dict_Absent[7U];struct Cyc_Dict_Absent_exn_struct{char*tag;};struct Cyc_Relations_Reln;struct _union_Nmspace_Rel_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Abs_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_C_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Loc_n{int tag;int val;};union Cyc_Absyn_Nmspace{struct _union_Nmspace_Rel_n Rel_n;struct _union_Nmspace_Abs_n Abs_n;struct _union_Nmspace_C_n C_n;struct _union_Nmspace_Loc_n Loc_n;};struct _tuple0{union Cyc_Absyn_Nmspace f1;struct _fat_ptr*f2;};
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
enum Cyc_Absyn_Format_Type{Cyc_Absyn_Printf_ft =0U,Cyc_Absyn_Scanf_ft =1U};struct Cyc_Absyn_Regparm_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Stdcall_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Cdecl_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Fastcall_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Noreturn_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Const_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Aligned_att_Absyn_Attribute_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Packed_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Section_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Nocommon_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Shared_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Unused_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Weak_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Dllimport_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Dllexport_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_instrument_function_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Constructor_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Destructor_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_check_memory_usage_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Format_att_Absyn_Attribute_struct{int tag;enum Cyc_Absyn_Format_Type f1;int f2;int f3;};struct Cyc_Absyn_Initializes_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Noliveunique_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Consume_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Pure_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Mode_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Alias_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Always_inline_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_throw_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Non_null_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Deprecated_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_VectorSize_att_Absyn_Attribute_struct{int tag;int f1;};
# 521
extern struct Cyc_Absyn_Noreturn_att_Absyn_Attribute_struct Cyc_Absyn_Noreturn_att_val;struct Cyc_Absyn_Carray_mod_Absyn_Type_modifier_struct{int tag;void*f1;unsigned f2;};struct Cyc_Absyn_ConstArray_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;unsigned f3;};struct Cyc_Absyn_Pointer_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_Absyn_PtrAtts f1;struct Cyc_Absyn_Tqual f2;};struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct{int tag;void*f1;};struct Cyc_Absyn_TypeParams_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_List_List*f1;unsigned f2;int f3;};struct Cyc_Absyn_Attributes_mod_Absyn_Type_modifier_struct{int tag;unsigned f1;struct Cyc_List_List*f2;};struct _union_Cnst_Null_c{int tag;int val;};struct _tuple3{enum Cyc_Absyn_Sign f1;char f2;};struct _union_Cnst_Char_c{int tag;struct _tuple3 val;};struct _union_Cnst_Wchar_c{int tag;struct _fat_ptr val;};struct _tuple4{enum Cyc_Absyn_Sign f1;short f2;};struct _union_Cnst_Short_c{int tag;struct _tuple4 val;};struct _tuple5{enum Cyc_Absyn_Sign f1;int f2;};struct _union_Cnst_Int_c{int tag;struct _tuple5 val;};struct _tuple6{enum Cyc_Absyn_Sign f1;long long f2;};struct _union_Cnst_LongLong_c{int tag;struct _tuple6 val;};struct _tuple7{struct _fat_ptr f1;int f2;};struct _union_Cnst_Float_c{int tag;struct _tuple7 val;};struct _union_Cnst_String_c{int tag;struct _fat_ptr val;};struct _union_Cnst_Wstring_c{int tag;struct _fat_ptr val;};union Cyc_Absyn_Cnst{struct _union_Cnst_Null_c Null_c;struct _union_Cnst_Char_c Char_c;struct _union_Cnst_Wchar_c Wchar_c;struct _union_Cnst_Short_c Short_c;struct _union_Cnst_Int_c Int_c;struct _union_Cnst_LongLong_c LongLong_c;struct _union_Cnst_Float_c Float_c;struct _union_Cnst_String_c String_c;struct _union_Cnst_Wstring_c Wstring_c;};
# 574
enum Cyc_Absyn_Primop{Cyc_Absyn_Plus =0U,Cyc_Absyn_Times =1U,Cyc_Absyn_Minus =2U,Cyc_Absyn_Div =3U,Cyc_Absyn_Mod =4U,Cyc_Absyn_Eq =5U,Cyc_Absyn_Neq =6U,Cyc_Absyn_Gt =7U,Cyc_Absyn_Lt =8U,Cyc_Absyn_Gte =9U,Cyc_Absyn_Lte =10U,Cyc_Absyn_Not =11U,Cyc_Absyn_Bitnot =12U,Cyc_Absyn_Bitand =13U,Cyc_Absyn_Bitor =14U,Cyc_Absyn_Bitxor =15U,Cyc_Absyn_Bitlshift =16U,Cyc_Absyn_Bitlrshift =17U,Cyc_Absyn_Numelts =18U};
# 581
enum Cyc_Absyn_Incrementor{Cyc_Absyn_PreInc =0U,Cyc_Absyn_PostInc =1U,Cyc_Absyn_PreDec =2U,Cyc_Absyn_PostDec =3U};struct Cyc_Absyn_VarargCallInfo{int num_varargs;struct Cyc_List_List*injectors;struct Cyc_Absyn_VarargInfo*vai;};struct Cyc_Absyn_StructField_Absyn_OffsetofField_struct{int tag;struct _fat_ptr*f1;};struct Cyc_Absyn_TupleIndex_Absyn_OffsetofField_struct{int tag;unsigned f1;};
# 599
enum Cyc_Absyn_Coercion{Cyc_Absyn_Unknown_coercion =0U,Cyc_Absyn_No_coercion =1U,Cyc_Absyn_Null_to_NonNull =2U,Cyc_Absyn_Other_coercion =3U};struct Cyc_Absyn_MallocInfo{int is_calloc;struct Cyc_Absyn_Exp*rgn;void**elt_type;struct Cyc_Absyn_Exp*num_elts;int fat_result;int inline_call;};struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct{int tag;union Cyc_Absyn_Cnst f1;};struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Pragma_e_Absyn_Raw_exp_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct{int tag;enum Cyc_Absyn_Primop f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;enum Cyc_Absyn_Incrementor f2;};struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct _tuple8{struct Cyc_List_List*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_VarargCallInfo*f3;int f4;struct _tuple8*f5;};struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;int f2;};struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Exp*f2;int f3;enum Cyc_Absyn_Coercion f4;};struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;int f3;int f4;};struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;int f3;int f4;};struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Tuple_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;};struct _tuple9{struct _fat_ptr*f1;struct Cyc_Absyn_Tqual f2;void*f3;};struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct{int tag;struct _tuple9*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;int f4;};struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;int f3;};struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct Cyc_Absyn_Aggrdecl*f4;};struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Datatypedecl*f2;struct Cyc_Absyn_Datatypefield*f3;};struct Cyc_Absyn_Enum_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_Spawn_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;struct _tuple8*f3;};struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_MallocInfo f1;};struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Core_Opt*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Stmt*f1;};struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;};struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct{int tag;int f1;struct _fat_ptr f2;struct Cyc_List_List*f3;struct Cyc_List_List*f4;struct Cyc_List_List*f5;};struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Exp{void*topt;void*r;unsigned loc;void*annot;};struct Cyc_Absyn_Skip_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;struct Cyc_Absyn_Stmt*f3;};struct _tuple10{struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct{int tag;struct _tuple10 f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Break_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Continue_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Goto_s_Absyn_Raw_stmt_struct{int tag;struct _fat_ptr*f1;};struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _tuple10 f2;struct _tuple10 f3;struct Cyc_Absyn_Stmt*f4;};struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;void*f3;};struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Switch_clause**f2;};struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Decl*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct{int tag;struct _fat_ptr*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct _tuple10 f2;};struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_List_List*f2;void*f3;};struct Cyc_Absyn_Stmt{void*r;unsigned loc;void*annot;};struct Cyc_Absyn_Wild_p_Absyn_Raw_pat_struct{int tag;};struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_AliasVar_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_TagInt_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Tuple_p_Absyn_Raw_pat_struct{int tag;struct Cyc_List_List*f1;int f2;};struct Cyc_Absyn_Pointer_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Pat*f1;};struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct{int tag;union Cyc_Absyn_AggrInfo*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;int f4;};struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;struct Cyc_List_List*f3;int f4;};struct Cyc_Absyn_Null_p_Absyn_Raw_pat_struct{int tag;};struct Cyc_Absyn_Int_p_Absyn_Raw_pat_struct{int tag;enum Cyc_Absyn_Sign f1;int f2;};struct Cyc_Absyn_Char_p_Absyn_Raw_pat_struct{int tag;char f1;};struct Cyc_Absyn_Float_p_Absyn_Raw_pat_struct{int tag;struct _fat_ptr f1;int f2;};struct Cyc_Absyn_Enum_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_AnonEnum_p_Absyn_Raw_pat_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_UnknownId_p_Absyn_Raw_pat_struct{int tag;struct _tuple0*f1;};struct Cyc_Absyn_UnknownCall_p_Absyn_Raw_pat_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;int f3;};struct Cyc_Absyn_Exp_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Pat{void*r;void*topt;unsigned loc;};struct Cyc_Absyn_Switch_clause{struct Cyc_Absyn_Pat*pattern;struct Cyc_Core_Opt*pat_vars;struct Cyc_Absyn_Exp*where_clause;struct Cyc_Absyn_Stmt*body;unsigned loc;};struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct{int tag;struct _tuple0*f1;};struct Cyc_Absyn_Global_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Funname_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Param_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Local_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Pat_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Vardecl{enum Cyc_Absyn_Scope sc;struct _tuple0*name;unsigned varloc;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*initializer;void*rgn;struct Cyc_List_List*attributes;int escapes;};struct Cyc_Absyn_Fndecl{enum Cyc_Absyn_Scope sc;int is_inline;struct _tuple0*name;struct Cyc_Absyn_Stmt*body;struct Cyc_Absyn_FnInfo i;void*cached_type;struct Cyc_Core_Opt*param_vardecls;struct Cyc_Absyn_Vardecl*fn_vardecl;};struct Cyc_Absyn_Aggrfield{struct _fat_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*width;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*requires_clause;};struct Cyc_Absyn_AggrdeclImpl{struct Cyc_List_List*exist_vars;struct Cyc_List_List*rgn_po;struct Cyc_List_List*fields;int tagged;};struct Cyc_Absyn_Aggrdecl{enum Cyc_Absyn_AggrKind kind;enum Cyc_Absyn_Scope sc;struct _tuple0*name;struct Cyc_List_List*tvs;struct Cyc_Absyn_AggrdeclImpl*impl;struct Cyc_List_List*attributes;int expected_mem_kind;};struct Cyc_Absyn_Datatypefield{struct _tuple0*name;struct Cyc_List_List*typs;unsigned loc;enum Cyc_Absyn_Scope sc;};struct Cyc_Absyn_Datatypedecl{enum Cyc_Absyn_Scope sc;struct _tuple0*name;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*fields;int is_extensible;};struct Cyc_Absyn_Enumfield{struct _tuple0*name;struct Cyc_Absyn_Exp*tag;unsigned loc;};struct Cyc_Absyn_Enumdecl{enum Cyc_Absyn_Scope sc;struct _tuple0*name;struct Cyc_Core_Opt*fields;};struct Cyc_Absyn_Typedefdecl{struct _tuple0*name;struct Cyc_Absyn_Tqual tq;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*kind;void*defn;struct Cyc_List_List*atts;int extern_c;};struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Pat*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;void*f4;};struct Cyc_Absyn_Letv_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;};struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;};struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Typedefdecl*f1;};struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct{int tag;struct _fat_ptr*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_ExternC_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;};struct _tuple11{unsigned f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct _tuple11*f4;};struct Cyc_Absyn_Porton_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Portoff_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Tempeston_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Tempestoff_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Decl{void*r;unsigned loc;};struct Cyc_Absyn_ArrayElement_Absyn_Designator_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_FieldName_Absyn_Designator_struct{int tag;struct _fat_ptr*f1;};extern char Cyc_Absyn_EmptyAnnot[11U];struct Cyc_Absyn_EmptyAnnot_Absyn_AbsynAnnot_struct{char*tag;};
# 956 "absyn.h"
int Cyc_Absyn_tvar_cmp(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*);
# 991
extern void*Cyc_Absyn_heap_rgn_type;
# 997
extern void*Cyc_Absyn_void_type;
# 1026
void*Cyc_Absyn_bounds_one();
# 1127
struct Cyc_Absyn_Stmt*Cyc_Absyn_new_stmt(void*,unsigned);
struct Cyc_Absyn_Stmt*Cyc_Absyn_skip_stmt(unsigned);
# 1219
int Cyc_Absyn_attribute_cmp(void*,void*);
# 1242
void Cyc_Absyn_visit_stmt(int(*)(void*,struct Cyc_Absyn_Exp*),int(*)(void*,struct Cyc_Absyn_Stmt*),void*,struct Cyc_Absyn_Stmt*);
# 1245
void Cyc_Absyn_visit_type(int(*f)(void*,void*),void*env,void*t);
# 1256
void Cyc_Absyn_visit_stmt_pop(int(*)(void*,struct Cyc_Absyn_Exp*),void(*)(void*,struct Cyc_Absyn_Exp*),int(*)(void*,struct Cyc_Absyn_Stmt*),void(*f)(void*,struct Cyc_Absyn_Stmt*),void*,struct Cyc_Absyn_Stmt*);
# 1277
struct _fat_ptr Cyc_Absyn_exnstr();
# 1280
struct Cyc_Absyn_Tvar*Cyc_Absyn_type2tvar(void*t);
# 1283
struct Cyc_List_List*Cyc_Absyn_add_qvar(struct Cyc_List_List*t,struct _tuple0*q);
# 1285
int Cyc_Absyn_get_debug();
# 1287
int Cyc_Absyn_is_xrgn_tvar(struct Cyc_Absyn_Tvar*tv);
int Cyc_Absyn_is_xrgn(void*r);
# 1290
struct _fat_ptr Cyc_Absyn_list2string(struct Cyc_List_List*l,struct _fat_ptr(*cb)(void*));
# 1295
void*Cyc_Absyn_new_nat_cap(int i,int b);
# 1300
int Cyc_Absyn_is_nat_cap(void*c);
# 1303
int Cyc_Absyn_get_nat_cap(void*c);
# 1316
void*Cyc_Absyn_rgn_cap(struct Cyc_List_List*c);struct _tuple12{int f1;int f2;int f3;};
# 1323
struct _tuple12 Cyc_Absyn_caps2tup(struct Cyc_List_List*);
struct Cyc_List_List*Cyc_Absyn_tup2caps(int a,int b,int c);
# 1326
void Cyc_Absyn_updatecaps_rgneffect(struct Cyc_Absyn_RgnEffect*r,struct Cyc_List_List*c);
struct Cyc_Absyn_RgnEffect*Cyc_Absyn_new_rgneffect(void*t1,struct Cyc_List_List*c,void*t2);
# 1338
struct Cyc_Absyn_RgnEffect*Cyc_Absyn_find_rgneffect(struct Cyc_Absyn_Tvar*x,struct Cyc_List_List*l);
# 1343
int Cyc_Absyn_equal_rgneffects(struct Cyc_List_List*l1,struct Cyc_List_List*l2);
# 1347
struct _fat_ptr Cyc_Absyn_effect2string(struct Cyc_List_List*f);
struct _fat_ptr Cyc_Absyn_rgneffect2string(struct Cyc_Absyn_RgnEffect*r);
struct Cyc_Absyn_RgnEffect*Cyc_Absyn_copy_rgneffect(struct Cyc_Absyn_RgnEffect*eff);
struct Cyc_List_List*Cyc_Absyn_copy_effect(struct Cyc_List_List*f);
void*Cyc_Absyn_rgneffect_name(struct Cyc_Absyn_RgnEffect*r);
void*Cyc_Absyn_rgneffect_parent(struct Cyc_Absyn_RgnEffect*r);
struct Cyc_List_List*Cyc_Absyn_rgneffect_caps(struct Cyc_Absyn_RgnEffect*r);
# 1362
int Cyc_Absyn_is_aliasable_rgneffect(struct Cyc_Absyn_RgnEffect*r);
# 1369
struct _tuple0*Cyc_Absyn_any_qvar();
# 1371
struct _tuple0*Cyc_Absyn_default_case_qvar();
# 1373
struct Cyc_List_List*Cyc_Absyn_throwsany();
int Cyc_Absyn_is_throwsany(struct Cyc_List_List*t);
# 1376
int Cyc_Absyn_is_nothrow(struct Cyc_List_List*t);
# 1379
int Cyc_Absyn_exists_throws(struct Cyc_List_List*throws,struct _tuple0*q);
struct _fat_ptr Cyc_Absyn_throws_qvar2string(struct _tuple0*q);
struct _fat_ptr Cyc_Absyn_throws2string(struct Cyc_List_List*t);
struct _tuple0*Cyc_Absyn_throws_hd2qvar(struct Cyc_List_List*t);
# 1385
struct _tuple0*Cyc_Absyn_get_qvar(enum Cyc_Absyn_DefExn de);
int Cyc_Absyn_is_exn_stmt(struct Cyc_Absyn_Stmt*s);
# 1399
int Cyc_Absyn_is_reentrant(int);
# 1401
extern const int Cyc_Absyn_reentrant;
# 1406
struct _fat_ptr Cyc_Absyn_typcon2string(void*t);
# 29 "warn.h"
void Cyc_Warn_warn(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap);
# 31
void Cyc_Warn_flush_warnings();
# 40
void*Cyc_Warn_impos(struct _fat_ptr fmt,struct _fat_ptr ap);struct Cyc_Warn_String_Warn_Warg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Warn_Exp_Warn_Warg_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Warn_Typ_Warn_Warg_struct{int tag;void*f1;};struct Cyc_Warn_Qvar_Warn_Warg_struct{int tag;struct _tuple0*f1;};struct Cyc_RgnOrder_RgnPO;extern char Cyc_Tcenv_Env_error[10U];struct Cyc_Tcenv_Env_error_exn_struct{char*tag;};struct Cyc_Tcenv_Genv{struct Cyc_Dict_Dict aggrdecls;struct Cyc_Dict_Dict datatypedecls;struct Cyc_Dict_Dict enumdecls;struct Cyc_Dict_Dict typedefs;struct Cyc_Dict_Dict ordinaries;};struct Cyc_Tcenv_Fenv;struct Cyc_Tcenv_Tenv{struct Cyc_List_List*ns;struct Cyc_Tcenv_Genv*ae;struct Cyc_Tcenv_Fenv*le;int allow_valueof: 1;int in_extern_c_include: 1;int in_tempest: 1;int tempest_generalize: 1;};
# 90 "tcenv.h"
enum Cyc_Tcenv_NewStatus{Cyc_Tcenv_NoneNew =0U,Cyc_Tcenv_InNew =1U,Cyc_Tcenv_InNewAggr =2U};struct Cyc_Tcpat_TcPatResult{struct _tuple8*tvars_and_bounds_opt;struct Cyc_List_List*patvars;};struct Cyc_Tcpat_WhereTest_Tcpat_PatTest_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Tcpat_EqNull_Tcpat_PatTest_struct{int tag;};struct Cyc_Tcpat_NeqNull_Tcpat_PatTest_struct{int tag;};struct Cyc_Tcpat_EqEnum_Tcpat_PatTest_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Tcpat_EqAnonEnum_Tcpat_PatTest_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Tcpat_EqFloat_Tcpat_PatTest_struct{int tag;struct _fat_ptr f1;int f2;};struct Cyc_Tcpat_EqConst_Tcpat_PatTest_struct{int tag;unsigned f1;};struct Cyc_Tcpat_EqDatatypeTag_Tcpat_PatTest_struct{int tag;int f1;struct Cyc_Absyn_Datatypedecl*f2;struct Cyc_Absyn_Datatypefield*f3;};struct Cyc_Tcpat_EqTaggedUnion_Tcpat_PatTest_struct{int tag;struct _fat_ptr*f1;int f2;};struct Cyc_Tcpat_EqExtensibleDatatype_Tcpat_PatTest_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;};struct Cyc_Tcpat_Dummy_Tcpat_Access_struct{int tag;};struct Cyc_Tcpat_Deref_Tcpat_Access_struct{int tag;};struct Cyc_Tcpat_TupleField_Tcpat_Access_struct{int tag;unsigned f1;};struct Cyc_Tcpat_DatatypeField_Tcpat_Access_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;unsigned f3;};struct Cyc_Tcpat_AggrField_Tcpat_Access_struct{int tag;int f1;struct _fat_ptr*f2;};struct _union_PatOrWhere_pattern{int tag;struct Cyc_Absyn_Pat*val;};struct _union_PatOrWhere_where_clause{int tag;struct Cyc_Absyn_Exp*val;};union Cyc_Tcpat_PatOrWhere{struct _union_PatOrWhere_pattern pattern;struct _union_PatOrWhere_where_clause where_clause;};struct Cyc_Tcpat_PathNode{union Cyc_Tcpat_PatOrWhere orig_pat;void*access;};struct Cyc_Tcpat_Rhs{int used;unsigned pat_loc;struct Cyc_Absyn_Stmt*rhs;};struct Cyc_Tcpat_Failure_Tcpat_Decision_struct{int tag;void*f1;};struct Cyc_Tcpat_Success_Tcpat_Decision_struct{int tag;struct Cyc_Tcpat_Rhs*f1;};struct Cyc_Tcpat_SwitchDec_Tcpat_Decision_struct{int tag;struct Cyc_List_List*f1;struct Cyc_List_List*f2;void*f3;};struct Cyc_PP_Ppstate;struct Cyc_PP_Out;struct Cyc_PP_Doc;struct Cyc_Absynpp_Params{int expand_typedefs;int qvar_to_Cids;int add_cyc_prefix;int to_VC;int decls_first;int rewrite_temp_tvars;int print_all_tvars;int print_all_kinds;int print_all_effects;int print_using_stmts;int print_externC_stmts;int print_full_evars;int print_zeroterm;int generate_line_directives;int use_curr_namespace;struct Cyc_List_List*curr_namespace;};
# 62 "absynpp.h"
struct _fat_ptr Cyc_Absynpp_typ2string(void*);
# 65
struct _fat_ptr Cyc_Absynpp_kindbound2string(void*);
# 67
struct _fat_ptr Cyc_Absynpp_exp2string(struct Cyc_Absyn_Exp*);
struct _fat_ptr Cyc_Absynpp_stmt2string(struct Cyc_Absyn_Stmt*);
struct _fat_ptr Cyc_Absynpp_qvar2string(struct _tuple0*);
# 72
struct _fat_ptr Cyc_Absynpp_pat2string(struct Cyc_Absyn_Pat*p);struct _union_RelnOp_RConst{int tag;unsigned val;};struct _union_RelnOp_RVar{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RNumelts{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RType{int tag;void*val;};struct _union_RelnOp_RParam{int tag;unsigned val;};struct _union_RelnOp_RParamNumelts{int tag;unsigned val;};struct _union_RelnOp_RReturn{int tag;unsigned val;};union Cyc_Relations_RelnOp{struct _union_RelnOp_RConst RConst;struct _union_RelnOp_RVar RVar;struct _union_RelnOp_RNumelts RNumelts;struct _union_RelnOp_RType RType;struct _union_RelnOp_RParam RParam;struct _union_RelnOp_RParamNumelts RParamNumelts;struct _union_RelnOp_RReturn RReturn;};
# 50 "relations-ap.h"
enum Cyc_Relations_Relation{Cyc_Relations_Req =0U,Cyc_Relations_Rneq =1U,Cyc_Relations_Rlte =2U,Cyc_Relations_Rlt =3U};struct Cyc_Relations_Reln{union Cyc_Relations_RelnOp rop1;enum Cyc_Relations_Relation relation;union Cyc_Relations_RelnOp rop2;};
# 31 "tcutil.h"
void Cyc_Tcutil_terr(unsigned,struct _fat_ptr,struct _fat_ptr);
# 53
int Cyc_Tcutil_is_zeroterm_pointer_type(void*);
# 75
struct Cyc_Absyn_Exp*Cyc_Tcutil_get_bounds_exp(void*def,void*b);
# 80
int Cyc_Tcutil_is_fat_pointer_type_elt(void*t,void**elt_dest);
# 82
int Cyc_Tcutil_is_zero_pointer_type_elt(void*t,void**elt_type_dest);
# 87
struct Cyc_Absyn_Exp*Cyc_Tcutil_get_bounds_exp(void*def,void*b);
# 103
int Cyc_Tcutil_kind_leq(struct Cyc_Absyn_Kind*,struct Cyc_Absyn_Kind*);
int Cyc_Tcutil_kind_eq(struct Cyc_Absyn_Kind*,struct Cyc_Absyn_Kind*);
# 109
struct Cyc_Absyn_Kind*Cyc_Tcutil_type_kind(void*);
void*Cyc_Tcutil_compress(void*);
# 138
extern struct Cyc_Absyn_Kind Cyc_Tcutil_xrk;
# 148
extern struct Cyc_Absyn_Kind Cyc_Tcutil_trk;
# 178
void*Cyc_Tcutil_kind_to_bound(struct Cyc_Absyn_Kind*k);
# 233
int Cyc_Tcutil_is_zero_ptr_deref(struct Cyc_Absyn_Exp*,void**ptr_type,int*is_fat,void**elt_type);
# 261
struct Cyc_Absyn_Tvar*Cyc_Tcutil_new_tvar(void*);
# 329
void*Cyc_Tcutil_rgns_of(void*t);struct Cyc_JumpAnalysis_Jump_Anal_Result{struct Cyc_Hashtable_Table*pop_tables;struct Cyc_Hashtable_Table*succ_tables;struct Cyc_Hashtable_Table*pat_pop_tables;};extern char Cyc_InsertChecks_FatBound[9U];struct Cyc_InsertChecks_FatBound_Absyn_AbsynAnnot_struct{char*tag;};extern char Cyc_InsertChecks_NoCheck[8U];struct Cyc_InsertChecks_NoCheck_Absyn_AbsynAnnot_struct{char*tag;};extern char Cyc_InsertChecks_NullAndFatBound[16U];struct Cyc_InsertChecks_NullAndFatBound_Absyn_AbsynAnnot_struct{char*tag;};extern char Cyc_InsertChecks_NullAndThinBound[17U];struct Cyc_InsertChecks_NullAndThinBound_Absyn_AbsynAnnot_struct{char*tag;struct Cyc_Absyn_Exp*f1;};extern char Cyc_InsertChecks_NullOnly[9U];struct Cyc_InsertChecks_NullOnly_Absyn_AbsynAnnot_struct{char*tag;};extern char Cyc_InsertChecks_ThinBound[10U];struct Cyc_InsertChecks_ThinBound_Absyn_AbsynAnnot_struct{char*tag;struct Cyc_Absyn_Exp*f1;};struct _tuple13{unsigned f1;int f2;};
# 28 "evexp.h"
struct _tuple13 Cyc_Evexp_eval_const_uint_exp(struct Cyc_Absyn_Exp*e);extern char Cyc_Toc_Dest[5U];struct Cyc_Toc_Dest_Absyn_AbsynAnnot_struct{char*tag;struct Cyc_Absyn_Exp*f1;};
# 46 "toc.h"
int Cyc_Toc_is_zero(struct Cyc_Absyn_Exp*e);
int Cyc_Toc_do_null_check(struct Cyc_Absyn_Exp*e);
int Cyc_Toc_is_tagged_union_project(struct Cyc_Absyn_Exp*e,int*f_tag,void**tagged_member_type,int clear_read);
# 52
void Cyc_Toc_init_toc_state();
# 27 "ioeffect.h"
void Cyc_IOEffect_analysis(struct Cyc_JumpAnalysis_Jump_Anal_Result*tables,struct Cyc_List_List*tds);
struct Cyc_List_List*Cyc_IOEffect_summarize_all(unsigned loc,struct Cyc_List_List*f);struct _tuple14{struct _tuple0*f1;struct Cyc_List_List*f2;int f3;};struct Cyc_IOEffect_Enclosed{struct Cyc_List_List*labels;};struct Cyc_IOEffect_Fenv{struct Cyc_List_List**throws;struct Cyc_List_List*annot;struct Cyc_Hashtable_Table*nodes;struct Cyc_List_List*catch_stack;struct Cyc_Absyn_Fndecl*fd;struct Cyc_List_List*enclosing_xrgn;struct Cyc_Hashtable_Table*enclosed_map;struct Cyc_Absyn_Stmt*exit_stmt;struct Cyc_List_List*labels;int ignore_xrgn;};struct Cyc_IOEffect_Exp_IOEffect_NodeType_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_IOEffect_Stmt_IOEffect_NodeType_struct{int tag;struct Cyc_Absyn_Stmt*f1;};
# 95 "ioeffect.cyc"
enum Cyc_IOEffect_NodeAnnot{Cyc_IOEffect_MayJump =0,Cyc_IOEffect_MustJump =1};struct Cyc_IOEffect_Node{void*n;enum Cyc_IOEffect_NodeAnnot annot;unsigned loc;struct Cyc_List_List*input;struct Cyc_List_List*output;struct Cyc_List_List*throws;struct Cyc_List_List*succ;};struct Cyc_IOEffect_Env{struct Cyc_JumpAnalysis_Jump_Anal_Result*tables;struct Cyc_IOEffect_Fenv*fenv;struct Cyc_List_List*annot;int const_effect;struct Cyc_List_List*jlist;};char Cyc_IOEffect_IOEffect_error[15U]="IOEffect_error";struct Cyc_IOEffect_IOEffect_error_exn_struct{char*tag;};
# 130
struct Cyc_IOEffect_IOEffect_error_exn_struct Cyc_IOEffect_IOEffect_error_val={Cyc_IOEffect_IOEffect_error};
# 134
struct Cyc_Absyn_TupleType_Absyn_Type_struct Cyc_IOEffect_dummy_type={6U,0};
struct Cyc_Absyn_RgnEffect Cyc_IOEffect_dummy_rgneffect={(void*)& Cyc_IOEffect_dummy_type,0,0};
struct Cyc_List_List Cyc_IOEffect_empty_effect={(void*)& Cyc_IOEffect_dummy_rgneffect,0};
# 140
static struct _tuple8 Cyc_IOEffect_split_list(int(*pred)(void*,void*),void*env,struct Cyc_List_List*l,struct _RegionHandle*h){
# 145
struct Cyc_List_List*fst=0;struct Cyc_List_List*snd=0;struct Cyc_List_List**z=0;
for(0;l != 0;l=l->tl){
z=({pred(env,l->hd);})?& fst:& snd;({struct Cyc_List_List*_tmp606=({struct Cyc_List_List*_tmp0=_region_malloc(h,sizeof(*_tmp0));_tmp0->hd=l->hd,_tmp0->tl=*((struct Cyc_List_List**)_check_null(z));_tmp0;});*z=_tmp606;});}
return({struct _tuple8 _tmp563;({struct Cyc_List_List*_tmp608=({Cyc_List_rrev(h,fst);});_tmp563.f1=_tmp608;}),({struct Cyc_List_List*_tmp607=({Cyc_List_rrev(h,snd);});_tmp563.f2=_tmp607;});_tmp563;});}
# 150
static struct Cyc_List_List*Cyc_IOEffect_cond_map_list(void*(*pred)(void*,void*),void*env,struct Cyc_List_List*l,struct _RegionHandle*h){
# 155
if(l == 0)return 0;{struct Cyc_List_List*hd;struct Cyc_List_List*tl;
# 157
void*p=0;
for(0;l != 0 && p == 0;(p=({pred(env,l->hd);}),l=l->tl)){;}
if(p == 0)return 0;hd=(tl=({struct Cyc_List_List*_tmp2=_cycalloc(sizeof(*_tmp2));
_tmp2->hd=p,_tmp2->tl=0;_tmp2;}));
for(0;l != 0;l=l->tl){
# 163
p=({pred(env,l->hd);});
if(p != 0)tl=({struct Cyc_List_List*_tmp609=({struct Cyc_List_List*_tmp3=_cycalloc(sizeof(*_tmp3));_tmp3->hd=p,_tmp3->tl=0;_tmp3;});((struct Cyc_List_List*)_check_null(tl))->tl=_tmp609;});}
# 166
return hd;}}
# 171
static int Cyc_IOEffect_is_heaprgn(void*r){return r == Cyc_Absyn_heap_rgn_type;}struct _tuple15{void*f1;void*f2;};
# 173
static int Cyc_IOEffect_rgn_cmp(void*r1,void*r2){
# 175
if(({Cyc_IOEffect_is_heaprgn(r1);}))return({Cyc_IOEffect_is_heaprgn(r2);});if(({Cyc_IOEffect_is_heaprgn(r2);}))
return({Cyc_IOEffect_is_heaprgn(r1);});{
# 175
struct _tuple15 _stmttmp0=({struct _tuple15 _tmp566;_tmp566.f1=r1,_tmp566.f2=r2;_tmp566;});struct _tuple15 _tmp6=_stmttmp0;struct Cyc_Absyn_Tvar*_tmp8;struct Cyc_Absyn_Tvar*_tmp7;if(((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp6.f1)->tag == 2U){if(((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp6.f2)->tag == 2U){_LL1: _tmp7=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp6.f1)->f1;_tmp8=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp6.f2)->f1;_LL2: {struct Cyc_Absyn_Tvar*vt1=_tmp7;struct Cyc_Absyn_Tvar*vt2=_tmp8;
# 179
return({Cyc_Absyn_tvar_cmp(vt1,vt2);})== 0;}}else{goto _LL3;}}else{_LL3: _LL4:
# 181
({struct Cyc_String_pa_PrintArg_struct _tmpC=({struct Cyc_String_pa_PrintArg_struct _tmp565;_tmp565.tag=0U,({
struct _fat_ptr _tmp60A=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(r1);}));_tmp565.f1=_tmp60A;});_tmp565;});struct Cyc_String_pa_PrintArg_struct _tmpD=({struct Cyc_String_pa_PrintArg_struct _tmp564;_tmp564.tag=0U,({struct _fat_ptr _tmp60B=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(r2);}));_tmp564.f1=_tmp60B;});_tmp564;});void*_tmp9[2U];_tmp9[0]=& _tmpC,_tmp9[1]=& _tmpD;({int(*_tmp60D)(struct _fat_ptr fmt,struct _fat_ptr ap)=({
# 181
int(*_tmpB)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmpB;});struct _fat_ptr _tmp60C=({const char*_tmpA="IOEffect::rgn_cmp %s and %s";_tag_fat(_tmpA,sizeof(char),28U);});_tmp60D(_tmp60C,_tag_fat(_tmp9,sizeof(void*),2U));});});}_LL0:;}}
# 186
static struct Cyc_Absyn_RgnEffect*Cyc_IOEffect_find_effect(void*r1,struct Cyc_List_List*l){
# 188
void*_stmttmp1=({Cyc_Tcutil_compress(r1);});void*_tmpF=_stmttmp1;struct Cyc_Absyn_Tvar*_tmp10;if(((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmpF)->tag == 2U){_LL1: _tmp10=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmpF)->f1;_LL2: {struct Cyc_Absyn_Tvar*vt1=_tmp10;
# 190
return({Cyc_Absyn_find_rgneffect(vt1,l);});}}else{_LL3: _LL4:
({void*_tmp11=0U;({int(*_tmp60F)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp13)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp13;});struct _fat_ptr _tmp60E=({const char*_tmp12="IOEffect::find_effect";_tag_fat(_tmp12,sizeof(char),22U);});_tmp60F(_tmp60E,_tag_fat(_tmp11,sizeof(void*),0U));});});}_LL0:;}
# 196
static struct Cyc_Absyn_RgnEffect*Cyc_IOEffect_copyeffelt(struct Cyc_Absyn_RgnEffect*f){
return({Cyc_Absyn_copy_rgneffect(f);});}
# 199
static struct Cyc_List_List*Cyc_IOEffect_new_rgn_effect(void*r,void*rpar,struct Cyc_List_List*f){
# 201
struct Cyc_List_List*cp=({struct Cyc_List_List*_tmp18=_cycalloc(sizeof(*_tmp18));({void*_tmp612=({Cyc_Absyn_new_nat_cap(1,0);});_tmp18->hd=_tmp612;}),({
struct Cyc_List_List*_tmp611=({struct Cyc_List_List*_tmp17=_cycalloc(sizeof(*_tmp17));({void*_tmp610=({Cyc_Absyn_new_nat_cap(1,0);});_tmp17->hd=_tmp610;}),_tmp17->tl=0;_tmp17;});_tmp18->tl=_tmp611;});_tmp18;});
return({struct Cyc_List_List*_tmp16=_cycalloc(sizeof(*_tmp16));({struct Cyc_Absyn_RgnEffect*_tmp613=({Cyc_Absyn_new_rgneffect(r,cp,rpar);});_tmp16->hd=_tmp613;}),_tmp16->tl=f;_tmp16;});}
# 206
void*Cyc_IOEffect_env_err(struct _fat_ptr msg){
# 208
({struct Cyc_String_pa_PrintArg_struct _tmp1C=({struct Cyc_String_pa_PrintArg_struct _tmp567;_tmp567.tag=0U,_tmp567.f1=(struct _fat_ptr)((struct _fat_ptr)msg);_tmp567;});void*_tmp1A[1U];_tmp1A[0]=& _tmp1C;({struct _fat_ptr _tmp614=({const char*_tmp1B="\n%s\n";_tag_fat(_tmp1B,sizeof(char),5U);});Cyc_printf(_tmp614,_tag_fat(_tmp1A,sizeof(void*),1U));});});({Cyc_fflush(Cyc_stdout);});
(int)_throw(& Cyc_IOEffect_IOEffect_error_val);}
# 212
void*Cyc_IOEffect_safe_cast(void*ptr,struct _fat_ptr msg){
# 214
if(ptr == 0)
({struct Cyc_String_pa_PrintArg_struct _tmp21=({struct Cyc_String_pa_PrintArg_struct _tmp568;_tmp568.tag=0U,_tmp568.f1=(struct _fat_ptr)((struct _fat_ptr)msg);_tmp568;});void*_tmp1E[1U];_tmp1E[0]=& _tmp21;({int(*_tmp616)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp20)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp20;});struct _fat_ptr _tmp615=({const char*_tmp1F="IOEffect::safe_cast: \"%s\"";_tag_fat(_tmp1F,sizeof(char),26U);});_tmp616(_tmp615,_tag_fat(_tmp1E,sizeof(void*),1U));});});else{
return ptr;}}
# 212
struct _fat_ptr Cyc_IOEffect_annot2string(void*x){
# 228 "ioeffect.cyc"
void*_tmp23=x;struct Cyc_Absyn_Exp*_tmp24;struct Cyc_Absyn_Exp*_tmp25;if(((struct Cyc_InsertChecks_NoCheck_Absyn_AbsynAnnot_struct*)_tmp23)->tag == Cyc_InsertChecks_NoCheck){_LL1: _LL2:
# 230
 return({const char*_tmp26="no check";_tag_fat(_tmp26,sizeof(char),9U);});}else{if(((struct Cyc_InsertChecks_NullOnly_Absyn_AbsynAnnot_struct*)_tmp23)->tag == Cyc_InsertChecks_NullOnly){_LL3: _LL4:
 return({const char*_tmp27="null check";_tag_fat(_tmp27,sizeof(char),11U);});}else{if(((struct Cyc_InsertChecks_NullAndFatBound_Absyn_AbsynAnnot_struct*)_tmp23)->tag == Cyc_InsertChecks_NullAndFatBound){_LL5: _LL6:
 return({const char*_tmp28="null and bound";_tag_fat(_tmp28,sizeof(char),15U);});}else{if(((struct Cyc_InsertChecks_FatBound_Absyn_AbsynAnnot_struct*)_tmp23)->tag == Cyc_InsertChecks_FatBound){_LL7: _LL8:
 return({const char*_tmp29="bound check";_tag_fat(_tmp29,sizeof(char),12U);});}else{if(((struct Cyc_InsertChecks_NullAndThinBound_Absyn_AbsynAnnot_struct*)_tmp23)->tag == Cyc_InsertChecks_NullAndThinBound){_LL9: _tmp25=((struct Cyc_InsertChecks_NullAndThinBound_Absyn_AbsynAnnot_struct*)_tmp23)->f1;_LLA: {struct Cyc_Absyn_Exp*e=_tmp25;
# 235
return(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp2C=({struct Cyc_String_pa_PrintArg_struct _tmp569;_tmp569.tag=0U,({struct _fat_ptr _tmp617=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_exp2string(e);}));_tmp569.f1=_tmp617;});_tmp569;});void*_tmp2A[1U];_tmp2A[0]=& _tmp2C;({struct _fat_ptr _tmp618=({const char*_tmp2B="null and thin bound of %s";_tag_fat(_tmp2B,sizeof(char),26U);});Cyc_aprintf(_tmp618,_tag_fat(_tmp2A,sizeof(void*),1U));});});}}else{if(((struct Cyc_InsertChecks_ThinBound_Absyn_AbsynAnnot_struct*)_tmp23)->tag == Cyc_InsertChecks_ThinBound){_LLB: _tmp24=((struct Cyc_InsertChecks_ThinBound_Absyn_AbsynAnnot_struct*)_tmp23)->f1;_LLC: {struct Cyc_Absyn_Exp*e=_tmp24;
# 237
return(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp2F=({struct Cyc_String_pa_PrintArg_struct _tmp56A;_tmp56A.tag=0U,({struct _fat_ptr _tmp619=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_exp2string(e);}));_tmp56A.f1=_tmp619;});_tmp56A;});void*_tmp2D[1U];_tmp2D[0]=& _tmp2F;({struct _fat_ptr _tmp61A=({const char*_tmp2E="thin bound of %s";_tag_fat(_tmp2E,sizeof(char),17U);});Cyc_aprintf(_tmp61A,_tag_fat(_tmp2D,sizeof(void*),1U));});});}}else{_LLD: _LLE:
# 239
 return({const char*_tmp30="unknown annotation";_tag_fat(_tmp30,sizeof(char),19U);});}}}}}}_LL0:;}struct _tuple16{struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Exp*f2;};
# 245
struct _tuple16 Cyc_IOEffect_decl2xrgn(struct Cyc_Absyn_Decl*d){
# 247
void*_stmttmp2=d->r;void*_tmp32=_stmttmp2;struct Cyc_Absyn_Exp*_tmp34;struct Cyc_Absyn_Tvar*_tmp33;if(((struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*)_tmp32)->tag == 4U){_LL1: _tmp33=((struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*)_tmp32)->f1;_tmp34=((struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*)_tmp32)->f3;if(
# 250
_tmp34 != 0 &&(int)_tmp34->loc < 0){_LL2: {struct Cyc_Absyn_Tvar*tv=_tmp33;struct Cyc_Absyn_Exp*eo=_tmp34;
return({struct _tuple16 _tmp56B;_tmp56B.f1=tv,_tmp56B.f2=eo;_tmp56B;});}}else{goto _LL3;}}else{_LL3: _LL4:
({({int(*_tmp61B)(struct _fat_ptr msg)=({int(*_tmp35)(struct _fat_ptr msg)=(int(*)(struct _fat_ptr msg))Cyc_IOEffect_env_err;_tmp35;});_tmp61B(({const char*_tmp36="decl2xrgn";_tag_fat(_tmp36,sizeof(char),10U);}));});});}_LL0:;}struct _tuple17{struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Stmt*f3;};
# 257
struct _tuple17 Cyc_IOEffect_stmt2xrgn(struct Cyc_Absyn_Stmt*s){
# 259
void*_stmttmp3=s->r;void*_tmp38=_stmttmp3;struct Cyc_Absyn_Stmt*_tmp3A;struct Cyc_Absyn_Decl*_tmp39;if(((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp38)->tag == 12U){_LL1: _tmp39=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp38)->f1;_tmp3A=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp38)->f2;_LL2: {struct Cyc_Absyn_Decl*d=_tmp39;struct Cyc_Absyn_Stmt*s1=_tmp3A;
# 262
void*_stmttmp4=d->r;void*_tmp3B=_stmttmp4;struct Cyc_Absyn_Exp*_tmp3D;struct Cyc_Absyn_Tvar*_tmp3C;if(((struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*)_tmp3B)->tag == 4U){_LL6: _tmp3C=((struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*)_tmp3B)->f1;_tmp3D=((struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*)_tmp3B)->f3;if(
# 265
_tmp3D != 0 &&(int)_tmp3D->loc < 0){_LL7: {struct Cyc_Absyn_Tvar*tv=_tmp3C;struct Cyc_Absyn_Exp*eo=_tmp3D;
return({struct _tuple17 _tmp56C;_tmp56C.f1=tv,_tmp56C.f2=eo,_tmp56C.f3=s1;_tmp56C;});}}else{goto _LL8;}}else{_LL8: _LL9:
({({int(*_tmp61C)(struct _fat_ptr msg)=({int(*_tmp3E)(struct _fat_ptr msg)=(int(*)(struct _fat_ptr msg))Cyc_IOEffect_env_err;_tmp3E;});_tmp61C(({const char*_tmp3F="stmt2xrgn";_tag_fat(_tmp3F,sizeof(char),10U);}));});});}_LL5:;}}else{_LL3: _LL4:
# 269
({({int(*_tmp61D)(struct _fat_ptr msg)=({int(*_tmp40)(struct _fat_ptr msg)=(int(*)(struct _fat_ptr msg))Cyc_IOEffect_env_err;_tmp40;});_tmp61D(({const char*_tmp41="stmt2xrn";_tag_fat(_tmp41,sizeof(char),9U);}));});});}_LL0:;}
# 274
static int Cyc_IOEffect_hash_ptr(void*s){return(int)s;}
static struct Cyc_Hashtable_Table*Cyc_IOEffect_make_ptr_table(){
return({Cyc_Hashtable_create(33,Cyc_Core_ptrcmp,Cyc_IOEffect_hash_ptr);});}
# 278
static struct Cyc_Absyn_FnInfo Cyc_IOEffect_fn_type(void*t){
# 280
void*_tmp45=t;struct Cyc_Absyn_FnInfo _tmp46;struct Cyc_Absyn_FnInfo _tmp47;struct Cyc_Absyn_FnInfo _tmp48;struct Cyc_Absyn_FnInfo _tmp49;switch(*((int*)_tmp45)){case 5U: _LL1: _tmp49=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp45)->f1;_LL2: {struct Cyc_Absyn_FnInfo i=_tmp49;
# 282
return i;}case 3U: if(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp45)->f1).elt_type)->tag == 5U){_LL3: _tmp48=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp45)->f1).elt_type)->f1;_LL4: {struct Cyc_Absyn_FnInfo i=_tmp48;
return i;}}else{goto _LL9;}case 8U: if(((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp45)->f4 != 0){if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp45)->f4)->tag == 3U){if(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp45)->f4)->f1).elt_type)->tag == 5U){_LL5: _tmp47=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp45)->f4)->f1).elt_type)->f1;_LL6: {struct Cyc_Absyn_FnInfo i=_tmp47;
# 285
return i;}}else{goto _LL9;}}else{goto _LL9;}}else{goto _LL9;}case 1U: if(((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp45)->f2 != 0){if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp45)->f2)->tag == 3U){if(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp45)->f2)->f1).elt_type)->tag == 5U){_LL7: _tmp46=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp45)->f2)->f1).elt_type)->f1;_LL8: {struct Cyc_Absyn_FnInfo i=_tmp46;
return i;}}else{goto _LL9;}}else{goto _LL9;}}else{goto _LL9;}default: _LL9: _LLA:
# 288
({int(*_tmp4A)(struct _fat_ptr msg)=({int(*_tmp4B)(struct _fat_ptr msg)=(int(*)(struct _fat_ptr msg))Cyc_IOEffect_env_err;_tmp4B;});struct _fat_ptr _tmp4C=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp4F=({struct Cyc_String_pa_PrintArg_struct _tmp56E;_tmp56E.tag=0U,({
struct _fat_ptr _tmp61E=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t);}));_tmp56E.f1=_tmp61E;});_tmp56E;});struct Cyc_String_pa_PrintArg_struct _tmp50=({struct Cyc_String_pa_PrintArg_struct _tmp56D;_tmp56D.tag=0U,({struct _fat_ptr _tmp61F=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_typcon2string(t);}));_tmp56D.f1=_tmp61F;});_tmp56D;});void*_tmp4D[2U];_tmp4D[0]=& _tmp4F,_tmp4D[1]=& _tmp50;({struct _fat_ptr _tmp620=({const char*_tmp4E="IOEffect::fn_type %s => %s";_tag_fat(_tmp4E,sizeof(char),27U);});Cyc_aprintf(_tmp620,_tag_fat(_tmp4D,sizeof(void*),2U));});});_tmp4A(_tmp4C);});}_LL0:;}
# 298
static struct Cyc_List_List*Cyc_IOEffect_fn_throw_annot(void*t){
# 300
struct Cyc_List_List*z=({Cyc_IOEffect_fn_type(t);}).throws;
return z != 0?z:({Cyc_Absyn_throwsany();});}
# 298
int Cyc_IOEffect_cmp_effect(struct Cyc_List_List*f1,struct Cyc_List_List*f2){
# 307
return f1 == f2 ||({Cyc_Absyn_equal_rgneffects(f1,f2);});}
# 298
int Cyc_IOEffect_cmp(void*a,void*b){
# 310
return a == b;}
# 314
static struct Cyc_IOEffect_Fenv*Cyc_IOEffect_new_fenv(struct Cyc_Absyn_Fndecl*fd,unsigned loc,struct Cyc_Absyn_Stmt*s){
# 316
void*ftyp=(void*)_check_null(fd->cached_type);
return({struct Cyc_IOEffect_Fenv*_tmp56=_cycalloc(sizeof(*_tmp56));
({
struct Cyc_List_List**_tmp624=({struct Cyc_List_List**_tmp55=_cycalloc(sizeof(*_tmp55));*_tmp55=0;_tmp55;});_tmp56->throws=_tmp624;}),({
struct Cyc_List_List*_tmp623=({Cyc_IOEffect_fn_throw_annot(ftyp);});_tmp56->annot=_tmp623;}),({
struct Cyc_Hashtable_Table*_tmp622=({Cyc_IOEffect_make_ptr_table();});_tmp56->nodes=_tmp622;}),_tmp56->catch_stack=0,_tmp56->fd=fd,_tmp56->enclosing_xrgn=0,({
# 325
struct Cyc_Hashtable_Table*_tmp621=({Cyc_IOEffect_make_ptr_table();});_tmp56->enclosed_map=_tmp621;}),_tmp56->exit_stmt=s,_tmp56->labels=0,_tmp56->ignore_xrgn=0;_tmp56;});}
# 332
inline static void Cyc_IOEffect_set_ignore_xrgn(struct Cyc_IOEffect_Fenv*f,int b){
f->ignore_xrgn=b;}
# 336
inline static int Cyc_IOEffect_should_ignore_xrgn(struct Cyc_IOEffect_Fenv*f){
return f->ignore_xrgn;}
# 354 "ioeffect.cyc"
static struct Cyc_Absyn_Stmt*Cyc_IOEffect_exit_stmt(struct Cyc_IOEffect_Fenv*f){return f->exit_stmt;}
# 356
static struct Cyc_IOEffect_Enclosed*Cyc_IOEffect_get_enclosed(struct Cyc_IOEffect_Fenv*env,struct Cyc_Absyn_Decl*d){
# 358
struct Cyc_IOEffect_Enclosed**x=({({struct Cyc_IOEffect_Enclosed**(*_tmp626)(struct Cyc_Hashtable_Table*t,struct Cyc_Absyn_Decl*key)=({struct Cyc_IOEffect_Enclosed**(*_tmp5D)(struct Cyc_Hashtable_Table*t,struct Cyc_Absyn_Decl*key)=(struct Cyc_IOEffect_Enclosed**(*)(struct Cyc_Hashtable_Table*t,struct Cyc_Absyn_Decl*key))Cyc_Hashtable_lookup_opt;_tmp5D;});struct Cyc_Hashtable_Table*_tmp625=env->enclosed_map;_tmp626(_tmp625,d);});});
if(x == 0){
# 361
struct Cyc_IOEffect_Enclosed*n=({struct Cyc_IOEffect_Enclosed*_tmp5C=_cycalloc(sizeof(*_tmp5C));_tmp5C->labels=0;_tmp5C;});
({({void(*_tmp629)(struct Cyc_Hashtable_Table*t,struct Cyc_Absyn_Decl*key,struct Cyc_IOEffect_Enclosed*val)=({void(*_tmp5B)(struct Cyc_Hashtable_Table*t,struct Cyc_Absyn_Decl*key,struct Cyc_IOEffect_Enclosed*val)=(void(*)(struct Cyc_Hashtable_Table*t,struct Cyc_Absyn_Decl*key,struct Cyc_IOEffect_Enclosed*val))Cyc_Hashtable_insert;_tmp5B;});struct Cyc_Hashtable_Table*_tmp628=env->enclosed_map;struct Cyc_Absyn_Decl*_tmp627=d;_tmp629(_tmp628,_tmp627,n);});});
return n;}else{
# 365
return*x;}}
# 368
static void Cyc_IOEffect_push_label(struct Cyc_IOEffect_Fenv*fenv,struct _fat_ptr*v){
# 370
({struct Cyc_List_List*_tmp62A=({struct Cyc_List_List*_tmp5F=_cycalloc(sizeof(*_tmp5F));_tmp5F->hd=v,_tmp5F->tl=fenv->labels;_tmp5F;});fenv->labels=_tmp62A;});}
# 373
static void Cyc_IOEffect_pop_label(struct Cyc_IOEffect_Fenv*fenv){
# 375
if(fenv->labels == 0)({({int(*_tmp62B)(struct _fat_ptr msg)=({int(*_tmp61)(struct _fat_ptr msg)=(int(*)(struct _fat_ptr msg))Cyc_IOEffect_env_err;_tmp61;});_tmp62B(({const char*_tmp62="pop_label";_tag_fat(_tmp62,sizeof(char),10U);}));});});fenv->labels=((struct Cyc_List_List*)_check_null(fenv->labels))->tl;}struct _tuple18{struct _fat_ptr*f1;struct Cyc_Absyn_Stmt*f2;};
# 380
static void Cyc_IOEffect_add_enclosed_label(struct Cyc_IOEffect_Fenv*env,struct _fat_ptr*v,struct Cyc_Absyn_Stmt*s){
# 382
struct Cyc_List_List*iter=env->enclosing_xrgn;
for(0;iter != 0;iter=iter->tl){
# 385
struct Cyc_IOEffect_Enclosed*enc=({Cyc_IOEffect_get_enclosed(env,(struct Cyc_Absyn_Decl*)iter->hd);});
({struct Cyc_List_List*_tmp62D=({struct Cyc_List_List*_tmp65=_cycalloc(sizeof(*_tmp65));({struct _tuple18*_tmp62C=({struct _tuple18*_tmp64=_cycalloc(sizeof(*_tmp64));_tmp64->f1=v,_tmp64->f2=s;_tmp64;});_tmp65->hd=_tmp62C;}),_tmp65->tl=enc->labels;_tmp65;});enc->labels=_tmp62D;});}}
# 401 "ioeffect.cyc"
static void Cyc_IOEffect_push_enclosing_xrgn(struct Cyc_IOEffect_Env*env,struct Cyc_Absyn_Decl*s){
# 403
struct Cyc_List_List*tl=(env->fenv)->enclosing_xrgn;
({struct Cyc_List_List*_tmp62E=({struct Cyc_List_List*_tmp67=_cycalloc(sizeof(*_tmp67));_tmp67->hd=s,_tmp67->tl=tl;_tmp67;});(env->fenv)->enclosing_xrgn=_tmp62E;});}
# 407
static void Cyc_IOEffect_pop_enclosing_xrgn(struct Cyc_IOEffect_Env*env){
# 409
struct Cyc_List_List*tl=(env->fenv)->enclosing_xrgn;
if(tl == 0)({({int(*_tmp62F)(struct _fat_ptr msg)=({int(*_tmp69)(struct _fat_ptr msg)=(int(*)(struct _fat_ptr msg))Cyc_IOEffect_env_err;_tmp69;});_tmp62F(({const char*_tmp6A="pop_enclosing_xrgn";_tag_fat(_tmp6A,sizeof(char),19U);}));});});(env->fenv)->enclosing_xrgn=tl->tl;}struct _tuple19{struct Cyc_Absyn_Stmt*f1;void*f2;};
# 414
static void Cyc_IOEffect_push_catch_block(struct Cyc_IOEffect_Env*env,struct Cyc_Absyn_Stmt*s,void*d){
# 416
struct Cyc_IOEffect_Fenv*fenv=env->fenv;
({struct Cyc_List_List*_tmp631=({struct Cyc_List_List*_tmp6D=_cycalloc(sizeof(*_tmp6D));({struct _tuple19*_tmp630=({struct _tuple19*_tmp6C=_cycalloc(sizeof(*_tmp6C));_tmp6C->f1=s,_tmp6C->f2=d;_tmp6C;});_tmp6D->hd=_tmp630;}),_tmp6D->tl=fenv->catch_stack;_tmp6D;});fenv->catch_stack=_tmp631;});}
# 420
static void Cyc_IOEffect_cond_pop_catch_block(struct Cyc_IOEffect_Env*env,struct Cyc_Absyn_Stmt*s){
# 422
struct Cyc_IOEffect_Fenv*fenv=env->fenv;
if(fenv->catch_stack == 0)return;{struct Cyc_List_List*l=fenv->catch_stack;
# 425
if((*((struct _tuple19*)((struct Cyc_List_List*)_check_null(l))->hd)).f1 == s)fenv->catch_stack=l->tl;}}
# 420
static struct Cyc_IOEffect_Node*Cyc_IOEffect_get_stmt(struct Cyc_IOEffect_Fenv*env,struct Cyc_Absyn_Stmt*s);
# 463 "ioeffect.cyc"
static struct _fat_ptr Cyc_IOEffect_node2string(struct Cyc_IOEffect_Node*n){
struct Cyc_IOEffect_Node*_tmp70=n;struct Cyc_Absyn_Exp*_tmp71;struct Cyc_Absyn_Stmt*_tmp72;if(((struct Cyc_IOEffect_Stmt_IOEffect_NodeType_struct*)((struct Cyc_IOEffect_Node*)_tmp70)->n)->tag == 1U){_LL1: _tmp72=((struct Cyc_IOEffect_Stmt_IOEffect_NodeType_struct*)_tmp70->n)->f1;_LL2: {struct Cyc_Absyn_Stmt*s=_tmp72;
return({Cyc_Absynpp_stmt2string(s);});}}else{_LL3: _tmp71=((struct Cyc_IOEffect_Exp_IOEffect_NodeType_struct*)_tmp70->n)->f1;_LL4: {struct Cyc_Absyn_Exp*e=_tmp71;
return({Cyc_Absynpp_exp2string(e);});}}_LL0:;}
# 469
static struct Cyc_IOEffect_Node*Cyc_IOEffect_stmt_node(struct Cyc_Absyn_Stmt*s){
# 471
return({struct Cyc_IOEffect_Node*_tmp75=_cycalloc(sizeof(*_tmp75));({void*_tmp632=(void*)({struct Cyc_IOEffect_Stmt_IOEffect_NodeType_struct*_tmp74=_cycalloc(sizeof(*_tmp74));_tmp74->tag=1U,_tmp74->f1=s;_tmp74;});_tmp75->n=_tmp632;}),_tmp75->annot=Cyc_IOEffect_MayJump,_tmp75->loc=0U,_tmp75->input=0,_tmp75->output=0,_tmp75->throws=0,_tmp75->succ=0;_tmp75;});}
# 474
static struct Cyc_IOEffect_Node*Cyc_IOEffect_exp_node(struct Cyc_Absyn_Exp*e){
# 476
return({struct Cyc_IOEffect_Node*_tmp78=_cycalloc(sizeof(*_tmp78));({void*_tmp633=(void*)({struct Cyc_IOEffect_Exp_IOEffect_NodeType_struct*_tmp77=_cycalloc(sizeof(*_tmp77));_tmp77->tag=0U,_tmp77->f1=e;_tmp77;});_tmp78->n=_tmp633;}),_tmp78->annot=Cyc_IOEffect_MayJump,_tmp78->loc=0U,_tmp78->input=0,_tmp78->output=0,_tmp78->throws=0,_tmp78->succ=0;_tmp78;});}
# 479
static struct Cyc_IOEffect_Node*Cyc_IOEffect_get_exp(struct Cyc_IOEffect_Fenv*env,struct Cyc_Absyn_Exp*e){
# 481
struct Cyc_IOEffect_Node**x=({({struct Cyc_IOEffect_Node**(*_tmp635)(struct Cyc_Hashtable_Table*t,void*key)=({struct Cyc_IOEffect_Node**(*_tmp7B)(struct Cyc_Hashtable_Table*t,void*key)=(struct Cyc_IOEffect_Node**(*)(struct Cyc_Hashtable_Table*t,void*key))Cyc_Hashtable_lookup_opt;_tmp7B;});struct Cyc_Hashtable_Table*_tmp634=env->nodes;_tmp635(_tmp634,(void*)e);});});
if(x == 0){
# 484
struct Cyc_IOEffect_Node*n=({Cyc_IOEffect_exp_node(e);});
({({void(*_tmp638)(struct Cyc_Hashtable_Table*t,void*key,struct Cyc_IOEffect_Node*val)=({void(*_tmp7A)(struct Cyc_Hashtable_Table*t,void*key,struct Cyc_IOEffect_Node*val)=(void(*)(struct Cyc_Hashtable_Table*t,void*key,struct Cyc_IOEffect_Node*val))Cyc_Hashtable_insert;_tmp7A;});struct Cyc_Hashtable_Table*_tmp637=env->nodes;void*_tmp636=(void*)e;_tmp638(_tmp637,_tmp636,n);});});
return n;}else{
# 488
return*x;}}
# 491
static struct Cyc_IOEffect_Node*Cyc_IOEffect_get_stmt(struct Cyc_IOEffect_Fenv*env,struct Cyc_Absyn_Stmt*s){
# 493
struct Cyc_IOEffect_Node**x=({({struct Cyc_IOEffect_Node**(*_tmp63A)(struct Cyc_Hashtable_Table*t,void*key)=({struct Cyc_IOEffect_Node**(*_tmp7E)(struct Cyc_Hashtable_Table*t,void*key)=(struct Cyc_IOEffect_Node**(*)(struct Cyc_Hashtable_Table*t,void*key))Cyc_Hashtable_lookup_opt;_tmp7E;});struct Cyc_Hashtable_Table*_tmp639=env->nodes;_tmp63A(_tmp639,(void*)s);});});
if(x == 0){
# 496
struct Cyc_IOEffect_Node*n=({Cyc_IOEffect_stmt_node(s);});
({({void(*_tmp63D)(struct Cyc_Hashtable_Table*t,void*key,struct Cyc_IOEffect_Node*val)=({void(*_tmp7D)(struct Cyc_Hashtable_Table*t,void*key,struct Cyc_IOEffect_Node*val)=(void(*)(struct Cyc_Hashtable_Table*t,void*key,struct Cyc_IOEffect_Node*val))Cyc_Hashtable_insert;_tmp7D;});struct Cyc_Hashtable_Table*_tmp63C=env->nodes;void*_tmp63B=(void*)s;_tmp63D(_tmp63C,_tmp63B,n);});});
return n;}else{
# 500
return*x;}}
# 503
void Cyc_IOEffect_add_succ(struct Cyc_IOEffect_Node*n1,struct Cyc_IOEffect_Node*n2,struct _fat_ptr msg){
# 505
if(!({({int(*_tmp640)(int(*pred)(struct Cyc_IOEffect_Node*,struct Cyc_IOEffect_Node*),struct Cyc_IOEffect_Node*env,struct Cyc_List_List*x)=({int(*_tmp80)(int(*pred)(struct Cyc_IOEffect_Node*,struct Cyc_IOEffect_Node*),struct Cyc_IOEffect_Node*env,struct Cyc_List_List*x)=(int(*)(int(*pred)(struct Cyc_IOEffect_Node*,struct Cyc_IOEffect_Node*),struct Cyc_IOEffect_Node*env,struct Cyc_List_List*x))Cyc_List_exists_c;_tmp80;});int(*_tmp63F)(struct Cyc_IOEffect_Node*a,struct Cyc_IOEffect_Node*b)=({int(*_tmp81)(struct Cyc_IOEffect_Node*a,struct Cyc_IOEffect_Node*b)=(int(*)(struct Cyc_IOEffect_Node*a,struct Cyc_IOEffect_Node*b))Cyc_IOEffect_cmp;_tmp81;});struct Cyc_IOEffect_Node*_tmp63E=n2;_tmp640(_tmp63F,_tmp63E,n1->succ);});}))
# 526 "ioeffect.cyc"
({struct Cyc_List_List*_tmp641=({struct Cyc_List_List*_tmp82=_cycalloc(sizeof(*_tmp82));_tmp82->hd=n2,_tmp82->tl=n1->succ;_tmp82;});n1->succ=_tmp641;});}
# 529
void Cyc_IOEffect_add_stmt_succ(struct Cyc_IOEffect_Fenv*env,struct Cyc_Absyn_Stmt*s,struct Cyc_Absyn_Stmt*s1){
# 531
struct Cyc_IOEffect_Node*n1=({Cyc_IOEffect_get_stmt(env,s);});
struct Cyc_IOEffect_Node*n2=({Cyc_IOEffect_get_stmt(env,s1);});
({({struct Cyc_IOEffect_Node*_tmp643=n1;struct Cyc_IOEffect_Node*_tmp642=n2;Cyc_IOEffect_add_succ(_tmp643,_tmp642,({const char*_tmp84="add_stmt_succ";_tag_fat(_tmp84,sizeof(char),14U);}));});});}
# 536
void Cyc_IOEffect_add_exp_succ(struct Cyc_IOEffect_Fenv*env,struct Cyc_Absyn_Exp*s,struct Cyc_Absyn_Stmt*s1){
# 538
struct Cyc_IOEffect_Node*n1=({Cyc_IOEffect_get_exp(env,s);});
struct Cyc_IOEffect_Node*n2=({Cyc_IOEffect_get_stmt(env,s1);});
({({struct Cyc_IOEffect_Node*_tmp645=n1;struct Cyc_IOEffect_Node*_tmp644=n2;Cyc_IOEffect_add_succ(_tmp645,_tmp644,({const char*_tmp86="add_exp_succ";_tag_fat(_tmp86,sizeof(char),13U);}));});});}
# 542
static struct Cyc_List_List*Cyc_IOEffect_get_input(struct Cyc_IOEffect_Node*n){
# 544
struct Cyc_List_List*i=n->input;
if(i == & Cyc_IOEffect_empty_effect)return 0;else{
return i;}}
# 548
static struct Cyc_List_List*Cyc_IOEffect_get_output(struct Cyc_IOEffect_Node*n){
struct Cyc_List_List*i=n->output;
if(i == & Cyc_IOEffect_empty_effect)return 0;else{
return i;}}
# 554
static unsigned Cyc_IOEffect_node_seg(struct Cyc_IOEffect_Node*n){
# 556
void*_stmttmp5=n->n;void*_tmp8A=_stmttmp5;struct Cyc_Absyn_Stmt*_tmp8B;struct Cyc_Absyn_Exp*_tmp8C;if(((struct Cyc_IOEffect_Exp_IOEffect_NodeType_struct*)_tmp8A)->tag == 0U){_LL1: _tmp8C=((struct Cyc_IOEffect_Exp_IOEffect_NodeType_struct*)_tmp8A)->f1;_LL2: {struct Cyc_Absyn_Exp*e=_tmp8C;
# 558
return e->loc;}}else{_LL3: _tmp8B=((struct Cyc_IOEffect_Stmt_IOEffect_NodeType_struct*)_tmp8A)->f1;_LL4: {struct Cyc_Absyn_Stmt*s=_tmp8B;
return s->loc;}}_LL0:;}
# 562
static void Cyc_IOEffect_set_throws(struct Cyc_IOEffect_Node*n,struct Cyc_List_List*ql){
({struct Cyc_List_List*_tmp646=({Cyc_List_copy(ql);});n->throws=_tmp646;});}
# 565
static void Cyc_IOEffect_set_input_effect(struct Cyc_IOEffect_Node*n,struct Cyc_List_List*f,unsigned loc){
# 578 "ioeffect.cyc"
if(f == 0)f=& Cyc_IOEffect_empty_effect;{struct Cyc_List_List*curr=n->input;
# 580
if(curr == 0){
# 582
n->input=f;
n->loc=loc;}else{
# 587
if(curr != f &&(
(f == & Cyc_IOEffect_empty_effect || curr == & Cyc_IOEffect_empty_effect)|| !({Cyc_IOEffect_cmp_effect(curr,f);}))){
# 591
unsigned nseg=({Cyc_IOEffect_node_seg(n);});
struct _fat_ptr is=({Cyc_Position_string_of_segment(n->loc);});
struct _fat_ptr cs=({Cyc_Position_string_of_segment(loc);});
struct _fat_ptr f1=curr == & Cyc_IOEffect_empty_effect?({const char*_tmp96="(empty)";_tag_fat(_tmp96,sizeof(char),8U);}):({Cyc_Absyn_effect2string(curr);});struct _fat_ptr f2=
f == & Cyc_IOEffect_empty_effect?({const char*_tmp95="(empty)";_tag_fat(_tmp95,sizeof(char),8U);}):({Cyc_Absyn_effect2string(f);});
({struct Cyc_String_pa_PrintArg_struct _tmp91=({struct Cyc_String_pa_PrintArg_struct _tmp572;_tmp572.tag=0U,_tmp572.f1=(struct _fat_ptr)((struct _fat_ptr)f1);_tmp572;});struct Cyc_String_pa_PrintArg_struct _tmp92=({struct Cyc_String_pa_PrintArg_struct _tmp571;_tmp571.tag=0U,_tmp571.f1=(struct _fat_ptr)((struct _fat_ptr)is);_tmp571;});struct Cyc_String_pa_PrintArg_struct _tmp93=({struct Cyc_String_pa_PrintArg_struct _tmp570;_tmp570.tag=0U,_tmp570.f1=(struct _fat_ptr)((struct _fat_ptr)f2);_tmp570;});struct Cyc_String_pa_PrintArg_struct _tmp94=({struct Cyc_String_pa_PrintArg_struct _tmp56F;_tmp56F.tag=0U,_tmp56F.f1=(struct _fat_ptr)((struct _fat_ptr)cs);_tmp56F;});void*_tmp8F[4U];_tmp8F[0]=& _tmp91,_tmp8F[1]=& _tmp92,_tmp8F[2]=& _tmp93,_tmp8F[3]=& _tmp94;({unsigned _tmp648=nseg;struct _fat_ptr _tmp647=({const char*_tmp90="The input effect of this node %s (set at %s) does not match the effect that flows into this node %s (jump from %s)";_tag_fat(_tmp90,sizeof(char),115U);});Cyc_Tcutil_terr(_tmp648,_tmp647,_tag_fat(_tmp8F,sizeof(void*),4U));});});}}}}
# 603
static void Cyc_IOEffect_propagate_succ(struct Cyc_IOEffect_Node*n,struct Cyc_List_List*f){
# 605
if(n->succ != 0 && f != 0){
# 616 "ioeffect.cyc"
struct Cyc_List_List*iter=n->succ;
unsigned loc=({Cyc_IOEffect_node_seg(n);});
for(0;iter != 0;iter=iter->tl){({Cyc_IOEffect_set_input_effect((struct Cyc_IOEffect_Node*)iter->hd,f,loc);});}}}
# 627
static void Cyc_IOEffect_set_output_effect(struct Cyc_IOEffect_Node*n,struct Cyc_List_List*f){
# 630
if(f == 0)f=& Cyc_IOEffect_empty_effect;{struct Cyc_List_List*curr=n->output;
# 632
if(curr == 0)
# 634
n->output=f;else{
# 638
if(curr != f &&(
(f == & Cyc_IOEffect_empty_effect || curr == & Cyc_IOEffect_empty_effect)|| !({Cyc_IOEffect_cmp_effect(curr,f);}))){
# 642
unsigned nseg=({Cyc_IOEffect_node_seg(n);});
struct _fat_ptr is=({Cyc_Position_string_of_segment(n->loc);});
struct _fat_ptr cs=({Cyc_Position_string_of_segment(n->loc);});
struct _fat_ptr f1=curr == & Cyc_IOEffect_empty_effect?({const char*_tmpA0="(empty)";_tag_fat(_tmpA0,sizeof(char),8U);}):({Cyc_Absyn_effect2string(curr);});struct _fat_ptr f2=
f == & Cyc_IOEffect_empty_effect?({const char*_tmp9F="(empty)";_tag_fat(_tmp9F,sizeof(char),8U);}):({Cyc_Absyn_effect2string(f);});
({struct Cyc_String_pa_PrintArg_struct _tmp9B=({struct Cyc_String_pa_PrintArg_struct _tmp576;_tmp576.tag=0U,_tmp576.f1=(struct _fat_ptr)((struct _fat_ptr)f1);_tmp576;});struct Cyc_String_pa_PrintArg_struct _tmp9C=({struct Cyc_String_pa_PrintArg_struct _tmp575;_tmp575.tag=0U,_tmp575.f1=(struct _fat_ptr)((struct _fat_ptr)is);_tmp575;});struct Cyc_String_pa_PrintArg_struct _tmp9D=({struct Cyc_String_pa_PrintArg_struct _tmp574;_tmp574.tag=0U,_tmp574.f1=(struct _fat_ptr)((struct _fat_ptr)f2);_tmp574;});struct Cyc_String_pa_PrintArg_struct _tmp9E=({struct Cyc_String_pa_PrintArg_struct _tmp573;_tmp573.tag=0U,_tmp573.f1=(struct _fat_ptr)((struct _fat_ptr)cs);_tmp573;});void*_tmp99[4U];_tmp99[0]=& _tmp9B,_tmp99[1]=& _tmp9C,_tmp99[2]=& _tmp9D,_tmp99[3]=& _tmp9E;({unsigned _tmp64A=nseg;struct _fat_ptr _tmp649=({const char*_tmp9A="The output effect of this node %s (set at %s) does not match the effect that flows into this node %s (jump from %s)";_tag_fat(_tmp9A,sizeof(char),116U);});Cyc_Tcutil_terr(_tmp64A,_tmp649,_tag_fat(_tmp99,sizeof(void*),4U));});});}}
# 651
({Cyc_IOEffect_propagate_succ(n,f);});}}
# 655
static void Cyc_IOEffect_add_throws_effect(struct Cyc_IOEffect_Fenv*env,struct Cyc_Absyn_Exp*e,struct Cyc_List_List*cl){
# 657
struct Cyc_List_List*throws;
struct Cyc_List_List*succ;
struct Cyc_IOEffect_Node*n=({Cyc_IOEffect_get_exp(env,e);});
# 661
struct Cyc_Absyn_Stmt*ex=({Cyc_IOEffect_exit_stmt(env);});
for(0;cl != 0;cl=cl->tl){
# 664
struct _tuple14 c=*((struct _tuple14*)cl->hd);
({struct Cyc_List_List*_tmp64B=({Cyc_Absyn_add_qvar(n->throws,c.f1);});n->throws=_tmp64B;});{
# 668
struct Cyc_List_List*iter=c.f3?({struct Cyc_List_List*_tmpA3=_cycalloc(sizeof(*_tmpA3));_tmpA3->hd=ex,_tmpA3->tl=c.f2;_tmpA3;}): c.f2;
for(0;iter != 0;iter=iter->tl){
# 672
struct Cyc_IOEffect_Node*n2=({Cyc_IOEffect_get_stmt(env,(struct Cyc_Absyn_Stmt*)iter->hd);});
({({struct Cyc_IOEffect_Node*_tmp64D=n;struct Cyc_IOEffect_Node*_tmp64C=n2;Cyc_IOEffect_add_succ(_tmp64D,_tmp64C,({const char*_tmpA2="add_throws_effect";_tag_fat(_tmpA2,sizeof(char),18U);}));});});}}}}
# 689 "ioeffect.cyc"
static void Cyc_IOEffect_register_stmt_succ(struct Cyc_IOEffect_Env*env,struct Cyc_Absyn_Stmt*s){
# 691
{void*_stmttmp6=s->r;void*_tmpA5=_stmttmp6;switch(*((int*)_tmpA5)){case 6U: _LL1: _LL2:
# 693
 goto _LL4;case 7U: _LL3: _LL4:
 goto _LL6;case 8U: _LL5: _LL6:
 goto _LL8;case 11U: _LL7: _LL8:
 goto _LL0;case 3U: _LL9: _LLA:
# 699
({Cyc_IOEffect_add_stmt_succ(env->fenv,s,(env->fenv)->exit_stmt);});
# 701
return;default: _LLB: _LLC:
 return;}_LL0:;}{
# 704
struct Cyc_Hashtable_Table*tab=(env->tables)->succ_tables;
struct Cyc_Hashtable_Table**opt=({({struct Cyc_Hashtable_Table**(*_tmp64F)(struct Cyc_Hashtable_Table*t,struct Cyc_Absyn_Fndecl*key)=({struct Cyc_Hashtable_Table**(*_tmpAB)(struct Cyc_Hashtable_Table*t,struct Cyc_Absyn_Fndecl*key)=(struct Cyc_Hashtable_Table**(*)(struct Cyc_Hashtable_Table*t,struct Cyc_Absyn_Fndecl*key))Cyc_Hashtable_lookup_opt;_tmpAB;});struct Cyc_Hashtable_Table*_tmp64E=tab;_tmp64F(_tmp64E,(env->fenv)->fd);});});
if(opt == 0)({({int(*_tmp650)(struct _fat_ptr msg)=({int(*_tmpA6)(struct _fat_ptr msg)=(int(*)(struct _fat_ptr msg))Cyc_IOEffect_env_err;_tmpA6;});_tmp650(({const char*_tmpA7="IOEffect::register_stmt_succ";_tag_fat(_tmpA7,sizeof(char),29U);}));});});else{
struct Cyc_Absyn_Stmt**_stmttmp7=({({struct Cyc_Absyn_Stmt**(*_tmp652)(struct Cyc_Hashtable_Table*t,struct Cyc_Absyn_Stmt*key)=({struct Cyc_Absyn_Stmt**(*_tmpAA)(struct Cyc_Hashtable_Table*t,struct Cyc_Absyn_Stmt*key)=(struct Cyc_Absyn_Stmt**(*)(struct Cyc_Hashtable_Table*t,struct Cyc_Absyn_Stmt*key))Cyc_Hashtable_lookup_opt;_tmpAA;});struct Cyc_Hashtable_Table*_tmp651=*opt;_tmp652(_tmp651,s);});});struct Cyc_Absyn_Stmt**_tmpA8=_stmttmp7;struct Cyc_Absyn_Stmt*_tmpA9;if(_tmpA8 != 0){_LLE: _tmpA9=*_tmpA8;if(_tmpA9 != 0){_LLF: {struct Cyc_Absyn_Stmt*opt1=_tmpA9;
# 710
({Cyc_IOEffect_add_stmt_succ(env->fenv,s,opt1);});
# 712
goto _LLD;}}else{goto _LL10;}}else{_LL10: _LL11:
# 714
({Cyc_IOEffect_add_stmt_succ(env->fenv,s,(env->fenv)->exit_stmt);});
# 716
goto _LLD;}_LLD:;}}}struct _tuple20{struct _tuple0*f1;struct Cyc_Absyn_Stmt*f2;};
# 726
static struct _tuple8 Cyc_IOEffect_analyze_tree(struct _tuple0*n,void*d,int depth){
# 728
struct Cyc_List_List*in=0;
struct Cyc_List_List*out=0;
{void*_tmpAD=d;void*_tmpAF;struct Cyc_List_List*_tmpAE;struct Cyc_Tcpat_Rhs*_tmpB0;switch(*((int*)_tmpAD)){case 0U: _LL1: _LL2:
# 732
({({int(*_tmp653)(struct _fat_ptr msg)=({int(*_tmpB1)(struct _fat_ptr msg)=(int(*)(struct _fat_ptr msg))Cyc_IOEffect_env_err;_tmpB1;});_tmp653(({const char*_tmpB2="analyze_tree";_tag_fat(_tmpB2,sizeof(char),13U);}));});});case 1U: _LL3: _tmpB0=((struct Cyc_Tcpat_Success_Tcpat_Decision_struct*)_tmpAD)->f1;_LL4: {struct Cyc_Tcpat_Rhs*rhs=_tmpB0;
# 734
if(depth > 0){
# 736
if(({Cyc_Absyn_is_exn_stmt(rhs->rhs);}))
out=({struct Cyc_List_List*_tmpB3=_cycalloc(sizeof(*_tmpB3));_tmpB3->hd=n,_tmpB3->tl=out;_tmpB3;});else{
# 739
in=({struct Cyc_List_List*_tmpB5=_cycalloc(sizeof(*_tmpB5));({struct _tuple20*_tmp654=({struct _tuple20*_tmpB4=_cycalloc(sizeof(*_tmpB4));_tmpB4->f1=n,_tmpB4->f2=rhs->rhs;_tmpB4;});_tmpB5->hd=_tmp654;}),_tmpB5->tl=in;_tmpB5;});}}else{
# 741
if(depth == 0){
# 743
if(({Cyc_Absyn_is_exn_stmt(rhs->rhs);}))return({struct _tuple8 _tmp577;_tmp577.f1=0,_tmp577.f2=0;_tmp577;});else{
# 745
in=({struct Cyc_List_List*_tmpB7=_cycalloc(sizeof(*_tmpB7));({struct _tuple20*_tmp656=({struct _tuple20*_tmpB6=_cycalloc(sizeof(*_tmpB6));({struct _tuple0*_tmp655=({Cyc_Absyn_default_case_qvar();});_tmpB6->f1=_tmp655;}),_tmpB6->f2=rhs->rhs;_tmpB6;});_tmpB7->hd=_tmp656;}),_tmpB7->tl=0;_tmpB7;});}}else{
# 747
({({int(*_tmp657)(struct _fat_ptr msg)=({int(*_tmpB8)(struct _fat_ptr msg)=(int(*)(struct _fat_ptr msg))Cyc_IOEffect_env_err;_tmpB8;});_tmp657(({const char*_tmpB9="analyze_tree impos";_tag_fat(_tmpB9,sizeof(char),19U);}));});});}}
goto _LL0;}default: _LL5: _tmpAE=((struct Cyc_Tcpat_SwitchDec_Tcpat_Decision_struct*)_tmpAD)->f2;_tmpAF=(void*)((struct Cyc_Tcpat_SwitchDec_Tcpat_Decision_struct*)_tmpAD)->f3;_LL6: {struct Cyc_List_List*cases=_tmpAE;void*def=_tmpAF;
# 750
if(depth == 0)
# 752
for(0;cases != 0;cases=cases->tl){
# 754
struct _tuple15 _stmttmp8=*((struct _tuple15*)cases->hd);void*_tmpBB;void*_tmpBA;_LL8: _tmpBA=_stmttmp8.f1;_tmpBB=_stmttmp8.f2;_LL9: {void*pt=_tmpBA;void*d=_tmpBB;
void*_tmpBC=pt;struct Cyc_Absyn_Datatypefield*_tmpBD;if(((struct Cyc_Tcpat_EqExtensibleDatatype_Tcpat_PatTest_struct*)_tmpBC)->tag == 9U){_LLB: _tmpBD=((struct Cyc_Tcpat_EqExtensibleDatatype_Tcpat_PatTest_struct*)_tmpBC)->f2;_LLC: {struct Cyc_Absyn_Datatypefield*f=_tmpBD;
# 758
struct _tuple8 _stmttmp9=({Cyc_IOEffect_analyze_tree(f->name,d,1);});struct Cyc_List_List*_tmpBF;struct Cyc_List_List*_tmpBE;_LL10: _tmpBE=_stmttmp9.f1;_tmpBF=_stmttmp9.f2;_LL11: {struct Cyc_List_List*in1=_tmpBE;struct Cyc_List_List*out1=_tmpBF;
in=({Cyc_List_append(in,in1);});
out=({Cyc_List_append(out,out1);});
goto _LLA;}}}else{_LLD: _LLE:
({({int(*_tmp658)(struct _fat_ptr msg)=({int(*_tmpC0)(struct _fat_ptr msg)=(int(*)(struct _fat_ptr msg))Cyc_IOEffect_env_err;_tmpC0;});_tmp658(({const char*_tmpC1="analyze_tree";_tag_fat(_tmpC1,sizeof(char),13U);}));});});}_LLA:;}}else{
# 766
if(depth > 0)
# 768
for(0;cases != 0;cases=cases->tl){
# 770
struct _tuple15 _stmttmpA=*((struct _tuple15*)cases->hd);void*_tmpC3;void*_tmpC2;_LL13: _tmpC2=_stmttmpA.f1;_tmpC3=_stmttmpA.f2;_LL14: {void*pt=_tmpC2;void*d=_tmpC3;
struct _tuple8 _stmttmpB=({Cyc_IOEffect_analyze_tree(n,d,1);});struct Cyc_List_List*_tmpC5;struct Cyc_List_List*_tmpC4;_LL16: _tmpC4=_stmttmpB.f1;_tmpC5=_stmttmpB.f2;_LL17: {struct Cyc_List_List*in1=_tmpC4;struct Cyc_List_List*out1=_tmpC5;
in=({Cyc_List_append(in,in1);});
out=({Cyc_List_append(out,out1);});}}}else{
# 776
({({int(*_tmp659)(struct _fat_ptr msg)=({int(*_tmpC6)(struct _fat_ptr msg)=(int(*)(struct _fat_ptr msg))Cyc_IOEffect_env_err;_tmpC6;});_tmp659(({const char*_tmpC7="analyze_tree impos";_tag_fat(_tmpC7,sizeof(char),19U);}));});});}}{
struct _tuple8 _stmttmpC=({Cyc_IOEffect_analyze_tree(n,def,depth);});struct Cyc_List_List*_tmpC9;struct Cyc_List_List*_tmpC8;_LL19: _tmpC8=_stmttmpC.f1;_tmpC9=_stmttmpC.f2;_LL1A: {struct Cyc_List_List*in1=_tmpC8;struct Cyc_List_List*out1=_tmpC9;
in=({Cyc_List_append(in,in1);});
out=({Cyc_List_append(out,out1);});
goto _LL0;}}}}_LL0:;}
# 782
return({struct _tuple8 _tmp578;_tmp578.f1=in,_tmp578.f2=out;_tmp578;});}
# 785
static struct _fat_ptr Cyc_IOEffect_catch2string(struct _tuple20*c){
# 787
return(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmpCD=({struct Cyc_String_pa_PrintArg_struct _tmp57A;_tmp57A.tag=0U,_tmp57A.f1=(struct _fat_ptr)((struct _fat_ptr)*(*(*c).f1).f2);_tmp57A;});struct Cyc_String_pa_PrintArg_struct _tmpCE=({struct Cyc_String_pa_PrintArg_struct _tmp579;_tmp579.tag=0U,({
struct _fat_ptr _tmp65A=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_stmt2string((*c).f2);}));_tmp579.f1=_tmp65A;});_tmp579;});void*_tmpCB[2U];_tmpCB[0]=& _tmpCD,_tmpCB[1]=& _tmpCE;({struct _fat_ptr _tmp65B=({const char*_tmpCC="BEGIN_CATCH(%s,%s)END_CATCH";_tag_fat(_tmpCC,sizeof(char),28U);});Cyc_aprintf(_tmp65B,_tag_fat(_tmpCB,sizeof(void*),2U));});});}
# 791
static struct _fat_ptr Cyc_IOEffect_nocatch2string(struct _tuple0*c){
# 793
return(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmpD2=({struct Cyc_String_pa_PrintArg_struct _tmp57B;_tmp57B.tag=0U,_tmp57B.f1=(struct _fat_ptr)((struct _fat_ptr)*(*c).f2);_tmp57B;});void*_tmpD0[1U];_tmpD0[0]=& _tmpD2;({struct _fat_ptr _tmp65C=({const char*_tmpD1="BEGIN_NOCATCH(%s)END_NOCATCH";_tag_fat(_tmpD1,sizeof(char),29U);});Cyc_aprintf(_tmp65C,_tag_fat(_tmpD0,sizeof(void*),1U));});});}
# 796
static struct _fat_ptr Cyc_IOEffect_cnc2string(struct _tuple8 z){
# 798
return(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmpD6=({struct Cyc_String_pa_PrintArg_struct _tmp57D;_tmp57D.tag=0U,({
struct _fat_ptr _tmp65E=(struct _fat_ptr)((struct _fat_ptr)({({struct _fat_ptr(*_tmp65D)(struct Cyc_List_List*l,struct _fat_ptr(*cb)(struct _tuple20*))=({struct _fat_ptr(*_tmpD9)(struct Cyc_List_List*l,struct _fat_ptr(*cb)(struct _tuple20*))=(struct _fat_ptr(*)(struct Cyc_List_List*l,struct _fat_ptr(*cb)(struct _tuple20*)))Cyc_Absyn_list2string;_tmpD9;});_tmp65D(z.f1,Cyc_IOEffect_catch2string);});}));_tmp57D.f1=_tmp65E;});_tmp57D;});struct Cyc_String_pa_PrintArg_struct _tmpD7=({struct Cyc_String_pa_PrintArg_struct _tmp57C;_tmp57C.tag=0U,({struct _fat_ptr _tmp660=(struct _fat_ptr)((struct _fat_ptr)({({struct _fat_ptr(*_tmp65F)(struct Cyc_List_List*l,struct _fat_ptr(*cb)(struct _tuple0*))=({struct _fat_ptr(*_tmpD8)(struct Cyc_List_List*l,struct _fat_ptr(*cb)(struct _tuple0*))=(struct _fat_ptr(*)(struct Cyc_List_List*l,struct _fat_ptr(*cb)(struct _tuple0*)))Cyc_Absyn_list2string;_tmpD8;});_tmp65F(z.f2,Cyc_IOEffect_nocatch2string);});}));_tmp57C.f1=_tmp660;});_tmp57C;});void*_tmpD4[2U];_tmpD4[0]=& _tmpD6,_tmpD4[1]=& _tmpD7;({struct _fat_ptr _tmp661=({const char*_tmpD5="BEGIN CATCH LIST\n %s\nEND CATCH LIST\nBEGIN NO CATCH LIST\n%s\nEND NO CATCH LIST";_tag_fat(_tmpD5,sizeof(char),77U);});Cyc_aprintf(_tmp661,_tag_fat(_tmpD4,sizeof(void*),2U));});});}
# 802
static struct _fat_ptr Cyc_IOEffect_combined2string(struct _tuple14*c){
# 804
struct _tuple14 d=*c;
return(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmpDD=({struct Cyc_String_pa_PrintArg_struct _tmp580;_tmp580.tag=0U,({
struct _fat_ptr _tmp662=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(d.f1);}));_tmp580.f1=_tmp662;});_tmp580;});struct Cyc_Int_pa_PrintArg_struct _tmpDE=({struct Cyc_Int_pa_PrintArg_struct _tmp57F;_tmp57F.tag=1U,_tmp57F.f1=(unsigned long)d.f3;_tmp57F;});struct Cyc_String_pa_PrintArg_struct _tmpDF=({struct Cyc_String_pa_PrintArg_struct _tmp57E;_tmp57E.tag=0U,({struct _fat_ptr _tmp664=(struct _fat_ptr)((struct _fat_ptr)({({struct _fat_ptr(*_tmp663)(struct Cyc_List_List*l,struct _fat_ptr(*cb)(struct Cyc_Absyn_Stmt*))=({struct _fat_ptr(*_tmpE0)(struct Cyc_List_List*l,struct _fat_ptr(*cb)(struct Cyc_Absyn_Stmt*))=(struct _fat_ptr(*)(struct Cyc_List_List*l,struct _fat_ptr(*cb)(struct Cyc_Absyn_Stmt*)))Cyc_Absyn_list2string;_tmpE0;});_tmp663(d.f2,Cyc_Absynpp_stmt2string);});}));_tmp57E.f1=_tmp664;});_tmp57E;});void*_tmpDB[3U];_tmpDB[0]=& _tmpDD,_tmpDB[1]=& _tmpDE,_tmpDB[2]=& _tmpDF;({struct _fat_ptr _tmp665=({const char*_tmpDC="\nExn name: %s, Uncaught: %d , May Handlers: %s";_tag_fat(_tmpDC,sizeof(char),47U);});Cyc_aprintf(_tmp665,_tag_fat(_tmpDB,sizeof(void*),3U));});});}
# 809
static struct _fat_ptr Cyc_IOEffect_comblist2string(struct Cyc_List_List*c){
return({({struct _fat_ptr(*_tmp666)(struct Cyc_List_List*l,struct _fat_ptr(*cb)(struct _tuple14*))=({struct _fat_ptr(*_tmpE2)(struct Cyc_List_List*l,struct _fat_ptr(*cb)(struct _tuple14*))=(struct _fat_ptr(*)(struct Cyc_List_List*l,struct _fat_ptr(*cb)(struct _tuple14*)))Cyc_Absyn_list2string;_tmpE2;});_tmp666(c,Cyc_IOEffect_combined2string);});});}
# 812
static int Cyc_IOEffect_my_qvar_cmp(struct _tuple0*q1,struct _tuple0*q2){
# 814
return({int(*_tmpE4)(struct _fat_ptr s1,struct _fat_ptr s2)=Cyc_strcmp;struct _fat_ptr _tmpE5=(struct _fat_ptr)({Cyc_Absynpp_qvar2string(q1);});struct _fat_ptr _tmpE6=(struct _fat_ptr)({Cyc_Absynpp_qvar2string(q2);});_tmpE4(_tmpE5,_tmpE6);})== 0;}
# 816
static int Cyc_IOEffect_not_my_qvar_cmp(struct _tuple0*q1,struct _tuple0*q2){return !({Cyc_IOEffect_my_qvar_cmp(q1,q2);});}
static int Cyc_IOEffect_my_qvar_cmp2(struct _tuple0*q1,struct _tuple20*q2){return({Cyc_IOEffect_my_qvar_cmp(q1,(*q2).f1);});}
static struct Cyc_List_List*Cyc_IOEffect_remove_duplicates(int(*eq)(void*,void*),struct Cyc_List_List*l){
# 820
if(l == 0)return 0;{struct _tuple8 _stmttmpD=({Cyc_IOEffect_split_list(eq,l->hd,l,Cyc_Core_heap_region);});struct Cyc_List_List*_tmpEB;struct Cyc_List_List*_tmpEA;_LL1: _tmpEA=_stmttmpD.f1;_tmpEB=_stmttmpD.f2;_LL2: {struct Cyc_List_List*a=_tmpEA;struct Cyc_List_List*b=_tmpEB;
# 822
return({struct Cyc_List_List*_tmpEC=_cycalloc(sizeof(*_tmpEC));_tmpEC->hd=((struct Cyc_List_List*)_check_null(a))->hd,({struct Cyc_List_List*_tmp667=({Cyc_IOEffect_remove_duplicates(eq,b);});_tmpEC->tl=_tmp667;});_tmpEC;});}}}
# 824
static struct Cyc_Absyn_Stmt*Cyc_IOEffect_catch2stmt(struct _tuple20*c){return(*c).f2;}
static struct Cyc_List_List*Cyc_IOEffect_sum_catch(struct Cyc_List_List*cl){
# 827
if(cl == 0)return 0;{struct _tuple20*c=(struct _tuple20*)cl->hd;
# 829
struct _tuple0*q=(*c).f1;
struct _tuple8 _stmttmpE=({({struct _tuple8(*_tmp66A)(int(*pred)(struct _tuple0*,struct _tuple20*),struct _tuple0*env,struct Cyc_List_List*l,struct _RegionHandle*h)=({struct _tuple8(*_tmpF9)(int(*pred)(struct _tuple0*,struct _tuple20*),struct _tuple0*env,struct Cyc_List_List*l,struct _RegionHandle*h)=(struct _tuple8(*)(int(*pred)(struct _tuple0*,struct _tuple20*),struct _tuple0*env,struct Cyc_List_List*l,struct _RegionHandle*h))Cyc_IOEffect_split_list;_tmpF9;});struct _tuple0*_tmp669=q;struct Cyc_List_List*_tmp668=cl;_tmp66A(Cyc_IOEffect_my_qvar_cmp2,_tmp669,_tmp668,Cyc_Core_heap_region);});});struct Cyc_List_List*_tmpF0;struct Cyc_List_List*_tmpEF;_LL1: _tmpEF=_stmttmpE.f1;_tmpF0=_stmttmpE.f2;_LL2: {struct Cyc_List_List*a=_tmpEF;struct Cyc_List_List*b=_tmpF0;
struct Cyc_List_List*stmts=({struct Cyc_List_List*(*_tmpF3)(int(*eq)(struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt*),struct Cyc_List_List*l)=({struct Cyc_List_List*(*_tmpF4)(int(*eq)(struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt*),struct Cyc_List_List*l)=(struct Cyc_List_List*(*)(int(*eq)(struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt*),struct Cyc_List_List*l))Cyc_IOEffect_remove_duplicates;_tmpF4;});int(*_tmpF5)(struct Cyc_Absyn_Stmt*a,struct Cyc_Absyn_Stmt*b)=({int(*_tmpF6)(struct Cyc_Absyn_Stmt*a,struct Cyc_Absyn_Stmt*b)=(int(*)(struct Cyc_Absyn_Stmt*a,struct Cyc_Absyn_Stmt*b))Cyc_IOEffect_cmp;_tmpF6;});struct Cyc_List_List*_tmpF7=({({struct Cyc_List_List*(*_tmp66B)(struct Cyc_Absyn_Stmt*(*f)(struct _tuple20*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmpF8)(struct Cyc_Absyn_Stmt*(*f)(struct _tuple20*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_Stmt*(*f)(struct _tuple20*),struct Cyc_List_List*x))Cyc_List_map;_tmpF8;});_tmp66B(Cyc_IOEffect_catch2stmt,a);});});_tmpF3(_tmpF5,_tmpF7);});
return({struct Cyc_List_List*_tmpF2=_cycalloc(sizeof(*_tmpF2));({struct _tuple14*_tmp66D=({struct _tuple14*_tmpF1=_cycalloc(sizeof(*_tmpF1));_tmpF1->f1=q,_tmpF1->f2=stmts,_tmpF1->f3=0;_tmpF1;});_tmpF2->hd=_tmp66D;}),({struct Cyc_List_List*_tmp66C=({Cyc_IOEffect_sum_catch(b);});_tmpF2->tl=_tmp66C;});_tmpF2;});}}}
# 834
static void Cyc_IOEffect_set_nocatch_flag(struct Cyc_List_List*nc,struct _tuple14*c){
if(({({int(*_tmp66F)(int(*pred)(struct _tuple0*,struct _tuple0*),struct _tuple0*env,struct Cyc_List_List*x)=({int(*_tmpFB)(int(*pred)(struct _tuple0*,struct _tuple0*),struct _tuple0*env,struct Cyc_List_List*x)=(int(*)(int(*pred)(struct _tuple0*,struct _tuple0*),struct _tuple0*env,struct Cyc_List_List*x))Cyc_List_exists_c;_tmpFB;});struct _tuple0*_tmp66E=(*c).f1;_tmp66F(Cyc_IOEffect_my_qvar_cmp,_tmp66E,nc);});}))(*c).f3=1;}
static int Cyc_IOEffect_qvar_combined_cmp(struct _tuple0*q1,struct _tuple14*q2){return({Cyc_IOEffect_my_qvar_cmp(q1,(*q2).f1);});}struct _tuple21{struct Cyc_List_List*f1;struct Cyc_Absyn_Stmt*f2;};
static struct _tuple21 Cyc_IOEffect_combine(struct Cyc_List_List*c,struct Cyc_List_List*nc){
# 839
struct _tuple8 _stmttmpF=({struct _tuple8(*_tmp101)(int(*pred)(struct _tuple0*,struct _tuple14*),struct _tuple0*env,struct Cyc_List_List*l,struct _RegionHandle*h)=({struct _tuple8(*_tmp102)(int(*pred)(struct _tuple0*,struct _tuple14*),struct _tuple0*env,struct Cyc_List_List*l,struct _RegionHandle*h)=(struct _tuple8(*)(int(*pred)(struct _tuple0*,struct _tuple14*),struct _tuple0*env,struct Cyc_List_List*l,struct _RegionHandle*h))Cyc_IOEffect_split_list;_tmp102;});int(*_tmp103)(struct _tuple0*q1,struct _tuple14*q2)=Cyc_IOEffect_qvar_combined_cmp;struct _tuple0*_tmp104=({Cyc_Absyn_default_case_qvar();});struct Cyc_List_List*_tmp105=({Cyc_IOEffect_sum_catch(c);});struct _RegionHandle*_tmp106=Cyc_Core_heap_region;_tmp101(_tmp103,_tmp104,_tmp105,_tmp106);});struct Cyc_List_List*_tmpFF;struct Cyc_List_List*_tmpFE;_LL1: _tmpFE=_stmttmpF.f1;_tmpFF=_stmttmpF.f2;_LL2: {struct Cyc_List_List*def=_tmpFE;struct Cyc_List_List*ret=_tmpFF;
# 841
struct Cyc_Absyn_Stmt*defstmt=def == 0?0:(struct Cyc_Absyn_Stmt*)((struct Cyc_List_List*)_check_null((*((struct _tuple14*)def->hd)).f2))->hd;
# 843
if(defstmt == 0)
# 845
({({void(*_tmp671)(void(*f)(struct Cyc_List_List*,struct _tuple14*),struct Cyc_List_List*env,struct Cyc_List_List*x)=({void(*_tmp100)(void(*f)(struct Cyc_List_List*,struct _tuple14*),struct Cyc_List_List*env,struct Cyc_List_List*x)=(void(*)(void(*f)(struct Cyc_List_List*,struct _tuple14*),struct Cyc_List_List*env,struct Cyc_List_List*x))Cyc_List_iter_c;_tmp100;});struct Cyc_List_List*_tmp670=nc;_tmp671(Cyc_IOEffect_set_nocatch_flag,_tmp670,ret);});});
# 843
return({struct _tuple21 _tmp581;_tmp581.f1=ret,_tmp581.f2=defstmt;_tmp581;});}}struct _tuple22{struct Cyc_List_List**f1;int f2;};
# 849
static void Cyc_IOEffect_comb_comb(struct _tuple22*par,struct _tuple14*newl){
# 851
struct _tuple22 _stmttmp10=*par;int _tmp109;struct Cyc_List_List**_tmp108;_LL1: _tmp108=_stmttmp10.f1;_tmp109=_stmttmp10.f2;_LL2: {struct Cyc_List_List**curr=_tmp108;int any=_tmp109;
struct Cyc_List_List*iter=*curr;
struct _tuple0*q=(*newl).f1;
for(0;iter != 0;iter=iter->tl){
if(({Cyc_IOEffect_my_qvar_cmp((*((struct _tuple14*)iter->hd)).f1,q);})){
# 857
if((*((struct _tuple14*)iter->hd)).f3){
# 859
({struct Cyc_List_List*_tmp672=({struct Cyc_List_List*(*_tmp10A)(int(*eq)(struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt*),struct Cyc_List_List*l)=({struct Cyc_List_List*(*_tmp10B)(int(*eq)(struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt*),struct Cyc_List_List*l)=(struct Cyc_List_List*(*)(int(*eq)(struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt*),struct Cyc_List_List*l))Cyc_IOEffect_remove_duplicates;_tmp10B;});int(*_tmp10C)(struct Cyc_Absyn_Stmt*a,struct Cyc_Absyn_Stmt*b)=({int(*_tmp10D)(struct Cyc_Absyn_Stmt*a,struct Cyc_Absyn_Stmt*b)=(int(*)(struct Cyc_Absyn_Stmt*a,struct Cyc_Absyn_Stmt*b))Cyc_IOEffect_cmp;_tmp10D;});struct Cyc_List_List*_tmp10E=({Cyc_List_append((*((struct _tuple14*)iter->hd)).f2,(*newl).f2);});_tmp10A(_tmp10C,_tmp10E);});(*((struct _tuple14*)iter->hd)).f2=_tmp672;});
# 861
(*((struct _tuple14*)iter->hd)).f3=(*newl).f3;}
# 857
return;}}
# 854
if(any)
# 865
({struct Cyc_List_List*_tmp673=({struct Cyc_List_List*_tmp10F=_cycalloc(sizeof(*_tmp10F));_tmp10F->hd=newl,_tmp10F->tl=*curr;_tmp10F;});*curr=_tmp673;});}}
# 867
static void Cyc_IOEffect_defstmt_comb(struct Cyc_Absyn_Stmt*def,struct _tuple14*comb){
# 869
if((*comb).f3){
# 871
(*comb).f3=0;
({struct Cyc_List_List*_tmp676=({({struct Cyc_List_List*(*_tmp675)(int(*eq)(struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt*),struct Cyc_List_List*l)=({struct Cyc_List_List*(*_tmp111)(int(*eq)(struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt*),struct Cyc_List_List*l)=(struct Cyc_List_List*(*)(int(*eq)(struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt*),struct Cyc_List_List*l))Cyc_IOEffect_remove_duplicates;_tmp111;});int(*_tmp674)(struct Cyc_Absyn_Stmt*a,struct Cyc_Absyn_Stmt*b)=({int(*_tmp112)(struct Cyc_Absyn_Stmt*a,struct Cyc_Absyn_Stmt*b)=(int(*)(struct Cyc_Absyn_Stmt*a,struct Cyc_Absyn_Stmt*b))Cyc_IOEffect_cmp;_tmp112;});_tmp675(_tmp674,({struct Cyc_List_List*_tmp113=_cycalloc(sizeof(*_tmp113));_tmp113->hd=def,_tmp113->tl=(*comb).f2;_tmp113;}));});});(*comb).f2=_tmp676;});}}
# 876
static struct _tuple14*Cyc_IOEffect_q2comb(struct _tuple0*q){
return({struct _tuple14*_tmp115=_cycalloc(sizeof(*_tmp115));_tmp115->f1=q,_tmp115->f2=0,_tmp115->f3=1;_tmp115;});}
static int Cyc_IOEffect_is_uncaught(struct _tuple14*c){return(*c).f3;}
# 880
static void Cyc_IOEffect_check_exn_annot(unsigned loc,struct Cyc_List_List*annot,struct Cyc_List_List*comb){
# 882
if(annot == 0)return;for(0;comb != 0;comb=comb->tl){
# 885
struct _tuple14 c=*((struct _tuple14*)comb->hd);
if(c.f3 && !({Cyc_Absyn_exists_throws(annot,c.f1);})){
# 888
struct _fat_ptr s1=({Cyc_Absyn_throws_qvar2string(c.f1);});
struct _fat_ptr s2=({Cyc_Absyn_throws2string(annot);});
({struct Cyc_String_pa_PrintArg_struct _tmp11A=({struct Cyc_String_pa_PrintArg_struct _tmp584;_tmp584.tag=0U,_tmp584.f1=(struct _fat_ptr)((struct _fat_ptr)s1);_tmp584;});struct Cyc_String_pa_PrintArg_struct _tmp11B=({struct Cyc_String_pa_PrintArg_struct _tmp583;_tmp583.tag=0U,_tmp583.f1=(struct _fat_ptr)((struct _fat_ptr)s1);_tmp583;});struct Cyc_String_pa_PrintArg_struct _tmp11C=({struct Cyc_String_pa_PrintArg_struct _tmp582;_tmp582.tag=0U,_tmp582.f1=(struct _fat_ptr)((struct _fat_ptr)s2);_tmp582;});void*_tmp118[3U];_tmp118[0]=& _tmp11A,_tmp118[1]=& _tmp11B,_tmp118[2]=& _tmp11C;({unsigned _tmp678=loc;struct _fat_ptr _tmp677=({const char*_tmp119="Expression throws uncaught exception %s. %s is not declared in throws annotation  %s so it needs to be caught.";_tag_fat(_tmp119,sizeof(char),111U);});Cyc_Tcutil_terr(_tmp678,_tmp677,_tag_fat(_tmp118,sizeof(void*),3U));});});}}}
# 896
static struct Cyc_List_List*Cyc_IOEffect_derive_comblist(struct Cyc_List_List*iter,struct Cyc_List_List*ql,int any){
# 898
int set_def=0;
struct Cyc_List_List*comb=({({struct Cyc_List_List*(*_tmp679)(struct _tuple14*(*f)(struct _tuple0*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp12E)(struct _tuple14*(*f)(struct _tuple0*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct _tuple14*(*f)(struct _tuple0*),struct Cyc_List_List*x))Cyc_List_map;_tmp12E;});_tmp679(Cyc_IOEffect_q2comb,ql);});});
for(0;(unsigned)iter;iter=iter->tl){
# 902
struct _tuple8 _stmttmp11=({struct _tuple8(*_tmp128)(struct _tuple0*n,void*d,int depth)=Cyc_IOEffect_analyze_tree;struct _tuple0*_tmp129=({Cyc_Absyn_default_case_qvar();});void*_tmp12A=(*((struct _tuple19*)iter->hd)).f2;int _tmp12B=0;_tmp128(_tmp129,_tmp12A,_tmp12B);});struct Cyc_List_List*_tmp11F;struct Cyc_List_List*_tmp11E;_LL1: _tmp11E=_stmttmp11.f1;_tmp11F=_stmttmp11.f2;_LL2: {struct Cyc_List_List*c=_tmp11E;struct Cyc_List_List*nc=_tmp11F;
struct _tuple21 _stmttmp12=({Cyc_IOEffect_combine(c,nc);});struct Cyc_Absyn_Stmt*_tmp121;struct Cyc_List_List*_tmp120;_LL4: _tmp120=_stmttmp12.f1;_tmp121=_stmttmp12.f2;_LL5: {struct Cyc_List_List*newcomb=_tmp120;struct Cyc_Absyn_Stmt*defstmt=_tmp121;
struct _tuple22 comb_par=({struct _tuple22 _tmp585;_tmp585.f1=& comb,_tmp585.f2=any;_tmp585;});
({({void(*_tmp67A)(void(*f)(struct _tuple22*,struct _tuple14*),struct _tuple22*env,struct Cyc_List_List*x)=({void(*_tmp122)(void(*f)(struct _tuple22*,struct _tuple14*),struct _tuple22*env,struct Cyc_List_List*x)=(void(*)(void(*f)(struct _tuple22*,struct _tuple14*),struct _tuple22*env,struct Cyc_List_List*x))Cyc_List_iter_c;_tmp122;});_tmp67A(Cyc_IOEffect_comb_comb,& comb_par,newcomb);});});
if(defstmt != 0){
# 908
({({void(*_tmp67C)(void(*f)(struct Cyc_Absyn_Stmt*,struct _tuple14*),struct Cyc_Absyn_Stmt*env,struct Cyc_List_List*x)=({void(*_tmp123)(void(*f)(struct Cyc_Absyn_Stmt*,struct _tuple14*),struct Cyc_Absyn_Stmt*env,struct Cyc_List_List*x)=(void(*)(void(*f)(struct Cyc_Absyn_Stmt*,struct _tuple14*),struct Cyc_Absyn_Stmt*env,struct Cyc_List_List*x))Cyc_List_iter_c;_tmp123;});struct Cyc_Absyn_Stmt*_tmp67B=defstmt;_tmp67C(Cyc_IOEffect_defstmt_comb,_tmp67B,comb);});});
if(any && !set_def){
# 911
set_def=1;
comb=({struct Cyc_List_List*_tmp126=_cycalloc(sizeof(*_tmp126));({struct _tuple14*_tmp67F=({struct _tuple14*_tmp125=_cycalloc(sizeof(*_tmp125));({struct _tuple0*_tmp67E=({Cyc_Absyn_any_qvar();});_tmp125->f1=_tmp67E;}),({struct Cyc_List_List*_tmp67D=({struct Cyc_List_List*_tmp124=_cycalloc(sizeof(*_tmp124));_tmp124->hd=defstmt,_tmp124->tl=0;_tmp124;});_tmp125->f2=_tmp67D;}),_tmp125->f3=0;_tmp125;});_tmp126->hd=_tmp67F;}),_tmp126->tl=comb;_tmp126;});}
# 909
break;}
# 906
if(
# 916
!any && !({({int(*_tmp680)(int(*pred)(struct _tuple14*),struct Cyc_List_List*x)=({int(*_tmp127)(int(*pred)(struct _tuple14*),struct Cyc_List_List*x)=(int(*)(int(*pred)(struct _tuple14*),struct Cyc_List_List*x))Cyc_List_exists;_tmp127;});_tmp680(Cyc_IOEffect_is_uncaught,comb);});}))break;}}}
# 918
if(any && !set_def)comb=({struct Cyc_List_List*_tmp12D=_cycalloc(sizeof(*_tmp12D));({struct _tuple14*_tmp682=({struct _tuple14*_tmp12C=_cycalloc(sizeof(*_tmp12C));({struct _tuple0*_tmp681=({Cyc_Absyn_any_qvar();});_tmp12C->f1=_tmp681;}),_tmp12C->f2=0,_tmp12C->f3=1;_tmp12C;});_tmp12D->hd=_tmp682;}),_tmp12D->tl=comb;_tmp12D;});return comb;}
# 924
static void Cyc_IOEffect_register_exp_succs_generic(struct Cyc_IOEffect_Fenv*env,struct Cyc_Absyn_Exp*e,struct Cyc_List_List*ql,int any){
# 927
if(ql == 0 && any == 0)({({int(*_tmp683)(struct _fat_ptr msg)=({int(*_tmp130)(struct _fat_ptr msg)=(int(*)(struct _fat_ptr msg))Cyc_IOEffect_env_err;_tmp130;});_tmp683(({const char*_tmp131="register_exp_succs";_tag_fat(_tmp131,sizeof(char),19U);}));});});{struct Cyc_List_List*comb=({Cyc_IOEffect_derive_comblist(env->catch_stack,ql,any);});
# 934
if(({Cyc_Absyn_is_throwsany(env->annot);})== 0)
({Cyc_IOEffect_check_exn_annot(e->loc,env->annot,comb);});
# 934
({Cyc_IOEffect_add_throws_effect(env,e,comb);});}}
# 940
static void Cyc_IOEffect_register_exp_succs(struct Cyc_IOEffect_Fenv*env,struct Cyc_Absyn_Exp*e,struct Cyc_List_List*ql){
({Cyc_IOEffect_register_exp_succs_generic(env,e,ql,0);});}
# 943
static void Cyc_IOEffect_register_exp_throwsany(struct Cyc_IOEffect_Fenv*env,struct Cyc_Absyn_Exp*e){
# 945
({Cyc_IOEffect_register_exp_succs_generic(env,e,0,1);});}
# 947
static void Cyc_IOEffect_register_exp_succ(struct Cyc_IOEffect_Fenv*env,struct Cyc_Absyn_Exp*e,struct _tuple0*q){
struct Cyc_List_List l=({struct Cyc_List_List _tmp586;_tmp586.hd=q,_tmp586.tl=0;_tmp586;});({Cyc_IOEffect_register_exp_succs(env,e,& l);});}
# 955
static void Cyc_IOEffect_add_env_exn(struct Cyc_IOEffect_Fenv*te,struct Cyc_Absyn_Exp*e,struct _tuple0*q){
# 957
struct Cyc_List_List*x=*({({struct Cyc_List_List**(*_tmp685)(struct Cyc_List_List**ptr,struct _fat_ptr msg)=({struct Cyc_List_List**(*_tmp136)(struct Cyc_List_List**ptr,struct _fat_ptr msg)=(struct Cyc_List_List**(*)(struct Cyc_List_List**ptr,struct _fat_ptr msg))Cyc_IOEffect_safe_cast;_tmp136;});struct Cyc_List_List**_tmp684=te->throws;_tmp685(_tmp684,({const char*_tmp137="add_env_exn";_tag_fat(_tmp137,sizeof(char),12U);}));});});
({Cyc_IOEffect_register_exp_succ(te,e,q);});
({struct Cyc_List_List*_tmp686=(void*)({Cyc_Absyn_add_qvar((struct Cyc_List_List*)((struct Cyc_List_List*)_check_null(x))->hd,q);});x->hd=_tmp686;});}
# 962
static struct _tuple0*Cyc_IOEffect_conv(struct Cyc_Absyn_Datatypefield*d){return d->name;}
# 964
static void Cyc_IOEffect_add_throws(struct Cyc_IOEffect_Env*te,struct Cyc_Absyn_Exp*e){
# 966
void*t=({({void*_tmp687=e->topt;Cyc_IOEffect_safe_cast(_tmp687,({const char*_tmp140="add_throws";_tag_fat(_tmp140,sizeof(char),11U);}));});});
unsigned loc=e->loc;
struct Cyc_List_List*iter=({Cyc_IOEffect_fn_throw_annot(t);});
struct Cyc_IOEffect_Fenv*fenv=0;
fenv=te->fenv;
if(({Cyc_Absyn_is_nothrow(iter);}))return;else{
if(({Cyc_Absyn_is_throwsany(iter);}))
# 974
({Cyc_IOEffect_register_exp_throwsany(fenv,e);});else{
# 978
struct Cyc_List_List*x=*({({struct Cyc_List_List**(*_tmp689)(struct Cyc_List_List**ptr,struct _fat_ptr msg)=({struct Cyc_List_List**(*_tmp13E)(struct Cyc_List_List**ptr,struct _fat_ptr msg)=(struct Cyc_List_List**(*)(struct Cyc_List_List**ptr,struct _fat_ptr msg))Cyc_IOEffect_safe_cast;_tmp13E;});struct Cyc_List_List**_tmp688=(te->fenv)->throws;_tmp689(_tmp688,({const char*_tmp13F="add_env_exn";_tag_fat(_tmp13F,sizeof(char),12U);}));});});
struct Cyc_List_List*qvs=({({struct Cyc_List_List*(*_tmp68A)(struct _tuple0*(*f)(struct Cyc_Absyn_Datatypefield*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp13D)(struct _tuple0*(*f)(struct Cyc_Absyn_Datatypefield*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct _tuple0*(*f)(struct Cyc_Absyn_Datatypefield*),struct Cyc_List_List*x))Cyc_List_map;_tmp13D;});_tmp68A(Cyc_IOEffect_conv,iter);});});
({Cyc_IOEffect_register_exp_succs(te->fenv,e,qvs);});
for(0;iter != 0;iter=iter->tl){
({struct Cyc_List_List*_tmp68B=(void*)({struct Cyc_List_List*(*_tmp13A)(struct Cyc_List_List*t,struct _tuple0*q)=Cyc_Absyn_add_qvar;struct Cyc_List_List*_tmp13B=(struct Cyc_List_List*)((struct Cyc_List_List*)_check_null(x))->hd;struct _tuple0*_tmp13C=({Cyc_Absyn_throws_hd2qvar(iter);});_tmp13A(_tmp13B,_tmp13C);});x->hd=_tmp68B;});}}}}struct _tuple23{struct _tuple0*f1;int f2;};
# 986
static struct _tuple23 Cyc_IOEffect_etyp2qvar(struct Cyc_Absyn_Exp*e){
# 988
void*t=({void*(*_tmp158)(void*)=Cyc_Tcutil_compress;void*_tmp159=({({void*_tmp68C=e->topt;Cyc_IOEffect_safe_cast(_tmp68C,({const char*_tmp15A="etyp2qvar";_tag_fat(_tmp15A,sizeof(char),10U);}));});});_tmp158(_tmp159);});
void*_stmttmp13=({Cyc_Tcutil_compress(t);});void*_tmp142=_stmttmp13;void*_tmp143;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp142)->tag == 3U){_LL1: _tmp143=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp142)->f1).elt_type;_LL2: {void*t1=_tmp143;
# 992
t1=({Cyc_Tcutil_compress(t1);});
{void*_tmp144=t1;struct Cyc_List_List*_tmp146;union Cyc_Absyn_DatatypeFieldInfo _tmp145;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp144)->tag == 0U)switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp144)->f1)){case 21U: _LL6: _tmp145=((struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp144)->f1)->f1;_tmp146=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp144)->f2;_LL7: {union Cyc_Absyn_DatatypeFieldInfo tuf_info=_tmp145;struct Cyc_List_List*ts=_tmp146;
# 997
{union Cyc_Absyn_DatatypeFieldInfo _tmp147=tuf_info;struct _tuple0*_tmp14A;int _tmp149;struct _tuple0*_tmp148;int _tmp14D;struct _tuple0*_tmp14C;struct _tuple0*_tmp14B;if((_tmp147.UnknownDatatypefield).tag == 1){_LLD: _tmp14B=((_tmp147.UnknownDatatypefield).val).datatype_name;_tmp14C=((_tmp147.UnknownDatatypefield).val).field_name;_tmp14D=((_tmp147.UnknownDatatypefield).val).is_extensible;_LLE: {struct _tuple0*tname=_tmp14B;struct _tuple0*fname=_tmp14C;int is_x=_tmp14D;
# 1002
_tmp148=tname;_tmp149=is_x;_tmp14A=fname;goto _LL10;}}else{_LLF: _tmp148=(((_tmp147.KnownDatatypefield).val).f1)->name;_tmp149=(((_tmp147.KnownDatatypefield).val).f1)->is_extensible;_tmp14A=(((_tmp147.KnownDatatypefield).val).f2)->name;_LL10: {struct _tuple0*tname=_tmp148;int is_x=_tmp149;struct _tuple0*fname=_tmp14A;
# 1006
return({struct _tuple23 _tmp587;_tmp587.f1=fname,_tmp587.f2=0;_tmp587;});}}_LLC:;}
# 1008
goto _LL5;}case 20U: _LL8: _LL9: {
# 1010
void*_stmttmp14=e->r;void*_tmp14E=_stmttmp14;struct _tuple0*_tmp14F;if(((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp14E)->tag == 1U){if(((struct Cyc_Absyn_Local_b_Absyn_Binding_struct*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp14E)->f1)->tag == 4U){_LL12: _tmp14F=(((struct Cyc_Absyn_Local_b_Absyn_Binding_struct*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp14E)->f1)->f1)->name;_LL13: {struct _tuple0*name=_tmp14F;
# 1017
if(({char*_tmp68D=(char*)((*name).f2)->curr;_tmp68D == (char*)({Cyc_Absyn_exnstr();}).curr;}))return({struct _tuple23 _tmp588;_tmp588.f1=0,_tmp588.f2=1;_tmp588;});return({struct _tuple23 _tmp589;_tmp589.f1=0,_tmp589.f2=2;_tmp589;});}}else{goto _LL14;}}else{_LL14: _LL15:
# 1019
 return({struct _tuple23 _tmp58A;_tmp58A.f1=0,_tmp58A.f2=3;_tmp58A;});}_LL11:;}default: goto _LLA;}else{_LLA: _LLB:
# 1021
({struct Cyc_String_pa_PrintArg_struct _tmp153=({struct Cyc_String_pa_PrintArg_struct _tmp58B;_tmp58B.tag=0U,({struct _fat_ptr _tmp68E=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t);}));_tmp58B.f1=_tmp68E;});_tmp58B;});void*_tmp150[1U];_tmp150[0]=& _tmp153;({int(*_tmp690)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp152)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp152;});struct _fat_ptr _tmp68F=({const char*_tmp151="etyp2qvar : %s";_tag_fat(_tmp151,sizeof(char),15U);});_tmp690(_tmp68F,_tag_fat(_tmp150,sizeof(void*),1U));});});}_LL5:;}
# 1023
goto _LL0;}}else{_LL3: _LL4:
({struct Cyc_String_pa_PrintArg_struct _tmp157=({struct Cyc_String_pa_PrintArg_struct _tmp58C;_tmp58C.tag=0U,({struct _fat_ptr _tmp691=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t);}));_tmp58C.f1=_tmp691;});_tmp58C;});void*_tmp154[1U];_tmp154[0]=& _tmp157;({int(*_tmp693)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp156)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp156;});struct _fat_ptr _tmp692=({const char*_tmp155="etyp2qvar : %s.";_tag_fat(_tmp155,sizeof(char),16U);});_tmp693(_tmp692,_tag_fat(_tmp154,sizeof(void*),1U));});});}_LL0:;}
# 1028
static void Cyc_IOEffect_add_exception(struct Cyc_IOEffect_Env*te,struct Cyc_Absyn_Exp*e0,struct Cyc_Absyn_Exp*e){
# 1030
struct _tuple23 _stmttmp15=({Cyc_IOEffect_etyp2qvar(e);});int _tmp15D;struct _tuple0*_tmp15C;_LL1: _tmp15C=_stmttmp15.f1;_tmp15D=_stmttmp15.f2;_LL2: {struct _tuple0*q=_tmp15C;int b=_tmp15D;
if(q != 0)
({Cyc_IOEffect_add_env_exn(te->fenv,e0,q);});else{
int _tmp15E=b;if(_tmp15E == 1){_LL4: _LL5:
# 1035
 goto _LL3;}else{_LL6: _LL7:
({Cyc_IOEffect_register_exp_throwsany(te->fenv,e);});goto _LL3;}_LL3:;}}}
# 1049 "ioeffect.cyc"
static void Cyc_IOEffect_insert_badalloc_check(struct Cyc_IOEffect_Fenv*env,struct Cyc_Absyn_Exp*e){
# 1051
if(({Cyc_Absyn_get_debug();}))({void*_tmp160=0U;({unsigned _tmp695=e->loc;struct _fat_ptr _tmp694=({const char*_tmp161="inserted bad alloc check.";_tag_fat(_tmp161,sizeof(char),26U);});Cyc_Warn_warn(_tmp695,_tmp694,_tag_fat(_tmp160,sizeof(void*),0U));});});({void(*_tmp162)(struct Cyc_IOEffect_Fenv*te,struct Cyc_Absyn_Exp*e,struct _tuple0*q)=Cyc_IOEffect_add_env_exn;struct Cyc_IOEffect_Fenv*_tmp163=env;struct Cyc_Absyn_Exp*_tmp164=e;struct _tuple0*_tmp165=({Cyc_Absyn_get_qvar(Cyc_Absyn_De_AllocCheck);});_tmp162(_tmp163,_tmp164,_tmp165);});}
# 1055
static void Cyc_IOEffect_insert_null_check(struct Cyc_IOEffect_Fenv*env,struct Cyc_Absyn_Exp*e,struct _fat_ptr msg){
# 1057
if(({Cyc_Absyn_get_debug();}))
({struct Cyc_String_pa_PrintArg_struct _tmp169=({struct Cyc_String_pa_PrintArg_struct _tmp58E;_tmp58E.tag=0U,_tmp58E.f1=(struct _fat_ptr)((struct _fat_ptr)msg);_tmp58E;});struct Cyc_String_pa_PrintArg_struct _tmp16A=({struct Cyc_String_pa_PrintArg_struct _tmp58D;_tmp58D.tag=0U,({
struct _fat_ptr _tmp696=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_exp2string(e);}));_tmp58D.f1=_tmp696;});_tmp58D;});void*_tmp167[2U];_tmp167[0]=& _tmp169,_tmp167[1]=& _tmp16A;({unsigned _tmp698=e->loc;struct _fat_ptr _tmp697=({const char*_tmp168="inserted NULL check (%s, expression = %s).";_tag_fat(_tmp168,sizeof(char),43U);});Cyc_Warn_warn(_tmp698,_tmp697,_tag_fat(_tmp167,sizeof(void*),2U));});});
# 1057
({void(*_tmp16B)(struct Cyc_IOEffect_Fenv*te,struct Cyc_Absyn_Exp*e,struct _tuple0*q)=Cyc_IOEffect_add_env_exn;struct Cyc_IOEffect_Fenv*_tmp16C=env;struct Cyc_Absyn_Exp*_tmp16D=e;struct _tuple0*_tmp16E=({Cyc_Absyn_get_qvar(Cyc_Absyn_De_NullCheck);});_tmp16B(_tmp16C,_tmp16D,_tmp16E);});}
# 1066
static void Cyc_IOEffect_insert_array_bounds_check(struct Cyc_IOEffect_Fenv*env,struct Cyc_Absyn_Exp*e){
# 1068
if(({Cyc_Absyn_get_debug();}))({void*_tmp170=0U;({unsigned _tmp69A=e->loc;struct _fat_ptr _tmp699=({const char*_tmp171="inserted BOUNDS check.";_tag_fat(_tmp171,sizeof(char),23U);});Cyc_Warn_warn(_tmp69A,_tmp699,_tag_fat(_tmp170,sizeof(void*),0U));});});({void(*_tmp172)(struct Cyc_IOEffect_Fenv*te,struct Cyc_Absyn_Exp*e,struct _tuple0*q)=Cyc_IOEffect_add_env_exn;struct Cyc_IOEffect_Fenv*_tmp173=env;struct Cyc_Absyn_Exp*_tmp174=e;struct _tuple0*_tmp175=({Cyc_Absyn_get_qvar(Cyc_Absyn_De_BoundsCheck);});_tmp172(_tmp173,_tmp174,_tmp175);});}
# 1072
static void Cyc_IOEffect_insert_match_check(struct Cyc_IOEffect_Fenv*env,struct Cyc_Absyn_Exp*e){
# 1074
if(({Cyc_Absyn_get_debug();}))({void*_tmp177=0U;({unsigned _tmp69C=e->loc;struct _fat_ptr _tmp69B=({const char*_tmp178="inserted MATCH check.";_tag_fat(_tmp178,sizeof(char),22U);});Cyc_Warn_warn(_tmp69C,_tmp69B,_tag_fat(_tmp177,sizeof(void*),0U));});});({void(*_tmp179)(struct Cyc_IOEffect_Fenv*te,struct Cyc_Absyn_Exp*e,struct _tuple0*q)=Cyc_IOEffect_add_env_exn;struct Cyc_IOEffect_Fenv*_tmp17A=env;struct Cyc_Absyn_Exp*_tmp17B=e;struct _tuple0*_tmp17C=({Cyc_Absyn_get_qvar(Cyc_Absyn_De_PatternCheck);});_tmp179(_tmp17A,_tmp17B,_tmp17C);});}
# 1078
static void Cyc_IOEffect_insert_array_bounds_null_check(struct Cyc_IOEffect_Fenv*env,struct Cyc_Absyn_Exp*e){
# 1080
if(({Cyc_Absyn_get_debug();}))({void*_tmp17E=0U;({unsigned _tmp69E=e->loc;struct _fat_ptr _tmp69D=({const char*_tmp17F="inserted BOUNDS and NULL check.";_tag_fat(_tmp17F,sizeof(char),32U);});Cyc_Warn_warn(_tmp69E,_tmp69D,_tag_fat(_tmp17E,sizeof(void*),0U));});});({void(*_tmp180)(struct Cyc_IOEffect_Fenv*te,struct Cyc_Absyn_Exp*e,struct _tuple0*q)=Cyc_IOEffect_add_env_exn;struct Cyc_IOEffect_Fenv*_tmp181=env;struct Cyc_Absyn_Exp*_tmp182=e;struct _tuple0*_tmp183=({Cyc_Absyn_get_qvar(Cyc_Absyn_De_BoundsCheck);});_tmp180(_tmp181,_tmp182,_tmp183);});
# 1082
({void(*_tmp184)(struct Cyc_IOEffect_Fenv*te,struct Cyc_Absyn_Exp*e,struct _tuple0*q)=Cyc_IOEffect_add_env_exn;struct Cyc_IOEffect_Fenv*_tmp185=env;struct Cyc_Absyn_Exp*_tmp186=e;struct _tuple0*_tmp187=({Cyc_Absyn_get_qvar(Cyc_Absyn_De_NullCheck);});_tmp184(_tmp185,_tmp186,_tmp187);});}
# 1085
static int Cyc_IOEffect_is_tagged_union_project(struct Cyc_Absyn_Exp*e2){
# 1087
void*ignore_typ=Cyc_Absyn_void_type;
int ignore_bool=0;
int ignore_int=0;
return({Cyc_Toc_is_tagged_union_project(e2,& ignore_int,& ignore_typ,1);});}
# 1094
static int Cyc_IOEffect_is_zero_ptr_deref(struct Cyc_Absyn_Exp*e){
# 1096
void*ptr_type=Cyc_Absyn_void_type;
void*elt_type=Cyc_Absyn_void_type;
int is_fat=0;
return({Cyc_Tcutil_is_zero_ptr_deref(e,& ptr_type,& is_fat,& elt_type);});}struct _tuple24{struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};
# 1117 "ioeffect.cyc"
static void Cyc_IOEffect_add_check_exception(struct Cyc_IOEffect_Env*env,struct Cyc_Absyn_Exp*e,void*annot){
# 1121
void*old_typ=({void*(*_tmp1B7)(void*)=Cyc_Tcutil_compress;void*_tmp1B8=({({void*_tmp69F=e->topt;Cyc_IOEffect_safe_cast(_tmp69F,({const char*_tmp1B9="add_check_exception";_tag_fat(_tmp1B9,sizeof(char),20U);}));});});_tmp1B7(_tmp1B8);});
# 1123
struct Cyc_IOEffect_Fenv*fenv=env->fenv;
int what=0;
# 1130
{void*_tmp18B=old_typ;void*_tmp190;void*_tmp18F;void*_tmp18E;struct Cyc_Absyn_Tqual _tmp18D;void*_tmp18C;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp18B)->tag == 3U){_LL1: _tmp18C=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp18B)->f1).elt_type;_tmp18D=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp18B)->f1).elt_tq;_tmp18E=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp18B)->f1).ptr_atts).nullable;_tmp18F=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp18B)->f1).ptr_atts).bounds;_tmp190=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp18B)->f1).ptr_atts).zero_term;_LL2: {void*ta=_tmp18C;struct Cyc_Absyn_Tqual tq=_tmp18D;void*nbl=_tmp18E;void*b=_tmp18F;void*zt=_tmp190;
# 1133
{void*_tmp191=annot;struct Cyc_Absyn_Exp*_tmp192;struct Cyc_Absyn_Exp*_tmp193;if(((struct Cyc_InsertChecks_NullOnly_Absyn_AbsynAnnot_struct*)_tmp191)->tag == Cyc_InsertChecks_NullOnly){_LL6: _LL7:
# 1138
({({struct Cyc_IOEffect_Fenv*_tmp6A1=fenv;struct Cyc_Absyn_Exp*_tmp6A0=e;Cyc_IOEffect_insert_null_check(_tmp6A1,_tmp6A0,({const char*_tmp194="debug info: 1";_tag_fat(_tmp194,sizeof(char),14U);}));});});
goto _LL5;}else{if(((struct Cyc_InsertChecks_NullAndThinBound_Absyn_AbsynAnnot_struct*)_tmp191)->tag == Cyc_InsertChecks_NullAndThinBound){_LL8: _tmp193=((struct Cyc_InsertChecks_NullAndThinBound_Absyn_AbsynAnnot_struct*)_tmp191)->f1;_LL9: {struct Cyc_Absyn_Exp*bd=_tmp193;
# 1143
what=1;
_tmp192=bd;goto _LLB;}}else{if(((struct Cyc_InsertChecks_ThinBound_Absyn_AbsynAnnot_struct*)_tmp191)->tag == Cyc_InsertChecks_ThinBound){_LLA: _tmp192=((struct Cyc_InsertChecks_ThinBound_Absyn_AbsynAnnot_struct*)_tmp191)->f1;_LLB: {struct Cyc_Absyn_Exp*bd=_tmp192;
# 1146
struct _tuple13 _stmttmp16=({Cyc_Evexp_eval_const_uint_exp(bd);});int _tmp196;unsigned _tmp195;_LL13: _tmp195=_stmttmp16.f1;_tmp196=_stmttmp16.f2;_LL14: {unsigned i=_tmp195;int valid=_tmp196;
if((valid && i == (unsigned)1)&&({Cyc_Tcutil_is_zeroterm_pointer_type(old_typ);})){
# 1151
if(!({Cyc_Toc_is_zero(bd);})){
# 1153
if(what == 1)
({Cyc_IOEffect_insert_array_bounds_null_check(fenv,e);});}else{
# 1156
if(what == 0)
({Cyc_IOEffect_insert_array_bounds_check(fenv,e);});else{
if(what == 1)
({Cyc_IOEffect_insert_array_bounds_null_check(fenv,e);});}}}else{
# 1163
if(what == 0)({Cyc_IOEffect_insert_array_bounds_check(fenv,e);});else{
if(what == 1)
({Cyc_IOEffect_insert_array_bounds_null_check(fenv,e);});}}
# 1167
goto _LL5;}}}else{if(((struct Cyc_InsertChecks_NullAndFatBound_Absyn_AbsynAnnot_struct*)_tmp191)->tag == Cyc_InsertChecks_NullAndFatBound){_LLC: _LLD:
 goto _LLF;}else{if(((struct Cyc_InsertChecks_FatBound_Absyn_AbsynAnnot_struct*)_tmp191)->tag == Cyc_InsertChecks_FatBound){_LLE: _LLF:
# 1170
({Cyc_IOEffect_insert_array_bounds_check(fenv,e);});
goto _LL5;}else{_LL10: _LL11:
 goto _LL5;}}}}}_LL5:;}
# 1174
goto _LL0;}}else{_LL3: _LL4:
 goto _LL0;}_LL0:;}{
# 1178
void*_stmttmp17=e->r;void*_tmp197=_stmttmp17;int _tmp19A;int _tmp199;struct Cyc_Absyn_Exp*_tmp198;int _tmp19D;int _tmp19C;struct Cyc_Absyn_Exp*_tmp19B;struct Cyc_Core_Opt*_tmp19F;struct Cyc_Absyn_Exp*_tmp19E;enum Cyc_Absyn_Incrementor _tmp1A1;struct Cyc_Absyn_Exp*_tmp1A0;enum Cyc_Absyn_Coercion _tmp1A5;int _tmp1A4;struct Cyc_Absyn_Exp*_tmp1A3;void*_tmp1A2;struct Cyc_Absyn_Exp*_tmp1A6;struct Cyc_Absyn_Exp*_tmp1A7;switch(*((int*)_tmp197)){case 11U: _LL16: _tmp1A7=((struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct*)_tmp197)->f1;_LL17: {struct Cyc_Absyn_Exp*e1=_tmp1A7;
# 1180
({Cyc_IOEffect_add_exception(env,e,e1);});goto _LL15;}case 35U: _LL18: _LL19:
 goto _LL1B;case 16U: _LL1A: _LL1B:
({Cyc_IOEffect_insert_badalloc_check(fenv,e);});goto _LL15;case 10U: _LL1C: _tmp1A6=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp197)->f1;_LL1D: {struct Cyc_Absyn_Exp*e1=_tmp1A6;
# 1184
if(({Cyc_Toc_do_null_check(e);}))({({struct Cyc_IOEffect_Fenv*_tmp6A3=fenv;struct Cyc_Absyn_Exp*_tmp6A2=e;Cyc_IOEffect_insert_null_check(_tmp6A3,_tmp6A2,({const char*_tmp1A8="debug info: 2";_tag_fat(_tmp1A8,sizeof(char),14U);}));});});{void*ftyp=({({void*_tmp6A4=e1->topt;Cyc_IOEffect_safe_cast(_tmp6A4,({const char*_tmp1A9="exp";_tag_fat(_tmp1A9,sizeof(char),4U);}));});});
# 1186
struct Cyc_Absyn_FnInfo i=({Cyc_IOEffect_fn_type(ftyp);});({Cyc_IOEffect_add_throws(env,e1);});goto _LL15;
goto _LL15;}}case 14U: _LL1E: _tmp1A2=(void*)((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp197)->f1;_tmp1A3=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp197)->f2;_tmp1A4=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp197)->f3;_tmp1A5=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp197)->f4;_LL1F: {void*new_typ=_tmp1A2;struct Cyc_Absyn_Exp*e1=_tmp1A3;int user_inserted=_tmp1A4;enum Cyc_Absyn_Coercion coercion=_tmp1A5;
# 1189
void*old_t2=({({void*_tmp6A5=e1->topt;Cyc_IOEffect_safe_cast(_tmp6A5,({const char*_tmp1B6="add_check_exception";_tag_fat(_tmp1B6,sizeof(char),20U);}));});});
# 1191
{struct _tuple15 _stmttmp18=({struct _tuple15 _tmp590;({void*_tmp6A7=({Cyc_Tcutil_compress(old_t2);});_tmp590.f1=_tmp6A7;}),({void*_tmp6A6=({Cyc_Tcutil_compress(new_typ);});_tmp590.f2=_tmp6A6;});_tmp590;});struct _tuple15 _tmp1AA=_stmttmp18;struct Cyc_Absyn_PtrInfo _tmp1AC;struct Cyc_Absyn_PtrInfo _tmp1AB;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp1AA.f1)->tag == 3U){if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp1AA.f2)->tag == 3U){_LL2B: _tmp1AB=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp1AA.f1)->f1;_tmp1AC=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp1AA.f2)->f1;_LL2C: {struct Cyc_Absyn_PtrInfo p1=_tmp1AB;struct Cyc_Absyn_PtrInfo p2=_tmp1AC;
# 1194
struct Cyc_Absyn_Exp*b1=({struct Cyc_Absyn_Exp*(*_tmp1B3)(void*def,void*b)=Cyc_Tcutil_get_bounds_exp;void*_tmp1B4=({Cyc_Absyn_bounds_one();});void*_tmp1B5=(p1.ptr_atts).bounds;_tmp1B3(_tmp1B4,_tmp1B5);});
# 1196
struct Cyc_Absyn_Exp*b2=({struct Cyc_Absyn_Exp*(*_tmp1B0)(void*def,void*b)=Cyc_Tcutil_get_bounds_exp;void*_tmp1B1=({Cyc_Absyn_bounds_one();});void*_tmp1B2=(p2.ptr_atts).bounds;_tmp1B0(_tmp1B1,_tmp1B2);});
# 1205
{struct _tuple24 _stmttmp19=({struct _tuple24 _tmp58F;_tmp58F.f1=b1,_tmp58F.f2=b2;_tmp58F;});struct _tuple24 _tmp1AD=_stmttmp19;if(_tmp1AD.f1 != 0){if(_tmp1AD.f2 != 0){_LL30: _LL31:
# 1208
 if(({Cyc_Toc_do_null_check(e);}))({({struct Cyc_IOEffect_Fenv*_tmp6A9=fenv;struct Cyc_Absyn_Exp*_tmp6A8=e;Cyc_IOEffect_insert_null_check(_tmp6A9,_tmp6A8,({const char*_tmp1AE="debug info: 3";_tag_fat(_tmp1AE,sizeof(char),14U);}));});});goto _LL2F;}else{goto _LL34;}}else{if(_tmp1AD.f2 != 0){_LL32: _LL33:
# 1211
 if(({Cyc_Toc_do_null_check(e);}))({({struct Cyc_IOEffect_Fenv*_tmp6AB=fenv;struct Cyc_Absyn_Exp*_tmp6AA=e;Cyc_IOEffect_insert_null_check(_tmp6AB,_tmp6AA,({const char*_tmp1AF="debug info: 4";_tag_fat(_tmp1AF,sizeof(char),14U);}));});});goto _LL2F;}else{_LL34: _LL35:
# 1213
 goto _LL2F;}}_LL2F:;}
# 1215
goto _LL2A;}}else{goto _LL2D;}}else{_LL2D: _LL2E:
 goto _LL2A;}_LL2A:;}
# 1218
goto _LL15;}case 5U: _LL20: _tmp1A0=((struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*)_tmp197)->f1;_tmp1A1=((struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*)_tmp197)->f2;_LL21: {struct Cyc_Absyn_Exp*e2=_tmp1A0;enum Cyc_Absyn_Incrementor incr=_tmp1A1;
# 1221
void*elt_typ=Cyc_Absyn_void_type;
if(({Cyc_IOEffect_is_tagged_union_project(e2);})){_tmp19E=e2;_tmp19F=0;goto _LL23;}else{
# 1224
if((!({Cyc_Tcutil_is_fat_pointer_type_elt(old_typ,& elt_typ);})&&({Cyc_Tcutil_is_zero_pointer_type_elt(old_typ,& elt_typ);}))&&(int)incr != (int)1U)
# 1228
({Cyc_IOEffect_insert_array_bounds_check(fenv,e2);});}
# 1222
goto _LL15;}case 4U: _LL22: _tmp19E=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp197)->f1;_tmp19F=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp197)->f2;_LL23: {struct Cyc_Absyn_Exp*e1=_tmp19E;struct Cyc_Core_Opt*popt=_tmp19F;
# 1231
if(({Cyc_IOEffect_is_tagged_union_project(e1);})&& popt != 0)
({Cyc_IOEffect_insert_match_check(fenv,e1);});
# 1231
goto _LL15;
# 1234
if(({Cyc_IOEffect_is_zero_ptr_deref(e1);}))
({Cyc_IOEffect_insert_array_bounds_check(fenv,e1);});
# 1234
goto _LL15;}case 21U: _LL24: _tmp19B=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp197)->f1;_tmp19C=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp197)->f3;_tmp19D=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp197)->f4;_LL25: {struct Cyc_Absyn_Exp*e1=_tmp19B;int is_tagged=_tmp19C;int is_read=_tmp19D;
# 1239
_tmp198=e1;_tmp199=is_tagged;_tmp19A=is_read;goto _LL27;}case 22U: _LL26: _tmp198=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp197)->f1;_tmp199=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp197)->f3;_tmp19A=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp197)->f4;_LL27: {struct Cyc_Absyn_Exp*e1=_tmp198;int is_tagged=_tmp199;int is_read=_tmp19A;
# 1241
if(is_tagged && is_read)({Cyc_IOEffect_insert_match_check(fenv,e1);});goto _LL15;}default: _LL28: _LL29:
# 1245
 goto _LL15;}_LL15:;}}
# 1250
static struct _fat_ptr Cyc_IOEffect_q2string(struct _tuple0*q){
# 1252
static struct _tuple0*any=0;
if(!((unsigned)any))any=({struct _tuple0*(*_tmp1BB)(struct Cyc_List_List*t)=Cyc_Absyn_throws_hd2qvar;struct Cyc_List_List*_tmp1BC=({Cyc_Absyn_throwsany();});_tmp1BB(_tmp1BC);});if(({Cyc_strptrcmp((*((struct _tuple0*)_check_null(any))).f2,(*q).f2);})== 0)
# 1255
return({const char*_tmp1BD="<any exception>";_tag_fat(_tmp1BD,sizeof(char),16U);});else{
return({Cyc_Absynpp_qvar2string(q);});}}
# 1259
static struct _fat_ptr Cyc_IOEffect_qvl2string(struct Cyc_List_List*l){
# 1261
if(l == 0)return({const char*_tmp1BF="";_tag_fat(_tmp1BF,sizeof(char),1U);});{struct _fat_ptr x=({struct Cyc_String_pa_PrintArg_struct _tmp1C9=({struct Cyc_String_pa_PrintArg_struct _tmp594;_tmp594.tag=0U,({
struct _fat_ptr _tmp6AC=(struct _fat_ptr)((struct _fat_ptr)({Cyc_IOEffect_q2string((struct _tuple0*)l->hd);}));_tmp594.f1=_tmp6AC;});_tmp594;});void*_tmp1C7[1U];_tmp1C7[0]=& _tmp1C9;({struct _fat_ptr _tmp6AD=({const char*_tmp1C8="{%s";_tag_fat(_tmp1C8,sizeof(char),4U);});Cyc_aprintf(_tmp6AD,_tag_fat(_tmp1C7,sizeof(void*),1U));});});
for(l=l->tl;l != 0;l=l->tl){
# 1265
x=({struct Cyc_String_pa_PrintArg_struct _tmp1C2=({struct Cyc_String_pa_PrintArg_struct _tmp592;_tmp592.tag=0U,_tmp592.f1=(struct _fat_ptr)((struct _fat_ptr)x);_tmp592;});struct Cyc_String_pa_PrintArg_struct _tmp1C3=({struct Cyc_String_pa_PrintArg_struct _tmp591;_tmp591.tag=0U,({struct _fat_ptr _tmp6AE=(struct _fat_ptr)((struct _fat_ptr)({Cyc_IOEffect_q2string((struct _tuple0*)l->hd);}));_tmp591.f1=_tmp6AE;});_tmp591;});void*_tmp1C0[2U];_tmp1C0[0]=& _tmp1C2,_tmp1C0[1]=& _tmp1C3;({struct _fat_ptr _tmp6AF=({const char*_tmp1C1="%s,%s";_tag_fat(_tmp1C1,sizeof(char),6U);});Cyc_aprintf(_tmp6AF,_tag_fat(_tmp1C0,sizeof(void*),2U));});});}
# 1267
return(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp1C6=({struct Cyc_String_pa_PrintArg_struct _tmp593;_tmp593.tag=0U,_tmp593.f1=(struct _fat_ptr)((struct _fat_ptr)x);_tmp593;});void*_tmp1C4[1U];_tmp1C4[0]=& _tmp1C6;({struct _fat_ptr _tmp6B0=({const char*_tmp1C5="%s}";_tag_fat(_tmp1C5,sizeof(char),4U);});Cyc_aprintf(_tmp6B0,_tag_fat(_tmp1C4,sizeof(void*),1U));});});}}
# 1270
void Cyc_IOEffect_push_throws_scope(struct Cyc_IOEffect_Env*te){
# 1272
struct Cyc_IOEffect_Fenv*fenv=te->fenv;
({struct Cyc_List_List*_tmp6B1=({struct Cyc_List_List*_tmp1CB=_cycalloc(sizeof(*_tmp1CB));_tmp1CB->hd=0,_tmp1CB->tl=*((struct Cyc_List_List**)_check_null(fenv->throws));_tmp1CB;});*((struct Cyc_List_List**)_check_null(fenv->throws))=_tmp6B1;});}
# 1276
void Cyc_IOEffect_pop_throws_scope(struct Cyc_IOEffect_Env*te,struct Cyc_IOEffect_Node*n){
# 1278
struct Cyc_IOEffect_Fenv*fenv=te->fenv;
if(*((struct Cyc_List_List**)_check_null(fenv->throws))== 0)
({({int(*_tmp6B2)(struct _fat_ptr msg)=({int(*_tmp1CD)(struct _fat_ptr msg)=(int(*)(struct _fat_ptr msg))Cyc_IOEffect_env_err;_tmp1CD;});_tmp6B2(({const char*_tmp1CE="IOEffect::pop_throws_scope";_tag_fat(_tmp1CE,sizeof(char),27U);}));});});else{
# 1283
struct Cyc_List_List*hd=(struct Cyc_List_List*)((struct Cyc_List_List*)_check_null(*((struct Cyc_List_List**)_check_null(fenv->throws))))->hd;
({Cyc_IOEffect_set_throws(n,hd);});
({struct Cyc_List_List*_tmp6B3=((struct Cyc_List_List*)_check_null(*((struct Cyc_List_List**)_check_null(fenv->throws))))->tl;*((struct Cyc_List_List**)_check_null(fenv->throws))=_tmp6B3;});
if(*((struct Cyc_List_List**)_check_null(fenv->throws))!= 0 &&(struct Cyc_List_List*)((struct Cyc_List_List*)_check_null(*((struct Cyc_List_List**)_check_null(fenv->throws))))->hd != 0)
for(0;hd != 0;hd=hd->tl){
({struct Cyc_List_List*_tmp6B4=(void*)({Cyc_Absyn_add_qvar((struct Cyc_List_List*)((struct Cyc_List_List*)_check_null(*((struct Cyc_List_List**)_check_null(fenv->throws))))->hd,(struct _tuple0*)hd->hd);});((struct Cyc_List_List*)_check_null(*((struct Cyc_List_List**)_check_null(fenv->throws))))->hd=_tmp6B4;});}}}
# 1300 "ioeffect.cyc"
static struct Cyc_IOEffect_Env*Cyc_IOEffect_new_env(struct Cyc_JumpAnalysis_Jump_Anal_Result*tables,struct Cyc_IOEffect_Fenv*f){
# 1302
return({struct Cyc_IOEffect_Env*_tmp1D0=_cycalloc(sizeof(*_tmp1D0));_tmp1D0->tables=tables,_tmp1D0->fenv=f,_tmp1D0->annot=0,_tmp1D0->const_effect=0,_tmp1D0->jlist=0;_tmp1D0;});}
# 1306
static void Cyc_IOEffect_add_jump_node(struct Cyc_IOEffect_Env*env,struct Cyc_IOEffect_Node*n){
# 1308
({struct Cyc_List_List*_tmp6B5=({struct Cyc_List_List*_tmp1D2=_cycalloc(sizeof(*_tmp1D2));_tmp1D2->hd=n,_tmp1D2->tl=env->jlist;_tmp1D2;});env->jlist=_tmp6B5;});}
# 1311
static void Cyc_IOEffect_rem_jump_node(struct Cyc_IOEffect_Env*env,struct Cyc_IOEffect_Node*n){
# 1313
struct Cyc_List_List*iter=env->jlist;
struct Cyc_List_List*prev=0;
for(0;iter != 0;(prev=iter,iter=iter->tl)){if((struct Cyc_IOEffect_Node*)iter->hd == n){
# 1317
if(prev == 0)env->jlist=iter->tl;else{
prev->tl=iter->tl;}
return;}}
# 1315
({({int(*_tmp6B6)(struct _fat_ptr msg)=({int(*_tmp1D4)(struct _fat_ptr msg)=(int(*)(struct _fat_ptr msg))Cyc_IOEffect_env_err;_tmp1D4;});_tmp6B6(({const char*_tmp1D5="rem_jump_node";_tag_fat(_tmp1D5,sizeof(char),14U);}));});});}
# 1330
static void Cyc_IOEffect_push_annot(struct Cyc_IOEffect_Env*env,void*annot){
# 1332
({struct Cyc_List_List*_tmp6B7=({struct Cyc_List_List*_tmp1D7=_cycalloc(sizeof(*_tmp1D7));_tmp1D7->hd=annot,_tmp1D7->tl=env->annot;_tmp1D7;});env->annot=_tmp6B7;});}
# 1335
static void Cyc_IOEffect_pop_annot(struct Cyc_IOEffect_Env*env){
# 1337
if(env->annot == 0)({({int(*_tmp6B8)(struct _fat_ptr msg)=({int(*_tmp1D9)(struct _fat_ptr msg)=(int(*)(struct _fat_ptr msg))Cyc_IOEffect_env_err;_tmp1D9;});_tmp6B8(({const char*_tmp1DA="pop_annot";_tag_fat(_tmp1DA,sizeof(char),10U);}));});});env->annot=((struct Cyc_List_List*)_check_null(env->annot))->tl;}
# 1340
static void*Cyc_IOEffect_peek_annot(struct Cyc_IOEffect_Env*env){
# 1342
if(env->annot == 0)({({int(*_tmp6B9)(struct _fat_ptr msg)=({int(*_tmp1DC)(struct _fat_ptr msg)=(int(*)(struct _fat_ptr msg))Cyc_IOEffect_env_err;_tmp1DC;});_tmp6B9(({const char*_tmp1DD="peek_annot";_tag_fat(_tmp1DD,sizeof(char),11U);}));});});return(void*)((struct Cyc_List_List*)_check_null(env->annot))->hd;}
# 1340
static void Cyc_IOEffect_analyze_fd(struct Cyc_JumpAnalysis_Jump_Anal_Result*tables,struct Cyc_Absyn_Fndecl*fd,unsigned loc);
# 1362 "ioeffect.cyc"
static struct _tuple0*Cyc_IOEffect_pat2qvar(struct Cyc_Absyn_Pat*p){
# 1364
void*_stmttmp1A=p->r;void*_tmp1DF=_stmttmp1A;struct Cyc_Absyn_Datatypefield*_tmp1E0;struct Cyc_Absyn_Pat*_tmp1E1;switch(*((int*)_tmp1DF)){case 6U: _LL1: _tmp1E1=((struct Cyc_Absyn_Pointer_p_Absyn_Raw_pat_struct*)_tmp1DF)->f1;_LL2: {struct Cyc_Absyn_Pat*p1=_tmp1E1;
# 1366
return({Cyc_IOEffect_pat2qvar(p1);});}case 8U: _LL3: _tmp1E0=((struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct*)_tmp1DF)->f2;_LL4: {struct Cyc_Absyn_Datatypefield*tuf=_tmp1E0;
return tuf->name;}case 0U: _LL5: _LL6:
 return({Cyc_Absyn_default_case_qvar();});default: _LL7: _LL8:
({int(*_tmp1E2)(struct _fat_ptr msg)=({int(*_tmp1E3)(struct _fat_ptr msg)=(int(*)(struct _fat_ptr msg))Cyc_IOEffect_env_err;_tmp1E3;});struct _fat_ptr _tmp1E4=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp1E7=({struct Cyc_String_pa_PrintArg_struct _tmp595;_tmp595.tag=0U,({
struct _fat_ptr _tmp6BA=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_pat2string(p);}));_tmp595.f1=_tmp6BA;});_tmp595;});void*_tmp1E5[1U];_tmp1E5[0]=& _tmp1E7;({struct _fat_ptr _tmp6BB=({const char*_tmp1E6="pat2var: \"%s\"";_tag_fat(_tmp1E6,sizeof(char),14U);});Cyc_aprintf(_tmp6BB,_tag_fat(_tmp1E5,sizeof(void*),1U));});});_tmp1E2(_tmp1E4);});}_LL0:;}
# 1362
static void Cyc_IOEffect_check_access(struct Cyc_IOEffect_Fenv*fe,unsigned loc,struct Cyc_List_List*f,void*t);
# 1378
static int Cyc_IOEffect_throws_exp_push(struct Cyc_IOEffect_Env*env,struct Cyc_Absyn_Exp*e){
# 1380
({Cyc_IOEffect_push_annot(env,e->annot);});
# 1382
{void*_stmttmp1B=e->r;void*_tmp1E9=_stmttmp1B;struct Cyc_Absyn_Exp*_tmp1EA;struct Cyc_Absyn_Exp*_tmp1EB;switch(*((int*)_tmp1E9)){case 23U: _LL1: _tmp1EB=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp1E9)->f1;_LL2: {struct Cyc_Absyn_Exp*e1=_tmp1EB;
_tmp1EA=e1;goto _LL4;}case 20U: _LL3: _tmp1EA=((struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_tmp1E9)->f1;_LL4: {struct Cyc_Absyn_Exp*e1=_tmp1EA;
# 1385
({Cyc_IOEffect_check_access(env->fenv,e1->loc,0,e1->topt);});
goto _LL0;}default: _LL5: _LL6:
 goto _LL0;}_LL0:;}
# 1389
return 1;}
# 1392
static void Cyc_IOEffect_throws_exp_pop(struct Cyc_IOEffect_Env*env,struct Cyc_Absyn_Exp*e){
# 1394
({Cyc_IOEffect_pop_annot(env);});
# 1401
({void(*_tmp1ED)(struct Cyc_IOEffect_Env*env,struct Cyc_Absyn_Exp*e,void*annot)=Cyc_IOEffect_add_check_exception;struct Cyc_IOEffect_Env*_tmp1EE=env;struct Cyc_Absyn_Exp*_tmp1EF=e;void*_tmp1F0=({Cyc_IOEffect_peek_annot(env);});_tmp1ED(_tmp1EE,_tmp1EF,_tmp1F0);});}
# 1405
static struct Cyc_Absyn_Decl*Cyc_IOEffect_stmt2decl(struct Cyc_Absyn_Stmt*s){
# 1407
void*_stmttmp1C=s->r;void*_tmp1F2=_stmttmp1C;struct Cyc_Absyn_Decl*_tmp1F3;if(((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp1F2)->tag == 12U){_LL1: _tmp1F3=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp1F2)->f1;_LL2: {struct Cyc_Absyn_Decl*d=_tmp1F3;
return d;}}else{_LL3: _LL4:
({({int(*_tmp6BC)(struct _fat_ptr msg)=({int(*_tmp1F4)(struct _fat_ptr msg)=(int(*)(struct _fat_ptr msg))Cyc_IOEffect_env_err;_tmp1F4;});_tmp6BC(({const char*_tmp1F5="stmt2decl";_tag_fat(_tmp1F5,sizeof(char),10U);}));});});}_LL0:;}
# 1405
static void Cyc_IOEffect_check_pat_access(struct Cyc_IOEffect_Fenv*fe,struct Cyc_Absyn_Pat*pat,struct Cyc_List_List*f,unsigned loc);
# 1415
static int Cyc_IOEffect_throws_stmt_push(struct Cyc_IOEffect_Env*env,struct Cyc_Absyn_Stmt*s){
# 1420
({Cyc_IOEffect_register_stmt_succ(env,s);});
# 1422
({Cyc_IOEffect_push_annot(env,s->annot);});{
struct Cyc_IOEffect_Fenv*fenv=env->fenv;
# 1425
struct Cyc_IOEffect_Node*n=({Cyc_IOEffect_get_stmt(fenv,s);});
# 1431
{void*_stmttmp1D=s->r;void*_tmp1F7=_stmttmp1D;void*_tmp1FA;struct Cyc_List_List*_tmp1F9;struct Cyc_Absyn_Stmt*_tmp1F8;struct Cyc_Absyn_Stmt*_tmp1FC;struct _fat_ptr*_tmp1FB;struct Cyc_Absyn_Exp*_tmp1FE;struct Cyc_Absyn_Tvar*_tmp1FD;struct Cyc_Absyn_Pat*_tmp1FF;struct Cyc_Absyn_Fndecl*_tmp200;switch(*((int*)_tmp1F7)){case 12U: switch(*((int*)((struct Cyc_Absyn_Decl*)((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp1F7)->f1)->r)){case 1U: _LL1: _tmp200=((struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*)(((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp1F7)->f1)->r)->f1;_LL2: {struct Cyc_Absyn_Fndecl*fd=_tmp200;
# 1441
({Cyc_IOEffect_analyze_fd(env->tables,fd,s->loc);});
return 0;}case 2U: _LL7: _tmp1FF=((struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct*)(((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp1F7)->f1)->r)->f1;_LL8: {struct Cyc_Absyn_Pat*pat=_tmp1FF;
# 1475 "ioeffect.cyc"
({Cyc_IOEffect_check_pat_access(env->fenv,pat,0,pat->loc);});
goto _LL0;}case 4U: _LL9: _tmp1FD=((struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*)(((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp1F7)->f1)->r)->f1;_tmp1FE=((struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*)(((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp1F7)->f1)->r)->f3;_LLA: {struct Cyc_Absyn_Tvar*tv=_tmp1FD;struct Cyc_Absyn_Exp*eo=_tmp1FE;
# 1479
if(({Cyc_Absyn_is_xrgn_tvar(tv);}))
({void(*_tmp206)(struct Cyc_IOEffect_Env*env,struct Cyc_Absyn_Decl*s)=Cyc_IOEffect_push_enclosing_xrgn;struct Cyc_IOEffect_Env*_tmp207=env;struct Cyc_Absyn_Decl*_tmp208=({Cyc_IOEffect_stmt2decl(s);});_tmp206(_tmp207,_tmp208);});
# 1479
goto _LL0;}default: goto _LLB;}case 13U: _LL3: _tmp1FB=((struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct*)_tmp1F7)->f1;_tmp1FC=((struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct*)_tmp1F7)->f2;_LL4: {struct _fat_ptr*v=_tmp1FB;struct Cyc_Absyn_Stmt*s1=_tmp1FC;
# 1446 "ioeffect.cyc"
({Cyc_IOEffect_push_label(env->fenv,v);});
({Cyc_IOEffect_add_enclosed_label(env->fenv,v,s);});
goto _LL0;}case 15U: _LL5: _tmp1F8=((struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct*)_tmp1F7)->f1;_tmp1F9=((struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct*)_tmp1F7)->f2;_tmp1FA=(void*)((struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct*)_tmp1F7)->f3;_LL6: {struct Cyc_Absyn_Stmt*s1=_tmp1F8;struct Cyc_List_List*scs=_tmp1F9;void*dtp0=_tmp1FA;
# 1451
void*dtp=({({void*_tmp6BD=dtp0;Cyc_IOEffect_safe_cast(_tmp6BD,({const char*_tmp205="throws_stmt_push";_tag_fat(_tmp205,sizeof(char),17U);}));});});
# 1454
({Cyc_IOEffect_push_catch_block(env,s1,dtp);});
# 1456
for(0;scs != 0;scs=scs->tl){
struct Cyc_Absyn_Switch_clause*sc=(struct Cyc_Absyn_Switch_clause*)scs->hd;
struct Cyc_Absyn_Stmt*body=sc->body;
struct Cyc_Absyn_Pat*pat=sc->pattern;
({Cyc_IOEffect_check_pat_access(env->fenv,pat,0,pat->loc);});
# 1462
if(({int(*_tmp201)(struct _fat_ptr s1,struct _fat_ptr s2)=Cyc_strcmp;struct _fat_ptr _tmp202=(struct _fat_ptr)({Cyc_Absynpp_pat2string(pat);});struct _fat_ptr _tmp203=({const char*_tmp204="exn";_tag_fat(_tmp204,sizeof(char),4U);});_tmp201(_tmp202,_tmp203);})== 0)continue;}
# 1472 "ioeffect.cyc"
goto _LL0;}default: _LLB: _LLC:
# 1482
 goto _LL0;}_LL0:;}
# 1484
return 1;}}
# 1493
static void Cyc_IOEffect_throws_stmt_pop(struct Cyc_IOEffect_Env*env,struct Cyc_Absyn_Stmt*s){
# 1496
({Cyc_IOEffect_pop_annot(env);});
# 1498
{void*_stmttmp1E=s->r;void*_tmp20A=_stmttmp1E;struct Cyc_Absyn_Exp*_tmp20C;struct Cyc_Absyn_Tvar*_tmp20B;struct Cyc_Absyn_Stmt*_tmp20E;struct _fat_ptr*_tmp20D;switch(*((int*)_tmp20A)){case 13U: _LL1: _tmp20D=((struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct*)_tmp20A)->f1;_tmp20E=((struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct*)_tmp20A)->f2;_LL2: {struct _fat_ptr*v=_tmp20D;struct Cyc_Absyn_Stmt*s1=_tmp20E;
# 1501
({Cyc_IOEffect_pop_label(env->fenv);});
goto _LL0;}case 12U: if(((struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*)((struct Cyc_Absyn_Decl*)((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp20A)->f1)->r)->tag == 4U){_LL3: _tmp20B=((struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*)(((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp20A)->f1)->r)->f1;_tmp20C=((struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*)(((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp20A)->f1)->r)->f3;if(
# 1520 "ioeffect.cyc"
_tmp20C != 0 &&(int)_tmp20C->loc < 0){_LL4: {struct Cyc_Absyn_Tvar*tv=_tmp20B;struct Cyc_Absyn_Exp*eo=_tmp20C;
# 1522
({Cyc_IOEffect_pop_enclosing_xrgn(env);});goto _LL0;}}else{goto _LL5;}}else{goto _LL5;}default: _LL5: _LL6:
 goto _LL0;}_LL0:;}
# 1525
({Cyc_IOEffect_cond_pop_catch_block(env,s);});}
# 1493 "ioeffect.cyc"
static int Cyc_IOEffect_region_accessible(void*r,struct Cyc_List_List*e);
# 1534 "ioeffect.cyc"
static int Cyc_IOEffect_region_live(void*r,struct Cyc_List_List*e);struct _tuple25{void*f1;void*f2;void*f3;struct Cyc_List_List*f4;struct Cyc_List_List*f5;struct Cyc_List_List*f6;struct Cyc_List_List*f7;int f8;struct Cyc_List_List*f9;};
# 1538
static struct _tuple25 Cyc_IOEffect_extract_fun_type_info(void*ftyp){
void*_tmp210=ftyp;void*_tmp212;void*_tmp211;struct Cyc_List_List*_tmp219;int _tmp218;struct Cyc_List_List*_tmp217;struct Cyc_List_List*_tmp216;struct Cyc_List_List*_tmp215;void*_tmp214;struct Cyc_List_List*_tmp213;switch(*((int*)_tmp210)){case 5U: _LL1: _tmp213=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp210)->f1).tvars;_tmp214=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp210)->f1).effect;_tmp215=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp210)->f1).ieffect;_tmp216=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp210)->f1).oeffect;_tmp217=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp210)->f1).throws;_tmp218=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp210)->f1).reentrant;_tmp219=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp210)->f1).attributes;_LL2: {struct Cyc_List_List*tvs=_tmp213;void*e1=_tmp214;struct Cyc_List_List*ieff=_tmp215;struct Cyc_List_List*oeff=_tmp216;struct Cyc_List_List*t=_tmp217;int r=_tmp218;struct Cyc_List_List*a=_tmp219;
# 1544
return({struct _tuple25 _tmp596;_tmp596.f1=ftyp,_tmp596.f2=Cyc_Absyn_void_type,_tmp596.f3=(void*)_check_null(e1),_tmp596.f4=ieff,_tmp596.f5=oeff,_tmp596.f6=tvs,_tmp596.f7=t,_tmp596.f8=r,_tmp596.f9=a;_tmp596;});}case 3U: _LL3: _tmp211=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp210)->f1).elt_type;_tmp212=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp210)->f1).ptr_atts).rgn;_LL4: {void*t=_tmp211;void*rt=_tmp212;
# 1547
ftyp=({Cyc_Tcutil_compress(t);});
{void*_tmp21A=ftyp;struct Cyc_List_List*_tmp221;int _tmp220;struct Cyc_List_List*_tmp21F;struct Cyc_List_List*_tmp21E;struct Cyc_List_List*_tmp21D;void*_tmp21C;struct Cyc_List_List*_tmp21B;if(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp21A)->tag == 5U){_LL8: _tmp21B=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp21A)->f1).tvars;_tmp21C=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp21A)->f1).effect;_tmp21D=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp21A)->f1).ieffect;_tmp21E=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp21A)->f1).oeffect;_tmp21F=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp21A)->f1).throws;_tmp220=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp21A)->f1).reentrant;_tmp221=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp21A)->f1).attributes;_LL9: {struct Cyc_List_List*tvs=_tmp21B;void*e1=_tmp21C;struct Cyc_List_List*ieff=_tmp21D;struct Cyc_List_List*oeff=_tmp21E;struct Cyc_List_List*t=_tmp21F;int r=_tmp220;struct Cyc_List_List*a=_tmp221;
# 1552
return({struct _tuple25 _tmp597;_tmp597.f1=ftyp,_tmp597.f2=rt,_tmp597.f3=(void*)_check_null(e1),_tmp597.f4=ieff,_tmp597.f5=oeff,_tmp597.f6=tvs,_tmp597.f7=t,_tmp597.f8=r,_tmp597.f9=a;_tmp597;});}}else{_LLA: _LLB:
({({int(*_tmp6BE)(struct _fat_ptr msg)=({int(*_tmp222)(struct _fat_ptr msg)=(int(*)(struct _fat_ptr msg))Cyc_IOEffect_env_err;_tmp222;});_tmp6BE(({const char*_tmp223="extract_fun_type_info";_tag_fat(_tmp223,sizeof(char),22U);}));});});}_LL7:;}
# 1555
goto _LL0;}default: _LL5: _LL6:
({({int(*_tmp6BF)(struct _fat_ptr msg)=({int(*_tmp224)(struct _fat_ptr msg)=(int(*)(struct _fat_ptr msg))Cyc_IOEffect_env_err;_tmp224;});_tmp6BF(({const char*_tmp225="extract_fun_type_info";_tag_fat(_tmp225,sizeof(char),22U);}));});});}_LL0:;}
# 1560
static int Cyc_IOEffect_reentrant_type_visitor(struct _fat_ptr*bad,void*t){
# 1562
void*_tmp227=t;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp227)->tag == 0U)switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp227)->f1)){case 9U: _LL1: _LL2:
# 1570
 goto _LL4;case 10U: _LL3: _LL4:
 return 0;case 6U: _LL5: _LL6:
 goto _LL8;case 8U: _LL7: _LL8:
 goto _LLA;case 7U: _LL9: _LLA:
# 1576
({struct _fat_ptr _tmp6C2=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp22A=({struct Cyc_String_pa_PrintArg_struct _tmp598;_tmp598.tag=0U,({struct _fat_ptr _tmp6C0=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t);}));_tmp598.f1=_tmp6C0;});_tmp598;});void*_tmp228[1U];_tmp228[0]=& _tmp22A;({struct _fat_ptr _tmp6C1=({const char*_tmp229="%s";_tag_fat(_tmp229,sizeof(char),3U);});Cyc_aprintf(_tmp6C1,_tag_fat(_tmp228,sizeof(void*),1U));});});*bad=_tmp6C2;});
return 0;default: goto _LLB;}else{_LLB: _LLC:
 return 1;}_LL0:;}
# 1582
static void Cyc_IOEffect___check_access(struct Cyc_IOEffect_Fenv*fenv,unsigned loc,struct Cyc_List_List*f,void*t,int just_live){
# 1586
if(f == & Cyc_IOEffect_empty_effect)f=0;if(t == 0)
({void*_tmp22C=0U;({int(*_tmp6C4)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp22E)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp22E;});struct _fat_ptr _tmp6C3=({const char*_tmp22D="IOEffect::__check_access";_tag_fat(_tmp22D,sizeof(char),25U);});_tmp6C4(_tmp6C3,_tag_fat(_tmp22C,sizeof(void*),0U));});});{
# 1586
int(*fun)(void*r,struct Cyc_List_List*e)=
# 1588
just_live?Cyc_IOEffect_region_live: Cyc_IOEffect_region_accessible;
struct _fat_ptr errmsg=(struct _fat_ptr)(just_live?({const char*_tmp245="dead";_tag_fat(_tmp245,sizeof(char),5U);}):({const char*_tmp246="inaccessible";_tag_fat(_tmp246,sizeof(char),13U);}));
int should_ignore_x=({Cyc_IOEffect_should_ignore_xrgn(fenv);});
# 1593
t=({Cyc_Tcutil_compress(t);});
if(should_ignore_x){
struct _tuple25 _stmttmp1F=({Cyc_IOEffect_extract_fun_type_info((void*)_check_null((fenv->fd)->cached_type));});int _tmp22F;_LL1: _tmp22F=_stmttmp1F.f8;_LL2: {int reentrant0=_tmp22F;
# 1597
int reentrant=({Cyc_Absyn_is_reentrant(reentrant0);});
if(reentrant){
struct _fat_ptr bad=({const char*_tmp235="";_tag_fat(_tmp235,sizeof(char),1U);});
void*ftyp=(fenv->fd)->cached_type;
({({void(*_tmp6C5)(int(*f)(struct _fat_ptr*,void*),struct _fat_ptr*env,void*t)=({void(*_tmp230)(int(*f)(struct _fat_ptr*,void*),struct _fat_ptr*env,void*t)=(void(*)(int(*f)(struct _fat_ptr*,void*),struct _fat_ptr*env,void*t))Cyc_Absyn_visit_type;_tmp230;});_tmp6C5(Cyc_IOEffect_reentrant_type_visitor,& bad,(void*)_check_null(ftyp));});});
if(({({struct _fat_ptr _tmp6C6=(struct _fat_ptr)bad;Cyc_strcmp(_tmp6C6,({const char*_tmp231="";_tag_fat(_tmp231,sizeof(char),1U);}));});})!= 0)
({struct Cyc_String_pa_PrintArg_struct _tmp234=({struct Cyc_String_pa_PrintArg_struct _tmp599;_tmp599.tag=0U,_tmp599.f1=(struct _fat_ptr)((struct _fat_ptr)bad);_tmp599;});void*_tmp232[1U];_tmp232[0]=& _tmp234;({unsigned _tmp6C8=loc;struct _fat_ptr _tmp6C7=({const char*_tmp233="Cannot use `H, `U,  `RC in @re_entrant functions. Offending type : %s";_tag_fat(_tmp233,sizeof(char),70U);});Cyc_Tcutil_terr(_tmp6C8,_tmp6C7,_tag_fat(_tmp232,sizeof(void*),1U));});});}
# 1598
return;}}{
# 1594
void*rgns=({Cyc_Tcutil_rgns_of(t);});
# 1610
void*_tmp236=rgns;struct Cyc_List_List*_tmp237;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp236)->tag == 0U){if(((struct Cyc_Absyn_JoinCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp236)->f1)->tag == 11U){_LL4: _tmp237=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp236)->f2;_LL5: {struct Cyc_List_List*iter=_tmp237;
# 1612
for(0;iter != 0;iter=iter->tl){
void*t1=(void*)iter->hd;
# 1616
if((!just_live && Cyc_Absyn_reentrant)&&({Cyc_IOEffect_is_heaprgn(t1);}))
({void*_tmp238=0U;({unsigned _tmp6CA=loc;struct _fat_ptr _tmp6C9=({const char*_tmp239="Cannot use pointers allocated at the heap (`H) in @re_entrant functions.";_tag_fat(_tmp239,sizeof(char),73U);});Cyc_Tcutil_terr(_tmp6CA,_tmp6C9,_tag_fat(_tmp238,sizeof(void*),0U));});});
# 1616
{void*_tmp23A=t1;void*_tmp23B;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp23A)->tag == 0U){if(((struct Cyc_Absyn_AccessCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp23A)->f1)->tag == 9U){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp23A)->f2 != 0){_LL9: _tmp23B=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp23A)->f2)->hd;_LLA: {void*r=_tmp23B;
# 1621
struct Cyc_Absyn_Kind*k=({Cyc_Tcutil_type_kind(r);});
int is_x=({Cyc_Tcutil_kind_eq(k,& Cyc_Tcutil_xrk);});
if(is_x){
if(!({fun(r,f);}))
({struct Cyc_String_pa_PrintArg_struct _tmp23E=({struct Cyc_String_pa_PrintArg_struct _tmp59C;_tmp59C.tag=0U,_tmp59C.f1=(struct _fat_ptr)((struct _fat_ptr)errmsg);_tmp59C;});struct Cyc_String_pa_PrintArg_struct _tmp23F=({struct Cyc_String_pa_PrintArg_struct _tmp59B;_tmp59B.tag=0U,({
# 1627
struct _fat_ptr _tmp6CB=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(r);}));_tmp59B.f1=_tmp6CB;});_tmp59B;});struct Cyc_String_pa_PrintArg_struct _tmp240=({struct Cyc_String_pa_PrintArg_struct _tmp59A;_tmp59A.tag=0U,({
struct _fat_ptr _tmp6CC=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_effect2string(f);}));_tmp59A.f1=_tmp6CC;});_tmp59A;});void*_tmp23C[3U];_tmp23C[0]=& _tmp23E,_tmp23C[1]=& _tmp23F,_tmp23C[2]=& _tmp240;({unsigned _tmp6CE=loc;struct _fat_ptr _tmp6CD=({const char*_tmp23D="Region %s is %s in effect %s";_tag_fat(_tmp23D,sizeof(char),29U);});Cyc_Tcutil_terr(_tmp6CE,_tmp6CD,_tag_fat(_tmp23C,sizeof(void*),3U));});});}
# 1623
goto _LL8;}}else{goto _LLB;}}else{goto _LLB;}}else{_LLB: _LLC:
# 1632
({({int(*_tmp6CF)(struct _fat_ptr msg)=({int(*_tmp241)(struct _fat_ptr msg)=(int(*)(struct _fat_ptr msg))Cyc_IOEffect_env_err;_tmp241;});_tmp6CF(({const char*_tmp242="check_access 1";_tag_fat(_tmp242,sizeof(char),15U);}));});});}_LL8:;}
# 1634
break;}
goto _LL3;}}else{goto _LL6;}}else{_LL6: _LL7:
({({int(*_tmp6D0)(struct _fat_ptr msg)=({int(*_tmp243)(struct _fat_ptr msg)=(int(*)(struct _fat_ptr msg))Cyc_IOEffect_env_err;_tmp243;});_tmp6D0(({const char*_tmp244="check_access 2";_tag_fat(_tmp244,sizeof(char),15U);}));});});}_LL3:;}}}
# 1657 "ioeffect.cyc"
static void Cyc_IOEffect_check_access(struct Cyc_IOEffect_Fenv*fe,unsigned loc,struct Cyc_List_List*f,void*t){
# 1660
({Cyc_IOEffect___check_access(fe,loc,f,t,0);});}
# 1663
static void Cyc_IOEffect_check_live(struct Cyc_IOEffect_Fenv*fe,unsigned loc,struct Cyc_List_List*f,void*t){
# 1666
({Cyc_IOEffect___check_access(fe,loc,f,t,1);});}struct _tuple26{struct Cyc_List_List*f1;struct Cyc_Absyn_Pat*f2;};
# 1669
static void Cyc_IOEffect_check_pat_access(struct Cyc_IOEffect_Fenv*fe,struct Cyc_Absyn_Pat*pat,struct Cyc_List_List*f,unsigned loc){
void*_stmttmp20=pat->r;void*_tmp24A=_stmttmp20;struct Cyc_List_List*_tmp24D;struct Cyc_Absyn_Datatypefield*_tmp24C;struct Cyc_Absyn_Datatypedecl*_tmp24B;struct Cyc_List_List*_tmp250;struct Cyc_List_List*_tmp24F;union Cyc_Absyn_AggrInfo*_tmp24E;struct Cyc_Absyn_Pat*_tmp251;struct Cyc_List_List*_tmp252;struct Cyc_Absyn_Vardecl*_tmp254;struct Cyc_Absyn_Tvar*_tmp253;struct Cyc_Absyn_Vardecl*_tmp256;struct Cyc_Absyn_Tvar*_tmp255;struct Cyc_Absyn_Pat*_tmp258;struct Cyc_Absyn_Vardecl*_tmp257;struct Cyc_Absyn_Pat*_tmp25A;struct Cyc_Absyn_Vardecl*_tmp259;switch(*((int*)_tmp24A)){case 0U: _LL1: _LL2:
 goto _LL0;case 1U: _LL3: _tmp259=((struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct*)_tmp24A)->f1;_tmp25A=((struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct*)_tmp24A)->f2;_LL4: {struct Cyc_Absyn_Vardecl*vd=_tmp259;struct Cyc_Absyn_Pat*p=_tmp25A;
# 1673
({Cyc_IOEffect_check_pat_access(fe,p,f,loc);});goto _LL0;}case 3U: _LL5: _tmp257=((struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct*)_tmp24A)->f1;_tmp258=((struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct*)_tmp24A)->f2;_LL6: {struct Cyc_Absyn_Vardecl*vd=_tmp257;struct Cyc_Absyn_Pat*p=_tmp258;
# 1676
({Cyc_IOEffect_check_pat_access(fe,p,f,loc);});goto _LL0;}case 2U: _LL7: _tmp255=((struct Cyc_Absyn_AliasVar_p_Absyn_Raw_pat_struct*)_tmp24A)->f1;_tmp256=((struct Cyc_Absyn_AliasVar_p_Absyn_Raw_pat_struct*)_tmp24A)->f2;_LL8: {struct Cyc_Absyn_Tvar*tv=_tmp255;struct Cyc_Absyn_Vardecl*vd=_tmp256;
goto _LL0;}case 4U: _LL9: _tmp253=((struct Cyc_Absyn_TagInt_p_Absyn_Raw_pat_struct*)_tmp24A)->f1;_tmp254=((struct Cyc_Absyn_TagInt_p_Absyn_Raw_pat_struct*)_tmp24A)->f2;_LLA: {struct Cyc_Absyn_Tvar*tv=_tmp253;struct Cyc_Absyn_Vardecl*vd=_tmp254;
goto _LL0;}case 5U: _LLB: _tmp252=((struct Cyc_Absyn_Tuple_p_Absyn_Raw_pat_struct*)_tmp24A)->f1;_LLC: {struct Cyc_List_List*lpat=_tmp252;
# 1680
for(0;lpat != 0;lpat=lpat->tl){
({Cyc_IOEffect_check_pat_access(fe,(struct Cyc_Absyn_Pat*)lpat->hd,f,loc);});}
# 1683
goto _LL0;}case 6U: _LLD: _tmp251=((struct Cyc_Absyn_Pointer_p_Absyn_Raw_pat_struct*)_tmp24A)->f1;_LLE: {struct Cyc_Absyn_Pat*p=_tmp251;
# 1685
({void(*_tmp25B)(struct Cyc_IOEffect_Fenv*fe,unsigned loc,struct Cyc_List_List*f,void*t)=Cyc_IOEffect_check_access;struct Cyc_IOEffect_Fenv*_tmp25C=fe;unsigned _tmp25D=loc;struct Cyc_List_List*_tmp25E=f;void*_tmp25F=({({void*_tmp6D1=p->topt;Cyc_IOEffect_safe_cast(_tmp6D1,({const char*_tmp260="check_pat_access";_tag_fat(_tmp260,sizeof(char),17U);}));});});_tmp25B(_tmp25C,_tmp25D,_tmp25E,_tmp25F);});
({Cyc_IOEffect_check_pat_access(fe,p,f,loc);});goto _LL0;}case 7U: _LLF: _tmp24E=((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp24A)->f1;_tmp24F=((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp24A)->f2;_tmp250=((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp24A)->f3;_LL10: {union Cyc_Absyn_AggrInfo*info=_tmp24E;struct Cyc_List_List*tvlist=_tmp24F;struct Cyc_List_List*dplist=_tmp250;
# 1688
for(0;dplist != 0;dplist=dplist->tl){
struct _tuple26*_stmttmp21=(struct _tuple26*)dplist->hd;struct Cyc_Absyn_Pat*_tmp262;struct Cyc_List_List*_tmp261;_LL22: _tmp261=_stmttmp21->f1;_tmp262=_stmttmp21->f2;_LL23: {struct Cyc_List_List*dlist=_tmp261;struct Cyc_Absyn_Pat*p=_tmp262;
({Cyc_IOEffect_check_pat_access(fe,p,f,loc);});}}
# 1692
goto _LL0;}case 8U: _LL11: _tmp24B=((struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct*)_tmp24A)->f1;_tmp24C=((struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct*)_tmp24A)->f2;_tmp24D=((struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct*)_tmp24A)->f3;_LL12: {struct Cyc_Absyn_Datatypedecl*dcl=_tmp24B;struct Cyc_Absyn_Datatypefield*fld=_tmp24C;struct Cyc_List_List*patlist=_tmp24D;
# 1694
for(0;patlist != 0;patlist=patlist->tl){
({Cyc_IOEffect_check_pat_access(fe,(struct Cyc_Absyn_Pat*)patlist->hd,f,loc);});}
# 1697
goto _LL0;}case 9U: _LL13: _LL14:
 goto _LL0;case 10U: _LL15: _LL16:
 goto _LL0;case 11U: _LL17: _LL18:
 goto _LL0;case 12U: _LL19: _LL1A:
 goto _LL0;case 13U: _LL1B: _LL1C:
 goto _LL0;case 14U: _LL1D: _LL1E:
 goto _LL0;default: _LL1F: _LL20:
({({int(*_tmp6D2)(struct _fat_ptr msg)=({int(*_tmp263)(struct _fat_ptr msg)=(int(*)(struct _fat_ptr msg))Cyc_IOEffect_env_err;_tmp263;});_tmp6D2(({const char*_tmp264="check_pat_access";_tag_fat(_tmp264,sizeof(char),17U);}));});});}_LL0:;}
# 1715
static struct _tuple25 Cyc_IOEffect_extract_fun_ptr_type_info(struct Cyc_Absyn_Exp*e){
# 1717
void*ftyp=({void*(*_tmp266)(void*)=Cyc_Tcutil_compress;void*_tmp267=({({void*_tmp6D3=e->topt;Cyc_IOEffect_safe_cast(_tmp6D3,({const char*_tmp268="extract_fun_ptr_type_info_e";_tag_fat(_tmp268,sizeof(char),28U);}));});});_tmp266(_tmp267);});
return({Cyc_IOEffect_extract_fun_type_info(ftyp);});}
# 1721
static int Cyc_IOEffect_check_effect(void*e1){
# 1723
void*comp=({Cyc_Tcutil_compress(e1);});
void*_tmp26A=comp;struct Cyc_List_List*_tmp26B;struct Cyc_List_List*_tmp26C;switch(*((int*)_tmp26A)){case 0U: switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp26A)->f1)){case 6U: _LL1: _LL2:
# 1726
 goto _LL4;case 8U: _LL3: _LL4:
 goto _LL6;case 7U: _LL5: _LL6:
 goto _LL8;case 9U: _LL7: _LL8:
 goto _LLA;case 12U: _LLD: _LLE:
# 1732
 return 0;case 11U: _LLF: _tmp26C=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp26A)->f2;_LL10: {struct Cyc_List_List*es=_tmp26C;
# 1734
for(0;es != 0;es=es->tl){
if(!({Cyc_IOEffect_check_effect((void*)es->hd);}))return 0;}
# 1734
return 1;}case 10U: _LL11: _tmp26B=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp26A)->f2;_LL12: {struct Cyc_List_List*r2=_tmp26B;
# 1737
return 1;}default: goto _LL13;}case 2U: _LL9: _LLA:
# 1730
 goto _LLC;case 1U: _LLB: _LLC:
 goto _LLE;default: _LL13: _LL14:
# 1738
({struct Cyc_String_pa_PrintArg_struct _tmp270=({struct Cyc_String_pa_PrintArg_struct _tmp59D;_tmp59D.tag=0U,({
struct _fat_ptr _tmp6D4=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(e1);}));_tmp59D.f1=_tmp6D4;});_tmp59D;});void*_tmp26D[1U];_tmp26D[0]=& _tmp270;({int(*_tmp6D6)(struct _fat_ptr fmt,struct _fat_ptr ap)=({
# 1738
int(*_tmp26F)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp26F;});struct _fat_ptr _tmp6D5=({const char*_tmp26E="check_effect: bad effect: e1=%s";_tag_fat(_tmp26E,sizeof(char),32U);});_tmp6D6(_tmp6D5,_tag_fat(_tmp26D,sizeof(void*),1U));});});}_LL0:;}
# 1744
static int Cyc_IOEffect_thread_type_visitor(struct _fat_ptr*bad,void*t){
# 1747
void*_tmp272=t;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp272)->tag == 0U)switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp272)->f1)){case 10U: _LL1: _LL2:
# 1755
 return 0;case 6U: _LL3: _LL4:
 goto _LL6;case 8U: _LL5: _LL6:
 goto _LL8;case 7U: _LL7: _LL8:
 goto _LLA;case 9U: _LL9: _LLA:
# 1761
({struct _fat_ptr _tmp6D9=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp275=({struct Cyc_String_pa_PrintArg_struct _tmp59E;_tmp59E.tag=0U,({struct _fat_ptr _tmp6D7=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t);}));_tmp59E.f1=_tmp6D7;});_tmp59E;});void*_tmp273[1U];_tmp273[0]=& _tmp275;({struct _fat_ptr _tmp6D8=({const char*_tmp274="%s";_tag_fat(_tmp274,sizeof(char),3U);});Cyc_aprintf(_tmp6D8,_tag_fat(_tmp273,sizeof(void*),1U));});});*bad=_tmp6D9;});
return 0;default: goto _LLB;}else{_LLB: _LLC:
 return 1;}_LL0:;}
# 1768
static void Cyc_IOEffect_check_no_normal_regions_in_thread(unsigned loc,struct Cyc_Absyn_FnInfo*info,void*ftyp){
# 1771
if(info->c_varargs)
# 1773
({struct Cyc_String_pa_PrintArg_struct _tmp279=({struct Cyc_String_pa_PrintArg_struct _tmp59F;_tmp59F.tag=0U,({struct _fat_ptr _tmp6DA=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(ftyp);}));_tmp59F.f1=_tmp6DA;});_tmp59F;});void*_tmp277[1U];_tmp277[0]=& _tmp279;({unsigned _tmp6DC=loc;struct _fat_ptr _tmp6DB=({const char*_tmp278="Cannot spawn a thread that accepts variable arguments : %s";_tag_fat(_tmp278,sizeof(char),59U);});Cyc_Tcutil_terr(_tmp6DC,_tmp6DB,_tag_fat(_tmp277,sizeof(void*),1U));});});{
# 1771
void*eff=info->effect;
# 1784 "ioeffect.cyc"
if(eff == 0)return;eff=({Cyc_Tcutil_compress(eff);});
# 1786
if(({Cyc_IOEffect_check_effect(eff);})== 0){
# 1788
struct _fat_ptr str=({Cyc_Absynpp_typ2string(eff);});
({struct Cyc_String_pa_PrintArg_struct _tmp27C=({struct Cyc_String_pa_PrintArg_struct _tmp5A0;_tmp5A0.tag=0U,_tmp5A0.f1=(struct _fat_ptr)((struct _fat_ptr)str);_tmp5A0;});void*_tmp27A[1U];_tmp27A[0]=& _tmp27C;({unsigned _tmp6DE=loc;struct _fat_ptr _tmp6DD=({const char*_tmp27B="Cannot spawn a thread that has a non-empty effect of \"ordinary\" regions: %s";_tag_fat(_tmp27B,sizeof(char),76U);});Cyc_Tcutil_terr(_tmp6DE,_tmp6DD,_tag_fat(_tmp27A,sizeof(void*),1U));});});}{
# 1786
struct _fat_ptr bad=({const char*_tmp288="";_tag_fat(_tmp288,sizeof(char),1U);});
# 1792
({({void(*_tmp6DF)(int(*f)(struct _fat_ptr*,void*),struct _fat_ptr*env,void*t)=({void(*_tmp27D)(int(*f)(struct _fat_ptr*,void*),struct _fat_ptr*env,void*t)=(void(*)(int(*f)(struct _fat_ptr*,void*),struct _fat_ptr*env,void*t))Cyc_Absyn_visit_type;_tmp27D;});_tmp6DF(Cyc_IOEffect_thread_type_visitor,& bad,ftyp);});});
if(({({struct _fat_ptr _tmp6E0=(struct _fat_ptr)bad;Cyc_strcmp(_tmp6E0,({const char*_tmp27E="";_tag_fat(_tmp27E,sizeof(char),1U);}));});})!= 0){
# 1795
void*rgs=({Cyc_Tcutil_rgns_of(ftyp);});
struct _fat_ptr str=({Cyc_Absynpp_typ2string(rgs);});
({struct Cyc_String_pa_PrintArg_struct _tmp281=({struct Cyc_String_pa_PrintArg_struct _tmp5A2;_tmp5A2.tag=0U,({struct _fat_ptr _tmp6E3=(struct _fat_ptr)(({({struct _fat_ptr _tmp6E2=(struct _fat_ptr)str;Cyc_strcmp(_tmp6E2,({const char*_tmp283="{}";_tag_fat(_tmp283,sizeof(char),3U);}));});})== 0?({const char*_tmp284="";_tag_fat(_tmp284,sizeof(char),1U);}):(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp287=({struct Cyc_String_pa_PrintArg_struct _tmp5A3;_tmp5A3.tag=0U,_tmp5A3.f1=(struct _fat_ptr)((struct _fat_ptr)str);_tmp5A3;});void*_tmp285[1U];_tmp285[0]=& _tmp287;({struct _fat_ptr _tmp6E1=({const char*_tmp286=" (function effect: %s)";_tag_fat(_tmp286,sizeof(char),23U);});Cyc_aprintf(_tmp6E1,_tag_fat(_tmp285,sizeof(void*),1U));});}));_tmp5A2.f1=_tmp6E3;});_tmp5A2;});struct Cyc_String_pa_PrintArg_struct _tmp282=({struct Cyc_String_pa_PrintArg_struct _tmp5A1;_tmp5A1.tag=0U,_tmp5A1.f1=(struct _fat_ptr)((struct _fat_ptr)bad);_tmp5A1;});void*_tmp27F[2U];_tmp27F[0]=& _tmp281,_tmp27F[1]=& _tmp282;({unsigned _tmp6E5=loc;struct _fat_ptr _tmp6E4=({const char*_tmp280="Cannot spawn a thread that uses `H, `U,  `RC or traditional regions%s. Offending type : %s";_tag_fat(_tmp280,sizeof(char),91U);});Cyc_Tcutil_terr(_tmp6E5,_tmp6E4,_tag_fat(_tmp27F,sizeof(void*),2U));});});}}}}
# 1803
static int Cyc_IOEffect_is_region_kind(struct Cyc_Absyn_Tvar*tv){
# 1805
void*_stmttmp22=tv->kind;void*_tmp28A=_stmttmp22;struct Cyc_Absyn_Kind*_tmp28B;if(((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp28A)->tag == 0U){_LL1: _tmp28B=((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp28A)->f1;_LL2: {struct Cyc_Absyn_Kind*k=_tmp28B;
# 1808
return({Cyc_Tcutil_kind_leq(k,& Cyc_Tcutil_trk);});}}else{_LL3: _LL4:
# 1811
 return 0;}_LL0:;}struct _tuple27{void*f1;int f2;};
# 1815
static struct _tuple27 Cyc_IOEffect_check_fun_call_ptr(struct Cyc_IOEffect_Env*env,struct Cyc_Absyn_Exp*e,int spwn,unsigned loc){
# 1818
struct _tuple25 _stmttmp23=({Cyc_IOEffect_extract_fun_ptr_type_info(e);});struct Cyc_List_List*_tmp295;int _tmp294;struct Cyc_List_List*_tmp293;struct Cyc_List_List*_tmp292;struct Cyc_List_List*_tmp291;struct Cyc_List_List*_tmp290;void*_tmp28F;void*_tmp28E;void*_tmp28D;_LL1: _tmp28D=_stmttmp23.f1;_tmp28E=_stmttmp23.f2;_tmp28F=_stmttmp23.f3;_tmp290=_stmttmp23.f4;_tmp291=_stmttmp23.f5;_tmp292=_stmttmp23.f6;_tmp293=_stmttmp23.f7;_tmp294=_stmttmp23.f8;_tmp295=_stmttmp23.f9;_LL2: {void*ftyp=_tmp28D;void*rt=_tmp28E;void*eff=_tmp28F;struct Cyc_List_List*ieff=_tmp290;struct Cyc_List_List*oeff=_tmp291;struct Cyc_List_List*tvs=_tmp292;struct Cyc_List_List*throws=_tmp293;int reentrant=_tmp294;struct Cyc_List_List*att=_tmp295;
int noreturn=0;
# 1823
if(tvs != 0)
({void*_tmp296=0U;({unsigned _tmp6E7=loc;struct _fat_ptr _tmp6E6=({const char*_tmp297="Cannot call a partially instantiated function.";_tag_fat(_tmp297,sizeof(char),47U);});Cyc_Tcutil_terr(_tmp6E7,_tmp6E6,_tag_fat(_tmp296,sizeof(void*),0U));});});
# 1823
if(spwn){
# 1828
if(!({Cyc_Absyn_is_reentrant(reentrant);}))
({struct Cyc_String_pa_PrintArg_struct _tmp29A=({struct Cyc_String_pa_PrintArg_struct _tmp5A4;_tmp5A4.tag=0U,({
struct _fat_ptr _tmp6E8=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(ftyp);}));_tmp5A4.f1=_tmp6E8;});_tmp5A4;});void*_tmp298[1U];_tmp298[0]=& _tmp29A;({unsigned _tmp6EA=loc;struct _fat_ptr _tmp6E9=({const char*_tmp299="Thread function type is not @re_entrant: %s";_tag_fat(_tmp299,sizeof(char),44U);});Cyc_Tcutil_terr(_tmp6EA,_tmp6E9,_tag_fat(_tmp298,sizeof(void*),1U));});});
# 1828
if(!({Cyc_IOEffect_is_heaprgn(rt);}))
# 1832
({void*_tmp29B=0U;({unsigned _tmp6EC=loc;struct _fat_ptr _tmp6EB=({const char*_tmp29C="Cannot pass an non Heap function pointer to spawn.";_tag_fat(_tmp29C,sizeof(char),51U);});Cyc_Tcutil_terr(_tmp6EC,_tmp6EB,_tag_fat(_tmp29B,sizeof(void*),0U));});});
# 1828
if(oeff != 0)
# 1835
({struct Cyc_String_pa_PrintArg_struct _tmp29F=({struct Cyc_String_pa_PrintArg_struct _tmp5A5;_tmp5A5.tag=0U,({struct _fat_ptr _tmp6ED=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(ftyp);}));_tmp5A5.f1=_tmp6ED;});_tmp5A5;});void*_tmp29D[1U];_tmp29D[0]=& _tmp29F;({unsigned _tmp6EF=loc;struct _fat_ptr _tmp6EE=({const char*_tmp29E="Spawn operation: Function type %s must have an empty (or no)  @oeffect anotation";_tag_fat(_tmp29E,sizeof(char),81U);});Cyc_Tcutil_terr(_tmp6EF,_tmp6EE,_tag_fat(_tmp29D,sizeof(void*),1U));});});
# 1828
if(({Cyc_Absyn_is_nothrow(throws);})== 0)
# 1838
({struct Cyc_String_pa_PrintArg_struct _tmp2A2=({struct Cyc_String_pa_PrintArg_struct _tmp5A6;_tmp5A6.tag=0U,({struct _fat_ptr _tmp6F0=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(ftyp);}));_tmp5A6.f1=_tmp6F0;});_tmp5A6;});void*_tmp2A0[1U];_tmp2A0[0]=& _tmp2A2;({unsigned _tmp6F2=loc;struct _fat_ptr _tmp6F1=({const char*_tmp2A1="Spawn operation: Function type %s must be @nothrow";_tag_fat(_tmp2A1,sizeof(char),51U);});Cyc_Tcutil_terr(_tmp6F2,_tmp6F1,_tag_fat(_tmp2A0,sizeof(void*),1U));});});{
# 1828
void*_tmp2A3=ftyp;struct Cyc_Absyn_FnInfo _tmp2A4;if(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp2A3)->tag == 5U){_LL4: _tmp2A4=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp2A3)->f1;_LL5: {struct Cyc_Absyn_FnInfo info=_tmp2A4;
# 1842
({Cyc_IOEffect_check_no_normal_regions_in_thread(loc,& info,ftyp);});
goto _LL3;}}else{_LL6: _LL7:
# 1845
({({int(*_tmp6F3)(struct _fat_ptr msg)=({int(*_tmp2A5)(struct _fat_ptr msg)=(int(*)(struct _fat_ptr msg))Cyc_IOEffect_env_err;_tmp2A5;});_tmp6F3(({const char*_tmp2A6="check_fun_call_ptr";_tag_fat(_tmp2A6,sizeof(char),19U);}));});});}_LL3:;}}else{
# 1850
void*t=({Cyc_Tcutil_rgns_of(eff);});
{void*_tmp2A7=t;struct Cyc_List_List*_tmp2A8;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2A7)->tag == 0U){if(((struct Cyc_Absyn_JoinCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2A7)->f1)->tag == 11U){_LL9: _tmp2A8=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2A7)->f2;_LLA: {struct Cyc_List_List*iter=_tmp2A8;
# 1853
for(0;iter != 0;iter=iter->tl){
# 1856
void*_stmttmp24=(void*)iter->hd;void*_tmp2A9=_stmttmp24;void*_tmp2AA;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2A9)->tag == 0U){if(((struct Cyc_Absyn_AccessCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2A9)->f1)->tag == 9U){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2A9)->f2 != 0){_LLE: _tmp2AA=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2A9)->f2)->hd;_LLF: {void*r=_tmp2AA;
# 1858
struct Cyc_Absyn_Kind*k=({Cyc_Tcutil_type_kind(r);});
if(({Cyc_Tcutil_kind_eq(k,& Cyc_Tcutil_xrk);}))
({struct Cyc_String_pa_PrintArg_struct _tmp2AD=({struct Cyc_String_pa_PrintArg_struct _tmp5A7;_tmp5A7.tag=0U,({struct _fat_ptr _tmp6F4=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(r);}));_tmp5A7.f1=_tmp6F4;});_tmp5A7;});void*_tmp2AB[1U];_tmp2AB[0]=& _tmp2AD;({unsigned _tmp6F6=loc;struct _fat_ptr _tmp6F5=({const char*_tmp2AC="Passing an X-Region effect as a traditional Cyclone effect is disallowed. Offending region : %s.";_tag_fat(_tmp2AC,sizeof(char),97U);});Cyc_Tcutil_terr(_tmp6F6,_tmp6F5,_tag_fat(_tmp2AB,sizeof(void*),1U));});});
# 1859
goto _LLD;}}else{goto _LL10;}}else{goto _LL10;}}else{_LL10: _LL11:
# 1867
 goto _LLD;}_LLD:;}
# 1872
goto _LL8;}}else{goto _LLB;}}else{_LLB: _LLC:
({({int(*_tmp6F7)(struct _fat_ptr msg)=({int(*_tmp2AE)(struct _fat_ptr msg)=(int(*)(struct _fat_ptr msg))Cyc_IOEffect_env_err;_tmp2AE;});_tmp6F7(({const char*_tmp2AF="check_fun_call_ptr";_tag_fat(_tmp2AF,sizeof(char),19U);}));});});}_LL8:;}
# 1877
for(0;att != 0;att=att->tl){
# 1879
if(!({Cyc_Absyn_attribute_cmp((void*)att->hd,(void*)& Cyc_Absyn_Noreturn_att_val);})){
# 1881
noreturn=1;break;}}}
# 1910 "ioeffect.cyc"
return({struct _tuple27 _tmp5A8;_tmp5A8.f1=ftyp,_tmp5A8.f2=noreturn;_tmp5A8;});}}
# 1914
static struct Cyc_List_List*Cyc_IOEffect_check_parent_add_child(void*r,void*typ,struct Cyc_List_List*f,unsigned loc){
# 1917
if(f == & Cyc_IOEffect_empty_effect)f=0;{void*_tmp2B1=typ;struct Cyc_List_List*_tmp2B2;struct Cyc_List_List*_tmp2B3;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2B1)->tag == 0U){if(((struct Cyc_Absyn_RgnHandleCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2B1)->f1)->tag == 3U){_LL1: _tmp2B3=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2B1)->f2;if((void*)((struct Cyc_List_List*)_check_null(_tmp2B3))->hd == Cyc_Absyn_heap_rgn_type){_LL2: {struct Cyc_List_List*ts=_tmp2B3;
# 1921
typ=Cyc_Absyn_heap_rgn_type;
goto _LL0;}}else{_LL3: _tmp2B2=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2B1)->f2;_LL4: {struct Cyc_List_List*ts=_tmp2B2;
# 1924
typ=(void*)((struct Cyc_List_List*)_check_null(ts))->hd;
if(({struct Cyc_Absyn_RgnEffect*(*_tmp2B4)(struct Cyc_Absyn_Tvar*x,struct Cyc_List_List*l)=Cyc_Absyn_find_rgneffect;struct Cyc_Absyn_Tvar*_tmp2B5=({Cyc_Absyn_type2tvar((void*)ts->hd);});struct Cyc_List_List*_tmp2B6=f;_tmp2B4(_tmp2B5,_tmp2B6);})== 0)
# 1927
({struct Cyc_String_pa_PrintArg_struct _tmp2B9=({struct Cyc_String_pa_PrintArg_struct _tmp5A9;_tmp5A9.tag=0U,({
struct _fat_ptr _tmp6F8=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(typ);}));_tmp5A9.f1=_tmp6F8;});_tmp5A9;});void*_tmp2B7[1U];_tmp2B7[0]=& _tmp2B9;({unsigned _tmp6FA=loc;struct _fat_ptr _tmp6F9=({const char*_tmp2B8="Invalid parent region handle %s";_tag_fat(_tmp2B8,sizeof(char),32U);});Cyc_Tcutil_terr(_tmp6FA,_tmp6F9,_tag_fat(_tmp2B7,sizeof(void*),1U));});});
# 1925
goto _LL0;}}}else{goto _LL5;}}else{_LL5: _LL6:
# 1931
({({int(*_tmp6FB)(struct _fat_ptr msg)=({int(*_tmp2BA)(struct _fat_ptr msg)=(int(*)(struct _fat_ptr msg))Cyc_IOEffect_env_err;_tmp2BA;});_tmp6FB(({const char*_tmp2BB="check_parent_add_child";_tag_fat(_tmp2BB,sizeof(char),23U);}));});});}_LL0:;}
# 1935
if(({struct Cyc_Absyn_RgnEffect*(*_tmp2BC)(struct Cyc_Absyn_Tvar*x,struct Cyc_List_List*l)=Cyc_Absyn_find_rgneffect;struct Cyc_Absyn_Tvar*_tmp2BD=({Cyc_Absyn_type2tvar(r);});struct Cyc_List_List*_tmp2BE=f;_tmp2BC(_tmp2BD,_tmp2BE);})!= 0)
({({int(*_tmp6FC)(struct _fat_ptr msg)=({int(*_tmp2BF)(struct _fat_ptr msg)=(int(*)(struct _fat_ptr msg))Cyc_IOEffect_env_err;_tmp2BF;});_tmp6FC(({const char*_tmp2C0="check_parent_add_child";_tag_fat(_tmp2C0,sizeof(char),23U);}));});});
# 1935
return({Cyc_IOEffect_new_rgn_effect(r,typ,f);});}struct Cyc_IOEffect_Break_js_IOEffect_JmpSummary_struct{int tag;};struct Cyc_IOEffect_Continue_js_IOEffect_JmpSummary_struct{int tag;};struct Cyc_IOEffect_Fallthru_js_IOEffect_JmpSummary_struct{int tag;struct Cyc_Absyn_Stmt*f1;};struct Cyc_IOEffect_Goto_js_IOEffect_JmpSummary_struct{int tag;struct _fat_ptr*f1;};struct Cyc_IOEffect_Throw_js_IOEffect_JmpSummary_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_IOEffect_Return_js_IOEffect_JmpSummary_struct{int tag;};struct Cyc_IOEffect_NoJump_js_IOEffect_JmpSummary_struct{int tag;};struct Cyc_IOEffect_Or_js_IOEffect_JmpSummary_struct{int tag;void*f1;void*f2;};
# 1954
static struct Cyc_IOEffect_Break_js_IOEffect_JmpSummary_struct Cyc_IOEffect_break_js={0U};
static struct Cyc_IOEffect_Continue_js_IOEffect_JmpSummary_struct Cyc_IOEffect_continue_js={1U};
static struct Cyc_IOEffect_Return_js_IOEffect_JmpSummary_struct Cyc_IOEffect_return_js={5U};
static struct Cyc_IOEffect_NoJump_js_IOEffect_JmpSummary_struct Cyc_IOEffect_nojump_js={6U};
# 1967 "ioeffect.cyc"
void*Cyc_IOEffect_join_js(void*j1,void*j2){
# 1969
struct _tuple15 _stmttmp25=({struct _tuple15 _tmp5AA;_tmp5AA.f1=j1,_tmp5AA.f2=j2;_tmp5AA;});struct _tuple15 _tmp2C2=_stmttmp25;struct Cyc_Absyn_Exp*_tmp2C4;struct Cyc_Absyn_Exp*_tmp2C3;struct _fat_ptr*_tmp2C6;struct _fat_ptr*_tmp2C5;struct Cyc_Absyn_Stmt*_tmp2C8;struct Cyc_Absyn_Stmt*_tmp2C7;if(((struct Cyc_IOEffect_Return_js_IOEffect_JmpSummary_struct*)_tmp2C2.f1)->tag == 5U){_LL1: _LL2:
# 1971
 goto _LL4;}else{if(((struct Cyc_IOEffect_Return_js_IOEffect_JmpSummary_struct*)_tmp2C2.f2)->tag == 5U){_LL3: _LL4:
 return(void*)& Cyc_IOEffect_return_js;}else{switch(*((int*)_tmp2C2.f1)){case 0U: if(((struct Cyc_IOEffect_Break_js_IOEffect_JmpSummary_struct*)_tmp2C2.f2)->tag == 0U){_LL5: _LL6:
 return(void*)& Cyc_IOEffect_break_js;}else{goto _LL11;}case 1U: if(((struct Cyc_IOEffect_Continue_js_IOEffect_JmpSummary_struct*)_tmp2C2.f2)->tag == 1U){_LL7: _LL8:
 return(void*)& Cyc_IOEffect_continue_js;}else{goto _LL11;}case 6U: if(((struct Cyc_IOEffect_NoJump_js_IOEffect_JmpSummary_struct*)_tmp2C2.f2)->tag == 6U){_LL9: _LLA:
 return(void*)& Cyc_IOEffect_nojump_js;}else{goto _LL11;}case 2U: if(((struct Cyc_IOEffect_Fallthru_js_IOEffect_JmpSummary_struct*)_tmp2C2.f2)->tag == 2U){_LLB: _tmp2C7=((struct Cyc_IOEffect_Fallthru_js_IOEffect_JmpSummary_struct*)_tmp2C2.f1)->f1;_tmp2C8=((struct Cyc_IOEffect_Fallthru_js_IOEffect_JmpSummary_struct*)_tmp2C2.f2)->f1;if(_tmp2C7 == _tmp2C8){_LLC: {struct Cyc_Absyn_Stmt*s1=_tmp2C7;struct Cyc_Absyn_Stmt*s2=_tmp2C8;
return j1;}}else{goto _LL11;}}else{goto _LL11;}case 3U: if(((struct Cyc_IOEffect_Goto_js_IOEffect_JmpSummary_struct*)_tmp2C2.f2)->tag == 3U){_LLD: _tmp2C5=((struct Cyc_IOEffect_Goto_js_IOEffect_JmpSummary_struct*)_tmp2C2.f1)->f1;_tmp2C6=((struct Cyc_IOEffect_Goto_js_IOEffect_JmpSummary_struct*)_tmp2C2.f2)->f1;if(_tmp2C5 == _tmp2C6){_LLE: {struct _fat_ptr*s1=_tmp2C5;struct _fat_ptr*s2=_tmp2C6;
return j1;}}else{goto _LL11;}}else{goto _LL11;}case 4U: if(((struct Cyc_IOEffect_Throw_js_IOEffect_JmpSummary_struct*)_tmp2C2.f2)->tag == 4U){_LLF: _tmp2C3=((struct Cyc_IOEffect_Throw_js_IOEffect_JmpSummary_struct*)_tmp2C2.f1)->f1;_tmp2C4=((struct Cyc_IOEffect_Throw_js_IOEffect_JmpSummary_struct*)_tmp2C2.f2)->f1;if(_tmp2C3 == _tmp2C4){_LL10: {struct Cyc_Absyn_Exp*s1=_tmp2C3;struct Cyc_Absyn_Exp*s2=_tmp2C4;
return j1;}}else{goto _LL11;}}else{goto _LL11;}default: _LL11: _LL12:
 return(void*)({struct Cyc_IOEffect_Or_js_IOEffect_JmpSummary_struct*_tmp2C9=_cycalloc(sizeof(*_tmp2C9));_tmp2C9->tag=7U,_tmp2C9->f1=j1,_tmp2C9->f2=j2;_tmp2C9;});}}}_LL0:;}
# 1967
static void Cyc_IOEffect_ioeffect_stmt(struct Cyc_IOEffect_Env*env,struct Cyc_Absyn_Stmt*s);
# 1986
static void Cyc_IOEffect_ioeffect_exp(struct Cyc_IOEffect_Env*env,struct Cyc_Absyn_Exp*e);struct Cyc_IOEffect_Contains{int found;struct _fat_ptr*v;struct Cyc_Absyn_Stmt*s;};
# 1994
static int Cyc_IOEffect_contains_e(struct Cyc_IOEffect_Contains*ret,struct Cyc_Absyn_Exp*e){
return 1;}
# 1997
static int Cyc_IOEffect_contains_s(struct Cyc_IOEffect_Contains*ret,struct Cyc_Absyn_Stmt*s){
# 1999
if(s == ret->s)goto OK;else{
# 2002
struct _fat_ptr*v=ret->v;
if(v != 0){void*_stmttmp26=s->r;void*_tmp2CC=_stmttmp26;struct _fat_ptr*_tmp2CD;if(((struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct*)_tmp2CC)->tag == 13U){_LL1: _tmp2CD=((struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct*)_tmp2CC)->f1;_LL2: {struct _fat_ptr*v1=_tmp2CD;
# 2006
if(({Cyc_strptrcmp(v,v1);})== 0)goto OK;goto _LL0;}}else{_LL3: _LL4:
 goto _LL0;}_LL0:;}else{
# 2009
return 0;}}
# 2011
return 1;
OK:
 ret->found=1;
return 0;}
# 2017
static int Cyc_IOEffect_contains_label(struct Cyc_Absyn_Stmt*s,struct _fat_ptr*v){
# 2019
struct Cyc_IOEffect_Contains c=({struct Cyc_IOEffect_Contains _tmp5AB;_tmp5AB.found=0,_tmp5AB.v=v,_tmp5AB.s=0;_tmp5AB;});
({({void(*_tmp6FD)(int(*)(struct Cyc_IOEffect_Contains*,struct Cyc_Absyn_Exp*),int(*)(struct Cyc_IOEffect_Contains*,struct Cyc_Absyn_Stmt*),struct Cyc_IOEffect_Contains*,struct Cyc_Absyn_Stmt*)=({void(*_tmp2CF)(int(*)(struct Cyc_IOEffect_Contains*,struct Cyc_Absyn_Exp*),int(*)(struct Cyc_IOEffect_Contains*,struct Cyc_Absyn_Stmt*),struct Cyc_IOEffect_Contains*,struct Cyc_Absyn_Stmt*)=(void(*)(int(*)(struct Cyc_IOEffect_Contains*,struct Cyc_Absyn_Exp*),int(*)(struct Cyc_IOEffect_Contains*,struct Cyc_Absyn_Stmt*),struct Cyc_IOEffect_Contains*,struct Cyc_Absyn_Stmt*))Cyc_Absyn_visit_stmt;_tmp2CF;});_tmp6FD(Cyc_IOEffect_contains_e,Cyc_IOEffect_contains_s,& c,s);});});
return c.found;}
# 2024
static int Cyc_IOEffect_contains_statement(struct Cyc_Absyn_Stmt*s,struct Cyc_Absyn_Stmt*s1){
# 2026
struct Cyc_IOEffect_Contains c=({struct Cyc_IOEffect_Contains _tmp5AC;_tmp5AC.found=0,_tmp5AC.v=0,_tmp5AC.s=s1;_tmp5AC;});
({({void(*_tmp6FE)(int(*)(struct Cyc_IOEffect_Contains*,struct Cyc_Absyn_Exp*),int(*)(struct Cyc_IOEffect_Contains*,struct Cyc_Absyn_Stmt*),struct Cyc_IOEffect_Contains*,struct Cyc_Absyn_Stmt*)=({void(*_tmp2D1)(int(*)(struct Cyc_IOEffect_Contains*,struct Cyc_Absyn_Exp*),int(*)(struct Cyc_IOEffect_Contains*,struct Cyc_Absyn_Stmt*),struct Cyc_IOEffect_Contains*,struct Cyc_Absyn_Stmt*)=(void(*)(int(*)(struct Cyc_IOEffect_Contains*,struct Cyc_Absyn_Exp*),int(*)(struct Cyc_IOEffect_Contains*,struct Cyc_Absyn_Stmt*),struct Cyc_IOEffect_Contains*,struct Cyc_Absyn_Stmt*))Cyc_Absyn_visit_stmt;_tmp2D1;});_tmp6FE(Cyc_IOEffect_contains_e,Cyc_IOEffect_contains_s,& c,s);});});
return c.found;}
# 2024
int Cyc_IOEffect_iter_jump_list(int(*cb)(struct Cyc_IOEffect_Node*,void*e),struct Cyc_IOEffect_Env*env,void*e){
# 2033
struct Cyc_List_List*iter=env->jlist;
struct Cyc_List_List*prev=0;
struct Cyc_IOEffect_Node*n;
int ret=0;
for(0;iter != 0;(prev=iter,iter=iter->tl)){
# 2039
n=(struct Cyc_IOEffect_Node*)iter->hd;
if(({cb(n,e);})){
# 2042
ret=1;
if(prev == 0)env->jlist=iter->tl;else{
prev->tl=iter->tl;}}}
# 2047
return ret;}
# 2024
int Cyc_IOEffect_remove_throw(struct Cyc_IOEffect_Node*n,struct Cyc_List_List*scs){
# 2053
void*_stmttmp27=n->n;void*_tmp2D4=_stmttmp27;struct Cyc_Absyn_Exp*_tmp2D5;if(((struct Cyc_IOEffect_Exp_IOEffect_NodeType_struct*)_tmp2D4)->tag == 0U){_LL1: _tmp2D5=((struct Cyc_IOEffect_Exp_IOEffect_NodeType_struct*)_tmp2D4)->f1;_LL2: {struct Cyc_Absyn_Exp*e=_tmp2D5;
# 2056
void*_stmttmp28=e->r;void*_tmp2D6=_stmttmp28;if(((struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct*)_tmp2D6)->tag == 11U){_LL6: _LL7: {
# 2059
struct Cyc_List_List*iter=n->succ;
for(0;iter != 0;iter->tl){
# 2062
struct Cyc_IOEffect_Node*ns=(struct Cyc_IOEffect_Node*)iter->hd;
void*_stmttmp29=ns->n;void*_tmp2D7=_stmttmp29;struct Cyc_Absyn_Stmt*_tmp2D8;if(((struct Cyc_IOEffect_Stmt_IOEffect_NodeType_struct*)_tmp2D7)->tag == 1U){_LLB: _tmp2D8=((struct Cyc_IOEffect_Stmt_IOEffect_NodeType_struct*)_tmp2D7)->f1;_LLC: {struct Cyc_Absyn_Stmt*s=_tmp2D8;
# 2066
struct Cyc_List_List*iter2=scs;
for(0;iter2 != 0 &&((struct Cyc_Absyn_Switch_clause*)iter2->hd)->body != s;iter2=iter2->tl){
;}
if(iter2 == 0)return 0;goto _LLA;}}else{_LLD: _LLE:
# 2071
 goto _LLA;}_LLA:;}
# 2074
return 1;}}else{_LL8: _LL9:
 return 0;}_LL5:;}}else{_LL3: _LL4:
# 2077
 return 0;}_LL0:;}
# 2082
static int Cyc_IOEffect_remove_gotos(struct Cyc_IOEffect_Node*n,struct Cyc_Absyn_Stmt*s){
# 2084
void*_stmttmp2A=n->n;void*_tmp2DA=_stmttmp2A;struct _fat_ptr*_tmp2DB;if(((struct Cyc_IOEffect_Stmt_IOEffect_NodeType_struct*)_tmp2DA)->tag == 1U){if(((struct Cyc_Absyn_Goto_s_Absyn_Raw_stmt_struct*)((struct Cyc_Absyn_Stmt*)((struct Cyc_IOEffect_Stmt_IOEffect_NodeType_struct*)_tmp2DA)->f1)->r)->tag == 8U){_LL1: _tmp2DB=((struct Cyc_Absyn_Goto_s_Absyn_Raw_stmt_struct*)(((struct Cyc_IOEffect_Stmt_IOEffect_NodeType_struct*)_tmp2DA)->f1)->r)->f1;_LL2: {struct _fat_ptr*x=_tmp2DB;
# 2087
return({Cyc_IOEffect_contains_label(s,x);});}}else{goto _LL3;}}else{_LL3: _LL4:
 return 0;}_LL0:;}
# 2093
static int Cyc_IOEffect_remove_loop_jump(struct Cyc_IOEffect_Node*n,void*z){
# 2095
void*_stmttmp2B=n->n;void*_tmp2DD=_stmttmp2B;if(((struct Cyc_IOEffect_Stmt_IOEffect_NodeType_struct*)_tmp2DD)->tag == 1U)switch(*((int*)((struct Cyc_Absyn_Stmt*)((struct Cyc_IOEffect_Stmt_IOEffect_NodeType_struct*)_tmp2DD)->f1)->r)){case 7U: _LL1: _LL2:
# 2097
 return 1;case 6U: _LL3: _LL4:
 return 1;default: goto _LL5;}else{_LL5: _LL6:
 return 0;}_LL0:;}
# 2103
static int Cyc_IOEffect_remove_pattern_jump(struct Cyc_IOEffect_Node*n,void*z){
# 2105
void*_stmttmp2C=n->n;void*_tmp2DF=_stmttmp2C;if(((struct Cyc_IOEffect_Stmt_IOEffect_NodeType_struct*)_tmp2DF)->tag == 1U)switch(*((int*)((struct Cyc_Absyn_Stmt*)((struct Cyc_IOEffect_Stmt_IOEffect_NodeType_struct*)_tmp2DF)->f1)->r)){case 6U: _LL1: _LL2:
# 2107
 return 1;case 11U: _LL3: _LL4:
 return 1;default: goto _LL5;}else{_LL5: _LL6:
 return 0;}_LL0:;}struct _tuple28{struct Cyc_List_List*f1;enum Cyc_IOEffect_NodeAnnot f2;};
# 2159 "ioeffect.cyc"
static struct _tuple28 Cyc_IOEffect_propagate_s_noinput(struct Cyc_List_List*nf,struct Cyc_IOEffect_Env*env,struct Cyc_Absyn_Stmt*s1,unsigned loc){
# 2167
struct _tuple28 _tmp2E1=({{void*_stmttmp2E=({Cyc_IOEffect_get_stmt(env->fenv,s1);})->n;void*_tmp2E4=_stmttmp2E;struct Cyc_Absyn_Exp*_tmp2E5;struct Cyc_Absyn_Stmt*_tmp2E6;if(((struct Cyc_IOEffect_Stmt_IOEffect_NodeType_struct*)_tmp2E4)->tag == 1U){_LL4: _tmp2E6=((struct Cyc_IOEffect_Stmt_IOEffect_NodeType_struct*)_tmp2E4)->f1;_LL5: {struct Cyc_Absyn_Stmt*s=_tmp2E6;({Cyc_IOEffect_ioeffect_stmt(env,s);});goto _LL3;}}else{_LL6: _tmp2E5=((struct Cyc_IOEffect_Exp_IOEffect_NodeType_struct*)_tmp2E4)->f1;_LL7: {struct Cyc_Absyn_Exp*e=_tmp2E5;({Cyc_IOEffect_ioeffect_exp(env,e);});goto _LL3;}}_LL3:;}({struct _tuple28 _tmp5AE;({struct Cyc_List_List*_tmp700=({struct Cyc_List_List*(*_tmp2E7)(struct Cyc_IOEffect_Node*n)=Cyc_IOEffect_get_output;struct Cyc_IOEffect_Node*_tmp2E8=({Cyc_IOEffect_get_stmt(env->fenv,s1);});_tmp2E7(_tmp2E8);});_tmp5AE.f1=_tmp700;}),({enum Cyc_IOEffect_NodeAnnot _tmp6FF=({Cyc_IOEffect_get_stmt(env->fenv,s1);})->annot;_tmp5AE.f2=_tmp6FF;});_tmp5AE;});});struct _tuple28 _stmttmp2D=_tmp2E1;enum Cyc_IOEffect_NodeAnnot _tmp2E3;struct Cyc_List_List*_tmp2E2;_LL1: _tmp2E2=_stmttmp2D.f1;_tmp2E3=_stmttmp2D.f2;_LL2: {struct Cyc_List_List*a=_tmp2E2;enum Cyc_IOEffect_NodeAnnot b=_tmp2E3;
# 2178 "ioeffect.cyc"
return({struct _tuple28 _tmp5AD;_tmp5AD.f1=a,_tmp5AD.f2=b;_tmp5AD;});}}
# 2183
static struct _tuple28 Cyc_IOEffect_propagate_s(struct Cyc_List_List*nf,struct Cyc_IOEffect_Env*env,struct Cyc_Absyn_Stmt*s1,unsigned loc){
# 2191
struct _tuple28 _tmp2EA=({({void(*_tmp2ED)(struct Cyc_IOEffect_Node*n,struct Cyc_List_List*f,unsigned loc)=Cyc_IOEffect_set_input_effect;struct Cyc_IOEffect_Node*_tmp2EE=({Cyc_IOEffect_get_stmt(env->fenv,s1);});struct Cyc_List_List*_tmp2EF=nf;unsigned _tmp2F0=loc;_tmp2ED(_tmp2EE,_tmp2EF,_tmp2F0);});{void*_stmttmp30=({Cyc_IOEffect_get_stmt(env->fenv,s1);})->n;void*_tmp2F1=_stmttmp30;struct Cyc_Absyn_Exp*_tmp2F2;struct Cyc_Absyn_Stmt*_tmp2F3;if(((struct Cyc_IOEffect_Stmt_IOEffect_NodeType_struct*)_tmp2F1)->tag == 1U){_LL4: _tmp2F3=((struct Cyc_IOEffect_Stmt_IOEffect_NodeType_struct*)_tmp2F1)->f1;_LL5: {struct Cyc_Absyn_Stmt*s=_tmp2F3;({Cyc_IOEffect_ioeffect_stmt(env,s);});goto _LL3;}}else{_LL6: _tmp2F2=((struct Cyc_IOEffect_Exp_IOEffect_NodeType_struct*)_tmp2F1)->f1;_LL7: {struct Cyc_Absyn_Exp*e=_tmp2F2;({Cyc_IOEffect_ioeffect_exp(env,e);});goto _LL3;}}_LL3:;}({struct _tuple28 _tmp5B0;({struct Cyc_List_List*_tmp702=({struct Cyc_List_List*(*_tmp2F4)(struct Cyc_IOEffect_Node*n)=Cyc_IOEffect_get_output;struct Cyc_IOEffect_Node*_tmp2F5=({Cyc_IOEffect_get_stmt(env->fenv,s1);});_tmp2F4(_tmp2F5);});_tmp5B0.f1=_tmp702;}),({enum Cyc_IOEffect_NodeAnnot _tmp701=({Cyc_IOEffect_get_stmt(env->fenv,s1);})->annot;_tmp5B0.f2=_tmp701;});_tmp5B0;});});struct _tuple28 _stmttmp2F=_tmp2EA;enum Cyc_IOEffect_NodeAnnot _tmp2EC;struct Cyc_List_List*_tmp2EB;_LL1: _tmp2EB=_stmttmp2F.f1;_tmp2EC=_stmttmp2F.f2;_LL2: {struct Cyc_List_List*a=_tmp2EB;enum Cyc_IOEffect_NodeAnnot b=_tmp2EC;
# 2202 "ioeffect.cyc"
return({struct _tuple28 _tmp5AF;_tmp5AF.f1=a,_tmp5AF.f2=b;_tmp5AF;});}}
# 2205
static struct _tuple28 Cyc_IOEffect_propagate_e(struct Cyc_List_List*nf,struct Cyc_IOEffect_Env*env,struct Cyc_Absyn_Exp*e,unsigned loc){
# 2213
return({({void(*_tmp2F7)(struct Cyc_IOEffect_Node*n,struct Cyc_List_List*f,unsigned loc)=Cyc_IOEffect_set_input_effect;struct Cyc_IOEffect_Node*_tmp2F8=({Cyc_IOEffect_get_exp(env->fenv,e);});struct Cyc_List_List*_tmp2F9=nf;unsigned _tmp2FA=loc;_tmp2F7(_tmp2F8,_tmp2F9,_tmp2FA);});{void*_stmttmp31=({Cyc_IOEffect_get_exp(env->fenv,e);})->n;void*_tmp2FB=_stmttmp31;struct Cyc_Absyn_Exp*_tmp2FC;struct Cyc_Absyn_Stmt*_tmp2FD;if(((struct Cyc_IOEffect_Stmt_IOEffect_NodeType_struct*)_tmp2FB)->tag == 1U){_LL1: _tmp2FD=((struct Cyc_IOEffect_Stmt_IOEffect_NodeType_struct*)_tmp2FB)->f1;_LL2: {struct Cyc_Absyn_Stmt*s=_tmp2FD;({Cyc_IOEffect_ioeffect_stmt(env,s);});goto _LL0;}}else{_LL3: _tmp2FC=((struct Cyc_IOEffect_Exp_IOEffect_NodeType_struct*)_tmp2FB)->f1;_LL4: {struct Cyc_Absyn_Exp*e=_tmp2FC;({Cyc_IOEffect_ioeffect_exp(env,e);});goto _LL0;}}_LL0:;}({struct _tuple28 _tmp5B1;({struct Cyc_List_List*_tmp704=({struct Cyc_List_List*(*_tmp2FE)(struct Cyc_IOEffect_Node*n)=Cyc_IOEffect_get_output;struct Cyc_IOEffect_Node*_tmp2FF=({Cyc_IOEffect_get_exp(env->fenv,e);});_tmp2FE(_tmp2FF);});_tmp5B1.f1=_tmp704;}),({enum Cyc_IOEffect_NodeAnnot _tmp703=({Cyc_IOEffect_get_exp(env->fenv,e);})->annot;_tmp5B1.f2=_tmp703;});_tmp5B1;});});}
# 2222
void Cyc_IOEffect_checked_set_output_effect(struct Cyc_IOEffect_Env*env,struct Cyc_IOEffect_Node*n,struct Cyc_List_List*out){
# 2225
if(env->const_effect &&({int(*_tmp301)(struct Cyc_List_List*f1,struct Cyc_List_List*f2)=Cyc_IOEffect_cmp_effect;struct Cyc_List_List*_tmp302=out;struct Cyc_List_List*_tmp303=({Cyc_IOEffect_get_input(n);});_tmp301(_tmp302,_tmp303);})== 0){
# 2228
({void*_tmp304=0U;({unsigned _tmp706=({Cyc_IOEffect_node_seg(n);});struct _fat_ptr _tmp705=({const char*_tmp305="Effect should be constant within expression/statement";_tag_fat(_tmp305,sizeof(char),54U);});Cyc_Tcutil_terr(_tmp706,_tmp705,_tag_fat(_tmp304,sizeof(void*),0U));});});
# 2230
out=({Cyc_IOEffect_get_input(n);});}
# 2225
({Cyc_IOEffect_set_output_effect(n,out);});}struct _tuple29{enum Cyc_IOEffect_NodeAnnot f1;enum Cyc_IOEffect_NodeAnnot f2;};
# 2235
static void Cyc_IOEffect_propagate_if_n_n_n_n(struct Cyc_List_List*nf,struct Cyc_IOEffect_Env*env,struct Cyc_IOEffect_Node*n1,struct Cyc_IOEffect_Node*n2,struct Cyc_IOEffect_Node*n3,unsigned loc,struct Cyc_IOEffect_Node*ifn){
# 2239
{
struct _tuple28 _tmp307=({({Cyc_IOEffect_set_input_effect(n1,nf,loc);});{void*_stmttmp33=n1->n;void*_tmp319=_stmttmp33;struct Cyc_Absyn_Exp*_tmp31A;struct Cyc_Absyn_Stmt*_tmp31B;if(((struct Cyc_IOEffect_Stmt_IOEffect_NodeType_struct*)_tmp319)->tag == 1U){_LL1F: _tmp31B=((struct Cyc_IOEffect_Stmt_IOEffect_NodeType_struct*)_tmp319)->f1;_LL20: {struct Cyc_Absyn_Stmt*s=_tmp31B;({Cyc_IOEffect_ioeffect_stmt(env,s);});goto _LL1E;}}else{_LL21: _tmp31A=((struct Cyc_IOEffect_Exp_IOEffect_NodeType_struct*)_tmp319)->f1;_LL22: {struct Cyc_Absyn_Exp*e=_tmp31A;({Cyc_IOEffect_ioeffect_exp(env,e);});goto _LL1E;}}_LL1E:;}({struct _tuple28 _tmp5B5;({struct Cyc_List_List*_tmp707=({Cyc_IOEffect_get_output(n1);});_tmp5B5.f1=_tmp707;}),_tmp5B5.f2=n1->annot;_tmp5B5;});});struct _tuple28 _stmttmp32=_tmp307;enum Cyc_IOEffect_NodeAnnot _tmp309;struct Cyc_List_List*_tmp308;_LL1: _tmp308=_stmttmp32.f1;_tmp309=_stmttmp32.f2;_LL2: {struct Cyc_List_List*f1=_tmp308;enum Cyc_IOEffect_NodeAnnot jmp1=_tmp309;
if((int)jmp1 != (int)0){ifn->annot=jmp1;goto RETURN;}
loc=({Cyc_IOEffect_node_seg(n1);});{
struct _tuple28 _tmp30A=({({Cyc_IOEffect_set_input_effect(n2,f1,loc);});{void*_stmttmp35=n2->n;void*_tmp316=_stmttmp35;struct Cyc_Absyn_Exp*_tmp317;struct Cyc_Absyn_Stmt*_tmp318;if(((struct Cyc_IOEffect_Stmt_IOEffect_NodeType_struct*)_tmp316)->tag == 1U){_LL1A: _tmp318=((struct Cyc_IOEffect_Stmt_IOEffect_NodeType_struct*)_tmp316)->f1;_LL1B: {struct Cyc_Absyn_Stmt*s=_tmp318;({Cyc_IOEffect_ioeffect_stmt(env,s);});goto _LL19;}}else{_LL1C: _tmp317=((struct Cyc_IOEffect_Exp_IOEffect_NodeType_struct*)_tmp316)->f1;_LL1D: {struct Cyc_Absyn_Exp*e=_tmp317;({Cyc_IOEffect_ioeffect_exp(env,e);});goto _LL19;}}_LL19:;}({struct _tuple28 _tmp5B4;({struct Cyc_List_List*_tmp708=({Cyc_IOEffect_get_output(n2);});_tmp5B4.f1=_tmp708;}),_tmp5B4.f2=n2->annot;_tmp5B4;});});struct _tuple28 _stmttmp34=_tmp30A;enum Cyc_IOEffect_NodeAnnot _tmp30C;struct Cyc_List_List*_tmp30B;_LL4: _tmp30B=_stmttmp34.f1;_tmp30C=_stmttmp34.f2;_LL5: {struct Cyc_List_List*f2=_tmp30B;enum Cyc_IOEffect_NodeAnnot jmp2=_tmp30C;
struct _tuple28 _tmp30D=({({Cyc_IOEffect_set_input_effect(n3,f1,loc);});{void*_stmttmp37=n3->n;void*_tmp313=_stmttmp37;struct Cyc_Absyn_Exp*_tmp314;struct Cyc_Absyn_Stmt*_tmp315;if(((struct Cyc_IOEffect_Stmt_IOEffect_NodeType_struct*)_tmp313)->tag == 1U){_LL15: _tmp315=((struct Cyc_IOEffect_Stmt_IOEffect_NodeType_struct*)_tmp313)->f1;_LL16: {struct Cyc_Absyn_Stmt*s=_tmp315;({Cyc_IOEffect_ioeffect_stmt(env,s);});goto _LL14;}}else{_LL17: _tmp314=((struct Cyc_IOEffect_Exp_IOEffect_NodeType_struct*)_tmp313)->f1;_LL18: {struct Cyc_Absyn_Exp*e=_tmp314;({Cyc_IOEffect_ioeffect_exp(env,e);});goto _LL14;}}_LL14:;}({struct _tuple28 _tmp5B3;({struct Cyc_List_List*_tmp709=({Cyc_IOEffect_get_output(n3);});_tmp5B3.f1=_tmp709;}),_tmp5B3.f2=n3->annot;_tmp5B3;});});struct _tuple28 _stmttmp36=_tmp30D;enum Cyc_IOEffect_NodeAnnot _tmp30F;struct Cyc_List_List*_tmp30E;_LL7: _tmp30E=_stmttmp36.f1;_tmp30F=_stmttmp36.f2;_LL8: {struct Cyc_List_List*f3=_tmp30E;enum Cyc_IOEffect_NodeAnnot jmp3=_tmp30F;
struct _tuple29 _stmttmp38=({struct _tuple29 _tmp5B2;_tmp5B2.f1=jmp2,_tmp5B2.f2=jmp3;_tmp5B2;});struct _tuple29 _tmp310=_stmttmp38;switch(_tmp310.f1){case Cyc_IOEffect_MustJump: switch(_tmp310.f2){case Cyc_IOEffect_MustJump: _LLA: _LLB:
# 2248
 if((int)1 != (int)0){ifn->annot=Cyc_IOEffect_MustJump;goto RETURN;}
goto _LL9;case Cyc_IOEffect_MayJump: goto _LL10;default: goto _LL12;}case Cyc_IOEffect_MayJump: if(_tmp310.f2 == Cyc_IOEffect_MayJump){_LLC: _LLD:
# 2251
({Cyc_IOEffect_checked_set_output_effect(env,ifn,f2);});
({Cyc_IOEffect_checked_set_output_effect(env,ifn,f3);});
goto _LL9;}else{_LLE: _LLF:
({Cyc_IOEffect_checked_set_output_effect(env,ifn,f2);});goto _LL9;}default: if(_tmp310.f2 == Cyc_IOEffect_MayJump){_LL10: _LL11:
({Cyc_IOEffect_checked_set_output_effect(env,ifn,f3);});goto _LL9;}else{_LL12: _LL13:
({({int(*_tmp70A)(struct _fat_ptr msg)=({int(*_tmp311)(struct _fat_ptr msg)=(int(*)(struct _fat_ptr msg))Cyc_IOEffect_env_err;_tmp311;});_tmp70A(({const char*_tmp312="propagate_if_n_n_n_n: impossible";_tag_fat(_tmp312,sizeof(char),33U);}));});});}}_LL9:;}}}}}
# 2259
RETURN: return;}
# 2263
static void Cyc_IOEffect_propagate_loop_e_e_s_e_s_n(struct Cyc_List_List*nf,struct Cyc_IOEffect_Env*env,struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2,struct Cyc_Absyn_Stmt*body,struct Cyc_Absyn_Exp*e3,struct Cyc_Absyn_Stmt*cont,unsigned loc,struct Cyc_IOEffect_Node*n){
# 2268
{
# 2270
struct Cyc_List_List*output_effect=0;
# 2272
if(e1 != 0){
# 2274
struct _tuple28 _stmttmp39=({Cyc_IOEffect_propagate_e(nf,env,e1,loc);});enum Cyc_IOEffect_NodeAnnot _tmp31E;struct Cyc_List_List*_tmp31D;_LL1: _tmp31D=_stmttmp39.f1;_tmp31E=_stmttmp39.f2;_LL2: {struct Cyc_List_List*f1=_tmp31D;enum Cyc_IOEffect_NodeAnnot jmp1=_tmp31E;
if((int)jmp1 != (int)0){n->annot=jmp1;goto RETURN;}
nf=f1;
output_effect=nf;
loc=e1->loc;}}
# 2272
if(e2 != 0){
# 2283
struct _tuple28 _stmttmp3A=({Cyc_IOEffect_propagate_e(nf,env,e2,loc);});enum Cyc_IOEffect_NodeAnnot _tmp320;struct Cyc_List_List*_tmp31F;_LL4: _tmp31F=_stmttmp3A.f1;_tmp320=_stmttmp3A.f2;_LL5: {struct Cyc_List_List*f2=_tmp31F;enum Cyc_IOEffect_NodeAnnot jmp2=_tmp320;
if((int)jmp2 != (int)0){n->annot=jmp2;goto RETURN;}
# 2286
({void(*_tmp321)(struct Cyc_IOEffect_Env*env,struct Cyc_IOEffect_Node*n,struct Cyc_List_List*out)=Cyc_IOEffect_checked_set_output_effect;struct Cyc_IOEffect_Env*_tmp322=env;struct Cyc_IOEffect_Node*_tmp323=({Cyc_IOEffect_get_exp(env->fenv,e2);});struct Cyc_List_List*_tmp324=nf;_tmp321(_tmp322,_tmp323,_tmp324);});
loc=e2->loc;
if(e1 == 0)output_effect=nf;}}
# 2272

# 2292
({Cyc_IOEffect_propagate_s(nf,env,cont,loc);});{
struct _tuple28 _stmttmp3B=({Cyc_IOEffect_propagate_s(nf,env,body,loc);});enum Cyc_IOEffect_NodeAnnot _tmp326;struct Cyc_List_List*_tmp325;_LL7: _tmp325=_stmttmp3B.f1;_tmp326=_stmttmp3B.f2;_LL8: {struct Cyc_List_List*f3=_tmp325;enum Cyc_IOEffect_NodeAnnot jmp3=_tmp326;
# 2295
if(({Cyc_IOEffect_iter_jump_list(Cyc_IOEffect_remove_loop_jump,env,0);})== 0 &&(int)jmp3 == (int)1){
if((int)1 != (int)0){n->annot=Cyc_IOEffect_MustJump;goto RETURN;}}
({void(*_tmp327)(struct Cyc_IOEffect_Env*env,struct Cyc_IOEffect_Node*n,struct Cyc_List_List*out)=Cyc_IOEffect_checked_set_output_effect;struct Cyc_IOEffect_Env*_tmp328=env;struct Cyc_IOEffect_Node*_tmp329=({Cyc_IOEffect_get_stmt(env->fenv,body);});struct Cyc_List_List*_tmp32A=nf;_tmp327(_tmp328,_tmp329,_tmp32A);});
loc=body->loc;}}
# 2301
if(e3 != 0){
# 2303
struct _tuple28 _stmttmp3C=({Cyc_IOEffect_propagate_e(nf,env,e3,loc);});enum Cyc_IOEffect_NodeAnnot _tmp32C;struct Cyc_List_List*_tmp32B;_LLA: _tmp32B=_stmttmp3C.f1;_tmp32C=_stmttmp3C.f2;_LLB: {struct Cyc_List_List*f4=_tmp32B;enum Cyc_IOEffect_NodeAnnot jmp4=_tmp32C;
if((int)jmp4 != (int)0){n->annot=jmp4;goto RETURN;}
({void(*_tmp32D)(struct Cyc_IOEffect_Env*env,struct Cyc_IOEffect_Node*n,struct Cyc_List_List*out)=Cyc_IOEffect_checked_set_output_effect;struct Cyc_IOEffect_Env*_tmp32E=env;struct Cyc_IOEffect_Node*_tmp32F=({Cyc_IOEffect_get_exp(env->fenv,e3);});struct Cyc_List_List*_tmp330=nf;_tmp32D(_tmp32E,_tmp32F,_tmp330);});}}
# 2301
({Cyc_IOEffect_checked_set_output_effect(env,n,nf);});}
# 2310
RETURN: return;}
# 2313
static struct _tuple28 Cyc_IOEffect_propagate_e_s(struct Cyc_List_List*nf,struct Cyc_IOEffect_Env*env,struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Stmt*s2,unsigned loc){
# 2316
struct _tuple28 _stmttmp3D=({Cyc_IOEffect_propagate_e(nf,env,e1,loc);});enum Cyc_IOEffect_NodeAnnot _tmp333;struct Cyc_List_List*_tmp332;_LL1: _tmp332=_stmttmp3D.f1;_tmp333=_stmttmp3D.f2;_LL2: {struct Cyc_List_List*f1=_tmp332;enum Cyc_IOEffect_NodeAnnot ne_annot=_tmp333;
return(int)ne_annot != (int)0?({struct _tuple28 _tmp5B6;_tmp5B6.f1=f1,_tmp5B6.f2=ne_annot;_tmp5B6;}):({Cyc_IOEffect_propagate_s(f1,env,s2,e1->loc);});}}
# 2321
static struct _tuple28 Cyc_IOEffect_propagate_e_e(struct Cyc_List_List*nf,struct Cyc_IOEffect_Env*env,struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2,unsigned loc){
# 2324
struct _tuple28 _stmttmp3E=({Cyc_IOEffect_propagate_e(nf,env,e1,loc);});enum Cyc_IOEffect_NodeAnnot _tmp336;struct Cyc_List_List*_tmp335;_LL1: _tmp335=_stmttmp3E.f1;_tmp336=_stmttmp3E.f2;_LL2: {struct Cyc_List_List*f1=_tmp335;enum Cyc_IOEffect_NodeAnnot ne1_annot=_tmp336;
return(int)ne1_annot != (int)0?({struct _tuple28 _tmp5B7;_tmp5B7.f1=f1,_tmp5B7.f2=ne1_annot;_tmp5B7;}):({Cyc_IOEffect_propagate_e(f1,env,e2,e1->loc);});}}
# 2328
static struct _tuple28 Cyc_IOEffect_propagate_list_e(struct Cyc_List_List*nf,struct Cyc_IOEffect_Env*env,struct Cyc_List_List*l,unsigned loc){
# 2331
for(0;l != 0;l=l->tl){
# 2338
struct _tuple28 _stmttmp3F=({Cyc_IOEffect_propagate_e(nf,env,(struct Cyc_Absyn_Exp*)l->hd,loc);});enum Cyc_IOEffect_NodeAnnot _tmp339;struct Cyc_List_List*_tmp338;_LL1: _tmp338=_stmttmp3F.f1;_tmp339=_stmttmp3F.f2;_LL2: {struct Cyc_List_List*nf1=_tmp338;enum Cyc_IOEffect_NodeAnnot jmp=_tmp339;
loc=((struct Cyc_Absyn_Exp*)l->hd)->loc;
if((int)jmp != (int)0)return({struct _tuple28 _tmp5B8;_tmp5B8.f1=nf1,_tmp5B8.f2=jmp;_tmp5B8;});nf=nf1;}}
# 2343
return({struct _tuple28 _tmp5B9;_tmp5B9.f1=nf,_tmp5B9.f2=Cyc_IOEffect_MayJump;_tmp5B9;});}struct _tuple30{void*f1;struct Cyc_Absyn_Exp*f2;};
# 2346
static struct _tuple28 Cyc_IOEffect_propagate_listc_e(struct Cyc_List_List*nf,struct Cyc_IOEffect_Env*env,struct Cyc_List_List*l,unsigned loc){
# 2349
for(0;l != 0;l=l->tl){
# 2351
struct Cyc_Absyn_Exp*e=(*((struct _tuple30*)l->hd)).f2;
struct _tuple28 _stmttmp40=({Cyc_IOEffect_propagate_e(nf,env,e,loc);});enum Cyc_IOEffect_NodeAnnot _tmp33C;struct Cyc_List_List*_tmp33B;_LL1: _tmp33B=_stmttmp40.f1;_tmp33C=_stmttmp40.f2;_LL2: {struct Cyc_List_List*nf1=_tmp33B;enum Cyc_IOEffect_NodeAnnot jmp=_tmp33C;
loc=e->loc;
if((int)jmp != (int)0)return({struct _tuple28 _tmp5BA;_tmp5BA.f1=nf1,_tmp5BA.f2=jmp;_tmp5BA;});nf=nf1;}}
# 2357
return({struct _tuple28 _tmp5BB;_tmp5BB.f1=nf,_tmp5BB.f2=Cyc_IOEffect_MayJump;_tmp5BB;});}
# 2346
static struct Cyc_List_List*Cyc_IOEffect_join_env(unsigned loc,struct Cyc_List_List*env,struct Cyc_List_List*in,struct Cyc_List_List*out,int sp);
# 2368
static void Cyc_IOEffect_ioeffect_exp(struct Cyc_IOEffect_Env*env,struct Cyc_Absyn_Exp*e){
# 2370
struct Cyc_IOEffect_Fenv*fenv=env->fenv;
struct Cyc_IOEffect_Node*n=({Cyc_IOEffect_get_exp(fenv,e);});
struct Cyc_List_List*nf=({Cyc_IOEffect_get_input(n);});
struct Cyc_List_List*const_nf=nf;
{void*_stmttmp41=e->r;void*_tmp33E=_stmttmp41;struct Cyc_Absyn_Exp*_tmp340;struct Cyc_Absyn_Exp*_tmp33F;struct Cyc_Absyn_Exp*_tmp341;struct Cyc_Absyn_MallocInfo _tmp342;struct Cyc_Absyn_Exp*_tmp344;struct Cyc_Absyn_Exp*_tmp343;struct Cyc_List_List*_tmp345;struct Cyc_List_List*_tmp346;struct Cyc_List_List*_tmp347;struct Cyc_List_List*_tmp348;struct Cyc_Absyn_Exp*_tmp349;struct Cyc_Absyn_Exp*_tmp34A;struct Cyc_Absyn_Exp*_tmp34B;struct Cyc_Absyn_Exp*_tmp34C;struct Cyc_Absyn_Exp*_tmp34D;struct Cyc_Absyn_Exp*_tmp34E;struct Cyc_Absyn_Exp*_tmp34F;struct Cyc_Absyn_Exp*_tmp350;struct Cyc_Absyn_Exp*_tmp351;struct Cyc_Absyn_Exp*_tmp352;struct Cyc_Absyn_Exp*_tmp353;struct _tuple8*_tmp356;struct Cyc_Absyn_Exp*_tmp355;struct Cyc_Absyn_Exp*_tmp354;struct _tuple8*_tmp359;struct Cyc_List_List*_tmp358;struct Cyc_Absyn_Exp*_tmp357;struct Cyc_Absyn_Exp*_tmp35B;struct Cyc_Absyn_Exp*_tmp35A;struct Cyc_Absyn_Exp*_tmp35D;struct Cyc_Absyn_Exp*_tmp35C;struct Cyc_Absyn_Exp*_tmp35F;struct Cyc_Absyn_Exp*_tmp35E;struct Cyc_Absyn_Exp*_tmp361;struct Cyc_Absyn_Exp*_tmp360;struct Cyc_Absyn_Exp*_tmp363;struct Cyc_Absyn_Exp*_tmp362;struct Cyc_Absyn_Exp*_tmp365;struct Cyc_Absyn_Exp*_tmp364;struct Cyc_Absyn_Exp*_tmp368;struct Cyc_Absyn_Exp*_tmp367;struct Cyc_Absyn_Exp*_tmp366;struct Cyc_List_List*_tmp369;struct Cyc_List_List*_tmp36A;struct Cyc_List_List*_tmp36B;struct Cyc_Absyn_Stmt*_tmp36C;switch(*((int*)_tmp33E)){case 32U: _LL1: _LL2:
# 2376
 goto _LL4;case 33U: _LL3: _LL4:
 goto _LL6;case 40U: _LL5: _LL6:
 goto _LL8;case 41U: _LL7: _LL8:
 goto _LLA;case 42U: _LL9: _LLA:
 goto _LLC;case 0U: _LLB: _LLC:
 goto _LLE;case 17U: _LLD: _LLE:
 goto _LL10;case 19U: _LLF: _LL10:
 goto _LL12;case 1U: _LL11: _LL12:
 goto _LL14;case 2U: _LL13: _LL14:
# 2394 "ioeffect.cyc"
({Cyc_IOEffect_checked_set_output_effect(env,n,nf);});
goto _LL0;case 38U: _LL15: _tmp36C=((struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct*)_tmp33E)->f1;_LL16: {struct Cyc_Absyn_Stmt*s=_tmp36C;
# 2398
struct _tuple28 _stmttmp42=({Cyc_IOEffect_propagate_s(nf,env,s,e->loc);});enum Cyc_IOEffect_NodeAnnot _tmp36E;struct Cyc_List_List*_tmp36D;_LL58: _tmp36D=_stmttmp42.f1;_tmp36E=_stmttmp42.f2;_LL59: {struct Cyc_List_List*nf1=_tmp36D;enum Cyc_IOEffect_NodeAnnot jmp=_tmp36E;
if((int)jmp != (int)0){n->annot=jmp;goto RETURN;}
({Cyc_IOEffect_checked_set_output_effect(env,n,nf1);});
goto _LL0;}}case 24U: _LL17: _tmp36B=((struct Cyc_Absyn_Tuple_e_Absyn_Raw_exp_struct*)_tmp33E)->f1;_LL18: {struct Cyc_List_List*le=_tmp36B;
_tmp36A=le;goto _LL1A;}case 31U: _LL19: _tmp36A=((struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*)_tmp33E)->f1;_LL1A: {struct Cyc_List_List*le=_tmp36A;
_tmp369=le;goto _LL1C;}case 3U: _LL1B: _tmp369=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp33E)->f2;_LL1C: {struct Cyc_List_List*le=_tmp369;
# 2405
struct _tuple28 _stmttmp43=({Cyc_IOEffect_propagate_list_e(nf,env,le,e->loc);});enum Cyc_IOEffect_NodeAnnot _tmp370;struct Cyc_List_List*_tmp36F;_LL5B: _tmp36F=_stmttmp43.f1;_tmp370=_stmttmp43.f2;_LL5C: {struct Cyc_List_List*nf1=_tmp36F;enum Cyc_IOEffect_NodeAnnot jmp=_tmp370;
if((int)jmp != (int)0){n->annot=jmp;goto RETURN;}
({Cyc_IOEffect_checked_set_output_effect(env,n,nf1);});
goto _LL0;}}case 6U: _LL1D: _tmp366=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp33E)->f1;_tmp367=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp33E)->f2;_tmp368=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp33E)->f3;_LL1E: {struct Cyc_Absyn_Exp*e1=_tmp366;struct Cyc_Absyn_Exp*e2=_tmp367;struct Cyc_Absyn_Exp*e3=_tmp368;
# 2410
({void(*_tmp371)(struct Cyc_List_List*nf,struct Cyc_IOEffect_Env*env,struct Cyc_IOEffect_Node*n1,struct Cyc_IOEffect_Node*n2,struct Cyc_IOEffect_Node*n3,unsigned loc,struct Cyc_IOEffect_Node*ifn)=Cyc_IOEffect_propagate_if_n_n_n_n;struct Cyc_List_List*_tmp372=nf;struct Cyc_IOEffect_Env*_tmp373=env;struct Cyc_IOEffect_Node*_tmp374=({Cyc_IOEffect_get_exp(env->fenv,e1);});struct Cyc_IOEffect_Node*_tmp375=({Cyc_IOEffect_get_exp(env->fenv,e2);});struct Cyc_IOEffect_Node*_tmp376=({Cyc_IOEffect_get_exp(env->fenv,e3);});unsigned _tmp377=e->loc;struct Cyc_IOEffect_Node*_tmp378=n;_tmp371(_tmp372,_tmp373,_tmp374,_tmp375,_tmp376,_tmp377,_tmp378);});
# 2412
goto _LL0;}case 23U: _LL1F: _tmp364=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp33E)->f1;_tmp365=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp33E)->f2;_LL20: {struct Cyc_Absyn_Exp*e1=_tmp364;struct Cyc_Absyn_Exp*e2=_tmp365;
# 2438 "ioeffect.cyc"
struct _tuple28 _stmttmp44=({Cyc_IOEffect_propagate_e_e(nf,env,e1,e2,e->loc);});enum Cyc_IOEffect_NodeAnnot _tmp37A;struct Cyc_List_List*_tmp379;_LL5E: _tmp379=_stmttmp44.f1;_tmp37A=_stmttmp44.f2;_LL5F: {struct Cyc_List_List*f1=_tmp379;enum Cyc_IOEffect_NodeAnnot jmp1=_tmp37A;
if((int)jmp1 != (int)0){n->annot=jmp1;goto RETURN;}{
struct _tuple28 _stmttmp45=({Cyc_IOEffect_propagate_e_e(f1,env,e1,e2,e->loc);});enum Cyc_IOEffect_NodeAnnot _tmp37C;struct Cyc_List_List*_tmp37B;_LL61: _tmp37B=_stmttmp45.f1;_tmp37C=_stmttmp45.f2;_LL62: {struct Cyc_List_List*f2=_tmp37B;enum Cyc_IOEffect_NodeAnnot jmp2=_tmp37C;
if((int)jmp2 != (int)0){n->annot=jmp2;goto RETURN;}
({Cyc_IOEffect_checked_set_output_effect(env,n,f2);});
({Cyc_IOEffect_check_access(fenv,e1->loc,f2,e1->topt);});
goto _LL0;}}}}case 7U: _LL21: _tmp362=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp33E)->f1;_tmp363=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp33E)->f2;_LL22: {struct Cyc_Absyn_Exp*e1=_tmp362;struct Cyc_Absyn_Exp*e2=_tmp363;
_tmp360=e1;_tmp361=e2;goto _LL24;}case 8U: _LL23: _tmp360=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp33E)->f1;_tmp361=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp33E)->f2;_LL24: {struct Cyc_Absyn_Exp*e1=_tmp360;struct Cyc_Absyn_Exp*e2=_tmp361;
# 2447
int prev=({int tmp=env->const_effect;env->const_effect=1;tmp;});
struct _tuple28 _stmttmp46=({Cyc_IOEffect_propagate_e_e(nf,env,e1,e2,e->loc);});enum Cyc_IOEffect_NodeAnnot _tmp37D;_LL64: _tmp37D=_stmttmp46.f2;_LL65: {enum Cyc_IOEffect_NodeAnnot jmp=_tmp37D;
if((int)jmp != (int)0)({int tmp=env->const_effect;env->const_effect=prev;tmp;});if((int)jmp != (int)0){
n->annot=jmp;goto RETURN;}
({Cyc_IOEffect_checked_set_output_effect(env,n,nf);});
({int tmp=env->const_effect;env->const_effect=prev;tmp;});
goto _LL0;}}case 4U: _LL25: _tmp35E=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp33E)->f1;_tmp35F=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp33E)->f3;_LL26: {struct Cyc_Absyn_Exp*e1=_tmp35E;struct Cyc_Absyn_Exp*e2=_tmp35F;
_tmp35C=e1;_tmp35D=e2;goto _LL28;}case 36U: _LL27: _tmp35C=((struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct*)_tmp33E)->f1;_tmp35D=((struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct*)_tmp33E)->f2;_LL28: {struct Cyc_Absyn_Exp*e1=_tmp35C;struct Cyc_Absyn_Exp*e2=_tmp35D;
_tmp35A=e1;_tmp35B=e2;goto _LL2A;}case 9U: _LL29: _tmp35A=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp33E)->f1;_tmp35B=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp33E)->f2;_LL2A: {struct Cyc_Absyn_Exp*e1=_tmp35A;struct Cyc_Absyn_Exp*e2=_tmp35B;
# 2457
struct _tuple28 _stmttmp47=({Cyc_IOEffect_propagate_e_e(nf,env,e1,e2,e->loc);});enum Cyc_IOEffect_NodeAnnot _tmp37F;struct Cyc_List_List*_tmp37E;_LL67: _tmp37E=_stmttmp47.f1;_tmp37F=_stmttmp47.f2;_LL68: {struct Cyc_List_List*f=_tmp37E;enum Cyc_IOEffect_NodeAnnot jmp=_tmp37F;
if((int)jmp != (int)0){n->annot=jmp;goto RETURN;}
({Cyc_IOEffect_checked_set_output_effect(env,n,f);});
goto _LL0;}}case 10U: _LL2B: _tmp357=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp33E)->f1;_tmp358=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp33E)->f2;_tmp359=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp33E)->f5;_LL2C: {struct Cyc_Absyn_Exp*e1=_tmp357;struct Cyc_List_List*le=_tmp358;struct _tuple8*res=_tmp359;
# 2468
struct Cyc_List_List*env_effect_in=({Cyc_Absyn_copy_effect(nf);});
struct _tuple28 _stmttmp48=({Cyc_IOEffect_propagate_e(nf,env,e1,e->loc);});enum Cyc_IOEffect_NodeAnnot _tmp381;struct Cyc_List_List*_tmp380;_LL6A: _tmp380=_stmttmp48.f1;_tmp381=_stmttmp48.f2;_LL6B: {struct Cyc_List_List*f1=_tmp380;enum Cyc_IOEffect_NodeAnnot jmp1=_tmp381;
if((int)jmp1 != (int)0){n->annot=jmp1;goto RETURN;}{
struct _tuple28 _stmttmp49=({Cyc_IOEffect_propagate_list_e(f1,env,le,e1->loc);});enum Cyc_IOEffect_NodeAnnot _tmp383;struct Cyc_List_List*_tmp382;_LL6D: _tmp382=_stmttmp49.f1;_tmp383=_stmttmp49.f2;_LL6E: {struct Cyc_List_List*f2=_tmp382;enum Cyc_IOEffect_NodeAnnot jmp2=_tmp383;
({struct _tuple8 _tmp70B=({struct _tuple8 _tmp5BC;_tmp5BC.f1=env_effect_in,_tmp5BC.f2=0;_tmp5BC;});*res=_tmp70B;});
if((int)jmp2 != (int)0){n->annot=jmp2;goto RETURN;}{
struct _tuple27 _stmttmp4A=({Cyc_IOEffect_check_fun_call_ptr(env,e1,0,e->loc);});int _tmp385;void*_tmp384;_LL70: _tmp384=_stmttmp4A.f1;_tmp385=_stmttmp4A.f2;_LL71: {void*ftyp=_tmp384;int noreturn=_tmp385;
struct Cyc_Absyn_FnInfo i=({Cyc_IOEffect_fn_type(ftyp);});
struct Cyc_List_List*out=({Cyc_IOEffect_join_env(e1->loc,f2,i.ieffect,i.oeffect,0);});
({struct _tuple8 _tmp70D=({struct _tuple8 _tmp5BD;_tmp5BD.f1=env_effect_in,({struct Cyc_List_List*_tmp70C=({Cyc_Absyn_copy_effect(out);});_tmp5BD.f2=_tmp70C;});_tmp5BD;});*res=_tmp70D;});
# 2485
if(noreturn){
# 2490
if((int)1 != (int)0){n->annot=Cyc_IOEffect_MustJump;goto RETURN;}}else{
# 2492
({Cyc_IOEffect_checked_set_output_effect(env,n,out);});}
goto _LL0;}}}}}}case 34U: _LL2D: _tmp354=((struct Cyc_Absyn_Spawn_e_Absyn_Raw_exp_struct*)_tmp33E)->f1;_tmp355=((struct Cyc_Absyn_Spawn_e_Absyn_Raw_exp_struct*)_tmp33E)->f2;_tmp356=((struct Cyc_Absyn_Spawn_e_Absyn_Raw_exp_struct*)_tmp33E)->f3;_LL2E: {struct Cyc_Absyn_Exp*e1=_tmp354;struct Cyc_Absyn_Exp*e2=_tmp355;struct _tuple8*res=_tmp356;
# 2500
struct Cyc_List_List*env_effect_in=({Cyc_Absyn_copy_effect(nf);});
({struct _tuple8 _tmp70E=({struct _tuple8 _tmp5BE;_tmp5BE.f1=env_effect_in,_tmp5BE.f2=0;_tmp5BE;});*res=_tmp70E;});{
struct _tuple28 _stmttmp4B=({Cyc_IOEffect_propagate_e(nf,env,e1,e->loc);});enum Cyc_IOEffect_NodeAnnot _tmp387;struct Cyc_List_List*_tmp386;_LL73: _tmp386=_stmttmp4B.f1;_tmp387=_stmttmp4B.f2;_LL74: {struct Cyc_List_List*f1=_tmp386;enum Cyc_IOEffect_NodeAnnot jmp1=_tmp387;
if((int)jmp1 != (int)0){n->annot=jmp1;goto RETURN;}{
struct Cyc_List_List*ef_in=0;
struct Cyc_List_List*ef_out=0;
{void*_stmttmp4C=e2->r;void*_tmp388=_stmttmp4C;struct Cyc_List_List*_tmp38A;struct Cyc_Absyn_Exp*_tmp389;if(((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp388)->tag == 10U){_LL76: _tmp389=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp388)->f1;_tmp38A=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp388)->f2;_LL77: {struct Cyc_Absyn_Exp*fn=_tmp389;struct Cyc_List_List*es=_tmp38A;
# 2508
struct _tuple27 _stmttmp4D=({Cyc_IOEffect_check_fun_call_ptr(env,fn,1,e->loc);});void*_tmp38B;_LL7B: _tmp38B=_stmttmp4D.f1;_LL7C: {void*ftyp=_tmp38B;
# 2510
struct Cyc_Absyn_FnInfo i=({Cyc_IOEffect_fn_type(ftyp);});
ef_in=i.ieffect;
ef_out=i.oeffect;{
struct _tuple28 _stmttmp4E=({({struct Cyc_List_List*_tmp711=f1;struct Cyc_IOEffect_Env*_tmp710=env;struct Cyc_List_List*_tmp70F=({struct Cyc_List_List*_tmp38E=_cycalloc(sizeof(*_tmp38E));
# 2515
_tmp38E->hd=fn,_tmp38E->tl=es;_tmp38E;});Cyc_IOEffect_propagate_list_e(_tmp711,_tmp710,_tmp70F,e1->loc);});});
# 2513
enum Cyc_IOEffect_NodeAnnot _tmp38D;struct Cyc_List_List*_tmp38C;_LL7E: _tmp38C=_stmttmp4E.f1;_tmp38D=_stmttmp4E.f2;_LL7F: {struct Cyc_List_List*f2=_tmp38C;enum Cyc_IOEffect_NodeAnnot jmp2=_tmp38D;
# 2516
if((int)jmp2 != (int)0){n->annot=jmp2;goto RETURN;}
f1=f2;
goto _LL75;}}}}}else{_LL78: _LL79:
({({int(*_tmp712)(struct _fat_ptr msg)=({int(*_tmp38F)(struct _fat_ptr msg)=(int(*)(struct _fat_ptr msg))Cyc_IOEffect_env_err;_tmp38F;});_tmp712(({const char*_tmp390="ioeffect spawn_e";_tag_fat(_tmp390,sizeof(char),17U);}));});});}_LL75:;}{
# 2521
struct Cyc_List_List*out=({Cyc_IOEffect_join_env(e1->loc,f1,ef_in,ef_out,1);});
({struct _tuple8 _tmp714=({struct _tuple8 _tmp5BF;_tmp5BF.f1=env_effect_in,({struct Cyc_List_List*_tmp713=({Cyc_Absyn_copy_effect(out);});_tmp5BF.f2=_tmp713;});_tmp5BF;});*res=_tmp714;});
# 2528
({Cyc_IOEffect_checked_set_output_effect(env,n,out);});
goto _LL0;}}}}}case 22U: _LL2F: _tmp353=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp33E)->f1;_LL30: {struct Cyc_Absyn_Exp*e1=_tmp353;
_tmp352=e1;goto _LL32;}case 20U: _LL31: _tmp352=((struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_tmp33E)->f1;_LL32: {struct Cyc_Absyn_Exp*e1=_tmp352;
# 2532
struct _tuple28 _stmttmp4F=({Cyc_IOEffect_propagate_e(nf,env,e1,e->loc);});enum Cyc_IOEffect_NodeAnnot _tmp392;struct Cyc_List_List*_tmp391;_LL81: _tmp391=_stmttmp4F.f1;_tmp392=_stmttmp4F.f2;_LL82: {struct Cyc_List_List*f=_tmp391;enum Cyc_IOEffect_NodeAnnot jmp=_tmp392;
if((int)jmp != (int)0){n->annot=jmp;goto RETURN;}
({Cyc_IOEffect_checked_set_output_effect(env,n,f);});
({Cyc_IOEffect_check_access(fenv,e1->loc,f,e1->topt);});
goto _LL0;}}case 11U: _LL33: _tmp351=((struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct*)_tmp33E)->f1;_LL34: {struct Cyc_Absyn_Exp*e1=_tmp351;
# 2538
struct _tuple28 _stmttmp50=({Cyc_IOEffect_propagate_e(nf,env,e1,e->loc);});enum Cyc_IOEffect_NodeAnnot _tmp394;struct Cyc_List_List*_tmp393;_LL84: _tmp393=_stmttmp50.f1;_tmp394=_stmttmp50.f2;_LL85: {struct Cyc_List_List*f=_tmp393;enum Cyc_IOEffect_NodeAnnot jmp=_tmp394;
if((int)jmp != (int)0){n->annot=jmp;goto RETURN;}
({Cyc_IOEffect_add_jump_node(env,n);});
({Cyc_IOEffect_propagate_succ(n,f);});
if((int)1 != (int)0){n->annot=Cyc_IOEffect_MustJump;goto RETURN;}
goto _LL0;}}case 39U: _LL35: _tmp350=((struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct*)_tmp33E)->f1;_LL36: {struct Cyc_Absyn_Exp*e1=_tmp350;
_tmp34F=e1;goto _LL38;}case 13U: _LL37: _tmp34F=((struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*)_tmp33E)->f1;_LL38: {struct Cyc_Absyn_Exp*e1=_tmp34F;
_tmp34E=e1;goto _LL3A;}case 14U: _LL39: _tmp34E=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp33E)->f2;_LL3A: {struct Cyc_Absyn_Exp*e1=_tmp34E;
_tmp34D=e1;goto _LL3C;}case 18U: _LL3B: _tmp34D=((struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct*)_tmp33E)->f1;_LL3C: {struct Cyc_Absyn_Exp*e1=_tmp34D;
_tmp34C=e1;goto _LL3E;}case 5U: _LL3D: _tmp34C=((struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*)_tmp33E)->f1;_LL3E: {struct Cyc_Absyn_Exp*e1=_tmp34C;
_tmp34B=e1;goto _LL40;}case 12U: _LL3F: _tmp34B=((struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*)_tmp33E)->f1;_LL40: {struct Cyc_Absyn_Exp*e1=_tmp34B;
_tmp34A=e1;goto _LL42;}case 21U: _LL41: _tmp34A=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp33E)->f1;_LL42: {struct Cyc_Absyn_Exp*e1=_tmp34A;
_tmp349=e1;goto _LL44;}case 15U: _LL43: _tmp349=((struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct*)_tmp33E)->f1;_LL44: {struct Cyc_Absyn_Exp*e1=_tmp349;
# 2552
struct _tuple28 _stmttmp51=({Cyc_IOEffect_propagate_e(nf,env,e1,e->loc);});enum Cyc_IOEffect_NodeAnnot _tmp396;struct Cyc_List_List*_tmp395;_LL87: _tmp395=_stmttmp51.f1;_tmp396=_stmttmp51.f2;_LL88: {struct Cyc_List_List*f1=_tmp395;enum Cyc_IOEffect_NodeAnnot jmp1=_tmp396;
if((int)jmp1 != (int)0){n->annot=jmp1;goto RETURN;}
({Cyc_IOEffect_checked_set_output_effect(env,n,f1);});
goto _LL0;}}case 25U: _LL45: _tmp348=((struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct*)_tmp33E)->f2;_LL46: {struct Cyc_List_List*lst=_tmp348;
_tmp347=lst;goto _LL48;}case 26U: _LL47: _tmp347=((struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*)_tmp33E)->f1;_LL48: {struct Cyc_List_List*lst=_tmp347;
_tmp346=lst;goto _LL4A;}case 29U: _LL49: _tmp346=((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_tmp33E)->f3;_LL4A: {struct Cyc_List_List*lst=_tmp346;
_tmp345=lst;goto _LL4C;}case 30U: _LL4B: _tmp345=((struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct*)_tmp33E)->f2;_LL4C: {struct Cyc_List_List*lst=_tmp345;
# 2560
struct _tuple28 _stmttmp52=({Cyc_IOEffect_propagate_listc_e(nf,env,lst,e->loc);});enum Cyc_IOEffect_NodeAnnot _tmp398;struct Cyc_List_List*_tmp397;_LL8A: _tmp397=_stmttmp52.f1;_tmp398=_stmttmp52.f2;_LL8B: {struct Cyc_List_List*f=_tmp397;enum Cyc_IOEffect_NodeAnnot jmp=_tmp398;
if((int)jmp != (int)0){n->annot=jmp;goto RETURN;}
({Cyc_IOEffect_checked_set_output_effect(env,n,f);});
goto _LL0;}}case 16U: _LL4D: _tmp343=((struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct*)_tmp33E)->f1;_tmp344=((struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct*)_tmp33E)->f2;_LL4E: {struct Cyc_Absyn_Exp*eopt=_tmp343;struct Cyc_Absyn_Exp*e1=_tmp344;
# 2565
unsigned loc=e->loc;
if(eopt != 0){
# 2568
struct _tuple28 _stmttmp53=({Cyc_IOEffect_propagate_e(nf,env,eopt,loc);});enum Cyc_IOEffect_NodeAnnot _tmp39A;struct Cyc_List_List*_tmp399;_LL8D: _tmp399=_stmttmp53.f1;_tmp39A=_stmttmp53.f2;_LL8E: {struct Cyc_List_List*f=_tmp399;enum Cyc_IOEffect_NodeAnnot jmp=_tmp39A;
if((int)jmp != (int)0){n->annot=jmp;goto RETURN;}
loc=eopt->loc;
nf=f;}}{
# 2566
struct _tuple28 _stmttmp54=({Cyc_IOEffect_propagate_e(nf,env,e1,loc);});enum Cyc_IOEffect_NodeAnnot _tmp39C;struct Cyc_List_List*_tmp39B;_LL90: _tmp39B=_stmttmp54.f1;_tmp39C=_stmttmp54.f2;_LL91: {struct Cyc_List_List*f2=_tmp39B;enum Cyc_IOEffect_NodeAnnot jmp2=_tmp39C;
# 2574
if((int)jmp2 != (int)0){n->annot=jmp2;goto RETURN;}
({Cyc_IOEffect_checked_set_output_effect(env,n,f2);});
if(eopt != 0)
({Cyc_IOEffect_check_live(fenv,eopt->loc,f2,eopt->topt);});
# 2576
goto _LL0;}}}case 35U: _LL4F: _tmp342=((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp33E)->f1;_LL50: {struct Cyc_Absyn_MallocInfo m=_tmp342;
# 2580
struct Cyc_Absyn_Exp*eopt=m.rgn;
unsigned loc=e->loc;
if(eopt != 0){
# 2584
struct _tuple28 _stmttmp55=({Cyc_IOEffect_propagate_e(nf,env,eopt,loc);});enum Cyc_IOEffect_NodeAnnot _tmp39E;struct Cyc_List_List*_tmp39D;_LL93: _tmp39D=_stmttmp55.f1;_tmp39E=_stmttmp55.f2;_LL94: {struct Cyc_List_List*f=_tmp39D;enum Cyc_IOEffect_NodeAnnot jmp=_tmp39E;
if((int)jmp != (int)0){n->annot=jmp;goto RETURN;}
nf=f;
loc=eopt->loc;}}{
# 2582
struct _tuple28 _stmttmp56=({Cyc_IOEffect_propagate_e(nf,env,m.num_elts,loc);});enum Cyc_IOEffect_NodeAnnot _tmp3A0;struct Cyc_List_List*_tmp39F;_LL96: _tmp39F=_stmttmp56.f1;_tmp3A0=_stmttmp56.f2;_LL97: {struct Cyc_List_List*f2=_tmp39F;enum Cyc_IOEffect_NodeAnnot jmp2=_tmp3A0;
# 2590
if((int)jmp2 != (int)0){n->annot=jmp2;goto RETURN;}
({Cyc_IOEffect_checked_set_output_effect(env,n,f2);});
if(eopt != 0)
({Cyc_IOEffect_check_live(fenv,eopt->loc,f2,eopt->topt);});
# 2592
goto _LL0;}}}case 28U: _LL51: _tmp341=((struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*)_tmp33E)->f1;_LL52: {struct Cyc_Absyn_Exp*e1=_tmp341;
# 2597
int prev=({int tmp=env->const_effect;env->const_effect=1;tmp;});
struct _tuple28 _stmttmp57=({Cyc_IOEffect_propagate_e(nf,env,e1,e->loc);});enum Cyc_IOEffect_NodeAnnot _tmp3A2;struct Cyc_List_List*_tmp3A1;_LL99: _tmp3A1=_stmttmp57.f1;_tmp3A2=_stmttmp57.f2;_LL9A: {struct Cyc_List_List*f1=_tmp3A1;enum Cyc_IOEffect_NodeAnnot jmp1=_tmp3A2;
if((int)jmp1 != (int)0)({int tmp=env->const_effect;env->const_effect=prev;tmp;});if((int)jmp1 != (int)0){
n->annot=jmp1;goto RETURN;}
({Cyc_IOEffect_checked_set_output_effect(env,n,nf);});
({int tmp=env->const_effect;env->const_effect=prev;tmp;});
goto _LL0;}}case 27U: _LL53: _tmp33F=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmp33E)->f2;_tmp340=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmp33E)->f3;_LL54: {struct Cyc_Absyn_Exp*e1=_tmp33F;struct Cyc_Absyn_Exp*e2=_tmp340;
# 2605
int prev=({int tmp=env->const_effect;env->const_effect=1;tmp;});
struct _tuple28 _stmttmp58=({Cyc_IOEffect_propagate_e_e(nf,env,e1,e2,e->loc);});enum Cyc_IOEffect_NodeAnnot _tmp3A3;_LL9C: _tmp3A3=_stmttmp58.f2;_LL9D: {enum Cyc_IOEffect_NodeAnnot jmp1=_tmp3A3;
if((int)jmp1 != (int)0)({int tmp=env->const_effect;env->const_effect=prev;tmp;});if((int)jmp1 != (int)0){
n->annot=jmp1;goto RETURN;}
({Cyc_IOEffect_checked_set_output_effect(env,n,nf);});
({int tmp=env->const_effect;env->const_effect=prev;tmp;});
goto _LL0;}}default: _LL55: _LL56:
({({int(*_tmp715)(struct _fat_ptr msg)=({int(*_tmp3A4)(struct _fat_ptr msg)=(int(*)(struct _fat_ptr msg))Cyc_IOEffect_env_err;_tmp3A4;});_tmp715(({const char*_tmp3A5="ioeffect_exp";_tag_fat(_tmp3A5,sizeof(char),13U);}));});});goto _LL0;}_LL0:;}
# 2615
RETURN: return;}
# 2644 "ioeffect.cyc"
static void Cyc_IOEffect_ioeffect_stmt(struct Cyc_IOEffect_Env*env,struct Cyc_Absyn_Stmt*s){
# 2646
struct Cyc_IOEffect_Fenv*fenv=env->fenv;
struct Cyc_IOEffect_Node*n=({Cyc_IOEffect_get_stmt(fenv,s);});
struct Cyc_List_List*nf=({Cyc_IOEffect_get_input(n);});
# 2654
{void*_stmttmp59=s->r;void*_tmp3A7=_stmttmp59;struct Cyc_Absyn_Stmt*_tmp3A8;struct Cyc_Absyn_Stmt*_tmp3A9;struct Cyc_Absyn_Stmt*_tmp3AB;struct Cyc_Absyn_Fndecl*_tmp3AA;struct Cyc_Absyn_Stmt*_tmp3AD;struct Cyc_Absyn_Vardecl*_tmp3AC;struct Cyc_Absyn_Stmt*_tmp3B0;struct Cyc_Absyn_Exp*_tmp3AF;struct Cyc_Absyn_Pat*_tmp3AE;struct Cyc_Absyn_Stmt*_tmp3B3;struct Cyc_Absyn_Exp*_tmp3B2;struct Cyc_Absyn_Tvar*_tmp3B1;struct Cyc_List_List*_tmp3B5;struct Cyc_Absyn_Exp*_tmp3B4;struct Cyc_List_List*_tmp3B7;struct Cyc_Absyn_Stmt*_tmp3B6;struct Cyc_Absyn_Stmt*_tmp3BA;struct Cyc_Absyn_Exp*_tmp3B9;struct Cyc_Absyn_Stmt*_tmp3B8;struct Cyc_Absyn_Stmt*_tmp3BF;struct Cyc_Absyn_Stmt*_tmp3BE;struct Cyc_Absyn_Exp*_tmp3BD;struct Cyc_Absyn_Exp*_tmp3BC;struct Cyc_Absyn_Exp*_tmp3BB;struct Cyc_Absyn_Stmt*_tmp3C2;struct Cyc_Absyn_Stmt*_tmp3C1;struct Cyc_Absyn_Exp*_tmp3C0;struct Cyc_Absyn_Stmt*_tmp3C5;struct Cyc_Absyn_Stmt*_tmp3C4;struct Cyc_Absyn_Exp*_tmp3C3;struct Cyc_Absyn_Stmt*_tmp3C7;struct Cyc_Absyn_Stmt*_tmp3C6;struct Cyc_Absyn_Exp*_tmp3C8;struct Cyc_Absyn_Exp*_tmp3C9;switch(*((int*)_tmp3A7)){case 0U: _LL1: _LL2:
# 2656
({Cyc_IOEffect_checked_set_output_effect(env,n,nf);});goto _LL0;case 6U: _LL3: _LL4:
 goto _LL6;case 7U: _LL5: _LL6:
 goto _LL8;case 8U: _LL7: _LL8:
 goto _LLA;case 11U: _LL9: _LLA:
 _tmp3C9=0;goto _LLC;case 3U: _LLB: _tmp3C9=((struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct*)_tmp3A7)->f1;_LLC: {struct Cyc_Absyn_Exp*e0=_tmp3C9;
# 2662
if(e0 != 0){
# 2664
struct _tuple28 _stmttmp5A=({Cyc_IOEffect_propagate_e(nf,env,e0,s->loc);});enum Cyc_IOEffect_NodeAnnot _tmp3CB;struct Cyc_List_List*_tmp3CA;_LL2A: _tmp3CA=_stmttmp5A.f1;_tmp3CB=_stmttmp5A.f2;_LL2B: {struct Cyc_List_List*f=_tmp3CA;enum Cyc_IOEffect_NodeAnnot jmp=_tmp3CB;
if((int)jmp != (int)0){n->annot=jmp;goto RETURN;}
nf=f;}}
# 2662
({Cyc_IOEffect_add_jump_node(env,n);});
# 2669
({Cyc_IOEffect_propagate_succ(n,nf);});
if((int)1 != (int)0){n->annot=Cyc_IOEffect_MustJump;goto RETURN;}
goto _LL0;}case 1U: _LLD: _tmp3C8=((struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct*)_tmp3A7)->f1;_LLE: {struct Cyc_Absyn_Exp*e=_tmp3C8;
# 2673
struct _tuple28 _stmttmp5B=({Cyc_IOEffect_propagate_e(nf,env,e,s->loc);});enum Cyc_IOEffect_NodeAnnot _tmp3CD;struct Cyc_List_List*_tmp3CC;_LL2D: _tmp3CC=_stmttmp5B.f1;_tmp3CD=_stmttmp5B.f2;_LL2E: {struct Cyc_List_List*f=_tmp3CC;enum Cyc_IOEffect_NodeAnnot jmp=_tmp3CD;
if((int)jmp != (int)0){n->annot=jmp;goto RETURN;}
({Cyc_IOEffect_checked_set_output_effect(env,n,f);});
goto _LL0;}}case 2U: _LLF: _tmp3C6=((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_tmp3A7)->f1;_tmp3C7=((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_tmp3A7)->f2;_LL10: {struct Cyc_Absyn_Stmt*s1=_tmp3C6;struct Cyc_Absyn_Stmt*s2=_tmp3C7;
# 2678
struct _tuple28 _stmttmp5C=({Cyc_IOEffect_propagate_s(nf,env,s1,s->loc);});enum Cyc_IOEffect_NodeAnnot _tmp3CF;struct Cyc_List_List*_tmp3CE;_LL30: _tmp3CE=_stmttmp5C.f1;_tmp3CF=_stmttmp5C.f2;_LL31: {struct Cyc_List_List*fs1=_tmp3CE;enum Cyc_IOEffect_NodeAnnot jmp1=_tmp3CF;
if((int)jmp1 != (int)0){n->annot=jmp1;goto RETURN;}{
struct _tuple28 _stmttmp5D=({Cyc_IOEffect_propagate_s(fs1,env,s2,s->loc);});enum Cyc_IOEffect_NodeAnnot _tmp3D1;struct Cyc_List_List*_tmp3D0;_LL33: _tmp3D0=_stmttmp5D.f1;_tmp3D1=_stmttmp5D.f2;_LL34: {struct Cyc_List_List*fs2=_tmp3D0;enum Cyc_IOEffect_NodeAnnot jmp2=_tmp3D1;
if((int)jmp2 != (int)0){n->annot=jmp2;goto RETURN;}
({Cyc_IOEffect_checked_set_output_effect(env,n,fs2);});
goto _LL0;}}}}case 4U: _LL11: _tmp3C3=((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmp3A7)->f1;_tmp3C4=((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmp3A7)->f2;_tmp3C5=((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmp3A7)->f3;_LL12: {struct Cyc_Absyn_Exp*e=_tmp3C3;struct Cyc_Absyn_Stmt*s1=_tmp3C4;struct Cyc_Absyn_Stmt*s2=_tmp3C5;
# 2685
({void(*_tmp3D2)(struct Cyc_List_List*nf,struct Cyc_IOEffect_Env*env,struct Cyc_IOEffect_Node*n1,struct Cyc_IOEffect_Node*n2,struct Cyc_IOEffect_Node*n3,unsigned loc,struct Cyc_IOEffect_Node*ifn)=Cyc_IOEffect_propagate_if_n_n_n_n;struct Cyc_List_List*_tmp3D3=nf;struct Cyc_IOEffect_Env*_tmp3D4=env;struct Cyc_IOEffect_Node*_tmp3D5=({Cyc_IOEffect_get_exp(env->fenv,e);});struct Cyc_IOEffect_Node*_tmp3D6=({Cyc_IOEffect_get_stmt(env->fenv,s1);});struct Cyc_IOEffect_Node*_tmp3D7=({Cyc_IOEffect_get_stmt(env->fenv,s2);});unsigned _tmp3D8=s->loc;struct Cyc_IOEffect_Node*_tmp3D9=n;_tmp3D2(_tmp3D3,_tmp3D4,_tmp3D5,_tmp3D6,_tmp3D7,_tmp3D8,_tmp3D9);});
# 2687
goto _LL0;}case 5U: _LL13: _tmp3C0=(((struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct*)_tmp3A7)->f1).f1;_tmp3C1=(((struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct*)_tmp3A7)->f1).f2;_tmp3C2=((struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct*)_tmp3A7)->f2;_LL14: {struct Cyc_Absyn_Exp*e=_tmp3C0;struct Cyc_Absyn_Stmt*skip=_tmp3C1;struct Cyc_Absyn_Stmt*s1=_tmp3C2;
# 2689
({Cyc_IOEffect_propagate_loop_e_e_s_e_s_n(nf,env,0,e,s1,0,skip,s->loc,n);});
goto _LL0;}case 9U: _LL15: _tmp3BB=((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp3A7)->f1;_tmp3BC=(((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp3A7)->f2).f1;_tmp3BD=(((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp3A7)->f3).f1;_tmp3BE=(((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp3A7)->f3).f2;_tmp3BF=((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp3A7)->f4;_LL16: {struct Cyc_Absyn_Exp*e1=_tmp3BB;struct Cyc_Absyn_Exp*e2=_tmp3BC;struct Cyc_Absyn_Exp*e3=_tmp3BD;struct Cyc_Absyn_Stmt*skip=_tmp3BE;struct Cyc_Absyn_Stmt*s1=_tmp3BF;
# 2693
({Cyc_IOEffect_propagate_loop_e_e_s_e_s_n(nf,env,e1,e2,s1,e3,skip,s->loc,n);});
goto _LL0;}case 14U: _LL17: _tmp3B8=((struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct*)_tmp3A7)->f1;_tmp3B9=(((struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct*)_tmp3A7)->f2).f1;_tmp3BA=(((struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct*)_tmp3A7)->f2).f2;_LL18: {struct Cyc_Absyn_Stmt*s1=_tmp3B8;struct Cyc_Absyn_Exp*e=_tmp3B9;struct Cyc_Absyn_Stmt*skip=_tmp3BA;
# 2696
({Cyc_IOEffect_propagate_loop_e_e_s_e_s_n(nf,env,0,0,s1,e,skip,s->loc,n);});
goto _LL0;}case 15U: _LL19: _tmp3B6=((struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct*)_tmp3A7)->f1;_tmp3B7=((struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct*)_tmp3A7)->f2;_LL1A: {struct Cyc_Absyn_Stmt*s0=_tmp3B6;struct Cyc_List_List*scs=_tmp3B7;
# 2699
struct _tuple28 _stmttmp5E=({Cyc_IOEffect_propagate_s(nf,env,s0,s->loc);});enum Cyc_IOEffect_NodeAnnot _tmp3DB;struct Cyc_List_List*_tmp3DA;_LL36: _tmp3DA=_stmttmp5E.f1;_tmp3DB=_stmttmp5E.f2;_LL37: {struct Cyc_List_List*ns0f=_tmp3DA;enum Cyc_IOEffect_NodeAnnot jmp1=_tmp3DB;
nf=ns0f;
# 2704
if(({({int(*_tmp717)(int(*cb)(struct Cyc_IOEffect_Node*,struct Cyc_List_List*e),struct Cyc_IOEffect_Env*env,struct Cyc_List_List*e)=({int(*_tmp3DC)(int(*cb)(struct Cyc_IOEffect_Node*,struct Cyc_List_List*e),struct Cyc_IOEffect_Env*env,struct Cyc_List_List*e)=(int(*)(int(*cb)(struct Cyc_IOEffect_Node*,struct Cyc_List_List*e),struct Cyc_IOEffect_Env*env,struct Cyc_List_List*e))Cyc_IOEffect_iter_jump_list;_tmp3DC;});struct Cyc_IOEffect_Env*_tmp716=env;_tmp717(Cyc_IOEffect_remove_throw,_tmp716,scs);});}))jmp1=0;if((int)jmp1 == (int)0)
({Cyc_IOEffect_checked_set_output_effect(env,n,nf);});{
# 2704
int all_jump=scs != 0;
# 2708
for(0;scs != 0 && scs->tl != 0;scs=scs->tl){
# 2710
struct Cyc_Absyn_Switch_clause*sc=(struct Cyc_Absyn_Switch_clause*)scs->hd;
struct Cyc_Absyn_Exp*wc=sc->where_clause;
# 2719
struct _tuple28 _stmttmp5F=({Cyc_IOEffect_propagate_s_noinput(0,env,sc->body,s0->loc);});enum Cyc_IOEffect_NodeAnnot _tmp3DE;struct Cyc_List_List*_tmp3DD;_LL39: _tmp3DD=_stmttmp5F.f1;_tmp3DE=_stmttmp5F.f2;_LL3A: {struct Cyc_List_List*outf=_tmp3DD;enum Cyc_IOEffect_NodeAnnot jmp=_tmp3DE;
if((int)jmp == (int)0){
# 2722
({Cyc_IOEffect_checked_set_output_effect(env,n,outf);});
all_jump=0;}
# 2720
if(({Cyc_IOEffect_iter_jump_list(Cyc_IOEffect_remove_pattern_jump,env,0);}))
# 2726
all_jump=0;}}
# 2728
if(all_jump == 0)({Cyc_IOEffect_checked_set_output_effect(env,n,nf);});goto _LL0;}}}case 10U: _LL1B: _tmp3B4=((struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct*)_tmp3A7)->f1;_tmp3B5=((struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct*)_tmp3A7)->f2;_LL1C: {struct Cyc_Absyn_Exp*e1=_tmp3B4;struct Cyc_List_List*scs=_tmp3B5;
# 2731
struct _tuple28 _stmttmp60=({Cyc_IOEffect_propagate_e(nf,env,e1,s->loc);});enum Cyc_IOEffect_NodeAnnot _tmp3E0;struct Cyc_List_List*_tmp3DF;_LL3C: _tmp3DF=_stmttmp60.f1;_tmp3E0=_stmttmp60.f2;_LL3D: {struct Cyc_List_List*f=_tmp3DF;enum Cyc_IOEffect_NodeAnnot jmp1=_tmp3E0;
if((int)jmp1 != (int)0){n->annot=jmp1;goto RETURN;}
for(0;scs != 0;scs=scs->tl){
# 2735
struct Cyc_Absyn_Switch_clause*sc=(struct Cyc_Absyn_Switch_clause*)scs->hd;
struct Cyc_Absyn_Exp*wc=sc->where_clause;
struct Cyc_Absyn_Pat*pat=sc->pattern;
({Cyc_IOEffect_check_pat_access(fenv,pat,f,pat->loc);});
# 2752 "ioeffect.cyc"
if(wc != 0){
struct _tuple28 _stmttmp61=({Cyc_IOEffect_propagate_e_e(f,env,wc,wc,e1->loc);});struct Cyc_List_List*_tmp3E1;_LL3F: _tmp3E1=_stmttmp61.f1;_LL40: {struct Cyc_List_List*tmp=_tmp3E1;
f=tmp;}}{
# 2752
struct _tuple28 _stmttmp62=({Cyc_IOEffect_propagate_s(f,env,sc->body,e1->loc);});enum Cyc_IOEffect_NodeAnnot _tmp3E3;struct Cyc_List_List*_tmp3E2;_LL42: _tmp3E2=_stmttmp62.f1;_tmp3E3=_stmttmp62.f2;_LL43: {struct Cyc_List_List*outf=_tmp3E2;enum Cyc_IOEffect_NodeAnnot jmp2=_tmp3E3;
# 2760
{struct Cyc_List_List*iter=env->jlist;for(0;iter != 0;iter=iter->tl){
struct Cyc_IOEffect_Node*_stmttmp63=(struct Cyc_IOEffect_Node*)iter->hd;struct Cyc_IOEffect_Node*_tmp3E4=_stmttmp63;if(((struct Cyc_IOEffect_Stmt_IOEffect_NodeType_struct*)((struct Cyc_IOEffect_Node*)_tmp3E4)->n)->tag == 1U){if(((struct Cyc_Absyn_Break_s_Absyn_Raw_stmt_struct*)((struct Cyc_Absyn_Stmt*)((struct Cyc_IOEffect_Stmt_IOEffect_NodeType_struct*)((struct Cyc_IOEffect_Node*)_tmp3E4)->n)->f1)->r)->tag == 6U){_LL45: _LL46:
# 2763
({void(*_tmp3E5)(struct Cyc_IOEffect_Node*n,struct Cyc_List_List*f)=Cyc_IOEffect_set_output_effect;struct Cyc_IOEffect_Node*_tmp3E6=n;struct Cyc_List_List*_tmp3E7=({Cyc_IOEffect_get_input((struct Cyc_IOEffect_Node*)iter->hd);});_tmp3E5(_tmp3E6,_tmp3E7);});
goto _LL44;}else{goto _LL47;}}else{_LL47: _LL48:
 goto _LL44;}_LL44:;}}
# 2767
({Cyc_IOEffect_iter_jump_list(Cyc_IOEffect_remove_pattern_jump,env,0);});}}}
# 2770
goto _LL0;}}case 12U: switch(*((int*)((struct Cyc_Absyn_Decl*)((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp3A7)->f1)->r)){case 4U: _LL1D: _tmp3B1=((struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*)(((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp3A7)->f1)->r)->f1;_tmp3B2=((struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*)(((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp3A7)->f1)->r)->f3;_tmp3B3=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp3A7)->f2;if(_tmp3B2 != 0){_LL1E: {struct Cyc_Absyn_Tvar*tv=_tmp3B1;struct Cyc_Absyn_Exp*eo=_tmp3B2;struct Cyc_Absyn_Stmt*s1=_tmp3B3;
# 2775
int xrgn=({Cyc_Absyn_is_xrgn_tvar(tv);});
int is_xopen=tv->identity == - 2;
struct _tuple28 _stmttmp64=({Cyc_IOEffect_propagate_e(nf,env,eo,s->loc);});enum Cyc_IOEffect_NodeAnnot _tmp3E9;struct Cyc_List_List*_tmp3E8;_LL4A: _tmp3E8=_stmttmp64.f1;_tmp3E9=_stmttmp64.f2;_LL4B: {struct Cyc_List_List*neof=_tmp3E8;enum Cyc_IOEffect_NodeAnnot jmp1=_tmp3E9;
if((int)jmp1 != (int)0){n->annot=jmp1;goto RETURN;}{
struct Cyc_List_List*eff;
void*rgn=Cyc_Absyn_void_type;
int is_aliased=1;
if(xrgn)
eff=({struct Cyc_List_List*(*_tmp3EA)(void*r,void*typ,struct Cyc_List_List*f,unsigned loc)=Cyc_IOEffect_check_parent_add_child;void*_tmp3EB=(void*)({struct Cyc_Absyn_VarType_Absyn_Type_struct*_tmp3EC=_cycalloc(sizeof(*_tmp3EC));_tmp3EC->tag=2U,_tmp3EC->f1=tv;_tmp3EC;});void*_tmp3ED=({void*(*_tmp3EE)(void*)=Cyc_Tcutil_compress;void*_tmp3EF=({({void*_tmp718=eo->topt;Cyc_IOEffect_safe_cast(_tmp718,({const char*_tmp3F0="ioeffect_stmt";_tag_fat(_tmp3F0,sizeof(char),14U);}));});});_tmp3EE(_tmp3EF);});struct Cyc_List_List*_tmp3F1=neof;unsigned _tmp3F2=s->loc;_tmp3EA(_tmp3EB,_tmp3ED,_tmp3F1,_tmp3F2);});else{
# 2788
if(is_xopen){
void*typ=({({void*_tmp719=eo->topt;Cyc_IOEffect_safe_cast(_tmp719,({const char*_tmp40D="xopen ioeffect";_tag_fat(_tmp40D,sizeof(char),15U);}));});});
# 2792
{void*_tmp3F3=typ;void*_tmp3F4;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp3F3)->tag == 0U){if(((struct Cyc_Absyn_RgnHandleCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp3F3)->f1)->tag == 3U){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp3F3)->f2 != 0){_LL4D: _tmp3F4=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp3F3)->f2)->hd;_LL4E: {void*r=_tmp3F4;
rgn=r;goto _LL4C;}}else{goto _LL4F;}}else{goto _LL4F;}}else{_LL4F: _LL50:
({void*_tmp3F5=0U;({int(*_tmp71B)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp3F7)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp3F7;});struct _fat_ptr _tmp71A=({const char*_tmp3F6="IOEffect::? xopen";_tag_fat(_tmp3F6,sizeof(char),18U);});_tmp71B(_tmp71A,_tag_fat(_tmp3F5,sizeof(void*),0U));});});}_LL4C:;}{
# 2796
int iter;
# 2800
struct Cyc_Absyn_RgnEffect*r=({struct Cyc_Absyn_RgnEffect*(*_tmp405)(struct Cyc_Absyn_RgnEffect*ptr,struct _fat_ptr msg)=({struct Cyc_Absyn_RgnEffect*(*_tmp406)(struct Cyc_Absyn_RgnEffect*ptr,struct _fat_ptr msg)=(struct Cyc_Absyn_RgnEffect*(*)(struct Cyc_Absyn_RgnEffect*ptr,struct _fat_ptr msg))Cyc_IOEffect_safe_cast;_tmp406;});struct Cyc_Absyn_RgnEffect*_tmp407=({struct Cyc_Absyn_RgnEffect*(*_tmp408)(struct Cyc_Absyn_Tvar*x,struct Cyc_List_List*l)=Cyc_Absyn_find_rgneffect;struct Cyc_Absyn_Tvar*_tmp409=({Cyc_Absyn_type2tvar(rgn);});struct Cyc_List_List*_tmp40A=neof;_tmp408(_tmp409,_tmp40A);});struct _fat_ptr _tmp40B=({const char*_tmp40C="ioeffect xopen";_tag_fat(_tmp40C,sizeof(char),15U);});_tmp405(_tmp407,_tmp40B);});
# 2802
struct _tuple12 _stmttmp65=({struct _tuple12(*_tmp403)(struct Cyc_List_List*)=Cyc_Absyn_caps2tup;struct Cyc_List_List*_tmp404=({Cyc_Absyn_rgneffect_caps(r);});_tmp403(_tmp404);});int _tmp3FA;int _tmp3F9;int _tmp3F8;_LL52: _tmp3F8=_stmttmp65.f1;_tmp3F9=_stmttmp65.f2;_tmp3FA=_stmttmp65.f3;_LL53: {int n1=_tmp3F8;int n2=_tmp3F9;int alias=_tmp3FA;
is_aliased=alias;
if(n2 < 1)
({struct Cyc_String_pa_PrintArg_struct _tmp3FD=({struct Cyc_String_pa_PrintArg_struct _tmp5C0;_tmp5C0.tag=0U,({struct _fat_ptr _tmp71C=(struct _fat_ptr)((struct _fat_ptr)({struct _fat_ptr(*_tmp3FE)(void*)=Cyc_Absynpp_typ2string;void*_tmp3FF=({Cyc_Absyn_rgneffect_name(r);});_tmp3FE(_tmp3FF);}));_tmp5C0.f1=_tmp71C;});_tmp5C0;});void*_tmp3FB[1U];_tmp3FB[0]=& _tmp3FD;({unsigned _tmp71E=s->loc;struct _fat_ptr _tmp71D=({const char*_tmp3FC="Region %s has no lock capabilities but `xopen' requires at least one lock capability.";_tag_fat(_tmp3FC,sizeof(char),86U);});Cyc_Tcutil_terr(_tmp71E,_tmp71D,_tag_fat(_tmp3FB,sizeof(void*),1U));});});else{
# 2808
({void(*_tmp400)(struct Cyc_Absyn_RgnEffect*r,struct Cyc_List_List*c)=Cyc_Absyn_updatecaps_rgneffect;struct Cyc_Absyn_RgnEffect*_tmp401=r;struct Cyc_List_List*_tmp402=({Cyc_Absyn_tup2caps(n1,n2 - 1,1);});_tmp400(_tmp401,_tmp402);});}
# 2811
eff=neof;}}}else{
# 2813
eff=neof;}}{
# 2815
struct _tuple28 _stmttmp66=({Cyc_IOEffect_propagate_s(eff,env,s1,eo->loc);});enum Cyc_IOEffect_NodeAnnot _tmp40F;struct Cyc_List_List*_tmp40E;_LL55: _tmp40E=_stmttmp66.f1;_tmp40F=_stmttmp66.f2;_LL56: {struct Cyc_List_List*neof1=_tmp40E;enum Cyc_IOEffect_NodeAnnot jmp2=_tmp40F;
neof=neof1;
if(neof == & Cyc_IOEffect_empty_effect)neof=0;if(
xrgn &&({Cyc_Absyn_find_rgneffect(tv,neof);})!= 0)
# 2820
({void*_tmp410=0U;({unsigned _tmp720=s->loc;struct _fat_ptr _tmp71F=({const char*_tmp411="Region is not freed by the end of its scope.";_tag_fat(_tmp411,sizeof(char),45U);});Cyc_Tcutil_terr(_tmp720,_tmp71F,_tag_fat(_tmp410,sizeof(void*),0U));});});else{
if(is_xopen){
struct Cyc_Absyn_RgnEffect*r=({struct Cyc_Absyn_RgnEffect*(*_tmp41D)(struct Cyc_Absyn_Tvar*x,struct Cyc_List_List*l)=Cyc_Absyn_find_rgneffect;struct Cyc_Absyn_Tvar*_tmp41E=({Cyc_Absyn_type2tvar(rgn);});struct Cyc_List_List*_tmp41F=neof;_tmp41D(_tmp41E,_tmp41F);});
if(r == 0)
({struct Cyc_String_pa_PrintArg_struct _tmp414=({struct Cyc_String_pa_PrintArg_struct _tmp5C1;_tmp5C1.tag=0U,({struct _fat_ptr _tmp721=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(rgn);}));_tmp5C1.f1=_tmp721;});_tmp5C1;});void*_tmp412[1U];_tmp412[0]=& _tmp414;({unsigned _tmp723=s->loc;struct _fat_ptr _tmp722=({const char*_tmp413="Region %s was aliased here (`xopen') but also freed within the scope of `xopen'.";_tag_fat(_tmp413,sizeof(char),81U);});Cyc_Tcutil_terr(_tmp723,_tmp722,_tag_fat(_tmp412,sizeof(void*),1U));});});else{
# 2826
struct _tuple12 _stmttmp67=({struct _tuple12(*_tmp41B)(struct Cyc_List_List*)=Cyc_Absyn_caps2tup;struct Cyc_List_List*_tmp41C=({Cyc_Absyn_rgneffect_caps(r);});_tmp41B(_tmp41C);});int _tmp417;int _tmp416;int _tmp415;_LL58: _tmp415=_stmttmp67.f1;_tmp416=_stmttmp67.f2;_tmp417=_stmttmp67.f3;_LL59: {int n1=_tmp415;int n2=_tmp416;int alias=_tmp417;
({void(*_tmp418)(struct Cyc_Absyn_RgnEffect*r,struct Cyc_List_List*c)=Cyc_Absyn_updatecaps_rgneffect;struct Cyc_Absyn_RgnEffect*_tmp419=r;struct Cyc_List_List*_tmp41A=({Cyc_Absyn_tup2caps(n1,n2 + 1,is_aliased);});_tmp418(_tmp419,_tmp41A);});}}}}
# 2817
({Cyc_IOEffect_checked_set_output_effect(env,n,neof);});
# 2833
if((int)jmp2 != (int)0){n->annot=jmp2;goto RETURN;}
goto _LL0;}}}}}}else{goto _LL25;}case 2U: _LL1F: _tmp3AE=((struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct*)(((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp3A7)->f1)->r)->f1;_tmp3AF=((struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct*)(((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp3A7)->f1)->r)->f3;_tmp3B0=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp3A7)->f2;_LL20: {struct Cyc_Absyn_Pat*pat=_tmp3AE;struct Cyc_Absyn_Exp*e1=_tmp3AF;struct Cyc_Absyn_Stmt*s11=_tmp3B0;
# 2836
struct _tuple28 _stmttmp68=({Cyc_IOEffect_propagate_e(nf,env,e1,s->loc);});enum Cyc_IOEffect_NodeAnnot _tmp421;struct Cyc_List_List*_tmp420;_LL5B: _tmp420=_stmttmp68.f1;_tmp421=_stmttmp68.f2;_LL5C: {struct Cyc_List_List*nf1=_tmp420;enum Cyc_IOEffect_NodeAnnot jmp1=_tmp421;
if((int)jmp1 != (int)0){n->annot=jmp1;goto RETURN;}
({Cyc_IOEffect_check_pat_access(fenv,pat,nf1,pat->loc);});{
# 2840
struct _tuple28 _stmttmp69=({Cyc_IOEffect_propagate_s(nf1,env,s11,s->loc);});enum Cyc_IOEffect_NodeAnnot _tmp423;struct Cyc_List_List*_tmp422;_LL5E: _tmp422=_stmttmp69.f1;_tmp423=_stmttmp69.f2;_LL5F: {struct Cyc_List_List*nf2=_tmp422;enum Cyc_IOEffect_NodeAnnot jmp2=_tmp423;
# 2845
if((int)jmp2 != (int)0){n->annot=jmp2;goto RETURN;}
({Cyc_IOEffect_checked_set_output_effect(env,n,nf2);});
goto _LL0;}}}}case 0U: _LL21: _tmp3AC=((struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)(((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp3A7)->f1)->r)->f1;_tmp3AD=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp3A7)->f2;_LL22: {struct Cyc_Absyn_Vardecl*vdecl=_tmp3AC;struct Cyc_Absyn_Stmt*s11=_tmp3AD;
# 2850
struct Cyc_Absyn_Exp*e0=vdecl->initializer;
# 2856
if(e0 != 0){
# 2858
struct _tuple28 _stmttmp6A=({Cyc_IOEffect_propagate_e(nf,env,e0,s->loc);});enum Cyc_IOEffect_NodeAnnot _tmp425;struct Cyc_List_List*_tmp424;_LL61: _tmp424=_stmttmp6A.f1;_tmp425=_stmttmp6A.f2;_LL62: {struct Cyc_List_List*nf1=_tmp424;enum Cyc_IOEffect_NodeAnnot jmp1=_tmp425;
nf=nf1;
if((int)jmp1 != (int)0){n->annot=jmp1;goto RETURN;}}}{
# 2856
struct _tuple28 _stmttmp6B=({Cyc_IOEffect_propagate_s(nf,env,s11,s->loc);});enum Cyc_IOEffect_NodeAnnot _tmp427;struct Cyc_List_List*_tmp426;_LL64: _tmp426=_stmttmp6B.f1;_tmp427=_stmttmp6B.f2;_LL65: {struct Cyc_List_List*nf2=_tmp426;enum Cyc_IOEffect_NodeAnnot jmp2=_tmp427;
# 2867
nf=nf2;
# 2873
if((int)jmp2 != (int)0){n->annot=jmp2;goto RETURN;}
# 2877
({Cyc_IOEffect_checked_set_output_effect(env,n,nf);});
goto _LL0;}}}case 1U: _LL23: _tmp3AA=((struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*)(((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp3A7)->f1)->r)->f1;_tmp3AB=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp3A7)->f2;_LL24: {struct Cyc_Absyn_Fndecl*fd=_tmp3AA;struct Cyc_Absyn_Stmt*s1=_tmp3AB;
# 2880
({Cyc_IOEffect_analyze_fd(env->tables,fd,s->loc);});
_tmp3A9=s1;goto _LL26;}default: _LL25: _tmp3A9=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp3A7)->f2;_LL26: {struct Cyc_Absyn_Stmt*s1=_tmp3A9;
# 2883
_tmp3A8=s1;goto _LL28;
goto _LL0;}}default: _LL27: _tmp3A8=((struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct*)_tmp3A7)->f2;_LL28: {struct Cyc_Absyn_Stmt*s1=_tmp3A8;
# 2886
struct _tuple28 _stmttmp6C=({Cyc_IOEffect_propagate_s(nf,env,s1,s->loc);});enum Cyc_IOEffect_NodeAnnot _tmp429;struct Cyc_List_List*_tmp428;_LL67: _tmp428=_stmttmp6C.f1;_tmp429=_stmttmp6C.f2;_LL68: {struct Cyc_List_List*ns0f=_tmp428;enum Cyc_IOEffect_NodeAnnot jmp1=_tmp429;
if((int)jmp1 != (int)0){n->annot=jmp1;goto RETURN;}
({Cyc_IOEffect_checked_set_output_effect(env,n,ns0f);});
goto _LL0;}}}_LL0:;}
# 2891
RETURN:
# 2894
({({int(*_tmp725)(int(*cb)(struct Cyc_IOEffect_Node*,struct Cyc_Absyn_Stmt*e),struct Cyc_IOEffect_Env*env,struct Cyc_Absyn_Stmt*e)=({int(*_tmp42A)(int(*cb)(struct Cyc_IOEffect_Node*,struct Cyc_Absyn_Stmt*e),struct Cyc_IOEffect_Env*env,struct Cyc_Absyn_Stmt*e)=(int(*)(int(*cb)(struct Cyc_IOEffect_Node*,struct Cyc_Absyn_Stmt*e),struct Cyc_IOEffect_Env*env,struct Cyc_Absyn_Stmt*e))Cyc_IOEffect_iter_jump_list;_tmp42A;});struct Cyc_IOEffect_Env*_tmp724=env;_tmp725(Cyc_IOEffect_remove_gotos,_tmp724,s);});});}
# 2644 "ioeffect.cyc"
int Cyc_IOEffect_check_contains_xrgn_e(int*b,struct Cyc_Absyn_Exp*e){
# 2899 "ioeffect.cyc"
void*_stmttmp6D=e->r;void*_tmp42C=_stmttmp6D;if(((struct Cyc_Absyn_Spawn_e_Absyn_Raw_exp_struct*)_tmp42C)->tag == 34U){_LL1: _LL2:
*b=1;return 0;}else{_LL3: _LL4:
 return 1;}_LL0:;}
# 2644 "ioeffect.cyc"
int Cyc_IOEffect_check_contains_xrgn_s(int*b,struct Cyc_Absyn_Stmt*s){
# 2907 "ioeffect.cyc"
void*_stmttmp6E=s->r;void*_tmp42E=_stmttmp6E;struct Cyc_Absyn_Exp*_tmp430;struct Cyc_Absyn_Tvar*_tmp42F;if(((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp42E)->tag == 12U){if(((struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*)((struct Cyc_Absyn_Decl*)((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp42E)->f1)->r)->tag == 4U){_LL1: _tmp42F=((struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*)(((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp42E)->f1)->r)->f1;_tmp430=((struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*)(((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp42E)->f1)->r)->f3;if(_tmp430 != 0){_LL2: {struct Cyc_Absyn_Tvar*tv=_tmp42F;struct Cyc_Absyn_Exp*eo=_tmp430;
# 2911
if(({Cyc_Absyn_is_xrgn_tvar(tv);})){
# 2913
*b=1;
return 0;}
# 2911
return 1;}}else{goto _LL3;}}else{goto _LL3;}}else{_LL3: _LL4:
# 2917
 return 1;}_LL0:;}
# 2644 "ioeffect.cyc"
int Cyc_IOEffect_contains_xrgn(struct Cyc_Absyn_Stmt*s){
# 2923 "ioeffect.cyc"
int ret=0;
({({void(*_tmp726)(int(*)(int*,struct Cyc_Absyn_Exp*),int(*)(int*,struct Cyc_Absyn_Stmt*),int*,struct Cyc_Absyn_Stmt*)=({void(*_tmp432)(int(*)(int*,struct Cyc_Absyn_Exp*),int(*)(int*,struct Cyc_Absyn_Stmt*),int*,struct Cyc_Absyn_Stmt*)=(void(*)(int(*)(int*,struct Cyc_Absyn_Exp*),int(*)(int*,struct Cyc_Absyn_Stmt*),int*,struct Cyc_Absyn_Stmt*))Cyc_Absyn_visit_stmt;_tmp432;});_tmp726(Cyc_IOEffect_check_contains_xrgn_e,Cyc_IOEffect_check_contains_xrgn_s,& ret,s);});});
return ret;}
# 2929
static void Cyc_IOEffect_analyze_fd(struct Cyc_JumpAnalysis_Jump_Anal_Result*tables,struct Cyc_Absyn_Fndecl*fd,unsigned loc){
# 2940
struct Cyc_Absyn_Stmt*s=fd->body;
struct Cyc_Absyn_Stmt*skip=({Cyc_Absyn_skip_stmt(s->loc);});
# 2944
struct Cyc_Absyn_Stmt*s0=({struct Cyc_Absyn_Stmt*(*_tmp456)(void*,unsigned)=Cyc_Absyn_new_stmt;void*_tmp457=(void*)({struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*_tmp458=_cycalloc(sizeof(*_tmp458));_tmp458->tag=2U,({struct Cyc_Absyn_Stmt*_tmp727=({Cyc_Absyn_new_stmt(s->r,s->loc);});_tmp458->f1=_tmp727;}),_tmp458->f2=skip;_tmp458;});unsigned _tmp459=s->loc;_tmp456(_tmp457,_tmp459);});
# 2946
struct Cyc_IOEffect_Env*env=({struct Cyc_IOEffect_Env*(*_tmp453)(struct Cyc_JumpAnalysis_Jump_Anal_Result*tables,struct Cyc_IOEffect_Fenv*f)=Cyc_IOEffect_new_env;struct Cyc_JumpAnalysis_Jump_Anal_Result*_tmp454=tables;struct Cyc_IOEffect_Fenv*_tmp455=({Cyc_IOEffect_new_fenv(fd,loc,skip);});_tmp453(_tmp454,_tmp455);});
# 2954
({Cyc_IOEffect_push_throws_scope(env);});
if(({Cyc_Absyn_get_debug();}))
({void*_tmp434=0U;({struct Cyc___cycFILE*_tmp729=Cyc_stderr;struct _fat_ptr _tmp728=({const char*_tmp435="\nIOEffect analysis:";_tag_fat(_tmp435,sizeof(char),20U);});Cyc_fprintf(_tmp729,_tmp728,_tag_fat(_tmp434,sizeof(void*),0U));});});
# 2955
if(({Cyc_Absyn_get_debug();}))
# 2959
({struct Cyc_String_pa_PrintArg_struct _tmp438=({struct Cyc_String_pa_PrintArg_struct _tmp5C4;_tmp5C4.tag=0U,({
struct _fat_ptr _tmp72A=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Position_string_of_loc(loc);}));_tmp5C4.f1=_tmp72A;});_tmp5C4;});struct Cyc_String_pa_PrintArg_struct _tmp439=({struct Cyc_String_pa_PrintArg_struct _tmp5C3;_tmp5C3.tag=0U,_tmp5C3.f1=(struct _fat_ptr)((struct _fat_ptr)*(*fd->name).f2);_tmp5C3;});struct Cyc_String_pa_PrintArg_struct _tmp43A=({struct Cyc_String_pa_PrintArg_struct _tmp5C2;_tmp5C2.tag=0U,({
# 2962
struct _fat_ptr _tmp72B=(struct _fat_ptr)({const char*_tmp43B="";_tag_fat(_tmp43B,sizeof(char),1U);});_tmp5C2.f1=_tmp72B;});_tmp5C2;});void*_tmp436[3U];_tmp436[0]=& _tmp438,_tmp436[1]=& _tmp439,_tmp436[2]=& _tmp43A;({struct Cyc___cycFILE*_tmp72D=Cyc_stderr;struct _fat_ptr _tmp72C=({const char*_tmp437="\nIOEffect: Function declaration (%s) %s : %s";_tag_fat(_tmp437,sizeof(char),45U);});Cyc_fprintf(_tmp72D,_tmp72C,_tag_fat(_tmp436,sizeof(void*),3U));});});{
# 2955
struct Cyc_List_List*ieff=(fd->i).ieffect;
# 2965
struct Cyc_List_List*oeff=(fd->i).oeffect;
if(({Cyc_Absyn_get_debug();})){
# 2968
void*cyc_eff=(fd->i).effect;
if(cyc_eff != 0)
({struct Cyc_String_pa_PrintArg_struct _tmp43E=({struct Cyc_String_pa_PrintArg_struct _tmp5C5;_tmp5C5.tag=0U,({struct _fat_ptr _tmp72E=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(cyc_eff);}));_tmp5C5.f1=_tmp72E;});_tmp5C5;});void*_tmp43C[1U];_tmp43C[0]=& _tmp43E;({struct Cyc___cycFILE*_tmp730=Cyc_stderr;struct _fat_ptr _tmp72F=({const char*_tmp43D="\nTraditional effect: %s";_tag_fat(_tmp43D,sizeof(char),24U);});Cyc_fprintf(_tmp730,_tmp72F,_tag_fat(_tmp43C,sizeof(void*),1U));});});
# 2969
if(ieff != 0)
# 2972
({struct Cyc_String_pa_PrintArg_struct _tmp441=({struct Cyc_String_pa_PrintArg_struct _tmp5C6;_tmp5C6.tag=0U,({struct _fat_ptr _tmp731=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_effect2string(ieff);}));_tmp5C6.f1=_tmp731;});_tmp5C6;});void*_tmp43F[1U];_tmp43F[0]=& _tmp441;({struct Cyc___cycFILE*_tmp733=Cyc_stderr;struct _fat_ptr _tmp732=({const char*_tmp440="\nInput effect: %s";_tag_fat(_tmp440,sizeof(char),18U);});Cyc_fprintf(_tmp733,_tmp732,_tag_fat(_tmp43F,sizeof(void*),1U));});});
# 2969
if(oeff != 0)
# 2974
({struct Cyc_String_pa_PrintArg_struct _tmp444=({struct Cyc_String_pa_PrintArg_struct _tmp5C7;_tmp5C7.tag=0U,({struct _fat_ptr _tmp734=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_effect2string(oeff);}));_tmp5C7.f1=_tmp734;});_tmp5C7;});void*_tmp442[1U];_tmp442[0]=& _tmp444;({struct Cyc___cycFILE*_tmp736=Cyc_stderr;struct _fat_ptr _tmp735=({const char*_tmp443="\nOutput effect: %s";_tag_fat(_tmp443,sizeof(char),19U);});Cyc_fprintf(_tmp736,_tmp735,_tag_fat(_tmp442,sizeof(void*),1U));});});
# 2969
({void*_tmp445=0U;({struct Cyc___cycFILE*_tmp738=Cyc_stderr;struct _fat_ptr _tmp737=({const char*_tmp446="\n";_tag_fat(_tmp446,sizeof(char),2U);});Cyc_fprintf(_tmp738,_tmp737,_tag_fat(_tmp445,sizeof(void*),0U));});});
# 2976
({Cyc_fflush(Cyc_stderr);});}{
# 2966
struct Cyc_IOEffect_Node*ns=({Cyc_IOEffect_get_stmt(env->fenv,s0);});
# 2980
struct Cyc_IOEffect_Node*nskip=({Cyc_IOEffect_get_stmt(env->fenv,skip);});
# 2983
({Cyc_IOEffect_set_input_effect(ns,ieff,s0->loc);});
if(oeff != 0){
# 2986
({Cyc_IOEffect_set_output_effect(ns,oeff);});
({Cyc_IOEffect_set_input_effect(nskip,oeff,s->loc);});
({Cyc_IOEffect_set_output_effect(nskip,oeff);});}else{
# 2992
({Cyc_IOEffect_set_output_effect(ns,& Cyc_IOEffect_empty_effect);});
({Cyc_IOEffect_set_input_effect(nskip,& Cyc_IOEffect_empty_effect,s->loc);});
({Cyc_IOEffect_set_output_effect(nskip,& Cyc_IOEffect_empty_effect);});}
# 2997
if(({Cyc_Absyn_get_debug();}))
({void*_tmp447=0U;({struct Cyc___cycFILE*_tmp73A=Cyc_stderr;struct _fat_ptr _tmp739=({const char*_tmp448="\nIOEffect pass: Exception Analysis";_tag_fat(_tmp448,sizeof(char),35U);});Cyc_fprintf(_tmp73A,_tmp739,_tag_fat(_tmp447,sizeof(void*),0U));});});
# 2997
({Cyc_IOEffect_set_ignore_xrgn(env->fenv,1);});
# 3003
({({void(*_tmp73C)(int(*)(struct Cyc_IOEffect_Env*,struct Cyc_Absyn_Exp*),void(*)(struct Cyc_IOEffect_Env*,struct Cyc_Absyn_Exp*),int(*)(struct Cyc_IOEffect_Env*,struct Cyc_Absyn_Stmt*),void(*f)(struct Cyc_IOEffect_Env*,struct Cyc_Absyn_Stmt*),struct Cyc_IOEffect_Env*,struct Cyc_Absyn_Stmt*)=({void(*_tmp449)(int(*)(struct Cyc_IOEffect_Env*,struct Cyc_Absyn_Exp*),void(*)(struct Cyc_IOEffect_Env*,struct Cyc_Absyn_Exp*),int(*)(struct Cyc_IOEffect_Env*,struct Cyc_Absyn_Stmt*),void(*f)(struct Cyc_IOEffect_Env*,struct Cyc_Absyn_Stmt*),struct Cyc_IOEffect_Env*,struct Cyc_Absyn_Stmt*)=(void(*)(int(*)(struct Cyc_IOEffect_Env*,struct Cyc_Absyn_Exp*),void(*)(struct Cyc_IOEffect_Env*,struct Cyc_Absyn_Exp*),int(*)(struct Cyc_IOEffect_Env*,struct Cyc_Absyn_Stmt*),void(*f)(struct Cyc_IOEffect_Env*,struct Cyc_Absyn_Stmt*),struct Cyc_IOEffect_Env*,struct Cyc_Absyn_Stmt*))Cyc_Absyn_visit_stmt_pop;_tmp449;});struct Cyc_IOEffect_Env*_tmp73B=env;_tmp73C(Cyc_IOEffect_throws_exp_push,Cyc_IOEffect_throws_exp_pop,Cyc_IOEffect_throws_stmt_push,Cyc_IOEffect_throws_stmt_pop,_tmp73B,s0);});});
# 3006
if(({Cyc_Absyn_get_debug();}))
({void*_tmp44A=0U;({struct Cyc___cycFILE*_tmp73E=Cyc_stderr;struct _fat_ptr _tmp73D=({const char*_tmp44B="\nIOEffect pass: Effect Analysis";_tag_fat(_tmp44B,sizeof(char),32U);});Cyc_fprintf(_tmp73E,_tmp73D,_tag_fat(_tmp44A,sizeof(void*),0U));});});
# 3006
if(
# 3010
(fd->i).ieffect != 0 ||({Cyc_IOEffect_contains_xrgn(fd->body);})){
({Cyc_IOEffect_set_ignore_xrgn(env->fenv,0);});
({Cyc_IOEffect_ioeffect_stmt(env,s0);});}
# 3006
if(({Cyc_Absyn_get_debug();}))
# 3016
({void*_tmp44C=0U;({struct Cyc___cycFILE*_tmp740=Cyc_stderr;struct _fat_ptr _tmp73F=({const char*_tmp44D="\nIOEffect: DONE";_tag_fat(_tmp44D,sizeof(char),16U);});Cyc_fprintf(_tmp740,_tmp73F,_tag_fat(_tmp44C,sizeof(void*),0U));});});
# 3006
({void(*_tmp44E)(struct Cyc_IOEffect_Env*te,struct Cyc_IOEffect_Node*n)=Cyc_IOEffect_pop_throws_scope;struct Cyc_IOEffect_Env*_tmp44F=env;struct Cyc_IOEffect_Node*_tmp450=({Cyc_IOEffect_get_stmt(env->fenv,s0);});_tmp44E(_tmp44F,_tmp450);});
# 3023
if(({Cyc_Absyn_get_debug();}))({void*_tmp451=0U;({struct Cyc___cycFILE*_tmp742=Cyc_stderr;struct _fat_ptr _tmp741=({const char*_tmp452="\n";_tag_fat(_tmp452,sizeof(char),2U);});Cyc_fprintf(_tmp742,_tmp741,_tag_fat(_tmp451,sizeof(void*),0U));});});({Cyc_Warn_flush_warnings();});}}}
# 3028
void Cyc_IOEffect_analysis(struct Cyc_JumpAnalysis_Jump_Anal_Result*tables,struct Cyc_List_List*tds){
# 3031
({Cyc_Toc_init_toc_state();});{
struct _fat_ptr lst=_tag_fat(0,0,0);
# 3034
for(0;tds != 0;tds=tds->tl){
void*_stmttmp6F=((struct Cyc_Absyn_Decl*)tds->hd)->r;void*_tmp45B=_stmttmp6F;struct Cyc_Absyn_Fndecl*_tmp45C;struct Cyc_List_List*_tmp45D;struct Cyc_List_List*_tmp45E;switch(*((int*)_tmp45B)){case 2U: _LL1: _LL2:
# 3037
 goto _LL4;case 3U: _LL3: _LL4:
 goto _LL6;case 4U: _LL5: _LL6:
 goto _LL8;case 0U: _LL7: _LL8:
 goto _LLA;case 8U: _LL9: _LLA:
 goto _LLC;case 5U: _LLB: _LLC:
 goto _LLE;case 6U: _LLD: _LLE:
 goto _LL10;case 7U: _LLF: _LL10:
 goto _LL12;case 13U: _LL11: _LL12:
 goto _LL14;case 14U: _LL13: _LL14:
 goto _LL16;case 15U: _LL15: _LL16:
 goto _LL18;case 16U: _LL17: _LL18:
 goto _LL1A;case 11U: _LL19: _LL1A:
 goto _LL1C;case 12U: _LL1B: _LL1C:
 continue;case 9U: _LL1D: _tmp45E=((struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct*)_tmp45B)->f2;_LL1E: {struct Cyc_List_List*tds2=_tmp45E;
_tmp45D=tds2;goto _LL20;}case 10U: _LL1F: _tmp45D=((struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct*)_tmp45B)->f2;_LL20: {struct Cyc_List_List*tds2=_tmp45D;
({Cyc_IOEffect_analysis(tables,tds2);});goto _LL0;}default: _LL21: _tmp45C=((struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*)_tmp45B)->f1;_LL22: {struct Cyc_Absyn_Fndecl*fd=_tmp45C;
# 3054
({Cyc_IOEffect_analyze_fd(tables,fd,((struct Cyc_Absyn_Decl*)tds->hd)->loc);});goto _LL0;}}_LL0:;}}}struct _tuple31{struct Cyc_Absyn_RgnEffect*f1;int f2;};
# 3065
static struct _tuple31 Cyc_IOEffect_find_effect_extended(void*r,struct Cyc_List_List*e){
# 3067
r=({Cyc_Tcutil_compress(r);});
if(({Cyc_IOEffect_is_heaprgn(r);}))return({struct _tuple31 _tmp5C8;_tmp5C8.f1=0,_tmp5C8.f2=1;_tmp5C8;});else{
if(({Cyc_Absyn_is_xrgn(r);})== 0)
# 3071
({int(*_tmp460)(struct _fat_ptr msg)=({int(*_tmp461)(struct _fat_ptr msg)=(int(*)(struct _fat_ptr msg))Cyc_IOEffect_env_err;_tmp461;});struct _fat_ptr _tmp462=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp465=({struct Cyc_String_pa_PrintArg_struct _tmp5CA;_tmp5CA.tag=0U,({
struct _fat_ptr _tmp743=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(r);}));_tmp5CA.f1=_tmp743;});_tmp5CA;});struct Cyc_String_pa_PrintArg_struct _tmp466=({struct Cyc_String_pa_PrintArg_struct _tmp5C9;_tmp5C9.tag=0U,({
struct _fat_ptr _tmp744=(struct _fat_ptr)((struct _fat_ptr)({struct _fat_ptr(*_tmp467)(void*)=Cyc_Absynpp_kindbound2string;void*_tmp468=({Cyc_Absyn_type2tvar(r);})->kind;_tmp467(_tmp468);}));_tmp5C9.f1=_tmp744;});_tmp5C9;});void*_tmp463[2U];_tmp463[0]=& _tmp465,_tmp463[1]=& _tmp466;({struct _fat_ptr _tmp745=({const char*_tmp464="IOEffect::find_effect_extended Not xrgn : %s (kind =%s)";_tag_fat(_tmp464,sizeof(char),56U);});Cyc_aprintf(_tmp745,_tag_fat(_tmp463,sizeof(void*),2U));});});_tmp460(_tmp462);});}{
# 3068
struct Cyc_Absyn_RgnEffect*z=({Cyc_IOEffect_find_effect(r,e);});
# 3076
return z == 0?({struct _tuple31 _tmp5CB;_tmp5CB.f1=0,_tmp5CB.f2=0;_tmp5CB;}):({struct _tuple31 _tmp5CC;_tmp5CC.f1=z,_tmp5CC.f2=1;_tmp5CC;});}}struct _tuple32{int f1;int f2;};
# 3080
static struct _tuple32 Cyc_IOEffect_region_accessible_common(void*r,struct Cyc_List_List*e){
# 3082
struct _tuple31 _stmttmp70=({Cyc_IOEffect_find_effect_extended(r,e);});int _tmp46B;struct Cyc_Absyn_RgnEffect*_tmp46A;_LL1: _tmp46A=_stmttmp70.f1;_tmp46B=_stmttmp70.f2;_LL2: {struct Cyc_Absyn_RgnEffect*z=_tmp46A;int b=_tmp46B;
# 3085
if(!b || z == 0)return({struct _tuple32 _tmp5CD;_tmp5CD.f1=b && z == 0,_tmp5CD.f2=0;_tmp5CD;});{struct _tuple12 _stmttmp71=({struct _tuple12(*_tmp477)(struct Cyc_List_List*)=Cyc_Absyn_caps2tup;struct Cyc_List_List*_tmp478=({Cyc_Absyn_rgneffect_caps(z);});_tmp477(_tmp478);});int _tmp46D;int _tmp46C;_LL4: _tmp46C=_stmttmp71.f1;_tmp46D=_stmttmp71.f2;_LL5: {int n1=_tmp46C;int n2=_tmp46D;
# 3087
if(n1 < 1)({({int(*_tmp746)(struct _fat_ptr msg)=({int(*_tmp46E)(struct _fat_ptr msg)=(int(*)(struct _fat_ptr msg))Cyc_IOEffect_env_err;_tmp46E;});_tmp746(({const char*_tmp46F="IOEffect::region_live_common: impossible";_tag_fat(_tmp46F,sizeof(char),41U);}));});});{void*p=({Cyc_Absyn_rgneffect_parent(z);});
# 3089
if(p == 0)return({struct _tuple32 _tmp5CE;_tmp5CE.f1=1,_tmp5CE.f2=n2 > 0;_tmp5CE;});if(({int(*_tmp470)(void*r1,void*r2)=Cyc_IOEffect_rgn_cmp;void*_tmp471=({Cyc_Absyn_rgneffect_name(z);});void*_tmp472=p;_tmp470(_tmp471,_tmp472);})){
# 3092
({void*_tmp473=0U;({struct _fat_ptr _tmp747=({const char*_tmp474="Found cycle within instantiated effect. If it is a spawn operation try explicit instantiation";_tag_fat(_tmp474,sizeof(char),94U);});Cyc_Tcutil_terr(0U,_tmp747,_tag_fat(_tmp473,sizeof(void*),0U));});});
return({struct _tuple32 _tmp5CF;_tmp5CF.f1=0,_tmp5CF.f2=0;_tmp5CF;});}{
# 3089
struct _tuple32 _stmttmp72=({Cyc_IOEffect_region_accessible_common(p,e);});int _tmp476;int _tmp475;_LL7: _tmp475=_stmttmp72.f1;_tmp476=_stmttmp72.f2;_LL8: {int a=_tmp475;int b=_tmp476;
# 3098
return({struct _tuple32 _tmp5D0;_tmp5D0.f1=a,_tmp5D0.f2=a &&(b || n2 > 0);_tmp5D0;});}}}}}}}
# 3100
static int Cyc_IOEffect_region_live(void*r,struct Cyc_List_List*e){
return({
# 3106
struct _tuple32 _stmttmp73=({Cyc_IOEffect_region_accessible_common(r,e);});int _tmp47A;_LL1: _tmp47A=_stmttmp73.f1;_LL2: ({int a=_tmp47A;
# 3111
a;});});}
# 3113
static int Cyc_IOEffect_region_accessible(void*r,struct Cyc_List_List*e){
return({struct _tuple32 _stmttmp74=({Cyc_IOEffect_region_accessible_common(r,e);});int _tmp47D;int _tmp47C;_LL1: _tmp47C=_stmttmp74.f1;_tmp47D=_stmttmp74.f2;_LL2: ({int a=_tmp47C;int b=_tmp47D;
# 3119
a && b;});});}
# 3113
static struct Cyc_Absyn_RgnEffect*Cyc_IOEffect_join_env_local(unsigned loc,struct Cyc_Absyn_RgnEffect*r1,struct Cyc_Absyn_RgnEffect*r2,struct Cyc_Absyn_RgnEffect*r3);
# 3129
static struct Cyc_Absyn_RgnEffect*Cyc_IOEffect_join_env_spawn(unsigned loc,struct Cyc_Absyn_RgnEffect*r1,struct Cyc_Absyn_RgnEffect*r2,struct Cyc_Absyn_RgnEffect*r3);
# 3136
struct Cyc_List_List*Cyc_IOEffect_summarize_all(unsigned loc,struct Cyc_List_List*f);
# 3140
static struct _tuple8 Cyc_IOEffect_split(unsigned loc,struct Cyc_List_List*dom,struct Cyc_List_List*f,int strict);
# 3145
static struct Cyc_Absyn_Tvar*Cyc_IOEffect_get_tvar(void*t,unsigned loc){
# 3147
void*_tmp47F=t;void*_tmp480;struct Cyc_Absyn_Tvar*_tmp481;switch(*((int*)_tmp47F)){case 2U: _LL1: _tmp481=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp47F)->f1;_LL2: {struct Cyc_Absyn_Tvar*tv=_tmp481;
# 3149
return tv;}case 1U: _LL3: _tmp480=(void*)((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp47F)->f2;_LL4: {void*z=_tmp480;
# 3151
if(z == 0){({struct Cyc_String_pa_PrintArg_struct _tmp484=({struct Cyc_String_pa_PrintArg_struct _tmp5D1;_tmp5D1.tag=0U,({struct _fat_ptr _tmp748=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t);}));_tmp5D1.f1=_tmp748;});_tmp5D1;});void*_tmp482[1U];_tmp482[0]=& _tmp484;({unsigned _tmp74A=loc;struct _fat_ptr _tmp749=({const char*_tmp483="Found a non-unified xregion variable \"%s\" (if it is spawn try using explicit instantiation).";_tag_fat(_tmp483,sizeof(char),93U);});Cyc_Tcutil_terr(_tmp74A,_tmp749,_tag_fat(_tmp482,sizeof(void*),1U));});});
return({struct Cyc_Absyn_Tvar*(*_tmp485)(void*)=Cyc_Tcutil_new_tvar;void*_tmp486=({Cyc_Tcutil_kind_to_bound(& Cyc_Tcutil_xrk);});_tmp485(_tmp486);});}else{
# 3154
return({struct Cyc_Absyn_Tvar*(*_tmp487)(void*t,unsigned loc)=Cyc_IOEffect_get_tvar;void*_tmp488=({({void*_tmp74B=z;Cyc_IOEffect_safe_cast(_tmp74B,({const char*_tmp489="get_tvar";_tag_fat(_tmp489,sizeof(char),9U);}));});});unsigned _tmp48A=loc;_tmp487(_tmp488,_tmp48A);});}}default: _LL5: _LL6:
({int(*_tmp48B)(struct _fat_ptr msg)=({int(*_tmp48C)(struct _fat_ptr msg)=(int(*)(struct _fat_ptr msg))Cyc_IOEffect_env_err;_tmp48C;});struct _fat_ptr _tmp48D=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp490=({struct Cyc_String_pa_PrintArg_struct _tmp5D2;_tmp5D2.tag=0U,({struct _fat_ptr _tmp74C=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t);}));_tmp5D2.f1=_tmp74C;});_tmp5D2;});void*_tmp48E[1U];_tmp48E[0]=& _tmp490;({struct _fat_ptr _tmp74D=({const char*_tmp48F="IOEffect::get_tvar %s";_tag_fat(_tmp48F,sizeof(char),22U);});Cyc_aprintf(_tmp74D,_tag_fat(_tmp48E,sizeof(void*),1U));});});_tmp48B(_tmp48D);});}_LL0:;}
# 3159
static int Cyc_IOEffect_exists_in(struct Cyc_List_List*f,void*t){
# 3161
return({Cyc_IOEffect_is_heaprgn(t);})||({struct Cyc_Absyn_RgnEffect*(*_tmp492)(struct Cyc_Absyn_Tvar*x,struct Cyc_List_List*l)=Cyc_Absyn_find_rgneffect;struct Cyc_Absyn_Tvar*_tmp493=({Cyc_Absyn_type2tvar(t);});struct Cyc_List_List*_tmp494=f;_tmp492(_tmp493,_tmp494);})!= 0;}
# 3165
static int Cyc_IOEffect_not_exists_in(struct Cyc_List_List*f,void*t){
# 3167
return !({Cyc_IOEffect_is_heaprgn(t);})&&({struct Cyc_Absyn_RgnEffect*(*_tmp496)(struct Cyc_Absyn_Tvar*x,struct Cyc_List_List*l)=Cyc_Absyn_find_rgneffect;struct Cyc_Absyn_Tvar*_tmp497=({Cyc_Absyn_type2tvar(t);});struct Cyc_List_List*_tmp498=f;_tmp496(_tmp497,_tmp498);})== 0;}
# 3172
static struct Cyc_List_List*Cyc_IOEffect_abstracted_parents(struct Cyc_List_List*env,struct Cyc_List_List*in){
# 3174
struct Cyc_List_List*ret=0;
void*p;struct Cyc_Absyn_RgnEffect*r;
for(0;in != 0;in=in->tl){
# 3178
p=({Cyc_Absyn_rgneffect_parent((struct Cyc_Absyn_RgnEffect*)in->hd);});
if(p == 0){
# 3181
r=({struct Cyc_Absyn_RgnEffect*(*_tmp49A)(struct Cyc_Absyn_Tvar*x,struct Cyc_List_List*l)=Cyc_Absyn_find_rgneffect;struct Cyc_Absyn_Tvar*_tmp49B=({struct Cyc_Absyn_Tvar*(*_tmp49C)(void*t)=Cyc_Absyn_type2tvar;void*_tmp49D=({Cyc_Absyn_rgneffect_name((struct Cyc_Absyn_RgnEffect*)in->hd);});_tmp49C(_tmp49D);});struct Cyc_List_List*_tmp49E=env;_tmp49A(_tmp49B,_tmp49E);});
# 3183
if(r == 0)continue;{void*p=({Cyc_Absyn_rgneffect_parent(r);});
# 3185
if(p != 0)
ret=({struct Cyc_List_List*_tmp49F=_cycalloc(sizeof(*_tmp49F));_tmp49F->hd=p,_tmp49F->tl=ret;_tmp49F;});}}}
# 3189
return ret;}
# 3193
static int Cyc_IOEffect_all_live(struct Cyc_List_List*f,unsigned loc,struct _fat_ptr errmsg){
# 3195
struct Cyc_List_List*iter=f;
int ret=1;
for(0;iter != 0;iter=iter->tl){
# 3199
if(!({int(*_tmp4A1)(void*r,struct Cyc_List_List*e)=Cyc_IOEffect_region_live;void*_tmp4A2=({Cyc_Absyn_rgneffect_name((struct Cyc_Absyn_RgnEffect*)iter->hd);});struct Cyc_List_List*_tmp4A3=f;_tmp4A1(_tmp4A2,_tmp4A3);})){
# 3201
({struct Cyc_String_pa_PrintArg_struct _tmp4A6=({struct Cyc_String_pa_PrintArg_struct _tmp5D5;_tmp5D5.tag=0U,_tmp5D5.f1=(struct _fat_ptr)((struct _fat_ptr)errmsg);_tmp5D5;});struct Cyc_String_pa_PrintArg_struct _tmp4A7=({struct Cyc_String_pa_PrintArg_struct _tmp5D4;_tmp5D4.tag=0U,({
# 3203
struct _fat_ptr _tmp74E=(struct _fat_ptr)((struct _fat_ptr)({struct _fat_ptr(*_tmp4A9)(void*)=Cyc_Absynpp_typ2string;void*_tmp4AA=({Cyc_Absyn_rgneffect_name((struct Cyc_Absyn_RgnEffect*)iter->hd);});_tmp4A9(_tmp4AA);}));_tmp5D4.f1=_tmp74E;});_tmp5D4;});struct Cyc_String_pa_PrintArg_struct _tmp4A8=({struct Cyc_String_pa_PrintArg_struct _tmp5D3;_tmp5D3.tag=0U,({
struct _fat_ptr _tmp74F=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_effect2string(f);}));_tmp5D3.f1=_tmp74F;});_tmp5D3;});void*_tmp4A4[3U];_tmp4A4[0]=& _tmp4A6,_tmp4A4[1]=& _tmp4A7,_tmp4A4[2]=& _tmp4A8;({unsigned _tmp751=loc;struct _fat_ptr _tmp750=({const char*_tmp4A5="%s => Region %s is not live  in effect %s";_tag_fat(_tmp4A5,sizeof(char),42U);});Cyc_Tcutil_terr(_tmp751,_tmp750,_tag_fat(_tmp4A4,sizeof(void*),3U));});});
ret=0;}}
# 3208
return ret;}
# 3212
static int Cyc_IOEffect_all_live_list(struct Cyc_List_List*t,struct Cyc_List_List*f,unsigned loc,struct _fat_ptr errmsg){
# 3214
struct Cyc_List_List*iter=t;
int ret=1;
for(0;iter != 0;iter=iter->tl){
# 3218
if(!({Cyc_IOEffect_region_live((void*)iter->hd,f);})){
# 3220
({struct Cyc_String_pa_PrintArg_struct _tmp4AE=({struct Cyc_String_pa_PrintArg_struct _tmp5D8;_tmp5D8.tag=0U,_tmp5D8.f1=(struct _fat_ptr)((struct _fat_ptr)errmsg);_tmp5D8;});struct Cyc_String_pa_PrintArg_struct _tmp4AF=({struct Cyc_String_pa_PrintArg_struct _tmp5D7;_tmp5D7.tag=0U,({
# 3222
struct _fat_ptr _tmp752=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string((void*)iter->hd);}));_tmp5D7.f1=_tmp752;});_tmp5D7;});struct Cyc_String_pa_PrintArg_struct _tmp4B0=({struct Cyc_String_pa_PrintArg_struct _tmp5D6;_tmp5D6.tag=0U,({
struct _fat_ptr _tmp753=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_effect2string(f);}));_tmp5D6.f1=_tmp753;});_tmp5D6;});void*_tmp4AC[3U];_tmp4AC[0]=& _tmp4AE,_tmp4AC[1]=& _tmp4AF,_tmp4AC[2]=& _tmp4B0;({unsigned _tmp755=loc;struct _fat_ptr _tmp754=({const char*_tmp4AD="%s => Region %s is not live  in effect %s";_tag_fat(_tmp4AD,sizeof(char),42U);});Cyc_Tcutil_terr(_tmp755,_tmp754,_tag_fat(_tmp4AC,sizeof(void*),3U));});});
ret=0;}}
# 3227
return ret;}
# 3231
static struct Cyc_List_List*Cyc_IOEffect_live(struct Cyc_List_List*in){
# 3233
struct Cyc_List_List*iter=in;
struct Cyc_List_List*ret=0;
# 3239
for(0;iter != 0;iter=iter->tl){
# 3245
if(({int(*_tmp4B2)(void*r,struct Cyc_List_List*e)=Cyc_IOEffect_region_live;void*_tmp4B3=({Cyc_Absyn_rgneffect_name((struct Cyc_Absyn_RgnEffect*)iter->hd);});struct Cyc_List_List*_tmp4B4=in;_tmp4B2(_tmp4B3,_tmp4B4);}))
ret=({struct Cyc_List_List*_tmp4B5=_cycalloc(sizeof(*_tmp4B5));_tmp4B5->hd=(struct Cyc_Absyn_RgnEffect*)iter->hd,_tmp4B5->tl=ret;_tmp4B5;});}
# 3252
return ret;}
# 3259
static struct Cyc_List_List*Cyc_IOEffect_join_env(unsigned loc,struct Cyc_List_List*env,struct Cyc_List_List*in,struct Cyc_List_List*out,int sp){
# 3266
if(env == & Cyc_IOEffect_empty_effect)env=0;if(in == & Cyc_IOEffect_empty_effect)
in=0;
# 3266
if(out == & Cyc_IOEffect_empty_effect)
# 3268
out=0;{
# 3266
struct Cyc_List_List*sin=({Cyc_IOEffect_summarize_all(loc,in);});
# 3270
if(!({({struct Cyc_List_List*_tmp757=sin;unsigned _tmp756=loc;Cyc_IOEffect_all_live(_tmp757,_tmp756,({const char*_tmp4B7="Input effect";_tag_fat(_tmp4B7,sizeof(char),13U);}));});}))return 0;{struct Cyc_List_List*sout=({Cyc_IOEffect_summarize_all(loc,out);});
# 3283 "ioeffect.cyc"
if(!({({struct Cyc_List_List*_tmp759=sout;unsigned _tmp758=loc;Cyc_IOEffect_all_live(_tmp759,_tmp758,({const char*_tmp4B8="Output effect";_tag_fat(_tmp4B8,sizeof(char),14U);}));});}))return 0;{struct _tuple8 _stmttmp75=({Cyc_IOEffect_split(loc,sin,env,1);});struct Cyc_List_List*_tmp4BA;struct Cyc_List_List*_tmp4B9;_LL1: _tmp4B9=_stmttmp75.f1;_tmp4BA=_stmttmp75.f2;_LL2: {struct Cyc_List_List*in1=_tmp4B9;struct Cyc_List_List*kept_by_env=_tmp4BA;
# 3285
struct Cyc_Absyn_RgnEffect*(*fun)(unsigned loc,struct Cyc_Absyn_RgnEffect*r1,struct Cyc_Absyn_RgnEffect*r2,struct Cyc_Absyn_RgnEffect*r3)=sp?Cyc_IOEffect_join_env_spawn: Cyc_IOEffect_join_env_local;
struct Cyc_List_List*outenv=0;
struct Cyc_List_List*abs_par=({Cyc_IOEffect_abstracted_parents(env,sin);});
# 3289
for(0;sin != 0;sin=sin->tl){
# 3291
struct Cyc_Absyn_Tvar*tv=({Cyc_IOEffect_get_tvar(((struct Cyc_Absyn_RgnEffect*)sin->hd)->name,loc);});
struct Cyc_Absyn_RgnEffect*r_out=({struct Cyc_Absyn_RgnEffect*(*_tmp4BE)(unsigned loc,struct Cyc_Absyn_RgnEffect*r1,struct Cyc_Absyn_RgnEffect*r2,struct Cyc_Absyn_RgnEffect*r3)=fun;unsigned _tmp4BF=loc;struct Cyc_Absyn_RgnEffect*_tmp4C0=(struct Cyc_Absyn_RgnEffect*)sin->hd;struct Cyc_Absyn_RgnEffect*_tmp4C1=({Cyc_Absyn_find_rgneffect(tv,sout);});struct Cyc_Absyn_RgnEffect*_tmp4C2=({struct Cyc_Absyn_RgnEffect*(*_tmp4C3)(struct Cyc_Absyn_RgnEffect*ptr,struct _fat_ptr msg)=({
# 3294
struct Cyc_Absyn_RgnEffect*(*_tmp4C4)(struct Cyc_Absyn_RgnEffect*ptr,struct _fat_ptr msg)=(struct Cyc_Absyn_RgnEffect*(*)(struct Cyc_Absyn_RgnEffect*ptr,struct _fat_ptr msg))Cyc_IOEffect_safe_cast;_tmp4C4;});struct Cyc_Absyn_RgnEffect*_tmp4C5=({Cyc_Absyn_find_rgneffect(tv,in1);});struct _fat_ptr _tmp4C6=({const char*_tmp4C7="join_env";_tag_fat(_tmp4C7,sizeof(char),9U);});_tmp4C3(_tmp4C5,_tmp4C6);});_tmp4BE(_tmp4BF,_tmp4C0,_tmp4C1,_tmp4C2);});
# 3296
if(r_out == 0)continue;outenv=({struct Cyc_List_List*_tmp4BD=_cycalloc(sizeof(*_tmp4BD));
({struct Cyc_Absyn_RgnEffect*_tmp75C=({({struct Cyc_Absyn_RgnEffect*(*_tmp75B)(struct Cyc_Absyn_RgnEffect*ptr,struct _fat_ptr msg)=({struct Cyc_Absyn_RgnEffect*(*_tmp4BB)(struct Cyc_Absyn_RgnEffect*ptr,struct _fat_ptr msg)=(struct Cyc_Absyn_RgnEffect*(*)(struct Cyc_Absyn_RgnEffect*ptr,struct _fat_ptr msg))Cyc_IOEffect_safe_cast;_tmp4BB;});struct Cyc_Absyn_RgnEffect*_tmp75A=r_out;_tmp75B(_tmp75A,({const char*_tmp4BC="join_env";_tag_fat(_tmp4BC,sizeof(char),9U);}));});});_tmp4BD->hd=_tmp75C;}),_tmp4BD->tl=outenv;_tmp4BD;});}
# 3299
outenv=({struct Cyc_List_List*(*_tmp4C8)(struct Cyc_List_List*in)=Cyc_IOEffect_live;struct Cyc_List_List*_tmp4C9=({Cyc_List_append(outenv,kept_by_env);});_tmp4C8(_tmp4C9);});
if(!({({struct Cyc_List_List*_tmp75F=abs_par;struct Cyc_List_List*_tmp75E=outenv;unsigned _tmp75D=loc;Cyc_IOEffect_all_live_list(_tmp75F,_tmp75E,_tmp75D,({const char*_tmp4CA="Abstracted parent not live";_tag_fat(_tmp4CA,sizeof(char),27U);}));});}))
return 0;
# 3300
return outenv;}}}}}
# 3307
static struct _tuple8 Cyc_IOEffect_split(unsigned loc,struct Cyc_List_List*dom,struct Cyc_List_List*f,int strict){
# 3311
int len;int len1;
if(dom == 0)return({struct _tuple8 _tmp5D9;_tmp5D9.f1=0,({struct Cyc_List_List*_tmp760=({Cyc_Absyn_copy_effect(f);});_tmp5D9.f2=_tmp760;});_tmp5D9;});else{
if(({int _tmp761=len=({Cyc_List_length(dom);});_tmp761 > ({Cyc_List_length(f);});})){
# 3315
({struct Cyc_String_pa_PrintArg_struct _tmp4CE=({struct Cyc_String_pa_PrintArg_struct _tmp5DB;_tmp5DB.tag=0U,({
struct _fat_ptr _tmp762=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_effect2string(dom);}));_tmp5DB.f1=_tmp762;});_tmp5DB;});struct Cyc_String_pa_PrintArg_struct _tmp4CF=({struct Cyc_String_pa_PrintArg_struct _tmp5DA;_tmp5DA.tag=0U,({struct _fat_ptr _tmp763=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_effect2string(f);}));_tmp5DA.f1=_tmp763;});_tmp5DA;});void*_tmp4CC[2U];_tmp4CC[0]=& _tmp4CE,_tmp4CC[1]=& _tmp4CF;({unsigned _tmp765=loc;struct _fat_ptr _tmp764=({const char*_tmp4CD="Function accepts more regions than the regions available in the calling context. Function = %s , Calling context = %s";_tag_fat(_tmp4CD,sizeof(char),118U);});Cyc_Tcutil_terr(_tmp765,_tmp764,_tag_fat(_tmp4CC,sizeof(void*),2U));});});
# 3318
return({struct _tuple8 _tmp5DC;_tmp5DC.f1=0,_tmp5DC.f2=0;_tmp5DC;});}}{
# 3312
struct Cyc_List_List*first=0;struct Cyc_List_List*second=0;
# 3322
struct Cyc_Absyn_RgnEffect*r=0;
struct Cyc_Absyn_RgnEffect*cp=0;
struct Cyc_List_List*ftmp=f;
for(len1=0;f != 0;f=f->tl){
# 3327
struct Cyc_Absyn_Tvar*tv=({Cyc_IOEffect_get_tvar(((struct Cyc_Absyn_RgnEffect*)f->hd)->name,loc);});
r=({Cyc_Absyn_find_rgneffect(tv,dom);});
# 3332
cp=({Cyc_IOEffect_copyeffelt((struct Cyc_Absyn_RgnEffect*)f->hd);});
if(r != 0){
first=({struct Cyc_List_List*_tmp4D0=_cycalloc(sizeof(*_tmp4D0));_tmp4D0->hd=cp,_tmp4D0->tl=first;_tmp4D0;});++ len1;}else{
second=({struct Cyc_List_List*_tmp4D1=_cycalloc(sizeof(*_tmp4D1));_tmp4D1->hd=cp,_tmp4D1->tl=second;_tmp4D1;});}}
# 3338
if(strict && len != len1){
# 3340
({struct Cyc_String_pa_PrintArg_struct _tmp4D4=({struct Cyc_String_pa_PrintArg_struct _tmp5DE;_tmp5DE.tag=0U,({
# 3342
struct _fat_ptr _tmp766=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_effect2string(dom);}));_tmp5DE.f1=_tmp766;});_tmp5DE;});struct Cyc_String_pa_PrintArg_struct _tmp4D5=({struct Cyc_String_pa_PrintArg_struct _tmp5DD;_tmp5DD.tag=0U,({struct _fat_ptr _tmp767=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_effect2string(ftmp);}));_tmp5DD.f1=_tmp767;});_tmp5DD;});void*_tmp4D2[2U];_tmp4D2[0]=& _tmp4D4,_tmp4D2[1]=& _tmp4D5;({unsigned _tmp769=loc;struct _fat_ptr _tmp768=({const char*_tmp4D3="Function accepts more regions than the regions available in the calling context. Function = %s , Calling context = %s";_tag_fat(_tmp4D3,sizeof(char),118U);});Cyc_Tcutil_terr(_tmp769,_tmp768,_tag_fat(_tmp4D2,sizeof(void*),2U));});});
# 3344
return({struct _tuple8 _tmp5DF;_tmp5DF.f1=0,_tmp5DF.f2=0;_tmp5DF;});}
# 3338
return({struct _tuple8 _tmp5E0;_tmp5E0.f1=first,_tmp5E0.f2=second;_tmp5E0;});}}
# 3350
static void Cyc_IOEffect_check_positive(unsigned loc,void*c){
# 3352
if(({Cyc_Absyn_is_nat_cap(c);})&&({Cyc_Absyn_get_nat_cap(c);})< 1)
({void*_tmp4D7=0U;({unsigned _tmp76B=loc;struct _fat_ptr _tmp76A=({const char*_tmp4D8="Found a non-positive capability. Only positive capabilities can be passed.";_tag_fat(_tmp4D8,sizeof(char),75U);});Cyc_Tcutil_terr(_tmp76B,_tmp76A,_tag_fat(_tmp4D7,sizeof(void*),0U));});});}
# 3357
static struct Cyc_Absyn_RgnEffect*Cyc_IOEffect_join_rgneffect(unsigned loc,struct Cyc_Absyn_RgnEffect*r1,struct Cyc_Absyn_RgnEffect*r2){
# 3363
struct _tuple12 _stmttmp76=({struct _tuple12(*_tmp4EA)(struct Cyc_List_List*)=Cyc_Absyn_caps2tup;struct Cyc_List_List*_tmp4EB=({Cyc_Absyn_rgneffect_caps(r1);});_tmp4EA(_tmp4EB);});int _tmp4DC;int _tmp4DB;int _tmp4DA;_LL1: _tmp4DA=_stmttmp76.f1;_tmp4DB=_stmttmp76.f2;_tmp4DC=_stmttmp76.f3;_LL2: {int rg_r1=_tmp4DA;int lk_r1=_tmp4DB;int alias_r1=_tmp4DC;
struct _tuple12 _stmttmp77=({struct _tuple12(*_tmp4E8)(struct Cyc_List_List*)=Cyc_Absyn_caps2tup;struct Cyc_List_List*_tmp4E9=({Cyc_Absyn_rgneffect_caps(r2);});_tmp4E8(_tmp4E9);});int _tmp4DF;int _tmp4DE;int _tmp4DD;_LL4: _tmp4DD=_stmttmp77.f1;_tmp4DE=_stmttmp77.f2;_tmp4DF=_stmttmp77.f3;_LL5: {int rg_r2=_tmp4DD;int lk_r2=_tmp4DE;int alias_r2=_tmp4DF;
# 3366
if(!alias_r1 || !alias_r2){
# 3368
({struct Cyc_String_pa_PrintArg_struct _tmp4E2=({struct Cyc_String_pa_PrintArg_struct _tmp5E2;_tmp5E2.tag=0U,({
struct _fat_ptr _tmp76C=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_rgneffect2string(r1);}));_tmp5E2.f1=_tmp76C;});_tmp5E2;});struct Cyc_String_pa_PrintArg_struct _tmp4E3=({struct Cyc_String_pa_PrintArg_struct _tmp5E1;_tmp5E1.tag=0U,({
struct _fat_ptr _tmp76D=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_rgneffect2string(r2);}));_tmp5E1.f1=_tmp76D;});_tmp5E1;});void*_tmp4E0[2U];_tmp4E0[0]=& _tmp4E2,_tmp4E0[1]=& _tmp4E3;({unsigned _tmp76F=loc;struct _fat_ptr _tmp76E=({const char*_tmp4E1="Cannot join capabilties of region effects %s and %s. Both need to be *impure*";_tag_fat(_tmp4E1,sizeof(char),78U);});Cyc_Tcutil_terr(_tmp76F,_tmp76E,_tag_fat(_tmp4E0,sizeof(void*),2U));});});
return r1;}
# 3366
return({struct Cyc_Absyn_RgnEffect*(*_tmp4E4)(void*t1,struct Cyc_List_List*c,void*t2)=Cyc_Absyn_new_rgneffect;void*_tmp4E5=({Cyc_Absyn_rgneffect_name(r1);});struct Cyc_List_List*_tmp4E6=({Cyc_Absyn_tup2caps(rg_r1 + rg_r2,lk_r1 + lk_r2,1);});void*_tmp4E7=({Cyc_Absyn_rgneffect_parent(r1);});_tmp4E4(_tmp4E5,_tmp4E6,_tmp4E7);});}}}
# 3379
static int Cyc_IOEffect_not_aliasable(struct Cyc_Absyn_RgnEffect*r){
# 3381
return !({Cyc_Absyn_is_aliasable_rgneffect(r);});}
# 3386
static struct Cyc_Absyn_RgnEffect*Cyc_IOEffect_summarize(unsigned loc,struct Cyc_List_List*f){
# 3388
if(f == 0)({({int(*_tmp770)(struct _fat_ptr msg)=({int(*_tmp4EE)(struct _fat_ptr msg)=(int(*)(struct _fat_ptr msg))Cyc_IOEffect_env_err;_tmp4EE;});_tmp770(({const char*_tmp4EF="IOEffect::summarize";_tag_fat(_tmp4EF,sizeof(char),20U);}));});});{struct Cyc_Absyn_RgnEffect*ret=({Cyc_IOEffect_copyeffelt((struct Cyc_Absyn_RgnEffect*)f->hd);});
# 3390
for(f=f->tl;f != 0;f=f->tl){
ret=({Cyc_IOEffect_join_rgneffect(loc,ret,(struct Cyc_Absyn_RgnEffect*)f->hd);});}
return ret;}}
# 3386
struct Cyc_List_List*Cyc_IOEffect_summarize_all(unsigned loc,struct Cyc_List_List*f){
# 3398
struct Cyc_List_List*ret=0;
while(f != 0){
# 3401
struct Cyc_List_List dummy=({struct Cyc_List_List _tmp5E3;_tmp5E3.hd=(struct Cyc_Absyn_RgnEffect*)f->hd,_tmp5E3.tl=0;_tmp5E3;});
# 3404
struct _tuple8 _stmttmp78=({Cyc_IOEffect_split(loc,& dummy,f,0);});struct Cyc_List_List*_tmp4F2;struct Cyc_List_List*_tmp4F1;_LL1: _tmp4F1=_stmttmp78.f1;_tmp4F2=_stmttmp78.f2;_LL2: {struct Cyc_List_List*dom=_tmp4F1;struct Cyc_List_List*rest=_tmp4F2;
struct Cyc_Absyn_RgnEffect*r=({Cyc_IOEffect_summarize(loc,dom);});
# 3412
ret=({struct Cyc_List_List*_tmp4F3=_cycalloc(sizeof(*_tmp4F3));_tmp4F3->hd=r,_tmp4F3->tl=ret;_tmp4F3;});
f=rest;}}
# 3416
return ret;}
# 3386
static struct Cyc_List_List*Cyc_IOEffect_output_caps(unsigned loc,struct Cyc_Absyn_RgnEffect*r1,struct Cyc_Absyn_RgnEffect*r2,struct Cyc_Absyn_RgnEffect*r3);
# 3428
static struct Cyc_Absyn_RgnEffect*Cyc_IOEffect_guess_out_cap(unsigned loc,struct Cyc_Absyn_RgnEffect*r1,struct Cyc_Absyn_RgnEffect*r2){
# 3434
if(r2 != 0)return r2;return({struct Cyc_Absyn_RgnEffect*(*_tmp4F5)(void*t1,struct Cyc_List_List*c,void*t2)=Cyc_Absyn_new_rgneffect;void*_tmp4F6=({Cyc_Absyn_rgneffect_name(r1);});struct Cyc_List_List*_tmp4F7=({struct Cyc_List_List*(*_tmp4F8)(int a,int b,int c)=Cyc_Absyn_tup2caps;int _tmp4F9=0;int _tmp4FA=0;int _tmp4FB=({Cyc_Absyn_is_aliasable_rgneffect(r1);});_tmp4F8(_tmp4F9,_tmp4FA,_tmp4FB);});void*_tmp4FC=({Cyc_Absyn_rgneffect_parent(r1);});_tmp4F5(_tmp4F6,_tmp4F7,_tmp4FC);});}
# 3444
static struct Cyc_Absyn_RgnEffect*Cyc_IOEffect_join_env_local(unsigned loc,struct Cyc_Absyn_RgnEffect*r1,struct Cyc_Absyn_RgnEffect*r2,struct Cyc_Absyn_RgnEffect*env){
# 3451
void*r1p=({Cyc_Absyn_rgneffect_parent(r1);});
void*envp=({Cyc_Absyn_rgneffect_parent(env);});
# 3455
if(r1p != 0 &&(envp == 0 || !({Cyc_IOEffect_rgn_cmp(r1p,envp);}))){
# 3457
if(({Cyc_Absyn_get_debug();}))
({struct Cyc_String_pa_PrintArg_struct _tmp500=({struct Cyc_String_pa_PrintArg_struct _tmp5EA;_tmp5EA.tag=0U,({
struct _fat_ptr _tmp771=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_rgneffect2string(env);}));_tmp5EA.f1=_tmp771;});_tmp5EA;});struct Cyc_String_pa_PrintArg_struct _tmp501=({struct Cyc_String_pa_PrintArg_struct _tmp5E9;_tmp5E9.tag=0U,({struct _fat_ptr _tmp772=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_rgneffect2string(r1);}));_tmp5E9.f1=_tmp772;});_tmp5E9;});struct Cyc_Int_pa_PrintArg_struct _tmp502=({struct Cyc_Int_pa_PrintArg_struct _tmp5E8;_tmp5E8.tag=1U,_tmp5E8.f1=(unsigned long)(r1p != 0);_tmp5E8;});struct Cyc_Int_pa_PrintArg_struct _tmp503=({struct Cyc_Int_pa_PrintArg_struct _tmp5E7;_tmp5E7.tag=1U,_tmp5E7.f1=(unsigned long)(envp == 0);_tmp5E7;});struct Cyc_Int_pa_PrintArg_struct _tmp504=({struct Cyc_Int_pa_PrintArg_struct _tmp5E6;_tmp5E6.tag=1U,({unsigned long _tmp773=(unsigned long)!({Cyc_IOEffect_rgn_cmp(r1p,(void*)_check_null(envp));});_tmp5E6.f1=_tmp773;});_tmp5E6;});struct Cyc_String_pa_PrintArg_struct _tmp505=({struct Cyc_String_pa_PrintArg_struct _tmp5E5;_tmp5E5.tag=0U,({struct _fat_ptr _tmp774=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(r1p);}));_tmp5E5.f1=_tmp774;});_tmp5E5;});struct Cyc_String_pa_PrintArg_struct _tmp506=({struct Cyc_String_pa_PrintArg_struct _tmp5E4;_tmp5E4.tag=0U,({struct _fat_ptr _tmp775=(struct _fat_ptr)((struct _fat_ptr)(envp != 0?({Cyc_Absynpp_typ2string(envp);}):(struct _fat_ptr)({const char*_tmp507="<null>";_tag_fat(_tmp507,sizeof(char),7U);})));_tmp5E4.f1=_tmp775;});_tmp5E4;});void*_tmp4FE[7U];_tmp4FE[0]=& _tmp500,_tmp4FE[1]=& _tmp501,_tmp4FE[2]=& _tmp502,_tmp4FE[3]=& _tmp503,_tmp4FE[4]=& _tmp504,_tmp4FE[5]=& _tmp505,_tmp4FE[6]=& _tmp506;({unsigned _tmp777=loc;struct _fat_ptr _tmp776=({const char*_tmp4FF="Cannot convert %s to %s (rp1!=NULL = %d envp == NULL = %d rgn_cmp(r1p,envp) = %d r1p= %s envp = %s).";_tag_fat(_tmp4FF,sizeof(char),101U);});Cyc_Tcutil_terr(_tmp777,_tmp776,_tag_fat(_tmp4FE,sizeof(void*),7U));});});else{
# 3461
({struct Cyc_String_pa_PrintArg_struct _tmp50A=({struct Cyc_String_pa_PrintArg_struct _tmp5EC;_tmp5EC.tag=0U,({
struct _fat_ptr _tmp778=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_rgneffect2string(env);}));_tmp5EC.f1=_tmp778;});_tmp5EC;});struct Cyc_String_pa_PrintArg_struct _tmp50B=({struct Cyc_String_pa_PrintArg_struct _tmp5EB;_tmp5EB.tag=0U,({struct _fat_ptr _tmp779=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_rgneffect2string(r1);}));_tmp5EB.f1=_tmp779;});_tmp5EB;});void*_tmp508[2U];_tmp508[0]=& _tmp50A,_tmp508[1]=& _tmp50B;({unsigned _tmp77B=loc;struct _fat_ptr _tmp77A=({const char*_tmp509="Cannot convert %s to %s.";_tag_fat(_tmp509,sizeof(char),25U);});Cyc_Tcutil_terr(_tmp77B,_tmp77A,_tag_fat(_tmp508,sizeof(void*),2U));});});}
return 0;}
# 3455
({void(*_tmp50C)(unsigned loc,void*c)=Cyc_IOEffect_check_positive;unsigned _tmp50D=loc;void*_tmp50E=({void*(*_tmp50F)(struct Cyc_List_List*c)=Cyc_Absyn_rgn_cap;struct Cyc_List_List*_tmp510=({Cyc_Absyn_rgneffect_caps(r1);});_tmp50F(_tmp510);});_tmp50C(_tmp50D,_tmp50E);});{
# 3475 "ioeffect.cyc"
struct Cyc_List_List*caps=({struct Cyc_List_List*(*_tmp517)(unsigned loc,struct Cyc_Absyn_RgnEffect*r1,struct Cyc_Absyn_RgnEffect*r2,struct Cyc_Absyn_RgnEffect*r3)=Cyc_IOEffect_output_caps;unsigned _tmp518=loc;struct Cyc_Absyn_RgnEffect*_tmp519=r1;struct Cyc_Absyn_RgnEffect*_tmp51A=({Cyc_IOEffect_guess_out_cap(loc,r1,r2);});struct Cyc_Absyn_RgnEffect*_tmp51B=env;_tmp517(_tmp518,_tmp519,_tmp51A,_tmp51B);});
if(({int(*_tmp511)(void*c)=Cyc_Absyn_get_nat_cap;void*_tmp512=({Cyc_Absyn_rgn_cap(caps);});_tmp511(_tmp512);})== 0)return 0;return({struct Cyc_Absyn_RgnEffect*(*_tmp513)(void*t1,struct Cyc_List_List*c,void*t2)=Cyc_Absyn_new_rgneffect;void*_tmp514=({Cyc_Absyn_rgneffect_name(r1);});struct Cyc_List_List*_tmp515=caps;void*_tmp516=envp;_tmp513(_tmp514,_tmp515,_tmp516);});}}
# 3480
static struct Cyc_Absyn_RgnEffect*Cyc_IOEffect_join_env_spawn(unsigned loc,struct Cyc_Absyn_RgnEffect*r1,struct Cyc_Absyn_RgnEffect*r2,struct Cyc_Absyn_RgnEffect*r3){
# 3487
if(r2 != 0){
# 3489
({struct Cyc_String_pa_PrintArg_struct _tmp51F=({struct Cyc_String_pa_PrintArg_struct _tmp5EE;_tmp5EE.tag=0U,({
struct _fat_ptr _tmp77C=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_rgneffect2string(r1);}));_tmp5EE.f1=_tmp77C;});_tmp5EE;});struct Cyc_String_pa_PrintArg_struct _tmp520=({struct Cyc_String_pa_PrintArg_struct _tmp5ED;_tmp5ED.tag=0U,({struct _fat_ptr _tmp77D=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_rgneffect2string(r2);}));_tmp5ED.f1=_tmp77D;});_tmp5ED;});void*_tmp51D[2U];_tmp51D[0]=& _tmp51F,_tmp51D[1]=& _tmp520;({unsigned _tmp77F=loc;struct _fat_ptr _tmp77E=({const char*_tmp51E="Spawn should consume region effect %s but it appears on the output effect %s";_tag_fat(_tmp51E,sizeof(char),77U);});Cyc_Tcutil_terr(_tmp77F,_tmp77E,_tag_fat(_tmp51D,sizeof(void*),2U));});});
return 0;}{
# 3487
struct _tuple12 _stmttmp79=({struct _tuple12(*_tmp532)(struct Cyc_List_List*)=Cyc_Absyn_caps2tup;struct Cyc_List_List*_tmp533=({Cyc_Absyn_rgneffect_caps(r1);});_tmp532(_tmp533);});int _tmp523;int _tmp522;int _tmp521;_LL1: _tmp521=_stmttmp79.f1;_tmp522=_stmttmp79.f2;_tmp523=_stmttmp79.f3;_LL2: {int rg=_tmp521;int lk=_tmp522;int aliased=_tmp523;
# 3494
if(aliased && lk > 0){
# 3496
({struct Cyc_String_pa_PrintArg_struct _tmp526=({struct Cyc_String_pa_PrintArg_struct _tmp5EF;_tmp5EF.tag=0U,({
struct _fat_ptr _tmp780=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_rgneffect2string(r1);}));_tmp5EF.f1=_tmp780;});_tmp5EF;});void*_tmp524[1U];_tmp524[0]=& _tmp526;({unsigned _tmp782=loc;struct _fat_ptr _tmp781=({const char*_tmp525="Invalid lock capability in region input effect  %s: Cannot pass a positive impure lock to spawn";_tag_fat(_tmp525,sizeof(char),96U);});Cyc_Tcutil_terr(_tmp782,_tmp781,_tag_fat(_tmp524,sizeof(void*),1U));});});
return 0;}
# 3494
if(
# 3500
({Cyc_Absyn_rgneffect_parent(r1);})== 0 ||({Cyc_Absyn_rgneffect_parent(r3);})== 0){
# 3504
({struct Cyc_String_pa_PrintArg_struct _tmp529=({struct Cyc_String_pa_PrintArg_struct _tmp5F1;_tmp5F1.tag=0U,({
struct _fat_ptr _tmp783=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_rgneffect2string(r1);}));_tmp5F1.f1=_tmp783;});_tmp5F1;});struct Cyc_String_pa_PrintArg_struct _tmp52A=({struct Cyc_String_pa_PrintArg_struct _tmp5F0;_tmp5F0.tag=0U,({struct _fat_ptr _tmp784=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_rgneffect2string(r3);}));_tmp5F0.f1=_tmp784;});_tmp5F0;});void*_tmp527[2U];_tmp527[0]=& _tmp529,_tmp527[1]=& _tmp52A;({unsigned _tmp786=loc;struct _fat_ptr _tmp785=({const char*_tmp528="Cannot pass to spawn effects having abstracted parents (thread=%s) (enviroment=%s)";_tag_fat(_tmp528,sizeof(char),83U);});Cyc_Tcutil_terr(_tmp786,_tmp785,_tag_fat(_tmp527,sizeof(void*),2U));});});
return 0;}else{
# 3524 "ioeffect.cyc"
struct Cyc_Absyn_RgnEffect*ret=({Cyc_IOEffect_join_env_local(loc,r1,r2,r3);});
if((!aliased && lk > 0)&& ret != 0){
struct _tuple12 _stmttmp7A=({struct _tuple12(*_tmp530)(struct Cyc_List_List*)=Cyc_Absyn_caps2tup;struct Cyc_List_List*_tmp531=({Cyc_Absyn_rgneffect_caps(ret);});_tmp530(_tmp531);});int _tmp52B;_LL4: _tmp52B=_stmttmp7A.f2;_LL5: {int lk_out=_tmp52B;
if(lk_out == 0)return ret;({struct Cyc_String_pa_PrintArg_struct _tmp52E=({struct Cyc_String_pa_PrintArg_struct _tmp5F3;_tmp5F3.tag=0U,({
# 3529
struct _fat_ptr _tmp787=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_rgneffect2string(r1);}));_tmp5F3.f1=_tmp787;});_tmp5F3;});struct Cyc_String_pa_PrintArg_struct _tmp52F=({struct Cyc_String_pa_PrintArg_struct _tmp5F2;_tmp5F2.tag=0U,({struct _fat_ptr _tmp788=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_rgneffect2string(ret);}));_tmp5F2.f1=_tmp788;});_tmp5F2;});void*_tmp52C[2U];_tmp52C[0]=& _tmp52E,_tmp52C[1]=& _tmp52F;({unsigned _tmp78A=loc;struct _fat_ptr _tmp789=({const char*_tmp52D="Invalid lock capability in region input effect  %s: Cannot pass a positive pure lock to spawn and retain the effect %s to this thread.";_tag_fat(_tmp52D,sizeof(char),135U);});Cyc_Tcutil_terr(_tmp78A,_tmp789,_tag_fat(_tmp52C,sizeof(void*),2U));});});
return 0;}}
# 3525
return ret;}}}}
# 3536
static struct Cyc_List_List*Cyc_IOEffect_output_caps(unsigned loc,struct Cyc_Absyn_RgnEffect*r1,struct Cyc_Absyn_RgnEffect*r2,struct Cyc_Absyn_RgnEffect*r3){
# 3542
struct _tuple12 _stmttmp7B=({struct _tuple12(*_tmp560)(struct Cyc_List_List*)=Cyc_Absyn_caps2tup;struct Cyc_List_List*_tmp561=({Cyc_Absyn_rgneffect_caps(r1);});_tmp560(_tmp561);});int _tmp537;int _tmp536;int _tmp535;_LL1: _tmp535=_stmttmp7B.f1;_tmp536=_stmttmp7B.f2;_tmp537=_stmttmp7B.f3;_LL2: {int in_rg=_tmp535;int in_lk=_tmp536;int alias_in=_tmp537;
# 3544
struct _tuple12 _stmttmp7C=({struct _tuple12(*_tmp55E)(struct Cyc_List_List*)=Cyc_Absyn_caps2tup;struct Cyc_List_List*_tmp55F=({Cyc_Absyn_rgneffect_caps(r2);});_tmp55E(_tmp55F);});int _tmp53A;int _tmp539;int _tmp538;_LL4: _tmp538=_stmttmp7C.f1;_tmp539=_stmttmp7C.f2;_tmp53A=_stmttmp7C.f3;_LL5: {int out_rg=_tmp538;int out_lk=_tmp539;int alias_out=_tmp53A;
# 3546
struct _tuple12 _stmttmp7D=({struct _tuple12(*_tmp55C)(struct Cyc_List_List*)=Cyc_Absyn_caps2tup;struct Cyc_List_List*_tmp55D=({Cyc_Absyn_rgneffect_caps(r3);});_tmp55C(_tmp55D);});int _tmp53D;int _tmp53C;int _tmp53B;_LL7: _tmp53B=_stmttmp7D.f1;_tmp53C=_stmttmp7D.f2;_tmp53D=_stmttmp7D.f3;_LL8: {int env_rg=_tmp53B;int env_lk=_tmp53C;int alias_env=_tmp53D;
# 3548
int ret_rg=0;
int ret_lk=0;
# 3551
if(alias_in && !alias_out || !alias_in && alias_out)
# 3553
({struct Cyc_String_pa_PrintArg_struct _tmp540=({struct Cyc_String_pa_PrintArg_struct _tmp5F6;_tmp5F6.tag=0U,({
struct _fat_ptr _tmp78B=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_rgneffect2string(r1);}));_tmp5F6.f1=_tmp78B;});_tmp5F6;});struct Cyc_String_pa_PrintArg_struct _tmp541=({struct Cyc_String_pa_PrintArg_struct _tmp5F5;_tmp5F5.tag=0U,({struct _fat_ptr _tmp78C=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_rgneffect2string(r2);}));_tmp5F5.f1=_tmp78C;});_tmp5F5;});struct Cyc_String_pa_PrintArg_struct _tmp542=({struct Cyc_String_pa_PrintArg_struct _tmp5F4;_tmp5F4.tag=0U,({
struct _fat_ptr _tmp78D=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_rgneffect2string(r3);}));_tmp5F4.f1=_tmp78D;});_tmp5F4;});void*_tmp53E[3U];_tmp53E[0]=& _tmp540,_tmp53E[1]=& _tmp541,_tmp53E[2]=& _tmp542;({unsigned _tmp78F=loc;struct _fat_ptr _tmp78E=({const char*_tmp53F="No output capability for input = %s output = %s env = %s: Input and output should be *both* pure/impure";_tag_fat(_tmp53F,sizeof(char),104U);});Cyc_Tcutil_terr(_tmp78F,_tmp78E,_tag_fat(_tmp53E,sizeof(void*),3U));});});else{
# 3557
if(alias_env && !alias_in)
# 3559
({struct Cyc_String_pa_PrintArg_struct _tmp545=({struct Cyc_String_pa_PrintArg_struct _tmp5F9;_tmp5F9.tag=0U,({
struct _fat_ptr _tmp790=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_rgneffect2string(r1);}));_tmp5F9.f1=_tmp790;});_tmp5F9;});struct Cyc_String_pa_PrintArg_struct _tmp546=({struct Cyc_String_pa_PrintArg_struct _tmp5F8;_tmp5F8.tag=0U,({struct _fat_ptr _tmp791=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_rgneffect2string(r2);}));_tmp5F8.f1=_tmp791;});_tmp5F8;});struct Cyc_String_pa_PrintArg_struct _tmp547=({struct Cyc_String_pa_PrintArg_struct _tmp5F7;_tmp5F7.tag=0U,({
struct _fat_ptr _tmp792=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_rgneffect2string(r3);}));_tmp5F7.f1=_tmp792;});_tmp5F7;});void*_tmp543[3U];_tmp543[0]=& _tmp545,_tmp543[1]=& _tmp546,_tmp543[2]=& _tmp547;({unsigned _tmp794=loc;struct _fat_ptr _tmp793=({const char*_tmp544="No output capability for input = %s output = %s env = %s: Cannot convert the enviroment impure effect to a pure one";_tag_fat(_tmp544,sizeof(char),116U);});Cyc_Tcutil_terr(_tmp794,_tmp793,_tag_fat(_tmp543,sizeof(void*),3U));});});else{
# 3565
if(env_rg < in_rg)
({struct Cyc_String_pa_PrintArg_struct _tmp54A=({struct Cyc_String_pa_PrintArg_struct _tmp5FB;_tmp5FB.tag=0U,({
struct _fat_ptr _tmp795=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_rgneffect2string(r1);}));_tmp5FB.f1=_tmp795;});_tmp5FB;});struct Cyc_String_pa_PrintArg_struct _tmp54B=({struct Cyc_String_pa_PrintArg_struct _tmp5FA;_tmp5FA.tag=0U,({struct _fat_ptr _tmp796=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_rgneffect2string(r3);}));_tmp5FA.f1=_tmp796;});_tmp5FA;});void*_tmp548[2U];_tmp548[0]=& _tmp54A,_tmp548[1]=& _tmp54B;({unsigned _tmp798=loc;struct _fat_ptr _tmp797=({const char*_tmp549="Function requires more region capabilities (%s) than the current region capabilities (%s)";_tag_fat(_tmp549,sizeof(char),90U);});Cyc_Tcutil_terr(_tmp798,_tmp797,_tag_fat(_tmp548,sizeof(void*),2U));});});
# 3565
ret_rg=(env_rg + out_rg)- in_rg;
# 3570
if(ret_rg < 0){
# 3572
({struct Cyc_String_pa_PrintArg_struct _tmp54E=({struct Cyc_String_pa_PrintArg_struct _tmp5FF;_tmp5FF.tag=0U,({
struct _fat_ptr _tmp799=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_rgneffect2string(r1);}));_tmp5FF.f1=_tmp799;});_tmp5FF;});struct Cyc_String_pa_PrintArg_struct _tmp54F=({struct Cyc_String_pa_PrintArg_struct _tmp5FE;_tmp5FE.tag=0U,({struct _fat_ptr _tmp79A=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_rgneffect2string(r2);}));_tmp5FE.f1=_tmp79A;});_tmp5FE;});struct Cyc_String_pa_PrintArg_struct _tmp550=({struct Cyc_String_pa_PrintArg_struct _tmp5FD;_tmp5FD.tag=0U,({
struct _fat_ptr _tmp79B=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_rgneffect2string(r3);}));_tmp5FD.f1=_tmp79B;});_tmp5FD;});struct Cyc_Int_pa_PrintArg_struct _tmp551=({struct Cyc_Int_pa_PrintArg_struct _tmp5FC;_tmp5FC.tag=1U,_tmp5FC.f1=(unsigned long)ret_rg;_tmp5FC;});void*_tmp54C[4U];_tmp54C[0]=& _tmp54E,_tmp54C[1]=& _tmp54F,_tmp54C[2]=& _tmp550,_tmp54C[3]=& _tmp551;({unsigned _tmp79D=loc;struct _fat_ptr _tmp79C=({const char*_tmp54D="No output (region) capability for input = %s output = %s env = %s: Negative output (region) capability : %d";_tag_fat(_tmp54D,sizeof(char),108U);});Cyc_Tcutil_terr(_tmp79D,_tmp79C,_tag_fat(_tmp54C,sizeof(void*),4U));});});
ret_rg=0;}
# 3570
if(env_lk < in_lk)
# 3578
({struct Cyc_String_pa_PrintArg_struct _tmp554=({struct Cyc_String_pa_PrintArg_struct _tmp601;_tmp601.tag=0U,({
struct _fat_ptr _tmp79E=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_rgneffect2string(r1);}));_tmp601.f1=_tmp79E;});_tmp601;});struct Cyc_String_pa_PrintArg_struct _tmp555=({struct Cyc_String_pa_PrintArg_struct _tmp600;_tmp600.tag=0U,({struct _fat_ptr _tmp79F=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_rgneffect2string(r3);}));_tmp600.f1=_tmp79F;});_tmp600;});void*_tmp552[2U];_tmp552[0]=& _tmp554,_tmp552[1]=& _tmp555;({unsigned _tmp7A1=loc;struct _fat_ptr _tmp7A0=({const char*_tmp553="Function requires more lock capabilities (%s) than the current lock capabilities (%s)";_tag_fat(_tmp553,sizeof(char),86U);});Cyc_Tcutil_terr(_tmp7A1,_tmp7A0,_tag_fat(_tmp552,sizeof(void*),2U));});});
# 3570
ret_lk=(env_lk + out_lk)- in_lk;
# 3582
if(ret_lk < 0){
# 3584
({struct Cyc_String_pa_PrintArg_struct _tmp558=({struct Cyc_String_pa_PrintArg_struct _tmp605;_tmp605.tag=0U,({
struct _fat_ptr _tmp7A2=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_rgneffect2string(r1);}));_tmp605.f1=_tmp7A2;});_tmp605;});struct Cyc_String_pa_PrintArg_struct _tmp559=({struct Cyc_String_pa_PrintArg_struct _tmp604;_tmp604.tag=0U,({struct _fat_ptr _tmp7A3=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_rgneffect2string(r2);}));_tmp604.f1=_tmp7A3;});_tmp604;});struct Cyc_String_pa_PrintArg_struct _tmp55A=({struct Cyc_String_pa_PrintArg_struct _tmp603;_tmp603.tag=0U,({
struct _fat_ptr _tmp7A4=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absyn_rgneffect2string(r3);}));_tmp603.f1=_tmp7A4;});_tmp603;});struct Cyc_Int_pa_PrintArg_struct _tmp55B=({struct Cyc_Int_pa_PrintArg_struct _tmp602;_tmp602.tag=1U,_tmp602.f1=(unsigned long)ret_lk;_tmp602;});void*_tmp556[4U];_tmp556[0]=& _tmp558,_tmp556[1]=& _tmp559,_tmp556[2]=& _tmp55A,_tmp556[3]=& _tmp55B;({unsigned _tmp7A6=loc;struct _fat_ptr _tmp7A5=({const char*_tmp557="No output (lock) capability for input = %s output = %s env = %s: Negative output (lock) capability : %d";_tag_fat(_tmp557,sizeof(char),104U);});Cyc_Tcutil_terr(_tmp7A6,_tmp7A5,_tag_fat(_tmp556,sizeof(void*),4U));});});
ret_lk=0;}}}
# 3590
return({Cyc_Absyn_tup2caps(ret_rg,ret_lk,alias_env);});}}}}

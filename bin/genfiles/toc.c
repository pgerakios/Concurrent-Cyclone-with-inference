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
extern struct _RegionHandle*Cyc_Core_unique_region;
# 175
void Cyc_Core_ufree(void*ptr);
# 188
struct _fat_ptr Cyc_Core_alias_refptr(struct _fat_ptr ptr);struct Cyc_Core_DynamicRegion;struct Cyc_Core_NewDynamicRegion{struct Cyc_Core_DynamicRegion*key;};
# 237
struct Cyc_Core_NewDynamicRegion Cyc_Core__new_rckey(const char*file,const char*func,int lineno);
# 255 "core.h"
void Cyc_Core_free_rckey(struct Cyc_Core_DynamicRegion*k);struct Cyc_Core_ThinRes{void*arr;unsigned nelts;};struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};
# 54 "list.h"
struct Cyc_List_List*Cyc_List_list(struct _fat_ptr);
# 57
struct Cyc_List_List*Cyc_List_rlist(struct _RegionHandle*,struct _fat_ptr);
# 61
int Cyc_List_length(struct Cyc_List_List*x);
# 76
struct Cyc_List_List*Cyc_List_map(void*(*f)(void*),struct Cyc_List_List*x);
# 79
struct Cyc_List_List*Cyc_List_rmap(struct _RegionHandle*,void*(*f)(void*),struct Cyc_List_List*x);
# 83
struct Cyc_List_List*Cyc_List_map_c(void*(*f)(void*,void*),void*env,struct Cyc_List_List*x);
# 86
struct Cyc_List_List*Cyc_List_rmap_c(struct _RegionHandle*,void*(*f)(void*,void*),void*env,struct Cyc_List_List*x);extern char Cyc_List_List_mismatch[14U];struct Cyc_List_List_mismatch_exn_struct{char*tag;};
# 133
void Cyc_List_iter(void(*f)(void*),struct Cyc_List_List*x);
# 135
void Cyc_List_iter_c(void(*f)(void*,void*),void*env,struct Cyc_List_List*x);
# 153
void*Cyc_List_fold_right(void*(*f)(void*,void*),struct Cyc_List_List*x,void*accum);
# 172
struct Cyc_List_List*Cyc_List_rev(struct Cyc_List_List*x);
# 178
struct Cyc_List_List*Cyc_List_imp_rev(struct Cyc_List_List*x);
# 184
struct Cyc_List_List*Cyc_List_append(struct Cyc_List_List*x,struct Cyc_List_List*y);
# 195
struct Cyc_List_List*Cyc_List_imp_append(struct Cyc_List_List*x,struct Cyc_List_List*y);extern char Cyc_List_Nth[4U];struct Cyc_List_Nth_exn_struct{char*tag;};
# 242
void*Cyc_List_nth(struct Cyc_List_List*x,int n);
# 251
int Cyc_List_forall(int(*pred)(void*),struct Cyc_List_List*x);
# 270
struct Cyc_List_List*Cyc_List_zip(struct Cyc_List_List*x,struct Cyc_List_List*y);
# 276
struct Cyc_List_List*Cyc_List_rzip(struct _RegionHandle*r1,struct _RegionHandle*r2,struct Cyc_List_List*x,struct Cyc_List_List*y);struct _tuple0{struct Cyc_List_List*f1;struct Cyc_List_List*f2;};
# 294
struct _tuple0 Cyc_List_split(struct Cyc_List_List*x);
# 328
void*Cyc_List_assoc(struct Cyc_List_List*l,void*k);
# 371
struct Cyc_List_List*Cyc_List_from_array(struct _fat_ptr arr);
# 383
int Cyc_List_list_cmp(int(*cmp)(void*,void*),struct Cyc_List_List*l1,struct Cyc_List_List*l2);struct Cyc___cycFILE;struct Cyc_String_pa_PrintArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Int_pa_PrintArg_struct{int tag;unsigned long f1;};struct Cyc_Double_pa_PrintArg_struct{int tag;double f1;};struct Cyc_LongDouble_pa_PrintArg_struct{int tag;long double f1;};struct Cyc_ShortPtr_pa_PrintArg_struct{int tag;short*f1;};struct Cyc_IntPtr_pa_PrintArg_struct{int tag;unsigned long*f1;};
# 73 "cycboot.h"
struct _fat_ptr Cyc_aprintf(struct _fat_ptr,struct _fat_ptr);struct Cyc_ShortPtr_sa_ScanfArg_struct{int tag;short*f1;};struct Cyc_UShortPtr_sa_ScanfArg_struct{int tag;unsigned short*f1;};struct Cyc_IntPtr_sa_ScanfArg_struct{int tag;int*f1;};struct Cyc_UIntPtr_sa_ScanfArg_struct{int tag;unsigned*f1;};struct Cyc_StringPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_DoublePtr_sa_ScanfArg_struct{int tag;double*f1;};struct Cyc_FloatPtr_sa_ScanfArg_struct{int tag;float*f1;};struct Cyc_CharPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};
# 157 "cycboot.h"
int Cyc_printf(struct _fat_ptr,struct _fat_ptr);extern char Cyc_FileCloseError[15U];struct Cyc_FileCloseError_exn_struct{char*tag;};extern char Cyc_FileOpenError[14U];struct Cyc_FileOpenError_exn_struct{char*tag;struct _fat_ptr f1;};
# 38 "string.h"
unsigned long Cyc_strlen(struct _fat_ptr s);
# 49 "string.h"
int Cyc_strcmp(struct _fat_ptr s1,struct _fat_ptr s2);
int Cyc_strptrcmp(struct _fat_ptr*s1,struct _fat_ptr*s2);
# 62
struct _fat_ptr Cyc_strconcat(struct _fat_ptr,struct _fat_ptr);
# 67
struct _fat_ptr Cyc_rstr_sepstr(struct _RegionHandle*,struct Cyc_List_List*,struct _fat_ptr);struct Cyc_Iter_Iter{void*env;int(*next)(void*env,void*dest);};struct Cyc_Set_Set;
# 54 "set.h"
struct Cyc_Set_Set*Cyc_Set_rempty(struct _RegionHandle*r,int(*cmp)(void*,void*));
# 69
struct Cyc_Set_Set*Cyc_Set_rinsert(struct _RegionHandle*r,struct Cyc_Set_Set*s,void*elt);
# 100
int Cyc_Set_member(struct Cyc_Set_Set*s,void*elt);extern char Cyc_Set_Absent[7U];struct Cyc_Set_Absent_exn_struct{char*tag;};struct Cyc_Dict_T;struct Cyc_Dict_Dict{int(*rel)(void*,void*);struct _RegionHandle*r;const struct Cyc_Dict_T*t;};extern char Cyc_Dict_Present[8U];struct Cyc_Dict_Present_exn_struct{char*tag;};extern char Cyc_Dict_Absent[7U];struct Cyc_Dict_Absent_exn_struct{char*tag;};
# 68 "dict.h"
struct Cyc_Dict_Dict Cyc_Dict_rempty(struct _RegionHandle*,int(*cmp)(void*,void*));
# 87
struct Cyc_Dict_Dict Cyc_Dict_insert(struct Cyc_Dict_Dict d,void*k,void*v);
# 113
void*Cyc_Dict_lookup_other(struct Cyc_Dict_Dict d,int(*cmp)(void*,void*),void*k);
# 122 "dict.h"
void**Cyc_Dict_lookup_opt(struct Cyc_Dict_Dict d,void*k);struct Cyc_Xarray_Xarray{struct _fat_ptr elmts;int num_elmts;};
# 40 "xarray.h"
int Cyc_Xarray_length(struct Cyc_Xarray_Xarray*);
# 42
void*Cyc_Xarray_get(struct Cyc_Xarray_Xarray*,int);
# 57
struct Cyc_Xarray_Xarray*Cyc_Xarray_rcreate_empty(struct _RegionHandle*);
# 69
int Cyc_Xarray_add_ind(struct Cyc_Xarray_Xarray*,void*);
# 121
void Cyc_Xarray_reuse(struct Cyc_Xarray_Xarray*xarr);struct Cyc_Position_Error;struct Cyc_Relations_Reln;struct _union_Nmspace_Rel_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Abs_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_C_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Loc_n{int tag;int val;};union Cyc_Absyn_Nmspace{struct _union_Nmspace_Rel_n Rel_n;struct _union_Nmspace_Abs_n Abs_n;struct _union_Nmspace_C_n C_n;struct _union_Nmspace_Loc_n Loc_n;};
# 110 "absyn.h"
extern union Cyc_Absyn_Nmspace Cyc_Absyn_Loc_n;
union Cyc_Absyn_Nmspace Cyc_Absyn_Rel_n(struct Cyc_List_List*);
# 113
union Cyc_Absyn_Nmspace Cyc_Absyn_Abs_n(struct Cyc_List_List*ns,int C_scope);struct _tuple1{union Cyc_Absyn_Nmspace f1;struct _fat_ptr*f2;};
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
enum Cyc_Absyn_KindQual{Cyc_Absyn_AnyKind =0U,Cyc_Absyn_MemKind =1U,Cyc_Absyn_BoxKind =2U,Cyc_Absyn_XRgnKind =3U,Cyc_Absyn_RgnKind =4U,Cyc_Absyn_EffKind =5U,Cyc_Absyn_IntKind =6U,Cyc_Absyn_BoolKind =7U,Cyc_Absyn_PtrBndKind =8U};struct Cyc_Absyn_Kind{enum Cyc_Absyn_KindQual kind;enum Cyc_Absyn_AliasQual aliasqual;};struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct{int tag;struct Cyc_Absyn_Kind*f1;};struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct{int tag;struct Cyc_Core_Opt*f1;};struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct{int tag;struct Cyc_Core_Opt*f1;struct Cyc_Absyn_Kind*f2;};struct Cyc_Absyn_Tvar{struct _fat_ptr*name;int identity;void*kind;};struct Cyc_Absyn_PtrLoc{unsigned ptr_loc;unsigned rgn_loc;unsigned zt_loc;};struct Cyc_Absyn_PtrAtts{void*rgn;void*nullable;void*bounds;void*zero_term;struct Cyc_Absyn_PtrLoc*ptrloc;};struct Cyc_Absyn_PtrInfo{void*elt_type;struct Cyc_Absyn_Tqual elt_tq;struct Cyc_Absyn_PtrAtts ptr_atts;};struct Cyc_Absyn_VarargInfo{struct _fat_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;int inject;};struct Cyc_Absyn_Star_Absyn_Cap_struct{int tag;};struct Cyc_Absyn_Bot_Absyn_Cap_struct{int tag;};struct Cyc_Absyn_Nat_Absyn_Cap_struct{int tag;int f1;int f2;};struct Cyc_Absyn_RgnEffect{void*name;struct Cyc_List_List*caps;void*parent;};struct Cyc_Absyn_FnInfo{struct Cyc_List_List*tvars;void*effect;struct Cyc_Absyn_Tqual ret_tqual;void*ret_type;struct Cyc_List_List*args;int c_varargs;struct Cyc_Absyn_VarargInfo*cyc_varargs;struct Cyc_List_List*rgn_po;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*requires_clause;struct Cyc_List_List*requires_relns;struct Cyc_Absyn_Exp*ensures_clause;struct Cyc_List_List*ensures_relns;struct Cyc_List_List*ieffect;struct Cyc_List_List*oeffect;struct Cyc_List_List*throws;int reentrant;};struct Cyc_Absyn_UnknownDatatypeInfo{struct _tuple1*name;int is_extensible;};struct _union_DatatypeInfo_UnknownDatatype{int tag;struct Cyc_Absyn_UnknownDatatypeInfo val;};struct _union_DatatypeInfo_KnownDatatype{int tag;struct Cyc_Absyn_Datatypedecl**val;};union Cyc_Absyn_DatatypeInfo{struct _union_DatatypeInfo_UnknownDatatype UnknownDatatype;struct _union_DatatypeInfo_KnownDatatype KnownDatatype;};struct Cyc_Absyn_UnknownDatatypeFieldInfo{struct _tuple1*datatype_name;struct _tuple1*field_name;int is_extensible;};struct _union_DatatypeFieldInfo_UnknownDatatypefield{int tag;struct Cyc_Absyn_UnknownDatatypeFieldInfo val;};struct _tuple2{struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;};struct _union_DatatypeFieldInfo_KnownDatatypefield{int tag;struct _tuple2 val;};union Cyc_Absyn_DatatypeFieldInfo{struct _union_DatatypeFieldInfo_UnknownDatatypefield UnknownDatatypefield;struct _union_DatatypeFieldInfo_KnownDatatypefield KnownDatatypefield;};struct _tuple3{enum Cyc_Absyn_AggrKind f1;struct _tuple1*f2;struct Cyc_Core_Opt*f3;};struct _union_AggrInfo_UnknownAggr{int tag;struct _tuple3 val;};struct _union_AggrInfo_KnownAggr{int tag;struct Cyc_Absyn_Aggrdecl**val;};union Cyc_Absyn_AggrInfo{struct _union_AggrInfo_UnknownAggr UnknownAggr;struct _union_AggrInfo_KnownAggr KnownAggr;};
# 366
union Cyc_Absyn_AggrInfo Cyc_Absyn_UnknownAggr(enum Cyc_Absyn_AggrKind,struct _tuple1*,struct Cyc_Core_Opt*);
union Cyc_Absyn_AggrInfo Cyc_Absyn_KnownAggr(struct Cyc_Absyn_Aggrdecl**);struct Cyc_Absyn_ArrayInfo{void*elt_type;struct Cyc_Absyn_Tqual tq;struct Cyc_Absyn_Exp*num_elts;void*zero_term;unsigned zt_loc;};struct Cyc_Absyn_Aggr_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Enum_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;};struct Cyc_Absyn_Datatype_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;};struct Cyc_Absyn_TypeDecl{void*r;unsigned loc;};struct Cyc_Absyn_VoidCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_IntCon_Absyn_TyCon_struct{int tag;enum Cyc_Absyn_Sign f1;enum Cyc_Absyn_Size_of f2;};struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct{int tag;int f1;};struct Cyc_Absyn_RgnHandleCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_XRgnHandleCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_TagCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_HeapCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_UniqueCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_RefCntCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_AccessCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_CAccessCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_JoinCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_RgnsCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_TrueCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_FalseCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_ThinCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_FatCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct{int tag;struct _tuple1*f1;struct Cyc_Absyn_Enumdecl*f2;};struct Cyc_Absyn_AnonEnumCon_Absyn_TyCon_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_BuiltinCon_Absyn_TyCon_struct{int tag;struct _fat_ptr f1;struct Cyc_Absyn_Kind*f2;};struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_DatatypeInfo f1;};struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_DatatypeFieldInfo f1;};struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_AggrInfo f1;};struct Cyc_Absyn_AppType_Absyn_Type_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Evar_Absyn_Type_struct{int tag;struct Cyc_Core_Opt*f1;void*f2;int f3;struct Cyc_Core_Opt*f4;};struct Cyc_Absyn_VarType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Tvar*f1;};struct Cyc_Absyn_PointerType_Absyn_Type_struct{int tag;struct Cyc_Absyn_PtrInfo f1;};struct Cyc_Absyn_ArrayType_Absyn_Type_struct{int tag;struct Cyc_Absyn_ArrayInfo f1;};struct Cyc_Absyn_FnType_Absyn_Type_struct{int tag;struct Cyc_Absyn_FnInfo f1;};struct Cyc_Absyn_TupleType_Absyn_Type_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct{int tag;enum Cyc_Absyn_AggrKind f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_TypedefType_Absyn_Type_struct{int tag;struct _tuple1*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_Typedefdecl*f3;void*f4;};struct Cyc_Absyn_ValueofType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct{int tag;struct Cyc_Absyn_TypeDecl*f1;void**f2;};struct Cyc_Absyn_TypeofType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_NoTypes_Absyn_Funcparams_struct{int tag;struct Cyc_List_List*f1;unsigned f2;};struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct{int tag;struct Cyc_List_List*f1;int f2;struct Cyc_Absyn_VarargInfo*f3;void*f4;struct Cyc_List_List*f5;struct Cyc_Absyn_Exp*f6;struct Cyc_Absyn_Exp*f7;struct Cyc_List_List*f8;struct Cyc_List_List*f9;struct Cyc_List_List*f10;int f11;};
# 478 "absyn.h"
enum Cyc_Absyn_Format_Type{Cyc_Absyn_Printf_ft =0U,Cyc_Absyn_Scanf_ft =1U};struct Cyc_Absyn_Regparm_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Stdcall_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Cdecl_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Fastcall_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Noreturn_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Const_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Aligned_att_Absyn_Attribute_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Packed_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Section_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Nocommon_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Shared_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Unused_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Weak_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Dllimport_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Dllexport_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_instrument_function_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Constructor_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Destructor_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_check_memory_usage_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Format_att_Absyn_Attribute_struct{int tag;enum Cyc_Absyn_Format_Type f1;int f2;int f3;};struct Cyc_Absyn_Initializes_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Noliveunique_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Consume_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Pure_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Mode_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Alias_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Always_inline_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_throw_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Non_null_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Deprecated_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_VectorSize_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Carray_mod_Absyn_Type_modifier_struct{int tag;void*f1;unsigned f2;};struct Cyc_Absyn_ConstArray_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;unsigned f3;};struct Cyc_Absyn_Pointer_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_Absyn_PtrAtts f1;struct Cyc_Absyn_Tqual f2;};struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct{int tag;void*f1;};struct Cyc_Absyn_TypeParams_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_List_List*f1;unsigned f2;int f3;};struct Cyc_Absyn_Attributes_mod_Absyn_Type_modifier_struct{int tag;unsigned f1;struct Cyc_List_List*f2;};struct _union_Cnst_Null_c{int tag;int val;};struct _tuple4{enum Cyc_Absyn_Sign f1;char f2;};struct _union_Cnst_Char_c{int tag;struct _tuple4 val;};struct _union_Cnst_Wchar_c{int tag;struct _fat_ptr val;};struct _tuple5{enum Cyc_Absyn_Sign f1;short f2;};struct _union_Cnst_Short_c{int tag;struct _tuple5 val;};struct _tuple6{enum Cyc_Absyn_Sign f1;int f2;};struct _union_Cnst_Int_c{int tag;struct _tuple6 val;};struct _tuple7{enum Cyc_Absyn_Sign f1;long long f2;};struct _union_Cnst_LongLong_c{int tag;struct _tuple7 val;};struct _tuple8{struct _fat_ptr f1;int f2;};struct _union_Cnst_Float_c{int tag;struct _tuple8 val;};struct _union_Cnst_String_c{int tag;struct _fat_ptr val;};struct _union_Cnst_Wstring_c{int tag;struct _fat_ptr val;};union Cyc_Absyn_Cnst{struct _union_Cnst_Null_c Null_c;struct _union_Cnst_Char_c Char_c;struct _union_Cnst_Wchar_c Wchar_c;struct _union_Cnst_Short_c Short_c;struct _union_Cnst_Int_c Int_c;struct _union_Cnst_LongLong_c LongLong_c;struct _union_Cnst_Float_c Float_c;struct _union_Cnst_String_c String_c;struct _union_Cnst_Wstring_c Wstring_c;};
# 574
enum Cyc_Absyn_Primop{Cyc_Absyn_Plus =0U,Cyc_Absyn_Times =1U,Cyc_Absyn_Minus =2U,Cyc_Absyn_Div =3U,Cyc_Absyn_Mod =4U,Cyc_Absyn_Eq =5U,Cyc_Absyn_Neq =6U,Cyc_Absyn_Gt =7U,Cyc_Absyn_Lt =8U,Cyc_Absyn_Gte =9U,Cyc_Absyn_Lte =10U,Cyc_Absyn_Not =11U,Cyc_Absyn_Bitnot =12U,Cyc_Absyn_Bitand =13U,Cyc_Absyn_Bitor =14U,Cyc_Absyn_Bitxor =15U,Cyc_Absyn_Bitlshift =16U,Cyc_Absyn_Bitlrshift =17U,Cyc_Absyn_Numelts =18U};
# 581
enum Cyc_Absyn_Incrementor{Cyc_Absyn_PreInc =0U,Cyc_Absyn_PostInc =1U,Cyc_Absyn_PreDec =2U,Cyc_Absyn_PostDec =3U};struct Cyc_Absyn_VarargCallInfo{int num_varargs;struct Cyc_List_List*injectors;struct Cyc_Absyn_VarargInfo*vai;};struct Cyc_Absyn_StructField_Absyn_OffsetofField_struct{int tag;struct _fat_ptr*f1;};struct Cyc_Absyn_TupleIndex_Absyn_OffsetofField_struct{int tag;unsigned f1;};
# 599
enum Cyc_Absyn_Coercion{Cyc_Absyn_Unknown_coercion =0U,Cyc_Absyn_No_coercion =1U,Cyc_Absyn_Null_to_NonNull =2U,Cyc_Absyn_Other_coercion =3U};struct Cyc_Absyn_MallocInfo{int is_calloc;struct Cyc_Absyn_Exp*rgn;void**elt_type;struct Cyc_Absyn_Exp*num_elts;int fat_result;int inline_call;};struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct{int tag;union Cyc_Absyn_Cnst f1;};struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Pragma_e_Absyn_Raw_exp_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct{int tag;enum Cyc_Absyn_Primop f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;enum Cyc_Absyn_Incrementor f2;};struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_VarargCallInfo*f3;int f4;struct _tuple0*f5;};struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;int f2;};struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Exp*f2;int f3;enum Cyc_Absyn_Coercion f4;};struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;int f3;int f4;};struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;int f3;int f4;};struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Tuple_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;};struct _tuple9{struct _fat_ptr*f1;struct Cyc_Absyn_Tqual f2;void*f3;};struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct{int tag;struct _tuple9*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;int f4;};struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;int f3;};struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct{int tag;struct _tuple1*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct Cyc_Absyn_Aggrdecl*f4;};struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Datatypedecl*f2;struct Cyc_Absyn_Datatypefield*f3;};struct Cyc_Absyn_Enum_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_Spawn_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;struct _tuple0*f3;};struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_MallocInfo f1;};struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Core_Opt*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Stmt*f1;};struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;};struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct{int tag;int f1;struct _fat_ptr f2;struct Cyc_List_List*f3;struct Cyc_List_List*f4;struct Cyc_List_List*f5;};struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Exp{void*topt;void*r;unsigned loc;void*annot;};struct Cyc_Absyn_Skip_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;struct Cyc_Absyn_Stmt*f3;};struct _tuple10{struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct{int tag;struct _tuple10 f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Break_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Continue_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Goto_s_Absyn_Raw_stmt_struct{int tag;struct _fat_ptr*f1;};struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _tuple10 f2;struct _tuple10 f3;struct Cyc_Absyn_Stmt*f4;};struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;void*f3;};struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Switch_clause**f2;};struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Decl*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct{int tag;struct _fat_ptr*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct _tuple10 f2;};struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_List_List*f2;void*f3;};struct Cyc_Absyn_Stmt{void*r;unsigned loc;void*annot;};struct Cyc_Absyn_Wild_p_Absyn_Raw_pat_struct{int tag;};struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_AliasVar_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_TagInt_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Tuple_p_Absyn_Raw_pat_struct{int tag;struct Cyc_List_List*f1;int f2;};struct Cyc_Absyn_Pointer_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Pat*f1;};struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct{int tag;union Cyc_Absyn_AggrInfo*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;int f4;};struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;struct Cyc_List_List*f3;int f4;};struct Cyc_Absyn_Null_p_Absyn_Raw_pat_struct{int tag;};struct Cyc_Absyn_Int_p_Absyn_Raw_pat_struct{int tag;enum Cyc_Absyn_Sign f1;int f2;};struct Cyc_Absyn_Char_p_Absyn_Raw_pat_struct{int tag;char f1;};struct Cyc_Absyn_Float_p_Absyn_Raw_pat_struct{int tag;struct _fat_ptr f1;int f2;};struct Cyc_Absyn_Enum_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_AnonEnum_p_Absyn_Raw_pat_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_UnknownId_p_Absyn_Raw_pat_struct{int tag;struct _tuple1*f1;};struct Cyc_Absyn_UnknownCall_p_Absyn_Raw_pat_struct{int tag;struct _tuple1*f1;struct Cyc_List_List*f2;int f3;};struct Cyc_Absyn_Exp_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Exp*f1;};
# 771 "absyn.h"
extern struct Cyc_Absyn_Wild_p_Absyn_Raw_pat_struct Cyc_Absyn_Wild_p_val;struct Cyc_Absyn_Pat{void*r;void*topt;unsigned loc;};struct Cyc_Absyn_Switch_clause{struct Cyc_Absyn_Pat*pattern;struct Cyc_Core_Opt*pat_vars;struct Cyc_Absyn_Exp*where_clause;struct Cyc_Absyn_Stmt*body;unsigned loc;};struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct{int tag;struct _tuple1*f1;};struct Cyc_Absyn_Global_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Funname_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Param_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Local_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Pat_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Vardecl{enum Cyc_Absyn_Scope sc;struct _tuple1*name;unsigned varloc;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*initializer;void*rgn;struct Cyc_List_List*attributes;int escapes;};struct Cyc_Absyn_Fndecl{enum Cyc_Absyn_Scope sc;int is_inline;struct _tuple1*name;struct Cyc_Absyn_Stmt*body;struct Cyc_Absyn_FnInfo i;void*cached_type;struct Cyc_Core_Opt*param_vardecls;struct Cyc_Absyn_Vardecl*fn_vardecl;};struct Cyc_Absyn_Aggrfield{struct _fat_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*width;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*requires_clause;};struct Cyc_Absyn_AggrdeclImpl{struct Cyc_List_List*exist_vars;struct Cyc_List_List*rgn_po;struct Cyc_List_List*fields;int tagged;};struct Cyc_Absyn_Aggrdecl{enum Cyc_Absyn_AggrKind kind;enum Cyc_Absyn_Scope sc;struct _tuple1*name;struct Cyc_List_List*tvs;struct Cyc_Absyn_AggrdeclImpl*impl;struct Cyc_List_List*attributes;int expected_mem_kind;};struct Cyc_Absyn_Datatypefield{struct _tuple1*name;struct Cyc_List_List*typs;unsigned loc;enum Cyc_Absyn_Scope sc;};struct Cyc_Absyn_Datatypedecl{enum Cyc_Absyn_Scope sc;struct _tuple1*name;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*fields;int is_extensible;};struct Cyc_Absyn_Enumfield{struct _tuple1*name;struct Cyc_Absyn_Exp*tag;unsigned loc;};struct Cyc_Absyn_Enumdecl{enum Cyc_Absyn_Scope sc;struct _tuple1*name;struct Cyc_Core_Opt*fields;};struct Cyc_Absyn_Typedefdecl{struct _tuple1*name;struct Cyc_Absyn_Tqual tq;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*kind;void*defn;struct Cyc_List_List*atts;int extern_c;};struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Pat*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;void*f4;};struct Cyc_Absyn_Letv_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;};struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;};struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Typedefdecl*f1;};struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct{int tag;struct _fat_ptr*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct{int tag;struct _tuple1*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_ExternC_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;};struct _tuple11{unsigned f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct _tuple11*f4;};struct Cyc_Absyn_Porton_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Portoff_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Tempeston_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Tempestoff_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Decl{void*r;unsigned loc;};struct Cyc_Absyn_ArrayElement_Absyn_Designator_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_FieldName_Absyn_Designator_struct{int tag;struct _fat_ptr*f1;};extern char Cyc_Absyn_EmptyAnnot[11U];struct Cyc_Absyn_EmptyAnnot_Absyn_AbsynAnnot_struct{char*tag;};
# 947
extern struct Cyc_Absyn_EmptyAnnot_Absyn_AbsynAnnot_struct Cyc_Absyn_EmptyAnnot_val;
# 954
int Cyc_Absyn_qvar_cmp(struct _tuple1*,struct _tuple1*);
# 963
struct Cyc_Absyn_Tqual Cyc_Absyn_empty_tqual(unsigned);
# 984
extern void*Cyc_Absyn_char_type;extern void*Cyc_Absyn_uchar_type;extern void*Cyc_Absyn_uint_type;
# 986
extern void*Cyc_Absyn_sint_type;
# 991
extern void*Cyc_Absyn_heap_rgn_type;
# 995
extern void*Cyc_Absyn_true_type;extern void*Cyc_Absyn_false_type;
# 997
extern void*Cyc_Absyn_void_type;
# 1014
void*Cyc_Absyn_exn_type();
# 1022
extern void*Cyc_Absyn_fat_bound_type;
# 1026
void*Cyc_Absyn_bounds_one();
# 1036
void*Cyc_Absyn_star_type(void*t,void*rgn,struct Cyc_Absyn_Tqual,void*zero_term);
# 1040
void*Cyc_Absyn_cstar_type(void*,struct Cyc_Absyn_Tqual);
# 1042
void*Cyc_Absyn_fatptr_type(void*t,void*rgn,struct Cyc_Absyn_Tqual,void*zeroterm);
# 1044
void*Cyc_Absyn_strct(struct _fat_ptr*name);
void*Cyc_Absyn_strctq(struct _tuple1*name);
void*Cyc_Absyn_unionq_type(struct _tuple1*name);
# 1050
void*Cyc_Absyn_array_type(void*elt_type,struct Cyc_Absyn_Tqual tq,struct Cyc_Absyn_Exp*num_elts,void*zero_term,unsigned ztloc);
# 1055
void*Cyc_Absyn_aggr_type(union Cyc_Absyn_AggrInfo,struct Cyc_List_List*args);
# 1058
struct Cyc_Absyn_Exp*Cyc_Absyn_new_exp(void*,unsigned);
# 1060
struct Cyc_Absyn_Exp*Cyc_Absyn_copy_exp(struct Cyc_Absyn_Exp*);
# 1066
struct Cyc_Absyn_Exp*Cyc_Absyn_int_exp(enum Cyc_Absyn_Sign,int,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_signed_int_exp(int,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_uint_exp(unsigned,unsigned);
# 1071
struct Cyc_Absyn_Exp*Cyc_Absyn_float_exp(struct _fat_ptr,int,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_string_exp(struct _fat_ptr,unsigned);
# 1074
struct Cyc_Absyn_Exp*Cyc_Absyn_var_exp(struct _tuple1*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_varb_exp(void*,unsigned);
# 1079
struct Cyc_Absyn_Exp*Cyc_Absyn_prim1_exp(enum Cyc_Absyn_Primop,struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_prim2_exp(enum Cyc_Absyn_Primop,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
# 1082
struct Cyc_Absyn_Exp*Cyc_Absyn_add_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
# 1084
struct Cyc_Absyn_Exp*Cyc_Absyn_divide_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_eq_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_neq_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
# 1091
struct Cyc_Absyn_Exp*Cyc_Absyn_assignop_exp(struct Cyc_Absyn_Exp*,struct Cyc_Core_Opt*,struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_assign_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_increment_exp(struct Cyc_Absyn_Exp*,enum Cyc_Absyn_Incrementor,unsigned);
# 1095
struct Cyc_Absyn_Exp*Cyc_Absyn_and_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
# 1099
struct Cyc_Absyn_Exp*Cyc_Absyn_fncall_exp(struct Cyc_Absyn_Exp*,struct Cyc_List_List*,unsigned);
# 1104
struct Cyc_Absyn_Exp*Cyc_Absyn_cast_exp(void*,struct Cyc_Absyn_Exp*,int user_cast,enum Cyc_Absyn_Coercion,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_address_exp(struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_sizeoftype_exp(void*t,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_sizeofexp_exp(struct Cyc_Absyn_Exp*e,unsigned);
# 1109
struct Cyc_Absyn_Exp*Cyc_Absyn_deref_exp(struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_aggrmember_exp(struct Cyc_Absyn_Exp*,struct _fat_ptr*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_aggrarrow_exp(struct Cyc_Absyn_Exp*,struct _fat_ptr*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_subscript_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_tuple_exp(struct Cyc_List_List*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_stmt_exp(struct Cyc_Absyn_Stmt*,unsigned);
# 1116
struct Cyc_Absyn_Exp*Cyc_Absyn_array_exp(struct Cyc_List_List*,unsigned);
# 1121
struct Cyc_Absyn_Exp*Cyc_Absyn_unresolvedmem_exp(struct Cyc_Core_Opt*,struct Cyc_List_List*,unsigned);
# 1127
struct Cyc_Absyn_Stmt*Cyc_Absyn_new_stmt(void*,unsigned);
struct Cyc_Absyn_Stmt*Cyc_Absyn_skip_stmt(unsigned);
struct Cyc_Absyn_Stmt*Cyc_Absyn_exp_stmt(struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Stmt*Cyc_Absyn_seq_stmt(struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt*,unsigned);
# 1132
struct Cyc_Absyn_Stmt*Cyc_Absyn_return_stmt(struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Stmt*Cyc_Absyn_ifthenelse_stmt(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt*,unsigned);
# 1140
struct Cyc_Absyn_Stmt*Cyc_Absyn_decl_stmt(struct Cyc_Absyn_Decl*,struct Cyc_Absyn_Stmt*,unsigned);
struct Cyc_Absyn_Stmt*Cyc_Absyn_declare_stmt(struct _tuple1*,void*,struct Cyc_Absyn_Exp*init,struct Cyc_Absyn_Stmt*,unsigned);
struct Cyc_Absyn_Stmt*Cyc_Absyn_label_stmt(struct _fat_ptr*,struct Cyc_Absyn_Stmt*,unsigned);
# 1144
struct Cyc_Absyn_Stmt*Cyc_Absyn_goto_stmt(struct _fat_ptr*,unsigned);
struct Cyc_Absyn_Stmt*Cyc_Absyn_assign_stmt(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
# 1149
struct Cyc_Absyn_Pat*Cyc_Absyn_new_pat(void*,unsigned);
# 1153
struct Cyc_Absyn_Decl*Cyc_Absyn_new_decl(void*,unsigned);
# 1158
struct Cyc_Absyn_Vardecl*Cyc_Absyn_new_vardecl(unsigned varloc,struct _tuple1*,void*,struct Cyc_Absyn_Exp*init);
struct Cyc_Absyn_Vardecl*Cyc_Absyn_static_vardecl(struct _tuple1*x,void*t,struct Cyc_Absyn_Exp*init);
# 1200
int Cyc_Absyn_is_lvalue(struct Cyc_Absyn_Exp*);
# 1203
struct Cyc_Absyn_Aggrfield*Cyc_Absyn_lookup_field(struct Cyc_List_List*,struct _fat_ptr*);
# 1221
struct _fat_ptr*Cyc_Absyn_fieldname(int);struct _tuple12{enum Cyc_Absyn_AggrKind f1;struct _tuple1*f2;};
# 1223
struct _tuple12 Cyc_Absyn_aggr_kinded_name(union Cyc_Absyn_AggrInfo);
# 1225
struct Cyc_Absyn_Aggrdecl*Cyc_Absyn_get_known_aggrdecl(union Cyc_Absyn_AggrInfo);
# 1231
struct _tuple1*Cyc_Absyn_binding2qvar(void*);
# 1234
struct _fat_ptr*Cyc_Absyn_designatorlist_to_fieldname(struct Cyc_List_List*);
# 1239
extern int Cyc_Absyn_no_regions;
# 1249
int Cyc_Absyn_var_may_appear_exp(struct _tuple1*,struct Cyc_Absyn_Exp*);
# 1280
struct Cyc_Absyn_Tvar*Cyc_Absyn_type2tvar(void*t);
# 1287
int Cyc_Absyn_is_xrgn_tvar(struct Cyc_Absyn_Tvar*tv);
int Cyc_Absyn_is_xrgn(void*r);struct _tuple13{int f1;int f2;int f3;};
# 1323
struct _tuple13 Cyc_Absyn_caps2tup(struct Cyc_List_List*);
# 1335
int Cyc_Absyn_rgneffect_cmp_name(struct Cyc_Absyn_RgnEffect*r1,struct Cyc_Absyn_RgnEffect*r2);
# 1338
struct Cyc_Absyn_RgnEffect*Cyc_Absyn_find_rgneffect(struct Cyc_Absyn_Tvar*x,struct Cyc_List_List*l);
# 1351
void*Cyc_Absyn_rgneffect_name(struct Cyc_Absyn_RgnEffect*r);
# 1353
struct Cyc_List_List*Cyc_Absyn_rgneffect_caps(struct Cyc_Absyn_RgnEffect*r);
# 1400
extern const int Cyc_Absyn_noreentrant;
# 1404
void Cyc_Absyn_patch_stmt_exp(struct Cyc_Absyn_Stmt*s);
# 1410
int Cyc_Absyn_no_side_effects_exp(struct Cyc_Absyn_Exp*);
# 38 "warn.h"
void*Cyc_Warn_vimpos(struct _fat_ptr fmt,struct _fat_ptr ap);
# 46
void*Cyc_Warn_impos_loc(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap);struct Cyc_Warn_String_Warn_Warg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Warn_Exp_Warn_Warg_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Warn_Typ_Warn_Warg_struct{int tag;void*f1;};struct Cyc_Warn_Qvar_Warn_Warg_struct{int tag;struct _tuple1*f1;};struct Cyc_PP_Ppstate;struct Cyc_PP_Out;struct Cyc_PP_Doc;struct Cyc_Absynpp_Params{int expand_typedefs;int qvar_to_Cids;int add_cyc_prefix;int to_VC;int decls_first;int rewrite_temp_tvars;int print_all_tvars;int print_all_kinds;int print_all_effects;int print_using_stmts;int print_externC_stmts;int print_full_evars;int print_zeroterm;int generate_line_directives;int use_curr_namespace;struct Cyc_List_List*curr_namespace;};
# 62 "absynpp.h"
struct _fat_ptr Cyc_Absynpp_typ2string(void*);
# 67
struct _fat_ptr Cyc_Absynpp_exp2string(struct Cyc_Absyn_Exp*);
struct _fat_ptr Cyc_Absynpp_stmt2string(struct Cyc_Absyn_Stmt*);
struct _fat_ptr Cyc_Absynpp_qvar2string(struct _tuple1*);
# 29 "unify.h"
int Cyc_Unify_unify(void*,void*);struct _union_RelnOp_RConst{int tag;unsigned val;};struct _union_RelnOp_RVar{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RNumelts{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RType{int tag;void*val;};struct _union_RelnOp_RParam{int tag;unsigned val;};struct _union_RelnOp_RParamNumelts{int tag;unsigned val;};struct _union_RelnOp_RReturn{int tag;unsigned val;};union Cyc_Relations_RelnOp{struct _union_RelnOp_RConst RConst;struct _union_RelnOp_RVar RVar;struct _union_RelnOp_RNumelts RNumelts;struct _union_RelnOp_RType RType;struct _union_RelnOp_RParam RParam;struct _union_RelnOp_RParamNumelts RParamNumelts;struct _union_RelnOp_RReturn RReturn;};
# 50 "relations-ap.h"
enum Cyc_Relations_Relation{Cyc_Relations_Req =0U,Cyc_Relations_Rneq =1U,Cyc_Relations_Rlte =2U,Cyc_Relations_Rlt =3U};struct Cyc_Relations_Reln{union Cyc_Relations_RelnOp rop1;enum Cyc_Relations_Relation relation;union Cyc_Relations_RelnOp rop2;};struct Cyc_RgnOrder_RgnPO;extern char Cyc_Tcenv_Env_error[10U];struct Cyc_Tcenv_Env_error_exn_struct{char*tag;};struct Cyc_Tcenv_Genv{struct Cyc_Dict_Dict aggrdecls;struct Cyc_Dict_Dict datatypedecls;struct Cyc_Dict_Dict enumdecls;struct Cyc_Dict_Dict typedefs;struct Cyc_Dict_Dict ordinaries;};struct Cyc_Tcenv_Fenv;struct Cyc_Tcenv_Tenv{struct Cyc_List_List*ns;struct Cyc_Tcenv_Genv*ae;struct Cyc_Tcenv_Fenv*le;int allow_valueof: 1;int in_extern_c_include: 1;int in_tempest: 1;int tempest_generalize: 1;};
# 90 "tcenv.h"
enum Cyc_Tcenv_NewStatus{Cyc_Tcenv_NoneNew =0U,Cyc_Tcenv_InNew =1U,Cyc_Tcenv_InNewAggr =2U};
# 30 "tcutil.h"
void*Cyc_Tcutil_impos(struct _fat_ptr,struct _fat_ptr);
void Cyc_Tcutil_terr(unsigned,struct _fat_ptr,struct _fat_ptr);
void Cyc_Tcutil_warn(unsigned,struct _fat_ptr,struct _fat_ptr);
# 39
int Cyc_Tcutil_is_void_type(void*);
# 44
int Cyc_Tcutil_is_arithmetic_type(void*);
# 47
int Cyc_Tcutil_is_pointer_type(void*);
int Cyc_Tcutil_is_array_type(void*);
int Cyc_Tcutil_is_boxed(void*);
# 52
int Cyc_Tcutil_is_fat_ptr(void*);
int Cyc_Tcutil_is_zeroterm_pointer_type(void*);
# 57
int Cyc_Tcutil_is_fat_pointer_type(void*);
# 67
void*Cyc_Tcutil_pointer_elt_type(void*);
# 75
struct Cyc_Absyn_Exp*Cyc_Tcutil_get_bounds_exp(void*def,void*b);
# 80
int Cyc_Tcutil_is_fat_pointer_type_elt(void*t,void**elt_dest);
# 82
int Cyc_Tcutil_is_zero_pointer_type_elt(void*t,void**elt_type_dest);
# 87
struct Cyc_Absyn_Exp*Cyc_Tcutil_get_bounds_exp(void*def,void*b);
# 100
struct Cyc_Absyn_Exp*Cyc_Tcutil_deep_copy_exp(int preserve_types,struct Cyc_Absyn_Exp*);
# 103
int Cyc_Tcutil_kind_leq(struct Cyc_Absyn_Kind*,struct Cyc_Absyn_Kind*);
# 108
struct Cyc_Absyn_Kind*Cyc_Tcutil_tvar_kind(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Kind*def);
struct Cyc_Absyn_Kind*Cyc_Tcutil_type_kind(void*);
void*Cyc_Tcutil_compress(void*);
# 140
extern struct Cyc_Absyn_Kind Cyc_Tcutil_ak;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_bk;
# 150
extern struct Cyc_Absyn_Kind Cyc_Tcutil_tbk;
# 186
int Cyc_Tcutil_typecmp(void*,void*);
int Cyc_Tcutil_aggrfield_cmp(struct Cyc_Absyn_Aggrfield*,struct Cyc_Absyn_Aggrfield*);
# 189
void*Cyc_Tcutil_substitute(struct Cyc_List_List*,void*);
# 191
void*Cyc_Tcutil_rsubstitute(struct _RegionHandle*,struct Cyc_List_List*,void*);
# 226
struct Cyc_List_List*Cyc_Tcutil_resolve_aggregate_designators(struct _RegionHandle*,unsigned,struct Cyc_List_List*,enum Cyc_Absyn_AggrKind,struct Cyc_List_List*);
# 233
int Cyc_Tcutil_is_zero_ptr_deref(struct Cyc_Absyn_Exp*,void**ptr_type,int*is_fat,void**elt_type);struct _tuple14{struct Cyc_Absyn_Tqual f1;void*f2;};
# 281
void*Cyc_Tcutil_snd_tqt(struct _tuple14*);
# 299
struct Cyc_List_List*Cyc_Tcutil_filter_nulls(struct Cyc_List_List*);
# 310
int Cyc_Tcutil_force_type2bool(int desired,void*);
# 331
int Cyc_Tcutil_is_tagged_union_and_needs_check(struct Cyc_Absyn_Exp*e);struct _tuple15{unsigned f1;int f2;};
# 28 "evexp.h"
struct _tuple15 Cyc_Evexp_eval_const_uint_exp(struct Cyc_Absyn_Exp*e);
# 32
int Cyc_Evexp_c_can_eval(struct Cyc_Absyn_Exp*e);struct Cyc_Hashtable_Table;
# 52 "hashtable.h"
void*Cyc_Hashtable_lookup(struct Cyc_Hashtable_Table*t,void*key);struct Cyc_JumpAnalysis_Jump_Anal_Result{struct Cyc_Hashtable_Table*pop_tables;struct Cyc_Hashtable_Table*succ_tables;struct Cyc_Hashtable_Table*pat_pop_tables;};extern char Cyc_InsertChecks_FatBound[9U];struct Cyc_InsertChecks_FatBound_Absyn_AbsynAnnot_struct{char*tag;};extern char Cyc_InsertChecks_NoCheck[8U];struct Cyc_InsertChecks_NoCheck_Absyn_AbsynAnnot_struct{char*tag;};extern char Cyc_InsertChecks_NullAndFatBound[16U];struct Cyc_InsertChecks_NullAndFatBound_Absyn_AbsynAnnot_struct{char*tag;};extern char Cyc_InsertChecks_NullAndThinBound[17U];struct Cyc_InsertChecks_NullAndThinBound_Absyn_AbsynAnnot_struct{char*tag;struct Cyc_Absyn_Exp*f1;};extern char Cyc_InsertChecks_NullOnly[9U];struct Cyc_InsertChecks_NullOnly_Absyn_AbsynAnnot_struct{char*tag;};extern char Cyc_InsertChecks_ThinBound[10U];struct Cyc_InsertChecks_ThinBound_Absyn_AbsynAnnot_struct{char*tag;struct Cyc_Absyn_Exp*f1;};
# 35 "insert_checks.h"
extern struct Cyc_InsertChecks_NoCheck_Absyn_AbsynAnnot_struct Cyc_InsertChecks_NoCheck_val;
# 37
extern struct Cyc_InsertChecks_NullAndFatBound_Absyn_AbsynAnnot_struct Cyc_InsertChecks_NullAndFatBound_val;
# 38 "toc.h"
void*Cyc_Toc_typ_to_c(void*);
# 40
struct _tuple1*Cyc_Toc_temp_var();
extern struct _fat_ptr Cyc_Toc_globals;extern char Cyc_Toc_Dest[5U];struct Cyc_Toc_Dest_Absyn_AbsynAnnot_struct{char*tag;struct Cyc_Absyn_Exp*f1;};
# 46
int Cyc_Toc_is_zero(struct Cyc_Absyn_Exp*e);
int Cyc_Toc_do_null_check(struct Cyc_Absyn_Exp*e);
int Cyc_Toc_is_tagged_union_project(struct Cyc_Absyn_Exp*e,int*f_tag,void**tagged_member_type,int clear_read);
# 52
void Cyc_Toc_init_toc_state();struct Cyc_Tcpat_TcPatResult{struct _tuple0*tvars_and_bounds_opt;struct Cyc_List_List*patvars;};struct Cyc_Tcpat_WhereTest_Tcpat_PatTest_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Tcpat_EqNull_Tcpat_PatTest_struct{int tag;};struct Cyc_Tcpat_NeqNull_Tcpat_PatTest_struct{int tag;};struct Cyc_Tcpat_EqEnum_Tcpat_PatTest_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Tcpat_EqAnonEnum_Tcpat_PatTest_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Tcpat_EqFloat_Tcpat_PatTest_struct{int tag;struct _fat_ptr f1;int f2;};struct Cyc_Tcpat_EqConst_Tcpat_PatTest_struct{int tag;unsigned f1;};struct Cyc_Tcpat_EqDatatypeTag_Tcpat_PatTest_struct{int tag;int f1;struct Cyc_Absyn_Datatypedecl*f2;struct Cyc_Absyn_Datatypefield*f3;};struct Cyc_Tcpat_EqTaggedUnion_Tcpat_PatTest_struct{int tag;struct _fat_ptr*f1;int f2;};struct Cyc_Tcpat_EqExtensibleDatatype_Tcpat_PatTest_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;};struct Cyc_Tcpat_Dummy_Tcpat_Access_struct{int tag;};struct Cyc_Tcpat_Deref_Tcpat_Access_struct{int tag;};struct Cyc_Tcpat_TupleField_Tcpat_Access_struct{int tag;unsigned f1;};struct Cyc_Tcpat_DatatypeField_Tcpat_Access_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;unsigned f3;};struct Cyc_Tcpat_AggrField_Tcpat_Access_struct{int tag;int f1;struct _fat_ptr*f2;};struct _union_PatOrWhere_pattern{int tag;struct Cyc_Absyn_Pat*val;};struct _union_PatOrWhere_where_clause{int tag;struct Cyc_Absyn_Exp*val;};union Cyc_Tcpat_PatOrWhere{struct _union_PatOrWhere_pattern pattern;struct _union_PatOrWhere_where_clause where_clause;};struct Cyc_Tcpat_PathNode{union Cyc_Tcpat_PatOrWhere orig_pat;void*access;};struct Cyc_Tcpat_Rhs{int used;unsigned pat_loc;struct Cyc_Absyn_Stmt*rhs;};struct Cyc_Tcpat_Failure_Tcpat_Decision_struct{int tag;void*f1;};struct Cyc_Tcpat_Success_Tcpat_Decision_struct{int tag;struct Cyc_Tcpat_Rhs*f1;};struct Cyc_Tcpat_SwitchDec_Tcpat_Decision_struct{int tag;struct Cyc_List_List*f1;struct Cyc_List_List*f2;void*f3;};
# 28 "ioeffect.h"
struct Cyc_List_List*Cyc_IOEffect_summarize_all(unsigned loc,struct Cyc_List_List*f);
# 67 "toc.cyc"
extern int Cyc_noexpand_r;char Cyc_Toc_Dest[5U]="Dest";
# 82 "toc.cyc"
static void*Cyc_Toc_unimp(struct _fat_ptr fmt,struct _fat_ptr ap){
# 84
({void*(*_tmp0)(struct _fat_ptr fmt,struct _fat_ptr ap)=Cyc_Warn_vimpos;struct _fat_ptr _tmp1=(struct _fat_ptr)({({struct _fat_ptr _tmpC9F=({const char*_tmp2="Toc (unimplemented): ";_tag_fat(_tmp2,sizeof(char),22U);});Cyc_strconcat(_tmpC9F,(struct _fat_ptr)fmt);});});struct _fat_ptr _tmp3=ap;_tmp0(_tmp1,_tmp3);});}
# 86
static void*Cyc_Toc_toc_impos(struct _fat_ptr fmt,struct _fat_ptr ap){
# 88
({void*(*_tmp5)(struct _fat_ptr fmt,struct _fat_ptr ap)=Cyc_Warn_vimpos;struct _fat_ptr _tmp6=(struct _fat_ptr)({({struct _fat_ptr _tmpCA0=({const char*_tmp7="Toc: ";_tag_fat(_tmp7,sizeof(char),6U);});Cyc_strconcat(_tmpCA0,(struct _fat_ptr)fmt);});});struct _fat_ptr _tmp8=ap;_tmp5(_tmp6,_tmp8);});}
# 86
struct _fat_ptr Cyc_Toc_globals={(void*)0,(void*)0,(void*)(0 + 0)};
# 94
static struct Cyc_Hashtable_Table**Cyc_Toc_gpop_tables=0;
static struct Cyc_Hashtable_Table**Cyc_Toc_fn_pop_table=0;
static int Cyc_Toc_tuple_type_counter=0;
static int Cyc_Toc_temp_var_counter=0;
static int Cyc_Toc_fresh_label_counter=0;
static unsigned Cyc_Toc_xrgn_count=0U;
static struct Cyc_List_List*Cyc_Toc_xrgn_map=0;
static struct Cyc_List_List*Cyc_Toc_xrgn_effect=0;
# 106
static struct Cyc_List_List*Cyc_Toc_result_decls=0;
# 108
static int Cyc_Toc_get_npop(struct Cyc_Absyn_Stmt*s){
return({({int(*_tmpCA2)(struct Cyc_Hashtable_Table*t,struct Cyc_Absyn_Stmt*key)=({int(*_tmpA)(struct Cyc_Hashtable_Table*t,struct Cyc_Absyn_Stmt*key)=(int(*)(struct Cyc_Hashtable_Table*t,struct Cyc_Absyn_Stmt*key))Cyc_Hashtable_lookup;_tmpA;});struct Cyc_Hashtable_Table*_tmpCA1=*((struct Cyc_Hashtable_Table**)_check_null(Cyc_Toc_fn_pop_table));_tmpCA2(_tmpCA1,s);});});}struct Cyc_Toc_TocState{struct Cyc_List_List**tuple_types;struct Cyc_List_List**anon_aggr_types;struct Cyc_Dict_Dict*aggrs_so_far;struct Cyc_List_List**abs_struct_types;struct Cyc_Set_Set**datatypes_so_far;struct Cyc_Dict_Dict*xdatatypes_so_far;struct Cyc_Dict_Dict*qvar_tags;struct Cyc_Xarray_Xarray*temp_labels;};struct _tuple16{struct _tuple1*f1;struct _tuple1*f2;};
# 134
static int Cyc_Toc_qvar_tag_cmp(struct _tuple16*x,struct _tuple16*y){
int i=({Cyc_Absyn_qvar_cmp((*x).f1,(*y).f1);});
if(i != 0)return i;return({Cyc_Absyn_qvar_cmp((*x).f2,(*y).f2);});}
# 141
static struct Cyc_Toc_TocState*Cyc_Toc_empty_toc_state(struct _RegionHandle*d){
return({struct Cyc_Toc_TocState*_tmp18=_region_malloc(d,sizeof(*_tmp18));
({struct Cyc_List_List**_tmpCB2=({struct Cyc_List_List**_tmpD=_region_malloc(d,sizeof(*_tmpD));*_tmpD=0;_tmpD;});_tmp18->tuple_types=_tmpCB2;}),({
struct Cyc_List_List**_tmpCB1=({struct Cyc_List_List**_tmpE=_region_malloc(d,sizeof(*_tmpE));*_tmpE=0;_tmpE;});_tmp18->anon_aggr_types=_tmpCB1;}),({
struct Cyc_Dict_Dict*_tmpCB0=({struct Cyc_Dict_Dict*_tmp10=_region_malloc(d,sizeof(*_tmp10));({struct Cyc_Dict_Dict _tmpCAF=({({struct Cyc_Dict_Dict(*_tmpCAE)(struct _RegionHandle*,int(*cmp)(struct _tuple1*,struct _tuple1*))=({struct Cyc_Dict_Dict(*_tmpF)(struct _RegionHandle*,int(*cmp)(struct _tuple1*,struct _tuple1*))=(struct Cyc_Dict_Dict(*)(struct _RegionHandle*,int(*cmp)(struct _tuple1*,struct _tuple1*)))Cyc_Dict_rempty;_tmpF;});_tmpCAE(d,Cyc_Absyn_qvar_cmp);});});*_tmp10=_tmpCAF;});_tmp10;});_tmp18->aggrs_so_far=_tmpCB0;}),({
struct Cyc_List_List**_tmpCAD=({struct Cyc_List_List**_tmp11=_region_malloc(d,sizeof(*_tmp11));*_tmp11=0;_tmp11;});_tmp18->abs_struct_types=_tmpCAD;}),({
struct Cyc_Set_Set**_tmpCAC=({struct Cyc_Set_Set**_tmp13=_region_malloc(d,sizeof(*_tmp13));({struct Cyc_Set_Set*_tmpCAB=({({struct Cyc_Set_Set*(*_tmpCAA)(struct _RegionHandle*r,int(*cmp)(struct _tuple1*,struct _tuple1*))=({struct Cyc_Set_Set*(*_tmp12)(struct _RegionHandle*r,int(*cmp)(struct _tuple1*,struct _tuple1*))=(struct Cyc_Set_Set*(*)(struct _RegionHandle*r,int(*cmp)(struct _tuple1*,struct _tuple1*)))Cyc_Set_rempty;_tmp12;});_tmpCAA(d,Cyc_Absyn_qvar_cmp);});});*_tmp13=_tmpCAB;});_tmp13;});_tmp18->datatypes_so_far=_tmpCAC;}),({
struct Cyc_Dict_Dict*_tmpCA9=({struct Cyc_Dict_Dict*_tmp15=_region_malloc(d,sizeof(*_tmp15));({struct Cyc_Dict_Dict _tmpCA8=({({struct Cyc_Dict_Dict(*_tmpCA7)(struct _RegionHandle*,int(*cmp)(struct _tuple1*,struct _tuple1*))=({struct Cyc_Dict_Dict(*_tmp14)(struct _RegionHandle*,int(*cmp)(struct _tuple1*,struct _tuple1*))=(struct Cyc_Dict_Dict(*)(struct _RegionHandle*,int(*cmp)(struct _tuple1*,struct _tuple1*)))Cyc_Dict_rempty;_tmp14;});_tmpCA7(d,Cyc_Absyn_qvar_cmp);});});*_tmp15=_tmpCA8;});_tmp15;});_tmp18->xdatatypes_so_far=_tmpCA9;}),({
struct Cyc_Dict_Dict*_tmpCA6=({struct Cyc_Dict_Dict*_tmp17=_region_malloc(d,sizeof(*_tmp17));({struct Cyc_Dict_Dict _tmpCA5=({({struct Cyc_Dict_Dict(*_tmpCA4)(struct _RegionHandle*,int(*cmp)(struct _tuple16*,struct _tuple16*))=({struct Cyc_Dict_Dict(*_tmp16)(struct _RegionHandle*,int(*cmp)(struct _tuple16*,struct _tuple16*))=(struct Cyc_Dict_Dict(*)(struct _RegionHandle*,int(*cmp)(struct _tuple16*,struct _tuple16*)))Cyc_Dict_rempty;_tmp16;});_tmpCA4(d,Cyc_Toc_qvar_tag_cmp);});});*_tmp17=_tmpCA5;});_tmp17;});_tmp18->qvar_tags=_tmpCA6;}),({
struct Cyc_Xarray_Xarray*_tmpCA3=({Cyc_Xarray_rcreate_empty(d);});_tmp18->temp_labels=_tmpCA3;});_tmp18;});}struct Cyc_Toc_TocStateWrap{struct Cyc_Core_DynamicRegion*dyn;struct Cyc_Toc_TocState*state;};
# 160
static struct Cyc_Toc_TocStateWrap*Cyc_Toc_toc_state=0;struct _tuple17{struct Cyc_Toc_TocState*f1;void*f2;};
# 166
static void*Cyc_Toc_use_toc_state(void*arg,void*(*f)(struct _RegionHandle*,struct _tuple17*)){
# 169
struct Cyc_Toc_TocStateWrap*ts=0;
({struct Cyc_Toc_TocStateWrap*_tmp1A=ts;struct Cyc_Toc_TocStateWrap*_tmp1B=Cyc_Toc_toc_state;ts=_tmp1B;Cyc_Toc_toc_state=_tmp1A;});{
struct Cyc_Toc_TocStateWrap _stmttmp0=*((struct Cyc_Toc_TocStateWrap*)_check_null(ts));struct Cyc_Toc_TocState*_tmp1D;struct Cyc_Core_DynamicRegion*_tmp1C;_LL1: _tmp1C=_stmttmp0.dyn;_tmp1D=_stmttmp0.state;_LL2: {struct Cyc_Core_DynamicRegion*dyn=_tmp1C;struct Cyc_Toc_TocState*s=_tmp1D;
struct _fat_ptr dyn2=({Cyc_Core_alias_refptr(_tag_fat(dyn,sizeof(struct Cyc_Core_DynamicRegion),1U));});
({struct Cyc_Toc_TocStateWrap _tmpCB3=({struct Cyc_Toc_TocStateWrap _tmpC59;_tmpC59.dyn=dyn,_tmpC59.state=s;_tmpC59;});*ts=_tmpCB3;});
({struct Cyc_Toc_TocStateWrap*_tmp1E=ts;struct Cyc_Toc_TocStateWrap*_tmp1F=Cyc_Toc_toc_state;ts=_tmp1F;Cyc_Toc_toc_state=_tmp1E;});{
void*res;
{struct _RegionHandle*h=&((struct Cyc_Core_DynamicRegion*)_check_null(_untag_fat_ptr(dyn2,sizeof(struct Cyc_Core_DynamicRegion),1U)))->h;
{struct _tuple17 env=({struct _tuple17 _tmpC5A;_tmpC5A.f1=s,_tmpC5A.f2=arg;_tmpC5A;});
res=({f(h,& env);});}
# 177
;}
# 179
({Cyc_Core_free_rckey((struct Cyc_Core_DynamicRegion*)_untag_fat_ptr(dyn2,sizeof(struct Cyc_Core_DynamicRegion),1U));});
return res;}}}}struct _tuple18{struct _tuple1*f1;void*(*f2)(struct _tuple1*);};struct _tuple19{struct Cyc_Toc_TocState*f1;struct _tuple18*f2;};struct _tuple20{struct Cyc_Absyn_Aggrdecl*f1;void*f2;};
# 183
static void*Cyc_Toc_aggrdecl_type_body(struct _RegionHandle*d,struct _tuple19*env){
# 186
struct _tuple19 _stmttmp1=*env;void*(*_tmp24)(struct _tuple1*);struct _tuple1*_tmp23;struct Cyc_Toc_TocState*_tmp22;_LL1: _tmp22=_stmttmp1.f1;_tmp23=(_stmttmp1.f2)->f1;_tmp24=(_stmttmp1.f2)->f2;_LL2: {struct Cyc_Toc_TocState*s=_tmp22;struct _tuple1*q=_tmp23;void*(*type_maker)(struct _tuple1*)=_tmp24;
struct _tuple20**v=({({struct _tuple20**(*_tmpCB5)(struct Cyc_Dict_Dict d,struct _tuple1*k)=({struct _tuple20**(*_tmp25)(struct Cyc_Dict_Dict d,struct _tuple1*k)=(struct _tuple20**(*)(struct Cyc_Dict_Dict d,struct _tuple1*k))Cyc_Dict_lookup_opt;_tmp25;});struct Cyc_Dict_Dict _tmpCB4=*s->aggrs_so_far;_tmpCB5(_tmpCB4,q);});});
return v == 0?({type_maker(q);}):(*(*v)).f2;}}
# 191
static void*Cyc_Toc_aggrdecl_type(struct _tuple1*q,void*(*type_maker)(struct _tuple1*)){
struct _tuple18 env=({struct _tuple18 _tmpC5B;_tmpC5B.f1=q,_tmpC5B.f2=type_maker;_tmpC5B;});
return({({void*(*_tmp27)(struct _tuple18*arg,void*(*f)(struct _RegionHandle*,struct _tuple19*))=(void*(*)(struct _tuple18*arg,void*(*f)(struct _RegionHandle*,struct _tuple19*)))Cyc_Toc_use_toc_state;_tmp27;})(& env,Cyc_Toc_aggrdecl_type_body);});}static char _tmp29[5U]="curr";
# 191
static struct _fat_ptr Cyc_Toc_curr_string={_tmp29,_tmp29,_tmp29 + 5U};
# 204 "toc.cyc"
static struct _fat_ptr*Cyc_Toc_curr_sp=& Cyc_Toc_curr_string;static char _tmp2A[4U]="tag";
static struct _fat_ptr Cyc_Toc_tag_string={_tmp2A,_tmp2A,_tmp2A + 4U};static struct _fat_ptr*Cyc_Toc_tag_sp=& Cyc_Toc_tag_string;static char _tmp2B[4U]="val";
static struct _fat_ptr Cyc_Toc_val_string={_tmp2B,_tmp2B,_tmp2B + 4U};static struct _fat_ptr*Cyc_Toc_val_sp=& Cyc_Toc_val_string;static char _tmp2C[14U]="_handler_cons";
static struct _fat_ptr Cyc_Toc__handler_cons_string={_tmp2C,_tmp2C,_tmp2C + 14U};static struct _fat_ptr*Cyc_Toc__handler_cons_sp=& Cyc_Toc__handler_cons_string;static char _tmp2D[8U]="handler";
static struct _fat_ptr Cyc_Toc_handler_string={_tmp2D,_tmp2D,_tmp2D + 8U};static struct _fat_ptr*Cyc_Toc_handler_sp=& Cyc_Toc_handler_string;static char _tmp2E[14U]="_RegionHandle";
static struct _fat_ptr Cyc_Toc__RegionHandle_string={_tmp2E,_tmp2E,_tmp2E + 14U};static struct _fat_ptr*Cyc_Toc__RegionHandle_sp=& Cyc_Toc__RegionHandle_string;static char _tmp2F[15U]="_XRegionHandle";
static struct _fat_ptr Cyc_Toc__XRegionHandle_string={_tmp2F,_tmp2F,_tmp2F + 15U};static struct _fat_ptr*Cyc_Toc__XRegionHandle_sp=& Cyc_Toc__XRegionHandle_string;static char _tmp30[13U]="_XTreeHandle";
static struct _fat_ptr Cyc_Toc__XTreeHandle_string={_tmp30,_tmp30,_tmp30 + 13U};static struct _fat_ptr*Cyc_Toc__XTreeHandle_sp=& Cyc_Toc__XTreeHandle_string;static char _tmp31[7U]="_throw";
# 225 "toc.cyc"
static struct _fat_ptr Cyc_Toc__throw_str={_tmp31,_tmp31,_tmp31 + 7U};static struct _tuple1 Cyc_Toc__throw_pr={{.Loc_n={4,0}},& Cyc_Toc__throw_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__throw_bnd={0U,& Cyc_Toc__throw_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__throw_re={1U,(void*)& Cyc_Toc__throw_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__throw_ev={0,(void*)& Cyc_Toc__throw_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__throw_e=& Cyc_Toc__throw_ev;static char _tmp32[7U]="setjmp";
static struct _fat_ptr Cyc_Toc_setjmp_str={_tmp32,_tmp32,_tmp32 + 7U};static struct _tuple1 Cyc_Toc_setjmp_pr={{.Loc_n={4,0}},& Cyc_Toc_setjmp_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc_setjmp_bnd={0U,& Cyc_Toc_setjmp_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc_setjmp_re={1U,(void*)& Cyc_Toc_setjmp_bnd};static struct Cyc_Absyn_Exp Cyc_Toc_setjmp_ev={0,(void*)& Cyc_Toc_setjmp_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc_setjmp_e=& Cyc_Toc_setjmp_ev;static char _tmp33[14U]="_push_handler";
static struct _fat_ptr Cyc_Toc__push_handler_str={_tmp33,_tmp33,_tmp33 + 14U};static struct _tuple1 Cyc_Toc__push_handler_pr={{.Loc_n={4,0}},& Cyc_Toc__push_handler_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__push_handler_bnd={0U,& Cyc_Toc__push_handler_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__push_handler_re={1U,(void*)& Cyc_Toc__push_handler_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__push_handler_ev={0,(void*)& Cyc_Toc__push_handler_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__push_handler_e=& Cyc_Toc__push_handler_ev;static char _tmp34[13U]="_pop_handler";
static struct _fat_ptr Cyc_Toc__pop_handler_str={_tmp34,_tmp34,_tmp34 + 13U};static struct _tuple1 Cyc_Toc__pop_handler_pr={{.Loc_n={4,0}},& Cyc_Toc__pop_handler_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__pop_handler_bnd={0U,& Cyc_Toc__pop_handler_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__pop_handler_re={1U,(void*)& Cyc_Toc__pop_handler_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__pop_handler_ev={0,(void*)& Cyc_Toc__pop_handler_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__pop_handler_e=& Cyc_Toc__pop_handler_ev;static char _tmp35[12U]="_exn_thrown";
static struct _fat_ptr Cyc_Toc__exn_thrown_str={_tmp35,_tmp35,_tmp35 + 12U};static struct _tuple1 Cyc_Toc__exn_thrown_pr={{.Loc_n={4,0}},& Cyc_Toc__exn_thrown_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__exn_thrown_bnd={0U,& Cyc_Toc__exn_thrown_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__exn_thrown_re={1U,(void*)& Cyc_Toc__exn_thrown_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__exn_thrown_ev={0,(void*)& Cyc_Toc__exn_thrown_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__exn_thrown_e=& Cyc_Toc__exn_thrown_ev;static char _tmp36[14U]="_npop_handler";
static struct _fat_ptr Cyc_Toc__npop_handler_str={_tmp36,_tmp36,_tmp36 + 14U};static struct _tuple1 Cyc_Toc__npop_handler_pr={{.Loc_n={4,0}},& Cyc_Toc__npop_handler_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__npop_handler_bnd={0U,& Cyc_Toc__npop_handler_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__npop_handler_re={1U,(void*)& Cyc_Toc__npop_handler_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__npop_handler_ev={0,(void*)& Cyc_Toc__npop_handler_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__npop_handler_e=& Cyc_Toc__npop_handler_ev;static char _tmp37[12U]="_check_null";
static struct _fat_ptr Cyc_Toc__check_null_str={_tmp37,_tmp37,_tmp37 + 12U};static struct _tuple1 Cyc_Toc__check_null_pr={{.Loc_n={4,0}},& Cyc_Toc__check_null_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__check_null_bnd={0U,& Cyc_Toc__check_null_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__check_null_re={1U,(void*)& Cyc_Toc__check_null_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__check_null_ev={0,(void*)& Cyc_Toc__check_null_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__check_null_e=& Cyc_Toc__check_null_ev;static char _tmp38[28U]="_check_known_subscript_null";
static struct _fat_ptr Cyc_Toc__check_known_subscript_null_str={_tmp38,_tmp38,_tmp38 + 28U};static struct _tuple1 Cyc_Toc__check_known_subscript_null_pr={{.Loc_n={4,0}},& Cyc_Toc__check_known_subscript_null_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__check_known_subscript_null_bnd={0U,& Cyc_Toc__check_known_subscript_null_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__check_known_subscript_null_re={1U,(void*)& Cyc_Toc__check_known_subscript_null_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__check_known_subscript_null_ev={0,(void*)& Cyc_Toc__check_known_subscript_null_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__check_known_subscript_null_e=& Cyc_Toc__check_known_subscript_null_ev;static char _tmp39[31U]="_check_known_subscript_notnull";
static struct _fat_ptr Cyc_Toc__check_known_subscript_notnull_str={_tmp39,_tmp39,_tmp39 + 31U};static struct _tuple1 Cyc_Toc__check_known_subscript_notnull_pr={{.Loc_n={4,0}},& Cyc_Toc__check_known_subscript_notnull_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__check_known_subscript_notnull_bnd={0U,& Cyc_Toc__check_known_subscript_notnull_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__check_known_subscript_notnull_re={1U,(void*)& Cyc_Toc__check_known_subscript_notnull_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__check_known_subscript_notnull_ev={0,(void*)& Cyc_Toc__check_known_subscript_notnull_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__check_known_subscript_notnull_e=& Cyc_Toc__check_known_subscript_notnull_ev;static char _tmp3A[21U]="_check_fat_subscript";
static struct _fat_ptr Cyc_Toc__check_fat_subscript_str={_tmp3A,_tmp3A,_tmp3A + 21U};static struct _tuple1 Cyc_Toc__check_fat_subscript_pr={{.Loc_n={4,0}},& Cyc_Toc__check_fat_subscript_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__check_fat_subscript_bnd={0U,& Cyc_Toc__check_fat_subscript_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__check_fat_subscript_re={1U,(void*)& Cyc_Toc__check_fat_subscript_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__check_fat_subscript_ev={0,(void*)& Cyc_Toc__check_fat_subscript_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__check_fat_subscript_e=& Cyc_Toc__check_fat_subscript_ev;static char _tmp3B[9U]="_fat_ptr";
static struct _fat_ptr Cyc_Toc__fat_ptr_str={_tmp3B,_tmp3B,_tmp3B + 9U};static struct _tuple1 Cyc_Toc__fat_ptr_pr={{.Loc_n={4,0}},& Cyc_Toc__fat_ptr_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__fat_ptr_bnd={0U,& Cyc_Toc__fat_ptr_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__fat_ptr_re={1U,(void*)& Cyc_Toc__fat_ptr_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__fat_ptr_ev={0,(void*)& Cyc_Toc__fat_ptr_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__fat_ptr_e=& Cyc_Toc__fat_ptr_ev;static char _tmp3C[9U]="_tag_fat";
static struct _fat_ptr Cyc_Toc__tag_fat_str={_tmp3C,_tmp3C,_tmp3C + 9U};static struct _tuple1 Cyc_Toc__tag_fat_pr={{.Loc_n={4,0}},& Cyc_Toc__tag_fat_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__tag_fat_bnd={0U,& Cyc_Toc__tag_fat_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__tag_fat_re={1U,(void*)& Cyc_Toc__tag_fat_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__tag_fat_ev={0,(void*)& Cyc_Toc__tag_fat_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__tag_fat_e=& Cyc_Toc__tag_fat_ev;static char _tmp3D[15U]="_untag_fat_ptr";
static struct _fat_ptr Cyc_Toc__untag_fat_ptr_str={_tmp3D,_tmp3D,_tmp3D + 15U};static struct _tuple1 Cyc_Toc__untag_fat_ptr_pr={{.Loc_n={4,0}},& Cyc_Toc__untag_fat_ptr_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__untag_fat_ptr_bnd={0U,& Cyc_Toc__untag_fat_ptr_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__untag_fat_ptr_re={1U,(void*)& Cyc_Toc__untag_fat_ptr_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__untag_fat_ptr_ev={0,(void*)& Cyc_Toc__untag_fat_ptr_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__untag_fat_ptr_e=& Cyc_Toc__untag_fat_ptr_ev;static char _tmp3E[14U]="_get_fat_size";
static struct _fat_ptr Cyc_Toc__get_fat_size_str={_tmp3E,_tmp3E,_tmp3E + 14U};static struct _tuple1 Cyc_Toc__get_fat_size_pr={{.Loc_n={4,0}},& Cyc_Toc__get_fat_size_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__get_fat_size_bnd={0U,& Cyc_Toc__get_fat_size_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__get_fat_size_re={1U,(void*)& Cyc_Toc__get_fat_size_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__get_fat_size_ev={0,(void*)& Cyc_Toc__get_fat_size_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__get_fat_size_e=& Cyc_Toc__get_fat_size_ev;static char _tmp3F[19U]="_get_zero_arr_size";
static struct _fat_ptr Cyc_Toc__get_zero_arr_size_str={_tmp3F,_tmp3F,_tmp3F + 19U};static struct _tuple1 Cyc_Toc__get_zero_arr_size_pr={{.Loc_n={4,0}},& Cyc_Toc__get_zero_arr_size_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__get_zero_arr_size_bnd={0U,& Cyc_Toc__get_zero_arr_size_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__get_zero_arr_size_re={1U,(void*)& Cyc_Toc__get_zero_arr_size_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__get_zero_arr_size_ev={0,(void*)& Cyc_Toc__get_zero_arr_size_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__get_zero_arr_size_e=& Cyc_Toc__get_zero_arr_size_ev;static char _tmp40[24U]="_get_zero_arr_size_char";
static struct _fat_ptr Cyc_Toc__get_zero_arr_size_char_str={_tmp40,_tmp40,_tmp40 + 24U};static struct _tuple1 Cyc_Toc__get_zero_arr_size_char_pr={{.Loc_n={4,0}},& Cyc_Toc__get_zero_arr_size_char_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__get_zero_arr_size_char_bnd={0U,& Cyc_Toc__get_zero_arr_size_char_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__get_zero_arr_size_char_re={1U,(void*)& Cyc_Toc__get_zero_arr_size_char_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__get_zero_arr_size_char_ev={0,(void*)& Cyc_Toc__get_zero_arr_size_char_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__get_zero_arr_size_char_e=& Cyc_Toc__get_zero_arr_size_char_ev;static char _tmp41[25U]="_get_zero_arr_size_short";
static struct _fat_ptr Cyc_Toc__get_zero_arr_size_short_str={_tmp41,_tmp41,_tmp41 + 25U};static struct _tuple1 Cyc_Toc__get_zero_arr_size_short_pr={{.Loc_n={4,0}},& Cyc_Toc__get_zero_arr_size_short_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__get_zero_arr_size_short_bnd={0U,& Cyc_Toc__get_zero_arr_size_short_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__get_zero_arr_size_short_re={1U,(void*)& Cyc_Toc__get_zero_arr_size_short_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__get_zero_arr_size_short_ev={0,(void*)& Cyc_Toc__get_zero_arr_size_short_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__get_zero_arr_size_short_e=& Cyc_Toc__get_zero_arr_size_short_ev;static char _tmp42[23U]="_get_zero_arr_size_int";
static struct _fat_ptr Cyc_Toc__get_zero_arr_size_int_str={_tmp42,_tmp42,_tmp42 + 23U};static struct _tuple1 Cyc_Toc__get_zero_arr_size_int_pr={{.Loc_n={4,0}},& Cyc_Toc__get_zero_arr_size_int_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__get_zero_arr_size_int_bnd={0U,& Cyc_Toc__get_zero_arr_size_int_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__get_zero_arr_size_int_re={1U,(void*)& Cyc_Toc__get_zero_arr_size_int_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__get_zero_arr_size_int_ev={0,(void*)& Cyc_Toc__get_zero_arr_size_int_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__get_zero_arr_size_int_e=& Cyc_Toc__get_zero_arr_size_int_ev;static char _tmp43[25U]="_get_zero_arr_size_float";
static struct _fat_ptr Cyc_Toc__get_zero_arr_size_float_str={_tmp43,_tmp43,_tmp43 + 25U};static struct _tuple1 Cyc_Toc__get_zero_arr_size_float_pr={{.Loc_n={4,0}},& Cyc_Toc__get_zero_arr_size_float_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__get_zero_arr_size_float_bnd={0U,& Cyc_Toc__get_zero_arr_size_float_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__get_zero_arr_size_float_re={1U,(void*)& Cyc_Toc__get_zero_arr_size_float_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__get_zero_arr_size_float_ev={0,(void*)& Cyc_Toc__get_zero_arr_size_float_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__get_zero_arr_size_float_e=& Cyc_Toc__get_zero_arr_size_float_ev;static char _tmp44[26U]="_get_zero_arr_size_double";
static struct _fat_ptr Cyc_Toc__get_zero_arr_size_double_str={_tmp44,_tmp44,_tmp44 + 26U};static struct _tuple1 Cyc_Toc__get_zero_arr_size_double_pr={{.Loc_n={4,0}},& Cyc_Toc__get_zero_arr_size_double_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__get_zero_arr_size_double_bnd={0U,& Cyc_Toc__get_zero_arr_size_double_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__get_zero_arr_size_double_re={1U,(void*)& Cyc_Toc__get_zero_arr_size_double_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__get_zero_arr_size_double_ev={0,(void*)& Cyc_Toc__get_zero_arr_size_double_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__get_zero_arr_size_double_e=& Cyc_Toc__get_zero_arr_size_double_ev;static char _tmp45[30U]="_get_zero_arr_size_longdouble";
static struct _fat_ptr Cyc_Toc__get_zero_arr_size_longdouble_str={_tmp45,_tmp45,_tmp45 + 30U};static struct _tuple1 Cyc_Toc__get_zero_arr_size_longdouble_pr={{.Loc_n={4,0}},& Cyc_Toc__get_zero_arr_size_longdouble_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__get_zero_arr_size_longdouble_bnd={0U,& Cyc_Toc__get_zero_arr_size_longdouble_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__get_zero_arr_size_longdouble_re={1U,(void*)& Cyc_Toc__get_zero_arr_size_longdouble_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__get_zero_arr_size_longdouble_ev={0,(void*)& Cyc_Toc__get_zero_arr_size_longdouble_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__get_zero_arr_size_longdouble_e=& Cyc_Toc__get_zero_arr_size_longdouble_ev;static char _tmp46[28U]="_get_zero_arr_size_voidstar";
static struct _fat_ptr Cyc_Toc__get_zero_arr_size_voidstar_str={_tmp46,_tmp46,_tmp46 + 28U};static struct _tuple1 Cyc_Toc__get_zero_arr_size_voidstar_pr={{.Loc_n={4,0}},& Cyc_Toc__get_zero_arr_size_voidstar_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__get_zero_arr_size_voidstar_bnd={0U,& Cyc_Toc__get_zero_arr_size_voidstar_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__get_zero_arr_size_voidstar_re={1U,(void*)& Cyc_Toc__get_zero_arr_size_voidstar_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__get_zero_arr_size_voidstar_ev={0,(void*)& Cyc_Toc__get_zero_arr_size_voidstar_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__get_zero_arr_size_voidstar_e=& Cyc_Toc__get_zero_arr_size_voidstar_ev;static char _tmp47[14U]="_fat_ptr_plus";
static struct _fat_ptr Cyc_Toc__fat_ptr_plus_str={_tmp47,_tmp47,_tmp47 + 14U};static struct _tuple1 Cyc_Toc__fat_ptr_plus_pr={{.Loc_n={4,0}},& Cyc_Toc__fat_ptr_plus_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__fat_ptr_plus_bnd={0U,& Cyc_Toc__fat_ptr_plus_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__fat_ptr_plus_re={1U,(void*)& Cyc_Toc__fat_ptr_plus_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__fat_ptr_plus_ev={0,(void*)& Cyc_Toc__fat_ptr_plus_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__fat_ptr_plus_e=& Cyc_Toc__fat_ptr_plus_ev;static char _tmp48[15U]="_zero_arr_plus";
static struct _fat_ptr Cyc_Toc__zero_arr_plus_str={_tmp48,_tmp48,_tmp48 + 15U};static struct _tuple1 Cyc_Toc__zero_arr_plus_pr={{.Loc_n={4,0}},& Cyc_Toc__zero_arr_plus_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__zero_arr_plus_bnd={0U,& Cyc_Toc__zero_arr_plus_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__zero_arr_plus_re={1U,(void*)& Cyc_Toc__zero_arr_plus_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__zero_arr_plus_ev={0,(void*)& Cyc_Toc__zero_arr_plus_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__zero_arr_plus_e=& Cyc_Toc__zero_arr_plus_ev;static char _tmp49[20U]="_zero_arr_plus_char";
static struct _fat_ptr Cyc_Toc__zero_arr_plus_char_str={_tmp49,_tmp49,_tmp49 + 20U};static struct _tuple1 Cyc_Toc__zero_arr_plus_char_pr={{.Loc_n={4,0}},& Cyc_Toc__zero_arr_plus_char_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__zero_arr_plus_char_bnd={0U,& Cyc_Toc__zero_arr_plus_char_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__zero_arr_plus_char_re={1U,(void*)& Cyc_Toc__zero_arr_plus_char_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__zero_arr_plus_char_ev={0,(void*)& Cyc_Toc__zero_arr_plus_char_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__zero_arr_plus_char_e=& Cyc_Toc__zero_arr_plus_char_ev;static char _tmp4A[21U]="_zero_arr_plus_short";
static struct _fat_ptr Cyc_Toc__zero_arr_plus_short_str={_tmp4A,_tmp4A,_tmp4A + 21U};static struct _tuple1 Cyc_Toc__zero_arr_plus_short_pr={{.Loc_n={4,0}},& Cyc_Toc__zero_arr_plus_short_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__zero_arr_plus_short_bnd={0U,& Cyc_Toc__zero_arr_plus_short_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__zero_arr_plus_short_re={1U,(void*)& Cyc_Toc__zero_arr_plus_short_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__zero_arr_plus_short_ev={0,(void*)& Cyc_Toc__zero_arr_plus_short_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__zero_arr_plus_short_e=& Cyc_Toc__zero_arr_plus_short_ev;static char _tmp4B[19U]="_zero_arr_plus_int";
static struct _fat_ptr Cyc_Toc__zero_arr_plus_int_str={_tmp4B,_tmp4B,_tmp4B + 19U};static struct _tuple1 Cyc_Toc__zero_arr_plus_int_pr={{.Loc_n={4,0}},& Cyc_Toc__zero_arr_plus_int_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__zero_arr_plus_int_bnd={0U,& Cyc_Toc__zero_arr_plus_int_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__zero_arr_plus_int_re={1U,(void*)& Cyc_Toc__zero_arr_plus_int_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__zero_arr_plus_int_ev={0,(void*)& Cyc_Toc__zero_arr_plus_int_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__zero_arr_plus_int_e=& Cyc_Toc__zero_arr_plus_int_ev;static char _tmp4C[21U]="_zero_arr_plus_float";
static struct _fat_ptr Cyc_Toc__zero_arr_plus_float_str={_tmp4C,_tmp4C,_tmp4C + 21U};static struct _tuple1 Cyc_Toc__zero_arr_plus_float_pr={{.Loc_n={4,0}},& Cyc_Toc__zero_arr_plus_float_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__zero_arr_plus_float_bnd={0U,& Cyc_Toc__zero_arr_plus_float_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__zero_arr_plus_float_re={1U,(void*)& Cyc_Toc__zero_arr_plus_float_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__zero_arr_plus_float_ev={0,(void*)& Cyc_Toc__zero_arr_plus_float_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__zero_arr_plus_float_e=& Cyc_Toc__zero_arr_plus_float_ev;static char _tmp4D[22U]="_zero_arr_plus_double";
static struct _fat_ptr Cyc_Toc__zero_arr_plus_double_str={_tmp4D,_tmp4D,_tmp4D + 22U};static struct _tuple1 Cyc_Toc__zero_arr_plus_double_pr={{.Loc_n={4,0}},& Cyc_Toc__zero_arr_plus_double_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__zero_arr_plus_double_bnd={0U,& Cyc_Toc__zero_arr_plus_double_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__zero_arr_plus_double_re={1U,(void*)& Cyc_Toc__zero_arr_plus_double_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__zero_arr_plus_double_ev={0,(void*)& Cyc_Toc__zero_arr_plus_double_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__zero_arr_plus_double_e=& Cyc_Toc__zero_arr_plus_double_ev;static char _tmp4E[26U]="_zero_arr_plus_longdouble";
static struct _fat_ptr Cyc_Toc__zero_arr_plus_longdouble_str={_tmp4E,_tmp4E,_tmp4E + 26U};static struct _tuple1 Cyc_Toc__zero_arr_plus_longdouble_pr={{.Loc_n={4,0}},& Cyc_Toc__zero_arr_plus_longdouble_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__zero_arr_plus_longdouble_bnd={0U,& Cyc_Toc__zero_arr_plus_longdouble_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__zero_arr_plus_longdouble_re={1U,(void*)& Cyc_Toc__zero_arr_plus_longdouble_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__zero_arr_plus_longdouble_ev={0,(void*)& Cyc_Toc__zero_arr_plus_longdouble_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__zero_arr_plus_longdouble_e=& Cyc_Toc__zero_arr_plus_longdouble_ev;static char _tmp4F[24U]="_zero_arr_plus_voidstar";
static struct _fat_ptr Cyc_Toc__zero_arr_plus_voidstar_str={_tmp4F,_tmp4F,_tmp4F + 24U};static struct _tuple1 Cyc_Toc__zero_arr_plus_voidstar_pr={{.Loc_n={4,0}},& Cyc_Toc__zero_arr_plus_voidstar_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__zero_arr_plus_voidstar_bnd={0U,& Cyc_Toc__zero_arr_plus_voidstar_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__zero_arr_plus_voidstar_re={1U,(void*)& Cyc_Toc__zero_arr_plus_voidstar_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__zero_arr_plus_voidstar_ev={0,(void*)& Cyc_Toc__zero_arr_plus_voidstar_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__zero_arr_plus_voidstar_e=& Cyc_Toc__zero_arr_plus_voidstar_ev;static char _tmp50[22U]="_fat_ptr_inplace_plus";
static struct _fat_ptr Cyc_Toc__fat_ptr_inplace_plus_str={_tmp50,_tmp50,_tmp50 + 22U};static struct _tuple1 Cyc_Toc__fat_ptr_inplace_plus_pr={{.Loc_n={4,0}},& Cyc_Toc__fat_ptr_inplace_plus_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__fat_ptr_inplace_plus_bnd={0U,& Cyc_Toc__fat_ptr_inplace_plus_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__fat_ptr_inplace_plus_re={1U,(void*)& Cyc_Toc__fat_ptr_inplace_plus_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__fat_ptr_inplace_plus_ev={0,(void*)& Cyc_Toc__fat_ptr_inplace_plus_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__fat_ptr_inplace_plus_e=& Cyc_Toc__fat_ptr_inplace_plus_ev;static char _tmp51[23U]="_zero_arr_inplace_plus";
static struct _fat_ptr Cyc_Toc__zero_arr_inplace_plus_str={_tmp51,_tmp51,_tmp51 + 23U};static struct _tuple1 Cyc_Toc__zero_arr_inplace_plus_pr={{.Loc_n={4,0}},& Cyc_Toc__zero_arr_inplace_plus_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__zero_arr_inplace_plus_bnd={0U,& Cyc_Toc__zero_arr_inplace_plus_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__zero_arr_inplace_plus_re={1U,(void*)& Cyc_Toc__zero_arr_inplace_plus_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__zero_arr_inplace_plus_ev={0,(void*)& Cyc_Toc__zero_arr_inplace_plus_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__zero_arr_inplace_plus_e=& Cyc_Toc__zero_arr_inplace_plus_ev;static char _tmp52[28U]="_zero_arr_inplace_plus_char";
static struct _fat_ptr Cyc_Toc__zero_arr_inplace_plus_char_str={_tmp52,_tmp52,_tmp52 + 28U};static struct _tuple1 Cyc_Toc__zero_arr_inplace_plus_char_pr={{.Loc_n={4,0}},& Cyc_Toc__zero_arr_inplace_plus_char_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__zero_arr_inplace_plus_char_bnd={0U,& Cyc_Toc__zero_arr_inplace_plus_char_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__zero_arr_inplace_plus_char_re={1U,(void*)& Cyc_Toc__zero_arr_inplace_plus_char_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__zero_arr_inplace_plus_char_ev={0,(void*)& Cyc_Toc__zero_arr_inplace_plus_char_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__zero_arr_inplace_plus_char_e=& Cyc_Toc__zero_arr_inplace_plus_char_ev;static char _tmp53[29U]="_zero_arr_inplace_plus_short";
static struct _fat_ptr Cyc_Toc__zero_arr_inplace_plus_short_str={_tmp53,_tmp53,_tmp53 + 29U};static struct _tuple1 Cyc_Toc__zero_arr_inplace_plus_short_pr={{.Loc_n={4,0}},& Cyc_Toc__zero_arr_inplace_plus_short_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__zero_arr_inplace_plus_short_bnd={0U,& Cyc_Toc__zero_arr_inplace_plus_short_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__zero_arr_inplace_plus_short_re={1U,(void*)& Cyc_Toc__zero_arr_inplace_plus_short_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__zero_arr_inplace_plus_short_ev={0,(void*)& Cyc_Toc__zero_arr_inplace_plus_short_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__zero_arr_inplace_plus_short_e=& Cyc_Toc__zero_arr_inplace_plus_short_ev;static char _tmp54[27U]="_zero_arr_inplace_plus_int";
static struct _fat_ptr Cyc_Toc__zero_arr_inplace_plus_int_str={_tmp54,_tmp54,_tmp54 + 27U};static struct _tuple1 Cyc_Toc__zero_arr_inplace_plus_int_pr={{.Loc_n={4,0}},& Cyc_Toc__zero_arr_inplace_plus_int_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__zero_arr_inplace_plus_int_bnd={0U,& Cyc_Toc__zero_arr_inplace_plus_int_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__zero_arr_inplace_plus_int_re={1U,(void*)& Cyc_Toc__zero_arr_inplace_plus_int_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__zero_arr_inplace_plus_int_ev={0,(void*)& Cyc_Toc__zero_arr_inplace_plus_int_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__zero_arr_inplace_plus_int_e=& Cyc_Toc__zero_arr_inplace_plus_int_ev;static char _tmp55[29U]="_zero_arr_inplace_plus_float";
static struct _fat_ptr Cyc_Toc__zero_arr_inplace_plus_float_str={_tmp55,_tmp55,_tmp55 + 29U};static struct _tuple1 Cyc_Toc__zero_arr_inplace_plus_float_pr={{.Loc_n={4,0}},& Cyc_Toc__zero_arr_inplace_plus_float_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__zero_arr_inplace_plus_float_bnd={0U,& Cyc_Toc__zero_arr_inplace_plus_float_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__zero_arr_inplace_plus_float_re={1U,(void*)& Cyc_Toc__zero_arr_inplace_plus_float_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__zero_arr_inplace_plus_float_ev={0,(void*)& Cyc_Toc__zero_arr_inplace_plus_float_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__zero_arr_inplace_plus_float_e=& Cyc_Toc__zero_arr_inplace_plus_float_ev;static char _tmp56[30U]="_zero_arr_inplace_plus_double";
static struct _fat_ptr Cyc_Toc__zero_arr_inplace_plus_double_str={_tmp56,_tmp56,_tmp56 + 30U};static struct _tuple1 Cyc_Toc__zero_arr_inplace_plus_double_pr={{.Loc_n={4,0}},& Cyc_Toc__zero_arr_inplace_plus_double_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__zero_arr_inplace_plus_double_bnd={0U,& Cyc_Toc__zero_arr_inplace_plus_double_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__zero_arr_inplace_plus_double_re={1U,(void*)& Cyc_Toc__zero_arr_inplace_plus_double_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__zero_arr_inplace_plus_double_ev={0,(void*)& Cyc_Toc__zero_arr_inplace_plus_double_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__zero_arr_inplace_plus_double_e=& Cyc_Toc__zero_arr_inplace_plus_double_ev;static char _tmp57[34U]="_zero_arr_inplace_plus_longdouble";
static struct _fat_ptr Cyc_Toc__zero_arr_inplace_plus_longdouble_str={_tmp57,_tmp57,_tmp57 + 34U};static struct _tuple1 Cyc_Toc__zero_arr_inplace_plus_longdouble_pr={{.Loc_n={4,0}},& Cyc_Toc__zero_arr_inplace_plus_longdouble_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__zero_arr_inplace_plus_longdouble_bnd={0U,& Cyc_Toc__zero_arr_inplace_plus_longdouble_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__zero_arr_inplace_plus_longdouble_re={1U,(void*)& Cyc_Toc__zero_arr_inplace_plus_longdouble_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__zero_arr_inplace_plus_longdouble_ev={0,(void*)& Cyc_Toc__zero_arr_inplace_plus_longdouble_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__zero_arr_inplace_plus_longdouble_e=& Cyc_Toc__zero_arr_inplace_plus_longdouble_ev;static char _tmp58[32U]="_zero_arr_inplace_plus_voidstar";
static struct _fat_ptr Cyc_Toc__zero_arr_inplace_plus_voidstar_str={_tmp58,_tmp58,_tmp58 + 32U};static struct _tuple1 Cyc_Toc__zero_arr_inplace_plus_voidstar_pr={{.Loc_n={4,0}},& Cyc_Toc__zero_arr_inplace_plus_voidstar_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__zero_arr_inplace_plus_voidstar_bnd={0U,& Cyc_Toc__zero_arr_inplace_plus_voidstar_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__zero_arr_inplace_plus_voidstar_re={1U,(void*)& Cyc_Toc__zero_arr_inplace_plus_voidstar_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__zero_arr_inplace_plus_voidstar_ev={0,(void*)& Cyc_Toc__zero_arr_inplace_plus_voidstar_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__zero_arr_inplace_plus_voidstar_e=& Cyc_Toc__zero_arr_inplace_plus_voidstar_ev;static char _tmp59[27U]="_fat_ptr_inplace_plus_post";
static struct _fat_ptr Cyc_Toc__fat_ptr_inplace_plus_post_str={_tmp59,_tmp59,_tmp59 + 27U};static struct _tuple1 Cyc_Toc__fat_ptr_inplace_plus_post_pr={{.Loc_n={4,0}},& Cyc_Toc__fat_ptr_inplace_plus_post_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__fat_ptr_inplace_plus_post_bnd={0U,& Cyc_Toc__fat_ptr_inplace_plus_post_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__fat_ptr_inplace_plus_post_re={1U,(void*)& Cyc_Toc__fat_ptr_inplace_plus_post_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__fat_ptr_inplace_plus_post_ev={0,(void*)& Cyc_Toc__fat_ptr_inplace_plus_post_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__fat_ptr_inplace_plus_post_e=& Cyc_Toc__fat_ptr_inplace_plus_post_ev;static char _tmp5A[28U]="_zero_arr_inplace_plus_post";
static struct _fat_ptr Cyc_Toc__zero_arr_inplace_plus_post_str={_tmp5A,_tmp5A,_tmp5A + 28U};static struct _tuple1 Cyc_Toc__zero_arr_inplace_plus_post_pr={{.Loc_n={4,0}},& Cyc_Toc__zero_arr_inplace_plus_post_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__zero_arr_inplace_plus_post_bnd={0U,& Cyc_Toc__zero_arr_inplace_plus_post_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__zero_arr_inplace_plus_post_re={1U,(void*)& Cyc_Toc__zero_arr_inplace_plus_post_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__zero_arr_inplace_plus_post_ev={0,(void*)& Cyc_Toc__zero_arr_inplace_plus_post_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__zero_arr_inplace_plus_post_e=& Cyc_Toc__zero_arr_inplace_plus_post_ev;static char _tmp5B[33U]="_zero_arr_inplace_plus_post_char";
static struct _fat_ptr Cyc_Toc__zero_arr_inplace_plus_post_char_str={_tmp5B,_tmp5B,_tmp5B + 33U};static struct _tuple1 Cyc_Toc__zero_arr_inplace_plus_post_char_pr={{.Loc_n={4,0}},& Cyc_Toc__zero_arr_inplace_plus_post_char_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__zero_arr_inplace_plus_post_char_bnd={0U,& Cyc_Toc__zero_arr_inplace_plus_post_char_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__zero_arr_inplace_plus_post_char_re={1U,(void*)& Cyc_Toc__zero_arr_inplace_plus_post_char_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__zero_arr_inplace_plus_post_char_ev={0,(void*)& Cyc_Toc__zero_arr_inplace_plus_post_char_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__zero_arr_inplace_plus_post_char_e=& Cyc_Toc__zero_arr_inplace_plus_post_char_ev;static char _tmp5C[34U]="_zero_arr_inplace_plus_post_short";
static struct _fat_ptr Cyc_Toc__zero_arr_inplace_plus_post_short_str={_tmp5C,_tmp5C,_tmp5C + 34U};static struct _tuple1 Cyc_Toc__zero_arr_inplace_plus_post_short_pr={{.Loc_n={4,0}},& Cyc_Toc__zero_arr_inplace_plus_post_short_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__zero_arr_inplace_plus_post_short_bnd={0U,& Cyc_Toc__zero_arr_inplace_plus_post_short_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__zero_arr_inplace_plus_post_short_re={1U,(void*)& Cyc_Toc__zero_arr_inplace_plus_post_short_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__zero_arr_inplace_plus_post_short_ev={0,(void*)& Cyc_Toc__zero_arr_inplace_plus_post_short_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__zero_arr_inplace_plus_post_short_e=& Cyc_Toc__zero_arr_inplace_plus_post_short_ev;static char _tmp5D[32U]="_zero_arr_inplace_plus_post_int";
static struct _fat_ptr Cyc_Toc__zero_arr_inplace_plus_post_int_str={_tmp5D,_tmp5D,_tmp5D + 32U};static struct _tuple1 Cyc_Toc__zero_arr_inplace_plus_post_int_pr={{.Loc_n={4,0}},& Cyc_Toc__zero_arr_inplace_plus_post_int_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__zero_arr_inplace_plus_post_int_bnd={0U,& Cyc_Toc__zero_arr_inplace_plus_post_int_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__zero_arr_inplace_plus_post_int_re={1U,(void*)& Cyc_Toc__zero_arr_inplace_plus_post_int_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__zero_arr_inplace_plus_post_int_ev={0,(void*)& Cyc_Toc__zero_arr_inplace_plus_post_int_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__zero_arr_inplace_plus_post_int_e=& Cyc_Toc__zero_arr_inplace_plus_post_int_ev;static char _tmp5E[34U]="_zero_arr_inplace_plus_post_float";
static struct _fat_ptr Cyc_Toc__zero_arr_inplace_plus_post_float_str={_tmp5E,_tmp5E,_tmp5E + 34U};static struct _tuple1 Cyc_Toc__zero_arr_inplace_plus_post_float_pr={{.Loc_n={4,0}},& Cyc_Toc__zero_arr_inplace_plus_post_float_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__zero_arr_inplace_plus_post_float_bnd={0U,& Cyc_Toc__zero_arr_inplace_plus_post_float_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__zero_arr_inplace_plus_post_float_re={1U,(void*)& Cyc_Toc__zero_arr_inplace_plus_post_float_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__zero_arr_inplace_plus_post_float_ev={0,(void*)& Cyc_Toc__zero_arr_inplace_plus_post_float_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__zero_arr_inplace_plus_post_float_e=& Cyc_Toc__zero_arr_inplace_plus_post_float_ev;static char _tmp5F[35U]="_zero_arr_inplace_plus_post_double";
static struct _fat_ptr Cyc_Toc__zero_arr_inplace_plus_post_double_str={_tmp5F,_tmp5F,_tmp5F + 35U};static struct _tuple1 Cyc_Toc__zero_arr_inplace_plus_post_double_pr={{.Loc_n={4,0}},& Cyc_Toc__zero_arr_inplace_plus_post_double_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__zero_arr_inplace_plus_post_double_bnd={0U,& Cyc_Toc__zero_arr_inplace_plus_post_double_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__zero_arr_inplace_plus_post_double_re={1U,(void*)& Cyc_Toc__zero_arr_inplace_plus_post_double_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__zero_arr_inplace_plus_post_double_ev={0,(void*)& Cyc_Toc__zero_arr_inplace_plus_post_double_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__zero_arr_inplace_plus_post_double_e=& Cyc_Toc__zero_arr_inplace_plus_post_double_ev;static char _tmp60[39U]="_zero_arr_inplace_plus_post_longdouble";
static struct _fat_ptr Cyc_Toc__zero_arr_inplace_plus_post_longdouble_str={_tmp60,_tmp60,_tmp60 + 39U};static struct _tuple1 Cyc_Toc__zero_arr_inplace_plus_post_longdouble_pr={{.Loc_n={4,0}},& Cyc_Toc__zero_arr_inplace_plus_post_longdouble_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__zero_arr_inplace_plus_post_longdouble_bnd={0U,& Cyc_Toc__zero_arr_inplace_plus_post_longdouble_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__zero_arr_inplace_plus_post_longdouble_re={1U,(void*)& Cyc_Toc__zero_arr_inplace_plus_post_longdouble_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__zero_arr_inplace_plus_post_longdouble_ev={0,(void*)& Cyc_Toc__zero_arr_inplace_plus_post_longdouble_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__zero_arr_inplace_plus_post_longdouble_e=& Cyc_Toc__zero_arr_inplace_plus_post_longdouble_ev;static char _tmp61[37U]="_zero_arr_inplace_plus_post_voidstar";
static struct _fat_ptr Cyc_Toc__zero_arr_inplace_plus_post_voidstar_str={_tmp61,_tmp61,_tmp61 + 37U};static struct _tuple1 Cyc_Toc__zero_arr_inplace_plus_post_voidstar_pr={{.Loc_n={4,0}},& Cyc_Toc__zero_arr_inplace_plus_post_voidstar_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__zero_arr_inplace_plus_post_voidstar_bnd={0U,& Cyc_Toc__zero_arr_inplace_plus_post_voidstar_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__zero_arr_inplace_plus_post_voidstar_re={1U,(void*)& Cyc_Toc__zero_arr_inplace_plus_post_voidstar_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__zero_arr_inplace_plus_post_voidstar_ev={0,(void*)& Cyc_Toc__zero_arr_inplace_plus_post_voidstar_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__zero_arr_inplace_plus_post_voidstar_e=& Cyc_Toc__zero_arr_inplace_plus_post_voidstar_ev;static char _tmp62[10U]="_cycalloc";
static struct _fat_ptr Cyc_Toc__cycalloc_str={_tmp62,_tmp62,_tmp62 + 10U};static struct _tuple1 Cyc_Toc__cycalloc_pr={{.Loc_n={4,0}},& Cyc_Toc__cycalloc_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__cycalloc_bnd={0U,& Cyc_Toc__cycalloc_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__cycalloc_re={1U,(void*)& Cyc_Toc__cycalloc_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__cycalloc_ev={0,(void*)& Cyc_Toc__cycalloc_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__cycalloc_e=& Cyc_Toc__cycalloc_ev;static char _tmp63[11U]="_cyccalloc";
static struct _fat_ptr Cyc_Toc__cyccalloc_str={_tmp63,_tmp63,_tmp63 + 11U};static struct _tuple1 Cyc_Toc__cyccalloc_pr={{.Loc_n={4,0}},& Cyc_Toc__cyccalloc_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__cyccalloc_bnd={0U,& Cyc_Toc__cyccalloc_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__cyccalloc_re={1U,(void*)& Cyc_Toc__cyccalloc_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__cyccalloc_ev={0,(void*)& Cyc_Toc__cyccalloc_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__cyccalloc_e=& Cyc_Toc__cyccalloc_ev;static char _tmp64[17U]="_cycalloc_atomic";
static struct _fat_ptr Cyc_Toc__cycalloc_atomic_str={_tmp64,_tmp64,_tmp64 + 17U};static struct _tuple1 Cyc_Toc__cycalloc_atomic_pr={{.Loc_n={4,0}},& Cyc_Toc__cycalloc_atomic_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__cycalloc_atomic_bnd={0U,& Cyc_Toc__cycalloc_atomic_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__cycalloc_atomic_re={1U,(void*)& Cyc_Toc__cycalloc_atomic_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__cycalloc_atomic_ev={0,(void*)& Cyc_Toc__cycalloc_atomic_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__cycalloc_atomic_e=& Cyc_Toc__cycalloc_atomic_ev;static char _tmp65[18U]="_cyccalloc_atomic";
static struct _fat_ptr Cyc_Toc__cyccalloc_atomic_str={_tmp65,_tmp65,_tmp65 + 18U};static struct _tuple1 Cyc_Toc__cyccalloc_atomic_pr={{.Loc_n={4,0}},& Cyc_Toc__cyccalloc_atomic_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__cyccalloc_atomic_bnd={0U,& Cyc_Toc__cyccalloc_atomic_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__cyccalloc_atomic_re={1U,(void*)& Cyc_Toc__cyccalloc_atomic_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__cyccalloc_atomic_ev={0,(void*)& Cyc_Toc__cyccalloc_atomic_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__cyccalloc_atomic_e=& Cyc_Toc__cyccalloc_atomic_ev;static char _tmp66[15U]="_region_malloc";
static struct _fat_ptr Cyc_Toc__region_malloc_str={_tmp66,_tmp66,_tmp66 + 15U};static struct _tuple1 Cyc_Toc__region_malloc_pr={{.Loc_n={4,0}},& Cyc_Toc__region_malloc_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__region_malloc_bnd={0U,& Cyc_Toc__region_malloc_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__region_malloc_re={1U,(void*)& Cyc_Toc__region_malloc_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__region_malloc_ev={0,(void*)& Cyc_Toc__region_malloc_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__region_malloc_e=& Cyc_Toc__region_malloc_ev;static char _tmp67[15U]="_region_calloc";
static struct _fat_ptr Cyc_Toc__region_calloc_str={_tmp67,_tmp67,_tmp67 + 15U};static struct _tuple1 Cyc_Toc__region_calloc_pr={{.Loc_n={4,0}},& Cyc_Toc__region_calloc_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__region_calloc_bnd={0U,& Cyc_Toc__region_calloc_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__region_calloc_re={1U,(void*)& Cyc_Toc__region_calloc_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__region_calloc_ev={0,(void*)& Cyc_Toc__region_calloc_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__region_calloc_e=& Cyc_Toc__region_calloc_ev;static char _tmp68[13U]="_check_times";
static struct _fat_ptr Cyc_Toc__check_times_str={_tmp68,_tmp68,_tmp68 + 13U};static struct _tuple1 Cyc_Toc__check_times_pr={{.Loc_n={4,0}},& Cyc_Toc__check_times_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__check_times_bnd={0U,& Cyc_Toc__check_times_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__check_times_re={1U,(void*)& Cyc_Toc__check_times_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__check_times_ev={0,(void*)& Cyc_Toc__check_times_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__check_times_e=& Cyc_Toc__check_times_ev;static char _tmp69[12U]="_new_region";
static struct _fat_ptr Cyc_Toc__new_region_str={_tmp69,_tmp69,_tmp69 + 12U};static struct _tuple1 Cyc_Toc__new_region_pr={{.Loc_n={4,0}},& Cyc_Toc__new_region_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__new_region_bnd={0U,& Cyc_Toc__new_region_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__new_region_re={1U,(void*)& Cyc_Toc__new_region_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__new_region_ev={0,(void*)& Cyc_Toc__new_region_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__new_region_e=& Cyc_Toc__new_region_ev;static char _tmp6A[13U]="_push_region";
static struct _fat_ptr Cyc_Toc__push_region_str={_tmp6A,_tmp6A,_tmp6A + 13U};static struct _tuple1 Cyc_Toc__push_region_pr={{.Loc_n={4,0}},& Cyc_Toc__push_region_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__push_region_bnd={0U,& Cyc_Toc__push_region_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__push_region_re={1U,(void*)& Cyc_Toc__push_region_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__push_region_ev={0,(void*)& Cyc_Toc__push_region_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__push_region_e=& Cyc_Toc__push_region_ev;static char _tmp6B[12U]="_pop_region";
static struct _fat_ptr Cyc_Toc__pop_region_str={_tmp6B,_tmp6B,_tmp6B + 12U};static struct _tuple1 Cyc_Toc__pop_region_pr={{.Loc_n={4,0}},& Cyc_Toc__pop_region_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__pop_region_bnd={0U,& Cyc_Toc__pop_region_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__pop_region_re={1U,(void*)& Cyc_Toc__pop_region_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__pop_region_ev={0,(void*)& Cyc_Toc__pop_region_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__pop_region_e=& Cyc_Toc__pop_region_ev;static char _tmp6C[13U]="_push_effect";
static struct _fat_ptr Cyc_Toc__push_effect_str={_tmp6C,_tmp6C,_tmp6C + 13U};static struct _tuple1 Cyc_Toc__push_effect_pr={{.Loc_n={4,0}},& Cyc_Toc__push_effect_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__push_effect_bnd={0U,& Cyc_Toc__push_effect_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__push_effect_re={1U,(void*)& Cyc_Toc__push_effect_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__push_effect_ev={0,(void*)& Cyc_Toc__push_effect_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__push_effect_e=& Cyc_Toc__push_effect_ev;static char _tmp6D[12U]="_pop_effect";
static struct _fat_ptr Cyc_Toc__pop_effect_str={_tmp6D,_tmp6D,_tmp6D + 12U};static struct _tuple1 Cyc_Toc__pop_effect_pr={{.Loc_n={4,0}},& Cyc_Toc__pop_effect_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__pop_effect_bnd={0U,& Cyc_Toc__pop_effect_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__pop_effect_re={1U,(void*)& Cyc_Toc__pop_effect_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__pop_effect_ev={0,(void*)& Cyc_Toc__pop_effect_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__pop_effect_e=& Cyc_Toc__pop_effect_ev;static char _tmp6E[13U]="_peek_effect";
static struct _fat_ptr Cyc_Toc__peek_effect_str={_tmp6E,_tmp6E,_tmp6E + 13U};static struct _tuple1 Cyc_Toc__peek_effect_pr={{.Loc_n={4,0}},& Cyc_Toc__peek_effect_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__peek_effect_bnd={0U,& Cyc_Toc__peek_effect_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__peek_effect_re={1U,(void*)& Cyc_Toc__peek_effect_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__peek_effect_ev={0,(void*)& Cyc_Toc__peek_effect_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__peek_effect_e=& Cyc_Toc__peek_effect_ev;static char _tmp6F[19U]="_throw_arraybounds";
static struct _fat_ptr Cyc_Toc__throw_arraybounds_str={_tmp6F,_tmp6F,_tmp6F + 19U};static struct _tuple1 Cyc_Toc__throw_arraybounds_pr={{.Loc_n={4,0}},& Cyc_Toc__throw_arraybounds_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__throw_arraybounds_bnd={0U,& Cyc_Toc__throw_arraybounds_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__throw_arraybounds_re={1U,(void*)& Cyc_Toc__throw_arraybounds_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__throw_arraybounds_ev={0,(void*)& Cyc_Toc__throw_arraybounds_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__throw_arraybounds_e=& Cyc_Toc__throw_arraybounds_ev;static char _tmp70[23U]="_fat_ptr_decrease_size";
static struct _fat_ptr Cyc_Toc__fat_ptr_decrease_size_str={_tmp70,_tmp70,_tmp70 + 23U};static struct _tuple1 Cyc_Toc__fat_ptr_decrease_size_pr={{.Loc_n={4,0}},& Cyc_Toc__fat_ptr_decrease_size_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__fat_ptr_decrease_size_bnd={0U,& Cyc_Toc__fat_ptr_decrease_size_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__fat_ptr_decrease_size_re={1U,(void*)& Cyc_Toc__fat_ptr_decrease_size_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__fat_ptr_decrease_size_ev={0,(void*)& Cyc_Toc__fat_ptr_decrease_size_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__fat_ptr_decrease_size_e=& Cyc_Toc__fat_ptr_decrease_size_ev;static char _tmp71[13U]="_throw_match";
static struct _fat_ptr Cyc_Toc__throw_match_str={_tmp71,_tmp71,_tmp71 + 13U};static struct _tuple1 Cyc_Toc__throw_match_pr={{.Loc_n={4,0}},& Cyc_Toc__throw_match_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__throw_match_bnd={0U,& Cyc_Toc__throw_match_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__throw_match_re={1U,(void*)& Cyc_Toc__throw_match_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__throw_match_ev={0,(void*)& Cyc_Toc__throw_match_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__throw_match_e=& Cyc_Toc__throw_match_ev;static char _tmp72[9U]="_rethrow";
static struct _fat_ptr Cyc_Toc__rethrow_str={_tmp72,_tmp72,_tmp72 + 9U};static struct _tuple1 Cyc_Toc__rethrow_pr={{.Loc_n={4,0}},& Cyc_Toc__rethrow_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__rethrow_bnd={0U,& Cyc_Toc__rethrow_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__rethrow_re={1U,(void*)& Cyc_Toc__rethrow_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__rethrow_ev={0,(void*)& Cyc_Toc__rethrow_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__rethrow_e=& Cyc_Toc__rethrow_ev;static char _tmp73[20U]="_fast_region_malloc";
static struct _fat_ptr Cyc_Toc__fast_region_malloc_str={_tmp73,_tmp73,_tmp73 + 20U};static struct _tuple1 Cyc_Toc__fast_region_malloc_pr={{.Loc_n={4,0}},& Cyc_Toc__fast_region_malloc_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__fast_region_malloc_bnd={0U,& Cyc_Toc__fast_region_malloc_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__fast_region_malloc_re={1U,(void*)& Cyc_Toc__fast_region_malloc_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__fast_region_malloc_ev={0,(void*)& Cyc_Toc__fast_region_malloc_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__fast_region_malloc_e=& Cyc_Toc__fast_region_malloc_ev;static char _tmp74[19U]="_spawn_with_effect";
static struct _fat_ptr Cyc_Toc__spawn_with_effect_str={_tmp74,_tmp74,_tmp74 + 19U};static struct _tuple1 Cyc_Toc__spawn_with_effect_pr={{.Loc_n={4,0}},& Cyc_Toc__spawn_with_effect_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__spawn_with_effect_bnd={0U,& Cyc_Toc__spawn_with_effect_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__spawn_with_effect_re={1U,(void*)& Cyc_Toc__spawn_with_effect_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__spawn_with_effect_ev={0,(void*)& Cyc_Toc__spawn_with_effect_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__spawn_with_effect_e=& Cyc_Toc__spawn_with_effect_ev;static char _tmp75[13U]="_new_xregion";
static struct _fat_ptr Cyc_Toc__new_xregion_str={_tmp75,_tmp75,_tmp75 + 13U};static struct _tuple1 Cyc_Toc__new_xregion_pr={{.Loc_n={4,0}},& Cyc_Toc__new_xregion_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__new_xregion_bnd={0U,& Cyc_Toc__new_xregion_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__new_xregion_re={1U,(void*)& Cyc_Toc__new_xregion_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__new_xregion_ev={0,(void*)& Cyc_Toc__new_xregion_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__new_xregion_e=& Cyc_Toc__new_xregion_ev;static char _tmp76[16U]="_xregion_malloc";
static struct _fat_ptr Cyc_Toc__xregion_malloc_str={_tmp76,_tmp76,_tmp76 + 16U};static struct _tuple1 Cyc_Toc__xregion_malloc_pr={{.Loc_n={4,0}},& Cyc_Toc__xregion_malloc_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__xregion_malloc_bnd={0U,& Cyc_Toc__xregion_malloc_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__xregion_malloc_re={1U,(void*)& Cyc_Toc__xregion_malloc_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__xregion_malloc_ev={0,(void*)& Cyc_Toc__xregion_malloc_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__xregion_malloc_e=& Cyc_Toc__xregion_malloc_ev;static char _tmp77[16U]="_xregion_calloc";
static struct _fat_ptr Cyc_Toc__xregion_calloc_str={_tmp77,_tmp77,_tmp77 + 16U};static struct _tuple1 Cyc_Toc__xregion_calloc_pr={{.Loc_n={4,0}},& Cyc_Toc__xregion_calloc_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__xregion_calloc_bnd={0U,& Cyc_Toc__xregion_calloc_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__xregion_calloc_re={1U,(void*)& Cyc_Toc__xregion_calloc_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__xregion_calloc_ev={0,(void*)& Cyc_Toc__xregion_calloc_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__xregion_calloc_e=& Cyc_Toc__xregion_calloc_ev;static char _tmp78[22U]="_DECLARE_Effect_Frame";
static struct _fat_ptr Cyc_Toc__DECLARE_Effect_Frame_str={_tmp78,_tmp78,_tmp78 + 22U};static struct _tuple1 Cyc_Toc__DECLARE_Effect_Frame_pr={{.Loc_n={4,0}},& Cyc_Toc__DECLARE_Effect_Frame_str};static struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct Cyc_Toc__DECLARE_Effect_Frame_bnd={0U,& Cyc_Toc__DECLARE_Effect_Frame_pr};static struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct Cyc_Toc__DECLARE_Effect_Frame_re={1U,(void*)& Cyc_Toc__DECLARE_Effect_Frame_bnd};static struct Cyc_Absyn_Exp Cyc_Toc__DECLARE_Effect_Frame_ev={0,(void*)& Cyc_Toc__DECLARE_Effect_Frame_re,0U,(void*)& Cyc_Absyn_EmptyAnnot_val};static struct Cyc_Absyn_Exp*Cyc_Toc__DECLARE_Effect_Frame_e=& Cyc_Toc__DECLARE_Effect_Frame_ev;
# 301
struct Cyc_Absyn_Exp*Cyc_Toc_get_exn_thrown_expression(){
static struct Cyc_Absyn_Exp*_get_exn_thrown_e=0;
# 304
if((unsigned)_get_exn_thrown_e)
return(struct Cyc_Absyn_Exp*)_check_null(_get_exn_thrown_e);{
# 304
struct _tuple1*qv=({struct _tuple1*_tmp84=_cycalloc(sizeof(*_tmp84));
# 306
((_tmp84->f1).Abs_n).tag=2U,({struct Cyc_List_List*_tmpCBA=({struct _fat_ptr*_tmp7F[1U];({struct _fat_ptr*_tmpCB9=({struct _fat_ptr*_tmp81=_cycalloc(sizeof(*_tmp81));({struct _fat_ptr _tmpCB8=({const char*_tmp80="Core";_tag_fat(_tmp80,sizeof(char),5U);});*_tmp81=_tmpCB8;});_tmp81;});_tmp7F[0]=_tmpCB9;});Cyc_List_list(_tag_fat(_tmp7F,sizeof(struct _fat_ptr*),1U));});((_tmp84->f1).Abs_n).val=_tmpCBA;}),({struct _fat_ptr*_tmpCB7=({struct _fat_ptr*_tmp83=_cycalloc(sizeof(*_tmp83));({struct _fat_ptr _tmpCB6=({const char*_tmp82="get_exn_thrown";_tag_fat(_tmp82,sizeof(char),15U);});*_tmp83=_tmpCB6;});_tmp83;});_tmp84->f2=_tmpCB7;});_tmp84;});
void*bnd=(void*)({struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct*_tmp7E=_cycalloc(sizeof(*_tmp7E));_tmp7E->tag=0U,_tmp7E->f1=qv;_tmp7E;});
struct Cyc_Absyn_Exp*fnname=({struct Cyc_Absyn_Exp*_tmp7D=_cycalloc(sizeof(*_tmp7D));_tmp7D->topt=0,({void*_tmpCBB=(void*)({struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*_tmp7C=_cycalloc(sizeof(*_tmp7C));_tmp7C->tag=1U,_tmp7C->f1=bnd;_tmp7C;});_tmp7D->r=_tmpCBB;}),_tmp7D->loc=0U,_tmp7D->annot=(void*)& Cyc_Absyn_EmptyAnnot_val;_tmp7D;});
void*fncall_re=(void*)({struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*_tmp7B=_cycalloc(sizeof(*_tmp7B));_tmp7B->tag=10U,_tmp7B->f1=fnname,_tmp7B->f2=0,_tmp7B->f3=0,_tmp7B->f4=0,({struct _tuple0*_tmpCBC=({struct _tuple0*_tmp7A=_cycalloc(sizeof(*_tmp7A));_tmp7A->f1=0,_tmp7A->f2=0;_tmp7A;});_tmp7B->f5=_tmpCBC;});_tmp7B;});
_get_exn_thrown_e=({struct Cyc_Absyn_Exp*_tmp79=_cycalloc(sizeof(*_tmp79));_tmp79->topt=0,_tmp79->r=fncall_re,_tmp79->loc=0U,_tmp79->annot=(void*)& Cyc_Absyn_EmptyAnnot_val;_tmp79;});
return(struct Cyc_Absyn_Exp*)_check_null(_get_exn_thrown_e);}}
# 301
static struct Cyc_Absyn_Tqual Cyc_Toc_mt_tq={0,0,0,0,0U};
# 327 "toc.cyc"
void*Cyc_Toc_effect_frame_caps_type(){
static void*t=0;
if(t == 0)
t=({void*(*_tmp86)(struct _fat_ptr*name)=Cyc_Absyn_strct;struct _fat_ptr*_tmp87=({struct _fat_ptr*_tmp8A=_cycalloc(sizeof(*_tmp8A));({struct _fat_ptr _tmpCBE=(struct _fat_ptr)({void*_tmp88=0U;({struct _fat_ptr _tmpCBD=({const char*_tmp89="_EffectFrameCaps";_tag_fat(_tmp89,sizeof(char),17U);});Cyc_aprintf(_tmpCBD,_tag_fat(_tmp88,sizeof(void*),0U));});});*_tmp8A=_tmpCBE;});_tmp8A;});_tmp86(_tmp87);});
# 329
return(void*)_check_null(t);}
# 327
void*Cyc_Toc_effect_frame_type(){
# 336
static void*t=0;
if(t == 0)
t=({void*(*_tmp8C)(struct _fat_ptr*name)=Cyc_Absyn_strct;struct _fat_ptr*_tmp8D=({struct _fat_ptr*_tmp90=_cycalloc(sizeof(*_tmp90));({struct _fat_ptr _tmpCC0=(struct _fat_ptr)({void*_tmp8E=0U;({struct _fat_ptr _tmpCBF=({const char*_tmp8F="_EffectFrame";_tag_fat(_tmp8F,sizeof(char),13U);});Cyc_aprintf(_tmpCBF,_tag_fat(_tmp8E,sizeof(void*),0U));});});*_tmp90=_tmpCC0;});_tmp90;});_tmp8C(_tmp8D);});
# 337
return(void*)_check_null(t);}
# 327
void*Cyc_Toc_void_star_type(){
# 344
static void*t=0;
if(t == 0)
t=({void*(*_tmp92)(void*t,void*rgn,struct Cyc_Absyn_Tqual,void*zero_term)=Cyc_Absyn_star_type;void*_tmp93=Cyc_Absyn_void_type;void*_tmp94=Cyc_Absyn_heap_rgn_type;struct Cyc_Absyn_Tqual _tmp95=({Cyc_Absyn_empty_tqual(0U);});void*_tmp96=Cyc_Absyn_false_type;_tmp92(_tmp93,_tmp94,_tmp95,_tmp96);});
# 345
return(void*)_check_null(t);}
# 349
static void*Cyc_Toc_fat_ptr_type(){
static void*t=0;
if(t == 0)
t=({void*(*_tmp98)(union Cyc_Absyn_AggrInfo,struct Cyc_List_List*args)=Cyc_Absyn_aggr_type;union Cyc_Absyn_AggrInfo _tmp99=({Cyc_Absyn_UnknownAggr(Cyc_Absyn_StructA,& Cyc_Toc__fat_ptr_pr,0);});struct Cyc_List_List*_tmp9A=0;_tmp98(_tmp99,_tmp9A);});
# 351
return(void*)_check_null(t);}
# 355
static void*Cyc_Toc_rgn_type(){
static void*r=0;
if(r == 0)
r=({void*(*_tmp9C)(void*,struct Cyc_Absyn_Tqual)=Cyc_Absyn_cstar_type;void*_tmp9D=({Cyc_Absyn_strct(Cyc_Toc__RegionHandle_sp);});struct Cyc_Absyn_Tqual _tmp9E=Cyc_Toc_mt_tq;_tmp9C(_tmp9D,_tmp9E);});
# 357
return(void*)_check_null(r);}
# 362
static void*Cyc_Toc_xrgn_type(){
static void*r=0;
if(r == 0)
r=({void*(*_tmpA0)(void*,struct Cyc_Absyn_Tqual)=Cyc_Absyn_cstar_type;void*_tmpA1=({Cyc_Absyn_strct(Cyc_Toc__XRegionHandle_sp);});struct Cyc_Absyn_Tqual _tmpA2=Cyc_Toc_mt_tq;_tmpA0(_tmpA1,_tmpA2);});
# 364
return(void*)_check_null(r);}
# 369
static struct Cyc_Absyn_Stmt*Cyc_Toc_skip_stmt_dl(){
return({Cyc_Absyn_skip_stmt(0U);});}struct _tuple21{struct Cyc_List_List*f1;struct Cyc_Absyn_Exp*f2;};
# 373
static struct _tuple21*Cyc_Toc_make_field(struct _fat_ptr*name,struct Cyc_Absyn_Exp*e){
return({struct _tuple21*_tmpA7=_cycalloc(sizeof(*_tmpA7));({struct Cyc_List_List*_tmpCC2=({struct Cyc_List_List*_tmpA6=_cycalloc(sizeof(*_tmpA6));({void*_tmpCC1=(void*)({struct Cyc_Absyn_FieldName_Absyn_Designator_struct*_tmpA5=_cycalloc(sizeof(*_tmpA5));_tmpA5->tag=1U,_tmpA5->f1=name;_tmpA5;});_tmpA6->hd=_tmpCC1;}),_tmpA6->tl=0;_tmpA6;});_tmpA7->f1=_tmpCC2;}),_tmpA7->f2=e;_tmpA7;});}
# 377
static struct Cyc_Absyn_Exp*Cyc_Toc_fncall_exp_dl(struct Cyc_Absyn_Exp*f,struct _fat_ptr args){
return({struct Cyc_Absyn_Exp*(*_tmpA9)(struct Cyc_Absyn_Exp*,struct Cyc_List_List*,unsigned)=Cyc_Absyn_fncall_exp;struct Cyc_Absyn_Exp*_tmpAA=f;struct Cyc_List_List*_tmpAB=({Cyc_List_from_array(args);});unsigned _tmpAC=0U;_tmpA9(_tmpAA,_tmpAB,_tmpAC);});}
# 380
static struct Cyc_Absyn_Exp*Cyc_Toc_cast_it(void*t,struct Cyc_Absyn_Exp*e){
void*_stmttmp2=e->r;void*_tmpAE=_stmttmp2;struct Cyc_Absyn_Exp*_tmpAF;if(((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmpAE)->tag == 14U){if(((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmpAE)->f4 == Cyc_Absyn_No_coercion){_LL1: _tmpAF=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmpAE)->f2;_LL2: {struct Cyc_Absyn_Exp*e=_tmpAF;
return({Cyc_Toc_cast_it(t,e);});}}else{goto _LL3;}}else{_LL3: _LL4:
 return({Cyc_Absyn_cast_exp(t,e,0,Cyc_Absyn_No_coercion,0U);});}_LL0:;}
# 388
static struct Cyc_Absyn_Exp*Cyc_Toc_void_star_exp(unsigned i){
struct Cyc_Absyn_Exp*r=({struct Cyc_Absyn_Exp*(*_tmpB1)(void*t,struct Cyc_Absyn_Exp*e)=Cyc_Toc_cast_it;void*_tmpB2=({Cyc_Toc_void_star_type();});struct Cyc_Absyn_Exp*_tmpB3=({Cyc_Absyn_uint_exp(i,0U);});_tmpB1(_tmpB2,_tmpB3);});
({void*_tmpCC3=({Cyc_Toc_void_star_type();});r->topt=_tmpCC3;});
return r;}
# 394
static struct Cyc_Absyn_Exp*Cyc_Toc_char_exp(unsigned i){
struct Cyc_Absyn_Exp*r=({struct Cyc_Absyn_Exp*(*_tmpB5)(void*t,struct Cyc_Absyn_Exp*e)=Cyc_Toc_cast_it;void*_tmpB6=Cyc_Absyn_char_type;struct Cyc_Absyn_Exp*_tmpB7=({Cyc_Absyn_uint_exp(i,0U);});_tmpB5(_tmpB6,_tmpB7);});
({void*_tmpCC4=({Cyc_Absyn_cstar_type(Cyc_Absyn_char_type,Cyc_Toc_mt_tq);});r->topt=_tmpCC4;});
return r;}
# 522 "toc.cyc"
static struct Cyc_Absyn_Exp*Cyc_Toc_effect_frame_caps_init(unsigned idx,unsigned rg,unsigned lk){
# 524
static struct _tuple1*qv=0;
static struct _fat_ptr*field1_name=0;
static struct _fat_ptr*field2_name=0;
static struct _fat_ptr*field3_name=0;
# 530
if(field1_name == 0){
qv=({struct _tuple1*_tmpBB=_cycalloc(sizeof(*_tmpBB));((_tmpBB->f1).Loc_n).tag=4U,((_tmpBB->f1).Loc_n).val=0,({struct _fat_ptr*_tmpCC6=({struct _fat_ptr*_tmpBA=_cycalloc(sizeof(*_tmpBA));({struct _fat_ptr _tmpCC5=({const char*_tmpB9="_EffectFrameCaps";_tag_fat(_tmpB9,sizeof(char),17U);});*_tmpBA=_tmpCC5;});_tmpBA;});_tmpBB->f2=_tmpCC6;});_tmpBB;});
field1_name=({struct _fat_ptr*_tmpBE=_cycalloc(sizeof(*_tmpBE));({struct _fat_ptr _tmpCC8=(struct _fat_ptr)({void*_tmpBC=0U;({struct _fat_ptr _tmpCC7=({const char*_tmpBD="idx";_tag_fat(_tmpBD,sizeof(char),4U);});Cyc_aprintf(_tmpCC7,_tag_fat(_tmpBC,sizeof(void*),0U));});});*_tmpBE=_tmpCC8;});_tmpBE;});
field2_name=({struct _fat_ptr*_tmpC1=_cycalloc(sizeof(*_tmpC1));({struct _fat_ptr _tmpCCA=(struct _fat_ptr)({void*_tmpBF=0U;({struct _fat_ptr _tmpCC9=({const char*_tmpC0="rg";_tag_fat(_tmpC0,sizeof(char),3U);});Cyc_aprintf(_tmpCC9,_tag_fat(_tmpBF,sizeof(void*),0U));});});*_tmpC1=_tmpCCA;});_tmpC1;});
field3_name=({struct _fat_ptr*_tmpC4=_cycalloc(sizeof(*_tmpC4));({struct _fat_ptr _tmpCCC=(struct _fat_ptr)({void*_tmpC2=0U;({struct _fat_ptr _tmpCCB=({const char*_tmpC3="lk";_tag_fat(_tmpC3,sizeof(char),3U);});Cyc_aprintf(_tmpCCB,_tag_fat(_tmpC2,sizeof(void*),0U));});});*_tmpC4=_tmpCCC;});_tmpC4;});}
# 530
if(
# 538
(rg > (unsigned)255 || lk > (unsigned)255)|| idx > (unsigned)255)
({struct Cyc_Int_pa_PrintArg_struct _tmpC8=({struct Cyc_Int_pa_PrintArg_struct _tmpC5D;_tmpC5D.tag=1U,_tmpC5D.f1=(unsigned long)((int)rg);_tmpC5D;});struct Cyc_Int_pa_PrintArg_struct _tmpC9=({struct Cyc_Int_pa_PrintArg_struct _tmpC5C;_tmpC5C.tag=1U,_tmpC5C.f1=(unsigned long)((int)lk);_tmpC5C;});void*_tmpC5[2U];_tmpC5[0]=& _tmpC8,_tmpC5[1]=& _tmpC9;({int(*_tmpCCE)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmpC7)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmpC7;});struct _fat_ptr _tmpCCD=({const char*_tmpC6="could not generate code for _EffectFrameCaps. Too many rgion counts found (rg=%d ,lk=%d)";_tag_fat(_tmpC6,sizeof(char),89U);});_tmpCCE(_tmpCCD,_tag_fat(_tmpC5,sizeof(void*),2U));});});{
# 530
struct Cyc_Absyn_Exp*addr=({Cyc_Absyn_uint_exp(idx,0U);});
# 542
struct Cyc_Absyn_Exp*i_exp1=({Cyc_Absyn_uint_exp(rg,0U);});
struct Cyc_Absyn_Exp*i_exp2=({Cyc_Absyn_uint_exp(lk,0U);});
# 546
struct _tuple21*f1=({Cyc_Toc_make_field((struct _fat_ptr*)_check_null(field1_name),addr);});
struct _tuple21*f2=({Cyc_Toc_make_field((struct _fat_ptr*)_check_null(field2_name),i_exp1);});
struct _tuple21*f3=({Cyc_Toc_make_field((struct _fat_ptr*)_check_null(field3_name),i_exp2);});
# 550
struct Cyc_Absyn_Exp*aggr=({Cyc_Absyn_new_exp((void*)({struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*_tmpCD=_cycalloc(sizeof(*_tmpCD));_tmpCD->tag=29U,_tmpCD->f1=(struct _tuple1*)_check_null(qv),_tmpCD->f2=0,({struct Cyc_List_List*_tmpCD1=({struct Cyc_List_List*_tmpCC=_cycalloc(sizeof(*_tmpCC));_tmpCC->hd=f1,({struct Cyc_List_List*_tmpCD0=({struct Cyc_List_List*_tmpCB=_cycalloc(sizeof(*_tmpCB));_tmpCB->hd=f2,({struct Cyc_List_List*_tmpCCF=({struct Cyc_List_List*_tmpCA=_cycalloc(sizeof(*_tmpCA));_tmpCA->hd=f3,_tmpCA->tl=0;_tmpCA;});_tmpCB->tl=_tmpCCF;});_tmpCB;});_tmpCC->tl=_tmpCD0;});_tmpCC;});_tmpCD->f3=_tmpCD1;}),_tmpCD->f4=0;_tmpCD;}),0U);});
# 553
return aggr;}}struct _tuple22{struct Cyc_Absyn_Exp*f1;unsigned f2;};struct _tuple23{struct _fat_ptr f1;unsigned f2;};
# 556
static struct _tuple22 Cyc_Toc_effect_frame_caps_array_init(struct Cyc_List_List*iter){
struct Cyc_List_List*lst=0;
int i=0;int idx=0;
struct _fat_ptr found_list=_tag_fat(0,0,0);
# 561
for(0;iter != 0;iter=iter->tl){
struct Cyc_Absyn_RgnEffect*rf=(struct Cyc_Absyn_RgnEffect*)iter->hd;
struct _fat_ptr str=({Cyc_Absynpp_typ2string(rf->name);});
idx=-1;
{struct Cyc_List_List*iter0=Cyc_Toc_xrgn_map;for(0;iter0 != 0;iter0=iter0->tl){
struct _tuple23*_stmttmp3=(struct _tuple23*)iter0->hd;unsigned _tmpD0;struct _fat_ptr _tmpCF;_LL1: _tmpCF=_stmttmp3->f1;_tmpD0=_stmttmp3->f2;_LL2: {struct _fat_ptr name=_tmpCF;unsigned idx0=_tmpD0;
if(({Cyc_strcmp((struct _fat_ptr)name,(struct _fat_ptr)str);})== 0){
idx=(int)idx0;
break;}}}}
# 572
if(idx == -1)
({void*_tmpD1=0U;({int(*_tmpCD3)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmpD3)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmpD3;});struct _fat_ptr _tmpCD2=({const char*_tmpD2="effect_frames_caps_array_init";_tag_fat(_tmpD2,sizeof(char),30U);});_tmpCD3(_tmpCD2,_tag_fat(_tmpD1,sizeof(void*),0U));});});{
# 572
struct _tuple13 _stmttmp4=({struct _tuple13(*_tmpDE)(struct Cyc_List_List*)=Cyc_Absyn_caps2tup;struct Cyc_List_List*_tmpDF=({Cyc_Absyn_rgneffect_caps((struct Cyc_Absyn_RgnEffect*)iter->hd);});_tmpDE(_tmpDF);});int _tmpD5;int _tmpD4;_LL4: _tmpD4=_stmttmp4.f1;_tmpD5=_stmttmp4.f2;_LL5: {int rgcp=_tmpD4;int lkcp=_tmpD5;
# 575
lst=({struct Cyc_List_List*_tmpDD=_cycalloc(sizeof(*_tmpDD));({struct _tuple21*_tmpCD6=({struct _tuple21*(*_tmpD6)(struct _fat_ptr*name,struct Cyc_Absyn_Exp*e)=Cyc_Toc_make_field;struct _fat_ptr*_tmpD7=({struct _fat_ptr*_tmpDB=_cycalloc(sizeof(*_tmpDB));({struct _fat_ptr _tmpCD5=(struct _fat_ptr)({struct Cyc_Int_pa_PrintArg_struct _tmpDA=({struct Cyc_Int_pa_PrintArg_struct _tmpC5E;_tmpC5E.tag=1U,_tmpC5E.f1=(unsigned long)i ++;_tmpC5E;});void*_tmpD8[1U];_tmpD8[0]=& _tmpDA;({struct _fat_ptr _tmpCD4=({const char*_tmpD9="%d";_tag_fat(_tmpD9,sizeof(char),3U);});Cyc_aprintf(_tmpCD4,_tag_fat(_tmpD8,sizeof(void*),1U));});});*_tmpDB=_tmpCD5;});_tmpDB;});struct Cyc_Absyn_Exp*_tmpDC=({Cyc_Toc_effect_frame_caps_init((unsigned)idx,(unsigned)rgcp,(unsigned)lkcp);});_tmpD6(_tmpD7,_tmpDC);});_tmpDD->hd=_tmpCD6;}),_tmpDD->tl=lst;_tmpDD;});}}}{
# 581
int j=i;
if(j == 0)++ j;lst=({Cyc_List_imp_rev(lst);});{
# 584
struct Cyc_Absyn_Exp*init0=({Cyc_Absyn_new_exp((void*)({struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*_tmpE6=_cycalloc(sizeof(*_tmpE6));_tmpE6->tag=26U,_tmpE6->f1=lst;_tmpE6;}),0U);});
({void*_tmpCD7=({void*(*_tmpE0)(void*elt_type,struct Cyc_Absyn_Tqual tq,struct Cyc_Absyn_Exp*num_elts,void*zero_term,unsigned ztloc)=Cyc_Absyn_array_type;void*_tmpE1=({Cyc_Toc_effect_frame_caps_type();});struct Cyc_Absyn_Tqual _tmpE2=Cyc_Toc_mt_tq;struct Cyc_Absyn_Exp*_tmpE3=({Cyc_Absyn_uint_exp((unsigned)j,0U);});void*_tmpE4=Cyc_Absyn_false_type;unsigned _tmpE5=0U;_tmpE0(_tmpE1,_tmpE2,_tmpE3,_tmpE4,_tmpE5);});init0->topt=_tmpCD7;});
# 587
return({struct _tuple22 _tmpC5F;_tmpC5F.f1=init0,_tmpC5F.f2=(unsigned)i;_tmpC5F;});}}}
# 591
static unsigned Cyc_Toc_search_rgnmap(void*name){
struct Cyc_List_List*iter0;
struct _fat_ptr str=({Cyc_Absynpp_typ2string(name);});
for(iter0=Cyc_Toc_xrgn_map;iter0 != 0;iter0=iter0->tl){
struct _tuple23*_stmttmp5=(struct _tuple23*)iter0->hd;unsigned _tmpE9;struct _fat_ptr _tmpE8;_LL1: _tmpE8=_stmttmp5->f1;_tmpE9=_stmttmp5->f2;_LL2: {struct _fat_ptr name=_tmpE8;unsigned idx0=_tmpE9;
# 598
if(({Cyc_strcmp((struct _fat_ptr)name,(struct _fat_ptr)str);})== 0)
# 600
return idx0;}}
# 603
({struct Cyc_String_pa_PrintArg_struct _tmpED=({struct Cyc_String_pa_PrintArg_struct _tmpC60;_tmpC60.tag=0U,_tmpC60.f1=(struct _fat_ptr)((struct _fat_ptr)str);_tmpC60;});void*_tmpEA[1U];_tmpEA[0]=& _tmpED;({int(*_tmpCD9)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmpEC)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmpEC;});struct _fat_ptr _tmpCD8=({const char*_tmpEB="search_rgnmap: could not find index for :%s";_tag_fat(_tmpEB,sizeof(char),44U);});_tmpCD9(_tmpCD8,_tag_fat(_tmpEA,sizeof(void*),1U));});});}
# 606
static struct _tuple22 Cyc_Toc_uint_array_init(struct Cyc_List_List*iter){
struct Cyc_List_List*lst=0;
unsigned i=0U;unsigned idx=0U;
for(0;iter != 0;iter=iter->tl){
struct Cyc_Absyn_RgnEffect*rf=(struct Cyc_Absyn_RgnEffect*)iter->hd;
struct _fat_ptr str=({Cyc_Absynpp_typ2string(rf->name);});
struct Cyc_List_List*iter0;
for(iter0=Cyc_Toc_xrgn_map;iter0 != 0;iter0=iter0->tl){
struct _tuple23*_stmttmp6=(struct _tuple23*)iter0->hd;unsigned _tmpF0;struct _fat_ptr _tmpEF;_LL1: _tmpEF=_stmttmp6->f1;_tmpF0=_stmttmp6->f2;_LL2: {struct _fat_ptr name=_tmpEF;unsigned idx0=_tmpF0;
# 617
if(({Cyc_strcmp((struct _fat_ptr)name,(struct _fat_ptr)str);})== 0){
idx=idx0;
# 620
break;}}}
# 625
lst=({struct Cyc_List_List*_tmpF8=_cycalloc(sizeof(*_tmpF8));({struct _tuple21*_tmpCDC=({struct _tuple21*(*_tmpF1)(struct _fat_ptr*name,struct Cyc_Absyn_Exp*e)=Cyc_Toc_make_field;struct _fat_ptr*_tmpF2=({struct _fat_ptr*_tmpF6=_cycalloc(sizeof(*_tmpF6));({struct _fat_ptr _tmpCDB=(struct _fat_ptr)({struct Cyc_Int_pa_PrintArg_struct _tmpF5=({struct Cyc_Int_pa_PrintArg_struct _tmpC61;_tmpC61.tag=1U,_tmpC61.f1=(unsigned long)((int)i ++);_tmpC61;});void*_tmpF3[1U];_tmpF3[0]=& _tmpF5;({struct _fat_ptr _tmpCDA=({const char*_tmpF4="%d";_tag_fat(_tmpF4,sizeof(char),3U);});Cyc_aprintf(_tmpCDA,_tag_fat(_tmpF3,sizeof(void*),1U));});});*_tmpF6=_tmpCDB;});_tmpF6;});struct Cyc_Absyn_Exp*_tmpF7=({Cyc_Absyn_uint_exp(idx,0U);});_tmpF1(_tmpF2,_tmpF7);});_tmpF8->hd=_tmpCDC;}),_tmpF8->tl=lst;_tmpF8;});}{
# 628
int j=(int)i;
if(j == 0)++ j;lst=({Cyc_List_imp_rev(lst);});{
# 631
struct Cyc_Absyn_Exp*init0=({Cyc_Absyn_new_exp((void*)({struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*_tmpFF=_cycalloc(sizeof(*_tmpFF));_tmpFF->tag=26U,_tmpFF->f1=lst;_tmpFF;}),0U);});
({void*_tmpCDD=({void*(*_tmpF9)(void*elt_type,struct Cyc_Absyn_Tqual tq,struct Cyc_Absyn_Exp*num_elts,void*zero_term,unsigned ztloc)=Cyc_Absyn_array_type;void*_tmpFA=Cyc_Absyn_uint_type;struct Cyc_Absyn_Tqual _tmpFB=Cyc_Toc_mt_tq;struct Cyc_Absyn_Exp*_tmpFC=({Cyc_Absyn_uint_exp((unsigned)j,0U);});void*_tmpFD=Cyc_Absyn_false_type;unsigned _tmpFE=0U;_tmpF9(_tmpFA,_tmpFB,_tmpFC,_tmpFD,_tmpFE);});init0->topt=_tmpCDD;});
# 634
return({struct _tuple22 _tmpC62;_tmpC62.f1=init0,_tmpC62.f2=i;_tmpC62;});}}}
# 637
static struct _tuple22 Cyc_Toc_void_star_array_init(struct Cyc_List_List*iter){
struct Cyc_List_List*lst=0;
unsigned i=0U;unsigned idx=0U;
for(0;iter != 0;iter=iter->tl){
struct Cyc_Absyn_RgnEffect*rf=(struct Cyc_Absyn_RgnEffect*)iter->hd;
struct _fat_ptr str=({Cyc_Absynpp_typ2string(rf->name);});
struct Cyc_List_List*iter0;
idx=0U;
for(iter0=Cyc_Toc_xrgn_map;iter0 != 0;iter0=iter0->tl){
struct _tuple23*_stmttmp7=(struct _tuple23*)iter0->hd;unsigned _tmp102;struct _fat_ptr _tmp101;_LL1: _tmp101=_stmttmp7->f1;_tmp102=_stmttmp7->f2;_LL2: {struct _fat_ptr name=_tmp101;unsigned idx0=_tmp102;
if(({Cyc_strcmp((struct _fat_ptr)name,(struct _fat_ptr)str);})== 0){
idx=idx0;
break;}}}
# 653
lst=({struct Cyc_List_List*_tmp10A=_cycalloc(sizeof(*_tmp10A));({struct _tuple21*_tmpCE0=({struct _tuple21*(*_tmp103)(struct _fat_ptr*name,struct Cyc_Absyn_Exp*e)=Cyc_Toc_make_field;struct _fat_ptr*_tmp104=({struct _fat_ptr*_tmp108=_cycalloc(sizeof(*_tmp108));({struct _fat_ptr _tmpCDF=(struct _fat_ptr)({struct Cyc_Int_pa_PrintArg_struct _tmp107=({struct Cyc_Int_pa_PrintArg_struct _tmpC63;_tmpC63.tag=1U,_tmpC63.f1=(unsigned long)((int)i ++);_tmpC63;});void*_tmp105[1U];_tmp105[0]=& _tmp107;({struct _fat_ptr _tmpCDE=({const char*_tmp106="%d";_tag_fat(_tmp106,sizeof(char),3U);});Cyc_aprintf(_tmpCDE,_tag_fat(_tmp105,sizeof(void*),1U));});});*_tmp108=_tmpCDF;});_tmp108;});struct Cyc_Absyn_Exp*_tmp109=({Cyc_Toc_char_exp(idx);});_tmp103(_tmp104,_tmp109);});_tmp10A->hd=_tmpCE0;}),_tmp10A->tl=lst;_tmp10A;});}{
# 657
int j=(int)i;
if(j == 0)++ j;lst=({Cyc_List_imp_rev(lst);});{
# 660
struct Cyc_Absyn_Exp*init0=({Cyc_Absyn_new_exp((void*)({struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*_tmp111=_cycalloc(sizeof(*_tmp111));_tmp111->tag=26U,_tmp111->f1=lst;_tmp111;}),0U);});
({void*_tmpCE1=({void*(*_tmp10B)(void*elt_type,struct Cyc_Absyn_Tqual tq,struct Cyc_Absyn_Exp*num_elts,void*zero_term,unsigned ztloc)=Cyc_Absyn_array_type;void*_tmp10C=Cyc_Absyn_char_type;struct Cyc_Absyn_Tqual _tmp10D=Cyc_Toc_mt_tq;struct Cyc_Absyn_Exp*_tmp10E=({Cyc_Absyn_uint_exp((unsigned)j,0U);});void*_tmp10F=Cyc_Absyn_false_type;unsigned _tmp110=0U;_tmp10B(_tmp10C,_tmp10D,_tmp10E,_tmp10F,_tmp110);});init0->topt=_tmpCE1;});
# 665
return({struct _tuple22 _tmpC64;_tmpC64.f1=init0,_tmpC64.f2=i;_tmpC64;});}}}
# 668
static struct Cyc_Absyn_Exp*Cyc_Toc_effect_frame_init(struct _tuple1*formal_handles,struct _tuple1*inner_handles,int max_formal){
# 671
static struct _tuple1*qv=0;
static struct _fat_ptr*field1_name=0;
static struct _fat_ptr*field2_name=0;
static struct _fat_ptr*field3_name=0;
# 678
if(field1_name == 0){
qv=({struct _tuple1*_tmp115=_cycalloc(sizeof(*_tmp115));((_tmp115->f1).Loc_n).tag=4U,((_tmp115->f1).Loc_n).val=0,({struct _fat_ptr*_tmpCE3=({struct _fat_ptr*_tmp114=_cycalloc(sizeof(*_tmp114));({struct _fat_ptr _tmpCE2=({const char*_tmp113="_EffectFrame";_tag_fat(_tmp113,sizeof(char),13U);});*_tmp114=_tmpCE2;});_tmp114;});_tmp115->f2=_tmpCE3;});_tmp115;});
field1_name=({struct _fat_ptr*_tmp118=_cycalloc(sizeof(*_tmp118));({struct _fat_ptr _tmpCE5=(struct _fat_ptr)({void*_tmp116=0U;({struct _fat_ptr _tmpCE4=({const char*_tmp117="formal_handles";_tag_fat(_tmp117,sizeof(char),15U);});Cyc_aprintf(_tmpCE4,_tag_fat(_tmp116,sizeof(void*),0U));});});*_tmp118=_tmpCE5;});_tmp118;});
field2_name=({struct _fat_ptr*_tmp11B=_cycalloc(sizeof(*_tmp11B));({struct _fat_ptr _tmpCE7=(struct _fat_ptr)({void*_tmp119=0U;({struct _fat_ptr _tmpCE6=({const char*_tmp11A="max_formal";_tag_fat(_tmp11A,sizeof(char),11U);});Cyc_aprintf(_tmpCE6,_tag_fat(_tmp119,sizeof(void*),0U));});});*_tmp11B=_tmpCE7;});_tmp11B;});
field3_name=({struct _fat_ptr*_tmp11E=_cycalloc(sizeof(*_tmp11E));({struct _fat_ptr _tmpCE9=(struct _fat_ptr)({void*_tmp11C=0U;({struct _fat_ptr _tmpCE8=({const char*_tmp11D="inner_handles";_tag_fat(_tmp11D,sizeof(char),14U);});Cyc_aprintf(_tmpCE8,_tag_fat(_tmp11C,sizeof(void*),0U));});});*_tmp11E=_tmpCE9;});_tmp11E;});}
# 678
if(max_formal > 255)
# 688
({struct Cyc_Int_pa_PrintArg_struct _tmp122=({struct Cyc_Int_pa_PrintArg_struct _tmpC65;_tmpC65.tag=1U,_tmpC65.f1=(unsigned long)max_formal;_tmpC65;});void*_tmp11F[1U];_tmp11F[0]=& _tmp122;({int(*_tmpCEB)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp121)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmp121;});struct _fat_ptr _tmpCEA=({const char*_tmp120="could not generate code for _EffectFrame. Too many formal  handles passed (max_formal=%d)";_tag_fat(_tmp120,sizeof(char),90U);});_tmpCEB(_tmpCEA,_tag_fat(_tmp11F,sizeof(void*),1U));});});{
# 678
struct Cyc_Absyn_Exp*formal_handles_addr;
# 691
if(formal_handles != 0)
formal_handles_addr=({struct Cyc_Absyn_Exp*(*_tmp123)(void*t,struct Cyc_Absyn_Exp*e)=Cyc_Toc_cast_it;void*_tmp124=({Cyc_Absyn_cstar_type(Cyc_Absyn_char_type,Cyc_Toc_mt_tq);});struct Cyc_Absyn_Exp*_tmp125=({Cyc_Absyn_var_exp(formal_handles,0U);});_tmp123(_tmp124,_tmp125);});else{
# 696
formal_handles_addr=({struct Cyc_Absyn_Exp*(*_tmp126)(void*t,struct Cyc_Absyn_Exp*e)=Cyc_Toc_cast_it;void*_tmp127=({Cyc_Absyn_cstar_type(Cyc_Absyn_char_type,Cyc_Toc_mt_tq);});struct Cyc_Absyn_Exp*_tmp128=({Cyc_Absyn_uint_exp(0U,0U);});_tmp126(_tmp127,_tmp128);});}{
# 698
struct _tuple21*f1=({Cyc_Toc_make_field((struct _fat_ptr*)_check_null(field1_name),formal_handles_addr);});
struct _tuple21*f2=({struct _tuple21*(*_tmp139)(struct _fat_ptr*name,struct Cyc_Absyn_Exp*e)=Cyc_Toc_make_field;struct _fat_ptr*_tmp13A=(struct _fat_ptr*)_check_null(field2_name);struct Cyc_Absyn_Exp*_tmp13B=({Cyc_Absyn_uint_exp((unsigned)max_formal,0U);});_tmp139(_tmp13A,_tmp13B);});
# 702
struct _tuple21*f3;
struct Cyc_Absyn_Exp*f3_exp;
# 705
if(inner_handles == 0)
f3_exp=({struct Cyc_Absyn_Exp*(*_tmp129)(void*t,struct Cyc_Absyn_Exp*e)=Cyc_Toc_cast_it;void*_tmp12A=({void*(*_tmp12B)(void*,struct Cyc_Absyn_Tqual)=Cyc_Absyn_cstar_type;void*_tmp12C=({Cyc_Absyn_cstar_type(Cyc_Absyn_void_type,Cyc_Toc_mt_tq);});struct Cyc_Absyn_Tqual _tmp12D=Cyc_Toc_mt_tq;_tmp12B(_tmp12C,_tmp12D);});struct Cyc_Absyn_Exp*_tmp12E=({Cyc_Absyn_uint_exp(0U,0U);});_tmp129(_tmp12A,_tmp12E);});else{
# 709
f3_exp=({struct Cyc_Absyn_Exp*(*_tmp12F)(void*t,struct Cyc_Absyn_Exp*e)=Cyc_Toc_cast_it;void*_tmp130=({void*(*_tmp131)(void*,struct Cyc_Absyn_Tqual)=Cyc_Absyn_cstar_type;void*_tmp132=({Cyc_Absyn_cstar_type(Cyc_Absyn_void_type,Cyc_Toc_mt_tq);});struct Cyc_Absyn_Tqual _tmp133=Cyc_Toc_mt_tq;_tmp131(_tmp132,_tmp133);});struct Cyc_Absyn_Exp*_tmp134=({Cyc_Absyn_var_exp(inner_handles,0U);});_tmp12F(_tmp130,_tmp134);});}
# 711
f3=({Cyc_Toc_make_field((struct _fat_ptr*)_check_null(field3_name),f3_exp);});{
# 721 "toc.cyc"
struct Cyc_Absyn_Exp*aggr=({Cyc_Absyn_new_exp((void*)({struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*_tmp138=_cycalloc(sizeof(*_tmp138));_tmp138->tag=29U,_tmp138->f1=(struct _tuple1*)_check_null(qv),_tmp138->f2=0,({struct Cyc_List_List*_tmpCEE=({struct Cyc_List_List*_tmp137=_cycalloc(sizeof(*_tmp137));_tmp137->hd=f1,({struct Cyc_List_List*_tmpCED=({struct Cyc_List_List*_tmp136=_cycalloc(sizeof(*_tmp136));_tmp136->hd=f2,({struct Cyc_List_List*_tmpCEC=({struct Cyc_List_List*_tmp135=_cycalloc(sizeof(*_tmp135));_tmp135->hd=f3,_tmp135->tl=0;_tmp135;});_tmp136->tl=_tmpCEC;});_tmp136;});_tmp137->tl=_tmpCED;});_tmp137;});_tmp138->f3=_tmpCEE;}),_tmp138->f4=0;_tmp138;}),0U);});
# 725
({void*_tmpCEF=({Cyc_Toc_effect_frame_type();});aggr->topt=_tmpCEF;});
return aggr;}}}}
# 732
static void*Cyc_Toc_cast_it_r(void*t,struct Cyc_Absyn_Exp*e){
return(void*)({struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*_tmp13D=_cycalloc(sizeof(*_tmp13D));_tmp13D->tag=14U,_tmp13D->f1=t,_tmp13D->f2=e,_tmp13D->f3=0,_tmp13D->f4=Cyc_Absyn_No_coercion;_tmp13D;});}
# 735
static void*Cyc_Toc_deref_exp_r(struct Cyc_Absyn_Exp*e){
return(void*)({struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*_tmp13F=_cycalloc(sizeof(*_tmp13F));_tmp13F->tag=20U,_tmp13F->f1=e;_tmp13F;});}
# 738
static void*Cyc_Toc_subscript_exp_r(struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2){
return(void*)({struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*_tmp141=_cycalloc(sizeof(*_tmp141));_tmp141->tag=23U,_tmp141->f1=e1,_tmp141->f2=e2;_tmp141;});}
# 741
static void*Cyc_Toc_stmt_exp_r(struct Cyc_Absyn_Stmt*s){
return(void*)({struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct*_tmp143=_cycalloc(sizeof(*_tmp143));_tmp143->tag=38U,_tmp143->f1=s;_tmp143;});}
# 744
static void*Cyc_Toc_sizeoftype_exp_r(void*t){
return(void*)({struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct*_tmp145=_cycalloc(sizeof(*_tmp145));_tmp145->tag=17U,_tmp145->f1=t;_tmp145;});}
# 747
static void*Cyc_Toc_fncall_exp_r(struct Cyc_Absyn_Exp*e,struct _fat_ptr es){
return(void*)({struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*_tmp148=_cycalloc(sizeof(*_tmp148));_tmp148->tag=10U,_tmp148->f1=e,({struct Cyc_List_List*_tmpCF1=({Cyc_List_from_array(es);});_tmp148->f2=_tmpCF1;}),_tmp148->f3=0,_tmp148->f4=1,({struct _tuple0*_tmpCF0=({struct _tuple0*_tmp147=_cycalloc(sizeof(*_tmp147));_tmp147->f1=0,_tmp147->f2=0;_tmp147;});_tmp148->f5=_tmpCF0;});_tmp148;});}
# 750
static void*Cyc_Toc_seq_stmt_r(struct Cyc_Absyn_Stmt*s1,struct Cyc_Absyn_Stmt*s2){
return(void*)({struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*_tmp14A=_cycalloc(sizeof(*_tmp14A));_tmp14A->tag=2U,_tmp14A->f1=s1,_tmp14A->f2=s2;_tmp14A;});}
# 753
static void*Cyc_Toc_conditional_exp_r(struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2,struct Cyc_Absyn_Exp*e3){
return(void*)({struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*_tmp14C=_cycalloc(sizeof(*_tmp14C));_tmp14C->tag=6U,_tmp14C->f1=e1,_tmp14C->f2=e2,_tmp14C->f3=e3;_tmp14C;});}
# 756
static void*Cyc_Toc_aggrmember_exp_r(struct Cyc_Absyn_Exp*e,struct _fat_ptr*n){
return(void*)({struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*_tmp14E=_cycalloc(sizeof(*_tmp14E));_tmp14E->tag=21U,_tmp14E->f1=e,_tmp14E->f2=n,_tmp14E->f3=0,_tmp14E->f4=0;_tmp14E;});}
# 759
static void*Cyc_Toc_unresolvedmem_exp_r(struct Cyc_Core_Opt*tdopt,struct Cyc_List_List*ds){
# 762
return(void*)({struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*_tmp150=_cycalloc(sizeof(*_tmp150));_tmp150->tag=37U,_tmp150->f1=tdopt,_tmp150->f2=ds;_tmp150;});}
# 764
static void*Cyc_Toc_goto_stmt_r(struct _fat_ptr*v){
return(void*)({struct Cyc_Absyn_Goto_s_Absyn_Raw_stmt_struct*_tmp152=_cycalloc(sizeof(*_tmp152));_tmp152->tag=8U,_tmp152->f1=v;_tmp152;});}
# 764
static struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct Cyc_Toc_zero_exp={0U,{.Int_c={5,{Cyc_Absyn_Signed,0}}}};
# 771
static struct Cyc_Absyn_Exp*Cyc_Toc_member_exp(struct Cyc_Absyn_Exp*e,struct _fat_ptr*f,unsigned loc){
void*_stmttmp8=e->r;void*_tmp154=_stmttmp8;struct Cyc_Absyn_Exp*_tmp155;if(((struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_tmp154)->tag == 20U){_LL1: _tmp155=((struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_tmp154)->f1;_LL2: {struct Cyc_Absyn_Exp*e1=_tmp155;
return({Cyc_Absyn_aggrarrow_exp(e1,f,loc);});}}else{_LL3: _LL4:
 return({Cyc_Absyn_aggrmember_exp(e,f,loc);});}_LL0:;}struct Cyc_Toc_functionSet{struct Cyc_Absyn_Exp*fchar;struct Cyc_Absyn_Exp*fshort;struct Cyc_Absyn_Exp*fint;struct Cyc_Absyn_Exp*ffloat;struct Cyc_Absyn_Exp*fdouble;struct Cyc_Absyn_Exp*flongdouble;struct Cyc_Absyn_Exp*fvoidstar;};
# 799 "toc.cyc"
struct Cyc_Toc_functionSet Cyc_Toc__zero_arr_plus_functionSet={& Cyc_Toc__zero_arr_plus_char_ev,& Cyc_Toc__zero_arr_plus_short_ev,& Cyc_Toc__zero_arr_plus_int_ev,& Cyc_Toc__zero_arr_plus_float_ev,& Cyc_Toc__zero_arr_plus_double_ev,& Cyc_Toc__zero_arr_plus_longdouble_ev,& Cyc_Toc__zero_arr_plus_voidstar_ev};
struct Cyc_Toc_functionSet Cyc_Toc__zero_arr_inplace_plus_functionSet={& Cyc_Toc__zero_arr_inplace_plus_char_ev,& Cyc_Toc__zero_arr_inplace_plus_short_ev,& Cyc_Toc__zero_arr_inplace_plus_int_ev,& Cyc_Toc__zero_arr_inplace_plus_float_ev,& Cyc_Toc__zero_arr_inplace_plus_double_ev,& Cyc_Toc__zero_arr_inplace_plus_longdouble_ev,& Cyc_Toc__zero_arr_inplace_plus_voidstar_ev};
struct Cyc_Toc_functionSet Cyc_Toc__zero_arr_inplace_plus_post_functionSet={& Cyc_Toc__zero_arr_inplace_plus_post_char_ev,& Cyc_Toc__zero_arr_inplace_plus_post_short_ev,& Cyc_Toc__zero_arr_inplace_plus_post_int_ev,& Cyc_Toc__zero_arr_inplace_plus_post_float_ev,& Cyc_Toc__zero_arr_inplace_plus_post_double_ev,& Cyc_Toc__zero_arr_inplace_plus_post_longdouble_ev,& Cyc_Toc__zero_arr_inplace_plus_post_voidstar_ev};
struct Cyc_Toc_functionSet Cyc_Toc__get_zero_arr_size_functionSet={& Cyc_Toc__get_zero_arr_size_char_ev,& Cyc_Toc__get_zero_arr_size_short_ev,& Cyc_Toc__get_zero_arr_size_int_ev,& Cyc_Toc__get_zero_arr_size_float_ev,& Cyc_Toc__get_zero_arr_size_double_ev,& Cyc_Toc__get_zero_arr_size_longdouble_ev,& Cyc_Toc__get_zero_arr_size_voidstar_ev};
# 805
static struct Cyc_Absyn_Exp*Cyc_Toc_getFunctionType(struct Cyc_Toc_functionSet*fS,void*t){
void*_stmttmp9=({Cyc_Tcutil_compress(t);});void*_tmp157=_stmttmp9;switch(*((int*)_tmp157)){case 0U: switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp157)->f1)){case 1U: switch(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp157)->f1)->f2){case Cyc_Absyn_Char_sz: _LL1: _LL2:
 return fS->fchar;case Cyc_Absyn_Short_sz: _LL3: _LL4:
 return fS->fshort;case Cyc_Absyn_Int_sz: _LL5: _LL6:
 return fS->fint;default: goto _LLF;}case 2U: switch(((struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp157)->f1)->f1){case 0U: _LL7: _LL8:
 return fS->ffloat;case 1U: _LL9: _LLA:
 return fS->fdouble;default: _LLB: _LLC:
 return fS->flongdouble;}default: goto _LLF;}case 3U: _LLD: _LLE:
 return fS->fvoidstar;default: _LLF: _LL10:
({struct Cyc_String_pa_PrintArg_struct _tmp15B=({struct Cyc_String_pa_PrintArg_struct _tmpC66;_tmpC66.tag=0U,({struct _fat_ptr _tmpCF2=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t);}));_tmpC66.f1=_tmpCF2;});_tmpC66;});void*_tmp158[1U];_tmp158[0]=& _tmp15B;({int(*_tmpCF4)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp15A)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmp15A;});struct _fat_ptr _tmpCF3=({const char*_tmp159="expression type %s (not int, float, double, or pointer)";_tag_fat(_tmp159,sizeof(char),56U);});_tmpCF4(_tmpCF3,_tag_fat(_tmp158,sizeof(void*),1U));});});}_LL0:;}
# 817
static struct Cyc_Absyn_Exp*Cyc_Toc_getFunctionRemovePointer(struct Cyc_Toc_functionSet*fS,struct Cyc_Absyn_Exp*arr){
void*_stmttmpA=({Cyc_Tcutil_compress((void*)_check_null(arr->topt));});void*_tmp15D=_stmttmpA;void*_tmp15E;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp15D)->tag == 3U){_LL1: _tmp15E=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp15D)->f1).elt_type;_LL2: {void*et=_tmp15E;
return({Cyc_Toc_getFunctionType(fS,et);});}}else{_LL3: _LL4:
({void*_tmp15F=0U;({int(*_tmpCF6)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp161)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmp161;});struct _fat_ptr _tmpCF5=({const char*_tmp160="impossible type (not pointer)";_tag_fat(_tmp160,sizeof(char),30U);});_tmpCF6(_tmpCF5,_tag_fat(_tmp15F,sizeof(void*),0U));});});}_LL0:;}
# 817
int Cyc_Toc_is_zero(struct Cyc_Absyn_Exp*e){
# 828
void*_stmttmpB=e->r;void*_tmp163=_stmttmpB;struct Cyc_List_List*_tmp164;struct Cyc_List_List*_tmp165;struct Cyc_List_List*_tmp166;struct Cyc_List_List*_tmp167;struct Cyc_List_List*_tmp168;struct Cyc_Absyn_Exp*_tmp169;long long _tmp16A;int _tmp16B;short _tmp16C;struct _fat_ptr _tmp16D;char _tmp16E;switch(*((int*)_tmp163)){case 0U: switch(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp163)->f1).Null_c).tag){case 2U: _LL1: _tmp16E=(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp163)->f1).Char_c).val).f2;_LL2: {char c=_tmp16E;
return(int)c == (int)'\000';}case 3U: _LL3: _tmp16D=((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp163)->f1).Wchar_c).val;_LL4: {struct _fat_ptr s=_tmp16D;
# 831
unsigned long l=({Cyc_strlen((struct _fat_ptr)s);});
int i=0;
if(l >= (unsigned long)2 &&(int)*((const char*)_check_fat_subscript(s,sizeof(char),0))== (int)'\\'){
if((int)*((const char*)_check_fat_subscript(s,sizeof(char),1))== (int)'0')i=2;else{
if(((int)((const char*)s.curr)[1]== (int)'x' && l >= (unsigned long)3)&&(int)*((const char*)_check_fat_subscript(s,sizeof(char),2))== (int)'0')i=3;else{
return 0;}}
for(0;(unsigned long)i < l;++ i){
if((int)*((const char*)_check_fat_subscript(s,sizeof(char),i))!= (int)'0')return 0;}
# 837
return 1;}else{
# 841
return 0;}}case 4U: _LL5: _tmp16C=(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp163)->f1).Short_c).val).f2;_LL6: {short i=_tmp16C;
return(int)i == 0;}case 5U: _LL7: _tmp16B=(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp163)->f1).Int_c).val).f2;_LL8: {int i=_tmp16B;
return i == 0;}case 6U: _LL9: _tmp16A=(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp163)->f1).LongLong_c).val).f2;_LLA: {long long i=_tmp16A;
return i == (long long)0;}case 1U: _LLD: _LLE:
# 846
 return 1;default: goto _LL1B;}case 2U: _LLB: _LLC:
# 845
 goto _LLE;case 14U: _LLF: _tmp169=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp163)->f2;_LL10: {struct Cyc_Absyn_Exp*e1=_tmp169;
# 847
return({Cyc_Toc_is_zero(e1);});}case 24U: _LL11: _tmp168=((struct Cyc_Absyn_Tuple_e_Absyn_Raw_exp_struct*)_tmp163)->f1;_LL12: {struct Cyc_List_List*es=_tmp168;
return({({int(*_tmpCF7)(int(*pred)(struct Cyc_Absyn_Exp*),struct Cyc_List_List*x)=({int(*_tmp16F)(int(*pred)(struct Cyc_Absyn_Exp*),struct Cyc_List_List*x)=(int(*)(int(*pred)(struct Cyc_Absyn_Exp*),struct Cyc_List_List*x))Cyc_List_forall;_tmp16F;});_tmpCF7(Cyc_Toc_is_zero,es);});});}case 26U: _LL13: _tmp167=((struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*)_tmp163)->f1;_LL14: {struct Cyc_List_List*dles=_tmp167;
_tmp166=dles;goto _LL16;}case 29U: _LL15: _tmp166=((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_tmp163)->f3;_LL16: {struct Cyc_List_List*dles=_tmp166;
_tmp165=dles;goto _LL18;}case 25U: _LL17: _tmp165=((struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct*)_tmp163)->f2;_LL18: {struct Cyc_List_List*dles=_tmp165;
_tmp164=dles;goto _LL1A;}case 37U: _LL19: _tmp164=((struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*)_tmp163)->f2;_LL1A: {struct Cyc_List_List*dles=_tmp164;
# 853
for(0;dles != 0;dles=dles->tl){
if(!({Cyc_Toc_is_zero((*((struct _tuple21*)dles->hd)).f2);}))return 0;}
# 853
return 1;}default: _LL1B: _LL1C:
# 856
 return 0;}_LL0:;}
# 861
static struct _fat_ptr Cyc_Toc_collapse_qvar(struct _fat_ptr*s,struct _tuple1*x){
struct _fat_ptr*_tmp172;union Cyc_Absyn_Nmspace _tmp171;_LL1: _tmp171=x->f1;_tmp172=x->f2;_LL2: {union Cyc_Absyn_Nmspace ns=_tmp171;struct _fat_ptr*v=_tmp172;
union Cyc_Absyn_Nmspace _tmp173=ns;struct Cyc_List_List*_tmp174;struct Cyc_List_List*_tmp175;struct Cyc_List_List*_tmp176;switch((_tmp173.Abs_n).tag){case 4U: _LL4: _LL5:
 _tmp176=0;goto _LL7;case 1U: _LL6: _tmp176=(_tmp173.Rel_n).val;_LL7: {struct Cyc_List_List*vs=_tmp176;
_tmp175=vs;goto _LL9;}case 2U: _LL8: _tmp175=(_tmp173.Abs_n).val;_LL9: {struct Cyc_List_List*vs=_tmp175;
_tmp174=vs;goto _LLB;}default: _LLA: _tmp174=(_tmp173.C_n).val;_LLB: {struct Cyc_List_List*vs=_tmp174;
# 870
if(vs == 0)
return(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp179=({struct Cyc_String_pa_PrintArg_struct _tmpC68;_tmpC68.tag=0U,_tmpC68.f1=(struct _fat_ptr)((struct _fat_ptr)*s);_tmpC68;});struct Cyc_String_pa_PrintArg_struct _tmp17A=({struct Cyc_String_pa_PrintArg_struct _tmpC67;_tmpC67.tag=0U,_tmpC67.f1=(struct _fat_ptr)((struct _fat_ptr)*v);_tmpC67;});void*_tmp177[2U];_tmp177[0]=& _tmp179,_tmp177[1]=& _tmp17A;({struct _fat_ptr _tmpCF8=({const char*_tmp178="%s_%s_struct";_tag_fat(_tmp178,sizeof(char),13U);});Cyc_aprintf(_tmpCF8,_tag_fat(_tmp177,sizeof(void*),2U));});});{
# 870
struct _RegionHandle _tmp17B=_new_region("r");struct _RegionHandle*r=& _tmp17B;_push_region(r);
# 873
{struct _fat_ptr _tmp182=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp17E=({struct Cyc_String_pa_PrintArg_struct _tmpC6B;_tmpC6B.tag=0U,_tmpC6B.f1=(struct _fat_ptr)((struct _fat_ptr)*s);_tmpC6B;});struct Cyc_String_pa_PrintArg_struct _tmp17F=({struct Cyc_String_pa_PrintArg_struct _tmpC6A;_tmpC6A.tag=0U,({struct _fat_ptr _tmpCFB=(struct _fat_ptr)((struct _fat_ptr)({({struct _RegionHandle*_tmpCFA=r;struct Cyc_List_List*_tmpCF9=vs;Cyc_rstr_sepstr(_tmpCFA,_tmpCF9,({const char*_tmp181="_";_tag_fat(_tmp181,sizeof(char),2U);}));});}));_tmpC6A.f1=_tmpCFB;});_tmpC6A;});struct Cyc_String_pa_PrintArg_struct _tmp180=({struct Cyc_String_pa_PrintArg_struct _tmpC69;_tmpC69.tag=0U,_tmpC69.f1=(struct _fat_ptr)((struct _fat_ptr)*v);_tmpC69;});void*_tmp17C[3U];_tmp17C[0]=& _tmp17E,_tmp17C[1]=& _tmp17F,_tmp17C[2]=& _tmp180;({struct _fat_ptr _tmpCFC=({const char*_tmp17D="%s_%s_%s_struct";_tag_fat(_tmp17D,sizeof(char),16U);});Cyc_aprintf(_tmpCFC,_tag_fat(_tmp17C,sizeof(void*),3U));});});_npop_handler(0U);return _tmp182;};_pop_region();}}}_LL3:;}}struct _tuple24{struct Cyc_Toc_TocState*f1;struct _tuple16*f2;};
# 883
static struct _tuple1*Cyc_Toc_collapse_qvars_body(struct _RegionHandle*d,struct _tuple24*env){
# 886
struct _tuple24 _stmttmpC=*env;struct _tuple16*_tmp185;struct Cyc_Dict_Dict*_tmp184;_LL1: _tmp184=(_stmttmpC.f1)->qvar_tags;_tmp185=_stmttmpC.f2;_LL2: {struct Cyc_Dict_Dict*qvs=_tmp184;struct _tuple16*pair=_tmp185;
struct _tuple16 _stmttmpD=*pair;struct _tuple1*_tmp187;struct _tuple1*_tmp186;_LL4: _tmp186=_stmttmpD.f1;_tmp187=_stmttmpD.f2;_LL5: {struct _tuple1*fieldname=_tmp186;struct _tuple1*dtname=_tmp187;
struct _handler_cons _tmp188;_push_handler(& _tmp188);{int _tmp18A=0;if(setjmp(_tmp188.handler))_tmp18A=1;if(!_tmp18A){{struct _tuple1*_tmp18C=({({struct _tuple1*(*_tmpCFE)(struct Cyc_Dict_Dict d,int(*cmp)(struct _tuple16*,struct _tuple16*),struct _tuple16*k)=({struct _tuple1*(*_tmp18B)(struct Cyc_Dict_Dict d,int(*cmp)(struct _tuple16*,struct _tuple16*),struct _tuple16*k)=(struct _tuple1*(*)(struct Cyc_Dict_Dict d,int(*cmp)(struct _tuple16*,struct _tuple16*),struct _tuple16*k))Cyc_Dict_lookup_other;_tmp18B;});struct Cyc_Dict_Dict _tmpCFD=*qvs;_tmpCFE(_tmpCFD,Cyc_Toc_qvar_tag_cmp,pair);});});_npop_handler(0U);return _tmp18C;};_pop_handler();}else{void*_tmp189=(void*)Cyc_Core_get_exn_thrown();void*_tmp18D=_tmp189;void*_tmp18E;if(((struct Cyc_Dict_Absent_exn_struct*)_tmp18D)->tag == Cyc_Dict_Absent){_LL7: _LL8: {
# 890
struct _tuple16*new_pair=({struct _tuple16*_tmp194=_cycalloc(sizeof(*_tmp194));_tmp194->f1=fieldname,_tmp194->f2=dtname;_tmp194;});
struct _fat_ptr*_tmp190;union Cyc_Absyn_Nmspace _tmp18F;_LLC: _tmp18F=fieldname->f1;_tmp190=fieldname->f2;_LLD: {union Cyc_Absyn_Nmspace nmspace=_tmp18F;struct _fat_ptr*fieldvar=_tmp190;
struct _fat_ptr newvar=({Cyc_Toc_collapse_qvar(fieldvar,dtname);});
struct _tuple1*res=({struct _tuple1*_tmp193=_cycalloc(sizeof(*_tmp193));_tmp193->f1=nmspace,({struct _fat_ptr*_tmpCFF=({struct _fat_ptr*_tmp192=_cycalloc(sizeof(*_tmp192));*_tmp192=newvar;_tmp192;});_tmp193->f2=_tmpCFF;});_tmp193;});
({struct Cyc_Dict_Dict _tmpD03=({({struct Cyc_Dict_Dict(*_tmpD02)(struct Cyc_Dict_Dict d,struct _tuple16*k,struct _tuple1*v)=({struct Cyc_Dict_Dict(*_tmp191)(struct Cyc_Dict_Dict d,struct _tuple16*k,struct _tuple1*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _tuple16*k,struct _tuple1*v))Cyc_Dict_insert;_tmp191;});struct Cyc_Dict_Dict _tmpD01=*qvs;struct _tuple16*_tmpD00=new_pair;_tmpD02(_tmpD01,_tmpD00,res);});});*qvs=_tmpD03;});
return res;}}}else{_LL9: _tmp18E=_tmp18D;_LLA: {void*exn=_tmp18E;(int)_rethrow(exn);}}_LL6:;}}}}}
# 898
static struct _tuple1*Cyc_Toc_collapse_qvars(struct _tuple1*fieldname,struct _tuple1*dtname){
struct _tuple16 env=({struct _tuple16 _tmpC6C;_tmpC6C.f1=fieldname,_tmpC6C.f2=dtname;_tmpC6C;});
return({({struct _tuple1*(*_tmp196)(struct _tuple16*arg,struct _tuple1*(*f)(struct _RegionHandle*,struct _tuple24*))=(struct _tuple1*(*)(struct _tuple16*arg,struct _tuple1*(*f)(struct _RegionHandle*,struct _tuple24*)))Cyc_Toc_use_toc_state;_tmp196;})(& env,Cyc_Toc_collapse_qvars_body);});}
# 905
static struct Cyc_Absyn_Aggrdecl*Cyc_Toc_make_c_struct_defn(struct _fat_ptr*name,struct Cyc_List_List*fs){
return({struct Cyc_Absyn_Aggrdecl*_tmp19A=_cycalloc(sizeof(*_tmp19A));_tmp19A->kind=Cyc_Absyn_StructA,_tmp19A->sc=Cyc_Absyn_Public,_tmp19A->tvs=0,_tmp19A->attributes=0,_tmp19A->expected_mem_kind=0,({
# 908
struct _tuple1*_tmpD06=({struct _tuple1*_tmp198=_cycalloc(sizeof(*_tmp198));({union Cyc_Absyn_Nmspace _tmpD05=({Cyc_Absyn_Rel_n(0);});_tmp198->f1=_tmpD05;}),_tmp198->f2=name;_tmp198;});_tmp19A->name=_tmpD06;}),({
struct Cyc_Absyn_AggrdeclImpl*_tmpD04=({struct Cyc_Absyn_AggrdeclImpl*_tmp199=_cycalloc(sizeof(*_tmp199));_tmp199->exist_vars=0,_tmp199->rgn_po=0,_tmp199->fields=fs,_tmp199->tagged=0;_tmp199;});_tmp19A->impl=_tmpD04;});_tmp19A;});}struct _tuple25{struct Cyc_Toc_TocState*f1;struct Cyc_List_List*f2;};struct _tuple26{void*f1;struct Cyc_List_List*f2;};
# 914
static void*Cyc_Toc_add_tuple_type_body(struct _RegionHandle*d,struct _tuple25*env){
# 917
struct _tuple25 _stmttmpE=*env;struct Cyc_List_List*_tmp19D;struct Cyc_List_List**_tmp19C;_LL1: _tmp19C=(_stmttmpE.f1)->tuple_types;_tmp19D=_stmttmpE.f2;_LL2: {struct Cyc_List_List**tuple_types=_tmp19C;struct Cyc_List_List*tqs0=_tmp19D;
# 919
{struct Cyc_List_List*tts=*tuple_types;for(0;tts != 0;tts=tts->tl){
struct _tuple26*_stmttmpF=(struct _tuple26*)tts->hd;struct Cyc_List_List*_tmp19F;void*_tmp19E;_LL4: _tmp19E=_stmttmpF->f1;_tmp19F=_stmttmpF->f2;_LL5: {void*x=_tmp19E;struct Cyc_List_List*ts=_tmp19F;
struct Cyc_List_List*tqs=tqs0;
for(0;tqs != 0 && ts != 0;(tqs=tqs->tl,ts=ts->tl)){
if(!({Cyc_Unify_unify((*((struct _tuple14*)tqs->hd)).f2,(void*)ts->hd);}))
break;}
# 922
if(
# 925
tqs == 0 && ts == 0)
return x;}}}{
# 930
struct _fat_ptr*xname=({struct _fat_ptr*_tmp1AE=_cycalloc(sizeof(*_tmp1AE));({struct _fat_ptr _tmpD08=(struct _fat_ptr)({struct Cyc_Int_pa_PrintArg_struct _tmp1AD=({struct Cyc_Int_pa_PrintArg_struct _tmpC6D;_tmpC6D.tag=1U,_tmpC6D.f1=(unsigned long)Cyc_Toc_tuple_type_counter ++;_tmpC6D;});void*_tmp1AB[1U];_tmp1AB[0]=& _tmp1AD;({struct _fat_ptr _tmpD07=({const char*_tmp1AC="_tuple%d";_tag_fat(_tmp1AC,sizeof(char),9U);});Cyc_aprintf(_tmpD07,_tag_fat(_tmp1AB,sizeof(void*),1U));});});*_tmp1AE=_tmpD08;});_tmp1AE;});
struct Cyc_List_List*ts=({({struct Cyc_List_List*(*_tmpD0A)(struct _RegionHandle*,void*(*f)(struct _tuple14*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp1AA)(struct _RegionHandle*,void*(*f)(struct _tuple14*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct _RegionHandle*,void*(*f)(struct _tuple14*),struct Cyc_List_List*x))Cyc_List_rmap;_tmp1AA;});struct _RegionHandle*_tmpD09=d;_tmpD0A(_tmpD09,Cyc_Tcutil_snd_tqt,tqs0);});});
struct Cyc_List_List*fs=0;
struct Cyc_List_List*ts2=ts;
{int i=1;for(0;ts2 != 0;(ts2=ts2->tl,i ++)){
fs=({struct Cyc_List_List*_tmp1A1=_cycalloc(sizeof(*_tmp1A1));({struct Cyc_Absyn_Aggrfield*_tmpD0C=({struct Cyc_Absyn_Aggrfield*_tmp1A0=_cycalloc(sizeof(*_tmp1A0));({struct _fat_ptr*_tmpD0B=({Cyc_Absyn_fieldname(i);});_tmp1A0->name=_tmpD0B;}),_tmp1A0->tq=Cyc_Toc_mt_tq,_tmp1A0->type=(void*)ts2->hd,_tmp1A0->width=0,_tmp1A0->attributes=0,_tmp1A0->requires_clause=0;_tmp1A0;});_tmp1A1->hd=_tmpD0C;}),_tmp1A1->tl=fs;_tmp1A1;});}}
fs=({Cyc_List_imp_rev(fs);});{
struct Cyc_Absyn_Aggrdecl*sd=({Cyc_Toc_make_c_struct_defn(xname,fs);});
void*ans=({void*(*_tmp1A6)(union Cyc_Absyn_AggrInfo,struct Cyc_List_List*args)=Cyc_Absyn_aggr_type;union Cyc_Absyn_AggrInfo _tmp1A7=({Cyc_Absyn_KnownAggr(({struct Cyc_Absyn_Aggrdecl**_tmp1A8=_cycalloc(sizeof(*_tmp1A8));*_tmp1A8=sd;_tmp1A8;}));});struct Cyc_List_List*_tmp1A9=0;_tmp1A6(_tmp1A7,_tmp1A9);});
Cyc_Toc_result_decls=({struct Cyc_List_List*_tmp1A3=_cycalloc(sizeof(*_tmp1A3));({struct Cyc_Absyn_Decl*_tmpD0D=({Cyc_Absyn_new_decl((void*)({struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct*_tmp1A2=_cycalloc(sizeof(*_tmp1A2));_tmp1A2->tag=5U,_tmp1A2->f1=sd;_tmp1A2;}),0U);});_tmp1A3->hd=_tmpD0D;}),_tmp1A3->tl=Cyc_Toc_result_decls;_tmp1A3;});
({struct Cyc_List_List*_tmpD0F=({struct Cyc_List_List*_tmp1A5=_region_malloc(d,sizeof(*_tmp1A5));({struct _tuple26*_tmpD0E=({struct _tuple26*_tmp1A4=_region_malloc(d,sizeof(*_tmp1A4));_tmp1A4->f1=ans,_tmp1A4->f2=ts;_tmp1A4;});_tmp1A5->hd=_tmpD0E;}),_tmp1A5->tl=*tuple_types;_tmp1A5;});*tuple_types=_tmpD0F;});
return ans;}}}}
# 943
static void*Cyc_Toc_add_tuple_type(struct Cyc_List_List*tqs0){
return({({void*(*_tmpD10)(struct Cyc_List_List*arg,void*(*f)(struct _RegionHandle*,struct _tuple25*))=({void*(*_tmp1B0)(struct Cyc_List_List*arg,void*(*f)(struct _RegionHandle*,struct _tuple25*))=(void*(*)(struct Cyc_List_List*arg,void*(*f)(struct _RegionHandle*,struct _tuple25*)))Cyc_Toc_use_toc_state;_tmp1B0;});_tmpD10(tqs0,Cyc_Toc_add_tuple_type_body);});});}struct _tuple27{enum Cyc_Absyn_AggrKind f1;struct Cyc_List_List*f2;};struct _tuple28{struct Cyc_Toc_TocState*f1;struct _tuple27*f2;};struct _tuple29{void*f1;enum Cyc_Absyn_AggrKind f2;struct Cyc_List_List*f3;};
# 948
static void*Cyc_Toc_add_anon_aggr_type_body(struct _RegionHandle*d,struct _tuple28*env){
# 951
struct Cyc_List_List*_tmp1B4;enum Cyc_Absyn_AggrKind _tmp1B3;struct Cyc_List_List**_tmp1B2;_LL1: _tmp1B2=(env->f1)->anon_aggr_types;_tmp1B3=(env->f2)->f1;_tmp1B4=(env->f2)->f2;_LL2: {struct Cyc_List_List**anon_aggr_types=_tmp1B2;enum Cyc_Absyn_AggrKind ak=_tmp1B3;struct Cyc_List_List*fs=_tmp1B4;
# 953
{struct Cyc_List_List*ts=*anon_aggr_types;for(0;ts != 0;ts=ts->tl){
struct _tuple29*_stmttmp10=(struct _tuple29*)ts->hd;struct Cyc_List_List*_tmp1B7;enum Cyc_Absyn_AggrKind _tmp1B6;void*_tmp1B5;_LL4: _tmp1B5=_stmttmp10->f1;_tmp1B6=_stmttmp10->f2;_tmp1B7=_stmttmp10->f3;_LL5: {void*x=_tmp1B5;enum Cyc_Absyn_AggrKind ak2=_tmp1B6;struct Cyc_List_List*fs2=_tmp1B7;
if((int)ak != (int)ak2)
continue;
# 955
if(!({({int(*_tmpD12)(int(*cmp)(struct Cyc_Absyn_Aggrfield*,struct Cyc_Absyn_Aggrfield*),struct Cyc_List_List*l1,struct Cyc_List_List*l2)=({
# 957
int(*_tmp1B8)(int(*cmp)(struct Cyc_Absyn_Aggrfield*,struct Cyc_Absyn_Aggrfield*),struct Cyc_List_List*l1,struct Cyc_List_List*l2)=(int(*)(int(*cmp)(struct Cyc_Absyn_Aggrfield*,struct Cyc_Absyn_Aggrfield*),struct Cyc_List_List*l1,struct Cyc_List_List*l2))Cyc_List_list_cmp;_tmp1B8;});struct Cyc_List_List*_tmpD11=fs2;_tmpD12(Cyc_Tcutil_aggrfield_cmp,_tmpD11,fs);});}))
return x;}}}{
# 962
struct _fat_ptr*xname=({struct _fat_ptr*_tmp1C4=_cycalloc(sizeof(*_tmp1C4));({struct _fat_ptr _tmpD14=(struct _fat_ptr)({struct Cyc_Int_pa_PrintArg_struct _tmp1C3=({struct Cyc_Int_pa_PrintArg_struct _tmpC6E;_tmpC6E.tag=1U,_tmpC6E.f1=(unsigned long)Cyc_Toc_tuple_type_counter ++;_tmpC6E;});void*_tmp1C1[1U];_tmp1C1[0]=& _tmp1C3;({struct _fat_ptr _tmpD13=({const char*_tmp1C2="_tuple%d";_tag_fat(_tmp1C2,sizeof(char),9U);});Cyc_aprintf(_tmpD13,_tag_fat(_tmp1C1,sizeof(void*),1U));});});*_tmp1C4=_tmpD14;});_tmp1C4;});
struct Cyc_Absyn_Aggrdecl*sd=({Cyc_Toc_make_c_struct_defn(xname,fs);});
sd->kind=ak;{
void*ans=({void*(*_tmp1BD)(union Cyc_Absyn_AggrInfo,struct Cyc_List_List*args)=Cyc_Absyn_aggr_type;union Cyc_Absyn_AggrInfo _tmp1BE=({Cyc_Absyn_KnownAggr(({struct Cyc_Absyn_Aggrdecl**_tmp1BF=_cycalloc(sizeof(*_tmp1BF));*_tmp1BF=sd;_tmp1BF;}));});struct Cyc_List_List*_tmp1C0=0;_tmp1BD(_tmp1BE,_tmp1C0);});
Cyc_Toc_result_decls=({struct Cyc_List_List*_tmp1BA=_cycalloc(sizeof(*_tmp1BA));({struct Cyc_Absyn_Decl*_tmpD15=({Cyc_Absyn_new_decl((void*)({struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct*_tmp1B9=_cycalloc(sizeof(*_tmp1B9));_tmp1B9->tag=5U,_tmp1B9->f1=sd;_tmp1B9;}),0U);});_tmp1BA->hd=_tmpD15;}),_tmp1BA->tl=Cyc_Toc_result_decls;_tmp1BA;});
({struct Cyc_List_List*_tmpD17=({struct Cyc_List_List*_tmp1BC=_region_malloc(d,sizeof(*_tmp1BC));({struct _tuple29*_tmpD16=({struct _tuple29*_tmp1BB=_region_malloc(d,sizeof(*_tmp1BB));_tmp1BB->f1=ans,_tmp1BB->f2=ak,_tmp1BB->f3=fs;_tmp1BB;});_tmp1BC->hd=_tmpD16;}),_tmp1BC->tl=*anon_aggr_types;_tmp1BC;});*anon_aggr_types=_tmpD17;});
return ans;}}}}
# 970
static void*Cyc_Toc_add_anon_aggr_type(enum Cyc_Absyn_AggrKind ak,struct Cyc_List_List*fs){
return({({void*(*_tmpD18)(struct _tuple27*arg,void*(*f)(struct _RegionHandle*,struct _tuple28*))=({void*(*_tmp1C6)(struct _tuple27*arg,void*(*f)(struct _RegionHandle*,struct _tuple28*))=(void*(*)(struct _tuple27*arg,void*(*f)(struct _RegionHandle*,struct _tuple28*)))Cyc_Toc_use_toc_state;_tmp1C6;});_tmpD18(({struct _tuple27*_tmp1C7=_cycalloc(sizeof(*_tmp1C7));_tmp1C7->f1=ak,_tmp1C7->f2=fs;_tmp1C7;}),Cyc_Toc_add_anon_aggr_type_body);});});}struct _tuple30{struct _tuple1*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct Cyc_List_List*f4;};struct _tuple31{struct Cyc_Toc_TocState*f1;struct _tuple30*f2;};struct _tuple32{struct _tuple1*f1;struct Cyc_List_List*f2;void*f3;};
# 979
static void*Cyc_Toc_add_struct_type_body(struct _RegionHandle*d,struct _tuple31*env){
# 988
struct _tuple31 _stmttmp11=*env;struct Cyc_List_List*_tmp1CD;struct Cyc_List_List*_tmp1CC;struct Cyc_List_List*_tmp1CB;struct _tuple1*_tmp1CA;struct Cyc_List_List**_tmp1C9;_LL1: _tmp1C9=(_stmttmp11.f1)->abs_struct_types;_tmp1CA=(_stmttmp11.f2)->f1;_tmp1CB=(_stmttmp11.f2)->f2;_tmp1CC=(_stmttmp11.f2)->f3;_tmp1CD=(_stmttmp11.f2)->f4;_LL2: {struct Cyc_List_List**abs_struct_types=_tmp1C9;struct _tuple1*struct_name=_tmp1CA;struct Cyc_List_List*type_vars=_tmp1CB;struct Cyc_List_List*type_args=_tmp1CC;struct Cyc_List_List*fields=_tmp1CD;
# 992
{struct Cyc_List_List*tts=*abs_struct_types;for(0;tts != 0;tts=tts->tl){
struct _tuple32*_stmttmp12=(struct _tuple32*)tts->hd;void*_tmp1D0;struct Cyc_List_List*_tmp1CF;struct _tuple1*_tmp1CE;_LL4: _tmp1CE=_stmttmp12->f1;_tmp1CF=_stmttmp12->f2;_tmp1D0=_stmttmp12->f3;_LL5: {struct _tuple1*x=_tmp1CE;struct Cyc_List_List*ts2=_tmp1CF;void*t=_tmp1D0;
if(({Cyc_Absyn_qvar_cmp(x,struct_name);})== 0 &&({
int _tmpD19=({Cyc_List_length(type_args);});_tmpD19 == ({Cyc_List_length(ts2);});})){
int okay=1;
{struct Cyc_List_List*ts=type_args;for(0;ts != 0;(ts=ts->tl,ts2=ts2->tl)){
void*t=(void*)ts->hd;
void*t2=(void*)((struct Cyc_List_List*)_check_null(ts2))->hd;
{struct Cyc_Absyn_Kind*_stmttmp13=({Cyc_Tcutil_type_kind(t);});struct Cyc_Absyn_Kind*_tmp1D1=_stmttmp13;switch(((struct Cyc_Absyn_Kind*)_tmp1D1)->kind){case Cyc_Absyn_EffKind: _LL7: _LL8:
 goto _LLA;case Cyc_Absyn_RgnKind: _LL9: _LLA:
 continue;default: _LLB: _LLC:
# 1005
 if(({Cyc_Unify_unify(t,t2);})||({int(*_tmp1D2)(void*,void*)=Cyc_Unify_unify;void*_tmp1D3=({Cyc_Toc_typ_to_c(t);});void*_tmp1D4=({Cyc_Toc_typ_to_c(t2);});_tmp1D2(_tmp1D3,_tmp1D4);}))
continue;
# 1005
okay=0;
# 1008
goto _LL6;}_LL6:;}
# 1010
break;}}
# 1012
if(okay)
return t;}}}}{
# 1019
struct _fat_ptr*xname=({struct _fat_ptr*_tmp1E8=_cycalloc(sizeof(*_tmp1E8));({struct _fat_ptr _tmpD1B=(struct _fat_ptr)({struct Cyc_Int_pa_PrintArg_struct _tmp1E7=({struct Cyc_Int_pa_PrintArg_struct _tmpC6F;_tmpC6F.tag=1U,_tmpC6F.f1=(unsigned long)Cyc_Toc_tuple_type_counter ++;_tmpC6F;});void*_tmp1E5[1U];_tmp1E5[0]=& _tmp1E7;({struct _fat_ptr _tmpD1A=({const char*_tmp1E6="_tuple%d";_tag_fat(_tmp1E6,sizeof(char),9U);});Cyc_aprintf(_tmpD1A,_tag_fat(_tmp1E5,sizeof(void*),1U));});});*_tmp1E8=_tmpD1B;});_tmp1E8;});
void*x=({Cyc_Absyn_strct(xname);});
struct Cyc_List_List*fs=0;
# 1023
({struct Cyc_List_List*_tmpD1D=({struct Cyc_List_List*_tmp1D6=_region_malloc(d,sizeof(*_tmp1D6));({struct _tuple32*_tmpD1C=({struct _tuple32*_tmp1D5=_region_malloc(d,sizeof(*_tmp1D5));_tmp1D5->f1=struct_name,_tmp1D5->f2=type_args,_tmp1D5->f3=x;_tmp1D5;});_tmp1D6->hd=_tmpD1C;}),_tmp1D6->tl=*abs_struct_types;_tmp1D6;});*abs_struct_types=_tmpD1D;});{
# 1026
struct _RegionHandle _tmp1D7=_new_region("r");struct _RegionHandle*r=& _tmp1D7;_push_region(r);
{struct Cyc_List_List*inst=({Cyc_List_rzip(r,r,type_vars,type_args);});
for(0;fields != 0;fields=fields->tl){
struct Cyc_Absyn_Aggrfield*f=(struct Cyc_Absyn_Aggrfield*)fields->hd;
void*t=f->type;
struct Cyc_List_List*atts=f->attributes;
# 1036
if((fields->tl == 0 &&({int(*_tmp1D8)(struct Cyc_Absyn_Kind*,struct Cyc_Absyn_Kind*)=Cyc_Tcutil_kind_leq;struct Cyc_Absyn_Kind*_tmp1D9=& Cyc_Tcutil_ak;struct Cyc_Absyn_Kind*_tmp1DA=({Cyc_Tcutil_type_kind(t);});_tmp1D8(_tmp1D9,_tmp1DA);}))&& !({Cyc_Tcutil_is_array_type(t);}))
# 1039
atts=({struct Cyc_List_List*_tmp1DC=_cycalloc(sizeof(*_tmp1DC));({void*_tmpD1E=(void*)({struct Cyc_Absyn_Aligned_att_Absyn_Attribute_struct*_tmp1DB=_cycalloc(sizeof(*_tmp1DB));_tmp1DB->tag=6U,_tmp1DB->f1=0;_tmp1DB;});_tmp1DC->hd=_tmpD1E;}),_tmp1DC->tl=atts;_tmp1DC;});
# 1036
t=({void*(*_tmp1DD)(void*)=Cyc_Toc_typ_to_c;void*_tmp1DE=({Cyc_Tcutil_rsubstitute(r,inst,t);});_tmp1DD(_tmp1DE);});
# 1043
if(({Cyc_Unify_unify(t,Cyc_Absyn_void_type);}))
t=(void*)({struct Cyc_Absyn_ArrayType_Absyn_Type_struct*_tmp1DF=_cycalloc(sizeof(*_tmp1DF));_tmp1DF->tag=4U,({void*_tmpD21=({Cyc_Toc_void_star_type();});(_tmp1DF->f1).elt_type=_tmpD21;}),({struct Cyc_Absyn_Tqual _tmpD20=({Cyc_Absyn_empty_tqual(0U);});(_tmp1DF->f1).tq=_tmpD20;}),({
struct Cyc_Absyn_Exp*_tmpD1F=({Cyc_Absyn_uint_exp(0U,0U);});(_tmp1DF->f1).num_elts=_tmpD1F;}),(_tmp1DF->f1).zero_term=Cyc_Absyn_false_type,(_tmp1DF->f1).zt_loc=0U;_tmp1DF;});
# 1043
fs=({struct Cyc_List_List*_tmp1E1=_cycalloc(sizeof(*_tmp1E1));
# 1047
({struct Cyc_Absyn_Aggrfield*_tmpD22=({struct Cyc_Absyn_Aggrfield*_tmp1E0=_cycalloc(sizeof(*_tmp1E0));_tmp1E0->name=f->name,_tmp1E0->tq=Cyc_Toc_mt_tq,_tmp1E0->type=t,_tmp1E0->width=f->width,_tmp1E0->attributes=atts,_tmp1E0->requires_clause=0;_tmp1E0;});_tmp1E1->hd=_tmpD22;}),_tmp1E1->tl=fs;_tmp1E1;});}
# 1049
fs=({Cyc_List_imp_rev(fs);});{
struct Cyc_Absyn_Aggrdecl*sd=({Cyc_Toc_make_c_struct_defn(xname,fs);});
Cyc_Toc_result_decls=({struct Cyc_List_List*_tmp1E3=_cycalloc(sizeof(*_tmp1E3));({struct Cyc_Absyn_Decl*_tmpD23=({Cyc_Absyn_new_decl((void*)({struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct*_tmp1E2=_cycalloc(sizeof(*_tmp1E2));_tmp1E2->tag=5U,_tmp1E2->f1=sd;_tmp1E2;}),0U);});_tmp1E3->hd=_tmpD23;}),_tmp1E3->tl=Cyc_Toc_result_decls;_tmp1E3;});{
void*_tmp1E4=x;_npop_handler(0U);return _tmp1E4;}}}
# 1027
;_pop_region();}}}}
# 1055
static void*Cyc_Toc_add_struct_type(struct _tuple1*struct_name,struct Cyc_List_List*type_vars,struct Cyc_List_List*type_args,struct Cyc_List_List*fields){
# 1059
struct _tuple30 env=({struct _tuple30 _tmpC70;_tmpC70.f1=struct_name,_tmpC70.f2=type_vars,_tmpC70.f3=type_args,_tmpC70.f4=fields;_tmpC70;});
return({({void*(*_tmp1EA)(struct _tuple30*arg,void*(*f)(struct _RegionHandle*,struct _tuple31*))=(void*(*)(struct _tuple30*arg,void*(*f)(struct _RegionHandle*,struct _tuple31*)))Cyc_Toc_use_toc_state;_tmp1EA;})(& env,Cyc_Toc_add_struct_type_body);});}
# 1055
struct _tuple1*Cyc_Toc_temp_var(){
# 1068
return({struct _tuple1*_tmp1F0=_cycalloc(sizeof(*_tmp1F0));_tmp1F0->f1=Cyc_Absyn_Loc_n,({struct _fat_ptr*_tmpD26=({struct _fat_ptr*_tmp1EF=_cycalloc(sizeof(*_tmp1EF));({struct _fat_ptr _tmpD25=(struct _fat_ptr)({struct Cyc_Int_pa_PrintArg_struct _tmp1EE=({struct Cyc_Int_pa_PrintArg_struct _tmpC71;_tmpC71.tag=1U,_tmpC71.f1=(unsigned)Cyc_Toc_temp_var_counter ++;_tmpC71;});void*_tmp1EC[1U];_tmp1EC[0]=& _tmp1EE;({struct _fat_ptr _tmpD24=({const char*_tmp1ED="_tmp%X";_tag_fat(_tmp1ED,sizeof(char),7U);});Cyc_aprintf(_tmpD24,_tag_fat(_tmp1EC,sizeof(void*),1U));});});*_tmp1EF=_tmpD25;});_tmp1EF;});_tmp1F0->f2=_tmpD26;});_tmp1F0;});}struct _tuple33{struct _tuple1*f1;struct Cyc_Absyn_Exp*f2;};
# 1070
struct _tuple33 Cyc_Toc_temp_var_and_exp(){
struct _tuple1*v=({Cyc_Toc_temp_var();});
return({struct _tuple33 _tmpC72;_tmpC72.f1=v,({struct Cyc_Absyn_Exp*_tmpD27=({Cyc_Absyn_var_exp(v,0U);});_tmpC72.f2=_tmpD27;});_tmpC72;});}struct _tuple34{struct Cyc_Toc_TocState*f1;int f2;};
# 1077
static struct _fat_ptr*Cyc_Toc_fresh_label_body(struct _RegionHandle*d,struct _tuple34*env){
struct _tuple34 _stmttmp14=*env;struct Cyc_Xarray_Xarray*_tmp1F3;_LL1: _tmp1F3=(_stmttmp14.f1)->temp_labels;_LL2: {struct Cyc_Xarray_Xarray*temp_labels=_tmp1F3;
int i=Cyc_Toc_fresh_label_counter ++;
if(({int _tmpD28=i;_tmpD28 < ({Cyc_Xarray_length(temp_labels);});}))
return({({struct _fat_ptr*(*_tmpD2A)(struct Cyc_Xarray_Xarray*,int)=({struct _fat_ptr*(*_tmp1F4)(struct Cyc_Xarray_Xarray*,int)=(struct _fat_ptr*(*)(struct Cyc_Xarray_Xarray*,int))Cyc_Xarray_get;_tmp1F4;});struct Cyc_Xarray_Xarray*_tmpD29=temp_labels;_tmpD2A(_tmpD29,i);});});{
# 1080
struct _fat_ptr*res=({struct _fat_ptr*_tmp1FC=_cycalloc(sizeof(*_tmp1FC));({
# 1082
struct _fat_ptr _tmpD2C=(struct _fat_ptr)({struct Cyc_Int_pa_PrintArg_struct _tmp1FB=({struct Cyc_Int_pa_PrintArg_struct _tmpC73;_tmpC73.tag=1U,_tmpC73.f1=(unsigned)i;_tmpC73;});void*_tmp1F9[1U];_tmp1F9[0]=& _tmp1FB;({struct _fat_ptr _tmpD2B=({const char*_tmp1FA="_LL%X";_tag_fat(_tmp1FA,sizeof(char),6U);});Cyc_aprintf(_tmpD2B,_tag_fat(_tmp1F9,sizeof(void*),1U));});});*_tmp1FC=_tmpD2C;});_tmp1FC;});
if(({int _tmpD2F=({({int(*_tmpD2E)(struct Cyc_Xarray_Xarray*,struct _fat_ptr*)=({int(*_tmp1F5)(struct Cyc_Xarray_Xarray*,struct _fat_ptr*)=(int(*)(struct Cyc_Xarray_Xarray*,struct _fat_ptr*))Cyc_Xarray_add_ind;_tmp1F5;});struct Cyc_Xarray_Xarray*_tmpD2D=temp_labels;_tmpD2E(_tmpD2D,res);});});_tmpD2F != i;}))
({void*_tmp1F6=0U;({int(*_tmpD31)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp1F8)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmp1F8;});struct _fat_ptr _tmpD30=({const char*_tmp1F7="fresh_label: add_ind returned bad index...";_tag_fat(_tmp1F7,sizeof(char),43U);});_tmpD31(_tmpD30,_tag_fat(_tmp1F6,sizeof(void*),0U));});});
# 1083
return res;}}}
# 1087
static struct _fat_ptr*Cyc_Toc_fresh_label(){
return({({struct _fat_ptr*(*_tmp1FE)(int arg,struct _fat_ptr*(*f)(struct _RegionHandle*,struct _tuple34*))=(struct _fat_ptr*(*)(int arg,struct _fat_ptr*(*f)(struct _RegionHandle*,struct _tuple34*)))Cyc_Toc_use_toc_state;_tmp1FE;})(0,Cyc_Toc_fresh_label_body);});}
# 1093
static struct Cyc_Absyn_Exp*Cyc_Toc_datatype_tag(struct Cyc_Absyn_Datatypedecl*td,struct _tuple1*name){
int ans=0;
struct Cyc_List_List*fs=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(td->fields))->v;
while(({Cyc_Absyn_qvar_cmp(name,((struct Cyc_Absyn_Datatypefield*)((struct Cyc_List_List*)_check_null(fs))->hd)->name);})!= 0){
++ ans;
fs=fs->tl;}
# 1100
return({Cyc_Absyn_uint_exp((unsigned)ans,0U);});}
# 1093
static void Cyc_Toc_enumdecl_to_c(struct Cyc_Absyn_Enumdecl*ed);
# 1107
static void Cyc_Toc_aggrdecl_to_c(struct Cyc_Absyn_Aggrdecl*ad);
static void Cyc_Toc_datatypedecl_to_c(struct Cyc_Absyn_Datatypedecl*tud);
static struct _tuple9*Cyc_Toc_arg_to_c(struct _tuple9*a){
return({struct _tuple9*_tmp201=_cycalloc(sizeof(*_tmp201));_tmp201->f1=(*a).f1,_tmp201->f2=(*a).f2,({void*_tmpD32=({Cyc_Toc_typ_to_c((*a).f3);});_tmp201->f3=_tmpD32;});_tmp201;});}
# 1128 "toc.cyc"
static void*Cyc_Toc_typ_to_c_array(void*t){
void*_stmttmp15=({Cyc_Tcutil_compress(t);});void*_tmp203=_stmttmp15;struct Cyc_Absyn_ArrayInfo _tmp204;if(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp203)->tag == 4U){_LL1: _tmp204=((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp203)->f1;_LL2: {struct Cyc_Absyn_ArrayInfo ai=_tmp204;
return({void*(*_tmp205)(void*,struct Cyc_Absyn_Tqual)=Cyc_Absyn_cstar_type;void*_tmp206=({Cyc_Toc_typ_to_c_array(ai.elt_type);});struct Cyc_Absyn_Tqual _tmp207=ai.tq;_tmp205(_tmp206,_tmp207);});}}else{_LL3: _LL4:
 return({Cyc_Toc_typ_to_c(t);});}_LL0:;}
# 1135
static struct Cyc_Absyn_Aggrfield*Cyc_Toc_aggrfield_to_c(struct Cyc_Absyn_Aggrfield*f,void*new_type){
# 1137
struct Cyc_Absyn_Aggrfield*ans=({struct Cyc_Absyn_Aggrfield*_tmp209=_cycalloc(sizeof(*_tmp209));*_tmp209=*f;_tmp209;});
ans->type=new_type;
ans->requires_clause=0;
ans->tq=Cyc_Toc_mt_tq;
return ans;}
# 1144
static void Cyc_Toc_enumfields_to_c(struct Cyc_List_List*fs){
# 1146
return;}
# 1150
static int Cyc_Toc_is_boxed_tvar(void*t){
void*_stmttmp16=({Cyc_Tcutil_compress(t);});void*_tmp20C=_stmttmp16;struct Cyc_Absyn_Tvar*_tmp20D;if(((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp20C)->tag == 2U){_LL1: _tmp20D=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp20C)->f1;_LL2: {struct Cyc_Absyn_Tvar*tv=_tmp20D;
return({int(*_tmp20E)(struct Cyc_Absyn_Kind*,struct Cyc_Absyn_Kind*)=Cyc_Tcutil_kind_leq;struct Cyc_Absyn_Kind*_tmp20F=({Cyc_Tcutil_type_kind(t);});struct Cyc_Absyn_Kind*_tmp210=& Cyc_Tcutil_tbk;_tmp20E(_tmp20F,_tmp210);});}}else{_LL3: _LL4:
 return 0;}_LL0:;}
# 1157
static int Cyc_Toc_is_abstract_type(void*t){
struct Cyc_Absyn_Kind*_stmttmp17=({Cyc_Tcutil_type_kind(t);});struct Cyc_Absyn_Kind*_tmp212=_stmttmp17;if(((struct Cyc_Absyn_Kind*)_tmp212)->kind == Cyc_Absyn_AnyKind){_LL1: _LL2:
 return 1;}else{_LL3: _LL4:
 return 0;}_LL0:;}
# 1164
static int Cyc_Toc_is_void_star(void*t){
void*_stmttmp18=({Cyc_Tcutil_compress(t);});void*_tmp214=_stmttmp18;void*_tmp215;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp214)->tag == 3U){_LL1: _tmp215=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp214)->f1).elt_type;_LL2: {void*t2=_tmp215;
return({Cyc_Tcutil_is_void_type(t2);});}}else{_LL3: _LL4:
 return 0;}_LL0:;}
# 1171
static int Cyc_Toc_is_void_star_or_boxed_tvar(void*t){
return({Cyc_Toc_is_void_star(t);})||({Cyc_Toc_is_boxed_tvar(t);});}
# 1174
static int Cyc_Toc_is_pointer_or_boxed_tvar(void*t){
return({Cyc_Tcutil_is_pointer_type(t);})||({Cyc_Toc_is_boxed_tvar(t);});}
# 1174
void*Cyc_Toc_typ_to_c(void*t){
# 1180
void*_tmp219=t;void**_tmp21B;struct Cyc_Absyn_Datatypedecl*_tmp21A;struct Cyc_Absyn_Enumdecl*_tmp21C;struct Cyc_Absyn_Aggrdecl*_tmp21D;struct Cyc_Absyn_Exp*_tmp21E;struct Cyc_Absyn_Exp*_tmp21F;void*_tmp223;struct Cyc_Absyn_Typedefdecl*_tmp222;struct Cyc_List_List*_tmp221;struct _tuple1*_tmp220;struct Cyc_List_List*_tmp225;enum Cyc_Absyn_AggrKind _tmp224;struct Cyc_List_List*_tmp226;struct Cyc_List_List*_tmp22C;struct Cyc_Absyn_VarargInfo*_tmp22B;int _tmp22A;struct Cyc_List_List*_tmp229;void*_tmp228;struct Cyc_Absyn_Tqual _tmp227;unsigned _tmp230;struct Cyc_Absyn_Exp*_tmp22F;struct Cyc_Absyn_Tqual _tmp22E;void*_tmp22D;void*_tmp233;struct Cyc_Absyn_Tqual _tmp232;void*_tmp231;struct Cyc_Absyn_Tvar*_tmp234;void**_tmp235;struct Cyc_List_List*_tmp236;struct Cyc_List_List*_tmp237;struct _tuple1*_tmp238;struct Cyc_List_List*_tmp23B;union Cyc_Absyn_AggrInfo _tmp23A;void*_tmp239;struct Cyc_Absyn_Datatypefield*_tmp23D;struct Cyc_Absyn_Datatypedecl*_tmp23C;switch(*((int*)_tmp219)){case 0U: switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp219)->f1)){case 0U: _LL1: _LL2:
 return t;case 20U: _LL7: _LL8:
# 1192
 return Cyc_Absyn_void_type;case 21U: if(((((struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp219)->f1)->f1).KnownDatatypefield).tag == 2){_LL9: _tmp23C=(((((struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp219)->f1)->f1).KnownDatatypefield).val).f1;_tmp23D=(((((struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp219)->f1)->f1).KnownDatatypefield).val).f2;_LLA: {struct Cyc_Absyn_Datatypedecl*tud=_tmp23C;struct Cyc_Absyn_Datatypefield*tuf=_tmp23D;
# 1194
return({void*(*_tmp23E)(struct _tuple1*name)=Cyc_Absyn_strctq;struct _tuple1*_tmp23F=({Cyc_Toc_collapse_qvars(tuf->name,tud->name);});_tmp23E(_tmp23F);});}}else{_LLB: _LLC:
# 1196
({void*_tmp240=0U;({int(*_tmpD34)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp242)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmp242;});struct _fat_ptr _tmpD33=({const char*_tmp241="unresolved DatatypeFieldType";_tag_fat(_tmp241,sizeof(char),29U);});_tmpD34(_tmpD33,_tag_fat(_tmp240,sizeof(void*),0U));});});}case 1U: _LLF: _LL10:
# 1205
 goto _LL12;case 2U: _LL11: _LL12:
 return t;case 22U: _LL1B: _tmp239=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp219)->f1;_tmp23A=((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp219)->f1)->f1;_tmp23B=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp219)->f2;_LL1C: {void*c=_tmp239;union Cyc_Absyn_AggrInfo info=_tmp23A;struct Cyc_List_List*ts=_tmp23B;
# 1253
{union Cyc_Absyn_AggrInfo _tmp25C=info;if((_tmp25C.UnknownAggr).tag == 1){_LL45: _LL46:
# 1255
 if(ts == 0)return t;else{
return(void*)({struct Cyc_Absyn_AppType_Absyn_Type_struct*_tmp25D=_cycalloc(sizeof(*_tmp25D));_tmp25D->tag=0U,_tmp25D->f1=c,_tmp25D->f2=0;_tmp25D;});}}else{_LL47: _LL48:
 goto _LL44;}_LL44:;}{
# 1259
struct Cyc_Absyn_Aggrdecl*ad=({Cyc_Absyn_get_known_aggrdecl(info);});
if(ad->expected_mem_kind){
# 1262
if(ad->impl == 0)
({struct Cyc_String_pa_PrintArg_struct _tmp260=({struct Cyc_String_pa_PrintArg_struct _tmpC75;_tmpC75.tag=0U,({
struct _fat_ptr _tmpD35=(struct _fat_ptr)((int)ad->kind == (int)Cyc_Absyn_UnionA?({const char*_tmp262="union";_tag_fat(_tmp262,sizeof(char),6U);}):({const char*_tmp263="struct";_tag_fat(_tmp263,sizeof(char),7U);}));_tmpC75.f1=_tmpD35;});_tmpC75;});struct Cyc_String_pa_PrintArg_struct _tmp261=({struct Cyc_String_pa_PrintArg_struct _tmpC74;_tmpC74.tag=0U,({
struct _fat_ptr _tmpD36=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(ad->name);}));_tmpC74.f1=_tmpD36;});_tmpC74;});void*_tmp25E[2U];_tmp25E[0]=& _tmp260,_tmp25E[1]=& _tmp261;({struct _fat_ptr _tmpD37=({const char*_tmp25F="%s %s was never defined.";_tag_fat(_tmp25F,sizeof(char),25U);});Cyc_Tcutil_warn(0U,_tmpD37,_tag_fat(_tmp25E,sizeof(void*),2U));});});}
# 1260
if((int)ad->kind == (int)Cyc_Absyn_UnionA)
# 1269
return({Cyc_Toc_aggrdecl_type(ad->name,Cyc_Absyn_unionq_type);});{
# 1260
struct Cyc_List_List*fs=
# 1270
ad->impl == 0?0:((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->fields;
if(fs == 0)return({Cyc_Toc_aggrdecl_type(ad->name,Cyc_Absyn_strctq);});for(0;((struct Cyc_List_List*)_check_null(fs))->tl != 0;fs=fs->tl){
;}{
void*last_type=((struct Cyc_Absyn_Aggrfield*)fs->hd)->type;
if(({int(*_tmp264)(struct Cyc_Absyn_Kind*,struct Cyc_Absyn_Kind*)=Cyc_Tcutil_kind_leq;struct Cyc_Absyn_Kind*_tmp265=& Cyc_Tcutil_ak;struct Cyc_Absyn_Kind*_tmp266=({Cyc_Tcutil_type_kind(last_type);});_tmp264(_tmp265,_tmp266);})){
if(ad->expected_mem_kind)
({struct Cyc_String_pa_PrintArg_struct _tmp269=({struct Cyc_String_pa_PrintArg_struct _tmpC76;_tmpC76.tag=0U,({struct _fat_ptr _tmpD38=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(ad->name);}));_tmpC76.f1=_tmpD38;});_tmpC76;});void*_tmp267[1U];_tmp267[0]=& _tmp269;({struct _fat_ptr _tmpD39=({const char*_tmp268="struct %s ended up being abstract.";_tag_fat(_tmp268,sizeof(char),35U);});Cyc_Tcutil_warn(0U,_tmpD39,_tag_fat(_tmp267,sizeof(void*),1U));});});{
# 1275
struct _RegionHandle _tmp26A=_new_region("r");struct _RegionHandle*r=& _tmp26A;_push_region(r);
# 1282
{struct Cyc_List_List*inst=({Cyc_List_rzip(r,r,ad->tvs,ts);});
void*t=({Cyc_Tcutil_rsubstitute(r,inst,last_type);});
if(({Cyc_Toc_is_abstract_type(t);})){void*_tmp26B=({Cyc_Toc_aggrdecl_type(ad->name,Cyc_Absyn_strctq);});_npop_handler(0U);return _tmp26B;}{void*_tmp26C=({Cyc_Toc_add_struct_type(ad->name,ad->tvs,ts,((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->fields);});_npop_handler(0U);return _tmp26C;}}
# 1282
;_pop_region();}}
# 1274
return({Cyc_Toc_aggrdecl_type(ad->name,Cyc_Absyn_strctq);});}}}}case 17U: _LL1D: _tmp238=((struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp219)->f1)->f1;_LL1E: {struct _tuple1*tdn=_tmp238;
# 1288
return t;}case 18U: _LL1F: _tmp237=((struct Cyc_Absyn_AnonEnumCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp219)->f1)->f1;_LL20: {struct Cyc_List_List*fs=_tmp237;
({Cyc_Toc_enumfields_to_c(fs);});return t;}case 5U: _LL23: _LL24:
# 1299
 return Cyc_Absyn_uint_type;case 3U: _LL25: _tmp236=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp219)->f2;_LL26: {struct Cyc_List_List*l=_tmp236;
# 1301
if(l != 0 &&({Cyc_Absyn_is_xrgn((void*)l->hd);}))return({Cyc_Toc_xrgn_type();});else{
return({Cyc_Toc_rgn_type();});}}case 19U: _LL27: _LL28:
 return t;default: _LL29: _LL2A:
# 1306
 return({Cyc_Toc_void_star_type();});}case 1U: _LL3: _tmp235=(void**)&((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp219)->f2;_LL4: {void**t2=_tmp235;
# 1183
if(*t2 == 0){
*t2=Cyc_Absyn_sint_type;
return Cyc_Absyn_sint_type;}
# 1183
return({Cyc_Toc_typ_to_c((void*)_check_null(*t2));});}case 2U: _LL5: _tmp234=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp219)->f1;_LL6: {struct Cyc_Absyn_Tvar*tv=_tmp234;
# 1189
if((int)({Cyc_Tcutil_tvar_kind(tv,& Cyc_Tcutil_bk);})->kind == (int)Cyc_Absyn_AnyKind)
return Cyc_Absyn_void_type;else{
return({Cyc_Toc_void_star_type();});}}case 3U: _LLD: _tmp231=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp219)->f1).elt_type;_tmp232=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp219)->f1).elt_tq;_tmp233=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp219)->f1).ptr_atts).bounds;_LLE: {void*t2=_tmp231;struct Cyc_Absyn_Tqual tq=_tmp232;void*bnds=_tmp233;
# 1200
t2=({Cyc_Toc_typ_to_c(t2);});
if(({Cyc_Tcutil_get_bounds_exp(Cyc_Absyn_fat_bound_type,bnds);})== 0)
return({Cyc_Toc_fat_ptr_type();});else{
# 1204
return({Cyc_Absyn_star_type(t2,Cyc_Absyn_heap_rgn_type,tq,Cyc_Absyn_false_type);});}}case 4U: _LL13: _tmp22D=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp219)->f1).elt_type;_tmp22E=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp219)->f1).tq;_tmp22F=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp219)->f1).num_elts;_tmp230=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp219)->f1).zt_loc;_LL14: {void*t2=_tmp22D;struct Cyc_Absyn_Tqual tq=_tmp22E;struct Cyc_Absyn_Exp*e=_tmp22F;unsigned ztl=_tmp230;
# 1208
return({void*(*_tmp243)(void*elt_type,struct Cyc_Absyn_Tqual tq,struct Cyc_Absyn_Exp*num_elts,void*zero_term,unsigned ztloc)=Cyc_Absyn_array_type;void*_tmp244=({Cyc_Toc_typ_to_c(t2);});struct Cyc_Absyn_Tqual _tmp245=tq;struct Cyc_Absyn_Exp*_tmp246=e;void*_tmp247=Cyc_Absyn_false_type;unsigned _tmp248=ztl;_tmp243(_tmp244,_tmp245,_tmp246,_tmp247,_tmp248);});}case 5U: _LL15: _tmp227=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp219)->f1).ret_tqual;_tmp228=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp219)->f1).ret_type;_tmp229=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp219)->f1).args;_tmp22A=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp219)->f1).c_varargs;_tmp22B=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp219)->f1).cyc_varargs;_tmp22C=(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp219)->f1).attributes;_LL16: {struct Cyc_Absyn_Tqual tq2=_tmp227;void*t2=_tmp228;struct Cyc_List_List*args=_tmp229;int c_vararg=_tmp22A;struct Cyc_Absyn_VarargInfo*cyc_vararg=_tmp22B;struct Cyc_List_List*atts=_tmp22C;
# 1215
struct Cyc_List_List*new_atts=0;
for(0;atts != 0;atts=atts->tl){
void*_stmttmp19=(void*)atts->hd;void*_tmp249=_stmttmp19;switch(*((int*)_tmp249)){case 4U: _LL36: _LL37:
 goto _LL39;case 5U: _LL38: _LL39:
 goto _LL3B;case 19U: _LL3A: _LL3B:
 continue;case 22U: _LL3C: _LL3D:
 continue;case 21U: _LL3E: _LL3F:
 continue;case 20U: _LL40: _LL41:
 continue;default: _LL42: _LL43:
 new_atts=({struct Cyc_List_List*_tmp24A=_cycalloc(sizeof(*_tmp24A));_tmp24A->hd=(void*)atts->hd,_tmp24A->tl=new_atts;_tmp24A;});goto _LL35;}_LL35:;}{
# 1226
struct Cyc_List_List*new_args=({({struct Cyc_List_List*(*_tmpD3A)(struct _tuple9*(*f)(struct _tuple9*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp250)(struct _tuple9*(*f)(struct _tuple9*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct _tuple9*(*f)(struct _tuple9*),struct Cyc_List_List*x))Cyc_List_map;_tmp250;});_tmpD3A(Cyc_Toc_arg_to_c,args);});});
if(cyc_vararg != 0){
# 1229
void*t=({void*(*_tmp24D)(void*t)=Cyc_Toc_typ_to_c;void*_tmp24E=({Cyc_Absyn_fatptr_type(cyc_vararg->type,Cyc_Absyn_heap_rgn_type,Cyc_Toc_mt_tq,Cyc_Absyn_false_type);});_tmp24D(_tmp24E);});
struct _tuple9*vararg=({struct _tuple9*_tmp24C=_cycalloc(sizeof(*_tmp24C));_tmp24C->f1=cyc_vararg->name,_tmp24C->f2=cyc_vararg->tq,_tmp24C->f3=t;_tmp24C;});
new_args=({({struct Cyc_List_List*_tmpD3B=new_args;Cyc_List_imp_append(_tmpD3B,({struct Cyc_List_List*_tmp24B=_cycalloc(sizeof(*_tmp24B));_tmp24B->hd=vararg,_tmp24B->tl=0;_tmp24B;}));});});}
# 1227
return(void*)({struct Cyc_Absyn_FnType_Absyn_Type_struct*_tmp24F=_cycalloc(sizeof(*_tmp24F));
# 1233
_tmp24F->tag=5U,(_tmp24F->f1).tvars=0,(_tmp24F->f1).effect=0,(_tmp24F->f1).ret_tqual=tq2,({void*_tmpD3C=({Cyc_Toc_typ_to_c(t2);});(_tmp24F->f1).ret_type=_tmpD3C;}),(_tmp24F->f1).args=new_args,(_tmp24F->f1).c_varargs=c_vararg,(_tmp24F->f1).cyc_varargs=0,(_tmp24F->f1).rgn_po=0,(_tmp24F->f1).attributes=new_atts,(_tmp24F->f1).requires_clause=0,(_tmp24F->f1).requires_relns=0,(_tmp24F->f1).ensures_clause=0,(_tmp24F->f1).ensures_relns=0,(_tmp24F->f1).ieffect=0,(_tmp24F->f1).oeffect=0,(_tmp24F->f1).throws=0,(_tmp24F->f1).reentrant=Cyc_Absyn_noreentrant;_tmp24F;});}}case 6U: _LL17: _tmp226=((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp219)->f1;_LL18: {struct Cyc_List_List*tqs=_tmp226;
# 1238
struct Cyc_List_List*tqs2=0;
for(0;tqs != 0;tqs=tqs->tl){
tqs2=({struct Cyc_List_List*_tmp252=_cycalloc(sizeof(*_tmp252));({struct _tuple14*_tmpD3E=({struct _tuple14*_tmp251=_cycalloc(sizeof(*_tmp251));_tmp251->f1=(*((struct _tuple14*)tqs->hd)).f1,({void*_tmpD3D=({Cyc_Toc_typ_to_c((*((struct _tuple14*)tqs->hd)).f2);});_tmp251->f2=_tmpD3D;});_tmp251;});_tmp252->hd=_tmpD3E;}),_tmp252->tl=tqs2;_tmp252;});}
return({void*(*_tmp253)(struct Cyc_List_List*tqs0)=Cyc_Toc_add_tuple_type;struct Cyc_List_List*_tmp254=({Cyc_List_imp_rev(tqs2);});_tmp253(_tmp254);});}case 7U: _LL19: _tmp224=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp219)->f1;_tmp225=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp219)->f2;_LL1A: {enum Cyc_Absyn_AggrKind k=_tmp224;struct Cyc_List_List*fs=_tmp225;
# 1247
struct Cyc_List_List*fs2=0;
for(0;fs != 0;fs=fs->tl){
fs2=({struct Cyc_List_List*_tmp258=_cycalloc(sizeof(*_tmp258));({struct Cyc_Absyn_Aggrfield*_tmpD3F=({struct Cyc_Absyn_Aggrfield*(*_tmp255)(struct Cyc_Absyn_Aggrfield*f,void*new_type)=Cyc_Toc_aggrfield_to_c;struct Cyc_Absyn_Aggrfield*_tmp256=(struct Cyc_Absyn_Aggrfield*)fs->hd;void*_tmp257=({Cyc_Toc_typ_to_c(((struct Cyc_Absyn_Aggrfield*)fs->hd)->type);});_tmp255(_tmp256,_tmp257);});_tmp258->hd=_tmpD3F;}),_tmp258->tl=fs2;_tmp258;});}
return({void*(*_tmp259)(enum Cyc_Absyn_AggrKind ak,struct Cyc_List_List*fs)=Cyc_Toc_add_anon_aggr_type;enum Cyc_Absyn_AggrKind _tmp25A=k;struct Cyc_List_List*_tmp25B=({Cyc_List_imp_rev(fs2);});_tmp259(_tmp25A,_tmp25B);});}case 8U: _LL21: _tmp220=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp219)->f1;_tmp221=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp219)->f2;_tmp222=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp219)->f3;_tmp223=(void*)((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp219)->f4;_LL22: {struct _tuple1*tdn=_tmp220;struct Cyc_List_List*ts=_tmp221;struct Cyc_Absyn_Typedefdecl*td=_tmp222;void*topt=_tmp223;
# 1293
if(topt == 0){
if(ts != 0)
return(void*)({struct Cyc_Absyn_TypedefType_Absyn_Type_struct*_tmp26D=_cycalloc(sizeof(*_tmp26D));_tmp26D->tag=8U,_tmp26D->f1=tdn,_tmp26D->f2=0,_tmp26D->f3=td,_tmp26D->f4=0;_tmp26D;});else{
return t;}}else{
# 1298
return(void*)({struct Cyc_Absyn_TypedefType_Absyn_Type_struct*_tmp26E=_cycalloc(sizeof(*_tmp26E));_tmp26E->tag=8U,_tmp26E->f1=tdn,_tmp26E->f2=0,_tmp26E->f3=td,({void*_tmpD40=({Cyc_Toc_typ_to_c(topt);});_tmp26E->f4=_tmpD40;});_tmp26E;});}}case 9U: _LL2B: _tmp21F=((struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_tmp219)->f1;_LL2C: {struct Cyc_Absyn_Exp*e=_tmp21F;
# 1311
return t;}case 11U: _LL2D: _tmp21E=((struct Cyc_Absyn_TypeofType_Absyn_Type_struct*)_tmp219)->f1;_LL2E: {struct Cyc_Absyn_Exp*e=_tmp21E;
# 1316
if(e->topt != 0)
return({Cyc_Toc_typ_to_c((void*)_check_null(e->topt));});
# 1316
return t;}default: switch(*((int*)((struct Cyc_Absyn_TypeDecl*)((struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_tmp219)->f1)->r)){case 0U: _LL2F: _tmp21D=((struct Cyc_Absyn_Aggr_td_Absyn_Raw_typedecl_struct*)(((struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_tmp219)->f1)->r)->f1;_LL30: {struct Cyc_Absyn_Aggrdecl*ad=_tmp21D;
# 1320
({Cyc_Toc_aggrdecl_to_c(ad);});
if((int)ad->kind == (int)Cyc_Absyn_UnionA)
return({Cyc_Toc_aggrdecl_type(ad->name,Cyc_Absyn_unionq_type);});
# 1321
return({Cyc_Toc_aggrdecl_type(ad->name,Cyc_Absyn_strctq);});}case 1U: _LL31: _tmp21C=((struct Cyc_Absyn_Enum_td_Absyn_Raw_typedecl_struct*)(((struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_tmp219)->f1)->r)->f1;_LL32: {struct Cyc_Absyn_Enumdecl*ed=_tmp21C;
# 1325
({Cyc_Toc_enumdecl_to_c(ed);});
return t;}default: _LL33: _tmp21A=((struct Cyc_Absyn_Datatype_td_Absyn_Raw_typedecl_struct*)(((struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_tmp219)->f1)->r)->f1;_tmp21B=((struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_tmp219)->f2;_LL34: {struct Cyc_Absyn_Datatypedecl*dd=_tmp21A;void**t=_tmp21B;
# 1328
({Cyc_Toc_datatypedecl_to_c(dd);});
return({Cyc_Toc_typ_to_c(*((void**)_check_null(t)));});}}}_LL0:;}
# 1333
static struct Cyc_Absyn_Exp*Cyc_Toc_array_to_ptr_cast(void*t,struct Cyc_Absyn_Exp*e,unsigned l){
void*_tmp270=t;struct Cyc_Absyn_Tqual _tmp272;void*_tmp271;if(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp270)->tag == 4U){_LL1: _tmp271=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp270)->f1).elt_type;_tmp272=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp270)->f1).tq;_LL2: {void*t2=_tmp271;struct Cyc_Absyn_Tqual tq=_tmp272;
# 1336
return({struct Cyc_Absyn_Exp*(*_tmp273)(void*t,struct Cyc_Absyn_Exp*e)=Cyc_Toc_cast_it;void*_tmp274=({Cyc_Absyn_star_type(t2,Cyc_Absyn_heap_rgn_type,tq,Cyc_Absyn_false_type);});struct Cyc_Absyn_Exp*_tmp275=e;_tmp273(_tmp274,_tmp275);});}}else{_LL3: _LL4:
 return({Cyc_Toc_cast_it(t,e);});}_LL0:;}
# 1343
static int Cyc_Toc_atomic_type(void*t){
void*_stmttmp1A=({Cyc_Tcutil_compress(t);});void*_tmp277=_stmttmp1A;struct Cyc_List_List*_tmp278;struct Cyc_List_List*_tmp279;void*_tmp27A;struct Cyc_List_List*_tmp27C;void*_tmp27B;switch(*((int*)_tmp277)){case 2U: _LL1: _LL2:
 return 0;case 0U: _LL3: _tmp27B=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp277)->f1;_tmp27C=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp277)->f2;_LL4: {void*c=_tmp27B;struct Cyc_List_List*ts=_tmp27C;
# 1347
void*_tmp27D=c;struct Cyc_Absyn_Datatypefield*_tmp27F;struct Cyc_Absyn_Datatypedecl*_tmp27E;union Cyc_Absyn_AggrInfo _tmp280;switch(*((int*)_tmp27D)){case 0U: _LL12: _LL13:
 goto _LL15;case 1U: _LL14: _LL15: goto _LL17;case 2U: _LL16: _LL17: goto _LL19;case 5U: _LL18: _LL19:
 goto _LL1B;case 17U: _LL1A: _LL1B: goto _LL1D;case 18U: _LL1C: _LL1D: return 1;case 20U: _LL1E: _LL1F:
 goto _LL21;case 3U: _LL20: _LL21:
 goto _LL23;case 4U: _LL22: _LL23:
 goto _LL25;case 19U: _LL24: _LL25:
 return 0;case 22U: _LL26: _tmp280=((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)_tmp27D)->f1;_LL27: {union Cyc_Absyn_AggrInfo info=_tmp280;
# 1355
{union Cyc_Absyn_AggrInfo _tmp281=info;if((_tmp281.UnknownAggr).tag == 1){_LL2D: _LL2E:
 return 0;}else{_LL2F: _LL30:
 goto _LL2C;}_LL2C:;}{
# 1359
struct Cyc_Absyn_Aggrdecl*ad=({Cyc_Absyn_get_known_aggrdecl(info);});
if(ad->impl == 0)
return 0;
# 1360
{
# 1362
struct Cyc_List_List*fs=((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->fields;
# 1360
for(0;fs != 0;fs=fs->tl){
# 1363
if(!({Cyc_Toc_atomic_type(((struct Cyc_Absyn_Aggrfield*)fs->hd)->type);}))return 0;}}
# 1360
return 1;}}case 21U: if(((((struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)_tmp27D)->f1).KnownDatatypefield).tag == 2){_LL28: _tmp27E=(((((struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)_tmp27D)->f1).KnownDatatypefield).val).f1;_tmp27F=(((((struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)_tmp27D)->f1).KnownDatatypefield).val).f2;_LL29: {struct Cyc_Absyn_Datatypedecl*tud=_tmp27E;struct Cyc_Absyn_Datatypefield*tuf=_tmp27F;
# 1366
{struct Cyc_List_List*tqs=tuf->typs;for(0;tqs != 0;tqs=tqs->tl){
if(!({Cyc_Toc_atomic_type((*((struct _tuple14*)tqs->hd)).f2);}))return 0;}}
# 1366
return 1;}}else{goto _LL2A;}default: _LL2A: _LL2B:
# 1369
({struct Cyc_String_pa_PrintArg_struct _tmp285=({struct Cyc_String_pa_PrintArg_struct _tmpC77;_tmpC77.tag=0U,({struct _fat_ptr _tmpD41=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t);}));_tmpC77.f1=_tmpD41;});_tmpC77;});void*_tmp282[1U];_tmp282[0]=& _tmp285;({int(*_tmpD43)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp284)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmp284;});struct _fat_ptr _tmpD42=({const char*_tmp283="atomic_typ:  bad type %s";_tag_fat(_tmp283,sizeof(char),25U);});_tmpD43(_tmpD42,_tag_fat(_tmp282,sizeof(void*),1U));});});}_LL11:;}case 5U: _LL5: _LL6:
# 1371
 return 1;case 4U: _LL7: _tmp27A=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp277)->f1).elt_type;_LL8: {void*t=_tmp27A;
return({Cyc_Toc_atomic_type(t);});}case 7U: _LL9: _tmp279=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp277)->f2;_LLA: {struct Cyc_List_List*fs=_tmp279;
# 1379
for(0;fs != 0;fs=fs->tl){
if(!({Cyc_Toc_atomic_type(((struct Cyc_Absyn_Aggrfield*)fs->hd)->type);}))return 0;}
# 1379
return 1;}case 6U: _LLB: _tmp278=((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp277)->f1;_LLC: {struct Cyc_List_List*tqs=_tmp278;
# 1383
for(0;tqs != 0;tqs=tqs->tl){
if(!({Cyc_Toc_atomic_type((*((struct _tuple14*)tqs->hd)).f2);}))return 0;}
# 1383
return 1;}case 3U: _LLD: _LLE:
# 1388
 return 0;default: _LLF: _LL10:
({struct Cyc_String_pa_PrintArg_struct _tmp289=({struct Cyc_String_pa_PrintArg_struct _tmpC78;_tmpC78.tag=0U,({struct _fat_ptr _tmpD44=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t);}));_tmpC78.f1=_tmpD44;});_tmpC78;});void*_tmp286[1U];_tmp286[0]=& _tmp289;({int(*_tmpD46)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp288)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmp288;});struct _fat_ptr _tmpD45=({const char*_tmp287="atomic_typ:  bad type %s";_tag_fat(_tmp287,sizeof(char),25U);});_tmpD46(_tmpD45,_tag_fat(_tmp286,sizeof(void*),1U));});});}_LL0:;}
# 1395
static int Cyc_Toc_is_poly_field(void*t,struct _fat_ptr*f){
# 1397
void*_stmttmp1B=({Cyc_Tcutil_compress(t);});void*_tmp28B=_stmttmp1B;struct Cyc_List_List*_tmp28C;union Cyc_Absyn_AggrInfo _tmp28D;switch(*((int*)_tmp28B)){case 0U: if(((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp28B)->f1)->tag == 22U){_LL1: _tmp28D=((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp28B)->f1)->f1;_LL2: {union Cyc_Absyn_AggrInfo info=_tmp28D;
# 1399
struct Cyc_Absyn_Aggrdecl*ad=({Cyc_Absyn_get_known_aggrdecl(info);});
if(ad->impl == 0)
({void*_tmp28E=0U;({int(*_tmpD48)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp290)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmp290;});struct _fat_ptr _tmpD47=({const char*_tmp28F="is_poly_field: type missing fields";_tag_fat(_tmp28F,sizeof(char),35U);});_tmpD48(_tmpD47,_tag_fat(_tmp28E,sizeof(void*),0U));});});
# 1400
_tmp28C=((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->fields;goto _LL4;}}else{goto _LL5;}case 7U: _LL3: _tmp28C=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp28B)->f2;_LL4: {struct Cyc_List_List*fs=_tmp28C;
# 1404
struct Cyc_Absyn_Aggrfield*field=({Cyc_Absyn_lookup_field(fs,f);});
if(field == 0)
({struct Cyc_String_pa_PrintArg_struct _tmp294=({struct Cyc_String_pa_PrintArg_struct _tmpC79;_tmpC79.tag=0U,_tmpC79.f1=(struct _fat_ptr)((struct _fat_ptr)*f);_tmpC79;});void*_tmp291[1U];_tmp291[0]=& _tmp294;({int(*_tmpD4A)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp293)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmp293;});struct _fat_ptr _tmpD49=({const char*_tmp292="is_poly_field: bad field %s";_tag_fat(_tmp292,sizeof(char),28U);});_tmpD4A(_tmpD49,_tag_fat(_tmp291,sizeof(void*),1U));});});
# 1405
return({Cyc_Toc_is_void_star_or_boxed_tvar(field->type);});}default: _LL5: _LL6:
# 1408
({struct Cyc_String_pa_PrintArg_struct _tmp298=({struct Cyc_String_pa_PrintArg_struct _tmpC7A;_tmpC7A.tag=0U,({struct _fat_ptr _tmpD4B=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t);}));_tmpC7A.f1=_tmpD4B;});_tmpC7A;});void*_tmp295[1U];_tmp295[0]=& _tmp298;({int(*_tmpD4D)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp297)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmp297;});struct _fat_ptr _tmpD4C=({const char*_tmp296="is_poly_field: bad type %s";_tag_fat(_tmp296,sizeof(char),27U);});_tmpD4D(_tmpD4C,_tag_fat(_tmp295,sizeof(void*),1U));});});}_LL0:;}
# 1417
static int Cyc_Toc_is_poly_project(struct Cyc_Absyn_Exp*e){
void*_stmttmp1C=e->r;void*_tmp29A=_stmttmp1C;struct _fat_ptr*_tmp29C;struct Cyc_Absyn_Exp*_tmp29B;struct _fat_ptr*_tmp29E;struct Cyc_Absyn_Exp*_tmp29D;switch(*((int*)_tmp29A)){case 21U: _LL1: _tmp29D=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp29A)->f1;_tmp29E=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp29A)->f2;_LL2: {struct Cyc_Absyn_Exp*e1=_tmp29D;struct _fat_ptr*f=_tmp29E;
# 1420
return({Cyc_Toc_is_poly_field((void*)_check_null(e1->topt),f);})&& !({Cyc_Toc_is_void_star_or_boxed_tvar((void*)_check_null(e->topt));});}case 22U: _LL3: _tmp29B=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp29A)->f1;_tmp29C=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp29A)->f2;_LL4: {struct Cyc_Absyn_Exp*e1=_tmp29B;struct _fat_ptr*f=_tmp29C;
# 1423
void*_stmttmp1D=({Cyc_Tcutil_compress((void*)_check_null(e1->topt));});void*_tmp29F=_stmttmp1D;void*_tmp2A0;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp29F)->tag == 3U){_LL8: _tmp2A0=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp29F)->f1).elt_type;_LL9: {void*t=_tmp2A0;
# 1425
return({Cyc_Toc_is_poly_field(t,f);})&& !({Cyc_Toc_is_void_star_or_boxed_tvar((void*)_check_null(e->topt));});}}else{_LLA: _LLB:
({struct Cyc_String_pa_PrintArg_struct _tmp2A4=({struct Cyc_String_pa_PrintArg_struct _tmpC7B;_tmpC7B.tag=0U,({struct _fat_ptr _tmpD4E=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string((void*)_check_null(e1->topt));}));_tmpC7B.f1=_tmpD4E;});_tmpC7B;});void*_tmp2A1[1U];_tmp2A1[0]=& _tmp2A4;({int(*_tmpD50)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp2A3)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmp2A3;});struct _fat_ptr _tmpD4F=({const char*_tmp2A2="is_poly_project: bad type %s";_tag_fat(_tmp2A2,sizeof(char),29U);});_tmpD50(_tmpD4F,_tag_fat(_tmp2A1,sizeof(void*),1U));});});}_LL7:;}default: _LL5: _LL6:
# 1428
 return 0;}_LL0:;}
# 1439 "toc.cyc"
static struct Cyc_Absyn_Exp*Cyc_Toc_malloc_exp(void*t,struct Cyc_Absyn_Exp*s){
struct Cyc_Absyn_Exp*fn_e=({Cyc_Toc_atomic_type(t);})?Cyc_Toc__cycalloc_atomic_e: Cyc_Toc__cycalloc_e;
return({struct Cyc_Absyn_Exp*_tmp2A6[1U];_tmp2A6[0]=s;({struct Cyc_Absyn_Exp*_tmpD51=fn_e;Cyc_Toc_fncall_exp_dl(_tmpD51,_tag_fat(_tmp2A6,sizeof(struct Cyc_Absyn_Exp*),1U));});});}
# 1444
static struct Cyc_Absyn_Exp*Cyc_Toc_calloc_exp(void*elt_type,struct Cyc_Absyn_Exp*s,struct Cyc_Absyn_Exp*n){
struct Cyc_Absyn_Exp*fn_e=({Cyc_Toc_atomic_type(elt_type);})?Cyc_Toc__cyccalloc_atomic_e: Cyc_Toc__cyccalloc_e;
return({struct Cyc_Absyn_Exp*_tmp2A8[2U];_tmp2A8[0]=s,_tmp2A8[1]=n;({struct Cyc_Absyn_Exp*_tmpD52=fn_e;Cyc_Toc_fncall_exp_dl(_tmpD52,_tag_fat(_tmp2A8,sizeof(struct Cyc_Absyn_Exp*),2U));});});}
# 1449
static struct Cyc_Absyn_Exp*Cyc_Toc_xrmalloc_exp(struct Cyc_Absyn_Exp*rgn,struct Cyc_Absyn_Exp*s,int _calloc){
# 1451
void*rtyp=rgn->topt;
if(rtyp != 0){
# 1454
void*_tmp2AA=rtyp;void*_tmp2AB;if(_tmp2AA != 0){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2AA)->tag == 0U){if(((struct Cyc_Absyn_RgnHandleCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2AA)->f1)->tag == 3U){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2AA)->f2 != 0){_LL1: _tmp2AB=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2AA)->f2)->hd;_LL2: {void*r=_tmp2AB;
# 1458
if(!({Cyc_Absyn_is_xrgn(r);}))goto _LL0;if(!_calloc)
# 1460
return({struct Cyc_Absyn_Exp*_tmp2AC[2U];_tmp2AC[0]=rgn,_tmp2AC[1]=s;({struct Cyc_Absyn_Exp*_tmpD53=Cyc_Toc__xregion_malloc_e;Cyc_Toc_fncall_exp_dl(_tmpD53,_tag_fat(_tmp2AC,sizeof(struct Cyc_Absyn_Exp*),2U));});});else{
# 1462
return({struct Cyc_Absyn_Exp*_tmp2AD[2U];_tmp2AD[0]=rgn,_tmp2AD[1]=s;({struct Cyc_Absyn_Exp*_tmpD54=Cyc_Toc__xregion_calloc_e;Cyc_Toc_fncall_exp_dl(_tmpD54,_tag_fat(_tmp2AD,sizeof(struct Cyc_Absyn_Exp*),2U));});});}}}else{goto _LL3;}}else{goto _LL3;}}else{goto _LL3;}}else{_LL3: _LL4:
 goto _LL0;}_LL0:;}
# 1452
return 0;}
# 1469
static struct Cyc_Absyn_Exp*Cyc_Toc_rmalloc_exp(struct Cyc_Absyn_Exp*rgn,struct Cyc_Absyn_Exp*s){
struct Cyc_Absyn_Exp*e=({Cyc_Toc_xrmalloc_exp(rgn,s,0);});
if((unsigned)e)return e;return({struct Cyc_Absyn_Exp*_tmp2AF[2U];_tmp2AF[0]=rgn,_tmp2AF[1]=s;({struct Cyc_Absyn_Exp*_tmpD55=Cyc_Toc__region_malloc_e;Cyc_Toc_fncall_exp_dl(_tmpD55,_tag_fat(_tmp2AF,sizeof(struct Cyc_Absyn_Exp*),2U));});});}
# 1475
static struct Cyc_Absyn_Exp*Cyc_Toc_rmalloc_inline_exp(struct Cyc_Absyn_Exp*rgn,struct Cyc_Absyn_Exp*s){
struct Cyc_Absyn_Exp*e=({Cyc_Toc_xrmalloc_exp(rgn,s,0);});
if((unsigned)e)return e;return({struct Cyc_Absyn_Exp*_tmp2B1[2U];_tmp2B1[0]=rgn,_tmp2B1[1]=s;({struct Cyc_Absyn_Exp*_tmpD56=Cyc_Toc__fast_region_malloc_e;Cyc_Toc_fncall_exp_dl(_tmpD56,_tag_fat(_tmp2B1,sizeof(struct Cyc_Absyn_Exp*),2U));});});}
# 1481
static struct Cyc_Absyn_Exp*Cyc_Toc_rcalloc_exp(struct Cyc_Absyn_Exp*rgn,struct Cyc_Absyn_Exp*s,struct Cyc_Absyn_Exp*n){
struct Cyc_Absyn_Exp*e=({Cyc_Toc_xrmalloc_exp(rgn,s,1);});
if((unsigned)e)return e;return({struct Cyc_Absyn_Exp*_tmp2B3[3U];_tmp2B3[0]=rgn,_tmp2B3[1]=s,_tmp2B3[2]=n;({struct Cyc_Absyn_Exp*_tmpD57=Cyc_Toc__region_calloc_e;Cyc_Toc_fncall_exp_dl(_tmpD57,_tag_fat(_tmp2B3,sizeof(struct Cyc_Absyn_Exp*),3U));});});}
# 1487
static struct Cyc_Absyn_Stmt*Cyc_Toc_throw_match_stmt(){
return({struct Cyc_Absyn_Stmt*(*_tmp2B5)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_exp_stmt;struct Cyc_Absyn_Exp*_tmp2B6=({void*_tmp2B7=0U;({struct Cyc_Absyn_Exp*_tmpD58=Cyc_Toc__throw_match_e;Cyc_Toc_fncall_exp_dl(_tmpD58,_tag_fat(_tmp2B7,sizeof(struct Cyc_Absyn_Exp*),0U));});});unsigned _tmp2B8=0U;_tmp2B5(_tmp2B6,_tmp2B8);});}
# 1492
static struct Cyc_Absyn_Exp*Cyc_Toc_make_toplevel_dyn_arr(void*t,struct Cyc_Absyn_Exp*sz,struct Cyc_Absyn_Exp*e){
# 1500
struct Cyc_Absyn_Exp*xexp;
struct Cyc_Absyn_Exp*xplussz;
{void*_stmttmp1E=e->r;void*_tmp2BA=_stmttmp1E;if(((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp2BA)->tag == 0U)switch(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp2BA)->f1).Wstring_c).tag){case 8U: _LL1: _LL2:
 goto _LL4;case 9U: _LL3: _LL4: {
# 1505
struct _tuple1*x=({Cyc_Toc_temp_var();});
void*vd_typ=({Cyc_Absyn_array_type(Cyc_Absyn_char_type,Cyc_Toc_mt_tq,sz,Cyc_Absyn_false_type,0U);});
struct Cyc_Absyn_Vardecl*vd=({Cyc_Absyn_static_vardecl(x,vd_typ,e);});
Cyc_Toc_result_decls=({struct Cyc_List_List*_tmp2BC=_cycalloc(sizeof(*_tmp2BC));({struct Cyc_Absyn_Decl*_tmpD59=({Cyc_Absyn_new_decl((void*)({struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*_tmp2BB=_cycalloc(sizeof(*_tmp2BB));_tmp2BB->tag=0U,_tmp2BB->f1=vd;_tmp2BB;}),0U);});_tmp2BC->hd=_tmpD59;}),_tmp2BC->tl=Cyc_Toc_result_decls;_tmp2BC;});
xexp=({Cyc_Absyn_var_exp(x,0U);});
xplussz=({Cyc_Absyn_add_exp(xexp,sz,0U);});
goto _LL0;}default: goto _LL5;}else{_LL5: _LL6:
# 1513
 xexp=({struct Cyc_Absyn_Exp*(*_tmp2BD)(void*t,struct Cyc_Absyn_Exp*e)=Cyc_Toc_cast_it;void*_tmp2BE=({Cyc_Toc_void_star_type();});struct Cyc_Absyn_Exp*_tmp2BF=e;_tmp2BD(_tmp2BE,_tmp2BF);});
# 1515
xplussz=({struct Cyc_Absyn_Exp*(*_tmp2C0)(void*t,struct Cyc_Absyn_Exp*e)=Cyc_Toc_cast_it;void*_tmp2C1=({Cyc_Toc_void_star_type();});struct Cyc_Absyn_Exp*_tmp2C2=({Cyc_Absyn_add_exp(e,sz,0U);});_tmp2C0(_tmp2C1,_tmp2C2);});
goto _LL0;}_LL0:;}
# 1518
return({struct Cyc_Absyn_Exp*(*_tmp2C3)(struct Cyc_Core_Opt*,struct Cyc_List_List*,unsigned)=Cyc_Absyn_unresolvedmem_exp;struct Cyc_Core_Opt*_tmp2C4=0;struct Cyc_List_List*_tmp2C5=({struct _tuple21*_tmp2C6[3U];({struct _tuple21*_tmpD5C=({struct _tuple21*_tmp2C7=_cycalloc(sizeof(*_tmp2C7));_tmp2C7->f1=0,_tmp2C7->f2=xexp;_tmp2C7;});_tmp2C6[0]=_tmpD5C;}),({
struct _tuple21*_tmpD5B=({struct _tuple21*_tmp2C8=_cycalloc(sizeof(*_tmp2C8));_tmp2C8->f1=0,_tmp2C8->f2=xexp;_tmp2C8;});_tmp2C6[1]=_tmpD5B;}),({
struct _tuple21*_tmpD5A=({struct _tuple21*_tmp2C9=_cycalloc(sizeof(*_tmp2C9));_tmp2C9->f1=0,_tmp2C9->f2=xplussz;_tmp2C9;});_tmp2C6[2]=_tmpD5A;});Cyc_List_list(_tag_fat(_tmp2C6,sizeof(struct _tuple21*),3U));});unsigned _tmp2CA=0U;_tmp2C3(_tmp2C4,_tmp2C5,_tmp2CA);});}struct Cyc_Toc_FallthruInfo{struct _fat_ptr*label;struct Cyc_List_List*binders;};struct Cyc_Toc_Env{struct _fat_ptr**break_lab;struct _fat_ptr**continue_lab;struct Cyc_Toc_FallthruInfo*fallthru_info;int toplevel;int*in_lhs;struct _RegionHandle*rgn;};
# 1553 "toc.cyc"
static void Cyc_Toc_fndecl_to_c(struct Cyc_Toc_Env*nv,struct Cyc_Absyn_Fndecl*f,int cinclude);
# 1556
static struct Cyc_Toc_Env*Cyc_Toc_empty_env(struct _RegionHandle*r){
return({struct Cyc_Toc_Env*_tmp2CD=_region_malloc(r,sizeof(*_tmp2CD));_tmp2CD->break_lab=0,_tmp2CD->continue_lab=0,_tmp2CD->fallthru_info=0,_tmp2CD->toplevel=1,({int*_tmpD5D=({int*_tmp2CC=_region_malloc(r,sizeof(*_tmp2CC));*_tmp2CC=0;_tmp2CC;});_tmp2CD->in_lhs=_tmpD5D;}),_tmp2CD->rgn=r;_tmp2CD;});}
# 1560
static int Cyc_Toc_is_toplevel(struct Cyc_Toc_Env*nv){
int _tmp2CF;_LL1: _tmp2CF=nv->toplevel;_LL2: {int t=_tmp2CF;
return t;}}
# 1564
static struct Cyc_Toc_Env*Cyc_Toc_clear_toplevel(struct _RegionHandle*r,struct Cyc_Toc_Env*e){
int*_tmp2D5;int _tmp2D4;struct Cyc_Toc_FallthruInfo*_tmp2D3;struct _fat_ptr**_tmp2D2;struct _fat_ptr**_tmp2D1;_LL1: _tmp2D1=e->break_lab;_tmp2D2=e->continue_lab;_tmp2D3=e->fallthru_info;_tmp2D4=e->toplevel;_tmp2D5=e->in_lhs;_LL2: {struct _fat_ptr**b=_tmp2D1;struct _fat_ptr**c=_tmp2D2;struct Cyc_Toc_FallthruInfo*f=_tmp2D3;int t=_tmp2D4;int*lhs=_tmp2D5;
return({struct Cyc_Toc_Env*_tmp2D6=_region_malloc(r,sizeof(*_tmp2D6));_tmp2D6->break_lab=b,_tmp2D6->continue_lab=c,_tmp2D6->fallthru_info=f,_tmp2D6->toplevel=0,_tmp2D6->in_lhs=lhs,_tmp2D6->rgn=r;_tmp2D6;});}}
# 1568
static struct Cyc_Toc_Env*Cyc_Toc_set_toplevel(struct _RegionHandle*r,struct Cyc_Toc_Env*e){
int*_tmp2DC;int _tmp2DB;struct Cyc_Toc_FallthruInfo*_tmp2DA;struct _fat_ptr**_tmp2D9;struct _fat_ptr**_tmp2D8;_LL1: _tmp2D8=e->break_lab;_tmp2D9=e->continue_lab;_tmp2DA=e->fallthru_info;_tmp2DB=e->toplevel;_tmp2DC=e->in_lhs;_LL2: {struct _fat_ptr**b=_tmp2D8;struct _fat_ptr**c=_tmp2D9;struct Cyc_Toc_FallthruInfo*f=_tmp2DA;int t=_tmp2DB;int*lhs=_tmp2DC;
return({struct Cyc_Toc_Env*_tmp2DD=_region_malloc(r,sizeof(*_tmp2DD));_tmp2DD->break_lab=b,_tmp2DD->continue_lab=c,_tmp2DD->fallthru_info=f,_tmp2DD->toplevel=1,_tmp2DD->in_lhs=lhs,_tmp2DD->rgn=r;_tmp2DD;});}}
# 1572
static int Cyc_Toc_in_lhs(struct Cyc_Toc_Env*nv){
int*_tmp2DF;_LL1: _tmp2DF=nv->in_lhs;_LL2: {int*b=_tmp2DF;
return*b;}}
# 1576
static void Cyc_Toc_set_lhs(struct Cyc_Toc_Env*e,int b){
int*_tmp2E1;_LL1: _tmp2E1=e->in_lhs;_LL2: {int*lhs=_tmp2E1;
*lhs=b;}}
# 1581
static struct Cyc_Toc_Env*Cyc_Toc_share_env(struct _RegionHandle*r,struct Cyc_Toc_Env*e){
int*_tmp2E7;int _tmp2E6;struct Cyc_Toc_FallthruInfo*_tmp2E5;struct _fat_ptr**_tmp2E4;struct _fat_ptr**_tmp2E3;_LL1: _tmp2E3=e->break_lab;_tmp2E4=e->continue_lab;_tmp2E5=e->fallthru_info;_tmp2E6=e->toplevel;_tmp2E7=e->in_lhs;_LL2: {struct _fat_ptr**b=_tmp2E3;struct _fat_ptr**c=_tmp2E4;struct Cyc_Toc_FallthruInfo*f=_tmp2E5;int t=_tmp2E6;int*lhs=_tmp2E7;
return({struct Cyc_Toc_Env*_tmp2E8=_region_malloc(r,sizeof(*_tmp2E8));_tmp2E8->break_lab=b,_tmp2E8->continue_lab=c,_tmp2E8->fallthru_info=f,_tmp2E8->toplevel=t,_tmp2E8->in_lhs=lhs,_tmp2E8->rgn=r;_tmp2E8;});}}
# 1588
static struct Cyc_Toc_Env*Cyc_Toc_loop_env(struct _RegionHandle*r,struct Cyc_Toc_Env*e){
int*_tmp2EE;int _tmp2ED;struct Cyc_Toc_FallthruInfo*_tmp2EC;struct _fat_ptr**_tmp2EB;struct _fat_ptr**_tmp2EA;_LL1: _tmp2EA=e->break_lab;_tmp2EB=e->continue_lab;_tmp2EC=e->fallthru_info;_tmp2ED=e->toplevel;_tmp2EE=e->in_lhs;_LL2: {struct _fat_ptr**b=_tmp2EA;struct _fat_ptr**c=_tmp2EB;struct Cyc_Toc_FallthruInfo*f=_tmp2EC;int t=_tmp2ED;int*lhs=_tmp2EE;
return({struct Cyc_Toc_Env*_tmp2EF=_region_malloc(r,sizeof(*_tmp2EF));_tmp2EF->break_lab=0,_tmp2EF->continue_lab=0,_tmp2EF->fallthru_info=f,_tmp2EF->toplevel=t,_tmp2EF->in_lhs=lhs,_tmp2EF->rgn=r;_tmp2EF;});}}
# 1595
static struct Cyc_Toc_Env*Cyc_Toc_non_last_switchclause_env(struct _RegionHandle*r,struct Cyc_Toc_Env*e,struct _fat_ptr*break_l,struct _fat_ptr*fallthru_l,struct Cyc_List_List*fallthru_binders){
# 1598
int*_tmp2F5;int _tmp2F4;struct Cyc_Toc_FallthruInfo*_tmp2F3;struct _fat_ptr**_tmp2F2;struct _fat_ptr**_tmp2F1;_LL1: _tmp2F1=e->break_lab;_tmp2F2=e->continue_lab;_tmp2F3=e->fallthru_info;_tmp2F4=e->toplevel;_tmp2F5=e->in_lhs;_LL2: {struct _fat_ptr**b=_tmp2F1;struct _fat_ptr**c=_tmp2F2;struct Cyc_Toc_FallthruInfo*f=_tmp2F3;int t=_tmp2F4;int*lhs=_tmp2F5;
struct Cyc_Toc_FallthruInfo*fi=({struct Cyc_Toc_FallthruInfo*_tmp2F8=_region_malloc(r,sizeof(*_tmp2F8));
_tmp2F8->label=fallthru_l,_tmp2F8->binders=fallthru_binders;_tmp2F8;});
return({struct Cyc_Toc_Env*_tmp2F7=_region_malloc(r,sizeof(*_tmp2F7));({struct _fat_ptr**_tmpD5E=({struct _fat_ptr**_tmp2F6=_region_malloc(r,sizeof(*_tmp2F6));*_tmp2F6=break_l;_tmp2F6;});_tmp2F7->break_lab=_tmpD5E;}),_tmp2F7->continue_lab=c,_tmp2F7->fallthru_info=fi,_tmp2F7->toplevel=t,_tmp2F7->in_lhs=lhs,_tmp2F7->rgn=r;_tmp2F7;});}}
# 1604
static struct Cyc_Toc_Env*Cyc_Toc_last_switchclause_env(struct _RegionHandle*r,struct Cyc_Toc_Env*e,struct _fat_ptr*break_l){
int*_tmp2FE;int _tmp2FD;struct Cyc_Toc_FallthruInfo*_tmp2FC;struct _fat_ptr**_tmp2FB;struct _fat_ptr**_tmp2FA;_LL1: _tmp2FA=e->break_lab;_tmp2FB=e->continue_lab;_tmp2FC=e->fallthru_info;_tmp2FD=e->toplevel;_tmp2FE=e->in_lhs;_LL2: {struct _fat_ptr**b=_tmp2FA;struct _fat_ptr**c=_tmp2FB;struct Cyc_Toc_FallthruInfo*f=_tmp2FC;int t=_tmp2FD;int*lhs=_tmp2FE;
return({struct Cyc_Toc_Env*_tmp300=_region_malloc(r,sizeof(*_tmp300));({struct _fat_ptr**_tmpD5F=({struct _fat_ptr**_tmp2FF=_region_malloc(r,sizeof(*_tmp2FF));*_tmp2FF=break_l;_tmp2FF;});_tmp300->break_lab=_tmpD5F;}),_tmp300->continue_lab=c,_tmp300->fallthru_info=0,_tmp300->toplevel=t,_tmp300->in_lhs=lhs,_tmp300->rgn=r;_tmp300;});}}
# 1613
static struct Cyc_Toc_Env*Cyc_Toc_switch_as_switch_env(struct _RegionHandle*r,struct Cyc_Toc_Env*e,struct _fat_ptr*next_l){
int*_tmp306;int _tmp305;struct Cyc_Toc_FallthruInfo*_tmp304;struct _fat_ptr**_tmp303;struct _fat_ptr**_tmp302;_LL1: _tmp302=e->break_lab;_tmp303=e->continue_lab;_tmp304=e->fallthru_info;_tmp305=e->toplevel;_tmp306=e->in_lhs;_LL2: {struct _fat_ptr**b=_tmp302;struct _fat_ptr**c=_tmp303;struct Cyc_Toc_FallthruInfo*f=_tmp304;int t=_tmp305;int*lhs=_tmp306;
return({struct Cyc_Toc_Env*_tmp308=_region_malloc(r,sizeof(*_tmp308));_tmp308->break_lab=0,_tmp308->continue_lab=c,({struct Cyc_Toc_FallthruInfo*_tmpD60=({struct Cyc_Toc_FallthruInfo*_tmp307=_region_malloc(r,sizeof(*_tmp307));_tmp307->label=next_l,_tmp307->binders=0;_tmp307;});_tmp308->fallthru_info=_tmpD60;}),_tmp308->toplevel=t,_tmp308->in_lhs=lhs,_tmp308->rgn=r;_tmp308;});}}
# 1613
static void Cyc_Toc_exp_to_c(struct Cyc_Toc_Env*nv,struct Cyc_Absyn_Exp*e);
# 1628 "toc.cyc"
static void Cyc_Toc_stmt_to_c(struct Cyc_Toc_Env*nv,struct Cyc_Absyn_Stmt*s);
# 1630
int Cyc_Toc_do_null_check(struct Cyc_Absyn_Exp*e){
void*_stmttmp1F=e->annot;void*_tmp30A=_stmttmp1F;if(((struct Cyc_InsertChecks_NoCheck_Absyn_AbsynAnnot_struct*)_tmp30A)->tag == Cyc_InsertChecks_NoCheck){_LL1: _LL2:
 return 0;}else{if(((struct Cyc_InsertChecks_NullOnly_Absyn_AbsynAnnot_struct*)_tmp30A)->tag == Cyc_InsertChecks_NullOnly){_LL3: _LL4:
 goto _LL6;}else{if(((struct Cyc_InsertChecks_NullAndFatBound_Absyn_AbsynAnnot_struct*)_tmp30A)->tag == Cyc_InsertChecks_NullAndFatBound){_LL5: _LL6:
 goto _LL8;}else{if(((struct Cyc_InsertChecks_NullAndThinBound_Absyn_AbsynAnnot_struct*)_tmp30A)->tag == Cyc_InsertChecks_NullAndThinBound){_LL7: _LL8:
 return 1;}else{if(((struct Cyc_InsertChecks_FatBound_Absyn_AbsynAnnot_struct*)_tmp30A)->tag == Cyc_InsertChecks_FatBound){_LL9: _LLA:
 goto _LLC;}else{if(((struct Cyc_InsertChecks_ThinBound_Absyn_AbsynAnnot_struct*)_tmp30A)->tag == Cyc_InsertChecks_ThinBound){_LLB: _LLC:
 return 0;}else{_LLD: _LLE:
({struct Cyc_String_pa_PrintArg_struct _tmp30E=({struct Cyc_String_pa_PrintArg_struct _tmpC7C;_tmpC7C.tag=0U,({
struct _fat_ptr _tmpD61=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_exp2string(e);}));_tmpC7C.f1=_tmpD61;});_tmpC7C;});void*_tmp30B[1U];_tmp30B[0]=& _tmp30E;({int(*_tmpD64)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap)=({
# 1638
int(*_tmp30D)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos_loc;_tmp30D;});unsigned _tmpD63=e->loc;struct _fat_ptr _tmpD62=({const char*_tmp30C="Toc: do_null_check (Exp: \"%s\")";_tag_fat(_tmp30C,sizeof(char),31U);});_tmpD64(_tmpD63,_tmpD62,_tag_fat(_tmp30B,sizeof(void*),1U));});});}}}}}}_LL0:;}
# 1647
static int Cyc_Toc_ptr_use_to_c(struct Cyc_Toc_Env*nv,struct Cyc_Absyn_Exp*ptr,void*annot,struct Cyc_Absyn_Exp*index){
# 1649
int ans;
int old_lhs=({Cyc_Toc_in_lhs(nv);});
void*old_typ=({Cyc_Tcutil_compress((void*)_check_null(ptr->topt));});
void*new_typ=({Cyc_Toc_typ_to_c(old_typ);});
struct Cyc_Absyn_Exp*fn_e=Cyc_Toc__check_known_subscript_notnull_e;
({Cyc_Toc_set_lhs(nv,0);});
({Cyc_Toc_exp_to_c(nv,ptr);});
if(index != 0)
({Cyc_Toc_exp_to_c(nv,index);});{
# 1656
void*_tmp310=old_typ;void*_tmp314;void*_tmp313;struct Cyc_Absyn_Tqual _tmp312;void*_tmp311;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp310)->tag == 3U){_LL1: _tmp311=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp310)->f1).elt_type;_tmp312=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp310)->f1).elt_tq;_tmp313=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp310)->f1).ptr_atts).bounds;_tmp314=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp310)->f1).ptr_atts).zero_term;_LL2: {void*ta=_tmp311;struct Cyc_Absyn_Tqual tq=_tmp312;void*b=_tmp313;void*zt=_tmp314;
# 1660
{void*_tmp315=annot;struct Cyc_Absyn_Exp*_tmp316;struct Cyc_Absyn_Exp*_tmp317;if(((struct Cyc_InsertChecks_NoCheck_Absyn_AbsynAnnot_struct*)_tmp315)->tag == Cyc_InsertChecks_NoCheck){_LL6: _LL7:
# 1662
 if(!((unsigned)({struct Cyc_Absyn_Exp*(*_tmp318)(void*def,void*b)=Cyc_Tcutil_get_bounds_exp;void*_tmp319=({Cyc_Absyn_bounds_one();});void*_tmp31A=b;_tmp318(_tmp319,_tmp31A);}))){
# 1665
void*newt=({void*(*_tmp322)(void*,struct Cyc_Absyn_Tqual)=Cyc_Absyn_cstar_type;void*_tmp323=({Cyc_Toc_typ_to_c(ta);});struct Cyc_Absyn_Tqual _tmp324=tq;_tmp322(_tmp323,_tmp324);});
({void*_tmpD65=({void*(*_tmp31B)(void*t,struct Cyc_Absyn_Exp*e)=Cyc_Toc_cast_it_r;void*_tmp31C=newt;struct Cyc_Absyn_Exp*_tmp31D=({struct Cyc_Absyn_Exp*(*_tmp31E)(struct Cyc_Absyn_Exp*e,struct _fat_ptr*f,unsigned loc)=Cyc_Toc_member_exp;struct Cyc_Absyn_Exp*_tmp31F=({Cyc_Absyn_new_exp(ptr->r,0U);});struct _fat_ptr*_tmp320=Cyc_Toc_curr_sp;unsigned _tmp321=0U;_tmp31E(_tmp31F,_tmp320,_tmp321);});_tmp31B(_tmp31C,_tmp31D);});ptr->r=_tmpD65;});
ptr->topt=newt;}
# 1662
ans=0;
# 1670
goto _LL5;}else{if(((struct Cyc_InsertChecks_NullOnly_Absyn_AbsynAnnot_struct*)_tmp315)->tag == Cyc_InsertChecks_NullOnly){_LL8: _LL9:
# 1672
 if(!((unsigned)({struct Cyc_Absyn_Exp*(*_tmp325)(void*def,void*b)=Cyc_Tcutil_get_bounds_exp;void*_tmp326=({Cyc_Absyn_bounds_one();});void*_tmp327=b;_tmp325(_tmp326,_tmp327);}))){
# 1674
void*newt=({void*(*_tmp332)(void*,struct Cyc_Absyn_Tqual)=Cyc_Absyn_cstar_type;void*_tmp333=({Cyc_Toc_typ_to_c(ta);});struct Cyc_Absyn_Tqual _tmp334=tq;_tmp332(_tmp333,_tmp334);});
({void*_tmpD66=({void*(*_tmp328)(void*t,struct Cyc_Absyn_Exp*e)=Cyc_Toc_cast_it_r;void*_tmp329=newt;struct Cyc_Absyn_Exp*_tmp32A=({struct Cyc_Absyn_Exp*(*_tmp32B)(struct Cyc_Absyn_Exp*e,struct _fat_ptr*f,unsigned loc)=Cyc_Toc_member_exp;struct Cyc_Absyn_Exp*_tmp32C=({Cyc_Absyn_new_exp(ptr->r,0U);});struct _fat_ptr*_tmp32D=Cyc_Toc_curr_sp;unsigned _tmp32E=0U;_tmp32B(_tmp32C,_tmp32D,_tmp32E);});_tmp328(_tmp329,_tmp32A);});ptr->r=_tmpD66;});
ptr->topt=newt;
# 1681
if(index != 0)
({void*_tmp32F=0U;({int(*_tmpD68)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp331)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmp331;});struct _fat_ptr _tmpD67=({const char*_tmp330="subscript of ? with no bounds check but need null check";_tag_fat(_tmp330,sizeof(char),56U);});_tmpD68(_tmpD67,_tag_fat(_tmp32F,sizeof(void*),0U));});});}
# 1672
({void*_tmpD6B=({void*(*_tmp335)(void*t,struct Cyc_Absyn_Exp*e)=Cyc_Toc_cast_it_r;void*_tmp336=new_typ;struct Cyc_Absyn_Exp*_tmp337=({struct Cyc_Absyn_Exp*_tmp338[1U];({struct Cyc_Absyn_Exp*_tmpD69=({Cyc_Absyn_new_exp(ptr->r,0U);});_tmp338[0]=_tmpD69;});({struct Cyc_Absyn_Exp*_tmpD6A=Cyc_Toc__check_null_e;Cyc_Toc_fncall_exp_dl(_tmpD6A,_tag_fat(_tmp338,sizeof(struct Cyc_Absyn_Exp*),1U));});});_tmp335(_tmp336,_tmp337);});ptr->r=_tmpD6B;});
# 1686
ans=0;
goto _LL5;}else{if(((struct Cyc_InsertChecks_NullAndFatBound_Absyn_AbsynAnnot_struct*)_tmp315)->tag == Cyc_InsertChecks_NullAndFatBound){_LLA: _LLB:
 goto _LLD;}else{if(((struct Cyc_InsertChecks_FatBound_Absyn_AbsynAnnot_struct*)_tmp315)->tag == Cyc_InsertChecks_FatBound){_LLC: _LLD: {
# 1691
void*ta1=({Cyc_Toc_typ_to_c(ta);});
void*newt=({Cyc_Absyn_cstar_type(ta1,tq);});
struct Cyc_Absyn_Exp*ind=(unsigned)index?index:({Cyc_Absyn_uint_exp(0U,0U);});
({void*_tmpD6F=({void*(*_tmp339)(void*t,struct Cyc_Absyn_Exp*e)=Cyc_Toc_cast_it_r;void*_tmp33A=newt;struct Cyc_Absyn_Exp*_tmp33B=({struct Cyc_Absyn_Exp*_tmp33C[3U];({
struct Cyc_Absyn_Exp*_tmpD6D=({Cyc_Absyn_new_exp(ptr->r,0U);});_tmp33C[0]=_tmpD6D;}),({
struct Cyc_Absyn_Exp*_tmpD6C=({Cyc_Absyn_sizeoftype_exp(ta1,0U);});_tmp33C[1]=_tmpD6C;}),_tmp33C[2]=ind;({struct Cyc_Absyn_Exp*_tmpD6E=Cyc_Toc__check_fat_subscript_e;Cyc_Toc_fncall_exp_dl(_tmpD6E,_tag_fat(_tmp33C,sizeof(struct Cyc_Absyn_Exp*),3U));});});_tmp339(_tmp33A,_tmp33B);});
# 1694
ptr->r=_tmpD6F;});
# 1698
ptr->topt=newt;
ans=1;
goto _LL5;}}else{if(((struct Cyc_InsertChecks_NullAndThinBound_Absyn_AbsynAnnot_struct*)_tmp315)->tag == Cyc_InsertChecks_NullAndThinBound){_LLE: _tmp317=((struct Cyc_InsertChecks_NullAndThinBound_Absyn_AbsynAnnot_struct*)_tmp315)->f1;_LLF: {struct Cyc_Absyn_Exp*bd=_tmp317;
# 1702
fn_e=Cyc_Toc__check_known_subscript_null_e;
_tmp316=bd;goto _LL11;}}else{if(((struct Cyc_InsertChecks_ThinBound_Absyn_AbsynAnnot_struct*)_tmp315)->tag == Cyc_InsertChecks_ThinBound){_LL10: _tmp316=((struct Cyc_InsertChecks_ThinBound_Absyn_AbsynAnnot_struct*)_tmp315)->f1;_LL11: {struct Cyc_Absyn_Exp*bd=_tmp316;
# 1705
void*ta1=({Cyc_Toc_typ_to_c(ta);});
struct Cyc_Absyn_Exp*ind=(unsigned)index?index:({Cyc_Absyn_uint_exp(0U,0U);});
# 1710
struct _tuple15 _stmttmp20=({Cyc_Evexp_eval_const_uint_exp(bd);});int _tmp33E;unsigned _tmp33D;_LL15: _tmp33D=_stmttmp20.f1;_tmp33E=_stmttmp20.f2;_LL16: {unsigned i=_tmp33D;int valid=_tmp33E;
if((!valid || i != (unsigned)1)|| !({Cyc_Tcutil_is_zeroterm_pointer_type((void*)_check_null(ptr->topt));})){
# 1713
({void*_tmpD73=({void*(*_tmp33F)(void*t,struct Cyc_Absyn_Exp*e)=Cyc_Toc_cast_it_r;void*_tmp340=({Cyc_Absyn_cstar_type(ta1,tq);});struct Cyc_Absyn_Exp*_tmp341=({struct Cyc_Absyn_Exp*_tmp342[4U];({
struct Cyc_Absyn_Exp*_tmpD71=({Cyc_Absyn_new_exp(ptr->r,0U);});_tmp342[0]=_tmpD71;}),_tmp342[1]=bd,({
struct Cyc_Absyn_Exp*_tmpD70=({Cyc_Absyn_sizeoftype_exp(ta1,0U);});_tmp342[2]=_tmpD70;}),_tmp342[3]=ind;({struct Cyc_Absyn_Exp*_tmpD72=fn_e;Cyc_Toc_fncall_exp_dl(_tmpD72,_tag_fat(_tmp342,sizeof(struct Cyc_Absyn_Exp*),4U));});});_tmp33F(_tmp340,_tmp341);});
# 1713
ptr->r=_tmpD73;});
# 1716
ans=1;}else{
# 1719
if(({Cyc_Toc_is_zero(bd);})){
if(fn_e == Cyc_Toc__check_known_subscript_null_e)
# 1722
({void*_tmpD76=({void*(*_tmp343)(void*t,struct Cyc_Absyn_Exp*e)=Cyc_Toc_cast_it_r;void*_tmp344=new_typ;struct Cyc_Absyn_Exp*_tmp345=({struct Cyc_Absyn_Exp*_tmp346[1U];({struct Cyc_Absyn_Exp*_tmpD74=({Cyc_Absyn_new_exp(ptr->r,0U);});_tmp346[0]=_tmpD74;});({struct Cyc_Absyn_Exp*_tmpD75=Cyc_Toc__check_null_e;Cyc_Toc_fncall_exp_dl(_tmpD75,_tag_fat(_tmp346,sizeof(struct Cyc_Absyn_Exp*),1U));});});_tmp343(_tmp344,_tmp345);});ptr->r=_tmpD76;});
# 1720
ans=0;}else{
# 1728
fn_e=({Cyc_Toc_getFunctionRemovePointer(& Cyc_Toc__zero_arr_plus_functionSet,ptr);});
({void*_tmpD79=({void*(*_tmp347)(void*t,struct Cyc_Absyn_Exp*e)=Cyc_Toc_cast_it_r;void*_tmp348=({Cyc_Absyn_cstar_type(ta1,tq);});struct Cyc_Absyn_Exp*_tmp349=({struct Cyc_Absyn_Exp*_tmp34A[3U];({
struct Cyc_Absyn_Exp*_tmpD77=({Cyc_Absyn_new_exp(ptr->r,0U);});_tmp34A[0]=_tmpD77;}),_tmp34A[1]=bd,_tmp34A[2]=ind;({struct Cyc_Absyn_Exp*_tmpD78=fn_e;Cyc_Toc_fncall_exp_dl(_tmpD78,_tag_fat(_tmp34A,sizeof(struct Cyc_Absyn_Exp*),3U));});});_tmp347(_tmp348,_tmp349);});
# 1729
ptr->r=_tmpD79;});
# 1731
ans=1;}}
# 1734
goto _LL5;}}}else{_LL12: _LL13:
({void*_tmp34B=0U;({int(*_tmpD7B)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp34D)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp34D;});struct _fat_ptr _tmpD7A=({const char*_tmp34C="FIX: ptr_use_to_c, bad annotation";_tag_fat(_tmp34C,sizeof(char),34U);});_tmpD7B(_tmpD7A,_tag_fat(_tmp34B,sizeof(void*),0U));});});}}}}}}_LL5:;}
# 1737
({Cyc_Toc_set_lhs(nv,old_lhs);});
return ans;}}else{_LL3: _LL4:
({void*_tmp34E=0U;({int(*_tmpD7D)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp350)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp350;});struct _fat_ptr _tmpD7C=({const char*_tmp34F="ptr_use_to_c: non-pointer-type";_tag_fat(_tmp34F,sizeof(char),31U);});_tmpD7D(_tmpD7C,_tag_fat(_tmp34E,sizeof(void*),0U));});});}_LL0:;}}
# 1743
static void*Cyc_Toc_get_cyc_type(struct Cyc_Absyn_Exp*e){
if(e->topt == 0)({void*_tmp352=0U;({int(*_tmpD7F)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp354)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmp354;});struct _fat_ptr _tmpD7E=({const char*_tmp353="Missing type in primop ";_tag_fat(_tmp353,sizeof(char),24U);});_tmpD7F(_tmpD7E,_tag_fat(_tmp352,sizeof(void*),0U));});});return(void*)_check_null(e->topt);}
# 1747
static struct _tuple14*Cyc_Toc_tup_to_c(struct Cyc_Absyn_Exp*e){
return({struct _tuple14*_tmp356=_cycalloc(sizeof(*_tmp356));_tmp356->f1=Cyc_Toc_mt_tq,({void*_tmpD80=({Cyc_Toc_typ_to_c((void*)_check_null(e->topt));});_tmp356->f2=_tmpD80;});_tmp356;});}
# 1752
static struct Cyc_Absyn_Exp*Cyc_Toc_array_length_exp(struct Cyc_Absyn_Exp*e){
void*_stmttmp21=e->r;void*_tmp358=_stmttmp21;int _tmp35A;struct Cyc_Absyn_Exp*_tmp359;int _tmp35C;struct Cyc_Absyn_Exp*_tmp35B;struct Cyc_List_List*_tmp35D;switch(*((int*)_tmp358)){case 26U: _LL1: _tmp35D=((struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*)_tmp358)->f1;_LL2: {struct Cyc_List_List*dles=_tmp35D;
# 1755
{struct Cyc_List_List*dles2=dles;for(0;dles2 != 0;dles2=dles2->tl){
if((*((struct _tuple21*)dles2->hd)).f1 != 0)
({void*_tmp35E=0U;({int(*_tmpD82)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp360)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_unimp;_tmp360;});struct _fat_ptr _tmpD81=({const char*_tmp35F="array designators for abstract-field initialization";_tag_fat(_tmp35F,sizeof(char),52U);});_tmpD82(_tmpD81,_tag_fat(_tmp35E,sizeof(void*),0U));});});}}
# 1755
_tmp35B=({struct Cyc_Absyn_Exp*(*_tmp361)(int,unsigned)=Cyc_Absyn_signed_int_exp;int _tmp362=({Cyc_List_length(dles);});unsigned _tmp363=0U;_tmp361(_tmp362,_tmp363);});_tmp35C=0;goto _LL4;}case 27U: _LL3: _tmp35B=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmp358)->f2;_tmp35C=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmp358)->f4;_LL4: {struct Cyc_Absyn_Exp*bd=_tmp35B;int zt=_tmp35C;
# 1759
_tmp359=bd;_tmp35A=zt;goto _LL6;}case 28U: _LL5: _tmp359=((struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*)_tmp358)->f1;_tmp35A=((struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*)_tmp358)->f3;_LL6: {struct Cyc_Absyn_Exp*bd=_tmp359;int zt=_tmp35A;
# 1761
bd=({Cyc_Absyn_copy_exp(bd);});
return zt?({struct Cyc_Absyn_Exp*(*_tmp364)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_add_exp;struct Cyc_Absyn_Exp*_tmp365=bd;struct Cyc_Absyn_Exp*_tmp366=({Cyc_Absyn_uint_exp(1U,0U);});unsigned _tmp367=0U;_tmp364(_tmp365,_tmp366,_tmp367);}): bd;}default: _LL7: _LL8:
 return 0;}_LL0:;}
# 1770
static struct Cyc_Absyn_Exp*Cyc_Toc_get_varsizeexp(struct Cyc_Absyn_Exp*e){
# 1778
struct Cyc_List_List*dles;
struct Cyc_List_List*field_types;
{void*_stmttmp22=e->r;void*_tmp369=_stmttmp22;struct Cyc_List_List*_tmp36A;if(((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_tmp369)->tag == 29U){_LL1: _tmp36A=((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_tmp369)->f3;_LL2: {struct Cyc_List_List*dles2=_tmp36A;
dles=dles2;goto _LL0;}}else{_LL3: _LL4:
 return 0;}_LL0:;}
# 1784
{void*_stmttmp23=({Cyc_Tcutil_compress((void*)_check_null(e->topt));});void*_tmp36B=_stmttmp23;struct Cyc_Absyn_Aggrdecl*_tmp36C;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp36B)->tag == 0U){if(((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp36B)->f1)->tag == 22U){if(((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp36B)->f1)->f1).KnownAggr).tag == 2){_LL6: _tmp36C=*((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp36B)->f1)->f1).KnownAggr).val;_LL7: {struct Cyc_Absyn_Aggrdecl*ad=_tmp36C;
# 1786
if(ad->impl == 0)
return 0;
# 1786
if((int)ad->kind == (int)Cyc_Absyn_UnionA)
# 1789
return 0;
# 1786
field_types=((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->fields;
# 1791
goto _LL5;}}else{goto _LL8;}}else{goto _LL8;}}else{_LL8: _LL9:
# 1794
 return 0;}_LL5:;}
# 1796
if(field_types == 0)
return 0;
# 1796
for(0;((struct Cyc_List_List*)_check_null(field_types))->tl != 0;field_types=field_types->tl){
# 1799
;}{
struct Cyc_Absyn_Aggrfield*last_type_field=(struct Cyc_Absyn_Aggrfield*)field_types->hd;
for(0;dles != 0;dles=dles->tl){
struct _tuple21*_stmttmp24=(struct _tuple21*)dles->hd;struct Cyc_Absyn_Exp*_tmp36E;struct Cyc_List_List*_tmp36D;_LLB: _tmp36D=_stmttmp24->f1;_tmp36E=_stmttmp24->f2;_LLC: {struct Cyc_List_List*ds=_tmp36D;struct Cyc_Absyn_Exp*e2=_tmp36E;
struct _fat_ptr*f=({Cyc_Absyn_designatorlist_to_fieldname(ds);});
if(!({Cyc_strptrcmp(f,last_type_field->name);})){
struct Cyc_Absyn_Exp*nested_ans=({Cyc_Toc_get_varsizeexp(e2);});
if(nested_ans != 0)
return nested_ans;{
# 1806
void*_stmttmp25=({Cyc_Tcutil_compress(last_type_field->type);});void*_tmp36F=_stmttmp25;struct Cyc_Absyn_Exp*_tmp371;void*_tmp370;if(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp36F)->tag == 4U){_LLE: _tmp370=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp36F)->f1).elt_type;_tmp371=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp36F)->f1).num_elts;_LLF: {void*elt_type=_tmp370;struct Cyc_Absyn_Exp*type_bd=_tmp371;
# 1811
if(type_bd == 0 || !({Cyc_Toc_is_zero(type_bd);}))
return 0;
# 1811
return({struct Cyc_Absyn_Exp*(*_tmp372)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_add_exp;struct Cyc_Absyn_Exp*_tmp373=({struct Cyc_Absyn_Exp*_tmp374[2U];_tmp374[0]=(struct Cyc_Absyn_Exp*)_check_null(({Cyc_Toc_array_length_exp(e2);})),({
# 1819
struct Cyc_Absyn_Exp*_tmpD83=({Cyc_Absyn_sizeoftype_exp(elt_type,0U);});_tmp374[1]=_tmpD83;});({struct Cyc_Absyn_Exp*_tmpD84=Cyc_Toc__check_times_e;Cyc_Toc_fncall_exp_dl(_tmpD84,_tag_fat(_tmp374,sizeof(struct Cyc_Absyn_Exp*),2U));});});struct Cyc_Absyn_Exp*_tmp375=({Cyc_Absyn_signed_int_exp((int)sizeof(double),0U);});unsigned _tmp376=0U;_tmp372(_tmp373,_tmp375,_tmp376);});}}else{_LL10: _LL11:
# 1821
 return 0;}_LLD:;}}}}
# 1825
({void*_tmp377=0U;({int(*_tmpD86)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp379)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmp379;});struct _fat_ptr _tmpD85=({const char*_tmp378="get_varsizeexp: did not find last struct field";_tag_fat(_tmp378,sizeof(char),47U);});_tmpD86(_tmpD85,_tag_fat(_tmp377,sizeof(void*),0U));});});}}
# 1828
static int Cyc_Toc_get_member_offset(struct Cyc_Absyn_Aggrdecl*ad,struct _fat_ptr*f){
int i=1;
{struct Cyc_List_List*fields=((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->fields;for(0;fields != 0;fields=fields->tl){
struct Cyc_Absyn_Aggrfield*field=(struct Cyc_Absyn_Aggrfield*)fields->hd;
if(({Cyc_strcmp((struct _fat_ptr)*field->name,(struct _fat_ptr)*f);})== 0)return i;++ i;}}
# 1835
({void*_tmp37B=0U;({int(*_tmpD89)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp37F)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmp37F;});struct _fat_ptr _tmpD88=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp37E=({struct Cyc_String_pa_PrintArg_struct _tmpC7D;_tmpC7D.tag=0U,_tmpC7D.f1=(struct _fat_ptr)((struct _fat_ptr)*f);_tmpC7D;});void*_tmp37C[1U];_tmp37C[0]=& _tmp37E;({struct _fat_ptr _tmpD87=({const char*_tmp37D="get_member_offset %s failed";_tag_fat(_tmp37D,sizeof(char),28U);});Cyc_aprintf(_tmpD87,_tag_fat(_tmp37C,sizeof(void*),1U));});});_tmpD89(_tmpD88,_tag_fat(_tmp37B,sizeof(void*),0U));});});}struct _tuple35{struct Cyc_Core_Opt*f1;struct Cyc_Absyn_Exp*f2;};
# 1839
static struct Cyc_Absyn_Exp*Cyc_Toc_assignop_lvalue(struct Cyc_Absyn_Exp*el,struct _tuple35*pr){
return({Cyc_Absyn_assignop_exp(el,(*pr).f1,(*pr).f2,0U);});}
# 1842
static struct Cyc_Absyn_Exp*Cyc_Toc_address_lvalue(struct Cyc_Absyn_Exp*e1,int ignore){
return({Cyc_Absyn_address_exp(e1,0U);});}
# 1845
static struct Cyc_Absyn_Exp*Cyc_Toc_incr_lvalue(struct Cyc_Absyn_Exp*e1,enum Cyc_Absyn_Incrementor incr){
return({Cyc_Absyn_new_exp((void*)({struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*_tmp383=_cycalloc(sizeof(*_tmp383));_tmp383->tag=5U,_tmp383->f1=e1,_tmp383->f2=incr;_tmp383;}),0U);});}
# 1845
static void Cyc_Toc_lvalue_assign_stmt(struct Cyc_Absyn_Stmt*s,struct Cyc_List_List*fs,struct Cyc_Absyn_Exp*(*f)(struct Cyc_Absyn_Exp*,void*),void*f_env);
# 1858
static void Cyc_Toc_lvalue_assign(struct Cyc_Absyn_Exp*e1,struct Cyc_List_List*fs,struct Cyc_Absyn_Exp*(*f)(struct Cyc_Absyn_Exp*,void*),void*f_env){
# 1860
void*_stmttmp26=e1->r;void*_tmp385=_stmttmp26;int _tmp389;int _tmp388;struct _fat_ptr*_tmp387;struct Cyc_Absyn_Exp*_tmp386;struct Cyc_Absyn_Exp*_tmp38B;void*_tmp38A;struct Cyc_Absyn_Stmt*_tmp38C;switch(*((int*)_tmp385)){case 38U: _LL1: _tmp38C=((struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct*)_tmp385)->f1;_LL2: {struct Cyc_Absyn_Stmt*s=_tmp38C;
({Cyc_Toc_lvalue_assign_stmt(s,fs,f,f_env);});goto _LL0;}case 14U: _LL3: _tmp38A=(void*)((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp385)->f1;_tmp38B=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp385)->f2;_LL4: {void*t=_tmp38A;struct Cyc_Absyn_Exp*e=_tmp38B;
({Cyc_Toc_lvalue_assign(e,fs,f,f_env);});goto _LL0;}case 21U: _LL5: _tmp386=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp385)->f1;_tmp387=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp385)->f2;_tmp388=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp385)->f3;_tmp389=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp385)->f4;_LL6: {struct Cyc_Absyn_Exp*e=_tmp386;struct _fat_ptr*fld=_tmp387;int is_tagged=_tmp388;int is_read=_tmp389;
# 1865
e1->r=e->r;
({({struct Cyc_Absyn_Exp*_tmpD8C=e1;struct Cyc_List_List*_tmpD8B=({struct Cyc_List_List*_tmp38D=_cycalloc(sizeof(*_tmp38D));_tmp38D->hd=fld,_tmp38D->tl=fs;_tmp38D;});struct Cyc_Absyn_Exp*(*_tmpD8A)(struct Cyc_Absyn_Exp*,void*)=f;Cyc_Toc_lvalue_assign(_tmpD8C,_tmpD8B,_tmpD8A,f_env);});});
goto _LL0;}default: _LL7: _LL8: {
# 1873
struct Cyc_Absyn_Exp*e1_copy=({Cyc_Absyn_copy_exp(e1);});
# 1875
for(0;fs != 0;fs=fs->tl){
e1_copy=({Cyc_Toc_member_exp(e1_copy,(struct _fat_ptr*)fs->hd,e1_copy->loc);});}
({void*_tmpD8D=({f(e1_copy,f_env);})->r;e1->r=_tmpD8D;});
goto _LL0;}}_LL0:;}
# 1881
static void Cyc_Toc_lvalue_assign_stmt(struct Cyc_Absyn_Stmt*s,struct Cyc_List_List*fs,struct Cyc_Absyn_Exp*(*f)(struct Cyc_Absyn_Exp*,void*),void*f_env){
# 1883
void*_stmttmp27=s->r;void*_tmp38F=_stmttmp27;struct Cyc_Absyn_Stmt*_tmp390;struct Cyc_Absyn_Stmt*_tmp392;struct Cyc_Absyn_Decl*_tmp391;struct Cyc_Absyn_Exp*_tmp393;switch(*((int*)_tmp38F)){case 1U: _LL1: _tmp393=((struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct*)_tmp38F)->f1;_LL2: {struct Cyc_Absyn_Exp*e1=_tmp393;
({Cyc_Toc_lvalue_assign(e1,fs,f,f_env);});goto _LL0;}case 12U: _LL3: _tmp391=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp38F)->f1;_tmp392=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp38F)->f2;_LL4: {struct Cyc_Absyn_Decl*d=_tmp391;struct Cyc_Absyn_Stmt*s2=_tmp392;
# 1886
({Cyc_Toc_lvalue_assign_stmt(s2,fs,f,f_env);});goto _LL0;}case 2U: _LL5: _tmp390=((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_tmp38F)->f2;_LL6: {struct Cyc_Absyn_Stmt*s2=_tmp390;
({Cyc_Toc_lvalue_assign_stmt(s2,fs,f,f_env);});goto _LL0;}default: _LL7: _LL8:
({struct Cyc_String_pa_PrintArg_struct _tmp396=({struct Cyc_String_pa_PrintArg_struct _tmpC7E;_tmpC7E.tag=0U,({struct _fat_ptr _tmpD8E=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_stmt2string(s);}));_tmpC7E.f1=_tmpD8E;});_tmpC7E;});void*_tmp394[1U];_tmp394[0]=& _tmp396;({struct _fat_ptr _tmpD8F=({const char*_tmp395="lvalue_assign_stmt: %s";_tag_fat(_tmp395,sizeof(char),23U);});Cyc_Toc_toc_impos(_tmpD8F,_tag_fat(_tmp394,sizeof(void*),1U));});});}_LL0:;}
# 1881
static void Cyc_Toc_push_address_stmt(struct Cyc_Absyn_Stmt*s);
# 1896
static struct Cyc_Absyn_Exp*Cyc_Toc_push_address_exp(struct Cyc_Absyn_Exp*e){
void*_stmttmp28=e->r;void*_tmp398=_stmttmp28;struct Cyc_Absyn_Stmt*_tmp399;struct Cyc_Absyn_Exp*_tmp39A;struct Cyc_Absyn_Exp**_tmp39C;void**_tmp39B;switch(*((int*)_tmp398)){case 14U: _LL1: _tmp39B=(void**)&((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp398)->f1;_tmp39C=(struct Cyc_Absyn_Exp**)&((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp398)->f2;_LL2: {void**t=_tmp39B;struct Cyc_Absyn_Exp**e1=_tmp39C;
# 1899
({struct Cyc_Absyn_Exp*_tmpD90=({Cyc_Toc_push_address_exp(*e1);});*e1=_tmpD90;});
({void*_tmpD91=({Cyc_Absyn_cstar_type(*t,Cyc_Toc_mt_tq);});*t=_tmpD91;});
return e;}case 20U: _LL3: _tmp39A=((struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_tmp398)->f1;_LL4: {struct Cyc_Absyn_Exp*e1=_tmp39A;
# 1903
return e1;}case 38U: _LL5: _tmp399=((struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct*)_tmp398)->f1;_LL6: {struct Cyc_Absyn_Stmt*s=_tmp399;
# 1907
({Cyc_Toc_push_address_stmt(s);});
return e;}default: _LL7: _LL8:
# 1910
 if(({Cyc_Absyn_is_lvalue(e);}))return({Cyc_Absyn_address_exp(e,0U);});({struct Cyc_String_pa_PrintArg_struct _tmp3A0=({struct Cyc_String_pa_PrintArg_struct _tmpC7F;_tmpC7F.tag=0U,({
struct _fat_ptr _tmpD92=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_exp2string(e);}));_tmpC7F.f1=_tmpD92;});_tmpC7F;});void*_tmp39D[1U];_tmp39D[0]=& _tmp3A0;({int(*_tmpD94)(struct _fat_ptr fmt,struct _fat_ptr ap)=({
# 1910
int(*_tmp39F)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmp39F;});struct _fat_ptr _tmpD93=({const char*_tmp39E="can't take & of exp %s";_tag_fat(_tmp39E,sizeof(char),23U);});_tmpD94(_tmpD93,_tag_fat(_tmp39D,sizeof(void*),1U));});});}_LL0:;}
# 1914
static void Cyc_Toc_push_address_stmt(struct Cyc_Absyn_Stmt*s){
void*_stmttmp29=s->r;void*_tmp3A2=_stmttmp29;struct Cyc_Absyn_Exp**_tmp3A3;struct Cyc_Absyn_Stmt*_tmp3A4;struct Cyc_Absyn_Stmt*_tmp3A5;switch(*((int*)_tmp3A2)){case 2U: _LL1: _tmp3A5=((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_tmp3A2)->f2;_LL2: {struct Cyc_Absyn_Stmt*s2=_tmp3A5;
_tmp3A4=s2;goto _LL4;}case 12U: _LL3: _tmp3A4=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp3A2)->f2;_LL4: {struct Cyc_Absyn_Stmt*s2=_tmp3A4;
({Cyc_Toc_push_address_stmt(s2);});goto _LL0;}case 1U: _LL5: _tmp3A3=(struct Cyc_Absyn_Exp**)&((struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct*)_tmp3A2)->f1;_LL6: {struct Cyc_Absyn_Exp**e=_tmp3A3;
({struct Cyc_Absyn_Exp*_tmpD95=({Cyc_Toc_push_address_exp(*e);});*e=_tmpD95;});goto _LL0;}default: _LL7: _LL8:
({struct Cyc_String_pa_PrintArg_struct _tmp3A9=({struct Cyc_String_pa_PrintArg_struct _tmpC80;_tmpC80.tag=0U,({struct _fat_ptr _tmpD96=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_stmt2string(s);}));_tmpC80.f1=_tmpD96;});_tmpC80;});void*_tmp3A6[1U];_tmp3A6[0]=& _tmp3A9;({int(*_tmpD98)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp3A8)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmp3A8;});struct _fat_ptr _tmpD97=({const char*_tmp3A7="can't take & of stmt %s";_tag_fat(_tmp3A7,sizeof(char),24U);});_tmpD98(_tmpD97,_tag_fat(_tmp3A6,sizeof(void*),1U));});});}_LL0:;}
# 1926
static void Cyc_Toc_zero_ptr_assign_to_c(struct Cyc_Toc_Env*nv,struct Cyc_Absyn_Exp*e,struct Cyc_Absyn_Exp*e1,struct Cyc_Core_Opt*popt,struct Cyc_Absyn_Exp*e2,void*ptr_type,int is_fat,void*elt_type){
# 1938
void*fat_ptr_type=({Cyc_Absyn_fatptr_type(elt_type,Cyc_Absyn_heap_rgn_type,Cyc_Toc_mt_tq,Cyc_Absyn_true_type);});
void*c_elt_type=({Cyc_Toc_typ_to_c(elt_type);});
void*c_fat_ptr_type=({Cyc_Toc_typ_to_c(fat_ptr_type);});
void*c_ptr_type=({Cyc_Absyn_cstar_type(c_elt_type,Cyc_Toc_mt_tq);});
struct Cyc_Core_Opt*c_ptr_type_opt=({struct Cyc_Core_Opt*_tmp3EA=_cycalloc(sizeof(*_tmp3EA));_tmp3EA->v=c_ptr_type;_tmp3EA;});
struct Cyc_Absyn_Exp*xinit;
{void*_stmttmp2A=e1->r;void*_tmp3AB=_stmttmp2A;struct Cyc_Absyn_Exp*_tmp3AD;struct Cyc_Absyn_Exp*_tmp3AC;struct Cyc_Absyn_Exp*_tmp3AE;switch(*((int*)_tmp3AB)){case 20U: _LL1: _tmp3AE=((struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_tmp3AB)->f1;_LL2: {struct Cyc_Absyn_Exp*ea=_tmp3AE;
# 1946
if(!is_fat){
ea=({Cyc_Absyn_cast_exp(fat_ptr_type,ea,0,Cyc_Absyn_Other_coercion,0U);});
ea->topt=fat_ptr_type;
ea->annot=(void*)& Cyc_InsertChecks_NoCheck_val;}
# 1946
({Cyc_Toc_exp_to_c(nv,ea);});
# 1951
xinit=ea;goto _LL0;}case 23U: _LL3: _tmp3AC=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp3AB)->f1;_tmp3AD=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp3AB)->f2;_LL4: {struct Cyc_Absyn_Exp*ea=_tmp3AC;struct Cyc_Absyn_Exp*eb=_tmp3AD;
# 1953
if(!is_fat){
ea=({Cyc_Absyn_cast_exp(fat_ptr_type,ea,0,Cyc_Absyn_Other_coercion,0U);});
ea->topt=fat_ptr_type;
ea->annot=(void*)& Cyc_InsertChecks_NoCheck_val;}
# 1953
({Cyc_Toc_exp_to_c(nv,ea);});
# 1958
({Cyc_Toc_exp_to_c(nv,eb);});
xinit=({struct Cyc_Absyn_Exp*_tmp3AF[3U];_tmp3AF[0]=ea,({
struct Cyc_Absyn_Exp*_tmpD99=({struct Cyc_Absyn_Exp*(*_tmp3B0)(void*t,unsigned)=Cyc_Absyn_sizeoftype_exp;void*_tmp3B1=({Cyc_Toc_typ_to_c(elt_type);});unsigned _tmp3B2=0U;_tmp3B0(_tmp3B1,_tmp3B2);});_tmp3AF[1]=_tmpD99;}),_tmp3AF[2]=eb;({struct Cyc_Absyn_Exp*_tmpD9A=Cyc_Toc__fat_ptr_plus_e;Cyc_Toc_fncall_exp_dl(_tmpD9A,_tag_fat(_tmp3AF,sizeof(struct Cyc_Absyn_Exp*),3U));});});
goto _LL0;}default: _LL5: _LL6:
({void*_tmp3B3=0U;({int(*_tmpD9C)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp3B5)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmp3B5;});struct _fat_ptr _tmpD9B=({const char*_tmp3B4="found bad lhs for zero-terminated pointer assignment";_tag_fat(_tmp3B4,sizeof(char),53U);});_tmpD9C(_tmpD9B,_tag_fat(_tmp3B3,sizeof(void*),0U));});});}_LL0:;}{
# 1964
struct _tuple1*x=({Cyc_Toc_temp_var();});
struct _RegionHandle _tmp3B6=_new_region("rgn2");struct _RegionHandle*rgn2=& _tmp3B6;_push_region(rgn2);
{struct Cyc_Absyn_Vardecl*x_vd=({struct Cyc_Absyn_Vardecl*_tmp3E9=_cycalloc(sizeof(*_tmp3E9));_tmp3E9->sc=Cyc_Absyn_Public,_tmp3E9->name=x,_tmp3E9->varloc=0U,_tmp3E9->tq=Cyc_Toc_mt_tq,_tmp3E9->type=c_fat_ptr_type,_tmp3E9->initializer=xinit,_tmp3E9->rgn=0,_tmp3E9->attributes=0,_tmp3E9->escapes=0;_tmp3E9;});
struct Cyc_Absyn_Local_b_Absyn_Binding_struct*x_bnd=({struct Cyc_Absyn_Local_b_Absyn_Binding_struct*_tmp3E8=_cycalloc(sizeof(*_tmp3E8));_tmp3E8->tag=4U,_tmp3E8->f1=x_vd;_tmp3E8;});
struct Cyc_Absyn_Exp*x_exp=({Cyc_Absyn_varb_exp((void*)x_bnd,0U);});
x_exp->topt=fat_ptr_type;{
struct Cyc_Absyn_Exp*deref_x=({Cyc_Absyn_deref_exp(x_exp,0U);});
deref_x->topt=elt_type;
deref_x->annot=(void*)& Cyc_InsertChecks_NullAndFatBound_val;
({Cyc_Toc_exp_to_c(nv,deref_x);});{
struct _tuple1*y=({Cyc_Toc_temp_var();});
struct Cyc_Absyn_Vardecl*y_vd=({struct Cyc_Absyn_Vardecl*_tmp3E7=_cycalloc(sizeof(*_tmp3E7));_tmp3E7->sc=Cyc_Absyn_Public,_tmp3E7->name=y,_tmp3E7->varloc=0U,_tmp3E7->tq=Cyc_Toc_mt_tq,_tmp3E7->type=c_elt_type,_tmp3E7->initializer=deref_x,_tmp3E7->rgn=0,_tmp3E7->attributes=0,_tmp3E7->escapes=0;_tmp3E7;});
struct Cyc_Absyn_Local_b_Absyn_Binding_struct*y_bnd=({struct Cyc_Absyn_Local_b_Absyn_Binding_struct*_tmp3E6=_cycalloc(sizeof(*_tmp3E6));_tmp3E6->tag=4U,_tmp3E6->f1=y_vd;_tmp3E6;});
struct Cyc_Absyn_Exp*z_init=e2;
if(popt != 0){
struct Cyc_Absyn_Exp*y_exp=({Cyc_Absyn_varb_exp((void*)y_bnd,0U);});
y_exp->topt=deref_x->topt;
z_init=({struct Cyc_Absyn_Exp*(*_tmp3B7)(enum Cyc_Absyn_Primop,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_prim2_exp;enum Cyc_Absyn_Primop _tmp3B8=(enum Cyc_Absyn_Primop)popt->v;struct Cyc_Absyn_Exp*_tmp3B9=y_exp;struct Cyc_Absyn_Exp*_tmp3BA=({Cyc_Absyn_copy_exp(e2);});unsigned _tmp3BB=0U;_tmp3B7(_tmp3B8,_tmp3B9,_tmp3BA,_tmp3BB);});
z_init->topt=y_exp->topt;
z_init->annot=(void*)& Cyc_InsertChecks_NoCheck_val;}
# 1978
({Cyc_Toc_exp_to_c(nv,z_init);});{
# 1986
struct _tuple1*z=({Cyc_Toc_temp_var();});
struct Cyc_Absyn_Vardecl*z_vd=({struct Cyc_Absyn_Vardecl*_tmp3E5=_cycalloc(sizeof(*_tmp3E5));_tmp3E5->sc=Cyc_Absyn_Public,_tmp3E5->name=z,_tmp3E5->varloc=0U,_tmp3E5->tq=Cyc_Toc_mt_tq,_tmp3E5->type=c_elt_type,_tmp3E5->initializer=z_init,_tmp3E5->rgn=0,_tmp3E5->attributes=0,_tmp3E5->escapes=0;_tmp3E5;});
struct Cyc_Absyn_Local_b_Absyn_Binding_struct*z_bnd=({struct Cyc_Absyn_Local_b_Absyn_Binding_struct*_tmp3E4=_cycalloc(sizeof(*_tmp3E4));_tmp3E4->tag=4U,_tmp3E4->f1=z_vd;_tmp3E4;});
# 1991
struct Cyc_Absyn_Exp*y2_exp=({Cyc_Absyn_varb_exp((void*)y_bnd,0U);});y2_exp->topt=deref_x->topt;{
struct Cyc_Absyn_Exp*zero1_exp=({Cyc_Absyn_signed_int_exp(0,0U);});
struct Cyc_Absyn_Exp*comp1_exp=({Cyc_Absyn_prim2_exp(Cyc_Absyn_Eq,y2_exp,zero1_exp,0U);});
zero1_exp->topt=Cyc_Absyn_sint_type;
comp1_exp->topt=Cyc_Absyn_sint_type;
({Cyc_Toc_exp_to_c(nv,comp1_exp);});{
# 1998
struct Cyc_Absyn_Exp*z_exp=({Cyc_Absyn_varb_exp((void*)z_bnd,0U);});z_exp->topt=deref_x->topt;{
struct Cyc_Absyn_Exp*zero2_exp=({Cyc_Absyn_signed_int_exp(0,0U);});
struct Cyc_Absyn_Exp*comp2_exp=({Cyc_Absyn_prim2_exp(Cyc_Absyn_Neq,z_exp,zero2_exp,0U);});
zero2_exp->topt=Cyc_Absyn_sint_type;
comp2_exp->topt=Cyc_Absyn_sint_type;
({Cyc_Toc_exp_to_c(nv,comp2_exp);});{
# 2005
struct Cyc_List_List*xsizeargs=({struct Cyc_Absyn_Exp*_tmp3E0[2U];({struct Cyc_Absyn_Exp*_tmpD9E=({Cyc_Absyn_varb_exp((void*)x_bnd,0U);});_tmp3E0[0]=_tmpD9E;}),({
struct Cyc_Absyn_Exp*_tmpD9D=({struct Cyc_Absyn_Exp*(*_tmp3E1)(void*t,unsigned)=Cyc_Absyn_sizeoftype_exp;void*_tmp3E2=({Cyc_Toc_typ_to_c(elt_type);});unsigned _tmp3E3=0U;_tmp3E1(_tmp3E2,_tmp3E3);});_tmp3E0[1]=_tmpD9D;});Cyc_List_list(_tag_fat(_tmp3E0,sizeof(struct Cyc_Absyn_Exp*),2U));});
struct Cyc_Absyn_Exp*oneexp=({Cyc_Absyn_uint_exp(1U,0U);});
struct Cyc_Absyn_Exp*xsize;
xsize=({struct Cyc_Absyn_Exp*(*_tmp3BC)(enum Cyc_Absyn_Primop,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_prim2_exp;enum Cyc_Absyn_Primop _tmp3BD=Cyc_Absyn_Eq;struct Cyc_Absyn_Exp*_tmp3BE=({Cyc_Absyn_fncall_exp(Cyc_Toc__get_fat_size_e,xsizeargs,0U);});struct Cyc_Absyn_Exp*_tmp3BF=oneexp;unsigned _tmp3C0=0U;_tmp3BC(_tmp3BD,_tmp3BE,_tmp3BF,_tmp3C0);});{
# 2012
struct Cyc_Absyn_Exp*comp_exp=({struct Cyc_Absyn_Exp*(*_tmp3DC)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_and_exp;struct Cyc_Absyn_Exp*_tmp3DD=xsize;struct Cyc_Absyn_Exp*_tmp3DE=({Cyc_Absyn_and_exp(comp1_exp,comp2_exp,0U);});unsigned _tmp3DF=0U;_tmp3DC(_tmp3DD,_tmp3DE,_tmp3DF);});
struct Cyc_Absyn_Stmt*thr_stmt=({struct Cyc_Absyn_Stmt*(*_tmp3D8)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_exp_stmt;struct Cyc_Absyn_Exp*_tmp3D9=({void*_tmp3DA=0U;({struct Cyc_Absyn_Exp*_tmpD9F=Cyc_Toc__throw_arraybounds_e;Cyc_Toc_fncall_exp_dl(_tmpD9F,_tag_fat(_tmp3DA,sizeof(struct Cyc_Absyn_Exp*),0U));});});unsigned _tmp3DB=0U;_tmp3D8(_tmp3D9,_tmp3DB);});
struct Cyc_Absyn_Exp*xcurr=({struct Cyc_Absyn_Exp*(*_tmp3D4)(struct Cyc_Absyn_Exp*e,struct _fat_ptr*f,unsigned loc)=Cyc_Toc_member_exp;struct Cyc_Absyn_Exp*_tmp3D5=({Cyc_Absyn_varb_exp((void*)x_bnd,0U);});struct _fat_ptr*_tmp3D6=Cyc_Toc_curr_sp;unsigned _tmp3D7=0U;_tmp3D4(_tmp3D5,_tmp3D6,_tmp3D7);});
xcurr=({Cyc_Toc_cast_it(c_ptr_type,xcurr);});{
struct Cyc_Absyn_Exp*deref_xcurr=({Cyc_Absyn_deref_exp(xcurr,0U);});
struct Cyc_Absyn_Exp*asn_exp=({struct Cyc_Absyn_Exp*(*_tmp3D0)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_assign_exp;struct Cyc_Absyn_Exp*_tmp3D1=deref_xcurr;struct Cyc_Absyn_Exp*_tmp3D2=({Cyc_Absyn_var_exp(z,0U);});unsigned _tmp3D3=0U;_tmp3D0(_tmp3D1,_tmp3D2,_tmp3D3);});
struct Cyc_Absyn_Stmt*s=({Cyc_Absyn_exp_stmt(asn_exp,0U);});
s=({struct Cyc_Absyn_Stmt*(*_tmp3C1)(struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_seq_stmt;struct Cyc_Absyn_Stmt*_tmp3C2=({struct Cyc_Absyn_Stmt*(*_tmp3C3)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_ifthenelse_stmt;struct Cyc_Absyn_Exp*_tmp3C4=comp_exp;struct Cyc_Absyn_Stmt*_tmp3C5=thr_stmt;struct Cyc_Absyn_Stmt*_tmp3C6=({Cyc_Absyn_skip_stmt(0U);});unsigned _tmp3C7=0U;_tmp3C3(_tmp3C4,_tmp3C5,_tmp3C6,_tmp3C7);});struct Cyc_Absyn_Stmt*_tmp3C8=s;unsigned _tmp3C9=0U;_tmp3C1(_tmp3C2,_tmp3C8,_tmp3C9);});
s=({({struct Cyc_Absyn_Decl*_tmpDA1=({struct Cyc_Absyn_Decl*_tmp3CB=_cycalloc(sizeof(*_tmp3CB));({void*_tmpDA0=(void*)({struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*_tmp3CA=_cycalloc(sizeof(*_tmp3CA));_tmp3CA->tag=0U,_tmp3CA->f1=z_vd;_tmp3CA;});_tmp3CB->r=_tmpDA0;}),_tmp3CB->loc=0U;_tmp3CB;});Cyc_Absyn_decl_stmt(_tmpDA1,s,0U);});});
s=({({struct Cyc_Absyn_Decl*_tmpDA3=({struct Cyc_Absyn_Decl*_tmp3CD=_cycalloc(sizeof(*_tmp3CD));({void*_tmpDA2=(void*)({struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*_tmp3CC=_cycalloc(sizeof(*_tmp3CC));_tmp3CC->tag=0U,_tmp3CC->f1=y_vd;_tmp3CC;});_tmp3CD->r=_tmpDA2;}),_tmp3CD->loc=0U;_tmp3CD;});Cyc_Absyn_decl_stmt(_tmpDA3,s,0U);});});
s=({({struct Cyc_Absyn_Decl*_tmpDA5=({struct Cyc_Absyn_Decl*_tmp3CF=_cycalloc(sizeof(*_tmp3CF));({void*_tmpDA4=(void*)({struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*_tmp3CE=_cycalloc(sizeof(*_tmp3CE));_tmp3CE->tag=0U,_tmp3CE->f1=x_vd;_tmp3CE;});_tmp3CF->r=_tmpDA4;}),_tmp3CF->loc=0U;_tmp3CF;});Cyc_Absyn_decl_stmt(_tmpDA5,s,0U);});});
({void*_tmpDA6=({Cyc_Toc_stmt_exp_r(s);});e->r=_tmpDA6;});}}}}}}}}}}
# 1966
;_pop_region();}}
# 2038 "toc.cyc"
static void*Cyc_Toc_check_tagged_union(struct Cyc_Absyn_Exp*e1,void*e1_c_type,void*aggrtype,struct _fat_ptr*f,int in_lhs,struct Cyc_Absyn_Exp*(*aggrproj)(struct Cyc_Absyn_Exp*,struct _fat_ptr*,unsigned)){
# 2042
struct Cyc_Absyn_Aggrdecl*ad;
{void*_stmttmp2B=({Cyc_Tcutil_compress(aggrtype);});void*_tmp3EC=_stmttmp2B;union Cyc_Absyn_AggrInfo _tmp3ED;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp3EC)->tag == 0U){if(((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp3EC)->f1)->tag == 22U){_LL1: _tmp3ED=((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp3EC)->f1)->f1;_LL2: {union Cyc_Absyn_AggrInfo info=_tmp3ED;
ad=({Cyc_Absyn_get_known_aggrdecl(info);});goto _LL0;}}else{goto _LL3;}}else{_LL3: _LL4:
({void*_tmp3EE=0U;({int(*_tmpDAA)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp3F2)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp3F2;});struct _fat_ptr _tmpDA9=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp3F1=({struct Cyc_String_pa_PrintArg_struct _tmpC81;_tmpC81.tag=0U,({
struct _fat_ptr _tmpDA7=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(aggrtype);}));_tmpC81.f1=_tmpDA7;});_tmpC81;});void*_tmp3EF[1U];_tmp3EF[0]=& _tmp3F1;({struct _fat_ptr _tmpDA8=({const char*_tmp3F0="expecting union but found %s in check_tagged_union";_tag_fat(_tmp3F0,sizeof(char),51U);});Cyc_aprintf(_tmpDA8,_tag_fat(_tmp3EF,sizeof(void*),1U));});});_tmpDAA(_tmpDA9,_tag_fat(_tmp3EE,sizeof(void*),0U));});});}_LL0:;}{
# 2048
struct _tuple33 _stmttmp2C=({Cyc_Toc_temp_var_and_exp();});struct Cyc_Absyn_Exp*_tmp3F4;struct _tuple1*_tmp3F3;_LL6: _tmp3F3=_stmttmp2C.f1;_tmp3F4=_stmttmp2C.f2;_LL7: {struct _tuple1*temp=_tmp3F3;struct Cyc_Absyn_Exp*temp_exp=_tmp3F4;
struct Cyc_Absyn_Exp*f_tag=({struct Cyc_Absyn_Exp*(*_tmp419)(int,unsigned)=Cyc_Absyn_signed_int_exp;int _tmp41A=({Cyc_Toc_get_member_offset(ad,f);});unsigned _tmp41B=0U;_tmp419(_tmp41A,_tmp41B);});
if(in_lhs){
struct Cyc_Absyn_Exp*temp_f_tag=({Cyc_Absyn_aggrarrow_exp(temp_exp,Cyc_Toc_tag_sp,0U);});
struct Cyc_Absyn_Exp*test_exp=({Cyc_Absyn_neq_exp(temp_f_tag,f_tag,0U);});
struct Cyc_Absyn_Exp*temp_f_val=({Cyc_Absyn_aggrarrow_exp(temp_exp,Cyc_Toc_val_sp,0U);});
struct Cyc_Absyn_Stmt*sres=({struct Cyc_Absyn_Stmt*(*_tmp403)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_exp_stmt;struct Cyc_Absyn_Exp*_tmp404=({Cyc_Absyn_address_exp(temp_f_val,0U);});unsigned _tmp405=0U;_tmp403(_tmp404,_tmp405);});
struct Cyc_Absyn_Stmt*ifs=({struct Cyc_Absyn_Stmt*(*_tmp3FE)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_ifthenelse_stmt;struct Cyc_Absyn_Exp*_tmp3FF=test_exp;struct Cyc_Absyn_Stmt*_tmp400=({Cyc_Toc_throw_match_stmt();});struct Cyc_Absyn_Stmt*_tmp401=({Cyc_Toc_skip_stmt_dl();});unsigned _tmp402=0U;_tmp3FE(_tmp3FF,_tmp400,_tmp401,_tmp402);});
void*e1_p_type=({Cyc_Absyn_cstar_type(e1_c_type,Cyc_Toc_mt_tq);});
struct Cyc_Absyn_Exp*e1_f=({struct Cyc_Absyn_Exp*(*_tmp3FB)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_address_exp;struct Cyc_Absyn_Exp*_tmp3FC=({aggrproj(e1,f,0U);});unsigned _tmp3FD=0U;_tmp3FB(_tmp3FC,_tmp3FD);});
struct Cyc_Absyn_Stmt*s=({struct Cyc_Absyn_Stmt*(*_tmp3F5)(struct _tuple1*,void*,struct Cyc_Absyn_Exp*init,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_declare_stmt;struct _tuple1*_tmp3F6=temp;void*_tmp3F7=e1_p_type;struct Cyc_Absyn_Exp*_tmp3F8=e1_f;struct Cyc_Absyn_Stmt*_tmp3F9=({Cyc_Absyn_seq_stmt(ifs,sres,0U);});unsigned _tmp3FA=0U;_tmp3F5(_tmp3F6,_tmp3F7,_tmp3F8,_tmp3F9,_tmp3FA);});
return({Cyc_Toc_stmt_exp_r(s);});}else{
# 2061
struct Cyc_Absyn_Exp*temp_f_tag=({struct Cyc_Absyn_Exp*(*_tmp415)(struct Cyc_Absyn_Exp*e,struct _fat_ptr*f,unsigned loc)=Cyc_Toc_member_exp;struct Cyc_Absyn_Exp*_tmp416=({aggrproj(temp_exp,f,0U);});struct _fat_ptr*_tmp417=Cyc_Toc_tag_sp;unsigned _tmp418=0U;_tmp415(_tmp416,_tmp417,_tmp418);});
struct Cyc_Absyn_Exp*test_exp=({Cyc_Absyn_neq_exp(temp_f_tag,f_tag,0U);});
struct Cyc_Absyn_Exp*temp_f_val=({struct Cyc_Absyn_Exp*(*_tmp411)(struct Cyc_Absyn_Exp*e,struct _fat_ptr*f,unsigned loc)=Cyc_Toc_member_exp;struct Cyc_Absyn_Exp*_tmp412=({aggrproj(temp_exp,f,0U);});struct _fat_ptr*_tmp413=Cyc_Toc_val_sp;unsigned _tmp414=0U;_tmp411(_tmp412,_tmp413,_tmp414);});
struct Cyc_Absyn_Stmt*sres=({Cyc_Absyn_exp_stmt(temp_f_val,0U);});
struct Cyc_Absyn_Stmt*ifs=({struct Cyc_Absyn_Stmt*(*_tmp40C)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_ifthenelse_stmt;struct Cyc_Absyn_Exp*_tmp40D=test_exp;struct Cyc_Absyn_Stmt*_tmp40E=({Cyc_Toc_throw_match_stmt();});struct Cyc_Absyn_Stmt*_tmp40F=({Cyc_Toc_skip_stmt_dl();});unsigned _tmp410=0U;_tmp40C(_tmp40D,_tmp40E,_tmp40F,_tmp410);});
struct Cyc_Absyn_Stmt*s=({struct Cyc_Absyn_Stmt*(*_tmp406)(struct _tuple1*,void*,struct Cyc_Absyn_Exp*init,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_declare_stmt;struct _tuple1*_tmp407=temp;void*_tmp408=e1_c_type;struct Cyc_Absyn_Exp*_tmp409=e1;struct Cyc_Absyn_Stmt*_tmp40A=({Cyc_Absyn_seq_stmt(ifs,sres,0U);});unsigned _tmp40B=0U;_tmp406(_tmp407,_tmp408,_tmp409,_tmp40A,_tmp40B);});
return({Cyc_Toc_stmt_exp_r(s);});}}}}
# 2071
static int Cyc_Toc_is_tagged_union_project_impl(void*t,struct _fat_ptr*f,int*f_tag,void**tagged_member_type,int clear_read,int*is_read){
# 2074
void*_stmttmp2D=({Cyc_Tcutil_compress(t);});void*_tmp41D=_stmttmp2D;union Cyc_Absyn_AggrInfo _tmp41E;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp41D)->tag == 0U){if(((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp41D)->f1)->tag == 22U){_LL1: _tmp41E=((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp41D)->f1)->f1;_LL2: {union Cyc_Absyn_AggrInfo info=_tmp41E;
# 2076
struct Cyc_Absyn_Aggrdecl*ad=({Cyc_Absyn_get_known_aggrdecl(info);});
({int _tmpDAB=({Cyc_Toc_get_member_offset(ad,f);});*f_tag=_tmpDAB;});{
# 2079
struct _fat_ptr str=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp422=({struct Cyc_String_pa_PrintArg_struct _tmpC83;_tmpC83.tag=0U,_tmpC83.f1=(struct _fat_ptr)((struct _fat_ptr)*(*ad->name).f2);_tmpC83;});struct Cyc_String_pa_PrintArg_struct _tmp423=({struct Cyc_String_pa_PrintArg_struct _tmpC82;_tmpC82.tag=0U,_tmpC82.f1=(struct _fat_ptr)((struct _fat_ptr)*f);_tmpC82;});void*_tmp420[2U];_tmp420[0]=& _tmp422,_tmp420[1]=& _tmp423;({struct _fat_ptr _tmpDAC=({const char*_tmp421="_union_%s_%s";_tag_fat(_tmp421,sizeof(char),13U);});Cyc_aprintf(_tmpDAC,_tag_fat(_tmp420,sizeof(void*),2U));});});
({void*_tmpDAD=({Cyc_Absyn_strct(({struct _fat_ptr*_tmp41F=_cycalloc(sizeof(*_tmp41F));*_tmp41F=str;_tmp41F;}));});*tagged_member_type=_tmpDAD;});
if(clear_read)*((int*)_check_null(is_read))=0;return((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->tagged;}}}else{goto _LL3;}}else{_LL3: _LL4:
# 2083
 return 0;}_LL0:;}
# 2071
int Cyc_Toc_is_tagged_union_project(struct Cyc_Absyn_Exp*e,int*f_tag,void**tagged_member_type,int clear_read){
# 2093
void*_stmttmp2E=e->r;void*_tmp425=_stmttmp2E;int*_tmp428;struct _fat_ptr*_tmp427;struct Cyc_Absyn_Exp*_tmp426;int*_tmp42B;struct _fat_ptr*_tmp42A;struct Cyc_Absyn_Exp*_tmp429;struct Cyc_Absyn_Exp*_tmp42C;switch(*((int*)_tmp425)){case 14U: _LL1: _tmp42C=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp425)->f2;_LL2: {struct Cyc_Absyn_Exp*e1=_tmp42C;
({void*_tmp42D=0U;({int(*_tmpDAF)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp42F)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmp42F;});struct _fat_ptr _tmpDAE=({const char*_tmp42E="cast on lhs!";_tag_fat(_tmp42E,sizeof(char),13U);});_tmpDAF(_tmpDAE,_tag_fat(_tmp42D,sizeof(void*),0U));});});}case 21U: _LL3: _tmp429=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp425)->f1;_tmp42A=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp425)->f2;_tmp42B=(int*)&((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp425)->f4;_LL4: {struct Cyc_Absyn_Exp*e1=_tmp429;struct _fat_ptr*f=_tmp42A;int*is_read=_tmp42B;
# 2096
return({Cyc_Toc_is_tagged_union_project_impl((void*)_check_null(e1->topt),f,f_tag,tagged_member_type,clear_read,is_read);});}case 22U: _LL5: _tmp426=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp425)->f1;_tmp427=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp425)->f2;_tmp428=(int*)&((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp425)->f4;_LL6: {struct Cyc_Absyn_Exp*e1=_tmp426;struct _fat_ptr*f=_tmp427;int*is_read=_tmp428;
# 2099
void*_stmttmp2F=({Cyc_Tcutil_compress((void*)_check_null(e1->topt));});void*_tmp430=_stmttmp2F;void*_tmp431;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp430)->tag == 3U){_LLA: _tmp431=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp430)->f1).elt_type;_LLB: {void*et=_tmp431;
# 2101
return({Cyc_Toc_is_tagged_union_project_impl(et,f,f_tag,tagged_member_type,clear_read,is_read);});}}else{_LLC: _LLD:
# 2103
 return 0;}_LL9:;}default: _LL7: _LL8:
# 2105
 return 0;}_LL0:;}
# 2118 "toc.cyc"
static void*Cyc_Toc_tagged_union_assignop(struct Cyc_Absyn_Exp*e1,void*e1_cyc_type,struct Cyc_Core_Opt*popt,struct Cyc_Absyn_Exp*e2,void*e2_cyc_type,int tag_num,void*member_type){
# 2122
struct _tuple33 _stmttmp30=({Cyc_Toc_temp_var_and_exp();});struct Cyc_Absyn_Exp*_tmp434;struct _tuple1*_tmp433;_LL1: _tmp433=_stmttmp30.f1;_tmp434=_stmttmp30.f2;_LL2: {struct _tuple1*temp=_tmp433;struct Cyc_Absyn_Exp*temp_exp=_tmp434;
struct Cyc_Absyn_Exp*temp_val=({Cyc_Absyn_aggrarrow_exp(temp_exp,Cyc_Toc_val_sp,0U);});
struct Cyc_Absyn_Exp*temp_tag=({Cyc_Absyn_aggrarrow_exp(temp_exp,Cyc_Toc_tag_sp,0U);});
struct Cyc_Absyn_Exp*f_tag=({Cyc_Absyn_signed_int_exp(tag_num,0U);});
struct Cyc_Absyn_Stmt*s3=({struct Cyc_Absyn_Stmt*(*_tmp443)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_exp_stmt;struct Cyc_Absyn_Exp*_tmp444=({Cyc_Absyn_assignop_exp(temp_val,popt,e2,0U);});unsigned _tmp445=0U;_tmp443(_tmp444,_tmp445);});
struct Cyc_Absyn_Stmt*s2;
if(popt == 0)
s2=({struct Cyc_Absyn_Stmt*(*_tmp435)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_exp_stmt;struct Cyc_Absyn_Exp*_tmp436=({Cyc_Absyn_assign_exp(temp_tag,f_tag,0U);});unsigned _tmp437=0U;_tmp435(_tmp436,_tmp437);});else{
# 2131
struct Cyc_Absyn_Exp*test_exp=({Cyc_Absyn_neq_exp(temp_tag,f_tag,0U);});
s2=({struct Cyc_Absyn_Stmt*(*_tmp438)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_ifthenelse_stmt;struct Cyc_Absyn_Exp*_tmp439=test_exp;struct Cyc_Absyn_Stmt*_tmp43A=({Cyc_Toc_throw_match_stmt();});struct Cyc_Absyn_Stmt*_tmp43B=({Cyc_Toc_skip_stmt_dl();});unsigned _tmp43C=0U;_tmp438(_tmp439,_tmp43A,_tmp43B,_tmp43C);});}{
# 2134
struct Cyc_Absyn_Stmt*s1=({struct Cyc_Absyn_Stmt*(*_tmp43D)(struct _tuple1*,void*,struct Cyc_Absyn_Exp*init,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_declare_stmt;struct _tuple1*_tmp43E=temp;void*_tmp43F=({Cyc_Absyn_cstar_type(member_type,Cyc_Toc_mt_tq);});struct Cyc_Absyn_Exp*_tmp440=({Cyc_Toc_push_address_exp(e1);});struct Cyc_Absyn_Stmt*_tmp441=({Cyc_Absyn_seq_stmt(s2,s3,0U);});unsigned _tmp442=0U;_tmp43D(_tmp43E,_tmp43F,_tmp440,_tmp441,_tmp442);});
# 2137
return({Cyc_Toc_stmt_exp_r(s1);});}}}
# 2141
inline static int Cyc_Toc_should_insert_xeffect(struct Cyc_List_List*in,struct Cyc_List_List*out){
# 2143
return in != 0 &&({Cyc_List_length(in);})> 1;}struct _tuple36{struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Exp*f2;int f3;struct Cyc_Absyn_Exp*f4;struct Cyc_Absyn_Vardecl*f5;struct Cyc_Absyn_Vardecl*f6;};
# 2167 "toc.cyc"
static struct _tuple36 Cyc_Toc_genEffectFrameDecl(struct Cyc_List_List*formal_f,struct Cyc_List_List*update_f){
# 2184 "toc.cyc"
struct _tuple22 _stmttmp31=({Cyc_Toc_void_star_array_init(formal_f);});unsigned _tmp449;struct Cyc_Absyn_Exp*_tmp448;_LL1: _tmp448=_stmttmp31.f1;_tmp449=_stmttmp31.f2;_LL2: {struct Cyc_Absyn_Exp*init0=_tmp448;unsigned formal_handles_sz=_tmp449;
if(formal_handles_sz > (unsigned)255)
({struct Cyc_Int_pa_PrintArg_struct _tmp44D=({struct Cyc_Int_pa_PrintArg_struct _tmpC84;_tmpC84.tag=1U,_tmpC84.f1=(unsigned long)((int)formal_handles_sz);_tmpC84;});void*_tmp44A[1U];_tmp44A[0]=& _tmp44D;({int(*_tmpDB1)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp44C)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmp44C;});struct _fat_ptr _tmpDB0=({const char*_tmp44B="could not generate code for function call. Too many formal handles found: %d";_tag_fat(_tmp44B,sizeof(char),77U);});_tmpDB1(_tmpDB0,_tag_fat(_tmp44A,sizeof(void*),1U));});});{
# 2185
struct _tuple1*formal_handles_var=({Cyc_Toc_temp_var();});
# 2189
struct Cyc_Absyn_Exp*handles_var_e=({Cyc_Absyn_var_exp(formal_handles_var,0U);});
struct Cyc_Absyn_Vardecl*formal_handles_var_decl=({Cyc_Absyn_new_vardecl(0U,formal_handles_var,(void*)_check_null(init0->topt),init0);});
# 2193
struct _tuple22 _stmttmp32=({Cyc_Toc_effect_frame_caps_array_init(update_f);});unsigned _tmp44F;struct Cyc_Absyn_Exp*_tmp44E;_LL4: _tmp44E=_stmttmp32.f1;_tmp44F=_stmttmp32.f2;_LL5: {struct Cyc_Absyn_Exp*update_handles_init_e=_tmp44E;unsigned update_handles_sz=_tmp44F;
# 2195
struct _tuple1*update_handles_var=({Cyc_Toc_temp_var();});
struct Cyc_Absyn_Exp*update_handles_var_e=({Cyc_Absyn_var_exp(update_handles_var,0U);});
# 2198
struct Cyc_Absyn_Vardecl*update_handles_var_decl=({Cyc_Absyn_new_vardecl(0U,update_handles_var,(void*)_check_null(update_handles_init_e->topt),update_handles_init_e);});
# 2203
struct _tuple1*effVar=({Cyc_Toc_temp_var();});
struct Cyc_Absyn_Exp*addr=({struct Cyc_Absyn_Exp*(*_tmp45B)(void*t,struct Cyc_Absyn_Exp*e)=Cyc_Toc_cast_it;void*_tmp45C=({void*(*_tmp45D)(void*,struct Cyc_Absyn_Tqual)=Cyc_Absyn_cstar_type;void*_tmp45E=({Cyc_Toc_effect_frame_type();});struct Cyc_Absyn_Tqual _tmp45F=Cyc_Toc_mt_tq;_tmp45D(_tmp45E,_tmp45F);});struct Cyc_Absyn_Exp*_tmp460=({struct Cyc_Absyn_Exp*(*_tmp461)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_address_exp;struct Cyc_Absyn_Exp*_tmp462=({Cyc_Absyn_var_exp(effVar,0U);});unsigned _tmp463=0U;_tmp461(_tmp462,_tmp463);});_tmp45B(_tmp45C,_tmp460);});
# 2206
struct Cyc_Absyn_Vardecl*effVarDecl=({struct Cyc_Absyn_Vardecl*(*_tmp456)(unsigned varloc,struct _tuple1*,void*,struct Cyc_Absyn_Exp*init)=Cyc_Absyn_new_vardecl;unsigned _tmp457=0U;struct _tuple1*_tmp458=effVar;void*_tmp459=({Cyc_Toc_effect_frame_type();});struct Cyc_Absyn_Exp*_tmp45A=({Cyc_Toc_effect_frame_init(formal_handles_var,0,(int)formal_handles_sz);});_tmp456(_tmp457,_tmp458,_tmp459,_tmp45A);});
# 2209
if(update_handles_sz == (unsigned)0)
update_handles_var_e=({struct Cyc_Absyn_Exp*(*_tmp450)(void*t,struct Cyc_Absyn_Exp*e)=Cyc_Toc_cast_it;void*_tmp451=({void*(*_tmp452)(void*,struct Cyc_Absyn_Tqual)=Cyc_Absyn_cstar_type;void*_tmp453=({Cyc_Toc_effect_frame_caps_type();});struct Cyc_Absyn_Tqual _tmp454=Cyc_Toc_mt_tq;_tmp452(_tmp453,_tmp454);});struct Cyc_Absyn_Exp*_tmp455=({Cyc_Absyn_uint_exp(0U,0U);});_tmp450(_tmp451,_tmp455);});
# 2209
return({struct _tuple36 _tmpC85;_tmpC85.f1=effVarDecl,_tmpC85.f2=update_handles_var_e,_tmpC85.f3=(int)update_handles_sz,_tmpC85.f4=addr,_tmpC85.f5=update_handles_var_decl,_tmpC85.f6=formal_handles_var_decl;_tmpC85;});}}}}struct _tuple37{struct Cyc_Absyn_Exp*f1;int f2;};struct _tuple38{struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;struct _tuple37*f3;struct Cyc_Absyn_Exp*f4;};
# 2217
static struct Cyc_Absyn_Stmt*Cyc_Toc_genPush(struct Cyc_Absyn_Exp*update_handles_var_e,int update_handles_sz,struct Cyc_Absyn_Exp*addr,struct Cyc_Absyn_Vardecl*update_handles_var_decl,struct _tuple38*thread_args){
# 2225
struct Cyc_Absyn_Stmt*push;
if(thread_args != 0){
struct Cyc_Absyn_Exp*_tmp469;int _tmp468;struct Cyc_Absyn_Exp*_tmp467;struct Cyc_Absyn_Exp*_tmp466;struct Cyc_Absyn_Exp*_tmp465;if(thread_args != 0){if(((struct _tuple38*)thread_args)->f2 != 0){_LL1: _tmp465=thread_args->f1;_tmp466=(struct Cyc_Absyn_Exp*)(thread_args->f2)->hd;_tmp467=(thread_args->f3)->f1;_tmp468=(thread_args->f3)->f2;_tmp469=thread_args->f4;_LL2: {struct Cyc_Absyn_Exp*e1=_tmp465;struct Cyc_Absyn_Exp*e2=_tmp466;struct Cyc_Absyn_Exp*offset_init_e=_tmp467;int offset_sz=_tmp468;struct Cyc_Absyn_Exp*stack_size_e=_tmp469;
# 2229
struct Cyc_Absyn_FnType_Absyn_Type_struct*ftyp=({struct Cyc_Absyn_FnType_Absyn_Type_struct*_tmp47B=_cycalloc(sizeof(*_tmp47B));_tmp47B->tag=5U,(_tmp47B->f1).tvars=0,(_tmp47B->f1).effect=0,(_tmp47B->f1).ret_tqual=Cyc_Toc_mt_tq,(_tmp47B->f1).ret_type=Cyc_Absyn_void_type,({struct Cyc_List_List*_tmpDB4=({struct Cyc_List_List*_tmp47A=_cycalloc(sizeof(*_tmp47A));({
struct _tuple9*_tmpDB3=({struct _tuple9*_tmp479=_cycalloc(sizeof(*_tmp479));_tmp479->f1=0,_tmp479->f2=Cyc_Toc_mt_tq,({void*_tmpDB2=({Cyc_Toc_void_star_type();});_tmp479->f3=_tmpDB2;});_tmp479;});_tmp47A->hd=_tmpDB3;}),_tmp47A->tl=0;_tmp47A;});
# 2229
(_tmp47B->f1).args=_tmpDB4;}),(_tmp47B->f1).c_varargs=0,(_tmp47B->f1).cyc_varargs=0,(_tmp47B->f1).rgn_po=0,(_tmp47B->f1).attributes=0,(_tmp47B->f1).requires_clause=0,(_tmp47B->f1).requires_relns=0,(_tmp47B->f1).ensures_clause=0,(_tmp47B->f1).ensures_relns=0,(_tmp47B->f1).ieffect=0,(_tmp47B->f1).oeffect=0,(_tmp47B->f1).throws=0,(_tmp47B->f1).reentrant=Cyc_Absyn_noreentrant;_tmp47B;});
# 2237
struct _tuple33 _stmttmp33=({Cyc_Toc_temp_var_and_exp();});struct Cyc_Absyn_Exp*_tmp46B;struct _tuple1*_tmp46A;_LL4: _tmp46A=_stmttmp33.f1;_tmp46B=_stmttmp33.f2;_LL5: {struct _tuple1*v=_tmp46A;struct Cyc_Absyn_Exp*ve=_tmp46B;
ve->topt=offset_init_e->topt;
addr=({struct Cyc_Absyn_Exp*(*_tmp46C)(void*t,struct Cyc_Absyn_Exp*e)=Cyc_Toc_cast_it;void*_tmp46D=({Cyc_Toc_void_star_type();});struct Cyc_Absyn_Exp*_tmp46E=({Cyc_Absyn_address_exp(e2,0U);});_tmp46C(_tmp46D,_tmp46E);});
({void*_tmpDB5=({Cyc_Toc_void_star_type();});addr->topt=_tmpDB5;});
push=({struct Cyc_Absyn_Stmt*(*_tmp46F)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_exp_stmt;struct Cyc_Absyn_Exp*_tmp470=({struct Cyc_Absyn_Exp*_tmp471[8U];_tmp471[0]=update_handles_var_e,({
# 2244
struct Cyc_Absyn_Exp*_tmpDBA=({Cyc_Absyn_uint_exp((unsigned)update_handles_sz,0U);});_tmp471[1]=_tmpDBA;}),({
struct Cyc_Absyn_Exp*_tmpDB9=({struct Cyc_Absyn_Exp*(*_tmp472)(void*t,struct Cyc_Absyn_Exp*e)=Cyc_Toc_cast_it;void*_tmp473=({Cyc_Absyn_star_type((void*)ftyp,Cyc_Absyn_heap_rgn_type,Cyc_Toc_mt_tq,Cyc_Absyn_false_type);});struct Cyc_Absyn_Exp*_tmp474=e1;_tmp472(_tmp473,_tmp474);});_tmp471[2]=_tmpDB9;}),_tmp471[3]=addr,({
# 2248
struct Cyc_Absyn_Exp*_tmpDB8=({struct Cyc_Absyn_Exp*(*_tmp475)(void*t,struct Cyc_Absyn_Exp*e)=Cyc_Toc_cast_it;void*_tmp476=({Cyc_Absyn_star_type(Cyc_Absyn_uchar_type,Cyc_Absyn_heap_rgn_type,Cyc_Toc_mt_tq,Cyc_Absyn_false_type);});struct Cyc_Absyn_Exp*_tmp477=ve;_tmp475(_tmp476,_tmp477);});_tmp471[4]=_tmpDB8;}),({
# 2250
struct Cyc_Absyn_Exp*_tmpDB7=({Cyc_Absyn_uint_exp((unsigned)offset_sz,0U);});_tmp471[5]=_tmpDB7;}),({
struct Cyc_Absyn_Exp*_tmpDB6=({Cyc_Absyn_sizeoftype_exp((void*)_check_null(e2->topt),0U);});_tmp471[6]=_tmpDB6;}),_tmp471[7]=stack_size_e;({struct Cyc_Absyn_Exp*_tmpDBB=Cyc_Toc__spawn_with_effect_e;Cyc_Toc_fncall_exp_dl(_tmpDBB,_tag_fat(_tmp471,sizeof(struct Cyc_Absyn_Exp*),8U));});});unsigned _tmp478=0U;_tmp46F(_tmp470,_tmp478);});
# 2255
push=({Cyc_Absyn_declare_stmt(v,(void*)_check_null(ve->topt),offset_init_e,push,0U);});}}}else{_throw_match();}}else{_throw_match();}}else{
# 2257
push=({struct Cyc_Absyn_Stmt*(*_tmp47C)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_exp_stmt;struct Cyc_Absyn_Exp*_tmp47D=({struct Cyc_Absyn_Exp*_tmp47E[3U];_tmp47E[0]=addr,_tmp47E[1]=update_handles_var_e,({
# 2259
struct Cyc_Absyn_Exp*_tmpDBC=({Cyc_Absyn_uint_exp((unsigned)update_handles_sz,0U);});_tmp47E[2]=_tmpDBC;});({struct Cyc_Absyn_Exp*_tmpDBD=Cyc_Toc__push_effect_e;Cyc_Toc_fncall_exp_dl(_tmpDBD,_tag_fat(_tmp47E,sizeof(struct Cyc_Absyn_Exp*),3U));});});unsigned _tmp47F=0U;_tmp47C(_tmp47D,_tmp47F);});}
# 2262
if(update_handles_sz > 0)
push=({({struct Cyc_Absyn_Decl*_tmpDBF=({struct Cyc_Absyn_Decl*_tmp481=_cycalloc(sizeof(*_tmp481));({void*_tmpDBE=(void*)({struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*_tmp480=_cycalloc(sizeof(*_tmp480));_tmp480->tag=0U,_tmp480->f1=update_handles_var_decl;_tmp480;});_tmp481->r=_tmpDBE;}),_tmp481->loc=0U;_tmp481;});Cyc_Absyn_decl_stmt(_tmpDBF,push,0U);});});
# 2262
return push;}
# 2268
static struct Cyc_Absyn_Stmt*Cyc_Toc_genCallAndPop(struct Cyc_Absyn_Exp*e1,struct Cyc_List_List*es,void*ret_type,int new_thread){
# 2270
struct Cyc_Absyn_Stmt*s;
if(new_thread)
# 2274
s=({Cyc_Absyn_skip_stmt(0U);});else{
# 2277
struct Cyc_Absyn_Stmt*pop;
struct Cyc_Absyn_Exp*fncall;
fncall=({Cyc_Absyn_new_exp((void*)({struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*_tmp484=_cycalloc(sizeof(*_tmp484));_tmp484->tag=10U,_tmp484->f1=e1,_tmp484->f2=es,_tmp484->f3=0,_tmp484->f4=1,({struct _tuple0*_tmpDC0=({struct _tuple0*_tmp483=_cycalloc(sizeof(*_tmp483));_tmp483->f1=0,_tmp483->f2=0;_tmp483;});_tmp484->f5=_tmpDC0;});_tmp484;}),0U);});
pop=({struct Cyc_Absyn_Stmt*(*_tmp485)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_exp_stmt;struct Cyc_Absyn_Exp*_tmp486=({Cyc_Absyn_new_exp((void*)({struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*_tmp488=_cycalloc(sizeof(*_tmp488));_tmp488->tag=10U,_tmp488->f1=Cyc_Toc__pop_effect_e,_tmp488->f2=0,_tmp488->f3=0,_tmp488->f4=0,({struct _tuple0*_tmpDC1=({struct _tuple0*_tmp487=_cycalloc(sizeof(*_tmp487));_tmp487->f1=0,_tmp487->f2=0;_tmp487;});_tmp488->f5=_tmpDC1;});_tmp488;}),0U);});unsigned _tmp489=0U;_tmp485(_tmp486,_tmp489);});
# 2283
if(({Cyc_Tcutil_typecmp(ret_type,Cyc_Absyn_void_type);})){
struct _tuple1*v1=({Cyc_Toc_temp_var();});
# 2286
struct Cyc_Absyn_Stmt*v1e=({struct Cyc_Absyn_Stmt*(*_tmp495)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_exp_stmt;struct Cyc_Absyn_Exp*_tmp496=({Cyc_Absyn_var_exp(v1,0U);});unsigned _tmp497=0U;_tmp495(_tmp496,_tmp497);});
# 2288
struct Cyc_Absyn_Vardecl*letv=({struct Cyc_Absyn_Vardecl*(*_tmp490)(unsigned varloc,struct _tuple1*,void*,struct Cyc_Absyn_Exp*init)=Cyc_Absyn_new_vardecl;unsigned _tmp491=0U;struct _tuple1*_tmp492=v1;void*_tmp493=({Cyc_Toc_typ_to_c(ret_type);});struct Cyc_Absyn_Exp*_tmp494=fncall;_tmp490(_tmp491,_tmp492,_tmp493,_tmp494);});
s=({struct Cyc_Absyn_Stmt*(*_tmp48A)(struct Cyc_Absyn_Decl*,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_decl_stmt;struct Cyc_Absyn_Decl*_tmp48B=({struct Cyc_Absyn_Decl*_tmp48D=_cycalloc(sizeof(*_tmp48D));({void*_tmpDC2=(void*)({struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*_tmp48C=_cycalloc(sizeof(*_tmp48C));_tmp48C->tag=0U,_tmp48C->f1=letv;_tmp48C;});_tmp48D->r=_tmpDC2;}),_tmp48D->loc=0U;_tmp48D;});struct Cyc_Absyn_Stmt*_tmp48E=({Cyc_Absyn_seq_stmt(pop,v1e,0U);});unsigned _tmp48F=0U;_tmp48A(_tmp48B,_tmp48E,_tmp48F);});}else{
# 2292
s=({struct Cyc_Absyn_Stmt*(*_tmp498)(struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_seq_stmt;struct Cyc_Absyn_Stmt*_tmp499=({Cyc_Absyn_exp_stmt(fncall,0U);});struct Cyc_Absyn_Stmt*_tmp49A=pop;unsigned _tmp49B=0U;_tmp498(_tmp499,_tmp49A,_tmp49B);});}}
# 2295
return s;}struct _tuple39{void*f1;struct Cyc_Absyn_Exp*f2;};
# 2299
static struct _tuple39 Cyc_Toc_make_copy(struct Cyc_Absyn_Exp*e1){
struct Cyc_Absyn_Exp*e2=({Cyc_Absyn_new_exp(e1->r,e1->loc);});
e2->annot=e1->annot;
e2->topt=e1->topt;{
void*t=e2->topt == 0?(void*)({struct Cyc_Absyn_TypeofType_Absyn_Type_struct*_tmp49D=_cycalloc(sizeof(*_tmp49D));_tmp49D->tag=11U,_tmp49D->f1=e2;_tmp49D;}):({Cyc_Toc_typ_to_c((void*)_check_null(e2->topt));});
return({struct _tuple39 _tmpC86;_tmpC86.f1=t,_tmpC86.f2=e2;_tmpC86;});}}
# 2306
static void Cyc_Toc_print_exp(struct Cyc_Absyn_Exp*e){
({struct Cyc_String_pa_PrintArg_struct _tmp4A1=({struct Cyc_String_pa_PrintArg_struct _tmpC87;_tmpC87.tag=0U,({struct _fat_ptr _tmpDC3=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_exp2string(e);}));_tmpC87.f1=_tmpDC3;});_tmpC87;});void*_tmp49F[1U];_tmp49F[0]=& _tmp4A1;({struct _fat_ptr _tmpDC4=({const char*_tmp4A0="-- %s\n";_tag_fat(_tmp4A0,sizeof(char),7U);});Cyc_printf(_tmpDC4,_tag_fat(_tmp49F,sizeof(void*),1U));});});}struct _tuple40{struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Exp*f2;};
# 2310
static struct Cyc_Absyn_Stmt*Cyc_Toc_vardecl2stmt(struct _tuple40*tup,struct Cyc_Absyn_Stmt*s){
struct Cyc_Absyn_Exp*_tmp4A4;struct Cyc_Absyn_Vardecl*_tmp4A3;_LL1: _tmp4A3=tup->f1;_tmp4A4=tup->f2;_LL2: {struct Cyc_Absyn_Vardecl*vd=_tmp4A3;struct Cyc_Absyn_Exp*e=_tmp4A4;
return({({struct Cyc_Absyn_Decl*_tmpDC6=({struct Cyc_Absyn_Decl*_tmp4A6=_cycalloc(sizeof(*_tmp4A6));({void*_tmpDC5=(void*)({struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*_tmp4A5=_cycalloc(sizeof(*_tmp4A5));_tmp4A5->tag=0U,_tmp4A5->f1=vd;_tmp4A5;});_tmp4A6->r=_tmpDC5;}),_tmp4A6->loc=0U;_tmp4A6;});Cyc_Absyn_decl_stmt(_tmpDC6,s,0U);});});}}
# 2315
static struct Cyc_Absyn_Exp*Cyc_Toc_snd_elt(struct _tuple40*tup){
return(*tup).f2;}
# 2319
static struct _tuple40*Cyc_Toc_simplify(struct Cyc_Toc_Env*nv,struct Cyc_Absyn_Exp*e1){
struct _tuple33 _stmttmp34=({Cyc_Toc_temp_var_and_exp();});struct Cyc_Absyn_Exp*_tmp4AA;struct _tuple1*_tmp4A9;_LL1: _tmp4A9=_stmttmp34.f1;_tmp4AA=_stmttmp34.f2;_LL2: {struct _tuple1*v=_tmp4A9;struct Cyc_Absyn_Exp*ve=_tmp4AA;
struct _tuple39 _stmttmp35=({Cyc_Toc_make_copy(e1);});struct Cyc_Absyn_Exp*_tmp4AC;void*_tmp4AB;_LL4: _tmp4AB=_stmttmp35.f1;_tmp4AC=_stmttmp35.f2;_LL5: {void*t=_tmp4AB;struct Cyc_Absyn_Exp*e2=_tmp4AC;
ve->topt=t;
({Cyc_Toc_exp_to_c(nv,e2);});
# 2325
return({struct _tuple40*_tmp4AD=_cycalloc(sizeof(*_tmp4AD));({struct Cyc_Absyn_Vardecl*_tmpDC7=({Cyc_Absyn_new_vardecl(0U,v,t,e2);});_tmp4AD->f1=_tmpDC7;}),_tmp4AD->f2=ve;_tmp4AD;});}}}
# 2330
static struct Cyc_Absyn_FnInfo Cyc_Toc_getFnInfo(struct Cyc_Absyn_Exp*e1){
void*e1_typ=(void*)_check_null(e1->topt);
void*_tmp4AF=e1_typ;struct Cyc_Absyn_FnInfo _tmp4B0;struct Cyc_Absyn_FnInfo _tmp4B1;struct Cyc_Absyn_FnInfo _tmp4B2;struct Cyc_Absyn_FnInfo _tmp4B3;switch(*((int*)_tmp4AF)){case 3U: if(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp4AF)->f1).elt_type)->tag == 5U){_LL1: _tmp4B3=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp4AF)->f1).elt_type)->f1;_LL2: {struct Cyc_Absyn_FnInfo i=_tmp4B3;
# 2335
_tmp4B2=i;goto _LL4;}}else{goto _LL9;}case 8U: if(((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp4AF)->f4 != 0){if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp4AF)->f4)->tag == 3U){if(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp4AF)->f4)->f1).elt_type)->tag == 5U){_LL3: _tmp4B2=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp4AF)->f4)->f1).elt_type)->f1;_LL4: {struct Cyc_Absyn_FnInfo i=_tmp4B2;
_tmp4B1=i;goto _LL6;}}else{goto _LL9;}}else{goto _LL9;}}else{goto _LL9;}case 1U: if(((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp4AF)->f2 != 0){if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp4AF)->f2)->tag == 3U){if(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp4AF)->f2)->f1).elt_type)->tag == 5U){_LL5: _tmp4B1=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp4AF)->f2)->f1).elt_type)->f1;_LL6: {struct Cyc_Absyn_FnInfo i=_tmp4B1;
_tmp4B0=i;goto _LL8;}}else{goto _LL9;}}else{goto _LL9;}}else{goto _LL9;}case 5U: _LL7: _tmp4B0=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp4AF)->f1;_LL8: {struct Cyc_Absyn_FnInfo info2=_tmp4B0;
# 2339
return info2;}default: _LL9: _LLA:
# 2341
({struct Cyc_String_pa_PrintArg_struct _tmp4B7=({struct Cyc_String_pa_PrintArg_struct _tmpC88;_tmpC88.tag=0U,({struct _fat_ptr _tmpDC8=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(e1_typ);}));_tmpC88.f1=_tmpDC8;});_tmpC88;});void*_tmp4B4[1U];_tmp4B4[0]=& _tmp4B7;({int(*_tmpDCA)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp4B6)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmp4B6;});struct _fat_ptr _tmpDC9=({const char*_tmp4B5="genFnInfo: %s";_tag_fat(_tmp4B5,sizeof(char),14U);});_tmpDCA(_tmpDC9,_tag_fat(_tmp4B4,sizeof(void*),1U));});});}_LL0:;}
# 2346
static struct Cyc_List_List*Cyc_Toc_children_first(struct Cyc_List_List*upd){
if(upd == 0)return 0;if(upd->tl == 0)
return upd;{
# 2347
struct _fat_ptr ret=
# 2349
_tag_fat(0,0,0);
struct Cyc_List_List*roots=0;
struct Cyc_List_List*children=0;
{struct Cyc_List_List*iter=upd;for(0;iter != 0;iter=iter->tl){
struct Cyc_Absyn_RgnEffect*r=(struct Cyc_Absyn_RgnEffect*)iter->hd;
void*p=r->parent;
if(p == 0)({void*_tmp4B9=0U;({int(*_tmpDCC)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp4BB)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmp4BB;});struct _fat_ptr _tmpDCB=({const char*_tmp4BA="children_first: found abstracted parent in deletion effect";_tag_fat(_tmp4BA,sizeof(char),59U);});_tmpDCC(_tmpDCB,_tag_fat(_tmp4B9,sizeof(void*),0U));});});{void*_tmp4BC=p;if(_tmp4BC != 0){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp4BC)->tag == 0U){if(((struct Cyc_Absyn_HeapCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp4BC)->f1)->tag == 6U){_LL1: _LL2:
# 2361
 roots=({struct Cyc_List_List*_tmp4BD=_cycalloc(sizeof(*_tmp4BD));_tmp4BD->hd=r,_tmp4BD->tl=roots;_tmp4BD;});
continue;}else{goto _LL3;}}else{goto _LL3;}}else{_LL3: _LL4:
 goto _LL0;}_LL0:;}{
# 2365
struct Cyc_Absyn_RgnEffect*r1=({struct Cyc_Absyn_RgnEffect*(*_tmp4C0)(struct Cyc_Absyn_Tvar*x,struct Cyc_List_List*l)=Cyc_Absyn_find_rgneffect;struct Cyc_Absyn_Tvar*_tmp4C1=({Cyc_Absyn_type2tvar(p);});struct Cyc_List_List*_tmp4C2=upd;_tmp4C0(_tmp4C1,_tmp4C2);});
if(r1 == 0)roots=({struct Cyc_List_List*_tmp4BE=_cycalloc(sizeof(*_tmp4BE));_tmp4BE->hd=r,_tmp4BE->tl=roots;_tmp4BE;});else{
children=({struct Cyc_List_List*_tmp4BF=_cycalloc(sizeof(*_tmp4BF));_tmp4BF->hd=r,_tmp4BF->tl=children;_tmp4BF;});}}}}
# 2369
if(children == 0)return({Cyc_List_rev(roots);});else{
return({struct Cyc_List_List*(*_tmp4C3)(struct Cyc_List_List*x,struct Cyc_List_List*y)=Cyc_List_imp_append;struct Cyc_List_List*_tmp4C4=({struct Cyc_List_List*(*_tmp4C5)(struct Cyc_List_List*upd)=Cyc_Toc_children_first;struct Cyc_List_List*_tmp4C6=({Cyc_List_rev(children);});_tmp4C5(_tmp4C6);});struct Cyc_List_List*_tmp4C7=({Cyc_List_rev(roots);});_tmp4C3(_tmp4C4,_tmp4C7);});}}}
# 2373
static void*Cyc_Toc_id(void*obj){
return obj;}
# 2377
static struct _tuple22*Cyc_Toc_findThreadOffsets(struct Cyc_List_List*threadEffect,struct Cyc_List_List*summarizedEffect){
# 2379
struct Cyc_List_List*ret=0;
int i=0;
for(0;threadEffect != 0;threadEffect=threadEffect->tl){
struct Cyc_Absyn_RgnEffect*r=(struct Cyc_Absyn_RgnEffect*)threadEffect->hd;
int idx=0;
struct Cyc_List_List*iter0;
for(iter0=summarizedEffect;iter0 != 0 &&({Cyc_Absyn_rgneffect_cmp_name(r,(struct Cyc_Absyn_RgnEffect*)iter0->hd);})== 0;(
# 2387
iter0=iter0->tl,idx ++)){;}
if(iter0 == 0)({void*_tmp4CA=0U;({int(*_tmpDCE)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp4CC)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmp4CC;});struct _fat_ptr _tmpDCD=({const char*_tmp4CB="findThreadOffset";_tag_fat(_tmp4CB,sizeof(char),17U);});_tmpDCE(_tmpDCD,_tag_fat(_tmp4CA,sizeof(void*),0U));});});ret=({struct Cyc_List_List*_tmp4D4=_cycalloc(sizeof(*_tmp4D4));
({struct _tuple21*_tmpDD1=({struct _tuple21*(*_tmp4CD)(struct _fat_ptr*name,struct Cyc_Absyn_Exp*e)=Cyc_Toc_make_field;struct _fat_ptr*_tmp4CE=({struct _fat_ptr*_tmp4D2=_cycalloc(sizeof(*_tmp4D2));({struct _fat_ptr _tmpDD0=(struct _fat_ptr)({struct Cyc_Int_pa_PrintArg_struct _tmp4D1=({struct Cyc_Int_pa_PrintArg_struct _tmpC89;_tmpC89.tag=1U,_tmpC89.f1=(unsigned long)i ++;_tmpC89;});void*_tmp4CF[1U];_tmp4CF[0]=& _tmp4D1;({struct _fat_ptr _tmpDCF=({const char*_tmp4D0="%d";_tag_fat(_tmp4D0,sizeof(char),3U);});Cyc_aprintf(_tmpDCF,_tag_fat(_tmp4CF,sizeof(void*),1U));});});*_tmp4D2=_tmpDD0;});_tmp4D2;});struct Cyc_Absyn_Exp*_tmp4D3=({Cyc_Absyn_uint_exp((unsigned)idx,0U);});_tmp4CD(_tmp4CE,_tmp4D3);});_tmp4D4->hd=_tmpDD1;}),_tmp4D4->tl=ret;_tmp4D4;});}
# 2392
ret=({Cyc_List_imp_rev(ret);});{
int j=i;
if(j == 0)++ j;{struct Cyc_Absyn_Exp*init0=({Cyc_Absyn_new_exp((void*)({struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*_tmp4DC=_cycalloc(sizeof(*_tmp4DC));_tmp4DC->tag=26U,_tmp4DC->f1=ret;_tmp4DC;}),0U);});
# 2396
({void*_tmpDD2=({void*(*_tmp4D5)(void*elt_type,struct Cyc_Absyn_Tqual tq,struct Cyc_Absyn_Exp*num_elts,void*zero_term,unsigned ztloc)=Cyc_Absyn_array_type;void*_tmp4D6=Cyc_Absyn_uchar_type;struct Cyc_Absyn_Tqual _tmp4D7=Cyc_Toc_mt_tq;struct Cyc_Absyn_Exp*_tmp4D8=({Cyc_Absyn_uint_exp((unsigned)j,0U);});void*_tmp4D9=Cyc_Absyn_false_type;unsigned _tmp4DA=0U;_tmp4D5(_tmp4D6,_tmp4D7,_tmp4D8,_tmp4D9,_tmp4DA);});init0->topt=_tmpDD2;});
# 2398
return({struct _tuple22*_tmp4DB=_cycalloc(sizeof(*_tmp4DB));_tmp4DB->f1=init0,_tmp4DB->f2=(unsigned)i;_tmp4DB;});}}}
# 2401
static struct Cyc_Absyn_Stmt*Cyc_Toc_genFnCallStmt(struct Cyc_Absyn_Exp*e1,struct Cyc_List_List*es,struct Cyc_List_List*ef_in,struct Cyc_List_List*ef_out,struct Cyc_Absyn_Exp*stack_size_e){
# 2406
const int new_thread=stack_size_e != 0;
struct Cyc_Absyn_FnInfo info=({Cyc_Toc_getFnInfo(e1);});
# 2410
if(new_thread == 0 &&({Cyc_Toc_should_insert_xeffect(info.ieffect,info.oeffect);})== 0)
# 2412
return({struct Cyc_Absyn_Stmt*(*_tmp4DE)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_exp_stmt;struct Cyc_Absyn_Exp*_tmp4DF=({Cyc_Absyn_new_exp((void*)({struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*_tmp4E1=_cycalloc(sizeof(*_tmp4E1));_tmp4E1->tag=10U,_tmp4E1->f1=e1,_tmp4E1->f2=es,_tmp4E1->f3=0,_tmp4E1->f4=1,({struct _tuple0*_tmpDD3=({struct _tuple0*_tmp4E0=_cycalloc(sizeof(*_tmp4E0));_tmp4E0->f1=0,_tmp4E0->f2=0;_tmp4E0;});_tmp4E1->f5=_tmpDD3;});_tmp4E1;}),0U);});unsigned _tmp4E2=0U;_tmp4DE(_tmp4DF,_tmp4E2);});{
# 2440 "toc.cyc"
struct Cyc_List_List*iter;
struct Cyc_List_List*update_f=0;
struct Cyc_List_List*formal_f=0;
# 2444
if(new_thread)for(iter=info.ieffect;iter != 0;iter=iter->tl){
struct Cyc_Absyn_Tvar*tv=({struct Cyc_Absyn_Tvar*(*_tmp4E7)(void*t)=Cyc_Absyn_type2tvar;void*_tmp4E8=({Cyc_Absyn_rgneffect_name((struct Cyc_Absyn_RgnEffect*)iter->hd);});_tmp4E7(_tmp4E8);});
struct _tuple13 _stmttmp36=({struct _tuple13(*_tmp4E5)(struct Cyc_List_List*)=Cyc_Absyn_caps2tup;struct Cyc_List_List*_tmp4E6=({Cyc_Absyn_rgneffect_caps((struct Cyc_Absyn_RgnEffect*)iter->hd);});_tmp4E5(_tmp4E6);});int _tmp4E3;_LL1: _tmp4E3=_stmttmp36.f3;_LL2: {int alias_r1=_tmp4E3;
struct Cyc_Absyn_RgnEffect*if1;
if((if1=({Cyc_Absyn_find_rgneffect(tv,info.ieffect);}))!= 0)
update_f=({struct Cyc_List_List*_tmp4E4=_cycalloc(sizeof(*_tmp4E4));_tmp4E4->hd=(struct Cyc_Absyn_RgnEffect*)iter->hd,_tmp4E4->tl=update_f;_tmp4E4;});}}else{
# 2451
for(iter=ef_in;iter != 0;iter=iter->tl){
struct Cyc_Absyn_Tvar*tv=({struct Cyc_Absyn_Tvar*(*_tmp4EA)(void*t)=Cyc_Absyn_type2tvar;void*_tmp4EB=({Cyc_Absyn_rgneffect_name((struct Cyc_Absyn_RgnEffect*)iter->hd);});_tmp4EA(_tmp4EB);});
if(({Cyc_Absyn_find_rgneffect(tv,ef_out);})== 0){
struct Cyc_Absyn_RgnEffect*if1=({Cyc_Absyn_find_rgneffect(tv,info.ieffect);});
if(if1 != 0)
# 2468 "toc.cyc"
;else{
update_f=({struct Cyc_List_List*_tmp4E9=_cycalloc(sizeof(*_tmp4E9));_tmp4E9->hd=(struct Cyc_Absyn_RgnEffect*)iter->hd,_tmp4E9->tl=update_f;_tmp4E9;});}}}}
# 2479
formal_f=info.ieffect;
update_f=({Cyc_List_imp_rev(update_f);});{
# 2482
struct Cyc_List_List*actual_order=({({struct Cyc_List_List*(*_tmpDD5)(struct Cyc_Absyn_RgnEffect*(*f)(struct Cyc_Absyn_RgnEffect*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp4FD)(struct Cyc_Absyn_RgnEffect*(*f)(struct Cyc_Absyn_RgnEffect*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_RgnEffect*(*f)(struct Cyc_Absyn_RgnEffect*),struct Cyc_List_List*x))Cyc_List_map;_tmp4FD;});struct Cyc_Absyn_RgnEffect*(*_tmpDD4)(struct Cyc_Absyn_RgnEffect*obj)=({struct Cyc_Absyn_RgnEffect*(*_tmp4FE)(struct Cyc_Absyn_RgnEffect*obj)=(struct Cyc_Absyn_RgnEffect*(*)(struct Cyc_Absyn_RgnEffect*obj))Cyc_Toc_id;_tmp4FE;});_tmpDD5(_tmpDD4,update_f);});});
# 2484
struct _tuple22*thread_offsets_init=0;
# 2486
if(new_thread == 1){
# 2489
struct Cyc_List_List*update_f_sum=({struct Cyc_List_List*(*_tmp4EC)(struct Cyc_List_List*x)=Cyc_List_rev;struct Cyc_List_List*_tmp4ED=({Cyc_IOEffect_summarize_all(0U,update_f);});_tmp4EC(_tmp4ED);});
# 2496
thread_offsets_init=({Cyc_Toc_findThreadOffsets(actual_order,update_f_sum);});
# 2502
update_f=update_f_sum;}else{
# 2507
update_f=({Cyc_Toc_children_first(update_f);});}{
# 2511
struct _tuple36 _stmttmp37=({Cyc_Toc_genEffectFrameDecl(formal_f,update_f);});struct Cyc_Absyn_Vardecl*_tmp4F3;struct Cyc_Absyn_Vardecl*_tmp4F2;struct Cyc_Absyn_Exp*_tmp4F1;int _tmp4F0;struct Cyc_Absyn_Exp*_tmp4EF;struct Cyc_Absyn_Vardecl*_tmp4EE;_LL4: _tmp4EE=_stmttmp37.f1;_tmp4EF=_stmttmp37.f2;_tmp4F0=_stmttmp37.f3;_tmp4F1=_stmttmp37.f4;_tmp4F2=_stmttmp37.f5;_tmp4F3=_stmttmp37.f6;_LL5: {struct Cyc_Absyn_Vardecl*effVarDecl=_tmp4EE;struct Cyc_Absyn_Exp*update_handles_var_e=_tmp4EF;int update_handles_sz=_tmp4F0;struct Cyc_Absyn_Exp*addr=_tmp4F1;struct Cyc_Absyn_Vardecl*update_handles_var_decl=_tmp4F2;struct Cyc_Absyn_Vardecl*formal_handles_var_decl=_tmp4F3;
# 2519
struct Cyc_Absyn_Stmt*push=({({struct Cyc_Absyn_Exp*_tmpDD9=update_handles_var_e;int _tmpDD8=update_handles_sz;struct Cyc_Absyn_Exp*_tmpDD7=addr;struct Cyc_Absyn_Vardecl*_tmpDD6=update_handles_var_decl;Cyc_Toc_genPush(_tmpDD9,_tmpDD8,_tmpDD7,_tmpDD6,new_thread?({struct _tuple38*_tmp4FC=_cycalloc(sizeof(*_tmp4FC));_tmp4FC->f1=e1,_tmp4FC->f2=es,_tmp4FC->f3=(struct _tuple37*)_check_null(thread_offsets_init),_tmp4FC->f4=(struct Cyc_Absyn_Exp*)_check_null(stack_size_e);_tmp4FC;}): 0);});});
# 2523
struct Cyc_Absyn_Stmt*pop=({Cyc_Toc_genCallAndPop(e1,es,info.ret_type,new_thread);});
# 2526
if(new_thread == 1)
push=({Cyc_Absyn_seq_stmt(push,pop,0U);});else{
# 2529
push=({struct Cyc_Absyn_Stmt*(*_tmp4F4)(struct Cyc_Absyn_Decl*,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_decl_stmt;struct Cyc_Absyn_Decl*_tmp4F5=({struct Cyc_Absyn_Decl*_tmp4F7=_cycalloc(sizeof(*_tmp4F7));({void*_tmpDDA=(void*)({struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*_tmp4F6=_cycalloc(sizeof(*_tmp4F6));_tmp4F6->tag=0U,_tmp4F6->f1=effVarDecl;_tmp4F6;});_tmp4F7->r=_tmpDDA;}),_tmp4F7->loc=0U;_tmp4F7;});struct Cyc_Absyn_Stmt*_tmp4F8=({Cyc_Absyn_seq_stmt(push,pop,0U);});unsigned _tmp4F9=0U;_tmp4F4(_tmp4F5,_tmp4F8,_tmp4F9);});}
# 2531
return({({struct Cyc_Absyn_Decl*_tmpDDC=({struct Cyc_Absyn_Decl*_tmp4FB=_cycalloc(sizeof(*_tmp4FB));({void*_tmpDDB=(void*)({struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*_tmp4FA=_cycalloc(sizeof(*_tmp4FA));_tmp4FA->tag=0U,_tmp4FA->f1=formal_handles_var_decl;_tmp4FA;});_tmp4FB->r=_tmpDDB;}),_tmp4FB->loc=0U;_tmp4FB;});Cyc_Absyn_decl_stmt(_tmpDDC,push,0U);});});}}}}}
# 2534
struct _tuple14*Cyc_Toc_map_tup(struct _tuple9*t){
return({struct _tuple14*_tmp500=_cycalloc(sizeof(*_tmp500));_tmp500->f1=(*t).f2,_tmp500->f2=(*t).f3;_tmp500;});}
# 2538
void Cyc_Toc_makeFnCall(struct Cyc_Toc_Env*nv,struct Cyc_Absyn_Exp*e,struct Cyc_Absyn_Exp*e1,struct Cyc_List_List*es,struct Cyc_List_List*ef_in,struct Cyc_List_List*ef_out,int need_check,int is_thread){
# 2541
static unsigned thread_uid=0U;
int must_convert=0;
# 2544
if(is_thread){
if(es == 0)({void*_tmp502=0U;({int(*_tmpDDE)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp504)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmp504;});struct _fat_ptr _tmpDDD=({const char*_tmp503="makeFnCall 1";_tag_fat(_tmp503,sizeof(char),13U);});_tmpDDE(_tmpDDD,_tag_fat(_tmp502,sizeof(void*),0U));});});{struct Cyc_Absyn_Exp*_tmp505;if(es != 0){_LL1: _tmp505=(struct Cyc_Absyn_Exp*)es->hd;_LL2: {struct Cyc_Absyn_Exp*ez=_tmp505;
# 2547
struct _fat_ptr lst=_tag_fat(0,0,0);
void*fn_typ=0;
struct Cyc_Absyn_Exp*tup=0;
int arg_len=0;
struct Cyc_List_List*fn_in=0;
struct Cyc_List_List*fn_out=0;
{void*_stmttmp38=ez->r;void*_tmp506=_stmttmp38;struct Cyc_List_List*_tmp508;struct Cyc_Absyn_Exp*_tmp507;if(((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp506)->tag == 10U){_LL4: _tmp507=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp506)->f1;_tmp508=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp506)->f2;_LL5: {struct Cyc_Absyn_Exp*fn=_tmp507;struct Cyc_List_List*es1=_tmp508;
# 2555
fn_typ=fn->topt;{
struct Cyc_Absyn_FnInfo i=({Cyc_Toc_getFnInfo(fn);});
fn_in=i.ieffect;
fn_out=i.oeffect;
arg_len=({Cyc_List_length(es1);})+ 1;
tup=({Cyc_Absyn_tuple_exp(({struct Cyc_List_List*_tmp509=_cycalloc(sizeof(*_tmp509));_tmp509->hd=fn,_tmp509->tl=es1;_tmp509;}),0U);});
({void*_tmpDE3=(void*)({struct Cyc_Absyn_TupleType_Absyn_Type_struct*_tmp50D=_cycalloc(sizeof(*_tmp50D));_tmp50D->tag=6U,({struct Cyc_List_List*_tmpDE2=({struct Cyc_List_List*_tmp50C=_cycalloc(sizeof(*_tmp50C));
({struct _tuple14*_tmpDE1=({struct _tuple14*_tmp50A=_cycalloc(sizeof(*_tmp50A));_tmp50A->f1=Cyc_Toc_mt_tq,_tmp50A->f2=(void*)_check_null(fn_typ);_tmp50A;});_tmp50C->hd=_tmpDE1;}),({
struct Cyc_List_List*_tmpDE0=({({struct Cyc_List_List*(*_tmpDDF)(struct _tuple14*(*f)(struct _tuple9*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp50B)(struct _tuple14*(*f)(struct _tuple9*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct _tuple14*(*f)(struct _tuple9*),struct Cyc_List_List*x))Cyc_List_map;_tmp50B;});_tmpDDF(Cyc_Toc_map_tup,i.args);});});_tmp50C->tl=_tmpDE0;});_tmp50C;});
# 2561
_tmp50D->f1=_tmpDE2;});_tmp50D;});tup->topt=_tmpDE3;});
# 2564
es=({struct Cyc_List_List*_tmp50F=_cycalloc(sizeof(*_tmp50F));_tmp50F->hd=e1,({struct Cyc_List_List*_tmpDE4=({struct Cyc_List_List*_tmp50E=_cycalloc(sizeof(*_tmp50E));_tmp50E->hd=tup,_tmp50E->tl=0;_tmp50E;});_tmp50F->tl=_tmpDE4;});_tmp50F;});
goto _LL3;}}}else{_LL6: _LL7:
({void*_tmp510=0U;({int(*_tmpDE6)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp512)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmp512;});struct _fat_ptr _tmpDE5=({const char*_tmp511="makeFnCall 2";_tag_fat(_tmp511,sizeof(char),13U);});_tmpDE6(_tmpDE5,_tag_fat(_tmp510,sizeof(void*),0U));});});}_LL3:;}{
# 2569
struct Cyc_List_List*lst=({({struct Cyc_List_List*(*_tmpDE8)(struct _tuple40*(*f)(struct Cyc_Toc_Env*,struct Cyc_Absyn_Exp*),struct Cyc_Toc_Env*env,struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp52C)(struct _tuple40*(*f)(struct Cyc_Toc_Env*,struct Cyc_Absyn_Exp*),struct Cyc_Toc_Env*env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct _tuple40*(*f)(struct Cyc_Toc_Env*,struct Cyc_Absyn_Exp*),struct Cyc_Toc_Env*env,struct Cyc_List_List*x))Cyc_List_map_c;_tmp52C;});struct Cyc_Toc_Env*_tmpDE7=nv;_tmpDE8(Cyc_Toc_simplify,_tmpDE7,es);});});
struct Cyc_List_List*vars=({({struct Cyc_List_List*(*_tmpDE9)(struct Cyc_Absyn_Exp*(*f)(struct _tuple40*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp52B)(struct Cyc_Absyn_Exp*(*f)(struct _tuple40*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_Exp*(*f)(struct _tuple40*),struct Cyc_List_List*x))Cyc_List_map;_tmp52B;});_tmpDE9(Cyc_Toc_snd_elt,lst);});});
struct Cyc_List_List*_tmp514;struct Cyc_Absyn_Exp*_tmp513;if(vars != 0){_LL9: _tmp513=(struct Cyc_Absyn_Exp*)vars->hd;_tmp514=vars->tl;_LLA: {struct Cyc_Absyn_Exp*v_stack_e=_tmp513;struct Cyc_List_List*args=_tmp514;
void*param_typ=({void*(*_tmp528)(void*,struct Cyc_Absyn_Tqual)=Cyc_Absyn_cstar_type;void*_tmp529=({Cyc_Toc_typ_to_c((void*)_check_null(tup->topt));});struct Cyc_Absyn_Tqual _tmp52A=Cyc_Toc_mt_tq;_tmp528(_tmp529,_tmp52A);});
struct _tuple33 _stmttmp39=({Cyc_Toc_temp_var_and_exp();});struct Cyc_Absyn_Exp*_tmp516;struct _tuple1*_tmp515;_LLC: _tmp515=_stmttmp39.f1;_tmp516=_stmttmp39.f2;_LLD: {struct _tuple1*wrap_var=_tmp515;struct Cyc_Absyn_Exp*wrap_var_e=_tmp516;
struct _tuple33 _stmttmp3A=({Cyc_Toc_temp_var_and_exp();});struct Cyc_Absyn_Exp*_tmp518;struct _tuple1*_tmp517;_LLF: _tmp517=_stmttmp3A.f1;_tmp518=_stmttmp3A.f2;_LL10: {struct _tuple1*param_var=_tmp517;struct Cyc_Absyn_Exp*param_var_e=_tmp518;
struct Cyc_Absyn_FnInfo wrap_info=({struct Cyc_Absyn_FnInfo _tmpC8A;_tmpC8A.tvars=0,_tmpC8A.effect=0,({struct Cyc_Absyn_Tqual _tmpDEC=({Cyc_Absyn_empty_tqual(0U);});_tmpC8A.ret_tqual=_tmpDEC;}),_tmpC8A.ret_type=Cyc_Absyn_void_type,({
# 2577
struct Cyc_List_List*_tmpDEB=({struct Cyc_List_List*_tmp527=_cycalloc(sizeof(*_tmp527));({struct _tuple9*_tmpDEA=({struct _tuple9*_tmp526=_cycalloc(sizeof(*_tmp526));_tmp526->f1=(*param_var).f2,_tmp526->f2=Cyc_Toc_mt_tq,_tmp526->f3=param_typ;_tmp526;});_tmp527->hd=_tmpDEA;}),_tmp527->tl=0;_tmp527;});_tmpC8A.args=_tmpDEB;}),_tmpC8A.c_varargs=0,_tmpC8A.cyc_varargs=0,_tmpC8A.rgn_po=0,_tmpC8A.attributes=0,_tmpC8A.requires_clause=0,_tmpC8A.requires_relns=0,_tmpC8A.ensures_clause=0,_tmpC8A.ensures_relns=0,_tmpC8A.ieffect=fn_in,_tmpC8A.oeffect=fn_out,_tmpC8A.throws=0,_tmpC8A.reentrant=Cyc_Absyn_noreentrant;_tmpC8A;});
# 2583
({void*_tmpDED=(void*)({struct Cyc_Absyn_FnType_Absyn_Type_struct*_tmp519=_cycalloc(sizeof(*_tmp519));_tmp519->tag=5U,_tmp519->f1=wrap_info;_tmp519;});wrap_var_e->topt=_tmpDED;});{
struct Cyc_List_List*wrap_fn_args=0;
struct Cyc_Absyn_Exp*deref_param_var_e=({Cyc_Absyn_deref_exp(param_var_e,0U);});
{int i=1;for(0;i <= arg_len;++ i){
wrap_fn_args=({struct Cyc_List_List*_tmp51E=_cycalloc(sizeof(*_tmp51E));({struct Cyc_Absyn_Exp*_tmpDEE=({struct Cyc_Absyn_Exp*(*_tmp51A)(struct Cyc_Absyn_Exp*,struct _fat_ptr*,unsigned)=Cyc_Absyn_aggrmember_exp;struct Cyc_Absyn_Exp*_tmp51B=deref_param_var_e;struct _fat_ptr*_tmp51C=({Cyc_Absyn_fieldname(i);});unsigned _tmp51D=0U;_tmp51A(_tmp51B,_tmp51C,_tmp51D);});_tmp51E->hd=_tmpDEE;}),_tmp51E->tl=wrap_fn_args;_tmp51E;});}}
# 2589
wrap_fn_args=({Cyc_List_imp_rev(wrap_fn_args);});{
# 2593
struct Cyc_Absyn_Stmt*wrap_body_s=({struct Cyc_Absyn_Stmt*(*_tmp523)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_exp_stmt;struct Cyc_Absyn_Exp*_tmp524=({Cyc_Absyn_fncall_exp((struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(wrap_fn_args))->hd,wrap_fn_args->tl,0U);});unsigned _tmp525=0U;_tmp523(_tmp524,_tmp525);});
# 2596
struct Cyc_Absyn_Stmt*inner=({Cyc_Toc_genFnCallStmt(wrap_var_e,args,ef_in,ef_out,e1);});
# 2598
struct Cyc_Absyn_Fndecl*wrap_fn_d=({struct Cyc_Absyn_Fndecl*_tmp522=_cycalloc(sizeof(*_tmp522));_tmp522->sc=Cyc_Absyn_Static,_tmp522->is_inline=0,_tmp522->name=wrap_var,_tmp522->body=wrap_body_s,_tmp522->i=wrap_info,_tmp522->cached_type=0,_tmp522->param_vardecls=0,_tmp522->fn_vardecl=0;_tmp522;});
# 2601
Cyc_Toc_result_decls=({struct Cyc_List_List*_tmp520=_cycalloc(sizeof(*_tmp520));({struct Cyc_Absyn_Decl*_tmpDEF=({Cyc_Absyn_new_decl((void*)({struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*_tmp51F=_cycalloc(sizeof(*_tmp51F));_tmp51F->tag=1U,_tmp51F->f1=wrap_fn_d;_tmp51F;}),0U);});_tmp520->hd=_tmpDEF;}),_tmp520->tl=Cyc_Toc_result_decls;_tmp520;});{
struct Cyc_Absyn_Stmt*s0=({({struct Cyc_Absyn_Stmt*(*_tmpDF1)(struct Cyc_Absyn_Stmt*(*f)(struct _tuple40*,struct Cyc_Absyn_Stmt*),struct Cyc_List_List*x,struct Cyc_Absyn_Stmt*accum)=({struct Cyc_Absyn_Stmt*(*_tmp521)(struct Cyc_Absyn_Stmt*(*f)(struct _tuple40*,struct Cyc_Absyn_Stmt*),struct Cyc_List_List*x,struct Cyc_Absyn_Stmt*accum)=(struct Cyc_Absyn_Stmt*(*)(struct Cyc_Absyn_Stmt*(*f)(struct _tuple40*,struct Cyc_Absyn_Stmt*),struct Cyc_List_List*x,struct Cyc_Absyn_Stmt*accum))Cyc_List_fold_right;_tmp521;});struct Cyc_List_List*_tmpDF0=lst;_tmpDF1(Cyc_Toc_vardecl2stmt,_tmpDF0,inner);});});
# 2605
({void*_tmpDF2=({Cyc_Toc_stmt_exp_r(s0);});e->r=_tmpDF2;});
return;}}}}}}}else{_throw_match();}}}}else{_throw_match();}}}
# 2544
{
# 2609
struct Cyc_List_List*iter=({struct Cyc_List_List*_tmp52D=_cycalloc(sizeof(*_tmp52D));_tmp52D->hd=e1,_tmp52D->tl=es;_tmp52D;});
# 2544
for(0;iter != 0;iter=iter->tl){
# 2610
struct Cyc_Absyn_Exp*e0=(struct Cyc_Absyn_Exp*)iter->hd;
if(({Cyc_Absyn_no_side_effects_exp(e0);})== 0){
# 2613
must_convert=1;
break;}}}
# 2620
if(must_convert){
# 2622
struct Cyc_List_List*lst=({({struct Cyc_List_List*(*_tmpDF4)(struct _tuple40*(*f)(struct Cyc_Toc_Env*,struct Cyc_Absyn_Exp*),struct Cyc_Toc_Env*env,struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp536)(struct _tuple40*(*f)(struct Cyc_Toc_Env*,struct Cyc_Absyn_Exp*),struct Cyc_Toc_Env*env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct _tuple40*(*f)(struct Cyc_Toc_Env*,struct Cyc_Absyn_Exp*),struct Cyc_Toc_Env*env,struct Cyc_List_List*x))Cyc_List_map_c;_tmp536;});struct Cyc_Toc_Env*_tmpDF3=nv;_tmpDF4(Cyc_Toc_simplify,_tmpDF3,({struct Cyc_List_List*_tmp537=_cycalloc(sizeof(*_tmp537));_tmp537->hd=e1,_tmp537->tl=es;_tmp537;}));});});
struct Cyc_List_List*vars=({({struct Cyc_List_List*(*_tmpDF5)(struct Cyc_Absyn_Exp*(*f)(struct _tuple40*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp535)(struct Cyc_Absyn_Exp*(*f)(struct _tuple40*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_Exp*(*f)(struct _tuple40*),struct Cyc_List_List*x))Cyc_List_map;_tmp535;});_tmpDF5(Cyc_Toc_snd_elt,lst);});});
struct Cyc_List_List*_tmp52F;struct Cyc_Absyn_Exp*_tmp52E;if(vars != 0){_LL12: _tmp52E=(struct Cyc_Absyn_Exp*)vars->hd;_tmp52F=vars->tl;_LL13: {struct Cyc_Absyn_Exp*fn=_tmp52E;struct Cyc_List_List*args=_tmp52F;
# 2626
void*fn_typ=e1->topt;
fn->topt=fn_typ;
if(need_check)
({void*_tmpDF8=({void*(*_tmp530)(void*t,struct Cyc_Absyn_Exp*e)=Cyc_Toc_cast_it_r;void*_tmp531=({Cyc_Toc_typ_to_c((void*)_check_null(fn_typ));});struct Cyc_Absyn_Exp*_tmp532=({struct Cyc_Absyn_Exp*_tmp533[1U];({struct Cyc_Absyn_Exp*_tmpDF6=({Cyc_Absyn_copy_exp(fn);});_tmp533[0]=_tmpDF6;});({struct Cyc_Absyn_Exp*_tmpDF7=Cyc_Toc__check_null_e;Cyc_Toc_fncall_exp_dl(_tmpDF7,_tag_fat(_tmp533,sizeof(struct Cyc_Absyn_Exp*),1U));});});_tmp530(_tmp531,_tmp532);});fn->r=_tmpDF8;});{
# 2628
struct Cyc_Absyn_Stmt*inner=({Cyc_Toc_genFnCallStmt(fn,args,ef_in,ef_out,0);});
# 2633
struct Cyc_Absyn_Stmt*s0=({({struct Cyc_Absyn_Stmt*(*_tmpDFA)(struct Cyc_Absyn_Stmt*(*f)(struct _tuple40*,struct Cyc_Absyn_Stmt*),struct Cyc_List_List*x,struct Cyc_Absyn_Stmt*accum)=({struct Cyc_Absyn_Stmt*(*_tmp534)(struct Cyc_Absyn_Stmt*(*f)(struct _tuple40*,struct Cyc_Absyn_Stmt*),struct Cyc_List_List*x,struct Cyc_Absyn_Stmt*accum)=(struct Cyc_Absyn_Stmt*(*)(struct Cyc_Absyn_Stmt*(*f)(struct _tuple40*,struct Cyc_Absyn_Stmt*),struct Cyc_List_List*x,struct Cyc_Absyn_Stmt*accum))Cyc_List_fold_right;_tmp534;});struct Cyc_List_List*_tmpDF9=lst;_tmpDFA(Cyc_Toc_vardecl2stmt,_tmpDF9,inner);});});
# 2643 "toc.cyc"
({void*_tmpDFB=({Cyc_Toc_stmt_exp_r(s0);});e->r=_tmpDFB;});}}}else{_throw_match();}}else{
# 2645
void*e1_typ=(void*)_check_null(e1->topt);
({Cyc_Toc_exp_to_c(nv,e1);});
if(need_check)
({void*_tmpDFE=({void*(*_tmp538)(void*t,struct Cyc_Absyn_Exp*e)=Cyc_Toc_cast_it_r;void*_tmp539=({Cyc_Toc_typ_to_c(e1_typ);});struct Cyc_Absyn_Exp*_tmp53A=({struct Cyc_Absyn_Exp*_tmp53B[1U];({struct Cyc_Absyn_Exp*_tmpDFC=({Cyc_Absyn_copy_exp(e1);});_tmp53B[0]=_tmpDFC;});({struct Cyc_Absyn_Exp*_tmpDFD=Cyc_Toc__check_null_e;Cyc_Toc_fncall_exp_dl(_tmpDFD,_tag_fat(_tmp53B,sizeof(struct Cyc_Absyn_Exp*),1U));});});_tmp538(_tmp539,_tmp53A);});e1->r=_tmpDFE;});
# 2647
({({void(*_tmpE00)(void(*f)(struct Cyc_Toc_Env*,struct Cyc_Absyn_Exp*),struct Cyc_Toc_Env*env,struct Cyc_List_List*x)=({void(*_tmp53C)(void(*f)(struct Cyc_Toc_Env*,struct Cyc_Absyn_Exp*),struct Cyc_Toc_Env*env,struct Cyc_List_List*x)=(void(*)(void(*f)(struct Cyc_Toc_Env*,struct Cyc_Absyn_Exp*),struct Cyc_Toc_Env*env,struct Cyc_List_List*x))Cyc_List_iter_c;_tmp53C;});struct Cyc_Toc_Env*_tmpDFF=nv;_tmpE00(Cyc_Toc_exp_to_c,_tmpDFF,es);});});
# 2651
({void*_tmpE01=({void*(*_tmp53D)(struct Cyc_Absyn_Stmt*s)=Cyc_Toc_stmt_exp_r;struct Cyc_Absyn_Stmt*_tmp53E=({Cyc_Toc_genFnCallStmt(e1,es,ef_in,ef_out,0);});_tmp53D(_tmp53E);});e->r=_tmpE01;});}}struct _tuple41{void*f1;void*f2;};struct _tuple42{struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct _tuple43{struct Cyc_Absyn_Aggrfield*f1;struct Cyc_Absyn_Exp*f2;};
# 2656
static void Cyc_Toc_exp_to_c(struct Cyc_Toc_Env*nv,struct Cyc_Absyn_Exp*e){
if(e->topt == 0)
({struct Cyc_String_pa_PrintArg_struct _tmp543=({struct Cyc_String_pa_PrintArg_struct _tmpC8B;_tmpC8B.tag=0U,({struct _fat_ptr _tmpE02=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_exp2string(e);}));_tmpC8B.f1=_tmpE02;});_tmpC8B;});void*_tmp540[1U];_tmp540[0]=& _tmp543;({int(*_tmpE05)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp542)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos_loc;_tmp542;});unsigned _tmpE04=e->loc;struct _fat_ptr _tmpE03=({const char*_tmp541="exp_to_c: no type for %s";_tag_fat(_tmp541,sizeof(char),25U);});_tmpE05(_tmpE04,_tmpE03,_tag_fat(_tmp540,sizeof(void*),1U));});});{
# 2657
void*old_typ=(void*)_check_null(e->topt);
# 2661
int did_inserted_checks=0;
{void*_stmttmp3B=e->annot;void*_tmp544=_stmttmp3B;if(((struct Cyc_Absyn_EmptyAnnot_Absyn_AbsynAnnot_struct*)_tmp544)->tag == Cyc_Absyn_EmptyAnnot){_LL1: _LL2:
 goto _LL4;}else{if(((struct Cyc_InsertChecks_NoCheck_Absyn_AbsynAnnot_struct*)_tmp544)->tag == Cyc_InsertChecks_NoCheck){_LL3: _LL4:
 did_inserted_checks=1;goto _LL0;}else{_LL5: _LL6:
 goto _LL0;}}_LL0:;}
# 2667
{void*_stmttmp3C=e->r;void*_tmp545=_stmttmp3C;struct Cyc_Absyn_Stmt*_tmp546;struct _fat_ptr*_tmp548;struct Cyc_Absyn_Exp*_tmp547;struct Cyc_Absyn_Exp*_tmp54A;struct Cyc_Absyn_Exp*_tmp549;int _tmp550;int _tmp54F;struct Cyc_Absyn_Exp*_tmp54E;void**_tmp54D;struct Cyc_Absyn_Exp*_tmp54C;int _tmp54B;struct Cyc_Absyn_Datatypefield*_tmp553;struct Cyc_Absyn_Datatypedecl*_tmp552;struct Cyc_List_List*_tmp551;struct Cyc_Absyn_Aggrdecl*_tmp557;struct Cyc_List_List*_tmp556;struct Cyc_List_List*_tmp555;struct _tuple1**_tmp554;struct Cyc_List_List*_tmp559;void*_tmp558;int _tmp55C;void*_tmp55B;struct Cyc_Absyn_Exp*_tmp55A;int _tmp560;struct Cyc_Absyn_Exp*_tmp55F;struct Cyc_Absyn_Exp*_tmp55E;struct Cyc_Absyn_Vardecl*_tmp55D;struct Cyc_List_List*_tmp561;struct Cyc_List_List*_tmp562;struct Cyc_Absyn_Exp*_tmp564;struct Cyc_Absyn_Exp*_tmp563;struct Cyc_Absyn_Exp*_tmp565;int _tmp569;int _tmp568;struct _fat_ptr*_tmp567;struct Cyc_Absyn_Exp*_tmp566;int _tmp56D;int _tmp56C;struct _fat_ptr*_tmp56B;struct Cyc_Absyn_Exp*_tmp56A;struct Cyc_List_List*_tmp56F;void*_tmp56E;void*_tmp570;struct Cyc_Absyn_Exp*_tmp571;struct Cyc_Absyn_Exp*_tmp573;struct Cyc_Absyn_Exp*_tmp572;struct Cyc_Absyn_Exp*_tmp574;enum Cyc_Absyn_Coercion _tmp578;int _tmp577;struct Cyc_Absyn_Exp*_tmp576;void**_tmp575;struct Cyc_List_List*_tmp57A;struct Cyc_Absyn_Exp*_tmp579;struct Cyc_Absyn_Exp*_tmp57B;int _tmp57D;struct Cyc_Absyn_Exp*_tmp57C;struct Cyc_List_List*_tmp581;struct Cyc_List_List*_tmp580;struct Cyc_Absyn_Exp*_tmp57F;struct Cyc_Absyn_Exp*_tmp57E;struct _tuple0*_tmp587;struct Cyc_Absyn_VarargInfo*_tmp586;struct Cyc_List_List*_tmp585;int _tmp584;struct Cyc_List_List*_tmp583;struct Cyc_Absyn_Exp*_tmp582;struct Cyc_List_List*_tmp58B;struct Cyc_List_List*_tmp58A;struct Cyc_List_List*_tmp589;struct Cyc_Absyn_Exp*_tmp588;struct Cyc_Absyn_Exp*_tmp58D;struct Cyc_Absyn_Exp*_tmp58C;struct Cyc_Absyn_Exp*_tmp58F;struct Cyc_Absyn_Exp*_tmp58E;struct Cyc_Absyn_Exp*_tmp591;struct Cyc_Absyn_Exp*_tmp590;struct Cyc_Absyn_Exp*_tmp594;struct Cyc_Absyn_Exp*_tmp593;struct Cyc_Absyn_Exp*_tmp592;struct Cyc_Absyn_Exp*_tmp597;struct Cyc_Core_Opt*_tmp596;struct Cyc_Absyn_Exp*_tmp595;enum Cyc_Absyn_Incrementor _tmp599;struct Cyc_Absyn_Exp*_tmp598;struct Cyc_List_List*_tmp59B;enum Cyc_Absyn_Primop _tmp59A;struct Cyc_Absyn_Exp*_tmp59C;switch(*((int*)_tmp545)){case 2U: _LL8: _LL9:
# 2669
 e->r=(void*)& Cyc_Toc_zero_exp;
goto _LL7;case 0U: if(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp545)->f1).Null_c).tag == 1){_LLA: _LLB: {
# 2676
struct Cyc_Absyn_Exp*zero=({Cyc_Absyn_signed_int_exp(0,0U);});
if(({Cyc_Tcutil_is_fat_pointer_type(old_typ);})){
if(({Cyc_Toc_is_toplevel(nv);}))
({void*_tmpE06=({Cyc_Toc_make_toplevel_dyn_arr(old_typ,zero,zero);})->r;e->r=_tmpE06;});else{
# 2681
({void*_tmpE08=({struct Cyc_Absyn_Exp*_tmp59D[3U];_tmp59D[0]=zero,_tmp59D[1]=zero,_tmp59D[2]=zero;({struct Cyc_Absyn_Exp*_tmpE07=Cyc_Toc__tag_fat_e;Cyc_Toc_fncall_exp_r(_tmpE07,_tag_fat(_tmp59D,sizeof(struct Cyc_Absyn_Exp*),3U));});});e->r=_tmpE08;});}}else{
# 2683
e->r=(void*)& Cyc_Toc_zero_exp;}
# 2685
goto _LL7;}}else{_LLC: _LLD:
 goto _LL7;}case 1U: _LLE: _LLF:
 goto _LL7;case 42U: _LL10: _tmp59C=((struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct*)_tmp545)->f1;_LL11: {struct Cyc_Absyn_Exp*e1=_tmp59C;
({Cyc_Toc_exp_to_c(nv,e1);});goto _LL7;}case 3U: _LL12: _tmp59A=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp545)->f1;_tmp59B=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp545)->f2;_LL13: {enum Cyc_Absyn_Primop p=_tmp59A;struct Cyc_List_List*es=_tmp59B;
# 2691
struct Cyc_List_List*old_types=({({struct Cyc_List_List*(*_tmpE09)(void*(*f)(struct Cyc_Absyn_Exp*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp5ED)(void*(*f)(struct Cyc_Absyn_Exp*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(void*(*f)(struct Cyc_Absyn_Exp*),struct Cyc_List_List*x))Cyc_List_map;_tmp5ED;});_tmpE09(Cyc_Toc_get_cyc_type,es);});});
# 2693
({({void(*_tmpE0B)(void(*f)(struct Cyc_Toc_Env*,struct Cyc_Absyn_Exp*),struct Cyc_Toc_Env*env,struct Cyc_List_List*x)=({void(*_tmp59E)(void(*f)(struct Cyc_Toc_Env*,struct Cyc_Absyn_Exp*),struct Cyc_Toc_Env*env,struct Cyc_List_List*x)=(void(*)(void(*f)(struct Cyc_Toc_Env*,struct Cyc_Absyn_Exp*),struct Cyc_Toc_Env*env,struct Cyc_List_List*x))Cyc_List_iter_c;_tmp59E;});struct Cyc_Toc_Env*_tmpE0A=nv;_tmpE0B(Cyc_Toc_exp_to_c,_tmpE0A,es);});});
{enum Cyc_Absyn_Primop _tmp59F=p;switch(_tmp59F){case Cyc_Absyn_Numelts: _LL63: _LL64: {
# 2696
struct Cyc_Absyn_Exp*arg=(struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(es))->hd;
{void*_stmttmp3D=({Cyc_Tcutil_compress((void*)_check_null(arg->topt));});void*_tmp5A0=_stmttmp3D;void*_tmp5A4;void*_tmp5A3;void*_tmp5A2;void*_tmp5A1;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp5A0)->tag == 3U){_LL78: _tmp5A1=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp5A0)->f1).elt_type;_tmp5A2=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp5A0)->f1).ptr_atts).nullable;_tmp5A3=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp5A0)->f1).ptr_atts).bounds;_tmp5A4=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp5A0)->f1).ptr_atts).zero_term;_LL79: {void*elt_type=_tmp5A1;void*nbl=_tmp5A2;void*bound=_tmp5A3;void*zt=_tmp5A4;
# 2699
struct Cyc_Absyn_Exp*eopt=({struct Cyc_Absyn_Exp*(*_tmp5B0)(void*def,void*b)=Cyc_Tcutil_get_bounds_exp;void*_tmp5B1=({Cyc_Absyn_bounds_one();});void*_tmp5B2=bound;_tmp5B0(_tmp5B1,_tmp5B2);});
if(eopt == 0)
# 2702
({void*_tmpE0E=({struct Cyc_Absyn_Exp*_tmp5A5[2U];_tmp5A5[0]=(struct Cyc_Absyn_Exp*)es->hd,({
struct Cyc_Absyn_Exp*_tmpE0C=({struct Cyc_Absyn_Exp*(*_tmp5A6)(void*t,unsigned)=Cyc_Absyn_sizeoftype_exp;void*_tmp5A7=({Cyc_Toc_typ_to_c(elt_type);});unsigned _tmp5A8=0U;_tmp5A6(_tmp5A7,_tmp5A8);});_tmp5A5[1]=_tmpE0C;});({struct Cyc_Absyn_Exp*_tmpE0D=Cyc_Toc__get_fat_size_e;Cyc_Toc_fncall_exp_r(_tmpE0D,_tag_fat(_tmp5A5,sizeof(struct Cyc_Absyn_Exp*),2U));});});
# 2702
e->r=_tmpE0E;});else{
# 2705
struct Cyc_Absyn_Exp*e2=eopt;
# 2707
if(({Cyc_Tcutil_force_type2bool(0,zt);})){
struct Cyc_Absyn_Exp*function_e=({Cyc_Toc_getFunctionRemovePointer(& Cyc_Toc__get_zero_arr_size_functionSet,(struct Cyc_Absyn_Exp*)es->hd);});
# 2711
({void*_tmpE10=({struct Cyc_Absyn_Exp*_tmp5A9[2U];_tmp5A9[0]=(struct Cyc_Absyn_Exp*)es->hd,_tmp5A9[1]=e2;({struct Cyc_Absyn_Exp*_tmpE0F=function_e;Cyc_Toc_fncall_exp_r(_tmpE0F,_tag_fat(_tmp5A9,sizeof(struct Cyc_Absyn_Exp*),2U));});});e->r=_tmpE10;});}else{
if(({Cyc_Tcutil_force_type2bool(0,nbl);})){
if(!({Cyc_Evexp_c_can_eval(e2);}))
({void*_tmp5AA=0U;({unsigned _tmpE12=e->loc;struct _fat_ptr _tmpE11=({const char*_tmp5AB="can't calculate numelts";_tag_fat(_tmp5AB,sizeof(char),24U);});Cyc_Tcutil_terr(_tmpE12,_tmpE11,_tag_fat(_tmp5AA,sizeof(void*),0U));});});
# 2713
({void*_tmpE13=({void*(*_tmp5AC)(struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2,struct Cyc_Absyn_Exp*e3)=Cyc_Toc_conditional_exp_r;struct Cyc_Absyn_Exp*_tmp5AD=arg;struct Cyc_Absyn_Exp*_tmp5AE=e2;struct Cyc_Absyn_Exp*_tmp5AF=({Cyc_Absyn_uint_exp(0U,0U);});_tmp5AC(_tmp5AD,_tmp5AE,_tmp5AF);});e->r=_tmpE13;});}else{
# 2718
e->r=e2->r;goto _LL77;}}}
# 2721
goto _LL77;}}else{_LL7A: _LL7B:
({struct Cyc_String_pa_PrintArg_struct _tmp5B6=({struct Cyc_String_pa_PrintArg_struct _tmpC8C;_tmpC8C.tag=0U,({
struct _fat_ptr _tmpE14=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string((void*)_check_null(arg->topt));}));_tmpC8C.f1=_tmpE14;});_tmpC8C;});void*_tmp5B3[1U];_tmp5B3[0]=& _tmp5B6;({int(*_tmpE16)(struct _fat_ptr fmt,struct _fat_ptr ap)=({
# 2722
int(*_tmp5B5)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmp5B5;});struct _fat_ptr _tmpE15=({const char*_tmp5B4="numelts primop applied to non-pointer %s";_tag_fat(_tmp5B4,sizeof(char),41U);});_tmpE16(_tmpE15,_tag_fat(_tmp5B3,sizeof(void*),1U));});});}_LL77:;}
# 2725
goto _LL62;}case Cyc_Absyn_Plus: _LL65: _LL66:
# 2730
 if(({Cyc_Toc_is_toplevel(nv);}))
({void*_tmp5B7=0U;({int(*_tmpE1A)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp5BB)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_unimp;_tmp5BB;});struct _fat_ptr _tmpE19=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp5BA=({struct Cyc_String_pa_PrintArg_struct _tmpC8D;_tmpC8D.tag=0U,({struct _fat_ptr _tmpE17=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_exp2string(e);}));_tmpC8D.f1=_tmpE17;});_tmpC8D;});void*_tmp5B8[1U];_tmp5B8[0]=& _tmp5BA;({struct _fat_ptr _tmpE18=({const char*_tmp5B9="can't do pointer arithmetic at top-level. Expression:\n%s\n";_tag_fat(_tmp5B9,sizeof(char),58U);});Cyc_aprintf(_tmpE18,_tag_fat(_tmp5B8,sizeof(void*),1U));});});_tmpE1A(_tmpE19,_tag_fat(_tmp5B7,sizeof(void*),0U));});});
# 2730
{void*_stmttmp3E=({Cyc_Tcutil_compress((void*)((struct Cyc_List_List*)_check_null(old_types))->hd);});void*_tmp5BC=_stmttmp3E;void*_tmp5BF;void*_tmp5BE;void*_tmp5BD;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp5BC)->tag == 3U){_LL7D: _tmp5BD=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp5BC)->f1).elt_type;_tmp5BE=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp5BC)->f1).ptr_atts).bounds;_tmp5BF=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp5BC)->f1).ptr_atts).zero_term;_LL7E: {void*elt_type=_tmp5BD;void*b=_tmp5BE;void*zt=_tmp5BF;
# 2734
struct Cyc_Absyn_Exp*eopt=({struct Cyc_Absyn_Exp*(*_tmp5C5)(void*def,void*b)=Cyc_Tcutil_get_bounds_exp;void*_tmp5C6=({Cyc_Absyn_bounds_one();});void*_tmp5C7=b;_tmp5C5(_tmp5C6,_tmp5C7);});
struct Cyc_Absyn_Exp*e1=(struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(es))->hd;
struct Cyc_Absyn_Exp*e2=(struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(es->tl))->hd;
if(eopt == 0)
({void*_tmpE1D=({struct Cyc_Absyn_Exp*_tmp5C0[3U];_tmp5C0[0]=e1,({
struct Cyc_Absyn_Exp*_tmpE1B=({struct Cyc_Absyn_Exp*(*_tmp5C1)(void*t,unsigned)=Cyc_Absyn_sizeoftype_exp;void*_tmp5C2=({Cyc_Toc_typ_to_c(elt_type);});unsigned _tmp5C3=0U;_tmp5C1(_tmp5C2,_tmp5C3);});_tmp5C0[1]=_tmpE1B;}),_tmp5C0[2]=e2;({struct Cyc_Absyn_Exp*_tmpE1C=Cyc_Toc__fat_ptr_plus_e;Cyc_Toc_fncall_exp_r(_tmpE1C,_tag_fat(_tmp5C0,sizeof(struct Cyc_Absyn_Exp*),3U));});});
# 2738
e->r=_tmpE1D;});else{
# 2740
if(({Cyc_Tcutil_force_type2bool(0,zt);}))
({void*_tmpE1F=({struct Cyc_Absyn_Exp*_tmp5C4[3U];_tmp5C4[0]=e1,_tmp5C4[1]=eopt,_tmp5C4[2]=e2;({struct Cyc_Absyn_Exp*_tmpE1E=({Cyc_Toc_getFunctionRemovePointer(& Cyc_Toc__zero_arr_plus_functionSet,e1);});Cyc_Toc_fncall_exp_r(_tmpE1E,_tag_fat(_tmp5C4,sizeof(struct Cyc_Absyn_Exp*),3U));});});e->r=_tmpE1F;});}
# 2737
goto _LL7C;}}else{_LL7F: _LL80:
# 2743
 goto _LL7C;}_LL7C:;}
# 2745
goto _LL62;case Cyc_Absyn_Minus: _LL67: _LL68: {
# 2750
void*elt_type=Cyc_Absyn_void_type;
if(({Cyc_Tcutil_is_fat_pointer_type_elt((void*)((struct Cyc_List_List*)_check_null(old_types))->hd,& elt_type);})){
struct Cyc_Absyn_Exp*e1=(struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(es))->hd;
struct Cyc_Absyn_Exp*e2=(struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(es->tl))->hd;
if(({Cyc_Tcutil_is_fat_pointer_type((void*)((struct Cyc_List_List*)_check_null(old_types->tl))->hd);})){
({void*_tmpE20=({void*(*_tmp5C8)(struct Cyc_Absyn_Exp*e,struct _fat_ptr*n)=Cyc_Toc_aggrmember_exp_r;struct Cyc_Absyn_Exp*_tmp5C9=({Cyc_Absyn_new_exp(e1->r,0U);});struct _fat_ptr*_tmp5CA=Cyc_Toc_curr_sp;_tmp5C8(_tmp5C9,_tmp5CA);});e1->r=_tmpE20;});
({void*_tmpE21=({void*(*_tmp5CB)(struct Cyc_Absyn_Exp*e,struct _fat_ptr*n)=Cyc_Toc_aggrmember_exp_r;struct Cyc_Absyn_Exp*_tmp5CC=({Cyc_Absyn_new_exp(e2->r,0U);});struct _fat_ptr*_tmp5CD=Cyc_Toc_curr_sp;_tmp5CB(_tmp5CC,_tmp5CD);});e2->r=_tmpE21;});
({void*_tmpE23=({void*_tmpE22=({Cyc_Absyn_cstar_type(Cyc_Absyn_uchar_type,Cyc_Toc_mt_tq);});e2->topt=_tmpE22;});e1->topt=_tmpE23;});
({void*_tmpE24=({struct Cyc_Absyn_Exp*(*_tmp5CE)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_divide_exp;struct Cyc_Absyn_Exp*_tmp5CF=({Cyc_Absyn_copy_exp(e);});struct Cyc_Absyn_Exp*_tmp5D0=({struct Cyc_Absyn_Exp*(*_tmp5D1)(void*t,unsigned)=Cyc_Absyn_sizeoftype_exp;void*_tmp5D2=({Cyc_Toc_typ_to_c(elt_type);});unsigned _tmp5D3=0U;_tmp5D1(_tmp5D2,_tmp5D3);});unsigned _tmp5D4=0U;_tmp5CE(_tmp5CF,_tmp5D0,_tmp5D4);})->r;e->r=_tmpE24;});}else{
# 2761
({void*_tmpE28=({struct Cyc_Absyn_Exp*_tmp5D5[3U];_tmp5D5[0]=e1,({
struct Cyc_Absyn_Exp*_tmpE26=({struct Cyc_Absyn_Exp*(*_tmp5D6)(void*t,unsigned)=Cyc_Absyn_sizeoftype_exp;void*_tmp5D7=({Cyc_Toc_typ_to_c(elt_type);});unsigned _tmp5D8=0U;_tmp5D6(_tmp5D7,_tmp5D8);});_tmp5D5[1]=_tmpE26;}),({
struct Cyc_Absyn_Exp*_tmpE25=({Cyc_Absyn_prim1_exp(Cyc_Absyn_Minus,e2,0U);});_tmp5D5[2]=_tmpE25;});({struct Cyc_Absyn_Exp*_tmpE27=Cyc_Toc__fat_ptr_plus_e;Cyc_Toc_fncall_exp_r(_tmpE27,_tag_fat(_tmp5D5,sizeof(struct Cyc_Absyn_Exp*),3U));});});
# 2761
e->r=_tmpE28;});}}
# 2751
goto _LL62;}case Cyc_Absyn_Eq: _LL69: _LL6A:
# 2766
 goto _LL6C;case Cyc_Absyn_Neq: _LL6B: _LL6C: goto _LL6E;case Cyc_Absyn_Gt: _LL6D: _LL6E: goto _LL70;case Cyc_Absyn_Gte: _LL6F: _LL70: goto _LL72;case Cyc_Absyn_Lt: _LL71: _LL72: goto _LL74;case Cyc_Absyn_Lte: _LL73: _LL74: {
# 2768
struct Cyc_Absyn_Exp*e1=(struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(es))->hd;
struct Cyc_Absyn_Exp*e2=(struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(es->tl))->hd;
void*t1=(void*)((struct Cyc_List_List*)_check_null(old_types))->hd;
void*t2=(void*)((struct Cyc_List_List*)_check_null(old_types->tl))->hd;
void*elt_type=Cyc_Absyn_void_type;
if(({Cyc_Tcutil_is_fat_pointer_type_elt(t1,& elt_type);})){
void*t=({void*(*_tmp5E0)(void*,struct Cyc_Absyn_Tqual)=Cyc_Absyn_cstar_type;void*_tmp5E1=({Cyc_Toc_typ_to_c(elt_type);});struct Cyc_Absyn_Tqual _tmp5E2=Cyc_Toc_mt_tq;_tmp5E0(_tmp5E1,_tmp5E2);});
({void*_tmpE29=({void*(*_tmp5D9)(void*t,struct Cyc_Absyn_Exp*e)=Cyc_Toc_cast_it_r;void*_tmp5DA=t;struct Cyc_Absyn_Exp*_tmp5DB=({struct Cyc_Absyn_Exp*(*_tmp5DC)(struct Cyc_Absyn_Exp*e,struct _fat_ptr*f,unsigned loc)=Cyc_Toc_member_exp;struct Cyc_Absyn_Exp*_tmp5DD=({Cyc_Absyn_new_exp(e1->r,0U);});struct _fat_ptr*_tmp5DE=Cyc_Toc_curr_sp;unsigned _tmp5DF=0U;_tmp5DC(_tmp5DD,_tmp5DE,_tmp5DF);});_tmp5D9(_tmp5DA,_tmp5DB);});e1->r=_tmpE29;});
e1->topt=t;}
# 2773
if(({Cyc_Tcutil_is_fat_pointer_type(t2);})){
# 2779
void*t=({void*(*_tmp5EA)(void*,struct Cyc_Absyn_Tqual)=Cyc_Absyn_cstar_type;void*_tmp5EB=({Cyc_Toc_typ_to_c(elt_type);});struct Cyc_Absyn_Tqual _tmp5EC=Cyc_Toc_mt_tq;_tmp5EA(_tmp5EB,_tmp5EC);});
({void*_tmpE2A=({void*(*_tmp5E3)(void*t,struct Cyc_Absyn_Exp*e)=Cyc_Toc_cast_it_r;void*_tmp5E4=t;struct Cyc_Absyn_Exp*_tmp5E5=({struct Cyc_Absyn_Exp*(*_tmp5E6)(struct Cyc_Absyn_Exp*e,struct _fat_ptr*f,unsigned loc)=Cyc_Toc_member_exp;struct Cyc_Absyn_Exp*_tmp5E7=({Cyc_Absyn_new_exp(e2->r,0U);});struct _fat_ptr*_tmp5E8=Cyc_Toc_curr_sp;unsigned _tmp5E9=0U;_tmp5E6(_tmp5E7,_tmp5E8,_tmp5E9);});_tmp5E3(_tmp5E4,_tmp5E5);});e2->r=_tmpE2A;});
e2->topt=t;}
# 2773
goto _LL62;}default: _LL75: _LL76:
# 2784
 goto _LL62;}_LL62:;}
# 2786
goto _LL7;}case 5U: _LL14: _tmp598=((struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*)_tmp545)->f1;_tmp599=((struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*)_tmp545)->f2;_LL15: {struct Cyc_Absyn_Exp*e2=_tmp598;enum Cyc_Absyn_Incrementor incr=_tmp599;
# 2788
void*e2_cyc_typ=(void*)_check_null(e2->topt);
# 2797 "toc.cyc"
void*ignore_typ=Cyc_Absyn_void_type;
int ignore_bool=0;
int ignore_int=0;
struct _fat_ptr incr_str=({const char*_tmp638="increment";_tag_fat(_tmp638,sizeof(char),10U);});
if((int)incr == (int)2U ||(int)incr == (int)3U)incr_str=({const char*_tmp5EE="decrement";_tag_fat(_tmp5EE,sizeof(char),10U);});if(({Cyc_Tcutil_is_zero_ptr_deref(e2,& ignore_typ,& ignore_bool,& ignore_typ);}))
# 2803
({struct Cyc_String_pa_PrintArg_struct _tmp5F2=({struct Cyc_String_pa_PrintArg_struct _tmpC8E;_tmpC8E.tag=0U,_tmpC8E.f1=(struct _fat_ptr)((struct _fat_ptr)incr_str);_tmpC8E;});void*_tmp5EF[1U];_tmp5EF[0]=& _tmp5F2;({int(*_tmpE2D)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp5F1)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos_loc;_tmp5F1;});unsigned _tmpE2C=e->loc;struct _fat_ptr _tmpE2B=({const char*_tmp5F0="in-place %s is not supported when dereferencing a zero-terminated pointer";_tag_fat(_tmp5F0,sizeof(char),74U);});_tmpE2D(_tmpE2C,_tmpE2B,_tag_fat(_tmp5EF,sizeof(void*),1U));});});
# 2801
if(({Cyc_Toc_is_tagged_union_project(e2,& ignore_int,& ignore_typ,1);})){
# 2806
struct Cyc_Absyn_Exp*one=({Cyc_Absyn_signed_int_exp(1,0U);});
enum Cyc_Absyn_Primop op;
one->topt=Cyc_Absyn_sint_type;
{enum Cyc_Absyn_Incrementor _tmp5F3=incr;switch(_tmp5F3){case Cyc_Absyn_PreInc: _LL82: _LL83:
 op=0U;goto _LL81;case Cyc_Absyn_PreDec: _LL84: _LL85:
 op=2U;goto _LL81;default: _LL86: _LL87:
({struct Cyc_String_pa_PrintArg_struct _tmp5F7=({struct Cyc_String_pa_PrintArg_struct _tmpC8F;_tmpC8F.tag=0U,_tmpC8F.f1=(struct _fat_ptr)((struct _fat_ptr)incr_str);_tmpC8F;});void*_tmp5F4[1U];_tmp5F4[0]=& _tmp5F7;({int(*_tmpE30)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp5F6)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos_loc;_tmp5F6;});unsigned _tmpE2F=e->loc;struct _fat_ptr _tmpE2E=({const char*_tmp5F5="in-place post%s is not supported on @tagged union members";_tag_fat(_tmp5F5,sizeof(char),58U);});_tmpE30(_tmpE2F,_tmpE2E,_tag_fat(_tmp5F4,sizeof(void*),1U));});});}_LL81:;}
# 2815
({void*_tmpE32=(void*)({struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*_tmp5F9=_cycalloc(sizeof(*_tmp5F9));_tmp5F9->tag=4U,_tmp5F9->f1=e2,({struct Cyc_Core_Opt*_tmpE31=({struct Cyc_Core_Opt*_tmp5F8=_cycalloc(sizeof(*_tmp5F8));_tmp5F8->v=(void*)op;_tmp5F8;});_tmp5F9->f2=_tmpE31;}),_tmp5F9->f3=one;_tmp5F9;});e->r=_tmpE32;});
({Cyc_Toc_exp_to_c(nv,e);});
return;}
# 2801
({Cyc_Toc_set_lhs(nv,1);});
# 2820
({Cyc_Toc_exp_to_c(nv,e2);});
({Cyc_Toc_set_lhs(nv,0);});{
# 2824
void*elt_typ=Cyc_Absyn_void_type;
if(({Cyc_Tcutil_is_fat_pointer_type_elt(old_typ,& elt_typ);})){
struct Cyc_Absyn_Exp*fn_e;
int change=1;
fn_e=((int)incr == (int)1U ||(int)incr == (int)3U)?Cyc_Toc__fat_ptr_inplace_plus_post_e: Cyc_Toc__fat_ptr_inplace_plus_e;
# 2830
if((int)incr == (int)2U ||(int)incr == (int)3U)
change=-1;
# 2830
({void*_tmpE37=({struct Cyc_Absyn_Exp*_tmp5FA[3U];({
# 2832
struct Cyc_Absyn_Exp*_tmpE35=({Cyc_Toc_push_address_exp(e2);});_tmp5FA[0]=_tmpE35;}),({
struct Cyc_Absyn_Exp*_tmpE34=({struct Cyc_Absyn_Exp*(*_tmp5FB)(void*t,unsigned)=Cyc_Absyn_sizeoftype_exp;void*_tmp5FC=({Cyc_Toc_typ_to_c(elt_typ);});unsigned _tmp5FD=0U;_tmp5FB(_tmp5FC,_tmp5FD);});_tmp5FA[1]=_tmpE34;}),({
struct Cyc_Absyn_Exp*_tmpE33=({Cyc_Absyn_signed_int_exp(change,0U);});_tmp5FA[2]=_tmpE33;});({struct Cyc_Absyn_Exp*_tmpE36=fn_e;Cyc_Toc_fncall_exp_r(_tmpE36,_tag_fat(_tmp5FA,sizeof(struct Cyc_Absyn_Exp*),3U));});});
# 2830
e->r=_tmpE37;});}else{
# 2835
if(({Cyc_Tcutil_is_zero_pointer_type_elt(old_typ,& elt_typ);})){
# 2841
did_inserted_checks=1;
if((int)incr != (int)1U){
struct _tuple1*x=({Cyc_Toc_temp_var();});
void*t=({void*(*_tmp633)(void*,struct Cyc_Absyn_Tqual)=Cyc_Absyn_cstar_type;void*_tmp634=({Cyc_Toc_typ_to_c(old_typ);});struct Cyc_Absyn_Tqual _tmp635=Cyc_Toc_mt_tq;_tmp633(_tmp634,_tmp635);});
struct Cyc_Absyn_Exp*xexp=({Cyc_Toc_push_address_exp(e2);});
struct Cyc_Absyn_Exp*testexp=({struct Cyc_Absyn_Exp*(*_tmp629)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_neq_exp;struct Cyc_Absyn_Exp*_tmp62A=({struct Cyc_Absyn_Exp*(*_tmp62B)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_deref_exp;struct Cyc_Absyn_Exp*_tmp62C=({struct Cyc_Absyn_Exp*(*_tmp62D)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_deref_exp;struct Cyc_Absyn_Exp*_tmp62E=({Cyc_Absyn_var_exp(x,0U);});unsigned _tmp62F=0U;_tmp62D(_tmp62E,_tmp62F);});unsigned _tmp630=0U;_tmp62B(_tmp62C,_tmp630);});struct Cyc_Absyn_Exp*_tmp631=({Cyc_Absyn_int_exp(Cyc_Absyn_None,0,0U);});unsigned _tmp632=0U;_tmp629(_tmp62A,_tmp631,_tmp632);});
# 2848
if(({Cyc_Toc_do_null_check(e);}))
testexp=({struct Cyc_Absyn_Exp*(*_tmp5FE)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_and_exp;struct Cyc_Absyn_Exp*_tmp5FF=({struct Cyc_Absyn_Exp*(*_tmp600)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_neq_exp;struct Cyc_Absyn_Exp*_tmp601=({struct Cyc_Absyn_Exp*(*_tmp602)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_deref_exp;struct Cyc_Absyn_Exp*_tmp603=({Cyc_Absyn_var_exp(x,0U);});unsigned _tmp604=0U;_tmp602(_tmp603,_tmp604);});struct Cyc_Absyn_Exp*_tmp605=({Cyc_Absyn_int_exp(Cyc_Absyn_None,0,0U);});unsigned _tmp606=0U;_tmp600(_tmp601,_tmp605,_tmp606);});struct Cyc_Absyn_Exp*_tmp607=testexp;unsigned _tmp608=0U;_tmp5FE(_tmp5FF,_tmp607,_tmp608);});{
# 2848
struct Cyc_Absyn_Stmt*s=({struct Cyc_Absyn_Stmt*(*_tmp616)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_ifthenelse_stmt;struct Cyc_Absyn_Exp*_tmp617=testexp;struct Cyc_Absyn_Stmt*_tmp618=({struct Cyc_Absyn_Stmt*(*_tmp619)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_exp_stmt;struct Cyc_Absyn_Exp*_tmp61A=({struct Cyc_Absyn_Exp*(*_tmp61B)(struct Cyc_Absyn_Exp*,enum Cyc_Absyn_Incrementor,unsigned)=Cyc_Absyn_increment_exp;struct Cyc_Absyn_Exp*_tmp61C=({struct Cyc_Absyn_Exp*(*_tmp61D)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_deref_exp;struct Cyc_Absyn_Exp*_tmp61E=({Cyc_Absyn_var_exp(x,0U);});unsigned _tmp61F=0U;_tmp61D(_tmp61E,_tmp61F);});enum Cyc_Absyn_Incrementor _tmp620=Cyc_Absyn_PreInc;unsigned _tmp621=0U;_tmp61B(_tmp61C,_tmp620,_tmp621);});unsigned _tmp622=0U;_tmp619(_tmp61A,_tmp622);});struct Cyc_Absyn_Stmt*_tmp623=({struct Cyc_Absyn_Stmt*(*_tmp624)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_exp_stmt;struct Cyc_Absyn_Exp*_tmp625=({void*_tmp626=0U;({struct Cyc_Absyn_Exp*_tmpE38=Cyc_Toc__throw_arraybounds_e;Cyc_Toc_fncall_exp_dl(_tmpE38,_tag_fat(_tmp626,sizeof(struct Cyc_Absyn_Exp*),0U));});});unsigned _tmp627=0U;_tmp624(_tmp625,_tmp627);});unsigned _tmp628=0U;_tmp616(_tmp617,_tmp618,_tmp623,_tmp628);});
# 2854
s=({struct Cyc_Absyn_Stmt*(*_tmp609)(struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_seq_stmt;struct Cyc_Absyn_Stmt*_tmp60A=s;struct Cyc_Absyn_Stmt*_tmp60B=({struct Cyc_Absyn_Stmt*(*_tmp60C)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_exp_stmt;struct Cyc_Absyn_Exp*_tmp60D=({struct Cyc_Absyn_Exp*(*_tmp60E)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_deref_exp;struct Cyc_Absyn_Exp*_tmp60F=({Cyc_Absyn_var_exp(x,0U);});unsigned _tmp610=0U;_tmp60E(_tmp60F,_tmp610);});unsigned _tmp611=0U;_tmp60C(_tmp60D,_tmp611);});unsigned _tmp612=0U;_tmp609(_tmp60A,_tmp60B,_tmp612);});
({void*_tmpE39=({struct Cyc_Absyn_Exp*(*_tmp613)(struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_stmt_exp;struct Cyc_Absyn_Stmt*_tmp614=({Cyc_Absyn_declare_stmt(x,t,xexp,s,0U);});unsigned _tmp615=0U;_tmp613(_tmp614,_tmp615);})->r;e->r=_tmpE39;});}}else{
# 2858
struct Cyc_Toc_functionSet*fnSet=& Cyc_Toc__zero_arr_inplace_plus_post_functionSet;
struct Cyc_Absyn_Exp*fn_e=({Cyc_Toc_getFunctionRemovePointer(fnSet,e2);});
({void*_tmpE3D=({struct Cyc_Absyn_Exp*_tmp636[2U];({struct Cyc_Absyn_Exp*_tmpE3B=({Cyc_Toc_push_address_exp(e2);});_tmp636[0]=_tmpE3B;}),({struct Cyc_Absyn_Exp*_tmpE3A=({Cyc_Absyn_signed_int_exp(1,0U);});_tmp636[1]=_tmpE3A;});({struct Cyc_Absyn_Exp*_tmpE3C=fn_e;Cyc_Toc_fncall_exp_r(_tmpE3C,_tag_fat(_tmp636,sizeof(struct Cyc_Absyn_Exp*),2U));});});e->r=_tmpE3D;});}}else{
# 2862
if(elt_typ == Cyc_Absyn_void_type && !({Cyc_Absyn_is_lvalue(e2);})){
({({void(*_tmpE3F)(struct Cyc_Absyn_Exp*e1,struct Cyc_List_List*fs,struct Cyc_Absyn_Exp*(*f)(struct Cyc_Absyn_Exp*,enum Cyc_Absyn_Incrementor),enum Cyc_Absyn_Incrementor f_env)=({void(*_tmp637)(struct Cyc_Absyn_Exp*e1,struct Cyc_List_List*fs,struct Cyc_Absyn_Exp*(*f)(struct Cyc_Absyn_Exp*,enum Cyc_Absyn_Incrementor),enum Cyc_Absyn_Incrementor f_env)=(void(*)(struct Cyc_Absyn_Exp*e1,struct Cyc_List_List*fs,struct Cyc_Absyn_Exp*(*f)(struct Cyc_Absyn_Exp*,enum Cyc_Absyn_Incrementor),enum Cyc_Absyn_Incrementor f_env))Cyc_Toc_lvalue_assign;_tmp637;});struct Cyc_Absyn_Exp*_tmpE3E=e2;_tmpE3F(_tmpE3E,0,Cyc_Toc_incr_lvalue,incr);});});
e->r=e2->r;}}}
# 2825
goto _LL7;}}case 4U: _LL16: _tmp595=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp545)->f1;_tmp596=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp545)->f2;_tmp597=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp545)->f3;_LL17: {struct Cyc_Absyn_Exp*e1=_tmp595;struct Cyc_Core_Opt*popt=_tmp596;struct Cyc_Absyn_Exp*e2=_tmp597;
# 2885 "toc.cyc"
void*e1_old_typ=(void*)_check_null(e1->topt);
void*e2_old_typ=(void*)_check_null(e2->topt);
int f_tag=0;
void*tagged_member_struct_type=Cyc_Absyn_void_type;
if(({Cyc_Toc_is_tagged_union_project(e1,& f_tag,& tagged_member_struct_type,1);})){
({Cyc_Toc_set_lhs(nv,1);});
({Cyc_Toc_exp_to_c(nv,e1);});
({Cyc_Toc_set_lhs(nv,0);});
({Cyc_Toc_exp_to_c(nv,e2);});
({void*_tmpE40=({Cyc_Toc_tagged_union_assignop(e1,e1_old_typ,popt,e2,e2_old_typ,f_tag,tagged_member_struct_type);});e->r=_tmpE40;});
# 2896
goto _LL7;}{
# 2889
void*ptr_type=Cyc_Absyn_void_type;
# 2899
void*elt_type=Cyc_Absyn_void_type;
int is_fat=0;
if(({Cyc_Tcutil_is_zero_ptr_deref(e1,& ptr_type,& is_fat,& elt_type);})){
({Cyc_Toc_zero_ptr_assign_to_c(nv,e,e1,popt,e2,ptr_type,is_fat,elt_type);});
# 2904
return;}{
# 2901
int e1_poly=({Cyc_Toc_is_poly_project(e1);});
# 2909
({Cyc_Toc_set_lhs(nv,1);});
({Cyc_Toc_exp_to_c(nv,e1);});
({Cyc_Toc_set_lhs(nv,0);});
({Cyc_Toc_exp_to_c(nv,e2);});{
# 2914
int done=0;
if(popt != 0){
void*elt_typ=Cyc_Absyn_void_type;
if(({Cyc_Tcutil_is_fat_pointer_type_elt(old_typ,& elt_typ);})){
struct Cyc_Absyn_Exp*change;
{enum Cyc_Absyn_Primop _stmttmp3F=(enum Cyc_Absyn_Primop)popt->v;enum Cyc_Absyn_Primop _tmp639=_stmttmp3F;switch(_tmp639){case Cyc_Absyn_Plus: _LL89: _LL8A:
 change=e2;goto _LL88;case Cyc_Absyn_Minus: _LL8B: _LL8C:
 change=({Cyc_Absyn_prim1_exp(Cyc_Absyn_Minus,e2,0U);});goto _LL88;default: _LL8D: _LL8E:
({void*_tmp63A=0U;({int(*_tmpE42)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp63C)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmp63C;});struct _fat_ptr _tmpE41=({const char*_tmp63B="bad t ? pointer arithmetic";_tag_fat(_tmp63B,sizeof(char),27U);});_tmpE42(_tmpE41,_tag_fat(_tmp63A,sizeof(void*),0U));});});}_LL88:;}
# 2924
done=1;{
# 2926
struct Cyc_Absyn_Exp*fn_e=Cyc_Toc__fat_ptr_inplace_plus_e;
({void*_tmpE46=({struct Cyc_Absyn_Exp*_tmp63D[3U];({struct Cyc_Absyn_Exp*_tmpE44=({Cyc_Toc_push_address_exp(e1);});_tmp63D[0]=_tmpE44;}),({
struct Cyc_Absyn_Exp*_tmpE43=({struct Cyc_Absyn_Exp*(*_tmp63E)(void*t,unsigned)=Cyc_Absyn_sizeoftype_exp;void*_tmp63F=({Cyc_Toc_typ_to_c(elt_typ);});unsigned _tmp640=0U;_tmp63E(_tmp63F,_tmp640);});_tmp63D[1]=_tmpE43;}),_tmp63D[2]=change;({struct Cyc_Absyn_Exp*_tmpE45=fn_e;Cyc_Toc_fncall_exp_r(_tmpE45,_tag_fat(_tmp63D,sizeof(struct Cyc_Absyn_Exp*),3U));});});
# 2927
e->r=_tmpE46;});}}else{
# 2930
if(({Cyc_Tcutil_is_zero_pointer_type_elt(old_typ,& elt_typ);})){
# 2933
enum Cyc_Absyn_Primop _stmttmp40=(enum Cyc_Absyn_Primop)popt->v;enum Cyc_Absyn_Primop _tmp641=_stmttmp40;if(_tmp641 == Cyc_Absyn_Plus){_LL90: _LL91:
# 2935
 done=1;
({void*_tmpE48=({struct Cyc_Absyn_Exp*_tmp642[2U];_tmp642[0]=e1,_tmp642[1]=e2;({struct Cyc_Absyn_Exp*_tmpE47=({Cyc_Toc_getFunctionRemovePointer(& Cyc_Toc__zero_arr_inplace_plus_functionSet,e1);});Cyc_Toc_fncall_exp_r(_tmpE47,_tag_fat(_tmp642,sizeof(struct Cyc_Absyn_Exp*),2U));});});e->r=_tmpE48;});
goto _LL8F;}else{_LL92: _LL93:
({void*_tmp643=0U;({int(*_tmpE4A)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp645)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmp645;});struct _fat_ptr _tmpE49=({const char*_tmp644="bad zero-terminated pointer arithmetic";_tag_fat(_tmp644,sizeof(char),39U);});_tmpE4A(_tmpE49,_tag_fat(_tmp643,sizeof(void*),0U));});});}_LL8F:;}}}
# 2915
if(!done){
# 2944
if(e1_poly)
({void*_tmpE4B=({void*(*_tmp646)(void*t,struct Cyc_Absyn_Exp*e)=Cyc_Toc_cast_it_r;void*_tmp647=({Cyc_Toc_void_star_type();});struct Cyc_Absyn_Exp*_tmp648=({Cyc_Absyn_new_exp(e2->r,0U);});_tmp646(_tmp647,_tmp648);});e2->r=_tmpE4B;});
# 2944
if(!({Cyc_Absyn_is_lvalue(e1);})){
# 2951
({({void(*_tmpE4D)(struct Cyc_Absyn_Exp*e1,struct Cyc_List_List*fs,struct Cyc_Absyn_Exp*(*f)(struct Cyc_Absyn_Exp*,struct _tuple35*),struct _tuple35*f_env)=({void(*_tmp649)(struct Cyc_Absyn_Exp*e1,struct Cyc_List_List*fs,struct Cyc_Absyn_Exp*(*f)(struct Cyc_Absyn_Exp*,struct _tuple35*),struct _tuple35*f_env)=(void(*)(struct Cyc_Absyn_Exp*e1,struct Cyc_List_List*fs,struct Cyc_Absyn_Exp*(*f)(struct Cyc_Absyn_Exp*,struct _tuple35*),struct _tuple35*f_env))Cyc_Toc_lvalue_assign;_tmp649;});struct Cyc_Absyn_Exp*_tmpE4C=e1;_tmpE4D(_tmpE4C,0,Cyc_Toc_assignop_lvalue,({struct _tuple35*_tmp64A=_cycalloc(sizeof(*_tmp64A));_tmp64A->f1=popt,_tmp64A->f2=e2;_tmp64A;}));});});
e->r=e1->r;}}
# 2915
goto _LL7;}}}}case 6U: _LL18: _tmp592=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp545)->f1;_tmp593=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp545)->f2;_tmp594=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp545)->f3;_LL19: {struct Cyc_Absyn_Exp*e1=_tmp592;struct Cyc_Absyn_Exp*e2=_tmp593;struct Cyc_Absyn_Exp*e3=_tmp594;
# 2957
({Cyc_Toc_exp_to_c(nv,e1);});
({Cyc_Toc_exp_to_c(nv,e2);});
({Cyc_Toc_exp_to_c(nv,e3);});
goto _LL7;}case 7U: _LL1A: _tmp590=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp545)->f1;_tmp591=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp545)->f2;_LL1B: {struct Cyc_Absyn_Exp*e1=_tmp590;struct Cyc_Absyn_Exp*e2=_tmp591;
_tmp58E=e1;_tmp58F=e2;goto _LL1D;}case 8U: _LL1C: _tmp58E=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp545)->f1;_tmp58F=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp545)->f2;_LL1D: {struct Cyc_Absyn_Exp*e1=_tmp58E;struct Cyc_Absyn_Exp*e2=_tmp58F;
_tmp58C=e1;_tmp58D=e2;goto _LL1F;}case 9U: _LL1E: _tmp58C=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp545)->f1;_tmp58D=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp545)->f2;_LL1F: {struct Cyc_Absyn_Exp*e1=_tmp58C;struct Cyc_Absyn_Exp*e2=_tmp58D;
# 2964
({Cyc_Toc_exp_to_c(nv,e1);});
({Cyc_Toc_exp_to_c(nv,e2);});
goto _LL7;}case 10U: if(((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp545)->f3 == 0){_LL20: _tmp588=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp545)->f1;_tmp589=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp545)->f2;_tmp58A=(((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp545)->f5)->f1;_tmp58B=(((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp545)->f5)->f2;_LL21: {struct Cyc_Absyn_Exp*e1=_tmp588;struct Cyc_List_List*es=_tmp589;struct Cyc_List_List*ef_in=_tmp58A;struct Cyc_List_List*ef_out=_tmp58B;
# 2968
did_inserted_checks=1;
({void(*_tmp64B)(struct Cyc_Toc_Env*nv,struct Cyc_Absyn_Exp*e,struct Cyc_Absyn_Exp*e1,struct Cyc_List_List*es,struct Cyc_List_List*ef_in,struct Cyc_List_List*ef_out,int need_check,int is_thread)=Cyc_Toc_makeFnCall;struct Cyc_Toc_Env*_tmp64C=nv;struct Cyc_Absyn_Exp*_tmp64D=e;struct Cyc_Absyn_Exp*_tmp64E=e1;struct Cyc_List_List*_tmp64F=es;struct Cyc_List_List*_tmp650=ef_in;struct Cyc_List_List*_tmp651=ef_out;int _tmp652=({Cyc_Toc_do_null_check(e);});int _tmp653=0;_tmp64B(_tmp64C,_tmp64D,_tmp64E,_tmp64F,_tmp650,_tmp651,_tmp652,_tmp653);});
goto _LL7;}}else{_LL24: _tmp582=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp545)->f1;_tmp583=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp545)->f2;_tmp584=(((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp545)->f3)->num_varargs;_tmp585=(((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp545)->f3)->injectors;_tmp586=(((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp545)->f3)->vai;_tmp587=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp545)->f5;_LL25: {struct Cyc_Absyn_Exp*e1=_tmp582;struct Cyc_List_List*es=_tmp583;int num_varargs=_tmp584;struct Cyc_List_List*injectors=_tmp585;struct Cyc_Absyn_VarargInfo*vai=_tmp586;struct _tuple0*env_effect=_tmp587;
# 2989 "toc.cyc"
struct _RegionHandle _tmp65E=_new_region("r");struct _RegionHandle*r=& _tmp65E;_push_region(r);{
struct _tuple33 _stmttmp41=({Cyc_Toc_temp_var_and_exp();});struct Cyc_Absyn_Exp*_tmp660;struct _tuple1*_tmp65F;_LL95: _tmp65F=_stmttmp41.f1;_tmp660=_stmttmp41.f2;_LL96: {struct _tuple1*argv=_tmp65F;struct Cyc_Absyn_Exp*argvexp=_tmp660;
struct Cyc_Absyn_Exp*num_varargs_exp=({Cyc_Absyn_uint_exp((unsigned)num_varargs,0U);});
void*cva_type=({Cyc_Toc_typ_to_c(vai->type);});
void*arr_type=({Cyc_Absyn_array_type(cva_type,Cyc_Toc_mt_tq,num_varargs_exp,Cyc_Absyn_false_type,0U);});
# 2996
int num_args=({Cyc_List_length(es);});
int num_normargs=num_args - num_varargs;
# 3000
struct Cyc_List_List*new_args=0;
{int i=0;for(0;i < num_normargs;(++ i,es=es->tl)){
({Cyc_Toc_exp_to_c(nv,(struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(es))->hd);});
new_args=({struct Cyc_List_List*_tmp661=_cycalloc(sizeof(*_tmp661));_tmp661->hd=(struct Cyc_Absyn_Exp*)es->hd,_tmp661->tl=new_args;_tmp661;});}}
# 3005
new_args=({struct Cyc_List_List*_tmp663=_cycalloc(sizeof(*_tmp663));({struct Cyc_Absyn_Exp*_tmpE50=({struct Cyc_Absyn_Exp*_tmp662[3U];_tmp662[0]=argvexp,({
# 3007
struct Cyc_Absyn_Exp*_tmpE4E=({Cyc_Absyn_sizeoftype_exp(cva_type,0U);});_tmp662[1]=_tmpE4E;}),_tmp662[2]=num_varargs_exp;({struct Cyc_Absyn_Exp*_tmpE4F=Cyc_Toc__tag_fat_e;Cyc_Toc_fncall_exp_dl(_tmpE4F,_tag_fat(_tmp662,sizeof(struct Cyc_Absyn_Exp*),3U));});});
# 3005
_tmp663->hd=_tmpE50;}),_tmp663->tl=new_args;_tmp663;});
# 3010
new_args=({Cyc_List_imp_rev(new_args);});{
# 3012
void*e1_typ=(void*)_check_null(e1->topt);
({Cyc_Toc_exp_to_c(nv,e1);});
did_inserted_checks=1;
if(({Cyc_Toc_do_null_check(e);}))
# 3017
({void*_tmpE53=({void*(*_tmp664)(void*t,struct Cyc_Absyn_Exp*e)=Cyc_Toc_cast_it_r;void*_tmp665=({Cyc_Toc_typ_to_c(e1_typ);});struct Cyc_Absyn_Exp*_tmp666=({struct Cyc_Absyn_Exp*_tmp667[1U];({struct Cyc_Absyn_Exp*_tmpE51=({Cyc_Absyn_copy_exp(e1);});_tmp667[0]=_tmpE51;});({struct Cyc_Absyn_Exp*_tmpE52=Cyc_Toc__check_null_e;Cyc_Toc_fncall_exp_dl(_tmpE52,_tag_fat(_tmp667,sizeof(struct Cyc_Absyn_Exp*),1U));});});_tmp664(_tmp665,_tmp666);});e1->r=_tmpE53;});{
# 3015
struct Cyc_Absyn_Stmt*s=({struct Cyc_Absyn_Stmt*(*_tmp69B)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_exp_stmt;struct Cyc_Absyn_Exp*_tmp69C=({Cyc_Absyn_fncall_exp(e1,new_args,0U);});unsigned _tmp69D=0U;_tmp69B(_tmp69C,_tmp69D);});
# 3022
if(vai->inject){
struct Cyc_Absyn_Datatypedecl*tud;
{void*_stmttmp42=({void*(*_tmp66D)(void*)=Cyc_Tcutil_compress;void*_tmp66E=({Cyc_Tcutil_pointer_elt_type(vai->type);});_tmp66D(_tmp66E);});void*_tmp668=_stmttmp42;struct Cyc_Absyn_Datatypedecl*_tmp669;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp668)->tag == 0U){if(((struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp668)->f1)->tag == 20U){if(((((struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp668)->f1)->f1).KnownDatatype).tag == 2){_LL98: _tmp669=*((((struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp668)->f1)->f1).KnownDatatype).val;_LL99: {struct Cyc_Absyn_Datatypedecl*x=_tmp669;
tud=x;goto _LL97;}}else{goto _LL9A;}}else{goto _LL9A;}}else{_LL9A: _LL9B:
({void*_tmp66A=0U;({int(*_tmpE55)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp66C)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmp66C;});struct _fat_ptr _tmpE54=({const char*_tmp66B="toc: unknown datatype in vararg with inject";_tag_fat(_tmp66B,sizeof(char),44U);});_tmpE55(_tmpE54,_tag_fat(_tmp66A,sizeof(void*),0U));});});}_LL97:;}{
# 3028
struct _fat_ptr vs=({unsigned _tmp690=(unsigned)num_varargs;struct _tuple1**_tmp68F=({struct _RegionHandle*_tmpE56=r;_region_malloc(_tmpE56,_check_times(_tmp690,sizeof(struct _tuple1*)));});({{unsigned _tmpC90=(unsigned)num_varargs;unsigned i;for(i=0;i < _tmpC90;++ i){({struct _tuple1*_tmpE57=({Cyc_Toc_temp_var();});_tmp68F[i]=_tmpE57;});}}0;});_tag_fat(_tmp68F,sizeof(struct _tuple1*),_tmp690);});
# 3030
if(num_varargs != 0){
# 3034
struct Cyc_List_List*array_args=0;
{int i=num_varargs - 1;for(0;i >= 0;-- i){
array_args=({struct Cyc_List_List*_tmp672=_cycalloc(sizeof(*_tmp672));({struct Cyc_Absyn_Exp*_tmpE58=({struct Cyc_Absyn_Exp*(*_tmp66F)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_address_exp;struct Cyc_Absyn_Exp*_tmp670=({Cyc_Absyn_var_exp(*((struct _tuple1**)_check_fat_subscript(vs,sizeof(struct _tuple1*),i)),0U);});unsigned _tmp671=0U;_tmp66F(_tmp670,_tmp671);});_tmp672->hd=_tmpE58;}),_tmp672->tl=array_args;_tmp672;});}}
s=({struct Cyc_Absyn_Stmt*(*_tmp673)(struct _tuple1*,void*,struct Cyc_Absyn_Exp*init,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_declare_stmt;struct _tuple1*_tmp674=argv;void*_tmp675=arr_type;struct Cyc_Absyn_Exp*_tmp676=({Cyc_Absyn_array_exp(array_args,0U);});struct Cyc_Absyn_Stmt*_tmp677=s;unsigned _tmp678=0U;_tmp673(_tmp674,_tmp675,_tmp676,_tmp677,_tmp678);});
# 3039
es=({Cyc_List_imp_rev(es);});
injectors=({Cyc_List_imp_rev(injectors);});{
int i=({Cyc_List_length(es);})- 1;
for(0;es != 0;(es=es->tl,injectors=injectors->tl,-- i)){
struct Cyc_Absyn_Exp*arg=(struct Cyc_Absyn_Exp*)es->hd;
void*arg_type=(void*)_check_null(arg->topt);
struct _tuple1*var=*((struct _tuple1**)_check_fat_subscript(vs,sizeof(struct _tuple1*),i));
struct Cyc_Absyn_Exp*varexp=({Cyc_Absyn_var_exp(var,0U);});
struct Cyc_Absyn_Datatypefield*_stmttmp43=(struct Cyc_Absyn_Datatypefield*)((struct Cyc_List_List*)_check_null(injectors))->hd;struct Cyc_List_List*_tmp67A;struct _tuple1*_tmp679;_LL9D: _tmp679=_stmttmp43->name;_tmp67A=_stmttmp43->typs;_LL9E: {struct _tuple1*qv=_tmp679;struct Cyc_List_List*tqts=_tmp67A;
void*field_typ=({Cyc_Toc_typ_to_c((*((struct _tuple14*)((struct Cyc_List_List*)_check_null(tqts))->hd)).f2);});
({Cyc_Toc_exp_to_c(nv,arg);});
if(({Cyc_Toc_is_void_star_or_boxed_tvar(field_typ);}))
arg=({Cyc_Toc_cast_it(field_typ,arg);});{
# 3050
struct _tuple1*tdn=({Cyc_Toc_collapse_qvars(qv,tud->name);});
# 3054
struct Cyc_List_List*dles=({struct _tuple21*_tmp682[2U];({struct _tuple21*_tmpE5A=({struct _tuple21*(*_tmp683)(struct _fat_ptr*name,struct Cyc_Absyn_Exp*e)=Cyc_Toc_make_field;struct _fat_ptr*_tmp684=Cyc_Toc_tag_sp;struct Cyc_Absyn_Exp*_tmp685=({Cyc_Toc_datatype_tag(tud,qv);});_tmp683(_tmp684,_tmp685);});_tmp682[0]=_tmpE5A;}),({
struct _tuple21*_tmpE59=({struct _tuple21*(*_tmp686)(struct _fat_ptr*name,struct Cyc_Absyn_Exp*e)=Cyc_Toc_make_field;struct _fat_ptr*_tmp687=({Cyc_Absyn_fieldname(1);});struct Cyc_Absyn_Exp*_tmp688=arg;_tmp686(_tmp687,_tmp688);});_tmp682[1]=_tmpE59;});Cyc_List_list(_tag_fat(_tmp682,sizeof(struct _tuple21*),2U));});
s=({struct Cyc_Absyn_Stmt*(*_tmp67B)(struct _tuple1*,void*,struct Cyc_Absyn_Exp*init,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_declare_stmt;struct _tuple1*_tmp67C=var;void*_tmp67D=({Cyc_Absyn_strctq(tdn);});struct Cyc_Absyn_Exp*_tmp67E=({Cyc_Absyn_new_exp((void*)({struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*_tmp67F=_cycalloc(sizeof(*_tmp67F));_tmp67F->tag=29U,_tmp67F->f1=tdn,_tmp67F->f2=0,_tmp67F->f3=dles,_tmp67F->f4=0;_tmp67F;}),0U);});struct Cyc_Absyn_Stmt*_tmp680=s;unsigned _tmp681=0U;_tmp67B(_tmp67C,_tmp67D,_tmp67E,_tmp680,_tmp681);});}}}}}else{
# 3064
s=({struct Cyc_Absyn_Stmt*(*_tmp689)(struct _tuple1*,void*,struct Cyc_Absyn_Exp*init,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_declare_stmt;struct _tuple1*_tmp68A=argv;void*_tmp68B=({Cyc_Toc_void_star_type();});struct Cyc_Absyn_Exp*_tmp68C=({Cyc_Absyn_uint_exp(0U,0U);});struct Cyc_Absyn_Stmt*_tmp68D=s;unsigned _tmp68E=0U;_tmp689(_tmp68A,_tmp68B,_tmp68C,_tmp68D,_tmp68E);});}}}else{
# 3069
if(num_varargs != 0){
struct Cyc_List_List*array_args=0;
for(0;es != 0;es=es->tl){
({Cyc_Toc_exp_to_c(nv,(struct Cyc_Absyn_Exp*)es->hd);});
array_args=({struct Cyc_List_List*_tmp691=_cycalloc(sizeof(*_tmp691));_tmp691->hd=(struct Cyc_Absyn_Exp*)es->hd,_tmp691->tl=array_args;_tmp691;});}{
# 3075
struct Cyc_Absyn_Exp*init=({struct Cyc_Absyn_Exp*(*_tmp692)(struct Cyc_List_List*,unsigned)=Cyc_Absyn_array_exp;struct Cyc_List_List*_tmp693=({Cyc_List_imp_rev(array_args);});unsigned _tmp694=0U;_tmp692(_tmp693,_tmp694);});
s=({Cyc_Absyn_declare_stmt(argv,arr_type,init,s,0U);});}}else{
# 3078
s=({struct Cyc_Absyn_Stmt*(*_tmp695)(struct _tuple1*,void*,struct Cyc_Absyn_Exp*init,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_declare_stmt;struct _tuple1*_tmp696=argv;void*_tmp697=({Cyc_Toc_void_star_type();});struct Cyc_Absyn_Exp*_tmp698=({Cyc_Absyn_uint_exp(0U,0U);});struct Cyc_Absyn_Stmt*_tmp699=s;unsigned _tmp69A=0U;_tmp695(_tmp696,_tmp697,_tmp698,_tmp699,_tmp69A);});}}
# 3081
({void*_tmpE5B=({Cyc_Toc_stmt_exp_r(s);});e->r=_tmpE5B;});}}}}
# 3083
_npop_handler(0U);goto _LL7;
# 2989
;_pop_region();}}case 34U: _LL22: _tmp57E=((struct Cyc_Absyn_Spawn_e_Absyn_Raw_exp_struct*)_tmp545)->f1;_tmp57F=((struct Cyc_Absyn_Spawn_e_Absyn_Raw_exp_struct*)_tmp545)->f2;_tmp580=(((struct Cyc_Absyn_Spawn_e_Absyn_Raw_exp_struct*)_tmp545)->f3)->f1;_tmp581=(((struct Cyc_Absyn_Spawn_e_Absyn_Raw_exp_struct*)_tmp545)->f3)->f2;_LL23: {struct Cyc_Absyn_Exp*e1=_tmp57E;struct Cyc_Absyn_Exp*e2=_tmp57F;struct Cyc_List_List*ef_in=_tmp580;struct Cyc_List_List*ef_out=_tmp581;
# 2972 "toc.cyc"
({void(*_tmp654)(struct Cyc_Toc_Env*nv,struct Cyc_Absyn_Exp*e,struct Cyc_Absyn_Exp*e1,struct Cyc_List_List*es,struct Cyc_List_List*ef_in,struct Cyc_List_List*ef_out,int need_check,int is_thread)=Cyc_Toc_makeFnCall;struct Cyc_Toc_Env*_tmp655=nv;struct Cyc_Absyn_Exp*_tmp656=e;struct Cyc_Absyn_Exp*_tmp657=e1;struct Cyc_List_List*_tmp658=({struct Cyc_List_List*_tmp659=_cycalloc(sizeof(*_tmp659));_tmp659->hd=e2,_tmp659->tl=0;_tmp659;});struct Cyc_List_List*_tmp65A=ef_in;struct Cyc_List_List*_tmp65B=ef_out;int _tmp65C=({Cyc_Toc_do_null_check(e);});int _tmp65D=1;_tmp654(_tmp655,_tmp656,_tmp657,_tmp658,_tmp65A,_tmp65B,_tmp65C,_tmp65D);});
# 2979
goto _LL7;}case 11U: _LL26: _tmp57C=((struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct*)_tmp545)->f1;_tmp57D=((struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct*)_tmp545)->f2;_LL27: {struct Cyc_Absyn_Exp*e1=_tmp57C;int b=_tmp57D;
# 3086 "toc.cyc"
({Cyc_Toc_exp_to_c(nv,e1);});{
struct Cyc_Absyn_Exp*fn_e=b?Cyc_Toc__rethrow_e: Cyc_Toc__throw_e;
({void*_tmpE5D=({struct Cyc_Absyn_Exp*(*_tmp69E)(void*t,struct Cyc_Absyn_Exp*e,unsigned l)=Cyc_Toc_array_to_ptr_cast;void*_tmp69F=({Cyc_Toc_typ_to_c(old_typ);});struct Cyc_Absyn_Exp*_tmp6A0=({struct Cyc_Absyn_Exp*_tmp6A1[1U];_tmp6A1[0]=e1;({struct Cyc_Absyn_Exp*_tmpE5C=fn_e;Cyc_Toc_fncall_exp_dl(_tmpE5C,_tag_fat(_tmp6A1,sizeof(struct Cyc_Absyn_Exp*),1U));});});unsigned _tmp6A2=0U;_tmp69E(_tmp69F,_tmp6A0,_tmp6A2);})->r;e->r=_tmpE5D;});
goto _LL7;}}case 12U: _LL28: _tmp57B=((struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*)_tmp545)->f1;_LL29: {struct Cyc_Absyn_Exp*e1=_tmp57B;
({Cyc_Toc_exp_to_c(nv,e1);});goto _LL7;}case 13U: _LL2A: _tmp579=((struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*)_tmp545)->f1;_tmp57A=((struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*)_tmp545)->f2;_LL2B: {struct Cyc_Absyn_Exp*e1=_tmp579;struct Cyc_List_List*ts=_tmp57A;
# 3092
({Cyc_Toc_exp_to_c(nv,e1);});{
# 3101 "toc.cyc"
void*tc=({Cyc_Toc_typ_to_c((void*)_check_null(e->topt));});
void*tc1=({Cyc_Toc_typ_to_c((void*)_check_null(e1->topt));});
if(!({Cyc_Tcutil_typecmp(tc,tc1);})== 0){
# 3113 "toc.cyc"
struct _tuple33 _stmttmp44=({Cyc_Toc_temp_var_and_exp();});struct Cyc_Absyn_Exp*_tmp6A4;struct _tuple1*_tmp6A3;_LLA0: _tmp6A3=_stmttmp44.f1;_tmp6A4=_stmttmp44.f2;_LLA1: {struct _tuple1*temp=_tmp6A3;struct Cyc_Absyn_Exp*temp_exp=_tmp6A4;
({void*_tmpE5E=({void*(*_tmp6A5)(struct Cyc_Absyn_Stmt*s)=Cyc_Toc_stmt_exp_r;struct Cyc_Absyn_Stmt*_tmp6A6=({struct Cyc_Absyn_Stmt*(*_tmp6A7)(struct _tuple1*,void*,struct Cyc_Absyn_Exp*init,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_declare_stmt;struct _tuple1*_tmp6A8=temp;void*_tmp6A9=tc;struct Cyc_Absyn_Exp*_tmp6AA=({Cyc_Toc_array_to_ptr_cast(tc,e1,0U);});struct Cyc_Absyn_Stmt*_tmp6AB=({Cyc_Absyn_exp_stmt(temp_exp,e->loc);});unsigned _tmp6AC=e->loc;_tmp6A7(_tmp6A8,_tmp6A9,_tmp6AA,_tmp6AB,_tmp6AC);});_tmp6A5(_tmp6A6);});e->r=_tmpE5E;});}}
# 3103 "toc.cyc"
goto _LL7;}}case 14U: _LL2C: _tmp575=(void**)&((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp545)->f1;_tmp576=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp545)->f2;_tmp577=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp545)->f3;_tmp578=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp545)->f4;_LL2D: {void**t=_tmp575;struct Cyc_Absyn_Exp*e1=_tmp576;int user_inserted=_tmp577;enum Cyc_Absyn_Coercion coercion=_tmp578;
# 3145 "toc.cyc"
void*old_t2=(void*)_check_null(e1->topt);
void*old_t2_c=({Cyc_Toc_typ_to_c(old_t2);});
void*new_typ=*t;
void*new_typ_c=({Cyc_Toc_typ_to_c(new_typ);});
*t=new_typ_c;
({Cyc_Toc_exp_to_c(nv,e1);});
# 3152
{struct _tuple41 _stmttmp45=({struct _tuple41 _tmpC92;({void*_tmpE60=({Cyc_Tcutil_compress(old_t2);});_tmpC92.f1=_tmpE60;}),({void*_tmpE5F=({Cyc_Tcutil_compress(new_typ);});_tmpC92.f2=_tmpE5F;});_tmpC92;});struct _tuple41 _tmp6AD=_stmttmp45;struct Cyc_Absyn_PtrInfo _tmp6AE;struct Cyc_Absyn_PtrInfo _tmp6B0;struct Cyc_Absyn_PtrInfo _tmp6AF;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp6AD.f1)->tag == 3U)switch(*((int*)_tmp6AD.f2)){case 3U: _LLA3: _tmp6AF=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp6AD.f1)->f1;_tmp6B0=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp6AD.f2)->f1;_LLA4: {struct Cyc_Absyn_PtrInfo p1=_tmp6AF;struct Cyc_Absyn_PtrInfo p2=_tmp6B0;
# 3154
struct Cyc_Absyn_Exp*b1=({struct Cyc_Absyn_Exp*(*_tmp6EC)(void*def,void*b)=Cyc_Tcutil_get_bounds_exp;void*_tmp6ED=({Cyc_Absyn_bounds_one();});void*_tmp6EE=(p1.ptr_atts).bounds;_tmp6EC(_tmp6ED,_tmp6EE);});
struct Cyc_Absyn_Exp*b2=({struct Cyc_Absyn_Exp*(*_tmp6E9)(void*def,void*b)=Cyc_Tcutil_get_bounds_exp;void*_tmp6EA=({Cyc_Absyn_bounds_one();});void*_tmp6EB=(p2.ptr_atts).bounds;_tmp6E9(_tmp6EA,_tmp6EB);});
int zt1=({Cyc_Tcutil_force_type2bool(0,(p1.ptr_atts).zero_term);});
int zt2=({Cyc_Tcutil_force_type2bool(0,(p2.ptr_atts).zero_term);});
{struct _tuple42 _stmttmp46=({struct _tuple42 _tmpC91;_tmpC91.f1=b1,_tmpC91.f2=b2;_tmpC91;});struct _tuple42 _tmp6B1=_stmttmp46;if(_tmp6B1.f1 != 0){if(_tmp6B1.f2 != 0){_LLAA: _LLAB:
# 3161
 did_inserted_checks=1;
if(({Cyc_Toc_do_null_check(e);}))
({void*_tmpE62=({void*(*_tmp6B2)(void*t,struct Cyc_Absyn_Exp*e)=Cyc_Toc_cast_it_r;void*_tmp6B3=*t;struct Cyc_Absyn_Exp*_tmp6B4=({struct Cyc_Absyn_Exp*_tmp6B5[1U];_tmp6B5[0]=e1;({struct Cyc_Absyn_Exp*_tmpE61=Cyc_Toc__check_null_e;Cyc_Toc_fncall_exp_dl(_tmpE61,_tag_fat(_tmp6B5,sizeof(struct Cyc_Absyn_Exp*),1U));});});_tmp6B2(_tmp6B3,_tmp6B4);});e->r=_tmpE62;});else{
if(({Cyc_Unify_unify(old_t2_c,new_typ_c);}))
e->r=e1->r;}
# 3162
goto _LLA9;}else{_LLAC: _LLAD: {
# 3172
struct Cyc_Absyn_Exp*e2=(struct Cyc_Absyn_Exp*)_check_null(b1);
struct _tuple15 _stmttmp47=({Cyc_Evexp_eval_const_uint_exp(e2);});int _tmp6B7;unsigned _tmp6B6;_LLB3: _tmp6B6=_stmttmp47.f1;_tmp6B7=_stmttmp47.f2;_LLB4: {unsigned i=_tmp6B6;int valid=_tmp6B7;
if(({Cyc_Toc_is_toplevel(nv);})){
# 3178
if((zt1 && !(p2.elt_tq).real_const)&& !zt2)
e2=({struct Cyc_Absyn_Exp*(*_tmp6B8)(enum Cyc_Absyn_Primop,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_prim2_exp;enum Cyc_Absyn_Primop _tmp6B9=Cyc_Absyn_Minus;struct Cyc_Absyn_Exp*_tmp6BA=e2;struct Cyc_Absyn_Exp*_tmp6BB=({Cyc_Absyn_uint_exp(1U,0U);});unsigned _tmp6BC=0U;_tmp6B8(_tmp6B9,_tmp6BA,_tmp6BB,_tmp6BC);});
# 3178
({void*_tmpE63=({Cyc_Toc_make_toplevel_dyn_arr(old_t2,e2,e1);})->r;e->r=_tmpE63;});}else{
# 3183
if(zt1){
# 3188
struct _tuple33 _stmttmp48=({Cyc_Toc_temp_var_and_exp();});struct Cyc_Absyn_Exp*_tmp6BE;struct _tuple1*_tmp6BD;_LLB6: _tmp6BD=_stmttmp48.f1;_tmp6BE=_stmttmp48.f2;_LLB7: {struct _tuple1*x=_tmp6BD;struct Cyc_Absyn_Exp*x_exp=_tmp6BE;
struct Cyc_Absyn_Exp*arg3;
# 3192
{void*_stmttmp49=e1->r;void*_tmp6BF=_stmttmp49;struct Cyc_Absyn_Vardecl*_tmp6C0;struct Cyc_Absyn_Vardecl*_tmp6C1;switch(*((int*)_tmp6BF)){case 0U: switch(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp6BF)->f1).Wstring_c).tag){case 8U: _LLB9: _LLBA:
 arg3=e2;goto _LLB8;case 9U: _LLBB: _LLBC:
 arg3=e2;goto _LLB8;default: goto _LLC1;}case 1U: switch(*((int*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp6BF)->f1)){case 1U: _LLBD: _tmp6C1=((struct Cyc_Absyn_Global_b_Absyn_Binding_struct*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp6BF)->f1)->f1;_LLBE: {struct Cyc_Absyn_Vardecl*vd=_tmp6C1;
_tmp6C0=vd;goto _LLC0;}case 4U: _LLBF: _tmp6C0=((struct Cyc_Absyn_Local_b_Absyn_Binding_struct*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp6BF)->f1)->f1;_LLC0: {struct Cyc_Absyn_Vardecl*vd=_tmp6C0;
# 3197
if(!({Cyc_Tcutil_is_array_type(vd->type);}))
goto _LLC2;
# 3197
arg3=e2;
# 3200
goto _LLB8;}default: goto _LLC1;}default: _LLC1: _LLC2:
# 3204
 if(valid && i != (unsigned)1)
arg3=e2;else{
# 3207
arg3=({struct Cyc_Absyn_Exp*_tmp6C2[2U];({
struct Cyc_Absyn_Exp*_tmpE64=({struct Cyc_Absyn_Exp*(*_tmp6C3)(void*t,struct Cyc_Absyn_Exp*e)=Cyc_Toc_cast_it;void*_tmp6C4=({Cyc_Toc_void_star_type();});struct Cyc_Absyn_Exp*_tmp6C5=x_exp;_tmp6C3(_tmp6C4,_tmp6C5);});_tmp6C2[0]=_tmpE64;}),_tmp6C2[1]=e2;({struct Cyc_Absyn_Exp*_tmpE65=({Cyc_Toc_getFunctionRemovePointer(& Cyc_Toc__get_zero_arr_size_functionSet,e1);});Cyc_Toc_fncall_exp_dl(_tmpE65,_tag_fat(_tmp6C2,sizeof(struct Cyc_Absyn_Exp*),2U));});});}
goto _LLB8;}_LLB8:;}
# 3211
if(!zt2 && !(p2.elt_tq).real_const)
# 3214
arg3=({struct Cyc_Absyn_Exp*(*_tmp6C6)(enum Cyc_Absyn_Primop,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_prim2_exp;enum Cyc_Absyn_Primop _tmp6C7=Cyc_Absyn_Minus;struct Cyc_Absyn_Exp*_tmp6C8=arg3;struct Cyc_Absyn_Exp*_tmp6C9=({Cyc_Absyn_uint_exp(1U,0U);});unsigned _tmp6CA=0U;_tmp6C6(_tmp6C7,_tmp6C8,_tmp6C9,_tmp6CA);});{
# 3211
struct Cyc_Absyn_Exp*arg2=({struct Cyc_Absyn_Exp*(*_tmp6D2)(void*t,unsigned)=Cyc_Absyn_sizeoftype_exp;void*_tmp6D3=({Cyc_Toc_typ_to_c(p2.elt_type);});unsigned _tmp6D4=0U;_tmp6D2(_tmp6D3,_tmp6D4);});
# 3217
struct Cyc_Absyn_Exp*tg_exp=({struct Cyc_Absyn_Exp*_tmp6D1[3U];_tmp6D1[0]=x_exp,_tmp6D1[1]=arg2,_tmp6D1[2]=arg3;({struct Cyc_Absyn_Exp*_tmpE66=Cyc_Toc__tag_fat_e;Cyc_Toc_fncall_exp_dl(_tmpE66,_tag_fat(_tmp6D1,sizeof(struct Cyc_Absyn_Exp*),3U));});});
struct Cyc_Absyn_Stmt*s=({Cyc_Absyn_exp_stmt(tg_exp,0U);});
s=({struct Cyc_Absyn_Stmt*(*_tmp6CB)(struct _tuple1*,void*,struct Cyc_Absyn_Exp*init,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_declare_stmt;struct _tuple1*_tmp6CC=x;void*_tmp6CD=({Cyc_Toc_typ_to_c(old_t2);});struct Cyc_Absyn_Exp*_tmp6CE=e1;struct Cyc_Absyn_Stmt*_tmp6CF=s;unsigned _tmp6D0=0U;_tmp6CB(_tmp6CC,_tmp6CD,_tmp6CE,_tmp6CF,_tmp6D0);});
({void*_tmpE67=({Cyc_Toc_stmt_exp_r(s);});e->r=_tmpE67;});}}}else{
# 3223
({void*_tmpE6A=({struct Cyc_Absyn_Exp*_tmp6D5[3U];_tmp6D5[0]=e1,({
# 3225
struct Cyc_Absyn_Exp*_tmpE68=({struct Cyc_Absyn_Exp*(*_tmp6D6)(void*t,unsigned)=Cyc_Absyn_sizeoftype_exp;void*_tmp6D7=({Cyc_Toc_typ_to_c(p2.elt_type);});unsigned _tmp6D8=0U;_tmp6D6(_tmp6D7,_tmp6D8);});_tmp6D5[1]=_tmpE68;}),_tmp6D5[2]=e2;({struct Cyc_Absyn_Exp*_tmpE69=Cyc_Toc__tag_fat_e;Cyc_Toc_fncall_exp_r(_tmpE69,_tag_fat(_tmp6D5,sizeof(struct Cyc_Absyn_Exp*),3U));});});
# 3223
e->r=_tmpE6A;});}}
# 3228
goto _LLA9;}}}}else{if(_tmp6B1.f2 != 0){_LLAE: _LLAF: {
# 3238 "toc.cyc"
struct Cyc_Absyn_Exp*new_e2=(struct Cyc_Absyn_Exp*)_check_null(b2);
if(zt1 && !zt2)
new_e2=({struct Cyc_Absyn_Exp*(*_tmp6D9)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_add_exp;struct Cyc_Absyn_Exp*_tmp6DA=b2;struct Cyc_Absyn_Exp*_tmp6DB=({Cyc_Absyn_uint_exp(1U,0U);});unsigned _tmp6DC=0U;_tmp6D9(_tmp6DA,_tmp6DB,_tmp6DC);});{
# 3239
struct Cyc_Absyn_Exp*ptr_exp=({struct Cyc_Absyn_Exp*_tmp6DE[3U];_tmp6DE[0]=e1,({
# 3247
struct Cyc_Absyn_Exp*_tmpE6B=({struct Cyc_Absyn_Exp*(*_tmp6DF)(void*t,unsigned)=Cyc_Absyn_sizeoftype_exp;void*_tmp6E0=({Cyc_Toc_typ_to_c(p1.elt_type);});unsigned _tmp6E1=0U;_tmp6DF(_tmp6E0,_tmp6E1);});_tmp6DE[1]=_tmpE6B;}),_tmp6DE[2]=new_e2;({struct Cyc_Absyn_Exp*_tmpE6C=Cyc_Toc__untag_fat_ptr_e;Cyc_Toc_fncall_exp_dl(_tmpE6C,_tag_fat(_tmp6DE,sizeof(struct Cyc_Absyn_Exp*),3U));});});
# 3249
did_inserted_checks=1;
if(({Cyc_Toc_do_null_check(e);}))
({void*_tmpE6F=({struct Cyc_Absyn_Exp*_tmp6DD[1U];({struct Cyc_Absyn_Exp*_tmpE6D=({Cyc_Absyn_copy_exp(ptr_exp);});_tmp6DD[0]=_tmpE6D;});({struct Cyc_Absyn_Exp*_tmpE6E=Cyc_Toc__check_null_e;Cyc_Toc_fncall_exp_r(_tmpE6E,_tag_fat(_tmp6DD,sizeof(struct Cyc_Absyn_Exp*),1U));});});ptr_exp->r=_tmpE6F;});
# 3250
({void*_tmpE70=({Cyc_Toc_cast_it_r(*t,ptr_exp);});e->r=_tmpE70;});
# 3253
goto _LLA9;}}}else{_LLB0: _LLB1:
# 3257
 if((zt1 && !zt2)&& !(p2.elt_tq).real_const){
if(({Cyc_Toc_is_toplevel(nv);}))
({void*_tmp6E2=0U;({int(*_tmpE72)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp6E4)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_unimp;_tmp6E4;});struct _fat_ptr _tmpE71=({const char*_tmp6E3="can't coerce a ZEROTERM to a non-const NOZEROTERM pointer at toplevel";_tag_fat(_tmp6E3,sizeof(char),70U);});_tmpE72(_tmpE71,_tag_fat(_tmp6E2,sizeof(void*),0U));});});
# 3258
({void*_tmpE76=({struct Cyc_Absyn_Exp*_tmp6E5[3U];_tmp6E5[0]=e1,({
# 3261
struct Cyc_Absyn_Exp*_tmpE74=({struct Cyc_Absyn_Exp*(*_tmp6E6)(void*t,unsigned)=Cyc_Absyn_sizeoftype_exp;void*_tmp6E7=({Cyc_Toc_typ_to_c(p1.elt_type);});unsigned _tmp6E8=0U;_tmp6E6(_tmp6E7,_tmp6E8);});_tmp6E5[1]=_tmpE74;}),({
struct Cyc_Absyn_Exp*_tmpE73=({Cyc_Absyn_uint_exp(1U,0U);});_tmp6E5[2]=_tmpE73;});({struct Cyc_Absyn_Exp*_tmpE75=Cyc_Toc__fat_ptr_decrease_size_e;Cyc_Toc_fncall_exp_r(_tmpE75,_tag_fat(_tmp6E5,sizeof(struct Cyc_Absyn_Exp*),3U));});});
# 3258
e->r=_tmpE76;});}
# 3257
goto _LLA9;}}_LLA9:;}
# 3266
goto _LLA2;}case 0U: if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp6AD.f2)->f1)->tag == 1U){_LLA5: _tmp6AE=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp6AD.f1)->f1;_LLA6: {struct Cyc_Absyn_PtrInfo p1=_tmp6AE;
# 3268
{struct Cyc_Absyn_Exp*_stmttmp4A=({struct Cyc_Absyn_Exp*(*_tmp6F3)(void*def,void*b)=Cyc_Tcutil_get_bounds_exp;void*_tmp6F4=({Cyc_Absyn_bounds_one();});void*_tmp6F5=(p1.ptr_atts).bounds;_tmp6F3(_tmp6F4,_tmp6F5);});struct Cyc_Absyn_Exp*_tmp6EF=_stmttmp4A;if(_tmp6EF == 0){_LLC4: _LLC5:
# 3270
({void*_tmpE77=({void*(*_tmp6F0)(struct Cyc_Absyn_Exp*e,struct _fat_ptr*n)=Cyc_Toc_aggrmember_exp_r;struct Cyc_Absyn_Exp*_tmp6F1=({Cyc_Absyn_new_exp(e1->r,e1->loc);});struct _fat_ptr*_tmp6F2=Cyc_Toc_curr_sp;_tmp6F0(_tmp6F1,_tmp6F2);});e1->r=_tmpE77;});
e1->topt=new_typ_c;
goto _LLC3;}else{_LLC6: _LLC7:
 goto _LLC3;}_LLC3:;}
# 3275
goto _LLA2;}}else{goto _LLA7;}default: goto _LLA7;}else{_LLA7: _LLA8:
# 3277
 if(({Cyc_Unify_unify(old_t2_c,new_typ_c);}))
e->r=e1->r;
# 3277
goto _LLA2;}_LLA2:;}
# 3281
goto _LL7;}case 15U: _LL2E: _tmp574=((struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct*)_tmp545)->f1;_LL2F: {struct Cyc_Absyn_Exp*e1=_tmp574;
# 3284
({Cyc_Toc_set_lhs(nv,1);});
({Cyc_Toc_exp_to_c(nv,e1);});
({Cyc_Toc_set_lhs(nv,0);});
if(!({Cyc_Absyn_is_lvalue(e1);})){
({({void(*_tmpE78)(struct Cyc_Absyn_Exp*e1,struct Cyc_List_List*fs,struct Cyc_Absyn_Exp*(*f)(struct Cyc_Absyn_Exp*,int),int f_env)=({void(*_tmp6F6)(struct Cyc_Absyn_Exp*e1,struct Cyc_List_List*fs,struct Cyc_Absyn_Exp*(*f)(struct Cyc_Absyn_Exp*,int),int f_env)=(void(*)(struct Cyc_Absyn_Exp*e1,struct Cyc_List_List*fs,struct Cyc_Absyn_Exp*(*f)(struct Cyc_Absyn_Exp*,int),int f_env))Cyc_Toc_lvalue_assign;_tmp6F6;});_tmpE78(e1,0,Cyc_Toc_address_lvalue,1);});});
# 3290
({void*_tmpE79=({void*(*_tmp6F7)(void*t,struct Cyc_Absyn_Exp*e)=Cyc_Toc_cast_it_r;void*_tmp6F8=({Cyc_Toc_typ_to_c((void*)_check_null(e->topt));});struct Cyc_Absyn_Exp*_tmp6F9=e1;_tmp6F7(_tmp6F8,_tmp6F9);});e->r=_tmpE79;});}else{
if(({int(*_tmp6FA)(struct Cyc_Absyn_Kind*,struct Cyc_Absyn_Kind*)=Cyc_Tcutil_kind_leq;struct Cyc_Absyn_Kind*_tmp6FB=& Cyc_Tcutil_ak;struct Cyc_Absyn_Kind*_tmp6FC=({Cyc_Tcutil_type_kind((void*)_check_null(e1->topt));});_tmp6FA(_tmp6FB,_tmp6FC);}))
# 3294
({void*_tmpE7A=({void*(*_tmp6FD)(void*t,struct Cyc_Absyn_Exp*e)=Cyc_Toc_cast_it_r;void*_tmp6FE=({Cyc_Toc_typ_to_c((void*)_check_null(e->topt));});struct Cyc_Absyn_Exp*_tmp6FF=e1;_tmp6FD(_tmp6FE,_tmp6FF);});e->r=_tmpE7A;});}
# 3287
goto _LL7;}case 16U: _LL30: _tmp572=((struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct*)_tmp545)->f1;_tmp573=((struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct*)_tmp545)->f2;_LL31: {struct Cyc_Absyn_Exp*rgnopt=_tmp572;struct Cyc_Absyn_Exp*e1=_tmp573;
# 3303
({Cyc_Toc_exp_to_c(nv,e1);});{
# 3305
void*elt_typ=({Cyc_Toc_typ_to_c((void*)_check_null(e1->topt));});
struct _tuple33 _stmttmp4B=({Cyc_Toc_temp_var_and_exp();});struct Cyc_Absyn_Exp*_tmp701;struct _tuple1*_tmp700;_LLC9: _tmp700=_stmttmp4B.f1;_tmp701=_stmttmp4B.f2;_LLCA: {struct _tuple1*x=_tmp700;struct Cyc_Absyn_Exp*xexp=_tmp701;
struct Cyc_Absyn_Exp*lhs;
{void*_stmttmp4C=({Cyc_Tcutil_compress(elt_typ);});void*_tmp702=_stmttmp4C;void*_tmp703;if(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp702)->tag == 4U){_LLCC: _tmp703=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp702)->f1).elt_type;_LLCD: {void*t2=_tmp703;
# 3310
elt_typ=({Cyc_Toc_typ_to_c(t2);});
lhs=({Cyc_Absyn_copy_exp(xexp);});
goto _LLCB;}}else{_LLCE: _LLCF:
# 3314
 lhs=({struct Cyc_Absyn_Exp*(*_tmp704)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_deref_exp;struct Cyc_Absyn_Exp*_tmp705=({Cyc_Absyn_copy_exp(xexp);});unsigned _tmp706=0U;_tmp704(_tmp705,_tmp706);});
goto _LLCB;}_LLCB:;}{
# 3317
struct Cyc_Absyn_Exp*array_len=({Cyc_Toc_array_length_exp(e1);});
struct _tuple1*lenvar=(unsigned)array_len?({Cyc_Toc_temp_var();}): 0;
struct Cyc_Absyn_Exp*lenexp=(unsigned)array_len?({Cyc_Absyn_var_exp((struct _tuple1*)_check_null(lenvar),0U);}): 0;
struct Cyc_Absyn_Exp*mexp;
if((unsigned)lenexp)
mexp=({struct Cyc_Absyn_Exp*_tmp707[2U];_tmp707[0]=lenexp,({struct Cyc_Absyn_Exp*_tmpE7B=({Cyc_Absyn_sizeoftype_exp(elt_typ,0U);});_tmp707[1]=_tmpE7B;});({struct Cyc_Absyn_Exp*_tmpE7C=Cyc_Toc__check_times_e;Cyc_Toc_fncall_exp_dl(_tmpE7C,_tag_fat(_tmp707,sizeof(struct Cyc_Absyn_Exp*),2U));});});else{
# 3324
mexp=({struct Cyc_Absyn_Exp*(*_tmp708)(struct Cyc_Absyn_Exp*e,unsigned)=Cyc_Absyn_sizeofexp_exp;struct Cyc_Absyn_Exp*_tmp709=({Cyc_Absyn_deref_exp(xexp,0U);});unsigned _tmp70A=0U;_tmp708(_tmp709,_tmp70A);});}{
# 3327
struct Cyc_Absyn_Exp*vse=({Cyc_Toc_get_varsizeexp(e1);});
if(vse != 0)
mexp=({Cyc_Absyn_add_exp(mexp,vse,0U);});
# 3328
if(
# 3331
rgnopt == 0 || Cyc_Absyn_no_regions)
mexp=({Cyc_Toc_malloc_exp(elt_typ,mexp);});else{
# 3334
({Cyc_Toc_exp_to_c(nv,rgnopt);});
mexp=({Cyc_Toc_rmalloc_exp(rgnopt,mexp);});}{
# 3337
struct Cyc_Absyn_Exp*answer;
if(({Cyc_Tcutil_is_fat_ptr(old_typ);}))
answer=({struct Cyc_Absyn_Exp*_tmp70B[3U];({
struct Cyc_Absyn_Exp*_tmpE7F=({Cyc_Absyn_copy_exp(xexp);});_tmp70B[0]=_tmpE7F;}),({
struct Cyc_Absyn_Exp*_tmpE7E=({Cyc_Absyn_sizeoftype_exp(elt_typ,0U);});_tmp70B[1]=_tmpE7E;}),
(unsigned)lenexp?_tmp70B[2]=lenexp:({struct Cyc_Absyn_Exp*_tmpE7D=({Cyc_Absyn_uint_exp(1U,0U);});_tmp70B[2]=_tmpE7D;});({struct Cyc_Absyn_Exp*_tmpE80=Cyc_Toc__tag_fat_e;Cyc_Toc_fncall_exp_dl(_tmpE80,_tag_fat(_tmp70B,sizeof(struct Cyc_Absyn_Exp*),3U));});});else{
# 3344
answer=({Cyc_Absyn_copy_exp(xexp);});}
({void*_tmpE81=(void*)({struct Cyc_Toc_Dest_Absyn_AbsynAnnot_struct*_tmp70C=_cycalloc(sizeof(*_tmp70C));_tmp70C->tag=Cyc_Toc_Dest,_tmp70C->f1=xexp;_tmp70C;});e->annot=_tmpE81;});{
struct Cyc_Absyn_Stmt*s=({struct Cyc_Absyn_Stmt*(*_tmp710)(struct _tuple1*,void*,struct Cyc_Absyn_Exp*init,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_declare_stmt;struct _tuple1*_tmp711=x;void*_tmp712=({Cyc_Absyn_cstar_type(elt_typ,Cyc_Toc_mt_tq);});struct Cyc_Absyn_Exp*_tmp713=mexp;struct Cyc_Absyn_Stmt*_tmp714=({struct Cyc_Absyn_Stmt*(*_tmp715)(struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_seq_stmt;struct Cyc_Absyn_Stmt*_tmp716=({struct Cyc_Absyn_Stmt*(*_tmp717)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_exp_stmt;struct Cyc_Absyn_Exp*_tmp718=({Cyc_Absyn_copy_exp(e);});unsigned _tmp719=0U;_tmp717(_tmp718,_tmp719);});struct Cyc_Absyn_Stmt*_tmp71A=({Cyc_Absyn_exp_stmt(answer,0U);});unsigned _tmp71B=0U;_tmp715(_tmp716,_tmp71A,_tmp71B);});unsigned _tmp71C=0U;_tmp710(_tmp711,_tmp712,_tmp713,_tmp714,_tmp71C);});
# 3350
if((unsigned)array_len)
s=({Cyc_Absyn_declare_stmt((struct _tuple1*)_check_null(lenvar),Cyc_Absyn_uint_type,array_len,s,0U);});
# 3350
({void*_tmpE82=({Cyc_Toc_stmt_exp_r(s);});e->r=_tmpE82;});
# 3353
if(vse != 0)
({void*_tmpE83=({void*(*_tmp70D)(void*t,struct Cyc_Absyn_Exp*e)=Cyc_Toc_cast_it_r;void*_tmp70E=({Cyc_Toc_typ_to_c(old_typ);});struct Cyc_Absyn_Exp*_tmp70F=({Cyc_Absyn_copy_exp(e);});_tmp70D(_tmp70E,_tmp70F);});e->r=_tmpE83;});
# 3353
goto _LL7;}}}}}}}case 18U: _LL32: _tmp571=((struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct*)_tmp545)->f1;_LL33: {struct Cyc_Absyn_Exp*e1=_tmp571;
# 3358
({Cyc_Toc_exp_to_c(nv,e1);});goto _LL7;}case 17U: _LL34: _tmp570=(void*)((struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct*)_tmp545)->f1;_LL35: {void*t=_tmp570;
({void*_tmpE85=(void*)({struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct*_tmp71D=_cycalloc(sizeof(*_tmp71D));_tmp71D->tag=17U,({void*_tmpE84=({Cyc_Toc_typ_to_c(t);});_tmp71D->f1=_tmpE84;});_tmp71D;});e->r=_tmpE85;});goto _LL7;}case 19U: _LL36: _tmp56E=(void*)((struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct*)_tmp545)->f1;_tmp56F=((struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct*)_tmp545)->f2;_LL37: {void*t=_tmp56E;struct Cyc_List_List*fs=_tmp56F;
# 3361
void*t2=t;
struct Cyc_List_List*l=fs;
for(0;l != 0;l=l->tl){
void*_stmttmp4D=(void*)l->hd;void*_tmp71E=_stmttmp4D;unsigned _tmp71F;struct _fat_ptr*_tmp720;if(((struct Cyc_Absyn_StructField_Absyn_OffsetofField_struct*)_tmp71E)->tag == 0U){_LLD1: _tmp720=((struct Cyc_Absyn_StructField_Absyn_OffsetofField_struct*)_tmp71E)->f1;_LLD2: {struct _fat_ptr*n=_tmp720;
goto _LLD0;}}else{_LLD3: _tmp71F=((struct Cyc_Absyn_TupleIndex_Absyn_OffsetofField_struct*)_tmp71E)->f1;_LLD4: {unsigned n=_tmp71F;
# 3367
{void*_stmttmp4E=({Cyc_Tcutil_compress(t2);});void*_tmp721=_stmttmp4E;struct Cyc_List_List*_tmp722;struct Cyc_List_List*_tmp723;struct Cyc_Absyn_Datatypefield*_tmp724;union Cyc_Absyn_AggrInfo _tmp725;switch(*((int*)_tmp721)){case 0U: switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp721)->f1)){case 22U: _LLD6: _tmp725=((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp721)->f1)->f1;_LLD7: {union Cyc_Absyn_AggrInfo info=_tmp725;
# 3369
struct Cyc_Absyn_Aggrdecl*ad=({Cyc_Absyn_get_known_aggrdecl(info);});
if(ad->impl == 0)
({void*_tmp726=0U;({int(*_tmpE87)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp728)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmp728;});struct _fat_ptr _tmpE86=({const char*_tmp727="struct fields must be known";_tag_fat(_tmp727,sizeof(char),28U);});_tmpE87(_tmpE86,_tag_fat(_tmp726,sizeof(void*),0U));});});
# 3370
_tmp723=((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->fields;goto _LLD9;}case 21U: if(((((struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp721)->f1)->f1).KnownDatatypefield).tag == 2){_LLDC: _tmp724=(((((struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp721)->f1)->f1).KnownDatatypefield).val).f2;_LLDD: {struct Cyc_Absyn_Datatypefield*tuf=_tmp724;
# 3383
if(n == (unsigned)0)
({void*_tmpE88=(void*)((void*)({struct Cyc_Absyn_StructField_Absyn_OffsetofField_struct*_tmp72D=_cycalloc(sizeof(*_tmp72D));_tmp72D->tag=0U,_tmp72D->f1=Cyc_Toc_tag_sp;_tmp72D;}));l->hd=_tmpE88;});else{
# 3386
t2=(*({({struct _tuple14*(*_tmpE8A)(struct Cyc_List_List*x,int n)=({struct _tuple14*(*_tmp72E)(struct Cyc_List_List*x,int n)=(struct _tuple14*(*)(struct Cyc_List_List*x,int n))Cyc_List_nth;_tmp72E;});struct Cyc_List_List*_tmpE89=tuf->typs;_tmpE8A(_tmpE89,(int)(n - (unsigned)1));});})).f2;
({void*_tmpE8C=(void*)((void*)({struct Cyc_Absyn_StructField_Absyn_OffsetofField_struct*_tmp72F=_cycalloc(sizeof(*_tmp72F));_tmp72F->tag=0U,({struct _fat_ptr*_tmpE8B=({Cyc_Absyn_fieldname((int)n);});_tmp72F->f1=_tmpE8B;});_tmp72F;}));l->hd=_tmpE8C;});}
# 3389
goto _LLD5;}}else{goto _LLDE;}default: goto _LLDE;}case 7U: _LLD8: _tmp723=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp721)->f2;_LLD9: {struct Cyc_List_List*fields=_tmp723;
# 3374
struct Cyc_Absyn_Aggrfield*nth_field=({({struct Cyc_Absyn_Aggrfield*(*_tmpE8E)(struct Cyc_List_List*x,int n)=({struct Cyc_Absyn_Aggrfield*(*_tmp72A)(struct Cyc_List_List*x,int n)=(struct Cyc_Absyn_Aggrfield*(*)(struct Cyc_List_List*x,int n))Cyc_List_nth;_tmp72A;});struct Cyc_List_List*_tmpE8D=fields;_tmpE8E(_tmpE8D,(int)n);});});
({void*_tmpE8F=(void*)((void*)({struct Cyc_Absyn_StructField_Absyn_OffsetofField_struct*_tmp729=_cycalloc(sizeof(*_tmp729));_tmp729->tag=0U,_tmp729->f1=nth_field->name;_tmp729;}));l->hd=_tmpE8F;});
t2=nth_field->type;
goto _LLD5;}case 6U: _LLDA: _tmp722=((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp721)->f1;_LLDB: {struct Cyc_List_List*ts=_tmp722;
# 3379
({void*_tmpE91=(void*)((void*)({struct Cyc_Absyn_StructField_Absyn_OffsetofField_struct*_tmp72B=_cycalloc(sizeof(*_tmp72B));_tmp72B->tag=0U,({struct _fat_ptr*_tmpE90=({Cyc_Absyn_fieldname((int)(n + (unsigned)1));});_tmp72B->f1=_tmpE90;});_tmp72B;}));l->hd=_tmpE91;});
t2=(*({({struct _tuple14*(*_tmpE93)(struct Cyc_List_List*x,int n)=({struct _tuple14*(*_tmp72C)(struct Cyc_List_List*x,int n)=(struct _tuple14*(*)(struct Cyc_List_List*x,int n))Cyc_List_nth;_tmp72C;});struct Cyc_List_List*_tmpE92=ts;_tmpE93(_tmpE92,(int)n);});})).f2;
goto _LLD5;}default: _LLDE: _LLDF:
# 3390
({void*_tmp730=0U;({int(*_tmpE95)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp732)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmp732;});struct _fat_ptr _tmpE94=({const char*_tmp731="impossible type for offsetof tuple index";_tag_fat(_tmp731,sizeof(char),41U);});_tmpE95(_tmpE94,_tag_fat(_tmp730,sizeof(void*),0U));});});}_LLD5:;}
# 3392
goto _LLD0;}}_LLD0:;}
# 3395
({void*_tmpE97=(void*)({struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct*_tmp733=_cycalloc(sizeof(*_tmp733));_tmp733->tag=19U,({void*_tmpE96=({Cyc_Toc_typ_to_c(t);});_tmp733->f1=_tmpE96;}),_tmp733->f2=fs;_tmp733;});e->r=_tmpE97;});
goto _LL7;}case 21U: _LL38: _tmp56A=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp545)->f1;_tmp56B=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp545)->f2;_tmp56C=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp545)->f3;_tmp56D=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp545)->f4;_LL39: {struct Cyc_Absyn_Exp*e1=_tmp56A;struct _fat_ptr*f=_tmp56B;int is_tagged=_tmp56C;int is_read=_tmp56D;
# 3398
int is_poly=({Cyc_Toc_is_poly_project(e);});
void*e1_cyc_type=(void*)_check_null(e1->topt);
({Cyc_Toc_exp_to_c(nv,e1);});
# 3403
if(({Cyc_Tcutil_is_tagged_union_and_needs_check(e);}))
({void*_tmpE98=({void*(*_tmp734)(struct Cyc_Absyn_Exp*e1,void*e1_c_type,void*aggrtype,struct _fat_ptr*f,int in_lhs,struct Cyc_Absyn_Exp*(*aggrproj)(struct Cyc_Absyn_Exp*,struct _fat_ptr*,unsigned))=Cyc_Toc_check_tagged_union;struct Cyc_Absyn_Exp*_tmp735=e1;void*_tmp736=({Cyc_Toc_typ_to_c(e1_cyc_type);});void*_tmp737=e1_cyc_type;struct _fat_ptr*_tmp738=f;int _tmp739=({Cyc_Toc_in_lhs(nv);});struct Cyc_Absyn_Exp*(*_tmp73A)(struct Cyc_Absyn_Exp*e,struct _fat_ptr*f,unsigned loc)=Cyc_Toc_member_exp;_tmp734(_tmp735,_tmp736,_tmp737,_tmp738,_tmp739,_tmp73A);});e->r=_tmpE98;});
# 3403
if(is_poly)
# 3407
({void*_tmpE99=({struct Cyc_Absyn_Exp*(*_tmp73B)(void*t,struct Cyc_Absyn_Exp*e,unsigned l)=Cyc_Toc_array_to_ptr_cast;void*_tmp73C=({Cyc_Toc_typ_to_c((void*)_check_null(e->topt));});struct Cyc_Absyn_Exp*_tmp73D=({Cyc_Absyn_new_exp(e->r,0U);});unsigned _tmp73E=0U;_tmp73B(_tmp73C,_tmp73D,_tmp73E);})->r;e->r=_tmpE99;});
# 3403
goto _LL7;}case 22U: _LL3A: _tmp566=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp545)->f1;_tmp567=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp545)->f2;_tmp568=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp545)->f3;_tmp569=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp545)->f4;_LL3B: {struct Cyc_Absyn_Exp*e1=_tmp566;struct _fat_ptr*f=_tmp567;int is_tagged=_tmp568;int is_read=_tmp569;
# 3410
int is_poly=({Cyc_Toc_is_poly_project(e);});
void*e1typ=(void*)_check_null(e1->topt);
void*ta;
{void*_stmttmp4F=({Cyc_Tcutil_compress(e1typ);});void*_tmp73F=_stmttmp4F;struct Cyc_Absyn_PtrInfo _tmp740;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp73F)->tag == 3U){_LLE1: _tmp740=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp73F)->f1;_LLE2: {struct Cyc_Absyn_PtrInfo p=_tmp740;
ta=p.elt_type;goto _LLE0;}}else{_LLE3: _LLE4:
({void*_tmp741=0U;({int(*_tmpE9B)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp743)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmp743;});struct _fat_ptr _tmpE9A=({const char*_tmp742="get_ptr_typ: not a pointer!";_tag_fat(_tmp742,sizeof(char),28U);});_tmpE9B(_tmpE9A,_tag_fat(_tmp741,sizeof(void*),0U));});});}_LLE0:;}
# 3417
did_inserted_checks=1;
({Cyc_Toc_ptr_use_to_c(nv,e1,e->annot,0);});
# 3422
if(({Cyc_Tcutil_is_tagged_union_and_needs_check(e);}))
({void*_tmpE9C=({void*(*_tmp744)(struct Cyc_Absyn_Exp*e1,void*e1_c_type,void*aggrtype,struct _fat_ptr*f,int in_lhs,struct Cyc_Absyn_Exp*(*aggrproj)(struct Cyc_Absyn_Exp*,struct _fat_ptr*,unsigned))=Cyc_Toc_check_tagged_union;struct Cyc_Absyn_Exp*_tmp745=e1;void*_tmp746=({Cyc_Toc_typ_to_c(e1typ);});void*_tmp747=ta;struct _fat_ptr*_tmp748=f;int _tmp749=({Cyc_Toc_in_lhs(nv);});struct Cyc_Absyn_Exp*(*_tmp74A)(struct Cyc_Absyn_Exp*,struct _fat_ptr*,unsigned)=Cyc_Absyn_aggrarrow_exp;_tmp744(_tmp745,_tmp746,_tmp747,_tmp748,_tmp749,_tmp74A);});e->r=_tmpE9C;});
# 3422
if(
# 3425
is_poly && is_read)
({void*_tmpE9D=({struct Cyc_Absyn_Exp*(*_tmp74B)(void*t,struct Cyc_Absyn_Exp*e,unsigned l)=Cyc_Toc_array_to_ptr_cast;void*_tmp74C=({Cyc_Toc_typ_to_c((void*)_check_null(e->topt));});struct Cyc_Absyn_Exp*_tmp74D=({Cyc_Absyn_new_exp(e->r,0U);});unsigned _tmp74E=0U;_tmp74B(_tmp74C,_tmp74D,_tmp74E);})->r;e->r=_tmpE9D;});
# 3422
goto _LL7;}case 20U: _LL3C: _tmp565=((struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_tmp545)->f1;_LL3D: {struct Cyc_Absyn_Exp*e1=_tmp565;
# 3429
did_inserted_checks=1;
({Cyc_Toc_ptr_use_to_c(nv,e1,e->annot,0);});
goto _LL7;}case 23U: _LL3E: _tmp563=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp545)->f1;_tmp564=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp545)->f2;_LL3F: {struct Cyc_Absyn_Exp*e1=_tmp563;struct Cyc_Absyn_Exp*e2=_tmp564;
# 3433
{void*_stmttmp50=({Cyc_Tcutil_compress((void*)_check_null(e1->topt));});void*_tmp74F=_stmttmp50;struct Cyc_List_List*_tmp750;if(((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp74F)->tag == 6U){_LLE6: _tmp750=((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp74F)->f1;_LLE7: {struct Cyc_List_List*ts=_tmp750;
# 3435
({Cyc_Toc_exp_to_c(nv,e1);});{
int old_lhs=({Cyc_Toc_in_lhs(nv);});
({Cyc_Toc_set_lhs(nv,0);});
({Cyc_Toc_exp_to_c(nv,e2);});{
struct _tuple15 _stmttmp51=({Cyc_Evexp_eval_const_uint_exp(e2);});int _tmp752;unsigned _tmp751;_LLEB: _tmp751=_stmttmp51.f1;_tmp752=_stmttmp51.f2;_LLEC: {unsigned i=_tmp751;int known=_tmp752;
if(!known)
({void*_tmp753=0U;({int(*_tmpE9F)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp755)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp755;});struct _fat_ptr _tmpE9E=({const char*_tmp754="unknown tuple subscript in translation to C";_tag_fat(_tmp754,sizeof(char),44U);});_tmpE9F(_tmpE9E,_tag_fat(_tmp753,sizeof(void*),0U));});});
# 3440
({void*_tmpEA0=({void*(*_tmp756)(struct Cyc_Absyn_Exp*e,struct _fat_ptr*n)=Cyc_Toc_aggrmember_exp_r;struct Cyc_Absyn_Exp*_tmp757=e1;struct _fat_ptr*_tmp758=({Cyc_Absyn_fieldname((int)(i + (unsigned)1));});_tmp756(_tmp757,_tmp758);});e->r=_tmpEA0;});
# 3443
goto _LLE5;}}}}}else{_LLE8: _LLE9:
# 3445
 did_inserted_checks=1;{
int index_used=({Cyc_Toc_ptr_use_to_c(nv,e1,e->annot,e2);});
if(index_used)
({void*_tmpEA1=({Cyc_Toc_deref_exp_r(e1);});e->r=_tmpEA1;});
# 3447
goto _LLE5;}}_LLE5:;}
# 3451
goto _LL7;}case 24U: _LL40: _tmp562=((struct Cyc_Absyn_Tuple_e_Absyn_Raw_exp_struct*)_tmp545)->f1;_LL41: {struct Cyc_List_List*es=_tmp562;
# 3453
if(!({Cyc_Toc_is_toplevel(nv);})){
# 3455
void*strct_typ=({void*(*_tmp763)(struct Cyc_List_List*tqs0)=Cyc_Toc_add_tuple_type;struct Cyc_List_List*_tmp764=({({struct Cyc_List_List*(*_tmpEA2)(struct _tuple14*(*f)(struct Cyc_Absyn_Exp*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp765)(struct _tuple14*(*f)(struct Cyc_Absyn_Exp*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct _tuple14*(*f)(struct Cyc_Absyn_Exp*),struct Cyc_List_List*x))Cyc_List_map;_tmp765;});_tmpEA2(Cyc_Toc_tup_to_c,es);});});_tmp763(_tmp764);});
{void*_tmp759=strct_typ;union Cyc_Absyn_AggrInfo _tmp75A;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp759)->tag == 0U){if(((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp759)->f1)->tag == 22U){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp759)->f2 == 0){_LLEE: _tmp75A=((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp759)->f1)->f1;_LLEF: {union Cyc_Absyn_AggrInfo aggrinfo=_tmp75A;
# 3458
struct Cyc_List_List*dles=0;
struct Cyc_Absyn_Aggrdecl*sd=({Cyc_Absyn_get_known_aggrdecl(aggrinfo);});
{int i=1;for(0;es != 0;(es=es->tl,++ i)){
({Cyc_Toc_exp_to_c(nv,(struct Cyc_Absyn_Exp*)es->hd);});
dles=({struct Cyc_List_List*_tmp75E=_cycalloc(sizeof(*_tmp75E));({struct _tuple21*_tmpEA3=({struct _tuple21*(*_tmp75B)(struct _fat_ptr*name,struct Cyc_Absyn_Exp*e)=Cyc_Toc_make_field;struct _fat_ptr*_tmp75C=({Cyc_Absyn_fieldname(i);});struct Cyc_Absyn_Exp*_tmp75D=(struct Cyc_Absyn_Exp*)es->hd;_tmp75B(_tmp75C,_tmp75D);});_tmp75E->hd=_tmpEA3;}),_tmp75E->tl=dles;_tmp75E;});}}
# 3464
({void*_tmpEA5=(void*)({struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*_tmp75F=_cycalloc(sizeof(*_tmp75F));_tmp75F->tag=29U,_tmp75F->f1=sd->name,_tmp75F->f2=0,({struct Cyc_List_List*_tmpEA4=({Cyc_List_imp_rev(dles);});_tmp75F->f3=_tmpEA4;}),_tmp75F->f4=sd;_tmp75F;});e->r=_tmpEA5;});
e->topt=strct_typ;
goto _LLED;}}else{goto _LLF0;}}else{goto _LLF0;}}else{_LLF0: _LLF1:
({void*_tmp760=0U;({int(*_tmpEA7)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp762)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmp762;});struct _fat_ptr _tmpEA6=({const char*_tmp761="tuple type not an aggregate";_tag_fat(_tmp761,sizeof(char),28U);});_tmpEA7(_tmpEA6,_tag_fat(_tmp760,sizeof(void*),0U));});});}_LLED:;}
# 3469
goto _LL7;}else{
# 3473
struct Cyc_List_List*dles=0;
for(0;es != 0;es=es->tl){
({Cyc_Toc_exp_to_c(nv,(struct Cyc_Absyn_Exp*)es->hd);});
dles=({struct Cyc_List_List*_tmp767=_cycalloc(sizeof(*_tmp767));({struct _tuple21*_tmpEA8=({struct _tuple21*_tmp766=_cycalloc(sizeof(*_tmp766));_tmp766->f1=0,_tmp766->f2=(struct Cyc_Absyn_Exp*)es->hd;_tmp766;});_tmp767->hd=_tmpEA8;}),_tmp767->tl=dles;_tmp767;});}
# 3478
({void*_tmpEA9=({void*(*_tmp768)(struct Cyc_Core_Opt*tdopt,struct Cyc_List_List*ds)=Cyc_Toc_unresolvedmem_exp_r;struct Cyc_Core_Opt*_tmp769=0;struct Cyc_List_List*_tmp76A=({Cyc_List_imp_rev(dles);});_tmp768(_tmp769,_tmp76A);});e->r=_tmpEA9;});}
# 3480
goto _LL7;}case 26U: _LL42: _tmp561=((struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*)_tmp545)->f1;_LL43: {struct Cyc_List_List*dles0=_tmp561;
# 3486
{struct Cyc_List_List*dles=dles0;for(0;dles != 0;dles=dles->tl){
({Cyc_Toc_exp_to_c(nv,(*((struct _tuple21*)dles->hd)).f2);});}}
if(({Cyc_Toc_is_toplevel(nv);}))
({void*_tmpEAA=({Cyc_Toc_unresolvedmem_exp_r(0,dles0);});e->r=_tmpEAA;});
# 3488
goto _LL7;}case 27U: _LL44: _tmp55D=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmp545)->f1;_tmp55E=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmp545)->f2;_tmp55F=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmp545)->f3;_tmp560=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmp545)->f4;_LL45: {struct Cyc_Absyn_Vardecl*vd=_tmp55D;struct Cyc_Absyn_Exp*e1=_tmp55E;struct Cyc_Absyn_Exp*e2=_tmp55F;int iszeroterm=_tmp560;
# 3494
({Cyc_Toc_exp_to_c(nv,e1);});
({Cyc_Toc_exp_to_c(nv,e2);});
if(({Cyc_Toc_is_toplevel(nv);})){
struct _tuple15 _stmttmp52=({Cyc_Evexp_eval_const_uint_exp(e1);});int _tmp76C;unsigned _tmp76B;_LLF3: _tmp76B=_stmttmp52.f1;_tmp76C=_stmttmp52.f2;_LLF4: {unsigned sz=_tmp76B;int known=_tmp76C;
void*elt_typ=({Cyc_Toc_typ_to_c((void*)_check_null(e2->topt));});
struct Cyc_List_List*es=0;
# 3501
if(!({Cyc_Toc_is_zero(e2);})){
if(!known)
({void*_tmp76D=0U;({unsigned _tmpEAC=e1->loc;struct _fat_ptr _tmpEAB=({const char*_tmp76E="cannot determine value of constant";_tag_fat(_tmp76E,sizeof(char),35U);});Cyc_Tcutil_terr(_tmpEAC,_tmpEAB,_tag_fat(_tmp76D,sizeof(void*),0U));});});
# 3502
{
# 3504
unsigned i=0U;
# 3502
for(0;i < sz;++ i){
# 3505
es=({struct Cyc_List_List*_tmp770=_cycalloc(sizeof(*_tmp770));({struct _tuple21*_tmpEAD=({struct _tuple21*_tmp76F=_cycalloc(sizeof(*_tmp76F));_tmp76F->f1=0,_tmp76F->f2=e2;_tmp76F;});_tmp770->hd=_tmpEAD;}),_tmp770->tl=es;_tmp770;});}}
# 3507
if(iszeroterm){
struct Cyc_Absyn_Exp*ezero=({struct Cyc_Absyn_Exp*(*_tmp773)(void*t,struct Cyc_Absyn_Exp*e)=Cyc_Toc_cast_it;void*_tmp774=elt_typ;struct Cyc_Absyn_Exp*_tmp775=({Cyc_Absyn_uint_exp(0U,0U);});_tmp773(_tmp774,_tmp775);});
es=({({struct Cyc_List_List*_tmpEAF=es;Cyc_List_imp_append(_tmpEAF,({struct Cyc_List_List*_tmp772=_cycalloc(sizeof(*_tmp772));({struct _tuple21*_tmpEAE=({struct _tuple21*_tmp771=_cycalloc(sizeof(*_tmp771));_tmp771->f1=0,_tmp771->f2=ezero;_tmp771;});_tmp772->hd=_tmpEAE;}),_tmp772->tl=0;_tmp772;}));});});}}
# 3501
({void*_tmpEB0=({Cyc_Toc_unresolvedmem_exp_r(0,es);});e->r=_tmpEB0;});}}
# 3496
goto _LL7;}case 28U: _LL46: _tmp55A=((struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*)_tmp545)->f1;_tmp55B=(void*)((struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*)_tmp545)->f2;_tmp55C=((struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*)_tmp545)->f3;_LL47: {struct Cyc_Absyn_Exp*e1=_tmp55A;void*t1=_tmp55B;int iszeroterm=_tmp55C;
# 3521
if(({Cyc_Toc_is_toplevel(nv);}))
({void*_tmpEB1=({Cyc_Toc_unresolvedmem_exp_r(0,0);});e->r=_tmpEB1;});else{
# 3524
({Cyc_Toc_exp_to_c(nv,e1);});}
goto _LL7;}case 30U: _LL48: _tmp558=(void*)((struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct*)_tmp545)->f1;_tmp559=((struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct*)_tmp545)->f2;_LL49: {void*st=_tmp558;struct Cyc_List_List*dles=_tmp559;
# 3528
{struct Cyc_List_List*dles2=dles;for(0;dles2 != 0;dles2=dles2->tl){
({Cyc_Toc_exp_to_c(nv,(*((struct _tuple21*)dles2->hd)).f2);});}}{
void*strct_typ=({Cyc_Toc_typ_to_c((void*)_check_null(e->topt));});
e->topt=strct_typ;
if(!({Cyc_Toc_is_toplevel(nv);})){
void*_stmttmp53=({Cyc_Tcutil_compress(strct_typ);});void*_tmp776=_stmttmp53;union Cyc_Absyn_AggrInfo _tmp777;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp776)->tag == 0U){if(((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp776)->f1)->tag == 22U){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp776)->f2 == 0){_LLF6: _tmp777=((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp776)->f1)->f1;_LLF7: {union Cyc_Absyn_AggrInfo aggrinfo=_tmp777;
# 3535
struct Cyc_Absyn_Aggrdecl*sd=({Cyc_Absyn_get_known_aggrdecl(aggrinfo);});
({void*_tmpEB2=(void*)({struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*_tmp778=_cycalloc(sizeof(*_tmp778));_tmp778->tag=29U,_tmp778->f1=sd->name,_tmp778->f2=0,_tmp778->f3=dles,_tmp778->f4=sd;_tmp778;});e->r=_tmpEB2;});
e->topt=strct_typ;
goto _LLF5;}}else{goto _LLF8;}}else{goto _LLF8;}}else{_LLF8: _LLF9:
({void*_tmp779=0U;({int(*_tmpEB4)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp77B)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmp77B;});struct _fat_ptr _tmpEB3=({const char*_tmp77A="anonStruct type not an aggregate";_tag_fat(_tmp77A,sizeof(char),33U);});_tmpEB4(_tmpEB3,_tag_fat(_tmp779,sizeof(void*),0U));});});}_LLF5:;}else{
# 3542
({void*_tmpEB5=({Cyc_Toc_unresolvedmem_exp_r(0,dles);});e->r=_tmpEB5;});}
goto _LL7;}}case 29U: _LL4A: _tmp554=(struct _tuple1**)&((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_tmp545)->f1;_tmp555=((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_tmp545)->f2;_tmp556=((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_tmp545)->f3;_tmp557=((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_tmp545)->f4;_LL4B: {struct _tuple1**tdn=_tmp554;struct Cyc_List_List*exist_ts=_tmp555;struct Cyc_List_List*dles=_tmp556;struct Cyc_Absyn_Aggrdecl*sd=_tmp557;
# 3548
if(sd == 0 || sd->impl == 0)
({struct Cyc_String_pa_PrintArg_struct _tmp77F=({struct Cyc_String_pa_PrintArg_struct _tmpC93;_tmpC93.tag=0U,({struct _fat_ptr _tmpEB6=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_exp2string(e);}));_tmpC93.f1=_tmpEB6;});_tmpC93;});void*_tmp77C[1U];_tmp77C[0]=& _tmp77F;({int(*_tmpEB8)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp77E)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmp77E;});struct _fat_ptr _tmpEB7=({const char*_tmp77D="exp_to_c, Aggregate_e: missing aggrdecl pointer or fields: \"%s\"";_tag_fat(_tmp77D,sizeof(char),64U);});_tmpEB8(_tmpEB7,_tag_fat(_tmp77C,sizeof(void*),1U));});});{
# 3548
void*new_typ=({Cyc_Toc_typ_to_c(old_typ);});
# 3551
{void*_stmttmp54=({Cyc_Tcutil_compress(new_typ);});void*_tmp780=_stmttmp54;union Cyc_Absyn_AggrInfo _tmp781;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp780)->tag == 0U){if(((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp780)->f1)->tag == 22U){_LLFB: _tmp781=((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp780)->f1)->f1;_LLFC: {union Cyc_Absyn_AggrInfo info=_tmp781;
({struct _tuple1*_tmpEB9=({Cyc_Absyn_aggr_kinded_name(info);}).f2;*tdn=_tmpEB9;});goto _LLFA;}}else{goto _LLFD;}}else{_LLFD: _LLFE:
({void*_tmp782=0U;({int(*_tmpEBB)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp784)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmp784;});struct _fat_ptr _tmpEBA=({const char*_tmp783="exp_to_c, Aggregate_e: bad type translation";_tag_fat(_tmp783,sizeof(char),44U);});_tmpEBB(_tmpEBA,_tag_fat(_tmp782,sizeof(void*),0U));});});}_LLFA:;}{
# 3559
struct Cyc_List_List*typ_fields=((struct Cyc_Absyn_AggrdeclImpl*)_check_null(sd->impl))->fields;
if(typ_fields == 0)goto _LL7;for(0;((struct Cyc_List_List*)_check_null(typ_fields))->tl != 0;typ_fields=typ_fields->tl){
# 3562
;}{
struct Cyc_Absyn_Aggrfield*last_typ_field=(struct Cyc_Absyn_Aggrfield*)typ_fields->hd;
struct Cyc_List_List*fields=({Cyc_Tcutil_resolve_aggregate_designators(Cyc_Core_heap_region,e->loc,dles,sd->kind,((struct Cyc_Absyn_AggrdeclImpl*)_check_null(sd->impl))->fields);});
# 3566
if(!({Cyc_Toc_is_toplevel(nv);})){
void*_stmttmp55=({Cyc_Tcutil_compress(old_typ);});void*_tmp785=_stmttmp55;struct Cyc_List_List*_tmp786;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp785)->tag == 0U){if(((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp785)->f1)->tag == 22U){_LL100: _tmp786=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp785)->f2;_LL101: {struct Cyc_List_List*param_ts=_tmp786;
# 3570
{struct Cyc_List_List*fields2=fields;for(0;fields2 != 0;fields2=fields2->tl){
struct _tuple43*_stmttmp56=(struct _tuple43*)fields2->hd;struct Cyc_Absyn_Exp*_tmp788;struct Cyc_Absyn_Aggrfield*_tmp787;_LL105: _tmp787=_stmttmp56->f1;_tmp788=_stmttmp56->f2;_LL106: {struct Cyc_Absyn_Aggrfield*aggrfield=_tmp787;struct Cyc_Absyn_Exp*fieldexp=_tmp788;
void*old_field_typ=fieldexp->topt;
({Cyc_Toc_exp_to_c(nv,fieldexp);});
# 3575
if(({Cyc_Toc_is_void_star_or_boxed_tvar(aggrfield->type);})&& !({Cyc_Toc_is_pointer_or_boxed_tvar((void*)_check_null(fieldexp->topt));}))
# 3577
({void*_tmpEBC=({struct Cyc_Absyn_Exp*(*_tmp789)(void*t,struct Cyc_Absyn_Exp*e)=Cyc_Toc_cast_it;void*_tmp78A=({Cyc_Toc_typ_to_c(aggrfield->type);});struct Cyc_Absyn_Exp*_tmp78B=({Cyc_Absyn_copy_exp(fieldexp);});_tmp789(_tmp78A,_tmp78B);})->r;fieldexp->r=_tmpEBC;});
# 3575
if(((struct Cyc_Absyn_AggrdeclImpl*)_check_null(sd->impl))->tagged){
# 3581
struct _fat_ptr*n=aggrfield->name;
struct Cyc_Absyn_Exp*tag_exp=({struct Cyc_Absyn_Exp*(*_tmp79C)(unsigned,unsigned)=Cyc_Absyn_uint_exp;unsigned _tmp79D=(unsigned)({Cyc_Toc_get_member_offset(sd,n);});unsigned _tmp79E=0U;_tmp79C(_tmp79D,_tmp79E);});
struct _tuple21*tag_dle=({Cyc_Toc_make_field(Cyc_Toc_tag_sp,tag_exp);});
struct _tuple21*val_dle=({Cyc_Toc_make_field(Cyc_Toc_val_sp,fieldexp);});
struct _tuple1*s=({struct _tuple1*_tmp79B=_cycalloc(sizeof(*_tmp79B));({union Cyc_Absyn_Nmspace _tmpEC0=({Cyc_Absyn_Abs_n(0,1);});_tmp79B->f1=_tmpEC0;}),({
struct _fat_ptr*_tmpEBF=({struct _fat_ptr*_tmp79A=_cycalloc(sizeof(*_tmp79A));({struct _fat_ptr _tmpEBE=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp798=({struct Cyc_String_pa_PrintArg_struct _tmpC95;_tmpC95.tag=0U,_tmpC95.f1=(struct _fat_ptr)((struct _fat_ptr)*(*sd->name).f2);_tmpC95;});struct Cyc_String_pa_PrintArg_struct _tmp799=({struct Cyc_String_pa_PrintArg_struct _tmpC94;_tmpC94.tag=0U,_tmpC94.f1=(struct _fat_ptr)((struct _fat_ptr)*n);_tmpC94;});void*_tmp796[2U];_tmp796[0]=& _tmp798,_tmp796[1]=& _tmp799;({struct _fat_ptr _tmpEBD=({const char*_tmp797="_union_%s_%s";_tag_fat(_tmp797,sizeof(char),13U);});Cyc_aprintf(_tmpEBD,_tag_fat(_tmp796,sizeof(void*),2U));});});*_tmp79A=_tmpEBE;});_tmp79A;});_tmp79B->f2=_tmpEBF;});_tmp79B;});
# 3588
struct _tuple21*u_dle=({struct _tuple21*(*_tmp78E)(struct _fat_ptr*name,struct Cyc_Absyn_Exp*e)=Cyc_Toc_make_field;struct _fat_ptr*_tmp78F=n;struct Cyc_Absyn_Exp*_tmp790=({struct Cyc_Absyn_Exp*(*_tmp791)(void*,unsigned)=Cyc_Absyn_new_exp;void*_tmp792=(void*)({struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*_tmp794=_cycalloc(sizeof(*_tmp794));
_tmp794->tag=29U,_tmp794->f1=s,_tmp794->f2=0,({struct Cyc_List_List*_tmpEC1=({struct _tuple21*_tmp793[2U];_tmp793[0]=tag_dle,_tmp793[1]=val_dle;Cyc_List_list(_tag_fat(_tmp793,sizeof(struct _tuple21*),2U));});_tmp794->f3=_tmpEC1;}),_tmp794->f4=0;_tmp794;});unsigned _tmp795=0U;_tmp791(_tmp792,_tmp795);});_tmp78E(_tmp78F,_tmp790);});
# 3591
({void*_tmpEC3=(void*)({struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*_tmp78D=_cycalloc(sizeof(*_tmp78D));_tmp78D->tag=29U,_tmp78D->f1=*tdn,_tmp78D->f2=0,({struct Cyc_List_List*_tmpEC2=({struct _tuple21*_tmp78C[1U];_tmp78C[0]=u_dle;Cyc_List_list(_tag_fat(_tmp78C,sizeof(struct _tuple21*),1U));});_tmp78D->f3=_tmpEC2;}),_tmp78D->f4=sd;_tmp78D;});e->r=_tmpEC3;});}
# 3575
if(
# 3597
({Cyc_Toc_is_abstract_type(old_typ);})&& last_typ_field == aggrfield){
struct Cyc_List_List*inst=({struct Cyc_List_List*(*_tmp7B9)(struct Cyc_List_List*x,struct Cyc_List_List*y)=Cyc_List_append;struct Cyc_List_List*_tmp7BA=({Cyc_List_zip(((struct Cyc_Absyn_AggrdeclImpl*)_check_null(sd->impl))->exist_vars,exist_ts);});struct Cyc_List_List*_tmp7BB=({Cyc_List_zip(sd->tvs,param_ts);});_tmp7B9(_tmp7BA,_tmp7BB);});
# 3600
struct Cyc_List_List*new_fields=0;
{struct Cyc_List_List*fs=((struct Cyc_Absyn_AggrdeclImpl*)_check_null(sd->impl))->fields;for(0;fs != 0;fs=fs->tl){
struct Cyc_Absyn_Aggrfield*old_f=(struct Cyc_Absyn_Aggrfield*)fs->hd;
void*old_ftyp=({Cyc_Tcutil_substitute(inst,old_f->type);});
struct Cyc_Absyn_Aggrfield*new_f=({struct Cyc_Absyn_Aggrfield*(*_tmp7A9)(struct Cyc_Absyn_Aggrfield*f,void*new_type)=Cyc_Toc_aggrfield_to_c;struct Cyc_Absyn_Aggrfield*_tmp7AA=old_f;void*_tmp7AB=({void*(*_tmp7AC)(void*t)=Cyc_Toc_typ_to_c;void*_tmp7AD=({Cyc_Tcutil_substitute(inst,old_ftyp);});_tmp7AC(_tmp7AD);});_tmp7A9(_tmp7AA,_tmp7AB);});
# 3606
new_fields=({struct Cyc_List_List*_tmp79F=_cycalloc(sizeof(*_tmp79F));_tmp79F->hd=new_f,_tmp79F->tl=new_fields;_tmp79F;});
# 3612
if(fs->tl == 0){
{void*_stmttmp57=({Cyc_Tcutil_compress(new_f->type);});void*_tmp7A0=_stmttmp57;struct Cyc_Absyn_ArrayInfo _tmp7A1;if(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp7A0)->tag == 4U){_LL108: _tmp7A1=((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp7A0)->f1;_LL109: {struct Cyc_Absyn_ArrayInfo ai=_tmp7A1;
# 3615
if(!({Cyc_Evexp_c_can_eval((struct Cyc_Absyn_Exp*)_check_null(ai.num_elts));})){
struct Cyc_Absyn_ArrayInfo ai2=ai;
({struct Cyc_Absyn_Exp*_tmpEC4=({Cyc_Absyn_uint_exp(0U,0U);});ai2.num_elts=_tmpEC4;});
({void*_tmpEC5=(void*)({struct Cyc_Absyn_ArrayType_Absyn_Type_struct*_tmp7A2=_cycalloc(sizeof(*_tmp7A2));_tmp7A2->tag=4U,_tmp7A2->f1=ai2;_tmp7A2;});new_f->type=_tmpEC5;});}
# 3615
goto _LL107;}}else{_LL10A: _LL10B:
# 3624
 if(fieldexp->topt == 0)
goto _LL107;
# 3624
{void*_stmttmp58=({Cyc_Tcutil_compress((void*)_check_null(fieldexp->topt));});void*_tmp7A3=_stmttmp58;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp7A3)->tag == 0U){if(((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp7A3)->f1)->tag == 22U){_LL10D: _LL10E:
# 3628
 new_f->type=(void*)_check_null(fieldexp->topt);goto _LL10C;}else{goto _LL10F;}}else{_LL10F: _LL110:
 goto _LL10C;}_LL10C:;}
# 3631
goto _LL107;}_LL107:;}
# 3635
if(!({Cyc_Tcutil_is_array_type(old_f->type);})&&({int(*_tmp7A4)(struct Cyc_Absyn_Kind*,struct Cyc_Absyn_Kind*)=Cyc_Tcutil_kind_leq;struct Cyc_Absyn_Kind*_tmp7A5=& Cyc_Tcutil_ak;struct Cyc_Absyn_Kind*_tmp7A6=({Cyc_Tcutil_type_kind(old_f->type);});_tmp7A4(_tmp7A5,_tmp7A6);}))
# 3637
({struct Cyc_List_List*_tmpEC7=({struct Cyc_List_List*_tmp7A8=_cycalloc(sizeof(*_tmp7A8));({void*_tmpEC6=(void*)({struct Cyc_Absyn_Aligned_att_Absyn_Attribute_struct*_tmp7A7=_cycalloc(sizeof(*_tmp7A7));_tmp7A7->tag=6U,_tmp7A7->f1=0;_tmp7A7;});_tmp7A8->hd=_tmpEC6;}),_tmp7A8->tl=new_f->attributes;_tmp7A8;});new_f->attributes=_tmpEC7;});}}}
# 3641
sd=({struct Cyc_Absyn_Aggrdecl*(*_tmp7AE)(struct _fat_ptr*name,struct Cyc_List_List*fs)=Cyc_Toc_make_c_struct_defn;struct _fat_ptr*_tmp7AF=({struct _fat_ptr*_tmp7B3=_cycalloc(sizeof(*_tmp7B3));({struct _fat_ptr _tmpEC9=(struct _fat_ptr)({struct Cyc_Int_pa_PrintArg_struct _tmp7B2=({struct Cyc_Int_pa_PrintArg_struct _tmpC96;_tmpC96.tag=1U,_tmpC96.f1=(unsigned long)Cyc_Toc_tuple_type_counter ++;_tmpC96;});void*_tmp7B0[1U];_tmp7B0[0]=& _tmp7B2;({struct _fat_ptr _tmpEC8=({const char*_tmp7B1="_genStruct%d";_tag_fat(_tmp7B1,sizeof(char),13U);});Cyc_aprintf(_tmpEC8,_tag_fat(_tmp7B0,sizeof(void*),1U));});});*_tmp7B3=_tmpEC9;});_tmp7B3;});struct Cyc_List_List*_tmp7B4=({Cyc_List_imp_rev(new_fields);});_tmp7AE(_tmp7AF,_tmp7B4);});
# 3644
*tdn=sd->name;
({Cyc_Toc_aggrdecl_to_c(sd);});
# 3647
({void*_tmpECA=({void*(*_tmp7B5)(union Cyc_Absyn_AggrInfo,struct Cyc_List_List*args)=Cyc_Absyn_aggr_type;union Cyc_Absyn_AggrInfo _tmp7B6=({Cyc_Absyn_KnownAggr(({struct Cyc_Absyn_Aggrdecl**_tmp7B7=_cycalloc(sizeof(*_tmp7B7));*_tmp7B7=sd;_tmp7B7;}));});struct Cyc_List_List*_tmp7B8=0;_tmp7B5(_tmp7B6,_tmp7B8);});e->topt=_tmpECA;});}}}}
# 3650
goto _LLFF;}}else{goto _LL102;}}else{_LL102: _LL103:
({void*_tmp7BC=0U;({int(*_tmpECC)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp7BE)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmp7BE;});struct _fat_ptr _tmpECB=({const char*_tmp7BD="exp_to_c, Aggregate_e: bad struct type";_tag_fat(_tmp7BD,sizeof(char),39U);});_tmpECC(_tmpECB,_tag_fat(_tmp7BC,sizeof(void*),0U));});});}_LLFF:;}else{
# 3659
if(((struct Cyc_Absyn_AggrdeclImpl*)_check_null(sd->impl))->tagged){
# 3661
struct _tuple43*_stmttmp59=(struct _tuple43*)((struct Cyc_List_List*)_check_null(fields))->hd;struct Cyc_Absyn_Exp*_tmp7C0;struct Cyc_Absyn_Aggrfield*_tmp7BF;_LL112: _tmp7BF=_stmttmp59->f1;_tmp7C0=_stmttmp59->f2;_LL113: {struct Cyc_Absyn_Aggrfield*field=_tmp7BF;struct Cyc_Absyn_Exp*fieldexp=_tmp7C0;
void*fieldexp_type=(void*)_check_null(fieldexp->topt);
void*fieldtyp=field->type;
({Cyc_Toc_exp_to_c(nv,fieldexp);});
if(({Cyc_Toc_is_void_star_or_boxed_tvar(fieldtyp);})&& !({Cyc_Toc_is_void_star_or_boxed_tvar(fieldexp_type);}))
# 3667
({void*_tmpECD=({void*(*_tmp7C1)(void*t,struct Cyc_Absyn_Exp*e)=Cyc_Toc_cast_it_r;void*_tmp7C2=({Cyc_Toc_void_star_type();});struct Cyc_Absyn_Exp*_tmp7C3=({Cyc_Absyn_new_exp(fieldexp->r,0U);});_tmp7C1(_tmp7C2,_tmp7C3);});fieldexp->r=_tmpECD;});{
# 3665
int i=({Cyc_Toc_get_member_offset(sd,field->name);});
# 3671
struct Cyc_Absyn_Exp*field_tag_exp=({Cyc_Absyn_signed_int_exp(i,0U);});
struct Cyc_List_List*newdles=({struct _tuple21*_tmp7C8[2U];({struct _tuple21*_tmpECF=({struct _tuple21*_tmp7C9=_cycalloc(sizeof(*_tmp7C9));_tmp7C9->f1=0,_tmp7C9->f2=field_tag_exp;_tmp7C9;});_tmp7C8[0]=_tmpECF;}),({struct _tuple21*_tmpECE=({struct _tuple21*_tmp7CA=_cycalloc(sizeof(*_tmp7CA));_tmp7CA->f1=0,_tmp7CA->f2=fieldexp;_tmp7CA;});_tmp7C8[1]=_tmpECE;});Cyc_List_list(_tag_fat(_tmp7C8,sizeof(struct _tuple21*),2U));});
struct Cyc_Absyn_Exp*umem=({Cyc_Absyn_unresolvedmem_exp(0,newdles,0U);});
({void*_tmpED1=({void*(*_tmp7C4)(struct Cyc_Core_Opt*tdopt,struct Cyc_List_List*ds)=Cyc_Toc_unresolvedmem_exp_r;struct Cyc_Core_Opt*_tmp7C5=0;struct Cyc_List_List*_tmp7C6=({struct _tuple21*_tmp7C7[1U];({struct _tuple21*_tmpED0=({Cyc_Toc_make_field(field->name,umem);});_tmp7C7[0]=_tmpED0;});Cyc_List_list(_tag_fat(_tmp7C7,sizeof(struct _tuple21*),1U));});_tmp7C4(_tmp7C5,_tmp7C6);});e->r=_tmpED1;});}}}else{
# 3677
struct Cyc_List_List*newdles=0;
struct Cyc_List_List*sdfields=((struct Cyc_Absyn_AggrdeclImpl*)_check_null(sd->impl))->fields;
for(0;sdfields != 0;sdfields=sdfields->tl){
struct Cyc_List_List*fields2=fields;for(0;fields2 != 0;fields2=fields2->tl){
if((*((struct _tuple43*)fields2->hd)).f1 == (struct Cyc_Absyn_Aggrfield*)sdfields->hd){
struct _tuple43*_stmttmp5A=(struct _tuple43*)fields2->hd;struct Cyc_Absyn_Exp*_tmp7CC;struct Cyc_Absyn_Aggrfield*_tmp7CB;_LL115: _tmp7CB=_stmttmp5A->f1;_tmp7CC=_stmttmp5A->f2;_LL116: {struct Cyc_Absyn_Aggrfield*field=_tmp7CB;struct Cyc_Absyn_Exp*fieldexp=_tmp7CC;
void*fieldtyp=({Cyc_Toc_typ_to_c(field->type);});
void*fieldexp_typ=({Cyc_Toc_typ_to_c((void*)_check_null(fieldexp->topt));});
({Cyc_Toc_exp_to_c(nv,fieldexp);});
# 3687
if(!({Cyc_Unify_unify(fieldtyp,fieldexp_typ);})){
# 3689
if(!({Cyc_Tcutil_is_arithmetic_type(fieldtyp);})|| !({Cyc_Tcutil_is_arithmetic_type(fieldexp_typ);}))
# 3691
fieldexp=({struct Cyc_Absyn_Exp*(*_tmp7CD)(void*t,struct Cyc_Absyn_Exp*e)=Cyc_Toc_cast_it;void*_tmp7CE=fieldtyp;struct Cyc_Absyn_Exp*_tmp7CF=({Cyc_Absyn_copy_exp(fieldexp);});_tmp7CD(_tmp7CE,_tmp7CF);});}
# 3687
newdles=({struct Cyc_List_List*_tmp7D1=_cycalloc(sizeof(*_tmp7D1));
# 3692
({struct _tuple21*_tmpED2=({struct _tuple21*_tmp7D0=_cycalloc(sizeof(*_tmp7D0));_tmp7D0->f1=0,_tmp7D0->f2=fieldexp;_tmp7D0;});_tmp7D1->hd=_tmpED2;}),_tmp7D1->tl=newdles;_tmp7D1;});
break;}}}}
# 3696
({void*_tmpED3=({void*(*_tmp7D2)(struct Cyc_Core_Opt*tdopt,struct Cyc_List_List*ds)=Cyc_Toc_unresolvedmem_exp_r;struct Cyc_Core_Opt*_tmp7D3=0;struct Cyc_List_List*_tmp7D4=({Cyc_List_imp_rev(newdles);});_tmp7D2(_tmp7D3,_tmp7D4);});e->r=_tmpED3;});}}
# 3699
goto _LL7;}}}}case 31U: _LL4C: _tmp551=((struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*)_tmp545)->f1;_tmp552=((struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*)_tmp545)->f2;_tmp553=((struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*)_tmp545)->f3;_LL4D: {struct Cyc_List_List*es=_tmp551;struct Cyc_Absyn_Datatypedecl*tud=_tmp552;struct Cyc_Absyn_Datatypefield*tuf=_tmp553;
# 3703
struct Cyc_List_List*tqts=tuf->typs;
{struct Cyc_List_List*es2=es;for(0;es2 != 0;(es2=es2->tl,tqts=tqts->tl)){
struct Cyc_Absyn_Exp*cur_e=(struct Cyc_Absyn_Exp*)es2->hd;
void*cur_e_typ=(void*)_check_null(cur_e->topt);
void*field_typ=({Cyc_Toc_typ_to_c((*((struct _tuple14*)((struct Cyc_List_List*)_check_null(tqts))->hd)).f2);});
({Cyc_Toc_exp_to_c(nv,cur_e);});
if(({Cyc_Toc_is_void_star_or_boxed_tvar(field_typ);})&& !({Cyc_Toc_is_pointer_or_boxed_tvar(cur_e_typ);}))
# 3711
({void*_tmpED4=({Cyc_Toc_cast_it(field_typ,cur_e);})->r;cur_e->r=_tmpED4;});}}{
# 3714
struct Cyc_Absyn_Exp*tag_exp=
tud->is_extensible?({Cyc_Absyn_var_exp(tuf->name,0U);}):({Cyc_Toc_datatype_tag(tud,tuf->name);});
# 3717
if(!({Cyc_Toc_is_toplevel(nv);})){
# 3719
struct Cyc_List_List*dles=0;
{int i=1;for(0;es != 0;(es=es->tl,++ i)){
dles=({struct Cyc_List_List*_tmp7D8=_cycalloc(sizeof(*_tmp7D8));({struct _tuple21*_tmpED5=({struct _tuple21*(*_tmp7D5)(struct _fat_ptr*name,struct Cyc_Absyn_Exp*e)=Cyc_Toc_make_field;struct _fat_ptr*_tmp7D6=({Cyc_Absyn_fieldname(i);});struct Cyc_Absyn_Exp*_tmp7D7=(struct Cyc_Absyn_Exp*)es->hd;_tmp7D5(_tmp7D6,_tmp7D7);});_tmp7D8->hd=_tmpED5;}),_tmp7D8->tl=dles;_tmp7D8;});}}{
# 3725
struct _tuple21*tag_dle=({Cyc_Toc_make_field(Cyc_Toc_tag_sp,tag_exp);});
({void*_tmpED9=(void*)({struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*_tmp7DA=_cycalloc(sizeof(*_tmp7DA));_tmp7DA->tag=29U,({struct _tuple1*_tmpED8=({Cyc_Toc_collapse_qvars(tuf->name,tud->name);});_tmp7DA->f1=_tmpED8;}),_tmp7DA->f2=0,({
struct Cyc_List_List*_tmpED7=({struct Cyc_List_List*_tmp7D9=_cycalloc(sizeof(*_tmp7D9));_tmp7D9->hd=tag_dle,({struct Cyc_List_List*_tmpED6=({Cyc_List_imp_rev(dles);});_tmp7D9->tl=_tmpED6;});_tmp7D9;});_tmp7DA->f3=_tmpED7;}),_tmp7DA->f4=0;_tmp7DA;});
# 3726
e->r=_tmpED9;});}}else{
# 3731
struct Cyc_List_List*dles=0;
for(0;es != 0;es=es->tl){
dles=({struct Cyc_List_List*_tmp7DC=_cycalloc(sizeof(*_tmp7DC));({struct _tuple21*_tmpEDA=({struct _tuple21*_tmp7DB=_cycalloc(sizeof(*_tmp7DB));_tmp7DB->f1=0,_tmp7DB->f2=(struct Cyc_Absyn_Exp*)es->hd;_tmp7DB;});_tmp7DC->hd=_tmpEDA;}),_tmp7DC->tl=dles;_tmp7DC;});}
({void*_tmpEDD=({void*(*_tmp7DD)(struct Cyc_Core_Opt*tdopt,struct Cyc_List_List*ds)=Cyc_Toc_unresolvedmem_exp_r;struct Cyc_Core_Opt*_tmp7DE=0;struct Cyc_List_List*_tmp7DF=({struct Cyc_List_List*_tmp7E1=_cycalloc(sizeof(*_tmp7E1));({struct _tuple21*_tmpEDC=({struct _tuple21*_tmp7E0=_cycalloc(sizeof(*_tmp7E0));_tmp7E0->f1=0,_tmp7E0->f2=tag_exp;_tmp7E0;});_tmp7E1->hd=_tmpEDC;}),({
struct Cyc_List_List*_tmpEDB=({Cyc_List_imp_rev(dles);});_tmp7E1->tl=_tmpEDB;});_tmp7E1;});_tmp7DD(_tmp7DE,_tmp7DF);});
# 3734
e->r=_tmpEDD;});}
# 3737
goto _LL7;}}case 32U: _LL4E: _LL4F:
# 3739
 goto _LL51;case 33U: _LL50: _LL51:
 goto _LL7;case 35U: _LL52: _tmp54B=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp545)->f1).is_calloc;_tmp54C=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp545)->f1).rgn;_tmp54D=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp545)->f1).elt_type;_tmp54E=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp545)->f1).num_elts;_tmp54F=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp545)->f1).fat_result;_tmp550=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp545)->f1).inline_call;_LL53: {int is_calloc=_tmp54B;struct Cyc_Absyn_Exp*rgnopt=_tmp54C;void**topt=_tmp54D;struct Cyc_Absyn_Exp*num_elts=_tmp54E;int is_fat=_tmp54F;int inline_call=_tmp550;
# 3743
void*t_c=({Cyc_Toc_typ_to_c(*((void**)_check_null(topt)));});
({Cyc_Toc_exp_to_c(nv,num_elts);});{
struct _tuple1*x=0;
struct Cyc_Absyn_Exp*pexp;
struct Cyc_Absyn_Exp*num_elts2=num_elts;
if(is_fat){
x=({Cyc_Toc_temp_var();});
num_elts2=({Cyc_Absyn_var_exp(x,0U);});}
# 3748
if(is_calloc){
# 3753
if(rgnopt != 0 && !Cyc_Absyn_no_regions){
({Cyc_Toc_exp_to_c(nv,rgnopt);});
pexp=({struct Cyc_Absyn_Exp*_tmp7E2[3U];_tmp7E2[0]=rgnopt,({
struct Cyc_Absyn_Exp*_tmpEDE=({Cyc_Absyn_sizeoftype_exp(t_c,0U);});_tmp7E2[1]=_tmpEDE;}),_tmp7E2[2]=num_elts2;({struct Cyc_Absyn_Exp*_tmpEDF=Cyc_Toc__region_calloc_e;Cyc_Toc_fncall_exp_dl(_tmpEDF,_tag_fat(_tmp7E2,sizeof(struct Cyc_Absyn_Exp*),3U));});});}else{
# 3758
pexp=({struct Cyc_Absyn_Exp*(*_tmp7E3)(void*elt_type,struct Cyc_Absyn_Exp*s,struct Cyc_Absyn_Exp*n)=Cyc_Toc_calloc_exp;void*_tmp7E4=*topt;struct Cyc_Absyn_Exp*_tmp7E5=({Cyc_Absyn_sizeoftype_exp(t_c,0U);});struct Cyc_Absyn_Exp*_tmp7E6=num_elts2;_tmp7E3(_tmp7E4,_tmp7E5,_tmp7E6);});}}else{
# 3760
if(rgnopt != 0 && !Cyc_Absyn_no_regions){
({Cyc_Toc_exp_to_c(nv,rgnopt);});
if(inline_call)
pexp=({Cyc_Toc_rmalloc_inline_exp(rgnopt,num_elts2);});else{
# 3765
pexp=({Cyc_Toc_rmalloc_exp(rgnopt,num_elts2);});}}else{
# 3767
pexp=({Cyc_Toc_malloc_exp(*topt,num_elts2);});}}
# 3769
if(is_fat){
struct Cyc_Absyn_Exp*elt_sz=is_calloc?({Cyc_Absyn_sizeoftype_exp(t_c,0U);}):({Cyc_Absyn_uint_exp(1U,0U);});
struct Cyc_Absyn_Exp*rexp=({struct Cyc_Absyn_Exp*_tmp7ED[3U];_tmp7ED[0]=pexp,_tmp7ED[1]=elt_sz,_tmp7ED[2]=num_elts2;({struct Cyc_Absyn_Exp*_tmpEE0=Cyc_Toc__tag_fat_e;Cyc_Toc_fncall_exp_dl(_tmpEE0,_tag_fat(_tmp7ED,sizeof(struct Cyc_Absyn_Exp*),3U));});});
struct Cyc_Absyn_Stmt*s=({struct Cyc_Absyn_Stmt*(*_tmp7E7)(struct _tuple1*,void*,struct Cyc_Absyn_Exp*init,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_declare_stmt;struct _tuple1*_tmp7E8=(struct _tuple1*)_check_null(x);void*_tmp7E9=Cyc_Absyn_uint_type;struct Cyc_Absyn_Exp*_tmp7EA=num_elts;struct Cyc_Absyn_Stmt*_tmp7EB=({Cyc_Absyn_exp_stmt(rexp,0U);});unsigned _tmp7EC=0U;_tmp7E7(_tmp7E8,_tmp7E9,_tmp7EA,_tmp7EB,_tmp7EC);});
# 3774
({void*_tmpEE1=({Cyc_Toc_stmt_exp_r(s);});e->r=_tmpEE1;});}else{
# 3776
e->r=pexp->r;}
goto _LL7;}}case 36U: _LL54: _tmp549=((struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct*)_tmp545)->f1;_tmp54A=((struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct*)_tmp545)->f2;_LL55: {struct Cyc_Absyn_Exp*e1=_tmp549;struct Cyc_Absyn_Exp*e2=_tmp54A;
# 3786
void*e1_old_typ=(void*)_check_null(e1->topt);
void*e2_old_typ=(void*)_check_null(e2->topt);
if(!({Cyc_Tcutil_is_boxed(e1_old_typ);})&& !({Cyc_Tcutil_is_pointer_type(e1_old_typ);}))
({void*_tmp7EE=0U;({int(*_tmpEE3)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp7F0)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmp7F0;});struct _fat_ptr _tmpEE2=({const char*_tmp7EF="Swap_e: is_pointer_or_boxed: not a pointer or boxed type";_tag_fat(_tmp7EF,sizeof(char),57U);});_tmpEE3(_tmpEE2,_tag_fat(_tmp7EE,sizeof(void*),0U));});});{
# 3788
unsigned loc=e->loc;
# 3795
struct _tuple1*v1=({Cyc_Toc_temp_var();});
struct Cyc_Absyn_Exp*v1e=({Cyc_Absyn_var_exp(v1,loc);});v1e->topt=e1_old_typ;{
struct _tuple1*v2=({Cyc_Toc_temp_var();});
struct Cyc_Absyn_Exp*v2e=({Cyc_Absyn_var_exp(v2,loc);});v2e->topt=e2_old_typ;{
# 3800
struct Cyc_Absyn_Exp*s1e=({struct Cyc_Absyn_Exp*(*_tmp801)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_assign_exp;struct Cyc_Absyn_Exp*_tmp802=({Cyc_Tcutil_deep_copy_exp(1,e1);});struct Cyc_Absyn_Exp*_tmp803=v2e;unsigned _tmp804=loc;_tmp801(_tmp802,_tmp803,_tmp804);});s1e->topt=e1_old_typ;{
struct Cyc_Absyn_Stmt*s1=({Cyc_Absyn_exp_stmt(s1e,loc);});
struct Cyc_Absyn_Exp*s2e=({struct Cyc_Absyn_Exp*(*_tmp7FD)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_assign_exp;struct Cyc_Absyn_Exp*_tmp7FE=({Cyc_Tcutil_deep_copy_exp(1,e2);});struct Cyc_Absyn_Exp*_tmp7FF=v1e;unsigned _tmp800=loc;_tmp7FD(_tmp7FE,_tmp7FF,_tmp800);});s2e->topt=e2_old_typ;{
struct Cyc_Absyn_Stmt*s2=({Cyc_Absyn_exp_stmt(s2e,loc);});
# 3805
struct Cyc_Absyn_Stmt*s=({struct Cyc_Absyn_Stmt*(*_tmp7F1)(struct _tuple1*,void*,struct Cyc_Absyn_Exp*init,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_declare_stmt;struct _tuple1*_tmp7F2=v1;void*_tmp7F3=e1_old_typ;struct Cyc_Absyn_Exp*_tmp7F4=e1;struct Cyc_Absyn_Stmt*_tmp7F5=({struct Cyc_Absyn_Stmt*(*_tmp7F6)(struct _tuple1*,void*,struct Cyc_Absyn_Exp*init,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_declare_stmt;struct _tuple1*_tmp7F7=v2;void*_tmp7F8=e2_old_typ;struct Cyc_Absyn_Exp*_tmp7F9=e2;struct Cyc_Absyn_Stmt*_tmp7FA=({Cyc_Absyn_seq_stmt(s1,s2,loc);});unsigned _tmp7FB=loc;_tmp7F6(_tmp7F7,_tmp7F8,_tmp7F9,_tmp7FA,_tmp7FB);});unsigned _tmp7FC=loc;_tmp7F1(_tmp7F2,_tmp7F3,_tmp7F4,_tmp7F5,_tmp7FC);});
# 3808
({Cyc_Toc_stmt_to_c(nv,s);});
({void*_tmpEE4=({Cyc_Toc_stmt_exp_r(s);});e->r=_tmpEE4;});
goto _LL7;}}}}}}case 39U: _LL56: _tmp547=((struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct*)_tmp545)->f1;_tmp548=((struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct*)_tmp545)->f2;_LL57: {struct Cyc_Absyn_Exp*e1=_tmp547;struct _fat_ptr*f=_tmp548;
# 3813
void*e1_typ=({Cyc_Tcutil_compress((void*)_check_null(e1->topt));});
({Cyc_Toc_exp_to_c(nv,e1);});
{void*_tmp805=e1_typ;struct Cyc_Absyn_Aggrdecl*_tmp806;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp805)->tag == 0U){if(((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp805)->f1)->tag == 22U){if(((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp805)->f1)->f1).KnownAggr).tag == 2){_LL118: _tmp806=*((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp805)->f1)->f1).KnownAggr).val;_LL119: {struct Cyc_Absyn_Aggrdecl*ad=_tmp806;
# 3817
struct Cyc_Absyn_Exp*f_tag=({struct Cyc_Absyn_Exp*(*_tmp807)(int,unsigned)=Cyc_Absyn_signed_int_exp;int _tmp808=({Cyc_Toc_get_member_offset(ad,f);});unsigned _tmp809=0U;_tmp807(_tmp808,_tmp809);});
struct Cyc_Absyn_Exp*e1_f=({Cyc_Toc_member_exp(e1,f,0U);});
struct Cyc_Absyn_Exp*e1_f_tag=({Cyc_Toc_member_exp(e1_f,Cyc_Toc_tag_sp,0U);});
({void*_tmpEE5=({Cyc_Absyn_eq_exp(e1_f_tag,f_tag,0U);})->r;e->r=_tmpEE5;});
goto _LL117;}}else{goto _LL11A;}}else{goto _LL11A;}}else{_LL11A: _LL11B:
({void*_tmp80A=0U;({int(*_tmpEE7)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp80C)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmp80C;});struct _fat_ptr _tmpEE6=({const char*_tmp80B="non-aggregate type in tagcheck";_tag_fat(_tmp80B,sizeof(char),31U);});_tmpEE7(_tmpEE6,_tag_fat(_tmp80A,sizeof(void*),0U));});});}_LL117:;}
# 3824
goto _LL7;}case 38U: _LL58: _tmp546=((struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct*)_tmp545)->f1;_LL59: {struct Cyc_Absyn_Stmt*s=_tmp546;
# 3827
({Cyc_Absyn_patch_stmt_exp(s);});
({Cyc_Toc_stmt_to_c(nv,s);});goto _LL7;}case 41U: _LL5A: _LL5B:
 goto _LL7;case 37U: _LL5C: _LL5D:
({void*_tmp80D=0U;({int(*_tmpEE9)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp80F)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmp80F;});struct _fat_ptr _tmpEE8=({const char*_tmp80E="UnresolvedMem";_tag_fat(_tmp80E,sizeof(char),14U);});_tmpEE9(_tmpEE8,_tag_fat(_tmp80D,sizeof(void*),0U));});});case 25U: _LL5E: _LL5F:
({void*_tmp810=0U;({int(*_tmpEEB)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp812)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmp812;});struct _fat_ptr _tmpEEA=({const char*_tmp811="compoundlit";_tag_fat(_tmp811,sizeof(char),12U);});_tmpEEB(_tmpEEA,_tag_fat(_tmp810,sizeof(void*),0U));});});default: _LL60: _LL61:
({void*_tmp813=0U;({int(*_tmpEED)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp815)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmp815;});struct _fat_ptr _tmpEEC=({const char*_tmp814="valueof(-)";_tag_fat(_tmp814,sizeof(char),11U);});_tmpEED(_tmpEEC,_tag_fat(_tmp813,sizeof(void*),0U));});});}_LL7:;}
# 3834
if(!did_inserted_checks)
({void*_tmp816=0U;({int(*_tmpEF2)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp81A)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos_loc;_tmp81A;});unsigned _tmpEF1=e->loc;struct _fat_ptr _tmpEF0=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp819=({struct Cyc_String_pa_PrintArg_struct _tmpC97;_tmpC97.tag=0U,({
struct _fat_ptr _tmpEEE=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_exp2string(e);}));_tmpC97.f1=_tmpEEE;});_tmpC97;});void*_tmp817[1U];_tmp817[0]=& _tmp819;({struct _fat_ptr _tmpEEF=({const char*_tmp818="Toc did not examine an inserted check: %s";_tag_fat(_tmp818,sizeof(char),42U);});Cyc_aprintf(_tmpEEF,_tag_fat(_tmp817,sizeof(void*),1U));});});_tmpEF2(_tmpEF1,_tmpEF0,_tag_fat(_tmp816,sizeof(void*),0U));});});}}struct _tuple44{int f1;struct _fat_ptr*f2;struct _fat_ptr*f3;struct Cyc_Absyn_Switch_clause*f4;};
# 3863 "toc.cyc"
static struct _tuple44*Cyc_Toc_gen_labels(struct _RegionHandle*r,struct Cyc_Absyn_Switch_clause*sc){
# 3865
return({struct _tuple44*_tmp81C=_region_malloc(r,sizeof(*_tmp81C));_tmp81C->f1=0,({struct _fat_ptr*_tmpEF4=({Cyc_Toc_fresh_label();});_tmp81C->f2=_tmpEF4;}),({struct _fat_ptr*_tmpEF3=({Cyc_Toc_fresh_label();});_tmp81C->f3=_tmpEF3;}),_tmp81C->f4=sc;_tmp81C;});}
# 3871
static struct Cyc_Absyn_Exp*Cyc_Toc_compile_path(struct Cyc_List_List*ps,struct Cyc_Absyn_Exp*v){
for(0;ps != 0;ps=((struct Cyc_List_List*)_check_null(ps))->tl){
struct Cyc_Tcpat_PathNode*p=(struct Cyc_Tcpat_PathNode*)ps->hd;
# 3877
if((int)(((p->orig_pat).pattern).tag == 1)){
void*t=(void*)_check_null(({union Cyc_Tcpat_PatOrWhere _tmp824=p->orig_pat;if((_tmp824.pattern).tag != 1)_throw_match();(_tmp824.pattern).val;})->topt);
void*t2=({void*(*_tmp822)(void*)=Cyc_Tcutil_compress;void*_tmp823=({Cyc_Toc_typ_to_c_array(t);});_tmp822(_tmp823);});
void*_tmp81E=t2;switch(*((int*)_tmp81E)){case 0U: switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp81E)->f1)){case 0U: _LL1: _LL2:
# 3882
 goto _LL4;case 22U: _LL3: _LL4:
 goto _LL6;default: goto _LL7;}case 7U: _LL5: _LL6:
 goto _LL0;default: _LL7: _LL8:
 v=({struct Cyc_Absyn_Exp*(*_tmp81F)(void*t,struct Cyc_Absyn_Exp*e)=Cyc_Toc_cast_it;void*_tmp820=({Cyc_Toc_typ_to_c_array(t);});struct Cyc_Absyn_Exp*_tmp821=v;_tmp81F(_tmp820,_tmp821);});goto _LL0;}_LL0:;}{
# 3877
void*_stmttmp5B=p->access;void*_tmp825=_stmttmp5B;struct _fat_ptr*_tmp827;int _tmp826;unsigned _tmp828;unsigned _tmp829;switch(*((int*)_tmp825)){case 0U: _LLA: _LLB:
# 3892
 goto _LL9;case 1U: _LLC: _LLD:
# 3897
 if(ps->tl != 0){
void*_stmttmp5C=((struct Cyc_Tcpat_PathNode*)((struct Cyc_List_List*)_check_null(ps->tl))->hd)->access;void*_tmp82A=_stmttmp5C;unsigned _tmp82D;struct Cyc_Absyn_Datatypefield*_tmp82C;struct Cyc_Absyn_Datatypedecl*_tmp82B;if(((struct Cyc_Tcpat_DatatypeField_Tcpat_Access_struct*)_tmp82A)->tag == 3U){_LL15: _tmp82B=((struct Cyc_Tcpat_DatatypeField_Tcpat_Access_struct*)_tmp82A)->f1;_tmp82C=((struct Cyc_Tcpat_DatatypeField_Tcpat_Access_struct*)_tmp82A)->f2;_tmp82D=((struct Cyc_Tcpat_DatatypeField_Tcpat_Access_struct*)_tmp82A)->f3;_LL16: {struct Cyc_Absyn_Datatypedecl*tud=_tmp82B;struct Cyc_Absyn_Datatypefield*tuf=_tmp82C;unsigned i=_tmp82D;
# 3900
ps=ps->tl;
v=({struct Cyc_Absyn_Exp*(*_tmp82E)(void*t,struct Cyc_Absyn_Exp*e)=Cyc_Toc_cast_it;void*_tmp82F=({void*(*_tmp830)(void*,struct Cyc_Absyn_Tqual)=Cyc_Absyn_cstar_type;void*_tmp831=({void*(*_tmp832)(struct _tuple1*name)=Cyc_Absyn_strctq;struct _tuple1*_tmp833=({Cyc_Toc_collapse_qvars(tuf->name,tud->name);});_tmp832(_tmp833);});struct Cyc_Absyn_Tqual _tmp834=Cyc_Toc_mt_tq;_tmp830(_tmp831,_tmp834);});struct Cyc_Absyn_Exp*_tmp835=v;_tmp82E(_tmp82F,_tmp835);});
v=({struct Cyc_Absyn_Exp*(*_tmp836)(struct Cyc_Absyn_Exp*,struct _fat_ptr*,unsigned)=Cyc_Absyn_aggrarrow_exp;struct Cyc_Absyn_Exp*_tmp837=v;struct _fat_ptr*_tmp838=({Cyc_Absyn_fieldname((int)(i + (unsigned)1));});unsigned _tmp839=0U;_tmp836(_tmp837,_tmp838,_tmp839);});
continue;}}else{_LL17: _LL18:
 goto _LL14;}_LL14:;}
# 3897
v=({Cyc_Absyn_deref_exp(v,0U);});
# 3908
goto _LL9;case 3U: _LLE: _tmp829=((struct Cyc_Tcpat_DatatypeField_Tcpat_Access_struct*)_tmp825)->f3;_LLF: {unsigned i=_tmp829;
_tmp828=i;goto _LL11;}case 2U: _LL10: _tmp828=((struct Cyc_Tcpat_TupleField_Tcpat_Access_struct*)_tmp825)->f1;_LL11: {unsigned i=_tmp828;
v=({struct Cyc_Absyn_Exp*(*_tmp83A)(struct Cyc_Absyn_Exp*e,struct _fat_ptr*f,unsigned loc)=Cyc_Toc_member_exp;struct Cyc_Absyn_Exp*_tmp83B=v;struct _fat_ptr*_tmp83C=({Cyc_Absyn_fieldname((int)(i + (unsigned)1));});unsigned _tmp83D=0U;_tmp83A(_tmp83B,_tmp83C,_tmp83D);});goto _LL9;}default: _LL12: _tmp826=((struct Cyc_Tcpat_AggrField_Tcpat_Access_struct*)_tmp825)->f1;_tmp827=((struct Cyc_Tcpat_AggrField_Tcpat_Access_struct*)_tmp825)->f2;_LL13: {int tagged=_tmp826;struct _fat_ptr*f=_tmp827;
# 3912
v=({Cyc_Toc_member_exp(v,f,0U);});
if(tagged)
v=({Cyc_Toc_member_exp(v,Cyc_Toc_val_sp,0U);});
# 3913
goto _LL9;}}_LL9:;}}
# 3918
return v;}
# 3923
static struct Cyc_Absyn_Exp*Cyc_Toc_compile_pat_test(struct Cyc_Absyn_Exp*v,void*t){
void*_tmp83F=t;struct Cyc_Absyn_Datatypefield*_tmp841;struct Cyc_Absyn_Datatypedecl*_tmp840;int _tmp843;struct _fat_ptr*_tmp842;struct Cyc_Absyn_Datatypefield*_tmp846;struct Cyc_Absyn_Datatypedecl*_tmp845;int _tmp844;unsigned _tmp847;int _tmp849;struct _fat_ptr _tmp848;struct Cyc_Absyn_Enumfield*_tmp84B;void*_tmp84A;struct Cyc_Absyn_Enumfield*_tmp84D;struct Cyc_Absyn_Enumdecl*_tmp84C;struct Cyc_Absyn_Exp*_tmp84E;switch(*((int*)_tmp83F)){case 0U: _LL1: _tmp84E=((struct Cyc_Tcpat_WhereTest_Tcpat_PatTest_struct*)_tmp83F)->f1;_LL2: {struct Cyc_Absyn_Exp*e=_tmp84E;
return e == 0?v: e;}case 1U: _LL3: _LL4:
 return({struct Cyc_Absyn_Exp*(*_tmp84F)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_eq_exp;struct Cyc_Absyn_Exp*_tmp850=v;struct Cyc_Absyn_Exp*_tmp851=({Cyc_Absyn_signed_int_exp(0,0U);});unsigned _tmp852=0U;_tmp84F(_tmp850,_tmp851,_tmp852);});case 2U: _LL5: _LL6:
 return({struct Cyc_Absyn_Exp*(*_tmp853)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_neq_exp;struct Cyc_Absyn_Exp*_tmp854=v;struct Cyc_Absyn_Exp*_tmp855=({Cyc_Absyn_signed_int_exp(0,0U);});unsigned _tmp856=0U;_tmp853(_tmp854,_tmp855,_tmp856);});case 3U: _LL7: _tmp84C=((struct Cyc_Tcpat_EqEnum_Tcpat_PatTest_struct*)_tmp83F)->f1;_tmp84D=((struct Cyc_Tcpat_EqEnum_Tcpat_PatTest_struct*)_tmp83F)->f2;_LL8: {struct Cyc_Absyn_Enumdecl*ed=_tmp84C;struct Cyc_Absyn_Enumfield*ef=_tmp84D;
return({struct Cyc_Absyn_Exp*(*_tmp857)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_eq_exp;struct Cyc_Absyn_Exp*_tmp858=v;struct Cyc_Absyn_Exp*_tmp859=({Cyc_Absyn_new_exp((void*)({struct Cyc_Absyn_Enum_e_Absyn_Raw_exp_struct*_tmp85A=_cycalloc(sizeof(*_tmp85A));_tmp85A->tag=32U,_tmp85A->f1=ed,_tmp85A->f2=ef;_tmp85A;}),0U);});unsigned _tmp85B=0U;_tmp857(_tmp858,_tmp859,_tmp85B);});}case 4U: _LL9: _tmp84A=(void*)((struct Cyc_Tcpat_EqAnonEnum_Tcpat_PatTest_struct*)_tmp83F)->f1;_tmp84B=((struct Cyc_Tcpat_EqAnonEnum_Tcpat_PatTest_struct*)_tmp83F)->f2;_LLA: {void*t=_tmp84A;struct Cyc_Absyn_Enumfield*ef=_tmp84B;
return({struct Cyc_Absyn_Exp*(*_tmp85C)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_eq_exp;struct Cyc_Absyn_Exp*_tmp85D=v;struct Cyc_Absyn_Exp*_tmp85E=({Cyc_Absyn_new_exp((void*)({struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct*_tmp85F=_cycalloc(sizeof(*_tmp85F));_tmp85F->tag=33U,_tmp85F->f1=t,_tmp85F->f2=ef;_tmp85F;}),0U);});unsigned _tmp860=0U;_tmp85C(_tmp85D,_tmp85E,_tmp860);});}case 5U: _LLB: _tmp848=((struct Cyc_Tcpat_EqFloat_Tcpat_PatTest_struct*)_tmp83F)->f1;_tmp849=((struct Cyc_Tcpat_EqFloat_Tcpat_PatTest_struct*)_tmp83F)->f2;_LLC: {struct _fat_ptr s=_tmp848;int i=_tmp849;
return({struct Cyc_Absyn_Exp*(*_tmp861)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_eq_exp;struct Cyc_Absyn_Exp*_tmp862=v;struct Cyc_Absyn_Exp*_tmp863=({Cyc_Absyn_float_exp(s,i,0U);});unsigned _tmp864=0U;_tmp861(_tmp862,_tmp863,_tmp864);});}case 6U: _LLD: _tmp847=((struct Cyc_Tcpat_EqConst_Tcpat_PatTest_struct*)_tmp83F)->f1;_LLE: {unsigned i=_tmp847;
return({struct Cyc_Absyn_Exp*(*_tmp865)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_eq_exp;struct Cyc_Absyn_Exp*_tmp866=v;struct Cyc_Absyn_Exp*_tmp867=({Cyc_Absyn_signed_int_exp((int)i,0U);});unsigned _tmp868=0U;_tmp865(_tmp866,_tmp867,_tmp868);});}case 7U: _LLF: _tmp844=((struct Cyc_Tcpat_EqDatatypeTag_Tcpat_PatTest_struct*)_tmp83F)->f1;_tmp845=((struct Cyc_Tcpat_EqDatatypeTag_Tcpat_PatTest_struct*)_tmp83F)->f2;_tmp846=((struct Cyc_Tcpat_EqDatatypeTag_Tcpat_PatTest_struct*)_tmp83F)->f3;_LL10: {int i=_tmp844;struct Cyc_Absyn_Datatypedecl*tud=_tmp845;struct Cyc_Absyn_Datatypefield*tuf=_tmp846;
# 3935
LOOP1: {
void*_stmttmp5D=v->r;void*_tmp869=_stmttmp5D;struct Cyc_Absyn_Exp*_tmp86A;struct Cyc_Absyn_Exp*_tmp86B;switch(*((int*)_tmp869)){case 14U: _LL16: _tmp86B=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp869)->f2;_LL17: {struct Cyc_Absyn_Exp*e=_tmp86B;
v=e;goto LOOP1;}case 20U: _LL18: _tmp86A=((struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_tmp869)->f1;_LL19: {struct Cyc_Absyn_Exp*e=_tmp86A;
v=e;goto _LL15;}default: _LL1A: _LL1B:
 goto _LL15;}_LL15:;}
# 3942
v=({struct Cyc_Absyn_Exp*(*_tmp86C)(void*t,struct Cyc_Absyn_Exp*e)=Cyc_Toc_cast_it;void*_tmp86D=({void*(*_tmp86E)(void*,struct Cyc_Absyn_Tqual)=Cyc_Absyn_cstar_type;void*_tmp86F=({void*(*_tmp870)(struct _tuple1*name)=Cyc_Absyn_strctq;struct _tuple1*_tmp871=({Cyc_Toc_collapse_qvars(tuf->name,tud->name);});_tmp870(_tmp871);});struct Cyc_Absyn_Tqual _tmp872=Cyc_Toc_mt_tq;_tmp86E(_tmp86F,_tmp872);});struct Cyc_Absyn_Exp*_tmp873=v;_tmp86C(_tmp86D,_tmp873);});return({struct Cyc_Absyn_Exp*(*_tmp874)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_eq_exp;struct Cyc_Absyn_Exp*_tmp875=({Cyc_Absyn_aggrarrow_exp(v,Cyc_Toc_tag_sp,0U);});struct Cyc_Absyn_Exp*_tmp876=({Cyc_Absyn_uint_exp((unsigned)i,0U);});unsigned _tmp877=0U;_tmp874(_tmp875,_tmp876,_tmp877);});}case 8U: _LL11: _tmp842=((struct Cyc_Tcpat_EqTaggedUnion_Tcpat_PatTest_struct*)_tmp83F)->f1;_tmp843=((struct Cyc_Tcpat_EqTaggedUnion_Tcpat_PatTest_struct*)_tmp83F)->f2;_LL12: {struct _fat_ptr*f=_tmp842;int i=_tmp843;
# 3946
return({struct Cyc_Absyn_Exp*(*_tmp878)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_eq_exp;struct Cyc_Absyn_Exp*_tmp879=({struct Cyc_Absyn_Exp*(*_tmp87A)(struct Cyc_Absyn_Exp*e,struct _fat_ptr*f,unsigned loc)=Cyc_Toc_member_exp;struct Cyc_Absyn_Exp*_tmp87B=({Cyc_Toc_member_exp(v,f,0U);});struct _fat_ptr*_tmp87C=Cyc_Toc_tag_sp;unsigned _tmp87D=0U;_tmp87A(_tmp87B,_tmp87C,_tmp87D);});struct Cyc_Absyn_Exp*_tmp87E=({Cyc_Absyn_signed_int_exp(i,0U);});unsigned _tmp87F=0U;_tmp878(_tmp879,_tmp87E,_tmp87F);});}default: _LL13: _tmp840=((struct Cyc_Tcpat_EqExtensibleDatatype_Tcpat_PatTest_struct*)_tmp83F)->f1;_tmp841=((struct Cyc_Tcpat_EqExtensibleDatatype_Tcpat_PatTest_struct*)_tmp83F)->f2;_LL14: {struct Cyc_Absyn_Datatypedecl*tud=_tmp840;struct Cyc_Absyn_Datatypefield*tuf=_tmp841;
# 3950
LOOP2: {
void*_stmttmp5E=v->r;void*_tmp880=_stmttmp5E;struct Cyc_Absyn_Exp*_tmp881;struct Cyc_Absyn_Exp*_tmp882;switch(*((int*)_tmp880)){case 14U: _LL1D: _tmp882=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp880)->f2;_LL1E: {struct Cyc_Absyn_Exp*e=_tmp882;
v=e;goto LOOP2;}case 20U: _LL1F: _tmp881=((struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_tmp880)->f1;_LL20: {struct Cyc_Absyn_Exp*e=_tmp881;
v=e;goto _LL1C;}default: _LL21: _LL22:
 goto _LL1C;}_LL1C:;}
# 3957
v=({struct Cyc_Absyn_Exp*(*_tmp883)(void*t,struct Cyc_Absyn_Exp*e)=Cyc_Toc_cast_it;void*_tmp884=({void*(*_tmp885)(void*,struct Cyc_Absyn_Tqual)=Cyc_Absyn_cstar_type;void*_tmp886=({void*(*_tmp887)(struct _tuple1*name)=Cyc_Absyn_strctq;struct _tuple1*_tmp888=({Cyc_Toc_collapse_qvars(tuf->name,tud->name);});_tmp887(_tmp888);});struct Cyc_Absyn_Tqual _tmp889=Cyc_Toc_mt_tq;_tmp885(_tmp886,_tmp889);});struct Cyc_Absyn_Exp*_tmp88A=v;_tmp883(_tmp884,_tmp88A);});
return({struct Cyc_Absyn_Exp*(*_tmp88B)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_eq_exp;struct Cyc_Absyn_Exp*_tmp88C=({Cyc_Absyn_aggrarrow_exp(v,Cyc_Toc_tag_sp,0U);});struct Cyc_Absyn_Exp*_tmp88D=({Cyc_Absyn_var_exp(tuf->name,0U);});unsigned _tmp88E=0U;_tmp88B(_tmp88C,_tmp88D,_tmp88E);});}}_LL0:;}struct Cyc_Toc_OtherTest_Toc_TestKind_struct{int tag;};struct Cyc_Toc_DatatypeTagTest_Toc_TestKind_struct{int tag;};struct Cyc_Toc_WhereTest_Toc_TestKind_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Toc_TaggedUnionTest_Toc_TestKind_struct{int tag;struct _fat_ptr*f1;};
# 3969
struct Cyc_Toc_OtherTest_Toc_TestKind_struct Cyc_Toc_OtherTest_val={0U};
struct Cyc_Toc_DatatypeTagTest_Toc_TestKind_struct Cyc_Toc_DatatypeTagTest_val={1U};struct _tuple45{int f1;void*f2;};
# 3977
static struct _tuple45 Cyc_Toc_admits_switch(struct Cyc_List_List*ss){
int c=0;
void*k=(void*)& Cyc_Toc_OtherTest_val;
for(0;ss != 0;(ss=ss->tl,c=c + 1)){
struct _tuple41 _stmttmp5F=*((struct _tuple41*)ss->hd);void*_tmp890;_LL1: _tmp890=_stmttmp5F.f1;_LL2: {void*ptest=_tmp890;
void*_tmp891=ptest;struct Cyc_Absyn_Exp*_tmp892;struct _fat_ptr*_tmp893;switch(*((int*)_tmp891)){case 3U: _LL4: _LL5:
# 3984
 goto _LL7;case 4U: _LL6: _LL7:
 goto _LL9;case 6U: _LL8: _LL9:
 continue;case 8U: _LLA: _tmp893=((struct Cyc_Tcpat_EqTaggedUnion_Tcpat_PatTest_struct*)_tmp891)->f1;_LLB: {struct _fat_ptr*f=_tmp893;
# 3988
if(k == (void*)& Cyc_Toc_OtherTest_val)
k=(void*)({struct Cyc_Toc_TaggedUnionTest_Toc_TestKind_struct*_tmp894=_cycalloc(sizeof(*_tmp894));_tmp894->tag=3U,_tmp894->f1=f;_tmp894;});
# 3988
continue;}case 7U: _LLC: _LLD:
# 3991
 k=(void*)& Cyc_Toc_DatatypeTagTest_val;continue;case 0U: _LLE: _tmp892=((struct Cyc_Tcpat_WhereTest_Tcpat_PatTest_struct*)_tmp891)->f1;if(_tmp892 != 0){_LLF: {struct Cyc_Absyn_Exp*e=_tmp892;
# 3993
k=(void*)({struct Cyc_Toc_WhereTest_Toc_TestKind_struct*_tmp895=_cycalloc(sizeof(*_tmp895));_tmp895->tag=2U,_tmp895->f1=e;_tmp895;});return({struct _tuple45 _tmpC98;_tmpC98.f1=0,_tmpC98.f2=k;_tmpC98;});}}else{_LL10: _LL11:
 goto _LL13;}case 1U: _LL12: _LL13:
 goto _LL15;case 2U: _LL14: _LL15:
 goto _LL17;case 5U: _LL16: _LL17:
 goto _LL19;default: _LL18: _LL19:
 return({struct _tuple45 _tmpC99;_tmpC99.f1=0,_tmpC99.f2=k;_tmpC99;});}_LL3:;}}
# 4001
return({struct _tuple45 _tmpC9A;_tmpC9A.f1=c,_tmpC9A.f2=k;_tmpC9A;});}
# 4006
static struct Cyc_Absyn_Pat*Cyc_Toc_compile_pat_test_as_case(void*p){
struct Cyc_Absyn_Exp*e;
{void*_tmp897=p;int _tmp898;int _tmp899;unsigned _tmp89A;struct Cyc_Absyn_Enumfield*_tmp89C;void*_tmp89B;struct Cyc_Absyn_Enumfield*_tmp89E;struct Cyc_Absyn_Enumdecl*_tmp89D;switch(*((int*)_tmp897)){case 3U: _LL1: _tmp89D=((struct Cyc_Tcpat_EqEnum_Tcpat_PatTest_struct*)_tmp897)->f1;_tmp89E=((struct Cyc_Tcpat_EqEnum_Tcpat_PatTest_struct*)_tmp897)->f2;_LL2: {struct Cyc_Absyn_Enumdecl*ed=_tmp89D;struct Cyc_Absyn_Enumfield*ef=_tmp89E;
e=({Cyc_Absyn_new_exp((void*)({struct Cyc_Absyn_Enum_e_Absyn_Raw_exp_struct*_tmp89F=_cycalloc(sizeof(*_tmp89F));_tmp89F->tag=32U,_tmp89F->f1=ed,_tmp89F->f2=ef;_tmp89F;}),0U);});goto _LL0;}case 4U: _LL3: _tmp89B=(void*)((struct Cyc_Tcpat_EqAnonEnum_Tcpat_PatTest_struct*)_tmp897)->f1;_tmp89C=((struct Cyc_Tcpat_EqAnonEnum_Tcpat_PatTest_struct*)_tmp897)->f2;_LL4: {void*t=_tmp89B;struct Cyc_Absyn_Enumfield*ef=_tmp89C;
e=({Cyc_Absyn_new_exp((void*)({struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct*_tmp8A0=_cycalloc(sizeof(*_tmp8A0));_tmp8A0->tag=33U,_tmp8A0->f1=t,_tmp8A0->f2=ef;_tmp8A0;}),0U);});goto _LL0;}case 6U: _LL5: _tmp89A=((struct Cyc_Tcpat_EqConst_Tcpat_PatTest_struct*)_tmp897)->f1;_LL6: {unsigned i=_tmp89A;
_tmp899=(int)i;goto _LL8;}case 7U: _LL7: _tmp899=((struct Cyc_Tcpat_EqDatatypeTag_Tcpat_PatTest_struct*)_tmp897)->f1;_LL8: {int i=_tmp899;
_tmp898=i;goto _LLA;}case 8U: _LL9: _tmp898=((struct Cyc_Tcpat_EqTaggedUnion_Tcpat_PatTest_struct*)_tmp897)->f2;_LLA: {int i=_tmp898;
e=({Cyc_Absyn_uint_exp((unsigned)i,0U);});goto _LL0;}default: _LLB: _LLC:
({void*_tmp8A1=0U;({int(*_tmpEF6)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp8A3)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmp8A3;});struct _fat_ptr _tmpEF5=({const char*_tmp8A2="compile_pat_test_as_case!";_tag_fat(_tmp8A2,sizeof(char),26U);});_tmpEF6(_tmpEF5,_tag_fat(_tmp8A1,sizeof(void*),0U));});});}_LL0:;}
# 4016
return({Cyc_Absyn_new_pat((void*)({struct Cyc_Absyn_Exp_p_Absyn_Raw_pat_struct*_tmp8A4=_cycalloc(sizeof(*_tmp8A4));_tmp8A4->tag=17U,_tmp8A4->f1=e;_tmp8A4;}),0U);});}
# 4020
static struct Cyc_Absyn_Stmt*Cyc_Toc_seq_stmt_opt(struct Cyc_Absyn_Stmt*s1,struct Cyc_Absyn_Stmt*s2){
if(s1 == 0)return s2;if(s2 == 0)
return s1;
# 4021
return({Cyc_Absyn_seq_stmt(s1,s2,0U);});}struct _tuple46{struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Vardecl*f2;};struct _tuple47{struct Cyc_List_List*f1;struct Cyc_Absyn_Pat*f2;};
# 4031
static struct Cyc_Absyn_Stmt*Cyc_Toc_extract_pattern_vars(struct _RegionHandle*rgn,struct Cyc_Toc_Env**nv,struct Cyc_List_List**decls,struct Cyc_Absyn_Exp*path,struct Cyc_Absyn_Pat*p){
# 4035
void*t=(void*)_check_null(p->topt);
void*_stmttmp60=p->r;void*_tmp8A7=_stmttmp60;struct Cyc_List_List*_tmp8A9;union Cyc_Absyn_AggrInfo _tmp8A8;struct Cyc_List_List*_tmp8AA;struct Cyc_List_List*_tmp8AB;struct Cyc_Absyn_Pat*_tmp8AC;struct Cyc_List_List*_tmp8AF;struct Cyc_Absyn_Datatypefield*_tmp8AE;struct Cyc_Absyn_Datatypedecl*_tmp8AD;struct Cyc_Absyn_Pat*_tmp8B1;struct Cyc_Absyn_Vardecl*_tmp8B0;struct Cyc_Absyn_Vardecl*_tmp8B2;struct Cyc_Absyn_Pat*_tmp8B4;struct Cyc_Absyn_Vardecl*_tmp8B3;struct Cyc_Absyn_Vardecl*_tmp8B5;switch(*((int*)_tmp8A7)){case 2U: _LL1: _tmp8B5=((struct Cyc_Absyn_AliasVar_p_Absyn_Raw_pat_struct*)_tmp8A7)->f2;_LL2: {struct Cyc_Absyn_Vardecl*vd=_tmp8B5;
# 4038
struct Cyc_Absyn_Pat*p2=({Cyc_Absyn_new_pat((void*)& Cyc_Absyn_Wild_p_val,0U);});
p2->topt=p->topt;
_tmp8B3=vd;_tmp8B4=p2;goto _LL4;}case 1U: _LL3: _tmp8B3=((struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct*)_tmp8A7)->f1;_tmp8B4=((struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct*)_tmp8A7)->f2;_LL4: {struct Cyc_Absyn_Vardecl*vd=_tmp8B3;struct Cyc_Absyn_Pat*p2=_tmp8B4;
# 4043
struct Cyc_Absyn_Vardecl*new_vd=({struct Cyc_Absyn_Vardecl*(*_tmp8C0)(unsigned varloc,struct _tuple1*,void*,struct Cyc_Absyn_Exp*init)=Cyc_Absyn_new_vardecl;unsigned _tmp8C1=0U;struct _tuple1*_tmp8C2=({Cyc_Toc_temp_var();});void*_tmp8C3=({Cyc_Toc_typ_to_c_array(vd->type);});struct Cyc_Absyn_Exp*_tmp8C4=0;_tmp8C0(_tmp8C1,_tmp8C2,_tmp8C3,_tmp8C4);});
({struct Cyc_List_List*_tmpEF8=({struct Cyc_List_List*_tmp8B7=_region_malloc(rgn,sizeof(*_tmp8B7));({struct _tuple46*_tmpEF7=({struct _tuple46*_tmp8B6=_region_malloc(rgn,sizeof(*_tmp8B6));_tmp8B6->f1=vd,_tmp8B6->f2=new_vd;_tmp8B6;});_tmp8B7->hd=_tmpEF7;}),_tmp8B7->tl=*decls;_tmp8B7;});*decls=_tmpEF8;});{
struct Cyc_Absyn_Stmt*s=({struct Cyc_Absyn_Stmt*(*_tmp8BB)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_assign_stmt;struct Cyc_Absyn_Exp*_tmp8BC=({Cyc_Absyn_varb_exp((void*)({struct Cyc_Absyn_Local_b_Absyn_Binding_struct*_tmp8BD=_cycalloc(sizeof(*_tmp8BD));_tmp8BD->tag=4U,_tmp8BD->f1=new_vd;_tmp8BD;}),0U);});struct Cyc_Absyn_Exp*_tmp8BE=({Cyc_Absyn_copy_exp(path);});unsigned _tmp8BF=0U;_tmp8BB(_tmp8BC,_tmp8BE,_tmp8BF);});
return({struct Cyc_Absyn_Stmt*(*_tmp8B8)(struct Cyc_Absyn_Stmt*s1,struct Cyc_Absyn_Stmt*s2)=Cyc_Toc_seq_stmt_opt;struct Cyc_Absyn_Stmt*_tmp8B9=s;struct Cyc_Absyn_Stmt*_tmp8BA=({Cyc_Toc_extract_pattern_vars(rgn,nv,decls,path,p2);});_tmp8B8(_tmp8B9,_tmp8BA);});}}case 4U: _LL5: _tmp8B2=((struct Cyc_Absyn_TagInt_p_Absyn_Raw_pat_struct*)_tmp8A7)->f2;_LL6: {struct Cyc_Absyn_Vardecl*vd=_tmp8B2;
# 4049
struct Cyc_Absyn_Vardecl*new_vd=({struct Cyc_Absyn_Vardecl*(*_tmp8CC)(unsigned varloc,struct _tuple1*,void*,struct Cyc_Absyn_Exp*init)=Cyc_Absyn_new_vardecl;unsigned _tmp8CD=0U;struct _tuple1*_tmp8CE=({Cyc_Toc_temp_var();});void*_tmp8CF=({Cyc_Toc_typ_to_c_array(vd->type);});struct Cyc_Absyn_Exp*_tmp8D0=0;_tmp8CC(_tmp8CD,_tmp8CE,_tmp8CF,_tmp8D0);});
({struct Cyc_List_List*_tmpEFA=({struct Cyc_List_List*_tmp8C6=_region_malloc(rgn,sizeof(*_tmp8C6));({struct _tuple46*_tmpEF9=({struct _tuple46*_tmp8C5=_region_malloc(rgn,sizeof(*_tmp8C5));_tmp8C5->f1=vd,_tmp8C5->f2=new_vd;_tmp8C5;});_tmp8C6->hd=_tmpEF9;}),_tmp8C6->tl=*decls;_tmp8C6;});*decls=_tmpEFA;});
return({struct Cyc_Absyn_Stmt*(*_tmp8C7)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_assign_stmt;struct Cyc_Absyn_Exp*_tmp8C8=({Cyc_Absyn_varb_exp((void*)({struct Cyc_Absyn_Local_b_Absyn_Binding_struct*_tmp8C9=_cycalloc(sizeof(*_tmp8C9));_tmp8C9->tag=4U,_tmp8C9->f1=new_vd;_tmp8C9;}),0U);});struct Cyc_Absyn_Exp*_tmp8CA=({Cyc_Absyn_copy_exp(path);});unsigned _tmp8CB=0U;_tmp8C7(_tmp8C8,_tmp8CA,_tmp8CB);});}case 0U: _LL7: _LL8:
 return 0;case 3U: _LL9: _tmp8B0=((struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct*)_tmp8A7)->f1;_tmp8B1=((struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct*)_tmp8A7)->f2;_LLA: {struct Cyc_Absyn_Vardecl*vd=_tmp8B0;struct Cyc_Absyn_Pat*p2=_tmp8B1;
# 4055
({void*_tmpEFB=({Cyc_Absyn_cstar_type(t,Cyc_Toc_mt_tq);});vd->type=_tmpEFB;});{
struct Cyc_Absyn_Vardecl*new_vd=({struct Cyc_Absyn_Vardecl*(*_tmp8E3)(unsigned varloc,struct _tuple1*,void*,struct Cyc_Absyn_Exp*init)=Cyc_Absyn_new_vardecl;unsigned _tmp8E4=0U;struct _tuple1*_tmp8E5=({Cyc_Toc_temp_var();});void*_tmp8E6=({Cyc_Toc_typ_to_c_array(vd->type);});struct Cyc_Absyn_Exp*_tmp8E7=0;_tmp8E3(_tmp8E4,_tmp8E5,_tmp8E6,_tmp8E7);});
({struct Cyc_List_List*_tmpEFD=({struct Cyc_List_List*_tmp8D2=_region_malloc(rgn,sizeof(*_tmp8D2));({struct _tuple46*_tmpEFC=({struct _tuple46*_tmp8D1=_region_malloc(rgn,sizeof(*_tmp8D1));_tmp8D1->f1=vd,_tmp8D1->f2=new_vd;_tmp8D1;});_tmp8D2->hd=_tmpEFC;}),_tmp8D2->tl=*decls;_tmp8D2;});*decls=_tmpEFD;});{
# 4059
struct Cyc_Absyn_Stmt*s=({struct Cyc_Absyn_Stmt*(*_tmp8D6)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_assign_stmt;struct Cyc_Absyn_Exp*_tmp8D7=({Cyc_Absyn_varb_exp((void*)({struct Cyc_Absyn_Local_b_Absyn_Binding_struct*_tmp8D8=_cycalloc(sizeof(*_tmp8D8));_tmp8D8->tag=4U,_tmp8D8->f1=new_vd;_tmp8D8;}),0U);});struct Cyc_Absyn_Exp*_tmp8D9=({struct Cyc_Absyn_Exp*(*_tmp8DA)(void*t,struct Cyc_Absyn_Exp*e)=Cyc_Toc_cast_it;void*_tmp8DB=({void*(*_tmp8DC)(void*,struct Cyc_Absyn_Tqual)=Cyc_Absyn_cstar_type;void*_tmp8DD=({Cyc_Toc_typ_to_c_array(t);});struct Cyc_Absyn_Tqual _tmp8DE=Cyc_Toc_mt_tq;_tmp8DC(_tmp8DD,_tmp8DE);});struct Cyc_Absyn_Exp*_tmp8DF=({struct Cyc_Absyn_Exp*(*_tmp8E0)(struct Cyc_Absyn_Exp*e)=Cyc_Toc_push_address_exp;struct Cyc_Absyn_Exp*_tmp8E1=({Cyc_Absyn_copy_exp(path);});_tmp8E0(_tmp8E1);});_tmp8DA(_tmp8DB,_tmp8DF);});unsigned _tmp8E2=0U;_tmp8D6(_tmp8D7,_tmp8D9,_tmp8E2);});
# 4062
return({struct Cyc_Absyn_Stmt*(*_tmp8D3)(struct Cyc_Absyn_Stmt*s1,struct Cyc_Absyn_Stmt*s2)=Cyc_Toc_seq_stmt_opt;struct Cyc_Absyn_Stmt*_tmp8D4=s;struct Cyc_Absyn_Stmt*_tmp8D5=({Cyc_Toc_extract_pattern_vars(rgn,nv,decls,path,p2);});_tmp8D3(_tmp8D4,_tmp8D5);});}}}case 9U: _LLB: _LLC:
# 4064
 goto _LLE;case 10U: _LLD: _LLE:
 goto _LL10;case 11U: _LLF: _LL10:
 goto _LL12;case 12U: _LL11: _LL12:
 goto _LL14;case 13U: _LL13: _LL14:
 goto _LL16;case 14U: _LL15: _LL16:
 return 0;case 6U: if(((struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct*)((struct Cyc_Absyn_Pat*)((struct Cyc_Absyn_Pointer_p_Absyn_Raw_pat_struct*)_tmp8A7)->f1)->r)->tag == 8U){_LL17: _tmp8AD=((struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct*)(((struct Cyc_Absyn_Pointer_p_Absyn_Raw_pat_struct*)_tmp8A7)->f1)->r)->f1;_tmp8AE=((struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct*)(((struct Cyc_Absyn_Pointer_p_Absyn_Raw_pat_struct*)_tmp8A7)->f1)->r)->f2;_tmp8AF=((struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct*)(((struct Cyc_Absyn_Pointer_p_Absyn_Raw_pat_struct*)_tmp8A7)->f1)->r)->f3;_LL18: {struct Cyc_Absyn_Datatypedecl*tud=_tmp8AD;struct Cyc_Absyn_Datatypefield*tuf=_tmp8AE;struct Cyc_List_List*ps=_tmp8AF;
# 4073
if(ps == 0)return 0;{struct _tuple1*tufstrct=({Cyc_Toc_collapse_qvars(tuf->name,tud->name);});
# 4075
void*field_ptr_typ=({void*(*_tmp8F1)(void*,struct Cyc_Absyn_Tqual)=Cyc_Absyn_cstar_type;void*_tmp8F2=({Cyc_Absyn_strctq(tufstrct);});struct Cyc_Absyn_Tqual _tmp8F3=Cyc_Toc_mt_tq;_tmp8F1(_tmp8F2,_tmp8F3);});
path=({Cyc_Toc_cast_it(field_ptr_typ,path);});{
int cnt=1;
struct Cyc_List_List*tuf_tqts=tuf->typs;
struct Cyc_Absyn_Stmt*s=0;
for(0;ps != 0;(ps=ps->tl,tuf_tqts=((struct Cyc_List_List*)_check_null(tuf_tqts))->tl,++ cnt)){
struct Cyc_Absyn_Pat*p2=(struct Cyc_Absyn_Pat*)ps->hd;
if(p2->r == (void*)& Cyc_Absyn_Wild_p_val)continue;{void*tuf_typ=(*((struct _tuple14*)((struct Cyc_List_List*)_check_null(tuf_tqts))->hd)).f2;
# 4084
void*t2=(void*)_check_null(p2->topt);
void*t2c=({Cyc_Toc_typ_to_c_array(t2);});
struct Cyc_Absyn_Exp*arrow_exp=({struct Cyc_Absyn_Exp*(*_tmp8ED)(struct Cyc_Absyn_Exp*,struct _fat_ptr*,unsigned)=Cyc_Absyn_aggrarrow_exp;struct Cyc_Absyn_Exp*_tmp8EE=path;struct _fat_ptr*_tmp8EF=({Cyc_Absyn_fieldname(cnt);});unsigned _tmp8F0=0U;_tmp8ED(_tmp8EE,_tmp8EF,_tmp8F0);});
if(({int(*_tmp8E8)(void*t)=Cyc_Toc_is_void_star_or_boxed_tvar;void*_tmp8E9=({Cyc_Toc_typ_to_c(tuf_typ);});_tmp8E8(_tmp8E9);}))
arrow_exp=({Cyc_Toc_cast_it(t2c,arrow_exp);});
# 4087
s=({struct Cyc_Absyn_Stmt*(*_tmp8EA)(struct Cyc_Absyn_Stmt*s1,struct Cyc_Absyn_Stmt*s2)=Cyc_Toc_seq_stmt_opt;struct Cyc_Absyn_Stmt*_tmp8EB=s;struct Cyc_Absyn_Stmt*_tmp8EC=({Cyc_Toc_extract_pattern_vars(rgn,nv,decls,arrow_exp,p2);});_tmp8EA(_tmp8EB,_tmp8EC);});}}
# 4091
return s;}}}}else{_LL21: _tmp8AC=((struct Cyc_Absyn_Pointer_p_Absyn_Raw_pat_struct*)_tmp8A7)->f1;_LL22: {struct Cyc_Absyn_Pat*p2=_tmp8AC;
# 4136
return({struct Cyc_Absyn_Stmt*(*_tmp90C)(struct _RegionHandle*rgn,struct Cyc_Toc_Env**nv,struct Cyc_List_List**decls,struct Cyc_Absyn_Exp*path,struct Cyc_Absyn_Pat*p)=Cyc_Toc_extract_pattern_vars;struct _RegionHandle*_tmp90D=rgn;struct Cyc_Toc_Env**_tmp90E=nv;struct Cyc_List_List**_tmp90F=decls;struct Cyc_Absyn_Exp*_tmp910=({Cyc_Absyn_deref_exp(path,0U);});struct Cyc_Absyn_Pat*_tmp911=p2;_tmp90C(_tmp90D,_tmp90E,_tmp90F,_tmp910,_tmp911);});}}case 8U: _LL19: _tmp8AB=((struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct*)_tmp8A7)->f3;_LL1A: {struct Cyc_List_List*ps=_tmp8AB;
# 4093
_tmp8AA=ps;goto _LL1C;}case 5U: _LL1B: _tmp8AA=((struct Cyc_Absyn_Tuple_p_Absyn_Raw_pat_struct*)_tmp8A7)->f1;_LL1C: {struct Cyc_List_List*ps=_tmp8AA;
# 4095
struct Cyc_Absyn_Stmt*s=0;
int cnt=1;
for(0;ps != 0;(ps=ps->tl,++ cnt)){
struct Cyc_Absyn_Pat*p2=(struct Cyc_Absyn_Pat*)ps->hd;
if(p2->r == (void*)& Cyc_Absyn_Wild_p_val)
continue;{
# 4099
void*t2=(void*)_check_null(p2->topt);
# 4102
struct _fat_ptr*f=({Cyc_Absyn_fieldname(cnt);});
s=({struct Cyc_Absyn_Stmt*(*_tmp8F4)(struct Cyc_Absyn_Stmt*s1,struct Cyc_Absyn_Stmt*s2)=Cyc_Toc_seq_stmt_opt;struct Cyc_Absyn_Stmt*_tmp8F5=s;struct Cyc_Absyn_Stmt*_tmp8F6=({struct Cyc_Absyn_Stmt*(*_tmp8F7)(struct _RegionHandle*rgn,struct Cyc_Toc_Env**nv,struct Cyc_List_List**decls,struct Cyc_Absyn_Exp*path,struct Cyc_Absyn_Pat*p)=Cyc_Toc_extract_pattern_vars;struct _RegionHandle*_tmp8F8=rgn;struct Cyc_Toc_Env**_tmp8F9=nv;struct Cyc_List_List**_tmp8FA=decls;struct Cyc_Absyn_Exp*_tmp8FB=({Cyc_Toc_member_exp(path,f,0U);});struct Cyc_Absyn_Pat*_tmp8FC=p2;_tmp8F7(_tmp8F8,_tmp8F9,_tmp8FA,_tmp8FB,_tmp8FC);});_tmp8F4(_tmp8F5,_tmp8F6);});}}
# 4105
return s;}case 7U: if(((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp8A7)->f1 == 0){_LL1D: _LL1E:
({void*_tmp8FD=0U;({int(*_tmpEFF)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp8FF)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmp8FF;});struct _fat_ptr _tmpEFE=({const char*_tmp8FE="unresolved aggregate pattern!";_tag_fat(_tmp8FE,sizeof(char),30U);});_tmpEFF(_tmpEFE,_tag_fat(_tmp8FD,sizeof(void*),0U));});});}else{_LL1F: _tmp8A8=*((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp8A7)->f1;_tmp8A9=((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp8A7)->f3;_LL20: {union Cyc_Absyn_AggrInfo info=_tmp8A8;struct Cyc_List_List*dlps=_tmp8A9;
# 4109
struct Cyc_Absyn_Aggrdecl*ad=({Cyc_Absyn_get_known_aggrdecl(info);});
struct Cyc_Absyn_Stmt*s=0;
for(0;dlps != 0;dlps=dlps->tl){
struct _tuple47*tup=(struct _tuple47*)dlps->hd;
struct Cyc_Absyn_Pat*p2=(*tup).f2;
if(p2->r == (void*)& Cyc_Absyn_Wild_p_val)
continue;{
# 4114
struct _fat_ptr*f=({Cyc_Absyn_designatorlist_to_fieldname((*tup).f1);});
# 4117
void*t2=(void*)_check_null(p2->topt);
void*t2c=({Cyc_Toc_typ_to_c_array(t2);});
struct Cyc_Absyn_Exp*memexp=({Cyc_Toc_member_exp(path,f,0U);});
# 4121
if(((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->tagged)memexp=({Cyc_Toc_member_exp(memexp,Cyc_Toc_val_sp,0U);});{void*ftype=((struct Cyc_Absyn_Aggrfield*)_check_null(({Cyc_Absyn_lookup_field(((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->fields,f);})))->type;
# 4123
if(({Cyc_Toc_is_void_star_or_boxed_tvar(ftype);}))
memexp=({Cyc_Toc_cast_it(t2c,memexp);});else{
if(!({Cyc_Tcutil_is_array_type(ftype);})&&({int(*_tmp900)(struct Cyc_Absyn_Kind*,struct Cyc_Absyn_Kind*)=Cyc_Tcutil_kind_leq;struct Cyc_Absyn_Kind*_tmp901=& Cyc_Tcutil_ak;struct Cyc_Absyn_Kind*_tmp902=({Cyc_Tcutil_type_kind(ftype);});_tmp900(_tmp901,_tmp902);}))
# 4128
memexp=({struct Cyc_Absyn_Exp*(*_tmp903)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_deref_exp;struct Cyc_Absyn_Exp*_tmp904=({struct Cyc_Absyn_Exp*(*_tmp905)(void*t,struct Cyc_Absyn_Exp*e)=Cyc_Toc_cast_it;void*_tmp906=({Cyc_Absyn_cstar_type(t2c,Cyc_Toc_mt_tq);});struct Cyc_Absyn_Exp*_tmp907=({Cyc_Absyn_address_exp(memexp,0U);});_tmp905(_tmp906,_tmp907);});unsigned _tmp908=0U;_tmp903(_tmp904,_tmp908);});}
# 4123
s=({struct Cyc_Absyn_Stmt*(*_tmp909)(struct Cyc_Absyn_Stmt*s1,struct Cyc_Absyn_Stmt*s2)=Cyc_Toc_seq_stmt_opt;struct Cyc_Absyn_Stmt*_tmp90A=s;struct Cyc_Absyn_Stmt*_tmp90B=({Cyc_Toc_extract_pattern_vars(rgn,nv,decls,memexp,p2);});_tmp909(_tmp90A,_tmp90B);});}}}
# 4133
return s;}}case 15U: _LL23: _LL24:
# 4138
({void*_tmp912=0U;({int(*_tmpF01)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp914)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmp914;});struct _fat_ptr _tmpF00=({const char*_tmp913="unknownid pat";_tag_fat(_tmp913,sizeof(char),14U);});_tmpF01(_tmpF00,_tag_fat(_tmp912,sizeof(void*),0U));});});case 16U: _LL25: _LL26:
({void*_tmp915=0U;({int(*_tmpF03)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp917)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmp917;});struct _fat_ptr _tmpF02=({const char*_tmp916="unknowncall pat";_tag_fat(_tmp916,sizeof(char),16U);});_tmpF03(_tmpF02,_tag_fat(_tmp915,sizeof(void*),0U));});});default: _LL27: _LL28:
({void*_tmp918=0U;({int(*_tmpF05)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp91A)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmp91A;});struct _fat_ptr _tmpF04=({const char*_tmp919="exp pat";_tag_fat(_tmp919,sizeof(char),8U);});_tmpF05(_tmpF04,_tag_fat(_tmp918,sizeof(void*),0U));});});}_LL0:;}
# 4144
static struct Cyc_Absyn_Vardecl*Cyc_Toc_find_new_var(struct Cyc_List_List*vs,struct Cyc_Absyn_Vardecl*v){
for(0;vs != 0;vs=vs->tl){
struct _tuple46 _stmttmp61=*((struct _tuple46*)vs->hd);struct Cyc_Absyn_Vardecl*_tmp91D;struct Cyc_Absyn_Vardecl*_tmp91C;_LL1: _tmp91C=_stmttmp61.f1;_tmp91D=_stmttmp61.f2;_LL2: {struct Cyc_Absyn_Vardecl*oldv=_tmp91C;struct Cyc_Absyn_Vardecl*newv=_tmp91D;
if(oldv == newv)return newv;}}
# 4149
({void*_tmp91E=0U;({int(*_tmpF07)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp920)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmp920;});struct _fat_ptr _tmpF06=({const char*_tmp91F="find_new_var";_tag_fat(_tmp91F,sizeof(char),13U);});_tmpF07(_tmpF06,_tag_fat(_tmp91E,sizeof(void*),0U));});});}
# 4155
static void Cyc_Toc_subst_pattern_vars(struct Cyc_List_List*env,struct Cyc_Absyn_Exp*e){
void*_stmttmp62=e->r;void*_tmp922=_stmttmp62;struct Cyc_List_List*_tmp923;struct Cyc_List_List*_tmp924;struct Cyc_List_List*_tmp925;struct Cyc_List_List*_tmp926;struct Cyc_List_List*_tmp927;struct Cyc_List_List*_tmp928;struct Cyc_List_List*_tmp929;struct Cyc_List_List*_tmp92A;struct Cyc_Absyn_Exp*_tmp92C;struct Cyc_Absyn_Exp*_tmp92B;struct Cyc_Absyn_Exp*_tmp92E;struct Cyc_Absyn_Exp*_tmp92D;struct Cyc_Absyn_Exp*_tmp92F;struct Cyc_Absyn_Exp*_tmp930;struct Cyc_Absyn_Exp*_tmp931;struct Cyc_Absyn_Exp*_tmp932;struct Cyc_Absyn_Exp*_tmp933;struct Cyc_Absyn_Exp*_tmp934;struct Cyc_Absyn_Exp*_tmp935;struct Cyc_Absyn_Exp*_tmp936;struct Cyc_Absyn_Exp*_tmp937;struct Cyc_Absyn_Exp*_tmp938;struct Cyc_Absyn_Exp*_tmp939;struct Cyc_Absyn_Exp*_tmp93A;struct Cyc_Absyn_Exp*_tmp93C;struct Cyc_Absyn_Exp*_tmp93B;struct Cyc_Absyn_Exp*_tmp93E;struct Cyc_Absyn_Exp*_tmp93D;struct Cyc_Absyn_Exp*_tmp940;struct Cyc_Absyn_Exp*_tmp93F;struct Cyc_Absyn_Exp*_tmp942;struct Cyc_Absyn_Exp*_tmp941;struct Cyc_Absyn_Exp*_tmp944;struct Cyc_Absyn_Exp*_tmp943;struct Cyc_Absyn_Exp*_tmp947;struct Cyc_Absyn_Exp*_tmp946;struct Cyc_Absyn_Exp*_tmp945;struct Cyc_Absyn_Vardecl*_tmp948;struct Cyc_Absyn_Vardecl*_tmp949;switch(*((int*)_tmp922)){case 1U: switch(*((int*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp922)->f1)){case 5U: _LL1: _tmp949=((struct Cyc_Absyn_Pat_b_Absyn_Binding_struct*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp922)->f1)->f1;_LL2: {struct Cyc_Absyn_Vardecl*vd=_tmp949;
_tmp948=vd;goto _LL4;}case 4U: _LL3: _tmp948=((struct Cyc_Absyn_Local_b_Absyn_Binding_struct*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp922)->f1)->f1;_LL4: {struct Cyc_Absyn_Vardecl*vd=_tmp948;
# 4159
{struct _handler_cons _tmp94A;_push_handler(& _tmp94A);{int _tmp94C=0;if(setjmp(_tmp94A.handler))_tmp94C=1;if(!_tmp94C){
({void*_tmpF0C=(void*)({struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*_tmp94F=_cycalloc(sizeof(*_tmp94F));_tmp94F->tag=1U,({void*_tmpF0B=(void*)({struct Cyc_Absyn_Local_b_Absyn_Binding_struct*_tmp94E=_cycalloc(sizeof(*_tmp94E));_tmp94E->tag=4U,({struct Cyc_Absyn_Vardecl*_tmpF0A=({({struct Cyc_Absyn_Vardecl*(*_tmpF09)(struct Cyc_List_List*l,struct Cyc_Absyn_Vardecl*k)=({struct Cyc_Absyn_Vardecl*(*_tmp94D)(struct Cyc_List_List*l,struct Cyc_Absyn_Vardecl*k)=(struct Cyc_Absyn_Vardecl*(*)(struct Cyc_List_List*l,struct Cyc_Absyn_Vardecl*k))Cyc_List_assoc;_tmp94D;});struct Cyc_List_List*_tmpF08=env;_tmpF09(_tmpF08,vd);});});_tmp94E->f1=_tmpF0A;});_tmp94E;});_tmp94F->f1=_tmpF0B;});_tmp94F;});e->r=_tmpF0C;});;_pop_handler();}else{void*_tmp94B=(void*)Cyc_Core_get_exn_thrown();void*_tmp950=_tmp94B;void*_tmp951;if(((struct Cyc_Core_Not_found_exn_struct*)_tmp950)->tag == Cyc_Core_Not_found){_LL40: _LL41:
# 4162
 goto _LL3F;}else{_LL42: _tmp951=_tmp950;_LL43: {void*exn=_tmp951;(int)_rethrow(exn);}}_LL3F:;}}}
# 4164
goto _LL0;}default: goto _LL3D;}case 6U: _LL5: _tmp945=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp922)->f1;_tmp946=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp922)->f2;_tmp947=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp922)->f3;_LL6: {struct Cyc_Absyn_Exp*e1=_tmp945;struct Cyc_Absyn_Exp*e2=_tmp946;struct Cyc_Absyn_Exp*e3=_tmp947;
# 4166
({Cyc_Toc_subst_pattern_vars(env,e1);});_tmp943=e2;_tmp944=e3;goto _LL8;}case 27U: _LL7: _tmp943=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmp922)->f2;_tmp944=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmp922)->f3;_LL8: {struct Cyc_Absyn_Exp*e1=_tmp943;struct Cyc_Absyn_Exp*e2=_tmp944;
_tmp941=e1;_tmp942=e2;goto _LLA;}case 7U: _LL9: _tmp941=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp922)->f1;_tmp942=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp922)->f2;_LLA: {struct Cyc_Absyn_Exp*e1=_tmp941;struct Cyc_Absyn_Exp*e2=_tmp942;
_tmp93F=e1;_tmp940=e2;goto _LLC;}case 8U: _LLB: _tmp93F=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp922)->f1;_tmp940=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp922)->f2;_LLC: {struct Cyc_Absyn_Exp*e1=_tmp93F;struct Cyc_Absyn_Exp*e2=_tmp940;
_tmp93D=e1;_tmp93E=e2;goto _LLE;}case 23U: _LLD: _tmp93D=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp922)->f1;_tmp93E=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp922)->f2;_LLE: {struct Cyc_Absyn_Exp*e1=_tmp93D;struct Cyc_Absyn_Exp*e2=_tmp93E;
_tmp93B=e1;_tmp93C=e2;goto _LL10;}case 9U: _LLF: _tmp93B=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp922)->f1;_tmp93C=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp922)->f2;_LL10: {struct Cyc_Absyn_Exp*e1=_tmp93B;struct Cyc_Absyn_Exp*e2=_tmp93C;
# 4172
({Cyc_Toc_subst_pattern_vars(env,e1);});_tmp93A=e2;goto _LL12;}case 42U: _LL11: _tmp93A=((struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct*)_tmp922)->f1;_LL12: {struct Cyc_Absyn_Exp*e=_tmp93A;
_tmp939=e;goto _LL14;}case 39U: _LL13: _tmp939=((struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct*)_tmp922)->f1;_LL14: {struct Cyc_Absyn_Exp*e=_tmp939;
_tmp938=e;goto _LL16;}case 11U: _LL15: _tmp938=((struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct*)_tmp922)->f1;_LL16: {struct Cyc_Absyn_Exp*e=_tmp938;
_tmp937=e;goto _LL18;}case 12U: _LL17: _tmp937=((struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*)_tmp922)->f1;_LL18: {struct Cyc_Absyn_Exp*e=_tmp937;
_tmp936=e;goto _LL1A;}case 13U: _LL19: _tmp936=((struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*)_tmp922)->f1;_LL1A: {struct Cyc_Absyn_Exp*e=_tmp936;
_tmp935=e;goto _LL1C;}case 14U: _LL1B: _tmp935=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp922)->f2;_LL1C: {struct Cyc_Absyn_Exp*e=_tmp935;
_tmp934=e;goto _LL1E;}case 18U: _LL1D: _tmp934=((struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct*)_tmp922)->f1;_LL1E: {struct Cyc_Absyn_Exp*e=_tmp934;
_tmp933=e;goto _LL20;}case 20U: _LL1F: _tmp933=((struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_tmp922)->f1;_LL20: {struct Cyc_Absyn_Exp*e=_tmp933;
_tmp932=e;goto _LL22;}case 21U: _LL21: _tmp932=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp922)->f1;_LL22: {struct Cyc_Absyn_Exp*e=_tmp932;
_tmp931=e;goto _LL24;}case 22U: _LL23: _tmp931=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp922)->f1;_LL24: {struct Cyc_Absyn_Exp*e=_tmp931;
_tmp930=e;goto _LL26;}case 28U: _LL25: _tmp930=((struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*)_tmp922)->f1;_LL26: {struct Cyc_Absyn_Exp*e=_tmp930;
_tmp92F=e;goto _LL28;}case 15U: _LL27: _tmp92F=((struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct*)_tmp922)->f1;_LL28: {struct Cyc_Absyn_Exp*e=_tmp92F;
({Cyc_Toc_subst_pattern_vars(env,e);});goto _LL0;}case 35U: _LL29: _tmp92D=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp922)->f1).rgn;_tmp92E=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp922)->f1).num_elts;_LL2A: {struct Cyc_Absyn_Exp*eopt=_tmp92D;struct Cyc_Absyn_Exp*e=_tmp92E;
_tmp92B=eopt;_tmp92C=e;goto _LL2C;}case 16U: _LL2B: _tmp92B=((struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct*)_tmp922)->f1;_tmp92C=((struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct*)_tmp922)->f2;_LL2C: {struct Cyc_Absyn_Exp*eopt=_tmp92B;struct Cyc_Absyn_Exp*e=_tmp92C;
# 4187
if(eopt != 0)({Cyc_Toc_subst_pattern_vars(env,eopt);});({Cyc_Toc_subst_pattern_vars(env,e);});
goto _LL0;}case 3U: _LL2D: _tmp92A=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp922)->f2;_LL2E: {struct Cyc_List_List*es=_tmp92A;
_tmp929=es;goto _LL30;}case 31U: _LL2F: _tmp929=((struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*)_tmp922)->f1;_LL30: {struct Cyc_List_List*es=_tmp929;
_tmp928=es;goto _LL32;}case 24U: _LL31: _tmp928=((struct Cyc_Absyn_Tuple_e_Absyn_Raw_exp_struct*)_tmp922)->f1;_LL32: {struct Cyc_List_List*es=_tmp928;
({({void(*_tmpF0E)(void(*f)(struct Cyc_List_List*,struct Cyc_Absyn_Exp*),struct Cyc_List_List*env,struct Cyc_List_List*x)=({void(*_tmp952)(void(*f)(struct Cyc_List_List*,struct Cyc_Absyn_Exp*),struct Cyc_List_List*env,struct Cyc_List_List*x)=(void(*)(void(*f)(struct Cyc_List_List*,struct Cyc_Absyn_Exp*),struct Cyc_List_List*env,struct Cyc_List_List*x))Cyc_List_iter_c;_tmp952;});struct Cyc_List_List*_tmpF0D=env;_tmpF0E(Cyc_Toc_subst_pattern_vars,_tmpF0D,es);});});goto _LL0;}case 37U: _LL33: _tmp927=((struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*)_tmp922)->f2;_LL34: {struct Cyc_List_List*dles=_tmp927;
_tmp926=dles;goto _LL36;}case 29U: _LL35: _tmp926=((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_tmp922)->f3;_LL36: {struct Cyc_List_List*dles=_tmp926;
_tmp925=dles;goto _LL38;}case 30U: _LL37: _tmp925=((struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct*)_tmp922)->f2;_LL38: {struct Cyc_List_List*dles=_tmp925;
_tmp924=dles;goto _LL3A;}case 26U: _LL39: _tmp924=((struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*)_tmp922)->f1;_LL3A: {struct Cyc_List_List*dles=_tmp924;
_tmp923=dles;goto _LL3C;}case 25U: _LL3B: _tmp923=((struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct*)_tmp922)->f2;_LL3C: {struct Cyc_List_List*dles=_tmp923;
# 4197
for(0;dles != 0;dles=dles->tl){
struct _tuple21*_stmttmp63=(struct _tuple21*)dles->hd;struct Cyc_Absyn_Exp*_tmp953;_LL45: _tmp953=_stmttmp63->f2;_LL46: {struct Cyc_Absyn_Exp*e=_tmp953;
({Cyc_Toc_subst_pattern_vars(env,e);});}}
# 4201
goto _LL0;}default: _LL3D: _LL3E:
 goto _LL0;}_LL0:;}struct _tuple48{struct Cyc_Toc_Env*f1;struct _fat_ptr*f2;struct Cyc_Absyn_Stmt*f3;};
# 4211
static struct Cyc_Absyn_Stmt*Cyc_Toc_compile_decision_tree(struct _RegionHandle*rgn,struct Cyc_Toc_Env*nv,struct Cyc_List_List**decls,struct Cyc_List_List**bodies,void*dopt,struct Cyc_List_List*lscs,struct _tuple1*v){
# 4219
void*_tmp955=dopt;void*_tmp958;struct Cyc_List_List*_tmp957;struct Cyc_List_List*_tmp956;struct Cyc_Tcpat_Rhs*_tmp959;if(_tmp955 == 0){_LL1: _LL2:
# 4221
 return({Cyc_Absyn_skip_stmt(0U);});}else{switch(*((int*)_tmp955)){case 0U: _LL3: _LL4:
# 4225
 return({Cyc_Toc_throw_match_stmt();});case 1U: _LL5: _tmp959=((struct Cyc_Tcpat_Success_Tcpat_Decision_struct*)_tmp955)->f1;_LL6: {struct Cyc_Tcpat_Rhs*rhs=_tmp959;
# 4228
for(0;lscs != 0;lscs=lscs->tl){
struct _tuple44*_stmttmp64=(struct _tuple44*)lscs->hd;struct Cyc_Absyn_Switch_clause*_tmp95D;struct _fat_ptr*_tmp95C;struct _fat_ptr*_tmp95B;int*_tmp95A;_LLA: _tmp95A=(int*)& _stmttmp64->f1;_tmp95B=_stmttmp64->f2;_tmp95C=_stmttmp64->f3;_tmp95D=_stmttmp64->f4;_LLB: {int*already_emitted=_tmp95A;struct _fat_ptr*init_lab=_tmp95B;struct _fat_ptr*code_lab=_tmp95C;struct Cyc_Absyn_Switch_clause*sc=_tmp95D;
struct Cyc_Absyn_Stmt*body=sc->body;
if(body == rhs->rhs){
# 4233
if(*already_emitted)
return({Cyc_Absyn_goto_stmt(init_lab,0U);});
# 4233
*already_emitted=1;{
# 4236
struct Cyc_List_List*newdecls=0;
# 4238
struct Cyc_Absyn_Stmt*init_opt=({struct Cyc_Absyn_Stmt*(*_tmp967)(struct _RegionHandle*rgn,struct Cyc_Toc_Env**nv,struct Cyc_List_List**decls,struct Cyc_Absyn_Exp*path,struct Cyc_Absyn_Pat*p)=Cyc_Toc_extract_pattern_vars;struct _RegionHandle*_tmp968=rgn;struct Cyc_Toc_Env**_tmp969=& nv;struct Cyc_List_List**_tmp96A=& newdecls;struct Cyc_Absyn_Exp*_tmp96B=({Cyc_Absyn_var_exp(v,0U);});struct Cyc_Absyn_Pat*_tmp96C=sc->pattern;_tmp967(_tmp968,_tmp969,_tmp96A,_tmp96B,_tmp96C);});
# 4242
struct Cyc_Absyn_Stmt*res=sc->body;
{struct Cyc_List_List*ds=newdecls;for(0;ds != 0;ds=ds->tl){
struct _tuple46 _stmttmp65=*((struct _tuple46*)ds->hd);struct Cyc_Absyn_Vardecl*_tmp95F;struct Cyc_Absyn_Vardecl*_tmp95E;_LLD: _tmp95E=_stmttmp65.f1;_tmp95F=_stmttmp65.f2;_LLE: {struct Cyc_Absyn_Vardecl*oldv=_tmp95E;struct Cyc_Absyn_Vardecl*newv=_tmp95F;
({struct Cyc_List_List*_tmpF10=({struct Cyc_List_List*_tmp961=_region_malloc(rgn,sizeof(*_tmp961));({struct _tuple46*_tmpF0F=({struct _tuple46*_tmp960=_region_malloc(rgn,sizeof(*_tmp960));_tmp960->f1=oldv,_tmp960->f2=newv;_tmp960;});_tmp961->hd=_tmpF0F;}),_tmp961->tl=*decls;_tmp961;});*decls=_tmpF10;});
({struct Cyc_Absyn_Exp*_tmpF11=({Cyc_Absyn_varb_exp((void*)({struct Cyc_Absyn_Local_b_Absyn_Binding_struct*_tmp962=_cycalloc(sizeof(*_tmp962));_tmp962->tag=4U,_tmp962->f1=newv;_tmp962;}),0U);});oldv->initializer=_tmpF11;});
((struct Cyc_Absyn_Exp*)_check_null(oldv->initializer))->topt=newv->type;
oldv->type=newv->type;
res=({({struct Cyc_Absyn_Decl*_tmpF13=({struct Cyc_Absyn_Decl*_tmp964=_cycalloc(sizeof(*_tmp964));({void*_tmpF12=(void*)({struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*_tmp963=_cycalloc(sizeof(*_tmp963));_tmp963->tag=0U,_tmp963->f1=oldv;_tmp963;});_tmp964->r=_tmpF12;}),_tmp964->loc=0U;_tmp964;});Cyc_Absyn_decl_stmt(_tmpF13,res,0U);});});}}}
# 4251
res=({Cyc_Absyn_label_stmt(code_lab,res,0U);});
if(init_opt != 0)
res=({Cyc_Absyn_seq_stmt(init_opt,res,0U);});
# 4252
res=({Cyc_Absyn_label_stmt(init_lab,res,0U);});
# 4255
({struct Cyc_List_List*_tmpF15=({struct Cyc_List_List*_tmp966=_region_malloc(rgn,sizeof(*_tmp966));({struct _tuple48*_tmpF14=({struct _tuple48*_tmp965=_region_malloc(rgn,sizeof(*_tmp965));_tmp965->f1=nv,_tmp965->f2=code_lab,_tmp965->f3=body;_tmp965;});_tmp966->hd=_tmpF14;}),_tmp966->tl=*bodies;_tmp966;});*bodies=_tmpF15;});
return res;}}}}
# 4259
({void*_tmp96D=0U;({int(*_tmpF17)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp96F)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmp96F;});struct _fat_ptr _tmpF16=({const char*_tmp96E="couldn't find rhs!";_tag_fat(_tmp96E,sizeof(char),19U);});_tmpF17(_tmpF16,_tag_fat(_tmp96D,sizeof(void*),0U));});});}default: _LL7: _tmp956=((struct Cyc_Tcpat_SwitchDec_Tcpat_Decision_struct*)_tmp955)->f1;_tmp957=((struct Cyc_Tcpat_SwitchDec_Tcpat_Decision_struct*)_tmp955)->f2;_tmp958=(void*)((struct Cyc_Tcpat_SwitchDec_Tcpat_Decision_struct*)_tmp955)->f3;_LL8: {struct Cyc_List_List*symbolic_path=_tmp956;struct Cyc_List_List*switches=_tmp957;void*other_decision=_tmp958;
# 4262
struct Cyc_Absyn_Stmt*res=({Cyc_Toc_compile_decision_tree(rgn,nv,decls,bodies,other_decision,lscs,v);});
# 4264
struct Cyc_Absyn_Exp*p=({struct Cyc_Absyn_Exp*(*_tmp9A8)(struct Cyc_List_List*ps,struct Cyc_Absyn_Exp*v)=Cyc_Toc_compile_path;struct Cyc_List_List*_tmp9A9=({Cyc_List_rev(symbolic_path);});struct Cyc_Absyn_Exp*_tmp9AA=({Cyc_Absyn_var_exp(v,0U);});_tmp9A8(_tmp9A9,_tmp9AA);});
struct Cyc_List_List*ss=({Cyc_List_rev(switches);});
# 4267
struct _tuple45 _stmttmp66=({Cyc_Toc_admits_switch(ss);});void*_tmp971;int _tmp970;_LL10: _tmp970=_stmttmp66.f1;_tmp971=_stmttmp66.f2;_LL11: {int allows_switch=_tmp970;void*test_kind=_tmp971;
if(allows_switch > 1){
# 4271
struct Cyc_List_List*new_lscs=({struct Cyc_List_List*_tmp987=_cycalloc(sizeof(*_tmp987));
({struct Cyc_Absyn_Switch_clause*_tmpF19=({struct Cyc_Absyn_Switch_clause*_tmp986=_cycalloc(sizeof(*_tmp986));({struct Cyc_Absyn_Pat*_tmpF18=({Cyc_Absyn_new_pat((void*)& Cyc_Absyn_Wild_p_val,0U);});_tmp986->pattern=_tmpF18;}),_tmp986->pat_vars=0,_tmp986->where_clause=0,_tmp986->body=res,_tmp986->loc=0U;_tmp986;});_tmp987->hd=_tmpF19;}),_tmp987->tl=0;_tmp987;});
# 4274
for(0;ss != 0;ss=ss->tl){
struct _tuple41 _stmttmp67=*((struct _tuple41*)ss->hd);void*_tmp973;void*_tmp972;_LL13: _tmp972=_stmttmp67.f1;_tmp973=_stmttmp67.f2;_LL14: {void*pat_test=_tmp972;void*dec_tree=_tmp973;
# 4277
struct Cyc_Absyn_Pat*case_exp=({Cyc_Toc_compile_pat_test_as_case(pat_test);});
# 4279
struct Cyc_Absyn_Stmt*s=({Cyc_Toc_compile_decision_tree(rgn,nv,decls,bodies,dec_tree,lscs,v);});
# 4281
new_lscs=({struct Cyc_List_List*_tmp975=_cycalloc(sizeof(*_tmp975));({struct Cyc_Absyn_Switch_clause*_tmpF1A=({struct Cyc_Absyn_Switch_clause*_tmp974=_cycalloc(sizeof(*_tmp974));_tmp974->pattern=case_exp,_tmp974->pat_vars=0,_tmp974->where_clause=0,_tmp974->body=s,_tmp974->loc=0U;_tmp974;});_tmp975->hd=_tmpF1A;}),_tmp975->tl=new_lscs;_tmp975;});}}
# 4283
{void*_tmp976=test_kind;struct _fat_ptr*_tmp977;switch(*((int*)_tmp976)){case 1U: _LL16: _LL17:
# 4285
 LOOP1: {
void*_stmttmp68=p->r;void*_tmp978=_stmttmp68;struct Cyc_Absyn_Exp*_tmp979;struct Cyc_Absyn_Exp*_tmp97A;switch(*((int*)_tmp978)){case 14U: _LL1F: _tmp97A=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp978)->f2;_LL20: {struct Cyc_Absyn_Exp*e=_tmp97A;
p=e;goto LOOP1;}case 20U: _LL21: _tmp979=((struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_tmp978)->f1;_LL22: {struct Cyc_Absyn_Exp*e=_tmp979;
p=e;goto _LL1E;}default: _LL23: _LL24:
 goto _LL1E;}_LL1E:;}
# 4291
p=({struct Cyc_Absyn_Exp*(*_tmp97B)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_deref_exp;struct Cyc_Absyn_Exp*_tmp97C=({struct Cyc_Absyn_Exp*(*_tmp97D)(void*t,struct Cyc_Absyn_Exp*e)=Cyc_Toc_cast_it;void*_tmp97E=({Cyc_Absyn_cstar_type(Cyc_Absyn_sint_type,Cyc_Toc_mt_tq);});struct Cyc_Absyn_Exp*_tmp97F=p;_tmp97D(_tmp97E,_tmp97F);});unsigned _tmp980=0U;_tmp97B(_tmp97C,_tmp980);});goto _LL15;case 3U: _LL18: _tmp977=((struct Cyc_Toc_TaggedUnionTest_Toc_TestKind_struct*)_tmp976)->f1;_LL19: {struct _fat_ptr*f=_tmp977;
# 4293
p=({struct Cyc_Absyn_Exp*(*_tmp981)(struct Cyc_Absyn_Exp*e,struct _fat_ptr*f,unsigned loc)=Cyc_Toc_member_exp;struct Cyc_Absyn_Exp*_tmp982=({Cyc_Toc_member_exp(p,f,0U);});struct _fat_ptr*_tmp983=Cyc_Toc_tag_sp;unsigned _tmp984=0U;_tmp981(_tmp982,_tmp983,_tmp984);});goto _LL15;}case 2U: _LL1A: _LL1B:
 goto _LL1D;default: _LL1C: _LL1D:
 goto _LL15;}_LL15:;}
# 4297
res=({Cyc_Absyn_new_stmt((void*)({struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct*_tmp985=_cycalloc(sizeof(*_tmp985));_tmp985->tag=10U,_tmp985->f1=p,_tmp985->f2=new_lscs,_tmp985->f3=0;_tmp985;}),0U);});}else{
# 4301
void*_tmp988=test_kind;struct Cyc_Absyn_Exp*_tmp989;if(((struct Cyc_Toc_WhereTest_Toc_TestKind_struct*)_tmp988)->tag == 2U){_LL26: _tmp989=((struct Cyc_Toc_WhereTest_Toc_TestKind_struct*)_tmp988)->f1;_LL27: {struct Cyc_Absyn_Exp*e=_tmp989;
# 4303
struct Cyc_List_List*_tmp98A=ss;struct Cyc_Tcpat_Rhs*_tmp98C;void*_tmp98B;if(_tmp98A != 0){if(((struct Cyc_Tcpat_Success_Tcpat_Decision_struct*)((struct _tuple41*)((struct Cyc_List_List*)_tmp98A)->hd)->f2)->tag == 1U){if(((struct Cyc_List_List*)_tmp98A)->tl == 0){_LL2B: _tmp98B=((struct _tuple41*)_tmp98A->hd)->f1;_tmp98C=((struct Cyc_Tcpat_Success_Tcpat_Decision_struct*)((struct _tuple41*)_tmp98A->hd)->f2)->f1;_LL2C: {void*pat_test=_tmp98B;struct Cyc_Tcpat_Rhs*rhs=_tmp98C;
# 4309
for(0;lscs != 0;lscs=lscs->tl){
struct _tuple44*_stmttmp69=(struct _tuple44*)lscs->hd;struct Cyc_Absyn_Switch_clause*_tmp990;struct _fat_ptr*_tmp98F;struct _fat_ptr*_tmp98E;int*_tmp98D;_LL30: _tmp98D=(int*)& _stmttmp69->f1;_tmp98E=_stmttmp69->f2;_tmp98F=_stmttmp69->f3;_tmp990=_stmttmp69->f4;_LL31: {int*already_emitted=_tmp98D;struct _fat_ptr*init_lab=_tmp98E;struct _fat_ptr*code_lab=_tmp98F;struct Cyc_Absyn_Switch_clause*sc=_tmp990;
struct Cyc_Absyn_Stmt*body=sc->body;
if(body == rhs->rhs){
# 4314
if(*already_emitted)
return({Cyc_Absyn_goto_stmt(init_lab,0U);});
# 4314
*already_emitted=1;{
# 4317
struct Cyc_List_List*newdecls=0;
# 4319
struct Cyc_Absyn_Stmt*init_opt=({struct Cyc_Absyn_Stmt*(*_tmp99A)(struct _RegionHandle*rgn,struct Cyc_Toc_Env**nv,struct Cyc_List_List**decls,struct Cyc_Absyn_Exp*path,struct Cyc_Absyn_Pat*p)=Cyc_Toc_extract_pattern_vars;struct _RegionHandle*_tmp99B=rgn;struct Cyc_Toc_Env**_tmp99C=& nv;struct Cyc_List_List**_tmp99D=& newdecls;struct Cyc_Absyn_Exp*_tmp99E=({Cyc_Absyn_var_exp(v,0U);});struct Cyc_Absyn_Pat*_tmp99F=sc->pattern;_tmp99A(_tmp99B,_tmp99C,_tmp99D,_tmp99E,_tmp99F);});
# 4323
struct Cyc_Absyn_Stmt*r=sc->body;
{struct Cyc_List_List*ds=newdecls;for(0;ds != 0;ds=ds->tl){
struct _tuple46 _stmttmp6A=*((struct _tuple46*)ds->hd);struct Cyc_Absyn_Vardecl*_tmp992;struct Cyc_Absyn_Vardecl*_tmp991;_LL33: _tmp991=_stmttmp6A.f1;_tmp992=_stmttmp6A.f2;_LL34: {struct Cyc_Absyn_Vardecl*oldv=_tmp991;struct Cyc_Absyn_Vardecl*newv=_tmp992;
({struct Cyc_List_List*_tmpF1C=({struct Cyc_List_List*_tmp994=_region_malloc(rgn,sizeof(*_tmp994));({struct _tuple46*_tmpF1B=({struct _tuple46*_tmp993=_region_malloc(rgn,sizeof(*_tmp993));_tmp993->f1=oldv,_tmp993->f2=newv;_tmp993;});_tmp994->hd=_tmpF1B;}),_tmp994->tl=*decls;_tmp994;});*decls=_tmpF1C;});
({struct Cyc_Absyn_Exp*_tmpF1D=({Cyc_Absyn_varb_exp((void*)({struct Cyc_Absyn_Local_b_Absyn_Binding_struct*_tmp995=_cycalloc(sizeof(*_tmp995));_tmp995->tag=4U,_tmp995->f1=newv;_tmp995;}),0U);});oldv->initializer=_tmpF1D;});
((struct Cyc_Absyn_Exp*)_check_null(oldv->initializer))->topt=newv->type;
oldv->type=newv->type;
r=({({struct Cyc_Absyn_Decl*_tmpF1F=({struct Cyc_Absyn_Decl*_tmp997=_cycalloc(sizeof(*_tmp997));({void*_tmpF1E=(void*)({struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*_tmp996=_cycalloc(sizeof(*_tmp996));_tmp996->tag=0U,_tmp996->f1=oldv;_tmp996;});_tmp997->r=_tmpF1E;}),_tmp997->loc=0U;_tmp997;});Cyc_Absyn_decl_stmt(_tmpF1F,r,0U);});});}}}
# 4332
r=({Cyc_Absyn_label_stmt(code_lab,r,0U);});
# 4336
({Cyc_Toc_subst_pattern_vars(*decls,e);});
({Cyc_Toc_exp_to_c(nv,e);});
r=({Cyc_Absyn_ifthenelse_stmt(e,r,res,0U);});
if(init_opt != 0)
r=({Cyc_Absyn_seq_stmt(init_opt,r,0U);});
# 4339
r=({Cyc_Absyn_label_stmt(init_lab,r,0U);});
# 4342
({struct Cyc_List_List*_tmpF21=({struct Cyc_List_List*_tmp999=_region_malloc(rgn,sizeof(*_tmp999));({struct _tuple48*_tmpF20=({struct _tuple48*_tmp998=_region_malloc(rgn,sizeof(*_tmp998));_tmp998->f1=nv,_tmp998->f2=code_lab,_tmp998->f3=body;_tmp998;});_tmp999->hd=_tmpF20;}),_tmp999->tl=*bodies;_tmp999;});*bodies=_tmpF21;});
return r;}}}}
# 4346
({void*_tmp9A0=0U;({int(*_tmpF23)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp9A2)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmp9A2;});struct _fat_ptr _tmpF22=({const char*_tmp9A1="couldn't find rhs!";_tag_fat(_tmp9A1,sizeof(char),19U);});_tmpF23(_tmpF22,_tag_fat(_tmp9A0,sizeof(void*),0U));});});}}else{goto _LL2D;}}else{goto _LL2D;}}else{_LL2D: _LL2E:
({void*_tmp9A3=0U;({int(*_tmpF25)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp9A5)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmp9A5;});struct _fat_ptr _tmpF24=({const char*_tmp9A4="bad where clause in match compiler";_tag_fat(_tmp9A4,sizeof(char),35U);});_tmpF25(_tmpF24,_tag_fat(_tmp9A3,sizeof(void*),0U));});});}_LL2A:;}}else{_LL28: _LL29:
# 4351
 for(0;ss != 0;ss=ss->tl){
struct _tuple41 _stmttmp6B=*((struct _tuple41*)ss->hd);void*_tmp9A7;void*_tmp9A6;_LL36: _tmp9A6=_stmttmp6B.f1;_tmp9A7=_stmttmp6B.f2;_LL37: {void*pat_test=_tmp9A6;void*dec_tree=_tmp9A7;
struct Cyc_Absyn_Exp*test_exp=({Cyc_Toc_compile_pat_test(p,pat_test);});
struct Cyc_Absyn_Stmt*s=({Cyc_Toc_compile_decision_tree(rgn,nv,decls,bodies,dec_tree,lscs,v);});
res=({Cyc_Absyn_ifthenelse_stmt(test_exp,s,res,0U);});}}}_LL25:;}
# 4359
return res;}}}}_LL0:;}
# 4370
static struct Cyc_Toc_Env**Cyc_Toc_find_case_env(struct Cyc_List_List*bodies,struct Cyc_Absyn_Stmt*s){
for(0;bodies != 0;bodies=bodies->tl){
struct _tuple48*_stmttmp6C=(struct _tuple48*)bodies->hd;struct Cyc_Absyn_Stmt*_tmp9AD;struct Cyc_Toc_Env**_tmp9AC;_LL1: _tmp9AC=(struct Cyc_Toc_Env**)& _stmttmp6C->f1;_tmp9AD=_stmttmp6C->f3;_LL2: {struct Cyc_Toc_Env**nv=_tmp9AC;struct Cyc_Absyn_Stmt*s2=_tmp9AD;
if(s2 == s)return nv;}}
# 4377
return 0;}
# 4381
static void Cyc_Toc_xlate_switch(struct Cyc_Toc_Env*nv,struct Cyc_Absyn_Stmt*whole_s,struct Cyc_Absyn_Exp*e,struct Cyc_List_List*scs,void*dopt){
# 4384
void*t=(void*)_check_null(e->topt);
({Cyc_Toc_exp_to_c(nv,e);});{
# 4387
struct _tuple33 _stmttmp6D=({Cyc_Toc_temp_var_and_exp();});struct Cyc_Absyn_Exp*_tmp9B0;struct _tuple1*_tmp9AF;_LL1: _tmp9AF=_stmttmp6D.f1;_tmp9B0=_stmttmp6D.f2;_LL2: {struct _tuple1*v=_tmp9AF;struct Cyc_Absyn_Exp*path=_tmp9B0;
struct _fat_ptr*end_l=({Cyc_Toc_fresh_label();});
struct _RegionHandle _tmp9B1=_new_region("rgn");struct _RegionHandle*rgn=& _tmp9B1;_push_region(rgn);
{struct Cyc_Toc_Env*_tmp9B2=({Cyc_Toc_share_env(rgn,nv);});struct Cyc_Toc_Env*nv=_tmp9B2;
# 4393
struct Cyc_List_List*lscs=({({struct Cyc_List_List*(*_tmpF28)(struct _RegionHandle*,struct _tuple44*(*f)(struct _RegionHandle*,struct Cyc_Absyn_Switch_clause*),struct _RegionHandle*env,struct Cyc_List_List*x)=({
struct Cyc_List_List*(*_tmp9DC)(struct _RegionHandle*,struct _tuple44*(*f)(struct _RegionHandle*,struct Cyc_Absyn_Switch_clause*),struct _RegionHandle*env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct _RegionHandle*,struct _tuple44*(*f)(struct _RegionHandle*,struct Cyc_Absyn_Switch_clause*),struct _RegionHandle*env,struct Cyc_List_List*x))Cyc_List_rmap_c;_tmp9DC;});struct _RegionHandle*_tmpF27=rgn;struct _RegionHandle*_tmpF26=rgn;_tmpF28(_tmpF27,Cyc_Toc_gen_labels,_tmpF26,scs);});});
# 4398
struct Cyc_List_List*mydecls=0;
struct Cyc_List_List*mybodies=0;
struct Cyc_Absyn_Stmt*test_tree=({Cyc_Toc_compile_decision_tree(rgn,nv,& mydecls,& mybodies,dopt,lscs,v);});
# 4406
{struct Cyc_List_List*lscs2=lscs;for(0;lscs2 != 0;lscs2=lscs2->tl){
struct _tuple44*_stmttmp6E=(struct _tuple44*)lscs2->hd;struct Cyc_Absyn_Switch_clause*_tmp9B4;struct _fat_ptr*_tmp9B3;_LL4: _tmp9B3=_stmttmp6E->f3;_tmp9B4=_stmttmp6E->f4;_LL5: {struct _fat_ptr*body_lab=_tmp9B3;struct Cyc_Absyn_Switch_clause*body_sc=_tmp9B4;
struct Cyc_Absyn_Stmt*s=body_sc->body;
# 4410
struct Cyc_Toc_Env**envp=({Cyc_Toc_find_case_env(mybodies,s);});
# 4413
if(envp == 0)continue;{struct Cyc_Toc_Env*env=*envp;
# 4416
if(lscs2->tl != 0){
struct _tuple44*_stmttmp6F=(struct _tuple44*)((struct Cyc_List_List*)_check_null(lscs2->tl))->hd;struct Cyc_Absyn_Switch_clause*_tmp9B6;struct _fat_ptr*_tmp9B5;_LL7: _tmp9B5=_stmttmp6F->f3;_tmp9B6=_stmttmp6F->f4;_LL8: {struct _fat_ptr*fallthru_lab=_tmp9B5;struct Cyc_Absyn_Switch_clause*next_sc=_tmp9B6;
# 4420
struct Cyc_Toc_Env**next_case_env=({Cyc_Toc_find_case_env(mybodies,next_sc->body);});
# 4425
if(next_case_env == 0)
({void(*_tmp9B7)(struct Cyc_Toc_Env*nv,struct Cyc_Absyn_Stmt*s)=Cyc_Toc_stmt_to_c;struct Cyc_Toc_Env*_tmp9B8=({Cyc_Toc_last_switchclause_env(rgn,env,end_l);});struct Cyc_Absyn_Stmt*_tmp9B9=s;_tmp9B7(_tmp9B8,_tmp9B9);});else{
# 4429
struct Cyc_List_List*vs=0;
if(next_sc->pat_vars != 0){
vs=({struct Cyc_List_List*(*_tmp9BA)(struct Cyc_List_List*)=Cyc_Tcutil_filter_nulls;struct Cyc_List_List*_tmp9BB=({Cyc_List_split((struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(next_sc->pat_vars))->v);}).f1;_tmp9BA(_tmp9BB);});
vs=({struct Cyc_List_List*(*_tmp9BC)(struct Cyc_Absyn_Vardecl*(*f)(struct Cyc_List_List*,struct Cyc_Absyn_Vardecl*),struct Cyc_List_List*env,struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp9BD)(struct Cyc_Absyn_Vardecl*(*f)(struct Cyc_List_List*,struct Cyc_Absyn_Vardecl*),struct Cyc_List_List*env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_Vardecl*(*f)(struct Cyc_List_List*,struct Cyc_Absyn_Vardecl*),struct Cyc_List_List*env,struct Cyc_List_List*x))Cyc_List_map_c;_tmp9BD;});struct Cyc_Absyn_Vardecl*(*_tmp9BE)(struct Cyc_List_List*l,struct Cyc_Absyn_Vardecl*k)=({struct Cyc_Absyn_Vardecl*(*_tmp9BF)(struct Cyc_List_List*l,struct Cyc_Absyn_Vardecl*k)=(struct Cyc_Absyn_Vardecl*(*)(struct Cyc_List_List*l,struct Cyc_Absyn_Vardecl*k))Cyc_List_assoc;_tmp9BF;});struct Cyc_List_List*_tmp9C0=mydecls;struct Cyc_List_List*_tmp9C1=({Cyc_List_imp_rev(vs);});_tmp9BC(_tmp9BE,_tmp9C0,_tmp9C1);});}
# 4430
({void(*_tmp9C2)(struct Cyc_Toc_Env*nv,struct Cyc_Absyn_Stmt*s)=Cyc_Toc_stmt_to_c;struct Cyc_Toc_Env*_tmp9C3=({Cyc_Toc_non_last_switchclause_env(rgn,env,end_l,fallthru_lab,vs);});struct Cyc_Absyn_Stmt*_tmp9C4=s;_tmp9C2(_tmp9C3,_tmp9C4);});}}}else{
# 4438
({void(*_tmp9C5)(struct Cyc_Toc_Env*nv,struct Cyc_Absyn_Stmt*s)=Cyc_Toc_stmt_to_c;struct Cyc_Toc_Env*_tmp9C6=({Cyc_Toc_last_switchclause_env(rgn,env,end_l);});struct Cyc_Absyn_Stmt*_tmp9C7=s;_tmp9C5(_tmp9C6,_tmp9C7);});}}}}}{
# 4442
struct Cyc_Absyn_Stmt*res=({struct Cyc_Absyn_Stmt*(*_tmp9D4)(struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_seq_stmt;struct Cyc_Absyn_Stmt*_tmp9D5=test_tree;struct Cyc_Absyn_Stmt*_tmp9D6=({struct Cyc_Absyn_Stmt*(*_tmp9D7)(struct _fat_ptr*,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_label_stmt;struct _fat_ptr*_tmp9D8=end_l;struct Cyc_Absyn_Stmt*_tmp9D9=({Cyc_Toc_skip_stmt_dl();});unsigned _tmp9DA=0U;_tmp9D7(_tmp9D8,_tmp9D9,_tmp9DA);});unsigned _tmp9DB=0U;_tmp9D4(_tmp9D5,_tmp9D6,_tmp9DB);});
# 4444
for(0;mydecls != 0;mydecls=((struct Cyc_List_List*)_check_null(mydecls))->tl){
struct _tuple46 _stmttmp70=*((struct _tuple46*)((struct Cyc_List_List*)_check_null(mydecls))->hd);struct Cyc_Absyn_Vardecl*_tmp9C8;_LLA: _tmp9C8=_stmttmp70.f2;_LLB: {struct Cyc_Absyn_Vardecl*vd=_tmp9C8;
res=({struct Cyc_Absyn_Stmt*(*_tmp9C9)(struct Cyc_Absyn_Decl*,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_decl_stmt;struct Cyc_Absyn_Decl*_tmp9CA=({Cyc_Absyn_new_decl((void*)({struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*_tmp9CB=_cycalloc(sizeof(*_tmp9CB));_tmp9CB->tag=0U,_tmp9CB->f1=vd;_tmp9CB;}),0U);});struct Cyc_Absyn_Stmt*_tmp9CC=res;unsigned _tmp9CD=0U;_tmp9C9(_tmp9CA,_tmp9CC,_tmp9CD);});}}
# 4449
({void*_tmpF29=({struct Cyc_Absyn_Stmt*(*_tmp9CE)(struct _tuple1*,void*,struct Cyc_Absyn_Exp*init,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_declare_stmt;struct _tuple1*_tmp9CF=v;void*_tmp9D0=({Cyc_Toc_typ_to_c((void*)_check_null(e->topt));});struct Cyc_Absyn_Exp*_tmp9D1=e;struct Cyc_Absyn_Stmt*_tmp9D2=res;unsigned _tmp9D3=0U;_tmp9CE(_tmp9CF,_tmp9D0,_tmp9D1,_tmp9D2,_tmp9D3);})->r;whole_s->r=_tmpF29;});
_npop_handler(0U);return;}}
# 4390
;_pop_region();}}}
# 4381
static struct Cyc_Absyn_Stmt*Cyc_Toc_letdecl_to_c(struct Cyc_Toc_Env*nv,struct Cyc_Absyn_Pat*p,void*,void*t,struct Cyc_Absyn_Exp*e,struct Cyc_Absyn_Stmt*s);
# 4457
static void Cyc_Toc_local_decl_to_c(struct Cyc_Toc_Env*nv,struct Cyc_Absyn_Vardecl*vd,struct Cyc_Absyn_Stmt*s);
# 4462
struct Cyc_Absyn_Stmt*Cyc_Toc_make_npop_handler(int n){
return({struct Cyc_Absyn_Stmt*(*_tmp9DE)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_exp_stmt;struct Cyc_Absyn_Exp*_tmp9DF=({struct Cyc_Absyn_Exp*_tmp9E0[1U];({struct Cyc_Absyn_Exp*_tmpF2A=({Cyc_Absyn_uint_exp((unsigned)(n - 1),0U);});_tmp9E0[0]=_tmpF2A;});({struct Cyc_Absyn_Exp*_tmpF2B=Cyc_Toc__npop_handler_e;Cyc_Toc_fncall_exp_dl(_tmpF2B,_tag_fat(_tmp9E0,sizeof(struct Cyc_Absyn_Exp*),1U));});});unsigned _tmp9E1=0U;_tmp9DE(_tmp9DF,_tmp9E1);});}
# 4465
void Cyc_Toc_do_npop_before(int n,struct Cyc_Absyn_Stmt*s){
if(n > 0)
({void*_tmpF2C=({void*(*_tmp9E3)(struct Cyc_Absyn_Stmt*s1,struct Cyc_Absyn_Stmt*s2)=Cyc_Toc_seq_stmt_r;struct Cyc_Absyn_Stmt*_tmp9E4=({Cyc_Toc_make_npop_handler(n);});struct Cyc_Absyn_Stmt*_tmp9E5=({Cyc_Absyn_new_stmt(s->r,0U);});_tmp9E3(_tmp9E4,_tmp9E5);});s->r=_tmpF2C;});}
# 4470
static void Cyc_Toc_stmt_to_c(struct Cyc_Toc_Env*nv,struct Cyc_Absyn_Stmt*s){
# 4472
while(1){
void*_stmttmp71=s->r;void*_tmp9E7=_stmttmp71;void*_tmp9EA;struct Cyc_List_List*_tmp9E9;struct Cyc_Absyn_Stmt*_tmp9E8;struct Cyc_Absyn_Stmt*_tmp9EC;struct Cyc_Absyn_Decl*_tmp9EB;struct Cyc_Absyn_Switch_clause**_tmp9EE;struct Cyc_List_List*_tmp9ED;void*_tmp9F1;struct Cyc_List_List*_tmp9F0;struct Cyc_Absyn_Exp*_tmp9EF;struct Cyc_Absyn_Stmt*_tmp9F3;struct _fat_ptr*_tmp9F2;struct Cyc_Absyn_Stmt*_tmp9F5;struct Cyc_Absyn_Exp*_tmp9F4;struct Cyc_Absyn_Exp*_tmp9F7;struct Cyc_Absyn_Stmt*_tmp9F6;struct Cyc_Absyn_Stmt*_tmp9FB;struct Cyc_Absyn_Exp*_tmp9FA;struct Cyc_Absyn_Exp*_tmp9F9;struct Cyc_Absyn_Exp*_tmp9F8;struct Cyc_Absyn_Stmt*_tmp9FE;struct Cyc_Absyn_Stmt*_tmp9FD;struct Cyc_Absyn_Exp*_tmp9FC;struct Cyc_Absyn_Stmt*_tmpA00;struct Cyc_Absyn_Stmt*_tmp9FF;struct Cyc_Absyn_Exp*_tmpA01;struct Cyc_Absyn_Exp*_tmpA02;switch(*((int*)_tmp9E7)){case 0U: _LL1: _LL2:
 return;case 1U: _LL3: _tmpA02=((struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct*)_tmp9E7)->f1;_LL4: {struct Cyc_Absyn_Exp*e=_tmpA02;
({Cyc_Toc_exp_to_c(nv,e);});return;}case 3U: if(((struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct*)_tmp9E7)->f1 == 0){_LL5: _LL6:
({void(*_tmpA03)(int n,struct Cyc_Absyn_Stmt*s)=Cyc_Toc_do_npop_before;int _tmpA04=({Cyc_Toc_get_npop(s);});struct Cyc_Absyn_Stmt*_tmpA05=s;_tmpA03(_tmpA04,_tmpA05);});return;}else{_LL7: _tmpA01=((struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct*)_tmp9E7)->f1;_LL8: {struct Cyc_Absyn_Exp*e=_tmpA01;
# 4479
void*t=({Cyc_Toc_typ_to_c((void*)_check_null(((struct Cyc_Absyn_Exp*)_check_null(e))->topt));});
({Cyc_Toc_exp_to_c(nv,e);});{
int npop=({Cyc_Toc_get_npop(s);});
if(npop > 0){
struct _tuple1*x=({Cyc_Toc_temp_var();});
struct Cyc_Absyn_Stmt*retn_stmt=({struct Cyc_Absyn_Stmt*(*_tmpA06)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_return_stmt;struct Cyc_Absyn_Exp*_tmpA07=({Cyc_Absyn_var_exp(x,0U);});unsigned _tmpA08=0U;_tmpA06(_tmpA07,_tmpA08);});
({Cyc_Toc_do_npop_before(npop,retn_stmt);});
({void*_tmpF2D=({Cyc_Absyn_declare_stmt(x,t,e,retn_stmt,0U);})->r;s->r=_tmpF2D;});}
# 4482
return;}}}case 2U: _LL9: _tmp9FF=((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_tmp9E7)->f1;_tmpA00=((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_tmp9E7)->f2;_LLA: {struct Cyc_Absyn_Stmt*s1=_tmp9FF;struct Cyc_Absyn_Stmt*s2=_tmpA00;
# 4490
({Cyc_Toc_stmt_to_c(nv,s1);});
s=s2;
continue;}case 4U: _LLB: _tmp9FC=((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmp9E7)->f1;_tmp9FD=((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmp9E7)->f2;_tmp9FE=((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmp9E7)->f3;_LLC: {struct Cyc_Absyn_Exp*e=_tmp9FC;struct Cyc_Absyn_Stmt*s1=_tmp9FD;struct Cyc_Absyn_Stmt*s2=_tmp9FE;
# 4494
({Cyc_Toc_exp_to_c(nv,e);});
({Cyc_Toc_stmt_to_c(nv,s1);});
s=s2;
continue;}case 9U: _LLD: _tmp9F8=((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp9E7)->f1;_tmp9F9=(((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp9E7)->f2).f1;_tmp9FA=(((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp9E7)->f3).f1;_tmp9FB=((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp9E7)->f4;_LLE: {struct Cyc_Absyn_Exp*e1=_tmp9F8;struct Cyc_Absyn_Exp*e2=_tmp9F9;struct Cyc_Absyn_Exp*e3=_tmp9FA;struct Cyc_Absyn_Stmt*s2=_tmp9FB;
# 4499
({Cyc_Toc_exp_to_c(nv,e1);});
({Cyc_Toc_exp_to_c(nv,e2);});
_tmp9F6=s2;_tmp9F7=e3;goto _LL10;}case 14U: _LLF: _tmp9F6=((struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct*)_tmp9E7)->f1;_tmp9F7=(((struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct*)_tmp9E7)->f2).f1;_LL10: {struct Cyc_Absyn_Stmt*s2=_tmp9F6;struct Cyc_Absyn_Exp*e=_tmp9F7;
_tmp9F4=e;_tmp9F5=s2;goto _LL12;}case 5U: _LL11: _tmp9F4=(((struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct*)_tmp9E7)->f1).f1;_tmp9F5=((struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct*)_tmp9E7)->f2;_LL12: {struct Cyc_Absyn_Exp*e=_tmp9F4;struct Cyc_Absyn_Stmt*s2=_tmp9F5;
# 4504
({Cyc_Toc_exp_to_c(nv,e);});{
struct _RegionHandle _tmpA09=_new_region("temp");struct _RegionHandle*temp=& _tmpA09;_push_region(temp);({void(*_tmpA0A)(struct Cyc_Toc_Env*nv,struct Cyc_Absyn_Stmt*s)=Cyc_Toc_stmt_to_c;struct Cyc_Toc_Env*_tmpA0B=({Cyc_Toc_loop_env(temp,nv);});struct Cyc_Absyn_Stmt*_tmpA0C=s2;_tmpA0A(_tmpA0B,_tmpA0C);});
_npop_handler(0U);return;
# 4505
;_pop_region();}}case 6U: _LL13: _LL14: {
# 4508
struct _fat_ptr**_tmpA0D;_LL24: _tmpA0D=nv->break_lab;_LL25: {struct _fat_ptr**b=_tmpA0D;
if(b != 0)
({void*_tmpF2E=({Cyc_Toc_goto_stmt_r(*b);});s->r=_tmpF2E;});
# 4509
({void(*_tmpA0E)(int n,struct Cyc_Absyn_Stmt*s)=Cyc_Toc_do_npop_before;int _tmpA0F=({Cyc_Toc_get_npop(s);});struct Cyc_Absyn_Stmt*_tmpA10=s;_tmpA0E(_tmpA0F,_tmpA10);});
# 4513
return;}}case 7U: _LL15: _LL16: {
# 4515
struct _fat_ptr**_tmpA11;_LL27: _tmpA11=nv->continue_lab;_LL28: {struct _fat_ptr**c=_tmpA11;
if(c != 0)
({void*_tmpF2F=({Cyc_Toc_goto_stmt_r(*c);});s->r=_tmpF2F;});
# 4516
goto _LL18;}}case 8U: _LL17: _LL18:
# 4520
({void(*_tmpA12)(int n,struct Cyc_Absyn_Stmt*s)=Cyc_Toc_do_npop_before;int _tmpA13=({Cyc_Toc_get_npop(s);});struct Cyc_Absyn_Stmt*_tmpA14=s;_tmpA12(_tmpA13,_tmpA14);});
return;case 13U: _LL19: _tmp9F2=((struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct*)_tmp9E7)->f1;_tmp9F3=((struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct*)_tmp9E7)->f2;_LL1A: {struct _fat_ptr*lab=_tmp9F2;struct Cyc_Absyn_Stmt*s1=_tmp9F3;
s=s1;continue;}case 10U: _LL1B: _tmp9EF=((struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct*)_tmp9E7)->f1;_tmp9F0=((struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct*)_tmp9E7)->f2;_tmp9F1=(void*)((struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct*)_tmp9E7)->f3;_LL1C: {struct Cyc_Absyn_Exp*e=_tmp9EF;struct Cyc_List_List*scs=_tmp9F0;void*dec_tree_opt=_tmp9F1;
# 4524
({Cyc_Toc_xlate_switch(nv,s,e,scs,dec_tree_opt);});
return;}case 11U: _LL1D: _tmp9ED=((struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct*)_tmp9E7)->f1;_tmp9EE=((struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct*)_tmp9E7)->f2;_LL1E: {struct Cyc_List_List*es=_tmp9ED;struct Cyc_Absyn_Switch_clause**dest_clause=_tmp9EE;
# 4527
struct Cyc_Toc_FallthruInfo*_tmpA15;_LL2A: _tmpA15=nv->fallthru_info;_LL2B: {struct Cyc_Toc_FallthruInfo*fi=_tmpA15;
if(fi == 0)
({void*_tmpA16=0U;({int(*_tmpF31)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmpA18)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmpA18;});struct _fat_ptr _tmpF30=({const char*_tmpA17="fallthru in unexpected place";_tag_fat(_tmpA17,sizeof(char),29U);});_tmpF31(_tmpF30,_tag_fat(_tmpA16,sizeof(void*),0U));});});{
# 4528
struct Cyc_Toc_FallthruInfo _stmttmp72=*fi;struct Cyc_List_List*_tmpA1A;struct _fat_ptr*_tmpA19;_LL2D: _tmpA19=_stmttmp72.label;_tmpA1A=_stmttmp72.binders;_LL2E: {struct _fat_ptr*l=_tmpA19;struct Cyc_List_List*vs=_tmpA1A;
# 4531
struct Cyc_Absyn_Stmt*s2=({Cyc_Absyn_goto_stmt(l,0U);});
# 4533
({void(*_tmpA1B)(int n,struct Cyc_Absyn_Stmt*s)=Cyc_Toc_do_npop_before;int _tmpA1C=({Cyc_Toc_get_npop(s);});struct Cyc_Absyn_Stmt*_tmpA1D=s2;_tmpA1B(_tmpA1C,_tmpA1D);});{
struct Cyc_List_List*vs2=({Cyc_List_rev(vs);});
struct Cyc_List_List*es2=({Cyc_List_rev(es);});
for(0;vs2 != 0;(vs2=vs2->tl,es2=es2->tl)){
({Cyc_Toc_exp_to_c(nv,(struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(es2))->hd);});
s2=({struct Cyc_Absyn_Stmt*(*_tmpA1E)(struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_seq_stmt;struct Cyc_Absyn_Stmt*_tmpA1F=({struct Cyc_Absyn_Stmt*(*_tmpA20)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_assign_stmt;struct Cyc_Absyn_Exp*_tmpA21=({Cyc_Absyn_varb_exp((void*)({struct Cyc_Absyn_Pat_b_Absyn_Binding_struct*_tmpA22=_cycalloc(sizeof(*_tmpA22));_tmpA22->tag=5U,_tmpA22->f1=(struct Cyc_Absyn_Vardecl*)vs2->hd;_tmpA22;}),0U);});struct Cyc_Absyn_Exp*_tmpA23=(struct Cyc_Absyn_Exp*)es2->hd;unsigned _tmpA24=0U;_tmpA20(_tmpA21,_tmpA23,_tmpA24);});struct Cyc_Absyn_Stmt*_tmpA25=s2;unsigned _tmpA26=0U;_tmpA1E(_tmpA1F,_tmpA25,_tmpA26);});}
# 4541
s->r=s2->r;
return;}}}}}case 12U: _LL1F: _tmp9EB=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp9E7)->f1;_tmp9EC=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp9E7)->f2;_LL20: {struct Cyc_Absyn_Decl*d=_tmp9EB;struct Cyc_Absyn_Stmt*s1=_tmp9EC;
# 4547
{void*_stmttmp73=d->r;void*_tmpA27=_stmttmp73;struct Cyc_Absyn_Exp*_tmpA2A;struct Cyc_Absyn_Vardecl*_tmpA29;struct Cyc_Absyn_Tvar*_tmpA28;struct Cyc_Absyn_Fndecl*_tmpA2B;struct Cyc_List_List*_tmpA2C;void*_tmpA2F;struct Cyc_Absyn_Exp*_tmpA2E;struct Cyc_Absyn_Pat*_tmpA2D;struct Cyc_Absyn_Vardecl*_tmpA30;switch(*((int*)_tmpA27)){case 0U: _LL30: _tmpA30=((struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_tmpA27)->f1;_LL31: {struct Cyc_Absyn_Vardecl*vd=_tmpA30;
({Cyc_Toc_local_decl_to_c(nv,vd,s1);});goto _LL2F;}case 2U: _LL32: _tmpA2D=((struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct*)_tmpA27)->f1;_tmpA2E=((struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct*)_tmpA27)->f3;_tmpA2F=(void*)((struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct*)_tmpA27)->f4;_LL33: {struct Cyc_Absyn_Pat*p=_tmpA2D;struct Cyc_Absyn_Exp*e=_tmpA2E;void*dec_tree=_tmpA2F;
# 4557
{void*_stmttmp74=p->r;void*_tmpA31=_stmttmp74;struct Cyc_Absyn_Vardecl*_tmpA32;if(((struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct*)_tmpA31)->tag == 1U){if(((struct Cyc_Absyn_Wild_p_Absyn_Raw_pat_struct*)((struct Cyc_Absyn_Pat*)((struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct*)_tmpA31)->f2)->r)->tag == 0U){_LL3D: _tmpA32=((struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct*)_tmpA31)->f1;_LL3E: {struct Cyc_Absyn_Vardecl*vd=_tmpA32;
# 4559
if(({Cyc_Absyn_var_may_appear_exp(vd->name,e);})){
struct Cyc_Absyn_Vardecl*new_vd=({struct Cyc_Absyn_Vardecl*(*_tmpA3E)(unsigned varloc,struct _tuple1*,void*,struct Cyc_Absyn_Exp*init)=Cyc_Absyn_new_vardecl;unsigned _tmpA3F=0U;struct _tuple1*_tmpA40=({Cyc_Toc_temp_var();});void*_tmpA41=vd->type;struct Cyc_Absyn_Exp*_tmpA42=e;_tmpA3E(_tmpA3F,_tmpA40,_tmpA41,_tmpA42);});
({struct Cyc_Absyn_Exp*_tmpF32=({Cyc_Absyn_varb_exp((void*)({struct Cyc_Absyn_Local_b_Absyn_Binding_struct*_tmpA33=_cycalloc(sizeof(*_tmpA33));_tmpA33->tag=4U,_tmpA33->f1=new_vd;_tmpA33;}),0U);});vd->initializer=_tmpF32;});
((struct Cyc_Absyn_Exp*)_check_null(vd->initializer))->topt=new_vd->type;
({void*_tmpF35=({struct Cyc_Absyn_Stmt*(*_tmpA34)(struct Cyc_Absyn_Decl*,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_decl_stmt;struct Cyc_Absyn_Decl*_tmpA35=({({void*_tmpF33=(void*)({struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*_tmpA36=_cycalloc(sizeof(*_tmpA36));_tmpA36->tag=0U,_tmpA36->f1=new_vd;_tmpA36;});Cyc_Absyn_new_decl(_tmpF33,s->loc);});});struct Cyc_Absyn_Stmt*_tmpA37=({struct Cyc_Absyn_Stmt*(*_tmpA38)(struct Cyc_Absyn_Decl*,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_decl_stmt;struct Cyc_Absyn_Decl*_tmpA39=({({void*_tmpF34=(void*)({struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*_tmpA3A=_cycalloc(sizeof(*_tmpA3A));
_tmpA3A->tag=0U,_tmpA3A->f1=vd;_tmpA3A;});Cyc_Absyn_new_decl(_tmpF34,s->loc);});});struct Cyc_Absyn_Stmt*_tmpA3B=s1;unsigned _tmpA3C=s->loc;_tmpA38(_tmpA39,_tmpA3B,_tmpA3C);});unsigned _tmpA3D=s->loc;_tmpA34(_tmpA35,_tmpA37,_tmpA3D);})->r;
# 4563
s->r=_tmpF35;});}else{
# 4567
vd->initializer=e;
({void*_tmpF37=({struct Cyc_Absyn_Stmt*(*_tmpA43)(struct Cyc_Absyn_Decl*,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_decl_stmt;struct Cyc_Absyn_Decl*_tmpA44=({({void*_tmpF36=(void*)({struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*_tmpA45=_cycalloc(sizeof(*_tmpA45));_tmpA45->tag=0U,_tmpA45->f1=vd;_tmpA45;});Cyc_Absyn_new_decl(_tmpF36,s->loc);});});struct Cyc_Absyn_Stmt*_tmpA46=s1;unsigned _tmpA47=s->loc;_tmpA43(_tmpA44,_tmpA46,_tmpA47);})->r;s->r=_tmpF37;});}
# 4570
({Cyc_Toc_stmt_to_c(nv,s);});
goto _LL3C;}}else{goto _LL3F;}}else{_LL3F: _LL40:
# 4576
({void*_tmpF38=({Cyc_Toc_letdecl_to_c(nv,p,dec_tree,(void*)_check_null(e->topt),e,s1);})->r;s->r=_tmpF38;});
goto _LL3C;}_LL3C:;}
# 4599 "toc.cyc"
goto _LL2F;}case 3U: _LL34: _tmpA2C=((struct Cyc_Absyn_Letv_d_Absyn_Raw_decl_struct*)_tmpA27)->f1;_LL35: {struct Cyc_List_List*vds=_tmpA2C;
# 4603
struct Cyc_List_List*rvds=({Cyc_List_rev(vds);});
if(rvds == 0)
({void*_tmpA48=0U;({int(*_tmpF3A)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmpA4A)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmpA4A;});struct _fat_ptr _tmpF39=({const char*_tmpA49="empty Letv_d";_tag_fat(_tmpA49,sizeof(char),13U);});_tmpF3A(_tmpF39,_tag_fat(_tmpA48,sizeof(void*),0U));});});
# 4604
({void*_tmpF3B=(void*)({struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*_tmpA4B=_cycalloc(sizeof(*_tmpA4B));
# 4606
_tmpA4B->tag=0U,_tmpA4B->f1=(struct Cyc_Absyn_Vardecl*)rvds->hd;_tmpA4B;});
# 4604
d->r=_tmpF3B;});
# 4607
rvds=rvds->tl;
for(0;rvds != 0;rvds=rvds->tl){
struct Cyc_Absyn_Decl*d2=({Cyc_Absyn_new_decl((void*)({struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*_tmpA50=_cycalloc(sizeof(*_tmpA50));_tmpA50->tag=0U,_tmpA50->f1=(struct Cyc_Absyn_Vardecl*)rvds->hd;_tmpA50;}),0U);});
({void*_tmpF3C=({struct Cyc_Absyn_Stmt*(*_tmpA4C)(struct Cyc_Absyn_Decl*,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_decl_stmt;struct Cyc_Absyn_Decl*_tmpA4D=d2;struct Cyc_Absyn_Stmt*_tmpA4E=({Cyc_Absyn_new_stmt(s->r,0U);});unsigned _tmpA4F=0U;_tmpA4C(_tmpA4D,_tmpA4E,_tmpA4F);})->r;s->r=_tmpF3C;});}
# 4612
({Cyc_Toc_stmt_to_c(nv,s);});
goto _LL2F;}case 1U: _LL36: _tmpA2B=((struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*)_tmpA27)->f1;_LL37: {struct Cyc_Absyn_Fndecl*fd=_tmpA2B;
# 4615
({Cyc_Toc_fndecl_to_c(nv,fd,0);});
({Cyc_Toc_stmt_to_c(nv,s1);});
goto _LL2F;}case 4U: _LL38: _tmpA28=((struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*)_tmpA27)->f1;_tmpA29=((struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*)_tmpA27)->f2;_tmpA2A=((struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*)_tmpA27)->f3;_LL39: {struct Cyc_Absyn_Tvar*tv=_tmpA28;struct Cyc_Absyn_Vardecl*vd=_tmpA29;struct Cyc_Absyn_Exp*open_exp_opt=_tmpA2A;
# 4619
struct Cyc_Absyn_Stmt*body=s1;
# 4621
void*rh_struct_typ=({Cyc_Absyn_strct(Cyc_Toc__RegionHandle_sp);});
void*rh_struct_ptr_typ=({Cyc_Absyn_cstar_type(rh_struct_typ,Cyc_Toc_mt_tq);});
struct _tuple33 _stmttmp75=({Cyc_Toc_temp_var_and_exp();});struct Cyc_Absyn_Exp*_tmpA52;struct _tuple1*_tmpA51;_LL42: _tmpA51=_stmttmp75.f1;_tmpA52=_stmttmp75.f2;_LL43: {struct _tuple1*rh_var=_tmpA51;struct Cyc_Absyn_Exp*rh_exp=_tmpA52;
struct _tuple1*x_var=vd->name;
struct Cyc_Absyn_Exp*x_exp=({Cyc_Absyn_var_exp(x_var,0U);});
int is_xopen=tv->identity == - 2;
int is__xrgn=({Cyc_Absyn_is_xrgn_tvar(tv);});
unsigned xc=Cyc_Toc_xrgn_count;
void*xrh_struct_typ=({Cyc_Absyn_strct(Cyc_Toc__XRegionHandle_sp);});
void*xrh_struct_ptr_typ=({Cyc_Absyn_cstar_type(xrh_struct_typ,Cyc_Toc_mt_tq);});
# 4632
if(is__xrgn){
++ Cyc_Toc_xrgn_count;
# 4635
Cyc_Toc_xrgn_map=({struct Cyc_List_List*_tmpA54=_cycalloc(sizeof(*_tmpA54));({struct _tuple23*_tmpF3D=({struct _tuple23*_tmpA53=_cycalloc(sizeof(*_tmpA53));_tmpA53->f1=*tv->name,_tmpA53->f2=xc;_tmpA53;});_tmpA54->hd=_tmpF3D;}),_tmpA54->tl=Cyc_Toc_xrgn_map;_tmpA54;});}
# 4632
({Cyc_Toc_stmt_to_c(nv,body);});
# 4638
if(Cyc_Absyn_no_regions){
if(is__xrgn)({void*_tmpA55=0U;({int(*_tmpF3F)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmpA57)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmpA57;});struct _fat_ptr _tmpF3E=({const char*_tmpA56="Cannot generate code for XRegions when -noregions flag is set";_tag_fat(_tmpA56,sizeof(char),62U);});_tmpF3F(_tmpF3E,_tag_fat(_tmpA55,sizeof(void*),0U));});});({void*_tmpF40=({struct Cyc_Absyn_Stmt*(*_tmpA58)(struct _tuple1*,void*,struct Cyc_Absyn_Exp*init,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_declare_stmt;struct _tuple1*_tmpA59=x_var;void*_tmpA5A=rh_struct_ptr_typ;struct Cyc_Absyn_Exp*_tmpA5B=({Cyc_Absyn_uint_exp(0U,0U);});struct Cyc_Absyn_Stmt*_tmpA5C=body;unsigned _tmpA5D=0U;_tmpA58(_tmpA59,_tmpA5A,_tmpA5B,_tmpA5C,_tmpA5D);})->r;s->r=_tmpF40;});}else{
# 4642
if((unsigned)open_exp_opt){
if(!is__xrgn && !is_xopen){
# 4645
({Cyc_Toc_exp_to_c(nv,open_exp_opt);});{
struct Cyc_Absyn_Exp*arg=({struct Cyc_Absyn_Exp*(*_tmpA5E)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_address_exp;struct Cyc_Absyn_Exp*_tmpA5F=({({struct Cyc_Absyn_Exp*_tmpF42=open_exp_opt;Cyc_Absyn_aggrarrow_exp(_tmpF42,({struct _fat_ptr*_tmpA61=_cycalloc(sizeof(*_tmpA61));({struct _fat_ptr _tmpF41=({const char*_tmpA60="h";_tag_fat(_tmpA60,sizeof(char),2U);});*_tmpA61=_tmpF41;});_tmpA61;}),0U);});});unsigned _tmpA62=0U;_tmpA5E(_tmpA5F,_tmpA62);});
({void*_tmpF43=({Cyc_Absyn_declare_stmt(x_var,rh_struct_ptr_typ,arg,body,0U);})->r;s->r=_tmpF43;});}}else{
if(!is__xrgn && is_xopen){
# 4650
({Cyc_Toc_exp_to_c(nv,open_exp_opt);});{
struct Cyc_Absyn_Exp*arg=({Cyc_Toc_cast_it(rh_struct_ptr_typ,open_exp_opt);});
({void*_tmpF44=({Cyc_Absyn_declare_stmt(x_var,rh_struct_ptr_typ,arg,body,0U);})->r;s->r=_tmpF44;});}}else{
# 4659
void*xtree_struct_typ=({Cyc_Absyn_strct(Cyc_Toc__XTreeHandle_sp);});
# 4661
void*xtree_struct_ptr_typ=({Cyc_Absyn_cstar_type(xtree_struct_typ,Cyc_Toc_mt_tq);});
struct _tuple33 _stmttmp76=({Cyc_Toc_temp_var_and_exp();});struct Cyc_Absyn_Exp*_tmpA64;struct _tuple1*_tmpA63;_LL45: _tmpA63=_stmttmp76.f1;_tmpA64=_stmttmp76.f2;_LL46: {struct _tuple1*y_var=_tmpA63;struct Cyc_Absyn_Exp*y_exp=_tmpA64;
# 4665
struct Cyc_Absyn_Exp*arg=0;
struct Cyc_Absyn_Exp*par_idx_exp;
void*c=({Cyc_Tcutil_compress((void*)_check_null(open_exp_opt->topt));});
{void*_tmpA65=c;void*_tmpA66;void*_tmpA67;struct Cyc_List_List*_tmpA68;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpA65)->tag == 0U)switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpA65)->f1)){case 6U: _LL48: _LL49:
 _tmpA68=0;goto _LL4B;case 3U: _LL4A: _tmpA68=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpA65)->f2;if((void*)((struct Cyc_List_List*)_check_null(_tmpA68))->hd == Cyc_Absyn_heap_rgn_type){_LL4B: {struct Cyc_List_List*ts=_tmpA68;
# 4672
arg=({Cyc_Absyn_uint_exp(0U,0U);});
par_idx_exp=({Cyc_Absyn_uint_exp(0U,0U);});
goto _LL47;}}else{if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpA65)->f2 != 0){_LL4C: _tmpA67=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpA65)->f2)->hd;_LL4D: {void*r_par=_tmpA67;
_tmpA66=r_par;goto _LL4F;}}else{goto _LL50;}}case 4U: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpA65)->f2 != 0){_LL4E: _tmpA66=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpA65)->f2)->hd;_LL4F: {void*r_par=_tmpA66;
# 4677
arg=open_exp_opt;
par_idx_exp=({struct Cyc_Absyn_Exp*(*_tmpA69)(unsigned,unsigned)=Cyc_Absyn_uint_exp;unsigned _tmpA6A=({Cyc_Toc_search_rgnmap(r_par);});unsigned _tmpA6B=0U;_tmpA69(_tmpA6A,_tmpA6B);});
goto _LL47;}}else{goto _LL50;}default: goto _LL50;}else{_LL50: _LL51:
# 4681
({struct Cyc_String_pa_PrintArg_struct _tmpA6F=({struct Cyc_String_pa_PrintArg_struct _tmpC9B;_tmpC9B.tag=0U,({struct _fat_ptr _tmpF45=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(c);}));_tmpC9B.f1=_tmpF45;});_tmpC9B;});void*_tmpA6C[1U];_tmpA6C[0]=& _tmpA6F;({int(*_tmpF47)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmpA6E)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmpA6E;});struct _fat_ptr _tmpF46=({const char*_tmpA6D="Region_d : XRgn : %s";_tag_fat(_tmpA6D,sizeof(char),21U);});_tmpF47(_tmpF46,_tag_fat(_tmpA6C,sizeof(void*),1U));});});
goto _LL47;}_LL47:;}
# 4685
arg=({struct Cyc_Absyn_Exp*(*_tmpA70)(void*t,struct Cyc_Absyn_Exp*e)=Cyc_Toc_cast_it;void*_tmpA71=xrh_struct_ptr_typ;struct Cyc_Absyn_Exp*_tmpA72=({struct Cyc_Absyn_Exp*_tmpA73[3U];({
# 4687
struct Cyc_Absyn_Exp*_tmpF4A=({Cyc_Toc_cast_it(xrh_struct_ptr_typ,arg);});_tmpA73[0]=_tmpF4A;}),({
struct Cyc_Absyn_Exp*_tmpF49=({Cyc_Absyn_uint_exp(xc,0U);});_tmpA73[1]=_tmpF49;}),({
# 4690
struct Cyc_Absyn_Exp*_tmpF48=({Cyc_Absyn_address_exp(y_exp,0U);});_tmpA73[2]=_tmpF48;});({struct Cyc_Absyn_Exp*_tmpF4B=Cyc_Toc__new_xregion_e;Cyc_Toc_fncall_exp_dl(_tmpF4B,_tag_fat(_tmpA73,sizeof(struct Cyc_Absyn_Exp*),3U));});});_tmpA70(_tmpA71,_tmpA72);});
# 4693
({void*_tmpF4C=({struct Cyc_Absyn_Stmt*(*_tmpA74)(struct _tuple1*,void*,struct Cyc_Absyn_Exp*init,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_declare_stmt;struct _tuple1*_tmpA75=y_var;void*_tmpA76=xtree_struct_typ;struct Cyc_Absyn_Exp*_tmpA77=0;struct Cyc_Absyn_Stmt*_tmpA78=({Cyc_Absyn_declare_stmt(x_var,xrh_struct_ptr_typ,arg,body,0U);});unsigned _tmpA79=0U;_tmpA74(_tmpA75,_tmpA76,_tmpA77,_tmpA78,_tmpA79);})->r;s->r=_tmpF4C;});}}}}else{
# 4704
({void*_tmpF51=({struct Cyc_Absyn_Stmt*(*_tmpA7A)(struct _tuple1*,void*,struct Cyc_Absyn_Exp*init,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_declare_stmt;struct _tuple1*_tmpA7B=rh_var;void*_tmpA7C=rh_struct_typ;struct Cyc_Absyn_Exp*_tmpA7D=({struct Cyc_Absyn_Exp*_tmpA7E[1U];({struct Cyc_Absyn_Exp*_tmpF4D=({struct Cyc_Absyn_Exp*(*_tmpA7F)(struct _fat_ptr,unsigned)=Cyc_Absyn_string_exp;struct _fat_ptr _tmpA80=({Cyc_Absynpp_qvar2string(x_var);});unsigned _tmpA81=0U;_tmpA7F(_tmpA80,_tmpA81);});_tmpA7E[0]=_tmpF4D;});({struct Cyc_Absyn_Exp*_tmpF4E=Cyc_Toc__new_region_e;Cyc_Toc_fncall_exp_dl(_tmpF4E,_tag_fat(_tmpA7E,sizeof(struct Cyc_Absyn_Exp*),1U));});});struct Cyc_Absyn_Stmt*_tmpA82=({struct Cyc_Absyn_Stmt*(*_tmpA83)(struct _tuple1*,void*,struct Cyc_Absyn_Exp*init,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_declare_stmt;struct _tuple1*_tmpA84=x_var;void*_tmpA85=rh_struct_ptr_typ;struct Cyc_Absyn_Exp*_tmpA86=({Cyc_Absyn_address_exp(rh_exp,0U);});struct Cyc_Absyn_Stmt*_tmpA87=({struct Cyc_Absyn_Stmt*(*_tmpA88)(struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_seq_stmt;struct Cyc_Absyn_Stmt*_tmpA89=({struct Cyc_Absyn_Stmt*(*_tmpA8A)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_exp_stmt;struct Cyc_Absyn_Exp*_tmpA8B=({struct Cyc_Absyn_Exp*_tmpA8C[1U];_tmpA8C[0]=x_exp;({struct Cyc_Absyn_Exp*_tmpF4F=Cyc_Toc__push_region_e;Cyc_Toc_fncall_exp_dl(_tmpF4F,_tag_fat(_tmpA8C,sizeof(struct Cyc_Absyn_Exp*),1U));});});unsigned _tmpA8D=0U;_tmpA8A(_tmpA8B,_tmpA8D);});struct Cyc_Absyn_Stmt*_tmpA8E=({struct Cyc_Absyn_Stmt*(*_tmpA8F)(struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_seq_stmt;struct Cyc_Absyn_Stmt*_tmpA90=body;struct Cyc_Absyn_Stmt*_tmpA91=({struct Cyc_Absyn_Stmt*(*_tmpA92)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_exp_stmt;struct Cyc_Absyn_Exp*_tmpA93=({Cyc_Absyn_new_exp((void*)({struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*_tmpA95=_cycalloc(sizeof(*_tmpA95));_tmpA95->tag=10U,_tmpA95->f1=Cyc_Toc__pop_region_e,_tmpA95->f2=0,_tmpA95->f3=0,_tmpA95->f4=0,({struct _tuple0*_tmpF50=({struct _tuple0*_tmpA94=_cycalloc(sizeof(*_tmpA94));_tmpA94->f1=0,_tmpA94->f2=0;_tmpA94;});_tmpA95->f5=_tmpF50;});_tmpA95;}),0U);});unsigned _tmpA96=0U;_tmpA92(_tmpA93,_tmpA96);});unsigned _tmpA97=0U;_tmpA8F(_tmpA90,_tmpA91,_tmpA97);});unsigned _tmpA98=0U;_tmpA88(_tmpA89,_tmpA8E,_tmpA98);});unsigned _tmpA99=0U;_tmpA83(_tmpA84,_tmpA85,_tmpA86,_tmpA87,_tmpA99);});unsigned _tmpA9A=0U;_tmpA7A(_tmpA7B,_tmpA7C,_tmpA7D,_tmpA82,_tmpA9A);})->r;s->r=_tmpF51;});}}
# 4715
return;}}default: _LL3A: _LL3B:
({void*_tmpA9B=0U;({int(*_tmpF53)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmpA9D)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmpA9D;});struct _fat_ptr _tmpF52=({const char*_tmpA9C="bad nested declaration within function";_tag_fat(_tmpA9C,sizeof(char),39U);});_tmpF53(_tmpF52,_tag_fat(_tmpA9B,sizeof(void*),0U));});});}_LL2F:;}
# 4718
return;}default: _LL21: _tmp9E8=((struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct*)_tmp9E7)->f1;_tmp9E9=((struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct*)_tmp9E7)->f2;_tmp9EA=(void*)((struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct*)_tmp9E7)->f3;_LL22: {struct Cyc_Absyn_Stmt*body=_tmp9E8;struct Cyc_List_List*scs=_tmp9E9;void*dec_tree=_tmp9EA;
# 4732 "toc.cyc"
struct _tuple33 _stmttmp77=({Cyc_Toc_temp_var_and_exp();});struct Cyc_Absyn_Exp*_tmpA9F;struct _tuple1*_tmpA9E;_LL53: _tmpA9E=_stmttmp77.f1;_tmpA9F=_stmttmp77.f2;_LL54: {struct _tuple1*h_var=_tmpA9E;struct Cyc_Absyn_Exp*h_exp=_tmpA9F;
struct _tuple33 _stmttmp78=({Cyc_Toc_temp_var_and_exp();});struct Cyc_Absyn_Exp*_tmpAA1;struct _tuple1*_tmpAA0;_LL56: _tmpAA0=_stmttmp78.f1;_tmpAA1=_stmttmp78.f2;_LL57: {struct _tuple1*e_var=_tmpAA0;struct Cyc_Absyn_Exp*e_exp=_tmpAA1;
struct _tuple33 _stmttmp79=({Cyc_Toc_temp_var_and_exp();});struct Cyc_Absyn_Exp*_tmpAA3;struct _tuple1*_tmpAA2;_LL59: _tmpAA2=_stmttmp79.f1;_tmpAA3=_stmttmp79.f2;_LL5A: {struct _tuple1*was_thrown_var=_tmpAA2;struct Cyc_Absyn_Exp*was_thrown_exp=_tmpAA3;
void*h_typ=({Cyc_Absyn_strct(Cyc_Toc__handler_cons_sp);});
void*e_typ=({void*(*_tmpADA)(void*t)=Cyc_Toc_typ_to_c;void*_tmpADB=({Cyc_Absyn_exn_type();});_tmpADA(_tmpADB);});
void*was_thrown_typ=({Cyc_Toc_typ_to_c(Cyc_Absyn_sint_type);});
# 4739
e_exp->topt=e_typ;{
struct _RegionHandle _tmpAA4=_new_region("temp");struct _RegionHandle*temp=& _tmpAA4;_push_region(temp);
# 4742
({Cyc_Toc_stmt_to_c(nv,body);});{
struct Cyc_Absyn_Stmt*tryandpop_stmt=({struct Cyc_Absyn_Stmt*(*_tmpAD2)(struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_seq_stmt;struct Cyc_Absyn_Stmt*_tmpAD3=body;struct Cyc_Absyn_Stmt*_tmpAD4=({struct Cyc_Absyn_Stmt*(*_tmpAD5)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_exp_stmt;struct Cyc_Absyn_Exp*_tmpAD6=({void*_tmpAD7=0U;({struct Cyc_Absyn_Exp*_tmpF54=Cyc_Toc__pop_handler_e;Cyc_Toc_fncall_exp_dl(_tmpF54,_tag_fat(_tmpAD7,sizeof(struct Cyc_Absyn_Exp*),0U));});});unsigned _tmpAD8=0U;_tmpAD5(_tmpAD6,_tmpAD8);});unsigned _tmpAD9=0U;_tmpAD2(_tmpAD3,_tmpAD4,_tmpAD9);});
# 4746
struct Cyc_Absyn_Stmt*handler_stmt=({Cyc_Absyn_new_stmt((void*)({struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct*_tmpAD1=_cycalloc(sizeof(*_tmpAD1));_tmpAD1->tag=10U,_tmpAD1->f1=e_exp,_tmpAD1->f2=scs,_tmpAD1->f3=dec_tree;_tmpAD1;}),0U);});
# 4748
({Cyc_Toc_stmt_to_c(nv,handler_stmt);});{
# 4751
struct Cyc_Absyn_Exp*setjmp_call=({struct Cyc_Absyn_Exp*_tmpAD0[1U];({struct Cyc_Absyn_Exp*_tmpF55=({Cyc_Toc_member_exp(h_exp,Cyc_Toc_handler_sp,0U);});_tmpAD0[0]=_tmpF55;});({struct Cyc_Absyn_Exp*_tmpF56=Cyc_Toc_setjmp_e;Cyc_Toc_fncall_exp_dl(_tmpF56,_tag_fat(_tmpAD0,sizeof(struct Cyc_Absyn_Exp*),1U));});});
# 4753
struct Cyc_Absyn_Stmt*pushhandler_call=({struct Cyc_Absyn_Stmt*(*_tmpACC)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_exp_stmt;struct Cyc_Absyn_Exp*_tmpACD=({struct Cyc_Absyn_Exp*_tmpACE[1U];({struct Cyc_Absyn_Exp*_tmpF57=({Cyc_Absyn_address_exp(h_exp,0U);});_tmpACE[0]=_tmpF57;});({struct Cyc_Absyn_Exp*_tmpF58=Cyc_Toc__push_handler_e;Cyc_Toc_fncall_exp_dl(_tmpF58,_tag_fat(_tmpACE,sizeof(struct Cyc_Absyn_Exp*),1U));});});unsigned _tmpACF=0U;_tmpACC(_tmpACD,_tmpACF);});
# 4755
struct Cyc_Absyn_Exp*zero_exp=({Cyc_Absyn_int_exp(Cyc_Absyn_Signed,0,0U);});
struct Cyc_Absyn_Exp*one_exp=({Cyc_Absyn_int_exp(Cyc_Absyn_Signed,1,0U);});
({void*_tmpF59=({struct Cyc_Absyn_Stmt*(*_tmpAA5)(struct _tuple1*,void*,struct Cyc_Absyn_Exp*init,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_declare_stmt;struct _tuple1*_tmpAA6=h_var;void*_tmpAA7=h_typ;struct Cyc_Absyn_Exp*_tmpAA8=0;struct Cyc_Absyn_Stmt*_tmpAA9=({struct Cyc_Absyn_Stmt*(*_tmpAAA)(struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_seq_stmt;struct Cyc_Absyn_Stmt*_tmpAAB=pushhandler_call;struct Cyc_Absyn_Stmt*_tmpAAC=({struct Cyc_Absyn_Stmt*(*_tmpAAD)(struct _tuple1*,void*,struct Cyc_Absyn_Exp*init,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_declare_stmt;struct _tuple1*_tmpAAE=was_thrown_var;void*_tmpAAF=was_thrown_typ;struct Cyc_Absyn_Exp*_tmpAB0=zero_exp;struct Cyc_Absyn_Stmt*_tmpAB1=({struct Cyc_Absyn_Stmt*(*_tmpAB2)(struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_seq_stmt;struct Cyc_Absyn_Stmt*_tmpAB3=({struct Cyc_Absyn_Stmt*(*_tmpAB4)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_ifthenelse_stmt;struct Cyc_Absyn_Exp*_tmpAB5=setjmp_call;struct Cyc_Absyn_Stmt*_tmpAB6=({Cyc_Absyn_assign_stmt(was_thrown_exp,one_exp,0U);});struct Cyc_Absyn_Stmt*_tmpAB7=({Cyc_Toc_skip_stmt_dl();});unsigned _tmpAB8=0U;_tmpAB4(_tmpAB5,_tmpAB6,_tmpAB7,_tmpAB8);});struct Cyc_Absyn_Stmt*_tmpAB9=({struct Cyc_Absyn_Stmt*(*_tmpABA)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_ifthenelse_stmt;struct Cyc_Absyn_Exp*_tmpABB=({Cyc_Absyn_prim1_exp(Cyc_Absyn_Not,was_thrown_exp,0U);});struct Cyc_Absyn_Stmt*_tmpABC=tryandpop_stmt;struct Cyc_Absyn_Stmt*_tmpABD=({struct Cyc_Absyn_Stmt*(*_tmpABE)(struct _tuple1*,void*,struct Cyc_Absyn_Exp*init,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_declare_stmt;struct _tuple1*_tmpABF=e_var;void*_tmpAC0=e_typ;struct Cyc_Absyn_Exp*_tmpAC1=({struct Cyc_Absyn_Exp*(*_tmpAC2)(void*t,struct Cyc_Absyn_Exp*e)=Cyc_Toc_cast_it;void*_tmpAC3=e_typ;struct Cyc_Absyn_Exp*_tmpAC4=({Cyc_Toc_get_exn_thrown_expression();});_tmpAC2(_tmpAC3,_tmpAC4);});struct Cyc_Absyn_Stmt*_tmpAC5=handler_stmt;unsigned _tmpAC6=0U;_tmpABE(_tmpABF,_tmpAC0,_tmpAC1,_tmpAC5,_tmpAC6);});unsigned _tmpAC7=0U;_tmpABA(_tmpABB,_tmpABC,_tmpABD,_tmpAC7);});unsigned _tmpAC8=0U;_tmpAB2(_tmpAB3,_tmpAB9,_tmpAC8);});unsigned _tmpAC9=0U;_tmpAAD(_tmpAAE,_tmpAAF,_tmpAB0,_tmpAB1,_tmpAC9);});unsigned _tmpACA=0U;_tmpAAA(_tmpAAB,_tmpAAC,_tmpACA);});unsigned _tmpACB=0U;_tmpAA5(_tmpAA6,_tmpAA7,_tmpAA8,_tmpAA9,_tmpACB);})->r;s->r=_tmpF59;});}}
# 4770
_npop_handler(0U);return;
# 4740
;_pop_region();}}}}}}_LL0:;}}
# 4470 "toc.cyc"
static void Cyc_Toc_stmttypes_to_c(struct Cyc_Absyn_Stmt*s);
# 4780 "toc.cyc"
static void Cyc_Toc_fndecl_to_c(struct Cyc_Toc_Env*nv,struct Cyc_Absyn_Fndecl*f,int cinclude){
(f->i).tvars=0;
(f->i).effect=0;
(f->i).rgn_po=0;
(f->i).requires_clause=0;
(f->i).ensures_clause=0;
({void*_tmpF5A=({Cyc_Toc_typ_to_c((f->i).ret_type);});(f->i).ret_type=_tmpF5A;});
({void*_tmpF5B=({Cyc_Toc_typ_to_c((void*)_check_null(f->cached_type));});f->cached_type=_tmpF5B;});{
struct _RegionHandle _tmpADD=_new_region("frgn");struct _RegionHandle*frgn=& _tmpADD;_push_region(frgn);
{struct Cyc_Toc_Env*_tmpADE=({Cyc_Toc_share_env(frgn,nv);});struct Cyc_Toc_Env*nv=_tmpADE;
{struct Cyc_List_List*args=(f->i).args;for(0;args != 0;args=args->tl){
struct _tuple1*x=({struct _tuple1*_tmpADF=_cycalloc(sizeof(*_tmpADF));_tmpADF->f1=Cyc_Absyn_Loc_n,_tmpADF->f2=(*((struct _tuple9*)args->hd)).f1;_tmpADF;});
({void*_tmpF5C=({Cyc_Toc_typ_to_c((*((struct _tuple9*)args->hd)).f3);});(*((struct _tuple9*)args->hd)).f3=_tmpF5C;});}}
# 4796
if(cinclude){
({Cyc_Toc_stmttypes_to_c(f->body);});
_npop_handler(0U);return;}
# 4796
Cyc_Toc_fn_pop_table=({struct Cyc_Hashtable_Table**_tmpAE1=_cycalloc(sizeof(*_tmpAE1));({
# 4800
struct Cyc_Hashtable_Table*_tmpF5F=({({struct Cyc_Hashtable_Table*(*_tmpF5E)(struct Cyc_Hashtable_Table*t,struct Cyc_Absyn_Fndecl*key)=({struct Cyc_Hashtable_Table*(*_tmpAE0)(struct Cyc_Hashtable_Table*t,struct Cyc_Absyn_Fndecl*key)=(struct Cyc_Hashtable_Table*(*)(struct Cyc_Hashtable_Table*t,struct Cyc_Absyn_Fndecl*key))Cyc_Hashtable_lookup;_tmpAE0;});struct Cyc_Hashtable_Table*_tmpF5D=*((struct Cyc_Hashtable_Table**)_check_null(Cyc_Toc_gpop_tables));_tmpF5E(_tmpF5D,f);});});*_tmpAE1=_tmpF5F;});_tmpAE1;});
if((unsigned)(f->i).cyc_varargs &&((struct Cyc_Absyn_VarargInfo*)_check_null((f->i).cyc_varargs))->name != 0){
struct Cyc_Absyn_VarargInfo _stmttmp7A=*((struct Cyc_Absyn_VarargInfo*)_check_null((f->i).cyc_varargs));int _tmpAE5;void*_tmpAE4;struct Cyc_Absyn_Tqual _tmpAE3;struct _fat_ptr*_tmpAE2;_LL1: _tmpAE2=_stmttmp7A.name;_tmpAE3=_stmttmp7A.tq;_tmpAE4=_stmttmp7A.type;_tmpAE5=_stmttmp7A.inject;_LL2: {struct _fat_ptr*n=_tmpAE2;struct Cyc_Absyn_Tqual tq=_tmpAE3;void*t=_tmpAE4;int i=_tmpAE5;
void*t2=({void*(*_tmpAE9)(void*t)=Cyc_Toc_typ_to_c;void*_tmpAEA=({Cyc_Absyn_fatptr_type(t,Cyc_Absyn_heap_rgn_type,tq,Cyc_Absyn_false_type);});_tmpAE9(_tmpAEA);});
struct _tuple1*x2=({struct _tuple1*_tmpAE8=_cycalloc(sizeof(*_tmpAE8));_tmpAE8->f1=Cyc_Absyn_Loc_n,_tmpAE8->f2=(struct _fat_ptr*)_check_null(n);_tmpAE8;});
({struct Cyc_List_List*_tmpF62=({({struct Cyc_List_List*_tmpF61=(f->i).args;Cyc_List_append(_tmpF61,({struct Cyc_List_List*_tmpAE7=_cycalloc(sizeof(*_tmpAE7));({struct _tuple9*_tmpF60=({struct _tuple9*_tmpAE6=_cycalloc(sizeof(*_tmpAE6));_tmpAE6->f1=n,_tmpAE6->f2=tq,_tmpAE6->f3=t2;_tmpAE6;});_tmpAE7->hd=_tmpF60;}),_tmpAE7->tl=0;_tmpAE7;}));});});(f->i).args=_tmpF62;});
(f->i).cyc_varargs=0;}}
# 4801
{
# 4809
struct Cyc_List_List*arg_vds=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(f->param_vardecls))->v;
# 4801
for(0;arg_vds != 0;arg_vds=arg_vds->tl){
# 4810
({void*_tmpF63=({Cyc_Toc_typ_to_c(((struct Cyc_Absyn_Vardecl*)arg_vds->hd)->type);});((struct Cyc_Absyn_Vardecl*)arg_vds->hd)->type=_tmpF63;});}}
({void(*_tmpAEB)(struct Cyc_Toc_Env*nv,struct Cyc_Absyn_Stmt*s)=Cyc_Toc_stmt_to_c;struct Cyc_Toc_Env*_tmpAEC=({Cyc_Toc_clear_toplevel(frgn,nv);});struct Cyc_Absyn_Stmt*_tmpAED=f->body;_tmpAEB(_tmpAEC,_tmpAED);});}
# 4789
;_pop_region();}}
# 4814
static enum Cyc_Absyn_Scope Cyc_Toc_scope_to_c(enum Cyc_Absyn_Scope s){
enum Cyc_Absyn_Scope _tmpAEF=s;switch(_tmpAEF){case Cyc_Absyn_Abstract: _LL1: _LL2:
 return Cyc_Absyn_Public;case Cyc_Absyn_ExternC: _LL3: _LL4:
 return Cyc_Absyn_Extern;default: _LL5: _LL6:
 return s;}_LL0:;}struct _tuple49{struct Cyc_Toc_TocState*f1;struct Cyc_Absyn_Aggrdecl**f2;};
# 4830 "toc.cyc"
static int Cyc_Toc_aggrdecl_to_c_body(struct _RegionHandle*d,struct _tuple49*env){
# 4832
struct _tuple49 _stmttmp7B=*env;struct Cyc_Absyn_Aggrdecl*_tmpAF2;struct Cyc_Toc_TocState*_tmpAF1;_LL1: _tmpAF1=_stmttmp7B.f1;_tmpAF2=*_stmttmp7B.f2;_LL2: {struct Cyc_Toc_TocState*s=_tmpAF1;struct Cyc_Absyn_Aggrdecl*ad=_tmpAF2;
struct _tuple1*n=ad->name;
struct Cyc_Toc_TocState _stmttmp7C=*s;struct Cyc_Dict_Dict*_tmpAF3;_LL4: _tmpAF3=_stmttmp7C.aggrs_so_far;_LL5: {struct Cyc_Dict_Dict*aggrs_so_far=_tmpAF3;
int seen_defn_before;
struct _tuple20**dopt=({({struct _tuple20**(*_tmpF65)(struct Cyc_Dict_Dict d,struct _tuple1*k)=({struct _tuple20**(*_tmpB1B)(struct Cyc_Dict_Dict d,struct _tuple1*k)=(struct _tuple20**(*)(struct Cyc_Dict_Dict d,struct _tuple1*k))Cyc_Dict_lookup_opt;_tmpB1B;});struct Cyc_Dict_Dict _tmpF64=*aggrs_so_far;_tmpF65(_tmpF64,n);});});
if(dopt == 0){
seen_defn_before=0;{
struct _tuple20*v;
if((int)ad->kind == (int)Cyc_Absyn_StructA)
v=({struct _tuple20*_tmpAF4=_region_malloc(d,sizeof(*_tmpAF4));_tmpAF4->f1=ad,({void*_tmpF66=({Cyc_Absyn_strctq(n);});_tmpAF4->f2=_tmpF66;});_tmpAF4;});else{
# 4843
v=({struct _tuple20*_tmpAF5=_region_malloc(d,sizeof(*_tmpAF5));_tmpAF5->f1=ad,({void*_tmpF67=({Cyc_Absyn_unionq_type(n);});_tmpAF5->f2=_tmpF67;});_tmpAF5;});}
({struct Cyc_Dict_Dict _tmpF6B=({({struct Cyc_Dict_Dict(*_tmpF6A)(struct Cyc_Dict_Dict d,struct _tuple1*k,struct _tuple20*v)=({struct Cyc_Dict_Dict(*_tmpAF6)(struct Cyc_Dict_Dict d,struct _tuple1*k,struct _tuple20*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _tuple1*k,struct _tuple20*v))Cyc_Dict_insert;_tmpAF6;});struct Cyc_Dict_Dict _tmpF69=*aggrs_so_far;struct _tuple1*_tmpF68=n;_tmpF6A(_tmpF69,_tmpF68,v);});});*aggrs_so_far=_tmpF6B;});}}else{
# 4846
struct _tuple20*_stmttmp7D=*dopt;void*_tmpAF8;struct Cyc_Absyn_Aggrdecl*_tmpAF7;_LL7: _tmpAF7=_stmttmp7D->f1;_tmpAF8=_stmttmp7D->f2;_LL8: {struct Cyc_Absyn_Aggrdecl*ad2=_tmpAF7;void*t=_tmpAF8;
if(ad2->impl == 0){
({struct Cyc_Dict_Dict _tmpF6F=({({struct Cyc_Dict_Dict(*_tmpF6E)(struct Cyc_Dict_Dict d,struct _tuple1*k,struct _tuple20*v)=({struct Cyc_Dict_Dict(*_tmpAF9)(struct Cyc_Dict_Dict d,struct _tuple1*k,struct _tuple20*v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _tuple1*k,struct _tuple20*v))Cyc_Dict_insert;_tmpAF9;});struct Cyc_Dict_Dict _tmpF6D=*aggrs_so_far;struct _tuple1*_tmpF6C=n;_tmpF6E(_tmpF6D,_tmpF6C,({struct _tuple20*_tmpAFA=_region_malloc(d,sizeof(*_tmpAFA));_tmpAFA->f1=ad,_tmpAFA->f2=t;_tmpAFA;}));});});*aggrs_so_far=_tmpF6F;});
seen_defn_before=0;}else{
# 4851
seen_defn_before=1;}}}{
# 4853
struct Cyc_Absyn_Aggrdecl*new_ad=({struct Cyc_Absyn_Aggrdecl*_tmpB1A=_cycalloc(sizeof(*_tmpB1A));_tmpB1A->kind=ad->kind,_tmpB1A->sc=Cyc_Absyn_Public,_tmpB1A->name=ad->name,_tmpB1A->tvs=0,_tmpB1A->impl=0,_tmpB1A->expected_mem_kind=0,_tmpB1A->attributes=ad->attributes;_tmpB1A;});
# 4860
if(ad->impl != 0 && !seen_defn_before){
({struct Cyc_Absyn_AggrdeclImpl*_tmpF70=({struct Cyc_Absyn_AggrdeclImpl*_tmpAFB=_cycalloc(sizeof(*_tmpAFB));_tmpAFB->exist_vars=0,_tmpAFB->rgn_po=0,_tmpAFB->fields=0,_tmpAFB->tagged=0;_tmpAFB;});new_ad->impl=_tmpF70;});{
# 4865
struct Cyc_List_List*new_fields=0;
{struct Cyc_List_List*fields=((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->fields;for(0;fields != 0;fields=fields->tl){
# 4869
struct Cyc_Absyn_Aggrfield*old_field=(struct Cyc_Absyn_Aggrfield*)fields->hd;
void*old_type=old_field->type;
struct Cyc_List_List*old_atts=old_field->attributes;
if(({int(*_tmpAFC)(struct Cyc_Absyn_Kind*,struct Cyc_Absyn_Kind*)=Cyc_Tcutil_kind_leq;struct Cyc_Absyn_Kind*_tmpAFD=& Cyc_Tcutil_ak;struct Cyc_Absyn_Kind*_tmpAFE=({Cyc_Tcutil_type_kind(old_type);});_tmpAFC(_tmpAFD,_tmpAFE);})&&(
(int)ad->kind == (int)Cyc_Absyn_StructA && fields->tl == 0 ||(int)ad->kind == (int)Cyc_Absyn_UnionA)){
# 4883 "toc.cyc"
void*_stmttmp7E=({Cyc_Tcutil_compress(old_type);});void*_tmpAFF=_stmttmp7E;unsigned _tmpB03;void*_tmpB02;struct Cyc_Absyn_Tqual _tmpB01;void*_tmpB00;if(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmpAFF)->tag == 4U){_LLA: _tmpB00=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmpAFF)->f1).elt_type;_tmpB01=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmpAFF)->f1).tq;_tmpB02=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmpAFF)->f1).zero_term;_tmpB03=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmpAFF)->f1).zt_loc;_LLB: {void*et=_tmpB00;struct Cyc_Absyn_Tqual tq=_tmpB01;void*zt=_tmpB02;unsigned ztl=_tmpB03;
# 4886
old_type=(void*)({struct Cyc_Absyn_ArrayType_Absyn_Type_struct*_tmpB04=_cycalloc(sizeof(*_tmpB04));_tmpB04->tag=4U,(_tmpB04->f1).elt_type=et,(_tmpB04->f1).tq=tq,({struct Cyc_Absyn_Exp*_tmpF71=({Cyc_Absyn_uint_exp(0U,0U);});(_tmpB04->f1).num_elts=_tmpF71;}),(_tmpB04->f1).zero_term=zt,(_tmpB04->f1).zt_loc=ztl;_tmpB04;});
goto _LL9;}}else{_LLC: _LLD:
# 4889
 old_atts=({struct Cyc_List_List*_tmpB06=_cycalloc(sizeof(*_tmpB06));({void*_tmpF72=(void*)({struct Cyc_Absyn_Aligned_att_Absyn_Attribute_struct*_tmpB05=_cycalloc(sizeof(*_tmpB05));_tmpB05->tag=6U,_tmpB05->f1=0;_tmpB05;});_tmpB06->hd=_tmpF72;}),_tmpB06->tl=old_atts;_tmpB06;});
old_type=(void*)({struct Cyc_Absyn_ArrayType_Absyn_Type_struct*_tmpB07=_cycalloc(sizeof(*_tmpB07));_tmpB07->tag=4U,({void*_tmpF75=({Cyc_Toc_void_star_type();});(_tmpB07->f1).elt_type=_tmpF75;}),({
struct Cyc_Absyn_Tqual _tmpF74=({Cyc_Absyn_empty_tqual(0U);});(_tmpB07->f1).tq=_tmpF74;}),({
struct Cyc_Absyn_Exp*_tmpF73=({Cyc_Absyn_uint_exp(0U,0U);});(_tmpB07->f1).num_elts=_tmpF73;}),(_tmpB07->f1).zero_term=Cyc_Absyn_false_type,(_tmpB07->f1).zt_loc=0U;_tmpB07;});}_LL9:;}{
# 4872 "toc.cyc"
struct Cyc_Absyn_Aggrfield*new_field=({struct Cyc_Absyn_Aggrfield*_tmpB16=_cycalloc(sizeof(*_tmpB16));
# 4896 "toc.cyc"
_tmpB16->name=old_field->name,_tmpB16->tq=Cyc_Toc_mt_tq,({
# 4898
void*_tmpF76=({Cyc_Toc_typ_to_c(old_type);});_tmpB16->type=_tmpF76;}),_tmpB16->width=old_field->width,_tmpB16->attributes=old_atts,_tmpB16->requires_clause=0;_tmpB16;});
# 4906
if(((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->tagged){
void*T=new_field->type;
struct _fat_ptr*f=new_field->name;
struct _fat_ptr s=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmpB13=({struct Cyc_String_pa_PrintArg_struct _tmpC9D;_tmpC9D.tag=0U,_tmpC9D.f1=(struct _fat_ptr)((struct _fat_ptr)*(*ad->name).f2);_tmpC9D;});struct Cyc_String_pa_PrintArg_struct _tmpB14=({struct Cyc_String_pa_PrintArg_struct _tmpC9C;_tmpC9C.tag=0U,_tmpC9C.f1=(struct _fat_ptr)((struct _fat_ptr)*f);_tmpC9C;});void*_tmpB11[2U];_tmpB11[0]=& _tmpB13,_tmpB11[1]=& _tmpB14;({struct _fat_ptr _tmpF77=({const char*_tmpB12="_union_%s_%s";_tag_fat(_tmpB12,sizeof(char),13U);});Cyc_aprintf(_tmpF77,_tag_fat(_tmpB11,sizeof(void*),2U));});});
struct _fat_ptr*str=({struct _fat_ptr*_tmpB10=_cycalloc(sizeof(*_tmpB10));*_tmpB10=s;_tmpB10;});
struct Cyc_Absyn_Aggrfield*value_field=({struct Cyc_Absyn_Aggrfield*_tmpB0F=_cycalloc(sizeof(*_tmpB0F));_tmpB0F->name=Cyc_Toc_val_sp,_tmpB0F->tq=Cyc_Toc_mt_tq,_tmpB0F->type=T,_tmpB0F->width=0,_tmpB0F->attributes=0,_tmpB0F->requires_clause=0;_tmpB0F;});
struct Cyc_Absyn_Aggrfield*tag_field=({struct Cyc_Absyn_Aggrfield*_tmpB0E=_cycalloc(sizeof(*_tmpB0E));_tmpB0E->name=Cyc_Toc_tag_sp,_tmpB0E->tq=Cyc_Toc_mt_tq,_tmpB0E->type=Cyc_Absyn_sint_type,_tmpB0E->width=0,_tmpB0E->attributes=0,_tmpB0E->requires_clause=0;_tmpB0E;});
struct Cyc_Absyn_Aggrdecl*ad2=({struct Cyc_Absyn_Aggrdecl*(*_tmpB0A)(struct _fat_ptr*name,struct Cyc_List_List*fs)=Cyc_Toc_make_c_struct_defn;struct _fat_ptr*_tmpB0B=str;struct Cyc_List_List*_tmpB0C=({struct Cyc_Absyn_Aggrfield*_tmpB0D[2U];_tmpB0D[0]=tag_field,_tmpB0D[1]=value_field;Cyc_List_list(_tag_fat(_tmpB0D,sizeof(struct Cyc_Absyn_Aggrfield*),2U));});_tmpB0A(_tmpB0B,_tmpB0C);});
Cyc_Toc_result_decls=({struct Cyc_List_List*_tmpB09=_cycalloc(sizeof(*_tmpB09));({struct Cyc_Absyn_Decl*_tmpF78=({Cyc_Absyn_new_decl((void*)({struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct*_tmpB08=_cycalloc(sizeof(*_tmpB08));_tmpB08->tag=5U,_tmpB08->f1=ad2;_tmpB08;}),0U);});_tmpB09->hd=_tmpF78;}),_tmpB09->tl=Cyc_Toc_result_decls;_tmpB09;});
({void*_tmpF79=({Cyc_Absyn_strct(str);});new_field->type=_tmpF79;});}
# 4906
new_fields=({struct Cyc_List_List*_tmpB15=_cycalloc(sizeof(*_tmpB15));
# 4917
_tmpB15->hd=new_field,_tmpB15->tl=new_fields;_tmpB15;});}}}
# 4919
({struct Cyc_List_List*_tmpF7A=({Cyc_List_imp_rev(new_fields);});(new_ad->impl)->fields=_tmpF7A;});}}
# 4860 "toc.cyc"
if(!seen_defn_before)
# 4923 "toc.cyc"
Cyc_Toc_result_decls=({struct Cyc_List_List*_tmpB19=_cycalloc(sizeof(*_tmpB19));({struct Cyc_Absyn_Decl*_tmpF7C=({struct Cyc_Absyn_Decl*_tmpB18=_cycalloc(sizeof(*_tmpB18));({void*_tmpF7B=(void*)({struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct*_tmpB17=_cycalloc(sizeof(*_tmpB17));_tmpB17->tag=5U,_tmpB17->f1=new_ad;_tmpB17;});_tmpB18->r=_tmpF7B;}),_tmpB18->loc=0U;_tmpB18;});_tmpB19->hd=_tmpF7C;}),_tmpB19->tl=Cyc_Toc_result_decls;_tmpB19;});
# 4860 "toc.cyc"
return 0;}}}}
# 4926 "toc.cyc"
static void Cyc_Toc_aggrdecl_to_c(struct Cyc_Absyn_Aggrdecl*ad){
({({int(*_tmpB1D)(struct Cyc_Absyn_Aggrdecl**arg,int(*f)(struct _RegionHandle*,struct _tuple49*))=(int(*)(struct Cyc_Absyn_Aggrdecl**arg,int(*f)(struct _RegionHandle*,struct _tuple49*)))Cyc_Toc_use_toc_state;_tmpB1D;})(& ad,Cyc_Toc_aggrdecl_to_c_body);});}struct _tuple50{struct Cyc_Toc_TocState*f1;struct Cyc_Absyn_Datatypedecl*f2;};
# 4954 "toc.cyc"
static int Cyc_Toc_datatypedecl_to_c_body(struct _RegionHandle*d,struct _tuple50*env){
# 4957
struct _tuple50 _stmttmp7F=*env;struct Cyc_Absyn_Datatypedecl*_tmpB20;struct Cyc_Set_Set**_tmpB1F;_LL1: _tmpB1F=(_stmttmp7F.f1)->datatypes_so_far;_tmpB20=_stmttmp7F.f2;_LL2: {struct Cyc_Set_Set**datatypes_so_far=_tmpB1F;struct Cyc_Absyn_Datatypedecl*tud=_tmpB20;
struct _tuple1*n=tud->name;
if(tud->fields == 0 ||({({int(*_tmpF7E)(struct Cyc_Set_Set*s,struct _tuple1*elt)=({int(*_tmpB21)(struct Cyc_Set_Set*s,struct _tuple1*elt)=(int(*)(struct Cyc_Set_Set*s,struct _tuple1*elt))Cyc_Set_member;_tmpB21;});struct Cyc_Set_Set*_tmpF7D=*datatypes_so_far;_tmpF7E(_tmpF7D,n);});}))
return 0;
# 4959
({struct Cyc_Set_Set*_tmpF82=({({struct Cyc_Set_Set*(*_tmpF81)(struct _RegionHandle*r,struct Cyc_Set_Set*s,struct _tuple1*elt)=({
# 4961
struct Cyc_Set_Set*(*_tmpB22)(struct _RegionHandle*r,struct Cyc_Set_Set*s,struct _tuple1*elt)=(struct Cyc_Set_Set*(*)(struct _RegionHandle*r,struct Cyc_Set_Set*s,struct _tuple1*elt))Cyc_Set_rinsert;_tmpB22;});struct _RegionHandle*_tmpF80=d;struct Cyc_Set_Set*_tmpF7F=*datatypes_so_far;_tmpF81(_tmpF80,_tmpF7F,n);});});
# 4959
*datatypes_so_far=_tmpF82;});
# 4962
{struct Cyc_List_List*fields=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(tud->fields))->v;for(0;fields != 0;fields=fields->tl){
struct Cyc_Absyn_Datatypefield*f=(struct Cyc_Absyn_Datatypefield*)fields->hd;
# 4965
struct Cyc_List_List*fs=0;
int i=1;
{struct Cyc_List_List*ts=f->typs;for(0;ts != 0;(ts=ts->tl,i ++)){
struct _fat_ptr*fname=({Cyc_Absyn_fieldname(i);});
struct Cyc_Absyn_Aggrfield*f=({struct Cyc_Absyn_Aggrfield*_tmpB24=_cycalloc(sizeof(*_tmpB24));_tmpB24->name=fname,_tmpB24->tq=(*((struct _tuple14*)ts->hd)).f1,({
void*_tmpF83=({Cyc_Toc_typ_to_c((*((struct _tuple14*)ts->hd)).f2);});_tmpB24->type=_tmpF83;}),_tmpB24->width=0,_tmpB24->attributes=0,_tmpB24->requires_clause=0;_tmpB24;});
fs=({struct Cyc_List_List*_tmpB23=_cycalloc(sizeof(*_tmpB23));_tmpB23->hd=f,_tmpB23->tl=fs;_tmpB23;});}}
# 4973
fs=({struct Cyc_List_List*_tmpB26=_cycalloc(sizeof(*_tmpB26));({struct Cyc_Absyn_Aggrfield*_tmpF85=({struct Cyc_Absyn_Aggrfield*_tmpB25=_cycalloc(sizeof(*_tmpB25));_tmpB25->name=Cyc_Toc_tag_sp,_tmpB25->tq=Cyc_Toc_mt_tq,_tmpB25->type=Cyc_Absyn_sint_type,_tmpB25->width=0,_tmpB25->attributes=0,_tmpB25->requires_clause=0;_tmpB25;});_tmpB26->hd=_tmpF85;}),({
struct Cyc_List_List*_tmpF84=({Cyc_List_imp_rev(fs);});_tmpB26->tl=_tmpF84;});_tmpB26;});{
struct Cyc_Absyn_Aggrdecl*ad=({({struct _fat_ptr*_tmpF87=({struct _fat_ptr*_tmpB2A=_cycalloc(sizeof(*_tmpB2A));({struct _fat_ptr _tmpF86=({const char*_tmpB29="";_tag_fat(_tmpB29,sizeof(char),1U);});*_tmpB2A=_tmpF86;});_tmpB2A;});Cyc_Toc_make_c_struct_defn(_tmpF87,fs);});});
({struct _tuple1*_tmpF88=({Cyc_Toc_collapse_qvars(f->name,tud->name);});ad->name=_tmpF88;});
Cyc_Toc_result_decls=({struct Cyc_List_List*_tmpB28=_cycalloc(sizeof(*_tmpB28));({struct Cyc_Absyn_Decl*_tmpF89=({Cyc_Absyn_new_decl((void*)({struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct*_tmpB27=_cycalloc(sizeof(*_tmpB27));_tmpB27->tag=5U,_tmpB27->f1=ad;_tmpB27;}),0U);});_tmpB28->hd=_tmpF89;}),_tmpB28->tl=Cyc_Toc_result_decls;_tmpB28;});}}}
# 4979
return 0;}}
# 4982
static void Cyc_Toc_datatypedecl_to_c(struct Cyc_Absyn_Datatypedecl*tud){
({({int(*_tmpF8A)(struct Cyc_Absyn_Datatypedecl*arg,int(*f)(struct _RegionHandle*,struct _tuple50*))=({int(*_tmpB2C)(struct Cyc_Absyn_Datatypedecl*arg,int(*f)(struct _RegionHandle*,struct _tuple50*))=(int(*)(struct Cyc_Absyn_Datatypedecl*arg,int(*f)(struct _RegionHandle*,struct _tuple50*)))Cyc_Toc_use_toc_state;_tmpB2C;});_tmpF8A(tud,Cyc_Toc_datatypedecl_to_c_body);});});}
# 5001 "toc.cyc"
static int Cyc_Toc_xdatatypedecl_to_c_body(struct _RegionHandle*d,struct _tuple50*env){
# 5004
struct _tuple50 _stmttmp80=*env;struct Cyc_Absyn_Datatypedecl*_tmpB2F;struct Cyc_Toc_TocState*_tmpB2E;_LL1: _tmpB2E=_stmttmp80.f1;_tmpB2F=_stmttmp80.f2;_LL2: {struct Cyc_Toc_TocState*s=_tmpB2E;struct Cyc_Absyn_Datatypedecl*xd=_tmpB2F;
if(xd->fields == 0)
return 0;{
# 5005
struct Cyc_Toc_TocState _stmttmp81=*s;struct Cyc_Dict_Dict*_tmpB30;_LL4: _tmpB30=_stmttmp81.xdatatypes_so_far;_LL5: {struct Cyc_Dict_Dict*xdatatypes_so_far=_tmpB30;
# 5008
struct _tuple1*n=xd->name;
{struct Cyc_List_List*fs=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(xd->fields))->v;for(0;fs != 0;fs=fs->tl){
struct Cyc_Absyn_Datatypefield*f=(struct Cyc_Absyn_Datatypefield*)fs->hd;
struct _fat_ptr*fn=(*f->name).f2;
struct Cyc_Absyn_Exp*sz_exp=({Cyc_Absyn_uint_exp(_get_fat_size(*fn,sizeof(char)),0U);});
void*tag_typ=({Cyc_Absyn_array_type(Cyc_Absyn_char_type,Cyc_Toc_mt_tq,sz_exp,Cyc_Absyn_false_type,0U);});
# 5015
int*_stmttmp82=({({int*(*_tmpF8C)(struct Cyc_Dict_Dict d,struct _tuple1*k)=({int*(*_tmpB44)(struct Cyc_Dict_Dict d,struct _tuple1*k)=(int*(*)(struct Cyc_Dict_Dict d,struct _tuple1*k))Cyc_Dict_lookup_opt;_tmpB44;});struct Cyc_Dict_Dict _tmpF8B=*xdatatypes_so_far;_tmpF8C(_tmpF8B,f->name);});});int*_tmpB31=_stmttmp82;if(_tmpB31 == 0){_LL7: _LL8: {
# 5017
struct Cyc_Absyn_Exp*initopt=0;
if((int)f->sc != (int)Cyc_Absyn_Extern)
initopt=({Cyc_Absyn_string_exp(*fn,0U);});{
# 5018
struct Cyc_Absyn_Vardecl*tag_decl=({Cyc_Absyn_new_vardecl(0U,f->name,tag_typ,initopt);});
# 5022
tag_decl->sc=f->sc;
Cyc_Toc_result_decls=({struct Cyc_List_List*_tmpB33=_cycalloc(sizeof(*_tmpB33));({struct Cyc_Absyn_Decl*_tmpF8D=({Cyc_Absyn_new_decl((void*)({struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*_tmpB32=_cycalloc(sizeof(*_tmpB32));_tmpB32->tag=0U,_tmpB32->f1=tag_decl;_tmpB32;}),0U);});_tmpB33->hd=_tmpF8D;}),_tmpB33->tl=Cyc_Toc_result_decls;_tmpB33;});
({struct Cyc_Dict_Dict _tmpF91=({({struct Cyc_Dict_Dict(*_tmpF90)(struct Cyc_Dict_Dict d,struct _tuple1*k,int v)=({
struct Cyc_Dict_Dict(*_tmpB34)(struct Cyc_Dict_Dict d,struct _tuple1*k,int v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _tuple1*k,int v))Cyc_Dict_insert;_tmpB34;});struct Cyc_Dict_Dict _tmpF8F=*xdatatypes_so_far;struct _tuple1*_tmpF8E=f->name;_tmpF90(_tmpF8F,_tmpF8E,(int)f->sc != (int)Cyc_Absyn_Extern);});});
# 5024
*xdatatypes_so_far=_tmpF91;});{
# 5026
struct Cyc_List_List*fields=0;
int i=1;
{struct Cyc_List_List*tqts=f->typs;for(0;tqts != 0;(tqts=tqts->tl,i ++)){
struct _fat_ptr*field_n=({struct _fat_ptr*_tmpB3A=_cycalloc(sizeof(*_tmpB3A));({struct _fat_ptr _tmpF93=(struct _fat_ptr)({struct Cyc_Int_pa_PrintArg_struct _tmpB39=({struct Cyc_Int_pa_PrintArg_struct _tmpC9E;_tmpC9E.tag=1U,_tmpC9E.f1=(unsigned long)i;_tmpC9E;});void*_tmpB37[1U];_tmpB37[0]=& _tmpB39;({struct _fat_ptr _tmpF92=({const char*_tmpB38="f%d";_tag_fat(_tmpB38,sizeof(char),4U);});Cyc_aprintf(_tmpF92,_tag_fat(_tmpB37,sizeof(void*),1U));});});*_tmpB3A=_tmpF93;});_tmpB3A;});
struct Cyc_Absyn_Aggrfield*newf=({struct Cyc_Absyn_Aggrfield*_tmpB36=_cycalloc(sizeof(*_tmpB36));_tmpB36->name=field_n,_tmpB36->tq=(*((struct _tuple14*)tqts->hd)).f1,({
void*_tmpF94=({Cyc_Toc_typ_to_c((*((struct _tuple14*)tqts->hd)).f2);});_tmpB36->type=_tmpF94;}),_tmpB36->width=0,_tmpB36->attributes=0,_tmpB36->requires_clause=0;_tmpB36;});
fields=({struct Cyc_List_List*_tmpB35=_cycalloc(sizeof(*_tmpB35));_tmpB35->hd=newf,_tmpB35->tl=fields;_tmpB35;});}}
# 5034
fields=({struct Cyc_List_List*_tmpB3C=_cycalloc(sizeof(*_tmpB3C));({struct Cyc_Absyn_Aggrfield*_tmpF97=({struct Cyc_Absyn_Aggrfield*_tmpB3B=_cycalloc(sizeof(*_tmpB3B));_tmpB3B->name=Cyc_Toc_tag_sp,_tmpB3B->tq=Cyc_Toc_mt_tq,({
void*_tmpF96=({Cyc_Absyn_cstar_type(Cyc_Absyn_char_type,Cyc_Toc_mt_tq);});_tmpB3B->type=_tmpF96;}),_tmpB3B->width=0,_tmpB3B->attributes=0,_tmpB3B->requires_clause=0;_tmpB3B;});
# 5034
_tmpB3C->hd=_tmpF97;}),({
# 5036
struct Cyc_List_List*_tmpF95=({Cyc_List_imp_rev(fields);});_tmpB3C->tl=_tmpF95;});_tmpB3C;});{
struct Cyc_Absyn_Aggrdecl*strct_decl=({({struct _fat_ptr*_tmpF99=({struct _fat_ptr*_tmpB40=_cycalloc(sizeof(*_tmpB40));({struct _fat_ptr _tmpF98=({const char*_tmpB3F="";_tag_fat(_tmpB3F,sizeof(char),1U);});*_tmpB40=_tmpF98;});_tmpB40;});Cyc_Toc_make_c_struct_defn(_tmpF99,fields);});});
({struct _tuple1*_tmpF9A=({Cyc_Toc_collapse_qvars(f->name,xd->name);});strct_decl->name=_tmpF9A;});
Cyc_Toc_result_decls=({struct Cyc_List_List*_tmpB3E=_cycalloc(sizeof(*_tmpB3E));({struct Cyc_Absyn_Decl*_tmpF9B=({Cyc_Absyn_new_decl((void*)({struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct*_tmpB3D=_cycalloc(sizeof(*_tmpB3D));_tmpB3D->tag=5U,_tmpB3D->f1=strct_decl;_tmpB3D;}),0U);});_tmpB3E->hd=_tmpF9B;}),_tmpB3E->tl=Cyc_Toc_result_decls;_tmpB3E;});
goto _LL6;}}}}}else{if(*((int*)_tmpB31)== 0){_LL9: _LLA:
# 5042
 if((int)f->sc != (int)Cyc_Absyn_Extern){
struct Cyc_Absyn_Exp*initopt=({Cyc_Absyn_string_exp(*fn,0U);});
struct Cyc_Absyn_Vardecl*tag_decl=({Cyc_Absyn_new_vardecl(0U,f->name,tag_typ,initopt);});
tag_decl->sc=f->sc;
Cyc_Toc_result_decls=({struct Cyc_List_List*_tmpB42=_cycalloc(sizeof(*_tmpB42));({struct Cyc_Absyn_Decl*_tmpF9C=({Cyc_Absyn_new_decl((void*)({struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*_tmpB41=_cycalloc(sizeof(*_tmpB41));_tmpB41->tag=0U,_tmpB41->f1=tag_decl;_tmpB41;}),0U);});_tmpB42->hd=_tmpF9C;}),_tmpB42->tl=Cyc_Toc_result_decls;_tmpB42;});
({struct Cyc_Dict_Dict _tmpF9F=({({struct Cyc_Dict_Dict(*_tmpF9E)(struct Cyc_Dict_Dict d,struct _tuple1*k,int v)=({struct Cyc_Dict_Dict(*_tmpB43)(struct Cyc_Dict_Dict d,struct _tuple1*k,int v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _tuple1*k,int v))Cyc_Dict_insert;_tmpB43;});struct Cyc_Dict_Dict _tmpF9D=*xdatatypes_so_far;_tmpF9E(_tmpF9D,f->name,1);});});*xdatatypes_so_far=_tmpF9F;});}
# 5042
goto _LL6;}else{_LLB: _LLC:
# 5050
 goto _LL6;}}_LL6:;}}
# 5053
return 0;}}}}
# 5056
static void Cyc_Toc_xdatatypedecl_to_c(struct Cyc_Absyn_Datatypedecl*xd){
({({int(*_tmpFA0)(struct Cyc_Absyn_Datatypedecl*arg,int(*f)(struct _RegionHandle*,struct _tuple50*))=({int(*_tmpB46)(struct Cyc_Absyn_Datatypedecl*arg,int(*f)(struct _RegionHandle*,struct _tuple50*))=(int(*)(struct Cyc_Absyn_Datatypedecl*arg,int(*f)(struct _RegionHandle*,struct _tuple50*)))Cyc_Toc_use_toc_state;_tmpB46;});_tmpFA0(xd,Cyc_Toc_xdatatypedecl_to_c_body);});});}
# 5060
static void Cyc_Toc_enumdecl_to_c(struct Cyc_Absyn_Enumdecl*ed){
ed->sc=Cyc_Absyn_Public;
if(ed->fields != 0)
({Cyc_Toc_enumfields_to_c((struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(ed->fields))->v);});}
# 5066
static void Cyc_Toc_local_decl_to_c(struct Cyc_Toc_Env*nv,struct Cyc_Absyn_Vardecl*vd,struct Cyc_Absyn_Stmt*s){
void*old_typ=vd->type;
({void*_tmpFA1=({Cyc_Toc_typ_to_c(old_typ);});vd->type=_tmpFA1;});
# 5070
if((int)vd->sc == (int)Cyc_Absyn_Register &&({Cyc_Tcutil_is_fat_pointer_type(old_typ);}))
vd->sc=Cyc_Absyn_Public;
# 5070
({Cyc_Toc_stmt_to_c(nv,s);});
# 5073
if(vd->initializer != 0){
struct Cyc_Absyn_Exp*init=(struct Cyc_Absyn_Exp*)_check_null(vd->initializer);
if((int)vd->sc == (int)Cyc_Absyn_Static){
# 5079
struct _RegionHandle _tmpB49=_new_region("temp");struct _RegionHandle*temp=& _tmpB49;_push_region(temp);
{struct Cyc_Toc_Env*nv2=({Cyc_Toc_set_toplevel(temp,nv);});
({Cyc_Toc_exp_to_c(nv2,init);});}
# 5080
;_pop_region();}else{
# 5084
({Cyc_Toc_exp_to_c(nv,init);});}}else{
# 5087
void*_stmttmp83=({Cyc_Tcutil_compress(old_typ);});void*_tmpB4A=_stmttmp83;void*_tmpB4D;struct Cyc_Absyn_Exp*_tmpB4C;void*_tmpB4B;if(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmpB4A)->tag == 4U){_LL1: _tmpB4B=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmpB4A)->f1).elt_type;_tmpB4C=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmpB4A)->f1).num_elts;_tmpB4D=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmpB4A)->f1).zero_term;_LL2: {void*et=_tmpB4B;struct Cyc_Absyn_Exp*num_elts_opt=_tmpB4C;void*zt=_tmpB4D;
# 5089
if(({Cyc_Tcutil_force_type2bool(0,zt);})){
if(num_elts_opt == 0)
({void*_tmpB4E=0U;({int(*_tmpFA3)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmpB50)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmpB50;});struct _fat_ptr _tmpFA2=({const char*_tmpB4F="can't initialize zero-terminated array -- size unknown";_tag_fat(_tmpB4F,sizeof(char),55U);});_tmpFA3(_tmpFA2,_tag_fat(_tmpB4E,sizeof(void*),0U));});});{
# 5090
struct Cyc_Absyn_Exp*num_elts=num_elts_opt;
# 5093
struct Cyc_Absyn_Exp*lhs=({struct Cyc_Absyn_Exp*(*_tmpB57)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_subscript_exp;struct Cyc_Absyn_Exp*_tmpB58=({Cyc_Absyn_var_exp(vd->name,0U);});struct Cyc_Absyn_Exp*_tmpB59=({struct Cyc_Absyn_Exp*(*_tmpB5A)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_add_exp;struct Cyc_Absyn_Exp*_tmpB5B=num_elts;struct Cyc_Absyn_Exp*_tmpB5C=({Cyc_Absyn_signed_int_exp(- 1,0U);});unsigned _tmpB5D=0U;_tmpB5A(_tmpB5B,_tmpB5C,_tmpB5D);});unsigned _tmpB5E=0U;_tmpB57(_tmpB58,_tmpB59,_tmpB5E);});
# 5096
struct Cyc_Absyn_Exp*rhs=({Cyc_Absyn_signed_int_exp(0,0U);});
({void*_tmpFA4=({void*(*_tmpB51)(struct Cyc_Absyn_Stmt*s1,struct Cyc_Absyn_Stmt*s2)=Cyc_Toc_seq_stmt_r;struct Cyc_Absyn_Stmt*_tmpB52=({struct Cyc_Absyn_Stmt*(*_tmpB53)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_exp_stmt;struct Cyc_Absyn_Exp*_tmpB54=({Cyc_Absyn_assign_exp(lhs,rhs,0U);});unsigned _tmpB55=0U;_tmpB53(_tmpB54,_tmpB55);});struct Cyc_Absyn_Stmt*_tmpB56=({Cyc_Absyn_new_stmt(s->r,0U);});_tmpB51(_tmpB52,_tmpB56);});s->r=_tmpFA4;});}}
# 5089
goto _LL0;}}else{_LL3: _LL4:
# 5101
 goto _LL0;}_LL0:;}}
# 5108
static void*Cyc_Toc_rewrite_decision(void*d,struct Cyc_Absyn_Stmt*success){
void*_tmpB60=d;void**_tmpB63;struct Cyc_List_List*_tmpB62;struct Cyc_List_List*_tmpB61;struct Cyc_Tcpat_Rhs*_tmpB64;switch(*((int*)_tmpB60)){case 0U: _LL1: _LL2:
 return d;case 1U: _LL3: _tmpB64=((struct Cyc_Tcpat_Success_Tcpat_Decision_struct*)_tmpB60)->f1;_LL4: {struct Cyc_Tcpat_Rhs*rhs=_tmpB64;
rhs->rhs=success;return d;}default: _LL5: _tmpB61=((struct Cyc_Tcpat_SwitchDec_Tcpat_Decision_struct*)_tmpB60)->f1;_tmpB62=((struct Cyc_Tcpat_SwitchDec_Tcpat_Decision_struct*)_tmpB60)->f2;_tmpB63=(void**)&((struct Cyc_Tcpat_SwitchDec_Tcpat_Decision_struct*)_tmpB60)->f3;_LL6: {struct Cyc_List_List*path=_tmpB61;struct Cyc_List_List*sws=_tmpB62;void**d2=_tmpB63;
# 5113
({void*_tmpFA5=({Cyc_Toc_rewrite_decision(*d2,success);});*d2=_tmpFA5;});
for(0;sws != 0;sws=sws->tl){
struct _tuple41*_stmttmp84=(struct _tuple41*)sws->hd;void**_tmpB65;_LL8: _tmpB65=(void**)& _stmttmp84->f2;_LL9: {void**d2=_tmpB65;
({void*_tmpFA6=({Cyc_Toc_rewrite_decision(*d2,success);});*d2=_tmpFA6;});}}
# 5118
return d;}}_LL0:;}
# 5129 "toc.cyc"
static struct Cyc_Absyn_Stmt*Cyc_Toc_letdecl_to_c(struct Cyc_Toc_Env*nv,struct Cyc_Absyn_Pat*p,void*dopt,void*t,struct Cyc_Absyn_Exp*e,struct Cyc_Absyn_Stmt*s){
# 5131
struct _RegionHandle _tmpB67=_new_region("rgn");struct _RegionHandle*rgn=& _tmpB67;_push_region(rgn);
{struct Cyc_Toc_Env*_tmpB68=({Cyc_Toc_share_env(rgn,nv);});struct Cyc_Toc_Env*nv=_tmpB68;
void*t=(void*)_check_null(e->topt);
({Cyc_Toc_exp_to_c(nv,e);});{
struct _tuple1*v;
int already_var;
{void*_stmttmp85=e->r;void*_tmpB69=_stmttmp85;void*_tmpB6A;if(((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmpB69)->tag == 1U){_LL1: _tmpB6A=(void*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmpB69)->f1;_LL2: {void*b=_tmpB6A;
already_var=1;v=({Cyc_Absyn_binding2qvar(b);});goto _LL0;}}else{_LL3: _LL4:
 already_var=0;v=({Cyc_Toc_temp_var();});goto _LL0;}_LL0:;}{
# 5142
struct Cyc_Absyn_Exp*path=({Cyc_Absyn_var_exp(v,0U);});
struct _fat_ptr*end_l=({Cyc_Toc_fresh_label();});
# 5147
struct Cyc_Absyn_Stmt*succ_stmt=s;
if(dopt != 0)
# 5151
dopt=({Cyc_Toc_rewrite_decision(dopt,succ_stmt);});{
# 5148
struct Cyc_Absyn_Switch_clause*c1=({struct Cyc_Absyn_Switch_clause*_tmpB84=_cycalloc(sizeof(*_tmpB84));
# 5153
_tmpB84->pattern=p,_tmpB84->pat_vars=0,_tmpB84->where_clause=0,_tmpB84->body=succ_stmt,_tmpB84->loc=0U;_tmpB84;});
struct Cyc_List_List*lscs=({struct Cyc_List_List*(*_tmpB7D)(struct _RegionHandle*,struct _tuple44*(*f)(struct _RegionHandle*,struct Cyc_Absyn_Switch_clause*),struct _RegionHandle*env,struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmpB7E)(struct _RegionHandle*,struct _tuple44*(*f)(struct _RegionHandle*,struct Cyc_Absyn_Switch_clause*),struct _RegionHandle*env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct _RegionHandle*,struct _tuple44*(*f)(struct _RegionHandle*,struct Cyc_Absyn_Switch_clause*),struct _RegionHandle*env,struct Cyc_List_List*x))Cyc_List_rmap_c;_tmpB7E;});struct _RegionHandle*_tmpB7F=rgn;struct _tuple44*(*_tmpB80)(struct _RegionHandle*r,struct Cyc_Absyn_Switch_clause*sc)=Cyc_Toc_gen_labels;struct _RegionHandle*_tmpB81=rgn;struct Cyc_List_List*_tmpB82=({struct Cyc_Absyn_Switch_clause*_tmpB83[1U];_tmpB83[0]=c1;({struct _RegionHandle*_tmpFA7=rgn;Cyc_List_rlist(_tmpFA7,_tag_fat(_tmpB83,sizeof(struct Cyc_Absyn_Switch_clause*),1U));});});_tmpB7D(_tmpB7F,_tmpB80,_tmpB81,_tmpB82);});
# 5158
struct Cyc_List_List*mydecls=0;
struct Cyc_List_List*mybodies=0;
struct Cyc_Absyn_Stmt*test_tree=({Cyc_Toc_compile_decision_tree(rgn,nv,& mydecls,& mybodies,dopt,lscs,v);});
# 5164
for(0;1;mybodies=((struct Cyc_List_List*)_check_null(mybodies))->tl){
if(mybodies == 0)
({void*_tmpB6B=0U;({int(*_tmpFA9)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmpB6D)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmpB6D;});struct _fat_ptr _tmpFA8=({const char*_tmpB6C="letdecl_to_c: couldn't find env!";_tag_fat(_tmpB6C,sizeof(char),33U);});_tmpFA9(_tmpFA8,_tag_fat(_tmpB6B,sizeof(void*),0U));});});{
# 5165
struct _tuple48*_stmttmp86=(struct _tuple48*)((struct Cyc_List_List*)_check_null(mybodies))->hd;struct Cyc_Absyn_Stmt*_tmpB6F;struct Cyc_Toc_Env*_tmpB6E;_LL6: _tmpB6E=_stmttmp86->f1;_tmpB6F=_stmttmp86->f3;_LL7: {struct Cyc_Toc_Env*env=_tmpB6E;struct Cyc_Absyn_Stmt*st=_tmpB6F;
# 5168
if(st == succ_stmt){({Cyc_Toc_stmt_to_c(env,s);});break;}}}}{
# 5171
struct Cyc_Absyn_Stmt*res=test_tree;
# 5173
for(0;mydecls != 0;mydecls=((struct Cyc_List_List*)_check_null(mydecls))->tl){
struct _tuple46*_stmttmp87=(struct _tuple46*)((struct Cyc_List_List*)_check_null(mydecls))->hd;struct Cyc_Absyn_Vardecl*_tmpB70;_LL9: _tmpB70=_stmttmp87->f2;_LLA: {struct Cyc_Absyn_Vardecl*vd=_tmpB70;
res=({struct Cyc_Absyn_Stmt*(*_tmpB71)(struct Cyc_Absyn_Decl*,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_decl_stmt;struct Cyc_Absyn_Decl*_tmpB72=({Cyc_Absyn_new_decl((void*)({struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*_tmpB73=_cycalloc(sizeof(*_tmpB73));_tmpB73->tag=0U,_tmpB73->f1=vd;_tmpB73;}),0U);});struct Cyc_Absyn_Stmt*_tmpB74=res;unsigned _tmpB75=0U;_tmpB71(_tmpB72,_tmpB74,_tmpB75);});}}
# 5177
if(!already_var)
res=({struct Cyc_Absyn_Stmt*(*_tmpB76)(struct _tuple1*,void*,struct Cyc_Absyn_Exp*init,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_declare_stmt;struct _tuple1*_tmpB77=v;void*_tmpB78=({Cyc_Toc_typ_to_c(t);});struct Cyc_Absyn_Exp*_tmpB79=e;struct Cyc_Absyn_Stmt*_tmpB7A=res;unsigned _tmpB7B=0U;_tmpB76(_tmpB77,_tmpB78,_tmpB79,_tmpB7A,_tmpB7B);});{
# 5177
struct Cyc_Absyn_Stmt*_tmpB7C=res;_npop_handler(0U);return _tmpB7C;}}}}}}
# 5132
;_pop_region();}
# 5214 "toc.cyc"
static void Cyc_Toc_exptypes_to_c(struct Cyc_Absyn_Exp*e){
void*_stmttmp88=e->r;void*_tmpB86=_stmttmp88;struct Cyc_Absyn_MallocInfo*_tmpB87;struct Cyc_Absyn_Stmt*_tmpB88;void**_tmpB89;void**_tmpB8A;struct Cyc_List_List*_tmpB8B;struct Cyc_List_List*_tmpB8C;struct Cyc_List_List*_tmpB8E;void**_tmpB8D;struct Cyc_Absyn_Exp*_tmpB90;void**_tmpB8F;struct Cyc_List_List*_tmpB92;struct Cyc_Absyn_Exp*_tmpB91;struct Cyc_Absyn_Exp*_tmpB95;struct Cyc_Absyn_Exp*_tmpB94;struct Cyc_Absyn_Exp*_tmpB93;struct Cyc_Absyn_Exp*_tmpB97;struct Cyc_Absyn_Exp*_tmpB96;struct Cyc_Absyn_Exp*_tmpB99;struct Cyc_Absyn_Exp*_tmpB98;struct Cyc_Absyn_Exp*_tmpB9B;struct Cyc_Absyn_Exp*_tmpB9A;struct Cyc_Absyn_Exp*_tmpB9D;struct Cyc_Absyn_Exp*_tmpB9C;struct Cyc_Absyn_Exp*_tmpB9F;struct Cyc_Absyn_Exp*_tmpB9E;struct Cyc_Absyn_Exp*_tmpBA1;struct Cyc_Absyn_Exp*_tmpBA0;struct Cyc_List_List*_tmpBA2;struct Cyc_Absyn_Exp*_tmpBA3;struct Cyc_Absyn_Exp*_tmpBA4;struct Cyc_Absyn_Exp*_tmpBA5;struct Cyc_Absyn_Exp*_tmpBA6;struct Cyc_Absyn_Exp*_tmpBA7;struct Cyc_Absyn_Exp*_tmpBA8;struct Cyc_Absyn_Exp*_tmpBA9;struct Cyc_Absyn_Exp*_tmpBAA;struct Cyc_Absyn_Exp*_tmpBAB;switch(*((int*)_tmpB86)){case 42U: _LL1: _tmpBAB=((struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct*)_tmpB86)->f1;_LL2: {struct Cyc_Absyn_Exp*e=_tmpBAB;
_tmpBAA=e;goto _LL4;}case 20U: _LL3: _tmpBAA=((struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_tmpB86)->f1;_LL4: {struct Cyc_Absyn_Exp*e=_tmpBAA;
_tmpBA9=e;goto _LL6;}case 21U: _LL5: _tmpBA9=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmpB86)->f1;_LL6: {struct Cyc_Absyn_Exp*e=_tmpBA9;
_tmpBA8=e;goto _LL8;}case 22U: _LL7: _tmpBA8=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmpB86)->f1;_LL8: {struct Cyc_Absyn_Exp*e=_tmpBA8;
_tmpBA7=e;goto _LLA;}case 15U: _LL9: _tmpBA7=((struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct*)_tmpB86)->f1;_LLA: {struct Cyc_Absyn_Exp*e=_tmpBA7;
_tmpBA6=e;goto _LLC;}case 11U: _LLB: _tmpBA6=((struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct*)_tmpB86)->f1;_LLC: {struct Cyc_Absyn_Exp*e=_tmpBA6;
_tmpBA5=e;goto _LLE;}case 12U: _LLD: _tmpBA5=((struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*)_tmpB86)->f1;_LLE: {struct Cyc_Absyn_Exp*e=_tmpBA5;
_tmpBA4=e;goto _LL10;}case 18U: _LLF: _tmpBA4=((struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct*)_tmpB86)->f1;_LL10: {struct Cyc_Absyn_Exp*e=_tmpBA4;
_tmpBA3=e;goto _LL12;}case 5U: _LL11: _tmpBA3=((struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*)_tmpB86)->f1;_LL12: {struct Cyc_Absyn_Exp*e1=_tmpBA3;
({Cyc_Toc_exptypes_to_c(e1);});goto _LL0;}case 3U: _LL13: _tmpBA2=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmpB86)->f2;_LL14: {struct Cyc_List_List*es=_tmpBA2;
({({void(*_tmpFAA)(void(*f)(struct Cyc_Absyn_Exp*),struct Cyc_List_List*x)=({void(*_tmpBAC)(void(*f)(struct Cyc_Absyn_Exp*),struct Cyc_List_List*x)=(void(*)(void(*f)(struct Cyc_Absyn_Exp*),struct Cyc_List_List*x))Cyc_List_iter;_tmpBAC;});_tmpFAA(Cyc_Toc_exptypes_to_c,es);});});goto _LL0;}case 7U: _LL15: _tmpBA0=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmpB86)->f1;_tmpBA1=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmpB86)->f2;_LL16: {struct Cyc_Absyn_Exp*e1=_tmpBA0;struct Cyc_Absyn_Exp*e2=_tmpBA1;
_tmpB9E=e1;_tmpB9F=e2;goto _LL18;}case 8U: _LL17: _tmpB9E=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmpB86)->f1;_tmpB9F=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmpB86)->f2;_LL18: {struct Cyc_Absyn_Exp*e1=_tmpB9E;struct Cyc_Absyn_Exp*e2=_tmpB9F;
_tmpB9C=e1;_tmpB9D=e2;goto _LL1A;}case 9U: _LL19: _tmpB9C=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmpB86)->f1;_tmpB9D=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmpB86)->f2;_LL1A: {struct Cyc_Absyn_Exp*e1=_tmpB9C;struct Cyc_Absyn_Exp*e2=_tmpB9D;
_tmpB9A=e1;_tmpB9B=e2;goto _LL1C;}case 23U: _LL1B: _tmpB9A=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmpB86)->f1;_tmpB9B=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmpB86)->f2;_LL1C: {struct Cyc_Absyn_Exp*e1=_tmpB9A;struct Cyc_Absyn_Exp*e2=_tmpB9B;
_tmpB98=e1;_tmpB99=e2;goto _LL1E;}case 36U: _LL1D: _tmpB98=((struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct*)_tmpB86)->f1;_tmpB99=((struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct*)_tmpB86)->f2;_LL1E: {struct Cyc_Absyn_Exp*e1=_tmpB98;struct Cyc_Absyn_Exp*e2=_tmpB99;
_tmpB96=e1;_tmpB97=e2;goto _LL20;}case 4U: _LL1F: _tmpB96=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmpB86)->f1;_tmpB97=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmpB86)->f3;_LL20: {struct Cyc_Absyn_Exp*e1=_tmpB96;struct Cyc_Absyn_Exp*e2=_tmpB97;
({Cyc_Toc_exptypes_to_c(e1);});({Cyc_Toc_exptypes_to_c(e2);});goto _LL0;}case 6U: _LL21: _tmpB93=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmpB86)->f1;_tmpB94=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmpB86)->f2;_tmpB95=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmpB86)->f3;_LL22: {struct Cyc_Absyn_Exp*e1=_tmpB93;struct Cyc_Absyn_Exp*e2=_tmpB94;struct Cyc_Absyn_Exp*e3=_tmpB95;
# 5233
({Cyc_Toc_exptypes_to_c(e1);});({Cyc_Toc_exptypes_to_c(e2);});({Cyc_Toc_exptypes_to_c(e3);});goto _LL0;}case 10U: _LL23: _tmpB91=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmpB86)->f1;_tmpB92=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmpB86)->f2;_LL24: {struct Cyc_Absyn_Exp*e=_tmpB91;struct Cyc_List_List*es=_tmpB92;
# 5235
({Cyc_Toc_exptypes_to_c(e);});({({void(*_tmpFAB)(void(*f)(struct Cyc_Absyn_Exp*),struct Cyc_List_List*x)=({void(*_tmpBAD)(void(*f)(struct Cyc_Absyn_Exp*),struct Cyc_List_List*x)=(void(*)(void(*f)(struct Cyc_Absyn_Exp*),struct Cyc_List_List*x))Cyc_List_iter;_tmpBAD;});_tmpFAB(Cyc_Toc_exptypes_to_c,es);});});goto _LL0;}case 14U: _LL25: _tmpB8F=(void**)&((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmpB86)->f1;_tmpB90=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmpB86)->f2;_LL26: {void**t=_tmpB8F;struct Cyc_Absyn_Exp*e=_tmpB90;
({void*_tmpFAC=({Cyc_Toc_typ_to_c(*t);});*t=_tmpFAC;});({Cyc_Toc_exptypes_to_c(e);});goto _LL0;}case 25U: _LL27: _tmpB8D=(void**)&(((struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct*)_tmpB86)->f1)->f3;_tmpB8E=((struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct*)_tmpB86)->f2;_LL28: {void**t=_tmpB8D;struct Cyc_List_List*dles=_tmpB8E;
# 5238
({void*_tmpFAD=({Cyc_Toc_typ_to_c(*t);});*t=_tmpFAD;});
_tmpB8C=dles;goto _LL2A;}case 37U: _LL29: _tmpB8C=((struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*)_tmpB86)->f2;_LL2A: {struct Cyc_List_List*dles=_tmpB8C;
_tmpB8B=dles;goto _LL2C;}case 26U: _LL2B: _tmpB8B=((struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*)_tmpB86)->f1;_LL2C: {struct Cyc_List_List*dles=_tmpB8B;
# 5242
for(0;dles != 0;dles=dles->tl){
({Cyc_Toc_exptypes_to_c((*((struct _tuple21*)dles->hd)).f2);});}
goto _LL0;}case 19U: _LL2D: _tmpB8A=(void**)&((struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct*)_tmpB86)->f1;_LL2E: {void**t=_tmpB8A;
_tmpB89=t;goto _LL30;}case 17U: _LL2F: _tmpB89=(void**)&((struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct*)_tmpB86)->f1;_LL30: {void**t=_tmpB89;
({void*_tmpFAE=({Cyc_Toc_typ_to_c(*t);});*t=_tmpFAE;});goto _LL0;}case 38U: _LL31: _tmpB88=((struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct*)_tmpB86)->f1;_LL32: {struct Cyc_Absyn_Stmt*s=_tmpB88;
({Cyc_Toc_stmttypes_to_c(s);});goto _LL0;}case 35U: _LL33: _tmpB87=(struct Cyc_Absyn_MallocInfo*)&((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmpB86)->f1;_LL34: {struct Cyc_Absyn_MallocInfo*m=_tmpB87;
# 5249
if(m->elt_type != 0)
({void**_tmpFB0=({void**_tmpBAE=_cycalloc(sizeof(*_tmpBAE));({void*_tmpFAF=({Cyc_Toc_typ_to_c(*((void**)_check_null(m->elt_type)));});*_tmpBAE=_tmpFAF;});_tmpBAE;});m->elt_type=_tmpFB0;});
# 5249
({Cyc_Toc_exptypes_to_c(m->num_elts);});
# 5252
goto _LL0;}case 0U: _LL35: _LL36:
 goto _LL38;case 1U: _LL37: _LL38:
 goto _LL3A;case 32U: _LL39: _LL3A:
 goto _LL3C;case 41U: _LL3B: _LL3C:
 goto _LL3E;case 33U: _LL3D: _LL3E:
 goto _LL0;case 2U: _LL3F: _LL40:
# 5259
 goto _LL42;case 30U: _LL41: _LL42:
 goto _LL44;case 31U: _LL43: _LL44:
 goto _LL46;case 29U: _LL45: _LL46:
 goto _LL48;case 27U: _LL47: _LL48:
 goto _LL4A;case 28U: _LL49: _LL4A:
 goto _LL4C;case 34U: _LL4B: _LL4C:
 goto _LL4E;case 24U: _LL4D: _LL4E:
 goto _LL50;case 13U: _LL4F: _LL50:
 goto _LL52;case 16U: _LL51: _LL52:
 goto _LL54;case 40U: _LL53: _LL54:
 goto _LL56;default: _LL55: _LL56:
({void*_tmpBAF=0U;({int(*_tmpFB3)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmpBB1)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos_loc;_tmpBB1;});unsigned _tmpFB2=e->loc;struct _fat_ptr _tmpFB1=({const char*_tmpBB0="Cyclone expression in C code";_tag_fat(_tmpBB0,sizeof(char),29U);});_tmpFB3(_tmpFB2,_tmpFB1,_tag_fat(_tmpBAF,sizeof(void*),0U));});});}_LL0:;}
# 5274
static void Cyc_Toc_decltypes_to_c(struct Cyc_Absyn_Decl*d){
void*_stmttmp89=d->r;void*_tmpBB3=_stmttmp89;struct Cyc_Absyn_Typedefdecl*_tmpBB4;struct Cyc_Absyn_Enumdecl*_tmpBB5;struct Cyc_Absyn_Aggrdecl*_tmpBB6;struct Cyc_Absyn_Fndecl*_tmpBB7;struct Cyc_Absyn_Vardecl*_tmpBB8;switch(*((int*)_tmpBB3)){case 0U: _LL1: _tmpBB8=((struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_tmpBB3)->f1;_LL2: {struct Cyc_Absyn_Vardecl*vd=_tmpBB8;
# 5277
({void*_tmpFB4=({Cyc_Toc_typ_to_c(vd->type);});vd->type=_tmpFB4;});
if(vd->initializer != 0)({Cyc_Toc_exptypes_to_c((struct Cyc_Absyn_Exp*)_check_null(vd->initializer));});goto _LL0;}case 1U: _LL3: _tmpBB7=((struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*)_tmpBB3)->f1;_LL4: {struct Cyc_Absyn_Fndecl*fd=_tmpBB7;
# 5281
({void*_tmpFB5=({Cyc_Toc_typ_to_c((fd->i).ret_type);});(fd->i).ret_type=_tmpFB5;});
{struct Cyc_List_List*args=(fd->i).args;for(0;args != 0;args=args->tl){
({void*_tmpFB6=({Cyc_Toc_typ_to_c((*((struct _tuple9*)args->hd)).f3);});(*((struct _tuple9*)args->hd)).f3=_tmpFB6;});}}
goto _LL0;}case 5U: _LL5: _tmpBB6=((struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct*)_tmpBB3)->f1;_LL6: {struct Cyc_Absyn_Aggrdecl*ad=_tmpBB6;
({Cyc_Toc_aggrdecl_to_c(ad);});goto _LL0;}case 7U: _LL7: _tmpBB5=((struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct*)_tmpBB3)->f1;_LL8: {struct Cyc_Absyn_Enumdecl*ed=_tmpBB5;
# 5287
if(ed->fields != 0){
struct Cyc_List_List*fs=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(ed->fields))->v;for(0;fs != 0;fs=fs->tl){
struct Cyc_Absyn_Enumfield*f=(struct Cyc_Absyn_Enumfield*)fs->hd;
if(f->tag != 0)({Cyc_Toc_exptypes_to_c((struct Cyc_Absyn_Exp*)_check_null(f->tag));});}}
# 5287
goto _LL0;}case 8U: _LL9: _tmpBB4=((struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct*)_tmpBB3)->f1;_LLA: {struct Cyc_Absyn_Typedefdecl*td=_tmpBB4;
# 5293
({void*_tmpFB7=({Cyc_Toc_typ_to_c((void*)_check_null(td->defn));});td->defn=_tmpFB7;});goto _LL0;}case 2U: _LLB: _LLC:
 goto _LLE;case 3U: _LLD: _LLE:
 goto _LL10;case 6U: _LLF: _LL10:
 goto _LL12;case 9U: _LL11: _LL12:
 goto _LL14;case 10U: _LL13: _LL14:
 goto _LL16;case 11U: _LL15: _LL16:
 goto _LL18;case 12U: _LL17: _LL18:
 goto _LL1A;case 4U: _LL19: _LL1A:
# 5302
({void*_tmpBB9=0U;({int(*_tmpFBA)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmpBBB)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos_loc;_tmpBBB;});unsigned _tmpFB9=d->loc;struct _fat_ptr _tmpFB8=({const char*_tmpBBA="Cyclone declaration in C code";_tag_fat(_tmpBBA,sizeof(char),30U);});_tmpFBA(_tmpFB9,_tmpFB8,_tag_fat(_tmpBB9,sizeof(void*),0U));});});case 13U: _LL1B: _LL1C:
 goto _LL1E;case 14U: _LL1D: _LL1E:
 goto _LL20;case 15U: _LL1F: _LL20:
 goto _LL22;default: _LL21: _LL22:
 goto _LL0;}_LL0:;}
# 5310
static void Cyc_Toc_stmttypes_to_c(struct Cyc_Absyn_Stmt*s){
void*_stmttmp8A=s->r;void*_tmpBBD=_stmttmp8A;struct Cyc_Absyn_Stmt*_tmpBBE;struct Cyc_Absyn_Exp*_tmpBC0;struct Cyc_Absyn_Stmt*_tmpBBF;struct Cyc_Absyn_Stmt*_tmpBC2;struct Cyc_Absyn_Decl*_tmpBC1;void*_tmpBC5;struct Cyc_List_List*_tmpBC4;struct Cyc_Absyn_Exp*_tmpBC3;struct Cyc_Absyn_Stmt*_tmpBC9;struct Cyc_Absyn_Exp*_tmpBC8;struct Cyc_Absyn_Exp*_tmpBC7;struct Cyc_Absyn_Exp*_tmpBC6;struct Cyc_Absyn_Stmt*_tmpBCB;struct Cyc_Absyn_Exp*_tmpBCA;struct Cyc_Absyn_Stmt*_tmpBCE;struct Cyc_Absyn_Stmt*_tmpBCD;struct Cyc_Absyn_Exp*_tmpBCC;struct Cyc_Absyn_Exp*_tmpBCF;struct Cyc_Absyn_Stmt*_tmpBD1;struct Cyc_Absyn_Stmt*_tmpBD0;struct Cyc_Absyn_Exp*_tmpBD2;switch(*((int*)_tmpBBD)){case 1U: _LL1: _tmpBD2=((struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct*)_tmpBBD)->f1;_LL2: {struct Cyc_Absyn_Exp*e=_tmpBD2;
({Cyc_Toc_exptypes_to_c(e);});goto _LL0;}case 2U: _LL3: _tmpBD0=((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_tmpBBD)->f1;_tmpBD1=((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_tmpBBD)->f2;_LL4: {struct Cyc_Absyn_Stmt*s1=_tmpBD0;struct Cyc_Absyn_Stmt*s2=_tmpBD1;
({Cyc_Toc_stmttypes_to_c(s1);});({Cyc_Toc_stmttypes_to_c(s2);});goto _LL0;}case 3U: _LL5: _tmpBCF=((struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct*)_tmpBBD)->f1;_LL6: {struct Cyc_Absyn_Exp*eopt=_tmpBCF;
if(eopt != 0)({Cyc_Toc_exptypes_to_c(eopt);});goto _LL0;}case 4U: _LL7: _tmpBCC=((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmpBBD)->f1;_tmpBCD=((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmpBBD)->f2;_tmpBCE=((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmpBBD)->f3;_LL8: {struct Cyc_Absyn_Exp*e=_tmpBCC;struct Cyc_Absyn_Stmt*s1=_tmpBCD;struct Cyc_Absyn_Stmt*s2=_tmpBCE;
# 5316
({Cyc_Toc_exptypes_to_c(e);});({Cyc_Toc_stmttypes_to_c(s1);});({Cyc_Toc_stmttypes_to_c(s2);});goto _LL0;}case 5U: _LL9: _tmpBCA=(((struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct*)_tmpBBD)->f1).f1;_tmpBCB=((struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct*)_tmpBBD)->f2;_LLA: {struct Cyc_Absyn_Exp*e=_tmpBCA;struct Cyc_Absyn_Stmt*s=_tmpBCB;
# 5318
({Cyc_Toc_exptypes_to_c(e);});({Cyc_Toc_stmttypes_to_c(s);});goto _LL0;}case 9U: _LLB: _tmpBC6=((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmpBBD)->f1;_tmpBC7=(((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmpBBD)->f2).f1;_tmpBC8=(((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmpBBD)->f3).f1;_tmpBC9=((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmpBBD)->f4;_LLC: {struct Cyc_Absyn_Exp*e1=_tmpBC6;struct Cyc_Absyn_Exp*e2=_tmpBC7;struct Cyc_Absyn_Exp*e3=_tmpBC8;struct Cyc_Absyn_Stmt*s=_tmpBC9;
# 5320
({Cyc_Toc_exptypes_to_c(e1);});({Cyc_Toc_exptypes_to_c(e2);});({Cyc_Toc_exptypes_to_c(e3);});
({Cyc_Toc_stmttypes_to_c(s);});goto _LL0;}case 10U: _LLD: _tmpBC3=((struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct*)_tmpBBD)->f1;_tmpBC4=((struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct*)_tmpBBD)->f2;_tmpBC5=(void*)((struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct*)_tmpBBD)->f3;_LLE: {struct Cyc_Absyn_Exp*e=_tmpBC3;struct Cyc_List_List*scs=_tmpBC4;void*dec_tree=_tmpBC5;
# 5323
({Cyc_Toc_exptypes_to_c(e);});
for(0;scs != 0;scs=scs->tl){({Cyc_Toc_stmttypes_to_c(((struct Cyc_Absyn_Switch_clause*)scs->hd)->body);});}
goto _LL0;}case 12U: _LLF: _tmpBC1=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmpBBD)->f1;_tmpBC2=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmpBBD)->f2;_LL10: {struct Cyc_Absyn_Decl*d=_tmpBC1;struct Cyc_Absyn_Stmt*s=_tmpBC2;
({Cyc_Toc_decltypes_to_c(d);});({Cyc_Toc_stmttypes_to_c(s);});goto _LL0;}case 14U: _LL11: _tmpBBF=((struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct*)_tmpBBD)->f1;_tmpBC0=(((struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct*)_tmpBBD)->f2).f1;_LL12: {struct Cyc_Absyn_Stmt*s=_tmpBBF;struct Cyc_Absyn_Exp*e=_tmpBC0;
({Cyc_Toc_stmttypes_to_c(s);});({Cyc_Toc_exptypes_to_c(e);});goto _LL0;}case 13U: _LL13: _tmpBBE=((struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct*)_tmpBBD)->f2;_LL14: {struct Cyc_Absyn_Stmt*s=_tmpBBE;
({Cyc_Toc_stmttypes_to_c(s);});goto _LL0;}case 0U: _LL15: _LL16:
 goto _LL18;case 6U: _LL17: _LL18:
 goto _LL1A;case 7U: _LL19: _LL1A:
 goto _LL1C;case 8U: _LL1B: _LL1C:
 goto _LL0;case 11U: _LL1D: _LL1E:
# 5335
({void*_tmpFBB=(void*)({struct Cyc_Absyn_Skip_s_Absyn_Raw_stmt_struct*_tmpBD3=_cycalloc(sizeof(*_tmpBD3));_tmpBD3->tag=0U;_tmpBD3;});s->r=_tmpFBB;});goto _LL0;default: _LL1F: _LL20:
({void*_tmpBD4=0U;({int(*_tmpFBE)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmpBD6)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos_loc;_tmpBD6;});unsigned _tmpFBD=s->loc;struct _fat_ptr _tmpFBC=({const char*_tmpBD5="Cyclone statement in C code";_tag_fat(_tmpBD5,sizeof(char),28U);});_tmpFBE(_tmpFBD,_tmpFBC,_tag_fat(_tmpBD4,sizeof(void*),0U));});});}_LL0:;}
# 5343
static struct Cyc_Toc_Env*Cyc_Toc_decls_to_c(struct _RegionHandle*r,struct Cyc_Toc_Env*nv,struct Cyc_List_List*ds,int cinclude){
# 5345
for(0;ds != 0;ds=ds->tl){
if(!({Cyc_Toc_is_toplevel(nv);}))
({void*_tmpBD8=0U;({int(*_tmpFC0)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmpBDA)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmpBDA;});struct _fat_ptr _tmpFBF=({const char*_tmpBD9="decls_to_c: not at toplevel!";_tag_fat(_tmpBD9,sizeof(char),29U);});_tmpFC0(_tmpFBF,_tag_fat(_tmpBD8,sizeof(void*),0U));});});
# 5346
Cyc_Toc_fresh_label_counter=0;{
# 5349
struct Cyc_Absyn_Decl*d=(struct Cyc_Absyn_Decl*)ds->hd;
void*_stmttmp8B=d->r;void*_tmpBDB=_stmttmp8B;struct Cyc_List_List*_tmpBDC;struct Cyc_List_List*_tmpBDD;struct Cyc_List_List*_tmpBDE;struct Cyc_List_List*_tmpBDF;struct Cyc_Absyn_Typedefdecl*_tmpBE0;struct Cyc_Absyn_Enumdecl*_tmpBE1;struct Cyc_Absyn_Datatypedecl*_tmpBE2;struct Cyc_Absyn_Aggrdecl*_tmpBE3;struct Cyc_Absyn_Fndecl*_tmpBE4;struct Cyc_Absyn_Vardecl*_tmpBE5;switch(*((int*)_tmpBDB)){case 0U: _LL1: _tmpBE5=((struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_tmpBDB)->f1;_LL2: {struct Cyc_Absyn_Vardecl*vd=_tmpBE5;
# 5352
struct _tuple1*c_name=vd->name;
# 5354
if((int)vd->sc == (int)Cyc_Absyn_ExternC)
c_name=({struct _tuple1*_tmpBE6=_cycalloc(sizeof(*_tmpBE6));({union Cyc_Absyn_Nmspace _tmpFC1=({Cyc_Absyn_Abs_n(0,1);});_tmpBE6->f1=_tmpFC1;}),_tmpBE6->f2=(*c_name).f2;_tmpBE6;});
# 5354
if(vd->initializer != 0){
# 5357
if((int)vd->sc == (int)Cyc_Absyn_ExternC)vd->sc=Cyc_Absyn_Public;if(cinclude)
# 5359
({Cyc_Toc_exptypes_to_c((struct Cyc_Absyn_Exp*)_check_null(vd->initializer));});else{
# 5361
({Cyc_Toc_exp_to_c(nv,(struct Cyc_Absyn_Exp*)_check_null(vd->initializer));});}}
# 5354
vd->name=c_name;
# 5364
({enum Cyc_Absyn_Scope _tmpFC2=({Cyc_Toc_scope_to_c(vd->sc);});vd->sc=_tmpFC2;});
({void*_tmpFC3=({Cyc_Toc_typ_to_c(vd->type);});vd->type=_tmpFC3;});
Cyc_Toc_result_decls=({struct Cyc_List_List*_tmpBE7=_cycalloc(sizeof(*_tmpBE7));_tmpBE7->hd=d,_tmpBE7->tl=Cyc_Toc_result_decls;_tmpBE7;});
goto _LL0;}case 1U: _LL3: _tmpBE4=((struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*)_tmpBDB)->f1;_LL4: {struct Cyc_Absyn_Fndecl*fd=_tmpBE4;
# 5370
if((int)fd->sc == (int)Cyc_Absyn_ExternC){
({struct _tuple1*_tmpFC5=({struct _tuple1*_tmpBE8=_cycalloc(sizeof(*_tmpBE8));({union Cyc_Absyn_Nmspace _tmpFC4=({Cyc_Absyn_Abs_n(0,1);});_tmpBE8->f1=_tmpFC4;}),_tmpBE8->f2=(*fd->name).f2;_tmpBE8;});fd->name=_tmpFC5;});
fd->sc=Cyc_Absyn_Public;}{
# 5370
struct Cyc_List_List*saved_map=Cyc_Toc_xrgn_map;
# 5375
unsigned saved_xrgn_count=Cyc_Toc_xrgn_count;
struct Cyc_List_List*saved_effect=Cyc_Toc_xrgn_effect;
int max=0;
# 5379
{
struct Cyc_List_List*iter;
Cyc_Toc_xrgn_effect=(fd->i).ieffect;
Cyc_Toc_xrgn_map=0;
for(iter=Cyc_Toc_xrgn_effect;iter != 0;(iter=iter->tl,max ++)){
void*_stmttmp8C=({Cyc_Absyn_rgneffect_name((struct Cyc_Absyn_RgnEffect*)iter->hd);});void*_tmpBE9=_stmttmp8C;struct Cyc_Absyn_Tvar*_tmpBEA;if(((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmpBE9)->tag == 2U){_LL24: _tmpBEA=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmpBE9)->f1;_LL25: {struct Cyc_Absyn_Tvar*tv=_tmpBEA;
# 5386
Cyc_Toc_xrgn_map=({struct Cyc_List_List*_tmpBEC=_cycalloc(sizeof(*_tmpBEC));({struct _tuple23*_tmpFC6=({struct _tuple23*_tmpBEB=_cycalloc(sizeof(*_tmpBEB));_tmpBEB->f1=*tv->name,_tmpBEB->f2=(unsigned)max;_tmpBEB;});_tmpBEC->hd=_tmpFC6;}),_tmpBEC->tl=Cyc_Toc_xrgn_map;_tmpBEC;});
goto _LL23;}}else{_LL26: _LL27:
({void*_tmpBED=0U;({int(*_tmpFC8)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmpBEF)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmpBEF;});struct _fat_ptr _tmpFC7=({const char*_tmpBEE="Function declaration.";_tag_fat(_tmpBEE,sizeof(char),22U);});_tmpFC8(_tmpFC7,_tag_fat(_tmpBED,sizeof(void*),0U));});});}_LL23:;}}
# 5392
Cyc_Toc_xrgn_count=(unsigned)max;
# 5397
({Cyc_Toc_fndecl_to_c(nv,fd,cinclude);});{
void*voidstar=({Cyc_Absyn_cstar_type(Cyc_Absyn_void_type,Cyc_Toc_mt_tq);});
# 5400
const unsigned real_arr_size=Cyc_Toc_xrgn_count - (unsigned)max;
const unsigned arr_size=real_arr_size > (unsigned)0?real_arr_size: 1U;
struct _tuple1*inner_handles_var=({Cyc_Toc_temp_var();});
struct Cyc_Absyn_Exp*inner_handles_var_e=({Cyc_Absyn_var_exp(inner_handles_var,0U);});
void*inner_handles_var_typ=({void*(*_tmpC24)(void*elt_type,struct Cyc_Absyn_Tqual tq,struct Cyc_Absyn_Exp*num_elts,void*zero_term,unsigned ztloc)=Cyc_Absyn_array_type;void*_tmpC25=voidstar;struct Cyc_Absyn_Tqual _tmpC26=Cyc_Toc_mt_tq;struct Cyc_Absyn_Exp*_tmpC27=({Cyc_Absyn_uint_exp(arr_size,0U);});void*_tmpC28=Cyc_Absyn_false_type;unsigned _tmpC29=0U;_tmpC24(_tmpC25,_tmpC26,_tmpC27,_tmpC28,_tmpC29);});
# 5406
struct Cyc_Absyn_Vardecl*inner_handles_var_decl=({Cyc_Absyn_new_vardecl(0U,inner_handles_var,inner_handles_var_typ,0);});
# 5409
if(({Cyc_Toc_should_insert_xeffect((fd->i).ieffect,(fd->i).oeffect);})== 0){
# 5411
if(real_arr_size > (unsigned)0){
struct _tuple1*effVar=({Cyc_Toc_temp_var();});
struct Cyc_Absyn_Exp*addr=({struct Cyc_Absyn_Exp*(*_tmpC13)(void*t,struct Cyc_Absyn_Exp*e)=Cyc_Toc_cast_it;void*_tmpC14=({void*(*_tmpC15)(void*,struct Cyc_Absyn_Tqual)=Cyc_Absyn_cstar_type;void*_tmpC16=({Cyc_Toc_effect_frame_type();});struct Cyc_Absyn_Tqual _tmpC17=Cyc_Toc_mt_tq;_tmpC15(_tmpC16,_tmpC17);});struct Cyc_Absyn_Exp*_tmpC18=({struct Cyc_Absyn_Exp*(*_tmpC19)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_address_exp;struct Cyc_Absyn_Exp*_tmpC1A=({Cyc_Absyn_var_exp(effVar,0U);});unsigned _tmpC1B=0U;_tmpC19(_tmpC1A,_tmpC1B);});_tmpC13(_tmpC14,_tmpC18);});
# 5415
struct Cyc_Absyn_Vardecl*effVarDecl=({struct Cyc_Absyn_Vardecl*(*_tmpC0E)(unsigned varloc,struct _tuple1*,void*,struct Cyc_Absyn_Exp*init)=Cyc_Absyn_new_vardecl;unsigned _tmpC0F=0U;struct _tuple1*_tmpC10=effVar;void*_tmpC11=({Cyc_Toc_effect_frame_type();});struct Cyc_Absyn_Exp*_tmpC12=({Cyc_Toc_effect_frame_init(0,inner_handles_var,0);});_tmpC0E(_tmpC0F,_tmpC10,_tmpC11,_tmpC12);});
# 5417
struct Cyc_Absyn_Exp*null_upd_handles_e=({struct Cyc_Absyn_Exp*(*_tmpC08)(void*t,struct Cyc_Absyn_Exp*e)=Cyc_Toc_cast_it;void*_tmpC09=({void*(*_tmpC0A)(void*,struct Cyc_Absyn_Tqual)=Cyc_Absyn_cstar_type;void*_tmpC0B=({Cyc_Toc_effect_frame_caps_type();});struct Cyc_Absyn_Tqual _tmpC0C=Cyc_Toc_mt_tq;_tmpC0A(_tmpC0B,_tmpC0C);});struct Cyc_Absyn_Exp*_tmpC0D=({Cyc_Absyn_uint_exp(0U,0U);});_tmpC08(_tmpC09,_tmpC0D);});
# 5419
struct Cyc_Absyn_Stmt*push=({struct Cyc_Absyn_Stmt*(*_tmpC04)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_exp_stmt;struct Cyc_Absyn_Exp*_tmpC05=({struct Cyc_Absyn_Exp*_tmpC06[3U];_tmpC06[0]=addr,_tmpC06[1]=null_upd_handles_e,({
# 5421
struct Cyc_Absyn_Exp*_tmpFC9=({Cyc_Absyn_uint_exp(0U,0U);});_tmpC06[2]=_tmpFC9;});({struct Cyc_Absyn_Exp*_tmpFCA=Cyc_Toc__push_effect_e;Cyc_Toc_fncall_exp_dl(_tmpFCA,_tag_fat(_tmpC06,sizeof(struct Cyc_Absyn_Exp*),3U));});});unsigned _tmpC07=0U;_tmpC04(_tmpC05,_tmpC07);});
struct Cyc_Absyn_Stmt*pop=({struct Cyc_Absyn_Stmt*(*_tmpC00)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_exp_stmt;struct Cyc_Absyn_Exp*_tmpC01=({struct Cyc_Absyn_Exp*_tmpC02[1U];_tmpC02[0]=addr;({struct Cyc_Absyn_Exp*_tmpFCB=Cyc_Toc__pop_effect_e;Cyc_Toc_fncall_exp_dl(_tmpFCB,_tag_fat(_tmpC02,sizeof(struct Cyc_Absyn_Exp*),1U));});});unsigned _tmpC03=0U;_tmpC00(_tmpC01,_tmpC03);});
# 5424
({struct Cyc_Absyn_Stmt*_tmpFCE=({struct Cyc_Absyn_Stmt*(*_tmpBF0)(struct Cyc_Absyn_Decl*,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_decl_stmt;struct Cyc_Absyn_Decl*_tmpBF1=({struct Cyc_Absyn_Decl*_tmpBF3=_cycalloc(sizeof(*_tmpBF3));({void*_tmpFCC=(void*)({struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*_tmpBF2=_cycalloc(sizeof(*_tmpBF2));_tmpBF2->tag=0U,_tmpBF2->f1=inner_handles_var_decl;_tmpBF2;});_tmpBF3->r=_tmpFCC;}),_tmpBF3->loc=0U;_tmpBF3;});struct Cyc_Absyn_Stmt*_tmpBF4=({struct Cyc_Absyn_Stmt*(*_tmpBF5)(struct Cyc_Absyn_Decl*,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_decl_stmt;struct Cyc_Absyn_Decl*_tmpBF6=({struct Cyc_Absyn_Decl*_tmpBF8=_cycalloc(sizeof(*_tmpBF8));
({void*_tmpFCD=(void*)({struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*_tmpBF7=_cycalloc(sizeof(*_tmpBF7));_tmpBF7->tag=0U,_tmpBF7->f1=effVarDecl;_tmpBF7;});_tmpBF8->r=_tmpFCD;}),_tmpBF8->loc=0U;_tmpBF8;});struct Cyc_Absyn_Stmt*_tmpBF9=({struct Cyc_Absyn_Stmt*(*_tmpBFA)(struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_seq_stmt;struct Cyc_Absyn_Stmt*_tmpBFB=push;struct Cyc_Absyn_Stmt*_tmpBFC=({Cyc_Absyn_seq_stmt(fd->body,pop,0U);});unsigned _tmpBFD=0U;_tmpBFA(_tmpBFB,_tmpBFC,_tmpBFD);});unsigned _tmpBFE=0U;_tmpBF5(_tmpBF6,_tmpBF9,_tmpBFE);});unsigned _tmpBFF=0U;_tmpBF0(_tmpBF1,_tmpBF4,_tmpBFF);});
# 5424
fd->body=_tmpFCE;});}}else{
# 5430
struct Cyc_Absyn_Exp*peek_fncall=({struct Cyc_Absyn_Exp*_tmpC22[1U];_tmpC22[0]=inner_handles_var_e;({struct Cyc_Absyn_Exp*_tmpFCF=Cyc_Toc__peek_effect_e;Cyc_Toc_fncall_exp_dl(_tmpFCF,_tag_fat(_tmpC22,sizeof(struct Cyc_Absyn_Exp*),1U));});});
struct Cyc_Absyn_Stmt*peek=({Cyc_Absyn_exp_stmt(peek_fncall,0U);});
# 5434
({struct Cyc_Absyn_Stmt*_tmpFD1=({struct Cyc_Absyn_Stmt*(*_tmpC1C)(struct Cyc_Absyn_Decl*,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_decl_stmt;struct Cyc_Absyn_Decl*_tmpC1D=({struct Cyc_Absyn_Decl*_tmpC1F=_cycalloc(sizeof(*_tmpC1F));({void*_tmpFD0=(void*)({struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*_tmpC1E=_cycalloc(sizeof(*_tmpC1E));_tmpC1E->tag=0U,_tmpC1E->f1=inner_handles_var_decl;_tmpC1E;});_tmpC1F->r=_tmpFD0;}),_tmpC1F->loc=0U;_tmpC1F;});struct Cyc_Absyn_Stmt*_tmpC20=({Cyc_Absyn_seq_stmt(peek,fd->body,0U);});unsigned _tmpC21=0U;_tmpC1C(_tmpC1D,_tmpC20,_tmpC21);});fd->body=_tmpFD1;});}
# 5438
Cyc_Toc_result_decls=({struct Cyc_List_List*_tmpC23=_cycalloc(sizeof(*_tmpC23));_tmpC23->hd=d,_tmpC23->tl=Cyc_Toc_result_decls;_tmpC23;});
# 5440
Cyc_Toc_xrgn_map=saved_map;
Cyc_Toc_xrgn_count=saved_xrgn_count;
Cyc_Toc_xrgn_effect=saved_effect;
goto _LL0;}}}case 2U: _LL5: _LL6:
 goto _LL8;case 3U: _LL7: _LL8:
({void*_tmpC2A=0U;({int(*_tmpFD3)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmpC2C)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmpC2C;});struct _fat_ptr _tmpFD2=({const char*_tmpC2B="letdecl at toplevel";_tag_fat(_tmpC2B,sizeof(char),20U);});_tmpFD3(_tmpFD2,_tag_fat(_tmpC2A,sizeof(void*),0U));});});case 4U: _LL9: _LLA:
({void*_tmpC2D=0U;({int(*_tmpFD5)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmpC2F)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Toc_toc_impos;_tmpC2F;});struct _fat_ptr _tmpFD4=({const char*_tmpC2E="region decl at toplevel";_tag_fat(_tmpC2E,sizeof(char),24U);});_tmpFD5(_tmpFD4,_tag_fat(_tmpC2D,sizeof(void*),0U));});});case 5U: _LLB: _tmpBE3=((struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct*)_tmpBDB)->f1;_LLC: {struct Cyc_Absyn_Aggrdecl*sd=_tmpBE3;
({Cyc_Toc_aggrdecl_to_c(sd);});goto _LL0;}case 6U: _LLD: _tmpBE2=((struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct*)_tmpBDB)->f1;_LLE: {struct Cyc_Absyn_Datatypedecl*tud=_tmpBE2;
# 5449
tud->is_extensible?({Cyc_Toc_xdatatypedecl_to_c(tud);}):({Cyc_Toc_datatypedecl_to_c(tud);});
goto _LL0;}case 7U: _LLF: _tmpBE1=((struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct*)_tmpBDB)->f1;_LL10: {struct Cyc_Absyn_Enumdecl*ed=_tmpBE1;
# 5452
({Cyc_Toc_enumdecl_to_c(ed);});
Cyc_Toc_result_decls=({struct Cyc_List_List*_tmpC30=_cycalloc(sizeof(*_tmpC30));_tmpC30->hd=d,_tmpC30->tl=Cyc_Toc_result_decls;_tmpC30;});
goto _LL0;}case 8U: _LL11: _tmpBE0=((struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct*)_tmpBDB)->f1;_LL12: {struct Cyc_Absyn_Typedefdecl*td=_tmpBE0;
# 5456
td->tvs=0;
if(td->defn != 0){
({void*_tmpFD6=({Cyc_Toc_typ_to_c((void*)_check_null(td->defn));});td->defn=_tmpFD6;});{
# 5461
struct Cyc_Absyn_Decl*ed;
{void*_stmttmp8D=td->defn;void*_tmpC31=_stmttmp8D;unsigned _tmpC33;struct Cyc_Absyn_Enumdecl*_tmpC32;if(_tmpC31 != 0){if(((struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_tmpC31)->tag == 10U){if(((struct Cyc_Absyn_Enum_td_Absyn_Raw_typedecl_struct*)((struct Cyc_Absyn_TypeDecl*)((struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_tmpC31)->f1)->r)->tag == 1U){_LL29: _tmpC32=((struct Cyc_Absyn_Enum_td_Absyn_Raw_typedecl_struct*)(((struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_tmpC31)->f1)->r)->f1;_tmpC33=(((struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_tmpC31)->f1)->loc;_LL2A: {struct Cyc_Absyn_Enumdecl*ed2=_tmpC32;unsigned loc=_tmpC33;
# 5464
ed=({struct Cyc_Absyn_Decl*_tmpC35=_cycalloc(sizeof(*_tmpC35));({void*_tmpFD7=(void*)({struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct*_tmpC34=_cycalloc(sizeof(*_tmpC34));_tmpC34->tag=7U,_tmpC34->f1=ed2;_tmpC34;});_tmpC35->r=_tmpFD7;}),_tmpC35->loc=loc;_tmpC35;});goto _LL28;}}else{goto _LL2B;}}else{goto _LL2B;}}else{_LL2B: _LL2C:
 ed=0;}_LL28:;}
# 5467
if(ed != 0){
Cyc_Toc_result_decls=({struct Cyc_List_List*_tmpC36=_cycalloc(sizeof(*_tmpC36));_tmpC36->hd=ed,_tmpC36->tl=Cyc_Toc_result_decls;_tmpC36;});{
void*_stmttmp8E=ed->r;void*_tmpC37=_stmttmp8E;struct Cyc_Absyn_Enumdecl*_tmpC38;if(((struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct*)_tmpC37)->tag == 7U){_LL2E: _tmpC38=((struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct*)_tmpC37)->f1;_LL2F: {struct Cyc_Absyn_Enumdecl*ed=_tmpC38;
# 5471
({void*_tmpFD9=(void*)({struct Cyc_Absyn_AppType_Absyn_Type_struct*_tmpC3A=_cycalloc(sizeof(*_tmpC3A));_tmpC3A->tag=0U,({void*_tmpFD8=(void*)({struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct*_tmpC39=_cycalloc(sizeof(*_tmpC39));_tmpC39->tag=17U,_tmpC39->f1=ed->name,_tmpC39->f2=ed;_tmpC39;});_tmpC3A->f1=_tmpFD8;}),_tmpC3A->f2=0;_tmpC3A;});td->defn=_tmpFD9;});goto _LL2D;}}else{_LL30: _LL31:
({void*_tmpC3B=0U;({int(*_tmpFDB)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmpC3D)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmpC3D;});struct _fat_ptr _tmpFDA=({const char*_tmpC3C="Toc: enumdecl to name";_tag_fat(_tmpC3C,sizeof(char),22U);});_tmpFDB(_tmpFDA,_tag_fat(_tmpC3B,sizeof(void*),0U));});});}_LL2D:;}}}}else{
# 5476
enum Cyc_Absyn_KindQual _stmttmp8F=((struct Cyc_Absyn_Kind*)((struct Cyc_Core_Opt*)_check_null(td->kind))->v)->kind;enum Cyc_Absyn_KindQual _tmpC3E=_stmttmp8F;if(_tmpC3E == Cyc_Absyn_BoxKind){_LL33: _LL34:
({void*_tmpFDC=({Cyc_Toc_void_star_type();});td->defn=_tmpFDC;});goto _LL32;}else{_LL35: _LL36:
 td->defn=Cyc_Absyn_void_type;goto _LL32;}_LL32:;}
# 5484
if(Cyc_noexpand_r)
Cyc_Toc_result_decls=({struct Cyc_List_List*_tmpC3F=_cycalloc(sizeof(*_tmpC3F));_tmpC3F->hd=d,_tmpC3F->tl=Cyc_Toc_result_decls;_tmpC3F;});
# 5484
goto _LL0;}case 13U: _LL13: _LL14:
# 5487
 goto _LL16;case 14U: _LL15: _LL16:
 goto _LL18;case 15U: _LL17: _LL18:
 goto _LL1A;case 16U: _LL19: _LL1A:
 goto _LL0;case 9U: _LL1B: _tmpBDF=((struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct*)_tmpBDB)->f2;_LL1C: {struct Cyc_List_List*ds2=_tmpBDF;
_tmpBDE=ds2;goto _LL1E;}case 10U: _LL1D: _tmpBDE=((struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct*)_tmpBDB)->f2;_LL1E: {struct Cyc_List_List*ds2=_tmpBDE;
_tmpBDD=ds2;goto _LL20;}case 11U: _LL1F: _tmpBDD=((struct Cyc_Absyn_ExternC_d_Absyn_Raw_decl_struct*)_tmpBDB)->f1;_LL20: {struct Cyc_List_List*ds2=_tmpBDD;
nv=({Cyc_Toc_decls_to_c(r,nv,ds2,cinclude);});goto _LL0;}default: _LL21: _tmpBDC=((struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct*)_tmpBDB)->f1;_LL22: {struct Cyc_List_List*ds2=_tmpBDC;
nv=({Cyc_Toc_decls_to_c(r,nv,ds2,1);});goto _LL0;}}_LL0:;}}
# 5497
return nv;}
# 5500
void Cyc_Toc_init_toc_state(){
# 5502
struct Cyc_Core_NewDynamicRegion _stmttmp90=({Cyc_Core__new_rckey("internal-error","internal-error",0);});struct Cyc_Core_DynamicRegion*_tmpC41;_LL1: _tmpC41=_stmttmp90.key;_LL2: {struct Cyc_Core_DynamicRegion*dyn=_tmpC41;
struct Cyc_Toc_TocState*ts;
{struct _RegionHandle*h=& dyn->h;
ts=({Cyc_Toc_empty_toc_state(h);});;}
Cyc_Toc_toc_state=({struct Cyc_Toc_TocStateWrap*_tmpC43=_region_malloc(Cyc_Core_unique_region,sizeof(*_tmpC43));_tmpC43->dyn=dyn,_tmpC43->state=ts;_tmpC43;});}}
# 5510
static void Cyc_Toc_init(){
({Cyc_Toc_init_toc_state();});
Cyc_Toc_result_decls=0;
Cyc_Toc_tuple_type_counter=0;
Cyc_Toc_temp_var_counter=0;
Cyc_Toc_fresh_label_counter=0;
Cyc_Toc_globals=_tag_fat(({unsigned _tmpC46=38;struct _fat_ptr**_tmpC45=_cycalloc(_check_times(_tmpC46,sizeof(struct _fat_ptr*)));_tmpC45[0]=& Cyc_Toc__throw_str,_tmpC45[1]=& Cyc_Toc_setjmp_str,_tmpC45[2]=& Cyc_Toc__push_handler_str,_tmpC45[3]=& Cyc_Toc__pop_handler_str,_tmpC45[4]=& Cyc_Toc__exn_thrown_str,_tmpC45[5]=& Cyc_Toc__npop_handler_str,_tmpC45[6]=& Cyc_Toc__check_null_str,_tmpC45[7]=& Cyc_Toc__check_known_subscript_null_str,_tmpC45[8]=& Cyc_Toc__check_known_subscript_notnull_str,_tmpC45[9]=& Cyc_Toc__check_fat_subscript_str,_tmpC45[10]=& Cyc_Toc__tag_fat_str,_tmpC45[11]=& Cyc_Toc__untag_fat_ptr_str,_tmpC45[12]=& Cyc_Toc__get_fat_size_str,_tmpC45[13]=& Cyc_Toc__get_zero_arr_size_str,_tmpC45[14]=& Cyc_Toc__fat_ptr_plus_str,_tmpC45[15]=& Cyc_Toc__zero_arr_plus_str,_tmpC45[16]=& Cyc_Toc__fat_ptr_inplace_plus_str,_tmpC45[17]=& Cyc_Toc__zero_arr_inplace_plus_str,_tmpC45[18]=& Cyc_Toc__fat_ptr_inplace_plus_post_str,_tmpC45[19]=& Cyc_Toc__zero_arr_inplace_plus_post_str,_tmpC45[20]=& Cyc_Toc__cycalloc_str,_tmpC45[21]=& Cyc_Toc__cyccalloc_str,_tmpC45[22]=& Cyc_Toc__cycalloc_atomic_str,_tmpC45[23]=& Cyc_Toc__cyccalloc_atomic_str,_tmpC45[24]=& Cyc_Toc__region_malloc_str,_tmpC45[25]=& Cyc_Toc__region_calloc_str,_tmpC45[26]=& Cyc_Toc__check_times_str,_tmpC45[27]=& Cyc_Toc__new_region_str,_tmpC45[28]=& Cyc_Toc__push_region_str,_tmpC45[29]=& Cyc_Toc__pop_region_str,_tmpC45[30]=& Cyc_Toc__push_effect_str,_tmpC45[31]=& Cyc_Toc__pop_effect_str,_tmpC45[32]=& Cyc_Toc__peek_effect_str,_tmpC45[33]=& Cyc_Toc__throw_arraybounds_str,_tmpC45[34]=& Cyc_Toc__fat_ptr_decrease_size_str,_tmpC45[35]=& Cyc_Toc__throw_match_str,_tmpC45[36]=& Cyc_Toc__fast_region_malloc_str,_tmpC45[37]=& Cyc_Toc__spawn_with_effect_str;_tmpC45;}),sizeof(struct _fat_ptr*),38U);}
# 5558
void Cyc_Toc_finish(){
struct Cyc_Toc_TocStateWrap*ts=0;
({struct Cyc_Toc_TocStateWrap*_tmpC48=ts;struct Cyc_Toc_TocStateWrap*_tmpC49=Cyc_Toc_toc_state;ts=_tmpC49;Cyc_Toc_toc_state=_tmpC48;});{
struct Cyc_Toc_TocStateWrap _stmttmp91=*((struct Cyc_Toc_TocStateWrap*)_check_null(ts));struct Cyc_Toc_TocState*_tmpC4B;struct Cyc_Core_DynamicRegion*_tmpC4A;_LL1: _tmpC4A=_stmttmp91.dyn;_tmpC4B=_stmttmp91.state;_LL2: {struct Cyc_Core_DynamicRegion*dyn=_tmpC4A;struct Cyc_Toc_TocState*s=_tmpC4B;
# 5563
{struct _RegionHandle*h=& dyn->h;
{struct Cyc_Toc_TocState _stmttmp92=*s;struct Cyc_Xarray_Xarray*_tmpC4D;_LL4: _tmpC4D=_stmttmp92.temp_labels;_LL5: {struct Cyc_Xarray_Xarray*tls=_tmpC4D;
({Cyc_Xarray_reuse(tls);});}}
# 5564
;}
# 5567
({Cyc_Core_free_rckey(dyn);});
({({void(*_tmpFDD)(struct Cyc_Toc_TocStateWrap*ptr)=({void(*_tmpC4E)(struct Cyc_Toc_TocStateWrap*ptr)=(void(*)(struct Cyc_Toc_TocStateWrap*ptr))Cyc_Core_ufree;_tmpC4E;});_tmpFDD(ts);});});
# 5570
Cyc_Toc_gpop_tables=0;
Cyc_Toc_fn_pop_table=0;}}}
# 5576
struct Cyc_List_List*Cyc_Toc_toc(struct Cyc_Hashtable_Table*pop_tables,struct Cyc_List_List*ds){
# 5578
Cyc_Toc_gpop_tables=({struct Cyc_Hashtable_Table**_tmpC50=_cycalloc(sizeof(*_tmpC50));*_tmpC50=pop_tables;_tmpC50;});
({Cyc_Toc_init();});{
struct _RegionHandle _tmpC51=_new_region("start");struct _RegionHandle*start=& _tmpC51;_push_region(start);
({struct Cyc_Toc_Env*(*_tmpC52)(struct _RegionHandle*r,struct Cyc_Toc_Env*nv,struct Cyc_List_List*ds,int cinclude)=Cyc_Toc_decls_to_c;struct _RegionHandle*_tmpC53=start;struct Cyc_Toc_Env*_tmpC54=({Cyc_Toc_empty_env(start);});struct Cyc_List_List*_tmpC55=ds;int _tmpC56=0;_tmpC52(_tmpC53,_tmpC54,_tmpC55,_tmpC56);});{
# 5583
struct Cyc_List_List*_tmpC57=({Cyc_List_imp_rev(Cyc_Toc_result_decls);});_npop_handler(0U);return _tmpC57;}
# 5581
;_pop_region();}}

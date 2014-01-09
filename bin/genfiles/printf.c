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
extern struct _RegionHandle*Cyc_Core_unique_region;struct Cyc_Core_DynamicRegion;struct Cyc_Core_NewDynamicRegion{struct Cyc_Core_DynamicRegion*key;};struct Cyc_Core_ThinRes{void*arr;unsigned nelts;};
# 41 "cycboot.h"
double modf(double,double*);struct Cyc___cycFILE;
# 51
extern struct Cyc___cycFILE*Cyc_stdout;struct Cyc_String_pa_PrintArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Int_pa_PrintArg_struct{int tag;unsigned long f1;};struct Cyc_Double_pa_PrintArg_struct{int tag;double f1;};struct Cyc_LongDouble_pa_PrintArg_struct{int tag;long double f1;};struct Cyc_ShortPtr_pa_PrintArg_struct{int tag;short*f1;};struct Cyc_IntPtr_pa_PrintArg_struct{int tag;unsigned long*f1;};
# 73
struct _fat_ptr Cyc_aprintf(struct _fat_ptr,struct _fat_ptr);struct Cyc_ShortPtr_sa_ScanfArg_struct{int tag;short*f1;};struct Cyc_UShortPtr_sa_ScanfArg_struct{int tag;unsigned short*f1;};struct Cyc_IntPtr_sa_ScanfArg_struct{int tag;int*f1;};struct Cyc_UIntPtr_sa_ScanfArg_struct{int tag;unsigned*f1;};struct Cyc_StringPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_DoublePtr_sa_ScanfArg_struct{int tag;double*f1;};struct Cyc_FloatPtr_sa_ScanfArg_struct{int tag;float*f1;};struct Cyc_CharPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};
# 161 "cycboot.h"
int Cyc_putc(int,struct Cyc___cycFILE*);
# 224 "cycboot.h"
int Cyc_vfprintf(struct Cyc___cycFILE*,struct _fat_ptr,struct _fat_ptr);
# 228
int Cyc_vprintf(struct _fat_ptr,struct _fat_ptr);
# 232
struct _fat_ptr Cyc_vrprintf(struct _RegionHandle*,struct _fat_ptr,struct _fat_ptr);
# 239
int Cyc_vsnprintf(struct _fat_ptr,unsigned long,struct _fat_ptr,struct _fat_ptr);
# 243
int Cyc_vsprintf(struct _fat_ptr,struct _fat_ptr,struct _fat_ptr);extern char Cyc_FileCloseError[15U];struct Cyc_FileCloseError_exn_struct{char*tag;};extern char Cyc_FileOpenError[14U];struct Cyc_FileOpenError_exn_struct{char*tag;struct _fat_ptr f1;};struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};extern char Cyc_List_List_mismatch[14U];struct Cyc_List_List_mismatch_exn_struct{char*tag;};extern char Cyc_List_Nth[4U];struct Cyc_List_Nth_exn_struct{char*tag;};
# 104 "string.h"
struct _fat_ptr Cyc_strdup(struct _fat_ptr src);
# 87 "printf.cyc"
static struct _fat_ptr Cyc_parg2string(void*x){
void*_tmp0=x;switch(*((int*)_tmp0)){case 0U: _LL1: _LL2:
 return({const char*_tmp1="string";_tag_fat(_tmp1,sizeof(char),7U);});case 1U: _LL3: _LL4:
 return({const char*_tmp2="int";_tag_fat(_tmp2,sizeof(char),4U);});case 2U: _LL5: _LL6:
# 92
 return({const char*_tmp3="double";_tag_fat(_tmp3,sizeof(char),7U);});case 3U: _LL7: _LL8:
 return({const char*_tmp4="long double";_tag_fat(_tmp4,sizeof(char),12U);});case 4U: _LL9: _LLA:
 return({const char*_tmp5="short *";_tag_fat(_tmp5,sizeof(char),8U);});default: _LLB: _LLC:
 return({const char*_tmp6="unsigned long *";_tag_fat(_tmp6,sizeof(char),16U);});}_LL0:;}
# 99
static void*Cyc_badarg(struct _fat_ptr s){
return(void*)_throw(({struct Cyc_Core_Invalid_argument_exn_struct*_tmp8=_cycalloc(sizeof(*_tmp8));_tmp8->tag=Cyc_Core_Invalid_argument,_tmp8->f1=s;_tmp8;}));}
# 104
static int Cyc_va_arg_int(struct _fat_ptr ap){
void*_stmttmp0=*((void**)_check_fat_subscript(ap,sizeof(void*),0U));void*_tmpA=_stmttmp0;unsigned long _tmpB;if(((struct Cyc_Int_pa_PrintArg_struct*)_tmpA)->tag == 1U){_LL1: _tmpB=((struct Cyc_Int_pa_PrintArg_struct*)_tmpA)->f1;_LL2: {unsigned long i=_tmpB;
return(int)i;}}else{_LL3: _LL4:
 return({({int(*_tmpE4)(struct _fat_ptr s)=({int(*_tmpC)(struct _fat_ptr s)=(int(*)(struct _fat_ptr s))Cyc_badarg;_tmpC;});_tmpE4(({const char*_tmpD="printf expected int";_tag_fat(_tmpD,sizeof(char),20U);}));});});}_LL0:;}
# 111
static long Cyc_va_arg_long(struct _fat_ptr ap){
void*_stmttmp1=*((void**)_check_fat_subscript(ap,sizeof(void*),0U));void*_tmpF=_stmttmp1;unsigned long _tmp10;if(((struct Cyc_Int_pa_PrintArg_struct*)_tmpF)->tag == 1U){_LL1: _tmp10=((struct Cyc_Int_pa_PrintArg_struct*)_tmpF)->f1;_LL2: {unsigned long i=_tmp10;
return(long)i;}}else{_LL3: _LL4:
 return({({long(*_tmpE5)(struct _fat_ptr s)=({long(*_tmp11)(struct _fat_ptr s)=(long(*)(struct _fat_ptr s))Cyc_badarg;_tmp11;});_tmpE5(({const char*_tmp12="printf expected int";_tag_fat(_tmp12,sizeof(char),20U);}));});});}_LL0:;}
# 118
static unsigned long Cyc_va_arg_ulong(struct _fat_ptr ap){
void*_stmttmp2=*((void**)_check_fat_subscript(ap,sizeof(void*),0U));void*_tmp14=_stmttmp2;unsigned long _tmp15;if(((struct Cyc_Int_pa_PrintArg_struct*)_tmp14)->tag == 1U){_LL1: _tmp15=((struct Cyc_Int_pa_PrintArg_struct*)_tmp14)->f1;_LL2: {unsigned long i=_tmp15;
return i;}}else{_LL3: _LL4:
 return({({unsigned long(*_tmpE6)(struct _fat_ptr s)=({unsigned long(*_tmp16)(struct _fat_ptr s)=(unsigned long(*)(struct _fat_ptr s))Cyc_badarg;_tmp16;});_tmpE6(({const char*_tmp17="printf expected int";_tag_fat(_tmp17,sizeof(char),20U);}));});});}_LL0:;}
# 125
static unsigned long Cyc_va_arg_uint(struct _fat_ptr ap){
void*_stmttmp3=*((void**)_check_fat_subscript(ap,sizeof(void*),0U));void*_tmp19=_stmttmp3;unsigned long _tmp1A;if(((struct Cyc_Int_pa_PrintArg_struct*)_tmp19)->tag == 1U){_LL1: _tmp1A=((struct Cyc_Int_pa_PrintArg_struct*)_tmp19)->f1;_LL2: {unsigned long i=_tmp1A;
return i;}}else{_LL3: _LL4:
 return({({unsigned long(*_tmpE7)(struct _fat_ptr s)=({unsigned long(*_tmp1B)(struct _fat_ptr s)=(unsigned long(*)(struct _fat_ptr s))Cyc_badarg;_tmp1B;});_tmpE7(({const char*_tmp1C="printf expected int";_tag_fat(_tmp1C,sizeof(char),20U);}));});});}_LL0:;}
# 133
static double Cyc_va_arg_double(struct _fat_ptr ap){
void*_stmttmp4=*((void**)_check_fat_subscript(ap,sizeof(void*),0U));void*_tmp1E=_stmttmp4;long double _tmp1F;double _tmp20;switch(*((int*)_tmp1E)){case 2U: _LL1: _tmp20=((struct Cyc_Double_pa_PrintArg_struct*)_tmp1E)->f1;_LL2: {double d=_tmp20;
return d;}case 3U: _LL3: _tmp1F=((struct Cyc_LongDouble_pa_PrintArg_struct*)_tmp1E)->f1;_LL4: {long double ld=_tmp1F;
return(double)ld;}default: _LL5: _LL6:
# 138
(int)_throw(({void*(*_tmp21)(struct _fat_ptr s)=Cyc_badarg;struct _fat_ptr _tmp22=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp25=({struct Cyc_String_pa_PrintArg_struct _tmpE0;_tmpE0.tag=0U,({struct _fat_ptr _tmpE8=(struct _fat_ptr)((struct _fat_ptr)({Cyc_parg2string(*((void**)_check_fat_subscript(ap,sizeof(void*),0U)));}));_tmpE0.f1=_tmpE8;});_tmpE0;});void*_tmp23[1U];_tmp23[0]=& _tmp25;({struct _fat_ptr _tmpE9=({const char*_tmp24="printf expected double but found %s";_tag_fat(_tmp24,sizeof(char),36U);});Cyc_aprintf(_tmpE9,_tag_fat(_tmp23,sizeof(void*),1U));});});_tmp21(_tmp22);}));}_LL0:;}
# 144
static short*Cyc_va_arg_short_ptr(struct _fat_ptr ap){
void*_stmttmp5=*((void**)_check_fat_subscript(ap,sizeof(void*),0U));void*_tmp27=_stmttmp5;short*_tmp28;if(((struct Cyc_ShortPtr_pa_PrintArg_struct*)_tmp27)->tag == 4U){_LL1: _tmp28=((struct Cyc_ShortPtr_pa_PrintArg_struct*)_tmp27)->f1;_LL2: {short*p=_tmp28;
return p;}}else{_LL3: _LL4:
(int)_throw(({Cyc_badarg(({const char*_tmp29="printf expected short pointer";_tag_fat(_tmp29,sizeof(char),30U);}));}));}_LL0:;}
# 152
static unsigned long*Cyc_va_arg_int_ptr(struct _fat_ptr ap){
void*_stmttmp6=*((void**)_check_fat_subscript(ap,sizeof(void*),0U));void*_tmp2B=_stmttmp6;unsigned long*_tmp2C;if(((struct Cyc_IntPtr_pa_PrintArg_struct*)_tmp2B)->tag == 5U){_LL1: _tmp2C=((struct Cyc_IntPtr_pa_PrintArg_struct*)_tmp2B)->f1;_LL2: {unsigned long*p=_tmp2C;
return p;}}else{_LL3: _LL4:
(int)_throw(({Cyc_badarg(({const char*_tmp2D="printf expected int pointer";_tag_fat(_tmp2D,sizeof(char),28U);}));}));}_LL0:;}
# 160
static const struct _fat_ptr Cyc_va_arg_string(struct _fat_ptr ap){
void*_stmttmp7=*((void**)_check_fat_subscript(ap,sizeof(void*),0U));void*_tmp2F=_stmttmp7;struct _fat_ptr _tmp30;if(((struct Cyc_String_pa_PrintArg_struct*)_tmp2F)->tag == 0U){_LL1: _tmp30=((struct Cyc_String_pa_PrintArg_struct*)_tmp2F)->f1;_LL2: {struct _fat_ptr s=_tmp30;
return s;}}else{_LL3: _LL4:
(int)_throw(({Cyc_badarg(({const char*_tmp31="printf expected string";_tag_fat(_tmp31,sizeof(char),23U);}));}));}_LL0:;}
# 160
int Cyc___cvt_double(double number,int prec,int flags,int*signp,int fmtch,struct _fat_ptr startp,struct _fat_ptr endp);
# 206 "printf.cyc"
enum Cyc_BASE{Cyc_OCT =0U,Cyc_DEC =1U,Cyc_HEX =2U};
# 212
inline static int Cyc__IO_sputn(int(*ioputc)(int,void*),void*ioputc_env,struct _fat_ptr s,int howmany){
# 214
int i=0;
while(i < howmany){
if(({ioputc((int)*((const char*)_check_fat_subscript(s,sizeof(char),0U)),ioputc_env);})== - 1)return i;_fat_ptr_inplace_plus(& s,sizeof(char),1);
# 218
++ i;}
# 220
return i;}
# 223
static int Cyc__IO_nzsputn(int(*ioputc)(int,void*),void*ioputc_env,struct _fat_ptr s,int howmany){
# 225
int i=0;
while(i < howmany){
char c;
if((int)(c=*((const char*)_check_fat_subscript(s,sizeof(char),0U)))== 0 ||({ioputc((int)c,ioputc_env);})== - 1)return i;_fat_ptr_inplace_plus(& s,sizeof(char),1);
# 230
++ i;}
# 232
return i;}
# 238
static int Cyc__IO_padn(int(*ioputc)(int,void*),void*ioputc_env,char c,int howmany){
# 240
int i=0;
while(i < howmany){
if(({ioputc((int)c,ioputc_env);})== - 1)return i;++ i;}
# 245
return i;}
# 249
static struct _fat_ptr Cyc_my_memchr(struct _fat_ptr s,char c,int n){
int sz=(int)_get_fat_size(s,sizeof(char));
n=n < sz?n: sz;
for(0;n != 0;(n --,_fat_ptr_inplace_plus_post(& s,sizeof(char),1))){
if((int)*((const char*)_check_fat_subscript(s,sizeof(char),0U))== (int)c)return s;}
# 255
return _tag_fat(0,0,0);}
# 258
static struct _fat_ptr Cyc_my_nzmemchr(struct _fat_ptr s,char c,int n){
int sz=(int)_get_fat_size(s,sizeof(char));
n=n < sz?n: sz;
for(0;n != 0;(n --,_fat_ptr_inplace_plus_post(& s,sizeof(char),1))){
if((int)*((const char*)_check_fat_subscript(s,sizeof(char),0U))== (int)c)return s;}
# 264
return _tag_fat(0,0,0);}
# 267
inline static const unsigned long Cyc_my_strlen(struct _fat_ptr s){
unsigned sz=_get_fat_size(s,sizeof(char));
unsigned i=0U;
while(i < sz &&(int)((const char*)s.curr)[(int)i]!= 0){++ i;}
return i;}
# 278
int Cyc__IO_vfprintf(int(*ioputc)(int,void*),void*ioputc_env,struct _fat_ptr fmt0,struct _fat_ptr ap){
# 282
struct _fat_ptr fmt;
register int ch;
register int n;
# 286
struct _fat_ptr cp=_tag_fat(0,0,0);
# 289
struct _fat_ptr cp2=_tag_fat(0,0,0);
# 292
struct _fat_ptr cp3=_tag_fat(0,0,0);
# 295
int which_cp;
struct _fat_ptr fmark;
register int flags;
int ret;
int width;
int prec;
char sign;
# 303
char sign_string[2U];sign_string[0]='\000',sign_string[1]='\000';{
int softsign=0;
double _double;
int fpprec;
unsigned long _ulong;
int dprec;
int dpad;
int fieldsz;
# 314
int size=0;
# 316
char buf[349U];({{unsigned _tmpE1=348U;unsigned i;for(i=0;i < _tmpE1;++ i){buf[i]='\000';}buf[_tmpE1]=0;}0;});{
char ox[2U];ox[0]='\000',ox[1]='\000';{
enum Cyc_BASE base;
# 342 "printf.cyc"
fmt=fmt0;
ret=0;
# 348
for(0;1;0){
# 351
fmark=fmt;{
unsigned fmt_sz=_get_fat_size(fmt,sizeof(char));
for(n=0;((unsigned)n < fmt_sz &&(ch=(int)((const char*)fmt.curr)[n])!= (int)'\000')&& ch != (int)'%';++ n){
;}
fmt=_fat_ptr_plus(fmt,sizeof(char),n);
# 357
if((n=(fmt.curr - fmark.curr)/ sizeof(char))!= 0){
do{if(({int _tmpEA=({Cyc__IO_sputn(ioputc,ioputc_env,(struct _fat_ptr)fmark,n);});_tmpEA != n;}))goto error;}while(0);
ret +=n;}
# 357
if(ch == (int)'\000')
# 362
goto done;
# 357
_fat_ptr_inplace_plus(& fmt,sizeof(char),1);
# 365
flags=0;
dprec=0;
fpprec=0;
width=0;
prec=-1;
sign='\000';
# 372
rflag: ch=(int)*((const char*)_check_fat_subscript(_fat_ptr_inplace_plus_post(& fmt,sizeof(char),1),sizeof(char),0U));
reswitch: which_cp=0;
{int _tmp39=ch;switch(_tmp39){case 32U: _LL1: _LL2:
# 381
 if(!((int)sign))
sign=' ';
# 381
goto rflag;case 35U: _LL3: _LL4:
# 385
 flags |=8;
goto rflag;case 42U: _LL5: _LL6:
# 394
 width=({Cyc_va_arg_int(ap);});_fat_ptr_inplace_plus(& ap,sizeof(void*),1);
if(width >= 0)
goto rflag;
# 395
width=- width;
# 398
goto _LL8;case 45U: _LL7: _LL8:
# 400
 flags |=16;
flags &=~ 32;
goto rflag;case 43U: _LL9: _LLA:
# 404
 sign='+';
goto rflag;case 46U: _LLB: _LLC:
# 407
 if((ch=(int)*((const char*)_check_fat_subscript(_fat_ptr_inplace_plus_post(& fmt,sizeof(char),1),sizeof(char),0U)))== (int)'*'){
n=({Cyc_va_arg_int(ap);});_fat_ptr_inplace_plus(& ap,sizeof(void*),1);
prec=n < 0?- 1: n;
goto rflag;}
# 407
n=0;
# 413
while((unsigned)(ch - (int)'0')<= (unsigned)9){
n=10 * n + (ch - (int)'0');
ch=(int)*((const char*)_check_fat_subscript(_fat_ptr_inplace_plus_post(& fmt,sizeof(char),1),sizeof(char),0U));}
# 417
prec=n < 0?- 1: n;
goto reswitch;case 48U: _LLD: _LLE:
# 425
 if(!(flags & 16))
flags |=32;
# 425
goto rflag;case 49U: _LLF: _LL10:
# 428
 goto _LL12;case 50U: _LL11: _LL12: goto _LL14;case 51U: _LL13: _LL14: goto _LL16;case 52U: _LL15: _LL16: goto _LL18;case 53U: _LL17: _LL18:
 goto _LL1A;case 54U: _LL19: _LL1A: goto _LL1C;case 55U: _LL1B: _LL1C: goto _LL1E;case 56U: _LL1D: _LL1E: goto _LL20;case 57U: _LL1F: _LL20:
 n=0;
do{
n=10 * n + (ch - (int)'0');
ch=(int)*((const char*)_check_fat_subscript(_fat_ptr_inplace_plus_post(& fmt,sizeof(char),1),sizeof(char),0U));}while((unsigned)(ch - (int)'0')<= (unsigned)9);
# 435
width=n;
goto reswitch;case 76U: _LL21: _LL22:
# 438
 flags |=2;
goto rflag;case 104U: _LL23: _LL24:
# 441
 flags |=4;
goto rflag;case 108U: _LL25: _LL26:
# 444
 flags |=1;
goto rflag;case 99U: _LL27: _LL28:
# 447
 cp=({char*_tmp3A=buf;_tag_fat(_tmp3A,sizeof(char),349U);});
({struct _fat_ptr _tmp3B=cp;char _tmp3C=*((char*)_check_fat_subscript(_tmp3B,sizeof(char),0U));char _tmp3D=(char)({Cyc_va_arg_int(ap);});if(_get_fat_size(_tmp3B,sizeof(char))== 1U &&(_tmp3C == 0 && _tmp3D != 0))_throw_arraybounds();*((char*)_tmp3B.curr)=_tmp3D;});_fat_ptr_inplace_plus(& ap,sizeof(void*),1);
size=1;
sign='\000';
goto _LL0;case 68U: _LL29: _LL2A:
# 453
 flags |=1;
goto _LL2C;case 100U: _LL2B: _LL2C:
 goto _LL2E;case 105U: _LL2D: _LL2E:
 _ulong=(unsigned long)(flags & 1?({Cyc_va_arg_long(ap);}):(flags & 4?(long)((short)({Cyc_va_arg_int(ap);})):({Cyc_va_arg_int(ap);})));_fat_ptr_inplace_plus(& ap,sizeof(void*),1);
if((long)_ulong < 0){
_ulong=- _ulong;
sign='-';}
# 457
base=1U;
# 462
goto number;case 101U: _LL2F: _LL30:
 goto _LL32;case 69U: _LL31: _LL32: goto _LL34;case 102U: _LL33: _LL34: goto _LL36;case 70U: _LL35: _LL36: goto _LL38;case 103U: _LL37: _LL38:
 goto _LL3A;case 71U: _LL39: _LL3A:
 _double=({Cyc_va_arg_double(ap);});_fat_ptr_inplace_plus(& ap,sizeof(void*),1);
# 470
if(prec > 39){
if(ch != (int)'g' && ch != (int)'G' || flags & 8)
fpprec=prec - 39;
# 471
prec=39;}else{
# 474
if(prec == -1)
prec=6;}
# 470
cp=({char*_tmp3E=buf;_tag_fat(_tmp3E,sizeof(char),349U);});
# 483
({struct _fat_ptr _tmp3F=cp;char _tmp40=*((char*)_check_fat_subscript(_tmp3F,sizeof(char),0U));char _tmp41='\000';if(_get_fat_size(_tmp3F,sizeof(char))== 1U &&(_tmp40 == 0 && _tmp41 != 0))_throw_arraybounds();*((char*)_tmp3F.curr)=_tmp41;});
size=({({double _tmpEF=_double;int _tmpEE=prec;int _tmpED=flags;int _tmpEC=ch;struct _fat_ptr _tmpEB=cp;Cyc___cvt_double(_tmpEF,_tmpEE,_tmpED,& softsign,_tmpEC,_tmpEB,_fat_ptr_plus(({char*_tmp42=buf;_tag_fat(_tmp42,sizeof(char),349U);}),sizeof(char),(int)(sizeof(buf)- (unsigned)1)));});});
# 487
if(softsign)
sign='-';
# 487
if((int)*((char*)_check_fat_subscript(cp,sizeof(char),0U))== (int)'\000')
# 490
_fat_ptr_inplace_plus(& cp,sizeof(char),1);
# 487
goto _LL0;case 110U: _LL3B: _LL3C:
# 493
 if(flags & 1)
({unsigned long _tmpF0=(unsigned long)ret;*({Cyc_va_arg_int_ptr(ap);})=_tmpF0;});else{
if(flags & 4)
({short _tmpF1=(short)ret;*({Cyc_va_arg_short_ptr(ap);})=_tmpF1;});else{
# 498
({unsigned long _tmpF2=(unsigned long)ret;*({Cyc_va_arg_int_ptr(ap);})=_tmpF2;});}}
_fat_ptr_inplace_plus(& ap,sizeof(void*),1);
continue;case 79U: _LL3D: _LL3E:
# 502
 flags |=1;
goto _LL40;case 111U: _LL3F: _LL40:
# 505
 _ulong=flags & 1?({Cyc_va_arg_ulong(ap);}):(flags & 4?(unsigned long)((unsigned short)({Cyc_va_arg_int(ap);})):({Cyc_va_arg_uint(ap);}));_fat_ptr_inplace_plus(& ap,sizeof(void*),1);
base=0U;
goto nosign;case 112U: _LL41: _LL42:
# 517 "printf.cyc"
 _ulong=(unsigned long)({Cyc_va_arg_long(ap);});_fat_ptr_inplace_plus(& ap,sizeof(void*),1);
base=2U;
flags |=64;
ch=(int)'x';
goto nosign;case 115U: _LL43: _LL44: {
# 523
struct _fat_ptr b=({Cyc_va_arg_string(ap);});_fat_ptr_inplace_plus(& ap,sizeof(void*),1);
which_cp=3;cp3=b;
if(prec >= 0){
struct _fat_ptr p=({Cyc_my_nzmemchr(cp3,'\000',prec);});
if(({char*_tmpF3=(char*)p.curr;_tmpF3 != (char*)(_tag_fat(0,0,0)).curr;})){
size=(p.curr - cp3.curr)/ sizeof(char);
if(size > prec)
size=prec;}else{
# 532
size=prec;}}else{
# 534
size=(int)({Cyc_my_strlen(cp3);});}
sign='\000';
goto _LL0;}case 85U: _LL45: _LL46:
# 538
 flags |=1;
goto _LL48;case 117U: _LL47: _LL48:
# 541
 _ulong=flags & 1?({Cyc_va_arg_ulong(ap);}):(flags & 4?(unsigned long)((unsigned short)({Cyc_va_arg_int(ap);})):({Cyc_va_arg_uint(ap);}));_fat_ptr_inplace_plus(& ap,sizeof(void*),1);
base=1U;
goto nosign;case 88U: _LL49: _LL4A:
 goto _LL4C;case 120U: _LL4B: _LL4C:
 _ulong=flags & 1?({Cyc_va_arg_ulong(ap);}):(flags & 4?(unsigned long)((unsigned short)({Cyc_va_arg_int(ap);})):({Cyc_va_arg_uint(ap);}));_fat_ptr_inplace_plus(& ap,sizeof(void*),1);
base=2U;
# 548
if(flags & 8 && _ulong != (unsigned long)0)
flags |=64;
# 548
nosign:
# 552
 sign='\000';
# 558
number: if((dprec=prec)>= 0)
flags &=~ 32;
# 558
cp=
# 566
_fat_ptr_plus(({char*_tmp43=buf;_tag_fat(_tmp43,sizeof(char),349U);}),sizeof(char),(308 + 39)+ 1);
if(_ulong != (unsigned long)0 || prec != 0){
struct _fat_ptr xdigs;
# 574
enum Cyc_BASE _tmp44=base;switch(_tmp44){case Cyc_OCT: _LL50: _LL51:
# 576
 do{
({struct _fat_ptr _tmp45=_fat_ptr_inplace_plus(& cp,sizeof(char),-1);char _tmp46=*((char*)_check_fat_subscript(_tmp45,sizeof(char),0U));char _tmp47=(char)((_ulong & (unsigned long)7)+ (unsigned long)'0');if(_get_fat_size(_tmp45,sizeof(char))== 1U &&(_tmp46 == 0 && _tmp47 != 0))_throw_arraybounds();*((char*)_tmp45.curr)=_tmp47;});
_ulong >>=3U;}while((int)_ulong);
# 581
if(flags & 8 &&(int)*((char*)_check_fat_subscript(cp,sizeof(char),0U))!= (int)'0')
({struct _fat_ptr _tmp48=_fat_ptr_inplace_plus(& cp,sizeof(char),-1);char _tmp49=*((char*)_check_fat_subscript(_tmp48,sizeof(char),0U));char _tmp4A='0';if(_get_fat_size(_tmp48,sizeof(char))== 1U &&(_tmp49 == 0 && _tmp4A != 0))_throw_arraybounds();*((char*)_tmp48.curr)=_tmp4A;});
# 581
goto _LL4F;case Cyc_DEC: _LL52: _LL53:
# 587
 while(_ulong >= (unsigned long)10){
({struct _fat_ptr _tmp4B=_fat_ptr_inplace_plus(& cp,sizeof(char),-1);char _tmp4C=*((char*)_check_fat_subscript(_tmp4B,sizeof(char),0U));char _tmp4D=(char)(_ulong % (unsigned long)10 + (unsigned long)'0');if(_get_fat_size(_tmp4B,sizeof(char))== 1U &&(_tmp4C == 0 && _tmp4D != 0))_throw_arraybounds();*((char*)_tmp4B.curr)=_tmp4D;});
_ulong /=10U;}
# 591
({struct _fat_ptr _tmp4E=_fat_ptr_inplace_plus(& cp,sizeof(char),-1);char _tmp4F=*((char*)_check_fat_subscript(_tmp4E,sizeof(char),0U));char _tmp50=(char)(_ulong + (unsigned long)'0');if(_get_fat_size(_tmp4E,sizeof(char))== 1U &&(_tmp4F == 0 && _tmp50 != 0))_throw_arraybounds();*((char*)_tmp4E.curr)=_tmp50;});
goto _LL4F;case Cyc_HEX: _LL54: _LL55:
# 595
 if(ch == (int)'X')
xdigs=({const char*_tmp51="0123456789ABCDEF";_tag_fat(_tmp51,sizeof(char),17U);});else{
# 598
xdigs=({const char*_tmp52="0123456789abcdef";_tag_fat(_tmp52,sizeof(char),17U);});}
do{
({struct _fat_ptr _tmp53=_fat_ptr_inplace_plus(& cp,sizeof(char),-1);char _tmp54=*((char*)_check_fat_subscript(_tmp53,sizeof(char),0U));char _tmp55=*((const char*)_check_fat_subscript(xdigs,sizeof(char),(int)(_ulong & (unsigned long)15)));if(_get_fat_size(_tmp53,sizeof(char))== 1U &&(_tmp54 == 0 && _tmp55 != 0))_throw_arraybounds();*((char*)_tmp53.curr)=_tmp55;});
_ulong >>=4U;}while((int)_ulong);
# 603
goto _LL4F;default: _LL56: _LL57:
# 605
 cp=(struct _fat_ptr)({Cyc_strdup(({const char*_tmp56="bug in vform: bad base";_tag_fat(_tmp56,sizeof(char),23U);}));});
goto skipsize;}_LL4F:;}
# 567
size=({
# 609
unsigned char*_tmpF4=(_fat_ptr_plus(({char*_tmp57=buf;_tag_fat(_tmp57,sizeof(char),349U);}),sizeof(char),(308 + 39)+ 1)).curr;_tmpF4 - cp.curr;})/ sizeof(char);
skipsize:
 goto _LL0;default: _LL4D: _LL4E:
# 613
 if(ch == (int)'\000')
goto done;
# 613
cp=({char*_tmp58=buf;_tag_fat(_tmp58,sizeof(char),349U);});
# 617
({struct _fat_ptr _tmp59=cp;char _tmp5A=*((char*)_check_fat_subscript(_tmp59,sizeof(char),0U));char _tmp5B=(char)ch;if(_get_fat_size(_tmp59,sizeof(char))== 1U &&(_tmp5A == 0 && _tmp5B != 0))_throw_arraybounds();*((char*)_tmp59.curr)=_tmp5B;});
size=1;
sign='\000';
goto _LL0;}_LL0:;}
# 645 "printf.cyc"
fieldsz=size + fpprec;
dpad=dprec - size;
if(dpad < 0)
dpad=0;
# 647
if((int)sign)
# 651
++ fieldsz;else{
if(flags & 64)
fieldsz +=2;}
# 647
fieldsz +=dpad;
# 657
if((flags & (16 | 32))== 0){
if(({int _tmpF5=({Cyc__IO_padn(ioputc,ioputc_env,' ',width - fieldsz);});_tmpF5 < width - fieldsz;}))goto error;}
# 661
if((int)sign){
({struct _fat_ptr _tmp5D=_fat_ptr_plus(({char*_tmp5C=sign_string;_tag_fat(_tmp5C,sizeof(char),2U);}),sizeof(char),0);char _tmp5E=*((char*)_check_fat_subscript(_tmp5D,sizeof(char),0U));char _tmp5F=sign;if(_get_fat_size(_tmp5D,sizeof(char))== 1U &&(_tmp5E == 0 && _tmp5F != 0))_throw_arraybounds();*((char*)_tmp5D.curr)=_tmp5F;});
do{if(({({int(*_tmpF7)(int,void*)=ioputc;void*_tmpF6=ioputc_env;Cyc__IO_sputn(_tmpF7,_tmpF6,(struct _fat_ptr)({char*_tmp60=sign_string;_tag_fat(_tmp60,sizeof(char),2U);}),1);});})!= 1)goto error;}while(0);}else{
if(flags & 64){
ox[0]='0';
ox[1]=(char)ch;
do{if(({({int(*_tmpF9)(int,void*)=ioputc;void*_tmpF8=ioputc_env;Cyc__IO_nzsputn(_tmpF9,_tmpF8,_tag_fat(ox,sizeof(char),2U),2);});})!= 2)goto error;}while(0);}}
# 661
if((flags & (16 | 32))== 32){
# 672
if(({int _tmpFA=({Cyc__IO_padn(ioputc,ioputc_env,'0',width - fieldsz);});_tmpFA < width - fieldsz;}))goto error;}
# 675
if(({int _tmpFB=({Cyc__IO_padn(ioputc,ioputc_env,'0',dpad);});_tmpFB < dpad;}))goto error;
# 678
if(which_cp == 0)
do{if(({int _tmpFC=({Cyc__IO_sputn(ioputc,ioputc_env,(struct _fat_ptr)cp,size);});_tmpFC != size;}))goto error;}while(0);else{
if(which_cp == 2)
do{if(({int _tmpFD=({Cyc__IO_sputn(ioputc,ioputc_env,(struct _fat_ptr)cp2,size);});_tmpFD != size;}))goto error;}while(0);else{
if(which_cp == 3)
do{if(({int _tmpFE=({Cyc__IO_nzsputn(ioputc,ioputc_env,(struct _fat_ptr)cp3,size);});_tmpFE != size;}))goto error;}while(0);}}
# 678
if(({
# 686
int _tmpFF=({Cyc__IO_padn(ioputc,ioputc_env,'0',fpprec);});_tmpFF < fpprec;}))goto error;
# 689
if(flags & 16){
if(({int _tmp100=({Cyc__IO_padn(ioputc,ioputc_env,' ',width - fieldsz);});_tmp100 < width - fieldsz;}))goto error;}
# 693
ret +=width > fieldsz?width: fieldsz;}}
# 696
done:
 return ret;
error:
 return - 1;}}}}
# 703
static struct _fat_ptr Cyc_exponent(struct _fat_ptr p,int exp,int fmtch){
# 705
struct _fat_ptr t;
char expbuffer[309U];({{unsigned _tmpE2=308U;unsigned i;for(i=0;i < _tmpE2;++ i){expbuffer[i]='0';}expbuffer[_tmpE2]=0;}0;});{
struct _fat_ptr expbuf=({char*_tmp7A=(char*)expbuffer;_tag_fat(_tmp7A,sizeof(char),309U);});
({struct _fat_ptr _tmp62=_fat_ptr_inplace_plus_post(& p,sizeof(char),1);char _tmp63=*((char*)_check_fat_subscript(_tmp62,sizeof(char),0U));char _tmp64=(char)fmtch;if(_get_fat_size(_tmp62,sizeof(char))== 1U &&(_tmp63 == 0 && _tmp64 != 0))_throw_arraybounds();*((char*)_tmp62.curr)=_tmp64;});
if(exp < 0){
exp=- exp;
({struct _fat_ptr _tmp65=_fat_ptr_inplace_plus_post(& p,sizeof(char),1);char _tmp66=*((char*)_check_fat_subscript(_tmp65,sizeof(char),0U));char _tmp67='-';if(_get_fat_size(_tmp65,sizeof(char))== 1U &&(_tmp66 == 0 && _tmp67 != 0))_throw_arraybounds();*((char*)_tmp65.curr)=_tmp67;});}else{
# 714
({struct _fat_ptr _tmp68=_fat_ptr_inplace_plus_post(& p,sizeof(char),1);char _tmp69=*((char*)_check_fat_subscript(_tmp68,sizeof(char),0U));char _tmp6A='+';if(_get_fat_size(_tmp68,sizeof(char))== 1U &&(_tmp69 == 0 && _tmp6A != 0))_throw_arraybounds();*((char*)_tmp68.curr)=_tmp6A;});}
t=_fat_ptr_plus(expbuf,sizeof(char),308);
if(exp > 9){
do{
({struct _fat_ptr _tmp6B=_fat_ptr_inplace_plus(& t,sizeof(char),-1);char _tmp6C=*((char*)_check_fat_subscript(_tmp6B,sizeof(char),0U));char _tmp6D=(char)(exp % 10 + (int)'0');if(_get_fat_size(_tmp6B,sizeof(char))== 1U &&(_tmp6C == 0 && _tmp6D != 0))_throw_arraybounds();*((char*)_tmp6B.curr)=_tmp6D;});}while((exp /=10)> 9);
# 720
({struct _fat_ptr _tmp6E=_fat_ptr_inplace_plus(& t,sizeof(char),-1);char _tmp6F=*((char*)_check_fat_subscript(_tmp6E,sizeof(char),0U));char _tmp70=(char)(exp + (int)'0');if(_get_fat_size(_tmp6E,sizeof(char))== 1U &&(_tmp6F == 0 && _tmp70 != 0))_throw_arraybounds();*((char*)_tmp6E.curr)=_tmp70;});
for(0;({char*_tmp101=(char*)t.curr;_tmp101 < (char*)(_fat_ptr_plus(expbuf,sizeof(char),308)).curr;});({struct _fat_ptr _tmp71=_fat_ptr_inplace_plus_post(& p,sizeof(char),1);char _tmp72=*((char*)_check_fat_subscript(_tmp71,sizeof(char),0U));char _tmp73=*((char*)_check_fat_subscript(_fat_ptr_inplace_plus_post(& t,sizeof(char),1),sizeof(char),0U));if(_get_fat_size(_tmp71,sizeof(char))== 1U &&(_tmp72 == 0 && _tmp73 != 0))_throw_arraybounds();*((char*)_tmp71.curr)=_tmp73;})){;}}else{
# 724
({struct _fat_ptr _tmp74=_fat_ptr_inplace_plus_post(& p,sizeof(char),1);char _tmp75=*((char*)_check_fat_subscript(_tmp74,sizeof(char),0U));char _tmp76='0';if(_get_fat_size(_tmp74,sizeof(char))== 1U &&(_tmp75 == 0 && _tmp76 != 0))_throw_arraybounds();*((char*)_tmp74.curr)=_tmp76;});
({struct _fat_ptr _tmp77=_fat_ptr_inplace_plus_post(& p,sizeof(char),1);char _tmp78=*((char*)_check_fat_subscript(_tmp77,sizeof(char),0U));char _tmp79=(char)(exp + (int)'0');if(_get_fat_size(_tmp77,sizeof(char))== 1U &&(_tmp78 == 0 && _tmp79 != 0))_throw_arraybounds();*((char*)_tmp77.curr)=_tmp79;});}
# 727
return p;}}
# 730
static struct _fat_ptr Cyc_sround(double fract,int*exp,struct _fat_ptr start,struct _fat_ptr end,char ch,int*signp){
# 734
double tmp=0.0;
# 736
if(fract != 0.0)
({modf(fract * (double)10,& tmp);});else{
# 739
tmp=(double)((int)ch - (int)'0');}
if(tmp > (double)4)
for(0;1;_fat_ptr_inplace_plus(& end,sizeof(char),-1)){
if((int)*((char*)_check_fat_subscript(end,sizeof(char),0U))== (int)'.')
_fat_ptr_inplace_plus(& end,sizeof(char),-1);
# 742
if((int)({struct _fat_ptr _tmp7C=end;char _tmp7D=*((char*)_check_fat_subscript(_tmp7C,sizeof(char),0U));char _tmp7E=_tmp7D + (int)'\001';if(_get_fat_size(_tmp7C,sizeof(char))== 1U &&(_tmp7D == 0 && _tmp7E != 0))_throw_arraybounds();*((char*)_tmp7C.curr)=_tmp7E;})<= (int)'9')
# 745
break;
# 742
({struct _fat_ptr _tmp7F=end;char _tmp80=*((char*)_check_fat_subscript(_tmp7F,sizeof(char),0U));char _tmp81='0';if(_get_fat_size(_tmp7F,sizeof(char))== 1U &&(_tmp80 == 0 && _tmp81 != 0))_throw_arraybounds();*((char*)_tmp7F.curr)=_tmp81;});
# 747
if((char*)end.curr == (char*)start.curr){
if((unsigned)exp){
({struct _fat_ptr _tmp82=end;char _tmp83=*((char*)_check_fat_subscript(_tmp82,sizeof(char),0U));char _tmp84='1';if(_get_fat_size(_tmp82,sizeof(char))== 1U &&(_tmp83 == 0 && _tmp84 != 0))_throw_arraybounds();*((char*)_tmp82.curr)=_tmp84;});
++(*exp);}else{
# 753
({struct _fat_ptr _tmp85=_fat_ptr_inplace_plus(& end,sizeof(char),-1);char _tmp86=*((char*)_check_fat_subscript(_tmp85,sizeof(char),0U));char _tmp87='1';if(_get_fat_size(_tmp85,sizeof(char))== 1U &&(_tmp86 == 0 && _tmp87 != 0))_throw_arraybounds();*((char*)_tmp85.curr)=_tmp87;});
_fat_ptr_inplace_plus(& start,sizeof(char),-1);}
# 756
break;}}else{
# 760
if(*signp == (int)'-')
for(0;1;_fat_ptr_inplace_plus(& end,sizeof(char),-1)){
if((int)*((char*)_check_fat_subscript(end,sizeof(char),0U))== (int)'.')
_fat_ptr_inplace_plus(& end,sizeof(char),-1);
# 762
if((int)*((char*)_check_fat_subscript(end,sizeof(char),0U))!= (int)'0')
# 765
break;
# 762
if((char*)end.curr == (char*)start.curr)
# 767
*signp=0;}}
# 740
return start;}
# 772
int Cyc___cvt_double(double number,int prec,int flags,int*signp,int fmtch,struct _fat_ptr startp,struct _fat_ptr endp){
# 775
struct _fat_ptr p;struct _fat_ptr t;
register double fract;
int dotrim=0;int expcnt;int gformat=0;
double integer=0.0;double tmp=0.0;
# 780
expcnt=0;
if(number < (double)0){
number=- number;
*signp=(int)'-';}else{
# 785
*signp=0;}
# 787
fract=({modf(number,& integer);});
# 790
t=_fat_ptr_inplace_plus(& startp,sizeof(char),1);
# 796
for(p=_fat_ptr_plus(endp,sizeof(char),- 1);(char*)p.curr >= (char*)startp.curr && integer != 0.0;++ expcnt){
tmp=({modf(integer / (double)10,& integer);});
({struct _fat_ptr _tmp89=_fat_ptr_inplace_plus_post(& p,sizeof(char),-1);char _tmp8A=*((char*)_check_fat_subscript(_tmp89,sizeof(char),0U));char _tmp8B=(char)((int)((tmp + .01)* (double)10)+ (int)'0');if(_get_fat_size(_tmp89,sizeof(char))== 1U &&(_tmp8A == 0 && _tmp8B != 0))_throw_arraybounds();*((char*)_tmp89.curr)=_tmp8B;});}
# 800
{int _tmp8C=fmtch;switch(_tmp8C){case 102U: _LL1: _LL2:
 goto _LL4;case 70U: _LL3: _LL4:
# 803
 if(expcnt)
for(0;({char*_tmp102=(char*)(_fat_ptr_inplace_plus(& p,sizeof(char),1)).curr;_tmp102 < (char*)endp.curr;});({struct _fat_ptr _tmp8D=_fat_ptr_inplace_plus_post(& t,sizeof(char),1);char _tmp8E=*((char*)_check_fat_subscript(_tmp8D,sizeof(char),0U));char _tmp8F=*((char*)_check_fat_subscript(p,sizeof(char),0U));if(_get_fat_size(_tmp8D,sizeof(char))== 1U &&(_tmp8E == 0 && _tmp8F != 0))_throw_arraybounds();*((char*)_tmp8D.curr)=_tmp8F;})){;}else{
# 806
({struct _fat_ptr _tmp90=_fat_ptr_inplace_plus_post(& t,sizeof(char),1);char _tmp91=*((char*)_check_fat_subscript(_tmp90,sizeof(char),0U));char _tmp92='0';if(_get_fat_size(_tmp90,sizeof(char))== 1U &&(_tmp91 == 0 && _tmp92 != 0))_throw_arraybounds();*((char*)_tmp90.curr)=_tmp92;});}
# 811
if(prec || flags & 8)
({struct _fat_ptr _tmp93=_fat_ptr_inplace_plus_post(& t,sizeof(char),1);char _tmp94=*((char*)_check_fat_subscript(_tmp93,sizeof(char),0U));char _tmp95='.';if(_get_fat_size(_tmp93,sizeof(char))== 1U &&(_tmp94 == 0 && _tmp95 != 0))_throw_arraybounds();*((char*)_tmp93.curr)=_tmp95;});
# 811
if(fract != 0.0){
# 815
if(prec)
do{
fract=({modf(fract * (double)10,& tmp);});
({struct _fat_ptr _tmp96=_fat_ptr_inplace_plus_post(& t,sizeof(char),1);char _tmp97=*((char*)_check_fat_subscript(_tmp96,sizeof(char),0U));char _tmp98=(char)((int)tmp + (int)'0');if(_get_fat_size(_tmp96,sizeof(char))== 1U &&(_tmp97 == 0 && _tmp98 != 0))_throw_arraybounds();*((char*)_tmp96.curr)=_tmp98;});}while(
-- prec && fract != 0.0);
# 815
if(fract != 0.0)
# 821
startp=({({double _tmp105=fract;struct _fat_ptr _tmp104=startp;struct _fat_ptr _tmp103=
_fat_ptr_plus(t,sizeof(char),- 1);Cyc_sround(_tmp105,0,_tmp104,_tmp103,'\000',signp);});});}
# 811
for(0;prec --;({struct _fat_ptr _tmp99=
# 824
_fat_ptr_inplace_plus_post(& t,sizeof(char),1);char _tmp9A=*((char*)_check_fat_subscript(_tmp99,sizeof(char),0U));char _tmp9B='0';if(_get_fat_size(_tmp99,sizeof(char))== 1U &&(_tmp9A == 0 && _tmp9B != 0))_throw_arraybounds();*((char*)_tmp99.curr)=_tmp9B;})){;}
goto _LL0;case 101U: _LL5: _LL6:
 goto _LL8;case 69U: _LL7: _LL8:
 eformat: if(expcnt){
({struct _fat_ptr _tmp9C=_fat_ptr_inplace_plus_post(& t,sizeof(char),1);char _tmp9D=*((char*)_check_fat_subscript(_tmp9C,sizeof(char),0U));char _tmp9E=*((char*)_check_fat_subscript(_fat_ptr_inplace_plus(& p,sizeof(char),1),sizeof(char),0U));if(_get_fat_size(_tmp9C,sizeof(char))== 1U &&(_tmp9D == 0 && _tmp9E != 0))_throw_arraybounds();*((char*)_tmp9C.curr)=_tmp9E;});
if(prec || flags & 8)
({struct _fat_ptr _tmp9F=_fat_ptr_inplace_plus_post(& t,sizeof(char),1);char _tmpA0=*((char*)_check_fat_subscript(_tmp9F,sizeof(char),0U));char _tmpA1='.';if(_get_fat_size(_tmp9F,sizeof(char))== 1U &&(_tmpA0 == 0 && _tmpA1 != 0))_throw_arraybounds();*((char*)_tmp9F.curr)=_tmpA1;});
# 829
for(0;
# 832
prec &&({char*_tmp106=(char*)(_fat_ptr_inplace_plus(& p,sizeof(char),1)).curr;_tmp106 < (char*)endp.curr;});-- prec){
({struct _fat_ptr _tmpA2=_fat_ptr_inplace_plus_post(& t,sizeof(char),1);char _tmpA3=*((char*)_check_fat_subscript(_tmpA2,sizeof(char),0U));char _tmpA4=*((char*)_check_fat_subscript(p,sizeof(char),0U));if(_get_fat_size(_tmpA2,sizeof(char))== 1U &&(_tmpA3 == 0 && _tmpA4 != 0))_throw_arraybounds();*((char*)_tmpA2.curr)=_tmpA4;});}
# 839
if(!prec &&({char*_tmp107=(char*)(_fat_ptr_inplace_plus(& p,sizeof(char),1)).curr;_tmp107 < (char*)endp.curr;})){
fract=(double)0;
startp=({({struct _fat_ptr _tmp10A=startp;struct _fat_ptr _tmp109=
_fat_ptr_plus(t,sizeof(char),- 1);char _tmp108=*((char*)_check_fat_subscript(p,sizeof(char),0U));Cyc_sround((double)0,& expcnt,_tmp10A,_tmp109,_tmp108,signp);});});}
# 839
-- expcnt;}else{
# 848
if(fract != 0.0){
# 850
for(expcnt=- 1;1;-- expcnt){
fract=({modf(fract * (double)10,& tmp);});
if(tmp != 0.0)
break;}
# 855
({struct _fat_ptr _tmpA5=_fat_ptr_inplace_plus_post(& t,sizeof(char),1);char _tmpA6=*((char*)_check_fat_subscript(_tmpA5,sizeof(char),0U));char _tmpA7=(char)((int)tmp + (int)'0');if(_get_fat_size(_tmpA5,sizeof(char))== 1U &&(_tmpA6 == 0 && _tmpA7 != 0))_throw_arraybounds();*((char*)_tmpA5.curr)=_tmpA7;});
if(prec || flags & 8)
({struct _fat_ptr _tmpA8=_fat_ptr_inplace_plus_post(& t,sizeof(char),1);char _tmpA9=*((char*)_check_fat_subscript(_tmpA8,sizeof(char),0U));char _tmpAA='.';if(_get_fat_size(_tmpA8,sizeof(char))== 1U &&(_tmpA9 == 0 && _tmpAA != 0))_throw_arraybounds();*((char*)_tmpA8.curr)=_tmpAA;});}else{
# 860
({struct _fat_ptr _tmpAB=_fat_ptr_inplace_plus_post(& t,sizeof(char),1);char _tmpAC=*((char*)_check_fat_subscript(_tmpAB,sizeof(char),0U));char _tmpAD='0';if(_get_fat_size(_tmpAB,sizeof(char))== 1U &&(_tmpAC == 0 && _tmpAD != 0))_throw_arraybounds();*((char*)_tmpAB.curr)=_tmpAD;});
if(prec || flags & 8)
({struct _fat_ptr _tmpAE=_fat_ptr_inplace_plus_post(& t,sizeof(char),1);char _tmpAF=*((char*)_check_fat_subscript(_tmpAE,sizeof(char),0U));char _tmpB0='.';if(_get_fat_size(_tmpAE,sizeof(char))== 1U &&(_tmpAF == 0 && _tmpB0 != 0))_throw_arraybounds();*((char*)_tmpAE.curr)=_tmpB0;});}}
# 865
if(fract != 0.0){
if(prec)
do{
fract=({modf(fract * (double)10,& tmp);});
({struct _fat_ptr _tmpB1=_fat_ptr_inplace_plus_post(& t,sizeof(char),1);char _tmpB2=*((char*)_check_fat_subscript(_tmpB1,sizeof(char),0U));char _tmpB3=(char)((int)tmp + (int)'0');if(_get_fat_size(_tmpB1,sizeof(char))== 1U &&(_tmpB2 == 0 && _tmpB3 != 0))_throw_arraybounds();*((char*)_tmpB1.curr)=_tmpB3;});}while(
-- prec && fract != 0.0);
# 866
if(fract != 0.0)
# 872
startp=({({double _tmp10D=fract;struct _fat_ptr _tmp10C=startp;struct _fat_ptr _tmp10B=
_fat_ptr_plus(t,sizeof(char),- 1);Cyc_sround(_tmp10D,& expcnt,_tmp10C,_tmp10B,'\000',signp);});});}
# 865
for(0;prec --;({struct _fat_ptr _tmpB4=
# 876
_fat_ptr_inplace_plus_post(& t,sizeof(char),1);char _tmpB5=*((char*)_check_fat_subscript(_tmpB4,sizeof(char),0U));char _tmpB6='0';if(_get_fat_size(_tmpB4,sizeof(char))== 1U &&(_tmpB5 == 0 && _tmpB6 != 0))_throw_arraybounds();*((char*)_tmpB4.curr)=_tmpB6;})){;}
# 879
if(gformat && !(flags & 8)){
while((char*)t.curr > (char*)startp.curr &&(int)*((char*)_check_fat_subscript(_fat_ptr_inplace_plus(& t,sizeof(char),-1),sizeof(char),0U))== (int)'0'){;}
if((int)*((char*)_check_fat_subscript(t,sizeof(char),0U))== (int)'.')
_fat_ptr_inplace_plus(& t,sizeof(char),-1);
# 881
_fat_ptr_inplace_plus(& t,sizeof(char),1);}
# 879
t=({Cyc_exponent(t,expcnt,fmtch);});
# 886
goto _LL0;case 103U: _LL9: _LLA:
 goto _LLC;case 71U: _LLB: _LLC:
# 889
 if(!prec)
++ prec;
# 889
if(
# 897
expcnt > prec ||(!expcnt && fract != 0.0)&& fract < .0001){
# 905
-- prec;
fmtch -=2;
gformat=1;
goto eformat;}
# 889
if(expcnt)
# 915
for(0;({char*_tmp10E=(char*)(_fat_ptr_inplace_plus(& p,sizeof(char),1)).curr;_tmp10E < (char*)endp.curr;});(({struct _fat_ptr _tmpB7=_fat_ptr_inplace_plus_post(& t,sizeof(char),1);char _tmpB8=*((char*)_check_fat_subscript(_tmpB7,sizeof(char),0U));char _tmpB9=*((char*)_check_fat_subscript(p,sizeof(char),0U));if(_get_fat_size(_tmpB7,sizeof(char))== 1U &&(_tmpB8 == 0 && _tmpB9 != 0))_throw_arraybounds();*((char*)_tmpB7.curr)=_tmpB9;}),-- prec)){;}else{
# 917
({struct _fat_ptr _tmpBA=_fat_ptr_inplace_plus_post(& t,sizeof(char),1);char _tmpBB=*((char*)_check_fat_subscript(_tmpBA,sizeof(char),0U));char _tmpBC='0';if(_get_fat_size(_tmpBA,sizeof(char))== 1U &&(_tmpBB == 0 && _tmpBC != 0))_throw_arraybounds();*((char*)_tmpBA.curr)=_tmpBC;});}
# 922
if(prec || flags & 8){
dotrim=1;
({struct _fat_ptr _tmpBD=_fat_ptr_inplace_plus_post(& t,sizeof(char),1);char _tmpBE=*((char*)_check_fat_subscript(_tmpBD,sizeof(char),0U));char _tmpBF='.';if(_get_fat_size(_tmpBD,sizeof(char))== 1U &&(_tmpBE == 0 && _tmpBF != 0))_throw_arraybounds();*((char*)_tmpBD.curr)=_tmpBF;});}else{
# 927
dotrim=0;}
# 929
if(fract != 0.0){
if(prec){
# 933
do{
fract=({modf(fract * (double)10,& tmp);});
({struct _fat_ptr _tmpC0=_fat_ptr_inplace_plus_post(& t,sizeof(char),1);char _tmpC1=*((char*)_check_fat_subscript(_tmpC0,sizeof(char),0U));char _tmpC2=(char)((int)tmp + (int)'0');if(_get_fat_size(_tmpC0,sizeof(char))== 1U &&(_tmpC1 == 0 && _tmpC2 != 0))_throw_arraybounds();*((char*)_tmpC0.curr)=_tmpC2;});}while(
tmp == 0.0 && !expcnt);
while(-- prec && fract != 0.0){
fract=({modf(fract * (double)10,& tmp);});
({struct _fat_ptr _tmpC3=_fat_ptr_inplace_plus_post(& t,sizeof(char),1);char _tmpC4=*((char*)_check_fat_subscript(_tmpC3,sizeof(char),0U));char _tmpC5=(char)((int)tmp + (int)'0');if(_get_fat_size(_tmpC3,sizeof(char))== 1U &&(_tmpC4 == 0 && _tmpC5 != 0))_throw_arraybounds();*((char*)_tmpC3.curr)=_tmpC5;});}}
# 930
if(fract != 0.0)
# 943
startp=({({double _tmp111=fract;struct _fat_ptr _tmp110=startp;struct _fat_ptr _tmp10F=
_fat_ptr_plus(t,sizeof(char),- 1);Cyc_sround(_tmp111,0,_tmp110,_tmp10F,'\000',signp);});});}
# 929
if(flags & 8)
# 948
for(0;prec --;({struct _fat_ptr _tmpC6=_fat_ptr_inplace_plus_post(& t,sizeof(char),1);char _tmpC7=*((char*)_check_fat_subscript(_tmpC6,sizeof(char),0U));char _tmpC8='0';if(_get_fat_size(_tmpC6,sizeof(char))== 1U &&(_tmpC7 == 0 && _tmpC8 != 0))_throw_arraybounds();*((char*)_tmpC6.curr)=_tmpC8;})){;}else{
if(dotrim){
while((char*)t.curr > (char*)startp.curr &&(int)*((char*)_check_fat_subscript(_fat_ptr_inplace_plus(& t,sizeof(char),-1),sizeof(char),0U))== (int)'0'){;}
if((int)*((char*)_check_fat_subscript(t,sizeof(char),0U))!= (int)'.')
_fat_ptr_inplace_plus(& t,sizeof(char),1);}}
# 929
goto _LL0;default: _LLD: _LLE:
# 955
(int)_throw(({struct Cyc_Core_Impossible_exn_struct*_tmpCA=_cycalloc(sizeof(*_tmpCA));_tmpCA->tag=Cyc_Core_Impossible,({struct _fat_ptr _tmp112=({const char*_tmpC9="__cvt_double";_tag_fat(_tmpC9,sizeof(char),13U);});_tmpCA->f1=_tmp112;});_tmpCA;}));}_LL0:;}
# 957
return(t.curr - startp.curr)/ sizeof(char);}
# 961
int Cyc_vfprintf(struct Cyc___cycFILE*f,struct _fat_ptr fmt,struct _fat_ptr ap){
# 964
int ans;
ans=({({int(*_tmp115)(int(*ioputc)(int,struct Cyc___cycFILE*),struct Cyc___cycFILE*ioputc_env,struct _fat_ptr fmt0,struct _fat_ptr ap)=({int(*_tmpCC)(int(*ioputc)(int,struct Cyc___cycFILE*),struct Cyc___cycFILE*ioputc_env,struct _fat_ptr fmt0,struct _fat_ptr ap)=(int(*)(int(*ioputc)(int,struct Cyc___cycFILE*),struct Cyc___cycFILE*ioputc_env,struct _fat_ptr fmt0,struct _fat_ptr ap))Cyc__IO_vfprintf;_tmpCC;});struct Cyc___cycFILE*_tmp114=f;struct _fat_ptr _tmp113=fmt;_tmp115(Cyc_putc,_tmp114,_tmp113,ap);});});
return ans;}
# 969
int Cyc_fprintf(struct Cyc___cycFILE*f,struct _fat_ptr fmt,struct _fat_ptr ap){
# 973
return({Cyc_vfprintf(f,fmt,ap);});}
# 976
int Cyc_vprintf(struct _fat_ptr fmt,struct _fat_ptr ap){
# 979
int ans;
ans=({({int(*_tmp118)(int(*ioputc)(int,struct Cyc___cycFILE*),struct Cyc___cycFILE*ioputc_env,struct _fat_ptr fmt0,struct _fat_ptr ap)=({int(*_tmpCF)(int(*ioputc)(int,struct Cyc___cycFILE*),struct Cyc___cycFILE*ioputc_env,struct _fat_ptr fmt0,struct _fat_ptr ap)=(int(*)(int(*ioputc)(int,struct Cyc___cycFILE*),struct Cyc___cycFILE*ioputc_env,struct _fat_ptr fmt0,struct _fat_ptr ap))Cyc__IO_vfprintf;_tmpCF;});struct Cyc___cycFILE*_tmp117=Cyc_stdout;struct _fat_ptr _tmp116=fmt;_tmp118(Cyc_putc,_tmp117,_tmp116,ap);});});
return ans;}
# 984
int Cyc_printf(struct _fat_ptr fmt,struct _fat_ptr ap){
# 987
int ans;
ans=({Cyc_vprintf(fmt,ap);});
return ans;}struct _tuple0{struct _fat_ptr*f1;unsigned long*f2;};
# 992
static int Cyc_putc_string(int c,struct _tuple0*sptr_n){
unsigned long*_tmpD3;struct _fat_ptr*_tmpD2;_LL1: _tmpD2=sptr_n->f1;_tmpD3=sptr_n->f2;_LL2: {struct _fat_ptr*sptr=_tmpD2;unsigned long*nptr=_tmpD3;
struct _fat_ptr s=*sptr;
unsigned long n=*nptr;
if(n == (unsigned long)0)return - 1;*((char*)_check_fat_subscript(s,sizeof(char),0U))=(char)c;
# 998
_fat_ptr_inplace_plus(sptr,sizeof(char),1);
*nptr=n - (unsigned long)1;
return 1;}}
# 1003
int Cyc_vsnprintf(struct _fat_ptr s,unsigned long n,struct _fat_ptr fmt,struct _fat_ptr ap){
# 1006
int ans;
struct _fat_ptr sptr=s;
unsigned long nptr=n;
struct _tuple0 sptr_n=({struct _tuple0 _tmpE3;_tmpE3.f1=& sptr,_tmpE3.f2=& nptr;_tmpE3;});
ans=({({int(*_tmp11A)(int(*ioputc)(int,struct _tuple0*),struct _tuple0*ioputc_env,struct _fat_ptr fmt0,struct _fat_ptr ap)=({int(*_tmpD5)(int(*ioputc)(int,struct _tuple0*),struct _tuple0*ioputc_env,struct _fat_ptr fmt0,struct _fat_ptr ap)=(int(*)(int(*ioputc)(int,struct _tuple0*),struct _tuple0*ioputc_env,struct _fat_ptr fmt0,struct _fat_ptr ap))Cyc__IO_vfprintf;_tmpD5;});struct _fat_ptr _tmp119=fmt;_tmp11A(Cyc_putc_string,& sptr_n,_tmp119,ap);});});
if(0 <= ans)
*((char*)_check_fat_subscript(s,sizeof(char),ans))='\000';
# 1011
return ans;}
# 1016
int Cyc_snprintf(struct _fat_ptr s,unsigned long n,struct _fat_ptr fmt,struct _fat_ptr ap){
# 1019
return({Cyc_vsnprintf(s,n,fmt,ap);});}
# 1022
int Cyc_vsprintf(struct _fat_ptr s,struct _fat_ptr fmt,struct _fat_ptr ap){
# 1025
return({Cyc_vsnprintf(s,_get_fat_size(s,sizeof(char)),fmt,ap);});}
# 1028
int Cyc_sprintf(struct _fat_ptr s,struct _fat_ptr fmt,struct _fat_ptr ap){
# 1031
return({Cyc_vsnprintf(s,_get_fat_size(s,sizeof(char)),fmt,ap);});}
# 1034
static int Cyc_putc_void(int c,int dummy){
return 1;}
# 1038
struct _fat_ptr Cyc_vrprintf(struct _RegionHandle*r1,struct _fat_ptr fmt,struct _fat_ptr ap){
# 1042
int size=({({int(*_tmp11C)(int(*ioputc)(int,int),int ioputc_env,struct _fat_ptr fmt0,struct _fat_ptr ap)=({int(*_tmpDC)(int(*ioputc)(int,int),int ioputc_env,struct _fat_ptr fmt0,struct _fat_ptr ap)=(int(*)(int(*ioputc)(int,int),int ioputc_env,struct _fat_ptr fmt0,struct _fat_ptr ap))Cyc__IO_vfprintf;_tmpDC;});struct _fat_ptr _tmp11B=fmt;_tmp11C(Cyc_putc_void,0,_tmp11B,ap);});})+ 1;
struct _fat_ptr s=(struct _fat_ptr)({unsigned _tmpDB=size + 1;_tag_fat(_cyccalloc_atomic(sizeof(char),_tmpDB),sizeof(char),_tmpDB);});
({({struct _fat_ptr _tmp11E=_fat_ptr_decrease_size(s,sizeof(char),1U);struct _fat_ptr _tmp11D=fmt;Cyc_vsprintf(_tmp11E,_tmp11D,ap);});});
return s;}
# 1048
struct _fat_ptr Cyc_rprintf(struct _RegionHandle*r1,struct _fat_ptr fmt,struct _fat_ptr ap){
# 1051
return({Cyc_vrprintf(r1,fmt,ap);});}
# 1054
struct _fat_ptr Cyc_aprintf(struct _fat_ptr fmt,struct _fat_ptr ap){
# 1057
return({Cyc_vrprintf(Cyc_Core_heap_region,fmt,ap);});}

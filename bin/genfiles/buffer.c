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
# 168 "core.h"
extern struct _RegionHandle*Cyc_Core_unique_region;
# 175
void Cyc_Core_ufree(void*ptr);struct Cyc_Core_DynamicRegion;struct Cyc_Core_NewDynamicRegion{struct Cyc_Core_DynamicRegion*key;};struct Cyc_Core_ThinRes{void*arr;unsigned nelts;};struct Cyc___cycFILE;struct Cyc_String_pa_PrintArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Int_pa_PrintArg_struct{int tag;unsigned long f1;};struct Cyc_Double_pa_PrintArg_struct{int tag;double f1;};struct Cyc_LongDouble_pa_PrintArg_struct{int tag;long double f1;};struct Cyc_ShortPtr_pa_PrintArg_struct{int tag;short*f1;};struct Cyc_IntPtr_pa_PrintArg_struct{int tag;unsigned long*f1;};struct Cyc_ShortPtr_sa_ScanfArg_struct{int tag;short*f1;};struct Cyc_UShortPtr_sa_ScanfArg_struct{int tag;unsigned short*f1;};struct Cyc_IntPtr_sa_ScanfArg_struct{int tag;int*f1;};struct Cyc_UIntPtr_sa_ScanfArg_struct{int tag;unsigned*f1;};struct Cyc_StringPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_DoublePtr_sa_ScanfArg_struct{int tag;double*f1;};struct Cyc_FloatPtr_sa_ScanfArg_struct{int tag;float*f1;};struct Cyc_CharPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};extern char Cyc_FileCloseError[15U];struct Cyc_FileCloseError_exn_struct{char*tag;};extern char Cyc_FileOpenError[14U];struct Cyc_FileOpenError_exn_struct{char*tag;struct _fat_ptr f1;};struct Cyc_Buffer_t;
# 83 "buffer.h"
void Cyc_Buffer_add_substring(struct Cyc_Buffer_t*,struct _fat_ptr,int offset,int len);struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};extern char Cyc_List_List_mismatch[14U];struct Cyc_List_List_mismatch_exn_struct{char*tag;};extern char Cyc_List_Nth[4U];struct Cyc_List_Nth_exn_struct{char*tag;};
# 38 "string.h"
unsigned long Cyc_strlen(struct _fat_ptr s);
# 72 "string.h"
struct _fat_ptr Cyc_strncpy(struct _fat_ptr,struct _fat_ptr,unsigned long);
struct _fat_ptr Cyc_zstrncpy(struct _fat_ptr,struct _fat_ptr,unsigned long);
# 109 "string.h"
struct _fat_ptr Cyc_substring(struct _fat_ptr,int ofs,unsigned long n);struct Cyc_Buffer_t{struct _fat_ptr buffer;unsigned position;unsigned length;struct _fat_ptr initial_buffer;};
# 48 "buffer.cyc"
struct Cyc_Buffer_t*Cyc_Buffer_create(unsigned n){
if(n > (unsigned)0){
struct _fat_ptr s=({unsigned _tmp1=n + (unsigned)1;_tag_fat(_region_calloc(Cyc_Core_unique_region,sizeof(char),_tmp1),sizeof(char),_tmp1);});
return({struct Cyc_Buffer_t*_tmp0=_cycalloc(sizeof(*_tmp0));_tmp0->buffer=s,_tmp0->position=0U,_tmp0->length=n,({struct _fat_ptr _tmp3B=_tag_fat(0,0,0);_tmp0->initial_buffer=_tmp3B;});_tmp0;});}else{
# 53
return({struct Cyc_Buffer_t*_tmp2=_cycalloc(sizeof(*_tmp2));({struct _fat_ptr _tmp3D=_tag_fat(0,0,0);_tmp2->buffer=_tmp3D;}),_tmp2->position=0U,_tmp2->length=0U,({struct _fat_ptr _tmp3C=_tag_fat(0,0,0);_tmp2->initial_buffer=_tmp3C;});_tmp2;});}}
# 48
struct _fat_ptr Cyc_Buffer_contents(struct Cyc_Buffer_t*b){
# 58
struct _fat_ptr buf=_tag_fat(0,0,0);
({struct _fat_ptr _tmp4=buf;struct _fat_ptr _tmp5=b->buffer;buf=_tmp5;b->buffer=_tmp4;});
if(({char*_tmp3E=(char*)buf.curr;_tmp3E == (char*)(_tag_fat(0,0,0)).curr;}))
return _tag_fat(0,0,0);{
# 60
struct _fat_ptr _tmp6=({
# 62
struct _fat_ptr _tmp9;_LL1: _tmp9=buf;_LL2: ({struct _fat_ptr __aliasvar0=_tmp9;({Cyc_substring(__aliasvar0,0,b->position);});});});
# 60
struct _fat_ptr res=_tmp6;
# 63
({struct _fat_ptr _tmp7=buf;struct _fat_ptr _tmp8=b->buffer;buf=_tmp8;b->buffer=_tmp7;});
return res;}}
# 67
struct _fat_ptr Cyc_Buffer_extract(struct Cyc_Buffer_t*b){
struct _fat_ptr buf=_tag_fat(0,0,0);
({struct _fat_ptr _tmpB=buf;struct _fat_ptr _tmpC=b->buffer;buf=_tmpC;b->buffer=_tmpB;});
({struct _fat_ptr _tmpD=_fat_ptr_plus(buf,sizeof(char),(int)b->position);char _tmpE=*((char*)_check_fat_subscript(_tmpD,sizeof(char),0U));char _tmpF='\000';if(_get_fat_size(_tmpD,sizeof(char))== 1U &&(_tmpE == 0 && _tmpF != 0))_throw_arraybounds();*((char*)_tmpD.curr)=_tmpF;});
b->position=0U;
b->length=0U;
return buf;}
# 67
int Cyc_Buffer_restore(struct Cyc_Buffer_t*b,struct _fat_ptr buf){
# 77
int len=(int)({struct _fat_ptr _tmp16;_LL1: _tmp16=buf;_LL2: ({struct _fat_ptr __aliasvar1=_tmp16;({Cyc_strlen(__aliasvar1);});});});
({struct _fat_ptr _tmp11=buf;struct _fat_ptr _tmp12=b->buffer;buf=_tmp12;b->buffer=_tmp11;});
if(({char*_tmp3F=(char*)buf.curr;_tmp3F != (char*)(_tag_fat(0,0,0)).curr;})){
({struct _fat_ptr _tmp13=buf;struct _fat_ptr _tmp14=b->buffer;buf=_tmp14;b->buffer=_tmp13;});
({({void(*_tmp40)(char*ptr)=({void(*_tmp15)(char*ptr)=(void(*)(char*ptr))Cyc_Core_ufree;_tmp15;});_tmp40((char*)_untag_fat_ptr(buf,sizeof(char),1U + 1U));});});
return 0;}
# 79
b->position=0U;
# 85
b->length=(unsigned)len;
return 1;}
# 67
unsigned long Cyc_Buffer_length(struct Cyc_Buffer_t*b){
# 90
return b->position;}
# 93
void Cyc_Buffer_clear(struct Cyc_Buffer_t*b){
b->position=0U;}
# 97
void Cyc_Buffer_reset(struct Cyc_Buffer_t*b){
b->position=0U;
if(({char*_tmp41=(char*)(b->initial_buffer).curr;_tmp41 != (char*)(_tag_fat(0,0,0)).curr;})){
struct _fat_ptr buf=_tag_fat(0,0,0);
({struct _fat_ptr _tmp1A=b->initial_buffer;struct _fat_ptr _tmp1B=buf;b->initial_buffer=_tmp1B;buf=_tmp1A;});
b->length=_get_fat_size(buf,sizeof(char))- (unsigned)1;
({struct _fat_ptr _tmp1C=b->buffer;struct _fat_ptr _tmp1D=buf;b->buffer=_tmp1D;buf=_tmp1C;});
({({void(*_tmp42)(char*ptr)=({void(*_tmp1E)(char*ptr)=(void(*)(char*ptr))Cyc_Core_ufree;_tmp1E;});_tmp42((char*)_untag_fat_ptr(buf,sizeof(char),1U + 1U));});});}
# 99
return;}
# 109
static void Cyc_Buffer_resize(struct Cyc_Buffer_t*b,unsigned more){
unsigned long len=b->length;
unsigned long new_len=len == (unsigned long)0?1U: len;
struct _fat_ptr new_buffer;
while(b->position + more > new_len){
new_len=(unsigned long)2 * new_len;}
# 116
new_buffer=({unsigned _tmp20=new_len + (unsigned long)1;_tag_fat(_region_calloc(Cyc_Core_unique_region,sizeof(char),_tmp20),sizeof(char),_tmp20);});
if(b->length != (unsigned)0){
struct _fat_ptr _tmp21;_LL1: _tmp21=new_buffer;_LL2: {struct _fat_ptr x=_tmp21;
({({struct _fat_ptr _tmp44=_fat_ptr_decrease_size(x,sizeof(char),1U);struct _fat_ptr _tmp43=(struct _fat_ptr)b->buffer;Cyc_strncpy(_tmp44,_tmp43,b->position);});});}}
# 117
if(({
# 121
char*_tmp45=(char*)(b->initial_buffer).curr;_tmp45 == (char*)(_tag_fat(0,0,0)).curr;}))
({struct _fat_ptr _tmp22=b->initial_buffer;struct _fat_ptr _tmp23=b->buffer;b->initial_buffer=_tmp23;b->buffer=_tmp22;});
# 117
({struct _fat_ptr _tmp24=b->buffer;struct _fat_ptr _tmp25=new_buffer;b->buffer=_tmp25;new_buffer=_tmp24;});
# 124
({({void(*_tmp46)(char*ptr)=({void(*_tmp26)(char*ptr)=(void(*)(char*ptr))Cyc_Core_ufree;_tmp26;});_tmp46((char*)_untag_fat_ptr(new_buffer,sizeof(char),1U + 1U));});});
b->length=new_len;
return;}
# 129
void Cyc_Buffer_add_char(struct Cyc_Buffer_t*b,char c){
int pos=(int)b->position;
if((unsigned)pos >= b->length)({Cyc_Buffer_resize(b,1U);});({struct _fat_ptr _tmp28=_fat_ptr_plus(b->buffer,sizeof(char),pos);char _tmp29=*((char*)_check_fat_subscript(_tmp28,sizeof(char),0U));char _tmp2A=c;if(_get_fat_size(_tmp28,sizeof(char))== 1U &&(_tmp29 == 0 && _tmp2A != 0))_throw_arraybounds();*((char*)_tmp28.curr)=_tmp2A;});
# 133
b->position=(unsigned)(pos + 1);
return;}
# 140
void Cyc_Buffer_add_substring(struct Cyc_Buffer_t*b,struct _fat_ptr s,int offset,int len){
if((offset < 0 || len < 0)||(unsigned)(offset + len)> _get_fat_size(s,sizeof(char)))
(int)_throw(({struct Cyc_Core_Invalid_argument_exn_struct*_tmp2D=_cycalloc(sizeof(*_tmp2D));_tmp2D->tag=Cyc_Core_Invalid_argument,({struct _fat_ptr _tmp47=({const char*_tmp2C="Buffer::add_substring";_tag_fat(_tmp2C,sizeof(char),22U);});_tmp2D->f1=_tmp47;});_tmp2D;}));{
# 141
int new_position=(int)(b->position + (unsigned)len);
# 144
if((unsigned)new_position > b->length)({Cyc_Buffer_resize(b,(unsigned)len);});{struct _fat_ptr buf=
_tag_fat(0,0,0);
({struct _fat_ptr _tmp2E=buf;struct _fat_ptr _tmp2F=b->buffer;buf=_tmp2F;b->buffer=_tmp2E;});
{struct _fat_ptr _tmp30;_LL1: _tmp30=buf;_LL2: {struct _fat_ptr x=_tmp30;
({({struct _fat_ptr _tmp49=_fat_ptr_decrease_size(_fat_ptr_plus(x,sizeof(char),(int)b->position),sizeof(char),1U);struct _fat_ptr _tmp48=(struct _fat_ptr)_fat_ptr_plus(s,sizeof(char),offset);Cyc_zstrncpy(_tmp49,_tmp48,(unsigned long)len);});});}}
# 150
({struct _fat_ptr _tmp31=buf;struct _fat_ptr _tmp32=b->buffer;buf=_tmp32;b->buffer=_tmp31;});
b->position=(unsigned)new_position;
return;}}}
# 156
void Cyc_Buffer_add_string(struct Cyc_Buffer_t*b,struct _fat_ptr s){
int len=(int)({Cyc_strlen((struct _fat_ptr)s);});
int new_position=(int)(b->position + (unsigned)len);
if((unsigned)new_position > b->length)({Cyc_Buffer_resize(b,(unsigned)len);});{struct _fat_ptr buf=
_tag_fat(0,0,0);
({struct _fat_ptr _tmp34=buf;struct _fat_ptr _tmp35=b->buffer;buf=_tmp35;b->buffer=_tmp34;});
{struct _fat_ptr _tmp36;_LL1: _tmp36=buf;_LL2: {struct _fat_ptr x=_tmp36;
({({struct _fat_ptr _tmp4C=({struct _fat_ptr _tmp4A=_fat_ptr_decrease_size(x,sizeof(char),1U);_fat_ptr_plus(_tmp4A,sizeof(char),(int)b->position);});struct _fat_ptr _tmp4B=(struct _fat_ptr)s;Cyc_strncpy(_tmp4C,_tmp4B,(unsigned long)len);});});}}
# 165
({struct _fat_ptr _tmp37=buf;struct _fat_ptr _tmp38=b->buffer;buf=_tmp38;b->buffer=_tmp37;});
b->position=(unsigned)new_position;
return;}}
# 170
void Cyc_Buffer_add_buffer(struct Cyc_Buffer_t*b,struct Cyc_Buffer_t*bs){
({Cyc_Buffer_add_substring(b,(struct _fat_ptr)bs->buffer,0,(int)bs->position);});
return;}

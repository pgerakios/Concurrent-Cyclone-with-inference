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
void Cyc_Core_ufree(void*ptr);struct Cyc_Core_DynamicRegion;struct Cyc_Core_NewDynamicRegion{struct Cyc_Core_DynamicRegion*key;};struct Cyc_Core_ThinRes{void*arr;unsigned nelts;};struct Cyc_Xarray_Xarray{struct _fat_ptr elmts;int num_elmts;};
# 40 "xarray.h"
int Cyc_Xarray_length(struct Cyc_Xarray_Xarray*);
# 51
struct Cyc_Xarray_Xarray*Cyc_Xarray_rcreate(struct _RegionHandle*,int,void*);
# 57
struct Cyc_Xarray_Xarray*Cyc_Xarray_rcreate_empty(struct _RegionHandle*);
# 63
struct Cyc_Xarray_Xarray*Cyc_Xarray_rsingleton(struct _RegionHandle*,int,void*);
# 66
void Cyc_Xarray_add(struct Cyc_Xarray_Xarray*,void*);
# 75
struct _fat_ptr Cyc_Xarray_rto_array(struct _RegionHandle*,struct Cyc_Xarray_Xarray*);
# 81
struct Cyc_Xarray_Xarray*Cyc_Xarray_rfrom_array(struct _RegionHandle*,struct _fat_ptr arr);
# 89
struct Cyc_Xarray_Xarray*Cyc_Xarray_rappend(struct _RegionHandle*,struct Cyc_Xarray_Xarray*,struct Cyc_Xarray_Xarray*);
# 110
struct Cyc_Xarray_Xarray*Cyc_Xarray_rmap(struct _RegionHandle*,void*(*f)(void*),struct Cyc_Xarray_Xarray*);
# 117
struct Cyc_Xarray_Xarray*Cyc_Xarray_rmap_c(struct _RegionHandle*,void*(*f)(void*,void*),void*,struct Cyc_Xarray_Xarray*);
# 25 "xarray.cyc"
int Cyc_Xarray_length(struct Cyc_Xarray_Xarray*xarr){
return xarr->num_elmts;}
# 29
void*Cyc_Xarray_get(struct Cyc_Xarray_Xarray*xarr,int i){
if(i < 0 || i >= xarr->num_elmts)
(int)_throw(({struct Cyc_Core_Invalid_argument_exn_struct*_tmp2=_cycalloc(sizeof(*_tmp2));_tmp2->tag=Cyc_Core_Invalid_argument,({struct _fat_ptr _tmp4E=({const char*_tmp1="Xarray::get: bad index";_tag_fat(_tmp1,sizeof(char),23U);});_tmp2->f1=_tmp4E;});_tmp2;}));
# 30
return*((void**)_check_fat_subscript(xarr->elmts,sizeof(void*),i));}
# 35
void Cyc_Xarray_set(struct Cyc_Xarray_Xarray*xarr,int i,void*a){
if(i < 0 || i >= xarr->num_elmts)
(int)_throw(({struct Cyc_Core_Invalid_argument_exn_struct*_tmp5=_cycalloc(sizeof(*_tmp5));_tmp5->tag=Cyc_Core_Invalid_argument,({struct _fat_ptr _tmp4F=({const char*_tmp4="Xarray::set: bad index";_tag_fat(_tmp4,sizeof(char),23U);});_tmp5->f1=_tmp4F;});_tmp5;}));
# 36
*((void**)_check_fat_subscript(xarr->elmts,sizeof(void*),i))=a;}
# 41
struct Cyc_Xarray_Xarray*Cyc_Xarray_rcreate_empty(struct _RegionHandle*r){
struct _fat_ptr x=_tag_fat(0,0,0);
return({struct Cyc_Xarray_Xarray*_tmp7=_region_malloc(r,sizeof(*_tmp7));_tmp7->elmts=x,_tmp7->num_elmts=0;_tmp7;});}
# 45
struct Cyc_Xarray_Xarray*Cyc_Xarray_create_empty(){return({Cyc_Xarray_rcreate_empty(Cyc_Core_heap_region);});}
# 48
struct Cyc_Xarray_Xarray*Cyc_Xarray_rcreate(struct _RegionHandle*r,int len,void*a){
if(len < 0)
(int)_throw(({struct Cyc_Core_Invalid_argument_exn_struct*_tmpB=_cycalloc(sizeof(*_tmpB));_tmpB->tag=Cyc_Core_Invalid_argument,({struct _fat_ptr _tmp50=({const char*_tmpA="xarrays must have a non-negative size buffer";_tag_fat(_tmpA,sizeof(char),45U);});_tmpB->f1=_tmp50;});_tmpB;}));
# 49
return({struct Cyc_Xarray_Xarray*_tmpE=_region_malloc(r,sizeof(*_tmpE));
# 51
({struct _fat_ptr _tmp52=({unsigned _tmpD=(unsigned)len;void**_tmpC=({struct _RegionHandle*_tmp51=Cyc_Core_unique_region;_region_malloc(_tmp51,_check_times(_tmpD,sizeof(void*)));});({{unsigned _tmp46=(unsigned)len;unsigned i;for(i=0;i < _tmp46;++ i){_tmpC[i]=a;}}0;});_tag_fat(_tmpC,sizeof(void*),_tmpD);});_tmpE->elmts=_tmp52;}),_tmpE->num_elmts=0;_tmpE;});}
# 54
struct Cyc_Xarray_Xarray*Cyc_Xarray_create(int len,void*a){
return({Cyc_Xarray_rcreate(Cyc_Core_heap_region,len,a);});}
# 58
struct Cyc_Xarray_Xarray*Cyc_Xarray_rsingleton(struct _RegionHandle*r,int len,void*a){
if(len < 1)(int)_throw(({struct Cyc_Core_Invalid_argument_exn_struct*_tmp12=_cycalloc(sizeof(*_tmp12));_tmp12->tag=Cyc_Core_Invalid_argument,({struct _fat_ptr _tmp53=({const char*_tmp11="singleton xarray must have size >=1";_tag_fat(_tmp11,sizeof(char),36U);});_tmp12->f1=_tmp53;});_tmp12;}));{struct Cyc_Xarray_Xarray*x=({Cyc_Xarray_rcreate(r,len,a);});
# 61
x->num_elmts=1;
return x;}}
# 65
struct Cyc_Xarray_Xarray*Cyc_Xarray_singleton(int len,void*a){
return({Cyc_Xarray_rsingleton(Cyc_Core_heap_region,len,a);});}
# 69
void Cyc_Xarray_add(struct Cyc_Xarray_Xarray*xarr,void*a){
if((unsigned)xarr->num_elmts == _get_fat_size(xarr->elmts,sizeof(void*))){
if(xarr->num_elmts == 0)
({struct _fat_ptr _tmp55=_tag_fat(({unsigned _tmp16=10U;void**_tmp15=({struct _RegionHandle*_tmp54=Cyc_Core_unique_region;_region_malloc(_tmp54,_check_times(_tmp16,sizeof(void*)));});({{unsigned _tmp47=10U;unsigned i;for(i=0;i < _tmp47;++ i){_tmp15[i]=a;}}0;});_tmp15;}),sizeof(void*),10U);xarr->elmts=_tmp55;});else{
# 74
struct _fat_ptr newarr=({unsigned _tmp1B=(unsigned)(xarr->num_elmts * 2);void**_tmp1A=({struct _RegionHandle*_tmp56=Cyc_Core_unique_region;_region_malloc(_tmp56,_check_times(_tmp1B,sizeof(void*)));});({{unsigned _tmp48=(unsigned)(xarr->num_elmts * 2);unsigned i;for(i=0;i < _tmp48;++ i){_tmp1A[i]=*((void**)_check_fat_subscript(xarr->elmts,sizeof(void*),0));}}0;});_tag_fat(_tmp1A,sizeof(void*),_tmp1B);});
{int i=1;for(0;i < xarr->num_elmts;++ i){
({void*_tmp57=*((void**)_check_fat_subscript(xarr->elmts,sizeof(void*),i));*((void**)_check_fat_subscript(newarr,sizeof(void*),i))=_tmp57;});}}
({struct _fat_ptr _tmp17=xarr->elmts;struct _fat_ptr _tmp18=newarr;xarr->elmts=_tmp18;newarr=_tmp17;});
({({void(*_tmp58)(void**ptr)=({void(*_tmp19)(void**ptr)=(void(*)(void**ptr))Cyc_Core_ufree;_tmp19;});_tmp58((void**)_untag_fat_ptr(newarr,sizeof(void*),1U));});});}}
# 70
*((void**)_check_fat_subscript(xarr->elmts,sizeof(void*),xarr->num_elmts ++))=a;}
# 84
int Cyc_Xarray_add_ind(struct Cyc_Xarray_Xarray*xarr,void*a){
({Cyc_Xarray_add(xarr,a);});
return xarr->num_elmts - 1;}
# 89
struct _fat_ptr Cyc_Xarray_rto_array(struct _RegionHandle*r,struct Cyc_Xarray_Xarray*xarr){
if(xarr->num_elmts == 0)
return _tag_fat(({unsigned _tmp1F=0;void**_tmp1E=({struct _RegionHandle*_tmp59=r;_region_malloc(_tmp59,_check_times(_tmp1F,sizeof(void*)));});*_tmp1E=0;_tmp1E;}),sizeof(void*),0U);{
# 90
struct _fat_ptr ans=({unsigned _tmp21=(unsigned)xarr->num_elmts;void**_tmp20=({struct _RegionHandle*_tmp5A=r;_region_malloc(_tmp5A,_check_times(_tmp21,sizeof(void*)));});({{unsigned _tmp49=(unsigned)xarr->num_elmts;unsigned i;for(i=0;i < _tmp49;++ i){_tmp20[i]=*((void**)_check_fat_subscript(xarr->elmts,sizeof(void*),(int)i));}}0;});_tag_fat(_tmp20,sizeof(void*),_tmp21);});
# 93
return ans;}}
# 96
struct _fat_ptr Cyc_Xarray_to_array(struct Cyc_Xarray_Xarray*xarr){
return({Cyc_Xarray_rto_array(Cyc_Core_heap_region,xarr);});}
# 100
struct Cyc_Xarray_Xarray*Cyc_Xarray_rfrom_array(struct _RegionHandle*r,struct _fat_ptr arr){
if(_get_fat_size(arr,sizeof(void*))== (unsigned)0)
return({Cyc_Xarray_rcreate_empty(r);});{
# 101
struct Cyc_Xarray_Xarray*ans=({struct Cyc_Xarray_Xarray*_tmp26=_region_malloc(r,sizeof(*_tmp26));
# 103
({
struct _fat_ptr _tmp5C=({unsigned _tmp25=_get_fat_size(arr,sizeof(void*));void**_tmp24=({struct _RegionHandle*_tmp5B=Cyc_Core_unique_region;_region_malloc(_tmp5B,_check_times(_tmp25,sizeof(void*)));});({{unsigned _tmp4A=_get_fat_size(arr,sizeof(void*));unsigned i;for(i=0;i < _tmp4A;++ i){_tmp24[i]=((void**)arr.curr)[(int)i];}}0;});_tag_fat(_tmp24,sizeof(void*),_tmp25);});_tmp26->elmts=_tmp5C;}),_tmp26->num_elmts=(int)
_get_fat_size(arr,sizeof(void*));_tmp26;});
return ans;}}
# 109
struct Cyc_Xarray_Xarray*Cyc_Xarray_from_array(struct _fat_ptr arr){
return({Cyc_Xarray_rfrom_array(Cyc_Core_heap_region,arr);});}
# 114
struct Cyc_Xarray_Xarray*Cyc_Xarray_rappend(struct _RegionHandle*r,struct Cyc_Xarray_Xarray*xarr1,struct Cyc_Xarray_Xarray*xarr2){
int newsz=(int)(_get_fat_size(xarr1->elmts,sizeof(void*))+ _get_fat_size(xarr2->elmts,sizeof(void*)));
if(newsz == 0)
return({Cyc_Xarray_rcreate_empty(r);});{
# 116
void*init=
# 118
_get_fat_size(xarr1->elmts,sizeof(void*))== (unsigned)0?*((void**)_check_fat_subscript(xarr2->elmts,sizeof(void*),0)):*((void**)_check_fat_subscript(xarr1->elmts,sizeof(void*),0));
struct Cyc_Xarray_Xarray*ans=({struct Cyc_Xarray_Xarray*_tmp2B=_region_malloc(r,sizeof(*_tmp2B));({struct _fat_ptr _tmp5E=({unsigned _tmp2A=(unsigned)newsz;void**_tmp29=({struct _RegionHandle*_tmp5D=Cyc_Core_unique_region;_region_malloc(_tmp5D,_check_times(_tmp2A,sizeof(void*)));});({{unsigned _tmp4B=(unsigned)newsz;unsigned i;for(i=0;i < _tmp4B;++ i){_tmp29[i]=init;}}0;});_tag_fat(_tmp29,sizeof(void*),_tmp2A);});_tmp2B->elmts=_tmp5E;}),_tmp2B->num_elmts=0;_tmp2B;});
# 121
{int i=0;for(0;i < xarr1->num_elmts;++ i){
({Cyc_Xarray_add(ans,*((void**)_check_fat_subscript(xarr1->elmts,sizeof(void*),i)));});}}
{int i=0;for(0;i < xarr2->num_elmts;++ i){
({Cyc_Xarray_add(ans,*((void**)_check_fat_subscript(xarr2->elmts,sizeof(void*),i)));});}}
return ans;}}
# 128
struct Cyc_Xarray_Xarray*Cyc_Xarray_append(struct Cyc_Xarray_Xarray*xarr1,struct Cyc_Xarray_Xarray*xarr2){
return({Cyc_Xarray_rappend(Cyc_Core_heap_region,xarr1,xarr2);});}
# 132
void Cyc_Xarray_app(void*(*f)(void*),struct Cyc_Xarray_Xarray*xarr){
int i=0;for(0;i < xarr->num_elmts;++ i){
({f(*((void**)_check_fat_subscript(xarr->elmts,sizeof(void*),i)));});}}
# 137
void Cyc_Xarray_app_c(void*(*f)(void*,void*),void*env,struct Cyc_Xarray_Xarray*xarr){
int i=0;for(0;i < xarr->num_elmts;++ i){
({f(env,*((void**)_check_fat_subscript(xarr->elmts,sizeof(void*),i)));});}}
# 142
void Cyc_Xarray_iter(void(*f)(void*),struct Cyc_Xarray_Xarray*xarr){
int i=0;for(0;i < xarr->num_elmts;++ i){
({f(*((void**)_check_fat_subscript(xarr->elmts,sizeof(void*),i)));});}}
# 147
void Cyc_Xarray_iter_c(void(*f)(void*,void*),void*env,struct Cyc_Xarray_Xarray*xarr){
int i=0;for(0;i < xarr->num_elmts;++ i){
({f(env,*((void**)_check_fat_subscript(xarr->elmts,sizeof(void*),i)));});}}
# 152
struct Cyc_Xarray_Xarray*Cyc_Xarray_rmap(struct _RegionHandle*r,void*(*f)(void*),struct Cyc_Xarray_Xarray*xarr){
if(xarr->num_elmts == 0)return({Cyc_Xarray_rcreate_empty(r);});{struct Cyc_Xarray_Xarray*ans=({struct Cyc_Xarray_Xarray*_tmp34=_region_malloc(r,sizeof(*_tmp34));
# 156
({struct _fat_ptr _tmp61=({unsigned _tmp33=_get_fat_size(xarr->elmts,sizeof(void*));void**_tmp32=({struct _RegionHandle*_tmp5F=Cyc_Core_unique_region;_region_malloc(_tmp5F,_check_times(_tmp33,sizeof(void*)));});({{unsigned _tmp4C=_get_fat_size(xarr->elmts,sizeof(void*));unsigned i;for(i=0;i < _tmp4C;++ i){({void*_tmp60=({f(*((void**)_check_fat_subscript(xarr->elmts,sizeof(void*),0)));});_tmp32[i]=_tmp60;});}}0;});_tag_fat(_tmp32,sizeof(void*),_tmp33);});_tmp34->elmts=_tmp61;}),_tmp34->num_elmts=xarr->num_elmts;_tmp34;});
# 158
{int i=1;for(0;i < xarr->num_elmts;++ i){
({void*_tmp62=({f(*((void**)_check_fat_subscript(xarr->elmts,sizeof(void*),i)));});*((void**)_check_fat_subscript(ans->elmts,sizeof(void*),i))=_tmp62;});}}
return ans;}}
# 163
struct Cyc_Xarray_Xarray*Cyc_Xarray_map(void*(*f)(void*),struct Cyc_Xarray_Xarray*xarr){
return({Cyc_Xarray_rmap(Cyc_Core_heap_region,f,xarr);});}
# 167
struct Cyc_Xarray_Xarray*Cyc_Xarray_rmap_c(struct _RegionHandle*r,void*(*f)(void*,void*),void*env,struct Cyc_Xarray_Xarray*xarr){
if(xarr->num_elmts == 0)return({Cyc_Xarray_rcreate_empty(r);});{struct Cyc_Xarray_Xarray*ans=({struct Cyc_Xarray_Xarray*_tmp39=_region_malloc(r,sizeof(*_tmp39));
# 171
({struct _fat_ptr _tmp65=({unsigned _tmp38=_get_fat_size(xarr->elmts,sizeof(void*));void**_tmp37=({struct _RegionHandle*_tmp63=Cyc_Core_unique_region;_region_malloc(_tmp63,_check_times(_tmp38,sizeof(void*)));});({{unsigned _tmp4D=_get_fat_size(xarr->elmts,sizeof(void*));unsigned i;for(i=0;i < _tmp4D;++ i){({void*_tmp64=({f(env,*((void**)_check_fat_subscript(xarr->elmts,sizeof(void*),0)));});_tmp37[i]=_tmp64;});}}0;});_tag_fat(_tmp37,sizeof(void*),_tmp38);});_tmp39->elmts=_tmp65;}),_tmp39->num_elmts=xarr->num_elmts;_tmp39;});
# 173
{int i=1;for(0;i < xarr->num_elmts;++ i){
({void*_tmp66=({f(env,*((void**)_check_fat_subscript(xarr->elmts,sizeof(void*),i)));});*((void**)_check_fat_subscript(ans->elmts,sizeof(void*),i))=_tmp66;});}}
return ans;}}
# 178
struct Cyc_Xarray_Xarray*Cyc_Xarray_map_c(void*(*f)(void*,void*),void*env,struct Cyc_Xarray_Xarray*xarr){
return({Cyc_Xarray_rmap_c(Cyc_Core_heap_region,f,env,xarr);});}
# 182
void Cyc_Xarray_reuse(struct Cyc_Xarray_Xarray*xarr){
struct _fat_ptr newarr=_tag_fat(0,0,0);
({struct _fat_ptr _tmp3C=newarr;struct _fat_ptr _tmp3D=xarr->elmts;newarr=_tmp3D;xarr->elmts=_tmp3C;});
xarr->num_elmts=0;
({({void(*_tmp67)(void**ptr)=({void(*_tmp3E)(void**ptr)=(void(*)(void**ptr))Cyc_Core_ufree;_tmp3E;});_tmp67((void**)_untag_fat_ptr(newarr,sizeof(void*),1U));});});}
# 189
void Cyc_Xarray_delete(struct Cyc_Xarray_Xarray*xarr,int num){
if(({int _tmp68=num;_tmp68 > ({Cyc_Xarray_length(xarr);});}))
(int)_throw(({struct Cyc_Core_Invalid_argument_exn_struct*_tmp41=_cycalloc(sizeof(*_tmp41));_tmp41->tag=Cyc_Core_Invalid_argument,({struct _fat_ptr _tmp69=({const char*_tmp40="number deleted is greater than length of xarray";_tag_fat(_tmp40,sizeof(char),48U);});_tmp41->f1=_tmp69;});_tmp41;}));
# 190
xarr->num_elmts -=num;}
# 195
void Cyc_Xarray_remove(struct Cyc_Xarray_Xarray*xarr,int i){
if(i < 0 || i > xarr->num_elmts - 1)
(int)_throw(({struct Cyc_Core_Invalid_argument_exn_struct*_tmp44=_cycalloc(sizeof(*_tmp44));_tmp44->tag=Cyc_Core_Invalid_argument,({struct _fat_ptr _tmp6A=({const char*_tmp43="xarray index out of bounds";_tag_fat(_tmp43,sizeof(char),27U);});_tmp44->f1=_tmp6A;});_tmp44;}));
# 196
{
# 198
int j=i;
# 196
for(0;j < xarr->num_elmts - 1;++ j){
# 199
({void*_tmp6B=*((void**)_check_fat_subscript(xarr->elmts,sizeof(void*),j + 1));*((void**)_check_fat_subscript(xarr->elmts,sizeof(void*),j))=_tmp6B;});}}
-- xarr->num_elmts;}

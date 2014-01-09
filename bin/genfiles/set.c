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
int Cyc_List_length(struct Cyc_List_List*x);extern char Cyc_List_List_mismatch[14U];struct Cyc_List_List_mismatch_exn_struct{char*tag;};
# 117
void Cyc_List_app(void*(*f)(void*),struct Cyc_List_List*x);
# 133
void Cyc_List_iter(void(*f)(void*),struct Cyc_List_List*x);
# 135
void Cyc_List_iter_c(void(*f)(void*,void*),void*env,struct Cyc_List_List*x);
# 210
struct Cyc_List_List*Cyc_List_merge_sort(int(*cmp)(void*,void*),struct Cyc_List_List*x);extern char Cyc_List_Nth[4U];struct Cyc_List_Nth_exn_struct{char*tag;};struct Cyc_Iter_Iter{void*env;int(*next)(void*env,void*dest);};struct Cyc_Set_Set;
# 100 "set.h"
int Cyc_Set_member(struct Cyc_Set_Set*s,void*elt);
# 106
int Cyc_Set_setcmp(struct Cyc_Set_Set*s1,struct Cyc_Set_Set*s2);extern char Cyc_Set_Absent[7U];struct Cyc_Set_Absent_exn_struct{char*tag;};struct Cyc_Set_Set{int(*cmp)(void*,void*);int cardinality;struct Cyc_List_List*nodes;};
# 39 "set.cyc"
struct Cyc_Set_Set*Cyc_Set_empty(int(*comp)(void*,void*)){
return({struct Cyc_Set_Set*_tmp0=_cycalloc(sizeof(*_tmp0));_tmp0->cmp=comp,_tmp0->cardinality=0,_tmp0->nodes=0;_tmp0;});}
# 42
struct Cyc_Set_Set*Cyc_Set_rempty(struct _RegionHandle*rgn,int(*comp)(void*,void*)){
return({struct Cyc_Set_Set*_tmp2=_region_malloc(rgn,sizeof(*_tmp2));_tmp2->cmp=comp,_tmp2->cardinality=0,_tmp2->nodes=0;_tmp2;});}
# 46
struct Cyc_Set_Set*Cyc_Set_singleton(int(*comp)(void*,void*),void*x){
return({struct Cyc_Set_Set*_tmp5=_cycalloc(sizeof(*_tmp5));_tmp5->cmp=comp,_tmp5->cardinality=1,({struct Cyc_List_List*_tmp3D=({struct Cyc_List_List*_tmp4=_cycalloc(sizeof(*_tmp4));_tmp4->hd=x,_tmp4->tl=0;_tmp4;});_tmp5->nodes=_tmp3D;});_tmp5;});}
# 50
int Cyc_Set_cardinality(struct Cyc_Set_Set*s){
return s->cardinality;}
# 50
int Cyc_Set_is_empty(struct Cyc_Set_Set*s){
# 56
return s->cardinality == 0;}
# 60
static int Cyc_Set_member_b(int(*cmp)(void*,void*),struct Cyc_List_List*n,void*elt){
while(n != 0){
int i=({cmp(elt,n->hd);});
if(i == 0)return 1;else{
if(i < 0)return 0;else{
n=n->tl;}}}
# 67
return 0;}
# 60
int Cyc_Set_member(struct Cyc_Set_Set*s,void*elt){
# 71
return({Cyc_Set_member_b(s->cmp,s->nodes,elt);});}
# 78
static struct Cyc_List_List*Cyc_Set_insert_b(struct _RegionHandle*rgn,int(*cmp)(void*,void*),struct Cyc_List_List*n,void*elt){
# 81
if(n == 0)
return({struct Cyc_List_List*_tmpB=_region_malloc(rgn,sizeof(*_tmpB));_tmpB->hd=elt,_tmpB->tl=0;_tmpB;});else{
# 84
int i=({cmp(elt,n->hd);});
if(i < 0)
return({struct Cyc_List_List*_tmpC=_region_malloc(rgn,sizeof(*_tmpC));_tmpC->hd=elt,_tmpC->tl=n;_tmpC;});else{
# 88
struct Cyc_List_List*result=({struct Cyc_List_List*_tmpF=_region_malloc(rgn,sizeof(*_tmpF));_tmpF->hd=n->hd,_tmpF->tl=0;_tmpF;});
struct Cyc_List_List*prev=result;
n=n->tl;
while(n != 0 &&(i=({cmp(n->hd,elt);}))< 0){
({struct Cyc_List_List*_tmp3E=({struct Cyc_List_List*_tmpD=_region_malloc(rgn,sizeof(*_tmpD));_tmpD->hd=n->hd,_tmpD->tl=0;_tmpD;});((struct Cyc_List_List*)_check_null(prev))->tl=_tmp3E;});
prev=prev->tl;
n=n->tl;}
# 96
({struct Cyc_List_List*_tmp3F=({struct Cyc_List_List*_tmpE=_region_malloc(rgn,sizeof(*_tmpE));_tmpE->hd=elt,_tmpE->tl=n;_tmpE;});((struct Cyc_List_List*)_check_null(prev))->tl=_tmp3F;});
return result;}}}
# 103
struct Cyc_Set_Set*Cyc_Set_insert(struct Cyc_Set_Set*s,void*elt){
if(({Cyc_Set_member(s,elt);}))return s;else{
return({struct Cyc_Set_Set*_tmp11=_cycalloc(sizeof(*_tmp11));_tmp11->cmp=s->cmp,_tmp11->cardinality=s->cardinality + 1,({
struct Cyc_List_List*_tmp40=({Cyc_Set_insert_b(Cyc_Core_heap_region,s->cmp,s->nodes,elt);});_tmp11->nodes=_tmp40;});_tmp11;});}}
# 108
struct Cyc_Set_Set*Cyc_Set_rinsert(struct _RegionHandle*rgn,struct Cyc_Set_Set*s,void*elt){
if(({Cyc_Set_member(s,elt);}))return s;else{
return({struct Cyc_Set_Set*_tmp13=_region_malloc(rgn,sizeof(*_tmp13));_tmp13->cmp=s->cmp,_tmp13->cardinality=s->cardinality + 1,({
struct Cyc_List_List*_tmp41=({Cyc_Set_insert_b(rgn,s->cmp,s->nodes,elt);});_tmp13->nodes=_tmp41;});_tmp13;});}}
# 117
static struct Cyc_List_List*Cyc_Set_imp_insert_b(struct _RegionHandle*rgn,int(*cmp)(void*,void*),struct Cyc_List_List*n,void*elt){
# 120
if(n == 0)
return({struct Cyc_List_List*_tmp15=_region_malloc(rgn,sizeof(*_tmp15));_tmp15->hd=elt,_tmp15->tl=0;_tmp15;});else{
# 123
int i=({cmp(elt,n->hd);});
if(i < 0)
return({struct Cyc_List_List*_tmp16=_region_malloc(rgn,sizeof(*_tmp16));_tmp16->hd=elt,_tmp16->tl=n;_tmp16;});else{
# 127
struct Cyc_List_List*prev=n;struct Cyc_List_List*res=n;
n=n->tl;
while(n != 0 &&(i=({cmp(n->hd,elt);}))< 0){
prev=((struct Cyc_List_List*)_check_null(prev))->tl;
n=n->tl;}
# 133
({struct Cyc_List_List*_tmp42=({struct Cyc_List_List*_tmp17=_region_malloc(rgn,sizeof(*_tmp17));_tmp17->hd=elt,_tmp17->tl=n;_tmp17;});((struct Cyc_List_List*)_check_null(prev))->tl=_tmp42;});
return res;}}}
# 140
void Cyc_Set_imp_insert(struct Cyc_Set_Set*s,void*elt){
if(({Cyc_Set_member(s,elt);}))return;({struct Cyc_List_List*_tmp43=({Cyc_Set_imp_insert_b(Cyc_Core_heap_region,s->cmp,s->nodes,elt);});s->nodes=_tmp43;});
# 143
return;}
# 145
void Cyc_Set_imp_rinsert(struct _RegionHandle*rgn,struct Cyc_Set_Set*s,void*elt){
if(({Cyc_Set_member(s,elt);}))return;({struct Cyc_List_List*_tmp44=({Cyc_Set_imp_insert_b(rgn,s->cmp,s->nodes,elt);});s->nodes=_tmp44;});
# 148
return;}
# 152
struct Cyc_Set_Set*Cyc_Set_union_two(struct Cyc_Set_Set*s1,struct Cyc_Set_Set*s2){
if(s1 == s2)
return s1;
# 153
if(s1->cardinality == 0)
# 156
return s2;
# 153
if(s2->cardinality == 0)
# 158
return s1;{
# 153
struct Cyc_List_List*nodes=0;
# 161
int cardinality=0;
int(*comp)(void*,void*)=s1->cmp;
# 164
struct Cyc_List_List*x1=s1->nodes;
struct Cyc_List_List*x2=s2->nodes;
struct Cyc_List_List*curr=0;
# 168
while(x1 != 0 && x2 != 0){
int i=({comp(x1->hd,x2->hd);});
if(i == 0)
# 172
x2=x2->tl;else{
if(i < 0){
# 175
if(curr == 0){
nodes=({struct Cyc_List_List*_tmp1B=_cycalloc(sizeof(*_tmp1B));_tmp1B->hd=x1->hd,_tmp1B->tl=0;_tmp1B;});
curr=nodes;}else{
# 179
({struct Cyc_List_List*_tmp45=({struct Cyc_List_List*_tmp1C=_cycalloc(sizeof(*_tmp1C));_tmp1C->hd=x1->hd,_tmp1C->tl=0;_tmp1C;});curr->tl=_tmp45;});
curr=curr->tl;}
# 182
x1=x1->tl;
++ cardinality;}else{
# 186
if(curr == 0){
nodes=({struct Cyc_List_List*_tmp1D=_cycalloc(sizeof(*_tmp1D));_tmp1D->hd=x2->hd,_tmp1D->tl=0;_tmp1D;});
curr=nodes;}else{
# 190
({struct Cyc_List_List*_tmp46=({struct Cyc_List_List*_tmp1E=_cycalloc(sizeof(*_tmp1E));_tmp1E->hd=x2->hd,_tmp1E->tl=0;_tmp1E;});curr->tl=_tmp46;});
curr=curr->tl;}
# 193
x2=x2->tl;
++ cardinality;}}}
# 197
if(x1 != 0){
# 199
if(curr == 0)
nodes=x1;else{
# 202
curr->tl=x1;}
({int _tmp47=({Cyc_List_length(x1);});cardinality +=_tmp47;});}else{
if(x2 != 0){
# 206
if(curr == 0)
nodes=x2;else{
# 209
curr->tl=x2;}
({int _tmp48=({Cyc_List_length(x2);});cardinality +=_tmp48;});}}
# 197
return({struct Cyc_Set_Set*_tmp1F=_cycalloc(sizeof(*_tmp1F));
# 212
_tmp1F->cmp=comp,_tmp1F->cardinality=cardinality,_tmp1F->nodes=nodes;_tmp1F;});}}
# 218
static struct Cyc_List_List*Cyc_Set_delete_b(int(*cmp)(void*,void*),struct Cyc_List_List*n,void*elt){
if(({cmp(((struct Cyc_List_List*)_check_null(n))->hd,elt);})== 0)return n->tl;{struct Cyc_List_List*result=({struct Cyc_List_List*_tmp22=_cycalloc(sizeof(*_tmp22));
# 221
_tmp22->hd=n->hd,_tmp22->tl=0;_tmp22;});
struct Cyc_List_List*prev=result;
n=n->tl;
while(n != 0 &&({cmp(n->hd,elt);})!= 0){
({struct Cyc_List_List*_tmp49=({struct Cyc_List_List*_tmp21=_cycalloc(sizeof(*_tmp21));_tmp21->hd=n->hd,_tmp21->tl=0;_tmp21;});((struct Cyc_List_List*)_check_null(prev))->tl=_tmp49;});
prev=prev->tl;
n=n->tl;}
# 229
({struct Cyc_List_List*_tmp4A=((struct Cyc_List_List*)_check_null(n))->tl;((struct Cyc_List_List*)_check_null(prev))->tl=_tmp4A;});
return result;}}
# 234
struct Cyc_Set_Set*Cyc_Set_delete(struct Cyc_Set_Set*s,void*elt){
if(({Cyc_Set_member(s,elt);}))
return({struct Cyc_Set_Set*_tmp24=_cycalloc(sizeof(*_tmp24));_tmp24->cmp=s->cmp,_tmp24->cardinality=s->cardinality - 1,({
struct Cyc_List_List*_tmp4B=({Cyc_Set_delete_b(s->cmp,s->nodes,elt);});_tmp24->nodes=_tmp4B;});_tmp24;});else{
return s;}}
# 245
static struct Cyc_List_List*Cyc_Set_imp_delete_b(int(*cmp)(void*,void*),struct Cyc_List_List*n,void*elt,void**ret){
if(({cmp(((struct Cyc_List_List*)_check_null(n))->hd,elt);})== 0)return n->tl;{struct Cyc_List_List*prev=n;struct Cyc_List_List*res=n;
# 249
n=n->tl;
while(n != 0 &&({cmp(n->hd,elt);})!= 0){
prev=((struct Cyc_List_List*)_check_null(prev))->tl;
n=n->tl;}
# 254
({struct Cyc_List_List*_tmp4C=((struct Cyc_List_List*)_check_null(n))->tl;((struct Cyc_List_List*)_check_null(prev))->tl=_tmp4C;});
*ret=n->hd;
return res;}}
# 260
void*Cyc_Set_imp_delete(struct Cyc_Set_Set*s,void*elt){
void*ret=elt;
if(({Cyc_Set_member(s,elt);}))
({struct Cyc_List_List*_tmp4D=({Cyc_Set_imp_delete_b(s->cmp,s->nodes,elt,& ret);});s->nodes=_tmp4D;});
# 262
return ret;}
# 268
void*Cyc_Set_fold(void*(*f)(void*,void*),struct Cyc_Set_Set*s,void*accum){
struct Cyc_List_List*n=s->nodes;
# 271
while(n != 0){
accum=({f(n->hd,accum);});
n=n->tl;}
# 275
return accum;}
# 277
void*Cyc_Set_fold_c(void*(*f)(void*,void*,void*),void*env,struct Cyc_Set_Set*s,void*accum){
struct Cyc_List_List*n=s->nodes;
# 280
while(n != 0){
accum=({f(env,n->hd,accum);});
n=n->tl;}
# 284
return accum;}
# 289
void Cyc_Set_app(void*(*f)(void*),struct Cyc_Set_Set*s){
({Cyc_List_app(f,s->nodes);});}
# 292
void Cyc_Set_iter(void(*f)(void*),struct Cyc_Set_Set*s){
({Cyc_List_iter(f,s->nodes);});}
# 295
void Cyc_Set_iter_c(void(*f)(void*,void*),void*env,struct Cyc_Set_Set*s){
({Cyc_List_iter_c(f,env,s->nodes);});}
# 302
struct Cyc_Set_Set*Cyc_Set_intersect(struct Cyc_Set_Set*s1,struct Cyc_Set_Set*s2){
if(s1 == s2)
return s1;{
# 303
int(*comp)(void*,void*)=s1->cmp;
# 306
struct Cyc_List_List*x1=s1->nodes;
struct Cyc_List_List*x2=s2->nodes;
struct Cyc_List_List*result=0;struct Cyc_List_List*prev=0;
int card=0;
if(x1 == 0)
return s1;
# 310
if(x2 == 0)
# 313
return s2;
# 310
while(
# 315
x1 != 0 && x2 != 0){
int i=({comp(x1->hd,x2->hd);});
if(i == 0){
if(result == 0){
result=({struct Cyc_List_List*_tmp2D=_cycalloc(sizeof(*_tmp2D));_tmp2D->hd=x1->hd,_tmp2D->tl=0;_tmp2D;});
prev=result;}else{
# 322
({struct Cyc_List_List*_tmp4E=({struct Cyc_List_List*_tmp2E=_cycalloc(sizeof(*_tmp2E));_tmp2E->hd=x1->hd,_tmp2E->tl=0;_tmp2E;});((struct Cyc_List_List*)_check_null(prev))->tl=_tmp4E;});
prev=prev->tl;}
# 325
++ card;
x1=x1->tl;
x2=x2->tl;}else{
if(i < 0)
x1=x1->tl;else{
# 331
x2=x2->tl;}}}
# 334
return({struct Cyc_Set_Set*_tmp2F=_cycalloc(sizeof(*_tmp2F));_tmp2F->cmp=comp,_tmp2F->cardinality=card,_tmp2F->nodes=result;_tmp2F;});}}
# 337
struct Cyc_Set_Set*Cyc_Set_from_list(int(*comp)(void*,void*),struct Cyc_List_List*x){
struct Cyc_List_List*z=({Cyc_List_merge_sort(comp,x);});
# 340
{struct Cyc_List_List*y=z;for(0;y != 0;y=y->tl){
if(y->tl != 0 &&({comp(y->hd,((struct Cyc_List_List*)_check_null(y->tl))->hd);})== 0)
y->tl=((struct Cyc_List_List*)_check_null(y->tl))->tl;}}
# 344
return({struct Cyc_Set_Set*_tmp31=_cycalloc(sizeof(*_tmp31));_tmp31->cmp=comp,({int _tmp4F=({Cyc_List_length(z);});_tmp31->cardinality=_tmp4F;}),_tmp31->nodes=z;_tmp31;});}
# 337
int Cyc_Set_subset(struct Cyc_Set_Set*s1,struct Cyc_Set_Set*s2){
# 348
int(*comp)(void*,void*)=s1->cmp;
struct Cyc_List_List*x1=s1->nodes;
struct Cyc_List_List*x2=s2->nodes;
# 352
while(1){
if(x1 == 0)return 1;if(x2 == 0)
return 0;{
# 353
int i=({comp(x1->hd,x2->hd);});
# 356
if(i == 0){
x1=x1->tl;
x2=x2->tl;}else{
if(i > 0)
x2=x2->tl;else{
return 0;}}}}
# 363
return 1;}
# 366
struct Cyc_Set_Set*Cyc_Set_diff(struct Cyc_Set_Set*s1,struct Cyc_Set_Set*s2){
int(*comp)(void*,void*)=s1->cmp;
struct Cyc_List_List*x1=s1->nodes;
struct Cyc_List_List*x2=s2->nodes;
int card=s1->cardinality;
# 372
if(x2 == 0)return s1;while(x2 != 0){
# 375
void*elt=x2->hd;
# 377
if(({Cyc_Set_member_b(comp,x1,elt);})){
-- card;
x1=({Cyc_Set_delete_b(comp,x1,elt);});}
# 377
x2=x2->tl;}
# 383
return({struct Cyc_Set_Set*_tmp34=_cycalloc(sizeof(*_tmp34));_tmp34->cmp=comp,_tmp34->cardinality=card,_tmp34->nodes=x1;_tmp34;});}
# 386
int Cyc_Set_setcmp(struct Cyc_Set_Set*s1,struct Cyc_Set_Set*s2){
# 389
if(s1->cardinality != s2->cardinality)return s1->cardinality - s2->cardinality;{struct Cyc_List_List*x1=s1->nodes;
# 392
struct Cyc_List_List*x2=s2->nodes;
int(*cmp)(void*,void*)=s1->cmp;
while(x1 != 0){
int diff=({cmp(x1->hd,((struct Cyc_List_List*)_check_null(x2))->hd);});
if(diff != 0)return diff;x1=x1->tl;
# 398
x2=x2->tl;}
# 400
return 0;}}
# 386
int Cyc_Set_equals(struct Cyc_Set_Set*s1,struct Cyc_Set_Set*s2){
# 404
return({Cyc_Set_setcmp(s1,s2);})== 0;}char Cyc_Set_Absent[7U]="Absent";
# 408
struct Cyc_Set_Absent_exn_struct Cyc_Set_Absent_val={Cyc_Set_Absent};
# 413
void*Cyc_Set_choose(struct Cyc_Set_Set*s){
if(s->nodes == 0)(int)_throw(& Cyc_Set_Absent_val);return((struct Cyc_List_List*)_check_null(s->nodes))->hd;}
# 413
int Cyc_Set_iter_f(struct Cyc_List_List**elts_left,void**dest){
# 419
if(!((unsigned)*elts_left))
return 0;
# 419
*dest=((struct Cyc_List_List*)_check_null(*elts_left))->hd;
# 422
*elts_left=((struct Cyc_List_List*)_check_null(*elts_left))->tl;
return 1;}
# 425
struct Cyc_Iter_Iter Cyc_Set_make_iter(struct _RegionHandle*rgn,struct Cyc_Set_Set*s){
# 428
return({struct Cyc_Iter_Iter _tmp3C;({struct Cyc_List_List**_tmp50=({struct Cyc_List_List**_tmp3A=_region_malloc(rgn,sizeof(*_tmp3A));*_tmp3A=s->nodes;_tmp3A;});_tmp3C.env=_tmp50;}),_tmp3C.next=Cyc_Set_iter_f;_tmp3C;});}

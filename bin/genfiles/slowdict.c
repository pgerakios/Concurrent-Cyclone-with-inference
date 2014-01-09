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
extern struct _RegionHandle*Cyc_Core_unique_region;struct Cyc_Core_DynamicRegion;struct Cyc_Core_NewDynamicRegion{struct Cyc_Core_DynamicRegion*key;};struct Cyc_Core_ThinRes{void*arr;unsigned nelts;};struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};extern char Cyc_List_List_mismatch[14U];struct Cyc_List_List_mismatch_exn_struct{char*tag;};extern char Cyc_List_Nth[4U];struct Cyc_List_Nth_exn_struct{char*tag;};struct Cyc_Splay_node;struct Cyc_Splay_noderef{struct Cyc_Splay_node*v;};struct Cyc_Splay_Leaf_Splay_tree_struct{int tag;int f1;};struct Cyc_Splay_Node_Splay_tree_struct{int tag;struct Cyc_Splay_noderef*f1;};struct Cyc_Splay_node{void*key;void*data;void*left;void*right;};
# 49 "splay.h"
int Cyc_Splay_splay(int(*f)(void*,void*),void*,void*);struct Cyc_SlowDict_Dict;extern char Cyc_SlowDict_Present[8U];struct Cyc_SlowDict_Present_exn_struct{char*tag;};extern char Cyc_SlowDict_Absent[7U];struct Cyc_SlowDict_Absent_exn_struct{char*tag;};
# 64 "slowdict.h"
struct Cyc_SlowDict_Dict*Cyc_SlowDict_insert(struct Cyc_SlowDict_Dict*d,void*k,void*v);
# 89
struct Cyc_SlowDict_Dict*Cyc_SlowDict_delete(struct Cyc_SlowDict_Dict*d,void*k);
# 98
void*Cyc_SlowDict_fold(void*(*f)(void*,void*,void*),struct Cyc_SlowDict_Dict*d,void*accum);char Cyc_SlowDict_Absent[7U]="Absent";char Cyc_SlowDict_Present[8U]="Present";
# 32 "slowdict.cyc"
struct Cyc_SlowDict_Absent_exn_struct Cyc_SlowDict_Absent_val={Cyc_SlowDict_Absent};
struct Cyc_SlowDict_Present_exn_struct Cyc_SlowDict_Present_val={Cyc_SlowDict_Present};struct Cyc_SlowDict_Dict{int(*reln)(void*,void*);void*tree;};
# 41
struct Cyc_SlowDict_Dict*Cyc_SlowDict_empty(int(*comp)(void*,void*)){
void*t=(void*)({struct Cyc_Splay_Leaf_Splay_tree_struct*_tmp1=_cycalloc(sizeof(*_tmp1));_tmp1->tag=0U,_tmp1->f1=0;_tmp1;});
return({struct Cyc_SlowDict_Dict*_tmp0=_cycalloc(sizeof(*_tmp0));_tmp0->reln=comp,_tmp0->tree=t;_tmp0;});}
# 41
int Cyc_SlowDict_is_empty(struct Cyc_SlowDict_Dict*d){
# 50
void*_stmttmp0=d->tree;void*_tmp3=_stmttmp0;if(((struct Cyc_Splay_Leaf_Splay_tree_struct*)_tmp3)->tag == 0U){_LL1: _LL2:
 return 1;}else{_LL3: _LL4:
 return 0;}_LL0:;}
# 41
int Cyc_SlowDict_member(struct Cyc_SlowDict_Dict*d,void*key){
# 57
return({Cyc_Splay_splay(d->reln,key,d->tree);});}
# 60
struct Cyc_SlowDict_Dict*Cyc_SlowDict_insert(struct Cyc_SlowDict_Dict*d,void*key,void*data){
void*leaf=(void*)({struct Cyc_Splay_Leaf_Splay_tree_struct*_tmp14=_cycalloc(sizeof(*_tmp14));_tmp14->tag=0U,_tmp14->f1=0;_tmp14;});
void*newleft=leaf;void*newright=leaf;
if(({Cyc_Splay_splay(d->reln,key,d->tree);})){
# 65
void*_stmttmp1=d->tree;void*_tmp6=_stmttmp1;struct Cyc_Splay_noderef*_tmp7;if(((struct Cyc_Splay_Node_Splay_tree_struct*)_tmp6)->tag == 1U){_LL1: _tmp7=((struct Cyc_Splay_Node_Splay_tree_struct*)_tmp6)->f1;_LL2: {struct Cyc_Splay_noderef*n=_tmp7;
# 67
newleft=(n->v)->left;
newright=(n->v)->right;
goto _LL0;}}else{_LL3: _LL4:
# 71
 goto _LL0;}_LL0:;}else{
# 75
void*_stmttmp2=d->tree;void*_tmp8=_stmttmp2;struct Cyc_Splay_noderef*_tmp9;if(((struct Cyc_Splay_Node_Splay_tree_struct*)_tmp8)->tag == 1U){_LL6: _tmp9=((struct Cyc_Splay_Node_Splay_tree_struct*)_tmp8)->f1;_LL7: {struct Cyc_Splay_noderef*nr=_tmp9;
# 77
struct Cyc_Splay_node*n=nr->v;
if(({(d->reln)(key,n->key);})< 0){
newleft=n->left;
newright=(void*)({struct Cyc_Splay_Node_Splay_tree_struct*_tmpC=_cycalloc(sizeof(*_tmpC));_tmpC->tag=1U,({struct Cyc_Splay_noderef*_tmp80=({struct Cyc_Splay_noderef*_tmpB=_cycalloc(sizeof(*_tmpB));({struct Cyc_Splay_node*_tmp7F=({struct Cyc_Splay_node*_tmpA=_cycalloc(sizeof(*_tmpA));
_tmpA->key=n->key,_tmpA->data=n->data,_tmpA->left=leaf,_tmpA->right=n->right;_tmpA;});
# 80
_tmpB->v=_tmp7F;});_tmpB;});_tmpC->f1=_tmp80;});_tmpC;});}else{
# 84
newleft=(void*)({struct Cyc_Splay_Node_Splay_tree_struct*_tmpF=_cycalloc(sizeof(*_tmpF));_tmpF->tag=1U,({struct Cyc_Splay_noderef*_tmp82=({struct Cyc_Splay_noderef*_tmpE=_cycalloc(sizeof(*_tmpE));({struct Cyc_Splay_node*_tmp81=({struct Cyc_Splay_node*_tmpD=_cycalloc(sizeof(*_tmpD));
_tmpD->key=n->key,_tmpD->data=n->data,_tmpD->left=n->left,_tmpD->right=leaf;_tmpD;});
# 84
_tmpE->v=_tmp81;});_tmpE;});_tmpF->f1=_tmp82;});_tmpF;});
# 86
newright=n->right;}
# 88
goto _LL5;}}else{_LL8: _LL9:
# 90
 goto _LL5;}_LL5:;}
# 93
return({struct Cyc_SlowDict_Dict*_tmp13=_cycalloc(sizeof(*_tmp13));_tmp13->reln=d->reln,({void*_tmp85=(void*)({struct Cyc_Splay_Node_Splay_tree_struct*_tmp12=_cycalloc(sizeof(*_tmp12));_tmp12->tag=1U,({struct Cyc_Splay_noderef*_tmp84=({struct Cyc_Splay_noderef*_tmp11=_cycalloc(sizeof(*_tmp11));({
struct Cyc_Splay_node*_tmp83=({struct Cyc_Splay_node*_tmp10=_cycalloc(sizeof(*_tmp10));_tmp10->key=key,_tmp10->data=data,_tmp10->left=newleft,_tmp10->right=newright;_tmp10;});_tmp11->v=_tmp83;});_tmp11;});
# 93
_tmp12->f1=_tmp84;});_tmp12;});_tmp13->tree=_tmp85;});_tmp13;});}
# 98
struct Cyc_SlowDict_Dict*Cyc_SlowDict_insert_new(struct Cyc_SlowDict_Dict*d,void*key,void*data){
# 100
if(({Cyc_Splay_splay(d->reln,key,d->tree);}))
(int)_throw(& Cyc_SlowDict_Present_val);
# 100
return({Cyc_SlowDict_insert(d,key,data);});}struct _tuple0{void*f1;void*f2;};
# 105
struct Cyc_SlowDict_Dict*Cyc_SlowDict_inserts(struct Cyc_SlowDict_Dict*d,struct Cyc_List_List*kds){
# 107
for(0;kds != 0;kds=kds->tl){
d=({Cyc_SlowDict_insert(d,(*((struct _tuple0*)kds->hd)).f1,(*((struct _tuple0*)kds->hd)).f2);});}
return d;}
# 112
struct Cyc_SlowDict_Dict*Cyc_SlowDict_singleton(int(*comp)(void*,void*),void*key,void*data){
struct Cyc_Splay_Leaf_Splay_tree_struct*leaf=({struct Cyc_Splay_Leaf_Splay_tree_struct*_tmp1C=_cycalloc(sizeof(*_tmp1C));_tmp1C->tag=0U,_tmp1C->f1=0;_tmp1C;});
return({struct Cyc_SlowDict_Dict*_tmp1B=_cycalloc(sizeof(*_tmp1B));_tmp1B->reln=comp,({void*_tmp88=(void*)({struct Cyc_Splay_Node_Splay_tree_struct*_tmp1A=_cycalloc(sizeof(*_tmp1A));_tmp1A->tag=1U,({struct Cyc_Splay_noderef*_tmp87=({struct Cyc_Splay_noderef*_tmp19=_cycalloc(sizeof(*_tmp19));({struct Cyc_Splay_node*_tmp86=({struct Cyc_Splay_node*_tmp18=_cycalloc(sizeof(*_tmp18));_tmp18->key=key,_tmp18->data=data,_tmp18->left=(void*)leaf,_tmp18->right=(void*)leaf;_tmp18;});_tmp19->v=_tmp86;});_tmp19;});_tmp1A->f1=_tmp87;});_tmp1A;});_tmp1B->tree=_tmp88;});_tmp1B;});}
# 117
void*Cyc_SlowDict_lookup(struct Cyc_SlowDict_Dict*d,void*key){
if(({Cyc_Splay_splay(d->reln,key,d->tree);})){
void*_stmttmp3=d->tree;void*_tmp1E=_stmttmp3;struct Cyc_Splay_noderef*_tmp1F;if(((struct Cyc_Splay_Node_Splay_tree_struct*)_tmp1E)->tag == 1U){_LL1: _tmp1F=((struct Cyc_Splay_Node_Splay_tree_struct*)_tmp1E)->f1;_LL2: {struct Cyc_Splay_noderef*nr=_tmp1F;
return(nr->v)->data;}}else{_LL3: _LL4:
(int)_throw(({struct Cyc_Core_Impossible_exn_struct*_tmp21=_cycalloc(sizeof(*_tmp21));_tmp21->tag=Cyc_Core_Impossible,({struct _fat_ptr _tmp89=({const char*_tmp20="Dict::lookup";_tag_fat(_tmp20,sizeof(char),13U);});_tmp21->f1=_tmp89;});_tmp21;}));}_LL0:;}
# 118
(int)_throw(& Cyc_SlowDict_Absent_val);}
# 126
struct Cyc_Core_Opt*Cyc_SlowDict_lookup_opt(struct Cyc_SlowDict_Dict*d,void*key){
if(({Cyc_Splay_splay(d->reln,key,d->tree);})){
void*_stmttmp4=d->tree;void*_tmp23=_stmttmp4;struct Cyc_Splay_noderef*_tmp24;if(((struct Cyc_Splay_Node_Splay_tree_struct*)_tmp23)->tag == 1U){_LL1: _tmp24=((struct Cyc_Splay_Node_Splay_tree_struct*)_tmp23)->f1;_LL2: {struct Cyc_Splay_noderef*nr=_tmp24;
return({struct Cyc_Core_Opt*_tmp25=_cycalloc(sizeof(*_tmp25));_tmp25->v=(nr->v)->data;_tmp25;});}}else{_LL3: _LL4:
(int)_throw(({struct Cyc_Core_Impossible_exn_struct*_tmp27=_cycalloc(sizeof(*_tmp27));_tmp27->tag=Cyc_Core_Impossible,({struct _fat_ptr _tmp8A=({const char*_tmp26="Dict::lookup";_tag_fat(_tmp26,sizeof(char),13U);});_tmp27->f1=_tmp8A;});_tmp27;}));}_LL0:;}
# 127
return 0;}
# 135
static int Cyc_SlowDict_get_largest(void*x,void*y){return 1;}
# 137
struct Cyc_SlowDict_Dict*Cyc_SlowDict_delete(struct Cyc_SlowDict_Dict*d,void*key){
if(({Cyc_Splay_splay(d->reln,key,d->tree);})){
void*_stmttmp5=d->tree;void*_tmp2A=_stmttmp5;struct Cyc_Splay_noderef*_tmp2B;if(((struct Cyc_Splay_Leaf_Splay_tree_struct*)_tmp2A)->tag == 0U){_LL1: _LL2:
(int)_throw(({struct Cyc_Core_Impossible_exn_struct*_tmp2D=_cycalloc(sizeof(*_tmp2D));_tmp2D->tag=Cyc_Core_Impossible,({struct _fat_ptr _tmp8B=({const char*_tmp2C="Dict::lookup";_tag_fat(_tmp2C,sizeof(char),13U);});_tmp2D->f1=_tmp8B;});_tmp2D;}));}else{_LL3: _tmp2B=((struct Cyc_Splay_Node_Splay_tree_struct*)_tmp2A)->f1;_LL4: {struct Cyc_Splay_noderef*nr=_tmp2B;
# 142
struct Cyc_Splay_node*n=nr->v;
void*_stmttmp6=n->left;void*_tmp2E=_stmttmp6;struct Cyc_Splay_noderef*_tmp2F;if(((struct Cyc_Splay_Leaf_Splay_tree_struct*)_tmp2E)->tag == 0U){_LL6: _LL7:
 return({struct Cyc_SlowDict_Dict*_tmp30=_cycalloc(sizeof(*_tmp30));_tmp30->reln=d->reln,_tmp30->tree=n->right;_tmp30;});}else{_LL8: _tmp2F=((struct Cyc_Splay_Node_Splay_tree_struct*)_tmp2E)->f1;_LL9: {struct Cyc_Splay_noderef*noderef_left=_tmp2F;
# 146
void*_stmttmp7=n->right;void*_tmp31=_stmttmp7;struct Cyc_Splay_noderef*_tmp32;if(((struct Cyc_Splay_Leaf_Splay_tree_struct*)_tmp31)->tag == 0U){_LLB: _LLC:
 return({struct Cyc_SlowDict_Dict*_tmp33=_cycalloc(sizeof(*_tmp33));_tmp33->reln=d->reln,_tmp33->tree=n->left;_tmp33;});}else{_LLD: _tmp32=((struct Cyc_Splay_Node_Splay_tree_struct*)_tmp31)->f1;_LLE: {struct Cyc_Splay_noderef*node_ref_right=_tmp32;
# 149
({Cyc_Splay_splay(Cyc_SlowDict_get_largest,key,n->left);});{
struct Cyc_Splay_node*newtop=noderef_left->v;
return({struct Cyc_SlowDict_Dict*_tmp37=_cycalloc(sizeof(*_tmp37));_tmp37->reln=d->reln,({
void*_tmp8E=(void*)({struct Cyc_Splay_Node_Splay_tree_struct*_tmp36=_cycalloc(sizeof(*_tmp36));_tmp36->tag=1U,({struct Cyc_Splay_noderef*_tmp8D=({struct Cyc_Splay_noderef*_tmp35=_cycalloc(sizeof(*_tmp35));({struct Cyc_Splay_node*_tmp8C=({struct Cyc_Splay_node*_tmp34=_cycalloc(sizeof(*_tmp34));_tmp34->key=newtop->key,_tmp34->data=newtop->data,_tmp34->left=newtop->left,_tmp34->right=n->right;_tmp34;});_tmp35->v=_tmp8C;});_tmp35;});_tmp36->f1=_tmp8D;});_tmp36;});_tmp37->tree=_tmp8E;});_tmp37;});}}}_LLA:;}}_LL5:;}}_LL0:;}else{
# 158
return d;}}
# 161
struct Cyc_SlowDict_Dict*Cyc_SlowDict_delete_present(struct Cyc_SlowDict_Dict*d,void*key){
struct Cyc_SlowDict_Dict*d2=({Cyc_SlowDict_delete(d,key);});
if(d == d2)
(int)_throw(& Cyc_SlowDict_Absent_val);
# 163
return d2;}
# 168
static void*Cyc_SlowDict_fold_tree(void*(*f)(void*,void*,void*),void*t,void*accum){
void*_tmp3A=t;struct Cyc_Splay_noderef*_tmp3B;if(((struct Cyc_Splay_Leaf_Splay_tree_struct*)_tmp3A)->tag == 0U){_LL1: _LL2:
# 171
 return accum;}else{_LL3: _tmp3B=((struct Cyc_Splay_Node_Splay_tree_struct*)_tmp3A)->f1;_LL4: {struct Cyc_Splay_noderef*nr=_tmp3B;
# 173
struct Cyc_Splay_node*n=nr->v;
return({void*(*_tmp3C)(void*,void*,void*)=f;void*_tmp3D=n->key;void*_tmp3E=n->data;void*_tmp3F=({void*(*_tmp40)(void*(*f)(void*,void*,void*),void*t,void*accum)=Cyc_SlowDict_fold_tree;void*(*_tmp41)(void*,void*,void*)=f;void*_tmp42=n->left;void*_tmp43=({Cyc_SlowDict_fold_tree(f,n->right,accum);});_tmp40(_tmp41,_tmp42,_tmp43);});_tmp3C(_tmp3D,_tmp3E,_tmp3F);});}}_LL0:;}
# 178
void*Cyc_SlowDict_fold(void*(*f)(void*,void*,void*),struct Cyc_SlowDict_Dict*d,void*accum){
return({Cyc_SlowDict_fold_tree(f,d->tree,accum);});}
# 182
static void*Cyc_SlowDict_fold_tree_c(void*(*f)(void*,void*,void*,void*),void*env,void*t,void*accum){
void*_tmp46=t;struct Cyc_Splay_noderef*_tmp47;if(((struct Cyc_Splay_Leaf_Splay_tree_struct*)_tmp46)->tag == 0U){_LL1: _LL2:
# 185
 return accum;}else{_LL3: _tmp47=((struct Cyc_Splay_Node_Splay_tree_struct*)_tmp46)->f1;_LL4: {struct Cyc_Splay_noderef*nr=_tmp47;
# 187
struct Cyc_Splay_node*n=nr->v;
return({void*(*_tmp48)(void*,void*,void*,void*)=f;void*_tmp49=env;void*_tmp4A=n->key;void*_tmp4B=n->data;void*_tmp4C=({void*(*_tmp4D)(void*(*f)(void*,void*,void*,void*),void*env,void*t,void*accum)=Cyc_SlowDict_fold_tree_c;void*(*_tmp4E)(void*,void*,void*,void*)=f;void*_tmp4F=env;void*_tmp50=n->left;void*_tmp51=({Cyc_SlowDict_fold_tree_c(f,env,n->right,accum);});_tmp4D(_tmp4E,_tmp4F,_tmp50,_tmp51);});_tmp48(_tmp49,_tmp4A,_tmp4B,_tmp4C);});}}_LL0:;}
# 192
void*Cyc_SlowDict_fold_c(void*(*f)(void*,void*,void*,void*),void*env,struct Cyc_SlowDict_Dict*dict,void*accum){
return({Cyc_SlowDict_fold_tree_c(f,env,dict->tree,accum);});}
# 196
static void Cyc_SlowDict_app_tree(void*(*f)(void*,void*),void*t){
void*_tmp54=t;struct Cyc_Splay_noderef*_tmp55;if(((struct Cyc_Splay_Leaf_Splay_tree_struct*)_tmp54)->tag == 0U){_LL1: _LL2:
 goto _LL0;}else{_LL3: _tmp55=((struct Cyc_Splay_Node_Splay_tree_struct*)_tmp54)->f1;_LL4: {struct Cyc_Splay_noderef*nr=_tmp55;
# 200
struct Cyc_Splay_node*n=nr->v;
({Cyc_SlowDict_app_tree(f,n->left);});
({f(n->key,n->data);});
({Cyc_SlowDict_app_tree(f,n->right);});
goto _LL0;}}_LL0:;}
# 207
void Cyc_SlowDict_app(void*(*f)(void*,void*),struct Cyc_SlowDict_Dict*d){
({Cyc_SlowDict_app_tree(f,d->tree);});}
# 211
static void Cyc_SlowDict_iter_tree(void(*f)(void*,void*),void*t){
void*_tmp58=t;struct Cyc_Splay_noderef*_tmp59;if(((struct Cyc_Splay_Leaf_Splay_tree_struct*)_tmp58)->tag == 0U){_LL1: _LL2:
 goto _LL0;}else{_LL3: _tmp59=((struct Cyc_Splay_Node_Splay_tree_struct*)_tmp58)->f1;_LL4: {struct Cyc_Splay_noderef*nr=_tmp59;
# 215
struct Cyc_Splay_node*n=nr->v;
({Cyc_SlowDict_iter_tree(f,n->left);});
({f(n->key,n->data);});
({Cyc_SlowDict_iter_tree(f,n->right);});
goto _LL0;}}_LL0:;}
# 222
void Cyc_SlowDict_iter(void(*f)(void*,void*),struct Cyc_SlowDict_Dict*d){
({Cyc_SlowDict_iter_tree(f,d->tree);});}
# 226
static void Cyc_SlowDict_app_tree_c(void*(*f)(void*,void*,void*),void*env,void*t){
void*_tmp5C=t;struct Cyc_Splay_noderef*_tmp5D;if(((struct Cyc_Splay_Leaf_Splay_tree_struct*)_tmp5C)->tag == 0U){_LL1: _LL2:
 goto _LL0;}else{_LL3: _tmp5D=((struct Cyc_Splay_Node_Splay_tree_struct*)_tmp5C)->f1;_LL4: {struct Cyc_Splay_noderef*nr=_tmp5D;
# 230
struct Cyc_Splay_node*n=nr->v;
({Cyc_SlowDict_app_tree_c(f,env,n->left);});
({f(env,n->key,n->data);});
({Cyc_SlowDict_app_tree_c(f,env,n->right);});
goto _LL0;}}_LL0:;}
# 237
void Cyc_SlowDict_app_c(void*(*f)(void*,void*,void*),void*env,struct Cyc_SlowDict_Dict*d){
({Cyc_SlowDict_app_tree_c(f,env,d->tree);});}
# 241
static void Cyc_SlowDict_iter_tree_c(void(*f)(void*,void*,void*),void*env,void*t){
void*_tmp60=t;struct Cyc_Splay_noderef*_tmp61;if(((struct Cyc_Splay_Leaf_Splay_tree_struct*)_tmp60)->tag == 0U){_LL1: _LL2:
 goto _LL0;}else{_LL3: _tmp61=((struct Cyc_Splay_Node_Splay_tree_struct*)_tmp60)->f1;_LL4: {struct Cyc_Splay_noderef*nr=_tmp61;
# 245
struct Cyc_Splay_node*n=nr->v;
({Cyc_SlowDict_iter_tree_c(f,env,n->left);});
({f(env,n->key,n->data);});
({Cyc_SlowDict_iter_tree_c(f,env,n->right);});
goto _LL0;}}_LL0:;}
# 252
void Cyc_SlowDict_iter_c(void(*f)(void*,void*,void*),void*env,struct Cyc_SlowDict_Dict*d){
({Cyc_SlowDict_iter_tree_c(f,env,d->tree);});}
# 256
static void*Cyc_SlowDict_map_tree(void*(*f)(void*),void*t){
void*_tmp64=t;struct Cyc_Splay_noderef*_tmp65;if(((struct Cyc_Splay_Leaf_Splay_tree_struct*)_tmp64)->tag == 0U){_LL1: _LL2:
 return(void*)({struct Cyc_Splay_Leaf_Splay_tree_struct*_tmp66=_cycalloc(sizeof(*_tmp66));_tmp66->tag=0U,_tmp66->f1=0;_tmp66;});}else{_LL3: _tmp65=((struct Cyc_Splay_Node_Splay_tree_struct*)_tmp64)->f1;_LL4: {struct Cyc_Splay_noderef*nr=_tmp65;
# 260
struct Cyc_Splay_node*n=nr->v;
return(void*)({struct Cyc_Splay_Node_Splay_tree_struct*_tmp69=_cycalloc(sizeof(*_tmp69));_tmp69->tag=1U,({struct Cyc_Splay_noderef*_tmp93=({struct Cyc_Splay_noderef*_tmp68=_cycalloc(sizeof(*_tmp68));({struct Cyc_Splay_node*_tmp92=({struct Cyc_Splay_node*_tmp67=_cycalloc(sizeof(*_tmp67));_tmp67->key=n->key,({
void*_tmp91=({f(n->data);});_tmp67->data=_tmp91;}),({
void*_tmp90=({Cyc_SlowDict_map_tree(f,n->left);});_tmp67->left=_tmp90;}),({
void*_tmp8F=({Cyc_SlowDict_map_tree(f,n->right);});_tmp67->right=_tmp8F;});_tmp67;});
# 261
_tmp68->v=_tmp92;});_tmp68;});_tmp69->f1=_tmp93;});_tmp69;});}}_LL0:;}
# 267
struct Cyc_SlowDict_Dict*Cyc_SlowDict_map(void*(*f)(void*),struct Cyc_SlowDict_Dict*d){
return({struct Cyc_SlowDict_Dict*_tmp6B=_cycalloc(sizeof(*_tmp6B));_tmp6B->reln=d->reln,({void*_tmp94=({Cyc_SlowDict_map_tree(f,d->tree);});_tmp6B->tree=_tmp94;});_tmp6B;});}
# 271
static void*Cyc_SlowDict_map_tree_c(void*(*f)(void*,void*),void*env,void*t){
void*_tmp6D=t;struct Cyc_Splay_noderef*_tmp6E;if(((struct Cyc_Splay_Leaf_Splay_tree_struct*)_tmp6D)->tag == 0U){_LL1: _LL2:
 return(void*)({struct Cyc_Splay_Leaf_Splay_tree_struct*_tmp6F=_cycalloc(sizeof(*_tmp6F));_tmp6F->tag=0U,_tmp6F->f1=0;_tmp6F;});}else{_LL3: _tmp6E=((struct Cyc_Splay_Node_Splay_tree_struct*)_tmp6D)->f1;_LL4: {struct Cyc_Splay_noderef*nr=_tmp6E;
# 275
struct Cyc_Splay_node*n=nr->v;
return(void*)({struct Cyc_Splay_Node_Splay_tree_struct*_tmp72=_cycalloc(sizeof(*_tmp72));_tmp72->tag=1U,({struct Cyc_Splay_noderef*_tmp99=({struct Cyc_Splay_noderef*_tmp71=_cycalloc(sizeof(*_tmp71));({struct Cyc_Splay_node*_tmp98=({struct Cyc_Splay_node*_tmp70=_cycalloc(sizeof(*_tmp70));_tmp70->key=n->key,({void*_tmp97=({f(env,n->data);});_tmp70->data=_tmp97;}),({
void*_tmp96=({Cyc_SlowDict_map_tree_c(f,env,n->left);});_tmp70->left=_tmp96;}),({
void*_tmp95=({Cyc_SlowDict_map_tree_c(f,env,n->right);});_tmp70->right=_tmp95;});_tmp70;});
# 276
_tmp71->v=_tmp98;});_tmp71;});_tmp72->f1=_tmp99;});_tmp72;});}}_LL0:;}
# 281
struct Cyc_SlowDict_Dict*Cyc_SlowDict_map_c(void*(*f)(void*,void*),void*env,struct Cyc_SlowDict_Dict*d){
return({struct Cyc_SlowDict_Dict*_tmp74=_cycalloc(sizeof(*_tmp74));_tmp74->reln=d->reln,({void*_tmp9A=({Cyc_SlowDict_map_tree_c(f,env,d->tree);});_tmp74->tree=_tmp9A;});_tmp74;});}
# 285
struct _tuple0*Cyc_SlowDict_choose(struct Cyc_SlowDict_Dict*d){
void*_stmttmp8=d->tree;void*_tmp76=_stmttmp8;struct Cyc_Splay_noderef*_tmp77;if(((struct Cyc_Splay_Leaf_Splay_tree_struct*)_tmp76)->tag == 0U){_LL1: _LL2:
(int)_throw(& Cyc_SlowDict_Absent_val);}else{_LL3: _tmp77=((struct Cyc_Splay_Node_Splay_tree_struct*)_tmp76)->f1;_LL4: {struct Cyc_Splay_noderef*nr=_tmp77;
return({struct _tuple0*_tmp78=_cycalloc(sizeof(*_tmp78));_tmp78->f1=(nr->v)->key,_tmp78->f2=(nr->v)->data;_tmp78;});}}_LL0:;}
# 292
struct Cyc_List_List*Cyc_SlowDict_to_list_f(void*k,void*v,struct Cyc_List_List*accum){
return({struct Cyc_List_List*_tmp7B=_cycalloc(sizeof(*_tmp7B));({struct _tuple0*_tmp9B=({struct _tuple0*_tmp7A=_cycalloc(sizeof(*_tmp7A));_tmp7A->f1=k,_tmp7A->f2=v;_tmp7A;});_tmp7B->hd=_tmp9B;}),_tmp7B->tl=accum;_tmp7B;});}
# 296
struct Cyc_List_List*Cyc_SlowDict_to_list(struct Cyc_SlowDict_Dict*d){
return({({struct Cyc_List_List*(*_tmp9C)(struct Cyc_List_List*(*f)(void*,void*,struct Cyc_List_List*),struct Cyc_SlowDict_Dict*d,struct Cyc_List_List*accum)=({struct Cyc_List_List*(*_tmp7D)(struct Cyc_List_List*(*f)(void*,void*,struct Cyc_List_List*),struct Cyc_SlowDict_Dict*d,struct Cyc_List_List*accum)=(struct Cyc_List_List*(*)(struct Cyc_List_List*(*f)(void*,void*,struct Cyc_List_List*),struct Cyc_SlowDict_Dict*d,struct Cyc_List_List*accum))Cyc_SlowDict_fold;_tmp7D;});_tmp9C(Cyc_SlowDict_to_list_f,d,0);});});}

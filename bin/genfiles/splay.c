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
void* _region_calloc(struct _RegionHandle*, unsigned t, unsigned n);
void   _free_region(struct _RegionHandle*);
struct _RegionHandle*_open_dynregion(struct _DynRegionFrame*,struct _DynRegionHandle*);
void   _pop_dynregion();

/* Exceptions */
struct _handler_cons {
  struct _RuntimeStack s;
  jmp_buf handler;
};
void _push_handler(struct _handler_cons *);
void _push_region(struct _RegionHandle *);
void _npop_handler(int);
void _pop_handler();
void _pop_region();

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
extern struct _RegionHandle*Cyc_Core_unique_region;struct Cyc_Core_DynamicRegion;struct Cyc_Core_NewDynamicRegion{struct Cyc_Core_DynamicRegion*key;};struct Cyc_Core_ThinRes{void*arr;unsigned nelts;};struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};extern char Cyc_List_List_mismatch[14U];struct Cyc_List_List_mismatch_exn_struct{char*tag;};extern char Cyc_List_Nth[4U];struct Cyc_List_Nth_exn_struct{char*tag;};struct Cyc_Splay_node;struct Cyc_Splay_noderef{struct Cyc_Splay_node*v;};struct Cyc_Splay_Leaf_Splay_tree_struct{int tag;int f1;};struct Cyc_Splay_Node_Splay_tree_struct{int tag;struct Cyc_Splay_noderef*f1;};struct Cyc_Splay_node{void*key;void*data;void*left;void*right;};
# 48 "splay.h"
int Cyc_Splay_rsplay(struct _RegionHandle*,int(*f)(void*,void*),void*,void*);
# 46 "splay.cyc"
enum Cyc_Splay_direction{Cyc_Splay_LEFT =0U,Cyc_Splay_RIGHT =1U};
# 50
static void Cyc_Splay_rotate_left(struct _RegionHandle*r,struct Cyc_Splay_noderef*nr){
struct Cyc_Splay_node*n=nr->v;
void*_stmttmp0=n->left;void*_tmp0=_stmttmp0;struct Cyc_Splay_noderef*_tmp1;if(((struct Cyc_Splay_Node_Splay_tree_struct*)_tmp0)->tag == 1U){_LL1: _tmp1=((struct Cyc_Splay_Node_Splay_tree_struct*)_tmp0)->f1;_LL2: {struct Cyc_Splay_noderef*nr2=_tmp1;
# 54
struct Cyc_Splay_node*n2=nr2->v;
struct Cyc_Splay_Node_Splay_tree_struct*t=({struct Cyc_Splay_Node_Splay_tree_struct*_tmp5=_region_malloc(r,sizeof(*_tmp5));_tmp5->tag=1U,({struct Cyc_Splay_noderef*_tmp24=({struct Cyc_Splay_noderef*_tmp4=_region_malloc(r,sizeof(*_tmp4));({struct Cyc_Splay_node*_tmp23=({struct Cyc_Splay_node*_tmp3=_region_malloc(r,sizeof(*_tmp3));_tmp3->key=n->key,_tmp3->data=n->data,_tmp3->left=n2->right,_tmp3->right=n->right;_tmp3;});_tmp4->v=_tmp23;});_tmp4;});_tmp5->f1=_tmp24;});_tmp5;});
({struct Cyc_Splay_node*_tmp25=({struct Cyc_Splay_node*_tmp2=_region_malloc(r,sizeof(*_tmp2));_tmp2->key=n2->key,_tmp2->data=n2->data,_tmp2->left=n2->left,_tmp2->right=(void*)t;_tmp2;});nr->v=_tmp25;});
goto _LL0;}}else{_LL3: _LL4:
# 59
(int)_throw((void*)({struct Cyc_Core_Invalid_argument_exn_struct*_tmp7=_cycalloc(sizeof(*_tmp7));_tmp7->tag=Cyc_Core_Invalid_argument,({struct _fat_ptr _tmp26=({const char*_tmp6="Splay::rotate_left";_tag_fat(_tmp6,sizeof(char),19U);});_tmp7->f1=_tmp26;});_tmp7;}));}_LL0:;}
# 50
static void Cyc_Splay_rotate_right(struct _RegionHandle*r,struct Cyc_Splay_noderef*nr){
# 64
struct Cyc_Splay_node*n=nr->v;
void*_stmttmp1=n->right;void*_tmp8=_stmttmp1;struct Cyc_Splay_noderef*_tmp9;if(((struct Cyc_Splay_Node_Splay_tree_struct*)_tmp8)->tag == 1U){_LL1: _tmp9=((struct Cyc_Splay_Node_Splay_tree_struct*)_tmp8)->f1;_LL2: {struct Cyc_Splay_noderef*nr2=_tmp9;
# 67
struct Cyc_Splay_node*n2=nr2->v;
struct Cyc_Splay_Node_Splay_tree_struct*t=({struct Cyc_Splay_Node_Splay_tree_struct*_tmpD=_region_malloc(r,sizeof(*_tmpD));_tmpD->tag=1U,({struct Cyc_Splay_noderef*_tmp28=({struct Cyc_Splay_noderef*_tmpC=_region_malloc(r,sizeof(*_tmpC));({struct Cyc_Splay_node*_tmp27=({struct Cyc_Splay_node*_tmpB=_region_malloc(r,sizeof(*_tmpB));_tmpB->key=n->key,_tmpB->data=n->data,_tmpB->left=n->left,_tmpB->right=n2->left;_tmpB;});_tmpC->v=_tmp27;});_tmpC;});_tmpD->f1=_tmp28;});_tmpD;});
({struct Cyc_Splay_node*_tmp29=({struct Cyc_Splay_node*_tmpA=_region_malloc(r,sizeof(*_tmpA));_tmpA->key=n2->key,_tmpA->data=n2->data,_tmpA->left=(void*)t,_tmpA->right=n2->right;_tmpA;});nr->v=_tmp29;});
goto _LL0;}}else{_LL3: _LL4:
# 72
(int)_throw((void*)({struct Cyc_Core_Invalid_argument_exn_struct*_tmpF=_cycalloc(sizeof(*_tmpF));_tmpF->tag=Cyc_Core_Invalid_argument,({struct _fat_ptr _tmp2A=({const char*_tmpE="Splay::rotate_right";_tag_fat(_tmpE,sizeof(char),20U);});_tmpF->f1=_tmp2A;});_tmpF;}));}_LL0:;}struct _tuple0{enum Cyc_Splay_direction f1;struct Cyc_Splay_noderef*f2;};
# 50
static void Cyc_Splay_lift(struct _RegionHandle*r,struct Cyc_List_List*dnl){
# 109 "splay.cyc"
while(dnl != 0){
if(dnl->tl == 0){
{struct _tuple0*_stmttmp2=(struct _tuple0*)dnl->hd;struct _tuple0*_tmp10=_stmttmp2;struct Cyc_Splay_noderef*_tmp11;struct Cyc_Splay_noderef*_tmp12;if(((struct _tuple0*)_tmp10)->f1 == Cyc_Splay_LEFT){_LL1: _tmp12=_tmp10->f2;_LL2: {struct Cyc_Splay_noderef*nr=_tmp12;
Cyc_Splay_rotate_left(r,nr);goto _LL0;}}else{_LL3: _tmp11=_tmp10->f2;_LL4: {struct Cyc_Splay_noderef*nr=_tmp11;
Cyc_Splay_rotate_right(r,nr);goto _LL0;}}_LL0:;}
# 115
return;}{
# 110
struct _tuple0*_stmttmp3=(struct _tuple0*)dnl->hd;struct Cyc_Splay_noderef*_tmp14;enum Cyc_Splay_direction _tmp13;_LL6: _tmp13=_stmttmp3->f1;_tmp14=_stmttmp3->f2;_LL7: {enum Cyc_Splay_direction parent_dir=_tmp13;struct Cyc_Splay_noderef*parent=_tmp14;
# 118
struct _tuple0*_stmttmp4=(struct _tuple0*)((struct Cyc_List_List*)_check_null(dnl->tl))->hd;struct Cyc_Splay_noderef*_tmp16;enum Cyc_Splay_direction _tmp15;_LL9: _tmp15=_stmttmp4->f1;_tmp16=_stmttmp4->f2;_LLA: {enum Cyc_Splay_direction grandparent_dir=_tmp15;struct Cyc_Splay_noderef*grandparent=_tmp16;
dnl=((struct Cyc_List_List*)_check_null(dnl->tl))->tl;{
enum Cyc_Splay_direction _tmp17=parent_dir;if(_tmp17 == Cyc_Splay_LEFT){_LLC: _LLD:
# 122
{enum Cyc_Splay_direction _tmp18=grandparent_dir;if(_tmp18 == Cyc_Splay_LEFT){_LL11: _LL12:
 Cyc_Splay_rotate_left(r,grandparent);Cyc_Splay_rotate_left(r,grandparent);goto _LL10;}else{_LL13: _LL14:
 Cyc_Splay_rotate_left(r,parent);Cyc_Splay_rotate_right(r,grandparent);goto _LL10;}_LL10:;}
# 126
goto _LLB;}else{_LLE: _LLF:
# 128
{enum Cyc_Splay_direction _tmp19=grandparent_dir;if(_tmp19 == Cyc_Splay_LEFT){_LL16: _LL17:
 Cyc_Splay_rotate_right(r,parent);Cyc_Splay_rotate_left(r,grandparent);goto _LL15;}else{_LL18: _LL19:
 Cyc_Splay_rotate_right(r,grandparent);Cyc_Splay_rotate_right(r,grandparent);goto _LL15;}_LL15:;}
# 132
goto _LLB;}_LLB:;}}}}}}
# 50 "splay.cyc"
int Cyc_Splay_rsplay(struct _RegionHandle*r,int(*reln)(void*,void*),void*reln_first_arg,void*tree){
# 141 "splay.cyc"
struct _RegionHandle _tmp1A=_new_region("temp");struct _RegionHandle*temp=& _tmp1A;_push_region(temp);
{struct Cyc_List_List*path=0;
while(1){
void*_tmp1B=tree;struct Cyc_Splay_noderef*_tmp1C;if(((struct Cyc_Splay_Node_Splay_tree_struct*)_tmp1B)->tag == 1U){_LL1: _tmp1C=((struct Cyc_Splay_Node_Splay_tree_struct*)_tmp1B)->f1;_LL2: {struct Cyc_Splay_noderef*nr=_tmp1C;
# 146
struct Cyc_Splay_node*n=nr->v;
int comp=reln(reln_first_arg,n->key);
if(comp == 0){
Cyc_Splay_lift(r,path);{
int _tmp1D=1;_npop_handler(0U);return _tmp1D;}}else{
if(comp < 0){
path=({struct Cyc_List_List*_tmp1F=_region_malloc(temp,sizeof(*_tmp1F));({struct _tuple0*_tmp2B=({struct _tuple0*_tmp1E=_region_malloc(temp,sizeof(*_tmp1E));_tmp1E->f1=Cyc_Splay_LEFT,_tmp1E->f2=nr;_tmp1E;});_tmp1F->hd=_tmp2B;}),_tmp1F->tl=path;_tmp1F;});
tree=n->left;}else{
# 155
path=({struct Cyc_List_List*_tmp21=_region_malloc(temp,sizeof(*_tmp21));({struct _tuple0*_tmp2C=({struct _tuple0*_tmp20=_region_malloc(temp,sizeof(*_tmp20));_tmp20->f1=Cyc_Splay_RIGHT,_tmp20->f2=nr;_tmp20;});_tmp21->hd=_tmp2C;}),_tmp21->tl=path;_tmp21;});
tree=n->right;}}
# 158
goto _LL0;}}else{_LL3: _LL4:
# 160
 if(path != 0)
Cyc_Splay_lift(r,path->tl);{
# 160
int _tmp22=0;_npop_handler(0U);return _tmp22;}}_LL0:;}}
# 142
;_pop_region(temp);}
# 50 "splay.cyc"
int Cyc_Splay_splay(int(*reln)(void*,void*),void*reln_first_arg,void*tree){
# 170 "splay.cyc"
return Cyc_Splay_rsplay(Cyc_Core_heap_region,reln,reln_first_arg,tree);}

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
# 168 "core.h"
extern struct _RegionHandle*Cyc_Core_unique_region;struct Cyc_Core_DynamicRegion;struct Cyc_Core_NewDynamicRegion{struct Cyc_Core_DynamicRegion*key;};struct Cyc_Core_ThinRes{void*arr;unsigned nelts;};struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};extern char Cyc_List_List_mismatch[14U];struct Cyc_List_List_mismatch_exn_struct{char*tag;};extern char Cyc_List_Nth[4U];struct Cyc_List_Nth_exn_struct{char*tag;};
# 364 "list.h"
struct _fat_ptr Cyc_List_to_array(struct Cyc_List_List*x);
# 371
struct Cyc_List_List*Cyc_List_from_array(struct _fat_ptr arr);extern char Cyc_Array_Array_mismatch[15U];struct Cyc_Array_Array_mismatch_exn_struct{char*tag;};
# 59 "array.cyc"
void Cyc_Array_qsort(int(*less_eq)(void**,void**),struct _fat_ptr arr,int len){
# 62
int base_ofs=0;
# 64
void*temp;
int sp[40U];
int sp_ofs;
int i;int j;int limit_ofs;
# 69
if((base_ofs < 0 ||(unsigned)(base_ofs + len)> _get_fat_size(arr,sizeof(void*)))|| len < 0)
(int)_throw((void*)({struct Cyc_Core_Invalid_argument_exn_struct*_tmp1=_cycalloc(sizeof(*_tmp1));_tmp1->tag=Cyc_Core_Invalid_argument,({struct _fat_ptr _tmp25=({const char*_tmp0="Array::qsort";_tag_fat(_tmp0,sizeof(char),13U);});_tmp1->f1=_tmp25;});_tmp1;}));
# 69
limit_ofs=base_ofs + len;
# 73
sp_ofs=0;
# 75
for(0;1;0){
if(limit_ofs - base_ofs > 3){
# 78
temp=*((void**)_check_fat_subscript(arr,sizeof(void*),(limit_ofs - base_ofs)/ 2 + base_ofs));({void*_tmp26=*((void**)_check_fat_subscript(arr,sizeof(void*),base_ofs));*((void**)_check_fat_subscript(arr,sizeof(void*),(limit_ofs - base_ofs)/ 2 + base_ofs))=_tmp26;});((void**)arr.curr)[base_ofs]=temp;
i=base_ofs + 1;
j=limit_ofs - 1;
if(({int(*_tmp28)(void**,void**)=less_eq;void**_tmp27=(void**)_check_null(_untag_fat_ptr(_fat_ptr_plus(arr,sizeof(void*),i),sizeof(void*),1U));_tmp28(_tmp27,(void**)_check_null(_untag_fat_ptr(_fat_ptr_plus(arr,sizeof(void*),j),sizeof(void*),1U)));})> 0){
temp=*((void**)_check_fat_subscript(arr,sizeof(void*),i));((void**)arr.curr)[i]=*((void**)_check_fat_subscript(arr,sizeof(void*),j));((void**)arr.curr)[j]=temp;}
if(({int(*_tmp2A)(void**,void**)=less_eq;void**_tmp29=(void**)_check_null(_untag_fat_ptr(_fat_ptr_plus(arr,sizeof(void*),base_ofs),sizeof(void*),1U));_tmp2A(_tmp29,(void**)_check_null(_untag_fat_ptr(_fat_ptr_plus(arr,sizeof(void*),j),sizeof(void*),1U)));})> 0){
temp=((void**)arr.curr)[base_ofs];((void**)arr.curr)[base_ofs]=*((void**)_check_fat_subscript(arr,sizeof(void*),j));((void**)arr.curr)[j]=temp;}
if(({int(*_tmp2C)(void**,void**)=less_eq;void**_tmp2B=(void**)_check_null(_untag_fat_ptr(_fat_ptr_plus(arr,sizeof(void*),i),sizeof(void*),1U));_tmp2C(_tmp2B,(void**)_check_null(_untag_fat_ptr(_fat_ptr_plus(arr,sizeof(void*),base_ofs),sizeof(void*),1U)));})> 0){
temp=*((void**)_check_fat_subscript(arr,sizeof(void*),i));((void**)arr.curr)[i]=((void**)arr.curr)[base_ofs];((void**)arr.curr)[base_ofs]=temp;}
for(0;1;0){
do{
++ i;}while(({
int(*_tmp2E)(void**,void**)=less_eq;void**_tmp2D=(void**)_check_null(_untag_fat_ptr(_fat_ptr_plus(arr,sizeof(void*),i),sizeof(void*),1U));_tmp2E(_tmp2D,(void**)_check_null(_untag_fat_ptr(_fat_ptr_plus(arr,sizeof(void*),base_ofs),sizeof(void*),1U)));})< 0);
do{
-- j;}while(({
int(*_tmp30)(void**,void**)=less_eq;void**_tmp2F=(void**)_check_null(_untag_fat_ptr(_fat_ptr_plus(arr,sizeof(void*),j),sizeof(void*),1U));_tmp30(_tmp2F,(void**)_check_null(_untag_fat_ptr(_fat_ptr_plus(arr,sizeof(void*),base_ofs),sizeof(void*),1U)));})> 0);
if(i > j)
break;
temp=*((void**)_check_fat_subscript(arr,sizeof(void*),i));((void**)arr.curr)[i]=*((void**)_check_fat_subscript(arr,sizeof(void*),j));((void**)arr.curr)[j]=temp;}
# 98
temp=((void**)arr.curr)[base_ofs];((void**)arr.curr)[base_ofs]=*((void**)_check_fat_subscript(arr,sizeof(void*),j));((void**)arr.curr)[j]=temp;
# 100
if(j - base_ofs > limit_ofs - i){
*((int*)_check_known_subscript_notnull(sp,40U,sizeof(int),sp_ofs))=base_ofs;
*((int*)_check_known_subscript_notnull(sp,40U,sizeof(int),sp_ofs + 1))=j;
base_ofs=i;}else{
# 105
*((int*)_check_known_subscript_notnull(sp,40U,sizeof(int),sp_ofs))=i;
*((int*)_check_known_subscript_notnull(sp,40U,sizeof(int),sp_ofs + 1))=limit_ofs;
limit_ofs=j;}
# 109
sp_ofs +=2;}else{
# 111
for((j=base_ofs,i=j + 1);i < limit_ofs;(j=i,i ++)){
for(0;({int(*_tmp32)(void**,void**)=less_eq;void**_tmp31=(void**)_check_null(_untag_fat_ptr(_fat_ptr_plus(arr,sizeof(void*),j),sizeof(void*),1U));_tmp32(_tmp31,(void**)_check_null(_untag_fat_ptr(_fat_ptr_plus(_fat_ptr_plus(arr,sizeof(void*),j),sizeof(void*),1),sizeof(void*),1U)));})> 0;-- j){
temp=*((void**)_check_fat_subscript(arr,sizeof(void*),j));((void**)arr.curr)[j]=*((void**)_check_fat_subscript(arr,sizeof(void*),j + 1));*((void**)_check_fat_subscript(arr,sizeof(void*),j + 1))=temp;
if(j == base_ofs)
break;}}
# 117
if(sp_ofs != 0){
sp_ofs -=2;
base_ofs=*((int*)_check_known_subscript_notnull(sp,40U,sizeof(int),sp_ofs));
limit_ofs=*((int*)_check_known_subscript_notnull(sp,40U,sizeof(int),sp_ofs + 1));}else{
# 122
break;}}}}
# 59
void Cyc_Array_msort(int(*less_eq)(void**,void**),struct _fat_ptr arr,int len){
# 142 "array.cyc"
if((unsigned)len > _get_fat_size(arr,sizeof(void*))|| len < 0)
(int)_throw((void*)({struct Cyc_Core_Invalid_argument_exn_struct*_tmp3=_cycalloc(sizeof(*_tmp3));_tmp3->tag=Cyc_Core_Invalid_argument,({struct _fat_ptr _tmp33=({const char*_tmp2="Array::msort";_tag_fat(_tmp2,sizeof(char),13U);});_tmp3->f1=_tmp33;});_tmp3;}));{
# 142
struct _fat_ptr from=({unsigned _tmp5=(unsigned)len;void**_tmp4=_cycalloc(_check_times(_tmp5,sizeof(void*)));({{unsigned _tmp1A=(unsigned)len;unsigned i;for(i=0;i < _tmp1A;++ i){_tmp4[i]=*((void**)_check_fat_subscript(arr,sizeof(void*),0));}}0;});_tag_fat(_tmp4,sizeof(void*),_tmp5);});
# 146
struct _fat_ptr to=arr;
struct _fat_ptr swap;
# 149
int swapped=0;
# 151
int stepsize;
# 153
int start;
# 155
int lstart;int lend;int rstart;int rend;
# 158
int dest;
# 162
for(stepsize=1;stepsize < len;stepsize=stepsize * 2){
swap=from;
from=to;
to=swap;
dest=0;
# 168
for(start=0;start < len;start=start + stepsize * 2){
lstart=start;
rstart=start + stepsize < len?start + stepsize: len;
lend=rstart;
rend=start + stepsize * 2 < len?start + stepsize * 2: len;
# 174
while(lstart < lend && rstart < rend){
if(({int(*_tmp35)(void**,void**)=less_eq;void**_tmp34=(void**)_check_null(_untag_fat_ptr(_fat_ptr_plus(from,sizeof(void*),lstart),sizeof(void*),1U));_tmp35(_tmp34,(void**)_check_null(_untag_fat_ptr(_fat_ptr_plus(from,sizeof(void*),rstart),sizeof(void*),1U)));})<= 0)
({void*_tmp36=*((void**)_check_fat_subscript(from,sizeof(void*),lstart ++));*((void**)_check_fat_subscript(to,sizeof(void*),dest ++))=_tmp36;});else{
# 178
({void*_tmp37=*((void**)_check_fat_subscript(from,sizeof(void*),rstart ++));*((void**)_check_fat_subscript(to,sizeof(void*),dest ++))=_tmp37;});}}
# 183
while(lstart < lend){
({void*_tmp38=*((void**)_check_fat_subscript(from,sizeof(void*),lstart ++));*((void**)_check_fat_subscript(to,sizeof(void*),dest ++))=_tmp38;});}
# 186
while(rstart < rend){
({void*_tmp39=*((void**)_check_fat_subscript(from,sizeof(void*),rstart ++));*((void**)_check_fat_subscript(to,sizeof(void*),dest ++))=_tmp39;});}}}
# 192
if(swapped){
int i=0;for(0;i < len;++ i){({void*_tmp3A=*((void**)_check_fat_subscript(to,sizeof(void*),i));*((void**)_check_fat_subscript(from,sizeof(void*),i))=_tmp3A;});}}}}
# 59 "array.cyc"
struct _fat_ptr Cyc_Array_from_list(struct Cyc_List_List*x){
# 207 "array.cyc"
return Cyc_List_to_array(x);}
# 59 "array.cyc"
struct Cyc_List_List*Cyc_Array_to_list(struct _fat_ptr x){
# 211 "array.cyc"
return Cyc_List_from_array(x);}
# 59 "array.cyc"
struct _fat_ptr Cyc_Array_copy(struct _fat_ptr x){
# 216 "array.cyc"
int sx=(int)_get_fat_size(x,sizeof(void*));
return({unsigned _tmp7=(unsigned)sx;void**_tmp6=_cycalloc(_check_times(_tmp7,sizeof(void*)));({{unsigned _tmp1B=(unsigned)sx;unsigned i;for(i=0;i < _tmp1B;++ i){_tmp6[i]=((void**)x.curr)[(int)i];}}0;});_tag_fat(_tmp6,sizeof(void*),_tmp7);});}
# 59 "array.cyc"
struct _fat_ptr Cyc_Array_map(void*(*f)(void*),struct _fat_ptr x){
# 222 "array.cyc"
int sx=(int)_get_fat_size(x,sizeof(void*));
return({unsigned _tmp9=(unsigned)sx;void**_tmp8=_cycalloc(_check_times(_tmp9,sizeof(void*)));({{unsigned _tmp1C=(unsigned)sx;unsigned i;for(i=0;i < _tmp1C;++ i){({void*_tmp3B=f(((void**)x.curr)[(int)i]);_tmp8[i]=_tmp3B;});}}0;});_tag_fat(_tmp8,sizeof(void*),_tmp9);});}
# 59 "array.cyc"
struct _fat_ptr Cyc_Array_map_c(void*(*f)(void*,void*),void*env,struct _fat_ptr x){
# 228 "array.cyc"
int sx=(int)_get_fat_size(x,sizeof(void*));
return({unsigned _tmpB=(unsigned)sx;void**_tmpA=_cycalloc(_check_times(_tmpB,sizeof(void*)));({{unsigned _tmp1D=(unsigned)sx;unsigned i;for(i=0;i < _tmp1D;++ i){({void*_tmp3C=f(env,((void**)x.curr)[(int)i]);_tmpA[i]=_tmp3C;});}}0;});_tag_fat(_tmpA,sizeof(void*),_tmpB);});}
# 59 "array.cyc"
void Cyc_Array_imp_map(void*(*f)(void*),struct _fat_ptr x){
# 235 "array.cyc"
int sx=(int)_get_fat_size(x,sizeof(void*));
int i=0;for(0;i < sx;++ i){({void*_tmp3D=f(*((void**)_check_fat_subscript(x,sizeof(void*),i)));((void**)x.curr)[i]=_tmp3D;});}}
# 59 "array.cyc"
void Cyc_Array_imp_map_c(void*(*f)(void*,void*),void*env,struct _fat_ptr x){
# 239 "array.cyc"
int sx=(int)_get_fat_size(x,sizeof(void*));
int i=0;for(0;i < sx;++ i){({void*_tmp3E=f(env,*((void**)_check_fat_subscript(x,sizeof(void*),i)));((void**)x.curr)[i]=_tmp3E;});}}char Cyc_Array_Array_mismatch[15U]="Array_mismatch";
# 245
static struct Cyc_Array_Array_mismatch_exn_struct Cyc_Array_Array_mismatch_val={Cyc_Array_Array_mismatch};
# 250
struct _fat_ptr Cyc_Array_map2(void*(*f)(void*,void*),struct _fat_ptr x,struct _fat_ptr y){
int sx=(int)_get_fat_size(x,sizeof(void*));
if((unsigned)sx != _get_fat_size(y,sizeof(void*)))(int)_throw((void*)& Cyc_Array_Array_mismatch_val);return({unsigned _tmpD=(unsigned)sx;void**_tmpC=_cycalloc(_check_times(_tmpD,sizeof(void*)));({{unsigned _tmp1E=(unsigned)sx;unsigned i;for(i=0;i < _tmp1E;++ i){({
void*_tmp3F=f(((void**)x.curr)[(int)i],((void**)y.curr)[(int)i]);_tmpC[i]=_tmp3F;});}}0;});_tag_fat(_tmpC,sizeof(void*),_tmpD);});}
# 250
void Cyc_Array_app(void*(*f)(void*),struct _fat_ptr x){
# 262
int sx=(int)_get_fat_size(x,sizeof(void*));
int i=0;for(0;i < sx;++ i){f(*((void**)_check_fat_subscript(x,sizeof(void*),i)));}}
# 250
void Cyc_Array_app_c(void*(*f)(void*,void*),void*env,struct _fat_ptr x){
# 266
int sx=(int)_get_fat_size(x,sizeof(void*));
int i=0;for(0;i < sx;++ i){f(env,*((void**)_check_fat_subscript(x,sizeof(void*),i)));}}
# 250
void Cyc_Array_iter(void(*f)(void*),struct _fat_ptr x){
# 274
int sx=(int)_get_fat_size(x,sizeof(void*));
int i=0;for(0;i < sx;++ i){f(*((void**)_check_fat_subscript(x,sizeof(void*),i)));}}
# 250
void Cyc_Array_iter_c(void(*f)(void*,void*),void*env,struct _fat_ptr x){
# 280
int sx=(int)_get_fat_size(x,sizeof(void*));
int i=0;for(0;i < sx;++ i){f(env,*((void**)_check_fat_subscript(x,sizeof(void*),i)));}}
# 250
void Cyc_Array_app2(void*(*f)(void*,void*),struct _fat_ptr x,struct _fat_ptr y){
# 287
int sx=(int)_get_fat_size(x,sizeof(void*));
if((unsigned)sx != _get_fat_size(y,sizeof(void*)))(int)_throw((void*)& Cyc_Array_Array_mismatch_val);{
int i=0;
# 288
for(0;i < sx;++ i){
f(*((void**)_check_fat_subscript(x,sizeof(void*),i)),((void**)y.curr)[i]);}}}
# 250
void Cyc_Array_app2_c(void*(*f)(void*,void*,void*),void*env,struct _fat_ptr x,struct _fat_ptr y){
# 293
int sx=(int)_get_fat_size(x,sizeof(void*));
if((unsigned)sx != _get_fat_size(y,sizeof(void*)))(int)_throw((void*)& Cyc_Array_Array_mismatch_val);{
int i=0;
# 294
for(0;i < sx;++ i){
f(env,*((void**)_check_fat_subscript(x,sizeof(void*),i)),((void**)y.curr)[i]);}}}
# 250
void Cyc_Array_iter2(void(*f)(void*,void*),struct _fat_ptr x,struct _fat_ptr y){
# 301
int sx=(int)_get_fat_size(x,sizeof(void*));
if((unsigned)sx != _get_fat_size(y,sizeof(void*)))(int)_throw((void*)& Cyc_Array_Array_mismatch_val);{
int i=0;
# 302
for(0;i < sx;++ i){
f(*((void**)_check_fat_subscript(x,sizeof(void*),i)),((void**)y.curr)[i]);}}}
# 250
void Cyc_Array_iter2_c(void(*f)(void*,void*,void*),void*env,struct _fat_ptr x,struct _fat_ptr y){
# 307
int sx=(int)_get_fat_size(x,sizeof(void*));
if((unsigned)sx != _get_fat_size(y,sizeof(void*)))(int)_throw((void*)& Cyc_Array_Array_mismatch_val);{
int i=0;
# 308
for(0;i < sx;++ i){
f(env,*((void**)_check_fat_subscript(x,sizeof(void*),i)),((void**)y.curr)[i]);}}}
# 250
void*Cyc_Array_fold_left(void*(*f)(void*,void*),void*accum,struct _fat_ptr x){
# 318
int sx=(int)_get_fat_size(x,sizeof(void*));
{int i=0;for(0;i < sx;++ i){
accum=f(accum,*((void**)_check_fat_subscript(x,sizeof(void*),i)));}}
# 322
return accum;}
# 250
void*Cyc_Array_fold_left_c(void*(*f)(void*,void*,void*),void*env,void*accum,struct _fat_ptr x){
# 326
int sx=(int)_get_fat_size(x,sizeof(void*));
{int i=0;for(0;i < sx;++ i){
accum=f(env,accum,*((void**)_check_fat_subscript(x,sizeof(void*),i)));}}
# 330
return accum;}
# 250
void*Cyc_Array_fold_right(void*(*f)(void*,void*),struct _fat_ptr x,void*accum){
# 337
{int i=(int)(_get_fat_size(x,sizeof(void*))- (unsigned)1);for(0;i >= 0;-- i){
accum=f(*((void**)_check_fat_subscript(x,sizeof(void*),i)),accum);}}
# 340
return accum;}
# 250
void*Cyc_Array_fold_right_c(void*(*f)(void*,void*,void*),void*env,struct _fat_ptr x,void*accum){
# 343
{int i=(int)(_get_fat_size(x,sizeof(void*))- (unsigned)1);for(0;i >= 0;-- i){
accum=f(env,*((void**)_check_fat_subscript(x,sizeof(void*),i)),accum);}}
# 346
return accum;}
# 250
struct _fat_ptr Cyc_Array_rev_copy(struct _fat_ptr x){
# 351
int sx=(int)_get_fat_size(x,sizeof(void*));
int n=sx - 1;
return({unsigned _tmpF=(unsigned)sx;void**_tmpE=_cycalloc(_check_times(_tmpF,sizeof(void*)));({{unsigned _tmp1F=(unsigned)sx;unsigned i;for(i=0;i < _tmp1F;++ i){_tmpE[i]=*((void**)_check_fat_subscript(x,sizeof(void*),(int)((unsigned)n - i)));}}0;});_tag_fat(_tmpE,sizeof(void*),_tmpF);});}
# 250
void Cyc_Array_imp_rev(struct _fat_ptr x){
# 358
void*temp;
int i=0;
int j=(int)(_get_fat_size(x,sizeof(void*))- (unsigned)1);
while(i < j){
temp=*((void**)_check_fat_subscript(x,sizeof(void*),i));
((void**)x.curr)[i]=*((void**)_check_fat_subscript(x,sizeof(void*),j));
((void**)x.curr)[j]=temp;
++ i;
-- j;}}
# 250
int Cyc_Array_forall(int(*pred)(void*),struct _fat_ptr x){
# 373
int sx=(int)_get_fat_size(x,sizeof(void*));
{int i=0;for(0;i < sx;++ i){
if(!pred(*((void**)_check_fat_subscript(x,sizeof(void*),i))))return 0;}}
# 377
return 1;}
# 250
int Cyc_Array_forall_c(int(*pred)(void*,void*),void*env,struct _fat_ptr x){
# 380
int sx=(int)_get_fat_size(x,sizeof(void*));
{int i=0;for(0;i < sx;++ i){
if(!pred(env,*((void**)_check_fat_subscript(x,sizeof(void*),i))))return 0;}}
# 384
return 1;}
# 250
int Cyc_Array_exists(int(*pred)(void*),struct _fat_ptr x){
# 390
int sx=(int)_get_fat_size(x,sizeof(void*));
{int i=0;for(0;i < sx;++ i){
if(pred(*((void**)_check_fat_subscript(x,sizeof(void*),i))))return 1;}}
# 394
return 0;}
# 250
int Cyc_Array_exists_c(int(*pred)(void*,void*),void*env,struct _fat_ptr x){
# 397
int sx=(int)_get_fat_size(x,sizeof(void*));
{int i=0;for(0;i < sx;++ i){
if(pred(env,*((void**)_check_fat_subscript(x,sizeof(void*),i))))return 1;}}
# 401
return 0;}struct _tuple0{void*f1;void*f2;};
# 250
struct _fat_ptr Cyc_Array_zip(struct _fat_ptr x,struct _fat_ptr y){
# 407
int sx=(int)_get_fat_size(x,sizeof(void*));
if((unsigned)sx != _get_fat_size(y,sizeof(void*)))(int)_throw((void*)& Cyc_Array_Array_mismatch_val);return({unsigned _tmp11=(unsigned)sx;struct _tuple0*_tmp10=_cycalloc(_check_times(_tmp11,sizeof(struct _tuple0)));({{unsigned _tmp20=(unsigned)sx;unsigned i;for(i=0;i < _tmp20;++ i){
# 410
(_tmp10[i]).f1=((void**)x.curr)[(int)i],(_tmp10[i]).f2=((void**)y.curr)[(int)i];}}0;});_tag_fat(_tmp10,sizeof(struct _tuple0),_tmp11);});}struct _tuple1{struct _fat_ptr f1;struct _fat_ptr f2;};
# 250
struct _tuple1 Cyc_Array_split(struct _fat_ptr x){
# 415
int sx=(int)_get_fat_size(x,sizeof(struct _tuple0));
return({struct _tuple1 _tmp21;({struct _fat_ptr _tmp41=({unsigned _tmp13=(unsigned)sx;void**_tmp12=_cycalloc(_check_times(_tmp13,sizeof(void*)));({{unsigned _tmp22=(unsigned)sx;unsigned i;for(i=0;i < _tmp22;++ i){_tmp12[i]=(((struct _tuple0*)x.curr)[(int)i]).f1;}}0;});_tag_fat(_tmp12,sizeof(void*),_tmp13);});_tmp21.f1=_tmp41;}),({
struct _fat_ptr _tmp40=({unsigned _tmp15=(unsigned)sx;void**_tmp14=_cycalloc(_check_times(_tmp15,sizeof(void*)));({{unsigned _tmp23=(unsigned)sx;unsigned i;for(i=0;i < _tmp23;++ i){_tmp14[i]=(((struct _tuple0*)x.curr)[(int)i]).f2;}}0;});_tag_fat(_tmp14,sizeof(void*),_tmp15);});_tmp21.f2=_tmp40;});_tmp21;});}
# 250
int Cyc_Array_memq(struct _fat_ptr l,void*x){
# 424
int s=(int)_get_fat_size(l,sizeof(void*));
{int i=0;for(0;i < s;++ i){
if(*((void**)_check_fat_subscript(l,sizeof(void*),i))== x)return 1;}}
# 428
return 0;}
# 250
int Cyc_Array_mem(int(*compare)(void*,void*),struct _fat_ptr l,void*x){
# 432
int s=(int)_get_fat_size(l,sizeof(void*));
{int i=0;for(0;i < s;++ i){
if(0 == compare(*((void**)_check_fat_subscript(l,sizeof(void*),i)),x))return 1;}}
# 436
return 0;}
# 250
struct _fat_ptr Cyc_Array_extract(struct _fat_ptr x,int start,int*n_opt){
# 443
int sx=(int)_get_fat_size(x,sizeof(void*));
int n=n_opt == 0?sx - start:*n_opt;
# 446
if((start < 0 || n <= 0)|| start + (n_opt == 0?0: n)> sx)
(int)_throw((void*)({struct Cyc_Core_Invalid_argument_exn_struct*_tmp17=_cycalloc(sizeof(*_tmp17));_tmp17->tag=Cyc_Core_Invalid_argument,({struct _fat_ptr _tmp42=({const char*_tmp16="Array::extract";_tag_fat(_tmp16,sizeof(char),15U);});_tmp17->f1=_tmp42;});_tmp17;}));
# 446
return({unsigned _tmp19=(unsigned)n;void**_tmp18=_cycalloc(_check_times(_tmp19,sizeof(void*)));({{unsigned _tmp24=(unsigned)n;unsigned i;for(i=0;i < _tmp24;++ i){_tmp18[i]=*((void**)_check_fat_subscript(x,sizeof(void*),(int)((unsigned)start + i)));}}0;});_tag_fat(_tmp18,sizeof(void*),_tmp19);});}

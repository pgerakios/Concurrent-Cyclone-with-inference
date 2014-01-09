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
extern struct _RegionHandle*Cyc_Core_unique_region;struct Cyc_Core_DynamicRegion;struct Cyc_Core_NewDynamicRegion{struct Cyc_Core_DynamicRegion*key;};struct Cyc_Core_ThinRes{void*arr;unsigned nelts;};struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};extern char Cyc_List_List_mismatch[14U];struct Cyc_List_List_mismatch_exn_struct{char*tag;};extern char Cyc_List_Nth[4U];struct Cyc_List_Nth_exn_struct{char*tag;};struct Cyc___cycFILE;struct Cyc_String_pa_PrintArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Int_pa_PrintArg_struct{int tag;unsigned long f1;};struct Cyc_Double_pa_PrintArg_struct{int tag;double f1;};struct Cyc_LongDouble_pa_PrintArg_struct{int tag;long double f1;};struct Cyc_ShortPtr_pa_PrintArg_struct{int tag;short*f1;};struct Cyc_IntPtr_pa_PrintArg_struct{int tag;unsigned long*f1;};struct Cyc_ShortPtr_sa_ScanfArg_struct{int tag;short*f1;};struct Cyc_UShortPtr_sa_ScanfArg_struct{int tag;unsigned short*f1;};struct Cyc_IntPtr_sa_ScanfArg_struct{int tag;int*f1;};struct Cyc_UIntPtr_sa_ScanfArg_struct{int tag;unsigned*f1;};struct Cyc_StringPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_DoublePtr_sa_ScanfArg_struct{int tag;double*f1;};struct Cyc_FloatPtr_sa_ScanfArg_struct{int tag;float*f1;};struct Cyc_CharPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};
# 157 "cycboot.h"
int Cyc_printf(struct _fat_ptr,struct _fat_ptr);extern char Cyc_FileCloseError[15U];struct Cyc_FileCloseError_exn_struct{char*tag;};extern char Cyc_FileOpenError[14U];struct Cyc_FileOpenError_exn_struct{char*tag;struct _fat_ptr f1;};struct Cyc_Iter_Iter{void*env;int(*next)(void*env,void*dest);};struct Cyc_Dict_T;struct Cyc_Dict_Dict{int(*rel)(void*,void*);struct _RegionHandle*r;const struct Cyc_Dict_T*t;};extern char Cyc_Dict_Present[8U];struct Cyc_Dict_Present_exn_struct{char*tag;};extern char Cyc_Dict_Absent[7U];struct Cyc_Dict_Absent_exn_struct{char*tag;};
# 110 "dict.h"
void*Cyc_Dict_lookup(struct Cyc_Dict_Dict d,void*k);
# 34 "bitvec.h"
struct _fat_ptr Cyc_Bitvec_new_empty(int);
# 46
int Cyc_Bitvec_get(struct _fat_ptr,int);
# 48
void Cyc_Bitvec_set(struct _fat_ptr,int);
# 62
void Cyc_Bitvec_union_two(struct _fat_ptr dest,struct _fat_ptr src1,struct _fat_ptr src2);union _tuple0{unsigned __wch;char __wchb[4U];};struct _tuple1{int __count;union _tuple0 __value;};struct _tuple2{long __pos;struct _tuple1 __state;};struct Cyc___cycFILE;struct __abstractFILE;
# 225 "stdio.h"
int Cyc_printf(struct _fat_ptr,struct _fat_ptr);
# 29 "assert.h"
void*Cyc___assert_fail(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line);
# 41 "bitvec.cyc"
int Cyc_Bitvec_get(struct _fat_ptr bvec,int pos){
int word=pos >> 5;
int offset=pos & 31;
return(*((int*)_check_fat_subscript(bvec,sizeof(int),word))>> offset & 1)== 1;}
# 41
void Cyc_Bitvec_set(struct _fat_ptr bvec,int pos){
# 48
int word=pos >> 5;
int offset=pos & 31;
((int*)bvec.curr)[word]=*((int*)_check_fat_subscript(bvec,sizeof(int),word))| 1 << offset;}
# 41
void Cyc_Bitvec_clear(struct _fat_ptr bvec,int pos){
# 54
int word=pos >> 5;
int offset=pos & 31;
((int*)bvec.curr)[word]=*((int*)_check_fat_subscript(bvec,sizeof(int),word))& ~(1 << offset);}
# 41
int Cyc_Bitvec_get_and_set(struct _fat_ptr bvec,int pos){
# 60
int word=pos >> 5;
int offset=pos & 31;
int slot=*((int*)_check_fat_subscript(bvec,sizeof(int),word));
int ans=(slot >> offset & 1)== 1;
if(!ans)
((int*)bvec.curr)[word]=slot | 1 << offset;
# 64
return ans;}
# 41
void Cyc_Bitvec_union_two(struct _fat_ptr dest,struct _fat_ptr src1,struct _fat_ptr src2){
# 70
unsigned len=_get_fat_size(dest,sizeof(int));
len <= _get_fat_size(src1,sizeof(int))&& len <= _get_fat_size(src2,sizeof(int))?0:({int(*_tmp21)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmp0)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmp0;});struct _fat_ptr _tmp20=({const char*_tmp1="len <= numelts(src1) && len <= numelts(src2)";_tag_fat(_tmp1,sizeof(char),45U);});_tmp21(_tmp20,({const char*_tmp2="bitvec.cyc";_tag_fat(_tmp2,sizeof(char),11U);}),71U);});{
int i=0;for(0;(unsigned)i < len;++ i){
((int*)dest.curr)[i]=((int*)src1.curr)[i]| ((int*)src2.curr)[i];}}}
# 41
void Cyc_Bitvec_intersect_two(struct _fat_ptr dest,struct _fat_ptr src1,struct _fat_ptr src2){
# 77
unsigned len=_get_fat_size(dest,sizeof(int));
len <= _get_fat_size(src1,sizeof(int))&& len <= _get_fat_size(src2,sizeof(int))?0:({int(*_tmp23)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmp3)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmp3;});struct _fat_ptr _tmp22=({const char*_tmp4="len <= numelts(src1) && len <= numelts(src2)";_tag_fat(_tmp4,sizeof(char),45U);});_tmp23(_tmp22,({const char*_tmp5="bitvec.cyc";_tag_fat(_tmp5,sizeof(char),11U);}),78U);});{
int i=0;for(0;(unsigned)i < len;++ i){
((int*)dest.curr)[i]=((int*)src1.curr)[i]& ((int*)src2.curr)[i];}}}
# 41
void Cyc_Bitvec_diff_two(struct _fat_ptr dest,struct _fat_ptr src1,struct _fat_ptr src2){
# 84
unsigned len=_get_fat_size(dest,sizeof(int));
len <= _get_fat_size(src1,sizeof(int))&& len <= _get_fat_size(src2,sizeof(int))?0:({int(*_tmp25)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmp6)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmp6;});struct _fat_ptr _tmp24=({const char*_tmp7="len <= numelts(src1) && len <= numelts(src2)";_tag_fat(_tmp7,sizeof(char),45U);});_tmp25(_tmp24,({const char*_tmp8="bitvec.cyc";_tag_fat(_tmp8,sizeof(char),11U);}),85U);});{
int i=0;for(0;(unsigned)i < len;++ i){
((int*)dest.curr)[i]=((int*)src1.curr)[i]& ~((int*)src2.curr)[i];}}}
# 41
int Cyc_Bitvec_compare_two(struct _fat_ptr src1,struct _fat_ptr src2){
# 91
unsigned len=_get_fat_size(src1,sizeof(int));
len <= _get_fat_size(src2,sizeof(int))?0:({int(*_tmp27)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmp9)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmp9;});struct _fat_ptr _tmp26=({const char*_tmpA="len <= numelts(src2)";_tag_fat(_tmpA,sizeof(char),21U);});_tmp27(_tmp26,({const char*_tmpB="bitvec.cyc";_tag_fat(_tmpB,sizeof(char),11U);}),92U);});
{int i=0;for(0;(unsigned)i < len;++ i){
if(((int*)src1.curr)[i]!= ((int*)src2.curr)[i])
return 0;}}
# 93
return 1;}
# 41
struct _fat_ptr Cyc_Bitvec_new_empty(int sz){
# 101
struct _fat_ptr ans=({unsigned _tmpD=(unsigned)(sz / 32 + 1);int*_tmpC=_cycalloc_atomic(_check_times(_tmpD,sizeof(int)));({{unsigned _tmp1C=(unsigned)(sz / 32 + 1);unsigned i;for(i=0;i < _tmp1C;++ i){_tmpC[i]=0;}}0;});_tag_fat(_tmpC,sizeof(int),_tmpD);});
return ans;}
# 41
struct _fat_ptr Cyc_Bitvec_new_full(int sz){
# 105
struct _fat_ptr ans=({unsigned _tmpF=(unsigned)(sz / 32 + 1);int*_tmpE=_cycalloc_atomic(_check_times(_tmpF,sizeof(int)));({{unsigned _tmp1D=(unsigned)(sz / 32 + 1);unsigned i;for(i=0;i < _tmp1D;++ i){_tmpE[i]=-1;}}0;});_tag_fat(_tmpE,sizeof(int),_tmpF);});
return ans;}
# 41
struct _fat_ptr Cyc_Bitvec_new_copy(struct _fat_ptr old){
# 110
struct _fat_ptr copy=Cyc_Bitvec_new_empty((int)_get_fat_size(old,sizeof(int)));
Cyc_Bitvec_union_two(copy,copy,old);
return copy;}
# 41
struct _fat_ptr Cyc_Bitvec_from_list(struct Cyc_Dict_Dict d,int(*f)(void*),int sz,struct Cyc_List_List*l){
# 117
struct _fat_ptr ans=({unsigned _tmp11=(unsigned)(sz % 32 + 1);int*_tmp10=_cycalloc_atomic(_check_times(_tmp11,sizeof(int)));({{unsigned _tmp1E=(unsigned)(sz % 32 + 1);unsigned i;for(i=0;i < _tmp1E;++ i){_tmp10[i]=0;}}0;});_tag_fat(_tmp10,sizeof(int),_tmp11);});
for(0;l != 0;l=l->tl){
({struct _fat_ptr _tmp29=ans;Cyc_Bitvec_set(_tmp29,({int(*_tmp28)(void*)=f;_tmp28(Cyc_Dict_lookup(d,l->hd));}));});}
return ans;}
# 41
struct Cyc_List_List*Cyc_Bitvec_to_sorted_list(struct _fat_ptr bvec,int sz){
# 124
struct Cyc_List_List*ans=0;
{int pos=sz - 1;for(0;pos >= 0;0){
int word=pos >> 5;
int bits=*((int*)_check_fat_subscript(bvec,sizeof(int),word));
int offset=pos & 31;for(0;offset >= 0;(-- offset,-- pos)){
if((bits >> offset & 1)== 1)
ans=({unsigned _tmp13=1;struct Cyc_List_List*_tmp12=_cycalloc(_check_times(_tmp13,sizeof(struct Cyc_List_List)));(_tmp12[0]).hd=(void*)pos,(_tmp12[0]).tl=ans;_tmp12;});}}}
# 132
return ans;}
# 41
void Cyc_Bitvec_clear_all(struct _fat_ptr bvec){
# 136
unsigned len=_get_fat_size(bvec,sizeof(int));
int i=0;for(0;(unsigned)i < len;++ i){
((int*)bvec.curr)[i]=0;}}
# 41
void Cyc_Bitvec_set_all(struct _fat_ptr bvec){
# 142
unsigned len=_get_fat_size(bvec,sizeof(int));
int i=0;for(0;(unsigned)i < len;++ i){
((int*)bvec.curr)[i]=-1;}}
# 41
int Cyc_Bitvec_all_set(struct _fat_ptr bvec,int sz){
# 148
int words=sz >> 5;
(unsigned)words < _get_fat_size(bvec,sizeof(int))?0:({int(*_tmp2B)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmp14)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmp14;});struct _fat_ptr _tmp2A=({const char*_tmp15="words < numelts(bvec)";_tag_fat(_tmp15,sizeof(char),22U);});_tmp2B(_tmp2A,({const char*_tmp16="bitvec.cyc";_tag_fat(_tmp16,sizeof(char),11U);}),149U);});
{int i=0;for(0;i < words;++ i){
if(*((int*)_check_fat_subscript(bvec,sizeof(int),i))!= -1)
return 0;}}
# 150
{
# 153
int i=words * 32;
# 150
for(0;i < sz;++ i){
# 154
if(!Cyc_Bitvec_get(bvec,i))
return 0;}}
# 150
return 1;}
# 41
void Cyc_Bitvec_print_bvec(struct _fat_ptr bvec){
# 161
{int i=0;for(0;(unsigned)i < (unsigned)32 * _get_fat_size(bvec,sizeof(int));++ i){
({struct Cyc_Int_pa_PrintArg_struct _tmp19=({struct Cyc_Int_pa_PrintArg_struct _tmp1F;_tmp1F.tag=1U,({unsigned long _tmp2C=(unsigned long)(Cyc_Bitvec_get(bvec,i)?1: 0);_tmp1F.f1=_tmp2C;});_tmp1F;});void*_tmp17[1U];_tmp17[0]=& _tmp19;({struct _fat_ptr _tmp2D=({const char*_tmp18="%d";_tag_fat(_tmp18,sizeof(char),3U);});Cyc_printf(_tmp2D,_tag_fat(_tmp17,sizeof(void*),1U));});});}}
({void*_tmp1A=0U;({struct _fat_ptr _tmp2E=({const char*_tmp1B="\n";_tag_fat(_tmp1B,sizeof(char),2U);});Cyc_printf(_tmp2E,_tag_fat(_tmp1A,sizeof(void*),0U));});});}

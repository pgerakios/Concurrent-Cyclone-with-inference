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
extern struct _RegionHandle*Cyc_Core_unique_region;struct Cyc_Core_DynamicRegion;struct Cyc_Core_NewDynamicRegion{struct Cyc_Core_DynamicRegion*key;};struct Cyc_Core_ThinRes{void*arr;unsigned nelts;};union _tuple0{unsigned __wch;char __wchb[4U];};struct _tuple1{int __count;union _tuple0 __value;};struct _tuple2{long __pos;struct _tuple1 __state;};struct Cyc___cycFILE;
# 91 "stdio.h"
extern struct Cyc___cycFILE*Cyc_stderr;struct __abstractFILE;struct Cyc_String_pa_PrintArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Int_pa_PrintArg_struct{int tag;unsigned long f1;};struct Cyc_Double_pa_PrintArg_struct{int tag;double f1;};struct Cyc_LongDouble_pa_PrintArg_struct{int tag;long double f1;};struct Cyc_ShortPtr_pa_PrintArg_struct{int tag;short*f1;};struct Cyc_IntPtr_pa_PrintArg_struct{int tag;unsigned long*f1;};
# 152
int Cyc_fprintf(struct Cyc___cycFILE*,struct _fat_ptr,struct _fat_ptr);struct Cyc_ShortPtr_sa_ScanfArg_struct{int tag;short*f1;};struct Cyc_UShortPtr_sa_ScanfArg_struct{int tag;unsigned short*f1;};struct Cyc_IntPtr_sa_ScanfArg_struct{int tag;int*f1;};struct Cyc_UIntPtr_sa_ScanfArg_struct{int tag;unsigned*f1;};struct Cyc_StringPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_DoublePtr_sa_ScanfArg_struct{int tag;double*f1;};struct Cyc_FloatPtr_sa_ScanfArg_struct{int tag;float*f1;};struct Cyc_CharPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};extern char Cyc_FileCloseError[15U];struct Cyc_FileCloseError_exn_struct{char*tag;};extern char Cyc_FileOpenError[14U];struct Cyc_FileOpenError_exn_struct{char*tag;struct _fat_ptr f1;};struct _tuple3{int quot;int rem;};
# 111 "stdlib.h"
char*getenv(const char*);
# 53 "unistd.h"
enum Cyc___anonymous_enum_103__{Cyc__PC_LINK_MAX =0U,Cyc__PC_MAX_CANON =1U,Cyc__PC_MAX_INPUT =2U,Cyc__PC_NAME_MAX =3U,Cyc__PC_PATH_MAX =4U,Cyc__PC_PIPE_BUF =5U,Cyc__PC_CHOWN_RESTRICTED =6U,Cyc__PC_NO_TRUNC =7U,Cyc__PC_VDISABLE =8U,Cyc__PC_SYNC_IO =9U,Cyc__PC_ASYNC_IO =10U,Cyc__PC_PRIO_IO =11U,Cyc__PC_SOCK_MAXBUF =12U,Cyc__PC_FILESIZEBITS =13U,Cyc__PC_REC_INCR_XFER_SIZE =14U,Cyc__PC_REC_MAX_XFER_SIZE =15U,Cyc__PC_REC_MIN_XFER_SIZE =16U,Cyc__PC_REC_XFER_ALIGN =17U,Cyc__PC_ALLOC_SIZE_MIN =18U,Cyc__PC_SYMLINK_MAX =19U,Cyc__PC_2_SYMLINKS =20U};
# 79
enum Cyc___anonymous_enum_105__{Cyc__CS_PATH =0U,Cyc__CS_V6_WIDTH_RESTRICTED_ENVS =1U,Cyc__CS_GNU_LIBC_VERSION =2U,Cyc__CS_GNU_LIBPTHREAD_VERSION =3U,Cyc__CS_LFS_CFLAGS =1000,Cyc__CS_LFS_LDFLAGS =1001U,Cyc__CS_LFS_LIBS =1002U,Cyc__CS_LFS_LINTFLAGS =1003U,Cyc__CS_LFS64_CFLAGS =1004U,Cyc__CS_LFS64_LDFLAGS =1005U,Cyc__CS_LFS64_LIBS =1006U,Cyc__CS_LFS64_LINTFLAGS =1007U,Cyc__CS_XBS5_ILP32_OFF32_CFLAGS =1100,Cyc__CS_XBS5_ILP32_OFF32_LDFLAGS =1101U,Cyc__CS_XBS5_ILP32_OFF32_LIBS =1102U,Cyc__CS_XBS5_ILP32_OFF32_LINTFLAGS =1103U,Cyc__CS_XBS5_ILP32_OFFBIG_CFLAGS =1104U,Cyc__CS_XBS5_ILP32_OFFBIG_LDFLAGS =1105U,Cyc__CS_XBS5_ILP32_OFFBIG_LIBS =1106U,Cyc__CS_XBS5_ILP32_OFFBIG_LINTFLAGS =1107U,Cyc__CS_XBS5_LP64_OFF64_CFLAGS =1108U,Cyc__CS_XBS5_LP64_OFF64_LDFLAGS =1109U,Cyc__CS_XBS5_LP64_OFF64_LIBS =1110U,Cyc__CS_XBS5_LP64_OFF64_LINTFLAGS =1111U,Cyc__CS_XBS5_LPBIG_OFFBIG_CFLAGS =1112U,Cyc__CS_XBS5_LPBIG_OFFBIG_LDFLAGS =1113U,Cyc__CS_XBS5_LPBIG_OFFBIG_LIBS =1114U,Cyc__CS_XBS5_LPBIG_OFFBIG_LINTFLAGS =1115U,Cyc__CS_POSIX_V6_ILP32_OFF32_CFLAGS =1116U,Cyc__CS_POSIX_V6_ILP32_OFF32_LDFLAGS =1117U,Cyc__CS_POSIX_V6_ILP32_OFF32_LIBS =1118U,Cyc__CS_POSIX_V6_ILP32_OFF32_LINTFLAGS =1119U,Cyc__CS_POSIX_V6_ILP32_OFFBIG_CFLAGS =1120U,Cyc__CS_POSIX_V6_ILP32_OFFBIG_LDFLAGS =1121U,Cyc__CS_POSIX_V6_ILP32_OFFBIG_LIBS =1122U,Cyc__CS_POSIX_V6_ILP32_OFFBIG_LINTFLAGS =1123U,Cyc__CS_POSIX_V6_LP64_OFF64_CFLAGS =1124U,Cyc__CS_POSIX_V6_LP64_OFF64_LDFLAGS =1125U,Cyc__CS_POSIX_V6_LP64_OFF64_LIBS =1126U,Cyc__CS_POSIX_V6_LP64_OFF64_LINTFLAGS =1127U,Cyc__CS_POSIX_V6_LPBIG_OFFBIG_CFLAGS =1128U,Cyc__CS_POSIX_V6_LPBIG_OFFBIG_LDFLAGS =1129U,Cyc__CS_POSIX_V6_LPBIG_OFFBIG_LIBS =1130U,Cyc__CS_POSIX_V6_LPBIG_OFFBIG_LINTFLAGS =1131U};
# 31 "getopt.h"
extern struct _fat_ptr Cyc_optarg;
# 45 "getopt.h"
extern int Cyc_optind;
# 50
extern int Cyc_opterr;
# 54
extern int Cyc_optopt;struct Cyc_option{struct _fat_ptr name;int has_arg;int*flag;int val;};
# 124 "getopt.h"
int Cyc__getopt_internal(int __argc,struct _fat_ptr __argv,struct _fat_ptr __shortopts,struct _fat_ptr __longopts,int*__longind,int __long_only);struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};extern char Cyc_List_List_mismatch[14U];struct Cyc_List_List_mismatch_exn_struct{char*tag;};extern char Cyc_List_Nth[4U];struct Cyc_List_Nth_exn_struct{char*tag;};struct Cyc___cycFILE;
# 53 "cycboot.h"
extern struct Cyc___cycFILE*Cyc_stderr;
# 100 "cycboot.h"
int Cyc_fprintf(struct Cyc___cycFILE*,struct _fat_ptr,struct _fat_ptr);
# 313 "cycboot.h"
char*getenv(const char*);
# 38 "string.h"
unsigned Cyc_strlen(struct _fat_ptr s);
# 49 "string.h"
int Cyc_strcmp(struct _fat_ptr s1,struct _fat_ptr s2);
# 51
int Cyc_strncmp(struct _fat_ptr s1,struct _fat_ptr s2,unsigned len);
# 120 "string.h"
struct _fat_ptr Cyc_strchr(struct _fat_ptr s,char c);
# 67 "getopt.cyc"
struct _fat_ptr Cyc_optarg;
# 82 "getopt.cyc"
int Cyc_optind=1;
# 88
int Cyc___getopt_initialized;
# 97 "getopt.cyc"
static struct _fat_ptr Cyc_nextchar;
# 102
int Cyc_opterr=1;
# 108
int Cyc_optopt=(int)'?';
# 140 "getopt.cyc"
enum Cyc_ordering_tag{Cyc_REQUIRE_ORDER =0U,Cyc_PERMUTE =1U,Cyc_RETURN_IN_ORDER =2U};
# 144
static enum Cyc_ordering_tag Cyc_ordering;
# 147
static struct _fat_ptr Cyc_posixly_correct;
# 155
static int Cyc_first_nonopt;
static int Cyc_last_nonopt;
# 161
static int Cyc_nonoption_flags_max_len;
static int Cyc_nonoption_flags_len;
# 164
static int Cyc_original_argc;
static int Cyc_original_argv;
# 170
static void  __attribute__((unused )) Cyc_store_args_and_env(int argc,struct _fat_ptr argv){
# 176
Cyc_original_argc=argc;
Cyc_original_argv=(int)((struct _fat_ptr*)_untag_fat_ptr(argv,sizeof(struct _fat_ptr),1U));}
# 170
static void Cyc_exchange(struct _fat_ptr argv){
# 194 "getopt.cyc"
int bottom=Cyc_first_nonopt;
int middle=Cyc_last_nonopt;
int top=Cyc_optind;
struct _fat_ptr tem;
# 204
while(top > middle && middle > bottom){
# 206
if(top - middle > middle - bottom){
# 209
int len=middle - bottom;
register int i;
# 213
for(i=0;i < len;++ i){
# 215
tem=*((struct _fat_ptr*)_check_fat_subscript(argv,sizeof(struct _fat_ptr),bottom + i));
({struct _fat_ptr _tmp58=*((struct _fat_ptr*)_check_fat_subscript(argv,sizeof(struct _fat_ptr),(top - (middle - bottom))+ i));*((struct _fat_ptr*)_check_fat_subscript(argv,sizeof(struct _fat_ptr),bottom + i))=_tmp58;});
*((struct _fat_ptr*)_check_fat_subscript(argv,sizeof(struct _fat_ptr),(top - (middle - bottom))+ i))=tem;}
# 221
top -=len;}else{
# 226
int len=top - middle;
register int i;
# 230
for(i=0;i < len;++ i){
# 232
tem=*((struct _fat_ptr*)_check_fat_subscript(argv,sizeof(struct _fat_ptr),bottom + i));
({struct _fat_ptr _tmp59=*((struct _fat_ptr*)_check_fat_subscript(argv,sizeof(struct _fat_ptr),middle + i));*((struct _fat_ptr*)_check_fat_subscript(argv,sizeof(struct _fat_ptr),bottom + i))=_tmp59;});
*((struct _fat_ptr*)_check_fat_subscript(argv,sizeof(struct _fat_ptr),middle + i))=tem;}
# 238
bottom +=len;}}
# 244
Cyc_first_nonopt +=Cyc_optind - Cyc_last_nonopt;
Cyc_last_nonopt=Cyc_optind;}
# 170 "getopt.cyc"
static struct _fat_ptr Cyc__getopt_initialize(int argc,struct _fat_ptr argv,struct _fat_ptr optstring){
# 259 "getopt.cyc"
Cyc_first_nonopt=(Cyc_last_nonopt=Cyc_optind);
# 261
Cyc_nextchar=_tag_fat(0,0,0);
# 263
Cyc_posixly_correct=({char*_tmp0=getenv("POSIXLY_CORRECT");_tag_fat(_tmp0,sizeof(char),_get_zero_arr_size_char((void*)_tmp0,1U));});
# 267
if((int)*((const char*)_check_fat_subscript(optstring,sizeof(char),0))== (int)'-'){
# 269
Cyc_ordering=Cyc_RETURN_IN_ORDER;
_fat_ptr_inplace_plus(& optstring,sizeof(char),1);}else{
# 272
if((int)((const char*)optstring.curr)[0]== (int)'+'){
# 274
Cyc_ordering=Cyc_REQUIRE_ORDER;
_fat_ptr_inplace_plus(& optstring,sizeof(char),1);}else{
# 277
if(({char*_tmp5A=(char*)Cyc_posixly_correct.curr;_tmp5A != (char*)(_tag_fat(0,0,0)).curr;}))
Cyc_ordering=Cyc_REQUIRE_ORDER;else{
# 280
Cyc_ordering=Cyc_PERMUTE;}}}
# 282
return optstring;}
# 170 "getopt.cyc"
int Cyc__getopt_internal(int argc,struct _fat_ptr argv,struct _fat_ptr optstring,struct _fat_ptr longopts,int*longind,int long_only){
# 345 "getopt.cyc"
int print_errors=Cyc_opterr;
if((int)*((const char*)_check_fat_subscript(optstring,sizeof(char),0))== (int)':')
print_errors=0;
# 346
if(argc < 1)
# 350
return - 1;
# 346
Cyc_optarg=
# 352
_tag_fat(0,0,0);
# 354
if(Cyc_optind == 0 || !Cyc___getopt_initialized){
# 356
if(Cyc_optind == 0)
Cyc_optind=1;
# 356
optstring=
# 358
Cyc__getopt_initialize(argc,argv,optstring);
Cyc___getopt_initialized=1;}
# 354
if(
# 365
({char*_tmp5B=(char*)Cyc_nextchar.curr;_tmp5B == (char*)(_tag_fat(0,0,0)).curr;})||(int)*((char*)_check_fat_subscript(Cyc_nextchar,sizeof(char),0U))== (int)'\000'){
# 371
if(Cyc_last_nonopt > Cyc_optind)
Cyc_last_nonopt=Cyc_optind;
# 371
if(Cyc_first_nonopt > Cyc_optind)
# 374
Cyc_first_nonopt=Cyc_optind;
# 371
if((int)Cyc_ordering == (int)Cyc_PERMUTE){
# 381
if(Cyc_first_nonopt != Cyc_last_nonopt && Cyc_last_nonopt != Cyc_optind)
Cyc_exchange(argv);else{
if(Cyc_last_nonopt != Cyc_optind)
Cyc_first_nonopt=Cyc_optind;}
# 381
while(
# 389
Cyc_optind < argc &&((int)*((char*)_check_fat_subscript(*((struct _fat_ptr*)_check_fat_subscript(argv,sizeof(struct _fat_ptr),Cyc_optind)),sizeof(char),0))!= (int)'-' ||(int)*((char*)_check_fat_subscript(*((struct _fat_ptr*)_check_fat_subscript(argv,sizeof(struct _fat_ptr),Cyc_optind)),sizeof(char),1))== (int)'\000')){
++ Cyc_optind;}
Cyc_last_nonopt=Cyc_optind;}
# 371
if(
# 399
Cyc_optind != argc && !({struct _fat_ptr _tmp5C=(struct _fat_ptr)*((struct _fat_ptr*)_check_fat_subscript(argv,sizeof(struct _fat_ptr),Cyc_optind));Cyc_strcmp(_tmp5C,({const char*_tmp1="--";_tag_fat(_tmp1,sizeof(char),3U);}));})){
# 401
++ Cyc_optind;
# 403
if(Cyc_first_nonopt != Cyc_last_nonopt && Cyc_last_nonopt != Cyc_optind)
Cyc_exchange(argv);else{
if(Cyc_first_nonopt == Cyc_last_nonopt)
Cyc_first_nonopt=Cyc_optind;}
# 403
Cyc_last_nonopt=argc;
# 409
Cyc_optind=argc;}
# 371
if(Cyc_optind == argc){
# 419
if(Cyc_first_nonopt != Cyc_last_nonopt)
Cyc_optind=Cyc_first_nonopt;
# 419
return - 1;}
# 371
if(
# 427
(int)*((char*)_check_fat_subscript(*((struct _fat_ptr*)_check_fat_subscript(argv,sizeof(struct _fat_ptr),Cyc_optind)),sizeof(char),0))!= (int)'-' ||(int)*((char*)_check_fat_subscript(*((struct _fat_ptr*)_check_fat_subscript(argv,sizeof(struct _fat_ptr),Cyc_optind)),sizeof(char),1))== (int)'\000'){
# 429
if((int)Cyc_ordering == (int)Cyc_REQUIRE_ORDER)
return - 1;
# 429
Cyc_optarg=*((struct _fat_ptr*)_check_fat_subscript(argv,sizeof(struct _fat_ptr),Cyc_optind ++));
# 432
return 1;}
# 371
Cyc_nextchar=({
# 438
struct _fat_ptr _tmp5E=_fat_ptr_plus(*((struct _fat_ptr*)_check_fat_subscript(argv,sizeof(struct _fat_ptr),Cyc_optind)),sizeof(char),1);_fat_ptr_plus(_tmp5E,sizeof(char),({struct Cyc_option*_tmp5D=(struct Cyc_option*)longopts.curr;_tmp5D != (struct Cyc_option*)(_tag_fat(0,0,0)).curr;})&&(int)*((char*)_check_fat_subscript(*((struct _fat_ptr*)_check_fat_subscript(argv,sizeof(struct _fat_ptr),Cyc_optind)),sizeof(char),1))== (int)'-');});}
# 354
if(
# 457 "getopt.cyc"
({struct Cyc_option*_tmp5F=(struct Cyc_option*)longopts.curr;_tmp5F != (struct Cyc_option*)(_tag_fat(0,0,0)).curr;})&&(
(int)*((char*)_check_fat_subscript(*((struct _fat_ptr*)_check_fat_subscript(argv,sizeof(struct _fat_ptr),Cyc_optind)),sizeof(char),1))== (int)'-' ||
 long_only &&((int)*((char*)_check_fat_subscript(*((struct _fat_ptr*)_check_fat_subscript(argv,sizeof(struct _fat_ptr),Cyc_optind)),sizeof(char),2))|| !((unsigned)(Cyc_strchr(optstring,*((char*)_check_fat_subscript(*((struct _fat_ptr*)_check_fat_subscript(argv,sizeof(struct _fat_ptr),Cyc_optind)),sizeof(char),1)))).curr)))){
# 461
struct _fat_ptr nameend;
struct _fat_ptr p;
struct _fat_ptr pfound=_tag_fat(0,0,0);
int exact=0;
int ambig=0;
int indfound=-1;
int option_index;
# 469
for(nameend=Cyc_nextchar;(int)*((char*)_check_fat_subscript(nameend,sizeof(char),0U))&&(int)*((char*)_check_fat_subscript(nameend,sizeof(char),0U))!= (int)'=';_fat_ptr_inplace_plus(& nameend,sizeof(char),1)){
;}
# 474
for((p=longopts,option_index=0);(unsigned)(((const struct Cyc_option*)_check_fat_subscript(p,sizeof(struct Cyc_option),0U))->name).curr;(_fat_ptr_inplace_plus_post(& p,sizeof(struct Cyc_option),1),option_index ++)){
if(!Cyc_strncmp((struct _fat_ptr)((const struct Cyc_option*)_check_fat_subscript(p,sizeof(struct Cyc_option),0U))->name,(struct _fat_ptr)Cyc_nextchar,(unsigned)((nameend.curr - Cyc_nextchar.curr)/ sizeof(char)))){
# 477
if(({unsigned _tmp60=(unsigned)((nameend.curr - Cyc_nextchar.curr)/ sizeof(char));_tmp60 == 
Cyc_strlen((struct _fat_ptr)((const struct Cyc_option*)_check_fat_subscript(p,sizeof(struct Cyc_option),0U))->name);})){
# 481
pfound=p;
indfound=option_index;
exact=1;
break;}else{
# 486
if(({struct Cyc_option*_tmp61=(struct Cyc_option*)pfound.curr;_tmp61 == (struct Cyc_option*)(_tag_fat(0,0,0)).curr;})){
# 489
pfound=p;
indfound=option_index;}else{
# 492
if(((long_only ||({
int _tmp64=((const struct Cyc_option*)_check_fat_subscript(pfound,sizeof(struct Cyc_option),0U))->has_arg;_tmp64 != ((const struct Cyc_option*)_check_fat_subscript(p,sizeof(struct Cyc_option),0U))->has_arg;}))||({
int*_tmp63=((const struct Cyc_option*)_check_fat_subscript(pfound,sizeof(struct Cyc_option),0U))->flag;_tmp63 != ((const struct Cyc_option*)_check_fat_subscript(p,sizeof(struct Cyc_option),0U))->flag;}))||({
int _tmp62=((const struct Cyc_option*)_check_fat_subscript(pfound,sizeof(struct Cyc_option),0U))->val;_tmp62 != ((const struct Cyc_option*)_check_fat_subscript(p,sizeof(struct Cyc_option),0U))->val;}))
# 497
ambig=1;}}}}
# 474
if(
# 500
ambig && !exact){
# 502
if(print_errors)
({struct Cyc_String_pa_PrintArg_struct _tmp4=({struct Cyc_String_pa_PrintArg_struct _tmp3C;_tmp3C.tag=0U,_tmp3C.f1=(struct _fat_ptr)((struct _fat_ptr)*((struct _fat_ptr*)_check_fat_subscript(argv,sizeof(struct _fat_ptr),0)));_tmp3C;});struct Cyc_String_pa_PrintArg_struct _tmp5=({struct Cyc_String_pa_PrintArg_struct _tmp3B;_tmp3B.tag=0U,_tmp3B.f1=(struct _fat_ptr)((struct _fat_ptr)*((struct _fat_ptr*)_check_fat_subscript(argv,sizeof(struct _fat_ptr),Cyc_optind)));_tmp3B;});void*_tmp2[2U];_tmp2[0]=& _tmp4,_tmp2[1]=& _tmp5;({struct Cyc___cycFILE*_tmp66=Cyc_stderr;struct _fat_ptr _tmp65=({const char*_tmp3="%s: option `%s' is ambiguous\n";_tag_fat(_tmp3,sizeof(char),30U);});Cyc_fprintf(_tmp66,_tmp65,_tag_fat(_tmp2,sizeof(void*),2U));});});
# 502
_fat_ptr_inplace_plus(& Cyc_nextchar,sizeof(char),(int)Cyc_strlen((struct _fat_ptr)Cyc_nextchar));
# 506
++ Cyc_optind;
Cyc_optopt=0;
return(int)'?';}
# 474
if(({
# 511
struct Cyc_option*_tmp67=(struct Cyc_option*)pfound.curr;_tmp67 != (struct Cyc_option*)(_tag_fat(0,0,0)).curr;})){
# 513
option_index=indfound;
++ Cyc_optind;
if((int)*((char*)_check_fat_subscript(nameend,sizeof(char),0U))){
# 519
if(((const struct Cyc_option*)_check_fat_subscript(pfound,sizeof(struct Cyc_option),0U))->has_arg)
Cyc_optarg=_fat_ptr_plus(nameend,sizeof(char),1);else{
# 523
if(print_errors){
# 525
if((int)*((char*)_check_fat_subscript(*((struct _fat_ptr*)_check_fat_subscript(argv,sizeof(struct _fat_ptr),Cyc_optind - 1)),sizeof(char),1))== (int)'-')
# 527
({struct Cyc_String_pa_PrintArg_struct _tmp8=({struct Cyc_String_pa_PrintArg_struct _tmp3E;_tmp3E.tag=0U,_tmp3E.f1=(struct _fat_ptr)((struct _fat_ptr)*((struct _fat_ptr*)_check_fat_subscript(argv,sizeof(struct _fat_ptr),0)));_tmp3E;});struct Cyc_String_pa_PrintArg_struct _tmp9=({struct Cyc_String_pa_PrintArg_struct _tmp3D;_tmp3D.tag=0U,_tmp3D.f1=(struct _fat_ptr)((struct _fat_ptr)((const struct Cyc_option*)_check_fat_subscript(pfound,sizeof(struct Cyc_option),0U))->name);_tmp3D;});void*_tmp6[2U];_tmp6[0]=& _tmp8,_tmp6[1]=& _tmp9;({struct Cyc___cycFILE*_tmp69=Cyc_stderr;struct _fat_ptr _tmp68=({const char*_tmp7="%s: option `--%s' doesn't allow an argument\n";_tag_fat(_tmp7,sizeof(char),45U);});Cyc_fprintf(_tmp69,_tmp68,_tag_fat(_tmp6,sizeof(void*),2U));});});else{
# 532
({struct Cyc_String_pa_PrintArg_struct _tmpC=({struct Cyc_String_pa_PrintArg_struct _tmp41;_tmp41.tag=0U,_tmp41.f1=(struct _fat_ptr)((struct _fat_ptr)*((struct _fat_ptr*)_check_fat_subscript(argv,sizeof(struct _fat_ptr),0)));_tmp41;});struct Cyc_Int_pa_PrintArg_struct _tmpD=({struct Cyc_Int_pa_PrintArg_struct _tmp40;_tmp40.tag=1U,_tmp40.f1=(unsigned long)((int)*((char*)_check_fat_subscript(*((struct _fat_ptr*)_check_fat_subscript(argv,sizeof(struct _fat_ptr),Cyc_optind - 1)),sizeof(char),0)));_tmp40;});struct Cyc_String_pa_PrintArg_struct _tmpE=({struct Cyc_String_pa_PrintArg_struct _tmp3F;_tmp3F.tag=0U,_tmp3F.f1=(struct _fat_ptr)((struct _fat_ptr)((const struct Cyc_option*)_check_fat_subscript(pfound,sizeof(struct Cyc_option),0U))->name);_tmp3F;});void*_tmpA[3U];_tmpA[0]=& _tmpC,_tmpA[1]=& _tmpD,_tmpA[2]=& _tmpE;({struct Cyc___cycFILE*_tmp6B=Cyc_stderr;struct _fat_ptr _tmp6A=({const char*_tmpB="%s: option `%c%s' doesn't allow an argument\n";_tag_fat(_tmpB,sizeof(char),45U);});Cyc_fprintf(_tmp6B,_tmp6A,_tag_fat(_tmpA,sizeof(void*),3U));});});}}
# 523
_fat_ptr_inplace_plus(& Cyc_nextchar,sizeof(char),(int)Cyc_strlen((struct _fat_ptr)Cyc_nextchar));
# 539
Cyc_optopt=((const struct Cyc_option*)_check_fat_subscript(pfound,sizeof(struct Cyc_option),0U))->val;
return(int)'?';}}else{
# 543
if(((const struct Cyc_option*)_check_fat_subscript(pfound,sizeof(struct Cyc_option),0U))->has_arg == 1){
# 545
if(Cyc_optind < argc)
Cyc_optarg=*((struct _fat_ptr*)_check_fat_subscript(argv,sizeof(struct _fat_ptr),Cyc_optind ++));else{
# 549
if(print_errors)
({struct Cyc_String_pa_PrintArg_struct _tmp11=({struct Cyc_String_pa_PrintArg_struct _tmp43;_tmp43.tag=0U,_tmp43.f1=(struct _fat_ptr)((struct _fat_ptr)*((struct _fat_ptr*)_check_fat_subscript(argv,sizeof(struct _fat_ptr),0)));_tmp43;});struct Cyc_String_pa_PrintArg_struct _tmp12=({struct Cyc_String_pa_PrintArg_struct _tmp42;_tmp42.tag=0U,_tmp42.f1=(struct _fat_ptr)((struct _fat_ptr)*((struct _fat_ptr*)_check_fat_subscript(argv,sizeof(struct _fat_ptr),Cyc_optind - 1)));_tmp42;});void*_tmpF[2U];_tmpF[0]=& _tmp11,_tmpF[1]=& _tmp12;({struct Cyc___cycFILE*_tmp6D=Cyc_stderr;struct _fat_ptr _tmp6C=({const char*_tmp10="%s: option `%s' requires an argument\n";_tag_fat(_tmp10,sizeof(char),38U);});Cyc_fprintf(_tmp6D,_tmp6C,_tag_fat(_tmpF,sizeof(void*),2U));});});
# 549
_fat_ptr_inplace_plus(& Cyc_nextchar,sizeof(char),(int)Cyc_strlen((struct _fat_ptr)Cyc_nextchar));
# 554
Cyc_optopt=((const struct Cyc_option*)_check_fat_subscript(pfound,sizeof(struct Cyc_option),0U))->val;
return(int)((int)*((const char*)_check_fat_subscript(optstring,sizeof(char),0))== (int)':'?':':'?');}}}
# 515
_fat_ptr_inplace_plus(& Cyc_nextchar,sizeof(char),(int)Cyc_strlen((struct _fat_ptr)Cyc_nextchar));
# 559
if(longind != 0)
*longind=option_index;
# 559
if((unsigned)((const struct Cyc_option*)_check_fat_subscript(pfound,sizeof(struct Cyc_option),0U))->flag){
# 563
({int _tmp6E=((const struct Cyc_option*)_check_fat_subscript(pfound,sizeof(struct Cyc_option),0U))->val;*((int*)_check_null(((const struct Cyc_option*)_check_fat_subscript(pfound,sizeof(struct Cyc_option),0U))->flag))=_tmp6E;});
return 0;}
# 559
return((const struct Cyc_option*)_check_fat_subscript(pfound,sizeof(struct Cyc_option),0U))->val;}
# 474
if(
# 573
(!long_only ||(int)*((char*)_check_fat_subscript(*((struct _fat_ptr*)_check_fat_subscript(argv,sizeof(struct _fat_ptr),Cyc_optind)),sizeof(char),1))== (int)'-')||({
char*_tmp6F=(char*)(Cyc_strchr(optstring,*((char*)_check_fat_subscript(Cyc_nextchar,sizeof(char),0U)))).curr;_tmp6F == (char*)(_tag_fat(0,0,0)).curr;})){
# 576
if(print_errors){
# 578
if((int)*((char*)_check_fat_subscript(*((struct _fat_ptr*)_check_fat_subscript(argv,sizeof(struct _fat_ptr),Cyc_optind)),sizeof(char),1))== (int)'-')
# 580
({struct Cyc_String_pa_PrintArg_struct _tmp15=({struct Cyc_String_pa_PrintArg_struct _tmp45;_tmp45.tag=0U,_tmp45.f1=(struct _fat_ptr)((struct _fat_ptr)*((struct _fat_ptr*)_check_fat_subscript(argv,sizeof(struct _fat_ptr),0)));_tmp45;});struct Cyc_String_pa_PrintArg_struct _tmp16=({struct Cyc_String_pa_PrintArg_struct _tmp44;_tmp44.tag=0U,_tmp44.f1=(struct _fat_ptr)((struct _fat_ptr)Cyc_nextchar);_tmp44;});void*_tmp13[2U];_tmp13[0]=& _tmp15,_tmp13[1]=& _tmp16;({struct Cyc___cycFILE*_tmp71=Cyc_stderr;struct _fat_ptr _tmp70=({const char*_tmp14="%s: unrecognized option `--%s'\n";_tag_fat(_tmp14,sizeof(char),32U);});Cyc_fprintf(_tmp71,_tmp70,_tag_fat(_tmp13,sizeof(void*),2U));});});else{
# 584
({struct Cyc_String_pa_PrintArg_struct _tmp19=({struct Cyc_String_pa_PrintArg_struct _tmp48;_tmp48.tag=0U,_tmp48.f1=(struct _fat_ptr)((struct _fat_ptr)*((struct _fat_ptr*)_check_fat_subscript(argv,sizeof(struct _fat_ptr),0)));_tmp48;});struct Cyc_Int_pa_PrintArg_struct _tmp1A=({struct Cyc_Int_pa_PrintArg_struct _tmp47;_tmp47.tag=1U,_tmp47.f1=(unsigned long)((int)*((char*)_check_fat_subscript(*((struct _fat_ptr*)_check_fat_subscript(argv,sizeof(struct _fat_ptr),Cyc_optind)),sizeof(char),0)));_tmp47;});struct Cyc_String_pa_PrintArg_struct _tmp1B=({struct Cyc_String_pa_PrintArg_struct _tmp46;_tmp46.tag=0U,_tmp46.f1=(struct _fat_ptr)((struct _fat_ptr)Cyc_nextchar);_tmp46;});void*_tmp17[3U];_tmp17[0]=& _tmp19,_tmp17[1]=& _tmp1A,_tmp17[2]=& _tmp1B;({struct Cyc___cycFILE*_tmp73=Cyc_stderr;struct _fat_ptr _tmp72=({const char*_tmp18="%s: unrecognized option `%c%s'\n";_tag_fat(_tmp18,sizeof(char),32U);});Cyc_fprintf(_tmp73,_tmp72,_tag_fat(_tmp17,sizeof(void*),3U));});});}}
# 576
Cyc_nextchar=({char*_tmp1E=({unsigned _tmp1D=1U + 1U;char*_tmp1C=_cycalloc_atomic(_check_times(_tmp1D,sizeof(char)));({{unsigned _tmp49=1U;unsigned i;for(i=0;i < _tmp49;++ i){_tmp1C[i]='\000';}_tmp1C[_tmp49]=0;}0;});_tmp1C;});_tag_fat(_tmp1E,sizeof(char),2U);});
# 588
++ Cyc_optind;
Cyc_optopt=0;
return(int)'?';}}{
# 597
char c=*((char*)_check_fat_subscript(_fat_ptr_inplace_plus_post(& Cyc_nextchar,sizeof(char),1),sizeof(char),0U));
struct _fat_ptr temp=Cyc_strchr(optstring,c);
# 601
if((int)*((char*)_check_fat_subscript(Cyc_nextchar,sizeof(char),0U))== (int)'\000')
++ Cyc_optind;
# 601
if(
# 604
({char*_tmp74=(char*)temp.curr;_tmp74 == (char*)(_tag_fat(0,0,0)).curr;})||(int)c == (int)':'){
# 606
if(print_errors){
# 608
if((unsigned)Cyc_posixly_correct.curr)
# 610
({struct Cyc_String_pa_PrintArg_struct _tmp21=({struct Cyc_String_pa_PrintArg_struct _tmp4B;_tmp4B.tag=0U,_tmp4B.f1=(struct _fat_ptr)((struct _fat_ptr)*((struct _fat_ptr*)_check_fat_subscript(argv,sizeof(struct _fat_ptr),0)));_tmp4B;});struct Cyc_Int_pa_PrintArg_struct _tmp22=({struct Cyc_Int_pa_PrintArg_struct _tmp4A;_tmp4A.tag=1U,_tmp4A.f1=(unsigned long)((int)c);_tmp4A;});void*_tmp1F[2U];_tmp1F[0]=& _tmp21,_tmp1F[1]=& _tmp22;({struct Cyc___cycFILE*_tmp76=Cyc_stderr;struct _fat_ptr _tmp75=({const char*_tmp20="%s: illegal option -- %c\n";_tag_fat(_tmp20,sizeof(char),26U);});Cyc_fprintf(_tmp76,_tmp75,_tag_fat(_tmp1F,sizeof(void*),2U));});});else{
# 613
({struct Cyc_String_pa_PrintArg_struct _tmp25=({struct Cyc_String_pa_PrintArg_struct _tmp4D;_tmp4D.tag=0U,_tmp4D.f1=(struct _fat_ptr)((struct _fat_ptr)*((struct _fat_ptr*)_check_fat_subscript(argv,sizeof(struct _fat_ptr),0)));_tmp4D;});struct Cyc_Int_pa_PrintArg_struct _tmp26=({struct Cyc_Int_pa_PrintArg_struct _tmp4C;_tmp4C.tag=1U,_tmp4C.f1=(unsigned long)((int)c);_tmp4C;});void*_tmp23[2U];_tmp23[0]=& _tmp25,_tmp23[1]=& _tmp26;({struct Cyc___cycFILE*_tmp78=Cyc_stderr;struct _fat_ptr _tmp77=({const char*_tmp24="%s: invalid option -- %c\n";_tag_fat(_tmp24,sizeof(char),26U);});Cyc_fprintf(_tmp78,_tmp77,_tag_fat(_tmp23,sizeof(void*),2U));});});}}
# 606
Cyc_optopt=(int)c;
# 617
return(int)'?';}
# 601
if(
# 620
(int)*((const char*)_check_fat_subscript(temp,sizeof(char),0))== (int)'W' &&(int)*((const char*)_check_fat_subscript(temp,sizeof(char),1))== (int)';'){
# 622
struct _fat_ptr nameend;
struct _fat_ptr p;
struct _fat_ptr pfound=_tag_fat(0,0,0);
int exact=0;
int ambig=0;
int indfound=0;
int option_index;
# 631
if((int)*((char*)_check_fat_subscript(Cyc_nextchar,sizeof(char),0U))!= (int)'\000'){
# 633
Cyc_optarg=Cyc_nextchar;
# 636
++ Cyc_optind;}else{
# 638
if(Cyc_optind == argc){
# 640
if(print_errors)
# 643
({struct Cyc_String_pa_PrintArg_struct _tmp29=({struct Cyc_String_pa_PrintArg_struct _tmp4F;_tmp4F.tag=0U,_tmp4F.f1=(struct _fat_ptr)((struct _fat_ptr)*((struct _fat_ptr*)_check_fat_subscript(argv,sizeof(struct _fat_ptr),0)));_tmp4F;});struct Cyc_Int_pa_PrintArg_struct _tmp2A=({struct Cyc_Int_pa_PrintArg_struct _tmp4E;_tmp4E.tag=1U,_tmp4E.f1=(unsigned long)((int)c);_tmp4E;});void*_tmp27[2U];_tmp27[0]=& _tmp29,_tmp27[1]=& _tmp2A;({struct Cyc___cycFILE*_tmp7A=Cyc_stderr;struct _fat_ptr _tmp79=({const char*_tmp28="%s: option requires an argument -- %c\n";_tag_fat(_tmp28,sizeof(char),39U);});Cyc_fprintf(_tmp7A,_tmp79,_tag_fat(_tmp27,sizeof(void*),2U));});});
# 640
Cyc_optopt=(int)c;
# 647
if((int)*((const char*)_check_fat_subscript(optstring,sizeof(char),0))== (int)':')
c=':';else{
# 650
c='?';}
return(int)c;}else{
# 656
Cyc_optarg=*((struct _fat_ptr*)_check_fat_subscript(argv,sizeof(struct _fat_ptr),Cyc_optind ++));}}
# 661
for(Cyc_nextchar=(nameend=Cyc_optarg);(int)*((char*)_check_fat_subscript(nameend,sizeof(char),0U))&&(int)*((char*)_check_fat_subscript(nameend,sizeof(char),0U))!= (int)'=';_fat_ptr_inplace_plus(& nameend,sizeof(char),1)){
;}
# 666
for((p=longopts,option_index=0);(unsigned)(((const struct Cyc_option*)_check_fat_subscript(p,sizeof(struct Cyc_option),0U))->name).curr;(_fat_ptr_inplace_plus_post(& p,sizeof(struct Cyc_option),1),option_index ++)){
if(!Cyc_strncmp((struct _fat_ptr)((const struct Cyc_option*)_check_fat_subscript(p,sizeof(struct Cyc_option),0U))->name,(struct _fat_ptr)Cyc_nextchar,(unsigned)((nameend.curr - Cyc_nextchar.curr)/ sizeof(char)))){
# 669
if(({unsigned _tmp7B=(unsigned)((nameend.curr - Cyc_nextchar.curr)/ sizeof(char));_tmp7B == Cyc_strlen((struct _fat_ptr)((const struct Cyc_option*)_check_fat_subscript(p,sizeof(struct Cyc_option),0U))->name);})){
# 672
pfound=_tag_fat((const struct Cyc_option*)_untag_fat_ptr(p,sizeof(struct Cyc_option),1U),sizeof(struct Cyc_option),1U);
indfound=option_index;
exact=1;
break;}else{
# 677
if(({struct Cyc_option*_tmp7C=(struct Cyc_option*)pfound.curr;_tmp7C == (struct Cyc_option*)(_tag_fat(0,0,0)).curr;})){
# 680
pfound=_tag_fat((const struct Cyc_option*)_untag_fat_ptr(p,sizeof(struct Cyc_option),1U),sizeof(struct Cyc_option),1U);
indfound=option_index;}else{
# 685
ambig=1;}}}}
# 666
if(
# 687
ambig && !exact){
# 689
if(print_errors)
({struct Cyc_String_pa_PrintArg_struct _tmp2D=({struct Cyc_String_pa_PrintArg_struct _tmp51;_tmp51.tag=0U,_tmp51.f1=(struct _fat_ptr)((struct _fat_ptr)*((struct _fat_ptr*)_check_fat_subscript(argv,sizeof(struct _fat_ptr),0)));_tmp51;});struct Cyc_String_pa_PrintArg_struct _tmp2E=({struct Cyc_String_pa_PrintArg_struct _tmp50;_tmp50.tag=0U,_tmp50.f1=(struct _fat_ptr)((struct _fat_ptr)*((struct _fat_ptr*)_check_fat_subscript(argv,sizeof(struct _fat_ptr),Cyc_optind)));_tmp50;});void*_tmp2B[2U];_tmp2B[0]=& _tmp2D,_tmp2B[1]=& _tmp2E;({struct Cyc___cycFILE*_tmp7E=Cyc_stderr;struct _fat_ptr _tmp7D=({const char*_tmp2C="%s: option `-W %s' is ambiguous\n";_tag_fat(_tmp2C,sizeof(char),33U);});Cyc_fprintf(_tmp7E,_tmp7D,_tag_fat(_tmp2B,sizeof(void*),2U));});});
# 689
_fat_ptr_inplace_plus(& Cyc_nextchar,sizeof(char),(int)Cyc_strlen((struct _fat_ptr)Cyc_nextchar));
# 693
++ Cyc_optind;
return(int)'?';}
# 666
if(({
# 696
struct Cyc_option*_tmp7F=(struct Cyc_option*)pfound.curr;_tmp7F != (struct Cyc_option*)(_tag_fat(0,0,0)).curr;})){
# 698
option_index=indfound;
if((int)*((char*)_check_fat_subscript(nameend,sizeof(char),0U))){
# 703
if(((const struct Cyc_option*)_check_fat_subscript(pfound,sizeof(struct Cyc_option),0U))->has_arg)
Cyc_optarg=_fat_ptr_plus(nameend,sizeof(char),1);else{
# 707
if(print_errors)
({struct Cyc_String_pa_PrintArg_struct _tmp31=({struct Cyc_String_pa_PrintArg_struct _tmp53;_tmp53.tag=0U,_tmp53.f1=(struct _fat_ptr)((struct _fat_ptr)*((struct _fat_ptr*)_check_fat_subscript(argv,sizeof(struct _fat_ptr),0)));_tmp53;});struct Cyc_String_pa_PrintArg_struct _tmp32=({struct Cyc_String_pa_PrintArg_struct _tmp52;_tmp52.tag=0U,_tmp52.f1=(struct _fat_ptr)((struct _fat_ptr)((const struct Cyc_option*)_check_fat_subscript(pfound,sizeof(struct Cyc_option),0U))->name);_tmp52;});void*_tmp2F[2U];_tmp2F[0]=& _tmp31,_tmp2F[1]=& _tmp32;({struct Cyc___cycFILE*_tmp81=Cyc_stderr;struct _fat_ptr _tmp80=({const char*_tmp30="%s: option `-W %s' doesn't allow an argument\n";_tag_fat(_tmp30,sizeof(char),46U);});Cyc_fprintf(_tmp81,_tmp80,_tag_fat(_tmp2F,sizeof(void*),2U));});});
# 707
_fat_ptr_inplace_plus(& Cyc_nextchar,sizeof(char),(int)Cyc_strlen((struct _fat_ptr)Cyc_nextchar));
# 713
return(int)'?';}}else{
# 716
if(((const struct Cyc_option*)_check_fat_subscript(pfound,sizeof(struct Cyc_option),0U))->has_arg == 1){
# 718
if(Cyc_optind < argc)
Cyc_optarg=*((struct _fat_ptr*)_check_fat_subscript(argv,sizeof(struct _fat_ptr),Cyc_optind ++));else{
# 722
if(print_errors)
({struct Cyc_String_pa_PrintArg_struct _tmp35=({struct Cyc_String_pa_PrintArg_struct _tmp55;_tmp55.tag=0U,_tmp55.f1=(struct _fat_ptr)((struct _fat_ptr)*((struct _fat_ptr*)_check_fat_subscript(argv,sizeof(struct _fat_ptr),0)));_tmp55;});struct Cyc_String_pa_PrintArg_struct _tmp36=({struct Cyc_String_pa_PrintArg_struct _tmp54;_tmp54.tag=0U,_tmp54.f1=(struct _fat_ptr)((struct _fat_ptr)*((struct _fat_ptr*)_check_fat_subscript(argv,sizeof(struct _fat_ptr),Cyc_optind - 1)));_tmp54;});void*_tmp33[2U];_tmp33[0]=& _tmp35,_tmp33[1]=& _tmp36;({struct Cyc___cycFILE*_tmp83=Cyc_stderr;struct _fat_ptr _tmp82=({const char*_tmp34="%s: option `%s' requires an argument\n";_tag_fat(_tmp34,sizeof(char),38U);});Cyc_fprintf(_tmp83,_tmp82,_tag_fat(_tmp33,sizeof(void*),2U));});});
# 722
_fat_ptr_inplace_plus(& Cyc_nextchar,sizeof(char),(int)Cyc_strlen((struct _fat_ptr)Cyc_nextchar));
# 727
return(int)((int)*((const char*)_check_fat_subscript(optstring,sizeof(char),0))== (int)':'?':':'?');}}}
# 699
_fat_ptr_inplace_plus(& Cyc_nextchar,sizeof(char),(int)Cyc_strlen((struct _fat_ptr)Cyc_nextchar));
# 731
if(longind != 0)
*longind=option_index;
# 731
if((unsigned)((const struct Cyc_option*)_check_fat_subscript(pfound,sizeof(struct Cyc_option),0U))->flag){
# 735
({int _tmp84=((const struct Cyc_option*)_check_fat_subscript(pfound,sizeof(struct Cyc_option),0U))->val;*((int*)_check_null(((const struct Cyc_option*)_check_fat_subscript(pfound,sizeof(struct Cyc_option),0U))->flag))=_tmp84;});
return 0;}
# 731
return((const struct Cyc_option*)_check_fat_subscript(pfound,sizeof(struct Cyc_option),0U))->val;}
# 666
Cyc_nextchar=
# 740
_tag_fat(0,0,0);
return(int)'W';}
# 601
if((int)*((const char*)_check_fat_subscript(temp,sizeof(char),1))== (int)':'){
# 745
if((int)*((const char*)_check_fat_subscript(temp,sizeof(char),2))== (int)':'){
# 748
if((int)*((char*)_check_fat_subscript(Cyc_nextchar,sizeof(char),0U))!= (int)'\000'){
# 750
Cyc_optarg=Cyc_nextchar;
++ Cyc_optind;}else{
# 754
Cyc_optarg=_tag_fat(0,0,0);}
Cyc_nextchar=_tag_fat(0,0,0);}else{
# 760
if((int)*((char*)_check_fat_subscript(Cyc_nextchar,sizeof(char),0U))!= (int)'\000'){
# 762
Cyc_optarg=Cyc_nextchar;
# 765
++ Cyc_optind;}else{
# 767
if(Cyc_optind == argc){
# 769
if(print_errors)
# 772
({struct Cyc_String_pa_PrintArg_struct _tmp39=({struct Cyc_String_pa_PrintArg_struct _tmp57;_tmp57.tag=0U,_tmp57.f1=(struct _fat_ptr)((struct _fat_ptr)*((struct _fat_ptr*)_check_fat_subscript(argv,sizeof(struct _fat_ptr),0)));_tmp57;});struct Cyc_Int_pa_PrintArg_struct _tmp3A=({struct Cyc_Int_pa_PrintArg_struct _tmp56;_tmp56.tag=1U,_tmp56.f1=(unsigned long)((int)c);_tmp56;});void*_tmp37[2U];_tmp37[0]=& _tmp39,_tmp37[1]=& _tmp3A;({struct Cyc___cycFILE*_tmp86=Cyc_stderr;struct _fat_ptr _tmp85=({const char*_tmp38="%s: option requires an argument -- %c\n";_tag_fat(_tmp38,sizeof(char),39U);});Cyc_fprintf(_tmp86,_tmp85,_tag_fat(_tmp37,sizeof(void*),2U));});});
# 769
Cyc_optopt=(int)c;
# 777
if((int)*((const char*)_check_fat_subscript(optstring,sizeof(char),0))== (int)':')
c=':';else{
# 780
c='?';}}else{
# 785
Cyc_optarg=*((struct _fat_ptr*)_check_fat_subscript(argv,sizeof(struct _fat_ptr),Cyc_optind ++));}}
Cyc_nextchar=_tag_fat(0,0,0);}}
# 601
return(int)c;}}
# 170 "getopt.cyc"
int Cyc_getopt(int argc,struct _fat_ptr argv,struct _fat_ptr optstring){
# 796 "getopt.cyc"
return({int _tmp89=argc;struct _fat_ptr _tmp88=argv;struct _fat_ptr _tmp87=optstring;Cyc__getopt_internal(_tmp89,_tmp88,_tmp87,(struct _fat_ptr)
_tag_fat(0,0,0),0,0);});}

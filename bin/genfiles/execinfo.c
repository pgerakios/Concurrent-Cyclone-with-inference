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
extern struct _RegionHandle*Cyc_Core_unique_region;struct Cyc_Core_DynamicRegion;struct Cyc_Core_NewDynamicRegion{struct Cyc_Core_DynamicRegion*key;};struct Cyc_Core_ThinRes{void*arr;unsigned nelts;};struct _tuple0{int quot;int rem;};
# 105 "stdlib.h"
void exit(int);
# 23 "execinfo.h"
int Cyc_Execinfo_backtrace(struct _fat_ptr,int);union _tuple1{unsigned __wch;char __wchb[4U];};struct _tuple2{int __count;union _tuple1 __value;};struct _tuple3{long __pos;struct _tuple2 __state;};struct Cyc___cycFILE;struct __abstractFILE;struct Cyc_String_pa_PrintArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Int_pa_PrintArg_struct{int tag;unsigned long f1;};struct Cyc_Double_pa_PrintArg_struct{int tag;double f1;};struct Cyc_LongDouble_pa_PrintArg_struct{int tag;long double f1;};struct Cyc_ShortPtr_pa_PrintArg_struct{int tag;short*f1;};struct Cyc_IntPtr_pa_PrintArg_struct{int tag;unsigned long*f1;};
# 118 "stdio.h"
struct _fat_ptr Cyc_aprintf(struct _fat_ptr,struct _fat_ptr);struct Cyc_ShortPtr_sa_ScanfArg_struct{int tag;short*f1;};struct Cyc_UShortPtr_sa_ScanfArg_struct{int tag;unsigned short*f1;};struct Cyc_IntPtr_sa_ScanfArg_struct{int tag;int*f1;};struct Cyc_UIntPtr_sa_ScanfArg_struct{int tag;unsigned*f1;};struct Cyc_StringPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_DoublePtr_sa_ScanfArg_struct{int tag;double*f1;};struct Cyc_FloatPtr_sa_ScanfArg_struct{int tag;float*f1;};struct Cyc_CharPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};
# 221
void perror(const char*);
# 225
int Cyc_printf(struct _fat_ptr,struct _fat_ptr);
# 231
int putchar(int);
# 267
int Cyc_sprintf(struct _fat_ptr,struct _fat_ptr,struct _fat_ptr);extern char Cyc_FileCloseError[15U];struct Cyc_FileCloseError_exn_struct{char*tag;};extern char Cyc_FileOpenError[14U];struct Cyc_FileOpenError_exn_struct{char*tag;struct _fat_ptr f1;};struct Cyc_timeval{long tv_sec;long tv_usec;};struct Cyc_rusage{struct Cyc_timeval ru_utime;struct Cyc_timeval ru_stime;long ru_maxrss;long ru_ixrss;long ru_idrss;long ru_isrss;long ru_minflt;long ru_majflt;long ru_nswap;long ru_inblock;long ru_oublock;long ru_msgsnd;long ru_msgrcv;long ru_nsignals;long ru_nvcsw;long ru_nivcsw;};
# 145 "wait.h"
int waitpid(int,int*,int);struct _tuple4{unsigned long __val[1024U / (8U * sizeof(unsigned long))];};union Cyc_sigval{int sival_int;void*sival_ptr;};struct _tuple5{int si_pid;unsigned si_uid;};struct _tuple6{int si_tid;int si_overrun;union Cyc_sigval si_sigval;};struct _tuple7{int si_pid;unsigned si_uid;union Cyc_sigval si_sigval;};struct _tuple8{int si_pid;unsigned si_uid;int si_status;long si_utime;long si_stime;};struct _tuple9{void*si_addr;};struct _tuple10{long si_band;int si_fd;};union _tuple11{int _pad[128U / sizeof(int)- 3U];struct _tuple5 _kill;struct _tuple6 _timer;struct _tuple7 _rt;struct _tuple8 _sigchld;struct _tuple9 _sigfault;struct _tuple10 _sigpoll;};struct Cyc_siginfo{int si_signo;int si_errno;int si_code;union _tuple11 _sifields;};
# 69 "signal.h"
enum Cyc___anonymous_enum_60__{Cyc_SIGEV_SIGNAL =0,Cyc_SIGEV_NONE =1U,Cyc_SIGEV_THREAD =2U,Cyc_SIGEV_THREAD_ID =4};union _tuple12{void(*sa_handler)(int);void(*sa_sigaction)(int,struct Cyc_siginfo*,void*);};struct Cyc_sigaction{union _tuple12 __sigaction_handler;struct _tuple4 sa_mask;int sa_flags;void(*sa_restorer)();};
# 238 "signal.h"
int kill(int,int);
# 53 "unistd.h"
enum Cyc___anonymous_enum_103__{Cyc__PC_LINK_MAX =0U,Cyc__PC_MAX_CANON =1U,Cyc__PC_MAX_INPUT =2U,Cyc__PC_NAME_MAX =3U,Cyc__PC_PATH_MAX =4U,Cyc__PC_PIPE_BUF =5U,Cyc__PC_CHOWN_RESTRICTED =6U,Cyc__PC_NO_TRUNC =7U,Cyc__PC_VDISABLE =8U,Cyc__PC_SYNC_IO =9U,Cyc__PC_ASYNC_IO =10U,Cyc__PC_PRIO_IO =11U,Cyc__PC_SOCK_MAXBUF =12U,Cyc__PC_FILESIZEBITS =13U,Cyc__PC_REC_INCR_XFER_SIZE =14U,Cyc__PC_REC_MAX_XFER_SIZE =15U,Cyc__PC_REC_MIN_XFER_SIZE =16U,Cyc__PC_REC_XFER_ALIGN =17U,Cyc__PC_ALLOC_SIZE_MIN =18U,Cyc__PC_SYMLINK_MAX =19U,Cyc__PC_2_SYMLINKS =20U};
# 79
enum Cyc___anonymous_enum_105__{Cyc__CS_PATH =0U,Cyc__CS_V6_WIDTH_RESTRICTED_ENVS =1U,Cyc__CS_GNU_LIBC_VERSION =2U,Cyc__CS_GNU_LIBPTHREAD_VERSION =3U,Cyc__CS_LFS_CFLAGS =1000,Cyc__CS_LFS_LDFLAGS =1001U,Cyc__CS_LFS_LIBS =1002U,Cyc__CS_LFS_LINTFLAGS =1003U,Cyc__CS_LFS64_CFLAGS =1004U,Cyc__CS_LFS64_LDFLAGS =1005U,Cyc__CS_LFS64_LIBS =1006U,Cyc__CS_LFS64_LINTFLAGS =1007U,Cyc__CS_XBS5_ILP32_OFF32_CFLAGS =1100,Cyc__CS_XBS5_ILP32_OFF32_LDFLAGS =1101U,Cyc__CS_XBS5_ILP32_OFF32_LIBS =1102U,Cyc__CS_XBS5_ILP32_OFF32_LINTFLAGS =1103U,Cyc__CS_XBS5_ILP32_OFFBIG_CFLAGS =1104U,Cyc__CS_XBS5_ILP32_OFFBIG_LDFLAGS =1105U,Cyc__CS_XBS5_ILP32_OFFBIG_LIBS =1106U,Cyc__CS_XBS5_ILP32_OFFBIG_LINTFLAGS =1107U,Cyc__CS_XBS5_LP64_OFF64_CFLAGS =1108U,Cyc__CS_XBS5_LP64_OFF64_LDFLAGS =1109U,Cyc__CS_XBS5_LP64_OFF64_LIBS =1110U,Cyc__CS_XBS5_LP64_OFF64_LINTFLAGS =1111U,Cyc__CS_XBS5_LPBIG_OFFBIG_CFLAGS =1112U,Cyc__CS_XBS5_LPBIG_OFFBIG_LDFLAGS =1113U,Cyc__CS_XBS5_LPBIG_OFFBIG_LIBS =1114U,Cyc__CS_XBS5_LPBIG_OFFBIG_LINTFLAGS =1115U,Cyc__CS_POSIX_V6_ILP32_OFF32_CFLAGS =1116U,Cyc__CS_POSIX_V6_ILP32_OFF32_LDFLAGS =1117U,Cyc__CS_POSIX_V6_ILP32_OFF32_LIBS =1118U,Cyc__CS_POSIX_V6_ILP32_OFF32_LINTFLAGS =1119U,Cyc__CS_POSIX_V6_ILP32_OFFBIG_CFLAGS =1120U,Cyc__CS_POSIX_V6_ILP32_OFFBIG_LDFLAGS =1121U,Cyc__CS_POSIX_V6_ILP32_OFFBIG_LIBS =1122U,Cyc__CS_POSIX_V6_ILP32_OFFBIG_LINTFLAGS =1123U,Cyc__CS_POSIX_V6_LP64_OFF64_CFLAGS =1124U,Cyc__CS_POSIX_V6_LP64_OFF64_LDFLAGS =1125U,Cyc__CS_POSIX_V6_LP64_OFF64_LIBS =1126U,Cyc__CS_POSIX_V6_LP64_OFF64_LINTFLAGS =1127U,Cyc__CS_POSIX_V6_LPBIG_OFFBIG_CFLAGS =1128U,Cyc__CS_POSIX_V6_LPBIG_OFFBIG_LDFLAGS =1129U,Cyc__CS_POSIX_V6_LPBIG_OFFBIG_LIBS =1130U,Cyc__CS_POSIX_V6_LPBIG_OFFBIG_LINTFLAGS =1131U};struct Cyc_option{struct _fat_ptr name;int has_arg;int*flag;int val;};
# 611 "unistd.h"
int close(int);
# 618
int dup2(int,int);
# 636
int execvp(const char*file,const char**argv);
# 646
int fork();
# 680 "unistd.h"
int getpid();
# 707
int pipe(int*);
# 709
int Cyc_read(int,struct _fat_ptr,unsigned);
# 762
int Cyc_write(int,struct _fat_ptr,unsigned);
# 32 "execinfo.cyc"
int backtrace(int*,int);
# 38
int Cyc_Execinfo_backtrace(struct _fat_ptr array,int size){
# 40
if((unsigned)size > _get_fat_size(array,sizeof(int)))
(int)_throw((void*)({struct Cyc_Core_Failure_exn_struct*_tmp1=_cycalloc(sizeof(*_tmp1));_tmp1->tag=Cyc_Core_Failure,({struct _fat_ptr _tmp1D=({const char*_tmp0="backtrace: size > numelts(array)";_tag_fat(_tmp0,sizeof(char),33U);});_tmp1->f1=_tmp1D;});_tmp1;}));
# 40
return
# 42
 backtrace((int*)_untag_fat_ptr(array,sizeof(int),1U),size);}
# 38
int Cyc_Execinfo_bt(){
# 53
int bt[20U];({{unsigned _tmp1C=20U;unsigned i;for(i=0;i < _tmp1C;++ i){bt[i]=0;}}0;});{
int pid;int self_pid;
# 56
int tochild[2U];tochild[0]=0,tochild[1]=0;{int fromchild[2U];fromchild[0]=0,fromchild[1]=0;
if(pipe(tochild)|| pipe(fromchild))return 1;self_pid=
# 59
getpid();
# 61
if((pid=fork())== 0){
if(dup2(tochild[0],0)< 0){
perror("dup failed in backtrace");
exit(1);}
# 62
close(tochild[1]);
# 67
if(dup2(fromchild[1],1)< 0){
perror("dup failed in backtrace");
exit(1);}
# 67
close(fromchild[0]);{
# 73
const char*args[5U];args[0]="addr2line",args[1]="--functions",args[2]="-e",args[3]="",args[4]=0;
({struct _fat_ptr _tmp3=_fat_ptr_plus(({const char**_tmp2=args;_tag_fat(_tmp2,sizeof(const char*),5U);}),sizeof(const char*),3);const char*_tmp4=*((const char**)_check_fat_subscript(_tmp3,sizeof(const char*),0U));const char*_tmp8=(const char*)_untag_fat_ptr(({struct Cyc_Int_pa_PrintArg_struct _tmp7=({struct Cyc_Int_pa_PrintArg_struct _tmp18;_tmp18.tag=1U,_tmp18.f1=(unsigned long)self_pid;_tmp18;});void*_tmp5[1U];_tmp5[0]=& _tmp7;({struct _fat_ptr _tmp1E=({const char*_tmp6="/proc/%d/exe";_tag_fat(_tmp6,sizeof(char),13U);});Cyc_aprintf(_tmp1E,_tag_fat(_tmp5,sizeof(void*),1U));});}),sizeof(char),1U);if(_get_fat_size(_tmp3,sizeof(const char*))== 1U &&(_tmp4 == 0 && _tmp8 != 0))_throw_arraybounds();*((const char**)_tmp3.curr)=_tmp8;});
if(execvp("addr2line",args)== - 1)
perror("execlp failed during backtrace");
# 75
exit(1);}}else{
# 80
if(pid < 0){
close(tochild[0]);close(tochild[1]);
close(fromchild[0]);close(fromchild[1]);
return 1;}}
# 61
close(tochild[0]);
# 88
close(fromchild[1]);{
int infd=fromchild[0];
int outfd=tochild[1];
# 93
int n=Cyc_Execinfo_backtrace(_tag_fat(bt,sizeof(int),20U),(int)20U);
# 95
{int c=0;for(0;c < n;++ c){
char buf[100U];
int len=({struct Cyc_Int_pa_PrintArg_struct _tmpB=({struct Cyc_Int_pa_PrintArg_struct _tmp19;_tmp19.tag=1U,_tmp19.f1=(unsigned)*((int*)_check_known_subscript_notnull(bt,20U,sizeof(int),c));_tmp19;});void*_tmp9[1U];_tmp9[0]=& _tmpB;({struct _fat_ptr _tmp20=_tag_fat(buf,sizeof(char),100U);struct _fat_ptr _tmp1F=({const char*_tmpA="%#x\n";_tag_fat(_tmpA,sizeof(char),5U);});Cyc_sprintf(_tmp20,_tmp1F,_tag_fat(_tmp9,sizeof(void*),1U));});});
({int _tmp22=outfd;struct _fat_ptr _tmp21=_tag_fat(buf,sizeof(char),100U);Cyc_write(_tmp22,_tmp21,(unsigned)len);});}}
# 102
({void*_tmpC=0U;({struct _fat_ptr _tmp23=({const char*_tmpD="Backtrace:\n  Function          Location\n  ----------------  --------------------------------\n";_tag_fat(_tmpD,sizeof(char),94U);});Cyc_printf(_tmp23,_tag_fat(_tmpC,sizeof(void*),0U));});});
# 105
{int c=0;for(0;c < n;++ c){
int unknown=1;
char d;
int ret;int pos=0;
# 110
({void*_tmpE=0U;({struct _fat_ptr _tmp24=({const char*_tmpF="  ";_tag_fat(_tmpF,sizeof(char),3U);});Cyc_printf(_tmp24,_tag_fat(_tmpE,sizeof(void*),0U));});});
do{
ret=({int _tmp25=infd;Cyc_read(_tmp25,_tag_fat(& d,sizeof(char),1U),1U);});
if((int)d == (int)'\n')break;if((int)d != (int)'?')
unknown=0;
# 113
++ pos;
# 116
putchar((int)d);}while(1);
# 119
if(unknown){
char buf[100U];
int len=({struct Cyc_Int_pa_PrintArg_struct _tmp15=({struct Cyc_Int_pa_PrintArg_struct _tmp1B;_tmp1B.tag=1U,_tmp1B.f1=(unsigned)*((int*)_check_known_subscript_notnull(bt,20U,sizeof(int),c));_tmp1B;});void*_tmp13[1U];_tmp13[0]=& _tmp15;({struct _fat_ptr _tmp27=_tag_fat(buf,sizeof(char),100U);struct _fat_ptr _tmp26=({const char*_tmp14="(%#x)";_tag_fat(_tmp14,sizeof(char),6U);});Cyc_sprintf(_tmp27,_tmp26,_tag_fat(_tmp13,sizeof(void*),1U));});});
({struct Cyc_String_pa_PrintArg_struct _tmp12=({struct Cyc_String_pa_PrintArg_struct _tmp1A;_tmp1A.tag=0U,({struct _fat_ptr _tmp28=(struct _fat_ptr)_tag_fat(buf,sizeof(char),100U);_tmp1A.f1=_tmp28;});_tmp1A;});void*_tmp10[1U];_tmp10[0]=& _tmp12;({struct _fat_ptr _tmp29=({const char*_tmp11="%s";_tag_fat(_tmp11,sizeof(char),3U);});Cyc_printf(_tmp29,_tag_fat(_tmp10,sizeof(void*),1U));});});
pos +=len;}
# 119
while(pos ++ < 16){
# 127
putchar((int)' ');}
({void*_tmp16=0U;({struct _fat_ptr _tmp2A=({const char*_tmp17="  ";_tag_fat(_tmp17,sizeof(char),3U);});Cyc_printf(_tmp2A,_tag_fat(_tmp16,sizeof(void*),0U));});});
# 130
do{
ret=({int _tmp2B=infd;Cyc_read(_tmp2B,_tag_fat(& d,sizeof(char),1U),1U);});
if((int)d == (int)'\n')break;putchar((int)d);}while(1);
# 135
putchar((int)'\n');}}
# 138
close(infd);
if(infd != outfd)
close(outfd);
# 139
kill(pid,15);
# 143
waitpid(pid,0,0);
return 0;}}}}

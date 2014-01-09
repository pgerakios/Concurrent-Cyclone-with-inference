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
 struct Cyc___cycFILE;
# 53 "cycboot.h"
extern struct Cyc___cycFILE*Cyc_stderr;struct Cyc_String_pa_PrintArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Int_pa_PrintArg_struct{int tag;unsigned long f1;};struct Cyc_Double_pa_PrintArg_struct{int tag;double f1;};struct Cyc_LongDouble_pa_PrintArg_struct{int tag;long double f1;};struct Cyc_ShortPtr_pa_PrintArg_struct{int tag;short*f1;};struct Cyc_IntPtr_pa_PrintArg_struct{int tag;unsigned long*f1;};
# 79
int Cyc_fclose(struct Cyc___cycFILE*);
# 88
int Cyc_fflush(struct Cyc___cycFILE*);
# 90
int Cyc_fgetc(struct Cyc___cycFILE*);
# 98
struct Cyc___cycFILE*Cyc_fopen(const char*,const char*);
# 100
int Cyc_fprintf(struct Cyc___cycFILE*,struct _fat_ptr,struct _fat_ptr);struct Cyc_ShortPtr_sa_ScanfArg_struct{int tag;short*f1;};struct Cyc_UShortPtr_sa_ScanfArg_struct{int tag;unsigned short*f1;};struct Cyc_IntPtr_sa_ScanfArg_struct{int tag;int*f1;};struct Cyc_UIntPtr_sa_ScanfArg_struct{int tag;unsigned*f1;};struct Cyc_StringPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_DoublePtr_sa_ScanfArg_struct{int tag;double*f1;};struct Cyc_FloatPtr_sa_ScanfArg_struct{int tag;float*f1;};struct Cyc_CharPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};extern char Cyc_FileCloseError[15U];struct Cyc_FileCloseError_exn_struct{char*tag;};extern char Cyc_FileOpenError[14U];struct Cyc_FileOpenError_exn_struct{char*tag;struct _fat_ptr f1;};
# 271 "cycboot.h"
struct Cyc___cycFILE*Cyc_file_open(struct _fat_ptr,struct _fat_ptr);
# 300
int isspace(int);
# 313
char*getenv(const char*);struct Cyc_Core_Opt{void*v;};extern char Cyc_Core_Invalid_argument[17U];struct Cyc_Core_Invalid_argument_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Failure[8U];struct Cyc_Core_Failure_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Impossible[11U];struct Cyc_Core_Impossible_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Not_found[10U];struct Cyc_Core_Not_found_exn_struct{char*tag;};extern char Cyc_Core_Unreachable[12U];struct Cyc_Core_Unreachable_exn_struct{char*tag;struct _fat_ptr f1;};
# 168 "core.h"
extern struct _RegionHandle*Cyc_Core_unique_region;struct Cyc_Core_DynamicRegion;struct Cyc_Core_NewDynamicRegion{struct Cyc_Core_DynamicRegion*key;};struct Cyc_Core_ThinRes{void*arr;unsigned nelts;};struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};extern char Cyc_List_List_mismatch[14U];struct Cyc_List_List_mismatch_exn_struct{char*tag;};
# 178 "list.h"
struct Cyc_List_List*Cyc_List_imp_rev(struct Cyc_List_List*x);extern char Cyc_List_Nth[4U];struct Cyc_List_Nth_exn_struct{char*tag;};
# 336
void*Cyc_List_assoc_cmp(int(*cmp)(void*,void*),struct Cyc_List_List*l,void*x);
# 364
struct _fat_ptr Cyc_List_to_array(struct Cyc_List_List*x);
# 38 "string.h"
unsigned long Cyc_strlen(struct _fat_ptr s);
# 49 "string.h"
int Cyc_strcmp(struct _fat_ptr s1,struct _fat_ptr s2);
int Cyc_strptrcmp(struct _fat_ptr*s1,struct _fat_ptr*s2);
int Cyc_strncmp(struct _fat_ptr s1,struct _fat_ptr s2,unsigned long len);
# 62
struct _fat_ptr Cyc_strconcat(struct _fat_ptr,struct _fat_ptr);
# 104 "string.h"
struct _fat_ptr Cyc_strdup(struct _fat_ptr src);
# 109
struct _fat_ptr Cyc_substring(struct _fat_ptr,int ofs,unsigned long n);
# 30 "filename.h"
struct _fat_ptr Cyc_Filename_concat(struct _fat_ptr,struct _fat_ptr);extern char Cyc_Arg_Bad[4U];struct Cyc_Arg_Bad_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Arg_Error[6U];struct Cyc_Arg_Error_exn_struct{char*tag;};struct Cyc_Arg_Unit_spec_Arg_Spec_struct{int tag;void(*f1)();};struct Cyc_Arg_Flag_spec_Arg_Spec_struct{int tag;void(*f1)(struct _fat_ptr);};struct Cyc_Arg_FlagString_spec_Arg_Spec_struct{int tag;void(*f1)(struct _fat_ptr,struct _fat_ptr);};struct Cyc_Arg_Set_spec_Arg_Spec_struct{int tag;int*f1;};struct Cyc_Arg_Clear_spec_Arg_Spec_struct{int tag;int*f1;};struct Cyc_Arg_String_spec_Arg_Spec_struct{int tag;void(*f1)(struct _fat_ptr);};struct Cyc_Arg_Int_spec_Arg_Spec_struct{int tag;void(*f1)(int);};struct Cyc_Arg_Rest_spec_Arg_Spec_struct{int tag;void(*f1)(struct _fat_ptr);};
# 69 "arg.h"
extern int Cyc_Arg_current;
# 71
void Cyc_Arg_parse(struct Cyc_List_List*specs,void(*anonfun)(struct _fat_ptr),int(*anonflagfun)(struct _fat_ptr),struct _fat_ptr errmsg,struct _fat_ptr args);
# 29 "specsfile.h"
struct _fat_ptr Cyc_Specsfile_target_arch;
# 31
struct Cyc_List_List*Cyc_Specsfile_cyclone_exec_path;
# 37
struct Cyc_List_List*Cyc_Specsfile_cyclone_arch_path;
struct _fat_ptr Cyc_Specsfile_def_lib_path;
# 31 "specsfile.cyc"
extern char*Cdef_lib_path;
extern char*Carch;
# 38
struct _fat_ptr Cyc_Specsfile_target_arch={(void*)0,(void*)0,(void*)(0 + 0)};
void Cyc_Specsfile_set_target_arch(struct _fat_ptr s){
Cyc_Specsfile_target_arch=s;}
# 39
struct Cyc_List_List*Cyc_Specsfile_cyclone_exec_path=0;
# 44
void Cyc_Specsfile_add_cyclone_exec_path(struct _fat_ptr s){
unsigned long len=({Cyc_strlen((struct _fat_ptr)s);});
if(len <= (unsigned long)2)return;{struct _fat_ptr dir=(struct _fat_ptr)({Cyc_substring((struct _fat_ptr)s,2,len - (unsigned long)2);});
# 48
Cyc_Specsfile_cyclone_exec_path=({struct Cyc_List_List*_tmp2=_cycalloc(sizeof(*_tmp2));({struct _fat_ptr*_tmp63=({struct _fat_ptr*_tmp1=_cycalloc(sizeof(*_tmp1));*_tmp1=dir;_tmp1;});_tmp2->hd=_tmp63;}),_tmp2->tl=Cyc_Specsfile_cyclone_exec_path;_tmp2;});}}struct _tuple0{struct _fat_ptr*f1;struct _fat_ptr*f2;};
# 44
struct Cyc_List_List*Cyc_Specsfile_read_specs(struct _fat_ptr file){
# 58
struct Cyc_List_List*result=0;
int c;
int i;
char strname[256U];
char strvalue[4096U];
struct Cyc___cycFILE*spec_file=({Cyc_fopen((const char*)_check_null(_untag_fat_ptr(file,sizeof(char),1U)),"r");});
# 65
if(spec_file == 0){
({struct Cyc_String_pa_PrintArg_struct _tmp6=({struct Cyc_String_pa_PrintArg_struct _tmp56;_tmp56.tag=0U,_tmp56.f1=(struct _fat_ptr)((struct _fat_ptr)file);_tmp56;});void*_tmp4[1U];_tmp4[0]=& _tmp6;({struct Cyc___cycFILE*_tmp65=Cyc_stderr;struct _fat_ptr _tmp64=({const char*_tmp5="Error opening spec file %s\n";_tag_fat(_tmp5,sizeof(char),28U);});Cyc_fprintf(_tmp65,_tmp64,_tag_fat(_tmp4,sizeof(void*),1U));});});
({Cyc_fflush(Cyc_stderr);});
return 0;}
# 65
while(1){
# 72
while(1){
c=({Cyc_fgetc(spec_file);});
if(({isspace(c);}))continue;if(c == (int)'*')
break;
# 74
if(c != -1){
# 77
({struct Cyc_String_pa_PrintArg_struct _tmp9=({struct Cyc_String_pa_PrintArg_struct _tmp58;_tmp58.tag=0U,_tmp58.f1=(struct _fat_ptr)((struct _fat_ptr)file);_tmp58;});struct Cyc_Int_pa_PrintArg_struct _tmpA=({struct Cyc_Int_pa_PrintArg_struct _tmp57;_tmp57.tag=1U,_tmp57.f1=(unsigned long)c;_tmp57;});void*_tmp7[2U];_tmp7[0]=& _tmp9,_tmp7[1]=& _tmpA;({struct Cyc___cycFILE*_tmp67=Cyc_stderr;struct _fat_ptr _tmp66=({const char*_tmp8="Error reading spec file %s: unexpected character '%c'\n";_tag_fat(_tmp8,sizeof(char),55U);});Cyc_fprintf(_tmp67,_tmp66,_tag_fat(_tmp7,sizeof(void*),2U));});});
# 80
({Cyc_fflush(Cyc_stderr);});}
# 74
goto CLEANUP_AND_RETURN;}
# 84
JUST_AFTER_STAR:
 i=0;
while(1){
c=({Cyc_fgetc(spec_file);});
if(c == -1){
({struct Cyc_String_pa_PrintArg_struct _tmpD=({struct Cyc_String_pa_PrintArg_struct _tmp59;_tmp59.tag=0U,_tmp59.f1=(struct _fat_ptr)((struct _fat_ptr)file);_tmp59;});void*_tmpB[1U];_tmpB[0]=& _tmpD;({struct Cyc___cycFILE*_tmp69=Cyc_stderr;struct _fat_ptr _tmp68=({const char*_tmpC="Error reading spec file %s: unexpected EOF\n";_tag_fat(_tmpC,sizeof(char),44U);});Cyc_fprintf(_tmp69,_tmp68,_tag_fat(_tmpB,sizeof(void*),1U));});});
# 92
({Cyc_fflush(Cyc_stderr);});
goto CLEANUP_AND_RETURN;}
# 88
if(c == (int)':'){
# 96
*((char*)_check_known_subscript_notnull(strname,256U,sizeof(char),i))='\000';
break;}
# 88
*((char*)_check_known_subscript_notnull(strname,256U,sizeof(char),i))=(char)c;
# 100
++ i;
if(i >= 256){
({struct Cyc_String_pa_PrintArg_struct _tmp10=({struct Cyc_String_pa_PrintArg_struct _tmp5A;_tmp5A.tag=0U,_tmp5A.f1=(struct _fat_ptr)((struct _fat_ptr)file);_tmp5A;});void*_tmpE[1U];_tmpE[0]=& _tmp10;({struct Cyc___cycFILE*_tmp6B=Cyc_stderr;struct _fat_ptr _tmp6A=({const char*_tmpF="Error reading spec file %s: string name too long\n";_tag_fat(_tmpF,sizeof(char),50U);});Cyc_fprintf(_tmp6B,_tmp6A,_tag_fat(_tmpE,sizeof(void*),1U));});});
# 105
({Cyc_fflush(Cyc_stderr);});
goto CLEANUP_AND_RETURN;}}
# 109
while(1){
c=({Cyc_fgetc(spec_file);});
if(({isspace(c);}))continue;break;}
# 114
if(c == (int)'*'){
# 116
result=({struct Cyc_List_List*_tmp15=_cycalloc(sizeof(*_tmp15));({struct _tuple0*_tmp70=({struct _tuple0*_tmp14=_cycalloc(sizeof(*_tmp14));({struct _fat_ptr*_tmp6F=({struct _fat_ptr*_tmp11=_cycalloc(sizeof(*_tmp11));({struct _fat_ptr _tmp6E=(struct _fat_ptr)({Cyc_strdup(_tag_fat(strname,sizeof(char),256U));});*_tmp11=_tmp6E;});_tmp11;});_tmp14->f1=_tmp6F;}),({
struct _fat_ptr*_tmp6D=({struct _fat_ptr*_tmp13=_cycalloc(sizeof(*_tmp13));({struct _fat_ptr _tmp6C=(struct _fat_ptr)({Cyc_strdup(({const char*_tmp12="";_tag_fat(_tmp12,sizeof(char),1U);}));});*_tmp13=_tmp6C;});_tmp13;});_tmp14->f2=_tmp6D;});_tmp14;});
# 116
_tmp15->hd=_tmp70;}),_tmp15->tl=result;_tmp15;});
# 118
goto JUST_AFTER_STAR;}
# 114
strvalue[0]=(char)c;
# 122
i=1;
while(1){
c=({Cyc_fgetc(spec_file);});
if((c == -1 || c == (int)'\n')|| c == (int)'\r'){*((char*)_check_known_subscript_notnull(strvalue,4096U,sizeof(char),i))='\000';break;}*((char*)_check_known_subscript_notnull(strvalue,4096U,sizeof(char),i))=(char)c;
# 127
++ i;
if(i >= 4096){
({struct Cyc_String_pa_PrintArg_struct _tmp18=({struct Cyc_String_pa_PrintArg_struct _tmp5C;_tmp5C.tag=0U,_tmp5C.f1=(struct _fat_ptr)((struct _fat_ptr)file);_tmp5C;});struct Cyc_String_pa_PrintArg_struct _tmp19=({struct Cyc_String_pa_PrintArg_struct _tmp5B;_tmp5B.tag=0U,({
# 131
struct _fat_ptr _tmp71=(struct _fat_ptr)_tag_fat(strname,sizeof(char),256U);_tmp5B.f1=_tmp71;});_tmp5B;});void*_tmp16[2U];_tmp16[0]=& _tmp18,_tmp16[1]=& _tmp19;({struct Cyc___cycFILE*_tmp73=Cyc_stderr;struct _fat_ptr _tmp72=({const char*_tmp17="Error reading spec file %s: value of %s too long\n";_tag_fat(_tmp17,sizeof(char),50U);});Cyc_fprintf(_tmp73,_tmp72,_tag_fat(_tmp16,sizeof(void*),2U));});});
({Cyc_fflush(Cyc_stderr);});
goto CLEANUP_AND_RETURN;}}
# 137
result=({struct Cyc_List_List*_tmp1D=_cycalloc(sizeof(*_tmp1D));({struct _tuple0*_tmp78=({struct _tuple0*_tmp1C=_cycalloc(sizeof(*_tmp1C));({struct _fat_ptr*_tmp77=({struct _fat_ptr*_tmp1A=_cycalloc(sizeof(*_tmp1A));({struct _fat_ptr _tmp76=(struct _fat_ptr)({Cyc_strdup(_tag_fat(strname,sizeof(char),256U));});*_tmp1A=_tmp76;});_tmp1A;});_tmp1C->f1=_tmp77;}),({
struct _fat_ptr*_tmp75=({struct _fat_ptr*_tmp1B=_cycalloc(sizeof(*_tmp1B));({struct _fat_ptr _tmp74=(struct _fat_ptr)({Cyc_strdup(_tag_fat(strvalue,sizeof(char),4096U));});*_tmp1B=_tmp74;});_tmp1B;});_tmp1C->f2=_tmp75;});_tmp1C;});
# 137
_tmp1D->hd=_tmp78;}),_tmp1D->tl=result;_tmp1D;});
# 139
if(c == -1)goto CLEANUP_AND_RETURN;}
# 142
CLEANUP_AND_RETURN:
({Cyc_fclose(spec_file);});
return result;}
# 148
struct _fat_ptr Cyc_Specsfile_split_specs(struct _fat_ptr cmdline){
if(({char*_tmp79=(char*)cmdline.curr;_tmp79 == (char*)(_tag_fat(0,0,0)).curr;}))return _tag_fat(0,0,0);{unsigned long n=({Cyc_strlen((struct _fat_ptr)cmdline);});
# 151
struct Cyc_List_List*l=0;
char buf[4096U];
int i=0;
int j=0;
if(n > (unsigned long)4096)goto DONE;while(1){
# 158
while(1){
if((unsigned long)i >= n)goto DONE;if((int)*((const char*)_check_fat_subscript(cmdline,sizeof(char),i))== 0)
goto DONE;
# 159
if(!({isspace((int)((const char*)cmdline.curr)[i]);}))
# 161
break;
# 159
++ i;}
# 164
j=0;
# 169
while(1){
if((unsigned long)i >= n)break;if((int)*((const char*)_check_fat_subscript(cmdline,sizeof(char),i))== 0)
break;
# 170
if(({isspace((int)((const char*)cmdline.curr)[i]);}))
# 172
break;
# 170
if((int)((const char*)cmdline.curr)[i]== (int)'\\'){
# 175
++ i;
if((unsigned long)i >= n)break;if((int)*((const char*)_check_fat_subscript(cmdline,sizeof(char),i))== 0)
break;
# 176
*((char*)_check_known_subscript_notnull(buf,4096U,sizeof(char),j))=((const char*)cmdline.curr)[i];
# 179
++ j;}else{
# 182
*((char*)_check_known_subscript_notnull(buf,4096U,sizeof(char),j))=((const char*)cmdline.curr)[i];
++ j;}
# 185
++ i;}
# 187
if(j < 4096)
*((char*)_check_known_subscript_notnull(buf,4096U,sizeof(char),j))='\000';
# 187
l=({struct Cyc_List_List*_tmp20=_cycalloc(sizeof(*_tmp20));
# 190
({struct _fat_ptr*_tmp7B=({struct _fat_ptr*_tmp1F=_cycalloc(sizeof(*_tmp1F));({struct _fat_ptr _tmp7A=(struct _fat_ptr)({Cyc_strdup(_tag_fat(buf,sizeof(char),4096U));});*_tmp1F=_tmp7A;});_tmp1F;});_tmp20->hd=_tmp7B;}),_tmp20->tl=l;_tmp20;});}
# 192
DONE:
 l=({Cyc_List_imp_rev(l);});
l=({struct Cyc_List_List*_tmp23=_cycalloc(sizeof(*_tmp23));({struct _fat_ptr*_tmp7D=({struct _fat_ptr*_tmp22=_cycalloc(sizeof(*_tmp22));({struct _fat_ptr _tmp7C=({const char*_tmp21="";_tag_fat(_tmp21,sizeof(char),1U);});*_tmp22=_tmp7C;});_tmp22;});_tmp23->hd=_tmp7D;}),_tmp23->tl=l;_tmp23;});{
struct _fat_ptr ptrarray=({Cyc_List_to_array(l);});
struct _fat_ptr result=({unsigned _tmp25=_get_fat_size(ptrarray,sizeof(struct _fat_ptr*));struct _fat_ptr*_tmp24=_cycalloc(_check_times(_tmp25,sizeof(struct _fat_ptr)));({{unsigned _tmp5D=_get_fat_size(ptrarray,sizeof(struct _fat_ptr*));unsigned k;for(k=0;k < _tmp5D;++ k){_tmp24[k]=*((struct _fat_ptr**)ptrarray.curr)[(int)k];}}0;});_tag_fat(_tmp24,sizeof(struct _fat_ptr),_tmp25);});
return result;}}}
# 200
struct _fat_ptr Cyc_Specsfile_get_spec(struct Cyc_List_List*specs,struct _fat_ptr spec_name){
struct _handler_cons _tmp27;_push_handler(& _tmp27);{int _tmp29=0;if(setjmp(_tmp27.handler))_tmp29=1;if(!_tmp29){
{struct _fat_ptr _tmp2B=*({({struct _fat_ptr*(*_tmp7E)(int(*cmp)(struct _fat_ptr*,struct _fat_ptr*),struct Cyc_List_List*l,struct _fat_ptr*x)=({struct _fat_ptr*(*_tmp2A)(int(*cmp)(struct _fat_ptr*,struct _fat_ptr*),struct Cyc_List_List*l,struct _fat_ptr*x)=(struct _fat_ptr*(*)(int(*cmp)(struct _fat_ptr*,struct _fat_ptr*),struct Cyc_List_List*l,struct _fat_ptr*x))Cyc_List_assoc_cmp;_tmp2A;});_tmp7E(Cyc_strptrcmp,specs,& spec_name);});});_npop_handler(0U);return _tmp2B;};_pop_handler();}else{void*_tmp28=(void*)Cyc_Core_get_exn_thrown();void*_tmp2C=_tmp28;void*_tmp2D;if(((struct Cyc_Core_Not_found_exn_struct*)_tmp2C)->tag == Cyc_Core_Not_found){_LL1: _LL2:
# 205
 return _tag_fat(0,0,0);}else{_LL3: _tmp2D=_tmp2C;_LL4: {void*exn=_tmp2D;(int)_rethrow(exn);}}_LL0:;}}}
# 200
struct Cyc_List_List*Cyc_Specsfile_cyclone_arch_path=0;
# 210
struct _fat_ptr Cyc_Specsfile_def_lib_path={(void*)0,(void*)0,(void*)(0 + 0)};
# 215
static struct Cyc_List_List*Cyc_Specsfile_also_subdir(struct Cyc_List_List*dirs,struct _fat_ptr subdir){
struct Cyc_List_List*l=0;
for(0;dirs != 0;dirs=dirs->tl){
l=({struct Cyc_List_List*_tmp2F=_cycalloc(sizeof(*_tmp2F));_tmp2F->hd=(struct _fat_ptr*)dirs->hd,_tmp2F->tl=l;_tmp2F;});
l=({struct Cyc_List_List*_tmp31=_cycalloc(sizeof(*_tmp31));({struct _fat_ptr*_tmp80=({struct _fat_ptr*_tmp30=_cycalloc(sizeof(*_tmp30));({struct _fat_ptr _tmp7F=(struct _fat_ptr)({Cyc_Filename_concat(*((struct _fat_ptr*)dirs->hd),subdir);});*_tmp30=_tmp7F;});_tmp30;});_tmp31->hd=_tmp80;}),_tmp31->tl=l;_tmp31;});}
# 221
l=({Cyc_List_imp_rev(l);});
return l;}
# 237 "specsfile.cyc"
struct _fat_ptr Cyc_Specsfile_parse_b(struct Cyc_List_List*specs,void(*anonfun)(struct _fat_ptr),int(*anonflagfun)(struct _fat_ptr),struct _fat_ptr errmsg,struct _fat_ptr argv){
# 244
int argc=(int)_get_fat_size(argv,sizeof(struct _fat_ptr));
struct _fat_ptr bindices=({unsigned _tmp42=(unsigned)argc;int*_tmp41=_cycalloc_atomic(_check_times(_tmp42,sizeof(int)));({{unsigned _tmp60=(unsigned)argc;unsigned i;for(i=0;i < _tmp60;++ i){_tmp41[i]=0;}}0;});_tag_fat(_tmp41,sizeof(int),_tmp42);});
int numbindices=0;
int i;int j;int k;
for(i=1;i < argc;++ i){
if(({({struct _fat_ptr _tmp81=({const char*_tmp33="-B";_tag_fat(_tmp33,sizeof(char),3U);});Cyc_strncmp(_tmp81,(struct _fat_ptr)*((struct _fat_ptr*)_check_fat_subscript(argv,sizeof(struct _fat_ptr),i)),2U);});})== 0){
*((int*)_check_fat_subscript(bindices,sizeof(int),i))=1;
++ numbindices;}else{
# 253
if(({({struct _fat_ptr _tmp82=({const char*_tmp34="-b";_tag_fat(_tmp34,sizeof(char),3U);});Cyc_strcmp(_tmp82,(struct _fat_ptr)((struct _fat_ptr*)argv.curr)[i]);});})== 0){
*((int*)_check_fat_subscript(bindices,sizeof(int),i))=1;
++ numbindices;
++ i;if(i >= argc)break;*((int*)_check_fat_subscript(bindices,sizeof(int),i))=1;
# 258
++ numbindices;}}}{
# 265
struct _fat_ptr bargs=({unsigned _tmp40=(unsigned)(numbindices + 1);struct _fat_ptr*_tmp3F=_cycalloc(_check_times(_tmp40,sizeof(struct _fat_ptr)));({{unsigned _tmp5F=(unsigned)(numbindices + 1);unsigned n;for(n=0;n < _tmp5F;++ n){({struct _fat_ptr _tmp83=(struct _fat_ptr)_tag_fat(0,0,0);_tmp3F[n]=_tmp83;});}}0;});_tag_fat(_tmp3F,sizeof(struct _fat_ptr),_tmp40);});
struct _fat_ptr otherargs=({unsigned _tmp3E=(unsigned)(argc - numbindices);struct _fat_ptr*_tmp3D=_cycalloc(_check_times(_tmp3E,sizeof(struct _fat_ptr)));({{unsigned _tmp5E=(unsigned)(argc - numbindices);unsigned n;for(n=0;n < _tmp5E;++ n){({struct _fat_ptr _tmp84=(struct _fat_ptr)_tag_fat(0,0,0);_tmp3D[n]=_tmp84;});}}0;});_tag_fat(_tmp3D,sizeof(struct _fat_ptr),_tmp3E);});
({struct _fat_ptr _tmp86=({struct _fat_ptr _tmp85=*((struct _fat_ptr*)_check_fat_subscript(argv,sizeof(struct _fat_ptr),0));*((struct _fat_ptr*)_check_fat_subscript(otherargs,sizeof(struct _fat_ptr),0))=_tmp85;});*((struct _fat_ptr*)_check_fat_subscript(bargs,sizeof(struct _fat_ptr),0))=_tmp86;});
for(i=(j=(k=1));i < argc;++ i){
if(*((int*)_check_fat_subscript(bindices,sizeof(int),i)))({struct _fat_ptr _tmp87=*((struct _fat_ptr*)_check_fat_subscript(argv,sizeof(struct _fat_ptr),i));*((struct _fat_ptr*)_check_fat_subscript(bargs,sizeof(struct _fat_ptr),j ++))=_tmp87;});else{
({struct _fat_ptr _tmp88=*((struct _fat_ptr*)_check_fat_subscript(argv,sizeof(struct _fat_ptr),i));*((struct _fat_ptr*)_check_fat_subscript(otherargs,sizeof(struct _fat_ptr),k ++))=_tmp88;});}}
# 273
Cyc_Arg_current=0;
({Cyc_Arg_parse(specs,anonfun,anonflagfun,errmsg,bargs);});
# 278
if(({char*_tmp89=(char*)Cyc_Specsfile_target_arch.curr;_tmp89 == (char*)(_tag_fat(0,0,0)).curr;}))Cyc_Specsfile_target_arch=({char*_tmp35=Carch;_tag_fat(_tmp35,sizeof(char),_get_zero_arr_size_char((void*)_tmp35,1U));});{struct _fat_ptr cyclone_exec_prefix=({char*_tmp3C=({getenv("CYCLONE_EXEC_PREFIX");});_tag_fat(_tmp3C,sizeof(char),_get_zero_arr_size_char((void*)_tmp3C,1U));});
# 288 "specsfile.cyc"
if(({char*_tmp8A=(char*)cyclone_exec_prefix.curr;_tmp8A != (char*)(_tag_fat(0,0,0)).curr;}))
Cyc_Specsfile_cyclone_exec_path=({struct Cyc_List_List*_tmp37=_cycalloc(sizeof(*_tmp37));({struct _fat_ptr*_tmp8B=({struct _fat_ptr*_tmp36=_cycalloc(sizeof(*_tmp36));*_tmp36=cyclone_exec_prefix;_tmp36;});_tmp37->hd=_tmp8B;}),_tmp37->tl=Cyc_Specsfile_cyclone_exec_path;_tmp37;});
# 288
Cyc_Specsfile_def_lib_path=({char*_tmp38=Cdef_lib_path;_tag_fat(_tmp38,sizeof(char),_get_zero_arr_size_char((void*)_tmp38,1U));});
# 291
if(({Cyc_strlen((struct _fat_ptr)Cyc_Specsfile_def_lib_path);})> (unsigned long)0)
Cyc_Specsfile_cyclone_exec_path=({struct Cyc_List_List*_tmp3B=_cycalloc(sizeof(*_tmp3B));
({struct _fat_ptr*_tmp8E=({struct _fat_ptr*_tmp3A=_cycalloc(sizeof(*_tmp3A));({struct _fat_ptr _tmp8D=(struct _fat_ptr)({({struct _fat_ptr _tmp8C=Cyc_Specsfile_def_lib_path;Cyc_Filename_concat(_tmp8C,({const char*_tmp39="cyc-lib";_tag_fat(_tmp39,sizeof(char),8U);}));});});*_tmp3A=_tmp8D;});_tmp3A;});_tmp3B->hd=_tmp8E;}),_tmp3B->tl=Cyc_Specsfile_cyclone_exec_path;_tmp3B;});
# 291
Cyc_Specsfile_cyclone_exec_path=({Cyc_List_imp_rev(Cyc_Specsfile_cyclone_exec_path);});
# 296
Cyc_Specsfile_cyclone_arch_path=({Cyc_Specsfile_also_subdir(Cyc_Specsfile_cyclone_exec_path,Cyc_Specsfile_target_arch);});
return otherargs;}}}
# 301
static int Cyc_Specsfile_file_exists(struct _fat_ptr file){
struct Cyc___cycFILE*f=0;
{struct _handler_cons _tmp44;_push_handler(& _tmp44);{int _tmp46=0;if(setjmp(_tmp44.handler))_tmp46=1;if(!_tmp46){f=({({struct _fat_ptr _tmp8F=file;Cyc_file_open(_tmp8F,({const char*_tmp47="r";_tag_fat(_tmp47,sizeof(char),2U);}));});});;_pop_handler();}else{void*_tmp45=(void*)Cyc_Core_get_exn_thrown();void*_tmp48=_tmp45;_LL1: _LL2: goto _LL0;_LL0:;}}}
if(f == 0)return 0;else{
({Cyc_fclose(f);});return 1;}}
# 310
static struct _fat_ptr*Cyc_Specsfile_find(struct Cyc_List_List*dirs,struct _fat_ptr file){
if(({char*_tmp90=(char*)file.curr;_tmp90 == (char*)(_tag_fat(0,0,0)).curr;}))return 0;for(0;dirs != 0;dirs=dirs->tl){
# 313
struct _fat_ptr dir=*((struct _fat_ptr*)dirs->hd);
if(({char*_tmp91=(char*)dir.curr;_tmp91 == (char*)(_tag_fat(0,0,0)).curr;})||({Cyc_strlen((struct _fat_ptr)dir);})== (unsigned long)0)continue;{struct _fat_ptr s=(struct _fat_ptr)({Cyc_Filename_concat(dir,file);});
# 316
if(({Cyc_Specsfile_file_exists(s);}))return({struct _fat_ptr*_tmp4A=_cycalloc(sizeof(*_tmp4A));*_tmp4A=s;_tmp4A;});}}
# 318
return 0;}
# 322
static struct _fat_ptr Cyc_Specsfile_sprint_list(struct Cyc_List_List*dirs){
struct _fat_ptr tmp=({const char*_tmp4D="";_tag_fat(_tmp4D,sizeof(char),1U);});
for(0;dirs != 0;dirs=dirs->tl){
struct _fat_ptr dir=*((struct _fat_ptr*)dirs->hd);
if(({char*_tmp92=(char*)dir.curr;_tmp92 == (char*)(_tag_fat(0,0,0)).curr;})||({Cyc_strlen((struct _fat_ptr)dir);})== (unsigned long)0)continue;dir=(struct _fat_ptr)({({struct _fat_ptr _tmp93=(struct _fat_ptr)dir;Cyc_strconcat(_tmp93,({const char*_tmp4C=":";_tag_fat(_tmp4C,sizeof(char),2U);}));});});
# 328
tmp=(struct _fat_ptr)({Cyc_strconcat((struct _fat_ptr)dir,(struct _fat_ptr)tmp);});}
# 330
return tmp;}
# 333
static struct _fat_ptr Cyc_Specsfile_do_find(struct Cyc_List_List*dirs,struct _fat_ptr file){
struct _fat_ptr*f=({Cyc_Specsfile_find(dirs,file);});
if(f == 0){
({struct Cyc_String_pa_PrintArg_struct _tmp51=({struct Cyc_String_pa_PrintArg_struct _tmp62;_tmp62.tag=0U,_tmp62.f1=(struct _fat_ptr)((struct _fat_ptr)file);_tmp62;});struct Cyc_String_pa_PrintArg_struct _tmp52=({struct Cyc_String_pa_PrintArg_struct _tmp61;_tmp61.tag=0U,({
struct _fat_ptr _tmp94=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Specsfile_sprint_list(dirs);}));_tmp61.f1=_tmp94;});_tmp61;});void*_tmp4F[2U];_tmp4F[0]=& _tmp51,_tmp4F[1]=& _tmp52;({struct Cyc___cycFILE*_tmp96=Cyc_stderr;struct _fat_ptr _tmp95=({const char*_tmp50="Error: can't find internal compiler file %s in path %s\n";_tag_fat(_tmp50,sizeof(char),56U);});Cyc_fprintf(_tmp96,_tmp95,_tag_fat(_tmp4F,sizeof(void*),2U));});});
({Cyc_fflush(Cyc_stderr);});
return _tag_fat(0,0,0);}
# 335
return*f;}
# 333
struct _fat_ptr Cyc_Specsfile_find_in_arch_path(struct _fat_ptr s){
# 345
return({Cyc_Specsfile_do_find(Cyc_Specsfile_cyclone_arch_path,s);});}
# 333
struct _fat_ptr Cyc_Specsfile_find_in_exec_path(struct _fat_ptr s){
# 349
return({Cyc_Specsfile_do_find(Cyc_Specsfile_cyclone_exec_path,s);});}

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

# 4 "ctype.h"
 int isalnum(int);
# 22
int isspace(int);struct Cyc_Core_Opt{void*v;};extern char Cyc_Core_Invalid_argument[17U];struct Cyc_Core_Invalid_argument_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Failure[8U];struct Cyc_Core_Failure_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Impossible[11U];struct Cyc_Core_Impossible_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Not_found[10U];struct Cyc_Core_Not_found_exn_struct{char*tag;};extern char Cyc_Core_Unreachable[12U];struct Cyc_Core_Unreachable_exn_struct{char*tag;struct _fat_ptr f1;};
# 168 "core.h"
extern struct _RegionHandle*Cyc_Core_unique_region;struct Cyc_Core_DynamicRegion;struct Cyc_Core_NewDynamicRegion{struct Cyc_Core_DynamicRegion*key;};struct Cyc_Core_ThinRes{void*arr;unsigned nelts;};struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};extern char Cyc_List_List_mismatch[14U];struct Cyc_List_List_mismatch_exn_struct{char*tag;};extern char Cyc_List_Nth[4U];struct Cyc_List_Nth_exn_struct{char*tag;};struct Cyc___cycFILE;struct Cyc_String_pa_PrintArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Int_pa_PrintArg_struct{int tag;unsigned long f1;};struct Cyc_Double_pa_PrintArg_struct{int tag;double f1;};struct Cyc_LongDouble_pa_PrintArg_struct{int tag;long double f1;};struct Cyc_ShortPtr_pa_PrintArg_struct{int tag;short*f1;};struct Cyc_IntPtr_pa_PrintArg_struct{int tag;unsigned long*f1;};struct Cyc_ShortPtr_sa_ScanfArg_struct{int tag;short*f1;};struct Cyc_UShortPtr_sa_ScanfArg_struct{int tag;unsigned short*f1;};struct Cyc_IntPtr_sa_ScanfArg_struct{int tag;int*f1;};struct Cyc_UIntPtr_sa_ScanfArg_struct{int tag;unsigned*f1;};struct Cyc_StringPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_DoublePtr_sa_ScanfArg_struct{int tag;double*f1;};struct Cyc_FloatPtr_sa_ScanfArg_struct{int tag;float*f1;};struct Cyc_CharPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};extern char Cyc_FileCloseError[15U];struct Cyc_FileCloseError_exn_struct{char*tag;};extern char Cyc_FileOpenError[14U];struct Cyc_FileOpenError_exn_struct{char*tag;struct _fat_ptr f1;};
# 282 "cycboot.h"
int isalnum(int);
# 300
int isspace(int);
# 81 "string.h"
struct _fat_ptr Cyc__memcpy(struct _fat_ptr d,struct _fat_ptr s,unsigned long,unsigned);
# 86
struct _fat_ptr Cyc_memset(struct _fat_ptr s,char c,unsigned long n);
# 29 "assert.h"
void*Cyc___assert_fail(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line);
# 9 "xp.h"
int Cyc_XP_sub(int n,struct _fat_ptr z,struct _fat_ptr x,struct _fat_ptr y,int borrow);
# 12
int Cyc_XP_sum(int n,struct _fat_ptr z,struct _fat_ptr x,int y);
# 14
int Cyc_XP_product(int n,struct _fat_ptr z,struct _fat_ptr x,int y);
int Cyc_XP_quotient(int n,struct _fat_ptr z,struct _fat_ptr x,int y);
# 25
int Cyc_XP_length(int n,struct _fat_ptr x);static char _tmp0[37U]="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
# 7 "xp.cyc"
static struct _fat_ptr Cyc_digits={_tmp0,_tmp0,_tmp0 + 37U};
static char Cyc_map[75U]={'\000','\001','\002','\003','\004','\005','\006','\a','\b','\t','$','$','$','$','$','$','$','\n','\v','\f','\r','\016','\017','\020','\021','\022','\023','\024','\025','\026','\027','\030','\031','\032','\033','\034','\035','\036','\037',' ','!','"','#','$','$','$','$','$','$','\n','\v','\f','\r','\016','\017','\020','\021','\022','\023','\024','\025','\026','\027','\030','\031','\032','\033','\034','\035','\036','\037',' ','!','"','#'};
# 17
unsigned long Cyc_XP_fromint(int n,struct _fat_ptr z,unsigned long u){
int i=0;
do{
*((unsigned char*)_check_fat_subscript(z,sizeof(unsigned char),i ++))=(unsigned char)(u % (unsigned long)(1 << 8));}while(
(u /=(unsigned long)(1 << 8))> (unsigned long)0 && i < n);
for(0;i < n;++ i){
*((unsigned char*)_check_fat_subscript(z,sizeof(unsigned char),i))='\000';}
return u;}
# 26
unsigned long Cyc_XP_toint(int n,struct _fat_ptr x){
unsigned long u=0U;
int i=(int)sizeof(u);
if(i > n)
i=n;
# 29
while(-- i >= 0){
# 32
u=(unsigned long)(1 << 8)* u + (unsigned long)*((unsigned char*)_check_fat_subscript(x,sizeof(unsigned char),i));}
return u;}
# 35
int Cyc_XP_length(int n,struct _fat_ptr x){
while(n > 1 &&(int)*((unsigned char*)_check_fat_subscript(x,sizeof(unsigned char),n - 1))== 0){
-- n;}
return n;}
# 40
int Cyc_XP_add(int n,struct _fat_ptr z,struct _fat_ptr x,struct _fat_ptr y,int carry){
int i;
for(i=0;i < n;++ i){
({int _tmp48=({int _tmp47=(int)*((unsigned char*)_check_fat_subscript(x,sizeof(unsigned char),i));_tmp47 + (int)*((unsigned char*)_check_fat_subscript(y,sizeof(unsigned char),i));});carry +=_tmp48;});
*((unsigned char*)_check_fat_subscript(z,sizeof(unsigned char),i))=(unsigned char)(carry % (1 << 8));
carry /=1 << 8;}
# 47
return carry;}
# 49
int Cyc_XP_sub(int n,struct _fat_ptr z,struct _fat_ptr x,struct _fat_ptr y,int borrow){
int i;
for(i=0;i < n;++ i){
int d=({int _tmp49=((int)*((unsigned char*)_check_fat_subscript(x,sizeof(unsigned char),i))+ (1 << 8))- borrow;_tmp49 - (int)*((unsigned char*)_check_fat_subscript(y,sizeof(unsigned char),i));});
*((unsigned char*)_check_fat_subscript(z,sizeof(unsigned char),i))=(unsigned char)(d % (1 << 8));
borrow=1 - d / (1 << 8);}
# 56
return borrow;}
# 58
int Cyc_XP_sum(int n,struct _fat_ptr z,struct _fat_ptr x,int y){
int i;
for(i=0;i < n;++ i){
y +=(int)*((unsigned char*)_check_fat_subscript(x,sizeof(unsigned char),i));
*((unsigned char*)_check_fat_subscript(z,sizeof(unsigned char),i))=(unsigned char)(y % (1 << 8));
y /=1 << 8;}
# 65
return y;}
# 67
int Cyc_XP_diff(int n,struct _fat_ptr z,struct _fat_ptr x,int y){
int i;
for(i=0;i < n;++ i){
int d=((int)*((unsigned char*)_check_fat_subscript(x,sizeof(unsigned char),i))+ (1 << 8))- y;
*((unsigned char*)_check_fat_subscript(z,sizeof(unsigned char),i))=(unsigned char)(d % (1 << 8));
y=1 - d / (1 << 8);}
# 74
return y;}
# 76
int Cyc_XP_neg(int n,struct _fat_ptr z,struct _fat_ptr x,int carry){
int i;
for(i=0;i < n;++ i){
carry +=(int)~(*((unsigned char*)_check_fat_subscript(x,sizeof(unsigned char),i)));
*((unsigned char*)_check_fat_subscript(z,sizeof(unsigned char),i))=(unsigned char)(carry % (1 << 8));
carry /=1 << 8;}
# 83
return carry;}
# 85
int Cyc_XP_mul(struct _fat_ptr z,int n,struct _fat_ptr x,int m,struct _fat_ptr y){
int i;int j;int carryout=0;
for(i=0;i < n;++ i){
unsigned carry=0U;
for(j=0;j < m;++ j){
({unsigned _tmp4C=(unsigned)({int _tmp4B=({int _tmp4A=(int)*((unsigned char*)_check_fat_subscript(x,sizeof(unsigned char),i));_tmp4A * (int)*((unsigned char*)_check_fat_subscript(y,sizeof(unsigned char),j));});_tmp4B + (int)*((unsigned char*)_check_fat_subscript(z,sizeof(unsigned char),i + j));});carry +=_tmp4C;});
*((unsigned char*)_check_fat_subscript(z,sizeof(unsigned char),i + j))=(unsigned char)(carry % (unsigned)(1 << 8));
carry /=(unsigned)(1 << 8);}
# 94
for(0;j < (n + m)- i;++ j){
carry +=(unsigned)*((unsigned char*)_check_fat_subscript(z,sizeof(unsigned char),i + j));
*((unsigned char*)_check_fat_subscript(z,sizeof(unsigned char),i + j))=(unsigned char)(carry % (unsigned)(1 << 8));
carry /=(unsigned)(1 << 8);}
# 99
carryout |=carry;}
# 101
return carryout;}
# 103
int Cyc_XP_product(int n,struct _fat_ptr z,struct _fat_ptr x,int y){
int i;
unsigned carry=0U;
for(i=0;i < n;++ i){
carry +=(unsigned)((int)*((unsigned char*)_check_fat_subscript(x,sizeof(unsigned char),i))* y);
*((unsigned char*)_check_fat_subscript(z,sizeof(unsigned char),i))=(unsigned char)(carry % (unsigned)(1 << 8));
carry /=(unsigned)(1 << 8);}
# 111
return(int)carry;}
# 113
int Cyc_XP_div(int n,struct _fat_ptr q,struct _fat_ptr x,int m,struct _fat_ptr y,struct _fat_ptr r,struct _fat_ptr tmp){
int nx=n;int my=m;
n=({Cyc_XP_length(n,x);});
m=({Cyc_XP_length(m,y);});
if(m == 1){
if((int)*((unsigned char*)_check_fat_subscript(y,sizeof(unsigned char),0))== 0)
return 0;
# 118
({unsigned char _tmp4D=(unsigned char)({Cyc_XP_quotient(nx,q,x,(int)((unsigned char*)y.curr)[0]);});*((unsigned char*)_check_fat_subscript(r,sizeof(unsigned char),0))=_tmp4D;});
# 121
if(_get_fat_size(r,sizeof(unsigned char))> (unsigned)1)
({({struct _fat_ptr _tmp4E=_fat_ptr_plus((struct _fat_ptr)r,sizeof(char),1);Cyc_memset(_tmp4E,'\000',(unsigned long)(my - 1));});});}else{
if(m > n){
({Cyc_memset((struct _fat_ptr)q,'\000',(unsigned long)nx);});
({Cyc__memcpy(r,(struct _fat_ptr)x,(unsigned)n / sizeof(*((unsigned char*)x.curr))+ (unsigned)((unsigned)n % sizeof(*((unsigned char*)x.curr))== (unsigned)0?0: 1),sizeof(*((unsigned char*)x.curr)));});
({({struct _fat_ptr _tmp4F=_fat_ptr_plus((struct _fat_ptr)r,sizeof(char),n);Cyc_memset(_tmp4F,'\000',(unsigned long)(my - n));});});}else{
# 128
int k;
struct _fat_ptr rem=tmp;struct _fat_ptr dq=_fat_ptr_plus(_fat_ptr_plus(tmp,sizeof(unsigned char),n),sizeof(unsigned char),1);
2 <= m && m <= n?0:({({int(*_tmp51)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmpB)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmpB;});struct _fat_ptr _tmp50=({const char*_tmpC="2 <= m && m <= n";_tag_fat(_tmpC,sizeof(char),17U);});_tmp51(_tmp50,({const char*_tmpD="xp.cyc";_tag_fat(_tmpD,sizeof(char),7U);}),130U);});});
({Cyc__memcpy(rem,(struct _fat_ptr)x,(unsigned)n / sizeof(*((unsigned char*)x.curr))+ (unsigned)((unsigned)n % sizeof(*((unsigned char*)x.curr))== (unsigned)0?0: 1),sizeof(*((unsigned char*)x.curr)));});
*((unsigned char*)_check_fat_subscript(rem,sizeof(unsigned char),n))='\000';
for(k=n - m;k >= 0;-- k){
int qk;
{
int i;
(2 <= m && m <= k + m)&& k + m <= n?0:({({int(*_tmp53)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmpE)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmpE;});struct _fat_ptr _tmp52=({const char*_tmpF="2 <= m && m <= k+m && k+m <= n";_tag_fat(_tmpF,sizeof(char),31U);});_tmp53(_tmp52,({const char*_tmp10="xp.cyc";_tag_fat(_tmp10,sizeof(char),7U);}),137U);});});
{
int km=k + m;
unsigned long y2=(unsigned long)({int _tmp54=(int)*((unsigned char*)_check_fat_subscript(y,sizeof(unsigned char),m - 1))* (1 << 8);_tmp54 + (int)*((unsigned char*)_check_fat_subscript(y,sizeof(unsigned char),m - 2));});
unsigned long r3=(unsigned long)({int _tmp56=({int _tmp55=(int)*((unsigned char*)_check_fat_subscript(rem,sizeof(unsigned char),km))* ((1 << 8)* (1 << 8));_tmp55 + (int)*((unsigned char*)_check_fat_subscript(rem,sizeof(unsigned char),km - 1))* (1 << 8);});_tmp56 + (int)*((unsigned char*)_check_fat_subscript(rem,sizeof(unsigned char),km - 2));});
# 143
qk=(int)(r3 / y2);
if(qk >= 1 << 8)
qk=255U;}
# 147
({unsigned char _tmp57=(unsigned char)({Cyc_XP_product(m,dq,y,qk);});*((unsigned char*)_check_fat_subscript(dq,sizeof(unsigned char),m))=_tmp57;});
for(i=m;i > 0;-- i){
if(({int _tmp58=(int)*((unsigned char*)_check_fat_subscript(rem,sizeof(unsigned char),i + k));_tmp58 != (int)*((unsigned char*)_check_fat_subscript(dq,sizeof(unsigned char),i));}))
break;}
# 148
if(({
# 151
int _tmp59=(int)*((unsigned char*)_check_fat_subscript(rem,sizeof(unsigned char),i + k));_tmp59 < (int)*((unsigned char*)_check_fat_subscript(dq,sizeof(unsigned char),i));}))
({unsigned char _tmp5A=(unsigned char)({int(*_tmp11)(int n,struct _fat_ptr z,struct _fat_ptr x,int y)=Cyc_XP_product;int _tmp12=m;struct _fat_ptr _tmp13=dq;struct _fat_ptr _tmp14=y;int _tmp15=-- qk;_tmp11(_tmp12,_tmp13,_tmp14,_tmp15);});((unsigned char*)dq.curr)[m]=_tmp5A;});}
# 154
*((unsigned char*)_check_fat_subscript(q,sizeof(unsigned char),k))=(unsigned char)qk;{
# 156
int borrow;
0 <= k && k <= k + m?0:({({int(*_tmp5C)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmp16)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmp16;});struct _fat_ptr _tmp5B=({const char*_tmp17="0 <= k && k <= k+m";_tag_fat(_tmp17,sizeof(char),19U);});_tmp5C(_tmp5B,({const char*_tmp18="xp.cyc";_tag_fat(_tmp18,sizeof(char),7U);}),157U);});});
borrow=({({int _tmp5F=m + 1;struct _fat_ptr _tmp5E=_fat_ptr_plus(rem,sizeof(unsigned char),k);struct _fat_ptr _tmp5D=_fat_ptr_plus(rem,sizeof(unsigned char),k);Cyc_XP_sub(_tmp5F,_tmp5E,_tmp5D,dq,0);});});
borrow == 0?0:({({int(*_tmp61)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmp19)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmp19;});struct _fat_ptr _tmp60=({const char*_tmp1A="borrow == 0";_tag_fat(_tmp1A,sizeof(char),12U);});_tmp61(_tmp60,({const char*_tmp1B="xp.cyc";_tag_fat(_tmp1B,sizeof(char),7U);}),159U);});});}}
# 162
({Cyc__memcpy(r,(struct _fat_ptr)rem,(unsigned)m / sizeof(*((unsigned char*)rem.curr))+ (unsigned)((unsigned)m % sizeof(*((unsigned char*)rem.curr))== (unsigned)0?0: 1),sizeof(*((unsigned char*)rem.curr)));});{
# 164
int i;
for(i=(n - m)+ 1;i < nx;++ i){
*((unsigned char*)_check_fat_subscript(q,sizeof(unsigned char),i))='\000';}
for(i=m;i < my;++ i){
*((unsigned char*)_check_fat_subscript(r,sizeof(unsigned char),i))='\000';}}}}
# 171
return 1;}
# 173
int Cyc_XP_quotient(int n,struct _fat_ptr z,struct _fat_ptr x,int y){
int i;
unsigned carry=0U;
for(i=n - 1;i >= 0;-- i){
carry=carry * (unsigned)(1 << 8)+ (unsigned)*((unsigned char*)_check_fat_subscript(x,sizeof(unsigned char),i));
*((unsigned char*)_check_fat_subscript(z,sizeof(unsigned char),i))=(unsigned char)(carry / (unsigned)y);
carry %=(unsigned)y;}
# 181
return(int)carry;}
# 183
int Cyc_XP_cmp(int n,struct _fat_ptr x,struct _fat_ptr y){
int i=n - 1;
while(i > 0 &&({int _tmp62=(int)*((unsigned char*)_check_fat_subscript(x,sizeof(unsigned char),i));_tmp62 == (int)*((unsigned char*)_check_fat_subscript(y,sizeof(unsigned char),i));})){
-- i;}
return({int _tmp63=(int)*((unsigned char*)_check_fat_subscript(x,sizeof(unsigned char),i));_tmp63 - (int)*((unsigned char*)_check_fat_subscript(y,sizeof(unsigned char),i));});}
# 189
void Cyc_XP_lshift(int n,struct _fat_ptr z,int m,struct _fat_ptr x,int s,int fill){
fill=fill?255: 0;
{
int i;int j=n - 1;
if(n > m)
i=m - 1;else{
# 196
i=(n - s / 8)- 1;}
for(0;j >= m + s / 8;-- j){
*((unsigned char*)_check_fat_subscript(z,sizeof(unsigned char),j))='\000';}
for(0;i >= 0;(i --,j --)){
({unsigned char _tmp64=*((unsigned char*)_check_fat_subscript(x,sizeof(unsigned char),i));*((unsigned char*)_check_fat_subscript(z,sizeof(unsigned char),j))=_tmp64;});}
for(0;j >= 0;-- j){
*((unsigned char*)_check_fat_subscript(z,sizeof(unsigned char),j))=(unsigned char)fill;}}
# 204
s %=8;
if(s > 0){
# 207
({Cyc_XP_product(n,z,z,1 << s);});
*((unsigned char*)_check_fat_subscript(z,sizeof(unsigned char),0))|=fill >> 8 - s;}}
# 211
void Cyc_XP_rshift(int n,struct _fat_ptr z,int m,struct _fat_ptr x,int s,int fill){
fill=fill?255: 0;
{
int i;int j=0;
for(i=s / 8;i < m && j < n;(i ++,j ++)){
({unsigned char _tmp65=*((unsigned char*)_check_fat_subscript(x,sizeof(unsigned char),i));*((unsigned char*)_check_fat_subscript(z,sizeof(unsigned char),j))=_tmp65;});}
for(0;j < n;++ j){
*((unsigned char*)_check_fat_subscript(z,sizeof(unsigned char),j))=(unsigned char)fill;}}
# 220
s %=8;
if(s > 0){
# 223
({Cyc_XP_quotient(n,z,z,1 << s);});
*((unsigned char*)_check_fat_subscript(z,sizeof(unsigned char),n - 1))|=fill << 8 - s;}}
# 227
void Cyc_XP_and(int n,struct _fat_ptr z,struct _fat_ptr x,struct _fat_ptr y){
int i;
for(i=0;i < n;++ i){
({unsigned char _tmp67=(unsigned char)({int _tmp66=(int)*((unsigned char*)_check_fat_subscript(x,sizeof(unsigned char),i));_tmp66 & (int)*((unsigned char*)_check_fat_subscript(y,sizeof(unsigned char),i));});*((unsigned char*)_check_fat_subscript(z,sizeof(unsigned char),i))=_tmp67;});}}
# 232
void Cyc_XP_or(int n,struct _fat_ptr z,struct _fat_ptr x,struct _fat_ptr y){
int i;
for(i=0;i < n;++ i){
({unsigned char _tmp69=(unsigned char)({int _tmp68=(int)*((unsigned char*)_check_fat_subscript(x,sizeof(unsigned char),i));_tmp68 | (int)*((unsigned char*)_check_fat_subscript(y,sizeof(unsigned char),i));});*((unsigned char*)_check_fat_subscript(z,sizeof(unsigned char),i))=_tmp69;});}}
# 237
void Cyc_XP_xor(int n,struct _fat_ptr z,struct _fat_ptr x,struct _fat_ptr y){
int i;
for(i=0;i < n;++ i){
({unsigned char _tmp6B=(unsigned char)({int _tmp6A=(int)*((unsigned char*)_check_fat_subscript(x,sizeof(unsigned char),i));_tmp6A ^ (int)*((unsigned char*)_check_fat_subscript(y,sizeof(unsigned char),i));});*((unsigned char*)_check_fat_subscript(z,sizeof(unsigned char),i))=_tmp6B;});}}
# 242
void Cyc_XP_not(int n,struct _fat_ptr z,struct _fat_ptr x){
int i;
for(i=0;i < n;++ i){
({unsigned char _tmp6C=~(*((unsigned char*)_check_fat_subscript(x,sizeof(unsigned char),i)));*((unsigned char*)_check_fat_subscript(z,sizeof(unsigned char),i))=_tmp6C;});}}
# 247
int Cyc_XP_fromstr(int n,struct _fat_ptr z,const char*str,int base){
# 249
const char*p=str;
(unsigned)p?0:({({int(*_tmp6E)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmp25)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmp25;});struct _fat_ptr _tmp6D=({const char*_tmp26="p";_tag_fat(_tmp26,sizeof(char),2U);});_tmp6E(_tmp6D,({const char*_tmp27="xp.cyc";_tag_fat(_tmp27,sizeof(char),7U);}),250U);});});
base >= 2 && base <= 36?0:({({int(*_tmp70)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmp28)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmp28;});struct _fat_ptr _tmp6F=({const char*_tmp29="base >= 2 && base <= 36";_tag_fat(_tmp29,sizeof(char),24U);});_tmp70(_tmp6F,({const char*_tmp2A="xp.cyc";_tag_fat(_tmp2A,sizeof(char),7U);}),251U);});});
while((int)*((const char*)_check_null(p))&&({isspace((int)*p);})){
({const char**_tmp2B=& p;if(*(*_tmp2B)!= 0)++(*_tmp2B);else{_throw_arraybounds();}*_tmp2B;});}
if(((int)*p &&({isalnum((int)*p);}))&&(int)*((char*)_check_known_subscript_notnull(Cyc_map,75U,sizeof(char),(int)*p - (int)'0'))< base){
int carry;
for(0;((int)*((const char*)_check_null(p))&&({isalnum((int)*p);}))&&(int)*((char*)_check_known_subscript_notnull(Cyc_map,75U,sizeof(char),(int)*p - (int)'0'))< base;({const char**_tmp2C=& p;if(*(*_tmp2C)!= 0)++(*_tmp2C);else{_throw_arraybounds();}*_tmp2C;})){
carry=({Cyc_XP_product(n,z,z,base);});
if(carry)
break;
# 258
({Cyc_XP_sum(n,z,z,(int)*((char*)_check_known_subscript_notnull(Cyc_map,75U,sizeof(char),(int)*p - (int)'0')));});}
# 262
return carry;}else{
# 264
return 0;}}
# 267
struct _fat_ptr Cyc_XP_tostr(struct _fat_ptr str,int size,int base,int n,struct _fat_ptr x){
# 269
int i=0;
(unsigned)str.curr?0:({({int(*_tmp72)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmp2E)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmp2E;});struct _fat_ptr _tmp71=({const char*_tmp2F="str";_tag_fat(_tmp2F,sizeof(char),4U);});_tmp72(_tmp71,({const char*_tmp30="xp.cyc";_tag_fat(_tmp30,sizeof(char),7U);}),270U);});});
base >= 2 && base <= 36?0:({({int(*_tmp74)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmp31)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmp31;});struct _fat_ptr _tmp73=({const char*_tmp32="base >= 2 && base <= 36";_tag_fat(_tmp32,sizeof(char),24U);});_tmp74(_tmp73,({const char*_tmp33="xp.cyc";_tag_fat(_tmp33,sizeof(char),7U);}),271U);});});
do{
int r=({Cyc_XP_quotient(n,x,x,base);});
i < size?0:({({int(*_tmp76)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmp34)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmp34;});struct _fat_ptr _tmp75=({const char*_tmp35="i < size";_tag_fat(_tmp35,sizeof(char),9U);});_tmp76(_tmp75,({const char*_tmp36="xp.cyc";_tag_fat(_tmp36,sizeof(char),7U);}),274U);});});
({struct _fat_ptr _tmp37=_fat_ptr_plus(str,sizeof(char),i ++);char _tmp38=*((char*)_check_fat_subscript(_tmp37,sizeof(char),0U));char _tmp39=*((const char*)_check_fat_subscript(Cyc_digits,sizeof(char),r));if(_get_fat_size(_tmp37,sizeof(char))== 1U &&(_tmp38 == 0 && _tmp39 != 0))_throw_arraybounds();*((char*)_tmp37.curr)=_tmp39;});
while(n > 1 &&(int)*((unsigned char*)_check_fat_subscript(x,sizeof(unsigned char),n - 1))== 0){
-- n;}}while(
n > 1 ||(int)*((unsigned char*)_check_fat_subscript(x,sizeof(unsigned char),0))!= 0);
i < size?0:({({int(*_tmp78)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=({int(*_tmp3A)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line)=(int(*)(struct _fat_ptr assertion,struct _fat_ptr file,unsigned line))Cyc___assert_fail;_tmp3A;});struct _fat_ptr _tmp77=({const char*_tmp3B="i < size";_tag_fat(_tmp3B,sizeof(char),9U);});_tmp78(_tmp77,({const char*_tmp3C="xp.cyc";_tag_fat(_tmp3C,sizeof(char),7U);}),279U);});});
({struct _fat_ptr _tmp3D=_fat_ptr_plus(str,sizeof(char),i);char _tmp3E=*((char*)_check_fat_subscript(_tmp3D,sizeof(char),0U));char _tmp3F='\000';if(_get_fat_size(_tmp3D,sizeof(char))== 1U &&(_tmp3E == 0 && _tmp3F != 0))_throw_arraybounds();*((char*)_tmp3D.curr)=_tmp3F;});
{
int j;
for(j=0;j < -- i;++ j){
char c=*((char*)_check_fat_subscript(str,sizeof(char),j));
({struct _fat_ptr _tmp40=_fat_ptr_plus(str,sizeof(char),j);char _tmp41=*((char*)_check_fat_subscript(_tmp40,sizeof(char),0U));char _tmp42=*((char*)_check_fat_subscript(str,sizeof(char),i));if(_get_fat_size(_tmp40,sizeof(char))== 1U &&(_tmp41 == 0 && _tmp42 != 0))_throw_arraybounds();*((char*)_tmp40.curr)=_tmp42;});
({struct _fat_ptr _tmp43=_fat_ptr_plus(str,sizeof(char),i);char _tmp44=*((char*)_check_fat_subscript(_tmp43,sizeof(char),0U));char _tmp45=c;if(_get_fat_size(_tmp43,sizeof(char))== 1U &&(_tmp44 == 0 && _tmp45 != 0))_throw_arraybounds();*((char*)_tmp43.curr)=_tmp45;});}}
# 289
return str;}

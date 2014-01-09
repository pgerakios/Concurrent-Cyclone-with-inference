/* This file is part of the Cyclone Library.
   Copyright (C) 2001 Greg Morrisett, AT&T

   This library is free software; you can redistribute it and/or it
   under the terms of the GNU Lesser General Public License as
   published by the Free Software Foundation; either version 2.1 of
   the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place, Suite
   330, Boston, MA 02111-1307 USA. */

// This is implements the stack used by exceptions and lexical regions.

#include "runtime_internal.h"

//#define __RUNTIME_STACK_DEBUG
//#define  __RUNTIME_STACK_DEBUG_LOOKUP
//#define __RUNTIME_STACK_DEBUG_GET_EFFECT_FRAME
//  #define __RUNTIME_STACK_DEBUG_THREAD

#ifdef  __RUNTIME_STACK_DEBUG_THREAD
 #define dbgmsg_thread(arg...)  { fprintf(stderr,##arg); fflush(stderr);} 
#else
 #define dbgmsg_thread(arg...)  {} 
#endif

#ifdef  __RUNTIME_STACK_DEBUG_LOOKUP
 #define dbgmsg_lup(arg...)  { fprintf(stderr,##arg); fflush(stderr);} 
#else
 #define dbgmsg_lup(arg...)  {} 
#endif

#ifdef  __RUNTIME_STACK_DEBUG_GET_EFFECT_FRAME
 #define dbgmsg_gef(arg...)  { fprintf(stderr,##arg); fflush(stderr);} 
#else
 #define dbgmsg_gef(arg...)  {} 
#endif

#ifdef  __RUNTIME_STACK_DEBUG 
 #define dbgmsg(arg...)  { fprintf(stderr,##arg); fflush(stderr);} 
#else
 #define dbgmsg(arg...)  {} 
#endif

/************************************************************************/ 
void _push_frame(struct _RuntimeStack *frame) {
  struct _ThreadKey *t = tcb();
  frame->next = t->stack._current_frame;
  t->stack._current_frame = frame;
}

// set _current_frame to its n+1'th tail (i.e. pop n+1 frames)
// Invariant: result is non-null
void _npop_frame(unsigned int n) {
  struct _ThreadKey *t = tcb();
  struct _RuntimeStack *current_frame = t->stack._current_frame;
  unsigned int i;

  for(i = n; i <= n; i--) {
    if(current_frame == NULL) 
      errquit("internal error: empty frame stack\n");
    dbgmsg("\nPOPING frame : %p code = %p" ,
             current_frame, current_frame->cleanup);
    if (current_frame->cleanup != NULL)
       current_frame->cleanup(current_frame);
    current_frame = current_frame->next;
  }
  t->stack._current_frame = current_frame;
}
/***********************************************************************/
#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif
#define ALIGN_SZ(x)  (((unsigned long)(x) & ~0x7) + 0x8)
#define ALIGN_PG(p)   ((((long long) p + PAGE_SIZE-1) & (long long)~(PAGE_SIZE-1)))
#ifndef PROT_READ
#define PROT_READ 1
#endif

#ifndef SIGSTKSZ
  # define SIGSTKSZ 16384
  #define  MINSIGSTKSZ 2048
#endif


#define ts(x) ((thread_stack_t *)x)

#define BACKUP_STK_SZ  PAGE_SIZE
__thread char __backup__stack[BACKUP_STK_SZ]; // suitable for catching sigsegv
__thread int  __free_stack = 0;
__thread void *__si = 0;
__thread struct _ThreadKey _thread_key;
//invariant: next_tid SHOULD never be zero. Proof:
//(0) it starts off with 1
//(1) it always gets incremented by two
static volatile unsigned int next_tid = 1; 

typedef struct _ThreadStack {
   void *(*thread)(void *);
   unsigned int stack_size;
   unsigned int stack_real_size;
   unsigned int tid;
} *sinfo_t;

unsigned int _next_tid() {
  return __sync_fetch_and_add(&next_tid,2);
}    

void _init_thread_context(unsigned int id){
   memset(&_thread_key,0,sizeof(struct _ThreadKey));	 
   _thread_key. h =  create_hashtable(25);
   _thread_key.tid = id;
   // install stack overflow handler
   if(stackoverflow_install_handler
      (&_exn_stackoverflow_handler,   
       __backup__stack,BACKUP_STK_SZ) < 0)
    errquit("Could not install stack handler");
   // instacl seg fault handler
   sigsegv_install_handler(&_exn_segv_handler);
}
#ifdef HAVE_THREADS
static inline void _fini_thread_context(){
	struct hashtable *h = tcb()->h;
	if( h != NULL) hashtable_destroy(h);
}

static void *_thread_internal2(void *arg){
    if(!arg) errquit("_thread_internal2: null arg");
    sinfo_t i = (sinfo_t) arg;
    __si = arg;
    _init_thread_context(i->tid);
   // install top exception handler and execute thread if there's no exn
   if( _set_top_handler() == NULL ) 
	  (i->thread)((void *) (i+1));
  // finalize thread context	 
  _fini_thread_context();
  // wake up main thread if I'm the last thread
  if(__sync_fetch_and_add(&all_done,-1) == 1)
   futex_wake((int *)&all_done,1);
  //HACK: generate segfault so as to switch
  //to the backup stack and free the current stack.
  // Mark the last page W/R. so that it can be deallocated
  char *guard = ((char *) i) + i->stack_real_size - PAGE_SIZE;
  if (mprotect((void *)guard,1024,3))
   errquit("__thread_internal2: mprotect failed.");
  __free_stack = 1;
  ((int *)0)[0] = 10;
  pthread_exit(0); // should NOT reach this point
  return NULL;
}

typedef struct Caps {
  struct _XRegionHandle *h;
  int rc;
  int wc;
  int lc;
} *caps_t;


void qsort(void *base, size_t nel, size_t width,
       int (*compar)(const void *, const void *));

//assumes h points to a TreeHandle (this invariant is
//satisfied by calling merge_caps prior to qsort)
// TreeHandles are stack-allocated. So if h is a parent
// of h' , h' is less than h (h-h' will be positive as
// h is allocated on the stack before h'. Here we sort
// regions in the order they were allocated on the 
// stack: parents come first, then their children and 
// so on. If some regions are allocated in different 
// subtrees, then the one that got allocated "first"
// is "less than" the other. We sort the array in this
// way so as to CONSTRUCT an identical subtree of the
// original tree of this thread. ORDER matters because
// locking is performed by traversing the tree and this
// may cause DEADLOCKS if the locking ORDER is different
static int caps_cmp( caps_t c1, caps_t c2 ){
   return (int) -(c1->h - c2->h);
}
//returns a merged tree in terms of cap counts
// sorted by allocation order (most recently allocated
// regions come last).
static int merge_caps( caps_t c, int len) {
  if(len == 0) return 0;
  int i;
  for(i=1,c++;i<len;i++){ // find tree handles
   c[i].h = (struct _XRegionHandle *) 
                  get_tkey(c[i].h);
  qsort(c,sizeof(*c),len,(int (*)(const void *,
                                  const void *))caps_cmp);
  int i,n=1;
  caps_t it = c,next=c;
  for(i=1,c++;i<len;i++){
    if(it->h == c[i].h) {
      it->rc += c[i].rc;
      it->wc += c[i].wc;
      it->lc += c[i].lc;
    }
    else {
      *next = *it;
      next++;
      it = c+i;
      n++;
    }
  }
  *next = *it;
  return n;
  }
}

/*
 *  Thread Stack
 *
 * A. void (*thread)(void *);
 * C. unsigned int stack_size
 * D. unsigned int stack_real_size
 * E. tid
 * F. targ ---> arg_len bytes
 * G. XTreeHandle[merge_caps(caps,len)]
 * H. Actual Stack
 * ...
 * Z. Stack End
 */
unsigned int __get_stack_size(
  const unsigned int  caps_len,
   const unsigned int arg_len,
  const unsigned int  desired_stack_size){
  static unsigned int PADDING = 16;
  const unsigned int min_allocation = 
                                 SIGSTKSZ + PAGE_SIZE;
  const unsigned long default_allocation =  
            ALIGN_SZ(sizeof(struct _ThreadStack)+ 
                     arg_len +
                     sizeof(struct _XTreeHandle)*caps_len+
                     PADDING);
  unsigned int stack_estimation = default_allocation+
                                  PAGE_SIZE +
          ((min_allocation > desired_stack_size)?
           min_allocation:desired_stack_size);

 if( ALIGN_PG(stack_estimation) != stack_estimation )
   stack_estimation = ALIGN_PG(stack_estimation) + 
                      PAGE_SIZE;
 return stack_estimation;
}

/////////////////////////////////////////
/*
 *typedef struct Caps {
  struct _XRegionHandle *h;
  int rc;
  int wc;
  int lc;
} *caps_t;



 */
//static void __listcopy(struct _XTreeHandle *t,

/*
 *typedef struct Caps {
  struct _XRegionHandle *h;
  int rc;
  int wc;
  int lc;
} */

#define th(n,i) ((struct _XTreeHandle *) n[i].h)

/* treecopy assumes that n[0-n] is filled with zeros*/
 void __treecopy(caps_t o, 
                  struct _XTreeHandle *n, int len, 
                       unsigned int tid) {
  int i;
  //initialize fresh memory
  memset(n,0,len*sizeof(struct _XTreeHandle));
  // create new tree with the appropriate capabilities
  for(i=0;i<len;i++) {
    struct _XTreeHandle *t = th(o,i);
    n[i].caps.rcnt = o[i].rc;
    n[i].caps.wr_cnt = o[i].wc;
    n[i].caps.rd_cnt= o[i].lc;
    n[i].h = t->h;
    //increment ref count of this region
    __sync_fetch_and_add(&t->h->caps.rcnt,1);
    if(o[i].wc) { // transfer writer capabilities
      if( tcb()->tid == t->h->caps.writer_cv) {
        t->h->caps.writer_cv = tid;
      } else
        errquit("_treecopy: writer_cv"); 
    }
    //if the spawning thread is "sharing"
    // read capabilities, increment read count
    if(o[i].lc && t->caps.rd_cnt > o[i].lc )
     __sync_fetch_and_add(&t->h->caps.rd_cnt,1);
    struct _XTreeHandle *parent= t->parent_key;
    int idx;
    for(idx=0;idx<len && th(o,idx) != parent;idx++);
    if(idx < len) {
      n[i].parent_key = n + idx;
      n[i].next_key =  n[idx].sub_keys;
      n[idx].sub_keys = n + i;
    }  else
      n[i].parent_key = 0;
  }
  //remove capabilities transferred from this tree
  //start off child nodes so that we don't bump into
  //a deallocated node (if a parent gets deallocated
  //its subregions are also removed). Notice that the
  //array "o" is already sorted in the natural 
  //allocation order.
  for(i=len-1;i>-1;i--) 
    _xregion_cap_th(-o[i].rc,-o[i].wc,-o[i].lc,th(o,i));
}

typedef struct SpawnInfo {
 void *stack;
 unsigned int size;
} spawn_info_t;

spawn_info_t __init_new_thread_stack
   (caps_t c, int len,void *(*f)(void *) , 
    void *targ,int arg_len) {
  static unsigned int PADDING = 16;

   len = merge_caps(c,len); // sum caps
 // it should be at least 16 or we'll be
  const unsigned int STACK_SIZE = 
                     __get_stack_size(len,arg_len,0);

  sinfo_t i = (sinfo_t) GC_malloc(STACK_SIZE);
  i->tid = _next_tid();
  i->thread = f;
  i->stack_real_size = STACK_SIZE;
  i->stack_size =  STACK_SIZE - 
                   sizeof(struct _ThreadStack) + arg_len +
                   sizeof(struct _XTreeHandle)*len +
                   PADDING;
  memcpy(i+1,targ,arg_len);
  struct _XTreeHandle *r = (struct _XTreeHandle *)
                        (((char *)(i+1)) + arg_len);
  // fill-in the new tree and transfer capabilities
  __treecopy(c,r,len,i->tid);

  // set stack guard page so as to handle, page faults.
  // Here we simply quit.
  char *guard = ((char *) i) + STACK_SIZE - PAGE_SIZE;
  if( guard != (char *)ALIGN_PG(guard) )
      errquit("__init_new_thread: unaligned guard");
  	 /* Mark the page read-only. */
  if (mprotect(guard,1024,1))
    errquit("__init_new_thread: mprotect failed.");
 
  //return new stack
  spawn_info_t ret;
  ret.stack = (void *) i;
  ret.size = STACK_SIZE;
  return ret;
}

void _xspawn(caps_t caps, int len ,
             void * (*thread)(void *), void *arg, 
             int arg_len) {
 pthread_attr_t attr;
 pthread_t id;
 
if(pthread_attr_init(&attr) != 0)
   errquit("pthread_attr_init");

 if (pthread_attr_setdetachstate(&attr, 
                        PTHREAD_CREATE_DETACHED) != 0)
    errquit("pthread_attr_setdetachstate");

  spawn_info_t si = __init_new_thread_stack(caps,len,
                                   thread,arg,arg_len);

   //force "main" to wait for the new thread
   //TODO: if pthread_create fails throw 
   //an exception and atomically decrement all_done
   __sync_fetch_and_add(&all_done,1);

  if(pthread_attr_setstack(&attr,si.stack,si.size))
      errquit("pthread_attr_setstack: ");

 if(GC_pthread_create(&id,&attr,
                _thread_internal2,si.stack) != 0)
    errquit("internal error: _xspawn could"
                   " not create thread");
 if( pthread_attr_destroy(&attr) != 0 )
    errquit("pthread_attr_destroy");
}
#endif

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

#include <errno.h>
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
#define ALIGN_PG(p)   ((((long long) (p) + PAGE_SIZE-1) & (long long)~(PAGE_SIZE-1)))
#ifndef PROT_READ
#define PROT_READ 1
#endif

#ifndef SIGSTKSZ
  # define SIGSTKSZ 16384
  #define  MINSIGSTKSZ 2048
#endif


#define ts(x) ((thread_stack_t *)x)

#define BACKUP_STK_SZ  SIGSTKSZ
//#PAGE_SIZE
//__thread char __backup__stack[BACKUP_STK_SZ]; // suitable for catching sigsegv
//__thread char __second__stack[BACKUP_STK_SZ]  __attribute__ ((aligned (8))); // suitable for catching sigsegv
__thread int  __free_stack = 0;
__thread struct _ThreadKey _thread_key;
//invariant: next_tid SHOULD never be zero. Proof:
//(0) it starts off with 1
//(1) it always gets incremented by two
static volatile unsigned int next_tid = 1; 

typedef struct StackSizeInfo {
  unsigned int real_size;
  unsigned int begin_offset;
  unsigned int end_offset;
} ssinfo_t;

typedef struct _ThreadStack {
   void *(*thread)(void *);
   unsigned int tid;
   struct hashtable *h;
   ssinfo_t info;
} *sinfo_t;

unsigned int _next_tid() {
  return __sync_fetch_and_add(&next_tid,2);
}    

#define THREAD_STACKS 4
//volatile void *thread_stacks[THREAD_STACKS]; 
//volatile int 
void _init_thread_context(unsigned int id,
                          struct hashtable *h){
   memset(&_thread_key,0,sizeof(struct _ThreadKey));	 // create hashtable if there is not one specified 
/*	if(!h) { // main thread
		int i;
		for(i=0;i< THREAD_STACKS;i++) {
			thread_stacks[i] = 0;
		}
	}*/
   _thread_key.h = h?h:create_hashtable(25);
   _thread_key.tid = id;
   // install stack overflow handler
/*   if(stackoverflow_install_handler
      (&_exn_stackoverflow_handler,   
       __backup__stack,BACKUP_STK_SZ) < 0)
    errquit("Could not install stack handler");*/
   // instacl seg fault handler
   sigsegv_install_handler(&_exn_segv_handler);
}
#ifdef HAVE_THREADS
static inline void _fini_thread_context(){
	struct hashtable *h = tcb()->h;
	if( h != NULL) hashtable_destroy(h);
}

/*static void _thread_cleanup_stack() {
	void *si = __si;
  	fprintf(stderr,"\n[%d] thread_free: %p",tcb()->tid, si);
//   sigsegv_install_handler(&_exn_segv_handler);
	GC_free(si);
  	fprintf(stderr,"\n[%d] thread_free DONE: %p",tcb()->tid,si);
   // pthread_exit(0);
}*/

static void *_thread_internal2(void *arg){
    if(!arg) errquit("_thread_internal2: null arg");
    sinfo_t i = (sinfo_t) arg;
    _init_thread_context(i->tid,i->h);
//	 fprintf(stderr,"\n[%d]si = %p",tcb()->tid,i);
   // install top exception handler and execute thread if there's no exn
   if( _set_top_handler() == NULL ) {
#ifdef __RUNTIME_MEMORY_DEBUG_LOCKOP
            fprintf(stderr,"\n[%d] NEW THREAD.",	
              	 	tcb()->tid); 
#endif
	  (i->thread)((void *) (i+1));
   }
  // finalize thread context	 
//	fprintf(stderr,"\n[%d] fini_thread_context\n",i->tid);
  _fini_thread_context();
  // wake up main thread if I'm the last thread
  if(__sync_fetch_and_add(&all_done,-1) == 1)
   futex_wake((int *)&all_done,1);
  //HACK: generate segfault so as to switch
  //to the backup stack and free the current stack.
  // Mark the last page W/R. so that it can be deallocated
//  if (mprotect((void *) (((char *)i) + i->info.end_offset),1024,3))
//   errquit("__thread_internal2: mprotect failed.");

//  fprintf(stderr,"\n[%d]2 __si = %p",tcb()->tid,i);
  GC_free(i);
//  fprintf(stderr,"\n[%d]2 __si = %p. DONE",tcb()->tid,i);
  //goto_context(_thread_cleanup_stack,__second__stack,BACKUP_STK_SZ);
  //abort(); // should NOT reach this point
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
  for(i=0;i<len;i++){ // find tree handles
   //assert(c[i].h > 100);
   c[i].h = (struct _XRegionHandle *) 
                  get_tkey(c[i].h);
  // assert(c[i].h > 100);
  }
  qsort(c,len,sizeof(struct Caps),(int (*)(const void *,
                                  const void *))caps_cmp);
  int n=1;
  caps_t it = c,next=c;
  for(i=1;i<len;i++){
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

/*
 *  Thread Stack
 *
 * A. void (*thread)(void *);
 * B. tid
 * C. struct hashtable *
 * D. ssinfo_t
 * E. targ ---> arg_len bytes
 * F. XTreeHandle[merge_caps(caps,len)]
 * G. Actual Stack
 * ...
 * H. Stack End
 */

ssinfo_t __get_stack_size(
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
                      2 * PAGE_SIZE;
 ssinfo_t ret;
 ret.real_size = default_allocation;
	 //stack_estimation;
 ret.begin_offset = default_allocation;
 ret.end_offset = ret.real_size; // -  2 * PAGE_SIZE;
 return ret;
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
                 unsigned int tid, 
                 struct hashtable **h) {
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
    hashtable_insert(h,n+i); // create mapping

    //increment ref count of this region
      __sync_fetch_and_add(&t->h->caps.rcnt,1);
    t->h->single_thread_key = 0; //not thread local
    if(o[i].wc) { // transfer writer capabilities
      if( tcb()->tid == t->h->caps.writer_cv) {
        t->h->caps.writer_cv = tid;
      } else
        errquit("_treecopy: writer_cv"); 
    }
    //if there exist read capabilities, increment read count
    if(o[i].lc)
      __sync_fetch_and_add(&t->h->caps.rd_cnt,1);

#ifdef __RUNTIME_MEMORY_DEBUG_LOCKOP
    print_xtree(n+i,0);
#endif
    struct _XTreeHandle *parent= t->parent_key;
    int idx;
    if(parent)
       for(idx=0;idx<len && th(o,idx) != parent;idx++);
    else
       idx = len;
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

unsigned int stack_space = 0; // PTHREAD_STACK_MIN; // 10485760 / 10; // 1MB

sinfo_t __init_new_thread_stack
   (caps_t c, int len,void *(*f)(void *) , 
    void *targ,int arg_len) {
  len = merge_caps(c,len); // sum caps
 // it should be at least 16 or we'll be
  ssinfo_t ss = __get_stack_size(len,arg_len,stack_space);
  sinfo_t i = (sinfo_t) GC_malloc(ss.real_size);
  i->tid = _next_tid();
  i->thread = f;
  i->info = ss;
  i->h = create_hashtable(25);
  
//  fprintf(stderr,"\nthread_malloc: %p", i);

  memcpy(i+1,targ,arg_len);
  struct _XTreeHandle *r = (struct _XTreeHandle *)
                        (((char *)(i+1)) + arg_len);
  // fill-in the new tree and transfer capabilities
  __treecopy(c,r,len,i->tid,&i->h);

  // set stack guard page so as to handle, page faults.
  // Here we simply quit.
//  char *guard = (char *) ALIGN_PG(((char *) i) + ss.end_offset);

//  if(mprotect(guard,1024,1))
//    errquit("__init_new_thread: mprotect failed");
 
   //i->info.end_offset = (unsigned long long)guard -
 // 	 							   (unsigned long long)i;
  //assert(i->info.end_offset+1024 <= i->info.real_size);
  return i;
}

void _xspawn(caps_t caps, int len ,
             void * (*thread)(void *), void *arg, 
             int arg_len)
{
  pthread_attr_t attr;
  pthread_t id;
  int s;
 
  if ((s = pthread_attr_init(&attr)) != 0) {
    errno = s; perror("pthread_attr_init");
    errquit("internal error: _xspawn aborting\n");
  }

  if ((s = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) != 0) {
    errno = s; perror("pthread_attr_setdetachstate");
    errquit("internal error: _xspawn aborting\n");
  }

  sinfo_t si = __init_new_thread_stack(caps,len,thread,arg,arg_len);

  //force "main" to wait for the new thread
  //TODO: if pthread_create fails throw 
  //an exception and atomically decrement all_done
  __sync_fetch_and_add(&all_done,1);

  //  if(pthread_attr_setstack(&attr,
  //             ((char *)si)+si->info.begin_offset,
  //           si->info.end_offset-si->info.begin_offset))
  //      errquit("pthread_attr_setstack ");

  if (stack_space > 0) {
    if ((s = pthread_attr_setstacksize(&attr,stack_space)) != 0) {
      errno = s; perror("pthread_attr_setstacksize");
      errquit("internal error: _xspawn aborting\n");
    }
  }

  if ((s = GC_pthread_create(&id,&attr,_thread_internal2,si)) != 0) {
    errno = s; perror("pthread_create");
    errquit("internal error: _xspawn aborting\n");
  }
  if ((s = pthread_attr_destroy(&attr)) != 0) {
    errno = s; perror("pthread_attr_destroy");
    errquit("internal error: _xspawn aborting\n");
  }
}
#endif

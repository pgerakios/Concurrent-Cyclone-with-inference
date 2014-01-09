/* Copyright (C) 2008, 2009 Free Software Foundation, Inc.
   Contributed by Richard Henderson <rth@redhat.com>.
   This file is part of the GNU Transactional Memory Library (libitm).
   Libitm is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   Libitm is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
   FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
   more details.

   Under Section 7 of GPL version 3, you are granted additional
   permissions described in the GCC Runtime Library Exception, version
   3.1, as published by the Free Software Foundation.

   You should have received a copy of the GNU General Public License and
   a copy of the GCC Runtime Library Exception along with this program;
   see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
   <http://www.gnu.org/licenses/>.  */

/* Provide access to the futex system call.  */
//
  #include <stdio.h> 
//  #include <unistd.h>
  #include <errno.h>
//  #include <sched.h>
  #include <sys/syscall.h>
#ifndef __LP64__
  #define u32 unsigned int
#endif
  #include <linux/futex.h>
//  #include <sys/time.h>

#ifdef __LP64__
# ifndef SYS_futex
#  define SYS_futex	202
# endif

#define sys_futex0_wait x86_64_sys_futex0_wait

static inline long
x86_64_sys_futex0_wait (int *addr, long op, long val)
{
  register long r10 __asm__("%r10");
  long res;
  r10 = 0;            /* Wait indefinitely. */
   __asm volatile ("syscall"
                     : "=a" (res)
                     : "0" (SYS_futex), 
                         "D" (addr), 
                         "S" (op),
                         "d" (val), "r" (r10)
                       : "r11", "rcx", "memory");
      
    return res;
}
       
static inline long
sys_futex0 (int *addr, long op, long val)
{
  long res;

  __asm volatile ("syscall"
		  : "=a" (res)
		  : "0" (SYS_futex), "D" (addr), "S" (op), "d" (val)
		  : "r11", "rcx", "memory");

  return res;
}

#else
# ifndef SYS_futex
#  define SYS_futex	240
# endif

# ifdef __PIC__

static inline long
sys_futex0 (int *addr, int op, int val)
{
  long res;

  __asm volatile ("xchgl\t%%ebx, %2\n\t"
		  "int\t$0x80\n\t"
		  "xchgl\t%%ebx, %2"
		  : "=a" (res)
		  : "0"(SYS_futex), "r" (addr), "c"(op),
		    "d"(val), "S"(0)
		  : "memory");
  return res;
}

# else

static inline long
sys_futex0 (int *addr, int op, int val)
{
  long res;

  __asm volatile ("int $0x80"
		  : "=a" (res)
		  : "0"(SYS_futex), "b" (addr), "c"(op),
		    "d"(val), "S"(0)
		  : "memory");
  return res;
}

# endif /* __PIC__ */
#endif /* __LP64__ */


//#define FUTEX_WAIT		0
//#define FUTEX_WAKE		1
//#define FUTEX_PRIVATE_FLAG	128L


static long int gtm_futex_wait = FUTEX_WAIT | FUTEX_PRIVATE_FLAG;
static long int gtm_futex_wake = FUTEX_WAKE | FUTEX_PRIVATE_FLAG;

/* Define sys_futex0_wait if backend's wait syscall is different than
     the wake. See x86_64 for an example. */
#ifndef sys_futex0_wait
#define sys_futex0_wait sys_futex0
#endif 


#ifdef __PERF__COUNTERS__
volatile long long spin_count = 0;
volatile long long futex_count = 0;
volatile long long futex_neq = 0;
volatile long long futex_timeout = 0;
volatile long long futex_other = 0;
volatile long long lock_count = 0;
volatile long long unlock_count = 0;
void print_stats() {
  fprintf(stderr,"\nSTATS:\n"
         "spin count   : %lld\n"
         "futex count  : %lld\n"
         "futex neq    : %lld\n"
         "futex_timeout: %lld\n"
         "lock_count   : %lld\n"
         "unlock_count : %lld\n",
          spin_count,
          futex_count,
          futex_neq,
          futex_timeout,
          lock_count,
          unlock_count);
}
#else
void print_stats() {
}
#endif

void exit(int);
void err( char *msg ){
   fprintf(stderr,"\nERROR : %s\n",msg);
   exit(-1);
}


#define __FUTEX_SPIN_COUNT__ 50
#define __DO__SPIN

#ifdef __DO__SPIN
#define cpu_relax()  asm volatile("rep; nop" ::: "memory")
volatile long __do__spin = 0;
#endif
//#define cpu_relax()      __asm__ __volatile__ ( "pause")
void futex_wait (int *addr, int val)
{
  long res;
   #ifdef __DO__SPIN
   if(__do__spin){
   unsigned long long i;
    for (i = 0; i < __FUTEX_SPIN_COUNT__; i++){
    int addr_val = *addr; //__sync_fetch_and_add(addr, 0);
    if (__builtin_expect (addr_val != val, 0)){
      #ifdef __PERF__COUNTERS__
      __sync_fetch_and_add(&spin_count, 1);
      #endif
      return;
    }
    else
      cpu_relax ();
  }
  }
  #endif
  res = sys_futex0_wait (addr, gtm_futex_wait, val); 
 #ifdef __PERF__COUNTERS__
  __sync_fetch_and_add(&futex_count, 1);
 #endif    
  if (__builtin_expect (res == -ENOSYS, 0))
    {
       err("ENOSYS");
      //gtm_futex_wait = FUTEX_WAIT;
      //gtm_futex_wake = FUTEX_WAKE;
      //res = sys_futex0_wait (addr, FUTEX_WAIT, val); 
    }
  if (__builtin_expect (res < 0, 0))
    {
      if (res == -EWOULDBLOCK ){
        #ifdef __PERF__COUNTERS__
         __sync_fetch_and_add(&futex_neq, 1);
        #endif
      }
      else if ( res == -ETIMEDOUT){
        #ifdef __PERF__COUNTERS__
         __sync_fetch_and_add(&futex_timeout, 1);
        #endif            
      }
      else if (res == -EFAULT){
        err("EFAULT");
      }
      else{
        err("OTHER");
      }
    }
}

void
futex_wake (int *addr, int count)
{
  long res = sys_futex0 (addr, gtm_futex_wake, count);
  if (__builtin_expect (res == -ENOSYS, 0))
    {
      gtm_futex_wait = FUTEX_WAIT;
      gtm_futex_wake = FUTEX_WAKE;
      res = sys_futex0 (addr, FUTEX_WAKE, count);
    }
  if (__builtin_expect (res < 0, 0)){
     err("FUTEX_WAKE impl");
  }
}

void simple_futex_wait(int *addr, int val)
{
  long res;
  res = sys_futex0_wait (addr, gtm_futex_wait, val); 
  if (__builtin_expect (res == -ENOSYS, 0))
    {
       err("ENOSYS");
    }
  if (__builtin_expect (res < 0, 0))
    {
      if (res == -EWOULDBLOCK ){
      }
      else if ( res == -ETIMEDOUT){
      }
      else if (res == -EFAULT){
        err("EFAULT");
      }
      else{
        err("OTHER");
      }
    }
}
#include "runtime_internal.h"

/*	
static unsigned next_thread_tid = 5;
static __thread unsigned int thread_tid = 0;
unsigned int gettid () {
  if(thread_tid == 0){
    thread_tid =  __sync_fetch_and_add(&next_thread_tid,1);
     //thread_tid = (unsigned int) syscall(SYS_gettid);
	 if(thread_tid == 0) {
		 fprintf(stderr,"\nImpossible ! gettid"); 
		 abort();
	 }
  }
  return thread_tid;
}  */

#define gettid() tcb()->tid

int wait_lock(unsigned int *lock){
  unsigned int self = gettid(); 
  int prev_val = 1;
/*int lock_val;
  if( *lock == self ){
    fprintf(stderr,"\n[%d] GOT LOCK %p."
			 	" I HAD IT ALREADY.", tcb()->tid,lock);
	 abort();
	 // return 0;
  }*/
#ifdef __PERF__COUNTERS__
  __sync_fetch_and_add(&lock_count, 1);
#endif  
  while(!__sync_bool_compare_and_swap(lock,0,self)) {
	//prev_val = __sync_val_compare_and_swap(lock,0,self);
	/*lock_val = *lock;
	if( prev_val !=0  && lock_val == self ) {
		fprintf(stderr,"\nBug 0: __sync_bool_compare_and_swap");
	   abort();	
	}
	if( prev_val == 0  && lock_val != self ) {
		fprintf(stderr,"\nBug 1: __sync_bool_compare_and_swap");
	   abort();	
	}
 	if(prev_val == 0)  break;
        */
    //if(!prev_val) break;
    prev_val = *lock;
    if(!prev_val) continue;
    futex_wait((int *)lock,(int) prev_val);
  }
/*  if(*lock != self){
		 fprintf(stderr,"\nImpossible ! wait_lock"); 
		 abort();
  }
  */
  return 1;
}

int unlock(unsigned int *lock) {
/* if(*lock != self) {
	  fprintf(stderr,"\n[%d]UNLOCK BUG",self);
	  abort();
  }*/
  unsigned int self = gettid(); 
  assert(*lock == self);
  *lock = 0;
#ifdef __PERF__COUNTERS__
     __sync_fetch_and_add(&unlock_count, 1);
#endif     
//  __sync_fetch_and_add(lock, 0); //redundant for x86 ?
  futex_wake((int *)lock,1); // wake up exactly one thread
  return 1;
}

int wake_all(unsigned int *lock) {
  futex_wake((int *)lock,(int) -1); // wake up all threads
  return 0;
}

int wake_single(unsigned int *lock) {
  futex_wake((int *)lock,(int) 1); // wake up exactly one thread
  return 0;
}
#if 0
#define SPIN_COUNT  ((unsigned int)12)
#define MAX_SPINS    0
static __thread unsigned long dummy = 1;
/* Spin for 2**n units. */
static void spin(int n){
  int i;
  unsigned long j = (dummy);
  for (i = 0; i < (2 << n); ++i){
       j *= 5;
       j -= 4;
  }
  dummy = j;
}
#endif 

//######################################################################
#include <ucontext.h>
static __thread ucontext_t uctx_aux;

//#define errquit(msg) do { perror(msg); exit(-1); } while (0)

static void setup_context(ucontext_t *ctx,void (*fun)(void),void *stack, unsigned int size, ucontext_t *next) {
   if (getcontext(ctx) == -1)
        errquit("getcontext");
    ctx->uc_stack.ss_sp = stack;
    ctx->uc_stack.ss_size = size;
    ctx->uc_link = next;
    makecontext(ctx,fun,0);
}

void goto_context(void (*fun)(void),void *stack, unsigned int size) {
   setup_context(&uctx_aux,fun,stack,size,NULL);
   setcontext(&uctx_aux);
}
//######################################################################
#ifdef  __TEST__
int main(int argc, char *argv[]){
   //PG: test it
   return 0;
}
#endif
//######################################################################

#ifdef __PERF__COUNTERS__
//#include <sys/time.h>
#include <time.h>
static const int cld_id = CLOCK_THREAD_CPUTIME_ID;
void _start_time(struct timespec *last_time){
   assert(clock_gettime(CLOCK_MONOTONIC, last_time) == 0);
}

unsigned long _end_time( struct timespec last_time){
 struct timespec current_time;
//  long elapsed_utime;    /* elapsed time in microseconds */
//  long elapsed_mtime;    /* elapsed time in milliseconds */
//  long elapsed_seconds;  /* diff between seconds counter */
//  long elapsed_useconds; /* diff between microseconds counter */

   assert(clock_gettime(CLOCK_MONOTONIC, &current_time) == 0);
   assert(current_time.tv_sec >= last_time.tv_sec);
   assert((current_time.tv_sec > last_time.tv_sec) ||
              (current_time.tv_nsec >= last_time.tv_nsec));

   unsigned long long  ret =
          current_time.tv_sec*1000ULL  +
          current_time.tv_nsec/1000000ULL  -
          last_time.tv_sec*1000ULL  -
          last_time.tv_nsec/1000000ULL ;

  if(ret < 0){
    fprintf(stderr,"\nerror: ret = %ld\n",
            (long)ret);
    exit(-1);
  }
  return (unsigned long) ret;
}


static __thread struct timespec last_time;
void start_time() {
    _start_time(&last_time);
}
void end_time() {
   _end_time(last_time);
}

static __thread struct timespec last_time0;
__thread unsigned long long xregion_malloc_counter = 0;
void start_xregion_malloc() {
    _start_time(&last_time0); 
}

void end_xregion_malloc() {
   xregion_malloc_counter += _end_time(last_time0);
}

unsigned long get_xregion_malloc_time(){
   return (unsigned long) xregion_malloc_counter;
}
#endif

void Cyc_Core_set_lock_yield_on() {
    __do__spin = 1;
}

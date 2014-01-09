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
#define cpu_relax()  asm volatile("rep; nop" ::: "memory")

#ifdef __PERF__COUNTERS__
extern __thread int tid; // index from  0-1024 set extern
extern unsigned int futex_count[1024];
extern unsigned long usec[1024];
extern unsigned int futex_timeout[1024];
extern unsigned int futex_neq[1024];
extern unsigned int futex_count[1024];

 #include <sys/time.h>
__thread  struct timeval tempo1, tempo2;
void start_time(){
   gettimeofday(&tempo1, NULL);
}

long end_time(){
//  long elapsed_utime;    /* elapsed time in microseconds */
  long elapsed_mtime;    /* elapsed time in milliseconds */
  long elapsed_seconds;  /* diff between seconds counter */
  long elapsed_useconds; /* diff between microseconds counter */

  gettimeofday(&tempo2, NULL);
  elapsed_seconds  = tempo2.tv_sec  - tempo1.tv_sec;
  elapsed_useconds = tempo2.tv_usec - tempo1.tv_usec;
  //elapsed_utime = (elapsed_seconds) * 1000000 + elapsed_useconds;
  elapsed_mtime = ((elapsed_seconds) * 1000 + elapsed_useconds/1000.0) ;
 return elapsed_useconds;                                                                        
}
#endif

void exit(int);
void err( char *msg ){
   fprintf(stderr,"\nERROR : %s\n",msg);
   exit(-1);
}

void futex_wait (int *addr, int val)
{
  //unsigned long long i;
  long res;

  /*for (i = 0; i < __FUTEX_SPIN_COUNT__; i++){
    addr_val = __sync_fetch_and_add(addr, 0);
    if (__builtin_expect (addr_val != val, 0)){
      return;
    }
    else
      cpu_relax ();
  }*/

 #ifdef __PERF__COUNTERS__
  start_time();
 #endif
  res = sys_futex0_wait (addr, gtm_futex_wait, val); 
 #ifdef __PERF__COUNTERS__
   long tm = end_time();
    __sync_fetch_and_add(&usec[tid],tm);
    __sync_fetch_and_add(&futex_count[tid], 1);
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
            __sync_fetch_and_add(&futex_neq[tid], 1);
        #endif
      }
      else if ( res == -ETIMEDOUT){
        #ifdef __PERF__COUNTERS__
            __sync_fetch_and_add(&futex_timeout[tid], 1);
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

__thread unsigned int thread_tid = 0;

unsigned int gettid () {
  if(thread_tid == 0)
    thread_tid = (unsigned int) syscall(SYS_gettid);
  return thread_tid;
}

int wait_lock(unsigned int *lock){
  unsigned int self = gettid(); 
  if( *lock == self ) return 0;
  do {
     while(*lock != 0)
      futex_wait((int *)lock,(int) *lock);
  } while(!__sync_bool_compare_and_swap(lock,0,self));
  return 1;
}

int unlock(unsigned int *lock){
  unsigned int self = gettid(); 
  if(*lock != self) return 0;
  *lock = 0;
  __sync_fetch_and_add(lock, 0); //redundant for x86 ?
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

#ifdef  __TEST__
int main(int argc, char *argv[]){
   //PG: test it
   return 0;
}
#endif

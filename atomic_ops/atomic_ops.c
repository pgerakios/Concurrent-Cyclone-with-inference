/*
 * Copyright (c) 2003 by Hewlett-Packard Company.  All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE. 
 */

/*
 * Initialized data and out-of-line functions to support atomic_ops.h
 * go here.  Currently this is needed only for pthread-based atomics
 * emulation, or for compare-and-swap emulation.
 */

#undef AO_FORCE_CAS

#include <pthread.h>
#include <signal.h>
#include <sys/select.h>
#include "atomic_ops.h"  /* Without cas emulation! */

/*
 * Lock for pthreads-based implementation.
 */

pthread_mutex_t AO_pt_lock = PTHREAD_MUTEX_INITIALIZER;

/*
 * Out of line compare-and-swap emulation based on test and set.
 * 
 * We use a small table of locks for different compare_and_swap locations.
 * Before we update perform a compare-and-swap, we grap the corresponding
 * lock.  Different locations may hash to the same lock, but since we
 * never acquire more than one lock at a time, this can't deadlock.
 * We explicitly disable signals while we perform this operation.
 *
 * FIXME: We should probably also suppport emulation based on Lamport
 * locks, since we may not have test_and_set either.
 */
#define AO_HASH_SIZE 16

#define AO_HASH(x) (((unsigned long)(x) >> 12) & (AO_HASH_SIZE-1))

AO_TS_T AO_locks[AO_HASH_SIZE] = {
	AO_TS_INITIALIZER, AO_TS_INITIALIZER,
	AO_TS_INITIALIZER, AO_TS_INITIALIZER,
	AO_TS_INITIALIZER, AO_TS_INITIALIZER,
	AO_TS_INITIALIZER, AO_TS_INITIALIZER,
	AO_TS_INITIALIZER, AO_TS_INITIALIZER,
	AO_TS_INITIALIZER, AO_TS_INITIALIZER,
	AO_TS_INITIALIZER, AO_TS_INITIALIZER,
	AO_TS_INITIALIZER, AO_TS_INITIALIZER,
};

static AO_T dummy = 1;

#define SPIN_COUNT  ((unsigned int)12)

/* Spin for 2**n units. */
static void spin(int n)
{
  int i;
  AO_T j = AO_load(&dummy);

  for (i = 0; i < (2 << n); ++i)
    {
       j *= 5;
       j -= 4;
    }
  AO_store(&dummy, j);
}

static void lock_ool(volatile AO_TS_T *l)
{
  int i = 0;
  struct timeval tv;

  while (AO_test_and_set_acquire(l) == AO_TS_SET) {
    if (++i < 12)
      spin(i);
    else
      {
	/* Short async-signal-safe sleep. */
	tv.tv_sec = 0;
	tv.tv_usec = (i > 28? 100000 : (1 << (i - 12)));
	select(0, 0, 0, 0, &tv);
      }
  }
}

AO_INLINE void lock(volatile AO_TS_T *l)
{
  if (AO_test_and_set_acquire(l) == AO_TS_SET)
    lock_ool(l);
}

AO_INLINE void unlock(volatile AO_TS_T *l)
{
  AO_CLEAR(l);
}

static sigset_t all_sigs;

static volatile AO_T initialized = 0;

static volatile AO_TS_T init_lock = AO_TS_INITIALIZER;

int AO_compare_and_swap_emulation(volatile AO_T *addr, AO_T old,
				   AO_T new_val)
{
  AO_TS_T *my_lock = AO_locks + AO_HASH(addr);
  sigset_t old_sigs;
  int result;

  if (!AO_load_acquire(&initialized))
    {
      lock(&init_lock);
      if (!initialized) sigfillset(&all_sigs);
      unlock(&init_lock);
      AO_store_release(&initialized, 1);
    }
  sigprocmask(SIG_BLOCK, &all_sigs, &old_sigs);
  	/* Neither sigprocmask nor pthread_sigmask is 100%	*/
  	/* guaranteed to work here.  Sigprocmask is not 	*/
  	/* guaranteed be thread safe, and pthread_sigmask	*/
  	/* is not async-signal-safe.  Under linuxthreads,	*/
  	/* sigprocmask may block some pthreads-internal		*/
  	/* signals.  So long as we do that for short periods,	*/
  	/* we should be OK.					*/
  lock(my_lock);
  if (*addr == old)
    {
      *addr = new_val;
      result = 1;
    }
  else
    result = 0;
  unlock(my_lock);
  sigprocmask(SIG_SETMASK, &old_sigs, NULL);
  return result;
}

void AO_store_full_emulation(volatile AO_T *addr, AO_T val)
{
  AO_TS_T *my_lock = AO_locks + AO_HASH(addr);
  lock(my_lock);
  *addr = val;
  unlock(my_lock);
}

 #if defined(_MSC_VER) \
	      || defined(_WIN32) && !defined(__CYGWIN32__) && !defined(__CYGWIN__)
 #error Unsupported platform.
 #elif defined(__linux__)
 #include <sys/syscall.h>
  #define FUTEX_WAIT      0
  #define FUTEX_WAKE      1
  #define FUTEX_FD     2
  #define FUTEX_REQUEUE      3
  #define FUTEX_CMP_REQUEUE  4
  #define FUTEX_WAKE_OP      5
  #define FUTEX_LOCK_PI      6
  #define FUTEX_UNLOCK_PI    7
  #define FUTEX_TRYLOCK_PI   8

   #define futex(a,b,c,d,e,f) syscall(SYS_futex,a,b,c,d,e,f)
	#define futex_wait(ptr,val) futex(ptr, FUTEX_WAIT, val, NULL, NULL , 0)
	#define futex_wake(ptr,val) futex(ptr, FUTEX_WAKE, val, NULL, NULL , 0)
	void   AO_destroy_lock( AO_lock_ptr_t lock ){}
	void   AO_initialize_lock( AO_lock_ptr_t lock, int locked ){
		if( locked )
		 AO_store_release((ao_t *)lock,pthread_self());
		else
		 AO_store_release((ao_t *)lock,0);
	}

	/* may cause starvation ... */
	int AO_wait_lock( AO_lock_ptr_t lock  ){
	  pthread_t self = pthread_self();
	  AO_lock_t lk_val;
	  if( AO_load((ao_t*) lock) == (ao_t) self ) return 0;
	  while( AO_compare_and_swap_release((ao_t *)lock,(ao_t )0,(ao_t) self) == 0 ){
		  lk_val = AO_load((ao_t*) lock);
		  if( lk_val  == 0 ) continue;
			futex_wait(lock,lk_val);
	  }
	  return 1;
	}
	int  AO_unlock( AO_lock_ptr_t lock ){
	  pthread_t self = pthread_self();
	  if( AO_load((ao_t*) lock) != (ao_t) self ) return 0;
	  AO_store_release(lock,0);
	  futex_wake(lock,1); // wake up exactly one thread
	  return 1;
	}

	void AO_steal_lock( AO_lock_ptr_t lock ){
		AO_store_release(lock,pthread_self());
	}

	int    AO_have_lock( AO_lock_ptr_t lock ){
	  pthread_t self = pthread_self();
	  return  AO_compare_and_swap_release((ao_t *)lock,(ao_t )self,(ao_t) 
			  												self) == 1;
	}
/////////////////////////////////////////////////////////////////
//
#define AO_do_wait_spin	  { if (++i < SPIN_COUNT) \
							      	spin(i); \
 									else \
									   sched_yield(); \
								  i++;		  }


	void   AO_initialize_lock_g( AO_lock_ptr_t lock,
										  AO_lock_val_t *val){
		if( val != 0)
		 AO_store_release((ao_t *)lock,(ao_t)*val);
		else
		 AO_store_release((ao_t *)lock,0);
	}

	/* may cause starvation ... */

	int AO_wait_lock_g( AO_lock_ptr_t lock, AO_lock_val_t self ){
	  AO_lock_t lk_val;
	  if( AO_load((ao_t*) lock) == (ao_t) self ) return 0;
	  while( AO_compare_and_swap_release((ao_t *)lock,(ao_t )0,(ao_t) self) == 0 ){
		  lk_val = AO_load((ao_t*) lock);
		  if( lk_val  == 0 ) continue;
			futex_wait(lock,lk_val);
	  }
	  return 1;
	}
	int  AO_unlock_g( AO_lock_ptr_t lock, AO_lock_val_t self ){
	  if( AO_load((ao_t*) lock) != (ao_t) self ) return 0;
	  AO_store_release(lock,0);
	  futex_wake(lock,1); // wake up exactly one thread
	  return 1;
	}


/*	int AO_wait_lock_g( AO_lock_ptr_t lock, AO_lock_val_t self ){
	  AO_lock_t lk_val;
	  unsigned int i=0;
	  if( AO_load((ao_t*) lock) == (ao_t) self ) return 0;
	  while(1){
		  lk_val = *lock; //AO_load((ao_t*) lock);
		  if( lk_val  == 0 ) AO_do_wait_spin;
		  if(AO_compare_and_swap_release((ao_t *)lock,(ao_t )0,(ao_t) self)) break;
	  }
	  return 1;
	}

	int  AO_unlock_g( AO_lock_ptr_t lock, AO_lock_val_t self ){
	  if( AO_load((ao_t*) lock) != (ao_t) self ) return 0;
	  AO_store_release(lock,0);
	  return 1;
	}
*/	

	void AO_steal_lock_g( AO_lock_ptr_t lock,AO_lock_val_t self ){
		AO_store_release((ao_t *)lock,(ao_t)self);
	}

	int  AO_have_lock_g( AO_lock_ptr_t lock, AO_lock_val_t self ){
	  return  AO_compare_and_swap_release((ao_t *)lock,(ao_t )self,(ao_t) 
			  												self) == 1;
	}
	
/////////////////////////////////////////////////////////////////
/*	void AO_barrier_wait(AO_lock_ptr_t lock){
	  AO_lock_t lk_val;
	  while( (lk_val=AO_load((ao_t*) lock)) != (ao_t) 0 ){
			futex_wait(lock,lk_val);
	  }
	}*/

	void AO_barrier_add(AO_lock_ptr_t lock, unsigned int v){
     ao_t v1 =	AO_fetch_and_add_release((ao_t*)lock,(ao_t)v);
	  if( v1 == 1 )
	   futex_wake(lock,1); // wake up exactly one thread
	}
////////////////////////////////////////////////////////////////

void AO_wait_value(AO_lock_ptr_t lock, AO_lock_val_t val,int eq){
	  AO_lock_t lk_val;
	  if ( eq == 0 ){ // not equal
		  while((lk_val = AO_load((ao_t*) lock)) != (AO_lock_t) val ){
			  futex_wait(lock,lk_val);
		  }
		}
	  else if(eq  == 1 ){ // equal
	   while((lk_val = AO_load((ao_t*) lock)) == (AO_lock_t) val ){
			futex_wait(lock,lk_val);
	   }
	  }
	  else if( eq == 2 ){ //less than
		  while((lk_val = AO_load((ao_t*) lock)) < (AO_lock_t) val ){
			  futex_wait(lock,lk_val);
		  }
	  }
	  else if( eq == 3 ){ //gt than
		  while((lk_val = AO_load((ao_t*) lock)) > (AO_lock_t) val ){
			  futex_wait(lock,lk_val);
		  }
	  }
}

void AO_signal_value(AO_lock_ptr_t lock, AO_lock_val_t val){
	 futex_wake(lock,val); // wake up exactly one thread
}

void AO_check_value_yield(AO_lock_ptr_t lock, AO_lock_val_t val,int eq){
	  AO_lock_t lk_val;
	  if ( eq == 0 ){ // not equal
		  while((lk_val = AO_load((ao_t*) lock)) != (AO_lock_t) val ){
			  sched_yield();
		  }
		}
	  else if(eq  == 1 ){ // equal
	   while((lk_val = AO_load((ao_t*) lock)) == (AO_lock_t) val ){
			  sched_yield();
	   }
	  }
	  else if( eq == 2 ){ //less than
		  while((lk_val = AO_load((ao_t*) lock)) < (AO_lock_t) val ){
			  sched_yield();
		  }
	  }
	  else if( eq == 3 ){ //gt than
		  while((lk_val = AO_load((ao_t*) lock)) > (AO_lock_t) val ){
			  sched_yield();
		  }
	  }
}
#define AO_check_value_spin_loop_body_break { \
			    if (++i < SPIN_COUNT) \
			      spin(i); \
 				 else \
				  sched_yield(); \
			  i++;		  }\
		      break;  

#define AO_check_value_spin_loop_header(symb)   while((lk_val = AO_load((ao_t*) lock)) symb (AO_lock_t) val ) \
							AO_check_value_spin_loop_body_break

void AO_check_value_spin(const AO_lock_ptr_t lock, const AO_lock_val_t val,const int eq){
	  AO_lock_t lk_val;
	  unsigned int i=0;
	  switch(eq){
	  case 0:
		AO_check_value_spin_loop_header(!=) 
	  case 1:
		AO_check_value_spin_loop_header(==) 
	  case 2:
		AO_check_value_spin_loop_header(<) 
	  case 3:
		AO_check_value_spin_loop_header(>) 
	  }
}

 #else
 #error Unsupported platform.
 #endif


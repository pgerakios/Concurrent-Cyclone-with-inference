#ifndef _PTHREAD_H_
#define _PTHREAD_H_
#ifndef _size_t_def_
#define _size_t_def_
typedef unsigned int size_t;
#endif
#ifndef ___sched_param_def_
#define ___sched_param_def_
struct __sched_param{
  int __sched_priority;
};
#endif
#ifndef ___atomic_lock_t_def_
#define ___atomic_lock_t_def_
typedef int __atomic_lock_t;
#endif
#ifndef __pthread_fastlock_def_
#define __pthread_fastlock_def_
struct _pthread_fastlock{
  long __status;
  __atomic_lock_t __spinlock;
};
#endif
#ifndef __pthread_descr_def_
#define __pthread_descr_def_
typedef struct _pthread_descr_struct * _pthread_descr;
#endif
#ifndef _pthread_attr_t_def_
#define _pthread_attr_t_def_
typedef struct __pthread_attr_s{
  int __detachstate;
  int __schedpolicy;
  struct __sched_param __schedparam;
  int __inheritsched;
  int __scope;
  size_t __guardsize;
  int __stackaddr_set;
  void * __stackaddr;
  size_t __stacksize;
} pthread_attr_t;
#endif
#ifndef ___pthread_cond_align_t_def_
#define ___pthread_cond_align_t_def_
typedef long long __pthread_cond_align_t;
#endif
#ifndef _pthread_cond_t_def_
#define _pthread_cond_t_def_
typedef struct {struct _pthread_fastlock __c_lock;
		_pthread_descr __c_waiting;
		char __padding[((48U - sizeof(struct _pthread_fastlock)) - sizeof(_pthread_descr)) - sizeof(__pthread_cond_align_t)];
		__pthread_cond_align_t __align;} pthread_cond_t;
#endif
#ifndef _pthread_condattr_t_def_
#define _pthread_condattr_t_def_
typedef struct {int __dummy;} pthread_condattr_t;
#endif
#ifndef _pthread_mutex_t_def_
#define _pthread_mutex_t_def_
typedef struct {int __m_reserved;
		int __m_count;
		_pthread_descr __m_owner;
		int __m_kind;
		struct _pthread_fastlock __m_lock;} pthread_mutex_t;
#endif
#ifndef _pthread_mutexattr_t_def_
#define _pthread_mutexattr_t_def_
typedef struct {int __mutexkind;} pthread_mutexattr_t;
#endif
#ifndef _pthread_t_def_
#define _pthread_t_def_
typedef unsigned long pthread_t;
#endif

  extern "C" int pthread_mutex_init(pthread_mutex_t @mutex,
				    const pthread_mutexattr_t *attr);

  extern "C" int pthread_mutex_lock (pthread_mutex_t @mutex);

  extern "C" int pthread_mutex_unlock (pthread_mutex_t @mutex);

  extern "C" int pthread_cond_init (pthread_cond_t @cond,
                                    const pthread_condattr_t *attr);

  extern "C" int pthread_cond_signal (pthread_cond_t *cond);

  extern "C" int pthread_cond_wait (pthread_cond_t *cond, pthread_mutex_t *mutex);

      // TODO: could change the type of pthread_t to be pthread_t<`a>
      // where `a is the return type for the invocation function.
      // Then the type of join() could be changed to properly
      // communicate the type back to the caller.
  extern "C" int pthread_create (pthread_t @thread, const pthread_attr_t *attr,
                                 `a (*__start_routine) (`b), `b arg: regions(`b) > `H);

  extern "C" int pthread_join (pthread_t thread, void **ret);
#endif

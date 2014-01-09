#ifndef _PTHREAD_H_
#define _PTHREAD_H_
#ifndef _pthread_t_def_
#define _pthread_t_def_
typedef unsigned long pthread_t;
#endif
#ifndef _pthread_attr_t_def_
#define _pthread_attr_t_def_
typedef union {char __size[36U];
	       long __align;} pthread_attr_t;
#endif
#ifndef ___pthread_slist_t_def_
#define ___pthread_slist_t_def_
typedef struct __pthread_internal_slist{
  struct __pthread_internal_slist * __next;
} __pthread_slist_t;
#endif
#ifndef _pthread_mutex_t_def_
#define _pthread_mutex_t_def_
typedef union {struct __pthread_mutex_s{
		 int __lock;
		 unsigned int __count;
		 int __owner;
		 int __kind;
		 unsigned int __nusers;
		 union {int __spins;
			__pthread_slist_t __list;} ;
	       } __data;
	       char __size[24U];
	       long __align;} pthread_mutex_t;
#endif
#ifndef _pthread_mutexattr_t_def_
#define _pthread_mutexattr_t_def_
typedef union {char __size[4U];
	       long __align;} pthread_mutexattr_t;
#endif
#ifndef _pthread_cond_t_def_
#define _pthread_cond_t_def_
typedef union {struct {int __lock;
		       unsigned int __futex;
		       unsigned long long __total_seq;
		       unsigned long long __wakeup_seq;
		       unsigned long long __woken_seq;
		       void * __mutex;
		       unsigned int __nwaiters;
		       unsigned int __broadcast_seq;} __data;
	       char __size[48U];
	       long long __align;} pthread_cond_t;
#endif
#ifndef _pthread_condattr_t_def_
#define _pthread_condattr_t_def_
typedef union {char __size[4U];
	       long __align;} pthread_condattr_t;
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

  extern "C" int pthread_attr_init    (pthread_attr_t @attr);
  extern "C" int pthread_attr_destroy (pthread_attr_t @attr);

  extern "C" int pthread_attr_setstacksize (pthread_attr_t @attr,
                                            size_t stacksize);
  extern "C" int pthread_attr_getstacksize (const pthread_attr_t @attr,
                                            size_t @stacksize);

#endif

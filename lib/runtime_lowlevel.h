#ifndef __RUNTIME__LOWLEVEL__
#define  __RUNTIME__LOWLEVEL__

void futex_wake (int *addr, int count);
void futex_wait (int *addr, int val);
void simple_futex_wait(int *addr, int val);
unsigned int gettid();
int wait_lock(unsigned int *);
int unlock(unsigned int *);
int wake_all(unsigned int *);
int wake_single(unsigned int *);
void goto_context(void (*fun)(void),void *stack, unsigned int size);

#endif

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

// This is the C "runtime library" to be used with the output of the
// Cyclone to C translator

#include <stdio.h>
#include "runtime_internal.h"

/* struct _fat_ptr Cstring_to_string(Cstring s) { */
/*   struct _fat_ptr str; */
/*   if (s == NULL) { */
/*     str.base = str.curr = str.last_plus_one = NULL; */
/*   } */
/*   else { */
/*     int sz = strlen(s)+1; */
/*     str.curr = (char *)_cycalloc_atomic(sz); */
/*     if (str.curr == NULL)  */
/*       _throw_badalloc(); */
/*     str.base = str.curr; */
/*     str.last_plus_one = str.curr + sz; */

/*     // Copy the string in case the C code frees it or mangles it */
/*     str.curr[--sz] = '\0'; */
/*     while(--sz>=0) */
/*       str.curr[sz]=s[sz]; */
/*   } */
/*   return str; */
/* } */
// no longer copying the C string (see above if this is bogus)
static struct _fat_ptr Cstring_to_string(Cstring s) {
  struct _fat_ptr str;
  if (s == NULL) {
    str.base = str.curr = str.last_plus_one = NULL;
  }
  else {
    unsigned int sz = strlen(s)+1;
    str.curr = str.base = (unsigned char *)s;
    str.last_plus_one = str.curr + sz;
  }
  return str;
}

// argc is redundant
struct _fat_argv { 
  struct _fat_ptr *base;
  struct _fat_ptr *curr;
  struct _fat_ptr *last_plus_one;
};

// Define struct __cycFILE, and initialize stdin, stdout, stderr
struct Cyc___cycFILE { // must match defn in boot_cstubs.c and boot_cycstubs.cyc
  FILE *file;
} Cyc_stdin_v, Cyc_stdout_v, Cyc_stderr_v,
  *Cyc_stdin = &Cyc_stdin_v,
  *Cyc_stdout = &Cyc_stdout_v,
  *Cyc_stderr = &Cyc_stderr_v;

extern int Cyc_main(int argc, struct _fat_argv argv);

/* #ifdef _HAVE_PTHREAD_ */
/* static pthread_once_t key_once = PTHREAD_ONCE_INIT; */
/* void init_keys_once() { */
/*   int status; */
/*   //  fprintf(stderr, "Initing keys ... \n"); */
/*   _init_exceptions(); */
/*   _init_stack(); */
/* } */
/* #endif */

volatile unsigned int all_done = 0;
void Cyc_Core_main_join();

void print_stats();
extern unsigned int stack_space;
void setup_runtime_lowlevel(); 
int main(int argc, char **argv) {
  // initialize region system
  //
#if HAVE_THREADS
  char *stack = getenv("CYC_THREAD_STACK");
  if(stack) {
	  stack_space = atoi(stack); 
  }
#endif
 _init_thread_context(_next_tid(),NULL); //no _fini required let OS reclaim memory.
  // MWH: could do this in pthread_once above, but there's no need
  // because we won't have multiple main() threads
 // _init_stack();
 // _init_exceptions();
/* #ifdef _HAVE_PTHREAD_ */
/*   if((status = pthread_once(&key_once, init_keys_once))) */
/*     do_exit("Failed pthread_once", status); */
/* #endif */
  // install outermost exception handler
  if (_set_top_handler() != NULL) return 1;
  // set standard file descriptors
  Cyc_stdin->file  = stdin;
  Cyc_stdout->file = stdout;
  Cyc_stderr->file = stderr;
  // convert command-line args to Cyclone strings -- we add an extra
  // NULL to the end of the argv so that people can step through argv
  // until they hit NULL. (Calling calloc with argc+1 takes care of this.)
  {struct _fat_argv args;
  int i, result;
  void *alloc = GC_calloc(argc+1,sizeof(struct _fat_ptr));
  args.curr = (struct _fat_ptr *) alloc;
  args.base = args.curr;
  args.last_plus_one = args.curr + argc + 1;
  for(i = 0; i < argc; ++i)
    args.curr[i] = Cstring_to_string(argv[i]);
  result = Cyc_main(argc, args);
  _fini_regions();
#if HAVE_THREADS
  Cyc_Core_main_join();
#endif
  GC_free(alloc);
  print_stats();
  return result;
  }
}

#ifdef HAVE_THREADS
int Cyc_Core_tid(){ 
  return tid(); 
}

void Cyc_Core_print_xstring( const char *s){
  if( s!= NULL ) printf("%s",s);
}

void Cyc_Core_main_join() {
      while(all_done != 0 )
         futex_wait(&all_done,all_done);
}

void Cyc_Core_set_stack(unsigned sz){
   stack_space = sz;
}

 #define _POSIX_C_SOURCE 199309 
 #include <time.h>
void Cyc_Core_nsleep (long nsec)
{
  struct timespec sleepTime;
  struct timespec remainingSleepTime;
  sleepTime.tv_sec = 0;
  sleepTime.tv_nsec = nsec;
  while (nanosleep(&sleepTime, &remainingSleepTime) != 0)
    sleepTime = remainingSleepTime;
}

void Cyc_Core_flush() {
 fflush(stdout);
}

const unsigned int Cyc_Core_stack_min = PTHREAD_STACK_MIN;

#endif
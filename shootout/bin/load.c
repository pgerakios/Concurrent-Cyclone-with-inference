#define _GNU_SOURCE
#include <stdio.h>
int do_it(char *f, char *argv[],int c_factor,char **ret);

int main(int argc, char *argv[])
{
   if(argc<2) {
      fprintf(stderr,"Need at least one argument\n");
      exit(-1);
   }
   char *c_b,*cyc_b,*orig;
   asprintf(&orig,"%s", argv[1]);
   int c_el = do_it(argv[1],argv+1,-1.00,&c_b);
   asprintf(&argv[1],"%s_cyc.cyco",argv[1]);
   int cyc_el = do_it(argv[1],argv+1,c_el,&cyc_b);
   printf("\%\n\\benchmark{%s}\n%s\n%s",orig,c_b,cyc_b);
   return 0;
}

/*void print_header() {
 printf("\\begin{table}[t]\n"
        "\\centering\\small%\n%"
        "\\def\\arraystretch{1.1}%\n"
        "\\def\\benchmark\#1{\\hline\\multicolumn{9}{l}{%\n"
        "\\raisebox{0pt}[9pt][4pt]{Benchmark:}\\ \\ \\textbf{#1}} \\\\}%\n"
        "\\begin{tabular}{lrrr@{\\ }r@{\\ }r@{\\ }rrr} \\hline\n"
        "lang & \\begin{tabular}{@{}c@{}} CPU \\\\ (s)  \\end{tabular}\n"
        "&\\begin{tabular}{@{}c@{}} memory \\\\ (KB) \\end{tabular}\n"
        "& \\multicolumn{4}{c}{%\n"
        "\\begin{tabular}{@{}c@{}} load per core \\\\ (\\%) \\end{tabular}\n"
        "}\n"
        "&\\begin{tabular}{@{}c@{}} elapsed \\\\ (s)  \\end{tabular}\n"
        "& factor \\\\"
       );
}

void print_footer() {
   printf("\\end{tabular}\n"
          "\\caption{Performance overhead, compared to GCC,"
          "for benchmarks taken from"
          " ``The Computer Language Benchmarks Game.''%"
          "\\label{tab:benchmarks}}"
          "\\end{table}%");
}*/

#include <stdlib.h> 
#include <pthread.h> 
#include <string.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h> 
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>
#ifndef HZ
# define HZ sysconf(_SC_CLK_TCK)
#endif

struct T { 
    pthread_mutex_t m;
    pid_t           p;
    int            total[5]; 
    int            avg_mem; //kb
    int            max_mem; //kb
};

void nsleep (long nsec)
{
  struct timespec sleepTime;
  struct timespec remainingSleepTime;
  sleepTime.tv_sec = 0;
  sleepTime.tv_nsec = nsec;
  while (nanosleep(&sleepTime, &remainingSleepTime) != 0)
   sleepTime = remainingSleepTime;
}

typedef enum {  USLEEP=0 , RUNNING=1 , ISLEEP=2  , STOPPED=3 , PAGING=4 , DEAD=5 , DEFUNCT=6 } pstate_t;

typedef struct {
  int ppid;
  unsigned long mem_kbyte;
  unsigned long  cputime;                                           
  pstate_t state;
} pinfo_t;

int process_info(pinfo_t *p, int pid) {
  static char buf[4096];
  int            stat_ppid = 0;
  char          *tmp = NULL;
  char           stat_item_state;
  long           stat_item_cutime = 0;
  long           stat_item_cstime = 0;
  long           stat_item_rss = 0;
  unsigned long  stat_item_utime = 0;
  unsigned long  stat_item_stime = 0;
  int page_shift_to_kb;
  FILE *f; 
  int len=0;
  long  page_size;
  int   page_shift;  
  static char fname[256];

     sprintf(fname,"/proc/%d/stat" , pid);
     f = fopen( fname,"r");
     if( f == NULL )
	  {
		 return -1;
	  }

     int c,i;
     for(c=fgetc(f),i=0;c!=EOF;c=fgetc(f),i++) buf[i] = c;
     fclose(f);
     len = i;
     if( len == 0 ){
       return -1;
     }
    
    /* Move along the buffer to get past the process name */
    if(!(tmp = strrchr(buf, ')')))
    {
        return -1;
    }

    tmp += 2;

    /* This implementation is done by using fs/procfs/array.c as a basis
     * it is also worth looking into the source of the procps utils */
    if(sscanf(tmp,
         "%c %d %*d %*d %*d %*d %*u %*u"
         "%*u %*u %*u %lu %lu %ld %ld %*d %*d %*d "
         "%*d %*u %*u %ld %*u %*u %*u %*u %*u "
         "%*u %*u %*u %*u %*u %*u %*u %*u %*d %*d\n",
         &stat_item_state,
         &stat_ppid,
         &stat_item_utime,
         &stat_item_stime,
         &stat_item_cutime,
         &stat_item_cstime,
         &stat_item_rss) != 7)
    { 
        return -1;
    }
    
    /* abs to please the compiler... we dont want to shift negatively.
     * why doesn't C understand this??? */
    p->ppid = stat_ppid;
  
    /* jiffies -> seconds = 1 / HZ
     * HZ is defined in "asm/param.h"  and it is usually 1/100s but on
     * alpha system it is 1/1024s */
    p->cputime =  ((float)(stat_item_utime + stat_item_stime) /** 10.0*/) / HZ;

 
   if((page_size = sysconf(_SC_PAGESIZE)) <= 0)
	{ 
		return -1;
	}
   for(page_shift=0; page_size!=1; page_size>>=1, page_shift++);
   page_shift_to_kb = page_shift-10;
   if(page_shift_to_kb < 0)
   {
      p->mem_kbyte = (stat_item_rss >> abs(page_shift_to_kb));
   }
   else
   {
      p->mem_kbyte = (stat_item_rss << abs(page_shift_to_kb));
   }

   switch((char) stat_item_state){
     case 'D': p->state = USLEEP; break; 
     case 'R': p->state = RUNNING; break; 
     case 'S': p->state = ISLEEP; break;
     case 'T': p->state = STOPPED; break;
     case 'W': p->state = PAGING; break; 
     case 'X': p->state = DEAD; break; 
     case 'Z': p->state = DEFUNCT ; break;
   }
  return 0; 
}

void *thread(void *p) {
   char b[4096]; 
   struct T *t = (struct T*) p;
   int i;
   int total[5] = {0,0,0,0,0};
   int prev_idle[5] = {0,0,0,0,0};
   int prev_total[5] = {0,0,0,0,0};
   int diff_idle[5] = {0,0,0,0,0};
   int diff_total[5] = {0,0,0,0,0};
   int diff_usage[5] = {0,0,0,0,0};
   int rounds = 0,rounds0=0;

   pthread_mutex_lock(&t->m); 
//   fprintf(stderr,"PID : %d\n",t->p);
   while(t->p) {
      FILE *f = fopen("/proc/stat","r");
      if(!f) exit(-1);
/*      int c,i;
      for(c=fgetc(f),i=0; c != EOF ; c=fgetc(f),i++)
         b[i] = c;*/
      int nums[5][9];
      int cnt=4;
      for(cnt=0; cnt < 5 ; cnt++) {
          fscanf(f,"%s %d %d %d %d %d %d %d %d %d\n",
                  b,&nums[cnt][0],&nums[cnt][1],&nums[cnt][2],
                  &nums[cnt][3],&nums[cnt][4],&nums[cnt][5],
                  &nums[cnt][6],&nums[cnt][7],&nums[cnt][8]);

         total[cnt] = 0;
         for(i = 0; i < 9 ;i++ ){
  //          printf("%d ", nums[cnt][i]);
            total[cnt] += nums[cnt][i]; 
         }
//         printf("\n");
         int idle = nums[cnt][3];
//         printf("\nidle=%d\n",idle);
         diff_idle[cnt] = idle - prev_idle[cnt];
         diff_total[cnt] = total[cnt] - prev_total[cnt];
 //        printf("\ndiff total: %d diff idle: %d\n" ,diff_total[cnt],
   //               diff_idle[cnt]);
         if(diff_total[cnt])               
            diff_usage[cnt] = ((100*(diff_total[cnt] - diff_idle[cnt]))/
                           diff_total[cnt]);
         else
            diff_usage[cnt] = 0;

         prev_total[cnt] = total[cnt];
         prev_idle[cnt] = idle;
         assert(diff_usage[cnt] < 101);
         t->total[cnt] += diff_usage[cnt]; 
      };

  /*     printf("%d %d %d %d %d\n",
               diff_usage[0], 
               diff_usage[1], 
               diff_usage[2], 
               diff_usage[3], 
               diff_usage[4]
             );
         fflush(stdout);*/
      fclose(f);
      rounds++;
      
       pinfo_t pi;
       if(!process_info(&pi,t->p)){
         if(pi.mem_kbyte > t->max_mem)
         t->max_mem = pi.mem_kbyte;
         t->avg_mem += pi.mem_kbyte;
         rounds0++;
      }
      nsleep(1000000 * 50); // 100ms
 //     sleep(1);
/*      i += scanf("%s %d %d %d %d %d %d %d %d %d\n",
                  NULL,
      for(i=0; b[i] != '\n' ;i++); i++;
      i = skip(i,4);
      printf("%s\n",b+i);*/
//      break;
   }
//   fprintf(stderr,"Rounds : %d\n",rounds);
   assert(rounds);
  assert(rounds0);
   for(i =0 ; i < 5 ; i++ ){
      t->total[i] /= (rounds);
      assert(t->total[i] < 101);
   }
   t->avg_mem /= rounds0;
   return 0;
}

#include <locale.h>
char *commaprint(unsigned long n)
{
   static int comma = '\0';
   static char retbuf[30];
   char *p = &retbuf[sizeof(retbuf)-1];
   int i = 0;

                  if(comma == '\0') {
                           struct lconv *lcp = localeconv();
                                 if(lcp != NULL) {
                                             if(lcp->thousands_sep != NULL &&
                                                               *lcp->thousands_sep != '\0')
                                                            comma = *lcp->thousands_sep;
                                                      else  comma = ',';
                                                            }
                                    }

                     *p = '\0';

                        do {
                                 if(i%3 == 0 && i != 0)
                                             *--p = comma;
                                       *--p = '0' + n % 10;
                                             n /= 10;
                                                   i++;
                                                      } while(n != 0);

                           return p;
}


int do_it(char *f, char *argv[], int c_factor,char **ret) {
   int pid;
   struct T t;
   memset(&t,0,sizeof(struct T));
   pthread_mutex_lock(&t.m);
   pthread_t tid;
   if(pthread_create(&tid,0,thread,&t)){
      exit(-1);
   }

   struct timeval t0,t1;
   gettimeofday(&t0,0);
   if ((t.p=pid = fork()) == 0){
       //close(STDIN_FILENO);
       close(STDOUT_FILENO);
       close(STDERR_FILENO);
      execvp(f,argv);
   } else {
      struct rusage r1, r2;
      int status;
      pthread_mutex_unlock(&t.m);
      getrusage (RUSAGE_CHILDREN, &r1);
      waitpid (pid,&status, 0);
      getrusage (RUSAGE_CHILDREN, &r2);
      gettimeofday(&t1,0);
      t.p = 0;
      pthread_join(tid,0);

      double el =  (double)t1.tv_sec - (double)t0.tv_sec +
                   (double)t1.tv_usec/(double)1000000 - 
                   (double)t0.tv_usec/(double)1000000;
      double v1 =  (double)r2.ru_utime.tv_sec - 
                   (double)r1.ru_utime.tv_sec +
                   (double)r2.ru_utime.tv_usec/(double)1000000 -
                   (double)r1.ru_utime.tv_usec/(double)1000000;
      double v2 =  (double)r2.ru_stime.tv_sec - 
                   (double)r1.ru_stime.tv_sec +
                   (double)r2.ru_stime.tv_usec/(double)1000000 -
                   (double)r1.ru_stime.tv_usec/(double)1000000;

      char *lang  = strstr(f,"cyc")?"cyc":"gcc"; 
/*      char bench[256] = "0";
      int i;
      for(i=0;f[i] != '_' && f[i] != '.' && f[i] != '\0';i++);
      strncpy(bench,f,i);*/


      /*printf( "%s%s & %2.2f \$\\pm\$ %2.2f"
             " & %2.2f \$\\pm\$ %2.2f &"
             " %2.2f%s & %2.2f \$\\pm\$"
             " %2.2f & %2.2f%s \\\\\n",

             , $testname, $dagger, $baselineval, $baselinevar, $compval, $compvar, $compvalp, $per, $compvalnobc, $compvarnobc, $compvalnobcp, $per;
            )*/
      
      asprintf(ret,"%s & %.2lf & %s & %d & %d"
                   " & %d & %d & %.2lf & %.2lf \\\\",
                  lang,
                  v1+v2,
                  commaprint(t.max_mem),
                  t.total[1],
                  t.total[2],
                  t.total[3],
                  t.total[4],
                  el,
                  c_factor==-1.00?(double)1.0:el/c_factor
            );
      return el;
    }
}

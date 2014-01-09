#include <stdio.h>
#include <stdlib.h> 

void do_it(char *f, char *argv[]);
int main()
{
   do_it(argv[0],argv+1);
   return 0;
}

#include <sys/time.h>
#include <sys/resource.h>
struct T { 
    pthread_mutex_t m;
    pid_t           p;
    int            total[5]; 
};

void *thread(void *p) {
   struct T *t = (struct T*) p;
   pthread_mutex_lock(t->m); 

       int total[5] = {0,0,0,0,0};
      int prev_idle[5] = {0,0,0,0,0};
      int prev_total[5] = {0,0,0,0,0};
      int diff_idle[5] = {0,0,0,0,0};
      int diff_total[5] = {0,0,0,0,0};
      int diff_usage[5] = {0,0,0,0,0};
  
   int rounds = 0;
   while(t->p) {
      FILE *f = fopen("/proc/stat","r");
      if(!f) exit(-1);
/*      int c,i;
      for(c=fgetc(f),i=0; c != EOF ; c=fgetc(f),i++)
         b[i] = c;*/
      int nums[5][9];
      int cnt=4,i;
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
         diff_usage[cnt] = ((100*(diff_total[cnt] - diff_idle[cnt]))/
                           diff_total[cnt]);
         prev_total[cnt] = total[cnt];
         prev_idle[cnt] = idle; 
         t->total[cnt] += diff_usage[cnt]; 
      };

       printf("\r                 \r%d %d %d %d %d",
               diff_usage[0], 
               diff_usage[1], 
               diff_usage[2], 
               diff_usage[3], 
               diff_usage[4]
             );
         fflush(stdout);
      fclose(f);
      rounds++;
      sleep(1);
/*      i += scanf("%s %d %d %d %d %d %d %d %d %d\n",
                  NULL,
      for(i=0; b[i] != '\n' ;i++); i++;
      i = skip(i,4);
      printf("%s\n",b+i);*/
//      break;
   }  
   for(i =0 ; i < 5 ; i++ ){
       t->total[i] /= rounds;
   }
   return 0;
}
void do_it(char *f, char *argv[]) {
   int pid;
   struct T t;
   memset(&t,0,sizeof(struct T));
   pthread_mutex_lock(&t.m);
   pthread_create(&tid,0,thread,&t); 
   if ((pid = fork()) == 0){
      execvp(f,argv);
   } else {
      getrusage (RUSAGE_CHILDREN, &r1);
      pthread_mutex_unlock(&m);
      pthread_t tid;
      waitpid (pid, WIFEXITED, 0);
      getrusage (RUSAGE_CHILDREN, &r2);
      pthread_join(&tid,0);
      t->p = 0;
      int v1 =  r2.ru_utime.tv_sec - r1.ru_utime.tv_sec +
                r2.ru_utime.tv_usec/1000000 - r1.ru_utime.tv_usec/1000000;
      int v2 =  r2.ru_stime.tv_sec - r1.ru_stime.tv_sec +
                r2.ru_stime.tv_usec/1000000 - r1.ru_stime.tv_usec/1000000;
      printf("%d %d %d %d %d %d %d\n", v1+v2,v1,v2,
                  t.total[1],
                  t.total[2],
                  t.total[3],
                  t.total[4]);
    }
}




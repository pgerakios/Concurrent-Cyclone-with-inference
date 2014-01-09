/* The Computer Language Benchmarks Game
   http://shootout.alioth.debian.org/
*/

#include <stdio.h>
#include <pthread.h> 
#include <stdlib.h>

#define waitValue(x,y) futex_wait(x,y)
#define signalValue(x,y) wake_single(x)

#define      THREADS  ((unsigned int)8)

typedef struct {
  unsigned int times;
  unsigned int width_bytes;
  unsigned int id;
  pthread_cond_t *signal;
  pthread_cond_t *peersignal;
} info;

/* begin global data */
pthread_cond_t signals[THREADS];
pthread_mutex_t lock;
FILE *output = 0;
/* end global data */
int next_id = 0;

void *thread(void *param0)
{
  info *param = (info *) param0;
  unsigned int times = param->times;
  unsigned int width_bytes = param->width_bytes;
  unsigned int id = param->id;
  //fprintf(stderr,"\nSTARTED THREAD id=%d \n",id);

  const double inverse_n = 2.0 / (double) times;
  const int r = times % THREADS;
  const int l = times / THREADS;
  const int myN = l + (id < r);
  int gN = myN;
  int lN = myN*width_bytes;
  unsigned int *lengths = (unsigned int*)malloc(gN*sizeof(unsigned int));
  unsigned char *data = (unsigned char *)malloc(lN*sizeof(unsigned char));
  unsigned int y,tmp;
  int i;
  for (i=0; i<gN; i++) lengths[i]=0;
  for (i=0; i<lN; i++) data[i]=0;
  unsigned int byte_count  = 0;
  int bit_num     = 0;
  int byte_acc    = 0;
  double Civ;
  double Crv;
  double Zrv;
  double Ziv;
  double Trv;
  double Tiv;
  unsigned int offset;
  int x;
  for(  y = 0 ; y < gN ; y++ ) {
    byte_count = 0;
    bit_num = 0;
    byte_acc = 0;
    unsigned int yy = y + id * l + (id < r ? id : r);
    Civ = yy * inverse_n - 1.0;
    offset = y * width_bytes;
    for (x = 0; x < times ; x++) {
      Crv = x * inverse_n - 1.5;
      Zrv   = Crv;
      Ziv   = Civ;
      Trv   = Crv * Crv;
      Tiv   = Civ * Civ;
      i = 49;
      do {
        Ziv = (Zrv*Ziv) + (Zrv*Ziv) + Civ;
        Zrv = Trv - Tiv + Crv;
        Trv = Zrv * Zrv;
        Tiv = Ziv * Ziv;
      } while ( ((Trv + Tiv) <= 4.0) && (--i > 0) );

      byte_acc <<= 1;
      byte_acc |= (i == 0) ? 1 : 0;

      if (++bit_num == 8)
        {
          tmp = offset + byte_count;
          if (tmp<lN)
            data[tmp] = (unsigned char) byte_acc;
          byte_count++;
          bit_num = byte_acc = 0;
        }
    } // end foreach (column)
    if (bit_num != 0) { // write left over bits
      byte_acc <<= (8 - (gN & 7));
      tmp = offset + byte_count;
      if( tmp < lN )
        data[tmp] = (unsigned char) byte_acc;
      byte_count++;
    }
    lengths[y] = byte_count;
  }

  unsigned char *ft = data; 
  pthread_cond_t*signal = param->signal;
  pthread_cond_t*peersignal = param->peersignal;

    pthread_mutex_lock(&lock);
    int should_wait = next_id != id;

    if(should_wait) {     
      //fprintf(stderr,"\nTHREAD id=%d wait signal=%d BEFORE WAIT\n",id, (int) (signal - &signals[0]));
      pthread_cond_wait(signal,&lock);
    }

    for( y = 0, tmp = 0 ; y < gN && tmp < lN ;
           y++,tmp+=width_bytes ) {
        fwrite(&ft[tmp], lengths[y], 1, output);
    }
   next_id++;
   pthread_mutex_unlock(&lock);
   if(peersignal) {
      //fprintf(stderr,"\nTHREAD SIGNALING id=%d-->id=%d\n",id,
      //         (int) (peersignal - &signals[0]));
      pthread_cond_signal(peersignal);
   }
//   fprintf(stderr,"\nTHREAD id=%d DONE\n",id);
   
  if(!peersignal && output != stdout) fclose(output);
  free(param);
  free(lengths);
  free(data);
  return 0;
}

int main(int argc, char *argv[])
{
  if (argc != 2) exit(255);
  unsigned int width_bytes,n,i;

  n = atoi(argv[1]);
  if( n < 1 ) exit(255);
  width_bytes = n/8;
  if (width_bytes*8 < n)
    width_bytes += 1;
  output = stdout;
  fprintf(output,"P4\n%d %d\n", n, n);
   pthread_mutex_init (&lock, NULL);
  for (i=0; i<THREADS; i++)
    pthread_cond_init (&signals[i], NULL);
  
  for (i = 0 ; i < THREADS -1 ; i++) {
    info *param = (info *) calloc(1,sizeof(info));
    param->times = n;
    param->width_bytes = width_bytes;
    param->id = i;
    param->signal = &signals[i];
    param->peersignal = &signals[i+1];
    pthread_t tid;
    pthread_create(&tid, NULL, thread,param);
  }
  info *param0 = (info *) calloc(1,sizeof(info));
  param0->times = n;
  param0->width_bytes = width_bytes;
  param0->id = THREADS-1;
  param0->signal = &signals[THREADS-1];
  param0->peersignal = NULL;
  thread(param0);
  return 0;
}

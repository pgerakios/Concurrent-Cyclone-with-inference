#ifndef CYC_SPECTRAL_THREADS
#define CYC_SPECTRAL_THREADS 4
#endif

#include <pthread.h> 
 void barrier(int thread_nr) {
    static int count = 0;
    static pthread_cond_t condition = PTHREAD_COND_INITIALIZER;
    static pthread_mutex_t barrier_mutex = PTHREAD_MUTEX_INITIALIZER;
     pthread_mutex_lock(&barrier_mutex);
     count++;
     if (count == thread_nr) {
       pthread_cond_broadcast(&condition);
       count = 0;
     }
     else
       pthread_cond_wait(&condition, &barrier_mutex);
     pthread_mutex_unlock(&barrier_mutex);
 }


#include <stdlib.h>
#include <stdio.h>
#include <math.h>


/* parameter for evaluate functions */
struct Param
{
    double * u;          /* source vector */
    double * tmp;        /* temporary */
    double * v;          /* destination vector */

    unsigned N;       // /* source/destination vector length */
    unsigned N2;             /* = N/2 */

    int r_begin;        /* working range of each thread */
    int r_end;

    double * vv;
    double * vBv;	
};

/* Return: 1.0 / (i + j) * (i + j +1) / 2 + i + 1; */
static double
eval_A(int i, int j)
{
    /*
     * 1.0 / (i + j) * (i + j +1) / 2 + i + 1;
     * n * (n+1) is even number. Therefore, just (>> 1) for (/2)
     */
    int d = (((i+j) * (i+j+1)) >> 1) + i+1;

    return 1.0 / (double) d;
}

/* This function is called by many threads */
//READ U, WRITE TMP, 

// no read/write access for tmp, 
// read-only access for u,
// no read/write access for v 
static void eval_A_times_u(struct Param *p)
{
    /* alias of source vector */
    int          i;
    int          ie;

    double * u = p->u;
    double * tmp = p->tmp;        /* temporary */
    unsigned N = p->N;

    for (i = p->r_begin, ie = p->r_end; i < ie && i < N; i++)
    {
		 double sum = 0.0;
		 int j;
		 for( j = 0 ; j < N ; j++ )
			 sum += eval_A(i,j) * u[j];
      tmp[i] = sum; //tmp_i;
    }
}

// no read/write access for v, 
// read-only access for u,
// read-only access for tmp
static void
eval_At_times_u(struct Param *p)
{
    int          i;
    int          ie;
    double * tmp = p->tmp;        /* temporary */
    double * v = p->v;          /* destination vector */
    unsigned N = p->N;

    for (i = p->r_begin, ie = p->r_end; i < ie && i < N; i++)
    {
		  double sum = 0.0;
        int     j;
        for (j = 0; j <  N; j++) 
			   sum += eval_A(j,i) * tmp[j];
		 v[i] = sum; // v_i;
    }
}

static void
eval_AtA_times_u(struct Param *p,
					  int threads)
{
    eval_A_times_u(p); // u(R),tmp(-),v(-)
//    fprintf(stderr,"\n[%d] before first barrier", Core::tid());
	 barrier(threads);
  //  fprintf(stderr,"\n[%d] after first barrier", Core::tid());
    eval_At_times_u(p); // u(-),tmp(R),v(-)
	 barrier(threads);
//    fprintf(stderr,"\n[%d] after second barrier", Core::tid());
}

// u(R),tmp(-),v(R)
static void do_add(struct Param *p)
{
    double * u = p->u;
    double * v = p->v;          /* destination vector */
    unsigned N = p->N;

	 double tmp0 = 0.0,tmp1=0.0;
	 int i,ie; 

 	 // u(R),tmp(-),v(R)
    for (i = p->r_begin, ie = p->r_end; i < ie && i < N; i++) {
    	tmp0 += v[i] * v[i]; 
    	tmp1 += u[i] * v[i]; 
	 }
    *p->vv  += tmp0;
    *p->vBv += tmp1;
}

static void
compute(struct Param *p, 
	int tid, int threads)

{

    unsigned N = p->N;

   int chunk = N / threads;
   int begin = tid * chunk;
   int end = (tid < (threads -1)) ?
	            (begin + chunk) : N;

  p->r_begin = begin;
  p->r_end = end;

  struct Param *p2 = (struct Param *) malloc(sizeof(struct Param));
  *p2 = *p;
  p2->u = p->v;
  p2->v = p->u;

	
	 int ite;
   // fprintf(stderr,"\n[%d] new thread", Core::tid());
	 for (ite = 0; ite < 10; ite++)
	 {
//    	fprintf(stderr,"\n[%d] before first pass", Core::tid());
 		 eval_AtA_times_u(p,threads);
//    	fprintf(stderr,"\n[%d] first pass done", Core::tid());
		 eval_AtA_times_u(p2,threads);
//    	fprintf(stderr,"\n[%d] second pass done", Core::tid());
	 }

		  do_add(p);
//		 fprintf(stderr,"\n[%d] NEXT DONE", Core::tid());
//    	fprintf(stderr,"\n[%d] EXITING", Core::tid());
}

typedef struct {
   struct Param *p;
   int id;
   int threads;
} tparam; 

void *thread(void *p0){ 
   compute(((tparam *)p0)->p,
           ((tparam *)p0)->id,
          ((tparam *)p0)->threads);
   return 0;
}

#define 	THREADS  CYC_SPECTRAL_THREADS
int main(int argc, char *argv[]) {
  unsigned N = ((argc >= 2) ? atoi(argv[1]) : 2000);
    __attribute__((aligned(64))) double * u = 
       calloc(N,sizeof(double));
   int i;
   for(i=0;i<N;i++) u[i] = 1.0;
  __attribute__((aligned(64))) double * tmp = 
       calloc(N,sizeof(double));
  __attribute__((aligned(64))) double * v = 
       calloc(N,sizeof(double));
   for(i=0;i<N;i++){
       tmp[i] = 0.0;
       v[i] = 0.0;
   }
  
  double * vv  = calloc(1,sizeof(double));
  double * vBv  = calloc(1,sizeof(double));
  *vv = 0.0;
  *vBv = 0.0;

  struct Param  p1 =
			{
				.u = u,
				.tmp = tmp,
				.v = v,
				.N = N,
				.N2 = N/2,
				.r_begin = 0,
				.r_end = 0, 
				.vv = vv,
				.vBv = vBv,
			};

  pthread_t ids[THREADS];
  for( i = 0 ; i < THREADS ; i++) {
    struct Param *pp= (struct Param *) malloc(sizeof(struct Param));
    *pp = p1;
    tparam *param = (tparam *) calloc(1,sizeof(tparam));
    param->p = pp;
    param->id = i;
    param->threads=THREADS;
    pthread_create(&ids[i], NULL, thread,param);
  }
  for( i = 0 ; i < THREADS ; i++) 
     pthread_join(ids[i], NULL);
  double v1 = *vBv,v2 = *vv;
  double s = sqrt(v1/v2);
  printf("%.9f\n",s);
  return 0;
}

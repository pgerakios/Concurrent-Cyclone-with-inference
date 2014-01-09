#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>
#include <glib.h>
#include <tcl.h>
#include <pthread.h>

typedef struct MapBuf {
  const char * p;
  unsigned n;       // /* source/destination vector length */
} buf_t;

typedef buf_t (subst_t)(char *begin, int len);

extern void attach_region(void *handle,
                          void *ptr, int len,
                          void (*fun)(void*,int));

buf_t subst_exp(
        const char* s,
        int n,
        const char* regex,
        const char* subst)
{
    GError* err = 0;
    char* rv = NULL;
    GRegex* prog = NULL;

    /* Compile regex. */
    prog = g_regex_new(regex,
                       G_REGEX_CASELESS |
                       G_REGEX_RAW |
                       G_REGEX_NO_AUTO_CAPTURE |
                       G_REGEX_OPTIMIZE,
                       0,
                       &err);
    /* Substitute. */
   if(!err)
    rv = g_regex_replace_literal(prog, s,n, 0, subst, 0, &err);

    /* Clean up. */
    if (prog) {
        g_regex_unref(prog);
    }

       char *tmp = rv;
       int cnt  = 0;
       while(*tmp++ !='\0') cnt++;
    buf_t r =  {rv,cnt};
    return r;
}

#define copybuf(dst,i,sz,src,len) \
   do { \
      int tmp2 = i+len; \
      if( tmp2 > sz ){ \
         int tmp1 = tmp2; \
         int tmp0 = sz*2; \
         if(tmp0<tmp1) tmp0 = tmp1;\
         dst = realloc(dst,tmp0); \
         sz = tmp0;\
      } \
      memcpy(dst+i,src,len);\
      i = tmp2;\
   } while(0);

buf_t subst_fun(char *orig, int len, char *pat, subst_t f ){
    GError* err = 0;
    GMatchInfo *match_info=0;
    GRegex* prog = g_regex_new(pat,
                       G_REGEX_CASELESS |
                       G_REGEX_RAW |
                       G_REGEX_NO_AUTO_CAPTURE |
                       G_REGEX_OPTIMIZE,
                       0,
                       &err);
   
   int ret_len = len;
   char *end0=orig+len,*prev=orig;
   char *ret = (char *) malloc(ret_len);
   int  ret_next = 0;
   int s=0,e=0;

   if (err) {
      goto out;
   }

    g_regex_match_full (prog,orig,len, 0, 0, &match_info, &err);
    while (g_match_info_matches (match_info)){
        if(!g_match_info_fetch_pos(match_info,0,&s,&e)){
           exit(-1);
        }
        int tmp0 = e - s;
        buf_t b = f(orig+s,tmp0);
        tmp0 = (orig+s) - prev;
        copybuf(ret,ret_next,ret_len,prev,tmp0);
        prev = orig+e;
        if(b.n>0) 
           copybuf(ret,ret_next,ret_len,b.p,b.n);
        g_match_info_next(match_info, NULL);
    }
   if(prev < end0){
     int tmp0 = end0 - prev;
      copybuf(ret,ret_next,ret_len,prev,tmp0);
   }
   g_match_info_free(match_info);
out:   
   g_regex_unref(prog);
    buf_t r =  {ret,ret_next};
    return r;
}

/* Return the number of times the regular expression "regexp_cstr"
 * uniquely matches against the input string "s". */
static unsigned long
regcount(
         const char *p,int len, const char* regexp_cstr)
{
   Tcl_Obj* s = 
    s = Tcl_NewStringObj(p, len);
    Tcl_IncrRefCount(s);

    int regexec_rv = 0;
    int index = 0;
    int index_max = 0;
    unsigned long rv = 0;
    Tcl_Obj* regexp_cstr_obj = NULL;
    Tcl_RegExp regexp = NULL;
    struct Tcl_RegExpInfo info = {0};

    /* Get "regexp_cstr" as a Tcl string object. */
    regexp_cstr_obj = Tcl_NewStringObj(regexp_cstr, strlen(regexp_cstr));
    Tcl_IncrRefCount(regexp_cstr_obj);

    /* Compile the regular expression. */
    regexp = Tcl_GetRegExpFromObj(NULL, regexp_cstr_obj,
                 TCL_REG_ADVANCED | TCL_REG_NOCASE | TCL_REG_NEWLINE);
    if (!regexp) {
        fprintf(stderr, "*** Error: Tcl_GetRegExpFromObj: failed");
        exit(1);
    }

    /* Iterate over each match. */
    index = 0;
    index_max = Tcl_GetCharLength(s);
    while (index < index_max) {

        /* Test for a match. */
        regexec_rv = Tcl_RegExpExecObj(NULL, regexp, s, index, 1, 0);
        if (regexec_rv == -1) {
            fprintf(stderr, "*** Error: Tcl_RegExpExecObj: failed");
            exit(1);
        }
        if (regexec_rv == 0) {
            /* No matches. */
            break;
        }

        /* Get the match information. */
        Tcl_RegExpGetInfo(regexp, &info);

        /* Advance curr. */
        index += info.matches[0].end;

        /* Increment the match count. */
        ++rv;
    }

    /* Clean up.  Note that "regexp" is owned by "regexp_cstr_obj" so
     * it does not need explicit clean up. */
    Tcl_DecrRefCount(regexp_cstr_obj);
    Tcl_DecrRefCount(s);

    return rv;
}

volatile int gtid = 1;
 __thread int mytid = 0;
int count_matches(char *text, int len, char *regex) {
    return regcount(text,len,regex);
}

#define cs(a,b,c) case a: return (buf_t){b,c}; 
buf_t sub(char *p, int n) {
  if( n < 1 ) exit(-1); 
  switch(p[0]) {
    cs('B',"(c|g|t)",7);
    cs('D',"(a|g|t)",7);
    cs('H',"(a|c|t)",7);
    cs('V',"(a|c|g)",7);
    cs('N',"(a|c|g|t)",9);
    cs('K',"(g|t)",5);
    cs('M',"(a|c)",5);
    cs('R',"(a|g)",5);
    cs('S',"(c|g)",5);
    cs('W',"(a|t)",5);
    cs('Y',"(c|t)",5);
  default:
    exit(-1);
  }
}

typedef struct Task {
  buf_t mb;
  int *counts;
} task_t;

typedef struct { task_t t; unsigned i; } info_t;

void* compute(void *ptr) {
  task_t t = ((info_t *) ptr)->t;
  unsigned i = ((info_t *) ptr)->i;

  int k0=0,k1=0;
  char * p1 = t.mb.p;
  unsigned n1 = t.mb.n;
  const char * pat[] = {
    "[cgt]gggtaaa|tttaccc[acg]",
    "a[act]ggtaaa|tttacc[agt]t",
    "ag[act]gtaaa|tttac[agt]ct",
    "agg[act]taaa|ttta[agt]cct",
    "aggg[acg]aaa|ttt[cgt]ccct",
    "agggt[cgt]aa|tt[acg]accct",
    "agggta[cgt]a|t[acg]taccct",
    "agggtaa[cgt]|[acg]ttaccct"
  }; 
  
  unsigned a = i*2; 
  unsigned b = a+1;
  k0 = count_matches(p1, n1, pat[a]);
  k1 = count_matches(p1, n1, pat[b]);
  t.counts[a]= k0;
  t.counts[b]= k1;
  return NULL;
}

buf_t map_file (FILE *f)
{
  long long len = 0;
  char *p = 0;
  fseek(f, 0, SEEK_END); // seek to end of file
  len = ftell(f); // get current file pointer
  fseek(f, 0, SEEK_SET); // seek back to beginning of file
  p = (char *) malloc((len+1));
  if(fread(p,1,len,f)!=len){
    exit(-1);
  }
  p[len] = '\0';
  return (buf_t){p, len};
}

int main(int argc, const char *argv[])
{
  int i;

  buf_t mb0 = map_file(stdin); /* read file to memory*/
  char *p = mb0.p;
  unsigned n = mb0.n;

  buf_t mb = subst_exp(p, n, "(>.*\\n)|\\n",""); // perform substitution 
  int *counts = calloc(9, sizeof(int));

#define THREADS 4
  pthread_t cthread[THREADS];
  for (i=0; i<THREADS; i++) {
    info_t *ii = malloc(sizeof(info_t));
    ii->t.mb = mb;
    ii->t.counts = counts;
    ii->i = i;
    pthread_create(&(cthread[i]), NULL, compute, ii);
  }
  
  char *p1 = mb.p;
  unsigned n1 = mb.n;

  const char pat0[] = "agggtaaa|tttaccct";
  int n00 = count_matches(p1,n1,pat0);

  buf_t mbt = subst_fun(p1,n1,"[WYKMSRBDVHN]",sub);
  unsigned n2 = mbt.n;

  for (i = 0; i < THREADS; i++) 
    pthread_join(cthread[i], NULL);

  printf(	   "agggtaaa|tttaccct %d\n"
		   "[cgt]gggtaaa|tttaccc[acg] %d\n"
		   "a[act]ggtaaa|tttacc[agt]t %d\n"
		   "ag[act]gtaaa|tttac[agt]ct %d\n"
		   "agg[act]taaa|ttta[agt]cct %d\n"
		   "aggg[acg]aaa|ttt[cgt]ccct %d\n"
		   "agggt[cgt]aa|tt[acg]accct %d\n"
		   "agggta[cgt]a|t[acg]taccct %d\n"
		   "agggtaa[cgt]|[acg]ttaccct %d\n"
		  	"\n%d\n%d\n%d\n",
				 n00,
				 counts[0],
				 counts[1],
				 counts[2],
				 counts[3],
				 counts[4],
				 counts[5],
				 counts[6],
				 counts[7],
		  		n,n1,n2); 
  return 0;
}

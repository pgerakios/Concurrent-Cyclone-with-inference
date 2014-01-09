#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#define safe_free free
#define bool int
#define false 0
#define true 1

void column(int i, char *orig, int *s, int *e) {
   char *tmp=orig,*tmp1;
   for(;i>0;i--,tmp=tmp1+1) 
     if(!(tmp1=strchr(tmp,','))) break;
   *s = tmp - orig;
   if(!(tmp=strchr(tmp,','))){
      tmp = tmp1+1;
      while(*tmp != '\0') tmp++;
   }
   *e = tmp - orig;
}

void print_col(char *orig , int s , int e){
   for(; s < e; s++)
      printf("%c" , orig[s]);
}

void swap(char *orig, int s , int e ){
   char *tmp1,*tmp=orig;
  for(; (tmp1=strchr(tmp,'%')) && tmp1 < orig+e;tmp1++){
      *tmp1 = '&';
  }
}

double print_line(int print, char *line , double cfact){ 
        int s,e;
        double ret = 0.0;
        if(print) {
           column(0,line,&s,&e);
           printf("\%\n\\benchmark{");
           print_col(line,s,e);
           printf("}\n");
         }
           printf(" & ");
           
           column(1,line,&s,&e);
           print_col(line,s,e);
           printf(" & ");

           column(5,line,&s,&e);
           print_col(line,s,e);
           printf(" & ");

           column(6,line,&s,&e);
           print_col(line,s,e);
           printf(" & ");

           column(8,line,&s,&e);
           swap(line,s,e);
           print_col(line,s,e);

           //printf(" & ");

           column(9,line,&s,&e);
           print_col(line,s,e);


           ret= atof(line+s);
           if(print) {
              printf("  & 1.00 ");
           } else {
              printf(" & %.2lf" , ret/cfact); 
           }

           printf("\\\\\n");
           fflush(stdout);
           return ret;
}

int main(int argc, char *argv[]) {

    FILE * fp;
    char * line = NULL,*line2=0;
     size_t len = 0;
     ssize_t read;
      if(argc !=2 )exit(-1);
     if(!(fp = fopen(argv[1], "r"))) exit(-1);
      
      system("cat bin/header.txt");
     int first =0;
     getline(&line, &len, fp);
        for (;(read = getline(&line, &len, fp)) != -1 && 
           (read = getline(&line2, &len, fp)) != -1
         ;first++) {
         if(strstr(line,"cyc")) {
            print_line(0,line,print_line(1,line2,1.0));
         } else {
            print_line(0,line2,print_line(1,line,1.0));
         }
   }
   system("cat bin/footer.txt");
    return 0;
}

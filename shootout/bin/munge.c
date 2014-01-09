 #include <stdio.h> 
 #include <stdlib.h> 
 #include <string.h>
 char buff[4096*2];
 int readLine(FILE *f){
	int c;
	int i = 0;
	while( (c=fgetc(f)) != EOF && c != '\n' ) buff[i++] = (char) c;
	buff[i] = '\0';
	return c != EOF;
 }
 int main( int argc , char *argv[] ){
	 FILE *f = stdin;
	 if( argc == 2 )  f= fopen(argv[1],"r");
	 int count = 0;
	 char *tmp;
	 while( readLine(f) ){
	   tmp = strstr(buff,"VAR ");
		if( tmp != NULL ){
			tmp += 4;
			if(count)
				printf("\n%s",tmp);
			else 
				printf("%s",tmp);
			count++;
		}
		if(buff[0] >= '0' && buff[0] <= '9')
		 printf(" %s", buff);
	 }
//	 printf("\n");
	 return 0;
 }

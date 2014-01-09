
  #include <stdio.h>
  #include <stdlib.h> 
  char buf[1024];
  int main(int argc , char *argv[] )
  {
	 char *p,*p1=NULL,*p2=NULL;
	 if( argc != 2 ) return -1;
	 p = argv[1];
	 for( p = argv[1] + strlen(argv[1]) ; p >= argv[1] ; p-- )
	 {
		if( *p == '.' && p2 == NULL )
		{
			 *p = '\0';
			  p2 = p;
		}
		else if( *p == '/' && p2 != NULL )
		{
			*p = '\0';
			 p1 = p+1;
			 break;
		}
	 }
	 sprintf(buf,"./mkfile.sh %s %s",argv[1],p1);
	 printf("\n%s\n",buf);
	 system(buf);
	 return 0;
  }

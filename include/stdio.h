#ifndef _STDIO_H_
#define _STDIO_H_
#ifndef _size_t_def_
#define _size_t_def_
typedef unsigned int size_t;
#endif
#ifndef ___off_t_def_
#define ___off_t_def_
typedef long __off_t;
#endif
#ifndef _wint_t_def_
#define _wint_t_def_
typedef unsigned int wint_t;
#endif
#ifndef ___mbstate_t_def_
#define ___mbstate_t_def_
typedef struct {int __count;
		union {wint_t __wch;
		       char __wchb[4U];} __value;} __mbstate_t;
#endif
#ifndef __G_fpos_t_def_
#define __G_fpos_t_def_
typedef struct {__off_t __pos;
		__mbstate_t __state;} _G_fpos_t;
#endif
#ifndef _fpos_t_def_
#define _fpos_t_def_
typedef _G_fpos_t fpos_t;
#endif
#ifndef _IOFBF
#define _IOFBF 0
#endif
#ifndef _G_BUFSIZ
#define _G_BUFSIZ 8192
#endif
#ifndef TMP_MAX
#define TMP_MAX 238328
#endif
#ifndef _IONBF
#define _IONBF 2
#endif
#ifndef L_tmpnam
#define L_tmpnam 20
#endif
#ifndef L_ctermid
#define L_ctermid 9
#endif
#ifndef SEEK_CUR
#define SEEK_CUR 1
#endif
#ifndef SEEK_SET
#define SEEK_SET 0
#endif
#ifndef _IOLBF
#define _IOLBF 1
#endif
#ifndef FILENAME_MAX
#define FILENAME_MAX 4096
#endif
#ifndef BUFSIZ
#define BUFSIZ _IO_BUFSIZ
#endif
#ifndef EOF
#define EOF (-1)
#endif
#ifndef SEEK_END
#define SEEK_END 2
#endif
#ifndef P_tmpdir
#define P_tmpdir "/tmp"
#endif
#ifndef FOPEN_MAX
#define FOPEN_MAX 16
#endif
#ifndef _IO_BUFSIZ
#define _IO_BUFSIZ _G_BUFSIZ
#endif

  // NOTES:
  // NULL is not defined because it is a keyword in Cyclone.
  // va_list is not defined because Cyclone has safe varargs.

  extern struct __cycFILE;  
  // Be sure to use _def_ so we don't interfere with cycboot.h
#ifndef _FILE_def_
#define _FILE_def_
  typedef struct __cycFILE FILE;
#endif
  extern FILE @stdout;
  extern FILE @stdin;
  extern FILE @stderr;

  namespace Cstdio {
    extern "C" struct __abstractFILE;    // needed by tmp.cyc
  }

  // fromCfile needed by tmp.cyc
  extern FILE *`H fromCfile(struct Cstdio::__abstractFILE *`H cf);

  // vararg for printf functions.  The functions themselves are defined
  // in printf.cyc.
  extern datatype PrintArg<`r::R> {
    String_pa(const char ? @nozeroterm`r);
    Int_pa(unsigned long);
    Double_pa(double);
    LongDouble_pa(long double);
    ShortPtr_pa(short @`r);
    IntPtr_pa(unsigned long @`r);
  };
  // Be sure to use _def_ so we don't interfere with cycboot.h
#ifndef _parg_t_def_
#define _parg_t_def_
  typedef datatype PrintArg<`r> @`r parg_t<`r>;
#endif

  // Cyclone specific.
  // Similar to sprintf but allocates a result of the right size on the heap.
  extern char ?aprintf(const char ?, ... inject parg_t)
    __attribute__((format(printf,1,2)))
    ;

  extern void clearerr(FILE @);

  // FIX: not supported yet
  //  char *ctermid(char *);

  extern int fclose(FILE @);

  // FIX: second arg allowed to be NULL?
  extern FILE *fdopen(int, const char *);

  extern int feof(FILE @);

  extern int ferror(FILE @);

  extern int fflush(FILE *);

  extern int fgetc(FILE @);

  extern int fgetpos(FILE @, fpos_t @);

  extern char ? @nozeroterm`r fgets(char ? @nozeroterm`r, int, FILE @);

  extern int fileno(FILE @);

  #ifndef __CYGWIN__
  extern void flockfile(FILE @);
  #endif

  extern FILE *fopen(const char @, const char @);

  extern int fprintf(FILE @,const char ?, ... inject parg_t)
    __attribute__((format(printf,2,3)))
    ;

  extern int fputc(int, FILE @);

  extern int fputs(const char @, FILE @);

  extern size_t fread(char ? @nozeroterm, size_t, size_t, FILE @);

  extern FILE *`r freopen(const char *, const char @, FILE @`r);

  // vararg for scanf functions.  The functions themselves are defined
  // in scanf.cyc.
  extern datatype ScanfArg<`r::R> {
    ShortPtr_sa(short @`r);
    UShortPtr_sa(unsigned short @`r);
    IntPtr_sa(int @`r);
    UIntPtr_sa(unsigned int @`r);
    StringPtr_sa(char ?`r);
    DoublePtr_sa(double @`r);
    FloatPtr_sa(float @`r);
    CharPtr_sa(char ? @nozeroterm`r)
  };
  // Be sure to use _def_ so we don't interfere with cycboot.h
#ifndef _sarg_t_def_
#define _sarg_t_def_ 
  typedef datatype ScanfArg<`r1> @`r2 sarg_t<`r1,`r2>;
#endif

  extern int fscanf(FILE @, const char ?, ... inject sarg_t)
    __attribute__((format(scanf,2,3)))
    ;

  extern int fseek(FILE @, long, int);

  extern int fsetpos(FILE @, const fpos_t @);

  extern long ftell(FILE @);

  #ifndef __CYGWIN__
  extern int ftrylockfile(FILE @);
  #endif

  #ifndef __CYGWIN__
  extern void funlockfile(FILE @);
  #endif

  extern size_t fwrite(const char ? @nozeroterm, size_t, size_t, FILE @);

  extern int getc(FILE @);

  extern int getchar(void);

  #ifndef __CYGWIN__
  extern int getc_unlocked(FILE @);
  #endif

#ifndef __CYGWIN__
  extern int getchar_unlocked(void);
#endif

  // FIX: we don't support gets because the C version is completely
  // unsafe.  We could support it safely by writing it in Cyclone.
  //  extern char *gets(char *);

  extern int pclose(FILE @);

  // FIX: worry about char * being NULL?
  extern "C" void perror(const char *);

  extern FILE *popen(const char @, const char @);

  extern int printf(const char ?, ... inject parg_t)
#ifdef XRGN
	 @nothrow
	 @re_entrant
#endif
    __attribute__((format(printf,1,2)))
    ;

  extern int putc(int, FILE @);

  extern "C" int putchar(int);

  #ifndef __CYGWIN__
  extern int putc_unlocked(int, FILE @);
  #endif

  extern "C" int putchar_unlocked(int);

  extern "C" int puts(const char @);

  extern "C" int remove(const char @);

  extern "C" int rename(const char @, const char @);

  extern void rewind(FILE @);

  // Cyclone specific.
  extern char ?`r rprintf(region_t<`r>, const char ?, ... inject parg_t)
    __attribute__((format(printf,2,3)))
    ;

  extern int scanf(const char ?, ... inject sarg_t)
    __attribute__((format(scanf,1,2)))
    ;

  // FIX: should the second arg be const?
  extern int setvbuf(FILE @, char ? @nozeroterm`H, int, size_t);
  // The second arg of setvbuf must be heap-allocated because
  // it will be used by the IO library after setvbuf returns.

  extern void setbuf(FILE @, char ? @nozeroterm`H);

  extern int snprintf(char ? @nozeroterm, size_t, const char ?, ... inject parg_t)
    __attribute__((format(printf,3,4)))
    ;

  extern int sprintf(char ? @nozeroterm, const char ?, ... inject parg_t)
    __attribute__((format(printf,2,3)))
    ;

  extern int sscanf(const char ?, const char ?, ... inject sarg_t)
    __attribute__((format(scanf,2,3)))
    ;

  // FIX: tempnam is not yet supported.  The C version does a heap
  // allocation (with malloc).  We need to write a C stub to free
  // this.
  // Note, tempnam is considered insecure because in between the call
  // of tempnam and open, a file can be created with the supposedly
  // fresh name.
  //  extern char *tempnam(const char *, const char *);

  extern FILE *tmpfile(void);

  // Note, tmpnam is considered insecure because in between the call
  // of tmpnam and open, a file can be created with the supposedly
  // fresh name.
  // Note, we require the argument of tmpnam to be heap-allocated.
  // This is because if tmpnam is called with NULL, it is supposed
  // to return a pointer to a static (heap allocated) buffer, while if
  // tmpnam is called with a non-NULL argument, it returns that
  // argument.  So the only way to type tmpnam is to require the arg
  // to be heap allocated.
  extern char ? tmpnam(char ?`H);

  extern int ungetc(int, FILE @);

  extern int vfprintf(FILE @, const char ?, parg_t ?)
    __attribute__((format(printf,2,0)))
    ;

  extern int vprintf(const char ?, parg_t ?)
    __attribute__((format(printf,1,0)))
    ;

  extern char ?`r vrprintf(region_t<`r>, const char ?, parg_t ?)
    __attribute__((format(printf,2,0)))
    ;

  extern int vsnprintf(char ? @nozeroterm, size_t, const char ?, parg_t ?) 
    __attribute__((format(printf,3,0)))
    ;

  extern int vsprintf(char ? @nozeroterm, const char ?, parg_t ?)
    __attribute__((format(printf,2,0)))
    ;

  // getw is a glibc function but does not appear in posix; remove ??
  extern int getw(FILE @);

  // putw is a glibc function but does not appear in posix; remove ??
  extern int putw(int, FILE @);

  // These functions appear in glibc but are not part of the posix
  // standard.  They are omitted for now.
  //  extern void setbuffer (FILE @, char ?, size_t);
  //  extern void setlinebuf (FILE @);

  // FIX: These functions are are not part of POSIX, they
  // are Cyclone-specific.  We should get rid of them or move
  // them elsewhere.

  // FIX: Not sure if these should use NULL-terminated strings or not
  extern datatype exn {
    extern FileOpenError(const char ?);
    extern FileCloseError;
  };
  extern FILE @file_open(const char ?, const char ?);
  extern void file_close(FILE @);

  // These two don't seem to be defined anywhere.
  //  extern void file_delete(const char ?);
  //  extern void file_length(const char ?);

  // these two provided in c stubs
  extern int file_string_read(FILE @, char ?dest, int dest_offset, 
                              int max_count);
  extern int file_string_write(FILE @, const char ?src, int src_offset, 
                               int max_count);
#endif

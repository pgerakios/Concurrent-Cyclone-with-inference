#ifndef _TAR_H_
#define _TAR_H_
#ifndef TMAGIC
#define TMAGIC "ustar"
#endif
#ifndef TVERSLEN
#define TVERSLEN 2
#endif
#ifndef BLKTYPE
#define BLKTYPE '4'
#endif
#ifndef LNKTYPE
#define LNKTYPE '1'
#endif
#ifndef TOWRITE
#define TOWRITE 00002
#endif
#ifndef TVERSION
#define TVERSION "00"
#endif
#ifndef DIRTYPE
#define DIRTYPE '5'
#endif
#ifndef AREGTYPE
#define AREGTYPE '\0'
#endif
#ifndef TOREAD
#define TOREAD 00004
#endif
#ifndef CONTTYPE
#define CONTTYPE '7'
#endif
#ifndef TGWRITE
#define TGWRITE 00020
#endif
#ifndef TUEXEC
#define TUEXEC 00100
#endif
#ifndef TMAGLEN
#define TMAGLEN 6
#endif
#ifndef TGEXEC
#define TGEXEC 00010
#endif
#ifndef FIFOTYPE
#define FIFOTYPE '6'
#endif
#ifndef SYMTYPE
#define SYMTYPE '2'
#endif
#ifndef REGTYPE
#define REGTYPE '0'
#endif
#ifndef TSUID
#define TSUID 04000
#endif
#ifndef CHRTYPE
#define CHRTYPE '3'
#endif
#ifndef TSGID
#define TSGID 02000
#endif
#ifndef TUWRITE
#define TUWRITE 00200
#endif
#ifndef TSVTX
#define TSVTX 01000
#endif
#ifndef TUREAD
#define TUREAD 00400
#endif
#ifndef TOEXEC
#define TOEXEC 00001
#endif
#ifndef TGREAD
#define TGREAD 00040
#endif
#endif

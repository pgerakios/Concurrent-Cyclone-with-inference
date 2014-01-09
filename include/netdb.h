#ifndef _NETDB_H_
#define _NETDB_H_
#ifndef _uint16_t_def_
#define _uint16_t_def_
typedef unsigned short uint16_t;
#endif
#ifndef _uint32_t_def_
#define _uint32_t_def_
typedef unsigned int uint32_t;
#endif
#ifndef ___socklen_t_def_
#define ___socklen_t_def_
typedef unsigned int __socklen_t;
#endif
#ifndef _socklen_t_def_
#define _socklen_t_def_
typedef __socklen_t socklen_t;
#endif
#ifndef _in_port_t_def_
#define _in_port_t_def_
typedef uint16_t in_port_t;
#endif
#ifndef ___anonymous_enum_38___def_
#define ___anonymous_enum_38___def_
enum __anonymous_enum_38__{
  IPPORT_ECHO = 7,
  IPPORT_DISCARD = 9,
  IPPORT_SYSTAT = 11,
  IPPORT_DAYTIME = 13,
  IPPORT_NETSTAT = 15,
  IPPORT_FTP = 21,
  IPPORT_TELNET = 23,
  IPPORT_SMTP = 25,
  IPPORT_TIMESERVER = 37,
  IPPORT_NAMESERVER = 42,
  IPPORT_WHOIS = 43,
  IPPORT_MTP = 57,
  IPPORT_TFTP = 69,
  IPPORT_RJE = 77,
  IPPORT_FINGER = 79,
  IPPORT_TTYLINK = 87,
  IPPORT_SUPDUP = 95,
  IPPORT_EXECSERVER = 512,
  IPPORT_LOGINSERVER = 513,
  IPPORT_CMDSERVER = 514,
  IPPORT_EFSSERVER = 520,
  IPPORT_BIFFUDP = 512,
  IPPORT_WHOSERVER = 513,
  IPPORT_ROUTESERVER = 520,
  IPPORT_RESERVED = 1024,
  IPPORT_USERRESERVED = 5000
};
#endif
#ifndef _in_addr_t_def_
#define _in_addr_t_def_
typedef uint32_t in_addr_t;
#endif
#ifndef _in_addr_def_
#define _in_addr_def_
struct in_addr{
  in_addr_t s_addr;
};
#endif
#ifndef _servent_def_
#define _servent_def_
struct servent{
  char * s_name;
  char *@zeroterm * s_aliases;
  int s_port;
  char * s_proto;
};
#endif
#ifndef _protoent_def_
#define _protoent_def_
struct protoent{
  char * p_name;
  char *@zeroterm * p_aliases;
  int p_proto;
};
#endif
#ifndef TRY_AGAIN
#define TRY_AGAIN 2
#endif
#ifndef NO_RECOVERY
#define NO_RECOVERY 3
#endif
#ifndef HOST_NOT_FOUND
#define HOST_NOT_FOUND 1
#endif
#ifndef IPPORT_RESERVED
#define IPPORT_RESERVED 1024
#endif
#ifndef NO_DATA
#define NO_DATA 4
#endif

  /* Not defined in OS X or cygwin so we guess :-( */
  #ifndef _in_port_t_def_
  #define _in_port_t_def_
  typedef unsigned short in_port_t;
  #endif

  // FIX:  Need to put in by hand so that we can translate
  // to struct in_addr, rather than the prescribed char *.
  // We thus bake in the fact that addresses are 4 bytes long; 
  // i.e. that the length is assumed to be 4.  If this is not
  // the case, then we are in trouble.
  struct hostent {
    char *h_name;
    char ** @zeroterm h_aliases;
#if defined(__CYGWIN32__) || defined(__CYGWIN__)
      short h_addrtype;
      short h_length; // always 4
#else
      int h_addrtype;
      int h_length;   // always 4
#endif
    struct in_addr **h_addr_list;
#define h_addr h_addr_list[0]
  };

  // FIX: might this be unsafe?  In particular, we might either end up
  //   with a memory leak, since this was probably malloced, or
  //   with some clobbered memory if it's using a static buffer.
  // FIX: should strings be non-NULL?
  extern "C" struct servent *getservbyname(const char * name, const char * proto);

  // FIX: see getservbyname comment
  extern "C" struct hostent *gethostbyname(const char * name);

  // FIX: see getservbyname comment
  extern "C" struct protoent * getprotobyname(const char * name);

  // FIX: should string be non-NULL
  extern "C" void herror(const char *);
#endif

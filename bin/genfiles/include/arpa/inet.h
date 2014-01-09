#ifndef _ARPA_INET_H_
#define _ARPA_INET_H_
#ifndef _uint16_t_def_
#define _uint16_t_def_
typedef unsigned short uint16_t;
#endif
#ifndef _uint32_t_def_
#define _uint32_t_def_
typedef unsigned int uint32_t;
#endif
#ifndef _in_port_t_def_
#define _in_port_t_def_
typedef uint16_t in_port_t;
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
#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16
#endif
#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 46
#endif

  /* Cygwin does not define these types so we take a guess :-( */
  /* FIX: make this specific to cygwin ?? */
  #ifndef _in_addr_t_def_
  #define _in_addr_t_def_
  typedef unsigned long in_addr_t;
  #endif

  /* Posix says these can be macros or functions.  Typically they are
     macros but in Linux at least they use asm, which we don't support
     in Cyclone.  So, we make them functions. */
  extern uint32_t htonl(uint32_t x);
  extern uint16_t htons(uint16_t x);
  extern uint32_t ntohl(uint32_t x);
  extern uint16_t ntohs(uint16_t x);

  // FIX: should I worry about Cstring being not-NULL?
  extern "C" in_addr_t inet_addr(const char *);

  /* Note, inet_aton is not required by POSIX */
  // FIX: should I worry about char * being not-NULL?
  extern "C" int inet_aton(const char *, struct in_addr @);

  extern "C" char * inet_ntoa(struct in_addr);

  // inet_ntop is not supported yet

  // inet_pton is not supported yet
#endif

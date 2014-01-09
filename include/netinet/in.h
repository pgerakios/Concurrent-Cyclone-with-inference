#ifndef _NETINET_IN_H_
#define _NETINET_IN_H_
#ifndef _uint8_t_def_
#define _uint8_t_def_
typedef unsigned char uint8_t;
#endif
#ifndef _uint16_t_def_
#define _uint16_t_def_
typedef unsigned short uint16_t;
#endif
#ifndef _uint32_t_def_
#define _uint32_t_def_
typedef unsigned int uint32_t;
#endif
#ifndef _sa_family_t_def_
#define _sa_family_t_def_
typedef unsigned short sa_family_t;
#endif
#ifndef _sockaddr_def_
#define _sockaddr_def_
struct sockaddr{
  sa_family_t sa_family;
  char sa_data[14U];
};
#endif
#ifndef ___anonymous_enum_42___def_
#define ___anonymous_enum_42___def_
enum __anonymous_enum_42__{
  IPPROTO_IP = 0,
  IPPROTO_HOPOPTS = 0,
  IPPROTO_ICMP = 1,
  IPPROTO_IGMP = 2,
  IPPROTO_IPIP = 4,
  IPPROTO_TCP = 6,
  IPPROTO_EGP = 8,
  IPPROTO_PUP = 12,
  IPPROTO_UDP = 17,
  IPPROTO_IDP = 22,
  IPPROTO_TP = 29,
  IPPROTO_IPV6 = 41,
  IPPROTO_ROUTING = 43,
  IPPROTO_FRAGMENT = 44,
  IPPROTO_RSVP = 46,
  IPPROTO_GRE = 47,
  IPPROTO_ESP = 50,
  IPPROTO_AH = 51,
  IPPROTO_ICMPV6 = 58,
  IPPROTO_NONE = 59,
  IPPROTO_DSTOPTS = 60,
  IPPROTO_MTP = 92,
  IPPROTO_ENCAP = 98,
  IPPROTO_PIM = 103,
  IPPROTO_COMP = 108,
  IPPROTO_SCTP = 132,
  IPPROTO_RAW = 255,
  IPPROTO_MAX = 256U
};
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
#ifndef _in6_addr_def_
#define _in6_addr_def_
struct in6_addr{
  union {uint8_t u6_addr8[16U];
	 uint16_t u6_addr16[8U];
	 uint32_t u6_addr32[4U];} in6_u;
};
#endif
#ifndef _in6addr_any_def_
#define _in6addr_any_def_
extern const struct in6_addr in6addr_any;
#endif
#ifndef _in6addr_loopback_def_
#define _in6addr_loopback_def_
extern const struct in6_addr in6addr_loopback;
#endif
#ifndef _sockaddr_in_def_
#define _sockaddr_in_def_
struct sockaddr_in{
  sa_family_t sin_family;
  in_port_t sin_port;
  struct in_addr sin_addr;
  unsigned char sin_zero[((sizeof(struct sockaddr) - sizeof(unsigned short)) - sizeof(in_port_t)) - sizeof(struct in_addr)];
};
#endif
#ifndef _sockaddr_in6_def_
#define _sockaddr_in6_def_
struct sockaddr_in6{
  sa_family_t sin6_family;
  in_port_t sin6_port;
  uint32_t sin6_flowinfo;
  struct in6_addr sin6_addr;
  uint32_t sin6_scope_id;
};
#endif
#ifndef _ipv6_mreq_def_
#define _ipv6_mreq_def_
struct ipv6_mreq{
  struct in6_addr ipv6mr_multiaddr;
  unsigned int ipv6mr_interface;
};
#endif
#ifndef IPPROTO_IP
#define IPPROTO_IP IPPROTO_IP
#endif
#ifndef IPV6_LEAVE_GROUP
#define IPV6_LEAVE_GROUP 21
#endif
#ifndef IN6ADDR_ANY_INIT
#define IN6ADDR_ANY_INIT { { { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 } } }
#endif
#ifndef IN6_IS_ADDR_UNSPECIFIED
#define IN6_IS_ADDR_UNSPECIFIED(a) (((__const uint32_t *) (a))[0] == 0 && ((__const uint32_t *) (a))[1] == 0 && ((__const uint32_t *) (a))[2] == 0 && ((__const uint32_t *) (a))[3] == 0)
#endif
#ifndef IN6_IS_ADDR_MULTICAST
#define IN6_IS_ADDR_MULTICAST(a) (((__const uint8_t *) (a))[0] == 0xff)
#endif
#ifndef IN6ADDR_LOOPBACK_INIT
#define IN6ADDR_LOOPBACK_INIT { { { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 } } }
#endif
#ifndef IPPROTO_TCP
#define IPPROTO_TCP IPPROTO_TCP
#endif
#ifndef IN6_IS_ADDR_MC_SITELOCAL
#define IN6_IS_ADDR_MC_SITELOCAL(a) (IN6_IS_ADDR_MULTICAST(a) && ((((__const uint8_t *) (a))[1] & 0xf) == 0x5))
#endif
#ifndef IPPROTO_IPV6
#define IPPROTO_IPV6 IPPROTO_IPV6
#endif
#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16
#endif
#ifndef IPV6_UNICAST_HOPS
#define IPV6_UNICAST_HOPS 16
#endif
#ifndef IPV6_V6ONLY
#define IPV6_V6ONLY 26
#endif
#ifndef IPV6_MULTICAST_IF
#define IPV6_MULTICAST_IF 17
#endif
#ifndef IN6_IS_ADDR_MC_ORGLOCAL
#define IN6_IS_ADDR_MC_ORGLOCAL(a) (IN6_IS_ADDR_MULTICAST(a) && ((((__const uint8_t *) (a))[1] & 0xf) == 0x8))
#endif
#ifndef INADDR_ANY
#define INADDR_ANY ((in_addr_t) 0x00000000)
#endif
#ifndef IPV6_MULTICAST_HOPS
#define IPV6_MULTICAST_HOPS 18
#endif
#ifndef IPPROTO_UDP
#define IPPROTO_UDP IPPROTO_UDP
#endif
#ifndef IN6_IS_ADDR_MC_NODELOCAL
#define IN6_IS_ADDR_MC_NODELOCAL(a) (IN6_IS_ADDR_MULTICAST(a) && ((((__const uint8_t *) (a))[1] & 0xf) == 0x1))
#endif
#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 46
#endif
#ifndef IPV6_JOIN_GROUP
#define IPV6_JOIN_GROUP 20
#endif
#ifndef IPPROTO_RAW
#define IPPROTO_RAW IPPROTO_RAW
#endif
#ifndef IPPROTO_ICMP
#define IPPROTO_ICMP IPPROTO_ICMP
#endif
#ifndef INADDR_NONE
#define INADDR_NONE ((in_addr_t) 0xffffffff)
#endif
#ifndef IN6_IS_ADDR_MC_LINKLOCAL
#define IN6_IS_ADDR_MC_LINKLOCAL(a) (IN6_IS_ADDR_MULTICAST(a) && ((((__const uint8_t *) (a))[1] & 0xf) == 0x2))
#endif
#ifndef IN6_IS_ADDR_MC_GLOBAL
#define IN6_IS_ADDR_MC_GLOBAL(a) (IN6_IS_ADDR_MULTICAST(a) && ((((__const uint8_t *) (a))[1] & 0xf) == 0xe))
#endif
#ifndef INADDR_LOOPBACK
#define INADDR_LOOPBACK ((in_addr_t) 0x7f000001)
#endif
#ifndef INADDR_BROADCAST
#define INADDR_BROADCAST ((in_addr_t) 0xffffffff)
#endif
#ifndef IPV6_MULTICAST_LOOP
#define IPV6_MULTICAST_LOOP 19
#endif

  /* We'd like to do
     include {
     IN6_IS_ADDR_LINKLOCAL
     IN6_IS_ADDR_LOOPBACK
     IN6_IS_ADDR_SITELOCAL
     IN6_IS_ADDR_V4COMPAT
     IN6_IS_ADDR_V4MAPPED
     }
     but these bring in declarations for htonl and ntohl, which
     conflict with the declarations below.
  */

  /* These aren't defined in OS X or Cygwin so we guess :-( */
  #ifndef _in_port_t_def_
  #define _in_port_t_def_
  typedef unsigned short in_port_t;
  #endif
  #ifndef _in_addr_t_def_
  #define _in_addr_t_def_
  typedef unsigned int in_addr_t;
  #endif
  /* Not defined in OS X */
  #ifndef _sa_family_t_def_
  #define _sa_family_t_def_
  typedef unsigned char sa_family_t;
  #endif

  extern uint32_t htonl(uint32_t x);

  extern uint16_t htons(uint16_t x);

  extern uint32_t ntohl(uint32_t x);

  extern uint16_t ntohs(uint16_t x);
#endif

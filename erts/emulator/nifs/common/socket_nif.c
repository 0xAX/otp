/*
 * %CopyrightBegin%
 *
 * Copyright Ericsson AB 2018-2018. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * %CopyrightEnd%
 *
 * ----------------------------------------------------------------------
 *  Purpose : The NIF (C) part of the socket interface
 * ----------------------------------------------------------------------
 *
 */

#define STATIC_ERLANG_NIF 1


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* If we HAVE_SCTP_H and Solaris, we need to define the following in
 * order to get SCTP working:
 */
#if (defined(HAVE_SCTP_H) && defined(__sun) && defined(__SVR4))
#define SOLARIS10    1
/* WARNING: This is not quite correct, it may also be Solaris 11! */
#define _XPG4_2
#define __EXTENSIONS__
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <ctype.h>
#include <sys/types.h>
#include <errno.h>
#include <time.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif

#ifdef HAVE_NET_IF_DL_H
#include <net/if_dl.h>
#endif

#ifdef HAVE_IFADDRS_H
#include <ifaddrs.h>
#endif

#ifdef HAVE_NETPACKET_PACKET_H
#include <netpacket/packet.h>
#endif

#ifdef HAVE_SYS_UN_H
#include <sys/un.h>
#endif

/* SENDFILE STUFF HERE IF WE NEED IT... */

#if defined(__APPLE__) && defined(__MACH__) && !defined(__DARWIN__)
#define __DARWIN__ 1
#endif


#ifdef __WIN32__
#define STRNCASECMP               strncasecmp
#define INCL_WINSOCK_API_TYPEDEFS 1

#ifndef WINDOWS_H_INCLUDES_WINSOCK2_H
#include <winsock2.h>
#endif
#include <windows.h>
#include <Ws2tcpip.h>   /* NEED VC 6.0 or higher */

/* Visual studio 2008+: NTDDI_VERSION needs to be set for iphlpapi.h
 * to define the right structures. It needs to be set to WINXP (or LONGHORN)
 * for IPV6 to work and it's set lower by default, so we need to change it.
 */
#ifdef HAVE_SDKDDKVER_H
#  include <sdkddkver.h>
#  ifdef NTDDI_VERSION
#    undef NTDDI_VERSION
#  endif
#  define NTDDI_VERSION NTDDI_WINXP
#endif
#include <iphlpapi.h>

#undef WANT_NONBLOCKING
#include "sys.h"




/* AND HERE WE MAY HAVE A BUNCH OF DEFINES....SEE INET DRIVER.... */




#else /* !__WIN32__ */

#include <sys/time.h>
#ifdef NETDB_H_NEEDS_IN_H
#include <netinet/in.h>
#endif
#include <netdb.h>

#include <sys/socket.h>
#include <netinet/in.h>

#ifdef DEF_INADDR_LOOPBACK_IN_RPC_TYPES_H
#include <rpc/types.h>
#endif

#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <arpa/inet.h>

#include <sys/param.h>
#ifdef HAVE_ARPA_NAMESER_H
#include <arpa/nameser.h>
#endif

#ifdef HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#include <net/if.h>

#ifdef HAVE_SCHED_H
#include <sched.h>
#endif

#ifdef HAVE_SETNS_H
#include <setns.h>
#endif

#define HAVE_UDP

/* SCTP support -- currently for UNIX platforms only: */
#undef HAVE_SCTP
#if defined(HAVE_SCTP_H)

#include <netinet/sctp.h>

/* SCTP Socket API Draft from version 11 on specifies that netinet/sctp.h must
   explicitly define HAVE_SCTP in case when SCTP is supported,  but Solaris 10
   still apparently uses Draft 10, and does not define that symbol, so we have
   to define it explicitly:
*/
#ifndef     HAVE_SCTP
#    define HAVE_SCTP
#endif

/* These changed in draft 11, so SOLARIS10 uses the old MSG_* */
#if ! HAVE_DECL_SCTP_UNORDERED
#     define    SCTP_UNORDERED  MSG_UNORDERED
#endif
#if ! HAVE_DECL_SCTP_ADDR_OVER
#     define    SCTP_ADDR_OVER  MSG_ADDR_OVER
#endif
#if ! HAVE_DECL_SCTP_ABORT
#     define    SCTP_ABORT      MSG_ABORT
#endif
#if ! HAVE_DECL_SCTP_EOF
#     define    SCTP_EOF        MSG_EOF
#endif

/* More Solaris 10 fixes: */
#if ! HAVE_DECL_SCTP_CLOSED && HAVE_DECL_SCTPS_IDLE
#    define SCTP_CLOSED SCTPS_IDLE
#    undef HAVE_DECL_SCTP_CLOSED
#    define HAVE_DECL_SCTP_CLOSED 1
#endif
#if ! HAVE_DECL_SCTP_BOUND && HAVE_DECL_SCTPS_BOUND
#    define SCTP_BOUND SCTPS_BOUND
#    undef HAVE_DECL_SCTP_BOUND
#    define HAVE_DECL_SCTP_BOUND 1
#endif
#if ! HAVE_DECL_SCTP_LISTEN && HAVE_DECL_SCTPS_LISTEN
#    define SCTP_LISTEN SCTPS_LISTEN
#    undef HAVE_DECL_SCTP_LISTEN
#    define HAVE_DECL_SCTP_LISTEN 1
#endif
#if ! HAVE_DECL_SCTP_COOKIE_WAIT && HAVE_DECL_SCTPS_COOKIE_WAIT
#    define SCTP_COOKIE_WAIT SCTPS_COOKIE_WAIT
#    undef HAVE_DECL_SCTP_COOKIE_WAIT
#    define HAVE_DECL_SCTP_COOKIE_WAIT 1
#endif
#if ! HAVE_DECL_SCTP_COOKIE_ECHOED && HAVE_DECL_SCTPS_COOKIE_ECHOED
#    define SCTP_COOKIE_ECHOED SCTPS_COOKIE_ECHOED
#    undef HAVE_DECL_SCTP_COOKIE_ECHOED
#    define HAVE_DECL_SCTP_COOKIE_ECHOED 1
#endif
#if ! HAVE_DECL_SCTP_ESTABLISHED && HAVE_DECL_SCTPS_ESTABLISHED
#    define SCTP_ESTABLISHED SCTPS_ESTABLISHED
#    undef HAVE_DECL_SCTP_ESTABLISHED
#    define HAVE_DECL_SCTP_ESTABLISHED 1
#endif
#if ! HAVE_DECL_SCTP_SHUTDOWN_PENDING && HAVE_DECL_SCTPS_SHUTDOWN_PENDING
#    define SCTP_SHUTDOWN_PENDING SCTPS_SHUTDOWN_PENDING
#    undef HAVE_DECL_SCTP_SHUTDOWN_PENDING
#    define HAVE_DECL_SCTP_SHUTDOWN_PENDING 1
#endif
#if ! HAVE_DECL_SCTP_SHUTDOWN_SENT && HAVE_DECL_SCTPS_SHUTDOWN_SENT
#    define SCTP_SHUTDOWN_SENT SCTPS_SHUTDOWN_SENT
#    undef HAVE_DECL_SCTP_SHUTDOWN_SENT
#    define HAVE_DECL_SCTP_SHUTDOWN_SENT 1
#endif
#if ! HAVE_DECL_SCTP_SHUTDOWN_RECEIVED && HAVE_DECL_SCTPS_SHUTDOWN_RECEIVED
#    define SCTP_SHUTDOWN_RECEIVED SCTPS_SHUTDOWN_RECEIVED
#    undef HAVE_DECL_SCTP_SHUTDOWN_RECEIVED
#    define HAVE_DECL_SCTP_SHUTDOWN_RECEIVED 1
#endif
#if ! HAVE_DECL_SCTP_SHUTDOWN_ACK_SENT && HAVE_DECL_SCTPS_SHUTDOWN_ACK_SENT
#    define SCTP_SHUTDOWN_ACK_SENT SCTPS_SHUTDOWN_ACK_SENT
#    undef HAVE_DECL_SCTP_SHUTDOWN_ACK_SENT
#    define HAVE_DECL_SCTP_SHUTDOWN_ACK_SENT 1
#endif
/* New spelling in lksctp 2.6.22 or maybe even earlier:
 *  adaption -> adaptation
 */
#if !defined(SCTP_ADAPTATION_LAYER) && defined (SCTP_ADAPTION_LAYER)
#     define SCTP_ADAPTATION_LAYER       SCTP_ADAPTION_LAYER
#     define SCTP_ADAPTATION_INDICATION  SCTP_ADAPTION_INDICATION
#     define sctp_adaptation_event       sctp_adaption_event
#     define sctp_setadaptation          sctp_setadaption
#     define sn_adaptation_event         sn_adaption_event
#     define sai_adaptation_ind          sai_adaption_ind
#     define ssb_adaptation_ind          ssb_adaption_ind
#     define sctp_adaptation_layer_event sctp_adaption_layer_event
#endif

/*
 * We *may* need this stuff later when we *fully* implement support for SCTP 
 *

#if defined(__GNUC__) && defined(HAVE_SCTP_BINDX)
static typeof(sctp_bindx) *esock_sctp_bindx = NULL;
#else
static int (*esock_sctp_bindx)
	(int sd, struct sockaddr *addrs, int addrcnt, int flags) = NULL;
#endif

#if defined(__GNUC__) && defined(HAVE_SCTP_PEELOFF)
static typeof(sctp_peeloff) *esock_sctp_peeloff = NULL;
#else
static int (*esock_sctp_peeloff)
        (int sd, sctp_assoc_t assoc_id) = NULL;
#endif

#if defined(__GNUC__) && defined(HAVE_SCTP_GETLADDRS)
static typeof(sctp_getladdrs) *esock_sctp_getladdrs = NULL;
#else
static int (*esock_sctp_getladdrs)
        (int sd, sctp_assoc_t assoc_id, struct sockaddr **ss) = NULL;
#endif

#if defined(__GNUC__) && defined(HAVE_SCTP_FREELADDRS)
static typeof(sctp_freeladdrs) *esock_sctp_freeladdrs = NULL;
#else
static void (*esock_sctp_freeladdrs)(struct sockaddr *addrs) = NULL;
#endif

#if defined(__GNUC__) && defined(HAVE_SCTP_GETPADDRS)
static typeof(sctp_getpaddrs) *esock_sctp_getpaddrs = NULL;
#else
static int (*esock_sctp_getpaddrs)
        (int sd, sctp_assoc_t assoc_id, struct sockaddr **ss) = NULL;
#endif

#if defined(__GNUC__) && defined(HAVE_SCTP_FREEPADDRS)
static typeof(sctp_freepaddrs) *esock_sctp_freepaddrs = NULL;
#else
static void (*esock_sctp_freepaddrs)(struct sockaddr *addrs) = NULL;
#endif

*/

#endif /* #if defined(HAVE_SCTP_H) */


#ifndef WANT_NONBLOCKING
#define WANT_NONBLOCKING
#endif
#include "sys.h"

#endif /* !__WIN32__ */

#include <erl_nif.h>

#include "socket_dbg.h"
#include "socket_tarray.h"
#include "socket_int.h"
#include "socket_util.h"

/* All platforms fail on malloc errors. */
#define FATAL_MALLOC


/* Debug stuff... */
#define SOCKET_NIF_DEBUG_DEFAULT FALSE
#define SOCKET_DEBUG_DEFAULT     FALSE

/* Counters and stuff (Don't know where to sent this stuff anyway) */
#define SOCKET_NIF_IOW_DEFAULT FALSE



/* Socket stuff */
#define INVALID_SOCKET -1
#define INVALID_EVENT  -1
#define SOCKET_ERROR   -1

#define SOCKET int
#define HANDLE long int


/* ==============================================================================
 * The IS_SOCKET_ERROR macro below is used for portability reasons.
 * While POSIX specifies that errors from socket-related system calls
 * should be indicated with a -1 return value, some users have experienced
 * non-Windows OS kernels that return negative values other than -1.
 * While one can argue that such kernels are technically broken, comparing
 * against values less than 0 covers their out-of-spec return values without
 * imposing incorrect semantics on systems that manage to correctly return -1
 * for errors, thus increasing Erlang's portability.
 */
#ifdef __WIN32__
#define IS_SOCKET_ERROR(val) ((val) == SOCKET_ERROR)
#else
#define IS_SOCKET_ERROR(val) ((val) < 0)
#endif


/* *** Misc macros and defines *** */

/* This macro exist on some (linux) platforms */
#if !defined(IPTOS_TOS_MASK)
#define IPTOS_TOS_MASK     0x1E
#endif
#if !defined(IPTOS_TOS)
#define IPTOS_TOS(tos)          ((tos)&IPTOS_TOS_MASK)
#endif


#if defined(TCP_CA_NAME_MAX)
#define SOCKET_OPT_TCP_CONGESTION_NAME_MAX TCP_CA_NAME_MAX
#else
/* This is really excessive, but just in case... */
#define SOCKET_OPT_TCP_CONGESTION_NAME_MAX 256
#endif


/* *** Socket state defs *** */

#define SOCKET_FLAG_OPEN         0x0001
#define SOCKET_FLAG_ACTIVE       0x0004
#define SOCKET_FLAG_LISTEN       0x0008
#define SOCKET_FLAG_CON          0x0010
#define SOCKET_FLAG_ACC          0x0020
#define SOCKET_FLAG_BUSY         0x0040
#define SOCKET_FLAG_CLOSE        0x0080

#define SOCKET_STATE_CLOSED          (0)
#define SOCKET_STATE_OPEN            (SOCKET_FLAG_OPEN)
#define SOCKET_STATE_CONNECTED       (SOCKET_STATE_OPEN      | SOCKET_FLAG_ACTIVE)
#define SOCKET_STATE_LISTENING       (SOCKET_STATE_OPEN      | SOCKET_FLAG_LISTEN)
#define SOCKET_STATE_CONNECTING      (SOCKET_STATE_OPEN      | SOCKET_FLAG_CON)
#define SOCKET_STATE_ACCEPTING       (SOCKET_STATE_LISTENING | SOCKET_FLAG_ACC)
#define SOCKET_STATE_CLOSING         (SOCKET_FLAG_CLOSE)

#define IS_OPEN(d) \
    (((d)->state & SOCKET_FLAG_OPEN) == SOCKET_FLAG_OPEN)

#define IS_CONNECTED(d)                                                 \
    (((d)->state & SOCKET_STATE_CONNECTED) == SOCKET_STATE_CONNECTED)

#define IS_CONNECTING(d)                                \
    (((d)->state & SOCKET_FLAG_CON) == SOCKET_FLAG_CON)

/*
#define IS_BUSY(d)                                      \
    (((d)->state & SOCKET_FLAG_BUSY) == SOCKET_FLAG_BUSY)
*/

#define SOCKET_SEND_FLAG_CONFIRM    0
#define SOCKET_SEND_FLAG_DONTROUTE  1
#define SOCKET_SEND_FLAG_EOR        2
#define SOCKET_SEND_FLAG_MORE       3
#define SOCKET_SEND_FLAG_NOSIGNAL   4
#define SOCKET_SEND_FLAG_OOB        5
#define SOCKET_SEND_FLAG_LOW        SOCKET_SEND_FLAG_CONFIRM
#define SOCKET_SEND_FLAG_HIGH       SOCKET_SEND_FLAG_OOB

#define SOCKET_RECV_FLAG_CMSG_CLOEXEC 0
#define SOCKET_RECV_FLAG_ERRQUEUE     1
#define SOCKET_RECV_FLAG_OOB          2
#define SOCKET_RECV_FLAG_PEEK         3
#define SOCKET_RECV_FLAG_TRUNC        4
#define SOCKET_RECV_FLAG_LOW          SOCKET_RECV_FLAG_CMSG_CLOEXEC
#define SOCKET_RECV_FLAG_HIGH         SOCKET_RECV_FLAG_TRUNC

#define SOCKET_RECV_BUFFER_SIZE_DEFAULT      2048
#define SOCKET_RECV_CTRL_BUFFER_SIZE_DEFAULT 1024
#define SOCKET_SEND_CTRL_BUFFER_SIZE_DEFAULT 1024

#define VT2S(__VT__) (((__VT__) == SOCKET_OPT_VALUE_TYPE_UNSPEC) ? "unspec" : \
                      (((__VT__) == SOCKET_OPT_VALUE_TYPE_INT) ? "int" : \
                       ((__VT__) == SOCKET_OPT_VALUE_TYPE_BOOL) ? "bool" : \
                       "undef"))

#define SOCKET_OPT_VALUE_TYPE_UNSPEC 0
#define SOCKET_OPT_VALUE_TYPE_INT    1
#define SOCKET_OPT_VALUE_TYPE_BOOL   2

typedef union {
    struct {
        // 0 = not open, 1 = open
        unsigned int open:1;
        // 0 = not conn, 1 = connecting, 2 = connected
        unsigned int connect:2;
        // unsigned int connecting:1;
        // unsigned int connected:1;
        // 0 = not listen, 1 = listening, 2 = accepting
        unsigned int listen:2;
        // unsigned int listening:1;
        // unsigned int accepting:1;
        /* Room for more... */
    } flags;
    unsigned int field; // Make it easy to reset all flags...
} SocketState;

/*
#define IS_OPEN(d)       ((d)->state.flags.open)
#define IS_CONNECTED(d)  ((d)->state.flags.connect == SOCKET_STATE_CONNECTED)
#define IS_CONNECTING(d) ((d)->state.flags.connect == SOCKET_STATE_CONNECTING)
*/


/*----------------------------------------------------------------------------
 * Interface constants.
 *
 * This section must be "identical" to the corresponding socket.hrl
 */

/* domain */
#define SOCKET_DOMAIN_LOCAL       1
#define SOCKET_DOMAIN_INET        2
#define SOCKET_DOMAIN_INET6       3

/* type */
#define SOCKET_TYPE_STREAM        1
#define SOCKET_TYPE_DGRAM         2
#define SOCKET_TYPE_RAW           3
// #define SOCKET_TYPE_RDM           4
#define SOCKET_TYPE_SEQPACKET     5

/* protocol */
#define SOCKET_PROTOCOL_IP        1
#define SOCKET_PROTOCOL_TCP       2
#define SOCKET_PROTOCOL_UDP       3
#define SOCKET_PROTOCOL_SCTP      4
#define SOCKET_PROTOCOL_ICMP      5
#define SOCKET_PROTOCOL_IGMP      6

/* shutdown how */
#define SOCKET_SHUTDOWN_HOW_RD    0
#define SOCKET_SHUTDOWN_HOW_WR    1
#define SOCKET_SHUTDOWN_HOW_RDWR  2


#define SOCKET_OPT_LEVEL_OTP        0
#define SOCKET_OPT_LEVEL_SOCKET     1
#define SOCKET_OPT_LEVEL_IP         2
#define SOCKET_OPT_LEVEL_IPV6       3
#define SOCKET_OPT_LEVEL_TCP        4
#define SOCKET_OPT_LEVEL_UDP        5
#define SOCKET_OPT_LEVEL_SCTP       6

#define SOCKET_OPT_OTP_DEBUG        1
#define SOCKET_OPT_OTP_IOW          2
#define SOCKET_OPT_OTP_CTRL_PROC    3
#define SOCKET_OPT_OTP_RCVBUF       4
#define SOCKET_OPT_OTP_RCVCTRLBUF   6
#define SOCKET_OPT_OTP_SNDCTRLBUF   7

#define SOCKET_OPT_SOCK_ACCEPTCONN     1
#define SOCKET_OPT_SOCK_BINDTODEVICE   3
#define SOCKET_OPT_SOCK_BROADCAST      4
#define SOCKET_OPT_SOCK_DEBUG          6
#define SOCKET_OPT_SOCK_DOMAIN         7
#define SOCKET_OPT_SOCK_DONTROUTE      8
#define SOCKET_OPT_SOCK_KEEPALIVE     10
#define SOCKET_OPT_SOCK_LINGER        11
#define SOCKET_OPT_SOCK_OOBINLINE     13
#define SOCKET_OPT_SOCK_PEEK_OFF      15
#define SOCKET_OPT_SOCK_PRIORITY      17
#define SOCKET_OPT_SOCK_PROTOCOL      18
#define SOCKET_OPT_SOCK_RCVBUF        19
#define SOCKET_OPT_SOCK_RCVLOWAT      21
#define SOCKET_OPT_SOCK_RCVTIMEO      22
#define SOCKET_OPT_SOCK_REUSEADDR     23
#define SOCKET_OPT_SOCK_REUSEPORT     24
#define SOCKET_OPT_SOCK_SNDBUF        27
#define SOCKET_OPT_SOCK_SNDLOWAT      29
#define SOCKET_OPT_SOCK_SNDTIMEO      30
#define SOCKET_OPT_SOCK_TIMESTAMP     31
#define SOCKET_OPT_SOCK_TYPE          32

#define SOCKET_OPT_IP_ADD_MEMBERSHIP          1
#define SOCKET_OPT_IP_ADD_SOURCE_MEMBERSHIP   2
#define SOCKET_OPT_IP_BLOCK_SOURCE            3
#define SOCKET_OPT_IP_DROP_MEMBERSHIP         5
#define SOCKET_OPT_IP_DROP_SOURCE_MEMBERSHIP  6
#define SOCKET_OPT_IP_FREEBIND                7
#define SOCKET_OPT_IP_HDRINCL                 8
#define SOCKET_OPT_IP_MINTTL                  9
#define SOCKET_OPT_IP_MSFILTER               10
#define SOCKET_OPT_IP_MTU                    11
#define SOCKET_OPT_IP_MTU_DISCOVER           12
#define SOCKET_OPT_IP_MULTICAST_ALL          13
#define SOCKET_OPT_IP_MULTICAST_IF           14
#define SOCKET_OPT_IP_MULTICAST_LOOP         15
#define SOCKET_OPT_IP_MULTICAST_TTL          16
#define SOCKET_OPT_IP_NODEFRAG               17
#define SOCKET_OPT_IP_PKTINFO                19
#define SOCKET_OPT_IP_RECVDSTADDR            20
#define SOCKET_OPT_IP_RECVERR                21
#define SOCKET_OPT_IP_RECVIF                 22
#define SOCKET_OPT_IP_RECVOPTS               23
#define SOCKET_OPT_IP_RECVORIGDSTADDR        24
#define SOCKET_OPT_IP_RECVTOS                25
#define SOCKET_OPT_IP_RECVTTL                26
#define SOCKET_OPT_IP_RETOPTS                27
#define SOCKET_OPT_IP_ROUTER_ALERT           28
#define SOCKET_OPT_IP_SENDSRCADDR            29 // Same as IP_RECVDSTADDR?
#define SOCKET_OPT_IP_TOS                    30
#define SOCKET_OPT_IP_TRANSPARENT            31
#define SOCKET_OPT_IP_TTL                    32
#define SOCKET_OPT_IP_UNBLOCK_SOURCE         33

#define SOCKET_OPT_IPV6_ADDRFORM              1
#define SOCKET_OPT_IPV6_ADD_MEMBERSHIP        2
#define SOCKET_OPT_IPV6_AUTHHDR               3
#define SOCKET_OPT_IPV6_DROP_MEMBERSHIP       6
#define SOCKET_OPT_IPV6_DSTOPTS               7
#define SOCKET_OPT_IPV6_FLOWINFO             11
#define SOCKET_OPT_IPV6_HOPLIMIT             12
#define SOCKET_OPT_IPV6_HOPOPTS              13
#define SOCKET_OPT_IPV6_MTU                  17
#define SOCKET_OPT_IPV6_MTU_DISCOVER         18
#define SOCKET_OPT_IPV6_MULTICAST_HOPS       19
#define SOCKET_OPT_IPV6_MULTICAST_IF         20
#define SOCKET_OPT_IPV6_MULTICAST_LOOP       21
#define SOCKET_OPT_IPV6_RECVERR              24
#define SOCKET_OPT_IPV6_RECVPKTINFO          25 // PKTINFO on FreeBSD
#define SOCKET_OPT_IPV6_ROUTER_ALERT         27
#define SOCKET_OPT_IPV6_RTHDR                28
#define SOCKET_OPT_IPV6_UNICAST_HOPS         30
#define SOCKET_OPT_IPV6_V6ONLY               32

#define SOCKET_OPT_TCP_CONGESTION   1
#define SOCKET_OPT_TCP_CORK         2
#define SOCKET_OPT_TCP_MAXSEG       7
#define SOCKET_OPT_TCP_NODELAY      9

#define SOCKET_OPT_UDP_CORK         1

#define SOCKET_OPT_SCTP_ASSOCINFO           2
#define SOCKET_OPT_SCTP_AUTOCLOSE           8
#define SOCKET_OPT_SCTP_DISABLE_FRAGMENTS  12
#define SOCKET_OPT_SCTP_EVENTS             14
#define SOCKET_OPT_SCTP_INITMSG            18
#define SOCKET_OPT_SCTP_MAXSEG             21
#define SOCKET_OPT_SCTP_NODELAY            23
#define SOCKET_OPT_SCTP_RTOINFO            29

/* We should *eventually* use this instead of hard-coding the size (to 1) */
#define ESOCK_RECVMSG_IOVEC_SZ 1



/* =================================================================== *
 *                                                                     *
 *                        Various enif macros                          *
 *                                                                     *
 * =================================================================== */

#define SGDBG( proto )         ESOCK_DBG_PRINTF( data.dbg , proto )
#define SSDBG( __D__ , proto ) ESOCK_DBG_PRINTF( (__D__)->dbg , proto )



/* =================================================================== *
 *                                                                     *
 *                    Basic socket operations                          *
 *                                                                     *
 * =================================================================== */

#ifdef __WIN32__

/* *** Windows macros *** */

#define sock_accept(s, addr, len) \
    make_noninheritable_handle(accept((s), (addr), (len)))
#define sock_bind(s, addr, len)        bind((s), (addr), (len))
#define sock_close(s)                  closesocket((s))
#define sock_close_event(e)            WSACloseEvent(e)
#define sock_connect(s, addr, len)     connect((s), (addr), (len))
#define sock_create_event(s)           WSACreateEvent()
#define sock_errno()                   WSAGetLastError()
#define sock_getopt(s,l,o,v,ln)        getsockopt((s),(l),(o),(v),(ln))
#define sock_htons(x)                  htons((x))
#define sock_htonl(x)                  htonl((x))
#define sock_listen(s, b)              listen((s), (b))
#define sock_name(s, addr, len)        getsockname((s), (addr), (len))
#define sock_ntohs(x)                  ntohs((x))
#define sock_open(domain, type, proto)                             \
    make_noninheritable_handle(socket((domain), (type), (proto)))
#define sock_peer(s, addr, len)    getpeername((s), (addr), (len))
#define sock_recv(s,buf,len,flag)  recv((s),(buf),(len),(flag))
#define sock_recvfrom(s,buf,blen,flag,addr,alen) \
    recvfrom((s),(buf),(blen),(flag),(addr),(alen))
#define sock_send(s,buf,len,flag)      send((s),(buf),(len),(flag))
#define sock_sendto(s,buf,blen,flag,addr,alen) \
    sendto((s),(buf),(blen),(flag),(addr),(alen))
#define sock_setopt(s,l,o,v,ln)        setsockopt((s),(l),(o),(v),(ln))
#define sock_shutdown(s, how)          shutdown((s), (how))


#define SET_BLOCKING(s)            ioctlsocket(s, FIONBIO, &zero_value)
#define SET_NONBLOCKING(s)         ioctlsocket(s, FIONBIO, &one_value)
static unsigned long zero_value = 0;
static unsigned long one_value  = 1;


#else /* !__WIN32__ */


#ifdef HAS_ACCEPT4
// We have to figure out what the flags are...
#define sock_accept(s, addr, len)       accept4((s), (addr), (len), (SOCK_CLOEXEC))
#else
#define sock_accept(s, addr, len)       accept((s), (addr), (len))
#endif
#define sock_bind(s, addr, len)         bind((s), (addr), (len))
#define sock_close(s)                   close((s))
#define sock_close_event(e)             /* do nothing */
#define sock_connect(s, addr, len)      connect((s), (addr), (len))
#define sock_create_event(s)            (s) /* return file descriptor */
#define sock_errno()                    errno
#define sock_getopt(s,t,n,v,l)          getsockopt((s),(t),(n),(v),(l))
#define sock_htons(x)                   htons((x))
#define sock_htonl(x)                   htonl((x))
#define sock_listen(s, b)               listen((s), (b))
#define sock_name(s, addr, len)         getsockname((s), (addr), (len))
#define sock_ntohs(x)                   ntohs((x))
#define sock_open(domain, type, proto)  socket((domain), (type), (proto))
#define sock_peer(s, addr, len)         getpeername((s), (addr), (len))
#define sock_recv(s,buf,len,flag)       recv((s),(buf),(len),(flag))
#define sock_recvfrom(s,buf,blen,flag,addr,alen) \
    recvfrom((s),(buf),(blen),(flag),(addr),(alen))
#define sock_recvmsg(s,msghdr,flag)     recvmsg((s),(msghdr),(flag))
#define sock_send(s,buf,len,flag)       send((s), (buf), (len), (flag))
#define sock_sendmsg(s,msghdr,flag)     sendmsg((s),(msghdr),(flag))
#define sock_sendto(s,buf,blen,flag,addr,alen) \
                sendto((s),(buf),(blen),(flag),(addr),(alen))
#define sock_setopt(s,l,o,v,ln)         setsockopt((s),(l),(o),(v),(ln))
#define sock_shutdown(s, how)           shutdown((s), (how))

#endif /* !__WIN32__ */

#ifdef HAVE_SOCKLEN_T
#  define SOCKLEN_T socklen_t
#else
#  define SOCKLEN_T size_t
#endif

#ifdef __WIN32__
#define SOCKOPTLEN_T int
#else
#define SOCKOPTLEN_T SOCKLEN_T
#endif

/* We can use the IPv4 def for this since the beginning
 * is the same for INET and INET6 */
#define which_address_port(sap)		     \
  ((((sap)->in4.sin_family == AF_INET) ||  \
    ((sap)->in4.sin_family == AF_INET6)) ? \
   ((sap)->in4.sin_port) : -1)


typedef struct {
    ErlNifPid     pid; // PID of the requesting process
    ErlNifMonitor mon; // Monitor to the requesting process
    ERL_NIF_TERM  ref; // The (unique) reference (ID) of the request
} SocketRequestor;

typedef struct socket_request_queue_element {
    struct socket_request_queue_element* nextP;
    SocketRequestor                      data;
} SocketRequestQueueElement;

typedef struct {
    SocketRequestQueueElement* first;
    SocketRequestQueueElement* last;
} SocketRequestQueue;


typedef struct {
    /* +++ The actual socket +++ */
    SOCKET         sock;
    HANDLE         event;

    /* +++ Stuff "about" the socket +++ */
    int            domain;
    int            type;
    int            protocol;

    unsigned int   state;
    SocketAddress  remote;
    unsigned int   addrLen;

    ErlNifEnv*     env;

    /* +++ Controller (owner) process +++ */
    ErlNifPid      ctrlPid;
    ErlNifMonitor  ctrlMon;

    /* +++ Write stuff +++ */
    ErlNifMutex*       writeMtx;
    SocketRequestor    currentWriter;
    SocketRequestor*   currentWriterP; // NULL or points to currentWriter
    SocketRequestQueue writersQ;
    BOOLEAN_T          isWritable;
    uint32_t           writePkgCnt;
    uint32_t           writeByteCnt;
    uint32_t           writeTries;
    uint32_t           writeWaits;
    uint32_t           writeFails;

    /* +++ Read stuff +++ */
    ErlNifMutex*       readMtx;
    SocketRequestor    currentReader;
    SocketRequestor*   currentReaderP; // NULL or points to currentReader
    SocketRequestQueue readersQ;
    BOOLEAN_T          isReadable;
    ErlNifBinary       rbuffer;      // DO WE NEED THIS
    uint32_t           readCapacity; // DO WE NEED THIS
    uint32_t           readPkgCnt;
    uint32_t           readByteCnt;
    uint32_t           readTries;
    uint32_t           readWaits;

    /* +++ Accept stuff +++ */
    ErlNifMutex*       accMtx;
    SocketRequestor    currentAcceptor;
    SocketRequestor*   currentAcceptorP; // NULL or points to currentAcceptor
    SocketRequestQueue acceptorsQ;

    /* +++ Config & Misc stuff +++ */
    size_t    rBufSz;  // Read buffer size (when data length = 0 is specified)
    size_t    rCtrlSz; // Read control buffer size
    size_t    wCtrlSz; // Write control buffer size
    BOOLEAN_T iow;     // Inform On (counter) Wrap
    BOOLEAN_T dbg;

    /* +++ Close stuff +++ */
    ErlNifMutex*  closeMtx;
    ErlNifPid     closerPid;
    ErlNifMonitor closerMon;
    ERL_NIF_TERM  closeRef;
    BOOLEAN_T     closeLocal;

} SocketDescriptor;


#define SOCKET_OPT_VALUE_UNDEF  0
#define SOCKET_OPT_VALUE_BOOL   1
#define SOCKET_OPT_VALUE_INT    2
#define SOCKET_OPT_VALUE_LINGER 3
#define SOCKET_OPT_VALUE_BIN    4
#define SOCKET_OPT_VALUE_STR    5

typedef struct {
    unsigned int tag;
    union {
        BOOLEAN_T     boolVal;
        int           intVal;
        struct linger lingerVal;
        ErlNifBinary  binVal;
        struct {
            unsigned int len;
            char*        str;
        } strVal;
    } u;
    /*
    void*     optValP;   // Points to the actual data (above)
    socklen_t optValLen; // The size of the option value
    */
} SocketOptValue;


/* Global stuff (do we really need to "collect"
 * these things?)
 */
typedef struct {
    /* These are for debugging, testing and the like */
    ERL_NIF_TERM version;
    ERL_NIF_TERM buildDate;
    BOOLEAN_T    dbg;

    BOOLEAN_T    iow;
    ErlNifMutex* cntMtx;
    uint32_t     numSockets;
    uint32_t     numTypeStreams;
    uint32_t     numTypeDGrams;
    uint32_t     numTypeSeqPkgs;
    uint32_t     numDomainInet;
    uint32_t     numDomainInet6;
    uint32_t     numDomainLocal;
    uint32_t     numProtoIP;
    uint32_t     numProtoTCP;
    uint32_t     numProtoUDP;
    uint32_t     numProtoSCTP;
} SocketData;


/* ----------------------------------------------------------------------
 *  F o r w a r d s
 * ----------------------------------------------------------------------
 */



static ERL_NIF_TERM nif_info(ErlNifEnv*         env,
                             int                argc,
                             const ERL_NIF_TERM argv[]);
/*
This is a *global* debug function (enable or disable for all
operations and all sockets.
static ERL_NIF_TERM nif_debug(ErlNifEnv*         env,
                              int                argc,
                              const ERL_NIF_TERM argv[]);
*/
static ERL_NIF_TERM nif_open(ErlNifEnv*         env,
                             int                argc,
                             const ERL_NIF_TERM argv[]);
static ERL_NIF_TERM nif_bind(ErlNifEnv*         env,
                             int                argc,
                             const ERL_NIF_TERM argv[]);
static ERL_NIF_TERM nif_connect(ErlNifEnv*         env,
                               int                argc,
                               const ERL_NIF_TERM argv[]);
static ERL_NIF_TERM nif_listen(ErlNifEnv*         env,
                               int                argc,
                               const ERL_NIF_TERM argv[]);
static ERL_NIF_TERM nif_accept(ErlNifEnv*         env,
                               int                argc,
                               const ERL_NIF_TERM argv[]);
static ERL_NIF_TERM nif_send(ErlNifEnv*         env,
                             int                argc,
                             const ERL_NIF_TERM argv[]);
static ERL_NIF_TERM nif_sendto(ErlNifEnv*         env,
                               int                argc,
                               const ERL_NIF_TERM argv[]);
static ERL_NIF_TERM nif_sendmsg(ErlNifEnv*         env,
                                int                argc,
                                const ERL_NIF_TERM argv[]);
static ERL_NIF_TERM nif_recv(ErlNifEnv*         env,
                             int                argc,
                             const ERL_NIF_TERM argv[]);
static ERL_NIF_TERM nif_recvfrom(ErlNifEnv*         env,
                                 int                argc,
                                 const ERL_NIF_TERM argv[]);
static ERL_NIF_TERM nif_recvmsg(ErlNifEnv*         env,
                                int                argc,
                                const ERL_NIF_TERM argv[]);
static ERL_NIF_TERM nif_close(ErlNifEnv*         env,
                              int                argc,
                              const ERL_NIF_TERM argv[]);
static ERL_NIF_TERM nif_shutdown(ErlNifEnv*         env,
                                 int                argc,
                                 const ERL_NIF_TERM argv[]);
static ERL_NIF_TERM nif_setopt(ErlNifEnv*         env,
                               int                argc,
                               const ERL_NIF_TERM argv[]);
static ERL_NIF_TERM nif_getopt(ErlNifEnv*         env,
                               int                argc,
                               const ERL_NIF_TERM argv[]);
static ERL_NIF_TERM nif_sockname(ErlNifEnv*         env,
                                 int                argc,
                                 const ERL_NIF_TERM argv[]);
static ERL_NIF_TERM nif_peername(ErlNifEnv*         env,
                                 int                argc,
                                 const ERL_NIF_TERM argv[]);
static ERL_NIF_TERM nif_finalize_connection(ErlNifEnv*         env,
                                            int                argc,
                                            const ERL_NIF_TERM argv[]);
static ERL_NIF_TERM nif_finalize_close(ErlNifEnv*         env,
                                       int                argc,
                                       const ERL_NIF_TERM argv[]);
static ERL_NIF_TERM nif_cancel(ErlNifEnv*         env,
                               int                argc,
                               const ERL_NIF_TERM argv[]);


static ERL_NIF_TERM nopen(ErlNifEnv* env,
                          int        domain,
                          int        type,
                          int        protocol,
                          char*      netns);
static ERL_NIF_TERM nbind(ErlNifEnv*        env,
                          SocketDescriptor* descP,
                          SocketAddress*    sockAddrP,
                          unsigned int      addrLen);
static ERL_NIF_TERM nconnect(ErlNifEnv*        env,
                             SocketDescriptor* descP);
static ERL_NIF_TERM nlisten(ErlNifEnv*        env,
                            SocketDescriptor* descP,
                            int               backlog);
static ERL_NIF_TERM naccept_listening(ErlNifEnv*        env,
                                      SocketDescriptor* descP,
                                      ERL_NIF_TERM      ref);
static ERL_NIF_TERM naccept_accepting(ErlNifEnv*        env,
                                      SocketDescriptor* descP,
                                      ERL_NIF_TERM      ref);
static ERL_NIF_TERM naccept(ErlNifEnv*        env,
                            SocketDescriptor* descP,
                            ERL_NIF_TERM      ref);
static ERL_NIF_TERM nsend(ErlNifEnv*        env,
                          SocketDescriptor* descP,
                          ERL_NIF_TERM      sendRef,
                          ErlNifBinary*     dataP,
                          int               flags);
static ERL_NIF_TERM nsendto(ErlNifEnv*        env,
                            SocketDescriptor* descP,
                            ERL_NIF_TERM      sendRef,
                            ErlNifBinary*     dataP,
                            int               flags,
                            SocketAddress*    toAddrP,
                            unsigned int      toAddrLen);
static ERL_NIF_TERM nsendmsg(ErlNifEnv*        env,
                             SocketDescriptor* descP,
                             ERL_NIF_TERM      sendRef,
                             ERL_NIF_TERM      eMsgHdr,
                             int               flags);
static ERL_NIF_TERM nrecv(ErlNifEnv*        env,
                          SocketDescriptor* descP,
                          ERL_NIF_TERM      recvRef,
                          int               len,
                          int               flags);
static ERL_NIF_TERM nrecvfrom(ErlNifEnv*        env,
                              SocketDescriptor* descP,
                              ERL_NIF_TERM      recvRef,
                              uint16_t          bufSz,
                              int               flags);
static ERL_NIF_TERM nrecvmsg(ErlNifEnv*        env,
                             SocketDescriptor* descP,
                             ERL_NIF_TERM      recvRef,
                             uint16_t          bufLen,
                             uint16_t          ctrlLen,
                             int               flags);
static ERL_NIF_TERM nclose(ErlNifEnv*        env,
                           SocketDescriptor* descP);
static ERL_NIF_TERM nshutdown(ErlNifEnv*        env,
                              SocketDescriptor* descP,
                              int               how);
static ERL_NIF_TERM nsetopt(ErlNifEnv*        env,
                            SocketDescriptor* descP,
                            BOOLEAN_T         isEncoded,
                            BOOLEAN_T         isOTP,
                            int               level,
                            int               eOpt,
                            ERL_NIF_TERM      eVal);
static ERL_NIF_TERM nsetopt_otp(ErlNifEnv*        env,
                                SocketDescriptor* descP,
                                int               eOpt,
                                ERL_NIF_TERM      eVal);
static ERL_NIF_TERM nsetopt_otp_debug(ErlNifEnv*        env,
                                      SocketDescriptor* descP,
                                      ERL_NIF_TERM      eVal);
static ERL_NIF_TERM nsetopt_otp_iow(ErlNifEnv*        env,
                                    SocketDescriptor* descP,
                                    ERL_NIF_TERM      eVal);
static ERL_NIF_TERM nsetopt_otp_ctrl_proc(ErlNifEnv*        env,
                                          SocketDescriptor* descP,
                                          ERL_NIF_TERM      eVal);
static ERL_NIF_TERM nsetopt_otp_rcvbuf(ErlNifEnv*        env,
                                       SocketDescriptor* descP,
                                       ERL_NIF_TERM      eVal);
static ERL_NIF_TERM nsetopt_otp_rcvctrlbuf(ErlNifEnv*        env,
                                           SocketDescriptor* descP,
                                           ERL_NIF_TERM      eVal);
static ERL_NIF_TERM nsetopt_otp_sndctrlbuf(ErlNifEnv*        env,
                                           SocketDescriptor* descP,
                                           ERL_NIF_TERM      eVal);
static ERL_NIF_TERM nsetopt_native(ErlNifEnv*        env,
                                   SocketDescriptor* descP,
                                   int               level,
                                   int               eOpt,
                                   ERL_NIF_TERM      eVal);
static ERL_NIF_TERM nsetopt_level(ErlNifEnv*        env,
                                  SocketDescriptor* descP,
                                  int               level,
                                  int               eOpt,
                                  ERL_NIF_TERM      eVal);
static ERL_NIF_TERM nsetopt_lvl_socket(ErlNifEnv*        env,
                                       SocketDescriptor* descP,
                                       int               eOpt,
                                       ERL_NIF_TERM      eVal);


/* *** Handling set of socket options for level = socket *** */

#if defined(SO_BINDTODEVICE)
static ERL_NIF_TERM nsetopt_lvl_sock_bindtodevice(ErlNifEnv*        env,
                                                  SocketDescriptor* descP,
                                                  ERL_NIF_TERM      eVal);
#endif
#if defined(SO_BROADCAST)
static ERL_NIF_TERM nsetopt_lvl_sock_broadcast(ErlNifEnv*        env,
                                               SocketDescriptor* descP,
                                               ERL_NIF_TERM      eVal);
#endif
#if defined(SO_DEBUG)
static ERL_NIF_TERM nsetopt_lvl_sock_debug(ErlNifEnv*        env,
                                           SocketDescriptor* descP,
                                           ERL_NIF_TERM      eVal);
#endif
#if defined(SO_DONTROUTE)
static ERL_NIF_TERM nsetopt_lvl_sock_dontroute(ErlNifEnv*        env,
                                               SocketDescriptor* descP,
                                               ERL_NIF_TERM      eVal);
#endif
#if defined(SO_KEEPALIVE)
static ERL_NIF_TERM nsetopt_lvl_sock_keepalive(ErlNifEnv*        env,
                                               SocketDescriptor* descP,
                                               ERL_NIF_TERM      eVal);
#endif
#if defined(SO_LINGER)
static ERL_NIF_TERM nsetopt_lvl_sock_linger(ErlNifEnv*        env,
                                            SocketDescriptor* descP,
                                            ERL_NIF_TERM      eVal);
#endif
#if defined(SO_OOBINLINE)
static ERL_NIF_TERM nsetopt_lvl_sock_oobinline(ErlNifEnv*        env,
                                               SocketDescriptor* descP,
                                               ERL_NIF_TERM      eVal);
#endif
#if defined(SO_PEEK_OFF)
static ERL_NIF_TERM nsetopt_lvl_sock_peek_off(ErlNifEnv*        env,
                                              SocketDescriptor* descP,
                                              ERL_NIF_TERM      eVal);
#endif
#if defined(SO_PRIORITY)
static ERL_NIF_TERM nsetopt_lvl_sock_priority(ErlNifEnv*        env,
                                              SocketDescriptor* descP,
                                              ERL_NIF_TERM      eVal);
#endif
#if defined(SO_RCVBUF)
static ERL_NIF_TERM nsetopt_lvl_sock_rcvbuf(ErlNifEnv*        env,
                                            SocketDescriptor* descP,
                                            ERL_NIF_TERM      eVal);
#endif
#if defined(SO_RCVLOWAT)
static ERL_NIF_TERM nsetopt_lvl_sock_rcvlowat(ErlNifEnv*        env,
                                              SocketDescriptor* descP,
                                              ERL_NIF_TERM      eVal);
#endif
#if defined(SO_RCVTIMEO)
static ERL_NIF_TERM nsetopt_lvl_sock_rcvtimeo(ErlNifEnv*        env,
                                              SocketDescriptor* descP,
                                              ERL_NIF_TERM      eVal);
#endif
#if defined(SO_REUSEADDR)
static ERL_NIF_TERM nsetopt_lvl_sock_reuseaddr(ErlNifEnv*        env,
                                               SocketDescriptor* descP,
                                               ERL_NIF_TERM      eVal);
#endif
#if defined(SO_REUSEPORT)
static ERL_NIF_TERM nsetopt_lvl_sock_reuseport(ErlNifEnv*        env,
                                               SocketDescriptor* descP,
                                               ERL_NIF_TERM      eVal);
#endif
#if defined(SO_SNDBUF)
static ERL_NIF_TERM nsetopt_lvl_sock_sndbuf(ErlNifEnv*        env,
                                            SocketDescriptor* descP,
                                            ERL_NIF_TERM      eVal);
#endif
#if defined(SO_SNDLOWAT)
static ERL_NIF_TERM nsetopt_lvl_sock_sndlowat(ErlNifEnv*        env,
                                              SocketDescriptor* descP,
                                              ERL_NIF_TERM      eVal);
#endif
#if defined(SO_SNDTIMEO)
static ERL_NIF_TERM nsetopt_lvl_sock_sndtimeo(ErlNifEnv*        env,
                                              SocketDescriptor* descP,
                                              ERL_NIF_TERM      eVal);
#endif
#if defined(SO_TIMESTAMP)
static ERL_NIF_TERM nsetopt_lvl_sock_timestamp(ErlNifEnv*        env,
                                               SocketDescriptor* descP,
                                               ERL_NIF_TERM      eVal);
#endif
static ERL_NIF_TERM nsetopt_lvl_ip(ErlNifEnv*        env,
                                   SocketDescriptor* descP,
                                   int               eOpt,
                                   ERL_NIF_TERM      eVal);

/* *** Handling set of socket options for level = ip *** */
#if defined(IP_ADD_MEMBERSHIP)
static ERL_NIF_TERM nsetopt_lvl_ip_add_membership(ErlNifEnv*        env,
                                                  SocketDescriptor* descP,
                                                  ERL_NIF_TERM      eVal);
#endif
#if defined(IP_ADD_SOURCE_MEMBERSHIP)
static ERL_NIF_TERM nsetopt_lvl_ip_add_source_membership(ErlNifEnv*        env,
                                                         SocketDescriptor* descP,
                                                         ERL_NIF_TERM      eVal);
#endif
#if defined(IP_BLOCK_SOURCE)
static ERL_NIF_TERM nsetopt_lvl_ip_block_source(ErlNifEnv*        env,
                                                SocketDescriptor* descP,
                                                ERL_NIF_TERM      eVal);
#endif
#if defined(IP_DROP_MEMBERSHIP)
static ERL_NIF_TERM nsetopt_lvl_ip_drop_membership(ErlNifEnv*        env,
                                                   SocketDescriptor* descP,
                                                   ERL_NIF_TERM      eVal);
#endif
#if defined(IP_DROP_SOURCE_MEMBERSHIP)
static ERL_NIF_TERM nsetopt_lvl_ip_drop_source_membership(ErlNifEnv*        env,
                                                          SocketDescriptor* descP,
                                                          ERL_NIF_TERM      eVal);
#endif
#if defined(IP_FREEBIND)
static ERL_NIF_TERM nsetopt_lvl_ip_freebind(ErlNifEnv*        env,
                                            SocketDescriptor* descP,
                                            ERL_NIF_TERM      eVal);
#endif
#if defined(IP_HDRINCL)
static ERL_NIF_TERM nsetopt_lvl_ip_hdrincl(ErlNifEnv*        env,
                                           SocketDescriptor* descP,
                                           ERL_NIF_TERM      eVal);
#endif
#if defined(IP_MINTTL)
static ERL_NIF_TERM nsetopt_lvl_ip_minttl(ErlNifEnv*        env,
                                          SocketDescriptor* descP,
                                          ERL_NIF_TERM      eVal);
#endif
#if defined(IP_MSFILTER) && defined(IP_MSFILTER_SIZE)
static ERL_NIF_TERM nsetopt_lvl_ip_msfilter(ErlNifEnv*        env,
                                            SocketDescriptor* descP,
                                            ERL_NIF_TERM      eVal);
static BOOLEAN_T decode_ip_msfilter_mode(ErlNifEnv*   env,
                                         ERL_NIF_TERM eVal,
                                         uint32_t*    mode);
static ERL_NIF_TERM nsetopt_lvl_ip_msfilter_set(ErlNifEnv*          env,
                                                SOCKET              sock,
                                                struct ip_msfilter* msfP,
                                                SOCKLEN_T           optLen);
#endif
#if defined(IP_MTU_DISCOVER)
static ERL_NIF_TERM nsetopt_lvl_ip_mtu_discover(ErlNifEnv*        env,
                                                SocketDescriptor* descP,
                                                ERL_NIF_TERM      eVal);
#endif
#if defined(IP_MULTICAST_ALL)
static ERL_NIF_TERM nsetopt_lvl_ip_multicast_all(ErlNifEnv*        env,
                                                 SocketDescriptor* descP,
                                                 ERL_NIF_TERM      eVal);
#endif
#if defined(IP_MULTICAST_IF)
static ERL_NIF_TERM nsetopt_lvl_ip_multicast_if(ErlNifEnv*        env,
                                                SocketDescriptor* descP,
                                                ERL_NIF_TERM      eVal);
#endif
#if defined(IP_MULTICAST_LOOP)
static ERL_NIF_TERM nsetopt_lvl_ip_multicast_loop(ErlNifEnv*        env,
                                                  SocketDescriptor* descP,
                                                  ERL_NIF_TERM      eVal);
#endif
#if defined(IP_MULTICAST_TTL)
static ERL_NIF_TERM nsetopt_lvl_ip_multicast_ttl(ErlNifEnv*        env,
                                                 SocketDescriptor* descP,
                                                 ERL_NIF_TERM      eVal);
#endif
#if defined(IP_NODEFRAG)
static ERL_NIF_TERM nsetopt_lvl_ip_nodefrag(ErlNifEnv*        env,
                                            SocketDescriptor* descP,
                                            ERL_NIF_TERM      eVal);
#endif
#if defined(IP_PKTINFO)
static ERL_NIF_TERM nsetopt_lvl_ip_pktinfo(ErlNifEnv*        env,
                                           SocketDescriptor* descP,
                                           ERL_NIF_TERM      eVal);
#endif
#if defined(IP_RECVDSTADDR)
static ERL_NIF_TERM nsetopt_lvl_ip_recvdstaddr(ErlNifEnv*        env,
                                               SocketDescriptor* descP,
                                               ERL_NIF_TERM      eVal);
#endif
#if defined(IP_RECVERR)
static ERL_NIF_TERM nsetopt_lvl_ip_recverr(ErlNifEnv*        env,
                                           SocketDescriptor* descP,
                                           ERL_NIF_TERM      eVal);
#endif
#if defined(IP_RECVIF)
static ERL_NIF_TERM nsetopt_lvl_ip_recvif(ErlNifEnv*        env,
                                          SocketDescriptor* descP,
                                          ERL_NIF_TERM      eVal);
#endif
#if defined(IP_RECVOPTS)
static ERL_NIF_TERM nsetopt_lvl_ip_recvopts(ErlNifEnv*        env,
                                            SocketDescriptor* descP,
                                            ERL_NIF_TERM      eVal);
#endif
#if defined(IP_RECVORIGDSTADDR)
static ERL_NIF_TERM nsetopt_lvl_ip_recvorigdstaddr(ErlNifEnv*        env,
                                                   SocketDescriptor* descP,
                                                   ERL_NIF_TERM      eVal);
#endif
#if defined(IP_RECVTOS)
static ERL_NIF_TERM nsetopt_lvl_ip_recvtos(ErlNifEnv*        env,
                                           SocketDescriptor* descP,
                                           ERL_NIF_TERM      eVal);
#endif
#if defined(IP_RECVTTL)
static ERL_NIF_TERM nsetopt_lvl_ip_recvttl(ErlNifEnv*        env,
                                           SocketDescriptor* descP,
                                           ERL_NIF_TERM      eVal);
#endif
#if defined(IP_RETOPTS)
static ERL_NIF_TERM nsetopt_lvl_ip_retopts(ErlNifEnv*        env,
                                           SocketDescriptor* descP,
                                           ERL_NIF_TERM      eVal);
#endif
#if defined(IP_ROUTER_ALERT)
static ERL_NIF_TERM nsetopt_lvl_ip_router_alert(ErlNifEnv*        env,
                                                SocketDescriptor* descP,
                                                ERL_NIF_TERM      eVal);
#endif
#if defined(IP_SENDSRCADDR)
static ERL_NIF_TERM nsetopt_lvl_ip_sendsrcaddr(ErlNifEnv*        env,
                                               SocketDescriptor* descP,
                                               ERL_NIF_TERM      eVal);
#endif
#if defined(IP_TOS)
static ERL_NIF_TERM nsetopt_lvl_ip_tos(ErlNifEnv*        env,
                                       SocketDescriptor* descP,
                                       ERL_NIF_TERM      eVal);
#endif
#if defined(IP_TRANSPARENT)
static ERL_NIF_TERM nsetopt_lvl_ip_transparent(ErlNifEnv*        env,
                                               SocketDescriptor* descP,
                                               ERL_NIF_TERM      eVal);
#endif
#if defined(IP_TTL)
static ERL_NIF_TERM nsetopt_lvl_ip_ttl(ErlNifEnv*        env,
                                       SocketDescriptor* descP,
                                       ERL_NIF_TERM      eVal);
#endif
#if defined(IP_UNBLOCK_SOURCE)
static ERL_NIF_TERM nsetopt_lvl_ip_unblock_source(ErlNifEnv*        env,
                                                  SocketDescriptor* descP,
                                                  ERL_NIF_TERM      eVal);
#endif

#if defined(IP_DROP_MEMBERSHIP) || defined(IP_ADD_MEMBERSHIP)
static
ERL_NIF_TERM nsetopt_lvl_ip_update_membership(ErlNifEnv*        env,
                                                  SocketDescriptor* descP,
                                              ERL_NIF_TERM      eVal,
                                              int               opt);
#endif
#if defined(IP_ADD_SOURCE_MEMBERSHIP) || defined(IP_DROP_SOURCE_MEMBERSHIP) || defined(IP_BLOCK_SOURCE) || defined(IP_UNBLOCK_SOURCE)
static
ERL_NIF_TERM nsetopt_lvl_ip_update_source(ErlNifEnv*        env,
                                              SocketDescriptor* descP,
                                              ERL_NIF_TERM      eVal,
                                              int               opt);
#endif


/* *** Handling set of socket options for level = ipv6 *** */
#if defined(SOL_IPV6)
static ERL_NIF_TERM nsetopt_lvl_ipv6(ErlNifEnv*        env,
                                     SocketDescriptor* descP,
                                     int               eOpt,
                                     ERL_NIF_TERM      eVal);
#if defined(IPV6_ADDRFORM)
static ERL_NIF_TERM nsetopt_lvl_ipv6_addrform(ErlNifEnv*        env,
                                              SocketDescriptor* descP,
                                              ERL_NIF_TERM      eVal);
#endif
#if defined(IPV6_ADD_MEMBERSHIP)
static ERL_NIF_TERM nsetopt_lvl_ipv6_add_membership(ErlNifEnv*        env,
                                                    SocketDescriptor* descP,
                                                    ERL_NIF_TERM      eVal);
#endif
#if defined(IPV6_AUTHHDR)
static ERL_NIF_TERM nsetopt_lvl_ipv6_authhdr(ErlNifEnv*        env,
                                             SocketDescriptor* descP,
                                             ERL_NIF_TERM      eVal);
#endif
#if defined(IPV6_DROP_MEMBERSHIP)
static ERL_NIF_TERM nsetopt_lvl_ipv6_drop_membership(ErlNifEnv*        env,
                                                     SocketDescriptor* descP,
                                                     ERL_NIF_TERM      eVal);
#endif
#if defined(IPV6_DSTOPTS)
static ERL_NIF_TERM nsetopt_lvl_ipv6_dstopts(ErlNifEnv*        env,
                                             SocketDescriptor* descP,
                                             ERL_NIF_TERM      eVal);
#endif
#if defined(IPV6_FLOWINFO)
static ERL_NIF_TERM nsetopt_lvl_ipv6_flowinfo(ErlNifEnv*        env,
                                              SocketDescriptor* descP,
                                              ERL_NIF_TERM      eVal);
#endif
#if defined(IPV6_HOPLIMIT)
static ERL_NIF_TERM nsetopt_lvl_ipv6_hoplimit(ErlNifEnv*        env,
                                              SocketDescriptor* descP,
                                              ERL_NIF_TERM      eVal);
#endif
#if defined(IPV6_HOPOPTS)
static ERL_NIF_TERM nsetopt_lvl_ipv6_hopopts(ErlNifEnv*        env,
                                             SocketDescriptor* descP,
                                             ERL_NIF_TERM      eVal);
#endif
#if defined(IPV6_MTU)
static ERL_NIF_TERM nsetopt_lvl_ipv6_mtu(ErlNifEnv*        env,
                                         SocketDescriptor* descP,
                                         ERL_NIF_TERM      eVal);
#endif
#if defined(IPV6_MTU_DISCOVER)
static ERL_NIF_TERM nsetopt_lvl_ipv6_mtu_discover(ErlNifEnv*        env,
                                                  SocketDescriptor* descP,
                                                  ERL_NIF_TERM      eVal);
#endif
#if defined(IPV6_MULTICAST_HOPS)
static ERL_NIF_TERM nsetopt_lvl_ipv6_multicast_hops(ErlNifEnv*        env,
                                                    SocketDescriptor* descP,
                                                    ERL_NIF_TERM      eVal);
#endif
#if defined(IPV6_MULTICAST_IF)
static ERL_NIF_TERM nsetopt_lvl_ipv6_multicast_if(ErlNifEnv*        env,
                                                  SocketDescriptor* descP,
                                                  ERL_NIF_TERM      eVal);
#endif
#if defined(IPV6_MULTICAST_LOOP)
static ERL_NIF_TERM nsetopt_lvl_ipv6_multicast_loop(ErlNifEnv*        env,
                                                    SocketDescriptor* descP,
                                                    ERL_NIF_TERM      eVal);
#endif
#if defined(IPV6_RECVERR)
static ERL_NIF_TERM nsetopt_lvl_ipv6_recverr(ErlNifEnv*        env,
                                             SocketDescriptor* descP,
                                             ERL_NIF_TERM      eVal);
#endif
#if defined(IPV6_RECVPKTINFO) || defined(IPV6_PKTINFO)
static ERL_NIF_TERM nsetopt_lvl_ipv6_recvpktinfo(ErlNifEnv*        env,
                                                 SocketDescriptor* descP,
                                                 ERL_NIF_TERM      eVal);
#endif
#if defined(IPV6_ROUTER_ALERT)
static ERL_NIF_TERM nsetopt_lvl_ipv6_router_alert(ErlNifEnv*        env,
                                                  SocketDescriptor* descP,
                                                  ERL_NIF_TERM      eVal);
#endif
#if defined(IPV6_RTHDR)
static ERL_NIF_TERM nsetopt_lvl_ipv6_rthdr(ErlNifEnv*        env,
                                           SocketDescriptor* descP,
                                           ERL_NIF_TERM      eVal);
#endif
#if defined(IPV6_UNICAST_HOPS)
static ERL_NIF_TERM nsetopt_lvl_ipv6_unicast_hops(ErlNifEnv*        env,
                                                  SocketDescriptor* descP,
                                                  ERL_NIF_TERM      eVal);
#endif
#if defined(IPV6_V6ONLY)
static ERL_NIF_TERM nsetopt_lvl_ipv6_v6only(ErlNifEnv*        env,
                                            SocketDescriptor* descP,
                                            ERL_NIF_TERM      eVal);
#endif

#if defined(IPV6_ADD_MEMBERSHIP) || defined(IPV6_DROP_MEMBERSHIP)
static ERL_NIF_TERM nsetopt_lvl_ipv6_update_membership(ErlNifEnv*        env,
                                                       SocketDescriptor* descP,
                                                       ERL_NIF_TERM      eVal,
                                                       int               opt);
#endif

#endif // defined(SOL_IPV6)
static ERL_NIF_TERM nsetopt_lvl_tcp(ErlNifEnv*        env,
                                    SocketDescriptor* descP,
                                    int               eOpt,
                                    ERL_NIF_TERM      eVal);
#if defined(TCP_CONGESTION)
static ERL_NIF_TERM nsetopt_lvl_tcp_congestion(ErlNifEnv*        env,
                                               SocketDescriptor* descP,
                                               ERL_NIF_TERM      eVal);
#endif
#if defined(TCP_MAXSEG)
static ERL_NIF_TERM nsetopt_lvl_tcp_maxseg(ErlNifEnv*        env,
                                           SocketDescriptor* descP,
                                           ERL_NIF_TERM      eVal);
#endif
#if defined(TCP_NODELAY)
static ERL_NIF_TERM nsetopt_lvl_tcp_nodelay(ErlNifEnv*        env,
                                            SocketDescriptor* descP,
                                            ERL_NIF_TERM      eVal);
#endif
static ERL_NIF_TERM nsetopt_lvl_udp(ErlNifEnv*        env,
                                    SocketDescriptor* descP,
                                    int               eOpt,
                                    ERL_NIF_TERM      eVal);
#if defined(UDP_CORK)
static ERL_NIF_TERM nsetopt_lvl_udp_cork(ErlNifEnv*        env,
                                         SocketDescriptor* descP,
                                         ERL_NIF_TERM      eVal);
#endif
#if defined(HAVE_SCTP)
static ERL_NIF_TERM nsetopt_lvl_sctp(ErlNifEnv*        env,
                                     SocketDescriptor* descP,
                                     int               eOpt,
                                     ERL_NIF_TERM      eVal);
#if defined(SCTP_ASSOCINFO)
static ERL_NIF_TERM nsetopt_lvl_sctp_associnfo(ErlNifEnv*        env,
                                               SocketDescriptor* descP,
                                               ERL_NIF_TERM      eVal);
#endif
#if defined(SCTP_AUTOCLOSE)
static ERL_NIF_TERM nsetopt_lvl_sctp_autoclose(ErlNifEnv*        env,
                                               SocketDescriptor* descP,
                                               ERL_NIF_TERM      eVal);
#endif
#if defined(SCTP_DISABLE_FRAGMENTS)
static ERL_NIF_TERM nsetopt_lvl_sctp_disable_fragments(ErlNifEnv*        env,
                                                       SocketDescriptor* descP,
                                                       ERL_NIF_TERM      eVal);
#endif
#if defined(SCTP_EVENTS)
static ERL_NIF_TERM nsetopt_lvl_sctp_events(ErlNifEnv*        env,
                                            SocketDescriptor* descP,
                                            ERL_NIF_TERM      eVal);
#endif
#if defined(SCTP_INITMSG)
static ERL_NIF_TERM nsetopt_lvl_sctp_initmsg(ErlNifEnv*        env,
                                             SocketDescriptor* descP,
                                             ERL_NIF_TERM      eVal);
#endif
#if defined(SCTP_MAXSEG)
static ERL_NIF_TERM nsetopt_lvl_sctp_maxseg(ErlNifEnv*        env,
                                            SocketDescriptor* descP,
                                            ERL_NIF_TERM      eVal);
#endif
#if defined(SCTP_NODELAY)
static ERL_NIF_TERM nsetopt_lvl_sctp_nodelay(ErlNifEnv*        env,
                                             SocketDescriptor* descP,
                                             ERL_NIF_TERM      eVal);
#endif
#if defined(SCTP_RTOINFO)
static ERL_NIF_TERM nsetopt_lvl_sctp_rtoinfo(ErlNifEnv*        env,
                                             SocketDescriptor* descP,
                                             ERL_NIF_TERM      eVal);
#endif
#endif // defined(HAVE_SCTP)

static ERL_NIF_TERM ngetopt(ErlNifEnv*        env,
                            SocketDescriptor* descP,
                            BOOLEAN_T         isEncoded,
                            BOOLEAN_T         isOTP,
                            int               level,
                            ERL_NIF_TERM      eOpt);
static ERL_NIF_TERM ngetopt_otp(ErlNifEnv*        env,
                                SocketDescriptor* descP,
                                int               eOpt);
static ERL_NIF_TERM ngetopt_otp_debug(ErlNifEnv*        env,
                                      SocketDescriptor* descP);
static ERL_NIF_TERM ngetopt_otp_iow(ErlNifEnv*        env,
                                    SocketDescriptor* descP);
static ERL_NIF_TERM ngetopt_otp_ctrl_proc(ErlNifEnv*        env,
                                          SocketDescriptor* descP);
static ERL_NIF_TERM ngetopt_otp_rcvbuf(ErlNifEnv*        env,
                                       SocketDescriptor* descP);
static ERL_NIF_TERM ngetopt_otp_rcvctrlbuf(ErlNifEnv*        env,
                                           SocketDescriptor* descP);
static ERL_NIF_TERM ngetopt_otp_sndctrlbuf(ErlNifEnv*        env,
                                           SocketDescriptor* descP);
static ERL_NIF_TERM ngetopt_native(ErlNifEnv*        env,
                                   SocketDescriptor* descP,
                                   int               level,
                                   ERL_NIF_TERM      eOpt);
static ERL_NIF_TERM ngetopt_native_unspec(ErlNifEnv*        env,
                                          SocketDescriptor* descP,
                                          int               level,
                                          int               opt,
                                          SOCKOPTLEN_T      valueSz);
static ERL_NIF_TERM ngetopt_level(ErlNifEnv*        env,
                                  SocketDescriptor* descP,
                                  int               level,
                                  int               eOpt);
static ERL_NIF_TERM ngetopt_lvl_socket(ErlNifEnv*        env,
                                       SocketDescriptor* descP,
                                       int               eOpt);
#if defined(SO_ACCEPTCONN)
static ERL_NIF_TERM ngetopt_lvl_sock_acceptconn(ErlNifEnv*        env,
                                                SocketDescriptor* descP);
#endif
#if defined(SO_BINDTODEVICE)
static ERL_NIF_TERM ngetopt_lvl_sock_bindtodevice(ErlNifEnv*        env,
                                                  SocketDescriptor* descP);
#endif
#if defined(SO_BROADCAST)
static ERL_NIF_TERM ngetopt_lvl_sock_broadcast(ErlNifEnv*        env,
                                               SocketDescriptor* descP);
#endif
#if defined(SO_DEBUG)
static ERL_NIF_TERM ngetopt_lvl_sock_debug(ErlNifEnv*        env,
                                           SocketDescriptor* descP);
#endif
#if defined(SO_DOMAIN)
static ERL_NIF_TERM ngetopt_lvl_sock_domain(ErlNifEnv*        env,
                                            SocketDescriptor* descP);
#endif
#if defined(SO_DONTROUTE)
static ERL_NIF_TERM ngetopt_lvl_sock_dontroute(ErlNifEnv*        env,
                                               SocketDescriptor* descP);
#endif
#if defined(SO_KEEPALIVE)
static ERL_NIF_TERM ngetopt_lvl_sock_keepalive(ErlNifEnv*        env,
                                               SocketDescriptor* descP);
#endif
#if defined(SO_LINGER)
static ERL_NIF_TERM ngetopt_lvl_sock_linger(ErlNifEnv*        env,
                                            SocketDescriptor* descP);
#endif
#if defined(SO_OOBINLINE)
static ERL_NIF_TERM ngetopt_lvl_sock_oobinline(ErlNifEnv*        env,
                                               SocketDescriptor* descP);
#endif
#if defined(SO_PEEK_OFF)
static ERL_NIF_TERM ngetopt_lvl_sock_peek_off(ErlNifEnv*        env,
                                              SocketDescriptor* descP);
#endif
#if defined(SO_PRIORITY)
static ERL_NIF_TERM ngetopt_lvl_sock_priority(ErlNifEnv*        env,
                                              SocketDescriptor* descP);
#endif
#if defined(SO_PROTOCOL)
static ERL_NIF_TERM ngetopt_lvl_sock_protocol(ErlNifEnv*        env,
                                              SocketDescriptor* descP);
#endif
#if defined(SO_RCVBUF)
static ERL_NIF_TERM ngetopt_lvl_sock_rcvbuf(ErlNifEnv*        env,
                                            SocketDescriptor* descP);
#endif
#if defined(SO_RCVLOWAT)
static ERL_NIF_TERM ngetopt_lvl_sock_rcvlowat(ErlNifEnv*        env,
                                              SocketDescriptor* descP);
#endif
#if defined(SO_RCVTIMEO)
static ERL_NIF_TERM ngetopt_lvl_sock_rcvtimeo(ErlNifEnv*        env,
                                              SocketDescriptor* descP);
#endif
#if defined(SO_REUSEADDR)
static ERL_NIF_TERM ngetopt_lvl_sock_reuseaddr(ErlNifEnv*        env,
                                               SocketDescriptor* descP);
#endif
#if defined(SO_REUSEPORT)
static ERL_NIF_TERM ngetopt_lvl_sock_reuseport(ErlNifEnv*        env,
                                               SocketDescriptor* descP);
#endif
#if defined(SO_SNDBUF)
static ERL_NIF_TERM ngetopt_lvl_sock_sndbuf(ErlNifEnv*        env,
                                            SocketDescriptor* descP);
#endif
#if defined(SO_SNDLOWAT)
static ERL_NIF_TERM ngetopt_lvl_sock_sndlowat(ErlNifEnv*        env,
                                              SocketDescriptor* descP);
#endif
#if defined(SO_SNDTIMEO)
static ERL_NIF_TERM ngetopt_lvl_sock_sndtimeo(ErlNifEnv*        env,
                                              SocketDescriptor* descP);
#endif
#if defined(SO_TIMESTAMP)
static ERL_NIF_TERM ngetopt_lvl_sock_timestamp(ErlNifEnv*        env,
                                               SocketDescriptor* descP);
#endif
#if defined(SO_TYPE)
static ERL_NIF_TERM ngetopt_lvl_sock_type(ErlNifEnv*        env,
                                          SocketDescriptor* descP);
#endif
static ERL_NIF_TERM ngetopt_lvl_ip(ErlNifEnv*        env,
                                   SocketDescriptor* descP,
                                   int               eOpt);
#if defined(IP_FREEBIND)
static ERL_NIF_TERM ngetopt_lvl_ip_freebind(ErlNifEnv*        env,
                                            SocketDescriptor* descP);
#endif
#if defined(IP_HDRINCL)
static ERL_NIF_TERM ngetopt_lvl_ip_hdrincl(ErlNifEnv*        env,
                                           SocketDescriptor* descP);
#endif
#if defined(IP_MINTTL)
static ERL_NIF_TERM ngetopt_lvl_ip_minttl(ErlNifEnv*        env,
                                          SocketDescriptor* descP);
#endif
#if defined(IP_MTU)
static ERL_NIF_TERM ngetopt_lvl_ip_mtu(ErlNifEnv*        env,
                                       SocketDescriptor* descP);
#endif
#if defined(IP_MTU_DISCOVER)
static ERL_NIF_TERM ngetopt_lvl_ip_mtu_discover(ErlNifEnv*        env,
                                                SocketDescriptor* descP);
#endif
#if defined(IP_MULTICAST_ALL)
static ERL_NIF_TERM ngetopt_lvl_ip_multicast_all(ErlNifEnv*        env,
                                                 SocketDescriptor* descP);
#endif
#if defined(IP_MULTICAST_IF)
static ERL_NIF_TERM ngetopt_lvl_ip_multicast_if(ErlNifEnv*        env,
                                                SocketDescriptor* descP);
#endif
#if defined(IP_MULTICAST_LOOP)
static ERL_NIF_TERM ngetopt_lvl_ip_multicast_loop(ErlNifEnv*        env,
                                                  SocketDescriptor* descP);
#endif
#if defined(IP_MULTICAST_TTL)
static ERL_NIF_TERM ngetopt_lvl_ip_multicast_ttl(ErlNifEnv*        env,
                                                 SocketDescriptor* descP);
#endif
#if defined(IP_NODEFRAG)
static ERL_NIF_TERM ngetopt_lvl_ip_nodefrag(ErlNifEnv*        env,
                                            SocketDescriptor* descP);
#endif
#if defined(IP_PKTINFO)
static ERL_NIF_TERM ngetopt_lvl_ip_pktinfo(ErlNifEnv*        env,
                                           SocketDescriptor* descP);
#endif
#if defined(IP_RECVDSTADDR)
static ERL_NIF_TERM ngetopt_lvl_ip_recvdstaddr(ErlNifEnv*        env,
                                               SocketDescriptor* descP);
#endif
#if defined(IP_RECVERR)
static ERL_NIF_TERM ngetopt_lvl_ip_recverr(ErlNifEnv*        env,
                                           SocketDescriptor* descP);
#endif
#if defined(IP_RECVIF)
static ERL_NIF_TERM ngetopt_lvl_ip_recvif(ErlNifEnv*        env,
                                          SocketDescriptor* descP);
#endif
#if defined(IP_RECVOPTS)
static ERL_NIF_TERM ngetopt_lvl_ip_recvopts(ErlNifEnv*        env,
                                            SocketDescriptor* descP);
#endif
#if defined(IP_RECVORIGDSTADDR)
static ERL_NIF_TERM ngetopt_lvl_ip_recvorigdstaddr(ErlNifEnv*        env,
                                                   SocketDescriptor* descP);
#endif
#if defined(IP_RECVTOS)
static ERL_NIF_TERM ngetopt_lvl_ip_recvtos(ErlNifEnv*        env,
                                           SocketDescriptor* descP);
#endif
#if defined(IP_RECVTTL)
static ERL_NIF_TERM ngetopt_lvl_ip_recvttl(ErlNifEnv*        env,
                                           SocketDescriptor* descP);
#endif
#if defined(IP_RETOPTS)
static ERL_NIF_TERM ngetopt_lvl_ip_retopts(ErlNifEnv*        env,
                                           SocketDescriptor* descP);
#endif
#if defined(IP_ROUTER_ALERT)
static ERL_NIF_TERM ngetopt_lvl_ip_router_alert(ErlNifEnv*        env,
                                                SocketDescriptor* descP);
#endif
#if defined(IP_SENDSRCADDR)
static ERL_NIF_TERM ngetopt_lvl_ip_sendsrcaddr(ErlNifEnv*        env,
                                               SocketDescriptor* descP);
#endif
#if defined(IP_TOS)
static ERL_NIF_TERM ngetopt_lvl_ip_tos(ErlNifEnv*        env,
                                       SocketDescriptor* descP);
#endif
#if defined(IP_TRANSPARENT)
static ERL_NIF_TERM ngetopt_lvl_ip_transparent(ErlNifEnv*        env,
                                               SocketDescriptor* descP);
#endif
#if defined(IP_TTL)
static ERL_NIF_TERM ngetopt_lvl_ip_ttl(ErlNifEnv*        env,
                                       SocketDescriptor* descP);
#endif
#if defined(SOL_IPV6)
static ERL_NIF_TERM ngetopt_lvl_ipv6(ErlNifEnv*        env,
                                     SocketDescriptor* descP,
                                     int               eOpt);
#if defined(IPV6_AUTHHDR)
static ERL_NIF_TERM ngetopt_lvl_ipv6_authhdr(ErlNifEnv*        env,
                                             SocketDescriptor* descP);
#endif
#if defined(IPV6_DSTOPTS)
static ERL_NIF_TERM ngetopt_lvl_ipv6_dstopts(ErlNifEnv*        env,
                                             SocketDescriptor* descP);
#endif
#if defined(IPV6_FLOWINFO)
static ERL_NIF_TERM ngetopt_lvl_ipv6_flowinfo(ErlNifEnv*        env,
                                              SocketDescriptor* descP);
#endif
#if defined(IPV6_HOPLIMIT)
static ERL_NIF_TERM ngetopt_lvl_ipv6_hoplimit(ErlNifEnv*        env,
                                              SocketDescriptor* descP);
#endif
#if defined(IPV6_HOPOPTS)
static ERL_NIF_TERM ngetopt_lvl_ipv6_hopopts(ErlNifEnv*        env,
                                             SocketDescriptor* descP);
#endif
#if defined(IPV6_MTU)
static ERL_NIF_TERM ngetopt_lvl_ipv6_mtu(ErlNifEnv*        env,
                                         SocketDescriptor* descP);
#endif
#if defined(IPV6_MTU_DISCOVER)
static ERL_NIF_TERM ngetopt_lvl_ipv6_mtu_discover(ErlNifEnv*        env,
                                                  SocketDescriptor* descP);
#endif
#if defined(IPV6_MULTICAST_HOPS)
static ERL_NIF_TERM ngetopt_lvl_ipv6_multicast_hops(ErlNifEnv*        env,
                                                    SocketDescriptor* descP);
#endif
#if defined(IPV6_MULTICAST_IF)
static ERL_NIF_TERM ngetopt_lvl_ipv6_multicast_if(ErlNifEnv*        env,
                                                  SocketDescriptor* descP);
#endif
#if defined(IPV6_MULTICAST_LOOP)
static ERL_NIF_TERM ngetopt_lvl_ipv6_multicast_loop(ErlNifEnv*        env,
                                                    SocketDescriptor* descP);
#endif
#if defined(IPV6_RECVERR)
static ERL_NIF_TERM ngetopt_lvl_ipv6_recverr(ErlNifEnv*        env,
                                             SocketDescriptor* descP);
#endif
#if defined(IPV6_RECVPKTINFO) || defined(IPV6_PKTINFO)
static ERL_NIF_TERM ngetopt_lvl_ipv6_recvpktinfo(ErlNifEnv*        env,
                                                 SocketDescriptor* descP);
#endif
#if defined(IPV6_ROUTER_ALERT)
static ERL_NIF_TERM ngetopt_lvl_ipv6_router_alert(ErlNifEnv*        env,
                                                  SocketDescriptor* descP);
#endif
#if defined(IPV6_RTHDR)
static ERL_NIF_TERM ngetopt_lvl_ipv6_rthdr(ErlNifEnv*        env,
                                           SocketDescriptor* descP);
#endif
#if defined(IPV6_UNICAST_HOPS)
static ERL_NIF_TERM ngetopt_lvl_ipv6_unicast_hops(ErlNifEnv*        env,
                                                  SocketDescriptor* descP);
#endif
#if defined(IPV6_V6ONLY)
static ERL_NIF_TERM ngetopt_lvl_ipv6_v6only(ErlNifEnv*        env,
                                            SocketDescriptor* descP);
#endif

#endif // defined(SOL_IPV6)

static ERL_NIF_TERM ngetopt_lvl_tcp(ErlNifEnv*        env,
                                    SocketDescriptor* descP,
                                    int               eOpt);
#if defined(TCP_CONGESTION)
static ERL_NIF_TERM ngetopt_lvl_tcp_congestion(ErlNifEnv*        env,
                                               SocketDescriptor* descP);
#endif
#if defined(TCP_MAXSEG)
static ERL_NIF_TERM ngetopt_lvl_tcp_maxseg(ErlNifEnv*        env,
                                           SocketDescriptor* descP);
#endif
#if defined(TCP_NODELAY)
static ERL_NIF_TERM ngetopt_lvl_tcp_nodelay(ErlNifEnv*        env,
                                            SocketDescriptor* descP);
#endif
static ERL_NIF_TERM ngetopt_lvl_udp(ErlNifEnv*        env,
                                    SocketDescriptor* descP,
                                    int               eOpt);
#if defined(UDP_CORK)
static ERL_NIF_TERM ngetopt_lvl_udp_cork(ErlNifEnv*        env,
                                         SocketDescriptor* descP);
#endif
#if defined(HAVE_SCTP)
static ERL_NIF_TERM ngetopt_lvl_sctp(ErlNifEnv*        env,
                                     SocketDescriptor* descP,
                                     int               eOpt);
#if defined(SCTP_ASSOCINFO)
static ERL_NIF_TERM ngetopt_lvl_sctp_associnfo(ErlNifEnv*        env,
                                               SocketDescriptor* descP);
#endif
#if defined(SCTP_AUTOCLOSE)
static ERL_NIF_TERM ngetopt_lvl_sctp_autoclose(ErlNifEnv*        env,
                                               SocketDescriptor* descP);
#endif
#if defined(SCTP_DISABLE_FRAGMENTS)
static ERL_NIF_TERM ngetopt_lvl_sctp_disable_fragments(ErlNifEnv*        env,
                                                       SocketDescriptor* descP);
#endif
#if defined(SCTP_MAXSEG)
static ERL_NIF_TERM ngetopt_lvl_sctp_maxseg(ErlNifEnv*        env,
                                            SocketDescriptor* descP);
#endif
#if defined(SCTP_INITMSG)
static ERL_NIF_TERM ngetopt_lvl_sctp_initmsg(ErlNifEnv*        env,
                                             SocketDescriptor* descP);
#endif
#if defined(SCTP_NODELAY)
static ERL_NIF_TERM ngetopt_lvl_sctp_nodelay(ErlNifEnv*        env,
                                             SocketDescriptor* descP);
#endif
#if defined(SCTP_RTOINFO)
static ERL_NIF_TERM ngetopt_lvl_sctp_rtoinfo(ErlNifEnv*        env,
                                             SocketDescriptor* descP);
#endif
#endif // defined(HAVE_SCTP)
static ERL_NIF_TERM nsockname(ErlNifEnv*        env,
                              SocketDescriptor* descP);
static ERL_NIF_TERM npeername(ErlNifEnv*        env,
                              SocketDescriptor* descP);
static ERL_NIF_TERM ncancel(ErlNifEnv*        env,
                            SocketDescriptor* descP,
                            ERL_NIF_TERM      op,
                            ERL_NIF_TERM      opRef);
static ERL_NIF_TERM ncancel_connect(ErlNifEnv*        env,
                                    SocketDescriptor* descP,
                                    ERL_NIF_TERM      opRef);
static ERL_NIF_TERM ncancel_accept(ErlNifEnv*        env,
                                   SocketDescriptor* descP,
                                   ERL_NIF_TERM      opRef);
static ERL_NIF_TERM ncancel_accept_current(ErlNifEnv*        env,
                                           SocketDescriptor* descP);
static ERL_NIF_TERM ncancel_accept_waiting(ErlNifEnv*        env,
                                           SocketDescriptor* descP,
                                           ERL_NIF_TERM      opRef);
static ERL_NIF_TERM ncancel_send(ErlNifEnv*        env,
                                 SocketDescriptor* descP,
                                 ERL_NIF_TERM      opRef);
static ERL_NIF_TERM ncancel_send_current(ErlNifEnv*        env,
                                         SocketDescriptor* descP);
static ERL_NIF_TERM ncancel_send_waiting(ErlNifEnv*        env,
                                         SocketDescriptor* descP,
                                         ERL_NIF_TERM      opRef);
static ERL_NIF_TERM ncancel_recv(ErlNifEnv*        env,
                                 SocketDescriptor* descP,
                                 ERL_NIF_TERM      opRef);
static ERL_NIF_TERM ncancel_recv_current(ErlNifEnv*        env,
                                         SocketDescriptor* descP);
static ERL_NIF_TERM ncancel_recv_waiting(ErlNifEnv*        env,
                                         SocketDescriptor* descP,
                                         ERL_NIF_TERM      opRef);
static ERL_NIF_TERM ncancel_read_select(ErlNifEnv*        env,
                                        SocketDescriptor* descP,
                                        ERL_NIF_TERM      opRef);
static ERL_NIF_TERM ncancel_write_select(ErlNifEnv*        env,
                                         SocketDescriptor* descP,
                                         ERL_NIF_TERM      opRef);
static ERL_NIF_TERM ncancel_mode_select(ErlNifEnv*        env,
                                        SocketDescriptor* descP,
                                        ERL_NIF_TERM      opRef,
                                        int               smode,
                                        int               rmode);

static ERL_NIF_TERM nsetopt_str_opt(ErlNifEnv*        env,
                                    SocketDescriptor* descP,
                                    int               level,
                                    int               opt,
                                    int               max,
                                    ERL_NIF_TERM      eVal);
static ERL_NIF_TERM nsetopt_bool_opt(ErlNifEnv*        env,
                                     SocketDescriptor* descP,
                                     int               level,
                                     int               opt,
                                     ERL_NIF_TERM      eVal);
static ERL_NIF_TERM nsetopt_int_opt(ErlNifEnv*        env,
                                    SocketDescriptor* descP,
                                    int               level,
                                    int               opt,
                                    ERL_NIF_TERM      eVal);
static ERL_NIF_TERM nsetopt_timeval_opt(ErlNifEnv*        env,
                                        SocketDescriptor* descP,
                                        int               level,
                                        int               opt,
                                        ERL_NIF_TERM      eVal);

static ERL_NIF_TERM ngetopt_str_opt(ErlNifEnv*        env,
                                    SocketDescriptor* descP,
                                    int               level,
                                    int               opt,
                                    int               max);
static ERL_NIF_TERM ngetopt_bool_opt(ErlNifEnv*        env,
                                     SocketDescriptor* descP,
                                     int               level,
                                     int               opt);
static ERL_NIF_TERM ngetopt_int_opt(ErlNifEnv*        env,
                                    SocketDescriptor* descP,
                                    int               level,
                                    int               opt);
static ERL_NIF_TERM ngetopt_timeval_opt(ErlNifEnv*        env,
                                        SocketDescriptor* descP,
                                        int               level,
                                        int               opt);

static BOOLEAN_T send_check_writer(ErlNifEnv*        env,
                                   SocketDescriptor* descP,
                                   ERL_NIF_TERM      ref,
                                   ERL_NIF_TERM*     checkResult);
static ERL_NIF_TERM send_check_result(ErlNifEnv*        env,
                                      SocketDescriptor* descP,
                                      ssize_t           written,
                                      ssize_t           dataSize,
                                      int               saveErrno,
                                      ERL_NIF_TERM      sendRef);
static BOOLEAN_T recv_check_reader(ErlNifEnv*        env,
                                   SocketDescriptor* descP,
                                   ERL_NIF_TERM      ref,
                                   ERL_NIF_TERM*     checkResult);
static char* recv_init_current_reader(ErlNifEnv*        env,
                                      SocketDescriptor* descP,
                                      ERL_NIF_TERM      ref);
static ERL_NIF_TERM recv_update_current_reader(ErlNifEnv*        env,
                                               SocketDescriptor* descP);
static void recv_error_current_reader(ErlNifEnv*        env,
                                      SocketDescriptor* descP,
                                      ERL_NIF_TERM      reason);
static ERL_NIF_TERM recv_check_result(ErlNifEnv*        env,
                                      SocketDescriptor* descP,
                                      int               read,
                                      int               toRead,
                                      int               saveErrno,
                                      ErlNifBinary*     bufP,
                                      ERL_NIF_TERM      recvRef);
static ERL_NIF_TERM recvfrom_check_result(ErlNifEnv*        env,
                                          SocketDescriptor* descP,
                                          int               read,
                                          int               saveErrno,
                                          ErlNifBinary*     bufP,
                                          SocketAddress*    fromAddrP,
                                          unsigned int      fromAddrLen,
                                          ERL_NIF_TERM      recvRef);
static ERL_NIF_TERM recvmsg_check_result(ErlNifEnv*        env,
                                         SocketDescriptor* descP,
                                         int               read,
                                         int               saveErrno,
                                         struct msghdr*    msgHdrP,
                                         ErlNifBinary*     dataBufP,
                                         ErlNifBinary*     ctrlBufP,
                                         ERL_NIF_TERM      recvRef);

static ERL_NIF_TERM nfinalize_connection(ErlNifEnv*        env,
                                         SocketDescriptor* descP);
static ERL_NIF_TERM nfinalize_close(ErlNifEnv*        env,
                                    SocketDescriptor* descP);

extern char* encode_msghdr(ErlNifEnv*        env,
                           SocketDescriptor* descP,
                           int               read,
                           struct msghdr*    msgHdrP,
                           ErlNifBinary*     dataBufP,
                           ErlNifBinary*     ctrlBufP,
                           ERL_NIF_TERM*     eSockAddr);
extern char* encode_cmsghdrs(ErlNifEnv*        env,
                             SocketDescriptor* descP,
                             ErlNifBinary*     cmsgBinP,
                             struct msghdr*    msgHdrP,
                             ERL_NIF_TERM*     eCMsgHdr);
extern char* decode_cmsghdrs(ErlNifEnv*        env,
                             SocketDescriptor* descP,
                             ERL_NIF_TERM      eCMsgHdr,
                             char*             cmsgHdrBufP,
                             size_t            cmsgHdrBufLen,
                             size_t*           cmsgHdrBufUsed);
extern char* decode_cmsghdr(ErlNifEnv*        env,
                            SocketDescriptor* descP,
                            ERL_NIF_TERM      eCMsgHdr,
                            char*             bufP,
                            size_t            rem,
                            size_t*           used);
static char* encode_cmsghdr_level(ErlNifEnv*    env,
                                  int           level,
                                  ERL_NIF_TERM* eLevel);
static char* decode_cmsghdr_level(ErlNifEnv*   env,
                                  ERL_NIF_TERM eLevel,
                                  int*         level);
static char* encode_cmsghdr_type(ErlNifEnv*    env,
                                 int           level,
                                 int           type,
                                 ERL_NIF_TERM* eType);
static char* decode_cmsghdr_type(ErlNifEnv*   env,
                                 int          level,
                                 ERL_NIF_TERM eType,
                                 int*         type);
static char* encode_cmsghdr_data(ErlNifEnv*     env,
                                 ERL_NIF_TERM   ctrlBuf,
                                 int            level,
                                 int            type,
                                 unsigned char* dataP,
                                 size_t         dataPos,
                                 size_t         dataLen,
                                 ERL_NIF_TERM*  eCMsgHdrData);
static char* encode_cmsghdr_data_socket(ErlNifEnv*     env,
                                        ERL_NIF_TERM   ctrlBuf,
                                        int            type,
                                        unsigned char* dataP,
                                        size_t         dataPos,
                                        size_t         dataLen,
                                        ERL_NIF_TERM*  eCMsgHdrData);
static char* encode_cmsghdr_data_ip(ErlNifEnv*     env,
                                    ERL_NIF_TERM   ctrlBuf,
                                    int            type,
                                    unsigned char* dataP,
                                    size_t         dataPos,
                                    size_t         dataLen,
                                    ERL_NIF_TERM*  eCMsgHdrData);
#if defined(SOL_IPV6)
static char* encode_cmsghdr_data_ipv6(ErlNifEnv*     env,
                                      ERL_NIF_TERM   ctrlBuf,
                                      int            type,
                                      unsigned char* dataP,
                                      size_t         dataPos,
                                      size_t         dataLen,
                                      ERL_NIF_TERM*  eCMsgHdrData);
#endif
extern char* encode_msghdr_flags(ErlNifEnv*        env,
                                 SocketDescriptor* descP,
                                 int               msgFlags,
                                 ERL_NIF_TERM*     flags);
static char* decode_cmsghdr_data(ErlNifEnv*        env,
                                 SocketDescriptor* descP,
                                 char*             bufP,
                                 size_t            rem,
                                 int               level,
                                 int               type,
                                 ERL_NIF_TERM      eData,
                                 size_t*           used);
static char* decode_cmsghdr_final(SocketDescriptor* descP,
                                  char*             bufP,
                                  size_t            rem,
                                  int               level,
                                  int               type,
                                  char*             data,
                                  int               sz,
                                  size_t*           used);
static BOOLEAN_T decode_sock_linger(ErlNifEnv*     env,
                                    ERL_NIF_TERM   eVal,
                                    struct linger* valP);
#if defined(IP_TOS)
static BOOLEAN_T decode_ip_tos(ErlNifEnv*   env,
                               ERL_NIF_TERM eVal,
                               int*         val);
#endif
#if defined(IP_MTU_DISCOVER)
static char* decode_ip_pmtudisc(ErlNifEnv*   env,
                                ERL_NIF_TERM eVal,
                                int*         val);
#endif
#if defined(IP_MTU_DISCOVER)
static void encode_ip_pmtudisc(ErlNifEnv*    env,
                               int           val,
                               ERL_NIF_TERM* eVal);
#endif
#if defined(IPV6_MTU_DISCOVER)
static char* decode_ipv6_pmtudisc(ErlNifEnv*   env,
                                  ERL_NIF_TERM eVal,
                                  int*         val);
#endif
#if defined(IPV6_MTU_DISCOVER)
static void encode_ipv6_pmtudisc(ErlNifEnv*    env,
                                 int           val,
                                 ERL_NIF_TERM* eVal);
#endif

/*
static BOOLEAN_T decode_bool(ErlNifEnv*   env,
                             ERL_NIF_TERM eVal,
                             BOOLEAN_T*   val);
*/
static BOOLEAN_T decode_native_get_opt(ErlNifEnv*   env,
                                       ERL_NIF_TERM eVal,
                                       int*         opt,
                                       uint16_t*    valueType,
                                       int*         valueSz);
// static void encode_bool(BOOLEAN_T val, ERL_NIF_TERM* eVal);
static ERL_NIF_TERM encode_ip_tos(ErlNifEnv* env, int val);

static void inform_waiting_procs(ErlNifEnv*          env,
                                 SocketDescriptor*   descP,
                                 SocketRequestQueue* q,
                                 BOOLEAN_T           free,
                                 ERL_NIF_TERM        reason);

static int socket_setopt(int             sock,
                         int             level,
                         int             opt,
                         const void*     optVal,
                         const socklen_t optLen);

static BOOLEAN_T verify_is_connected(SocketDescriptor* descP, int* err);

static SocketDescriptor* alloc_descriptor(SOCKET sock, HANDLE event);

static int compare_pids(ErlNifEnv*       env,
                        const ErlNifPid* pid1,
                        const ErlNifPid* pid2);



static BOOLEAN_T edomain2domain(int edomain, int* domain);
static BOOLEAN_T etype2type(int etype, int* type);
static BOOLEAN_T eproto2proto(ErlNifEnv*         env,
                              const ERL_NIF_TERM eproto,
                              int*               proto);
static BOOLEAN_T ehow2how(unsigned int ehow, int* how);
static BOOLEAN_T esendflags2sendflags(unsigned int esendflags, int* sendflags);
static BOOLEAN_T erecvflags2recvflags(unsigned int erecvflags, int* recvflags);
static BOOLEAN_T elevel2level(BOOLEAN_T  isEncoded,
                              int        eLevel,
                              BOOLEAN_T* isOTP,
                              int*       level);
#ifdef HAVE_SETNS
static BOOLEAN_T emap2netns(ErlNifEnv* env, ERL_NIF_TERM map, char** netns);
static BOOLEAN_T change_network_namespace(char* netns, int* cns, int* err);
static BOOLEAN_T restore_network_namespace(int ns, SOCKET sock, int* err);
#endif

static BOOLEAN_T cnt_inc(uint32_t* cnt, uint32_t inc);
static void      cnt_dec(uint32_t* cnt, uint32_t dec);

static void inc_socket(int domain, int type, int protocol);
static void dec_socket(int domain, int type, int protocol);


static BOOLEAN_T acceptor_search4pid(ErlNifEnv*        env,
                                     SocketDescriptor* descP,
                                     ErlNifPid*        pid);
static ERL_NIF_TERM acceptor_push(ErlNifEnv*        env,
                                  SocketDescriptor* descP,
                                  ErlNifPid         pid,
                                  ERL_NIF_TERM      ref);
static BOOLEAN_T acceptor_pop(ErlNifEnv*        env,
                              SocketDescriptor* descP,
                              ErlNifPid*        pid,
                              ErlNifMonitor*    mon,
                              ERL_NIF_TERM*     ref);
static BOOLEAN_T acceptor_unqueue(ErlNifEnv*        env,
                                  SocketDescriptor* descP,
                                  const ErlNifPid*  pid);

static BOOLEAN_T writer_search4pid(ErlNifEnv*        env,
                                   SocketDescriptor* descP,
                                   ErlNifPid*        pid);
static ERL_NIF_TERM writer_push(ErlNifEnv*        env,
                                SocketDescriptor* descP,
                                ErlNifPid         pid,
                                ERL_NIF_TERM      ref);
static BOOLEAN_T writer_pop(ErlNifEnv*        env,
                            SocketDescriptor* descP,
                            ErlNifPid*        pid,
                            ErlNifMonitor*    mon,
                            ERL_NIF_TERM*     ref);
static BOOLEAN_T writer_unqueue(ErlNifEnv*        env,
                                SocketDescriptor* descP,
                                const ErlNifPid*  pid);

static BOOLEAN_T reader_search4pid(ErlNifEnv*        env,
                                   SocketDescriptor* descP,
                                   ErlNifPid*        pid);
static ERL_NIF_TERM reader_push(ErlNifEnv*        env,
                                SocketDescriptor* descP,
                                ErlNifPid         pid,
                                ERL_NIF_TERM      ref);
static BOOLEAN_T reader_pop(ErlNifEnv*        env,
                            SocketDescriptor* descP,
                            ErlNifPid*        pid,
                            ErlNifMonitor*    mon,
                            ERL_NIF_TERM*     ref);
static BOOLEAN_T reader_unqueue(ErlNifEnv*        env,
                                SocketDescriptor* descP,
                                const ErlNifPid*  pid);

static BOOLEAN_T qsearch4pid(ErlNifEnv*          env,
                             SocketRequestQueue* q,
                             ErlNifPid*          pid);
static void qpush(SocketRequestQueue*        q,
                  SocketRequestQueueElement* e);
static SocketRequestQueueElement* qpop(SocketRequestQueue* q);
static BOOLEAN_T qunqueue(ErlNifEnv*          env,
                          SocketRequestQueue* q,
                          const ErlNifPid*    pid);
/*
#if defined(HAVE_SYS_UN_H) || defined(SO_BINDTODEVICE)
static size_t my_strnlen(const char *s, size_t maxlen);
#endif
*/

static void socket_dtor(ErlNifEnv* env, void* obj);
static void socket_stop(ErlNifEnv* env,
			void*      obj,
			int        fd,
			int        is_direct_call);
static void socket_down(ErlNifEnv*           env,
			void*                obj,
			const ErlNifPid*     pid,
			const ErlNifMonitor* mon);
static void socket_down_acceptor(ErlNifEnv*        env,
                                 SocketDescriptor* descP,
                                 const ErlNifPid*  pid);
static void socket_down_writer(ErlNifEnv*        env,
                               SocketDescriptor* descP,
                               const ErlNifPid*  pid);
static void socket_down_reader(ErlNifEnv*        env,
                               SocketDescriptor* descP,
                               const ErlNifPid*  pid);

/*
static char* send_msg_error_closed(ErlNifEnv*   env,
                                   ErlNifPid*   pid);
*/
/*
static char* send_msg_error(ErlNifEnv*   env,
                            ERL_NIF_TERM reason,
                            ErlNifPid*   pid);
*/
static char* send_msg_nif_abort(ErlNifEnv*   env,
                                ERL_NIF_TERM ref,
                                ERL_NIF_TERM reason,
                                ErlNifPid*   pid);
static char* send_msg(ErlNifEnv*   env,
                      ERL_NIF_TERM msg,
                      ErlNifPid*   pid);

static BOOLEAN_T extract_debug(ErlNifEnv*   env,
                               ERL_NIF_TERM map);
static BOOLEAN_T extract_iow(ErlNifEnv*   env,
                             ERL_NIF_TERM map);

static int on_load(ErlNifEnv* env, void** priv_data, ERL_NIF_TERM load_info);


#if HAVE_IN6
#  if ! defined(HAVE_IN6ADDR_ANY) || ! HAVE_IN6ADDR_ANY
#    if HAVE_DECL_IN6ADDR_ANY_INIT
static const struct in6_addr in6addr_any = { { IN6ADDR_ANY_INIT } };
#    else
static const struct in6_addr in6addr_any =
    { { { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 } } };
#    endif /* HAVE_IN6ADDR_ANY_INIT */
#  endif /* ! HAVE_DECL_IN6ADDR_ANY */

#  if ! defined(HAVE_IN6ADDR_LOOPBACK) || ! HAVE_IN6ADDR_LOOPBACK
#    if HAVE_DECL_IN6ADDR_LOOPBACK_INIT
static const struct in6_addr in6addr_loopback =
    { { IN6ADDR_LOOPBACK_INIT } };
#    else
static const struct in6_addr in6addr_loopback =
    { { { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 } } };
#    endif /* HAVE_IN6ADDR_LOOPBACk_INIT */
#  endif /* ! HAVE_DECL_IN6ADDR_LOOPBACK */
#endif /* HAVE_IN6 */



/* *** String constants *** */
static char str_adaptation_layer[] = "adaptation_layer";
static char str_address[]          = "address";
static char str_association[]      = "association";
static char str_assoc_id[]         = "assoc_id";
static char str_authentication[]   = "authentication";
// static char str_any[]              = "any";
static char str_bool[]             = "bool";
static char str_close[]            = "close";
static char str_closed[]           = "closed";
static char str_closing[]          = "closing";
static char str_cookie_life[]      = "cookie_life";
static char str_data_in[]          = "data_in";
static char str_do[]               = "do";
static char str_dont[]             = "dont";
static char str_exclude[]          = "exclude";
static char str_false[]            = "false";
static char str_global_counters[]  = "global_counters";
static char str_in4_sockaddr[]     = "in4_sockaddr";
static char str_in6_sockaddr[]     = "in6_sockaddr";
static char str_include[]          = "include";
static char str_initial[]          = "initial";
static char str_int[]              = "int";
static char str_interface[]        = "interface";
static char str_iow[]              = "iow";
static char str_local_rwnd[]       = "local_rwnd";
// static char str_loopback[]         = "loopback";
static char str_max[]              = "max";
static char str_max_attempts[]     = "max_attempts";
static char str_max_init_timeo[]   = "max_init_timeo";
static char str_max_instreams[]    = "max_instreams";
static char str_max_rxt[]          = "max_rxt";
static char str_min[]              = "min";
static char str_mode[]             = "mode";
static char str_multiaddr[]        = "multiaddr";
static char str_nif_abort[]        = "nif_abort";
static char str_null[]             = "null";
static char str_num_dlocal[]       = "num_domain_local";
static char str_num_dinet[]        = "num_domain_inet";
static char str_num_dinet6[]       = "num_domain_inet6";
static char str_num_outstreams[]   = "num_outstreams";
static char str_num_peer_dests[]   = "num_peer_dests";
static char str_num_pip[]          = "num_proto_ip";
static char str_num_psctp[]        = "num_proto_sctp";
static char str_num_ptcp[]         = "num_proto_tcp";
static char str_num_pudp[]         = "num_proto_udp";
static char str_num_sockets[]      = "num_sockets";
static char str_num_tdgrams[]      = "num_type_dgram";
static char str_num_tseqpkgs[]     = "num_type_seqpacket";
static char str_num_tstreams[]     = "num_type_stream";
static char str_partial_delivery[] = "partial_delivery";
static char str_peer_error[]       = "peer_error";
static char str_peer_rwnd[]        = "peer_rwnd";
static char str_probe[]            = "probe";
static char str_select[]           = "select";
static char str_sender_dry[]       = "sender_dry";
static char str_send_failure[]     = "send_failure";
static char str_shutdown[]         = "shutdown";
static char str_slist[]            = "slist";
static char str_sourceaddr[]       = "sourceaddr";
static char str_timeout[]          = "timeout";
static char str_true[]             = "true";
static char str_want[]             = "want";

/* (special) error string constants */
static char str_eisconn[]        = "eisconn";
static char str_enotclosing[]    = "enotclosing";
static char str_enotconn[]       = "enotconn";
static char str_exalloc[]        = "exalloc";
static char str_exbadstate[]     = "exbadstate";
static char str_exbusy[]         = "exbusy";
static char str_exmon[]          = "exmonitor";  // failed monitor
static char str_exself[]         = "exself";     // failed self
static char str_exsend[]         = "exsend";     // failed send


/* *** "Global" Atoms *** */
ERL_NIF_TERM esock_atom_accept;
ERL_NIF_TERM esock_atom_addr;
ERL_NIF_TERM esock_atom_any;
ERL_NIF_TERM esock_atom_connect;
ERL_NIF_TERM esock_atom_credentials;
ERL_NIF_TERM esock_atom_ctrl;
ERL_NIF_TERM esock_atom_ctrunc;
ERL_NIF_TERM esock_atom_data;
ERL_NIF_TERM esock_atom_dgram;
ERL_NIF_TERM esock_atom_debug;
ERL_NIF_TERM esock_atom_eor;
ERL_NIF_TERM esock_atom_error;
ERL_NIF_TERM esock_atom_errqueue;
ERL_NIF_TERM esock_atom_false;
ERL_NIF_TERM esock_atom_family;
ERL_NIF_TERM esock_atom_flags;
ERL_NIF_TERM esock_atom_flowinfo;
ERL_NIF_TERM esock_atom_ifindex;
ERL_NIF_TERM esock_atom_inet;
ERL_NIF_TERM esock_atom_inet6;
ERL_NIF_TERM esock_atom_iov;
ERL_NIF_TERM esock_atom_ip;
ERL_NIF_TERM esock_atom_ipv6;
ERL_NIF_TERM esock_atom_level;
ERL_NIF_TERM esock_atom_local;
ERL_NIF_TERM esock_atom_loopback;
ERL_NIF_TERM esock_atom_lowdelay;
ERL_NIF_TERM esock_atom_mincost;
ERL_NIF_TERM esock_atom_not_found;
ERL_NIF_TERM esock_atom_ok;
ERL_NIF_TERM esock_atom_oob;
ERL_NIF_TERM esock_atom_origdstaddr;
ERL_NIF_TERM esock_atom_path;
ERL_NIF_TERM esock_atom_pktinfo;
ERL_NIF_TERM esock_atom_port;
ERL_NIF_TERM esock_atom_protocol;
ERL_NIF_TERM esock_atom_raw;
ERL_NIF_TERM esock_atom_rdm;
ERL_NIF_TERM esock_atom_recv;
ERL_NIF_TERM esock_atom_recvfrom;
ERL_NIF_TERM esock_atom_recvmsg;
ERL_NIF_TERM esock_atom_reliability;
ERL_NIF_TERM esock_atom_rights;
ERL_NIF_TERM esock_atom_scope_id;
ERL_NIF_TERM esock_atom_sctp;
ERL_NIF_TERM esock_atom_sec;
ERL_NIF_TERM esock_atom_select_sent;
ERL_NIF_TERM esock_atom_send;
ERL_NIF_TERM esock_atom_sendmsg;
ERL_NIF_TERM esock_atom_sendto;
ERL_NIF_TERM esock_atom_seqpacket;
ERL_NIF_TERM esock_atom_socket;
ERL_NIF_TERM esock_atom_spec_dst;
ERL_NIF_TERM esock_atom_stream;
ERL_NIF_TERM esock_atom_tcp;
ERL_NIF_TERM esock_atom_throughput;
ERL_NIF_TERM esock_atom_timestamp;
ERL_NIF_TERM esock_atom_tos;
ERL_NIF_TERM esock_atom_true;
ERL_NIF_TERM esock_atom_trunc;
ERL_NIF_TERM esock_atom_ttl;
ERL_NIF_TERM esock_atom_type;
ERL_NIF_TERM esock_atom_udp;
ERL_NIF_TERM esock_atom_undefined;
ERL_NIF_TERM esock_atom_unknown;
ERL_NIF_TERM esock_atom_usec;

/* *** "Global" error (=reason) atoms *** */
ERL_NIF_TERM esock_atom_eagain;
ERL_NIF_TERM esock_atom_eafnosupport;
ERL_NIF_TERM esock_atom_einval;

/* *** Atoms *** */
static ERL_NIF_TERM atom_adaptation_layer;
static ERL_NIF_TERM atom_address;
static ERL_NIF_TERM atom_association;
static ERL_NIF_TERM atom_assoc_id;
static ERL_NIF_TERM atom_authentication;
static ERL_NIF_TERM atom_bool;
static ERL_NIF_TERM atom_close;
static ERL_NIF_TERM atom_closed;
static ERL_NIF_TERM atom_closing;
static ERL_NIF_TERM atom_cookie_life;
static ERL_NIF_TERM atom_data_in;
static ERL_NIF_TERM atom_do;
static ERL_NIF_TERM atom_dont;
static ERL_NIF_TERM atom_exclude;
static ERL_NIF_TERM atom_false;
static ERL_NIF_TERM atom_global_counters;
static ERL_NIF_TERM atom_in4_sockaddr;
static ERL_NIF_TERM atom_in6_sockaddr;
static ERL_NIF_TERM atom_include;
static ERL_NIF_TERM atom_initial;
static ERL_NIF_TERM atom_int;
static ERL_NIF_TERM atom_interface;
static ERL_NIF_TERM atom_iow;
static ERL_NIF_TERM atom_local_rwnd;
static ERL_NIF_TERM atom_max;
static ERL_NIF_TERM atom_max_attempts;
static ERL_NIF_TERM atom_max_init_timeo;
static ERL_NIF_TERM atom_max_instreams;
static ERL_NIF_TERM atom_max_rxt;
static ERL_NIF_TERM atom_min;
static ERL_NIF_TERM atom_mode;
static ERL_NIF_TERM atom_multiaddr;
static ERL_NIF_TERM atom_nif_abort;
static ERL_NIF_TERM atom_null;
static ERL_NIF_TERM atom_num_dinet;
static ERL_NIF_TERM atom_num_dinet6;
static ERL_NIF_TERM atom_num_dlocal;
static ERL_NIF_TERM atom_num_outstreams;
static ERL_NIF_TERM atom_num_peer_dests;
static ERL_NIF_TERM atom_num_pip;
static ERL_NIF_TERM atom_num_psctp;
static ERL_NIF_TERM atom_num_ptcp;
static ERL_NIF_TERM atom_num_pudp;
static ERL_NIF_TERM atom_num_sockets;
static ERL_NIF_TERM atom_num_tdgrams;
static ERL_NIF_TERM atom_num_tseqpkgs;
static ERL_NIF_TERM atom_num_tstreams;
static ERL_NIF_TERM atom_partial_delivery;
static ERL_NIF_TERM atom_peer_error;
static ERL_NIF_TERM atom_peer_rwnd;
static ERL_NIF_TERM atom_probe;
static ERL_NIF_TERM atom_select;
static ERL_NIF_TERM atom_sender_dry;
static ERL_NIF_TERM atom_send_failure;
static ERL_NIF_TERM atom_shutdown;
static ERL_NIF_TERM atom_slist;
static ERL_NIF_TERM atom_sourceaddr;
static ERL_NIF_TERM atom_timeout;
static ERL_NIF_TERM atom_true;
static ERL_NIF_TERM atom_want;

static ERL_NIF_TERM atom_eisconn;
static ERL_NIF_TERM atom_enotclosing;
static ERL_NIF_TERM atom_enotconn;
static ERL_NIF_TERM atom_exalloc;
static ERL_NIF_TERM atom_exbadstate;
static ERL_NIF_TERM atom_exbusy;
static ERL_NIF_TERM atom_exmon;
static ERL_NIF_TERM atom_exself;
static ERL_NIF_TERM atom_exsend;


/* *** Sockets *** */
static ErlNifResourceType*    sockets;
static ErlNifResourceTypeInit socketInit = {
   socket_dtor,
   socket_stop,
   (ErlNifResourceDown*) socket_down
};

// Initiated when the nif is loaded
static SocketData data;


/* ----------------------------------------------------------------------
 *  N I F   F u n c t i o n s
 * ----------------------------------------------------------------------
 *
 * Utility and admin functions:
 * ----------------------------
 * nif_info/0
 * (nif_debug/1)
 *
 * The "proper" socket functions:
 * ------------------------------
 * nif_open(Domain, Type, Protocol, Extra)
 * nif_bind(Sock, LocalAddr)
 * nif_connect(Sock, SockAddr)
 * nif_listen(Sock, Backlog)
 * nif_accept(LSock, Ref)
 * nif_send(Sock, SendRef, Data, Flags)
 * nif_sendto(Sock, SendRef, Data, Dest, Flags)
 * nif_sendmsg(Sock, SendRef, MsgHdr, Flags)
 * nif_recv(Sock, RecvRef, Length, Flags)
 * nif_recvfrom(Sock, RecvRef, BufSz, Flags)
 * nif_recvmsg(Sock, RecvRef, BufSz, CtrlSz, Flags)
 * nif_close(Sock)
 * nif_shutdown(Sock, How)
 * nif_sockname(Sock)
 * nif_peername(Sock)
 *
 * And some functions to manipulate and retrieve socket options:
 * -------------------------------------------------------------
 * nif_setopt/5
 * nif_getopt/4
 *
 * And some utility functions:
 * -------------------------------------------------------------
 *
 * And some socket admin functions:
 * -------------------------------------------------------------
 * nif_cancel(Sock, Ref)
 */


/* ----------------------------------------------------------------------
 * nif_info
 *
 * Description:
 * This is currently just a placeholder...
 */
#define MKCT(E, T, C) MKT2((E), (T), MKI((E), (C)))

static
ERL_NIF_TERM nif_info(ErlNifEnv*         env,
                      int                argc,
                      const ERL_NIF_TERM argv[])
{
    if (argc != 0) {
        return enif_make_badarg(env);
    } else {
        ERL_NIF_TERM numSockets     = MKCT(env, atom_num_sockets,  data.numSockets);
        ERL_NIF_TERM numTypeDGrams  = MKCT(env, atom_num_tdgrams,  data.numTypeDGrams);
        ERL_NIF_TERM numTypeStreams = MKCT(env, atom_num_tstreams, data.numTypeStreams);
        ERL_NIF_TERM numTypeSeqPkgs = MKCT(env, atom_num_tseqpkgs, data.numTypeSeqPkgs);
        ERL_NIF_TERM numDomLocal    = MKCT(env, atom_num_dlocal,   data.numDomainLocal);
        ERL_NIF_TERM numDomInet     = MKCT(env, atom_num_dinet,    data.numDomainInet);
        ERL_NIF_TERM numDomInet6    = MKCT(env, atom_num_dinet6,   data.numDomainInet6);
        ERL_NIF_TERM numProtoIP     = MKCT(env, atom_num_pip,      data.numProtoIP);
        ERL_NIF_TERM numProtoTCP    = MKCT(env, atom_num_ptcp,     data.numProtoTCP);
        ERL_NIF_TERM numProtoUDP    = MKCT(env, atom_num_pudp,     data.numProtoUDP);
        ERL_NIF_TERM numProtoSCTP   = MKCT(env, atom_num_psctp,    data.numProtoSCTP);
        ERL_NIF_TERM gcnt[]  = {numSockets,
                                numTypeDGrams, numTypeStreams, numTypeSeqPkgs,
                                numDomLocal, numDomInet, numDomInet6,
                                numProtoIP, numProtoTCP, numProtoUDP, numProtoSCTP};
        unsigned int lenGCnt = sizeof(gcnt) / sizeof(ERL_NIF_TERM);
        ERL_NIF_TERM lgcnt   = MKLA(env, gcnt, lenGCnt);
        ERL_NIF_TERM keys[]  = {esock_atom_debug, atom_iow, atom_global_counters};
        ERL_NIF_TERM vals[]  = {BOOL2ATOM(data.dbg), BOOL2ATOM(data.iow), lgcnt};
        ERL_NIF_TERM info;
        unsigned int numKeys = sizeof(keys) / sizeof(ERL_NIF_TERM);
        unsigned int numVals = sizeof(vals) / sizeof(ERL_NIF_TERM);

        ESOCK_ASSERT( (numKeys == numVals) );

        if (!MKMA(env, keys, vals, numKeys, &info))
            return enif_make_badarg(env);
    
        return info;
    }
}


/* ----------------------------------------------------------------------
 * nif_open
 *
 * Description:
 * Create an endpoint for communication.
 *
 * Arguments:
 * Domain   - The domain, for example 'inet'
 * Type     - Type of socket, for example 'stream'
 * Protocol - The protocol, for example 'tcp'
 * Extra    - A map with "obscure" options.
 *            Currently the only allowed option is netns (network namespace).
 *            This is *only* allowed on linux!
 */
static
ERL_NIF_TERM nif_open(ErlNifEnv*         env,
                      int                argc,
                      const ERL_NIF_TERM argv[])
{
    int          edomain, etype, eproto;
    int          domain, type, proto;
    char*        netns;
    ERL_NIF_TERM emap;
    ERL_NIF_TERM result;

    SGDBG( ("SOCKET", "nif_open -> entry with %d args\r\n", argc) );
    
    /* Extract arguments and perform preliminary validation */

    if ((argc != 4) ||
        !GET_INT(env, argv[0], &edomain) ||
        !GET_INT(env, argv[1], &etype) ||
        !IS_MAP(env,  argv[3])) {
        return enif_make_badarg(env);
    }
    eproto = argv[2];
    emap   = argv[3];

    SGDBG( ("SOCKET", "nif_open -> "
            "\r\n   edomain: %T"
            "\r\n   etype:   %T"
            "\r\n   eproto:  %T"
            "\r\n   extra:   %T"
            "\r\n", argv[0], argv[1], eproto, emap) );

    if (!edomain2domain(edomain, &domain)) {
        SGDBG( ("SOCKET", "nif_open -> domain: %d\r\n", domain) );
        return esock_make_error(env, esock_atom_einval);
    }

    if (!etype2type(etype, &type)) {
        SGDBG( ("SOCKET", "nif_open -> type: %d\r\n", type) );
        return esock_make_error(env, esock_atom_einval);
    }

    if (!eproto2proto(env, eproto, &proto)) {
        SGDBG( ("SOCKET", "nif_open -> protocol: %d\r\n", proto) );
        return esock_make_error(env, esock_atom_einval);
    }

#ifdef HAVE_SETNS
    /* We *currently* only support one extra option: netns */
    if (!emap2netns(env, emap, &netns)) {
        SGDBG( ("SOCKET", "nif_open -> namespace: %s\r\n", netns) );
        return enif_make_badarg(env);
    }
#else
    netns = NULL;
#endif

    result = nopen(env, domain, type, proto, netns);

    SGDBG( ("SOCKET", "nif_open -> done with result: "
           "\r\n   %T"
           "\r\n", result) );

    return result;
}


/* nopen - create an endpoint for communication
 *
 * Assumes the input has been validated.
 *
 * Normally we want debugging on (individual) sockets to be controlled
 * by the sockets own debug flag. But since we don't even have a socket
 * yet, we must use the global debug flag.
 */
static
ERL_NIF_TERM nopen(ErlNifEnv* env,
                   int domain, int type, int protocol,
                   char* netns)
{
    SocketDescriptor* descP;
    ERL_NIF_TERM      res;
    int               save_errno = 0;
    SOCKET            sock;
    HANDLE            event;
#ifdef HAVE_SETNS
    int               current_ns = 0;
#endif

    SGDBG( ("SOCKET", "nopen -> entry with"
            "\r\n   domain:   %d"
            "\r\n   type:     %d"
            "\r\n   protocol: %d"
            "\r\n   netns:    %s"
            "\r\n", domain, type, protocol, ((netns == NULL) ? "NULL" : netns)) );

#ifdef HAVE_SETNS
    if ((netns != NULL) &&
        !change_network_namespace(netns, &current_ns, &save_errno))
        return esock_make_error_errno(env, save_errno);
#endif

    if ((sock = sock_open(domain, type, protocol)) == INVALID_SOCKET)
        return esock_make_error_errno(env, sock_errno());

    SGDBG( ("SOCKET", "nopen -> open success: %d\r\n", sock) );

#ifdef HAVE_SETNS
    if ((netns != NULL) &&
        !restore_network_namespace(current_ns, sock, &save_errno))
        return esock_make_error_errno(env, save_errno);

    if (netns != NULL)
        FREE(netns);
#endif


    if ((event = sock_create_event(sock)) == INVALID_EVENT) {
        save_errno = sock_errno();
        while ((sock_close(sock) == INVALID_SOCKET) && (sock_errno() == EINTR));
        return esock_make_error_errno(env, save_errno);
    }

    SGDBG( ("SOCKET", "nopen -> event success: %d\r\n", event) );

    SET_NONBLOCKING(sock);


    /* Create and initiate the socket "descriptor" */
    if ((descP = alloc_descriptor(sock, event)) == NULL) {
        sock_close(sock);
        // Not sure if this is really the proper error, but...
        return enif_make_badarg(env);
    }

    descP->state    = SOCKET_STATE_OPEN;
    descP->domain   = domain;
    descP->type     = type;
    descP->protocol = protocol;

    res = enif_make_resource(env, descP);
    enif_release_resource(descP); // We should really store a reference ...


    /* Keep track of the creator
     * This should not be a problem but just in case
     * the *open* function is used with the wrong kind
     * of environment...
     */
    if (enif_self(env, &descP->ctrlPid) == NULL)
        return esock_make_error(env, atom_exself);

    if (MONP(env, descP,
             &descP->ctrlPid,
             &descP->ctrlMon) > 0)
        return esock_make_error(env, atom_exmon);


#ifdef __WIN32__
    /* <KOLLA
     *
     * What is the point of this?
     * And how do we handle it?
     * Since the select message will be delivered to the controlling
     * process, which has no idea what to do with this...
     *
     * TODO!
     *
     * </KOLLA>
     */
    SELECT(env,
           event,
           (ERL_NIF_SELECT_READ),
           descP, NULL, esock_atom_undefined);
#endif

    inc_socket(domain, type, protocol);

    return esock_make_ok2(env, res);
}



#ifdef HAVE_SETNS
/* We should really have another API, so that we can return errno... */

/* *** change network namespace ***
 * Retreive the current namespace and set the new.
 * Return result and previous namespace if successfull.
 */
static
BOOLEAN_T change_network_namespace(char* netns, int* cns, int* err)
{
    int save_errno;
    int current_ns = 0;
    int new_ns     = 0;

    SGDBG( ("SOCKET", "change_network_namespace -> entry with"
            "\r\n   new ns: %s", netns) );

    if (netns != NULL) {
        current_ns = open("/proc/self/ns/net", O_RDONLY);
        if (current_ns == INVALID_SOCKET) {
            *cns = current_ns;
            *err = sock_errno();
            return FALSE;
        }
        new_ns = open(netns, O_RDONLY);
        if (new_ns == INVALID_SOCKET) {
            save_errno = sock_errno();
            while (close(current_ns) == INVALID_SOCKET &&
                   sock_errno() == EINTR);
            *cns = -1;
            *err = save_errno;
            return FALSE;
        }
        if (setns(new_ns, CLONE_NEWNET) != 0) {
            save_errno = sock_errno();
            while ((close(new_ns) == INVALID_SOCKET) &&
                   (sock_errno() == EINTR));
            while ((close(current_ns) == INVALID_SOCKET) &&
                   (sock_errno() == EINTR));
            *cns = -1;
            *err = save_errno;
            return FALSE;
        } else {
            while ((close(new_ns) == INVALID_SOCKET) &&
                   (sock_errno() == EINTR));
            *cns = current_ns;
            *err = 0;
            return TRUE;
        }
    } else {
        *cns = INVALID_SOCKET;
        *err = 0;
        return TRUE;
    }
}


/* *** restore network namespace ***
 * Restore the previous namespace (see above).
 */
static
BOOLEAN_T restore_network_namespace(int ns, SOCKET sock, int* err)
{
    int save_errno;

    SGDBG( ("SOCKET", "restore_network_namespace -> entry with"
            "\r\n   ns: %d", ns) );

    if (ns != INVALID_SOCKET) {
        if (setns(ns, CLONE_NEWNET) != 0) {
            /* XXX Failed to restore network namespace.
             * What to do? Tidy up and return an error...
             * Note that the thread now might still be in the namespace.
             * Can this even happen? Should the emulator be aborted?
             */
            if (sock != INVALID_SOCKET)
                save_errno = sock_errno();
            while (close(sock) == INVALID_SOCKET &&
                   sock_errno() == EINTR);
            sock = INVALID_SOCKET;
            while (close(ns) == INVALID_SOCKET &&
                   sock_errno() == EINTR);
            *err = save_errno;
            return FALSE;
        } else {
            while (close(ns) == INVALID_SOCKET &&
                   sock_errno() == EINTR);
            *err = 0;
            return TRUE;
        }
  }

  *err = 0;
  return TRUE;
}
#endif



/* ----------------------------------------------------------------------
 * nif_bind
 *
 * Description:
 * Bind a name to a socket.
 *
 * Arguments:
 * [0] Socket (ref) - Points to the socket descriptor.
 * [1] LocalAddr    - Local address is a sockaddr map ( socket:sockaddr() ).
 */
static
ERL_NIF_TERM nif_bind(ErlNifEnv*         env,
                      int                argc,
                      const ERL_NIF_TERM argv[])
{
    SocketDescriptor* descP;
    ERL_NIF_TERM      eSockAddr;
    SocketAddress     sockAddr;
    unsigned int      addrLen;
    char*             xres;

    SGDBG( ("SOCKET", "nif_bind -> entry with argc: %d\r\n", argc) );

    /* Extract arguments and perform preliminary validation */

    if ((argc != 2) ||
        !enif_get_resource(env, argv[0], sockets, (void**) &descP)) {
        return enif_make_badarg(env);
    }
    eSockAddr = argv[1];

    SSDBG( descP,
           ("SOCKET", "nif_bind -> args when sock = %d (0x%lX)"
            "\r\n   Socket:   %T"
            "\r\n   SockAddr: %T"
            "\r\n", descP->sock, descP->state, argv[0], eSockAddr) );

    /* Make sure we are ready
     * Not sure how this would even happen, but...
     */
    if (descP->state != SOCKET_STATE_OPEN)
        return esock_make_error(env, atom_exbadstate);

    if ((xres = esock_decode_sockaddr(env, eSockAddr, &sockAddr, &addrLen)) != NULL)
        return esock_make_error_str(env, xres);
        
    return nbind(env, descP, &sockAddr, addrLen);
}


static
ERL_NIF_TERM nbind(ErlNifEnv*        env,
                   SocketDescriptor* descP,
                   SocketAddress*    sockAddrP,
                   unsigned int      addrLen)
{
    int port, ntohs_port;

    SSDBG( descP, ("SOCKET", "nbind -> try bind\r\n") );

    if (IS_SOCKET_ERROR(sock_bind(descP->sock,
                                  (struct sockaddr*) sockAddrP, addrLen))) {
        return esock_make_error_errno(env, sock_errno());
    }

    SSDBG( descP, ("SOCKET", "nbind -> bound - get port\r\n") );

    port = which_address_port(sockAddrP);
    SSDBG( descP, ("SOCKET", "nbind -> port: %d\r\n", port) );
    if (port == 0) {
        SOCKLEN_T len = sizeof(SocketAddress);
        sys_memzero((char *) sockAddrP, len);
        sock_name(descP->sock, &sockAddrP->sa, &len);
        port = which_address_port(sockAddrP);
    } else if (port == -1) {
        port = 0;
    }

    ntohs_port = sock_ntohs(port);
    
    SSDBG( descP, ("SOCKET", "nbind -> done with port = %d\r\n", ntohs_port) );

    return esock_make_ok2(env, MKI(env, ntohs_port));

}




/* ----------------------------------------------------------------------
 * nif_connect
 *
 * Description:
 * Initiate a connection on a socket
 *
 * Arguments:
 * Socket (ref) - Points to the socket descriptor.
 * SockAddr     - Socket Address of "remote" host.
 *                This is sockaddr(), which is either
 *                sockaddr_in4 or sockaddr_in6.
 */
static
ERL_NIF_TERM nif_connect(ErlNifEnv*         env,
                         int                argc,
                         const ERL_NIF_TERM argv[])
{
    SocketDescriptor* descP;
    ERL_NIF_TERM      eSockAddr;
    char*             xres;

    SGDBG( ("SOCKET", "nif_connect -> entry with argc: %d\r\n", argc) );

    /* Extract arguments and perform preliminary validation */

    if ((argc != 2) ||
        !enif_get_resource(env, argv[0], sockets, (void**) &descP)) {
        return enif_make_badarg(env);
    }
    eSockAddr = argv[1];

    SSDBG( descP,
           ("SOCKET", "nif_connect -> args when sock = %d:"
            "\r\n   Socket:   %T"
            "\r\n   SockAddr: %T"
            "\r\n", descP->sock, argv[0], eSockAddr) );
    
    if ((xres = esock_decode_sockaddr(env, eSockAddr,
                                      &descP->remote, &descP->addrLen)) != NULL) {
        return esock_make_error_str(env, xres);
    }

    return nconnect(env, descP);
}


static
ERL_NIF_TERM nconnect(ErlNifEnv*        env,
                      SocketDescriptor* descP)
{
    int code;

    /* Verify that we are where in the proper state */

    if (!IS_OPEN(descP))
        return esock_make_error(env, atom_exbadstate);

    if (IS_CONNECTED(descP))
        return esock_make_error(env, atom_eisconn);

    if (IS_CONNECTING(descP))
        return esock_make_error(env, esock_atom_einval);

    code = sock_connect(descP->sock,
                        (struct sockaddr*) &descP->remote,
                        descP->addrLen);

    if (IS_SOCKET_ERROR(code) &&
        ((sock_errno() == ERRNO_BLOCK) ||   /* Winsock2            */
         (sock_errno() == EINPROGRESS))) {  /* Unix & OSE!!        */
        ERL_NIF_TERM ref = MKREF(env);
        descP->state = SOCKET_STATE_CONNECTING;
        SELECT(env,
               descP->sock,
               (ERL_NIF_SELECT_WRITE),
               descP, NULL, ref);
        return esock_make_ok2(env, ref);
    } else if (code == 0) {                 /* ok we are connected */
        descP->state = SOCKET_STATE_CONNECTED;
        /* Do we need to do somthing for "active" mode?
         * Is there even such a thing *here*?
         */
        return esock_atom_ok;
    } else {
        return esock_make_error_errno(env, sock_errno());
    }

}


/* ----------------------------------------------------------------------
 * nif_finalize_connection
 *
 * Description:
 * Make socket ready for input and output.
 *
 * Arguments:
 * Socket (ref) - Points to the socket descriptor.
 */
static
ERL_NIF_TERM nif_finalize_connection(ErlNifEnv*         env,
                                     int                argc,
                                     const ERL_NIF_TERM argv[])
{
    SocketDescriptor* descP;

    /* Extract arguments and perform preliminary validation */

    if ((argc != 1) ||
        !enif_get_resource(env, argv[0], sockets, (void**) &descP)) {
        return enif_make_badarg(env);
    }

    return nfinalize_connection(env, descP);

}


/* *** nfinalize_connection ***
 * Perform the final check to verify a connection.
 */
static
ERL_NIF_TERM nfinalize_connection(ErlNifEnv*        env,
                                  SocketDescriptor* descP)
{
    int error;

    if (descP->state != SOCKET_STATE_CONNECTING)
        return esock_make_error(env, atom_enotconn);

    if (!verify_is_connected(descP, &error)) {
        descP->state = SOCKET_STATE_OPEN;  /* restore state */
        return esock_make_error_errno(env, error);
    }

    descP->state = SOCKET_STATE_CONNECTED;

    return esock_atom_ok;
}


/* *** verify_is_connected ***
 * Check if a connection has been established.
 */
static
BOOLEAN_T verify_is_connected(SocketDescriptor* descP, int* err)
{
    /*
     * *** This is strange ***
     *
     * This *should* work on Windows NT too, but doesn't.
     * An bug in Winsock 2.0 for Windows NT?
     *
     * See "Unix Netwok Programming", W.R.Stevens, p 412 for a
     * discussion about Unix portability and non blocking connect.
     */

#ifndef SO_ERROR
    
    int sz, code;

    sz = sizeof(descP->remote);
    sys_memzero((char *) &descP->remote, sz);
    code = sock_peer(desc->sock,
                     (struct sockaddr*) &descP->remote, &sz);

    if (IS_SOCKET_ERROR(code)) {
        *err = sock_errno();
        return FALSE;
    }

#else

    int          error = 0;             /* Has to be initiated, we check it */
    unsigned int sz    = sizeof(error); /* even if we get -1                */
    int          code  = sock_getopt(descP->sock,
                                     SOL_SOCKET, SO_ERROR,
                                     (void *)&error, &sz);

    if ((code < 0) || error) {
        *err = error;
        return FALSE;
    }

#endif /* SO_ERROR */

    *err = 0;

    return TRUE;
}



/* ----------------------------------------------------------------------
 * nif_listen
 *
 * Description:
 * Listen for connections on a socket.
 *
 * Arguments:
 * Socket (ref) - Points to the socket descriptor.
 * Backlog      - The maximum length to which the queue of pending
 *                connections for socket may grow.
 */
static
ERL_NIF_TERM nif_listen(ErlNifEnv*         env,
                        int                argc,
                        const ERL_NIF_TERM argv[])
{
    SocketDescriptor* descP;
    int               backlog;

    SGDBG( ("SOCKET", "nif_listen -> entry with argc: %d\r\n", argc) );
    
    /* Extract arguments and perform preliminary validation */

    if ((argc != 2) ||
        !enif_get_resource(env, argv[0], sockets, (void**) &descP) ||
        !GET_INT(env, argv[1], &backlog)) {
        return enif_make_badarg(env);
    }

    SSDBG( descP,
           ("SOCKET", "nif_listen -> args when sock = %d:"
            "\r\n   Socket:  %T"
            "\r\n   backlog: %d"
            "\r\n", descP->sock, argv[0], backlog) );
    
    return nlisten(env, descP, backlog);
}



static
ERL_NIF_TERM nlisten(ErlNifEnv*        env,
                     SocketDescriptor* descP,
                     int               backlog)
{
    if (descP->state == SOCKET_STATE_CLOSED)
        return esock_make_error(env, atom_exbadstate);

    if (!IS_OPEN(descP))
        return esock_make_error(env, atom_exbadstate);

    if (IS_SOCKET_ERROR(sock_listen(descP->sock, backlog)))
        return esock_make_error_errno(env, sock_errno());

    descP->state = SOCKET_STATE_LISTENING;

    return esock_atom_ok;
}



/* ----------------------------------------------------------------------
 * nif_accept
 *
 * Description:
 * Accept a connection on a socket.
 *
 * Arguments:
 * Socket (ref) - Points to the socket descriptor.
 * Request ref  - Unique "id" of this request
 *                (used for the select, if none is in queue).
 */
static
ERL_NIF_TERM nif_accept(ErlNifEnv*         env,
                        int                argc,
                        const ERL_NIF_TERM argv[])
{
    SocketDescriptor* descP;
    ERL_NIF_TERM      ref;

    SGDBG( ("SOCKET", "nif_accept -> entry with argc: %d\r\n", argc) );
    
    /* Extract arguments and perform preliminary validation */

    if ((argc != 2) ||
        !enif_get_resource(env, argv[0], sockets, (void**) &descP)) {
        return enif_make_badarg(env);
    }
    ref = argv[1];
    
    SSDBG( descP,
           ("SOCKET", "nif_accept -> args when sock = %d:"
            "\r\n   Socket: %T"
            "\r\n   ReqRef: %T"
            "\r\n", descP->sock, argv[0], ref) );

    return naccept(env, descP, ref);
}


static
ERL_NIF_TERM naccept(ErlNifEnv*        env,
                     SocketDescriptor* descP,
                     ERL_NIF_TERM      ref)
{
    ERL_NIF_TERM res;

    switch (descP->state) {
    case SOCKET_STATE_LISTENING:
        MLOCK(descP->accMtx);
        res = naccept_listening(env, descP, ref);
        MUNLOCK(descP->accMtx);
        break;

    case SOCKET_STATE_ACCEPTING:
        MLOCK(descP->accMtx);
        res = naccept_accepting(env, descP, ref);
        MUNLOCK(descP->accMtx);
        break;

    default:
        res = esock_make_error(env, esock_atom_einval);
        break;
    }

    return res;
}


/* *** naccept_listening ***
 * We have no active acceptor and no acceptors in queue.
 */
static
ERL_NIF_TERM naccept_listening(ErlNifEnv*        env,
                               SocketDescriptor* descP,
                               ERL_NIF_TERM      ref)
{
    SocketAddress remote;
    unsigned int  n;
    SOCKET        accSock;
    HANDLE        accEvent;
    int           save_errno;
    ErlNifPid     caller;

    SSDBG( descP, ("SOCKET", "naccept_listening -> get caller\r\n") );

    if (enif_self(env, &caller) == NULL)
        return esock_make_error(env, atom_exself);

    n = sizeof(remote);
    sys_memzero((char *) &remote, n);
    SSDBG( descP, ("SOCKET", "naccept_listening -> try accept\r\n") );
    accSock = sock_accept(descP->sock, (struct sockaddr*) &remote, &n);
    if (accSock == INVALID_SOCKET) {

        save_errno = sock_errno();

        SSDBG( descP,
               ("SOCKET",
                "naccept_listening -> accept failed (%d)\r\n", save_errno) );

        if (save_errno == ERRNO_BLOCK) {

            /* *** Try again later *** */
            SSDBG( descP, ("SOCKET", "naccept_listening -> would block\r\n") );

            descP->currentAcceptor.pid = caller;
            if (MONP(env, descP,
                     &descP->currentAcceptor.pid,
                     &descP->currentAcceptor.mon) > 0)
                return esock_make_error(env, atom_exmon);

            descP->currentAcceptor.ref = enif_make_copy(descP->env, ref);
            descP->currentAcceptorP    = &descP->currentAcceptor;

            SELECT(env,
                   descP->sock,
                   (ERL_NIF_SELECT_READ),
                   descP, NULL, ref);

            /* Shall we really change state?
             * The ready event is sent directly to the calling
             * process, which simply calls this function again.
             * Basically, state accepting means that we have
             * an "outstanding" accept.
             * Shall we store the pid of the calling process?
             * And if someone else calls accept, return with ebusy?
             * Can any process call accept or just the controlling
             * process?
             * We also need a monitor it case the calling process is
             * called before we are done!
             *
             * Change state (to accepting) and store pid of the acceptor
             * (current process). Only accept calls from the acceptor
             * process (ebusy) and once we have a successful accept,
             * change state back to listening. If cancel is called instead
             * (only accepted from the acceptor process), we reset
             * state to listening and also resets the pid to "null"
             * (is there such a value?).
             * Need a mutex to secure that we don't test and change the
             * pid at the same time.
             */

            descP->state = SOCKET_STATE_ACCEPTING;

            return esock_make_error(env, esock_atom_eagain);

        } else {
            SSDBG( descP,
                   ("SOCKET",
                    "naccept_listening -> errno: %d\r\n", save_errno) );
            return esock_make_error_errno(env, save_errno);
        }

    } else {
        SocketDescriptor* accDescP;
        ERL_NIF_TERM      accRef;

        /*
         * We got one
         */

        SSDBG( descP, ("SOCKET", "naccept_listening -> accept success\r\n") );

        if ((accEvent = sock_create_event(accSock)) == INVALID_EVENT) {
            save_errno = sock_errno();
            while ((sock_close(accSock) == INVALID_SOCKET) &&
                   (sock_errno() == EINTR));
            return esock_make_error_errno(env, save_errno);
        }

        if ((accDescP = alloc_descriptor(accSock, accEvent)) == NULL) {
            sock_close(accSock);
            return enif_make_badarg(env);
        }

        accDescP->domain   = descP->domain;
        accDescP->type     = descP->type;
        accDescP->protocol = descP->protocol;

        accRef = enif_make_resource(env, accDescP);
        enif_release_resource(accDescP); // We should really store a reference ...

        accDescP->ctrlPid = caller;
        if (MONP(env, accDescP,
                 &accDescP->ctrlPid,
                 &accDescP->ctrlMon) > 0) {
            sock_close(accSock);
            return esock_make_error(env, atom_exmon);
        }

        accDescP->remote = remote;
        SET_NONBLOCKING(accDescP->sock);

#ifdef __WIN32__
        /* See 'What is the point of this?' above */
        SELECT(env,
               descP->sock,
               (ERL_NIF_SELECT_READ),
               descP, NULL, esock_atom_undefined);
#endif

        accDescP->state = SOCKET_STATE_CONNECTED;

        return esock_make_ok2(env, accRef);
    }
}


/* *** naccept_accepting ***
 * We have an active acceptor and possibly acceptors waiting in queue.
 * If the pid of the calling process is not the pid of the "current process",
 * push the requester onto the queue.
 */
static
ERL_NIF_TERM naccept_accepting(ErlNifEnv*        env,
                               SocketDescriptor* descP,
                               ERL_NIF_TERM      ref)
{
    SocketAddress remote;
    unsigned int  n;
    SOCKET        accSock;
    HANDLE        accEvent;
    ErlNifPid     caller;
    int           save_errno;
    ERL_NIF_TERM  result;

    SSDBG( descP, ("SOCKET", "naccept_accepting -> get caller\r\n") );

    if (enif_self(env, &caller) == NULL)
        return esock_make_error(env, atom_exself);

    SSDBG( descP, ("SOCKET", "naccept_accepting -> check: "
                   "are caller current acceptor:"
                   "\r\n   Caller:  %T"
                   "\r\n   Current: %T"
                   "\r\n", caller, descP->currentAcceptor.pid) );

    if (!compare_pids(env, &descP->currentAcceptor.pid, &caller)) {

        /* Not the "current acceptor", so (maybe) push onto queue */

        SSDBG( descP, ("SOCKET", "naccept_accepting -> not (active) acceptor\r\n") );

        if (!acceptor_search4pid(env, descP, &caller))
            result = acceptor_push(env, descP, caller, ref);
        else
            result = esock_make_error(env, esock_atom_eagain);
        
        SSDBG( descP,
               ("SOCKET",
                "naccept_accepting -> queue (push) result: %T\r\n", result) );

        return result;
    }

    n = sizeof(descP->remote);
    sys_memzero((char *) &remote, n);
    SSDBG( descP, ("SOCKET", "naccept_accepting -> try accept\r\n") );
    accSock = sock_accept(descP->sock, (struct sockaddr*) &remote, &n);
    if (accSock == INVALID_SOCKET) {

        save_errno = sock_errno();

        SSDBG( descP,
               ("SOCKET",
                "naccept_accepting -> accept failed (%d)\r\n", save_errno) );

        if (save_errno == ERRNO_BLOCK) {

            /*
             * Just try again, no real error, just a ghost trigger from poll,
             */

            SSDBG( descP,
                   ("SOCKET",
                    "naccept_accepting -> would block: try again\r\n") );

            SELECT(env,
                   descP->sock,
                   (ERL_NIF_SELECT_READ),
                   descP, NULL, ref);

            return esock_make_error(env, esock_atom_eagain);
        } else {
            SSDBG( descP,
                   ("SOCKET",
                    "naccept_accepting -> errno: %d\r\n", save_errno) );
            return esock_make_error_errno(env, save_errno);
        }
    } else {
        SocketDescriptor* accDescP;
        ERL_NIF_TERM      accRef;

        /*
         * We got one
         */

        SSDBG( descP, ("SOCKET", "naccept_accepting -> accept success\r\n") );

        DEMONP(env, descP, &descP->currentAcceptor.mon);

        if ((accEvent = sock_create_event(accSock)) == INVALID_EVENT) {
            save_errno = sock_errno();
            while ((sock_close(accSock) == INVALID_SOCKET) &&
                   (sock_errno() == EINTR));
            return esock_make_error_errno(env, save_errno);
        }

        if ((accDescP = alloc_descriptor(accSock, accEvent)) == NULL) {
            sock_close(accSock);
            return enif_make_badarg(env);
        }

        accDescP->domain   = descP->domain;
        accDescP->type     = descP->type;
        accDescP->protocol = descP->protocol;

        accRef = enif_make_resource(env, accDescP);
        enif_release_resource(accDescP); // We should really store a reference ...

        accDescP->ctrlPid = caller;
        if (MONP(env, accDescP,
                 &accDescP->ctrlPid,
                 &accDescP->ctrlMon) > 0) {
            sock_close(accSock);
            return esock_make_error(env, atom_exmon);
        }

        accDescP->remote  = remote;
        SET_NONBLOCKING(accDescP->sock);

#ifdef __WIN32__
        /* See 'What is the point of this?' above */
        SELECT(env,
               descP->sock,
               (ERL_NIF_SELECT_READ),
               descP, NULL, esock_atom_undefined);
#endif

        accDescP->state = SOCKET_STATE_CONNECTED;

        /* Check if there are waiting acceptors (popping the acceptor queue) */

        if (acceptor_pop(env, descP,
                         &descP->currentAcceptor.pid,
                         &descP->currentAcceptor.mon,
                         &descP->currentAcceptor.ref)) {

            /* There was another one */

            SSDBG( descP, ("SOCKET", "naccept_accepting -> new (active) acceptor: "
                           "\r\n   pid: %T"
                           "\r\n   ref: %T"
                           "\r\n",
                           descP->currentAcceptor.pid,
                           descP->currentAcceptor.ref) );

            SELECT(env,
                   descP->sock,
                   (ERL_NIF_SELECT_READ),
                   descP, &descP->currentAcceptor.pid, descP->currentAcceptor.ref);
        } else {
            descP->currentAcceptorP = NULL;
            descP->state            = SOCKET_STATE_LISTENING;
        }
        
        return esock_make_ok2(env, accRef);
    }
}



/* ----------------------------------------------------------------------
 * nif_send
 *
 * Description:
 * Send a message on a socket
 *
 * Arguments:
 * Socket (ref) - Points to the socket descriptor.
 * SendRef      - A unique id for this (send) request.
 * Data         - The data to send in the form of a IOVec.
 * Flags        - Send flags.
 */

static
ERL_NIF_TERM nif_send(ErlNifEnv*         env,
                      int                argc,
                      const ERL_NIF_TERM argv[])
{
    SocketDescriptor* descP;
    ERL_NIF_TERM      sendRef;
    ErlNifBinary      sndData;
    unsigned int      eflags;
    int               flags;
    ERL_NIF_TERM      res;

    SGDBG( ("SOCKET", "nif_send -> entry with argc: %d\r\n", argc) );

    /* Extract arguments and perform preliminary validation */

    if ((argc != 4) ||
        !enif_get_resource(env, argv[0], sockets, (void**) &descP) ||
        !GET_BIN(env, argv[2], &sndData) ||
        !GET_UINT(env, argv[3], &eflags)) {
        return enif_make_badarg(env);
    }
    sendRef = argv[1];

    SSDBG( descP,
           ("SOCKET", "nif_send -> args when sock = %d:"
            "\r\n   Socket:       %T"
            "\r\n   SendRef:      %T"
            "\r\n   Size of data: %d"
            "\r\n   eFlags:       %d"
            "\r\n", descP->sock, argv[0], sendRef, sndData.size, eflags) );

    if (!IS_CONNECTED(descP))
        return esock_make_error(env, atom_enotconn);

    if (!esendflags2sendflags(eflags, &flags))
        return enif_make_badarg(env);

    MLOCK(descP->writeMtx);

    /* We need to handle the case when another process tries
     * to write at the same time.
     * If the current write could not write its entire package
     * this time (resulting in an select). The write of the
     * other process must be made to wait until current
     * is done!
     * Basically, we need a write queue!
     *
     * A 'writing' field (boolean), which is set if we did
     * not manage to write the entire message and reset every
     * time we do.
     */

    res = nsend(env, descP, sendRef, &sndData, flags);

    MUNLOCK(descP->writeMtx);

    return res;
}


/* *** nsend ***
 *
 * Do the actual send.
 * Do some initial writer checks, do the actual send and then
 * analyze the result. If we are done, another writer may be
 * scheduled (if there is one in the writer queue).
 */
static
ERL_NIF_TERM nsend(ErlNifEnv*        env,
                   SocketDescriptor* descP,
                   ERL_NIF_TERM      sendRef,
                   ErlNifBinary*     sndDataP,
                   int               flags)
{
    int          save_errno;
    ssize_t      written;
    ERL_NIF_TERM writerCheck;

    if (!descP->isWritable)
        return enif_make_badarg(env);

    /* Check if there is already a current writer and if its us */
    if (!send_check_writer(env, descP, sendRef, &writerCheck))
        return writerCheck;
    
    /* We ignore the wrap for the moment.
     * Maybe we should issue a wrap-message to controlling process...
     */
    cnt_inc(&descP->writeTries, 1);

    written = sock_send(descP->sock, sndDataP->data, sndDataP->size, flags);
    if (IS_SOCKET_ERROR(written))
        save_errno = sock_errno();
    else
        save_errno = -1; // The value does not actually matter in this case
    

    return send_check_result(env, descP,
                             written, sndDataP->size, save_errno, sendRef);

}



/* ----------------------------------------------------------------------
 * nif_sendto
 *
 * Description:
 * Send a message on a socket
 *
 * Arguments:
 * Socket (ref) - Points to the socket descriptor.
 * SendRef      - A unique id for this (send) request.
 * Data         - The data to send in the form of a IOVec.
 * Dest         - Destination (socket) address.
 * Flags        - Send flags.
 */

static
ERL_NIF_TERM nif_sendto(ErlNifEnv*         env,
                        int                argc,
                        const ERL_NIF_TERM argv[])
{
    SocketDescriptor* descP;
    ERL_NIF_TERM      sendRef;
    ErlNifBinary      sndData;
    unsigned int      eflags;
    int               flags;
    ERL_NIF_TERM      eSockAddr;
    SocketAddress     remoteAddr;
    unsigned int      remoteAddrLen;
    char*             xres;
    ERL_NIF_TERM      res;

    SGDBG( ("SOCKET", "nif_sendto -> entry with argc: %d\r\n", argc) );

    /* Extract arguments and perform preliminary validation */

    if ((argc != 5) ||
        !enif_get_resource(env, argv[0], sockets, (void**) &descP) ||
        !GET_BIN(env, argv[2], &sndData) ||
        !GET_UINT(env, argv[4], &eflags)) {
        return enif_make_badarg(env);
    }
    sendRef   = argv[1];
    eSockAddr = argv[3];

    SSDBG( descP,
           ("SOCKET", "nif_sendto -> args when sock = %d:"
            "\r\n   Socket:       %T"
            "\r\n   sendRef:      %T"
            "\r\n   size of data: %d"
            "\r\n   eSockAddr:    %T"
            "\r\n   eflags:       %d"
            "\r\n",
            descP->sock, argv[0], sendRef, sndData.size, eSockAddr, eflags) );

    /* THIS TEST IS NOT CORRECT!!! */
    if (!IS_OPEN(descP))
        return esock_make_error(env, esock_atom_einval);

    if (!esendflags2sendflags(eflags, &flags))
        return esock_make_error(env, esock_atom_einval);

    if ((xres = esock_decode_sockaddr(env, eSockAddr,
                                      &remoteAddr,
                                      &remoteAddrLen)) != NULL)
        return esock_make_error_str(env, xres);

    MLOCK(descP->writeMtx);

    res = nsendto(env, descP, sendRef, &sndData, flags,
                  &remoteAddr, remoteAddrLen);

    MUNLOCK(descP->writeMtx);

    SGDBG( ("SOCKET", "nif_sendto -> done with result: "
            "\r\n   %T"
            "\r\n", res) );

    return res;
}


static
ERL_NIF_TERM nsendto(ErlNifEnv*        env,
                     SocketDescriptor* descP,
                     ERL_NIF_TERM      sendRef,
                     ErlNifBinary*     dataP,
                     int               flags,
                     SocketAddress*    toAddrP,
                     unsigned int      toAddrLen)
{
    int          save_errno;
    ssize_t      written;
    ERL_NIF_TERM writerCheck;

    if (!descP->isWritable)
        return enif_make_badarg(env);

    /* Check if there is already a current writer and if its us */
    if (!send_check_writer(env, descP, sendRef, &writerCheck))
        return writerCheck;
    
    /* We ignore the wrap for the moment.
     * Maybe we should issue a wrap-message to controlling process...
     */
    cnt_inc(&descP->writeTries, 1);

    if (toAddrP != NULL) {
        written = sock_sendto(descP->sock,
                              dataP->data, dataP->size, flags,
                              &toAddrP->sa, toAddrLen);
    } else {
        written = sock_sendto(descP->sock,
                              dataP->data, dataP->size, flags,
                              NULL, 0);
    }
    if (IS_SOCKET_ERROR(written))
        save_errno = sock_errno();
    else
        save_errno = -1; // The value does not actually matter in this case

    return send_check_result(env, descP, written, dataP->size, save_errno, sendRef);
}



/* ----------------------------------------------------------------------
 * nif_sendmsg
 *
 * Description:
 * Send a message on a socket
 *
 * Arguments:
 * Socket (ref) - Points to the socket descriptor.
 * SendRef      - A unique id for this (send) request.
 * MsgHdr       - Message Header - data and (maybe) control and dest
 * Flags        - Send flags.
 */

static
ERL_NIF_TERM nif_sendmsg(ErlNifEnv*         env,
                         int                argc,
                         const ERL_NIF_TERM argv[])
{
    ERL_NIF_TERM      res, sendRef, eMsgHdr;
    SocketDescriptor* descP;
    unsigned int      eflags;
    int               flags;

    SGDBG( ("SOCKET", "nif_sendmsg -> entry with argc: %d\r\n", argc) );

    /* Extract arguments and perform preliminary validation */

    if ((argc != 4) ||
        !enif_get_resource(env, argv[0], sockets, (void**) &descP) ||
        !IS_MAP(env, argv[2]) ||
        !GET_UINT(env, argv[3], &eflags)) {
        return enif_make_badarg(env);
    }
    sendRef = argv[1];
    eMsgHdr = argv[2];

    SSDBG( descP,
           ("SOCKET", "nif_sendmsg -> args when sock = %d:"
            "\r\n   Socket:  %T"
            "\r\n   sendRef: %T"
            "\r\n   eflags:  %d"
            "\r\n",
            descP->sock, argv[0], sendRef, eflags) );

    /* THIS TEST IS NOT CORRECT!!! */
    if (!IS_OPEN(descP))
        return esock_make_error(env, esock_atom_einval);

    if (!esendflags2sendflags(eflags, &flags))
        return esock_make_error(env, esock_atom_einval);

    MLOCK(descP->writeMtx);

    res = nsendmsg(env, descP, sendRef, eMsgHdr, flags);

    MUNLOCK(descP->writeMtx);

    SSDBG( descP,
           ("SOCKET", "nif_sendmsg -> done with result: "
            "\r\n   %T"
            "\r\n", res) );

    return res;
}


static
ERL_NIF_TERM nsendmsg(ErlNifEnv*        env,
                      SocketDescriptor* descP,
                      ERL_NIF_TERM      sendRef,
                      ERL_NIF_TERM      eMsgHdr,
                      int               flags)
{
    ERL_NIF_TERM  res, eAddr, eIOV, eCtrl;
    SocketAddress addr;
    struct msghdr msgHdr;
    ErlNifBinary* iovBins;
    struct iovec* iov;
    unsigned int  iovLen;
    char*         ctrlBuf;
    size_t        ctrlBufLen, ctrlBufUsed;
    int           save_errno;
    ssize_t       written, dataSize;
    ERL_NIF_TERM  writerCheck;
    char*         xres;

    if (!descP->isWritable)
        return enif_make_badarg(env);

    /* Check if there is already a current writer and if its us */
    if (!send_check_writer(env, descP, sendRef, &writerCheck))
        return writerCheck;
    
    /* Depending on if we are *connected* or not, we require
     * different things in the msghdr map.
     */
    if (IS_CONNECTED(descP)) {

        /* We don't need the address */

        SSDBG( descP, ("SOCKET", "nsendmsg -> connected: no address\r\n") );

        msgHdr.msg_name    = NULL;
        msgHdr.msg_namelen = 0;
        
    } else {

        /* We need the address */

        msgHdr.msg_name    = (void*) &addr;
        msgHdr.msg_namelen = sizeof(addr);
        sys_memzero((char *) msgHdr.msg_name, msgHdr.msg_namelen);
        if (!GET_MAP_VAL(env, eMsgHdr, esock_atom_addr, &eAddr))
            return esock_make_error(env, esock_atom_einval);

        SSDBG( descP, ("SOCKET", "nsendmsg -> not connected: "
                       "\r\n   address: %T"
                       "\r\n", eAddr) );

        if ((xres = esock_decode_sockaddr(env, eAddr,
                                          msgHdr.msg_name,
                                          &msgHdr.msg_namelen)) != NULL)
            return esock_make_error_str(env, xres);
    }


    /* Extract the (other) attributes of the msghdr map: iov and maybe ctrl */

    /* The *mandatory* iov, which must be a list */
    if (!GET_MAP_VAL(env, eMsgHdr, esock_atom_iov, &eIOV))
        return esock_make_error(env, esock_atom_einval);

    if (!GET_LIST_LEN(env, eIOV, &iovLen) && (iovLen > 0))
        return esock_make_error(env, esock_atom_einval);

    SSDBG( descP, ("SOCKET", "nsendmsg -> iov length: %d\r\n", iovLen) );

    iovBins = MALLOC(iovLen * sizeof(ErlNifBinary));
    ESOCK_ASSERT( (iovBins != NULL) );

    iov     = MALLOC(iovLen * sizeof(struct iovec));
    ESOCK_ASSERT( (iov != NULL) );

    /* The *opional* ctrl */
    if (GET_MAP_VAL(env, eMsgHdr, esock_atom_ctrl, &eCtrl)) {
        ctrlBufLen = descP->wCtrlSz;
        ctrlBuf    = (char*) MALLOC(ctrlBufLen);
        ESOCK_ASSERT( (ctrlBuf != NULL) );
    } else {
        eCtrl      = esock_atom_undefined;
        ctrlBufLen = 0;
        ctrlBuf    = NULL;
    }
    SSDBG( descP, ("SOCKET", "nsendmsg -> optional ctrl: "
                   "\r\n   ctrlBuf:    0x%lX"
                   "\r\n   ctrlBufLen: %d"
                   "\r\n   eCtrl:      %T\r\n", ctrlBuf, ctrlBufLen, eCtrl) );
    
    /* Decode the iov and initiate that part of the msghdr */
    if ((xres = esock_decode_iov(env, eIOV,
                                 iovBins, iov, iovLen, &dataSize)) != NULL) {
        FREE(iovBins);
        FREE(iov);
        if (ctrlBuf != NULL) FREE(ctrlBuf);
        return esock_make_error_str(env, xres);
    }
    msgHdr.msg_iov    = iov;
    msgHdr.msg_iovlen = iovLen;
    

    SSDBG( descP, ("SOCKET",
                   "nsendmsg -> total (iov) data size: %d\r\n", dataSize) );


    /* Decode the ctrl and initiate that part of the msghdr.
     */
    if (ctrlBuf != NULL) {
        if ((xres = decode_cmsghdrs(env, descP,
                                    eCtrl,
                                    ctrlBuf, ctrlBufLen, &ctrlBufUsed)) != NULL) {
            FREE(iovBins);
            FREE(iov);
            if (ctrlBuf != NULL) FREE(ctrlBuf);
            return esock_make_error_str(env, xres);
        }
    } else {
        ctrlBufUsed = 0;
    }
    msgHdr.msg_control    = ctrlBuf;
    msgHdr.msg_controllen = ctrlBufUsed;
    

    /* The msg-flags field is not used when sending, but zero it just in case */
    msgHdr.msg_flags      = 0;
    

    /* We ignore the wrap for the moment.
     * Maybe we should issue a wrap-message to controlling process...
     */
    cnt_inc(&descP->writeTries, 1);

    /* And now, finally, try to send the message */
    written = sock_sendmsg(descP->sock, &msgHdr, flags);

    if (IS_SOCKET_ERROR(written))
        save_errno = sock_errno();
    else
        save_errno = -1; // The value does not actually matter in this case

    res = send_check_result(env, descP, written, dataSize, save_errno, sendRef);

    FREE(iovBins);
    FREE(iov);
    if (ctrlBuf != NULL) FREE(ctrlBuf);
    
    return res;
}



/* ----------------------------------------------------------------------
 * nif_writev / nif_sendv
 *
 * Description:
 * Send a message (vector) on a socket
 *
 * Arguments:
 * Socket (ref) - Points to the socket descriptor.
 * SendRef      - A unique id for this (send) request.
 * Data         - A vector of binaries
 * Flags        - Send flags.
 */

#ifdef FOBAR
static
ERL_NIF_TERM nwritev(ErlNifEnv*        env,
                     SocketDescriptor* descP,
                     ERL_NIF_TERM      sendRef,
                     ERL_NIF_TERM      data)
{
    ERL_NIF_TERM tail;
    ErlNifIOVec  vec;
    ErlNifIOVec* iovec = &vec;
    SysIOVec*    sysiovec;
    int          save_errno;
    int          iovcnt, n;

    if (!enif_inspect_iovec(env, MAX_VSZ, data, &tail, &iovec))
        return enif_make_badarg(env);

    if (enif_ioq_size(descP->outQ) > 0) {
        /* If the I/O queue contains data we enqueue the iovec
         * and then peek the data to write out of the queue.
         */
        if (!enif_ioq_enqv(q, iovec, 0))
            return -3;

        sysiovec = enif_ioq_peek(descP->outQ, &iovcnt);

    } else {
        /* If the I/O queue is empty we skip the trip through it. */
        iovcnt   = iovec->iovcnt;
        sysiovec = iovec->iov;
    }

    /* Attempt to write the data */
    n = writev(fd, sysiovec, iovcnt);
    saved_errno = errno;

    if (enif_ioq_size(descP->outQ) == 0) {
        /* If the I/O queue was initially empty we enqueue any
           remaining data into the queue for writing later. */
        if (n >= 0 && !enif_ioq_enqv(descP->outQ, iovec, n))
            return -3;
    } else {
        /* Dequeue any data that was written from the queue. */
        if (n > 0 && !enif_ioq_deq(descP->outQ, n, NULL))
            return -4;
    }
    /* return n, which is either number of bytes written or -1 if
       some error happened */
    errno = saved_errno;
    return n;
}
#endif



/* ----------------------------------------------------------------------
 * nif_recv
 *
 * Description:
 * Receive a message on a socket.
 * Normally used only on a connected socket!
 * If we are trying to read > 0 bytes, then that is what we do.
 * But if we have specified 0 bytes, then we want to read
 * whatever is in the buffers (everything it got).
 *
 * Arguments:
 * Socket (ref) - Points to the socket descriptor.
 * RecvRef      - A unique id for this (send) request.
 * Length       - The number of bytes to receive.
 * Flags        - Receive flags.
 */

static
ERL_NIF_TERM nif_recv(ErlNifEnv*         env,
                      int                argc,
                      const ERL_NIF_TERM argv[])
{
    SocketDescriptor* descP;
    ERL_NIF_TERM      recvRef;
    int               len;
    unsigned int      eflags;
    int               flags;
    ERL_NIF_TERM      res;

    if ((argc != 4) ||
        !enif_get_resource(env, argv[0], sockets, (void**) &descP) ||
        !GET_INT(env, argv[2], &len) ||
        !GET_UINT(env, argv[3], &eflags)) {
        return enif_make_badarg(env);
    }
    recvRef  = argv[1];

    if (!IS_CONNECTED(descP))
        return esock_make_error(env, atom_enotconn);

    if (!erecvflags2recvflags(eflags, &flags))
        return enif_make_badarg(env);

    MLOCK(descP->readMtx);

    /* We need to handle the case when another process tries
     * to receive at the same time.
     * If the current recv could not read its entire package
     * this time (resulting in an select). The read of the
     * other process must be made to wait until current
     * is done!
     * Basically, we need a read queue!
     *
     * A 'reading' field (boolean), which is set if we did
     * not manage to read the entire message and reset every
     * time we do.
     */

    res = nrecv(env, descP, recvRef, len, flags);

    MUNLOCK(descP->readMtx);

    return res;

}


/* The (read) buffer handling *must* be optimized!
 * But for now we make it easy for ourselves by
 * allocating a binary (of the specified or default
 * size) and then throwing it away...
 */
static
ERL_NIF_TERM nrecv(ErlNifEnv*        env,
                   SocketDescriptor* descP,
                   ERL_NIF_TERM      recvRef,
                   int               len,
                   int               flags)
{
    ssize_t      read;
    ErlNifBinary buf;
    ERL_NIF_TERM readerCheck;
    int          save_errno;
    int          bufSz = (len ? len : descP->rBufSz);

    SSDBG( descP, ("SOCKET", "nrecv -> entry with"
                   "\r\n   len:   %d (%d)"
                   "\r\n   flags: %d"
                   "\r\n", len, bufSz, flags) );

    if (!descP->isReadable)
        return enif_make_badarg(env);

    /* Check if there is already a current reader and if its us */
    if (!recv_check_reader(env, descP, recvRef, &readerCheck))
        return readerCheck;
    
    /* Allocate a buffer:
     * Either as much as we want to read or (if zero (0)) use the "default"
     * size (what has been configured).
     */
    if (!ALLOC_BIN(bufSz, &buf))
        return esock_make_error(env, atom_exalloc);

    /* We ignore the wrap for the moment.
     * Maybe we should issue a wrap-message to controlling process...
     */
    cnt_inc(&descP->readTries, 1);

    // If it fails (read = -1), we need errno...
    SSDBG( descP, ("SOCKET", "nrecv -> try read (%d)\r\n", buf.size) );
    read = sock_recv(descP->sock, buf.data, buf.size, flags);
    if (IS_SOCKET_ERROR(read))
        save_errno = sock_errno();
    else
        save_errno = -1; // The value does not actually matter in this case
    
    SSDBG( descP, ("SOCKET", "nrecv -> read: %d (%d)\r\n", read, save_errno) );

    return recv_check_result(env, descP,
                             read, len,
                             save_errno,
                             &buf,
                             recvRef);
}



/* ----------------------------------------------------------------------
 * nif_recvfrom
 *
 * Description:
 * Receive a message on a socket.
 * Normally used only on a (un-) connected socket!
 * If a buffer size = 0 is specified, then the we will use the default
 * buffer size for this socket (whatever has been configured).
 *
 * Arguments:
 * Socket (ref) - Points to the socket descriptor.
 * RecvRef      - A unique id for this (send) request.
 * BufSz        - Size of the buffer into which we put the received message.
 * Flags        - Receive flags.
 *
 * <KOLLA>
 *
 * How do we handle if the peek flag is set? We need to basically keep
 * track of if we expect any data from the read. Regardless of the 
 * number of bytes we try to read.
 *
 * </KOLLA>
 */

static
ERL_NIF_TERM nif_recvfrom(ErlNifEnv*         env,
                          int                argc,
                          const ERL_NIF_TERM argv[])
{
    SocketDescriptor* descP;
    ERL_NIF_TERM      recvRef;
    unsigned int      bufSz;
    unsigned int      eflags;
    int               flags;
    ERL_NIF_TERM      res;

    SGDBG( ("SOCKET", "nif_recvfrom -> entry with argc: %d\r\n", argc) );

    /* Extract arguments and perform preliminary validation */

    if ((argc != 4) ||
        !enif_get_resource(env, argv[0], sockets, (void**) &descP) ||
        !GET_UINT(env, argv[2], &bufSz) ||
        !GET_UINT(env, argv[3], &eflags)) {
        return enif_make_badarg(env);
    }
    recvRef  = argv[1];

    SSDBG( descP,
           ("SOCKET", "nif_recvfrom -> args when sock = %d:"
            "\r\n   Socket:  %T"
            "\r\n   recvRef: %T"
            "\r\n   bufSz:   %d"
            "\r\n   eflags:  %d"
            "\r\n", descP->sock, argv[0], recvRef, bufSz, eflags) );

    /* if (IS_OPEN(descP)) */
    /*     return esock_make_error(env, atom_enotconn); */

    if (!erecvflags2recvflags(eflags, &flags))
        return enif_make_badarg(env);

    MLOCK(descP->readMtx);

    /* <KOLLA>
     * We need to handle the case when another process tries
     * to receive at the same time.
     * If the current recv could not read its entire package
     * this time (resulting in an select). The read of the
     * other process must be made to wait until current
     * is done!
     * Basically, we need a read queue!
     *
     * A 'reading' field (boolean), which is set if we did
     * not manage to read the entire message and reset every
     * time we do.
     * </KOLLA>
     */

    res = nrecvfrom(env, descP, recvRef, bufSz, flags);

    MUNLOCK(descP->readMtx);

    return res;

}


/* The (read) buffer handling *must* be optimized!
 * But for now we make it easy for ourselves by
 * allocating a binary (of the specified or default
 * size) and then throwing it away...
 */
static
ERL_NIF_TERM nrecvfrom(ErlNifEnv*        env,
                       SocketDescriptor* descP,
                       ERL_NIF_TERM      recvRef,
                       uint16_t          len,
                       int               flags)
{
    SocketAddress fromAddr;
    unsigned int  addrLen;
    ssize_t       read;
    int           save_errno;
    ErlNifBinary  buf;
    ERL_NIF_TERM  readerCheck;
    int           bufSz = (len ? len : descP->rBufSz);

    SSDBG( descP, ("SOCKET", "nrecvfrom -> entry with"
                   "\r\n   len:   %d (%d)"
                   "\r\n   flags: %d"
                   "\r\n", len, bufSz, flags) );

    if (!descP->isReadable)
        return enif_make_badarg(env);

    /* Check if there is already a current reader and if its us */
    if (!recv_check_reader(env, descP, recvRef, &readerCheck))
        return readerCheck;
    
    /* Allocate a buffer:
     * Either as much as we want to read or (if zero (0)) use the "default"
     * size (what has been configured).
     */
    if (!ALLOC_BIN(bufSz, &buf))
        return esock_make_error(env, atom_exalloc);

    /* We ignore the wrap for the moment.
     * Maybe we should issue a wrap-message to controlling process...
     */
    cnt_inc(&descP->readTries, 1);

    addrLen = sizeof(fromAddr);
    sys_memzero((char*) &fromAddr, addrLen);

    read = sock_recvfrom(descP->sock, buf.data, buf.size, flags,
                         &fromAddr.sa, &addrLen);
    if (IS_SOCKET_ERROR(read))
        save_errno = sock_errno();
    else
        save_errno = -1; // The value does not actually matter in this case

    return recvfrom_check_result(env, descP,
                                 read,
                                 save_errno,
                                 &buf,
                                 &fromAddr, addrLen,
                                 recvRef);
}



/* ----------------------------------------------------------------------
 * nif_recvmsg
 *
 * Description:
 * Receive a message on a socket.
 * Normally used only on a (un-) connected socket!
 * If a buffer size = 0 is specified, then we will use the default
 * buffer size for this socket (whatever has been configured).
 * If ctrl (buffer) size = 0 is specified, then the default ctrl
 * (buffer) size is used (1024). 
 *
 * Arguments:
 * Socket (ref) - Points to the socket descriptor.
 * RecvRef      - A unique id for this (send) request.
 * BufSz        - Size of the buffer into which we put the received message.
 * CtrlSz       - Size of the ctrl (buffer) into which we put the received 
 *                ancillary data.
 * Flags        - Receive flags.
 *
 * <KOLLA>
 *
 * How do we handle if the peek flag is set? We need to basically keep
 * track of if we expect any data from the read. Regardless of the 
 * number of bytes we try to read.
 *
 * </KOLLA>
 */

static
ERL_NIF_TERM nif_recvmsg(ErlNifEnv*         env,
                         int                argc,
                         const ERL_NIF_TERM argv[])
{
    SocketDescriptor* descP;
    ERL_NIF_TERM      recvRef;
    unsigned int      bufSz;
    unsigned int      ctrlSz;
    unsigned int      eflags;
    int               flags;
    ERL_NIF_TERM      res;

    SGDBG( ("SOCKET", "nif_recvmsg -> entry with argc: %d\r\n", argc) );

    /* Extract arguments and perform preliminary validation */

    if ((argc != 5) ||
        !enif_get_resource(env, argv[0], sockets, (void**) &descP) ||
        !GET_UINT(env, argv[2], &bufSz) ||
        !GET_UINT(env, argv[3], &ctrlSz) ||
        !GET_UINT(env, argv[4], &eflags)) {
        return enif_make_badarg(env);
    }
    recvRef  = argv[1];

    SSDBG( descP,
           ("SOCKET", "nif_recvmsg -> args when sock = %d:"
            "\r\n   Socket:  %T"
            "\r\n   recvRef: %T"
            "\r\n   bufSz:   %d"
            "\r\n   ctrlSz:  %d"
            "\r\n   eflags:  %d"
            "\r\n", descP->sock, argv[0], recvRef, bufSz, ctrlSz, eflags) );

    /* if (IS_OPEN(descP)) */
    /*     return esock_make_error(env, atom_enotconn); */

    if (!erecvflags2recvflags(eflags, &flags))
        return enif_make_badarg(env);

    MLOCK(descP->readMtx);

    /* <KOLLA>
     *
     * We need to handle the case when another process tries
     * to receive at the same time.
     * If the current recv could not read its entire package
     * this time (resulting in an select). The read of the
     * other process must be made to wait until current
     * is done!
     * Basically, we need a read queue!
     *
     * A 'reading' field (boolean), which is set if we did
     * not manage to read the entire message and reset every
     * time we do.
     *
     * </KOLLA>
     */

    res = nrecvmsg(env, descP, recvRef, bufSz, ctrlSz, flags);

    MUNLOCK(descP->readMtx);

    return res;

}


/* The (read) buffer handling *must* be optimized!
 * But for now we make it easy for ourselves by
 * allocating a binary (of the specified or default
 * size) and then throwing it away...
 */
static
ERL_NIF_TERM nrecvmsg(ErlNifEnv*        env,
                      SocketDescriptor* descP,
                      ERL_NIF_TERM      recvRef,
                      uint16_t          bufLen,
                      uint16_t          ctrlLen,
                      int               flags)
{
    unsigned int  addrLen;
    ssize_t       read;
    int           save_errno;
    int           bufSz  = (bufLen  ? bufLen  : descP->rBufSz);
    int           ctrlSz = (ctrlLen ? ctrlLen : descP->rCtrlSz);
    struct msghdr msgHdr;
    struct iovec  iov[1];  // Shall we always use 1?
    ErlNifBinary  data[1]; // Shall we always use 1?
    ErlNifBinary  ctrl;
    ERL_NIF_TERM  readerCheck;
    SocketAddress addr;

    SSDBG( descP, ("SOCKET", "nrecvmsg -> entry with"
                   "\r\n   bufSz:  %d (%d)"
                   "\r\n   ctrlSz: %d (%d)"
                   "\r\n   flags:  %d"
                   "\r\n", bufSz, bufLen, ctrlSz, ctrlLen, flags) );

    if (!descP->isReadable)
        return enif_make_badarg(env);

    /* Check if there is already a current reader and if its us */
    if (!recv_check_reader(env, descP, recvRef, &readerCheck))
        return readerCheck;
    
    /*
    for (i = 0; i < sizeof(buf); i++) {
        if (!ALLOC_BIN(bifSz, &buf[i]))
            return esock_make_error(env, atom_exalloc);
        iov[i].iov_base = buf[i].data;
        iov[i].iov_len  = buf[i].size;
    }
    */
    
    /* Allocate the (msg) data buffer:
     */
    if (!ALLOC_BIN(bufSz, &data[0]))
        return esock_make_error(env, atom_exalloc);

    /* Allocate the ctrl (buffer):
     */
    if (!ALLOC_BIN(ctrlSz, &ctrl))
        return esock_make_error(env, atom_exalloc);

    /* We ignore the wrap for the moment.
     * Maybe we should issue a wrap-message to controlling process...
     */
    cnt_inc(&descP->readTries, 1);

    addrLen = sizeof(addr);
    sys_memzero((char*) &addr,   addrLen);
    sys_memzero((char*) &msgHdr, sizeof(msgHdr));

    iov[0].iov_base = data[0].data;
    iov[0].iov_len  = data[0].size;
        
    msgHdr.msg_name       = &addr;
    msgHdr.msg_namelen    = addrLen;
    msgHdr.msg_iov        = iov;
    msgHdr.msg_iovlen     = 1; // Should use a constant or calculate...
    msgHdr.msg_control    = ctrl.data;
    msgHdr.msg_controllen = ctrl.size;

    read = sock_recvmsg(descP->sock, &msgHdr, flags);
    if (IS_SOCKET_ERROR(read))
        save_errno = sock_errno();
    else
        save_errno = -1; // The value does not actually matter in this case

    return recvmsg_check_result(env, descP,
                                read,
                                save_errno,
                                &msgHdr,
                                data,  // Needed for iov encode
                                &ctrl, // Needed for ctrl header encode
                                recvRef);
}



/* ----------------------------------------------------------------------
 * nif_close
 *
 * Description:
 * Close a (socket) file descriptor.
 *
 * Arguments:
 * Socket (ref) - Points to the socket descriptor.
 */

static
ERL_NIF_TERM nif_close(ErlNifEnv*         env,
                       int                argc,
                       const ERL_NIF_TERM argv[])
{
    SocketDescriptor* descP;

    if ((argc != 1) ||
        !enif_get_resource(env, argv[0], sockets, (void**) &descP)) {
        return enif_make_badarg(env);
    }

    return nclose(env, descP);
}


static
ERL_NIF_TERM nclose(ErlNifEnv*        env,
                    SocketDescriptor* descP)
{
    ERL_NIF_TERM reply, reason;
    BOOLEAN_T    doClose;
    int          selectRes;
    int          domain   = descP->domain;
    int          type     = descP->type;
    int          protocol = descP->protocol;

    SSDBG( descP, ("SOCKET", "nclose -> [%d] entry\r\n", descP->sock) );

    MLOCK(descP->closeMtx);

    if (descP->state == SOCKET_STATE_CLOSED) {
        reason  = atom_closed;
        doClose = FALSE;
    } else if (descP->state == SOCKET_STATE_CLOSING) {
        reason  = atom_closing;
        doClose = FALSE;
    } else {

        /* Store the PID of the caller,
         * since we need to inform it when we
         * (that is, the stop callback function)
         * completes.
         */

        if (enif_self(env, &descP->closerPid) == NULL) {
            MUNLOCK(descP->closeMtx);
            return esock_make_error(env, atom_exself);
        }

        /* Monitor the caller, since we should complete this operation even if
         * the caller dies (for whatever reason).
         */

        if (MONP(env, descP,
             &descP->closerPid,
                 &descP->closerMon) > 0) {
            MUNLOCK(descP->closeMtx);
            return esock_make_error(env, atom_exmon);
        }

        descP->closeLocal = TRUE;
        descP->state      = SOCKET_STATE_CLOSING;
        doClose           = TRUE;
    }

    MUNLOCK(descP->closeMtx);

    if (doClose) {
        descP->closeRef = MKREF(env);
        selectRes       = enif_select(env, descP->sock, (ERL_NIF_SELECT_STOP),
                                      descP, NULL, descP->closeRef);
        if (selectRes & ERL_NIF_SELECT_STOP_CALLED) {
            /* Prep done - inform the caller it can finalize (close) directly */
            SSDBG( descP,
                   ("SOCKET", "nclose -> [%d] stop called\r\n", descP->sock) );
            dec_socket(domain, type, protocol);
            reply = esock_atom_ok;
        } else if (selectRes & ERL_NIF_SELECT_STOP_SCHEDULED) {
            /* The stop callback function has been *scheduled* which means that we
             * have to wait for it to complete. */
            SSDBG( descP,
                   ("SOCKET", "nclose -> [%d] stop scheduled\r\n", descP->sock) );
            dec_socket(domain, type, protocol); // SHALL WE DO THIS AT finalize?
            reply = esock_make_ok2(env, descP->closeRef);
        } else {
            /* <KOLLA>
             *
             * WE SHOULD REALLY HAVE A WAY TO CLOBBER THE SOCKET,
             * SO WE DON'T LET STUFF LEAK.
             * NOW, BECAUSE WE FAILED TO SELECT, WE CANNOT FINISH
             * THE CLOSE, WHAT TO DO? ABORT?
             *
             * </KOLLA>
             */
            reason = MKT2(env, atom_select, MKI(env, selectRes));
            reply  = esock_make_error(env, reason);
        }
    } else {
        reply = esock_make_error(env, reason);
    }

    SSDBG( descP,
           ("SOCKET", "nclose -> [%d] done when: "
            "\r\n   reply: %T"
            "\r\n", descP->sock, reply) );

    return reply;
}



/* ----------------------------------------------------------------------
 * nif_finalize_close
 *
 * Description:
 * Perform the actual socket close!
 * Note that this function is executed in a dirty scheduler.
 *
 * Arguments:
 * Socket (ref) - Points to the socket descriptor.
 */
static
ERL_NIF_TERM nif_finalize_close(ErlNifEnv*         env,
                                int                argc,
                                const ERL_NIF_TERM argv[])
{
    SocketDescriptor* descP;

    /* Extract arguments and perform preliminary validation */

    if ((argc != 1) ||
        !enif_get_resource(env, argv[0], sockets, (void**) &descP)) {
        return enif_make_badarg(env);
    }

    return nfinalize_close(env, descP);

}


/* *** nfinalize_close ***
 * Perform the final step in the socket close.
 */
static
ERL_NIF_TERM nfinalize_close(ErlNifEnv*        env,
                             SocketDescriptor* descP)
{
    ERL_NIF_TERM reply;

    if (descP->state == SOCKET_STATE_CLOSED)
        return esock_atom_ok;

    if (descP->state != SOCKET_STATE_CLOSING)
        return esock_make_error(env, atom_enotclosing);

    /* This nif is executed in a dirty scheduler just so that
     * it can "hang" (whith minumum effect on the VM) while the
     * kernel writes our buffers. IF we have set the linger option
     * for this ({true, integer() > 0}). For this to work we must
     * be blocking...
     */
    SET_BLOCKING(descP->sock);

    if (sock_close(descP->sock) != 0) {
        int save_errno = sock_errno();

        if (save_errno != ERRNO_BLOCK) {
            /* Not all data in the buffers where sent,
             * make sure the caller gets this.
             */
            reply = esock_make_error(env, atom_timeout);
        } else {
            reply = esock_make_error_errno(env, save_errno);
        }
    } else {
        reply = esock_atom_ok;
    }
    sock_close_event(descP->event);

    descP->sock  = INVALID_SOCKET;
    descP->event = INVALID_EVENT;

    descP->state = SOCKET_STATE_CLOSED;

    return reply;
}



/* ----------------------------------------------------------------------
 * nif_shutdown
 *
 * Description:
 * Disable sends and/or receives on a socket.
 *
 * Arguments:
 * [0] Socket (ref) - Points to the socket descriptor.
 * [1] How          - What will be shutdown.
 */

static
ERL_NIF_TERM nif_shutdown(ErlNifEnv*         env,
                          int                argc,
                          const ERL_NIF_TERM argv[])
{
    SocketDescriptor* descP;
    unsigned int      ehow;
    int               how;

    if ((argc != 2) ||
        !enif_get_resource(env, argv[0], sockets, (void**) &descP) ||
        !GET_UINT(env, argv[1], &ehow)) {
        return enif_make_badarg(env);
    }

    if (!ehow2how(ehow, &how))
        return enif_make_badarg(env);

    return nshutdown(env, descP, how);
}



static
ERL_NIF_TERM nshutdown(ErlNifEnv*        env,
                       SocketDescriptor* descP,
                       int               how)
{
    ERL_NIF_TERM reply;

    if (sock_shutdown(descP->sock, how) == 0) {
        switch (how) {
        case SHUT_RD:
            descP->isReadable = FALSE;
            break;
        case SHUT_WR:
            descP->isWritable = FALSE;
            break;
        case SHUT_RDWR:
            descP->isReadable = FALSE;
            descP->isWritable = FALSE;
            break;
        }
        reply = esock_atom_ok;
    } else {
        reply = esock_make_error_errno(env, sock_errno());
    }

    return reply;
}




/* ----------------------------------------------------------------------
 * nif_setopt
 *
 * Description:
 * Set socket option.
 * Its possible to use a "raw" mode (not encoded). That is, we do not
 * interpret level, opt and value. They are passed "as is" to the
 * setsockopt function call (the value arguments is assumed to be a
 * binary, already encoded).
 *
 * Arguments:
 * Socket (ref) - Points to the socket descriptor.
 * Encoded      - Are the "arguments" encoded or not.
 * Level        - Level of the socket option.
 * Opt          - The socket option.
 * Value        - Value of the socket option (type depend on the option).
 */

static
ERL_NIF_TERM nif_setopt(ErlNifEnv*         env,
                        int                argc,
                        const ERL_NIF_TERM argv[])
{
    SocketDescriptor* descP = NULL;
    int               eLevel, level = -1;
    int               eOpt;
    ERL_NIF_TERM      eIsEncoded;
    ERL_NIF_TERM      eVal;
    BOOLEAN_T         isEncoded, isOTP;
    ERL_NIF_TERM      result;

    SGDBG( ("SOCKET", "nif_setopt -> entry with argc: %d\r\n", argc) );

    /* Extract arguments and perform preliminary validation */

    if ((argc != 5) ||
        !enif_get_resource(env, argv[0], sockets, (void**) &descP) ||
        !GET_INT(env, argv[2], &eLevel) ||
        !GET_INT(env, argv[3], &eOpt)) {
        SGDBG( ("SOCKET", "nif_setopt -> failed initial arg check\r\n") );
        return enif_make_badarg(env);
    }
    eIsEncoded = argv[1];
    eVal       = argv[4];

    isEncoded = esock_decode_bool(eIsEncoded);

    /* SGDBG( ("SOCKET", "nif_setopt -> eIsDecoded (%T) decoded: %d\r\n", */
    /*         eIsEncoded, isEncoded) ); */

    if (!elevel2level(isEncoded, eLevel, &isOTP, &level)) {
        SSDBG( descP, ("SOCKET", "nif_seopt -> failed decode level\r\n") );
        return esock_make_error(env, esock_atom_einval);
    }

    SSDBG( descP,
           ("SOCKET", "nif_setopt -> args when sock = %d:"
            "\r\n   Socket:  %T"
            "\r\n   Encoded: %d (%T)"
            "\r\n   Level:   %d (%d)"
            "\r\n   Opt:     %d"
            "\r\n   Value:   %T"
            "\r\n",
            descP->sock, argv[0],
            isEncoded, eIsEncoded,
            level, eLevel,
            eOpt, eVal) );

    result = nsetopt(env, descP, isEncoded, isOTP, level, eOpt, eVal);

    SSDBG( descP,
           ("SOCKET", "nif_setopt -> done when"
            "\r\n   result: %T"
            "\r\n", result) );

    return result;
}


static
ERL_NIF_TERM nsetopt(ErlNifEnv*        env,
                     SocketDescriptor* descP,
                     BOOLEAN_T         isEncoded,
                     BOOLEAN_T         isOTP,
                     int               level,
                     int               eOpt,
                     ERL_NIF_TERM      eVal)
{
    ERL_NIF_TERM result;

    if (isOTP) {
        /* These are not actual socket options,
         * but options for our implementation.
         */
        result = nsetopt_otp(env, descP, eOpt, eVal);
    } else if (!isEncoded) {
        result = nsetopt_native(env, descP, level, eOpt, eVal);
    } else {
        result = nsetopt_level(env, descP, level, eOpt, eVal);
    }

    return result;
}



/* nsetopt_otp - Handle OTP (level) options
 */
static
ERL_NIF_TERM nsetopt_otp(ErlNifEnv*        env,
                         SocketDescriptor* descP,
                         int               eOpt,
                         ERL_NIF_TERM      eVal)
{
    ERL_NIF_TERM result;

    SSDBG( descP,
           ("SOCKET", "nsetopt_otp -> entry with"
            "\r\n   eOpt: %d"
            "\r\n   eVal: %T"
            "\r\n", eOpt, eVal) );

    switch (eOpt) {
    case SOCKET_OPT_OTP_DEBUG:
        result = nsetopt_otp_debug(env, descP, eVal);
        break;

    case SOCKET_OPT_OTP_IOW:
        result = nsetopt_otp_iow(env, descP, eVal);
        break;

    case SOCKET_OPT_OTP_CTRL_PROC:
        result = nsetopt_otp_ctrl_proc(env, descP, eVal);
        break;

    case SOCKET_OPT_OTP_RCVBUF:
        result = nsetopt_otp_rcvbuf(env, descP, eVal);
        break;

    case SOCKET_OPT_OTP_RCVCTRLBUF:
        result = nsetopt_otp_rcvctrlbuf(env, descP, eVal);
        break;

    case SOCKET_OPT_OTP_SNDCTRLBUF:
        result = nsetopt_otp_sndctrlbuf(env, descP, eVal);
        break;

    default:
        result = esock_make_error(env, esock_atom_einval);
        break;
    }

    return result;
}


/* nsetopt_otp_debug - Handle the OTP (level) debug options
 */
static
ERL_NIF_TERM nsetopt_otp_debug(ErlNifEnv*        env,
                               SocketDescriptor* descP,
                               ERL_NIF_TERM      eVal)
{
    descP->dbg = esock_decode_bool(eVal);

    return esock_atom_ok;
}


/* nsetopt_otp_iow - Handle the OTP (level) iow options
 */
static
ERL_NIF_TERM nsetopt_otp_iow(ErlNifEnv*        env,
                             SocketDescriptor* descP,
                             ERL_NIF_TERM      eVal)
{
    descP->iow = esock_decode_bool(eVal);

    return esock_atom_ok;
}



/* nsetopt_otp_ctrl_proc - Handle the OTP (level) controlling_process options
 */
static
ERL_NIF_TERM nsetopt_otp_ctrl_proc(ErlNifEnv*        env,
                                   SocketDescriptor* descP,
                                   ERL_NIF_TERM      eVal)
{
    ErlNifPid     newCtrlPid;
    ErlNifMonitor newCtrlMon;
    int           xres;

    SSDBG( descP,
           ("SOCKET", "nsetopt_otp_ctrl_proc -> entry with"
            "\r\n   eVal: %T"
            "\r\n", eVal) );

    if (!GET_LPID(env, eVal, &newCtrlPid)) {
        esock_warning_msg("Failed get pid of new controlling process\r\n");
        return esock_make_error(env, esock_atom_einval);
    }

    if ((xres = MONP(env, descP, &newCtrlPid, &newCtrlMon)) != 0) {
        esock_warning_msg("Failed monitor %d) (new) controlling process\r\n", xres);
        return esock_make_error(env, esock_atom_einval);
    }

    if ((xres = DEMONP(env, descP, &descP->ctrlMon)) != 0) {
        esock_warning_msg("Failed demonitor (%d) "
                          "old controlling process %T (%T)\r\n",
                          xres, descP->ctrlPid, descP->ctrlMon);
    }

    descP->ctrlPid = newCtrlPid;
    descP->ctrlMon = newCtrlMon;

    SSDBG( descP, ("SOCKET", "nsetopt_otp_ctrl_proc -> done\r\n") );

    return esock_atom_ok;
}



/* nsetopt_otp_rcvbuf - Handle the OTP (level) rcvbuf option
 */
static
ERL_NIF_TERM nsetopt_otp_rcvbuf(ErlNifEnv*        env,
                                SocketDescriptor* descP,
                                ERL_NIF_TERM      eVal)
{
    size_t val;
    char*  xres;

    if ((xres = esock_decode_bufsz(env,
                                   eVal,
                                   SOCKET_RECV_BUFFER_SIZE_DEFAULT, &val)) != NULL)
        return esock_make_error_str(env, xres);

    descP->rBufSz = val;
    
    return esock_atom_ok;
}



/* nsetopt_otp_rcvctrlbuf - Handle the OTP (level) rcvctrlbuf option
 */
static
ERL_NIF_TERM nsetopt_otp_rcvctrlbuf(ErlNifEnv*        env,
                                    SocketDescriptor* descP,
                                    ERL_NIF_TERM      eVal)
{
    size_t val;
    char*  xres;

    if ((xres = esock_decode_bufsz(env,
                                   eVal,
                                   SOCKET_RECV_CTRL_BUFFER_SIZE_DEFAULT,
                                   &val)) != NULL)
        return esock_make_error_str(env, xres);

    descP->rCtrlSz = val;
    
    return esock_atom_ok;
}



/* nsetopt_otp_sndctrlbuf - Handle the OTP (level) sndctrlbuf option
 */
static
ERL_NIF_TERM nsetopt_otp_sndctrlbuf(ErlNifEnv*        env,
                                    SocketDescriptor* descP,
                                    ERL_NIF_TERM      eVal)
{
    size_t val;
    char*  xres;

    if ((xres = esock_decode_bufsz(env,
                                   eVal,
                                   SOCKET_SEND_CTRL_BUFFER_SIZE_DEFAULT,
                                   &val)) != NULL)
        return esock_make_error_str(env, xres);

    descP->wCtrlSz = val;
    
    return esock_atom_ok;
}



/* The option has *not* been encoded. Instead it has been provided
 * in "native mode" (option is provided as is and value as a binary).
 */
static
ERL_NIF_TERM nsetopt_native(ErlNifEnv*        env,
                            SocketDescriptor* descP,
                            int               level,
                            int               opt,
                            ERL_NIF_TERM      eVal)
{
    ErlNifBinary val;
    ERL_NIF_TERM result;

    SSDBG( descP,
           ("SOCKET", "nsetopt_native -> entry with"
            "\r\n   level: %d"
            "\r\n   opt:   %d"
            "\r\n   eVal:  %T"
            "\r\n", level, opt, eVal) );

    if (GET_BIN(env, eVal, &val)) {
        int res = socket_setopt(descP->sock, level, opt,
                                val.data, val.size);
        if (res != 0)
            result = esock_make_error_errno(env, sock_errno());
        else
            result = esock_atom_ok;
    } else {
        result = esock_make_error(env, esock_atom_einval);
    }

    SSDBG( descP,
           ("SOCKET", "nsetopt_native -> done when"
            "\r\n   result: %T"
            "\r\n", result) );

    return result;
}



/* nsetopt_level - A "proper" level (option) has been specified
 */
static
ERL_NIF_TERM nsetopt_level(ErlNifEnv*        env,
                           SocketDescriptor* descP,
                           int               level,
                           int               eOpt,
                           ERL_NIF_TERM      eVal)
{
    ERL_NIF_TERM result;

    SSDBG( descP,
           ("SOCKET", "nsetopt_level -> entry with"
            "\r\n   level: %d"
            "\r\n", level) );

    switch (level) {
    case SOL_SOCKET:
        result = nsetopt_lvl_socket(env, descP, eOpt, eVal);
        break;

#if defined(SOL_IP)
    case SOL_IP:
#else
    case IPPROTO_IP:
#endif
        result = nsetopt_lvl_ip(env, descP, eOpt, eVal);
        break;

#if defined(SOL_IPV6)
    case SOL_IPV6:
        result = nsetopt_lvl_ipv6(env, descP, eOpt, eVal);
        break;
#endif

    case IPPROTO_TCP:
        result = nsetopt_lvl_tcp(env, descP, eOpt, eVal);
        break;

    case IPPROTO_UDP:
        result = nsetopt_lvl_udp(env, descP, eOpt, eVal);
        break;

#if defined(HAVE_SCTP)
    case IPPROTO_SCTP:
        result = nsetopt_lvl_sctp(env, descP, eOpt, eVal);
        break;
#endif

    default:
        SSDBG( descP,
               ("SOCKET", "nsetopt_level -> unknown level (%d)\r\n", level) );
        result = esock_make_error(env, esock_atom_einval);
        break;
    }

    SSDBG( descP,
           ("SOCKET", "nsetopt_level -> done when"
            "\r\n   result: %T"
            "\r\n", result) );

    return result;
}



/* nsetopt_lvl_socket - Level *SOCKET* option
 */
static
ERL_NIF_TERM nsetopt_lvl_socket(ErlNifEnv*        env,
                                SocketDescriptor* descP,
                                int               eOpt,
                                ERL_NIF_TERM      eVal)
{
    ERL_NIF_TERM result;

    SSDBG( descP,
           ("SOCKET", "nsetopt_lvl_socket -> entry with"
            "\r\n   opt: %d"
            "\r\n", eOpt) );

    switch (eOpt) {
#if defined(SO_BINDTODEVICE)
    case SOCKET_OPT_SOCK_BINDTODEVICE:
        result = nsetopt_lvl_sock_bindtodevice(env, descP, eVal);
        break;
#endif

#if defined(SO_BROADCAST)
    case SOCKET_OPT_SOCK_BROADCAST:
        result = nsetopt_lvl_sock_broadcast(env, descP, eVal);
        break;
#endif

#if defined(SO_DEBUG)
    case SOCKET_OPT_SOCK_DEBUG:
        result = nsetopt_lvl_sock_debug(env, descP, eVal);
        break;
#endif

#if defined(SO_DONTROUTE)
    case SOCKET_OPT_SOCK_DONTROUTE:
        result = nsetopt_lvl_sock_dontroute(env, descP, eVal);
        break;
#endif

#if defined(SO_KEEPALIVE)
    case SOCKET_OPT_SOCK_KEEPALIVE:
        result = nsetopt_lvl_sock_keepalive(env, descP, eVal);
        break;
#endif

#if defined(SO_LINGER)
    case SOCKET_OPT_SOCK_LINGER:
        result = nsetopt_lvl_sock_linger(env, descP, eVal);
        break;
#endif

#if defined(SO_PEEK_OFF)
    case SOCKET_OPT_SOCK_PEEK_OFF:
        result = nsetopt_lvl_sock_peek_off(env, descP, eVal);
        break;
#endif

#if defined(SO_OOBINLINE)
    case SOCKET_OPT_SOCK_OOBINLINE:
        result = nsetopt_lvl_sock_oobinline(env, descP, eVal);
        break;
#endif

#if defined(SO_PRIORITY)
    case SOCKET_OPT_SOCK_PRIORITY:
        result = nsetopt_lvl_sock_priority(env, descP, eVal);
        break;
#endif

#if defined(SO_RCVBUF)
    case SOCKET_OPT_SOCK_RCVBUF:
        result = nsetopt_lvl_sock_rcvbuf(env, descP, eVal);
        break;
#endif

#if defined(SO_RCVLOWAT)
    case SOCKET_OPT_SOCK_RCVLOWAT:
        result = nsetopt_lvl_sock_rcvlowat(env, descP, eVal);
        break;
#endif

#if defined(SO_RCVTIMEO)
    case SOCKET_OPT_SOCK_RCVTIMEO:
        result = nsetopt_lvl_sock_rcvtimeo(env, descP, eVal);
        break;
#endif

#if defined(SO_REUSEADDR)
    case SOCKET_OPT_SOCK_REUSEADDR:
        result = nsetopt_lvl_sock_reuseaddr(env, descP, eVal);
        break;
#endif

#if defined(SO_REUSEPORT)
    case SOCKET_OPT_SOCK_REUSEPORT:
        result = nsetopt_lvl_sock_reuseport(env, descP, eVal);
        break;
#endif

#if defined(SO_SNDBUF)
    case SOCKET_OPT_SOCK_SNDBUF:
        result = nsetopt_lvl_sock_sndbuf(env, descP, eVal);
        break;
#endif

#if defined(SO_SNDLOWAT)
    case SOCKET_OPT_SOCK_SNDLOWAT:
        result = nsetopt_lvl_sock_sndlowat(env, descP, eVal);
        break;
#endif

#if defined(SO_SNDTIMEO)
    case SOCKET_OPT_SOCK_SNDTIMEO:
        result = nsetopt_lvl_sock_sndtimeo(env, descP, eVal);
        break;
#endif

#if defined(SO_TIMESTAMP)
    case SOCKET_OPT_SOCK_TIMESTAMP:
        result = nsetopt_lvl_sock_timestamp(env, descP, eVal);
        break;
#endif

    default:
        SSDBG( descP,
               ("SOCKET", "nsetopt_lvl_socket -> unknown opt (%d)\r\n", eOpt) );
        result = esock_make_error(env, esock_atom_einval);
        break;
    }

    SSDBG( descP,
           ("SOCKET", "nsetopt_lvl_socket -> done when"
            "\r\n   result: %T"
            "\r\n", result) );

    return result;
}


#if defined(SO_BINDTODEVICE)
static
ERL_NIF_TERM nsetopt_lvl_sock_bindtodevice(ErlNifEnv*        env,
                                           SocketDescriptor* descP,
                                           ERL_NIF_TERM      eVal)
{
    return nsetopt_str_opt(env, descP,
                           SOL_SOCKET, SO_BROADCAST,
                           IFNAMSIZ, eVal);
}
#endif


#if defined(SO_BROADCAST)
static
ERL_NIF_TERM nsetopt_lvl_sock_broadcast(ErlNifEnv*        env,
                                        SocketDescriptor* descP,
                                        ERL_NIF_TERM      eVal)
{
    return nsetopt_bool_opt(env, descP, SOL_SOCKET, SO_BROADCAST, eVal);
}
#endif


#if defined(SO_DEBUG)
static
ERL_NIF_TERM nsetopt_lvl_sock_debug(ErlNifEnv*        env,
                                    SocketDescriptor* descP,
                                    ERL_NIF_TERM      eVal)
{
    return nsetopt_int_opt(env, descP, SOL_SOCKET, SO_DEBUG, eVal);
}
#endif


#if defined(SO_DONTROUTE)
static
ERL_NIF_TERM nsetopt_lvl_sock_dontroute(ErlNifEnv*        env,
                                        SocketDescriptor* descP,
                                        ERL_NIF_TERM      eVal)
{
    return nsetopt_bool_opt(env, descP, SOL_SOCKET, SO_DONTROUTE, eVal);
}
#endif


#if defined(SO_KEEPALIVE)
static
ERL_NIF_TERM nsetopt_lvl_sock_keepalive(ErlNifEnv*        env,
                                        SocketDescriptor* descP,
                                        ERL_NIF_TERM      eVal)
{
    return nsetopt_bool_opt(env, descP, SOL_SOCKET, SO_KEEPALIVE, eVal);
}
#endif


#if defined(SO_LINGER)
static
ERL_NIF_TERM nsetopt_lvl_sock_linger(ErlNifEnv*        env,
                                     SocketDescriptor* descP,
                                     ERL_NIF_TERM      eVal)
{
    ERL_NIF_TERM  result;
    struct linger val;

    if (decode_sock_linger(env, eVal, &val)) {
        int optLen = sizeof(val);
        int res    = socket_setopt(descP->sock, SOL_SOCKET, SO_LINGER,
                                   (void*) &val, optLen);
        if (res != 0)
            result = esock_make_error_errno(env, sock_errno());
        else
            result = esock_atom_ok;
    } else {
        result = esock_make_error(env, esock_atom_einval);
    }

    return result;
}
#endif


#if defined(SO_OOBINLINE)
static
ERL_NIF_TERM nsetopt_lvl_sock_oobinline(ErlNifEnv*        env,
                                        SocketDescriptor* descP,
                                        ERL_NIF_TERM      eVal)
{
    return nsetopt_bool_opt(env, descP, SOL_SOCKET, SO_OOBINLINE, eVal);
}
#endif


#if defined(SO_PEEK_OFF)
static
ERL_NIF_TERM nsetopt_lvl_sock_peek_off(ErlNifEnv*        env,
                                       SocketDescriptor* descP,
                                       ERL_NIF_TERM      eVal)
{
    return nsetopt_int_opt(env, descP, SOL_SOCKET, SO_PEEK_OFF, eVal);
}
#endif


#if defined(SO_PRIORITY)
static
ERL_NIF_TERM nsetopt_lvl_sock_priority(ErlNifEnv*        env,
                                       SocketDescriptor* descP,
                                       ERL_NIF_TERM      eVal)
{
    return nsetopt_int_opt(env, descP, SOL_SOCKET, SO_PRIORITY, eVal);
}
#endif


#if defined(SO_RCVBUF)
static
ERL_NIF_TERM nsetopt_lvl_sock_rcvbuf(ErlNifEnv*        env,
                                     SocketDescriptor* descP,
                                     ERL_NIF_TERM      eVal)
{
    return nsetopt_int_opt(env, descP, SOL_SOCKET, SO_RCVBUF, eVal);
}
#endif


#if defined(SO_RCVLOWAT)
static
ERL_NIF_TERM nsetopt_lvl_sock_rcvlowat(ErlNifEnv*        env,
                                     SocketDescriptor* descP,
                                     ERL_NIF_TERM      eVal)
{
    return nsetopt_int_opt(env, descP, SOL_SOCKET, SO_RCVLOWAT, eVal);
}
#endif


#if defined(SO_RCVTIMEO)
static
ERL_NIF_TERM nsetopt_lvl_sock_rcvtimeo(ErlNifEnv*        env,
                                       SocketDescriptor* descP,
                                       ERL_NIF_TERM      eVal)
{
    return nsetopt_timeval_opt(env, descP, SOL_SOCKET, SO_RCVTIMEO, eVal);
}
#endif


#if defined(SO_REUSEADDR)
static
ERL_NIF_TERM nsetopt_lvl_sock_reuseaddr(ErlNifEnv*        env,
                                        SocketDescriptor* descP,
                                        ERL_NIF_TERM      eVal)
{
    return nsetopt_bool_opt(env, descP, SOL_SOCKET, SO_REUSEADDR, eVal);
}
#endif


#if defined(SO_REUSEPORT)
static
ERL_NIF_TERM nsetopt_lvl_sock_reuseport(ErlNifEnv*        env,
                                        SocketDescriptor* descP,
                                        ERL_NIF_TERM      eVal)
{
    return nsetopt_bool_opt(env, descP, SOL_SOCKET, SO_REUSEPORT, eVal);
}
#endif


#if defined(SO_SNDBUF)
static
ERL_NIF_TERM nsetopt_lvl_sock_sndbuf(ErlNifEnv*        env,
                                     SocketDescriptor* descP,
                                     ERL_NIF_TERM      eVal)
{
    return nsetopt_int_opt(env, descP, SOL_SOCKET, SO_SNDBUF, eVal);
}
#endif


#if defined(SO_SNDLOWAT)
static
ERL_NIF_TERM nsetopt_lvl_sock_sndlowat(ErlNifEnv*        env,
                                       SocketDescriptor* descP,
                                       ERL_NIF_TERM      eVal)
{
    return nsetopt_int_opt(env, descP, SOL_SOCKET, SO_SNDLOWAT, eVal);
}
#endif


#if defined(SO_SNDTIMEO)
static
ERL_NIF_TERM nsetopt_lvl_sock_sndtimeo(ErlNifEnv*        env,
                                       SocketDescriptor* descP,
                                       ERL_NIF_TERM      eVal)
{
    SSDBG( descP,
           ("SOCKET", "nsetopt_lvl_sock_sndtimeo -> entry with"
            "\r\n   eVal: %T"
            "\r\n", eVal) );

    return nsetopt_timeval_opt(env, descP, SOL_SOCKET, SO_SNDTIMEO, eVal);
}
#endif


#if defined(SO_TIMESTAMP)
static
ERL_NIF_TERM nsetopt_lvl_sock_timestamp(ErlNifEnv*        env,
                                        SocketDescriptor* descP,
                                        ERL_NIF_TERM      eVal)
{
    return nsetopt_bool_opt(env, descP, SOL_SOCKET, SO_TIMESTAMP, eVal);
}
#endif



/* nsetopt_lvl_ip - Level *IP* option(s)
 */
static
ERL_NIF_TERM nsetopt_lvl_ip(ErlNifEnv*        env,
                            SocketDescriptor* descP,
                            int               eOpt,
                            ERL_NIF_TERM      eVal)
{
    ERL_NIF_TERM result;

    SSDBG( descP,
           ("SOCKET", "nsetopt_lvl_ip -> entry with"
            "\r\n   opt: %d"
            "\r\n", eOpt) );

    switch (eOpt) {
#if defined(IP_ADD_MEMBERSHIP)
    case SOCKET_OPT_IP_ADD_MEMBERSHIP:
        result = nsetopt_lvl_ip_add_membership(env, descP, eVal);
        break;
#endif

#if defined(IP_ADD_SOURCE_MEMBERSHIP)
    case SOCKET_OPT_IP_ADD_SOURCE_MEMBERSHIP:
        result = nsetopt_lvl_ip_add_source_membership(env, descP, eVal);
        break;
#endif

#if defined(IP_BLOCK_SOURCE)
    case SOCKET_OPT_IP_BLOCK_SOURCE:
        result = nsetopt_lvl_ip_block_source(env, descP, eVal);
        break;
#endif

#if defined(IP_DROP_MEMBERSHIP)
    case SOCKET_OPT_IP_DROP_MEMBERSHIP:
        result = nsetopt_lvl_ip_drop_membership(env, descP, eVal);
        break;
#endif

#if defined(IP_DROP_SOURCE_MEMBERSHIP)
    case SOCKET_OPT_IP_DROP_SOURCE_MEMBERSHIP:
        result = nsetopt_lvl_ip_drop_source_membership(env, descP, eVal);
        break;
#endif

#if defined(IP_FREEBIND)
    case SOCKET_OPT_IP_FREEBIND:
        result = nsetopt_lvl_ip_freebind(env, descP, eVal);
        break;
#endif

#if defined(IP_HDRINCL)
    case SOCKET_OPT_IP_HDRINCL:
        result = nsetopt_lvl_ip_hdrincl(env, descP, eVal);
        break;
#endif

#if defined(IP_MINTTL)
    case SOCKET_OPT_IP_MINTTL:
        result = nsetopt_lvl_ip_minttl(env, descP, eVal);
        break;
#endif

#if defined(IP_MSFILTER) && defined(IP_MSFILTER_SIZE)
    case SOCKET_OPT_IP_MSFILTER:
        result = nsetopt_lvl_ip_msfilter(env, descP, eVal);
        break;
#endif

#if defined(IP_MTU_DISCOVER)
    case SOCKET_OPT_IP_MTU_DISCOVER:
        result = nsetopt_lvl_ip_mtu_discover(env, descP, eVal);
        break;
#endif

#if defined(IP_MULTICAST_ALL)
    case SOCKET_OPT_IP_MULTICAST_ALL:
        result = nsetopt_lvl_ip_multicast_all(env, descP, eVal);
        break;
#endif

#if defined(IP_MULTICAST_IF)
    case SOCKET_OPT_IP_MULTICAST_IF:
        result = nsetopt_lvl_ip_multicast_if(env, descP, eVal);
        break;
#endif

#if defined(IP_MULTICAST_LOOP)
    case SOCKET_OPT_IP_MULTICAST_LOOP:
        result = nsetopt_lvl_ip_multicast_loop(env, descP, eVal);
        break;
#endif

#if defined(IP_MULTICAST_TTL)
    case SOCKET_OPT_IP_MULTICAST_TTL:
        result = nsetopt_lvl_ip_multicast_ttl(env, descP, eVal);
        break;
#endif

#if defined(IP_NODEFRAG)
    case SOCKET_OPT_IP_NODEFRAG:
        result = nsetopt_lvl_ip_nodefrag(env, descP, eVal);
        break;
#endif

#if defined(IP_PKTINFO)
    case SOCKET_OPT_IP_PKTINFO:
        result = nsetopt_lvl_ip_pktinfo(env, descP, eVal);
        break;
#endif

#if defined(IP_RECVDSTADDR)
    case SOCKET_OPT_IP_RECVDSTADDR:
        result = nsetopt_lvl_ip_recvdstaddr(env, descP, eVal);
        break;
#endif

#if defined(IP_RECVERR)
    case SOCKET_OPT_IP_RECVERR:
        result = nsetopt_lvl_ip_recverr(env, descP, eVal);
        break;
#endif

#if defined(IP_RECVIF)
    case SOCKET_OPT_IP_RECVIF:
        result = nsetopt_lvl_ip_recvif(env, descP, eVal);
        break;
#endif

#if defined(IP_RECVOPTS)
    case SOCKET_OPT_IP_RECVOPTS:
        result = nsetopt_lvl_ip_recvopts(env, descP, eVal);
        break;
#endif

#if defined(IP_RECVORIGDSTADDR)
    case SOCKET_OPT_IP_RECVORIGDSTADDR:
        result = nsetopt_lvl_ip_recvorigdstaddr(env, descP, eVal);
        break;
#endif

#if defined(IP_RECVTOS)
    case SOCKET_OPT_IP_RECVTOS:
        result = nsetopt_lvl_ip_recvtos(env, descP, eVal);
        break;
#endif

#if defined(IP_RECVTTL)
    case SOCKET_OPT_IP_RECVTTL:
        result = nsetopt_lvl_ip_recvttl(env, descP, eVal);
        break;
#endif

#if defined(IP_RETOPTS)
    case SOCKET_OPT_IP_RETOPTS:
        result = nsetopt_lvl_ip_retopts(env, descP, eVal);
        break;
#endif

#if defined(IP_ROUTER_ALERT)
    case SOCKET_OPT_IP_ROUTER_ALERT:
        result = nsetopt_lvl_ip_router_alert(env, descP, eVal);
        break;
#endif

#if defined(IP_SENDSRCADDR)
    case SOCKET_OPT_IP_SENDSRCADDR:
        result = nsetopt_lvl_ip_sendsrcaddr(env, descP, eVal);
        break;
#endif

#if defined(IP_TOS)
    case SOCKET_OPT_IP_TOS:
        result = nsetopt_lvl_ip_tos(env, descP, eVal);
        break;
#endif

#if defined(IP_TRANSPARENT)
    case SOCKET_OPT_IP_TRANSPARENT:
        result = nsetopt_lvl_ip_transparent(env, descP, eVal);
        break;
#endif

#if defined(IP_TTL)
    case SOCKET_OPT_IP_TTL:
        result = nsetopt_lvl_ip_ttl(env, descP, eVal);
        break;
#endif

#if defined(IP_UNBLOCK_SOURCE)
    case SOCKET_OPT_IP_UNBLOCK_SOURCE:
        result = nsetopt_lvl_ip_unblock_source(env, descP, eVal);
        break;
#endif

    default:
        SSDBG( descP, ("SOCKET", "nsetopt_lvl_ip -> unknown opt (%d)\r\n", eOpt) );
        result = esock_make_error(env, esock_atom_einval);
        break;
    }

    SSDBG( descP,
           ("SOCKET", "nsetopt_lvl_ip -> done when"
            "\r\n   result: %T"
            "\r\n", result) );

    return result;
}


/* nsetopt_lvl_ip_add_membership - Level IP ADD_MEMBERSHIP option
 *
 * The value is a map with two attributes: multiaddr and interface.
 * The attribute 'multiaddr' is always a 4-tuple (IPv4 address).
 * The attribute 'interface' is either the atom 'any' or a 4-tuple
 * (IPv4 address).
 */
#if defined(IP_ADD_MEMBERSHIP)
static
ERL_NIF_TERM nsetopt_lvl_ip_add_membership(ErlNifEnv*        env,
                                           SocketDescriptor* descP,
                                           ERL_NIF_TERM      eVal)
{
    return nsetopt_lvl_ip_update_membership(env, descP, eVal, IP_ADD_MEMBERSHIP);
}
#endif


/* nsetopt_lvl_ip_add_source_membership - Level IP ADD_SOURCE_MEMBERSHIP option
 *
 * The value is a map with three attributes: multiaddr, interface and
 * sourceaddr.
 * The attribute 'multiaddr' is always a 4-tuple (IPv4 address).
 * The attribute 'interface' is always a 4-tuple (IPv4 address).
 * The attribute 'sourceaddr' is always a 4-tuple (IPv4 address).
 * (IPv4 address).
 */
#if defined(IP_ADD_SOURCE_MEMBERSHIP)
static
ERL_NIF_TERM nsetopt_lvl_ip_add_source_membership(ErlNifEnv*        env,
                                                  SocketDescriptor* descP,
                                                  ERL_NIF_TERM      eVal)
{
    return nsetopt_lvl_ip_update_source(env, descP, eVal,
                                        IP_ADD_SOURCE_MEMBERSHIP);
}
#endif


/* nsetopt_lvl_ip_block_source - Level IP BLOCK_SOURCE option
 *
 * The value is a map with three attributes: multiaddr, interface and
 * sourceaddr.
 * The attribute 'multiaddr' is always a 4-tuple (IPv4 address).
 * The attribute 'interface' is always a 4-tuple (IPv4 address).
 * The attribute 'sourceaddr' is always a 4-tuple (IPv4 address).
 * (IPv4 address).
 */
#if defined(IP_BLOCK_SOURCE)
static
ERL_NIF_TERM nsetopt_lvl_ip_block_source(ErlNifEnv*        env,
                                         SocketDescriptor* descP,
                                         ERL_NIF_TERM      eVal)
{
    return nsetopt_lvl_ip_update_source(env, descP, eVal, IP_BLOCK_SOURCE);
}
#endif


/* nsetopt_lvl_ip_drop_membership - Level IP DROP_MEMBERSHIP option
 *
 * The value is a map with two attributes: multiaddr and interface.
 * The attribute 'multiaddr' is always a 4-tuple (IPv4 address).
 * The attribute 'interface' is either the atom 'any' or a 4-tuple
 * (IPv4 address).
 *
 * We should really have a common function with add_membership,
 * since the code is virtually identical (except for the option
 * value).
 */
#if defined(IP_DROP_MEMBERSHIP)
static
ERL_NIF_TERM nsetopt_lvl_ip_drop_membership(ErlNifEnv*        env,
                                            SocketDescriptor* descP,
                                            ERL_NIF_TERM      eVal)
{
    return nsetopt_lvl_ip_update_membership(env, descP, eVal,
                                            IP_DROP_MEMBERSHIP);
}
#endif



/* nsetopt_lvl_ip_drop_source_membership - Level IP DROP_SOURCE_MEMBERSHIP option
 *
 * The value is a map with three attributes: multiaddr, interface and
 * sourceaddr.
 * The attribute 'multiaddr' is always a 4-tuple (IPv4 address).
 * The attribute 'interface' is always a 4-tuple (IPv4 address).
 * The attribute 'sourceaddr' is always a 4-tuple (IPv4 address).
 * (IPv4 address).
 */
#if defined(IP_DROP_SOURCE_MEMBERSHIP)
static
ERL_NIF_TERM nsetopt_lvl_ip_drop_source_membership(ErlNifEnv*        env,
                                                  SocketDescriptor* descP,
                                                  ERL_NIF_TERM      eVal)
{
    return nsetopt_lvl_ip_update_source(env, descP, eVal,
                                        IP_DROP_SOURCE_MEMBERSHIP);
}
#endif



/* nsetopt_lvl_ip_freebind - Level IP FREEBIND option
 */
#if defined(IP_FREEBIND)
static
ERL_NIF_TERM nsetopt_lvl_ip_freebind(ErlNifEnv*        env,
                                     SocketDescriptor* descP,
                                     ERL_NIF_TERM      eVal)
{
#if defined(SOL_IP)
    int level = SOL_IP;
#else
    int level = IPPROTO_IP;
#endif

    return nsetopt_bool_opt(env, descP, level, IP_FREEBIND, eVal);
}
#endif



/* nsetopt_lvl_ip_hdrincl - Level IP HDRINCL option
 */
#if defined(IP_HDRINCL)
static
ERL_NIF_TERM nsetopt_lvl_ip_hdrincl(ErlNifEnv*        env,
                                    SocketDescriptor* descP,
                                    ERL_NIF_TERM      eVal)
{
#if defined(SOL_IP)
    int level = SOL_IP;
#else
    int level = IPPROTO_IP;
#endif

    return nsetopt_bool_opt(env, descP, level, IP_HDRINCL, eVal);
}
#endif



/* nsetopt_lvl_ip_minttl - Level IP MINTTL option
 */
#if defined(IP_MINTTL)
static
ERL_NIF_TERM nsetopt_lvl_ip_minttl(ErlNifEnv*        env,
                                   SocketDescriptor* descP,
                                   ERL_NIF_TERM      eVal)
{
#if defined(SOL_IP)
    int level = SOL_IP;
#else
    int level = IPPROTO_IP;
#endif

    return nsetopt_int_opt(env, descP, level, IP_MINTTL, eVal);
}
#endif



/* nsetopt_lvl_ip_msfilter - Level IP MSFILTER option
 *
 * The value can be *either* the atom 'null' or a map of type ip_msfilter().
 */
#if defined(IP_MSFILTER) && defined(IP_MSFILTER_SIZE)
static
ERL_NIF_TERM nsetopt_lvl_ip_msfilter(ErlNifEnv*        env,
                                     SocketDescriptor* descP,
                                     ERL_NIF_TERM      eVal)
{
    ERL_NIF_TERM result;

    if (COMPARE(eVal, atom_null) == 0) {
        return nsetopt_lvl_ip_msfilter_set(env, descP->sock, NULL, 0);
    } else {
        struct ip_msfilter* msfP;
        uint32_t            msfSz;
        ERL_NIF_TERM        eMultiAddr, eInterface, eFMode, eSList, elem, tail;
        size_t              sz;
        unsigned int        slistLen, idx;

        if (!IS_MAP(env, eVal))
            return esock_make_error(env, esock_atom_einval);
        
        // It must have atleast four attributes
        if (!enif_get_map_size(env, eVal, &sz) || (sz < 4))
            return esock_make_error(env, esock_atom_einval);

        if (!GET_MAP_VAL(env, eVal, atom_multiaddr, &eMultiAddr))
            return esock_make_error(env, esock_atom_einval);
        
        if (!GET_MAP_VAL(env, eVal, atom_interface, &eInterface))
            return esock_make_error(env, esock_atom_einval);
        
        if (!GET_MAP_VAL(env, eVal, atom_mode, &eFMode))
            return esock_make_error(env, esock_atom_einval);
        
        if (!GET_MAP_VAL(env, eVal, atom_slist, &eSList))
            return esock_make_error(env, esock_atom_einval);

        /* We start (decoding) with the slist, since without it we don't
         * really know how much (memory) to allocate.
         */
        if (!GET_LIST_LEN(env, eSList, &slistLen))
            return esock_make_error(env, esock_atom_einval);

        msfSz = IP_MSFILTER_SIZE(slistLen);
        msfP  = MALLOC(msfSz);

        if (!esock_decode_ip4_address(env, eMultiAddr, &msfP->imsf_multiaddr)) {
            FREE(msfP);
            return esock_make_error(env, esock_atom_einval);
        }
        
        if (!esock_decode_ip4_address(env, eInterface, &msfP->imsf_interface)) {
            FREE(msfP);
            return esock_make_error(env, esock_atom_einval);
        }
        
        if (!decode_ip_msfilter_mode(env, eFMode, &msfP->imsf_fmode)) {
            FREE(msfP);
            return esock_make_error(env, esock_atom_einval);
        }

        /* And finally, extract the source addresses */
        msfP->imsf_numsrc = slistLen;
        for (idx = 0; idx < slistLen; idx++) {
            if (GET_LIST_ELEM(env, eSList, &elem, &tail)) {
                if (!esock_decode_ip4_address(env, elem, &msfP->imsf_slist[idx])) {
                    FREE(msfP);
                    return esock_make_error(env, esock_atom_einval);
                } else {
                    eSList = tail;
                }
            }
        }

        /* And now, finally, set the option */
        result = nsetopt_lvl_ip_msfilter_set(env, descP->sock, msfP, msfSz);
        FREE(msfP);
        return result;
    }

}


static
BOOLEAN_T decode_ip_msfilter_mode(ErlNifEnv*   env,
                                  ERL_NIF_TERM eVal,
                                  uint32_t*    mode)
{
    BOOLEAN_T result;

    if (COMPARE(eVal, atom_include) == 0) {
        *mode  = MCAST_INCLUDE;
        result = TRUE;        
    } else if (COMPARE(eVal, atom_exclude) == 0) {
        *mode  = MCAST_EXCLUDE;
        result = TRUE;        
    } else {
        result = FALSE;
    }

    return result;
}


static
ERL_NIF_TERM nsetopt_lvl_ip_msfilter_set(ErlNifEnv*          env,
                                         SOCKET              sock,
                                         struct ip_msfilter* msfP,
                                         SOCKLEN_T           optLen)
{
    ERL_NIF_TERM result;
    int          res;
#if defined(SOL_IP)
    int          level = SOL_IP;
#else
    int          level = IPPROTO_IP;
#endif

    res = socket_setopt(sock, level, IP_MSFILTER, (void*) msfP, optLen);
    if (res != 0)
        result = esock_make_error_errno(env, sock_errno());
    else
        result = esock_atom_ok;

    return result;
}
#endif // IP_MSFILTER



/* nsetopt_lvl_ip_mtu_discover - Level IP MTU_DISCOVER option
 *
 * The value is an atom of the type ip_pmtudisc().
 */
#if defined(IP_MTU_DISCOVER)
static
ERL_NIF_TERM nsetopt_lvl_ip_mtu_discover(ErlNifEnv*        env,
                                         SocketDescriptor* descP,
                                         ERL_NIF_TERM      eVal)
{
    ERL_NIF_TERM   result;
    int            val;
    char*          xres;
    int            res;
#if defined(SOL_IP)
    int            level = SOL_IP;
#else
    int            level = IPPROTO_IP;
#endif

    if ((xres = decode_ip_pmtudisc(env, eVal, &val)) != NULL) {

        result = esock_make_error_str(env, xres);

    } else {

        res = socket_setopt(descP->sock, level, IP_MTU_DISCOVER,
                            &val, sizeof(val));

        if (res != 0)
            result = esock_make_error_errno(env, sock_errno());
        else
            result = esock_atom_ok;

    }

    return result;
}
#endif


/* nsetopt_lvl_ip_multicast_all - Level IP MULTICAST_ALL option
 */
#if defined(IP_MULTICAST_ALL)
static
ERL_NIF_TERM nsetopt_lvl_ip_multicast_all(ErlNifEnv*        env,
                                          SocketDescriptor* descP,
                                          ERL_NIF_TERM      eVal)
{
#if defined(SOL_IP)
    int level = SOL_IP;
#else
    int level = IPPROTO_IP;
#endif

    return nsetopt_bool_opt(env, descP, level, IP_MULTICAST_ALL, eVal);
}
#endif


/* nsetopt_lvl_ip_multicast_if - Level IP MULTICAST_IF option
 *
 * The value is either the atom 'any' or a 4-tuple.
 */
#if defined(IP_MULTICAST_IF)
static
ERL_NIF_TERM nsetopt_lvl_ip_multicast_if(ErlNifEnv*        env,
                                         SocketDescriptor* descP,
                                         ERL_NIF_TERM      eVal)
{
    ERL_NIF_TERM   result;
    struct in_addr ifAddr;
    char*          xres;
    int            res;
#if defined(SOL_IP)
    int            level = SOL_IP;
#else
    int            level = IPPROTO_IP;
#endif

    if ((xres = esock_decode_ip4_address(env, eVal, &ifAddr)) != NULL) {
        result = esock_make_error_str(env, xres);
    } else {
        
        res = socket_setopt(descP->sock, level, IP_MULTICAST_LOOP,
                            &ifAddr, sizeof(ifAddr));

        if (res != 0)
            result = esock_make_error_errno(env, sock_errno());
        else
            result = esock_atom_ok;

    }

    return result;
}
#endif


/* nsetopt_lvl_ip_multicast_loop - Level IP MULTICAST_LOOP option
 */
#if defined(IP_MULTICAST_LOOP)
static
ERL_NIF_TERM nsetopt_lvl_ip_multicast_loop(ErlNifEnv*        env,
                                           SocketDescriptor* descP,
                                           ERL_NIF_TERM      eVal)
{
#if defined(SOL_IP)
    int level = SOL_IP;
#else
    int level = IPPROTO_IP;
#endif

    return nsetopt_bool_opt(env, descP, level, IP_MULTICAST_LOOP, eVal);
}
#endif


/* nsetopt_lvl_ip_multicast_ttl - Level IP MULTICAST_TTL option
 */
#if defined(IP_MULTICAST_TTL)
static
ERL_NIF_TERM nsetopt_lvl_ip_multicast_ttl(ErlNifEnv*        env,
                                          SocketDescriptor* descP,
                                          ERL_NIF_TERM      eVal)
{
#if defined(SOL_IP)
    int level = SOL_IP;
#else
    int level = IPPROTO_IP;
#endif

    return nsetopt_int_opt(env, descP, level, IP_MULTICAST_TTL, eVal);
}
#endif


/* nsetopt_lvl_ip_nodefrag - Level IP NODEFRAG option
 */
#if defined(IP_NODEFRAG)
static
ERL_NIF_TERM nsetopt_lvl_ip_nodefrag(ErlNifEnv*        env,
                                     SocketDescriptor* descP,
                                     ERL_NIF_TERM      eVal)
{
#if defined(SOL_IP)
    int level = SOL_IP;
#else
    int level = IPPROTO_IP;
#endif

    return nsetopt_bool_opt(env, descP, level, IP_NODEFRAG, eVal);
}
#endif


/* nsetopt_lvl_ip_pktinfo - Level IP PKTINFO option
 */
#if defined(IP_PKTINFO)
static
ERL_NIF_TERM nsetopt_lvl_ip_pktinfo(ErlNifEnv*        env,
                                    SocketDescriptor* descP,
                                    ERL_NIF_TERM      eVal)
{
#if defined(SOL_IP)
    int level = SOL_IP;
#else
    int level = IPPROTO_IP;
#endif

    return nsetopt_bool_opt(env, descP, level, IP_PKTINFO, eVal);
}
#endif


/* nsetopt_lvl_ip_recvdstaddr - Level IP RECVDSTADDR option
 */
#if defined(IP_RECVDSTADDR)
static
ERL_NIF_TERM nsetopt_lvl_ip_recvdstaddr(ErlNifEnv*        env,
                                        SocketDescriptor* descP,
                                        ERL_NIF_TERM      eVal)
{
#if defined(SOL_IP)
    int level = SOL_IP;
#else
    int level = IPPROTO_IP;
#endif

    return nsetopt_bool_opt(env, descP, level, IP_RECVDSTADDR, eVal);
}
#endif


/* nsetopt_lvl_ip_recverr - Level IP RECVERR option
 */
#if defined(IP_RECVERR)
static
ERL_NIF_TERM nsetopt_lvl_ip_recverr(ErlNifEnv*        env,
                                    SocketDescriptor* descP,
                                    ERL_NIF_TERM      eVal)
{
#if defined(SOL_IP)
    int level = SOL_IP;
#else
    int level = IPPROTO_IP;
#endif

    return nsetopt_bool_opt(env, descP, level, IP_RECVERR, eVal);
}
#endif


/* nsetopt_lvl_ip_recvif - Level IP RECVIF option
 */
#if defined(IP_RECVIF)
static
ERL_NIF_TERM nsetopt_lvl_ip_recvif(ErlNifEnv*        env,
                                   SocketDescriptor* descP,
                                   ERL_NIF_TERM      eVal)
{
#if defined(SOL_IP)
    int level = SOL_IP;
#else
    int level = IPPROTO_IP;
#endif

    return nsetopt_bool_opt(env, descP, level, IP_RECVIF, eVal);
}
#endif


/* nsetopt_lvl_ip_recvopts - Level IP RECVOPTS option
 */
#if defined(IP_RECVOPTS)
static
ERL_NIF_TERM nsetopt_lvl_ip_recvopts(ErlNifEnv*        env,
                                     SocketDescriptor* descP,
                                     ERL_NIF_TERM      eVal)
{
#if defined(SOL_IP)
    int level = SOL_IP;
#else
    int level = IPPROTO_IP;
#endif

    return nsetopt_bool_opt(env, descP, level, IP_RECVOPTS, eVal);
}
#endif


/* nsetopt_lvl_ip_recvorigdstaddr - Level IP RECVORIGDSTADDR option
 */
#if defined(IP_RECVORIGDSTADDR)
static
ERL_NIF_TERM nsetopt_lvl_ip_recvorigdstaddr(ErlNifEnv*        env,
                                            SocketDescriptor* descP,
                                            ERL_NIF_TERM      eVal)
{
#if defined(SOL_IP)
    int level = SOL_IP;
#else
    int level = IPPROTO_IP;
#endif

    return nsetopt_bool_opt(env, descP, level, IP_RECVORIGDSTADDR, eVal);
}
#endif


/* nsetopt_lvl_ip_recvtos - Level IP RECVTOS option
 */
#if defined(IP_RECVTOS)
static
ERL_NIF_TERM nsetopt_lvl_ip_recvtos(ErlNifEnv*        env,
                                    SocketDescriptor* descP,
                                    ERL_NIF_TERM      eVal)
{
#if defined(SOL_IP)
    int level = SOL_IP;
#else
    int level = IPPROTO_IP;
#endif

    return nsetopt_bool_opt(env, descP, level, IP_RECVTOS, eVal);
}
#endif


/* nsetopt_lvl_ip_recvttl - Level IP RECVTTL option
 */
#if defined(IP_RECVTTL)
static
ERL_NIF_TERM nsetopt_lvl_ip_recvttl(ErlNifEnv*        env,
                                    SocketDescriptor* descP,
                                    ERL_NIF_TERM      eVal)
{
#if defined(SOL_IP)
    int level = SOL_IP;
#else
    int level = IPPROTO_IP;
#endif

    return nsetopt_bool_opt(env, descP, level, IP_RECVTTL, eVal);
}
#endif


/* nsetopt_lvl_ip_retopts - Level IP RETOPTS option
 */
#if defined(IP_RETOPTS)
static
ERL_NIF_TERM nsetopt_lvl_ip_retopts(ErlNifEnv*        env,
                                    SocketDescriptor* descP,
                                    ERL_NIF_TERM      eVal)
{
#if defined(SOL_IP)
    int level = SOL_IP;
#else
    int level = IPPROTO_IP;
#endif

    return nsetopt_bool_opt(env, descP, level, IP_RETOPTS, eVal);
}
#endif


/* nsetopt_lvl_ip_router_alert - Level IP ROUTER_ALERT option
 */
#if defined(IP_ROUTER_ALERT)
static
ERL_NIF_TERM nsetopt_lvl_ip_router_alert(ErlNifEnv*        env,
                                         SocketDescriptor* descP,
                                         ERL_NIF_TERM      eVal)
{
#if defined(SOL_IP)
    int level = SOL_IP;
#else
    int level = IPPROTO_IP;
#endif

    return nsetopt_int_opt(env, descP, level, IP_ROUTER_ALERT, eVal);
}
#endif


/* nsetopt_lvl_ip_sendsrcaddr - Level IP SENDSRCADDR option
 */
#if defined(IP_SENDSRCADDR)
static
ERL_NIF_TERM nsetopt_lvl_ip_sendsrcaddr(ErlNifEnv*        env,
                                        SocketDescriptor* descP,
                                        ERL_NIF_TERM      eVal)
{
#if defined(SOL_IP)
    int level = SOL_IP;
#else
    int level = IPPROTO_IP;
#endif

    return nsetopt_bool_opt(env, descP, level, IP_SENDSRCADDR, eVal);
}
#endif


/* nsetopt_lvl_ip_tos - Level IP TOS option
 */
#if defined(IP_TOS)
static
ERL_NIF_TERM nsetopt_lvl_ip_tos(ErlNifEnv*        env,
                                SocketDescriptor* descP,
                                ERL_NIF_TERM      eVal)
{
#if defined(SOL_IP)
    int          level = SOL_IP;
#else
    int          level = IPPROTO_IP;
#endif
    ERL_NIF_TERM result;
    int          val;

    if (decode_ip_tos(env, eVal, &val)) {
        int res = socket_setopt(descP->sock, level, IP_TOS, &val, sizeof(val));

        if (res != 0)
            result = esock_make_error_errno(env, sock_errno());
        else
            result = esock_atom_ok;

    } else {
        result = esock_make_error(env, esock_atom_einval);
    }

    return result;
}
#endif


/* nsetopt_lvl_ip_transparent - Level IP TRANSPARENT option
 */
#if defined(IP_TRANSPARENT)
static
ERL_NIF_TERM nsetopt_lvl_ip_transparent(ErlNifEnv*        env,
                                        SocketDescriptor* descP,
                                        ERL_NIF_TERM      eVal)
{
#if defined(SOL_IP)
    int level = SOL_IP;
#else
    int level = IPPROTO_IP;
#endif

    return nsetopt_bool_opt(env, descP, level, IP_TRANSPARENT, eVal);
}
#endif



/* nsetopt_lvl_ip_ttl - Level IP TTL option
 */
#if defined(IP_TTL)
static
ERL_NIF_TERM nsetopt_lvl_ip_ttl(ErlNifEnv*        env,
                                SocketDescriptor* descP,
                                ERL_NIF_TERM      eVal)
{
#if defined(SOL_IP)
    int level = SOL_IP;
#else
    int level = IPPROTO_IP;
#endif

    return nsetopt_int_opt(env, descP, level, IP_TTL, eVal);
}
#endif



/* nsetopt_lvl_ip_unblock_source - Level IP UNBLOCK_SOURCE option
 *
 * The value is a map with three attributes: multiaddr, interface and
 * sourceaddr.
 * The attribute 'multiaddr' is always a 4-tuple (IPv4 address).
 * The attribute 'interface' is always a 4-tuple (IPv4 address).
 * The attribute 'sourceaddr' is always a 4-tuple (IPv4 address).
 * (IPv4 address).
 */
#if defined(IP_UNBLOCK_SOURCE)
static
ERL_NIF_TERM nsetopt_lvl_ip_unblock_source(ErlNifEnv*        env,
                                           SocketDescriptor* descP,
                                           ERL_NIF_TERM      eVal)
{
    return nsetopt_lvl_ip_update_source(env, descP, eVal, IP_UNBLOCK_SOURCE);
}
#endif



#if defined(IP_ADD_MEMBERSHIP) || defined(IP_DROP_MEMBERSHIP)
static
ERL_NIF_TERM nsetopt_lvl_ip_update_membership(ErlNifEnv*        env,
                                              SocketDescriptor* descP,
                                              ERL_NIF_TERM      eVal,
                                              int               opt)
{
    ERL_NIF_TERM   result, eMultiAddr, eInterface;
    struct ip_mreq mreq;
    char*          xres;
    int            res;
    size_t         sz;
#if defined(SOL_IP)
    int            level = SOL_IP;
#else
    int            level = IPPROTO_IP;
#endif

    // It must be a map
    if (!IS_MAP(env, eVal))
        return enif_make_badarg(env);

    // It must have atleast two attributes
    if (!enif_get_map_size(env, eVal, &sz) || (sz >= 2))
        return enif_make_badarg(env);

    if (!GET_MAP_VAL(env, eVal, atom_multiaddr, &eMultiAddr))
        return enif_make_badarg(env);

    if (!GET_MAP_VAL(env, eVal, atom_interface, &eInterface))
        return enif_make_badarg(env);

    if ((xres = esock_decode_ip4_address(env,
                                         eMultiAddr,
                                         &mreq.imr_multiaddr)) != NULL)
        return esock_make_error_str(env, xres);

    if ((xres = esock_decode_ip4_address(env,
                                         eInterface,
                                         &mreq.imr_interface)) != NULL)
        return esock_make_error_str(env, xres);

    res = socket_setopt(descP->sock, level, opt, &mreq, sizeof(mreq));

    if (res != 0)
        result = esock_make_error_errno(env, sock_errno());
    else
        result = esock_atom_ok;

    return result;
}
#endif


#if defined(IP_ADD_SOURCE_MEMBERSHIP) || defined(IP_DROP_SOURCE_MEMBERSHIP) || defined(IP_BLOCK_SOURCE) || defined(IP_UNBLOCK_SOURCE)
static
ERL_NIF_TERM nsetopt_lvl_ip_update_source(ErlNifEnv*        env,
                                          SocketDescriptor* descP,
                                          ERL_NIF_TERM      eVal,
                                          int               opt)
{
    ERL_NIF_TERM          result, eMultiAddr, eInterface, eSourceAddr;
    struct ip_mreq_source mreq;
    char*                 xres;
    int                   res;
    size_t                sz;
#if defined(SOL_IP)
    int            level = SOL_IP;
#else
    int            level = IPPROTO_IP;
#endif

    // It must be a map
    if (!IS_MAP(env, eVal))
        return enif_make_badarg(env);

    // It must have atleast three attributes
    if (!enif_get_map_size(env, eVal, &sz) || (sz >= 3))
        return enif_make_badarg(env);

    if (!GET_MAP_VAL(env, eVal, atom_multiaddr, &eMultiAddr))
        return enif_make_badarg(env);

    if (!GET_MAP_VAL(env, eVal, atom_interface, &eInterface))
        return enif_make_badarg(env);

    if (!GET_MAP_VAL(env, eVal, atom_sourceaddr, &eSourceAddr))
        return enif_make_badarg(env);

    if ((xres = esock_decode_ip4_address(env,
                                         eMultiAddr,
                                         &mreq.imr_multiaddr)) != NULL)
        return esock_make_error_str(env, xres);

    if ((xres = esock_decode_ip4_address(env,
                                         eInterface,
                                         &mreq.imr_interface)) != NULL)
        return esock_make_error_str(env, xres);

    if ((xres = esock_decode_ip4_address(env,
                                         eSourceAddr,
                                         &mreq.imr_sourceaddr)) != NULL)
        return esock_make_error_str(env, xres);

    res = socket_setopt(descP->sock, level, opt, &mreq, sizeof(mreq));

    if (res != 0)
        result = esock_make_error_errno(env, sock_errno());
    else
        result = esock_atom_ok;

    return result;
}
#endif



/* *** Handling set of socket options for level = ipv6 *** */

/* nsetopt_lvl_ipv6 - Level *IPv6* option(s)
 */
#if defined(SOL_IPV6)
static
ERL_NIF_TERM nsetopt_lvl_ipv6(ErlNifEnv*        env,
                              SocketDescriptor* descP,
                              int               eOpt,
                              ERL_NIF_TERM      eVal)
{
    ERL_NIF_TERM result;

    SSDBG( descP,
           ("SOCKET", "nsetopt_lvl_ipv6 -> entry with"
            "\r\n   opt: %d"
            "\r\n", eOpt) );

    switch (eOpt) {
#if defined(IPV6_ADDRFORM)
    case SOCKET_OPT_IPV6_ADDRFORM:
        result = nsetopt_lvl_ipv6_addrform(env, descP, eVal);
        break;
#endif

#if defined(IPV6_ADD_MEMBERSHIP)
    case SOCKET_OPT_IPV6_ADD_MEMBERSHIP:
        result = nsetopt_lvl_ipv6_add_membership(env, descP, eVal);
        break;
#endif

#if defined(IPV6_AUTHHDR)
    case SOCKET_OPT_IPV6_AUTHHDR:
        result = nsetopt_lvl_ipv6_authhdr(env, descP, eVal);
        break;
#endif

#if defined(IPV6_DROP_MEMBERSHIP)
    case SOCKET_OPT_IPV6_DROP_MEMBERSHIP:
        result = nsetopt_lvl_ipv6_drop_membership(env, descP, eVal);
        break;
#endif

#if defined(IPV6_DSTOPTS)
    case SOCKET_OPT_IPV6_DSTOPTS:
        result = nsetopt_lvl_ipv6_dstopts(env, descP, eVal);
        break;
#endif

#if defined(IPV6_FLOWINFO)
    case SOCKET_OPT_IPV6_FLOWINFO:
        result = nsetopt_lvl_ipv6_flowinfo(env, descP, eVal);
        break;
#endif

#if defined(IPV6_HOPLIMIT)
    case SOCKET_OPT_IPV6_HOPLIMIT:
        result = nsetopt_lvl_ipv6_hoplimit(env, descP, eVal);
        break;
#endif

#if defined(IPV6_HOPOPTS)
    case SOCKET_OPT_IPV6_HOPOPTS:
        result = nsetopt_lvl_ipv6_hopopts(env, descP, eVal);
        break;
#endif

#if defined(IPV6_MTU)
    case SOCKET_OPT_IPV6_MTU:
        result = nsetopt_lvl_ipv6_mtu(env, descP, eVal);
        break;
#endif

#if defined(IPV6_MTU_DISCOVER)
    case SOCKET_OPT_IPV6_MTU_DISCOVER:
        result = nsetopt_lvl_ipv6_mtu_discover(env, descP, eVal);
        break;
#endif

#if defined(IPV6_MULTICAST_HOPS)
    case SOCKET_OPT_IPV6_MULTICAST_HOPS:
        result = nsetopt_lvl_ipv6_multicast_hops(env, descP, eVal);
        break;
#endif

#if defined(IPV6_MULTICAST_IF)
    case SOCKET_OPT_IPV6_MULTICAST_IF:
        result = nsetopt_lvl_ipv6_multicast_if(env, descP, eVal);
        break;
#endif

#if defined(IPV6_MULTICAST_LOOP)
    case SOCKET_OPT_IPV6_MULTICAST_LOOP:
        result = nsetopt_lvl_ipv6_multicast_loop(env, descP, eVal);
        break;
#endif

#if defined(IPV6_RECVERR)
    case SOCKET_OPT_IPV6_RECVERR:
        result = nsetopt_lvl_ipv6_recverr(env, descP, eVal);
        break;
#endif

#if defined(IPV6_RECVPKTINFO) || defined(IPV6_PKTINFO)
    case SOCKET_OPT_IPV6_RECVPKTINFO:
        result = nsetopt_lvl_ipv6_recvpktinfo(env, descP, eVal);
        break;
#endif

#if defined(IPV6_ROUTER_ALERT)
    case SOCKET_OPT_IPV6_ROUTER_ALERT:
        result = nsetopt_lvl_ipv6_router_alert(env, descP, eVal);
        break;
#endif

#if defined(IPV6_RTHDR)
    case SOCKET_OPT_IPV6_RTHDR:
        result = nsetopt_lvl_ipv6_rthdr(env, descP, eVal);
        break;
#endif

#if defined(IPV6_UNICAST_HOPS)
    case SOCKET_OPT_IPV6_UNICAST_HOPS:
        result = nsetopt_lvl_ipv6_unicast_hops(env, descP, eVal);
        break;
#endif

#if defined(IPV6_V6ONLY)
    case SOCKET_OPT_IPV6_V6ONLY:
        result = nsetopt_lvl_ipv6_v6only(env, descP, eVal);
        break;
#endif

    default:
        SSDBG( descP,
               ("SOCKET", "nsetopt_lvl_ipv6 -> unknown opt (%d)\r\n", eOpt) );
        result = esock_make_error(env, esock_atom_einval);
        break;
    }

    SSDBG( descP,
           ("SOCKET", "nsetopt_lvl_ipv6 -> done when"
            "\r\n   result: %T"
            "\r\n", result) );

    return result;
}


#if defined(IPV6_ADDRFORM)
static
ERL_NIF_TERM nsetopt_lvl_ipv6_addrform(ErlNifEnv*        env,
                                       SocketDescriptor* descP,
                                       ERL_NIF_TERM      eVal)
{
    ERL_NIF_TERM result;
    int          res, edomain, domain;

    SSDBG( descP,
           ("SOCKET", "nsetopt_lvl_ipv6_addrform -> entry with"
            "\r\n   eVal: %T"
            "\r\n", eVal) );

    if (!GET_INT(env, eVal, &edomain))
        return esock_make_error(env, esock_atom_einval);

    SSDBG( descP,
           ("SOCKET", "nsetopt_lvl_ipv6_addrform -> decode"
            "\r\n   edomain: %d"
            "\r\n", edomain) );

    if (!edomain2domain(edomain, &domain))
        return esock_make_error(env, esock_atom_einval);

    SSDBG( descP, ("SOCKET", "nsetopt_lvl_ipv6_addrform -> try set opt to %d\r\n",
                   domain) );
    
    res = socket_setopt(descP->sock,
                        SOL_IPV6, IPV6_ADDRFORM,
                        &domain, sizeof(domain));

    if (res != 0)
        result = esock_make_error_errno(env, sock_errno());
    else
        result = esock_atom_ok;

    return result;
}
#endif


#if defined(IPV6_ADD_MEMBERSHIP)
static
ERL_NIF_TERM nsetopt_lvl_ipv6_add_membership(ErlNifEnv*        env,
                                             SocketDescriptor* descP,
                                             ERL_NIF_TERM      eVal)
{
    return nsetopt_lvl_ipv6_update_membership(env, descP, eVal,
                                              IPV6_ADD_MEMBERSHIP);
}
#endif


#if defined(IPV6_AUTHHDR)
static
ERL_NIF_TERM nsetopt_lvl_ipv6_authhdr(ErlNifEnv*        env,
                                      SocketDescriptor* descP,
                                      ERL_NIF_TERM      eVal)
{
    return nsetopt_bool_opt(env, descP, SOL_IPV6, IPV6_AUTHHDR, eVal);
}
#endif


#if defined(IPV6_DROP_MEMBERSHIP)
static
ERL_NIF_TERM nsetopt_lvl_ipv6_drop_membership(ErlNifEnv*        env,
                                              SocketDescriptor* descP,
                                              ERL_NIF_TERM      eVal)
{
    return nsetopt_lvl_ipv6_update_membership(env, descP, eVal,
                                              IPV6_DROP_MEMBERSHIP);
}
#endif


#if defined(IPV6_DSTOPTS)
static
ERL_NIF_TERM nsetopt_lvl_ipv6_dstopts(ErlNifEnv*        env,
                                      SocketDescriptor* descP,
                                      ERL_NIF_TERM      eVal)
{
    return nsetopt_bool_opt(env, descP, SOL_IPV6, IPV6_DSTOPTS, eVal);
}
#endif


#if defined(IPV6_FLOWINFO)
static
ERL_NIF_TERM nsetopt_lvl_ipv6_flowinfo(ErlNifEnv*        env,
                                       SocketDescriptor* descP,
                                       ERL_NIF_TERM      eVal)
{
    return nsetopt_bool_opt(env, descP, SOL_IPV6, IPV6_FLOWINFO, eVal);
}
#endif


#if defined(IPV6_HOPLIMIT)
static
ERL_NIF_TERM nsetopt_lvl_ipv6_hoplimit(ErlNifEnv*        env,
                                       SocketDescriptor* descP,
                                       ERL_NIF_TERM      eVal)
{
    return nsetopt_bool_opt(env, descP, SOL_IPV6, IPV6_HOPLIMIT, eVal);
}
#endif


#if defined(IPV6_HOPOPTS)
static
ERL_NIF_TERM nsetopt_lvl_ipv6_hopopts(ErlNifEnv*        env,
                                      SocketDescriptor* descP,
                                      ERL_NIF_TERM      eVal)
{
    return nsetopt_bool_opt(env, descP, SOL_IPV6, IPV6_HOPOPTS, eVal);
}
#endif


#if defined(IPV6_MTU)
static
ERL_NIF_TERM nsetopt_lvl_ipv6_mtu(ErlNifEnv*        env,
                                  SocketDescriptor* descP,
                                  ERL_NIF_TERM      eVal)
{
    return nsetopt_int_opt(env, descP, SOL_IPV6, IPV6_MTU, eVal);
}
#endif


/* nsetopt_lvl_ipv6_mtu_discover - Level IPv6 MTU_DISCOVER option
 *
 * The value is an atom of the type ipv6_pmtudisc().
 */
#if defined(IPV6_MTU_DISCOVER)
static
ERL_NIF_TERM nsetopt_lvl_ipv6_mtu_discover(ErlNifEnv*        env,
                                           SocketDescriptor* descP,
                                           ERL_NIF_TERM      eVal)
{
    ERL_NIF_TERM  result;
    int           val;
    char*         xres;
    int           res;

    if ((xres = decode_ipv6_pmtudisc(env, eVal, &val)) != NULL) {

        result = esock_make_error_str(env, xres);

    } else {

        res = socket_setopt(descP->sock, SOL_IPV6, IPV6_MTU_DISCOVER,
                            &val, sizeof(val));

        if (res != 0)
            result = esock_make_error_errno(env, sock_errno());
        else
            result = esock_atom_ok;

    }

    return result;
}
#endif


#if defined(IPV6_MULTICAST_HOPS)
static
ERL_NIF_TERM nsetopt_lvl_ipv6_multicast_hops(ErlNifEnv*        env,
                                             SocketDescriptor* descP,
                                             ERL_NIF_TERM      eVal)
{
    return nsetopt_int_opt(env, descP, SOL_IPV6, IPV6_MULTICAST_HOPS, eVal);
}
#endif



#if defined(IPV6_MULTICAST_IF)
static
ERL_NIF_TERM nsetopt_lvl_ipv6_multicast_if(ErlNifEnv*        env,
                                           SocketDescriptor* descP,
                                           ERL_NIF_TERM      eVal)
{
    return nsetopt_int_opt(env, descP, SOL_IPV6, IPV6_MULTICAST_IF, eVal);
}
#endif



#if defined(IPV6_MULTICAST_LOOP)
static
ERL_NIF_TERM nsetopt_lvl_ipv6_multicast_loop(ErlNifEnv*        env,
                                             SocketDescriptor* descP,
                                             ERL_NIF_TERM      eVal)
{
    return nsetopt_bool_opt(env, descP, SOL_IPV6, IPV6_MULTICAST_LOOP, eVal);
}
#endif


#if defined(IPV6_RECVERR)
static
ERL_NIF_TERM nsetopt_lvl_ipv6_recverr(ErlNifEnv*        env,
                                      SocketDescriptor* descP,
                                      ERL_NIF_TERM      eVal)
{
    return nsetopt_bool_opt(env, descP, SOL_IPV6, IPV6_RECVERR, eVal);
}
#endif


#if defined(IPV6_RECVPKTINFO) || defined(IPV6_PKTINFO)
static
ERL_NIF_TERM nsetopt_lvl_ipv6_recvpktinfo(ErlNifEnv*        env,
                                          SocketDescriptor* descP,
                                          ERL_NIF_TERM      eVal)
{
#if defined(IPV6_RECVPKTINFO)
    int opt = IPV6_RECVPKTINFO;
#else
    int opt = IPV6_PKTINFO;
#endif

    return nsetopt_bool_opt(env, descP, SOL_IPV6, opt, eVal);
}
#endif


#if defined(IPV6_ROUTER_ALERT)
static
ERL_NIF_TERM nsetopt_lvl_ipv6_router_alert(ErlNifEnv*        env,
                                           SocketDescriptor* descP,
                                           ERL_NIF_TERM      eVal)
{
    return nsetopt_int_opt(env, descP, SOL_IPV6, IPV6_ROUTER_ALERT, eVal);
}
#endif



#if defined(IPV6_RTHDR)
static
ERL_NIF_TERM nsetopt_lvl_ipv6_rthdr(ErlNifEnv*        env,
                                    SocketDescriptor* descP,
                                    ERL_NIF_TERM      eVal)
{
    return nsetopt_bool_opt(env, descP, SOL_IPV6, IPV6_RTHDR, eVal);
}
#endif


#if defined(IPV6_UNICAST_HOPS)
static
ERL_NIF_TERM nsetopt_lvl_ipv6_unicast_hops(ErlNifEnv*        env,
                                           SocketDescriptor* descP,
                                           ERL_NIF_TERM      eVal)
{
    return nsetopt_int_opt(env, descP, SOL_IPV6, IPV6_UNICAST_HOPS, eVal);
}
#endif



#if defined(IPV6_V6ONLY)
static
ERL_NIF_TERM nsetopt_lvl_ipv6_v6only(ErlNifEnv*        env,
                                     SocketDescriptor* descP,
                                     ERL_NIF_TERM      eVal)
{
    return nsetopt_bool_opt(env, descP, SOL_IPV6, IPV6_V6ONLY, eVal);
}
#endif


#if defined(IPV6_ADD_MEMBERSHIP) || defined(IPV6_DROP_MEMBERSHIP)
static
ERL_NIF_TERM nsetopt_lvl_ipv6_update_membership(ErlNifEnv*        env,
                                                SocketDescriptor* descP,
                                                ERL_NIF_TERM      eVal,
                                                int               opt)
{
    ERL_NIF_TERM     result, eMultiAddr, eInterface;
    struct ipv6_mreq mreq;
    char*            xres;
    int              res;
    size_t           sz;
    int              level = IPPROTO_IPV6;

    // It must be a map
    if (!IS_MAP(env, eVal))
        return enif_make_badarg(env);

    // It must have atleast two attributes
    if (!enif_get_map_size(env, eVal, &sz) || (sz >= 2))
        return enif_make_badarg(env);

    if (!GET_MAP_VAL(env, eVal, atom_multiaddr, &eMultiAddr))
        return enif_make_badarg(env);

    if (!GET_MAP_VAL(env, eVal, atom_interface, &eInterface))
        return enif_make_badarg(env);

    if ((xres = esock_decode_ip6_address(env,
                                         eMultiAddr,
                                         &mreq.ipv6mr_multiaddr)) != NULL)
        return esock_make_error_str(env, xres);

    if (!GET_UINT(env, eInterface, &mreq.ipv6mr_interface))
        return esock_make_error(env, esock_atom_einval);

    res = socket_setopt(descP->sock, level, opt, &mreq, sizeof(mreq));

    if (res != 0)
        result = esock_make_error_errno(env, sock_errno());
    else
        result = esock_atom_ok;

    return result;
}
#endif



#endif // defined(SOL_IPV6)



/* nsetopt_lvl_tcp - Level *TCP* option(s)
 */
static
ERL_NIF_TERM nsetopt_lvl_tcp(ErlNifEnv*        env,
                             SocketDescriptor* descP,
                             int               eOpt,
                             ERL_NIF_TERM      eVal)
{
    ERL_NIF_TERM result;

    SSDBG( descP,
           ("SOCKET", "nsetopt_lvl_tcp -> entry with"
            "\r\n   opt: %d"
            "\r\n", eOpt) );

    switch (eOpt) {
#if defined(TCP_CONGESTION)
    case SOCKET_OPT_TCP_CONGESTION:
        result = nsetopt_lvl_tcp_congestion(env, descP, eVal);
        break;
#endif

#if defined(TCP_MAXSEG)
    case SOCKET_OPT_TCP_MAXSEG:
        result = nsetopt_lvl_tcp_maxseg(env, descP, eVal);
        break;
#endif

#if defined(TCP_NODELAY)
    case SOCKET_OPT_TCP_NODELAY:
        result = nsetopt_lvl_tcp_nodelay(env, descP, eVal);
        break;
#endif

    default:
        result = esock_make_error(env, esock_atom_einval);
        break;
    }

    return result;
}


/* nsetopt_lvl_tcp_congestion - Level TCP CONGESTION option
 */
#if defined(TCP_CONGESTION)
static
ERL_NIF_TERM nsetopt_lvl_tcp_congestion(ErlNifEnv*        env,
                                        SocketDescriptor* descP,
                                        ERL_NIF_TERM      eVal)
{
    int max = SOCKET_OPT_TCP_CONGESTION_NAME_MAX+1;

    return nsetopt_str_opt(env, descP, IPPROTO_TCP, TCP_CONGESTION, max, eVal);
}
#endif


/* nsetopt_lvl_tcp_maxseg - Level TCP MAXSEG option
 */
#if defined(TCP_MAXSEG)
static
ERL_NIF_TERM nsetopt_lvl_tcp_maxseg(ErlNifEnv*        env,
                                    SocketDescriptor* descP,
                                    ERL_NIF_TERM      eVal)
{
    return nsetopt_int_opt(env, descP, IPPROTO_TCP, TCP_MAXSEG, eVal);
}
#endif


/* nsetopt_lvl_tcp_nodelay - Level TCP NODELAY option
 */
#if defined(TCP_NODELAY)
static
ERL_NIF_TERM nsetopt_lvl_tcp_nodelay(ErlNifEnv*        env,
                                     SocketDescriptor* descP,
                                     ERL_NIF_TERM      eVal)
{
    return nsetopt_bool_opt(env, descP, IPPROTO_TCP, TCP_NODELAY, eVal);
}
#endif



/* nsetopt_lvl_udp - Level *UDP* option(s)
 */
static
ERL_NIF_TERM nsetopt_lvl_udp(ErlNifEnv*        env,
                             SocketDescriptor* descP,
                             int               eOpt,
                             ERL_NIF_TERM      eVal)
{
    ERL_NIF_TERM result;

    SSDBG( descP,
           ("SOCKET", "nsetopt_lvl_udp -> entry with"
            "\r\n   opt: %d"
            "\r\n", eOpt) );

    switch (eOpt) {
#if defined(UDP_CORK)
    case SOCKET_OPT_UDP_CORK:
        result = nsetopt_lvl_udp_cork(env, descP, eVal);
        break;
#endif

    default:
        result = esock_make_error(env, esock_atom_einval);
        break;
    }

    return result;
}


/* nsetopt_lvl_udp_cork - Level UDP CORK option
 */
#if defined(UDP_CORK)
static
ERL_NIF_TERM nsetopt_lvl_udp_cork(ErlNifEnv*        env,
                                  SocketDescriptor* descP,
                                  ERL_NIF_TERM      eVal)
{
    return nsetopt_bool_opt(env, descP, IPPROTO_UDP, UDP_CORK, eVal);
}
#endif




/* nsetopt_lvl_sctp - Level *SCTP* option(s)
 */
#if defined(HAVE_SCTP)
static
ERL_NIF_TERM nsetopt_lvl_sctp(ErlNifEnv*        env,
                              SocketDescriptor* descP,
                              int               eOpt,
                              ERL_NIF_TERM      eVal)
{
    ERL_NIF_TERM result;

    SSDBG( descP,
           ("SOCKET", "nsetopt_lvl_sctp -> entry with"
            "\r\n   opt: %d"
            "\r\n", eOpt) );

    switch (eOpt) {
#if defined(SCTP_ASSOCINFO)
    case SOCKET_OPT_SCTP_ASSOCINFO:
        result = nsetopt_lvl_sctp_associnfo(env, descP, eVal);
        break;
#endif

#if defined(SCTP_AUTOCLOSE)
    case SOCKET_OPT_SCTP_AUTOCLOSE:
        result = nsetopt_lvl_sctp_autoclose(env, descP, eVal);
        break;
#endif

#if defined(SCTP_DISABLE_FRAGMENTS)
    case SOCKET_OPT_SCTP_DISABLE_FRAGMENTS:
        result = nsetopt_lvl_sctp_disable_fragments(env, descP, eVal);
        break;
#endif

#if defined(SCTP_EVENTS)
    case SOCKET_OPT_SCTP_EVENTS:
        result = nsetopt_lvl_sctp_events(env, descP, eVal);
        break;
#endif

#if defined(SCTP_INITMSG)
    case SOCKET_OPT_SCTP_INITMSG:
        result = nsetopt_lvl_sctp_initmsg(env, descP, eVal);
        break;
#endif

#if defined(SCTP_MAXSEG)
    case SOCKET_OPT_SCTP_MAXSEG:
        result = nsetopt_lvl_sctp_maxseg(env, descP, eVal);
        break;
#endif

#if defined(SCTP_NODELAY)
    case SOCKET_OPT_SCTP_NODELAY:
        result = nsetopt_lvl_sctp_nodelay(env, descP, eVal);
        break;
#endif

#if defined(SCTP_RTOINFO)
    case SOCKET_OPT_SCTP_RTOINFO:
        result = nsetopt_lvl_sctp_rtoinfo(env, descP, eVal);
        break;
#endif

    default:
        result = esock_make_error(env, esock_atom_einval);
        break;
    }

    return result;
}


/* nsetopt_lvl_sctp_associnfo - Level SCTP ASSOCINFO option
 */
#if defined(SCTP_ASSOCINFO)
static
ERL_NIF_TERM nsetopt_lvl_sctp_associnfo(ErlNifEnv*        env,
                                        SocketDescriptor* descP,
                                        ERL_NIF_TERM      eVal)
{
    ERL_NIF_TERM            result;
    ERL_NIF_TERM            eAssocId, eMaxRxt, eNumPeerDests;
    ERL_NIF_TERM            ePeerRWND, eLocalRWND, eCookieLife;
    struct sctp_assocparams assocParams;
    int                     res;
    size_t                  sz;
    unsigned int            tmp;
    int32_t                 tmpAssocId;

    SSDBG( descP,
           ("SOCKET", "nsetopt_lvl_sctp_associnfo -> entry with"
            "\r\n   eVal: %T"
            "\r\n", eVal) );

    // It must be a map
    if (!IS_MAP(env, eVal))
        return esock_make_error(env, esock_atom_einval);

    // It must have atleast ten attributes
    if (!enif_get_map_size(env, eVal, &sz) || (sz < 6))
        return esock_make_error(env, esock_atom_einval);

    SSDBG( descP,
           ("SOCKET", "nsetopt_lvl_sctp_associnfo -> extract attributes\r\n") );    

    if (!GET_MAP_VAL(env, eVal, atom_assoc_id,          &eAssocId))
        return esock_make_error(env, esock_atom_einval);

    if (!GET_MAP_VAL(env, eVal, atom_max_rxt,        &eMaxRxt))
        return esock_make_error(env, esock_atom_einval);

    if (!GET_MAP_VAL(env, eVal, atom_num_peer_dests, &eNumPeerDests))
        return esock_make_error(env, esock_atom_einval);

    if (!GET_MAP_VAL(env, eVal, atom_peer_rwnd,      &ePeerRWND))
        return esock_make_error(env, esock_atom_einval);

    if (!GET_MAP_VAL(env, eVal, atom_local_rwnd,     &eLocalRWND))
        return esock_make_error(env, esock_atom_einval);

    if (!GET_MAP_VAL(env, eVal, atom_cookie_life,    &eCookieLife))
        return esock_make_error(env, esock_atom_einval);

    SSDBG( descP,
           ("SOCKET", "nsetopt_lvl_sctp_associnfo -> decode attributes\r\n") );

    /* On some platforms the assoc id is typed as an unsigned integer (uint32)
     * So, to avoid warnings there, we always make an explicit cast... 
     */
    if (!GET_INT(env, eAssocId, &tmpAssocId))
        return esock_make_error(env, esock_atom_einval);
    assocParams.sasoc_assoc_id = (typeof(assocParams.sasoc_assoc_id)) tmpAssocId;
    
    /*
     * We should really make sure this is ok in erlang (to ensure that 
     * the values (max-rxt and num-peer-dests) fits in 16-bits).
     * The value should be a 16-bit unsigned int...
     * Both sasoc_asocmaxrxt and sasoc_number_peer_destinations.
     */
    
    if (!GET_UINT(env, eMaxRxt, &tmp))
        return esock_make_error(env, esock_atom_einval);
    assocParams.sasoc_asocmaxrxt = (uint16_t) tmp;

    if (!GET_UINT(env, eNumPeerDests, &tmp))
        return esock_make_error(env, esock_atom_einval);
    assocParams.sasoc_number_peer_destinations = (uint16_t) tmp;

    if (!GET_UINT(env, ePeerRWND, &assocParams.sasoc_peer_rwnd))
        return esock_make_error(env, esock_atom_einval);

    if (!GET_UINT(env, eLocalRWND, &assocParams.sasoc_local_rwnd))
        return esock_make_error(env, esock_atom_einval);

    if (!GET_UINT(env, eCookieLife, &assocParams.sasoc_cookie_life))
        return esock_make_error(env, esock_atom_einval);
    
    SSDBG( descP,
           ("SOCKET", "nsetopt_lvl_sctp_associnfo -> set associnfo option\r\n") );

    res = socket_setopt(descP->sock, IPPROTO_SCTP, SCTP_ASSOCINFO,
                        &assocParams, sizeof(assocParams));

    if (res != 0)
        result = esock_make_error_errno(env, sock_errno());
    else
        result = esock_atom_ok;

    SSDBG( descP,
           ("SOCKET", "nsetopt_lvl_sctp_associnfo -> done with"
            "\r\n   result: %T"
            "\r\n", result) );

    return result;
    
}
#endif


/* nsetopt_lvl_sctp_autoclose - Level SCTP AUTOCLOSE option
 */
#if defined(SCTP_AUTOCLOSE)
static
ERL_NIF_TERM nsetopt_lvl_sctp_autoclose(ErlNifEnv*        env,
                                        SocketDescriptor* descP,
                                        ERL_NIF_TERM      eVal)
{
    return nsetopt_int_opt(env, descP, IPPROTO_SCTP, SCTP_AUTOCLOSE, eVal);
}
#endif


/* nsetopt_lvl_sctp_disable_fragments - Level SCTP DISABLE_FRAGMENTS option
 */
#if defined(SCTP_DISABLE_FRAGMENTS)
static
ERL_NIF_TERM nsetopt_lvl_sctp_disable_fragments(ErlNifEnv*        env,
                                                SocketDescriptor* descP,
                                                ERL_NIF_TERM      eVal)
{
    return nsetopt_bool_opt(env, descP, IPPROTO_SCTP, SCTP_DISABLE_FRAGMENTS, eVal);
}
#endif


/* nsetopt_lvl_sctp_events - Level SCTP EVENTS option
 */
#if defined(SCTP_EVENTS)
static
ERL_NIF_TERM nsetopt_lvl_sctp_events(ErlNifEnv*        env,
                                     SocketDescriptor* descP,
                                     ERL_NIF_TERM      eVal)
{
    ERL_NIF_TERM                result;
    ERL_NIF_TERM                eDataIn, eAssoc, eAddr, eSndFailure;
    ERL_NIF_TERM                ePeerError, eShutdown, ePartialDelivery;
    ERL_NIF_TERM                eAdaptLayer, eAuth, eSndDry;
    struct sctp_event_subscribe events;
    int                         res;
    size_t                      sz;

    SSDBG( descP,
           ("SOCKET", "nsetopt_lvl_sctp_events -> entry with"
            "\r\n   eVal: %T"
            "\r\n", eVal) );

    // It must be a map
    if (!IS_MAP(env, eVal))
        return esock_make_error(env, esock_atom_einval);

    // It must have atleast ten attributes
    if (!enif_get_map_size(env, eVal, &sz) || (sz < 10))
        return esock_make_error(env, esock_atom_einval);

    SSDBG( descP,
           ("SOCKET", "nsetopt_lvl_sctp_events -> extract attributes\r\n") );    

    if (!GET_MAP_VAL(env, eVal, atom_data_in,          &eDataIn))
        return esock_make_error(env, esock_atom_einval);

    if (!GET_MAP_VAL(env, eVal, atom_association,      &eAssoc))
        return esock_make_error(env, esock_atom_einval);

    if (!GET_MAP_VAL(env, eVal, atom_address,          &eAddr))
        return esock_make_error(env, esock_atom_einval);

    if (!GET_MAP_VAL(env, eVal, atom_send_failure,     &eSndFailure))
        return esock_make_error(env, esock_atom_einval);

    if (!GET_MAP_VAL(env, eVal, atom_peer_error,       &ePeerError))
        return esock_make_error(env, esock_atom_einval);

    if (!GET_MAP_VAL(env, eVal, atom_shutdown,         &eShutdown))
        return esock_make_error(env, esock_atom_einval);

    if (!GET_MAP_VAL(env, eVal, atom_partial_delivery, &ePartialDelivery))
        return esock_make_error(env, esock_atom_einval);

    if (!GET_MAP_VAL(env, eVal, atom_adaptation_layer, &eAdaptLayer))
        return esock_make_error(env, esock_atom_einval);

    if (!GET_MAP_VAL(env, eVal, atom_authentication,   &eAuth))
        return esock_make_error(env, esock_atom_einval);

    if (!GET_MAP_VAL(env, eVal, atom_sender_dry,       &eSndDry))
        return esock_make_error(env, esock_atom_einval);

    SSDBG( descP,
           ("SOCKET", "nsetopt_lvl_sctp_events -> decode attributes\r\n") );

    events.sctp_data_io_event          = esock_decode_bool(eDataIn);
    events.sctp_association_event      = esock_decode_bool(eAssoc);
    events.sctp_address_event          = esock_decode_bool(eAddr);
    events.sctp_send_failure_event     = esock_decode_bool(eSndFailure);
    events.sctp_peer_error_event       = esock_decode_bool(ePeerError);
    events.sctp_shutdown_event         = esock_decode_bool(eShutdown);
    events.sctp_partial_delivery_event = esock_decode_bool(ePartialDelivery);
    events.sctp_adaptation_layer_event = esock_decode_bool(eAdaptLayer);
    events.sctp_authentication_event   = esock_decode_bool(eAuth);
    events.sctp_sender_dry_event       = esock_decode_bool(eSndDry);
    
    SSDBG( descP,
           ("SOCKET", "nsetopt_lvl_sctp_events -> set events option\r\n") );

    res = socket_setopt(descP->sock, IPPROTO_SCTP, SCTP_EVENTS,
                        &events, sizeof(events));

    if (res != 0)
        result = esock_make_error_errno(env, sock_errno());
    else
        result = esock_atom_ok;

    SSDBG( descP,
           ("SOCKET", "nsetopt_lvl_sctp_events -> done with"
            "\r\n   result: %T"
            "\r\n", result) );

    return result;
    
}
#endif


/* nsetopt_lvl_sctp_initmsg - Level SCTP INITMSG option
 */
#if defined(SCTP_INITMSG)
static
ERL_NIF_TERM nsetopt_lvl_sctp_initmsg(ErlNifEnv*        env,
                                      SocketDescriptor* descP,
                                      ERL_NIF_TERM      eVal)
{
    ERL_NIF_TERM        result;
    ERL_NIF_TERM        eNumOut, eMaxIn, eMaxAttempts, eMaxInitTO;
    struct sctp_initmsg initMsg;
    int                 res;
    size_t              sz;
    unsigned int        tmp;

    SSDBG( descP,
           ("SOCKET", "nsetopt_lvl_sctp_initmsg -> entry with"
            "\r\n   eVal: %T"
            "\r\n", eVal) );

    // It must be a map
    if (!IS_MAP(env, eVal))
        return esock_make_error(env, esock_atom_einval);

    // It must have atleast ten attributes
    if (!enif_get_map_size(env, eVal, &sz) || (sz < 4))
        return esock_make_error(env, esock_atom_einval);

    SSDBG( descP,
           ("SOCKET", "nsetopt_lvl_sctp_initmsg -> extract attributes\r\n") );    

    if (!GET_MAP_VAL(env, eVal, atom_num_outstreams, &eNumOut))
        return esock_make_error(env, esock_atom_einval);

    if (!GET_MAP_VAL(env, eVal, atom_max_instreams,  &eMaxIn))
        return esock_make_error(env, esock_atom_einval);

    if (!GET_MAP_VAL(env, eVal, atom_max_attempts,   &eMaxAttempts))
        return esock_make_error(env, esock_atom_einval);

    if (!GET_MAP_VAL(env, eVal, atom_max_init_timeo, &eMaxInitTO))
        return esock_make_error(env, esock_atom_einval);

    SSDBG( descP,
           ("SOCKET", "nsetopt_lvl_sctp_initmsg -> decode attributes\r\n") );

    if (!GET_UINT(env, eNumOut, &tmp))
        return esock_make_error(env, esock_atom_einval);
    initMsg.sinit_num_ostreams = (uint16_t) tmp;
    
    if (!GET_UINT(env, eMaxIn, &tmp))
        return esock_make_error(env, esock_atom_einval);
    initMsg.sinit_max_instreams = (uint16_t) tmp;
    
    if (!GET_UINT(env, eMaxAttempts, &tmp))
        return esock_make_error(env, esock_atom_einval);
    initMsg.sinit_max_attempts = (uint16_t) tmp;
    
    if (!GET_UINT(env, eMaxInitTO, &tmp))
        return esock_make_error(env, esock_atom_einval);
    initMsg.sinit_max_init_timeo = (uint16_t) tmp;
    
    SSDBG( descP,
           ("SOCKET", "nsetopt_lvl_sctp_initmsg -> set initmsg option\r\n") );

    res = socket_setopt(descP->sock, IPPROTO_SCTP, SCTP_INITMSG,
                        &initMsg, sizeof(initMsg));

    if (res != 0)
        result = esock_make_error_errno(env, sock_errno());
    else
        result = esock_atom_ok;

    SSDBG( descP,
           ("SOCKET", "nsetopt_lvl_sctp_initmsg -> done with"
            "\r\n   result: %T"
            "\r\n", result) );

    return result;
    
}
#endif


/* nsetopt_lvl_sctp_maxseg - Level SCTP MAXSEG option
 */
#if defined(SCTP_MAXSEG)
static
ERL_NIF_TERM nsetopt_lvl_sctp_maxseg(ErlNifEnv*        env,
                                     SocketDescriptor* descP,
                                     ERL_NIF_TERM      eVal)
{
    return nsetopt_int_opt(env, descP, IPPROTO_SCTP, SCTP_MAXSEG, eVal);
}
#endif


/* nsetopt_lvl_sctp_nodelay - Level SCTP NODELAY option
 */
#if defined(SCTP_NODELAY)
static
ERL_NIF_TERM nsetopt_lvl_sctp_nodelay(ErlNifEnv*        env,
                                      SocketDescriptor* descP,
                                      ERL_NIF_TERM      eVal)
{
    return nsetopt_bool_opt(env, descP, IPPROTO_SCTP, SCTP_NODELAY, eVal);
}
#endif


/* nsetopt_lvl_sctp_rtoinfo - Level SCTP RTOINFO option
 */
#if defined(SCTP_RTOINFO)
static
ERL_NIF_TERM nsetopt_lvl_sctp_rtoinfo(ErlNifEnv*        env,
                                      SocketDescriptor* descP,
                                      ERL_NIF_TERM      eVal)
{
    ERL_NIF_TERM        result;
    ERL_NIF_TERM        eAssocId, eInitial, eMax, eMin;
    struct sctp_rtoinfo rtoInfo;
    int                 res;
    size_t              sz;
    int32_t             tmpAssocId;

    SSDBG( descP,
           ("SOCKET", "nsetopt_lvl_sctp_rtoinfo -> entry with"
            "\r\n   eVal: %T"
            "\r\n", eVal) );

    // It must be a map
    if (!IS_MAP(env, eVal))
        return esock_make_error(env, esock_atom_einval);

    // It must have atleast ten attributes
    if (!enif_get_map_size(env, eVal, &sz) || (sz < 4))
        return esock_make_error(env, esock_atom_einval);

    SSDBG( descP,
           ("SOCKET", "nsetopt_lvl_sctp_rtoinfo -> extract attributes\r\n") );    

    if (!GET_MAP_VAL(env, eVal, atom_assoc_id, &eAssocId))
        return esock_make_error(env, esock_atom_einval);

    if (!GET_MAP_VAL(env, eVal, atom_initial,  &eInitial))
        return esock_make_error(env, esock_atom_einval);

    if (!GET_MAP_VAL(env, eVal, atom_max,      &eMax))
        return esock_make_error(env, esock_atom_einval);

    if (!GET_MAP_VAL(env, eVal, atom_min,      &eMin))
        return esock_make_error(env, esock_atom_einval);

    SSDBG( descP,
           ("SOCKET", "nsetopt_lvl_sctp_rtoinfo -> decode attributes\r\n") );

    /* On some platforms the assoc id is typed as an unsigned integer (uint32)
     * So, to avoid warnings there, we always make an explicit cast... 
     */
    if (!GET_INT(env, eAssocId, &tmpAssocId))
        return esock_make_error(env, esock_atom_einval);
    rtoInfo.srto_assoc_id = (typeof(rtoInfo.srto_assoc_id)) tmpAssocId;
    
    if (!GET_UINT(env, eInitial, &rtoInfo.srto_initial))
        return esock_make_error(env, esock_atom_einval);

    if (!GET_UINT(env, eMax, &rtoInfo.srto_max))
        return esock_make_error(env, esock_atom_einval);

    if (!GET_UINT(env, eMin, &rtoInfo.srto_min))
        return esock_make_error(env, esock_atom_einval);

    SSDBG( descP,
           ("SOCKET", "nsetopt_lvl_sctp_rtoinfo -> set associnfo option\r\n") );

    res = socket_setopt(descP->sock, IPPROTO_SCTP, SCTP_RTOINFO,
                        &rtoInfo, sizeof(rtoInfo));

    if (res != 0)
        result = esock_make_error_errno(env, sock_errno());
    else
        result = esock_atom_ok;

    SSDBG( descP,
           ("SOCKET", "nsetopt_lvl_sctp_rtoinfo -> done with"
            "\r\n   result: %T"
            "\r\n", result) );

    return result;
    
}
#endif



#endif // defined(HAVE_SCTP)




/* nsetopt_bool_opt - set an option that has an (integer) bool value
 */
static
ERL_NIF_TERM nsetopt_bool_opt(ErlNifEnv*        env,
                              SocketDescriptor* descP,
                              int               level,
                              int               opt,
                              ERL_NIF_TERM      eVal)
{
    ERL_NIF_TERM result;
    BOOLEAN_T    val;
    int          ival, res;

    val = esock_decode_bool(eVal);
    
    ival = (val) ? 1 : 0;
    res  = socket_setopt(descP->sock, level, opt, &ival, sizeof(ival));

    if (res != 0)
        result = esock_make_error_errno(env, sock_errno());
    else
        result = esock_atom_ok;

    return result;
}


/* nsetopt_int_opt - set an option that has an integer value
 */
static
ERL_NIF_TERM nsetopt_int_opt(ErlNifEnv*        env,
                             SocketDescriptor* descP,
                             int               level,
                             int               opt,
                             ERL_NIF_TERM      eVal)
{
    ERL_NIF_TERM result;
    int          val;

    if (GET_INT(env, eVal, &val)) {
        int res = socket_setopt(descP->sock, level, opt, &val, sizeof(val));

        if (res != 0)
            result = esock_make_error_errno(env, sock_errno());
        else
            result = esock_atom_ok;

    } else {
        result = esock_make_error(env, esock_atom_einval);
    }

    return result;
}


/* nsetopt_str_opt - set an option that has an string value
 */
static
ERL_NIF_TERM nsetopt_str_opt(ErlNifEnv*        env,
                             SocketDescriptor* descP,
                             int               level,
                             int               opt,
                             int               max,
                             ERL_NIF_TERM      eVal)
{
    ERL_NIF_TERM result;
    char*        val = MALLOC(max);

    if (GET_STR(env, eVal, val, max) > 0) {
        int optLen = strlen(val);
        int res    = socket_setopt(descP->sock, level, opt, &val, optLen);

        if (res != 0)
            result = esock_make_error_errno(env, sock_errno());
        else
            result = esock_atom_ok;

    } else {
        result = esock_make_error(env, esock_atom_einval);
    }

    FREE(val);

    return result;
}


/* nsetopt_timeval_opt - set an option that has an (timeval) bool value
 */
static
ERL_NIF_TERM nsetopt_timeval_opt(ErlNifEnv*        env,
                                 SocketDescriptor* descP,
                                 int               level,
                                 int               opt,
                                 ERL_NIF_TERM      eVal)
{
    ERL_NIF_TERM   result;
    struct timeval timeVal;
    int            res;
    char*          xres;

    SSDBG( descP,
           ("SOCKET", "nsetopt_timeval_opt -> entry with"
            "\r\n   eVal: %T"
            "\r\n", eVal) );

    if ((xres = esock_decode_timeval(env, eVal, &timeVal)) != NULL)
        return esock_make_error_str(env, xres);

    SSDBG( descP,
           ("SOCKET", "nsetopt_timeval_opt -> set timeval option\r\n") );

    res = socket_setopt(descP->sock, level, opt, &timeVal, sizeof(timeVal));

    if (res != 0)
        result = esock_make_error_errno(env, sock_errno());
    else
        result = esock_atom_ok;

    SSDBG( descP,
           ("SOCKET", "nsetopt_timeval_opt -> done with"
            "\r\n   result: %T"
            "\r\n", result) );

    return result;
    
}



static
BOOLEAN_T elevel2level(BOOLEAN_T  isEncoded,
                       int        eLevel,
                       BOOLEAN_T* isOTP,
                       int*       level)
{
    BOOLEAN_T result;

    if (isEncoded) {
        switch (eLevel) {
        case SOCKET_OPT_LEVEL_OTP:
            *isOTP = TRUE;
            *level = -1;
            result = TRUE;
            break;

        case SOCKET_OPT_LEVEL_SOCKET:
            *isOTP = FALSE;
            *level = SOL_SOCKET;
            result = TRUE;
            break;

        case SOCKET_OPT_LEVEL_IP:
            *isOTP = FALSE;
#if defined(SOL_IP)
            *level = SOL_IP;
#else
            *level = IPPROTO_IP;
#endif
            result = TRUE;
            break;

#if defined(SOL_IPV6)
        case SOCKET_OPT_LEVEL_IPV6:
            *isOTP = FALSE;
            *level = SOL_IPV6;
            result = TRUE;
            break;
#endif

        case SOCKET_OPT_LEVEL_TCP:
            *isOTP = FALSE;
            *level = IPPROTO_TCP;
            result = TRUE;
            break;

        case SOCKET_OPT_LEVEL_UDP:
            *isOTP = FALSE;
            *level = IPPROTO_UDP;
            result = TRUE;
            break;

#ifdef HAVE_SCTP
        case SOCKET_OPT_LEVEL_SCTP:
            *isOTP = FALSE;
            *level = IPPROTO_SCTP;
            result = TRUE;
            break;
#endif

        default:
            *isOTP = FALSE;
            *level = -1;
            result = FALSE;
            break;
        }
    } else {
        *isOTP = FALSE;
        *level = eLevel;
        result = TRUE;
    }

    return result;
}



/* +++ socket_setopt +++
 *
 * <Per H @ Tail-f>
 * The original code here had problems that possibly
 * only occur if you abuse it for non-INET sockets, but anyway:
 * a) If the getsockopt for SO_PRIORITY or IP_TOS failed, the actual
 *    requested setsockopt was never even attempted.
 * b) If {get,set}sockopt for one of IP_TOS and SO_PRIORITY failed,
 *    but ditto for the other worked and that was actually the requested
 *    option, failure was still reported to erlang.
 * </Per H @ Tail-f>
 *
 * <PaN>
 * The relations between SO_PRIORITY, TOS and other options
 * is not what you (or at least I) would expect...:
 * If TOS is set after priority, priority is zeroed.
 * If any other option is set after tos, tos might be zeroed.
 * Therefore, save tos and priority. If something else is set,
 * restore both after setting, if  tos is set, restore only
 * prio and if prio is set restore none... All to keep the
 * user feeling socket options are independent.
 * </PaN>
 */
static
int socket_setopt(int sock, int level, int opt,
                  const void* optVal, const socklen_t optLen)
{
    int res;

#if  defined(IP_TOS) && defined(SOL_IP) && defined(SO_PRIORITY)
    int          tmpIValPRIO;
    int          tmpIValTOS;
    int          resPRIO;
    int          resTOS;
    SOCKOPTLEN_T tmpArgSzPRIO = sizeof(tmpIValPRIO);
    SOCKOPTLEN_T tmpArgSzTOS  = sizeof(tmpIValTOS);

    resPRIO = sock_getopt(sock, SOL_SOCKET, SO_PRIORITY,
                           &tmpIValPRIO, &tmpArgSzPRIO);
    resTOS  = sock_getopt(sock, SOL_IP, IP_TOS,
                          &tmpIValTOS, &tmpArgSzTOS);

    res = sock_setopt(sock, level, opt, optVal, optLen);
    if (res == 0) {

        /* Ok, now we *maybe* need to "maybe" restore PRIO and TOS...
         * maybe, possibly, ...
         */

        if (opt != SO_PRIORITY) {
	    if ((opt != IP_TOS) && (resTOS == 0)) {
		resTOS = sock_setopt(sock, SOL_IP, IP_TOS,
                                     (void *) &tmpIValTOS,
                                     tmpArgSzTOS);
                res = resTOS;
            }
	    if ((res == 0) && (resPRIO == 0)) {
		resPRIO = sock_setopt(sock, SOL_SOCKET, SO_PRIORITY,
                                      &tmpIValPRIO,
                                      tmpArgSzPRIO);

                /* Some kernels set a SO_PRIORITY by default
                 * that you are not permitted to reset,
                 * silently ignore this error condition.
                 */

                if ((resPRIO != 0) && (sock_errno() == EPERM)) {
                    res = 0;
                } else {
                    res = resPRIO;
		}
	    }
	}
    }

#else

    res = sock_setopt(sock, level, opt, optVal, optLen);

#endif

    return res;
}



/* ----------------------------------------------------------------------
 * nif_getopt
 *
 * Description:
 * Get socket option.
 * Its possible to use a "raw" mode (not encoded). That is, we do not
 * interpret level and opt. They are passed "as is" to the
 * getsockopt function call. The value in this case will "copied" as
 * is and provided to the user in the form of a binary.
 *
 * Arguments:
 * Socket (ref) - Points to the socket descriptor.
 * IsEncoded    - Are the "arguments" encoded or not.
 * Level        - Level of the socket option.
 * Opt          - The socket option.
 */

static
ERL_NIF_TERM nif_getopt(ErlNifEnv*         env,
                        int                argc,
                        const ERL_NIF_TERM argv[])
{
    SocketDescriptor* descP;
    int               eLevel, level = -1;
    ERL_NIF_TERM      eIsEncoded, eOpt;
    BOOLEAN_T         isEncoded, isOTP;

    SGDBG( ("SOCKET", "nif_getopt -> entry with argc: %d\r\n", argc) );

    if ((argc != 4) ||
        !enif_get_resource(env, argv[0], sockets, (void**) &descP) ||
        !GET_INT(env, argv[2], &eLevel)) {
        SGDBG( ("SOCKET", "nif_getopt -> failed processing args\r\n") );
        return enif_make_badarg(env);
    }
    eIsEncoded = argv[1];
    eOpt       = argv[3]; // Is "normally" an int, but if raw mode: {Int, ValueSz}

    SSDBG( descP,
           ("SOCKET", "nif_getopt -> args when sock = %d:"
            "\r\n   Socket:     %T"
            "\r\n   eIsEncoded: %T"
            "\r\n   eLevel:     %d"
            "\r\n   eOpt:       %T"
            "\r\n", descP->sock, argv[0], eIsEncoded, eLevel, eOpt) );

    isEncoded = esock_decode_bool(eIsEncoded);

    if (!elevel2level(isEncoded, eLevel, &isOTP, &level))
        return esock_make_error(env, esock_atom_einval);

    return ngetopt(env, descP, isEncoded, isOTP, level, eOpt);
}



static
ERL_NIF_TERM ngetopt(ErlNifEnv*        env,
                     SocketDescriptor* descP,
                     BOOLEAN_T         isEncoded,
                     BOOLEAN_T         isOTP,
                     int               level,
                     ERL_NIF_TERM      eOpt)
{
    ERL_NIF_TERM result;
    int          opt;

    SSDBG( descP,
           ("SOCKET", "ngetopt -> entry with"
            "\r\n   isEncoded: %s"
            "\r\n   isOTP:     %s"
            "\r\n   level:     %d"
            "\r\n   eOpt:      %T"
            "\r\n", B2S(isEncoded), B2S(isOTP), level, eOpt) );

    if (isOTP) {
        /* These are not actual socket options,
         * but options for our implementation.
         */
        if (GET_INT(env, eOpt, &opt))
            result = ngetopt_otp(env, descP, opt);
        else
            result = esock_make_error(env, esock_atom_einval);
    } else if (!isEncoded) {
        result = ngetopt_native(env, descP, level, eOpt);
    } else {
        if (GET_INT(env, eOpt, &opt))
            result = ngetopt_level(env, descP, level, opt);
        else
            result = esock_make_error(env, esock_atom_einval);
    }

    SSDBG( descP,
           ("SOCKET", "ngetopt -> done when"
            "\r\n   result: %T"
            "\r\n", result) );

    return result;
}



/* ngetopt_otp - Handle OTP (level) options
 */
static
ERL_NIF_TERM ngetopt_otp(ErlNifEnv*        env,
                         SocketDescriptor* descP,
                         int               eOpt)
{
    ERL_NIF_TERM result;

    SSDBG( descP,
           ("SOCKET", "ngetopt_otp -> entry with"
            "\r\n   eOpt: %d"
            "\r\n", eOpt) );

    switch (eOpt) {
    case SOCKET_OPT_OTP_DEBUG:
        result = ngetopt_otp_debug(env, descP);
        break;

    case SOCKET_OPT_OTP_IOW:
        result = ngetopt_otp_iow(env, descP);
        break;

    case SOCKET_OPT_OTP_CTRL_PROC:
        result = ngetopt_otp_ctrl_proc(env, descP);
        break;

    case SOCKET_OPT_OTP_RCVBUF:
        result = ngetopt_otp_rcvbuf(env, descP);
        break;

    case SOCKET_OPT_OTP_RCVCTRLBUF:
        result = ngetopt_otp_rcvctrlbuf(env, descP);
        break;

    case SOCKET_OPT_OTP_SNDCTRLBUF:
        result = ngetopt_otp_sndctrlbuf(env, descP);
        break;

    default:
        result = esock_make_error(env, esock_atom_einval);
        break;
    }

    SSDBG( descP,
           ("SOCKET", "ngetopt_otp -> done when"
            "\r\n   result: %T"
            "\r\n", result) );

    return result;
}


/* ngetopt_otp_debug - Handle the OTP (level) debug options
 */
static
ERL_NIF_TERM ngetopt_otp_debug(ErlNifEnv*        env,
                               SocketDescriptor* descP)
{
    ERL_NIF_TERM eVal = esock_encode_bool(descP->dbg);

    return esock_make_ok2(env, eVal);
}


/* ngetopt_otp_iow - Handle the OTP (level) iow options
 */
static
ERL_NIF_TERM ngetopt_otp_iow(ErlNifEnv*        env,
                             SocketDescriptor* descP)
{
    ERL_NIF_TERM eVal = esock_encode_bool(descP->iow);

    return esock_make_ok2(env, eVal);
}


/* ngetopt_otp_ctrl_proc - Handle the OTP (level) controlling_process options
 */
static
ERL_NIF_TERM ngetopt_otp_ctrl_proc(ErlNifEnv*        env,
                                   SocketDescriptor* descP)
{
    ERL_NIF_TERM eVal = MKPID(env, &descP->ctrlPid);

    return esock_make_ok2(env, eVal);
}



/* ngetopt_otp_rcvbuf - Handle the OTP (level) rcvbuf options
 */
static
ERL_NIF_TERM ngetopt_otp_rcvbuf(ErlNifEnv*        env,
                                SocketDescriptor* descP)
{
    ERL_NIF_TERM eVal = MKI(env, descP->rBufSz);

    return esock_make_ok2(env, eVal);
}


/* ngetopt_otp_rcvctrlbuf - Handle the OTP (level) rcvctrlbuf options
 */
static
ERL_NIF_TERM ngetopt_otp_rcvctrlbuf(ErlNifEnv*        env,
                                    SocketDescriptor* descP)
{
    ERL_NIF_TERM eVal = MKI(env, descP->rCtrlSz);

    return esock_make_ok2(env, eVal);
}


/* ngetopt_otp_sndctrlbuf - Handle the OTP (level) sndctrlbuf options
 */
static
ERL_NIF_TERM ngetopt_otp_sndctrlbuf(ErlNifEnv*        env,
                                    SocketDescriptor* descP)
{
    ERL_NIF_TERM eVal = MKI(env, descP->wCtrlSz);

    return esock_make_ok2(env, eVal);
}


/* The option has *not* been encoded. Instead it has been provided
 * in "native mode" (option is provided as is). In this case it will have the
 * format: {NativeOpt :: integer(), ValueSize :: non_neg_integer()}
 */
static
ERL_NIF_TERM ngetopt_native(ErlNifEnv*        env,
                            SocketDescriptor* descP,
                            int               level,
                            ERL_NIF_TERM      eOpt)
{
    ERL_NIF_TERM result = enif_make_badarg(env);
    int          opt;
    uint16_t     valueType;
    SOCKOPTLEN_T valueSz;

    SSDBG( descP,
           ("SOCKET", "ngetopt_native -> entry with"
            "\r\n   level: %d"
            "\r\n   eOpt:  %T"
            "\r\n", level, eOpt) );

    /* <KOLLA>
     * We should really make it possible to specify more common specific types,
     * such as integer or boolean (instead of the size)...
     * </KOLLA>
     */

    if (decode_native_get_opt(env, eOpt, &opt, &valueType, (int*) &valueSz)) {

        SSDBG( descP,
               ("SOCKET", "ngetopt_native -> decoded opt"
                "\r\n   valueType: %d (%s)"
                "\r\n   ValueSize: %d"
                "\r\n", valueType, VT2S(valueType), valueSz) );

        switch (valueType) {
        case SOCKET_OPT_VALUE_TYPE_UNSPEC:
            result = ngetopt_native_unspec(env, descP, level, opt, valueSz);
            break;
        case SOCKET_OPT_VALUE_TYPE_INT:
            result = ngetopt_int_opt(env, descP, level, opt);
            break;
        case SOCKET_OPT_VALUE_TYPE_BOOL:
            result = ngetopt_bool_opt(env, descP, level, opt);
            break;
        default:
            result = esock_make_error(env, esock_atom_einval);
            break;
        }
    } else {
        result = esock_make_error(env, esock_atom_einval);
    }

    SSDBG( descP,
           ("SOCKET", "ngetopt_native -> done when"
            "\r\n   result: %T"
            "\r\n", result) );

    return result;
}


static
ERL_NIF_TERM ngetopt_native_unspec(ErlNifEnv*        env,
                                   SocketDescriptor* descP,
                                   int               level,
                                   int               opt,
                                   SOCKOPTLEN_T      valueSz)
{
    ERL_NIF_TERM result = esock_make_error(env, esock_atom_einval);
    int          res;

    SSDBG( descP,
           ("SOCKET", "ngetopt_native_unspec -> entry with"
            "\r\n   level:   %d"
            "\r\n   opt:     %d"
            "\r\n   valueSz: %d"
            "\r\n", level, opt, valueSz) );

    if (valueSz == 0) {
        res = sock_getopt(descP->sock, level, opt, NULL, NULL);
        if (res != 0)
            result = esock_make_error_errno(env, sock_errno());
        else
            result = esock_atom_ok;
    } else {
        SOCKOPTLEN_T vsz = valueSz;
        ErlNifBinary val;

        SSDBG( descP, ("SOCKET", "ngetopt_native_unspec -> try alloc buffer\r\n") );

        if (ALLOC_BIN(vsz, &val)) {
            int saveErrno;
            res = sock_getopt(descP->sock, level, opt, val.data, &vsz);
            if (res != 0) {
                saveErrno = sock_errno();
                
                result = esock_make_error_errno(env, saveErrno);
            } else {

                /* Did we use all of the buffer? */
                if (vsz == val.size) {

                    result = esock_make_ok2(env, MKBIN(env, &val));

                } else {

                    ERL_NIF_TERM tmp;

                    tmp = MKBIN(env, &val);
                    tmp = MKSBIN(env, tmp, 0, vsz);
                    
                    result = esock_make_ok2(env, tmp);
                }
            }
        } else {
            result = enif_make_badarg(env);
        }
    }

    SSDBG( descP,
           ("SOCKET", "ngetopt_native_unspec -> done when"
            "\r\n   result: %T"
            "\r\n", result) );

    return result;
}



/* ngetopt_level - A "proper" level (option) has been specified
 */
static
ERL_NIF_TERM ngetopt_level(ErlNifEnv*        env,
                           SocketDescriptor* descP,
                           int               level,
                           int               eOpt)
{
    ERL_NIF_TERM result;

    SSDBG( descP,
           ("SOCKET", "ngetopt_level -> entry with"
            "\r\n   level: %d"
            "\r\n   eOpt:  %d"
            "\r\n", level, eOpt) );

    switch (level) {
    case SOL_SOCKET:
        result = ngetopt_lvl_socket(env, descP, eOpt);
        break;

#if defined(SOL_IP)
    case SOL_IP:
#else
    case IPPROTO_IP:
#endif
        result = ngetopt_lvl_ip(env, descP, eOpt);
        break;

#if defined(SOL_IPV6)
    case SOL_IPV6:
        result = ngetopt_lvl_ipv6(env, descP, eOpt);
        break;
#endif

    case IPPROTO_TCP:
        result = ngetopt_lvl_tcp(env, descP, eOpt);
        break;

    case IPPROTO_UDP:
        result = ngetopt_lvl_udp(env, descP, eOpt);
        break;

#if defined(HAVE_SCTP)
    case IPPROTO_SCTP:
        result = ngetopt_lvl_sctp(env, descP, eOpt);
        break;
#endif

    default:
        result = esock_make_error(env, esock_atom_einval);
        break;
    }

    SSDBG( descP,
           ("SOCKET", "ngetopt_level -> done when"
            "\r\n   result: %T"
            "\r\n", result) );

    return result;
}


/* ngetopt_lvl_socket - Level *SOCKET* option
 */
static
ERL_NIF_TERM ngetopt_lvl_socket(ErlNifEnv*        env,
                                SocketDescriptor* descP,
                                int               eOpt)
{
    ERL_NIF_TERM result;

    SSDBG( descP,
           ("SOCKET", "ngetopt_lvl_socket -> entry with"
            "\r\n   eOpt:  %d"
            "\r\n", eOpt) );

    switch (eOpt) {
#if defined(SO_ACCEPTCONN)
    case SOCKET_OPT_SOCK_ACCEPTCONN:
        result = ngetopt_lvl_sock_acceptconn(env, descP);
        break;
#endif

#if defined(SO_BINDTODEVICE)
    case SOCKET_OPT_SOCK_BINDTODEVICE:
        result = ngetopt_lvl_sock_bindtodevice(env, descP);
        break;
#endif

#if defined(SO_BROADCAST)
    case SOCKET_OPT_SOCK_BROADCAST:
        result = ngetopt_lvl_sock_broadcast(env, descP);
        break;
#endif

#if defined(SO_DEBUG)
    case SOCKET_OPT_SOCK_DEBUG:
        result = ngetopt_lvl_sock_debug(env, descP);
        break;
#endif

#if defined(SO_DOMAIN)
    case SOCKET_OPT_SOCK_DOMAIN:
        result = ngetopt_lvl_sock_domain(env, descP);
        break;
#endif

#if defined(SO_DONTROUTE)
    case SOCKET_OPT_SOCK_DONTROUTE:
        result = ngetopt_lvl_sock_dontroute(env, descP);
        break;
#endif

#if defined(SO_KEEPALIVE)
    case SOCKET_OPT_SOCK_KEEPALIVE:
        result = ngetopt_lvl_sock_keepalive(env, descP);
        break;
#endif

#if defined(SO_LINGER)
    case SOCKET_OPT_SOCK_LINGER:
        result = ngetopt_lvl_sock_linger(env, descP);
        break;
#endif

#if defined(SO_OOBINLINE)
    case SOCKET_OPT_SOCK_OOBINLINE:
        result = ngetopt_lvl_sock_oobinline(env, descP);
        break;
#endif

#if defined(SO_PEEK_OFF)
    case SOCKET_OPT_SOCK_PEEK_OFF:
        result = ngetopt_lvl_sock_peek_off(env, descP);
        break;
#endif

#if defined(SO_PRIORITY)
    case SOCKET_OPT_SOCK_PRIORITY:
        result = ngetopt_lvl_sock_priority(env, descP);
        break;
#endif

#if defined(SO_PROTOCOL)
    case SOCKET_OPT_SOCK_PROTOCOL:
        result = ngetopt_lvl_sock_protocol(env, descP);
        break;
#endif

#if defined(SO_RCVBUF)
    case SOCKET_OPT_SOCK_RCVBUF:
        result = ngetopt_lvl_sock_rcvbuf(env, descP);
        break;
#endif

#if defined(SO_RCVLOWAT)
    case SOCKET_OPT_SOCK_RCVLOWAT:
        result = ngetopt_lvl_sock_rcvlowat(env, descP);
        break;
#endif

#if defined(SO_RCVTIMEO)
    case SOCKET_OPT_SOCK_RCVTIMEO:
        result = ngetopt_lvl_sock_rcvtimeo(env, descP);
        break;
#endif

#if defined(SO_REUSEADDR)
    case SOCKET_OPT_SOCK_REUSEADDR:
        result = ngetopt_lvl_sock_reuseaddr(env, descP);
        break;
#endif

#if defined(SO_REUSEPORT)
    case SOCKET_OPT_SOCK_REUSEPORT:
        result = ngetopt_lvl_sock_reuseport(env, descP);
        break;
#endif

#if defined(SO_SNDBUF)
    case SOCKET_OPT_SOCK_SNDBUF:
        result = ngetopt_lvl_sock_sndbuf(env, descP);
        break;
#endif

#if defined(SO_SNDLOWAT)
    case SOCKET_OPT_SOCK_SNDLOWAT:
        result = ngetopt_lvl_sock_sndlowat(env, descP);
        break;
#endif

#if defined(SO_SNDTIMEO)
    case SOCKET_OPT_SOCK_SNDTIMEO:
        result = ngetopt_lvl_sock_sndtimeo(env, descP);
        break;
#endif

#if defined(SO_TIMESTAMP)
    case SOCKET_OPT_SOCK_TIMESTAMP:
        result = ngetopt_lvl_sock_timestamp(env, descP);
        break;
#endif

#if defined(SO_TYPE)
    case SOCKET_OPT_SOCK_TYPE:
        result = ngetopt_lvl_sock_type(env, descP);
        break;
#endif

    default:
        result = esock_make_error(env, esock_atom_einval);
        break;
    }

    SSDBG( descP,
           ("SOCKET", "ngetopt_lvl_socket -> done when"
            "\r\n   result:  %T"
            "\r\n", result) );

    return result;
}


#if defined(SO_ACCEPTCONN)
static
ERL_NIF_TERM ngetopt_lvl_sock_acceptconn(ErlNifEnv*        env,
                                        SocketDescriptor* descP)
{
    return ngetopt_bool_opt(env, descP, SOL_SOCKET, SO_ACCEPTCONN);
}
#endif


#if defined(SO_BINDTODEVICE)
static
ERL_NIF_TERM ngetopt_lvl_sock_bindtodevice(ErlNifEnv*        env,
                                           SocketDescriptor* descP)
{
    SSDBG( descP,
           ("SOCKET", "ngetopt_lvl_sock_bindtodevice -> entry with\r\n") );

    return ngetopt_str_opt(env, descP, SOL_SOCKET, SO_BROADCAST, IFNAMSIZ+1);
}
#endif


#if defined(SO_BROADCAST)
static
ERL_NIF_TERM ngetopt_lvl_sock_broadcast(ErlNifEnv*        env,
                                        SocketDescriptor* descP)
{
    return ngetopt_bool_opt(env, descP, SOL_SOCKET, SO_BROADCAST);
}
#endif


#if defined(SO_DEBUG)
static
ERL_NIF_TERM ngetopt_lvl_sock_debug(ErlNifEnv*        env,
                                        SocketDescriptor* descP)
{
    return ngetopt_int_opt(env, descP, SOL_SOCKET, SO_DEBUG);
}
#endif


#if defined(SO_DOMAIN)
static
ERL_NIF_TERM ngetopt_lvl_sock_domain(ErlNifEnv*        env,
                                     SocketDescriptor* descP)
{
    ERL_NIF_TERM result;
    int          val;
    SOCKOPTLEN_T valSz = sizeof(val);
    int          res;

    res = sock_getopt(descP->sock, SOL_SOCKET, SO_DOMAIN,
                      &val, &valSz);

    if (res != 0) {
        result = esock_make_error_errno(env, sock_errno());
    } else {
        switch (val) {
        case AF_INET:
            result = esock_make_ok2(env, esock_atom_inet);
            break;

#if defined(HAVE_IN6) && defined(AF_INET6)
        case AF_INET6:
            result = esock_make_ok2(env, esock_atom_inet6);
            break;
#endif

#ifdef HAVE_SYS_UN_H
        case AF_UNIX:
        result = esock_make_ok2(env, esock_atom_local);
        break;
#endif

        default:
            result = esock_make_error(env,
                                      MKT2(env,
                                           esock_atom_unknown,
                                           MKI(env, val)));
            break;
        }
    }

    return result;
}
#endif


#if defined(SO_DONTROUTE)
static
ERL_NIF_TERM ngetopt_lvl_sock_dontroute(ErlNifEnv*        env,
                                        SocketDescriptor* descP)
{
    return ngetopt_bool_opt(env, descP, SOL_SOCKET, SO_DONTROUTE);
}
#endif


#if defined(SO_KEEPALIVE)
static
ERL_NIF_TERM ngetopt_lvl_sock_keepalive(ErlNifEnv*        env,
                                        SocketDescriptor* descP)
{
    return ngetopt_bool_opt(env, descP, SOL_SOCKET, SO_KEEPALIVE);
}
#endif


#if defined(SO_LINGER)
static
ERL_NIF_TERM ngetopt_lvl_sock_linger(ErlNifEnv*        env,
                                     SocketDescriptor* descP)
{
    ERL_NIF_TERM  result;
    struct linger val;
    SOCKOPTLEN_T  valSz = sizeof(val);
    int           res;

    sys_memzero((void *) &val, sizeof(val));

    res = sock_getopt(descP->sock, SOL_SOCKET, SO_LINGER,
                      &val, &valSz);

    if (res != 0) {
        result = esock_make_error_errno(env, sock_errno());
    } else {
        ERL_NIF_TERM lOnOff = ((val.l_onoff) ? atom_true : atom_false);
        ERL_NIF_TERM lSecs  = MKI(env, val.l_linger);
        ERL_NIF_TERM linger = MKT2(env, lOnOff, lSecs);

        result = esock_make_ok2(env, linger);
    }

    return result;
}
#endif


#if defined(SO_OOBINLINE)
static
ERL_NIF_TERM ngetopt_lvl_sock_oobinline(ErlNifEnv*        env,
                                        SocketDescriptor* descP)
{
    return ngetopt_bool_opt(env, descP, SOL_SOCKET, SO_OOBINLINE);
}
#endif


#if defined(SO_PEEK_OFF)
static
ERL_NIF_TERM ngetopt_lvl_sock_peek_off(ErlNifEnv*        env,
                                       SocketDescriptor* descP)
{
    return ngetopt_int_opt(env, descP, SOL_SOCKET, SO_PEEK_OFF);
}
#endif


#if defined(SO_PRIORITY)
static
ERL_NIF_TERM ngetopt_lvl_sock_priority(ErlNifEnv*        env,
                                       SocketDescriptor* descP)
{
    return ngetopt_int_opt(env, descP, SOL_SOCKET, SO_PRIORITY);
}
#endif


#if defined(SO_PROTOCOL)
static
ERL_NIF_TERM ngetopt_lvl_sock_protocol(ErlNifEnv*        env,
                                       SocketDescriptor* descP)
{
    ERL_NIF_TERM result;
    int          val;
    SOCKOPTLEN_T valSz = sizeof(val);
    int          res;

    res = sock_getopt(descP->sock, SOL_SOCKET, SO_PROTOCOL,
                      &val, &valSz);

    if (res != 0) {
        result = esock_make_error_errno(env, sock_errno());
    } else {
        switch (val) {
        case IPPROTO_IP:
            result = esock_make_ok2(env, esock_atom_ip);
            break;

        case IPPROTO_TCP:
            result = esock_make_ok2(env, esock_atom_tcp);
            break;

        case IPPROTO_UDP:
            result = esock_make_ok2(env, esock_atom_udp);
            break;

#if defined(HAVE_SCTP)
        case IPPROTO_SCTP:
            result = esock_make_ok2(env, esock_atom_sctp);
            break;
#endif

        default:
            result = esock_make_error(env,
                                      MKT2(env, esock_atom_unknown, MKI(env, val)));
            break;
        }
    }

    return result;
}
#endif


#if defined(SO_RCVBUF)
static
ERL_NIF_TERM ngetopt_lvl_sock_rcvbuf(ErlNifEnv*        env,
                                     SocketDescriptor* descP)
{
    return ngetopt_int_opt(env, descP, SOL_SOCKET, SO_RCVBUF);
}
#endif


#if defined(SO_RCVLOWAT)
static
ERL_NIF_TERM ngetopt_lvl_sock_rcvlowat(ErlNifEnv*        env,
                                       SocketDescriptor* descP)
{
    return ngetopt_int_opt(env, descP, SOL_SOCKET, SO_RCVLOWAT);
}
#endif


#if defined(SO_RCVTIMEO)
static
ERL_NIF_TERM ngetopt_lvl_sock_rcvtimeo(ErlNifEnv*        env,
                                       SocketDescriptor* descP)
{
    return ngetopt_timeval_opt(env, descP, SOL_SOCKET, SO_RCVTIMEO);
}
#endif


#if defined(SO_REUSEADDR)
static
ERL_NIF_TERM ngetopt_lvl_sock_reuseaddr(ErlNifEnv*        env,
                                        SocketDescriptor* descP)
{
    return ngetopt_bool_opt(env, descP, SOL_SOCKET, SO_REUSEADDR);
}
#endif


#if defined(SO_REUSEPORT)
static
ERL_NIF_TERM ngetopt_lvl_sock_reuseport(ErlNifEnv*        env,
                                        SocketDescriptor* descP)
{
    return ngetopt_bool_opt(env, descP, SOL_SOCKET, SO_REUSEPORT);
}
#endif


#if defined(SO_SNDBUF)
static
ERL_NIF_TERM ngetopt_lvl_sock_sndbuf(ErlNifEnv*        env,
                                     SocketDescriptor* descP)
{
    return ngetopt_int_opt(env, descP, SOL_SOCKET, SO_SNDBUF);
}
#endif


#if defined(SO_SNDLOWAT)
static
ERL_NIF_TERM ngetopt_lvl_sock_sndlowat(ErlNifEnv*        env,
                                       SocketDescriptor* descP)
{
    return ngetopt_int_opt(env, descP, SOL_SOCKET, SO_SNDLOWAT);
}
#endif


#if defined(SO_SNDTIMEO)
static
ERL_NIF_TERM ngetopt_lvl_sock_sndtimeo(ErlNifEnv*        env,
                                       SocketDescriptor* descP)
{
    return ngetopt_timeval_opt(env, descP, SOL_SOCKET, SO_SNDTIMEO);
}
#endif


#if defined(SO_TIMESTAMP)
static
ERL_NIF_TERM ngetopt_lvl_sock_timestamp(ErlNifEnv*        env,
                                        SocketDescriptor* descP)
{
    return ngetopt_bool_opt(env, descP, SOL_SOCKET, SO_TIMESTAMP);
}
#endif


#if defined(SO_TYPE)
static
ERL_NIF_TERM ngetopt_lvl_sock_type(ErlNifEnv*        env,
                                     SocketDescriptor* descP)
{
    ERL_NIF_TERM result;
    int          val;
    SOCKOPTLEN_T valSz = sizeof(val);
    int          res;

    res = sock_getopt(descP->sock, SOL_SOCKET, SO_TYPE, &val, &valSz);

    if (res != 0) {
        result = esock_make_error_errno(env, sock_errno());
    } else {
        switch (val) {
        case SOCK_STREAM:
            result = esock_make_ok2(env, esock_atom_stream);
            break;
        case SOCK_DGRAM:
            result = esock_make_ok2(env, esock_atom_dgram);
            break;
#ifdef HAVE_SCTP
        case SOCK_SEQPACKET:
            result = esock_make_ok2(env, esock_atom_seqpacket);
            break;
#endif
        case SOCK_RAW:
            result = esock_make_ok2(env, esock_atom_raw);
            break;
        case SOCK_RDM:
            result = esock_make_ok2(env, esock_atom_rdm);
            break;
        default:
            result = esock_make_error(env,
                                      MKT2(env, esock_atom_unknown, MKI(env, val)));
            break;
        }
    }

    return result;
}
#endif


/* ngetopt_lvl_ip - Level *IP* option(s)
 */
static
ERL_NIF_TERM ngetopt_lvl_ip(ErlNifEnv*        env,
                            SocketDescriptor* descP,
                            int               eOpt)
{
    ERL_NIF_TERM result;

    SSDBG( descP,
           ("SOCKET", "ngetopt_lvl_ip -> entry with"
            "\r\n   eOpt: %d"
            "\r\n", eOpt) );

    switch (eOpt) {
#if defined(IP_FREEBIND)
    case SOCKET_OPT_IP_FREEBIND:
        result = ngetopt_lvl_ip_freebind(env, descP);
        break;
#endif

#if defined(IP_HDRINCL)
    case SOCKET_OPT_IP_HDRINCL:
        result = ngetopt_lvl_ip_hdrincl(env, descP);
        break;
#endif

#if defined(IP_MINTTL)
    case SOCKET_OPT_IP_MINTTL:
        result = ngetopt_lvl_ip_minttl(env, descP);
        break;
#endif

#if defined(IP_MTU)
    case SOCKET_OPT_IP_MTU:
        result = ngetopt_lvl_ip_mtu(env, descP);
        break;
#endif

#if defined(IP_MTU_DISCOVER)
    case SOCKET_OPT_IP_MTU_DISCOVER:
        result = ngetopt_lvl_ip_mtu_discover(env, descP);
        break;
#endif

#if defined(IP_MULTICAST_ALL)
    case SOCKET_OPT_IP_MULTICAST_ALL:
        result = ngetopt_lvl_ip_multicast_all(env, descP);
        break;
#endif

#if defined(IP_MULTICAST_IF)
    case SOCKET_OPT_IP_MULTICAST_IF:
        result = ngetopt_lvl_ip_multicast_if(env, descP);
        break;
#endif

#if defined(IP_MULTICAST_LOOP)
    case SOCKET_OPT_IP_MULTICAST_LOOP:
        result = ngetopt_lvl_ip_multicast_loop(env, descP);
        break;
#endif

#if defined(IP_MULTICAST_TTL)
    case SOCKET_OPT_IP_MULTICAST_TTL:
        result = ngetopt_lvl_ip_multicast_ttl(env, descP);
        break;
#endif

#if defined(IP_NODEFRAG)
    case SOCKET_OPT_IP_NODEFRAG:
        result = ngetopt_lvl_ip_nodefrag(env, descP);
        break;
#endif

#if defined(IP_PKTINFO)
    case SOCKET_OPT_IP_PKTINFO:
        result = ngetopt_lvl_ip_pktinfo(env, descP);
        break;
#endif

#if defined(IP_RECVDSTADDR)
    case SOCKET_OPT_IP_RECVDSTADDR:
        result = ngetopt_lvl_ip_recvdstaddr(env, descP);
        break;
#endif

#if defined(IP_RECVERR)
    case SOCKET_OPT_IP_RECVERR:
        result = ngetopt_lvl_ip_recverr(env, descP);
        break;
#endif

#if defined(IP_RECVIF)
    case SOCKET_OPT_IP_RECVIF:
        result = ngetopt_lvl_ip_recvif(env, descP);
        break;
#endif

#if defined(IP_RECVOPTS)
    case SOCKET_OPT_IP_RECVOPTS:
        result = ngetopt_lvl_ip_recvopts(env, descP);
        break;
#endif

#if defined(IP_RECVORIGDSTADDR)
    case SOCKET_OPT_IP_RECVORIGDSTADDR:
        result = ngetopt_lvl_ip_recvorigdstaddr(env, descP);
        break;
#endif

#if defined(IP_RECVTOS)
    case SOCKET_OPT_IP_RECVTOS:
        result = ngetopt_lvl_ip_recvtos(env, descP);
        break;
#endif

#if defined(IP_RECVTTL)
    case SOCKET_OPT_IP_RECVTTL:
        result = ngetopt_lvl_ip_recvttl(env, descP);
        break;
#endif

#if defined(IP_RETOPTS)
    case SOCKET_OPT_IP_RETOPTS:
        result = ngetopt_lvl_ip_retopts(env, descP);
        break;
#endif

#if defined(IP_ROUTER_ALERT)
    case SOCKET_OPT_IP_ROUTER_ALERT:
        result = ngetopt_lvl_ip_router_alert(env, descP);
        break;
#endif

#if defined(IP_SENDSRCADDR)
    case SOCKET_OPT_IP_SENDSRCADDR:
        result = ngetopt_lvl_ip_sendsrcaddr(env, descP);
        break;
#endif

#if defined(IP_TOS)
    case SOCKET_OPT_IP_TOS:
        result = ngetopt_lvl_ip_tos(env, descP);
        break;
#endif

#if defined(IP_TRANSPARENT)
    case SOCKET_OPT_IP_TRANSPARENT:
        result = ngetopt_lvl_ip_transparent(env, descP);
        break;
#endif

#if defined(IP_TTL)
    case SOCKET_OPT_IP_TTL:
        result = ngetopt_lvl_ip_ttl(env, descP);
        break;
#endif

    default:
        SSDBG( descP, ("SOCKET", "ngetopt_lvl_ip -> unknown opt %d\r\n", eOpt) );
        result = esock_make_error(env, esock_atom_einval);
        break;
    }

    SSDBG( descP,
           ("SOCKET", "ngetopt_lvl_ip -> done when"
            "\r\n   result: %T"
            "\r\n", result) );

    return result;
}


/* ngetopt_lvl_ip_minttl - Level IP MINTTL option
 */
#if defined(IP_MINTTL)
static
ERL_NIF_TERM ngetopt_lvl_ip_minttl(ErlNifEnv*        env,
                                   SocketDescriptor* descP)
{
#if defined(SOL_IP)
    int level = SOL_IP;
#else
    int level = IPPROTO_IP;
#endif

    return ngetopt_int_opt(env, descP, level, IP_MINTTL);
}
#endif


/* ngetopt_lvl_ip_freebind - Level IP FREEBIND option
 */
#if defined(IP_FREEBIND)
static
ERL_NIF_TERM ngetopt_lvl_ip_freebind(ErlNifEnv*        env,
                                     SocketDescriptor* descP)
{
#if defined(SOL_IP)
    int level = SOL_IP;
#else
    int level = IPPROTO_IP;
#endif

    return ngetopt_bool_opt(env, descP, level, IP_FREEBIND);
}
#endif


/* ngetopt_lvl_ip_hdrincl - Level IP HDRINCL option
 */
#if defined(IP_HDRINCL)
static
ERL_NIF_TERM ngetopt_lvl_ip_hdrincl(ErlNifEnv*        env,
                                    SocketDescriptor* descP)
{
#if defined(SOL_IP)
    int level = SOL_IP;
#else
    int level = IPPROTO_IP;
#endif

    return ngetopt_bool_opt(env, descP, level, IP_HDRINCL);
}
#endif


/* ngetopt_lvl_ip_mtu - Level IP MTU option
 */
#if defined(IP_MTU)
static
ERL_NIF_TERM ngetopt_lvl_ip_mtu(ErlNifEnv*        env,
                                SocketDescriptor* descP)
{
#if defined(SOL_IP)
    int level = SOL_IP;
#else
    int level = IPPROTO_IP;
#endif

    return ngetopt_int_opt(env, descP, level, IP_MTU);
}
#endif


/* ngetopt_lvl_ip_mtu_discover - Level IP MTU_DISCOVER option
 */
#if defined(IP_MTU_DISCOVER)
static
ERL_NIF_TERM ngetopt_lvl_ip_mtu_discover(ErlNifEnv*        env,
                                         SocketDescriptor* descP)
{
    ERL_NIF_TERM   result;
    ERL_NIF_TERM   eMtuDisc;
    int            mtuDisc;
    SOCKOPTLEN_T   mtuDiscSz = sizeof(mtuDisc);
    int            res;
#if defined(SOL_IP)
    int level = SOL_IP;
#else
    int level = IPPROTO_IP;
#endif

    res = sock_getopt(descP->sock, level, IP_MTU_DISCOVER,
                      &mtuDisc, &mtuDiscSz);

    if (res != 0) {
        result = esock_make_error_errno(env, sock_errno());
    } else {
        encode_ip_pmtudisc(env, mtuDisc, &eMtuDisc);
        result = esock_make_ok2(env, eMtuDisc);
    }

    return result;

}
#endif


/* ngetopt_lvl_ip_multicast_all - Level IP MULTICAST_ALL option
 */
#if defined(IP_MULTICAST_ALL)
static
ERL_NIF_TERM ngetopt_lvl_ip_multicast_all(ErlNifEnv*        env,
                                          SocketDescriptor* descP)
{
#if defined(SOL_IP)
    int level = SOL_IP;
#else
    int level = IPPROTO_IP;
#endif

    return ngetopt_bool_opt(env, descP, level, IP_MULTICAST_ALL);
}
#endif


/* ngetopt_lvl_ip_multicast_if - Level IP MULTICAST_IF option
 */
#if defined(IP_MULTICAST_IF)
static
ERL_NIF_TERM ngetopt_lvl_ip_multicast_if(ErlNifEnv*        env,
                                         SocketDescriptor* descP)
{
    ERL_NIF_TERM   result;
    ERL_NIF_TERM   eAddr;
    struct in_addr ifAddr;
    SOCKOPTLEN_T   ifAddrSz = sizeof(ifAddr);
    char*          xres;
    int            res;
#if defined(SOL_IP)
    int level = SOL_IP;
#else
    int level = IPPROTO_IP;
#endif

    res = sock_getopt(descP->sock, level, IP_MULTICAST_IF, &ifAddr, &ifAddrSz);

    if (res != 0) {
        result = esock_make_error_errno(env, sock_errno());
    } else {
        if ((xres = esock_encode_ip4_address(env, &ifAddr, &eAddr)) != NULL) {
            result = esock_make_error_str(env, xres);
        } else {
            result = esock_make_ok2(env, eAddr);
        }
    }

    return result;

}
#endif


/* ngetopt_lvl_ip_multicast_loop - Level IP MULTICAST_LOOP option
 */
#if defined(IP_MULTICAST_LOOP)
static
ERL_NIF_TERM ngetopt_lvl_ip_multicast_loop(ErlNifEnv*        env,
                                           SocketDescriptor* descP)
{
#if defined(SOL_IP)
    int level = SOL_IP;
#else
    int level = IPPROTO_IP;
#endif

    return ngetopt_bool_opt(env, descP, level, IP_MULTICAST_LOOP);
}
#endif


/* ngetopt_lvl_ip_multicast_ttl - Level IP MULTICAST_TTL option
 */
#if defined(IP_MULTICAST_TTL)
static
ERL_NIF_TERM ngetopt_lvl_ip_multicast_ttl(ErlNifEnv*        env,
                                          SocketDescriptor* descP)
{
#if defined(SOL_IP)
    int level = SOL_IP;
#else
    int level = IPPROTO_IP;
#endif

    return ngetopt_int_opt(env, descP, level, IP_MULTICAST_TTL);
}
#endif


/* ngetopt_lvl_ip_nodefrag - Level IP NODEFRAG option
 */
#if defined(IP_NODEFRAG)
static
ERL_NIF_TERM ngetopt_lvl_ip_nodefrag(ErlNifEnv*        env,
                                     SocketDescriptor* descP)
{
#if defined(SOL_IP)
    int level = SOL_IP;
#else
    int level = IPPROTO_IP;
#endif

    return ngetopt_bool_opt(env, descP, level, IP_NODEFRAG);
}
#endif


/* ngetopt_lvl_ip_pktinfo - Level IP PKTINFO option
 */
#if defined(IP_PKTINFO)
static
ERL_NIF_TERM ngetopt_lvl_ip_pktinfo(ErlNifEnv*        env,
                                    SocketDescriptor* descP)
{
#if defined(SOL_IP)
    int level = SOL_IP;
#else
    int level = IPPROTO_IP;
#endif

    return ngetopt_bool_opt(env, descP, level, IP_PKTINFO);
}
#endif


/* ngetopt_lvl_ip_recvtos - Level IP RECVTOS option
 */
#if defined(IP_RECVTOS)
static
ERL_NIF_TERM ngetopt_lvl_ip_recvtos(ErlNifEnv*        env,
                                    SocketDescriptor* descP)
{
#if defined(SOL_IP)
    int level = SOL_IP;
#else
    int level = IPPROTO_IP;
#endif

    return ngetopt_bool_opt(env, descP, level, IP_RECVTOS);
}
#endif


/* ngetopt_lvl_ip_recvdstaddr - Level IP RECVDSTADDR option
 */
#if defined(IP_RECVDSTADDR)
static
ERL_NIF_TERM ngetopt_lvl_ip_recvdstaddr(ErlNifEnv*        env,
                                        SocketDescriptor* descP)
{
#if defined(SOL_IP)
    int level = SOL_IP;
#else
    int level = IPPROTO_IP;
#endif

    return ngetopt_bool_opt(env, descP, level, IP_RECVDSTADDR);
}
#endif


/* ngetopt_lvl_ip_recverr - Level IP RECVERR option
 */
#if defined(IP_RECVERR)
static
ERL_NIF_TERM ngetopt_lvl_ip_recverr(ErlNifEnv*        env,
                                    SocketDescriptor* descP)
{
#if defined(SOL_IP)
    int level = SOL_IP;
#else
    int level = IPPROTO_IP;
#endif

    return ngetopt_bool_opt(env, descP, level, IP_RECVERR);
}
#endif


/* ngetopt_lvl_ip_recvif - Level IP RECVIF option
 */
#if defined(IP_RECVIF)
static
ERL_NIF_TERM ngetopt_lvl_ip_recvif(ErlNifEnv*        env,
                                   SocketDescriptor* descP)
{
#if defined(SOL_IP)
    int level = SOL_IP;
#else
    int level = IPPROTO_IP;
#endif

    return ngetopt_bool_opt(env, descP, level, IP_RECVIF);
}
#endif


/* ngetopt_lvl_ip_recvopt - Level IP RECVOPTS option
 */
#if defined(IP_RECVOPTS)
static
ERL_NIF_TERM ngetopt_lvl_ip_recvopts(ErlNifEnv*        env,
                                     SocketDescriptor* descP)
{
#if defined(SOL_IP)
    int level = SOL_IP;
#else
    int level = IPPROTO_IP;
#endif

    return ngetopt_bool_opt(env, descP, level, IP_RECVOPTS);
}
#endif


/* ngetopt_lvl_ip_recvorigdstaddr - Level IP RECVORIGDSTADDR option
 */
#if defined(IP_RECVORIGDSTADDR)
static
ERL_NIF_TERM ngetopt_lvl_ip_recvorigdstaddr(ErlNifEnv*        env,
                                            SocketDescriptor* descP)
{
#if defined(SOL_IP)
    int level = SOL_IP;
#else
    int level = IPPROTO_IP;
#endif

    return ngetopt_bool_opt(env, descP, level, IP_RECVORIGDSTADDR);
}
#endif


/* ngetopt_lvl_ip_recvttl - Level IP RECVTTL option
 */
#if defined(IP_RECVTTL)
static
ERL_NIF_TERM ngetopt_lvl_ip_recvttl(ErlNifEnv*        env,
                                    SocketDescriptor* descP)
{
#if defined(SOL_IP)
    int level = SOL_IP;
#else
    int level = IPPROTO_IP;
#endif

    return ngetopt_bool_opt(env, descP, level, IP_RECVTTL);
}
#endif


/* ngetopt_lvl_ip_retopts - Level IP RETOPTS option
 */
#if defined(IP_RETOPTS)
static
ERL_NIF_TERM ngetopt_lvl_ip_retopts(ErlNifEnv*        env,
                                    SocketDescriptor* descP)
{
#if defined(SOL_IP)
    int level = SOL_IP;
#else
    int level = IPPROTO_IP;
#endif

    return ngetopt_bool_opt(env, descP, level, IP_RETOPTS);
}
#endif


/* ngetopt_lvl_ip_router_alert - Level IP ROUTER_ALERT option
 */
#if defined(IP_ROUTER_ALERT)
static
ERL_NIF_TERM ngetopt_lvl_ip_router_alert(ErlNifEnv*        env,
                                         SocketDescriptor* descP)
{
#if defined(SOL_IP)
    int level = SOL_IP;
#else
    int level = IPPROTO_IP;
#endif

    return ngetopt_int_opt(env, descP, level, IP_ROUTER_ALERT);
}
#endif


/* ngetopt_lvl_ip_sendsrcaddr - Level IP SENDSRCADDR option
 */
#if defined(IP_SENDSRCADDR)
static
ERL_NIF_TERM ngetopt_lvl_ip_sendsrcaddr(ErlNifEnv*        env,
                                        SocketDescriptor* descP)
{
#if defined(SOL_IP)
    int level = SOL_IP;
#else
    int level = IPPROTO_IP;
#endif

    return ngetopt_bool_opt(env, descP, level, IP_SENDSRCADDR);
}
#endif


/* ngetopt_lvl_ip_tos - Level IP TOS option
 */
#if defined(IP_TOS)
static
ERL_NIF_TERM ngetopt_lvl_ip_tos(ErlNifEnv*        env,
                                SocketDescriptor* descP)
{
#if defined(SOL_IP)
    int          level = SOL_IP;
#else
    int          level = IPPROTO_IP;
#endif
    ERL_NIF_TERM result;
    int          val;
    SOCKOPTLEN_T valSz = sizeof(val);
    int          res;

    res = sock_getopt(descP->sock, level, IP_TOS, &val, &valSz);

    if (res != 0) {
        result = esock_make_error_errno(env, sock_errno());
    } else {
        result = encode_ip_tos(env, val);
    }

    return result;
}
#endif


/* ngetopt_lvl_ip_transparent - Level IP TRANSPARENT option
 */
#if defined(IP_TRANSPARENT)
static
ERL_NIF_TERM ngetopt_lvl_ip_transparent(ErlNifEnv*        env,
                                        SocketDescriptor* descP)
{
#if defined(SOL_IP)
    int level = SOL_IP;
#else
    int level = IPPROTO_IP;
#endif

    return ngetopt_bool_opt(env, descP, level, IP_TRANSPARENT);
}
#endif



/* ngetopt_lvl_ip_ttl - Level IP TTL option
 */
#if defined(IP_TTL)
static
ERL_NIF_TERM ngetopt_lvl_ip_ttl(ErlNifEnv*        env,
                                SocketDescriptor* descP)
{
#if defined(SOL_IP)
    int level = SOL_IP;
#else
    int level = IPPROTO_IP;
#endif

    return ngetopt_int_opt(env, descP, level, IP_TTL);
}
#endif



/* ngetopt_lvl_ipv6 - Level *IPv6* option(s)
 */
#if defined(SOL_IPV6)
static
ERL_NIF_TERM ngetopt_lvl_ipv6(ErlNifEnv*        env,
                              SocketDescriptor* descP,
                              int               eOpt)
{
    ERL_NIF_TERM result;

    SSDBG( descP,
           ("SOCKET", "ngetopt_lvl_ipv6 -> entry with"
            "\r\n   eOpt: %d"
            "\r\n", eOpt) );

    switch (eOpt) {
#if defined(IPV6_AUTHHDR)
    case SOCKET_OPT_IPV6_AUTHHDR:
        result = ngetopt_lvl_ipv6_authhdr(env, descP);
        break;
#endif

#if defined(IPV6_DSTOPTS)
    case SOCKET_OPT_IPV6_DSTOPTS:
        result = ngetopt_lvl_ipv6_dstopts(env, descP);
        break;
#endif

#if defined(IPV6_FLOWINFO)
    case SOCKET_OPT_IPV6_FLOWINFO:
        result = ngetopt_lvl_ipv6_flowinfo(env, descP);
        break;
#endif

#if defined(IPV6_HOPLIMIT)
    case SOCKET_OPT_IPV6_HOPLIMIT:
        result = ngetopt_lvl_ipv6_hoplimit(env, descP);
        break;
#endif

#if defined(IPV6_HOPOPTS)
    case SOCKET_OPT_IPV6_HOPOPTS:
        result = ngetopt_lvl_ipv6_hopopts(env, descP);
        break;
#endif

#if defined(IPV6_MTU)
    case SOCKET_OPT_IPV6_MTU:
        result = ngetopt_lvl_ipv6_mtu(env, descP);
        break;
#endif

#if defined(IPV6_MTU_DISCOVER)
    case SOCKET_OPT_IPV6_MTU_DISCOVER:
        result = ngetopt_lvl_ipv6_mtu_discover(env, descP);
        break;
#endif

#if defined(IPV6_MULTICAST_HOPS)
    case SOCKET_OPT_IPV6_MULTICAST_HOPS:
        result = ngetopt_lvl_ipv6_multicast_hops(env, descP);
        break;
#endif

#if defined(IPV6_MULTICAST_IF)
    case SOCKET_OPT_IPV6_MULTICAST_IF:
        result = ngetopt_lvl_ipv6_multicast_if(env, descP);
        break;
#endif

#if defined(IPV6_MULTICAST_LOOP)
    case SOCKET_OPT_IPV6_MULTICAST_LOOP:
        result = ngetopt_lvl_ipv6_multicast_loop(env, descP);
        break;
#endif

#if defined(IPV6_RECVERR)
    case SOCKET_OPT_IPV6_RECVERR:
        result = ngetopt_lvl_ipv6_recverr(env, descP);
        break;
#endif

#if defined(IPV6_RECVPKTINFO) || defined(IPV6_PKTINFO)
    case SOCKET_OPT_IPV6_RECVPKTINFO:
        result = ngetopt_lvl_ipv6_recvpktinfo(env, descP);
        break;
#endif

#if defined(IPV6_ROUTER_ALERT)
    case SOCKET_OPT_IPV6_ROUTER_ALERT:
        result = ngetopt_lvl_ipv6_router_alert(env, descP);
        break;
#endif

#if defined(IPV6_RTHDR)
    case SOCKET_OPT_IPV6_RTHDR:
        result = ngetopt_lvl_ipv6_rthdr(env, descP);
        break;
#endif

#if defined(IPV6_UNICAST_HOPS)
    case SOCKET_OPT_IPV6_UNICAST_HOPS:
        result = ngetopt_lvl_ipv6_unicast_hops(env, descP);
        break;
#endif

#if defined(IPV6_V6ONLY)
    case SOCKET_OPT_IPV6_V6ONLY:
        result = ngetopt_lvl_ipv6_v6only(env, descP);
        break;
#endif

    default:
        result = esock_make_error(env, esock_atom_einval);
        break;
    }

    SSDBG( descP,
           ("SOCKET", "ngetopt_lvl_ipv6 -> done when"
            "\r\n   result: %T"
            "\r\n", result) );

    return result;
}


#if defined(IPV6_AUTHHDR)
static
ERL_NIF_TERM ngetopt_lvl_ipv6_authhdr(ErlNifEnv*        env,
                                      SocketDescriptor* descP)
{
    return ngetopt_bool_opt(env, descP, SOL_IPV6, IPV6_AUTHHDR);
}
#endif


#if defined(IPV6_DSTOPTS)
static
ERL_NIF_TERM ngetopt_lvl_ipv6_dstopts(ErlNifEnv*        env,
                                      SocketDescriptor* descP)
{
    return ngetopt_bool_opt(env, descP, SOL_IPV6, IPV6_DSTOPTS);
}
#endif


#if defined(IPV6_FLOWINFO)
static
ERL_NIF_TERM ngetopt_lvl_ipv6_flowinfo(ErlNifEnv*        env,
                                       SocketDescriptor* descP)
{
    return ngetopt_bool_opt(env, descP, SOL_IPV6, IPV6_FLOWINFO);
}
#endif


#if defined(IPV6_HOPLIMIT)
static
ERL_NIF_TERM ngetopt_lvl_ipv6_hoplimit(ErlNifEnv*        env,
                                       SocketDescriptor* descP)
{
    return ngetopt_bool_opt(env, descP, SOL_IPV6, IPV6_HOPLIMIT);
}
#endif


#if defined(IPV6_HOPOPTS)
static
ERL_NIF_TERM ngetopt_lvl_ipv6_hopopts(ErlNifEnv*        env,
                                      SocketDescriptor* descP)
{
    return ngetopt_bool_opt(env, descP, SOL_IPV6, IPV6_HOPOPTS);
}
#endif


#if defined(IPV6_MTU)
static
ERL_NIF_TERM ngetopt_lvl_ipv6_mtu(ErlNifEnv*        env,
                                  SocketDescriptor* descP)
{
    return ngetopt_int_opt(env, descP, SOL_IPV6, IPV6_MTU);
}
#endif


/* ngetopt_lvl_ipv6_mtu_discover - Level IPv6 MTU_DISCOVER option
 */
#if defined(IPV6_MTU_DISCOVER)
static
ERL_NIF_TERM ngetopt_lvl_ipv6_mtu_discover(ErlNifEnv*        env,
                                           SocketDescriptor* descP)
{
    ERL_NIF_TERM  result;
    ERL_NIF_TERM  eMtuDisc;
    int           mtuDisc;
    SOCKOPTLEN_T  mtuDiscSz = sizeof(mtuDisc);
    int           res;

    res = sock_getopt(descP->sock, SOL_IPV6, IPV6_MTU_DISCOVER,
                      &mtuDisc, &mtuDiscSz);

    if (res != 0) {
        result = esock_make_error_errno(env, sock_errno());
    } else {
        encode_ipv6_pmtudisc(env, mtuDisc, &eMtuDisc);
        result = esock_make_ok2(env, eMtuDisc);
    }

    return result;

}
#endif


#if defined(IPV6_MULTICAST_HOPS)
static
ERL_NIF_TERM ngetopt_lvl_ipv6_multicast_hops(ErlNifEnv*        env,
                                             SocketDescriptor* descP)
{
    return ngetopt_int_opt(env, descP, SOL_IPV6, IPV6_MULTICAST_HOPS);
}
#endif


#if defined(IPV6_MULTICAST_IF)
static
ERL_NIF_TERM ngetopt_lvl_ipv6_multicast_if(ErlNifEnv*        env,
                                           SocketDescriptor* descP)
{
    return ngetopt_int_opt(env, descP, SOL_IPV6, IPV6_MULTICAST_IF);
}
#endif


#if defined(IPV6_MULTICAST_LOOP)
static
ERL_NIF_TERM ngetopt_lvl_ipv6_multicast_loop(ErlNifEnv*        env,
                                             SocketDescriptor* descP)
{
    return ngetopt_bool_opt(env, descP, SOL_IPV6, IPV6_MULTICAST_LOOP);
}
#endif


#if defined(IPV6_RECVERR)
static
ERL_NIF_TERM ngetopt_lvl_ipv6_recverr(ErlNifEnv*        env,
                                      SocketDescriptor* descP)
{
    return ngetopt_bool_opt(env, descP, SOL_IPV6, IPV6_RECVERR);
}
#endif


#if defined(IPV6_RECVPKTINFO) || defined(IPV6_PKTINFO)
static
ERL_NIF_TERM ngetopt_lvl_ipv6_recvpktinfo(ErlNifEnv*        env,
                                          SocketDescriptor* descP)
{
#if defined(IPV6_RECVPKTINFO)
    int opt = IPV6_RECVPKTINFO;
#else
    int opt = IPV6_PKTINFO;
#endif

    return ngetopt_bool_opt(env, descP, SOL_IPV6, opt);
}
#endif


#if defined(IPV6_ROUTER_ALERT)
static
ERL_NIF_TERM ngetopt_lvl_ipv6_router_alert(ErlNifEnv*        env,
                                           SocketDescriptor* descP)
{
    return ngetopt_int_opt(env, descP, SOL_IPV6, IPV6_ROUTER_ALERT);
}
#endif


#if defined(IPV6_RTHDR)
static
ERL_NIF_TERM ngetopt_lvl_ipv6_rthdr(ErlNifEnv*        env,
                                    SocketDescriptor* descP)
{
    return ngetopt_bool_opt(env, descP, SOL_IPV6, IPV6_RTHDR);
}
#endif


#if defined(IPV6_UNICAST_HOPS)
static
ERL_NIF_TERM ngetopt_lvl_ipv6_unicast_hops(ErlNifEnv*        env,
                                           SocketDescriptor* descP)
{
    return ngetopt_int_opt(env, descP, SOL_IPV6, IPV6_UNICAST_HOPS);
}
#endif


#if defined(IPV6_V6ONLY)
static
ERL_NIF_TERM ngetopt_lvl_ipv6_v6only(ErlNifEnv*        env,
                                     SocketDescriptor* descP)
{
    return ngetopt_bool_opt(env, descP, SOL_IPV6, IPV6_V6ONLY);
}
#endif


#endif // defined(SOL_IPV6)



/* ngetopt_lvl_tcp - Level *TCP* option(s)
 */
static
ERL_NIF_TERM ngetopt_lvl_tcp(ErlNifEnv*        env,
                             SocketDescriptor* descP,
                             int               eOpt)
{
    ERL_NIF_TERM result;

    switch (eOpt) {
#if defined(TCP_CONGESTION)
    case SOCKET_OPT_TCP_CONGESTION:
        result = ngetopt_lvl_tcp_congestion(env, descP);
        break;
#endif

#if defined(TCP_MAXSEG)
    case SOCKET_OPT_TCP_MAXSEG:
        result = ngetopt_lvl_tcp_maxseg(env, descP);
        break;
#endif

#if defined(TCP_NODELAY)
    case SOCKET_OPT_TCP_NODELAY:
        result = ngetopt_lvl_tcp_nodelay(env, descP);
        break;
#endif

    default:
        result = esock_make_error(env, esock_atom_einval);
        break;
    }

    return result;
}


/* ngetopt_lvl_tcp_congestion - Level TCP CONGESTION option
 */
#if defined(TCP_CONGESTION)
static
ERL_NIF_TERM ngetopt_lvl_tcp_congestion(ErlNifEnv*        env,
                                        SocketDescriptor* descP)
{
    int max = SOCKET_OPT_TCP_CONGESTION_NAME_MAX+1;

    return ngetopt_str_opt(env, descP, IPPROTO_TCP, TCP_CONGESTION, max);
}
#endif


/* ngetopt_lvl_tcp_maxseg - Level TCP MAXSEG option
 */
#if defined(TCP_MAXSEG)
static
ERL_NIF_TERM ngetopt_lvl_tcp_maxseg(ErlNifEnv*        env,
                                    SocketDescriptor* descP)
{
    return ngetopt_int_opt(env, descP, IPPROTO_TCP, TCP_MAXSEG);
}
#endif


/* ngetopt_lvl_tcp_nodelay - Level TCP NODELAY option
 */
#if defined(TCP_NODELAY)
static
ERL_NIF_TERM ngetopt_lvl_tcp_nodelay(ErlNifEnv*        env,
                                     SocketDescriptor* descP)
{
    return ngetopt_bool_opt(env, descP, IPPROTO_TCP, TCP_NODELAY);
}
#endif



/* ngetopt_lvl_udp - Level *UDP* option(s)
 */
static
ERL_NIF_TERM ngetopt_lvl_udp(ErlNifEnv*        env,
                             SocketDescriptor* descP,
                             int               eOpt)
{
    ERL_NIF_TERM result;

    switch (eOpt) {
#if defined(UDP_CORK)
    case SOCKET_OPT_UDP_CORK:
        result = ngetopt_lvl_udp_cork(env, descP);
        break;
#endif

    default:
        result = esock_make_error(env, esock_atom_einval);
        break;
    }

    return result;
}


/* ngetopt_lvl_udp_cork - Level UDP CORK option
 */
#if defined(UDP_CORK)
static
ERL_NIF_TERM ngetopt_lvl_udp_cork(ErlNifEnv*        env,
                                  SocketDescriptor* descP)
{
    return ngetopt_bool_opt(env, descP, IPPROTO_UDP, UDP_CORK);
}
#endif



/* ngetopt_lvl_sctp - Level *SCTP* option(s)
 */
#if defined(HAVE_SCTP)
static
ERL_NIF_TERM ngetopt_lvl_sctp(ErlNifEnv*        env,
                              SocketDescriptor* descP,
                              int               eOpt)
{
    ERL_NIF_TERM result;

    SSDBG( descP,
           ("SOCKET", "ngetopt_lvl_sctp -> entry with"
            "\r\n   opt: %d"
            "\r\n", eOpt) );

    switch (eOpt) {
#if defined(SCTP_ASSOCINFO)
    case SOCKET_OPT_SCTP_ASSOCINFO:
        result = ngetopt_lvl_sctp_associnfo(env, descP);
        break;
#endif

#if defined(SCTP_AUTOCLOSE)
    case SOCKET_OPT_SCTP_AUTOCLOSE:
        result = ngetopt_lvl_sctp_autoclose(env, descP);
        break;
#endif

#if defined(SCTP_DISABLE_FRAGMENTS)
    case SOCKET_OPT_SCTP_DISABLE_FRAGMENTS:
        result = ngetopt_lvl_sctp_disable_fragments(env, descP);
        break;
#endif

#if defined(SCTP_INITMSG)
    case SOCKET_OPT_SCTP_INITMSG:
        result = ngetopt_lvl_sctp_initmsg(env, descP);
        break;
#endif

#if defined(SCTP_MAXSEG)
    case SOCKET_OPT_SCTP_MAXSEG:
        result = ngetopt_lvl_sctp_maxseg(env, descP);
        break;
#endif

#if defined(SCTP_NODELAY)
    case SOCKET_OPT_SCTP_NODELAY:
        result = ngetopt_lvl_sctp_nodelay(env, descP);
        break;
#endif

#if defined(SCTP_RTOINFO)
    case SOCKET_OPT_SCTP_RTOINFO:
        result = ngetopt_lvl_sctp_rtoinfo(env, descP);
        break;
#endif

    default:
        result = esock_make_error(env, esock_atom_einval);
        break;
    }

    SSDBG( descP,
           ("SOCKET", "ngetopt_lvl_sctp -> done when"
            "\r\n   result: %T"
            "\r\n", result) );

    return result;
}


/* ngetopt_lvl_sctp_associnfo - Level SCTP ASSOCINFO option
 *
 * <KOLLA>
 *
 * We should really specify which association this relates to,
 * as it is now we get assoc-id = 0. If this socket is an 
 * association (and not an endpoint) then it will have an
 * assoc id. But since the sctp support at present is "limited",
 * we leave it for now.
 * What do we do if this is an endpoint? Invalid op?
 *
 * </KOLLA>
 */
#if defined(SCTP_ASSOCINFO)
static
ERL_NIF_TERM ngetopt_lvl_sctp_associnfo(ErlNifEnv*        env,
                                        SocketDescriptor* descP)
{
    ERL_NIF_TERM            result;
    struct sctp_assocparams val;
    SOCKOPTLEN_T            valSz = sizeof(val);
    int                     res;

    SSDBG( descP, ("SOCKET", "ngetopt_lvl_sctp_associnfo -> entry\r\n") );
    
    sys_memzero((char*) &val, valSz);
    res = sock_getopt(descP->sock, IPPROTO_SCTP, SCTP_ASSOCINFO, &val, &valSz);

    if (res != 0) {
        result = esock_make_error_errno(env, sock_errno());
    } else {
        ERL_NIF_TERM eAssocParams;
        ERL_NIF_TERM keys[]  = {atom_assoc_id, atom_max_rxt, atom_num_peer_dests,
                                atom_peer_rwnd, atom_local_rwnd, atom_cookie_life};
        ERL_NIF_TERM vals[]  = {MKUI(env, val.sasoc_assoc_id),
                                MKUI(env, val.sasoc_asocmaxrxt),
                                MKUI(env, val.sasoc_number_peer_destinations),
                                MKUI(env, val.sasoc_peer_rwnd),
                                MKUI(env, val.sasoc_local_rwnd),
                                MKUI(env, val.sasoc_cookie_life)};
        unsigned int numKeys = sizeof(keys) / sizeof(ERL_NIF_TERM);
        unsigned int numVals = sizeof(vals) / sizeof(ERL_NIF_TERM);

        ESOCK_ASSERT( (numKeys == numVals) );

        if (!MKMA(env, keys, vals, numKeys, &eAssocParams))
            return esock_make_error(env, esock_atom_einval);;
    
        result = esock_make_ok2(env, eAssocParams);
    }

    SSDBG( descP,
           ("SOCKET", "ngetopt_lvl_sctp_associnfo -> done with"
            "\r\n   res:    %d"
            "\r\n   result: %T"
            "\r\n", res, result) );

    return result;
}
#endif


/* ngetopt_lvl_sctp_autoclose - Level SCTP AUTOCLOSE option
 */
#if defined(SCTP_AUTOCLOSE)
static
ERL_NIF_TERM ngetopt_lvl_sctp_autoclose(ErlNifEnv*        env,
                                        SocketDescriptor* descP)
{
    return ngetopt_int_opt(env, descP, IPPROTO_SCTP, SCTP_AUTOCLOSE);
}
#endif


/* ngetopt_lvl_sctp_disable_fragments - Level SCTP DISABLE:FRAGMENTS option
 */
#if defined(SCTP_DISABLE_FRAGMENTS)
static
ERL_NIF_TERM ngetopt_lvl_sctp_disable_fragments(ErlNifEnv*        env,
                                                SocketDescriptor* descP)
{
    return ngetopt_bool_opt(env, descP, IPPROTO_SCTP, SCTP_DISABLE_FRAGMENTS);
}
#endif


/* ngetopt_lvl_sctp_initmsg - Level SCTP INITMSG option
 *
 */
#if defined(SCTP_INITMSG)
static
ERL_NIF_TERM ngetopt_lvl_sctp_initmsg(ErlNifEnv*        env,
                                      SocketDescriptor* descP)
{
    ERL_NIF_TERM        result;
    struct sctp_initmsg val;
    SOCKOPTLEN_T        valSz = sizeof(val);
    int                 res;

    SSDBG( descP, ("SOCKET", "ngetopt_lvl_sctp_initmsg -> entry\r\n") );
    
    sys_memzero((char*) &val, valSz);
    res = sock_getopt(descP->sock, IPPROTO_SCTP, SCTP_INITMSG, &val, &valSz);

    if (res != 0) {
        result = esock_make_error_errno(env, sock_errno());
    } else {
        ERL_NIF_TERM eInitMsg;
        ERL_NIF_TERM keys[]  = {atom_num_outstreams, atom_max_instreams,
                                atom_max_attempts, atom_max_init_timeo};
        ERL_NIF_TERM vals[]  = {MKUI(env, val.sinit_num_ostreams),
                                MKUI(env, val.sinit_max_instreams),
                                MKUI(env, val.sinit_max_attempts),
                                MKUI(env, val.sinit_max_init_timeo)};
        unsigned int numKeys = sizeof(keys) / sizeof(ERL_NIF_TERM);
        unsigned int numVals = sizeof(vals) / sizeof(ERL_NIF_TERM);

        ESOCK_ASSERT( (numKeys == numVals) );

        if (!MKMA(env, keys, vals, numKeys, &eInitMsg))
            return esock_make_error(env, esock_atom_einval);;
    
        result = esock_make_ok2(env, eInitMsg);
    }

    SSDBG( descP,
           ("SOCKET", "ngetopt_lvl_sctp_initmsg -> done with"
            "\r\n   res:    %d"
            "\r\n   result: %T"
            "\r\n", res, result) );

    return result;
}
#endif


/* ngetopt_lvl_sctp_maxseg - Level SCTP MAXSEG option
 */
#if defined(SCTP_MAXSEG)
static
ERL_NIF_TERM ngetopt_lvl_sctp_maxseg(ErlNifEnv*        env,
                                     SocketDescriptor* descP)
{
    return ngetopt_int_opt(env, descP, IPPROTO_SCTP, SCTP_MAXSEG);
}
#endif


/* ngetopt_lvl_sctp_nodelay - Level SCTP NODELAY option
 */
#if defined(SCTP_NODELAY)
static
ERL_NIF_TERM ngetopt_lvl_sctp_nodelay(ErlNifEnv*        env,
                                      SocketDescriptor* descP)
{
    return ngetopt_bool_opt(env, descP, IPPROTO_SCTP, SCTP_NODELAY);
}
#endif


/* ngetopt_lvl_sctp_associnfo - Level SCTP ASSOCINFO option
 *
 * <KOLLA>
 *
 * We should really specify which association this relates to,
 * as it is now we get assoc-id = 0. If this socket is an 
 * association (and not an endpoint) then it will have an
 * assoc id (we can assume). But since the sctp support at 
 * present is "limited", we leave it for now.
 * What do we do if this is an endpoint? Invalid op?
 *
 * </KOLLA>
 */
#if defined(SCTP_RTOINFO)
static
ERL_NIF_TERM ngetopt_lvl_sctp_rtoinfo(ErlNifEnv*        env,
                                      SocketDescriptor* descP)
{
    ERL_NIF_TERM        result;
    struct sctp_rtoinfo val;
    SOCKOPTLEN_T        valSz = sizeof(val);
    int                 res;

    SSDBG( descP, ("SOCKET", "ngetopt_lvl_sctp_rtoinfo -> entry\r\n") );
    
    sys_memzero((char*) &val, valSz);
    res = sock_getopt(descP->sock, IPPROTO_SCTP, SCTP_RTOINFO, &val, &valSz);

    if (res != 0) {
        result = esock_make_error_errno(env, sock_errno());
    } else {
        ERL_NIF_TERM eRTOInfo;        
        ERL_NIF_TERM keys[]  = {atom_assoc_id, atom_initial, atom_max, atom_min};
        ERL_NIF_TERM vals[]  = {MKUI(env, val.srto_assoc_id),
                                MKUI(env, val.srto_initial),
                                MKUI(env, val.srto_max),
                                MKUI(env, val.srto_min)};
        unsigned int numKeys = sizeof(keys) / sizeof(ERL_NIF_TERM);
        unsigned int numVals = sizeof(vals) / sizeof(ERL_NIF_TERM);

        ESOCK_ASSERT( (numKeys == numVals) );

        if (!MKMA(env, keys, vals, numKeys, &eRTOInfo))
            return esock_make_error(env, esock_atom_einval);;
    
        result = esock_make_ok2(env, eRTOInfo);
    }

    SSDBG( descP,
           ("SOCKET", "ngetopt_lvl_sctp_rtoinfo -> done with"
            "\r\n   res:    %d"
            "\r\n   result: %T"
            "\r\n", res, result) );

    return result;
}
#endif



#endif // defined(HAVE_SCTP)



/* ngetopt_bool_opt - get an (integer) bool option
 */
static
ERL_NIF_TERM ngetopt_bool_opt(ErlNifEnv*        env,
                              SocketDescriptor* descP,
                              int               level,
                              int               opt)
{
    ERL_NIF_TERM result;
    int          val;
    SOCKOPTLEN_T valSz = sizeof(val);
    int          res;

    /*
    SSDBG( descP, ("SOCKET", "ngetopt_bool_opt -> entry with"
                   "\r\n: level: %d"
                   "\r\n: opt:   %d"
                   "\r\n", level, opt) );
    */

    res = sock_getopt(descP->sock, level, opt, &val, &valSz);

    if (res != 0) {
        result = esock_make_error_errno(env, sock_errno());
    } else {
        ERL_NIF_TERM bval = ((val) ? atom_true : atom_false);

        result = esock_make_ok2(env, bval);
    }

    /*
    SSDBG( descP, ("SOCKET", "ngetopt_bool_opt -> done when"
                   "\r\n: res:    %d"
                   "\r\n: result: %T"
                   "\r\n", res, result) );
    */

    return result;
}


/* ngetopt_int_opt - get an integer option
 */
static
ERL_NIF_TERM ngetopt_int_opt(ErlNifEnv*        env,
                             SocketDescriptor* descP,
                             int               level,
                             int               opt)
{
    ERL_NIF_TERM result;
    int          val;
    SOCKOPTLEN_T valSz = sizeof(val);
    int          res;

    res = sock_getopt(descP->sock, level, opt, &val, &valSz);

    if (res != 0) {
        result = esock_make_error_errno(env, sock_errno());
    } else {
        result = esock_make_ok2(env, MKI(env, val));
    }

    return result;
}



/* ngetopt_timeval_opt - get an timeval option
 */
static
ERL_NIF_TERM ngetopt_timeval_opt(ErlNifEnv*        env,
                                 SocketDescriptor* descP,
                                 int               level,
                                 int               opt)
{
    ERL_NIF_TERM   result;
    struct timeval val;
    SOCKOPTLEN_T   valSz = sizeof(val);
    int            res;

    SSDBG( descP,
           ("SOCKET", "ngetopt_timeval_opt -> entry with"
            "\r\n   level: %d"
            "\r\n   opt:   %d"
            "\r\n", level, opt) );

    sys_memzero((char*) &val, valSz);
    res = sock_getopt(descP->sock, level, opt, &val, &valSz);

    if (res != 0) {
        result = esock_make_error_errno(env, sock_errno());
    } else {
        ERL_NIF_TERM eTimeVal;
        char*        xres;

        if ((xres = esock_encode_timeval(env, &val, &eTimeVal)) != NULL)
            result = esock_make_error_str(env, xres);
        else
            result = esock_make_ok2(env, eTimeVal);
    }

    SSDBG( descP,
           ("SOCKET", "ngetopt_timeval_opt -> done when"
            "\r\n   result: %T"
            "\r\n", result) );

    return result;
}



/* ngetopt_str_opt - get an string option
 *
 * We provide the max size of the string. This is the
 * size of the buffer we allocate for the value.
 * The actual size of the (read) value will be communicated
 * in the optSz variable.
 */
static
ERL_NIF_TERM ngetopt_str_opt(ErlNifEnv*        env,
                             SocketDescriptor* descP,
                             int               level,
                             int               opt,
                             int               max)
{
    ERL_NIF_TERM result;
    char*        val   = MALLOC(max);
    SOCKOPTLEN_T valSz = max;
    int          res;

    SSDBG( descP,
           ("SOCKET", "ngetopt_str_opt -> entry with"
            "\r\n   level: %d"
            "\r\n   opt:   %d"
            "\r\n   max:   %d"
            "\r\n", level, opt, max) );

    res = sock_getopt(descP->sock, level, opt, val, &valSz);

    if (res != 0) {
        result = esock_make_error_errno(env, sock_errno());
    } else {
        ERL_NIF_TERM sval = MKSL(env, val, valSz);

        result = esock_make_ok2(env, sval);
    }

    SSDBG( descP,
           ("SOCKET", "ngetopt_str_opt -> done when"
            "\r\n   result: %T"
            "\r\n", result) );

    FREE(val);

    return result;
}



/* ----------------------------------------------------------------------
 * nif_sockname - get socket name
 *
 * Description:
 * Returns the current address to which the socket is bound.
 *
 * Arguments:
 * Socket (ref) - Points to the socket descriptor.
 */

static
ERL_NIF_TERM nif_sockname(ErlNifEnv*         env,
                          int                argc,
                          const ERL_NIF_TERM argv[])
{
    SocketDescriptor* descP;
    ERL_NIF_TERM      res;

    SGDBG( ("SOCKET", "nif_sockname -> entry with argc: %d\r\n", argc) );

    /* Extract arguments and perform preliminary validation */

    if ((argc != 1) ||
        !enif_get_resource(env, argv[0], sockets, (void**) &descP)) {
        return enif_make_badarg(env);
    }

    SSDBG( descP,
           ("SOCKET", "nif_sockname -> args when sock = %d:"
            "\r\n   Socket: %T"
            "\r\n", descP->sock, argv[0]) );

    res = nsockname(env, descP);

    SSDBG( descP, ("SOCKET", "nif_sockname -> done with res = %T\r\n", res) );

    return res;
}



static
ERL_NIF_TERM nsockname(ErlNifEnv*        env,
                       SocketDescriptor* descP)
{
    SocketAddress  sa;
    SocketAddress* saP = &sa;
    unsigned int   sz  = sizeof(SocketAddress);

    sys_memzero((char*) saP, sz);
    if (IS_SOCKET_ERROR(sock_name(descP->sock, (struct sockaddr*) saP, &sz))) {
        return esock_make_error_errno(env, sock_errno());
    } else {
        ERL_NIF_TERM esa;
        char*        xres;

        if ((xres = esock_encode_sockaddr(env, saP, sz, &esa)) != NULL)
            return esock_make_error_str(env, xres);
        else
            return esock_make_ok2(env, esa);
    }
}



/* ----------------------------------------------------------------------
 * nif_peername - get name of the connected peer socket
 *
 * Description:
 * Returns the address of the peer connected to the socket.
 *
 * Arguments:
 * Socket (ref) - Points to the socket descriptor.
 */

static
ERL_NIF_TERM nif_peername(ErlNifEnv*         env,
                          int                argc,
                          const ERL_NIF_TERM argv[])
{
    SocketDescriptor* descP;
    ERL_NIF_TERM      res;

    SGDBG( ("SOCKET", "nif_peername -> entry with argc: %d\r\n", argc) );

    /* Extract arguments and perform preliminary validation */

    if ((argc != 1) ||
        !enif_get_resource(env, argv[0], sockets, (void**) &descP)) {
        return enif_make_badarg(env);
    }

    SSDBG( descP,
           ("SOCKET", "nif_peername -> args when sock = %d:"
            "\r\n   Socket: %T"
            "\r\n", descP->sock, argv[0]) );

    res = npeername(env, descP);

    SSDBG( descP, ("SOCKET", "nif_peername -> done with res = %T\r\n", res) );

    return res;
}



static
ERL_NIF_TERM npeername(ErlNifEnv*        env,
                       SocketDescriptor* descP)
{
    SocketAddress  sa;
    SocketAddress* saP = &sa;
    unsigned int   sz  = sizeof(SocketAddress);

    sys_memzero((char*) saP, sz);
    if (IS_SOCKET_ERROR(sock_peer(descP->sock, (struct sockaddr*) saP, &sz))) {
        return esock_make_error_errno(env, sock_errno());
    } else {
        ERL_NIF_TERM esa;
        char*        xres;

        if ((xres = esock_encode_sockaddr(env, saP, sz, &esa)) != NULL)
            return esock_make_error_str(env, xres);
        else
            return esock_make_ok2(env, esa);
    }
}



/* ----------------------------------------------------------------------
 * nif_cancel
 *
 * Description:
 * Cancel a previous select!
 *
 * Arguments:
 * Socket    (ref)  - Points to the socket descriptor.
 * Operation (atom) - What kind of operation (accept, send, ...) is to be cancelled
 * Ref       (ref)  - Unique id for the operation
 */
static
ERL_NIF_TERM nif_cancel(ErlNifEnv*         env,
                        int                argc,
                        const ERL_NIF_TERM argv[])
{
    SocketDescriptor* descP;
    ERL_NIF_TERM      op, opRef, result;

    SGDBG( ("SOCKET", "nif_cancel -> entry with argc: %d\r\n", argc) );

    /* Extract arguments and perform preliminary validation */

    if ((argc != 3) ||
        !enif_get_resource(env, argv[0], sockets, (void**) &descP)) {
        return enif_make_badarg(env);
    }
    op    = argv[1];
    opRef = argv[2];
        
    SSDBG( descP,
           ("SOCKET", "nif_cancel -> args when sock = %d:"
            "\r\n   op:    %T"
            "\r\n   opRef: %T"
            "\r\n", descP->sock, op, opRef) );
    
    result = ncancel(env, descP, op, opRef);

    SSDBG( descP,
           ("SOCKET", "nif_cancel -> done with result: "
           "\r\n   %T"
           "\r\n", result) );

    return result;

}


static
ERL_NIF_TERM ncancel(ErlNifEnv*        env,
                     SocketDescriptor* descP,
                     ERL_NIF_TERM      op,
                     ERL_NIF_TERM      opRef)
{
    /* <KOLLA>
     *
     * Do we really need all these variants? Should it not be enough with: 
     *
     *     connect | accept | send | recv
     *
     * </KOLLA>
     */
    if (COMPARE(op, esock_atom_connect) == 0) {
        return ncancel_connect(env, descP, opRef);
    } else if (COMPARE(op, esock_atom_accept) == 0) {
        return ncancel_accept(env, descP, opRef);
    } else if (COMPARE(op, esock_atom_send) == 0) {
        return ncancel_send(env, descP, opRef);
    } else if (COMPARE(op, esock_atom_sendto) == 0) {
        return ncancel_send(env, descP, opRef);
    } else if (COMPARE(op, esock_atom_sendmsg) == 0) {
        return ncancel_send(env, descP, opRef);
    } else if (COMPARE(op, esock_atom_recv) == 0) {
        return ncancel_recv(env, descP, opRef);
    } else if (COMPARE(op, esock_atom_recvfrom) == 0) {
        return ncancel_recv(env, descP, opRef);
    } else if (COMPARE(op, esock_atom_recvmsg) == 0) {
        return ncancel_recv(env, descP, opRef);
    } else {
        return esock_make_error(env, esock_atom_einval);
    }
}



/* *** ncancel_connect ***
 *
 *
 */
static
ERL_NIF_TERM ncancel_connect(ErlNifEnv*        env,
                             SocketDescriptor* descP,
                             ERL_NIF_TERM      opRef)
{
    return ncancel_write_select(env, descP, opRef);
}


/* *** ncancel_accept ***
 *
 * We have two different cases:
 *   *) Its the current acceptor
 *      Cancel the select!
 *      We need to activate one of the waiting acceptors.
 *   *) Its one of the acceptors ("waiting") in the queue
 *      Simply remove the acceptor from the queue.
 *
 */
static
ERL_NIF_TERM ncancel_accept(ErlNifEnv*        env,
                            SocketDescriptor* descP,
                            ERL_NIF_TERM      opRef)
{
    ERL_NIF_TERM res;

    SSDBG( descP,
           ("SOCKET", "ncancel_accept -> entry with"
            "\r\n   opRef: %T"
            "\r\n   %s"
            "\r\n", opRef,
            ((descP->currentAcceptorP == NULL) ? "without acceptor" : "with acceptor")) );

    MLOCK(descP->accMtx);

    if (descP->currentAcceptorP != NULL) {
        if (COMPARE(opRef, descP->currentAcceptor.ref) == 0) {
            res = ncancel_accept_current(env, descP);
        } else {
            res = ncancel_accept_waiting(env, descP, opRef);
        }
    } else {
        /* Or badarg? */
        res =  esock_make_error(env, esock_atom_einval);
    }

    MUNLOCK(descP->accMtx);

    SSDBG( descP,
           ("SOCKET", "ncancel_accept -> done with result:"
            "\r\n   %T"
            "\r\n", res) );

    return res;
}


/* The current acceptor process has an ongoing select we first must
 * cancel. Then we must re-activate the "first" (the first
 * in the acceptor queue).
 */
static
ERL_NIF_TERM ncancel_accept_current(ErlNifEnv*        env,
                                    SocketDescriptor* descP)
{
    ERL_NIF_TERM res;

    SSDBG( descP, ("SOCKET", "ncancel_accept_current -> entry\r\n") );

    res = ncancel_read_select(env, descP, descP->currentAcceptor.ref);

    SSDBG( descP, ("SOCKET", "ncancel_accept_current -> cancel res: %T\r\n", res) );

    if (acceptor_pop(env, descP,
                     &descP->currentAcceptor.pid,
                     &descP->currentAcceptor.mon,
                     &descP->currentAcceptor.ref)) {
        
        /* There was another one */
        
        SSDBG( descP, ("SOCKET", "ncancel_accept_current -> new (active) acceptor: "
                       "\r\n   pid: %T"
                       "\r\n   ref: %T"
                       "\r\n",
                       descP->currentAcceptor.pid,
                       descP->currentAcceptor.ref) );
        
        SELECT(env,
               descP->sock,
               (ERL_NIF_SELECT_READ),
               descP, &descP->currentAcceptor.pid, descP->currentAcceptor.ref);

    } else {
        SSDBG( descP, ("SOCKET", "ncancel_accept_current -> no more acceptors\r\n") );
        descP->currentAcceptorP = NULL;
        descP->state            = SOCKET_STATE_LISTENING;
    }
    
    SSDBG( descP, ("SOCKET", "ncancel_accept_current -> done with result:"
                   "\r\n   %T"
                   "\r\n", res) );

    return res;
}


/* These processes have not performed a select, so we can simply
 * remove them from the acceptor queue.
 */
static
ERL_NIF_TERM ncancel_accept_waiting(ErlNifEnv*        env,
                                    SocketDescriptor* descP,
                                    ERL_NIF_TERM      opRef)
{
    ErlNifPid caller;

    if (enif_self(env, &caller) == NULL)
        return esock_make_error(env, atom_exself);

    /* unqueue request from (acceptor) queue */

    if (acceptor_unqueue(env, descP, &caller)) {
        return esock_atom_ok;
    } else {
        /* Race? */
        return esock_make_error(env, esock_atom_not_found);
    }
}



/* *** ncancel_send ***
 *
 * Cancel a send operation.
 * Its either the current writer or one of the waiting writers.
 */
static
ERL_NIF_TERM ncancel_send(ErlNifEnv*        env,
                          SocketDescriptor* descP,
                          ERL_NIF_TERM      opRef)
{
    ERL_NIF_TERM res;

    MLOCK(descP->writeMtx);

    SSDBG( descP,
           ("SOCKET", "ncancel_send -> entry with"
            "\r\n   opRef: %T"
            "\r\n   %s"
            "\r\n", opRef,
            ((descP->currentWriterP == NULL) ? "without writer" : "with writer")) );

    if (descP->currentWriterP != NULL) {
        if (COMPARE(opRef, descP->currentWriter.ref) == 0) {
            res = ncancel_send_current(env, descP);
        } else {
            res = ncancel_send_waiting(env, descP, opRef);
        }
    } else {
        /* Or badarg? */
        res =  esock_make_error(env, esock_atom_einval);
    }

    MUNLOCK(descP->writeMtx);

    SSDBG( descP,
           ("SOCKET", "ncancel_send -> done with result:"
            "\r\n   %T"
            "\r\n", res) );

    return res;
}



/* The current writer process has an ongoing select we first must
 * cancel. Then we must re-activate the "first" (the first
 * in the writer queue).
 */
static
ERL_NIF_TERM ncancel_send_current(ErlNifEnv*        env,
                                  SocketDescriptor* descP)
{
    ERL_NIF_TERM res;

    SSDBG( descP, ("SOCKET", "ncancel_send_current -> entry\r\n") );

    res = ncancel_write_select(env, descP, descP->currentWriter.ref);

    SSDBG( descP, ("SOCKET", "ncancel_send_current -> cancel res: %T\r\n", res) );

    if (writer_pop(env, descP,
                   &descP->currentWriter.pid,
                   &descP->currentWriter.mon,
                   &descP->currentWriter.ref)) {
        
        /* There was another one */
        
        SSDBG( descP, ("SOCKET", "ncancel_send_current -> new (active) writer: "
                       "\r\n   pid: %T"
                       "\r\n   ref: %T"
                       "\r\n",
                       descP->currentWriter.pid,
                       descP->currentWriter.ref) );
        
        SELECT(env,
               descP->sock,
               (ERL_NIF_SELECT_WRITE),
               descP, &descP->currentWriter.pid, descP->currentWriter.ref);

    } else {
        SSDBG( descP, ("SOCKET", "ncancel_send_current -> no more writers\r\n") );
        descP->currentWriterP = NULL;
    }
    
    SSDBG( descP, ("SOCKET", "ncancel_send_current -> done with result:"
                   "\r\n   %T"
                   "\r\n", res) );

    return res;
}


/* These processes have not performed a select, so we can simply
 * remove them from the writer queue.
 */
static
ERL_NIF_TERM ncancel_send_waiting(ErlNifEnv*        env,
                                  SocketDescriptor* descP,
                                  ERL_NIF_TERM      opRef)
{
    ErlNifPid caller;

    if (enif_self(env, &caller) == NULL)
        return esock_make_error(env, atom_exself);

    /* unqueue request from (writer) queue */

    if (writer_unqueue(env, descP, &caller)) {
        return esock_atom_ok;
    } else {
        /* Race? */
        return esock_make_error(env, esock_atom_not_found);
    }
}



/* *** ncancel_recv ***
 *
 * Cancel a read operation.
 * Its either the current reader or one of the waiting readers.
 */
static
ERL_NIF_TERM ncancel_recv(ErlNifEnv*        env,
                          SocketDescriptor* descP,
                          ERL_NIF_TERM      opRef)
{
    ERL_NIF_TERM res;

    MLOCK(descP->readMtx);

    SSDBG( descP,
           ("SOCKET", "ncancel_recv -> entry with"
            "\r\n   opRef: %T"
            "\r\n   %s"
            "\r\n", opRef,
            ((descP->currentReaderP == NULL) ? "without reader" : "with reader")) );

    if (descP->currentReaderP != NULL) {
        if (COMPARE(opRef, descP->currentReader.ref) == 0) {
            res = ncancel_recv_current(env, descP);
        } else {
            res = ncancel_recv_waiting(env, descP, opRef);
        }
    } else {
        /* Or badarg? */
        res =  esock_make_error(env, esock_atom_einval);
    }

    MUNLOCK(descP->readMtx);

    SSDBG( descP,
           ("SOCKET", "ncancel_recv -> done with result:"
            "\r\n   %T"
            "\r\n", res) );

    return res;
}


/* The current reader process has an ongoing select we first must
 * cancel. Then we must re-activate the "first" (the first
 * in the reader queue).
 */
static
ERL_NIF_TERM ncancel_recv_current(ErlNifEnv*        env,
                                  SocketDescriptor* descP)
{
    ERL_NIF_TERM res;

    SSDBG( descP, ("SOCKET", "ncancel_recv_current -> entry\r\n") );

    res = ncancel_read_select(env, descP, descP->currentReader.ref);

    SSDBG( descP, ("SOCKET", "ncancel_recv_current -> cancel res: %T\r\n", res) );

    if (reader_pop(env, descP,
                   &descP->currentReader.pid,
                   &descP->currentReader.mon,
                   &descP->currentReader.ref)) {
        
        /* There was another one */
        
        SSDBG( descP, ("SOCKET", "ncancel_recv_current -> new (active) reader: "
                       "\r\n   pid: %T"
                       "\r\n   ref: %T"
                       "\r\n",
                       descP->currentReader.pid,
                       descP->currentReader.ref) );
        
        SELECT(env,
               descP->sock,
               (ERL_NIF_SELECT_READ),
               descP, &descP->currentReader.pid, descP->currentReader.ref);

    } else {
        SSDBG( descP, ("SOCKET", "ncancel_recv_current -> no more readers\r\n") );
        descP->currentReaderP = NULL;
    }
    
    SSDBG( descP, ("SOCKET", "ncancel_recv_current -> done with result:"
                   "\r\n   %T"
                   "\r\n", res) );

    return res;
}


/* These processes have not performed a select, so we can simply
 * remove them from the reader queue.
 */
static
ERL_NIF_TERM ncancel_recv_waiting(ErlNifEnv*        env,
                                  SocketDescriptor* descP,
                                  ERL_NIF_TERM      opRef)
{
    ErlNifPid caller;

    if (enif_self(env, &caller) == NULL)
        return esock_make_error(env, atom_exself);

    /* unqueue request from (reader) queue */

    if (reader_unqueue(env, descP, &caller)) {
        return esock_atom_ok;
    } else {
        /* Race? */
        return esock_make_error(env, esock_atom_not_found);
    }
}



static
ERL_NIF_TERM ncancel_read_select(ErlNifEnv*        env,
                                 SocketDescriptor* descP,
                                 ERL_NIF_TERM      opRef)
{
    return ncancel_mode_select(env, descP, opRef,
                               ERL_NIF_SELECT_READ,
                               ERL_NIF_SELECT_READ_CANCELLED);
}


static
ERL_NIF_TERM ncancel_write_select(ErlNifEnv*        env,
                                  SocketDescriptor* descP,
                                  ERL_NIF_TERM      opRef)
{
    return ncancel_mode_select(env, descP, opRef,
                               ERL_NIF_SELECT_WRITE,
                               ERL_NIF_SELECT_WRITE_CANCELLED);
}


static
ERL_NIF_TERM ncancel_mode_select(ErlNifEnv*        env,
                                 SocketDescriptor* descP,
                                 ERL_NIF_TERM      opRef,
                                 int               smode,
                                 int               rmode)
{
    int selectRes = enif_select(env, descP->sock,
                                (ERL_NIF_SELECT_CANCEL | smode),
                                descP, NULL, opRef);

    if (selectRes & rmode) {
        /* Was cancelled */
        return esock_atom_ok;
    } else if (selectRes > 0) {
        /* Has already sent the message */
        return esock_make_error(env, esock_atom_select_sent);
    } else {
        /* Stopped? */
        SSDBG( descP, ("SOCKET", "ncancel_mode_select -> failed: %d (0x%lX)"
                       "\r\n", selectRes, selectRes) );
        return esock_make_error(env, esock_atom_einval);
    }

}



/* ----------------------------------------------------------------------
 *  U t i l i t y   F u n c t i o n s
 * ----------------------------------------------------------------------
 */

/* *** send_check_writer ***
 *
 * Checks if we have a current writer and if that is us. If not, then we must
 * be made to wait for our turn. This is done by pushing us unto the writer queue.
 */
static
BOOLEAN_T send_check_writer(ErlNifEnv*        env,
                            SocketDescriptor* descP,
                            ERL_NIF_TERM      ref,
                            ERL_NIF_TERM*     checkResult)
{
    if (descP->currentWriterP != NULL) {
        ErlNifPid caller;
        
        if (enif_self(env, &caller) == NULL) {
            *checkResult = esock_make_error(env, atom_exself);
            return FALSE;
        }

        if (!compare_pids(env, &descP->currentWriter.pid, &caller)) {
            /* Not the "current writer", so (maybe) push onto queue */

            SSDBG( descP,
                   ("SOCKET", "send_check_writer -> not (current) writer\r\n") );

            if (!writer_search4pid(env, descP, &caller))
                *checkResult = writer_push(env, descP, caller, ref);
            else
                *checkResult = esock_make_error(env, esock_atom_eagain);
            
            SSDBG( descP,
                   ("SOCKET",
                    "send_check_writer -> queue (push) result: %T\r\n",
                    checkResult) );
            
            return FALSE;

        }
        
    }

    *checkResult = esock_atom_ok; // Does not actually matter in this case, but ...

    return TRUE;
}



/* *** send_check_result ***
 *
 * Check the result of a socket send (send, sendto and sendmsg) call.
 * If a "complete" send has been made, the next (waiting) writer will be 
 * scheduled (if there is one).
 * If we did not manage to send the entire package, make another select,
 * so that we can be informed when we can make another try (to send the rest),
 * and return with the amount we actually managed to send (its up to the caller
 * (that is the erlang code) to figure out hust much is left to send).
 * If the write fail, we give up and return with the appropriate error code.
 *
 * What about the remaining writers!!
 */
static
ERL_NIF_TERM send_check_result(ErlNifEnv*        env,
                               SocketDescriptor* descP,
                               ssize_t           written,
                               ssize_t           dataSize,
                               int               saveErrno,
                               ERL_NIF_TERM      sendRef)
{
    SSDBG( descP,
           ("SOCKET", "send_check_result -> entry with"
            "\r\n   written:   %d"
            "\r\n   dataSize:  %d"
            "\r\n   saveErrno: %d"
            "\r\n", written, dataSize, saveErrno) );

    if (written >= dataSize) {

        cnt_inc(&descP->writePkgCnt,  1);
        cnt_inc(&descP->writeByteCnt, written);
        if (descP->currentWriterP != NULL)
            DEMONP(env, descP, &descP->currentWriter.mon);

        SSDBG( descP,
               ("SOCKET", "send_check_result -> "
                "everything written (%d,%d) - done\r\n", dataSize, written) );

        /* Ok, this write is done maybe activate the next (if any) */

        if (writer_pop(env, descP,
                       &descP->currentWriter.pid,
                       &descP->currentWriter.mon,
                       &descP->currentWriter.ref)) {

            /* There was another one */

            SSDBG( descP, ("SOCKET", "send_check_result -> new (active) writer: "
                           "\r\n   pid: %T"
                           "\r\n   ref: %T"
                           "\r\n",
                           descP->currentWriter.pid,
                           descP->currentWriter.ref) );

            SELECT(env,
                   descP->sock,
                   (ERL_NIF_SELECT_WRITE),
                   descP, &descP->currentWriter.pid, descP->currentWriter.ref);

        } else {
            descP->currentWriterP = NULL;
        }

        return esock_atom_ok;

    } else if (written < 0) {

        /* Some kind of send failure - check what kind */

        if ((saveErrno != EAGAIN) && (saveErrno != EINTR)) {
            ErlNifPid     pid;
            ErlNifMonitor mon;
            ERL_NIF_TERM  ref, res;

            /* 
             * An actual failure - we (and everyone waiting) give up
             */

            cnt_inc(&descP->writeFails, 1);

            SSDBG( descP,
                   ("SOCKET", "send_check_result -> error: %d\r\n", saveErrno) );

            res = esock_make_error_errno(env, saveErrno);

            if (descP->currentWriterP != NULL) {
                DEMONP(env, descP, &descP->currentWriter.mon);

                while (writer_pop(env, descP, &pid, &mon, &ref)) {
                    SSDBG( descP,
                           ("SOCKET", "send_check_result -> abort %T\r\n", pid) );
                    send_msg_nif_abort(env, ref, res, &pid);
                    DEMONP(env, descP, &mon);
                }
            }
            
            return res;

        } else {

            /* Ok, try again later */

            SSDBG( descP, ("SOCKET", "send_check_result -> try again\r\n") );

            /* <KOLLA>
             * SHOULD RESULT IN {error, eagain}!!!!
             * </KOLLA>
             */
            written = 0;

        }
    }

    /* We failed to write the *entire* packet (anything less then size
     * of the packet, which is 0 <= written < sizeof packet),
     * so schedule the rest for later.
     */

    if (descP->currentWriterP == NULL) {
        ErlNifPid caller;

        if (enif_self(env, &caller) == NULL)
            return esock_make_error(env, atom_exself);
        descP->currentWriter.pid = caller;
        if (MONP(env, descP,
                 &descP->currentWriter.pid,
                 &descP->currentWriter.mon) > 0)
            return esock_make_error(env, atom_exmon);
        descP->currentWriter.ref = enif_make_copy(descP->env, sendRef);
        descP->currentWriterP    = &descP->currentWriter;
    }

    cnt_inc(&descP->writeWaits, 1);

    SELECT(env, descP->sock, (ERL_NIF_SELECT_WRITE),
           descP, NULL, sendRef);

    SSDBG( descP,
           ("SOCKET", "send_check_result -> not entire package written\r\n") );

    return esock_make_ok2(env, MKI(env, written));

}



/* *** recv_check_reader ***
 *
 * Checks if we have a current reader and if that is us. If not, then we must
 * be made to wait for our turn. This is done by pushing us unto the reader queue.
 */
static
BOOLEAN_T recv_check_reader(ErlNifEnv*        env,
                            SocketDescriptor* descP,
                            ERL_NIF_TERM      ref,
                            ERL_NIF_TERM*     checkResult)
{
    if (descP->currentReaderP != NULL) {
        ErlNifPid caller;
        
        if (enif_self(env, &caller) == NULL) {
            *checkResult = esock_make_error(env, atom_exself);
            return FALSE;
        }

        if (!compare_pids(env, &descP->currentReader.pid, &caller)) {
            /* Not the "current reader", so (maybe) push onto queue */

            SSDBG( descP,
                   ("SOCKET", "recv_check_reader -> not (current) reader\r\n") );

            if (!reader_search4pid(env, descP, &caller))
                *checkResult = reader_push(env, descP, caller, ref);
            else
                *checkResult = esock_make_error(env, esock_atom_eagain);
            
            SSDBG( descP,
                   ("SOCKET",
                    "recv_check_reader -> queue (push) result: %T\r\n",
                    checkResult) );
            
            return FALSE;

        }
        
    }

    *checkResult = esock_atom_ok; // Does not actually matter in this case, but ...

    return TRUE;
}



/* *** recv_init_current_reader ***
 *
 * Initiate (maybe) the currentReader structure of the descriptor.
 * Including monitoring the calling process.
 */
static
char* recv_init_current_reader(ErlNifEnv*        env,
                               SocketDescriptor* descP,
                               ERL_NIF_TERM      recvRef)
{
    if (descP->currentReaderP == NULL) {
        ErlNifPid caller;

        if (enif_self(env, &caller) == NULL)
            return str_exself;

        descP->currentReader.pid = caller;
        if (MONP(env, descP,
                 &descP->currentReader.pid,
                 &descP->currentReader.mon) > 0) {
            return str_exmon;
        }
        descP->currentReader.ref = enif_make_copy(descP->env, recvRef);
        descP->currentReaderP    = &descP->currentReader;
    }

    return NULL;
}



/* *** recv_update_current_reader ***
 *
 * Demonitors the current reader process and pop's the reader queue.
 * If there is a waiting (reader) process, then it will be assigned
 * as the new current reader and a new (read) select will be done.
 */

static
ERL_NIF_TERM recv_update_current_reader(ErlNifEnv*        env,
                                        SocketDescriptor* descP)
{
    if (descP->currentReaderP != NULL) {
        
        DEMONP(env, descP, &descP->currentReader.mon);
        
        if (reader_pop(env, descP,
                       &descP->currentReader.pid,
                       &descP->currentReader.mon,
                       &descP->currentReader.ref)) {
            
            /* There was another one */
            
            SSDBG( descP,
                   ("SOCKET", "recv_update_current_reader -> new (active) reader: "
                    "\r\n   pid: %T"
                    "\r\n   ref: %T"
                    "\r\n",
                    descP->currentReader.pid,
                    descP->currentReader.ref) );
            
            SELECT(env,
                   descP->sock,
                   (ERL_NIF_SELECT_READ),
                   descP,
                   &descP->currentReader.pid,
                   descP->currentReader.ref);
            
        } else {
            descP->currentWriterP = NULL;
        }
    }

    return esock_atom_ok;
}



/* *** recv_error_current_reader ***
 *
 * Process the current reader and any waiting readers
 * when a read (fatal) error has occured.
 * All waiting readers will be "aborted", that is a 
 * nif_abort message will be sent (with reaf and reason).
 */
static
void recv_error_current_reader(ErlNifEnv*        env,
                               SocketDescriptor* descP,
                               ERL_NIF_TERM      reason)
{
    if (descP->currentReaderP != NULL) {
        ErlNifPid     pid;
        ErlNifMonitor mon;
        ERL_NIF_TERM  ref;

        DEMONP(env, descP, &descP->currentReader.mon);

        while (reader_pop(env, descP, &pid, &mon, &ref)) {
            SSDBG( descP,
                   ("SOCKET", "recv_error_current_reader -> abort %T\r\n", pid) );
            send_msg_nif_abort(env, ref, reason, &pid);
            DEMONP(env, descP, &mon);
        }
    }
}



/* *** recv_check_result ***
 *
 * Process the result of a call to recv.
 */
static
ERL_NIF_TERM recv_check_result(ErlNifEnv*        env,
                               SocketDescriptor* descP,
                               int               read,
                               int               toRead,
                               int               saveErrno,
                               ErlNifBinary*     bufP,
                               ERL_NIF_TERM      recvRef)
{
    char*        xres;
    ERL_NIF_TERM data;

    SSDBG( descP,
           ("SOCKET", "recv_check_result -> entry with"
            "\r\n   read:      %d"
            "\r\n   toRead:    %d"
            "\r\n   saveErrno: %d"
            "\r\n   recvRef:   %T"
            "\r\n", read, toRead, saveErrno, recvRef) );


    /* <KOLLA>
     *
     * We need to handle read = 0 for other type(s) (DGRAM) when
     * its actually valid to read 0 bytes.
     *
     * </KOLLA>
     */
    
    if ((read == 0) && (descP->type == SOCK_STREAM)) {
        ERL_NIF_TERM res = esock_make_error(env, atom_closed);
        
        /*
         * When a stream socket peer has performed an orderly shutdown, the return
         * value will be 0 (the traditional "end-of-file" return).
         *
         * *We* do never actually try to read 0 bytes from a stream socket!
         *
         * We must also notify any waiting readers!
         */

        recv_error_current_reader(env, descP, res);

        return res;

    }
    
    /* There is a special case: If the provided 'to read' value is
     * zero (0) (only for type =/= stream).
     * That means that we reads as much as we can, using the default
     * read buffer size.
     */

    if (bufP->size == read) {

        /* +++ We filled the buffer +++ */

        SSDBG( descP,
               ("SOCKET",
                "recv_check_result -> [%d] filled the buffer\r\n", toRead) );

        if (toRead == 0) {

            /* +++ Give us everything you have got => needs to continue +++ */

            /* How do we do this?
             * Either:
             * 1) Send up each chunk of data for each of the read
             *    and let the erlang code assemble it: {ok, false, Bin}
             *    (when complete it should return {ok, true, Bin}).
             *    We need to read atleast one more time to be sure if its
             *    done...
             * 2) Or put it in a buffer here, and then let the erlang code
             *    know that it should call again (special return value)
             *    (continuous binary realloc "here").
             *
             * => We choose alt 1 for now.
             */

            cnt_inc(&descP->readByteCnt, read);

            if ((xres = recv_init_current_reader(env, descP, recvRef)) != NULL)
                return esock_make_error_str(env, xres);

            data = MKBIN(env, bufP);

            SSDBG( descP,
                   ("SOCKET",
                    "recv_check_result -> [%d] "
                    "we are done for now - read more\r\n", toRead) );

            return esock_make_ok3(env, atom_false, data);

        } else {

            /* +++ We got exactly as much as we requested +++ */

            /* <KOLLA>
             * WE NEED TO INFORM ANY WAITING READERS
             *
             * DEMONP of the current reader!
             *
             * </KOLLA>
             */

            cnt_inc(&descP->readPkgCnt,  1);
            cnt_inc(&descP->readByteCnt, read);

            SSDBG( descP,
                   ("SOCKET",
                    "recv_check_result -> [%d] "
                    "we got exactly what we could fit\r\n", toRead) );

            recv_update_current_reader(env, descP);

            data = MKBIN(env, bufP);

            return esock_make_ok3(env, atom_true, data);

        }

    } else if (read < 0) {

        /* +++ Error handling +++ */

        if (saveErrno == ECONNRESET)  {
            ERL_NIF_TERM res = esock_make_error(env, atom_closed);

            /* +++ Oups - closed +++ */

            SSDBG( descP, ("SOCKET",
                           "recv_check_result -> [%d] closed\r\n", toRead) );

            /* <KOLLA>
             *
             * IF THE CURRENT PROCESS IS *NOT* THE CONTROLLING
             * PROCESS, WE NEED TO INFORM IT!!!
             *
             * ALL WAITING PROCESSES MUST ALSO GET THE ERROR!!
             * HANDLED BY THE STOP (CALLBACK) FUNCTION?
             *
             * SINCE THIS IS A REMOTE CLOSE, WE DON'T NEED TO WAIT
             * FOR OUTPUT TO BE WRITTEN (NO ONE WILL READ), JUST
             * ABORT THE SOCKET REGARDLESS OF LINGER???
             *
             * </KOLLA>
             */

            descP->closeLocal = FALSE;
            descP->state      = SOCKET_STATE_CLOSING;

            recv_error_current_reader(env, descP, res);

            SELECT(env,
                   descP->sock,
                   (ERL_NIF_SELECT_STOP),
                   descP, NULL, recvRef);

            return res;

        } else if ((saveErrno == ERRNO_BLOCK) ||
                   (saveErrno == EAGAIN)) {
            SSDBG( descP, ("SOCKET",
                           "recv_check_result -> [%d] eagain\r\n", toRead) );

            SELECT(env, descP->sock, (ERL_NIF_SELECT_READ),
                   descP, NULL, recvRef);

            return esock_make_error(env, esock_atom_eagain);
        } else {
            ERL_NIF_TERM res = esock_make_error_errno(env, saveErrno);

            SSDBG( descP, ("SOCKET", "recv_check_result -> [%d] errno: %d\r\n",
                           toRead, saveErrno) );

            recv_error_current_reader(env, descP, res);

            return res;
        }

    } else {

        /* +++ We did not fill the buffer +++ */

        SSDBG( descP,
               ("SOCKET",
                "recv_check_result -> [%d] "
                "did not fill the buffer (%d of %d)\r\n",
                toRead, read, bufP->size) );

        if (toRead == 0) {

            /* +++ We got it all, but since we      +++
             * +++ did not fill the buffer, we      +++
             * +++ must split it into a sub-binary. +++
             */

            SSDBG( descP, ("SOCKET",
                           "recv_check_result -> [%d] split buffer\r\n", toRead) );

            cnt_inc(&descP->readPkgCnt,  1);
            cnt_inc(&descP->readByteCnt, read);

            recv_update_current_reader(env, descP);

            data = MKBIN(env, bufP);
            data = MKSBIN(env, data, 0, read);

            SSDBG( descP,
                   ("SOCKET", "recv_check_result -> [%d] done\r\n", toRead) );

            return esock_make_ok3(env, atom_true, data);

        } else {

            /* +++ We got only a part of what was expected +++
             * +++ => receive more later.                  +++ */

            SSDBG( descP, ("SOCKET", "recv_check_result -> [%d] "
                           "only part of message - expect more\r\n", toRead) );

            cnt_inc(&descP->readByteCnt, read);

            return esock_make_ok3(env, atom_false, MKBIN(env, bufP));
        }
    }
}


/* The recvfrom function delivers one (1) message. If our buffer
 * is to small, the message will be truncated. So, regardless
 * if we filled the buffer or not, we have got what we are going
 * to get regarding this message.
 */
static
ERL_NIF_TERM recvfrom_check_result(ErlNifEnv*        env,
                                   SocketDescriptor* descP,
                                   int               read,
                                   int               saveErrno,
                                   ErlNifBinary*     bufP,
                                   SocketAddress*    fromAddrP,
                                   unsigned int      fromAddrLen,
                                   ERL_NIF_TERM      recvRef)
{
    ERL_NIF_TERM data;

    SSDBG( descP,
           ("SOCKET", "recvfrom_check_result -> entry with"
            "\r\n   read:      %d"
            "\r\n   saveErrno: %d"
            "\r\n   recvRef:   %T"
            "\r\n", read, saveErrno, recvRef) );


    /* There is a special case: If the provided 'to read' value is
     * zero (0). That means that we reads as much as we can, using
     * the default read buffer size.
     */

    if (read < 0) {

        /* +++ Error handling +++ */

        if (saveErrno == ECONNRESET)  {

            /* +++ Oups - closed +++ */

            SSDBG( descP, ("SOCKET", "recvfrom_check_result -> closed\r\n") );

            /* <KOLLA>
             * IF THE CURRENT PROCESS IS *NOT* THE CONTROLLING
             * PROCESS, WE NEED TO INFORM IT!!!
             *
             * ALL WAITING PROCESSES MUST ALSO GET THE ERROR!!
             *
             * </KOLLA>
             */

            descP->closeLocal = FALSE;
            descP->state      = SOCKET_STATE_CLOSING;

            SELECT(env,
                   descP->sock,
                   (ERL_NIF_SELECT_STOP),
                   descP, NULL, recvRef);

            return esock_make_error(env, atom_closed);

        } else if ((saveErrno == ERRNO_BLOCK) ||
                   (saveErrno == EAGAIN)) {

            SSDBG( descP, ("SOCKET", "recvfrom_check_result -> eagain\r\n") );
            
            SELECT(env, descP->sock, (ERL_NIF_SELECT_READ),
                   descP, NULL, recvRef);

            return esock_make_error(env, esock_atom_eagain);
        } else {

            SSDBG( descP,
                   ("SOCKET",
                    "recvfrom_check_result -> errno: %d\r\n", saveErrno) );
            
            return esock_make_error_errno(env, saveErrno);
        }

    } else {

        /* +++ We sucessfully got a message - time to encode the address +++ */

        ERL_NIF_TERM eSockAddr;

        esock_encode_sockaddr(env,
                              fromAddrP, fromAddrLen,
                              &eSockAddr);

        if (read == bufP->size) {
            data = MKBIN(env, bufP);
        } else {

            /* +++ We got a chunk of data but +++
             * +++ since we did not fill the  +++
             * +++ buffer, we must split it   +++
             * +++ into a sub-binary.         +++
             */

            data = MKBIN(env, bufP);
            data = MKSBIN(env, data, 0, read);
        }

        return esock_make_ok2(env, MKT2(env, eSockAddr, data));

    }
}



/* The recvmsg function delivers one (1) message. If our buffer
 * is to small, the message will be truncated. So, regardless
 * if we filled the buffer or not, we have got what we are going
 * to get regarding this message.
 */
static
ERL_NIF_TERM recvmsg_check_result(ErlNifEnv*        env,
                                  SocketDescriptor* descP,
                                  int               read,
                                  int               saveErrno,
                                  struct msghdr*    msgHdrP,
                                  ErlNifBinary*     dataBufP,
                                  ErlNifBinary*     ctrlBufP,
                                  ERL_NIF_TERM      recvRef)
{

    SSDBG( descP,
           ("SOCKET", "recvmsg_check_result -> entry with"
            "\r\n   read:      %d"
            "\r\n   saveErrno: %d"
            "\r\n   recvRef:   %T"
            "\r\n", read, saveErrno, recvRef) );


    /* <KOLLA>
     *
     * We need to handle read = 0 for other type(s) (DGRAM) when
     * its actually valid to read 0 bytes.
     *
     * </KOLLA>
     */
    
    if ((read == 0) && (descP->type == SOCK_STREAM)) {
        
        /*
         * When a stream socket peer has performed an orderly shutdown, the return
         * value will be 0 (the traditional "end-of-file" return).
         *
         * *We* do never actually try to read 0 bytes from a stream socket!
         */

        return esock_make_error(env, atom_closed);

    }


    /* There is a special case: If the provided 'to read' value is
     * zero (0). That means that we reads as much as we can, using
     * the default read buffer size.
     */

    if (read < 0) {

        /* +++ Error handling +++ */

        if (saveErrno == ECONNRESET)  {

            /* +++ Oups - closed +++ */

            SSDBG( descP, ("SOCKET", "recvmsg_check_result -> closed\r\n") );

            /* <KOLLA>
             * IF THE CURRENT PROCESS IS *NOT* THE CONTROLLING
             * PROCESS, WE NEED TO INFORM IT!!!
             *
             * ALL WAITING PROCESSES MUST ALSO GET THE ERROR!!
             *
             * </KOLLA>
             */

            descP->closeLocal = FALSE;
            descP->state      = SOCKET_STATE_CLOSING;

            SELECT(env,
                   descP->sock,
                   (ERL_NIF_SELECT_STOP),
                   descP, NULL, recvRef);

            return esock_make_error(env, atom_closed);

        } else if ((saveErrno == ERRNO_BLOCK) ||
                   (saveErrno == EAGAIN)) {

            SSDBG( descP, ("SOCKET", "recvmsg_check_result -> eagain\r\n") );
            
            SELECT(env, descP->sock, (ERL_NIF_SELECT_READ),
                   descP, NULL, recvRef);

            return esock_make_error(env, esock_atom_eagain);
        } else {

            SSDBG( descP,
                   ("SOCKET",
                    "recvmsg_check_result -> errno: %d\r\n", saveErrno) );
            
            return esock_make_error_errno(env, saveErrno);
        }

    } else {

        /* +++ We sucessfully got a message - time to encode it +++ */

        ERL_NIF_TERM eMsgHdr;
        char*        xres;

        /*
         * <KOLLA>
         *
         * The return value of recvmsg is the *total* number of bytes
         * that where successfully read. This data has been put into
         * the *IO vector*.
         *
         * </KOLLA>
         */

        if ((xres = encode_msghdr(env, descP,
                                  read, msgHdrP, dataBufP, ctrlBufP,
                                  &eMsgHdr)) != NULL) {
            
            SSDBG( descP,
                   ("SOCKET",
                    "recvmsg_check_result -> "
                    "(msghdr) encode failed: %s\r\n", xres) );
            
            return esock_make_error_str(env, xres);
        } else {

            SSDBG( descP,
                   ("SOCKET",
                    "recvmsg_check_result -> "
                    "(msghdr) encode ok: %T\r\n", eMsgHdr) );
            
            return esock_make_ok2(env, eMsgHdr);
        }

    }
}




/* +++ encode_msghdr +++
 *
 * Encode a msghdr (recvmsg). In erlang its represented as
 * a map, which has a specific set of attributes:
 *
 *     addr (source address) - sockaddr()
 *     iov                   - [binary()]
 *     ctrl                  - [cmsghdr()]
 *     flags                 - msghdr_flags()
 */

extern
char* encode_msghdr(ErlNifEnv*        env,
                    SocketDescriptor* descP,
                    int               read,
                    struct msghdr*    msgHdrP,
                    ErlNifBinary*     dataBufP,
                    ErlNifBinary*     ctrlBufP,
                    ERL_NIF_TERM*     eSockAddr)
{
    char*        xres;
    ERL_NIF_TERM addr, iov, ctrl, flags;

    SSDBG( descP,
           ("SOCKET", "encode_msghdr -> entry with"
            "\r\n   read: %d"
            "\r\n", read) );

    /* The address is not used if we are connected,
     * so check (length = 0) before we try to encodel
     */
    if (msgHdrP->msg_namelen != 0) {
        if ((xres = esock_encode_sockaddr(env,
                                          (SocketAddress*) msgHdrP->msg_name,
                                          msgHdrP->msg_namelen,
                                          &addr)) != NULL)
            return xres;
    } else {
        addr = esock_atom_undefined;
    }

    SSDBG( descP, ("SOCKET", "encode_msghdr -> try encode iov\r\n") );
    if ((xres = esock_encode_iov(env,
                                 read,
                                 msgHdrP->msg_iov,
                                 msgHdrP->msg_iovlen,
                                 dataBufP,
                                 &iov)) != NULL)
        return xres;

    SSDBG( descP, ("SOCKET", "encode_msghdr -> try encode cmsghdrs\r\n") );
    if ((xres = encode_cmsghdrs(env, descP, ctrlBufP, msgHdrP, &ctrl)) != NULL)
        return xres;

    SSDBG( descP, ("SOCKET", "encode_msghdr -> try encode flags\r\n") );
    if ((xres = encode_msghdr_flags(env, descP, msgHdrP->msg_flags, &flags)) != NULL)
        return xres;

    SSDBG( descP,
           ("SOCKET", "encode_msghdr -> components encoded:"
            "\r\n   addr:  %T"
            "\r\n   iov:   %T"
            "\r\n   ctrl:  %T"
            "\r\n   flags: %T"
           "\r\n", addr, iov, ctrl, flags) );
    {
        ERL_NIF_TERM keys[]  = {esock_atom_addr,
                                esock_atom_iov,
                                esock_atom_ctrl,
                                esock_atom_flags};
        ERL_NIF_TERM vals[]  = {addr, iov, ctrl, flags};
        unsigned int numKeys = sizeof(keys) / sizeof(ERL_NIF_TERM);
        unsigned int numVals = sizeof(vals) / sizeof(ERL_NIF_TERM);
        ERL_NIF_TERM tmp;
        
        ESOCK_ASSERT( (numKeys == numVals) );
        
        SSDBG( descP, ("SOCKET", "encode_msghdr -> create msghdr map\r\n") );
        if (!MKMA(env, keys, vals, numKeys, &tmp))
            return ESOCK_STR_EINVAL;

        SSDBG( descP, ("SOCKET", "encode_msghdr -> msghdr: "
                       "\r\n   %T"
                       "\r\n", tmp) );

        *eSockAddr = tmp;
    }

    SSDBG( descP, ("SOCKET", "encode_msghdr -> done\r\n") );

    return NULL;
}




/* +++ encode_cmsghdrs +++
 *
 * Encode a list of cmsghdr(). There can be 0 or more cmsghdr "blocks".
 *
 * Our "problem" is that we have no idea how many control messages
 * we have.
 *
 * The cmsgHdrP arguments points to the start of the control data buffer,
 * an actual binary. Its the only way to create sub-binaries. So, what we
 * need to continue processing this is to turn that into an binary erlang 
 * term (which can then in turn be turned into sub-binaries).
 *
 * We need the cmsgBufP (even though cmsgHdrP points to it) to be able
 * to create sub-binaries (one for each cmsg hdr).
 *
 * The TArray (term array) is created with the size of 128, which should
 * be enough. But if its not, then it will be automatically realloc'ed during
 * add. Once we are done adding hdr's to it, we convert the tarray to a list.
 */

extern
char* encode_cmsghdrs(ErlNifEnv*        env,
                      SocketDescriptor* descP,
                      ErlNifBinary*     cmsgBinP,
                      struct msghdr*    msgHdrP,
                      ERL_NIF_TERM*     eCMsgHdr)
{
    ERL_NIF_TERM    ctrlBuf  = MKBIN(env, cmsgBinP); // The *entire* binary
    SocketTArray    cmsghdrs = TARRAY_CREATE(128);
    struct cmsghdr* firstP   = CMSG_FIRSTHDR(msgHdrP);
    struct cmsghdr* currentP;
    
    SSDBG( descP, ("SOCKET", "encode_cmsghdrs -> entry\r\n") );

    for (currentP = firstP;
         currentP != NULL;
         currentP = CMSG_NXTHDR(msgHdrP, currentP)) {

        SSDBG( descP,
               ("SOCKET", "encode_cmsghdrs -> process cmsg header when"
                "\r\n   TArray Size: %d"
                "\r\n", TARRAY_SZ(cmsghdrs)) );

        /* MUST check this since on Linux the returned "cmsg" may actually
         * go too far!
         */
        if (((CHARP(currentP) + currentP->cmsg_len) - CHARP(firstP)) >
            msgHdrP->msg_controllen) {
            /* Ouch, fatal error - give up 
             * We assume we cannot trust any data if this is wrong.
             */
            TARRAY_DELETE(cmsghdrs);
            return ESOCK_STR_EINVAL;
        } else {
            ERL_NIF_TERM   level, type, data;
            unsigned char* dataP   = (unsigned char*) CMSG_DATA(currentP);
            size_t         dataPos = dataP - cmsgBinP->data;
            size_t         dataLen = currentP->cmsg_len - (CHARP(currentP)-CHARP(dataP));

            SSDBG( descP,
                   ("SOCKET", "encode_cmsghdrs -> cmsg header data: "
                    "\r\n   dataPos: %d"
                    "\r\n   dataLen: %d"
                    "\r\n", dataPos, dataLen) );

            /* We can't give up just because its an unknown protocol,
             * so if its a protocol we don't know, we return its integer 
             * value and leave it to the user.
             */
            if (encode_cmsghdr_level(env, currentP->cmsg_level, &level) != NULL)
                level = MKI(env, currentP->cmsg_level);

            if (encode_cmsghdr_type(env,
                                    currentP->cmsg_level, currentP->cmsg_type,
                                    &type) != NULL)
                type = MKI(env, currentP->cmsg_type);

            if (encode_cmsghdr_data(env, ctrlBuf,
                                    currentP->cmsg_level,
                                    currentP->cmsg_type,
                                    dataP, dataPos, dataLen,
                                    &data) != NULL)
                data = MKSBIN(env, ctrlBuf, dataPos, dataLen);

            SSDBG( descP,
                   ("SOCKET", "encode_cmsghdrs -> "
                    "\r\n   level: %T"
                    "\r\n   type:  %T"
                    "\r\n   data:  %T"
                    "\r\n", level, type, data) );

            /* And finally create the 'cmsghdr' map -
             * and if successfull add it to the tarray.
             */
            {
                ERL_NIF_TERM keys[]  = {esock_atom_level,
                                        esock_atom_type,
                                        esock_atom_data};
                ERL_NIF_TERM vals[]  = {level, type, data};
                unsigned int numKeys = sizeof(keys) / sizeof(ERL_NIF_TERM);
                unsigned int numVals = sizeof(vals) / sizeof(ERL_NIF_TERM);
                ERL_NIF_TERM cmsgHdr;

                /* Guard agains cut-and-paste errors */
                ESOCK_ASSERT( (numKeys == numVals) );
            
                if (!MKMA(env, keys, vals, numKeys, &cmsgHdr)) {
                    TARRAY_DELETE(cmsghdrs);
                    return ESOCK_STR_EINVAL;
                }

                /* And finally add it to the list... */
                TARRAY_ADD(cmsghdrs, cmsgHdr);
            }
        }
    }

    SSDBG( descP,
           ("SOCKET", "encode_cmsghdrs -> cmsg headers processed when"
            "\r\n   TArray Size: %d"
            "\r\n", TARRAY_SZ(cmsghdrs)) );

    /* The tarray is populated - convert it to a list */
    TARRAY_TOLIST(cmsghdrs, env, eCMsgHdr);

    return NULL;
}



/* +++ decode_cmsghdrs +++
 *
 * Decode a list of cmsghdr(). There can be 0 or more cmsghdr "blocks".
 *
 * Each element can either be a (erlang) map that needs to be decoded,
 * or a (erlang) binary that just needs to be appended to the control
 * buffer.
 *
 * Our "problem" is that we have no idea much memory we actually need.
 *
 */

extern
char* decode_cmsghdrs(ErlNifEnv*        env,
                      SocketDescriptor* descP,
                      ERL_NIF_TERM      eCMsgHdr,
                      char*             cmsgHdrBufP,
                      size_t            cmsgHdrBufLen,
                      size_t*           cmsgHdrBufUsed)
{
    ERL_NIF_TERM elem, tail, list;
    char*        bufP;
    size_t       rem, used, totUsed = 0;
    unsigned int len;
    int          i;
    char*        xres;

    SSDBG( descP, ("SOCKET", "decode_cmsghdrs -> entry with"
                   "\r\n   cmsgHdrBufP:   0x%lX"
                   "\r\n   cmsgHdrBufLen: %d"
                   "\r\n", cmsgHdrBufP, cmsgHdrBufLen) );

    if (IS_LIST(env, eCMsgHdr) && GET_LIST_LEN(env, eCMsgHdr, &len)) {

        SSDBG( descP, ("SOCKET", "decode_cmsghdrs -> list length: %d\r\n", len) );

        for (i = 0, list = eCMsgHdr, rem  = cmsgHdrBufLen, bufP = cmsgHdrBufP;
             i < len; i++) {
            
            SSDBG( descP, ("SOCKET", "decode_cmsghdrs -> process elem %d:"
                           "\r\n   (buffer) rem:     %u"
                           "\r\n   (buffer) totUsed: %u"
                           "\r\n", i, rem, totUsed) );

            /* Extract the (current) head of the (cmsg hdr) list */
            if (!GET_LIST_ELEM(env, list, &elem, &tail))
                return ESOCK_STR_EINVAL;
            
            used = 0; // Just in case...
            if ((xres = decode_cmsghdr(env, descP, elem, bufP, rem, &used)) != NULL)
                return xres;

            bufP     = CHARP( ULONG(bufP) + used );
            rem      = SZT( rem - used );
            list     = tail;
            totUsed += used;

        }

        SSDBG( descP, ("SOCKET",
                       "decode_cmsghdrs -> all %d ctrl headers processed\r\n",
                       len) );

        xres = NULL;
    } else {
        xres = ESOCK_STR_EINVAL;
    }

    *cmsgHdrBufUsed = totUsed;

    SSDBG( descP, ("SOCKET", "decode_cmsghdrs -> done with %s when"
                   "\r\n   totUsed = %u\r\n",
                   ((xres != NULL) ? xres : "NULL"), totUsed) );

    return xres;
}


/* +++ decode_cmsghdr +++
 *
 * Decode one cmsghdr(). Put the "result" into the buffer and advance the
 * pointer (of the buffer) afterwards. Also update 'rem' accordingly.
 * But before the actual decode, make sure that there is enough room in 
 * the buffer for the cmsg header (sizeof(*hdr) < rem).
 *
 * The eCMsgHdr should be a map with three fields: 
 *
 *     level :: cmsghdr_level()   (socket | protocol() | integer())
 *     type  :: cmsghdr_type()    (atom() | integer())
 *                                What values are valid depend on the level
 *     data  :: cmsghdr_data()    (term() | binary())
 *                                The type of the data depends on
 *                                level and type, but can be a binary,
 *                                which means that the data is already coded.
 */
extern
char* decode_cmsghdr(ErlNifEnv*        env,
                     SocketDescriptor* descP,
                     ERL_NIF_TERM      eCMsgHdr,
                     char*             bufP,
                     size_t            rem,
                     size_t*           used)
{
    SSDBG( descP, ("SOCKET", "decode_cmsghdr -> entry with"
                   "\r\n   eCMsgHdr: %T"
                   "\r\n", eCMsgHdr) );

    if (IS_MAP(env, eCMsgHdr)) {
        ERL_NIF_TERM eLevel, eType, eData;
        int          level, type;
        char*        xres;

        /* First extract all three attributes (as terms) */

        if (!GET_MAP_VAL(env, eCMsgHdr, esock_atom_level, &eLevel))
            return ESOCK_STR_EINVAL;
        
        SSDBG( descP, ("SOCKET", "decode_cmsghdr -> eLevel: %T"
                       "\r\n", eLevel) );

        if (!GET_MAP_VAL(env, eCMsgHdr, esock_atom_type, &eType))
            return ESOCK_STR_EINVAL;
        
        SSDBG( descP, ("SOCKET", "decode_cmsghdr -> eType:  %T"
                       "\r\n", eType) );

        if (!GET_MAP_VAL(env, eCMsgHdr, esock_atom_data, &eData))
            return ESOCK_STR_EINVAL;

        SSDBG( descP, ("SOCKET", "decode_cmsghdr -> eData:  %T"
                       "\r\n", eData) );

        /* Second, decode level */
        if ((xres = decode_cmsghdr_level(env, eLevel, &level)) != NULL)
            return xres;

        SSDBG( descP, ("SOCKET", "decode_cmsghdr -> level:  %d\r\n", level) );

        /* third, decode type */
        if ((xres = decode_cmsghdr_type(env, level, eType, &type)) != NULL)
            return xres;
        
        SSDBG( descP, ("SOCKET", "decode_cmsghdr -> type:   %d\r\n", type) );

        /* And finally data
         * If its a binary, we are done. Otherwise, we need to check
         * level and type to know what kind of data to expect.
         */

        return decode_cmsghdr_data(env, descP, bufP, rem, level, type, eData, used);

    } else {
        *used = 0;
        return ESOCK_STR_EINVAL;
    }

    return NULL;
}


/* *** decode_cmsghdr_data ***
 *
 * For all combinations of level and type we accept a binary as data,
 * so we begin by testing for that. If its not a binary, then we check
 * level (ip) and type (tos or ttl), in which case the data *must* be
 * an integer and ip_tos() respectively.
 */
static
char* decode_cmsghdr_data(ErlNifEnv*        env,
                          SocketDescriptor* descP,
                          char*             bufP,
                          size_t            rem,
                          int               level,
                          int               type,
                          ERL_NIF_TERM      eData,
                          size_t*           used)
{
    char* xres;

    SSDBG( descP, ("SOCKET", "decode_cmsghdr_data -> entry with"
                   "\r\n   eData: %T"
                   "\r\n", eData) );

    if (IS_BIN(env, eData)) {
        ErlNifBinary bin;
        
        if (GET_BIN(env, eData, &bin)) {
            SSDBG( descP, ("SOCKET", "decode_cmsghdr_data -> "
                           "do final decode with binary\r\n") );
            return decode_cmsghdr_final(descP, bufP, rem, level, type,
                                        (char*) bin.data, bin.size,
                                        used);
        } else {
            *used = 0;
            xres  = ESOCK_STR_EINVAL;
        }
    } else {

        /* Its *not* a binary so we need to look at what level and type 
         * we have and treat them individually.
         */

        switch (level) {
#if defined(SOL_IP)
        case SOL_IP:
#else
        case IPPROTO_IP:
#endif
            switch (type) {
#if defined(IP_TOS)
            case IP_TOS:
                {
                    int data;
                    if (decode_ip_tos(env, eData, &data)) {
                        SSDBG( descP, ("SOCKET", "decode_cmsghdr_data -> "
                                       "do final decode with tos\r\n") );
                        return decode_cmsghdr_final(descP, bufP, rem, level, type,
                                                    (char*) &data,
                                                    sizeof(data),
                                                    used);
                    } else {
                        *used = 0;
                        xres  = ESOCK_STR_EINVAL;
                    }
                }
                break;
#endif

#if defined(IP_TTL)
            case IP_TTL:
                {
                    int data;
                    if (GET_INT(env, eData, &data)) {
                        SSDBG( descP, ("SOCKET", "decode_cmsghdr_data -> "
                                       "do final decode with ttl\r\n") );
                        return decode_cmsghdr_final(descP, bufP, rem, level, type,
                                                    (char*) &data,
                                                    sizeof(data),
                                                    used);
                    } else {
                        *used = 0;
                        xres  = ESOCK_STR_EINVAL;
                    }
                }
                break;
#endif

            }
            break;

        default:
            *used = 0;
            xres  = ESOCK_STR_EINVAL;
            break;
        }        

    }

    return xres;
}
                              

/* *** decode_cmsghdr_final ***
 *
 * This does the final create of the cmsghdr (including the data copy).
 */
static
char* decode_cmsghdr_final(SocketDescriptor* descP,
                           char*             bufP,
                           size_t            rem,
                           int               level,
                           int               type,
                           char*             data,
                           int               sz,
                           size_t*           used)
{
    int len   = CMSG_LEN(sz);
    int space = CMSG_SPACE(sz);

    SSDBG( descP, ("SOCKET", "decode_cmsghdr_data -> entry when"
                   "\r\n   level: %d"
                   "\r\n   type:  %d"
                   "\r\n   sz:    %d => %d, %d"
                   "\r\n", level, type, sz, len, space) );

    if (rem >= space) {
        struct cmsghdr* cmsgP = (struct cmsghdr*) bufP;
        
        /* The header */
        cmsgP->cmsg_len   = len;
        cmsgP->cmsg_level = level;
        cmsgP->cmsg_type  = type;

        sys_memcpy(CMSG_DATA(cmsgP), data, sz);
        *used = space;
    } else {
        *used = 0;
        return ESOCK_STR_EINVAL;
    }

    SSDBG( descP, ("SOCKET", "decode_cmsghdr_final -> done\r\n") );

    return NULL;
}


/* +++ encode_cmsghdr_level +++
 *
 * Encode the level part of the cmsghdr().
 *
 */

static
char* encode_cmsghdr_level(ErlNifEnv*    env,
                           int           level,
                           ERL_NIF_TERM* eLevel)
{
    char* xres;

    switch (level) {
    case SOL_SOCKET:
        *eLevel = esock_atom_socket;
        xres    = NULL;
        break;

#if defined(SOL_IP)
    case SOL_IP:
#else
    case IPPROTO_IP:
#endif
        *eLevel = esock_atom_ip;
        xres    = NULL;
        break;

#if defined(SOL_IPV6)
    case SOL_IPV6:
        *eLevel = esock_atom_ip;
        xres    = NULL;
        break;
#endif

    case IPPROTO_UDP:
        *eLevel = esock_atom_udp;
        xres    = NULL;
        break;

    default:
        *eLevel = MKI(env, level);
        xres    = NULL;
        break;
    }

    return xres;
}



/* +++ decode_cmsghdr_level +++
 *
 * Decode the level part of the cmsghdr().
 *
 */

static
char* decode_cmsghdr_level(ErlNifEnv*   env,
                           ERL_NIF_TERM eLevel,
                           int*         level)
{
    char* xres = NULL;

    if (IS_ATOM(env, eLevel)) {

        if (COMPARE(eLevel, esock_atom_socket) == 0) {
            *level = SOL_SOCKET;
            xres   = NULL;
        } else if (COMPARE(eLevel, esock_atom_ip) == 0) {
#if defined(SOL_IP)
            *level = SOL_IP;
#else
            *level = IPPROTO_IP;
#endif
            xres   = NULL;
#if defined(SOL_IPV6)
        } else if (COMPARE(eLevel, esock_atom_ipv6) == 0) {
            *level = SOL_IPV6;
            xres   = NULL;
#endif
        } else if (COMPARE(eLevel, esock_atom_udp) == 0) {
            *level = IPPROTO_UDP;
            xres   = NULL;
        } else {
            *level = -1;
            xres   = ESOCK_STR_EINVAL;
        }
    } else if (IS_NUM(env, eLevel)) {
        if (!GET_INT(env, eLevel, level))
            xres = ESOCK_STR_EINVAL;
    } else {
        *level = -1;
        xres   = ESOCK_STR_EINVAL;
    }

    return xres;
}



/* +++ encode_cmsghdr_type +++
 *
 * Encode the type part of the cmsghdr().
 *
 */

static
char* encode_cmsghdr_type(ErlNifEnv*    env,
                          int           level,
                          int           type,
                          ERL_NIF_TERM* eType)
{
    char* xres = NULL;

    switch (level) {
    case SOL_SOCKET:
        switch (type) {
#if defined(SO_TIMESTAMP)
        case SO_TIMESTAMP:
            *eType = esock_atom_timestamp;
            break;
#endif

#if defined(SCM_RIGHTS)
        case SCM_RIGHTS:
            *eType = esock_atom_rights;
            break;
#endif

#if defined(SCM_CREDENTIALS)
        case SCM_CREDENTIALS:
            *eType = esock_atom_credentials;
            break;
#endif

        default:
            xres = ESOCK_STR_EINVAL;
            break;
        }        
        break;

#if defined(SOL_IP)
    case SOL_IP:
#else
    case IPPROTO_IP:
#endif
        switch (type) {
#if defined(IP_TOS)
        case IP_TOS:
            *eType = esock_atom_tos;
            break;
#endif

#if defined(IP_TTL)
        case IP_TTL:
            *eType = esock_atom_ttl;
            break;
#endif

#if defined(IP_PKTINFO)
        case IP_PKTINFO:
            *eType = esock_atom_pktinfo;
            break;
#endif

#if defined(IP_ORIGDSTADDR)
        case IP_ORIGDSTADDR:
            *eType = esock_atom_origdstaddr;
            break;
#endif

        default:
            xres = ESOCK_STR_EINVAL;
            break;
        }
        break;

#if defined(SOL_IPV6)
    case SOL_IPV6:
        switch (type) {
#if defined(IPV6_PKTINFO)
        case IPV6_PKTINFO:
            *eType = esock_atom_pktinfo;
            break;
#endif

        default:
            xres = ESOCK_STR_EINVAL;
            break;
        }        
        break;
#endif

    case IPPROTO_TCP:
        switch (type) {
        default:
            xres = ESOCK_STR_EINVAL;
            break;
        }        
        break;

    case IPPROTO_UDP:
        switch (type) {
        default:
            xres = ESOCK_STR_EINVAL;
            break;
        }        
        break;

#if defined(HAVE_SCTP)
    case IPPROTO_SCTP:
        switch (type) {
        default:
            xres = ESOCK_STR_EINVAL;
            break;
        }        
        break;
#endif

    default:
        xres = ESOCK_STR_EINVAL;
        break;
    }

    return xres;
}



/* +++ decode_cmsghdr_type +++
 *
 * Decode the type part of the cmsghdr().
 *
 */

static
char* decode_cmsghdr_type(ErlNifEnv*   env,
                          int          level,
                          ERL_NIF_TERM eType,
                          int*         type)
{
    char* xres = NULL;

    switch (level) {
    case SOL_SOCKET:
        if (IS_NUM(env, eType)) {
            if (!GET_INT(env, eType, type)) {
                *type = -1;
                xres  = ESOCK_STR_EINVAL;
            }
        } else {
            *type = -1;
            xres = ESOCK_STR_EINVAL;
        }
        break;


#if defined(SOL_IP)
    case SOL_IP:
#else
    case IPPROTO_IP:
#endif
        if (IS_ATOM(env, eType)) {
            if (COMPARE(eType, esock_atom_tos) == 0) {
#if defined(IP_TOS)
                *type = IP_TOS;
#else
                xres  = ESOCK_STR_EINVAL;
#endif
            } else if (COMPARE(eType, esock_atom_ttl) == 0) {
#if defined(IP_TTL)
                *type = IP_TTL;
#else
                xres  = ESOCK_STR_EINVAL;
#endif
            } else {
                xres = ESOCK_STR_EINVAL;
            }
        } else if (IS_NUM(env, eType)) {
            if (!GET_INT(env, eType, type)) {
                *type = -1;
                xres  = ESOCK_STR_EINVAL;
            }
        } else {
            *type = -1;
            xres  = ESOCK_STR_EINVAL;
        }
        break;
        
#if defined(SOL_IPV6)
    case SOL_IPV6:
        if (IS_NUM(env, eType)) {
            if (!GET_INT(env, eType, type)) {
                *type = -1;
                xres  = ESOCK_STR_EINVAL;
            }
        } else {
            *type = -1;
            xres = ESOCK_STR_EINVAL;
        }
        break;
#endif
        
    case IPPROTO_UDP:
        if (IS_NUM(env, eType)) {
            if (!GET_INT(env, eType, type)) {
                *type = -1;
                xres  = ESOCK_STR_EINVAL;
            }
        } else {
            *type = -1;
            xres = ESOCK_STR_EINVAL;
        }
        break;

    default:
        *type = -1;
        xres  = ESOCK_STR_EINVAL;
        break;
    }

    return xres;
}



/* +++ encode_cmsghdr_data +++
 *
 * Encode the data part of the cmsghdr().
 *
 */

static
char* encode_cmsghdr_data(ErlNifEnv*     env,
                          ERL_NIF_TERM   ctrlBuf,
                          int            level,
                          int            type,
                          unsigned char* dataP,
                          size_t         dataPos,
                          size_t         dataLen,
                          ERL_NIF_TERM*  eCMsgHdrData)
{
    char* xres;

    switch (level) {
#if defined(SOL_SOCKET)
    case SOL_SOCKET:
        xres = encode_cmsghdr_data_socket(env, ctrlBuf, type,
                                          dataP, dataPos, dataLen,
                                          eCMsgHdrData);
        break;
#endif

#if defined(SOL_IP)
    case SOL_IP:
#else
    case IPPROTO_IP:
#endif
        xres = encode_cmsghdr_data_ip(env, ctrlBuf, type,
                                      dataP, dataPos, dataLen,
                                      eCMsgHdrData);
        break;

#if defined(SOL_IPV6)
    case SOL_IPV6:
        xres = encode_cmsghdr_data_ipv6(env, ctrlBuf, type,
                                        dataP, dataPos, dataLen,
                                        eCMsgHdrData);
        break;
#endif

        /*
          case IPPROTO_TCP:
          xres = encode_cmsghdr_data_tcp(env, type, dataP, eCMsgHdrData);
          break;
        */

        /*
          case IPPROTO_UDP:
          xres = encode_cmsghdr_data_udp(env, type, dataP, eCMsgHdrData);
          break;
        */

        /*
          #if defined(HAVE_SCTP)
          case IPPROTO_SCTP:
          xres = encode_cmsghdr_data_sctp(env, type, dataP, eCMsgHdrData);
          break;
          #endif
        */

    default:
        *eCMsgHdrData = MKSBIN(env, ctrlBuf, dataPos, dataLen);
        xres = NULL;
        break;
    }

    return xres;
}



/* +++ encode_cmsghdr_data_socket +++
 *
 * Encode the data part when "protocol" = socket of the cmsghdr().
 *
 */

static
char* encode_cmsghdr_data_socket(ErlNifEnv*     env,
                                 ERL_NIF_TERM   ctrlBuf,
                                 int            type,
                                 unsigned char* dataP,
                                 size_t         dataPos,
                                 size_t         dataLen,
                                 ERL_NIF_TERM*  eCMsgHdrData)
{
    // char* xres;

    switch (type) {
#if defined(SO_TIMESTAMP)
    case SO_TIMESTAMP:
        {
            struct timeval* timeP = (struct timeval*) dataP;

            if (esock_encode_timeval(env, timeP, eCMsgHdrData) != NULL)
                *eCMsgHdrData = MKSBIN(env, ctrlBuf, dataPos, dataLen);
        }
        break;
#endif

    default:
        *eCMsgHdrData = MKSBIN(env, ctrlBuf, dataPos, dataLen);
        break;
    }

    return NULL;
}



/* +++ encode_cmsghdr_data_ip +++
 *
 * Encode the data part when protocol = IP of the cmsghdr().
 *
 */

static
char* encode_cmsghdr_data_ip(ErlNifEnv*     env,
                             ERL_NIF_TERM   ctrlBuf,
                             int            type,
                             unsigned char* dataP,
                             size_t         dataPos,
                             size_t         dataLen,
                             ERL_NIF_TERM*  eCMsgHdrData)
{
    char* xres = NULL;

    switch (type) {
#if defined(IP_TOS)
    case IP_TOS:
        {
            unsigned char tos = *dataP;
            switch (IPTOS_TOS(tos)) {
            case IPTOS_LOWDELAY:
                *eCMsgHdrData = esock_atom_lowdelay;
                break;
            case IPTOS_THROUGHPUT:
                *eCMsgHdrData = esock_atom_throughput;
                break;
            case IPTOS_RELIABILITY:
                *eCMsgHdrData = esock_atom_reliability;
                break;
            case IPTOS_MINCOST:
                *eCMsgHdrData = esock_atom_mincost;
                break;
            default:
                *eCMsgHdrData = MKUI(env, tos);
                break;
            }
        }
        break;
#endif

#if defined(IP_TTL)
    case IP_TTL:
        {
            int ttl = *((int*) dataP);
            *eCMsgHdrData = MKI(env, ttl);
        }
        break;
#endif

#if defined(IP_PKTINFO)
    case IP_PKTINFO:
        {
            struct in_pktinfo* pktInfoP = (struct in_pktinfo*) dataP;
            ERL_NIF_TERM       ifIndex  = MKUI(env, pktInfoP->ipi_ifindex);
            ERL_NIF_TERM       specDst, addr;

            if ((xres = esock_encode_ip4_address(env,
                                                 &pktInfoP->ipi_spec_dst,
                                                 &specDst)) != NULL) {
                *eCMsgHdrData = esock_atom_undefined;
                return xres;
            }

            if ((xres = esock_encode_ip4_address(env,
                                                 &pktInfoP->ipi_addr,
                                                 &addr)) != NULL) {
                *eCMsgHdrData = esock_atom_undefined;
                return xres;
            }


            {
                ERL_NIF_TERM keys[] = {esock_atom_ifindex,
                                       esock_atom_spec_dst,
                                       esock_atom_addr};
                ERL_NIF_TERM vals[] = {ifIndex, specDst, addr};
                unsigned int numKeys = sizeof(keys) / sizeof(ERL_NIF_TERM);
                unsigned int numVals = sizeof(vals) / sizeof(ERL_NIF_TERM);
    
                ESOCK_ASSERT( (numKeys == numVals) );
                
                if (!MKMA(env, keys, vals, numKeys, eCMsgHdrData)) {
                    *eCMsgHdrData = esock_atom_undefined;
                    return ESOCK_STR_EINVAL;
                }
            }
        }
        break;
#endif

#if defined(IP_ORIGDSTADDR)
    case IP_ORIGDSTADDR:
        if ((xres = esock_encode_sockaddr_in4(env,
                                              (struct sockaddr_in*) dataP,
                                              dataLen,
                                              eCMsgHdrData)) != NULL) {
            *eCMsgHdrData = esock_atom_undefined;
            return xres;            
        }
        break;
#endif

    default:
        *eCMsgHdrData = MKSBIN(env, ctrlBuf, dataPos, dataLen);
        break;
    }

    return xres;
}



/* +++ encode_cmsghdr_data_ipv6 +++
 *
 * Encode the data part when protocol = IPv6 of the cmsghdr().
 *
 */
#if defined(SOL_IPV6)
static
char* encode_cmsghdr_data_ipv6(ErlNifEnv*     env,
                               ERL_NIF_TERM   ctrlBuf,
                               int            type,
                               unsigned char* dataP,
                               size_t         dataPos,
                               size_t         dataLen,
                               ERL_NIF_TERM*  eCMsgHdrData)
{
    char* xres;

    switch (type) {
#if defined(IPV6_PKTINFO)
    case IPV6_PKTINFO:
        {
            struct in6_pktinfo* pktInfoP = (struct in6_pktinfo*) dataP;
            ERL_NIF_TERM        ifIndex  = MKI(env, pktInfoP->ipi6_ifindex);
            ERL_NIF_TERM        addr;

            if ((xres = esock_encode_ip6_address(env,
                                                 &pktInfoP->ipi6_addr,
                                                 &addr)) != NULL) {
                *eCMsgHdrData = esock_atom_undefined;
                return xres;
            }

            {
                ERL_NIF_TERM keys[]  = {esock_atom_addr, esock_atom_ifindex};
                ERL_NIF_TERM vals[]  = {addr, ifIndex};
                unsigned int numKeys = sizeof(keys) / sizeof(ERL_NIF_TERM);
                unsigned int numVals = sizeof(vals) / sizeof(ERL_NIF_TERM);
    
                ESOCK_ASSERT( (numKeys == numVals) );
                
                if (!MKMA(env, keys, vals, numKeys, eCMsgHdrData)) {
                    *eCMsgHdrData = esock_atom_undefined;
                    return ESOCK_STR_EINVAL;
                }
            }
        }
        break;
#endif

    default:
        *eCMsgHdrData = MKSBIN(env, ctrlBuf, dataPos, dataLen);
        break;
    }

    return NULL;
}
#endif



/* +++ encode_msghdr_flags +++
 *
 * Encode a list of msghdr_flag().
 *
 * The following flags are handled: eor | trunc | ctrunc | oob | errqueue.
 */

extern
char* encode_msghdr_flags(ErlNifEnv*        env,
                          SocketDescriptor* descP,
                          int               msgFlags,
                          ERL_NIF_TERM*     flags)
{
    SSDBG( descP,
           ("SOCKET", "encode_cmsghdrs_flags -> entry with"
            "\r\n   msgFlags: %d (0x%lX)"
            "\r\n", msgFlags, msgFlags) );

    if (msgFlags == 0) {
        *flags = MKEL(env);
        return NULL;
    } else {
        SocketTArray ta = TARRAY_CREATE(10); // Just to be on the safe side

#if defined(MSG_EOR)
        if ((msgFlags & MSG_EOR) == MSG_EOR)
            TARRAY_ADD(ta, esock_atom_eor);
#endif

#if defined(MSG_TRUNC)
        if ((msgFlags & MSG_TRUNC) == MSG_TRUNC)
            TARRAY_ADD(ta, esock_atom_trunc);
#endif
    
#if defined(MSG_CTRUNC)
        if ((msgFlags & MSG_CTRUNC) == MSG_CTRUNC)
            TARRAY_ADD(ta, esock_atom_ctrunc);
#endif
    
#if defined(MSG_OOB)
        if ((msgFlags & MSG_OOB) == MSG_OOB)
            TARRAY_ADD(ta, esock_atom_oob);
#endif
    
#if defined(MSG_ERRQUEUE)
        if ((msgFlags & MSG_ERRQUEUE) == MSG_ERRQUEUE)
            TARRAY_ADD(ta, esock_atom_errqueue);
#endif

        SSDBG( descP,
               ("SOCKET", "esock_encode_cmsghdrs -> flags processed when"
                "\r\n   TArray size: %d"
                "\r\n", TARRAY_SZ(ta)) );

        TARRAY_TOLIST(ta, env, flags);

        return NULL;
    }
}




/* +++ decode the linger value +++
 * The (socket) linger option is provided as a two tuple:
 *
 *       {OnOff :: boolean(), Time :: integer()}
 *
 */
static
BOOLEAN_T decode_sock_linger(ErlNifEnv* env, ERL_NIF_TERM eVal, struct linger* valP)
{
    const ERL_NIF_TERM* lt; // The array of the elements of the tuple
    int                 sz; // The size of the tuple - should be 2
    BOOLEAN_T           onOff;
    int                 secs;

    if (!GET_TUPLE(env, eVal, &sz, &lt))
        return FALSE;

    if (sz != 2)
        return FALSE;


    /* So fas so good - now check the two elements of the tuple. */

    onOff = esock_decode_bool(lt[0]);

    if (!GET_INT(env, lt[1], &secs))
        return FALSE;

    valP->l_onoff  = (onOff) ? 1 : 0;
    valP->l_linger = secs;

    return TRUE;
}



/* +++ decode the ip socket option TOS +++
 * The (ip) option can be provide in two ways:
 *
 *           atom() | integer()
 *
 * When its an atom it can have the values:
 *
 *       lowdelay |  throughput | reliability | mincost
 *
 */
#if defined(IP_TOS)
static
BOOLEAN_T decode_ip_tos(ErlNifEnv* env, ERL_NIF_TERM eVal, int* val)
{
    BOOLEAN_T result = FALSE;

    if (IS_ATOM(env, eVal)) {

        if (COMPARE(eVal, esock_atom_lowdelay) == 0) {
            *val   = IPTOS_LOWDELAY;
            result = TRUE;
        } else if (COMPARE(eVal, esock_atom_throughput) == 0) {
            *val   = IPTOS_THROUGHPUT;
            result = TRUE;
        } else if (COMPARE(eVal, esock_atom_reliability) == 0) {
            *val   = IPTOS_RELIABILITY;
            result = TRUE;
        } else if (COMPARE(eVal, esock_atom_mincost) == 0) {
            *val   = IPTOS_MINCOST;
            result = TRUE;
        } else {
            *val   = -1;
            result = FALSE;
        }
            
    } else if (IS_NUM(env, eVal)) {

        if (GET_INT(env, eVal, val)) {
            result = TRUE;
        } else {
            *val   = -1;
            result = FALSE;
        }

    } else {
        *val   = -1;
        result = FALSE;
    }

    return result;
}
#endif



/* +++ decode the ip socket option MTU_DISCOVER +++
 * The (ip) option can be provide in two ways:
 *
 *           atom() | integer()
 *
 * When its an atom it can have the values:
 *
 *       want | dont | do | probe
 *
 */
#if defined(IP_MTU_DISCOVER)
static
char* decode_ip_pmtudisc(ErlNifEnv* env, ERL_NIF_TERM eVal, int* val)
{
    char* res = NULL;

    if (IS_ATOM(env, eVal)) {

        if (COMPARE(eVal, atom_want) == 0) {
            *val = IP_PMTUDISC_WANT;
        } else if (COMPARE(eVal, atom_dont) == 0) {
            *val = IP_PMTUDISC_DONT;
        } else if (COMPARE(eVal, atom_do) == 0) {
            *val = IP_PMTUDISC_DO;
        } else if (COMPARE(eVal, atom_probe) == 0) {
            *val = IP_PMTUDISC_PROBE;
        } else {
            *val = -1;
            res  = ESOCK_STR_EINVAL;
        }

    } else if (IS_NUM(env, eVal)) {

        if (!GET_INT(env, eVal, val)) {
            *val = -1;
            res  = ESOCK_STR_EINVAL;
        }

    } else {

        *val   = -1;
        res  = ESOCK_STR_EINVAL;

    }

    return res;
}
#endif



/* +++ decode the ipv6 socket option MTU_DISCOVER +++
 * The (ip) option can be provide in two ways:
 *
 *           atom() | integer()
 *
 * When its an atom it can have the values:
 *
 *       want | dont | do | probe
 *
 */
#if defined(IPV6_MTU_DISCOVER)
static
char* decode_ipv6_pmtudisc(ErlNifEnv* env, ERL_NIF_TERM eVal, int* val)
{
    char* res = NULL;

    if (IS_ATOM(env, eVal)) {

        if (COMPARE(eVal, atom_want) == 0) {
            *val = IPV6_PMTUDISC_WANT;
        } else if (COMPARE(eVal, atom_dont) == 0) {
            *val = IPV6_PMTUDISC_DONT;
        } else if (COMPARE(eVal, atom_do) == 0) {
            *val = IPV6_PMTUDISC_DO;
        } else if (COMPARE(eVal, atom_probe) == 0) {
            *val = IPV6_PMTUDISC_PROBE;
        } else {
            *val = -1;
            res  = ESOCK_STR_EINVAL;
        }

    } else if (IS_NUM(env, eVal)) {

        if (!GET_INT(env, eVal, val)) {
            *val = -1;
            res  = ESOCK_STR_EINVAL;
        }

    } else {

        *val   = -1;
        res  = ESOCK_STR_EINVAL;

    }

    return res;
}
#endif



/* +++ encode the ip socket option MTU_DISCOVER +++
 * The (ip) option can be provide in two ways:
 *
 *           atom() | integer()
 *
 * If its one of the "known" values, it will be an atom:
 *
 *       want | dont | do | probe
 *
 */
#if defined(IP_MTU_DISCOVER)
static
void encode_ip_pmtudisc(ErlNifEnv* env, int val, ERL_NIF_TERM* eVal)
{
    switch (val) {
    case IP_PMTUDISC_WANT:
        *eVal = atom_want;
        break;

    case IP_PMTUDISC_DONT:
        *eVal = atom_dont;
        break;

    case IP_PMTUDISC_DO:
        *eVal = atom_do;
        break;

    case IP_PMTUDISC_PROBE:
        *eVal = atom_probe;
        break;

    default:
        *eVal = MKI(env, val);
        break;
    }

    return;
}
#endif



/* +++ encode the ipv6 socket option MTU_DISCOVER +++
 * The (ipv6) option can be provide in two ways:
 *
 *           atom() | integer()
 *
 * If its one of the "known" values, it will be an atom:
 *
 *       want | dont | do | probe
 *
 */
#if defined(IPV6_MTU_DISCOVER)
static
void encode_ipv6_pmtudisc(ErlNifEnv* env, int val, ERL_NIF_TERM* eVal)
{
    switch (val) {
    case IPV6_PMTUDISC_WANT:
        *eVal = atom_want;
        break;

    case IPV6_PMTUDISC_DONT:
        *eVal = atom_dont;
        break;

    case IPV6_PMTUDISC_DO:
        *eVal = atom_do;
        break;

    case IPV6_PMTUDISC_PROBE:
        *eVal = atom_probe;
        break;

    default:
        *eVal = MKI(env, val);
        break;
    }

    return;
}
#endif



/* +++ decocde the native getopt option +++
 * The option is in this case provide in the form of a two tuple:
 *
 *           {NativeOpt, ValueSize}
 *
 * NativeOpt :: integer()
 * ValueSize :: int | bool | non_neg_integer()
 *
 */
static
BOOLEAN_T decode_native_get_opt(ErlNifEnv* env, ERL_NIF_TERM eVal,
                                int* opt, uint16_t* valueType, int* valueSz)
{
    const ERL_NIF_TERM* nativeOptT;
    int                 nativeOptTSz;

    /* First, get the tuple and verify its size (2) */

    if (!GET_TUPLE(env, eVal, &nativeOptTSz, &nativeOptT))
        return FALSE;

    if (nativeOptTSz != 2)
        return FALSE;

    /* So far so good.
     * First element is an integer.
     * Second element is an atom or an integer.
     * The only "types" that we support at the moment are:
     *
     *            bool - Which is actually a integer
     *                   (but will be *returned* as a boolean())
     *            int  - Just short for integer
     */

    if (!GET_INT(env, nativeOptT[0], opt))
        return FALSE;

    if (IS_ATOM(env, nativeOptT[1])) {

        if (COMPARE(nativeOptT[1], atom_int) == 0) {
            SGDBG( ("SOCKET", "decode_native_get_opt -> int\r\n") );
            *valueType = SOCKET_OPT_VALUE_TYPE_INT;
            *valueSz   = sizeof(int); // Just to be sure
        } else if (COMPARE(nativeOptT[1], atom_bool) == 0) {
            SGDBG( ("SOCKET", "decode_native_get_opt -> bool\r\n") );
            *valueType = SOCKET_OPT_VALUE_TYPE_BOOL;
            *valueSz   = sizeof(int); // Just to be sure
        } else {
            return FALSE;
        }
    } else if (IS_NUM(env, nativeOptT[1])) {
        if (GET_INT(env, nativeOptT[1], valueSz)) {
            SGDBG( ("SOCKET", "decode_native_get_opt -> unspec\r\n") );
            *valueType = SOCKET_OPT_VALUE_TYPE_UNSPEC;
        } else {
            return FALSE;
        }
    } else {
        return FALSE;
    }

    SGDBG( ("SOCKET", "decode_native_get_opt -> done\r\n") );

    return TRUE;
}



/* +++ encode the ip socket option tos +++
 * The (ip) option can be provide as:
 *
 *       lowdelay |  throughput | reliability | mincost | integer()
 *
 */
static
ERL_NIF_TERM encode_ip_tos(ErlNifEnv* env, int val)
{
    ERL_NIF_TERM result;

    switch (IPTOS_TOS(val)) {
    case IPTOS_LOWDELAY:
        result = esock_make_ok2(env, esock_atom_lowdelay);
        break;

    case IPTOS_THROUGHPUT:
        result = esock_make_ok2(env, esock_atom_throughput);
        break;

    case IPTOS_RELIABILITY:
        result = esock_make_ok2(env, esock_atom_reliability);
        break;

    case IPTOS_MINCOST:
        result = esock_make_ok2(env, esock_atom_mincost);
        break;

    default:
        result = esock_make_ok2(env, MKI(env, val));
        break;
    }

    return result;
}





/* *** alloc_descriptor ***
 * Allocate and perform basic initialization of a socket descriptor.
 *
 */
static
SocketDescriptor* alloc_descriptor(SOCKET sock, HANDLE event)
{
    SocketDescriptor* descP;

    if ((descP = enif_alloc_resource(sockets, sizeof(SocketDescriptor))) != NULL) {
        char buf[64]; /* Buffer used for building the mutex name */

        descP->env            = enif_alloc_env();

        sprintf(buf, "socket[w,%d]", sock);
        descP->writeMtx       = MCREATE(buf);
        descP->currentWriterP = NULL; // currentWriter not used
        descP->writersQ.first = NULL;
        descP->writersQ.last  = NULL;
        descP->isWritable     = TRUE;
        descP->writePkgCnt    = 0;
        descP->writeByteCnt   = 0;
        descP->writeTries     = 0;
        descP->writeWaits     = 0;
        descP->writeFails     = 0;

        sprintf(buf, "socket[r,%d]", sock);
        descP->readMtx        = MCREATE(buf);
        descP->currentReaderP = NULL; // currentReader not used
        descP->readersQ.first = NULL;
        descP->readersQ.last  = NULL;
        descP->isReadable     = TRUE;
        descP->readPkgCnt     = 0;
        descP->readByteCnt    = 0;
        descP->readTries      = 0;
        descP->readWaits      = 0;

        sprintf(buf, "socket[acc,%d]", sock);
        descP->accMtx           = MCREATE(buf);
        descP->currentAcceptorP = NULL; // currentAcceptor not used
        descP->acceptorsQ.first = NULL;
        descP->acceptorsQ.last  = NULL;

        sprintf(buf, "socket[close,%d]", sock);
        descP->closeMtx         = MCREATE(buf);

        descP->rBufSz           = SOCKET_RECV_BUFFER_SIZE_DEFAULT;
        descP->rCtrlSz          = SOCKET_RECV_CTRL_BUFFER_SIZE_DEFAULT;
        descP->wCtrlSz          = SOCKET_SEND_CTRL_BUFFER_SIZE_DEFAULT;
        descP->iow              = FALSE;
        descP->dbg              = SOCKET_DEBUG_DEFAULT;

        descP->sock             = sock;
        descP->event            = event;

    }

    return descP;
}



/* decrement counters for when a socket is closed */
static
void dec_socket(int domain, int type, int protocol)
{
    MLOCK(data.cntMtx);

    cnt_dec(&data.numSockets, 1);
    
    if (domain == AF_INET)
        cnt_dec(&data.numDomainInet, 1);
#if defined(HAVE_IN6) && defined(AF_INET6)
    else if (domain == AF_INET6)
        cnt_dec(&data.numDomainInet6, 1);
#endif
#if defined(HAVE_SYS_UN_H)
    else if (domain == AF_UNIX)
        cnt_dec(&data.numDomainInet6, 1);
#endif

    if (type == SOCK_STREAM)
        cnt_dec(&data.numTypeStreams, 1);
    else if (type == SOCK_DGRAM)
        cnt_dec(&data.numTypeDGrams, 1);
#ifdef HAVE_SCTP
    else if (type == SOCK_SEQPACKET)
        cnt_dec(&data.numTypeSeqPkgs, 1);
#endif

    if (protocol == IPPROTO_IP)
        cnt_dec(&data.numProtoIP, 1);
    else if (protocol == IPPROTO_TCP)
        cnt_dec(&data.numProtoTCP, 1);
    else if (protocol == IPPROTO_UDP)
        cnt_dec(&data.numProtoUDP, 1);
#if defined(HAVE_SCTP)
    else if (protocol == IPPROTO_SCTP)
        cnt_dec(&data.numProtoSCTP, 1);
#endif

    MUNLOCK(data.cntMtx);
}


/* increment counters for when a socket is opened */
static
void inc_socket(int domain, int type, int protocol)
{
    MLOCK(data.cntMtx);

    cnt_inc(&data.numSockets, 1);
    
    if (domain == AF_INET)
        cnt_inc(&data.numDomainInet, 1);
#if defined(HAVE_IN6) && defined(AF_INET6)
    else if (domain == AF_INET6)
        cnt_inc(&data.numDomainInet6, 1);
#endif
#if defined(HAVE_SYS_UN_H)
    else if (domain == AF_UNIX)
        cnt_inc(&data.numDomainInet6, 1);
#endif

    if (type == SOCK_STREAM)
        cnt_inc(&data.numTypeStreams, 1);
    else if (type == SOCK_DGRAM)
        cnt_inc(&data.numTypeDGrams, 1);
#ifdef HAVE_SCTP
    else if (type == SOCK_SEQPACKET)
        cnt_inc(&data.numTypeSeqPkgs, 1);
#endif

    if (protocol == IPPROTO_IP)
        cnt_inc(&data.numProtoIP, 1);
    else if (protocol == IPPROTO_TCP)
        cnt_inc(&data.numProtoTCP, 1);
    else if (protocol == IPPROTO_UDP)
        cnt_inc(&data.numProtoUDP, 1);
#if defined(HAVE_SCTP)
    else if (protocol == IPPROTO_SCTP)
        cnt_inc(&data.numProtoSCTP, 1);
#endif

    MUNLOCK(data.cntMtx);
}



/* compare_pids - Test if two pids are equal
 *
 */
static
int compare_pids(ErlNifEnv*       env,
                 const ErlNifPid* pid1,
                 const ErlNifPid* pid2)
{
    ERL_NIF_TERM p1 = enif_make_pid(env, pid1);
    ERL_NIF_TERM p2 = enif_make_pid(env, pid2);

    return enif_is_identical(p1, p2);
}


/* ----------------------------------------------------------------------
 *  D e c o d e / E n c o d e   F u n c t i o n s
 * ----------------------------------------------------------------------
 */

/* edomain2domain - convert internal (erlang) domain to (proper) domain
 *
 * Note that only a subset is supported.
 */
static
BOOLEAN_T edomain2domain(int edomain, int* domain)
{
    switch (edomain) {
    case SOCKET_DOMAIN_INET:
        *domain = AF_INET;
        break;

#if defined(HAVE_IN6) && defined(AF_INET6)
    case SOCKET_DOMAIN_INET6:
        *domain = AF_INET6;
        break;
#endif
#ifdef HAVE_SYS_UN_H
    case SOCKET_DOMAIN_LOCAL:
        *domain = AF_UNIX;
        break;
#endif

    default:
        *domain = -1;
        return FALSE;
    }

    return TRUE;
}


/* etype2type - convert internal (erlang) type to (proper) type
 *
 * Note that only a subset is supported.
 */
static
BOOLEAN_T etype2type(int etype, int* type)
{
    switch (etype) {
    case SOCKET_TYPE_STREAM:
        *type = SOCK_STREAM;
        break;

    case SOCKET_TYPE_DGRAM:
        *type = SOCK_DGRAM;
        break;

    case SOCKET_TYPE_RAW:
        *type = SOCK_RAW;
        break;

#ifdef HAVE_SCTP    
    case SOCKET_TYPE_SEQPACKET:
        *type = SOCK_SEQPACKET;
        break;
#endif

    default:
        *type = -1;
        return FALSE;
    }

    return TRUE;
}


/* eproto2proto - convert internal (erlang) protocol to (proper) protocol
 *
 * Note that only a subset is supported.
 */
static
BOOLEAN_T eproto2proto(ErlNifEnv*   env,
                       ERL_NIF_TERM eproto,
                       int*         proto)
{
    if (IS_NUM(env, eproto)) {
        int ep;

        if (!GET_INT(env, eproto, &ep)) {
            *proto = -1;
            return FALSE;
        }

        switch (ep) {
        case SOCKET_PROTOCOL_IP:
            *proto = IPPROTO_IP;
            break;
            
        case SOCKET_PROTOCOL_TCP:
            *proto = IPPROTO_TCP;
            break;
            
        case SOCKET_PROTOCOL_UDP:
            *proto = IPPROTO_UDP;
            break;
            
#if defined(HAVE_SCTP)
        case SOCKET_PROTOCOL_SCTP:
            *proto = IPPROTO_SCTP;
            break;
#endif
            
        case SOCKET_PROTOCOL_ICMP:
            *proto = IPPROTO_ICMP;
            break;
            
        case SOCKET_PROTOCOL_IGMP:
            *proto = IPPROTO_IGMP;
            break;
            
        default:
            *proto = -2;
            return FALSE;
        }
    } else {
        const ERL_NIF_TERM* a;
        int                 sz;

        if (!GET_TUPLE(env, eproto, &sz, &a)) {
            *proto = -3;
            return FALSE;
        }
        
        if (sz != 2) {
            *proto = -4;
            return FALSE;
        }

        if (COMPARE(a[0], esock_atom_raw) != 0) {
            *proto = -5;
            return FALSE;
        }

        if (!GET_INT(env, a[1], proto)) {
            *proto = -6;
            return FALSE;
        }
    }

    return TRUE;
}


#ifdef HAVE_SETNS
 /* emap2netns - extract the netns field from the extra map
 *
 * Note that currently we only support one extra option, the netns.
 */
static
BOOLEAN_T emap2netns(ErlNifEnv* env, ERL_NIF_TERM map, char** netns)
{
    size_t       sz;
    ERL_NIF_TERM key;
    ERL_NIF_TERM value;
    unsigned int len;
    char*        buf;
    int          written;

    /* Note that its acceptable that the extra map is empty */
    if (!enif_get_map_size(env, map, &sz) ||
        (sz != 1)) {
        *netns = NULL;
        return TRUE;
    }

    /* The currently only supported extra option is:  netns */
    key = enif_make_atom(env, "netns");
    if (!GET_MAP_VAL(env, map, key, &value)) {
        *netns = NULL; // Just in case...
        return FALSE;
    }

    /* So far so good. The value should be a string, check. */
    if (!enif_is_list(env, value)) {
        *netns = NULL; // Just in case...
        return FALSE;
    }

    if (!enif_get_list_length(env, value, &len)) {
        *netns = NULL; // Just in case...
        return FALSE;
    }

    if ((buf = MALLOC(len+1)) == NULL) {
        *netns = NULL; // Just in case...
        return FALSE;
    }

    written = enif_get_string(env, value, buf, len+1, ERL_NIF_LATIN1);
    if (written == (len+1)) {
        *netns = buf;
        return TRUE;
    } else {
        *netns = NULL; // Just in case...
        return FALSE;
    }
}
#endif


/* esendflags2sendflags - convert internal (erlang) send flags to (proper)
 * send flags.
 */
static
BOOLEAN_T esendflags2sendflags(unsigned int eflags, int* flags)
{
    unsigned int ef;
    int          tmp = 0;

    for (ef = SOCKET_SEND_FLAG_LOW; ef <= SOCKET_SEND_FLAG_HIGH; ef++) {

        switch (ef) {
#if defined(MSG_CONFIRM)
        case SOCKET_SEND_FLAG_CONFIRM:
            if ((1 << SOCKET_SEND_FLAG_CONFIRM) & eflags)
                tmp |= MSG_CONFIRM;
            break;
#endif

#if defined(MSG_DONTROUTE)
        case SOCKET_SEND_FLAG_DONTROUTE:
            if ((1 << SOCKET_SEND_FLAG_DONTROUTE) & eflags)
                tmp |= MSG_DONTROUTE;
            break;
#endif

#if defined(MSG_EOR)
        case SOCKET_SEND_FLAG_EOR:
            if ((1 << SOCKET_SEND_FLAG_EOR) & eflags)
                tmp |= MSG_EOR;
            break;
#endif

#if defined(MSG_MORE)
        case SOCKET_SEND_FLAG_MORE:
            if ((1 << SOCKET_SEND_FLAG_MORE) & eflags)
                tmp |= MSG_MORE;
            break;
#endif

#if defined(MSG_NOSIGNAL)
        case SOCKET_SEND_FLAG_NOSIGNAL:
            if ((1 << SOCKET_SEND_FLAG_NOSIGNAL) & eflags)
                tmp |= MSG_NOSIGNAL;
            break;
#endif

#if defined(MSG_OOB)
        case SOCKET_SEND_FLAG_OOB:
            if ((1 << SOCKET_SEND_FLAG_OOB) & eflags)
                tmp |= MSG_OOB;
            break;
#endif

        default:
            return FALSE;
        }

    }

    *flags = tmp;

    return TRUE;
}



/* erecvflags2recvflags - convert internal (erlang) send flags to (proper)
 * send flags.
 */
static
BOOLEAN_T erecvflags2recvflags(unsigned int eflags, int* flags)
{
    unsigned int ef;
    int          tmp = 0;

    SGDBG( ("SOCKET", "erecvflags2recvflags -> entry with"
            "\r\n   eflags: %d"
            "\r\n", eflags) );

    for (ef = SOCKET_RECV_FLAG_LOW; ef <= SOCKET_RECV_FLAG_HIGH; ef++) {

        SGDBG( ("SOCKET", "erecvflags2recvflags -> iteration"
                "\r\n   ef:  %d"
                "\r\n   tmp: %d"
                "\r\n", ef, tmp) );

        switch (ef) {
#if defined(MSG_CMSG_CLOEXEC)
        case SOCKET_RECV_FLAG_CMSG_CLOEXEC:
            if ((1 << SOCKET_RECV_FLAG_CMSG_CLOEXEC) & eflags)
                tmp |= MSG_CMSG_CLOEXEC;
            break;
#endif

#if defined(MSG_ERRQUEUE)
        case SOCKET_RECV_FLAG_ERRQUEUE:
            if ((1 << SOCKET_RECV_FLAG_ERRQUEUE) & eflags)
                tmp |= MSG_ERRQUEUE;
            break;
#endif

#if defined(MSG_OOB)
        case SOCKET_RECV_FLAG_OOB:
            if ((1 << SOCKET_RECV_FLAG_OOB) & eflags)
                tmp |= MSG_OOB;
            break;
#endif

            /*
             * <KOLLA>
             *
             * We need to handle this, because it may effect the read algorithm
             *
             * </KOLLA>
             */
#if defined(MSG_PEEK)
        case SOCKET_RECV_FLAG_PEEK:
            if ((1 << SOCKET_RECV_FLAG_PEEK) & eflags)
                tmp |= MSG_PEEK;
            break;
#endif

#if defined(MSG_TRUNC)
        case SOCKET_RECV_FLAG_TRUNC:
            if ((1 << SOCKET_RECV_FLAG_TRUNC) & eflags)
                tmp |= MSG_TRUNC;
            break;
#endif

        default:
            return FALSE;
        }

    }

    *flags = tmp;

    return TRUE;
}



/* eproto2proto - convert internal (erlang) protocol to (proper) protocol
 *
 * Note that only a subset is supported.
 */
static
BOOLEAN_T ehow2how(unsigned int ehow, int* how)
{
     switch (ehow) {
     case SOCKET_SHUTDOWN_HOW_RD:
         *how = SHUT_RD;
         break;

     case SOCKET_SHUTDOWN_HOW_WR:
         *how = SHUT_WR;
         break;

     case SOCKET_SHUTDOWN_HOW_RDWR:
         *how = SHUT_RDWR;
         break;

     default:
         return FALSE;
     }

     return TRUE;
}



#if defined(HAVE_SYS_UN_H) || defined(SO_BINDTODEVICE)
/* strnlen doesn't exist everywhere */
/*
static
size_t my_strnlen(const char *s, size_t maxlen)
{
    size_t i = 0;
    while (i < maxlen && s[i] != '\0')
        i++;
    return i;
}
*/
#endif


/* Send an error closed message to the specified process:
 *
 * This message is for processes that are waiting in the
 * erlang API functions for a select message.
 */
/*
static
char* send_msg_error_closed(ErlNifEnv* env,
                            ErlNifPid* pid)
{
  return send_msg_error(env, atom_closed, pid);
}
*/

/* Send an error message to the specified process:
 * A message in the form:
 *
 *     {error, Reason}
 *
 * This message is for processes that are waiting in the
 * erlang API functions for a select message.
 */
/*
static
char* send_msg_error(ErlNifEnv*   env,
                     ERL_NIF_TERM reason,
                     ErlNifPid*   pid)
{
    ERL_NIF_TERM msg = enif_make_tuple2(env, atom_error, reason);

    return send_msg(env, msg, pid);
}
*/


/* Send an (nif-) abort message to the specified process:
 * A message in the form:
 *
 *     {nif_abort, Ref, Reason}
 *
 * This message is for processes that are waiting in the
 * erlang API functions for a select message.
 */
static
char* send_msg_nif_abort(ErlNifEnv*   env,
                         ERL_NIF_TERM ref,
                         ERL_NIF_TERM reason,
                         ErlNifPid*   pid)
{
    ERL_NIF_TERM msg = MKT3(env, atom_nif_abort, ref, reason);

    return send_msg(env, msg, pid);
}


/* Send a message to the specified process.
 */
static
char* send_msg(ErlNifEnv*   env,
	       ERL_NIF_TERM msg,
	       ErlNifPid*   pid)
{
  if (!enif_send(env, pid, NULL, msg))
    return str_exsend;
  else
    return NULL;
}



/* ----------------------------------------------------------------------
 *  R e q u e s t   Q u e u e   F u n c t i o n s
 * ----------------------------------------------------------------------
 */

/* *** acceptor search for pid ***
 *
 * Search for a pid in the acceptor queue.
 */
static
BOOLEAN_T acceptor_search4pid(ErlNifEnv*        env,
                              SocketDescriptor* descP,
                              ErlNifPid*        pid)
{
    return qsearch4pid(env, &descP->acceptorsQ, pid);
}


/* *** acceptor push ***
 *
 * Push an acceptor onto the acceptor queue.
 * This happens when we already have atleast one current acceptor.
 */
static
ERL_NIF_TERM acceptor_push(ErlNifEnv*        env,
                           SocketDescriptor* descP,
                           ErlNifPid         pid,
                           ERL_NIF_TERM      ref)
{
    SocketRequestQueueElement* e    = MALLOC(sizeof(SocketRequestQueueElement));
    SocketRequestor*           reqP = &e->data;
    
    reqP->pid = pid;
    reqP->ref = enif_make_copy(descP->env, ref);
    
    if (MONP(env, descP, &pid, &reqP->mon) > 0) {
        FREE(reqP);
        return esock_make_error(env, atom_exmon);
    }
    
    qpush(&descP->acceptorsQ, e);
    
    // THIS IS OK => MAKES THE CALLER WAIT FOR ITS TURN
    return esock_make_error(env, esock_atom_eagain);
}


/* *** acceptor pop ***
 *
 * Pop an acceptor from the acceptor queue.
 */
static
BOOLEAN_T acceptor_pop(ErlNifEnv*        env,
                       SocketDescriptor* descP,
                       ErlNifPid*        pid,
                       ErlNifMonitor*    mon,
                       ERL_NIF_TERM*     ref)
{
    SocketRequestQueueElement* e = qpop(&descP->acceptorsQ);

    if (e != NULL) {
        *pid = e->data.pid;
        *mon = e->data.mon;
        *ref = e->data.ref;
        FREE(e);
        return TRUE;
    } else {
        /* (acceptors) Queue was empty */
        // *pid = NULL; we have no null value for pids
        // *mon = NULL; we have no null value for monitors
        *ref = esock_atom_undefined; // Just in case
        return FALSE;
    }
    
}


/* *** acceptor unqueue ***
 *
 * Remove an acceptor from the acceptor queue.
 */
static
BOOLEAN_T acceptor_unqueue(ErlNifEnv*        env,
                           SocketDescriptor* descP,
                           const ErlNifPid*  pid)
{
    return qunqueue(env, &descP->acceptorsQ, pid);
}



/* *** writer search for pid ***
 *
 * Search for a pid in the writer queue.
 */
static
BOOLEAN_T writer_search4pid(ErlNifEnv*        env,
                            SocketDescriptor* descP,
                            ErlNifPid*        pid)
{
    return qsearch4pid(env, &descP->writersQ, pid);
}


/* *** writer push ***
 *
 * Push an writer onto the writer queue.
 * This happens when we already have atleast one current writer.
 */
static
ERL_NIF_TERM writer_push(ErlNifEnv*        env,
                         SocketDescriptor* descP,
                         ErlNifPid         pid,
                         ERL_NIF_TERM      ref)
{
    SocketRequestQueueElement* e    = MALLOC(sizeof(SocketRequestQueueElement));
    SocketRequestor*           reqP = &e->data;
    
    reqP->pid = pid;
    reqP->ref = enif_make_copy(descP->env, ref);
    
    if (MONP(env, descP, &pid, &reqP->mon) > 0) {
        FREE(reqP);
        return esock_make_error(env, atom_exmon);
    }
    
    qpush(&descP->writersQ, e);
    
    // THIS IS OK => MAKES THE CALLER WAIT FOR ITS TURN
    return esock_make_error(env, esock_atom_eagain);
}


/* *** writer pop ***
 *
 * Pop an writer from the writer queue.
 */
static
BOOLEAN_T writer_pop(ErlNifEnv*        env,
                     SocketDescriptor* descP,
                     ErlNifPid*        pid,
                     ErlNifMonitor*    mon,
                     ERL_NIF_TERM*     ref)
{
    SocketRequestQueueElement* e = qpop(&descP->writersQ);

    if (e != NULL) {
        *pid = e->data.pid;
        *mon = e->data.mon;
        *ref = e->data.ref; // At this point the ref has already been copied (env)
        FREE(e);
        return TRUE;
    } else {
        /* (writers) Queue was empty */
        // *pid = NULL; we have no null value for pids
        // *mon = NULL; we have no null value for monitors
        *ref = esock_atom_undefined; // Just in case
        return FALSE;
    }
    
}


/* *** writer unqueue ***
 *
 * Remove an writer from the writer queue.
 */
static
BOOLEAN_T writer_unqueue(ErlNifEnv*        env,
                         SocketDescriptor* descP,
                         const ErlNifPid*  pid)
{
    return qunqueue(env, &descP->writersQ, pid);
}



/* *** reader search for pid ***
 *
 * Search for a pid in the reader queue.
 */
static
BOOLEAN_T reader_search4pid(ErlNifEnv*        env,
                            SocketDescriptor* descP,
                            ErlNifPid*        pid)
{
    return qsearch4pid(env, &descP->readersQ, pid);
}


/* *** reader push ***
 *
 * Push an reader onto the raeder queue.
 * This happens when we already have atleast one current reader.
 */
static
ERL_NIF_TERM reader_push(ErlNifEnv*        env,
                         SocketDescriptor* descP,
                         ErlNifPid         pid,
                         ERL_NIF_TERM      ref)
{
    SocketRequestQueueElement* e    = MALLOC(sizeof(SocketRequestQueueElement));
    SocketRequestor*           reqP = &e->data;
    
    reqP->pid = pid;
    reqP->ref = enif_make_copy(descP->env, ref);
    
    if (MONP(env, descP, &pid, &reqP->mon) > 0) {
        FREE(reqP);
        return esock_make_error(env, atom_exmon);
    }
    
    qpush(&descP->readersQ, e);
    
    // THIS IS OK => MAKES THE CALLER WAIT FOR ITS TURN
    return esock_make_error(env, esock_atom_eagain);
}


/* *** reader pop ***
 *
 * Pop an writer from the reader queue.
 */
static
BOOLEAN_T reader_pop(ErlNifEnv*        env,
                     SocketDescriptor* descP,
                     ErlNifPid*        pid,
                     ErlNifMonitor*    mon,
                     ERL_NIF_TERM*     ref)
{
    SocketRequestQueueElement* e = qpop(&descP->readersQ);

    if (e != NULL) {
        *pid = e->data.pid;
        *mon = e->data.mon;
        *ref = e->data.ref; // At this point the ref has already been copied (env)
        FREE(e);
        return TRUE;
    } else {
        /* (readers) Queue was empty */
        // *pid = NULL; we have no null value for pids
        // *mon = NULL; we have no null value for monitors
        *ref = esock_atom_undefined; // Just in case
        return FALSE;
    }
    
}


/* *** reader unqueue ***
 *
 * Remove an reader from the reader queue.
 */
static
BOOLEAN_T reader_unqueue(ErlNifEnv*        env,
                         SocketDescriptor* descP,
                         const ErlNifPid*  pid)
{
    return qunqueue(env, &descP->readersQ, pid);
}





static
BOOLEAN_T qsearch4pid(ErlNifEnv*          env,
                      SocketRequestQueue* q,
                      ErlNifPid*          pid)
{
    SocketRequestQueueElement* tmp = q->first;
    
    while (tmp != NULL) {
        if (compare_pids(env, &tmp->data.pid, pid))
            return TRUE;
        else
            tmp = tmp->nextP;
    }

    return FALSE;
}


static
void qpush(SocketRequestQueue*        q,
           SocketRequestQueueElement* e)
{
    if (q->first != NULL) {
        q->last->nextP = e;
        q->last        = e;
        e->nextP       = NULL;
    } else {
        q->first = e;
        q->last  = e;
        e->nextP = NULL;
    }
}
 
 
static
SocketRequestQueueElement* qpop(SocketRequestQueue* q)
{
    SocketRequestQueueElement* e = q->first;
    
    if (e != NULL) {
        /* Atleast one element in the queue */
        if (e == q->last) {
            /* Only one element in the queue */
            q->first = q->last = NULL;
        } else {
            /* More than one element in the queue */
            q->first = e->nextP;
        }
    }
    
    return e;
}



static
BOOLEAN_T qunqueue(ErlNifEnv*          env,
                   SocketRequestQueue* q,
                   const ErlNifPid*    pid)
{
    SocketRequestQueueElement* e = q->first;
    SocketRequestQueueElement* p = NULL;

    /* Check if it was one of the waiting acceptor processes */
    while (e != NULL) {
        if (compare_pids(env, &e->data.pid, pid)) {

            /* We have a match */

            if (p != NULL) {
                /* Not the first, but could be the last */
                if (q->last == e) {
                    q->last  = p;
                    p->nextP = NULL;                    
                } else {
                    p->nextP = e->nextP;
                }
                    
            } else {
                /* The first and could also be the last */
                if (q->last == e) {
                    q->last  = NULL;
                    q->first = NULL;
                } else {
                    q->first = e->nextP;
                }
            }

            FREE(e);

            return TRUE;
        }

        /* Try next */
        p = e;
        e = e->nextP;
    }

    return FALSE;
}



/* ----------------------------------------------------------------------
 *  C o u n t e r   F u n c t i o n s
 * ----------------------------------------------------------------------
 */

static
BOOLEAN_T cnt_inc(uint32_t* cnt, uint32_t inc)
{
    BOOLEAN_T wrap;
    uint32_t  max     = 0xFFFFFFFF;
    uint32_t  current = *cnt;

    if ((max - inc) >= current) {
        *cnt += inc;
        wrap  = FALSE;
    } else {
        *cnt = inc - (max - current) - 1;
        wrap = TRUE;
    }

    return (wrap);
}


static
void cnt_dec(uint32_t* cnt, uint32_t dec)
{
    uint32_t current = *cnt;

    if (dec > current)
        *cnt = 0; // The counter cannot be < 0 so this is the best we can do...
    else
        *cnt -= dec;

    return;
}



/* ----------------------------------------------------------------------
 *  C a l l b a c k   F u n c t i o n s
 * ----------------------------------------------------------------------
 */

/* =========================================================================
 * socket_dtor - Callback function for resource destructor
 *
 */
static
void socket_dtor(ErlNifEnv* env, void* obj)
{
  SocketDescriptor* descP = (SocketDescriptor*) obj;

  MDESTROY(descP->writeMtx);
  MDESTROY(descP->readMtx);
  MDESTROY(descP->accMtx);
  MDESTROY(descP->closeMtx);
}


/* =========================================================================
 * socket_stop - Callback function for resource stop
 *
 * When the socket is stopped, we need to inform:
 *
 *     * the controlling process
 *     * the current writer and any waiting writers
 *     * the current reader and any waiting readers
 *     * the current acceptor and any waiting acceptor
 *
 * Also, make sure no process gets the message twice
 * (in case it is, for instance, both controlling process
 * and a writer).
 *
 * <KOLLA>
 *
 * We do not handle linger-issues yet! So anything in the out
 * buffers will be left for the OS to solve...
 * Do we need a special "close"-thread? Dirty scheduler?
 *
 * What happens if we are "stopped" for another reason then 'close'?
 * For instance, down?
 *
 * </KOLLA>
 */
static
void socket_stop(ErlNifEnv* env, void* obj, int fd, int is_direct_call)
{
    SocketDescriptor* descP = (SocketDescriptor*) obj;
    
    SSDBG( descP,
           ("SOCKET", "socket_stop -> entry when"
            "\r\n   sock:           %d (%d)"
            "\r\n   Is Direct Call: %s"
            "\r\n", descP->sock, fd, B2S(is_direct_call)) );
    
    MLOCK(descP->writeMtx);
    MLOCK(descP->readMtx);
    MLOCK(descP->accMtx);
    MLOCK(descP->closeMtx);
    
    
    descP->state      = SOCKET_STATE_CLOSING; // Just in case...???
    descP->isReadable = FALSE;
    descP->isWritable = FALSE;
    
    
    /* We should check that we actually have a monitor.
     * This *should* be done with a "NULL" monitor value,
     * which there currently is none...
     */
    DEMONP(env, descP, &descP->ctrlMon);
    
    if (descP->currentWriterP != NULL) {
        /* We have a (current) writer and *may* therefor also have
         * writers waiting.
         */

        if (!compare_pids(env,
                          &descP->closerPid,
                          &descP->currentWriter.pid) &&
            send_msg_nif_abort(env,
                               descP->currentWriter.ref,
                               atom_closed,
                               &descP->currentWriter.pid) != NULL) {
            /* Shall we really do this? 
             * This happens if the controlling process has been killed!
             */
            esock_warning_msg("Failed sending abort (%T) message to "
                              "current writer %T\r\n",
                              descP->currentWriter.ref,
                              descP->currentWriter.pid);
        }
        
        /* And also deal with the waiting writers (in the same way) */
        inform_waiting_procs(env, descP, &descP->writersQ, TRUE, atom_closed);
    }
    
    if (descP->currentReaderP != NULL) {
        
        /* We have a (current) reader and *may* therefor also have
         * readers waiting.
         */
        
        if (!compare_pids(env,
                          &descP->closerPid,
                          &descP->currentReader.pid) &&
            send_msg_nif_abort(env,
                               descP->currentReader.ref,
                               atom_closed,
                               &descP->currentReader.pid) != NULL) {
            /* Shall we really do this? 
             * This happens if the controlling process has been killed!
             */
            esock_warning_msg("Failed sending abort (%T) message to "
                              "current reader %T\r\n",
                              descP->currentReader.ref,
                              descP->currentReader.pid);
        }
        
        /* And also deal with the waiting readers (in the same way) */
        inform_waiting_procs(env, descP, &descP->readersQ, TRUE, atom_closed);
    }
    
    if (descP->currentAcceptorP != NULL) {
        /* We have a (current) acceptor and *may* therefor also have
         * acceptors waiting.
         */
        
        if (!compare_pids(env,
                          &descP->closerPid,
                          &descP->currentAcceptor.pid) &&
            send_msg_nif_abort(env,
                               descP->currentAcceptor.ref,
                               atom_closed,
                               &descP->currentAcceptor.pid) != NULL) {
            /* Shall we really do this? 
             * This happens if the controlling process has been killed!
             */
            esock_warning_msg("Failed sending abort (%T) message to "
                              "current acceptor %T\r\n",
                              descP->currentAcceptor.ref,
                              descP->currentAcceptor.pid);
        }
        
        /* And also deal with the waiting acceptors (in the same way) */
        inform_waiting_procs(env, descP, &descP->acceptorsQ, TRUE, atom_closed);
    }
    
    
    if (descP->sock != INVALID_SOCKET) {
        
        /*
         * <KOLLA>
         *
         * WE NEED TO CHECK IF THIS OPERATION IS TRIGGERED
         * LOCALLY (VIA A CALL TO CLOSE) OR REMOTELLY
         * (VIA I.E. ECONSRESET).
         *
         * </KOLLA>
         */
        
        if (descP->closeLocal) {
            
            /* +++ send close message to the waiting process +++
             *
             *           {close, CloseRef}
             *
             * <KOLLA>
             *
             * WHAT HAPPENS IF THE RECEIVER HAS DIED IN THE MEANTIME????
             *
             * </KOLLA>
             */
            
            send_msg(env, MKT2(env, atom_close, descP->closeRef), &descP->closerPid);
            
            DEMONP(env, descP, &descP->closerMon);
            
        } else {
            
            /*
             * <KOLLA>
             *
             * ABORT?
             *
             * </KOLLA>
             */
        }
    }
    
    
    MUNLOCK(descP->closeMtx);
    MUNLOCK(descP->accMtx);
    MUNLOCK(descP->readMtx);
    MUNLOCK(descP->writeMtx);

    SSDBG( descP,
           ("SOCKET", "socket_stop -> done (%d, %d)\r\n", descP->sock, fd) );
    
}


/* This function traverse the queue and sends the specified
 * nif_abort message with the specified reason to each member,
 * and if the 'free' argument is TRUE, the queue will be emptied.
 */
static
void inform_waiting_procs(ErlNifEnv*          env,
                          SocketDescriptor*   descP,
                          SocketRequestQueue* q,
                          BOOLEAN_T           free,
                          ERL_NIF_TERM        reason)
{
    SocketRequestQueueElement* currentP = q->first;
    SocketRequestQueueElement* nextP;

    while (currentP != NULL) {

        /* <KOLLA>
         *
         * Should we inform anyone if we fail to demonitor?
         * NOT SURE WHAT THAT WOULD REPRESENT AND IT IS NOT
         * IMPORTANT IN *THIS* CASE, BUT ITS A FUNDAMENTAL OP...
         *
         * </KOLLA>
         */

        SSDBG( descP,
               ("SOCKET", "inform_waiting_procs -> abort %T (%T)\r\n",
                currentP->data.ref, currentP->data.pid) );

        ESOCK_ASSERT( (NULL == send_msg_nif_abort(env,
                                                  currentP->data.ref,
                                                  reason,
                                                  &currentP->data.pid)) );
        DEMONP(env, descP, &currentP->data.mon);
        nextP = currentP->nextP;
        if (free) FREE(currentP);
        currentP = nextP;
    }

    if (free) {
        q->first = NULL;
        q->last  = NULL;
    }
}



/* =========================================================================
 * socket_down - Callback function for resource down (monitored processes)
 *
 */
static
void socket_down(ErlNifEnv*           env,
                 void*                obj,
                 const ErlNifPid*     pid,
                 const ErlNifMonitor* mon)
{
    SocketDescriptor* descP = (SocketDescriptor*) obj;

    SSDBG( descP, ("SOCKET", "socket_down -> entry with"
                   "\r\n   sock: %d"
                   "\r\n   pid:  %T"
                   "\r\n", descP->sock, *pid) );

    
    if (compare_pids(env, &descP->ctrlPid, pid)) {
        /* We don't bother with the queue cleanup here - 
         * we leave it to the stop callback function.
         */

        descP->state      = SOCKET_STATE_CLOSING;
        descP->closeLocal = TRUE;
        descP->closerPid  = *pid;
        descP->closerMon  = *mon;
        descP->closeRef   = MKREF(env);
        enif_select(env, descP->sock, (ERL_NIF_SELECT_STOP),
                    descP, NULL, descP->closeRef);

    } else {
    
        /* check all operation queue(s): acceptor, writer and reader. */
        
        MLOCK(descP->accMtx);
        if (descP->currentAcceptorP != NULL)
            socket_down_acceptor(env, descP, pid);
        MUNLOCK(descP->accMtx);
        
        MLOCK(descP->writeMtx);
        if (descP->currentWriterP != NULL)
            socket_down_writer(env, descP, pid);
        MUNLOCK(descP->writeMtx);
        
        MLOCK(descP->readMtx);
        if (descP->currentReaderP != NULL)
            socket_down_reader(env, descP, pid);
        MUNLOCK(descP->readMtx);

    }
    
    SSDBG( descP, ("SOCKET", "socket_down -> done\r\n") );

}



/* *** socket_down_acceptor ***
 *
 * Check and then handle a downed acceptor process.
 *
 */
static
void socket_down_acceptor(ErlNifEnv*        env,
                          SocketDescriptor* descP,
                          const ErlNifPid*  pid)
{
    if (compare_pids(env, &descP->currentAcceptor.pid, pid)) {
        
        SSDBG( descP, ("SOCKET",
                       "socket_down_acceptor -> "
                       "current acceptor - try pop the queue\r\n") );
        
        if (acceptor_pop(env, descP,
                         &descP->currentAcceptor.pid,
                         &descP->currentAcceptor.mon,
                         &descP->currentAcceptor.ref)) {
            int res;
            
            /* There was another one, so we will still be in accepting state */
            
            SSDBG( descP, ("SOCKET",
                           "socket_down_acceptor -> new (active) acceptor: "
                           "\r\n   pid: %T"
                           "\r\n   ref: %T"
                           "\r\n",
                           descP->currentAcceptor.pid,
                           descP->currentAcceptor.ref) );
            
            if ((res = enif_select(env,
                                   descP->sock,
                                   (ERL_NIF_SELECT_READ),
                                   descP,
                                   &descP->currentAcceptor.pid,
                                   descP->currentAcceptor.ref) < 0)) {
                
                esock_warning_msg("Failed select (%d) for new acceptor "
                                  "after current (%T) died\r\n",
                                  res, *pid);
                
            }
            
        } else {
            
            SSDBG( descP, ("SOCKET",
                           "socket_down_acceptor -> no active acceptor\r\n") );
            
            descP->currentAcceptorP = NULL;
            descP->state            = SOCKET_STATE_LISTENING;
        }
        
    } else {
        
        /* Maybe unqueue one of the waiting acceptors */
        
        SSDBG( descP, ("SOCKET",
                       "socket_down_acceptor -> "
                       "not current acceptor - maybe a waiting acceptor\r\n") );
        
        acceptor_unqueue(env, descP, pid);
    }
}




/* *** socket_down_writer ***
 *
 * Check and then handle a downed writer process.
 *
 */
static
void socket_down_writer(ErlNifEnv*        env,
                        SocketDescriptor* descP,
                        const ErlNifPid*  pid)
{
    if (compare_pids(env, &descP->currentWriter.pid, pid)) {
        
        SSDBG( descP, ("SOCKET",
                       "socket_down_writer -> "
                       "current writer - try pop the queue\r\n") );
        
        if (writer_pop(env, descP,
                       &descP->currentWriter.pid,
                       &descP->currentWriter.mon,
                       &descP->currentWriter.ref)) {
            int res;
            
            /* There was another one */
            
            SSDBG( descP, ("SOCKET", "socket_down_writer -> new (current) writer: "
                           "\r\n   pid: %T"
                           "\r\n   ref: %T"
                           "\r\n",
                           descP->currentWriter.pid,
                           descP->currentWriter.ref) );
            
            if ((res = enif_select(env,
                                   descP->sock,
                                   (ERL_NIF_SELECT_WRITE),
                                   descP,
                                   &descP->currentWriter.pid,
                                   descP->currentWriter.ref) < 0)) {
                
                esock_warning_msg("Failed select (%d) for new writer "
                                  "after current (%T) died\r\n",
                                  res, *pid);
                
            }
            
        } else {
            
            SSDBG( descP, ("SOCKET",
                           "socket_down_writer -> no active writer\r\n") );
            
            descP->currentWriterP = NULL;
        }
        
    } else {
        
        /* Maybe unqueue one of the waiting writer(s) */
        
        SSDBG( descP, ("SOCKET",
                       "socket_down_writer -> "
                       "not current writer - maybe a waiting writer\r\n") );
        
        writer_unqueue(env, descP, pid);
    }
}




/* *** socket_down_reader ***
 *
 * Check and then handle a downed reader process.
 *
 */
static
void socket_down_reader(ErlNifEnv*        env,
                        SocketDescriptor* descP,
                        const ErlNifPid*  pid)
{
    if (compare_pids(env, &descP->currentReader.pid, pid)) {
        
        SSDBG( descP, ("SOCKET",
                       "socket_down_reader -> "
                       "current reader - try pop the queue\r\n") );
        
        if (reader_pop(env, descP,
                       &descP->currentReader.pid,
                       &descP->currentReader.mon,
                       &descP->currentReader.ref)) {
            int res;
            
            /* There was another one */
            
            SSDBG( descP, ("SOCKET", "socket_down_reader -> new (current) reader: "
                           "\r\n   pid: %T"
                           "\r\n   ref: %T"
                           "\r\n",
                           descP->currentReader.pid,
                           descP->currentReader.ref) );
            
            if ((res = enif_select(env,
                                   descP->sock,
                                   (ERL_NIF_SELECT_READ),
                                   descP,
                                   &descP->currentReader.pid,
                                   descP->currentReader.ref) < 0)) {
                
                esock_warning_msg("Failed select (%d) for new reader "
                                  "after current (%T) died\r\n",
                                  res, *pid);
                
            }
            
        } else {
            
            SSDBG( descP, ("SOCKET",
                           "socket_down_reader -> no active reader\r\n") );
            
            descP->currentReaderP = NULL;
        }
        
    } else {
        
        /* Maybe unqueue one of the waiting reader(s) */
        
        SSDBG( descP, ("SOCKET",
                       "socket_down_reader -> "
                       "not current reader - maybe a waiting reader\r\n") );
        
        reader_unqueue(env, descP, pid);
    }
}



/* ----------------------------------------------------------------------
 *  L o a d / u n l o a d / u p g r a d e   F u n c t i o n s
 * ----------------------------------------------------------------------
 */

static
ErlNifFunc socket_funcs[] =
{
    // Some utility functions
    {"nif_info",      0, nif_info, 0},
    // {"nif_debug",      1, nif_debug_, 0},

    // The proper "socket" interface
    // nif_open/1 is used when we already have a file descriptor
    // {"nif_open",                1, nif_open, 0},
    {"nif_open",                4, nif_open, 0},
    {"nif_bind",                2, nif_bind, 0},
    {"nif_connect",             2, nif_connect, 0},
    {"nif_listen",              2, nif_listen, 0},
    {"nif_accept",              2, nif_accept, 0},
    {"nif_send",                4, nif_send, 0},
    {"nif_sendto",              5, nif_sendto, 0},
    {"nif_sendmsg",             4, nif_sendmsg, 0},
    {"nif_recv",                4, nif_recv, 0},
    {"nif_recvfrom",            4, nif_recvfrom, 0},
    {"nif_recvmsg",             5, nif_recvmsg, 0},
    {"nif_close",               1, nif_close, 0},
    {"nif_shutdown",            2, nif_shutdown, 0},
    {"nif_setopt",              5, nif_setopt, 0},
    {"nif_getopt",              4, nif_getopt, 0},
    {"nif_sockname",            1, nif_sockname, 0},
    {"nif_peername",            1, nif_peername, 0},

    /* Misc utility functions */

    /* "Extra" functions to "complete" the socket interface.
     * For instance, the function nif_finalize_connection
     * is called after the connect *select* has "completed".
     */
    {"nif_finalize_connection", 1, nif_finalize_connection, 0},
    {"nif_cancel",              3, nif_cancel, 0},
    {"nif_finalize_close",      1, nif_finalize_close, ERL_NIF_DIRTY_JOB_IO_BOUND}
};


static
BOOLEAN_T extract_debug(ErlNifEnv*   env,
                        ERL_NIF_TERM map)
{
    /*
     * We need to do this here since the "proper" atom has not been
     * created when this function is called.
     */
    ERL_NIF_TERM debug = MKA(env, "debug");
    
    return esock_extract_bool_from_map(env, map, debug, SOCKET_NIF_DEBUG_DEFAULT);
}

static
BOOLEAN_T extract_iow(ErlNifEnv*   env,
                      ERL_NIF_TERM map)
{
    /*
     * We need to do this here since the "proper" atom has not been
     * created when this function is called.
     */
    ERL_NIF_TERM iow = MKA(env, "iow");
    
    return esock_extract_bool_from_map(env, map, iow, SOCKET_NIF_IOW_DEFAULT);
}



/* =======================================================================
 * load_info - A map of misc info (e.g global debug)
 */

static
int on_load(ErlNifEnv* env, void** priv_data, ERL_NIF_TERM load_info)
{
    data.dbg = extract_debug(env, load_info);
    data.iow = extract_iow(env, load_info);

    /* +++ Global Counters +++ */
    data.cntMtx         = MCREATE("socket[gcnt]");
    data.numSockets     = 0;
    data.numTypeDGrams  = 0;
    data.numTypeStreams = 0;
    data.numTypeSeqPkgs = 0;
    data.numDomainLocal = 0;
    data.numDomainInet  = 0;
    data.numDomainInet6 = 0;
    data.numProtoIP     = 0;
    data.numProtoTCP    = 0;
    data.numProtoUDP    = 0;
    data.numProtoSCTP   = 0;

    /* +++ Misc atoms +++ */
    atom_adaptation_layer    = MKA(env, str_adaptation_layer);
    atom_address             = MKA(env, str_address);
    atom_association         = MKA(env, str_association);
    atom_assoc_id            = MKA(env, str_assoc_id);
    atom_authentication      = MKA(env, str_authentication);
    atom_bool                = MKA(env, str_bool);
    atom_close               = MKA(env, str_close);
    atom_closed              = MKA(env, str_closed);
    atom_closing             = MKA(env, str_closing);
    atom_cookie_life         = MKA(env, str_cookie_life);
    atom_data_in             = MKA(env, str_data_in);
    atom_do                  = MKA(env, str_do);
    atom_dont                = MKA(env, str_dont);
    atom_exclude             = MKA(env, str_exclude);
    atom_false               = MKA(env, str_false);
    atom_global_counters     = MKA(env, str_global_counters);
    atom_in4_sockaddr        = MKA(env, str_in4_sockaddr);
    atom_in6_sockaddr        = MKA(env, str_in6_sockaddr);
    atom_include             = MKA(env, str_include);
    atom_initial             = MKA(env, str_initial);
    atom_int                 = MKA(env, str_int);
    atom_interface           = MKA(env, str_interface);
    atom_iow                 = MKA(env, str_iow);
    atom_local_rwnd          = MKA(env, str_local_rwnd);
    atom_max                 = MKA(env, str_max);
    atom_max_attempts        = MKA(env, str_max_attempts);
    atom_max_init_timeo      = MKA(env, str_max_init_timeo);
    atom_max_instreams       = MKA(env, str_max_instreams);
    atom_max_rxt             = MKA(env, str_max_rxt);
    atom_min                 = MKA(env, str_min);
    atom_mode                = MKA(env, str_mode);
    atom_multiaddr           = MKA(env, str_multiaddr);
    atom_nif_abort           = MKA(env, str_nif_abort);
    atom_null                = MKA(env, str_null);
    atom_num_dinet           = MKA(env, str_num_dinet);
    atom_num_dinet6          = MKA(env, str_num_dinet6);
    atom_num_dlocal          = MKA(env, str_num_dlocal);
    atom_num_outstreams      = MKA(env, str_num_outstreams);
    atom_num_peer_dests      = MKA(env, str_num_peer_dests);
    atom_num_pip             = MKA(env, str_num_pip);
    atom_num_psctp           = MKA(env, str_num_psctp);
    atom_num_ptcp            = MKA(env, str_num_ptcp);
    atom_num_pudp            = MKA(env, str_num_pudp);
    atom_num_sockets         = MKA(env, str_num_sockets);
    atom_num_tdgrams         = MKA(env, str_num_tdgrams);
    atom_num_tseqpkgs        = MKA(env, str_num_tseqpkgs);
    atom_num_tstreams        = MKA(env, str_num_tstreams);
    atom_partial_delivery    = MKA(env, str_partial_delivery);
    atom_peer_rwnd           = MKA(env, str_peer_rwnd);
    atom_peer_error          = MKA(env, str_peer_error);
    atom_probe               = MKA(env, str_probe);
    atom_select              = MKA(env, str_select);
    atom_sender_dry          = MKA(env, str_sender_dry);
    atom_send_failure        = MKA(env, str_send_failure);
    atom_shutdown            = MKA(env, str_shutdown);
    atom_slist               = MKA(env, str_slist);
    atom_sourceaddr          = MKA(env, str_sourceaddr);
    atom_timeout             = MKA(env, str_timeout);
    atom_true                = MKA(env, str_true);
    atom_want                = MKA(env, str_want);

    /* Global atom(s) */
    esock_atom_accept      = MKA(env, "accept");
    esock_atom_addr        = MKA(env, "addr");
    esock_atom_any         = MKA(env, "any");
    esock_atom_connect     = MKA(env, "connect");
    esock_atom_credentials = MKA(env, "credentials");
    esock_atom_ctrl        = MKA(env, "ctrl");
    esock_atom_ctrunc      = MKA(env, "ctrunc");
    esock_atom_data        = MKA(env, "data");
    esock_atom_debug       = MKA(env, "debug");
    esock_atom_dgram       = MKA(env, "dgram");
    esock_atom_eor         = MKA(env, "eor");
    esock_atom_error       = MKA(env, "error");
    esock_atom_errqueue    = MKA(env, "errqueue");
    esock_atom_false       = MKA(env, "false");
    esock_atom_family      = MKA(env, "family");
    esock_atom_flags       = MKA(env, "flags");
    esock_atom_flowinfo    = MKA(env, "flowinfo");
    esock_atom_ifindex     = MKA(env, "ifindex");
    esock_atom_inet        = MKA(env, "inet");
    esock_atom_inet6       = MKA(env, "inet6");
    esock_atom_iov         = MKA(env, "iov");
    esock_atom_ip          = MKA(env, "ip");
    esock_atom_ipv6        = MKA(env, "ipv6");
    esock_atom_level       = MKA(env, "level");
    esock_atom_local       = MKA(env, "local");
    esock_atom_loopback    = MKA(env, "loopback");
    esock_atom_lowdelay    = MKA(env, "lowdelay");
    esock_atom_mincost     = MKA(env, "mincost");
    esock_atom_not_found   = MKA(env, "not_found");
    esock_atom_ok          = MKA(env, "ok");
    esock_atom_oob         = MKA(env, "oob");
    esock_atom_origdstaddr = MKA(env, "origdstaddr");
    esock_atom_path        = MKA(env, "path");
    esock_atom_pktinfo     = MKA(env, "pktinfo");
    esock_atom_port        = MKA(env, "port");
    esock_atom_protocol    = MKA(env, "protocol");
    esock_atom_raw         = MKA(env, "raw");
    esock_atom_rdm         = MKA(env, "rdm");
    esock_atom_recv        = MKA(env, "recv");
    esock_atom_recvfrom    = MKA(env, "recvfrom");
    esock_atom_recvmsg     = MKA(env, "recvmsg");
    esock_atom_reliability = MKA(env, "reliability");
    esock_atom_rights      = MKA(env, "rights");
    esock_atom_scope_id    = MKA(env, "scope_id");
    esock_atom_sctp        = MKA(env, "sctp");
    esock_atom_sec         = MKA(env, "sec");
    esock_atom_select_sent = MKA(env, "select_sent");
    esock_atom_send        = MKA(env, "send");
    esock_atom_sendmsg     = MKA(env, "sendmsg");
    esock_atom_sendto      = MKA(env, "sendto");
    esock_atom_seqpacket   = MKA(env, "seqpacket");
    esock_atom_socket      = MKA(env, "socket");
    esock_atom_spec_dst    = MKA(env, "spec_dst");
    esock_atom_stream      = MKA(env, "stream");
    esock_atom_tcp         = MKA(env, "tcp");
    esock_atom_throughput  = MKA(env, "throughput");
    esock_atom_timestamp   = MKA(env, "timestamp");
    esock_atom_tos         = MKA(env, "tos");
    esock_atom_true        = MKA(env, "true");
    esock_atom_trunc       = MKA(env, "trunc");
    esock_atom_ttl         = MKA(env, "ttl");
    esock_atom_type        = MKA(env, "type");
    esock_atom_udp         = MKA(env, "udp");
    esock_atom_undefined   = MKA(env, "undefined");
    esock_atom_unknown     = MKA(env, "unknown");
    esock_atom_usec        = MKA(env, "usec");

    /* Global error codes */
    esock_atom_eafnosupport = MKA(env, ESOCK_STR_EAFNOSUPPORT);
    esock_atom_eagain       = MKA(env, ESOCK_STR_EAGAIN);
    esock_atom_einval       = MKA(env, ESOCK_STR_EINVAL);

    /* Error codes */
    atom_eisconn      = MKA(env, str_eisconn);
    atom_enotclosing  = MKA(env, str_enotclosing);
    atom_enotconn     = MKA(env, str_enotconn);
    atom_exalloc      = MKA(env, str_exalloc);
    atom_exbadstate   = MKA(env, str_exbadstate);
    atom_exbusy       = MKA(env, str_exbusy);
    atom_exmon        = MKA(env, str_exmon);
    atom_exself       = MKA(env, str_exself);
    atom_exsend       = MKA(env, str_exsend);

    // For storing "global" things...
    // data.env       = enif_alloc_env(); // We should really check
    // data.version   = MKA(env, ERTS_VERSION);
    // data.buildDate = MKA(env, ERTS_BUILD_DATE);

    sockets = enif_open_resource_type_x(env,
                                        "sockets",
                                        &socketInit,
                                        ERL_NIF_RT_CREATE,
                                        NULL);

    return !sockets;
}

ERL_NIF_INIT(socket, socket_funcs, on_load, NULL, NULL, NULL)

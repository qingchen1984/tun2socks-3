#ifndef TUN2SOCKS_TUN_H__
#define TUN2SOCKS_TUN_H__

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>

#include <net/if_utun.h>
#include <net/if.h>

#include <sys/kern_control.h>
#include <sys/kern_event.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

#define IP4_HDRLEN 20         // IPv4 header length
#define MTU 1500

int open_utun(u_int32_t);
void printSome(unsigned char*, int, char*);
uint16_t cksum(const uint8_t * _data, int len);

#endif // TUN2SOCKS_TUN_H__
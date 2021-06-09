#ifndef TUN2SOCKS_LWIP_CB_H__
#define TUN2SOCKS_LWIP_CB_H__

#include <lwip/timeouts.h>
#include <lwip/init.h>
#include <lwip/tcp.h>
#include <lwip/udp.h>
#include <lwip/sys.h>
#include <arpa/inet.h>
#include <uv.h>

#include "tun2socks.h"
#include "logger.h"

class LWIPCallback {
public:
	LWIPCallback() {};
	~LWIPCallback() {};
public:
	static err_t tcp_accept_cb(void*, struct tcp_pcb*, err_t);
	static err_t tcp_recv_cb(void*, struct tcp_pcb*, struct pbuf*, err_t);
	static err_t tcp_sent_cb(void*, struct tcp_pcb*, u16_t);
	static void tcp_error_cb(void*, err_t);
	static err_t tcp_poll_cb(void*, struct tcp_pcb*);
	static void udp_recv_cb(void*, struct udp_pcb*, struct pbuf*, const ip_addr_t*, u16_t, const ip_addr_t*, u16_t);
private:
	LWIPCallback(const LWIPCallback&);
	LWIPCallback& operator=(const LWIPCallback&);
};


#endif // TUN2SOCKS_LWIP_CB_H__
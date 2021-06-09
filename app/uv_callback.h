#ifndef TUN2SOCKS_UV_CB_H__
#define TUN2SOCKS_UV_CB_H__

#include <lwip/timeouts.h>
#include <lwip/init.h>
#include <lwip/tcp.h>
#include <lwip/udp.h>
#include <lwip/sys.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <uv.h>

#include "lwip_callback.h"
#include "tun2socks.h"
#include "logger.h"

class UVCallback {
public:
	UVCallback() {};
	~UVCallback() {};
public:
	static void server_connected_cb(uv_connect_t*, int);
	static void server_shutdown_cb(uv_shutdown_t*, int);
	static void server_handle_close_cb(uv_handle_t*);
private:
	UVCallback(const UVCallback&);
	UVCallback& operator=(const UVCallback&);
};

#endif // TUN2SOCKS_UV_CB_H__
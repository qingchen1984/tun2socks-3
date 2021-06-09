#include "lwip_callback.h"
#include "uv_callback.h"

err_t LWIPCallback::tcp_accept_cb(void* arg, struct tcp_pcb* newpcb, err_t err) {
	if(err != ERR_OK) {
		error("tcp_accept_cb() failed, errno: %d", err);
		return err;
	}

	info("LWIPCallback::tcp_accept_cb(): create connection from `%s:%d`-->`%s:%d`, &tcp_pcb: %p", 
		ipaddr_ntoa(&newpcb->remote_ip), (u16_t)newpcb->remote_port, 
		ipaddr_ntoa(&newpcb->local_ip), (u16_t)newpcb->local_port,
		newpcb);

	// 设置LWIP回调
	// todo：将设置lwip callback function移到连接建立成功之后，不知道会不会有什么影响
	/*
	tcp_recv(newpcb, LWIPCallback::tcp_recv_cb);
	tcp_sent(newpcb, LWIPCallback::tcp_sent_cb);
	tcp_err(newpcb, LWIPCallback::tcp_error_cb);
	tcp_poll(newpcb, LWIPCallback::tcp_poll_cb, TCP_POLL_INTERVAL);
	*/

	// 连接信息
	int ret = 0;
	uv_loop_t* LOOP = uv_default_loop();
	struct sockaddr_in addr;
	// todo：以下两个指针的释放时机还要再分析
	uv_tcp_t* uv_client = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
	uv_connect_t* uv_connect = (uv_connect_t*)malloc(sizeof(uv_connect_t));

	uv_tcp_init(LOOP, uv_client);
	// uv_tcp_keepalive(uv_client, 1, 60);
	ret = uv_ip4_addr("47.88.22.87", (u16_t)newpcb->local_port, &addr);
	if (ret != 0) {
		free(uv_client);
		free(uv_connect);
		error("failed to init server address, err: %s", uv_strerror(ret));
		return -1;
	}

	uv_connect->data = newpcb;
	ret = uv_tcp_connect(uv_connect, uv_client, (const struct sockaddr*)&addr, UVCallback::server_connected_cb);
	if (ret != 0) {
		free(uv_client);
		free(uv_connect);
		error("failed to connect to server, err: %s", uv_strerror(ret));
		return -1;
	}

	return ERR_OK;
}

err_t LWIPCallback::tcp_recv_cb(void* arg, struct tcp_pcb* tcppcb, struct pbuf* p, err_t err) {
	info("LWIPCallback::tcp_recv_cb(): `%s:%d`->`%s:%d`, err: %d, &tcp_pcb: %p", 
		ipaddr_ntoa(&tcppcb->remote_ip), (u16_t)tcppcb->remote_port, 
		ipaddr_ntoa(&tcppcb->local_ip), (u16_t)tcppcb->local_port, err, tcppcb);

	if (err != ERR_OK && err != ERR_ABRT) {
		error("call LWIPCallback::tcp_recv_cb() failed, errno: %d", err);
		return err;
	}

	err_t ret = ERR_OK;
	bool should_free_pbuf = true;

	// 从会话中查询uv连接句柄信息
	char key[64];
	sprintf(key, "0:%d@%s:%d", tcppcb->remote_port, ipaddr_ntoa(&tcppcb->local_ip), tcppcb->local_port);
	
	info("query session, key is: %s", key);

	// alex：如果和远程服务器的连接建立没有成功，会话信息为空
	if (Tun2Socks::sessions.find(key) == Tun2Socks::sessions.end()) {
		error("session is empty, key is `%s`, send `FIN` to client...", key);
		tcp_abort(tcppcb); //让lwip发送fin
		if(p != NULL) { pbuf_free(p); }
		return ERR_ABRT;
	}

	session_t session = Tun2Socks::sessions[key];

	// 如果pbuf为空，说明客户端已经断开连接，参考lwip/tpc.c文件中关于`tcp_recv()`的注释
	if (p == NULL) {
		info("client abort the connection `%s`...", key);
		// 关闭远程服务器连接，首先清除session，那么即使有新的client使用相同的五元组连接，也会建立新的session
		// todo：可以增加一个`正在释放的会话`的记录表
		Tun2Socks::sessions.erase(key);
		tcp_arg(tcppcb, NULL);
    	tcp_recv(tcppcb, NULL);
    	tcp_sent(tcppcb, NULL);
    	tcp_err(tcppcb, NULL);
    	tcp_poll(tcppcb, NULL, 0);
		// Aborts the connection by sending a RST (reset) segment to the remote host. 
		// The pcb is deallocated. This function never fails.
 		// ATTENTION: When calling this from one of the TCP callbacks, make sure you always return ERR_ABRT 
 		// (and never return ERR_ABRT otherwise or you will risk accessing deallocated memory or memory leaks)
 		// todo：是否还要调用tcp_close()
		tcp_abort(tcppcb);
		if(p != NULL) { pbuf_free(p); }
		assert(session.uv_conn != NULL);
		// todo：这里是否应该用malloc
		uv_shutdown_t *req = (uv_shutdown_t *)mem_malloc(sizeof(uv_shutdown_t));
    	ret = uv_shutdown(req, (uv_stream_t*)session.uv_conn->handle, UVCallback::server_shutdown_cb);
    	if (ret != 0) {
    		error("uv_shutdown() failed, errno: %d", ret);
    	}
		return ERR_ABRT;
	}

	/*
	void *buf;
	if (p->tot_len == p->len) {
		buf = p->payload;
		rerr = conn->receive(buf, p->tot_len);
	} else {
		buf = mem_malloc(p->tot_len);
		pbuf_copy_partial(p, buf, p->tot_len, 0);
		rerr = conn->receive(buf, p->tot_len);
		mem_free(buf);
	}
	*/
	/*
	if(rerr != LWIP_ERR_NULL) {
		switch (rerr) {
			case LWIP_ERR_ABRT:
				LOG("tcpRecvFn:ERR_ABRT");
				ret = ERR_ABRT;
				break;
			case LWIP_ERR_OK:
				LOG("tcpRecvFn:ERR_OK");
				ret = ERR_OK;
				break;
			case LWIP_ERR_CONN:
				should_free_pbuf = false;
				// Tell lwip we can't receive data at the moment,
				// lwip will store it and try again later.
				LOG("tcpRecvFn:ERR_CONN");
				ret = ERR_CONN;
				break;
			case LWIP_ERR_CLSD:
				should_free_pbuf = false;
				// lwip won't handle ERR_CLSD error for us, manually
				// shuts down the rx side.
				tcp_shutdown(tpcb, 1, 0);
				LOG("tcpRecvFn:ERR_CLSD");
				ret = ERR_CLSD;
				break;
			default:
				LOGE("tcpRecvFn:unexpected error\n");
		}
	}

	if(p != NULL && should_free_pbuf) {
		pbuf_free(p);
	}
	*/
	return ret;
}

err_t LWIPCallback::tcp_sent_cb(void *arg, struct tcp_pcb *tpcb, u16_t len) {
	info("LWIPCallback::tcp_sent_cb(): `localhost:%d`->`%s:%d`", 
		(u16_t) tpcb->remote_port, ipaddr_ntoa(&tpcb->local_ip), (u16_t) tpcb->local_port);
	/*
	tcp_conn *conn = (tcp_conn*)(arg);
	if(tcp_conn_map_get(conn->ts) != NULL) {
		lwip_errorcode err = conn->sent(len);
		if(err != LWIP_ERR_NULL) {
			switch (err) {
				case LWIP_ERR_ABRT:
					return ERR_ABRT;
				case LWIP_ERR_OK:
					return ERR_OK;
				default:
					LOGE("tcpSentFn:unexpected error\n");
			}
		}
	} else {
		tcp_abort(tpcb);
		return ERR_ABRT;
	}
	*/
	return ERR_OK;
}

void LWIPCallback::tcp_error_cb(void *arg, err_t err) {
	/*
	tcp_conn *conn = (tcp_conn*)(arg);
	if(tcp_conn_map_get(conn->ts) != NULL) {
		switch (err) {
			case ERR_ABRT:
				// Aborted through tcp_abort or by a TCP timer
				conn->err("connection aborted");
				break;
			case ERR_RST:
				// The connection was reset by the remote host
				conn->err("connection reseted");
				break;
			default:
				char *buf = (char *)mem_malloc(24);
				sprintf(buf, "lwip error code %d\n", err);
				conn->err(buf);
				mem_free(buf);
				break;
		}
	}
	*/
}

err_t LWIPCallback::tcp_poll_cb(void *arg, struct tcp_pcb *tpcb) {
	/*
	tcp_conn *conn = (tcp_conn*)(arg);
	if (tcp_conn_map_get(conn->ts) != NULL) {
		switch (conn->poll()) {
			case LWIP_ERR_ABRT:
				return ERR_ABRT;
			case LWIP_ERR_OK:
			case LWIP_ERR_NULL:
				return ERR_OK;
			default:
				LOGE("tcpPollFn:unexpected error\n");
		}
	} else {
		tcp_abort(tpcb);
		return ERR_ABRT;
	}
	*/
	return ERR_OK;
}

void LWIPCallback::udp_recv_cb(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port, const ip_addr_t *dest_addr, u16_t dest_port) {
	do {
		if (pcb == NULL) {
			break;
		}
		info("send udp packet from: `%s:%d`-->`%s:%d`", ipaddr_ntoa(addr), port, ipaddr_ntoa(dest_addr), dest_port);
		/*
		if (!udp_conn_map_isinited()) {
			set_udp_conn_map(createHashMap(NULL, _equal_udp));
		}

		struct sockaddr_in dst_addr{};
		dst_addr = get_socket_address(ipaddr_ntoa(dest_addr), (u16_t) dest_port);

		char *map_key = get_address_port(dest_addr, port, dest_port);
		udp_conn *conn = (udp_conn *)(udp_conn_map_get(map_key));
		mem_free(map_key);
		if(conn == NULL) {
			if (get_udp_handler() == NULL) {
				LOG("must register a UDP connection handler\n");
				break;
			}
			conn = new_udp_conn(pcb, get_udp_handler(), *addr, port, *dest_addr, dest_port,
								dst_addr);
			if(conn == NULL) {
				break;
			}
		}
		if (p->tot_len == p->len) {
			conn->receive_to(p->payload, p->tot_len, dst_addr);
		} else {
			void *buf = mem_malloc(p->tot_len);
			pbuf_copy_partial(p, buf, p->tot_len, 0);
			conn->receive_to(buf, p->tot_len, dst_addr);
			mem_free(buf);
		}
	*/
	} while (0);
	pbuf_free(p);
}



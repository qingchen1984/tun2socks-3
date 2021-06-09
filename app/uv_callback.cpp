#include "uv_callback.h"

void UVCallback::server_connected_cb(uv_connect_t* conn, int status) {
	if (status < 0) {
		error("failed to dial to server, err: %s", uv_strerror(status));
		return;
	}
	char key[64];
	struct tcp_pcb* newpcb = (struct tcp_pcb*)(conn->data);
	sprintf(key, "0:%d@%s:%d", newpcb->remote_port, ipaddr_ntoa(&newpcb->local_ip), newpcb->local_port);

	Tun2Socks::sessions[key].uv_conn = conn;
	Tun2Socks::sessions[key].tcp_pcb = (struct tcp_pcb*)(conn->data);
	Tun2Socks::sessions[key].status = CONNECTED;
	
	info("server `%s` is connectd, setup lwip tcp stack callback functions...", key);

	// Sets the callback function that will be called when new data arrives. The callback function will 
	// be passed a NULL pbuf to indicate that the remote host has closed the connection. If the callback 
	// function returns ERR_OK or ERR_ABRT it must have freed the pbuf, otherwise it must not have freed it.
	tcp_recv(newpcb, LWIPCallback::tcp_recv_cb);
	tcp_sent(newpcb, LWIPCallback::tcp_sent_cb);
	tcp_err(newpcb, LWIPCallback::tcp_error_cb);
}

void UVCallback::server_shutdown_cb(uv_shutdown_t* sd, int status) {
    if (status < 0) {
        error("failed to shutdown connection, err: %s", uv_strerror(status));
    } else {
        info("UVCallback::server_shutdown_cb()...");
        uv_close((uv_handle_t*)sd->handle, server_handle_close_cb);
    }
    if(sd != NULL) {
    	// todo：这里是否应该用free
        mem_free(sd);
    }
}

void UVCallback::server_handle_close_cb(uv_handle_t* handle) {
    mem_free(handle);
}


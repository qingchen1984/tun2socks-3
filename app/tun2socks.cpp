#include "tun2socks.h"

int Tun2Socks::tun_output_fd = -1;
std::map<std::string, session_t> Tun2Socks::sessions;

Tun2Socks::Tun2Socks(int tun_fd, uv_loop_t* loop, LWIPStack& l): lwip(l) {
	this->tun_fd = tun_fd;
	this->loop = loop;
	this->poll = (uv_poll_t*)malloc(sizeof(uv_poll_t));
	this->status = STOP;
	Tun2Socks::tun_output_fd = tun_fd;
}

Tun2Socks::~Tun2Socks() {
	info("stop and close tun event loop...");
	if (this->poll) {
		if (this->status == RUNNING) {
			uv_poll_stop(this->poll);
			uv_close((uv_handle_t*)this->poll, NULL);	
		}
		free(this->poll);
	}
}

// 启动tun事件处理器
int Tun2Socks::start() {
	info("tun event loop run...");
	if (this->tun_fd <= 0) {
		error("tun_fd is invalid, tun_fd= %d", tun_fd);
		return -1;
	}

	int ret = uv_poll_init(this->loop, this->poll, this->tun_fd);
	if (ret != 0) {
		error("uv_poll_init() failed, err: %s", uv_strerror(errno));
		return -1;
	}

	ret = uv_poll_start(this->poll, UV_READABLE/*|UV_WRITABLE|UV_PRIORITIZED|UV_DISCONNECT*/, &Tun2Socks::on_poll_cb);
	this->poll->data = this;
	if (ret != 0) {
		error("uv_poll_init() failed, err: %s", uv_strerror(errno));
		return -1;
	}

	setup_output_tun_cb();
	this->status = RUNNING;
	return 0;
}

// poll事件回调
void Tun2Socks::on_poll_cb(uv_poll_s* handle, int status, int events) {
	int tun_fd = ((Tun2Socks*)(handle->data))->tun_fd;
	char buffer[MTU];
	// 读取tun的数据
	if (!status && (events & UV_READABLE)) {
		ssize_t length = read(tun_fd, buffer, MTU);
		printSome((unsigned char*)buffer, length, "*");
		if(length < 0) {
			error("tun_read:(len < 0), err: %s", strerror(errno));
		} else if (length == 0) {
			error("tun is empty");
		} else if(length > 0) {
			((Tun2Socks*)(handle->data))->lwip.input((u8_t*)buffer+4, length);
		}
	}
}

void Tun2Socks::setup_output_tun_cb() {
	if (netif_list != NULL) {
        netif_list->output = Tun2Socks::output_ip4;
        netif_list->output_ip6 = Tun2Socks::output_ip6;
    }
}

err_t Tun2Socks::output_ip4(struct netif *netif, struct pbuf *p, const ip4_addr_t *ipaddr)
{
    return Tun2Socks::output_tun(p);
}

err_t Tun2Socks::output_ip6(struct netif *netif, struct pbuf *p, const ip6_addr_t *ipaddr)
{
	return Tun2Socks::output_tun(p); 
}

err_t Tun2Socks::output_tun(struct pbuf *p) {
    void *buf;
    int ret;
    if(p->tot_len == p->len) {
        buf = (u8_t *)p->payload;
        ret = Tun2Socks::__output_tun(buf, p->tot_len);
    } else {
        buf = malloc(p->tot_len);
        pbuf_copy_partial(p, buf, p->tot_len, 0);
        ret = Tun2Socks::__output_tun(buf, p->tot_len);
        free(buf);
    }
    return ret > 0 ? ERR_OK : -1;
}

// 数据回写tun网卡
err_t Tun2Socks::__output_tun(void *buf, u16_t len) {
	struct iovec iv[2];
	uint32_t type = htonl(AF_INET);
	
	iv[0].iov_base = (uint8_t *)&type;
	iv[0].iov_len = sizeof(type);
	iv[1].iov_base = (uint8_t *)buf;
	iv[1].iov_len = len;
	
	int wlen = writev(Tun2Socks::tun_output_fd, iv, ARRAY_SIZE(iv));
	info("write %d bytes", wlen);
	return wlen > 0 ? 1 : 0;
}




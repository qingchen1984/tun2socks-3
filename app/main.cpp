#include <sys/types.h>
#include <getopt.h>
#include <stdlib.h>

#include "lwip_timer.h"
#include "lwip_stack.h"
#include "tun2socks.h"
#include "logger.h"
#include "tun.h"

/*********************************************************************************/
/**																			 	**/
/** tun2socks																 	**/ 
/** - 使用LWIP																	**/ 
/**																			 	**/ 
/*********************************************************************************/

// 命令行参数
typedef struct _args {
	int tun_id;
	char* tun_ip;
	int mtu;
} args_t;

// 函数定义
int parse_args(int, char**, args_t&); 
int open_tun_device(int, const char*, int);
int start_tun2socks(int);
void set_non_block(int, bool);

// 主函数
int main(int argc, char* argv[]) {
	int ret = 0;
	int tun_fd;
	args_t args;	
	
	ret = parse_args(argc, argv, args);
	if (ret != 0) {
		return -1;
	}

	tun_fd = open_tun_device(args.tun_id, args.tun_ip, args.mtu);
	set_non_block(tun_fd, false);
#ifdef __TEST_TUN2SOCKS__
	info("route add -net 172.168.0.1 -netmask 255.255.255.0 10.25.0.1");
	system("route add -net 172.168.0.1 -netmask 255.255.255.0 10.25.0.1");
#endif

	start_tun2socks(tun_fd);
	return 0;
}

// 启动tun2socks服务
int start_tun2socks(int tun_fd) {
	info("start tun2socks service...");
	uv_loop_t* LOOP = uv_default_loop();
	// LWIP协议栈初始化
	LWIPStack lwip;
	if (lwip.init() !=0 ) {
		error("LWIPStack init failed, abort!");
		return -1;
	}
	// 超时事件处理
	LWIPTimer lt(LOOP);
	lt.start();
	// tun事件处理
	Tun2Socks ts(tun_fd, LOOP, lwip);
	ts.start();
	// 启动uvloop
	info("uvloop run...");
	uv_run(LOOP, UV_RUN_DEFAULT);
	uv_loop_delete(LOOP);	
	return 0;
}

// 命令行参数解析
int parse_args(int argc, char* argv[], args_t& args) {
	const char* usage = "[Usage] tun2socks --tun-id <id> --tun-ip <ip> --mtu <number>";
	const char* optstr = "d:p:m:h";

	struct option opts[] = {
		{"tun-id", 1, NULL, 'd'},
		{"tun-ip", 1, NULL, 'p'},
		{"mtu", 1, NULL, 'm'},
		{"help", 0, NULL, 'h'},
		{0, 0, 0, 0},
	};
	
	int opt;
	while((opt = getopt_long(argc, argv, optstr, opts, NULL)) != -1){
		switch(opt) {
		case 'd':
			args.tun_id = atoi(optarg);
			break;
		case 'p':
			args.tun_ip = optarg;
			break;
		case 'm':
			args.mtu = atoi(optarg);
			break;
		case 'h':
			fprintf(stderr, "%s\n", usage);
			return 0;
		case '?':
			if(strchr(optstr, optopt) == NULL){
				fprintf(stderr, "unknown option '-%c'\n", optopt);
			}else{
				fprintf(stderr, "option requires an argument '-%c'\n", optopt);
			}
			return -1;
		}
	}

	if (args.tun_id<=0 || args.tun_ip==NULL || args.mtu<=0) {
		info("%s", usage);
		return -1;
	}
	return 0;
}

// 创建并启动一个tun网卡
int open_tun_device(int id, const char* ip, int mtu) {
	info("open tun device `%d`, set ip `%s` and mtu `%d`", id, ip, mtu);

	auto sock = open_utun(id+1);
	if (sock >= 0) {
		info("A tun interface has been created");
	} else {
		error("create tun failed, abort!");
		return -1;
	}

	char cmd[128];
	memset(cmd, 0, 128);
	sprintf(cmd, "ifconfig utun%d inet %s %s mtu %d up", id, ip, ip, mtu);

	info("exec: %s ...", cmd);
	system(cmd);
	return sock;
}

void set_non_block(int fd, bool enable) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0 || fcntl(fd, F_SETFL, enable ? flags | O_NONBLOCK : flags & ~O_NONBLOCK) < 0) {
        error("set fd non block failed, err: %s\n", strerror(errno));
    }
}


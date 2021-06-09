#include "logger.h"
#include "tun.h"

// Protocol families are mapped onto address families
// --------------------------------------------------
// Notes:
//  - KEXT Controls and Notifications
//	  https://developer.apple.com/library/content/documentation/Darwin/Conceptual/NKEConceptual/control/control.html
int open_utun(u_int32_t unit) {
	auto fd = socket(PF_SYSTEM, SOCK_DGRAM, SYSPROTO_CONTROL);
	if (fd == -1) {
		error("socket(SYSPROTO_CONTROL) failed, abort!");
		return -1;
	}

	// set the socket non-blocking
	auto err = fcntl(fd, F_SETFL, O_NONBLOCK);
	if (err != 0) {
		error("set the socket non-blocking failed, abort!");
		close(fd);
		return err;
	}

	// set close-on-exec flag
	auto flags = fcntl(fd, F_GETFD);
	flags |= FD_CLOEXEC;
	err = fcntl(fd, F_SETFD, flags);
	if (err != 0) {
		error("set close-on-exec flag failed, abort!");
		close(fd);
		return err;
	}

	// Convert a kernel control name to a kernel control id
	// --------------------------------------------------
	// Notes:
	//  - CTLIOCGINFO
	//	  https://developer.apple.com/documentation/kernel/ctliocginfo?language=objc
	//  - strlcpy
	//	  https://en.wikipedia.org/wiki/C_string_handling
	ctl_info ci{};

	if (strlcpy(ci.ctl_name, UTUN_CONTROL_NAME, sizeof(ci.ctl_name)) >= sizeof(ci.ctl_name)) {
		error("UTUN_CONTROL_NAME is too long, abort!");
		return -1;
	}

	if (ioctl(fd, CTLIOCGINFO, &ci) == -1) {
		error("ioctl(CTLIOCGINFO) failed, abort!");
		close(fd);
		return -1;
	}
	
	sockaddr_ctl sc{};
	sc.sc_id		= ci.ctl_id;
	sc.sc_len	   	= sizeof(sc);
	sc.sc_family	= AF_SYSTEM;
	sc.ss_sysaddr	= AF_SYS_CONTROL;
	sc.sc_unit	  	= unit;

	if (connect(fd, (struct sockaddr *)&sc, sizeof(sc)) == -1) {
		error("connect(AF_SYS_CONTROL) failed, abort!");
		close(fd);
		return -1;
	}
	return fd;
}

uint16_t cksum (const uint8_t *_data, int len) {
  const uint8_t *data = _data;
  uint32_t sum;

  for (sum = 0;len >= 2; data += 2, len -= 2)
    sum += data[0] << 8 | data[1];
  if (len > 0)
    sum += data[0] << 8;
  while (sum > 0xffff)
    sum = (sum >> 16) + (sum & 0xffff);
  sum = htons (~sum);
  return sum ? sum : 0xffff;
}

void printSome(unsigned char* c, int len, char* pipe) {
	for (int i = 0; i < len; i++) {
		if (i % 0x10 == 0) {
			fprintf(stderr, "\n%s %04x ", pipe, i);
		}
		if (i % 8 == 0) {
			fprintf(stderr, " ");
		}
		fprintf(stderr, "%02x ", c[i]);
	}
	fprintf(stderr, "\n\n");
}
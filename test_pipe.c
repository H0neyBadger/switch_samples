#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <stdio.h>

#define DEBUG(msg, ...)do{printf("%s [%s:%d] " msg "\n", __FILE__, __PRETTY_FUNCTION__, __LINE__, ## __VA_ARGS__);}while(0);

int fds[2] = { 0 };

int main(int arg, char ** argv)
{
	// currently nor the pipe or socketpare are available in libnx
	// use a custom udp socket as pipe
	//int r = socket(PF_LOCAL, SOCK_RAW, IPPROTO_IP);
	struct sockaddr_in addr;
	int addr_size = sizeof(addr);

	int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(0);

	bind(fd, (struct sockaddr *) &addr, addr_size);
	getsockname(fd, (struct sockaddr *) &addr, &addr_size);

	//printf("Local port is: %d\n", (int) ntohs(addr.sin_port));
	//printf("Local IP is: %s\n", inet_ntoa(addr.sin_addr));
	if(fd < 0)
		return 1;

	fds[0] = dup(fd);
	fds[1] = dup(fd);
	//pipe(fds);
	/*int r = fcntl(fds[0], F_SETFL, O_NONBLOCK);
	if(r == -1)
	{
		close(fds[0]);
		close(fds[1]);
		return 1;
	}*/

	if(fork() != 0){

		fd_set rfds;
		FD_ZERO(&rfds);
		FD_SET(fds[0], &rfds);

		fd_set efds;
		FD_ZERO(&efds);
		FD_SET(fds[0], &efds);

		int nfds = fds[0];
		nfds++;

		struct timeval timeout_s;
		struct timeval *timeout = NULL;

		//uint8_t test[1];
    	//recv(fd, test, 1, MSG_DONTWAIT);


		printf("Select\n");
		int r = select(nfds, &rfds, NULL, &efds, timeout);
		printf("Select ends\n");
		if(r < 0){
			DEBUG("return single call CHIAKI_ERR_UNKNOWN");
			return 1;
		}

		if(FD_ISSET(fds[0], &rfds)){
			DEBUG("return READ single call CHIAKI_ERR_CANCELED");
			return 0;
		} else if( FD_ISSET(fds[0], &efds) ){
            DEBUG("return EXCEPTION single call CHIAKI_ERR_CANCELED");
            return 0;
		}

		DEBUG("select single timeout");
		return 0;
	} else {
		printf("Sleep\n");
		sleep(1);
		printf("write\n");
		sendto(fds[0], "\x00", 1, 0, (struct sockaddr*) &addr, addr_size);
		printf("Done\n");
	}
}


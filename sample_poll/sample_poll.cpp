#ifdef __SWITCH__
#include <switch.h>
#else
// fake libnx function
int appletMainLoop(){return 1;}
void consoleInit(void * i){return;}
void consoleUpdate(void * i){return;}
void consoleExit(void * i){return;}
void socketInitializeDefault(){return;};
#endif

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <stdio.h>

#include <fcntl.h>
#include <pthread.h>
#include <sys/poll.h>

#define DEBUG(msg, ...)do{ \
	printf("%s [%s:%d] " msg "\n", __FILE__, __func__, __LINE__, ## __VA_ARGS__); \
	consoleUpdate(NULL); \
	}while(0);

int fds[2] = { 0 };


void *poll_events(void *vargp){
	struct pollfd pfds[2];
	pfds[0].fd = fds[0];
	pfds[0].events = POLLIN; // | POLLOUT;
	pfds[0].revents = 0;

	pfds[1].fd = fds[1];
	pfds[1].events = POLLIN; // | POLLOUT;
	pfds[1].revents = 0;

	int loop = 1;
	while(loop){
		printf("Poll (block) on fd: %d, %d\n", fds[0], fds[1]);
		// infinit wait

		int ret = poll(pfds, 2, -1);
		if(ret < 0){
			printf("Poll ERROR\n");
		} else if(ret == 0){
			printf("TIMEOUT\n");
		} else if(ret > 0){
			printf("Event on FD %d\n", ret);
		}

		for(int i = 0; i<2; i++){
			//DEBUG("FD %d revents %x", pfds[i].fd, pfds[i].revents);
			if(pfds[i].revents & POLLIN){
				printf("FD %d, POLLIN = %x\n", pfds[i].fd, pfds[i].revents);
				loop=0;
			}

			if(pfds[i].revents & POLLOUT){
				printf("FD %d, POLLOUT = %x\n", pfds[i].fd, pfds[i].revents);
				loop=0;
			}
		}

	}
	return NULL;
}


void *select_events(void *vargp){
		fd_set rfds;
		FD_ZERO(&rfds);
		FD_SET(fds[0], &rfds);

		fd_set efds;
		FD_ZERO(&efds);
		FD_SET(fds[0], &efds);
		int nfds = fds[0];
		nfds++;

		//struct timeval timeout;
		struct timeval *timeout = NULL;

		//uint8_t test[1];
    	//recv(fd, test, 1, MSG_DONTWAIT);

		printf("Select\n");
		int r = select(nfds, &rfds, NULL, &efds, timeout);
		printf("Select ends !!!!\n");
		if(r < 0){
			printf("return single call CHIAKI_ERR_UNKNOWN\n");
			return NULL;
		}

		if(FD_ISSET(fds[0], &rfds)){
			printf("return READ single call CHIAKI_ERR_CANCELED\n");
			return NULL;
		} else if( FD_ISSET(fds[0], &efds) ){
            printf("return EXCEPTION single call CHIAKI_ERR_CANCELED\n");
            return NULL;
		}

		printf("select single timeout\n");
		return NULL;
}

int main(int arg, char ** argv)
{
    consoleInit(NULL);
    //setsysInitialize();
    socketInitializeDefault();

	DEBUG("start");
	// currently nor the pipe or socketpare are available in libnx
	// use a custom udp socket as pipe
	//int r = socket(PF_LOCAL, SOCK_RAW, IPPROTO_IP);

	struct sockaddr_in addr;
	socklen_t addr_size = sizeof(addr);

	int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	int r = fcntl(fd, F_SETFL, O_NONBLOCK);

    //memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(0);

	bind(fd, (struct sockaddr *) &addr, addr_size);
	getsockname(fd, (struct sockaddr *) &addr, &addr_size);
	connect(fd, (struct sockaddr *) &addr, addr_size);

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
	pthread_t thread_id;
	DEBUG("Create Thread");
	pthread_create(&thread_id, NULL, select_events, NULL);
	DEBUG("sleep");
	sleep(2);
	DEBUG("sendto ->");
	//sendto(fds[0], "\x00", 1, 0, (struct sockaddr*) &addr, addr_size);
	send(fds[0], "\x00", 1, 0);
	// write(fds[0], "\x00",1);
	//close(fds[0]);
	pthread_join(thread_id, NULL);
	DEBUG("Done");
	sleep(5000);
	consoleExit(NULL);
}


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


// memcpy
#include <cstring>
// hto ..
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
// addrinfo
#include <netdb.h>
#include <stdio.h>
#include <pthread.h>

#define DEBUG(msg, ...)do{ \
	printf("%s [%s:%d] " msg "\n", __FILE__, __func__, __LINE__, ## __VA_ARGS__); \
	consoleUpdate(NULL); \
	}while(0)


void *ps4_echo(void *vargp){
	struct sockaddr_in ps4_addr, ps4_remote;
	socklen_t ps4_remote_sz = sizeof(ps4_remote);

	int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    ps4_addr.sin_family = AF_INET;
    ps4_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    ps4_addr.sin_port = htons(1234);

	bind(fd, (struct sockaddr *) &ps4_addr, sizeof(ps4_addr));
	//connect(fd, (const struct sockaddr *) &ps4_remote, sizeof(ps4_remote));

	uint8_t echo_buffer[0x2000]= {0};
	int recv_size = 0;
	while(1){
		printf("PS4 init recv\n");
		recv_size = recvfrom(fd, echo_buffer, sizeof(echo_buffer), 0, (struct sockaddr *) &ps4_remote, &ps4_remote_sz);
		printf("PS4 received size: %d send echo\n", recv_size);
		sleep(1);
		int s = sendto(fd, echo_buffer, recv_size, 0, (struct sockaddr *) &ps4_remote, sizeof(ps4_remote));
		printf("PS4 echo sent ret: %d\n", s);
	}
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

	struct sockaddr_in remote;
	socklen_t addr_size =  sizeof(sockaddr_in);

	int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	const int rcvbuf_val = 0x19000;
	const int sndbuf_val = 0x19000;

	int r, old_val, new_val;
	socklen_t tmp;

	//r = getsockopt(fd, SOL_SOCKET, SO_RCVBUF, (void *)&old_val, &tmp);
	r = setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &rcvbuf_val, sizeof(rcvbuf_val));
	//r = getsockopt(fd, SOL_SOCKET, SO_RCVBUF, (void *)&new_val, &tmp);

#if defined(__SWITCH__)
		const int dontfrag_val = 1;
		r = setsockopt(fd, IPPROTO_IP, IP_DONTFRAG, (const void *)&dontfrag_val, sizeof(dontfrag_val));
#else
		const int mtu_discover_val = IP_PMTUDISC_DO;
		r = setsockopt(fd, IPPROTO_IP, IP_MTU_DISCOVER, (const void *)&mtu_discover_val, sizeof(mtu_discover_val));
#endif

	//DEBUG("rcvbuf_val %d, old %d, new %d", rcvbuf_val, old_val, new_val);

    //memset(&addr, 0, sizeof(struct sockaddr_in));
    //addr.sin_family = AF_INET;
    //addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    //addr.sin_port = htons(0);

    remote.sin_family = AF_INET;
    //remote.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	inet_aton("192.168.0.200", &(remote.sin_addr));
    remote.sin_port = htons(1234);

	//bind(fd, (struct sockaddr *) &addr, addr_size);
	//getsockname(fd, (struct sockaddr *) &addr, &addr_size);
	connect(fd, (struct sockaddr *) &remote, addr_size);

	//printf("Local port is: %d\n", (int) ntohs(addr.sin_port));
	//printf("Local IP is: %s\n", inet_ntoa(addr.sin_addr));
	pthread_t thread_id;
	//DEBUG("Create Thread");
	pthread_create(&thread_id, NULL, ps4_echo, NULL);
	uint8_t echo_buffer[1472] = { 'A' };
	int recv_size = 0;
	sleep(1);
	int loop =0;
	while(1){
		DEBUG("send loop %d->",loop++);
		r = send(fd, echo_buffer, sizeof(echo_buffer), 0);
		DEBUG("send return %d", r);
		recv_size = recv(fd, echo_buffer, sizeof(echo_buffer), 0);
		DEBUG("received echo recv_size %d", recv_size);
		//DEBUG("SLEEP refresh > recv ?");
		//sleep(1);
		DEBUG("SLEEP");
		//sleep(1);
	}

	pthread_join(thread_id, NULL);
	DEBUG("Done");
	sleep(5000);
	consoleExit(NULL);
}


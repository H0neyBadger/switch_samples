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

#include <cstdio>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

#ifdef __SWITCH__
static int s_nxlinkSock = -1;

static void initNxLink()
{
    if (R_FAILED(socketInitializeDefault()))
        return;

    s_nxlinkSock = nxlinkStdio();
    if (s_nxlinkSock >= 0)
        printf("initNxLink");
    else
    socketExit();
}

static void deinitNxLink()
{
    if (s_nxlinkSock >= 0)
    {
        close(s_nxlinkSock);
        socketExit();
        s_nxlinkSock = -1;
    }
}

extern "C" void userAppInit()
{
    initNxLink();
}

extern "C" void userAppExit()
{
    deinitNxLink();
}

#endif

#define D(x, ...){ printf("%s:%d [%s] " x "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__); }

int main(int arg, char ** argv) {
#ifdef __SWITCH__
    socketInitializeDefault();
#endif
	D("start");

	int sockfd;
	struct sockaddr_in addr;
	int numbytes;
	int broadcast = 1;


	if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		D("failed to create socket");
		return 1;
	}

	if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast,
		sizeof broadcast) == -1) {
		D("failed to setsockopt (SO_BROADCAST)");
		return 1;
	}

#ifdef __SWITCHD__
	if (setsockopt(sockfd, IPPROTO_IP, IP_ONESBCAST, &broadcast,
		sizeof broadcast) == -1){
		D("failed to setsockopt (IP_ONESBCAST)");
		return 1;
	}
#endif

	addr.sin_family = AF_INET;
	addr.sin_port = htons(2000);
	//addr.sin_addr.s_addr = inet_addr("255.255.255.255");
	addr.sin_addr.s_addr = inet_addr("192.168.0.255");
	memset(addr.sin_zero, '\0', sizeof addr.sin_zero);

	if ((numbytes=sendto(sockfd, "magic", strlen("magic"), 0,
		(struct sockaddr *)&addr, sizeof addr)) == -1) {
		D("failed sendto %d `%s`", errno, strerror(errno));
		return 1;
	}

	D("sent %d bytes to %s\n", numbytes,
	inet_ntoa(addr.sin_addr));

	close(sockfd);

	return 0;
}

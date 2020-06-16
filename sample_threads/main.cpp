#ifdef __SWITCH__
#include <switch.h>
#include <switch/kernel/condvar.h>
#include <switch/kernel/mutex.h>
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
#include <errno.h>


#ifdef FIXME__SWITCH__
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

#ifdef fix__SWITCH__
CondVar condition;
Mutex mutex;

#define DEBUG(msg, ...)do{ \
	mutexLock(&mutex); \
	printf("%s [%s:%d] " msg "\n", __FILE__, __func__, __LINE__, ## __VA_ARGS__); \
	condvarWakeAll(&condition); \
	mutexUnlock(&mutex); \
	}while(0)

#else
pthread_cond_t condition;
pthread_mutex_t mutex;

#define DEBUG(msg, ...)do{ \
	pthread_mutex_lock(&mutex); \
	printf("%s [%s:%d] " msg "\n", __FILE__, __func__, __LINE__, ## __VA_ARGS__); \
	pthread_cond_signal(&condition); \
	pthread_mutex_unlock(&mutex); \
	}while(0)

#endif

static void set_timeout(struct timespec *timeout, uint64_t ms_from_now)
{
    //clock_gettime(CLOCK_REALTIME, timeout);
    clock_gettime(CLOCK_MONOTONIC, timeout);

	printf("time -> %lld.%.9ld\n", (long long)timeout->tv_sec, timeout->tv_nsec);
    timeout->tv_sec += ms_from_now / 1000;
    timeout->tv_nsec += (ms_from_now % 1000) * 1000000;
    if(timeout->tv_nsec > 1000000000)
    {
        timeout->tv_sec += timeout->tv_nsec / 1000000000;
        timeout->tv_nsec %= 1000000000;
    }
	printf("time -> %lld.%.9ld\n", (long long)timeout->tv_sec, timeout->tv_nsec);
}

static uint64_t get_ns_timeout(uint64_t ms_from_now)
{
	struct timespec timeout;
    //clock_gettime(CLOCK_REALTIME, timeout);
    clock_gettime(CLOCK_MONOTONIC, &timeout);
	//return timeout.tv_sec * 1000000000 + ms_from_now * 1000000 + timeout.tv_nsec;
	return ms_from_now * 1000000;
}

void *sender(void *vargp){
	struct sockaddr_in remote;
	socklen_t addr_size =  sizeof(sockaddr_in);

	int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	const int rcvbuf_val = 0x19000;

	int r, old_val, new_val;

	r = setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &rcvbuf_val, sizeof(rcvbuf_val));

#if defined(__SWITCH__)
		const int dontfrag_val = 1;
		r = setsockopt(fd, IPPROTO_IP, IP_DONTFRAG, (const void *)&dontfrag_val, sizeof(dontfrag_val));
#else
		const int mtu_discover_val = IP_PMTUDISC_DO;
		r = setsockopt(fd, IPPROTO_IP, IP_MTU_DISCOVER, (const void *)&mtu_discover_val, sizeof(mtu_discover_val));
#endif

    remote.sin_family = AF_INET;
    //remote.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	inet_aton("192.168.0.200", &(remote.sin_addr));
    remote.sin_port = htons(1234);

	connect(fd, (struct sockaddr *) &remote, addr_size);

	uint8_t echo_buffer[500];
	while(1){
		DEBUG("send echo buffer");
		r = send(fd, echo_buffer, sizeof(echo_buffer), 0);
		DEBUG("send return %d", r);
		sleep(1);
	}

}

void *receiver(void *vargp){
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
		DEBUG("PS4 init recv");
		recv_size = recvfrom(fd, echo_buffer, sizeof(echo_buffer), 0, (struct sockaddr *) &ps4_remote, &ps4_remote_sz);
		DEBUG("PS4 received size: %d send echo", recv_size);
	}
}

int main(int arg, char ** argv)
{
    consoleInit(NULL);
    //setsysInitialize();
    socketInitializeDefault();

	printf("start\n");
	printf("Create Thread\n");

	pthread_t receiver_id, sender_id;
	pthread_create(&receiver_id, NULL, receiver, NULL);
	pthread_create(&sender_id, NULL, sender, NULL);

	static struct timespec timeout = {0, 0};
	uint64_t ms_from_now = 1000;
	uint64_t ns_timeout = 0;
	int r = 0;

    pthread_condattr_t attr;
    r = pthread_condattr_init(&attr);
	if(r != 0){
		printf("failed to init attr");
	}
	r = pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
	if(r != 0){
		printf("failed to setclock attr");
	}
	r = pthread_cond_init(&condition, &attr);
	if(r != 0){
		printf("failed cond init");
	}
	consoleUpdate(NULL);
	while(1){
#ifdef fix__SWITCH__
        mutexLock(&mutex);
		//set_timeout(&timeout, ms_from_now);
		ns_timeout = get_ns_timeout(ms_from_now);
		printf("%ld \n", ns_timeout);
		consoleUpdate(NULL);
	    r = condvarWaitTimeout(&condition, &mutex, ns_timeout);
        mutexUnlock(&mutex);
	    if(r == 0xEA01)
	    {
			printf("ETIMEDOUT\n");
	    } else if( r != 0 ) {
			printf("Error\n");
		}
		consoleUpdate(NULL);
#else
        pthread_mutex_lock(&mutex);
		set_timeout(&timeout, ms_from_now);
		consoleUpdate(NULL);
	    r = pthread_cond_timedwait(&condition, &mutex, &timeout);
        pthread_mutex_unlock(&mutex);
	    if(r == ETIMEDOUT)
	    {
			printf("ETIMEDOUT\n");
	    } else if( r != 0 ) {
			printf("Error\n");
		}
		consoleUpdate(NULL);
#endif
	}

	pthread_join(sender_id, NULL);
	pthread_join(receiver_id, NULL);

	DEBUG("Done");
	sleep(5000);
	consoleExit(NULL);
}


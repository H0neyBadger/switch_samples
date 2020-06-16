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
// malloc
#include <cstdlib>

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

pthread_cond_t condition;
pthread_mutex_t mutex;
pthread_barrier_t barrier;

#define DEBUG(msg, ...)do{ \
	pthread_mutex_lock(&mutex); \
	printf("%s [%s:%d] " msg "\n", __FILE__, __func__, __LINE__, ## __VA_ARGS__); \
	pthread_cond_signal(&condition); \
	pthread_mutex_unlock(&mutex); \
	}while(0)

typedef struct data {
	int index;
} Data;

typedef struct test {
	pthread_t thread_id;
	Data *data;
} Test;


static void set_timeout(struct timespec *timeout, uint64_t ms_from_now)
{
    clock_gettime(CLOCK_MONOTONIC, timeout);
    timeout->tv_sec += ms_from_now / 1000;
    timeout->tv_nsec += (ms_from_now % 1000) * 1000000;
    if(timeout->tv_nsec > 1000000000)
    {
        timeout->tv_sec += timeout->tv_nsec / 1000000000;
        timeout->tv_nsec %= 1000000000;
    }
}

void *test(void *vargp){
	Data *d = (Data*) vargp;
	pthread_barrier_wait(&barrier);
	int loop = 0;
	while(1){
		loop++;
		DEBUG("data %d %d\n", d->index, loop);
		pthread_barrier_wait(&barrier);
	}
}

int print_thread_list(){
	int out_num_threads = 0;
	svcGetThreadList(&out_num_threads, NULL, 0, INVALID_HANDLE);
	printf("GetThreadList %d\n", out_num_threads);
	//consoleUpdate(NULL);
	return out_num_threads;
}

int test_thread_limit(){
	u64 resource_limit_handle_value = INVALID_HANDLE;
	svcGetInfo(&resource_limit_handle_value, InfoType_ResourceLimit, INVALID_HANDLE, 0);

	s64 thread_cur_value = 0, thread_lim_value = 0;
	svcGetResourceLimitCurrentValue(&thread_cur_value, resource_limit_handle_value, LimitableResource_Threads);
	svcGetResourceLimitLimitValue(&thread_lim_value, resource_limit_handle_value, LimitableResource_Threads);

	printf("thread_cur_value: %lu, thread_lim_value: %lu\n", thread_cur_value, thread_lim_value);
	//consoleUpdate(NULL);
	return 0;
}



int main(int arg, char ** argv)
{
    //consoleInit(NULL);
    //setsysInitialize();
    //socketInitializeDefault();

	printf("start\n");
	printf("Create Thread\n");

	static struct timespec timeout = {0, 0};
	uint64_t ms_from_now = 5000;
	int r = 0;

	int base_count = print_thread_list();
	test_thread_limit();
	sleep(3);

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

	pthread_attr_t t_attr;
	pthread_attr_init(&t_attr);
	//size_t base;
	//pthread_attr_getstacksize(&t_attr, &base);
	//pthread_attr_setstacksize(&t_attr, 8388608);
	//printf("Thread stacksize %d\n", base);

	// OK
	//int count = 68;
	// KO
	int count = 69;
	pthread_barrier_init(&barrier, NULL, count);

	Test tests[count] = { 0 };
	for(int i=0; i<count; i++){
		tests[i].data = (Data *) malloc(sizeof(Data));
		tests[i].data->index = i;
		int r = pthread_create(&(tests[i].thread_id), &t_attr, test,(void*) tests[i].data);
		switch(r){
			case 0: break;
			case EAGAIN:
       			printf("EAGAIN Insufficient resources to create another thread.");
				//consoleUpdate(NULL); sleep(10);
				break;
			case EINVAL:
				printf("EINVAL Invalid settings in attr.");
				//consoleUpdate(NULL); sleep(10);
				break;
			case EPERM:
				printf("EPERM  No permission to set the scheduling policy and parameters specified in attr.");
				//consoleUpdate(NULL); sleep(10);
				break;
			default:
				continue;
		}
	}
	int new_count = print_thread_list();
	printf("base count %d new count %d diff %d\n", base_count, new_count, new_count - base_count);
	test_thread_limit();
	//consoleUpdate(NULL);

	while(1){
        pthread_mutex_lock(&mutex);
		set_timeout(&timeout, ms_from_now);
	    r = pthread_cond_timedwait(&condition, &mutex, &timeout);
        pthread_mutex_unlock(&mutex);
	    if(r == ETIMEDOUT)
	    {
			printf("ETIMEDOUT\n");
	    } else if( r != 0 ) {
			printf("Error\n");
		}
		//consoleUpdate(NULL);
	}

	for(int i=0; i<count; i++){
		pthread_join(tests[i].thread_id, NULL);
	}
	//consoleExit(NULL);
}


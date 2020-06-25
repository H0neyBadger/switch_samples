
#include <cstdio>

#ifdef __SWITCH__
#include <switch.h>
#include <unistd.h>
#define ENABLE_SWITCH_NXLINK 1
#endif


#ifdef ENABLE_SWITCH_NXLINK
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
#endif

int main(int argc, char* argv[]){
	printf("NXLink Hello World\n");
}


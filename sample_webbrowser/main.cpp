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
#include <cstdio>

#ifdef __SWITCH__
static int s_nxlinkSock = -1;

static void initNxLink()
{
    if (R_FAILED(socketInitializeDefault()))
        return;

    s_nxlinkSock = nxlinkStdio();
    if (s_nxlinkSock >= 0)
        printf("initNxLink\n");
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

#define DEBUG(msg, ...)do{ \
	printf("%s [%s:%d] " msg "\n", __FILE__, __func__, __LINE__, ## __VA_ARGS__); \
	}while(0)

#define RC(x){ \
	printf("%s [%s:%d] %s SUCCEEDED = %d\n", __FILE__, __func__, __LINE__, #x, R_SUCCEEDED(x)); \
}

int main(int arg, char ** argv)
{
	// the following sample does not work as expected
	// it does not allow sony authentication
    char auth_url[] = "https://auth.api.sonyentertainmentnetwork.com/2.0/oauth/authorize?service_entity=urn:service-entity:psn&response_type=code&client_id=ba495a24-818c-472b-b12d-ff231c1b5745&redirect_uri=https://remoteplay.dl.playstation.net/remoteplay/redirect&scope=psn:clientapp&request_locale=en_US&ui=pr&service_logo=ps&layout_type=popup&smcid=remoteplay&prompt=always&PlatformPrivacyWs1=minimal&";
    char callback_url[] = "https://remoteplay.dl.playstation.net/remoteplay/redirect";


	char whitelist[] = ".*\\.sonyentertainmentnetwork\\.com\n"
                       ".*\\.playstation\\.net\n"
					   ".*\\.playstation\\.com\n";

	char last_url[0x1000] = {0};
	// char whitelist[] = ".*";
	WebCommonConfig config;
	WebSession session;
	WebExitReason exit_reason;
	WebCommonReply reply;
	size_t out_size = sizeof(last_url);

	webSessionCreate(&session, &config);
	RC(webConfigSetWhitelist(&config, whitelist));
	RC(webConfigSetJsExtension(&config, true));
	RC(webConfigSetCallbackUrl(&config, callback_url));
	RC(webConfigSetCallbackableUrl(&config, callback_url));

	RC(webPageCreate(&config, auth_url));
	// webConfigSetPageCache(&config, true);

	RC(webConfigShow(&config, &reply));

	webReplyGetExitReason(&reply, &exit_reason);
	printf("ExitReason %02X\n", exit_reason);
	webReplyGetLastUrl(&reply, last_url, sizeof(last_url), &out_size);
	printf("LastUrl %s\n", last_url);
}


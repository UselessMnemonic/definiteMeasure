#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <ogcsys.h>
#include <gccore.h>
#include <network.h>
#include <debug.h>
#include <errno.h>
#include <wiiuse/wpad.h>
#include <ogc/pad.h>

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

void *initialise();
void *httpd (void *arg);
void poll();

static	lwp_t httd_handle = (lwp_t)NULL;

struct packet{
u16 dData;
s8 aData[6];
} p;


//---------------------------------------------------------------------------------
int main(int argc, char **argv) {
//---------------------------------------------------------------------------------
	s32 ret;

	char localip[16] = {0};
	char gateway[16] = {0};
	char netmask[16] = {0};
	
	xfb = initialise();
	printf ("\n");
	printf ("\n******************************************");
	printf ("\n*                                        *");
	printf ("\n*                 GC Pad                 *");
	printf ("\n*          Network Functionality         *");
	printf ("\n*                                        *");
	printf ("\n*                   by                   *");
	printf ("\n*                                        *");
	printf ("\n*          Christopher Madrigal          *");
	printf ("\n*                                        *");
	printf ("\n******************************************");
	printf("\nHit HOME on Wiimote to exit back to Loader...");
	// Configure the network interface
	ret = if_config ( localip, netmask, gateway, TRUE);
	if (ret>=0) {
		printf ("\network configured, ip: %s, gw: %s, mask: %s, port: 15\n", localip, gateway, netmask);

		LWP_CreateThread(	&httd_handle,	/* thread handle */ 
							httpd,			/* code */ 
							localip,		/* arg pointer for thread */
							NULL,			/* stack base */ 
							16*1024,		/* stack size */
							50				/* thread priority */ );
	} else {
		printf ("network configuration failed!\n");
		exit(0);
	}

	while(1) {

		VIDEO_WaitVSync();
		WPAD_ScanPads();

		int buttonsDown = WPAD_ButtonsDown(0);
		
		if (buttonsDown & WPAD_BUTTON_HOME) {
			exit(0);
		}
	}

	return 0;
}


//---------------------------------------------------------------------------------
void *httpd (void *arg) {
//---------------------------------------------------------------------------------

	s32 sock, csock;
	int ret;
	u32	clientlen;
	struct sockaddr_in client;
	struct sockaddr_in server;
	char temp[1026];
	
	
	
	clientlen = sizeof(client);

	sock = net_socket (AF_INET, SOCK_STREAM, IPPROTO_IP);

	if (sock == INVALID_SOCKET) {
      printf ("Cannot create a socket!\n");
    } else {

		memset (&server, 0, sizeof (server));
		memset (&client, 0, sizeof (client));

		server.sin_family = AF_INET;
		server.sin_port = htons (15);
		server.sin_addr.s_addr = INADDR_ANY;
		ret = net_bind (sock, (struct sockaddr *) &server, sizeof (server));
		
		if ( ret ) {

			printf("Error %d binding socket!\n", ret);
			exit(0);

		} else {

			if ( (ret = net_listen( sock, 5)) ) {

				printf("Error %d listening!\n", ret);
				exit(0);

			} else {
			
				while(1) {
	
					csock = net_accept (sock, (struct sockaddr *) &client, &clientlen);
				
					if ( csock < 0 ) {
						printf("Error connecting socket %d!\n", csock);
						while(1);
					}

					printf("Connecting port %d from %s\n", client.sin_port, inet_ntoa(client.sin_addr));
					memset (temp, 0, 1026);
					
					s32 gl = 8;
					
					while(gl == sizeof(p))
					{
					poll();
					gl = net_send(csock, &p, sizeof(p), 0);
					
					}
					printf("Stopped!\n\n");
				}
			}
		}
	}
	return NULL;
}

void poll()
{
	WPAD_ScanPads();

	if(WPAD_ButtonsHeld(0) & WPAD_BUTTON_HOME)
		exit(0);

	PAD_ScanPads();

	p.dData = PAD_ButtonsHeld(0); //Get button info

	p.aData[0] = PAD_StickX(0); //Get Stick position
	p.aData[1] = PAD_StickY(0);

	p.aData[2] = PAD_SubStickX(0); //Get C-Stick position
	p.aData[3] = PAD_SubStickY(0);

	p.aData[4] = PAD_TriggerL(0); //Get Shoulder positions
	p.aData[5] = PAD_TriggerR(0);
}


//---------------------------------------------------------------------------------
void *initialise() {
//---------------------------------------------------------------------------------

	void *framebuffer;

	VIDEO_Init();
	WPAD_Init();
	PAD_Init();
	
	rmode = VIDEO_GetPreferredMode(NULL);
	framebuffer = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	console_init(framebuffer,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);
	
	VIDEO_Configure(rmode);
	VIDEO_SetNextFramebuffer(framebuffer);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();

	return framebuffer;

}

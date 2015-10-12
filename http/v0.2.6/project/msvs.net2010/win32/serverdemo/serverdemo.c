//
// SERVERDEMO.C - Web Server Demo
//
// EBS -
//
// Copyright EBS Inc. , 2009
// All rights reserved
// This code may not be redistributed in source or linkable object form
// without the consent of its author.
//
/*---------------------------------------------------------------------------*/
//#include "rtpnet.h"
//#include "rtpthrd.h"
//#include "rtpmemdb.h"
//#include "rtpterm.h"
#include "rtpprint.h"
#include "rtpgui.h"
#include <process.h>

int http_server_demo(void);
int http_client_demo(void);
int http_advanced_server_demo(void);
/*---------------------------------------------------------------------------*/
#define CLIENT 1
#define SERVER 2
#define ADVANCED_SERVER 3
#define SERVER_BG 4
#define ADVANCED_SERVER_BG 5

/*---------------------------------------------------------------------------*/

int fidgets_main(void);
int test_real_main(int argc,char ** argv);

main(int argc, char *argv[])
{
	int choice = 0,i;
	void *pmenu;

//	test_real_main(0,0);
//	return 0;
	if (argc==2)
	{
		if (strcmp(argv[1],"advanced")==0)
			http_advanced_server_demo();
		else
			http_server_demo();
		return 0;

	}
	else if (argc==1)
	{
	char *args[4];
		args[0]=argv[0];
		args[1]="Client";
		args[2]=0;
		pmenu = rtp_gui_list_init(0, RTP_LIST_STYLE_MENU, "EBS HTTP Demo", "Select Option", "Option >");
		if (!pmenu)
			return(-1);;
		rtp_gui_list_add_int_item(pmenu, "Client only in Foreground", CLIENT, 0, 0, 0);
		rtp_gui_list_add_int_item(pmenu, "BASIC SERVER DEMO in Foreground", SERVER, 0, 0, 0);
		rtp_gui_list_add_int_item(pmenu, "ADVANCE SERVER DEMO in Foreground", ADVANCED_SERVER, 0, 0, 0);
		rtp_gui_list_add_int_item(pmenu, "BASIC SERVER DEMO in BG Run Client Applications in FG", SERVER_BG, 0, 0, 0);
		rtp_gui_list_add_int_item(pmenu, "ADVANCE SERVER DEMO in BG Run Client Applications in FG", ADVANCED_SERVER_BG, 0, 0, 0);



		choice = rtp_gui_list_execute_int(pmenu);
		rtp_gui_list_destroy(pmenu);

		switch(choice){
			case SERVER_BG:
			{
				rtp_printf("Spawning basic server demo\n");
				args[1]="basic";
				break;
    		}
			case ADVANCED_SERVER_BG:
			{
				rtp_printf("Spawning advanced server demo\n");
				args[1]="advanced";
				break;
			}
			case SERVER:
			{
				rtp_printf("Calling server demo\n");
                http_server_demo();
                rtp_printf("Returned from server demo\n");
				return 0;
				break;
			}

		default:
			rtp_printf("Invalid Selection Calling advance server demo\n");
		case ADVANCED_SERVER:
			{
				rtp_printf("Calling advanced server demo\n");

                http_advanced_server_demo();
                rtp_printf("Returned from advanced server demo\n");
				return 0;
				break;
			}
		case CLIENT:
			{
				rtp_printf("Calling client demo\n");
				http_client_demo();
                rtp_printf("Returned from client demo\n");
				return 0;
				break;
			}
		}
		/* If we get here we want to run the server in the background and the client in the foreground */
		spawnv(_P_DETACH, argv[0], args);	
		http_client_demo();
		return 0;
	}
#if (0)

	pmenu = rtp_gui_list_init(0, RTP_LIST_STYLE_MENU, "EBS HTTP Demo", "Select Option", "Option >");
	if (!pmenu)
		return(-1);;

	rtp_gui_list_add_int_item(pmenu, "CLIENT DEMO", CLIENT, 0, 0, 0);
	rtp_gui_list_add_int_item(pmenu, "SERVER DEMO", SERVER, 0, 0, 0);
	rtp_gui_list_add_int_item(pmenu, "ADVANCE SERVER DEMO", ADVANCED_SERVER, 0, 0, 0);

	choice = rtp_gui_list_execute_int(pmenu);
	rtp_gui_list_destroy(pmenu);

	switch(choice){
		case CLIENT:
			{
                rtp_printf("Calling client demo\n");
                http_client_demo();
                rtp_printf("Returned from client demo\n");
				break;
			}
		case SERVER:
			{
				rtp_printf("Calling server demo\n");
                http_server_demo();
                rtp_printf("Returned from server demo\n");
				break;
			}
		case ADVANCED_SERVER:
			{
				rtp_printf("Calling advanced server demo\n");
                http_advanced_server_demo();
                rtp_printf("Returned from advanced server demo\n");
				break;
			}
		default:
			{
				rtp_printf("Invalid Selection\n");
				break;
			}
	}
#endif
	return(0);
}

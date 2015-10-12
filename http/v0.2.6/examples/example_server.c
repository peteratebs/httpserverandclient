/*
|  example_server_basic.c
|
|  Copyright EBS Inc. , 2009. All rights reserved.

This source file contains an example of a simple web server application.

This example demonstrates the following:

. Configure initialize and call the web server.
. Serve file based content from a default directory /htdocs by default.

*/

/*****************************************************************************/
/* Header files
 *****************************************************************************/

#include "rtpthrd.h"
#include "rtpnet.h"
#include "rtpterm.h"
#include "rtpprint.h"
#include "rtpstr.h"
#include "httpsrv.h"

/*****************************************************************************/
/* Macros
 *****************************************************************************/

/* Configuration parameters */

/* These values may be changed to customize the server. */
#define DEMO_SERVER_NAME "EBS WEB SERVER" /* Server name that we use to format the Server property in a standard html reponse header */
//#define DEMO_WWW_ROOT "/htdocs"        /* Path used as the root for all content loads from disk. */
//#define DEMO_WWW_FILE "index.html"        /* The default file name to use if a user accesses the server providing no file name. Index.htm is recommended. */
#define DEMO_WWW_ROOT "../../../htdocs"        /* Path used as the root for all content loads from disk. */
#define DEMO_WWW_FILE "index.html"        /* The default file name to use if a user accesses the server providing no file name. Index.htm is recommended. */


static unsigned char
DEFAULT_DEMO_IPADDRESS[] = {0,0,0,0};   /* IP address of the interface to serve content to. Set to {0,0,0,0} to
                                           use the default interface. If you want to serve to an alternate interface
                                           set this to the IP addres of the alternate interface (for example: {192,168,1,1}.
                                           Note: Linux and windows do not but some emebedded network stack
                                           implementations may require this field to be initialized.  */

#define DEMO_MAX_CONNECTIONS 8		   /* Configure the maximum number of simultaneously connected/queued for acceptance */

#define DEMO_IPVERSION 4               /* Do not change. The API supports V6 but the demo is wired for V4 right now */
#define DEMO_MAX_HELPERS 1             /* Do not change. Number of helper threads to spawn. Must be 1. */


/*****************************************************************************/
/* Function Prototypes
/*****************************************************************************/


/* Utility functions used by this example */

/*****************************************************************************/
/* Data
 *****************************************************************************/

 /* A context structure for keeping our application data. This structure is not required by the library, it only used by the example. */
typedef struct s_HTTPExampleServerCtx
{
	HTTP_UINT8         rootDirectory[256];
	HTTP_UINT8         defaultFile[256];
  	HTTP_BOOL          disableVirtualIndex;
  	HTTP_BOOL          chunkedEncoding;
	HTTP_INT16         numHelperThreads;
	HTTP_INT16         numConnections;
	HTTP_UINT8         defaultIpAddr[4];
	HTTPServerConnection* connectCtxArray;
	HTTPServerContext  httpServer;
  	HTTP_BOOL          ModuleRequestingReload;
} HTTPExampleServerCtx;

HTTPExampleServerCtx ExampleServerCtx;

/*****************************************************************************/
/* Function Definitions
 *****************************************************************************/

/* Main entry point for the web server application.
	A callable web server.
*/

int http_server_demo(void)
{
	HTTP_UINT8     *pDefaultIpAddr;
	int idle_count = 0;
	rtp_net_init();
	rtp_threads_init();

	/* Set initial default values */
	rtp_memset(&ExampleServerCtx, 0, sizeof(ExampleServerCtx));
	rtp_strcpy(ExampleServerCtx.rootDirectory, DEMO_WWW_ROOT);
	rtp_strcpy(ExampleServerCtx.defaultFile,DEMO_WWW_FILE);
	ExampleServerCtx.chunkedEncoding 		= 0;
	ExampleServerCtx.numHelperThreads 		= DEMO_MAX_HELPERS;
	ExampleServerCtx.numConnections		 	= DEMO_MAX_CONNECTIONS;
	/* If these are {0,0,0,0} use the default interface otherwise we use the configured address */
	ExampleServerCtx.defaultIpAddr[0] = DEFAULT_DEMO_IPADDRESS[0];
	ExampleServerCtx.defaultIpAddr[1] = DEFAULT_DEMO_IPADDRESS[1];
	ExampleServerCtx.defaultIpAddr[2] = DEFAULT_DEMO_IPADDRESS[2];
	ExampleServerCtx.defaultIpAddr[3] = DEFAULT_DEMO_IPADDRESS[3];

	rtp_printf("Using IP address %d.%d.%d.%d (all zeroes means use default interface) \n",
		ExampleServerCtx.defaultIpAddr[0],	ExampleServerCtx.defaultIpAddr[1],
		ExampleServerCtx.defaultIpAddr[2],	ExampleServerCtx.defaultIpAddr[3]);

	/* Initialize the server */
	/* The demo uses 0,0,0,0 to signify use default but the API uses a null pointer so
	   map the demo methodology to the API metodology */
	if (ExampleServerCtx.defaultIpAddr[0] == 0 &&
		ExampleServerCtx.defaultIpAddr[1] == 0 &&
		ExampleServerCtx.defaultIpAddr[2] == 0 &&
		ExampleServerCtx.defaultIpAddr[3] == 0)
		pDefaultIpAddr = 0;
	else
		pDefaultIpAddr = &ExampleServerCtx.defaultIpAddr[0];

	rtp_printf("Starting or restarting server \n");

	/* Allocate and clear one server context per possible simultaneous connection. see (DEMO_MAX_CONNECTIONS) */
	if (!ExampleServerCtx.connectCtxArray)
	{
		ExampleServerCtx.connectCtxArray = (HTTPServerConnection*) rtp_malloc(sizeof(HTTPServerConnection) * ExampleServerCtx.numConnections);
		if (!ExampleServerCtx.connectCtxArray)
			return -1;
	}
	rtp_memset(ExampleServerCtx.connectCtxArray,0, sizeof(HTTPServerConnection) * ExampleServerCtx.numConnections);

	/* Initialize the server */
	if (HTTP_ServerInit (
			&ExampleServerCtx.httpServer,
			DEMO_SERVER_NAME, // name
			ExampleServerCtx.rootDirectory,    // rootDir
			ExampleServerCtx.defaultFile,      // defaultFile
			1,                // httpMajorVersion
			1,                // httpMinorVersion
			pDefaultIpAddr,
			80,               // 80  ?? Use the www default port
			DEMO_IPVERSION,   // ipversion type(4 or 6)
			0,                // allowKeepAlive
			ExampleServerCtx.connectCtxArray,  // connectCtxArray
			ExampleServerCtx.numConnections,   // maxConnections
			ExampleServerCtx.numHelperThreads  // maxHelperThreads
		) < 0)
		return -1;

	/* Now loop continuously process one request per loop. */
	for (;;)
	{
		if (HTTP_ServerProcessOneRequest (&ExampleServerCtx.httpServer, 1000*60) < 0)
		{
			/* Print an idle counter every minute the server is not accessed */
			idle_count += 1;
			if (idle_count == 1)
				rtp_printf("\n Idle %d minutes      ", idle_count);
			else
				rtp_printf("                                     \r Idle %d minutes", idle_count);
		}
		else
			idle_count = 0;
	}
	rtp_net_exit();
	return (0);
}

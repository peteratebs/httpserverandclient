#if (1)
/*
|  example_server_advanced.c
|
|  EBS -
|
|   $Author:  $
|   $Date: 2009/08/22 19:14:06 $
|   $Name:  $
|   $Revision: 1.2 $
|
|  Copyright EBS Inc. , 2009
|  All rights reserved.
|  This code may not be redistributed in source or linkable object form
|  without the consent of its author.
*/

/*****************************************************************************/
/* Header files
 *****************************************************************************/

// #define CALL_SSL_MAIN 0

#include "rtpthrd.h"
#include "rtpnet.h"
#include "rtpterm.h"
#include "rtpprint.h"
#include "rtpstr.h"
#include "rtpscnv.h"
#include "httpsrv.h"
#include "htmlutils.h"
#ifdef CALL_SSL_MAIN
#include "rtpssl.h"
#include "httpauth.h"
#include <openssl/ssl.h>

static HTTPAuthContext Authctx;
static HTTPAuthContext *pAuthctx;
static RTP_HANDLE sslContext = 0;
static unsigned sslEnabled  = 1;
#endif


 /* A context structure for keeping our application data
    This is a duplicate of the typedef in example_server.c
    this is bad form but not requiring an additional header file
    implifies the examples for documentation purposes. */
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

static  HTTPExampleServerCtx ExampleServerCtx;

/*****************************************************************************/
/* Macros
 *****************************************************************************/

 /* Configuration parameters */

/* These values may be changed to customize the server. */
#define DEMO_SERVER_NAME "EBS WEB SERVER" /* Server name that we use to format the Server property in a standard html reponse header */
#define DEMO_WWW_ROOT "../../htdocs"        /* Path used as the root for all content loads from disk. */
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



/* ========================================================================= */
/* Virtual index page code starts here. Demonstrates how to serve on the fly
   smart pages along side disk based pages */

#define DEMO_CONFIGURE_ID 	0
#define DEMO_CONFIGURE_URL 	"\\configure.htm"
#define DEMO_INFO_ID  		1
#define DEMO_INFO_URL  		"\\info.html"

static int AjaxDemoValue=0;

// #define CALL_SSL_MAIN 1
#ifdef CALL_SSL_MAIN
void ssl_server_main(/*SSL_CTX *ctx,*/char *Suite/*'a','c','n'*/, int sslmethod/* 1,2,3,4 */,char *servercert,char *serverpwd, char *privatekey, int socketno);
#endif
 /*****************************************************************************/
/* Types
 *****************************************************************************/

/*****************************************************************************/
/* Function Prototypes
/*****************************************************************************/

/* Utility functions used by this example */
static int http_server_demo_restart(void);


static int http_server_create_virtual_form(HTTPServerContext *phttpServer);

static char **http_demo_generate_vcontent(HTTPServerContext  *httpServer,HTTP_INT32 virtUrlId);
static void http_demo_release_vcontent(HTTP_INT32 virtUrlId, char **pContent);

char **http_demo_retrieve_vcontent(HTTPServerContext  *httpServer,HTTP_INT32 contentid);


/* Imported */
void _Demo_UrlRequestDestructor (void *userData);
int _Demo_UrlRequestHandler (void *userData, HTTPServerRequestContext *ctx,  HTTPSession *session, HTTPRequest *request, RTP_NET_ADDR *clientAddr);
HTTP_CHAR *http_demo_alloc_string(HTTP_CHAR *string);

/*****************************************************************************/
/* Data
 *****************************************************************************/

/*****************************************************************************/
/* Function Definitions
 *****************************************************************************/
static void http_demo_write_vcontent_bytes(HTTPSession *session, char **pContent);
static HTTP_INT32 http_demo_count_vcontent_bytes(char **pContent);

/*---------------------------------------------------------------------------*/
#ifdef CALL_SSL_MAIN
int  HTTPS_ServerProcessOneRequest (RTP_SSL_CONTEXT sslContext,
                                    HTTPServerContext *server,
                                    HTTP_INT32 timeoutMsec);

#endif

/* Main entry point for the web server application.
	A callable web server.
*/

int http_advanced_server_demo(void)
{
	HTTP_INT16 ipType = DEMO_IPVERSION;
	int idle_count = 0;
	rtp_net_init();
	rtp_threads_init();
#ifdef CALL_SSL_MAIN
	if (HTTP_AuthContextInit(&Authctx) < 0)
		return(-1);
	pAuthctx = &Authctx;


	if (rtp_ssl_init_context (
		&sslContext,
		RTP_SSL_MODE_SERVER,
		RTP_SSL_VERIFY_PEER|RTP_SSL_VERIFY_CLIENT_ONCE
	//	unsigned int verifyMode
	) < 0)
		return(-1);
#endif

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

	/* Initialize the server.
        Establish processing callbacks.
        Register memory based web pages.
	*/
	if (http_server_demo_restart() != 0)
		return(-1);
#ifdef CALL_SSL_MAIN
		// ssl_server_main(char *Suite/*'a','c','n'*/, int sslmethod/* 1,2,3,4 */,char *servercert,char *serverpwd, char *privatekey, int socketno);
		ssl_server_main((SSL_CTX *)sslContext, "a", 4,0,0,0,ExampleServerCtx.httpServer.serverSocket);
#endif


//========================
	/* Now loop continuously process one request per loop. */
	for (;;)
	{
#ifdef CALL_SSL_MAIN
        if (HTTPS_ServerProcessOneRequest (sslContext,
                                            &ExampleServerCtx.httpServer,
                                            1000*60) < 0)
#else
		if (HTTP_ServerProcessOneRequest (&ExampleServerCtx.httpServer, 1000*60) < 0)
#endif
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

		if (ExampleServerCtx.ModuleRequestingReload)
		{
			ExampleServerCtx.ModuleRequestingReload = 0;
			HTTP_ServerDestroy(&ExampleServerCtx.httpServer, &ExampleServerCtx.connectCtxArray);
			rtp_free(ExampleServerCtx.connectCtxArray);
			ExampleServerCtx.connectCtxArray = 0;
			/* Initialize the server */
			if (http_server_demo_restart() != 0)
			{
				rtp_net_exit();
				return(-1);
			}
		}
	}

	rtp_net_exit();
	return (0);

}


/* ========================================================================= */
/* Virtual form code starts here. Demonstrates how to serve on the fly
   forms and how to process form data */

static char *demo_form_content;
static char *virtual_romindex_content;

static char *ajax_content[]  = {
"<html><body><hr><center><b><big>Not done yet</big></b></center><hr><br><br>",
"<br><a href=\"index.htm\">Click here to return to main index.</a><br>",
"</body></html>",
0};


static int demo_form_cb (void *userData, HTTPServerRequestContext *ctx, HTTPSession *session, HTTPRequest *request, RTP_NET_ADDR *clientAddr);
static int ajax_update_demo_cb (void *userData, HTTPServerRequestContext *ctx, HTTPSession *session, HTTPRequest *request, RTP_NET_ADDR *clientAddr);

static int smb_client_cb (void *userData, HTTPServerRequestContext *ctx, HTTPSession *session, HTTPRequest *request, RTP_NET_ADDR *clientAddr);


/* Start or restart the server. Called when the application is first run and when callback handlers
   request a restart of the server because parametrs have been changed */
static int http_server_demo_restart(void)
{
	HTTP_INT16 ipType = DEMO_IPVERSION;	 /* The API supports V6 but the demo is wired for V4 right now */
	HTTP_UINT8     *pDefaultIpAddr;

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
			8080,  // 80,               // 80  ?? Use the www default port
			DEMO_IPVERSION,   // ipversion type(4 or 6)
			0,                // allowKeepAlive
			ExampleServerCtx.connectCtxArray,  // connectCtxArray
			ExampleServerCtx.numConnections,   // maxConnections
			ExampleServerCtx.numHelperThreads  // maxHelperThreads
		) < 0)
		return -1;

	/* Create a homepage romindex.html or romindex.htm */
	if (HTTP_ServerAddVirtualFile(&ExampleServerCtx.httpServer, "\\romindex.html",  virtual_romindex_content, rtp_strlen(virtual_romindex_content), "text/html") < 0)
		return -1;
	if (HTTP_ServerAddVirtualFile(&ExampleServerCtx.httpServer, "\\romindex.htm",  virtual_romindex_content, rtp_strlen(virtual_romindex_content), "text/html") < 0)
		return -1;

	/* Create "demo_form.html" uses a ram based web page and demonstrates forms processing */
	if (HTTP_ServerAddVirtualFile(&ExampleServerCtx.httpServer, "\\demo_form.htm",  demo_form_content, rtp_strlen(demo_form_content), "text/html") < 0)
		return -1;
	/* Create post handler for for demo_form.html if the browser posts with action="demo_form_submit". User data is not used in the logic but we
       pass demo_form_cb() a string ("example user data") for example purposes */
	if (HTTP_ServerAddPostHandler(&ExampleServerCtx.httpServer, "\\demo_form_submit", demo_form_cb, (void *)"example user data") < 0)
		return -1;
	rtp_printf("A virtual page named demo_form.html and a Post Handler names demo_form_submit have been created \n");

	/* Create an ajax applet that accepts value updates from a browser via POST and returns a modified
       value (multiplied by 4) in a reply.
       The client applications Html5Demo.html and Html4Demo.html use the reply data  from "\\demo_ajax_getval"
       to update their screens.
	*/
     /*  Add a url that updates a value stored in the variable AjaxDemoValue. The value is poste by the Html5Demo.html example when the slider is moved */
	if (HTTP_ServerAddPostHandler(&ExampleServerCtx.httpServer, "\\demo_ajax_setval_submit", ajax_update_demo_cb, (void *)"set_value") < 0)
		return -1;
     /*  Add a url that updates a value stored in the variable AjaxDemoValue. The value is posted by the Html4Demo.html example when a table cell is clicked. */
	if (HTTP_ServerAddPostHandler(&ExampleServerCtx.httpServer, "\\demo_ajax_command_submit", ajax_update_demo_cb, (void *)"process_command") < 0)
		return -1;
     /*  Add a url that returns the current tick count and the current value stored in the variable AjaxDemoValue. Html4Demo.html and Html5Demo.html retrieve these
         an update their screens. */
	if (HTTP_ServerAddPostHandler(&ExampleServerCtx.httpServer, "\\demo_ajax_getval", ajax_update_demo_cb, (void *)"get_value") < 0)
		return -1;
	rtp_printf("Post Handlers for demo_ajax_setval_submit, demo_ajax_command_submit and demo_ajax_getval have been assigned\n");


	/* Create an ajax applet that accepts value updates from a browser via POST and returns a modified
       value (multiplied by 4) in a reply.
       The client applications Html5Demo.html and Html4Demo.html use the reply data  from "\\demo_ajax_getval"
       to update their screens.
	*/
     /*  Add a url that accepts a command in string form. The value is posted by the SmbClientConsole.html when a command request is made. */
	if (HTTP_ServerAddPostHandler(&ExampleServerCtx.httpServer, "\\smb_client_command", smb_client_cb, (void *)"process_command") < 0)
		return -1;
	rtp_printf("Post Handler for smb_client command has been assigned\n");
     /*  Add a url that returns accumulated output to a "window" */
	if (HTTP_ServerAddPostHandler(&ExampleServerCtx.httpServer, "\\smb_client_console", smb_client_cb, (void *)"get_console_output") < 0)
		return -1;
	rtp_printf("Post Handler for smb_client console has been assigned\n");


     /*  Add a url that returns accumulated output to a "window" */
	if (HTTP_ServerAddPostHandler(&ExampleServerCtx.httpServer, "\\smb_client_json", smb_client_cb, (void *)"get_send_json") < 0)
		return -1;
	rtp_printf("Post Handler for smb_client console has been assigned\n");

	/* Create a virtual default web page that we'll use as a sign on instructional page. */
	if (HTTP_ServerAddPath(&ExampleServerCtx.httpServer, DEMO_CONFIGURE_URL, 1, (HTTPRequestHandler)_Demo_UrlRequestHandler,
	                        (HTTPServerPathDestructor)_Demo_UrlRequestDestructor,   (void *)DEMO_CONFIGURE_ID) < 0)
		return (-1);


	/* Make the server call _Demo_UrlRequestHandler when ajax.html is accessed. Pass it DEMO_INFO_ID*/
	if (HTTP_ServerAddPath(&ExampleServerCtx.httpServer, DEMO_INFO_URL, 1, (HTTPRequestHandler)_Demo_UrlRequestHandler,
	                        (HTTPServerPathDestructor)_Demo_UrlRequestDestructor,   (void *)DEMO_INFO_ID) < 0)
		return (-1);
	return (0);
}

static int _Demo_IndexFileProcessHeader(void *userData,	HTTPServerRequestContext *server, HTTPSession *session,HTTPHeaderType type, const HTTP_CHAR *name,	const HTTP_CHAR *value);



/*---------------------------------------------------------------------------*/
int             _HTTP_ServerHeaderCallback      (void *userData,
                                                 HTTPSession *session,
                                                 HTTPHeaderType type,
                                                 const HTTP_CHAR *name,
                                                 const HTTP_CHAR *value);



static int demo_form_cb (void *userData, HTTPServerRequestContext *ctx, HTTPSession *session, HTTPRequest *request, RTP_NET_ADDR *clientAddr)
{
int processing_configure = 0;
HTTPServerContext *server = ctx->server;
HTTP_CHAR formName[256];

	{
		HTTP_CHAR *p;
		p = rtp_strstr(request->target, "?");
		if (p)
		{
			rtp_strncpy(formName, request->target, (int)(p-request->target));
			formName[(int) (p-request->target)]=0;
		}
		else
		{
			rtp_strcpy(formName, request->target);
		}
	}
	rtp_printf("formName from URL == %s\n", formName);
	rtp_printf("user data  == %s\n", userData);


	switch (request->methodType)
	{
		case HTTP_METHOD_POST:
        {
			HTTPResponse response;
			HTTP_INT32 bytesRead=-1;
			HTTP_UINT8 tempBufferRaw[256];
				if (HTTP_ReadHeaders(session, _HTTP_ServerHeaderCallback, ctx, tempBufferRaw,sizeof(tempBufferRaw)) >= 0)
				{
					bytesRead = HTTP_Read(session,tempBufferRaw,sizeof(tempBufferRaw));
				}
				HTTP_ServerInitResponse(ctx, session, &response, 200, "OK");
				HTTP_ServerSetDefaultHeaders(ctx, &response);
				HTTP_SetResponseHeaderStr(&response, "Content-Type", "text/html");
				HTTP_WriteResponse(session, &response);
				{
				HTTP_UINT8 Buffer[64];
				rtp_sprintf(Buffer,"Received a post of %d bytes.<br>Value pairs are:<br>",bytesRead);
				HTTP_Write(session, Buffer, rtp_strlen(Buffer));
				// if (bytesRead > 0)
				//	 HTTP_Write(session, tempBufferRaw, bytesRead);
                // ==========================
                // Display name value pairs of posted data
				if (bytesRead > 0)
				{
					NVPairList PairList;
                    NVPair PairArray[10];
                    int n,escapedlen,consumed;
					HTTP_UINT8 tempBuffer[256];
					tempBufferRaw[bytesRead]=0;

					/* & characters are converted to zeroes for the convenience of name value pair parser. */
					escapedlen = HTML_UnEscapeFormString(tempBuffer,tempBufferRaw);
                    HTTP_InitNameValuePairList (&PairList, PairArray, 10);
                    consumed = HTTP_LoadNameValuePairs (tempBuffer, escapedlen, &PairList);
                    for (n=0; HTTP_GetNameValuePairIndexed (&PairList,n); n++)
                    {
					    HTTP_UINT8 *name = (HTTP_UINT8 *)  HTTP_GetNameValuePairIndexed(&PairList,n)->name;
					    HTTP_UINT8 *value = (HTTP_UINT8 *) HTTP_GetNameValuePairIndexed(&PairList,n)->value;
    					if (name && value)
    					{
						HTTP_UINT8 respBuffer[256];
    						if (processing_configure)
    						{
    							if (rtp_strcmp(name, "root") == 0)
    							{
    								rtp_strcpy(ExampleServerCtx.rootDirectory, value);
    								ExampleServerCtx.ModuleRequestingReload = 1;
    							}
    						}
    						rtp_strcpy(respBuffer, name);
    						rtp_strcat(respBuffer, " = ");
    						rtp_strcat(respBuffer, value);
    						rtp_strcat(respBuffer, " <br>");
							HTTP_Write(session, respBuffer, rtp_strlen(respBuffer));
    					}
    				}
                }
                // ==========================

				}
				HTTP_WriteFlush(session);
				HTTP_FreeResponse(&response);
				return (HTTP_REQUEST_STATUS_DONE);
                break;
        }
		case HTTP_METHOD_GET:
		{
			HTTPResponse response;
			HTTP_UINT8 tempBuffer[256];
			HTTP_INT32 tempBufferSize = 256;
			HTTP_UINT8 *respBuffer;
			HTTP_INT32 respBufferSize = 4096;

			HTTP_CHAR *p;
			int pairs = 0;

			if (rtp_strcmp("demo_configure_submit", (char *) userData) == 0)
			{
				processing_configure = 1;
			}

			/* Allocate a buffer for sending response. */
			respBuffer = (HTTP_UINT8 *) rtp_malloc(respBufferSize);
			*respBuffer = 0;
			/* Parse the submittd values from the command line and send them back to the user in a document */
			p = rtp_strstr(request->target, "?");
			if (p)
			{
			HTTP_UINT8 *name;
			HTTP_UINT8 *value;
			NVPairList PairList;
            NVPair PairArray[10];
            PNVPair pSetValPair;
            int escapedlen,consumed,n;

				p++;
			    /* & characters are converted to zeroes for the convenience of name value pair parser. */
				escapedlen = HTML_UnEscapeFormString(tempBuffer,p);
                HTTP_InitNameValuePairList (&PairList, PairArray, ARRAYSIZE(PairArray));
                consumed = HTTP_LoadNameValuePairs (tempBuffer, escapedlen, &PairList);

                pSetValPair=HTTP_GetNameValuePairAssociative (&PairList,"AjaxSetVal");

				rtp_strcat(respBuffer, "<hr>You submitted these values: <hr>");
                 pSetValPair=HTTP_GetNameValuePairIndexed (&PairList,0);
				for (n=0; HTTP_GetNameValuePairIndexed (&PairList,n); n++)
				{
					name = (HTTP_UINT8 *)  HTTP_GetNameValuePairIndexed(&PairList,n)->name;
                    value = (HTTP_UINT8 *) HTTP_GetNameValuePairIndexed(&PairList,n)->value;
					if (name && value)
					{
						if (processing_configure)
						{
							if (rtp_strcmp(name, "root") == 0)
							{
								rtp_strcpy(ExampleServerCtx.rootDirectory, value);
								ExampleServerCtx.ModuleRequestingReload = 1;
							}
						}
						rtp_strcat(respBuffer, name);
						rtp_strcat(respBuffer, " = ");
						rtp_strcat(respBuffer, value);
						rtp_strcat(respBuffer, "<br>");
					}
				}
			}

			/* Read the HTTP headers into an NV pair list. Dynamically allocate 2048 bytes to hold the header contents.
               This memory block is structured and the Headers object is populated with null terminated header name value pairs.
			   To reduce memory requirements a filter function can be passed in that selectively decides whether to keep elements.
			*/
            {
                NVHeaderList Headers;
                int n,HeaderBufferSize;
                HTTP_UINT8 *  HeaderBuffer = (HTTP_UINT8 *) rtp_malloc(2048);
                HeaderBufferSize = 2048;

                HTTP_InitHeaderNameValuePairList ( &Headers,HeaderBuffer,HeaderBufferSize,0,0);
                HTTP_ServerReadHeaders(	ctx, session, HTTP_CallbackHeaderToNameValuePairList, &Headers, tempBuffer, tempBufferSize);

                rtp_sprintf(respBuffer+rtp_strlen(respBuffer), "<hr>Your browser passed these HTTP header values in %d bytes: <hr>", Headers.BufferUsed);
				for (n=0; HTTP_GetNameValuePairIndexed (&Headers.PairList,n); n++)
				{
				    HTTP_CHAR *name = (HTTP_UINT8 *)  HTTP_GetNameValuePairIndexed(&Headers.PairList,n)->name;
				    HTTP_CHAR *value = (HTTP_UINT8 *) HTTP_GetNameValuePairIndexed(&Headers.PairList,n)->value;
					if (name && value)
					{
                        rtp_strcat(respBuffer, name);
                        rtp_strcat(respBuffer, " = ");
                        rtp_strcat(respBuffer, value);
                        rtp_strcat(respBuffer, "<br>");
					}
				}
				rtp_free(HeaderBuffer);
            }

			if (processing_configure)
				rtp_strcat(respBuffer, "<hr>The configruation was changed. To test your results select the option to revert to disk based html from the main page.<br><br>");
			rtp_strcat(respBuffer, "<hr>Thank you ... and have a nice day<hr>");
			rtp_strcat(respBuffer, "<br><a href=\"romindex.htm\">Click here to return to main index.</a><br>");

			HTTP_ServerInitResponse(ctx, session, &response, 200, "OK");
			HTTP_ServerSetDefaultHeaders(ctx, &response);
			HTTP_SetResponseHeaderStr(&response, "Content-Type", "text/html");

			HTTP_SetResponseHeaderInt(&response, "Content-Length", rtp_strlen(respBuffer));

			HTTP_WriteResponse(session, &response);
			HTTP_Write(session, respBuffer, rtp_strlen(respBuffer));
			HTTP_WriteFlush(session);

			rtp_free(respBuffer);
			HTTP_FreeResponse(&response);

			return (HTTP_REQUEST_STATUS_DONE);
		}

		case HTTP_METHOD_HEAD: /* tbd - implement HEAD method */
		default:
			return (HTTP_REQUEST_STATUS_CONTINUE);
	}
}

int MyHeaderCallback(
		void *userData,
		HTTPServerRequestContext *ctx,
		HTTPSession *session,
		HTTPHeaderType type,
		const HTTP_CHAR *name,
		const HTTP_CHAR *value
	)
{
	printf("Name == %s: Value ==:%s\n", name, value);
	return 0;
}
#define TEST_JSON 0
#if (TEST_JSON)
#define MAXVALS 200
static int value_array[MAXVALS];
static int value_count=0;

static void log_json_val(int new_time,int new_val)
{
   if (value_count < MAXVALS)
     value_array[value_count++] = new_val;
}

static void formatjson_val( char *buff)
{
  int index;
  char *b= buff;
//    b += rtp_sprintf(b,"<html><body><b>Systick [%d] <br><b>",rtp_get_system_msec(), AjaxDemoValue);
  	b += rtp_sprintf(b,"[");
    if(1)
    {
    for (index = 0; index < 400; index++)
	{
      int i = (int) 128*sin(index*0.0174532925);
      b += rtp_sprintf(b,"{\"x\":%d, \"y\":%d}",index, i);
      if (index < 400)
        b += rtp_sprintf(b,",");
//	  HTTP_Write(session, buff, rtp_strlen(buff));
	}
    }
    if(0)
    for (index = 0; index < value_count; index++)
	{
      b += rtp_sprintf(b,"{\"x\":%d, \"y\":%d}", index,value_array[index]);
	  if (index < value_count)
		  b += rtp_sprintf(b,",");
//	  HTTP_Write(session, buff, rtp_strlen(buff));
	}
  	b += rtp_sprintf(b,"]");
//    b += rtp_sprintf(b,"</b></body></html>",rtp_get_system_msec(), AjaxDemoValue);
}

#endif
static int ajax_update_demo_cb (void *userData, HTTPServerRequestContext *ctx, HTTPSession *session, HTTPRequest *request, RTP_NET_ADDR *clientAddr)
{
int processing_configure = 0;
HTTPServerContext *server = ctx->server;
HTTP_CHAR formName[256];
HTTP_CHAR cgiArgs[256];
    cgiArgs[0]=0;
	{
		HTTP_CHAR *p;
		p = rtp_strstr(request->target, "?");
		if (p)
		{
			rtp_strncpy(formName, request->target, (int)(p-request->target));
			formName[(int)(p-request->target)]=0;
			rtp_strcpy(cgiArgs, p+1);
		}
		else
		{
			rtp_strcpy(formName, request->target);
		}
	}
	rtp_printf("request->target  == %s\n", request->target);
	rtp_printf("formName from URL == %s\n", formName);
	rtp_printf("user date  == %s\n", userData);
	rtp_printf("cgiArgs  == %s\n", cgiArgs);

	switch (request->methodType)
	{
		case HTTP_METHOD_GET:
		{
			HTTPResponse response;
			HTTP_UINT8 *respBuffer;
			HTTP_INT32 respBufferSize = 40960;

			{
			HTTP_UINT8 tempBufferRaw[256];
			ctx->userCallback = MyHeaderCallback;
			printf("==========================================================================================\n");
			HTTP_ReadHeaders(session, _HTTP_ServerHeaderCallback, ctx, tempBufferRaw,sizeof(tempBufferRaw));
			printf("==========================================================================================\n");
			}

			/* Allocate a buffer for sending response. */
			respBuffer = (HTTP_UINT8 *) rtp_malloc(respBufferSize);
			*respBuffer = 0;
#if (TEST_JSON)
     		if (value_count > 100)
            {
			   formatjson_val(respBuffer);
               value_count = 0;
            }
#else
			sprintf(respBuffer,"<html><body><b>Systick [%d] <br>PanelValue [%d]</b></body></html>",rtp_get_system_msec(), AjaxDemoValue);
#endif
			HTTP_ServerInitResponse(ctx, session, &response, 200, "OK");
			HTTP_ServerSetDefaultHeaders(ctx, &response);
			HTTP_SetResponseHeaderStr(&response, "Content-Type", "text/html");
			HTTP_SetResponseHeaderInt(&response, "Content-Length", rtp_strlen(respBuffer));
			HTTP_WriteResponse(session, &response);
			HTTP_Write(session, respBuffer, rtp_strlen(respBuffer));
			HTTP_WriteFlush(session);
			rtp_free(respBuffer);
			HTTP_FreeResponse(&response);
			return (HTTP_REQUEST_STATUS_DONE);
		}
		case HTTP_METHOD_POST:
        {
			HTTP_INT32 bytesRead;
			HTTP_UINT8 tempBufferRaw[256];
			HTTP_UINT8 tempBuffer[256];
			ctx->userCallback = MyHeaderCallback;
			if (HTTP_ReadHeaders(session, _HTTP_ServerHeaderCallback, ctx, tempBufferRaw,sizeof(tempBufferRaw)) >= 0)
			{

				bytesRead = HTTP_Read(session,tempBufferRaw,sizeof(tempBufferRaw));
				if (bytesRead > 0)
				{
					NVPairList PairList;
                    NVPair PairArray[10];
                    PNVPair pSetValPair;
                    int escapedlen,consumed;
					tempBufferRaw[bytesRead]=0;
					/* & characters are converted to zeroes for the convenience of name value pair parser. */
					escapedlen = HTML_UnEscapeFormString(tempBuffer,tempBufferRaw);
                    HTTP_InitNameValuePairList (&PairList, PairArray, 10);
                    consumed = HTTP_LoadNameValuePairs (tempBuffer, escapedlen, &PairList);

                    pSetValPair=HTTP_GetNameValuePairAssociative (&PairList,"AjaxSetVal");

					if (pSetValPair)
					{
						if (userData && rtp_strcmp((HTTP_CHAR *)userData, "process_command")==0)
						{
						    if (rtp_strcmp(pSetValPair->value, "clear")==0)
							    AjaxDemoValue = 0;
							else if (rtp_strcmp(pSetValPair->value, "+10")==0)
							    AjaxDemoValue +=10;
							else if (rtp_strcmp(pSetValPair->value, "-10")==0)
							    AjaxDemoValue -=10;
							else if (rtp_strcmp(pSetValPair->value, "+100")==0)
							    AjaxDemoValue +=100;
							else if (rtp_strcmp(pSetValPair->value, "-100")==0)
							    AjaxDemoValue -=100;
                        }
						else if (userData && rtp_strcmp((HTTP_CHAR *)userData, "set_value")==0)
                        {
							AjaxDemoValue=rtp_atol(pSetValPair->value);
                        }
						rtp_printf("Changed V to %d\n", AjaxDemoValue);
#if (TEST_JSON)
						log_json_val(0,AjaxDemoValue);
#endif
					}
				}

				{
					HTTPResponse response;
					HTTP_ServerInitResponse(ctx, session, &response, 201, "OK");
					HTTP_WriteResponse(session, &response);
					{
					char buff[80];
						rtp_sprintf(buff,"%d\n",AjaxDemoValue*4);
					HTTP_Write(session, buff, rtp_strlen(buff));
					}
//					HTTP_Write(session, tempBufferRaw, rtp_strlen(tempBufferRaw));
					HTTP_WriteFlush(session);
					HTTP_FreeResponse(&response);
				}
			}
            return (HTTP_REQUEST_STATUS_DONE);
       }
		default:
			return (HTTP_REQUEST_STATUS_DONE);
    }
}



static int smb_cli_shell_proc_web(NVPairList *pPairList, char *outbuffer, int outbufferLength)
{
   int i;
   int r=-1;
   NVPair *pCommand,*pMedia;

   rtp_strcpy(outbuffer, "Command Not Found");
   return 1;

   pCommand = HTTP_GetNameValuePairAssociative (pPairList,"command");
   pMedia = HTTP_GetNameValuePairAssociative (pPairList,"media");
#if (0)
   // media (remote|local) and
   if (!pCommand)
   {
       return -1;
   }
   {
       for (i =0; web_cmds[i]; i++)
       {
           if (rtp_strnicmp(pCommand->value, web_cmds[i], rtp_strlen(web_cmds[i])) == 0)
           {
               r = web_functions[i](pPairList,outbuffer, outbufferLength);
               break;
           }
       }
   }
#endif
   r = 1;
   return r;
}

//=====================================

static char SmbCliConsoleOutput[4092];
static int SmbCliConsoleLength;
static char SmbCliConsoleInput[256];
static int SmbCliConsoleInputLength;

void HttpConsoleOutPut(char *output)
{
int linelen = rtp_strlen(output);
//    if (output[linelen]=='\n')
//    {
//        linelen--;
//    }
    if (linelen + SmbCliConsoleLength < sizeof(SmbCliConsoleOutput))
    {
        rtp_strcpy(&SmbCliConsoleOutput[SmbCliConsoleLength], output);
        SmbCliConsoleLength += linelen;
    }
}

static void HttpSetConsoleInPut(char *input)
{
   rtp_strcpy(SmbCliConsoleInput,input);
}
char *HttpGetConsoleInPut(char *inBuf)
{
   if (SmbCliConsoleInput[0])
   {
    rtp_strcpy(inBuf, SmbCliConsoleInput);
    SmbCliConsoleInput[0]=0;
   }
   else
   rtp_thread_sleep(100);
}



extern int smb_cli_shell_proc_web(NVPairList *pPairList, char *outbuffer, int outbufferLength);

static int smb_client_cb (void *userData, HTTPServerRequestContext *ctx, HTTPSession *session, HTTPRequest *request, RTP_NET_ADDR *clientAddr)
{
int processing_configure = 0;
HTTPServerContext *server = ctx->server;
HTTP_CHAR formName[256];
HTTP_CHAR cgiArgs[256];
HTTP_CHAR tempBuffer[256];
NVPairList PairList;
NVPair PairArray[10];

    cgiArgs[0]=0;
	{
		HTTP_CHAR *p;
		p = rtp_strstr(request->target, "?");
		if (p)
		{
			rtp_strncpy(formName, request->target, (int)(p-request->target));
			formName[(int)(p-request->target)]=0;
			rtp_strcpy(cgiArgs, p+1);
		}
		else
		{
			rtp_strcpy(formName, request->target);
		}
	}
	rtp_printf("request->target  == %s\n", request->target);
	rtp_printf("formName from URL == %s\n", formName);
	rtp_printf("user date  == %s\n", userData);
	rtp_printf("cgiArgs  == %s\n", cgiArgs);

	HTTP_InitNameValuePairList (&PairList, PairArray, ARRAYSIZE(PairArray));

	switch (request->methodType)
	{
		case HTTP_METHOD_GET:
		{
			HTTPResponse response;
			HTTP_UINT8 *respBuffer=0;
			HTTP_INT32 respBufferSize = 4096;
			int escapedlen = HTML_UnEscapeFormString(tempBuffer,cgiArgs);
			int consumed = HTTP_LoadNameValuePairs (tempBuffer, escapedlen, &PairList);
            int error=0;
            int n,r;
            NVPair *pCommand,*pMedia;

            pCommand = HTTP_GetNameValuePairAssociative (&PairList,"command");
            pMedia = HTTP_GetNameValuePairAssociative (&PairList,"media");

			/* Allocate a buffer for sending response. */
			respBuffer = (HTTP_UINT8 *) rtp_malloc(respBufferSize);
			*respBuffer = 0;

            r = smb_cli_shell_proc_web(&PairList, respBuffer, respBufferSize);

#if (0)

            if (rtp_strcmp(pMedia->value,"remote")==0)
            {
            int r;
                rtp_printf("Procesing %s\n", pCommand->value);
                r = smb_cli_shell_proc_web(&PairList, respBuffer, respBufferSize);
                rtp_printf("Result == %d\n", r);
                return (HTTP_REQUEST_STATUS_DONE);

                if (rtp_strcmp(pCommand->value,"diropen"))
                {
                // HEREHERE - Need to do a dir and json encode results
                }
                else if (rtp_strcmp(pCommand->value,"stat"))
                {

                }
                else
                    rtp_printf("Reguest = %s\n", "Not found");
            }
            for (n=0; HTTP_GetNameValuePairIndexed (&PairList,n); n++)
            {
			    HTTP_UINT8 *name = (HTTP_UINT8 *)  HTTP_GetNameValuePairIndexed(&PairList,n)->name;
			    HTTP_UINT8 *value = (HTTP_UINT8 *) HTTP_GetNameValuePairIndexed(&PairList,n)->value;
    			if (name && value)
    			{
                    rtp_printf("Got %s=%s\n", name,value);
                }
            }
//			sprintf(respBuffer,"<html><body><b>Systick [%d] <br>PanelValue [%d]</b></body></html>",rtp_get_system_msec(),  "I am console data for you <br>");
			if (rtp_strcmp(formName,"/smb_client_console")== 0)
			{
                if (*SmbCliConsoleOutput)
                {
			        sprintf(respBuffer,"%s", SmbCliConsoleOutput);
			        SmbCliConsoleLength=0;
			        SmbCliConsoleOutput[0]=0;
                }
                else
			        sprintf(respBuffer,"%s", "neep");
            }
            else
            {
			    sprintf(respBuffer,"%d  Unknown request %s<br>",rtp_get_system_msec(),  request->target);
            }
#endif
            if (r < 0)
            {
                HTTP_ServerSendError (ctx, session, 400, respBuffer);
            }
            else
            {
			    HTTP_ServerInitResponse(ctx, session, &response, 200, "OK");
			    HTTP_ServerSetDefaultHeaders(ctx, &response);
			    HTTP_SetResponseHeaderStr(&response, "Content-Type", "text/html");
			    HTTP_SetResponseHeaderInt(&response, "Content-Length", rtp_strlen(respBuffer));
			    HTTP_WriteResponse(session, &response);
			    HTTP_Write(session, respBuffer, rtp_strlen(respBuffer));
			    HTTP_WriteFlush(session);
			    rtp_free(respBuffer);
			    HTTP_FreeResponse(&response);
            }
			return (HTTP_REQUEST_STATUS_DONE);
		}
		case HTTP_METHOD_POST:
        {
			HTTP_INT32 bytesRead;
			HTTP_UINT8 tempBufferRaw[256];
			HTTP_UINT8 tempBuffer[256];
			ctx->userCallback = MyHeaderCallback;
			if (HTTP_ReadHeaders(session, _HTTP_ServerHeaderCallback, ctx, tempBufferRaw,sizeof(tempBufferRaw)) >= 0)
			{
				bytesRead = HTTP_Read(session,tempBufferRaw,sizeof(tempBufferRaw));
				if (bytesRead > 0)
				{
					NVPairList PairList;
                    NVPair PairArray[10];
                    PNVPair pSetValPair;
                    int escapedlen,consumed;
					tempBufferRaw[bytesRead]=0;
					/* & characters are converted to zeroes for the convenience of name value pair parser. */
					escapedlen = HTML_UnEscapeFormString(tempBuffer,tempBufferRaw);
                    HTTP_InitNameValuePairList (&PairList, PairArray, 10);
                    consumed = HTTP_LoadNameValuePairs (tempBuffer, escapedlen, &PairList);

                    pSetValPair=HTTP_GetNameValuePairAssociative (&PairList,"AjaxSetVal");

					if (pSetValPair)
					{
						if (userData && rtp_strcmp((HTTP_CHAR *)userData, "process_command")==0)
						{
						    rtp_printf("Execute Command: %s\n", pSetValPair->value);
                            HttpSetConsoleInPut(pSetValPair->value);
                        }
						else if (userData) // && rtp_strcmp((HTTP_CHAR *)userData, "set_value")==0)
                        {
						    rtp_printf("Unkown Command: %s\n", (HTTP_CHAR *)userData);
                        }
					}
				}
				{
					HTTPResponse response;
					HTTP_ServerInitResponse(ctx, session, &response, 201, "OK");
					HTTP_WriteResponse(session, &response);
					{
					char buff[80];
						rtp_sprintf(buff,"%s\n","Yeah you did it");
					HTTP_Write(session, buff, rtp_strlen(buff));
					}
//					HTTP_Write(session, tempBufferRaw, rtp_strlen(tempBufferRaw));
					HTTP_WriteFlush(session);
					HTTP_FreeResponse(&response);
				}
			}
            return (HTTP_REQUEST_STATUS_DONE);
       }
		default:
			return (HTTP_REQUEST_STATUS_DONE);
    }
}






int _Demo_UrlRequestHandler (void *userData,HTTPServerRequestContext *ctx,
                                     HTTPSession *session,
                                     HTTPRequest *request,RTP_NET_ADDR *clientAddr);
void _Demo_UrlRequestDestructor (void *userData);
static int _Demo_IndexFileProcessHeader(void *userData,	HTTPServerRequestContext *server, HTTPSession *session,HTTPHeaderType type, const HTTP_CHAR *name,	const HTTP_CHAR *value);




/*---------------------------------------------------------------------------*/
void _Demo_UrlRequestDestructor (void *userData)
{
}

/* Compiled in web pages

  virtual_index_content[] - Contains html content that is displayed when the browser requests an index file using the
							file names index.htm, index.html or a null page.

							If any of these files is accessed the call callback routine _Demo_UrlRequestHandler()
							(see below. Is called).

  							Processing for the other urls in the virtual_index_content array are installed by other
  							http_demo_init_vcontent(). Povided in a sperate module in this directory.

							The additional examples are segregated in addiditional fils to uncouple the example
							content from the structure as muich as possible.
  */


int _Demo_UrlRequestHandler (void *userData,
                                     HTTPServerRequestContext *ctx,
                                     HTTPSession *session,
                                     HTTPRequest *request,
                                     RTP_NET_ADDR *clientAddr)
{
	HTTPServerContext *server = ctx->server;
	HTTP_INT32 virtUrlId;

	if (ExampleServerCtx.disableVirtualIndex)
		return(HTTP_REQUEST_STATUS_CONTINUE);
	/* We passed an integer constant to identify the URL, use that to retrieve the html source and to perform any special processing */
	virtUrlId = (HTTP_INT32) userData;

	/* If the virtual index is disabled tell the server to get index.html index.htm aind \\ fro the file store */
    if (virtUrlId == 0 && ExampleServerCtx.disableVirtualIndex)
		return(HTTP_REQUEST_STATUS_CONTINUE);
	switch (request->methodType)
	{
		case HTTP_METHOD_GET:
		{
			HTTPResponse response;
			HTTP_UINT8 tempBuffer[256];
			HTTP_INT32 tempBufferSize = 256;
			HTTP_INT32 vfilesize;
			char **virtual_content;
			static char *virtual_content_store[32];
			if (request->target)
				rtp_printf("Ip [%d.%d.%d.%d] Requesting URL:%s \n",
					clientAddr->ipAddr[0],	clientAddr->ipAddr[1],
					clientAddr->ipAddr[2], clientAddr->ipAddr[3], request->target);

			/* Handle 0 is the default index page */
			if (virtUrlId == DEMO_INFO_ID)
			{
				int line = 0;
                virtual_content_store[line++] =	http_demo_alloc_string("<html><body><hr><center><b><big>Web Server Information.</big></b></center><hr><br><br>");
                virtual_content_store[line++] =	http_demo_alloc_string("<ul>");
                virtual_content_store[line++] =	http_demo_alloc_string("<li>'C' based handlers for POST and GET are provided in example_server_advanced.c<br></li>");
                virtual_content_store[line++] =	http_demo_alloc_string("<li>Disk based content may be placed in and displayed from the directory \\htdocs by default.<br></li>");
                virtual_content_store[line++] =	http_demo_alloc_string("<li>Try these disk based pages by typing for example: localhost/Html5Demo.html</li><ul>");
                virtual_content_store[line++] =	http_demo_alloc_string("<li>//Html5Demo.html   - Uses HTML5 and Ajax to create a dynamic networked application.</li>");
                virtual_content_store[line++] =	http_demo_alloc_string("<li>//Html4Demo.html   - Uses HTML4 and Ajax to create a dynamic networked application.</li></ul>");
	            virtual_content_store[line++] =	http_demo_alloc_string("<li>These Virtual pages and URLs are built into the server.<br></li><ul>");
                virtual_content_store[line++] =	http_demo_alloc_string("<li>//romindex.html    - This rom based page contains the main index page for the advanced server demo.</li>");
                virtual_content_store[line++] =	http_demo_alloc_string("<li>//romindex.htm     - This is an alias for romindex.html.</li>");
                 virtual_content_store[line++] =http_demo_alloc_string("<li>//info.html        - The current page being viewed. It is a virtual page that is generated on the fly.</li>");
				virtual_content_store[line++] =	http_demo_alloc_string("<li>These pages are use by Html5Demo.html, Html4Demo.html and the web client examples.<br>");
				virtual_content_store[line++] =	http_demo_alloc_string("<li>They can also be accessed by typing into the browser url bar.<br></li><ul>");
				virtual_content_store[line++] =	http_demo_alloc_string("<li>//demo_ajax_getval - Get from this url to retrieve a variable stored on the server<br> For example: localhost/demo_ajax_getval.</li>");
				virtual_content_store[line++] =	http_demo_alloc_string("<li>//demo_ajax_setval_submit  - Post to this to change the a variable on the <br> For example:localhost/demo_ajax_setval_submit?AjaxSetVal=298</li>");
                virtual_content_store[line++] =	http_demo_alloc_string("<li>//demo_ajax_command_submit - Post to this to change the a variable on the <br> For example:localhost/demo_ajax_command_submit?AjaxSetVal=+100</li></ul>");
                virtual_content_store[line++] =	http_demo_alloc_string("<li>//configure.html   - A virtual page that allows you to change the directory where disk based content is retrieved from.</li>");
                virtual_content_store[line++] =	http_demo_alloc_string("<li>//demo_form.htm    - A virtual page that presents a simple form.</li>");
                virtual_content_store[line++] =	http_demo_alloc_string("<li>//demo_form_submit - Page url is the destination for posts with action=demo_form_submit.<br>for example http://localhost/demo_form_submit?v0=0&v1=1&v3=3</li>");
                virtual_content_store[line++] =	http_demo_alloc_string("</ul></ul>");
  				virtual_content_store[line++]=0;
				virtual_content = &virtual_content_store[0];
			}
            else if (virtUrlId == DEMO_CONFIGURE_ID)
            {

                int line = 0;
                char temp_buff[255];
                virtual_content_store[line++] =	http_demo_alloc_string("<html><body><hr><center><b><big>Server Configuration form.</big></b></center><hr><br><br>");
                virtual_content_store[line++] =	http_demo_alloc_string("<form name=\"configure\" action=\"demo_configure_submit\" method=\"get\">");

                virtual_content_store[line++] =	http_demo_alloc_string("<br>Enter a new subdirectory to serve content from. And submit the form. <br>");
                virtual_content_store[line++] =	http_demo_alloc_string("The PostHandler function will change the configuration parameter <br>and arrange for the top level of the ");
                virtual_content_store[line++] =	http_demo_alloc_string(" loop to reconfigure the server. <br>");
                virtual_content_store[line++] =	http_demo_alloc_string(" <i>Changes will not be noticed until you Revert to disk based html index and refresh.</i> <br>");

                rtp_sprintf(temp_buff,"Content Directory :<input type=\"text\" name=\"root\" value=%s/>", ExampleServerCtx.rootDirectory);
                virtual_content_store[line++] =	http_demo_alloc_string(temp_buff);
                virtual_content_store[line++] =	http_demo_alloc_string("<input type=\"submit\" value=\"Submit\" /><br>");

                /* Create a form initialized from our current configuration */
                virtual_content_store[line++] =	http_demo_alloc_string("</form>"	);
                virtual_content_store[line++] =	http_demo_alloc_string("</body></html>"	);
                virtual_content_store[line++] =	0;
				virtual_content = &virtual_content_store[0];
                if (HTTP_ServerAddPostHandler(&ExampleServerCtx.httpServer, "\\demo_configure_submit", demo_form_cb, (void *)"demo_configure_submit") < 0)
                    return -1;
			}
 			else /* Must be a plug in */
            {
                char temp_buff[512];
                rtp_sprintf(temp_buff,"<html><body><hr><center><b><big>No virtual content found for the link:</big></b><br>%s</center><hr><br><br></body></html>", request->target);
      			virtual_content_store[0] =	http_demo_alloc_string(temp_buff);
				virtual_content_store[1] =	0;
				virtual_content = &virtual_content_store[0];
			}
			HTTP_ServerReadHeaders(	ctx,session, _Demo_IndexFileProcessHeader, 0, tempBuffer, tempBufferSize);

			HTTP_ServerInitResponse(ctx, session, &response, 200, "OK");
			HTTP_ServerSetDefaultHeaders(ctx, &response);
			HTTP_SetResponseHeaderStr(&response, "Content-Type", "text/html");


			vfilesize = http_demo_count_vcontent_bytes(virtual_content);
			HTTP_SetResponseHeaderInt(&response, "Content-Length", vfilesize);

			HTTP_WriteResponse(session, &response);
			http_demo_write_vcontent_bytes(session, virtual_content);

			HTTP_WriteFlush(session);
			if (virtUrlId != 0)
				http_demo_release_vcontent(virtUrlId,virtual_content);

			HTTP_FreeResponse(&response);

			return (HTTP_REQUEST_STATUS_DONE);
		}

		case HTTP_METHOD_HEAD: /* tbd - implement HEAD method */
		default:
			return (HTTP_REQUEST_STATUS_CONTINUE);
	}
}

/*---------------------------------------------------------------------------*/
static int _Demo_IndexFileProcessHeader(void *userData,	HTTPServerRequestContext *server, HTTPSession *session,
		HTTPHeaderType type, const HTTP_CHAR *name,	const HTTP_CHAR *value)
{
	switch (type)
	{
		case HTTP_HEADER_IF_MODIFIED_SINCE:
			/* tbd */
			break;
	}

	return (0);
}



static HTTP_INT32 http_demo_count_vcontent_bytes(char **pContent)
{
	HTTP_INT32 length = 0;
	while (*pContent)
	{
		length += rtp_strlen(*pContent);
		pContent++;
	}
	return(length);
}

static void http_demo_write_vcontent_bytes(HTTPSession *session, char **pContent)
{
	HTTP_INT32 length = 0;
	while (*pContent)
	{
		length = rtp_strlen(*pContent);
		HTTP_Write(session, *pContent, length);
		pContent++;
	}
}

static void http_demo_release_vcontent(HTTP_INT32 virtUrlId, char **pContent)
{
	HTTP_INT32 length = 0;
	char **_pContent = pContent;
return;
	while (*_pContent)
	{
		rtp_free(*_pContent);
		_pContent++;
	}
	rtp_free(pContent);
}

HTTP_CHAR *http_demo_alloc_string(HTTP_CHAR *string)
{
HTTP_CHAR *r;
	r = rtp_malloc(rtp_strlen(string)+1);
	if (r)
		rtp_strcpy(r, string);
	return(r);
}


// =========================
  /* Default content displayed when index.htm, index.html, or a null index page is selected
   Modify this page to presnt something else when the page is accessed. */
static char *virtual_romindex_content  =
"<html><body><hr><center><b><big>Welcome to the EBS WEB Server Demo</big></b></center><hr><br><br>\
<center>Login Page</center><br>\
Select an option <br>\
<br>Use this selection to demonstrate form processing.<br>\
<a href=\"demo_form.htm\">Demonstrate Simple Forms Processing.</a><br>\
<br>Use this selection to change the path where content is served from.<br>\
<a href=\"configure.htm\">Setup And Configuration Web Server.</a><br>\
<br>Use this selection to revert to disk based html. \
<br> When you refresh, index.html et al will be serverd from the disk.<br>\
<hr><br><center>The Ajax and jQuery examples are implemented as simple stubs. They are provide to demonstrate \
how to add new handlers.</center><br>\
<h2>If you selected the advanced server option you may interact with the server by executing these pages.  !</h2>\
<a href='Html5Demo.html'>Example using Ajax and html5 together </a> <br>\
<a href='Html4Demo.html'>Example using forms and Ajax with html4 together </a> <br>\
<a href='info.html'>Click here for more information on about this application.</a><br>\
<br><hr>\
</body></html>";

static char *demo_form_content = "\
<form name=\"input\" action=\"demo_form_submit\" method=\"get\">\
Do you have a bike: <input type=\"checkbox\" name=\"Bike\" value=\"yes\" /> <br>\
Do you have a car: <input type=\"checkbox\" name=\"Car\" value=\"yes\" /> <br>\
Do you have an airplane: <input type=\"checkbox\" name=\"Airplane\" value=\"yes\" /><br>\
What is your name: <input type=\"text\" name=\"user\" /><br>\
<input type=\"submit\" value=\"Submit\" /><br>\
</form>\
<form name=\"input\" action=\"demo_form_submit\" method=\"post\">\
Using post method<br>\
Do you have a bike: <input type=\"checkbox\" name=\"Bike\" value=\"yes\" /> <br>\
Do you have a car: <input type=\"checkbox\" name=\"Car\" value=\"yes\" /> <br>\
Do you have an airplane: <input type=\"checkbox\" name=\"Airplane\" value=\"yes\" /><br>\
What is your name: <input type=\"text\" name=\"user\" /><br>\
<input type=\"submit\" value=\"Submit\" /><br>\
</form> ";

#ifdef CALL_SSL_MAIN
/*
 * ExampleSSLServer.c
 *
 * This program is an OpenSSL server that sits and listens for an incoming
 * SSL connection.  Once a connection is made, the server reads a "message"
 * (defined as a stream terminated by <LF>) from the client, and sends
 * back a modified response to the client before closing the connection.
 * The code to perform the SSL stuff is (as much as possible) all contained
 * in "main" - the use of other functions is limited to just displaying
 * information, so that you should only need to read the "main" code to
 * see what order things are done.
 *
 * Environment variables:
 *   servercert - assumed to point at a .PEM file containing a certificate
 *                chain to be used by this server to authenticate itself
 *   privatekey - assumed to point at a .PEM file containing a private
 *                key for the server certificate
 *   serverpwd  - the password for the private key
 *
 * Tested on
 * Tru64 UNIX V5.1, OpenSSL 0.9.6c
 * OpenVMS V7.2-1, OpenSSL 0.9.5a with "CC/POINTER=64"
 *
 * Nick Hudson, May 2002
 */
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/rand.h>
//#include <openssl/err.h>

#include <ctype.h>

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

char *randbuf =
"this isn't a very good way to seed the PRNG but will suffice"
"this isn't a very good way to seed the PRNG but will suffice"
"this isn't a very good way to seed the PRNG but will suffice"
"this isn't a very good way to seed the PRNG but will suffice"
"this isn't a very good way to seed the PRNG but will suffice";

int isTracing = 0;

/* Print SSL errors and exit*/
void error_exit(char *string)
{
  printf("%s : \n",string);
  exit(0);
}

/*
 * If "doTrace" is non-zero, then send the message to the stdout
 */
void trace(int doTrace, const char *message)
{
  if (doTrace) {
    printf("%s\n",message);
  }
}
/*
 * This method looks at the run-time environment in an attempt to check
 * if things are set up correctly for the program to run.  It reports
 * areas of concern but does not abort
 */
void checkEnvironment()
{
#if (0)
  // Check whether they've specified a location for the server certificate
  if (getenv("servercert") == NULL) {
    trace(1,
"\nThe environment variable 'servercert' is not defined, which means that");
    trace(1,
"this program has no certificate chain to send to a client when using a");
    trace(1,
"cipher suite that requires authentication.  Set the environment variable");
    trace(1,
"'servercert' to point to a suitable file if you need to use one.\n");
    }

  // Is there an environment variable which we can use for the password?
  if (getenv("serverpwd") == NULL) {
    trace(1,
"\nThe environment variable 'serverpwd' is not defined, which means");
    trace(1,
"that a password be required for a certificate's private key, the SSL");
    trace(1,
"library will prompt you for it at the terminal.  Set the environment");
    trace(1,
"variable 'serverpwd' to a password to override this behaviour.\n");
    }

  // Is there an environment variable which we can use for the private key?
  if (getenv("privatekey") == NULL) {
    trace(1,
"\nThe environment variable 'privatekey' is not defined, which means");
    trace(1,
"that it may not be possible to use server certificates for cipher suites");
    trace(1,
"that require server authentication.  Set the environment variable");
    trace(1,
"'privatekey' to specify a PEM format private key file.\n");
    }
#endif
}


void usage()
{
  trace(1,"\nUsage: ExampleSSLServer <port> <ciphers> <protocol> [trace]");
  trace(1," port        : port number to use");
  trace(1," ciphers     : types of cipher suites to propose :-");
  trace(1,"               'c' : suites that use server-cert");
  trace(1,"               'a' : anonymous (no server-cert)");
  trace(1,"               'n' : suites with no encryption");
  trace(1," protocol    : '1' '2' or '3' or '4' where :-");
  trace(1,"               '1' means TLSv1");
  trace(1,"               '2' means SSLv2");
  trace(1,"               '3' means SSLv3");
  trace(1,"               '4' means SSLv23");
  trace(1," trace       : (optional) tracing to enable :-");
  trace(1,"               't' : turn on program tracing");
}

/*
 * This callback hands back the password to be used during decryption,
 * and returns a value derived from the environment variable "serverpwd".
 *
 * buf      : the function will write the password into this buffer
 * size     : the size of "buf"
 * rwflag   : indicates whether the callback is being used for reading/
 *            decryption (0) or writing/encryption (1)
 * userdata : pointer to area which can be used by the application

 */
static char *glserverpwd;

static int passwd_cb(char *buf,int size,
		     int rwflag,void *userdata)
{

  char *password;
  int password_length;

  trace(isTracing,"password_cb has been called");

  password = glserverpwd;
  password_length = strlen(password);

  if ((password_length + 1) > size) {
    trace(1,"Password specified by environment variable is too big");
    return 0;
  }

  strcpy(buf,password);
  return password_length;

}
/*
 * The following function was generated using the openssl utility, using
 * the command : "openssl dhparam -dsaparam -C 512"
 */
DH *get_dh512()
{
  static unsigned char dh512_p[]={
    0x9B,0x9A,0x2B,0x34,0xDA,0x9A,0x55,0x53,0x47,0xDB,0xCF,0xB4,
    0x26,0xAA,0x4D,0xFD,0x01,0x91,0x4A,0x19,0xE0,0x90,0xFA,0x6B,
    0x99,0xD6,0xE2,0x78,0xF3,0x31,0xD3,0x93,0x9B,0x7B,0xE1,0x65,
    0x57,0xFD,0x4D,0x2C,0x4E,0x17,0xE1,0xAC,0x30,0xB7,0xD0,0xA6,
    0x80,0x13,0xEE,0x37,0xD1,0x83,0xCD,0x5F,0x88,0x38,0x79,0x9C,
    0xFD,0xCE,0x85,0xED,
  };
  static unsigned char dh512_g[]={
    0x8B,0x17,0x22,0x46,0x30,0xAD,0xE5,0x06,0x42,0x60,0x15,0x79,
    0xA2,0x2F,0xD9,0xAA,0x7B,0xD7,0x8A,0x6F,0x39,0xEB,0x13,0x38,
    0x54,0xA6,0xBE,0xAD,0xC6,0x6A,0x17,0x95,0xBE,0x8B,0x29,0xE0,
    0x60,0x14,0x72,0xC9,0x5C,0x84,0x5D,0xD6,0x8B,0x57,0xD9,0x9D,
    0x08,0x60,0x73,0x78,0x3F,0xDD,0x26,0x2C,0x40,0x63,0xCF,0xE0,
    0xDC,0x58,0x7A,0x9C,
  };
  DH *dh;

  if ((dh=DH_new()) == NULL) return(NULL);
  dh->p=BN_bin2bn(dh512_p,sizeof(dh512_p),NULL);
  dh->g=BN_bin2bn(dh512_g,sizeof(dh512_g),NULL);
  if ((dh->p == NULL) || (dh->g == NULL))
    { DH_free(dh); return(NULL); }
  dh->length = 160;
  return(dh);
}

/*
 * Display all the ciphers available for a specific SSL structure
 */
void enumerate_ciphers(SSL *ssl)
{
  char tracebuf[512];

  int index = 0;
  const char *next = NULL;
  do {
    next = SSL_get_cipher_list(ssl,index);
    if (next != NULL) {
      sprintf(tracebuf,"  %s",next);
      trace(1,tracebuf);
      index++;
    }
  }
  while (next != NULL);
}

#if (0)
/*
 * Set up a socket to listen on a specific port, and then wait for the
 * first incoming connection.  Once a connection appears, the function
 * returns the socket id for the new connection.
 */
int getNewConnection(int portNum)
{
  int ListenSock, newConnection;
  int stat;
  int val = 1;
  struct sockaddr_in SA;

  ListenSock = socket(AF_INET,SOCK_STREAM,0);
  if (ListenSock < 0) {
    error_exit("Call to create socket failed");
  }

  memset(&SA,0,sizeof(SA));

  // Fill in the name & address for the listen socket
  SA.sin_family = AF_INET;
  SA.sin_port = htons(portNum);
  SA.sin_addr.s_addr = INADDR_ANY;

  setsockopt(ListenSock,SOL_SOCKET,SO_REUSEADDR,&val,sizeof(val));

  stat = bind(ListenSock,
		 (struct sockaddr *)&SA,
		 sizeof(SA));

  if (stat == -1) {
    error_exit("Call to bind failed");
  }

  stat = listen(ListenSock,5);  // 5 is the "backlog"
  if (stat == -1) {
    error_exit("Call to listen failed");
  }

  newConnection = accept(ListenSock,0,0);

  return newConnection;
}
#endif
void ssl_server_main(SSL_CTX *ctx,char *Suite/*'a','c','n'*/, int sslmethod/* 1,2,3,4 */,char *servercert,char *serverpwd, char *privatekey, int socketno)
{
  // parse command line args
  int portNumber;
  int enableAnonSuites = 0;
  int enableCertSuites = 0;
  int enableNullSuites = 0;
  int stat;
  char tracebuf[512];
  char buff[512];
  char ciphers[512];
  char clientMessage[512];
  int clientSocket;
  int totalBytes;
  int messageComplete;
  int i;

  SSL_METHOD *meth = NULL;

  SSL *ssl;
  BIO *sbio;


  enableAnonSuites = (strchr(Suite,'a') != NULL);
  enableCertSuites = (strchr(Suite,'c') != NULL);
  enableNullSuites = (strchr(Suite,'n') != NULL);

  switch (sslmethod)
  {
  case 1 : meth = TLSv1_method(); break;
  case 2 : meth = SSLv2_method(); break;
  case 3 : meth = SSLv3_method(); break;
  case 4 : meth = SSLv23_method(); break;
  default : usage(); return;
  }

  isTracing = 1;

  if (!ctx)
  {
	trace(1,"Configuring SSL connection...");

	// Register all the libssl error strings
	trace(isTracing,"Calling SSL_load_error_strings()...");
	SSL_load_error_strings();

	// Register all available ciphers and digests.  The documentation
	// says you can disregard the return status
	trace(isTracing,"Calling SSL_library_init()...");
	SSL_library_init();

	 // Seed the pseudo random number generator (PRNG).  If you don't do this
	// then if you're lucky, SSL will complain "PRNG not seeded".  Alternatively
	// things may just fail.
	trace(isTracing,"Calling RAND_seed()...");
	RAND_seed(randbuf,strlen(randbuf));



	// Now create the SSL context from which we will be able to create
	// SSL sessions
	trace(isTracing,"Calling SSL_CTX_new()...");
	ctx = SSL_CTX_new(meth);

	if (ctx == NULL) {
		error_exit("call to SSL_CTX_new() returned NULL");
	}
  }

  // Tell SSL that we don't want to request client certificates for
  // verification
  trace(isTracing,"SSL_CTX_set_verify()...");
  SSL_CTX_set_verify(ctx,
		     SSL_VERIFY_NONE,
		     NULL);

  // Now tell SSL where its server certificate chain is (assuming the
  // user has told us)
  if (servercert!= NULL) {
    trace(isTracing,"Calling SSL_CTX_use_certificate_chain_file()...");
    stat = SSL_CTX_use_certificate_chain_file(ctx,servercert);
    if (stat != 1) {
      error_exit("SSL_CTX_use_certificate_chain_file failed");
    }
  }


  // If the user has specified a password, tell SSL the address of
  // a callback routine which will return it
  glserverpwd=serverpwd;
  if (serverpwd!= NULL) {
	glserverpwd = serverpwd;
    trace(isTracing,"Calling SSL_CTX_set_default_passwd_cb()...");
    SSL_CTX_set_default_passwd_cb(ctx,passwd_cb);
  }

  if (privatekey != NULL) {
    trace(isTracing,"Calling SSL_CTX_use_PrivateKey_file()...");
    stat = SSL_CTX_use_PrivateKey_file(ctx,
				      privatekey,
				       SSL_FILETYPE_PEM);
  }



  // Load the ciphers that they've asked for.
  // It could be inferred from the documentation for ciphers(1) that you
  // can call SSL_CTX_set_cipher_list multiple times to build up a list
  // of ciphers.  But that isn't how it works; rather you build up a list
  // of ciphers in a string, and then pass that in a single call:

  // First of all disable any ciphers we've got by default
  ciphers[0] = 0;
  strcat(ciphers,"-ALL");

  // If they want cipher suites that require certificates, add them in
  if (enableCertSuites) {
    strcat(ciphers,":ALL:-aNULL");
  }

  // If they want the null ones, add them in
  if (enableNullSuites) {
    strcat(ciphers,":NULL");
  }

  // If they want the anonymous ones, add them in
  if (enableAnonSuites) {
    strcat(ciphers,":aNULL");
  }

  sprintf(tracebuf,"Calling SSL_CTX_set_cipher_list(\"%s\")...",ciphers);
  trace(isTracing,tracebuf);


  stat = SSL_CTX_set_cipher_list(ctx,ciphers);
  if (stat == 0) {
    error_exit("SSL_CTX_set_cipher_list() failed");
  }

  // Provide DH key information (if you don't do this, then ciphers that
  // require DH key exchange won't be used, even if they are in the list of
  // ciphers for the ctx)
  trace(isTracing,"Calling SSL_CTX_set_tmp_dh()...");
  stat = SSL_CTX_set_tmp_dh(ctx,get_dh512());

  ssl = SSL_new(ctx);
  if (isTracing) {
    trace(1,"Enabled cipher suites are :");
    enumerate_ciphers(ssl);
  }

  // Wait for an incoming connection
  trace(1,"\n...Now waiting for a client to connect");

  trace(1,"\n...Returning for processing to continue");
  return;

  {
	 unsigned char ipAddr[64]; int port; int type;

	if (rtp_net_read_select(socketno, 100000) < 0)
	{
		trace(1,"\n...select failed");
		return;
	}

	if (rtp_net_accept(&clientSocket, socketno, ipAddr, &port, &type) < 0)
	{
		trace(1,"\n...Accept failed");
		return;
	}
  }


  trace(1,"Connection received");

  // Generate a BIO wrapper for the TCP socket
  trace(isTracing,"Calling BIO_new_socket()...");
  sbio = BIO_new_socket(clientSocket,BIO_NOCLOSE);
  if (sbio == NULL) {
    error_exit("BIO_new_socket() failed");
  }

  // Connect up our SSL session with the socket
  trace(isTracing,"Calling SSL_set_bio()...");
  SSL_set_bio(ssl,sbio,sbio);

  trace(isTracing,"Calling SSL_accept()...");
  stat = SSL_accept(ssl);
  if (stat <= 0) {
    error_exit("SSL_accept failed");
  }

  // Now read stuff from the client, assuming that it will be a
  // message terminated by EOL
  totalBytes = 0;
  messageComplete = 0;
  do {
    trace(isTracing,"Calling SSL_read()...");
    stat = SSL_read(ssl,
		    &buff[totalBytes],
		    (sizeof(buff) - totalBytes));
    if (stat <= 0) {
      error_exit("SSL_read failed");
    }

    // Look for an EOL
    for (i=0; i<stat; i++) {
      if (buff[(i+totalBytes)] == '\n') {

	// Stick a NULL terminator just beyond it
	buff[(i + totalBytes + 1)] = 0;
	messageComplete = 1;
	break;
      }
    }
    totalBytes += stat;
  }
  while (messageComplete == 0);

  sprintf(tracebuf,"The client sent -->%s<--", buff);
  trace(1,tracebuf);

  sprintf(clientMessage,"OpenSSL server says thank-you for saying : %s",buff);

  trace(isTracing,"Calling SSL_write()...");
  stat = SSL_write(ssl,clientMessage,strlen(clientMessage));

  if (stat <= 0) {
    error_exit("SSL_write failed");
  }

  if (stat != strlen(clientMessage)) {
    error_exit("Incomplete SSL write");
  }

  trace(isTracing,"Shutting down connection...");
  stat = SSL_shutdown(ssl);
  if (stat < 0) {
    error_exit("SSL_shutdown failed");
  }
  if (stat == 0) {
    // According to docs at the OpenSSL website, this means that the shutdown
    // has not yet finished, and we must call SSL_shutdown again..
    stat = SSL_shutdown(ssl);
    if (stat <= 0) {
      error_exit("SSL_shutdown (2nd time) failed");
    }
  }


  SSL_free(ssl);
  SSL_CTX_free(ctx);
  rtp_net_closesocket(clientSocket);

  trace(1,"\nProgam terminating");
}


/*
 * ExampleSSLClient.c
 *
 * This program is an OpenSSL client that talks to a server over an SSL
 * connection.  Once a connection is made, the client sends a "message"
 * (defined as a stream terminated by <LF>) to the server, and reads
 * back the response which is a message sent back by the server.
 * The code to perform the SSL stuff is (as much as possible) all contained
 * in "main" - the use of other functions is limited to just displaying
 * information, so that you should only need to read the "main" code to
 * see what order things are done.
 *
 * Environment variables:
 *   truststore - assumed to point at .PEM file containing certificates
 *                that are trusted by this client
 *
 * Tested on
 * Tru64 UNIX V5.1, OpenSSL 0.9.6c
 * OpenVMS V7.2-1, OpenSSL 0.9.5a with "CC/POINTER=64"
 *
 * Nick Hudson, May 2002
 */
#if (0)
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/rand.h>

#include <ctype.h>

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

int isTracing = 0;
char *randbuf =
"this isn't a very good way to seed the PRNG but will suffice"
"this isn't a very good way to seed the PRNG but will suffice"
"this isn't a very good way to seed the PRNG but will suffice"
"this isn't a very good way to seed the PRNG but will suffice"
"this isn't a very good way to seed the PRNG but will suffice";

#endif

void clientusage()
{
  trace(1,"\nUsage: ExampleSSLClient <server> <port> <ciphers>"
	" <protocol> [trace]");
  trace(1," server      : the server to talk to");
  trace(1," port        : port number to use");
  trace(1," ciphers     : types of cipher suites to propose :-");
  trace(1,"               'c' : suites that use server-cert");
  trace(1,"               'a' : anonymous (no server-cert)");
  trace(1,"               'n' : suites with no encryption");
  trace(1," protocol    : '1' '2' or '3' or '4' where :-");
  trace(1,"               '1' means TLSv1");
  trace(1,"               '2' means SSLv2");
  trace(1,"               '3' means SSLv3");
  trace(1,"               '4' means SSLv23");
  trace(1," trace       : (optional) tracing to enable :-");
  trace(1,"               't' : turn on program tracing");
}

#if (0)

/*
 * Display all the ciphers available for a specific SSL structure
 */
void enumerate_ciphers(SSL *ssl)
{
  char tracebuf[512];

  int index = 0;
  const char *next = NULL;
  do {
    next = SSL_get_cipher_list(ssl,index);
    if (next != NULL) {
      sprintf(tracebuf,"  %s",next);
      trace(1,tracebuf);
      index++;
    }
  }
  while (next != NULL);
}
#endif

void ssl_client_main(char *Suite/*'a','c','n'*/, int sslmethod/* 1,2,3,4 */,char *truststore, int socket)
{
  int enableAnonSuites = 0;
  int enableCertSuites = 0;
  int enableNullSuites = 0;
  char tracebuf[512];
  char ciphers[512];
  char *clientMessage = "Message from OpenSSL client\n";
  char buff[512];
  char *serverResponse;
  int stat;
  int totalBytes;
  int messageComplete;
  int i;


  SSL_METHOD *meth = NULL;
  SSL_CTX *ctx;
  SSL *ssl;
  BIO *sbio;


  enableAnonSuites = (strchr(Suite,'a') != NULL);
  enableCertSuites = (strchr(Suite,'c') != NULL);
  enableNullSuites = (strchr(Suite,'n') != NULL);

  switch (sslmethod)
  {
  case 1 : meth = TLSv1_method(); break;
  case 2 : meth = SSLv2_method(); break;
  case 3 : meth = SSLv3_method(); break;
  case 4 : meth = SSLv23_method(); break;
  default : usage(); return;
  }

  isTracing = 1;


  trace(1,"Configuring SSL connection...");

  // Register all the libssl error strings
  trace(isTracing,"Calling SSL_load_error_strings()...");
  SSL_load_error_strings();

  // Register all available ciphers and digests.  The documentation
  // says you can disregard the return status
  trace(isTracing,"Calling SSL_library_init()...");
  SSL_library_init();

  // Seed the pseudo random number generator (PRNG).  If you don't do this
  // then certain SSL functions will complain "PRNG not seeded"
  trace(isTracing,"Calling RAND_seed()...");
  RAND_seed(randbuf,strlen(randbuf));

  // Now create the SSL context from which we will be able to create
  // SSL sessions
  trace(isTracing,"Calling SSL_CTX_new()...");
  ctx = SSL_CTX_new(meth);

  if (ctx == NULL) {
    error_exit("call to SSL_CTX_new() returned NULL");
  }

  // Tell SSL that we want verification of all server certificates.  Unless
  // this call is made, then any certificate presented by a server will
  // be acceptable.
  trace(isTracing,"SSL_CTX_set_verify()...");
  SSL_CTX_set_verify(ctx,
		     SSL_VERIFY_PEER,
		     NULL);

  // If the user has defined 'truststore', then tell SSL to use this file
  // as a source of CA certificates.
  if (truststore != NULL) {
    trace(isTracing,"Calling SSL_CTX_load_verify_locations()...");
    stat = SSL_CTX_load_verify_locations(ctx,
					 truststore, // CAfile
					 NULL                  // CApath
					 );
    if (stat != 1) {
      error_exit("SSL_CTX_load_verify_locations() failed");
    }
  }

  // Load the ciphers that they've asked for.
  // It could be inferred from the documentation for ciphers(1) that you
  // can call SSL_CTX_set_cipher_list multiple times to build up a list
  // of ciphers.  But that isn't how it works; rather you build up a list
  // of ciphers in a string, and then pass that in a single call:

  // First of all disable any ciphers we've got by default
  ciphers[0] = 0;
  strcat(ciphers,"-ALL");

  // If they want cipher suites that require certificates, add them in
  if (enableCertSuites) {
    strcat(ciphers,":ALL:-aNULL");
  }

  // If they want the null ones, add them in
  if (enableNullSuites) {
    strcat(ciphers,":NULL");
  }

  // If they want the anonymous ones, add them in
  if (enableAnonSuites) {
    strcat(ciphers,":aNULL");
  }

  sprintf(tracebuf,"Calling SSL_CTX_set_cipher_list(\"%s\")...",ciphers);
  trace(isTracing,tracebuf);


  stat = SSL_CTX_set_cipher_list(ctx,ciphers);
  if (stat == 0) {
    error_exit("SSL_CTX_set_cipher_list() failed");
  }

  ssl = SSL_new(ctx);
  if (isTracing) {
    trace(1,"Enabled cipher suites are :");
    enumerate_ciphers(ssl);
  }

  trace(isTracing,"Connecting to server");

  // Generate a BIO wrapper for the TCP socket
  trace(isTracing,"Calling BIO_new_socket()...");
  sbio = BIO_new_socket(socket,BIO_NOCLOSE);
  if (sbio == NULL) {
    error_exit("BIO_new_socket() failed");
  }

  // Connect up our SSL session with the socket
  trace(isTracing,"Calling SSL_set_bio()...");
  SSL_set_bio(ssl,sbio,sbio);

  // Initiate the SSL handshake
  trace(isTracing,"Calling SSL_connect()...");
  stat = SSL_connect(ssl);

  if (stat <= 0) {
    error_exit("SSL_connect failed");
  }

  // We are now ready to talk with the server
  trace(1,"\n...Now connected to server");

  trace(isTracing,"Calling SSL_write()...");
  stat = SSL_write(ssl,clientMessage,strlen(clientMessage));

  if (stat <= 0) {
    error_exit("SSL_write failed");
  }

  if (stat != strlen(clientMessage)) {
    error_exit("Incomplete SSL write");
  }

  // Now read the response from the server, assuming it will be
  // terminated by EOL
  totalBytes = 0;
  messageComplete = 0;

  do {
    trace(isTracing,"Calling SSL_read()...");
    stat = SSL_read(ssl,
		    &buff[totalBytes],
		    (sizeof(buff) - totalBytes));
    if (stat <= 0) {
      error_exit("SSL_read failed");
    }

    // Look for an EOL
    for (i=0; i<stat; i++) {
      if (buff[(i+totalBytes)] == '\n') {

	// Stick a NULL terminator just beyond it
	buff[(i + totalBytes + 1)] = 0;
	messageComplete = 1;
	break;
      }
    }
    totalBytes += stat;
  }
  while (messageComplete == 0);

  sprintf(tracebuf,"The server sent -->%s<--", buff);
  trace(1,tracebuf);

  trace(isTracing,"Shutting down connection...");
  stat = SSL_shutdown(ssl);
  if (stat < 0) {
    error_exit("SSL_shutdown failed");
  }
  if (stat == 0) {
    // According to docs at the OpenSSL website, this means that the shutdown
    // has not yet finished, and we must call SSL_shutdown again..
    stat = SSL_shutdown(ssl);
    if (stat <= 0) {
      error_exit("SSL_shutdown (2nd time) failed");
    }
  }


  SSL_free(ssl);
  SSL_CTX_free(ctx);
  rtp_net_closesocket(socket);

  trace(1,"\nProgam terminating");

}
#endif

#endif

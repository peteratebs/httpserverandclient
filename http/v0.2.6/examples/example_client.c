/*
|  example_client.c
|
|  Copyright EBS Inc. , 2009. All rights reserved.

This source file contains an example of a interactive HTTP application written using the managed HTTP library.

This example demontrates the following:

.
.
.
.
*/

/*7****************************************************************************/
/* Header files
 *****************************************************************************/

#include "rtpnet.h"
#include "rtpterm.h"
#include "rtpprint.h"
#include "rtpstr.h"
#include "httpcli.h"
#include "httpmcli.h"
#include "rtpgui.h"
#include "rtpthrd.h"
#include "rtpfile.h"
#include "rtpssl.h"
/*****************************************************************************/
/* Macros
 *****************************************************************************/

/* Configuration parameters */


/*****************************************************************************/
/* Data
 *****************************************************************************/

/*****************************************************************************/
/* Function Definitions
 *****************************************************************************/

/* Main entry point for the web server application.
    A callable web server.
*/
static int prompt_for_command(void);
static int http_client_init(HTTPManagedClient* phttpClient);
static int http_client_get(HTTPManagedClient* phttpClient, char *defaulturl, char *defaultfile, int savetofile, int printresults, int printajaxresults);
static int http_client_post(HTTPManagedClient* phttpClient,char *defaulturl, int setajaxvalue);
static int http_client_put(HTTPManagedClient* phttpClient);

#define COMMAND_QUIT             0
#define COMMAND_GET              1
#define COMMAND_GETTO_FILE       2
#define COMMAND_GETTO_SCREEN     4
#define COMMAND_GET_AJAXVAL      5
#define COMMAND_SET_AJAXVAL      6
#define COMMAND_SUBMIT           7
#define COMMAND_POST             8
#define COMMAND_PUT              9

// #define CALL_SSL_MAIN 1
#ifdef CALL_SSL_MAIN
void ssl_client_main(char *Suite/*'a','c','n'*/, int sslmethod/* 1,2,3,4 */,char *truststore, int socket);
extern int HTTP_ManagedClientGetSocket (HTTPManagedClientSession** session);
#endif
static HTTPAuthContext Authctx; 
static HTTPAuthContext *pAuthctx; 
static RTP_HANDLE sslContext = 0;
static unsigned sslEnabled  = 1;

int http_client_demo(void)
{
    int idle_count = 0,command;
    rtp_net_init();
    rtp_threads_init();

	if (HTTP_AuthContextInit(&Authctx) < 0)
		return(-1);
	pAuthctx = &Authctx;

#ifndef CALL_SSL_MAIN
	sslEnabled=0;
#else
	if (rtp_ssl_init_context (
		&sslContext,
		RTP_SSL_MODE_CLIENT,
		RTP_SSL_VERIFY_PEER|RTP_SSL_VERIFY_CLIENT_ONCE
	//	unsigned int verifyMode
	) < 0)
		return(-1);
#endif

    while ((command = prompt_for_command()) != COMMAND_QUIT)
    {
    HTTPManagedClient httpClient;
        if (http_client_init(&httpClient) < 0)
            return(-1);

        if (command == COMMAND_GET)
            http_client_get(&httpClient,"localhost", "Html5Demo.html", 0, 0, 0);
        else if (command == COMMAND_GETTO_FILE)
			http_client_get(&httpClient,"www.microsoft.com", "/en-us/default.aspx", 1, 0, 0);
 //           http_client_get(&httpClient,"www.fortify.net", "sslcheck.html", 1, 0, 0);
        else if (command == COMMAND_GETTO_SCREEN)
            http_client_get(&httpClient,"localhost", "Html5Demo.html", 0, 1, 0);
        else if (command == COMMAND_GET_AJAXVAL)
            http_client_get(&httpClient,"localhost", "/demo_ajax_getval", 0, 0,1);
        else if (command == COMMAND_SET_AJAXVAL)
            http_client_post(&httpClient,"localhost",1);
        else if (command == COMMAND_SUBMIT)
            http_client_get(&httpClient,"localhost", "/demo_form_submit?echo1=1&echo2=2&echo3=3", 0, 1,0);
        else if (command == COMMAND_POST)
            http_client_post(&httpClient,"localhost",0);
        else if (command == COMMAND_PUT)
            http_client_put(&httpClient);
    }
    rtp_net_exit();
    return (0);

}

static int http_client_init(HTTPManagedClient* phttpClient)
{
    rtp_memset(phttpClient, 0, sizeof(*phttpClient));

    if (HTTP_ManagedClientInit (
            phttpClient,
            "EBS MANAGED CLIENT" /* User-Agent */,
            0                      /* Accept (mime types) */,
            0                      /* KeepAlive (bool) */,
            0                      /* Cookie context */,
            pAuthctx               /* Authentication context */,
            sslContext                      /* SSL context */,
            sslEnabled                      /* SSL enabled? (bool) */,
            8192                   /* write buffer size */,
            0                      /* max connections (0 == no limit) */,
            0                      /* max connections per host (0 == no limit) */
            ) >= 0)
    {
            return (0);
    }
    else
    {
        rtp_printf("HTTP_ManagedClientInit Failed\n");
        return (-1);
    }
}

static int prompt_for_command(void)
{
    int command =0;
    void *pmenu;

    pmenu = rtp_gui_list_init(0, RTP_LIST_STYLE_MENU, "EBS HTTP Client Demo", "Select Option", "Option >");
    if (!pmenu)
        return(-1);;

    rtp_gui_list_add_int_item(pmenu, "Get URL and count number of bytes", COMMAND_GET, 0, 0, 0);
    rtp_gui_list_add_int_item(pmenu, "Get URL and save to file", COMMAND_GETTO_FILE, 0, 0, 0);
    rtp_gui_list_add_int_item(pmenu, "Get URL and display on screen", COMMAND_GETTO_SCREEN, 0, 0, 0);
    rtp_gui_list_add_int_item(pmenu, "Submit a form and view the response", COMMAND_SUBMIT, 0, 0, 0);
    rtp_gui_list_add_int_item(pmenu, "Get the value being managed by web apps and the server", COMMAND_GET_AJAXVAL, 0, 0, 0);
    rtp_gui_list_add_int_item(pmenu, "Set the value being managed by web apps and the server", COMMAND_SET_AJAXVAL, 0, 0, 0);
    rtp_gui_list_add_int_item(pmenu, "Post data to a URL", COMMAND_POST, 0, 0, 0);
    rtp_gui_list_add_int_item(pmenu, "Put a local file to a url", COMMAND_PUT, 0, 0, 0);
    rtp_gui_list_add_int_item(pmenu, "Quit", COMMAND_QUIT, 0, 0, 0);

    command = rtp_gui_list_execute_int(pmenu);
    rtp_gui_list_destroy(pmenu);

    return (command);
}

HTTP_UINT8 read_buffer[4098];
HTTP_INT32 read_buffer_size = 4096;


static int http_client_get(HTTPManagedClient* phttpClient, char *defaulturl, char *defaultfile, int savetofile, int printresults, int printajaxresults)
{
HTTPManagedClientSession * session = 0;
char urlpath[255];
char urlfile[255];
char localfile[255];
RTP_HANDLE  fd;

HTTP_INT32 result = -1;
HTTP_INT32 totalresult = 0;
HTTPResponseInfo info;

/*
    if (printajaxresults)
    {
        rtp_strcpy(urlpath,defaulturl);
        rtp_strcpy(urlfile,defaultfile);

    }
    else
*/
    {
        rtp_printf("You may use the defaults if the web server is running in the background\n\n");
        urlpath[0]=0;
        do {
            if (defaulturl)
                rtp_printf("Press return to select '%s' or enter an IP or DNS address\n",defaulturl);
            rtp_gui_prompt_text(" ","IP Address (eg: 192.161.2.1) or domain name of host (eg:www.google.com etc)\n    :", &urlpath[0]);
            if (!urlpath[0]&&defaulturl)
                rtp_strcpy(urlpath,defaulturl);
        } while (!urlpath[0]);

        urlfile[0]=0;
        do {
            if (defaultfile)
                rtp_printf("Press return to select '%s', 'x' for none or enter a file to retrieve\n",defaultfile);
    		rtp_gui_prompt_text(" ","File name to retrieve eg: index.html\n    :", &urlfile[0]);
            if (!urlfile[0]&&defaultfile)
                rtp_strcpy(urlfile,defaultfile);
        } while (!urlfile[0]);
    }
    if (savetofile)
    {
        rtp_gui_prompt_text(" ","Enter Local file name: \n    :", &localfile[0]);
        if (rtp_file_open (&fd, (const char *) &localfile[0], (RTP_FILE_O_CREAT|RTP_FILE_O_RDWR|RTP_FILE_O_TRUNC), RTP_FILE_S_IWRITE) < 0)
        {
             rtp_printf("Failure opening %s\n", localfile);
             return(-1);
        }
    }


    /* A HTTPManagedClientSession is the abstraction for a SINGLE HTTP request/response pair.
    Thus a new session must be opened for each HTTP operation
    (although this may not cause a new connection to be established, if keep alive is used),
    and closed when the operation has completed (though, again, this may not actually close a
    physical network connection) */

    if (HTTP_ManagedClientStartTransaction (
     phttpClient,
     &urlpath[0],
     80, // 443, // 80,
      4,
#ifndef CALL_SSL_MAIN
     HTTP_SESSION_TYPE_TCP, // HTTP_SESSION_TYPE_TCP,
#else
	 HTTP_SESSION_TYPE_SECURE_TCP, // HTTP_SESSION_TYPE_TCP,
#endif
     1, /* blocking? */ &session ) < 0)
    {
         rtp_printf("Failed: retrieving data from from %s/%s\n", urlpath, urlfile);
         return(-1);
    }
	
#ifdef CALL_SSL_MAIN
	{
	// int HTTP_ManagedClientGetSocket (HTTPManagedClientSession** session)
	ssl_client_main("a", 4,0,  HTTP_ManagedClientGetSocket( &session));
	}
#endif
     /* Once the session is open, one command may be issued; in this case a GET
       (by calling HTTP_ManagedClientGet) */
     HTTP_ManagedClientGet(session, urlfile, 0 /* if-modified-since */);

     /* This may be called at any time after the command is issued to get information
        about the server response; this info includes the status code given, date when
        the request was processed, file mime type information (if a file is transferred
        as the result of a command), authentication information, etc. */

     HTTP_ManagedClientReadResponseInfo(session, &info);

     do {
         /* Read data from the session */
         result = HTTP_ManagedClientRead(session, read_buffer, read_buffer_size);
        if (result > 0)
        {
            totalresult += result;
            if (savetofile)
            {
                if (rtp_file_write(fd,read_buffer, (long)result) < 0)
                {
                    rtp_printf("Failed writing to file\n");
                    return(-1);
                }
            }
            if (printresults)
            {
                read_buffer[result]=0;
                rtp_printf("%s",read_buffer);
            }
            if (printajaxresults)
            {   /* The result in bold text in html. so we can fix it up for display without parsing  */
                char *p, *p2;
                read_buffer[result]=0;
                p = rtp_strstr(read_buffer, "<b>");
                if (p)
                {
                    p+=3;
                    p2 = rtp_strstr(p, "<br>");
                    if (p2)
                    {   /* Convert <br> to a newline */
                        *p2=' '; *(p2+1)=' ';*(p2+2)=' ';*(p2+3)='\n';
                        p2+=3;
                    }
                    p2 = rtp_strstr(p, "</b>");
                    if (p2)
                        *p2=0;
                }
                else
                    p=&read_buffer[0];
                rtp_printf("%s",p);
            }
        }
     } while (result > 0);

    /* Now we are done; close the session (see note above about sessions) */
     HTTP_ManagedClientCloseSession(session);

     /* When all HTTP client activity is completed, the managed client context may safely be destroyed */
     HTTP_ManagedClientDestroy(phttpClient);


    if (savetofile)
    {
        rtp_file_close(fd);
    }
     if (result == 0)
     {
         rtp_printf("\nSuccess: (%d) bytes retrieved from %s/%s\n", totalresult, urlpath, urlfile);
         rtp_gui_prompt_text(" ","Press return", &urlpath[0]);
         return(0);
     }
     else
     {
         rtp_printf("\nFailure: (%d) bytes retrieved from %s/%s\n", totalresult, urlpath, urlfile);
		 rtp_gui_prompt_text(" ","Press return", &urlpath[0]);
         return(-1);
     }
}



static int http_client_post(HTTPManagedClient* phttpClient,char *defaulturl, int setajaxvalue)
{
HTTPManagedClientSession* session = 0;
char urlpath[255];
char urlfile[255];
char datatopost[255];
char contenttype[255];
HTTP_INT32 result = -1;
HTTP_INT32 totalresult = 0;
HTTPResponseInfo info;
HTTP_INT32 contentLength;



    rtp_strcpy(contenttype, "application/x-www-form-urlencoded"); /* content-type */

    rtp_printf("You may use the defaults if the web server is running in the background\n\n");
     urlpath[0]=0;
     do {
        if (defaulturl)
            rtp_printf("Press return to select '%s' or enter an IP or DNS address\n",defaulturl);
        rtp_gui_prompt_text(" ","IP Address (eg: 192.161.2.1) or domain name of host (eg:www.google.com etc)\n    :", &urlpath[0]);
        if (!urlpath[0]&&defaulturl)
            rtp_strcpy(urlpath,defaulturl);
        } while (!urlpath[0]);

     if (setajaxvalue)
     {
        char value[32];
        rtp_gui_prompt_text(" ","Enter the new integer value (0-100)\n    :", &value[0]);
        rtp_strcpy(&urlfile[0],"/demo_ajax_setval_submit");
        rtp_printf("posting: to %s\n",&urlfile[0]);
        rtp_sprintf(&datatopost[0],"AjaxSetVal=%s",&value[0]);
        rtp_printf("posting: data %s\n",&datatopost[0]);
    }
    else
    {
        do {
               rtp_printf("Press return to select '/demo_form_submit' or enter a post action\n");
               rtp_gui_prompt_text(" ","File (form name): (eg: /requestinfo.php", &urlfile[0]);
               if (!urlfile[0])
                    rtp_strcpy(urlfile,"/demo_form_submit");
        } while (!urlfile[0]);
        if (!setajaxvalue)
        {
            datatopost[0]=0;
            do {
               rtp_printf("Press return to select 'value0=Hello&value1=World' or enter values to post\n");
               rtp_gui_prompt_text(" ","Post data to be sent: (eg:value1=100&value2=50)", &datatopost[0]);
               if (!datatopost[0])
                    rtp_strcpy(datatopost,"value0=Hello&value1=World");
            } while (!datatopost[0]);

        }
    }
    contentLength = rtp_strlen(&datatopost[0]);

    /* A HTTPManagedClientSession is the abstraction for a SINGLE HTTP request/response pair.
    Thus a new session must be opened for each HTTP operation
    (although this may not cause a new connection to be established, if keep alive is used),
    and closed when the operation has completed (though, again, this may not actually close a
    physical network connection) */

    if (HTTP_ManagedClientStartTransaction (
     phttpClient,
     &urlpath[0],
     80,
      4,
     HTTP_SESSION_TYPE_TCP,
     1, /* blocking? */ &session ) < 0)
    {
         rtp_printf("Failed: connecting to %s\n", urlpath);
         return(-1);
    }


    /* Once the session is open, one command may be issued; in this case a Post */
     HTTP_ManagedClientPost ( session,
     urlfile, /* path */
     contenttype,
     contentLength /* content-length */ );

     /* write the POST data */
    HTTP_ManagedClientWrite (session, (HTTP_UINT8*) &datatopost[0], contentLength);

/* this function must be called when all data has been written */
    HTTP_ManagedClientWriteDone (session);

/* This may be called at any time after the command is issued to get information about the
   server response; this info includes the status code given, date when the request was
   processed, file mime type information (if a file is transferred as the result of a
   command), authentication information, etc. */

   HTTP_ManagedClientReadResponseInfo(session, &info);

   do { /* Read data from the session */
           result = HTTP_ManagedClientRead(session, read_buffer, read_buffer_size);
		   if (result)
		   {
			   read_buffer[result]=0;
				rtp_printf("%s\n",read_buffer);
		   }
   } while (result > 0);


   /* Now we are done; close the session (see note above about sessions) */
   HTTP_ManagedClientCloseSession(session);

   /* When all HTTP client activity is completed, the managed client context may safely be destroyed */
   HTTP_ManagedClientDestroy(phttpClient);


   if (result == 0)
   {
    rtp_printf("Success: posting to %s%s\n", urlpath, urlfile);
	rtp_gui_prompt_text(" ","Press return", &urlpath[0]);
    return(0);
    }
    else
    {
        rtp_printf("Failure: posting to %s%s\n",  urlpath, urlfile);
	    rtp_gui_prompt_text(" ","Press return", &urlpath[0]);
        return(-1);
    }
}


static int http_client_put(HTTPManagedClient* phttpClient)
{
HTTPManagedClientSession* session = 0;
char urlpath[255];
char urlfile[255];
char localfile[255];
char contenttype[255];
HTTP_INT32 result = -1;
HTTP_INT32 totalresult = 0;
HTTPResponseInfo info;
HTTP_INT32 contentLength;
RTP_HANDLE  fd;
HTTP_INT32 nread;

    rtp_strcpy(contenttype, "application/x-www-form-urlencoded"); /* content-type */

    rtp_gui_prompt_text(" ","IP Address (eg: 192.161.2.1) or domain name of host (eg: www.google.com)\n    :", &urlpath[0]);
    rtp_gui_prompt_text(" ","File name on server\n    :", &urlfile[0]);
    rtp_gui_prompt_text(" ","Local file name to put\n    :", &localfile[0]);
    rtp_gui_prompt_text(" ","Content type eg: text/html\n    :", &contenttype[0]);

    contentLength = 0;            /* Set content length to zero so we use chunked encoding */

    /* A HTTPManagedClientSession is the abstraction for a SINGLE HTTP request/response pair.
    Thus a new session must be opened for each HTTP operation
    (although this may not cause a new connection to be established, if keep alive is used),
    and closed when the operation has completed (though, again, this may not actually close a
    physical network connection) */

    if (HTTP_ManagedClientStartTransaction (
     phttpClient,
     &urlpath[0],
     80,
      4,
     HTTP_SESSION_TYPE_TCP,
     1, /* blocking? */ &session ) < 0)
    {
         rtp_printf("Failed: connecting to %s\n", urlpath);
         return(-1);
    }


    /* Once the session is open, one command may be issued; in this case a Post */
     HTTP_ManagedClientPut ( session,
     urlfile, /* path */
     contenttype,
     contentLength /* content-length */ );

    if (rtp_file_open (&fd, (const char *) &localfile[0], RTP_FILE_O_RDONLY, 0) < 0)
    {
         rtp_printf("Failure opening %s\n", localfile);
         return(-1);
    }

     /* write the POST data */
    do
    {
         nread = rtp_file_read(fd,read_buffer, (long)read_buffer_size);
         if (nread > 0)
            HTTP_ManagedClientWrite (session, (HTTP_UINT8*) &read_buffer[0], nread);
    } while (nread > 0);

/* this function must be called when all data has been written */
    HTTP_ManagedClientWriteDone (session);

/* This may be called at any time after the command is issued to get information about the
   server response; this info includes the status code given, date when the request was
   processed, file mime type information (if a file is transferred as the result of a
   command), authentication information, etc. */

   HTTP_ManagedClientReadResponseInfo(session, &info);

   do { /* Read data from the session */
           result = HTTP_ManagedClientRead(session, read_buffer, read_buffer_size);
   } while (result > 0);


   /* Now we are done; close the session (see note above about sessions) */
   HTTP_ManagedClientCloseSession(session);

   /* When all HTTP client activity is completed, the managed client context may safely be destroyed */
   HTTP_ManagedClientDestroy(phttpClient);

   rtp_file_close(fd);

   if (result == 0)
   {
           rtp_printf("Success: putting file: %s to %s%s\n", localfile, urlpath, urlfile);
           return(0);
    }
    else
    {
           rtp_printf("Failed: putting file: %s to %s%s\n", localfile, urlpath, urlfile);
        return(-1);
    }
}

//
// HTTPSRVFRAMWORK.C -
//
// EBS -
//
// Copyright EBS Inc. , 2013
// All rights reserved.
// This code may not be redistributed in source or linkable object form
// without the consent of its author.
//

/*****************************************************************************/
// Header files
/*****************************************************************************/
#include "rtpstr.h"
#include "rtpscnv.h"
#include "httpsrv.h"
#include "htmlutils.h"


/*****************************************************************************/
// Macros
/*****************************************************************************/


/*****************************************************************************/
// Types
/*****************************************************************************/

/*
    Process a page request.

    Process Arguments
    Process Headers

    Bundle, ctx, session, request, client address into structure.
    Update formname

    Todo: Add Virtual File open handler to HTTPServerPath, so file content can be virtualized.
    Todo: Add Virtual File handle ?.

    If it is a GET
        Check HTTPServerPath->userData
            If user supplied handler
                Call handler
            Else
                Process as ASP


    If it is a POST
        Update Content-Len value.

    If it is a PUT
        ???





    By formname



*/

/*---------------------------------------------------------------------------
  HTTP_FrameWork_AddPage
  ---------------------------------------------------------------------------*/
/** @memo Add a page name to be processed as a GET target by the framework.

    @doc  This assigns a URL to be processed via user callback. The callback processing may be performed
    at a low level through a raw callback function or at a higher level using Asp callbacks.

    @see  HTML_UnEscapeFormString

    @return Length of escaped string.
 */

 typedef int *(HTTP_RawCallback) (void *userData, HTTPServerRequestContext *ctx, HTTPSession *session, HTTPRequest *request, RTP_NET_ADDR *clientAddr);

HTTP_BOOL HTTP_FrameWork_AddPage (
                            HTTPFrameWorkPageObject PageObject,                           /**  Uninitialized page object, must persist for the length of the session */
                            const HTTP_CHAR *pPath,                            /**  Pathname /MyPage.html for example or /MyGetFormPost: Must persist for the length of the session */
                            HTTP_UINT32 Flags,                                 /**  Length of the memory image if non null. */
                            const HTTP_CHAR *pContent,                         /**  If non-null provides a memory image of the content */
                            HTTP_UINT32 ContentLength,                         /**  Length of the memory image if non null. */
                            HTTP_RawCallback RawCallback,                      /**  Callback function. If non null intercepts processing after header processing */
                            HTTPFrameWorkAspTable *LocalAspCallbackTable       /**  If non-null this callback list is added for this page.: Must persist for the length of the session */
                            )
{

    return HTTP_TRUE;
}



// void *userData, HTTPServerRequestContext *ctx, HTTPSession *session, HTTPRequest *request, RTP_NET_ADDR *clientAddr)



/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------
  HTML_EscapeFormString
  ---------------------------------------------------------------------------*/
/** @memo Escape out characters from form string.

    @doc  This routine converts a form string to escaped form as per RFC 1945.

    @see  HTML_UnEscapeFormString

    @return Length of escaped string.
 */

int HTTP_FrameWork_cb (void *userData, HTTPServerRequestContext *ctx, HTTPSession *session, HTTPRequest *request, RTP_NET_ADDR *clientAddr)
{


}



extern int             _HTTP_ServerHeaderCallback      (void *userData,
                                                 HTTPSession *session,
                                                 HTTPHeaderType type,
                                                 const HTTP_CHAR *name,
                                                 const HTTP_CHAR *value);
#if (0)
_HTTP_ServerHeaderCallback( HTTPServerRequestContext *server,
                            HTTPSession *session,
                            HTTPServerHeaderCallback processHeader,
                            void *userData,
                            HTTP_UINT8 *buffer,
                            HTTP_INT32 size)
{
	return (_HTTP_ServerReadHeaders(server, session, processHeader, userData, buffer, size));
}
#endif

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
 //   								rtp_strcpy(ExampleServerCtx.rootDirectory, value);
 //   								ExampleServerCtx.ModuleRequestingReload = 1;
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
//								rtp_strcpy(ExampleServerCtx.rootDirectory, value);
//								ExampleServerCtx.ModuleRequestingReload = 1;
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

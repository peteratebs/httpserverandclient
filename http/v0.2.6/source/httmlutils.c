//
// HTMLUTILS.C -
//
// EBS -
//
// Copyright EBS Inc. , 2006
// All rights reserved.
// This code may not be redistributed in source or linkable object form
// without the consent of its author.
//

/*****************************************************************************/
// Header files
/*****************************************************************************/
#include "httpp.h"
#include "httpsrv.h"
#include "htmlutils.h"
#include "rtpstr.h"
#include "rtpscnv.h"



/*****************************************************************************/
// Macros
/*****************************************************************************/


/*****************************************************************************/
// Types
/*****************************************************************************/



/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------
  HTML_EscapeFormString
  ---------------------------------------------------------------------------*/
/** @memo Escape out characters from form string.

    @doc  This routine converts a form string to escaped form as per RFC 1945.

    @see  HTML_UnEscapeFormString

    @return Length of escaped string.
 */

static char specialChars[] = {'@', /* <-- reserved */
	'$', '-', '_', '.', '!', '*', '(', ')', ',', '\0' /* <-- special */};

int HTML_EscapeFormString(HTTP_CHAR *str, const HTTP_CHAR *from)
{
int fpos,spos;

	for (fpos=0,spos=0;from[fpos];fpos++)
	{
		if (from[fpos]==' ')
		{
			str[spos]='+';
			spos++;
		}
		else if((from[fpos]>='0' && from[fpos]<='9')||
				(from[fpos]>='A' && from[fpos]<='Z')||
				(from[fpos]>='a' && from[fpos]<='z')||
				 rtp_strchr(specialChars, (char) from[fpos]))
		{
			str[spos]=from[fpos];
			spos++;
		}
		//if this is a new line or a carriage return then convert it to %0D%0A
		else if (from[fpos]=='\r' || from[fpos]=='\n')
		{
			rtp_strcpy(&(str[spos]),"%0D%0A");
			spos+=6;
		}
		else
		{
            char buffer[32];
			str[spos] = '%';
            rtp_itoa ((int)from[fpos],buffer,16);
            if (!buffer[1])
            {   /* Convert 1 byte hex to two bytes with a leading 0 */
                buffer[1]=buffer[0];
                buffer[0]='0';
                buffer[2]=0;
            }
            rtp_memcpy(&(str[spos+1]),buffer,2);
			spos+=3;
		}
	}
	str[spos]='\0';
	return(spos);
}

/*---------------------------------------------------------------------------
  HTML_UnEscapeFormString
  ---------------------------------------------------------------------------*/
/** @memo Escape out characters from form string.

    @doc  This routine converts a form string from escaped form as per RFC 1945.
          & characters are converted to zeroes for the convenience of name value pair parser.

    @see  HTML_EscapeFormString

    @return Length of unescaped string.
 */

int HTML_UnEscapeFormString(HTTP_CHAR *str, const HTTP_CHAR *from)
{
int fpos,spos;

	for (fpos=0,spos=0;from[fpos];)
	{
		if (from[fpos]=='+')
		{
			str[spos++]='.';
            fpos++;
		}
		else if (from[fpos]=='%')
        {
            HTTP_CHAR buffer[32];
            rtp_memcpy(buffer, &(from[fpos+1]),2);
            buffer[2]=0;
			str[spos++]=(HTTP_CHAR)rtp_hatoi(buffer);
            fpos += 3;
        }
		else if (from[fpos]=='&')
        {
			str[spos++]=0;
			fpos++;
        }
        else
			str[spos++]=from[fpos++];
	}
	str[spos]='\0';
	return(spos);
}



/** @memo Initialize an object to contain an array of name value pairs retrieved from the headers.

    @doc  Initialize an object with storage to contain an array of name value pairs retrieved from the headers. An optional
    filter function may be provided to

    @see HTTP_LoadNameValuePairs HTTP_GetNameValuePairAssociative, HTTP_GetNameValuePairIndexed

    @return nothing
 */

void HTTP_InitHeaderNameValuePairList ( PNVHeaderList pNVHeaderList,                /**     Pointer to uninitialized object.     */
                                        const   HTTP_CHAR* Bufferbase,              /**     Pointer to array to hold pair values */
                                        int len,                                    /**     Size of the array to hold pair values */
                                        HTTP_BOOL (* Filter)(const HTTP_CHAR *name, void *FilterArgs), /**     If non null, this fuction will be called with the name to test if the value is wanted */
                                        void * FilterArgs                           /**     User specified arguments passed to the filter (perhaps list of desired items) */
                                        )
{
    CLEARMEMPOBJ(pNVHeaderList);
    pNVHeaderList->Bufferbase=Bufferbase;
    pNVHeaderList->BufferSize=len;
    pNVHeaderList->Filter = Filter;
    pNVHeaderList->FilterArgs = FilterArgs;
    HTTP_InitNameValuePairList ( &pNVHeaderList->PairList, pNVHeaderList->PairItems, ARRAYSIZE(pNVHeaderList->PairItems));
}

/** @memo Callback routine that loads header values into a name value pairs list.

    @doc  Use this routine as the callback for HTTP_ServerReadHeaders with the user args pointing to a PNVHeaderList object (pNVHeaderList) intialized by
          HTTP_InitHeaderNameValuePairList. This will populate pNVHeaderList->PairList with NV pairs aof all the header elements. The NV pairs in pNVHeaderList->PairList
          can be accessed with HTTP_GetNameValuePairAssociative and HTTP_GetNameValuePairIndexed.

          The callback will extract as many pairs as possible, if the buffering is too small it will continue processing but not copy the nane value pairs.
          If PNVHeaderList->BufferUsed is greater than PNVHeaderList->BufferSize then this probably happened.


    @see HTTP_ServerReadHeaders

    @return nothing
 */

int HTTP_CallbackHeaderToNameValuePairList(
		void *userData,
		HTTPServerRequestContext *server,
		HTTPSession *session,
		HTTPHeaderType type,
		const HTTP_CHAR *name,
		const HTTP_CHAR *value)
{
PNVHeaderList pNVHeaderList;
PNVPairList pNVPairList;
int nameLen, valLen;
int doLoad = 0;
	if (!name || !value)
        return 0;

    pNVHeaderList = (PNVHeaderList)userData;
    pNVPairList = &pNVHeaderList->PairList;

    /* Skip it if we don't want it. */
    if (pNVHeaderList->Filter && !pNVHeaderList->Filter(name, pNVHeaderList->FilterArgs))
        return 0;
    nameLen= rtp_strlen(name);
    valLen = rtp_strlen(value);
    if (pNVHeaderList->BufferUsed+nameLen+valLen+2 <= pNVHeaderList->BufferSize)
        doLoad = 1;
    if (pNVHeaderList->PairList.len == pNVHeaderList->PairList.MaxPairs)
        doLoad = 0;
    /* If we have space load, otherwise continue and measure the size needed */

    if (doLoad)
    {
        pNVHeaderList->PairList.Items[pNVHeaderList->PairList.len].name = pNVHeaderList->Bufferbase+pNVHeaderList->BufferUsed;
        rtp_strcpy(pNVHeaderList->PairList.Items[pNVHeaderList->PairList.len].name, name);
        pNVHeaderList->BufferUsed += nameLen+1;

        pNVHeaderList->PairList.Items[pNVHeaderList->PairList.len].value = pNVHeaderList->Bufferbase+pNVHeaderList->BufferUsed;
        rtp_strcpy(pNVHeaderList->PairList.Items[pNVHeaderList->PairList.len].value, value);
        pNVHeaderList->BufferUsed += valLen+1;
        pNVHeaderList->PairList.len += 1;
    }
    else
    {
        pNVHeaderList->BufferUsed += valLen+nameLen+2;
    }
    return 0;
}


/** @memo Initialize an object to contain an array of name value pairs.

    @doc  Initialize an object to contain an array of name value pairs.

    @see HTTP_LoadNameValuePairs HTTP_GetNameValuePairAssociative, HTTP_GetNameValuePairIndexed

    @return nothing
 */

void HTTP_InitNameValuePairList (PNVPairList pNVPairList,   /**     Pointer to uninitialized object.     */
                                 PNVPair pPairArray,        /**     Pointer to array to hold pairs */
                                 int len                    /**     Szie of the array to hold pairs */
                                 )
{
    CLEARMEMPOBJ(pNVPairList);
	pNVPairList->MaxPairs=len;
    pNVPairList->Items=pPairArray;
}


/** @memo Traverse a buffer of name value pairs and create an index.

    @doc  The name value pairs must be seperated by nulls and the length must be known in advance.
          pvnPairs array is poulated with pointers to name an value.

    @see HTML_UnEscapeFormString HTTP_InitNameValuePairList, HTTP_GetNameValuePairAssociative, HTTP_GetNameValuePairIndexed

    @return The bytes consumed. -1 if the pnvPairs was used up. 0 if no pairs found.
 */


HTTP_INT32 HTTP_LoadNameValuePairs ( HTTP_CHAR* str,                  /**     Pointer to unescaped buffer. HTML_UnEscapeFormString      */
                                     HTTP_INT32 strlength,                 /**     Length of the buffer. */
                                     PNVPairList pNVPairList               /**     Pair object to populate. Must have been previously initalized by HTTP_InitNameValuePairList */
                                    )
{
    int nConsumed = 0;
	pNVPairList->len = 0;
    while (nConsumed < strlength)
    {
        HTTP_INT32 nThisByte;
        if (pNVPairList->len == pNVPairList->MaxPairs)
            return -1;
        pNVPairList->Items[pNVPairList->len].name = pNVPairList->Items[pNVPairList->len].value = 0;
        nThisByte = HTTP_ParseNameValuePairByReference (str,	 &pNVPairList->Items[pNVPairList->len].name, &pNVPairList->Items[pNVPairList->len].value);
        if (nThisByte == 0)
            break;
		str += nThisByte;
        nConsumed += nThisByte;
        pNVPairList->len++;
    }
    return nConsumed;
}

/** @memo Search a value pair list for a name and return the pair.

    @doc  Search a value pair list for a name and return the pair.

    @see HTTP_LoadNameValuePairs, HTTP_GetNameValuePairIndexed

    @return The matched pair or 0 if none found.
 */


PNVPair HTTP_GetNameValuePairAssociative (PNVPairList pNVPairList,                /**     Pair structures to search. */
                                             const HTTP_CHAR* name                /**     Name to search for */
                                    )
{
    int i;
    for (i = 0; i < pNVPairList->len; i++)
    {
        if (!rtp_strcmp(pNVPairList->Items[i].name , name))
            return &pNVPairList->Items[i];
    }
    return 0;
}

/** @memo Search a value pair list for a name and return the pair.

    @doc  Search a value pair list for a name and return the pair.

    @see HTTP_LoadNameValuePairs, HTTP_GetNameValuePairIndexed

    @return The matched pair or 0 if none found.
 */


PNVPair HTTP_GetNameValuePairIndexed (PNVPairList pNVPairList,                /**     Pair structures to search. */
                                    int index                               /**     Index */
                                    )
{
    if (index < pNVPairList->len)
    {
        return &pNVPairList->Items[index];
    }
    return 0;
}


/*---------------------------------------------------------------------------

String consisting of name value pairs seperated by nulls

    If the value is quoted the leading quote is skipped and the terminating quote is replaced with zero.

    Returns:
        *name points to the name, zero if no name.
        *value points to the value, zero if no value.

    returns number of bytes consumed.

*/
/** @memo Extract the next name value pair from a buffer.

    @doc  Extract the next name value pair from a buffer. The buffer is consists of name=value pair separated by nulls.
          This routines find the name and value portions.
          It null terminates the name portion by replacing the '=' with null. If the value string is quoted the leading quote is skipped
          and the trailing quote is replaced with null.


    @see  HTTP_LoadNameValuePairs

    @return The bytes consumed or 0 if none found. Name and or value are null valued if not found.
 */

HTTP_INT32 HTTP_ParseNameValuePairByReference (
		HTTP_CHAR* str,
		const HTTP_CHAR** name,
		const HTTP_CHAR** value
	)
{
	const HTTP_CHAR* strBegin = str;
	*name = *value = 0;

	while (IS_WHITESPACE(str[0]))
	{
		str++;
	}

	*name = str;
	while (str[0] && str[0] != '=')
		str++;

	while (str[0] && str[0] != '=')
		str++;

	if (str[0] != '=')
	{
		return (0);
	}
	str[0] = 0;
	str++;

	while (IS_WHITESPACE(str[0]))
	{
		str++;
	}
	if (str[0] == '\'' || str[0] == '"')
	{
		str++;
	}
	*value = str;
	while (str[0] && str[0] != '\'' && str[0] != '"')
	{
		str++;
	}

	if (str[0] == '\'' || str[0] == '"')
	{
	    str[0] = 0;
		str++;
	}
	str ++;		/* Skip the terminating zero */
	return (str - strBegin);
}

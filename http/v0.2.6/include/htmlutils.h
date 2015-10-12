//
// HTMLUTILS.H - Structs, prototypes, and defines for HTML
//
// EBS - Common Library
//
//  $Author: vmalaiya $
//  $Date: 2006/09/28 20:09:02 $
//  $Name:  $
//  $Revision: 1.1 $
//
// Copyright EBS Inc. , 2006
// All rights reserved.
// This code may not be redistributed in source or linkable object form
// without the consent of its author.
//

#ifndef __HTMLUTILS_H__
#define __HTMLUTILS_H__



#ifdef __cplusplus
extern "C" {
#endif
#define UPPERCASE(X) (((X) >= 'a' || (X) <= 'z')? ((X) + 'A' - 'a') : (X))
#define IS_WHITESPACE(C) ((C)==' ' ||  (C)=='\r' || (C)=='\n' || (C)=='\t' || (C)=='\f' || (C)=='\v')
#define IS_DIGIT(X)      ((X)>='0' && (X)<='9')
#define CLEARMEMPOBJ(POBJ) rtp_memset(POBJ, 0, sizeof(*POBJ))
#define CLEARMEMOBJ(POBJ)  rtp_memset(&OBJ, 0, sizeof(OBJ))
#define ARRAYSIZE(A) (sizeof(A)/sizeof(A[0]))
int HTML_EscapeFormString(HTTP_CHAR *str, const HTTP_CHAR *from);
int HTML_UnEscapeFormString(HTTP_CHAR *str, const HTTP_CHAR *from);


typedef struct s_NVPair
{
    HTTP_CHAR* name;
    HTTP_CHAR* value;
} NVPair, *PNVPair;


typedef struct s_NVPairList
{
    int     MaxPairs;
    int     Iterator;
    int     len;
    PNVPair Items;
} NVPairList, *PNVPairList;



void        HTTP_InitNameValuePairList (PNVPairList pNVPairList,  PNVPair pPairArray, int len );
HTTP_INT32  HTTP_LoadNameValuePairs (HTTP_CHAR* str, HTTP_INT32 strlength, PNVPairList pNVPairList);
PNVPair     HTTP_GetNameValuePairAssociative (PNVPairList pNVPairList, const HTTP_CHAR* name);
PNVPair     HTTP_GetNameValuePairIndexed (PNVPairList pNVPairList, int index);
HTTP_INT32 HTTP_ParseNameValuePairByReference (HTTP_CHAR* str,const HTTP_CHAR** name,const HTTP_CHAR** value);


typedef struct s_NVHeaderList
{
    HTTP_CHAR * Bufferbase;  /* Raw storage for copied value pairs */
    int     BufferSize;
    int     BufferUsed;
    HTTP_BOOL (* Filter)(const HTTP_CHAR *name, void *FilterArgs);
    void * FilterArgs;
#define HTTP_MAXHEADERLIST_SIZE 64
    NVPairList PairList;
	NVPair	   PairItems[HTTP_MAXHEADERLIST_SIZE];
} NVHeaderList, *PNVHeaderList;


void HTTP_InitHeaderNameValuePairList ( PNVHeaderList pNVHeaderList,                /**     Pointer to uninitialized object.     */
                                        const   HTTP_CHAR* Bufferbase,              /**     Pointer to array to hold pair values */
                                        int len,                                    /**     Size of the array to hold pair values */
                                        HTTP_BOOL (* Filter)(const HTTP_CHAR *name, void *FilterArgs), /**     If non null, this fuction will be called with the name to test if the value is wanted */
                                        void * FilterArgs                           /**     User specified arguments passed to the filter (perhaps list of desired items) */
                                        );

int HTTP_CallbackHeaderToNameValuePairList(
		void *userData,
		HTTPServerRequestContext *server,
		HTTPSession *session,
		HTTPHeaderType type,
		const HTTP_CHAR *name,
		const HTTP_CHAR *value);

#ifdef __cplusplus
}
#endif

#endif /* __HTMLUTILS_H__ */

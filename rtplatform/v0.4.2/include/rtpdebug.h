 /*
 | RTPDEBUG.H - Runtime Platform Debug Services
 |
 |   UNIVERSAL CODE - DO NOT CHANGE
 |
 | EBS - RT-Platform
 |
 |  $Author: vmalaiya $
 |  $Date: 2006/07/17 15:29:00 $
 |  $Name:  $
 |  $Revision: 1.3 $
 |
 | Copyright EBS Inc. , 2006
 | All rights reserved.
 | This code may not be redistributed in source or linkable object form
 | without the consent of its author.
 |
 | Module description:
 |  [tbd]
*/

#ifndef __RTPDEBUG_H__
#define __RTPDEBUG_H__

#include "rtp.h"


/************************************************************************
 * If RTP_DEBUG is defined, the debug implementaion of the macro        *
 * will be implemented.  This provides debug information otherwise not  *
 * available.                                                           *
 ************************************************************************/

/* Uncomment the below code to enable debug information **/
//#ifndef RTP_DEBUG
//#define RTP_DEBUG 1
//#endif

/************************************************************************
 * Debug System API
 ************************************************************************/

/************************************************************************
 * Debug System API which should be
 * used via the macros indicated below.
 ************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

void _rtp_debug_output_str (
	char* msg,
	const char *file,
	long line_num
	);
void _rtp_debug_output_errno (
	char* msg,
	const char *file,
	long line_num
	);
void _rtp_debug_output_neterrno (
	char* msg,
	long err,
	const char *file,
	long line_num
	);
void _rtp_debug_output_int (
	long val
	);

#ifdef __cplusplus
}
#endif


#ifdef RTP_DEBUG
#define RTP_DEBUG_OUTPUT_NETERRNO(msg,err) _rtp_debug_output_neterrno(msg,err,__FILE__, __LINE__)
#define RTP_DEBUG_OUTPUT_ERRNO(msg) _rtp_debug_output_errno(msg, __FILE__, __LINE__)
#define RTP_DEBUG_OUTPUT_STR(msg)  _rtp_debug_output_str(msg, __FILE__, __LINE__)
#define RTP_DEBUG_OUTPUT_INT(val)  _rtp_debug_output_int(val)
#else
#define RTP_DEBUG_OUTPUT_NETERRNO(msg,err)
#define RTP_DEBUG_OUTPUT_ERRNO(msg)
#define RTP_DEBUG_OUTPUT_STR(msg)
#define RTP_DEBUG_OUTPUT_INT(msg)
#endif /* RTP_DEBUG */


#endif /* __RTPDEBUG_H__ */

/* ----------------------------------- */
/*             END OF FILE             */
/* ----------------------------------- */

/** ***************************************************************************
 * Description       : VBIT Teletext Page Parser
 * Compiler          : GCC
 *
 * Copyright (C) 2010, Peter Kwan
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice and this
 * permission notice and warranty disclaimer appear in supporting
 * documentation, and that the name of the author not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * The author disclaims all warranties with regard to this
 * software, including all implied warranties of merchantability
 * and fitness.  In no event shall the author be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 *************************************************************************** **/
#ifndef _PAGE_H_
#define _PAGE_H_
#include "xitoa.h"
#include "../SDCard/ff.h"
#include "../SDCard/diskio.h"

/** Structure to hold page details after parsing
 */
typedef struct _PAGE_ 
{
	char filename[40]; /// May need to cut this down
	unsigned char mag;		/// 1..8 magazine number
	unsigned char page;		/// 00..99 page number
	unsigned char subpage;	/// 00..99 (not part of ETSI spec)
	unsigned int subcode;	/// subcode (we use it to hold subpage)
	unsigned char timerMode; /// C=times around magazine, T=timed
	unsigned int time;		/// seconds
	unsigned int control;	/// C bits and non ETSI bits (See tti specification)
	unsigned int filesize;	/// Size (bytes) of the file that this page was parsed from
} PAGE;

/** Parse a single line of a tti teletext page
 * \param str - String to parse
 * \param page - page to return values in
 * \return true if there is an error
 */
unsigned char ParseLine(PAGE *page, char *str);

/** Parse a teletext page
 * \param filaname - Name of the teletext file
 * \return true if there is an error
 */
unsigned char ParsePage(PAGE *page, char *filename);

/** Clear a page structure
 * The mag is set to 0x99 as a signal that it is not valid
 * \param page - Pointer to PAGE structure
 */
void ClearPage(PAGE *page);

#endif

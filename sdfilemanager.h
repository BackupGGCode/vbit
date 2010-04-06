/** ***************************************************************************
 * Description       : VBIT SD File Manager for XMEGA
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
#ifndef SDFileManager
#define SDFilemanager
#include <avr/io.h>
#include <avr/pgmspace.h>
#include "../SDCard/ff.h"
#include "../SDCard/diskio.h"
#include <string.h>
#include "xitoa.h"
#include "page.h"
#include <stdio.h>

/** Create the magazine and carousel list files
 */
void SDCreateLists(int mag, unsigned int pagecount);
#endif
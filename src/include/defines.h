/*
    bbs100 2.2 WJ105
    Copyright (C) 2005  Walter de Jong <walter@heiho.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
/*
	defines.h
*/

#ifndef DEFINES_H_WJ99
#define DEFINES_H_WJ99 1

#include <config.h>

#ifndef HAVE_UNISTD_H
#error C header file unistd.h is missing -- are you on a Unix platform?
#endif

#define MAX_NAME            18
#define MAX_TITLE			(MAX_NAME+10)
#define MAX_LINE            80
#define MAX_X_LINES         7
#define MAX_CRYPTED_PASSWD  140

#define MAX_PATHLEN			1024		/* MAXPATH variable */
#define MAX_OUTPUTBUF       128			/* per-user output buffer */
#define MAX_INPUTBUF		128			/* per-user input buffer */
#define PRINT_BUF			512

#define ONE_SECOND			1
#define SECS_IN_MIN			(60 * ONE_SECOND)
#define SECS_IN_HOUR		(60 * SECS_IN_MIN)
#define SECS_IN_DAY			(24 * SECS_IN_HOUR)
#define SECS_IN_WEEK		(7 * SECS_IN_DAY)

#endif	/* DEFINES_H_WJ99 */

/* EOB */

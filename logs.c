/*
 * Copyright (c) 2012-2018 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <proto/exec.h>
#include <exec/types.h>
#include <proto/dos.h>
#include <dos/dos.h>
#include <proto/utility.h>
#include <libvstring.h>

#include "globaldefines.h"
#include "support.h"
#include "locale.h"
#include "logs.h"

/* «KwaKwa» */

static VOID CheckName(STRPTR s) /* checks for forbidden chars in file name and changes them to '_' */
{
	while(*s)
	{
		if(*s == '/' || *s == ':' || *s == '?' || *s == '#' || *s == '(' || *s == ')' || *s == '|' ||
			*s == '~' || *s == '%' || *s == '(' || *s == '"' || *s == '[' || *s == ']')
		{
			*s = '_';
		}
		s++;
	}
}

BPTR LogOpen(STRPTR log_name)
{
	UBYTE buffer[LOGS_DRAWER_LEN + StrLen(log_name) + 2];
	BPTR result;

	FmtNPut((STRPTR)buffer, "%s/%s", sizeof(buffer), LOGS_DRAWER, log_name);

	CheckName((STRPTR)(buffer + LOGS_DRAWER_LEN + 1)); /* check only file name in copy (do not modify original string)*/

	if((result = Open((CONST_STRPTR)buffer, MODE_READWRITE)))
	{
		struct ClockData cd;

		Seek(result, 0, OFFSET_END);

		ActLocalTimeToClockData(&cd);

		FPrintf(result, "%ls [%02ld-%02ld-%04ld %02ld:%02ld:%02ld]\n", GetString(MSG_LOG_OPENED), cd.mday, cd.month, cd.year, cd.hour, cd.min, cd.sec);
	}

	return result;
}

VOID LogAdd(BPTR fh, STRPTR author, STRPTR message, ULONG timestamp)
{
	if(fh)
	{
		struct ClockData cd;

		Amiga2Date(timestamp, &cd);

		FPrintf(fh, "[%02ld:%02ld:%02ld]: <%ls> %ls\n", cd.hour, cd.min, cd.sec, author, message);
	}
}

VOID LogClose(BPTR fh)
{
	if(fh)
	{
		struct ClockData cd;

		ActLocalTimeToClockData(&cd);

		FPrintf(fh, "%ls [%02ld-%02ld-%04ld %02ld:%02ld:%02ld]\n\n", GetString(MSG_LOG_CLOSED), cd.mday, cd.month, cd.year, cd.hour, cd.min, cd.sec);

		Close(fh);
	}
}

LONG LogRename(STRPTR old_name, STRPTR new_name)
{
	BYTE old_path[LOGS_DRAWER_LEN + StrLen(old_name) + 2];
	BYTE new_path[LOGS_DRAWER_LEN + StrLen(new_name) + 2];

	FmtNPut((STRPTR)old_path, "%s/%s", sizeof(old_path), LOGS_DRAWER, old_name);
	FmtNPut((STRPTR)new_path, "%s/%s", sizeof(new_path), LOGS_DRAWER, new_name);

	return Rename((STRPTR)old_path, (STRPTR)new_path);
}


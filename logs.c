/*
 * Copyright (c) 2012-2022 Filip "widelec-BB" Maryjanski, BlaBla group.
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

static STRPTR sys_open_msg = NULL, sys_close_msg = NULL;

BOOL SetupLogsSystem(VOID)
{
	sys_open_msg = Utf8ToSystem(GetString(MSG_LOG_OPENED));
	sys_close_msg = Utf8ToSystem(GetString(MSG_LOG_CLOSED));

	MakeDirAll(LOGS_DRAWER);
	MakeDirAll(LOGS_UNICODE_DRAWER);

	return sys_open_msg && sys_close_msg;
}

VOID CleanupLogsSystem(VOID)
{
	if(sys_open_msg)
		StrFree(sys_open_msg);
	if(sys_close_msg)
		StrFree(sys_close_msg);
}

BPTR LogOpen(STRPTR log_name_utf8, BOOL unicode)
{
	BPTR result = NULL;
	STRPTR log_name;
	if((log_name = Utf8ToSystem(log_name_utf8)))
	{
		if(unicode)
		{
			UBYTE buffer[LOGS_UNICODE_DRAWER_LEN + StrLen(log_name) + 2];
			FmtNPut((STRPTR)buffer, "%s/%s", sizeof(buffer), LOGS_UNICODE_DRAWER, log_name);
			CheckName((STRPTR)(buffer + LOGS_UNICODE_DRAWER_LEN + 1)); /* check only file name in copy (do not modify original string)*/
			result = Open((CONST_STRPTR)buffer, MODE_READWRITE);
		}
		else
		{
			UBYTE buffer[LOGS_DRAWER_LEN + StrLen(log_name) + 2];
			FmtNPut((STRPTR)buffer, "%s/%s", sizeof(buffer), LOGS_DRAWER, log_name);
			CheckName((STRPTR)(buffer + LOGS_DRAWER_LEN + 1)); /* check only file name in copy (do not modify original string)*/
			result = Open((CONST_STRPTR)buffer, MODE_READWRITE);
		}

		if(result)
		{
			struct ClockData cd;

			Seek(result, 0, OFFSET_END);

			ActLocalTimeToClockData(&cd);

			FPrintf(result, "%ls [%02ld-%02ld-%04ld %02ld:%02ld:%02ld]\n", unicode ? GetString(MSG_LOG_OPENED) : sys_open_msg,
			 cd.mday, cd.month, cd.year, cd.hour, cd.min, cd.sec);
		}

		StrFree(log_name);
	}

	return result;
}

VOID LogAddSys(BPTR fh, STRPTR author_utf8, STRPTR message_utf8, ULONG timestamp)
{
	if(fh)
	{
		STRPTR author;
		if((author = Utf8ToSystem(author_utf8)))
		{
			STRPTR message;
			if((message = Utf8ToSystem(message_utf8)))
			{
				struct ClockData cd;

				Amiga2Date(timestamp, &cd);

				FPrintf(fh, "[%02ld:%02ld:%02ld]: <%ls> %ls\n", cd.hour, cd.min, cd.sec, author, message);

				StrFree(message);
			}
			StrFree(author);
		}
	}
}

VOID LogAddUnicode(BPTR fh, STRPTR author, STRPTR message, ULONG timestamp)
{
	if(fh)
	{
		struct ClockData cd;

		Amiga2Date(timestamp, &cd);

		FPrintf(fh, "[%02ld:%02ld:%02ld]: <%ls> %ls\n", cd.hour, cd.min, cd.sec, author, message);
	}
}

VOID LogClose(BPTR fh, BOOL unicode)
{
	if(fh)
	{
		struct ClockData cd;

		ActLocalTimeToClockData(&cd);

		FPrintf(fh, "%ls [%02ld-%02ld-%04ld %02ld:%02ld:%02ld]\n\n", unicode ? GetString(MSG_LOG_CLOSED) : sys_close_msg,
		 cd.mday, cd.month, cd.year, cd.hour, cd.min, cd.sec);

		Close(fh);
	}
}

VOID LogsRename(STRPTR old_name_utf8, STRPTR new_name_utf8)
{
	STRPTR old_name;
	if((old_name = Utf8ToSystem(old_name_utf8)))
	{
		STRPTR new_name;
		if((new_name = Utf8ToSystem(new_name_utf8)))
		{
			BYTE old_path[LOGS_UNICODE_DRAWER_LEN + StrLen(old_name) + 2];
			BYTE new_path[LOGS_UNICODE_DRAWER_LEN + StrLen(new_name) + 2];

			FmtNPut((STRPTR)old_path, "%s/%s", sizeof(old_path), LOGS_DRAWER, old_name);
			FmtNPut((STRPTR)new_path, "%s/%s", sizeof(new_path), LOGS_DRAWER, new_name);
			Rename((STRPTR)old_path, (STRPTR)new_path);

			FmtNPut((STRPTR)old_path, "%s/%s", sizeof(old_path), LOGS_UNICODE_DRAWER, old_name);
			FmtNPut((STRPTR)new_path, "%s/%s", sizeof(new_path), LOGS_UNICODE_DRAWER, new_name);
			Rename((STRPTR)old_path, (STRPTR)new_path);

			StrFree(new_name);
		}
		StrFree(old_name);
	}
}


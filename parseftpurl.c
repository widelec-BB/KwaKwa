/*
 * Copyright (c) 2012-2022 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <libvstring.h>
#include <proto/exec.h>

#include "globaldefines.h"
#include "parseftpurl.h"
#include "support.h"

static inline VOID ParseFtpUserPass(STRPTR url, LONG len, STRPTR *user, STRPTR *pass)
{
	STRPTR tmp = url;

	while(*tmp != 0x00 && *tmp != ':')
		tmp++;

	if(*tmp == 0x00) /* no pass information */
	{
		*user = StrNewLen(url, len);
	}
	else
	{
		*user = StrNewLen(url, tmp - url);
		*pass = StrNewLen(tmp + 1, len - (tmp - url) - 1);
	}
}

static inline VOID ParseFtpHostPort(STRPTR url, LONG len, STRPTR *host, ULONG *port)
{
	STRPTR tmp = url;

	while(*tmp != 0x00 && *tmp != ':')
		tmp++;

	if(*tmp == 0x00) /* no port information */
	{
		*host = StrNewLen(url, len);
		*port = 21; /* set default */
	}
	else
	{
		STRPTR tmp2 = tmp + 1;

		*host = StrNewLen(url, tmp - url);

		*port = 0;

		while(_between('0', *tmp2, '9') && tmp2 <= url + len)
		{
			*port *= 10;
			*port += *tmp2 - 0x30;
			tmp2++;
		}
	}
}

static inline VOID ParseFtpPath(STRPTR path, LONG len, STRPTR *result)
{
	*result = StrNewLen(path, len);
}

LONG ParseFtpUrl(STRPTR url, STRPTR *user, STRPTR *pass, STRPTR *host, STRPTR *path)
{
	LONG port = -1;
	STRPTR tmp, tmp2, newurl;
	STRPTR cut = NULL;

	if(url == NULL)
		return -1;

	/* make sure pointers are NULLed */
	*user = *pass = *host = *path = NULL;

	/* first, check if url contain ftp:// */
	if((StrNCmp(url, "ftp://", 6)) == NULL)
		newurl = url + 6;
	else
		newurl = url;

	tmp2 = tmp = newurl;

	/* cut url to two parts before and after '@' */
	while(*tmp != 0x00)
	{
		if(*tmp == '@')
			cut = tmp;

		tmp++;
	}

	if(cut != NULL)
		tmp = cut;

	if(*tmp != '@') /* url contains user information */
	{
		/* no user information in url, set defaults */
		*user = NULL;
		*pass = NULL;
		tmp = newurl; /* make sure that ParseFtpHostPort() will got begining of url */
	}
	else
		ParseFtpUserPass(newurl, tmp++ - newurl, user, pass);

	while(*tmp2 != 0x00 && *tmp2 != '/') /* find where path starts */
		tmp2++;

	ParseFtpHostPort(tmp, tmp2 - tmp, host, &port);

	if(*tmp2 != 0x00) /* if url contains path */
		ParseFtpPath(tmp2, StrLen(tmp2), path);
	else /* if not - set default */
		*path = StrNew("/");

	return port;
}

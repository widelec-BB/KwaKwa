/*
 * Copyright (c) 2012-2022 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef __HTTP_H__
#define __HTTP_H__

#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

struct HttpGet
{
	struct Message hg_Msg;
	STRPTR hg_Url;
	STRPTR hg_UserAgent;
	LONG hg_DataLen;
	UBYTE *hg_Data;
	struct Order *hg_Order;
};

struct HttpPost
{
	struct Message hp_Msg;
	STRPTR hp_Url;
	STRPTR hp_UserAgent;
	struct TagItem *hp_Post;
	LONG hp_DataLen;
	UBYTE *hp_Data;
	struct Order *hp_Order;
};


VOID HttpGetRequest(VOID);
VOID HttpPostRequest(VOID);

#endif /* __HTTP_H__ */

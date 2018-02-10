/*
 * Copyright (c) 2012-2018 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef __SLAVE_H__
#define __SLAVE_H__

#include <intuition/classes.h>
#include <magicaction/magicbeacon.h>
#include "http.h"
#include "ftp.h"

#define ORDER_TYPE_BEACON     0x000000
#define ORDER_TYPE_HTTP_GET   0x000001
#define ORDER_TYPE_HTTP_POST  0x000002
#define ORDER_TYPE_FTP_PUT    0x000003

struct Order
{
	struct Message ob_Msg;
	ULONG ob_Type;
	union
	{
		struct MagicBeaconNotificationMessage obo_Beacon;
		struct HttpGet obo_HttpGet;
		struct HttpPost obo_HttpPost;
		struct FTPPut obo_FtpPut;
	} ob_Order;
	/* callback stuff */
	Object *ob_Object;
	ULONG ob_MethodID;
	IPTR ob_UserData;
};

#define ob_Beacon ob_Order.obo_Beacon
#define ob_HttpGet ob_Order.obo_HttpGet
#define ob_HttpPost ob_Order.obo_HttpPost
#define ob_FtpPut ob_Order.obo_FtpPut

VOID SlaveProcStart(VOID);

#endif /* __SLAVE_H__ */

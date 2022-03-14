/*
 * Copyright (c) 2012-2022 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef __FTP_H__
#define __FTP_H__

#include <exec/ports.h>

#define FTP_ERROR_LIBRARY (-1)
#define FTP_ERROR_CONNECT (-2)
#define FTP_ERROR_LOGIN   (-3)
#define FTP_ERROR_PASSIVE (-4)
#define FTP_ERROR_SEND    (-5)

struct FTPPut
{
	struct Message ftpp_Msg;
	STRPTR ftpp_Host;
	SHORT  ftpp_Port;
	STRPTR ftpp_RemotePath;
	STRPTR ftpp_LocalPath;
	BOOL ftpp_Passive;
	STRPTR ftpp_User;
	STRPTR ftpp_Password;
	LONG ftpp_Error;
	struct Order *ftpp_Order;
};

VOID FTPPutRequest(VOID);

#endif /* __FTP_H__ */

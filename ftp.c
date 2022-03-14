/*
 * Copyright (c) 2012-2022 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/ftp.h>
#include "globaldefines.h"
#include "ftp.h"

VOID FTPPutRequest(VOID)
{
	struct FTPPut *smsg;
	ENTER();

	if(NewGetTaskAttrs(NULL, &smsg, sizeof(struct FTPPut *), TASKINFOTYPE_STARTUPMSG, TAG_DONE) && smsg)
	{
		struct Library *FTPBase;

		if((FTPBase = OpenLibrary("ftp.library", 52)))
		{
			struct FTPHandler *ftph;
			struct TagItem contags[] =
			{
				{FTP_TAG_LOGMSG, (IPTR)NULL},
				{FTP_TAG_TIMEOUT, 10},
				{TAG_END, 0}
			};

			if((ftph = FTPConnect(smsg->ftpp_Host, smsg->ftpp_Port, contags)))
			{
				if(ftph->Error == 0)
				{
					if(FTPLogin(ftph, smsg->ftpp_User, smsg->ftpp_Password) == 0)
					{
						if(smsg->ftpp_Passive)
						{
							struct TagItem passive[] =
							{
								{FTP_TAG_CONNMODE, FTPLIB_PASSIVE},
								{TAG_END, 0}
							};

							if(FTPSet(ftph, passive) != 0)
								smsg->ftpp_Error = FTP_ERROR_PASSIVE;
						}

						if(FTPSend(ftph, smsg->ftpp_LocalPath, smsg->ftpp_RemotePath, FTPLIB_BINARY, 0) == 0)
							smsg->ftpp_Error = 0;
						else
							smsg->ftpp_Error = FTP_ERROR_SEND;
					}
					else
						smsg->ftpp_Error = FTP_ERROR_LOGIN;
				}
				else
					smsg->ftpp_Error = FTP_ERROR_CONNECT;

				FTPQuit(ftph);
			}
			else
				smsg->ftpp_Error = FTP_ERROR_CONNECT;
			CloseLibrary(FTPBase);
		}
		else
			smsg->ftpp_Error = FTP_ERROR_LIBRARY;
	}
	LEAVE();
}

/*
 * Copyright (c) 2012-2018 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <proto/exec.h>
#include <proto/multimedia.h>
#include <proto/intuition.h>
#include <proto/multimedia.h>
#include <classes/multimedia/video.h>
#include <classes/multimedia/streams.h>
#include <dos/dostags.h>
#include <libvstring.h>
#define __NOLIBBASE__
#define USE_INLINE_STDARG
#include <proto/magicbeacon.h>
#undef USE_INLINE_STDARG
#include <proto/dos.h>
#include "support.h"
#include "globaldefines.h"
#include "http.h"
#include "slave.h"

#ifdef __DEBUG__
#undef tprintf
#define tprintf(template, ...) KPrintF((CONST_STRPTR)"KwaSlave" " " __FILE__ " %d: " template, __LINE__ , ##__VA_ARGS__)
#endif /* __DEBUG__ */

struct SlaveProcData
{
	struct Library *DOSBase;
	struct Library *MagicBeaconBase;

	struct MsgPort *slave_port;

	struct MsgPort *get_port;
	struct MsgPort *post_port;
	struct MsgPort *ftp_port;

	APTR magic_beacon;

	ULONG notifications_no;
	ULONG gets_no;
	ULONG posts_no;
	ULONG ftp_no;
};

#define DOSBase d->DOSBase
#define MagicBeaconBase d->MagicBeaconBase

VOID DoBeaconOrder(struct SlaveProcData *d, struct MagicBeaconNotificationMessage *mbnm)
{
	IPTR err = MagicBeacon_BeaconSendTags(d->magic_beacon,
		MAGICBEACON_NOTIFICATIONNAME, (IPTR)mbnm->mbnm_NotificationName,
		MAGICBEACON_MESSAGE, (IPTR)mbnm->mbnm_NotificationMessage,
		MAGICBEACON_USERDATA, (IPTR)mbnm->mbnm_UserData,
		MAGICBEACON_WAITFORRESULT, mbnm->mbnm_WaitForResult,
	TAG_END);

	if (mbnm->mbnm_WaitForResult != TRUE || err != MAGICBEACON_ERROR_NOERROR)
		ReplyMsg((struct Message*)mbnm->mbnm_UserData);

	if (err != MAGICBEACON_ERROR_NOERROR)
		tprintf("magicbeacon retured error: %d\n", err);
}


VOID DoHttpGetOrder(struct SlaveProcData *d, struct HttpGet *hg)
{
	static UBYTE buffer[50];
	struct Process *proc;

	hg->hg_Msg.mn_Length = sizeof(struct HttpGet);
	hg->hg_Msg.mn_Node.ln_Name = (STRPTR)NT_MESSAGE;
	hg->hg_Msg.mn_ReplyPort = d->get_port;

	hg->hg_DataLen = 0;
	hg->hg_Data = NULL;

	FmtNPut(buffer, "KwaKwa Http Get Request %ld", sizeof(buffer), d->gets_no);

	if((proc = CreateNewProcTags(NP_CodeType, CODETYPE_PPC,
		NP_Entry, HttpGetRequest,
		NP_StartupMsg, (IPTR)hg,
		NP_Name, (IPTR) buffer,
	TAG_END)))
	{
		d->gets_no++;
	}
}

VOID DoHttpPostOrder(struct SlaveProcData *d, struct HttpPost *hp)
{
	static UBYTE buffer[50];
	struct Process *proc;

	hp->hp_Msg.mn_Length = sizeof(struct HttpPost);
	hp->hp_Msg.mn_Node.ln_Name = (STRPTR)NT_MESSAGE;
	hp->hp_Msg.mn_ReplyPort = d->post_port;

	hp->hp_DataLen = 0;
	hp->hp_Data = NULL;

	FmtNPut(buffer, "KwaKwa Http Post Request %ld", sizeof(buffer), d->posts_no);

	if((proc = CreateNewProcTags(NP_CodeType, CODETYPE_PPC,
		NP_Entry, HttpPostRequest,
		NP_StartupMsg, (IPTR)hp,
		NP_Name, (IPTR) buffer,
	TAG_END)))
	{
		d->posts_no++;
	}
}

VOID DoFtpPutOrder(struct SlaveProcData *d, struct FTPPut *fp)
{
	static UBYTE buffer[50];
	struct Process *proc;
	ENTER();

	fp->ftpp_Msg.mn_Length = sizeof(struct FTPPut);
	fp->ftpp_Msg.mn_Node.ln_Name = (STRPTR)NT_MESSAGE;
	fp->ftpp_Msg.mn_ReplyPort = d->ftp_port;

	FmtNPut(buffer, "KwaKwa FTP STORE Request %ld", sizeof(buffer), d->ftp_no);

	if((proc = CreateNewProcTags(NP_CodeType, CODETYPE_PPC,
		NP_Entry, FTPPutRequest,
		NP_StartupMsg, (IPTR)fp,
		NP_Name, (IPTR) buffer,
	TAG_END)))
	{
		d->ftp_no++;
	}
	LEAVE();
}

VOID SlaveProcMainLoop(struct SlaveProcData *d)
{
	ULONG order_mask = (1UL << d->slave_port->mp_SigBit);
	ULONG get_mask = (1UL << d->get_port->mp_SigBit);
	ULONG post_mask = (1UL << d->get_port->mp_SigBit);
	ULONG ftp_mask = (1UL << d->ftp_port->mp_SigBit);
	ULONG beacon_mask = MagicBeacon_GetAttr(d->magic_beacon, NULL, MAGICBEACON_APPLICATIONSIGNALMASK);
	ULONG signals;
	BOOL running = TRUE;

	while(running)
	{
		signals = Wait(order_mask | beacon_mask | get_mask | post_mask | ftp_mask | SIGBREAKF_CTRL_C);

		if(signals & order_mask)
		{
			struct Order *nor;

			tprintf("new order recived!\n");

			while((nor = (struct Order*)GetMsg(d->slave_port)))
			{
				switch(nor->ob_Type)
				{
					case ORDER_TYPE_BEACON:
						DoBeaconOrder(d, &nor->ob_Beacon);
					break;

					case ORDER_TYPE_HTTP_GET:
						DoHttpGetOrder(d, &nor->ob_HttpGet);
					break;

					case ORDER_TYPE_HTTP_POST:
						DoHttpPostOrder(d, &nor->ob_HttpPost);
					break;

					case ORDER_TYPE_FTP_PUT:
						DoFtpPutOrder(d, &nor->ob_FtpPut);
					break;
				}
			}
		}

		if(signals & get_mask)
		{
			struct HttpGet *hg;

			while((hg = (struct HttpGet*)GetMsg(d->get_port)))
			{
				d->gets_no--;
				ReplyMsg((struct Message*)hg->hg_Order);
			}
		}

		if(signals & post_mask)
		{
			struct HttpPost *hp;

			while((hp = (struct HttpPost*)GetMsg(d->post_port)))
			{
				d->posts_no--;
				ReplyMsg((struct Message*)hp->hp_Order);
			}
		}

		if(signals & ftp_mask)
		{
			struct FTPPut *ftpp;

			while((ftpp = (struct FTPPut*)GetMsg(d->ftp_port)))
			{
				d->ftp_no--;
				ReplyMsg((struct Message*)ftpp->ftpp_Order);
			}
		}

		if(signals & beacon_mask)
		{
			APTR beacon = MagicBeacon_BeaconObtain(d->magic_beacon);
			if (beacon)
			{
				struct Order *ord;

				ord = (struct Order*)MagicBeacon_GetAttr(d->magic_beacon, beacon, MAGICBEACON_USERDATA);
				ord->ob_Beacon.mbnm_Result = MagicBeacon_GetAttr(d->magic_beacon, beacon, MAGICBEACON_RESULT) == NOTIFICATIONRESULT_CONFIRMED ? 1 : 0;

				ReplyMsg((struct Message*)ord);
				MagicBeacon_BeaconDispose(d->magic_beacon, beacon);
			}
		}

		if(signals & SIGBREAKF_CTRL_C)
		{
			tprintf("recived CTRL_C!\n");
			running = FALSE;
		}
	}

}

VOID SlaveProcStart(VOID)
{
	struct SlaveProcData data = {0};
	struct SlaveProcData *d = &data;

	if((DOSBase = OpenLibrary("dos.library", 0)))
	{
		if((MagicBeaconBase = OpenLibrary("magicbeacon.library", 1)))
		{
			d->magic_beacon = MagicBeacon_ApplicationRegisterTags(
				MAGICBEACON_APPLICATIONNAME, (IPTR)APP_BASE,
				MAGICBEACON_APPLICATIONSIGNALNUMBER, -1,
			TAG_DONE);
			if (d->magic_beacon)
			{
				if(NewGetTaskAttrs(NULL, &d->slave_port, sizeof(struct MsgPort *), TASKINFOTYPE_TASKMSGPORT, TAG_DONE) && d->slave_port)
				{
					if((d->get_port = CreateMsgPort()))
					{
						if((d->post_port = CreateMsgPort()))
						{
							if((d->ftp_port = CreateMsgPort()))
							{
								SlaveProcMainLoop(d);

								while(d->posts_no)
								{
									struct HttpPost *hp;

									WaitPort(d->post_port);

									while((hp = (struct HttpPost*)GetMsg(d->post_port)))
									{
										d->posts_no--;
										ReplyMsg((struct Message*)hp->hp_Order);
									}
								}

								while(d->gets_no)
								{
									struct HttpGet *hg;

									WaitPort(d->get_port);

									while((hg = (struct HttpGet*)GetMsg(d->get_port)))
									{
										d->gets_no--;
										ReplyMsg((struct Message*)hg->hg_Order);
									}
								}

								while(d->ftp_no)
								{
									struct FTPPut *ftpp;

									WaitPort(d->ftp_port);

									while((ftpp = (struct FTPPut*)GetMsg(d->ftp_port)))
									{
										d->ftp_no--;
										ReplyMsg((struct Message*)ftpp->ftpp_Order);
									}
								}
								DeleteMsgPort(d->ftp_port);
							}
							DeleteMsgPort(d->post_port);
						}
						DeleteMsgPort(d->get_port);
					}
				}
				MagicBeacon_ApplicationUnregister(d->magic_beacon);
			}
			CloseLibrary(MagicBeaconBase);
		}
		CloseLibrary(DOSBase);
	}
}

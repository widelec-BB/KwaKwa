/*
 * Copyright (c) 2012-2022 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <exec/ports.h>
#include <dos/dostags.h>
#include <dos/dosextens.h>
#include <libvstring.h>
#include "globaldefines.h"
#include "slaveprocess.h"
#include "support.h"
#include "slave.h"

Class *SlaveProcessClass;

static IPTR SlaveProcessDispatcher(VOID);
const struct EmulLibEntry SlaveProcessGate = {TRAP_LIB, 0, (VOID(*)(VOID))SlaveProcessDispatcher};

struct SPP_SendNotification {ULONG MethodID; STRPTR notification_name; STRPTR message; ULONG wait_for_result; Object *obj; ULONG method; IPTR user_data;};
struct SPP_FreeNotification {ULONG MethodID; struct MagicBeaconNotificationMessage *mbnm;};
struct SPP_SendHttpGet {ULONG MethodID; STRPTR url; STRPTR user_agent; Object *obj; ULONG method; IPTR user_data;};
struct SPP_FreeHttpGet {ULONG MethodID; struct HttpGet *hg;};
struct SPP_SendHttpPost {ULONG MethodID; STRPTR url; STRPTR user_agent; struct TagItem *data; ULONG items_no; Object *obj; ULONG method; IPTR user_data;};
struct SPP_FreeHttpPost {ULONG MethodID; struct HttpPost *hp;};
struct SPP_SendFtpPut {ULONG MethodID; STRPTR host; ULONG port; STRPTR rpath; STRPTR lpath; ULONG passive; STRPTR user; STRPTR password; Object *obj; ULONG method; IPTR user_data;};
struct SPP_FreeFtpPut {ULONG MethodID; struct FTPPut *ftpp;};

struct SlaveProcessData
{
	struct MsgPort *reply_port;
	struct MsgPort *slave_port;
	struct Process *slave_proc;

	struct Message startup_msg;

	ULONG notifications_no;
};

Class *CreateSlaveProcessClass(VOID)
{
	Class *cl;

	if ((cl = MakeClass(NULL, ROOTCLASS, NULL, sizeof(struct SlaveProcessData), 0)))
	{
		cl->cl_Dispatcher.h_Entry = (HOOKFUNC)&SlaveProcessGate;
		cl->cl_UserData = NULL;
	}

	SlaveProcessClass = cl;
	return cl;
}

VOID DeleteSlaveProcessClass(VOID)
{
	if (SlaveProcessClass) FreeClass(SlaveProcessClass);
}


static IPTR SlaveProcessNew(Class *cl, Object *obj, struct opSet *msg)
{
	Object *newobj = NULL;

	if ((obj = (Object*)DoSuperMethodA(cl, obj, (Msg)msg)))
	{
		struct SlaveProcessData *d = (struct SlaveProcessData*)INST_DATA(cl, obj);

		if((d->reply_port = CreateMsgPort()))
		{
			newobj = obj;
		}
	}

	if(!newobj)
		CoerceMethod(cl, obj, OM_DISPOSE);

	return (IPTR)newobj;
}

static IPTR SlaveProcessDispose(Class *cl, Object *obj, Msg msg)
{
	struct SlaveProcessData *d = INST_DATA(cl, obj);

	if(d->reply_port)
	{
		DeleteMsgPort(d->reply_port);
	}

	return DoSuperMethodA(cl, obj, msg);
}

static IPTR SlaveProcessGet(Class *cl, Object *obj, struct opGet *msg)
{
	struct SlaveProcessData *d = INST_DATA(cl, obj);
	int rv = FALSE;

	switch (msg->opg_AttrID)
	{
		case SPA_SigBit:
			*msg->opg_Storage = (IPTR)d->reply_port->mp_SigBit;
		return TRUE;

		default: rv = (DoSuperMethodA(cl, obj, (Msg)msg));
	}

	return rv;
}

static IPTR SlaveProcessStartProcess(Class *cl, Object *obj)
{
	struct SlaveProcessData *d = INST_DATA(cl, obj);

	if(d->reply_port)
	{
		d->startup_msg.mn_Length = sizeof(d->startup_msg);
		d->startup_msg.mn_Node.ln_Type = NT_MESSAGE;
		d->startup_msg.mn_ReplyPort = d->reply_port;

		if((d->slave_proc = CreateNewProcTags(NP_CodeType, CODETYPE_PPC,
			NP_Entry, SlaveProcStart,
			NP_StartupMsg, (IPTR)&d->startup_msg,
			NP_TaskMsgPort, (IPTR)&d->slave_port,
			NP_Name, (IPTR) "KwaKwa Slave Process",
		TAG_END)))
		{
			return (IPTR)TRUE;
		}
	}

	return (IPTR)FALSE;
}

static IPTR SlaveProcessKillProcess(Class *cl, Object *obj)
{
	struct SlaveProcessData *d = INST_DATA(cl, obj);

	if(d->slave_proc)
	{
		struct Order *ord;

		Signal(&d->slave_proc->pr_Task, SIGBREAKF_CTRL_C);

		WaitPort(d->reply_port);

		while((ord = (struct Order*)GetMsg(d->reply_port)) != (struct Order*)&d->startup_msg)
		{
			if(ord)
			{
				switch(ord->ob_Type)
				{
					case ORDER_TYPE_BEACON:
						if(ord->ob_Object)
							DoMethod(ord->ob_Object, ord->ob_MethodID, NOTIFICATIONRESULT_IGNORED, ord->ob_UserData);
						DoMethod(obj, SPM_FreeNotification, &ord->ob_Beacon);
					break;

					case ORDER_TYPE_HTTP_GET:
						if(ord->ob_Object)
							DoMethod(ord->ob_Object, ord->ob_MethodID, ord->ob_HttpGet.hg_DataLen, ord->ob_HttpGet.hg_Data, ord->ob_UserData);
						DoMethod(obj, SPM_FreeHttpGet, &ord->ob_HttpGet);
					break;

					case ORDER_TYPE_HTTP_POST:
						if(ord->ob_Object)
							DoMethod(ord->ob_Object, ord->ob_MethodID, ord->ob_HttpPost.hp_DataLen, ord->ob_HttpPost.hp_Data, ord->ob_UserData);
						DoMethod(obj, SPM_FreeHttpPost, &ord->ob_HttpPost);
					break;
				}
				FreeMem(ord, sizeof(struct Order));
			}

			WaitPort(d->reply_port);
		}
		d->slave_proc = NULL; /* proc closed, ptr no longer valid */
	}

	return (IPTR)0;
}

static IPTR SlaveProcessSendNotification(Class *cl, Object *obj, struct SPP_SendNotification *msg)
{
	struct SlaveProcessData *d = INST_DATA(cl, obj);
	struct Order *nord;

	if(!d->slave_proc)
		if(!DoMethod(obj, SPM_StartProcess))
			return (IPTR)0;

	if((nord = AllocMem(sizeof(struct Order), MEMF_PUBLIC)))
	{
		nord->ob_Msg.mn_Length = sizeof(struct Order);
		nord->ob_Msg.mn_Node.ln_Name = (STRPTR)NT_MESSAGE;
		nord->ob_Msg.mn_ReplyPort = d->reply_port;
		nord->ob_Type = ORDER_TYPE_BEACON;
		nord->ob_Beacon.mbnm_NotificationName = StrNewPublic(msg->notification_name);
		nord->ob_Beacon.mbnm_NotificationMessage = StrNewPublic(msg->message);
		nord->ob_Beacon.mbnm_WaitForResult = msg->wait_for_result ? TRUE : FALSE;
		nord->ob_Beacon.mbnm_UserData = nord;
		nord->ob_Object = msg->obj;
		nord->ob_MethodID = msg->method;
		nord->ob_UserData = msg->user_data;

		PutMsg(d->slave_port, (struct Message*)nord);
	}

	return (IPTR)0;
}

static IPTR SlaveProcessFreeNotification(Class *cl, Object *obj, struct SPP_FreeNotification *msg)
{
	ENTER();
	if(msg->mbnm)
	{
		if(msg->mbnm->mbnm_NotificationName)
			StrFree(msg->mbnm->mbnm_NotificationName);

		if(msg->mbnm->mbnm_NotificationMessage)
			StrFree(msg->mbnm->mbnm_NotificationMessage);
	}
	LEAVE();
	return (IPTR)0;
}

static IPTR SlaveProcessSignalMethod(Class *cl, Object *obj)
{
	struct SlaveProcessData *d = INST_DATA(cl, obj);
	struct Order *o;
	ENTER();

	while((o = (struct Order*)GetMsg(d->reply_port)))
	{
		if((struct Message*)o == (struct Message*)&d->startup_msg)
		{
			d->slave_proc = NULL;
			continue;
		}

		switch(o->ob_Type)
		{
			case ORDER_TYPE_BEACON:
				if(o->ob_Object && o->ob_MethodID)
					DoMethod(o->ob_Object, o->ob_MethodID, (ULONG)o->ob_Beacon.mbnm_Result, o->ob_UserData);
				DoMethod(obj, SPM_FreeNotification, &o->ob_Beacon);
			break;

			case ORDER_TYPE_HTTP_GET:
				if(o->ob_Object && o->ob_MethodID)
					DoMethod(o->ob_Object, o->ob_MethodID, o->ob_HttpGet.hg_DataLen, o->ob_HttpGet.hg_Data, o->ob_UserData);
				DoMethod(obj, SPM_FreeHttpGet, &o->ob_HttpGet);
			break;

			case ORDER_TYPE_HTTP_POST:
				if(o->ob_Object && o->ob_MethodID)
					DoMethod(o->ob_Object, o->ob_MethodID, o->ob_HttpPost.hp_DataLen, o->ob_HttpPost.hp_Data, o->ob_UserData);
				DoMethod(obj, SPM_FreeHttpPost, &o->ob_HttpPost);
			break;

			case ORDER_TYPE_FTP_PUT:
				if(o->ob_Object && o->ob_MethodID)
					DoMethod(o->ob_Object, o->ob_MethodID, o->ob_FtpPut.ftpp_Error, o->ob_UserData);
				DoMethod(obj, SPM_FreeFtpPut, &o->ob_FtpPut);
			break;
		}
		FreeMem(o, sizeof(struct Order));
	}

	LEAVE();
	return (IPTR)0;
}

static IPTR SlaveProcessSendHttpGet(Class *cl, Object *obj, struct SPP_SendHttpGet *msg)
{
	struct SlaveProcessData *d = INST_DATA(cl, obj);
	struct Order *nord;

	if(!d->slave_proc)
		if(!DoMethod(obj, SPM_StartProcess))
			return (IPTR)0;

	if((nord = AllocMem(sizeof(struct Order), MEMF_PUBLIC)))
	{
		nord->ob_Msg.mn_Length = sizeof(struct Order);
		nord->ob_Msg.mn_Node.ln_Name = (STRPTR)NT_MESSAGE;
		nord->ob_Msg.mn_ReplyPort = d->reply_port;
		nord->ob_Type = ORDER_TYPE_HTTP_GET;
		nord->ob_HttpGet.hg_Url = StrNewPublic(msg->url);
		nord->ob_HttpGet.hg_UserAgent = StrNewPublic(msg->user_agent);
		nord->ob_HttpGet.hg_Order = nord;
		nord->ob_Object = msg->obj;
		nord->ob_MethodID = msg->method;
		nord->ob_UserData = msg->user_data;

		PutMsg(d->slave_port, (struct Message*)nord);
	}

	return (IPTR)0;
}

static IPTR SlaveProcessFreeHttpGet(Class *cl, Object *obj, struct SPP_FreeHttpGet *msg)
{
	if(msg->hg)
	{
		if(msg->hg->hg_Url)
			StrFree(msg->hg->hg_Url);

		if(msg->hg->hg_UserAgent)
			StrFree(msg->hg->hg_UserAgent);

		if(msg->hg->hg_Data)
			FreeVec(msg->hg->hg_Data);
	}
	return (IPTR)0;
}

static IPTR SlaveProcessSendHttpPost(Class *cl, Object *obj, struct SPP_SendHttpPost *msg)
{
	struct SlaveProcessData *d = INST_DATA(cl, obj);
	struct TagItem *tags;

	if(!d->slave_proc)
		if(!DoMethod(obj, SPM_StartProcess))
			return (IPTR)0;

	if((tags = AllocVec(sizeof(struct TagItem) * msg->items_no, MEMF_PUBLIC)))
	{
		struct Order *nord;

		CopyMemQuick(msg->data, tags, sizeof(struct TagItem) * msg->items_no);

		if((nord = AllocMem(sizeof(struct Order), MEMF_PUBLIC)))
		{
			nord->ob_Msg.mn_Length = sizeof(struct Order);
			nord->ob_Msg.mn_Node.ln_Name = (STRPTR)NT_MESSAGE;
			nord->ob_Msg.mn_ReplyPort = d->reply_port;
			nord->ob_Type = ORDER_TYPE_HTTP_POST;
			nord->ob_HttpPost.hp_Url = StrNewPublic(msg->url);
			nord->ob_HttpPost.hp_UserAgent = StrNewPublic(msg->user_agent);
			nord->ob_HttpPost.hp_Post = tags;
			nord->ob_HttpPost.hp_Order = nord;
			nord->ob_Object = msg->obj;
			nord->ob_MethodID = msg->method;
			nord->ob_UserData = msg->user_data;

			PutMsg(d->slave_port, (struct Message*)nord);
		}
	}

	return (IPTR)0;
}

static IPTR SlaveProcessFreeHttpPost(Class *cl, Object *obj, struct SPP_FreeHttpPost *msg)
{
	if(msg->hp)
	{
		if(msg->hp->hp_Url)
			StrFree(msg->hp->hp_Url);

		if(msg->hp->hp_UserAgent)
			StrFree(msg->hp->hp_UserAgent);

		if(msg->hp->hp_Post)
			FreeVec(msg->hp->hp_Post);

		if(msg->hp->hp_Data)
			FreeVec(msg->hp->hp_Data);
	}
	return (IPTR)0;
}

static IPTR SlaveProcessSendFtpPut(Class *cl, Object *obj, struct SPP_SendFtpPut *msg)
{
	struct SlaveProcessData *d = INST_DATA(cl, obj);
	struct Order *nord;
	ENTER();

	if(!d->slave_proc)
		if(!DoMethod(obj, SPM_StartProcess))
			return (IPTR)0;

	if((nord = AllocMem(sizeof(struct Order), MEMF_PUBLIC)))
	{
		nord->ob_Msg.mn_Length = sizeof(struct Order);
		nord->ob_Msg.mn_Node.ln_Name = (STRPTR)NT_MESSAGE;
		nord->ob_Msg.mn_ReplyPort = d->reply_port;
		nord->ob_Type = ORDER_TYPE_FTP_PUT;

		nord->ob_FtpPut.ftpp_Host = StrNewPublic(msg->host);
		nord->ob_FtpPut.ftpp_Port = (SHORT)msg->port;
		nord->ob_FtpPut.ftpp_RemotePath = StrNewPublic(msg->rpath);
		nord->ob_FtpPut.ftpp_LocalPath = StrNewPublic(msg->lpath);
		nord->ob_FtpPut.ftpp_Passive = (BOOL)msg->passive;
		nord->ob_FtpPut.ftpp_User = StrNewPublic(msg->user);
		nord->ob_FtpPut.ftpp_Password = StrNewPublic(msg->password);
		nord->ob_FtpPut.ftpp_Error = 0;
		nord->ob_FtpPut.ftpp_Order = nord;

		nord->ob_Object = msg->obj;
		nord->ob_MethodID = msg->method;
		nord->ob_UserData = msg->user_data;

		PutMsg(d->slave_port, (struct Message*)nord);
	}

	LEAVE();
	return (IPTR)0;
}

static IPTR SlaveProcessFreeFtpPut(Class *cl, Object *obj, struct SPP_FreeFtpPut *msg)
{
	if(msg->ftpp)
	{
		if(msg->ftpp->ftpp_Host)
			StrFree(msg->ftpp->ftpp_Host);

		if(msg->ftpp->ftpp_RemotePath)
			StrFree(msg->ftpp->ftpp_RemotePath);

		if(msg->ftpp->ftpp_LocalPath)
			StrFree(msg->ftpp->ftpp_LocalPath);

		if(msg->ftpp->ftpp_User)
			StrFree(msg->ftpp->ftpp_User);

		if(msg->ftpp->ftpp_Password)
			StrFree(msg->ftpp->ftpp_Password);
	}

	return (IPTR)0;
}

static IPTR SlaveProcessDispatcher(VOID)
{
	Class *cl = (Class*)REG_A0;
	Object *obj = (Object*)REG_A2;
	Msg msg = (Msg)REG_A1;

	switch (msg->MethodID)
	{
		case OM_NEW:  return (SlaveProcessNew(cl, obj, (struct opSet*)msg));
		case OM_DISPOSE:  return (SlaveProcessDispose(cl, obj, msg));
		case OM_GET:  return (SlaveProcessGet(cl, obj, (struct opGet*)msg));
		case SPM_StartProcess: return(SlaveProcessStartProcess(cl, obj));
		case SPM_KillProcess: return(SlaveProcessKillProcess(cl, obj));
		case SPM_SignalMethod: return(SlaveProcessSignalMethod(cl, obj));
		case SPM_SendNotification: return(SlaveProcessSendNotification(cl, obj, (struct SPP_SendNotification*)msg));
		case SPM_FreeNotification: return(SlaveProcessFreeNotification(cl, obj, (struct SPP_FreeNotification*)msg));
		case SPM_SendHttpGet: return(SlaveProcessSendHttpGet(cl, obj, (struct SPP_SendHttpGet*)msg));
		case SPM_FreeHttpGet: return(SlaveProcessFreeHttpGet(cl, obj, (struct SPP_FreeHttpGet*)msg));
		case SPM_SendHttpPost: return(SlaveProcessSendHttpPost(cl, obj, (struct SPP_SendHttpPost*)msg));
		case SPM_FreeHttpPost: return(SlaveProcessFreeHttpPost(cl, obj, (struct SPP_FreeHttpPost*)msg));
		case SPM_SendFtpPut: return(SlaveProcessSendFtpPut(cl, obj, (struct SPP_SendFtpPut*)msg));
		case SPM_FreeFtpPut: return(SlaveProcessFreeFtpPut(cl, obj, (struct SPP_FreeFtpPut*)msg));
		default:  return (DoSuperMethodA(cl, obj, msg));
	}
}


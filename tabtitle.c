/*
 * Copyright (c) 2012-2022 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <proto/exec.h>
#include <proto/dos.h>
#include <libvstring.h>

#include "globaldefines.h"
#include "support.h"
#include "tabtitle.h"
#include "prefswindow.h"

#include "kwakwa_api/defs.h"

struct MUI_CustomClass *TabTitleClass;
static IPTR TabTitleDispatcher(VOID);
const struct EmulLibEntry TabTitleGate = {TRAP_LIB, 0, (VOID(*)(VOID))TabTitleDispatcher};

struct TabTitleData
{
	STRPTR ttd_ContactName;
	BOOL ttd_Unread;
	ULONG ttd_Status;

	BOOL ttd_ShowStatusImage;
};

struct MUI_CustomClass *CreateTabTitleClass(VOID)
{
	struct MUI_CustomClass *cl;

	cl = MUI_CreateCustomClass(NULL, MUIC_Text, NULL, sizeof(struct TabTitleData), (APTR)&TabTitleGate);
	TabTitleClass = cl;
	return cl;
}

VOID DeleteTabTitleClass(VOID)
{
	if (TabTitleClass) MUI_DeleteCustomClass(TabTitleClass);
}

static STRPTR createTextContent(STRPTR name, ULONG status)
{
	STRPTR image, res;
	LONG len;

	if(KWA_S_AVAIL(status))
		image = "\33I[4:PROGDIR:gfx/available.mbr]";
	else if(KWA_S_BUSY(status))
		image = "\33I[4:PROGDIR:gfx/away.mbr]";
	else if(KWA_S_FFC(status))
		image =  "\33I[4:PROGDIR:gfx/ffc.mbr]";
	else if(KWA_S_DND(status))
		image = "\33I[4:PROGDIR:gfx/dnd.mbr]";
	else if(KWA_S_BLOCKED(status))
		image = "\33I[4:PROGDIR:gfx/blocked.mbr]";
	else if(KWA_S_INVISIBLE(status))
		image = "\33I[4:PROGDIR:gfx/invisible.mbr]";
	else
		image = "\33I[4:PROGDIR:gfx/unavailable.mbr]";

	len = StrLen(image) + StrLen(name) + 2 /* 2 = space in the middle and 0x00 at end */;
	res = (STRPTR)AllocVec(len, MEMF_PUBLIC);
	if(res)
	{
		FmtPut(res, "%s %s", image, name);
	}

	return res;
}

static IPTR TabTitleNew(Class *cl, Object *obj, struct opSet *msg)
{
	struct TagItem *tag = 0, *tagptr = msg->ops_AttrList;
	ULONG status = KWA_STATUS_FRESH;
	BOOL show_status_image = FALSE, unread = FALSE;
	STRPTR contact_name = NULL;
	ENTER();

	while((tag = NextTagItem(&tagptr)))
	{
		switch(tag->ti_Tag)
		{
			case TTA_Unread:
				unread = (BOOL)tag->ti_Data;
			break;

			case TTA_ContactName:
				contact_name = (STRPTR)tag->ti_Data;
			break;

			case TTA_Status:
				status = (ULONG)tag->ti_Data;
			break;

			case TTA_ShowStatusImage:
				show_status_image = (BOOL)tag->ti_Data;
			break;
		}
	}

	if(contact_name)
	{
		if(show_status_image)
		{
			STRPTR content = createTextContent(contact_name, status);
			if(content)
			{
				obj = (Object*)DoSuperNew(cl, obj,
					MUIA_Text_Contents, (IPTR)content,
					MUIA_Text_PreParse, (IPTR)(unread ? "\33b" : NULL),
				TAG_MORE, (IPTR)msg->ops_AttrList);
				FreeVec(content);
			}
		}
		else
		{
			obj = (Object*)DoSuperNew(cl, obj,
				MUIA_Text_Contents, (IPTR)contact_name,
				MUIA_Text_PreParse, (IPTR)(unread ? "\33b" : NULL),
			TAG_MORE, (IPTR)msg->ops_AttrList);
		}

		if(obj)
		{
			struct TabTitleData *d = INST_DATA(cl, obj);

			d->ttd_ContactName = StrNew(contact_name);
			d->ttd_Unread = unread;
			d->ttd_Status = status;
			d->ttd_ShowStatusImage = show_status_image;

			DoMethod(prefs_object(USD_PREFS_TW_TABTITLE_IMAGE_ONOFF), MUIM_Notify, MUIA_Selected, MUIV_EveryTime, (IPTR)obj, 3,
			 MUIM_Set, TTA_ShowStatusImage, MUIV_TriggerValue);

			LEAVE();
			return (IPTR)obj;
		}
	}

	CoerceMethod(cl, obj, OM_DISPOSE);
	LEAVE();
	return (IPTR)NULL;
}

static IPTR TabTitleDispose(Class *cl, Object *obj, Msg msg)
{
	struct TabTitleData *d = INST_DATA(cl, obj);

	if(d->ttd_ContactName)
		StrFree(d->ttd_ContactName);

	return DoSuperMethodA(cl, obj, msg);
}

static IPTR TabTitleSet(Class *cl, Object *obj, struct opSet *msg)
{
	struct TabTitleData *d = INST_DATA(cl, obj);
	int tagcount = 0;
	struct TagItem *tag = 0, *tagptr = msg->ops_AttrList;
	BOOL relayout = FALSE, change_contents = FALSE;

	while((tag = NextTagItem(&tagptr)))
	{
		switch(tag->ti_Tag)
		{
			case TTA_Unread:
				nnset(obj, MUIA_Text_PreParse, (IPTR)((BOOL)tag->ti_Data ? "\33b" : NULL));
				relayout = TRUE;
				tagcount++;
			break;

			case TTA_Status:
				d->ttd_Status = (ULONG)tag->ti_Data;
				change_contents = TRUE;
				tagcount++;
			break;

			case TTA_ContactName:
				d->ttd_ContactName = StrNew((STRPTR)tag->ti_Data);
				change_contents = TRUE;
				tagcount++;
			break;

			case TTA_ShowStatusImage:
				d->ttd_ShowStatusImage = (BOOL)tag->ti_Data;
				change_contents = TRUE;
				tagcount++;
			break;
		}
	}

	tagcount += DoSuperMethodA(cl, obj, (Msg)msg);

	if(change_contents)
	{
		if(d->ttd_ShowStatusImage)
		{
			STRPTR content = createTextContent(d->ttd_ContactName, d->ttd_Status);
			if(content)
			{
				nnset(obj, MUIA_Text_Contents, content);
				relayout = TRUE;
				FreeVec(content);
			}
		}
		else
		{
			nnset(obj, MUIA_Text_Contents, d->ttd_ContactName);
		}
	}

	if(relayout)
	{
		Object *parent = (Object*)xget(obj, MUIA_Parent);

		DoMethod(parent, MUIM_Group_InitChange);
		DoMethod(parent, MUIM_Group_ExitChange);
	}

	return tagcount;
}

static IPTR TabTitleDispatcher(VOID)
{
	Class *cl = (Class*)REG_A0;
	Object *obj = (Object*)REG_A2;
	Msg msg = (Msg)REG_A1;

	switch (msg->MethodID)
	{
		case OM_NEW: return(TabTitleNew(cl, obj, (struct opSet*)msg));
		case OM_DISPOSE: return(TabTitleDispose(cl, obj, msg));
		case OM_SET: return(TabTitleSet(cl, obj, (struct opSet*)msg));
		default: return(DoSuperMethodA(cl, obj, msg));
	}
}

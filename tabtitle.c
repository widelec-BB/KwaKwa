/*
 * Copyright (c) 2012-2018 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <proto/exec.h>
#include <proto/dos.h>
#include <libvstring.h>

#include "globaldefines.h"
#include "support.h"
#include "tabtitle.h"

struct MUI_CustomClass *TabTitleClass;
static IPTR TabTitleDispatcher(VOID);
const struct EmulLibEntry TabTitleGate = {TRAP_LIB, 0, (VOID(*)(VOID))TabTitleDispatcher};

struct TabTitleData
{
	BOOL ttd_Unread;
	UBYTE ttd_PreParse[50];
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

static IPTR TabTitleSet(Class *cl, Object *obj, struct opSet *msg)
{
	struct TabTitleData *d = INST_DATA(cl, obj);
	int tagcount = 0;
	struct TagItem *tag = 0, *tagptr = msg->ops_AttrList;
	BOOL relayout = FALSE;

	while((tag = NextTagItem(&tagptr)))
	{
		switch(tag->ti_Tag)
		{
			case TTA_Unread:
				d->ttd_Unread = (BOOL)tag->ti_Data;
				relayout = TRUE;
				tagcount++;
			break;

			case MUIA_Text_PreParse:
				StrCopy((STRPTR)tag->ti_Data, d->ttd_PreParse);
				if(d->ttd_Unread)
					StrCat("\33b", d->ttd_PreParse);
				tag->ti_Data = (IPTR)d->ttd_PreParse;
			break;
		}
	}

	tagcount += DoSuperMethodA(cl, obj, (Msg)msg);

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
		case OM_SET: return(TabTitleSet(cl, obj, (struct opSet*)msg));
		default: return(DoSuperMethodA(cl, obj, msg));
	}
}

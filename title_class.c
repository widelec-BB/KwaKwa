/*
 * Copyright (c) 2012-2022 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/asl.h>
#include <libraries/mui.h>

#include <dos/dostags.h>

#include "globaldefines.h"
#include "title_class.h"

struct TitleClassData
{
	Object *last_deleted;
};

struct MUI_CustomClass *TitleClass;
static IPTR TitleDispatcher(void);
const struct EmulLibEntry TitleGate = {TRAP_LIB, 0, (void(*)(void))TitleDispatcher};

struct MUI_CustomClass *CreateTitleClass(void)
{
	struct MUI_CustomClass *cl;

	cl = MUI_CreateCustomClass(NULL, MUIC_Title, NULL, sizeof(struct TitleClassData), (APTR)&TitleGate);
	TitleClass = cl;
	return cl;
}

void DeleteTitleClass(void)
{
	if(TitleClass) MUI_DeleteCustomClass(TitleClass);
}

static IPTR TitleSet(Class *cl, Object *obj, struct opSet *msg)
{
	struct TitleClassData *d = INST_DATA(cl, obj);
	int tagcount = 0;
	struct TagItem *tag = 0, *tagptr = msg->ops_AttrList;

	while((tag = NextTagItem(&tagptr)))
	{
		switch(tag->ti_Tag)
		{
			case TITA_Deleted:
				d->last_deleted = (Object *)tag->ti_Data;
				tagcount++;
			break;
		}
	}

	tagcount += DoSuperMethodA(cl, obj, (Msg)msg);
	return tagcount;
}

static IPTR TitleGet(Class *cl, Object *obj, struct opGet *msg)
{
	struct TitleClassData *d = INST_DATA(cl, obj);
	int rv = FALSE;

	switch(msg->opg_AttrID)
	{
		case TITA_Deleted:
			*msg->opg_Storage = (ULONG)d->last_deleted;
		return TRUE;

		default:
			rv = (DoSuperMethodA(cl, obj, (Msg)msg));
	}

	return rv;
}

static IPTR TitleClose(Class *cl, Object *obj, struct MUIP_Title_Close *msg)
{
	if(msg->tito)
		set(obj, TITA_Deleted, msg->tito);

	return(DoSuperMethodA(cl,obj,msg));
}

static IPTR TitleDispatcher(void)
{
	Class *cl = (Class*)REG_A0;
	Object *obj = (Object*)REG_A2;
	Msg msg = (Msg)REG_A1;

	switch(msg->MethodID)
	{
		case OM_SET: return(TitleSet(cl, obj, (struct opSet*)msg));
		case OM_GET: return(TitleGet(cl, obj, (struct opGet*)msg));
		case MUIM_Title_Close: return(TitleClose(cl, obj, (struct MUIP_Title_Close*)msg));
		default: return (DoSuperMethodA(cl, obj, msg));
	}
}

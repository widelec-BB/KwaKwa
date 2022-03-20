/*
 * Copyright (c) 2012-2022 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <proto/exec.h>
#include <proto/dos.h>
#include <libvstring.h>
#include "locale.h"
#include "globaldefines.h"
#include "modulesmsglist.h"
#include "kwakwa_api/protocol.h"

struct MUI_CustomClass *ModulesMsgListClass;

static IPTR ModulesMsgListDispatcher(VOID);
const struct EmulLibEntry ModulesMsgListGate = {TRAP_LIB, 0, (VOID(*)(VOID))ModulesMsgListDispatcher};

struct MUI_CustomClass *CreateModulesMsgListClass(VOID)
{
	struct MUI_CustomClass *cl;

	cl = MUI_CreateCustomClass(NULL, MUIC_List, NULL, 0, (APTR)&ModulesMsgListGate);
	ModulesMsgListClass = cl;
	return cl;
}

VOID DeleteModulesMsgListClass(VOID)
{
	if (ModulesMsgListClass) MUI_DeleteCustomClass(ModulesMsgListClass);
}

static IPTR ModulesMsgListNew(Class *cl, Object *obj, struct opSet *msg)
{
	obj = DoSuperNew(cl, obj,
		MUIA_Unicode, TRUE,
		MUIA_Frame, MUIV_Frame_ReadList,
		MUIA_Background, MUII_ReadListBack,
		MUIA_List_Title, TRUE,
		MUIA_List_Format, "BAR,BAR,BAR",
	TAG_MORE, msg->ops_AttrList);

	if(obj)
	{
		return (IPTR)obj;
	}

	CoerceMethod(cl, obj, OM_DISPOSE);
	return (IPTR)NULL;
}

static IPTR ModulesMsgListConstruct(Class *cl, Object *obj, struct MUIP_List_Construct *msg)
{
	struct ModulesMsgListEntry *src = msg->entry;
	struct ModulesMsgListEntry *new = NULL;

	if(src)
	{
		if((new = AllocPooled(msg->pool, sizeof(struct ModulesMsgListEntry))))
		{
			new->custom_msg = StrNew(src->custom_msg);
			new->module_name = StrNew(src->module_name);
			new->errno = src->errno;
		}
	}

	return (IPTR)new;
}

static IPTR ModulesMsgListDestruct(Class *cl, Object *obj, struct MUIP_List_Destruct *msg)
{
	struct ModulesMsgListEntry *entry = (struct ModulesMsgListEntry*)msg->entry;

	if(entry)
	{
		if(entry->custom_msg)
			StrFree(entry->custom_msg);
		if(entry->module_name)
			StrFree(entry->module_name);

		FreePooled(msg->pool, msg->entry, sizeof(struct ModulesMsgListEntry));
	}
	return 0;
}

static IPTR ModulesMsgListDisplay(Class *cl, Object *obj, struct MUIP_List_Display *msg)
{
	struct ModulesMsgListEntry *entry = msg->entry;

	if(entry != NULL)
	{
		msg->array[0] = entry->module_name;
		msg->array[2] = entry->custom_msg ? entry->custom_msg : (STRPTR)"NULL";

		switch(entry->errno)
		{
			case ERRNO_CONNECTION_FAILED:
				msg->array[1] = GetString(MSG_MODULE_ERRNO_CONNECTION_FAILED);
			break;

			case ERRNO_LOGIN_FAILED:
				msg->array[1] = GetString(MSG_MODULE_ERRNO_LOGIN_FAILED);
			break;

			case ERRNO_NOT_SUPPORTED:
				msg->array[1] = GetString(MSG_MODULE_ERRNO_NOT_SUPPORTED);
			break;

			case ERRNO_ONLY_MESSAGE:
				msg->array[1] = GetString(MSG_MODULE_ERRNO_ONLY_MESSAGE);
			break;

			case ERRNO_OUT_OF_MEMORY:
				msg->array[1] = GetString(MSG_MODULE_ERRNO_OUT_OF_MEMORY);
			break;
		}
	}
	else
	{
		msg->array[0] = GetString(MSG_MODULE_MSG_LIST_TITLE_NAME);
		msg->array[1] = GetString(MSG_MODULE_MSG_LIST_TITLE_MSG_TYPE);
		msg->array[2] = GetString(MSG_MODULE_MSG_LIST_TITLE_CUSTOM_MSG);
	}

	return (IPTR)NULL;
}


static IPTR ModulesMsgListDispatcher(VOID)
{
	Class *cl = (Class*)REG_A0;
	Object *obj = (Object*)REG_A2;
	Msg msg = (Msg)REG_A1;

	switch (msg->MethodID)
	{
		case OM_NEW: return(ModulesMsgListNew(cl, obj, (struct opSet*) msg));
		case MUIM_List_Construct: return(ModulesMsgListConstruct(cl, obj, (struct MUIP_List_Construct*)msg));
		case MUIM_List_Destruct: return(ModulesMsgListDestruct(cl, obj, (struct MUIP_List_Destruct*)msg));
		case MUIM_List_Display: return(ModulesMsgListDisplay(cl, obj, (struct MUIP_List_Display*)msg));
		default: return(DoSuperMethodA(cl, obj, msg));
	}
}

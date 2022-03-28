/*
 * Copyright (c) 2012-2022 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include <libvstring.h>

#include "globaldefines.h"
#include "locale.h"
#include "support.h"
#include "modulesmsglist.h"
#include "moduleslogwindow.h"

struct MUI_CustomClass *ModulesLogWindowClass;
static IPTR ModulesLogWindowDispatcher(VOID);
const struct EmulLibEntry ModulesLogWindowGate = {TRAP_LIB, 0, (VOID(*)(VOID))ModulesLogWindowDispatcher};

struct MLP_AddMsg {ULONG MethodID; STRPTR module_name; ULONG errno; STRPTR msg_txt;};

struct ModulesLogWindowData
{
	Object *msg_list;
	STRPTR win_title;
};

struct MUI_CustomClass *CreateModulesLogWindowClass(VOID)
{
	struct MUI_CustomClass *cl;

	cl = MUI_CreateCustomClass(NULL, MUIC_Window, NULL, sizeof(struct ModulesLogWindowData), (APTR)&ModulesLogWindowGate);
	ModulesLogWindowClass = cl;
	return cl;
}

VOID DeleteModulesLogWindowClass(VOID)
{
	if (ModulesLogWindowClass) MUI_DeleteCustomClass(ModulesLogWindowClass);
}

static IPTR ModulesLogWindowNew(Class *cl, Object *obj, struct opSet *msg)
{
	Object *list;

	obj = (Object*)DoSuperNew(cl, obj,
		MUIA_Window_ID, (IPTR)USD_MODULESLOG_WINDOW,
		MUIA_Window_Title, (IPTR)GetString(MSG_MODULE_LOG_WINDOW_TITLE),
		MUIA_Window_ScreenTitle, (IPTR)APP_SCREEN_TITLE,
		MUIA_Window_RootObject, (IPTR)MUI_NewObject(MUIC_Group,
			MUIA_Frame, MUIV_Frame_Group,
			MUIA_Background, MUII_GroupBack,
			MUIA_Group_Child, (IPTR)(list = NewObject(ModulesMsgListClass->mcc_Class, NULL, TAG_END)),
		TAG_END),
	TAG_MORE, (IPTR)msg->ops_AttrList);

	if(obj)
	{
		struct ModulesLogWindowData *d = INST_DATA(cl, obj);

		if((d->win_title = Utf8ToSystem((STRPTR)xget(obj, MUIA_Window_Title))))
			set(obj, MUIA_Window_Title, (IPTR)d->win_title);

		d->msg_list = list;

		DoMethod(obj, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, (IPTR)obj, 3,
		 MUIM_Set, MUIA_Window_Open, FALSE);

		return (IPTR)obj;
	}
	return (IPTR)NULL;
}

static IPTR ModulesLogWindowDispose(Class *cl, Object *obj, Msg msg)
{
	struct ModulesLogWindowData *d = INST_DATA(cl, obj);

	if(d->win_title)
		StrFree(d->win_title);

	return DoSuperMethodA(cl, obj, msg);
}

static IPTR ModulesLogWindowAddMsg(Class *cl, Object *obj, struct MLP_AddMsg *msg)
{
	struct ModulesLogWindowData *d = INST_DATA(cl, obj);
	struct ModulesMsgListEntry new_entry;

	new_entry.module_name = msg->module_name;
	new_entry.errno = msg->errno;
	new_entry.custom_msg = msg->msg_txt;

	DoMethod(d->msg_list, MUIM_List_InsertSingle, (IPTR)&new_entry, MUIV_List_Insert_Bottom);

	return (IPTR)0;
}


static IPTR ModulesLogWindowDispatcher(VOID)
{
	Class *cl = (Class*)REG_A0;
	Object *obj = (Object*)REG_A2;
	Msg msg = (Msg)REG_A1;

	switch (msg->MethodID)
	{
		case OM_NEW: return(ModulesLogWindowNew(cl, obj, (struct opSet*)msg));
		case OM_DISPOSE: return(ModulesLogWindowDispose(cl, obj, msg));
		case MLW_AddMsg: return(ModulesLogWindowAddMsg(cl, obj, (struct MLP_AddMsg*)msg));
		default: return(DoSuperMethodA(cl, obj, msg));
	}
}

/*
 * Copyright (c) 2012-2022 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <proto/exec.h>
#include <proto/dos.h>
#include <libvstring.h>

#include "globaldefines.h"
#include "application.h"
#include "locale.h"
#include "support.h"
#include "simplestringlist.h"
#include "prefswindow.h"
#include "virtualtext.h"
#include "historycontactslist.h"
#include "historyconversationslist.h"
#include "historywindow.h"

struct MUI_CustomClass *HistoryWindowClass;
static IPTR HistoryWindowDispatcher(VOID);
const struct EmulLibEntry HistoryWindowGate = {TRAP_LIB, 0, (VOID(*)(VOID))HistoryWindowDispatcher};

struct HWP_ContactSelected {ULONG MethodID; LONG contact_no;};
struct HWP_ConversationSelected {ULONG MethodID; LONG contact_no;};
struct HWP_InsertMessage {ULONG MethodID; ULONG flags; ULONG timestamp; UBYTE *content; ULONG content_len;};

struct HistoryWindowData
{
	Object *contacts_list;
	Object *conversations_list;
	Object *virtual_text;

	STRPTR win_title;
};

struct MUI_CustomClass *CreateHistoryWindowClass(VOID)
{
	struct MUI_CustomClass *cl;

	cl = MUI_CreateCustomClass(NULL, MUIC_Window, NULL, sizeof(struct HistoryWindowData), (APTR)&HistoryWindowGate);
	HistoryWindowClass = cl;
	return cl;
}

VOID DeleteHistoryWindowClass(VOID)
{
	if (HistoryWindowClass) MUI_DeleteCustomClass(HistoryWindowClass);
}

static IPTR HistoryWindowNew(Class *cl, Object *obj, struct opSet *msg)
{
	Object *con_list, *conv_list, *vt, *scroll;

	obj = (Object*)DoSuperNew(cl, obj,
		MUIA_Window_ID, (IPTR)USD_HISTORY_WINDOW,
		MUIA_Window_Title, (IPTR)GetString(MSG_HISTORY_WINDOW_TITLE),
		MUIA_Window_ScreenTitle, (IPTR)APP_SCREEN_TITLE,
		MUIA_Window_RootObject, (IPTR)MUI_NewObject(MUIC_Group,
			MUIA_Group_Child, MUI_NewObject(MUIC_Group,
				MUIA_Group_Horiz, TRUE,
				MUIA_Group_Child, (IPTR)MUI_NewObject(MUIC_Group,
					MUIA_HorizWeight, 25,
					MUIA_Group_Child, (IPTR)(con_list = NewObject(HistoryContactsListClass->mcc_Class, NULL,
					TAG_END)),
				TAG_END),
				MUIA_Group_Child, (IPTR)MUI_NewObject(MUIC_Balance, TAG_END),
				MUIA_Group_Child, (IPTR)MUI_NewObject(MUIC_Group,
					MUIA_Group_Child, (IPTR)(conv_list = NewObject(HistoryConversationsListClass->mcc_Class, NULL,
					TAG_END)),
					MUIA_Group_Child, (IPTR)MUI_NewObject(MUIC_Balance, TAG_END),
					MUIA_Group_Child, (scroll = MUI_NewObject(MUIC_Scrollgroup,
						MUIA_Scrollgroup_FreeHoriz, FALSE,
						MUIA_Scrollgroup_UseWinBorder, TRUE,
						MUIA_Scrollgroup_Contents, (vt = NewObject(VirtualTextClass->mcc_Class, NULL,
							MUIA_Background, MUII_ReadListBack,
							MUIA_Frame, MUIV_Frame_ReadList,
							MUIA_UserData, USD_HISTORY_VTEXT,
							VTA_ScrollOnResize, (ULONG)FALSE,
						TAG_END)),
					TAG_END)),
				TAG_END),
			TAG_END),
		TAG_END),
	TAG_MORE, (IPTR)msg->ops_AttrList);

	if(obj)
	{
		struct HistoryWindowData *d = INST_DATA(cl, obj);

		if((d->win_title = Utf8ToSystem((STRPTR)xget(obj, MUIA_Window_Title))))
			set(obj, MUIA_Window_Title, (IPTR)d->win_title);

		d->contacts_list = con_list;
		d->conversations_list = conv_list;
		d->virtual_text = vt;

		DoMethod(obj, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, (IPTR)obj, 3,
		 MUIM_Set, MUIA_Window_Open, FALSE);

		DoMethod(obj, MUIM_Notify, MUIA_Window_Open, TRUE, d->contacts_list, 1,
		 MUIM_List_Clear);

		DoMethod(obj, MUIM_Notify, MUIA_Window_Open, TRUE, MUIV_Notify_Application, 2,
		 APPM_GetContactsFromHistory, d->contacts_list);

		DoMethod(d->contacts_list, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime, MUIV_Notify_Window, 2,
		 HWM_ContactSelected, MUIV_TriggerValue);

		DoMethod(d->conversations_list, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime, MUIV_Notify_Window, 2,
		 HWM_ConversationSelected, MUIV_TriggerValue);

		return (IPTR)obj;
	}
	return (IPTR)NULL;
}

static IPTR HistoryWindowDispose(Class *cl, Object *obj, Msg msg)
{
	struct HistoryWindowData *d = INST_DATA(cl, obj);

	if(d->win_title)
		StrFree(d->win_title);

	return DoSuperMethodA(cl, obj, msg);
}

static IPTR HistoryWindowContactSelected(Class *cl, Object *obj, struct HWP_ContactSelected *msg)
{
	struct HistoryWindowData *d = INST_DATA(cl, obj);
	struct HContactsListEntry *e;

	DoMethod(d->contacts_list, MUIM_List_GetEntry, msg->contact_no, &e);

	DoMethod(d->virtual_text, VTM_Clear);
	DoMethod(d->conversations_list, MUIM_List_Clear);

	if(e)
		DoMethod(_app(obj), APPM_GetConversationsFromHistory, d->conversations_list, e->plugin_id, e->id);

	return (IPTR)0;
}

static IPTR HistoryWindowConversationSelected(Class *cl, Object *obj, struct HWP_ConversationSelected *msg)
{
	struct HistoryWindowData *d = INST_DATA(cl, obj);
	struct HConversationsListEntry *e;
	ENTER();

	DoMethod(d->conversations_list, MUIM_List_GetEntry, msg->contact_no, &e);
	DoMethod(d->virtual_text, VTM_Clear);

	if(e)
	{
		if(DoMethod(d->virtual_text, VTM_InitChange))
		{
			DoMethod(_app(obj), APPM_GetMessagesFromHistory, obj, HWM_InsertMessage, &e->id);
			DoMethod(d->virtual_text, VTM_ExitChange);
		}
	}
	LEAVE();
	return (IPTR)0;
}

static IPTR HistoryWindowInsertMessage(Class *cl, Object *obj, struct HWP_InsertMessage *msg)
{
	struct HistoryWindowData *d = INST_DATA(cl, obj);
	STRPTR sender = NULL;
	struct HConversationsListEntry *e;
	ENTER();

	DoMethod(d->conversations_list, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &e);

	if(msg->flags & HISTORY_MESSAGES_NORMAL)
	{
		ULONG flags = 0;

		if(msg->flags & HISTORY_MESSAGES_FRIEND)
		{
			flags |= VTV_Incoming;
			sender = e->contact_name;
		}
		else
		{
			flags |= VTV_Outgoing;
			sender = (STRPTR)xget(prefs_object(USD_PREFS_TW_USRNAME_STRING), MUIA_String_Contents);
		}

		DoMethod(d->virtual_text, VTM_AddMessageHeadLine, sender, msg->timestamp, flags);
		DoMethod(d->virtual_text, VTM_AddMessage, msg->content, flags);
	}
	else if(msg->flags & HISTORY_MESSAGES_SYSTEM)
	{
		DoMethod(d->virtual_text, VTM_AddSystemMessage, msg->content, msg->timestamp, 0);
	}

	LEAVE();
	return(IPTR)0;
}

static IPTR HistoryWindowDispatcher(VOID)
{
	Class *cl = (Class*)REG_A0;
	Object *obj = (Object*)REG_A2;
	Msg msg = (Msg)REG_A1;

	switch (msg->MethodID)
	{
		case OM_NEW: return(HistoryWindowNew(cl, obj, (struct opSet*)msg));
		case OM_DISPOSE: return(HistoryWindowDispose(cl, obj, msg));
		case HWM_ContactSelected: return(HistoryWindowContactSelected(cl, obj, (struct HWP_ContactSelected*)msg));
		case HWM_ConversationSelected: return(HistoryWindowConversationSelected(cl, obj, (struct HWP_ConversationSelected*)msg));
		case HWM_InsertMessage: return(HistoryWindowInsertMessage(cl, obj, (struct HWP_InsertMessage*)msg));
		default: return(DoSuperMethodA(cl, obj, msg));
	}
}

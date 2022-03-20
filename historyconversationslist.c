/*
 * Copyright (c) 2012-2022 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/ezxml.h>
#include <libvstring.h>
#include "locale.h"
#include "globaldefines.h"
#include "application.h"
#include "support.h"
#include "kwakwa_api/defs.h"
#include "historyconversationslist.h"

#define HCLP_Remove MUIP_List_Remove

#define MUIV_HistoryConversationsList_Menu_Remove 1

struct MUI_CustomClass *HistoryConversationsListClass;

static IPTR HistoryConversationsListDispatcher(VOID);
const struct EmulLibEntry HistoryConversationsListGate = {TRAP_LIB, 0, (VOID(*)(VOID))HistoryConversationsListDispatcher};

struct HCVLP_TitleClick {ULONG MethodID; LONG column;};

struct HistoryConversationsListData
{
	Object *menu;
	UBYTE display_temp[255];
	LONG clicked_column;
	LONG sort_order;
	struct Locale *locale;
};

struct MUI_CustomClass *CreateHistoryConversationsListClass(VOID)
{
	struct MUI_CustomClass *cl;

	cl = MUI_CreateCustomClass(NULL, MUIC_List, NULL, sizeof(struct HistoryConversationsListData), (APTR)&HistoryConversationsListGate);
	HistoryConversationsListClass = cl;
	return cl;
}

VOID DeleteHistoryConversationsListClass(VOID)
{
	if (HistoryConversationsListClass) MUI_DeleteCustomClass(HistoryConversationsListClass);
}

static IPTR HistoryConversationsListNew(Class *cl, Object *obj, struct opSet *msg)
{
	Object *menu = MUI_NewObject(MUIC_Menustrip,
		MUIA_Unicode, TRUE,
		MUIA_Group_Child, MUI_NewObject(MUIC_Menu,
			MUIA_Group_Child, MUI_NewObject(MUIC_Menuitem,
				MUIA_Menuitem_Title, GetString(MSG_HISTORY_CONTACTS_LIST_MENU_REMOVE),
				MUIA_UserData, MUIV_HistoryConversationsList_Menu_Remove,
			TAG_END),
		TAG_END),
	TAG_END);

	obj = DoSuperNew(cl, obj,
		MUIA_Unicode, TRUE,
		MUIA_Frame, MUIV_Frame_InputList,
		MUIA_Background, MUII_ListBack,
		MUIA_VertWeight, 50,
		MUIA_List_Title, TRUE,
		MUIA_List_Format, (IPTR)"BAR,BAR,BAR",
		MUIA_ContextMenu, menu,
	TAG_MORE, msg->ops_AttrList);

	if(obj)
	{
		struct HistoryConversationsListData *d = INST_DATA(cl, obj);

		if((d->locale = OpenLocale(NULL)))
		{
			DoMethod(obj, MUIM_Notify, MUIA_List_TitleClick, MUIV_EveryTime, MUIV_Notify_Self, 2, HCVLM_TitleClick, MUIV_TriggerValue);

			d->sort_order = 1;
			d->clicked_column = -1;

			d->menu = menu;
			return (IPTR)obj;
		}
	}

	MUI_DisposeObject(menu);
	CoerceMethod(cl, obj, OM_DISPOSE);
	return (IPTR)NULL;
}

static IPTR HistoryConversationsListSet(Class *cl, Object *obj, struct opSet *msg)
{
	struct HistoryConversationsListData *d = INST_DATA(cl, obj);
	int tagcount = 0;
	struct TagItem *tag = 0, *tagptr = msg->ops_AttrList;

	while((tag = NextTagItem(&tagptr)))
	{
		switch(tag->ti_Tag)
		{
			case HCVLA_SortOrder:
				d->sort_order = tag->ti_Data;
			break;

			case HCVLA_ClickedColumn:
				d->clicked_column = tag->ti_Data;
			break;
		}
	}

	tagcount += DoSuperMethodA(cl, obj, (Msg)msg);
	return tagcount;
}

static IPTR HistoryConversationsListDispose(Class *cl, Object *obj, Msg msg)
{
	struct HistoryConversationsListData *d = INST_DATA(cl, obj);

	MUI_DisposeObject(d->menu);
	CloseLocale(d->locale);

	return DoSuperMethodA(cl, obj, msg);
}

static IPTR HistoryConversationsListConstruct(Class *cl, Object *obj, struct MUIP_List_Construct *msg)
{
	struct HConversationsListEntry *entry = (struct HConversationsListEntry*)msg->entry;
	struct HConversationsListEntry *new = NULL;

	if(entry && (new = AllocVecPooled(msg->pool, sizeof(struct HConversationsListEntry))))
	{
		new->id = entry->id;
		new->timestamp_start = entry->timestamp_start;
		new->timestamp_end = entry->timestamp_end;
		new->flags = entry->flags;

		if(entry->contact_id)
		{
			new->contact_id = AllocVecPooled(msg->pool, StrLen(entry->contact_id) + 1);
			StrCopy(entry->contact_id, new->contact_id);
		}
		else
			new->contact_id = NULL;

		if(entry->contact_name)
		{
			new->contact_name = AllocVecPooled(msg->pool, StrLen(entry->contact_name) + 1);
			StrCopy(entry->contact_name, new->contact_name);
		}
		else
			new->contact_name = NULL;

		if(entry->user_id)
		{
			new->user_id = AllocVecPooled(msg->pool, StrLen(entry->user_id) + 1);
			StrCopy(entry->user_id, new->user_id);
		}
		else
			new->user_id = NULL;
	}

	return (IPTR)new;
}

static IPTR HistoryConversationsListDestruct(Class *cl, Object *obj, struct MUIP_List_Destruct *msg)
{
	struct HConversationsListEntry *entry = (struct HConversationsListEntry*)msg->entry;

	if(entry)
	{
		if(entry->contact_id)
			FreeVecPooled(msg->pool, entry->contact_id);

		if(entry->contact_name)
			FreeVecPooled(msg->pool, entry->contact_name);

		if(entry->user_id)
			FreeVecPooled(msg->pool, entry->user_id);

		FreeVecPooled(msg->pool, entry);
	}

	return 0;
}

static IPTR HistoryConversationsListDisplay(Class *cl, Object *obj, struct MUIP_List_Display *msg)
{
	struct HistoryConversationsListData *d = INST_DATA(cl, obj);
	struct HConversationsListEntry *entry = (struct HConversationsListEntry*)msg->entry;
	struct DateTime dt;
	UBYTE day_buffer[128];
	static UBYTE started[255], ended[255], title_buf[255];
	ULONG timestamp;

	if(!entry)
	{
		msg->array[0] = GetString(MSG_HISTORY_CONVERSATIONS_LIST_TITLE_CONTACT);
		msg->array[1] = GetString(MSG_HISTORY_CONVERSATIONS_LIST_TITLE_START_TIME);
		msg->array[2] = GetString(MSG_HISTORY_CONVERSATIONS_LIST_TITLE_END_TIME);

		switch(d->clicked_column)
		{
			case 0:
				FmtNPut(title_buf, "%ls \033I[6:3%c]", sizeof(title_buf), GetString(MSG_HISTORY_CONVERSATIONS_LIST_TITLE_CONTACT), ((d->sort_order > 0) ? '8' : '9'));
			break;

			case 1:
				FmtNPut(title_buf, "%ls \033I[6:3%c]", sizeof(title_buf), GetString(MSG_HISTORY_CONVERSATIONS_LIST_TITLE_START_TIME), ((d->sort_order > 0) ? '8' : '9'));
			break;

			case 2:
				FmtNPut(title_buf, "%ls \033I[6:3%c]", sizeof(title_buf), GetString(MSG_HISTORY_CONVERSATIONS_LIST_TITLE_END_TIME), ((d->sort_order > 0) ? '8' : '9'));
			break;
		}

		msg->array[d->clicked_column] = title_buf;
	}
	else
	{
		STRPTR date_str_unicode;

		msg->array[0] = entry->contact_name;
		msg->array[1] = NULL;
		msg->array[2] = NULL;

		timestamp = UTCToLocal(entry->timestamp_start, NULL);

		Amiga2DateStamp(timestamp, &dt.dat_Stamp);
		dt.dat_Format = FORMAT_DEF;
		dt.dat_Flags = DTF_SUBST;
		dt.dat_StrDate = day_buffer;
		dt.dat_StrDay = NULL;
		dt.dat_StrTime = NULL;
		DateToStr(&dt);

		date_str_unicode = SystemToUtf8(dt.dat_StrDate);

		FmtNPut((STRPTR)started, "%ls %02d:%02d:%02d", sizeof(started), date_str_unicode ? date_str_unicode : (STRPTR)dt.dat_StrDate,
		 dt.dat_Stamp.ds_Minute / 60, dt.dat_Stamp.ds_Minute % 60, dt.dat_Stamp.ds_Tick / TICKS_PER_SECOND);

		if(date_str_unicode)
			StrFree(date_str_unicode);

		msg->array[1] = started;

		timestamp = UTCToLocal(entry->timestamp_end, NULL);

		Amiga2DateStamp(timestamp, &dt.dat_Stamp);
		dt.dat_Format = FORMAT_DEF;
		dt.dat_Flags = DTF_SUBST;
		dt.dat_StrDate = day_buffer;
		dt.dat_StrDay = NULL;
		dt.dat_StrTime = NULL;
		DateToStr(&dt);

		date_str_unicode = SystemToUtf8(dt.dat_StrDate);

		FmtNPut((STRPTR)ended, "%ls %02d:%02d:%02d", sizeof(ended), date_str_unicode ? date_str_unicode : (STRPTR)dt.dat_StrDate,
		 dt.dat_Stamp.ds_Minute / 60, dt.dat_Stamp.ds_Minute % 60, dt.dat_Stamp.ds_Tick / TICKS_PER_SECOND);

		if(date_str_unicode)
			StrFree(date_str_unicode);

		msg->array[2] = ended;
	}

	return (IPTR)0;
}

static IPTR HistoryConversationsListCompare(Class *cl, Object *obj, struct MUIP_List_Compare *msg)
{
	struct HistoryConversationsListData *d = INST_DATA(cl, obj);
	struct HConversationsListEntry *entry1 = (struct HConversationsListEntry *)msg->entry1;
	struct HConversationsListEntry *entry2 = (struct HConversationsListEntry *)msg->entry2;
	IPTR res = 0;

	switch(d->clicked_column)
	{
		case 0:
			res = StrnCmp(d->locale, entry1->contact_name, entry2->contact_name, -1, SC_COLLATE2);
		break;

		case 1:
			if(entry1->timestamp_start < entry2->timestamp_start)
				res = -1;
			else if(entry1->timestamp_start > entry2->timestamp_start)
				res = 1;
			else
				res = 0;
		break;

		case 2:
			if(entry1->timestamp_end < entry2->timestamp_end)
				res = -1;
			else if(entry1->timestamp_end > entry2->timestamp_end)
				res = 1;
			else
				res = 0;
		break;
	}

	return (IPTR)res * d->sort_order;
}

static IPTR HistoryConversationsListContextMenuChoice(Class *cl, Object *obj, struct MUIP_ContextMenuChoice *msg)
{
	switch(muiUserData(msg->item))
	{
		case MUIV_HistoryConversationsList_Menu_Remove:
			DoMethod(obj, HCVLM_Remove, MUIV_List_Remove_Active);
		break;
	}

	return (IPTR)0;
}

static IPTR HistoryConversationsListRemove(Class *cl, Object *obj, struct HCLP_Remove *msg)
{
	struct HConversationsListEntry *entry = NULL;

	DoMethod(obj, MUIM_List_GetEntry, msg->pos, &entry);

	if(entry)
	{
		set(_app(obj), MUIA_Application_Sleep, TRUE);

		if(MUI_Request_Unicode(_app(obj), _win(obj), APP_NAME, GetString(MSG_HISTORY_CONVERSATIONS_LIST_REMOVE_REQ_GADGETS), GetString(MSG_HISTORY_CONVERSATIONS_LIST_REMOVE_REQ_TXT)) == 1)
		{
			DoMethod(_app(obj), APPM_DeleteConversationFromHistory, &entry->id);

			DoMethod(obj, MUIM_List_Remove, MUIV_List_Remove_Active);
		}

		set(_app(obj), MUIA_Application_Sleep, FALSE);
	}

	return (IPTR)0;
}

static IPTR HistoryConversationsListTitleClick(Class *cl, Object *obj, struct HCVLP_TitleClick *msg)
{
	struct HistoryConversationsListData *d = INST_DATA(cl, obj);

	if(d->clicked_column != msg->column)
	{
		d->clicked_column = msg->column;
		d->sort_order = 1;
	}
	else
		d->sort_order = -d->sort_order;

	set(obj, MUIA_List_Quiet, TRUE);
	DoMethod(obj, MUIM_List_Sort);
	set(obj, MUIA_List_Quiet, FALSE);

	return 0;
}

static IPTR HistoryConversationsListDispatcher(VOID)
{
	Class *cl = (Class*)REG_A0;
	Object *obj = (Object*)REG_A2;
	Msg msg = (Msg)REG_A1;

	switch(msg->MethodID)
	{
		case OM_NEW: return (HistoryConversationsListNew(cl, obj, (struct opSet*) msg));
		case OM_SET: return(HistoryConversationsListSet(cl, obj, (struct opSet*)msg));
		case OM_DISPOSE: return (HistoryConversationsListDispose(cl, obj, msg));
		case MUIM_List_Construct: return(HistoryConversationsListConstruct(cl, obj, (struct MUIP_List_Construct*)msg));
		case MUIM_List_Destruct: return(HistoryConversationsListDestruct(cl, obj, (struct MUIP_List_Destruct*)msg));
		case MUIM_List_Display: return(HistoryConversationsListDisplay(cl, obj, (struct MUIP_List_Display*)msg));
		case MUIM_List_Compare: return(HistoryConversationsListCompare(cl, obj, (struct MUIP_List_Compare*)msg));
		case MUIM_ContextMenuChoice: return(HistoryConversationsListContextMenuChoice(cl, obj, (struct MUIP_ContextMenuChoice*)msg));
		case HCVLM_Remove: return(HistoryConversationsListRemove(cl, obj, (struct HCLP_Remove*)msg));
		case HCVLM_TitleClick: return(HistoryConversationsListTitleClick(cl, obj, (struct HCVLP_TitleClick*)msg));
		default: return (DoSuperMethodA(cl, obj, msg));
	}
}

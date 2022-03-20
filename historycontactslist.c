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
#include "historycontactslist.h"

#define HCLP_Remove MUIP_List_Remove

#define MUIV_HistoryContactsList_Menu_Remove 1

struct MUI_CustomClass *HistoryContactsListClass;

static IPTR HistoryContactsListDispatcher(VOID);
const struct EmulLibEntry HistoryContactsListGate = {TRAP_LIB, 0, (VOID(*)(VOID))HistoryContactsListDispatcher};

struct HCLP_TitleClick {ULONG MethodID; LONG column;};

struct HistoryContactsListData
{
	Object *menu;
	UBYTE display_temp[255];
	LONG sort_order;
	struct Locale *locale;
};

struct MUI_CustomClass *CreateHistoryContactsListClass(VOID)
{
	struct MUI_CustomClass *cl;

	cl = MUI_CreateCustomClass(NULL, MUIC_List, NULL, sizeof(struct HistoryContactsListData), (APTR)&HistoryContactsListGate);
	HistoryContactsListClass = cl;
	return cl;
}

VOID DeleteHistoryContactsListClass(VOID)
{
	if (HistoryContactsListClass) MUI_DeleteCustomClass(HistoryContactsListClass);
}

static IPTR HistoryContactsListNew(Class *cl, Object *obj, struct opSet *msg)
{
	Object *menu = MUI_NewObject(MUIC_Menustrip,
		MUIA_Unicode, TRUE,
		MUIA_Group_Child, MUI_NewObject(MUIC_Menu,
			MUIA_Group_Child, MUI_NewObject(MUIC_Menuitem,
				MUIA_Menuitem_Title, GetString(MSG_HISTORY_CONTACTS_LIST_MENU_REMOVE),
				MUIA_UserData, MUIV_HistoryContactsList_Menu_Remove,
			TAG_END),
		TAG_END),
	TAG_END);

	obj = DoSuperNew(cl, obj,
		MUIA_Frame, MUIV_Frame_InputList,
		MUIA_Background, MUII_ListBack,
		MUIA_List_Title, TRUE,
		MUIA_List_Format, "",
		MUIA_ContextMenu, menu,
		MUIA_Unicode, TRUE,
	TAG_MORE, msg->ops_AttrList);

	if(obj)
	{
		struct HistoryContactsListData *d = INST_DATA(cl, obj);

		if((d->locale = OpenLocale(NULL)))
		{
			DoMethod(obj, MUIM_Notify, MUIA_List_TitleClick, MUIV_EveryTime, MUIV_Notify_Self, 2, HCLM_TitleClick, MUIV_TriggerValue);

			d->menu = menu;
			d->sort_order = 1;

			return (IPTR)obj;
		}
	}

	MUI_DisposeObject(menu);
	CoerceMethod(cl, obj, OM_DISPOSE);
	return (IPTR)NULL;
}

static IPTR HistoryContactsListSet(Class *cl, Object *obj, struct opSet *msg)
{
	struct HistoryContactsListData *d = INST_DATA(cl, obj);
	int tagcount = 0;
	struct TagItem *tag = 0, *tagptr = msg->ops_AttrList;

	while((tag = NextTagItem(&tagptr)))
	{
		switch(tag->ti_Tag)
		{
			case HCLA_SortOrder:
				d->sort_order = tag->ti_Data;
			break;
		}
	}

	tagcount += DoSuperMethodA(cl, obj, (Msg)msg);
	return tagcount;
}

static IPTR HistoryContactsListDispose(Class *cl, Object *obj, Msg msg)
{
	struct HistoryContactsListData *d = INST_DATA(cl, obj);

	MUI_DisposeObject(d->menu);
	CloseLocale(d->locale);

	return DoSuperMethodA(cl, obj, msg);
}

static IPTR HistoryContactsListConstruct(Class *cl, Object *obj, struct MUIP_List_Construct *msg)
{
	struct HContactsListEntry *entry = (struct HContactsListEntry *)msg->entry;
	struct HContactsListEntry *new = NULL;

	if(msg->entry && (new = AllocVecPooled(msg->pool, sizeof(struct HContactsListEntry))))
	{
		new->plugin_id = entry->plugin_id;

		if(entry->id)
		{
			new->id = AllocVecPooled(msg->pool, StrLen(entry->id) + 1);
			StrCopy(entry->id, new->id);
		}
		else
			new->id = NULL;

		if(entry->name)
		{
			new->name = AllocVecPooled(msg->pool, StrLen(entry->name) + 1);
			StrCopy(entry->name, new->name);
		}
		else
			new->name = NULL;
	}

	return (IPTR)new;
}

static IPTR HistoryContactsListDestruct(Class *cl, Object *obj, struct MUIP_List_Destruct *msg)
{
	struct HContactsListEntry *entry = (struct HContactsListEntry *)msg->entry;

	if(entry)
	{
		if(entry->id)
			FreeVecPooled(msg->pool, entry->id);

		if(entry->name)
			FreeVecPooled(msg->pool, entry->name);

		FreeVecPooled(msg->pool, entry);
	}

	return (IPTR)NULL;
}

static IPTR HistoryContactsListDisplay(Class *cl, Object *obj, struct MUIP_List_Display *msg)
{
	struct HistoryContactsListData *d = INST_DATA(cl, obj);
	struct HContactsListEntry *entry = (struct HContactsListEntry *)msg->entry;

	if(!entry)
	{
		if(d->sort_order != 0)
		{
			FmtNPut(d->display_temp, "%ls \033I[6:3%c]", sizeof(d->display_temp), GetString(MSG_HISTORY_CONTACTS_LIST_TITLE_CONTACT), ((d->sort_order > 0) ? '8' : '9'));
			msg->array[0] = d->display_temp;
		}
		else
			msg->array[0] = GetString(MSG_HISTORY_CONTACTS_LIST_TITLE_CONTACT);
	}
	else
	{
		if(entry->name && entry->id)
		{
			FmtNPut(d->display_temp, "%ls (%ls)", sizeof(d->display_temp), entry->name, entry->id);
			msg->array[0] = d->display_temp;
		}
		else if(entry->name)
			msg->array[0] = entry->name;
		else if(entry->id)
			msg->array[0] = entry->id;
	}

	return (IPTR)0;
}

static IPTR HistoryContactsListCompare(Class *cl, Object *obj, struct MUIP_List_Compare *msg)
{
	struct HistoryContactsListData *d = INST_DATA(cl, obj);
	struct HContactsListEntry *entry1 = (struct HContactsListEntry *)msg->entry1;
	struct HContactsListEntry *entry2 = (struct HContactsListEntry *)msg->entry2;
	IPTR res = StrnCmp(d->locale, entry1->name, entry2->name, -1, SC_COLLATE2);

	if(res == 0)
		res = StrnCmp(d->locale, entry1->id, entry2->id, -1, SC_COLLATE2);

	return (IPTR)res * d->sort_order;
}

static IPTR HistoryContactsListContextMenuChoice(Class *cl, Object *obj, struct MUIP_ContextMenuChoice *msg)
{
	switch(muiUserData(msg->item))
	{
		case MUIV_HistoryContactsList_Menu_Remove:
			DoMethod(obj, HCLM_Remove, MUIV_List_Remove_Active);
		break;
	}

	return (IPTR)0;
}

static IPTR HistoryContactsListRemove(Class *cl, Object *obj, struct HCLP_Remove *msg)
{
	struct HistoryContactsListData *d = INST_DATA(cl, obj);
	STRPTR txt = NULL;
	struct HContactsListEntry *entry = NULL;

	DoMethod(obj, MUIM_List_GetEntry, msg->pos, &entry);

	if(entry)
	{
		if(entry->name && entry->id)
		{
			FmtNPut(d->display_temp, "%ls (%ls)", sizeof(d->display_temp), entry->name, entry->id);
			txt = d->display_temp;
		}
		else if(entry->name)
			txt = entry->name;
		else
			txt = entry->id;

		set(_app(obj), MUIA_Application_Sleep, TRUE);

		if(MUI_Request_Unicode(_app(obj), _win(obj), APP_NAME, GetString(MSG_HISTORY_CONTACTS_LIST_REMOVE_REQ_GADGETS), GetString(MSG_HISTORY_CONTACTS_LIST_REMOVE_REQ_TXT), txt) == 1)
		{
			DoMethod(_app(obj), APPM_DeleteContactFromHistory, entry->plugin_id, entry->id);

			DoMethod(obj, MUIM_List_Remove, MUIV_List_Remove_Active);
		}

		set(_app(obj), MUIA_Application_Sleep, FALSE);
	}

	return (IPTR)0;
}

static IPTR HistoryContactsListTitleClick(Class *cl, Object *obj, struct HCLP_TitleClick *msg)
{
	struct HistoryContactsListData *d = INST_DATA(cl, obj);

	if(d->sort_order == 0)
		d->sort_order = 1;
	else
		d->sort_order = -d->sort_order;

	set(obj, MUIA_List_Quiet, TRUE);
	DoMethod(obj, MUIM_List_Sort);
	set(obj, MUIA_List_Quiet, FALSE);

	return 0;
}

static IPTR HistoryContactsListDispatcher(VOID)
{
	Class *cl = (Class*)REG_A0;
	Object *obj = (Object*)REG_A2;
	Msg msg = (Msg)REG_A1;

	switch(msg->MethodID)
	{
		case OM_NEW: return(HistoryContactsListNew(cl, obj, (struct opSet*) msg));
		case OM_SET: return(HistoryContactsListSet(cl, obj, (struct opSet*)msg));
		case OM_DISPOSE: return(HistoryContactsListDispose(cl, obj, msg));
		case MUIM_List_Construct: return(HistoryContactsListConstruct(cl, obj, (struct MUIP_List_Construct*)msg));
		case MUIM_List_Destruct: return(HistoryContactsListDestruct(cl, obj, (struct MUIP_List_Destruct*)msg));
		case MUIM_List_Display: return(HistoryContactsListDisplay(cl, obj, (struct MUIP_List_Display*)msg));
		case MUIM_List_Compare: return(HistoryContactsListCompare(cl, obj, (struct MUIP_List_Compare*)msg));
		case MUIM_ContextMenuChoice: return(HistoryContactsListContextMenuChoice(cl, obj, (struct MUIP_ContextMenuChoice*)msg));
		case HCLM_Remove: return(HistoryContactsListRemove(cl, obj, (struct HCLP_Remove*)msg));
		case HCLM_TitleClick: return(HistoryContactsListTitleClick(cl, obj, (struct HCLP_TitleClick*)msg));
		default: return (DoSuperMethodA(cl, obj, msg));
	}
}

/*
 * Copyright (c) 2012-2022 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <proto/exec.h>
#include <proto/utility.h>
#include <libvstring.h>
#include <libraries/magicbeacon.h>

#include "locale.h"

#include "globaldefines.h"
#include "application.h"
#include "contactslist.h"
#include "title_class.h"
#include "prefswindow.h"
#include "talktab.h"
#include "talkwindow.h"
#include "tabtitle.h"
#include "smallsbar.h"
#include "support.h"

#include "kwakwa_api/defs.h"

struct MUI_CustomClass *TalkWindowClass;
static IPTR TalkWindowDispatcher(VOID);
const struct EmulLibEntry TalkWindowGate = {TRAP_LIB, 0, (VOID(*)(VOID))TalkWindowDispatcher};

struct TabEntry
{
	ULONG tab_id;
	LONG tab_unique_id;
	struct ContactEntry *list_entry;
	struct ContactEntry entry;
	Object *title;
	Object *tab;
	struct TabEntry *next;
};

static struct TabEntry* AddTabEntry(Class *cl, Object *obj, struct ContactEntry *contact, Object *title, Object *group);
static VOID DeleteTabsList(Class *cl, Object *obj);
static struct TabEntry* FindTabEntryByEntryData(Class *cl, Object *obj, STRPTR entryid, LONG pluginid);

struct TKWP_ShowMessage {ULONG MethodID; STRPTR entryid; LONG pluginid; STRPTR message; ULONG timestamp; ULONG flag;};
struct TKWP_DeleteTab {ULONG MethodID; Object *title;};
struct TKWP_ActivateTab {ULONG MethodID; ULONG tab_id;};
struct TKWP_OpenOnTab {ULONG MethodID; struct TabEntry *tab;};
struct TKWP_OpenOnTabById {ULONG MethodID; ULONG tab_id;};
struct TKWP_UpdateTabContact {ULONG MethodID; STRPTR entryid; LONG pluginid;};
struct TKWP_UpdateTabContactStatus {ULONG MethodID; STRPTR entryid; LONG pluginid;};
struct TKWP_UpdateTabWriteLamp {ULONG MethodID; STRPTR entryid; LONG pluginid; ULONG length;};
struct TKWP_AcceptBeacon {ULONG MethodID; ULONG notify_result; LONG tab_unique_id;};
struct TKWP_ShowPicture {ULONG MethodID; STRPTR entryid; LONG pluginid; ULONG timestamp; ULONG flag; APTR pic; ULONG pic_size;};
struct TKWP_GetTab {ULONG MethodID; ULONG position; Object **entry;};
struct TKWP_SendMessage {ULONG MethodID; STRPTR entryid; LONG pluginid; STRPTR message;};
struct TKWP_ShowInvite {ULONG MethodID; STRPTR entryid; LONG pluginid; ULONG timestamp;};
struct TKWP_DeleteTabByObject {ULONG MethodID; Object *tab;};

struct TalkWindowData
{
	Object *title;
	Object *page_group;
	struct TabEntry *tabs_list_start;
	struct TabEntry *tabs_list_end;
	ULONG tabs_list_entries;
	UBYTE window_title[512];
};

struct MUI_CustomClass *CreateTalkWindowClass(VOID)
{
	struct MUI_CustomClass *cl;

	cl = MUI_CreateCustomClass(NULL, MUIC_Window, NULL, sizeof(struct TalkWindowData), (APTR)&TalkWindowGate);
	TalkWindowClass = cl;
	return cl;
}

VOID DeleteTalkWindowClass(VOID)
{
	if (TalkWindowClass) MUI_DeleteCustomClass(TalkWindowClass);
}

static IPTR TalkWindowNew(Class *cl, Object *obj, struct opSet *msg)
{
	Object *title, *page_group;

	obj = DoSuperNew(cl, obj,
		MUIA_UserData, USD_TALKWINDOW_WINDOW,
		MUIA_Window_ID, USD_TALKWINDOW_WINDOW,
		MUIA_Background, MUII_WindowBack,
		MUIA_Window_Title, GetString(MSG_TALKWINDOW_TITLE),
		MUIA_Window_ScreenTitle, APP_SCREEN_TITLE,
		MUIA_Window_UseRightBorderScroller, TRUE,
		MUIA_Window_RootObject, MUI_NewObject(MUIC_Group,
			MUIA_Group_Child, (page_group = MUI_NewObject(MUIC_Group,
				MUIA_Background, MUII_RegisterBack,
				MUIA_Frame, MUIV_Frame_Register,
				MUIA_Group_Horiz, TRUE,
				MUIA_Group_PageMode, TRUE,
				MUIA_Group_Child, (title = NewObject(TitleClass->mcc_Class, NULL,
					MUIA_CycleChain, 1,
					MUIA_Title_Closable, TRUE,
					/* here add first tab contents */
				TAG_END)),
			TAG_END)),
		TAG_END),
	TAG_MORE, msg->ops_AttrList);

	if(obj)
	{
		struct TalkWindowData *d = INST_DATA(cl, obj);
		d->title = title;
		d->page_group = page_group;


		DoMethod(obj, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, MUIV_Notify_Self, 3,
		 MUIM_Set, MUIA_Window_Open, FALSE);

		DoMethod(title, MUIM_Notify, TITA_Deleted, MUIV_EveryTime, obj, 2,
		 TKWM_DeleteTab, MUIV_TriggerValue);

		DoMethod(page_group, MUIM_Notify, MUIA_Group_ActivePage, MUIV_EveryTime, obj, 2,
		 TKWM_ActivateTab, MUIV_TriggerValue);

		DoMethod(obj, MUIM_Notify, MUIA_Window_Activate, TRUE, MUIV_Notify_Self, 2,
		 TKWM_ActivateTab, -1);

		DoMethod(obj, MUIM_Setup, NULL);

		return (IPTR)obj;
	}
	CoerceMethod(cl, obj, OM_DISPOSE);
	return (IPTR)0;
}

static IPTR TalkWindowDispose(Class *cl, Object *obj, Msg msg)
{
	ENTER();
	DeleteTabsList(cl, obj);

	LEAVE();
	return DoSuperMethodA(cl, obj, msg);
}


static struct TabEntry* AddTabEntry(Class *cl, Object *obj, struct ContactEntry *contact, Object *title, Object *tab)
{
	struct TalkWindowData *d = INST_DATA(cl, obj);
	struct TabEntry *new = AllocMem(sizeof(struct TabEntry), MEMF_ANY | MEMF_CLEAR);

	if(new && (new->entry.entryid = StrNew(contact->entryid)))
	{
		new->tab_id = d->tabs_list_entries++;
		new->tab_unique_id = GetUniqueID();

		/* make copy of contacts list entry */
		new->entry.pluginid = contact->pluginid;
		new->entry.name = StrNew(contact->name);
		new->entry.nickname = StrNew(contact->nickname);
		new->entry.firstname = StrNew(contact->firstname);
		new->entry.lastname = StrNew(contact->lastname);
		new->entry.groupname = StrNew(contact->groupname);
		new->entry.birthyear = StrNew(contact->birthyear);
		new->entry.city = StrNew(contact->city);
		new->entry.statusdesc = StrNew(contact->statusdesc);
		new->entry.status = contact->status;
		new->entry.gender = contact->gender;
		new->entry.unread = contact->unread;
		new->entry.avatar = CopyPicture(contact->avatar);

		new->list_entry = contact;
		new->title = title;
		new->tab = tab;
		new->next = NULL;

		if(d->tabs_list_start == NULL)
		{
			d->tabs_list_start = new;
			d->tabs_list_end = new;
		}
		else
		{
			d->tabs_list_end->next = new;
			d->tabs_list_end = new;
		}
	}
	return new;
}


static struct TabEntry* FindTabEntryByEntryData(Class *cl, Object *obj, STRPTR entryid, LONG pluginid)
{
	struct TalkWindowData *d = INST_DATA(cl, obj);
	struct TabEntry *temp = d->tabs_list_start;

	while(temp != NULL)
	{
		if(StrEqu(temp->entry.entryid, entryid) && temp->entry.pluginid == pluginid)
			break;
		temp = temp->next;
	}
	return temp;
}

static struct TabEntry* FindTabEntryById(Class *cl, Object *obj, ULONG id)
{
	struct TalkWindowData *d = INST_DATA(cl, obj);
	struct TabEntry *temp = d->tabs_list_start;

	while(temp != NULL && temp->tab_id != id)
		temp = temp->next;

	return temp;
}

static struct TabEntry* FindTabEntryByUniqueId(Class *cl, Object *obj, LONG id)
{
	struct TalkWindowData *d = INST_DATA(cl, obj);
	struct TabEntry *temp = d->tabs_list_start;

	while(temp != NULL && temp->tab_unique_id != id)
		temp = temp->next;

	return temp;
}

static VOID DeleteTabsList(Class *cl, Object *obj)
{
	struct TalkWindowData *d = INST_DATA(cl, obj);
	struct TabEntry *temp = d->tabs_list_start, *to_free;

	while(temp)
	{
		to_free = temp;
		temp = temp->next;

		FreeMem(to_free, sizeof(struct TabEntry));
	}
}

static IPTR TalkWindowCreateNewTab(Class *cl, Object *obj, ULONG pluginid, STRPTR entryid, BOOL incoming)
{
	struct TalkWindowData *d = INST_DATA(cl, obj);
	struct ContactEntry *entry;
	Object *parent = (Object*)xget(d->title, MUIA_Parent);
	Object *new_group = NewObject(TalkTabClass->mcc_Class, NULL, TAG_END);
	Object *new_title = NULL, *contacts_list = findobj(USD_CONTACTS_LIST, _app(obj));
	struct TabEntry *new_tab = NULL;
	ULONG i;

	for(i=0;;i++)
	{
		DoMethod(contacts_list, CLSM_GetEntry, i, &entry);
		if(entry == NULL || (entry->pluginid == pluginid && StrEqu(entry->entryid, entryid)))
			break;
	}

	if(entry != NULL)
	{
		tprintf("%ls 0x%lX %ls %ls %ls %ls %ls %ls %ls 0x%lX %ls %ld %ls %lp\n", strd(entry->entryid), entry->pluginid, strd(entry->name),
			strd(entry->nickname), strd(entry->firstname), strd(entry->lastname), strd(entry->groupname), strd(entry->birthyear), strd(entry->city),
				entry->status, strd(entry->statusdesc), (LONG)entry->gender, entry->unread ? "TRUE" : "FALSE", entry->avatar);

		new_title = NewObject(TabTitleClass->mcc_Class, NULL,
			TTA_ContactName, ContactName(entry),
			TTA_Status, entry->status,
			TTA_Unread, incoming,
			TTA_ShowStatusImage, xget(prefs_object(USD_PREFS_TW_TABTITLE_IMAGE_ONOFF), MUIA_Selected),
		TAG_END);

		new_tab = AddTabEntry(cl, obj, entry, new_title, new_group);

		DoMethod(new_group, TTBM_Init, &new_tab->entry);
	}
	else
	{
		struct ContactEntry ce = {0};

		tprintf("contact unknown.\n");

		ce.pluginid = pluginid;
		if((ce.name = entryid))
		{
			ce.entryid = entryid;

			new_title = NewObject(TabTitleClass->mcc_Class, NULL,
				TTA_ContactName, ce.name,
				TTA_Status, ce.status,
				TTA_Unread, incoming,
				TTA_ShowStatusImage, xget(prefs_object(USD_PREFS_TW_TABTITLE_IMAGE_ONOFF), MUIA_Selected),
			TAG_END);

			new_tab = AddTabEntry(cl, obj, &ce, new_title, new_group);

			new_tab->list_entry = (struct ContactEntry*) DoMethod(contacts_list, CLSM_InsertSingle, &ce, CLSV_Insert_Top); /* insert him to list */

			if(incoming) /* ok, we got message from someone unknow */
			{
				DoMethod(_app(obj), APPM_AddNotify, ce.pluginid, ce.entryid, ce.status); /* check his status */
				DoMethod(_app(obj), APPM_PubDirRequest, ce.pluginid, ce.entryid, new_tab->tab, TTBM_PubdirParseResponse); /* and find out who the fuck he is */
			}

			DoMethod(new_group, TTBM_Init, &new_tab->entry);
		}
	}

	tprintf("adding tab to window...\n");

	/*
	** NOTE: Inserting new tab in MUIC_Title in background (without
	** setting it as active) causes trashing MUIC_Title background.
	** To fix this one have to call MUIM_Group_InitChange and
	** (after inserting) MUIM_Group_ExitChange on *parent of parent*
	** of MUIC_Title.
	*/

	DoMethod((Object*)xget(parent, MUIA_Parent), MUIM_Group_InitChange);
	DoMethod(parent, MUIM_Group_InitChange);
	DoMethod(d->title, MUIM_Group_InitChange);

	DoMethod(d->title, OM_ADDMEMBER, new_title);
	DoMethod(parent, OM_ADDMEMBER, new_group);

	DoMethod(d->title, MUIM_Group_ExitChange);
	DoMethod(parent, MUIM_Group_ExitChange);
	DoMethod((Object*)xget(parent, MUIA_Parent), MUIM_Group_ExitChange);

	DoMethod(new_group, TTBM_InsertLastMessages);

	if(incoming == FALSE) /* if user double clicked on contact in list we need to set tab to active */
		set(parent, MUIA_Group_ActivePage, d->tabs_list_entries);

	return (IPTR)new_tab;
}

static IPTR TalkWindowShowMessage(Class *cl, Object *obj, struct TKWP_ShowMessage *msg)
{
	struct TalkWindowData *d = INST_DATA(cl, obj);
	struct TabEntry *tab;
	ENTER();

	if((tab = FindTabEntryByEntryData(cl, obj, msg->entryid, msg->pluginid)))
	{
		tprintf("tab exist, found %lp\n", tab);
	}
	else
	{
		tprintf("tab not found, creating new...\n");
		tab = (struct TabEntry*) TalkWindowCreateNewTab(cl, obj, msg->pluginid, msg->entryid, msg->message != NULL);
	}

	if(tab != NULL)
	{
		if(msg->message == NULL) /* FALSE when we got a message, TRUE if user double clicked on contacts list */
		{
			/* user double clicked on contact in list, so we set tab to active */
			set(obj, MUIA_Window_Open, TRUE);
			set((Object*)xget(d->title, MUIA_Parent), MUIA_Group_ActivePage, tab->tab_id);
			DoMethod(tab->tab, TTBM_Activate);
			DoMethod(tab->tab, TTBM_ShowEnd);
			if(tab->list_entry != NULL)
			{
				if(tab->list_entry->unread == TRUE)
				{
					tab->list_entry->unread = FALSE;
					DoMethod(obj, TKWM_UpdateTabContact, tab->list_entry->entryid, tab->list_entry->pluginid);
					DoMethod(findobj(USD_CONTACTS_LIST, _app(obj)), CLSM_Sort);
					set(_app(obj), APPA_ScreenbarUnread, FALSE); /* have to be after sort! APPM_ScreenbarUnread rely on sorting... */
					set(tab->title, TTA_Unread, FALSE);
				}
			}
		}
		else
		{
			/* got new message, put it to tab, no fight for attention */
			switch(msg->flag)
			{
				case MSG_FLAG_NORMAL:
					DoMethod(tab->tab, TTBM_PutMessage, ContactNameLoc(tab->entry), msg->message, msg->timestamp);
				break;

				case MSG_FLAG_MULTILOGON:
					DoMethod(tab->tab, TTBM_SendMessage, msg->message, FALSE);
				break;

				default:
					tprintf("APPM_GGReciveMessage: unrecognized flag!\n");
				break;
			}

			/* if window is not active at all or we are in other tab set unread flag */
			if((tab->list_entry != NULL && (xget(obj, MUIA_Window_Activate) != TRUE || (xget(d->page_group, MUIA_Group_ActivePage) != tab->tab_id))))
			{
				if(msg->flag != MSG_FLAG_MULTILOGON) /* we don't want to notify when it's our meesage (but from other client) */
				{
					/* set unread state on list and screenbar */
					if(tab->list_entry->unread == FALSE)
					{
						tab->list_entry->unread = TRUE;

						DoMethod(obj, TKWM_UpdateTabContact, tab->entry.entryid, tab->entry.pluginid);
						DoMethod(findobj(USD_CONTACTS_LIST, _app(obj)), CLSM_Sort);
						set(_app(obj), APPA_ScreenbarUnread, TRUE); /* have to be after sort! APPM_ScreenbarUnread rely on sorting... */
					}

					DoMethod(_app(obj), APPM_NotifyBeacon, BEACON_MESSAGE, ContactNameLoc(tab->entry), TRUE, obj, TKWM_AcceptBeacon, tab->tab_unique_id);
					set(tab->title, TTA_Unread, TRUE);
				}
				else if(tab->list_entry->unread == TRUE) /* after receive multilogon message set unread to false (we read this on other session) */
				{
					tab->list_entry->unread = FALSE;
					DoMethod(obj, TKWM_UpdateTabContact, tab->entry.entryid, tab->entry.pluginid);
					DoMethod(findobj(USD_CONTACTS_LIST, _app(obj)), CLSM_Sort);
					set(_app(obj), APPA_ScreenbarUnread, TRUE); /* have to be after sort! APPM_ScreenbarUnread rely on sorting... */
					set(tab->title, TTA_Unread, FALSE);
				}
			}
		}
	}

	LEAVE();
	return (IPTR)1;
}

static IPTR TalkWindowDeleteTab(Class *cl, Object *obj, struct TKWP_DeleteTab *msg)
{
	struct TalkWindowData *d = INST_DATA(cl, obj);
	struct TabEntry *temp_back = NULL, *temp = d->tabs_list_start, *temp_next;

	while(temp != NULL)
	{
		if(temp->title == msg->title) break;
		temp_back = temp;
		temp = temp->next;
	}

	if(temp != NULL)
	{
		if(d->tabs_list_start == temp) /* we will try to remove first tab in the list */
		{
			d->tabs_list_start = temp->next;
		}
		else
		{
			if(d->tabs_list_end == temp) /* we will try to remove last tab in the list */
			{
				d->tabs_list_end = temp_back;
				d->tabs_list_end->next = NULL;
			}
			else /* we will try to remove tab in middle of list */
			{
				temp_back->next = temp->next;
			}
		}

		temp_next = temp->next;
		d->tabs_list_entries--;
		FreeMem(temp, sizeof(struct TabEntry));
		temp = temp_next;

		while(temp != NULL) /* and for every next tab decrement tab_id */
		{
			temp->tab_id--;
			temp_back = temp;
			temp = temp->next;
		}

		if(d->tabs_list_entries == 0)
			set(obj, MUIA_Window_Open, FALSE);
	}

	return (IPTR)1;
}

static IPTR TalkWindowActivateTab(Class *cl, Object *obj, struct TKWP_ActivateTab *msg)
{
	struct TalkWindowData *d = INST_DATA(cl, obj);
	struct TabEntry *tab = d->tabs_list_start;
	ULONG tab_id;

	if(msg->tab_id == -1)
		tab_id = xget(d->page_group, MUIA_Group_ActivePage);
	else
		tab_id = msg->tab_id;

	if(tab_id > d->tabs_list_entries) /* check params */
		return (IPTR)0;

	while(tab != NULL && tab->tab_id != tab_id)
		tab = tab->next;

	if(tab != NULL)
	{
		if(xget(obj, MUIA_Window_Open) == TRUE)
		{
			DoMethod(tab->tab, TTBM_Activate);
			if(tab->list_entry)
			{
				if(tab->list_entry->unread == TRUE)
				{
					tab->list_entry->unread = FALSE;
					DoMethod(obj, TKWM_UpdateTabContact, tab->entry.entryid, tab->entry.pluginid);
					DoMethod(findobj(USD_CONTACTS_LIST, _app(obj)), CLSM_Sort);
					set(_app(obj), APPA_ScreenbarUnread, FALSE); /* have to be after sort! APPM_ScreenbarUnread rely on sorting... */
					set(tab->title, TTA_Unread, FALSE);
				}
			}
		}
		if(ContactName(tab->list_entry))
		{
			FmtNPut(d->window_title, "%s: %s", sizeof(d->window_title) * sizeof(UBYTE), APP_NAME, ContactName(tab->list_entry));
			set(obj, MUIA_Window_Title, d->window_title);
		}
	}

	return (IPTR)1;
}

static IPTR TalkWindowOpenOnTab(Class *cl, Object *obj, struct TKWP_OpenOnTab *msg)
{
	struct TalkWindowData *d = INST_DATA(cl, obj);
	ENTER();

	if(msg->tab)
	{
		set(obj, MUIA_Window_Open, TRUE);
		set(d->page_group, MUIA_Group_ActivePage, msg->tab->tab_id);

		if(msg->tab->list_entry != NULL && msg->tab->list_entry->unread == TRUE)
		{
			msg->tab->list_entry->unread = FALSE;
			DoMethod(obj, TKWM_UpdateTabContact, msg->tab->list_entry->entryid, msg->tab->list_entry->pluginid);
			DoMethod(findobj(USD_CONTACTS_LIST, _app(obj)), CLSM_Sort);
			set(_app(obj), APPA_ScreenbarUnread, FALSE); /* have to be after sort! APPM_ScreenbarUnread rely on sorting... */
			set(msg->tab->title, TTA_Unread, FALSE);
		}

		DoMethod(obj, MUIM_Window_ScreenToFront);
		DoMethod(msg->tab->tab, TTBM_Activate);
		DoMethod(msg->tab->tab, TTBM_ShowEnd);
	}

	LEAVE();
	return (IPTR)1;
}

static IPTR TalkWindowOpenOnTabById(Class *cl, Object *obj, struct TKWP_OpenOnTabById *msg)
{
	DoMethod(obj, TKWM_OpenOnTab, FindTabEntryById(cl, obj, msg->tab_id));

	return (IPTR)1;
}

static IPTR TalkWindowCreateTabsMenuStrip(Class *cl, Object *obj)
{
	struct TalkWindowData *d = INST_DATA(cl, obj);
	Object *result = NULL, *item;
	struct TabEntry *tab = d->tabs_list_start;
	UBYTE buffer[512];

	if(d->tabs_list_entries > 0)
	{
		result = MUI_NewObject(MUIC_Menuitem,
			MUIA_Menuitem_Title, GetString(MSG_TALKWINDOW_TABS_MENU_TITLE),
		TAG_END);

		DoMethod(result, MUIM_Group_InitChange);

		while(tab != NULL)
		{
			if(tab->entry.unread)
				FmtNPut(buffer, "\33b%s", sizeof(buffer), ContactNameLoc(tab->entry));

			item = MUI_NewObject(MUIC_Menuitem,
				MUIA_UserData, SBR_MENU_TALK_TAB + tab->tab_id,
				MUIA_Menuitem_CopyStrings, TRUE,
				MUIA_Menuitem_Title, tab->entry.unread == TRUE ? (STRPTR)buffer : ContactNameLoc(tab->entry),
			TAG_END);

			DoMethod(result, OM_ADDMEMBER, item);

			tab = tab->next;
		}

		DoMethod(result, MUIM_Group_ExitChange);
	}

	return (IPTR)result;
}

static IPTR TalkWindowUpdateTabContact(Class *cl, Object *obj, struct TKWP_UpdateTabContact *msg)
{
	struct TabEntry *tab;

	if((tab = FindTabEntryByEntryData(cl, obj, msg->entryid, msg->pluginid)))
	{
		if(tab->list_entry != NULL)
		{
			STRPTR temp = NULL;
			struct Picture *av_temp = NULL;

			temp = tab->entry.name;
			tab->entry.name = StrNew(tab->list_entry->name);
			if(temp) StrFree(temp);

			temp = tab->entry.nickname;
			tab->entry.nickname = StrNew(tab->list_entry->nickname);
			if(temp) StrFree(temp);

			temp = tab->entry.firstname;
			tab->entry.firstname = StrNew(tab->list_entry->firstname);
			if(temp) StrFree(temp);

			temp = tab->entry.lastname;
			tab->entry.lastname = StrNew(tab->list_entry->lastname);
			if(temp) StrFree(temp);

			temp = tab->entry.groupname;
			tab->entry.groupname = StrNew(tab->list_entry->groupname);
			if(temp) StrFree(temp);

			temp = tab->entry.statusdesc;
			tab->entry.statusdesc = StrNew(tab->list_entry->statusdesc);
			if(temp) StrFree(temp);

			temp = tab->entry.city;
			tab->entry.city = StrNew(tab->list_entry->city);
			if(temp) StrFree(temp);

			temp = tab->entry.birthyear;
			tab->entry.birthyear = StrNew(tab->list_entry->birthyear);
			if(temp) StrFree(temp);

			av_temp = tab->entry.avatar;
			tab->entry.avatar = CopyPicture(tab->list_entry->avatar);
			if(av_temp) FreePicture(av_temp);

			tab->entry.gender = tab->list_entry->gender;
			tab->entry.unread = tab->list_entry->unread;
			tab->entry.status = tab->list_entry->status;

		}
		set(tab->title, TTA_ContactName, ContactNameLoc(tab->entry));
		set(tab->title, TTA_Status, tab->entry.status);
		DoMethod(tab->tab, TTBM_RedrawInfoBlock);
	}

	return (IPTR)tab;
}

static IPTR TalkWindowUpdateTabContactStatus(Class *cl, Object *obj, struct TKWP_UpdateTabContactStatus *msg)
{
	struct TabEntry *tab;

	if((tab = (struct TabEntry*) DoMethod(obj, TKWM_UpdateTabContact, msg->entryid, msg->pluginid)))
		return (IPTR)DoMethod(tab->tab, TTBM_NewStatus);

	return (IPTR)0;
}

static IPTR TalkWindowUpdateTabWriteLamp(Class *cl, Object *obj, struct TKWP_UpdateTabWriteLamp *msg)
{
	struct TabEntry *tab;

	if((tab = FindTabEntryByEntryData(cl, obj, msg->entryid, msg->pluginid)))
	{
		if(msg->length == 0)
			return (IPTR) DoMethod(tab->tab, TTBM_CleanLampState);
		else
			return (IPTR) DoMethod(tab->tab, TTBM_SetLampState);
	}

	return (IPTR)0;
}

static IPTR TalkWindowAcceptBeacon(Class *cl, Object *obj, struct TKWP_AcceptBeacon *msg)
{
	if(msg->notify_result == NOTIFICATIONRESULT_CONFIRMED)
		DoMethod(obj, TKWM_OpenOnTab, FindTabEntryByUniqueId(cl, obj, msg->tab_unique_id));

	return (IPTR)0;
}

static IPTR TalkWindowShowPicture(Class *cl, Object *obj, struct TKWP_ShowPicture *msg)
{
	struct TalkWindowData *d = INST_DATA(cl, obj);
	struct TabEntry *tab;
	ENTER();

	if(msg->pic == NULL)
		return (IPTR)0;

	if(!(tab = FindTabEntryByEntryData(cl, obj, msg->entryid, msg->pluginid)))
		tab = (struct TabEntry*) TalkWindowCreateNewTab(cl, obj, msg->pluginid, msg->entryid, TRUE);

	if(tab != NULL)
	{
		/* got new picture, put it to tab, no fight for attention */
		switch(msg->flag)
		{
			case MSG_FLAG_NORMAL:
				DoMethod(tab->tab, TTBM_PutPicture, ContactNameLoc(tab->entry), msg->timestamp, msg->pic, msg->pic_size);
			break;

			case MSG_FLAG_MULTILOGON:
				DoMethod(tab->tab, TTBM_PutPicture, NULL, msg->timestamp, msg->pic, msg->pic_size);
			break;
		}

		/* if window is not active at all or we are in other tab set unread flag */
		if((tab->list_entry != NULL && (xget(obj, MUIA_Window_Activate) != TRUE || (xget(d->page_group, MUIA_Group_ActivePage) != tab->tab_id))))
		{
			if(msg->flag != MSG_FLAG_MULTILOGON) /* we don't want to notify when it's our meesage (but from other client) */
			{
				/* set unread state on list and screenbar */
				if(tab->list_entry->unread == FALSE)
				{
					tab->list_entry->unread = TRUE;

					DoMethod(obj, TKWM_UpdateTabContact, tab->entry.entryid, tab->entry.pluginid);
					DoMethod(findobj(USD_CONTACTS_LIST, _app(obj)), CLSM_Sort);
					set(_app(obj), APPA_ScreenbarUnread, TRUE); /* have to be after sort! APPM_ScreenbarUnread rely on sorting... */
				}

				DoMethod(_app(obj), APPM_NotifyBeacon, BEACON_PICTURE, ContactNameLoc(tab->entry), TRUE, obj, TKWM_AcceptBeacon, tab->tab_unique_id);
				set(tab->title, TTA_Unread, TRUE);
			}
			else if(tab->list_entry->unread == TRUE) /* after receive multilogon message set unread to false (we read this on other session) */
			{
				tab->list_entry->unread = FALSE;
				DoMethod(obj, TKWM_UpdateTabContact, tab->entry.entryid, tab->entry.pluginid);
				DoMethod(findobj(USD_CONTACTS_LIST, _app(obj)), CLSM_Sort);
				set(_app(obj), APPA_ScreenbarUnread, TRUE); /* have to be after sort! APPM_ScreenbarUnread rely on sorting... */
				set(tab->title, TTA_Unread, FALSE);
			}
		}
	}

	LEAVE();
	return (IPTR)1;
}

static IPTR TalkWindowGetTab(Class *cl, Object *obj, struct TKWP_GetTab *msg)
{
	struct TalkWindowData *d = INST_DATA(cl, obj);
	struct TabEntry *e;

	*msg->entry = NULL;

	if(msg->position == TKWV_ActiveTab)
		msg->position = xget(d->page_group, MUIA_Group_ActivePage);

	if((e = FindTabEntryById(cl, obj, msg->position)))
	{
		if(msg->entry)
			*msg->entry = e->tab;
	}

	return (IPTR)0;
}

static IPTR TalkWindowSendMessage(Class *cl, Object *obj, struct TKWP_SendMessage *msg)
{
	struct TabEntry *tab;
	ENTER();

	if(msg->message == NULL)
		return (IPTR)0;

	if(!(tab = FindTabEntryByEntryData(cl, obj, msg->entryid, msg->pluginid)))
		tab = (struct TabEntry*) TalkWindowCreateNewTab(cl, obj, msg->pluginid, msg->entryid, TRUE);

	if(tab != NULL)
	{
		DoMethod(tab->tab, TTBM_SendMessage, msg->message, TRUE, NULL);
	}

	LEAVE();
	return (IPTR)1;
}

static IPTR TalkWindowShowInvite(Class *cl, Object *obj, struct TKWP_ShowInvite *msg)
{
	struct TalkWindowData *d = INST_DATA(cl, obj);
	struct TabEntry *tab;
	ENTER();

	if((tab = FindTabEntryByEntryData(cl, obj, msg->entryid, msg->pluginid)))
	{
		tprintf("tab exist, found %lp\n", tab);
	}
	else
	{
		tprintf("tab not found, creating new...\n");
		tab = (struct TabEntry*) TalkWindowCreateNewTab(cl, obj, msg->pluginid, msg->entryid, FALSE);
	}

	if(tab != NULL)
	{
		DoMethod(tab->tab, TTBM_PutInvite, msg->pluginid, msg->entryid, msg->timestamp);

		/* if window is not active at all or we are in other tab set unread flag */
		if((tab->list_entry != NULL && (xget(obj, MUIA_Window_Activate) != TRUE || (xget(d->page_group, MUIA_Group_ActivePage) != tab->tab_id))))
		{
			/* set unread state on list and screenbar */
			if(tab->list_entry->unread == FALSE)
			{
				tab->list_entry->unread = TRUE;

				DoMethod(obj, TKWM_UpdateTabContact, tab->entry.entryid, tab->entry.pluginid);
				DoMethod(findobj(USD_CONTACTS_LIST, _app(obj)), CLSM_Sort);
				set(_app(obj), APPA_ScreenbarUnread, TRUE); /* have to be after sort! APPM_ScreenbarUnread rely on sorting... */
			}

			DoMethod(_app(obj), APPM_NotifyBeacon, BEACON_INVITE, ContactNameLoc(tab->entry), TRUE, obj, TKWM_AcceptBeacon, tab->tab_unique_id);
			set(tab->title, TTA_Unread, TRUE);
		}
	}

	LEAVE();
	return (IPTR)1;
}

static IPTR TalkWindowDeleteTabByObject(Class *cl, Object *obj, struct TKWP_DeleteTabByObject *msg)
{
	struct TalkWindowData *d = INST_DATA(cl, obj);
	Object *parent = (Object*)xget(d->title, MUIA_Parent);
	struct TabEntry *temp = d->tabs_list_start;
	ENTER();

	while(temp != NULL)
	{
		if(temp->tab == msg->tab) break;
		temp = temp->next;
	}

	if(temp != NULL)
	{
		DoMethod(parent, MUIM_Group_InitChange);
		DoMethod(d->title, MUIM_Group_InitChange);

		DoMethod(d->title, OM_REMMEMBER, temp->title);
		DoMethod(parent, OM_REMMEMBER, temp->tab);

		DoMethod(d->title, MUIM_Group_ExitChange);
		DoMethod(parent, MUIM_Group_ExitChange);

		DoMethod(obj, TKWM_DeleteTab, temp->title);
	}

	LEAVE();
	return (IPTR)1;
}

static IPTR TalkWindowDispatcher(VOID)
{
	Class *cl = (Class*)REG_A0;
	Object *obj = (Object*)REG_A2;
	Msg msg = (Msg)REG_A1;

	switch (msg->MethodID)
	{
		case OM_NEW: return (TalkWindowNew(cl, obj, (struct opSet*)msg));
		case OM_DISPOSE: return (TalkWindowDispose(cl, obj, msg));
		case TKWM_ShowMessage: return (TalkWindowShowMessage(cl, obj, (struct TKWP_ShowMessage*)msg));
		case TKWM_DeleteTab: return(TalkWindowDeleteTab(cl, obj, (struct TKWP_DeleteTab*)msg));
		case TKWM_ActivateTab: return(TalkWindowActivateTab(cl, obj, (struct TKWP_ActivateTab*)msg));
		case TKWM_OpenOnTab: return(TalkWindowOpenOnTab(cl, obj, (struct TKWP_OpenOnTab*)msg));
		case TKWM_OpenOnTabById: return(TalkWindowOpenOnTabById(cl, obj, (struct TKWP_OpenOnTabById*)msg));
		case TKWM_CreateTabsMenuStrip: return(TalkWindowCreateTabsMenuStrip(cl, obj));
		case TKWM_UpdateTabContact: return(TalkWindowUpdateTabContact(cl, obj, (struct TKWP_UpdateTabContact*)msg));
		case TKWM_UpdateTabContactStatus: return(TalkWindowUpdateTabContactStatus(cl, obj, (struct TKWP_UpdateTabContactStatus*)msg));
		case TKWM_UpdateTabWriteLamp: return(TalkWindowUpdateTabWriteLamp(cl, obj, (struct TKWP_UpdateTabWriteLamp*)msg));
		case TKWM_AcceptBeacon: return(TalkWindowAcceptBeacon(cl, obj, (struct TKWP_AcceptBeacon*)msg));
		case TKWM_ShowPicture: return(TalkWindowShowPicture(cl, obj, (struct TKWP_ShowPicture*)msg));
		case TKWM_GetTab: return(TalkWindowGetTab(cl, obj, (struct TKWP_GetTab*)msg));
		case TKWM_SendMessage: return(TalkWindowSendMessage(cl, obj, (struct TKWP_SendMessage*)msg));
		case TKWM_ShowInvite: return(TalkWindowShowInvite(cl, obj, (struct TKWP_ShowInvite*)msg));
		case TKWM_DeleteTabByObject: return(TalkWindowDeleteTabByObject(cl, obj, (struct TKWP_DeleteTabByObject*)msg));
		default: return (DoSuperMethodA(cl, obj, msg));
	}
}

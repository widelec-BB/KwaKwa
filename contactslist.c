/*
 * Copyright (c) 2012-2022 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <proto/exec.h>
#include <exec/nodes.h>
#include <exec/lists.h>
#include <proto/dos.h>
#include <proto/ezxml.h>
#include <proto/asl.h>
#include <proto/graphics.h>
#include <libvstring.h>
#include <proto/locale.h>
#include <proto/openurl.h>
#include <proto/wb.h>
#include <libraries/locale.h>
#include <devices/rawkeycodes.h>
#include <libraries/gadtools.h>
#include <proto/cybergraphics.h>

#include "globaldefines.h"
#include "locale.h"
#include "application.h"
#include "prefswindow.h"
#include "contactslist.h"
#include "support.h"
#include "lexer.h"
#include "talkwindow.h"
#include "editconwindow.h"
#include "logs.h"

#include "kwakwa_api/defs.h"
#include "kwakwa_api/pictures.h"

#ifndef ForeachNode
#define ForeachNode(list, node) \
for((node) = (struct ListEntry*)(((struct MinList*)(list))->mlh_Head); \
((struct MinNode*)(node))->mln_Succ; \
(node) = (struct ListEntry*)(((struct MinNode*)(node))->mln_Succ))
#endif /* ForeachNode */

#ifndef ForeachNodeSafe
#define ForeachNodeSafe(l,n,n2)  \
for (  \
	n = (void *)(((struct List *)(l))->lh_Head);  \
	(n2 = (void *)((struct Node *)(n))->ln_Succ);  \
	n = (void *)n2  \
)
#endif /* ForeachNodeSafe */

#define ForeachNodeBack(list, node) \
for((node) = (struct ListEntry*)(((struct MinList*)(list))->mlh_TailPred); \
((struct MinNode*)(node))->mln_Pred; \
(node) = (struct ListEntry*)(((struct MinNode*)(node))->mln_Pred))

#define AVATAR_RATIO (((DOUBLE)(xget(prefs_object(USD_PREFS_CONTACTSLIST_AVATARSIZE_SLIDER), MUIA_Slider_Level)))/100)

#define DRAW_NEW_SIZE 	(1 << 0)
#define DRAW_SCROLL 		(1 << 1)
#define DRAW_ACTIVE 		(1 << 2)

#define STATUS_PICTURE_HEIGHT 18
#define STATUS_PICTURE_WIDTH 15
#define SPACE_BETWEEN_NAME_AND_DESCRIPTION xget(prefs_object(USD_PREFS_CONTACTSLIST_SPACENAMEDESC_SLIDER), MUIA_Slider_Level)
#define SPACE_BETWEEN_ENTRIES xget(prefs_object(USD_PREFS_CONTACTSLIST_SPACEENTRIES_SLIDER), MUIA_Slider_Level)

#define CONTACTS_LIST_PATH CACHE_DIR GUI_DIR "contactslist.cfg"

/* context menu */
#define CONLIST_CMENU_EDIT        11
#define CONLIST_CMENU_DELETE      12
#define CONLIST_CMENU_GO_TO_LINK  13
#define CONLIST_CMENU_COPY_DESC   14
#define CONLIST_CMENU_OPEN_TALK   15
#define CONLIST_CMENU_SEND_FTP    16
#define CONLIST_CMENU_OPEN_LOG    17

/* DIRTY HACK */
struct CustomRenderInfo
{
	char dummy[256];
	struct TextFont *mri_Font[MUIV_Font_Count];
};

static inline IPTR ChangeFont(Object *obj, LONG font)
{
	struct CustomRenderInfo *cri = (struct CustomRenderInfo *)muiRenderInfo(obj);
	IPTR result = (IPTR)_font(obj);

	_font(obj) = cri->mri_Font[-font];

	return result;
}

/* DIRTY HACK END */

static LONG TotalHeight(Class *cl, Object *obj);
struct MUI_CustomClass *ContactsListClass;
static IPTR ContactsListDispatcher(void);
const struct EmulLibEntry ContactsListGate = {TRAP_LIB, 0, (void(*)(void))ContactsListDispatcher};

//+ Methods args
struct CLSP_InsertSingle {ULONG MethodID; APTR entry; LONG pos;};
struct CLSP_Remove {ULONG MethodID; LONG pos;};
struct CLSP_GetEntry {ULONG MethodID; LONG pos; APTR *entry;};
struct CLSP_Compare {ULONG MethodID; APTR entry1; APTR entry2;};
struct CLSP_DrawContextMenu {ULONG MethodID; ULONG x; ULONG y;};
struct CLSP_SelectEntryByName {ULONG MethodID; ULONG next;};
struct CLSP_RemoveEntry {ULONG MethodID; struct ContactEntry *entry; ULONG confirm;};
struct CLSP_OpenLogFile {ULONG MethodID; STRPTR log_file_name;};
struct CLSP_FindEntry {ULONG MethodID; LONG mode; struct ContactEntry *entry; struct ContactEntry *start;};
//-

struct ListEntry
{
	struct MinNode node;
	/* NOTE: adding anything here (between "node" and "data") will break CLSM_FindEntryByName! */
	struct ContactEntry data;
	struct EntryDisplayData
	{
		ULONG pixel_height;
		LONG pixel_start, virtual_pixel_start;
	} display;
};

struct ContactsListData
{
	APTR memory_pool;
	struct MinList data_list;
	ULONG list_entries_no;

	Object *prop_object, *search_string;

	ULONG draw_flag;
	LONG old_height, old_width;
	LONG old_first;

	struct MUI_EventHandlerNode handler;

	struct ListEntry *prev_active_entry, *active_entry;

	struct Locale *locale;
};

struct MUI_CustomClass *CreateContactsListClass(void)
{
	struct MUI_CustomClass *cl;

	cl = MUI_CreateCustomClass(NULL, MUIC_Area, NULL, sizeof(struct ContactsListData), (APTR)&ContactsListGate);
	ContactsListClass = cl;
	return cl;
}

void DeleteContactsListClass(void)
{
	if (ContactsListClass) MUI_DeleteCustomClass(ContactsListClass);
}

static VOID ContactsListNotifications(Class *cl, Object *obj)
{
	struct ContactsListData *d = INST_DATA(cl, obj);

	DoMethod(d->prop_object, MUIM_Notify, MUIA_Prop_First, MUIV_EveryTime, obj, 1, CLSM_Scroll);

	DoMethod(d->search_string, MUIM_Notify, MUIA_Textinput_Acknowledge, MUIV_EveryTime, MUIV_Notify_Parent, 3, MUIM_Set, MUIA_ShowMe, FALSE);
	DoMethod(d->search_string, MUIM_Notify, MUIA_Textinput_AdvanceOnCR, MUIV_EveryTime, MUIV_Notify_Parent, 3, MUIM_Set, MUIA_ShowMe, FALSE);
	DoMethod((Object*)xget(d->search_string, MUIA_Parent), MUIM_Notify, MUIA_ShowMe, FALSE, d->search_string, 3, MUIM_Set, MUIA_Textinput_Contents, NULL);
	DoMethod(d->search_string, MUIM_Notify, MUIA_Textinput_Contents, MUIV_EveryTime, obj, 2, CLSM_SelectEntryByName, FALSE);

	DoMethod(obj, MUIM_Notify, CLSA_Active, MUIV_EveryTime, obj, 1, CLSM_DrawActive);

	prefs_changed(obj, CLSM_RedrawAll);

	DoMethod(prefs_object(USD_PREFS_CONTACTSLIST_SHOWAVATARS_CHECK), MUIM_Notify, MUIA_Selected, MUIV_EveryTime, obj, 1,
	 CLSM_RedrawAll);

	DoMethod(prefs_object(USD_PREFS_CONTACTSLIST_AVATARSIZE_SLIDER), MUIM_Notify, MUIA_Slider_Level, MUIV_EveryTime, obj, 1,
	 CLSM_RedrawAll);

	DoMethod(prefs_object(USD_PREFS_CONTACTSLIST_SPACEENTRIES_SLIDER), MUIM_Notify, MUIA_Slider_Level, MUIV_EveryTime, obj, 1,
	 CLSM_RedrawAll);

	DoMethod(prefs_object(USD_PREFS_CONTACTSLIST_SPACENAMEDESC_SLIDER), MUIM_Notify, MUIA_Slider_Level, MUIV_EveryTime, obj, 1,
	 CLSM_RedrawAll);

	DoMethod(prefs_object(USD_PREFS_CONTACTSLIST_ACTIVEENTRY_COLOR), MUIM_Notify, MUIA_Pendisplay_Spec, MUIV_EveryTime, obj, 1,
	 CLSM_RedrawAll);
}

static IPTR ContactsListNew(Class *cl, Object *obj, struct opSet *msg)
{
	struct TagItem *prop_tag, *search_tag;
	ENTER();

	prop_tag = FindTagItem(CLSA_Prop_Gadget, msg->ops_AttrList);
	search_tag = FindTagItem(CLSA_Search_String, msg->ops_AttrList);

	obj = DoSuperNew(cl, obj,
		MUIA_ObjectID, USD_CONTACTS_LIST,
		MUIA_UserData, USD_CONTACTS_LIST,
		MUIA_Frame, MUIV_Frame_ReadList,
		MUIA_Background, MUII_ReadListBack,
		MUIA_Font, MUIV_Font_List,
		MUIA_DoubleBuffer, TRUE,
	TAG_MORE, msg->ops_AttrList);

	if(obj)
	{
		struct ContactsListData *d = INST_DATA(cl, obj);

		if((d->locale = OpenLocale(NULL)))
		{
			if((d->memory_pool = CreatePool(MEMF_ANY, sizeof(struct ListEntry), sizeof(struct ListEntry))))
			{
				if((d->prop_object = (Object*) prop_tag->ti_Data))
				{
					if((d->search_string = (Object*) search_tag->ti_Data))
					{
						NEWLIST((struct List*)&d->data_list);

						ContactsListNotifications(cl, obj);

						LEAVE();
						return (IPTR)obj;
					}
				}
			}
		}
	}
	LEAVE();
	CoerceMethod(cl, obj, OM_DISPOSE);
	return (IPTR)NULL;
}

static IPTR ContactsListDispose(Class *cl, Object *obj, Msg msg)
{
	struct ContactsListData *d = INST_DATA(cl, obj);

	if(d->memory_pool) DeletePool(d->memory_pool);

	return 0;
}

static IPTR ContactsListSet(Class *cl, Object *obj, struct opSet *msg)
{
	struct ContactsListData *d = INST_DATA(cl, obj);
	int tagcount = 0;
	struct TagItem *tag, *tagptr = msg->ops_AttrList;

	while ((tag = NextTagItem(&tagptr)))
	{
		switch(tag->ti_Tag)
		{
			case CLSA_Active:
				d->prev_active_entry = d->active_entry;
				d->active_entry = (struct ListEntry*)tag->ti_Data;
			tagcount++;
		}
	}

	tagcount += DoSuperMethodA(cl, obj, (Msg)msg);
	return tagcount;
}

static IPTR ContactsListGet(Class *cl, Object *obj, struct opGet *msg)
{
	struct ContactsListData *d = INST_DATA(cl, obj);
	int rv = FALSE;

	switch (msg->opg_AttrID)
	{
		case CLSA_List_Entries:
			*msg->opg_Storage = d->list_entries_no;
		return TRUE;

		case CLSA_Active:
			*msg->opg_Storage = (ULONG)d->active_entry;
		return TRUE;

		default: rv = (DoSuperMethodA(cl, obj, (Msg)msg));
	}

	return rv;
}

static IPTR ContactsListInsertSingle(Class *cl, Object *obj, struct CLSP_InsertSingle *msg)
{
	struct ContactsListData *d = INST_DATA(cl, obj);
	IPTR result = 0;
	struct ListEntry *newentry;
	struct ContactEntry *oldentry = (struct ContactEntry *)msg->entry;

	if((newentry = AllocPooled(d->memory_pool, sizeof(struct ListEntry))))
	{
		if((newentry->data.entryid = StrNew(oldentry->entryid)))
		{
			newentry->data.pluginid = oldentry->pluginid;
			newentry->data.status = oldentry->status;
			newentry->data.gender = oldentry->gender;
			newentry->data.unread = oldentry->unread;
			newentry->data.name = StrNew(oldentry->name);
			newentry->data.nickname = StrNew(oldentry->nickname);
			newentry->data.firstname = StrNew(oldentry->firstname);
			newentry->data.lastname = StrNew(oldentry->lastname);
			newentry->data.groupname = StrNew(oldentry->groupname);
			newentry->data.statusdesc = StrNew(oldentry->statusdesc);
			newentry->data.city = StrNew(oldentry->city);
			newentry->data.birthyear = StrNew(oldentry->birthyear);
			newentry->data.avatar = NULL;

			switch(msg->pos)
			{
				case CLSV_Insert_Top:
					AddHead((struct List*)&d->data_list, (struct Node*)newentry);
					result = (IPTR)&newentry->data;
					d->list_entries_no++;
				break;

				case CLSV_Insert_Bottom:
					AddTail((struct List*)&d->data_list, (struct Node*)newentry);
					result = (IPTR)&newentry->data;
					d->list_entries_no++;
				break;

				case CLSV_Insert_Sorted: /* know, lame */
					AddHead((struct List*)&d->data_list, (struct Node*)newentry);
					result = (IPTR)&newentry->data;
					d->list_entries_no++;
					DoMethod(obj, CLSM_Sort);
				break;

				default:
					tprintf("CLSM_InsertSingle: %ld not implemented!\n", msg->pos);
			}
		}
	}

	set(obj, CLSA_List_Entries, d->list_entries_no);

	d->draw_flag |= DRAW_NEW_SIZE;

	MUI_Redraw(obj, MADF_DRAWOBJECT);

	return result;
}

static IPTR ContactsListRemove(Class *cl, Object *obj, struct CLSP_Remove *msg)
{
	struct ContactsListData *d = INST_DATA(cl, obj);
	struct ListEntry *entry = NULL;

	switch(msg->pos)
	{
		case CLSV_Remove_First:
			if(MUI_Request(_app(obj), _win(obj), 0L, APP_NAME, GetString(MSG_CONTACTLIST_DELETE_GADGETS), GetString(MSG_CONTACTLIST_DELETE_REQ), ContactNameLoc(((struct ListEntry*)(d->data_list.mlh_Head))->data)) == 1)
				entry = (struct ListEntry*) RemHead((struct List*)&d->data_list);
		break;

		case CLSV_Remove_Last:
			if(MUI_Request(_app(obj), _win(obj), 0L, APP_NAME, GetString(MSG_CONTACTLIST_DELETE_GADGETS), GetString(MSG_CONTACTLIST_DELETE_REQ), ContactNameLoc(((struct ListEntry*)(d->data_list.mlh_Tail))->data)) == 1)
				entry = (struct ListEntry*) RemTail((struct List*)&d->data_list);
		break;

		case CLSV_Remove_Active:
			if(MUI_Request(_app(obj), _win(obj), 0L, APP_NAME, GetString(MSG_CONTACTLIST_DELETE_GADGETS), GetString(MSG_CONTACTLIST_DELETE_REQ), ContactNameLoc(d->active_entry->data)) == 1)
			{
				entry = d->active_entry;
				set(obj, CLSA_Active, entry->node.mln_Succ);
				Remove((struct Node*)entry);
			}
		break;

		default:
			tprintf("CLSM_Remove: %ld not implemented!\n", msg->pos);
	}

	if(entry)
	{
		DoMethod(_app(obj), APPM_RemoveNotify, entry->data.pluginid, entry->data.entryid, entry->data.status);

		if(entry->data.entryid) StrFree(entry->data.entryid);
		if(entry->data.name) StrFree(entry->data.name);
		if(entry->data.nickname) StrFree(entry->data.nickname);
		if(entry->data.firstname) StrFree(entry->data.firstname);
		if(entry->data.lastname) StrFree(entry->data.lastname);
		if(entry->data.groupname) StrFree(entry->data.groupname);
		if(entry->data.statusdesc) StrFree(entry->data.statusdesc);
		if(entry->data.city) StrFree(entry->data.city);
		if(entry->data.avatar) FreePicture(entry->data.avatar);

		FreePooled(d->memory_pool, entry, sizeof(struct ListEntry));
		d->list_entries_no--;
		set(obj, CLSA_List_Entries, d->list_entries_no);

		d->draw_flag |= DRAW_NEW_SIZE;

		MUI_Redraw(obj, MADF_DRAWOBJECT);
		return (IPTR)1;
	}

	return (IPTR)0;
}


static IPTR ContactsListGetEntry(Class *cl, Object *obj, struct CLSP_GetEntry *msg)
{
	struct ContactsListData *d = INST_DATA(cl, obj);
	struct ListEntry *result = NULL;
	ULONG i = 0;

	*msg->entry = NULL;

	if(msg->pos == CLSV_GetEntry_Active)
	{
		if(d->active_entry)
			*msg->entry = &d->active_entry->data;
		else
			*msg->entry = NULL;
		return (IPTR)*msg->entry;
	}

	if(IsListEmpty((struct List*)&d->data_list))
		return (IPTR)0;

	ForeachNode((&d->data_list), (result))
	{
		if(i == msg->pos)
		{
			*msg->entry = &result->data;
			break;
		}
		i++;
	}
	return (IPTR)1;
}

static IPTR ContactsListClear(Class *cl, Object *obj)
{
	struct ContactsListData *d = INST_DATA(cl, obj);
	struct ListEntry *entry;

	while((entry = (struct ListEntry*) RemHead((struct List*)&d->data_list)))
	{
		DoMethod(_app(obj), APPM_RemoveNotify, entry->data.pluginid, entry->data.entryid, entry->data.status);
		if(entry->data.entryid) StrFree(entry->data.entryid);
		if(entry->data.name) StrFree(entry->data.name);
		if(entry->data.nickname) StrFree(entry->data.nickname);
		if(entry->data.firstname) StrFree(entry->data.firstname);
		if(entry->data.lastname) StrFree(entry->data.lastname);
		if(entry->data.groupname) StrFree(entry->data.groupname);
		if(entry->data.statusdesc) StrFree(entry->data.statusdesc);
		if(entry->data.avatar) FreePicture(entry->data.avatar);

		FreePooled(d->memory_pool, entry, sizeof(struct ListEntry));
	}

	NEWLIST(&d->data_list);

	d->list_entries_no = 0;
	set(obj, CLSA_List_Entries, d->list_entries_no);
	set(obj, CLSA_Active, NULL);
	d->prev_active_entry = NULL;

	d->draw_flag |= DRAW_NEW_SIZE;

	MUI_Redraw(obj, MADF_DRAWOBJECT);

	return (IPTR)1;
}

static ezxml_t ContactsListToEzxml_t(Object *obj)
{
	ezxml_t list;

	if((list = ezxml_new_d("ContactList")))
	{
		ULONG i;
		for(i = 0;;i++)
		{
			ezxml_t entry_xml;
			struct ContactEntry *entry;
			STRPTR pluginid;

			DoMethod(obj, CLSM_GetEntry, i, &entry);
			if(!entry) break;

			if((pluginid = FmtNew("%ld", entry->pluginid)))
			{
				entry_xml = ezxml_add_child_d(list, "Contact", list->off);

				ezxml_set_txt_d(entry_xml, entry->entryid);

				ezxml_set_attr_d(entry_xml, "PluginID", pluginid);
				if(entry->name) ezxml_set_attr_d(entry_xml, "Name", entry->name);
				if(entry->nickname) ezxml_set_attr_d(entry_xml, "NickName", entry->nickname);
				if(entry->firstname) ezxml_set_attr_d(entry_xml, "FirstName", entry->firstname);
				if(entry->lastname) ezxml_set_attr_d(entry_xml, "LastName", entry->lastname);
				if(entry->groupname) ezxml_set_attr_d(entry_xml, "GroupName", entry->groupname);
				if(entry->birthyear) ezxml_set_attr_d(entry_xml, "Birthyear", entry->birthyear);
				if(entry->city) ezxml_set_attr_d(entry_xml, "City", entry->city);
				ezxml_set_attr_d(entry_xml, "Gender", entry->gender ? "1" : "0");

				FmtFree(pluginid);
			}
		}
	}

	return list;
}

static ULONG ImportXMLTxt(Object *obj, STRPTR xml)
{
	struct ContactEntry *entry;
	ULONG entries = 0;

	if((entry = AllocMem(sizeof(struct ContactEntry), MEMF_ANY | MEMF_CLEAR)))
	{
		ezxml_t list;
		if((list = ezxml_parse_str(xml, StrLen(xml))))
		{
			ezxml_t entry_xml;
			if((entry_xml = ezxml_child(list, "Contact")))
			{
				do
				{
					STRPTR plugin_tag = (STRPTR)ezxml_attr(entry_xml, "PluginID");

					if(plugin_tag == NULL)
					{
						entry->pluginid = GG_MODULE_ID;
					}
					else
					{
						if(StrToLong(plugin_tag, &entry->pluginid) == -1)
							continue;
					}

					entry->entryid = ezxml_txt(entry_xml);
					entry->name = (STRPTR)ezxml_attr(entry_xml, "Name");
					entry->nickname = (STRPTR)ezxml_attr(entry_xml, "NickName");
					entry->firstname = (STRPTR)ezxml_attr(entry_xml, "FirstName");
					entry->lastname = (STRPTR)ezxml_attr(entry_xml, "LastName");
					entry->groupname = (STRPTR)ezxml_attr(entry_xml, "GroupName");
					entry->city = (STRPTR)ezxml_attr(entry_xml, "City");
					entry->birthyear = (STRPTR)ezxml_attr(entry_xml, "Birthyear");
					if(ezxml_attr(entry_xml, "Gender"))
						entry->gender = *ezxml_attr(entry_xml, "Gender") == '1' ? GENDER_MALE : *ezxml_attr(entry_xml, "Gender") == '2' ? GENDER_FEMALE : GENDER_UNKNOWN;
					entry->status = 0;

					DoMethod(obj, CLSM_InsertSingle, entry, CLSV_Insert_Bottom);
					DoMethod(_app(obj), APPM_AddNotify, entry->pluginid, entry->entryid, 0);
					entries++;
				}while((entry_xml = ezxml_next(entry_xml)));
			}
			ezxml_free(list);
		}
		FreeMem(entry, sizeof(struct ContactEntry));
	}

	return entries;
}

static IPTR ContactsListImport(Class *cl, Object *obj, struct MUIP_Import *msg) /* left for compatibility with 1.x */
{
	struct ContactEntry *first;
	ULONG id = muiNotifyData(obj)->mnd_ObjectID;
	ENTER();

	DoMethod(obj, CLSM_GetEntry, 0, &first);

	if(first == NULL) /* load list only if list is empty */
	{
		if (id != 0)
		{
			STRPTR buffer;

			if((buffer = (STRPTR) DoMethod(msg->dataspace, MUIM_Dataspace_Find, id)))
			{
				if(ImportXMLTxt(obj, buffer))
					DoMethod(obj, CLSM_SaveList);
			}
		}
	}

	LEAVE();
	return (IPTR)0;
}

static IPTR ContactsListSetAllUnavail(Class *cl, Object *obj)
{
	struct ContactEntry *con;
	ULONG i;

	for(i = 0;;i++)
	{
		DoMethod(obj, CLSM_GetEntry, i, &con);
		if(!con) break;
		con->status = KWA_STATUS_NOT_AVAIL;
		if(con->statusdesc)
		{
			StrFree(con->statusdesc);
			con->statusdesc = NULL; /* no longer valid */
		}
 	}

	DoMethod(obj, CLSM_Sort);

	return (IPTR)1;
}

static IPTR ContactsListCheckUnread(Class *cl, Object *obj)
{
	struct ContactEntry *con;
	ULONG no = 0, i;
	STRPTR result = NULL;

	for(i = 0;;i++)
	{
		DoMethod(obj, CLSM_GetEntry, i, &con);
		if(!con) break;
		if(con->unread)
		{
			if(no == 0)
			{
				result = FmtNew("%s", ContactName(con));
			}
			else
			{
				STRPTR old = result;

				if(old)
				{
					result = FmtNew("%s, %s", old, ContactName(con));
					FmtFree(old);
				}
			}
			no++;
		}
 	}
	return (IPTR)result;
}


static IPTR ContactsListExportToFile(Class *cl, Object *obj)
{
	struct FileRequester *freq;

	set(_app(obj), MUIA_Application_Sleep, TRUE);

	if((freq = MUI_AllocAslRequestTags(ASL_FileRequest, TAG_END)))
	{
		struct ClockData cd;
		STRPTR initialfile;
		STRPTR initialdrawer = LoadFile(CACHE_DIR GUI_DIR "list_export_asl.cfg", NULL);

		ActLocalTimeToClockData(&cd);

		initialfile = FmtNew("KwaKwa_%02ld_%02ld_%02ld.xml", (ULONG)cd.year, (ULONG)cd.month, (ULONG)cd.mday);

		if(MUI_AslRequestTags(freq,
			ASLFR_TitleText, GetString(MSG_CONTACTLIST_EXPORT_ASL_TITLE),
			ASLFR_PositiveText, GetString(MSG_CONTACTLIST_EXPORT_ASL_POSITIVE),
			ASLFR_InitialPattern, "#?.xml",
			ASLFR_DoPatterns, TRUE,
			ASLFR_RejectIcons, TRUE,
			ASLFR_DoSaveMode, TRUE,
			ASLFR_InitialFile, initialfile ? initialfile : (STRPTR)"",
			ASLFR_InitialDrawer, initialdrawer ? initialdrawer : (STRPTR)"",
			TAG_END))
		{
			UBYTE location[500];
			BPTR fh;

			FmtNPut(location, CACHE_DIR GUI_DIR "list_export_asl.cfg", sizeof(location));
			SaveFile(location, freq->fr_Drawer, StrLen(freq->fr_Drawer));

			StrNCopy(freq->fr_Drawer, (STRPTR)location, sizeof(location));
			AddPart((STRPTR)location, freq->fr_File, sizeof(location));

			if(freq->fr_File && !StrIStr(freq->fr_File, ".xml"))
				StrCat(".xml", location);

			if((fh = Open((STRPTR)location, MODE_NEWFILE)))
			{
				ezxml_t list;

				if((list = ContactsListToEzxml_t(obj)))
				{
					WriteEzxmlToFile(list, fh);
					ezxml_free(list);
				}

				Close(fh);
			}
		}

		if(initialfile)
			FmtFree(initialfile);

		if(initialdrawer)
			FreeVec(initialdrawer);

		MUI_FreeAslRequest(freq);
	}

	set(_app(obj), MUIA_Application_Sleep, FALSE);

	return (IPTR)1;
}


static IPTR ContactsListImportFromFile(Class *cl, Object *obj)
{
	struct FileRequester *freq;

	set(_app(obj), MUIA_Application_Sleep, TRUE);

	if((freq = MUI_AllocAslRequestTags(ASL_FileRequest, TAG_END)))
	{
		if(MUI_AslRequestTags(freq,
			ASLFR_TitleText, GetString(MSG_CONTACTLIST_IMPORT_ASL_TITLE),
			ASLFR_PositiveText, GetString(MSG_CONTACTLIST_IMPORT_ASL_POSITIVE),
			ASLFR_InitialPattern, "#?.xml",
			ASLFR_DoPatterns, TRUE,
			ASLFR_RejectIcons, TRUE,
			ASLFR_SleepWindow, TRUE,
			TAG_END))
		{
			UBYTE location[500];
			struct FileInfoBlock fib;
			BPTR fh;

			StrNCopy(freq->fr_Drawer, (STRPTR)location, 500);
			AddPart((STRPTR)location, freq->fr_File, 500);

			if((fh = Open((STRPTR)location, MODE_OLDFILE)))
			{
				STRPTR buffer;

				if((ExamineFH(fh, &fib)))
				{
					if((buffer = AllocMem(fib.fib_Size, MEMF_ANY | MEMF_CLEAR)) && (FRead(fh, (APTR)buffer, fib.fib_Size, 1) == 1))
					{
						ImportXMLTxt(obj, buffer);
						FreeMem(buffer, fib.fib_Size);
					}
				}
				Close(fh);
			}
		}
		MUI_FreeAslRequest(freq);
	}

	DoMethod(obj, CLSM_Sort);
	DoMethod(obj, CLSM_SaveList);
	set(_app(obj), MUIA_Application_Sleep, FALSE);

	return (IPTR)1;
}

static IPTR ContactsListCompare(Class *cl, Object *obj, struct CLSP_Compare *msg)
{
	struct ContactsListData *d = INST_DATA(cl, obj);
	struct ContactEntry *entry1 = (struct ContactEntry*)msg->entry1;
	struct ContactEntry *entry2 = (struct ContactEntry*)msg->entry2;

	if(entry1->unread == entry2->unread)
	{
		if((KWA_S_FFC(entry1->status) && KWA_S_FFC(entry2->status)) || (KWA_S_AVAIL(entry1->status) && KWA_S_AVAIL(entry2->status)) ||
			(KWA_S_BUSY(entry1->status) && KWA_S_BUSY(entry2->status)) || (KWA_S_DND(entry1->status) && KWA_S_DND(entry2->status)) ||
			(KWA_S_INVISIBLE(entry1->status) && KWA_S_INVISIBLE(entry2->status)) || (KWA_S_NAVAIL(entry1->status) && KWA_S_NAVAIL(entry2->status)))
		{
			if(d->locale)
				return StrnCmp(d->locale, ContactName(entry1), ContactName(entry2), -1, SC_COLLATE2);
			else
				return Stricmp(ContactName(entry1), ContactName(entry2));
		}
		else
		{
			if(KWA_S_FFC(entry1->status)) return (IPTR)-1;
			if(KWA_S_FFC(entry2->status)) return (IPTR)1;
			if(KWA_S_AVAIL(entry1->status)) return (IPTR)-1;
			if(KWA_S_AVAIL(entry2->status)) return (IPTR)1;
			if(KWA_S_BUSY(entry1->status)) return (IPTR)-1;
			if(KWA_S_BUSY(entry2->status)) return (IPTR)1;
			if(KWA_S_DND(entry1->status)) return (IPTR)-1;
			if(KWA_S_DND(entry2->status)) return (IPTR)1;
			if(KWA_S_INVISIBLE(entry1->status)) return (IPTR)-1;
			if(KWA_S_INVISIBLE(entry2->status)) return (IPTR)1;
			if(KWA_S_NAVAIL(entry1->status)) return (IPTR)-1;
			if(KWA_S_NAVAIL(entry2->status)) return (IPTR)1;
		}
	}
	else
	{
		if(entry1->unread) return -1;
		if(entry2->unread) return 1;
	}

	return 0;

}

static IPTR ContactsListSort(Class *cl, Object *obj)
{
	struct ContactsListData *d = INST_DATA(cl, obj);

	if(!IsListEmpty((struct List*)&d->data_list))
	{
		struct ListEntry *end;

		ForeachNodeBack(&d->data_list, end)
		{
			BOOL changed = FALSE;
			struct ListEntry *act, *next = NULL;

			ForeachNode(&d->data_list, act)
			{
				if(next != NULL) act = next;

				if(act == end) break;

				if(DoMethod(obj, CLSM_Compare, &act->data, &((struct ListEntry*)(act->node.mln_Succ))->data) == 1)
				{
					next = (struct ListEntry*)act->node.mln_Succ;

					SWAPNODES(act, act->node.mln_Succ);

					changed = TRUE;
				}
				else
				{
					next = NULL;
				}
			}
			if(!changed) break;
		}
		d->draw_flag |= DRAW_NEW_SIZE;
		MUI_Redraw(obj, MADF_DRAWOBJECT);
	}

	return (IPTR)1;
}

static LONG StatusDescHeight(Object *obj, STRPTR desc, LONG width)
{
	IPTR old_font = ChangeFont(obj, MUIV_Font_Tiny); // hack, change font
	LONG result = 0, words_in_line = 0;
	ULONG line_size;
	STRPTR line_end = desc, line_start = desc, prev_word_end = NULL, last_space = NULL;

	while(*line_end != 0x00)
	{
		if(*line_end == ' ')
		{
			line_size = DoMethod(obj, MUIM_TextDim, line_start, line_end - line_start, NULL, 0x00);

			if(words_in_line != 0 && (line_size & 0xFFFF) >= width)
			{
				ULONG prev_line_size = DoMethod(obj, MUIM_TextDim, line_start, prev_word_end - line_start, NULL, 0x00);

				result += prev_line_size >> 16; // add prev line height to desc height
				words_in_line = 0;
				line_start = prev_word_end + 1;
			}
			prev_word_end = line_end;
			words_in_line++;
		}
		line_end++;
	}

	line_size = DoMethod(obj, MUIM_TextDim, line_start, line_end - line_start, NULL, 0x00);


	if((line_size & 0xFFFF) >= width)
	{
		last_space = line_end;
		while(last_space >= line_start && *last_space != ' ') /* search for last ' ' in line */
			last_space--;
	}

	if(last_space != NULL && last_space > line_start && *last_space == ' ')
	{
		result += (line_size = DoMethod(obj, MUIM_TextDim, line_start, last_space - line_start, NULL, 0x00) >> 16);
		result += (line_size = DoMethod(obj, MUIM_TextDim, last_space + 1, line_end - last_space - 1, NULL, 0x00) >> 16);
	}
	else
	{
		result +=  line_size >> 16;
	}

	_font(obj) = (struct TextFont *) old_font; // hack, recover old font
	return result;
}


static LONG TotalHeight(Class *cl, Object *obj)
{
	struct ContactsListData *d = INST_DATA(cl, obj);
	struct ListEntry *act_entry;
	LONG act_height = 0, temp;
	BOOL avatars = (BOOL) xget(prefs_object(USD_PREFS_CONTACTSLIST_SHOWAVATARS_CHECK), MUIA_Selected);
	BOOL descs = (BOOL) xget(prefs_object(USD_PREFS_CONTACTSLIST_SHOWDESC_CHECK), MUIA_Selected);

	ForeachNode(&d->data_list, act_entry)
	{
		act_entry->display.virtual_pixel_start = act_height;
		act_entry->display.pixel_height = 0;

		temp = DoMethod(obj, MUIM_TextDim, ContactNameLoc(act_entry->data), StrLen(ContactNameLoc(act_entry->data)), NULL, 0x00);
		temp = temp >> 16; // name height

		act_entry->display.pixel_height += temp > STATUS_PICTURE_HEIGHT ? temp : STATUS_PICTURE_HEIGHT;

		if(descs && act_entry->data.statusdesc != NULL)
		{
			LONG space_for_desc = avatars && act_entry->data.avatar ? _mwidth(obj) - (act_entry->data.avatar->p_Width * AVATAR_RATIO) : _mwidth(obj);

			act_entry->display.pixel_height += SPACE_BETWEEN_NAME_AND_DESCRIPTION;
			act_entry->display.pixel_height += StatusDescHeight(obj, act_entry->data.statusdesc, space_for_desc);
		}

		if(avatars && act_entry->data.avatar && (act_entry->data.avatar->p_Height * AVATAR_RATIO) > act_entry->display.pixel_height) // avatar is bigger than everyting...
			act_entry->display.pixel_height = (act_entry->data.avatar->p_Height * AVATAR_RATIO);

		act_height += act_entry->display.pixel_height;

		if(act_entry->node.mln_Succ != NULL)
			act_height += SPACE_BETWEEN_ENTRIES;
	}

	return act_height;
}

static IPTR ContactsListAskMinMax(Class *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
	DoSuperMethodA(cl, obj, msg);

	msg->MinMaxInfo->MinWidth += 150;
	msg->MinMaxInfo->DefWidth += 200;
	msg->MinMaxInfo->MaxWidth += MUI_MAXMAX;

	msg->MinMaxInfo->MinHeight += 300;
	msg->MinMaxInfo->DefHeight += 400;
	msg->MinMaxInfo->MaxHeight += MUI_MAXMAX;

	return (IPTR)0;
}

static VOID DrawListPart(Class *cl, Object *obj, LONG start, LONG height, BOOL background)
{
	struct ContactsListData *d = INST_DATA(cl, obj);
	struct ListEntry *act_entry;
	APTR clip;
	BYTE buffer[1024];
	ULONG start_height = xget(d->prop_object, MUIA_Prop_First), virtual_height = 0;
	LONG act_height = 0;
	BOOL avatars = (BOOL) xget(prefs_object(USD_PREFS_CONTACTSLIST_SHOWAVATARS_CHECK), MUIA_Selected);
	BOOL descs = (BOOL) xget(prefs_object(USD_PREFS_CONTACTSLIST_SHOWDESC_CHECK), MUIA_Selected);

	clip = MUI_AddClipping(muiRenderInfo(obj), _mleft(obj), start, _mwidth(obj), height + 1);

	if(background == TRUE)
		DoMethod(obj, MUIM_DrawBackground, _mleft(obj), _mtop(obj), _mright(obj), _mbottom(obj), 0x00, 0x00, 0x00);

	ForeachNode(&d->data_list, act_entry)
	{
		act_entry->display.virtual_pixel_start = virtual_height;
		if(virtual_height + act_entry->display.pixel_height >= start_height && virtual_height < start_height + _mbottom(obj))
		{
			ULONG name_height;

			act_height = virtual_height - start_height;
			act_entry->display.pixel_start = act_height;

			if(act_entry == d->active_entry)
			{
				struct MUI_PenSpec *pen_spec;
				LONG active_pen;

				pen_spec = (struct MUI_PenSpec*)xget(prefs_object(USD_PREFS_CONTACTSLIST_ACTIVEENTRY_COLOR), MUIA_Pendisplay_Spec);
				active_pen = MUI_ObtainPen(muiRenderInfo(obj), pen_spec, 0);

				SetAPen(_rp(obj), MUIPEN(active_pen));
				RectFill(_rp(obj), _mleft(obj), _mtop(obj) + d->active_entry->display.pixel_start, _mright(obj), _mtop(obj) + d->active_entry->display.pixel_start + d->active_entry->display.pixel_height);
				MUI_ReleasePen(muiRenderInfo(obj), active_pen);
			}

			if(act_entry->data.unread)
				FmtNPut((STRPTR)buffer, "%ls %ls", sizeof(buffer), "\33I[4:PROGDIR:gfx/newmsg.mbr]", ContactNameLoc(act_entry->data));
			else
			{
				if(KWA_S_AVAIL(act_entry->data.status))
					FmtNPut((STRPTR)buffer, "%ls %ls", sizeof(buffer), "\33I[4:PROGDIR:gfx/available.mbr]", ContactNameLoc(act_entry->data));
				else if(KWA_S_BUSY(act_entry->data.status))
					FmtNPut((STRPTR)buffer, "%ls %ls", sizeof(buffer), "\33I[4:PROGDIR:gfx/away.mbr]", ContactNameLoc(act_entry->data));
				else if(KWA_S_FFC(act_entry->data.status))
					FmtNPut((STRPTR)buffer, "%ls %ls", sizeof(buffer), "\33I[4:PROGDIR:gfx/ffc.mbr]", ContactNameLoc(act_entry->data));
				else if(KWA_S_DND(act_entry->data.status))
					FmtNPut((STRPTR)buffer, "%ls %ls", sizeof(buffer), "\33I[4:PROGDIR:gfx/dnd.mbr]", ContactNameLoc(act_entry->data));
				else if(KWA_S_BLOCKED(act_entry->data.status))
					FmtNPut((STRPTR)buffer, "%ls %ls", sizeof(buffer), "\33I[4:PROGDIR:gfx/blocked.mbr]", ContactNameLoc(act_entry->data));
				else if(KWA_S_INVISIBLE(act_entry->data.status))
					FmtNPut((STRPTR)buffer, "%ls %ls", sizeof(buffer), "\33I[4:PROGDIR:gfx/invisible.mbr]", ContactNameLoc(act_entry->data));
				else
					FmtNPut((STRPTR)buffer, "%ls %ls", sizeof(buffer), "\33I[4:PROGDIR:gfx/unavailable.mbr]", ContactNameLoc(act_entry->data));
			}

			name_height = DoMethod(obj, MUIM_TextDim, (STRPTR)buffer, StrLen((STRPTR)buffer), NULL, 0x00) >> 16;

			DoMethod(obj, MUIM_Text, _mleft(obj), _mtop(obj) + act_height, _mwidth(obj), _mheight(obj), (STRPTR)buffer, StrLen((STRPTR)buffer), NULL, 0x00);

			if(descs && act_entry->data.statusdesc != NULL)
			{
				IPTR old_font = ChangeFont(obj, MUIV_Font_Tiny);
				LONG act_desc_height = act_height + name_height, words_in_line = 0, width = avatars && act_entry->data.avatar ? _mwidth(obj) - (act_entry->data.avatar->p_Width * AVATAR_RATIO) : _mwidth(obj);
				STRPTR line_end = act_entry->data.statusdesc, line_start = act_entry->data.statusdesc, prev_word_end = NULL, last_space = NULL;
				ULONG line_size;

				act_desc_height += SPACE_BETWEEN_NAME_AND_DESCRIPTION;

				while(*line_end != 0x00)
				{
					if(*line_end == ' ')
					{
						line_size = DoMethod(obj, MUIM_TextDim, line_start, line_end - line_start, NULL, 0x00);

						if(words_in_line != 0 && (line_size & 0xFFFF) >= width)
						{
							ULONG prev_line_size = DoMethod(obj, MUIM_TextDim, line_start, prev_word_end - line_start, NULL, 0x00);

							DoMethod(obj, MUIM_Text, _mleft(obj), _mtop(obj) + act_desc_height, _mwidth(obj), _mheight(obj), line_start, prev_word_end - line_start, NULL, 0x00);

							act_desc_height += prev_line_size >> 16;
							words_in_line = 0;
							line_start = prev_word_end + 1;
						}

						prev_word_end = line_end;
						words_in_line++;
					}
					line_end++;
				}

				line_size = DoMethod(obj, MUIM_TextDim, line_start, line_end - line_start, NULL, 0x00);

				if((line_size & 0xFFFF) >= width)
				{
					last_space = line_end;
					while(last_space >= line_start && *last_space != ' ') /* search for last ' ' in line */
						last_space--;
				}

				if(last_space != NULL && last_space > line_start && *last_space == ' ')
				{
					DoMethod(obj, MUIM_Text, _mleft(obj), _mtop(obj) + act_desc_height, _mwidth(obj), _mheight(obj), line_start, last_space - line_start, NULL, 0x00);

					act_desc_height += (line_size = DoMethod(obj, MUIM_TextDim, line_start, last_space - line_start, NULL, 0x00) >> 16);

					DoMethod(obj, MUIM_Text, _mleft(obj), _mtop(obj) + act_desc_height, _mwidth(obj), _mheight(obj), last_space + 1, line_end - last_space - 1, NULL, 0x00);

					act_desc_height += (line_size = DoMethod(obj, MUIM_TextDim, last_space + 1, line_end - last_space - 1, NULL, 0x00) >> 16);
				}
				else
				{
					DoMethod(obj, MUIM_Text, _mleft(obj), _mtop(obj) + act_desc_height, _mwidth(obj), _mheight(obj), line_start, line_end - line_start, NULL, 0x00);
					act_desc_height +=  line_size >> 16;
				}

				_font(obj) = (struct TextFont *) old_font;
			}

			if(avatars && act_entry->data.avatar != NULL)
			{
				DOUBLE ratio = AVATAR_RATIO;
				struct Picture *avatar = act_entry->data.avatar;

				ScalePixelArrayAlpha(avatar->p_Data, avatar->p_Width, avatar->p_Height, avatar->p_Width << 2, _rp(obj),
				 _mright(obj) - (avatar->p_Width * ratio), _mtop(obj) + act_height,
				 avatar->p_Width * ratio, avatar->p_Height * ratio, 0xFFFFFFFF);
			}

			act_height += act_entry->display.pixel_height;
		}
		else
		{
			act_entry->display.pixel_start = -1; /* not visible */
		}

		virtual_height += act_entry->display.pixel_height + SPACE_BETWEEN_ENTRIES;
	}
	MUI_RemoveClipping(muiRenderInfo(obj), clip);
}

static IPTR ContactsListDraw(Class *cl, Object *obj, struct MUIP_Draw *msg)
{
	struct ContactsListData *d = INST_DATA(cl, obj);

	DoSuperMethodA(cl, obj, msg);

	if(d->old_height != _mheight(obj) || d->old_width != _mwidth(obj))
		d->draw_flag |= DRAW_NEW_SIZE;

	if(d->draw_flag & DRAW_NEW_SIZE)
	{
		DoMethod(_app(obj), MUIM_Application_PushMethod, d->prop_object, 3, MUIM_Set, MUIA_Prop_Entries, TotalHeight(cl, obj));
		DoMethod(_app(obj), MUIM_Application_PushMethod, d->prop_object, 3, MUIM_Set, MUIA_Prop_Visible, _mheight(obj));
	}

	if(msg->flags & MADF_DRAWOBJECT)
	{
		DrawListPart(cl, obj, _mtop(obj), _mheight(obj), FALSE);
	}

	if(msg->flags & MADF_DRAWUPDATE)
	{
		if(d->draw_flag & DRAW_SCROLL)
		{
			LONG delta = d->old_first - xget(d->prop_object, MUIA_Prop_First);

			if(delta >= _mheight(obj) || delta <= -_mheight(obj))
			{
				set(obj, MUIA_DoubleBuffer, TRUE); /* can I set() here? */
				MUI_Redraw(obj, MADF_DRAWOBJECT);
			}
			else
			{
				if(delta > 0)
				{
					/* scroll bar goes up */
					ScrollWindowRasterNoFill(_window(obj), 0, -delta, _mleft(obj), _mtop(obj), _mright(obj), _mbottom(obj) + 2);

					DrawListPart(cl, obj, _mtop(obj), delta + 4, TRUE);
				}
				else
				{
					/* scroll bar goes down */
					ScrollWindowRasterNoFill(_window(obj), 0, -delta, _mleft(obj), _mtop(obj), _mright(obj), _mbottom(obj));

					DrawListPart(cl, obj, (_mbottom(obj)) + delta, -delta + 2, TRUE);
				}
			}
			d->old_first = xget(d->prop_object, MUIA_Prop_First);
		}

		if(d->draw_flag & DRAW_ACTIVE)
		{
			APTR clip;
			LONG clip_top, clip_height;

			if(d->active_entry)
			{
				struct MUI_PenSpec *pen_spec;
				LONG active_pen;

				pen_spec = (struct MUI_PenSpec*)xget(prefs_object(USD_PREFS_CONTACTSLIST_ACTIVEENTRY_COLOR), MUIA_Pendisplay_Spec);
				active_pen = MUI_ObtainPen(muiRenderInfo(obj), pen_spec, 0);

				clip_top = _mtop(obj) + d->active_entry->display.pixel_start > _mtop(obj) ? _mtop(obj) + d->active_entry->display.pixel_start : _mtop(obj);
				clip_height = clip_top + d->active_entry->display.pixel_height > _mbottom(obj) ? _bottom(obj) - clip_top : d->active_entry->display.pixel_height;

				clip = MUI_AddClipping(muiRenderInfo(obj), _mleft(obj), clip_top, _mwidth(obj), clip_height + 1);

				SetAPen(_rp(obj), MUIPEN(active_pen));
				RectFill(_rp(obj), _left(obj), _mtop(obj) + d->active_entry->display.pixel_start, _right(obj), _mtop(obj) + d->active_entry->display.pixel_start + d->active_entry->display.pixel_height);

				MUI_ReleasePen(muiRenderInfo(obj), active_pen);

				DrawListPart(cl, obj, _mtop(obj) + d->active_entry->display.pixel_start, d->active_entry->display.pixel_height, FALSE);

				MUI_RemoveClipping(muiRenderInfo(obj), clip);
			}

			if(d->prev_active_entry && d->prev_active_entry != d->active_entry)
			{
				clip_top = _mtop(obj) + d->prev_active_entry->display.pixel_start > _mtop(obj) ? _mtop(obj) + d->prev_active_entry->display.pixel_start : _mtop(obj);
				clip_height = clip_top + d->prev_active_entry->display.pixel_height > _mbottom(obj) ? _mbottom(obj) - clip_top : d->prev_active_entry->display.pixel_height;

				clip = MUI_AddClipping(muiRenderInfo(obj), _mleft(obj), clip_top, _mwidth(obj), clip_height + 2);

				DrawListPart(cl, obj, _mtop(obj) + d->prev_active_entry->display.pixel_start, d->prev_active_entry->display.pixel_height + 2, TRUE);

				MUI_RemoveClipping(muiRenderInfo(obj), clip);
			}
		}
	}

	d->old_height = _mheight(obj);
	d->old_width = _mwidth(obj);
	d->draw_flag = 0x00;

	return (IPTR)0;
}

static IPTR ContactsListScroll(Class *cl, Object *obj)
{
	struct ContactsListData *d = INST_DATA(cl, obj);

	set(obj, MUIA_DoubleBuffer, FALSE); /* when scrolling we use ScrollWindowRasterNoFill() which don't like DoubleBuffer */

	d->draw_flag |= DRAW_SCROLL;

	MUI_Redraw(obj, MADF_DRAWUPDATE);

	set(obj, MUIA_DoubleBuffer, TRUE);

	return 0;
}

static IPTR ContactsListSetup(Class *cl, Object *obj, struct MUIP_Setup *msg)
{
	struct ContactsListData *d = INST_DATA(cl, obj);

	IPTR result = (IPTR) DoSuperMethodA(cl, obj, msg);

	d->handler.ehn_Class = cl;
	d->handler.ehn_Object = obj;
	d->handler.ehn_Events = IDCMP_MOUSEBUTTONS | IDCMP_RAWKEY;
	d->handler.ehn_Flags = MUI_EHF_GUIMODE;
	d->handler.ehn_Priority = 0;

	DoMethod(_win(obj), MUIM_Window_AddEventHandler, &d->handler);

	return result;
}

static IPTR ContactsListCleanup(Class *cl, Object *obj, struct MUIP_Cleanup *msg)
{
	struct ContactsListData *d = INST_DATA(cl, obj);

	DoMethod(_win(obj), MUIM_Window_RemEventHandler, &d->handler);

	return (DoSuperMethodA(cl, obj, msg));
}

static IPTR ContactsListHandleEvent(Class *cl, Object *obj, struct MUIP_HandleEvent *msg)
{
	struct ContactsListData *d = INST_DATA(cl, obj);
	struct IntuiMessage *imsg = msg->imsg;
	static ULONG prev_click_sec = 0, prev_click_mic = 0;

	if(imsg != NULL)
	{
		if(imsg->Class == IDCMP_MOUSEBUTTONS)
		{
			if(_isinobject(imsg->MouseX, imsg->MouseY))
			{
				if(imsg->Code == IECODE_LBUTTON || imsg->Code == IECODE_RBUTTON)
				{
					struct ListEntry *act_entry = NULL;
					BOOL click_in_list = FALSE;
					IPTR result = 0;

					set(_win(obj), MUIA_Window_ActiveObject, obj);

					ForeachNode(&d->data_list, act_entry)
					{
						if(act_entry->display.pixel_start != -1 && _between(_mtop(obj) + act_entry->display.pixel_start, imsg->MouseY, _mtop(obj) + act_entry->display.pixel_start + act_entry->display.pixel_height))
						{
							click_in_list = TRUE;
							break;
						}
					}

					if(click_in_list == TRUE)
						set(obj, CLSA_Active, act_entry);
					else
					{
						nnset(obj, CLSA_Active, NULL);
						MUI_Redraw(obj, MADF_DRAWOBJECT); /* LAME */
					}

					if(click_in_list && imsg->Code == IECODE_LBUTTON)
					{
						ULONG cur_click_sec, cur_click_mic;

						CurrentTime(&cur_click_sec, &cur_click_mic);

						if(d->active_entry != NULL && d->active_entry == d->prev_active_entry && DoubleClick(prev_click_sec, prev_click_mic, cur_click_sec, cur_click_mic))
						{
							DoMethod(obj, CLSM_DoubleClick);
							result |= MUI_EventHandlerRC_Eat;
						}
						prev_click_sec = cur_click_sec;
						prev_click_mic = cur_click_mic;
					}

					if(click_in_list && imsg->Code == IECODE_RBUTTON)
					{
						DoMethod(obj, CLSM_DrawContextMenu, imsg->MouseX, imsg->MouseY);
						result |= MUI_EventHandlerRC_Eat;
					}

					return result;
				}
			}
		}

		if(imsg->Class == IDCMP_RAWKEY)
		{
			if(_isinobject(imsg->MouseX, imsg->MouseY))
			{
				LONG act_first = xget(d->prop_object, MUIA_Prop_First);

				if(imsg->Code == RAWKEY_NM_WHEEL_DOWN)
				{
					set(d->prop_object, MUIA_Prop_First, act_first + 30);
					return MUI_EventHandlerRC_Eat;
				}

				if(imsg->Code == RAWKEY_NM_WHEEL_UP)
				{
					set(d->prop_object, MUIA_Prop_First, act_first - 30);
					return MUI_EventHandlerRC_Eat;
				}
			}

			if((BOOL)xget(_win(obj), MUIA_Window_Activate))
			{
				if(imsg->Code == RAWKEY_RETURN)
				{
					DoMethod(obj, CLSM_DoubleClick);
					return MUI_EventHandlerRC_Eat;
				}
			}

			if((Object*)xget(_win(obj), MUIA_Window_ActiveObject) == obj)
			{
				if(imsg->Code == RAWKEY_DOWN)
				{
					if(d->active_entry)
					{
						set(obj, CLSA_Active, d->active_entry->node.mln_Succ);
					}
					else
					{
						set(obj, CLSA_Active, d->data_list.mlh_Head);
					}
				}

				if(imsg->Code == RAWKEY_UP)
				{
					if(d->active_entry)
					{
						set(obj, CLSA_Active, d->active_entry->node.mln_Pred);
					}
					else
					{
						set(obj, CLSA_Active, d->data_list.mlh_Tail);
					}
				}

				if(imsg->Code == RAWKEY_RETURN)
				{
					DoMethod(obj, CLSM_DoubleClick);
				}
			}
		}
	}
	return 0;
}

static IPTR ContactsListDoubleClick(Class *cl, Object *obj)
{
	struct ContactEntry *entry;
	Object *talkwindow = findobj(USD_TALKWINDOW_WINDOW, _app(obj));

	DoMethod(obj, CLSM_GetEntry, CLSV_GetEntry_Active, &entry);

	if(entry)
	{
		DoMethod(talkwindow, TKWM_ShowMessage, entry->entryid, entry->pluginid, NULL);
		DoMethod(talkwindow, MUIM_Window_ScreenToFront);
	}

	return (IPTR)1;
}

static IPTR ContactsListDrawActive(Class *cl, Object *obj)
{
	struct ContactsListData *d = INST_DATA(cl, obj);
	LONG start_height = xget(d->prop_object, MUIA_Prop_First);

	if(_between(start_height, d->active_entry->display.virtual_pixel_start, start_height + _mheight(obj)))
	{
		set(obj, MUIA_DoubleBuffer, FALSE); /* DRAW_ACTIVE don't like double buffer */

		d->draw_flag |= DRAW_ACTIVE;

		MUI_Redraw(obj, MADF_DRAWUPDATE);

		set(obj, MUIA_DoubleBuffer, TRUE);
	}
	else
	{
		LONG first = d->active_entry->display.virtual_pixel_start + d->active_entry->display.pixel_height - _mheight(obj);

		d->draw_flag |= DRAW_ACTIVE;

		set(d->prop_object, MUIA_Prop_First, first);
	}

	return (IPTR)1;
}

static IPTR ContactsListEditContact(Class *cl, Object *obj)
{
	struct ContactEntry *contact;

	DoMethod(obj, CLSM_GetEntry, CLSV_GetEntry_Active, &contact);
	DoMethod(findobj(USD_EDIT_CONTACT_WINDOW_WIN, _app(obj)), ECWM_EditContact, contact);

	return (IPTR)1;
}

static IPTR ContactsListDrawContextMenu(Class *cl, Object *obj, struct CLSP_DrawContextMenu *msg)
{
	struct ContactsListData *d = INST_DATA(cl, obj);

	if(d->active_entry)
	{
		STRPTR links[11]= {0};
		ULONG links_no = 0;
		Object *strip = NULL, *menu = NULL;

		if(d->active_entry->data.statusdesc)
		{
			BYTE special[1024], normal[1024];
			STRPTR parse = d->active_entry->data.statusdesc;
			ULONG type;

			do
			{
				type = MessageParse(&parse, (STRPTR)normal, (STRPTR)special);

				if(type & SPECIALWORD)
				{
					if(IS_URL(type))
					{
						STRPTR buffer = NULL;
						if(type & URL_MAIL)
						{
							buffer = FmtNew("mailto:%s", (STRPTR)special);
						}

						if((type & URL_PLAIN) || (type & URL_DOMAIN))
						{
							buffer = FmtNew("http://%s", (STRPTR)special);
						}

						if(buffer == NULL)
						{
							links[links_no++] = FmtNew("%s", (STRPTR)special);
						}
						else
						{
							links[links_no++] = buffer;
						}
					}
				}
				if(type & WHITECHAR)
					parse++;

				normal[0] = special[0] = 0x00;
			}while(type && links_no < 10);
		}

		strip = MUI_NewObject(MUIC_Menustrip,
			MUIA_Group_Child, (menu = MUI_NewObject(MUIC_Menu,
				MUIA_Group_Child, MUI_NewObject(MUIC_Menuitem,
					MUIA_UserData, CONLIST_CMENU_OPEN_TALK,
					MUIA_Menuitem_Title, GetString(MSG_CONTACTLIST_CONTEXTMENU_OPEN_TALK),
				TAG_END),
				MUIA_Group_Child, MUI_NewObject(MUIC_Menuitem,
					MUIA_UserData, CONLIST_CMENU_SEND_FTP,
					MUIA_Menuitem_Title, GetString(MSG_CONTACTLIST_CONTEXTMENU_SEND_FTP),
				TAG_END),
				MUIA_Group_Child, MUI_NewObject(MUIC_Menuitem,
					MUIA_UserData, CONLIST_CMENU_OPEN_LOG,
					MUIA_Menuitem_Title, GetString(MSG_CONTACTLIST_CONTEXTMENU_OPEN_LOG),
				TAG_END),
				MUIA_Group_Child, MUI_NewObject(MUIC_Menuitem,
					MUIA_Menuitem_Title, NM_BARLABEL,
				TAG_END),
				MUIA_Group_Child, MUI_NewObject(MUIC_Menuitem,
					MUIA_UserData, CONLIST_CMENU_EDIT,
					MUIA_Menuitem_Title, GetString(MSG_CONTACTLIST_CONTEXTMENU_EDIT),
				TAG_END),
				MUIA_Group_Child, MUI_NewObject(MUIC_Menuitem,
					MUIA_UserData, CONLIST_CMENU_DELETE,
					MUIA_Menuitem_Title, GetString(MSG_CONTACTLIST_CONTEXTMENU_DELETE),
				TAG_END),
			TAG_END)),
		TAG_END);

		if(strip && menu)
		{
			ULONG result, i;

			if(links_no > 0)
			{
				Object *links_menu = MUI_NewObject(MUIC_Menuitem, MUIA_UserData, CONLIST_CMENU_GO_TO_LINK,
															  MUIA_Menuitem_Title, GetString(MSG_CONTACTLIST_CONTEXTMENU_GO_TO_LINK),
															  TAG_END);

				DoMethod(menu, MUIM_Group_InitChange);

				DoMethod(menu, OM_ADDMEMBER, MUI_NewObject(MUIC_Menuitem, MUIA_Menuitem_Title, NM_BARLABEL, TAG_END));

				if(links_menu)
				{
					DoMethod(menu, OM_ADDMEMBER, links_menu);

					if(links_no > 1)
					{
						ULONG i;

						DoMethod(links_menu, MUIM_Group_InitChange);

						for(i = 0; i < links_no && links[i]; i++)
						{
							Object *member = MUI_NewObject(MUIC_Menuitem, MUIA_UserData, i+1, MUIA_Menuitem_Title, links[i], TAG_END);

							if(member)
							{
								DoMethod(links_menu, OM_ADDMEMBER, member);
							}
						}
						DoMethod(links_menu, MUIM_Group_ExitChange);
					}
				}
				DoMethod(menu, MUIM_Group_ExitChange);
			}

			if(d->active_entry->data.statusdesc)
			{
				Object *copy_desc = MUI_NewObject(MUIC_Menuitem,
															 MUIA_UserData, CONLIST_CMENU_COPY_DESC,
															 MUIA_Menuitem_Title, GetString(MSG_CONTACTLIST_CONTEXTMENU_COPY_DESC),
															 TAG_END);

				if(copy_desc)
				{
					DoMethod(menu, MUIM_Group_InitChange);

					DoMethod(menu, OM_ADDMEMBER, MUI_NewObject(MUIC_Menuitem, MUIA_Menuitem_Title, NM_BARLABEL, TAG_END));
					DoMethod(menu, OM_ADDMEMBER, copy_desc);

					DoMethod(menu, MUIM_Group_ExitChange);
				}
			}

			result = DoMethod(strip, MUIM_Menustrip_Popup, obj, 0, msg->x, msg->y);

			switch(result)
			{
				case 0:
					/* nothing selected */
				break;

				case CONLIST_CMENU_EDIT:
					DoMethod(obj, CLSM_EditContact);
				break;

				case CONLIST_CMENU_DELETE:
					if(DoMethod(obj, CLSM_Remove, CLSV_Remove_Active))
						DoMethod(obj, CLSM_SaveList);
				break;

				case CONLIST_CMENU_GO_TO_LINK:
					URL_Open(links[0], TAG_END);
				break;

				case CONLIST_CMENU_COPY_DESC:
					DoMethod(_app(obj), APPM_ClipboardStart, StrLen(d->active_entry->data.statusdesc));
					DoMethod(_app(obj), APPM_ClipboardWrite, d->active_entry->data.statusdesc, StrLen(d->active_entry->data.statusdesc));
					DoMethod(_app(obj), APPM_ClipboardEnd);
				break;

				case CONLIST_CMENU_OPEN_TALK:
					DoMethod(obj, CLSM_DoubleClick);
				break;

				case CONLIST_CMENU_SEND_FTP:
				{
					struct ContactEntry *entry;

					DoMethod(obj, CLSM_GetEntry, CLSV_GetEntry_Active, &entry);

					if(entry)
						DoMethod(_app(obj), APPM_FtpPut, entry->pluginid, entry->entryid);
				}
				break;

				case CONLIST_CMENU_OPEN_LOG:
				{
					struct ContactEntry *entry;

					DoMethod(obj, CLSM_GetEntry, CLSV_GetEntry_Active, &entry);

					if(entry)
						DoMethod(obj, CLSM_OpenLogFile, ContactName(entry));
				}
				break;

				default:
					URL_Open(links[result - 1], TAG_END);
				break;
			}

			for(i = 0; i < links_no && links[i]; i++)
				FmtFree(links[i]);
		}
	}

	return(IPTR)1;
}

static IPTR ContactsListSelectEntryByName(Class *cl, Object *obj, struct CLSP_SelectEntryByName *msg)
{
	struct ContactsListData *d = INST_DATA(cl, obj);
	struct ListEntry *act_entry = NULL;
	BOOL found = FALSE;
	STRPTR template = (STRPTR)xget(d->search_string, MUIA_String_Contents);

	if((BOOL)msg->next && d->active_entry)
	{
		act_entry = (struct ListEntry*) d->active_entry->node.mln_Succ;
	}
	else
	{
		act_entry = (struct ListEntry*) d->data_list.mlh_Head;
	}

	while(act_entry && act_entry->node.mln_Succ)
	{
		if(StrIStr(ContactNameLoc(act_entry->data), template))
		{
			found = TRUE;
			break;
		}
		act_entry = (struct ListEntry*) act_entry->node.mln_Succ;
	}

	if(found == TRUE)
	{
		LONG first = act_entry->display.virtual_pixel_start + act_entry->display.pixel_height - _mheight(obj);

		set(d->prop_object, MUIA_Prop_First, first);

		set(obj, CLSA_Active, act_entry);
	}
	else if((BOOL)msg->next)
	{
		DoMethod(obj, CLSM_SelectEntryByName, FALSE);
	}

	return found;
}

static IPTR ContactsListRedrawAll(Class *cl, Object *obj)
{
	struct ContactsListData *d = INST_DATA(cl, obj);

	d->draw_flag |= DRAW_NEW_SIZE;

	MUI_Redraw(obj, MADF_DRAWOBJECT);

	return (IPTR)1;
}

static IPTR ContactsListRemoveClones(Class *cl, Object *obj)
{
	struct ContactsListData *d = INST_DATA(cl, obj);
	struct ListEntry *fst = NULL, *sec = NULL;
	ULONG clones_no = 0;

	ForeachNode(&d->data_list, fst)
	{
		struct ListEntry *safe;
		ForeachNodeSafe(&d->data_list, sec, safe)
		{
			if(StrEqu(fst->data.entryid, sec->data.entryid) && fst->data.pluginid == sec->data.pluginid && fst != sec)
			{
				clones_no++;
				REMOVE(sec);
				ADDHEAD(&d->data_list, sec);
			}
		}
	}

	if(clones_no > 0  && MUI_Request(_app(obj), _win(obj), 0L, APP_NAME, GetString(MSG_CONTACTLIST_DELETE_GADGETS), GetString(MSG_CONTACTLIST_DELETE_CLONES_REQ), clones_no) == 1)
	{
		do
		{
			struct ListEntry *to_del;

			to_del = REMHEAD(&d->data_list);

			if(to_del->data.entryid) StrFree(to_del->data.entryid);
			if(to_del->data.name) StrFree(to_del->data.name);
			if(to_del->data.nickname) StrFree(to_del->data.nickname);
			if(to_del->data.firstname) StrFree(to_del->data.firstname);
			if(to_del->data.lastname) StrFree(to_del->data.lastname);
			if(to_del->data.groupname) StrFree(to_del->data.groupname);
			if(to_del->data.statusdesc) StrFree(to_del->data.statusdesc);
			if(to_del->data.city) StrFree(to_del->data.city);
			if(to_del->data.avatar) FreePicture(to_del->data.avatar);

			FreePooled(d->memory_pool, to_del, sizeof(struct ListEntry));
			d->list_entries_no--;
			clones_no--;
		}while(clones_no > 0);

		set(obj, CLSA_List_Entries, d->list_entries_no);
		DoMethod(obj, CLSM_RedrawAll);
		DoMethod(obj, CLSM_SaveList);
	}
	else
	{
		MUI_Request(_app(obj), _win(obj), 0L, APP_NAME, GetString(MSG_CONTACTLIST_NO_CLONES_GADGETS), GetString(MSG_CONTACTLIST_NO_CLONES), NULL);
	}

	return (IPTR)1;
}

static IPTR ContactsListSaveList(Class *cl, Object *obj)
{
	BPTR fh;

	if((fh = Open((STRPTR)CONTACTS_LIST_PATH, MODE_NEWFILE)))
	{
		ezxml_t list;

		if((list = ContactsListToEzxml_t(obj)))
		{
			WriteEzxmlToFile(list, fh);

			ezxml_free(list);
		}
		Close(fh);
	}

	return (IPTR)1;
}

static IPTR ContactsListReadList(Class *cl, Object *obj)
{
	BPTR fh;

	if((fh = Open((STRPTR)CONTACTS_LIST_PATH, MODE_OLDFILE)))
	{
		struct FileInfoBlock fib;

		if((ExamineFH(fh, &fib)))
		{
			STRPTR buffer;

			if((buffer = AllocMem(fib.fib_Size, MEMF_ANY | MEMF_CLEAR)) && (FRead(fh, (APTR)buffer, fib.fib_Size, 1) == 1))
			{
				ImportXMLTxt(obj, buffer);
				FreeMem(buffer, fib.fib_Size);
			}
		}
		Close(fh);
	}

	return (IPTR)1;
}

static IPTR ContactsListRemoveEntry(Class *cl, Object *obj, struct CLSP_RemoveEntry *msg)
{
	struct ContactsListData *d = INST_DATA(cl, obj);
	struct ListEntry *e;

	ForeachNode(&d->data_list, e)
	{
		if(&e->data == msg->entry)
			break;
	}

	if(e)
	{
		if(!msg->confirm || MUI_Request(_app(obj), _win(obj), 0L, APP_NAME, GetString(MSG_CONTACTLIST_DELETE_GADGETS), GetString(MSG_CONTACTLIST_DELETE_REQ), ContactNameLoc(((struct ListEntry*)(d->data_list.mlh_Head))->data)) == 1)
		{
			Remove((struct Node*)e);

			DoMethod(_app(obj), APPM_RemoveNotify, e->data.pluginid, e->data.entryid, e->data.status);

			if(e->data.entryid) StrFree(e->data.entryid);
			if(e->data.name) StrFree(e->data.name);
			if(e->data.nickname) StrFree(e->data.nickname);
			if(e->data.firstname) StrFree(e->data.firstname);
			if(e->data.lastname) StrFree(e->data.lastname);
			if(e->data.groupname) StrFree(e->data.groupname);
			if(e->data.statusdesc) StrFree(e->data.statusdesc);
			if(e->data.city) StrFree(e->data.city);
			if(e->data.avatar) FreePicture(e->data.avatar);

			FreePooled(d->memory_pool, e, sizeof(struct ListEntry));
			d->list_entries_no--;
			set(obj, CLSA_List_Entries, d->list_entries_no);

			d->draw_flag |= DRAW_NEW_SIZE;

			MUI_Redraw(obj, MADF_DRAWOBJECT);
		}
	}

	return (IPTR)1;
}

static IPTR ContactsListOpenLogFile(Class *cl, Object *obj, struct CLSP_OpenLogFile *msg)
{
	BOOL result = FALSE;
	UBYTE buffer[255];
	BPTR lock;

	if(msg->log_file_name)
	{
		FmtNPut((STRPTR)buffer, LOGS_DRAWER "/%ls", sizeof(buffer), msg->log_file_name);

		if((lock = Lock(buffer, ACCESS_READ)))
		{
			if(NameFromLock(lock, buffer, sizeof(buffer)))
			{
				result = OpenWorkbenchObjectA(buffer, NULL);
			}
			UnLock(lock);
		}
	}

	return(IPTR)result;
}

static IPTR ContactsListFindEntry(Class *cl, Object *obj, struct CLSP_FindEntry *msg)
{
	struct ContactsListData *d = INST_DATA(cl, obj);
	struct ListEntry *act_entry = (struct ListEntry*)d->data_list.mlh_Head;

	if(msg->start) /* will break when struct MinNode changes */
		act_entry = (struct ListEntry*)((struct ListEntry*)(((UBYTE*)msg->start) - sizeof(struct MinNode)))->node.mln_Succ;

	if(!act_entry)
		return (IPTR)NULL;

	while(act_entry && act_entry->node.mln_Succ)
	{
		switch(msg->mode)
		{
			case CLSV_FindEntry_Mode_Name:
				if(StrIStr(ContactNameLoc(act_entry->data), ContactName(msg->entry)))
					return (IPTR)&act_entry->data;
			break;

			case CLSV_FindEntry_Mode_ID:
			default:
				if(act_entry->data.pluginid == msg->entry->pluginid && StrEqu(act_entry->data.entryid, msg->entry->entryid))
					return (IPTR)&act_entry->data;
			break;
		}

		act_entry = (struct ListEntry*) act_entry->node.mln_Succ;
	}

	return (IPTR)NULL;
}

static IPTR ContactsListDispatcher(void)
{
	Class *cl = (Class*)REG_A0;
	Object *obj = (Object*)REG_A2;
	Msg msg = (Msg)REG_A1;

	switch (msg->MethodID)
	{
		case OM_NEW:  return (ContactsListNew(cl, obj, (struct opSet*)msg));
		case OM_DISPOSE:  return (ContactsListDispose(cl, obj, msg));
		case OM_SET:  return (ContactsListSet(cl, obj, (struct opSet*)msg));
		case OM_GET:  return (ContactsListGet(cl, obj, (struct opGet*)msg));
		case CLSM_InsertSingle: return(ContactsListInsertSingle(cl, obj, (struct CLSP_InsertSingle*)msg));
		case CLSM_Remove: return(ContactsListRemove(cl, obj, (struct CLSP_Remove*)msg));
		case CLSM_GetEntry: return(ContactsListGetEntry(cl, obj, (struct CLSP_GetEntry*)msg));
		case CLSM_Clear: return(ContactsListClear(cl, obj));
		case CLSM_Compare: return(ContactsListCompare(cl, obj, (struct CLSP_Compare*)msg));
		case CLSM_Sort: return(ContactsListSort(cl, obj));
		case MUIM_Import: return(ContactsListImport(cl, obj, (struct MUIP_Import*)msg));
		case CLSM_EditContact: return (ContactsListEditContact(cl, obj));
		case CLSM_SetAllUnavail: return (ContactsListSetAllUnavail(cl, obj));
		case CLSM_CheckUnread: return(ContactsListCheckUnread(cl, obj));
		case CLSM_ExportToFile: return(ContactsListExportToFile(cl, obj));
		case CLSM_ImportFromFile: return(ContactsListImportFromFile(cl, obj));
		case MUIM_AskMinMax: return(ContactsListAskMinMax(cl, obj, (struct MUIP_AskMinMax*)msg));
		case MUIM_Draw: return(ContactsListDraw(cl, obj, (struct MUIP_Draw*)msg));
		case CLSM_Scroll: return(ContactsListScroll(cl, obj));
		case MUIM_Setup: return(ContactsListSetup(cl, obj, (struct MUIP_Setup*)msg));
		case MUIM_Cleanup: return(ContactsListCleanup(cl, obj, (struct MUIP_Cleanup*)msg));
		case MUIM_HandleEvent: return(ContactsListHandleEvent(cl, obj, (struct MUIP_HandleEvent*)msg));
		case CLSM_DoubleClick: return(ContactsListDoubleClick(cl, obj));
		case CLSM_DrawActive: return(ContactsListDrawActive(cl, obj));
		case CLSM_DrawContextMenu: return(ContactsListDrawContextMenu(cl, obj, (struct CLSP_DrawContextMenu*)msg));
		case CLSM_SelectEntryByName: return(ContactsListSelectEntryByName(cl, obj, (struct CLSP_SelectEntryByName*)msg));
		case CLSM_RedrawAll: return(ContactsListRedrawAll(cl, obj));
		case CLSM_RemoveClones: return(ContactsListRemoveClones(cl, obj));
		case CLSM_ReadList: return(ContactsListReadList(cl, obj));
		case CLSM_SaveList: return(ContactsListSaveList(cl, obj));
		case CLSM_RemoveEntry: return(ContactsListRemoveEntry(cl, obj, (struct CLSP_RemoveEntry*)msg));
		case CLSM_OpenLogFile: return(ContactsListOpenLogFile(cl, obj, (struct CLSP_OpenLogFile*)msg));
		case CLSM_FindEntry: return(ContactsListFindEntry(cl, obj, (struct CLSP_FindEntry*)msg));
		default:  return (DoSuperMethodA(cl, obj, msg));
	}
}

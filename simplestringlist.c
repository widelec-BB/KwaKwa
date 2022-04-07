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
#include "simplestringlist.h"

#define MUIV_SimpleStringList_Menu_Remove 1
#define MUIV_SimpleStringList_Menu_Copy 2

struct MUI_CustomClass *SimpleStringListClass;

static IPTR SimpleStringListDispatcher(VOID);
const struct EmulLibEntry SimpleStringListGate = {TRAP_LIB, 0, (VOID(*)(VOID))SimpleStringListDispatcher};

struct MUIP_SimpleStringList_Find{ULONG MethodID; STRPTR string; LONG next;};

struct SimpleStringListData
{
	Object *menu;

	struct MUI_EventHandlerNode ehn;
};

struct MUI_CustomClass *CreateSimpleStringListClass(VOID)
{
	struct MUI_CustomClass *cl;

	cl = MUI_CreateCustomClass(NULL, MUIC_List, NULL, sizeof(struct SimpleStringListData), (APTR)&SimpleStringListGate);
	SimpleStringListClass = cl;
	return cl;
}

VOID DeleteSimpleStringListClass(VOID)
{
	if (SimpleStringListClass) MUI_DeleteCustomClass(SimpleStringListClass);
}

static IPTR SimpleStringListNew(Class *cl, Object *obj, struct opSet *msg)
{
	Object *menu = MUI_NewObject(MUIC_Menustrip,
		MUIA_Unicode, TRUE,
		MUIA_Group_Child, MUI_NewObject(MUIC_Menu,
			MUIA_Group_Child, MUI_NewObject(MUIC_Menuitem,
				MUIA_Menuitem_Title, GetString(MSG_SIMPLESTRINGLIST_MENU_REMOVE),
				MUIA_UserData, MUIV_SimpleStringList_Menu_Remove,
			TAG_END),
			MUIA_Group_Child, MUI_NewObject(MUIC_Menuitem,
				MUIA_Menuitem_Title, GetString(MSG_SIMPLESTRINGLIST_MENU_COPY),
				MUIA_UserData, MUIV_SimpleStringList_Menu_Copy,
			TAG_END),
		TAG_END),
	TAG_END);

	obj = DoSuperNew(cl, obj,
		MUIA_Frame, MUIV_Frame_InputList,
		MUIA_Background, MUII_ListBack,
		MUIA_ContextMenu, menu,
		MUIA_Unicode, TRUE,
		MUIA_List_ConstructHook, MUIV_List_ConstructHook_String,
		MUIA_List_DestructHook, MUIV_List_DestructHook_String,
	TAG_MORE, msg->ops_AttrList);

	if(obj)
	{
		struct SimpleStringListData *d = INST_DATA(cl, obj);

		d->menu = menu;
		DoMethod(obj, MUIM_Import, NULL);
		return (IPTR)obj;
	}

	MUI_DisposeObject(menu);
	CoerceMethod(cl, obj, OM_DISPOSE);
	return (IPTR)NULL;
}

static IPTR SimpleStringListDispose(Class *cl, Object *obj, Msg msg)
{
	struct SimpleStringListData *d = INST_DATA(cl, obj);

	DoMethod(obj, MUIM_Export, NULL);

	MUI_DisposeObject(d->menu);

	return DoSuperMethodA(cl, obj, msg);
}

static IPTR SimpleStringListExport(Class *cl, Object *obj, struct MUIP_Export *msg)
{
	BPTR fh;
	ENTER();

	if((fh = Open(CACHE_DIR GUI_DIR "desclist.cfg", MODE_NEWFILE)))
	{
		ULONG i;
		STRPTR entry = NULL;

		FPrintf(fh, "UTF-8\n");

		for(i=0;;i++)
		{
			DoMethod(obj, MUIM_List_GetEntry, i, &entry);
			if(entry == NULL)
				break;

			FPrintf(fh, "%s\n", entry);
		}
		Close(fh);
	}

	LEAVE();
	return (IPTR)0;
}

static IPTR SimpleStringListImport(Class *cl, Object *obj, struct MUIP_Import *msg)
{
	STRPTR first;
	ENTER();

	DoMethod(obj, MUIM_List_GetEntry, 0, &first);

	if(first == NULL)
	{
		BPTR fh;
		BOOL unicode_enabled = FALSE, charset_check = TRUE;

		if((fh = Open(CACHE_DIR GUI_DIR "desclist.cfg", MODE_OLDFILE)))
		{
			ULONG pos = 0;
			BYTE ch;
			BYTE buffer[KWA_STATUS_DESC_MAX_SIZE + 1];

			while((ch = FGetC(fh)) != -1) /* -1 == EOF */
			{
				if(pos <= KWA_STATUS_DESC_MAX_SIZE)
				{
					buffer[pos] = ch;
					if(ch == (BYTE)'\n' || pos == KWA_STATUS_DESC_MAX_SIZE - 1)
					{
						buffer[pos] = 0x00;
						if(charset_check)
						{
							if(StrEqu(buffer, "UTF-8"))
							{
								unicode_enabled = TRUE;
								pos = 0;
								continue;
							}
							charset_check = FALSE;
						}
						if(!unicode_enabled)
						{
							STRPTR converted = SystemToUtf8(buffer);
							if(converted)
							{
								DoMethod(obj, MUIM_List_InsertSingle, converted, MUIV_List_Insert_Bottom);
								StrFree(converted);
							}
						}
						else
							DoMethod(obj, MUIM_List_InsertSingle, buffer, MUIV_List_Insert_Bottom);
					}
				}
				if(ch == (BYTE)'\n')
					pos = 0;
				else
					pos++;
			}

			Close(fh);
		}
	}

	LEAVE();
	return (IPTR)0;
}

static IPTR SimpleStringListSetup(Class *cl, Object *obj, struct MUIP_Setup *msg)
{
	IPTR result = DoSuperMethodA(cl, obj, msg);

	if(result)
	{
		DoMethod(_win(obj), MUIM_Notify, MUIA_Window_Open, FALSE, obj, 2,
		 MUIM_Export, NULL);
	}

	return result;
}

static IPTR SimpleStringListCleanup(Class *cl, Object *obj, struct MUIP_Cleanup *msg)
{
	DoMethod(obj, MUIM_SimpleStringList_RemEventHandler);
	return DoSuperMethodA(cl, obj, (Msg)msg);
}

static IPTR SimpleStringListFind(Class *cl, Object *obj, struct MUIP_SimpleStringList_Find *msg)
{
	STRPTR found = NULL;
	LONG start = MUIV_List_Active_Off, pos;
	ENTER();

	if(!msg->string)
	{
		LEAVE();
		return (IPTR)0;
	}

	if((BOOL)msg->next)
	{
		if((start = xget(obj, MUIA_List_Active)) == MUIV_List_Active_Off)
		{
			LEAVE();
			return (IPTR)0;
		}
		++start;
	}

	if(start == MUIV_List_Active_Off)
		start = 0;

	for(pos = start;;++pos)
	{
		DoMethod(obj, MUIM_List_GetEntry, pos, &found);

		if(!found || StrIStr(found, msg->string))
			break;
	}

	if(!found)
	{
		for(pos = 0; pos < start; ++pos)
		{
			DoMethod(obj, MUIM_List_GetEntry, pos, &found);

			if(!found || StrIStr(found, msg->string))
				break;
		}
		if(pos == start)
			found = NULL;
	}

	if(found)
		set(obj, MUIA_List_Active, pos);

	LEAVE();
	return (IPTR)NULL;
}

static IPTR SimpleStringListContextMenuChoice(Class *cl, Object *obj, struct MUIP_ContextMenuChoice *msg)
{
	switch(muiUserData(msg->item))
	{
		case MUIV_SimpleStringList_Menu_Copy:
		{
			STRPTR s;

			DoMethod(obj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &s);

			if(s)
			{
				LONG l = StrLen(s);

				DoMethod(_app(obj), APPM_ClipboardStart, l);
				DoMethod(_app(obj), APPM_ClipboardWrite, s, l);
				DoMethod(_app(obj), APPM_ClipboardEnd);
			}
		}
		break;

		case MUIV_SimpleStringList_Menu_Remove:
			DoMethod(obj, MUIM_List_Remove, MUIV_List_Remove_Active);
		break;
	}

	return (IPTR)0;
}

static IPTR SimpleStringListAddEventHandler(Class *cl, Object *obj)
{
	struct SimpleStringListData *d = INST_DATA(cl, obj);
	Object *win = muiRenderInfo(obj) ? _win(obj) : NULL;

	if(win != NULL && !(d->ehn.ehn_Flags & MUI_EHF_ISENABLED))
	{
		d->ehn.ehn_Class = cl;
		d->ehn.ehn_Object = obj;
		d->ehn.ehn_Events = IDCMP_RAWKEY;
		d->ehn.ehn_Flags = MUI_EHF_PRIORITY; /* not for public use? bite me. needed to override sending event first to active/default gadget */
		d->ehn.ehn_Priority = 2;

		DoMethod(_win(obj), MUIM_Window_AddEventHandler, &d->ehn);
	}

	return (IPTR)0;
}

static IPTR SimpleStringListRemEventHandler(Class *cl, Object *obj)
{
	struct SimpleStringListData *d = INST_DATA(cl, obj);
	Object *win = muiRenderInfo(obj) ? _win(obj) : NULL;

	if(win != NULL)
		DoMethod(_win(obj), MUIM_Window_RemEventHandler, &d->ehn);

	return (IPTR)0;
}

static IPTR SimpleStringListDispatcher(VOID)
{
	Class *cl = (Class*)REG_A0;
	Object *obj = (Object*)REG_A2;
	Msg msg = (Msg)REG_A1;

	switch (msg->MethodID)
	{
		case OM_NEW: return (SimpleStringListNew(cl, obj, (struct opSet*) msg));
		case OM_DISPOSE: return (SimpleStringListDispose(cl, obj, msg));
		case MUIM_Export: return (SimpleStringListExport(cl, obj, (struct MUIP_Export*) msg));
		case MUIM_Import: return (SimpleStringListImport(cl, obj, (struct MUIP_Import*) msg));
		case MUIM_Setup: return (SimpleStringListSetup(cl, obj, (struct MUIP_Setup*) msg));
		case MUIM_Cleanup: return (SimpleStringListCleanup(cl, obj, (struct MUIP_Cleanup*)msg));
		case MUIM_SimpleStringList_Find: return(SimpleStringListFind(cl, obj, (struct MUIP_SimpleStringList_Find*)msg));
		case MUIM_ContextMenuChoice: return(SimpleStringListContextMenuChoice(cl, obj, (struct MUIP_ContextMenuChoice*)msg));
		case MUIM_SimpleStringList_AddEventHandler: return (SimpleStringListAddEventHandler(cl, obj));
		case MUIM_SimpleStringList_RemEventHandler: return (SimpleStringListRemEventHandler(cl, obj));
		default:  return (DoSuperMethodA(cl, obj, msg));
	}
}

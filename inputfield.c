/*
 * Copyright (c) 2012-2022 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/asl.h>
#include <libvstring.h>
#include <mui/TextEditor_mcc.h>
#include <libraries/gadtools.h>

#include "locale.h"
#include "globaldefines.h"
#include "prefswindow.h"
#include "support.h"
#include "inputfield.h"
#include "devices/rawkeycodes.h"
#include "talkwindow.h"
#include "talktab.h"
#include "virtualtext.h"

struct MUI_CustomClass *InputFieldClass;
static IPTR InputFieldDispatcher(VOID);
const struct EmulLibEntry InputFieldGate = {TRAP_LIB, 0, (VOID(*)(VOID))InputFieldDispatcher};

#define IFV_ContextMenu_Copy             1
#define IFV_ContextMenu_Cut              2
#define IFV_ContextMenu_Paste            3
#define IFV_ContextMenu_Clear            4
#define IFV_ContextMenu_ExternalEdit     5
#define IFV_ContextMenu_InsertFile       6
#define IFV_ContextMenu_PasteAndAccept   7

struct IFP_ContextMenu {ULONG MethodID; LONG mouse_x; LONG mouse_y;};
struct IFP_LoadTxtFile {ULONG MethodID; STRPTR path;};

struct InputFieldData
{
	BOOL send_after_return;
	struct MUI_EventHandlerNode handler;
	Object *talk_tab;
};

struct MUI_CustomClass *CreateInputFieldClass(VOID)
{
	struct MUI_CustomClass *cl;

	cl = MUI_CreateCustomClass(NULL, MUIC_TextEditor, NULL, sizeof(struct InputFieldData), (APTR)&InputFieldGate);
	InputFieldClass = cl;
	return cl;
}

VOID DeleteInputFieldClass(VOID)
{
	if (InputFieldClass) MUI_DeleteCustomClass(InputFieldClass);
}

static IPTR InputFieldNew(Class *cl, Object *obj, struct opSet *msg)
{
	obj = (Object*)DoSuperNew(cl, obj,
			MUIA_CycleChain, 1,
			MUIA_TextEditor_Rows, 4,
	TAG_MORE, (IPTR)msg->ops_AttrList);

	if(obj)
	{
		struct InputFieldData *d = INST_DATA(cl, obj);

		d->send_after_return = TRUE;

		return (IPTR)obj;
	}
	CoerceMethod(cl, obj, OM_DISPOSE);
	return (IPTR)NULL;
}

static IPTR InputFieldSet(Class *cl, Object *obj, struct opSet *msg)
{
	struct InputFieldData *d = INST_DATA(cl, obj);
	int tagcount = 0;
	struct TagItem *tag, *tagptr = msg->ops_AttrList;

	while((tag = NextTagItem(&tagptr)))
	{
		switch(tag->ti_Tag)
		{
			case IFA_SendAfterReturn:
				d->send_after_return = (BOOL)tag->ti_Data;
				tagcount++;
			break;

			case IFA_TalkTab:
				d->talk_tab = (Object*)tag->ti_Data;
				tagcount++;
			break;

			case IFA_TextContents:
				{
					STRPTR converted = Utf8ToSystem((STRPTR)tag->ti_Data);
					if(converted)
					{
						set(obj, MUIA_TextEditor_Contents, converted);
						StrFree(converted);
					}
				}
			break;
		}
	}

	tagcount += DoSuperMethodA(cl, obj, (Msg)msg);
	return tagcount;
}

static IPTR InputFieldGet(Class *cl, Object *obj, struct opGet *msg)
{
	int rv = FALSE;

	switch(msg->opg_AttrID)
	{
		case IFA_Acknowledge:
			return TRUE;
		break;

		default:
			rv = (DoSuperMethodA(cl, obj, (Msg)msg));
	}

	return rv;
}

static IPTR InputFieldSetup(Class *cl, Object *obj, struct MUIP_Setup *msg)
{
	struct InputFieldData *d = INST_DATA(cl, obj);
	IPTR result = (IPTR)DoSuperMethodA(cl, obj, msg);
	ENTER();

	if(result)
	{
		d->handler.ehn_Class = cl;
		d->handler.ehn_Object = obj;
		d->handler.ehn_Events = IDCMP_INACTIVEWINDOW | IDCMP_MOUSEBUTTONS | IDCMP_RAWKEY;
		d->handler.ehn_Priority = 1;
		d->handler.ehn_Flags = MUI_EHF_GUIMODE;
		DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&d->handler);
	}

	LEAVE();
	return result;
}

static IPTR InputFieldCleanup(Class *cl, Object *obj, struct MUIP_Cleanup *msg)
{
	struct InputFieldData *d = INST_DATA(cl, obj);

	DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&d->handler);

	return (DoSuperMethodA(cl, obj, msg));
}

static IPTR InputFieldHandleEvent(Class *cl, Object *obj, struct MUIP_HandleEvent *msg)
{
	struct InputFieldData *d = INST_DATA(cl, obj);

	if(msg->imsg != NULL)
	{
		if(msg->imsg->Class == IDCMP_RAWKEY)
		{
			if(d->send_after_return)
			{
				if(msg->imsg->Code == RAWKEY_RETURN)
				{
					set(obj, IFA_Acknowledge, TRUE);
					return MUI_EventHandlerRC_Eat;
				}
			}

			if(msg->imsg->Code == RAWKEY_KP_ENTER)
			{
				DoMethod(obj, MUIM_TextEditor_InsertText, "\n", MUIV_TextEditor_InsertText_Cursor);
				return MUI_EventHandlerRC_Eat;
			}

			if(msg->imsg->Qualifier & AMIGALEFT)
			{
				if(msg->imsg->Code >= RAWKEY_1 && msg->imsg->Code <= RAWKEY_9)
				{
					DoMethod(_win(obj), TKWM_OpenOnTabById, msg->imsg->Code - RAWKEY_1);
					return MUI_EventHandlerRC_Eat;
				}

				if(d->talk_tab)
				{
					if(msg->imsg->Code == RAWKEY_Z)
					{
						DoMethod(findobj(USD_TALKTAB_VIRTUALTEXT, d->talk_tab), VTM_Clear);
						return MUI_EventHandlerRC_Eat;
					}

					if(msg->imsg->Code == RAWKEY_D)
					{
						DoMethod(d->talk_tab, TTBM_ToggleDouble);
						return MUI_EventHandlerRC_Eat;
					}
				}
			}
			if(msg->imsg->Qualifier & AMIGARIGHT)
			{
				if(msg->imsg->Code == RAWKEY_E)
				{
					DoMethod(obj, IFM_ExternalEdit);
					return MUI_EventHandlerRC_Eat;
				}
			}

			if(msg->imsg->Code >= RAWKEY_F1 && msg->imsg->Code <= RAWKEY_F10)
			{
				DoMethod(obj, MUIM_TextEditor_InsertText,
				 (IPTR)xget(prefs_object(USD_PREFS_FKEYS_STRING(msg->imsg->Code - RAWKEY_F1)), MUIA_String_Contents), MUIV_TextEditor_InsertText_Cursor);

				return MUI_EventHandlerRC_Eat;
			}

			if(msg->imsg->Code == RAWKEY_F11)
			{
				DoMethod(obj, MUIM_TextEditor_InsertText, (IPTR)xget(prefs_object(USD_PREFS_FKEYS_STRING(10)), MUIA_String_Contents), MUIV_TextEditor_InsertText_Cursor);
				return MUI_EventHandlerRC_Eat;
			}

			if(msg->imsg->Code == RAWKEY_F12)
			{
				DoMethod(obj, MUIM_TextEditor_InsertText, (IPTR)xget(prefs_object(USD_PREFS_FKEYS_STRING(11)), MUIA_String_Contents), MUIV_TextEditor_InsertText_Cursor);
				return MUI_EventHandlerRC_Eat;
			}
		}
		else if(msg->imsg->Class == IDCMP_MOUSEBUTTONS)
		{
			if(_isinobject(msg->imsg->MouseX, msg->imsg->MouseY))
			{
				if(msg->imsg->Code == IECODE_RBUTTON)
				{
					DoMethod(obj, IFM_ContextMenu, msg->imsg->MouseX, msg->imsg->MouseY);
					return MUI_EventHandlerRC_Eat;
				}
			}
		}
	}

	return (IPTR)0;
}

static IPTR InputFieldContextMenu(Class *cl, Object *obj, struct IFP_ContextMenu *msg)
{
	Object *strip;

	strip = MUI_NewObject(MUIC_Menustrip,
		MUIA_Group_Child, MUI_NewObject(MUIC_Menu,
			MUIA_Group_Child, MUI_NewObject(MUIC_Menuitem,
				MUIA_Menuitem_Title, GetString(MSG_INPUTFIELD_CUT),
				MUIA_UserData, IFV_ContextMenu_Cut,
			TAG_END),
			MUIA_Group_Child, MUI_NewObject(MUIC_Menuitem,
				MUIA_Menuitem_Title, GetString(MSG_INPUTFIELD_COPY),
				MUIA_UserData, IFV_ContextMenu_Copy,
			TAG_END),
			MUIA_Group_Child, MUI_NewObject(MUIC_Menuitem,
				MUIA_Menuitem_Title, GetString(MSG_INPUTFIELD_PASTE),
				MUIA_UserData, IFV_ContextMenu_Paste,
			TAG_END),
			MUIA_Group_Child, MUI_NewObject(MUIC_Menuitem,
				MUIA_Menuitem_Title, GetString(MSG_INPUTFIELD_PASTEANDACCEPT),
				MUIA_UserData, IFV_ContextMenu_PasteAndAccept,
			TAG_END),
			MUIA_Group_Child, MUI_NewObject(MUIC_Menuitem,
				MUIA_Menuitem_Title, NM_BARLABEL,
			TAG_END),
			MUIA_Group_Child, MUI_NewObject(MUIC_Menuitem,
				MUIA_Menuitem_Title, GetString(MSG_INPUTFILED_CLEAR),
				MUIA_UserData, IFV_ContextMenu_Clear,
			TAG_END),
			MUIA_Group_Child, MUI_NewObject(MUIC_Menuitem,
				MUIA_Menuitem_Title, NM_BARLABEL,
			TAG_END),
			MUIA_Group_Child, MUI_NewObject(MUIC_Menuitem,
				MUIA_Menuitem_Title, GetString(MSG_INPUTFIELD_EXTERNAL_EDIT),
				MUIA_UserData, IFV_ContextMenu_ExternalEdit,
			TAG_END),
			MUIA_Group_Child, MUI_NewObject(MUIC_Menuitem,
				MUIA_Menuitem_Title, GetString(MSG_INPUTFIELD_INSERT_TEXT_FILE),
				MUIA_UserData, IFV_ContextMenu_InsertFile,
			TAG_END),
		TAG_END),
	TAG_END);

	if(strip)
	{
		ULONG result = DoMethod(strip, MUIM_Menustrip_Popup, (IPTR)obj, 0, msg->mouse_x, msg->mouse_y);

		switch(result)
		{
			case IFV_ContextMenu_Cut:
				DoMethod(obj, MUIM_TextEditor_ARexxCmd, (IPTR)"Cut");
			break;

			case IFV_ContextMenu_Copy:
				DoMethod(obj, MUIM_TextEditor_ARexxCmd, (IPTR)"Copy");
			break;

			case IFV_ContextMenu_Paste:
				DoMethod(obj, MUIM_TextEditor_ARexxCmd, (IPTR)"Paste");
			break;

			case IFV_ContextMenu_PasteAndAccept:
				DoMethod(obj, MUIM_TextEditor_ARexxCmd, (IPTR)"Paste");
				set(obj, IFA_Acknowledge, TRUE);
			break;

			case IFV_ContextMenu_ExternalEdit:
				DoMethod(obj, IFM_ExternalEdit);
			break;

			case IFV_ContextMenu_InsertFile:
				DoMethod(obj, IFM_ImportTxtFile);
			break;

			case IFV_ContextMenu_Clear:
				DoMethod(obj, MUIM_TextEditor_ClearText);
			break;
		}

		MUI_DisposeObject(strip);
	}

	return(IPTR)0;
}

static IPTR InputFieldExternalEdit(Class *cl, Object *obj)
{
	BPTR fh;
	UBYTE buffer[50];

	FmtNPut(buffer, "ed T:" APP_NAME "%lu", sizeof(buffer), GetUniqueID());

	if((fh = Open(buffer + 3, MODE_NEWFILE)))
	{
		STRPTR txt;

		if((txt = (STRPTR)DoMethod(obj, IFM_ExportText)))
		{
			ULONG len = StrLen(txt);

			FWrite(fh, txt, len, 1);

			FreeVec(txt);
		}

		Close(fh);
	}

	Execute(buffer, (BPTR)NULL, (BPTR)NULL);

	DoMethod(obj, IFM_LoadTxtFile, (IPTR)buffer + 3);

	return (IPTR)0;
}

static IPTR InputFieldImportTxtFile(Class *cl, Object *obj)
{
	struct FileRequester *freq;

	set(_app(obj), MUIA_Application_Sleep, TRUE);

	if((freq = MUI_AllocAslRequestTags(ASL_FileRequest, TAG_END)))
	{
		if(MUI_AslRequestTags(freq,
			ASLFR_Window, _window(obj),
			ASLFR_TitleText, GetString(MSG_INPUTFIELD_INSERT_TEXT_FILE_ASL_TITLE),
			ASLFR_PositiveText, GetString(MSG_INPUTFIELD_INSERT_TEXT_FILE_ASL_POSITIVE),
			ASLFR_InitialPattern, "#?",
			ASLFR_DoPatterns, TRUE,
			ASLFR_RejectIcons, TRUE,
			TAG_END))
		{
			UBYTE location[500];

			StrNCopy(freq->fr_Drawer, (STRPTR)location, 500);
			AddPart((STRPTR)location, freq->fr_File, 500);

			DoMethod(obj, IFM_LoadTxtFile, (IPTR)location);

			DeleteFile(location);
		}
		MUI_FreeAslRequest(freq);
	}

	set(_app(obj), MUIA_Application_Sleep, FALSE);

	return (IPTR)0;
}

static IPTR InputFieldLoadTxtFile(Class *cl, Object *obj, struct IFP_LoadTxtFile *msg)
{
	BPTR fh;

	if((fh = Open(msg->path, MODE_OLDFILE)))
	{
		UBYTE buffer[1024];
		ULONG loaded;

		DoMethod(obj, MUIM_TextEditor_ClearText);
		set(obj, MUIA_TextEditor_Quiet, TRUE);

		do
		{
			loaded = Read(fh, buffer, sizeof(buffer) - 1);

			buffer[loaded] = 0x00;

			DoMethod(obj, MUIM_TextEditor_InsertText, (IPTR)buffer, MUIV_TextEditor_InsertText_Cursor);

		}while(loaded != 0);

		set(obj, MUIA_TextEditor_Quiet, FALSE);
		Close(fh);
	}

	return (IPTR)0;
}

static IPTR InputFieldExportText(Class *cl, Object *obj)
{
	STRPTR sys_text = (STRPTR)DoSuperMethod(cl, obj, MUIM_TextEditor_ExportText);
	if(sys_text)
		return (IPTR)SystemToUtf8(sys_text);
	return (IPTR)NULL;
}

static IPTR InputFieldDispatcher(VOID)
{
	Class *cl = (Class*)REG_A0;
	Object *obj = (Object*)REG_A2;
	Msg msg = (Msg)REG_A1;

	switch (msg->MethodID)
	{
		case OM_NEW:  return (InputFieldNew(cl, obj, (struct opSet*)msg));
		case OM_SET:  return (InputFieldSet(cl, obj, (struct opSet*)msg));
		case OM_GET:  return (InputFieldGet(cl, obj, (struct opGet*)msg));
		case MUIM_Setup: return(InputFieldSetup(cl, obj, (struct MUIP_Setup*)msg));
		case MUIM_Cleanup: return(InputFieldCleanup(cl, obj, (struct MUIP_Cleanup*)msg));
		case MUIM_HandleEvent: return(InputFieldHandleEvent(cl, obj, (struct MUIP_HandleEvent*)msg));
		case IFM_ContextMenu: return(InputFieldContextMenu(cl, obj, (struct IFP_ContextMenu*)msg));
		case IFM_ExternalEdit: return(InputFieldExternalEdit(cl, obj));
		case IFM_ImportTxtFile: return(InputFieldImportTxtFile(cl, obj));
		case IFM_LoadTxtFile: return(InputFieldLoadTxtFile(cl, obj, (struct IFP_LoadTxtFile*)msg));
		case IFM_ExportText: return(InputFieldExportText(cl, obj));
		default:  return (DoSuperMethodA(cl, obj, msg));
	}
}


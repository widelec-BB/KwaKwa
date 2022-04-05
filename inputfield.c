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

#ifndef MUIC_StringUnicode
	#define MUIC_StringUnicode "StringU.mui"
#endif /* MUIC_StringUnicode */

struct Library *TextEditorMCC;

struct MUI_CustomClass *InputFieldClass;
static IPTR InputFieldDispatcher(VOID);
const struct EmulLibEntry InputFieldGate = {TRAP_LIB, 0, (VOID(*)(VOID))InputFieldDispatcher};

struct MUI_CustomClass *InputFieldUnicodeClass;
static IPTR InputFieldUnicodeDispatcher(VOID);
const struct EmulLibEntry InputFieldUnicodeGate = {TRAP_LIB, 0, (VOID(*)(VOID))InputFieldUnicodeDispatcher};

#define IFV_ContextMenu_Copy             1
#define IFV_ContextMenu_Cut              2
#define IFV_ContextMenu_Paste            3
#define IFV_ContextMenu_Clear            4
#define IFV_ContextMenu_ExternalEdit     5
#define IFV_ContextMenu_InsertFile       6
#define IFV_ContextMenu_PasteAndAccept   7

struct IFP_ContextMenu {ULONG MethodID; LONG mouse_x; LONG mouse_y;};
struct IFP_LoadTxtFile {ULONG MethodID; STRPTR path;};
struct IFP_AppendText  {ULONG MethodID; STRPTR text;};

struct InputFieldData
{
	BOOL send_after_return;
	BOOL add_event_handler;
	struct MUI_EventHandlerNode handler;
	Object *talk_tab;

	BOOL has_changed;
	BOOL is_unicode;
};

struct MUI_CustomClass *CreateInputFieldClass(VOID)
{
	struct MUI_CustomClass *cl;

	if((TextEditorMCC = OpenLibrary("mui/TextEditor.mcc", 15)))
	{
		cl = MUI_CreateCustomClass(NULL, MUIC_TextEditor, NULL, sizeof(struct InputFieldData), (APTR)&InputFieldGate);
		InputFieldClass = cl;
		return cl;
	}

	InputFieldClass = NULL;
	return NULL;
}

VOID DeleteInputFieldClass(VOID)
{
	if (InputFieldClass)
		MUI_DeleteCustomClass(InputFieldClass);

	if(TextEditorMCC)
		CloseLibrary(TextEditorMCC);
}

struct MUI_CustomClass *CreateInputFieldUnicodeClass(VOID)
{
	struct MUI_CustomClass *cl;

	cl = MUI_CreateCustomClass(NULL, MUIC_StringUnicode, NULL, sizeof(struct InputFieldData), (APTR)&InputFieldUnicodeGate);
	InputFieldUnicodeClass = cl;
	return cl;
}

VOID DeleteInputFieldUnicodeClass(VOID)
{
	if (InputFieldUnicodeClass)
		MUI_DeleteCustomClass(InputFieldUnicodeClass);
}

static IPTR InputFieldNew(Class *cl, Object *obj, struct opSet *msg)
{
	obj = (Object *)DoSuperNew(cl, obj,
		MUIA_CycleChain, TRUE,
		MUIA_TextEditor_Rows, 4,
	TAG_MORE, (IPTR)msg->ops_AttrList);

	if(obj)
	{
		struct InputFieldData *d = INST_DATA(cl, obj);

		d->send_after_return = TRUE;
		d->is_unicode = FALSE;
		d->add_event_handler = GetTagData(IFA_AddEventHandler, FALSE, msg->ops_AttrList);

		DoMethod(obj, MUIM_Notify, MUIA_TextEditor_HasChanged, MUIV_EveryTime, obj, 3, MUIM_Set, IFA_HasChanged, MUIV_TriggerValue);
		return (IPTR)obj;
	}
	CoerceMethod(cl, obj, OM_DISPOSE);
	return (IPTR)NULL;
}

static IPTR InputFieldUnicodeNew(Class *cl, Object *obj, struct opSet *msg)
{
	obj = (Object *)DoSuperNew(cl, obj,
		MUIA_Unicode, TRUE,
		MUIA_CycleChain, TRUE,
		MUIA_Frame, MUIV_Frame_String,
		MUIA_Background, MUII_StringBack,
		MUIA_String_MaxLen, 8192,
		MUIA_String_AdvanceOnCR, TRUE,
	TAG_MORE, (IPTR)msg->ops_AttrList);

	if(obj)
	{
		struct InputFieldData *d = INST_DATA(cl, obj);

		d->send_after_return = TRUE;
		d->is_unicode = TRUE;
		d->add_event_handler = GetTagData(IFA_AddEventHandler, FALSE, msg->ops_AttrList);

		DoMethod(obj, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime, obj, 3, MUIM_Set, IFA_HasChanged, TRUE);

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
				d->talk_tab = (Object *)tag->ti_Data;
				tagcount++;
				break;

			case IFA_HasChanged:
				d->has_changed = (BOOL)tag->ti_Data;
				tagcount++;
				break;

			case IFA_TextContents:
				if(d->is_unicode)
					set(obj, MUIA_String_Contents, tag->ti_Data);
				else
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
	struct InputFieldData *d = INST_DATA(cl, obj);
	int rv = FALSE;

	switch(msg->opg_AttrID)
	{
		case IFA_Acknowledge:
			return TRUE;
			break;

		case IFA_HasChanged:
			*msg->opg_Storage = d->has_changed;
			d->has_changed = FALSE;
			if(!d->is_unicode)
				nnset(obj, MUIA_TextEditor_HasChanged, FALSE);
			return TRUE;

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

	if(result && d->add_event_handler)
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

	if(d->add_event_handler)
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
				if(d->is_unicode)
					return (IPTR)0; /* this is one line gadget, so adding new line is unsupported */
				else
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
				IPTR text = (IPTR)xget(prefs_object(USD_PREFS_FKEYS_STRING(msg->imsg->Code - RAWKEY_F1)), MUIA_String_Contents);
				DoMethod(obj, IFM_AppendText, text);
				return MUI_EventHandlerRC_Eat;
			}

			if(msg->imsg->Code == RAWKEY_F11)
			{
				IPTR text = (IPTR)xget(prefs_object(USD_PREFS_FKEYS_STRING(10)), MUIA_String_Contents);
				DoMethod(obj, IFM_AppendText, text);
				return MUI_EventHandlerRC_Eat;
			}

			if(msg->imsg->Code == RAWKEY_F12)
			{
				IPTR text = (IPTR)xget(prefs_object(USD_PREFS_FKEYS_STRING(11)), MUIA_String_Contents);
				DoMethod(obj, IFM_AppendText, text);
				return MUI_EventHandlerRC_Eat;
			}
		}
		else if(msg->imsg->Class == IDCMP_MOUSEBUTTONS)
		{
			if(_isinobject(msg->imsg->MouseX, msg->imsg->MouseY))
			{
				if(msg->imsg->Code == IECODE_RBUTTON && !d->is_unicode)
				{
					DoMethod(obj, IFM_ContextMenu, msg->imsg->MouseX, msg->imsg->MouseY);
					return MUI_EventHandlerRC_Eat;
				}
			}
		}
	}

	if(!d->is_unicode)
		return 0;

	return (IPTR)DoSuperMethodA(cl, obj, msg);
}

static IPTR InputFieldContextMenu(Class *cl, Object *obj, struct IFP_ContextMenu *msg)
{
	Object *strip;

	strip = MUI_NewObject(MUIC_Menustrip,
		MUIA_Unicode, TRUE,
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
				DoMethod(obj, IFM_Clear);
			break;
		}

		MUI_DisposeObject(strip);
	}

	return(IPTR)0;
}

static IPTR InputFieldExternalEdit(Class *cl, Object *obj)
{
	struct InputFieldData *d = INST_DATA(cl, obj);
	BPTR fh;
	UBYTE buffer[50];

	FmtNPut(buffer, "ed T:" APP_NAME "%lu", sizeof(buffer), GetUniqueID());

	if((fh = Open(buffer + 3, MODE_NEWFILE)))
	{
		STRPTR txt;

		if(d->is_unicode)
			txt = Utf8ToSystem((STRPTR)xget(obj, MUIA_String_Contents));
		else
			txt = (STRPTR)DoMethod(obj, MUIM_TextEditor_ExportText);

		if(txt)
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
	struct InputFieldData *d = INST_DATA(cl, obj);
	BPTR fh;

	if((fh = Open(msg->path, MODE_OLDFILE)))
	{
		UBYTE buffer[1024];
		ULONG loaded;

		DoMethod(obj, IFM_Clear);

		if(!d->is_unicode)
			set(obj, MUIA_TextEditor_Quiet, TRUE);

		do
		{
			loaded = Read(fh, buffer, sizeof(buffer) - 1);

			buffer[loaded] = 0x00;

			if(d->is_unicode)
			{
				STRPTR txt_unicode = SystemToUtf8(buffer);
				if(txt_unicode)
				{
					DoMethod(obj, IFM_AppendText, (IPTR)txt_unicode);
					StrFree(txt_unicode);
				}
			}
			else
				DoMethod(obj, MUIM_TextEditor_InsertText, buffer, MUIV_TextEditor_InsertText_Bottom);
		}
		while(loaded != 0);

		if(!d->is_unicode)
			set(obj, MUIA_TextEditor_Quiet, FALSE);

		Close(fh);
	}

	return (IPTR)0;
}

static IPTR InputFieldExportText(Class *cl, Object *obj)
{
	struct InputFieldData *d = INST_DATA(cl, obj);
	STRPTR unicode_text = NULL;

	if(!d->is_unicode)
	{
		STRPTR sys_text = (STRPTR)DoSuperMethod(cl, obj, MUIM_TextEditor_ExportText);
		if(sys_text != NULL)
		{
			unicode_text = SystemToUtf8(sys_text);
			FreeVec(sys_text);
		}
	}
	else
		unicode_text = StrNew((STRPTR)xget(obj, MUIA_String_Contents));

	return (IPTR)unicode_text;
}

static IPTR InputFieldAppendText(Class *cl, Object *obj, struct IFP_AppendText *msg)
{
	struct InputFieldData *d = INST_DATA(cl, obj);
	STRPTR new;

	if(!d->is_unicode)
	{
		if((new = Utf8ToSystem(msg->text)))
		{
			DoMethod(obj, MUIM_TextEditor_InsertText, new, MUIV_TextEditor_InsertText_Cursor);
			StrFree(new);
		}
	}
	else
	{
		new = FmtNew("%s%s", xget(obj, MUIA_String_Contents), msg->text);
		if(new)
		{
			set(obj, MUIA_String_Contents, new);
			StrFree(new);
		}
	}
	return (IPTR)0;
}

static IPTR InputFieldClear(Class *cl, Object *obj)
{
	struct InputFieldData *d = INST_DATA(cl, obj);

	if(d->is_unicode)
		set(obj, MUIA_String_Contents, NULL);
	else
		DoMethod(obj, MUIM_TextEditor_ClearText);

	return (IPTR)0;
}

static IPTR InputFieldDispatcher(VOID)
{
	Class *cl = (Class *)REG_A0;
	Object *obj = (Object *)REG_A2;
	Msg msg = (Msg)REG_A1;

	switch(msg->MethodID)
	{
		case OM_NEW: return(InputFieldNew(cl, obj, (struct opSet *)msg));
		case OM_SET: return(InputFieldSet(cl, obj, (struct opSet *)msg));
		case OM_GET: return(InputFieldGet(cl, obj, (struct opGet *)msg));
		case MUIM_Setup: return(InputFieldSetup(cl, obj, (struct MUIP_Setup *)msg));
		case MUIM_Cleanup: return(InputFieldCleanup(cl, obj, (struct MUIP_Cleanup *)msg));
		case MUIM_HandleEvent: return(InputFieldHandleEvent(cl, obj, (struct MUIP_HandleEvent *)msg));
		case IFM_ContextMenu: return(InputFieldContextMenu(cl, obj, (struct IFP_ContextMenu *)msg));
		case IFM_ExternalEdit: return(InputFieldExternalEdit(cl, obj));
		case IFM_ImportTxtFile: return(InputFieldImportTxtFile(cl, obj));
		case IFM_LoadTxtFile: return(InputFieldLoadTxtFile(cl, obj, (struct IFP_LoadTxtFile *)msg));
		case IFM_ExportText: return(InputFieldExportText(cl, obj));
		case IFM_AppendText: return(InputFieldAppendText(cl, obj, (struct IFP_AppendText *)msg));
		case IFM_Clear: return(InputFieldClear(cl, obj));
		default: return(DoSuperMethodA(cl, obj, msg));
	}
}

static IPTR InputFieldUnicodeDispatcher(VOID)
{
	Class *cl = (Class *)REG_A0;
	Object *obj = (Object *)REG_A2;
	Msg msg = (Msg)REG_A1;

	switch(msg->MethodID)
	{
		case OM_NEW: return(InputFieldUnicodeNew(cl, obj, (struct opSet *)msg));
		case OM_SET: return(InputFieldSet(cl, obj, (struct opSet *)msg));
		case OM_GET: return(InputFieldGet(cl, obj, (struct opGet *)msg));
		case MUIM_Setup: return(InputFieldSetup(cl, obj, (struct MUIP_Setup *)msg));
		case MUIM_Cleanup: return(InputFieldCleanup(cl, obj, (struct MUIP_Cleanup *)msg));
		case MUIM_HandleEvent: return(InputFieldHandleEvent(cl, obj, (struct MUIP_HandleEvent *)msg));
		case IFM_ExternalEdit: return(InputFieldExternalEdit(cl, obj));
		case IFM_LoadTxtFile: return(InputFieldLoadTxtFile(cl, obj, (struct IFP_LoadTxtFile *)msg));
		case IFM_ExportText: return(InputFieldExportText(cl, obj));
		case IFM_AppendText: return(InputFieldAppendText(cl, obj, (struct IFP_AppendText *)msg));
		case IFM_Clear: return(InputFieldClear(cl, obj));
		default: return(DoSuperMethodA(cl, obj, msg));
	}
}


/*
 * Copyright (c) 2012-2018 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/asl.h>
#include <devices/rawkeycodes.h>
#include <mui/Lamp_mcc.h>
#include <libvstring.h>

#include "globaldefines.h"
#include "locale.h"

#include "virtualtext.h"
#include "prefswindow.h"
#include "contactslist.h"
#include "application.h"
#include "inputfield.h"
#include "logs.h"
#include "contactinfoblock.h"
#include "talkwindow.h"
#include "editconwindow.h"
#include "talktab.h"

#include "support.h"

#include "kwakwa_api/defs.h"

#define WRITE_LAMP_TIME 4000

struct MUI_CustomClass *TalkTabClass;
static IPTR TalkTabDispatcher(void);
const struct EmulLibEntry TalkTabGate = {TRAP_LIB, 0, (void(*)(void))TalkTabDispatcher};

struct TTBP_Init {ULONG MethodID; struct ContactEntry *contact;};
struct TTBP_PutMessage {ULONG MethodID; STRPTR sender; STRPTR message; ULONG timestamp;};
struct TTBP_SendMessage {ULONG MethodID; STRPTR message; ULONG to_send; Object *input;};
struct TTBP_ChangeLamp {ULONG MethodID; ULONG lamp_state;};
struct TTBP_PutPicture {ULONG MethodID; STRPTR sender; ULONG timestamp; APTR pic; ULONG pic_size;};
struct TTBP_PutInvite {ULONG MethodID; ULONG pluginid; STRPTR sender; ULONG timestamp;};
struct TTBP_PubdirParseResponse {ULONG MethodID; struct ContactEntry *response;};
struct TTBP_InsertMessage {ULONG MethodID; STRPTR message; ULONG timestamp; LONG isown; LONG isold;};
struct TTBP_AddToHistory {ULONG MethodID; STRPTR content; ULONG timestamp; ULONG flags;};
struct TTBP_InsertOldMessage {ULONG MethodID; ULONG flags; ULONG timestamp; STRPTR contactid; UBYTE *content; ULONG content_len;};

struct TalkTabData
{
	Object *info_block;
	Object *txt;
	Object *input, *sec_input;
	Object *send_but, *return_check, *lamp;
	Object *send_pic_but, *clear_txt_but, *open_log_but, *double_but, *edit_contact_button;
	struct ContactEntry *contact; /* pointer to local copy, not to entry from list -> needs update */
	BPTR log_fh;
	struct MUI_InputHandlerNode ihnode;
	BOOL ihnode_added;
	QUAD conversation_id;
	BOOL double_hidden;

	struct ClockData last_message_cd;
};

struct MUI_CustomClass *CreateTalkTabClass(void)
{
	struct MUI_CustomClass *cl;

	cl = MUI_CreateCustomClass(NULL, MUIC_Group, NULL, sizeof(struct TalkTabData), (APTR)&TalkTabGate);
	TalkTabClass = cl;
	return cl;
}

void DeleteTalkTabClass(void)
{
	if (TalkTabClass) MUI_DeleteCustomClass(TalkTabClass);
}

static VOID TalkTabNotifications(Class *cl, Object *obj)
{
	struct TalkTabData *d = INST_DATA(cl, obj);

	DoMethod(d->input, MUIM_Notify, IFA_Acknowledge, MUIV_EveryTime, obj, 4,
	 TTBM_SendMessage, NULL, TRUE, d->input);

	DoMethod(d->input, MUIM_Notify, IFA_Acknowledge, MUIV_EveryTime, MUIV_Notify_Self, 1,
	 MUIM_TextEditor_ClearText);

	DoMethod(d->input, MUIM_Notify, IFA_Acknowledge, MUIV_EveryTime, obj, 1,
	 TTBM_Activate);

	DoMethod(d->sec_input, MUIM_Notify, IFA_Acknowledge, MUIV_EveryTime, obj, 4,
	 TTBM_SendMessage, NULL, TRUE, d->sec_input);

	DoMethod(d->sec_input, MUIM_Notify, IFA_Acknowledge, MUIV_EveryTime, MUIV_Notify_Self, 1,
	 MUIM_TextEditor_ClearText);

	DoMethod(d->sec_input, MUIM_Notify, IFA_Acknowledge, MUIV_EveryTime, obj, 1,
	 TTBM_Activate);

	DoMethod(d->send_but, MUIM_Notify, MUIA_Pressed, FALSE, obj, 4,
	 TTBM_SendMessage, NULL, TRUE, d->input);

	DoMethod(d->send_but, MUIM_Notify, MUIA_Pressed, FALSE, d->input, 1,
	 MUIM_TextEditor_ClearText);

	DoMethod(d->send_but, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1,
	 TTBM_Activate);

	DoMethod(d->return_check, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, d->input, 3,
	 MUIM_Set, IFA_SendAfterReturn, MUIV_TriggerValue);

	DoMethod(d->input, MUIM_Notify, MUIA_TextEditor_HasChanged, TRUE, obj, 1,
	 TTBM_SendWriteNotify);

	DoMethod(d->send_pic_but, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1,
	 TTBM_SendPicture);

	DoMethod(d->clear_txt_but, MUIM_Notify, MUIA_Pressed, FALSE, d->txt, 1,
	 VTM_Clear);

	DoMethod(d->open_log_but, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1,
	 TTBM_OpenLogFile);

	DoMethod(d->double_but, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1,
	 TTBM_ToggleDouble);

	DoMethod(d->edit_contact_button, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, TTBM_EditContact);
}

static IPTR TalkTabNew(Class *cl, Object *obj, struct opSet *msg)
{
	Object *txt, *input, *send_but, *return_check, *lamp, *info_block, *scroll, *sec_input;
	Object *toolbar, *send_pic_but, *clear_txt_but, *open_log_but, *double_button;
	Object *edit_contact_button;

	obj = DoSuperNew(cl, obj,
		MUIA_Group_Child, (info_block = NewObject(ContactInfoBlockClass->mcc_Class, NULL, TAG_END)),
		MUIA_Group_Child, (scroll = MUI_NewObject(MUIC_Scrollgroup,
			MUIA_Scrollgroup_FreeHoriz, FALSE,
			MUIA_Scrollgroup_UseWinBorder, TRUE,
			MUIA_Scrollgroup_Contents, (txt = NewObject(VirtualTextClass->mcc_Class, NULL,
				MUIA_Background, MUII_ReadListBack,
				MUIA_Frame, MUIV_Frame_ReadList,
				MUIA_UserData, USD_TALKTAB_VIRTUALTEXT,
			TAG_END)),
		TAG_END)),
		MUIA_Group_Child, (toolbar = MUI_NewObject(MUIC_Group,
			MUIA_Group_Horiz, TRUE,
			MUIA_Group_HorizSpacing, 0,
			MUIA_Group_Child, (clear_txt_but = MUI_NewObject(MUIC_Text,
				MUIA_Text_Contents, "\33I[4:PROGDIR:gfx/toolbar/clean.mbr]",
				MUIA_Text_PreParse, "\33c",
				MUIA_Frame, MUIV_Frame_ImageButton,
				MUIA_Background, MUII_ImageButtonBack,
				MUIA_InputMode, MUIV_InputMode_RelVerify,
				MUIA_CycleChain, TRUE,
				MUIA_HorizWeight, 0,
				MUIA_ShortHelp, GetString(MSG_TALKTAB_BUTTON_CLEAR),
			TAG_END)),
			MUIA_Group_Child, (send_pic_but = MUI_NewObject(MUIC_Text,
				MUIA_Text_Contents, "\33I[4:PROGDIR:gfx/toolbar/sendimage.mbr]",
				MUIA_Text_PreParse, "\33c",
				MUIA_Frame, MUIV_Frame_ImageButton,
				MUIA_Background, MUII_ImageButtonBack,
				MUIA_InputMode, MUIV_InputMode_RelVerify,
				MUIA_CycleChain, TRUE,
				MUIA_HorizWeight, 0,
				MUIA_ShortHelp, GetString(MSG_TALKTAB_BUTTON_SEND_PICTURE),
				MUIA_UserData, USD_TALKTAB_SEND_PICTURE,
			TAG_END)),
			MUIA_Group_Child, (open_log_but = MUI_NewObject(MUIC_Text,
				MUIA_Text_Contents, "\33I[4:PROGDIR:gfx/toolbar/log.mbr]",
				MUIA_Text_PreParse, "\33c",
				MUIA_Frame, MUIV_Frame_ImageButton,
				MUIA_Background, MUII_ImageButtonBack,
				MUIA_InputMode, MUIV_InputMode_RelVerify,
				MUIA_CycleChain, TRUE,
				MUIA_HorizWeight, 0,
				MUIA_ShortHelp, GetString(MSG_TALKTAB_BUTTON_OPEN_LOG),
				MUIA_UserData, USD_TALKTAB_OPEN_LOG,
			TAG_END)),
			MUIA_Group_Child, (double_button = MUI_NewObject(MUIC_Text,
				MUIA_Text_Contents, "\33I[4:PROGDIR:gfx/toolbar/double.mbr]",
				MUIA_Text_PreParse, "\33c",
				MUIA_Frame, MUIV_Frame_ImageButton,
				MUIA_Background, MUII_ImageButtonBack,
				MUIA_InputMode, MUIV_InputMode_RelVerify,
				MUIA_CycleChain, TRUE,
				MUIA_HorizWeight, 0,
				MUIA_ShortHelp, GetString(MSG_TALKTAB_BUTTON_DOUBLE),
			TAG_END)),
			MUIA_Group_Child, (edit_contact_button = MUI_NewObject(MUIC_Text,
				MUIA_Text_Contents, "\33I[4:PROGDIR:gfx/toolbar/editcon.mbr]",
				MUIA_Text_PreParse, "\33c",
				MUIA_Frame, MUIV_Frame_ImageButton,
				MUIA_Background, MUII_ImageButtonBack,
				MUIA_InputMode, MUIV_InputMode_RelVerify,
				MUIA_CycleChain, TRUE,
				MUIA_HorizWeight, 0,
				MUIA_ShortHelp, GetString(MSG_TALKTAB_BUTTON_EDIT_CONTACT),
			TAG_END)),
			MUIA_Group_Child, EmptyRectangle(100),
		TAG_END)),
		MUIA_Group_Child, (input = NewObject(InputFieldClass->mcc_Class, NULL,
			MUIA_UserData, USD_TALKTAB_INPUT_FIRST,
		TAG_END)),
		MUIA_Group_Child, (sec_input = NewObject(InputFieldClass->mcc_Class, NULL,
			MUIA_UserData, USD_TALKTAB_INPUT_SECOND,
			MUIA_ShowMe, FALSE,
		TAG_END)),
		MUIA_Group_Child, MUI_NewObject(MUIC_Group,
			MUIA_Group_Horiz, TRUE,
			MUIA_Group_Child, (return_check = MUI_NewObject(MUIC_Image,
				MUIA_ObjectID, USD_TALKTAB_RETURN_CHECK,
				MUIA_UserData, USD_TALKTAB_RETURN_CHECK,
				MUIA_Image_Spec, "6:15",
				MUIA_ShowSelState, FALSE,
				MUIA_Selected, TRUE,
				MUIA_InputMode, MUIV_InputMode_Toggle,
				MUIA_CycleChain, TRUE,
			TAG_END)),
			MUIA_Group_Child, StringLabel(GetString(MSG_TALKTAB_RETURN_CHECK), "\33l"),
			MUIA_Group_Child, EmptyRectangle(100),
			MUIA_Group_Child, (lamp = MUI_NewObject(MUIC_Lamp,
				MUIA_Lamp_Type, MUIV_Lamp_Type_Huge,
				MUIA_Lamp_Color, MUIV_Lamp_Color_Ok,
			TAG_END)),
			MUIA_Group_Child, (send_but = NormalButton(GetString(MSG_TALKTAB_SEND_BUTTON), 0, 0, 0)),
		TAG_END),

	TAG_MORE, msg->ops_AttrList);

	if(obj)
	{
		struct TalkTabData *d = INST_DATA(cl, obj);

		d->txt = txt;
		d->input = input;
		d->return_check = return_check;
		d->send_but = send_but;
		d->lamp = lamp;
		d->info_block = info_block;
		d->send_pic_but = send_pic_but;
		d->clear_txt_but = clear_txt_but;
		d->sec_input = sec_input;
		d->open_log_but = open_log_but;
		d->double_but = double_button;
		d->edit_contact_button = edit_contact_button;
		d->conversation_id = -1;
		d->double_hidden = TRUE;

		set(toolbar, MUIA_ShowMe, xget(prefs_object(USD_PREFS_TW_TOOLBAR_ONOFF), MUIA_Selected));
		set(info_block, MUIA_ShowMe, xget(prefs_object(USD_PREFS_TW_CONTACTINFOBLOCK_ONOFF), MUIA_Selected));

		set(d->input, IFA_TalkTab, obj);
		set(d->sec_input, IFA_TalkTab, obj);

		DoMethod(prefs_object(USD_PREFS_TW_TOOLBAR_ONOFF), MUIM_Notify, MUIA_Selected, MUIV_EveryTime, toolbar, 3,
		 MUIM_Set, MUIA_ShowMe, MUIV_TriggerValue);
		DoMethod(prefs_object(USD_PREFS_TW_CONTACTINFOBLOCK_ONOFF), MUIM_Notify, MUIA_Selected, MUIV_EveryTime, info_block, 3,
		 MUIM_Set, MUIA_ShowMe, MUIV_TriggerValue);

		d->ihnode.ihn_Object  = obj;
		d->ihnode.ihn_Millis  = WRITE_LAMP_TIME;
		d->ihnode.ihn_Method  = TTBM_CleanLampState;
		d->ihnode.ihn_Flags   = MUIIHNF_TIMER;

		return (IPTR)obj;
	}

	CoerceMethod(cl, obj, OM_DISPOSE);
	return (IPTR)NULL;
}

static IPTR TalkTabCleanup(Class *cl, Object *obj, struct MUIP_Cleanup *msg)
{
	struct TalkTabData *d = INST_DATA(cl, obj);

	if(d->ihnode_added)
		DoMethod(_app(obj), MUIM_Application_RemInputHandler, &d->ihnode);

	return (DoSuperMethodA(cl, obj, msg));
}

static IPTR TalkTabDispose(Class *cl, Object *obj, Msg msg)
{
	struct TalkTabData *d = INST_DATA(cl, obj);

	LogClose(d->log_fh);

	return 0;
}

static IPTR TalkTabGet(Class *cl, Object *obj, struct opGet *msg)
{
	struct TalkTabData *d = INST_DATA(cl, obj);
	int rv = FALSE;

	switch(msg->opg_AttrID)
	{
		case TTBA_ContactEntry:
			*msg->opg_Storage = (IPTR)d->contact;
		return TRUE;

		default:
			rv = (DoSuperMethodA(cl, obj, (Msg)msg));
	}

	return rv;
}

static IPTR TalkTabInit(Class *cl, Object *obj, struct TTBP_Init *msg)
{
	struct TalkTabData *d = INST_DATA(cl, obj);

	d->contact = msg->contact;

	set(d->info_block, CIBA_ContactEntry, d->contact);

	TalkTabNotifications(cl, obj);

	if(xget(prefs_object(USD_PREFS_LOGS_ONOFF_CHECK), MUIA_Selected))
	{
		d->log_fh = LogOpen(ContactName(msg->contact));
	}

	return (IPTR)1;
}

/* adds recived message to list */
static IPTR TalkTabPutMessage(Class *cl, Object *obj, struct TTBP_PutMessage *msg)
{
	struct TalkTabData *d = INST_DATA(cl, obj);
	BOOL result = FALSE;

	LogAdd(d->log_fh, msg->sender, msg->message, msg->timestamp); /* adds recived message to log */

	DoMethod(obj, TTBM_AddToHistory, msg->message, msg->timestamp, HISTORY_MESSAGES_NORMAL | HISTORY_MESSAGES_FRIEND);

	if(DoMethod(d->txt, VTM_InitChange))
	{
		DoMethod(d->txt, VTM_AddMessageHeadLine, msg->sender ? msg->sender : ContactName(d->contact), msg->timestamp, 0);
		DoMethod(d->txt, VTM_AddMessage, msg->message, VTV_Incoming);
		DoMethod(d->txt, VTM_ExitChange);
		result = TRUE;
	}

	set(d->lamp, MUIA_Lamp_Color, MUIV_Lamp_Color_Ok);

	return (IPTR)result;
}

/* adds my own message to list */
static IPTR TalkTabSendMessage(Class *cl, Object *obj, struct TTBP_SendMessage *msg)
{
	struct TalkTabData *d = INST_DATA(cl, obj);
	STRPTR message, user_name = (STRPTR)xget(prefs_object(USD_PREFS_TW_USRNAME_STRING), MUIA_String_Contents);
	ULONG timestamp;
	struct DateStamp ds;

	DateStamp(&ds);
	timestamp = LocalToUTC(ds.ds_Days * 24 * 60 * 60 + ds.ds_Minute * 60 + ds.ds_Tick / TICKS_PER_SECOND, NULL);

	message = msg->message ? StrNew(msg->message) : (STRPTR)DoMethod(msg->input, MUIM_TextEditor_ExportText);

	if(message)
	{
		if(StrEqu(message, "")) /* to avoid trying to send empty message */
		{
			FreeVec(message);
			return (IPTR)0;
		}

		user_name = StrNew(user_name ? user_name : GetString(MSG_PREFS_TALKWINDOW_USER_USRNAME_DEAFAULT));

		if(user_name != NULL)
		{
			if((BOOL)msg->to_send)
				DoMethod(_app(obj), APPM_SendMessage, d->contact->pluginid, d->contact->entryid, message);

			if(DoMethod(d->txt, VTM_InitChange))
			{
				DoMethod(d->txt, VTM_AddMessageHeadLine, user_name, timestamp, 0);
				DoMethod(d->txt, VTM_AddMessage, message, VTV_Outgoing);
				DoMethod(d->txt, VTM_ExitChange);
			}

			LogAdd(d->log_fh, user_name, message, timestamp); /* adds my own message to log */

			DoMethod(obj, TTBM_AddToHistory, message, timestamp, HISTORY_MESSAGES_NORMAL | HISTORY_MESSAGES_MY);

			FreeVec(message);
			StrFree(user_name);

			return (IPTR)1;
		}
		FreeVec(message);
	}

	MUI_Request(_app(obj), _win(obj), 0L, APP_NAME, "*_OK", GetString(MSG_SENDMSG_FAILED), NULL);
	return (IPTR)0;
}

static IPTR TalkTabActivateString(Class *cl, Object *obj)
{
	struct TalkTabData *d = INST_DATA(cl, obj);

	nnset(_win(d->input), MUIA_Window_ActiveObject, d->input);
	return (IPTR)1;
}

static IPTR TalkTabNewStatus(Class *cl, Object *obj)
{
	struct TalkTabData *d = INST_DATA(cl, obj);
	IPTR result = 0;
	STRPTR buffer;
	ULONG len = d->contact->statusdesc ? StrLen(d->contact->statusdesc) + 100 : 100;
	ULONG timestamp = LocalToUTC(ActLocalTime2Amiga(), NULL);
	ENTER();

	if((buffer = AllocVec(len, MEMF_ANY)))
	{
		StrCopy(ContactName(d->contact), buffer);
		StrCat(" ", buffer);
		StrCat(GetString(MSG_TALKTAB_STATUS_CHANGE), buffer);
		StrCat(" ", buffer);

		if(KWA_S_NAVAIL(d->contact->status) || KWA_S_FRESH(d->contact->status))
			StrCat(GetString(MSG_GG_STATUS_UNAVAIL_SMALL), buffer);
		if(KWA_S_AVAIL(d->contact->status))
			StrCat(GetString(MSG_GG_STATUS_AVAIL_SMALL), buffer);
		if(KWA_S_BUSY(d->contact->status))
			StrCat(GetString(MSG_GG_STATUS_AWAY_SMALL), buffer);
		if(KWA_S_FFC(d->contact->status))
			StrCat(GetString(MSG_GG_STATUS_FFC_SMALL), buffer);
		if(KWA_S_DND(d->contact->status))
			StrCat(GetString(MSG_GG_STATUS_DND_SMALL), buffer);
		if(KWA_S_BLOCKED(d->contact->status))
			StrCat(GetString(MSG_GG_STATUS_BLOCKED_SMALL), buffer);
		if(KWA_S_INVISIBLE(d->contact->status))
			StrCat(GetString(MSG_GG_STATUS_INVISIBLE_SMALL), buffer);

		if(d->contact->statusdesc)
		{
			StrCat(" ", buffer);
			StrCat(GetString(MSG_WITH_DESCRIPTION_SMALL), buffer);
			StrCat(": ", buffer);
			StrCat(d->contact->statusdesc, buffer);
		}

		if(DoMethod(d->txt, VTM_InitChange))
		{
			result = DoMethod(d->txt, VTM_AddSystemMessage, buffer, timestamp, 0);
			DoMethod(d->txt, VTM_ExitChange);
		}

		DoMethod(obj, TTBM_AddToHistory, buffer, timestamp, HISTORY_MESSAGES_SYSTEM);

		FreeVec(buffer);
	}

	LEAVE();
	return result;
}

static IPTR TalkTabShowEnd(Class *cl, Object *obj)
{
	struct TalkTabData *d = INST_DATA(cl, obj);

	return (IPTR)DoMethod(d->txt, VTM_ShowEnd);
}

static IPTR TalkTabSendWriteNotify(Class *cl, Object *obj)
{
	struct TalkTabData *d = INST_DATA(cl, obj);
	STRPTR new_msg = (STRPTR)DoMethod(d->input, MUIM_TextEditor_ExportText);

	if(new_msg != NULL)
	{
		ULONG msg_len = StrLen(new_msg);

		if(msg_len > 0)
			DoMethod(_app(obj), APPM_SendTypingNotify, d->contact->pluginid, d->contact->entryid, msg_len);

		FreeVec(new_msg);
	}

	set(d->input, MUIA_TextEditor_HasChanged, FALSE);

	return (IPTR)1;
}

static IPTR TalkTabCleanLampState(Class *cl, Object *obj)
{
	struct TalkTabData *d = INST_DATA(cl, obj);

	set(d->lamp, MUIA_Lamp_Color, MUIV_Lamp_Color_Ok);

	if(d->ihnode_added)
	{
		DoMethod(_app(obj), MUIM_Application_RemInputHandler, &d->ihnode);
		d->ihnode_added = FALSE;
	}

	return (IPTR)0;
}

static IPTR TalkTabSetLampState(Class *cl, Object *obj)
{
	struct TalkTabData *d = INST_DATA(cl, obj);

	set(d->lamp, MUIA_Lamp_Color, MUIV_Lamp_Color_FatalError);

	if(d->ihnode_added == FALSE)
	{
		DoMethod(_app(obj), MUIM_Application_AddInputHandler, &d->ihnode);
		d->ihnode_added = TRUE;
	}

	return (IPTR)1;
}

static IPTR TalkTabPubdirParseResponse(Class *cl, Object *obj, struct TTBP_PubdirParseResponse *msg)
{
	struct TalkTabData *d = INST_DATA(cl, obj);

	if(msg->response)
	{
		BYTE buffer[1024];
		ULONG timestamp = LocalToUTC(ActLocalTime2Amiga(), NULL);
		BOOL addEmpty = TRUE;

		DoMethod(_app(obj), APPM_ConvertContactEntry, msg->response);

		StrCopy(GetString(MSG_TALKTAB_PUBDIR_DATA), (STRPTR)buffer);

		if(msg->response->firstname)
		{
			StrCat(GetString(MSG_TALKTAB_PUBDIR_FIRSTNAME), (STRPTR)buffer);
			StrCat(msg->response->firstname, (STRPTR)buffer);
			StrCat("\n", (STRPTR)buffer);
			addEmpty = FALSE;
		}

		if(msg->response->lastname)
		{
			StrCat(GetString(MSG_TALKTAB_PUBDIR_LASTNAME), (STRPTR)buffer);
			StrCat(msg->response->lastname, (STRPTR)buffer);
			StrCat("\n", (STRPTR)buffer);
			addEmpty = FALSE;
		}

		if(msg->response->nickname)
		{
			StrCat(GetString(MSG_TALKTAB_PUBDIR_NICKNAME), (STRPTR)buffer);
			StrCat(msg->response->nickname, (STRPTR)buffer);
			StrCat("\n", (STRPTR)buffer);
			addEmpty = FALSE;
		}

		if(msg->response->birthyear)
		{
			StrCat(GetString(MSG_TALKTAB_PUBDIR_BIRTHYEAR), (STRPTR)buffer);
			StrCat(msg->response->birthyear, (STRPTR)buffer);
			StrCat("\n", (STRPTR)buffer);
			addEmpty = FALSE;
		}

		if(msg->response->gender != GENDER_UNKNOWN)
		{
			StrCat(GetString(MSG_TALKTAB_PUBDIR_GENDER), (STRPTR)buffer);
			switch(msg->response->gender)
			{
				case GENDER_MALE:
					StrCat(GetString(MSG_GENDER_MALE), (STRPTR)buffer);
				break;

				case GENDER_FEMALE:
					StrCat(GetString(MSG_GENDER_FEMALE), (STRPTR)buffer);
				break;
			}
			StrCat("\n", (STRPTR)buffer);
			addEmpty = FALSE;
		}

		if(msg->response->city)
		{
			StrCat(GetString(MSG_TALKTAB_PUBDIR_CITY), (STRPTR)buffer);
			StrCat(msg->response->city, (STRPTR)buffer);
			addEmpty = FALSE;
		}

		if(addEmpty)
		{
			StrCat(GetString(MSG_TALKTAB_PUBDIR_EMPTY), (STRPTR)buffer);
		}

		if(DoMethod(d->txt, VTM_InitChange))
		{
			DoMethod(d->txt, VTM_AddSystemMessage, buffer, timestamp, 0);
			DoMethod(d->txt, VTM_ExitChange);
		}

		DoMethod(obj, TTBM_AddToHistory, buffer, timestamp, HISTORY_MESSAGES_SYSTEM);

		if(msg->response->entryid)
			StrFree(msg->response->entryid);
		if(msg->response->name)
			StrFree(msg->response->name);
		if(msg->response->firstname)
			StrFree(msg->response->firstname);
		if(msg->response->lastname)
			StrFree(msg->response->lastname);
		if(msg->response->groupname)
			StrFree(msg->response->groupname);
		if(msg->response->birthyear)
			StrFree(msg->response->birthyear);
		if(msg->response->city)
			StrFree(msg->response->city);
		if(msg->response->statusdesc)
			StrFree(msg->response->statusdesc);
		if(msg->response->avatar)
			FreePicture(msg->response->avatar);
		FreeVec(msg->response);
	}

	return(IPTR)1;
}

static IPTR TalkTabRedrawInfoBlock(Class *cl, Object *obj)
{
	struct TalkTabData *d = INST_DATA(cl, obj);

	MUI_Redraw(d->info_block, MADF_DRAWOBJECT);

	return (IPTR)1;
}

static IPTR TalkTabSendPicture(Class *cl, Object *obj)
{
	struct TalkTabData *d = INST_DATA(cl, obj);
	BOOL result = FALSE;
	struct FileRequester *freq;

	if((freq = MUI_AllocAslRequestTags(ASL_FileRequest, TAG_END)))
	{
		if(MUI_AslRequestTags(freq,
			ASLFR_Window, _window(obj),
			ASLFR_TitleText, GetString(MSG_SEND_PICTURE_ASL_TITLE),
			ASLFR_PositiveText, GetString(MSG_SEND_PICTURE_ASL_POSITIVE),
			ASLFR_InitialPattern, "#?",
			ASLFR_DoPatterns, TRUE,
			ASLFR_RejectIcons, TRUE,
			ASLFR_PrivateIDCMP, TRUE,
		TAG_END))
		{
			APTR pic;
			ULONG pic_size;
			UBYTE location[500];

			StrNCopy(freq->fr_Drawer, (STRPTR)location, 500);
			AddPart((STRPTR)location, freq->fr_File, 500);

			if(IsPictureFile(location) && (pic = LoadFile(location, &pic_size)))
			{
				STRPTR module_name = (STRPTR)DoMethod(_app(obj), APPM_GetModuleName, d->contact->pluginid);

				if(pic_size <= 255 * 1024)
				{
					if(DoMethod(_app(obj), APPM_SendPicture, d->contact->pluginid, d->contact->entryid, location))
					{
						ULONG timestamp;
						struct DateStamp ds;

						DateStamp(&ds);
						timestamp = ds.ds_Days * 24 * 60 * 60 + ds.ds_Minute * 60 + ds.ds_Tick / TICKS_PER_SECOND;

						DoMethod(_app(obj), MUIM_Application_InputBuffered);

						DoMethod(obj, TTBM_PutPicture, NULL, timestamp, pic, pic_size);

						result = TRUE;
					}
					else
						MUI_Request(_app(obj), obj, 0L, APP_NAME, GetString(MSG_SEND_PICTURE_FAIL_BUTTONS), GetString(MSG_SEND_PICTURE_NOTSUPPORTED_MSG), (IPTR)(module_name ? module_name : (STRPTR)"NULL"));
				}
				else
					MUI_Request(_app(obj), obj, 0L, APP_NAME, GetString(MSG_SEND_PICTURE_TOO_BIG_BUTTONS), GetString(MSG_SEND_PICTURE_TOO_BIG_MSG));

				FreeMem(pic, pic_size);
			}
			else
				MUI_Request(_app(obj), obj, 0L, APP_NAME, GetString(MSG_SEND_PICTURE_FAIL_BUTTONS), GetString(MSG_SEND_PICTURE_FAIL_MSG));
		}
		MUI_FreeAslRequest(freq);
	}

	return(IPTR)result;
}

static IPTR TalkTabPutPicture(Class *cl, Object *obj, struct TTBP_PutPicture *msg)
{
	struct TalkTabData *d = INST_DATA(cl, obj);
	ENTER();

	if(DoMethod(d->txt, VTM_InitChange))
	{
		if(!msg->sender)
		{
			STRPTR user_name = (STRPTR)xget(prefs_object(USD_PREFS_TW_USRNAME_STRING), MUIA_String_Contents);

			user_name = StrNew(user_name ? user_name : GetString(MSG_PREFS_TALKWINDOW_USER_USRNAME_DEAFAULT));

			if(user_name)
				DoMethod(d->txt, VTM_AddMessageHeadLine, user_name, msg->timestamp, 0);
		}
		else
			DoMethod(d->txt, VTM_AddMessageHeadLine, msg->sender, msg->timestamp, 0);

		if(xget(prefs_object(USD_PREFS_TW_PICTURES_ONOFF), MUIA_Selected))
			DoMethod(d->txt, VTM_AddPicture, msg->pic, msg->pic_size, 0);
		else
			DoMethod(d->txt, VTM_AddFileView, msg->pic, msg->pic_size, GetString(MSG_FILEVIEW_TEXT_CONTENTS), 0);

		DoMethod(d->txt, VTM_ExitChange);
	}

	LEAVE();
	return (IPTR)1;
}

static IPTR TalkTabOpenLogFile(Class *cl, Object *obj)
{
	struct TalkTabData *d = INST_DATA(cl, obj);

	return DoMethod(findobj(USD_CONTACTS_LIST, _app(obj)), CLSM_OpenLogFile, ContactName(d->contact));
}

static IPTR TalkTabPutInvite(Class *cl, Object *obj, struct TTBP_PutInvite *msg)
{
	struct TalkTabData *d = INST_DATA(cl, obj);
	Object *inv_group, *yes, *no;

	if((inv_group = MUI_NewObject(MUIC_Group,
		MUIA_Group_Horiz, TRUE,
		MUIA_Group_Child, MUI_NewObject(MUIC_Text,
			MUIA_Text_Contents, GetString(MSG_INVITE_TITLE),
		TAG_END),
		MUIA_Group_Child, (yes = MUI_NewObject(MUIC_Text,
			MUIA_Text_Contents, (IPTR)GetString(MSG_INVITE_YES),
			MUIA_Text_PreParse, (IPTR)"\33c",
			MUIA_Frame, MUIV_Frame_Button,
			MUIA_Background, MUII_ButtonBack,
			MUIA_Font, MUIV_Font_Button,
			MUIA_InputMode, MUIV_InputMode_RelVerify,
			MUIA_CycleChain, TRUE,
			MUIA_Weight, 0,
		TAG_END)),
		MUIA_Group_Child, (no = MUI_NewObject(MUIC_Text,
			MUIA_Text_Contents, (IPTR)GetString(MSG_INVITE_NO),
			MUIA_Text_PreParse, (IPTR)"\33c",
			MUIA_Frame, MUIV_Frame_Button,
			MUIA_Background, MUII_ButtonBack,
			MUIA_Font, MUIV_Font_Button,
			MUIA_InputMode, MUIV_InputMode_RelVerify,
			MUIA_CycleChain, TRUE,
			MUIA_Weight, 0,
		TAG_END)),
	TAG_END)))
	{
		struct ContactEntry *entry;
		Object *list = findobj(USD_CONTACTS_LIST, _app(obj));
		LONG i;

		for(i = 0;;i++)
		{
			DoMethod(list, CLSM_GetEntry, i, &entry);
			if(!entry)
				break;
			if(entry->pluginid == d->contact->pluginid && StrEqu(entry->entryid, d->contact->entryid))
				break;
		}

		if(entry)
		{
			DoMethod(no, MUIM_Notify, MUIA_Pressed, FALSE, list, 3,
			 CLSM_RemoveEntry, entry, (ULONG)FALSE);
		}

		DoMethod(yes, MUIM_Notify, MUIA_Pressed, FALSE, MUIV_Notify_Application, 4,
		 APPM_AnswerInvite, msg->pluginid, d->contact->entryid, (ULONG)TRUE);
		DoMethod(yes, MUIM_Notify, MUIA_Pressed, FALSE, d->txt, 1,
		 VTM_Clear);

		DoMethod(no, MUIM_Notify, MUIA_Pressed, FALSE, MUIV_Notify_Application, 4,
		 APPM_AnswerInvite, msg->pluginid, d->contact->entryid, (ULONG)FALSE);
		DoMethod(no, MUIM_Notify, MUIA_Pressed, FALSE, MUIV_Notify_Window, 2,
		 TKWM_DeleteTabByObject, obj);

		if(DoMethod(d->txt, VTM_InitChange))
		{
			DoMethod(d->txt, VTM_AddMessageHeadLine, msg->sender, msg->timestamp, 0);
			DoMethod(d->txt, OM_ADDMEMBER, inv_group);
			DoMethod(d->txt, VTM_ExitChange);
		}
	}

	return (IPTR)0;
}

static IPTR TalkTabInitConversation(Class *cl, Object *obj)
{
	struct TalkTabData *d = INST_DATA(cl, obj);
	ULONG mode = xget(prefs_object(USD_PREFS_HISTORY_CONVERSATION_END_MODE), MUIA_Cycle_Active);
	struct ClockData cd;
	IPTR res;
	ENTER();

	ActLocalTimeToClockData(&cd);

	if(d->conversation_id == -1)
	{
		QUAD c_id;
		ULONG timestamp;

		DoMethod(_app(obj), APPM_GetLastConversation, d->contact->pluginid, d->contact->entryid, &c_id, &timestamp);

		timestamp = UTCToLocal(timestamp, NULL);

		if(mode == 1)
		{
			if(Date2Amiga(&cd) - timestamp <= xget(prefs_object(USD_PREFS_HISTORY_CONVERSATION_END_TIME), MUIA_Slider_Level))
				d->conversation_id = c_id;
		}
		else if(mode == 2)
		{
			struct ClockData con_cd;

			Amiga2Date(timestamp, &con_cd);

			if(cd.year == con_cd.year && cd.month == con_cd.month && cd.mday == con_cd.mday)
				d->conversation_id = c_id;
		}
	}
	else
	{
		if(mode == 1)
		{
			if(Date2Amiga(&cd) - Date2Amiga(&d->last_message_cd) >= xget(prefs_object(USD_PREFS_HISTORY_CONVERSATION_END_TIME), MUIA_Slider_Level))
				d->conversation_id = -1;
		}
		else if(mode == 2)
		{
			if(cd.year != d->last_message_cd.year || cd.month != d->last_message_cd.month || cd.mday != d->last_message_cd.mday)
				d->conversation_id = -1;
		}
	}

	if(d->conversation_id == -1)
	{
		res = DoMethod(_app(obj), APPM_InsertConversationIntoHistory, d->contact->pluginid, d->contact->entryid,
		 ContactName(d->contact), HISTORY_CONVERSATIONS_NORMAL, &d->conversation_id);

		if(res != 0)
			MUI_RequestA(obj, _win(obj), 0, GetString(MSG_SQL_ERROR), GetString(MSG_SQL_GADGETS), GetString(MSG_SQL_ERROR_CONVERSATION), NULL);
	}

	LEAVE();
	return (IPTR)0;
}

static IPTR TalkTabAddToHistory(Class *cl, Object *obj, struct TTBP_AddToHistory *msg)
{
	struct TalkTabData *d = INST_DATA(cl, obj);
	IPTR res = 0;
	ENTER();

	if((BOOL)xget(prefs_object(USD_PREFS_HISTORY_ONOFF_CHECK), MUIA_Selected))
	{
		if(!(BOOL)xget(prefs_object(USD_PREFS_HISTORY_SAVE_SYSTEMMSGS), MUIA_Selected) && (msg->flags & HISTORY_MESSAGES_SYSTEM))
			return res;

		DoMethod(obj, TTBM_InitConversation);

		res = DoMethod(_app(obj), APPM_InsertMessageIntoHistory, &d->conversation_id, NULL, NULL, msg->timestamp, msg->flags, msg->content, -1);

		if(res != 0)
			MUI_RequestA(obj, _win(obj), 0, GetString(MSG_SQL_ERROR), GetString(MSG_SQL_GADGETS), GetString(MSG_SQL_ERROR_MESSAGE), NULL);
	}

	ActLocalTimeToClockData(&d->last_message_cd);

	LEAVE();
	return res;
}

static IPTR TalkTabInsertOldMessage(Class *cl, Object *obj, struct TTBP_InsertOldMessage *msg)
{
	struct TalkTabData *d = INST_DATA(cl, obj);
	IPTR result = -1;
	STRPTR sender = NULL;
	ENTER();

	if(msg->flags & HISTORY_MESSAGES_NORMAL)
	{
		ULONG flags = VTV_FromHistory;

		if(msg->flags & HISTORY_MESSAGES_FRIEND)
		{
			flags |= VTV_Incoming;
			sender = sender ? sender : ContactName(d->contact);
		}
		else
		{
			flags |= VTV_Outgoing;
			sender = (STRPTR)xget(prefs_object(USD_PREFS_TW_USRNAME_STRING), MUIA_String_Contents);
		}

		if(DoMethod(d->txt, VTM_InitChange))
		{
			if(!sender)
				sender = ContactName(d->contact);

			DoMethod(d->txt, VTM_AddMessageHeadLine, sender, msg->timestamp, flags);
			DoMethod(d->txt, VTM_AddMessage, msg->content, flags);
			DoMethod(d->txt, VTM_ExitChange);
		}
		result = 0;
	}
	else if(msg->flags & HISTORY_MESSAGES_SYSTEM)
	{
		if(DoMethod(d->txt, VTM_InitChange))
		{
			DoMethod(d->txt, VTM_AddSystemMessage, msg->content, msg->timestamp, VTV_FromHistory);
			DoMethod(d->txt, VTM_ExitChange);
		}
		result = 0;
	}

	LEAVE();
	return (IPTR)result;
}

static IPTR TalkTabInsertLastMessages(Class *cl, Object *obj)
{
	struct TalkTabData *d = INST_DATA(cl, obj);
	ULONG msg_time = xget(prefs_object(USD_PREFS_HISTORY_LOAD_OLD_TIME), MUIA_Slider_Level);
	ULONG returned = 0;
	ENTER();

	if(msg_time)
		returned = DoMethod(_app(obj), APPM_GetLastMessagesByTime, obj, TTBM_InsertOldMessage, d->contact->pluginid, d->contact->entryid, msg_time);

	if(returned == 0)
	{
		ULONG msg_no = xget(prefs_object(USD_PREFS_HISTORY_LOAD_OLD_NO), MUIA_Slider_Level);

		if(msg_no == xget(prefs_object(USD_PREFS_HISTORY_LOAD_OLD_NO), MUIA_Slider_Max))
			DoMethod(_app(obj), APPM_GetLastConvMsgsFromHistory, obj, TTBM_InsertOldMessage, d->contact->pluginid, d->contact->entryid);
		else if(msg_no != 0)
			DoMethod(_app(obj), APPM_GetLastMessagesFromHistory, obj, TTBM_InsertOldMessage, d->contact->pluginid, d->contact->entryid, msg_no);
	}

	LEAVE();
	return (IPTR)0;
}

static IPTR TalkTabEditContact(Class *cl, Object *obj)
{
	struct TalkTabData *d = INST_DATA(cl, obj);
	struct ContactEntry *to_edit;

	to_edit = (struct ContactEntry*)DoMethod(findobj(USD_CONTACTS_LIST, _app(obj)), CLSM_FindEntry, CLSV_FindEntry_Mode_ID, d->contact, NULL);
	if(to_edit)
		DoMethod(findobj(USD_EDIT_CONTACT_WINDOW_WIN, _app(obj)), ECWM_EditContact, to_edit);

	return 0;
}

static IPTR TalkTabToggleDouble(Class *cl, Object *obj)
{
	struct TalkTabData *d = INST_DATA(cl, obj);

	if(d->double_hidden)
	{
		set(d->sec_input, MUIA_ShowMe, TRUE);

		SetAttrs(d->double_but,
			MUIA_Selected, TRUE,
			MUIA_Text_Contents, "\33I[4:PROGDIR:gfx/toolbar/doublei.mbr]",
		TAG_END);
	}
	else
	{
		set(d->sec_input, MUIA_ShowMe, FALSE);

		SetAttrs(d->double_but,
			MUIA_Selected, FALSE,
			MUIA_Text_Contents, "\33I[4:PROGDIR:gfx/toolbar/double.mbr]",
		TAG_END);
	}

	d->double_hidden = !d->double_hidden;

	return 0;
}

static IPTR TalkTabDispatcher(VOID)
{
	Class *cl = (Class*)REG_A0;
	Object *obj = (Object*)REG_A2;
	Msg msg = (Msg)REG_A1;

	switch(msg->MethodID)
	{
		case OM_NEW: return (TalkTabNew(cl, obj, (struct opSet*)msg));
		case OM_DISPOSE:  return (TalkTabDispose(cl, obj, msg));
		case OM_GET: return (TalkTabGet(cl, obj, (struct opGet*)msg));
		case MUIM_Cleanup: return(TalkTabCleanup(cl, obj, (struct MUIP_Cleanup*)msg));
		case TTBM_Init: return (TalkTabInit(cl, obj, (struct TTBP_Init*)msg));
		case TTBM_PutMessage: return (TalkTabPutMessage(cl, obj, (struct TTBP_PutMessage*)msg));
		case TTBM_SendMessage: return (TalkTabSendMessage(cl, obj, (struct TTBP_SendMessage*)msg));
		case TTBM_Activate: return (TalkTabActivateString(cl, obj));
		case TTBM_ShowEnd: return(TalkTabShowEnd(cl, obj));
		case TTBM_NewStatus: return(TalkTabNewStatus(cl, obj));
		case TTBM_SendWriteNotify: return(TalkTabSendWriteNotify(cl, obj));
		case TTBM_CleanLampState: return(TalkTabCleanLampState(cl, obj));
		case TTBM_SetLampState: return(TalkTabSetLampState(cl, obj));
		case TTBM_PubdirParseResponse: return(TalkTabPubdirParseResponse(cl, obj, (struct TTBP_PubdirParseResponse*)msg));
		case TTBM_RedrawInfoBlock: return(TalkTabRedrawInfoBlock(cl, obj));
		case TTBM_SendPicture: return(TalkTabSendPicture(cl, obj));
		case TTBM_PutPicture: return(TalkTabPutPicture(cl, obj, (struct TTBP_PutPicture*)msg));
		case TTBM_OpenLogFile: return(TalkTabOpenLogFile(cl, obj));
		case TTBM_PutInvite: return(TalkTabPutInvite(cl, obj, (struct TTBP_PutInvite*)msg));
		case TTBM_InitConversation: return(TalkTabInitConversation(cl, obj));
		case TTBM_AddToHistory: return(TalkTabAddToHistory(cl, obj, (struct TTBP_AddToHistory*)msg));
		case TTBM_InsertLastMessages: return(TalkTabInsertLastMessages(cl, obj));
		case TTBM_InsertOldMessage: return(TalkTabInsertOldMessage(cl, obj, (struct TTBP_InsertOldMessage*)msg));
		case TTBM_EditContact: return (TalkTabEditContact(cl, obj));
		case TTBM_ToggleDouble: return (TalkTabToggleDouble(cl, obj));
		default: return (DoSuperMethodA(cl, obj, msg));
	}
}

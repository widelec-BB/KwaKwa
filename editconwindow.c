/*
 * Copyright (c) 2012-2022 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <proto/exec.h>
#include <libvstring.h>
#include <proto/dos.h>

#include "globaldefines.h"
#include "locale.h"
#include "contactslist.h"
#include "application.h"
#include "prefswindow.h"
#include "talkwindow.h"
#include "logs.h"
#include "modulescycle.h"
#include "editconwindow.h"

#include "support.h"

#include "kwakwa_api/defs.h"

struct MUI_CustomClass *EditContactWindowClass;
static IPTR EditContactWindowDispatcher(void);
const struct EmulLibEntry EditContactWindowGate = {TRAP_LIB, 0, (void(*)(void))EditContactWindowDispatcher};

struct ECWP_EditContact {ULONG MethodID; struct ContactEntry *contact;};
struct ECWP_AddModulesCycle {ULONG MethodID; Object *modules_cycle;};
struct ECWP_ParsePubDirResponse {ULONG MethodID; struct ContactEntry *response;};

struct EditContactWindowData
{
	Object *main_group;
	Object *uin_str, *name_str, *nickname_str, *firstname_str, *lastname_str, *birthyear_slider, *city_str, *gender_cycle;
	Object *ok_button, *fetch_pubdir_button;
	Object *modules_cycle;
	struct ContactEntry *act_edit;
};

struct MUI_CustomClass *CreateEditContactWindowClass(void)
{
	struct MUI_CustomClass *cl;

	cl = MUI_CreateCustomClass(NULL, MUIC_Window, NULL, sizeof(struct EditContactWindowData), (APTR)&EditContactWindowGate);
	EditContactWindowClass = cl;
	return cl;
}

void DeleteEditContactWindowClass(void)
{
	if (EditContactWindowClass) MUI_DeleteCustomClass(EditContactWindowClass);
}

static IPTR EditContactWindowNew(Class *cl, Object *obj, struct opSet *msg)
{
	Object *uin_str, *name_str, *nickname_str, *firstname_str, *lastname_str, *birthyear_slider, *city_str, *gender_cycle;
	Object *ok_button, *pub_button, *main_group;
	static CONST_STRPTR genders[4];

	genders[0] = GetString(MSG_GENDER_UNKNOWN);
	genders[1] = GetString(MSG_GENDER_MALE);
	genders[2] = GetString(MSG_GENDER_FEMALE);

	obj = DoSuperNew(cl, obj,
		MUIA_UserData, USD_EDIT_CONTACT_WINDOW_WIN,
		MUIA_Window_ID, USD_EDIT_CONTACT_WINDOW_WIN,
		MUIA_Background, MUII_WindowBack,
		MUIA_Window_ScreenTitle, APP_SCREEN_TITLE,
		MUIA_Window_Title, GetString(MSG_EDITCONTACTWINDOW_TITLE),
		MUIA_Window_RootObject, MUI_NewObject(MUIC_Group,
			MUIA_Group_Child, MUI_NewObject(MUIC_Group,
				MUIA_Group_Child, (main_group = MUI_NewObject(MUIC_Group,
					MUIA_Unicode, TRUE,
					MUIA_Frame, MUIV_Frame_Group,
					MUIA_Background, MUII_GroupBack,
					MUIA_FrameTitle, GetString(MSG_EDITCONTACTWINDOW_CONTACTDATA),
					MUIA_Group_Columns, 2,
					MUIA_Group_Child, StringLabel(GetString(MSG_EDITCONTACTWINDOW_ID), "\33r"),
					MUIA_Group_Child, (uin_str = StringGadget(0)),
					MUIA_Group_Child, StringLabel(GetString(MSG_EDITCONTACTWINDOW_NAME), "\33r"),
					MUIA_Group_Child, (name_str = StringGadget(0)),
					MUIA_Group_Child, StringLabel(GetString(MSG_EDITCONTACTWINDOW_NICKNAME), "\33r"),
					MUIA_Group_Child, (nickname_str = StringGadget(0)),
					MUIA_Group_Child, StringLabel(GetString(MSG_EDITCONTACTWINDOW_FIRSTNAME), "\33r"),
					MUIA_Group_Child, (firstname_str = StringGadget(0)),
					MUIA_Group_Child, StringLabel(GetString(MSG_EDITCONTACTWINDOW_LASTNAME), "\33r"),
					MUIA_Group_Child, (lastname_str = StringGadget(0)),
					MUIA_Group_Child, StringLabel(GetString(MSG_EDITCONTACTWINDOW_BIRTHYEAR), "\33r"),
					MUIA_Group_Child, (birthyear_slider = MUI_NewObject(MUIC_Slider,
						MUIA_Unicode, TRUE,
						MUIA_Slider_Horiz, TRUE,
						MUIA_Slider_Min, 1900,
						MUIA_Slider_Max, 2100,
					TAG_END)),
					MUIA_Group_Child, StringLabel(GetString(MSG_EDITCONTACTWINDOW_GENDER), "\33r"),
					MUIA_Group_Child, (gender_cycle = MUI_NewObject(MUIC_Cycle,
						MUIA_Unicode, TRUE,
						MUIA_Cycle_Entries, genders,
					TAG_END)),
					MUIA_Group_Child, StringLabel(GetString(MSG_EDITCONTACTWINDOW_CITY), "\33r"),
					MUIA_Group_Child, (city_str = StringGadget(0)),
					MUIA_Group_Child, EmptyRectangle(100),
					MUIA_Group_Child, MUI_NewObject(MUIC_Group,
						MUIA_Group_Horiz, TRUE,
						MUIA_Group_Child, EmptyRectangle(100),
						MUIA_Group_Child, (pub_button = NormalButton(GetString(MSG_EDITCONTACTWINDOW_PUBDIR_BUTTON), *GetString(MSG_EDITCONTACTWINDOW_PUBDIR_HOTKEY), 0, 1)),
					TAG_END),
				TAG_END)),
			MUIA_Group_Child, MUI_NewObject(MUIC_Group,
				MUIA_Group_Horiz, TRUE,
				MUIA_Group_Child, EmptyRectangle(20),
				MUIA_Group_Child, ok_button = NormalButton(GetString(MSG_EDITCONTACTWINDOW_BUTTONSAVE), 'a', USD_EDIT_CONTACT_WINDOW_BUTTON, 20),
				MUIA_Group_Child, EmptyRectangle(20),
			TAG_END),
			TAG_END),
		TAG_END),
	TAG_MORE, msg->ops_AttrList);

	if(obj)
	{
		struct EditContactWindowData *d = INST_DATA(cl, obj);

		d->uin_str = uin_str;
		d->name_str = name_str;
		d->nickname_str = nickname_str;
		d->firstname_str = firstname_str;
		d->lastname_str = lastname_str;
		d->ok_button = ok_button;
		d->fetch_pubdir_button = pub_button;
		d->birthyear_slider = birthyear_slider;
		d->city_str = city_str;
		d->gender_cycle = gender_cycle;
		d->main_group = main_group;

		DoMethod(obj, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, MUIV_Notify_Self, 3,
		 MUIM_Set, MUIA_Window_Open, FALSE);

		DoMethod(pub_button, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1,
		 ECWM_PubdirFindByUin);

		DoMethod(ok_button, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1,
		 ECWM_SaveContact);

		return (IPTR)obj;
	}

	CoerceMethod(cl, obj, OM_DISPOSE);
	return (IPTR)NULL;
}


static IPTR EditContactWindowEditContact(Class *cl, Object *obj, struct ECWP_EditContact *msg)
{
	struct EditContactWindowData *d = INST_DATA(cl, obj);
	ENTER();

	d->act_edit = msg->contact;

	if(msg->contact)
	{
		set(d->uin_str, MUIA_String_Contents, msg->contact->entryid);
		set(d->firstname_str, MUIA_String_Contents, msg->contact->firstname);
		set(d->lastname_str, MUIA_String_Contents, msg->contact->lastname);
		set(d->name_str, MUIA_String_Contents, msg->contact->name);
		set(d->nickname_str, MUIA_String_Contents, msg->contact->nickname);
		set(d->city_str, MUIA_String_Contents, msg->contact->city);
		set(d->gender_cycle, MUIA_Cycle_Active, msg->contact->gender);

		if(msg->contact->birthyear)
		{
			ULONG byear;

			if(StrToLong(msg->contact->birthyear, &byear) != -1)
				set(d->birthyear_slider, MUIA_Slider_Level, byear);
		}
		else
			set(d->birthyear_slider, MUIA_Slider_Level, xget(d->birthyear_slider, MUIA_Slider_Max)); /* if unknown set max */

		DoMethod(d->modules_cycle, MCM_SetActiveByID, msg->contact->pluginid);
	}
	else
	{
		set(d->uin_str, MUIA_String_Contents, NULL);
		set(d->firstname_str, MUIA_String_Contents, NULL);
		set(d->lastname_str, MUIA_String_Contents, NULL);
		set(d->name_str, MUIA_String_Contents, NULL);
		set(d->nickname_str, MUIA_String_Contents, NULL);
		set(d->city_str, MUIA_String_Contents, NULL);
	}

	set(obj, MUIA_Window_Open, TRUE);

	LEAVE();
	return (IPTR)1;
}

static IPTR EditContactWindowSaveContact(Class *cl, Object *obj)
{
	struct EditContactWindowData *d = INST_DATA(cl, obj);

	if(d->act_edit)
	{
		STRPTR old_log_name = StrNew(ContactName(d->act_edit));

		if(StrLen((STRPTR)xget(d->uin_str, MUIA_String_Contents)) > 0)
		{
			if(d->act_edit->entryid) StrFree(d->act_edit->entryid);
			d->act_edit->entryid = StrNew((STRPTR)xget(d->uin_str, MUIA_String_Contents));
			if(d->act_edit->firstname) StrFree(d->act_edit->firstname);
			d->act_edit->firstname = StrNew((STRPTR)xget(d->firstname_str, MUIA_String_Contents));
			if(d->act_edit->lastname) StrFree(d->act_edit->lastname);
			d->act_edit->lastname = StrNew((STRPTR)xget(d->lastname_str, MUIA_String_Contents));
			if(d->act_edit->name) StrFree(d->act_edit->name);
			d->act_edit->name = StrNew((STRPTR)xget(d->name_str, MUIA_String_Contents));
			if(d->act_edit->nickname) StrFree(d->act_edit->nickname);
			d->act_edit->nickname = StrNew((STRPTR)xget(d->nickname_str, MUIA_String_Contents));
			if(d->act_edit->city) StrFree(d->act_edit->city);
			d->act_edit->city = StrNew((STRPTR)xget(d->city_str, MUIA_String_Contents));
			if(d->act_edit->birthyear) StrFree(d->act_edit->birthyear);
			d->act_edit->birthyear = FmtNew("%ld", xget(d->birthyear_slider, MUIA_Slider_Level));
			d->act_edit->gender = xget(d->gender_cycle, MUIA_Cycle_Active);
			d->act_edit->pluginid = xget(d->modules_cycle, MCA_ActiveID);

			if(old_log_name) /* if we edit name we have to rename log file */
			{
				LogRename(old_log_name, ContactName(d->act_edit));
				StrFree(old_log_name);
			}
			DoMethod(findobj(USD_CONTACTS_LIST, _app(obj)), CLSM_Sort);
			DoMethod(findobj(USD_TALKWINDOW_WINDOW, _app(obj)), TKWM_UpdateTabContact, d->act_edit->entryid, d->act_edit->pluginid); /* update tab if opened */
		}
	}
	else
	{
		struct ContactEntry con = {0};

		if(StrLen((STRPTR)xget(d->uin_str, MUIA_String_Contents)) > 0)
		{
			con.entryid = StrNew((STRPTR)xget(d->uin_str, MUIA_String_Contents));
			con.firstname = StrNew((STRPTR)xget(d->firstname_str, MUIA_String_Contents));
			con.lastname = StrNew((STRPTR)xget(d->lastname_str, MUIA_String_Contents));
			con.name = StrNew((STRPTR)xget(d->name_str, MUIA_String_Contents));
			con.nickname = StrNew((STRPTR)xget(d->nickname_str, MUIA_String_Contents));
			con.city = StrNew((STRPTR)xget(d->city_str, MUIA_String_Contents));
			con.birthyear = FmtNew("%ld", xget(d->birthyear_slider, MUIA_Slider_Level));
			con.gender = xget(d->gender_cycle, MUIA_Cycle_Active);
			con.statusdesc = NULL;
			con.status = KWA_STATUS_NOT_AVAIL;
			con.unread = FALSE;
			con.pluginid = xget(d->modules_cycle, MCA_ActiveID);

			DoMethod(findobj(USD_CONTACTS_LIST, _app(obj)), CLSM_InsertSingle, &con, CLSV_Insert_Sorted);

			/* here we have to call sth that will bring us information about new contact status... */
			DoMethod(_app(obj), APPM_AddNotify, con.pluginid, con.entryid, con.status);
		}

		if(con.entryid)
			StrFree(con.entryid);
		if(con.firstname)
			StrFree(con.firstname);
		if(con.lastname)
			StrFree(con.lastname);
		if(con.name)
			StrFree(con.name);
		if(con.nickname)
			StrFree(con.nickname);
		if(con.city)
			StrFree(con.city);
		if(con.birthyear)
			StrFree(con.birthyear);
	}


	set(obj, MUIA_Window_Open, FALSE);
	DoMethod(findobj(USD_CONTACTS_LIST, _app(obj)), CLSM_SaveList);

	return (IPTR)1;
}

static IPTR EditContactWindowPubdirFindByUin(Class *cl, Object *obj)
{
	struct EditContactWindowData *d = INST_DATA(cl, obj);

	return DoMethod(_app(obj), APPM_PubDirRequest, xget(d->modules_cycle, MCA_ActiveID), xget(d->uin_str, MUIA_String_Contents), obj, ECWM_PubdirParseResponse);
}

static IPTR EditContactWindowPubdirParseResponse(Class *cl, Object *obj, struct ECWP_ParsePubDirResponse *msg)
{
	struct EditContactWindowData *d = INST_DATA(cl, obj);

	if(msg->response != NULL)
	{
		DoMethod(_app(obj), APPM_ConvertContactEntry, msg->response);


		set(d->uin_str, MUIA_String_Contents, msg->response->entryid);
		set(d->firstname_str, MUIA_String_Contents, msg->response->firstname);
		set(d->lastname_str, MUIA_String_Contents, msg->response->lastname);
		set(d->nickname_str, MUIA_String_Contents, msg->response->nickname);
		set(d->city_str, MUIA_String_Contents, msg->response->city);
		set(d->gender_cycle, MUIA_Cycle_Active, msg->response->gender);

		if(msg->response->name)
			set(d->name_str, MUIA_String_Contents, msg->response->name);

		if(msg->response->birthyear)
		{
			ULONG byear;

			if(StrToLong(msg->response->birthyear, &byear) != -1)
				set(d->birthyear_slider, MUIA_Slider_Level, byear);
		}
		else
			set(d->birthyear_slider, MUIA_Slider_Level, xget(d->birthyear_slider, MUIA_Slider_Max)); /* if unknown set max */

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

	return (IPTR)0;
}

static IPTR EditContactWindowAddModulesCycle(Class *cl, Object *obj, struct ECWP_AddModulesCycle *msg)
{
	struct EditContactWindowData *d = INST_DATA(cl, obj);

	if(msg->modules_cycle)
	{
		d->modules_cycle = msg->modules_cycle;

		if(DoMethod(d->main_group, MUIM_Group_InitChange))
		{
			DoMethod(d->main_group, OM_ADDMEMBER, d->modules_cycle);
			DoMethod(d->main_group, MUIM_Group_ExitChange);
		}
	}

	return (IPTR)0;
}

static IPTR EditContactWindowDispatcher(void)
{
	Class *cl = (Class*)REG_A0;
	Object *obj = (Object*)REG_A2;
	Msg msg = (Msg)REG_A1;

	switch (msg->MethodID)
	{
		case OM_NEW:  return (EditContactWindowNew(cl, obj, (struct opSet*)msg));
		case ECWM_EditContact: return (EditContactWindowEditContact(cl, obj, (struct ECWP_EditContact*)msg));
		case ECWM_SaveContact: return (EditContactWindowSaveContact(cl, obj));
		case ECWM_PubdirFindByUin: return(EditContactWindowPubdirFindByUin(cl, obj));
		case ECWM_PubdirParseResponse: return(EditContactWindowPubdirParseResponse(cl, obj, (struct ECWP_ParsePubDirResponse*)msg));
		case ECWM_AddModulesCycle: return(EditContactWindowAddModulesCycle(cl, obj, (struct ECWP_AddModulesCycle*)msg));
		default:  return (DoSuperMethodA(cl, obj, msg));
	}
}

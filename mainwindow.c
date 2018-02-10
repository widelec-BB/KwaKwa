/*
 * Copyright (c) 2012-2018 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <libraries/gadtools.h>
#include <libvstring.h>

#include <mui/Busy_mcc.h>

#include "globaldefines.h"
#include "locale.h"
#include "application.h"
#include "contactslist.h"
#include "editconwindow.h"
#include "virtualtext.h"
#include "mainwindow.h"
#include "descwindow.h"
#include "prefswindow.h"

#include "support.h"
#include "kwakwa_api/defs.h"

struct MUI_CustomClass *MainWindowClass;
static IPTR MainWindowDispatcher(void);
const struct EmulLibEntry MainWindowGate = {TRAP_LIB, 0, (void(*)(void))MainWindowDispatcher};

struct MWP_ChangeStatus {ULONG MethodID; ULONG new_status;};

struct MainWindowData
{
	Object *contact_list;
	Object *screenbarize_button;
	Object *gg_act_status;
	Object *gg_status_menu;
	Object *busy_bar;

	BOOL hide_installed;
	struct MUI_InputHandlerNode hide_ihn;
};

struct MUI_CustomClass *CreateMainWindowClass(void)
{
	struct MUI_CustomClass *cl;

	cl = MUI_CreateCustomClass(NULL, MUIC_Window, NULL, sizeof(struct MainWindowData), (APTR)&MainWindowGate);
	MainWindowClass = cl;
	return cl;
}

void DeleteMainWindowClass(void)
{
	if (MainWindowClass) MUI_DeleteCustomClass(MainWindowClass);
}

static IPTR MainWindowNotifications(Class *cl, Object *obj)
{
	struct MainWindowData *d = INST_DATA(cl, obj);

	DoMethod(d->gg_status_menu, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MWM_ShowGGStatusMenu);

	DoMethod(d->screenbarize_button, MUIM_Notify, MUIA_Pressed, FALSE, obj, 3, MUIM_Set, MUIA_Window_Open, FALSE);

	DoMethod(obj, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, MUIV_Notify_Self, 1, MWM_CloseRequest);
	DoMethod(obj, MUIM_Notify, MUIA_Window_Open, TRUE, MUIV_Notify_Self, 3, MUIM_Set, MUIA_Window_Activate, TRUE);
	DoMethod(obj, MUIM_Notify, MUIA_Window_Activate, FALSE, MUIV_Notify_Self, 1, MWM_InstallHideTimer);
	DoMethod(obj, MUIM_Notify, MUIA_Window_Activate, TRUE, MUIV_Notify_Self, 1, MWM_RemoveHideTimer);
	DoMethod(obj, MUIM_Notify, MUIA_Window_Open, FALSE, MUIV_Notify_Self, 1, MWM_RemoveHideTimer);

	return (IPTR)1;
}


static IPTR MainWindowNew(Class *cl, Object *obj, struct opSet *msg)
{
	Object *contact_search_string = 	MUI_NewObject(MUIC_Textinput, MUIA_Frame, MUIV_Frame_String, MUIA_Background, MUII_StringBack, MUIA_CycleChain, TRUE, TAG_END);
	Object *prop = MUI_NewObject(MUIC_Prop, MUIA_Prop_UseWinBorder, MUIV_Prop_UseWinBorder_Right, TAG_END);
	Object *contact_list = NewObject(ContactsListClass->mcc_Class, NULL, CLSA_Prop_Gadget, prop, CLSA_Search_String, contact_search_string, MUIA_CycleChain, TRUE, TAG_END);
	Object *screenbarize_button = NormalButton(GetString(MSG_MAINWINDOW_HIDE_BUTTON), *(GetString(MSG_MAINWINDOW_HIDE_BUTTON_HOTKEY)), USD_MAIN_WINDOW_SCREENBARIZE_BUTTON, 0);
	Object *gg_status_menu = NormalButton(GetString(MSG_MAINWINDOW_STATUS_BUTTON), *(GetString(MSG_MAINWINDOW_STATUS_BUTTON_HOTKEY)), USD_MAIN_WINDOW_STATUS_BUTTON, 0);
	Object *gg_act_status = MUI_NewObject(MUIC_Text, MUIA_Weight, 0, MUIA_Text_Contents, "\33I[4:PROGDIR:gfx/unavailable.mbr]", MUIA_ShortHelp, GetString(MSG_MAINWINDOW_ACT_STATUS_HELP), TAG_END);
	Object *next_button = NormalButton(GetString(MSG_MAINWINDOW_SEARCH_NEXT_BUTTON), *GetString(MSG_MAINWINDOW_SEARCH_NEXT_BUTTON_HOTKEY), USD_MAIN_WINDOW_SEARCH_NEXT_BUTTON, 0);
	Object *search_group, *busy_bar;

	obj = DoSuperNew(cl, obj,
		MUIA_UserData, USD_MAIN_WINDOW_WIN,
		MUIA_Background, MUII_WindowBack,
		MUIA_Window_ID, USD_MAIN_WINDOW_WIN,
		MUIA_Window_Title, GetString(MSG_MAINWINDOW_TITLE),
		MUIA_Window_ScreenTitle, APP_SCREEN_TITLE,
		MUIA_Window_UseRightBorderScroller, TRUE,
		MUIA_Window_RootObject, MUI_NewObject(MUIC_Group,

			MUIA_Group_Child, MUI_NewObject(MUIC_Group,
				MUIA_Group_Horiz, TRUE,
				MUIA_Group_Child, contact_list,
				MUIA_Group_Child, prop,
			TAG_END),

			MUIA_Group_Child, (search_group = MUI_NewObject(MUIC_Group,
				MUIA_UserData, USD_MAIN_WINDOW_SEARCH_GROUP,
				MUIA_ShowMe, FALSE,
				MUIA_Group_Horiz, TRUE,
				MUIA_Group_Child, contact_search_string,
				MUIA_Group_Child, next_button,
			TAG_END)),

			MUIA_Group_Child, (busy_bar = MUI_NewObject(MUIC_Busy,
				MUIA_ObjectID, USD_MAIN_WINDOW_BUSY_BAR,
				MUIA_UserData, USD_MAIN_WINDOW_BUSY_BAR,
				MUIA_ShowMe, FALSE,
				MUIA_Busy_Speed, MUIV_Busy_Speed_User,
				MUIA_FixHeightTxt, "MM",
			TAG_END)),

			MUIA_Group_Child, MUI_NewObject(MUIC_Group,
				MUIA_Group_Horiz, TRUE,
				MUIA_Group_Child, MUI_NewObject(MUIC_Group,
					MUIA_Group_Horiz, TRUE,
					MUIA_Group_Child, gg_act_status,
					MUIA_Group_Child, gg_status_menu,
				TAG_END),
				MUIA_Group_Child, EmptyRectangle(20),
				MUIA_Group_Child, screenbarize_button,
			TAG_END),


		TAG_END),
	TAG_MORE, msg->ops_AttrList);

	if(obj)
	{
		struct MainWindowData *d = INST_DATA(cl, obj);

		d->contact_list = contact_list;
		d->screenbarize_button = screenbarize_button;
		d->gg_act_status = gg_act_status;
		d->gg_status_menu = gg_status_menu;

		DoMethod(next_button, MUIM_Notify, MUIA_Pressed, FALSE, contact_list, 2, CLSM_SelectEntryByName, TRUE);
		DoMethod(next_button, MUIM_Notify, MUIA_Pressed, FALSE, obj, 3, MUIM_Set, MUIA_Window_ActiveObject, contact_search_string);
		DoMethod(search_group, MUIM_Notify, MUIA_ShowMe, TRUE, obj, 3, MUIM_Set, MUIA_Window_ActiveObject, contact_search_string);
		DoMethod(prefs_object(USD_PREFS_PROGRAM_MAIN_WINDOW_SHOW_HIDE_BUTTON), MUIM_Notify, MUIA_Selected,
		 MUIV_EveryTime, screenbarize_button, 3, MUIM_Set, MUIA_ShowMe, MUIV_TriggerValue);
		return (IPTR)obj;
	}
	MUI_DisposeObject(contact_list);
	MUI_DisposeObject(screenbarize_button);
	MUI_DisposeObject(gg_status_menu);
	MUI_DisposeObject(gg_act_status);
	MUI_DisposeObject(next_button);
	MUI_DisposeObject(prop);
	MUI_DisposeObject(contact_search_string);
	return (CoerceMethod(cl, obj, OM_DISPOSE));
}

static IPTR MainWindowShowGGStatusMenu(Class *cl, Object *obj)
{
	struct MainWindowData *d = INST_DATA(cl, obj);
	Object *strip = NULL;
	ULONG result;
	UBYTE avail[50] = "\33I[4:PROGDIR:gfx/available.mbr] ", unavail[50] = "\33I[4:PROGDIR:gfx/unavailable.mbr] ",
			invi[50] = "\33I[4:PROGDIR:gfx/invisible.mbr] ", away[50] = "\33I[4:PROGDIR:gfx/away.mbr] ",
			ffc[50] = "\33I[4:PROGDIR:gfx/ffc.mbr] ", dnd[50] = "\33I[4:PROGDIR:gfx/dnd.mbr] ";


	StrCat(GetString(MSG_GG_STATUS_AVAIL), (STRPTR)avail);
	StrCat(GetString(MSG_GG_STATUS_AWAY), (STRPTR)away);
	StrCat(GetString(MSG_GG_STATUS_INVISIBLE), (STRPTR)invi);
	StrCat(GetString(MSG_GG_STATUS_UNAVAIL), (STRPTR)unavail);
	StrCat(GetString(MSG_GG_STATUS_FFC), (STRPTR)ffc);
	StrCat(GetString(MSG_GG_STATUS_DND), (STRPTR)dnd);


	strip = MUI_NewObject(MUIC_Menustrip,
		MUIA_Group_Child, MUI_NewObject(MUIC_Menu,

			MUIA_Group_Child, MUI_NewObject(MUIC_Menuitem,
				MUIA_UserData, 1,
				MUIA_Menuitem_Title, avail,
			TAG_END),

			MUIA_Group_Child, MUI_NewObject(MUIC_Menuitem,
				MUIA_UserData, 2,
				MUIA_Menuitem_Title, away,
			TAG_END),

			MUIA_Group_Child, MUI_NewObject(MUIC_Menuitem,
				MUIA_UserData, 3,
				MUIA_Menuitem_Title, invi,
			TAG_END),

			MUIA_Group_Child, MUI_NewObject(MUIC_Menuitem,
				MUIA_UserData, 4,
				MUIA_Menuitem_Title, unavail,
			TAG_END),

			MUIA_Group_Child, MUI_NewObject(MUIC_Menuitem,
				MUIA_Menuitem_Title, NM_BARLABEL,
			TAG_END),

			MUIA_Group_Child, MUI_NewObject(MUIC_Menuitem,
				MUIA_UserData, 5,
				MUIA_Menuitem_Title, ffc,
			TAG_END),

			MUIA_Group_Child, MUI_NewObject(MUIC_Menuitem,
				MUIA_UserData, 6,
				MUIA_Menuitem_Title, dnd,
			TAG_END),

			MUIA_Group_Child, MUI_NewObject(MUIC_Menuitem,
				MUIA_Menuitem_Title, NM_BARLABEL,
			TAG_END),

			MUIA_Group_Child, MUI_NewObject(MUIC_Menuitem,
				MUIA_UserData, 7,
				MUIA_Menuitem_Title, GetString(MSG_WITH_DESCRIPTION),
			TAG_END),

		TAG_END),
	TAG_END);

	if(strip)
	{
		result = DoMethod(strip, MUIM_Menustrip_Popup, d->gg_status_menu, 0, _left(d->gg_status_menu), _bottom(d->gg_status_menu)+1);

		switch(result)
		{
			case 1:
				DoMethod(_app(obj), APPM_ChangeStatus, KWA_STATUS_AVAIL, NULL);
			break;

			case 2:
				DoMethod(_app(obj), APPM_ChangeStatus, KWA_STATUS_BUSY, NULL);
			break;

			case 3:
				DoMethod(_app(obj), APPM_ChangeStatus, KWA_STATUS_INVISIBLE, NULL);
			break;

			case 4:
				DoMethod(_app(obj), APPM_ChangeStatus, KWA_STATUS_NOT_AVAIL, NULL);
			break;

			case 5:
				DoMethod(_app(obj), APPM_ChangeStatus, KWA_STATUS_FFC, NULL);
			break;

			case 6:
				DoMethod(_app(obj), APPM_ChangeStatus, KWA_STATUS_DND, NULL);
			break;

			case 7:
				set(findobj(USD_DESC_WINDOW, _app(obj)), MUIA_Window_Open, TRUE);
			break;
		}
		MUI_DisposeObject(strip);
	}

	return (IPTR)1;
}

static IPTR MainWindowChangeStatus(Class *cl, Object *obj, struct MWP_ChangeStatus *msg)
{
	struct MainWindowData *d = INST_DATA(cl, obj);

	switch(KWA_S(msg->new_status))
	{
		case KWA_STATUS_NOT_AVAIL:
			set(d->gg_act_status, MUIA_Text_Contents, "\33I[4:PROGDIR:gfx/unavailable.mbr]");
		break;

		case KWA_STATUS_AVAIL:
			set(d->gg_act_status, MUIA_Text_Contents, "\33I[4:PROGDIR:gfx/available.mbr]");
		break;

		case KWA_STATUS_BUSY:
			set(d->gg_act_status, MUIA_Text_Contents, "\33I[4:PROGDIR:gfx/away.mbr]");
		break;

		case KWA_STATUS_INVISIBLE:
			set(d->gg_act_status, MUIA_Text_Contents, "\33I[4:PROGDIR:gfx/invisible.mbr]");
		break;

		case KWA_STATUS_FFC:
			set(d->gg_act_status, MUIA_Text_Contents, "\33I[4:PROGDIR:gfx/ffc.mbr]");
		break;

		case KWA_STATUS_DND:
			set(d->gg_act_status, MUIA_Text_Contents, "\33I[4:PROGDIR:gfx/dnd.mbr]");
		break;
	}

	return (IPTR)1;
}

static IPTR MainWindowCloseRequest(Class *cl, Object *obj)
{
	switch(xget(prefs_object(USD_PREFS_PROGRAM_MAIN_WINDOW_CLOSE_CYCLE), MUIA_Cycle_Active))
	{
		case 0:
			DoMethod(_app(obj), MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
		break;

		case 1:
			set(obj, MUIA_Window_Open, FALSE);
		break;

		case 2:
			set(_app(obj), MUIA_Application_Iconified, TRUE);
		break;
	}

	return (IPTR)0;
}

static IPTR MainWindowInstallHideTimer(Class *cl, Object *obj)
{
	struct MainWindowData *d = INST_DATA(cl, obj);
	UWORD timeout = (UWORD)xget(prefs_object(USD_PREFS_PROGRAM_MAIN_WINDOW_HIDE_TIME_SLIDER), MUIA_Slider_Level);

	if(timeout > 0)
	{
		d->hide_ihn.ihn_Object  = obj;
		d->hide_ihn.ihn_Millis  = timeout;
		d->hide_ihn.ihn_Method  = MWM_HideMethod;
		d->hide_ihn.ihn_Flags   = MUIIHNF_TIMER | MUIIHNF_TIMER_SCALE10 | MUIIHNF_TIMER_SCALE100;

		DoMethod(_app(obj), MUIM_Application_AddInputHandler, &d->hide_ihn);
		d->hide_installed = TRUE;
	}

	return (IPTR)0;
}

static IPTR MainWindowHideMethod(Class *cl, Object *obj)
{
	set(obj, MUIA_Window_Open, FALSE);

	return (IPTR)0;
}

static IPTR MainWindowRemoveHideTimer(Class *cl, Object *obj)
{
	struct MainWindowData *d = INST_DATA(cl, obj);

	if(d->hide_installed)
		DoMethod(_app(obj), MUIM_Application_RemInputHandler, &d->hide_ihn);

	d->hide_installed = FALSE;

	return(IPTR)0;
}

static IPTR MainWindowDispatcher(void)
{
	Class *cl = (Class*)REG_A0;
	Object *obj = (Object*)REG_A2;
	Msg msg = (Msg)REG_A1;

	switch (msg->MethodID)
	{
		case OM_NEW:  return (MainWindowNew(cl, obj, (struct opSet*)msg));
		case MWM_Notifications: return(MainWindowNotifications(cl, obj));
		case MWM_ShowGGStatusMenu: return (MainWindowShowGGStatusMenu(cl, obj));
		case MWM_ChangeStatus: return(MainWindowChangeStatus(cl, obj, (struct MWP_ChangeStatus*)msg));
		case MWM_CloseRequest: return(MainWindowCloseRequest(cl, obj));
		case MWM_InstallHideTimer: return(MainWindowInstallHideTimer(cl, obj));
		case MWM_HideMethod: return(MainWindowHideMethod(cl, obj));
		case MWM_RemoveHideTimer: return(MainWindowRemoveHideTimer(cl, obj));
		default:  return (DoSuperMethodA(cl, obj, msg));
	}
}


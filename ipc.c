/*
 * Copyright (c) 2012-2022 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/types.h>
#include <emul/emulregs.h>
#include <libvstring.h>

#include "kwakwa_api/defs.h"

#include "globaldefines.h"
#include "application.h"
#include "prefswindow.h"
#include "moduleslogwindow.h"
#include "mainwindow.h"
#include "talkwindow.h"
#include "talktab.h"
#include "editconwindow.h"
#include "contactslist.h"
#include "descwindow.h"
#include "support.h"
#include "inputfield.h"
#include "ipc.h"


struct changeStatusArgs
{
	STRPTR statusType;
	STRPTR statusDesc;
};

struct disconnectDescriptionArgs
{
	STRPTR description;
};

struct windowControlArgs
{
	STRPTR windowName;
	STRPTR action;
};

struct activateTabByNumberArgs
{
	STRPTR number;
};

struct openTabByNameArgs
{
	STRPTR name;
};

struct insertMessageArgs
{
	STRPTR input;
	STRPTR message;
};

static IPTR changeStatus(VOID);
static IPTR disconnectDescription(VOID);
static IPTR disconnect(VOID);
static IPTR connect(VOID);
static IPTR windowControl(VOID);
static IPTR activateTabByNumber(VOID);
static IPTR openTabByName(VOID);
static IPTR insertMessage(VOID);


struct EmulLibEntry changeStatusGate = {TRAP_LIB, 0, (VOID(*)(VOID))changeStatus};
struct Hook changeStatusHook = {{0, 0}, (HOOKFUNC)&changeStatusGate, 0, 0};
struct EmulLibEntry disconnectDescriptionGate = {TRAP_LIB, 0, (VOID(*)(VOID))disconnectDescription};
struct Hook disconnectDescriptionHook = {{0, 0}, (HOOKFUNC)&disconnectDescriptionGate, 0, 0};
struct EmulLibEntry disconnectGate = {TRAP_LIB, 0, (VOID(*)(VOID))disconnect};
struct Hook disconnectHook = {{0, 0}, (HOOKFUNC)&disconnectGate, 0, 0};
struct EmulLibEntry connectGate = {TRAP_LIB, 0, (VOID(*)(VOID))connect};
struct Hook connectHook = {{0, 0}, (HOOKFUNC)&connectGate, 0, 0};
struct EmulLibEntry windowControlGate = {TRAP_LIB, 0, (VOID(*)(VOID))&windowControl};
struct Hook windowControlHook = {{0, 0}, (HOOKFUNC)&windowControlGate, 0, 0};
struct EmulLibEntry activateTabByNumberGate = {TRAP_LIB, 0, (VOID(*)(VOID))&activateTabByNumber};
struct Hook activateTabByNumberHook = {{0, 0}, (HOOKFUNC)&activateTabByNumberGate, 0, 0};
struct EmulLibEntry openTabByNameGate = {TRAP_LIB, 0, (VOID(*)(VOID))&openTabByName};
struct Hook openTabByNameHook = {{0, 0}, (HOOKFUNC)&openTabByNameGate};
struct EmulLibEntry insertMessageGate = {TRAP_LIB, 0, (VOID(*)(VOID))&insertMessage};
struct Hook insertMessageHook = {{0, 0}, (HOOKFUNC)&insertMessageGate};


struct MUI_Command IpcCommands[] =
{
	{"ChangeStatus", "STATUSTYPE/A,STATUSDESC/F", 2, &changeStatusHook, {0L}},
	{"DisconnectDescription", "DESCRIPTION/F", 1, &disconnectDescriptionHook, {0L}},
	{"Disconnect", NULL, 0, &disconnectHook, {0L}},
	{"Connect", NULL, 0, &connectHook, {0L}},
	{"WindowControl", "WINDOW/A,ACTION/A", 2, &windowControlHook, {0L}},
	{"ActivateTabByNumber", "TABNUMBER/A", 1, &activateTabByNumberHook, {0L}},
	{"OpenTabByName", "NAME/F", 1, &openTabByNameHook, {0L}},
	{"InsertMessage", "INPUT/A,MESSAGE/F", 2, &insertMessageHook, {0L}},
	{NULL, NULL, 0, NULL, {0L}},
};


static IPTR changeStatus(VOID)
{
	struct changeStatusArgs *args = (struct changeStatusArgs*)REG_A1;
	Object *app = (Object*)REG_A2;
	ULONG status_type;

	if(StrEqu(args->statusType, "available"))
		status_type = KWA_STATUS_AVAIL;
	else if(StrEqu(args->statusType, "away"))
		status_type = KWA_STATUS_BUSY;
	else if(StrEqu(args->statusType, "invisible"))
		status_type = KWA_STATUS_INVISIBLE;
	else if(StrEqu(args->statusType, "unavailable"))
		status_type = KWA_STATUS_NOT_AVAIL;
	else if(StrEqu(args->statusType, "ffc"))
		status_type = KWA_STATUS_FFC;
	else if(StrEqu(args->statusType, "dnd"))
		status_type = KWA_STATUS_DND;
	else
	{
		set(app, MUIA_Application_RexxString, "Wrong status name");
		return 0;
	}

	if((BOOL)DoMethod(app, APPM_ChangeStatus, status_type, args->statusDesc))
		set(app, MUIA_Application_RexxString, "OK");
	else
		set(app, MUIA_Application_RexxString, "Failed");

	return 0;
}

static IPTR disconnectDescription(VOID)
{
	struct disconnectDescriptionArgs *args = (struct disconnectDescriptionArgs*)REG_A1;
	Object *app = (Object*)REG_A2;

	if((BOOL)DoMethod(app, APPM_ChangeStatus, KWA_STATUS_NOT_AVAIL, args->description))
		set(app, MUIA_Application_RexxString, "OK");
	else
		set(app, MUIA_Application_RexxString, "Failed");

	return 0;
}

static IPTR disconnect(VOID)
{
	Object *app = (Object*)REG_A2;

	if((BOOL)DoMethod(app, APPM_ChangeStatus, KWA_STATUS_NOT_AVAIL, xget(app, APPA_Description)))
		set(app, MUIA_Application_RexxString, "OK");
	else
		set(app, MUIA_Application_RexxString, "Failed");

	return 0;
}

static IPTR connect(VOID)
{
	Object *app = (Object*)REG_A2;
	BOOL result = FALSE;
	ULONG last_status = xget(app, APPA_Status);
	STRPTR last_desc = (STRPTR)xget(app, APPA_Description);

	if(last_status == KWA_STATUS_NOT_AVAIL)
	{
		switch(xget(prefs_object(USD_PREFS_PROGRAM_LASTNOTAVAIL_CYCLE), MUIA_Cycle_Active))
		{
			case 1:
				result = DoMethod(app, APPM_Connect, KWA_STATUS_AVAIL, last_desc);
			break;

			case 2:
				result = DoMethod(app, APPM_Connect, KWA_STATUS_BUSY, last_desc);
			break;

			case 3:
				result = DoMethod(app, APPM_Connect, KWA_STATUS_INVISIBLE, last_desc);
			break;

			case 4:
				result = DoMethod(app, APPM_Connect, KWA_STATUS_FFC, last_desc);
			break;

			case 5:
				result = DoMethod(app, APPM_Connect, KWA_STATUS_DND, last_desc);
			break;
		}
	}
	else
		result = DoMethod(app, APPM_Connect, last_status, last_desc);

	if(result)
		set(app, MUIA_Application_RexxString, "OK");
	else
		set(app, MUIA_Application_RexxString, "Failed");

	return 0;
}

static IPTR windowControl(VOID)
{
	struct windowControlArgs *args = (struct windowControlArgs*)REG_A1;
	Object *app = (Object*)REG_A2;
	BOOL result = FALSE;
	ENTER();

	if(args->windowName && args->action)
	{
		Object *win = NULL;

		if(StrEqu(args->windowName, "main"))
			win = findobj(USD_MAIN_WINDOW_WIN, app);
		else if(StrEqu(args->windowName, "preferences"))
			win = findobj(USD_PREFS_WINDOW_WIN, app);
		else if(StrEqu(args->windowName, "talk"))
			win = findobj(USD_TALKWINDOW_WINDOW, app);
		else if(StrEqu(args->windowName, "edit contact"))
			win = findobj(USD_EDIT_CONTACT_WINDOW_WIN, app);
		else if(StrEqu(args->windowName, "set status"))
			win = findobj(USD_DESC_WINDOW, app);
		else if(StrEqu(args->windowName, "modules log"))
			win = findobj(USD_MODULESLOG_WINDOW, app);

		if(win)
		{
			if(StrEqu(args->action, "show"))
			{
				set(win, MUIA_Window_Open, TRUE);
				result = TRUE;
			}
			else if(StrEqu(args->action, "hide"))
			{
				set(win, MUIA_Window_Open, FALSE);
				result = TRUE;
			}
			else if(StrEqu(args->action, "to front"))
			{
				DoMethod(win, MUIM_Window_ToFront);
				result = TRUE;
			}
			else if(StrEqu(args->action, "to back"))
			{
				DoMethod(win, MUIM_Window_ToBack);
				result = TRUE;
			}
			else if(StrEqu(args->action, "activate"))
			{
				set(win, MUIA_Window_Activate, TRUE);
				result = TRUE;
			}
			else if(StrEqu(args->action, "deactivate"))
			{
				set(win, MUIA_Window_Activate, FALSE);
				result = TRUE;
			}
			else if(StrEqu(args->action, "screen to front"))
			{
				DoMethod(win, MUIM_Window_ScreenToFront);
				result = TRUE;
			}
			else if(StrEqu(args->action, "screen to back"))
			{
				DoMethod(win, MUIM_Window_ScreenToBack);
				result = TRUE;
			}
		}
	}

	if(result)
		set(app, MUIA_Application_RexxString, "OK");
	else
		set(app, MUIA_Application_RexxString, "Failed");

	LEAVE();
	return 0;
}

static IPTR activateTabByNumber(VOID)
{
	struct activateTabByNumberArgs *args = (struct activateTabByNumberArgs*)REG_A1;
	Object *app = (Object*)REG_A2;
	BOOL result = FALSE;
	LONG number;

	if(StrToLong(args->number, &number) != -1)
	{
		Object *talk_win = findobj(USD_TALKWINDOW_WINDOW, app);

		result = DoMethod(talk_win, TKWM_OpenOnTabById, number);
	}

	if(result)
		set(app, MUIA_Application_RexxString, "OK");
	else
		set(app, MUIA_Application_RexxString, "Failed");

	return 0;
}

static IPTR openTabByName(VOID)
{
	struct openTabByNameArgs *args = (struct openTabByNameArgs*)REG_A1;
	Object *app = (Object*)REG_A2;
	BOOL result = FALSE;
	struct ContactEntry *entry = NULL;
	Object *contacts_list = findobj(USD_CONTACTS_LIST, app);
	LONG i;

	for(i = 0;;i++)
	{
		DoMethod(contacts_list, CLSM_GetEntry, i, &entry);

		if(!entry || StrEqu(ContactName(entry), args->name))
			break;
	}

	if(entry)
	{
		Object *talk_win = findobj(USD_TALKWINDOW_WINDOW, app);

		result = DoMethod(talk_win, TKWM_ShowMessage, entry->entryid, entry->pluginid, NULL, 0, 0);
	}

	if(result)
		set(app, MUIA_Application_RexxString, "OK");
	else
		set(app, MUIA_Application_RexxString, "Failed");

	return 0;
}

static IPTR insertMessage(VOID)
{
	struct insertMessageArgs *args = (struct insertMessageArgs*)REG_A1;
	Object *app = (Object*)REG_A2;
	BOOL result = FALSE;
	Object *tab;

	DoMethod(findobj(USD_TALKWINDOW_WINDOW, app), TKWM_GetTab, TKWV_ActiveTab, &tab);

	if(tab)
	{
		Object *input = NULL;

		if(StrEqu(args->input, "first"))
			input = findobj(USD_TALKTAB_INPUT_FIRST, app);
		else if(StrEqu(args->input, "second"))
			input = findobj(USD_TALKTAB_INPUT_SECOND, app);

		if(input)
			DoMethod(input, IFM_AppendText, args->message);
	}


	if(result)
		set(app, MUIA_Application_RexxString, "OK");
	else
		set(app, MUIA_Application_RexxString, "Failed");

	return 0;
}

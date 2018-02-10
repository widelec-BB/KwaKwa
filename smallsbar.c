/*
 * Copyright (c) 2012-2018 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <mui/Rawimage_mcc.h>
#include <mui/Crawling_mcc.h>
#include <intuition/screenbar.h>
#include <libraries/gadtools.h>
#include <proto/exec.h>
#include <proto/cybergraphics.h>
#include <cybergraphx/cybergraphics.h>

#include "globaldefines.h"
#include "application.h"
#include "prefswindow.h"
#include "mainwindow.h"
#include "talkwindow.h"
#include "support.h"
#include "locale.h"
#include "sbaricon.h"
#include "smallsbar.h"

#include "kwakwa_api/defs.h"
#include "kwakwa_api/pictures.h"

struct MUI_CustomClass *SmallSBarClass;

static IPTR SmallSBarDispatcher(void);
const struct EmulLibEntry SmallSBarGate = {TRAP_LIB, 0, (void(*)(void))SmallSBarDispatcher};

struct SmallSBarData
{
	struct SBarControl *sctl;
	struct MUI_EventHandlerNode handler;
	struct Picture *act_pic;
	Object *app;
};

Object *Icon_obj; /* global for all sbar objects */

struct MUI_CustomClass *CreateSmallSBarClass(void)
{
	struct MUI_CustomClass *cl = NULL;

	if((Icon_obj = MUI_NewObject(MUIC_Rawimage,
		MUIA_Rawimage_Data, KwaKwaSBarIcon,
	TAG_END)))
	{
		cl = MUI_CreateCustomClass(NULL, MUIC_Area, NULL, sizeof(struct SmallSBarData), (APTR)&SmallSBarGate);
		SmallSBarClass = cl;
	}
	return cl;
}

void DeleteSmallSBarClass(void)
{
	if(SmallSBarClass) MUI_DeleteCustomClass(SmallSBarClass);
	MUI_DisposeObject(Icon_obj);
}

static IPTR SmallSBarNew(Class *cl, Object *obj, struct opSet *msg)
{
	ENTER();
	obj = DoSuperNew(cl, obj,
		MUIA_InputMode, MUIV_InputMode_RelVerify,
		MUIA_DoubleBuffer, TRUE,
	TAG_MORE, msg->ops_AttrList);
	if(obj)
	{
		struct SmallSBarData *d = INST_DATA(cl, obj);

		if((d->sctl = (struct SBarControl*)cl->cl_UserData))
		{
			ObtainSemaphore(&d->sctl->semaphore);

			d->act_pic = d->sctl->actPic;
			d->app = d->sctl->app;

			DoMethod(obj, MUIM_Notify, MUIA_Pressed, FALSE, d->app, 4,
			 MUIM_Application_PushMethod, d->app, 1, APPM_Screenbarize);

			ReleaseSemaphore(&d->sctl->semaphore);

			DoMethod(obj, MUIM_Notify, SBRA_RightClick, MUIV_EveryTime, MUIV_Notify_Self, 1,
			 SBRM_ShowMenu);

			LEAVE();
			return (IPTR)obj;
		}
	}
	LEAVE();
	return (CoerceMethod(cl, obj, OM_DISPOSE));
}

static IPTR SmallSBarSetup(Class *cl, Object *obj, struct MUIP_Setup *msg)
{
	struct SmallSBarData *d = INST_DATA(cl, obj);
	IPTR result = (IPTR) DoSuperMethodA(cl, obj, msg);
	ENTER();

	if(d->sctl->sbarTask == NULL)
	{
		ObtainSemaphore(&d->sctl->semaphore);
		Forbid();
		d->sctl->sbarTask = FindTask(NULL);
		GetAttr(SA_ScreenbarSignal, _screen(obj), &d->sctl->sbarSignal);
		d->sctl->sbarSignal = 1L << d->sctl->sbarSignal;
		Permit();
		ReleaseSemaphore(&d->sctl->semaphore);
	}

	d->handler.ehn_Class = cl;
	d->handler.ehn_Object = obj;
	d->handler.ehn_Events = IDCMP_MOUSEBUTTONS;

	DoMethod(_win(obj), MUIM_Window_AddEventHandler, &d->handler);

	LEAVE();
	return result;
}


static IPTR SmallSBarCleanup(Class *cl, Object *obj, struct MUIP_Cleanup *msg)
{
	struct SmallSBarData *d = INST_DATA(cl, obj);
	ENTER();

	DoMethod(_win(obj), MUIM_Window_RemEventHandler, &d->handler);

	LEAVE();
	return DoSuperMethodA(cl, obj, msg);
}

static IPTR SmallSBarGet(Class *cl, Object *obj, struct opGet *msg)
{
	int rv = FALSE;

	switch (msg->opg_AttrID)
	{
		case MUIA_Screenbar_DisplayedImage:
			*msg->opg_Storage = (ULONG)Icon_obj;
		return TRUE;

		case SBRA_RightClick:
		return TRUE;

		default: rv = (DoSuperMethodA(cl, obj, (Msg)msg));
	}

	return rv;
}

static IPTR SmallSBarBuildSettingsPanel(Class *cl, Object *obj, struct MUIP_Screenbar_BuildSettingsPanel *msg)
{
	STRPTR credits = "\33c\33b" APP_NAME " " APP_VER_NO " " APP_DATE "\n\33n© " APP_CYEARS " BlaBla group";
	Object *prefs = MUI_NewObject(MUIC_Mccprefs,
		MUIA_Group_Child, MUI_NewObject(MUIC_Rectangle,
		TAG_DONE),
		MUIA_Group_Child, MUI_NewObject(MUIC_Crawling,
			MUIA_Frame,        MUIV_Frame_Text,
			MUIA_Background,   MUII_TextBack,
			MUIA_FixHeightTxt, credits,
			MUIA_Group_Child,  MUI_NewObject(MUIC_Text,
				MUIA_Text_Contents, credits,
			TAG_DONE),
		TAG_DONE),
	TAG_DONE);
	return (IPTR)prefs;
}

static IPTR SmallSBarAskMinMax(Class *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
	ULONG rc = DoSuperMethodA(cl, obj, msg);
	struct MUI_MinMax *mm = msg->MinMaxInfo;

	mm->MinWidth = mm->DefWidth = mm->MaxWidth = _screen(obj)->BarHeight + 2;
	mm->MinHeight = mm->DefHeight = mm->MaxHeight = _screen(obj)->BarHeight + 1;

	return rc;
}

static IPTR SmallSBarSignal(Class *cl, Object *obj, struct MUIP_Screenbar_Signal *msg)
{
	struct SmallSBarData *d = INST_DATA(cl, obj);

	ObtainSemaphore(&d->sctl->semaphore);

	if(d->sctl->unread)
	{
		d->act_pic = d->sctl->unreadPic;
		MUI_Redraw(obj, MADF_DRAWOBJECT);
	}
	else if(d->act_pic != d->sctl->actPic)
	{
		d->act_pic = d->sctl->actPic;
		MUI_Redraw(obj, MADF_DRAWOBJECT);
	}

	ReleaseSemaphore(&d->sctl->semaphore);

	return (IPTR)1;
}

static IPTR SmallSBarDraw(Class *cl, Object *obj, struct MUIP_Draw *msg)
{
	struct SmallSBarData *d = INST_DATA(cl, obj);
	IPTR result = (IPTR) DoSuperMethodA(cl, obj, msg);

	if (msg->flags & MADF_DRAWOBJECT)
	{
		ObtainSemaphore(&d->sctl->semaphore);

		if(d->act_pic->p_Height >= _mheight(obj))
		{
			DOUBLE hratio = ((DOUBLE)_mheight(obj) - 2) / (DOUBLE)d->act_pic->p_Height;
			DOUBLE wratio = ((DOUBLE)_mwidth(obj)  - 2) / (DOUBLE)d->act_pic->p_Width;
			DOUBLE ratio = hratio < wratio ? hratio : wratio;

			ScalePixelArrayAlpha(d->act_pic->p_Data, d->act_pic->p_Width, d->act_pic->p_Height, d->act_pic->p_Width << 2, _rp(obj),
			 _mleft(obj) + 1 + (((UWORD)(_mwidth(obj) - (d->act_pic->p_Width * ratio)) >> 1)),
			 _mtop(obj)  + (((UWORD)(_mheight(obj) - (d->act_pic->p_Height * ratio)) >> 1)),
			 d->act_pic->p_Width * ratio, (d->act_pic->p_Height * ratio)-1, 0xFFFFFFFF);
		}
		else
		{
			WritePixelArrayAlpha(d->act_pic->p_Data, 0, 0, d->act_pic->p_Width << 2, _rp(obj), _mleft(obj) + ((_mwidth(obj) - d->act_pic->p_Width) >> 1),
			 _mtop(obj) + ((_mheight(obj) - d->act_pic->p_Height) >> 1), d->act_pic->p_Width, d->act_pic->p_Height, 0xFFFFFFFF);
		}

		ReleaseSemaphore(&d->sctl->semaphore);
	}

	return (IPTR)result;
}

static IPTR SmallSBarHandleEvent(Class *cl, Object *obj, struct MUIP_HandleEvent *msg)
{
	if(msg->imsg)
	{
		if(msg->imsg->Class == IDCMP_MOUSEBUTTONS)
		{
			if(_isinobject(msg->imsg->MouseX, msg->imsg->MouseY))
			{
				if(msg->imsg->Code == IECODE_RBUTTON)
				{
					set(obj, SBRA_RightClick, TRUE);
					return MUI_EventHandlerRC_Eat;
				}
			}
		}
	}

	return 0;
}

static IPTR SmallSBarShowMenu(Class *cl, Object *obj)
{
	struct SmallSBarData *d = INST_DATA(cl, obj);
	Object *strip = NULL;
	Object *tabs_menu = NULL;
	Object *menu = NULL;
	Object *status_menu = NULL;

	strip = MUI_NewObject(MUIC_Menustrip,
		MUIA_Group_Child, (menu = MUI_NewObject(MUIC_Menu,

			MUIA_Group_Child, MUI_NewObject(MUIC_Menuitem,
				MUIA_UserData, SBR_MENU_SHOW_LIST,
				MUIA_Menuitem_Title, ((BOOL)xget(findobj(USD_MAIN_WINDOW_WIN, d->app), MUIA_Window_Open)) ?
											GetString(MSG_SBAR_HIDE_CONTACT_LIST) : GetString(MSG_SBAR_SHOW_CONTACT_LIST),
			TAG_END),

			MUIA_Group_Child, MUI_NewObject(MUIC_Menuitem,
				MUIA_Menuitem_Title, NM_BARLABEL,
			TAG_END),

		TAG_END)),
	TAG_END);

	DoMethod(menu, MUIM_Group_InitChange);

	tabs_menu = (Object*)DoMethod(findobj(USD_TALKWINDOW_WINDOW, d->app), TKWM_CreateTabsMenuStrip);
	DoMethod(menu, OM_ADDMEMBER, tabs_menu);

	status_menu = MUI_NewObject(MUIC_Menuitem,
		MUIA_Menuitem_Title, GetString(MSG_SBAR_CHANGE_STATUS),
			MUIA_Group_Child, MUI_NewObject(MUIC_Menuitem,
				MUIA_UserData, SBR_MENU_STATUS_AVAILABLE,
				MUIA_Menuitem_Title, GetString(MSG_GG_STATUS_AVAIL),
			TAG_END),

			MUIA_Group_Child, MUI_NewObject(MUIC_Menuitem,
				MUIA_UserData, SBR_MENU_STATUS_AWAY,
				MUIA_Menuitem_Title, GetString(MSG_GG_STATUS_AWAY),
			TAG_END),

			MUIA_Group_Child, MUI_NewObject(MUIC_Menuitem,
				MUIA_UserData, SBR_MENU_STATUS_INVISIBLE,
				MUIA_Menuitem_Title, GetString(MSG_GG_STATUS_INVISIBLE),
			TAG_END),

			MUIA_Group_Child, MUI_NewObject(MUIC_Menuitem,
				MUIA_UserData, SBR_MENU_STATUS_UNAVAILABLE,
				MUIA_Menuitem_Title, GetString(MSG_GG_STATUS_UNAVAIL),
			TAG_END),

			MUIA_Group_Child, MUI_NewObject(MUIC_Menuitem,
				MUIA_Menuitem_Title, NM_BARLABEL,
			TAG_END),

			MUIA_Group_Child, MUI_NewObject(MUIC_Menuitem,
				MUIA_UserData, SBR_MENU_STATUS_FFC,
				MUIA_Menuitem_Title, GetString(MSG_GG_STATUS_FFC),
			TAG_END),

			MUIA_Group_Child, MUI_NewObject(MUIC_Menuitem,
				MUIA_UserData, SBR_MENU_STATUS_DND,
				MUIA_Menuitem_Title, GetString(MSG_GG_STATUS_DND),
			TAG_END),

			MUIA_Group_Child, MUI_NewObject(MUIC_Menuitem,
				MUIA_Menuitem_Title, NM_BARLABEL,
			TAG_END),

			MUIA_Group_Child, MUI_NewObject(MUIC_Menuitem,
				MUIA_UserData, SBR_MENU_STATUS_DESCRIPTION,
				MUIA_Menuitem_Title, GetString(MSG_WITH_DESCRIPTION),
			TAG_END),
		TAG_END);

	DoMethod(menu, OM_ADDMEMBER, status_menu);

	DoMethod(menu, OM_ADDMEMBER, MUI_NewObject(MUIC_Menuitem, MUIA_Menuitem_Title, NM_BARLABEL, TAG_END));

	DoMethod(menu, OM_ADDMEMBER,
	 MUI_NewObject(MUIC_Menuitem, MUIA_UserData, SBR_MENU_QUIT, MUIA_Menuitem_Title, GetString(MSG_MENU_APPMENU_QUIT), TAG_END));

	DoMethod(menu, MUIM_Group_ExitChange);

	if(strip)
	{
		ULONG result = DoMethod(strip, MUIM_Menustrip_Popup, obj, 0, _left(obj), _bottom(obj)+1);

		if(result != 0) DoMethod(d->app, MUIM_Application_PushMethod, d->app, 2, APPM_ScreenbarMenu, result);
		MUI_DisposeObject(strip);
	}

	return (IPTR)1;
}

static IPTR SmallSBarDispatcher(void)
{
	Class *cl = (Class*)REG_A0;
	Object *obj = (Object*)REG_A2;
	Msg msg = (Msg)REG_A1;

	switch(msg->MethodID)
	{
		case OM_NEW: return (SmallSBarNew(cl, obj, (struct opSet*)msg));
		case OM_GET: return (SmallSBarGet(cl, obj, (struct opGet*)msg));
		case MUIM_Setup: return(SmallSBarSetup(cl, obj, (struct MUIP_Setup*)msg));
		case MUIM_Cleanup: return(SmallSBarCleanup(cl, obj, (struct MUIP_Cleanup*)msg));
		case MUIM_Draw: return(SmallSBarDraw(cl, obj, (struct MUIP_Draw*)msg));
		case MUIM_AskMinMax: return(SmallSBarAskMinMax(cl, obj, (struct MUIP_AskMinMax*)msg));
		case MUIM_Screenbar_BuildSettingsPanel: return(SmallSBarBuildSettingsPanel(cl, obj, (struct MUIP_Screenbar_BuildSettingsPanel*)msg));
		case MUIM_Screenbar_Signal: return(SmallSBarSignal(cl, obj, (struct MUIP_Screenbar_Signal*)msg));
		case MUIM_HandleEvent: return(SmallSBarHandleEvent(cl, obj, (struct MUIP_HandleEvent*)msg));
		case SBRM_ShowMenu: return(SmallSBarShowMenu(cl, obj));
		default:	return (DoSuperMethodA(cl, obj, msg));
	}
}


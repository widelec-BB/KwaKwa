/*
 * Copyright (c) 2012-2022 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/cybergraphics.h>
#include <proto/asl.h>
#include <proto/wb.h>
#include <libvstring.h>

#include "globaldefines.h"
#include "locale.h"
#include "prefswindow.h"
#include "kwakwa_api/pictures.h"
#include "support.h"
#include "pictureview.h"

#define PVV_CONTEXTMENU_SAVE_AS  1
#define PVV_CONTEXTMENU_OPEN     2

struct MUI_CustomClass *PictureViewClass;

static IPTR PictureViewDispatcher(VOID);
const struct EmulLibEntry PictureViewGate = {TRAP_LIB, 0, (VOID(*)(VOID))PictureViewDispatcher};

struct PictureViewData
{
	APTR  pvd_PictureData;
	ULONG pvd_PictureDataSize;
	struct Picture *pvd_Picture;
	Object *pvd_MenuStrip;
	struct MUI_EventHandlerNode handler;
	ULONG pvd_PrevClickSec, pvd_PrevClickMic;
	STRPTR pvd_ShortHelp;
	ULONG pvd_GlobalAlpha;
};


struct MUI_CustomClass *CreatePictureViewClass(VOID)
{
	struct MUI_CustomClass *cl;

	cl = MUI_CreateCustomClass(NULL, MUIC_Area, NULL, sizeof(struct PictureViewData), (APTR)&PictureViewGate);
	PictureViewClass = cl;
	return cl;
}


VOID DeletePictureViewClass(VOID)
{
	if (PictureViewClass) MUI_DeleteCustomClass(PictureViewClass);
}

static IPTR PictureViewNew(Class *cl, Object *obj, struct opSet *msg)
{
	obj = (Object*)DoSuperNew(cl, obj,
		MUIA_Unicode, TRUE,
		MUIA_DoubleBuffer, FALSE, /* virtual text object has own double buffer, so we can't use it here (MUI bug?) */
	TAG_MORE, (IPTR)msg->ops_AttrList);

	if(obj)
	{
		struct PictureViewData *d = INST_DATA(cl, obj);
		struct TagItem *act_tag, *itr_tag = msg->ops_AttrList;
		APTR data = NULL;
		STRPTR path = NULL;
		ULONG alpha = 0xFFFFFFFF;

		d->pvd_MenuStrip = MUI_NewObject(MUIC_Menustrip,
			MUIA_Unicode, TRUE,
			MUIA_Group_Child, MUI_NewObject(MUIC_Menu,
				MUIA_Group_Child, MUI_NewObject(MUIC_Menuitem,
					MUIA_Menuitem_Title, GetString(MSG_PICTUREVIEW_MENU_SAVEAS),
					MUIA_UserData, PVV_CONTEXTMENU_SAVE_AS,
				TAG_END),
				MUIA_Group_Child, MUI_NewObject(MUIC_Menuitem,
					MUIA_Menuitem_Title, GetString(MSG_PICTUREVIEW_MENU_OPEN),
					MUIA_UserData, PVV_CONTEXTMENU_OPEN,
				TAG_END),
			TAG_END),
		TAG_END);

		while((act_tag = NextTagItem(&itr_tag)))
		{
			switch(act_tag->ti_Tag)
			{
				case PVA_PictureFilePath:
					path = (STRPTR)act_tag->ti_Data;
				break;

				case PVA_PictureData:
					data = (APTR)act_tag->ti_Data; /* temporary store pointer to be sure we will know size before allocating memory */
				break;

				case PVA_PictureDataSize:
					d->pvd_PictureDataSize = act_tag->ti_Data;
				break;

				case MUIA_ShortHelp:
					d->pvd_ShortHelp = StrNew((STRPTR)act_tag->ti_Data);
				break;

				case PVA_GlobalAlpha:
					alpha = act_tag->ti_Data;
				break;
			}
		}

		d->pvd_GlobalAlpha = alpha;

		if(data && d->pvd_PictureDataSize)
		{
			if((d->pvd_PictureData = AllocMem(d->pvd_PictureDataSize, MEMF_ANY)))
			{
				QUAD len = d->pvd_PictureDataSize;

				CopyMem(data, d->pvd_PictureData, d->pvd_PictureDataSize);

				if((d->pvd_Picture = LoadPictureMemory(d->pvd_PictureData, &len)))
				{
					set(obj, MUIA_ContextMenu, d->pvd_MenuStrip);
					return (IPTR)obj;
				}
			}
		}

		if(path)
		{
			if((d->pvd_Picture = LoadPictureFile(path)))
				return (IPTR)obj;
		}
	}
	CoerceMethod(cl, obj, OM_DISPOSE);
	return (IPTR)NULL;
}

static IPTR PictureViewGet(Class *cl, Object *obj, struct opGet *msg)
{
	struct PictureViewData *d = INST_DATA(cl, obj);
	int rv = FALSE;

	switch(msg->opg_AttrID)
	{
		case MUIA_ShortHelp:
		case MUIA_Text_Contents:
			*msg->opg_Storage = (IPTR)(d->pvd_ShortHelp ? d->pvd_ShortHelp : (STRPTR)"");
		return TRUE;

		case PVA_GlobalAlpha:
			*msg->opg_Storage = d->pvd_GlobalAlpha;
		return TRUE;

		default:
			rv = (DoSuperMethodA(cl, obj, (Msg)msg));
	}

	return rv;
}

static IPTR PictureViewSet(Class *cl, Object *obj, struct opSet *msg)
{
	struct PictureViewData *d = INST_DATA(cl, obj);
	int tagcount = 0;
	struct TagItem *tag = 0, *tagptr = msg->ops_AttrList;

	while((tag = NextTagItem(&tagptr)))
	{
		switch(tag->ti_Tag)
		{
			case PVA_GlobalAlpha:
				d->pvd_GlobalAlpha = tag->ti_Data;
			break;
		}
	}

	tagcount += DoSuperMethodA(cl, obj, (Msg)msg);
	return tagcount;
}

static IPTR PictureViewDispose(Class *cl, Object *obj, Msg msg)
{
	struct PictureViewData *d = INST_DATA(cl, obj);

	if(d->pvd_PictureData)
		FreeMem(d->pvd_PictureData, d->pvd_PictureDataSize);

	if(d->pvd_Picture)
		FreePicture(d->pvd_Picture);

	if(d->pvd_MenuStrip)
		MUI_DisposeObject(d->pvd_MenuStrip);

	return DoSuperMethodA(cl, obj, msg);
}

static IPTR PictureViewHandleEvent(Class *cl, Object *obj, struct MUIP_HandleEvent *msg)
{
	struct PictureViewData *d = INST_DATA(cl, obj);
	struct IntuiMessage *imsg = msg->imsg;
	IPTR result = 0;

	if(imsg)
	{
		if(imsg->Class == IDCMP_MOUSEBUTTONS)
		{
			if(_isinobject(imsg->MouseX, imsg->MouseY))
			{
				if(imsg->Code == IECODE_LBUTTON)
				{
					ULONG cur_click_sec, cur_click_mic;

					CurrentTime(&cur_click_sec, &cur_click_mic);

					if(DoubleClick(d->pvd_PrevClickSec, d->pvd_PrevClickMic, cur_click_sec, cur_click_mic))
					{
						DoMethod(obj, PVM_OpenPicture);
						result |= MUI_EventHandlerRC_Eat;
					}
					else
					{
						d->pvd_PrevClickSec = cur_click_sec;
						d->pvd_PrevClickMic = cur_click_mic;
					}
				}
			}
		}
	}

	return result;
}

static IPTR PictureViewSetup(Class *cl, Object *obj, struct MUIP_Setup *msg)
{
	struct PictureViewData *d = INST_DATA(cl, obj);
	IPTR res = DoSuperMethodA(cl, obj, msg);

	d->handler.ehn_Class = cl;
	d->handler.ehn_Object = obj;
	d->handler.ehn_Events = IDCMP_MOUSEBUTTONS;

	DoMethod(_win(obj), MUIM_Window_AddEventHandler, &d->handler);

	return res;
}

static IPTR PictureViewCleanup(Class *cl, Object *obj, struct MUIP_Cleanup *msg)
{
	struct PictureViewData *d = INST_DATA(cl, obj);

	DoMethod(_win(obj), MUIM_Window_RemEventHandler, &d->handler);

	return DoSuperMethodA(cl, obj, msg);
}

static IPTR PictureViewAskMinMax(Class *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
	struct PictureViewData *d = INST_DATA(cl, obj);
	IPTR result = DoSuperMethodA(cl, obj, msg);
	ULONG max_width = xget(prefs_object(USD_PREFS_TW_PICTURES_MAXWIDTH), MUIA_Slider_Level);
	ULONG w, h;

	if(max_width == xget(prefs_object(USD_PREFS_TW_PICTURES_MAXWIDTH), MUIA_Slider_Max) || d->pvd_Picture->p_Width < max_width)
	{
		w = d->pvd_Picture->p_Width;
		h = d->pvd_Picture->p_Height;
	}
	else
	{
		w = max_width;
		h = d->pvd_Picture->p_Height * ((DOUBLE)w) / ((DOUBLE)d->pvd_Picture->p_Width);
	}

	msg->MinMaxInfo->MinWidth  += w;
	msg->MinMaxInfo->MinHeight += h;
	msg->MinMaxInfo->DefWidth  += w;
	msg->MinMaxInfo->DefHeight += h;
	msg->MinMaxInfo->MaxWidth  += w;
	msg->MinMaxInfo->MaxHeight += h;

	return result;
}

static IPTR PictureViewDraw(Class *cl, Object *obj, struct MUIP_Draw *msg)
{
	struct PictureViewData *d = INST_DATA(cl, obj);
	IPTR result = DoSuperMethodA(cl, obj, msg);

	if(msg->flags & MADF_DRAWOBJECT)
	{
		if(_mwidth(obj) < d->pvd_Picture->p_Width)
		{
			ScalePixelArrayAlpha(d->pvd_Picture->p_Data, d->pvd_Picture->p_Width, d->pvd_Picture->p_Height, d->pvd_Picture->p_Width << 2,
			 _rp(obj), _left(obj), _top(obj), _width(obj), _height(obj), d->pvd_GlobalAlpha);
		}
		else
		{
			WritePixelArrayAlpha(d->pvd_Picture->p_Data, 0, 0, d->pvd_Picture->p_Width << 2, _rp(obj), _left(obj),
			 _top(obj), _width(obj), _height(obj), d->pvd_GlobalAlpha);
		}
	}

	return result;
}

static IPTR PictureViewContextMenuChoice(Class *cl, Object *obj, struct MUIP_ContextMenuChoice *msg)
{
	switch(muiUserData(msg->item))
	{
		case PVV_CONTEXTMENU_SAVE_AS:
			return DoMethod(obj, PVM_SavePicture);

		case PVV_CONTEXTMENU_OPEN:
			return DoMethod(obj, PVM_OpenPicture);
	}

	return (IPTR)0;
}

static IPTR PictureViewSavePicture(Class *cl, Object *obj)
{
	struct PictureViewData *d = INST_DATA(cl, obj);
	BOOL result = FALSE;
	struct FileRequester *freq;

	if((freq = MUI_AllocAslRequestTags(ASL_FileRequest, TAG_END)))
	{
		STRPTR last_path = LoadFile(GUI_CACHE_DIR "picture_save_asl.cfg", NULL);

		if(MUI_AslRequestTags(freq,
			ASLFR_Window, _window(obj),
			ASLFR_TitleText, GetString(MSG_PICTUREVIEW_ASL_TITLE),
			ASLFR_PositiveText, GetString(MSG_PICTUREVIEW_ASL_SAVEAS_POSITIVE),
			ASLFR_InitialPattern, "#?",
			ASLFR_DoPatterns, TRUE,
			ASLFR_RejectIcons, TRUE,
			ASLFR_PrivateIDCMP, TRUE,
			ASLFR_DoSaveMode, TRUE,
			ASLFR_InitialDrawer, last_path ? last_path : (STRPTR)"",
		TAG_END))
		{
			UBYTE location[500];

			FmtNPut(location, GUI_CACHE_DIR "picture_save_asl.cfg", sizeof(location));
			SaveFile(location, freq->fr_Drawer, StrLen(freq->fr_Drawer));

			StrNCopy(freq->fr_Drawer, (STRPTR)location, sizeof(location));
			AddPart((STRPTR)location, freq->fr_File, sizeof(location));

			if(SaveFile(location, d->pvd_PictureData, d->pvd_PictureDataSize))
				result = TRUE;

			if(!result)
				MUI_Request_Unicode(_app(obj), obj, APP_NAME, GetString(MSG_SEND_PICTURE_FAIL_BUTTONS), GetString(MSG_SEND_PICTURE_FAIL_MSG));
		}

		if(last_path)
			FreeVec(last_path);

		MUI_FreeAslRequest(freq);
	}

	return(IPTR)result;
}

static IPTR PictureViewOpenPicture(Class *cl, Object *obj)
{
	struct PictureViewData *d = INST_DATA(cl, obj);
	BOOL result = FALSE;
	ULONG id = GetUniqueID();
	UBYTE buffer[32];

	FmtNPut(buffer, "T:.kwakwa_%x", sizeof(buffer), id);

	if(SaveFile(buffer, d->pvd_PictureData, d->pvd_PictureDataSize))
		result = OpenWorkbenchObjectA(buffer, NULL);

	return result;
}

static IPTR PictureViewDispatcher(VOID)
{
	Class *cl = (Class*)REG_A0;
	Object *obj = (Object*)REG_A2;
	Msg msg = (Msg)REG_A1;

	switch (msg->MethodID)
	{
		case OM_NEW: return(PictureViewNew(cl, obj, (struct opSet*)msg));
		case OM_GET: return(PictureViewGet(cl, obj, (struct opGet*)msg));
		case OM_SET: return(PictureViewSet(cl, obj, (struct opSet*)msg));
		case OM_DISPOSE: return(PictureViewDispose(cl, obj, msg));
		case MUIM_HandleEvent: return(PictureViewHandleEvent(cl, obj, (struct MUIP_HandleEvent*)msg));
		case MUIM_Setup: return(PictureViewSetup(cl, obj, (struct MUIP_Setup*)msg));
		case MUIM_Cleanup: return(PictureViewCleanup(cl, obj, (struct MUIP_Cleanup*)msg));
		case MUIM_AskMinMax: return(PictureViewAskMinMax(cl, obj, (struct MUIP_AskMinMax*)msg));
		case MUIM_Draw: return(PictureViewDraw(cl, obj, (struct MUIP_Draw*)msg));
		case MUIM_ContextMenuChoice: return(PictureViewContextMenuChoice(cl, obj, (struct MUIP_ContextMenuChoice*)msg));
		case PVM_SavePicture: return(PictureViewSavePicture(cl, obj));
		case PVM_OpenPicture: return(PictureViewOpenPicture(cl, obj));
		default: return(DoSuperMethodA(cl, obj, msg));
	}
}

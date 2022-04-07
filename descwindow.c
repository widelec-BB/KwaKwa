/*
 * Copyright (c) 2012-2022 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <proto/exec.h>
#include <libvstring.h>
#include "globaldefines.h"
#include "locale.h"
#include "support.h"
#include "application.h"
#include "kwakwa_api/defs.h"

#include "inputfield.h"
#include "simplestringlist.h"
#include "descwindow.h"
#include "prefswindow.h"

struct MUI_CustomClass *DescWindowClass;


static IPTR DescWindowDispatcher(void);
const struct EmulLibEntry DescWindowGate = {TRAP_LIB, 0, (void(*)(void))DescWindowDispatcher};

struct DWP_ChangeDesc
{
	ULONG MethodID;
	ULONG status;
};

struct DWP_SearchInList
{
	ULONG MethodID;
	STRPTR desc;
};

struct DescWindowData
{
	Object *string, *add_to_list, *list, *search_string;
	STRPTR win_title;
	LONG input_class;
};

struct MUI_CustomClass *CreateDescWindowClass(void)
{
	struct MUI_CustomClass *cl;

	cl = MUI_CreateCustomClass(NULL, MUIC_Window, NULL, sizeof(struct DescWindowData), (APTR)&DescWindowGate);
	DescWindowClass = cl;
	return cl;
}

void DeleteDescWindowClass(void)
{
	if (DescWindowClass) MUI_DeleteCustomClass(DescWindowClass);
}

static IPTR DescWindowNew(Class *cl, Object *obj, struct opSet *msg)
{
	Object *list = NewObject(SimpleStringListClass->mcc_Class, NULL,
		MUIA_ObjectID, USD_DESC_LIST,
		MUIA_UserData, USD_DESC_LIST,
	TAG_END);

	Object *string, *add_to_list, *show_list, *buttons[6], *search_string, *next_button;
	Object *list_group;

	obj = (Object*)DoSuperNew(cl, obj,
		MUIA_Background, (ULONG)MUII_WindowBack,
		MUIA_Window_ID, (ULONG)USD_DESC_WINDOW,
		MUIA_UserData, (ULONG)USD_DESC_WINDOW,
		MUIA_Window_Title, (ULONG)GetString(MSG_DESCWINDOW_TITLE),
		MUIA_Window_ScreenTitle, (ULONG)APP_SCREEN_TITLE,
		MUIA_Window_RootObject, (ULONG)MUI_NewObject(MUIC_Group,
			MUIA_Group_Child, (IPTR)MUI_NewObject(MUIC_Group,
				MUIA_Group_Child, (ULONG)(string = NewObject(InputFieldUnicodeClass->mcc_Class, NULL,
					MUIA_UserData, USD_DESC_STRING,
					MUIA_ObjectID, USD_DESC_STRING,
				TAG_END)),
			TAG_END),
			MUIA_Group_Child, MUI_NewObject(MUIC_Group,
				MUIA_Group_Horiz, TRUE,
				MUIA_Group_Child, (IPTR)(add_to_list = CheckBox(USD_DESC_ADDTOLIST_CHECK, TRUE, GetString(MSG_DESCWINDOW_ADDTOLIST_CHECK_HELP))),
				MUIA_Group_Child, StringLabel(GetString(MSG_DESCWINDOW_ADDTOLIST_CHECK), "\33l"),
				MUIA_Group_Child, EmptyRectangle(100),
				MUIA_Group_Child, (IPTR)(show_list = CheckBox(USD_DESC_SHOWLIST_CHECK, FALSE, NULL)),
				MUIA_Group_Child, StringLabel(GetString(MSG_DESCWINDOW_SHOWLIST_CHECK), "\33l"),
			TAG_END),
			MUIA_Group_Child, (ULONG)(list_group = MUI_NewObject(MUIC_Group,
				MUIA_ShowMe, FALSE,
				MUIA_Group_Child, (ULONG)MUI_NewObject(MUIC_Group,
					MUIA_Group_Horiz, TRUE,
					MUIA_Group_Child, (ULONG)(search_string = MUI_NewObject(MUIC_String,
						MUIA_Frame, MUIV_Frame_String,
						MUIA_Background, MUII_StringBack,
						MUIA_CycleChain, TRUE,
						MUIA_UserData, USD_DESC_SEARCH_STRING,
						MUIA_ShortHelp, GetString(MSG_DESCWINDOW_SEARCH_STRING_HELP),
					TAG_END)),
					MUIA_Group_Child, (ULONG)(next_button = NormalButton(GetString(MSG_DESCWINDOW_SEARCH_NEXT), *GetString(MSG_DESCWINDOW_SEARCH_NEXT_HOTKEY), USD_DESC_SEARCH_NEXT_BUTTON, 0)),
				TAG_END),
				MUIA_Group_Child, list,
			TAG_END)),
			MUIA_Group_Child, (ULONG)MUI_NewObject(MUIC_Group,
				MUIA_Group_Horiz, (ULONG)TRUE,
				MUIA_Group_Child, (ULONG)EmptyRectangle(40),
				MUIA_Group_Child, (ULONG) (buttons[0] = MUI_NewObject(MUIC_Text,
					MUIA_Text_Contents, (ULONG)"\33c\33I[4:PROGDIR:gfx/available.mbr]",
					MUIA_Frame, MUIV_Frame_Button,
					MUIA_Background, MUII_ButtonBack,
					MUIA_Font, MUIV_Font_Button,
					MUIA_InputMode, MUIV_InputMode_RelVerify,
					MUIA_CycleChain, TRUE,
					MUIA_HorizWeight, 1,
					MUIA_ShortHelp, GetString(MSG_GG_STATUS_AVAIL),
					MUIA_Unicode, TRUE,
				TAG_END)),
				MUIA_Group_Child, (ULONG) (buttons[1] = MUI_NewObject(MUIC_Text,
					MUIA_Text_Contents, (ULONG)"\33c\33I[4:PROGDIR:gfx/away.mbr]",
					MUIA_Frame, MUIV_Frame_Button,
					MUIA_Background, MUII_ButtonBack,
					MUIA_Font, MUIV_Font_Button,
					MUIA_InputMode, MUIV_InputMode_RelVerify,
					MUIA_CycleChain, TRUE,
					MUIA_HorizWeight, 1,
					MUIA_ShortHelp, GetString(MSG_GG_STATUS_AWAY),
					MUIA_Unicode, TRUE,
				TAG_END)),
				MUIA_Group_Child, (ULONG) (buttons[2] = MUI_NewObject(MUIC_Text,
					MUIA_Text_Contents, (ULONG)"\33c\33I[4:PROGDIR:gfx/invisible.mbr]",
					MUIA_Frame, MUIV_Frame_Button,
					MUIA_Background, MUII_ButtonBack,
					MUIA_Font, MUIV_Font_Button,
					MUIA_InputMode, MUIV_InputMode_RelVerify,
					MUIA_CycleChain, TRUE,
					MUIA_HorizWeight, 1,
					MUIA_ShortHelp, GetString(MSG_GG_STATUS_INVISIBLE),
					MUIA_Unicode, TRUE,
				TAG_END)),
				MUIA_Group_Child, (ULONG) (buttons[3] = MUI_NewObject(MUIC_Text,
					MUIA_Text_Contents, (ULONG)"\33c\33I[4:PROGDIR:gfx/unavailable.mbr]",
					MUIA_Frame, MUIV_Frame_Button,
					MUIA_Background, MUII_ButtonBack,
					MUIA_Font, MUIV_Font_Button,
					MUIA_InputMode, MUIV_InputMode_RelVerify,
					MUIA_CycleChain, TRUE,
					MUIA_HorizWeight, 1,
					MUIA_ShortHelp, GetString(MSG_GG_STATUS_UNAVAIL),
					MUIA_Unicode, TRUE,
				TAG_END)),
				MUIA_Group_Child, (ULONG)EmptyRectangle(5),
				MUIA_Group_Child, (ULONG) (buttons[4] = MUI_NewObject(MUIC_Text,
					MUIA_Text_Contents, (ULONG)"\33c\33I[4:PROGDIR:gfx/ffc.mbr]",
					MUIA_Frame, MUIV_Frame_Button,
					MUIA_Background, MUII_ButtonBack,
					MUIA_Font, MUIV_Font_Button,
					MUIA_InputMode, MUIV_InputMode_RelVerify,
					MUIA_CycleChain, TRUE,
					MUIA_HorizWeight, 1,
					MUIA_ShortHelp, GetString(MSG_GG_STATUS_FFC),
					MUIA_Unicode, TRUE,
				TAG_END)),
				MUIA_Group_Child, (ULONG) (buttons[5] = MUI_NewObject(MUIC_Text,
					MUIA_Text_Contents, (ULONG)"\33c\33I[4:PROGDIR:gfx/dnd.mbr]",
					MUIA_Frame, MUIV_Frame_Button,
					MUIA_Background, MUII_ButtonBack,
					MUIA_Font, MUIV_Font_Button,
					MUIA_InputMode, MUIV_InputMode_RelVerify,
					MUIA_CycleChain, TRUE,
					MUIA_HorizWeight, 1,
					MUIA_ShortHelp, GetString(MSG_GG_STATUS_DND),
					MUIA_Unicode, TRUE,
				TAG_END)),
				MUIA_Group_Child, (ULONG)EmptyRectangle(40),
			TAG_END),
		TAG_END),
	TAG_MORE, (ULONG)msg->ops_AttrList);

	if(obj)
	{
		struct DescWindowData *d = INST_DATA(cl, obj);

		if((d->win_title = Utf8ToSystem((STRPTR)xget(obj, MUIA_Window_Title))))
			set(obj, MUIA_Window_Title, (IPTR)d->win_title);

		d->string = string;
		d->add_to_list = add_to_list;
		d->list = list;
		d->search_string = search_string;
		d->input_class = DWV_InputFieldClassUnicode;

		DoMethod(buttons[0], MUIM_Notify, MUIA_Pressed, FALSE, MUIV_Notify_Window, 2,
		 DWM_ChangeDesc, KWA_STATUS_AVAIL);

		DoMethod(buttons[1], MUIM_Notify, MUIA_Pressed, FALSE, MUIV_Notify_Window, 2,
		 DWM_ChangeDesc, KWA_STATUS_BUSY);

		DoMethod(buttons[2], MUIM_Notify, MUIA_Pressed, FALSE, MUIV_Notify_Window, 2,
		 DWM_ChangeDesc, KWA_STATUS_INVISIBLE);

		DoMethod(buttons[3], MUIM_Notify, MUIA_Pressed, FALSE, MUIV_Notify_Window, 2,
		 DWM_ChangeDesc, KWA_STATUS_NOT_AVAIL);

		DoMethod(buttons[4], MUIM_Notify, MUIA_Pressed, FALSE, MUIV_Notify_Window, 2,
		 DWM_ChangeDesc, KWA_STATUS_FFC);

		DoMethod(buttons[5], MUIM_Notify, MUIA_Pressed, FALSE, MUIV_Notify_Window, 2,
		 DWM_ChangeDesc, KWA_STATUS_DND);

		DoMethod(obj, MUIM_Notify, MUIA_Window_Open, TRUE, MUIV_Notify_Self, 3,
		 MUIM_Set, MUIA_Window_ActiveObject, (ULONG)string);

		DoMethod(obj, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, MUIV_Notify_Self, 3,
		 MUIM_Set, MUIA_Window_Open, FALSE);

		DoMethod(show_list, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, (ULONG)list_group, 3,
		 MUIM_Set, MUIA_ShowMe, MUIV_TriggerValue);

		DoMethod(list, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime, MUIV_Notify_Window, 1,
		 DWM_ChangeListActive);

		DoMethod(obj, MUIM_Notify, MUIA_Window_Open, TRUE, MUIV_Notify_Self, 1,
		 DWM_LoadActualDescription);

		DoMethod(search_string, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime, (ULONG)list, 3,
		 MUIM_SimpleStringList_Find, MUIV_TriggerValue, FALSE);

		DoMethod(next_button, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)obj, 1,
		 DWM_SearchNext);

		DoMethod(prefs_object(USD_PREFS_DESCWINDOW_INPUT_GADGET), MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime, (IPTR)obj, 3,
		 MUIM_Set, DWA_InputFieldClass, MUIV_TriggerValue);

		set(string, IFA_SendAfterReturn, FALSE);

		return (IPTR)obj;
	}

	CoerceMethod(cl, obj, OM_DISPOSE);
	return (IPTR)0;
}

static IPTR DescWindowDispose(Class *cl, Object *obj, Msg msg)
{
	struct DescWindowData *d = INST_DATA(cl, obj);

	if(d->win_title)
		StrFree(d->win_title);

	return DoSuperMethodA(cl, obj, msg);
}

static BOOL ChangeInputField(Class *cl, Object *obj, LONG new_class_type)
{
	struct DescWindowData *d = INST_DATA(cl, obj);
	STRPTR current_text = (STRPTR)DoMethod(d->string, IFM_ExportText);
	Object *group = (Object*)xget(d->string, MUIA_Parent), *new_input;
	struct MUI_CustomClass *new_class = InputFieldUnicodeClass;

	if(d->input_class == new_class_type)
		return TRUE;

	if(new_class_type == DWV_InputFieldClassTextEditor && InputFieldClass)
	{
		DoMethod(d->list, MUIM_SimpleStringList_AddEventHandler);
		new_class = InputFieldClass;
	}
	else
		DoMethod(d->list, MUIM_SimpleStringList_RemEventHandler);

	new_input = NewObject(new_class->mcc_Class, NULL,
		MUIA_UserData, USD_DESC_STRING,
		MUIA_ObjectID, USD_DESC_STRING,
	TAG_END);

	if(!new_input)
		return FALSE;

	set(new_input, IFA_SendAfterReturn, FALSE);
	if(current_text)
	{
		set(new_input, IFA_TextContents, current_text);
		StrFree(current_text);
	}

	DoMethod(group, MUIM_Group_InitChange);

	DoMethod(group, OM_REMMEMBER, (IPTR)d->string);
	DoMethod(group, OM_ADDMEMBER, (IPTR)new_input);

	DoMethod(group, MUIM_Group_ExitChange);

	DoMethod(obj, MUIM_Notify, MUIA_Window_Open, TRUE, MUIV_Notify_Self, 3,
	 MUIM_Set, MUIA_Window_ActiveObject, (IPTR)new_input);

	MUI_DisposeObject(d->string);
	d->string = new_input;

	d->input_class = new_class_type;

	return TRUE;
}

static IPTR DescWindowSet(Class *cl, Object *obj, struct opSet *msg)
{
	int tagcount = 0;
	struct TagItem *tag, *tagptr = msg->ops_AttrList;

	while((tag = NextTagItem(&tagptr)))
	{
		switch(tag->ti_Tag)
		{
			case DWA_InputFieldClass:
				ChangeInputField(cl, obj, tag->ti_Data);
				tagcount++;
			break;
		}
	}

	tagcount += DoSuperMethodA(cl, obj, (Msg)msg);
	return tagcount;
}

static IPTR DescWindowChangeStatus(Class *cl, Object *obj, struct DWP_ChangeDesc *msg)
{
	struct DescWindowData *d = INST_DATA(cl, obj);
	STRPTR desc = (STRPTR) DoMethod(d->string, IFM_ExportText);
	IPTR result = 0;

	if(desc)
	{
		if((BOOL) xget(d->add_to_list, MUIA_Selected))
		{
			DoMethod(obj, DWM_AddDescToList);
			set(d->add_to_list, MUIA_Selected, FALSE);
		}

		set(obj, MUIA_Window_Open, FALSE);

		result = (IPTR)DoMethod(_app(obj), APPM_ChangeStatus, (ULONG)msg->status, (ULONG)desc);
		FreeVec(desc);
	}
	return (IPTR)result;
}

static IPTR DescWindowChangeListActive(Class *cl, Object *obj)
{
	struct DescWindowData *d = INST_DATA(cl, obj);
	STRPTR desc;

	DoMethod(d->list, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (ULONG)&desc);

	if((desc = StrNew(desc)))
	{
		STRPTR p = desc;

		while(*p != 0x00)
		{
			if(*p == 0x07)
				*p = '\n';
			p++;
		}
	}

	set(d->string, IFA_TextContents, desc);

	if(desc)
		StrFree(desc);

	return (IPTR)1;
}

static IPTR DescWindowAddDescToList(Class *cl, Object *obj)
{
	struct DescWindowData *d = INST_DATA(cl, obj);
	STRPTR new_desc = (STRPTR) DoMethod(d->string, IFM_ExportText), temp_desc;
	ULONG i;

	if(new_desc)
	{
		STRPTR p = new_desc;

		while(*p != 0x00)
		{
			if(*p == '\n')
				*p = 0x07;
			p++;
		}

		for(i=0; ;i++)
		{
			DoMethod(d->list, MUIM_List_GetEntry, i, (ULONG)&temp_desc);
			if(temp_desc == NULL)
				break;
			if(StrEqu(temp_desc, new_desc))
				break;
		}

		if(temp_desc == NULL)
		{
			DoMethod(d->list, MUIM_List_InsertSingle, (ULONG)new_desc, MUIV_List_Insert_Top);
		}
		StrFree(new_desc);
	}

	return (IPTR)1;
}

static IPTR DescWindowLoadActualDescription(Class *cl, Object *obj)
{
	struct DescWindowData *d = INST_DATA(cl, obj);

	set(d->string, IFA_TextContents, xget(_app(obj), APPA_Description));

	return (IPTR)0;
}

static IPTR DescWindowSearchNext(Class *cl, Object *obj)
{
	struct DescWindowData *d = INST_DATA(cl, obj);

	DoMethod(d->list, MUIM_SimpleStringList_Find, xget(d->search_string, MUIA_String_Contents), TRUE);

	return 0;
}

static IPTR DescWindowDispatcher(void)
{
	Class *cl = (Class*)REG_A0;
	Object *obj = (Object*)REG_A2;
	Msg msg = (Msg)REG_A1;

	switch (msg->MethodID)
	{
		case OM_NEW: return (DescWindowNew(cl, obj, (struct opSet*)msg));
		case OM_DISPOSE: return(DescWindowDispose(cl, obj, msg));
		case OM_SET: return(DescWindowSet(cl, obj, (struct opSet*)msg));
		case DWM_ChangeDesc: return(DescWindowChangeStatus(cl, obj, (struct DWP_ChangeDesc*)msg));
		case DWM_ChangeListActive: return(DescWindowChangeListActive(cl, obj));
		case DWM_AddDescToList: return(DescWindowAddDescToList(cl, obj));
		case DWM_LoadActualDescription: return(DescWindowLoadActualDescription(cl, obj));
		case DWM_SearchNext: return(DescWindowSearchNext(cl, obj));
		default: return (DoSuperMethodA(cl, obj, msg));
	}
}


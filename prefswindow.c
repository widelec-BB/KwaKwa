/*
 * Copyright (c) 2012-2022 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <proto/asl.h>
#include <proto/exec.h>
#include <libvstring.h>

#include "globaldefines.h"
#include "application.h"
#include "contactslist.h"
#include "locale.h"
#include "percentageslider.h"
#include "timeslider.h"
#include "minmaxslider.h"
#include "modules.h"
#include "kwakwa_api/protocol.h"
#include "support.h"
#include "parseftpurl.h"
#include "emoticonstab.h"
#include "prefswindow.h"


struct MUI_CustomClass *PrefsWindowClass;
static IPTR PrefsWindowDispatcher(void);
const struct EmulLibEntry PrefsWindowGate = {TRAP_LIB, 0, (void(*)(void))PrefsWindowDispatcher};

Object *PreferencesWindow; /* one, dirty, global pointer for reading preferences in all classes */

/* methods structs */

struct PWP_SavePrefs {ULONG MethodId; ULONG EnvArc;};
struct PWP_AddPrefsPage {ULONG MethodID; Object *PrefsPage;};
struct PWP_ParseFTPUrl {ULONG MethodID; STRPTR Url;};


struct PrefsWindowData
{
	CONST_STRPTR statuses[6];
	Object *page_group;
	Object *tabs_list;
};

struct MUI_CustomClass *CreatePrefsWindowClass(void)
{
	struct MUI_CustomClass *cl;

	cl = MUI_CreateCustomClass(NULL, MUIC_Window, NULL, sizeof(struct PrefsWindowData), (APTR)&PrefsWindowGate);
	PrefsWindowClass = cl;
	return cl;
}

void DeletePrefsWindowClass(void)
{
	if (PrefsWindowClass) MUI_DeleteCustomClass(PrefsWindowClass);
}

static inline Object *CreateProgramPrefs(CONST_STRPTR statuses[6])
{
	Object *result;
	Object *auto_check, *const_group, *last_group, *auto_type;
	Object *away_slider, *away_group, *show_hide_but_check;
	static CONST_STRPTR auto_type_entries[3];
	static CONST_STRPTR mwm_closing_entries[4];
	static CONST_STRPTR last_notavail_entries[7];

	auto_type_entries[0] = GetString(MSG_PREFS_PROGRAM_CONNECT_AUTO_TYPE_CONST);
	auto_type_entries[1] = GetString(MSG_PREFS_PROGRAM_CONNECT_AUTO_TYPE_LAST);
	auto_type_entries[2] = NULL;

	mwm_closing_entries[0] = GetString(MSG_PREFS_PROGRAM_MAIN_WINDOW_CLOSE_QUIT);
	mwm_closing_entries[1] = GetString(MSG_PREFS_PROGRAM_MAIN_WINDOW_CLOSE_HIDE);
	mwm_closing_entries[2] = GetString(MSG_PREFS_PROGRAM_MAIN_WINDOW_CLOSE_ICONIFY);
	mwm_closing_entries[3] = NULL;

	last_notavail_entries[0] = GetString(MSG_PREFS_PROGRAM_CONNECT_AUTO_NOTAVAIL_OFF);
	last_notavail_entries[1] = GetString(MSG_PREFS_PROGRAM_CONNECT_AUTO_NOTAVAIL_AVAIL);
	last_notavail_entries[2] = GetString(MSG_PREFS_PROGRAM_CONNECT_AUTO_NOTAVAIL_BUSY);
	last_notavail_entries[3] = GetString(MSG_PREFS_PROGRAM_CONNECT_AUTO_NOTAVAIL_INVISIBLE);
	last_notavail_entries[4] = GetString(MSG_PREFS_PROGRAM_CONNECT_AUTO_NOTAVAIL_FFC);
	last_notavail_entries[5] = GetString(MSG_PREFS_PROGRAM_CONNECT_AUTO_NOTAVAIL_DND);
	last_notavail_entries[6] = NULL;

	result = MUI_NewObject(MUIC_Group,
		MUIA_Group_Child, EmptyRectangle(100),
		/* main window */
		MUIA_Group_Child, MUI_NewObject(MUIC_Group,
			MUIA_Unicode, TRUE,
			MUIA_Background, MUII_GroupBack,
			MUIA_Frame, MUIV_Frame_Group,
			MUIA_FrameTitle, GetString(MSG_PREFS_PROGRAM_MAIN_WINDOW),
			MUIA_Group_Child, MUI_NewObject(MUIC_Group,
				MUIA_Group_Horiz, TRUE,
				MUIA_Group_Child, StringLabel(GetString(MSG_PREFS_PROGRAM_MAIN_WINDOW_CLOSING_LABEL), "\33r"),
				MUIA_Group_Child, MUI_NewObject(MUIC_Cycle,
					MUIA_Unicode, TRUE,
					MUIA_UserData, USD_PREFS_PROGRAM_MAIN_WINDOW_CLOSE_CYCLE,
					MUIA_ObjectID, USD_PREFS_PROGRAM_MAIN_WINDOW_CLOSE_CYCLE,
					MUIA_Cycle_Entries, mwm_closing_entries,
					MUIA_ShortHelp, GetString(MSG_PREFS_PROGRAM_MAIN_WINDOW_CLOSING_HELP),
				TAG_END),
			TAG_END),
			MUIA_Group_Child, MUI_NewObject(MUIC_Group,
				MUIA_Group_Horiz, TRUE,
				MUIA_Group_Child, StringLabel(GetString(MSG_PREFS_PROGRAM_MAIN_WINDOW_HIDE_TIME_LABEL), "\33r"),
				MUIA_Group_Child, NewObject(TimeSliderClass->mcc_Class, NULL,
					MUIA_UserData, USD_PREFS_PROGRAM_MAIN_WINDOW_HIDE_TIME_SLIDER,
					MUIA_ObjectID, USD_PREFS_PROGRAM_MAIN_WINDOW_HIDE_TIME_SLIDER,
					MUIA_Slider_Level, 0,
					MUIA_Slider_Max, 3600,
					TSA_OnlyCompleteValues, TRUE,
					MUIA_ShortHelp, GetString(MSG_PREFS_PROGRAM_MAIN_WINDOW_HIDE_TIME_HELP),
				TAG_END),
			TAG_END),
			MUIA_Group_Child, MUI_NewObject(MUIC_Group,
				MUIA_Group_Horiz, TRUE,
				MUIA_Group_Child, MUI_NewObject(MUIC_Image,
					MUIA_Unicode, TRUE,
					MUIA_ObjectID, USD_PREFS_PROGRAM_MAIN_WINDOW_SHOW_START,
					MUIA_UserData, USD_PREFS_PROGRAM_MAIN_WINDOW_SHOW_START,
					MUIA_Image_Spec, "6:15",
					MUIA_ShowSelState, FALSE,
					MUIA_Selected, TRUE,
					MUIA_InputMode, MUIV_InputMode_Toggle,
					MUIA_CycleChain, TRUE,
					MUIA_ShortHelp, GetString(MSG_PREFS_PROGRAM_MAIN_WINDOW_SHOW_HELP),
				TAG_END),
				MUIA_Group_Child, StringLabel(GetString(MSG_PREFS_PROGRAM_MAIN_WINDOW_SHOW), "\33l"),
				MUIA_Group_Child, EmptyRectangle(100),
			TAG_END),
			MUIA_Group_Child, (show_hide_but_check = MUI_NewObject(MUIC_Group,
				MUIA_Group_Horiz, TRUE,
				MUIA_Group_Child, MUI_NewObject(MUIC_Image,
					MUIA_Unicode, TRUE,
					MUIA_ObjectID, USD_PREFS_PROGRAM_MAIN_WINDOW_SHOW_HIDE_BUTTON,
					MUIA_UserData, USD_PREFS_PROGRAM_MAIN_WINDOW_SHOW_HIDE_BUTTON,
					MUIA_Image_Spec, "6:15",
					MUIA_ShowSelState, FALSE,
					MUIA_Selected, TRUE,
					MUIA_InputMode, MUIV_InputMode_Toggle,
					MUIA_CycleChain, TRUE,
					MUIA_ShortHelp, GetString(MSG_PREFS_PROGRAM_MAIN_WINDOW_SHOW_HIDE_BUTTON_HELP),
				TAG_END),
				MUIA_Group_Child, StringLabel(GetString(MSG_PREFS_PROGRAM_MAIN_WINDOW_SHOW_HIDE_BUTTON), "\33l"),
				MUIA_Group_Child, EmptyRectangle(100),
			TAG_END)),
		TAG_END),

		/* autoconnect */
		MUIA_Group_Child, MUI_NewObject(MUIC_Group,
			MUIA_Unicode, TRUE,
			MUIA_Background, MUII_GroupBack,
			MUIA_Frame, MUIV_Frame_Group,
			MUIA_UserData, USD_PREFS_PROGRAM_CONNECT_AUTO_GROUP,
			MUIA_FrameTitle, GetString(MSG_PREFS_PROGRAM_CONNECTION),
			MUIA_Group_Child, MUI_NewObject(MUIC_Group,
				MUIA_Group_Horiz, TRUE,
				MUIA_Group_Child, (auto_check = MUI_NewObject(MUIC_Image,
					MUIA_Unicode, TRUE,
					MUIA_ObjectID, USD_PREFS_PROGRAM_CONNECT_AUTO_CHECK,
					MUIA_UserData, USD_PREFS_PROGRAM_CONNECT_AUTO_CHECK,
					MUIA_Image_Spec, "6:15",
					MUIA_ShowSelState, FALSE,
					MUIA_Selected, FALSE,
					MUIA_InputMode, MUIV_InputMode_Toggle,
					MUIA_CycleChain, TRUE,
					MUIA_ShortHelp, GetString(MSG_PREFS_PROGRAM_CONNECT_AUTO_HELP),
				TAG_END)),
				MUIA_Group_Child, StringLabel(GetString(MSG_PREFS_PROGRAM_CONNECT_AUTO), "\33l"),
				MUIA_Group_Child, EmptyRectangle(100),
			TAG_END),
			MUIA_Group_Child, MUI_NewObject(MUIC_Group,
				MUIA_Group_Child, (auto_type = MUI_NewObject(MUIC_Cycle,
					MUIA_Unicode, TRUE,
					MUIA_Disabled, TRUE,
					MUIA_UserData, USD_PREFS_PROGRAM_CONNECT_AUTO_TYPE,
					MUIA_ObjectID, USD_PREFS_PROGRAM_CONNECT_AUTO_TYPE,
					MUIA_Cycle_Entries, auto_type_entries,
					MUIA_ShortHelp, GetString(MSG_PREFS_PROGRAM_CONNECT_AUTO_TYPE_HELP),
				TAG_END)),
			TAG_END),

			/* const auto-status group */
			MUIA_Group_Child, (const_group = MUI_NewObject(MUIC_Group,
				MUIA_Disabled, TRUE,
				MUIA_Group_Child, MUI_NewObject(MUIC_Group,
					MUIA_Group_Horiz, TRUE,
					MUIA_Group_Child, StringLabel(GetString(MSG_PREFS_PROGRAM_CONNECT_AUTO_STAT), "\33r"),
					MUIA_Group_Child, MUI_NewObject(MUIC_Cycle,
						MUIA_Unicode, TRUE,
						MUIA_UserData, USD_PREFS_PROGRAM_CONNECT_AUTO_CYCLE,
						MUIA_ObjectID, USD_PREFS_PROGRAM_CONNECT_AUTO_CYCLE,
						MUIA_Cycle_Entries, statuses,
						MUIA_ShortHelp, GetString(MSG_PREFS_PROGRAM_CONNECT_AUTO_STAT_HELP),
					TAG_END),
				TAG_END),
				MUIA_Group_Child, MUI_NewObject(MUIC_Group,
					MUIA_Group_Horiz, TRUE,
					MUIA_Group_Child, StringLabel(GetString(MSG_PREFS_PROGRAM_CONNECT_AUTO_DESC), "\33r"),
					MUIA_Group_Child, MUI_NewObject(MUIC_String,
						MUIA_Unicode, TRUE,
						MUIA_ObjectID, USD_PREFS_PROGRAM_CONNECT_AUTO_STRING,
						MUIA_UserData, USD_PREFS_PROGRAM_CONNECT_AUTO_STRING,
						MUIA_Frame, MUIV_Frame_String,
						MUIA_Background, MUII_StringBack,
						MUIA_CycleChain, TRUE,
						MUIA_ShortHelp, GetString(MSG_PREFS_PROGRAM_CONNECT_AUTO_DESC_HELP),
						MUIA_String_Contents, "KwaKwa rulez!",
					TAG_END),
				TAG_END),
			TAG_END)),
			/* const auto-status group end */

			/* last auto-status group */
			MUIA_Group_Child, (last_group = MUI_NewObject(MUIC_Group,
				MUIA_Disabled, TRUE,
				MUIA_Group_Child, MUI_NewObject(MUIC_Group,
					MUIA_Group_Horiz, TRUE,
					MUIA_Group_Child, StringLabel(GetString(MSG_PREFS_PROGRAM_CONNECT_AUTO_NOTAVAIL), "\33r"),
					MUIA_Group_Child, MUI_NewObject(MUIC_Cycle,
						MUIA_Unicode, TRUE,
						MUIA_UserData, USD_PREFS_PROGRAM_LASTNOTAVAIL_CYCLE,
						MUIA_ObjectID, USD_PREFS_PROGRAM_LASTNOTAVAIL_CYCLE,
						MUIA_Cycle_Entries, last_notavail_entries,
						MUIA_ShortHelp, GetString(MSG_PREFS_PROGRAM_CONNECT_AUTO_NOTAVAIL_HELP),
					TAG_END),
				TAG_END),
			TAG_END)),
			/* last auto-status group end */

		TAG_END),

		MUIA_Group_Child, MUI_NewObject(MUIC_Group,
			MUIA_Unicode, TRUE,
			MUIA_Background, MUII_GroupBack,
			MUIA_Frame, MUIV_Frame_Group,
			MUIA_FrameTitle, GetString(MSG_PREFS_PROGRAM_AUTOAWAY),
			MUIA_Group_Child, (away_slider = MUI_NewObject(MUIC_Group,
				MUIA_Group_Horiz, TRUE,
				MUIA_Group_Child, StringLabel(GetString(MSG_PREFS_PROGRAM_AUTOAWAY_SLIDER_LABEL), "\33r"),
				MUIA_Group_Child, NewObject(TimeSliderClass->mcc_Class, NULL,
					MUIA_UserData, USD_PREFS_PROGRAM_AUTOAWAY_SLIDER,
					MUIA_ObjectID, USD_PREFS_PROGRAM_AUTOAWAY_SLIDER,
					MUIA_Slider_Level, 0,
					MUIA_Slider_Max, 3600,
					TSA_OnlyCompleteValues, TRUE,
					MUIA_ShortHelp, GetString(MSG_PREFS_PROGRAM_AUTOAWAY_SLIDER_HELP),
				TAG_END),
			TAG_END)),

			MUIA_Group_Child, (away_group = MUI_NewObject(MUIC_Group,
				MUIA_ShowMe, FALSE,
				MUIA_Group_Child, MUI_NewObject(MUIC_Group,
					MUIA_Group_Horiz, TRUE,
					MUIA_Group_Child, StringLabel(GetString(MSG_PREFS_PROGRAM_AUTOAWAY_STAT), "\33r"),
					MUIA_Group_Child, MUI_NewObject(MUIC_Cycle,
						MUIA_Unicode, TRUE,
						MUIA_UserData, USD_PREFS_PROGRAM_AUTOAWAY_CYCLE,
						MUIA_ObjectID, USD_PREFS_PROGRAM_AUTOAWAY_CYCLE,
						MUIA_Cycle_Entries, statuses,
						MUIA_ShortHelp, GetString(MSG_PREFS_PROGRAM_AUTOAWAY_STAT_HELP),
					TAG_END),
				TAG_END),
				MUIA_Group_Child, MUI_NewObject(MUIC_Group,
					MUIA_Group_Horiz, TRUE,
					MUIA_Group_Child, StringLabel(GetString(MSG_PREFS_PROGRAM_AUTOAWAY_DESC), "\33r"),
					MUIA_Group_Child, MUI_NewObject(MUIC_String,
						MUIA_Unicode, TRUE,
						MUIA_ObjectID, USD_PREFS_PROGRAM_AUTOAWAY_STRING,
						MUIA_UserData, USD_PREFS_PROGRAM_AUTOAWAY_STRING,
						MUIA_Frame, MUIV_Frame_String,
						MUIA_Background, MUII_StringBack,
						MUIA_CycleChain, TRUE,
						MUIA_ShortHelp, GetString(MSG_PREFS_PROGRAM_AUTOAWAY_DESC_HELP),
					TAG_END),
				TAG_END),
			TAG_END)),
		TAG_END),
		MUIA_Group_Child, EmptyRectangle(100),
	TAG_END);

	DoMethod(auto_check, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, const_group, 3, MUIM_Set, MUIA_Disabled, MUIV_NotTriggerValue);
	DoMethod(auto_check, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, last_group, 3, MUIM_Set, MUIA_Disabled, MUIV_NotTriggerValue);
	DoMethod(auto_check, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, auto_type, 3, MUIM_Set, MUIA_Disabled, MUIV_NotTriggerValue);
	DoMethod(auto_type, MUIM_Notify, MUIA_Cycle_Active, 0, const_group, 3, MUIM_Set, MUIA_ShowMe, TRUE);
	DoMethod(auto_type, MUIM_Notify, MUIA_Cycle_Active, 1, const_group, 3, MUIM_Set, MUIA_ShowMe, FALSE);
	DoMethod(auto_type, MUIM_Notify, MUIA_Cycle_Active, 0, last_group, 3, MUIM_Set, MUIA_ShowMe, FALSE);
	DoMethod(auto_type, MUIM_Notify, MUIA_Cycle_Active, 1, last_group, 3, MUIM_Set, MUIA_ShowMe, TRUE);
	DoMethod(away_slider, MUIM_Notify, MUIA_Slider_Level, MUIV_EveryTime, away_group, 3, MUIM_Set, MUIA_ShowMe, MUIV_TriggerValue);

	return result;
}

static inline Object *CreateTalkWindowPrefs(VOID)
{
	Object *result, *pic_width_slider, *pic_width_onoff, *toolbar_onoff, *toolbar_spacer_size;
	Object *systemmsg_title, *systemmsg_title_bg, *systemmsg_title_width, *systemmsg_title_reverse;
	Object *systemmsg_title_bg_label, *systemmsg_title_width_label, *systemmsg_title_reverse_label;

	result = MUI_NewObject(MUIC_Group,
		MUIA_Group_Child, EmptyRectangle(100),
		MUIA_Group_Child, MUI_NewObject(MUIC_Group,
			MUIA_Group_Child, MUI_NewObject(MUIC_Group,
				MUIA_Unicode, TRUE,
				MUIA_Frame, MUIV_Frame_Group,
				MUIA_Background, MUII_GroupBack,
				MUIA_FrameTitle, GetString(MSG_PREFS_TALKWINDOW_USER),
				MUIA_Group_Columns, 2,
				MUIA_Group_Child, (ULONG)StringLabel(GetString(MSG_PREFS_TALKWINDOW_USER_USRNAME), "\33r"),
				MUIA_Group_Child, (ULONG)MUI_NewObject(MUIC_String,
					MUIA_Unicode, TRUE,
					MUIA_ObjectID, USD_PREFS_TW_USRNAME_STRING,
					MUIA_UserData, USD_PREFS_TW_USRNAME_STRING,
					MUIA_Frame, MUIV_Frame_String,
					MUIA_Background, MUII_StringBack,
					MUIA_CycleChain, TRUE,
					MUIA_String_AdvanceOnCR, TRUE,
					MUIA_String_Contents, (ULONG)GetString(MSG_PREFS_TALKWINDOW_USER_USRNAME_DEAFAULT),
					MUIA_ShortHelp, (ULONG)GetString(MSG_PREFS_TALKWINDOW_USER_USRNAME_HELP),
				TAG_END),
			TAG_END),

			MUIA_Group_Child, MUI_NewObject(MUIC_Group,
				MUIA_Group_Horiz, TRUE,

				MUIA_Group_Child, MUI_NewObject(MUIC_Group,
					MUIA_Unicode, TRUE,
					MUIA_Background, MUII_GroupBack,
					MUIA_Frame, MUIV_Frame_Group,
					MUIA_FrameTitle, GetString(MSG_PREFS_TALKWINDOW_COLORS),
					MUIA_Group_Columns, 2,

					MUIA_Group_Child, StringLabel(GetString(MSG_PREFS_TALKWINDOW_MYCOLOR), "\33r"),

					MUIA_Group_Child, MUI_NewObject(MUIC_Group,
						MUIA_Group_Horiz, TRUE,
						MUIA_Group_Child, MUI_NewObject(MUIC_Poppen,
							MUIA_Unicode, TRUE,
							MUIA_ObjectID, USD_PREFS_ML_MYCOLOR_POPPEN,
							MUIA_UserData, USD_PREFS_ML_MYCOLOR_POPPEN,
							MUIA_FixWidthTxt, "MM",
							MUIA_ShortHelp, GetString(MSG_PREFS_TALKWINDOW_MYCOLOR_HELP),
						TAG_END),
						MUIA_Group_Child, EmptyRectangle(200),
					TAG_END),

					MUIA_Group_Child, StringLabel(GetString(MSG_PREFS_TALKWINDOW_FRIENDCOLOR), "\33r"),

					MUIA_Group_Child, MUI_NewObject(MUIC_Group,
						MUIA_Group_Horiz, TRUE,
						MUIA_Group_Child, MUI_NewObject(MUIC_Poppen,
							MUIA_Unicode, TRUE,
							MUIA_ObjectID, USD_PREFS_ML_FRIENDCOLOR_POPPEN,
							MUIA_UserData, USD_PREFS_ML_FRIENDCOLOR_POPPEN,
							MUIA_FixWidthTxt, "MM",
							MUIA_ShortHelp, GetString(MSG_PREFS_TALKWINDOW_FRIENDCOLOR_HELP),
						TAG_END),
						MUIA_Group_Child, EmptyRectangle(200),
					TAG_END),

					MUIA_Group_Child, StringLabel(GetString(MSG_PREFS_TALKWINDOW_HEADLINE_COLOR), "\33r"),

					MUIA_Group_Child, MUI_NewObject(MUIC_Group,
						MUIA_Group_Horiz, TRUE,
						MUIA_Group_Child, MUI_NewObject(MUIC_Poppen,
							MUIA_Unicode, TRUE,
							MUIA_ObjectID, USD_PREFS_TW_HEADLINE_COLOR,
							MUIA_UserData, USD_PREFS_TW_HEADLINE_COLOR,
							MUIA_FixWidthTxt, "MM",
							MUIA_ShortHelp, GetString(MSG_PREFS_TALKWINDOW_HEADLINE_COLOR_HELP),
						TAG_END),
						MUIA_Group_Child, EmptyRectangle(200),
					TAG_END),

					MUIA_Group_Child, StringLabel(GetString(MSG_PREFS_TALKWINDOW_SYSTEMMSG_COLOR), "\33r"),

					MUIA_Group_Child, MUI_NewObject(MUIC_Group,
						MUIA_Group_Horiz, TRUE,
						MUIA_Group_Child, MUI_NewObject(MUIC_Poppen,
							MUIA_Unicode, TRUE,
							MUIA_ObjectID, USD_PREFS_TW_SYSTEMMSG_COLOR,
							MUIA_UserData, USD_PREFS_TW_SYSTEMMSG_COLOR,
							MUIA_FixWidthTxt, "MM",
							MUIA_ShortHelp, GetString(MSG_PREFS_TALKWINDOW_SYSTEMMSG_COLOR_HELP),
						TAG_END),
						MUIA_Group_Child, EmptyRectangle(200),
					TAG_END),

					MUIA_Group_Child, MUI_NewObject(MUIC_Group,
						MUIA_Group_Horiz, TRUE,
						MUIA_Group_Child, EmptyRectangle(200),
					TAG_END),

				TAG_END),

				MUIA_Group_Child, MUI_NewObject(MUIC_Group,
					MUIA_Unicode, TRUE,
					MUIA_Background, MUII_GroupBack,
					MUIA_Frame, MUIV_Frame_Group,
					MUIA_FrameTitle, GetString(MSG_PREFS_TALKWINDOW_COLORS_OLD),

					MUIA_Group_Child, MUI_NewObject(MUIC_Group,

						MUIA_Group_Columns, 2,
						MUIA_Group_Child, StringLabel(GetString(MSG_PREFS_TALKWINDOW_MYCOLOR_OLD), "\33r"),

						MUIA_Group_Child, MUI_NewObject(MUIC_Group,
							MUIA_Group_Horiz, TRUE,
							MUIA_Group_Child, MUI_NewObject(MUIC_Poppen,
								MUIA_Unicode, TRUE,
								MUIA_ObjectID, USD_PREFS_ML_MYCOLOR_OLD_POPPEN,
								MUIA_UserData, USD_PREFS_ML_MYCOLOR_OLD_POPPEN,
								MUIA_FixWidthTxt, "MM",
								MUIA_ShortHelp, GetString(MSG_PREFS_TALKWINDOW_MYCOLOR_OLD_HELP),
							TAG_END),
							MUIA_Group_Child, EmptyRectangle(200),
						TAG_END),

						MUIA_Group_Child, StringLabel(GetString(MSG_PREFS_TALKWINDOW_FRIENDCOLOR_OLD), "\33r"),

						MUIA_Group_Child, MUI_NewObject(MUIC_Group,
							MUIA_Group_Horiz, TRUE,
							MUIA_Group_Child, MUI_NewObject(MUIC_Poppen,
								MUIA_Unicode, TRUE,
								MUIA_ObjectID, USD_PREFS_ML_FRIENDCOLOR_OLD_POPPEN,
								MUIA_UserData, USD_PREFS_ML_FRIENDCOLOR_OLD_POPPEN,
								MUIA_FixWidthTxt, "MM",
								MUIA_ShortHelp, GetString(MSG_PREFS_TALKWINDOW_FRIENDCOLOR_OLD_HELP),
							TAG_END),
							MUIA_Group_Child, EmptyRectangle(200),
						TAG_END),

						MUIA_Group_Child, StringLabel(GetString(MSG_PREFS_TALKWINDOW_HEADLINE_OLD), "\33r"),

						MUIA_Group_Child, MUI_NewObject(MUIC_Group,
							MUIA_Group_Horiz, TRUE,
							MUIA_Group_Child, MUI_NewObject(MUIC_Poppen,
								MUIA_Unicode, TRUE,
								MUIA_ObjectID, USD_PREFS_TW_HEADLINE_OLD_COLOR,
								MUIA_UserData, USD_PREFS_TW_HEADLINE_OLD_COLOR,
								MUIA_FixWidthTxt, "MM",
								MUIA_ShortHelp, GetString(MSG_PREFS_TALKWINDOW_HEADLINE_OLD_HELP),
							TAG_END),
							MUIA_Group_Child, EmptyRectangle(200),
						TAG_END),

						MUIA_Group_Child, StringLabel(GetString(MSG_PREFS_TALKWINDOW_SYSTEMMSG_OLD), "\33r"),

						MUIA_Group_Child, MUI_NewObject(MUIC_Group,
							MUIA_Group_Horiz, TRUE,
							MUIA_Group_Child, MUI_NewObject(MUIC_Poppen,
								MUIA_Unicode, TRUE,
								MUIA_ObjectID, USD_PREFS_TW_SYSTEMMSG_OLD_COLOR,
								MUIA_UserData, USD_PREFS_TW_SYSTEMMSG_OLD_COLOR,
								MUIA_FixWidthTxt, "MM",
								MUIA_ShortHelp, GetString(MSG_PREFS_TALKWINDOW_SYSTEMMSG_OLD_HELP),
							TAG_END),
							MUIA_Group_Child, EmptyRectangle(200),
						TAG_END),

					TAG_END),

					MUIA_Group_Child, MUI_NewObject(MUIC_Group, /* old messages transparency */
						MUIA_Group_Horiz, TRUE,
						MUIA_Group_Child, StringLabel(GetString(MSG_PREFS_TALKWINDOW_OLD_TRANSPARENCY), "\33l"),
						MUIA_Group_Child, NewObject(PercentageSliderClass->mcc_Class, NULL,
							MUIA_ObjectID, USD_PREFS_OLD_MESSAGES_TRANSPARENCY,
							MUIA_UserData, USD_PREFS_OLD_MESSAGES_TRANSPARENCY,
							MUIA_ShortHelp, GetString(MSG_PREFS_TALKWINDOW_OLD_TRANSPARENCY_HELP),
							MUIA_Slider_Min, 0,
							MUIA_Slider_Level, 25,
						TAG_END),
					TAG_END),

				TAG_END),

			TAG_END),

			MUIA_Group_Child, MUI_NewObject(MUIC_Group,
				MUIA_Group_Horiz, TRUE,

				/* message headline */
				MUIA_Group_Child, MUI_NewObject(MUIC_Group,
					MUIA_Unicode, TRUE,
					MUIA_Frame, MUIV_Frame_Group,
					MUIA_Background, MUII_GroupBack,
					MUIA_FrameTitle, GetString(MSG_PREFS_TALKWINDOW_HEADLINE),
					MUIA_Group_Columns, 2,

					MUIA_Group_Child, MUI_NewObject(MUIC_Image,
						MUIA_Unicode, TRUE,
						MUIA_ObjectID, USD_PREFS_TW_HEADLINE_BOLD,
						MUIA_UserData, USD_PREFS_TW_HEADLINE_BOLD,
						MUIA_Image_Spec, "6:15",
						MUIA_ShowSelState, FALSE,
						MUIA_Selected, TRUE,
						MUIA_InputMode, MUIV_InputMode_Toggle,
						MUIA_CycleChain, TRUE,
						MUIA_ShortHelp, GetString(MSG_PREFS_TALKWINDOW_HEADLINE_BOLD_HELP),
					TAG_END),
					MUIA_Group_Child, StringLabel(GetString(MSG_PREFS_TALKWINDOW_HEADLINE_BOLD), "\33l"),

					MUIA_Group_Child, MUI_NewObject(MUIC_Image,
						MUIA_Unicode, TRUE,
						MUIA_ObjectID, USD_PREFS_TW_HEADLINE_ITALIC,
						MUIA_UserData, USD_PREFS_TW_HEADLINE_ITALIC,
						MUIA_Image_Spec, "6:15",
						MUIA_ShowSelState, FALSE,
						MUIA_Selected, FALSE,
						MUIA_InputMode, MUIV_InputMode_Toggle,
						MUIA_CycleChain, TRUE,
						MUIA_ShortHelp, GetString(MSG_PREFS_TALKWINDOW_HEADLINE_ITALICS_HELP),
					TAG_END),
					MUIA_Group_Child, StringLabel(GetString(MSG_PREFS_TALKWINDOW_HEADLINE_ITALICS), "\33l"),

					MUIA_Group_Child, MUI_NewObject(MUIC_Image,
						MUIA_Unicode, TRUE,
						MUIA_ObjectID, USD_PREFS_TW_HEADLINE_BACKGROUND,
						MUIA_UserData, USD_PREFS_TW_HEADLINE_BACKGROUND,
						MUIA_Image_Spec, "6:15",
						MUIA_ShowSelState, FALSE,
						MUIA_Selected, FALSE,
						MUIA_InputMode, MUIV_InputMode_Toggle,
						MUIA_CycleChain, TRUE,
						MUIA_ShortHelp, GetString(MSG_PREFS_TALKWINDOW_HEADLINE_BACKGROUND_HELP),
					TAG_END),
					MUIA_Group_Child, StringLabel(GetString(MSG_PREFS_TALKWINDOW_HEADLINE_BACKGROUND), "\33l"),

					MUIA_Group_Child, MUI_NewObject(MUIC_Image,
						MUIA_Unicode, TRUE,
						MUIA_ObjectID, USD_PREFS_TW_HEADLINE_MAXWIDTH,
						MUIA_UserData, USD_PREFS_TW_HEADLINE_MAXWIDTH,
						MUIA_Image_Spec, "6:15",
						MUIA_ShowSelState, FALSE,
						MUIA_Selected, FALSE,
						MUIA_InputMode, MUIV_InputMode_Toggle,
						MUIA_CycleChain, TRUE,
						MUIA_ShortHelp, GetString(MSG_PREFS_TALKWINDOW_HEADLINE_MAXWIDTH_HELP),
					TAG_END),
					MUIA_Group_Child, StringLabel(GetString(MSG_PREFS_TALKWINDOW_HEADLINE_MAXWIDTH), "\33l"),

					MUIA_Group_Child, MUI_NewObject(MUIC_Image,
						MUIA_Unicode, TRUE,
						MUIA_ObjectID, USD_PREFS_TW_HEADLINE_REVERSE,
						MUIA_UserData, USD_PREFS_TW_HEADLINE_REVERSE,
						MUIA_Image_Spec, "6:15",
						MUIA_ShowSelState, FALSE,
						MUIA_Selected, FALSE,
						MUIA_InputMode, MUIV_InputMode_Toggle,
						MUIA_CycleChain, TRUE,
						MUIA_ShortHelp, GetString(MSG_PREFS_TALKWINDOW_HEADLINE_REVERSE_HELP),
					TAG_END),
					MUIA_Group_Child, StringLabel(GetString(MSG_PREFS_TALKWINDOW_HEADLINE_REVERSE), "\33l"),

					MUIA_Group_Child, MUI_NewObject(MUIC_Image,
						MUIA_Unicode, TRUE,
						MUIA_ObjectID, USD_PREFS_TW_HEADLINE_INSERT_NEWLINE,
						MUIA_UserData, USD_PREFS_TW_HEADLINE_INSERT_NEWLINE,
						MUIA_Image_Spec, "6:15",
						MUIA_ShowSelState, FALSE,
						MUIA_Selected, TRUE,
						MUIA_InputMode, MUIV_InputMode_Toggle,
						MUIA_CycleChain, TRUE,
						MUIA_ShortHelp, GetString(MSG_PREFS_TALKWINDOW_HEADLINE_ADD_NEWLINE_HELP),
					TAG_END),
					MUIA_Group_Child, StringLabel(GetString(MSG_PREFS_TALKWINDOW_HEADLINE_ADD_NEWLINE), "\33l"),

					MUIA_Group_Child, EmptyRectangle(100),
					MUIA_Group_Child, EmptyRectangle(100),


				TAG_END),

				/* system messages */
				MUIA_Group_Child, MUI_NewObject(MUIC_Group,
					MUIA_Unicode, TRUE,
					MUIA_Frame, MUIV_Frame_Group,
					MUIA_Background, MUII_GroupBack,
					MUIA_FrameTitle, GetString(MSG_PREFS_TALKWINDOW_SYSTEMMSG),
					MUIA_Group_Columns, 2,

					MUIA_Group_Child, MUI_NewObject(MUIC_Image,
						MUIA_Unicode, TRUE,
						MUIA_ObjectID, USD_PREFS_TW_SYSTEMMSG_BOLD,
						MUIA_UserData, USD_PREFS_TW_SYSTEMMSG_BOLD,
						MUIA_Image_Spec, "6:15",
						MUIA_ShowSelState, FALSE,
						MUIA_Selected, FALSE,
						MUIA_InputMode, MUIV_InputMode_Toggle,
						MUIA_CycleChain, TRUE,
						MUIA_ShortHelp, GetString(MSG_PREFS_TALKWINDOW_SYSTEMMSG_BOLD_HELP),
					TAG_END),
					MUIA_Group_Child, StringLabel(GetString(MSG_PREFS_TALKWINDOW_SYSTEMMSG_BOLD), "\33l"),

					MUIA_Group_Child, MUI_NewObject(MUIC_Image,
						MUIA_Unicode, TRUE,
						MUIA_ObjectID, USD_PREFS_TW_SYSTEMMSG_ITALIC,
						MUIA_UserData, USD_PREFS_TW_SYSTEMMSG_ITALIC,
						MUIA_Image_Spec, "6:15",
						MUIA_ShowSelState, FALSE,
						MUIA_Selected, TRUE,
						MUIA_InputMode, MUIV_InputMode_Toggle,
						MUIA_CycleChain, TRUE,
						MUIA_ShortHelp, GetString(MSG_PREFS_TALKWINDOW_SYSTEMMSG_ITALICS_HELP),
					TAG_END),
					MUIA_Group_Child, StringLabel(GetString(MSG_PREFS_TALKWINDOW_SYSTEMMSG_ITALICS), "\33l"),

					MUIA_Group_Child, (systemmsg_title = MUI_NewObject(MUIC_Image,
						MUIA_Unicode, TRUE,
						MUIA_ObjectID, USD_PREFS_TW_SYSTEMMSG_HEADLINE,
						MUIA_UserData, USD_PREFS_TW_SYSTEMMSG_HEADLINE,
						MUIA_Image_Spec, "6:15",
						MUIA_ShowSelState, FALSE,
						MUIA_Selected, TRUE,
						MUIA_InputMode, MUIV_InputMode_Toggle,
						MUIA_CycleChain, TRUE,
						MUIA_ShortHelp, GetString(MSG_PREFS_TALKWINDOW_SYSTEMMSG_ADD_HEADLINE_HELP),
					TAG_END)),
					MUIA_Group_Child, StringLabel(GetString(MSG_PREFS_TALKWINDOW_SYSTEMMSG_ADD_HEADLINE), "\33l"),

					MUIA_Group_Child, (systemmsg_title_bg = MUI_NewObject(MUIC_Image,
						MUIA_Unicode, TRUE,
						MUIA_ObjectID, USD_PREFS_TW_SYSTEMMSG_BACKGROUND,
						MUIA_UserData, USD_PREFS_TW_SYSTEMMSG_BACKGROUND,
						MUIA_Image_Spec, "6:15",
						MUIA_ShowSelState, FALSE,
						MUIA_Selected, FALSE,
						MUIA_InputMode, MUIV_InputMode_Toggle,
						MUIA_CycleChain, TRUE,
						MUIA_ShortHelp, GetString(MSG_PREFS_TALKWINDOW_SYSTEMMSG_BACKGROUND_HELP),
					TAG_END)),
					MUIA_Group_Child, (systemmsg_title_bg_label = StringLabel(GetString(MSG_PREFS_TALKWINDOW_SYSTEMMSG_BACKGROUND), "\33l")),

					MUIA_Group_Child, (systemmsg_title_width = MUI_NewObject(MUIC_Image,
						MUIA_Unicode, TRUE,
						MUIA_ObjectID, USD_PREFS_TW_SYSTEMMSG_MAXWIDTH,
						MUIA_UserData, USD_PREFS_TW_SYSTEMMSG_MAXWIDTH,
						MUIA_Image_Spec, "6:15",
						MUIA_ShowSelState, FALSE,
						MUIA_Selected, FALSE,
						MUIA_InputMode, MUIV_InputMode_Toggle,
						MUIA_CycleChain, TRUE,
						MUIA_ShortHelp, GetString(MSG_PREFS_TALKWINDOW_SYSTEMMSG_MAXWIDTH_HELP),
					TAG_END)),
					MUIA_Group_Child, (systemmsg_title_width_label = StringLabel(GetString(MSG_PREFS_TALKWINDOW_SYSTEMMSG_MAXWIDTH), "\33l")),

					MUIA_Group_Child, (systemmsg_title_reverse = MUI_NewObject(MUIC_Image,
						MUIA_Unicode, TRUE,
						MUIA_ObjectID, USD_PREFS_TW_SYSTEMMSG_HEADLINE_REVERSE,
						MUIA_UserData, USD_PREFS_TW_SYSTEMMSG_HEADLINE_REVERSE,
						MUIA_Image_Spec, "6:15",
						MUIA_ShowSelState, FALSE,
						MUIA_Selected, FALSE,
						MUIA_InputMode, MUIV_InputMode_Toggle,
						MUIA_CycleChain, TRUE,
						MUIA_ShortHelp, GetString(MSG_PREFS_TALKWINDOW_SYSTEMMSG_HEADLINE_REVERSE_HELP),
					TAG_END)),
					MUIA_Group_Child, (systemmsg_title_reverse_label = StringLabel(GetString(MSG_PREFS_TALKWINDOW_SYSTEMMSG_HEADLINE_REVERSE), "\33l")),

					MUIA_Group_Child, EmptyRectangle(100),
					MUIA_Group_Child, EmptyRectangle(100),

				TAG_END),
			TAG_END),

			/* apperance */
			MUIA_Group_Child, MUI_NewObject(MUIC_Group,
				MUIA_Unicode, TRUE,
				MUIA_Frame, MUIV_Frame_Group,
				MUIA_Background, MUII_GroupBack,
				MUIA_FrameTitle, GetString(MSG_PREFS_TALKWINDOW_APPERANCE),
				MUIA_Group_Child, MUI_NewObject(MUIC_Group,
					MUIA_Group_Horiz, TRUE,
					MUIA_Group_Child, StringLabel(GetString(MSG_PREFS_TALKWINDOW_SELECTION_COLOR), "\33r"),
					MUIA_Group_Child, MUI_NewObject(MUIC_Poppen,
						MUIA_Unicode, TRUE,
						MUIA_ObjectID, USD_PREFS_TW_SELECTION_COLOR,
						MUIA_UserData, USD_PREFS_TW_SELECTION_COLOR,
						MUIA_FixWidthTxt, "MM",
						MUIA_ShortHelp, GetString(MSG_PREFS_TALKWINDOW_SELECTION_COLOR_HELP),
					TAG_END),
					MUIA_Group_Child, EmptyRectangle(200),
				TAG_END),
				MUIA_Group_Child, MUI_NewObject(MUIC_Group,
					MUIA_Group_Horiz, TRUE,
					MUIA_Group_Child, MUI_NewObject(MUIC_Image,
						MUIA_Unicode, TRUE,
						MUIA_ObjectID, USD_PREFS_TW_TABTITLE_IMAGE_ONOFF,
						MUIA_UserData, USD_PREFS_TW_TABTITLE_IMAGE_ONOFF,
						MUIA_Image_Spec, "6:15",
						MUIA_ShowSelState, FALSE,
						MUIA_Selected, FALSE,
						MUIA_InputMode, MUIV_InputMode_Toggle,
						MUIA_CycleChain, TRUE,
						MUIA_ShortHelp, GetString(MSG_PREFS_TALKWINDOW_TABTITLE_IMAGE_ONOFF_HELP),
					TAG_END),
					MUIA_Group_Child, StringLabel(GetString(MSG_PREFS_TALKWINDOW_TABTITLE_IMAGE_ONOFF), "\33l"),
					MUIA_Group_Child, EmptyRectangle(100),
				TAG_END),
				MUIA_Group_Child, MUI_NewObject(MUIC_Group,
					MUIA_Group_Horiz, TRUE,
					MUIA_Group_Child, MUI_NewObject(MUIC_Image,
						MUIA_Unicode, TRUE,
						MUIA_ObjectID, USD_PREFS_TW_CONTACTINFOBLOCK_ONOFF,
						MUIA_UserData, USD_PREFS_TW_CONTACTINFOBLOCK_ONOFF,
						MUIA_Image_Spec, "6:15",
						MUIA_ShowSelState, FALSE,
						MUIA_Selected, TRUE,
						MUIA_InputMode, MUIV_InputMode_Toggle,
						MUIA_CycleChain, TRUE,
						MUIA_ShortHelp, GetString(MSG_PREFS_TALKWINDOW_CONTACTINFOBLOCK_ON_OFF_HELP),
					TAG_END),
					MUIA_Group_Child, StringLabel(GetString(MSG_PREFS_TALKWINDOW_CONTACTINFOBLOCK_ON_OFF), "\33l"),
					MUIA_Group_Child, EmptyRectangle(100),
				TAG_END),
				MUIA_Group_Child, MUI_NewObject(MUIC_Group,
					MUIA_Group_Horiz, TRUE,
					MUIA_Group_Child, (toolbar_onoff = MUI_NewObject(MUIC_Image,
						MUIA_Unicode, TRUE,
						MUIA_ObjectID, USD_PREFS_TW_TOOLBAR_ONOFF,
						MUIA_UserData, USD_PREFS_TW_TOOLBAR_ONOFF,
						MUIA_Image_Spec, "6:15",
						MUIA_ShowSelState, FALSE,
						MUIA_Selected, TRUE,
						MUIA_InputMode, MUIV_InputMode_Toggle,
						MUIA_CycleChain, TRUE,
						MUIA_ShortHelp, GetString(MSG_PREFS_TALKWINDOW_TOOLBAR_ON_OFF_HELP),
					TAG_END)),
					MUIA_Group_Child, StringLabel(GetString(MSG_PREFS_TALKWINDOW_TOOLBAR_ON_OFF), "\33l"),
					MUIA_Group_Child, EmptyRectangle(100),
				TAG_END),
				MUIA_Group_Child, MUI_NewObject(MUIC_Group,
					MUIA_Group_Horiz, TRUE,
					MUIA_Group_Child, StringLabel(GetString(MSG_PREFS_TALKWINDOW_TOOLBAR_SPACE), "\33l"),
					MUIA_Group_Child, (toolbar_spacer_size = NewObject(MinMaxSliderClass->mcc_Class, NULL,
						MUIA_ObjectID, USD_PREFS_TW_TOOLBAR_SPACE_SIZE,
						MUIA_UserData, USD_PREFS_TW_TOOLBAR_SPACE_SIZE,
						MUIA_Slider_Horiz, TRUE,
						MUIA_Slider_Level, 0,
						MUIA_Slider_Max, 100,
						MUIA_Slider_Min, 0,
						MMSA_Unit, GetString(MSG_PIXEL_SLIDER_UNIT),
						MUIA_ShortHelp, GetString(MSG_PREFS_TALKWINDOW_TOOLBAR_SPACE_HELP),
					TAG_END)),
				TAG_END),
				MUIA_Group_Child, MUI_NewObject(MUIC_Group,
					MUIA_Group_Horiz, TRUE,
					MUIA_Group_Child, (pic_width_onoff = MUI_NewObject(MUIC_Image,
						MUIA_Unicode, TRUE,
						MUIA_ObjectID, USD_PREFS_TW_PICTURES_ONOFF,
						MUIA_UserData, USD_PREFS_TW_PICTURES_ONOFF,
						MUIA_Image_Spec, "6:15",
						MUIA_ShowSelState, FALSE,
						MUIA_Selected, TRUE,
						MUIA_InputMode, MUIV_InputMode_Toggle,
						MUIA_CycleChain, TRUE,
						MUIA_ShortHelp, GetString(MSG_PREFS_TALKWINDOW_PICTURES_ON_OFF_HELP),
					TAG_END)),
					MUIA_Group_Child, StringLabel(GetString(MSG_PREFS_TALKWINDOW_PICTURES_ON_OFF), "\33l"),
					MUIA_Group_Child, EmptyRectangle(100),
				TAG_END),
				MUIA_Group_Child, MUI_NewObject(MUIC_Group,
					MUIA_Group_Horiz, TRUE,
					MUIA_Group_Child, StringLabel(GetString(MSG_PREFS_TALKWINDOW_MAX_WIDTH), "\33l"),
					MUIA_Group_Child, (pic_width_slider = NewObject(MinMaxSliderClass->mcc_Class, NULL,
						MUIA_ObjectID, USD_PREFS_TW_PICTURES_MAXWIDTH,
						MUIA_UserData, USD_PREFS_TW_PICTURES_MAXWIDTH,
						MUIA_Slider_Horiz, TRUE,
						MUIA_Slider_Level, 150,
						MUIA_Slider_Max, 801,
						MUIA_Slider_Min, 20,
						MMSA_MaxText, GetString(MSG_PREFS_TALKWINDOW_MAX_WIDTH_UNLIMITED),
						MMSA_Unit, GetString(MSG_PIXEL_SLIDER_UNIT),
						MUIA_ShortHelp, GetString(MSG_PREFS_TALKWINDOW_MAX_WIDTH_HELP),
					TAG_END)),
				TAG_END),
			TAG_END),

		TAG_END),
		MUIA_Group_Child, EmptyRectangle(100),
	TAG_END);

	DoMethod(pic_width_onoff, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, pic_width_slider, 3,
	 MUIM_Set, MUIA_Disabled, MUIV_NotTriggerValue);

	DoMethod(toolbar_onoff, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, toolbar_spacer_size, 3,
	 MUIM_Set, MUIA_Disabled, MUIV_NotTriggerValue);

	DoMethod(systemmsg_title, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, systemmsg_title_bg, 3,
	 MUIM_Set, MUIA_Disabled, MUIV_NotTriggerValue);

	DoMethod(systemmsg_title, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, systemmsg_title_width, 3,
	 MUIM_Set, MUIA_Disabled, MUIV_NotTriggerValue);

	DoMethod(systemmsg_title, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, systemmsg_title_reverse, 3,
	 MUIM_Set, MUIA_Disabled, MUIV_NotTriggerValue);

	DoMethod(systemmsg_title, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, systemmsg_title_bg_label, 3,
	 MUIM_Set, MUIA_Disabled, MUIV_NotTriggerValue);

	DoMethod(systemmsg_title, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, systemmsg_title_width_label, 3,
	 MUIM_Set, MUIA_Disabled, MUIV_NotTriggerValue);

	DoMethod(systemmsg_title, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, systemmsg_title_reverse_label, 3,
	 MUIM_Set, MUIA_Disabled, MUIV_NotTriggerValue);

	return result;
}

static inline Object *CreateLogsPrefs(VOID)
{
	Object *result, *vac_button;
	Object *history_on, *history_group;
	Object *end_time, *end_cycle;
	static CONST_STRPTR end_conversation_entries[4];

	end_conversation_entries[0] = GetString(MSG_PREFS_LOGS_HISTORY_CONVERSATION_END_MODE_1);
	end_conversation_entries[1] = GetString(MSG_PREFS_LOGS_HISTORY_CONVERSATION_END_MODE_2);
	end_conversation_entries[2] = GetString(MSG_PREFS_LOGS_HISTORY_CONVERSATION_END_MODE_3);
	end_conversation_entries[3] = NULL;

	result = MUI_NewObject(MUIC_Group,
		MUIA_Group_Child, EmptyRectangle(100),

		MUIA_Group_Child, MUI_NewObject(MUIC_Group,
			MUIA_Unicode, TRUE,
			MUIA_Background, MUII_GroupBack,
			MUIA_Frame, MUIV_Frame_Group,
			MUIA_FrameTitle, GetString(MSG_PREFS_LOGS),

			MUIA_Group_Child, MUI_NewObject(MUIC_Group,
				MUIA_Group_Horiz, TRUE,
				MUIA_Group_Child, MUI_NewObject(MUIC_Image,
					MUIA_Unicode, TRUE,
					MUIA_ObjectID, USD_PREFS_LOGS_ONOFF_CHECK,
					MUIA_UserData, USD_PREFS_LOGS_ONOFF_CHECK,
					MUIA_Image_Spec, "6:15",
					MUIA_ShowSelState, FALSE,
					MUIA_Selected, TRUE,
					MUIA_InputMode, MUIV_InputMode_Toggle,
					MUIA_CycleChain, TRUE,
					MUIA_ShortHelp, GetString(MSG_PREFS_LOGS_ONOFF_HELP),
				TAG_END),
				MUIA_Group_Child, StringLabel(GetString(MSG_PREFS_LOGS_ONOFF), "\33l"),
				MUIA_Group_Child, EmptyRectangle(100),
			TAG_END),

			MUIA_Group_Child, MUI_NewObject(MUIC_Group,
				MUIA_Group_Horiz, TRUE,
				MUIA_Group_Child, (history_on = MUI_NewObject(MUIC_Image,
					MUIA_Unicode, TRUE,
					MUIA_ObjectID, USD_PREFS_HISTORY_ONOFF_CHECK,
					MUIA_UserData, USD_PREFS_HISTORY_ONOFF_CHECK,
					MUIA_Image_Spec, "6:15",
					MUIA_ShowSelState, FALSE,
					MUIA_Selected, FALSE,
					MUIA_InputMode, MUIV_InputMode_Toggle,
					MUIA_CycleChain, TRUE,
					MUIA_ShortHelp, GetString(MSG_PREFS_LOGS_HISTORY_ONOFF_HELP),
				TAG_END)),
				MUIA_Group_Child, StringLabel(GetString(MSG_PREFS_LOGS_HISTORY_ONOFF), "\33l"),
				MUIA_Group_Child, EmptyRectangle(100),
			TAG_END),

		TAG_END),

		MUIA_Group_Child, (history_group = MUI_NewObject(MUIC_Group,
			MUIA_Unicode, TRUE,
			MUIA_Background, MUII_GroupBack,
			MUIA_Frame, MUIV_Frame_Group,
			MUIA_FrameTitle, GetString(MSG_PREFS_HISTORY),
			MUIA_Disabled, TRUE,
			MUIA_Group_Child, MUI_NewObject(MUIC_Group,
				MUIA_Group_Horiz, TRUE,
				MUIA_Group_Child, MUI_NewObject(MUIC_Image,
					MUIA_Unicode, TRUE,
					MUIA_ObjectID, USD_PREFS_HISTORY_SAVE_SYSTEMMSGS,
					MUIA_UserData, USD_PREFS_HISTORY_SAVE_SYSTEMMSGS,
					MUIA_Image_Spec, "6:15",
					MUIA_ShowSelState, FALSE,
					MUIA_Selected, TRUE,
					MUIA_InputMode, MUIV_InputMode_Toggle,
					MUIA_CycleChain, TRUE,
					MUIA_ShortHelp, GetString(MSG_PREFS_LOGS_HISTORY_SAVE_SYSTEMMSGS_HELP),
				TAG_END),
				MUIA_Group_Child, StringLabel(GetString(MSG_PREFS_LOGS_HISTORY_SAVE_SYSTEMMSGS), "\33l"),
				MUIA_Group_Child, EmptyRectangle(100),
			TAG_END),
			MUIA_Group_Child, MUI_NewObject(MUIC_Group,
				MUIA_Group_Horiz, TRUE,
				MUIA_Group_Child, StringLabel(GetString(MSG_PREFS_LOGS_HISTORY_CONVERSATION_END_MODE), "\33l"),
				MUIA_Group_Child, (end_cycle = MUI_NewObject(MUIC_Cycle,
					MUIA_Unicode, TRUE,
					MUIA_ObjectID, USD_PREFS_HISTORY_CONVERSATION_END_MODE,
					MUIA_UserData, USD_PREFS_HISTORY_CONVERSATION_END_MODE,
					MUIA_ShortHelp, GetString(MSG_PREFS_LOGS_HISTORY_CONVERSATION_END_MODE_HELP),
					MUIA_CycleChain, TRUE,
					MUIA_Cycle_Entries, end_conversation_entries,
				TAG_END)),
				MUIA_Group_Child, (end_time = NewObject(TimeSliderClass->mcc_Class, NULL,
					MUIA_ObjectID, USD_PREFS_HISTORY_CONVERSATION_END_TIME,
					MUIA_UserData, USD_PREFS_HISTORY_CONVERSATION_END_TIME,
					MUIA_ShowMe, FALSE,
					MUIA_Slider_Min, 60,
					TSA_OnlyCompleteValues, TRUE,
				TAG_END)),
			TAG_END),
			MUIA_Group_Child, MUI_NewObject(MUIC_Group,
				MUIA_Group_Horiz, TRUE,
				MUIA_Group_Child, StringLabel(GetString(MSG_PREFS_LOGS_HISTORY_LOAD_OLD), "\33l"),
				MUIA_Group_Child, NewObject(TimeSliderClass->mcc_Class, NULL,
					MUIA_ObjectID, USD_PREFS_HISTORY_LOAD_OLD_TIME,
					MUIA_UserData, USD_PREFS_HISTORY_LOAD_OLD_TIME,
					MUIA_Slider_Level, 0,
					TSA_OnlyCompleteValues, TRUE,
					TSA_MinText, NULL,
				TAG_END),
			TAG_END),
			MUIA_Group_Child, MUI_NewObject(MUIC_Group,
				MUIA_Group_Horiz, TRUE,
				MUIA_Group_Child, StringLabel(GetString(MSG_PREFS_LOGS_HISTORY_LOAD_OLD_2), "\33l"),
				MUIA_Group_Child, NewObject(MinMaxSliderClass->mcc_Class, NULL,
					MUIA_ObjectID, USD_PREFS_HISTORY_LOAD_OLD_NO,
					MUIA_UserData, USD_PREFS_HISTORY_LOAD_OLD_NO,
					MUIA_Slider_Level, 0,
					MMSA_MinText, GetString(MSG_PREFS_LOGS_HISTORY_LOAD_OLD_MIN),
					MMSA_MaxText, GetString(MSG_PREFS_LOGS_HISTORY_LOAD_OLD_MAX),
					MMSA_Unit, GetString(MSG_PREFS_LOGS_HISTORY_LOAD_OLD_UNIT),
				TAG_END),
			TAG_END),
			MUIA_Group_Child, MUI_NewObject(MUIC_Group,
				MUIA_Group_Horiz, TRUE,
				MUIA_Group_Child, EmptyRectangle(100),
				MUIA_Group_Child, (vac_button = NormalButton("Vacuum", 'V', 0, 100)),
				MUIA_Group_Child, EmptyRectangle(100),
				MUIA_ShowMe, FALSE,
			TAG_END),
		TAG_END)),
		MUIA_Group_Child, EmptyRectangle(100),
	TAG_END);


	DoMethod(end_cycle, MUIM_Notify, MUIA_Cycle_Active, 0, end_time, 3,
	 MUIM_Set, MUIA_ShowMe, FALSE);

	DoMethod(end_cycle, MUIM_Notify, MUIA_Cycle_Active, 1, end_time, 3,
	 MUIM_Set, MUIA_ShowMe, TRUE);

	DoMethod(end_cycle, MUIM_Notify, MUIA_Cycle_Active, 2, end_time, 3,
	 MUIM_Set, MUIA_ShowMe, FALSE);

	DoMethod(history_on, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, history_group, 3,
	 MUIM_Set, MUIA_Disabled, MUIV_NotTriggerValue);

	DoMethod(vac_button, MUIM_Notify, MUIA_Pressed, FALSE, MUIV_Notify_Application, 5,
	 APPM_DoSqlOnHistoryDatabase, "VACUUM", NULL, 0, NULL);

	return result;
}

static inline Object *CreateContactsListPrefs(VOID)
{
	Object *obj, *show_avatars, *avatars_size, *space_bnd, *show_descs;
	Object *active_pen;

	obj =  MUI_NewObject(MUIC_Group,
		MUIA_Group_Child, EmptyRectangle(100),
		MUIA_Group_Child, MUI_NewObject(MUIC_Group,
			MUIA_Unicode, TRUE,
			MUIA_Background, MUII_GroupBack,
			MUIA_Frame, MUIV_Frame_Group,
			MUIA_FrameTitle, GetString(MSG_PREFS_CONTACTSLIST_DISPLAY),
			MUIA_Group_Child, MUI_NewObject(MUIC_Group,
				MUIA_Group_Child, MUI_NewObject(MUIC_Group, /* descs on/off */
					MUIA_Group_Horiz, TRUE,
					MUIA_Group_Child, (show_descs = MUI_NewObject(MUIC_Image,
						MUIA_Unicode, TRUE,
						MUIA_ObjectID, USD_PREFS_CONTACTSLIST_SHOWDESC_CHECK,
						MUIA_UserData, USD_PREFS_CONTACTSLIST_SHOWDESC_CHECK,
						MUIA_Image_Spec, "6:15",
						MUIA_ShowSelState, FALSE,
						MUIA_Selected, TRUE,
						MUIA_InputMode, MUIV_InputMode_Toggle,
						MUIA_CycleChain, TRUE,
						MUIA_ShortHelp, GetString(MSG_PREFS_CONTACTSLIST_DESC_ONOFF_HELP),
					TAG_END)),
					MUIA_Group_Child, StringLabel(GetString(MSG_PREFS_CONTACTSLIST_DESC_ONOFF), "\33l"),
					MUIA_Group_Child, EmptyRectangle(100),
				TAG_END),
				MUIA_Group_Child,  MUI_NewObject(MUIC_Group, /* space between name and desc */
					MUIA_Group_Horiz, TRUE,
					MUIA_Group_Child, StringLabel(GetString(MSG_PREFS_CONTACTSLIST_SPACENAMEDESC), "\33l"),
					MUIA_Group_Child, (space_bnd = MUI_NewObject(MUIC_Slider,
						MUIA_Unicode, TRUE,
						MUIA_ObjectID, USD_PREFS_CONTACTSLIST_SPACENAMEDESC_SLIDER,
						MUIA_UserData, USD_PREFS_CONTACTSLIST_SPACENAMEDESC_SLIDER,
						MUIA_Slider_Horiz, TRUE,
						MUIA_Slider_Level, 5,
						MUIA_Slider_Max, 20,
						MUIA_Slider_Min, 0,
						MUIA_ShortHelp, GetString(MSG_PREFS_CONTACTSLIST_SPACENAMEDESC_HELP),
					TAG_END)),
				TAG_END),
				MUIA_Group_Child, MUI_NewObject(MUIC_Group, /* avatars on/off */
					MUIA_Group_Horiz, TRUE,
					MUIA_Group_Child, (show_avatars = MUI_NewObject(MUIC_Image,
						MUIA_Unicode, TRUE,
						MUIA_ObjectID, USD_PREFS_CONTACTSLIST_SHOWAVATARS_CHECK,
						MUIA_UserData, USD_PREFS_CONTACTSLIST_SHOWAVATARS_CHECK,
						MUIA_Image_Spec, "6:15",
						MUIA_ShowSelState, FALSE,
						MUIA_Selected, TRUE,
						MUIA_InputMode, MUIV_InputMode_Toggle,
						MUIA_CycleChain, TRUE,
						MUIA_ShortHelp, GetString(MSG_PREFS_CONTACTSLIST_AVATARS_ONOFF_HELP),
					TAG_END)),
					MUIA_Group_Child, StringLabel(GetString(MSG_PREFS_CONTACTSLIST_AVATARS_ONOFF), "\33l"),
					MUIA_Group_Child, EmptyRectangle(100),
				TAG_END),
				MUIA_Group_Child, MUI_NewObject(MUIC_Group, /* avatars size */
					MUIA_Group_Horiz, TRUE,
					MUIA_Group_Child, StringLabel(GetString(MSG_PREFS_CONTACTSLIST_AVATARS_SIZE), "\33l"),
					MUIA_Group_Child, (avatars_size = NewObject(PercentageSliderClass->mcc_Class, NULL,
						MUIA_ObjectID, USD_PREFS_CONTACTSLIST_AVATARSIZE_SLIDER,
						MUIA_UserData, USD_PREFS_CONTACTSLIST_AVATARSIZE_SLIDER,
						MUIA_ShortHelp, GetString(MSG_PREFS_CONTACTSLIST_AVATARS_SIZE_HELP),
						MUIA_Slider_Max, 200,
					TAG_END)),
				TAG_END),
				MUIA_Group_Child, MUI_NewObject(MUIC_Group, /* space between entries */
					MUIA_Group_Horiz, TRUE,
					MUIA_Group_Child, StringLabel(GetString(MSG_PREFS_CONTACTSLIST_SPACEENTRIES), "\33l"),
					MUIA_Group_Child, NewObject(MinMaxSliderClass->mcc_Class, NULL,
						MUIA_ObjectID, USD_PREFS_CONTACTSLIST_SPACEENTRIES_SLIDER,
						MUIA_UserData, USD_PREFS_CONTACTSLIST_SPACEENTRIES_SLIDER,
						MUIA_Slider_Horiz, TRUE,
						MUIA_Slider_Level, 7,
						MUIA_Slider_Max, 20,
						MUIA_Slider_Min, 0,
						MMSA_Unit, GetString(MSG_PIXEL_SLIDER_UNIT),
						MMSA_MinText, GetString(MSG_PREFS_CONTACTSLIST_SPACEENTRIES_NONE),
						MUIA_ShortHelp, GetString(MSG_PREFS_CONTACTSLIST_SPACEENTRIES_HELP),
					TAG_END),
				TAG_END),
				MUIA_Group_Child, MUI_NewObject(MUIC_Group, /* active entry color */
					MUIA_Group_Horiz, TRUE,
					MUIA_Group_Child, StringLabel(GetString(MSG_PREFS_CONTACTSLIST_ACTIVEENTRY_COLOR), "\33r"),
					MUIA_Group_Child, (active_pen = MUI_NewObject(MUIC_Poppen,
						MUIA_Unicode, TRUE,
						MUIA_ObjectID, USD_PREFS_CONTACTSLIST_ACTIVEENTRY_COLOR,
						MUIA_UserData, USD_PREFS_CONTACTSLIST_ACTIVEENTRY_COLOR,
						MUIA_FixWidthTxt, "MM",
						MUIA_ShortHelp, GetString(MSG_PREFS_CONTACTSLIST_ACTIVEENTRY_COLOR_HELP),
					TAG_END)),
					MUIA_Group_Child, EmptyRectangle(200),
				TAG_END),
			TAG_END),
		TAG_END),
		MUIA_Group_Child, EmptyRectangle(100),
	TAG_END);

	DoMethod(show_avatars, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, avatars_size, 3,
	 MUIM_Set, MUIA_Disabled, MUIV_NotTriggerValue);

	DoMethod(show_descs, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, space_bnd, 3,
	 MUIM_Set, MUIA_Disabled, MUIV_NotTriggerValue);

	DoMethod(active_pen, MUIM_Pendisplay_SetMUIPen, HALFSHADOWPEN);

	return obj;
}

static inline Object *CreateFTPPrefs(VOID)
{
	Object *login_group, *parse_but,*login_cycle;
	Object *result;
	static STRPTR conntype[3] = {0};

	conntype[0] = GetString(MSG_PREFS_FTP_LOGIN_ANONYMOUS);
	conntype[1] = GetString(MSG_PREFS_FTP_LOGIN_USERPASS);
	conntype[2] = NULL;

	result = MUI_NewObject(MUIC_Group,
		MUIA_Group_Child, EmptyRectangle(100),
		MUIA_Group_Child, MUI_NewObject(MUIC_Group,
			MUIA_Unicode, TRUE,
			MUIA_Frame, MUIV_Frame_Group,
			MUIA_FrameTitle, GetString(MSG_PREFS_FTP_SERVER_TITLE),
			MUIA_Background, MUII_GroupBack,
			MUIA_Group_Child, MUI_NewObject(MUIC_Group,
				MUIA_Group_Horiz, TRUE,
				MUIA_Group_Child, StringLabel(GetString(MSG_PREFS_FTP_URL), "\33r"),
				MUIA_Group_Child, MUI_NewObject(MUIC_String,
					MUIA_Unicode, TRUE,
					MUIA_UserData, USD_PREFS_FTP_URL_STRING,
					MUIA_Frame, MUIV_Frame_String,
					MUIA_Background, MUII_StringBack,
					MUIA_CycleChain, TRUE,
					MUIA_String_AdvanceOnCR, TRUE,
				TAG_END),
				MUIA_Group_Child, (parse_but = NormalButton("+", 0x00, USD_PREFS_FTP_PARSE_BUTTON, 0)),
			TAG_END),
			MUIA_Group_Child, MUI_NewObject(MUIC_Group,
				MUIA_Group_Horiz, TRUE,
				MUIA_Group_Child, StringLabel(GetString(MSG_PREFS_FTP_HOST), "\33r"),
				MUIA_Group_Child, StringGadget(USD_PREFS_FTP_HOST_STRING),
				MUIA_Group_Child, StringLabel(GetString(MSG_PREFS_FTP_PORT), "\33r"),
				MUIA_Group_Child, StringGadget(USD_PREFS_FTP_PORT_STRING),
			TAG_END),
			MUIA_Group_Child, MUI_NewObject(MUIC_Group,
				MUIA_Group_Horiz, TRUE,
				MUIA_Group_Child, (login_group = MUI_NewObject(MUIC_Group,
					MUIA_Group_Horiz, TRUE,
					MUIA_Disabled, TRUE,
					MUIA_Group_Child, StringLabel(GetString(MSG_PREFS_FTP_USER), "\33r"),
					MUIA_Group_Child, StringGadget(USD_PREFS_FTP_USER_STRING),
					MUIA_Group_Child, StringLabel(GetString(MSG_PREFS_FTP_PASSWORD), "\33r"),
					MUIA_Group_Child, MUI_NewObject(MUIC_String,
						MUIA_Unicode, TRUE,
						MUIA_UserData, USD_PREFS_FTP_PASSWORD_STRING,
						MUIA_ObjectID, USD_PREFS_FTP_PASSWORD_STRING,
						MUIA_Frame, MUIV_Frame_String,
						MUIA_Background, MUII_StringBack,
						MUIA_CycleChain, TRUE,
						MUIA_String_AdvanceOnCR, TRUE,
						MUIA_String_Secret, TRUE,
					TAG_END),
				TAG_END)),
				MUIA_Group_Child, (login_cycle = MUI_NewObject(MUIC_Cycle,
					MUIA_Unicode, TRUE,
					MUIA_UserData, USD_PREFS_FTP_LOGINTYPE_CYCLE,
					MUIA_ObjectID, USD_PREFS_FTP_LOGINTYPE_CYCLE,
					MUIA_HorizWeight, 0,
					MUIA_Cycle_Entries, conntype,
				TAG_END)),
			TAG_END),
			MUIA_Group_Child, MUI_NewObject(MUIC_Group,
				MUIA_Group_Horiz, TRUE,
				MUIA_Group_Child, StringLabel(GetString(MSG_PREFS_FTP_PATH), "\33r"),
				MUIA_Group_Child, StringGadget(USD_PREFS_FTP_SERVERPATH_STRING),
			TAG_END),
		TAG_END),
		MUIA_Group_Child, EmptyRectangle(100),
	TAG_END);

	DoMethod(parse_but, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)MUIV_Notify_Window, 2,
	 PWM_ParseFTPUrl, NULL);

	DoMethod(login_cycle, MUIM_Notify, MUIA_Cycle_Active, 1, login_group, 3,
	 MUIM_Set, MUIA_Disabled, FALSE);

	DoMethod(login_cycle, MUIM_Notify, MUIA_Cycle_Active, 0, login_group, 3,
	 MUIM_Set, MUIA_Disabled, TRUE);

	return result;
}

static inline Object *CreateEmoticonFileSelectGroup(STRPTR emo, STRPTR deffile, ULONG path_id)
{
	Object *result;

	result = MUI_NewObject(MUIC_Group,
		MUIA_Group_Horiz, TRUE,
		MUIA_Group_Child, MUI_NewObject(MUIC_Text,
			MUIA_Unicode, TRUE,
			MUIA_FramePhantomHoriz, TRUE,
			MUIA_Frame, MUIV_Frame_String,
			MUIA_Text_PreParse, "\33c",
			MUIA_Text_Contents, emo,
			MUIA_HorizWeight, 30,
		TAG_END),
		MUIA_Group_Child, MUI_NewObject(MUIC_Popasl,
			MUIA_Unicode, TRUE,
			MUIA_HorizWeight, 100,
			MUIA_Popasl_Type, ASL_FileRequest,
				ASLFR_DrawersOnly, FALSE,
				ASLFR_TitleText, GetString(MSG_PREFS_EMOTICONS_ASL_TITLE),
				ASLFR_RejectIcons, TRUE,
				ASLFR_PositiveText, GetString(MSG_PREFS_EMOTICONS_ASL_OK),
				ASLFR_InitialPattern, "#?.(gif|png|jpg|jpeg|bmp)",
				ASLFR_DoPatterns, TRUE,
				ASLFR_RejectIcons, TRUE,
				ASLFR_DoSaveMode, TRUE,
			MUIA_Popstring_String, MUI_NewObject(MUIC_String,
				MUIA_Unicode, TRUE,
				MUIA_Frame, MUIV_Frame_String,
				MUIA_Background, MUII_StringBack,
				MUIA_ObjectID, path_id,
				MUIA_UserData, path_id,
				MUIA_CycleChain, TRUE,
				MUIA_String_AdvanceOnCR, TRUE,
				MUIA_String_Contents, deffile,
			TAG_END),
			MUIA_Popstring_Button, MUI_NewObject(MUIC_Image,
				MUIA_Unicode, TRUE,
				MUIA_Image_Spec, MUII_PopFile,
				MUIA_InputMode, MUIV_InputMode_RelVerify,
				MUIA_Frame, MUIV_Frame_ImageButton,
				MUIA_Background, MUII_ImageButtonBack,
				MUIA_Image_FreeVert, TRUE,
			TAG_END),
		TAG_END),
	TAG_END);

	return result;
}

static inline Object *CreateEmoticonsPrefs(VOID)
{
	UBYTE i;
	Object *result, *files_group, *onoff, *import, *export, *defaults;

	result = MUI_NewObject(MUIC_Group,
		MUIA_Group_Child, EmptyRectangle(100),
		MUIA_Group_Child, MUI_NewObject(MUIC_Group,
			MUIA_Unicode, TRUE,
			MUIA_Frame, MUIV_Frame_Group,
			MUIA_Background, MUII_GroupBack,
			MUIA_FrameTitle, GetString(MSG_PREFS_EMOTICONS_BASICS),
			MUIA_Group_Columns, 2,
			MUIA_Group_Child, (onoff = MUI_NewObject(MUIC_Image,
				MUIA_Unicode, TRUE,
				MUIA_ObjectID, USD_PREFS_EMOTICONS_ONOFF,
				MUIA_UserData, USD_PREFS_EMOTICONS_ONOFF,
				MUIA_Image_Spec, "6:15",
				MUIA_ShowSelState, FALSE,
				MUIA_Selected, TRUE,
				MUIA_InputMode, MUIV_InputMode_Toggle,
				MUIA_CycleChain, TRUE,
				MUIA_ShortHelp, GetString(MSG_PREFS_EMOTICONS_ON_OFF_HELP),
			TAG_END)),
			MUIA_Group_Child, MUI_NewObject(MUIC_Group,
				MUIA_Group_Horiz, TRUE,
				MUIA_Group_Child, StringLabel(GetString(MSG_PREFS_EMOTICONS_ON_OFF), "\33l"),
				MUIA_Group_Child, EmptyRectangle(100),
			TAG_END),
			MUIA_Group_Child, MUI_NewObject(MUIC_Image,
				MUIA_Unicode, TRUE,
				MUIA_ObjectID, USD_PREFS_EMOTICONS_LONG_ONOFF,
				MUIA_UserData, USD_PREFS_EMOTICONS_LONG_ONOFF,
				MUIA_Image_Spec, "6:15",
				MUIA_ShowSelState, FALSE,
				MUIA_Selected, TRUE,
				MUIA_InputMode, MUIV_InputMode_Toggle,
				MUIA_CycleChain, TRUE,
				MUIA_ShortHelp, GetString(MSG_PREFS_EMOTICONS_LONG_ON_OFF_HELP),
			TAG_END),
			MUIA_Group_Child, MUI_NewObject(MUIC_Group,
				MUIA_Group_Horiz, TRUE,
				MUIA_Group_Child, StringLabel(GetString(MSG_PREFS_EMOTICONS_LONG_ON_OFF), "\33l"),
				MUIA_Group_Child, EmptyRectangle(100),
			TAG_END),
		TAG_END),

		MUIA_Group_Child, (files_group = MUI_NewObject(MUIC_Group,
			MUIA_Unicode, TRUE,
			MUIA_Frame, MUIV_Frame_Group,
			MUIA_Background, MUII_GroupBack,
			MUIA_FrameTitle, GetString(MSG_PREFS_EMOTICONS_IMAGES),
			MUIA_Group_Child, CreateEmoticonFileSelectGroup("??", DEFAULT_QUESTION_EMOTICON_PATH, USD_PREFS_EMOTICONS_PATH_QUESTION),
			MUIA_Group_Child, CreateEmoticonFileSelectGroup("!!", DEFAULT_EXCLAMATION_EMOTICON_PATH, USD_PREFS_EMOTICONS_PATH_EXCLAMATION),
		TAG_END)),
		MUIA_Group_Child, EmptyRectangle(100),
	TAG_END);

	DoMethod(files_group, MUIM_Group_InitChange);

	for(i = 0; i < EmoticonsTabSize; i++)
		DoMethod(files_group, OM_ADDMEMBER, CreateEmoticonFileSelectGroup(EmoticonsTab[i].emo, EmoticonsTab[i].def_file, USD_PREFS_EMOTICONS_PATH(i)));

	DoMethod(files_group, OM_ADDMEMBER, MUI_NewObject(MUIC_Group,
		MUIA_Group_Horiz, TRUE,
		MUIA_Group_Child, EmptyRectangle(100),
		MUIA_Group_Child, (export = NormalButton(GetString(MSG_PREFS_EMOTICONS_EXPORT_PATHS), *GetString(MSG_PREFS_EMOTICONS_EXPORT_PATHS_HOTKEY), 0, 100)),
		MUIA_Group_Child, (import = NormalButton(GetString(MSG_PREFS_EMOTICONS_IMPORT_PATHS), *GetString(MSG_PREFS_EMOTICONS_IMPORT_PATHS_HOTKEY), 0, 100)),
		MUIA_Group_Child, (defaults = NormalButton(GetString(MSG_PREFS_EMOTICONS_DEFAULT_PATHS), *GetString(MSG_PREFS_EMOTICONS_DEFAULT_PATHS_HOTKEY), 0, 100)),
		MUIA_Group_Child, EmptyRectangle(100),
	TAG_END));

	set(defaults, MUIA_ShortHelp, GetString(MSG_PREFS_EMOTICONS_DEFAULT_PATHS_HELP));

	DoMethod(files_group, MUIM_Group_ExitChange);

	DoMethod(onoff, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, files_group, 3,
	 MUIM_Set, MUIA_Disabled, MUIV_NotTriggerValue);

	DoMethod(import, MUIM_Notify, MUIA_Pressed, FALSE, MUIV_Notify_Window, 1,
	 PWM_ImportEmoticonsPaths);

	DoMethod(export, MUIM_Notify, MUIA_Pressed, FALSE, MUIV_Notify_Window, 1,
	 PWM_ExportEmoticonsPaths);

	DoMethod(defaults, MUIM_Notify, MUIA_Pressed, FALSE, MUIV_Notify_Window, 1,
	 PWM_SetDefaultEmoticonsPaths);

	return result;
}

static inline Object *CreateFKeyPrefs(VOID)
{
	Object *result;

	result = MUI_NewObject(MUIC_Group,
		MUIA_Group_Child, EmptyRectangle(100),
		MUIA_Group_Child, MUI_NewObject(MUIC_Group,
			MUIA_Unicode, TRUE,
			MUIA_Frame, MUIV_Frame_Group,
			MUIA_FrameTitle, GetString(MSG_PREFSWINDOW_FKEYS),
			MUIA_Background, MUII_GroupBack,
			MUIA_Group_Columns, 2,
			MUIA_Group_Child, StringLabel("F1", "\33r"),
			MUIA_Group_Child, StringGadget(USD_PREFS_FKEYS_STRING(0)),
			MUIA_Group_Child, StringLabel("F2", "\33r"),
			MUIA_Group_Child, StringGadget(USD_PREFS_FKEYS_STRING(1)),
			MUIA_Group_Child, StringLabel("F3", "\33r"),
			MUIA_Group_Child, StringGadget(USD_PREFS_FKEYS_STRING(2)),
			MUIA_Group_Child, StringLabel("F4", "\33r"),
			MUIA_Group_Child, StringGadget(USD_PREFS_FKEYS_STRING(3)),
			MUIA_Group_Child, StringLabel("F5", "\33r"),
			MUIA_Group_Child, StringGadget(USD_PREFS_FKEYS_STRING(4)),
			MUIA_Group_Child, StringLabel("F6", "\33r"),
			MUIA_Group_Child, StringGadget(USD_PREFS_FKEYS_STRING(5)),
			MUIA_Group_Child, StringLabel("F7", "\33r"),
			MUIA_Group_Child, StringGadget(USD_PREFS_FKEYS_STRING(6)),
			MUIA_Group_Child, StringLabel("F8", "\33r"),
			MUIA_Group_Child, StringGadget(USD_PREFS_FKEYS_STRING(7)),
			MUIA_Group_Child, StringLabel("F9", "\33r"),
			MUIA_Group_Child, StringGadget(USD_PREFS_FKEYS_STRING(8)),
			MUIA_Group_Child, StringLabel("F10", "\33r"),
			MUIA_Group_Child, StringGadget(USD_PREFS_FKEYS_STRING(9)),
			MUIA_Group_Child, StringLabel("F11", "\33r"),
			MUIA_Group_Child, StringGadget(USD_PREFS_FKEYS_STRING(10)),
			MUIA_Group_Child, StringLabel("F12", "\33r"),
			MUIA_Group_Child, StringGadget(USD_PREFS_FKEYS_STRING(11)),
		TAG_END),
		MUIA_Group_Child, EmptyRectangle(100),
	TAG_END);

	return result;
}


static IPTR PrefsWindowNew(Class *cl, Object *obj, struct opSet *msg)
{
	Object *list, *page_group, *save_b, *use_b, *cancel_b;
	static CONST_STRPTR statuses[6];

	CONST_STRPTR pages[] =
	{
		GetString(MSG_PREFSWINDOW_PROGRAM),
		GetString(MSG_PREFSWINDOW_CONTACTSLISTPREFS),
		GetString(MSG_PREFSWINDOW_TALKWINDOW),
		GetString(MSG_PREFSWINDOW_LOGSPREFS),
		GetString(MSG_PREFSWINDOW_FTP),
		GetString(MSG_PREFSWINDOW_EMOTICONS),
		GetString(MSG_PREFSWINDOW_FKEYS),
		NULL,
	};

	if((statuses[0] = FmtNew("%s %s", "\33I[4:PROGDIR:gfx/available.mbr]", GetString(MSG_GG_STATUS_AVAIL))))
	{
		if((statuses[1] = FmtNew("%s %s", "\33I[4:PROGDIR:gfx/away.mbr]", GetString(MSG_GG_STATUS_AWAY))))
		{
			if((statuses[2] = FmtNew("%s %s", "\33I[4:PROGDIR:gfx/invisible.mbr]", GetString(MSG_GG_STATUS_INVISIBLE))))
			{
				if((statuses[3] = FmtNew("%s %s", "\33I[4:PROGDIR:gfx/ffc.mbr]", GetString(MSG_GG_STATUS_FFC))))
				{
					if((statuses[4] = FmtNew("%s %s", "\33I[4:PROGDIR:gfx/dnd.mbr]", GetString(MSG_GG_STATUS_DND))))
					{
						statuses[5] = NULL;

						obj = DoSuperNew(cl, obj,
							MUIA_UserData, USD_PREFS_WINDOW_WIN,
							MUIA_Window_ID, USD_PREFS_WINDOW_WIN,
							MUIA_Background, MUII_WindowBack,
							MUIA_Window_ScreenTitle, APP_SCREEN_TITLE,
							MUIA_Window_Title, GetString(MSG_PREFSWINDOW_TITLE),
							MUIA_Window_UseRightBorderScroller, TRUE,
							MUIA_Window_UseBottomBorderScroller, TRUE,
							MUIA_Window_RootObject, MUI_NewObject(MUIC_Group,
								MUIA_Group_Child, MUI_NewObject(MUIC_Group,
									MUIA_Background, MUII_PageBack,
									MUIA_Group_Horiz, TRUE,
									MUIA_Group_Child, (list = MUI_NewObject(MUIC_List,
										MUIA_Unicode, TRUE,
										MUIA_Frame, MUIV_Frame_ReadList,
										MUIA_Background, MUII_ReadListBack,
										MUIA_Font, MUIV_Font_List,
										MUIA_UserData, USD_PREFS_PAGES_LIST,
										MUIA_List_AdjustWidth, TRUE,
										MUIA_List_AutoVisible, TRUE,
										MUIA_List_Active, 0,
										MUIA_List_ConstructHook, MUIV_List_ConstructHook_String,
										MUIA_List_DestructHook, MUIV_List_DestructHook_String,
										MUIA_List_SourceArray, pages,
									TAG_END)),

									MUIA_Group_Child, (page_group = MUI_NewObject(MUIC_Group,
										MUIA_Group_PageMode, TRUE,
										MUIA_Group_Child, MUI_NewObject(MUIC_Scrollgroup,
											MUIA_Scrollgroup_UseWinBorder, TRUE,
											MUIA_Scrollgroup_Contents, MUI_NewObject(MUIC_Virtgroup,
												MUIA_Group_Child, CreateProgramPrefs(statuses),
											TAG_END),
										TAG_END),
										MUIA_Group_Child, MUI_NewObject(MUIC_Scrollgroup,
											MUIA_Scrollgroup_UseWinBorder, TRUE,
											MUIA_Scrollgroup_Contents, MUI_NewObject(MUIC_Virtgroup,
												MUIA_Group_Child, CreateContactsListPrefs(),
											TAG_END),
										TAG_END),
										MUIA_Group_Child, MUI_NewObject(MUIC_Scrollgroup,
											MUIA_Scrollgroup_UseWinBorder, TRUE,
											MUIA_Scrollgroup_Contents, MUI_NewObject(MUIC_Virtgroup,
												MUIA_Group_Child, CreateTalkWindowPrefs(),
											TAG_END),
										TAG_END),
										MUIA_Group_Child, MUI_NewObject(MUIC_Scrollgroup,
											MUIA_Scrollgroup_UseWinBorder, TRUE,
											MUIA_Scrollgroup_Contents, MUI_NewObject(MUIC_Virtgroup,
												MUIA_Group_Child, CreateLogsPrefs(),
											TAG_END),
										TAG_END),
										MUIA_Group_Child, MUI_NewObject(MUIC_Scrollgroup,
											MUIA_Scrollgroup_UseWinBorder, TRUE,
											MUIA_Scrollgroup_Contents, MUI_NewObject(MUIC_Virtgroup,
												MUIA_Group_Child, CreateFTPPrefs(),
											TAG_END),
										TAG_END),
										MUIA_Group_Child, MUI_NewObject(MUIC_Scrollgroup,
											MUIA_Scrollgroup_UseWinBorder, TRUE,
											MUIA_Scrollgroup_Contents, MUI_NewObject(MUIC_Virtgroup,
												MUIA_Group_Child, CreateEmoticonsPrefs(),
											TAG_END),
										TAG_END),
										MUIA_Group_Child, MUI_NewObject(MUIC_Scrollgroup,
											MUIA_Scrollgroup_UseWinBorder, TRUE,
											MUIA_Scrollgroup_Contents, MUI_NewObject(MUIC_Virtgroup,
												MUIA_Group_Child, CreateFKeyPrefs(),
											TAG_END),
										TAG_END),
									TAG_END)),

								TAG_END),
								MUIA_Group_Child, MUI_NewObject(MUIC_Group,
									MUIA_Group_Horiz, TRUE,
									MUIA_Group_Child, (save_b = NormalButton(GetString(MSG_PREFSWINDOW_SAVE), *(GetString(MSG_PREFSWINDOW_SAVE_HOTKEY)), USD_PREFS_WINDOW_SAVE, 50)),
									MUIA_Group_Child, EmptyRectangle(100),
									MUIA_Group_Child, (use_b = NormalButton(GetString(MSG_PREFSWINDOW_USE), *(GetString(MSG_PREFSWINDOW_USE_HOTKEY)), USD_PREFS_WINDOW_USE, 50)),
									MUIA_Group_Child, EmptyRectangle(100),
									MUIA_Group_Child, (cancel_b = NormalButton(GetString(MSG_PREFSWINDOW_CANCEL), *(GetString(MSG_PREFSWINDOW_CANCEL_HOTKEY)), USD_PREFS_WINDOW_CANCEL, 50)),
								TAG_END),
							TAG_END),
						TAG_MORE, msg->ops_AttrList);

						if(obj)
						{
							ULONG i;
							struct PrefsWindowData *d = INST_DATA(cl, obj);

							d->page_group = page_group;
							d->tabs_list = list;

							DoMethod(obj, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, MUIV_Notify_Self, 3,
							 MUIM_Set, MUIA_Window_Open, FALSE);

							DoMethod(cancel_b, MUIM_Notify, MUIA_Pressed, FALSE, obj, 3,
							 MUIM_Set, MUIA_Window_Open, FALSE);

							DoMethod(list, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime, page_group, 3,
							 MUIM_Set, MUIA_Group_ActivePage, MUIV_TriggerValue);

							DoMethod(save_b, MUIM_Notify, MUIA_Pressed, FALSE, obj, 2,
							 PWM_SavePrefs, TRUE);

							DoMethod(use_b, MUIM_Notify, MUIA_Pressed, FALSE, obj, 2,
							 PWM_SavePrefs, FALSE);

							PreferencesWindow = obj; /* initalize global pointer */

							for(i = 0; i < 6; i++)
								d->statuses[i] = statuses[i];

							return (IPTR)obj;
						}
					}
				}
			}
		}
	}

	CoerceMethod(cl, obj, OM_DISPOSE);

	return (IPTR)0;
}


static IPTR PrefsWindowDispose(Class *cl, Object *obj, Msg msg)
{
	struct PrefsWindowData *d = INST_DATA(cl, obj);
	ULONG i;

	for(i = 0; i < 6; i++)
		FmtFree((APTR)d->statuses[i]);

	return DoSuperMethodA(cl, obj, msg);
}


static IPTR PrefsWindowGet(Class *cl, Object *obj, struct opGet *msg)
{
	int rv = FALSE;

	switch(msg->opg_AttrID)
	{
		case PWA_PrefsChanged:
			return TRUE;

		default: rv = (DoSuperMethodA(cl, obj, (Msg)msg));
	}

	return rv;
}

static IPTR PrefsWindowSavePrefs(Class *cl, Object *obj, struct PWP_SavePrefs *msg)
{
	set(obj, MUIA_Window_Open, FALSE);
	DoMethod(_app(obj), MUIM_Application_Save, (IPTR)MUIV_Application_Save_ENV);
	if(msg->EnvArc) DoMethod(_app(obj), MUIM_Application_Save, (IPTR)MUIV_Application_Save_ENVARC);
	set(obj, PWA_PrefsChanged, TRUE);
	return (IPTR)1;
}

static IPTR PrefsWindowAddPrefsPage(Class *cl, Object *obj, struct PWP_AddPrefsPage *msg)
{
	struct PrefsWindowData *d = INST_DATA(cl, obj);

	if(DoMethod(d->page_group, MUIM_Group_InitChange))
	{
		DoMethod(d->page_group, OM_ADDMEMBER, MUI_NewObject(MUIC_Scrollgroup,
			MUIA_Scrollgroup_UseWinBorder, TRUE,
			MUIA_Scrollgroup_Contents, MUI_NewObject(MUIC_Virtgroup,
				MUIA_Group_Child, msg->PrefsPage,
			TAG_END),
		TAG_END));
		DoMethod(d->page_group, MUIM_Group_ExitChange);
	}

	return (IPTR)0;
}

static IPTR PrefsWindowParseFTPUrl(Class *cl, Object *obj, struct PWP_ParseFTPUrl *msg)
{
	if(!msg->Url)
		msg->Url = (STRPTR)xget(prefs_object(USD_PREFS_FTP_URL_STRING), MUIA_String_Contents);

	if(msg->Url)
	{
		STRPTR user = NULL, pass = NULL, host = NULL, path = NULL;
		LONG port;

		if((port = ParseFtpUrl(msg->Url, &user, &pass, &host, &path)) != -1)
		{
			if(user)
			{
				set(prefs_object(USD_PREFS_FTP_USER_STRING), MUIA_String_Contents, (IPTR)user);
				set(prefs_object(USD_PREFS_FTP_LOGINTYPE_CYCLE), MUIA_Cycle_Active, 1);
				StrFree(user);
			}
			else
				set(prefs_object(USD_PREFS_FTP_LOGINTYPE_CYCLE), MUIA_Cycle_Active, 0);

			if(pass)
			{
				set(prefs_object(USD_PREFS_FTP_PASSWORD_STRING), MUIA_String_Contents, (IPTR)pass);
				StrFree(pass);
			}

			if(host)
			{
				set(prefs_object(USD_PREFS_FTP_HOST_STRING), MUIA_String_Contents, (IPTR)host);
				StrFree(host);
			}

			if(path)
			{
				set(prefs_object(USD_PREFS_FTP_SERVERPATH_STRING), MUIA_String_Contents, (IPTR)path);
				StrFree(path);
			}
			set(prefs_object(USD_PREFS_FTP_PORT_STRING), MUIA_String_Integer, port);
		}
	}
	return (IPTR)0;
}

static IPTR PrefsWindowExportEmoticonsPaths(Class *cl, Object *obj)
{
	struct FileRequester *freq;

	set(_app(obj), MUIA_Application_Sleep, TRUE);

	if((freq = MUI_AllocAslRequestTags(ASL_FileRequest, TAG_END)))
	{
		if(MUI_AslRequestTags(freq,
			ASLFR_TitleText, GetString(MSG_PREFS_EMOTICONS_EXPORT_ASL_TITLE),
			ASLFR_PositiveText, GetString(MSG_PREFS_EMOTICONS_EXPORT_ASL_OK),
			ASLFR_InitialPattern, "#?.txt",
			ASLFR_DoPatterns, TRUE,
			ASLFR_RejectIcons, TRUE,
			ASLFR_DoSaveMode, TRUE,
			ASLFR_InitialFile, "emoticonspaths.txt",
		TAG_END))
		{
			UBYTE location[500];
			BPTR fh;

			StrNCopy(freq->fr_Drawer, (STRPTR)location, sizeof(location));
			AddPart((STRPTR)location, freq->fr_File, sizeof(location));

			if(freq->fr_File && !StrIStr(freq->fr_File, ".txt"))
				StrCat(".txt", location);

			if((fh = Open((STRPTR)location, MODE_NEWFILE)))
			{
				STRPTR path;
				ULONG len;
				LONG i;

				for(i = 0; i < EmoticonsTabSize; i++)
				{
					path = (STRPTR)xget(prefs_object(USD_PREFS_EMOTICONS_PATH(i)), MUIA_String_Contents);
					len = StrLen(path);

					if(path && len > 0)
					{
						FWrite(fh, EmoticonsTab[i].emo, StrLen(EmoticonsTab[i].emo), 1);
						FPutC(fh, '\t');
						FWrite(fh, path, StrLen(path), 1);
						FPutC(fh, '\n');
					}
				}

				path = (STRPTR)xget(prefs_object(USD_PREFS_EMOTICONS_PATH_QUESTION), MUIA_String_Contents);
				len = StrLen(path);

				if(path && len > 0)
				{
					FWrite(fh, "??", 2, 1);
					FPutC(fh, '\t');
					FWrite(fh, path, len, 1);
					FPutC(fh, '\n');
				}

				path = (STRPTR)xget(prefs_object(USD_PREFS_EMOTICONS_PATH_EXCLAMATION), MUIA_String_Contents);
				len = StrLen(path);

				if(path && len > 0)
				{
					FWrite(fh, "!!", 2, 1);
					FPutC(fh, '\t');
					FWrite(fh, path, len, 1);
					FPutC(fh, '\n');
				}

				Close(fh);
			}
		}

		MUI_FreeAslRequest(freq);
	}

	set(_app(obj), MUIA_Application_Sleep, FALSE);

	return (IPTR)0;
}

static IPTR PrefsWindowImportEmoticonsPaths(Class *cl, Object *obj)
{
	struct FileRequester *freq;

	set(_app(obj), MUIA_Application_Sleep, TRUE);

	if((freq = MUI_AllocAslRequestTags(ASL_FileRequest, TAG_END)))
	{
		if(MUI_AslRequestTags(freq,
			ASLFR_TitleText, GetString(MSG_PREFS_EMOTICONS_IMPORT_ASL_TITLE),
			ASLFR_PositiveText, GetString(MSG_PREFS_EMOTICONS_IMPORT_ASL_OK),
			ASLFR_InitialPattern, "#?.txt",
			ASLFR_DoPatterns, TRUE,
			ASLFR_RejectIcons, TRUE,
			ASLFR_SleepWindow, TRUE,
			TAG_END))
		{
			UBYTE location[500];
			struct FileInfoBlock fib;
			BPTR fh;

			StrNCopy(freq->fr_Drawer, (STRPTR)location, 500);
			AddPart((STRPTR)location, freq->fr_File, 500);

			if((fh = Open((STRPTR)location, MODE_OLDFILE)))
			{
				STRPTR buffer;

				if((ExamineFH(fh, &fib)))
				{
					if((buffer = AllocMem(fib.fib_Size + 1, MEMF_ANY | MEMF_CLEAR)))
					{
						if(FRead(fh, (APTR)buffer, fib.fib_Size, 1) == 1)
						{
							STRPTR emo = buffer;
							STRPTR path = buffer;

							buffer[fib.fib_Size] = 0x00;

							while(emo < buffer + fib.fib_Size)
							{
								if(*emo == '\t' || *emo == '\n')
									*emo = 0x00;
								emo++;
							}

							emo = buffer;

							while(emo < buffer + fib.fib_Size)
							{
								path++;
								while(path < buffer + fib.fib_Size && *path++);

								if(path < buffer + fib.fib_Size)
								{
									LONG i;

									if(StrEqu("??", emo))
									{
										set(prefs_object(USD_PREFS_EMOTICONS_PATH_QUESTION), MUIA_String_Contents, path);
									}
									else if(StrEqu("!!", emo))
									{
										set(prefs_object(USD_PREFS_EMOTICONS_PATH_EXCLAMATION), MUIA_String_Contents, path);
									}
									else
									{
										for(i = 0; i < EmoticonsTabSize; i++)
										{
											if(StrEqu(EmoticonsTab[i].emo, emo))
											{
												set(prefs_object(USD_PREFS_EMOTICONS_PATH(i)), MUIA_String_Contents, path);
												break;
											}
										}
									}
								}

								emo = path + 1;

								while(emo < buffer + fib.fib_Size && *emo++);

								path = emo;
							}
						}
						FreeMem(buffer, fib.fib_Size);
					}
				}
				Close(fh);
			}
		}
		MUI_FreeAslRequest(freq);
	}

	DoMethod(obj, CLSM_Sort);
	DoMethod(obj, CLSM_SaveList);
	set(_app(obj), MUIA_Application_Sleep, FALSE);

	return (IPTR)0;
}

static IPTR PrefsWindowSetDefaultEmoticonsPaths(Class *cl, Object *obj)
{
	struct PrefsWindowData *d = INST_DATA(cl, obj);
	LONG i;

	for(i = 0; i < EmoticonsTabSize; i++)
		set(findobj(USD_PREFS_EMOTICONS_PATH(i), d->page_group), MUIA_String_Contents, EmoticonsTab[i].def_file);

	set(findobj(USD_PREFS_EMOTICONS_PATH_QUESTION, d->page_group), MUIA_String_Contents, DEFAULT_QUESTION_EMOTICON_PATH);
	set(findobj(USD_PREFS_EMOTICONS_PATH_EXCLAMATION, d->page_group), MUIA_String_Contents, DEFAULT_EXCLAMATION_EMOTICON_PATH);

	return (IPTR)0;
}

static IPTR PrefsWindowDispatcher(VOID)
{
	Class *cl = (Class*)REG_A0;
	Object *obj = (Object*)REG_A2;
	Msg msg = (Msg)REG_A1;

	switch(msg->MethodID)
	{
		case OM_NEW: return (PrefsWindowNew(cl, obj, (struct opSet*)msg));
		case OM_GET: return (PrefsWindowGet(cl, obj, (struct opGet*)msg));
		case OM_DISPOSE: return (PrefsWindowDispose(cl, obj, msg));
		case PWM_SavePrefs: return(PrefsWindowSavePrefs(cl, obj, (struct PWP_SavePrefs*)msg));
		case PWM_AddPrefsPage: return(PrefsWindowAddPrefsPage(cl, obj, (struct PWP_AddPrefsPage*)msg));
		case PWM_ParseFTPUrl: return(PrefsWindowParseFTPUrl(cl, obj, (struct PWP_ParseFTPUrl*)msg));
		case PWM_ExportEmoticonsPaths: return(PrefsWindowExportEmoticonsPaths(cl, obj));
		case PWM_ImportEmoticonsPaths: return(PrefsWindowImportEmoticonsPaths(cl, obj));
		case PWM_SetDefaultEmoticonsPaths: return(PrefsWindowSetDefaultEmoticonsPaths(cl, obj));
		default: return(DoSuperMethodA(cl, obj, msg));
	}
}

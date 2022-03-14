/*
 * Copyright (c) 2012-2022 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/icon.h>
#include <proto/socket.h>
#include <proto/ezxml.h>
#include <proto/commodities.h>
#include <proto/asl.h>
#include <proto/sqlite.h>
#include <mui/Aboutbox_mcc.h>
#include <libraries/gadtools.h>
#include <intuition/screenbar.h>
#include <libvstring.h>

#include "globaldefines.h"
#include "application.h"
#include "mainwindow.h"
#include "prefswindow.h"
#include "contactslist.h"
#include "talktab.h"
#include "talkwindow.h"
#include "descwindow.h"
#include "editconwindow.h"
#include "smallsbar.h"
#include "support.h"
#include "ipc.h"
#include "locale.h"
#include "modules.h"
#include "modulescycle.h"
#include "ftp.h"
#include "moduleslogwindow.h"
#include "slaveprocess.h"
#include "historycontactslist.h"
#include "historyconversationslist.h"
#include "historywindow.h"
#include "historysql.h"

#include "kwakwa_api/defs.h"
#include "kwakwa_api/protocol.h"

#ifdef FD_ZERO
#undef FD_ZERO
#define FD_ZERO(ptr) MemSet(ptr, 0x00, sizeof(*ptr))
#endif /* FD_ZERO */

struct APPP_ScreenbarInit {ULONG MethodID; Object *sbar;};
struct APPP_ScreenbarMenu {ULONG MethodID; ULONG menu_result;};
struct APPP_ScreenbarChange {ULONG MethodID; ULONG new_status;};
struct APPP_Connect {ULONG MethodID; ULONG new_status; STRPTR description;};
struct APPP_Disconnect {ULONG MethodID; STRPTR description;};
struct APPP_ChangeStatus {ULONG MethodID; ULONG new_status; STRPTR description;};
struct APPP_NotifyContactList {ULONG MethodID; struct Module *m;};
struct APPP_ListChangeAck {ULONG MethodID; struct Module *m; struct ListChange *change;};
struct APPP_NewMessageAck {ULONG MethodID; struct Module *m; struct NewMessage *nmsg;};
struct APPP_SendMessage {ULONG MethodID; ULONG pluginid; STRPTR contactid; STRPTR txt;};
struct APPP_SendTypingNotify {ULONG MethodID; ULONG pluginid; STRPTR contactid; ULONG txt_len;};
struct APPP_AddNotify {ULONG MethodID; ULONG pluginid; STRPTR contactid; ULONG contact_status;};
struct APPP_RemoveNotify {ULONG MethodID; ULONG pluginid; STRPTR contactid; ULONG contact_status;};
struct APPP_StatusChangeAck {ULONG MethodID; struct Module *m; struct StatusChange *change;};
struct APPP_DisconnectAck {ULONG MethodID; struct Module *m;};
struct APPP_ParseEvent {ULONG MethodID; struct Module *m; struct KwaEvent *event;};
struct APPP_TypingNotifyAck {ULONG MethodID; struct Module *m; struct TypingNotify *notify;};
struct APPP_ConnectAck {ULONG MethodID; struct Module *m;};
struct APPP_ModuleMessageAck {ULONG MethodID; struct Module *m; struct ModuleMessage* event;};
struct APPP_ErrorNoMem {ULONG MethodID; struct Module *m;};
struct APPP_ErrorLoginFail {ULONG MethodID; struct Module *m;};
struct APPP_ErrorConnFail {ULONG MethodID; struct Module *m;};
struct APPP_ErrorNotSupported {ULONG MethodID; struct Module *m; STRPTR errormsg;};
struct APPP_NotifyBeacon {ULONG MethodID; STRPTR notification_name; STRPTR message; ULONG wait_for_result; Object *obj; ULONG method; IPTR usr_data;};
struct APPP_SendHttpGet {ULONG MethodID; STRPTR url; STRPTR user_agent; Object *obj; ULONG method; IPTR user_data;};
struct APPP_SendHttpPost {ULONG MethodID; STRPTR url; STRPTR user_agent; struct TagItem *data; ULONG items_no; Object *obj; ULONG method; IPTR user_data;};
struct APPP_NewAvatarAck {ULONG MethodID; struct Module *m; struct NewAvatar *newavatar;};
struct APPP_ClipboardStart {ULONG MethodID; ULONG length;};
struct APPP_ClipboardWrite {ULONG MethodID; STRPTR data; LONG length;};
struct APPP_ImportListAck {ULONG MethodID; struct Module *m; struct ImportList *event;};
struct APPP_ExportListAck {ULONG MethodID; struct Module *m; struct ExportList *event;};
struct APPP_SendPicture {ULONG MethodID; ULONG pluginid; STRPTR contactid; STRPTR path;};
struct APPP_NewPictureAck {ULONG MethodID; struct Module *m; struct NewPicture *event;};
struct APPP_FtpPut {ULONG MethodID; ULONG pluginid; STRPTR contactid;};
struct APPP_FtpPutCallback {ULONG MethodID; LONG error_code; struct InsertLinkParms *parms;};
struct APPP_AnswerInvite {ULONG MethodID; ULONG pluginid; STRPTR contactid; ULONG answer;};
struct APPP_NewInviteAck {ULONG MethodID; struct Module *m; struct NewInvite *event;};
struct APPP_PubDirRequest {ULONG MethodID; ULONG pluginid; STRPTR contactid; Object *obj; ULONG method;};
struct APPP_GetModuleName {ULONG MethodID; ULONG pluginid;};
struct APPP_GetModuleUserId {ULONG MethodID; ULONG pluginid;};
struct APPP_ConvertContactEntry {ULONG MethodID; struct ContactEntry *entry;};
struct APPP_DoSqlOnHistoryDatabase {ULONG MethodID; STRPTR sql; Object *callback_obj; ULONG callback_method; QUAD *last_row;};
struct APPP_InsertMessageIntoHistory {ULONG MethodID; QUAD *conversation_id; STRPTR contact_id; STRPTR contact_name; ULONG a_timestamp; ULONG flags; UBYTE *content; ULONG content_len;};
struct APPP_InsertConversationIntoHistory {ULONG MethodID; ULONG plugin_id; STRPTR contact_id; STRPTR contact_name; ULONG flags; QUAD *inserted_id;};
struct APPP_GetLastMessagesFromHistory {ULONG MethodID; Object *callback_obj; ULONG callback_method; ULONG plugin_id; STRPTR contact_id; ULONG messages_no;};
struct APPP_GetLastConvMsgsFromHistory {ULONG MethodID; Object *callback_obj; ULONG callback_method; ULONG plugin_id; STRPTR contact_id;};
struct APPP_GetLastMessagesByTime {ULONG MethodID; Object *callback_obj; ULONG callback_method; ULONG plugin_id; STRPTR contact_id; ULONG secs;};
struct APPP_GetLastConversation {ULONG MethodID; ULONG plugin_id; STRPTR contact_id; QUAD *conversation_id; ULONG *timestamp;};
struct APPP_GetContactsFromHistory {ULONG MethodID; Object *list;};
struct APPP_GetConversationsFromHistory {ULONG MethodID; Object *list; ULONG plugin_id; STRPTR contact_id;};
struct APPP_GetMessagesFromHistory {ULONG MethodID; Object *callback_obj; ULONG callback_method; QUAD *conversation_id;};
struct APPP_DeleteContactFromHistory {ULONG MethodID; ULONG plugin_id; STRPTR contact_id;};
struct APPP_DeleteConversationFromHistory {ULONG MethodID; QUAD *conversation_id;};
struct APPP_SetLastStatus {ULONG MethodID; ULONG status; STRPTR desc;};

struct ApplicationData
{
	struct DiskObject *icon;
	Object *about_window, *main_window, *prefs_window, *talk_window, *desc_window, *edit_con_window, *modules_log_window, *tools_menu;
	Object *history_window, *screenbar;
	struct SBarControl *sctl;
	struct Picture *available_pic, *away_pic, *dnd_pic, *ffc_pic, *invisible_pic, *unavailable_pic, *newmsg_pic;

	/* last status */
	ULONG last_status;
	STRPTR last_desc;

	/* slave process */
	Object *slave_object;
	ULONG slave_mask;

	/* modules */
	ULONG modules_no;
	ULONG connecting_modules_no;
	struct MinList modules;
	struct MUI_InputHandlerNode sec_ihn;

	/* clipboard */
	struct MsgPort *clipboard_port;
	struct IOClipReq *clipboard_request;
	BOOL add_byte;

	/* broker */
	BYTE broker_signal;
	CxObj *mouse_filter, *keyboard_filter;
	ULONG broker_start_time, broker_act_time;
	BOOL broker_status_away;
	ULONG broker_old_status;
	STRPTR broker_old_desc;

	/* history database */
	sqlite3 *history_database;
	sqlite3_stmt *history_stmt[SQL_STMT_NO];

	/* tooltypes */
	BOOL disable_autoconnect;
	STRPTR *ignore_modules;
	ULONG ignore_modules_count;
};

struct MUI_CustomClass *ApplicationClass;
static IPTR ApplicationDispatcher(VOID);
const struct EmulLibEntry ApplicationGate = {TRAP_LIB, 0, (VOID(*)(VOID))ApplicationDispatcher};

struct MUI_CustomClass *CreateApplicationClass(VOID)
{
	struct MUI_CustomClass *cl;

	cl = MUI_CreateCustomClass(NULL, MUIC_Application, NULL, sizeof(struct ApplicationData), (APTR)&ApplicationGate);
	ApplicationClass = cl;
	return cl;
}

VOID DeleteApplicationClass(VOID)
{
	if(ApplicationClass) MUI_DeleteCustomClass(ApplicationClass);
}

static inline struct Module* FindModuleById(struct MinList *modules, ULONG id)
{
	struct Module *m;

	ForeachNode(modules, m)
		if(m->mod_ID == id)
			return m;

	return NULL;
}

static inline Object *CreateMenuStrip(Object **app_menu, Object **contact_list_menu, Object **prefs_menu, Object **tools_menu)
{
	Object *obj = MUI_NewObject(MUIC_Menustrip,

		MUIA_Group_Child, MUI_NewObject(MUIC_Menu,
			MUIA_Menu_Title, GetString(MSG_MENU_APPMENU_TITLE),

			MUIA_Group_Child, (app_menu[0] = MUI_NewObject(MUIC_Menuitem,
				MUIA_Menuitem_Title, GetString(MSG_MENU_APPMENU_ABOUT),
				MUIA_Menuitem_Shortcut, "?",
			TAG_END)),

			MUIA_Group_Child, (app_menu[1] = MUI_NewObject(MUIC_Menuitem,
				MUIA_Menuitem_Title, GetString(MSG_MENU_APPMENU_ABOUTMUI),
			TAG_END)),

			MUIA_Group_Child, MUI_NewObject(MUIC_Menuitem,
				MUIA_Menuitem_Title, NM_BARLABEL,
			TAG_END),

			MUIA_Group_Child, (app_menu[2] = MUI_NewObject(MUIC_Menuitem,
				MUIA_Menuitem_Title, GetString(MSG_MENU_APPMENU_QUIT),
				MUIA_Menuitem_Shortcut, "Q",
			TAG_END)),

		TAG_END),


		MUIA_Group_Child, MUI_NewObject(MUIC_Menu,
			MUIA_Menu_Title, GetString(MSG_MENU_LIST_TITLE),

			MUIA_Group_Child, (contact_list_menu[0] = MUI_NewObject(MUIC_Menuitem,
				MUIA_Menuitem_Title, GetString(MSG_MENU_LIST_ADDENTRY),
				MUIA_Menuitem_Shortcut, "N",
			TAG_END)),

			MUIA_Group_Child, (contact_list_menu[1] = MUI_NewObject(MUIC_Menuitem,
				MUIA_Menuitem_Title, GetString(MSG_MENU_LIST_SEARCHENTRY),
				MUIA_Menuitem_Shortcut, "F",
			TAG_END)),

			MUIA_Group_Child, MUI_NewObject(MUIC_Menuitem,
				MUIA_Menuitem_Title, NM_BARLABEL,
			TAG_END),

			MUIA_Group_Child, (contact_list_menu[2] = MUI_NewObject(MUIC_Menuitem,
				MUIA_Menuitem_Title, GetString(MSG_MENU_LIST_CLEAR),
			TAG_END)),

			MUIA_Group_Child, (contact_list_menu[3] = MUI_NewObject(MUIC_Menuitem,
				MUIA_Menuitem_Title, GetString(MSG_MENU_LIST_REMOVE_CLONES),
			TAG_END)),

			MUIA_Group_Child, MUI_NewObject(MUIC_Menuitem,
				MUIA_Menuitem_Title, NM_BARLABEL,
			TAG_END),

			MUIA_Group_Child, (contact_list_menu[4] = MUI_NewObject(MUIC_Menuitem,
				MUIA_Menuitem_Title, GetString(MSG_MENU_LIST_IMPORT_FILE),
				MUIA_Menuitem_Shortcut, "L",
			TAG_END)),

			MUIA_Group_Child, (contact_list_menu[5] = MUI_NewObject(MUIC_Menuitem,
				MUIA_Menuitem_Title, GetString(MSG_MENU_LIST_EXPORT_FILE),
				MUIA_Menuitem_Shortcut, "S",
			TAG_END)),

			MUIA_Group_Child, MUI_NewObject(MUIC_Menuitem,
				MUIA_Menuitem_Title, NM_BARLABEL,
			TAG_END),

			MUIA_Group_Child, (contact_list_menu[6] = MUI_NewObject(MUIC_Menuitem,
				MUIA_Menuitem_Title, GetString(MSG_MENU_LIST_IMPORT_SERVER),
			TAG_END)),

			MUIA_Group_Child, (contact_list_menu[7] = MUI_NewObject(MUIC_Menuitem,
				MUIA_Menuitem_Title, GetString(MSG_MENU_LIST_EXPORT_SERVER),
			TAG_END)),

		TAG_END),

		MUIA_Group_Child, (tools_menu[0] = MUI_NewObject(MUIC_Menu,
			MUIA_Menu_Title, GetString(MSG_MENU_TOOLS_TITLE),
			MUIA_Group_Child, (tools_menu[3] = MUI_NewObject(MUIC_Menuitem,
				MUIA_Menuitem_Title, GetString(MSG_MENU_TOOLS_HISTORY),
				MUIA_Menuitem_Shortcut, "H",
			TAG_END)),
			MUIA_Group_Child, (tools_menu[1] = MUI_NewObject(MUIC_Menuitem,
				MUIA_Menuitem_Title, GetString(MSG_MENU_TOOLS_MODULES_CONSOLE),
				MUIA_Menuitem_Shortcut, "M",
			TAG_END)),
			MUIA_Group_Child, (tools_menu[2] = MUI_NewObject(MUIC_Menuitem,
				MUIA_Menuitem_Title, GetString(MSG_MENU_TOOLS_FTP_PUT),
			TAG_END)),
		TAG_END)),

		MUIA_Group_Child, MUI_NewObject(MUIC_Menu,
			MUIA_Menu_Title, GetString(MSG_MENU_PREFSMENU_TITLE),

			MUIA_Group_Child, (prefs_menu[0] = MUI_NewObject(MUIC_Menuitem,
				MUIA_Menuitem_Title, GetString(MSG_MENU_PREFSMENU_APP),
				MUIA_Menuitem_Shortcut, "P",
			TAG_END)),

			MUIA_Group_Child, MUI_NewObject(MUIC_Menuitem,
				MUIA_Menuitem_Title, NM_BARLABEL,
			TAG_END),

			MUIA_Group_Child, (prefs_menu[1] = MUI_NewObject(MUIC_Menuitem,
				MUIA_Menuitem_Title, GetString(MSG_MENU_PREFSMENU_MUI),
			TAG_END)),

		TAG_END),

	TAG_END);

	return obj;
}

static IPTR ApplicationNew(Class *cl, Object *obj, struct opSet *msg)
{
	/* first create a prefs window to avoid problems with global pointer to it. */
	Object *prefs_window = NewObject(PrefsWindowClass->mcc_Class, NULL, TAG_END);
	Object *about_window = MUI_NewObject(MUIC_Aboutbox, 
		MUIA_Aboutbox_Credits, APP_ABOUT,
#ifdef __SVNVERSION__
		MUIA_Aboutbox_Build, __SVNVERSION__,
#endif
	TAG_END);
	Object *desc_window = NewObject(DescWindowClass->mcc_Class, NULL, TAG_END);
	Object *main_window, *talk_window, *edit_con_window, *modules_log_window, *history_window;
	Object *app_menu[3], *contact_list_menu[8], *prefs_menu[2], *tools_menu[4];
	static STRPTR used_classes[] = {"Hyperlink.mcc", "Lamp.mcc", "TextEditor.mcc", "Busy.mcc", NULL};

	obj = DoSuperNew(cl, obj,
		MUIA_Application_Author, APP_AUTHOR,
		MUIA_Application_Base, APP_BASE,
		MUIA_Application_Copyright, "©" APP_CYEARS " " "BlaBla group",
		MUIA_Application_Description, APP_DESC,
		MUIA_Application_Title, APP_NAME,
		MUIA_Application_Version, APP_VER,
		MUIA_Application_BrokerPri, 127,
		MUIA_Application_UsedClasses, used_classes,
		MUIA_Application_Commands, IpcCommands,
		MUIA_Application_Menustrip, CreateMenuStrip(app_menu, contact_list_menu, prefs_menu, tools_menu),
		MUIA_Application_Window, about_window,
		MUIA_Application_Window, (history_window = NewObject(HistoryWindowClass->mcc_Class, NULL, TAG_END)),
		MUIA_Application_Window, (main_window = NewObject(MainWindowClass->mcc_Class, NULL, TAG_END)),
		MUIA_Application_Window, prefs_window,
		MUIA_Application_Window, (talk_window = NewObject(TalkWindowClass->mcc_Class, NULL, TAG_END)),
		MUIA_Application_Window, (edit_con_window = NewObject(EditContactWindowClass->mcc_Class, NULL, TAG_END)),
		MUIA_Application_Window, desc_window,
		MUIA_Application_Window, (modules_log_window = NewObject(ModulesLogWindowClass->mcc_Class, NULL, TAG_END)),
	TAG_MORE, msg->ops_AttrList);

	if(obj)
	{
		struct ApplicationData *d = INST_DATA(cl, obj);

		d->about_window = about_window;
		d->main_window = main_window;
		d->prefs_window = prefs_window;
		d->talk_window = talk_window;
		d->desc_window = desc_window;
		d->edit_con_window = edit_con_window;
		d->modules_log_window = modules_log_window;
		d->tools_menu = tools_menu[0];
		d->history_window = history_window;

		d->sec_ihn.ihn_Flags = MUIIHNF_TIMER;
		d->sec_ihn.ihn_Method = APPM_SecTrigger;
		d->sec_ihn.ihn_Object = obj;
		d->sec_ihn.ihn_Millis = 1000;

		DoMethod(obj, MUIM_Application_AddInputHandler, &d->sec_ihn);

		DoMethod(prefs_object(USD_PREFS_WINDOW_CANCEL), MUIM_Notify, MUIA_Pressed, FALSE, obj, 2,
		 MUIM_Application_Load, MUIV_Application_Load_ENV);
		DoMethod(d->prefs_window, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, obj, 2,
		 MUIM_Application_Load, MUIV_Application_Load_ENV);

		if((d->icon = GetDiskObject("PROGDIR:kwakwa")))
		{
			STRPTR time_fix, ignore_modules;

			if((time_fix = FindToolType(d->icon->do_ToolTypes, "TIME_FIX")))
			{
				LONG fix = 0;

				if(StrToLong(time_fix, &fix))
					SetTimeFixValue(fix);
			}

			if(FindToolType(d->icon->do_ToolTypes, "DISABLE_AUTOCONNECT"))
			{
				d->disable_autoconnect = TRUE;
				set(prefs_object(USD_PREFS_PROGRAM_CONNECT_AUTO_GROUP), MUIA_Disabled, TRUE);
			}

			if((ignore_modules = FindToolType(d->icon->do_ToolTypes, "IGNORE_MODULES")))
				d->ignore_modules = ExplodeString(ignore_modules, ',', &d->ignore_modules_count);

			set(obj, MUIA_Application_DiskObject, d->icon);
		}

		DoMethod(main_window, MWM_Notifications);

		DoMethod(app_menu[0], MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime, about_window, 3,
		 MUIM_Set, MUIA_Window_Open, TRUE);
		DoMethod(app_menu[1], MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime, obj, 2,
		 MUIM_Application_AboutMUI, main_window);
		DoMethod(app_menu[2], MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime, obj, 2,
		 MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

		DoMethod(prefs_menu[0], MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime, prefs_window, 3,
		 MUIM_Set, MUIA_Window_Open, TRUE);
		DoMethod(prefs_menu[1], MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime, obj, 3,
		 MUIM_Application_OpenConfigWindow, 0, NULL);

		DoMethod(contact_list_menu[0], MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime, edit_con_window, 2, ECWM_EditContact, NULL);
		DoMethod(contact_list_menu[1], MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime, main_window, 3, MUIM_Set, MUIA_Window_Open, TRUE);
		DoMethod(contact_list_menu[1], MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime, findobj(USD_MAIN_WINDOW_SEARCH_GROUP, main_window), 3,
		 MUIM_Set, MUIA_ShowMe, TRUE);
		DoMethod(contact_list_menu[1], MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime, main_window, 3,
		 MUIM_Set, MUIA_Window_Activate, TRUE);
		DoMethod(contact_list_menu[2], MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime, findobj(USD_CONTACTS_LIST, d->main_window), 1,
		 CLSM_Clear);
		DoMethod(contact_list_menu[3], MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime, findobj(USD_CONTACTS_LIST, d->main_window), 1,
		 CLSM_RemoveClones);
		DoMethod(contact_list_menu[4], MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime, findobj(USD_CONTACTS_LIST, d->main_window), 1,
		 CLSM_ImportFromFile);
		DoMethod(contact_list_menu[5], MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime, findobj(USD_CONTACTS_LIST, d->main_window), 1,
		 CLSM_ExportToFile);
		DoMethod(contact_list_menu[6], MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime, obj, 1, APPM_ImportList);
		DoMethod(contact_list_menu[7], MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime, obj, 1, APPM_ExportList);

		DoMethod(tools_menu[1], MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime, d->modules_log_window, 3, MUIM_Set, MUIA_Window_Open, TRUE);

		DoMethod(tools_menu[2], MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime, obj, 1, APPM_FtpPutActiveTab);

		DoMethod(tools_menu[3], MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime, d->history_window, 3, MUIM_Set, MUIA_Window_Open, TRUE);

		DoMethod(obj, MUIM_Notify, APPA_ScreenbarUnread, MUIV_EveryTime, MUIV_Notify_Self, 1, APPM_ScreenbarUnread);
		DoMethod(d->prefs_window, MUIM_Notify, PWA_PrefsChanged, TRUE, obj, 1, APPM_RemoveBroker);
		DoMethod(d->prefs_window, MUIM_Notify, PWA_PrefsChanged, TRUE, obj, 1, APPM_InstallBroker);

		if((d->available_pic = LoadPictureFile("PROGDIR:gfx/available.mbr")))
		{
			if((d->away_pic = LoadPictureFile("PROGDIR:gfx/away.mbr")))
			{
				if((d->dnd_pic = LoadPictureFile("PROGDIR:gfx/dnd.mbr")))
				{
					if((d->ffc_pic = LoadPictureFile("PROGDIR:gfx/ffc.mbr")))
					{
						if((d->invisible_pic = LoadPictureFile("PROGDIR:gfx/invisible.mbr")))
						{
							if((d->unavailable_pic = LoadPictureFile("PROGDIR:gfx/unavailable.mbr")))
							{
								if((d->newmsg_pic = LoadPictureFile("PROGDIR:gfx/newmsg.mbr")))
								{
									if((d->sctl = AllocMem(sizeof(struct SBarControl), MEMF_PUBLIC | MEMF_CLEAR)))
									{
										InitSemaphore(&d->sctl->semaphore);
										d->sctl->app = obj;
										d->sctl->actPic = d->unavailable_pic;
										d->sctl->unreadPic = d->newmsg_pic;

										SmallSBarClass->mcc_Class->cl_ID = (UBYTE*)APP_NAME;
										SmallSBarClass->mcc_Class->cl_UserData = (ULONG)d->sctl;
										if(ScreenbarControl(SBCT_InstallPlugin, (ULONG)SmallSBarClass, TAG_DONE))
										{
											if((d->slave_object = NewObject(SlaveProcessClass, NULL, TAG_END)))
											{
												d->slave_mask = 1UL << (UBYTE)xget(d->slave_object, SPA_SigBit);

												if((d->clipboard_port = CreateMsgPort()))
												{
													if((d->clipboard_request = (struct IOClipReq*)CreateExtIO(d->clipboard_port, sizeof(struct IOClipReq))))
													{
														if(!OpenDevice("clipboard.device", 0, (struct IORequest*)d->clipboard_request, 0))
														{
															d->broker_signal = -1;

															return (IPTR)obj;
														}
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	CoerceMethod(cl, obj, OM_DISPOSE);
	return (IPTR)NULL;
}

static IPTR ApplicationDispose(Class *cl, Object *obj, Msg msg)
{
	struct ApplicationData *d = INST_DATA(cl, obj);
	ENTER();

	DoMethod(obj, MUIM_Application_RemInputHandler, &d->sec_ihn);

	ScreenbarControl(SBCT_UninstallPlugin, (ULONG)SmallSBarClass, TAG_DONE);

	if(d->sctl)
		FreeMem(d->sctl, sizeof(struct SBarControl));

	/* it's safe to call FreePicture with NULL ptr */
	FreePicture(d->available_pic);
	FreePicture(d->away_pic);
	FreePicture(d->dnd_pic);
	FreePicture(d->ffc_pic);
	FreePicture(d->invisible_pic);
	FreePicture(d->unavailable_pic);
	FreePicture(d->newmsg_pic);

	if(d->icon) FreeDiskObject(d->icon);

	if(d->clipboard_request)
	{
		CloseDevice((struct IORequest*)d->clipboard_request);
		DeleteExtIO((struct IORequest*)d->clipboard_request);
	}
	if(d->clipboard_port)
		DeleteMsgPort(d->clipboard_port);

	if(d->slave_object)
		DisposeObject(d->slave_object);

	if(d->last_desc)
		StrFree(d->last_desc);

	if(d->ignore_modules)
	{
		LONG i;

		for(i = 0; i < d->ignore_modules_count; i++)
		{
			if(d->ignore_modules[i])
				StrFree(d->ignore_modules[i]);
		}

		FreeVec(d->ignore_modules);
	}

	LEAVE();
	return DoSuperMethodA(cl, obj, msg);
}

static IPTR ApplicationSet(Class *cl, Object *obj, struct opSet *msg)
{
	int tagcount = 0;
	struct TagItem *tag = 0, *tagptr = msg->ops_AttrList;

	while((tag = NextTagItem(&tagptr)))
	{
		switch(tag->ti_Tag)
		{
			case APPA_ScreenbarUnread:
				tagcount++;
			break;
		}
	}

	tagcount += DoSuperMethodA(cl, obj, (Msg)msg);
	return tagcount;
}

static IPTR ApplicationGet(Class *cl, Object *obj, struct opGet *msg)
{
	struct ApplicationData *d = INST_DATA(cl, obj);
	int rv = FALSE;

	switch(msg->opg_AttrID)
	{
		case APPA_ScreenbarUnread:
		return TRUE;

		case APPA_Status:
			*msg->opg_Storage = d->last_status;
		return TRUE;

		case APPA_Description:
			*msg->opg_Storage = (IPTR)d->last_desc;
		return TRUE;

		default:
			rv = (DoSuperMethodA(cl, obj, (Msg)msg));
	}

	return rv;
}

static IPTR ApplicationSetup(Class *cl, Object *obj)
{
	struct ApplicationData *d = INST_DATA(cl, obj);
	BPTR fh;
	ENTER();

	if(DoMethod(obj, APPM_OpenModules) > 0)
	{
		DoMethod(findobj(USD_CONTACTS_LIST, d->main_window), CLSM_ReadList);
		DoMethod(obj, MUIM_Application_Load, MUIV_Application_Load_ENV);

		/* open main window if user want that */
		if((BOOL)xget(prefs_object(USD_PREFS_PROGRAM_MAIN_WINDOW_SHOW_START), MUIA_Selected))
			set(d->main_window, MUIA_Window_Open, TRUE);

		if((fh = Open(LAST_STATUS_FILE, MODE_OLDFILE))) /* read last status set before exit */
		{
			ULONG len;

			FRead(fh, &d->last_status, sizeof(ULONG), 1);
			FRead(fh, &len, sizeof(ULONG), 1);

			if(len > 0 && (d->last_desc = AllocVec(len + 1, MEMF_ANY)))
			{
				FRead(fh, d->last_desc, len, 1);
				d->last_desc[len] = 0x00;
			}
			Close(fh);
		}

		/* autoconnect */
		if((BOOL)xget(prefs_object(USD_PREFS_PROGRAM_CONNECT_AUTO_CHECK), MUIA_Selected) && !d->disable_autoconnect)
		{
			if(xget(prefs_object(USD_PREFS_PROGRAM_CONNECT_AUTO_TYPE), MUIA_Cycle_Active) == 0)
			{
				ULONG status = KWA_STATUS_AVAIL;
				ULONG active = xget(prefs_object(USD_PREFS_PROGRAM_CONNECT_AUTO_CYCLE), MUIA_Cycle_Active);

				switch(active)
				{
					case 1:
						status = KWA_STATUS_BUSY;
					break;

					case 2:
						status = KWA_STATUS_INVISIBLE;
					break;

					case 3:
						status = KWA_STATUS_FFC;
					break;

					case 4:
						status = KWA_STATUS_DND;
					break;
				}

				DoMethod(obj, APPM_Connect, status, xget(prefs_object(USD_PREFS_PROGRAM_CONNECT_AUTO_STRING), MUIA_String_Contents));
			}
			else
			{
				if(d->last_status == KWA_STATUS_NOT_AVAIL)
				{
					switch(xget(prefs_object(USD_PREFS_PROGRAM_LASTNOTAVAIL_CYCLE), MUIA_Cycle_Active))
					{
						case 1:
							DoMethod(obj, APPM_Connect, KWA_STATUS_AVAIL, d->last_desc);
						break;

						case 2:
							DoMethod(obj, APPM_Connect, KWA_STATUS_BUSY, d->last_desc);
						break;

						case 3:
							DoMethod(obj, APPM_Connect, KWA_STATUS_INVISIBLE, d->last_desc);
						break;

						case 4:
							DoMethod(obj, APPM_Connect, KWA_STATUS_FFC, d->last_desc);
						break;

						case 5:
							DoMethod(obj, APPM_Connect, KWA_STATUS_DND, d->last_desc);
						break;
					}
				}
				else
					DoMethod(obj, APPM_Connect, d->last_status, d->last_desc);
			}
		}
		DoMethod(obj, APPM_InstallBroker);

		/* will create history database if not exists */
		DoMethod(obj, APPM_OpenHistoryDatabase);

		LEAVE();
		return (IPTR)TRUE;
	}

	LEAVE();
	return (IPTR)FALSE;
}

static IPTR ApplicationCleanup(Class *cl, Object *obj)
{
	struct ApplicationData *d = INST_DATA(cl, obj);
	STRPTR unread_list;
	ENTER();

	/* check if we have unread messages */
	if((unread_list = (STRPTR)DoMethod(findobj(USD_CONTACTS_LIST, d->main_window), CLSM_CheckUnread)) != NULL)
	{
		/* we have unread messages, so we ask about exiting */
		LONG req = MUI_Request(obj, d->main_window, 0L, APP_NAME, GetString(MSG_KWAKWAEXIT_UNREAD_BUTTONS), GetString(MSG_KWAKWAEXIT_UNREAD_MSG), unread_list);

		StrFree(unread_list);

		if(req == 0)
		{
			/* user doesn't want to exit, so we call MainLoop once again */
			DoMethod(obj, APPM_MainLoop);
		}
	}

	set(d->main_window, MUIA_Window_Open, FALSE);
	set(d->talk_window, MUIA_Window_Open, FALSE);
	set(d->prefs_window, MUIA_Window_Open, FALSE);

	DoMethod(obj, APPM_Disconnect, d->last_desc);
	DoMethod(d->slave_object, SPM_KillProcess);

	DoMethod(obj, APPM_RemoveBroker);
	DoMethod(obj, APPM_CloseModules);
	DoMethod(obj, APPM_CloseHistoryDatabase);

	LEAVE();
	return (IPTR)0;
}

static IPTR ApplicationClipboardStart(Class *cl, Object *obj, struct APPP_ClipboardStart *msg)
{
	struct ApplicationData *d = INST_DATA(cl, obj);
	BOOL odd = msg->length & 1;
	ULONG tiff_len = odd ? msg->length + 13 : msg->length + 12;
	ENTER();

	if(odd)
		d->add_byte = TRUE;

	d->clipboard_request->io_Offset = 0;
	d->clipboard_request->io_ClipID = 0;
	d->clipboard_request->io_Error = 0;

	d->clipboard_request->io_Data = (APTR)"FORM";
	d->clipboard_request->io_Length = 4;
	d->clipboard_request->io_Command = CMD_WRITE;

	if(DoIO((struct IORequest*)d->clipboard_request))
		return (IPTR)FALSE;

	d->clipboard_request->io_Data = (APTR)&tiff_len;
	d->clipboard_request->io_Length = 4;
	d->clipboard_request->io_Command = CMD_WRITE;

	if(DoIO((struct IORequest*)d->clipboard_request))
		return (IPTR)FALSE;

	d->clipboard_request->io_Data = (APTR)"FTXT";
	d->clipboard_request->io_Length = 4;
	d->clipboard_request->io_Command = CMD_WRITE;

	if(DoIO((struct IORequest*)d->clipboard_request))
		return (IPTR)FALSE;

	d->clipboard_request->io_Data = (APTR)"CHRS";
	d->clipboard_request->io_Length = 4;
	d->clipboard_request->io_Command = CMD_WRITE;

	if(DoIO((struct IORequest*)d->clipboard_request))
		return (IPTR)FALSE;

	d->clipboard_request->io_Data = (APTR)&msg->length;
	d->clipboard_request->io_Length = 4;
	d->clipboard_request->io_Command = CMD_WRITE;


	if(DoIO((struct IORequest*)d->clipboard_request))
		return (IPTR)FALSE;

	LEAVE();
	return (IPTR)TRUE;
}

static IPTR ApplicationClipboardWrite(Class *cl, Object *obj, struct APPP_ClipboardWrite *msg)
{
	struct ApplicationData *d = INST_DATA(cl, obj);

	if(msg->length == -1)
		msg->length = StrLen(msg->data);

	d->clipboard_request->io_Data = (APTR)msg->data;
	d->clipboard_request->io_Length = msg->length;
	d->clipboard_request->io_Command = CMD_WRITE;

	if(DoIO((struct IORequest*)d->clipboard_request))
		return (IPTR)FALSE;

	return (IPTR)TRUE;
}

static IPTR ApplicationClipboardEnd(Class *cl, Object *obj)
{
	struct ApplicationData *d = INST_DATA(cl, obj);

	if(d->add_byte)
	{
		d->clipboard_request->io_Data = (APTR)"\0";
		d->clipboard_request->io_Length = 1UL;
		d->clipboard_request->io_Command = CMD_WRITE;

		if(DoIO((struct IORequest*)d->clipboard_request))
			return (IPTR)FALSE;

		d->add_byte = FALSE;
	}

	d->clipboard_request->io_Data = NULL;
	d->clipboard_request->io_Length = 0UL;
	d->clipboard_request->io_Command = CMD_UPDATE;

	if(DoIO((struct IORequest*)d->clipboard_request))
		return (IPTR)FALSE;

	return (IPTR)TRUE;
}

static IPTR ApplicationInstallBroker(Class *cl, Object *obj)
{
	struct ApplicationData *d = INST_DATA(cl, obj);
	CxObj *broker = (CxObj*)xget(obj, MUIA_Application_Broker);

	d->broker_start_time = d->broker_act_time = xget(prefs_object(USD_PREFS_PROGRAM_AUTOAWAY_SLIDER), MUIA_Slider_Level);
	d->broker_status_away = FALSE;

	if(broker && d->broker_signal == -1 && d->broker_start_time)
	{
		if((d->broker_signal = AllocSignal(-1)) != -1)
		{
			struct InputXpression ix =
			{
				IX_VERSION,
				IECLASS_RAWKEY,
				0x00,
				0x0000,
				0x00,
				0xFFFF & ~(IEQUALIFIER_LALT | IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT | IEQUALIFIER_CAPSLOCK | IEQUALIFIER_RELATIVEMOUSE),
				IXSYM_CAPS
			};
			CxObj *sigobj_key, *sigobj_mouse;

			if((sigobj_key = CxSignal(FindTask(NULL), d->broker_signal)))
			{
				if((sigobj_mouse = CxSignal(FindTask(NULL), d->broker_signal)))
				{
					if((d->keyboard_filter = CxFilter("")))
					{
						if((d->mouse_filter = CxFilter("")))
						{
							SetFilterIX(d->keyboard_filter, &ix);
							ix.ix_Class = IECLASS_RAWMOUSE;
							SetFilterIX(d->mouse_filter, &ix);

							if(!CxObjError(d->keyboard_filter) || !CxObjError(d->mouse_filter))
							{
								AttachCxObj(broker, d->keyboard_filter);
								AttachCxObj(d->keyboard_filter, sigobj_key);
								AttachCxObj(broker, d->mouse_filter);
								AttachCxObj(d->mouse_filter, sigobj_mouse);

								return (IPTR)TRUE;
							}
						}
					}
					RemoveCxObj(sigobj_mouse);
				}
				RemoveCxObj(sigobj_key);
			}
		}
	}
	return (IPTR)FALSE;
}

static IPTR ApplicationRemoveBroker(Class *cl, Object *obj)
{
	struct ApplicationData *d = INST_DATA(cl, obj);

	if(d->keyboard_filter)
		RemoveCxObj(d->keyboard_filter);
	if(d->mouse_filter)
		RemoveCxObj(d->mouse_filter);

	if(d->broker_signal != -1)
		FreeSignal(d->broker_signal);

	d->keyboard_filter = d->mouse_filter = NULL;
	d->broker_signal = -1;

	if(d->broker_old_desc)
		StrFree(d->broker_old_desc);
	d->broker_old_desc = NULL;

	return (IPTR)0;
}

static IPTR ApplicationAutoAway(Class *cl, Object *obj)
{
	struct ApplicationData *d = INST_DATA(cl, obj);
	STRPTR new_desc = (STRPTR)xget(prefs_object(USD_PREFS_PROGRAM_AUTOAWAY_STRING), MUIA_String_Contents);
	ULONG status = xget(prefs_object(USD_PREFS_PROGRAM_AUTOAWAY_CYCLE), MUIA_Cycle_Active);

	d->broker_old_status = d->last_status;
	d->broker_old_desc = StrNew(d->last_desc);

	if(!new_desc || StrLen(new_desc) == 0)
		new_desc = d->last_desc;

	switch(status)
	{
		case 0:
			status = KWA_STATUS_AVAIL;
		break;

		case 1:
			status = KWA_STATUS_BUSY;
		break;

		case 2:
			status = KWA_STATUS_INVISIBLE;
		break;

		case 3:
			status = KWA_STATUS_FFC;
		break;

		case 4:
			status = KWA_STATUS_DND;
		break;
	}

	DoMethod(obj, APPM_ChangeStatus, status, new_desc);

	return (IPTR)0;
}

static IPTR ApplicationAutoBack(Class *cl, Object *obj)
{
	struct ApplicationData *d = INST_DATA(cl, obj);

	DoMethod(obj, APPM_ChangeStatus, d->broker_old_status, d->broker_old_desc);

	StrFree(d->broker_old_desc);
	d->broker_old_desc = NULL;

	return (IPTR)0;
}

static IPTR ApplicationMainLoop(Class *cl, Object *obj)
{
	struct ApplicationData *d = INST_DATA(cl, obj);
	struct Module *m;
	BOOL running = TRUE;
	fd_set wfdset;
	fd_set rfdset;
	struct timeval timeout;

	timeout.tv_secs = 50;
	timeout.tv_micro = 0;

	while(running)
	{
		ULONG signals = 0, signals2;
		LONG max_socket = -1;
		LONG retval;
		ULONG broker_mask = d->broker_signal == -1 ? 0UL : 1UL << d->broker_signal;

		FD_ZERO(&wfdset);
		FD_ZERO(&rfdset);

		if(DoMethod(obj, MUIM_Application_NewInput, &signals) == MUIV_Application_ReturnID_Quit)
			running = FALSE;

		ForeachNode(&d->modules, m)
		{
			LONG act_soc = (LONG)xget(m->mod_Object, KWAA_Socket);

			if(act_soc >= 0)
			{
				if(max_socket < act_soc)
					max_socket = act_soc;

				if((BOOL)xget(m->mod_Object, KWAA_WantRead))
					FD_SET(act_soc, &rfdset);
				if((BOOL)xget(m->mod_Object, KWAA_WantWrite))
					FD_SET(act_soc, &wfdset);
			}
		}

		signals2 = signals;

		signals |= SIGBREAKF_CTRL_C | d->slave_mask | broker_mask;

		if(max_socket == -1)
		{
			retval = 0;
			signals = Wait(signals);
		}
		else
			retval = WaitSelect(max_socket + 1, &rfdset, &wfdset, NULL, (APTR)&timeout, &signals);

		if(DoMethod(obj, MUIM_Application_NewInput, &signals2) == MUIV_Application_ReturnID_Quit)
			running = FALSE;

		if(retval >= 0)
		{
			ForeachNode(&d->modules, m)
			{
				struct MinList *events;
				BOOL can_read = FALSE, can_write = FALSE;
				LONG act_soc = (LONG)xget(m->mod_Object, KWAA_Socket);

				if(act_soc >= 0)
				{
					if(FD_ISSET(act_soc, &rfdset))
						can_read = TRUE;
					if(FD_ISSET(act_soc, &wfdset))
						can_write = TRUE;
				}

				if((events = (struct MinList*)DoMethod(m->mod_Object, KWAM_WatchEvents, (ULONG)can_read, (ULONG)can_write)))
				{
					struct KwaEvent *e;

					ForeachNode(events, e)
					{
						DoMethod(obj, APPM_ParseEvent, (IPTR)m, (IPTR)e);
					}

					DoMethod(m->mod_Object, KWAM_FreeEvents, (IPTR)events);
				}
			}
		}
		else if(retval < 0)
			running = FALSE;

		if(signals & SIGBREAKF_CTRL_C)
			running = FALSE;

		if(signals & d->slave_mask)
			DoMethod(d->slave_object, SPM_SignalMethod);

		if(signals & broker_mask)
		{
			if(d->broker_status_away)
				DoMethod(obj, APPM_AutoBack);
			d->broker_act_time = d->broker_start_time;
			d->broker_status_away = FALSE;
		}
	}

	return (IPTR)0;

}

static IPTR ApplicationScreenbarChange(Class *cl, Object *obj, struct APPP_ScreenbarChange *msg)
{
	struct ApplicationData *d = INST_DATA(cl, obj);

	ObtainSemaphore(&d->sctl->semaphore);

	if(d->sctl->sbarTask != NULL)
	{
		if(KWA_S_AVAIL(msg->new_status))/* here, we do it only once. if it would be in sbar class we had to do it once for each instance of sbar */
			d->sctl->actPic = d->available_pic;
		else if(KWA_S_NAVAIL(msg->new_status))
			d->sctl->actPic = d->unavailable_pic;
		else if(KWA_S_BUSY(msg->new_status))
			d->sctl->actPic = d->away_pic;
		else if(KWA_S_INVISIBLE(msg->new_status))
			d->sctl->actPic = d->invisible_pic;
		else if(KWA_S_FFC(msg->new_status))
			d->sctl->actPic = d->ffc_pic;
		else if(KWA_S_DND(msg->new_status))
			d->sctl->actPic = d->dnd_pic;

		if(d->sctl->sbarTask)
			Signal(d->sctl->sbarTask, d->sctl->sbarSignal);
	}

	ReleaseSemaphore(&d->sctl->semaphore);
	return (IPTR)1;
}

static IPTR ApplicationScreenbarMenu(Class *cl, Object *obj, struct APPP_ScreenbarMenu *msg)
{
	struct ApplicationData *d = INST_DATA(cl, obj);

	if(msg->menu_result < SBR_MENU_TALK_TAB)
	{
		switch(msg->menu_result)
		{
			case SBR_MENU_SHOW_LIST:
				set(d->main_window, MUIA_Window_Open, !xget(d->main_window, MUIA_Window_Open));
			break;

			case SBR_MENU_STATUS_AVAILABLE:
				DoMethod(obj, APPM_ChangeStatus, KWA_STATUS_AVAIL, NULL);
			break;

			case SBR_MENU_STATUS_AWAY:
				DoMethod(obj, APPM_ChangeStatus, KWA_STATUS_BUSY, NULL);
			break;

			case SBR_MENU_STATUS_INVISIBLE:
				DoMethod(obj, APPM_ChangeStatus, KWA_STATUS_INVISIBLE, NULL);
			break;

			case SBR_MENU_STATUS_FFC:
				DoMethod(obj, APPM_ChangeStatus, KWA_STATUS_FFC, NULL);
			break;

			case SBR_MENU_STATUS_DND:
				DoMethod(obj, APPM_ChangeStatus, KWA_STATUS_DND, NULL);
			break;

			case SBR_MENU_STATUS_UNAVAILABLE:
				DoMethod(obj, APPM_ChangeStatus, KWA_STATUS_NOT_AVAIL, NULL);
			break;

			case SBR_MENU_STATUS_DESCRIPTION:
				set(d->desc_window, MUIA_Window_Open, TRUE);
			break;

			case SBR_MENU_QUIT:
				DoMethod(obj, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
			break;
		}
	}
	else
	{
		DoMethod(d->talk_window, TKWM_OpenOnTabById, msg->menu_result - SBR_MENU_TALK_TAB);
		DoMethod(d->talk_window, MUIM_Window_ScreenToFront);
	}

	return (IPTR)1;
}

static IPTR ApplicationScreenbarScreenbarize(Class *cl, Object *obj)
{
	struct ApplicationData *d = INST_DATA(cl, obj);

	set(d->main_window, MUIA_Window_Open, !xget(d->main_window, MUIA_Window_Open));

	return (IPTR)1;
}

static IPTR ApplicationScreenbarUnread(Class *cl, Object *obj)
{
	struct ApplicationData *d = INST_DATA(cl, obj);
	struct ContactEntry *fst;
	ENTER();

	DoMethod(findobj(USD_CONTACTS_LIST, d->main_window), CLSM_GetEntry, 0, &fst);

	if(fst)
	{
		ObtainSemaphore(&d->sctl->semaphore);

		if(fst->unread)
			d->sctl->unread = TRUE;
		else
			d->sctl->unread = FALSE;

		if(d->sctl->sbarTask)
			Signal(d->sctl->sbarTask, d->sctl->sbarSignal);

		ReleaseSemaphore(&d->sctl->semaphore);
	}

	LEAVE();
	return (IPTR)1;
}


static IPTR ApplicationNotifyBeacon(Class *cl, Object *obj, struct APPP_NotifyBeacon *msg)
{
	struct ApplicationData *d = INST_DATA(cl, obj);

	DoMethod(d->slave_object, SPM_SendNotification, msg->notification_name, msg->message, msg->wait_for_result, msg->obj, msg->method, msg->usr_data);

	return (IPTR)0;
}

static IPTR ApplicationSendHttpGet(Class *cl, Object *obj, struct APPP_SendHttpGet *msg)
{
	struct ApplicationData *d = INST_DATA(cl, obj);

	DoMethod(d->slave_object, SPM_SendHttpGet, msg->url, msg->user_agent, msg->obj, msg->method, msg->user_data);

	return (IPTR)0;
}

static IPTR ApplicationSendHttpPost(Class *cl, Object *obj, struct APPP_SendHttpPost *msg)
{
	struct ApplicationData *d = INST_DATA(cl, obj);

	DoMethod(d->slave_object, SPM_SendHttpPost, msg->url, msg->user_agent, msg->data, msg->items_no, msg->obj, msg->method, msg->user_data);

	return (IPTR)0;
}

static IPTR ApplicationFtpPut(Class *cl, Object *obj, struct APPP_FtpPut *msg)
{
	struct ApplicationData *d = INST_DATA(cl, obj);
	STRPTR host = (STRPTR)xget(prefs_object(USD_PREFS_FTP_HOST_STRING), MUIA_String_Contents);
	LONG port = (LONG)xget(prefs_object(USD_PREFS_FTP_PORT_STRING), MUIA_String_Integer);
	ULONG type = xget(prefs_object(USD_PREFS_FTP_LOGINTYPE_CYCLE), MUIA_Cycle_Active);
	STRPTR user, pass;
	APTR win = NULL;
	struct FileRequester *freq;
	ENTER();

	if((BOOL)xget(d->talk_window, MUIA_Window_Open))
		win = d->talk_window;

	if(!host || StrLen(host) <= 0)
	{
		if(MUI_Request(obj, win, 0, APP_NAME, GetString(MSG_FTP_ERROR_GADGETS), GetString(MSG_FTP_ERROR_HOST)) == 0)
		{
			set(prefs_object(USD_PREFS_PAGES_LIST), MUIA_List_Active, 4);
			set(d->prefs_window, MUIA_Window_Open, TRUE);
		}
		return (IPTR)0;
	}

	if(port <= 0)
	{
		if(MUI_Request(obj, win, 0, APP_NAME, GetString(MSG_FTP_ERROR_GADGETS), GetString(MSG_FTP_ERROR_PORT)) == 0)
		{
			set(prefs_object(USD_PREFS_PAGES_LIST), MUIA_List_Active, 4);
			set(d->prefs_window, MUIA_Window_Open, TRUE);
		}
		return (IPTR)0;
	}

	if(type == 0)
	{
		user = "Anonymous";
		pass = "kwakwa@morphos";
	}
	else
	{
		user = (STRPTR)xget(prefs_object(USD_PREFS_FTP_USER_STRING), MUIA_String_Contents);
		pass = (STRPTR)xget(prefs_object(USD_PREFS_FTP_PASSWORD_STRING), MUIA_String_Contents);

		if(!user || !pass)
		{
			if(MUI_Request(obj, win, 0, APP_NAME, GetString(MSG_FTP_ERROR_GADGETS), GetString(MSG_FTP_ERROR_USER_PASS)) == 0)
			{
				set(prefs_object(USD_PREFS_PAGES_LIST), MUIA_List_Active, 4);
				set(d->prefs_window, MUIA_Window_Open, TRUE);
			}
			return (IPTR)0;
		}
	}

	if((freq = MUI_AllocAslRequestTags(ASL_FileRequest, TAG_END)))
	{
		set(obj, MUIA_Application_Sleep, TRUE);
		if(MUI_AslRequestTags(freq,
			ASLFR_PrivateIDCMP, TRUE,
			ASLFR_TitleText, GetString(MSG_FTP_ASL_TITLE),
			ASLFR_PositiveText, GetString(MSG_FTP_ASL_POSITIVE),
			ASLFR_InitialPattern, "#?",
			ASLFR_DoPatterns, TRUE,
		TAG_END))
		{
			UBYTE location[500];
			BOOL passive = TRUE; /* TODO: checkbox for selection passive or active connection type */
			STRPTR rpath = NULL, rpath_prefs = (STRPTR)xget(prefs_object(USD_PREFS_FTP_SERVERPATH_STRING), MUIA_String_Contents);

			StrNCopy(freq->fr_Drawer, (STRPTR)location, 500);
			AddPart((STRPTR)location, freq->fr_File, 500);

			if(!rpath_prefs || StrLen(rpath_prefs) == 0)
			{
				rpath = FmtNew("/%ls", freq->fr_File);
			}
			else
			{
				if(rpath_prefs[StrLen(rpath_prefs) - 1] == '/')
				{
					rpath = FmtNew("%ls%ls", rpath_prefs, freq->fr_File);
				}
				else
				{
					rpath = FmtNew("%ls/%ls", rpath_prefs, freq->fr_File);
				}
			}

			if(rpath)
			{
				struct InsertLinkParms *p;

				if((p = AllocMem(sizeof(struct InsertLinkParms), MEMF_ANY)))
				{
					p->contactid = StrNew(msg->contactid);
					p->pluginid = msg->pluginid;

					if(rpath[0] == '/')
						p->link = FmtNew("%ls%ls", host, rpath);
					else
						p->link = FmtNew("%ls/%ls", host, rpath);

					DoMethod(d->slave_object, SPM_SendFtpPut, host, port, rpath, location, passive, user, pass, obj, APPM_FtpPutCallback, p);
				}
				FmtFree(rpath);
			}
		}
		set(obj, MUIA_Application_Sleep, FALSE);
		MUI_FreeAslRequest(freq);
	}

	LEAVE();
	return (IPTR)0;
}

static IPTR ApplicationFtpPutActiveTab(Class *cl, Object *obj)
{
	Object *tab;
	struct ApplicationData *d = INST_DATA(cl, obj);

	DoMethod(d->talk_window, TKWM_GetTab, TKWV_ActiveTab, &tab);

	if(tab)
	{
		struct ContactEntry *con = (struct ContactEntry*)xget(tab, TTBA_ContactEntry);

		if(con)
			DoMethod(obj, APPM_FtpPut, con->pluginid, con->entryid);
	}
	else
		MUI_Request(obj, xget(d->main_window, MUIA_Window_Open) ? d->main_window : NULL, 0, APP_NAME,
		 GetString(MSG_FTP_NO_ACTIVE_TAB_GADGETS), GetString(MSG_FTP_NO_ACTIVE_TAB));

	return (IPTR)0;
}

static IPTR ApplicationFtpPutCallback(Class *cl, Object *obj, struct APPP_FtpPutCallback *msg)
{
	struct ApplicationData *d = INST_DATA(cl, obj);
	Object *win = NULL;
	ULONG request_return = -1;
	ENTER();

	if((BOOL)xget(d->main_window, MUIA_Window_Open))
		win = d->main_window;

	if((BOOL)xget(d->talk_window, MUIA_Window_Open))
		win = d->talk_window;

	switch(msg->error_code)
	{
		case FTP_ERROR_CONNECT:
			request_return = MUI_Request(obj, win, 0, APP_NAME, GetString(MSG_FTP_ERROR_GADGETS), GetString(MSG_FTP_ERROR_CONNECT));
		break;

		case FTP_ERROR_LIBRARY:
			request_return = MUI_Request(obj, win, 0, APP_NAME, GetString(MSG_FTP_ERROR_GADGETS), GetString(MSG_FTP_ERROR_LIBRARY));
		break;

		case FTP_ERROR_LOGIN:
			request_return = MUI_Request(obj, win, 0, APP_NAME, GetString(MSG_FTP_ERROR_GADGETS), GetString(MSG_FTP_ERROR_LOGIN));
		break;

		case FTP_ERROR_PASSIVE:
			request_return = MUI_Request(obj, win, 0, APP_NAME, GetString(MSG_FTP_ERROR_GADGETS), GetString(MSG_FTP_ERROR_PASSIVE));
		break;

		case FTP_ERROR_SEND:
			request_return = MUI_Request(obj, win, 0, APP_NAME, GetString(MSG_FTP_ERROR_GADGETS), GetString(MSG_FTP_ERROR_SEND));
		break;
	}

	if(request_return != -1)
	{
		set(prefs_object(USD_PREFS_PAGES_LIST), MUIA_List_Active, 4);
		set(d->prefs_window, MUIA_Window_Open, TRUE);
		return (IPTR)-1;
	}

	if(msg->parms)
	{
		if(msg->parms->contactid)
		{
			if(msg->parms->link)
			{
				STRPTR l = URLEncode(msg->parms->link);

				if(l)
				{
					DoMethod(d->talk_window, TKWM_SendMessage, msg->parms->contactid, msg->parms->pluginid, l);
					StrFree(l);
				}
				else
					DoMethod(d->talk_window, TKWM_SendMessage, msg->parms->contactid, msg->parms->pluginid, msg->parms->link);

				StrFree(msg->parms->link);
			}
			StrFree(msg->parms->contactid);
		}
		FreeMem(msg->parms, sizeof(struct InsertLinkParms));
	}

	LEAVE();
	return (IPTR)0;
}

static IPTR ApplicationOpenModules(Class *cl, Object *obj)
{
	struct ApplicationData *d = INST_DATA(cl, obj);
	BPTR lock;
	UBYTE path[255];
	ENTER();

	d->modules_no = 0;
	NEWLIST(&d->modules);

	if((lock = Lock((STRPTR) MODULES_DIR, ACCESS_READ)))
	{
		struct FileInfoBlock fib;

		Examine(lock, &fib);

		while(ExNext(lock, &fib))
		{
			if(fib.fib_DirEntryType < 0)
			{
				if(StrIStr((STRPTR)fib.fib_FileName, (STRPTR)".module"))
				{
					ULONG i;
					BOOL ignore = FALSE;
					struct Module *m;

					for(i = 0; i < d->ignore_modules_count; i++)
					{
						if(StrEqu((STRPTR)fib.fib_FileName, d->ignore_modules[i]))
						{
							ignore = TRUE;
							break;
						}
					}

					if(ignore)
						continue;

					FmtNPut(path, (STRPTR)MODULES_DIR "%s", sizeof(path), fib.fib_FileName);

					if((m = OpenModule(path, fib.fib_FileName, obj)))
					{
						ADDTAIL((struct List*)&d->modules, (struct Node*)m);
						d->modules_no++;
					}
				}
			}
		}
		UnLock(lock);
	}

	if(d->modules_no > 0)
	{
		/* here you can comunicate with modules before starting main loop */
		DoMethod(d->edit_con_window, ECWM_AddModulesCycle,  NewObject(ModulesCycleClass->mcc_Class, NULL, MCA_ModulesList, &d->modules, MCA_ModulesNo, d->modules_no, TAG_END));
		DoMethod(obj, APPM_AddModulesGui);
	}
	else
		MUI_Request(obj, NULL, 0, APP_NAME, GetString(MSG_APPLICATION_NO_PLUGINS_GADGETS), GetString(MSG_APPLICATION_NO_PLUGINS_MSG));

	LEAVE();
	return (IPTR)d->modules_no;
}

static IPTR ApplicationAddModulesGui(Class *cl, Object *obj)
{
	struct ApplicationData *d = INST_DATA(cl, obj);
	struct Module *m;

	DoMethod(d->tools_menu, MUIM_Group_InitChange);
	DoMethod(obj, MUIM_Group_InitChange);

	ForeachNode(&d->modules, m)
	{
		Object *menu = NULL;
		struct TagItem *gui_list = (struct TagItem*)xget(m->mod_Object, KWAA_GuiTagList);

		if(gui_list)
		{
			struct TagItem *tag, *tag_ptr = gui_list;

			while((tag = NextTagItem(&tag_ptr)))
			{
				switch(tag->ti_Tag)
				{
					case KWAG_PrefsEntry:
						DoMethod(prefs_object(USD_PREFS_PAGES_LIST), MUIM_List_InsertSingle, tag->ti_Data, MUIV_List_Insert_Bottom);
					break;

					case KWAG_PrefsPage:
						DoMethod(d->prefs_window, PWM_AddPrefsPage, tag->ti_Data);
					break;

					case KWAG_ToolsEntry:
						if(!menu)
						{
							if((menu = MUI_NewObject(MUIC_Menu,
								MUIA_Menu_Title, m->mod_Name,
							TAG_END)))
								DoMethod(menu, MUIM_Group_InitChange);
						}
						if(menu)
							DoMethod(menu, OM_ADDMEMBER, tag->ti_Data);
					break;

					case KWAG_Window:
						DoMethod(obj, OM_ADDMEMBER, tag->ti_Data);
						set((APTR)tag->ti_Data, MUIA_Window_ScreenTitle, APP_SCREEN_TITLE);
					break;
				}
			}
		}

		if(menu)
		{
			DoMethod(menu, MUIM_Group_ExitChange);
			DoMethod(d->tools_menu, OM_ADDMEMBER, menu);
		}
	}

	DoMethod(obj, MUIM_Group_ExitChange);
	DoMethod(d->tools_menu, MUIM_Group_ExitChange);

	return (IPTR)0;
}

static IPTR ApplicationGetModuleName(Class *cl, Object *obj, struct APPP_GetModuleName *msg)
{
	struct ApplicationData *d = INST_DATA(cl, obj);
	struct Module *m;

	ForeachNode(&d->modules, m)
	{
		if(m->mod_ID == msg->pluginid)
		{
			return (IPTR)m->mod_Name;
		}
	}

	return (IPTR)NULL;
}

static IPTR ApplicationGetModuleUserId(Class *cl, Object *obj, struct APPP_GetModuleUserId *msg)
{
	struct ApplicationData *d = INST_DATA(cl, obj);
	struct Module *m;
	STRPTR res = NULL;

	ForeachNode(&d->modules, m)
	{
		if(m->mod_ID == msg->pluginid)
		{
			if(GetAttr(KWAA_UserID, m->mod_Object, (ULONG*)&res))
				return (IPTR)res;
			else
				return (IPTR)NULL;
		}
	}

	return (IPTR)NULL;
}

static IPTR ApplicationCloseModules(Class *cl, Object *obj)
{
	struct ApplicationData *d = INST_DATA(cl, obj);
	struct Module *m;
	ENTER();

	while((m = (struct Module*) REMHEAD((struct List*)&d->modules)))
	{
		DoMethod(m->mod_Object, KWAM_Disconnect, NULL);
		CloseModule(m);
	}

	NEWLIST(&d->modules);
	d->modules_no = 0;

	LEAVE();
	return (IPTR)0;
}


static IPTR ApplicationConnect(Class *cl, Object *obj, struct APPP_Connect *msg)
{
	struct ApplicationData *d = INST_DATA(cl, obj);
	struct Module *m;
	STRPTR utf_desc = SystemToUtf8(msg->description);
	ENTER();

	d->connecting_modules_no = 0;

	ForeachNode(&d->modules, m)
	{
		if(!MODULE_IS_CONNECTED(m))
		{
			DoMethod(m->mod_Object, KWAM_Connect, msg->new_status, utf_desc);
			d->connecting_modules_no++;
		}
	}

	if(d->connecting_modules_no)
		set(findobj(USD_MAIN_WINDOW_BUSY_BAR, d->main_window), MUIA_ShowMe, TRUE);

	if(utf_desc)
		FreeVec(utf_desc);

	LEAVE();
	return (IPTR)0;
}

static IPTR ApplicationDisconnect(Class *cl, Object *obj, struct APPP_Disconnect *msg)
{
	struct ApplicationData *d = INST_DATA(cl, obj);
	struct Module *m;
	STRPTR utf_desc = SystemToUtf8(msg->description);
	ENTER();

	ForeachNode(&d->modules, m)
	{
		if(MODULE_IS_CONNECTED(m) && DoMethod(m->mod_Object, KWAM_Disconnect, utf_desc))
		{
			tprintf("disconnected module %ls\n", m->mod_Name);
		}
	}

	if(utf_desc)
		FreeVec(utf_desc);

	LEAVE();
	return (IPTR)0;
}

static IPTR ApplicationChangeStatus(Class *cl, Object *obj, struct APPP_ChangeStatus *msg)
{
	struct ApplicationData *d = INST_DATA(cl, obj);
	IPTR result = 0;
	STRPTR utf_desc;
	ENTER();

	d->connecting_modules_no = 0;

	utf_desc = SystemToUtf8(msg->description);

	if(msg->new_status != KWA_STATUS_NOT_AVAIL)
	{
		struct Module *m;

		ForeachNode(&d->modules, m)
		{
			if(!MODULE_IS_CONNECTED(m))
			{
				result = DoMethod(m->mod_Object, KWAM_Connect, msg->new_status, utf_desc);
				d->connecting_modules_no++;
			}
			else
				result = DoMethod(m->mod_Object, KWAM_ChangeStatus, msg->new_status, utf_desc);
		}
	}
	else
		result = DoMethod(obj, APPM_Disconnect, msg->description); /* APPM_Disconnect have own system->utf8 conversion */

	if(d->connecting_modules_no)
		set(findobj(USD_MAIN_WINDOW_BUSY_BAR, d->main_window), MUIA_ShowMe, TRUE);

	if(utf_desc)
		FreeVec(utf_desc);

	DoMethod(obj, APPM_SetLastStatus, msg->new_status, msg->description);

	LEAVE();
	return (IPTR)result;
}

static IPTR ApplicationNotifyContactList(Class *cl, Object *obj, struct APPP_NotifyContactList *msg)
{
	struct ApplicationData *d = INST_DATA(cl, obj);
	ULONG i;
	Object *con_list = findobj(USD_CONTACTS_LIST, d->main_window);
	struct NotifyListEntry *entries;
	ULONG entries_no = 0;
	ENTER();

	if(MODULE_IS_CONNECTED(msg->m))
	{
		for(i = 0;;i++)
		{
			struct ContactEntry *con;

			DoMethod(con_list, CLSM_GetEntry, i, &con);
			if(!con)
				break;
			if(con->pluginid == msg->m->mod_ID)
				entries_no++;
		}

		if(entries_no > 0 && (entries = AllocMem(entries_no * sizeof(struct NotifyListEntry), MEMF_PUBLIC)))
		{
			ULONG act_entry = 0;

			for(i = 0; act_entry < entries_no; i++)
			{
				struct ContactEntry *con;

				DoMethod(con_list, CLSM_GetEntry, i, &con);
				if(!con)
					break;
				if(con->pluginid == msg->m->mod_ID)
				{
					entries[act_entry].nle_EntryID = con->entryid;
					entries[act_entry++].nle_Status = con->status;
				}
			}

			DoMethod(msg->m->mod_Object, KWAM_NotifyList, entries_no, entries);

			FreeMem(entries, entries_no * sizeof(struct NotifyListEntry));
		}
		else
			DoMethod(msg->m->mod_Object, KWAM_NotifyList, 0, NULL);
	}

	LEAVE();
	return (IPTR)0;
}

static IPTR ApplicationListChangeAck(Class *cl, Object *obj, struct APPP_ListChangeAck *msg)
{
	struct ApplicationData *d = INST_DATA(cl, obj);
	Object *contacts_list = findobj(USD_CONTACTS_LIST, d->main_window);
	LONG j;
	struct ContactEntry *to_update;
	BYTE buffer[50];
	IPTR result = 0;

	for(j = 0;;j++)
	{
		DoMethod(contacts_list, CLSM_GetEntry, j, &to_update);
		if(!to_update || (to_update->pluginid == msg->m->mod_ID && StrEqu(to_update->entryid, msg->change->ke_ContactID)))
			break;
	}

	if(to_update)
	{
		STRPTR new_desc = Utf8ToSystem(msg->change->ke_Description);

		if(to_update->status != KWA_STATUS_FRESH && (to_update->status != msg->change->ke_NewStatus ||
		  (to_update->statusdesc != new_desc || (to_update->statusdesc && new_desc && !StrEqu(to_update->statusdesc, new_desc)))))
		{
			if(KWA_S_NAVAIL(msg->change->ke_NewStatus))
				FmtNPut((STRPTR)buffer, "%s: %s", sizeof(buffer), ContactName(to_update), GetString(MSG_GG_STATUS_UNAVAIL));
			if(KWA_S_AVAIL(msg->change->ke_NewStatus))
				FmtNPut((STRPTR)buffer, "%s: %s", sizeof(buffer), ContactName(to_update), GetString(MSG_GG_STATUS_AVAIL));
			if(KWA_S_BUSY(msg->change->ke_NewStatus))
				FmtNPut((STRPTR)buffer, "%s: %s", sizeof(buffer), ContactName(to_update), GetString(MSG_GG_STATUS_AWAY));
			if(KWA_S_FFC(msg->change->ke_NewStatus))
				FmtNPut((STRPTR)buffer, "%s: %s", sizeof(buffer), ContactName(to_update), GetString(MSG_GG_STATUS_FFC));
			if(KWA_S_DND(msg->change->ke_NewStatus))
				FmtNPut((STRPTR)buffer, "%s: %s", sizeof(buffer), ContactName(to_update), GetString(MSG_GG_STATUS_DND));
			if(KWA_S_BLOCKED(msg->change->ke_NewStatus))
				FmtNPut((STRPTR)buffer, "%s: %s", sizeof(buffer), ContactName(to_update), GetString(MSG_GG_STATUS_BLOCKED));
			if(KWA_S_INVISIBLE(msg->change->ke_NewStatus))
				FmtNPut((STRPTR)buffer, "%s: %s", sizeof(buffer), ContactName(to_update), GetString(MSG_GG_STATUS_INVISIBLE));

			DoMethod(obj, APPM_NotifyBeacon, (STRPTR)BEACON_STATUS, (STRPTR)buffer, FALSE, NULL, NULL, NULL);
		}

		to_update->status = msg->change->ke_NewStatus;

		if(to_update->statusdesc)
			StrFree(to_update->statusdesc);

		to_update->statusdesc = new_desc;

		DoMethod(d->talk_window, TKWM_UpdateTabContactStatus, to_update->entryid, to_update->pluginid);

		result++;
		DoMethod(contacts_list, CLSM_Sort);
	}

	return result;
}

static IPTR ApplicationNewMessageAck(Class *cl, Object *obj, struct APPP_NewMessageAck *msg)
{
	struct ApplicationData *d = INST_DATA(cl, obj);
	STRPTR sys_txt, sys_id;
	ENTER();

	if((sys_id = Utf8ToSystem(msg->nmsg->ke_ContactID)))
	{
		if((sys_txt = Utf8ToSystem(msg->nmsg->ke_Txt)))
		{
			DoMethod(d->talk_window, TKWM_ShowMessage, sys_id, msg->m->mod_ID, sys_txt, msg->nmsg->ke_TimeStamp, msg->nmsg->ke_Flags);
			FreeVec(sys_txt);
		}
		FreeVec(sys_id);
	}

	LEAVE();
	return(IPTR)0;
}


static IPTR ApplicationSendMessage(Class *cl, Object *obj, struct APPP_SendMessage *msg)
{
	struct ApplicationData *d = INST_DATA(cl, obj);
	BOOL result = FALSE;
	ENTER();

	if(msg->contactid && msg->pluginid)
	{
		struct Module *m = FindModuleById(&d->modules, msg->pluginid);

		if(m && MODULE_IS_CONNECTED(m))
		{
			STRPTR utf8_con_id;

			if((utf8_con_id = SystemToUtf8(msg->contactid)))
			{
				STRPTR utf8_msg;

				if((utf8_msg = SystemToUtf8(msg->txt)))
				{
					result = (BOOL)DoMethod(m->mod_Object, KWAM_SendMessage, utf8_con_id, utf8_msg);
					FreeVec(utf8_msg);
				}
				FreeVec(utf8_con_id);
			}
		}
	}

	LEAVE();
	return (IPTR)result;
}

static IPTR ApplicationSendTypingNotify(Class *cl, Object *obj, struct APPP_SendTypingNotify *msg)
{
	struct ApplicationData *d = INST_DATA(cl, obj);
	struct Module *m;

	if((m = FindModuleById(&d->modules, msg->pluginid)) && MODULE_IS_CONNECTED(m))
		return DoMethod(m->mod_Object, KWAM_TypingNotify, msg->contactid, msg->txt_len);
	return (IPTR)FALSE;
}

static IPTR ApplicationAddNotify(Class *cl, Object *obj, struct APPP_AddNotify *msg)
{
	struct ApplicationData *d = INST_DATA(cl, obj);
	struct Module *m;
	struct NotifyListEntry notify;

	if((m = FindModuleById(&d->modules, msg->pluginid)) && MODULE_IS_CONNECTED(m))
	{
		if((notify.nle_EntryID = SystemToUtf8(msg->contactid)))
		{
			notify.nle_Status = msg->contact_status;
			DoMethod(m->mod_Object, KWAM_AddNotify, &notify);
			FreeVec(notify.nle_EntryID);
		}
	}

	return(IPTR)0;
}

static IPTR ApplicationRemoveNotify(Class *cl, Object *obj, struct APPP_RemoveNotify *msg)
{
	struct ApplicationData *d = INST_DATA(cl, obj);
	struct Module *m;
	struct NotifyListEntry notify;
	ENTER();

	if((m = FindModuleById(&d->modules, msg->pluginid)) && MODULE_IS_CONNECTED(m))
	{
		if((notify.nle_EntryID = SystemToUtf8(msg->contactid)))
		{
			notify.nle_Status = msg->contact_status;
			DoMethod(m->mod_Object, KWAM_RemoveNotify, &notify);
			FreeVec(notify.nle_EntryID);
		}
	}

	LEAVE();
	return(IPTR)0;
}

static IPTR ApplicationImportList(Class *cl, Object *obj)
{
	struct ApplicationData *d = INST_DATA(cl, obj);
	struct Module *m;

	ForeachNode(&d->modules, m)
	{
		if(MODULE_IS_CONNECTED(m))
		{
			DoMethod(m->mod_Object, KWAM_ImportList);
		}
	}

	return (IPTR)0;
}

static IPTR ApplicationExportList(Class *cl, Object *obj)
{
	struct ApplicationData *d = INST_DATA(cl, obj);
	struct Module *m;
	Object *con_list = findobj(USD_CONTACTS_LIST, d->main_window);

	ForeachNode(&d->modules, m)
	{
		if(MODULE_IS_CONNECTED(m))
		{
			struct ContactEntry *entry;
			struct ContactEntry *array;
			ULONG i, cons_no = 0;

			for(i = 0;;i++)
			{
				DoMethod(con_list, CLSM_GetEntry, i, &entry);
				if(!entry)
					break;
				if(entry->pluginid == m->mod_ID)
					cons_no++;
			}

			if((array = AllocMem(cons_no * sizeof(struct ContactEntry), MEMF_ANY | MEMF_PUBLIC)))
			{
				ULONG j;
				for(i = 0, j = 0;;i++)
				{
					DoMethod(con_list, CLSM_GetEntry, i, &entry);
					if(!entry)
						break;
					if(entry->pluginid == m->mod_ID)
					{
						array[j].birthyear = SystemToUtf8(entry->birthyear);
						array[j].city = SystemToUtf8(entry->city);
						array[j].entryid = SystemToUtf8(entry->entryid);
						array[j].firstname = SystemToUtf8(entry->firstname);
						array[j].gender = entry->gender;
						array[j].groupname = SystemToUtf8(entry->groupname);
						array[j].lastname = SystemToUtf8(entry->lastname);
						array[j].name = SystemToUtf8(entry->name);
						array[j].nickname = SystemToUtf8(entry->nickname);
						array[j].pluginid = entry->pluginid;
						array[j].status = 0;
						array[j].statusdesc = NULL;
						array[j].unread = FALSE;
						j++;
					}
				}

				DoMethod(m->mod_Object, KWAM_ExportList, cons_no, array);

				for(i = 0; i < cons_no; i++)
				{
					if(array[i].entryid)
						FreeVec(array[i].entryid);
					if(array[i].city)
						FreeVec(array[i].city);
					if(array[i].firstname)
						FreeVec(array[i].firstname);
					if(array[i].groupname)
						FreeVec(array[i].groupname);
					if(array[i].lastname)
						FreeVec(array[i].lastname);
					if(array[i].name)
						FreeVec(array[i].name);
					if(array[i].nickname)
						FreeVec(array[i].nickname);
				}

				FreeMem(array, cons_no * sizeof(struct ContactEntry));
			}
		}
	}

	return (IPTR)0;
}

static IPTR ApplicationSendPicture(Class *cl, Object *obj, struct APPP_SendPicture *msg)
{
	struct ApplicationData *d = INST_DATA(cl, obj);
	BOOL result = FALSE;
	ENTER();

	if(msg->contactid && msg->pluginid)
	{
		struct Module *m = FindModuleById(&d->modules, msg->pluginid);

		if(m && MODULE_IS_CONNECTED(m))
			result = (BOOL)DoMethod(m->mod_Object, KWAM_SendPicture, msg->contactid, msg->path);
	}

	LEAVE();
	return (IPTR)result;
}

static IPTR ApplicationAnswerInvite(Class *cl, Object *obj, struct APPP_AnswerInvite *msg)
{
	struct ApplicationData *d = INST_DATA(cl, obj);
	BOOL result = FALSE;
	ENTER();

	if(msg->contactid && msg->pluginid)
	{
		struct Module *m = FindModuleById(&d->modules, msg->pluginid);

		if(m && MODULE_IS_CONNECTED(m))
			result = (BOOL)DoMethod(m->mod_Object, KWAM_AnswerInvite, msg->contactid, msg->answer);
	}

	LEAVE();
	return (IPTR)result;
}

static IPTR ApplicationPubDirRequest(Class *cl, Object *obj, struct APPP_PubDirRequest *msg)
{
	struct ApplicationData *d = INST_DATA(cl, obj);
	BOOL result = FALSE;
	ENTER();

	if(msg->contactid && msg->pluginid)
	{
		struct Module *m = FindModuleById(&d->modules, msg->pluginid);

		if(m && MODULE_IS_CONNECTED(m))
			result = (BOOL)DoMethod(m->mod_Object, KWAM_FetchContactInfo, msg->contactid, msg->obj, msg->method);
	}

	LEAVE();
	return (IPTR)result;
}

static IPTR ApplicationStatusChangeAck(Class *cl, Object *obj, struct APPP_StatusChangeAck *msg)
{
	struct ApplicationData *d = INST_DATA(cl, obj);
	ENTER();

	DoMethod(obj, APPM_ScreenbarChange, msg->change->ke_NewStatus);
	DoMethod(d->main_window, MWM_ChangeStatus, msg->change->ke_NewStatus);

	LEAVE();
	return (IPTR)0;
}

static IPTR ApplicationDisconnectAck(Class *cl, Object *obj, struct APPP_DisconnectAck *msg)
{
	struct ApplicationData *d = INST_DATA(cl, obj);
	Object *contacts_list = findobj(USD_CONTACTS_LIST, d->main_window);
	struct ContactEntry *entry;
	LONG i;
	ENTER();

	for(i = 0;;i++)
	{
		DoMethod(contacts_list, CLSM_GetEntry, i, &entry);
		if(!entry)
			break;

		if(entry->pluginid == msg->m->mod_ID)
		{
			entry->status = KWA_STATUS_FRESH;
			DoMethod(d->talk_window, TKWM_UpdateTabContactStatus, entry->entryid, entry->pluginid);
		}
	}

	DoMethod(contacts_list, CLSM_Sort);
	DoMethod(obj, APPM_ScreenbarChange, KWA_STATUS_NOT_AVAIL);
	DoMethod(d->main_window, MWM_ChangeStatus, KWA_STATUS_NOT_AVAIL);

	msg->m->mod_Connected = FALSE;

	LEAVE();
	return (IPTR)0;
}

static IPTR ApplicationTypingNotifyAck(Class *cl, Object *obj, struct APPP_TypingNotifyAck *msg)
{
	struct ApplicationData *d = INST_DATA(cl, obj);

	return DoMethod(d->talk_window, TKWM_UpdateTabWriteLamp, msg->notify->ke_ContactID, msg->m->mod_ID, msg->notify->ke_TxtLen);
}

static IPTR ApplicationConnectAck(Class *cl, Object *obj, struct APPP_ConnectAck *msg)
{
	struct ApplicationData *d = INST_DATA(cl, obj);

	msg->m->mod_Connected = TRUE;

	d->connecting_modules_no--;

	if(!d->connecting_modules_no)
		set(findobj(USD_MAIN_WINDOW_BUSY_BAR, d->main_window), MUIA_ShowMe, FALSE);

	return DoMethod(obj, APPM_NotifyContactList, msg->m);
}

static IPTR ApplicationSecTrigger(Class *cl, Object *obj)
{
	struct ApplicationData *d = INST_DATA(cl, obj);
	struct Module *m;

	ForeachNode(&d->modules, m)
	{
		DoMethod(m->mod_Object, KWAM_TimedMethod);
	}

	if(d->broker_status_away == FALSE && d->broker_start_time != 0 && --d->broker_act_time == 0)
	{
		DoMethod(obj, APPM_AutoAway);
		d->broker_act_time = d->broker_start_time;
		d->broker_status_away = TRUE;
	}

	return (IPTR)0;
}

static IPTR ApplicationErrorNoMem(Class *cl, Object *obj, struct APPP_ErrorNoMem *msg)
{
	struct ApplicationData *d = INST_DATA(cl, obj);
	Object *win = NULL;

	if((BOOL)xget(d->main_window, MUIA_Window_Open))
		win = d->main_window;

	switch(MUI_Request(obj, win, 0, APP_NAME, GetString(MSG_MODULE_ERROR_NOMEM_GADGETS), GetString(MSG_MODULE_ERROR_NOMEM_MSG), msg->m->mod_Name))
	{
		case 1:
			return (IPTR)0;

		case 2:
			return DoMethod(msg->m->mod_Object, KWAM_Disconnect, NULL);

		case 0:
			return DoMethod(obj, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
	}

	return (IPTR)0;
}

static IPTR ApplicationErrorConnFail(Class *cl, Object *obj, struct APPP_ErrorConnFail *msg)
{
	struct ApplicationData *d = INST_DATA(cl, obj);
	Object *win = NULL;

	if((BOOL)xget(d->main_window, MUIA_Window_Open))
		win = d->main_window;

	d->connecting_modules_no--;

	if(!d->connecting_modules_no)
		set(findobj(USD_MAIN_WINDOW_BUSY_BAR, d->main_window), MUIA_ShowMe, FALSE);

	switch(MUI_Request(obj, win, 0, APP_NAME, GetString(MSG_MODULE_ERROR_CONNFAIL_GADGETS), GetString(MSG_MODULE_ERROR_CONNFAIL_MSG), msg->m->mod_Name))
	{
		case 1:
			return (IPTR)0;

		case 2:
			return DoMethod(obj, APPM_Connect, d->last_status, d->last_desc);

		case 0:
			return DoMethod(obj, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
	}

	return (IPTR)0;
}

static IPTR ApplicationErrorLoginFail(Class *cl, Object *obj, struct APPP_ErrorLoginFail *msg)
{
	struct ApplicationData *d = INST_DATA(cl, obj);
	Object *win = NULL;

	if((BOOL)xget(d->main_window, MUIA_Window_Open))
		win = d->main_window;

	switch(MUI_Request(obj, win, 0, APP_NAME, GetString(MSG_MODULE_ERROR_LOGINFAIL_GADGETS), GetString(MSG_MODULE_ERROR_LOGINFAIL_MSG), msg->m->mod_Name))
	{
		case 1:
			return (IPTR)0;

		case 2:
			set(d->prefs_window, MUIA_Window_Open, TRUE);
		return (IPTR)0;

		case 0:
			return DoMethod(obj, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
	}

	return (IPTR)0;
}

static IPTR ApplicationErrorNotSupported(Class *cl, Object *obj, struct APPP_ErrorNotSupported *msg)
{
	struct ApplicationData *d = INST_DATA(cl, obj);
	Object *win = NULL;

	if((BOOL)xget(d->main_window, MUIA_Window_Open))
		win = d->main_window;

	switch(MUI_Request(obj, win, 0, APP_NAME, GetString(MSG_MODULE_ERROR_NOTSUPP_GADGETS), GetString(MSG_MODULE_ERROR_NOTSUPP_MSG),
	 msg->m->mod_Name, msg->errormsg))
	{
		case 1:
			return (IPTR)0;

		case 2:
			set(d->prefs_window, MUIA_Window_Open, TRUE);
		return (IPTR)0;

		case 0:
			return DoMethod(obj, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
	}

	return (IPTR)0;
}

static IPTR ApplicationModuleMessageAck(Class *cl, Object *obj, struct APPP_ModuleMessageAck *msg)
{
	struct ApplicationData *d = INST_DATA(cl, obj);

	DoMethod(d->modules_log_window, MLW_AddMsg, msg->m->mod_Name, msg->event->ke_Errno, msg->event->ke_MsgTxt);

	if(msg->event->ke_Errno == ERRNO_ONLY_MESSAGE)
		return (IPTR)0;

	switch(msg->event->ke_Errno)
	{
		case ERRNO_OUT_OF_MEMORY:
			return DoMethod(obj, APPM_ErrorNoMem, msg->m);

		case ERRNO_CONNECTION_FAILED:
			return DoMethod(obj, APPM_ErrorConnFail, msg->m);

		case ERRNO_LOGIN_FAILED:
			return DoMethod(obj, APPM_ErrorLoginFail, msg->m);

		case ERRNO_NOT_SUPPORTED:
			return DoMethod(obj, APPM_ErrorNotSupported, msg->m, msg->event->ke_MsgTxt);
	}

	return (IPTR)0;
}

static IPTR ApplicationNewAvatarAck(Class *cl, Object *obj, struct APPP_NewAvatarAck *msg)
{
	struct ApplicationData *d = INST_DATA(cl, obj);
	struct ContactEntry *con;
	Object *con_list = findobj(USD_CONTACTS_LIST, d->main_window);
	LONG i;

	for(i = 0;;i++)
	{
		DoMethod(con_list, CLSM_GetEntry, i, &con);
		if(!con || (con->pluginid == msg->m->mod_ID && StrEqu(con->entryid, msg->newavatar->ke_ContactID)))
			break;
	}

	if(con && (con->avatar = CopyPicture(msg->newavatar->ke_Picture)))
		DoMethod(con_list, CLSM_NewAvatar);

	return (IPTR)0;
}

static IPTR ApplicationImportListAck(Class *cl, Object *obj, struct APPP_ImportListAck *msg)
{
	struct ApplicationData *d = INST_DATA(cl, obj);
	Object *con_list = findobj(USD_CONTACTS_LIST, d->main_window);
	LONG i;

	for(i = 0; i < msg->event->ke_ContactsNo; i++)
	{
		if(msg->event->ke_Contacts[i].entryid != NULL && msg->event->ke_Contacts[i].pluginid != 0)
		{
			struct ContactEntry con = {0};

			if((con.entryid = Utf8ToSystem(msg->event->ke_Contacts[i].entryid)))
			{
				con.pluginid = msg->event->ke_Contacts[i].pluginid;
				con.birthyear = msg->event->ke_Contacts[i].birthyear;
				con.city = Utf8ToSystem(msg->event->ke_Contacts[i].city);
				con.firstname = Utf8ToSystem(msg->event->ke_Contacts[i].firstname);
				con.gender = msg->event->ke_Contacts[i].gender;
				con.groupname = Utf8ToSystem(msg->event->ke_Contacts[i].groupname);
				con.lastname = Utf8ToSystem(msg->event->ke_Contacts[i].lastname);
				con.name = Utf8ToSystem(msg->event->ke_Contacts[i].name);
				con.nickname = Utf8ToSystem(msg->event->ke_Contacts[i].nickname);

				DoMethod(con_list, CLSM_InsertSingle, &con, CLSV_Insert_Bottom);
				DoMethod(obj, APPM_AddNotify, con.pluginid, con.entryid, 0);

				if(con.entryid)
					FreeVec(con.entryid);
				if(con.city)
					FreeVec(con.city);
				if(con.firstname)
					FreeVec(con.firstname);
				if(con.groupname)
					FreeVec(con.groupname);
				if(con.lastname)
					FreeVec(con.lastname);
				if(con.name)
					FreeVec(con.name);
				if(con.nickname)
					FreeVec(con.nickname);
			}
		}
	}

	DoMethod(con_list, CLSM_Sort);
	DoMethod(con_list, CLSM_SaveList);

	return (IPTR)0;
}

static IPTR ApplicationExportListAck(Class *cl, Object *obj, struct APPP_ExportListAck *msg)
{
	if(!msg->event->ke_Accepted)
	{
		struct ApplicationData *d = INST_DATA(cl, obj);
		Object *win = NULL;

		if((BOOL)xget(d->main_window, MUIA_Window_Open))
			win = d->main_window;

		switch(MUI_Request(obj, win, 0, APP_NAME, GetString(MSG_MODULE_LIST_EXPORT_FAIL_GADGETS),
		 GetString(MSG_MODULE_LIST_EXPORT_FAIL), msg->m->mod_Name))
		{
			case 0:
				set(d->modules_log_window, MUIA_Window_Open, TRUE);
			break;
		}

	}

	return (IPTR)0;
}

static IPTR ApplicationNewPictureAck(Class *cl, Object *obj, struct APPP_NewPictureAck *msg)
{
	struct ApplicationData *d = INST_DATA(cl, obj);
	ENTER();

	DoMethod(d->talk_window, TKWM_ShowPicture, msg->event->ke_ContactID, msg->m->mod_ID, msg->event->ke_TimeStamp,
	 msg->event->ke_Flags, msg->event->ke_Data, msg->event->ke_DataSize);

	LEAVE();
	return (IPTR)0;
}

static IPTR ApplicationNewInviteAck(Class *cl, Object *obj, struct APPP_NewInviteAck *msg)
{
	struct ApplicationData *d = INST_DATA(cl, obj);
	ENTER();

	DoMethod(d->talk_window, TKWM_ShowInvite, msg->event->ke_ContactID, msg->m->mod_ID, msg->event->ke_TimeStamp);

	LEAVE();
	return (IPTR)0;
}

static IPTR ApplicationParseEvent(Class *cl, Object *obj, struct APPP_ParseEvent *msg)
{
	switch(msg->event->ke_Type)
	{
		case KE_TYPE_MODULE_MESSAGE:
			return DoMethod(obj, APPM_ModuleMessageAck, msg->m, &msg->event->ke_ModuleMessage);

		case KE_TYPE_CONNECT:
			return DoMethod(obj, APPM_ConnectAck, msg->m);

		case KE_TYPE_DISCONNECT:
			return DoMethod(obj, APPM_DisconnectAck, msg->m);

		case KE_TYPE_STATUS_CHANGE:
			return DoMethod(obj, APPM_StatusChangeAck, msg->m, &msg->event->ke_StatusChange);

		case KE_TYPE_LIST_CHANGE:
			return DoMethod(obj, APPM_ListChangeAck, msg->m, &msg->event->ke_ListChange);

		case KE_TYPE_NEW_MESSAGE:
			return DoMethod(obj, APPM_NewMessageAck, msg->m, &msg->event->ke_NewMessage);

		case KE_TYPE_TYPING_NOTIFY:
			return DoMethod(obj, APPM_TypingNotifyAck, msg->m, &msg->event->ke_TypingNotify);

		case KE_TYPE_NEW_AVATAR:
			return DoMethod(obj, APPM_NewAvatarAck, msg->m, &msg->event->ke_NewAvatar);

		case KE_TYPE_IMPORT_LIST:
			return DoMethod(obj, APPM_ImportListAck, msg->m, &msg->event->ke_ImportList);

		case KE_TYPE_EXPORT_LIST:
			return DoMethod(obj, APPM_ExportListAck, msg->m, &msg->event->ke_ExportList);

		case KE_TYPE_SEND_HTTP_GET:
			return DoMethod(obj, APPM_SendHttpGet, msg->event->ke_HttpGet.ke_Url, msg->event->ke_HttpGet.ke_UserAgent,
			 msg->m->mod_Object, msg->event->ke_HttpGet.ke_MethodID, msg->event->ke_HttpGet.ke_UserData);

		case KE_TYPE_SEND_HTTP_POST:
			return DoMethod(obj, APPM_SendHttpPost, msg->event->ke_HttpPost.ke_Url, msg->event->ke_HttpPost.ke_UserAgent,
			 msg->event->ke_HttpPost.ke_DataItems, msg->m->mod_Object, msg->event->ke_HttpPost.ke_MethodID, msg->event->ke_HttpPost.ke_UserData);

		case KE_TYPE_NEW_PICTURE:
			return DoMethod(obj, APPM_NewPictureAck, msg->m, &msg->event->ke_NewPicture);

		case KE_TYPE_NEW_INVITE:
			return DoMethod(obj, APPM_NewInviteAck, msg->m, &msg->event->ke_NewInvite);

		case KE_TYPE_NOTIFY_BEACON:
			return DoMethod(obj, APPM_NotifyBeacon, msg->event->ke_NotifyBeacon.ke_NotificationName, msg->event->ke_NotifyBeacon.ke_Message,
			 msg->event->ke_NotifyBeacon.ke_WaitForResult, msg->m->mod_Object, msg->event->ke_NotifyBeacon.ke_MethodID, msg->event->ke_NotifyBeacon.ke_UserData);
	}
	return (IPTR)0;
}

static IPTR ApplicationConvertContactEntry(Class *cl, Object *obj, struct APPP_ConvertContactEntry *msg)
{
	struct ContactEntry *c = msg->entry;

	if(c)
	{
		STRPTR t;

		if((t = Utf8ToSystem(c->name)))
		{
			StrFree(c->name);
			c->name = t;
		}
		if((t = Utf8ToSystem(c->nickname)))
		{
			StrFree(c->nickname);
			c->nickname = t;
		}
		if((t = Utf8ToSystem(c->firstname)))
		{
			StrFree(c->firstname);
			c->firstname = t;
		}
		if((t = Utf8ToSystem(c->lastname)))
		{
			StrFree(c->lastname);
			c->lastname = t;
		}
		if((t = Utf8ToSystem(c->groupname)))
		{
			StrFree(c->groupname);
			c->groupname = t;
		}
		if((t = Utf8ToSystem(c->birthyear)))
		{
			StrFree(c->birthyear);
			c->birthyear = t;
		}
		if((t = Utf8ToSystem(c->city)))
		{
			StrFree(c->city);
			c->city = t;
		}
		if((t = Utf8ToSystem(c->statusdesc)))
		{
			StrFree(c->statusdesc);
			c->statusdesc = t;
		}
	}

	return (IPTR)c;
}

static IPTR ApplicationSetLastStatus(Class *cl, Object *obj, struct APPP_SetLastStatus *msg)
{
	struct ApplicationData *d = INST_DATA(cl, obj);
	BOOL save_file = FALSE;
	ENTER();

	if(d->last_status != msg->status)
	{
		d->last_status = msg->status;
		save_file = TRUE;
	}

	if(msg->desc)
	{
		if(d->last_desc)
		{
			if(!StrEqu(d->last_desc, msg->desc))
			{
				StrFree(d->last_desc);
				d->last_desc = StrNew(msg->desc);
				save_file = TRUE;
			}
		}
		else
		{
			d->last_desc = StrNew(msg->desc);
			save_file = TRUE;
		}
	}
	else
	{
		if(d->last_desc)
		{
			StrFree(d->last_desc);
			d->last_desc = NULL;
		}
		else
			save_file = TRUE;
	}

	/* save to file */
	if(save_file)
	{
		BPTR fh;

		if((fh = Open(LAST_STATUS_FILE, MODE_NEWFILE)))
		{
			ULONG len = 0;

			FWrite(fh, &d->last_status, sizeof(ULONG), 1);

			if(d->last_desc)
			{
				len = StrLen(d->last_desc);

				FWrite(fh, &len, sizeof(ULONG), 1);
				FWrite(fh, d->last_desc, len, 1);
			}
			else
				FWrite(fh, &len, sizeof(ULONG), 1);

			Close(fh);
		}
	}

	LEAVE();
	return (IPTR)0;
}

static IPTR ApplicationOpenHistoryDatabase(Class *cl, Object *obj)
{
	struct ApplicationData *d = INST_DATA(cl, obj);
	IPTR result = -1;
	STRPTR errmsg = NULL;
	LONG sql_res, i = -1;
	static CONST TEXT schema[] = SQL_STMT_PRAGMA_CASCADE SQL_STMT_CREATE_TABLE_CONVERSATIONS
	 SQL_STMT_CREATE_TABLE_MESSAGES SQL_STMT_CREATE_INDEX_MESSAGES_CONVERSATIONS;

	if((sql_res = sqlite3_open_v2(HISTORY_DATABASE_PATH, &d->history_database, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL)) == SQLITE_OK)
	{
		if((sql_res = sqlite3_exec(d->history_database, schema, NULL, NULL, &errmsg)) == SQLITE_OK)
		{
			for(i = 0; i < SQL_STMT_NO; i++)
			{
				if((sql_res = sqlite3_prepare_v2(d->history_database, SQL[i], -1, &d->history_stmt[i], NULL)) != SQLITE_OK)
					break;
			}

			if(i == SQL_STMT_NO)
				result = 0;
		}
		else if(errmsg)
		{
			Object *win = NULL;

			if(xget(d->talk_window, MUIA_Window_Open))
				win = d->talk_window;
			else if(xget(d->main_window, MUIA_Window_Open))
				win = d->main_window;

			tprintf("SQLITE ERROR: %ld %ls\n", sql_res, errmsg);

			MUI_RequestA(obj, win, 0, GetString(MSG_SQL_ERROR), GetString(MSG_SQL_GADGETS), errmsg, NULL);

			sqlite3_free(errmsg);

			sqlite3_close(d->history_database);
			d->history_database = NULL;
		}
	}

	if(result != 0 && d->history_database)
	{
		Object *win = NULL;

		if(xget(d->talk_window, MUIA_Window_Open))
			win = d->talk_window;
		else if(xget(d->main_window, MUIA_Window_Open))
			win = d->main_window;

		tprintf("SQLITE ERROR: [%ld] %ld %ls\n", i, sql_res, sqlite3_errmsg(d->history_database));

		MUI_RequestA(obj, win, 0, GetString(MSG_SQL_ERROR), GetString(MSG_SQL_GADGETS), (STRPTR)sqlite3_errmsg(d->history_database), NULL);

		sqlite3_close(d->history_database);
		d->history_database = NULL;
	}

	return result;
}

static IPTR ApplicationCloseHistoryDatabase(Class *cl, Object *obj)
{
	struct ApplicationData *d = INST_DATA(cl, obj);
	LONG i;

	for(i = 0; i < SQL_STMT_NO; i++)
		sqlite3_finalize(d->history_stmt[i]);

	sqlite3_close(d->history_database);

	return (IPTR)0;
}

static LONG SqliteCallback(APTR p, LONG argc, STRPTR *argv, STRPTR *col_names)
{
	IPTR *params = (IPTR*)p;
	LONG result = 0;
	ENTER();

#ifdef __DEBUG_SQL__
	LONG i;

	if(col_names)
	{
		for(i = 0; i < argc; i++)
			KPrintF("|%ls\t|\t", strd(col_names[i]));
		KPrintF("\n");
	}

	if(argv)
	{
		for(i = 0; i < argc; i++)
			KPrintF("|%ls\t|\t", strd(argv[i]));
		KPrintF("\n");
	}
#endif /* __DEBUG_SQL__ */

	if((Object*)params[0] != NULL)
		result = DoMethod((Object*)params[0], params[1], argc, argv, col_names);

	LEAVE();
	return result;
}

static IPTR ApplicationDoSqlOnHistoryDatabase(Class *cl, Object *obj, struct APPP_DoSqlOnHistoryDatabase *msg)
{
	struct ApplicationData *d = INST_DATA(cl, obj);
	IPTR result = -1;
	IPTR params[2] = {(IPTR)msg->callback_obj, msg->callback_method};
	STRPTR errmsg = NULL;
	LONG sqlres = -1;
	ENTER();

	if(msg->last_row)
		*msg->last_row = 0;

	if(!d->history_database)
		return result;

	if(msg->sql)
	{
		if((sqlres = sqlite3_exec(d->history_database, msg->sql, (APTR)SqliteCallback, params, &errmsg)) == SQLITE_OK)
		{
			if(msg->last_row)
				*msg->last_row = sqlite3_last_insert_rowid(d->history_database);

			result = 0;
		}
		else if(errmsg)
		{
			Object *win = NULL;

			if(xget(d->talk_window, MUIA_Window_Open))
				win = d->talk_window;
			else if(xget(d->main_window, MUIA_Window_Open))
				win = d->main_window;

			tprintf("SQLITE ERROR: %ld %ls\n", sqlres, errmsg);

			MUI_RequestA(obj, win, 0, GetString(MSG_SQL_ERROR), GetString(MSG_SQL_GADGETS), errmsg, NULL);

			sqlite3_free(errmsg);
		}
	}
	LEAVE();
	return result;
}

static IPTR ApplicationInsertMessageIntoHistory(Class *cl, Object *obj, struct APPP_InsertMessageIntoHistory *msg)
{
	struct ApplicationData *d = INST_DATA(cl, obj);
	int rc;

	if(!d->history_database)
		return -1;

	sqlite3_clear_bindings(d->history_stmt[SQL_INSERT_MESSAGE]);
	sqlite3_reset(d->history_stmt[SQL_INSERT_MESSAGE]);

	if(!msg->conversation_id)
		return -2;

	if((rc = sqlite3_bind_int64(d->history_stmt[SQL_INSERT_MESSAGE], 1, *msg->conversation_id) != SQLITE_OK))
	{
		tprintf("SQLITE BIND ERROR: (%ld) %ls", rc, sqlite3_errmsg(d->history_database));
		return rc;
	}

	if((rc = sqlite3_bind_text(d->history_stmt[SQL_INSERT_MESSAGE], 2, msg->contact_id, -1, SQLITE_STATIC) != SQLITE_OK))
	{
		tprintf("SQLITE BIND ERROR: (%ld) %ls", rc, sqlite3_errmsg(d->history_database));
		return rc;
	}

	if((rc = sqlite3_bind_text(d->history_stmt[SQL_INSERT_MESSAGE], 3, msg->contact_name, -1, SQLITE_STATIC) != SQLITE_OK))
	{
		tprintf("SQLITE BIND ERROR: (%ld) %ls", rc, sqlite3_errmsg(d->history_database));
		return rc;
	}

	if((rc = sqlite3_bind_int(d->history_stmt[SQL_INSERT_MESSAGE], 4, msg->a_timestamp) != SQLITE_OK))
	{
		tprintf("SQLITE BIND ERROR: (%ld) %ls", rc, sqlite3_errmsg(d->history_database));
		return rc;
	}

	if((rc = sqlite3_bind_int(d->history_stmt[SQL_INSERT_MESSAGE], 5, msg->flags) != SQLITE_OK))
	{
		tprintf("SQLITE BIND ERROR: (%ld) %ls", rc, sqlite3_errmsg(d->history_database));
		return rc;
	}

	if(msg->content_len == -1)
	{
		if((rc = sqlite3_bind_text(d->history_stmt[SQL_INSERT_MESSAGE], 6, msg->content, -1, SQLITE_STATIC) != SQLITE_OK))
		{
			tprintf("SQLITE BIND ERROR: (%ld) %ls", rc, sqlite3_errmsg(d->history_database));
			return rc;
		}
	}
	else
	{
		if((rc = sqlite3_bind_text(d->history_stmt[SQL_INSERT_MESSAGE], 6, msg->content, msg->content_len, SQLITE_STATIC) != SQLITE_OK))
		{
			tprintf("SQLITE BIND ERROR: (%ld) %ls", rc, sqlite3_errmsg(d->history_database));
			return rc;
		}
	}

	if((rc = sqlite3_step(d->history_stmt[SQL_INSERT_MESSAGE])) != SQLITE_DONE)
	{
		tprintf("SQLITE STEP ERROR: (%ld) %ls\n", rc, sqlite3_errmsg(d->history_database));
		return rc;
	}

	return 0;
}

static IPTR ApplicationInsertConversationIntoHistory(Class *cl, Object *obj, struct APPP_InsertConversationIntoHistory *msg)
{
	struct ApplicationData *d = INST_DATA(cl, obj);
	STRPTR user_id = (STRPTR)DoMethod(obj, APPM_GetModuleUserId, msg->plugin_id);
	int rc;

	if(msg->inserted_id)
		*msg->inserted_id = -1;

	if(!d->history_database)
		return -1;

	if(!user_id || StrEqu(user_id, ""))
		user_id = (STRPTR)xget(prefs_object(USD_PREFS_TW_USRNAME_STRING), MUIA_String_Contents);

	sqlite3_clear_bindings(d->history_stmt[SQL_INSERT_CONVERSATION]);
	sqlite3_reset(d->history_stmt[SQL_INSERT_CONVERSATION]);

	if((rc = sqlite3_bind_int(d->history_stmt[SQL_INSERT_CONVERSATION], 1, msg->plugin_id) != SQLITE_OK))
	{
		tprintf("SQLITE BIND ERROR: (%ld) %ls", rc, sqlite3_errmsg(d->history_database));
		return rc;
	}

	if((rc = sqlite3_bind_text(d->history_stmt[SQL_INSERT_CONVERSATION], 2, msg->contact_id, -1, SQLITE_STATIC) != SQLITE_OK))
	{
		tprintf("SQLITE BIND ERROR: (%ld) %ls", rc, sqlite3_errmsg(d->history_database));
		return rc;
	}

	if((rc = sqlite3_bind_text(d->history_stmt[SQL_INSERT_CONVERSATION], 3, msg->contact_name, -1, SQLITE_STATIC) != SQLITE_OK))
	{
		tprintf("SQLITE BIND ERROR: (%ld) %ls", rc, sqlite3_errmsg(d->history_database));
		return rc;
	}

	if((rc = sqlite3_bind_text(d->history_stmt[SQL_INSERT_CONVERSATION], 4, user_id, -1, SQLITE_STATIC) != SQLITE_OK))
	{
		tprintf("SQLITE BIND ERROR: (%ld) %ls", rc, sqlite3_errmsg(d->history_database));
		return rc;
	}

	if((rc = sqlite3_bind_int(d->history_stmt[SQL_INSERT_CONVERSATION], 5, msg->flags) != SQLITE_OK))
	{
		tprintf("SQLITE BIND ERROR: (%ld) %ls", rc, sqlite3_errmsg(d->history_database));
		return rc;
	}

	if((rc = sqlite3_step(d->history_stmt[SQL_INSERT_CONVERSATION])) != SQLITE_DONE)
	{
		tprintf("SQLITE STEP ERROR: (%ld) %ls\n", rc, sqlite3_errmsg(d->history_database));
		return rc;
	}

	if(msg->inserted_id)
		*msg->inserted_id = sqlite3_last_insert_rowid(d->history_database);

	return 0;
}

static IPTR ApplicationGetLastMessagesFromHistory(Class *cl, Object *obj, struct APPP_GetLastMessagesFromHistory *msg)
{
	struct ApplicationData *d = INST_DATA(cl, obj);
	int rc;
	ULONG rows_no = 0;

	if(!msg->callback_obj)
		return -1;

	sqlite3_clear_bindings(d->history_stmt[SQL_SELECT_LAST_MESSAGES]);
	sqlite3_reset(d->history_stmt[SQL_SELECT_LAST_MESSAGES]);

	if((rc = sqlite3_bind_int(d->history_stmt[SQL_SELECT_LAST_MESSAGES], 1, msg->plugin_id)) != SQLITE_OK)
	{
		tprintf("SQLITE BIND ERROR: (%ld) %ls", rc, sqlite3_errmsg(d->history_database));
		return rc;
	}

	if((rc = sqlite3_bind_text(d->history_stmt[SQL_SELECT_LAST_MESSAGES], 2, msg->contact_id, -1, SQLITE_STATIC) != SQLITE_OK))
	{
		tprintf("SQLITE BIND ERROR: (%ld) %ls", rc, sqlite3_errmsg(d->history_database));
		return rc;
	}

	if((rc = sqlite3_bind_int(d->history_stmt[SQL_SELECT_LAST_MESSAGES], 3, msg->messages_no)) != SQLITE_OK)
	{
		tprintf("SQLITE BIND ERROR: (%ld) %ls", rc, sqlite3_errmsg(d->history_database));
		return rc;
	}

	while((rc = sqlite3_step(d->history_stmt[SQL_SELECT_LAST_MESSAGES])) == SQLITE_ROW)
	{
		ULONG flags, timestamp, content_len;
		STRPTR contactid;
		UBYTE *content = NULL;

		flags = sqlite3_column_int(d->history_stmt[SQL_SELECT_LAST_MESSAGES], 0);
		timestamp = sqlite3_column_int(d->history_stmt[SQL_SELECT_LAST_MESSAGES], 1);
		contactid = (STRPTR)sqlite3_column_text(d->history_stmt[SQL_SELECT_LAST_MESSAGES], 2);
		content_len = sqlite3_column_bytes(d->history_stmt[SQL_SELECT_LAST_MESSAGES], 3);

		if((flags & HISTORY_MESSAGES_NORMAL) || (flags & HISTORY_MESSAGES_SYSTEM))
		{
			content = (UBYTE*)sqlite3_column_text(d->history_stmt[SQL_SELECT_LAST_MESSAGES], 3);
		}
		else
		{
			/* TODO: images as blob? */
		}

		tprintf("RETURNED: %ld %ld %ls %ls\n", flags, timestamp, strd(contactid), strd(content));

		if(DoMethod(msg->callback_obj, msg->callback_method, flags, timestamp, contactid, content, content_len) != 0)
			return -2;

		rows_no++;
	}

	return rows_no;
}

static IPTR ApplicationGetLastCovMsgsFromHistory(Class *cl, Object *obj, struct APPP_GetLastConvMsgsFromHistory *msg)
{
	struct ApplicationData *d = INST_DATA(cl, obj);
	const LONG stmt_id = SQL_SELECT_LAST_CONVERSATION_MESSAGES;
	int rc;
	ULONG rows_no = 0;

	if(!msg->callback_obj)
		return -1;

	sqlite3_clear_bindings(d->history_stmt[stmt_id]);
	sqlite3_reset(d->history_stmt[stmt_id]);

	if((rc = sqlite3_bind_int(d->history_stmt[stmt_id], 1, msg->plugin_id)) != SQLITE_OK)
	{
		tprintf("SQLITE BIND ERROR: (%ld) %ls", rc, sqlite3_errmsg(d->history_database));
		return rc;
	}

	if((rc = sqlite3_bind_text(d->history_stmt[stmt_id], 2, msg->contact_id, -1, SQLITE_STATIC) != SQLITE_OK))
	{
		tprintf("SQLITE BIND ERROR: (%ld) %ls", rc, sqlite3_errmsg(d->history_database));
		return rc;
	}

	while((rc = sqlite3_step(d->history_stmt[stmt_id])) == SQLITE_ROW)
	{
		ULONG flags, timestamp, content_len;
		STRPTR contactid;
		UBYTE *content = NULL;

		flags = sqlite3_column_int(d->history_stmt[stmt_id], 0);
		timestamp = sqlite3_column_int(d->history_stmt[stmt_id], 1);
		contactid = (STRPTR)sqlite3_column_text(d->history_stmt[stmt_id], 2);
		content_len = sqlite3_column_bytes(d->history_stmt[stmt_id], 3);

		if((flags & HISTORY_MESSAGES_NORMAL) || (flags & HISTORY_MESSAGES_SYSTEM))
		{
			content = (UBYTE*)sqlite3_column_text(d->history_stmt[stmt_id], 3);
		}
		else
		{
			/* TODO: images as blob? */
		}

		tprintf("RETURNED: %ld %ld %ls %ls\n", flags, timestamp, strd(contactid), strd(content));

		if(DoMethod(msg->callback_obj, msg->callback_method, flags, timestamp, contactid, content, content_len) != 0)
			return -2;

		rows_no++;
	}

	return rows_no;
}

static IPTR ApplicationGetLastMessagesByTime(Class *cl, Object *obj, struct APPP_GetLastMessagesByTime *msg)
{
	struct ApplicationData *d = INST_DATA(cl, obj);
	const LONG stmt_id = SQL_SELECT_LAST_MESSAGES_TIME;
	int rc;
	ULONG rows_no = 0;
	ULONG timestamp = LocalToUTC(ActLocalTime2Amiga(), NULL) - msg->secs;

	if(!msg->callback_obj)
		return -1;

	sqlite3_clear_bindings(d->history_stmt[stmt_id]);
	sqlite3_reset(d->history_stmt[stmt_id]);

	if((rc = sqlite3_bind_int(d->history_stmt[stmt_id], 1, msg->plugin_id)) != SQLITE_OK)
	{
		tprintf("SQLITE BIND ERROR: (%ld) %ls", rc, sqlite3_errmsg(d->history_database));
		return rc;
	}

	if((rc = sqlite3_bind_text(d->history_stmt[stmt_id], 2, msg->contact_id, -1, SQLITE_STATIC) != SQLITE_OK))
	{
		tprintf("SQLITE BIND ERROR: (%ld) %ls", rc, sqlite3_errmsg(d->history_database));
		return rc;
	}

	if((rc = sqlite3_bind_int(d->history_stmt[stmt_id], 3, timestamp)) != SQLITE_OK)
	{
		tprintf("SQLITE BIND ERROR: (%ld) %ls", rc, sqlite3_errmsg(d->history_database));
		return rc;
	}

	while((rc = sqlite3_step(d->history_stmt[stmt_id])) == SQLITE_ROW)
	{
		ULONG flags, timestamp, content_len;
		STRPTR contactid;
		UBYTE *content = NULL;

		flags = sqlite3_column_int(d->history_stmt[stmt_id], 0);
		timestamp = sqlite3_column_int(d->history_stmt[stmt_id], 1);
		contactid = (STRPTR)sqlite3_column_text(d->history_stmt[stmt_id], 2);
		content_len = sqlite3_column_bytes(d->history_stmt[stmt_id], 3);

		if((flags & HISTORY_MESSAGES_NORMAL) || (flags & HISTORY_MESSAGES_SYSTEM))
		{
			content = (UBYTE*)sqlite3_column_text(d->history_stmt[stmt_id], 3);
		}
		else
		{
			/* TODO: images as blob? */
		}

		tprintf("RETURNED: %ld %ld %ls %ls\n", flags, timestamp, strd(contactid), strd(content));

		if(DoMethod(msg->callback_obj, msg->callback_method, flags, timestamp, contactid, content, content_len) != 0)
			return -2;

		rows_no++;
	}

	return rows_no;
}

static IPTR ApplicationGetLastConversation(Class *cl, Object *obj, struct APPP_GetLastConversation *msg)
{
	struct ApplicationData *d = INST_DATA(cl, obj);
	const LONG stmt_id = SQL_SELECT_LAST_CONVERSATION;
	int rc;
	ULONG rows_no = 0;

	sqlite3_clear_bindings(d->history_stmt[stmt_id]);
	sqlite3_reset(d->history_stmt[stmt_id]);

	if((rc = sqlite3_bind_int(d->history_stmt[stmt_id], 1, msg->plugin_id)) != SQLITE_OK)
	{
		tprintf("SQLITE BIND ERROR: (%ld) %ls", rc, sqlite3_errmsg(d->history_database));
		return rc;
	}

	if((rc = sqlite3_bind_text(d->history_stmt[stmt_id], 2, msg->contact_id, -1, SQLITE_STATIC) != SQLITE_OK))
	{
		tprintf("SQLITE BIND ERROR: (%ld) %ls", rc, sqlite3_errmsg(d->history_database));
		return rc;
	}

	while((rc = sqlite3_step(d->history_stmt[stmt_id])) == SQLITE_ROW)
	{
		if(msg->conversation_id)
			*msg->conversation_id = sqlite3_column_int64(d->history_stmt[stmt_id], 0);

		if(msg->timestamp)
			*msg->timestamp = sqlite3_column_int(d->history_stmt[stmt_id], 1);

		rows_no++;
	}

	return rows_no;
}

static IPTR ApplicationGetContactsFromHistory(Class *cl, Object *obj, struct APPP_GetContactsFromHistory *msg)
{
	struct ApplicationData *d = INST_DATA(cl, obj);
	const LONG stmt_id = SQL_SELECT_CONTACTS;
	struct HContactsListEntry e;
	struct ContactEntry *entry;
	Object *contacts_list = findobj(USD_CONTACTS_LIST, d->main_window);
	ULONG rows_no = 0;
	LONG i;
	int rc;

	sqlite3_clear_bindings(d->history_stmt[stmt_id]);
	sqlite3_reset(d->history_stmt[stmt_id]);

	set(msg->list, MUIA_List_Quiet, TRUE);

	while((rc = sqlite3_step(d->history_stmt[stmt_id])) == SQLITE_ROW)
	{
		e.id = (STRPTR)sqlite3_column_text(d->history_stmt[stmt_id], 0);
		e.name = (STRPTR)sqlite3_column_text(d->history_stmt[stmt_id], 1);
		e.plugin_id = sqlite3_column_int(d->history_stmt[stmt_id], 2);

		for(i = 0;;i++)
		{
			DoMethod(contacts_list, CLSM_GetEntry, i, &entry);

			if(entry == NULL || (entry->pluginid == e.plugin_id && StrEqu(entry->entryid, e.id)))
			{
				e.name = ContactName(entry);
				break;
			}
		}

		DoMethod(msg->list, MUIM_List_InsertSingle, &e, MUIV_List_Insert_Sorted);

		rows_no++;
	}

	set(msg->list, MUIA_List_Quiet, FALSE);

	return rows_no;
}

static IPTR ApplicationGetConversationsFromHistory(Class *cl, Object *obj, struct APPP_GetConversationsFromHistory *msg)
{
	struct ApplicationData *d = INST_DATA(cl, obj);
	Object *contacts_list = findobj(USD_CONTACTS_LIST, d->main_window);
	const LONG stmt_id = SQL_SELECT_CONVERSATIONS;
	ULONG rows_no = 0;
	int rc;

	sqlite3_clear_bindings(d->history_stmt[stmt_id]);
	sqlite3_reset(d->history_stmt[stmt_id]);

	if((rc = sqlite3_bind_int(d->history_stmt[stmt_id], 1, msg->plugin_id)) != SQLITE_OK)
	{
		tprintf("SQLITE BIND ERROR: (%ld) %ls", rc, sqlite3_errmsg(d->history_database));
		return rc;
	}

	if((rc = sqlite3_bind_text(d->history_stmt[stmt_id], 2, msg->contact_id, -1, SQLITE_STATIC) != SQLITE_OK))
	{
		tprintf("SQLITE BIND ERROR: (%ld) %ls", rc, sqlite3_errmsg(d->history_database));
		return rc;
	}

	set(msg->list, MUIA_List_Quiet, TRUE);

	while((rc = sqlite3_step(d->history_stmt[stmt_id])) == SQLITE_ROW)
	{
		struct HConversationsListEntry e;
		struct ContactEntry *entry;
		LONG i;
		ULONG plugin_id = sqlite3_column_int(d->history_stmt[stmt_id], 7);

		e.id = sqlite3_column_int64(d->history_stmt[stmt_id], 0);
		e.contact_id = (STRPTR)sqlite3_column_text(d->history_stmt[stmt_id], 1);
		e.contact_name = (STRPTR)sqlite3_column_text(d->history_stmt[stmt_id], 2);
		e.user_id = (STRPTR)sqlite3_column_text(d->history_stmt[stmt_id], 3);
		e.flags = sqlite3_column_int(d->history_stmt[stmt_id], 4);
		e.timestamp_start = sqlite3_column_int(d->history_stmt[stmt_id], 5);
		e.timestamp_end = sqlite3_column_int(d->history_stmt[stmt_id], 6);

		for(i = 0;;i++)
		{
			DoMethod(contacts_list, CLSM_GetEntry, i, &entry);

			if(entry == NULL || (entry->pluginid == plugin_id && StrEqu(entry->entryid, e.contact_id)))
			{
				e.contact_name = ContactName(entry);
				break;
			}
		}

		DoMethod(msg->list, MUIM_List_InsertSingle, &e, MUIV_List_Insert_Sorted);

		rows_no++;
	}

	set(msg->list, MUIA_List_Quiet, FALSE);

	return rows_no;
}

static IPTR ApplicationGetMessagesFromHistory(Class *cl, Object *obj, struct APPP_GetMessagesFromHistory *msg)
{
	struct ApplicationData *d = INST_DATA(cl, obj);
	const LONG stmt_id = SQL_SELECT_MESSAGES;
	int rc;
	ULONG rows_no = 0;
	ENTER();

	if(!msg->callback_obj || !msg->conversation_id)
		return -1;

	sqlite3_clear_bindings(d->history_stmt[stmt_id]);
	sqlite3_reset(d->history_stmt[stmt_id]);

	if((rc = sqlite3_bind_int64(d->history_stmt[stmt_id], 1, *msg->conversation_id)) != SQLITE_OK)
	{
		tprintf("SQLITE BIND ERROR: (%ld) %ls", rc, sqlite3_errmsg(d->history_database));
		return rc;
	}

	while((rc = sqlite3_step(d->history_stmt[stmt_id])) == SQLITE_ROW)
	{
		ULONG flags, timestamp, content_len;
		UBYTE *content = NULL;

		flags = sqlite3_column_int(d->history_stmt[stmt_id], 0);
		timestamp = sqlite3_column_int(d->history_stmt[stmt_id], 1);
		content_len = sqlite3_column_bytes(d->history_stmt[stmt_id], 2);

		if((flags & HISTORY_MESSAGES_NORMAL) || (flags & HISTORY_MESSAGES_SYSTEM))
		{
			content = (UBYTE*)sqlite3_column_text(d->history_stmt[stmt_id], 2);
		}
		else
		{
			/* TODO: images as blob? */
		}

		if(DoMethod(msg->callback_obj, msg->callback_method, flags, timestamp, content, content_len) != 0)
			return -2;

		rows_no++;
	}

	LEAVE();
	return rows_no;
}

static IPTR ApplicationDeleteContactFromHistory(Class *cl, Object *obj, struct APPP_DeleteContactFromHistory *msg)
{
	struct ApplicationData *d = INST_DATA(cl, obj);
	const LONG stmt_id = SQL_DELETE_CONTACT;
	int rc;
	ULONG rows_no = 0;
	ENTER();

	sqlite3_clear_bindings(d->history_stmt[stmt_id]);
	sqlite3_reset(d->history_stmt[stmt_id]);

	if((rc = sqlite3_bind_int(d->history_stmt[stmt_id], 1, msg->plugin_id)) != SQLITE_OK)
	{
		tprintf("SQLITE BIND ERROR: (%ld) %ls", rc, sqlite3_errmsg(d->history_database));
		return rc;
	}

	if((rc = sqlite3_bind_text(d->history_stmt[stmt_id], 2, msg->contact_id, -1, SQLITE_STATIC) != SQLITE_OK))
	{
		tprintf("SQLITE BIND ERROR: (%ld) %ls", rc, sqlite3_errmsg(d->history_database));
		return rc;
	}

	while((rc = sqlite3_step(d->history_stmt[stmt_id])) == SQLITE_ROW)
		rows_no++;

	LEAVE();
	return rows_no;
}

static IPTR ApplicationDeleteConversationFromHistory(Class *cl, Object *obj, struct APPP_DeleteConversationFromHistory *msg)
{
	struct ApplicationData *d = INST_DATA(cl, obj);
	const LONG stmt_id = SQL_DELETE_CONVERSATION;
	int rc;
	ULONG rows_no = 0;
	ENTER();

	if(!msg->conversation_id)
		return -1;

	sqlite3_clear_bindings(d->history_stmt[stmt_id]);
	sqlite3_reset(d->history_stmt[stmt_id]);

	if((rc = sqlite3_bind_int64(d->history_stmt[stmt_id], 1, *msg->conversation_id)) != SQLITE_OK)
	{
		tprintf("SQLITE BIND ERROR: (%ld) %ls", rc, sqlite3_errmsg(d->history_database));
		return rc;
	}

	while((rc = sqlite3_step(d->history_stmt[stmt_id])) == SQLITE_ROW)
		rows_no++;

	LEAVE();
	return rows_no;
}

static IPTR ApplicationDispatcher(VOID)
{
	Class *cl = (Class*)REG_A0;
	Object *obj = (Object*)REG_A2;
	Msg msg = (Msg)REG_A1;

	switch(msg->MethodID)
	{
		/* basics */
		case OM_NEW: return(ApplicationNew(cl, obj, (struct opSet*)msg));
		case OM_DISPOSE: return(ApplicationDispose(cl, obj, msg));
		case OM_SET: return(ApplicationSet(cl, obj, (struct opSet*)msg));
		case OM_GET: return(ApplicationGet(cl, obj, (struct opGet*)msg));
		case APPM_Setup: return (ApplicationSetup(cl, obj));
		case APPM_Cleanup: return (ApplicationCleanup(cl, obj));

		/* clipboard */
		case APPM_ClipboardStart: return(ApplicationClipboardStart(cl, obj, (struct APPP_ClipboardStart*)msg));
		case APPM_ClipboardWrite: return(ApplicationClipboardWrite(cl, obj, (struct APPP_ClipboardWrite*)msg));
		case APPM_ClipboardEnd: return(ApplicationClipboardEnd(cl, obj));

		/* broker */
		case APPM_InstallBroker: return(ApplicationInstallBroker(cl, obj));
		case APPM_RemoveBroker: return(ApplicationRemoveBroker(cl, obj));
		case APPM_AutoAway: return(ApplicationAutoAway(cl, obj));
		case APPM_AutoBack: return(ApplicationAutoBack(cl, obj));

		/* GUI */
		case APPM_MainLoop: return(ApplicationMainLoop(cl, obj));
		case APPM_ScreenbarChange: return(ApplicationScreenbarChange(cl, obj, (struct APPP_ScreenbarChange*)msg));
		case APPM_ScreenbarMenu: return(ApplicationScreenbarMenu(cl, obj, (struct APPP_ScreenbarMenu*)msg));
		case APPM_Screenbarize: return(ApplicationScreenbarScreenbarize(cl, obj));
		case APPM_ScreenbarUnread: return(ApplicationScreenbarUnread(cl, obj));
		case APPM_NotifyBeacon: return(ApplicationNotifyBeacon(cl, obj, (struct APPP_NotifyBeacon*)msg));
		case APPM_ConvertContactEntry: return(ApplicationConvertContactEntry(cl, obj, (struct APPP_ConvertContactEntry*)msg));
		case APPM_SetLastStatus: return(ApplicationSetLastStatus(cl, obj, (struct APPP_SetLastStatus*)msg));

		/* http */
		case APPM_SendHttpGet: return(ApplicationSendHttpGet(cl, obj, (struct APPP_SendHttpGet*)msg));
		case APPM_SendHttpPost: return(ApplicationSendHttpPost(cl, obj, (struct APPP_SendHttpPost*)msg));

		/* ftp */
		case APPM_FtpPut: return(ApplicationFtpPut(cl, obj, (struct APPP_FtpPut*)msg));
		case APPM_FtpPutActiveTab: return(ApplicationFtpPutActiveTab(cl, obj));
		case APPM_FtpPutCallback: return(ApplicationFtpPutCallback(cl, obj, (struct APPP_FtpPutCallback*)msg));

		/* modules -> basics */
		case APPM_OpenModules: return(ApplicationOpenModules(cl, obj));
		case APPM_CloseModules: return(ApplicationCloseModules(cl, obj));
		case APPM_SecTrigger: return(ApplicationSecTrigger(cl, obj));
		case APPM_AddModulesGui: return(ApplicationAddModulesGui(cl, obj));
		case APPM_GetModuleName: return(ApplicationGetModuleName(cl, obj, (struct APPP_GetModuleName*)msg));
		case APPM_GetModuleUserId: return(ApplicationGetModuleUserId(cl, obj, (struct APPP_GetModuleUserId*)msg));

		/* modules -> orders */
		case APPM_Connect: return(ApplicationConnect(cl, obj, (struct APPP_Connect*)msg));
		case APPM_Disconnect: return(ApplicationDisconnect(cl, obj, (struct APPP_Disconnect*)msg));
		case APPM_ChangeStatus: return(ApplicationChangeStatus(cl, obj, (struct APPP_ChangeStatus*)msg));
		case APPM_NotifyContactList: return(ApplicationNotifyContactList(cl, obj, (struct APPP_NotifyContactList*)msg));
		case APPM_SendMessage: return(ApplicationSendMessage(cl, obj, (struct APPP_SendMessage*)msg));
		case APPM_SendTypingNotify: return(ApplicationSendTypingNotify(cl, obj, (struct APPP_SendTypingNotify*)msg));
		case APPM_AddNotify: return(ApplicationAddNotify(cl, obj, (struct APPP_AddNotify*)msg));
		case APPM_RemoveNotify: return(ApplicationRemoveNotify(cl, obj, (struct APPP_RemoveNotify*)msg));
		case APPM_ImportList: return(ApplicationImportList(cl, obj));
		case APPM_ExportList: return(ApplicationExportList(cl, obj));
		case APPM_SendPicture: return(ApplicationSendPicture(cl, obj, (struct APPP_SendPicture*)msg));
		case APPM_AnswerInvite: return(ApplicationAnswerInvite(cl, obj, (struct APPP_AnswerInvite*)msg));
		case APPM_PubDirRequest: return(ApplicationPubDirRequest(cl, obj, (struct APPP_PubDirRequest*)msg));

		/* modules -> events */
		case APPM_ParseEvent: return(ApplicationParseEvent(cl, obj, (struct APPP_ParseEvent*)msg));
		case APPM_ListChangeAck: return(ApplicationListChangeAck(cl, obj, (struct APPP_ListChangeAck*)msg));
		case APPM_NewMessageAck: return(ApplicationNewMessageAck(cl, obj, (struct APPP_NewMessageAck*)msg));
		case APPM_StatusChangeAck: return(ApplicationStatusChangeAck(cl, obj, (struct APPP_StatusChangeAck*)msg));
		case APPM_DisconnectAck: return(ApplicationDisconnectAck(cl, obj, (struct APPP_DisconnectAck*)msg));
		case APPM_TypingNotifyAck: return(ApplicationTypingNotifyAck(cl, obj, (struct APPP_TypingNotifyAck*)msg));
		case APPM_ConnectAck: return(ApplicationConnectAck(cl, obj, (struct APPP_ConnectAck*)msg));
		case APPM_ModuleMessageAck: return(ApplicationModuleMessageAck(cl, obj, (struct APPP_ModuleMessageAck*)msg));
		case APPM_NewAvatarAck: return(ApplicationNewAvatarAck(cl, obj, (struct APPP_NewAvatarAck*)msg));
		case APPM_ImportListAck: return(ApplicationImportListAck(cl, obj, (struct APPP_ImportListAck*)msg));
		case APPM_ExportListAck: return(ApplicationExportListAck(cl, obj, (struct APPP_ExportListAck*)msg));
		case APPM_NewPictureAck: return(ApplicationNewPictureAck(cl, obj, (struct APPP_NewPictureAck*)msg));
		case APPM_NewInviteAck: return(ApplicationNewInviteAck(cl, obj, (struct APPP_NewInviteAck*)msg));

		/* modules -> error msgs */
		case APPM_ErrorNoMem: return(ApplicationErrorNoMem(cl, obj, (struct APPP_ErrorNoMem*)msg));
		case APPM_ErrorConnFail: return(ApplicationErrorConnFail(cl, obj, (struct APPP_ErrorConnFail*)msg));
		case APPM_ErrorLoginFail: return(ApplicationErrorLoginFail(cl, obj, (struct APPP_ErrorLoginFail*)msg));
		case APPM_ErrorNotSupported: return(ApplicationErrorNotSupported(cl, obj, (struct APPP_ErrorNotSupported*)msg));

		/* history database */
		case APPM_OpenHistoryDatabase: return(ApplicationOpenHistoryDatabase(cl, obj));
		case APPM_CloseHistoryDatabase: return(ApplicationCloseHistoryDatabase(cl, obj));
		case APPM_DoSqlOnHistoryDatabase: return(ApplicationDoSqlOnHistoryDatabase(cl, obj, (struct APPP_DoSqlOnHistoryDatabase*)msg));
		case APPM_InsertMessageIntoHistory: return(ApplicationInsertMessageIntoHistory(cl, obj, (struct APPP_InsertMessageIntoHistory*)msg));
		case APPM_InsertConversationIntoHistory: return(ApplicationInsertConversationIntoHistory(cl, obj, (struct APPP_InsertConversationIntoHistory*)msg));
		case APPM_GetLastMessagesFromHistory: return(ApplicationGetLastMessagesFromHistory(cl, obj, (struct APPP_GetLastMessagesFromHistory*)msg));
		case APPM_GetLastConvMsgsFromHistory: return(ApplicationGetLastCovMsgsFromHistory(cl, obj, (struct APPP_GetLastConvMsgsFromHistory*)msg));
		case APPM_GetLastMessagesByTime: return(ApplicationGetLastMessagesByTime(cl, obj, (struct APPP_GetLastMessagesByTime*)msg));
		case APPM_GetLastConversation: return(ApplicationGetLastConversation(cl, obj, (struct APPP_GetLastConversation*)msg));
		case APPM_GetContactsFromHistory: return(ApplicationGetContactsFromHistory(cl, obj, (struct APPP_GetContactsFromHistory*)msg));
		case APPM_GetConversationsFromHistory: return(ApplicationGetConversationsFromHistory(cl, obj, (struct APPP_GetConversationsFromHistory*)msg));
		case APPM_GetMessagesFromHistory: return(ApplicationGetMessagesFromHistory(cl, obj, (struct APPP_GetMessagesFromHistory*)msg));
		case APPM_DeleteContactFromHistory: return(ApplicationDeleteContactFromHistory(cl, obj, (struct APPP_DeleteContactFromHistory*)msg));
		case APPM_DeleteConversationFromHistory: return(ApplicationDeleteConversationFromHistory(cl, obj, (struct APPP_DeleteConversationFromHistory*)msg));

		default: return(DoSuperMethodA(cl, obj, msg));
	}
}

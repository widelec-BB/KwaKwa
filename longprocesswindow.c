#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/sqlite.h>
#include <mui/Busy_mcc.h>
#include <libvstring.h>

#include "globaldefines.h"
#include "locale.h"
#include "support.h"
#include "longprocesswindow.h"

enum TaskType
{
	TASK_UPDATE_HISTORY_DATABASE = 0,
};

/* history database upgrades */
enum SQL_E
{
	SQL_SELECT_DB_VERSION,
	SQL_INSERT_DB_VERSION,
	/* v1 - unicode introduction */
	SQL_COUNT_CONVERSATIONS,
	SQL_COUNT_MESSAGES,
	SQL_UPDATE_CONVERSATION_TEXTS,
	SQL_UPDATE_MESSAGES_TEXTS,
	SQL_SELECT_CONVERSATIONS_FOR_TEXTS_UPDATE,
	SQL_SELECT_MESSAGES_FOR_TEXTS_UPDATE,
};
static CONST TEXT *SQL[] =
{
	"SELECT v.id, v.aTimestamp FROM db_version as v ORDER BY id DESC LIMIT 1;",
	"INSERT INTO db_version VALUES(?, ?);",
	"SELECT COUNT(c.id) FROM conversations as c;",
	"SELECT COUNT(m.id) FROM messages as m;",
	"UPDATE conversations SET contactId = ?, contactName = ?, userId = ? WHERE id = ?;",
	"UPDATE messages SET contactId = ?, contactName = ?, content = ? WHERE id = ?;",
	"SELECT c.id, c.contactId, c.contactName, c.userId FROM conversations as c;",
	"SELECT m.id, m.contactId, m.contactName, m.content FROM messages as m;",
};
#define SQL_STMT_NO (sizeof(SQL) / sizeof(*SQL))

struct LPWP_UpdateHistoryDatabase {ULONG MethodID; LONG db_version;};

static IPTR LongProcessWindowDispatcher(VOID);
const struct EmulLibEntry LongProcessWindowGate = {TRAP_LIB, 0, (VOID(*)(VOID))LongProcessWindowDispatcher};

struct LongProcessWindowData
{
	Object *lpwd_StatusMessage, *lpwd_Gauge, *lpwd_BusyBar;
	Object *lpwd_Process;

	enum TaskType lpwd_TaskType; 
	ULONG lpwd_CurrentDatabaseVersion;
	LONG lpwd_Result;
	struct Task *lpwd_AppTask; 

	STRPTR win_title;
};

struct MUI_CustomClass *CreateLongProcessWindowClass(VOID)
{
	struct MUI_CustomClass *cl;

	cl = MUI_CreateCustomClass(NULL, MUIC_Window, NULL, sizeof(struct LongProcessWindowData), (APTR)&LongProcessWindowGate);
	return cl;
}

static IPTR LongProcessWindowNew(Class *cl, Object *obj, struct opSet *msg)
{
	Object *status_message, *progress_gauge, *stop_button, *busy_bar;

	obj = (Object*)DoSuperNew(cl, obj,
		MUIA_Background, MUII_WindowBack,
		MUIA_Window_Title, (IPTR)GetString(MSG_LONG_PROCESS_WINDOW_TITLE),
		MUIA_Window_Open, FALSE,
		MUIA_Window_CloseGadget, FALSE,
		MUIA_Window_NoMenus, TRUE,
		MUIA_Window_Width, MUIV_Window_Width_Visible(30),
		MUIA_Window_ScreenTitle, (IPTR)APP_SCREEN_TITLE,
		MUIA_Window_RootObject, (IPTR)MUI_NewObject(MUIC_Group,
			MUIA_Group_Child, (IPTR)(progress_gauge = MUI_NewObject(MUIC_Gauge,
				MUIA_Unicode, TRUE,
				MUIA_Gauge_Horiz, TRUE,
				MUIA_Gauge_InfoText, "%ld %%",
			TAG_END)),
			MUIA_Group_Child, (busy_bar = MUI_NewObject(MUIC_Busy,
				MUIA_ShowMe, FALSE,
				MUIA_Busy_Speed, MUIV_Busy_Speed_User,
				MUIA_FixHeightTxt, "MM",
			TAG_END)),
			MUIA_Group_Child, (IPTR)(status_message = MUI_NewObject(MUIC_Text,
				MUIA_Unicode, TRUE,
				MUIA_Text_PreParse, "\33c",
			TAG_END)),
			MUIA_Group_Child, (IPTR)MUI_NewObject(MUIC_Text,
				MUIA_Unicode, TRUE,
				MUIA_Text_PreParse, "\33c",
				MUIA_Text_Contents, GetString(MSG_LONG_PROCESS_WINDOW_PLEASE_WAIT),
			TAG_END),
			MUIA_Group_Child, MUI_NewObject(MUIC_Group,
				MUIA_Group_Horiz, TRUE,
				MUIA_Group_Child, EmptyRectangle(100),
				MUIA_Group_Child, (IPTR)(stop_button = NormalButton(GetString(MSG_LONG_PROCESS_WINDOW_STOP_BUTTON), *GetString(MSG_LONG_PROCESS_WINDOW_STOP_BUTTON_HOTKEY), 0L, 0)),
				MUIA_Group_Child, EmptyRectangle(100),
			TAG_END),
		TAG_END),
	TAG_MORE, (IPTR)msg->ops_AttrList);

	if(obj)
	{
		struct LongProcessWindowData *d = INST_DATA(cl, obj);

		if((d->win_title = Utf8ToSystem((STRPTR)xget(obj, MUIA_Window_Title))))
			set(obj, MUIA_Window_Title, (IPTR)d->win_title);

		d->lpwd_StatusMessage = status_message;
		d->lpwd_Gauge = progress_gauge;
		d->lpwd_BusyBar = busy_bar;
		d->lpwd_AppTask = FindTask(NULL);

		d->lpwd_Process = MUI_NewObject(MUIC_Process,
			MUIA_Process_Name, GetString(MSG_LONG_PROCESS_WINDOW_PROCESS_NAME),
			MUIA_Process_SourceClass, cl,
			MUIA_Process_SourceObject, obj,
			MUIA_Process_AutoLaunch, FALSE,
		TAG_DONE);

		DoMethod(stop_button, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)obj, 1, LPWM_Stop);

		if(d->lpwd_Process)
			return (IPTR)obj;
	}

	CoerceMethod(cl, obj, OM_DISPOSE);

	return (IPTR)NULL;
}

static IPTR LongProcessWindowDispose(Class *cl, Object *obj, Msg msg)
{
	struct LongProcessWindowData *d = INST_DATA(cl, obj);

	if(d->lpwd_Process)
		MUI_DisposeObject(d->lpwd_Process);

	if(d->win_title)
		StrFree(d->win_title);

	return DoSuperMethodA(cl, obj, msg);
}

static IPTR LongProcessWindowUpdateHistoryDatabase(Class *cl, Object *obj, struct LPWP_UpdateHistoryDatabase *msg)
{
	ULONG sigs = 0;
	struct LongProcessWindowData *d = INST_DATA(cl, obj);
	ENTER();

	d->lpwd_TaskType = TASK_UPDATE_HISTORY_DATABASE;
	d->lpwd_CurrentDatabaseVersion = msg->db_version;

	DoMethod(d->lpwd_Process, MUIM_Process_Launch);

	while(DoMethod(_app(obj), MUIM_Application_NewInput, (IPTR)&sigs) != MUIV_Application_ReturnID_Quit)
	{
		if(sigs)
		{
			sigs = Wait(sigs | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D);
			if(sigs & SIGBREAKF_CTRL_C)
			{
				d->lpwd_Result = -2;
				break;
			}
			if(sigs & SIGBREAKF_CTRL_D)
				break;
		}
	}

	LEAVE();
	return (IPTR)d->lpwd_Result == 0 ? TRUE : FALSE;
}


static LONG BackupDatabaseFile(LONG db_version, Object *gauge)
{
	STRPTR backup_name;
	LONG result = -1;
	ENTER();

	if((backup_name = FmtNew(HISTORY_DATABASE_PATH".v%ld.bak", db_version)))
	{
		BPTR db;

		if((db = Open(HISTORY_DATABASE_PATH, MODE_OLDFILE)))
		{
			struct FileInfoBlock fib;

			if(ExamineFH(db, &fib))
			{
				BPTR bak;

				if((bak = Open(backup_name, MODE_NEWFILE)))
				{
					UBYTE buffer[4096];
					LONG r, total = 0;

					while(total < fib.fib_Size)
					{
						ULONG signals = SetSignal(0L, SIGBREAKF_CTRL_C);

						if(signals & SIGBREAKF_CTRL_C)
							break;

						r = Read(db, buffer, sizeof(buffer));
						if(r == 0)
							break;

						if(Write(bak, buffer, r) != r)
							break;

						total += r;

						DoMethod(_app(gauge), MUIM_Application_PushMethod, (IPTR)gauge, 3,
						 MUIM_Set, MUIA_Gauge_Current, (ULONG)((total / (DOUBLE)fib.fib_Size) * 100.));
					}
					if(total == fib.fib_Size)
						result = 0;

					Close(bak);
				}

				if(result != 0)
					DeleteFile(backup_name);

			}
			Close(db);
		}
		StrFree(backup_name);
	}

	LEAVE();
	return result;
}

static LONG OpenHistoryDatabase(sqlite3 **db, sqlite3_stmt **stmts)
{
	LONG rc, i;
	ENTER();

	if((rc = sqlite3_open_v2(HISTORY_DATABASE_PATH, db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL)) == SQLITE_OK)
	{
		for(i = 0; i < SQL_STMT_NO; i++)
		{
			if((rc = sqlite3_prepare_v2(*db, SQL[i], -1, &stmts[i], NULL)) != SQLITE_OK)
				break;
		}

		if(i == SQL_STMT_NO)
		{
			sqlite3_clear_bindings(stmts[SQL_SELECT_DB_VERSION]);
			sqlite3_reset(stmts[SQL_SELECT_DB_VERSION]);

			rc = sqlite3_step(stmts[SQL_SELECT_DB_VERSION]);
			if(rc == SQLITE_ROW)
				return (LONG)sqlite3_column_int(stmts[SQL_SELECT_DB_VERSION], 0);
			else if(rc == SQLITE_DONE)
				return 0;
		}
	}

	LEAVE();
	return -1;
}

static LONG UpdateHistoryDatabase(Object *lpw, Object *gauge, Object *status_txt, LONG db_version, sqlite3_stmt **stmts)
{
	LONG total = 0, done = 0;
	int rc = 0;

	tprintf("current database version: %ld\n", db_version);

	if(db_version < 1)
	{
		sqlite3_clear_bindings(stmts[SQL_COUNT_CONVERSATIONS]);
		sqlite3_reset(stmts[SQL_COUNT_CONVERSATIONS]);

		if(sqlite3_step(stmts[SQL_COUNT_CONVERSATIONS]) != SQLITE_ROW)
			return -1;

		total = sqlite3_column_int(stmts[SQL_COUNT_CONVERSATIONS], 0);

		sqlite3_clear_bindings(stmts[SQL_COUNT_MESSAGES]);
		sqlite3_reset(stmts[SQL_COUNT_MESSAGES]);

		if(sqlite3_step(stmts[SQL_COUNT_MESSAGES]) != SQLITE_ROW)
			return -1;

		total += sqlite3_column_int(stmts[SQL_COUNT_MESSAGES], 0);

		tprintf("total entries to convert: %ld\n", total);

		if(total > 0)
		{
			DoMethod(_app(lpw), MUIM_Application_PushMethod, (IPTR)status_txt, 3,
			 MUIM_Set, MUIA_Text_Contents, (IPTR)GetString(MSG_UPDATE_DATABASE_UNICODE));
			DoMethod(_app(lpw), MUIM_Application_PushMethod, (IPTR)gauge, 3,
			 MUIM_Set, MUIA_Gauge_Current, 0);

			sqlite3_clear_bindings(stmts[SQL_SELECT_CONVERSATIONS_FOR_TEXTS_UPDATE]);
			sqlite3_reset(stmts[SQL_SELECT_CONVERSATIONS_FOR_TEXTS_UPDATE]);

			while(sqlite3_step(stmts[SQL_SELECT_CONVERSATIONS_FOR_TEXTS_UPDATE]) == SQLITE_ROW)
			{
				LONG id;
				STRPTR contactId, contactName, userId;
				ULONG signals = SetSignal(0L, SIGBREAKF_CTRL_C);

				if(signals & SIGBREAKF_CTRL_C)
					return -1;

				id = sqlite3_column_int(stmts[SQL_SELECT_CONVERSATIONS_FOR_TEXTS_UPDATE], 0);
				contactId = SystemToUtf8((STRPTR)sqlite3_column_text(stmts[SQL_SELECT_CONVERSATIONS_FOR_TEXTS_UPDATE], 1));
				contactName = SystemToUtf8((STRPTR)sqlite3_column_text(stmts[SQL_SELECT_CONVERSATIONS_FOR_TEXTS_UPDATE], 2));
				userId = SystemToUtf8((STRPTR)sqlite3_column_text(stmts[SQL_SELECT_CONVERSATIONS_FOR_TEXTS_UPDATE], 3));

				sqlite3_clear_bindings(stmts[SQL_UPDATE_CONVERSATION_TEXTS]);
				sqlite3_reset(stmts[SQL_UPDATE_CONVERSATION_TEXTS]);

				if((rc = sqlite3_bind_text(stmts[SQL_UPDATE_CONVERSATION_TEXTS], 1, contactId, -1, SQLITE_STATIC)) != SQLITE_OK)
					return -1;
				if(sqlite3_bind_text(stmts[SQL_UPDATE_CONVERSATION_TEXTS], 2, contactName, -1, SQLITE_STATIC) != SQLITE_OK)
					return -1;
				if(sqlite3_bind_text(stmts[SQL_UPDATE_CONVERSATION_TEXTS], 3, userId, -1, SQLITE_STATIC) != SQLITE_OK)
					return -1;
				if(sqlite3_bind_int(stmts[SQL_UPDATE_CONVERSATION_TEXTS], 4, id) != SQLITE_OK)
					return -1;
				if(sqlite3_step(stmts[SQL_UPDATE_CONVERSATION_TEXTS]) != SQLITE_DONE)
					return -1;

				if(contactId)
					StrFree(contactId);
				if(contactName)
					StrFree(contactName);
				if(userId)
					StrFree(userId);

				done++;

				DoMethod(_app(lpw), MUIM_Application_PushMethod, (IPTR)gauge, 3,
				 MUIM_Set, MUIA_Gauge_Current, (ULONG)((done / (DOUBLE)total) * 100.));
			}

			sqlite3_clear_bindings(stmts[SQL_SELECT_MESSAGES_FOR_TEXTS_UPDATE]);
			sqlite3_reset(stmts[SQL_SELECT_MESSAGES_FOR_TEXTS_UPDATE]);

			while(sqlite3_step(stmts[SQL_SELECT_MESSAGES_FOR_TEXTS_UPDATE]) == SQLITE_ROW)
			{
				LONG id;
				STRPTR contactId, contactName, content;
				ULONG signals = SetSignal(0L, SIGBREAKF_CTRL_C);

				if(signals & SIGBREAKF_CTRL_C)
					return -1;

				id = sqlite3_column_int(stmts[SQL_SELECT_MESSAGES_FOR_TEXTS_UPDATE], 0);
				contactId = SystemToUtf8((STRPTR)sqlite3_column_text(stmts[SQL_SELECT_MESSAGES_FOR_TEXTS_UPDATE], 1));
				contactName = SystemToUtf8((STRPTR)sqlite3_column_text(stmts[SQL_SELECT_MESSAGES_FOR_TEXTS_UPDATE], 2));
				content = SystemToUtf8((STRPTR)sqlite3_column_text(stmts[SQL_SELECT_MESSAGES_FOR_TEXTS_UPDATE], 3));

				sqlite3_clear_bindings(stmts[SQL_UPDATE_MESSAGES_TEXTS]);
				sqlite3_reset(stmts[SQL_UPDATE_MESSAGES_TEXTS]);

				if(sqlite3_bind_text(stmts[SQL_UPDATE_MESSAGES_TEXTS], 1, contactId, -1, SQLITE_STATIC) != SQLITE_OK)
					return -1;
				if(sqlite3_bind_text(stmts[SQL_UPDATE_MESSAGES_TEXTS], 2, contactName, -1, SQLITE_STATIC) != SQLITE_OK)
					return -1;
				if(sqlite3_bind_text(stmts[SQL_UPDATE_MESSAGES_TEXTS], 3, content, -1, SQLITE_STATIC) != SQLITE_OK)
					return -1;
				if(sqlite3_bind_int(stmts[SQL_UPDATE_MESSAGES_TEXTS], 4, id) != SQLITE_OK)
					return -1;
				if(sqlite3_step(stmts[SQL_UPDATE_MESSAGES_TEXTS]) != SQLITE_DONE)
					return -1;

				if(contactId)
					StrFree(contactId);
				if(contactName)
					StrFree(contactName);
				if(content)
					StrFree(content);

				done++;

				DoMethod(_app(lpw), MUIM_Application_PushMethod, (IPTR)gauge, 3,
				 MUIM_Set, MUIA_Gauge_Current, (ULONG)((done / (DOUBLE)total) * 100.));
			}
		}

		sqlite3_clear_bindings(stmts[SQL_INSERT_DB_VERSION]);
		sqlite3_reset(stmts[SQL_INSERT_DB_VERSION]);
		if(sqlite3_bind_int(stmts[SQL_INSERT_DB_VERSION], 1, 1) != SQLITE_OK)
			return -1;
		if(sqlite3_bind_int(stmts[SQL_INSERT_DB_VERSION], 2, ActLocalTime2Amiga()) != SQLITE_OK)
			return -1;
		if(sqlite3_step(stmts[SQL_INSERT_DB_VERSION]) != SQLITE_DONE)
			return -1;
	}

	return 0;
}

static IPTR LongProcessWindowProcess(Class *cl, Object *obj, struct MUIP_Process_Process *msg)
{
	struct LongProcessWindowData *d = INST_DATA(cl, obj);
	ENTER();

	d->lpwd_Result = -1;

	switch(d->lpwd_TaskType)
	{
		case TASK_UPDATE_HISTORY_DATABASE:
		{
			LONG db_version;
			sqlite3 *db;
			sqlite3_stmt *stmts[SQL_STMT_NO];

			DoMethod(_app(d->lpwd_Gauge), MUIM_Application_PushMethod, (IPTR)d->lpwd_Gauge, 3, MUIM_Set, MUIA_Gauge_Current, 0);
			DoMethod(_app(d->lpwd_StatusMessage), MUIM_Application_PushMethod, (IPTR)d->lpwd_StatusMessage, 3, 
				MUIM_Set, MUIA_Text_Contents, (IPTR)GetString(MSG_LONG_PROCESS_WINDOW_STATUS_DATABASE_BACKUP));

			if(BackupDatabaseFile(d->lpwd_CurrentDatabaseVersion, d->lpwd_Gauge) != 0)
				break;

			DoMethod(_app(d->lpwd_Gauge), MUIM_Application_PushMethod, (IPTR)d->lpwd_Gauge, 3, MUIM_Set, MUIA_Gauge_Current, 100);
			DoMethod(_app(d->lpwd_StatusMessage), MUIM_Application_PushMethod, (IPTR)d->lpwd_StatusMessage, 3, 
				MUIM_Set, MUIA_Text_Contents, (IPTR)NULL);

			if((db_version = OpenHistoryDatabase(&db, stmts)) != -1)
			{
				LONG i;

				d->lpwd_Result = UpdateHistoryDatabase(obj, d->lpwd_Gauge, d->lpwd_StatusMessage, db_version, stmts);

				for(i = 0; i < SQL_STMT_NO; i++)
					if(stmts[i])
						sqlite3_finalize(stmts[i]);

				if(d->lpwd_Result == 0)
				{
					DoMethod(_app(obj), MUIM_Application_PushMethod, (IPTR)d->lpwd_Gauge, 3, MUIM_Set, MUIA_ShowMe, FALSE);
					DoMethod(_app(obj), MUIM_Application_PushMethod, (IPTR)d->lpwd_BusyBar, 3, MUIM_Set, MUIA_ShowMe, TRUE);
					DoMethod(_app(obj), MUIM_Application_PushMethod, (IPTR)d->lpwd_StatusMessage, 3, MUIM_Set, MUIA_Text_Contents, (IPTR)GetString(MSG_LONG_PROCESS_WINDOW_STATUS_DATABASE_VACUUM));

					if(sqlite3_exec(db, "VACUUM", 0, 0, 0) != SQLITE_OK)
						d->lpwd_Result = -1;
				}
				sqlite3_close(db);
			}
		}
		break;
	}

	Signal(d->lpwd_AppTask, SIGBREAKF_CTRL_D);

	LEAVE();
	return (IPTR)0;
}

static IPTR LongProcessWindowStop(Class *cl, Object *obj)
{
	struct LongProcessWindowData *d = INST_DATA(cl, obj);

	DoMethod(d->lpwd_Process, MUIM_Process_Signal, SIGBREAKF_CTRL_C);

	return (IPTR)0;
}

static IPTR LongProcessWindowDispatcher(VOID)
{
	Class *cl = (Class*)REG_A0;
	Object *obj = (Object*)REG_A2;
	Msg msg = (Msg)REG_A1;

	switch (msg->MethodID)
	{
		case OM_NEW: return(LongProcessWindowNew(cl, obj, (struct opSet*)msg));
		case OM_DISPOSE: return(LongProcessWindowDispose(cl, obj, msg));
		case LPWM_UpdateHistoryDatabase: return(LongProcessWindowUpdateHistoryDatabase(cl, obj, (struct LPWP_UpdateHistoryDatabase*)msg));
		case LPWM_Stop: return(LongProcessWindowStop(cl, obj));
		case MUIM_Process_Process: return(LongProcessWindowProcess(cl, obj, (struct MUIP_Process_Process *)msg));
		default: return(DoSuperMethodA(cl, obj, msg));
	}

}
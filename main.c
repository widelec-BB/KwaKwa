/*
 * Copyright (c) 2012-2022 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/charsets.h>
#include <proto/openurl.h>
#include <proto/locale.h>
#include <libvstring.h>

#include "locale.h"

#include "globaldefines.h"
#include "application.h"
#include "contactslist.h"
#include "mainwindow.h"
#include "descwindow.h"
#include "prefswindow.h"
#include "smallsbar.h"
#include "talkwindow.h"
#include "editconwindow.h"
#include "talktab.h"
#include "virtualtext.h"
#include "title_class.h"
#include "inputfield.h"
#include "contactinfoblock.h"
#include "percentageslider.h"
#include "simplestringlist.h"
#include "modulescycle.h"
#include "emoticon.h"
#include "modulesmsglist.h"
#include "moduleslogwindow.h"
#include "slaveprocess.h"
#include "timeslider.h"
#include "pictureview.h"
#include "minmaxslider.h"
#include "fileview.h"
#include "historycontactslist.h"
#include "historyconversationslist.h"
#include "historywindow.h"
#include "tabtitle.h"
#include "support.h"
#include "logs.h"

struct Library *MUIMasterBase, *CharsetsBase, *OpenURLBase, *UtilityBase, *EzxmlBase, *SocketBase, *LocaleBase, *WorkbenchBase, *SQLiteBase;
struct Library *ClipboardBase;

BOOL StartApp(VOID)
{
	BYTE error[128];

	if(!(LocaleBase = (APTR) OpenLibrary("locale.library", 53)))
	{
		PutStr("Cannot open locale.library (V53)\n");
		return FALSE;
	}
	Locale_Open("kwakwa.catalog", 5, 0);
	if(!(MUIMasterBase = OpenLibrary("muimaster.library", 22)))
	{
		PutStr("Cannot open muimaster.library (V22)\n");
		FmtNPut((STRPTR)error, GetString(MSG_OPEN_LIBRARY_FAILED), 128, "muimaster.library (V22)");
		MUI_Request_Unicode(NULL, NULL, APP_NAME, "*_OK", (STRPTR)error, NULL);
		return FALSE;
	}
	if(!(CharsetsBase = OpenLibrary("charsets.library", 53)))
	{
		PutStr("Cannot open charsets.library (V53)!\n");
		FmtNPut((STRPTR)error, GetString(MSG_OPEN_LIBRARY_FAILED), 128, "charsets.library (V53)");
		MUI_Request_Unicode(NULL, NULL, APP_NAME, "*_OK", (STRPTR)error, NULL);
		return FALSE;
	}
	if(!(OpenURLBase = OpenLibrary("openurl.library", 8)))
	{
		PutStr("Cannot open openurl.library (V8)!\n");
		FmtNPut((STRPTR)error, GetString(MSG_OPEN_LIBRARY_FAILED), 128, "openurl.library (V8)");
		MUI_Request_Unicode(NULL, NULL, APP_NAME, "*_OK", (STRPTR)error, NULL);
		return FALSE;
	}
	if(!(EzxmlBase = OpenLibrary("ezxml.library", 8)))
	{
		PutStr("Cannot open ezxml.library (V8)!\n");
		FmtNPut((STRPTR)error, GetString(MSG_OPEN_LIBRARY_FAILED), 128, "ezxml.library (V8)");
		MUI_Request_Unicode(NULL, NULL, APP_NAME, "*_OK", (STRPTR)error, NULL);
		return FALSE;
	}
	if(!(UtilityBase = OpenLibrary("utility.library", 39)))
	{
		PutStr("Cannot open utility.library (V39)!\n");
		FmtNPut((STRPTR)error, GetString(MSG_OPEN_LIBRARY_FAILED), 128, "utility.library (V39)");
		MUI_Request_Unicode(NULL, NULL, APP_NAME, "*_OK", (STRPTR)error, NULL);
		return FALSE;
	}
	if(!(SocketBase = OpenLibrary("bsdsocket.library", 0)))
	{
		PutStr("Cannot open bsdsocket.library!");
		FmtNPut((STRPTR)error, GetString(MSG_OPEN_LIBRARY_FAILED), 128, "bsdsocket.library");
		MUI_Request_Unicode(NULL, NULL, APP_NAME, "*_OK", (STRPTR)error, NULL);
		return FALSE;
	}
	if(!(WorkbenchBase = OpenLibrary("workbench.library", 44)))
	{
		PutStr("Cannot open workbench.library (V44)!\n");
		FmtNPut((STRPTR)error, GetString(MSG_OPEN_LIBRARY_FAILED), 128, "workbench.library (V44)");
		MUI_Request_Unicode(NULL, NULL, APP_NAME, "*_OK", (STRPTR)error, NULL);
		return FALSE;
	}
	if(!(SQLiteBase = OpenLibrary("sqlite.library", 53)))
	{
		PutStr("Cannot open sqlite.library (V53)!\n");
		FmtNPut((STRPTR)error, GetString(MSG_OPEN_LIBRARY_FAILED), 128, "sqlite.library (V53)");
		MUI_Request_Unicode(NULL, NULL, APP_NAME, "*_OK", (STRPTR)error, NULL);
		return FALSE;
	}
	if(!(ClipboardBase = OpenLibrary("clipboard.library", 53)))
	{
		PutStr("Cannot open clipboard.library (V53)!\n");
		FmtNPut((STRPTR)error, GetString(MSG_OPEN_LIBRARY_FAILED), 128, "clipboard.library (V53)");
		MUI_Request_Unicode(NULL, NULL, APP_NAME, "*_OK", (STRPTR)error, NULL);
		return FALSE;
	}
	if(!SetupLogsSystem())
		return FALSE;

	return TRUE;
}

VOID QuitApp(VOID)
{
	CleanupLogsSystem();
	if(ClipboardBase) CloseLibrary(ClipboardBase);
	if(SQLiteBase) CloseLibrary(SQLiteBase);
	if(WorkbenchBase) CloseLibrary(WorkbenchBase);
	if(SocketBase) CloseLibrary(SocketBase);
	if(UtilityBase) CloseLibrary(UtilityBase);
	if(EzxmlBase) CloseLibrary(EzxmlBase);
	if(OpenURLBase) CloseLibrary(OpenURLBase);
	if(CharsetsBase) CloseLibrary(CharsetsBase);
	if(MUIMasterBase) CloseLibrary(MUIMasterBase);
	Locale_Close();
	if(LocaleBase) CloseLibrary(LocaleBase);
}

#define CreateClass(name) \
tprintf("Create %lsClass\n", #name); \
if(!Create##name##Class()) \
	return FALSE; \
tprintf("%lsClass created\n", #name); \

#define DeleteClass(name) \
tprintf("Delete %lsClass\n", #name); \
Delete##name##Class(); \
tprintf("%lsClass deleted\n", #name);

BOOL CreateClasses(VOID)
{
	CreateClass(Application);
	CreateClass(ContactsList);
	CreateClass(MainWindow);
	CreateClass(PrefsWindow);
	CreateClass(DescWindow);
	CreateClass(SmallSBar);
	CreateClass(TalkWindow);
	CreateClass(Title);
	CreateClass(TalkTab);
	CreateClass(PercentageSlider);
	CreateClass(ContactInfoBlock);
	CreateClass(VirtualText);
	CreateClass(SimpleStringList);
	CreateClass(InputFieldUnicode);
	CreateClass(Emoticon);
	CreateClass(ModulesCycle);
	CreateClass(EditContactWindow);
	CreateClass(ModulesMsgList);
	CreateClass(ModulesLogWindow);
	CreateClass(SlaveProcess);
	CreateClass(TimeSlider);
	CreateClass(PictureView);
	CreateClass(MinMaxSlider);
	CreateClass(TabTitle);
	CreateClass(FileView);
	CreateClass(HistoryWindow);
	CreateClass(HistoryContactsList);
	CreateClass(HistoryConversationsList);

	CreateInputFieldClass(); /* do not check result here, TextEditor.mcc is optional now. */

	return TRUE;
}


VOID DeleteClasses(VOID)
{
	DeleteClass(InputField);

	DeleteClass(HistoryConversationsList);
	DeleteClass(HistoryContactsList);
	DeleteClass(HistoryWindow);
	DeleteClass(FileView);
	DeleteClass(TabTitle);
	DeleteClass(MinMaxSlider);
	DeleteClass(PictureView);
	DeleteClass(TimeSlider);
	DeleteClass(SlaveProcess);
	DeleteClass(ModulesLogWindow);
	DeleteClass(ModulesMsgList);
	DeleteClass(EditContactWindow);
	DeleteClass(ModulesCycle);
	DeleteClass(Emoticon);
	DeleteClass(InputFieldUnicode);
	DeleteClass(SimpleStringList);
	DeleteClass(VirtualText);
	DeleteClass(ContactInfoBlock);
	DeleteClass(PercentageSlider);
	DeleteClass(TalkTab);
	DeleteClass(Title);
	DeleteClass(TalkWindow);
	DeleteClass(SmallSBar);
	DeleteClass(DescWindow);
	DeleteClass(PrefsWindow);
	DeleteClass(MainWindow);
	DeleteClass(ContactsList);
	DeleteClass(Application);
}

int main(VOID)
{
	if(StartApp())
	{
		if(CheckForOtherCopy("KWAKWA.1", TRUE))
		{
			tprintf("Sorry, there is another copy running.\n");

			Beep();

			MUI_Request_Unicode(NULL, NULL, APP_NAME, GetString(MSG_KWAKWA_ANOTHER_COPY_RUNNING_BUTTONS), GetString(MSG_KWAKWA_ANOTHER_COPY_RUNNING_MSG));

			QuitApp();
			return -1;
		}

		if(CreateClasses())
		{
			Object *app;

			if((app = NewObject(ApplicationClass->mcc_Class, NULL, TAG_END)))
			{
				tprintf("Gui constructed\n");

				if(DoMethod(app, APPM_Setup))
				{
					tprintf("Entering main loop...\n");
					do
					{
						DoMethod(app, APPM_MainLoop);
					}while(!(BOOL)DoMethod(app, APPM_ConfirmQuit));
				}

				DoMethod(app, APPM_Cleanup);
				MUI_DisposeObject(app);

				tprintf("bye bye :-(\n");
			}
		}
		DeleteClasses();
	}
	QuitApp();

	return 0;
}

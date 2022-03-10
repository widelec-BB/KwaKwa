/*
 * Copyright (c) 2012-2018 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef __GLOBAL_DEFINES_H__
#define __GLOBAL_DEFINES_H__

#ifdef __DEBUG__
#include <clib/debug_protos.h>
#define tprintf(template, ...) KPrintF((CONST_STRPTR)APP_NAME " " __FILE__ " %d: " template, __LINE__ , ##__VA_ARGS__)
#define ENTER(...) KPrintF((CONST_STRPTR)APP_NAME " enters: %s\n", __PRETTY_FUNCTION__)
#define LEAVE(...) KPrintF((CONST_STRPTR)APP_NAME " leaves: %s\n", __PRETTY_FUNCTION__)
#define strd(x)(((STRPTR)x) ? (STRPTR)(x) : (STRPTR)"NULL")
#else
#define tprintf(...)
#define ENTER(...)
#define LEAVE(...)
#define strd(x)
#endif

#define TO_STRING(x) #x
#define MACRO_TO_STRING(x) TO_STRING(x)

#define APP_DATE           __AMIGADATE__
#define APP_AUTHOR         "Filip \"widelec\" Maryjañski"
#define APP_NAME           "KwaKwa"
#define APP_CYEARS         "2012 - 2022"
#define APP_BASE           "KWAKWA"
#define APP_DESC           GetString(MSG_APPLICATION_DESCRIPTION)
#define APP_VER_MAJOR      1
#define APP_VER_MINOR      9
#define APP_VER_NO         MACRO_TO_STRING(APP_VER_MAJOR)"."MACRO_TO_STRING(APP_VER_MINOR)
#define APP_VER            "$VER: " APP_NAME " " APP_VER_NO " " APP_DATE " © " APP_CYEARS " BlaBla group"
#define APP_SCREEN_TITLE   APP_NAME " " APP_VER_NO " " APP_DATE

#define APP_ABOUT    "\33b%p\33n\n\t" APP_AUTHOR "\n\n" \
                     "\33b%P\33n\n\tTomasz \"Kaczu¶\" Kaczanowski\n\tPawe³ \"stefkos\" Stefañski\n\n" \
                     "\33b%I\33n\n\33b%g\33n\n\tKonrad \"recedent\" Czuba\n\n" \
                     "\33b%T\33n\n\tMarlena \"Kimonko\" Moradewicz\n\n" \
                     "\33b%t\33n\n\tDeez^BB\n\tDrako^BB\n\tEastone\n\tGrxmrx\n\tJacaDcaps\n\tKiero\n\tKrashan\n\tLubmil\n\tPampers\n\n"

#define CACHE_DIR                "PROGDIR:cache/"
#define GUI_DIR                  "gui/"
#define EMOT_DIR                 "PROGDIR:gfx/emots/"
#define MODULES_DIR              "PROGDIR:modules/"
#define LASTMESSAGESBASE         CACHE_DIR GUI_DIR "lastmessages.db"
#define HISTORY_DATABASE_PATH    CACHE_DIR GUI_DIR "history.db"
#define LAST_STATUS_FILE         CACHE_DIR GUI_DIR "laststatus.cfg"

#define GG_MODULE_ID 0x23000000  /* for compatibility -> importing old configuration and old contacts list */

#endif

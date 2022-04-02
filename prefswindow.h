/*
 * Copyright (c) 2012-2022 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef __PREFSWINDOW_H__
#define __PREFSWINDOW_H__

#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>

extern struct MUI_CustomClass *PrefsWindowClass;

struct MUI_CustomClass *CreatePrefsWindowClass(void);
void DeletePrefsWindowClass(void);

extern Object *PreferencesWindow;

#define prefs_object(x) findobj(x, PreferencesWindow)
#define prefs_changed(obj, method) DoMethod(PreferencesWindow, MUIM_Notify, PWA_PrefsChanged, MUIV_EveryTime, obj, 1, method)

/* objects */
#define USD_PREFS_WINDOW_WIN                             0x9EDA0000
#define USD_PREFS_WINDOW_SAVE                            0x9EDA0001
#define USD_PREFS_WINDOW_USE                             0x9EDA0002
#define USD_PREFS_WINDOW_CANCEL                          0x9EDA0003
#define USD_PREFS_PAGES_LIST                             0x9EDA0004

/* gg basic prefs */
#define USD_PREFS_TW_USRNAME_STRING                      0x9EDA1000

/* USD codes 0x9EDA1000-0x9EDA1003 and 0x9EDA1006-0x9EDA1011 are forbidden! (compatibility with < 1.5) */
#define USD_PREFS_PROGRAM_CONNECT_AUTO_CHECK             0x9EDA1003
#define USD_PREFS_PROGRAM_CONNECT_AUTO_CYCLE             0x9EDA1004
#define USD_PREFS_PROGRAM_CONNECT_AUTO_STRING            0x9EDA1005
#define USD_PREFS_PROGRAM_CONNECT_AUTO_TYPE              0x9EDA1012
#define USD_PREFS_PROGRAM_MAIN_WINDOW_CLOSE_CYCLE        0x9EDA1013
#define USD_PREFS_PROGRAM_MAIN_WINDOW_SHOW_START         0x9EDA1014
#define USD_PREFS_PROGRAM_MAIN_WINDOW_HIDE_TIME_SLIDER   0x9EDA1015
#define USD_PREFS_PROGRAM_AUTOAWAY_SLIDER                0x9EDA1016
#define USD_PREFS_PROGRAM_AUTOAWAY_CYCLE                 0x9EDA1017
#define USD_PREFS_PROGRAM_AUTOAWAY_STRING                0x9EDA1018
#define USD_PREFS_PROGRAM_LASTNOTAVAIL_CYCLE             0x9EDA1019
#define USD_PREFS_PROGRAM_MAIN_WINDOW_SHOW_HIDE_BUTTON   0x9EDA101A
#define USD_PREFS_PROGRAM_CONNECT_AUTO_GROUP             0x9EDA101B

/* message list prefs */
#define USD_PREFS_ML_MYCOLOR_POPPEN                      0x9EDA1102
#define USD_PREFS_ML_FRIENDCOLOR_POPPEN                  0x9EDA1103
#define USD_PREFS_ML_MYCOLOR_OLD_POPPEN                  0x9EDA1104
#define USD_PREFS_TW_HEADLINE_BOLD                       0x9EDA1105
#define USD_PREFS_TW_HEADLINE_ITALIC                     0x9EDA1106
#define USD_PREFS_TW_HEADLINE_BACKGROUND                 0x9EDA1107
#define USD_PREFS_TW_HEADLINE_MAXWIDTH                   0x9EDA1108
#define USD_PREFS_TW_HEADLINE_COLOR                      0x9EDA1109
/* 0x9EDA110A is taken for emoticons */
#define USD_PREFS_ML_FRIENDCOLOR_OLD_POPPEN              0x9EDA110B
#define USD_PREFS_OLD_MESSAGES_TRANSPARENCY              0x9EDA110C

#define USD_PREFS_TW_SYSTEMMSG_BOLD                      0x9EDA1110
#define USD_PREFS_TW_SYSTEMMSG_ITALIC                    0x9EDA1111
#define USD_PREFS_TW_SYSTEMMSG_BACKGROUND                0x9EDA1112
#define USD_PREFS_TW_SYSTEMMSG_MAXWIDTH                  0x9EDA1113
#define USD_PREFS_TW_SYSTEMMSG_COLOR                     0x9EDA1114
#define USD_PREFS_TW_SELECTION_COLOR                     0x9EDA1115

#define USD_PREFS_TW_PICTURES_ONOFF                      0x9EDA1120
#define USD_PREFS_TW_PICTURES_MAXWIDTH                   0x9EDA1121
#define USD_PREFS_TW_TOOLBAR_ONOFF                       0x9EDA1122
#define USD_PREFS_TW_CONTACTINFOBLOCK_ONOFF              0x9EDA1123
#define USD_PREFS_TW_SYSTEMMSG_OLD_COLOR                 0x9EDA1124
#define USD_PREFS_TW_HEADLINE_OLD_COLOR                  0x9EDA1125

#define USD_PREFS_TW_SYSTEMMSG_HEADLINE                  0x9EDA1126

#define USD_PREFS_TW_HEADLINE_INSERT_NEWLINE             0x9EDA1127
#define USD_PREFS_TW_HEADLINE_REVERSE                    0x9EDA1128

#define USD_PREFS_TW_SYSTEMMSG_HEADLINE_REVERSE          0x9EDA1129

#define USD_PREFS_TW_TOOLBAR_SPACE_SIZE                  0x9EDA112A

#define USD_PREFS_TW_TABTITLE_IMAGE_ONOFF                0x9EDA112B

/* logs prefs */
#define USD_PREFS_LOGS_ONOFF_CHECK                       0x9EDA1200
#define USD_PREFS_HISTORY_ONOFF_CHECK                    0x9EDA1201
#define USD_PREFS_HISTORY_CONVERSATION_END_MODE          0x9EDA1202
#define USD_PREFS_HISTORY_CONVERSATION_END_TIME          0x9EDA1203
#define USD_PREFS_HISTORY_LOAD_OLD_TIME                  0x9EDA1204
#define USD_PREFS_HISTORY_LOAD_OLD_NO                    0x9EDA1205
#define USD_PREFS_HISTORY_SAVE_SYSTEMMSGS                0x9EDA1206
#define USD_PREFS_LOGS_UNICODE_ONOFF_CHECK               0x9EDA1207

/* contacts list prefs */
#define USD_PREFS_CONTACTSLIST_SHOWDESC_CHECK            0x9EDA1300
#define USD_PREFS_CONTACTSLIST_SHOWAVATARS_CHECK         0x9EDA1301
#define USD_PREFS_CONTACTSLIST_SPACEENTRIES_SLIDER       0x9EDA1302
#define USD_PREFS_CONTACTSLIST_AVATARSIZE_SLIDER         0x9EDA1303
#define USD_PREFS_CONTACTSLIST_SPACENAMEDESC_SLIDER      0x9EDA1304
#define USD_PREFS_CONTACTSLIST_ACTIVEENTRY_COLOR         0x9EDA1305

/* ftp server prefs */
#define USD_PREFS_FTP_URL_STRING                         0x9EDA1400
#define USD_PREFS_FTP_PARSE_BUTTON                       0x9EDA1401
#define USD_PREFS_FTP_HOST_STRING                        0x9EDA1402
#define USD_PREFS_FTP_PORT_STRING                        0x9EDA1403
#define USD_PREFS_FTP_USER_STRING                        0x9EDA1404
#define USD_PREFS_FTP_PASSWORD_STRING                    0x9EDA1405
#define USD_PREFS_FTP_SERVERPATH_STRING                  0x9EDA1406
#define USD_PREFS_FTP_LOGINTYPE_CYCLE                    0x9EDA1407

/* emoticons */
#define USD_PREFS_EMOTICONS_ONOFF                        0x9EDA110A
#define USD_PREFS_EMOTICONS_PATH(x)                      (0x9EDA1500 | (x))
#define USD_PREFS_EMOTICONS_PATH_EXCLAMATION             0x9EDA15FE
#define USD_PREFS_EMOTICONS_PATH_QUESTION                0x9EDA15FF
#define USD_PREFS_EMOTICONS_LONG_ONOFF                   0x9EDA1600

/* function keys */
#define USD_PREFS_FKEYS_STRING(x)                        (0x9EDA1700 | (x))

/* tags */
#define PWA_PrefsChanged                                 0x9EDAE000

/* methods */
#define PWM_SavePrefs                                    0x9EDAF000
#define PWM_AddPrefsPage                                 0x9EDAF001
#define PWM_ParseFTPUrl                                  0x9EDAF002
#define PWM_ChangePage                                   0x9EDAF003
#define PWM_ExportEmoticonsPaths                         0x9EDAF004
#define PWM_ImportEmoticonsPaths                         0x9EDAF005
#define PWM_SetDefaultEmoticonsPaths                     0x9EDAF006

#endif /* __PREFSWINDOW_H__ */

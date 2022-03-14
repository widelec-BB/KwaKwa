/*
 * Copyright (c) 2012-2022 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef __MAINWINDOW_H__
#define __MAINWINDOW_H__

#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>

extern struct MUI_CustomClass *MainWindowClass;

struct MUI_CustomClass *CreateMainWindowClass(void);
void DeleteMainWindowClass(void);

/* objects */
#define USD_MAIN_WINDOW_WIN                  0x7EDA0000
#define USD_MAIN_WINDOW_SCREENBARIZE_BUTTON  0x7EDA0001
#define USD_MAIN_WINDOW_STATUS_BUTTON        0x7EDA0002
#define USD_MAIN_WINDOW_SEARCH_GROUP         0x7EDA0003
#define USD_MAIN_WINDOW_SEARCH_STRING        0x7EDA0004
#define USD_MAIN_WINDOW_SEARCH_NEXT_BUTTON   0x7EDA0005
#define USD_MAIN_WINDOW_BUSY_BAR             0x7EDA0006

/* attrs */


/* methods */
#define MWM_Notifications                    0x7EDA2000
#define MWM_ShowGGStatusMenu                 0x7EDA2001
#define MWM_ChangeStatus                     0x7EDA2002
#define MWM_CloseRequest                     0x7EDA2003
#define MWM_InstallHideTimer                 0x7EDA2004
#define MWM_HideMethod                       0x7EDA2005
#define MWM_RemoveHideTimer                  0x7EDA2006


#endif /* __MAINWINDOW_H__ */

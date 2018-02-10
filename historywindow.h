/*
 * Copyright (c) 2012-2018 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef __HISTORYWINDOW_H__
#define __HISTORYWINDOW_H__

#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>

extern struct MUI_CustomClass *HistoryWindowClass;

struct MUI_CustomClass *CreateHistoryWindowClass(VOID);
VOID DeleteHistoryWindowClass(VOID);

/* 0x7EDF XXXX HistoryWindow */
#define USD_HISTORY_WINDOW    0x8EDF0000
#define USD_HISTORY_VTEXT     0xBEDA1002

/* attrs */

/* methods */
#define HWM_ContactSelected         0x7EDFA000
#define HWM_ConversationSelected    0x7EDFA001
#define HWM_InsertMessage           0x7EDFA002

#endif /* __HISTORYWINDOW_H__ */

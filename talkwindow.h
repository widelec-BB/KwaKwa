/*
 * Copyright (c) 2012-2018 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef __TALKWINDOWCLASS_H__
#define __TALKWINDOWCLASS_H__

#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>

extern struct MUI_CustomClass *TalkWindowClass;

struct MUI_CustomClass *CreateTalkWindowClass(void);
void DeleteTalkWindowClass(void);

/* objects */
#define USD_TALKWINDOW_WINDOW          0xAEDA0000


/* methods */
#define TKWM_ShowMessage               0xAEDA1000
#define TKWM_DeleteTab                 0xAEDA1001
#define TKWM_ActivateTab               0xAEDA1002
#define TKWM_AcceptBeacon              0xAEDA1003
#define TKWM_OpenOnTab                 0xAEDA1004
#define TKWM_CreateTabsMenuStrip       0xAEDA1005
#define TKWM_UpdateTabContact          0xAEDA1006
#define TKWM_UpdateTabContactStatus    0xAEDA1007
#define TKWM_UpdateTabWriteLamp        0xAEDA1008
#define TKWM_OpenOnTabById             0xAEDA1009
#define TKWM_ShowPicture               0xAEDA100A
#define TKWM_GetTab                    0xAEDA100B
#define TKWM_SendMessage               0xAEDA100C
#define TKWM_ShowInvite                0xAEDA100D
#define TKWM_DeleteTabByObject         0xAEDA100E


#define TKWV_ActiveTab                 -1

#endif /* __TALKWINDOWCLASS_H__ */

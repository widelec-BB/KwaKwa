/*
 * Copyright (c) 2012-2018 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef __SMALLSBAR_H__
#define __SMALLSBAR_H__

#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>

extern struct MUI_CustomClass *SmallSBarClass;

struct MUI_CustomClass *CreateSmallSBarClass(void);
void DeleteSmallSBarClass(void);

struct SBarControl
{
	struct SignalSemaphore semaphore;
	Object *app;
	struct Task *sbarTask;
	ULONG sbarSignal;
	struct Picture *actPic;
	BOOL unread;
	struct Picture *unreadPic;
};

/* methods */
#define SBRM_ShowMenu               0xCEDD0000

/* attrs */
#define SBRA_RightClick             0xCEDD1000

/* menu results */
#define SBR_MENU_SHOW_LIST             0x000001
#define SBR_MENU_STATUS_AVAILABLE      0x000002
#define SBR_MENU_STATUS_AWAY           0x000003
#define SBR_MENU_STATUS_INVISIBLE      0x000004
#define SBR_MENU_STATUS_UNAVAILABLE    0x000005
#define SBR_MENU_STATUS_FFC            0x000006
#define SBR_MENU_STATUS_DND            0x000007
#define SBR_MENU_STATUS_DESCRIPTION    0x000008
#define SBR_MENU_QUIT                  0x000009
#define SBR_MENU_TALK_TAB              0x0000FF

#endif /* __SMALLSBAR_H__ */

/*
 * Copyright (c) 2012-2018 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef __TALKTAB_H__
#define __TALKTAB_H__

#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>

extern struct MUI_CustomClass *TalkTabClass;

struct MUI_CustomClass *CreateTalkTabClass(void);
void DeleteTalkTabClass(void);


/* methods */
#define TTBM_Init                    0xBEDA0000
#define TTBM_PutMessage              0xBEDA0001
#define TTBM_SendMessage             0xBEDA0002
#define TTBM_Activate                0xBEDA0003
#define TTBM_NewStatus               0xBEDA0004
#define TTBM_ShowEnd                 0xBEDA0005
#define TTBM_SendWriteNotify         0xBEDA0006
#define TTBM_CleanLampState          0xBEDA0007
#define TTBM_SetLampState            0xBEDA0008
#define TTBM_PubdirParseResponse     0xBEDA0009
#define TTBM_RedrawInfoBlock         0xBEDA000A
#define TTBM_SendPicture             0xBEDA000B
#define TTBM_PutPicture              0xBEDA000C
#define TTBM_OpenLogFile             0xBEDA000D
#define TTBM_PutInvite               0xBEDA000E
#define TTBM_InsertLastMessages      0xBEDA000F
#define TTBM_InsertOldMessage        0xBEDA0010
#define TTBM_InitConversation        0xBEDA0011
#define TTBM_AddToHistory            0xBEDA0012
#define TTBM_EditContact             0xBEDA0013
#define TTBM_ToggleDouble            0xBEDA0014


/* gadgets */
#define USD_TALKTAB_RETURN_CHECK     0xBEDA1001
#define USD_TALKTAB_VIRTUALTEXT      0xBEDA1002
#define USD_TALKTAB_SEND_PICTURE     0xBEDA1003
#define USD_TALKTAB_OPEN_LOG         0xBEDA1004
#define USD_TALKTAB_INPUT_FIRST      0xBEDA1005
#define USD_TALKTAB_INPUT_SECOND     0xBEDA1006

/* attrs */
#define TTBA_ContactEntry            0xBEDA2000

#endif /* __TALKTAB_H__ */

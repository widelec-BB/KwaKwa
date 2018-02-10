/*
 * Copyright (c) 2012-2018 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef __VIRTUALTEXT_H__
#define __VIRTUALTEXT_H__

#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>

extern struct MUI_CustomClass *VirtualTextClass;

struct MUI_CustomClass *CreateVirtualTextClass(void);
void DeleteVirtualTextClass(void);

#define VTV_NormalMessage        0x00000001
#define VTV_SystemMessage        0x00000002

#define VTV_Incoming             (1 << 0)
#define VTV_Outgoing             (1 << 1)
#define VTV_FromHistory          (1 << 2)

/* methods */
#define VTM_AddMessageText       0xCEDA0000
#define VTM_ShowEnd              0xCEDA0001
#define VTM_Clear                0xCEDA0002
#define VTM_FormatTime           0xCEDA0003
#define VTM_ApplyPrefs           0xCEDA0004
#define VTM_AddSystemMessage     0xCEDA0005
#define VTM_SelectStart          0xCEDA0006
#define VTM_SelectStop           0xCEDA0007
#define VTM_CopyText             0xCEDA0008
#define VTM_AddMessageHeadLine   0xCEDA0009
#define VTM_AddPicture           0xCEDA000A
#define VTM_AddFileView          0xCEDA000B
#define VTM_AddMessage           0xCEDA000C
#define VTM_Relayout             0xCEDA000D
/* TODO: #define VTM_Find 0xCEDA000E */
#define VTM_InitChange           0xCEDA000F
#define VTM_ExitChange           0xCEDA0010

/* attrs */
#define VTA_Width                0xCEDA1000
#define VTA_Height               0xCEDA1001
#define VTA_ScrollOnResize       0xCEDA1002

#endif /* __VIRTUALTEXT_H__ */

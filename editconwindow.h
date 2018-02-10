/*
 * Copyright (c) 2012-2018 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef __EDITCONWINDOW_H__
#define __EDITCONWINDOW_H__

#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>

extern struct MUI_CustomClass *EditContactWindowClass;

struct MUI_CustomClass *CreateEditContactWindowClass(void);
void DeleteEditContactWindowClass(void);

/* objects */
#define USD_EDIT_CONTACT_WINDOW_WIN          0xFEDA0000
#define USD_EDIT_CONTACT_WINDOW_BUTTON       0xFEDA0001

/* methods */
#define ECWM_EditContact                     0xFEDA1000
#define ECWM_SaveContact                     0xFEDA1001
#define ECWM_PubdirFindByUin                 0xFEDA1002
#define ECWM_PubdirParseResponse             0xFEDA1003
#define ECWM_AddModulesCycle                 0xFEDA1004

/* attrs */
#define ECWA_ModulesList                     0xFEDA2000
#define ECWA_ModulesNo                       0xFEDA2001

#endif

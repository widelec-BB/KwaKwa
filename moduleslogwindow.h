/*
 * Copyright (c) 2012-2018 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef __MODULESLOGWINDOW_H__
#define __MODULESLOGWINDOW_H__

#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>

extern struct MUI_CustomClass *ModulesLogWindowClass;

struct MUI_CustomClass *CreateModulesLogWindowClass(VOID);
VOID DeleteModulesLogWindowClass(VOID);

/* 0x6EDB XXXX ModulesLogWindow */
#define USD_MODULESLOG_WINDOW   0x6EDB0000
#define USD_MODULESLOG_LIST     0x6EDB0001

/* methods */
#define MLW_AddMsg              0x6EDB1000

#endif /* __MAINWINDOW_H__ */

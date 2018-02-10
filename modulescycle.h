/*
 * Copyright (c) 2012-2018 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef __MODULESCYCLE_H__
#define __MODULESCYCLE_H__

#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>

extern struct MUI_CustomClass *ModulesCycleClass;

struct MUI_CustomClass *CreateModulesCycleClass(VOID);
VOID DeleteModulesCycleClass(VOID);

/* attrs */
#define MCA_ModulesList       0xEEDB0000
#define MCA_ModulesNo         0xEEDB0001
#define MCA_ActiveID          0xEEDB0002

/* methods */
#define MCM_GetIDByName       0xEEDB1000
#define MCM_GetNameByID       0xEEDB1001
#define MCM_SetActiveByName   0xEEDB1002
#define MCM_SetActiveByID     0xEEDB1003

#endif /* __MODULESCYCLE_H__ */

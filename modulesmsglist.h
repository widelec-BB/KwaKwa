/*
 * Copyright (c) 2012-2022 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef __MODULESMSGLIST_H__
#define __MODULESMSGLIST_H__

#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>

extern struct MUI_CustomClass *ModulesMsgListClass;

struct MUI_CustomClass *CreateModulesMsgListClass(VOID);
VOID DeleteModulesMsgListClass(VOID);

struct ModulesMsgListEntry
{
	STRPTR module_name;
	ULONG errno;
	STRPTR custom_msg;
};

#endif /* __MODULESMSGLIST_H__ */

/*
 * Copyright (c) 2012-2018 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef __SIMPLESTRING_H__
#define __SIMPLESTRING_H__

#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>

extern struct MUI_CustomClass *SimpleStringListClass;

struct MUI_CustomClass *CreateSimpleStringListClass(VOID);
VOID DeleteSimpleStringListClass(VOID);

/* methods */
#define MUIM_SimpleStringList_Find 0x7EDC0000


#endif /* __SIMPLESTRING_H__ */

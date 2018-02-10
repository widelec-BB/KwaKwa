/*
 * Copyright (c) 2012-2018 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef __TITLE_H__
#define __TITLE_H__

#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>

extern struct MUI_CustomClass *TitleClass;

struct MUI_CustomClass *CreateTitleClass(void);
void DeleteTitleClass(void);

/* attrs */
#define TITA_Deleted       0xEEDA0000

/* methods */


#endif /* __TITLE_H__ */

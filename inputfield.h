/*
 * Copyright (c) 2012-2018 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef __INPUTFIELD_H__
#define __INPUTFIELD_H__

#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>
#include <mui/TextEditor_mcc.h>

extern struct MUI_CustomClass *InputFieldClass;

struct MUI_CustomClass *CreateInputFieldClass(void);
void DeleteInputFieldClass(void);

/* tags */
#define IFA_SendAfterReturn    0xCEDB0000
#define IFA_Acknowledge        0xCEDB0001
#define IFA_TalkTab            0xCEDB0002

/* methods */
#define IFM_ContextMenu        0xCEDB1000
#define IFM_ExternalEdit       0xCEDB1001
#define IFM_ImportTxtFile      0xCEDB1002
#define IFM_LoadTxtFile        0xCEDB1003


#endif /* __INPUTFIELD_H__ */

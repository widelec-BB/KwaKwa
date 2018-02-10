/*
 * Copyright (c) 2012-2018 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef __EMOTICON_H__
#define __EMOTICON_H__

#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>

extern struct MUI_CustomClass *EmoticonClass;

struct MUI_CustomClass *CreateEmoticonClass(VOID);
VOID DeleteEmoticonClass(VOID);

#define MCC_EMO_TAGBASE       (0xCEDF0000)
#define MCC_EMO_ID(x)         (MCC_EMO_TAGBASE + (x))

/* attrs */
#define EMOA_PicturePath      MCC_EMO_ID(0x0000)      /* [i.g] CONST STRPTR */
#define EMOA_GlobalAlpha      MCC_EMO_ID(0x0001)      /* [isg] ULONG  */
#define EMOA_TextPreparse     MCC_EMO_ID(0x0002)      /* [i..] CONST STRPTR */ /* NOTE: not copied */

/* methods */
#define EMOM_DrawNextFrame    MCC_EMO_ID(0x1000)
#define EMOM_DoubleClick      MCC_EMO_ID(0x1001)
#define EMOM_ReloadPicture    MCC_EMO_ID(0x1002)


#endif /* __EMOTICON_H__ */

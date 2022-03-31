#ifndef __LONGPROCESSWINDOW_H__
#define __LONGPROCESSWINDOW_H__

#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>

extern struct MUI_CustomClass *LongProcessWindowClass;

struct MUI_CustomClass *CreateLongProcessWindowClass(void);
void DeleteLongProcessWindowClass(void);

/* 0xFEDB XXXX TabTitle */
#define MCC_LPW_TAGBASE            (0xFEDB0000)
#define MCC_LPW_ID(x)              (MCC_LPW_TAGBASE + (x))

/* methods */
#define LPWM_Stop                         MCC_LPW_ID(0x1000)
#define LPWM_ConfirmStop                  MCC_LPW_ID(0x1001)
#define LPWM_UpdateHistoryDatabase        MCC_LPW_ID(0x1002)

#endif /* __LONGPROCESSWINDOW_H__ */

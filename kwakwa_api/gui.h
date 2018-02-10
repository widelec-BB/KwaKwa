/*
 * Copyright (c) 2012-2018 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef __KWAKWA_GUI_H__
#define __KWAKWA_GUI_H__

#define KWAKWA_GLOBAL_GUI      0x0EAC
#define KWAKWA_GUI_ID(x)       (TAG_USER | (KWAKWA_GLOBAL_GUI << 16) | (x))

#define KWAG_PrefsEntry        KWAKWA_GUI_ID(0x0000)    /* STRPTR  */
#define KWAG_PrefsPage         KWAKWA_GUI_ID(0x0001)    /* Object* */
#define KWAG_Window            KWAKWA_GUI_ID(0x0002)    /* Object* */
#define KWAG_ToolsEntry        KWAKWA_GUI_ID(0x0003)    /* Object* */


#endif /* __KWAKWA_GUI_H__ */

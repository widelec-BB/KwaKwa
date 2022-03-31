/*
 * Copyright (c) 2012-2022 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef __APPLICATION_H__
#define __APPLICATION_H__

#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>
#include <exec/lists.h>

extern struct MUI_CustomClass *ApplicationClass;

struct MUI_CustomClass *CreateApplicationClass(void);
void DeleteApplicationClass(void);

struct InsertLinkParms
{
	ULONG pluginid;
	STRPTR contactid;
	STRPTR link;
};

/* methods */
#define APPM_MainLoop                        0x6EDA0000
#define APPM_DisconnectAck                   0x6EDA0001
#define APPM_Connect                         0x6EDA0002
#define APPM_Disconnect                      0x6EDA0003
#define APPM_ChangeStatus                    0x6EDA0004
#define APPM_ErrorNotSupported               0x6EDA0005
#define APPM_NotifyContactList               0x6EDA0006
#define APPM_ListChangeAck                   0x6EDA0007
#define APPM_NewMessageAck                   0x6EDA0009
#define APPM_StatusChangeAck                 0x6EDA0008
#define APPM_SendMessage                     0x6EDA000A
#define APPM_ScreenbarChange                 0x6EDA000B
#define APPM_AddNotify                       0x6EDA000C
#define APPM_RemoveNotify                    0x6EDA000D
#define APPM_ScreenbarMenu                   0x6EDA000E
#define APPM_SendTypingNotify                0x6EDA000F
#define APPM_TypingNotifyAck                 0x6EDA0010
#define APPM_Screenbarize                    0x6EDA0011
#define APPM_ModuleMessageAck                0x6EDA0012
#define APPM_ErrorNoMem                      0x6EDA0013
#define APPM_ErrorConnFail                   0x6EDA0014
#define APPM_ErrorLoginFail                  0x6EDA0015
#define APPM_ScreenbarUnread                 0x6EDA0016
#define APPM_OpenModules                     0x6EDA0017
#define APPM_CloseModules                    0x6EDA0018
#define APPM_NewEvent                        0x6EDA0019
#define APPM_ReplyMsg                        0x6EDA001A
#define APPM_ParseEvent                      0x6EDA001B
#define APPM_ConnectAck                      0x6EDA001C
#define APPM_FtpPut                          0x6EDA001D
#define APPM_NotifyBeacon                    0x6EDA001E
#define APPM_SendHttpGet                     0x6EDA001F
#define APPM_SendHttpPost                    0x6EDA0020
#define APPM_NewAvatarAck                    0x6EDA0021
#define APPM_ClipboardStart                  0x6EDA0022
#define APPM_ClipboardWrite                  0x6EDA0023
#define APPM_ClipboardEnd                    0x6EDA0024
#define APPM_Setup                           0x6EDA0025
#define APPM_Cleanup                         0x6EDA0026
#define APPM_InstallBroker                   0x6EDA0027
#define APPM_RemoveBroker                    0x6EDA0028
#define APPM_AutoAway                        0x6EDA0029
#define APPM_AutoBack                        0x6EDA002A
#define APPM_ImportList                      0x6EDA002B
#define APPM_ExportList                      0x6EDA002C
#define APPM_ImportListAck                   0x6EDA002D
#define APPM_ExportListAck                   0x6EDA002E
#define APPM_AddModulesGui                   0x6EDA002F
#define APPM_SendPicture                     0x6EDA0030
#define APPM_NewPictureAck                   0x6EDA0031
#define APPM_FtpPutCallback                  0x6EDA0032
#define APPM_FtpPutActiveTab                 0x6EDA0033
#define APPM_AnswerInvite                    0x6EDA0034
#define APPM_NewInviteAck                    0x6EDA0035
#define APPM_PubDirRequest                   0x6EDA0036
#define APPM_GetModuleName                   0x6EDA0037
#define APPM_ConvertContactEntry             0x6EDA0038
#define APPM_OpenHistoryDatabase             0x6EDA0039
#define APPM_CloseHistoryDatabase            0x6EDA003A
#define APPM_DoSqlOnHistoryDatabase          0x6EDA003B
#define APPM_InsertMessageIntoHistory        0x6EDA003C
#define APPM_InsertConversationIntoHistory   0x6EDA003D
#define APPM_GetModuleUserId                 0x6EDA003E
#define APPM_GetLastMessagesFromHistory      0x6EDA004F
#define APPM_GetLastConvMsgsFromHistory      0x6EDA0050
#define APPM_GetLastMessagesByTime           0x6EDA0051
#define APPM_GetLastConversation             0x6EDA0052
#define APPM_GetContactsFromHistory          0x6EDA0053
#define APPM_GetConversationsFromHistory     0x6EDA0054
#define APPM_GetMessagesFromHistory          0x6EDA0055
#define APPM_DeleteContactFromHistory        0x6EDA0056
#define APPM_DeleteConversationFromHistory   0x6EDA0057
#define APPM_SetLastStatus                   0x6EDA0058
#define APPM_ConfirmQuit                     0x6EDA0059
#define APPM_ScreenbarInstall                0x6EDA005A
#define APPM_ScreenbarRemove                 0x6EDA005B
#define APPM_UpdateHistoryDatabase           0x6EDA005C

#define APPM_SecTrigger                      0x6EDA111D

/* attrs */
#define APPA_ScreenbarUnread                 0x6EDA1000
#define APPA_Status                          0x6EDA1001
#define APPA_Description                     0x6EDA1002

/* magic beacon notifications names */
#define BEACON_MESSAGE "MESSAGE"
#define BEACON_STATUS "STATUS"
#define BEACON_PICTURE "PICTURE"
#define BEACON_INVITE "INVITE"

/* history database flags - conversations table */
#define HISTORY_CONVERSATIONS_NORMAL      (0)
#define HISTORY_CONVERSATIONS_CONFERENCE  (1)

/* history database flags - messages table */
#define HISTORY_MESSAGES_MY             (0)
#define HISTORY_MESSAGES_FRIEND         (1 << 0)
#define HISTORY_MESSAGES_NORMAL         (1 << 1)
#define HISTORY_MESSAGES_SYSTEM         (1 << 2)
#define HISTORY_MESSAGES_PICTURE        (1 << 3) /* TODO: not yet implemented */
#define HISTORY_MESSAGES_INVITE         (1 << 4) /* TODO: not yet implemented */


#endif /* __APPLICATION_H__ */

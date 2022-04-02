/*
 * Copyright (c) 2012-2022 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef __LOGS_H__
#define __LOGS_H__

#define LOGS_DIR "logs"
#define LOGS_UNICODE_DIR "logs-utf8"
#define LOGS_DRAWER "PROGDIR:"LOGS_DIR
#define LOGS_UNICODE_DRAWER "PROGDIR:"LOGS_UNICODE_DIR
#define LOGS_DRAWER_LEN 12
#define LOGS_UNICODE_DRAWER_LEN 17

BOOL SetupLogsSystem(VOID);
VOID CleanupLogsSystem(VOID);
BPTR LogOpen(STRPTR log_name_utf8, BOOL unicode);
VOID LogAddSys(BPTR fh, STRPTR author, STRPTR message, ULONG timestamp);
VOID LogAddUnicode(BPTR fh, STRPTR author, STRPTR message, ULONG timestamp);
VOID LogClose(BPTR fh, BOOL unicode);
VOID LogsRename(STRPTR old_name_utf8, STRPTR new_name_utf8);

#endif /* __LOGS_H__ */

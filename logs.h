/*
 * Copyright (c) 2012-2022 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef __LOGS_H__
#define __LOGS_H__

#define LOGS_DIR "logs"
#define LOGS_DRAWER "PROGDIR:"LOGS_DIR
#define LOGS_DRAWER_LEN 12

BPTR LogOpen(STRPTR log_name);
VOID LogAdd(BPTR fh, STRPTR author, STRPTR message, ULONG timestamp);
VOID LogClose(BPTR fh);
LONG LogRename(STRPTR old_name, STRPTR new_name);

#endif /* __LOGS_H__ */

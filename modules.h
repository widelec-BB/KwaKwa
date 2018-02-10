/*
 * Copyright (c) 2012-2018 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef __MODULES_H__
#define __MODULES_H__

#include <proto/exec.h>
#include <exec/types.h>
#include <exec/nodes.h>
#include <intuition/classusr.h>

struct Module
{
	struct MinNode mod_Node;
	struct Library *mod_Libbase;
	STRPTR mod_ClassName;
	Object *mod_Object;
	STRPTR mod_Name;
	ULONG mod_ID;
	BOOL mod_Connected;
};

struct Module *OpenModule(STRPTR mod_path, STRPTR file_name, Object *app);
VOID CloseModule(struct Module *m);

#define MODULE_IS_CONNECTED(m) m->mod_Connected


#endif /* __MODULES_H__ */

/*
 * Copyright (c) 2012-2022 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/types.h>
#include <libvstring.h>

#include "globaldefines.h"
#include "modules.h"
#include "support.h"
#include "modulescycle.h"

#include "kwakwa_api/defs.h"

struct MUI_CustomClass *ModulesCycleClass;

static IPTR ModulesCycleDispatcher(VOID);
const struct EmulLibEntry ModulesCycleGate = {TRAP_LIB, 0, (VOID(*)(VOID))ModulesCycleDispatcher};

struct MCP_GetIDByName {ULONG MethodID; STRPTR name;};
struct MCP_GetNameByID {ULONG MethodID; ULONG id;};
struct MCP_SetActiveByName {ULONG MethodID; STRPTR name;};
struct MCP_SetActiveByID {ULONG MethodID; ULONG id;};

struct ModulesCycleData
{
	STRPTR *modules_names;
	ULONG *modules_ids;
	ULONG modules_no;
};

struct MUI_CustomClass *CreateModulesCycleClass(VOID)
{
	struct MUI_CustomClass *cl;

	cl = MUI_CreateCustomClass(NULL, MUIC_Cycle, NULL, sizeof(struct ModulesCycleData), (APTR)&ModulesCycleGate);
	ModulesCycleClass = cl;
	return cl;
}

VOID DeleteModulesCycleClass(VOID)
{
	if (ModulesCycleClass) MUI_DeleteCustomClass(ModulesCycleClass);
}

static IPTR ModulesCycleNew(Class *cl, Object *obj, struct opSet *msg)
{
	ULONG modules_no;
	struct MinList *modules_list;

	modules_list = (struct MinList*)GetTagData(MCA_ModulesList, (ULONG)NULL, msg->ops_AttrList);
	modules_no = GetTagData(MCA_ModulesNo, 0, msg->ops_AttrList);

	if(modules_list && modules_no)
	{
		STRPTR *modules_names;
		ULONG *modules_ids;

		if((modules_names = AllocVec((modules_no + 1) * sizeof(STRPTR), MEMF_ANY)))
		{
			if((modules_ids = AllocVec(modules_no * sizeof(ULONG), MEMF_ANY)))
			{
				struct Module *m;
				ULONG i = 0;

				ForeachNode(modules_list, m)
				{
					modules_names[i] = m->mod_Name;
					modules_ids[i++] = m->mod_ID;
				}

				modules_names[i] = NULL;

				obj = (Object*)DoSuperNew(cl, (APTR)obj,
					MUIA_Cycle_Entries, (ULONG)modules_names,
				TAG_MORE, (ULONG)msg->ops_AttrList);

				if(obj)
				{
					struct ModulesCycleData *d = INST_DATA(cl, obj);

					d->modules_names = modules_names;
					d->modules_ids = modules_ids;
					d->modules_no = modules_no;

					return (IPTR)obj;
				}
				FreeVec(modules_ids);
			}
			FreeVec(modules_names);
		}
	}
	CoerceMethod(cl, obj, OM_DISPOSE);
	return (IPTR)NULL;
}

static IPTR ModulesCycleDispose(Class *cl, Object *obj, Msg msg)
{
	struct ModulesCycleData *d = INST_DATA(cl, obj);

	if(d->modules_ids)
		FreeVec(d->modules_ids);

	if(d->modules_names)
		FreeVec(d->modules_names);

	return DoSuperMethodA(cl, obj, msg);
}

static IPTR ModulesCycleGet(Class *cl, Object *obj, struct opGet *msg)
{
	struct ModulesCycleData *d = INST_DATA(cl, obj);
	int rv = FALSE;

	switch(msg->opg_AttrID)
	{
		case MCA_ActiveID:
			*msg->opg_Storage = d->modules_ids[xget(obj, MUIA_Cycle_Active)];
			return TRUE;
		break;

		default:
			rv = (DoSuperMethodA(cl, obj, (Msg)msg));
	}

	return rv;
}

static IPTR ModulesCycleGetIDByName(Class *cl, Object *obj, struct MCP_GetIDByName *msg)
{
	struct ModulesCycleData *d = INST_DATA(cl, obj);
	ULONG i;

	for(i = 0; i < d->modules_no; i++)
	{
		if(StrEqu(msg->name, d->modules_names[i]))
			return (IPTR)d->modules_ids[i];
	}

	return (IPTR)0;
}

static IPTR ModulesCycleGetNameByID(Class *cl, Object *obj, struct MCP_GetNameByID *msg)
{
	struct ModulesCycleData *d = INST_DATA(cl, obj);
	ULONG i;

	for(i = 0; i < d->modules_no; i++)
	{
		if(d->modules_ids[i] == msg->id)
			return (IPTR)d->modules_names[i];
	}

	return (IPTR)0;
}

static IPTR ModulesCycleSetActiveByName(Class *cl, Object *obj, struct MCP_SetActiveByName *msg)
{
	struct ModulesCycleData *d = INST_DATA(cl, obj);
	ULONG i;

	for(i = 0; i < d->modules_no; i++)
	{
		if(StrEqu(msg->name, d->modules_names[i]))
		{
			set(obj, MUIA_Cycle_Active, i);
			return (IPTR)TRUE;
		}
	}

	return (IPTR)FALSE;
}

static IPTR ModulesCycleSetActiveByID(Class *cl, Object *obj, struct MCP_SetActiveByID *msg)
{
	struct ModulesCycleData *d = INST_DATA(cl, obj);
	ULONG i;

	for(i = 0; i < d->modules_no; i++)
	{
		if(d->modules_ids[i] == msg->id)
		{
			set(obj, MUIA_Cycle_Active, i);
			return (IPTR)TRUE;
		}
	}

	return (IPTR)FALSE;
}

static IPTR ModulesCycleDispatcher(VOID)
{
	Class *cl = (Class*)REG_A0;
	Object *obj = (Object*)REG_A2;
	Msg msg = (Msg)REG_A1;

	switch (msg->MethodID)
	{
		case OM_NEW: return (ModulesCycleNew(cl, obj, (struct opSet*) msg));
		case OM_DISPOSE: return (ModulesCycleDispose(cl, obj, msg));
		case OM_GET: return(ModulesCycleGet(cl, obj, (struct opGet*)msg));
		case MCM_GetIDByName: return(ModulesCycleGetIDByName(cl, obj, (struct MCP_GetIDByName*)msg));
		case MCM_GetNameByID: return(ModulesCycleGetNameByID(cl, obj, (struct MCP_GetNameByID*)msg));
		case MCM_SetActiveByName: return(ModulesCycleSetActiveByName(cl, obj, (struct MCP_SetActiveByName*)msg));
		case MCM_SetActiveByID: return(ModulesCycleSetActiveByID(cl, obj, (struct MCP_SetActiveByID*)msg));
		default:  return (DoSuperMethodA(cl, obj, msg));
	}
}

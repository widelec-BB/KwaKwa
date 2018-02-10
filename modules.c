/*
 * Copyright (c) 2012-2018 Filip "widelec-BB" Maryjanski, BlaBla group.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <intuition/classes.h>
#include <ppcinline/macros.h>
#include <proto/intuition.h>
#include <libvstring.h>
#include "kwakwa_api/protocol.h"
#include "globaldefines.h"
#include "support.h"
#include "modules.h"

#define GetClass(libbasename) LP0(30, Class *, GetClass, ,libbasename, 0, 0, 0, 0, 0, 0)

struct Module *OpenModule(STRPTR mod_path, STRPTR file_name, Object *app)
{
	struct Library *libbase;
	ENTER();

	if((libbase = OpenLibrary(mod_path, 0)))
	{
		struct Module *m;

		if((m = AllocMem(sizeof(struct Module), MEMF_ANY)))
		{
			m->mod_Libbase = libbase;
			m->mod_Connected = FALSE;
			m->mod_ClassName = StrNew(file_name);

			if((m->mod_Object = NewObject(GetClass(m->mod_Libbase), file_name,
				KWAA_AppObject, app,
			TAG_END)))
			{
				if(GetAttr(KWAA_ModuleID, m->mod_Object, &m->mod_ID))
				{
					if(GetAttr(KWAA_ProtocolName, m->mod_Object, (ULONG*)&m->mod_Name) && m->mod_Name)
					{
						LEAVE();
						return m;
					}
				}
				DisposeObject(m->mod_Object);
			}
			FreeMem(m, sizeof(struct Module));
		}
		CloseLibrary(libbase);
	}
	LEAVE();
	return NULL;
}

VOID CloseModule(struct Module *m)
{
	if(m)
	{
		if(m->mod_Object)
			DisposeObject(m->mod_Object);
		if(m->mod_Libbase)
			CloseLibrary(m->mod_Libbase);
		if(m->mod_ClassName)
			StrFree(m->mod_ClassName);

		FreeMem(m, sizeof(struct Module));
	}
}

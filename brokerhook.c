#include <libraries/mui.h>
#include <proto/alib.h>
#include <proto/commodities.h>
#include "globaldefines.h"
#include "mainwindow.h"
#include "support.h"
#include "brokerhook.h"

STATIC VOID BrokerFunc(void)
{
	APTR app = (APTR)REG_A2;
	CxMsg *msg = (APTR)REG_A1;
	ULONG type = CxMsgType(msg);
	ULONG id = CxMsgID(msg);

	tprintf("BROKER msg type == %ld id == %ld\n", type, id);
	if(type == CXM_COMMAND && id == CXCMD_APPEAR)
	{
		tprintf("CMD MESSAGE!!!!\n");
		Object *main_window = findobj(USD_MAIN_WINDOW_WIN, app);
		set(main_window, MUIA_Window_Open, TRUE);
	}
}

STATIC const struct EmulLibEntry BrokerHookTrap = { TRAP_LIBNR, 0, (APTR)&BrokerFunc };
const struct Hook BrokerHook = { { NULL, NULL }, (HOOKFUNC)&BrokerHookTrap, NULL, NULL };
#include "rpc.h"
#include "cfg.h"
#include "modbus_client.h"
#include "update.h"
#include "system.h"
#include <libmodbus/modbus.h>

#include <iostream>

static HostPointMap g_HostPointMaps[MAX_MODBUS];
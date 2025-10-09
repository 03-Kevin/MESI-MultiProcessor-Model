#ifndef BUS_H
#define BUS_H

#include "memory/memory.h"

typedef enum { BUS_RD, BUS_RDX, BUS_INV, BUS_WB } BusMsg;

void bus_init();
void bus_broadcast(BusMsg msg, int addr, int src_pe);

#endif

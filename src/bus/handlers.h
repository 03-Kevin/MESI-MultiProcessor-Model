// handlers/handlers.h
#ifndef HANDLERS_H
#define HANDLERS_H

#include "../bus/bus.h"

void bus_register_handlers(Bus* bus);

void handle_busrd(Bus* bus, int addr, int src_pe);
void handle_busrdx(Bus* bus, int addr, int src_pe);
void handle_busupgr(Bus* bus, int addr, int src_pe);
void handle_buswb(Bus* bus, int addr, int src_pe);

#endif // HANDLERS_H

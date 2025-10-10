#ifndef HANDLERS_H
#define HANDLERS_H

#include "bus.h"

// Registra los handlers en el bus
void bus_register_handlers(Bus* bus);

// Funciones individuales (no públicas fuera del bus)
void handle_busrd(Bus* bus, int addr, int src_pe);
void handle_busrdx(Bus* bus, int addr, int src_pe);
void handle_busupgr(Bus* bus, int addr, int src_pe);
void handle_buswb(Bus* bus, int addr, int src_pe);

#endif
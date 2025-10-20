#include "handlers.h"
#include "../memory/memory.h"
#include "../cache/cache.h"
#include <stdio.h>

// ================
// REGISTRO DE HANDLERS
// ================
void bus_register_handlers(Bus* bus) {
    bus->handlers[BUS_RD]   = handle_busrd;
    bus->handlers[BUS_RDX]  = handle_busrdx;
    bus->handlers[BUS_UPGR] = handle_busupgr;
    bus->handlers[BUS_WB]   = handle_buswb;
}

// ================
// HANDLER: BusRd
// ================
void handle_busrd(Bus* bus, int addr, int src_pe) {
    printf("[BUS] Ejecutando handler BusRd (PE%d, addr=%d)\n", src_pe, addr);
    int found = 0;

    // Propagar estado Shared a otras cachés
    for (int i = 0; i < NUM_PES; i++) {
        if (i == src_pe) continue; // No modificar la caché del PE que envió la señal
        CacheLine* line = cache_get_line(bus->caches[i], addr);
        if (line) {
            found = 1;
            if (line->state == M) {
                // Escribir el valor en memoria principal si está en estado Modified
                mem_write(addr, line->data[0]);
                line->state = S; // Cambiar a Shared
                printf("[BUS] Transición de estado en PE%d: M -> S para addr=%d\n", i, addr);
            } else if (line->state == E) {
                // Cambiar a Shared si está en estado Exclusive
                line->state = S;
                printf("[BUS] Transición de estado en PE%d: E -> S para addr=%d\n", i, addr);
            }
        }
    }

    // Si ninguna caché tiene la línea, leer desde memoria principal
    if (!found) {
        printf("[BUS] Línea no encontrada → leyendo de memoria\n");
        mem_read(addr);
    }
}

// ================
// HANDLER: BusRdX
// ================
void handle_busrdx(Bus* bus, int addr, int src_pe) {
    printf("[BUS] Ejecutando handler BusRdX (PE%d, addr=%d)\n", src_pe, addr);

    for (int i = 0; i < NUM_PES; i++) {
        if (i != src_pe) {
            CacheLine* line = cache_get_line(bus->caches[i], addr);
            if (line && line->valid) {
                printf("[BUS] Invalidando línea en PE%d para addr=%d\n", i, addr);
                line->state = I; // Cambiar a Invalid
            }
        }
    }
}

// ================
// HANDLER: BusUpgr
// ================
void handle_busupgr(Bus* bus, int addr, int src_pe) {
    printf("[BUS] Ejecutando handler BusUpgr (PE%d, addr=%d)\n", src_pe, addr);

    // Invalidar líneas en otras cachés
    for (int i = 0; i < NUM_PES; i++) {
        if (i == src_pe) continue; // No modificar la caché del PE que envió la señal
        CacheLine* line = cache_get_line(bus->caches[i], addr);
        if (line) {
            line->state = I; // Cambiar a Invalid
            printf("[BUS] Transición de estado en PE%d: %s -> I para addr=%d\n",
                   i, mesi_state_to_str(line->state), addr);
        }
    }
}

// ================
// HANDLER: BusWB
// ================
void handle_buswb(Bus* bus, int addr, int src_pe) {
    printf("[BUS] Ejecutando handler BusWB (PE%d, addr=%d)\n", src_pe, addr);

    // Escribir el valor en memoria principal si está en estado Modified
    CacheLine* line = cache_get_line(bus->caches[src_pe], addr);
    if (line && line->state == M) {
        mem_write(addr, line->data[0]);
        line->state = S; // Cambiar a Shared
        printf("[BUS] Transición de estado en PE%d: M -> S para addr=%d\n", src_pe, addr);
    }
}
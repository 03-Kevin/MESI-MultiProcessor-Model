#ifndef STEP_CONTROL_H
#define STEP_CONTROL_H

#include <stdbool.h>

void step_control_init(bool enabled, int interval);
void step_control_shutdown(void);

/* Llamar desde el loop de ejecución de cada PE:
 *    instr_count = número de instrucciones ejecutadas por este PE (incremental)
 * Este método hará block hasta que el usuario avance el step (si el modo está activado
 * y la instrucción actual coincide con el intervalo).
 */
void step_control_maybe_pause(long instr_count);

#endif // STEP_CONTROL_H

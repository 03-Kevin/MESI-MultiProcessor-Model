#define _POSIX_C_SOURCE 200809L
#include "step_control.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

/* Estado del controlador */
static bool sc_enabled = false;
static int sc_interval = 1;

static pthread_t sc_ctrl_thread;
static pthread_mutex_t sc_mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t sc_cv = PTHREAD_COND_INITIALIZER;

/* generación de pasos; se incrementa cada vez que se avanza 1 step (o cuando se hace continue) */
static volatile int sc_step_gen = 0;

/* Thread que escucha stdin y genera pasos */
static void *sc_controller(void *arg) {
    (void)arg;
    char buf[64];
    printf("[STEP] Control: modo stepping activo. ENTER = next step, 'c'+ENTER = continue, 'q'+ENTER = quit stepping\n");
    while (1) {
        if (!fgets(buf, sizeof(buf), stdin)) break;
        // trim newline
        size_t len = strlen(buf);
        if (len && buf[len-1] == '\n') buf[len-1] = '\0';

        if (buf[0] == 'c' || buf[0] == 'C') {
            pthread_mutex_lock(&sc_mtx);
            sc_enabled = false;      /* desactiva stepping (run normal) */
            sc_step_gen++;           /* un impulso final para despertar esperas actuales */
            pthread_cond_broadcast(&sc_cv);
            pthread_mutex_unlock(&sc_mtx);
            printf("[STEP] Continue: stepping desactivado. Los PEs seguirán sin pausas.\n");
            break;
        } else if (buf[0] == 'q' || buf[0] == 'Q') {
            pthread_mutex_lock(&sc_mtx);
            sc_enabled = false;
            sc_step_gen++;
            pthread_cond_broadcast(&sc_cv);
            pthread_mutex_unlock(&sc_mtx);
            printf("[STEP] Quit stepping.\n");
            break;
        } else {
            /* single step: incrementar generación y despertar */
            pthread_mutex_lock(&sc_mtx);
            sc_step_gen++;
            pthread_cond_broadcast(&sc_cv);
            pthread_mutex_unlock(&sc_mtx);
            /* notificar al usuario */
            printf("[STEP] Avanzado un paso (gen=%d)\n", sc_step_gen);
        }
    }
    return NULL;
}

void step_control_init(bool enabled, int interval) {
    sc_interval = (interval > 0) ? interval : 1;
    sc_enabled = enabled;
    if (!sc_enabled) return;

    sc_step_gen = 0;
    if (pthread_create(&sc_ctrl_thread, NULL, sc_controller, NULL) != 0) {
        perror("step_control: pthread_create");
        sc_enabled = false;
    }
}

void step_control_shutdown(void) {
    if (!sc_enabled) return;
    pthread_mutex_lock(&sc_mtx);
    sc_enabled = false;
    sc_step_gen++;
    pthread_cond_broadcast(&sc_cv);
    pthread_mutex_unlock(&sc_mtx);
    pthread_join(sc_ctrl_thread, NULL);
}

/* thread-local índice para saber qué generación hemos consumido ya */
void step_control_maybe_pause(long instr_count) {
    if (!sc_enabled) return;
    if ( (instr_count % sc_interval) != 0 ) return;

    static __thread int my_gen = 0;
    pthread_mutex_lock(&sc_mtx);
    /* si my_gen == sc_step_gen => aún no se ha avanzado la generación que necesitamos,
       por tanto espera hasta que el controller incremente sc_step_gen. */
    while (sc_enabled && my_gen == sc_step_gen) {
        pthread_cond_wait(&sc_cv, &sc_mtx);
    }
    /* al despertarnos, igualamos nuestra marca a la generación actual (consumida) */
    my_gen = sc_step_gen;
    pthread_mutex_unlock(&sc_mtx);
}

#include "pe.h"

int main() {
    mem_init();
    bus_init();

    PE pes[4];
    pthread_t threads[4];

    for (int i = 0; i < 4; i++) {
        pes[i].id = i;
        cache_init(&pes[i].cache);
        pthread_create(&threads[i], NULL, pe_run, &pes[i]);
    }

    for (int i = 0; i < 4; i++)
        pthread_join(threads[i], NULL);

    return 0;
}

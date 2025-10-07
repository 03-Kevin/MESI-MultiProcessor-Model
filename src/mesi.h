#ifndef MESI_H
#define MESI_H

typedef enum { M, E, S, I } MESI_State;

const char* mesi_state_to_str(MESI_State state);

#endif

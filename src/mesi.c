#include "mesi.h"

const char* mesi_state_to_str(MESI_State state) {
    switch (state) {
        case M: return "Modified";
        case E: return "Exclusive";
        case S: return "Shared";
        case I: return "Invalid";
        default: return "Unknown";
    }
}

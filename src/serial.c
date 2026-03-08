#include "serial.h"
#include "pico/unique_id.h"
#include <stdio.h>

char serial_str[SERIAL_STR_SIZE] = {0};

void init_serial_string(void) {
    pico_unique_board_id_t id;
    pico_get_unique_board_id(&id);
    for (int i = 0; i < PICO_UNIQUE_BOARD_ID_SIZE_BYTES; ++i) {
        sprintf(serial_str + i*2, "%02X", id.id[i]);
    }
    serial_str[16] = '\0';
}
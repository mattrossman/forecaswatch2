#include "persist.h"

enum key {TEMP_LO, TEMP_HI, TEMP_TREND, PRECIP_TREND, TEMP_START, CITY, NUM_ENTRIES};

void persist_init() {
    if (!persist_exists(TEMP_LO)) {
        persist_write_int(TEMP_LO, 2);
    }
    if (!persist_exists(TEMP_HI)) {
        persist_write_int(TEMP_HI, 12);
    }
    if (!persist_exists(TEMP_TREND)) {
        int16_t data[] = {2, 2, 2, 4, 7, 9, 11, 12, 12, 12, 11, 9};
        persist_write_data(TEMP_TREND, (void*) data, 12*sizeof(int16_t));
    }
    if (!persist_exists(PRECIP_TREND)) {
        uint8_t data[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        persist_write_data(TEMP_TREND, (void*) data, 12*sizeof(uint8_t));
    }
    if (!persist_exists(TEMP_START)) {
        persist_write_int(TEMP_START, 6);
    }
    if (!persist_exists(NUM_ENTRIES)) {
        persist_write_int(NUM_ENTRIES, 12);
    }
    if (!persist_exists(CITY)) {
        persist_write_string(CITY, "Koji");
    }
}

int persist_get_temp_lo() {
    return persist_read_int(TEMP_LO);
}

int persist_get_temp_hi() {
    return persist_read_int(TEMP_HI);
}

int persist_get_temp_trend(int16_t *buffer, const size_t buffer_size) {
    return persist_read_data(TEMP_TREND, (void*) buffer, buffer_size * sizeof(int16_t));
}

int persist_get_precip_trend(uint8_t *buffer, const size_t buffer_size) {
    return persist_read_data(PRECIP_TREND, (void*) buffer, buffer_size * sizeof(uint8_t));
}

int persist_get_temp_start() {
    return persist_read_int(TEMP_START);
}

int persist_get_num_entries() {
    return persist_read_int(NUM_ENTRIES);
}

int persist_get_city(char *buffer, const size_t buffer_size) {
    return persist_read_string(CITY, buffer, buffer_size);
}

void persist_set_temp_lo(int val) {
    persist_write_int(TEMP_LO, val);
}

void persist_set_temp_hi(int val) {
    persist_write_int(TEMP_HI, val);
}

void persist_set_temp_trend(int16_t *data, const size_t size) {
    persist_write_data(TEMP_TREND, (void*) data, size * sizeof(int16_t));
}

void persist_set_precip_trend(uint8_t *data, const size_t size) {
    persist_write_data(PRECIP_TREND, (void*) data, size * sizeof(uint8_t));
}

void persist_set_temp_start(int val) {
    persist_write_int(TEMP_START, val);
}

void persist_set_num_entries(int val) {
    persist_write_int(NUM_ENTRIES, val);
}

void persist_set_city(char *val) {
    persist_write_string(CITY, val);
}
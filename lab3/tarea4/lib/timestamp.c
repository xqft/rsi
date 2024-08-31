#include "contiki.h"
#include "timestamp.h"

// ponele que clock_seconds() = 5 s
// hago un set:
// timestamp_offset = 10000 - clock_seconds() = 9995

// pasan 5 seg mas
// hago un get:
// clock_seconds() = 10 s
// timestamp = timestamp_offset + clock_seconds() = 9995 + 10 = 10005

static unsigned long timestamp_offset = 0;

unsigned long timestamp_get(void) {
    return timestamp_offset + clock_seconds();
}

void timestamp_set(unsigned long int timestamp) {
    timestamp_offset = timestamp - clock_seconds();
}
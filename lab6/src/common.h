#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

/* (a * b) mod m без переполнения 64-бит */
uint64_t MultModulo(uint64_t a, uint64_t b, uint64_t mod);

#endif
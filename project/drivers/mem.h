// mem.h
#ifndef MEM_H
#define MEM_H

#include <stdint.h>

void* memset(void* dest, int value, unsigned int count);
void* memcpy(void* dest, const void* src, unsigned int count);

#endif

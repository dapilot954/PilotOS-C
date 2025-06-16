// mem.c
#include "mem.h"

void* memset(void* dest, int value, unsigned int count) {
    unsigned char* ptr = (unsigned char*)dest;
    while (count--) *ptr++ = (unsigned char)value;
    return dest;
}

void* memcpy(void* dest, const void* src, unsigned int count) {
    unsigned char* dst = (unsigned char*)dest;
    const unsigned char* src8 = (const unsigned char*)src;
    while (count--) *dst++ = *src8++;
    return dest;
}

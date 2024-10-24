#ifndef AUXILIAR_H
#define AUXILIAR_H

#include <stddef.h>
#define FLAG 0x7E
#define ESCAPE 0x7D

void byteStuffing(const unsigned char *input, size_t inputLength, unsigned char *output, size_t *outputLength);

void byteDestuffing(const unsigned char *input, size_t inputLength, unsigned char *output, size_t *outputLength);

unsigned char calculateBCC1(unsigned char a, unsigned char b);

unsigned char calculateBCC2(const unsigned char *buf, int bufSize);



#endif
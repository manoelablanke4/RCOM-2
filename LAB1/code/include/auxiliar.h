#ifndef AUXILIAR_H
#define AUXILIAR_H

#include <stddef.h>
#include <stdio.h>
#define FLAG 0x7E
#define ESCAPE 0x7D
#define CONTROL_START 0X01
#define CONTROL_END 0X03
#define TYPE_FILE 0X00
#define TYPE_FILENAME 0x02
#define CONTROL_DATA 0x02

void byteStuffing(const unsigned char *input, size_t inputLength, unsigned char *output, size_t *outputLength);

void byteDestuffing(const unsigned char *input, size_t inputLength, unsigned char *output, size_t *outputLength);

unsigned char calculateBCC1(unsigned char a, unsigned char b);

unsigned char calculateBCC2(const unsigned char *buf, int bufSize);

int SendFile(const char *file);

int ReceiveFile(const char *file);

int getFileSize(FILE *file);
int createControlPacket(unsigned char controlByte, unsigned char *packet, int fileSize, const char *filename);

int createDataPacket(unsigned char *packet, int sequenceNUmber, unsigned char *data, int dataSize);

#endif
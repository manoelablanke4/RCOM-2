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


//STUFFING AND DESTUFFING
// Function to stuff the bytes in the data field of the frame. In case the byte is FLAG or ESCAPE, it will be stuffed.
//@param input: the data to be stuffed
//@param inputLength: the length of the data to be stuffed
//@param output: the stuffed data
//@param outputLength: the length of the stuffed data
void byteStuffing(const unsigned char *input, size_t inputLength, unsigned char *output, size_t *outputLength);

// Function to destuff the bytes in the data field of the frame. In case the byte is ESCAPE followed by 0x5E or 0x5D, it will be destuffed.
//@param input: the data to be destuffed
//@param inputLength: the length of the data to be destuffed
//@param output: the destuffed data
//@param outputLength: the length of the destuffed data
void byteDestuffing(const unsigned char *input, size_t inputLength, unsigned char *output, size_t *outputLength);

//BCC CALCULATION
// Function to calculate the first Block Check Character, which is the XOR of two bytes
//@param a: the first byte to be XORed      
//@param b: the second byte to be XORed
//@return the result of the XOR operation
unsigned char calculateBCC1(unsigned char a, unsigned char b);

// Function to calculate the second Block Check Character, which is the XOR of all the bytes in the buffer
//@param buf: the buffer to be XORed
//@param bufSize: the size of the buffer
//@return the result of the XOR operation
unsigned char calculateBCC2(const unsigned char *buf, int bufSize);

//FILE FUNCTIONS
// Function to get the size of a file
//@param fp: the file to get the size
//@return the size of the file
int getFileSize(FILE *file);

// Function to send a file
//@param file: the name of the file to be sent
//@return 0 if the file was sent successfully, -1 otherwise
int SendFile(const char *file);

// Function to receive a file
//@param file: the name of the file to be received
//@return 0 if the file was received successfully, -1 otherwise
int ReceiveFile(const char *file);

//PACKET CREATION
// Function to create a control packet which contains control data byte, file size, filename length and filename
//@param controlByte: the control byte of the packet
//@param packet: pointer to the packet to be created
//@param fileSize: the size of the file to be sent
//@param filename: pointer to the name of the file to be sent
//@return the size of the packet created
int createControlPacket(unsigned char controlByte, unsigned char *packet, int fileSize, const char *filename);

// Function to create a data packet which contains control data byte, sequence number, data size(L1 and L2) and data
//@param packet: pointer to the packet to be created
//@param sequenceNumber: the sequence number of the packet (bewteen 0 and 99)
//@param data: pointer to the data to be sent
//@param dataSize: the size of the data to be sent
//@return the size of the packet created
int createDataPacket(unsigned char *packet, int sequenceNUmber, unsigned char *data, int dataSize);

#endif
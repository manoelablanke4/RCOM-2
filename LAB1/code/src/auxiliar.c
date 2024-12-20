#include "auxiliar.h"
#include "link_layer.h"
#include "serial_port.h"
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>


//STUFFING AND DESTUFFING
// Function to stuff the bytes in the data field of the frame. In case the byte is FLAG or ESCAPE, it will be stuffed.
//@param input: the data to be stuffed
//@param inputLength: the length of the data to be stuffed
//@param output: the stuffed data
//@param outputLength: the length of the stuffed data
void byteStuffing(const unsigned char *input, size_t inputLength, unsigned char *output, size_t *outputLength){
    size_t newLength = 0;

    for(size_t i = 0; i< inputLength;i++){
        if(input[i] == FLAG){
            output[newLength] = ESCAPE;
            newLength++;
            output[newLength] = 0x5E;
            newLength++;
        }
        else if(input[i] == ESCAPE){
            output[newLength] = ESCAPE;
            newLength++;
            output[newLength] = 0x5D;
            newLength++;

        }
        else{
            output[newLength] = input[i];
            newLength++;
        }
    }

    *outputLength = newLength;
}

// Function to destuff the bytes in the data field of the frame. In case the byte is ESCAPE followed by 0x5E or 0x5D, it will be destuffed.
//@param input: the data to be destuffed
//@param inputLength: the length of the data to be destuffed
//@param output: the destuffed data
//@param outputLength: the length of the destuffed data


void byteDestuffing(const unsigned char *input, size_t inputLength, unsigned char *output, size_t *outputLength){
    size_t newLength = 0;

    for(size_t i = 0; i< inputLength;i++){
        if(input[i] == ESCAPE){
            if(i +1 < inputLength){
                if(input[i+1] == 0x5E){
                    output[newLength] = FLAG;
                    newLength++;
                    i++;
                }
                else if(input[i+1] == 0x5D){
                    output[newLength] = ESCAPE;
                    newLength++;
                    i++;
                }
            }
        }
        else{
            output[newLength] = input[i];
            newLength++;
        }
    }

    *outputLength = newLength;
}



//BCC CALCULATION
// Function to calculate the first Block Check Character, which is the XOR of two bytes
//@param a: the first byte to be XORed
//@param b: the second byte to be XORed
//@return the result of the XOR operation
unsigned char calculateBCC1(unsigned char a, unsigned char b) {
    return a ^ b;
}

// Function to calculate the second Block Check Character, which is the XOR of all the bytes in the buffer
//@param buf: the buffer to be XORed
//@param bufSize: the size of the buffer
//@return the result of the XOR operation
unsigned char calculateBCC2(const unsigned char *buf, int bufSize){
    unsigned char bcc = 0x00;
    for(size_t i = 0; i < bufSize; i++){
        bcc ^= buf[i];
    }
    return bcc;
}


//PACKET CREATION
// Function to create a control packet
//@param controlByte: the control byte of the packet which can be CONTROL_START(in case of the start control packet) or CONTROL_END(in case of the end control packet)
//@param packet: the packet to be created
//@param fileSize: the size of the file to be sent
//@param filename: the name of the file to be sent
//@return the size of the packet created
int createControlPacket(unsigned char controlByte, unsigned char *packet, int fileSize, const char *filename){
    int index = 0;
    packet[index++] = controlByte;
    packet[index++] = TYPE_FILE;
    packet[index++] = 4;
    packet[index++] = (fileSize>>24)& 0xFF;
    packet[index++] = (fileSize>>16)& 0xFF;
    packet[index++] = (fileSize>>8)& 0xFF;
    packet[index++] = (fileSize)& 0xFF;
    packet[index++]= TYPE_FILENAME;
    packet[index++] = strlen(filename)+1;
    for(int i = 0; i < strlen(filename); i++){
        packet[index++] = filename[i];
    }
    return index;
}

// Function to create a data packet which contains control data byte, sequence number, data size(L1 and L2) and data
//@param packet: the packet to be created
//@param sequenceNumber: the sequence number of the packet (bewteen 0 and 99)
//@param data: the data to be sent
//@param dataSize: the size of the data to be sent
//@return the size of the packet created
int createDataPacket(unsigned char *packet, int sequenceNumber, unsigned char *data, int dataSize){
    int index = 0;
    packet[index++] = CONTROL_DATA;
    packet[index++] = (unsigned char )sequenceNumber;

    unsigned char L1 = dataSize & 0xFF; 
    unsigned char L2 = (dataSize >> 8) & 0xFF; 
    packet[index++] = L2;
    packet[index++] = L1;
   
    for(int i = 0; i < dataSize; i++){
        packet[index++] = data[i];
    }
    return index;
}

//FILE FUNCTIONS
// Function to get the size of a file 
//@param fp: the file to get the size
//@return the size of the file
   int getFileSize(FILE *fp){

    int lsize;
    
    fseek(fp, 0, SEEK_END);
    lsize = (int)ftell(fp);
    rewind(fp);

    return lsize;
}

// Function to send a file
//@param filename: the name of the file to be sent
//@return 0 if the file was sent successfully, -1 otherwise

int SendFile(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error opening file\n");
        return -1;
    }

    int fileSize = getFileSize(file);
    unsigned char controlPacket[MAX_PAYLOAD_SIZE+24];
    int controlPacketSize = createControlPacket(CONTROL_START, controlPacket, fileSize, filename);

    if (controlPacketSize < 0) {
        printf("Error building control packet\n");
        fclose(file);
        return -1;
    }

    if (llwrite(controlPacket, controlPacketSize) < 0) {
        printf("Error sending control packet\n");
        fclose(file);
        return -1;
    }

    printf("Control packet sent to start\n");

    unsigned char dataPacket[MAX_PAYLOAD_SIZE];
    unsigned char rawPacket[MAX_PAYLOAD_SIZE+4];
    int bytesRead;
    int sequenceNumber = 0;

    while ((bytesRead = fread(dataPacket, 1, MAX_PAYLOAD_SIZE, file)) > 0) {
        printf("bytes read: %d\n", bytesRead);
        printf("Sending data packet %d\n", sequenceNumber);
        int packetSize = createDataPacket(rawPacket, sequenceNumber, dataPacket, bytesRead);
        sequenceNumber = (sequenceNumber + 1) % 100;

        if (packetSize < 0) {
            printf("Error building data packet\n");
            fclose(file);
            return -1;
        }

        if (llwrite(rawPacket, packetSize) < 0) {
            printf("Error sending data packet\n");
            fclose(file);
            return -1;
        }
    }
    // Send end control packet
    controlPacketSize = createControlPacket(CONTROL_END, controlPacket, fileSize, filename);
    if (controlPacketSize < 0) {
        printf("Error building end control packet\n");
        fclose(file);
        return -1;
    }

    if (llwrite(controlPacket, controlPacketSize) < 0) {
        printf("Error sending end control packet\n");
        fclose(file);
        return -1;
    }

    printf("File transfer complete (send).\n");
    fclose(file);
    return 0;
}

// Function to receive a file
//@param filename: the name of the file to be received
//@return 0 if the file was received successfully, -1 otherwise

 int ReceiveFile(const char *filename) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        printf("Error opening file\n");
        return -1;
    }

    unsigned char controlPacket[MAX_PAYLOAD_SIZE+24];
    int bytesRead = llread(controlPacket);
    if (bytesRead < 0) {
        printf("Error receiving control packet\n");
        fclose(file);
        return -1;
    }

    if (controlPacket[0] != CONTROL_START) {
        printf("Expected start control packet\n");
        fclose(file);
        return -1;
    }

    //int fileSize = (controlPacket[3] << 24) | (controlPacket[4] << 16) | (controlPacket[5] << 8) | controlPacket[6];
    int filenameLength = controlPacket[8];
    char receivedFilename[MAX_PAYLOAD_SIZE+24];
    strncpy(receivedFilename, (char *)controlPacket + 9, filenameLength);
    receivedFilename[filenameLength] = '\0';

    int tries = 0;
    // if (strcmp(filename, receivedFilename) != 0) {
    //     printf("Filename mismatch\n");
    //     fclose(file);
    //     return -1;
    // }

    unsigned char dataPacket[MAX_PAYLOAD_SIZE+24];
    int sequenceNumber = 0;
    

    while (1) {
        bytesRead = llread(dataPacket);
        if (bytesRead < 0) {
            tries++;
            printf("Error receiving data packet\n");
            continue;
            // if(tries == maxTries){
            //     printf("Max tries reached\n");
            //     fclose(file);
            //     return -1;
            // }
        }
        // tries = 0;
        printf("The header is %u\n", dataPacket[0]);
        if (dataPacket[1] != sequenceNumber) {
            printf("Expected sequence number %d, received %d\n", sequenceNumber, dataPacket[1]);
            fclose(file);
            return -1;
        }
        int dataSize = (dataPacket[2] << 8) | dataPacket[3];
        if(dataPacket[0] == CONTROL_DATA) fwrite(dataPacket + 4, sizeof(unsigned char), dataSize, file);
        printf("The data size is %u\n", dataSize);
        if (dataSize < MAX_PAYLOAD_SIZE) {
            break;
        }
        sequenceNumber = (sequenceNumber + 1) % 100;
    }
    bytesRead = llread(controlPacket);
    if (bytesRead < 0) {
        printf("Error receiving end control packet\n");
        fclose(file);
        return -1;
    }

    if (controlPacket[0] != CONTROL_END) {
        printf("Expected end control packet\n");
        fclose(file);
        return -1;
    }

    printf("File transfer complete.\n");
    fclose(file);
    return 0;
}   
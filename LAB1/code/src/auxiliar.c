#include "auxiliar.h"
#include "link_layer.h"
#include "serial_port.h"
#include <unistd.h>
#include <stdio.h>
#include <signal.h>



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

unsigned char calculateBCC1(unsigned char a, unsigned char b) {
    return a ^ b;
}

unsigned char calculateBCC2(const unsigned char *buf, int bufSize){
    unsigned char bcc = 0x00;
    for(size_t i = 0; i < bufSize; i++){
        bcc ^= buf[i];
    }
    return bcc;
}

int sendFile(const char *filename)
{
FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        printf("Error opening file\n");
        return -1;
    }


    unsigned char buf[MAX_PAYLOAD_SIZE];
    int fileSize = getFileSize(file);

    while (fileSize > 0)
    {
        int bytesRead = fread(buf, 1, MAX_PAYLOAD_SIZE, file);
        llwrite(buf, bytesRead);
        fileSize -= bytesRead;
    }
    return 0;}

    int getFileSize(FILE *fp){

    int lsize;
    
    fseek(fp, 0, SEEK_END);
    lsize = (int)ftell(fp);
    rewind(fp);

    return lsize;
}
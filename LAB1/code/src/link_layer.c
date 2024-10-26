// Link layer protocol implementation

#include "link_layer.h"
#include "serial_port.h"
#include "auxiliar.h"
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <stdbool.h>


// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source
#define BUF_SIZE 5
#define SEND_TIMES 3
int alarmEnabled = FALSE;
int alarmCount = 0;
int send_times = 0;
unsigned char ACTUAL;
volatile int STOP = FALSE;
LinkLayer ll;

#define NS_0 0x00
#define NS_1 0x80

unsigned char control = 0x00;
#define START 0 
#define FLAG_RCV 1 
#define A_RCV 2 
#define C_RCV 3 
#define BCC_OK 4 
////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;

    printf("Alarm #%d\n", alarmCount);
}

int llopen(LinkLayer connectionParameters)
{
    int fd =openSerialPort(connectionParameters.serialPort,
                       connectionParameters.baudRate);
    if (fd < 0) return 1;
    ll.nRetransmissions = connectionParameters.nRetransmissions;
    ll.timeout = connectionParameters.timeout;
    ll.role = connectionParameters.role;
    ll.baudRate = connectionParameters.baudRate;


    unsigned char buf[BUF_SIZE] = {0};
    unsigned char buf2[BUF_SIZE] = {0};
    unsigned char A, C, F;
    A = 0x03; C= 0x03; F= 0x7E;
    unsigned char ACTUAL;
    (void)signal(SIGALRM, alarmHandler);

    switch (connectionParameters.role)
    {
    case LlTx:
        
        A = 0x03; C= 0x03; F= 0x7E;
        buf[0] = F;
        buf[1] = A;
        buf[2] = C;
        buf[3] = A^C;
        buf[4] = F;

        writeBytesSerialPort(buf, BUF_SIZE);
        send_times++;
        alarm(3);
        sleep(1);
        ACTUAL = START;

        while (STOP == FALSE && alarmCount < 3)
        {
            // Retorna após 5 caracteres terem sido recebidos

            readByteSerialPort(buf);
            switch (ACTUAL){
            case START:
                printf("i recieved this %u and im at START\n", buf[0]);
                if(buf[0] == F) ACTUAL = FLAG_RCV;
                
                break;
            case FLAG_RCV:
                printf("i recieved this %u im at FLAG_RCV\n", buf[0]);
                if(buf[0] == F) ACTUAL = FLAG_RCV;
                else if(buf[0] == A) ACTUAL = A_RCV;
                else ACTUAL = START;
                
                break;
            case A_RCV:
                printf("i recieved this %u and im at A_RCV\n", buf[0]);
                if(buf[0] == F) ACTUAL = FLAG_RCV;
                else if(buf[0] == 0x07) ACTUAL = C_RCV;
                else ACTUAL = START;
                
                break;
            case C_RCV:
            printf("i recieved this %u and im at C_RCV\n", buf[0]);
                if(buf[0] == F) ACTUAL = FLAG_RCV;
                else if(buf[0] != 0x00) ACTUAL = BCC_OK;
                else ACTUAL = START;
            
                break;
            case BCC_OK:
            printf("i recieved this %u and im at BCC_OK\n", buf[0]);
                if(buf[0] == F) STOP=TRUE;
                else ACTUAL = START;
                break;
            default:
                break;
            }
            if(STOP==TRUE) {
                alarm(0);
                printf("Received message is correct\n");
            } else if(alarmCount > 0 && alarmCount == send_times){
                printf("Resending message\n");
                buf[0] = F;
                buf[1] = A;
                buf[2] = C;
                buf[3] = A^C;
                buf[4] = F;
                writeBytesSerialPort(buf, BUF_SIZE);
                send_times++;
                alarm(3);
            }
        }   
        if(STOP == FALSE ) return -1;
        break;
    case LlRx:
        ACTUAL = START;
        
        while (STOP == FALSE)
    {
        // Retorna após 1 caracteres terem sido recebidos
        readByteSerialPort(buf);
        switch (ACTUAL){
            case START:
                printf("i recieved this %u and im at START\n", buf[0]);
                if(buf[0] == F) ACTUAL = FLAG_RCV;
                
                break;
            case FLAG_RCV:
                printf("i recieved this %u im at FLAG_RCV\n", buf[0]);
                if(buf[0] == F) ACTUAL = FLAG_RCV;
                else if(buf[0] == A) ACTUAL = A_RCV;
                else ACTUAL = START;
                
                break;
            case A_RCV:
                printf("i recieved this %u and im at A_RCV\n", buf[0]);
                if(buf[0] == F) ACTUAL = FLAG_RCV;
                else if(buf[0] == C) ACTUAL = C_RCV;
                else ACTUAL = START;
                
                break;
            case C_RCV:
            printf("i recieved this %u and im at C_RCV\n", buf[0]);
                if(buf[0] == F) ACTUAL = FLAG_RCV;
                else if(buf[0] == 0x00) ACTUAL = BCC_OK;
                else ACTUAL = START;
            
                break;
            case BCC_OK:
            printf("i recieved this %u and im at BCC_OK\n", buf[0]);
                if(buf[0] == F) STOP=TRUE;
                else ACTUAL = START;
                break;
            default:
                break;
            }
            if(STOP==TRUE){
                buf[0] = F;
                buf[1] = A;
                C = 0x07;
                buf[2] = C;
                buf[3] = A^C;
                buf[4] = F;

                writeBytesSerialPort(buf, BUF_SIZE);
                sleep(1);
                printf("Congrats!\n");
            }
    }   
        break;
    default:
        break;
    }

    closeSerialPort();
    return 1;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{ 
    alarmCount = 0; send_times = 0;
    unsigned char stuffedFrame[256];
    size_t stuffedLength;
    unsigned char wanted = 0xAB;

    byteStuffing(buf, bufSize, stuffedFrame, &stuffedLength);

    unsigned char transformedFrame[256];

    transformedFrame[0] = FLAG;
    transformedFrame[1] = 0x03;
    transformedFrame[2] = control;
    transformedFrame[3] = calculateBCC1(transformedFrame[1], transformedFrame[2]);
    
    for(int i = 0; i < stuffedLength; i++){
        transformedFrame[i+4] = stuffedFrame[i];
    }

    transformedFrame[stuffedLength+4] = FLAG;
    transformedFrame[stuffedLength+5] = calculateBCC2(stuffedFrame, stuffedLength);

    size_t transformedLength = stuffedLength + 6;

    if(writeBytesSerialPort(transformedFrame, transformedLength) == transformedLength){send_times++;}
    else { printf ("Error writing to serial port\n"); };

    send_times++;
    alarm(ll.timeout);
    ACTUAL=START;

    unsigned char C;

    while(STOP == FALSE && alarmCount < ll.nRetransmissions){   
        readByteSerialPort(buf);
        switch (ACTUAL){
        case START:
            printf("i recieved this %u and im at START\n", buf[0]);
            if(buf[0] == FLAG) ACTUAL = FLAG_RCV;
            break;
        case FLAG_RCV:
            printf("i recieved this %u im at FLAG_RCV\n", buf[0]);
            if(buf[0] == FLAG) ACTUAL = FLAG_RCV;
            else if(buf[0] == 0X03) ACTUAL = A_RCV;
            else ACTUAL = START;
            break;
        case A_RCV:
            printf("i recieved this %u and im at A_RCV\n", buf[0]);
            if(buf[0] == 0XAA){ 
                C = 0xAA;
                ACTUAL = C_RCV;
                control = 0x00;
                }
            else if(buf[0] == 0xAB){
                C = 0xAB;
                ACTUAL = C_RCV;
                control = 0x80;
            }
            else if(buf[0] == 0x54){
                printf("Received REJ\n");
                send_times= 0;
                writeBytesSerialPort(transformedFrame, transformedLength);
                send_times++;
                ACTUAL = START;
            }
            else if (buf[0]== 0x55){
                printf("Received RJ\n");
                send_times= 0;
                writeBytesSerialPort(transformedFrame, transformedLength);
                send_times++;
                ACTUAL = START;
            }
            else ACTUAL = START;
            break;
        case C_RCV:
        printf("i recieved this %u and im at C_RCV\n", buf[0]);
            if(buf[0] == FLAG) ACTUAL = FLAG_RCV;
            else if(buf[0] == calculateBCC1(0x03,C)) ACTUAL = BCC_OK;
            else ACTUAL = START;
            break;         

        case BCC_OK:
        printf("i recieved this %u and im at BCC_OK\n", buf[0]);
            if(buf[0] == FLAG) STOP=TRUE;
            else ACTUAL = START;
            break;
        default:
            break;
        }
        if(STOP==TRUE) {
            alarm(0);
            printf("Received message is correct\n");
        } else if(alarmCount > 0 && alarmCount == send_times){
            printf("Resending message\n");
            writeBytesSerialPort(transformedFrame, transformedLength); //We already have the frame? Just ship it
            send_times++;
            alarm(3);
        }
    }
    return stuffedLength;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    // TODO

    return 0;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics)
{
    // TODO

    int clstat = closeSerialPort();
    return clstat;
}

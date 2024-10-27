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
unsigned char T_STATE;
unsigned char R_STATE;
volatile int STOP = FALSE;
LinkLayer ll;

#define NS_0 0x00
#define NS_1 0x80

unsigned char control = 0x00;
#define START 0 
#define FLAG_RCV 1 
#define A_RCV 2 
#define C_RCV 3 
#define BCC1_OK 4
#define BCC2_OK 5
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
        alarm(ll.timeout);
        ACTUAL = START;

        while (STOP == FALSE && alarmCount < ll.nRetransmissions)
        {
            // Retorna após 5 caracteres terem sido recebidos

            if(readByteSerialPort(buf) > 0)
            {
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
                    else if(buf[0] != 0x00) ACTUAL = BCC1_OK;
                    else ACTUAL = START;

                    break;
                case BCC1_OK:
                printf("i recieved this %u and im at BCC1_OK\n", buf[0]);
                    if(buf[0] == F) STOP=TRUE;
                    else ACTUAL = START;
                    break;
                default:
                    break;
                    }
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
                alarm(ll.timeout);
            }
        }   
        if(STOP == FALSE ) return -1;
        break;
    case LlRx:
        ACTUAL = START;
        
        while (STOP == FALSE)
    {
        // Retorna após 1 caracteres terem sido recebidos
        if(readByteSerialPort(buf) > 0)
        {
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
                else if(buf[0] == 0x00) ACTUAL = BCC1_OK;
                else ACTUAL = START;
            
                break;
            case BCC1_OK:
            printf("i recieved this %u and im at BCC1_OK\n", buf[0]);
                if(buf[0] == F) STOP=TRUE;
                else ACTUAL = START;
                break;
            default:
                break;
            }
        }
            if(STOP==TRUE){
                buf[0] = F;
                buf[1] = A;
                C = 0x07;
                buf[2] = C;
                buf[3] = A^C;
                buf[4] = F;

                writeBytesSerialPort(buf, BUF_SIZE);
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
    unsigned char bcc2 = calculateBCC2(buf, bufSize);
    alarmCount = 0; send_times = 0;
    unsigned char stuffedFrame[256];
    size_t stuffedLength;
    unsigned char response[256];

    byteStuffing(buf, bufSize, stuffedFrame, &stuffedLength);
    
    unsigned char transformedFrame[256];

    transformedFrame[0] = FLAG;
    transformedFrame[1] = 0x03;
    transformedFrame[2] = control;
    transformedFrame[3] = calculateBCC1(transformedFrame[1], transformedFrame[2]);
    
    for(int i = 0; i < stuffedLength; i++){
        transformedFrame[i+4] = stuffedFrame[i];
    }

    transformedFrame[stuffedLength+4] = bcc2;
    transformedFrame[stuffedLength+5] = FLAG;

    size_t transformedLength = stuffedLength + 6;

    if(writeBytesSerialPort(transformedFrame, transformedLength) == transformedLength){send_times++;}
    else { printf ("Error writing to serial port\n"); };

    send_times++;
    alarm(ll.timeout);
    T_STATE=START;

    unsigned char C;

    while(STOP == FALSE && alarmCount < ll.nRetransmissions){   
        readByteSerialPort(response);
        switch (T_STATE){
        case START:
            printf("i recieved this %u and im at START\n", response[0]);
            if(response[0] == FLAG) T_STATE = FLAG_RCV;
            break;
        case FLAG_RCV:
            printf("i recieved this %u im at FLAG_RCV\n", response[0]);
            if(response[0] == FLAG) T_STATE = FLAG_RCV;
            else if(response[0] == 0X03) T_STATE = A_RCV;
            else T_STATE = START;
            break;
        case A_RCV:
            printf("i recieved this %u and im at A_RCV\n", response[0]);
            if(response[0] == 0XAA){ 
                C = 0xAA;
                T_STATE = C_RCV;
                control = 0x00;
                }
            else if(response[0] == 0xAB){
                C = 0xAB;
                T_STATE = C_RCV;
                control = 0x80;
            }
            else if(response[0] == 0x54){
                printf("Received REJ\n");
                send_times= 0;
                writeBytesSerialPort(transformedFrame, transformedLength);
                send_times++;
                T_STATE = START;
            }
            else if (response[0]== 0x55){
                printf("Received RJ\n");
                send_times= 0;
                writeBytesSerialPort(transformedFrame, transformedLength);
                send_times++;
                T_STATE = START;
            }
            else T_STATE = START;
            break;
        case C_RCV:
        printf("i recieved this %u and im at C_RCV\n", response[0]);
            if(response[0] == FLAG) T_STATE = FLAG_RCV;
            else if(response[0] == calculateBCC1(0x03,C)) T_STATE = BCC1_OK;
            else T_STATE = START;
            break;         

        case BCC1_OK:
        printf("i recieved this %u and im at BCC1_OK\n", response[0]);
            if(response[0] == FLAG) STOP=TRUE;
            else T_STATE = START;
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
            alarm(ll.timeout);
        }
    }
    return transformedLength;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    unsigned char buf[256];
    unsigned char frame[256];
    unsigned int pos;
    unsigned char num_frame;
    size_t frameLength;
    unsigned char destuffedFrame[256];
    R_STATE = START;

    while (STOP == FALSE)
    {
        // Retorna após 1 caracteres terem sido recebidos
        if(readByteSerialPort(buf) != 0)
        {
            frameLength++;
            switch (R_STATE){
            case START:
                printf("i recieved this %u and im at START\n", buf[0]);
                if(buf[0] == FLAG) {R_STATE = FLAG_RCV; frame[pos++] = buf[0];}
                break;
            case FLAG_RCV:
                printf("i recieved this %u im at FLAG_RCV\n", buf[0]);
                if(buf[0] == FLAG) R_STATE = FLAG_RCV;
                else if(buf[0] == 0x03) {R_STATE = A_RCV; frame[pos++] = buf[0];}
                else R_STATE = START;
                break;
            case A_RCV:
                printf("i recieved this %u and im at A_RCV\n", buf[0]);
                if(buf[0] == FLAG) R_STATE = FLAG_RCV;
                else if(buf[0] == 0x00) {R_STATE = C_RCV; frame[pos++] = buf[0]; num_frame = 0;} 
                else if(buf[0] == 0x80) {R_STATE = C_RCV; frame[pos++] = buf[0]; num_frame = 1;}
                else R_STATE = START;
                break;
            case C_RCV:
                printf("i recieved this %u and im at C_RCV\n", buf[0]);
                if(buf[0] == FLAG) R_STATE = FLAG_RCV;
                else if(buf[0] == calculateBCC1(frame[pos-1], frame[pos-2])) R_STATE = BCC1_OK;
                else {R_STATE = START; printf("BCC1 error\n");}
                break;
            case BCC1_OK:
            printf("i recieved this %u and im at BCC1_OK\n", buf[0]);
                if(buf[0] == FLAG) STOP=TRUE;
                else R_STATE = START;
                break;
            default:
                break;
            }
        }
            if(STOP==TRUE){

                // Look if there is something to put before
                writeBytesSerialPort(frame, frameLength);
                printf("Receiver sending response");
            }
    } 
    

    return frameLength;
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

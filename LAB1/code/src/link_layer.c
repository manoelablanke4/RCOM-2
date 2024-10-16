// Link layer protocol implementation

#include "link_layer.h"
#include "serial_port.h"
#include <unistd.h>
#include <stdio.h>

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source
#define BUF_SIZE 5
#define SEND_TIMES 3
extern int alarmCount;
extern int alarmEnabled;
int send_times = 0;
volatile int STOP = FALSE;


#define START 0 
#define FLAG_RCV 1 
#define A_RCV 2 
#define C_RCV 3 
#define BCC_OK 4 
////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{
    int fd =openSerialPort(connectionParameters.serialPort,
                       connectionParameters.baudRate);
    if (fd < 0) return 1;
    
    unsigned char buf[BUF_SIZE] = {0};
    unsigned char A, C, F;
    A = 0x03; C= 0x03; F= 0x7E;
    unsigned char ACTUAL;


    switch (connectionParameters.role)
    {
    case LlTx:

        A = 0x03; C= 0x03; F= 0x7E;
        buf[0] = F;
        buf[1] = A;
        buf[2] = C;
        buf[3] = A^C;
        buf[4] = F;


        // In non-canonical mode, '\n' does not end the writing.
        // Test this condition by placing a '\n' in the middle of the buffer.
        // The whole buffer must be sent even with the '\n'.

        writeBytesSerialPort(buf, BUF_SIZE);
        send_times++;
        alarm(3);

        // Wait until all bytes have been written to the serial port
        sleep(1);

        while (STOP == FALSE && alarmCount < 3)
        {
            
            // Retorna após 5 caracteres terem sido recebidos
            readByteSerialPort(buf);
            // Itera sobre os bytes lidos e imprime cada um como unsigned char
            if(buf[0] == 0x7E && buf[1] == 0x03 && buf[2] == 0x07 && buf[3] != 0x00 && buf[4] == 0x7E) {
                alarm(0);
                printf("Received message is correct\n");
                STOP = TRUE;
            } else if(alarmCount > 0 && alarmCount == send_times){
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
        A = 0x03; C= 0x03; F= 0x7E;
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
                printf("Congrats!\n");
            
                buf[0] = F;
                buf[1] = A;
                C = 0x07;
                buf[2] = C;
                buf[3] = A^C;
                buf[4] = F;

                writeBytesSerialPort(buf, BUF_SIZE);
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
    // TODO

    return 0;
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

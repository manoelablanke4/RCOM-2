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
#define BUF_SIZE 5 // Size of the buffer
#define SEND_TIMES 3 // Number max of times the message is sent
int alarmEnabled = FALSE; // Flag to check if the alarm is enabled
int alarmCount = 0; // Number of alarms
int send_times = 0; // Number of times the message is sent
int sequenceNumber = 0; 
unsigned char ACTUAL; // Actual state of the state machine
unsigned char T_STATE; // State of the transmitter
unsigned char R_STATE; // State of the receiver
unsigned int bytes_read = 0;
volatile int STOP = FALSE; // Flag to check if the program should stop
LinkLayer ll; 
int number_of_retransmissions = 0;
int number_of_frames_read = 0;
int number_of_frames_written = 0;
#define NS_0 0x00 // Information frame 0
#define NS_1 0x80 // Information frame 1
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

// Function to open the serial port
// @param connectionParameters: the parameters to open the serial port
// @return the file descriptor of the serial port
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

    unsigned char A, C;
    A = 0x03; C= 0x03;

    (void)signal(SIGALRM, alarmHandler);

    // Switch case to check the role of the connection
    switch (connectionParameters.role)
    {
    // Transmitter
    case LlTx:

        A = 0x03; C= 0x03;
        // Create the message
        buf[0] = FLAG;
        buf[1] = A;
        buf[2] = C;
        buf[3] = A^C;
        buf[4] = FLAG;

        writeBytesSerialPort(buf, BUF_SIZE); // Send the message
        send_times++; // Increment the number of times the message was sent
        alarm(ll.timeout); // Enable the alarm
        ACTUAL = START; // Set the actual state to START

        while (STOP == FALSE && alarmCount < ll.nRetransmissions)
        {
    
            if(readByteSerialPort(buf) > 0)
            {
                switch (ACTUAL){
                    //Stays in the START state until it receives a FLAG
                case START:
                    printf("i recieved this %u and im at START\n", buf[0]);
                    if(buf[0] == FLAG) ACTUAL = FLAG_RCV;
                    break;
                case FLAG_RCV:
                    printf("i recieved this %u im at FLAG_RCV\n", buf[0]);
                    if(buf[0] == FLAG) ACTUAL = FLAG_RCV;
                    else if(buf[0] == A) ACTUAL = A_RCV;
                    else ACTUAL = START;
                    break;
                case A_RCV:
                    printf("i recieved this %u and im at A_RCV\n", buf[0]);
                    if(buf[0] == FLAG) ACTUAL = FLAG_RCV;
                    else if(buf[0] == 0x07) ACTUAL = C_RCV;
                    else ACTUAL = START;
                    break;
                case C_RCV:
                printf("i recieved this %u and im at C_RCV\n", buf[0]);
                    if(buf[0] == FLAG) ACTUAL = FLAG_RCV;
                    else if(buf[0] != 0x00) ACTUAL = BCC1_OK;
                    else ACTUAL = START;
                    break;
                case BCC1_OK:
                printf("i recieved this %u and im at BCC1_OK\n", buf[0]);
                    if(buf[0] == FLAG) STOP=TRUE;
                    else ACTUAL = START;
                    break;
                default:
                    break;
                    }
            }
            if(STOP==TRUE) {
                alarm(0);
                printf("Received message is correct\n");
            }
            // If the program didn't stop and the alarm is enabled, resend the message
            else if(alarmCount > 0 && alarmCount == send_times){
                printf("Resending message\n");
                buf[0] = FLAG;
                buf[1] = A;
                buf[2] = C;
                buf[3] = A^C;
                buf[4] = FLAG;
                writeBytesSerialPort(buf, BUF_SIZE);
                send_times++;
                alarm(ll.timeout);
            }
        }   

        // else, return -1
        if(STOP == FALSE ) return -1;
        break;
    // Receiver
    case LlRx:
        ACTUAL = START;
        
        while (STOP == FALSE)
    {
        if(readByteSerialPort(buf) > 0)
        {
            switch (ACTUAL){
                //Stays in the START state until it receives a FLAG
            case START:
                printf("i recieved this %u and im at START\n", buf[0]);
                if(buf[0] == FLAG) ACTUAL = FLAG_RCV;
                
                break;
            case FLAG_RCV:
                printf("i recieved this %u im at FLAG_RCV\n", buf[0]);
                if(buf[0] == FLAG) ACTUAL = FLAG_RCV;
                else if(buf[0] == A) ACTUAL = A_RCV;
                else ACTUAL = START;
                
                break;
            case A_RCV:
                printf("i recieved this %u and im at A_RCV\n", buf[0]);
                if(buf[0] == FLAG) ACTUAL = FLAG_RCV;
                else if(buf[0] == C) ACTUAL = C_RCV;
                else ACTUAL = START;
                
                break;
            case C_RCV:
            printf("i recieved this %u and im at C_RCV\n", buf[0]);
                if(buf[0] == FLAG) ACTUAL = FLAG_RCV;
                else if(buf[0] == 0x00) ACTUAL = BCC1_OK;
                else ACTUAL = START;
            
                break;
            case BCC1_OK:
            printf("i recieved this %u and im at BCC1_OK\n", buf[0]);
                if(buf[0] == FLAG) STOP=TRUE; // If the message is correct, stop the program
                else ACTUAL = START;
                break;
            default:
                break;
            }
        }
            if(STOP==TRUE){
                buf[0] = FLAG;
                buf[1] = A;
                C = 0x07;
                buf[2] = C;
                buf[3] = A^C;
                buf[4] = FLAG;

                writeBytesSerialPort(buf, BUF_SIZE);
                printf("Congrats!\n");
            }
    }   
        break;
    default:
        break;
    }

    return 1;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////

// Function to write the bytes to the serial port
// @param buf: the buffer to be written
// @param bufSize: the size of the buffer
// @return the number of bytes written
int llwrite(const unsigned char *buf, int bufSize)
{
    STOP = FALSE;
    T_STATE=START;
    R_STATE=START;

    unsigned char bcc2 = calculateBCC2(buf, bufSize);
    alarmCount = 0; send_times = 0;
    unsigned char stuffedFrame[MAX_PAYLOAD_SIZE+24];
    size_t stuffedLength;
    unsigned char response[MAX_PAYLOAD_SIZE+24];

    byteStuffing(buf, bufSize, stuffedFrame, &stuffedLength);

    unsigned char transformedFrame[MAX_PAYLOAD_SIZE+24];

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

    alarm(ll.timeout);

    unsigned char C;

    while(STOP == FALSE && alarmCount <= ll.nRetransmissions){   
        if(readByteSerialPort(response) >  0)
        {
        switch (T_STATE){
        case START:
            printf("i recieved this %u and im at START\n", response[0]);
            if(response[0] != FLAG);
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
             
                T_STATE = START;
                return -1;
            }
            else if (response[0]== 0x55){
                printf("Received REJ\n");
                send_times= 0;
                T_STATE = START;
                return -1;
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
    }
        if(STOP==TRUE) {
            alarm(0);
            printf("Received message is correct\n");
            number_of_frames_written++;
        } else if(alarmCount > 0 && alarmCount == send_times){
            printf("Resending message\n");
            send_times++;
            alarm(ll.timeout);
            number_of_retransmissions++;
            // unsigned char flagtest[1];
            // flagtest[0] = FLAG;
            // writeBytesSerialPort(flagtest, 1);

            writeBytesSerialPort(transformedFrame, transformedLength); //We already have the frame? Just ship it
        }
    }
    if(STOP == FALSE) return -1;

    return transformedLength;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////

// Function to read the bytes from the serial port
// @param packet: the buffer to store the bytes read
// @return the number of bytes read

int llread(unsigned char *packet)
{
    STOP = FALSE;
    R_STATE = START;
    unsigned char buf[MAX_PAYLOAD_SIZE+24];
    unsigned char data[MAX_PAYLOAD_SIZE+24];
    unsigned char A=0, C=0;
    unsigned int pos=0;
    unsigned char num_frame;
    size_t frameLength=0;
    unsigned char data_destuffed[MAX_PAYLOAD_SIZE+24];
    size_t data_destuffed_length;
    R_STATE = START;
    bool error = false;

    while (STOP == FALSE)
    {
        // Retorna após 1 caracteres terem sido recebidos
        if(readByteSerialPort(buf) > 0)
        {
            frameLength++;
            switch (R_STATE){
            case START:
                printf("i recieved this %u and im at START\n", buf[0]);
                if(buf[0] == FLAG) {R_STATE = FLAG_RCV; }
                break;
            case FLAG_RCV:
                printf("i recieved this %u im at FLAG_RCV\n", buf[0]);
                if(buf[0] == FLAG) R_STATE = FLAG_RCV;
                else if(buf[0] == 0x03) {R_STATE = A_RCV; A = buf[0]; }
                else R_STATE = START;
                break;
            case A_RCV:
                printf("i recieved this %u and im at A_RCV\n", buf[0]);
                if(buf[0] == FLAG) R_STATE = FLAG_RCV;
                else if(buf[0] == 0x00) {R_STATE = C_RCV; num_frame = 0; C = buf[0]; } 
                else if(buf[0] == 0x80) {R_STATE = C_RCV; num_frame = 1; C = buf[0]; }
                else R_STATE = START;
                break;
            case C_RCV:
                printf("i recieved this %u and im at C_RCV\n", buf[0]);
                if(buf[0] == FLAG) R_STATE = FLAG_RCV;
                else if(buf[0] == calculateBCC1(A, C)) {R_STATE = BCC1_OK; }
                else {R_STATE = START; printf("BCC1 error because is this value %u\n", calculateBCC1(A,C));}
                break;
            case BCC1_OK:
                printf("i recieved this %u and im at DATA_READING\n", buf[0]);
                if(buf[0] != FLAG) {
                    data[pos++]=buf[0];
                }
                
                else if(buf[0] == FLAG && send_times == 0) {
                    byteDestuffing(data, pos - 1, data_destuffed, &data_destuffed_length);
                    if(data[pos-1] == calculateBCC2(data_destuffed, data_destuffed_length)) {
                        error = false;
                    }
                    else {error = true; printf("BCC2 Error\n"); 
                    return -1;}
    
                    STOP = TRUE;
                }
                else {R_STATE = START; printf("Flag not received\n");}
                break;
            default:
                break;
            }
        }
            if(STOP==TRUE){

                unsigned char frame[MAX_PAYLOAD_SIZE+24];
                frame[0] = FLAG;
                frame[1] = 0x03;
                switch (num_frame)
                {
                case 0:
                    if(!error) frame[2] = 0xAB;
                    else {frame[2] = 0x54; printf("BCC Error\n");alarm(0);return -1;}
                    break;
                case 1:
                    if(!error) frame[2] = 0xAA;
                    else {frame[2] = 0x55; printf("BCC Error\n");alarm(0);return -1;}
                    break;
                default:
                    break;
                }

                frame[3] = calculateBCC1(frame[1], frame[2]);
                frame[4] = FLAG;
                number_of_frames_read++;
                writeBytesSerialPort(frame, BUF_SIZE);
                printf("Receiver sending response\n");
            }
    }
    if(data_destuffed[0] == 0x01 || data_destuffed[0] == 0x03) {
        for(int i=0; i < data_destuffed_length; i++){
            packet[i] = data_destuffed[i];
        }
        printf("Bytes read for control: %zu\n", frameLength);
        bytes_read += data_destuffed_length;
        printf("Total bytes read: %u\n", bytes_read);
        return frameLength;
    }
    else if(data_destuffed[1] == sequenceNumber && data_destuffed[0] == 0x02)
    {
        for(int i=0; i < data_destuffed_length; i++){
            packet[i] = data_destuffed[i];
        }
        sequenceNumber++;
        bytes_read += data_destuffed_length;
        printf("Total bytes read: %u\n", bytes_read);
        printf("Bytes read for data: %zu\n", frameLength);
        return frameLength;
    }
    printf("Bytes read and error: %zu\n", frameLength);
    return -1;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////

// Function to close the serial port
// @param showStatistics: flag to show the statistics
// @return 1 if the serial port was closed successfully, -1 otherwise
int llclose(int showStatistics)
{
    bool disc_ok = false;
    unsigned char A, C;
    send_times = 0;
    STOP=FALSE;
    ACTUAL=START;
    unsigned char supervision[MAX_PAYLOAD_SIZE+24];
    unsigned char buf[MAX_PAYLOAD_SIZE+24];
    switch (ll.role)
    {
    case LlTx:
        
        supervision[0] = FLAG;
        supervision[1] = 0x03;
        supervision[2] = 0x0B;
        supervision[3] = calculateBCC1(supervision[1], supervision[2]);
        supervision[4] = FLAG;
        writeBytesSerialPort(supervision, BUF_SIZE);
        printf("Tx sending DISC\n");
        send_times++;
        alarm(ll.timeout);
        ACTUAL = START;

        while (STOP == FALSE && alarmCount < ll.nRetransmissions)
        {

            if(readByteSerialPort(buf) > 0)
            {
                switch (ACTUAL){
                case START:
                    printf("i recieved this %u and im at START\n", buf[0]);
                    if(buf[0] == FLAG) ACTUAL = FLAG_RCV;

                    break;
                case FLAG_RCV:
                    printf("i recieved this %u im at FLAG_RCV\n", buf[0]);
                    if(buf[0] == FLAG) ACTUAL = FLAG_RCV;
                    else if(buf[0] == 0X03) {ACTUAL = A_RCV; A = buf[0];}
                    else ACTUAL = START;
                    break;
                case A_RCV:
                    printf("i recieved this %u and im at A_RCV\n", buf[0]);
                    if(buf[0] == FLAG) ACTUAL = FLAG_RCV;
                    else if(buf[0] == 0x0B) {ACTUAL = C_RCV; C= buf[0];}
                    else ACTUAL = START;
                    break;
                case C_RCV:
                printf("i recieved this %u and im at C_RCV\n", buf[0]);
                    if(buf[0] == FLAG) ACTUAL = FLAG_RCV;
                    else if(buf[0] == calculateBCC1(A, C)) ACTUAL = BCC1_OK;
                    else ACTUAL = START;
                    break;
                case BCC1_OK:
                printf("i recieved this %u and im at BCC1_OK\n", buf[0]);
                    if(buf[0] == FLAG) STOP=TRUE;
                    else ACTUAL = START;
                    break;
                default:
                    break;
                    }
            }
            if(STOP==TRUE) {
                disc_ok = true;
                unsigned char ua[MAX_PAYLOAD_SIZE+24];
                A = 0x03; C= 0x07;
                ua[0] = FLAG;
                ua[1] = A;
                ua[2] = C;
                ua[3] = A^C;
                ua[4] = FLAG;
                writeBytesSerialPort(ua, BUF_SIZE);
                alarm(0);
                printf("Tx sending UA\n");
            } else if(alarmCount > 0 && alarmCount == send_times){
                printf("Resending disc message\n");
                writeBytesSerialPort(supervision, BUF_SIZE);
                send_times++;
                alarm(ll.timeout);
            }
        }
    printf("Number of frames written successfully: %d\n", number_of_frames_written);
    printf("Number of retransmissions: %d\n", number_of_retransmissions);
    int sum1 = number_of_frames_written + number_of_retransmissions;
    int percentage1 = (number_of_retransmissions * 100) / sum1;
    printf("Percentage of retransmissions: %d\n", percentage1);
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
                if(buf[0] == FLAG) ACTUAL = FLAG_RCV;             
                break;
            case FLAG_RCV:
                printf("i recieved this %u im at FLAG_RCV\n", buf[0]);
                if(buf[0] == FLAG) ACTUAL = FLAG_RCV;
                else if(buf[0] == 0x03) {ACTUAL = A_RCV; A = buf[0];}
                else ACTUAL = START;
                
                break;
            case A_RCV:
                printf("i recieved this %u and im at A_RCV\n", buf[0]);
                if(buf[0] == FLAG) ACTUAL = FLAG_RCV;
                else if(buf[0] == 0x0B) {ACTUAL = C_RCV; C = buf[0];}
                else if(disc_ok && buf[0] == 0x07) {ACTUAL = C_RCV; C = buf[0];}
                else ACTUAL = START;
                
                break;
            case C_RCV:
            printf("i recieved this %u and im at C_RCV\n", buf[0]);
                if(buf[0] == FLAG) ACTUAL = FLAG_RCV;
                else if(buf[0] == calculateBCC1(A,C)) ACTUAL = BCC1_OK;
                else ACTUAL = START;
            
                break;
            case BCC1_OK:
            printf("i recieved this %u and im at BCC1_OK\n", buf[0]);
                if(buf[0] == FLAG) STOP=TRUE;
                else ACTUAL = START;
                break;
            default:
                break;
            }
        }
            if(STOP==TRUE && !disc_ok){
                disc_ok = true;
                ACTUAL=START;
                STOP=FALSE;
                unsigned char disc[BUF_SIZE];
                disc[0] = FLAG;
                disc[1] = 0x03;
                disc[2] = 0x0B;
                disc[3] = calculateBCC1(disc[1], disc[2]);
                disc[4] = FLAG;
                writeBytesSerialPort(disc, BUF_SIZE);
                printf("Receiver send Disc!\n");
            }
            else if(STOP==TRUE && disc_ok){
                printf("Rx terminated susccesfully\n");
                printf("Number of frames read successfully: %d\n", number_of_frames_read);
                printf("Total number of bytes read: %d\n", bytes_read);
            }

    }
        break;
    default:
        break;
    }
    int clstat = closeSerialPort();
    return clstat;
}

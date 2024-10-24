// Read from serial port in non-canonical mode
//
// Modified by: Eduardo Nuno Almeida [enalmeida@fe.up.pt]

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

// Baudrate settings are defined in <asm/termbits.h>, which is
// included by <termios.h>
#define BAUDRATE B38400
#define _POSIX_SOURCE 1 // POSIX compliant source

#define FALSE 0
#define TRUE 1

#define BUF_SIZE 5

#define START 0 
#define FLAG_RCV 1 
#define A_RCV 2 
#define C_RCV 3 
#define BCC_OK 4 

volatile int STOP = FALSE;

int main(int argc, char *argv[])
{
    // Program usage: Uses either COM1 or COM2
    const char *serialPortName = argv[1];
   
    

    if (argc < 2)
    {
        printf("Incorrect program usage\n"
               "Usage: %s <SerialPort>\n"
               "Example: %s /dev/ttyS1\n",
               argv[0],
               argv[0]);
        exit(1);
    }

    // Open serial port device for reading and writing and not as controlling tty
    // because we don't want to get killed if linenoise sends CTRL-C.
    int fd = open(serialPortName, O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
        perror(serialPortName);
        exit(-1);
    }

    struct termios oldtio;
    struct termios newtio;

    // Save current port settings
    if (tcgetattr(fd, &oldtio) == -1)
    {
        perror("tcgetattr");
        exit(-1);
    }

    // Clear struct for new port settings
    memset(&newtio, 0, sizeof(newtio));

    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    // Set input mode (non-canonical, no echo,...)
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 0; // Inter-character timer unused
    newtio.c_cc[VMIN] = 5;  // Blocking read until 5 chars received

    // VTIME e VMIN should be changed in order to protect with a
    // timeout the reception of the following character(s)

    // Now clean the line and activate the settings for the port
    // tcflush() discards data written to the object referred to
    // by fd but not transmitted, or data received but not read,
    // depending on the value of queue_selector:
    //   TCIFLUSH - flushes data received but not read.
    tcflush(fd, TCIOFLUSH);

    // Set new port settings
    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    printf("New termios structure set\n");
	
    // Loop for input
    unsigned char buf[BUF_SIZE + 1] = {0}; // +1: Save space for the final '\0' char
	unsigned char ACTUAL;
	unsigned char F, A, C;
	A = 0x03; C= 0x03; F= 0x7E;

	
	ACTUAL = START;
	
    while (STOP == FALSE)
{
    // Retorna após 1 caracteres terem sido recebidos
    int bytes = read(fd, buf, 1);
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

            int bytes = write(fd, buf, BUF_SIZE);
		}
}   

    // The while() cycle should be changed in order to respect the specifications
    // of the protocol indicated in the Lab guide

    // Restore the old port settings
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    close(fd);

    return 0;
}
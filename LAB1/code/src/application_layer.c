// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"
#include "auxiliar.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define NS_0 0x00
#define NS_1 0x80


void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    bool transmitter_ok = false; 
    bool receiver_ok = false;
    printf("Application layer\n");
    LinkLayer port;
    LinkLayerRole port_role;
    if (strcmp(role, "tx") == 0)
    port_role = LlTx;
    else
    port_role = LlRx;
    
    strncpy(port.serialPort, serialPort, sizeof(port.serialPort) - 1);
    port.role = port_role;
    port.baudRate = baudRate;
    port.nRetransmissions = nTries;
    port.timeout = timeout;

     if(llopen(port) < 0){
       printf("Error opening connection\n");
     }
     printf("\n---------------llopen done---------------\n\n");
      if(port_role == LlTx){
        printf("Transmitter\n");
        SendFile(filename);
        transmitter_ok = true;
      } 
      else{
        printf("Receiver\n");
        ReceiveFile(filename);
        receiver_ok = true;

      }
      if(llclose(1) < 0){
        printf("Error closing connection\n");
      }
    
      printf("\n---------------llclose done---------------\n\n");
}

// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"

#include <stdio.h>
#include <string.h>

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
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
        return; 
     }

}

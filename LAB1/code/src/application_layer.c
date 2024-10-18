// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"
#include "string.h"

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    LinkLayer port;
    LinkLayerRole port_role;
    if(role == "tx") port_role= LlTx; else port_role=LlRx;
    
    strncpy(port.serialPort, serialPort, sizeof(port.serialPort) - 1);
    port.baudRate = baudRate;
    port.nRetransmissions = nTries;
    port.timeout = timeout;

    llopen(port);
}

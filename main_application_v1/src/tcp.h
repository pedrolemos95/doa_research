#ifndef TCP_H
#define TCP_H

int tcpOpen(char* ip, char* port);

void tcpTx(uint32_t dataLength, uint8_t* data);

int32_t tcpRx(uint32_t dataLength, uint8_t* data);

int32_t tcpRxPeek(void);

int tcpClose();

#endif
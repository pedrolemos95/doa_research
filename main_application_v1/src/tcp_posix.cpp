#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <netinet/ip.h>
#include <pthread.h>
#include "tcp.h"

/*****************************************************************************/
/*                             DEFINITIONS                                   */
/*****************************************************************************/

#define DEFAULT_BUFLEN 512
#define MAX_HANDLE_NUM 64

/*****************************************************************************/
/*                           GLOBAL VARIABLES                                */
/*****************************************************************************/

static int        socketHandles[MAX_HANDLE_NUM];
static pthread_t  threadIDs[MAX_HANDLE_NUM];
static int        activeThreadsUsingTCP = 0;

/*****************************************************************************/
/*                           STATIC FUNCTIONS                                */
/*****************************************************************************/

static void saveSocketHandle(int handle)
{
  pthread_t threadID = pthread_self();

  for (int i = 0; i < activeThreadsUsingTCP; i++) {
    if (threadIDs[i] == threadID) {
      socketHandles[i] = handle;
      return;
    }
  }

  threadIDs[activeThreadsUsingTCP] = threadID;
  socketHandles[activeThreadsUsingTCP] = handle;
  activeThreadsUsingTCP++;
}

static int getSocketHandle()
{
  pthread_t threadID = pthread_self();

  for (int i = 0; i < activeThreadsUsingTCP; i++) {
    if (threadIDs[i] == threadID) {
      return socketHandles[i];
    }
  }

  return -1;
}

static struct sockaddr_in getipa(const char* hostname, int port){
  struct sockaddr_in ipa;

  ipa.sin_family = AF_INET;
  ipa.sin_port = htons(port);

  struct hostent* host = gethostbyname(hostname);
  if(!host){
    printf("Error while resolving hostname\n");

    return ipa;
  }

  // Attempt to connect to the first address
  char* addr = host->h_addr_list[0];
  memcpy(&ipa.sin_addr.s_addr, addr, strlen(addr));

  return ipa;
}

/*****************************************************************************/
/*                           PUBLIC FUNCTIONS                                */
/*****************************************************************************/

int tcpOpen(char* ip, char* port)
{
  struct protoent* tcp;
  struct sockaddr_in isa;
  int ConnectSocket;

  tcp = getprotobyname("tcp");

  // Resolve the server address and port
  isa = getipa(ip, atoi(port));

  // Create a SOCKET for connecting to server
  ConnectSocket = socket(PF_INET, SOCK_STREAM, tcp->p_proto);
  if(ConnectSocket == -1){
    printf("Error while creating a tcp socket\n");
    return -1;
  }

  // Connect to server.
  if(connect(ConnectSocket, (struct sockaddr*)&isa, sizeof isa) == -1){
    printf("Error while connecting to server\n");
    shutdown(ConnectSocket, SHUT_RDWR);
    ConnectSocket = -1;
  }

  saveSocketHandle(ConnectSocket);

  // Should really try the next address returned by gethostbyname
  // if the connect call failed

  if (getSocketHandle() == -1) {
    printf("Unable to connect to server!\n");
    return -1;
  }

  //printf("Connected to server\r\n");

  return 0;
}

void tcpTx(uint32_t dataLength, uint8_t* data)
{
  int iResult;

  if (getSocketHandle() == -1){
    return;
  }

  iResult = send(getSocketHandle(), (void*) data, (size_t) dataLength, MSG_NOSIGNAL);

  if (iResult == -1) {
    //printf("send failed: %d\n", WSAGetLastError());
    tcpClose();
  }
}

int32_t tcpRx(uint32_t dataLength, uint8_t* data)
{
  /** The amount of bytes still needed to be read. */
  uint32_t dataToRead = dataLength;
  /** The amount of bytes read. */
  uint32_t dataRead;

  if (getSocketHandle() == -1){
    return -1;
  }

  while (dataToRead) {
    ssize_t size = recv(getSocketHandle(), (void*) data, (size_t) dataToRead, MSG_WAITALL);

    if (size == -1) {
      //printf("recv failed");
      tcpClose();
      return -1;
    } else {
      dataRead = size;
      if (!dataRead) {
        continue;
      }
    }
    dataToRead -= dataRead;
    data += dataRead;
  }

  return (int32_t)dataLength;
}

int32_t tcpRxPeek(void)
{
  u_long count;

  if (getSocketHandle() == -1){
    return -1;
  }

  ioctl(getSocketHandle(), FIONREAD, &count);

  return count;
}

int tcpClose()
{
  // cleanup
  shutdown(getSocketHandle(), SHUT_RDWR);
  saveSocketHandle(-1);

  return 0;
}

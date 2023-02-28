#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "common.h"
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdio.h>
#include "tcp.h"

//#pragma comment(lib, "Ws2_32.lib")

/*****************************************************************************/
/*                             DEFINITIONS                                   */
/*****************************************************************************/

#define DEFAULT_BUFLEN 512
#define MAX_HANDLE_NUM 64

/*****************************************************************************/
/*                           GLOBAL VARIABLES                                */
/*****************************************************************************/

static SOCKET  socketHandles[MAX_HANDLE_NUM];
static DWORD   threadIDs[MAX_HANDLE_NUM];
static int     activeThreadsUsingTCP = 0;

/*****************************************************************************/
/*                           STATIC FUNCTIONS                                */
/*****************************************************************************/

static void saveSocketHandle(SOCKET handle)
{
  DWORD threadID = GetCurrentThreadId();

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

static SOCKET getSocketHandle()
{
  DWORD threadID = GetCurrentThreadId();

  for (int i = 0; i < activeThreadsUsingTCP; i++) {
    if (threadIDs[i] == threadID) {
      return socketHandles[i];
    }
  }

  return INVALID_SOCKET;
}

/*****************************************************************************/
/*                           PUBLIC FUNCTIONS                                */
/*****************************************************************************/

int tcpOpen(char* ip, char* port)
{
  struct addrinfo *result = NULL, *ptr = NULL, hints;
  SOCKET ConnectSocket;
  WSADATA wsaData;
  int iResult;

  // Initialize Winsock
  iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
  if (iResult != 0) {
    //printf("WSAStartup failed: %d\n", iResult);
    return 1;
  }

  ZeroMemory( &hints, sizeof(hints) );
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  //printf("connecting to: %s\r\n",ip);

  // Resolve the server address and port
  iResult = getaddrinfo(ip, port, &hints, &result);
  if (iResult != 0) {
    //printf("getaddrinfo failed: %d\n", iResult);
    WSACleanup();
    return -1;
  }

  // Attempt to connect to the first address returned by
  // the call to getaddrinfo
  ptr=result;

  // Create a SOCKET for connecting to server
  ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
  if (ConnectSocket == INVALID_SOCKET) {
    //printf("Error at socket(): %ld\n", WSAGetLastError());
    freeaddrinfo(result);
    WSACleanup();
    return -1;
  }

  // Connect to server.
  iResult = connect( ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
  if (iResult == SOCKET_ERROR) {
    closesocket(ConnectSocket);
    ConnectSocket = INVALID_SOCKET;
  }

  saveSocketHandle(ConnectSocket);

  // Should really try the next address returned by getaddrinfo
  // if the connect call failed
  // But for this simple example we just free the resources
  // returned by getaddrinfo and print an error message

  freeaddrinfo(result);

  if (getSocketHandle() == INVALID_SOCKET) {
    //printf("Unable to connect to server!\n");
    WSACleanup();
    return -1;
  }

  //printf("Connected to server\r\n");

  return 0;
}

void tcpTx(uint32_t dataLength, uint8_t* data)
{
  int iResult;

  if (getSocketHandle() == INVALID_SOCKET){
    return;
  }

  iResult = send(getSocketHandle(), (const char*) data, (int) dataLength, 0);

  if (iResult == SOCKET_ERROR) {
    //printf("send failed: %d\n", WSAGetLastError());
    tcpClose();
  }
}

int32_t tcpRx(uint32_t dataLength, uint8_t* data)
{
  /** Variable for storing function return values. */
  int iResult;
  /** The amount of bytes still needed to be read. */
  uint32_t dataToRead = dataLength;
  /** The amount of bytes read. */
  uint32_t dataRead;

  if (getSocketHandle() == INVALID_SOCKET){
    return -1;
  }

  while (dataToRead) {
    iResult = recv(getSocketHandle(), (char*) data, (int) dataToRead, 0);

    if (iResult == SOCKET_ERROR) {
      //printf("recv failed: %d\n", WSAGetLastError());
      tcpClose();
      return -1;
    } else {
      dataRead = iResult;
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

  if (getSocketHandle() == INVALID_SOCKET){
    return -1;
  }

  ioctlsocket(getSocketHandle(), FIONREAD, &count);

  return count;
}

int tcpClose()
{
  // cleanup
  closesocket(getSocketHandle());
  saveSocketHandle(INVALID_SOCKET);
  WSACleanup();

  return 0;
}

#ifndef GECKO_BGLIB_H
#define GECKO_BGLIB_H
/**
 * gecko_bglib.h
 *
 */

/*****************************************************************************
 *
 *  This is an adaptation layer between host application and BGAPI protocol.
 *  It provides synchronization mechanism for BGAPI-protocol that allows
 *  using same application architecture between application in mcu and external
 *  host.
 *
 *  Synchronization is done by waiting for response after each command. If
 *  any events are received during response waiting, they are queued and
 *  delivered next time gecko_wait_event is called.
 *
 *  Queue length is controlled by defining macro "BGLIB_QUEUE_LEN", default is 30.
 *  Queue length depends on use cases and allowed host memory usage.
 *
 ****************************************************************************/

#if _MSC_VER  //msvc
#define inline __inline
#endif

#include "common.h"
#include "host_gecko.h"

#ifndef BGLIB_QUEUE_LEN
#define BGLIB_QUEUE_LEN 30
#endif

typedef struct {
  struct gecko_cmd_packet gecko_queue[BGLIB_QUEUE_LEN];     
  int    gecko_queue_w;                                 
  int    gecko_queue_r;
} bglibContext_t;

/**
 * Initialize BGLIB
 * @param OFUNC
 * @param IFUNC
 */
#define BGLIB_INITIALIZE(OFUNC, IFUNC) bglib_output = OFUNC; bglib_input = IFUNC; bglib_peek = NULL; MUTEX_INIT(&bgBufferCriticalSection);

/**
 * Initialize BGLIB to support nonblocking mode
 * @param OFUNC
 * @param IFUNC
 * @param PFUNC peek function to check if there is data to be read from UART
 */
#define BGLIB_INITIALIZE_NONBLOCK(OFUNC, IFUNC, PFUNC) bglib_output = OFUNC; bglib_input = IFUNC; bglib_peek = PFUNC; MUTEX_INIT(&bgBufferCriticalSection);

extern void(*bglib_output)(uint32_t len1, uint8_t* data1);
extern int32_t (*bglib_input)(uint32_t len1, uint8_t* data1);
extern int32_t(*bglib_peek)(void);

extern MUTEX_T bgBufferCriticalSection;

#endif

#include "common.h"
#include "gecko_bglib.h"


struct gecko_cmd_packet _gecko_cmd_msg;                   
struct gecko_cmd_packet _gecko_rsp_msg;                   
struct gecko_cmd_packet *gecko_cmd_msg = &_gecko_cmd_msg; 
struct gecko_cmd_packet *gecko_rsp_msg = &_gecko_rsp_msg; 

#define MAX_BGLIB_INSTANCE_COUNT 16
static uint8_t        activeThreadsUsingBglib = 0;
static bglibContext_t bglibContext[MAX_BGLIB_INSTANCE_COUNT];
static THREAD_ID_T    threadIDs[MAX_BGLIB_INSTANCE_COUNT];

void (*bglib_output)(uint32_t len1, uint8_t* data1);      
int32_t (*bglib_input)(uint32_t len1, uint8_t* data1);    
int32_t (*bglib_peek)(void);                              

static bglibContext_t* getBglibContext()
{
  THREAD_ID_T threadID = GET_THREAD_ID();

  for (int i = 0; i < activeThreadsUsingBglib; i++) {
    if (threadIDs[i] == threadID) {
      return &bglibContext[i];
    }
  }

  threadIDs[activeThreadsUsingBglib] = threadID;
  bglibContext[activeThreadsUsingBglib].gecko_queue_w = 0;
  bglibContext[activeThreadsUsingBglib].gecko_queue_r = 0;
  activeThreadsUsingBglib++;

  return &bglibContext[activeThreadsUsingBglib-1];
}

static struct gecko_cmd_packet* gecko_wait_message(void)//wait for event from system
{
  uint32_t msg_length;
  uint32_t header;
  uint8_t  *payload;
  struct gecko_cmd_packet *pck, *retVal = NULL;
  int      ret;

  bglibContext_t* context = getBglibContext();

  //sync to header byte
  ret = bglib_input(1, (uint8_t*)&header);
  if (ret < 0 || (header & 0x78) != gecko_dev_type_gecko) {
    return 0;
  }
  ret = bglib_input(BGLIB_MSG_HEADER_LEN - 1, &((uint8_t*)&header)[1]);
  if (ret < 0) {
    return 0;
  }

  msg_length = BGLIB_MSG_LEN(header);

  if (msg_length > BGLIB_MSG_MAX_PAYLOAD) {
    return 0;
  }

  if ((header & 0xf8) == (gecko_dev_type_gecko | gecko_msg_type_evt)) {
    //received event
    if ((context->gecko_queue_w + 1) % BGLIB_QUEUE_LEN == context->gecko_queue_r) {
      //drop packet
      if (msg_length) {
        uint8 tmp_payload[BGLIB_MSG_MAX_PAYLOAD];
        bglib_input(msg_length, tmp_payload);
      }
      return 0;      //NO ROOM IN QUEUE
    }
    pck = &(context->gecko_queue[context->gecko_queue_w]);
    context->gecko_queue_w = (context->gecko_queue_w + 1) % BGLIB_QUEUE_LEN;
  } else if ((header & 0xf8) == gecko_dev_type_gecko) {//response
    retVal = pck = gecko_rsp_msg;
  } else {
    //fail
    return 0;
  }
  pck->header = header;
  payload = (uint8_t*)&pck->data.payload;
  /**
   * Read the payload data if required and store it after the header.
   */
  if (msg_length) {
    ret = bglib_input(msg_length, payload);
    if (ret < 0) {
      return 0;
    }
  }

  // Using retVal avoid double handling of event msg types in outer function
  return retVal;
}

int gecko_event_pending(void)
{
  bglibContext_t* context = getBglibContext();

  if (context->gecko_queue_w != context->gecko_queue_r) {//event is waiting in queue
    return 1;
  }

  //something in uart waiting to be read
  if (bglib_peek && bglib_peek()) {
    return 1;
  }

  return 0;
}

struct gecko_cmd_packet* gecko_get_event(int block)
{
  struct gecko_cmd_packet* p;

  bglibContext_t* context = getBglibContext();

  while (1) {
    if (context->gecko_queue_w != context->gecko_queue_r) {
      p = &(context->gecko_queue[context->gecko_queue_r]);
      context->gecko_queue_r = (context->gecko_queue_r + 1) % BGLIB_QUEUE_LEN;
      return p;
    }
    //if not blocking and nothing in uart -> out
    if (!block && bglib_peek && bglib_peek() == 0) {
      return NULL;
    }

    //read more messages from device
    if ( (p = gecko_wait_message()) ) {
      return p;
    }
  }
}

struct gecko_cmd_packet* gecko_wait_event(void)
{
  return gecko_get_event(1);
}

struct gecko_cmd_packet* gecko_peek_event(void)
{
  return gecko_get_event(0);
}

struct gecko_cmd_packet* gecko_wait_response(void)
{
  struct gecko_cmd_packet* p;
  while (1) {
    p = gecko_wait_message();
    if (p && !(p->header & gecko_msg_type_evt)) {
      return p;
    }
  }
}

void gecko_handle_command(uint32_t hdr, void* data)
{
  ENTER_MUTEX(&bgBufferCriticalSection);

  //packet in gecko_cmd_msg is waiting for output
  bglib_output(BGLIB_MSG_HEADER_LEN + BGLIB_MSG_LEN(gecko_cmd_msg->header), (uint8_t*)gecko_cmd_msg);
  gecko_wait_response();

  EXIT_MUTEX(&bgBufferCriticalSection);
}

void gecko_handle_command_noresponse(uint32_t hdr, void* data)
{
  //packet in gecko_cmd_msg is waiting for output
  bglib_output(BGLIB_MSG_HEADER_LEN + BGLIB_MSG_LEN(gecko_cmd_msg->header), (uint8_t*)gecko_cmd_msg);
}

/***********************************************************************************************//**
 * @file   bg.c
 * @brief  Module responsible for communicating with the Bluetooth stack. The application finds
 *         available tags, connects to them, enables CTE on them, and receives IQ samples. 
 ***************************************************************************************************
 * # License
 * <b>Copyright 2019 Silicon Laboratories Inc. www.silabs.com</b>
 ***************************************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this software is governed 
 * by the terms of Silicon Labs Master Software License Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This software is distributed to
 * you in Source Code format and is governed by the sections of the MSLA applicable to Source Code.
 **************************************************************************************************/

#include "common.h"
#include "bg_types.h"
#include "gecko_bglib.h"
#include "bg.h"
#include "aox.h"
#include "tcp.h"
#include "uart.h"
#include "serial.h"

#define ERROR_CODE_MAX_TAGS_REACHED -1
#define ERROR_CODE_TAG_NOT_FOUND -2

/***************************************************************************************************
 * Local Macros and Definitions
 **************************************************************************************************/

/***************************************************************************************************
 * Public Variable Declarations
 **************************************************************************************************/

// IQ sample buffer
iqSamples_t iqSamplesBuffered[MAX_NUM_TAGS][MAX_NUM_LOCATORS];

uint8_t foundTags[MAX_NUM_TAGS][6];
uint32_t tagCounter;

/***************************************************************************************************
 * Static Variable Declarations
 **************************************************************************************************/

// Antenna switching pattern
static const uint8_t  antenna_array[NUM_ANTENNAS] = SWITCHING_PATTERN;

/***************************************************************************************************
 * Static Function Declarations
 **************************************************************************************************/

static void bgInit(int locatorID, void* pLocatorConfig);
static void bgWaitNewEvent(int locatorID, struct gecko_cmd_packet **evt);
static void bgProcessEvent(int locatorID, bool* appBooted, struct gecko_cmd_packet *evt);
static int32_t AddToFoundTags(bd_addr addr);
static int32_t GetTagId(bd_addr addr);
static int32_t HandleFoundTags(bd_addr addr, uint32_t locatorID);

static int32_t AddToFoundTags(bd_addr addr)
{
  int32_t index = GetTagId(addr);

  if (index == ERROR_CODE_TAG_NOT_FOUND)
  {
    // Returns tag id or -1 if max number of tags reached
    if (tagCounter >= MAX_NUM_TAGS)
      return ERROR_CODE_MAX_TAGS_REACHED;

    index = tagCounter;

    foundTags[index][0] = addr.addr[0];
    foundTags[index][1] = addr.addr[1];
    foundTags[index][2] = addr.addr[2];
    foundTags[index][3] = addr.addr[3];
    foundTags[index][4] = addr.addr[4];
    foundTags[index][5] = addr.addr[5];

    tagCounter++;
  }

  return index;
}

static int32_t GetTagId(bd_addr addr)
{
  // Return tag index if found or -1 if not found
  for (uint32_t index = 0; index < MAX_NUM_TAGS; index++)
  {
    if (foundTags[index][0] == addr.addr[0] &&
      foundTags[index][1] == addr.addr[1] &&
      foundTags[index][2] == addr.addr[2] &&
      foundTags[index][3] == addr.addr[3] &&
      foundTags[index][4] == addr.addr[4] &&
      foundTags[index][5] == addr.addr[5]) {
      return index;
    }
  }

  // Tag not yet found
  return ERROR_CODE_TAG_NOT_FOUND;
}

static int32_t HandleFoundTags(bd_addr addr, uint32_t locatorID)
{
  return AddToFoundTags(addr);
}


/***************************************************************************************************
 * Public Function Definitions
 **************************************************************************************************/

THREAD_RETURN_T bgMain(void* pLocatorConfig) 
{
  struct gecko_cmd_packet* evt;
  int    locatorID = ((locatorConfig_t*)pLocatorConfig)->locatorID;
  bool   appBooted = false;

  // Initialize BG
  bgInit(locatorID, pLocatorConfig);

  // Wait for threads to complete initialization
  while (eAppCtrl!=eAOX_SHUTDOWN) {
    // Wait for new Bluetooth stack event
    bgWaitNewEvent(locatorID, &evt);

    // Process Bluetooth event
    bgProcessEvent(locatorID, &appBooted, evt);
  }

  // Reset NCP to ensure it gets into a defined state
  ENTER_MUTEX(&printfCriticalSection);
  printf("                                                            \r");
  printf("Resetting NCP target locatorID %d ...\r\n", locatorID);
  fflush(stdout);
  EXIT_MUTEX(&printfCriticalSection);

  gecko_cmd_system_reset(0);

  if (((locatorConfig_t*)pLocatorConfig)->IPaddress[0] != '-') {
    // Clear tcp buffer
    uint8_t buf[2];
    for (int i = 0; i < 10000; i++) {
      if (tcpRxPeek() > 0) {
        tcpRx(1, buf);
      }
      fflush(stdout);
    }
  }

  THREAD_EXIT;
}

/***************************************************************************************************
 * Static Function Definitions
 **************************************************************************************************/
bool initialized = false;

static void bgInit(int locatorID, void* pLocatorConfig)
{
  if (!initialized) {
    for (int j = 0; j < MAX_NUM_TAGS; j++) {
      for (int k = 0; k < 6; k++) {
        foundTags[j][k] = 0;
      }
    }

    initialized = true;
    fflush(stdout);
  }

  // Make sure IQ sample buffers are not read/written while buffers are not initialized

  if (((locatorConfig_t*)pLocatorConfig)->IPaddress[0] != '-') {
    // Initialize BGLIB with our output function for sending messages
    BGLIB_INITIALIZE_NONBLOCK(tcpTx, tcpRx, tcpRxPeek);

    // Initialise serial communication as non-blocking
    if (tcpOpen(((locatorConfig_t*)pLocatorConfig)->IPaddress, "4901") < 0) {
      ENTER_MUTEX(&printfCriticalSection);
      printf("Non-blocking TCP port init failure\n");
      fflush(stdout);
      EXIT_MUTEX(&printfCriticalSection);
      exit(EXIT_FAILURE);
    }
  }
  else if (((locatorConfig_t*)pLocatorConfig)->COMport[0] != '-') {
    // Initialize BGLIB with our output function for sending messages
    BGLIB_INITIALIZE_NONBLOCK(on_message_send, uartRx, uartRxPeek);

    // Initialise serial communication as non-blocking
    if (appSerialPortInit(((locatorConfig_t*)pLocatorConfig)->COMport, 100) < 0) {
      ENTER_MUTEX(&printfCriticalSection);
      printf("Non-blocking serial port init failure\n");
      fflush(stdout);
      EXIT_MUTEX(&printfCriticalSection);
      exit(EXIT_FAILURE);
    }
  } 
  else {
    ENTER_MUTEX(&printfCriticalSection);
    printf("Invalid configuration\n");
    fflush(stdout);
    EXIT_MUTEX(&printfCriticalSection);
    exit(EXIT_FAILURE);
  }

  // Allocate buffers for IQ samples
  for (int tagID = 0; tagID < MAX_NUM_TAGS; tagID++) {
    ENTER_MUTEX(&iqSamplesCriticalSection[tagID][locatorID]);
    allocate2DFloatBuffer(&(iqSamplesBuffered[tagID][locatorID].i_samples), numSnapshots, numArrayElements);
    allocate2DFloatBuffer(&(iqSamplesBuffered[tagID][locatorID].q_samples), numSnapshots, numArrayElements);
    allocate2DFloatBuffer(&(iqSamplesBuffered[tagID][locatorID].ref_i_samples), 1, ref_period_samples);
    allocate2DFloatBuffer(&(iqSamplesBuffered[tagID][locatorID].ref_q_samples), 1, ref_period_samples);
    // Now buffers can be read/written
    EXIT_MUTEX(&iqSamplesCriticalSection[tagID][locatorID]);
  }

  gecko_cmd_system_reset(0);

  if (((locatorConfig_t*)pLocatorConfig)->IPaddress[0] != '-') {
    // Clear tcp buffer
    uint8_t buf[2];
    for (int i = 0; i < 10000; i++) {
      if (tcpRxPeek() > 0) {
        tcpRx(1, buf);
      }
    }
  }

  ENTER_MUTEX(&printfCriticalSection);
  printf("                                                            \r");
  printf("Starting up...\r\nResetting NCP target... Locator %d \r\n", locatorID);
  fflush(stdout);
  EXIT_MUTEX(&printfCriticalSection);

  gecko_cmd_system_reset(0);
}

static void bgWaitNewEvent(int locatorID, struct gecko_cmd_packet **evt)
{
  // Wait for next Bluetooth event
  *evt = gecko_peek_event();
}

static void bgProcessEvent(int locatorID, bool* appBooted, struct gecko_cmd_packet *evt)
{
  uint32_t slen;

  // If nothing to handle, return
  if (NULL == evt) {
    return;
  }

  // Do not handle any events until system is booted up properly
  if ( (BGLIB_MSG_ID(evt->header) != gecko_evt_system_boot_id)  && (false==(*appBooted)) ) {
    printf("Event: 0x%04x %d\n", BGLIB_MSG_ID(evt->header), locatorID);
    fflush(stdout);
    return;
  }

  // Handle events
  switch (BGLIB_MSG_ID(evt->header)) {
      // This boot event is generated when the system boots up after reset
      case gecko_evt_system_boot_id:
      {
        printf("System booted. Looking for tags... %d\n", locatorID);
        fflush(stdout);
        *appBooted = true;

        // Set scan interval and scan window
        gecko_cmd_le_gap_set_discovery_timing(le_gap_phy_1m, SCAN_INTERVAL, SCAN_WINDOW);
        // Start scanning
        gecko_cmd_le_gap_start_discovery(1, 2);
        // Configure CTE receiver
        gecko_cmd_cte_receiver_configure(0x00);

        uint8_t slot_durations = 1;
        uint8_t cte_count = 1;
        // Start Silabs CTE receiver IQ sampling
        gecko_cmd_cte_receiver_enable_silabs_cte(slot_durations, cte_count, sizeof(antenna_array), antenna_array);
      } break;

      case gecko_evt_le_gap_scan_response_id:
      {
        bd_addr a = evt->data.evt_le_gap_scan_response.address;
        printf("scan response %d %d %d \n", a.addr[0], a.addr[1], a.addr[2]);
        fflush(stdout);
      } break;

      // This event is generated when IQ samples are ready
      case gecko_evt_cte_receiver_silabs_iq_report_id:
      {
        // Get TagID
        ENTER_MUTEX(&tagHandleCriticalSection);
        int32_t tagID = HandleFoundTags(evt->data.evt_cte_receiver_silabs_iq_report.address, locatorID);
        EXIT_MUTEX(&tagHandleCriticalSection); 

        if (tagID == ERROR_CODE_MAX_TAGS_REACHED)
        {
          printf("Maximum number of tags found, cannot track new tag!\n");
          fflush(stdout);
          // Max number of tags reached
          break;
        }

        slen = evt->data.evt_cte_receiver_silabs_iq_report.samples.len;

        // Make sure we do not write the IQ sample buffer while the AoX thread is reading it
        ENTER_MUTEX(&iqSamplesCriticalSection[tagID][locatorID]);

        // Write auxiliary info into the IQ sample buffer
        iqSamplesBuffered[tagID][locatorID].connection = 0;
        iqSamplesBuffered[tagID][locatorID].channel = evt->data.evt_cte_receiver_silabs_iq_report.channel;
        iqSamplesBuffered[tagID][locatorID].rssi = evt->data.evt_cte_receiver_silabs_iq_report.rssi;

        if (evt->data.evt_cte_receiver_silabs_iq_report.samples.len > 0) {
          uint32_t index;
          index = 0;
          // Write reference IQ samples into the IQ sample buffer (sampled on one antenna)
          for (int sample = 0; sample < ref_period_samples; ++sample) {
            iqSamplesBuffered[tagID][locatorID].ref_i_samples[0][sample] = ((int8_t)(uint8_t)(evt->data.evt_cte_receiver_silabs_iq_report.samples.data[index++]))/127.0;
            if (index == slen) break;
            iqSamplesBuffered[tagID][locatorID].ref_q_samples[0][sample] = ((int8_t)(uint8_t)(evt->data.evt_cte_receiver_silabs_iq_report.samples.data[index++]))/127.0;
            if (index == slen) break;
          }

          index = ref_period_samples*2;
          // Write antenna IQ samples into the IQ sample buffer (sampled on all antennas)
          for (int snapshot = 0; snapshot < numSnapshots; ++snapshot) {
            for (int antenna = 0; antenna < numArrayElements; ++antenna) {
              iqSamplesBuffered[tagID][locatorID].i_samples[snapshot][antenna] = ((int8_t)(uint8_t)(evt->data.evt_cte_receiver_silabs_iq_report.samples.data[index++]))/127.0;
              if (index == slen) break;
              iqSamplesBuffered[tagID][locatorID].q_samples[snapshot][antenna] = ((int8_t)(uint8_t)(evt->data.evt_cte_receiver_silabs_iq_report.samples.data[index++]))/127.0;
              if (index == slen) break;
            }
            if (index == slen)
            break;
          }
        }
        // Now the AoX thread can read the IQ sample buffer
        EXIT_MUTEX(&iqSamplesCriticalSection[tagID][locatorID]);
#ifdef WINDOWS
        SEM_SIGNAL(newSamplesAvailable[locatorID][tagID]);
#else
        SEM_SIGNAL(&newSamplesAvailable[locatorID][tagID]);
#endif
      } break;

      default:
        break;
  }
}

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

/* standard library headers */
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>

#include "bg_types.h"
#include "gecko_bglib.h"
#include "bg.h"
#include "aox.h"
#include "uart.h"
#include "serial.h"

/***************************************************************************************************
 * Local Macros and Definitions
 **************************************************************************************************/

BGLIB_DEFINE();

/***************************************************************************************************
 * Static Variable Declarations
 **************************************************************************************************/

// State variables
static bool appBooted = false;
static bool bgInitialized = false;

// UUIDs defined by Bluetooth SIG
static const uint16_t CTE_service     = 0xaabb;

// Antenna switching pattern
static const uint8_t  antenna_array[NUM_ANTENNAS] = SWITCHING_PATTERN;

// Counters
static int32_t IQCount = 0;

/***************************************************************************************************
 * Static Function Declarations
 **************************************************************************************************/

static void bgWaitNewEvent(struct gecko_cmd_packet **evt);
static void bgProcessEvent(struct gecko_cmd_packet *evt);

/***************************************************************************************************
 * Public Function Definitions
 **************************************************************************************************/

void bgInit(void* args)
{
  if (!bgInitialized) {
    printf("BGLIB init...\r\n");

    // Initialize BGLIB with our output function for sending messages
    BGLIB_INITIALIZE_NONBLOCK(on_message_send, uartRx, uartRxPeek);

    // Initialise serial communication as non-blocking
    if (appSerialPortInit(((arguments_t*)args)->argc, ((arguments_t*)args)->argv, 100) < 0) {
      printf("Non-blocking serial port init failure\n");
      exit(EXIT_FAILURE);
    }

    // Flush std output
    fflush(stdout);

    bgInitialized = true;
  }
}

THREAD_RETURN_T bgMain(void* args)
{
  struct gecko_cmd_packet* evt;

  // If not initialized, initialize
  if (!bgInitialized) {
    bgInit(args);
  }

  printf("Starting up...\r\nResetting NCP target...\r\n");

  // Reset NCP to ensure it gets into a defined state
  gecko_cmd_system_reset(0);

  while (eAppCtrl!=eAOX_SHUTDOWN) {
    // Wait for new Bluetooth stack event
    bgWaitNewEvent(&evt);

    // Process Bluetooth event
    bgProcessEvent(evt);
  }

  // let AoX thread shutdown
  CONDITION_MET(&newSamplesAvailable);

  // Reset NCP to ensure it gets into a defined state
  printf("\nResetting NCP target...\n");
  gecko_cmd_system_reset(0);

  THREAD_EXIT;
}

/***************************************************************************************************
 * Static Function Definitions
 **************************************************************************************************/

static void bgWaitNewEvent(struct gecko_cmd_packet **evt)
{
  // Make sure we do not read events while command responses are read
  ENTER_MUTEX(&bgBufferCriticalSection);
  // Wait for next Bluetooth event
  *evt = gecko_wait_event();
  // Now command responses can be read again
  EXIT_MUTEX(&bgBufferCriticalSection);
}

static void bgProcessEvent(struct gecko_cmd_packet *evt)
{
  // If nothing to handle, return
  if (NULL == evt) {
    return;
  }

  // Do not handle any events until system is booted up properly
  if ( (BGLIB_MSG_ID(evt->header) != gecko_evt_system_boot_id)  && (false==appBooted) ) {
    printf("Event: 0x%04x\n", BGLIB_MSG_ID(evt->header));
    return;
  }

  // Handle events
  switch (BGLIB_MSG_ID(evt->header)) {

      // This boot event is generated when the system boots up after reset
      case gecko_evt_system_boot_id:
      {
        appBooted = true;
        printf("System booted. Looking for tags... \r\n");
        fflush(stdout);

        // Set scan interval and scan window
        gecko_cmd_le_gap_set_discovery_timing(le_gap_phy_1m, SCAN_INTERVAL, SCAN_WINDOW);
        // // Start scanning
        gecko_cmd_le_gap_start_discovery(1, 2);
        // // Configure CTE receiver
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
        IQCount++;
        //printf("IQ count: %d\n",IQCount);

        uint32_t slen = evt->data.evt_cte_receiver_silabs_iq_report.samples.len;

        // Make sure we do not write the IQ sample buffer while the AoX thread is reading it
        ENTER_MUTEX(&iqSamplesCriticalSection);

        // Write auxiliary info into the IQ sample buffer
        iqSamplesBuffered.connection = 0;
        iqSamplesBuffered.channel = evt->data.evt_cte_receiver_silabs_iq_report.channel;
        iqSamplesBuffered.rssi = evt->data.evt_cte_receiver_silabs_iq_report.rssi;

        if (evt->data.evt_cte_receiver_silabs_iq_report.samples.len > 0) {
          uint32_t index;
          index = 0;
          // Write reference IQ samples into the IQ sample buffer (sampled on one antenna)
          for (uint32_t sample = 0; sample < ref_period_samples; ++sample) {
            iqSamplesBuffered.ref_i_samples[0][sample] = ((int8_t)(uint8_t)(evt->data.evt_cte_receiver_silabs_iq_report.samples.data[index++]))/127.0;
            if (index == slen) break;
            iqSamplesBuffered.ref_q_samples[0][sample] = ((int8_t)(uint8_t)(evt->data.evt_cte_receiver_silabs_iq_report.samples.data[index++]))/127.0;
            if (index == slen) break;
          }

          index = ref_period_samples*2;
          // Write antenna IQ samples into the IQ sample buffer (sampled on all antennas)
          for (uint32_t snapshot = 0; snapshot < numSnapshots; ++snapshot) {
            for (uint32_t antenna = 0; antenna < numArrayElements; ++antenna) {
              iqSamplesBuffered.i_samples[snapshot][antenna] = ((int8_t)(uint8_t)(evt->data.evt_cte_receiver_silabs_iq_report.samples.data[index++]))/127.0;
              if (index == slen) break;
              iqSamplesBuffered.q_samples[snapshot][antenna] = ((int8_t)(uint8_t)(evt->data.evt_cte_receiver_silabs_iq_report.samples.data[index++]))/127.0;
              if (index == slen) break;
            }
            if (index == slen)
            break;
          }
        }

        // Notify AoX thread about the new samples
        CONDITION_MET(&newSamplesAvailable);
        // Now the AoX thread can read the IQ sample buffer
        EXIT_MUTEX(&iqSamplesCriticalSection);

      } break;
  }
}
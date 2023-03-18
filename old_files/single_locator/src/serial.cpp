/***********************************************************************************************//**
 * @file   serial.c
 * @brief  Module for initializing serial port
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
#include <errno.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>

#include "bg_types.h"
#include "uart.h"
#include "serial.h"

/***************************************************************************************************
 * Local Macros and Definitions
 **************************************************************************************************/

#define USAGE "Usage: %s <serial port> <baud rate> [flow control: 1(on, default) or 0(off)]\n\n"

/***************************************************************************************************
 * Static Variable Declarations
 **************************************************************************************************/

static char* uart_port = NULL;

/***************************************************************************************************
 * Public Function Definitions
 **************************************************************************************************/

/***********************************************************************************************//**
 *  \brief  Function called when a message needs to be written to the serial port.
 *  \param[in] msg_len Length of the message.
 *  \param[in] msg_data Message data, including the header.
 **************************************************************************************************/
void on_message_send(uint32_t msg_len, uint8_t* msg_data)
{
  // Variable for storing function return values
  int32_t ret;

  // Use uartTx to send out data
  ret = uartTx(msg_len, msg_data);
  if (ret < 0) {
    printf("Failed to write to serial port %s, ret: %d, errno: %d\n", uart_port, ret, errno);
    exit(EXIT_FAILURE);
  }
}

/***********************************************************************************************//**
 *  \brief  Serial Port initialisation routine.
 *  \param[in] argc Argument count.
 *  \param[in] argv Buffer contaning Serial Port data.
 *  \return  0 on success, -1 on failure.
 **************************************************************************************************/
int appSerialPortInit(int argc, char* argv[], int32_t timeout)
{
  uint32_t baud_rate = 0;
  uint32_t flowcontrol = 1;

  // Handle the command-line arguments
  baud_rate = DEFAULT_BAUD_RATE;
  uart_port = DEFAULT_UART_PORT;
  switch (argc) {
    case 4:
      flowcontrol = atoi(argv[3]);
    // Falls through on purpose
    case 3:
      baud_rate = atoi(argv[2]);
    // Falls through on purpose
    case 2:
      uart_port = argv[1];
      printf("uart_port: %s\n", uart_port);
    // Falls through on purpose
    default:
      break;
  }

  // If something is wrong, print usage info
  if (!uart_port || !baud_rate || (flowcontrol > 1)) {
    printf(USAGE, argv[0]);
    exit(EXIT_FAILURE);
  }

  // Initialise the serial port with given parameters
  return uartOpen((int8_t*)uart_port, baud_rate, flowcontrol, timeout);
}

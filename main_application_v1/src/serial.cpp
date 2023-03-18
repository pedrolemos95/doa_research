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

/***************************************************************************************************
 * Static Variable Declarations
 **************************************************************************************************/

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
    printf("Failed to write to target device, ret: %d, errno: %d\n", ret, errno);
    exit(EXIT_FAILURE);
  }
}

/***********************************************************************************************//**
 *  \brief  Serial Port initialisation routine.
 *  \param[in] uart_port String contaning Serial Port number.
 *  \return  0 on success, -1 on failure.
 **************************************************************************************************/
int appSerialPortInit(char* uart_port, int32_t timeout)
{
  uint32_t baud_rate = DEFAULT_BAUD_RATE;
  uint32_t flowcontrol = 1;

  // Initialise the serial port with given parameters
  return uartOpen((int8_t*)uart_port, baud_rate, flowcontrol, timeout);
}

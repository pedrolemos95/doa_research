/***********************************************************************************************//**
 * @file   bg.h
 * @brief  Bluetooth application header file
 ***************************************************************************************************
 * # License
 * <b>Copyright 2019 Silicon Laboratories Inc. www.silabs.com</b>
 ***************************************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this software is governed 
 * by the terms of Silicon Labs Master Software License Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This software is distributed to
 * you in Source Code format and is governed by the sections of the MSLA applicable to Source Code.
 **************************************************************************************************/

#ifndef BG_H
#define BG_H

#include "common.h"
#include "aox.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************//**
 * \defgroup app Application Code
 * \brief Sample Application Implementation
 **************************************************************************************************/

/***********************************************************************************************//**
 * @addtogroup Application
 * @{
 **************************************************************************************************/

/***********************************************************************************************//**
 * @addtogroup app
 * @{
 **************************************************************************************************/

#define SYNC_SKIP                     1    //one packet can be skipped
#define SYNC_TIMEOUT                  100  //1000ms

#define CTE_SLOT_DURATION             1    //1us
#define CTE_COUNT                     1    //1 per advertisement interval

#define SCAN_INTERVAL                 16   //10ms
#define SCAN_WINDOW                   16   //10ms
#define SCAN_PASSIVE                  0
#define SCAN_ACTIVE                   1

#if (ARRAY_TYPE == ARRAY_TYPE_4x4_URA)
#define NUM_ANTENNAS       16
#define SWITCHING_PATTERN  { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}
#elif (ARRAY_TYPE == ARRAY_TYPE_3x3_URA)
#define NUM_ANTENNAS       9
#define SWITCHING_PATTERN  { 1, 2, 3, 5, 6, 7, 9, 10, 11}
#elif (ARRAY_TYPE == ARRAY_TYPE_1x4_ULA)
#define NUM_ANTENNAS       4
#define SWITCHING_PATTERN  { 0, 1, 2, 3}
#endif 
 
/***************************************************************************************************
 * Type Definitions
 **************************************************************************************************/
 
typedef enum {
  scanning,
  opening,
  discoverServices,
  discoverCharacteristics,
  enableNotifications,
  enableCTE,
  running
} ConnState;

/***************************************************************************************************
 * Function Declarations
 **************************************************************************************************/

void bgInit(void* args);

THREAD_RETURN_T bgMain(void* args);

/** @} (end addtogroup app) */
/** @} (end addtogroup Application) */

#ifdef __cplusplus
};
#endif

#endif /* BG_H */

/***********************************************************************************************//**
 * @file   aox.h
 * @brief  AoX header file
 ***************************************************************************************************
 * # License
 * <b>Copyright 2019 Silicon Laboratories Inc. www.silabs.com</b>
 ***************************************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this software is governed 
 * by the terms of Silicon Labs Master Software License Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This software is distributed to
 * you in Source Code format and is governed by the sections of the MSLA applicable to Source Code.
 **************************************************************************************************/

#ifndef LOC_H
#define LOC_H

#include "common.h"
#include "sl_rtl_clib_api.h"

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

#define TAG_PER_ADV_INTERVAL (100)   // 100ms

enum utilityFunctions {
  UTIL_X,
  UTIL_Y,
  UTIL_Z,
  NUM_UTIL_FUNCTIONS
};

/***************************************************************************************************
 * Type Definitions
 **************************************************************************************************/

typedef struct {
  int                             locatorConfigID;      //original locatorID
  struct sl_rtl_loc_locator_item  locatorItem;
  uint32_t                        locatorItemID;        //assigned by RTL lib
} locator_t;

/***************************************************************************************************
 * Public variables
 **************************************************************************************************/

/***************************************************************************************************
 * Function Declarations
 **************************************************************************************************/

THREAD_RETURN_T locMain(void* pLocatorConfigTable);

/** @} (end addtogroup app) */
/** @} (end addtogroup Application) */

#ifdef __cplusplus
};
#endif

#endif /* LOC_H */

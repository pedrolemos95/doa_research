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

#ifndef AOX_H
#define AOX_H

#include "common.h"

extern "C" {
#include "sl_rtl_clib_api.h"
}

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

#define ARRAY_TYPE_4x4_URA (0)
#define ARRAY_TYPE_3x3_URA (1)
#define ARRAY_TYPE_1x4_ULA (2)
#define ARRAY_TYPE         ARRAY_TYPE_4x4_URA

#define AOX_MODE           SL_RTL_AOX_MODE_REAL_TIME_BASIC

#if (ARRAY_TYPE == ARRAY_TYPE_4x4_URA)
#define AOX_ARRAY_TYPE     SL_RTL_AOX_ARRAY_TYPE_4x4_URA
#define numSnapshots       (4)
#define numArrayElements   (4*4)
#define ref_period_samples (7)
#elif (ARRAY_TYPE == ARRAY_TYPE_3x3_URA)
#define AOX_ARRAY_TYPE     SL_RTL_AOX_ARRAY_TYPE_3x3_URA
#define numSnapshots       (4)
#define numArrayElements   (3*3)
#define ref_period_samples (7)
#elif (ARRAY_TYPE == ARRAY_TYPE_1x4_ULA)
#define AOX_ARRAY_TYPE     SL_RTL_AOX_ARRAY_TYPE_1x4_ULA
#define numSnapshots       (18)
#define numArrayElements   (1*4)
#define ref_period_samples (7)
#endif

#define TAG_TX_POWER       (-45.0)        //-45dBm at 1m distance

/***************************************************************************************************
 * Type Definitions
 **************************************************************************************************/

typedef struct IQsamples {
  float** ref_i_samples;
  float** ref_q_samples;
  float** i_samples;
  float** q_samples;
  uint8_t connection;
  uint8_t channel;
  int16_t rssi;
} iqSamples_t;

/***************************************************************************************************
 * Public variables
 **************************************************************************************************/

extern iqSamples_t iqSamplesBuffered, iqSamplesActive;

/***************************************************************************************************
 * Function Declarations
 **************************************************************************************************/

void aoxInit(void* args);

THREAD_RETURN_T aoxMain(void* args);

/** @} (end addtogroup app) */
/** @} (end addtogroup Application) */

#ifdef __cplusplus
};
#endif

#endif /* AOX_H */

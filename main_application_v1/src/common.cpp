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

/***************************************************************************************************
 * Public Function Definitions
 **************************************************************************************************/

int allocate2DFloatBuffer(float*** buf, int rows, int cols)
{
  *buf = malloc(sizeof(float*)*rows);
  if (*buf == NULL) {
    return 0;
  }
  
  for (int i = 0; i < rows; i++) {
    (*buf)[i] = malloc(sizeof(float)*cols);
    if ((*buf)[i] == NULL) {
      return 0;
    }
  }

  return 1;
}
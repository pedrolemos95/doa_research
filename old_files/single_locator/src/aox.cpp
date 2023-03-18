/***********************************************************************************************//**
 * @file   aox.c
 * @brief  Module responsible for processing IQ samples and calculate angle estimation from them
 *         using the AoX library
 ***************************************************************************************************
 * # License
 * <b>Copyright 2019 Silicon Laboratories Inc. www.silabs.com</b>
 ***************************************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this software is governed
 * by the terms of Silicon Labs Master Software License Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This software is distributed to
 * you in Source Code format and is governed by the sections of the MSLA applicable to Source Code.
 **************************************************************************************************/

// standard library headers
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>

#include "bg_types.h"
#include "aox.h"
#include "doa.h"

extern "C" {
#include "sl_rtl_clib_api.h"
}

/***************************************************************************************************
 * Public Variable Declarations
 **************************************************************************************************/

// IQ sample buffers
iqSamples_t iqSamplesBuffered, iqSamplesActive;

MUTEX_T     iqSamplesCriticalSection;
MUTEX_T     bgBufferCriticalSection;
MUTEX_T     printfCriticalSection;
#ifndef WINDOWS
MUTEX_T     newSamplesAvailableMutex;
#endif

//Conditional variable
CONDITION_T newSamplesAvailable;

// Application state
eAOX_APP_CTRL eAppCtrl;

/***************************************************************************************************
 * Static Variable Declarations
 **************************************************************************************************/

// State variables
static uint8_t  aox_initialized = 0;
static sl_rtl_aox_libitem  libitem;
static sl_rtl_util_libitem util_libitem;

/***************************************************************************************************
 * Static Function Declarations
 **************************************************************************************************/

static void aoxWaitNewSamples(iqSamples_t* samples);
static enum sl_rtl_error_code aoxProcessSamples(iqSamples_t* samples, float* azimuth, float* elevation, uint32_t *qaResult);
static float calcFrequencyFromChannel(uint8_t channel);
static uint32_t allocate2DFloatBuffer(float*** buf, int rows, int cols);

static void init_doa_estimator(aoa_estimator& doa_estimator, float elements_distance);
static void doaProcessSamples(aoa_estimator& doa_estimator, iqSamples_t* samples);

/***************************************************************************************************
 * Public Function Definitions
 **************************************************************************************************/

void aoxInit(void* args)
{
  if (!aox_initialized) {
      // Make sure IQ sample buffers are not read/written while buffers are not initialized
      ENTER_MUTEX(&iqSamplesCriticalSection);

      printf("AoX library init...\r\n");

      // Initialize AoX library
      sl_rtl_aox_init(&libitem);
      // Set the number of snapshots - how many times the antennas are scanned during one measurement
      sl_rtl_aox_set_num_snapshots(&libitem, numSnapshots);
      // Set the antenna array type
      sl_rtl_aox_set_array_type(&libitem, AOX_ARRAY_TYPE);
      // Select mode (high speed/high accuracy/etc.)
      sl_rtl_aox_set_mode(&libitem, AOX_MODE);
      // Enable IQ sample quality analysis processing
      sl_rtl_aox_iq_sample_qa_configure(&libitem);
      // Create AoX estimator
      sl_rtl_aox_create_estimator(&libitem);

      // Initialize an util item
      sl_rtl_util_init(&util_libitem);
      sl_rtl_util_set_parameter(&util_libitem, SL_RTL_UTIL_PARAMETER_AMOUNT_OF_FILTERING, 0.6f);

      // // Allocate buffers for IQ samples
      allocate2DFloatBuffer(&(iqSamplesBuffered.i_samples), numSnapshots, numArrayElements);
      allocate2DFloatBuffer(&(iqSamplesBuffered.q_samples), numSnapshots, numArrayElements);
      allocate2DFloatBuffer(&(iqSamplesActive.i_samples), numSnapshots, numArrayElements);
      allocate2DFloatBuffer(&(iqSamplesActive.q_samples), numSnapshots, numArrayElements);
      allocate2DFloatBuffer(&(iqSamplesBuffered.ref_i_samples), 1, ref_period_samples);
      allocate2DFloatBuffer(&(iqSamplesBuffered.ref_q_samples), 1, ref_period_samples);
      allocate2DFloatBuffer(&(iqSamplesActive.ref_i_samples), 1, ref_period_samples);
      allocate2DFloatBuffer(&(iqSamplesActive.ref_q_samples), 1, ref_period_samples);

      aox_initialized = 1;

      // Now buffers can be read/written
      EXIT_MUTEX(&iqSamplesCriticalSection);

  }
}

THREAD_RETURN_T aoxMain(void* args)
{
  float azimuth;
  float elevation;
  int32_t SampleCount = 0;
  uint32_t qualityResult;
  char *iqSampleQaString;

  // If not initialized, initialize
  if (!aox_initialized) {
    aoxInit(args);
  }

  // Define and initialize custom DoA estimator
  auto doa_estimator = aoa_estimator();
  float elements_distance = 0.04; // Adjacent antenna array elements distance in meters
  init_doa_estimator(doa_estimator, elements_distance);

  while (eAppCtrl!=eAOX_SHUTDOWN) {
    // Wait for new IQ samples
    aoxWaitNewSamples(&iqSamplesActive);
    SampleCount++;

    // Process new IQ Samples using the custom DoA estimator
    doaProcessSamples(doa_estimator, &iqSamplesActive);

    // Process new IQ samples and calculate Angle of Arrival (azimuth, elevation)
    enum sl_rtl_error_code ret = aoxProcessSamples(&iqSamplesActive, &azimuth, &elevation, &qualityResult);

    // sl_rtl_aox_process will return SL_RTL_ERROR_ESTIMATION_IN_PROGRESS until it has received enough packets for angle estimation
    if (ret == SL_RTL_ERROR_SUCCESS) {

      float distance;

      // Calculate distance from RSSI, and calculate a rough position estimation
      sl_rtl_util_rssi2distance(TAG_TX_POWER, iqSamplesActive.rssi/1.0, &distance);
      sl_rtl_util_filter(&util_libitem, distance, &distance);

      // Print out results
      // printf("azimuth: %6.1f  \televation: %6.1f  \trssi: %6.0f \tch: %2d \tSample Count: %5d \n",
      //       azimuth, elevation, iqSamplesActive.rssi/1.0, iqSamplesActive.channel, SampleCount);

      fflush(stdout);
    }
  }

  THREAD_EXIT;
}


/***************************************************************************************************
 * Custom Static Function Definitions
 **************************************************************************************************/

static void init_doa_estimator(aoa_estimator& doa_estimator, float elements_distance){
  doa_estimator.initDoAEstimator(elements_distance, 0);
  doa_estimator.initSelectionMatrices(0);
};

static void doaProcessSamples(aoa_estimator& doa_estimator, iqSamples_t* samples){

  // Estimate phase_rotation
  float phase_rotation;
  phase_rotation = doa_estimator.estimate_phase_rotation(samples->ref_i_samples[0], samples->ref_q_samples[0], ref_period_samples);

  // Load iq samples into the estimator
  doa_estimator.load_x(samples->i_samples, samples->q_samples, NUM_ANTENNAS, IQLENGTH, 0);

  // Compensate phase rotation from IQ samples
  doa_estimator.compensateRotation(phase_rotation, 0);

  // Estimate covariance matrix
  doa_estimator.estimateRxx(0);

  doa_estimator.processESPRIT(calcFrequencyFromChannel(samples->channel) , 0);

  float azimuth_esp, elevation_esp;

  doa_estimator.getProcessed(1, &azimuth_esp, &elevation_esp);

  // Change array angle reference to match Silabs PCB drawing
  // azimuth_esp = atan2(sin(PI_CTE*(180-azimuth_esp)/180), cos(PI_CTE*(180-azimuth_esp)/180));

  // azimuth_esp *= 180/PI_CTE;

  printf("azimuth: %f, elevation: %f\n", azimuth_esp, elevation_esp);

};

/***************************************************************************************************
 * Static Function Definitions
 **************************************************************************************************/

static void aoxWaitNewSamples(iqSamples_t* samples)
{
  // Make sure IQ sample buffer is not read while BG thread is writing it
  ENTER_MUTEX(&iqSamplesCriticalSection);
  // Wait until new samples are available (signalled by BG thread)
  CONDITION_WAIT(&newSamplesAvailable, &iqSamplesCriticalSection);

  // Copy auxiliary info from buffer into working copy
  samples->rssi = iqSamplesBuffered.rssi;
  samples->channel = iqSamplesBuffered.channel;
  samples->connection = iqSamplesBuffered.connection;

  // Copy reference IQ samples from buffer into working copy
  for (uint32_t sample = 0; sample < ref_period_samples; ++sample) {
    samples->ref_q_samples[0][sample] = iqSamplesBuffered.ref_q_samples[0][sample];
    samples->ref_i_samples[0][sample] = iqSamplesBuffered.ref_i_samples[0][sample];
  }

  // Copy IQ samples from buffer into working copy
  for (uint32_t snapshot = 0; snapshot < numSnapshots; ++snapshot) {
    for (uint32_t antenna = 0; antenna < numArrayElements; ++antenna) {
      samples->q_samples[snapshot][antenna] = iqSamplesBuffered.q_samples[snapshot][antenna];
      samples->i_samples[snapshot][antenna] = iqSamplesBuffered.i_samples[snapshot][antenna];
    }
  }
  // Now IQ sample buffer can be written by BG thread again
  EXIT_MUTEX(&iqSamplesCriticalSection);
}

static enum sl_rtl_error_code aoxProcessSamples(iqSamples_t *samples, float *azimuth, float *elevation, uint32_t *qaResult)
{
  float phase_rotation;

  // Calculate phase rotation from reference IQ samples
  sl_rtl_aox_calculate_iq_sample_phase_rotation(&libitem, 2.0f, samples->ref_i_samples[0], samples->ref_q_samples[0], ref_period_samples, &phase_rotation);

  // Provide calculated phase rotation to the estimator
  sl_rtl_aox_set_iq_sample_phase_rotation(&libitem, phase_rotation);

  // Estimate Angle of Arrival / Angle of Departure from IQ samples
  enum sl_rtl_error_code ret = sl_rtl_aox_process(&libitem, samples->i_samples, samples->q_samples, calcFrequencyFromChannel(samples->channel), azimuth, elevation);

  // fetch the quality results
  *qaResult = sl_rtl_aox_iq_sample_qa_get_results(&libitem);

  return ret;
}

static float calcFrequencyFromChannel(uint8_t channel)
{
  static const uint8_t logical_to_physical_channel[40] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
                                                          13, 14, 15, 16, 17, 18, 19, 20, 21,
                                                          22, 23, 24, 25, 26, 27, 28, 29, 30,
                                                          31, 32, 33, 34, 35, 36, 37, 38,
                                                          0, 12, 39};

  // return the center frequency of the given channel
  return 2402000000 + 2000000 * logical_to_physical_channel[channel];
}

static uint32_t allocate2DFloatBuffer(float*** buf, int rows, int cols)
{
  *buf = malloc(sizeof(float*)*rows);
  if (*buf == NULL) {
    return 0;
  }

  for (uint32_t i = 0; i < rows; i++) {
    (*buf)[i] = malloc(sizeof(float)*cols);
    if ((*buf)[i] == NULL) {
      return 0;
    }
  }

  return 1;
}

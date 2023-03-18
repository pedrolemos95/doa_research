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

#include "common.h"
#include "bg_types.h"
#include "aox.h"
#include "bg.h"
#include "doa.h"
extern "C" {
#include "sl_rtl_clib_api.h"
}

#define EXPORT_IQ_DATA

/***************************************************************************************************
 * Public Variable Declarations
 **************************************************************************************************/

// IQ sample buffer
iqSamples_t iqSamplesActive[MAX_NUM_LOCATORS][MAX_NUM_TAGS];
locatorMeasurement_t locatorMeasurements[MAX_NUM_TAGS][MAX_NUM_LOCATORS];

/***************************************************************************************************
 * Static Variable Declarations
 **************************************************************************************************/
MUTEX_T  iqSamplesCriticalSection[MAX_NUM_TAGS][MAX_NUM_LOCATORS];
MUTEX_T  printfCriticalSection;
MUTEX_T  bgBufferCriticalSection;
MUTEX_T  tagHandleCriticalSection;
SEM_T         newMeasurementAvailable[MAX_NUM_TAGS][MAX_NUM_LOCATORS];
SEM_T         newSamplesAvailable[MAX_NUM_LOCATORS][MAX_NUM_TAGS];
eAOX_APP_CTRL eAppCtrl;

/***************************************************************************************************
 * Static Function Declarations
 **************************************************************************************************/

static void aoxInit(int locatorID, uint32_t tagID, void* pLocatorConfig, sl_rtl_aox_libitem* plibitem, sl_rtl_util_libitem* pUtilLibitem);
static int aoxWaitNewSamples(uint32_t tagID, int locatorID, iqSamples_t samples[MAX_NUM_TAGS]);
static void aoxProcessSamples(int locatorID, iqSamples_t* samples, uint32_t tagID, sl_rtl_aox_libitem* plibitem, sl_rtl_util_libitem* pUtilLibitem);
static float calcFrequencyFromChannel(uint8_t channel);

/* Custom DoA estimator initializer and process function */
static void init_doa_estimator(aoa_estimator& doa_estimator, float elements_distance);
static void doaProcessSamples(aoa_estimator& doa_estimator, iqSamples_t* samples);

#ifdef EXPORT_IQ_DATA
static void writeIqDataToFile(int locatorID, uint32_t tagID, iqSamples_t samples);
#endif



/***************************************************************************************************
 * Public Function Definitions
 **************************************************************************************************/

THREAD_RETURN_T aoxMain(void* pLocatorConfig)
{
  int   locatorID = ((locatorConfig_t*)pLocatorConfig)->locatorID;
  uint32_t tagID = ((locatorConfig_t*)pLocatorConfig)->tagID;
  sl_rtl_aox_libitem  aoxlibitem;
  sl_rtl_util_libitem utillibitem;

  // Initialize AoX
  aoxInit(locatorID, tagID, pLocatorConfig, &aoxlibitem, &utillibitem);

  // Define and initialize custom DoA estimator
  auto doa_estimator = aoa_estimator();
  float elements_distance = 0.32;
  init_doa_estimator(doa_estimator, elements_distance);

  while (eAppCtrl!=eAOX_SHUTDOWN) {
    // Wait for new IQ samples
    if(aoxWaitNewSamples(tagID, locatorID, iqSamplesActive[locatorID]) == -1) {
      continue;
    }

    if (eAppCtrl == eAOX_SHUTDOWN) {
      break;
    }

    // Process new IQ samples
    aoxProcessSamples(locatorID, &iqSamplesActive[locatorID][tagID], tagID, &aoxlibitem, &utillibitem);

    // Process IQ samples with custom function
    doaProcessSamples(doa_estimator, &iqSamplesActive[locatorID][tagID]);

  }

  THREAD_EXIT;
}

/***************************************************************************************************
 * Static Function Definitions
 **************************************************************************************************/

static void init_doa_estimator(aoa_estimator& doa_estimator, float elements_distance){
  doa_estimator.initDoAEstimator(elements_distance, 0);
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

  doa_estimator.processMUSIC(360, 90, 0);

  float azimuth_esp, elevation_esp;

  doa_estimator.getProcessed(1, &azimuth_esp, &elevation_esp);

  // Change array angle reference to match Silabs PCB drawing
  azimuth_esp = atan2(sin(PI_CTE*(180-azimuth_esp)/180), cos(PI_CTE*(180-azimuth_esp)/180));

  azimuth_esp *= 180/PI_CTE;

  printf("azimuth: %f, elevation: %f\n", azimuth_esp, elevation_esp);

};


bool firstAoxInit = true;
static void aoxInit(int locatorID, uint32_t tagID, void* pLocatorConfig, sl_rtl_aox_libitem* plibitem, sl_rtl_util_libitem* pUtilLibitem)
{
  if (firstAoxInit) {
    printLog("                                                            \rAoX library init...");
    firstAoxInit = false;
  } else {
    printLog(".");
  }

  float az_min = ((locatorConfig_t*)pLocatorConfig)->az_constr_min;
  float az_max = ((locatorConfig_t*)pLocatorConfig)->az_constr_max;

  // Initialize AoX library
  sl_rtl_aox_init(plibitem);
  // Set constraint
  sl_rtl_aox_add_constraint(plibitem, SL_RTL_AOX_CONSTRAINT_TYPE_AZIMUTH, az_min, az_max);
  // Set the number of snapshots - how many times the antennas are scanned during one measurement
  sl_rtl_aox_set_num_snapshots(plibitem, numSnapshots);
  // Set the antenna array type
  sl_rtl_aox_set_array_type(plibitem, AOX_ARRAY_TYPE);
  // Select mode (high speed/high accuracy/etc.)
  sl_rtl_aox_set_mode(plibitem, AOX_MODE);
  // Create Angle-of-Arrival estimator
  sl_rtl_aox_create_estimator(plibitem);

  // Initialize util functions
  sl_rtl_util_init(pUtilLibitem);
  // Set RSSI filtering parameter
  sl_rtl_util_set_parameter(pUtilLibitem, SL_RTL_UTIL_PARAMETER_AMOUNT_OF_FILTERING, 1.0f);

  // Allocate buffers for IQ samples
  ENTER_MUTEX(&iqSamplesCriticalSection[tagID][locatorID]);
  allocate2DFloatBuffer(&(iqSamplesActive[locatorID][tagID].i_samples), numSnapshots, numArrayElements);
  allocate2DFloatBuffer(&(iqSamplesActive[locatorID][tagID].q_samples), numSnapshots, numArrayElements);
  allocate2DFloatBuffer(&(iqSamplesActive[locatorID][tagID].ref_i_samples), 1, ref_period_samples);
  allocate2DFloatBuffer(&(iqSamplesActive[locatorID][tagID].ref_q_samples), 1, ref_period_samples);
  EXIT_MUTEX(&iqSamplesCriticalSection[tagID][locatorID]);

  // Initialize sequence number
  iqSamplesActive[locatorID][tagID].sequence = 0;
}

static int aoxWaitNewSamples(uint32_t tagID, int locatorID, iqSamples_t samples[MAX_NUM_TAGS])
{
  // Wait until samples are available from any tag
#ifdef WINDOWS
  SEM_WAIT_T dwEvent = SEM_WAIT_S(newSamplesAvailable[locatorID][tagID]);
#else
  SEM_WAIT_S(&newSamplesAvailable[locatorID][tagID]);
#endif

  if (eAppCtrl == eAOX_SHUTDOWN) {
    return -1;
  }

#ifdef WINDOWS
  if (dwEvent == WAIT_FAILED) {
    return -1;
  }

  if (SEM_WAIT_SNB(newMeasurementAvailable[tagID][locatorID])== WAIT_OBJECT_0) {
    // Current tag id already in signaled state
    return -1;
  }

#endif

  ENTER_MUTEX(&iqSamplesCriticalSection[tagID][locatorID]);

  // Copy auxiliary info from buffer into working copy
  samples[tagID].rssi = iqSamplesBuffered[tagID][locatorID].rssi;
  samples[tagID].channel = iqSamplesBuffered[tagID][locatorID].channel;
  samples[tagID].connection = iqSamplesBuffered[tagID][locatorID].connection;

  // Copy reference IQ samples from buffer into working copy
  for (uint32_t sample = 0; sample < ref_period_samples; ++sample) {
    samples[tagID].ref_q_samples[0][sample] = iqSamplesBuffered[tagID][locatorID].ref_q_samples[0][sample];
    samples[tagID].ref_i_samples[0][sample] = iqSamplesBuffered[tagID][locatorID].ref_i_samples[0][sample];
  }

  // Copy IQ samples from buffer into working copy
  for (uint32_t snapshot = 0; snapshot < numSnapshots; ++snapshot) {
    for (uint32_t antenna = 0; antenna < numArrayElements; ++antenna) {
      samples[tagID].q_samples[snapshot][antenna] = iqSamplesBuffered[tagID][locatorID].q_samples[snapshot][antenna];
      samples[tagID].i_samples[snapshot][antenna] = iqSamplesBuffered[tagID][locatorID].i_samples[snapshot][antenna];
    }
  }

  samples[tagID].sequence++;

#ifdef EXPORT_IQ_DATA
  writeIqDataToFile(locatorID, tagID, samples[tagID]);
#endif

  // Now IQ sample buffer can be written by BG thread again
  EXIT_MUTEX(&iqSamplesCriticalSection[tagID][locatorID]);

  return tagID;
}

static void aoxProcessSamples(int locatorID, iqSamples_t* samples, uint32_t tagID, sl_rtl_aox_libitem* plibitem, sl_rtl_util_libitem* pUtilLibitem)
{
  static int SampleCount = 0;

  float phase_rotation;
  float filtered_rssi;
  float azimuth;
  float elevation;
  float distance;

  SampleCount++;

  // Calculate phase rotation from reference IQ samples
  sl_rtl_aox_calculate_iq_sample_phase_rotation(plibitem, 2.0f, samples->ref_i_samples[0], samples->ref_q_samples[0], ref_period_samples, &phase_rotation);

  // Provide calculated phase rotation to the estimator
  sl_rtl_aox_set_iq_sample_phase_rotation(plibitem, phase_rotation);

  // Estimate Angle of Arrival from IQ samples
  enum sl_rtl_error_code result = sl_rtl_aox_process(plibitem, samples->i_samples, samples->q_samples, calcFrequencyFromChannel(samples->channel), &azimuth, &elevation);

  ENTER_MUTEX(&printfCriticalSection);
  // printf("seq: %d - az: %f, el: %f, phase_rot: %f\n", samples->sequence, azimuth, elevation, phase_rotation);
  EXIT_MUTEX(&printfCriticalSection);

  // Filter RSSI value
  sl_rtl_util_filter(pUtilLibitem, (samples->rssi)/1.0, &filtered_rssi);

  // Estimate distance from filtered RSSI value
  sl_rtl_util_rssi2distance(TAG_TX_POWER, filtered_rssi, &distance);

  // Save the results for this locator (the Loc thread will use these results to estimate position)
  locatorMeasurements[tagID][locatorID].azimuth = azimuth;
  locatorMeasurements[tagID][locatorID].elevation = elevation;
  locatorMeasurements[tagID][locatorID].distance = distance;

  // Save additional parameters
  locatorMeasurements[tagID][locatorID].locatorID = locatorID;
  locatorMeasurements[tagID][locatorID].sampleCount = SampleCount;
  locatorMeasurements[tagID][locatorID].connection = samples->connection;
  locatorMeasurements[tagID][locatorID].channel = samples->channel;
  locatorMeasurements[tagID][locatorID].rssi = samples -> rssi;
  locatorMeasurements[tagID][locatorID].iq_sequence = samples->sequence;
  locatorMeasurements[tagID][locatorID].valid = 1;

#ifdef WINDOWS
  SEM_RESET(newSamplesAvailable[locatorID][tagID]);
#else
  SEM_RESET(&newSamplesAvailable[locatorID][tagID]);
#endif

  //  printLog("locator #%d:  \tazimuth: %6.1f  \televation: %6.1f  \trssi: %6.0f  \tch: %d \tSample Count: %d\r\n", locatorID, azimuth, elevation, samples->rssi/1.0, samples->channel, SampleCount);

  // Signal to the Loc thread that the angle estimation is ready for this locator
  if (result == SL_RTL_ERROR_SUCCESS) {
#ifdef WINDOWS
    SEM_SIGNAL(newMeasurementAvailable[tagID][locatorID]);
#else
    SEM_SIGNAL(&newMeasurementAvailable[tagID][locatorID]);
#endif
  }
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


/* Debug section ************************************************************************************/

#ifdef EXPORT_IQ_DATA
static void writeIqDataToFile(int locatorID, uint32_t tagID, iqSamples_t samples)
{
  static FILE* iqDataF[MAX_NUM_LOCATORS][MAX_NUM_TAGS] = {NULL};
  if (iqDataF[locatorID][tagID] == NULL)
  {
    char iq_dump_path[128];
#ifdef WINDOWS
    mkdir("logs");
#else
    mkdir("logs", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif
    sprintf(iq_dump_path, "logs/log_iq_data_loc%d_tag%d.out", locatorID, tagID);
    iqDataF[locatorID][tagID] = fopen(iq_dump_path, "w+t");
  }

  fprintf(iqDataF[locatorID][tagID], "SEQ: %d, %d, CH: %d, RSSI: %d, REF: %d, WP: 0\n", samples.sequence, numSnapshots, samples.channel, samples.rssi, ref_period_samples);

  // Dump first the reference period one IQ sample per line
  for (uint32_t j  = 0; j < ref_period_samples; j++) {
    float iSample = samples.ref_i_samples[0][j];
    float qSample = samples.ref_q_samples[0][j];
    fprintf(iqDataF[locatorID][tagID], "%f,%f\n", iSample, qSample);
  }

  for (uint32_t ai = 0; ai < numSnapshots; ai++) {
    for (uint32_t aj = 0; aj < numArrayElements; aj++) {
      float iSample = samples.i_samples[ai][aj];
      float qSample = samples.q_samples[ai][aj];
      fprintf(iqDataF[locatorID][tagID], "%f,", iSample);
      fprintf(iqDataF[locatorID][tagID], "%f", qSample);
      if (aj < numArrayElements - 1) fprintf(iqDataF[locatorID][tagID], ", ");
    }
    fprintf(iqDataF[locatorID][tagID], "\n");
  }

  fflush(iqDataF[locatorID][tagID]);
}
#endif

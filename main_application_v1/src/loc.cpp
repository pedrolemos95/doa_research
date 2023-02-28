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
#include "bg.h"
#include "aox.h"
#include "loc.h"
#include "sl_rtl_clib_api.h"

// #define DEBUG_ANGLES

//#define EXPORT_ANG_DATA
//#define EXPORT_LOC_DATA

/***************************************************************************************************
 * Public Variable Declarations
 **************************************************************************************************/

/***************************************************************************************************
 * Static Variable Declarations
 **************************************************************************************************/

static locator_t* locatorTable[MAX_NUM_LOCATORS];
static int numLocators = 0;

/***************************************************************************************************
 * Static Function Declarations
 **************************************************************************************************/

static void locInit(locatorConfig_t** pLocatorConfigTable, sl_rtl_loc_libitem* libitem);
static int locWaitNewMeasurements(sl_rtl_loc_libitem* plibitem, uint32_t tagID);
static void locProcessMeasurements(sl_rtl_loc_libitem* plibitem, uint32_t tagID, sl_rtl_util_libitem* utilLibitems);
static void dumpAngleData(sl_rtl_loc_libitem* plibitem, uint32_t tagID);

#ifdef EXPORT_LOC_DATA
static void writeLocDataToFile(uint32_t tagID, float x, float y, float z);
static void writeConfigDataToFile();
#endif

/***************************************************************************************************
 * Public Function Definitions
 **************************************************************************************************/

THREAD_RETURN_T locMain(void* pLocatorConfigTable)
{
  sl_rtl_loc_libitem libitem[MAX_NUM_TAGS];
  sl_rtl_util_libitem utilLibitems[MAX_NUM_TAGS][3];

  // Initialize Locationing
  for (int i = 0; i < MAX_NUM_TAGS; i++) {
    locInit((locatorConfig_t**)pLocatorConfigTable, &libitem[i]);

    // Initialize util functions
    sl_rtl_util_init(&utilLibitems[i][UTIL_X]);
    sl_rtl_util_init(&utilLibitems[i][UTIL_Y]);
    sl_rtl_util_init(&utilLibitems[i][UTIL_Z]);

    // Set position filtering parameter for x, y, z coordinates
    sl_rtl_util_set_parameter(&utilLibitems[i][UTIL_X], SL_RTL_UTIL_PARAMETER_AMOUNT_OF_FILTERING, 0.1f);
    sl_rtl_util_set_parameter(&utilLibitems[i][UTIL_Y], SL_RTL_UTIL_PARAMETER_AMOUNT_OF_FILTERING, 0.1f);
    sl_rtl_util_set_parameter(&utilLibitems[i][UTIL_Z], SL_RTL_UTIL_PARAMETER_AMOUNT_OF_FILTERING, 0.1f);
  }

  // Wait for threads to complete initialization
  while (eAppCtrl!=eAOX_SHUTDOWN) {
    // Wait for all locator to provide angle estimation
    for (int tagID = 0; tagID < MAX_NUM_TAGS; tagID++) {
      if (locWaitNewMeasurements(&libitem[tagID], tagID) != 0) {
        continue;
      }

      if (eAppCtrl == eAOX_SHUTDOWN) {
        break;
      }

      // Calculate position from angles
      locProcessMeasurements(&libitem[tagID], tagID, utilLibitems[tagID]);
    }
  }

  THREAD_EXIT;
}

/***************************************************************************************************
 * Static Function Definitions
 **************************************************************************************************/
bool firstLocInit = true;
static void locInit(locatorConfig_t** pLocatorConfigTable, sl_rtl_loc_libitem* plibitem)
{
  if (firstLocInit) {
    printLog("                                                            \rLocating library init...");
    firstLocInit = false;
  } else {
    printLog(".");
  }

  // Initialize RTL library
  sl_rtl_loc_init(plibitem);
  // Select mode
  sl_rtl_loc_set_mode(plibitem, SL_RTL_LOC_ESTIMATION_MODE_THREE_DIM);

  // Provide locator configurations to the position estimator
  for (numLocators=0; pLocatorConfigTable[numLocators] != NULL; numLocators++)
  {
    locatorTable[numLocators] = calloc(1, sizeof(locator_t));
    locatorTable[numLocators]->locatorConfigID = pLocatorConfigTable[numLocators]->locatorID;
    locatorTable[numLocators]->locatorItem.coordinate_x = pLocatorConfigTable[numLocators]->pos_x;
    locatorTable[numLocators]->locatorItem.coordinate_y = pLocatorConfigTable[numLocators]->pos_y;
    locatorTable[numLocators]->locatorItem.coordinate_z = pLocatorConfigTable[numLocators]->pos_z;
    locatorTable[numLocators]->locatorItem.orientation_x_axis_degrees = pLocatorConfigTable[numLocators]->rot_x;
    locatorTable[numLocators]->locatorItem.orientation_y_axis_degrees = pLocatorConfigTable[numLocators]->rot_y;
    locatorTable[numLocators]->locatorItem.orientation_z_axis_degrees = pLocatorConfigTable[numLocators]->rot_z;
    sl_rtl_loc_add_locator(plibitem, &locatorTable[numLocators]->locatorItem , &locatorTable[numLocators]->locatorItemID);
  }

  // Create position estimator
  sl_rtl_loc_create_position_estimator(plibitem);
}

bool configPrinted = false;

static int locWaitNewMeasurements(sl_rtl_loc_libitem* plibitem, uint32_t tagID)
{
  // Wait for measurements (angle estimations + distance estimation) from all locator
#ifdef WINDOWS
  SEM_WAIT_T ret = SEM_WAIT_MNB(numLocators, newMeasurementAvailable[tagID]);
#else
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);

  for (uint i = 0; i < numLocators; i++) {
    SEM_WAIT_T ret = sem_timedwait(&newMeasurementAvailable[tagID][i], &ts);
    if (ret == -1) {
      if (errno == ETIMEDOUT) {
        return 1;
      }
    }
  }
#endif

#ifdef WINDOWS
  if (ret == WAIT_TIMEOUT || ret == WAIT_FAILED) {
    return 1;
  }
#endif

  // All locators ready
  // Clear last measurements
  sl_rtl_loc_clear_measurements(plibitem);

  if (!configPrinted) {
    #ifdef EXPORT_LOC_DATA
    writeConfigDataToFile();
    #endif // EXPORT_LOC_DATA

    ENTER_MUTEX(&printfCriticalSection);
    for (int i=0; i < numLocators; i++) {
      printf("{\"config\": 1, \"numlocators\": \"%d\", \"loc\": \"%d\", \"loc_x\": \"%.2f\", \"loc_y\": \"%.2f\", \"loc_z\": \"%.2f\", \"ori_x\": \"%.2f\", \"ori_y\": \"%.2f\", \"ori_z\": \"%.2f\"}\n", numLocators, i, locatorTable[i]->locatorItem.coordinate_x, locatorTable[i]->locatorItem.coordinate_y, locatorTable[i]->locatorItem.coordinate_z, locatorTable[i]->locatorItem.orientation_x_axis_degrees, locatorTable[i]->locatorItem.orientation_y_axis_degrees, locatorTable[i]->locatorItem.orientation_z_axis_degrees);
      fflush(stdout);
    }
    configPrinted = true;
    EXIT_MUTEX(&printfCriticalSection);
  }

  // Provide new measurements (angle estimations + distance estimation) to the position estimator
  for (int i=0; i < numLocators; i++) {
    sl_rtl_loc_set_locator_measurement(plibitem, locatorTable[i]->locatorItemID, SL_RTL_LOC_LOCATOR_MEASUREMENT_AZIMUTH, locatorMeasurements[tagID][i].azimuth);
    sl_rtl_loc_set_locator_measurement(plibitem, locatorTable[i]->locatorItemID, SL_RTL_LOC_LOCATOR_MEASUREMENT_ELEVATION, locatorMeasurements[tagID][i].elevation);
      // Feeding RSSI distance measurement to the RTL library improves location accuracy when the measured distance is reasonably correct. If the received signal strength of the incoming
      // signal is altered for any other reason than the distance between the TX and RX itself, it will lead to incorrect measurement and it will lead to incorrect position estimates.
      // For this reason the RSSI distance usage is disabled by default in the multilocator case.
      // Single locator mode however always requires the distance measurement in addition to the angle, please note the if-condition below.
      // In case the distance estimation should be used in the  multilocator case, you can enable it by commenting out the condition.
      if (numLocators == 1) {
        sl_rtl_loc_set_locator_measurement(plibitem, locatorTable[i]->locatorItemID, SL_RTL_LOC_LOCATOR_MEASUREMENT_DISTANCE,  locatorMeasurements[tagID][i].distance);
      }
  }

  /* Debug data section */
#if defined(EXPORT_ANG_DATA) || defined(DEBUG_ANGLES)
  if (eAppCtrl != eAOX_SHUTDOWN) {
    dumpAngleData(plibitem, tagID);
  }
#endif // defined(EXPORT_ANG_DATA || defined(DEBUG_ANGLES)

  return 0;
}

static void locProcessMeasurements(sl_rtl_loc_libitem* plibitem, uint32_t tagID, sl_rtl_util_libitem* utilLibitems)
{
  float x, y, z;

  for (int i = 0; i < 5; i++) {
    // Process new measurements, time step given in seconds
    sl_rtl_loc_process(plibitem, TAG_PER_ADV_INTERVAL / 1000.0);

    // Get results from the estimator
    sl_rtl_loc_get_result(plibitem, SL_RTL_LOC_RESULT_POSITION_X, &x);
    sl_rtl_loc_get_result(plibitem, SL_RTL_LOC_RESULT_POSITION_Y, &y);
    sl_rtl_loc_get_result(plibitem, SL_RTL_LOC_RESULT_POSITION_Z, &z);

    sl_rtl_util_filter(&utilLibitems[UTIL_X], x, &x);
    sl_rtl_util_filter(&utilLibitems[UTIL_Y], y, &y);
    sl_rtl_util_filter(&utilLibitems[UTIL_Z], z, &z);
  }

  // Print results
  // ENTER_MUTEX(&printfCriticalSection);
  // printf("{\"tag\": \"%d\", \"x\": \"%.2f\", \"y\": \"%.2f\", \"z\": \"%.2f\", \"addr\": \"%02x:%02x:%02x:%02x:%02x:%02x\"}\n", tagID, x, y, z, foundTags[tagID][5], foundTags[tagID][4], foundTags[tagID][3], foundTags[tagID][2], foundTags[tagID][1], foundTags[tagID][0]);
  // fflush(stdout);
  // EXIT_MUTEX(&printfCriticalSection);

#ifdef EXPORT_LOC_DATA
  if (eAppCtrl != eAOX_SHUTDOWN) {
    writeLocDataToFile(tagID, x, y, z);
  }
#endif // EXPORT_LOC_DATA

  for (int i = 0; i < numLocators; i++) {
    locatorMeasurements[tagID][i].loc_sequence++;
  }


  for (int i = 0; i < numLocators; i++) {
    SEM_RESET(newMeasurementAvailable[tagID][i]);
  }
}

/* Debug section */

#ifdef EXPORT_LOC_DATA
static void writeConfigDataToFile()
{
  static FILE* confDataF = NULL;
  if (confDataF == NULL)
  {
    char confpath[50];
#ifdef WINDOWS
    mkdir("logs");
#else
    mkdir("logs", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif
    sprintf(confpath, "logs/log_config_data.out");
    confDataF = fopen(confpath, "w+t");
  }

  for (int i = 0; i < numLocators; i++) {
    fprintf(confDataF, "loc: %d, pos_x: %.3f, pos_y: %.3f, pos_z: %.3f, ori_x: %.3f, ori_y: %.3f, ori_z: %.3f\n", i, locatorTable[i]->locatorItem.coordinate_x, locatorTable[i]->locatorItem.coordinate_y, locatorTable[i]->locatorItem.coordinate_z, locatorTable[i]->locatorItem.orientation_x_axis_degrees, locatorTable[i]->locatorItem.orientation_y_axis_degrees, locatorTable[i]->locatorItem.orientation_z_axis_degrees);
  }

  fflush(confDataF);
  fclose(confDataF);
}
#endif // EXPORT_LOC_DATA

#ifdef EXPORT_LOC_DATA
static void writeLocDataToFile(uint32_t tagID, float x, float y, float z)
{
  static FILE* locDataF[MAX_NUM_TAGS] = {NULL};
  if (locDataF[tagID] == NULL)
  {
    char loc_dump_path[128];
    mkdir("logs");
    sprintf(loc_dump_path, "logs/log_loc_data_tag%d.out", tagID);
    locDataF[tagID] = fopen(loc_dump_path, "w+t");
  }

  fprintf(locDataF[tagID], "LOCSEQ: %d, x: %.3f, y: %.3f, z: %.3f\n", locatorMeasurements[tagID][0].loc_sequence, x, y, z);
  fflush(locDataF[tagID]);
}
#endif // EXPORT_LOC_DATA

static void dumpAngleData(sl_rtl_loc_libitem* plibitem, uint32_t tagID)
{
  float az_out = 0.0f;
  float el_out = 0.0f;

#ifdef EXPORT_ANG_DATA
  static FILE* angDataF[MAX_NUM_LOCATORS][MAX_NUM_TAGS] = {NULL};
  for (int i = 0; i < numLocators; i++) {
    if (angDataF[i][tagID] == NULL)
    {
      char ang_dump_path[128];
      mkdir("logs");
      sprintf(ang_dump_path, "logs/log_ang_data_loc%d_tag%d.out", i, tagID);
      angDataF[i][tagID] = fopen(ang_dump_path, "w+t");
    }
  }

#endif // EXPORT_ANG_DATA

// #ifdef DEBUG_ANGLES
//   ENTER_MUTEX(&printfCriticalSection);
//   printf("{\"numlocators\": \"%d\", \"tag\": \"%d\"", numLocators, tagID);
// #endif // DEBUG_ANGLES

  for (int i=0; i < numLocators; i++) {
    sl_rtl_loc_get_measurement_in_system_coordinates(plibitem, locatorTable[i]->locatorItemID, SL_RTL_LOC_LOCATOR_MEASUREMENT_AZIMUTH, &az_out);
    sl_rtl_loc_get_measurement_in_system_coordinates(plibitem, locatorTable[i]->locatorItemID, SL_RTL_LOC_LOCATOR_MEASUREMENT_ELEVATION, &el_out);
#ifdef DEBUG_ANGLES
    if (numLocators == 1) {
      printf(", \"az%d\": %.2f, \"el%d\": %.2f, \"dist%d\": %.2f", i, locatorMeasurements[tagID][i].azimuth, i, locatorMeasurements[tagID][i].elevation, i, locatorMeasurements[tagID][i].distance);
    } else {
      printf(", \"az%d\": %.2f, \"el%d\": %.2f, \"dist%d\": %.2f", i, az_out, i, el_out, i, locatorMeasurements[tagID][i].distance);
    }
    fflush(stdout);
#endif // DEBUG_ANGLES

#ifdef EXPORT_ANG_DATA
    fprintf(angDataF[i][tagID], "LOCSEQ: %d, IQSEQ: %d, az: %.3f, el: %.3f, dist: %.3f, az_sys: %.3f, el_sys: %.3f\n", locatorMeasurements[tagID][i].loc_sequence, locatorMeasurements[tagID][i].iq_sequence, locatorMeasurements[tagID][i].azimuth,
            locatorMeasurements[tagID][i].elevation, locatorMeasurements[tagID][i].distance, az_out, el_out);
    fflush(angDataF[i][tagID]);
#endif // EXPORT_ANG_DATA
  }

#ifdef DEBUG_ANGLES
  printf("}\n");
  fflush(stdout);
  EXIT_MUTEX(&printfCriticalSection);
#endif // DEBUG_ANGLES
}


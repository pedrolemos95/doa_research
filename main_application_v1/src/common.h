/***********************************************************************************************//**
 * @file   common.h
 * @brief  Common header file for different threads to enable communication between them
 ***************************************************************************************************
 * # License
 * <b>Copyright 2019 Silicon Laboratories Inc. www.silabs.com</b>
 ***************************************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this software is governed 
 * by the terms of Silicon Labs Master Software License Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This software is distributed to
 * you in Source Code format and is governed by the sections of the MSLA applicable to Source Code.
 **************************************************************************************************/

#ifndef COMMON_H
#define COMMON_H

#ifdef WINDOWS

#undef _WIN32_WINNT
#undef WINVER
#define _WIN32_WINNT 0x0600
#define WINVER 0x0600
#include <windows.h>

#define MUTEX_T               CRITICAL_SECTION
#define CONDITION_T           CONDITION_VARIABLE
#define THREAD_RETURN_T       DWORD WINAPI
#define THREAD_T              HANDLE
#define THREAD_ID_T           DWORD
#define SEM_T                 HANDLE
#define SEM_WAIT_T            DWORD
#define SEM_RETURN_T          SEM_T

#define SLEEP(ms)             Sleep(ms)
#define GET_THREAD_ID()       GetCurrentThreadId()
#define MUTEX_INIT(m)         InitializeCriticalSection(m)
#define MUTEX_DEINIT(m)       DeleteCriticalSection(m)
#define ENTER_MUTEX(m)        EnterCriticalSection(m)
#define EXIT_MUTEX(m)         LeaveCriticalSection(m)
#define CONDITION_INIT(c)     InitializeConditionVariable(c) 
#define CONDITION_DEINIT(c) 
#define CONDITION_WAIT(c,m)   SleepConditionVariableCS(c,m,INFINITE)
#define CONDITION_MET(c)      WakeConditionVariable(c)
#define THREAD_EXIT           return 0
#define SEM_INIT(m)           CreateEvent(NULL, TRUE, FALSE, NULL) 
#define SEM_DEINIT(m)         DeleteCriticalSection(m)
#define SEM_WAIT_S(m)         WaitForSingleObject(m, INFINITE)
#define SEM_WAIT_SNB(m)       WaitForSingleObject(m, 0)
#define SEM_WAIT_MNB(num, m)  WaitForMultipleObjects(num, m, TRUE, 0)
#define SEM_SIGNAL(m)         SetEvent(m)
#define SEM_RESET(m)          ResetEvent(m)

#else

#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <errno.h>
#include <time.h>

#define MUTEX_T             pthread_mutex_t
#define CONDITION_T         pthread_cond_t
#define THREAD_RETURN_T     void*
#define THREAD_T            pthread_t
#define THREAD_ID_T         pthread_t
#define SEM_T               sem_t
#define SEM_WAIT_T          int
#define SEM_RETURN_T        int

#define SLEEP(ms)             usleep(ms*1000)
#define GET_THREAD_ID()       pthread_self()
#define MUTEX_INIT(m)         pthread_mutex_init(m, NULL)
#define MUTEX_DEINIT(m)       pthread_mutex_destroy(m)
#define ENTER_MUTEX(m)        pthread_mutex_lock(m)
#define EXIT_MUTEX(m)         pthread_mutex_unlock(m)
#define CONDITION_INIT(c)     pthread_cond_init(c, NULL)
#define CONDITION_DEINIT(c)   pthread_cond_destroy(c)
#define CONDITION_WAIT(c,m)   pthread_cond_wait(c,m)
#define CONDITION_MET(c)      pthread_cond_broadcast(c)
#define THREAD_EXIT           pthread_exit(NULL)
#define SEM_INIT(m)           sem_init(m, 0, 0)
#define SEM_DEINIT(m)         sem_deinit(m)
#define SEM_WAIT_S(m)         sem_wait(m)
#define SEM_SIGNAL(m)         sem_post(m)
#define SEM_RESET(m)

#endif

#include <signal.h>

/* standard library headers */
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
//#include <unistd.h>
#include <errno.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>

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
 
/***************************************************************************************************
 * Local Macros and Definitions
 **************************************************************************************************/

 #define MAX_NUM_LOCATORS 6                 // max number of locators
 #define MAX_NUM_TAGS 50                    // max number of tags

/***************************************************************************************************
 * Type Definitions
 **************************************************************************************************/

typedef enum {
    eAOX_RUN = 0,
    eAOX_PAUSE,
    eAOX_SHUTDOWN

} eAOX_APP_CTRL;

typedef struct IQsamples {
  float** ref_i_samples;
  float** ref_q_samples;
  float** i_samples;
  float** q_samples;
  uint8_t connection;
  uint8_t channel;
  int16_t rssi;
  uint32_t sequence;
} iqSamples_t;

typedef struct LocatorMeasurement {
  int     locatorID;
  int     sampleCount;
  float   azimuth;
  float   elevation;
  float   distance;
  uint8_t connection;
  uint8_t channel;
  int16_t rssi;
  uint32_t iq_sequence;
  uint32_t loc_sequence;
  uint8_t valid;
} locatorMeasurement_t;

typedef struct locatorConfig {
  int    locatorID;
  uint32_t tagID;
  char   locatorName[16];
  char   COMport[64];
  char   IPaddress[16];
  float  pos_x;
  float  pos_y;
  float  pos_z;
  float  rot_x;
  float  rot_y;
  float  rot_z;
  float  az_constr_min;
  float  az_constr_max;
  float  el_constr_min;
  float  el_constr_max;
} locatorConfig_t;

/***************************************************************************************************
 * Global variables
 **************************************************************************************************/
// Mutexes
extern MUTEX_T  iqSamplesCriticalSection[MAX_NUM_TAGS][MAX_NUM_LOCATORS];
extern MUTEX_T  printfCriticalSection;

extern MUTEX_T  bgBufferCriticalSection;
extern MUTEX_T  tagHandleCriticalSection;

// Object handle
extern SEM_T         newMeasurementAvailable[MAX_NUM_TAGS][MAX_NUM_LOCATORS];
extern SEM_T         newSamplesAvailable[MAX_NUM_LOCATORS][MAX_NUM_TAGS];

// Application state
extern eAOX_APP_CTRL eAppCtrl;

/***************************************************************************************************
 * Function Declarations
 **************************************************************************************************/

int allocate2DFloatBuffer(float*** buf, int rows, int cols);

#define printLog(...) ENTER_MUTEX(&printfCriticalSection); printf(__VA_ARGS__); EXIT_MUTEX(&printfCriticalSection);

/** @} (end addtogroup app) */
/** @} (end addtogroup Application) */

#endif /* COMMON_H */

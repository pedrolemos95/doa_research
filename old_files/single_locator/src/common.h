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

#include <pthread.h>

#define MUTEX_T             pthread_mutex_t
#define CONDITION_T         pthread_cond_t
#define THREAD_RETURN_T     void*

#define ENTER_MUTEX(m)      pthread_mutex_lock(m)
#define EXIT_MUTEX(m)       pthread_mutex_unlock(m)
#define CONDITION_WAIT(c,m) pthread_cond_wait(c,m)
#define CONDITION_MET(c)    pthread_cond_broadcast(c)
#define THREAD_EXIT         pthread_exit(NULL)

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
 * Type Definitions
 **************************************************************************************************/

typedef struct arguments {
  int    argc;
  char** argv;
} arguments_t;

typedef enum {
    eAOX_RUN = 0,
    eAOX_PAUSE,
    eAOX_SHUTDOWN

} eAOX_APP_CTRL;

/***************************************************************************************************
 * Global variables
 **************************************************************************************************/

// Mutexes
extern MUTEX_T     iqSamplesCriticalSection;
extern MUTEX_T     bgBufferCriticalSection;
extern MUTEX_T     printfCriticalSection;
#ifndef WINDOWS
extern MUTEX_T     newSamplesAvailableMutex;
#endif

//Conditional variable
extern CONDITION_T newSamplesAvailable;

// Application state
extern eAOX_APP_CTRL eAppCtrl;

#define printLog(...) ENTER_MUTEX(&printfCriticalSection); printf(__VA_ARGS__); EXIT_MUTEX(&printfCriticalSection);

#endif /* COMMON_H */

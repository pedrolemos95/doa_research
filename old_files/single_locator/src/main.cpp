/***********************************************************************************************//**
 * @file   main.c
 * @brief  This sample app demonstrates AoX calculation running on a PC while Bluetooth stack is
 *         running on EFR. The PC communicates with the EFR via UART using BGLIB
 ***************************************************************************************************
 * # License
 * <b>Copyright 2019 Silicon Laboratories Inc. www.silabs.com</b>
 ***************************************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this software is governed
 * by the terms of Silicon Labs Master Software License Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This software is distributed to
 * you in Source Code format and is governed by the sections of the MSLA applicable to Source Code.
 **************************************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "infrastructure.h"
#include "common.h"
#include "bg.h"
#include "aox.h"

/***************************************************************************************************
 * Local Macros and Definitions
 **************************************************************************************************/

#define THREADCOUNT 2                 // max number of threads

/***************************************************************************************************
 * Static Variable Declarations
 **************************************************************************************************/

static pthread_t ghThreads[THREADCOUNT]; // array of thread handles

/***************************************************************************************************
 * Static Function Declarations
 **************************************************************************************************/

static void SignalHandler(int signal);
static void mutexInit();
static void mutexDeinit();

/***************************************************************************************************
 * Public Function Definitions
 **************************************************************************************************/

int main(int argc, char* argv[])
{
  arguments_t args;
  args.argc = argc;
  args.argv = &argv[0];

  int threadCount = 0;

  int retErr;
  pthread_attr_t attr;
  /* Initialize and set thread detached attribute */
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  // Attach signal handler to ctrl-C SIGTERM signal
  signal(SIGINT, SignalHandler);
  eAppCtrl=eAOX_RUN;

  // Initialize mutexes
  mutexInit();

  // Initialize AoX lib
  aoxInit(&args);

  // Initialize BG lib
  bgInit(&args);

  // Start AoX thread
  retErr = pthread_create( &ghThreads[threadCount++],         // Thread
                           &attr,                             // attribute, set to joinable
                           aoxMain,                           // Thread routine
                           (void*)&args );                    // Args

  if(retErr != 0){
    printf("\nError creating AoX thread. retErr :[%s]", strerror(retErr));
    return retErr;
  }

  // Start BG thread
  retErr = pthread_create( &ghThreads[threadCount++],         // Thread
                           &attr,                             // attribute, set to joinable
                           bgMain,                            // Thread routine
                           (void*)&args );                    // Args

  if(retErr != 0){
    printf("\nError creating BLE thread. retErr :[%s]", strerror(retErr));
    return retErr;
  }

  /* Free attribute and wait for the other threads */
  pthread_attr_destroy(&attr);

  for(threadCount = 0; threadCount < THREADCOUNT; threadCount++){
    pthread_join(ghThreads[threadCount], NULL);
  }

  mutexDeinit();

  return -1;
}

/***************************************************************************************************
 * Static Function Definitions
 **************************************************************************************************/
static void SignalHandler(int signal)
{
  // Cleanup and leave app
  if (signal == SIGINT)
  {
    printf("Main thread waiting for threads to exit...\n");

    eAppCtrl=eAOX_SHUTDOWN;
  }

  return;
}

static void mutexInit()
{
  // Initialize mutexes
  pthread_mutex_init(&iqSamplesCriticalSection, NULL);
  pthread_mutex_init(&bgBufferCriticalSection, NULL);
  pthread_cond_init(&newSamplesAvailable,NULL);
}

static void mutexDeinit()
{
  // Deinitialize mutexes
  pthread_mutex_destroy(&iqSamplesCriticalSection);
  pthread_mutex_destroy(&bgBufferCriticalSection);
  pthread_cond_destroy(&newSamplesAvailable);
}


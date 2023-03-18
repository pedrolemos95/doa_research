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

#include "infrastructure.h"
#include "common.h"
#include "bg.h"
#include "aox.h"
#include "loc.h"

/***************************************************************************************************
 * Local Macros and Definitions
 **************************************************************************************************/

/***************************************************************************************************
 * Static Variable Declarations
 **************************************************************************************************/

static THREAD_T ghThreads[64][64]; // array of thread handles, note: can only wait for 64 objects
static locatorConfig_t *pLocatorConfig[MAX_NUM_LOCATORS];
static locatorConfig_t *pLocatorConfigN[MAX_NUM_LOCATORS][MAX_NUM_TAGS];

/***************************************************************************************************
 * Static Function Declarations
 **************************************************************************************************/

static void SignalHandler(int signal);

/***************************************************************************************************
 * Public Function Definitions
 **************************************************************************************************/
int threadIndex;
int threadCount;
int threadStackCount;

static void increaseThreadCount()
{
  threadCount++;
  threadIndex = threadCount % 64;
  threadStackCount = threadCount / 64;
} 

int main(int argc, char* argv[])
{
  FILE * pConfigFile;
  char line [ 256 ];
  int locatorID = 0;

#ifdef WINDOWS
  DWORD dwThreadID;
#else
  int retErr;
  pthread_attr_t attr;

  // Initialize and set thread detached attribute
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
#endif

  // Check argument
  if (argc < 2) {
    printf("USAGE: ./locator-host config-file\r\n");
    return -1;
  }

  threadIndex = 0;
  threadCount = 0;

  // Attach signal handler to ctrl-C SIGTERM signal
  signal(SIGINT, SignalHandler);
  eAppCtrl=eAOX_RUN;

  // Initialized critical section for printf to avoid messy prints
  MUTEX_INIT(&printfCriticalSection);
  MUTEX_INIT(&bgBufferCriticalSection);
  MUTEX_INIT(&tagHandleCriticalSection);

  // Open config file
  pConfigFile = fopen(argv[1],"r");

  if (pConfigFile == NULL) {
    printf("couldn't open config file\r\n");
    return -1;
  }

  // Read the header of the config file
  fgets(line, sizeof(line), pConfigFile); //read version info
  fgets(line, sizeof(line), pConfigFile); //read header

  // Init locator config table
  for (int i = 0; i < MAX_NUM_LOCATORS; i++) {
    pLocatorConfig[i] = NULL;
    for (int j = 0; j < MAX_NUM_TAGS; j++) {
      pLocatorConfigN[i][j] = NULL;
    }
  }

  // Read the configuration of each locator from the config file
  // Start BG and AoX threads for each locator
  for ( locatorID = 0, pLocatorConfig[locatorID] = (locatorConfig_t*)calloc(1, sizeof(locatorConfig_t));
        fscanf(pConfigFile,"%s %s %s %f %f %f %f %f %f %f %f",  &pLocatorConfig[locatorID]->locatorName[0],
                                                          &pLocatorConfig[locatorID]->COMport[0],
                                                          &pLocatorConfig[locatorID]->IPaddress[0],
                                                          &pLocatorConfig[locatorID]->pos_x,
                                                          &pLocatorConfig[locatorID]->pos_y,
                                                          &pLocatorConfig[locatorID]->pos_z,
                                                          &pLocatorConfig[locatorID]->rot_x,
                                                          &pLocatorConfig[locatorID]->rot_y,
                                                          &pLocatorConfig[locatorID]->rot_z,
                                                          &pLocatorConfig[locatorID]->az_constr_min,
                                                          &pLocatorConfig[locatorID]->az_constr_max) == 11;
        locatorID++, pLocatorConfig[locatorID] = (locatorConfig_t*)calloc(1, sizeof(locatorConfig_t))) {

    pLocatorConfig[locatorID]->locatorID = locatorID;

    for (uint32_t tagID = 0; tagID < MAX_NUM_TAGS; tagID++) 
    {
      // Initialize critical sections and condition variables (signalling between BG -> AOX)
      MUTEX_INIT(&iqSamplesCriticalSection[tagID][locatorID]);

#ifdef WINDOWS
      // Initialize events (signalling between AOX -> LOC)
      newSamplesAvailable[locatorID][tagID] = CreateEvent( NULL,       // default security attributes
                                                      TRUE,      // auto reset event
                                                      FALSE,      // initial state is nonsignaled
                                                      NULL);  // object name 
      newMeasurementAvailable[tagID][locatorID] = CreateEvent( NULL,       // default security attributes
                                                      TRUE,      // manually reset event
                                                      FALSE,      // initial stat
                                                      NULL);
#else
      SEM_INIT(&newSamplesAvailable[locatorID][tagID]);
      SEM_INIT(&newMeasurementAvailable[tagID][locatorID]);
#endif
    }

#ifdef WINDOWS

    // Start BG thread for this locator (this thread will communicate with the locator via BGAPI)
    ghThreads[threadStackCount][threadIndex] = CreateThread( NULL,                // default security
                                             0,                   // default stack size
                                             bgMain,              // name of the thread function
                                             (void*)pLocatorConfig[locatorID], // thread parameters
                                             0,                   // default startup flags
                                             &dwThreadID );

#else

    // Start BG thread for this locator (this thread will communicate with the locator via BGAPI)
    retErr = pthread_create( &ghThreads[threadStackCount][threadIndex],          // Thread
                             &attr,                              // attribute, set to joinable
                             bgMain,                             // Thread routine
                             (void*)pLocatorConfig[locatorID] ); // Args

    if(retErr != 0){
      printf("\nError creating BLE thread. retErr :[%s]", strerror(retErr));
      return retErr;
    }

#endif

    increaseThreadCount();

    for (uint32_t tagID = 0; tagID < MAX_NUM_TAGS; tagID++)
    {
      pLocatorConfigN[locatorID][tagID] = (locatorConfig_t*)calloc(1, sizeof(locatorConfig_t));
      memcpy(pLocatorConfigN[locatorID][tagID], pLocatorConfig[locatorID], sizeof(locatorConfig_t)); 

      pLocatorConfigN[locatorID][tagID]->tagID = tagID;
      
#ifdef WINDOWS
      // Start AoX thread for this locator (this thread will calculate Angle-of-Arrival from the received IQ samples)
      ghThreads[threadStackCount][threadIndex] = CreateThread( NULL,                // default security
                                               0,                   // default stack size
                                               aoxMain,             // name of the thread function
                                               (void*)pLocatorConfigN[locatorID][tagID], // thread parameters
                                               0,                   // default startup flags
                                               &dwThreadID );
#else

    // Start AoX thread for this locator (this thread will calculate Angle-of-Arrival from the received IQ samples)
    retErr = pthread_create( &ghThreads[threadStackCount][threadIndex],          // Thread
                             &attr,                              // attribute, set to joinable
                             aoxMain,                            // Thread routine
                             (void*)pLocatorConfigN[locatorID][tagID] ); // Args

    if(retErr != 0){
      printf("\nError creating AoX thread. retErr :[%s]", strerror(retErr));
      return retErr;
    }

#endif

      increaseThreadCount();
    }
  }

  // Free the last, unused config
  free(pLocatorConfig[locatorID]);
  pLocatorConfig[locatorID] = NULL;

#ifdef WINDOWS

  // Start Loc thread (this thread will calculate location from the calculated Angles-of-Arrival)
  ghThreads[threadStackCount][threadIndex] = CreateThread( NULL,              // default security
                                           0,                 // default stack size
                                           locMain,           // name of the thread function
                                           (void*)pLocatorConfig, // thread parameters
                                           0,                 // default startup flags
                                           &dwThreadID );

#else

  // Start Loc thread (this thread will calculate location from the calculated Angles-of-Arrival)
  retErr = pthread_create(  &ghThreads[threadStackCount][threadIndex],          // Thread
                            &attr,                              // attribute, set to joinable
                            locMain,                            // Thread routine
                            (void*)pLocatorConfig );            // Args

  if(retErr != 0){
    printf("\nError creating LOC thread. retErr :[%s]", strerror(retErr));
    return retErr;
  }

#endif

  increaseThreadCount();

  printf("Max num threads: %d, thread count: %d \n", MAX_NUM_LOCATORS * MAX_NUM_TAGS + MAX_NUM_LOCATORS + 2, threadCount);
  fflush(stdout);
  printf("threadstackcount %d thread index : %d \n", threadStackCount, threadIndex);
  fflush(stdout);

#ifdef WINDOWS

  // Wait for threads to finish
  for (uint32_t i = 0; i < threadStackCount + 1; i++) {
    if (i == threadStackCount) {
      WaitForMultipleObjects(threadIndex, ghThreads[i], TRUE, INFINITE);
    }
    else {
      WaitForMultipleObjects(63, ghThreads[i], TRUE, INFINITE);
    }
  }

#else

  // Free attribute
  pthread_attr_destroy(&attr);

  // Wait for threads to finish
  for (uint32_t i = 0; i < threadStackCount + 1; i++) {
    uint threadMax = 64;
    if (i >= threadStackCount) {
      threadMax = threadIndex;
    } 

    for(threadCount = 0; threadCount < threadMax; threadCount++){
      pthread_join(ghThreads[i][threadCount], NULL);
    }
  }
  
#endif

  // Deinitialize critical sections
  for (int i = 0; i < locatorID; i++) {
    for (int j = 0; j < MAX_NUM_TAGS; j++) {
      MUTEX_DEINIT(&iqSamplesCriticalSection[j][i]);
    }
  }

  MUTEX_DEINIT(&printfCriticalSection);
  MUTEX_DEINIT(&bgBufferCriticalSection);
  MUTEX_DEINIT(&tagHandleCriticalSection);

  return 0;
}

/***************************************************************************************************
 * Static Function Definitions
 **************************************************************************************************/
static void SignalHandler(int signal)
{
  // Cleanup and leave app
  if (signal == SIGINT)
  {
    printLog("                                                            \rMain thread waiting for threads to exit...\n");
    eAppCtrl=eAOX_SHUTDOWN;

    for (int loc = 0; loc < MAX_NUM_LOCATORS; loc++) {
      for (int tagID = 0; tagID < MAX_NUM_TAGS; tagID++) { 
        // let AoX thread shutdown
#ifdef WINDOWS
        SEM_SIGNAL(newSamplesAvailable[loc][tagID]);
        SEM_SIGNAL(newMeasurementAvailable[tagID][loc]);
#else
        SEM_SIGNAL(&newSamplesAvailable[loc][tagID]);
        SEM_SIGNAL(&newMeasurementAvailable[tagID][loc]);
#endif

        EXIT_MUTEX(&iqSamplesCriticalSection[tagID][loc]);
      }
    }

    EXIT_MUTEX(&tagHandleCriticalSection);
    EXIT_MUTEX(&bgBufferCriticalSection); 

  }
  return;
}

#include "scheduler.h"

TaskHandle_t xHandle1 = NULL;
TaskHandle_t xHandle2 = NULL;
TaskHandle_t xHandle3 = NULL;

/*Task Set 1
 * Global variables of WCET of each task
 * According to Q3 of HW4, which IS SCHEDULABLE 
 */
TickType_t WCET_1 = pdMS_TO_TICKS(400);
TickType_t WCET_2 = pdMS_TO_TICKS(300);
//TickType_t WCET_3 = pdMS_TO_TICKS(400);

/* Task Set 2 
 *  Only 1 task WCET is changed
 *  Use this when testing RM PCP
*/
TickType_t WCET_3 = pdMS_TO_TICKS(700);

/* Used to Enable and Disable RM PCP Functionality in ino */
BaseType_t Enable_RMPCP = 1;

/* Lock on R1, used by T1 and T2*/
static SemaphoreHandle_t mutex_lock_R1;

/* Lock on R2, used by T1 and T3 */
static SemaphoreHandle_t mutex_lock_R2;

/* Ceiling of R1 is T1 Priority */
BaseType_t R1_Ceiling = 3;

/* Ceiling of R2 is T3 Priority */
BaseType_t R2_Celing = 3;

/* Var to hold the current ceiling at any given time */
int Current_Sys_Ceiling = 0;

/* Variable to hold return of xSemaphoreTake() R1*/
BaseType_t mutex_taken_R1 = pdFALSE;

/*Variable to hold return of xSemaphoreTake() R2 */
BaseType_t mutex_taken_R2 = pdFALSE;


void loop() {}

/*
 * Uses mutex_lock_R1
*/
static void testFunc1( void *pvParameters )
{
  (void) pvParameters;

  if( Enable_RMPCP == 1)
  {
    if(uxTaskPriorityGet(xHandle1) > Current_Sys_Ceiling)
    {
      Current_Sys_Ceiling = uxTaskPriorityGet(xHandle1);
      mutex_taken_R1 = xSemaphoreTake(mutex_lock_R1, 0);
  
      if(mutex_taken_R1 == pdTRUE)
      {
        Serial.println(" T1 is TAKING R1 mutex lock");
        Serial.flush();
      }
    }
    else 
    {
      Serial.println(" T1 is BLOCKED from R1 taking mutex lock");
      Serial.flush();
    }
  }
  volatile TickType_t i, j;

  i = xTaskGetTickCount();

  while (1)
  {
    j = xTaskGetTickCount();

    if ((j - i) >= WCET_1)
    {
      break;
    }
  }

  if( Enable_RMPCP == 1)
  {
    if(mutex_taken_R1 == pdTRUE)
    {
       Serial.println(" T1 is GIVING R1 mutex lock");
       Serial.flush();
       xSemaphoreGive(mutex_lock_R1);    
    }
  
    Current_Sys_Ceiling = 0;
  }
}

/*
 * mutex_lock_RMPCP_R1
*/
static void testFunc2( void *pvParameters )
{
  (void) pvParameters;

  if( Enable_RMPCP == 1)
  {
    if(uxTaskPriorityGet(xHandle2) > Current_Sys_Ceiling)
    {
      Current_Sys_Ceiling = uxTaskPriorityGet(xHandle1);
      mutex_taken_R1 = xSemaphoreTake(mutex_lock_R1, 0);
  
      if(mutex_taken_R1 == pdTRUE)
      {
        Serial.println(" T2 is TAKING R1 mutex lock");
        Serial.flush();
      }
    }
    else 
    {
      Serial.println(" T2 is BLOCKED from taking R1 mutex lock");
      Serial.flush();
    }
  }
  volatile TickType_t i, j;

  i = xTaskGetTickCount();

  while (1)
  {
    j = xTaskGetTickCount();

    if ((j - i) >= WCET_2)
    {
      break;
    }
  }

  if( Enable_RMPCP == 1)
  {
       if(mutex_taken_R1 == pdTRUE)
      {
         Serial.println(" T2 is GIVING R1 mutex lock");
         Serial.flush();
         xSemaphoreGive(mutex_lock_R2 );    
      }
    
        Current_Sys_Ceiling = 0;
    }
}

/*
 * mutex_lock_RMPCP_R2
*/
static void testFunc3( void *pvParameters )
{
  (void) pvParameters;

  if( Enable_RMPCP == 1)
  {
    if(uxTaskPriorityGet(xHandle3) > Current_Sys_Ceiling)
    {
      Current_Sys_Ceiling = uxTaskPriorityGet(xHandle1);
      mutex_taken_R2 = xSemaphoreTake(mutex_lock_R2, 0);
  
      if(mutex_taken_R2 == pdTRUE)
      {
        Serial.println(" T3 is TAKING R2 mutex lock");
        Serial.flush();
      }
    }
    else 
    {
      Serial.println(" T3 is BLOCKED from taking R2 mutex lock");
      Serial.flush();
    }
  }
  volatile TickType_t i, j;

  i = xTaskGetTickCount();
  
  while (1)
  {
    j = xTaskGetTickCount();

    if ((j - i) >= WCET_3)
    {
      break;
    }
    /*To validate priority inheritance of Task 3 when preempted by Task 1
    Serial.print("Task 3 Priority: ");
    Serial.println(uxTaskPriorityGet(xHandle3));
    Serial.flush();*/
  }
  if( Enable_RMPCP == 1)
  {
    if(mutex_taken_R2 == pdTRUE)
    {
       Serial.println(" T3 is GIVING R2 mutex lock");
       Serial.flush();
       xSemaphoreGive(mutex_lock_R2);    
    }
  }
    Current_Sys_Ceiling = 0;
}

int main( void )
{
    char c1 = 'a';
    char c2 = 'b';
    char c3 = 'c';
  
    //set up serial comm with board 
    Serial.begin(9600);
    while (!Serial) {
      ;
    }
  
    //initialize scheduler
    //initializes xExtended_TCB array
    vSchedulerInit();

    if(Enable_RMPCP == 1)
    { 
      mutex_lock_R1 = xSemaphoreCreateMutex();
      if(mutex_lock_R1 != NULL)
      {
        Serial.println("Mutex for Resource 1 CREATED - Used by T1 and T2");
        Serial.flush();
      }
      else
      {
        Serial.println("Mutex for Resource 1 NOT CREATED");
        Serial.flush();
      } 
      
      mutex_lock_R2 = xSemaphoreCreateMutex();
      if(mutex_lock_R2 != NULL)
      {
        Serial.println("Mutex for Resource 2 CREATED - Used by T3");
        Serial.flush();
      }
      else
      {
        Serial.println("Mutex for Resource 2 NOT CREATED");
        Serial.flush();
      }
    }
  
    //create periodic task
    //adds task to xExtended_TCB array
    //pdMS_TO_TICKS converts time specified in milliseconds to a time specified in RTOS ticks
    //Task Set #1
    //                           Function   Name      StackSize            Param Prio Handle   Phase             Period           Max Exec Time     Deadline  
    vSchedulerPeriodicTaskCreate(testFunc1, "t1", configMINIMAL_STACK_SIZE, &c1, 1, &xHandle1, pdMS_TO_TICKS(0), pdMS_TO_TICKS(1000), WCET_1, pdMS_TO_TICKS(1000));
    vSchedulerPeriodicTaskCreate(testFunc2, "t2", configMINIMAL_STACK_SIZE, &c2, 2, &xHandle2, pdMS_TO_TICKS(0), pdMS_TO_TICKS(1500), WCET_2, pdMS_TO_TICKS(1500));
    vSchedulerPeriodicTaskCreate(testFunc3, "t3", configMINIMAL_STACK_SIZE, &c3, 3, &xHandle3, pdMS_TO_TICKS(0), pdMS_TO_TICKS(2000), WCET_3, pdMS_TO_TICKS(2000));

    //start system
    //start scheduling tasks with specified scheduling policy
    vSchedulerStart();
  
    /* If all is well, the scheduler will now be running, and the following line
      will never be reached. */
    for ( ;; );
}

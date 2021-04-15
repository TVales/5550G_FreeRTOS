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
TickType_t WCET_3 = pdMS_TO_TICKS(400);

/*global variable handle for mutex*/
static SemaphoreHandle_t mutex_lock;
TickType_t mutex_wait = 5;

void loop() {}

/*Task L - Low Priority*/
static void testFunc1( void *pvParameters )
{
  (void) pvParameters;

  volatile TickType_t i, j;
  BaseType_t mutex_taken;

  i = xTaskGetTickCount();

  /*Return value of xSemaphoreTake...
   * pdTRUE, mutex taken successfully
   * pdFALSE, mutex is already taken
   */
  mutex_taken = xSemaphoreTake(mutex_lock, mutex_wait);
 
  if(mutex_taken == pdTRUE)
  {
    Serial.println("Task 1 is TAKING mutex lock");
    Serial.flush();
  }
  else
  {
    Serial.println("Task 1 is BLOCKED from taking mutex");
    Serial.flush();

    /*If take failed earlier because it was blocked, 
    wait for it to finish then take it again
    mutex_taken = xSemaphoreTake(mutex_lock, portMAX_DELAY);
    Serial.println("Task 1 is TAKING mutex lock");*/
  }

  while (1)
  {
    j = xTaskGetTickCount();

    if ((j - i) >= WCET_1)
    {
      break;
    }
  }
  
  if(mutex_taken == pdTRUE)
  {
    Serial.println("Task 1 is GIVING mutex lock");
    Serial.flush();
    xSemaphoreGive(mutex_lock);
  }
}

/*Task M - Medium Priority*/
static void testFunc2( void *pvParameters )
{
  (void) pvParameters;

  volatile TickType_t i, j;
  BaseType_t mutex_taken;

  i = xTaskGetTickCount();

  mutex_taken = xSemaphoreTake(mutex_lock, mutex_wait);

  if(mutex_taken == pdTRUE)
  {
    Serial.println("Task 2 is TAKING mutex lock");
    Serial.flush();
  }
  else
  {
    Serial.println("Task 2 is BLOCKED from taking mutex");
    Serial.flush();
  }

  while (1)
  {
    j = xTaskGetTickCount();

    if ((j - i) >= WCET_2)
    {
      break;
    }
  }
  
  if(mutex_taken == pdTRUE)
  {
    Serial.println("Task 2 is GIVING mutex lock");
    Serial.flush();
    xSemaphoreGive(mutex_lock);
  }
}

/*Task H - High Priority*/
static void testFunc3( void *pvParameters )
{
  (void) pvParameters;
  
  volatile TickType_t i, j;
  BaseType_t mutex_taken; 

  i = xTaskGetTickCount();

  mutex_taken = xSemaphoreTake(mutex_lock, mutex_wait);

  if(mutex_taken == pdTRUE)
  {
    Serial.println("Task 3 is TAKING mutex lock");
    Serial.flush();
  }
  else
  {
    Serial.println("Task 3 is BLOCKED from taking mutex");
    Serial.flush();
  }
  
  while (1)
  {
    j = xTaskGetTickCount();

    if ((j - i) >= WCET_3)
    {
      break;
    }
    /*Serial.print("Task 3 Priority: ");
    Serial.println(uxTaskPriorityGet(xHandle3));*/
    Serial.flush();
  }

  if(mutex_taken == pdTRUE)
  {
    Serial.println("Task 3 is GIVING mutex lock");
    Serial.flush();
    xSemaphoreGive(mutex_lock);
  }
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
  
    //create periodic task
    //adds task to xExtended_TCB array
    //pdMS_TO_TICKS converts time specified in milliseconds to a time specified in RTOS ticks
    //Task Set #1
    //                           Function   Name      StackSize            Param Prio Handle   Phase             Period           Max Exec Time     Deadline  
    vSchedulerPeriodicTaskCreate(testFunc1, "t1", configMINIMAL_STACK_SIZE, &c1, 1, &xHandle1, pdMS_TO_TICKS(0), pdMS_TO_TICKS(1000), WCET_1, pdMS_TO_TICKS(1000));
    vSchedulerPeriodicTaskCreate(testFunc2, "t2", configMINIMAL_STACK_SIZE, &c2, 2, &xHandle2, pdMS_TO_TICKS(0), pdMS_TO_TICKS(1500), WCET_2, pdMS_TO_TICKS(1500));
    vSchedulerPeriodicTaskCreate(testFunc3, "t3", configMINIMAL_STACK_SIZE, &c3, 3, &xHandle3, pdMS_TO_TICKS(0), pdMS_TO_TICKS(2000), WCET_3, pdMS_TO_TICKS(2000));

    /*Create Mutex Instance*/
    mutex_lock = xSemaphoreCreateMutex();

    if(mutex_lock != NULL)
    {
      Serial.println("Mutex for Resource 1 CREATED");
      Serial.flush();
    }
    else
    {
      Serial.println("Mutex for Resource 1 NOT CREATED");
      Serial.flush();
    }

    //start system
    //start scheduling tasks with specified scheduling policy
    vSchedulerStart();
  
    /* If all is well, the scheduler will now be running, and the following line
      will never be reached. */
    for ( ;; );
}

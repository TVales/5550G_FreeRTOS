#include "scheduler.h"

TaskHandle_t xHandle1 = NULL;
TaskHandle_t xHandle2 = NULL;
TaskHandle_t xHandle3 = NULL;
TaskHandle_t xHandle4 = NULL;

/* Change these WCET depending on what task set you are using
    Task Set 1: Task 1 = 100
                Task 2 = 200
                Task 3 = 150
                Task 4 = 300
    Task Set 2: Task 1 = 100
                Task 2 = 150
                Task 3 = 200
                Task 4 = 150 */
/*Task Set 1*/

TickType_t WCET_1 = pdMS_TO_TICKS(100);
TickType_t WCET_2 = pdMS_TO_TICKS(200);
TickType_t WCET_3 = pdMS_TO_TICKS(150);
TickType_t WCET_4 = pdMS_TO_TICKS(300);


/*Task Set 2*/
/*
TickType_t WCET_1 = pdMS_TO_TICKS(100);
TickType_t WCET_2 = pdMS_TO_TICKS(150);
TickType_t WCET_3 = pdMS_TO_TICKS(200);
TickType_t WCET_4 = pdMS_TO_TICKS(150);
*/

void loop() {}

static void testFunc1( void *pvParameters )
{
  (void) pvParameters;

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
}

static void testFunc2( void *pvParameters )
{
  (void) pvParameters;

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
}

static void testFunc3( void *pvParameters )
{
  (void) pvParameters;
  
  volatile TickType_t i, j;

  i = xTaskGetTickCount();

  while (1)
  {
    j = xTaskGetTickCount();

    if ((j - i) >= WCET_3)
    {
      break;
    }
  }
}

static void testFunc4( void *pvParameters )
{
  (void) pvParameters;

  volatile TickType_t i, j;

  i = xTaskGetTickCount();

  while (1)
  {
    j = xTaskGetTickCount();

    if ((j - i) >= WCET_4)
    {
      break;
    }
  } 
}

int main( void )
{
  char c1 = 'a';
  char c2 = 'b';
  char c3 = 'c';
  char c4 = 'd';

  //1 = Task Set 1
  //2 = task Set 2
  int TaskSet = 1;

  //set up serial comms with board 
  Serial.begin(9600);

  while (!Serial) {
    ;
  }

  //initialize scheduler
  //initializes xExtended_TCB array
  vSchedulerInit();

  switch (TaskSet) {
    case 1:
      //create periodic task
      //adds task to xExtended_TCB array
      //pdMS_TO_TICKS converts time specified in milliseconds to a time specified in RTOS ticks
      //Task Set #1
      //                           Function   Name      StackSize            Param Prio Handle   Phase             Period           Max Exec Time     Deadline   Value
      vSchedulerPeriodicTaskCreate(testFunc1, "t1", configMINIMAL_STACK_SIZE, &c1, 1, &xHandle1, pdMS_TO_TICKS(0), pdMS_TO_TICKS(400), WCET_1, pdMS_TO_TICKS(400));
      vSchedulerPeriodicTaskCreate(testFunc2, "t2", configMINIMAL_STACK_SIZE, &c2, 2, &xHandle2, pdMS_TO_TICKS(0), pdMS_TO_TICKS(700), WCET_2, pdMS_TO_TICKS(700));
      vSchedulerPeriodicTaskCreate(testFunc3, "t3", configMINIMAL_STACK_SIZE, &c3, 3, &xHandle3, pdMS_TO_TICKS(0), pdMS_TO_TICKS(1000), WCET_3, pdMS_TO_TICKS(1000));
      vSchedulerPeriodicTaskCreate(testFunc4, "t4", configMINIMAL_STACK_SIZE, &c4, 4, &xHandle4, pdMS_TO_TICKS(0), pdMS_TO_TICKS(5000), WCET_4, pdMS_TO_TICKS(5000));
      break;

    case 2:
      //Task Set #2
      vSchedulerPeriodicTaskCreate(testFunc1, "t1", configMINIMAL_STACK_SIZE, &c1, 1, &xHandle1, pdMS_TO_TICKS(0), pdMS_TO_TICKS(400), WCET_1, pdMS_TO_TICKS(400));
      vSchedulerPeriodicTaskCreate(testFunc2, "t2", configMINIMAL_STACK_SIZE, &c2, 2, &xHandle2, pdMS_TO_TICKS(0), pdMS_TO_TICKS(200), WCET_2, pdMS_TO_TICKS(200));
      vSchedulerPeriodicTaskCreate(testFunc3, "t3", configMINIMAL_STACK_SIZE, &c3, 3, &xHandle3, pdMS_TO_TICKS(0), pdMS_TO_TICKS(700), WCET_3, pdMS_TO_TICKS(700));
      vSchedulerPeriodicTaskCreate(testFunc4, "t4", configMINIMAL_STACK_SIZE, &c4, 4, &xHandle4, pdMS_TO_TICKS(0), pdMS_TO_TICKS(1000), WCET_4, pdMS_TO_TICKS(1000));
      break;
  }

  //start system
  //start scheduling tasks with specified scheduling policy
  vSchedulerStart();

  /* If all is well, the scheduler will now be running, and the following line
    will never be reached. */
  for ( ;; );
}

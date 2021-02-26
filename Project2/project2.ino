#include "scheduler.h"

TaskHandle_t xHandle1 = NULL;
TaskHandle_t xHandle2 = NULL;
TaskHandle_t xHandle3 = NULL;
TaskHandle_t xHandle4 = NULL;

void loop() {}

static void testFunc1( void *pvParameters )
{
	(void) pvParameters;
	int i,a;
	for( i = 0; i < 5; i++ )
	{
		a = 1 + i*i*i*i;
	}	
  Serial.println("Task 1 DONE");
  Serial.flush();
}

// 1+((a^2)*i)
static void testFunc2( void *pvParameters )
{ 
	(void) pvParameters;	
	int i, a;	
	for(i = 0; i < 5; i++ )
	{
		a = 1 + a * a * i;
	}	
  Serial.println("Task 2 DONE");
  Serial.flush();
}


//my own function
static void testFunc3( void *pvParameters )
{ 
  (void) pvParameters;  
  int i, a; 
  for(i = 0; i < 5; i++ )
  {
    a = 1 + i;
  } 
  Serial.println("Task 3 DONE");
  Serial.flush();
}

//my own function
static void testFunc4( void *pvParameters )
{ 
  (void) pvParameters;  
  int i, a; 
  for(i = 0; i < 10; i++ )
  {
    a = 1 + a * i;
  } 
  Serial.println("Task 4 DONE");
  Serial.flush();
}

int main( void )
{
	char c1 = 'a';
	char c2 = 'b';	
  char c3 = 'c';
  char c4 = 'd';

  Serial.begin(9600);

  while(!Serial) {
    ;
    }

  //initialize scheduler 
  //initializes xExtended_TCB array 
	vSchedulerInit();

  //create periodic task 
  //adds task to xExtended_TCB array
  //pdMS_TO_TICKS converts time specified in milliseconds to a time specified in RTOS ticks
  //                           Function   Name      StackSize            Param Prio Handle   Phase             Period              Max Exec Time       Deadline
	vSchedulerPeriodicTaskCreate(testFunc1, "t1", configMINIMAL_STACK_SIZE, &c1, 1, &xHandle1, pdMS_TO_TICKS(0), pdMS_TO_TICKS(400), pdMS_TO_TICKS(100), pdMS_TO_TICKS(400));
	vSchedulerPeriodicTaskCreate(testFunc2, "t2", configMINIMAL_STACK_SIZE, &c2, 2, &xHandle2, pdMS_TO_TICKS(0), pdMS_TO_TICKS(800), pdMS_TO_TICKS(200), pdMS_TO_TICKS(800));
  vSchedulerPeriodicTaskCreate(testFunc3, "t3", configMINIMAL_STACK_SIZE, &c3, 3, &xHandle3, pdMS_TO_TICKS(0), pdMS_TO_TICKS(1000), pdMS_TO_TICKS(150), pdMS_TO_TICKS(1000));
  vSchedulerPeriodicTaskCreate(testFunc4, "t4", configMINIMAL_STACK_SIZE, &c4, 4, &xHandle4, pdMS_TO_TICKS(0), pdMS_TO_TICKS(5000), pdMS_TO_TICKS(300), pdMS_TO_TICKS(5000));

  //start system 
  //start scheduling tasks with specified scheduling policy
	vSchedulerStart();

	/* If all is well, the scheduler will now be running, and the following line
	will never be reached. */
	for( ;; );
}

#include "scheduler.h"

TaskHandle_t xHandle1 = NULL;
TaskHandle_t xHandle2 = NULL;
 
static void testFunc1( void *pvParameters )
{
	(void) pvParameters;
	int i,a;
	for( i = 0; i < 10000; i++ )
	{
		a = 1 + i*i*i*i;
	}	
}

// 1+((a^2)*i)
static void testFunc2( void *pvParameters )
{ 
	(void) pvParameters;	
	int i, a;	
	for(i = 0; i < 10000; i++ )
	{
		a = 1 + a * a * i;
	}	
}

int main( void )
{
	char c1 = 'a';
	char c2 = 'b';	

  Serial.begin(9600);

  while(!Serial) {
    ;
    }

  //initialize scheduler 
  //initializes xExtended_TCB array 
	vSchedulerInit();

  Serial.println("vSchedulerInit Run");
  Serial.flush();

  //create periodic task 
  //adds task to xExtended_TCB array
	vSchedulerPeriodicTaskCreate(testFunc1, "t1", configMINIMAL_STACK_SIZE, &c1, 1, &xHandle1, pdMS_TO_TICKS(0), pdMS_TO_TICKS(800), pdMS_TO_TICKS(100), pdMS_TO_TICKS(800));
	vSchedulerPeriodicTaskCreate(testFunc2, "t2", configMINIMAL_STACK_SIZE, &c2, 2, &xHandle2, pdMS_TO_TICKS(0), pdMS_TO_TICKS(400), pdMS_TO_TICKS(400), pdMS_TO_TICKS(400));
  Serial.println();
  Serial.flush();

  //start system 
  //start scheduling tasks with specified scheduling policy
	//vSchedulerStart();
  
  //Serial.println("Application Scheduler Start");

	/* If all is well, the scheduler will now be running, and the following line
	will never be reached. */
	for( ;; );
}

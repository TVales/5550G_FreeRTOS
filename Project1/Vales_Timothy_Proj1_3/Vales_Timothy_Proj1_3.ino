#include <Arduino_FreeRTOS.h>

//function headers 
void TaskMsg1(void *pvParameters);
void TaskMsg2 (void *pvParameters);
void TaskBlink (void *pvParameters);

void setup() {
  //set up serial communication - 9600 Baud rate 
  Serial.begin(9600);

  while(!Serial) {
    ;
  }

  //priority of 2
  xTaskCreate(
    TaskMsg1, 
    "Msg1", 
    128, 
    NULL, 
    2, 
    NULL);

    //priority of 3 (highest)
    xTaskCreate(
    TaskMsg2, 
    "Msg2", 
    128, 
    NULL, 
    3, 
    NULL);

    //priority of 1
    xTaskCreate(
    TaskBlink, 
    "Blink", 
    128, 
    NULL, 
    1, 
    NULL);
}

void loop() {
  //Left empty because everything runs in the tasks
}

/* 
 *  Task 1: Display Msg 1: 
 *  Perdiod = 2 seconds 
 *  Priority = 2 (medium priority)
 *  
 *  Task 2 Display Msg 2: 
 *  Period = 3 seconds
 *  Priority = 3 (highest priority)
 *  
 *  Task 3 Blink LED:
 *  Period = 5 seconds 
 *  Priorty = 1 (lowest priority)
*/

//period of 2sec
void TaskMsg1(void *pvParameters) {
  (void) pvParameters;

  for(;;){
    Serial.println("Task 1: TaskMsg1 Executed");
    vTaskDelay(2000/portTICK_PERIOD_MS); //delay for 2 seconds
  }
}

//period of 3sec
void TaskMsg2(void *pvParameters) {
  (void) pvParameters;

  for(;;){
    Serial.println("Task 2: TaskMsg2 Executed");
    vTaskDelay(3000/portTICK_PERIOD_MS); //delay for 3 seconds
  }
}

//period of 5sec for blinking lights
//on for 1sec, off for 4sec
void TaskBlink(void *pvParameters) {
  (void) pvParameters;

  //initialize digital LED_BUILTIN on pin 13 as output
  pinMode(LED_BUILTIN, OUTPUT);  

  for(;;){
    Serial.println("Task 3: TaskBlink ON");
    digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
    vTaskDelay( 1000 / portTICK_PERIOD_MS ); // wait for 1 second 
    Serial.println("Task 3: TaskBlink OFF");
    digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
    vTaskDelay( 4000 / portTICK_PERIOD_MS ); // wait for 4 seconds 
  }
}

#include "scheduler.h"

#define schedUSE_TCB_ARRAY 1

/* Extended Task control block for managing periodic tasks within this library. */
typedef struct xExtended_TCB
{
	TaskFunction_t pvTaskCode; 		/* Function pointer to the code that will be run periodically. */
	const char *pcName; 			/* Name of the task. */
	UBaseType_t uxStackDepth; 			/* Stack size of the task. */
	void *pvParameters; 			/* Parameters to the task function. */
	UBaseType_t uxPriority; 		/* Priority of the task. */
	TaskHandle_t *pxTaskHandle;		/* Task handle for the task. */
	TickType_t xReleaseTime;		/* Release time of the task. */
	TickType_t xRelativeDeadline;	/* Relative deadline of the task. */
	TickType_t xAbsoluteDeadline;	/* Absolute deadline of the task. */
	TickType_t xPeriod;				/* Task period. */
	TickType_t xLastWakeTime; 		/* Last time stamp when the task was running. */
	TickType_t xMaxExecTime;		/* Worst-case execution time of the task. */
	TickType_t xExecTime;			/* Current execution time of the task. */

	BaseType_t xWorkIsDone; 		/* pdFALSE if the job is not finished, pdTRUE if the job is finished. */

	#if( schedUSE_TCB_ARRAY == 1 )
		BaseType_t xPriorityIsSet; 	/* pdTRUE if the priority is assigned. */
		BaseType_t xInUse; 			/* pdFALSE if this extended TCB is empty. */
	#endif

	#if( schedUSE_TIMING_ERROR_DETECTION_DEADLINE == 1 )
		BaseType_t xExecutedOnce;	/* pdTRUE if the task has executed once. */
	#endif /* schedUSE_TIMING_ERROR_DETECTION_DEADLINE */

	#if( schedUSE_TIMING_ERROR_DETECTION_EXECUTION_TIME == 1 || schedUSE_TIMING_ERROR_DETECTION_DEADLINE == 1 )
		TickType_t xAbsoluteUnblockTime; /* The task will be unblocked at this time if it is blocked by the scheduler task. */
	#endif /* schedUSE_TIMING_ERROR_DETECTION_EXECUTION_TIME || schedUSE_TIMING_ERROR_DETECTION_DEADLINE */

	#if( schedUSE_TIMING_ERROR_DETECTION_EXECUTION_TIME == 1 )
		BaseType_t xSuspended; 		/* pdTRUE if the task is suspended. */
		BaseType_t xMaxExecTimeExceeded; /* pdTRUE when execTime exceeds maxExecTime. */
	#endif /* schedUSE_TIMING_ERROR_DETECTION_EXECUTION_TIME */
	
	/* add if you need anything else */	

	#if( schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_HVDF )
		TickType_t xRemainingWCET; /*Set to max exec time and incremented every tick of task. Then, compared when needed.*/
		float xDensity; /* Density = Value / Remaining WCET */
		float xValue; /* Value given to task from task set */
	#endif
	
} SchedTCB_t;



#if( schedUSE_TCB_ARRAY == 1 )
	static BaseType_t prvGetTCBIndexFromHandle( TaskHandle_t xTaskHandle );
	static void prvInitTCBArray( void );
	/* Find index for an empty entry in xTCBArray. Return -1 if there is no empty entry. */
	static BaseType_t prvFindEmptyElementIndexTCB( void );
	/* Remove a pointer to extended TCB from xTCBArray. */
	static void prvDeleteTCBFromArray( BaseType_t xIndex );
#endif /* schedUSE_TCB_ARRAY */

static TickType_t xSystemStartTime = 0;

static void prvPeriodicTaskCode( void *pvParameters );
static void prvCreateAllTasks( void );

#if( schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_EDF || schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_HVDF )
	void prvPrioritySet( void ); /* used by EDF and HVDF */
	static TickType_t prvFindLargestValue ( void ); /* Used by EDF*/
	void prvSetInitialPriority ( void );
#endif

#if( schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_RMS)
	static void prvSetFixedPriorities( void );	
#endif /* schedSCHEDULING_POLICY_RMS */

#if( schedUSE_SCHEDULER_TASK == 1 )
	static void prvSchedulerCheckTimingError( TickType_t xTickCount, SchedTCB_t *pxTCB );
	static void prvSchedulerFunction( void );
	static void prvCreateSchedulerTask( void );
	static void prvWakeScheduler( void );

	#if( schedUSE_TIMING_ERROR_DETECTION_DEADLINE == 1 )
		static void prvPeriodicTaskRecreate( SchedTCB_t *pxTCB );
		static void prvDeadlineMissedHook( SchedTCB_t *pxTCB, TickType_t xTickCount );
		static void prvCheckDeadline( SchedTCB_t *pxTCB, TickType_t xTickCount );				
	#endif /* schedUSE_TIMING_ERROR_DETECTION_DEADLINE */

	#if( schedUSE_TIMING_ERROR_DETECTION_EXECUTION_TIME == 1 )
		static void prvExecTimeExceedHook( TickType_t xTickCount, SchedTCB_t *pxCurrentTask );
	#endif /* schedUSE_TIMING_ERROR_DETECTION_EXECUTION_TIME */
	
#endif /* schedUSE_SCHEDULER_TASK */



#if( schedUSE_TCB_ARRAY == 1 )
	/* Array for extended TCBs. */
	static SchedTCB_t xTCBArray[ schedMAX_NUMBER_OF_PERIODIC_TASKS ] = { 0 };
	/* Counter for number of periodic tasks. */
	static BaseType_t xTaskCounter = 0;
#endif /* schedUSE_TCB_ARRAY */

#if( schedUSE_SCHEDULER_TASK )
	static TickType_t xSchedulerWakeCounter = 0; /* useful. why? */
	static TaskHandle_t xSchedulerHandle = NULL; /* useful. why? */
#endif /* schedUSE_SCHEDULER_TASK */


#if( schedUSE_TCB_ARRAY == 1 )
	/* Returns index position in xTCBArray of TCB with same task handle as parameter. */
	static BaseType_t prvGetTCBIndexFromHandle( TaskHandle_t xTaskHandle )
	{
		static BaseType_t xIndex = 0;
		BaseType_t xIterator;

		for( xIterator = 0; xIterator < schedMAX_NUMBER_OF_PERIODIC_TASKS; xIterator++ )
		{
		
			if( pdTRUE == xTCBArray[ xIndex ].xInUse && *xTCBArray[ xIndex ].pxTaskHandle == xTaskHandle )
			{
				return xIndex;
			}
		
			xIndex++;
			if( schedMAX_NUMBER_OF_PERIODIC_TASKS == xIndex )
			{
				xIndex = 0;
			}
		}
		return -1;
	}

	/* Initializes xTCBArray. */
	static void prvInitTCBArray( void )
	{
	UBaseType_t uxIndex;
		for( uxIndex = 0; uxIndex < schedMAX_NUMBER_OF_PERIODIC_TASKS; uxIndex++)
		{
			xTCBArray[ uxIndex ].xInUse = pdFALSE;
			xTCBArray[uxIndex].pxTaskHandle = NULL;
		}
	}

	/* Find index for an empty entry in xTCBArray. Returns -1 if there is no empty entry. */
	static BaseType_t prvFindEmptyElementIndexTCB( void )
	{
		BaseType_t xIndex;
		for(xIndex=0;xIndex<schedMAX_NUMBER_OF_PERIODIC_TASKS;xIndex++){
			if(!xTCBArray[xIndex].xInUse){
				break;
			}
		}
		if(xIndex == schedMAX_NUMBER_OF_PERIODIC_TASKS){
			xIndex = -1;
		}
		return xIndex;

	}

	/* Remove a pointer to extended TCB from xTCBArray. */
	static void prvDeleteTCBFromArray( BaseType_t xIndex )
	{
		configASSERT(xIndex>=0);
		if(xTCBArray[xIndex].xInUse == pdTRUE){
			xTCBArray[xIndex].xInUse = pdFALSE;
			xTaskCounter--;
		}
	}
	
#endif /* schedUSE_TCB_ARRAY */


/* The whole function code that is executed by every periodic task.
 * This function wraps the task code specified by the user. */
static void prvPeriodicTaskCode( void *pvParameters )
{
 
	SchedTCB_t *pxThisTask;	
	TaskHandle_t xCurrentTaskHandle = xTaskGetCurrentTaskHandle();  
	
    /* your implementation goes here */
    
    /* Check the handle is not NULL. */
    configASSERT(xCurrentTaskHandle != NULL)
    BaseType_t xIndex = prvGetTCBIndexFromHandle(xCurrentTaskHandle);
	if(xIndex < 0){
		Serial.print("Invalid index\n");
		Serial.flush();
	}
	configASSERT(xIndex < 0);
	
	pxThisTask = &xTCBArray[xIndex];
	if( pxThisTask->xReleaseTime != 0){
	    xTaskDelayUntil(&pxThisTask->xLastWakeTime,pxThisTask->xReleaseTime);
	}    
	#if( schedUSE_TIMING_ERROR_DETECTION_DEADLINE == 1 )
        /* your implementation goes here */
        pxThisTask->xExecutedOnce = pdTRUE;
	#endif /* schedUSE_TIMING_ERROR_DETECTION_DEADLINE */
    
	if( 0 == pxThisTask->xReleaseTime )
	{
		pxThisTask->xLastWakeTime = xSystemStartTime;
	}
	
	for( ; ; )
	{	
		/*Deadline Check #2 
		Update priority when a task is released so if it is preempted, the next deadline can still be seen */	
		#if( schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_EDF || schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_HVDF )
			prvPrioritySet();
		#endif 

		Serial.print(pxThisTask->pcName);
		Serial.print(" Start - ");
		Serial.flush();
		Serial.println(xTaskGetTickCount());
		Serial.flush();

		pxThisTask->xWorkIsDone = pdFALSE;
		pxThisTask->pvTaskCode( pvParameters );
		pxThisTask->xWorkIsDone = pdTRUE;
		pxThisTask->xExecTime = 0;  

		#if( schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_HVDF )
		pxThisTask->xRemainingWCET = pxThisTask->xMaxExecTime; /*Set this back to max value at the end so next time it runs it is at max value */
		#endif

		pxThisTask->xAbsoluteDeadline = pxThisTask->xLastWakeTime + pxThisTask->xRelativeDeadline;

		#if( schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_EDF || schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_HVDF )
			prvPrioritySet();
		#endif 
		
        
		xTaskDelayUntil(&pxThisTask->xLastWakeTime, pxThisTask->xPeriod);  
	}
}

/* Creates a periodic task. */
void vSchedulerPeriodicTaskCreate( TaskFunction_t pvTaskCode, const char *pcName, UBaseType_t uxStackDepth, void *pvParameters, UBaseType_t uxPriority,
		TaskHandle_t *pxCreatedTask, TickType_t xPhaseTick, TickType_t xPeriodTick, TickType_t xMaxExecTimeTick, TickType_t xDeadlineTick, float xTaskValue )
{
	taskENTER_CRITICAL();
	SchedTCB_t *pxNewTCB;
	
	#if( schedUSE_TCB_ARRAY == 1 )
		BaseType_t xIndex = prvFindEmptyElementIndexTCB();
		configASSERT( xTaskCounter < schedMAX_NUMBER_OF_PERIODIC_TASKS );
		configASSERT( xIndex != -1 );
		pxNewTCB = &xTCBArray[ xIndex ];	
	#endif /* schedUSE_TCB_ARRAY */

	/* Intialize item. */
		
	pxNewTCB->pvTaskCode = pvTaskCode;
	pxNewTCB->pcName = pcName;
	pxNewTCB->uxStackDepth = uxStackDepth;
	pxNewTCB->pvParameters = pvParameters;
	pxNewTCB->uxPriority = uxPriority;
	pxNewTCB->pxTaskHandle = pxCreatedTask;
	pxNewTCB->xReleaseTime = xPhaseTick;
	pxNewTCB->xPeriod = xPeriodTick;
	
    pxNewTCB->xRelativeDeadline = xDeadlineTick;
    pxNewTCB->xAbsoluteDeadline = pxNewTCB->xReleaseTime + xSystemStartTime + pxNewTCB->xRelativeDeadline;
	pxNewTCB->xWorkIsDone = pdFALSE;
	pxNewTCB->xExecTime = 0;
	pxNewTCB->xMaxExecTime = xMaxExecTimeTick;

	#if( schedUSE_TCB_ARRAY == 1 )
		pxNewTCB->xInUse = pdTRUE;
	#endif /* schedUSE_TCB_ARRAY */
	
	#if( schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_RMS )
		/* member initialization */
		pxNewTCB->xPriorityIsSet = pdFALSE;
	#endif /* schedSCHEDULING_POLICY */

	#if( schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_HVDF )
		pxNewTCB->xRemainingWCET = xMaxExecTimeTick;
		pxNewTCB->xValue = xTaskValue;
		pxNewTCB->xDensity = pxNewTCB->xValue / pxNewTCB->xRemainingWCET;
	#endif
	
	#if( schedUSE_TIMING_ERROR_DETECTION_DEADLINE == 1 )
		/* member initialization */
		pxNewTCB->xExecutedOnce = pdFALSE;
	#endif /* schedUSE_TIMING_ERROR_DETECTION_DEADLINE */
	
	#if( schedUSE_TIMING_ERROR_DETECTION_EXECUTION_TIME == 1 )
        pxNewTCB->xSuspended = pdFALSE;
        pxNewTCB->xMaxExecTimeExceeded = pdFALSE;
	#endif /* schedUSE_TIMING_ERROR_DETECTION_EXECUTION_TIME */	

	#if( schedUSE_TCB_ARRAY == 1 )
		xTaskCounter++;	
	#endif /* schedUSE_TCB_SORTED_LIST */
	taskEXIT_CRITICAL();
  /*Serial.println(pxNewTCB->xDensity);
  Serial.flush();*/
}

/* Deletes a periodic task. */
void vSchedulerPeriodicTaskDelete( TaskHandle_t xTaskHandle )
{
	BaseType_t xIndex = prvGetTCBIndexFromHandle(xTaskHandle);
	prvDeleteTCBFromArray(xIndex);
	
	vTaskDelete( xTaskHandle );
}

/* Creates all periodic tasks stored in TCB array, or TCB list. */
static void prvCreateAllTasks( void )
{
	SchedTCB_t *pxTCB;

	#if( schedUSE_TCB_ARRAY == 1 )
		BaseType_t xIndex;
		for( xIndex = 0; xIndex < xTaskCounter; xIndex++ )
		{
			configASSERT( pdTRUE == xTCBArray[ xIndex ].xInUse );
			pxTCB = &xTCBArray[ xIndex ];
			BaseType_t xReturnValue = xTaskCreate(prvPeriodicTaskCode,pxTCB->pcName,pxTCB->uxStackDepth,pxTCB->pvParameters,pxTCB->uxPriority,pxTCB->pxTaskHandle);
			if(xReturnValue == pdPASS) {
				Serial.print(pxTCB->pcName);
				Serial.print(", Period- ");
				Serial.print(pxTCB->xPeriod);
				Serial.print(", Released at- ");
				Serial.print(pxTCB->xReleaseTime);
				Serial.print(", Priority- ");
				Serial.print(pxTCB->uxPriority); 
				Serial.print(", WCET- ");
				Serial.print(pxTCB->xMaxExecTime);
				Serial.print(", Deadline- ");
				Serial.print(pxTCB->xRelativeDeadline);
				Serial.println();
				Serial.flush();
			}
			else
			{
				Serial.println("Task creation failed\n");
				Serial.flush();
			}
		}	
	#endif /* schedUSE_TCB_ARRAY */
}

#if( schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_RMS )
	/* Initiazes fixed priorities of all periodic tasks with respect to RMS policy. */
static void prvSetFixedPriorities( void )
{
	BaseType_t xIter, xIndex;
	TickType_t xShortest, xPreviousShortest=0;
	SchedTCB_t *pxShortestTaskPointer, *pxTCB;

	#if( schedUSE_SCHEDULER_TASK == 1 )
		BaseType_t xHighestPriority = schedSCHEDULER_PRIORITY; 
		xHighestPriority--;
	#else
		BaseType_t xHighestPriority = configMAX_PRIORITIES;
	#endif /* schedUSE_SCHEDULER_TASK */

	for( xIter = 0; xIter < xTaskCounter; xIter++ )
	{
		xShortest = portMAX_DELAY;

		/* search for shortest period */
		for( xIndex = 0; xIndex < xTaskCounter; xIndex++ )
		{
			/* your implementation goes here */
			#if( schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_RMS )
				if ((xTCBArray[xIndex].xInUse == pdTRUE) && (xTCBArray[xIndex].xPriorityIsSet == pdFALSE) && (xTCBArray[xIndex].xPeriod < xShortest)){
					xShortest = xTCBArray[ xIndex ].xPeriod;
					pxShortestTaskPointer = &xTCBArray[xIndex];
				}
			#endif /* schedSCHEDULING_POLICY */
		}
		
		/* set highest priority to task with xShortest period (the highest priority is configMAX_PRIORITIES-1) */		
		
		/* your implementation goes here */
	pxShortestTaskPointer->uxPriority = xHighestPriority;
	if(xHighestPriority > 0){
		xHighestPriority--;
	}else{
		xHighestPriority = 0;
	}
	
	pxShortestTaskPointer->xPriorityIsSet = pdTRUE;
	}
}
#endif /* schedSCHEDULING_POLICY */


#if( schedUSE_TIMING_ERROR_DETECTION_DEADLINE == 1 )

	/* Recreates a deleted task that still has its information left in the task array (or list). */
	static void prvPeriodicTaskRecreate( SchedTCB_t *pxTCB )
	{
		BaseType_t xReturnValue = xTaskCreate(prvPeriodicTaskCode,pxTCB->pcName,pxTCB->uxStackDepth,pxTCB->pvParameters,pxTCB->uxPriority,pxTCB->pxTaskHandle);
				                      		
		if( pdPASS == xReturnValue )
		{
			#if( schedUSE_TCB_ARRAY == 1 )
				pxTCB->xInUse = pdTRUE;
			#endif /* schedUSE_TCB_ARRAY */
    		#if( schedUSE_TIMING_ERROR_DETECTION_DEADLINE == 1 )
				pxTCB->xExecutedOnce = pdFALSE;
			#endif /* schedUSE_TIMING_ERROR_DETECTION_DEADLINE */
			#if( schedUSE_TIMING_ERROR_DETECTION_EXECUTION_TIME == 1 )
        		pxTCB->xSuspended = pdFALSE;
        		pxTCB->xMaxExecTimeExceeded = pdFALSE;
			#endif /* schedUSE_TIMING_ERROR_DETECTION_EXECUTION_TIME */	
		}
		else
		{
			Serial.println("Task Recreation failed");
			Serial.flush();
		}
	}

	/* Called when a deadline of a periodic task is missed.
	 * Deletes the periodic task that has missed it's deadline and recreate it.
	 * The periodic task is released during next period. */
	static void prvDeadlineMissedHook( SchedTCB_t *pxTCB, TickType_t xTickCount )
	{
		Serial.print("Deadline missed - ");
		Serial.print(pxTCB->pcName);
		Serial.print(" - ");
		Serial.println(xTaskGetTickCount());
		Serial.flush();
		/* Delete the pxTask and recreate it. */
		vTaskDelete( *pxTCB->pxTaskHandle );
		pxTCB->xWorkIsDone = pdFALSE;
		pxTCB->xExecTime = 0;
		prvPeriodicTaskRecreate( pxTCB );	
		
		#if( schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_EDF || schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_HVDF ) 
			prvPrioritySet(); 
		#endif

		pxTCB->xReleaseTime = pxTCB->xLastWakeTime + pxTCB->xPeriod;
		pxTCB->xLastWakeTime = xTickCount;
		pxTCB->xAbsoluteDeadline = pxTCB->xRelativeDeadline + pxTCB->xReleaseTime;

		#if( schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_HVDF ) 
		pxTCB->xRemainingWCET = pxTCB->xMaxExecTime;
		#endif
	}

	/* Checks whether given task has missed deadline or not. */
	/* Absolute deadlines of all tasks are being updated here. If the task has not run during the period of the scheduler
	the abs deadline of the task will be updated here */
	static void prvCheckDeadline( SchedTCB_t *pxTCB, TickType_t xTickCount )
	{ 
		/* check whether deadline is missed. */     		
		/* your implementation goes here */

		if((pxTCB->xWorkIsDone==pdFALSE)&&(pxTCB->xExecutedOnce==pdTRUE)){ 
			pxTCB->xAbsoluteDeadline = pxTCB->xLastWakeTime + pxTCB->xRelativeDeadline; 
			if(pxTCB->xAbsoluteDeadline < xTickCount){ 
				prvDeadlineMissedHook(pxTCB, xTickCount); 
			} 
		} 
		#if( schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_EDF || schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_HVDF ) 
			prvPrioritySet(); 
		#endif 
	} 
#endif /* schedUSE_TIMING_ERROR_DETECTION_DEADLINE */


#if( schedUSE_TIMING_ERROR_DETECTION_EXECUTION_TIME == 1 )

	/* Called if a periodic task has exceeded its worst-case execution time.
	 * The periodic task is blocked until next period. A context switch to
	 * the scheduler task occur to block the periodic task. */
	static void prvExecTimeExceedHook( TickType_t xTickCount, SchedTCB_t *pxCurrentTask )
	{
        Serial.print(pxCurrentTask->pcName);
        Serial.print(" Exceeded - ");
		Serial.println(xTaskGetTickCount());
        Serial.flush();
        pxCurrentTask->xMaxExecTimeExceeded = pdTRUE;
        /* Is not suspended yet, but will be suspended by the scheduler later. */
        pxCurrentTask->xSuspended = pdTRUE;
        pxCurrentTask->xAbsoluteUnblockTime = pxCurrentTask->xLastWakeTime + pxCurrentTask->xPeriod;
        pxCurrentTask->xExecTime = 0;
        
        BaseType_t xHigherPriorityTaskWoken;
        vTaskNotifyGiveFromISR( xSchedulerHandle, &xHigherPriorityTaskWoken );
        xTaskResumeFromISR(xSchedulerHandle);
	}
#endif /* schedUSE_TIMING_ERROR_DETECTION_EXECUTION_TIME */


#if( schedUSE_SCHEDULER_TASK == 1 )
	/* Called by the scheduler task. Checks all tasks for any enabled
	 * Timing Error Detection feature. */
	static void prvSchedulerCheckTimingError( TickType_t xTickCount, SchedTCB_t *pxTCB )
	{
		/* your implementation goes here */
    

		#if( schedUSE_TIMING_ERROR_DETECTION_DEADLINE == 1 )						
			/* check if task missed deadline */
            /* your implementation goes here */
        if(xTickCount - (pxTCB->xLastWakeTime) > 0){                
        	pxTCB->xWorkIsDone = pdFALSE;
        }
			prvCheckDeadline( pxTCB, xTickCount );						
		#endif /* schedUSE_TIMING_ERROR_DETECTION_DEADLINE */
		

		#if( schedUSE_TIMING_ERROR_DETECTION_EXECUTION_TIME == 1 )
        if( pdTRUE == pxTCB->xMaxExecTimeExceeded )
        {
            pxTCB->xMaxExecTimeExceeded = pdFALSE;
            Serial.print(pxTCB->pcName);
            Serial.print(" suspended - ");
            Serial.print(xTaskGetTickCount());
            Serial.print("\n");
            Serial.flush();
            vTaskSuspend( *pxTCB->pxTaskHandle );
        }
        if( pdTRUE == pxTCB->xSuspended )
        {
            if( ( signed ) ( pxTCB->xAbsoluteUnblockTime - xTickCount ) <= 0 )
            {
                pxTCB->xSuspended = pdFALSE;
                pxTCB->xLastWakeTime = xTickCount;
                Serial.print(pxTCB->pcName);
                Serial.print(" resumed - ");
                Serial.print(xTaskGetTickCount());
            	Serial.print("\n");
                Serial.flush();
                vTaskResume( *pxTCB->pxTaskHandle );
            }
        }
		#endif /* schedUSE_TIMING_ERROR_DETECTION_EXECUTION_TIME */

		return;
	}

	/* Function code for the scheduler task. */
	static void prvSchedulerFunction( void *pvParameters )
	{

		for( ; ; )
		{ 
			/*Deadline Check #1 
			Update Priority during scheduler wakes*/
     		#if( schedUSE_TIMING_ERROR_DETECTION_DEADLINE == 1 || schedUSE_TIMING_ERROR_DETECTION_EXECUTION_TIME == 1 )
				TickType_t xTickCount = xTaskGetTickCount();
				SchedTCB_t *pxTCB;
        		for(BaseType_t xIndex=0;xIndex<xTaskCounter;xIndex++){
        			pxTCB = &xTCBArray[xIndex];
        			if ((pxTCB) && (pxTCB->xInUse == pdTRUE)&&(pxTCB->pxTaskHandle != NULL)) 
					{
                    	prvSchedulerCheckTimingError( xTickCount, pxTCB );
                    } 
              	}
			
			#endif /* schedUSE_TIMING_ERROR_DETECTION_DEADLINE || schedUSE_TIMING_ERROR_DETECTION_EXECUTION_TIME */

			ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
		}
	}

	/* Creates the scheduler task. */
	static void prvCreateSchedulerTask( void )
	{
		xTaskCreate( (TaskFunction_t) prvSchedulerFunction, "Scheduler", schedSCHEDULER_TASK_STACK_SIZE, NULL, schedSCHEDULER_PRIORITY, &xSchedulerHandle );	
	}
#endif /* schedUSE_SCHEDULER_TASK */


#if( schedUSE_SCHEDULER_TASK == 1 )
	/* Wakes up (context switches to) the scheduler task. */
	static void prvWakeScheduler( void )
	{
		BaseType_t xHigherPriorityTaskWoken;
		vTaskNotifyGiveFromISR( xSchedulerHandle, &xHigherPriorityTaskWoken );
		xTaskResumeFromISR(xSchedulerHandle);    
	}

	/* Called every software tick. */
	// In FreeRTOSConfig.h,
	// Enable configUSE_TICK_HOOK
	// Enable INCLUDE_xTaskGetIdleTaskHandle
	// Enable INCLUDE_xTaskGetCurrentTaskHandle
	
	void vApplicationTickHook( void )
	{            
		SchedTCB_t *pxCurrentTask;		
		TaskHandle_t xCurrentTaskHandle = xTaskGetCurrentTaskHandle();		
        UBaseType_t flag = 0;
        BaseType_t xIndex;
		BaseType_t prioCurrentTask = uxTaskPriorityGet(xCurrentTaskHandle);

		for(xIndex = 0; xIndex < xTaskCounter ; xIndex++){
			pxCurrentTask = &xTCBArray[xIndex];
			if(pxCurrentTask -> uxPriority == prioCurrentTask){
				flag = 1;
				break;
			}
		}
    
		if( xCurrentTaskHandle != xSchedulerHandle && xCurrentTaskHandle != xTaskGetIdleTaskHandle() && flag == 1)
		{
			pxCurrentTask->xExecTime++;

			#if( schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_HVDF )
				pxCurrentTask->xRemainingWCET--;     
				pxCurrentTask->xDensity = pxCurrentTask->xValue / pxCurrentTask->xRemainingWCET;
			#endif
     
			#if( schedUSE_TIMING_ERROR_DETECTION_EXECUTION_TIME == 1 )
				if( pxCurrentTask->xMaxExecTime + 100 < pxCurrentTask->xExecTime )
				{
					if( pdFALSE == pxCurrentTask->xMaxExecTimeExceeded )
					{
						if( pdFALSE == pxCurrentTask->xSuspended )
						{
							prvExecTimeExceedHook( xTaskGetTickCountFromISR(), pxCurrentTask );
						}
					}
				}
			#endif /* schedUSE_TIMING_ERROR_DETECTION_EXECUTION_TIME */
		}

		#if( schedUSE_TIMING_ERROR_DETECTION_DEADLINE == 1 )    
			xSchedulerWakeCounter++;      
			if( xSchedulerWakeCounter == schedSCHEDULER_TASK_PERIOD )
			{
				xSchedulerWakeCounter = 0;        
				prvWakeScheduler();
			}
		#endif /* schedUSE_TIMING_ERROR_DETECTION_DEADLINE */
	}
#endif /* schedUSE_SCHEDULER_TASK */

/* This function must be called before any other function call from this module. */
void vSchedulerInit( void )
{
	#if( schedUSE_TCB_ARRAY == 1 )
		prvInitTCBArray();
	#endif /* schedUSE_TCB_ARRAY */
}

/* Check all the deadlines first and assign priority accordingly
	All deadlines start at 1
	lowest deadline gets assigned a high priority 

	Search for smallest absolute deadline in unsorted array */
#if( schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_EDF || schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_HVDF )
	void prvPrioritySet( void )
	{
		#if( schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_EDF )
			/* Set smallest abs deadline to first item in xTCB */
			TickType_t xSmallestAbsDeadline = prvFindLargestValue();
			/* Holds index of smallest abs deadline in xTCB */
			BaseType_t xIndexOfSmallestAbsDeadline = 0;
		#endif

		/* Highest priority to set task to */
		BaseType_t xHighestPrio = 4;

		#if( schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_HVDF )
			/*highest density can start at 0 because density cannot be negative */
			float xHighestDensity = 0.0; 
			/* Index of highest density task */
			BaseType_t xIndexOfHighestDensity = 0; 
		#endif

		for(BaseType_t xIndex = 0; xIndex < xTaskCounter; xIndex++)
		{
			/*set all tasks to 1 priority to make make sure that they are starting from the same place*/
			vTaskPrioritySet(*xTCBArray[xIndex].pxTaskHandle, 1);

			/* if there's a deadline smaller than the largest absolute deadline in the array (so it scales 
			with the tick count), set smallest to that one and take index of it. 
			Keep going until the end of the xTCBArray */ 
			#if( schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_EDF )
				if((xTCBArray[xIndex].xAbsoluteDeadline <= xSmallestAbsDeadline))
				{
					xSmallestAbsDeadline = xTCBArray[xIndex].xAbsoluteDeadline;
					xIndexOfSmallestAbsDeadline = xIndex;
				} 
			#endif

			#if( schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_HVDF )
				if((xTCBArray[xIndex].xDensity >= xHighestDensity)) /*&& (xTCBArray[xIndex].xWorkIsDone == pdFALSE))*/
				{
					xHighestDensity = xTCBArray[xIndex].xDensity;
					xIndexOfHighestDensity = xIndex;
				} 
			#endif
		}

		/*Set task at index with smallest abs deadline to highest priority */
		#if( schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_EDF )
			vTaskPrioritySet(*xTCBArray[xIndexOfSmallestAbsDeadline].pxTaskHandle, xHighestPrio);
			/*Serial.print(xTCBArray[xIndexOfSmallestAbsDeadline].pcName); 
			Serial.flush();*/
		#endif

		#if( schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_HVDF )
			vTaskPrioritySet(*xTCBArray[xIndexOfHighestDensity].pxTaskHandle, xHighestPrio);
			/*Serial.print(xTCBArray[xIndexOfHighestDensity].pcName); 
			Serial.flush();*/
		#endif
		
	}
	
	static TickType_t prvFindLargestValue ( void )
	{
		TickType_t xLargest = 0;

		for( BaseType_t xIndex = 0; xIndex < xTaskCounter; xIndex++ )
		{
			if(xTCBArray[xIndex].xAbsoluteDeadline > xLargest && xTCBArray[xIndex].xWorkIsDone == pdFALSE)
			{
				xLargest = xTCBArray[xIndex].xAbsoluteDeadline; 
			}
		}

		return xLargest;
	}

	/* sets the priority for the tas that will go first. 
	i.e. i EDF and t1 has deadline of 3 and t2 has deadline of 6, t1 will be given highest priority*/
	void prvSetInitialPriority ( void )
	{
		float xLargestDensity = 0.0; 
		TickType_t xLargestAbsDeadline = 5000; /* initially set to a very high value so that no task can be above it */
		BaseType_t xIndexOfLargest = 0;

		for( BaseType_t xIndex = 0; xIndex < xTaskCounter; xIndex++ )
		{
			xTCBArray[xIndex].uxPriority = 1;

			#if( schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_EDF )
				if(xTCBArray[xIndex].xAbsoluteDeadline < xLargestAbsDeadline)
				{
					xLargestAbsDeadline = xTCBArray[xIndex].xAbsoluteDeadline; 
					xIndexOfLargest = xIndex;
				}
			#endif

			#if( schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_HVDF )
				if(xTCBArray[xIndex].xDensity >= xLargestDensity)
				{
					xLargestDensity = xTCBArray[xIndex].xDensity; 
					xIndexOfLargest = xIndex;
				}
			#endif
		}

		xTCBArray[xIndexOfLargest].uxPriority = 4;
	}
#endif

/* Starts scheduling tasks. All periodic tasks (including polling server) must
 * have been created with API function before calling this function. */
void vSchedulerStart( void )
{
	#if( schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_RMS )
		prvSetFixedPriorities();	
	#endif /* schedSCHEDULING_POLICY */

	#if( schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_EDF || schedSCHEDULING_POLICY == schedSCHEDULING_POLICY_HVDF )
		/* Before tasks are created, sort them into their priority based off initial absolute deadlines */
		prvSetInitialPriority();
	#endif /* schedSCHEDULING_POLICY */

	#if( schedUSE_SCHEDULER_TASK == 1 )
		prvCreateSchedulerTask();
	#endif /* schedUSE_SCHEDULER_TASK */

	prvCreateAllTasks();
	
	xSystemStartTime = xTaskGetTickCount();
	
	vTaskStartScheduler();
}

/*
for(BaseType_t xIndex = 0; xIndex < xTaskCounter; xIndex++)
{
	Serial.println();
	Serial.flush();
}

Serial.println();
Serial.flush(); 

for(BaseType_t xIndex = 0; xIndex < xTaskCounter; xIndex++)
{
	Serial.print(uxTaskPriorityGet(*xTCBArray[xIndex].pxTaskHandle));
	Serial.flush();
}

1) Assign priorities 4, 3, 2, 1 in Freertos scheduler according to which deadline is closest(only onethat matters is highest priority)
2) Create tasks with given priorities in step 1 
3) Start scheduler 
	T1 prio = 4
4) Task 1 starts 
5) Task 1 Finishes 
6) Look through all the rest of the tasks that have not done their jobs and choose which task is next

*/
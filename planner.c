/**
 * Program skeleton for the course "Programming embedded systems"
 *
 * Lab 1: the elevator control system
 */

/**
 * The planner module, which is responsible for consuming
 * pin/key events, and for deciding where the elevator
 * should go next
 */

#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>

#include "global.h"
#include "planner.h"
#include "assert.h"

int i = 0;

static void plannerTask(void *params) {

  // ...
	printf("Planner Task started \n");
	/*for(i=0;i<2;i++){
	setCarTargetPosition(400);
	}*/
	//vTaskDelay(1000);
	
	vTaskDelay(portMAX_DELAY);
  

}

void setupPlanner(unsigned portBASE_TYPE uxPriority) {
  xTaskCreate(plannerTask, "planner", 200, NULL, uxPriority, NULL);
}

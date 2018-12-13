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
PinEvent *event;

static void plannerTask(void *params) {

  // ...
	//printf("Planner Task started \n");
	xQueueReceive(pinEventQueue, event, portMAX_DELAY);
	// printf("Event recieved: %s\n", event_str(*event));
	
	// Make decision from event 
	// TO_FLOOR_1
	// TO_FLOOR_2
	// TO_FLOOR_3
	// ARRIVED_AT_FLOOR
	// LEFT_FLOOR
  // DOORS_CLOSED
	// DOORS_OPENING
  // STOP_PRESSED
	// STOP_RELEASED
	
	
	vTaskDelay(portMAX_DELAY);
  

}

void setupPlanner(unsigned portBASE_TYPE uxPriority) {
  xTaskCreate(plannerTask, "planner", 200, NULL, uxPriority, NULL);
}

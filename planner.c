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

#define PLANNER_POLL (10/portTICK_RATE_MS)
int i = 0;
PinEvent event;
static portTickType xLastWakeTime;

int stop_state, is_target_set = 0;
int floor_arr[] = {0,0,0};

void addFloor (int floor) {
	// check if floor is already called
	for( i = 0; i < 3; i++ ){
		if(floor_arr[i] == floor) {
			return;
		}
	}
	//check special case for floor2
	if(floor == 2){	
		// check that elevator is not too close to floor 2 
		// e.g.:  {3,1,0} -> {2,3,1}
		if( (floor_arr[0] == 3 && getCarPosition() <= (FLOOR_1_POS + FLOOR_2_POS)/2) || 
			(floor_arr[0] == 1 && getCarPosition() >= ((FLOOR_3_POS + FLOOR_2_POS)/2)) ) {
			// add floor 2 as first target 
			floor_arr[2] = floor_arr[1];
			floor_arr[1] = floor_arr[0];
			floor_arr[0] = floor;
			is_target_set = 0;
			return;
		} else {
			// else add floor 2 as second target 
			// e.g.:  {3,1,0} -> {3,2,1}
			floor_arr[2] = floor_arr[1];
			floor_arr[1] = floor;
			is_target_set = 0;
			return;
		} 
	}
	
	//for floor 1 and 3
	for( i = 0; i < 3; i++){
		if(floor_arr[i] == 0){
			floor_arr[i] = floor;
			is_target_set = 0;
			return;
		}
	} 
	
	
}

void removeFloor (int Floor) {

}

static void plannerTask(void *params) {
	xLastWakeTime = xTaskGetTickCount();
	for(;;){
		xQueueReceive(pinEventQueue, &event, portMAX_DELAY);
		printf("Event recieved: %s \n", event_str(event));
		switch( event ) {
			case(STOP_PRESSED) : stop_state = 1; setCarMotorStopped(1); break;
			case(STOP_RELEASED): stop_state = 0; break;
			case(TO_FLOOR_1): addFloor(0); break;
			case(TO_FLOOR_2): addFloor(1); break;
			case(TO_FLOOR_3): addFloor(2); break;
			// need to add other events
		}
		
		if(stop_state != 1 && is_target_set != 1){
			switch(floor_arr[0]) {
				case(1) : 
					vTaskDelay(1000/portTICK_RATE_MS);
					setCarTargetPosition(TO_FLOOR_1);
					is_target_set = 1;
					break;
				case(2) : 
					vTaskDelay(1000/portTICK_RATE_MS);
					setCarTargetPosition(TO_FLOOR_2);
					is_target_set = 1;
					break;
				case(3) : 
					vTaskDelay(1000/portTICK_RATE_MS);
					setCarTargetPosition(TO_FLOOR_3);
					is_target_set = 1;
					break;
			}		
		}
		vTaskDelayUntil(&xLastWakeTime, PLANNER_POLL);
	}

}

void setupPlanner(unsigned portBASE_TYPE uxPriority) {
  xTaskCreate(plannerTask, "planner", 200, NULL, uxPriority, NULL);
}

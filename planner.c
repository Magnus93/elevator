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

#include "stm32f10x_tim.h"

#include "global.h"
#include "planner.h"
#include "assert.h"

#define PLANNER_POLL_MS (10/portTICK_RATE_MS)
#define WAIT_FLOOR_MS (1000/portTICK_RATE_MS)
#define WAIT_EVENT_MS (10/portTICK_RATE_MS)



PinEvent event; 

s32 FLOOR_LEVELS[3] = {FLOOR_1_POS, FLOOR_2_POS, FLOOR_3_POS};

static portTickType xLastWakeTime;
static portTickType xStoppedAt; // Stores time when elevator stops at any floor
static portTickType xTimeAtFloor; // Elevator has stopped for this amount of time at a floor

int is_stopped; // Set to 1 if elevator is to be stopped
int is_target_set = 0; // Set to 1 if position target is set
int floor_order[] = {0,0,0}; // Stores floor order of execution (i=0 is current order)
int i = 0;  // General Iteration Variable 
int is_arriving_at_floor = 0; 	// Set to the floor that is being arrived at

/* Adds 'floor' to the floor order 
* 	PRE: 	FLOOR is 1, 2 or 3 
*/
void addFloor (int floor) {
	// checks if floor is already called if not add it to the first empty order
	for( i = 0; i < 3; i++ ) {
		if(floor_order[i] == floor) {
			return;
		}else if(floor_order[i] == 0){
			floor_order[i] = floor;
			is_target_set = 0;
			return;
		}
	}
	// checks if floor 2 call should be prioritized as the current order
	if(floor == 2){	
		// if current position is not too close to floor 2 then add it to the current order
		// e.g.:  {3,1,0} -> {2,3,1}
		// 
		if( (floor_order[0] == 3 && getCarPosition() <= (FLOOR_1_POS + FLOOR_2_POS)/2) || 
			(floor_order[0] == 1 && getCarPosition() >= ((FLOOR_3_POS + FLOOR_2_POS)/2)) ) {
			floor_order[2] = floor_order[1];
			floor_order[1] = floor_order[0];
			floor_order[0] = floor;
			is_target_set = 0;
			return;
		} else {
			// else add floor 2 as second order 
			// e.g.:  {3,1,0} -> {3,2,1}
			floor_order[2] = floor_order[1];
			floor_order[1] = floor;
			is_target_set = 0;
			return;
		} 
	}
}

void print_floor_order() {
	printf("{");
	for (i = 0; i < 3; i++) {
		printf(" %d ", floor_order[i]);
	}
	printf("}\n");
}


/* Removes current order */
void removeFloor () {
	floor_order[0] = floor_order[1];
	floor_order[1] = floor_order[2];
	floor_order[2] = 0;
	is_target_set = 0;
}

static void plannerTask(void *params) {
	xLastWakeTime = xTaskGetTickCount();
	for(;;){
		if(xQueueReceive(pinEventQueue, &event, WAIT_EVENT_MS) == pdPASS){
			printf("Event recieved: %s, pos: %lu\n", event_str(event), getCarPosition());
			switch( event ) {
				case(STOP_PRESSED) : 
					is_stopped = 1; setCarMotorStopped(1); break;
				case(STOP_RELEASED): 
					is_stopped = 0; 
					setCarMotorStopped(0); 
					is_target_set = 0; 
					break;
				case(TO_FLOOR_1): addFloor(1); print_floor_order(); break;
				case(TO_FLOOR_2): addFloor(2); print_floor_order(); break;
				case(TO_FLOOR_3): addFloor(3); print_floor_order(); break;
				case(ARRIVED_AT_FLOOR): 
					if (floor_order[0] != 0 && 
					FLOOR_LEVELS[floor_order[0]-1] - 1 <= getCarPosition() && 
					getCarPosition() <= FLOOR_LEVELS[floor_order[0]-1] + 1) {
							is_arriving_at_floor = 1;
					} break;
				// need to add other events?
			}
		}
		
		// if floor reached , start counter (xStoppedAt)
		if (MOTOR_STOPPED && is_arriving_at_floor) {
			removeFloor();
			print_floor_order();
			xStoppedAt = xTaskGetTickCount();
			is_arriving_at_floor = 0;
		}
		
		// get counter value and proceed if greater than 1s 
		xTimeAtFloor = xTaskGetTickCount()-xStoppedAt; 
		if(is_stopped != 1 && is_target_set != 1 && xTimeAtFloor/portTICK_RATE_MS > WAIT_FLOOR_MS) {
			printf("Time at floor %lu \n", xTimeAtFloor);
			if (floor_order[0] != 0) {
				setCarTargetPosition(FLOOR_LEVELS[floor_order[0]-1]);
				is_target_set = 1;
			}		
		}
		vTaskDelayUntil(&xLastWakeTime, PLANNER_POLL_MS);
	}

}

void setupPlanner(unsigned portBASE_TYPE uxPriority) {
  xTaskCreate(plannerTask, "planner", 200, NULL, uxPriority, NULL);
}

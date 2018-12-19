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
int doors_closed;			// 1 is doors are closed, 0 if doors open 

/*
* Add floor 2 to floor order 
*/
void addFloor2() {
	// check if array is empty , add it to first
	// case {0,0,0}
	if( floor_order[0] == 0) {
		floor_order[0] = 2;
		is_target_set = 0;
		return;
	}
	// case {3,1,0} or {1,3,0}
	// checks if floor 2 call should be prioritized as the current order
	// if current position is not too close to floor 2 then add it to the current order
	// e.g.:  {3,1,0} -> {2,3,1}
	// e.g.: 	{3,0,0} => {2,3,0} 
	if( (floor_order[0] == 3 && getCarPosition() <= (FLOOR_1_POS + FLOOR_2_POS)/2) ||   /* | 3 -------- 2 ----<-E- 1 | or */
		(floor_order[0] == 1 && getCarPosition() >= ((FLOOR_3_POS + FLOOR_2_POS)/2)) ) {  /* | 3 --E->--- 2 -------- 1 |  */
			floor_order[2] = floor_order[1];
			floor_order[1] = floor_order[0];
			floor_order[0] = 2;
			is_target_set = 0;
			return;
	} else {
		/* | 3 ---<-E-- 2 -------- 1 | or */
		// else add floor 2 as second order 
		// e.g.:  {3,1,0} -> {3,2,1}
		floor_order[2] = floor_order[1];
		floor_order[1] = 2;
		is_target_set = 0;
		return;
	} 
}

/* Adds 'floor' to the floor order 
* 	PRE: 	FLOOR is 1, 2 or 3 
		OBS!!! - look at snip
			current:		{3,1,0} -> {3,1,2}
			should be:	{3,1,0} -> {3,2,1}
*/
void addFloor (int floor) {
	// checks if floor is already called 
	for (i = 0; i < 3; i++ ) {
		if (floor_order[i] == floor) {
			return;
		}
	}
	if (floor == 2) {
		addFloor2();
		return;
	}
	for (i = 0; i < 3; i++) {
		if (floor_order[i] == 0) {
			floor_order[i] = floor;
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
	if (floor_order[0] != 0) {
		is_target_set = 0;
	} else {
		is_target_set = 1;
	}
}

void handleEvent(PinEvent evt) {
	switch( (int) evt ) {
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
			//	floor_order not empty AND position at floor level+-1
			if (floor_order[0] != 0 && FLOOR_LEVELS[floor_order[0]-1] - 1 <= getCarPosition() && getCarPosition() <= FLOOR_LEVELS[floor_order[0]-1] + 1) {
					is_arriving_at_floor = 1;
			} break;
		case(DOORS_CLOSED): doors_closed = 1; break;
		case(DOORS_OPENING): doors_closed = 0; break; 
			
		// need to add other events?
	}
}


static void plannerTask(void *params) {
	xLastWakeTime = xTaskGetTickCount();
	for(;;){
		if(xQueueReceive(pinEventQueue, &event, WAIT_EVENT_MS) == pdPASS){
			printf("Event recieved: %s, pos: %lu\n", event_str(event), getCarPosition());
			handleEvent(event);
		}
		
		// "req4" check - motor_halts => (AT_FLOOR || STOP_PRESSED)
		// Stop at floor 
		if (floor_order[0] != 0 && getCarPosition() == FLOOR_LEVELS[floor_order[0]-1] ) {
			printf("breaking for floor %d on pos %d\n", floor_order[0], (int) FLOOR_LEVELS[floor_order[0]-1]);
			setCarMotorStopped(1);
		}
		
		
		// "req5" check - Once MOTOR_STOPPED and AT_FLOOR start counting to 1 sec 
		// if floor reached , start counter (xStoppedAt)
		if (MOTOR_STOPPED && is_arriving_at_floor) {
			removeFloor();
			print_floor_order();
			xStoppedAt = xTaskGetTickCount();
			is_arriving_at_floor = 0;
		}
		
		// get counter value and proceed if greater than 1s 
		xTimeAtFloor = xTaskGetTickCount()-xStoppedAt; 
		if(is_stopped != 1 && is_target_set != 1 && xTimeAtFloor/portTICK_RATE_MS > WAIT_FLOOR_MS && doors_closed) {
			printf("Time at floor %lu \n", xTimeAtFloor);
			if (floor_order[0] != 0) {
				setCarMotorStopped(0);
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

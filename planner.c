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
#define WAIT_FLOOR (1000/portTICK_RATE_MS)

int i = 0;
PinEvent event;
static portTickType xLastWakeTime;
static portTickType StoppedAt;


int stop_state, is_target_set = 0;
int floor_arr[] = {0,0,0};
s16 time_at_floor = 0; 
s32 FLOOR_LEVELS[3] = {FLOOR_1_POS, FLOOR_2_POS, FLOOR_3_POS};



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

void print_floor_arr() {
	printf("{");
	for (i = 0; i < 3; i++) {
		printf(" %d ", floor_arr[i]);
	}
	printf("}\n");
}

/*
void addFloor(int floor) {
	floor_arr[0] = floor;
	is_target_set = 0; 
}*/

void removeFloor () {
	floor_arr[0] = floor_arr[1];
	floor_arr[1] = floor_arr[2];
	is_target_set = 0;
}

static void plannerTask(void *params) {
	xLastWakeTime = xTaskGetTickCount();
	for(;;){
		xQueueReceive(pinEventQueue, &event, portMAX_DELAY);
		printf("Event recieved: %s, pos: %lu\n", event_str(event), getCarPosition());
		switch( event ) {
			case(STOP_PRESSED) : 
				stop_state = 1; setCarMotorStopped(1); break;
			case(STOP_RELEASED): stop_state = 0; break;
			case(TO_FLOOR_1): addFloor(1); print_floor_arr(); break;
			case(TO_FLOOR_2): addFloor(2); print_floor_arr(); break;
			case(TO_FLOOR_3): addFloor(3); print_floor_arr(); break;
			case(ARRIVED_AT_FLOOR): 
				if (floor_arr[0] != 0 && 
				FLOOR_LEVELS[floor_arr[0]-1] - 1 <= getCarPosition() && 
				getCarPosition() <= FLOOR_LEVELS[floor_arr[0]-1] + 1) {
						removeFloor();
						time_at_floor = 0;
						print_floor_arr();
						StoppedAt = xTaskGetTickCount();
					printf("StoppedAt: %lu \n", StoppedAt);
				} break;
			// need to add other events
		}
		if(stop_state != 1 && is_target_set != 1) {
			switch(floor_arr[0]) {
				case(1) : 
					vTaskDelayUntil(&StoppedAt, WAIT_FLOOR);
				  printf("Stopped Until : %lu \n" ,xTaskGetTickCount());
					setCarTargetPosition(FLOOR_1_POS);
					is_target_set = 1;
					break;
				case(2) : 
					vTaskDelayUntil(&StoppedAt, WAIT_FLOOR);
					printf("Stopped Until : %lu \n" ,xTaskGetTickCount());
					setCarTargetPosition(FLOOR_2_POS);
					is_target_set = 1;
					break;
				case(3) : 
					vTaskDelayUntil(&StoppedAt, WAIT_FLOOR);
					printf("Stopped Until : %lu \n" ,xTaskGetTickCount());
					setCarTargetPosition(FLOOR_3_POS);
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

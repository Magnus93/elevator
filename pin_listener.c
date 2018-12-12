/**
 * Program skeleton for the course "Programming embedded systems"
 *
 * Lab 1: the elevator control system
 */

/**
 * Functions listening for changes of specified pins
 */

#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include "pin_listener.h"
#include "assert.h"

u16 count;

typedef enum {
	OFFOFF,
	OFFON,
	ONOFF,
	ONON
} debounce_state;


debounce_state nextState(debounce_state current, int value) {
	switch(current){
		case(OFFOFF):
			return value ? OFFON : OFFOFF;
		case(OFFON):
			return value ? ONON : OFFOFF;
		case(ONON):
			return value ? ONON : ONOFF;
		default:
			return value ? ONON : OFFOFF;	
	}
}

// States for events : unknown floor_1 floor_2 floor_3 Stop_pres arrived_at_floor left_floor doors_close doors_open stop_press stop_release
debounce_state eventstatus[10] = {OFFOFF, OFFOFF, OFFOFF, OFFOFF, OFFOFF, OFFOFF, OFFOFF, OFFOFF, OFFOFF};
debounce_state pre_value; 

void print_event(PinEvent evt) {
	switch (evt) {
		case(UNASSIGNED): 			printf("UNASSIGNED \n"); break;
		case(TO_FLOOR_1): 			printf("TO_FLOOR_1 \n"); break;
		case(TO_FLOOR_2): 			printf("TO_FLOOR_2 \n"); break;
		case(TO_FLOOR_3): 			printf("TO_FLOOR_3 \n"); break;
		case(STOP_PRESSED): 		printf("STOP_PRESSED \n"); break;
		case(STOP_RELEASED):		printf("STOP_RELEASED \n"); break;
		case(ARRIVED_AT_FLOOR): printf("ARRIVED_AT_FLOOR \n"); break;
		case(LEFT_FLOOR): 			printf("LEFT_FLOOR\n"); break;
		case(DOORS_CLOSED): 		printf("DOORS_CLOSED \n"); break;
		case(DOORS_OPENING): 		printf("DOORS_OPENING \n"); break;
	}
}

void debounce_rising(PinListener *listener, xQueueHandle pinEventQueue) {
	listener->status = GPIO_ReadInputDataBit(GPIOC, listener->pin);
	pre_value = eventstatus[listener->risingEvent];
	eventstatus[listener->risingEvent] = nextState(eventstatus[listener->risingEvent], listener->status);
	if ((eventstatus[listener->risingEvent] == ONON) && (pre_value != ONON)) {
		if((xQueueSend(pinEventQueue, (void*)&(listener->risingEvent), portMAX_DELAY)) == pdPASS) {
			printf("sent event: ");
			print_event(listener->risingEvent);
		}
	} 
}

void debounce_falling(PinListener *listener, xQueueHandle pinEventQueue) {
	listener->status = GPIO_ReadInputDataBit(GPIOC, listener->pin);
	pre_value = eventstatus[listener->fallingEvent];
	eventstatus[listener->fallingEvent] = nextState(eventstatus[listener->fallingEvent], ! listener->status);
	if ( eventstatus[listener->fallingEvent] == ONON && pre_value != ONON && listener->fallingEvent != UNASSIGNED ) {
		if (xQueueSend(pinEventQueue, &(listener->fallingEvent), portMAX_DELAY) == pdPASS) {
			printf("sent event: ");
			print_event(listener->fallingEvent);
		}
	}
	
}

static void pollPin(PinListener *listener,
                    xQueueHandle pinEventQueue) {
											
	switch(listener->pin) {
		case(GPIO_Pin_0):		// Check to floor 1
			debounce_rising(listener, pinEventQueue);
			break;
		case(GPIO_Pin_1):		// Check to floor 2
			debounce_rising(listener, pinEventQueue);
			break;
		case(GPIO_Pin_2):		// Check to floor 3
			debounce_rising(listener, pinEventQueue);
			break;
		case(GPIO_Pin_3):   // Check stop pressed
			debounce_rising(listener, pinEventQueue);
			debounce_falling(listener, pinEventQueue);
			break;
		case(GPIO_Pin_7):   // Check floor sensor
			debounce_rising(listener, pinEventQueue);
			debounce_falling(listener, pinEventQueue);
			break;
		case(GPIO_Pin_8):   // Check position sensor
			debounce_rising(listener, pinEventQueue);
			debounce_falling(listener, pinEventQueue);
			break;			
	}
}



static void pollPinsTask(void *params) {
  PinListenerSet listeners = *((PinListenerSet*)params);
  portTickType xLastWakeTime;
  int i;
  xLastWakeTime = xTaskGetTickCount();

  for (;;) {
    for (i = 0; i < listeners.num; ++i) {
			pollPin(listeners.listeners + i, listeners.pinEventQueue);
		}   
		
		vTaskDelayUntil(&xLastWakeTime, listeners.pollingPeriod);
  }
}

void setupPinListeners(PinListenerSet *listenerSet) {
  portBASE_TYPE res;

  res = xTaskCreate(pollPinsTask, "pin polling",
                    100, (void*)listenerSet,
					listenerSet->uxPriority, NULL);
  assert(res == pdTRUE);
}

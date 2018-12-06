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

// 					States for events : floor_1,floor_2,floor_3,Stop_pres,stop_rel,at_floor,left_floor,doors_close,doors_open  
debounce_state eventstatus[10] = {OFFOFF, OFFOFF, OFFOFF, OFFOFF, OFFOFF, OFFOFF, OFFOFF, OFFOFF, OFFOFF};
debounce_state pre_value; 

void debounce_rising(PinListener *listener, xQueueHandle pinEventQueue) {
	listener->status = GPIO_ReadInputDataBit(GPIOC, listener->pin);
	pre_value = eventstatus[listener->risingEvent];
	eventstatus[listener->risingEvent] = nextState(eventstatus[listener->risingEvent], listener->status);
	if (eventstatus[listener->risingEvent] == ONON && pre_value != ONON) {
		xQueueSend(pinEventQueue, &(listener->risingEvent), portMAX_DELAY); 
	} 
	printf("reise : %d \n", listener->risingEvent);
}

void debounce_falling(PinListener *listener, xQueueHandle pinEventQueue) {
	listener->status = GPIO_ReadInputDataBit(GPIOC, listener->pin);
	pre_value = eventstatus[listener->fallingEvent];
	eventstatus[listener->fallingEvent] = nextState(eventstatus[listener->fallingEvent], ! listener->status);
	if ( eventstatus[listener->fallingEvent] == ONON && pre_value != ONON && listener->fallingEvent != UNASSIGNED ) {
		xQueueSend(pinEventQueue, &(listener->fallingEvent), portMAX_DELAY);
	}
	printf("fall : %d \n", listener->fallingEvent);
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

/*
static void pollPin(PinListener *listener,
                    xQueueHandle pinEventQueue) {
	if (listener->pin == GPIO_Pin_0 && listener->risingEvent == TO_FLOOR_1) { // check floor 1 button
		listener->status = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_0);
		if (listener->status) {
			printf("Pin 0 is %d \n", listener->status);
		}
	} else 	if (listener->pin == GPIO_Pin_1) { // check floor 2 button
		listener->status = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_1);
		if (listener->status) {
			printf("Pin 1 is %d \n", listener->status);
		}
	} else 	if (listener->pin == GPIO_Pin_2) { // check floor 2 button
		listener->status = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_2);
		if (listener->status) {
			printf("Pin 2 is %d \n", listener->status);
		}
	} else 	if (listener->pin == GPIO_Pin_3) { // check stop button
		listener->status = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_3);
		if (listener->status) {
			printf("Pin 3 is %d \n", listener->status);
		}
	} else 	if (listener->pin == GPIO_Pin_7) { // check at floor sensor
		listener->status = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_7);
	} else 	if (listener->pin == GPIO_Pin_8) { // check door sensor 
		listener->status = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_8);
	}
}*/

static void pollPinsTask(void *params) {
  PinListenerSet listeners = *((PinListenerSet*)params);
  portTickType xLastWakeTime;
  int i;
	portBASE_TYPE result;
	void *event;
  xLastWakeTime = xTaskGetTickCount();

  for (;;) {
    for (i = 0; i < listeners.num; ++i) {
			pollPin(listeners.listeners + i, listeners.pinEventQueue);
		}
		result = xQueueReceive(pinEventQueue, event, portMAX_DELAY );
		if (result == pdPASS){
			switch((int) event) {
				case (TO_FLOOR_1):
					printf("TO_FLOOR_1\n");
					break;
				case (TO_FLOOR_2):
					printf("TO_FLOOR_2\n");
					break;
				default:
					printf("Event: %d", (int) event);
			}
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

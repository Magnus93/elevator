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


static void pollPin(PinListener *listener,
                    xQueueHandle pinEventQueue) {
	if (listener->pin == GPIO_Pin_0) { // check floor 1 button
		listener->status = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_0);
		printf("floor 1 button pressed");
	} else 	if (listener->pin == GPIO_Pin_1) { // check floor 2 button
		listener->status = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_1);
	} else 	if (listener->pin == GPIO_Pin_2) { // check floor 2 button
		listener->status = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_2);
	} else 	if (listener->pin == GPIO_Pin_3) { // check stop button
		listener->status = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_3);
	} else 	if (listener->pin == GPIO_Pin_7) { // check at floor sensor
		listener->status = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_7);
	} else 	if (listener->pin == GPIO_Pin_8) { // check door sensor 
		listener->status = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_8);
	}
	

}

static void pollPinsTask(void *params) {
  PinListenerSet listeners = *((PinListenerSet*)params);
  portTickType xLastWakeTime;
  int i;

  xLastWakeTime = xTaskGetTickCount();

  for (;;) {
    for (i = 0; i < listeners.num; ++i)
	  pollPin(listeners.listeners + i, listeners.pinEventQueue);
    
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

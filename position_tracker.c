/**
 * Program skeleton for the course "Programming embedded systems"
 *
 * Lab 1: the elevator control system
 */

/**
 * Class for keeping track of the car position.
 */

#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include "position_tracker.h"

#include "assert.h"


static void positionTrackerTask(void *params) {
	PositionTracker *tracker = (PositionTracker *) params; 
	int lastPinValue;
	int pinValue;
	for(;;) {
		pinValue = GPIO_ReadInputDataBit(tracker->gpio, tracker->pin);
		// Detect a change in the pinvalue only if position sensor moves from white to black (assuming 1 is black and 0 is white)
		if(pinValue != lastPinValue && pinValue == 1) {   
			if(tracker->direction == Up) {  // if the elevator direction is upward increment the position value
				tracker->position++;
			} else if(tracker->direction == Down) { // if the elevator direction is downward decrement the position value
				tracker->position--;
			}
			printf("curret pos: %lu\n", tracker->position);
		}
		lastPinValue = pinValue; // remember current pin value for next iteration 
		vTaskDelay(tracker->pollingPeriod);
	}
}

void setupPositionTracker(PositionTracker *tracker,
                          GPIO_TypeDef * gpio, u16 pin,
						  portTickType pollingPeriod,
						  unsigned portBASE_TYPE uxPriority) {
  portBASE_TYPE res;

  tracker->position = 0;
  tracker->lock = xSemaphoreCreateMutex();
  assert(tracker->lock != NULL);
  tracker->direction = Unknown;
  tracker->gpio = gpio;
  tracker->pin = pin;
  tracker->pollingPeriod = pollingPeriod;

  res = xTaskCreate(positionTrackerTask, "position tracker",
                    80, (void*)tracker, uxPriority, NULL);
  assert(res == pdTRUE);
}

void setDirection(PositionTracker *tracker, Direction dir) {
	tracker->direction = dir;
}

s32 getPosition(PositionTracker *tracker) {
	return tracker->position;
}


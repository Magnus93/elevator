/**
 * Program skeleton for the course "Programming embedded systems"
 *
 * Lab 1: the elevator control system
 */

/**
 * This file defines the safety module, which observes the running
 * elevator system and is able to stop the elevator in critical
 * situations
 */

#include "FreeRTOS.h"
#include "task.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_gpio.h"
#include <stdio.h>

#include "global.h"
#include "assert.h"

#define ABS(a) (a>=0 ? a : -a)
#define POLL_TIME (10 / portTICK_RATE_MS)
#define BUFF_SIZE 50

#define MOTOR_UPWARD   (TIM3->CCR1)
#define MOTOR_DOWNWARD (TIM3->CCR2)
#define MOTOR_STOPPED  (!MOTOR_UPWARD && !MOTOR_DOWNWARD)
#define MAX_SPEED 50


#define STOP_PRESSED  GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_3)
#define AT_FLOOR      GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_7)
#define DOORS_CLOSED  GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_8)

static portTickType xLastWakeTime;

s32 position_buffer[BUFF_SIZE];
int pos_head_index = 0;// head of the position buffer
s32 prev_pos, pos;
int speed;
s32 prev_time_ms = 500;

static void check(u8 assertion, char *name) {
  if (!assertion) {
    printf("SAFETY REQUIREMENT %s VIOLATED: STOPPING ELEVATOR\n", name);
    for (;;) {
	  setCarMotorStopped(1);
  	  vTaskDelayUntil(&xLastWakeTime, POLL_TIME);
    }
  }
}

static void safetyTask(void *params) {
  s16 timeSinceStopPressed = -1;
	s16 timeSinceAtFloor = -1;
  xLastWakeTime = xTaskGetTickCount();
	
	
  for (;;) {
		
		// Setting up variables to calculate speed
		
		pos_head_index = (pos_head_index+1) % BUFF_SIZE;			// roll index
		position_buffer[pos_head_index] = getCarPosition();   // set head of buffer
		pos = position_buffer[(pos_head_index)% BUFF_SIZE];					// get current position
		prev_pos = position_buffer[(pos_head_index+1)% BUFF_SIZE];    // the position 200ms ago
		speed = (ABS(pos-prev_pos)*1000/prev_time_ms);
		
    // Environment assumption 1: the doors can only be opened if
		//                           the elevator is at a floor and
    //                           the motor is not active
		check((AT_FLOOR && MOTOR_STOPPED) || DOORS_CLOSED, "env1");

		
		// Environment assumption 2 : the elevator moves at a maximum speed of 50 cm/s
		if (! (speed <= MAX_SPEED)) {
			printf("speed: %d cm/s \n", speed);
		}
		check(speed <= MAX_SPEED, "env2");
		
		
		// fill in environment assumption 3
		check(1, "env3");

		// fill in your own environment assumption 4
		// Environment Assumption 4 : Check that the AT_FLOOR sensor 
		// 														and position sensor are in agreement 
		check((0<=pos && pos<=1) || (399<=pos && pos<=401) || (799<=pos && pos<=800) || !AT_FLOOR, "env4");

    // Safety requirement 1: if the stop button is pressed, the motor is
	//                       stopped within 1s

	if (STOP_PRESSED) {
	  if (timeSinceStopPressed < 0)
	    timeSinceStopPressed = 0;
    else
	    timeSinceStopPressed += POLL_TIME;

      check(timeSinceStopPressed * portTICK_RATE_MS <= 1000 || MOTOR_STOPPED,
	        "req1");
	} else {
	  timeSinceStopPressed = -1;
	}

    // Safety requirement 2: the motor signals for upwards and downwards
	//                       movement are not active at the same time

    check(!MOTOR_UPWARD || !MOTOR_DOWNWARD,
          "req2");

	// Safety requirement 3 : the elevator may not pass the end positions,
	//												that is, go through the roof or the floor
	check(0 <= pos && pos <= 800, "req3");

	// Safety requirement 4 : a moving elevator halts only if the stop button
	//												is pressed or the elevator has arrived at a floor
	check(!getCarMotorStopped() || (AT_FLOOR || STOP_PRESSED), "req4");
		
	// Safety requirement 5 : once the elevator has stopped at a floor, it will
	//												wait for at least 1 s before it continues to another 
	//												floor
	if (AT_FLOOR) {
		printf("AT_FLOOR\n");
	}
	
	if(AT_FLOOR){
		if(timeSinceAtFloor < 0) {
			timeSinceAtFloor = 0;
		} else {
			timeSinceAtFloor += POLL_TIME;
		}
	} else if(timeSinceAtFloor > 0) {
		check(timeSinceAtFloor* portTICK_RATE_MS >= 1000, "req5");
		timeSinceAtFloor = -1;
	}
	
	// fill in safety requirement 6
	check(1, "req6");

	// fill in safety requirement 7
	check(1, "req7");

	vTaskDelayUntil(&xLastWakeTime, POLL_TIME);
  }

}

void setupSafety(unsigned portBASE_TYPE uxPriority) {
  xTaskCreate(safetyTask, "safety", 100, NULL, uxPriority, NULL);
}

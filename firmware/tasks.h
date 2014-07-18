/*
 *                  Copyright 2014 ARTaylor.co.uk
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Author: Richard Taylor (richard@artaylor.co.uk)
 */
#ifndef _TASKS_H
#define _TASKS_H

 /* 
  * Module to provide very simple round robin task scheduler.
  */

#include <stdint.h>
extern volatile uint32_t system_ticks;

typedef enum
{
	TASK_PROCESS_KEYPAD,
	TASK_PROCESS_STICKS,
	TASK_PROCESS_GUI,
	TASK_END
} Tasks;

void task_init(void);
void task_register(Tasks task, void (*fn)(uint32_t));
void task_schedule(Tasks task, uint32_t data, uint32_t time_ms);
void task_process_all(void);

// Utility functions (implemented in main.c)
void delay_ms(uint32_t delay);
void delay_us(uint32_t delay);

#endif // _TASKS_H

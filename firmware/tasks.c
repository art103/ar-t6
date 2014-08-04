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

/* Description:
 *
 * This is a simple round robin, cooperative task scheduler.
 * non-critical tasks are run from the main loop in sequence.
 *
 */

#include "stm32f10x.h"
#include "tasks.h"

volatile uint32_t system_ticks = 0;

static uint32_t tasks[TASK_END];
static uint32_t task_data[TASK_END];
static void (*task_fn[TASK_END])(uint32_t);

/**
  * @brief  Loop to process scheduled tasks.
  * @note   Tasks are run round robin in numerical order.
  * @param  None
  * @retval None
  */
void task_init(void)
{
	int i;
	for (i=0; i<TASK_END; ++i)
		task_fn[i] = 0;
}

/**
  * @brief  Register a task function.
  * @note   
  * @param  task: ID of the task to register.
  * @param  fn: Function pointer for the task ID.
  * @retval None
  */
void task_register(Tasks task, void (*fn)(uint32_t))
{
	task_fn[task] = fn;
	tasks[task] = FALSE;
}

/**
  * @brief  Schedule a task to run.
  * @note
  * @param  task: ID of the task to schedule.
  * @param  data: Data to pass to the task function.
  * @param  time_ms: When to schedule the task in ms from now (0 for ASAP).
  * @retval None
  */
void task_schedule(Tasks task, uint32_t data, uint32_t time_ms)
{
	if (tasks[task] == 0 || tasks[task] > system_ticks + time_ms)
	{
		tasks[task]  = system_ticks + time_ms;
		task_data[task] = data;
	}
}

/**
  * @brief  Stop a scheduled task from running.
  * @note
  * @param  task: ID of the task to deschedule.
  * @retval None
  */
void task_deschedule(Tasks task)
{
	tasks[task] = 0;
}

/**
  * @brief  Loop to process scheduled tasks.
  * @note   Tasks are run round robin in numerical order.
  * @param  None
  * @retval None
  */
void task_process_all(void)
{
	int task;
	
	/* Run all scheduled tasks */
	for (task = 0; task < TASK_END; ++task)
	{
		if (tasks[task] != 0 && tasks[task] <= system_ticks)
		{
			if (task_fn[task] != 0)
			{
				tasks[task] = 0;
				task_fn[task](task_data[task]);
			}
		}
	}
}

/**
  * @brief  Delay timer using the system tick timer
  * @param  delay: delay in ms.
  * @retval None
  */
void delay_ms(uint32_t delay)
{
	uint32_t start = system_ticks;
	while (system_ticks <  start + delay);
}

/**
  * @brief  Spin loop us delay routine
  * @param  delay: delay in us.
  * @retval None
  */
void delay_us(uint32_t delay)
{
	volatile uint32_t i;
	for (i=0; i < delay * 3; ++i);
}


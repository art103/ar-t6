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

#include "system.h"
#include "tasks.h"
#include "string.h"

static uint32_t tasks[TASK_END];
static uint32_t task_data[TASK_END];
static void (*task_fn[TASK_END])(uint32_t);

/**
  * @brief  Init tasks.
  * @note   None
  * @param  None
  * @retval None
  */
void task_init(void)
{
	memset( tasks, 0, sizeof(tasks) );
	memset( task_data, 0, sizeof(task_data) );
	memset( task_fn, 0, sizeof(task_fn) );
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
	tasks[task] = 0;
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
		task_data[task] = data;
		tasks[task]  = system_ticks + time_ms;
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
static int task;
void task_process_all(void)
{
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

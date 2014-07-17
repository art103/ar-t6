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

#include "stm32f10x.h"
#include "tasks.h"

static bool tasks[TASK_END];
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
  * @param  task: ID of the task to register.
  * @param  data: Data to pass to the task function.
  * @retval None
  */
void task_schedule(Tasks task, uint32_t data)
{
	tasks[task] = TRUE;
	task_data[task] = data;
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
		if (tasks[task] == TRUE)
		{
			if (task_fn[task] != 0)
			{
				task_fn[task](task_data[task]);
				tasks[task] = FALSE;
			}
		}
	}
}

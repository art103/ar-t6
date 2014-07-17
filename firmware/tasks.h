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
 
 /* 
  * Module to provide very simple round robin task scheduler.
  */

#include <stdint.h>

typedef enum
{
	TASK_PROCESS_KEYPAD,
	TASK_END
} Tasks;

/**
  * @brief  Initialize task data.
  * @note   
  * @param  None
  * @retval None
  */
void task_init(void);

/**
  * @brief  Register a task function.
  * @note   
  * @param  task: ID of the task to register.
  * @param  fn: Function pointer for the task ID.
  * @retval None
  */
void task_register(Tasks task, void (*fn)(uint32_t));

/**
  * @brief  Schedule a task to run.
  * @note
  * @param  task: ID of the task to register.
  * @param  data: Data to pass to the task function.
  * @retval None
  */
void task_schedule(Tasks task, uint32_t data);

/**
  * @brief  Loop to process scheduled tasks.
  * @note   Tasks are run round robin in numerical order.
  * @param  None
  * @retval None
  */
void task_process_all(void);

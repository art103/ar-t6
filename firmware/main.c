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

#include "tasks.h"


/**
  * @brief  Main Loop for non-IRQ based work
  * @note   Deals with init and non time critical work.
  * @param  None
  * @retval None
  */
void main(void)
{
	// PLL and stack setup has aready been done.

	// Initialize the task loop.
	task_init();

	// Initialize the keypad scanner (with IRQ wakeup)
	keypad_init();

	while (1)
	{
		task_process_all();
	}
}

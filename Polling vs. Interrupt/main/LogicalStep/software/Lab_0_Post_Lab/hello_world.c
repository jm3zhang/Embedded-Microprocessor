/*
 * "Hello World" example.
 *
 * This example prints 'Hello from Nios II' to the STDOUT stream. It runs on
 * the Nios II 'standard', 'full_featured', 'fast', and 'low_cost' example
 * designs. It runs with or without the MicroC/OS-II RTOS and requires a STDOUT
 * device in your system's hardware.
 * The memory footprint of this hosted application is ~69 kbytes by default
 * using the standard reference design.
 *
 * For a reduced footprint version of this template, and an explanation of how
 * to reduce the memory footprint for a given application, see the
 * "small_hello_world" template.
 *
 */

#include <stdio.h>
#include "system.h"
#include "altera_avalon_pio_regs.h"


int main()
{

	//This does not work....
	unsigned char buttons;
	unsigned char switches;

	do {
		//in a loop fashion
	  //read values from the push button pio core
		  //Reading push buttons(active low)
		buttons = IORD(BUTTON_PIO_BASE,0);

	  //read values from the switches pio core
		  //Reading push buttons(active high)
		switches = IORD(SWITCH_PIO_BASE,0);

	}while(buttons == 1 || switches == 0);

  //Make the Push-Button PIO core bits (3:0) appear on LED (3:0).
	  //Turn on every other LED on the board(active high)
	  IOWR(LED_PIO_BASE,0x0,buttons);

  //Make Switches PIO Core bits (3:0) appear on LEDs (7:4).
	  //Turn on every other LED on the board(active high)
	  IOWR(LED_PIO_BASE,0x3,switches);

	  return 0;
}

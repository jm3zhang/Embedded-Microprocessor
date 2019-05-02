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
#include "sys/alt_irq.h"

//#define Interrupt

//Background Tasks
int background()
{
	int j;
	int x = 0;
	int grainsize = 4;
	int g_taskProcessed = 0;

	for(j = 0; j < grainsize; j++)
	{
		g_taskProcessed++;
	}
	return x;
}

//Interrupt Service Routine
static void EGM_ISR(void* context, alt_u32 id) {

	//Writes rising then falling edge to create a response
	IOWR(RESPONSE_OUT_BASE, 0, 1);
	IOWR(RESPONSE_OUT_BASE, 0, 0);

	//Command to clear the interrupt goes at the end of the ISR
	IOWR(STIMULUS_IN_BASE, 3, 0);
}


//MAIN
int main()
{
#ifdef Interrupt

	printf("This the Interrupt mode\n");

	while(1)
	{
		//Wait for button press
		if(IORD(BUTTON_PIO_BASE,0) == 14)
		{
			//Initialize registers and variables
			IOWR(LED_PIO_BASE,0,0);
			int period=0;
			int dutyCycle = 0;
			int latency  = 0;
			int missed_pulses = 0;
			int total_tasks = 0;


			// Interrupt Mode
			for (period=2, dutyCycle = 1;period<=5000;period += 2, dutyCycle ++)
			{
				alt_irq_register(STIMULUS_IN_IRQ, (void *)0, EGM_ISR);

				//Enables the interrupt bit to allow for ISR
				IOWR(STIMULUS_IN_BASE, 2, 0x1);

				//Setting period and pulse width
				IOWR(EGM_BASE, 2, period);
				IOWR(EGM_BASE, 3, dutyCycle);
				int background_tasks = 0;
				total_tasks = 0;

				//Write to the enable to the BASE
				IOWR(EGM_BASE, 0, 1);

				//Checks EGM busy bit
				while(IORD(EGM_BASE, 1))
				{
					IOWR(LED_PIO_BASE,0,1);
					background();
					IOWR(LED_PIO_BASE,0,0);
					background_tasks++;
					total_tasks++;
				}

				int latency  = IORD(EGM_BASE, 4);
				int missed_pulses = IORD(EGM_BASE, 5);

				//printf("Period: %d, Duty Cycle: %d, Latency: %d, Missed Pulses: %d, Background Tasks: %d\n", period, dutyCycle, latency, missed_pulses, background_tasks);
				printf("%d ,%d\n", period, missed_pulses);

				//Disable EGM
				IOWR(EGM_BASE, 0, 0);

			}
			break;
		}
	}

	//Printout MODE and wait for PB0 to start
	//Execute code for chosen Mode

	//Tight Polling Mode
	/*
	 * code
	*/
#else
	printf("This the Tight Polling mode\n");
	while(1)
	{
		//Wait for button press
		if(IORD(BUTTON_PIO_BASE,0) == 14)
		{
			//Initialize variables
			int period = 0;
			int dutyCycle = 0;
			int previousFrame = 0;
			int currentFrame = 0;
			int risingEdgeCounter = 0;
			int background_tasks = 0;
			int characterization_cycle_counter = 0;
			int total_tasks = 0;

			for (period=2, dutyCycle = 1;period<=5000;period += 2, dutyCycle ++)
			{
				previousFrame = 0;
				risingEdgeCounter = 0;
				background_tasks = 0;
				total_tasks = 0;
				IOWR(EGM_BASE, 2, period);
				IOWR(EGM_BASE, 3, dutyCycle);
				IOWR(EGM_BASE, 0, 0);
				IOWR(EGM_BASE, 0, 1);

				//Reads if EGM is busy
				while(IORD(EGM_BASE, 1))
				{
					//Detecting rising edge
					currentFrame = IORD(STIMULUS_IN_BASE, 0);
					//charcterization_cycle = 0;
					if (previousFrame == 0 && currentFrame == 1){
						//printf("Entered the 1st if\n");
						IOWR(RESPONSE_OUT_BASE, 0, 1);
						IOWR(RESPONSE_OUT_BASE, 0, 0);
						risingEdgeCounter ++;
						characterization_cycle_counter = 0;
						//printf("risingEdgeCounter: %d\n", risingEdgeCounter);
					}
					previousFrame = currentFrame;

					//Characterization
					if (risingEdgeCounter == 1)
					{
						//printf("Entered the 2nd if\n");
						IOWR(LED_PIO_BASE,0,1);
						background();
						IOWR(LED_PIO_BASE,0,0);
						background_tasks++;
						total_tasks ++;
						//Checks busy bit of EGM
						if(IORD(EGM_BASE, 1)==0){
							break;
						}
						//printf("Tasks: %d\n", background_tasks);
						//continue;
					}
					//Cycles after characterization
					else //(risingEdgeCounter != 1 && risingEdgeCounter != 0)
					{
						if (characterization_cycle_counter < background_tasks - 1 ){
							//printf("Cycles: %d\n", characterization_cycle_counter);
							//printf("Tasks: %d\n", background_tasks);
							IOWR(LED_PIO_BASE,0,1);
							background();
							IOWR(LED_PIO_BASE,0,0);
							total_tasks ++;
							if(IORD(EGM_BASE, 1)==0){
								break;
							}
							characterization_cycle_counter++;
						}
					}
				}

				int latency  = IORD(EGM_BASE, 4);
				int missed_pulses = IORD(EGM_BASE, 5);
				//printf("Period: %d, Duty Cycle: %d, Latency: %d, Missed Pulses: %d, Total Background Tasks of one cycle: %d\n", period, dutyCycle, latency, missed_pulses, total_tasks);
				printf("%d ,%d\n", period, missed_pulses);
				IOWR(EGM_BASE, 0, 0);
			}
			break;
		}
	}


	//Do not do any printf calls until after each test is completed
	//Print out test results
	//If EGM is still running then update PERIOD/Duty Cycle parameters and do next test run unless PERIOD equals limit
#endif
	return 0;
}

/**
 * @file interrupt.c 
 * 
 * Alle necessary interrupt routines. Please check the source code of
 * this file for more documentation.
 */

#include <avr/interrupt.h>
#include "abstraction.h"
#include "system.h"

extern smartie_sorter ss;

/**
 * \brief Interrupt routine, executed every millisecond
 */
ISR (TIMER0_COMP_vect) {

	/* color sensor TCS stuff first, needs to be accurate */
	sensor_tcs_stuff();

	/* stepper motor stuff */
	motor_stuff();
	
	/* user input (rotary encoder) stuff */
	rotary_encoder_stuff();
	
	/* light barrier stuff */
	lightbarrier_stuff ();
	
	/* color sensor ADJD stuff */
	sensor_adjd_stuff();

	/* vibrator stuff */
	vibrator_stuff();
		
}

/**
 * \brief Interupt routine for measuring frequency of color sensor TCS OUT pin
 */
ISR (INT0_vect) {
	ss.sens_tcs.slopes++;
}
